#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <SDL.h>

#include <sal.h>

extern void S9xMixSamples(uint8_t *buffer, int sample_count);

#ifdef MIPS_XBURST
extern void snes_jzaux_reap_ready(void);
extern void snes_jzaux_flush(void);
extern void snes_jzaux_shutdown(void);
extern uint32_t snes_jzaux_inflight_frames(void);
extern int snes_jzaux_submit(uint32_t frames, int stereo);
#endif

#define BUFFER_FRAMES 4
// 48000 Hz maximum; 1/50 of a second; 4 frames to hold (plus a bit extra)
#define BUFFER_SAMPLES (48000 / 50 * (BUFFER_FRAMES + 1))

static SDL_AudioSpec audiospec;

volatile static unsigned int ReadPos, WritePos;

// 2 channels per sample (stereo); 2 bytes per sample-channel (16-bit)
static uint8_t Buffer[BUFFER_SAMPLES * 2 * 2];
static u32 SamplesPerFrame, BytesPerSample;
static u32 Muted; // S9xSetAudioMute(TRUE) gets undone after SNES Global Mute ends

static void sdl_audio_callback (void *userdata, Uint8 *stream, int len)
{
	u32 SamplesRequested = len / BytesPerSample, SamplesBuffered,
	    LocalWritePos = WritePos /* isolate a bit against races with the main thread */,
	    LocalReadPos = ReadPos /* keep a non-volatile copy at hand */;
	if (LocalReadPos <= LocalWritePos)
		SamplesBuffered = LocalWritePos - LocalReadPos;
	else
		SamplesBuffered = BUFFER_SAMPLES - (LocalReadPos - LocalWritePos);

	if (SamplesRequested > SamplesBuffered)
	{
		return;
	}

	if (Muted)
	{
		memset(stream, 0, len);
	}
	else if (LocalReadPos + SamplesRequested > BUFFER_SAMPLES)
	{
		memmove(stream, &Buffer[LocalReadPos * BytesPerSample], (BUFFER_SAMPLES - LocalReadPos) * BytesPerSample);
		memmove(&stream[(BUFFER_SAMPLES - LocalReadPos) * BytesPerSample], &Buffer[0], (SamplesRequested - (BUFFER_SAMPLES - LocalReadPos)) * BytesPerSample);
	}
	else
	{
		memmove(stream, &Buffer[LocalReadPos * BytesPerSample], len);
	}
	ReadPos = (LocalReadPos + SamplesRequested) % BUFFER_SAMPLES;
}

s32 sal_AudioInit(s32 rate, s32 bits, s32 stereo, s32 Hz)
{
	int buffer = 1;
	audiospec.freq = rate;
	audiospec.channels = (stereo + 1);
	audiospec.format = AUDIO_S16;

	audiospec.samples = (rate / Hz);
	if (!stereo && (audiospec.samples & 1))
		audiospec.samples--;

	 
	SamplesPerFrame = audiospec.samples;
	BytesPerSample = audiospec.channels * (bits >> 3);


	audiospec.callback = sdl_audio_callback;
	//RS-97 fix, need to be power of 2
	while (buffer < audiospec.samples)
	{
		buffer*=2;
	}
	audiospec.samples = buffer;

	if (SDL_OpenAudio(&audiospec, NULL) < 0) {
		fprintf(stderr, "Unable to initialize audio.\n");
		return SAL_ERROR;
	}

	WritePos = ReadPos = 0;

	return SAL_OK;
}

void sal_AudioCommitPcm(const int16_t *pcm, uint32_t frames)
{
	if (frames == 0 || BytesPerSample == 0)
		return;
	u32 LocalWritePos = WritePos;
	const uint8_t *src = (const uint8_t *)pcm;
	while (frames > 0)
	{
		u32 chunk = BUFFER_SAMPLES - LocalWritePos;
		if (chunk > frames)
			chunk = frames;
		u32 bytes = chunk * BytesPerSample;
		memcpy(&Buffer[LocalWritePos * BytesPerSample], src, bytes);
		src += bytes;
		LocalWritePos = (LocalWritePos + chunk) % BUFFER_SAMPLES;
		frames -= chunk;
	}
	WritePos = LocalWritePos;
}

void sal_AudioPause(void)
{
#ifdef MIPS_XBURST
	snes_jzaux_flush();
#endif
	SDL_PauseAudio(1);
}

void sal_AudioResume(void)
{
	SDL_PauseAudio(0);
}

void sal_AudioClose(void)
{
#ifdef MIPS_XBURST
	snes_jzaux_shutdown();
#endif
	SDL_CloseAudio();
}

void sal_AudioGenerate(u32 samples)
{
#ifdef MIPS_XBURST
	snes_jzaux_reap_ready();
#endif
	u32 SamplesAvailable,
	    LocalReadPos = ReadPos /* isolate a bit against races with the audio thread */,
	    LocalWritePos = WritePos /* keep a non-volatile copy at hand */;
	if (LocalReadPos <= LocalWritePos)
		SamplesAvailable = BUFFER_SAMPLES - (LocalWritePos - LocalReadPos);
	else
		SamplesAvailable = LocalReadPos - LocalWritePos;
#ifdef MIPS_XBURST
	{
		u32 inflight = snes_jzaux_inflight_frames();
		if (SamplesAvailable <= inflight + 1)
			return;
		SamplesAvailable -= inflight;
	}
#endif
	if (samples >= SamplesAvailable)
	{
		samples = SamplesAvailable - 1;
	}
	if (samples == 0)
		return;
#ifdef MIPS_XBURST
	if (snes_jzaux_submit(samples, audiospec.channels == 2))
		return;
	snes_jzaux_flush();
	/* Re-read LocalWritePos after flushing any in-flight AUX PCM */
	LocalWritePos = WritePos;
#endif
	if (samples > BUFFER_SAMPLES - LocalWritePos)
	{
		u32 first = BUFFER_SAMPLES - LocalWritePos;
		S9xMixSamples(&Buffer[LocalWritePos * BytesPerSample], first * audiospec.channels);
		WritePos = 0;
		S9xMixSamples(&Buffer[0], (samples - first) * audiospec.channels);
		WritePos = (samples - first) % BUFFER_SAMPLES;
	}
	else
	{
		S9xMixSamples(&Buffer[LocalWritePos * BytesPerSample], samples * audiospec.channels);
		WritePos = (LocalWritePos + samples) % BUFFER_SAMPLES;
	}
}

u32 sal_AudioGetFramesBuffered()
{
#ifdef MIPS_XBURST
	snes_jzaux_reap_ready();
#endif
	u32 SamplesBuffered,
	    LocalReadPos = ReadPos /* isolate a bit against races with the audio thread */,
	    LocalWritePos = WritePos /* keep a non-volatile copy at hand */;
	if (LocalReadPos <= LocalWritePos)
		SamplesBuffered = LocalWritePos - LocalReadPos;
	else
		SamplesBuffered = BUFFER_SAMPLES - (LocalReadPos - LocalWritePos);
#ifdef MIPS_XBURST
	SamplesBuffered += snes_jzaux_inflight_frames();
#endif
	return SamplesBuffered / SamplesPerFrame;
}

u32 sal_AudioGetBufferFillPermille(void)
{
#ifdef MIPS_XBURST
	snes_jzaux_reap_ready();
#endif
	u32 SamplesBuffered,
	    LocalReadPos = ReadPos,
	    LocalWritePos = WritePos;
	if (!SamplesPerFrame)
		return 1000;
	if (LocalReadPos <= LocalWritePos)
		SamplesBuffered = LocalWritePos - LocalReadPos;
	else
		SamplesBuffered = BUFFER_SAMPLES - (LocalReadPos - LocalWritePos);
#ifdef MIPS_XBURST
	SamplesBuffered += snes_jzaux_inflight_frames();
#endif
	u32 denom = SamplesPerFrame * BUFFER_FRAMES;
	u32 permille = (SamplesBuffered * 1000u) / denom;
	return permille > 1000u ? 1000u : permille;
}

u32 sal_AudioGetMinFrames()
{
	return BUFFER_FRAMES - 1;
}

u32 sal_AudioGetMaxFrames()
{
	return BUFFER_FRAMES;
}

u32 sal_AudioGetSamplesPerFrame()
{
	return SamplesPerFrame;
}

u32 sal_AudioGetBytesPerSample()
{
	return BytesPerSample;
}

void sal_AudioSetVolume(s32 l, s32 r)
{
}

void sal_AudioSetMuted(u32 muted)
{
	Muted = muted;
}
