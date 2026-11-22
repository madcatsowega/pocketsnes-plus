/*
 * Ingenic JZ4760B MXU SIMD kernels for PocketSNES+ v2 audio mixing & memory ops.
 *
 * All MXU opcodes use verified .word encodings (checked against Ingenic
 * mipsel-linux-as) and are only invoked after /proc/cpuinfo confirms kernel
 * MXU context-switch support ("ASEs implemented : ... mxu").
 */
	.text
	.set	noreorder

/* void jz_mxu_enable(void) */
	.globl	jz_mxu_enable
	.type	jz_mxu_enable, @function
jz_mxu_enable:
	li	$2, 1
	.word	0x7002042f		/* S32I2M xr16, $2 (enable MXU) */
	nop				/* >=3-cycle JZ4760B XBurst hazard */
	nop
	nop
	nop
	jr	$31
	nop

/* void snes_mix_noecho_mxu(const int *mbuf, short *out, int sample_count, int mvol0, int mvol1)
 * a0 ($4) = mbuf (int32*)
 * a1 ($5) = out  (int16*, 4-byte aligned)
 * a2 ($6) = sample_count (even, > 0)
 * a3 ($7) = mvol0
 * 16($sp) -> $14 = mvol1
 */
	.globl	snes_mix_noecho_mxu
	.type	snes_mix_noecho_mxu, @function
snes_mix_noecho_mxu:
	blez	$6, 4f
	lw	$14, 16($sp)		/* $14 = mvol1 */
	addiu	$5, $5, -4		/* pre-decrement out for S32SDI +4 */
	li	$8, 32767
	li	$9, -32768
	.word	0x700803af		/* S32I2M xr14, $8 (32767) */
	.word	0x700903ef		/* S32I2M xr15, $9 (-32768) */
	slti	$8, $6, 4
	bnez	$8, 2f
	nop
1:
	lw	$10, 0($4)
	lw	$11, 4($4)
	lw	$12, 8($4)
	lw	$13, 12($4)
	mul	$10, $10, $7
	mul	$11, $11, $14
	mul	$12, $12, $7
	mul	$13, $13, $14
	sra	$10, $10, 7
	sra	$11, $11, 7
	sra	$12, $12, 7
	sra	$13, $13, 7
	.word	0x700a006f		/* S32I2M xr1, $10 */
	.word	0x700b00af		/* S32I2M xr2, $11 */
	.word	0x700c016f		/* S32I2M xr5, $12 */
	.word	0x700d01af		/* S32I2M xr6, $13 */
	addiu	$6, $6, -4
	.word	0x70078443		/* S32MIN xr1, xr1, xr14 */
	.word	0x70078883		/* S32MIN xr2, xr2, xr14 */
	.word	0x70079543		/* S32MIN xr5, xr5, xr14 */
	.word	0x70079983		/* S32MIN xr6, xr6, xr14 */
	.word	0x7003c443		/* S32MAX xr1, xr1, xr15 */
	.word	0x7003c883		/* S32MAX xr2, xr2, xr15 */
	.word	0x7003d543		/* S32MAX xr5, xr5, xr15 */
	.word	0x7003d983		/* S32MAX xr6, xr6, xr15 */
	.word	0x731048fd		/* S32SFL xr3, xr2, xr1, xr4, ptn3 */
	.word	0x732159fd		/* S32SFL xr7, xr6, xr5, xr8, ptn3 */
	.word	0x70a00515		/* S32SDI xr4, $5, 4 */
	.word	0x70a00615		/* S32SDI xr8, $5, 4 */
	slti	$8, $6, 4
	beqz	$8, 1b
	addiu	$4, $4, 16
2:
	slti	$8, $6, 2
	bnez	$8, 4f
	nop
	lw	$10, 0($4)
	lw	$11, 4($4)
	mul	$10, $10, $7
	mul	$11, $11, $14
	sra	$10, $10, 7
	sra	$11, $11, 7
	.word	0x700a006f		/* S32I2M xr1, $10 */
	.word	0x700b00af		/* S32I2M xr2, $11 */
	.word	0x70078443		/* S32MIN xr1, xr1, xr14 */
	.word	0x70078883		/* S32MIN xr2, xr2, xr14 */
	.word	0x7003c443		/* S32MAX xr1, xr1, xr15 */
	.word	0x7003c883		/* S32MAX xr2, xr2, xr15 */
	.word	0x731048fd		/* S32SFL xr3, xr2, xr1, xr4, ptn3 */
	.word	0x70a00515		/* S32SDI xr4, $5, 4 */
4:
	jr	$31
	nop

/* void snes_mix_echo_nofir_mxu(
 *     const int *mbuf,   // $4
 *     const int *ebuf,   // $5
 *     int *echo,         // $6 (contiguous non-wrapping slice)
 *     short *out,        // $7 (4-byte aligned)
 *     int count,         // 16($sp) (even, > 0)
 *     const int *params  // 20($sp): {echo_fb, mvol0, mvol1, evol0, evol1}
 * );
 * Works for both stereo and mono (mono sets mvol1=mvol0, evol1=evol0).
 */
	.globl	snes_mix_echo_nofir_mxu
	.type	snes_mix_echo_nofir_mxu, @function
snes_mix_echo_nofir_mxu:
	lw	$8, 16($sp)		/* $8 = count */
	blez	$8, 9f
	lw	$9, 20($sp)		/* $9 = params */
	addiu	$sp, $sp, -24
	sw	$16, 0($sp)
	sw	$17, 4($sp)
	sw	$18, 8($sp)
	sw	$19, 12($sp)
	sw	$20, 16($sp)
	lw	$16, 0($9)		/* $16 = echo_fb */
	lw	$17, 4($9)		/* $17 = mvol0 */
	lw	$18, 8($9)		/* $18 = mvol1 */
	lw	$19, 12($9)		/* $19 = evol0 */
	lw	$20, 16($9)		/* $20 = evol1 */
	addiu	$7, $7, -4		/* pre-decrement out for S32SDI +4 */
	li	$10, 32767
	li	$11, -32768
	.word	0x700a03af		/* S32I2M xr14, $10 (32767) */
	.word	0x700b03ef		/* S32I2M xr15, $11 (-32768) */
1:
	lw	$10, 0($6)		/* E0 = echo[0] */
	lw	$11, 4($6)		/* E1 = echo[1] */
	lw	$14, 0($4)		/* mbuf[0] */
	lw	$15, 4($4)		/* mbuf[1] */
	mult	$14, $17		/* mbuf0 * mvol0 */
	madd	$10, $19		/* + E0 * evol0 */
	mflo	$2
	mult	$15, $18		/* mbuf1 * mvol1 */
	madd	$11, $20		/* + E1 * evol1 */
	mflo	$3
	lw	$12, 0($5)		/* ebuf[0] */
	lw	$13, 4($5)		/* ebuf[1] */
	sra	$2, $2, 7
	sra	$3, $3, 7
	.word	0x7002006f		/* S32I2M xr1, $2 */
	.word	0x700300af		/* S32I2M xr2, $3 */
	mul	$10, $10, $16		/* E0 * echo_fb */
	mul	$11, $11, $16		/* E1 * echo_fb */
	.word	0x70078443		/* S32MIN xr1, xr1, xr14 */
	.word	0x70078883		/* S32MIN xr2, xr2, xr14 */
	sra	$10, $10, 7
	sra	$11, $11, 7
	.word	0x7003c443		/* S32MAX xr1, xr1, xr15 */
	.word	0x7003c883		/* S32MAX xr2, xr2, xr15 */
	addu	$10, $10, $12
	addu	$11, $11, $13
	.word	0x731048fd		/* S32SFL xr3, xr2, xr1, xr4, ptn3 */
	sw	$10, 0($6)
	sw	$11, 4($6)
	.word	0x70e00515		/* S32SDI xr4, $7, 4 */
	addiu	$8, $8, -2
	addiu	$4, $4, 8
	addiu	$5, $5, 8
	bgtz	$8, 1b
	addiu	$6, $6, 8

	lw	$16, 0($sp)
	lw	$17, 4($sp)
	lw	$18, 8($sp)
	lw	$19, 12($sp)
	lw	$20, 16($sp)
	addiu	$sp, $sp, 24
9:
	jr	$31
	nop

/* void snes_mix_echo_fir_stereo_mxu(
 *     const int *mbuf,    // $4
 *     const int *ebuf,    // $5
 *     int *echo,          // $6 (contiguous non-wrapping slice)
 *     short *out,         // $7 (4-byte aligned)
 *     int count,          // 16($sp) (even, > 0)
 *     int *win,           // 20($sp) (&win_buf[16]; win[-16..-1] primed)
 *     const int *taps8,   // 24($sp) (FilterTaps[0..7])
 *     const int *params5  // 28($sp) ({echo_fb, mvol0, mvol1, evol0, evol1})
 * );
 */
	.globl	snes_mix_echo_fir_stereo_mxu
	.type	snes_mix_echo_fir_stereo_mxu, @function
snes_mix_echo_fir_stereo_mxu:
	lw	$8, 16($sp)		/* $8 = count */
	blez	$8, 9f
	nop
	addiu	$sp, $sp, -40
	sw	$16, 0($sp)
	sw	$17, 4($sp)
	sw	$18, 8($sp)
	sw	$19, 12($sp)
	sw	$20, 16($sp)
	sw	$21, 20($sp)
	sw	$22, 24($sp)
	sw	$23, 28($sp)
	sw	$30, 32($sp)

	lw	$30, 60($sp)		/* $30 = win (20 + 40) */
	lw	$10, 64($sp)		/* $10 = taps8 (24 + 40) */
	lw	$9,  68($sp)		/* $9  = params5 (28 + 40) */
	lw	$16, 0($10)		/* ft0 */
	lw	$17, 4($10)		/* ft1 */
	lw	$18, 8($10)		/* ft2 */
	lw	$19, 12($10)		/* ft3 */
	lw	$20, 16($10)		/* ft4 */
	lw	$21, 20($10)		/* ft5 */
	lw	$22, 24($10)		/* ft6 */
	lw	$23, 28($10)		/* ft7 */

	addiu	$7, $7, -4		/* pre-decrement out for S32SDI +4 */
	li	$10, 32767
	li	$11, -32768
	.word	0x700a03af		/* S32I2M xr14, $10 (32767) */
	.word	0x700b03ef		/* S32I2M xr15, $11 (-32768) */
1:
	/* Left 8-tap FIR */
	lw	$10, 0($6)		/* E_L = echo[0] */
	lw	$11, -8($30)
	lw	$12, -16($30)
	lw	$13, -24($30)
	sw	$10, 0($30)
	mult	$10, $16
	madd	$11, $17
	madd	$12, $18
	madd	$13, $19
	lw	$11, -32($30)
	lw	$12, -40($30)
	lw	$13, -48($30)
	lw	$14, -56($30)
	madd	$11, $20
	madd	$12, $21
	madd	$13, $22
	madd	$14, $23
	mflo	$10
	sra	$10, $10, 7		/* E_L filtered */

	/* Right 8-tap FIR */
	lw	$15, 4($6)		/* E_R = echo[1] */
	lw	$11, -4($30)
	lw	$12, -12($30)
	lw	$13, -20($30)
	sw	$15, 4($30)
	mult	$15, $16
	madd	$11, $17
	madd	$12, $18
	madd	$13, $19
	lw	$11, -28($30)
	lw	$12, -36($30)
	lw	$13, -44($30)
	lw	$14, -52($30)
	madd	$11, $20
	madd	$12, $21
	madd	$13, $22
	madd	$14, $23
	mflo	$15
	sra	$15, $15, 7		/* E_R filtered */

	/* Mix L & R with master + echo volumes */
	lw	$11, 0($4)		/* mbuf[0] */
	lw	$12, 4($9)		/* mvol0 */
	lw	$13, 12($9)		/* evol0 */
	mult	$11, $12
	madd	$10, $13
	mflo	$2
	lw	$11, 4($4)		/* mbuf[1] */
	lw	$12, 8($9)		/* mvol1 */
	lw	$13, 16($9)		/* evol1 */
	mult	$11, $12
	madd	$15, $13
	mflo	$3
	sra	$2, $2, 7
	sra	$3, $3, 7
	.word	0x7002006f		/* S32I2M xr1, $2 */
	.word	0x700300af		/* S32I2M xr2, $3 */

	/* Echo feedback update interleaved with MXU clamp + pack */
	lw	$14, 0($9)		/* echo_fb */
	lw	$11, 0($5)		/* ebuf[0] */
	lw	$12, 4($5)		/* ebuf[1] */
	mul	$10, $10, $14
	mul	$15, $15, $14
	.word	0x70078443		/* S32MIN xr1, xr1, xr14 */
	.word	0x70078883		/* S32MIN xr2, xr2, xr14 */
	sra	$10, $10, 7
	sra	$15, $15, 7
	.word	0x7003c443		/* S32MAX xr1, xr1, xr15 */
	.word	0x7003c883		/* S32MAX xr2, xr2, xr15 */
	addu	$10, $10, $11
	addu	$15, $15, $12
	.word	0x731048fd		/* S32SFL xr3, xr2, xr1, xr4, ptn3 */
	sw	$10, 0($6)
	sw	$15, 4($6)
	.word	0x70e00515		/* S32SDI xr4, $7, 4 */

	addiu	$8, $8, -2
	addiu	$4, $4, 8
	addiu	$5, $5, 8
	addiu	$6, $6, 8
	bgtz	$8, 1b
	addiu	$30, $30, 8

	lw	$16, 0($sp)
	lw	$17, 4($sp)
	lw	$18, 8($sp)
	lw	$19, 12($sp)
	lw	$20, 16($sp)
	lw	$21, 20($sp)
	lw	$22, 24($sp)
	lw	$23, 28($sp)
	lw	$30, 32($sp)
	addiu	$sp, $sp, 40
9:
	jr	$31
	nop

/* void snes_mix_echo_fir_mono_mxu(
 *     const int *mbuf,    // $4
 *     const int *ebuf,    // $5
 *     int *echo,          // $6 (contiguous non-wrapping slice)
 *     short *out,         // $7 (4-byte aligned)
 *     int count,          // 16($sp) (even, > 0)
 *     int *win,           // 20($sp) (&win_buf[8]; win[-8..-1] primed)
 *     const int *taps8,   // 24($sp) (FilterTaps[0..7])
 *     const int *params5  // 28($sp) ({echo_fb, mvol0, mvol0, evol0, evol0})
 * );
 */
	.globl	snes_mix_echo_fir_mono_mxu
	.type	snes_mix_echo_fir_mono_mxu, @function
snes_mix_echo_fir_mono_mxu:
	lw	$8, 16($sp)		/* $8 = count */
	blez	$8, 9f
	nop
	addiu	$sp, $sp, -40
	sw	$16, 0($sp)
	sw	$17, 4($sp)
	sw	$18, 8($sp)
	sw	$19, 12($sp)
	sw	$20, 16($sp)
	sw	$21, 20($sp)
	sw	$22, 24($sp)
	sw	$23, 28($sp)
	sw	$30, 32($sp)

	lw	$30, 60($sp)		/* $30 = win (20 + 40) */
	lw	$10, 64($sp)		/* $10 = taps8 (24 + 40) */
	lw	$9,  68($sp)		/* $9  = params5 (28 + 40) */
	lw	$16, 0($10)		/* ft0 */
	lw	$17, 4($10)		/* ft1 */
	lw	$18, 8($10)		/* ft2 */
	lw	$19, 12($10)		/* ft3 */
	lw	$20, 16($10)		/* ft4 */
	lw	$21, 20($10)		/* ft5 */
	lw	$22, 24($10)		/* ft6 */
	lw	$23, 28($10)		/* ft7 */

	addiu	$7, $7, -4		/* pre-decrement out for S32SDI +4 */
	li	$10, 32767
	li	$11, -32768
	.word	0x700a03af		/* S32I2M xr14, $10 (32767) */
	.word	0x700b03ef		/* S32I2M xr15, $11 (-32768) */
1:
	/* Sample 0 8-tap FIR */
	lw	$10, 0($6)		/* E0 = echo[0] */
	lw	$11, -4($30)
	lw	$12, -8($30)
	lw	$13, -12($30)
	sw	$10, 0($30)
	mult	$10, $16
	madd	$11, $17
	madd	$12, $18
	madd	$13, $19
	lw	$11, -16($30)
	lw	$12, -20($30)
	lw	$13, -24($30)
	lw	$14, -28($30)
	madd	$11, $20
	madd	$12, $21
	madd	$13, $22
	madd	$14, $23
	mflo	$10
	sra	$10, $10, 7		/* E0 filtered */

	/* Sample 1 8-tap FIR */
	lw	$15, 4($6)		/* E1 = echo[1] */
	lw	$11, 0($30)		/* raw E0 just stored */
	lw	$12, -4($30)
	lw	$13, -8($30)
	sw	$15, 4($30)
	mult	$15, $16
	madd	$11, $17
	madd	$12, $18
	madd	$13, $19
	lw	$11, -12($30)
	lw	$12, -16($30)
	lw	$13, -20($30)
	lw	$14, -24($30)
	madd	$11, $20
	madd	$12, $21
	madd	$13, $22
	madd	$14, $23
	mflo	$15
	sra	$15, $15, 7		/* E1 filtered */

	/* Mix 2 mono samples */
	lw	$11, 0($4)		/* mbuf[0] */
	lw	$12, 4($9)		/* mvol0 */
	lw	$13, 12($9)		/* evol0 */
	mult	$11, $12
	madd	$10, $13
	mflo	$2
	lw	$11, 4($4)		/* mbuf[1] */
	mult	$11, $12
	madd	$15, $13
	mflo	$3
	sra	$2, $2, 7
	sra	$3, $3, 7
	.word	0x7002006f		/* S32I2M xr1, $2 */
	.word	0x700300af		/* S32I2M xr2, $3 */

	/* Echo feedback update interleaved with MXU clamp + pack */
	lw	$14, 0($9)		/* echo_fb */
	lw	$11, 0($5)		/* ebuf[0] */
	lw	$12, 4($5)		/* ebuf[1] */
	mul	$10, $10, $14
	mul	$15, $15, $14
	.word	0x70078443		/* S32MIN xr1, xr1, xr14 */
	.word	0x70078883		/* S32MIN xr2, xr2, xr14 */
	sra	$10, $10, 7
	sra	$15, $15, 7
	.word	0x7003c443		/* S32MAX xr1, xr1, xr15 */
	.word	0x7003c883		/* S32MAX xr2, xr2, xr15 */
	addu	$10, $10, $11
	addu	$15, $15, $12
	.word	0x731048fd		/* S32SFL xr3, xr2, xr1, xr4, ptn3 */
	sw	$10, 0($6)
	sw	$15, 4($6)
	.word	0x70e00515		/* S32SDI xr4, $7, 4 */

	addiu	$8, $8, -2
	addiu	$4, $4, 8
	addiu	$5, $5, 8
	addiu	$6, $6, 8
	bgtz	$8, 1b
	addiu	$30, $30, 8

	lw	$16, 0($sp)
	lw	$17, 4($sp)
	lw	$18, 8($sp)
	lw	$19, 12($sp)
	lw	$20, 16($sp)
	lw	$21, 20($sp)
	lw	$22, 24($sp)
	lw	$23, 28($sp)
	lw	$30, 32($sp)
	addiu	$sp, $sp, 40
9:
	jr	$31
	nop

/* void mxu_memcpy32(void *dst, const void *src, unsigned int nbytes)
 * Fast 32-byte cache-line copy using MXU xr1..xr8 (dst & src 4-byte aligned,
 * nbytes multiple of 32).
 */
	.globl	mxu_memcpy32
	.type	mxu_memcpy32, @function
mxu_memcpy32:
	srl	$6, $6, 5		/* count = nbytes / 32 */
	blez	$6, 2f
	nop
1:
	.word	0x70a00050		/* S32LDD xr1, $5, 0 */
	.word	0x70a00490		/* S32LDD xr2, $5, 4 */
	.word	0x70a008d0		/* S32LDD xr3, $5, 8 */
	.word	0x70a00d10		/* S32LDD xr4, $5, 12 */
	.word	0x70a01150		/* S32LDD xr5, $5, 16 */
	.word	0x70a01590		/* S32LDD xr6, $5, 20 */
	.word	0x70a019d0		/* S32LDD xr7, $5, 24 */
	.word	0x70a01e10		/* S32LDD xr8, $5, 28 */
	addiu	$6, $6, -1
	addiu	$5, $5, 32
	.word	0x70800051		/* S32STD xr1, $4, 0 */
	.word	0x70800491		/* S32STD xr2, $4, 4 */
	.word	0x708008d1		/* S32STD xr3, $4, 8 */
	.word	0x70800d11		/* S32STD xr4, $4, 12 */
	.word	0x70801151		/* S32STD xr5, $4, 16 */
	.word	0x70801591		/* S32STD xr6, $4, 20 */
	.word	0x708019d1		/* S32STD xr7, $4, 24 */
	.word	0x70801e11		/* S32STD xr8, $4, 28 */
	bgtz	$6, 1b
	addiu	$4, $4, 32
2:
	jr	$31
	nop

/* void mxu_memset32(void *dst, unsigned int val32, unsigned int nbytes)
 * Fast 32-byte cache-line fill using MXU xr1 (dst 4-byte aligned,
 * nbytes multiple of 32).
 */
	.globl	mxu_memset32
	.type	mxu_memset32, @function
mxu_memset32:
	srl	$6, $6, 5		/* count = nbytes / 32 */
	blez	$6, 2f
	nop
	.word	0x7005006f		/* S32I2M xr1, $5 */
1:
	.word	0x70800051		/* S32STD xr1, $4, 0 */
	.word	0x70800451		/* S32STD xr1, $4, 4 */
	.word	0x70800851		/* S32STD xr1, $4, 8 */
	.word	0x70800c51		/* S32STD xr1, $4, 12 */
	.word	0x70801051		/* S32STD xr1, $4, 16 */
	.word	0x70801451		/* S32STD xr1, $4, 20 */
	.word	0x70801851		/* S32STD xr1, $4, 24 */
	.word	0x70801c51		/* S32STD xr1, $4, 28 */
	addiu	$6, $6, -1
	bgtz	$6, 1b
	addiu	$4, $4, 32
2:
	jr	$31
	nop
