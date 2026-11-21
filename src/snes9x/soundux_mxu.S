/*
 * Ingenic JZ4760B MXU SIMD kernels for PocketSNES+ audio mixing and blitting.
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

/* void mxu_memcpy32(void *dst, const void *src, unsigned int nbytes)
 * Fast 32-byte cache-line copy using MXU xr1..xr8 (dst & src 4-byte aligned,
 * nbytes multiple of 32).
 * a0 ($4) = dst
 * a1 ($5) = src
 * a2 ($6) = nbytes
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
