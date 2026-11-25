/*
 * JZ4760B 330/339 MHz AUX/VPU XBurst+MXU Coprocessor S-DSP Offload Backend
 * Double-buffered zero-wait-state execution in TCSM0 (Slot 0) + upper TCSM1 (Slot 1)
 * + persistent 8-ch BRR & 4-point Sony Hardware Gaussian state in SRAM (0x132D0000).
 */
#include <libjzaux.h>
#include "/home/sean/retrofw-aux/firmware/snes_dsp_proto.h"
#include "/home/sean/retrofw-aux/firmware/aux_snes_dsp_fw.h"

extern "C" void sal_AudioCommitPcm(const int16_t *pcm, uint32_t frames);

static jz_aux_ctx_t *g_snes_aux_ctx = NULL;
static volatile struct aux_snes_sram *g_snes_aux_sram = NULL;
static volatile struct aux_snes_slot *g_snes_aux_slots[2] = { NULL, NULL };

struct aux_snes_inflight {
	uint32_t n_pairs;
	uint32_t stereo;
	uint32_t echo_on;
	int32_t  echo_ptr;
};

static struct aux_snes_inflight g_snes_inflight[2];
static uint32_t g_snes_submit_seq = 0;
static uint32_t g_snes_reap_seq = 0;
static int      g_snes_aux_active = 0;
static uint32_t g_snes_ch_reset_mask = 0xFFU;
static uint32_t g_snes_ch_sync_mask  = 0xFFU;
static int      g_snes_echo_sync_host = 1;
static uint32_t g_snes_prof_jobs = 0;
static uint32_t g_snes_prof_fallbacks = 0;
static uint32_t g_snes_prof_waits = 0;

static uint32_t snes_calc_erate(int state, unsigned long rate)
{
	static const int64 steps[10] = {
		0,
		(int64) FIXED_POINT * 1000 * 64,
		(int64) FIXED_POINT * 1000 * 619,
		(int64) FIXED_POINT * 1000 * 619,
		(int64) FIXED_POINT * 1000 * 128,
		(int64) FIXED_POINT * 1000 * 1,
		(int64) FIXED_POINT * 1000 * 64,
		(int64) FIXED_POINT * 1000 * 55,
		(int64) FIXED_POINT * 1000 * 64,
		(int64) FIXED_POINT * 1000 * 619
	};
	if (rate == 0 || rate == ~0UL || so.playback_rate == 0 || state <= 0 || state >= 10)
		return 0;
	return (uint32_t)(steps[state] / ((int64)rate * (int64)so.playback_rate));
}

static void snes_jzaux_copy_brr9(volatile uint8_t *dst12, const uint8_t *src9)
{
	/* Aligned 32-bit stores into uncached TCSM */
	volatile uint32_t *d32 = (volatile uint32_t *)dst12;
	uint32_t w0 = (uint32_t)src9[0] | ((uint32_t)src9[1] << 8)
	            | ((uint32_t)src9[2] << 16) | ((uint32_t)src9[3] << 24);
	uint32_t w1 = (uint32_t)src9[4] | ((uint32_t)src9[5] << 8)
	            | ((uint32_t)src9[6] << 16) | ((uint32_t)src9[7] << 24);
	uint32_t w2 = (uint32_t)src9[8];
	d32[0] = w0;
	d32[1] = w1;
	d32[2] = w2;
}

static void snes_jzaux_reap_one(void)
{
	uint32_t seq = g_snes_reap_seq;
	uint32_t idx = seq & 1U;
	volatile struct aux_snes_slot *slot = g_snes_aux_slots[idx];
	struct aux_snes_inflight *inf = &g_snes_inflight[idx];
	int timeout = 5000000;
	int waited = 0;

	while (g_snes_aux_sram->done_seq <= seq) {
		waited = 1;
		if (--timeout <= 0) {
			printf("PocketSNES+: WARNING: AUX S-DSP timeout (done=%u want=%u), falling back to main CPU!\n",
			       g_snes_aux_sram->done_seq, seq + 1);
			g_snes_aux_active = 0;
			g_snes_reap_seq = g_snes_submit_seq;
			return;
		}
	}
	if (waited)
		g_snes_prof_waits++;

	uint32_t n_pairs = inf->n_pairs;
	uint32_t sample_count = inf->stereo ? (n_pairs << 1) : n_pairs;

	/* Copy updated Echo delay-line slice back into DDR2 Echo[] */
	if (inf->echo_on && inf->echo_ptr >= 0 &&
	    inf->echo_ptr + (int32_t)sample_count <= 24000) {
		const volatile int32_t *src_e = slot->echo_io;
		int32_t *dst_e = &Echo[inf->echo_ptr];
		for (uint32_t i = 0; i < sample_count; i++)
			dst_e[i] = src_e[i];
	}

	/* Copy final saturated 16-bit PCM into SDL audio ring buffer */
	int16_t local_pcm[AUX_SNES_MAX_PAIRS * 2];
	const volatile uint32_t *src32 = (const volatile uint32_t *)slot->out_pcm;
	uint32_t *dst32 = (uint32_t *)local_pcm;
	uint32_t nwords = (sample_count + 1U) >> 1;
	for (uint32_t i = 0; i < nwords; i++)
		dst32[i] = src32[i];

	sal_AudioCommitPcm(local_pcm, n_pairs);

	g_snes_reap_seq++;
	g_snes_prof_jobs++;
}

static void snes_jzaux_sync_sram_to_host(void)
{
	if (!g_snes_aux_sram)
		return;
	noise_gen = g_snes_aux_sram->noise_gen;
	Z = (unsigned long)g_snes_aux_sram->echo_z;
	for (int k = 0; k < 16; k++)
		Loop[k] = g_snes_aux_sram->echo_loop[k];
	for (int j = 0; j < NUM_CHANNELS; j++) {
		Channel *ch = &SoundData.channels[j];
		const volatile struct aux_snes_ch_persist *pst = &g_snes_aux_sram->ch_state[j];
		for (int k = 0; k < 16; k++)
			ch->decoded[k] = pst->decoded[k];
		ch->previous[0] = pst->previous[0];
		ch->previous[1] = pst->previous[1];
		ch->sample      = pst->sample;
		ch->next_sample = pst->next_sample;
		ch->block       = ch->decoded;
		ch->interpolate = 0;
	}
}

extern "C" void snes_jzaux_reap_ready(void)
{
	if (!g_snes_aux_active)
		return;
	while (g_snes_reap_seq != g_snes_submit_seq && g_snes_aux_sram->done_seq > g_snes_reap_seq)
		snes_jzaux_reap_one();
}

extern "C" void snes_jzaux_flush(void)
{
	if (!g_snes_aux_active)
		return;
	while (g_snes_reap_seq != g_snes_submit_seq && g_snes_aux_active)
		snes_jzaux_reap_one();
	if (g_snes_aux_active)
		snes_jzaux_sync_sram_to_host();
}

extern "C" uint32_t snes_jzaux_inflight_frames(void)
{
	if (!g_snes_aux_active)
		return 0;
	uint32_t total = 0;
	for (uint32_t s = g_snes_reap_seq; s != g_snes_submit_seq; s++)
		total += g_snes_inflight[s & 1U].n_pairs;
	return total;
}

static void snes_jzaux_mark_host_dirty(void)
{
	if (g_snes_aux_active)
		snes_jzaux_flush();
	g_snes_ch_sync_mask = 0xFFU;
	g_snes_echo_sync_host = 1;
}

static void snes_jzaux_mark_ch_reset(int ch)
{
	if (ch >= 0 && ch < NUM_CHANNELS)
		g_snes_ch_reset_mask |= (1U << ch);
}

/*
 * Fast O(events) host envelope stepper matching MixStereo bit-for-bit.
 * Returns number of output pairs before envelope reaches 0 (or n_pairs if still active).
 */
static uint32_t snes_host_step_envelope(Channel *ch, uint32_t n_pairs, int *out_ended)
{
	uint32_t rem = n_pairs;
	uint32_t done = 0;
	*out_ended = 0;

	while (rem > 0) {
		uint32_t erate = (uint32_t)ch->erate;
		if (erate == 0) {
			done += rem;
			break;
		}
		uint32_t need = (ch->env_error < FIXED_POINT) ? (FIXED_POINT - (uint32_t)ch->env_error) : 1U;
		uint32_t pairs_to_tick = (need + erate - 1U) / erate;
		if (pairs_to_tick == 0)
			pairs_to_tick = 1U;

		if (pairs_to_tick > rem) {
			ch->env_error += (unsigned long)((uint64_t)erate * rem);
			done += rem;
			break;
		}

		ch->env_error += (unsigned long)((uint64_t)erate * pairs_to_tick);
		done += pairs_to_tick;
		rem -= pairs_to_tick;

		uint32_t step = (uint32_t)(ch->env_error >> FIXED_POINT_SHIFT);
		switch (ch->state) {
		case SOUND_ATTACK:
			ch->env_error &= FIXED_POINT_REMAINDER;
			ch->envx += (int)(step << 1);
			ch->envxx = ch->envx << ENVX_SHIFT;
			if (ch->envx >= 126) {
				ch->envx = 127;
				ch->envxx = 127 << ENVX_SHIFT;
				ch->state = SOUND_DECAY;
				if (ch->sustain_level != 8) {
					S9xSetEnvRate(ch, ch->decay_rate, -1,
					              (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3);
				} else {
					ch->state = SOUND_SUSTAIN;
					S9xSetEnvRate(ch, ch->sustain_rate, -1, 0);
				}
			}
			break;

		case SOUND_DECAY:
			while (ch->env_error >= FIXED_POINT) {
				ch->envxx = (ch->envxx >> 8) * 255;
				ch->env_error -= FIXED_POINT;
			}
			ch->envx = ch->envxx >> ENVX_SHIFT;
			if (ch->envx <= ch->envx_target) {
				if (ch->envx <= 0) {
					*out_ended = 1;
					return done - 1U;
				}
				ch->state = SOUND_SUSTAIN;
				S9xSetEnvRate(ch, ch->sustain_rate, -1, 0);
			}
			break;

		case SOUND_SUSTAIN:
			while (ch->env_error >= FIXED_POINT) {
				ch->envxx = (ch->envxx >> 8) * 255;
				ch->env_error -= FIXED_POINT;
			}
			ch->envx = ch->envxx >> ENVX_SHIFT;
			if (ch->envx <= 0) {
				*out_ended = 1;
				return done - 1U;
			}
			break;

		case SOUND_RELEASE:
			while (ch->env_error >= FIXED_POINT) {
				ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
				ch->env_error -= FIXED_POINT;
			}
			ch->envx = ch->envxx >> ENVX_SHIFT;
			if (ch->envx <= 0) {
				*out_ended = 1;
				return done - 1U;
			}
			break;

		case SOUND_INCREASE_LINEAR:
			ch->env_error &= FIXED_POINT_REMAINDER;
			ch->envx += (int)(step << 1);
			ch->envxx = ch->envx << ENVX_SHIFT;
			if (ch->envx >= 126) {
				ch->envx = 127;
				ch->envxx = 127 << ENVX_SHIFT;
				ch->state = SOUND_GAIN;
				ch->mode = MODE_GAIN;
				S9xSetEnvRate(ch, 0, -1, 0);
			}
			break;

		case SOUND_INCREASE_BENT_LINE:
			if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4) {
				while (ch->env_error >= FIXED_POINT) {
					ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
					ch->env_error -= FIXED_POINT;
				}
				ch->envx = ch->envxx >> ENVX_SHIFT;
			} else {
				ch->env_error &= FIXED_POINT_REMAINDER;
				ch->envx += (int)(step << 1);
				ch->envxx = ch->envx << ENVX_SHIFT;
			}
			if (ch->envx >= 126) {
				ch->envx = 127;
				ch->envxx = 127 << ENVX_SHIFT;
				ch->state = SOUND_GAIN;
				ch->mode = MODE_GAIN;
				S9xSetEnvRate(ch, 0, -1, 0);
			}
			break;

		case SOUND_DECREASE_LINEAR:
			ch->env_error &= FIXED_POINT_REMAINDER;
			ch->envx -= (int)(step << 1);
			ch->envxx = ch->envx << ENVX_SHIFT;
			if (ch->envx <= 0) {
				*out_ended = 1;
				return done - 1U;
			}
			break;

		case SOUND_DECREASE_EXPONENTIAL:
			while (ch->env_error >= FIXED_POINT) {
				ch->envxx = (ch->envxx >> 8) * 255;
				ch->env_error -= FIXED_POINT;
			}
			ch->envx = ch->envxx >> ENVX_SHIFT;
			if (ch->envx <= 0) {
				*out_ended = 1;
				return done - 1U;
			}
			break;

		case SOUND_GAIN:
		default:
			S9xSetEnvRate(ch, 0, -1, 0);
			break;
		}
		ch->left_vol_level  = (ch->envx * ch->volume_left) >> 7;
		ch->right_vol_level = (ch->envx * ch->volume_right) >> 7;
	}
	return n_pairs;
}

/*
 * Fast O(blocks) host BRR block collector & pointer stepper.
 * Returns play_pairs (<= max_pairs), sets *out_ended=1 if sample ended, or -1 on block overflow.
 */
static int snes_host_collect_brr(int ch_idx, Channel *ch, uint32_t freq0, uint32_t max_pairs,
                                 volatile uint8_t (*dst_blocks)[12], uint32_t *blk_total,
                                 int *out_ended)
{
	*out_ended = 0;

	if (ch->needs_decode) {
		if (*blk_total >= AUX_SNES_MAX_BLOCKS)
			return -1;
		uint32_t bptr = ch->block_pointer & 0xFFFFU;
		if (bptr + 9U > 0x10000U)
			bptr = 0;
		const uint8_t *src = &IAPU.RAM[bptr];
		snes_jzaux_copy_brr9(dst_blocks[(*blk_total)++], src);
		uint8_t hdr = src[0];
		ch->last_block = hdr & 1;
		if (ch->last_block)
			ch->loop = (hdr & 2) != 0;
		ch->block_pointer = (bptr + 9U) & 0xFFFFU;
		ch->needs_decode = FALSE;

		uint32_t sp = freq0 >> FIXED_POINT_SHIFT;
		if (sp == 0) sp = 1;
		if (sp >= SOUND_DECODE_LENGTH) sp = SOUND_DECODE_LENGTH - 1;
		ch->sample_pointer = sp;
	}

	if (ch->type != SOUND_SAMPLE || freq0 == 0 || max_pairs == 0) {
		if (freq0 != 0 && max_pairs != 0) {
			uint64_t tot = (uint64_t)ch->count + (uint64_t)freq0 * max_pairs;
			ch->count = (uint32_t)(tot & FIXED_POINT_REMAINDER);
		}
		return (int)max_pairs;
	}

	uint32_t rem = max_pairs;
	uint32_t done = 0;

	while (rem > 0) {
		if (JUST_PLAYED_LAST_SAMPLE(ch)) {
			uint32_t need = (ch->count < FIXED_POINT) ? (FIXED_POINT - ch->count) : 1U;
			uint32_t p = (need + freq0 - 1U) / freq0;
			if (p == 0) p = 1U;
			if (p <= rem) {
				*out_ended = 1;
				return (int)(done + p - 1U);
			}
			ch->count += (uint32_t)((uint64_t)freq0 * rem);
			return (int)max_pairs;
		}

		uint32_t to_blk = (ch->sample_pointer < SOUND_DECODE_LENGTH)
		                ? (SOUND_DECODE_LENGTH - ch->sample_pointer) : 1U;
		uint64_t need_fp = ((uint64_t)to_blk << FIXED_POINT_SHIFT);
		if (need_fp > ch->count)
			need_fp -= ch->count;
		else
			need_fp = 1U;

		uint64_t p64 = (need_fp + freq0 - 1U) / freq0;
		if (p64 == 0) p64 = 1U;

		if (p64 > rem) {
			uint64_t tot = (uint64_t)ch->count + (uint64_t)freq0 * rem;
			ch->sample_pointer += (uint32_t)(tot >> FIXED_POINT_SHIFT);
			ch->count = (uint32_t)(tot & FIXED_POINT_REMAINDER);
			return (int)max_pairs;
		}

		uint32_t p = (uint32_t)p64;
		uint64_t tot = (uint64_t)ch->count + (uint64_t)freq0 * p;
		ch->sample_pointer += (uint32_t)(tot >> FIXED_POINT_SHIFT);
		ch->count = (uint32_t)(tot & FIXED_POINT_REMAINDER);
		done += p;
		rem -= p;

		while (ch->sample_pointer >= SOUND_DECODE_LENGTH) {
			ch->sample_pointer -= SOUND_DECODE_LENGTH;
			if (ch->last_block) {
				if (!ch->loop) {
					ch->sample_pointer = LAST_SAMPLE;
					break;
				} else {
					S9xAPUSetEndX(ch_idx);
					ch->last_block = FALSE;
					uint8 *dir = S9xGetSampleAddress(ch->sample_number);
					ch->block_pointer = READ_WORD(dir + 2);
				}
			}
			if (*blk_total >= AUX_SNES_MAX_BLOCKS)
				return -1;
			uint32_t bptr = ch->block_pointer & 0xFFFFU;
			if (bptr + 9U > 0x10000U)
				bptr = 0;
			const uint8_t *src = &IAPU.RAM[bptr];
			snes_jzaux_copy_brr9(dst_blocks[(*blk_total)++], src);
			uint8_t hdr = src[0];
			ch->last_block = hdr & 1;
			if (ch->last_block)
				ch->loop = (hdr & 2) != 0;
			ch->block_pointer = (bptr + 9U) & 0xFFFFU;
		}
	}

	return (int)max_pairs;
}

static int snes_jzaux_queue_one(uint32_t n_pairs, int stereo)
{
	if (!g_snes_aux_active || n_pairs == 0 || n_pairs > AUX_SNES_MAX_PAIRS)
		return 0;

	/* Check for rare pitch modulation on any active voice */
	int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];
	if (pitch_mod) {
		for (int j = 0; j < NUM_CHANNELS; j++) {
			if ((pitch_mod & (1 << j)) &&
			    SoundData.channels[j].state != SOUND_SILENT &&
			    (so.sound_switch & (1 << j)))
				return 0;
		}
	}

	int do_echo = (SoundData.echo_enable && SoundData.echo_buffer_size > 0) ? 1 : 0;
	uint32_t sample_count = stereo ? (n_pairs << 1) : n_pairs;
	int32_t echo_ptr = SoundData.echo_ptr;
	int32_t echo_size = SoundData.echo_buffer_size;

	/* Reap ready jobs, and ensure free slot + no overlapping Echo[] ranges */
	snes_jzaux_reap_ready();
	if ((g_snes_submit_seq - g_snes_reap_seq) >= 2U)
		snes_jzaux_reap_one();
	if (!g_snes_aux_active)
		return 0;

	if (do_echo && g_snes_submit_seq != g_snes_reap_seq) {
		uint32_t prev_idx = g_snes_reap_seq & 1U;
		struct aux_snes_inflight *pinf = &g_snes_inflight[prev_idx];
		if (pinf->echo_on) {
			int32_t p_start = pinf->echo_ptr;
			int32_t p_end   = p_start + (int32_t)(pinf->stereo ? (pinf->n_pairs << 1) : pinf->n_pairs);
			int32_t c_start = echo_ptr;
			int32_t c_end   = echo_ptr + (int32_t)sample_count;
			if (!(c_end <= p_start || c_start >= p_end))
				snes_jzaux_reap_one();
		}
	}
	if (!g_snes_aux_active)
		return 0;

	/* If host state was reset / snapshot-loaded, sync SRAM while AUX is idle */
	if (g_snes_echo_sync_host || g_snes_ch_sync_mask) {
		if (g_snes_submit_seq != g_snes_reap_seq)
			snes_jzaux_flush();
		if (!g_snes_aux_active)
			return 0;
		if (g_snes_echo_sync_host) {
			g_snes_aux_sram->noise_gen = noise_gen;
			g_snes_aux_sram->echo_z    = (uint32_t)Z;
			for (int k = 0; k < 16; k++)
				g_snes_aux_sram->echo_loop[k] = Loop[k];
			g_snes_echo_sync_host = 0;
		}
		for (int j = 0; j < NUM_CHANNELS; j++) {
			if (g_snes_ch_sync_mask & (1U << j)) {
				Channel *ch = &SoundData.channels[j];
				volatile struct aux_snes_ch_persist *pst = &g_snes_aux_sram->ch_state[j];
				for (int k = 0; k < 16; k++)
					pst->decoded[k] = ch->decoded[k];
				pst->previous[0] = ch->previous[0];
				pst->previous[1] = ch->previous[1];
				pst->sample      = ch->sample;
				pst->next_sample = ch->next_sample;
				pst->gauss_hist[0] = ch->sample;
				pst->gauss_hist[1] = ch->sample;
				pst->gauss_hist[2] = ch->sample;
				pst->gauss_hist[3] = ch->next_sample;
			}
		}
	}

	/* Snapshot mutable state in case BRR block buffer overflows (>384 blocks) */
	Channel saved_ch[NUM_CHANNELS];
	uint8 saved_endx = APU.DSP[APU_ENDX];
	uint8 saved_kon  = APU.DSP[APU_KON];
	uint8 saved_koff = APU.DSP[APU_KOFF];
	uint8 saved_keyed = APU.KeyedChannels;
	int   saved_echo_ptr = SoundData.echo_ptr;

	uint32_t idx = g_snes_submit_seq & 1U;
	volatile struct aux_snes_slot *slot = g_snes_aux_slots[idx];

	uint32_t active_mask = 0;
	uint32_t blk_total = 0;

	for (int j = 0; j < NUM_CHANNELS; j++) {
		Channel *ch = &SoundData.channels[j];
		if (ch->state == SOUND_SILENT || !(so.sound_switch & (1 << j)))
			continue;

		saved_ch[j] = *ch;
		active_mask |= (1U << j);

		uint32_t freq0 = (uint32_t)(((uint64_t)ch->frequency * 985U) / 1000U);
		volatile struct aux_snes_ch *ac = &slot->ch[j];

		ac->freq0           = freq0;
		ac->count           = ch->count;
		ac->sample_pointer  = ch->sample_pointer;
		ac->env_error       = (uint32_t)ch->env_error;
		ac->erate           = (uint32_t)ch->erate;
		ac->erate_decay     = snes_calc_erate(SOUND_DECAY, ch->decay_rate);
		ac->erate_sustain   = snes_calc_erate(SOUND_SUSTAIN, ch->sustain_rate);
		ac->envxx           = ch->envxx;
		ac->envx            = ch->envx;
		ac->envx_target     = ch->envx_target;
		ac->sustain_level   = (int32_t)ch->sustain_level;
		ac->volume_left     = ch->volume_left;
		ac->volume_right    = ch->volume_right;
		ac->left_vol_level  = ch->left_vol_level;
		ac->right_vol_level = ch->right_vol_level;
		ac->sample          = ch->sample;
		ac->next_sample     = ch->next_sample;
		ac->state           = (uint8_t)ch->state;

		uint8_t cflags = 0;
		if (ch->type != SOUND_SAMPLE) cflags |= AUX_SNES_CH_NOISE;
		if (ch->echo_buf_ptr == EchoBuffer) cflags |= AUX_SNES_CH_ECHO;
		if (ch->needs_decode) cflags |= AUX_SNES_CH_NEEDS_DECODE;
		if (ch->last_block)   cflags |= AUX_SNES_CH_LAST_BLOCK;
		if (ch->loop)         cflags |= AUX_SNES_CH_LOOP;
		if (g_snes_ch_reset_mask & (1U << j)) cflags |= AUX_SNES_CH_RESET_HIST;
		if (g_snes_ch_sync_mask & (1U << j))  cflags |= AUX_SNES_CH_SYNC_HOST;
		ac->flags = cflags;

		uint32_t blk_off = blk_total;
		int env_ended = 0, smp_ended = 0;
		uint32_t env_pairs = snes_host_step_envelope(ch, n_pairs, &env_ended);
		int play_pairs = snes_host_collect_brr(j, ch, freq0, env_pairs,
		                                       slot->brr_blocks, &blk_total, &smp_ended);
		if (play_pairs < 0) {
			/* Overflow (>384 BRR blocks): restore & fall back cleanly */
			for (int r = 0; r <= j; r++) {
				if (active_mask & (1U << r))
					SoundData.channels[r] = saved_ch[r];
			}
			APU.DSP[APU_ENDX] = saved_endx;
			APU.DSP[APU_KON]  = saved_kon;
			APU.DSP[APU_KOFF] = saved_koff;
			APU.KeyedChannels = saved_keyed;
			SoundData.echo_ptr = saved_echo_ptr;
			g_snes_prof_fallbacks++;
			return 0;
		}

		/* If sample ended before envelope ended, re-step envelope only to play_pairs */
		if (smp_ended && (uint32_t)play_pairs < env_pairs) {
			Channel tmp = saved_ch[j];
			int dummy_end = 0;
			snes_host_step_envelope(&tmp, (uint32_t)play_pairs, &dummy_end);
			ch->state           = tmp.state;
			ch->mode            = tmp.mode;
			ch->envx            = tmp.envx;
			ch->envxx           = tmp.envxx;
			ch->env_error       = tmp.env_error;
			ch->erate           = tmp.erate;
			ch->envx_target     = tmp.envx_target;
			ch->direction       = tmp.direction;
			ch->left_vol_level  = tmp.left_vol_level;
			ch->right_vol_level = tmp.right_vol_level;
		}

		if (env_ended || smp_ended)
			S9xAPUSetEndOfSample(j, ch);

		ac->blk_off    = (uint16_t)blk_off;
		ac->blk_cnt    = (uint16_t)(blk_total - blk_off);
		ac->play_pairs = (uint16_t)play_pairs;
	}

	g_snes_ch_reset_mask &= ~active_mask;
	g_snes_ch_sync_mask  &= ~active_mask;

	uint32_t sflags = 0;
	if (stereo) sflags |= AUX_SNES_FLAG_STEREO;
	if (Settings.InterpolatedSound) sflags |= AUX_SNES_FLAG_INTERP;
	if (do_echo) {
		sflags |= AUX_SNES_FLAG_ECHO;
		if (FilterTapDefinitionBitfield != 0)
			sflags |= AUX_SNES_FLAG_FIR;
		/* Copy contiguous Echo[] slice into slot */
		const int32_t *src_e = &Echo[echo_ptr];
		volatile int32_t *dst_e = slot->echo_io;
		for (uint32_t i = 0; i < sample_count; i++)
			dst_e[i] = src_e[i];

		echo_ptr += (int32_t)sample_count;
		if (echo_ptr >= echo_size)
			echo_ptr = 0;
		SoundData.echo_ptr = echo_ptr;
	}

	slot->n_pairs        = n_pairs;
	slot->flags          = sflags;
	slot->ch_active_mask = active_mask;
	slot->echo_fb        = SoundData.echo_feedback;
	slot->mvol0          = SoundData.master_volume[0];
	slot->mvol1          = SoundData.master_volume[1];
	slot->evol0          = SoundData.echo_volume[0];
	slot->evol1          = SoundData.echo_volume[1];
	for (int k = 0; k < 8; k++)
		slot->filter_taps[k] = FilterTaps[k];

	g_snes_inflight[idx].n_pairs  = n_pairs;
	g_snes_inflight[idx].stereo   = stereo ? 1U : 0U;
	g_snes_inflight[idx].echo_on  = do_echo ? 1U : 0U;
	g_snes_inflight[idx].echo_ptr = do_echo ? saved_echo_ptr : -1;

	__sync_synchronize();
	g_snes_submit_seq++;
	g_snes_aux_sram->submit_seq = g_snes_submit_seq;
	return 1;
}

extern "C" int snes_jzaux_submit(uint32_t frames, int stereo)
{
	if (!g_snes_aux_active || frames == 0 || so.mute_sound || Settings.EightBitConsoleSound)
		return 0;

	/* Quick pre-check for pitch modulation */
	int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];
	if (pitch_mod) {
		for (int j = 0; j < NUM_CHANNELS; j++) {
			if ((pitch_mod & (1 << j)) &&
			    SoundData.channels[j].state != SOUND_SILENT &&
			    (so.sound_switch & (1 << j))) {
				g_snes_prof_fallbacks++;
				return 0;
			}
		}
	}

	uint32_t rem = frames;
	while (rem > 0) {
		uint32_t chunk = (rem > AUX_SNES_MAX_PAIRS) ? AUX_SNES_MAX_PAIRS : rem;
		if (SoundData.echo_enable && SoundData.echo_buffer_size > 0) {
			int32_t avail_samples = SoundData.echo_buffer_size - SoundData.echo_ptr;
			if (avail_samples <= 0) {
				SoundData.echo_ptr = 0;
				avail_samples = SoundData.echo_buffer_size;
			}
			uint32_t avail_pairs = stereo ? ((uint32_t)avail_samples >> 1) : (uint32_t)avail_samples;
			if (avail_pairs == 0) {
				/* Odd echo_size/echo_ptr edge case: fall back cleanly */
				g_snes_prof_fallbacks++;
				return 0;
			}
			if (chunk > avail_pairs)
				chunk = avail_pairs;
		}

		if (!snes_jzaux_queue_one(chunk, stereo)) {
			/* Partial fallback: flush queued chunks, mix remaining locally */
			snes_jzaux_flush();
			int16_t tmp_pcm[AUX_SNES_MAX_PAIRS * 2];
			while (rem > 0) {
				uint32_t c2 = (rem > AUX_SNES_MAX_PAIRS) ? AUX_SNES_MAX_PAIRS : rem;
				S9xMixSamples((uint8 *)tmp_pcm, (int)(stereo ? (c2 << 1) : c2));
				sal_AudioCommitPcm(tmp_pcm, c2);
				rem -= c2;
			}
			g_snes_ch_sync_mask = 0xFFU;
			g_snes_echo_sync_host = 1;
			return 1;
		}
		rem -= chunk;
	}
	return 1;
}

extern "C" void snes_jzaux_init(void)
{
	if (g_snes_aux_ctx)
		return;

	jz_aux_ctx_t *ctx = jz_aux_open();
	if (!ctx) {
		printf("PocketSNES+: /dev/jz_aux not available; S-DSP running on main CPU.\n");
		return;
	}
	if (!ctx->sram || ctx->sram_mapped_size < sizeof(struct aux_snes_sram)) {
		printf("PocketSNES+: /dev/jz_aux SRAM too small (%uB); S-DSP on main CPU.\n",
		       (unsigned)ctx->sram_mapped_size);
		jz_aux_close(ctx);
		return;
	}
	volatile uint8_t *tcsm1 = (volatile uint8_t *)jz_aux_map_tcsm1(ctx);
	if (!tcsm1) {
		printf("PocketSNES+: /dev/jz_aux TCSM1 mmap failed; S-DSP on main CPU.\n");
		jz_aux_close(ctx);
		return;
	}

	volatile uint32_t *s32 = (volatile uint32_t *)ctx->sram;
	for (size_t i = 0; i < (sizeof(struct aux_snes_sram) + 3) / 4; i++)
		s32[i] = 0;

	if (jz_aux_load(ctx, aux_snes_dsp_fw, sizeof(aux_snes_dsp_fw), 1, 1) < 0) {
		printf("PocketSNES+: /dev/jz_aux firmware load failed; S-DSP on main CPU.\n");
		jz_aux_close(ctx);
		return;
	}

	volatile struct aux_snes_sram *sram = (volatile struct aux_snes_sram *)ctx->sram;
	int tries = 200000;
	while (sram->magic != AUX_SNES_MAGIC && --tries > 0)
		;
	if (sram->magic != AUX_SNES_MAGIC) {
		printf("PocketSNES+: /dev/jz_aux firmware boot timeout (magic=0x%08x); S-DSP on main CPU.\n",
		       sram->magic);
		jz_aux_close(ctx);
		return;
	}

	g_snes_aux_ctx      = ctx;
	g_snes_aux_sram     = sram;
	g_snes_aux_slots[0] = (volatile struct aux_snes_slot *)ctx->tcsm0;
	g_snes_aux_slots[1] = (volatile struct aux_snes_slot *)(tcsm1 + AUX_SNES_SLOT1_OFFSET);
	g_snes_submit_seq   = 0;
	g_snes_reap_seq     = 0;
	g_snes_prof_jobs    = 0;
	g_snes_prof_fallbacks = 0;
	g_snes_prof_waits   = 0;
	g_snes_ch_reset_mask = 0xFFU;
	g_snes_ch_sync_mask  = 0xFFU;
	g_snes_echo_sync_host = 1;
	g_snes_aux_active   = 1;

	printf("PocketSNES+: JZ4760B AUX+MXU S-DSP OFFLOAD ACTIVE! (%u MHz HCLK, 4-pt Gaussian + 8-tap Echo FIR, fw=%uB)\n",
	       ctx->info.hclk_mhz, (unsigned)sizeof(aux_snes_dsp_fw));
	FILE *lf = fopen("/tmp/pocketsnes_aux.log", "w");
	if (lf) {
		fprintf(lf, "ACTIVE hclk=%uMHz cclk=%uMHz fw=%uB\n",
		        ctx->info.hclk_mhz, ctx->info.cclk_mhz, (unsigned)sizeof(aux_snes_dsp_fw));
		fclose(lf);
	}
}

extern "C" void snes_jzaux_shutdown(void)
{
	if (!g_snes_aux_ctx)
		return;
	snes_jzaux_flush();
	printf("PocketSNES+: AUX S-DSP stats: jobs=%u fallbacks=%u waits=%u\n",
	       g_snes_prof_jobs, g_snes_prof_fallbacks, g_snes_prof_waits);
	FILE *lf = fopen("/tmp/pocketsnes_aux.log", "a");
	if (lf) {
		fprintf(lf, "SHUTDOWN jobs=%u fallbacks=%u waits=%u\n",
		        g_snes_prof_jobs, g_snes_prof_fallbacks, g_snes_prof_waits);
		fclose(lf);
	}
	g_snes_aux_active = 0;
	jz_aux_close(g_snes_aux_ctx);
	g_snes_aux_ctx = NULL;
	g_snes_aux_sram = NULL;
	g_snes_aux_slots[0] = NULL;
	g_snes_aux_slots[1] = NULL;
}
