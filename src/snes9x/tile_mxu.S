/*
 * Ingenic JZ4760B MXU 1.0 SIMD kernels for PocketSNES+ PPU color-math blending.
 *
 * All MXU opcodes use exact .word encodings cross-checked against Ingenic's
 * official mxu_as assembler and silicon-verified soundux_mxu.S encodings.
 * Only invoked when snes_use_mxu != 0 (after jz_mxu_enable()).
 *
 * All four functions are leaf functions with 4 register arguments and zero stack frame:
 *   void snes_sub_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
 *   void snes_sub_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
 *   void snes_sub1_2_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
 *   void snes_sub1_2_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
 */
	.text
	.set	noreorder

/* void snes_sub_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
 * $4 = dst (2-byte aligned), $5 = sub (2-byte aligned), $6 = p01, $7 = p23
 * Computes exact per-channel saturating RGB565 max(p - s, 0) for 4 pixels in parallel.
 */
	.globl	snes_sub_4px_mem_mxu
	.type	snes_sub_4px_mem_mxu, @function
snes_sub_4px_mem_mxu:
	lwl	$2, 3($5)
	lwr	$2, 0($5)		/* $2 = s01 */
	lwl	$3, 7($5)
	lwr	$3, 4($5)		/* $3 = s23 */
	lui	$14, 0xf800
	ori	$14, $14, 0xf800	/* mR = 0xF800F800 */
	lui	$15, 0x07e0
	ori	$15, $15, 0x07e0	/* mG = 0x07E007E0 */
	lui	$24, 0x001f
	ori	$24, $24, 0x001f	/* mB = 0x001F001F */

	/* R channel (shifted >> 1 to bits 14..10 so positive signed int16) */
	and	$8,  $6, $14
	and	$9,  $7, $14
	and	$10, $2, $14
	and	$11, $3, $14
	srl	$8,  $8, 1
	srl	$9,  $9, 1
	srl	$10, $10, 1
	srl	$11, $11, 1
	.word	0x7008006f		/* S32I2M xr1, $8  */
	.word	0x700900af		/* S32I2M xr2, $9  */
	.word	0x700a00ef		/* S32I2M xr3, $10 */
	.word	0x700b012f		/* S32I2M xr4, $11 */

	/* G channel interleaved with R subtract + clamp */
	and	$8,  $6, $15
	and	$9,  $7, $15
	.word	0x7300c44e		/* Q16ADD xr1, xr1, xr3, xr0, SS, WW */
	.word	0x7301088e		/* Q16ADD xr2, xr2, xr4, xr0, SS, WW */
	and	$10, $2, $15
	and	$11, $3, $15
	.word	0x7008016f		/* S32I2M xr5, $8  */
	.word	0x700901af		/* S32I2M xr6, $9  */
	.word	0x70080443		/* D16MAX xr1, xr1, xr0 */
	.word	0x70080883		/* D16MAX xr2, xr2, xr0 */
	.word	0x700a01ef		/* S32I2M xr7, $10 */
	.word	0x700b022f		/* S32I2M xr8, $11 */

	/* B channel interleaved with G subtract + R << 1 + G clamp */
	and	$8,  $6, $24
	and	$9,  $7, $24
	.word	0x7301d54e		/* Q16ADD xr5, xr5, xr7, xr0, SS, WW */
	.word	0x7302198e		/* Q16ADD xr6, xr6, xr8, xr0, SS, WW */
	and	$10, $2, $24
	and	$11, $3, $24
	.word	0x70488474		/* Q16SLL xr1, xr1, xr2, xr2, 1 (4x R << 1) */
	.word	0x7008026f		/* S32I2M xr9,  $8  */
	.word	0x700902af		/* S32I2M xr10, $9  */
	.word	0x70081543		/* D16MAX xr5, xr5, xr0 */
	.word	0x70081983		/* D16MAX xr6, xr6, xr0 */
	.word	0x700a02ef		/* S32I2M xr11, $10 */
	.word	0x700b032f		/* S32I2M xr12, $11 */

	/* B subtract + clamp interleaved with R/G/B recombination */
	.word	0x7302e64e		/* Q16ADD xr9,  xr9,  xr11, xr0, SS, WW */
	.word	0x73032a8e		/* Q16ADD xr10, xr10, xr12, xr0, SS, WW */
	.word	0x7008006e		/* S32M2I xr1, $8  (R01) */
	.word	0x700900ae		/* S32M2I xr2, $9  (R23) */
	.word	0x70082643		/* D16MAX xr9,  xr9,  xr0 */
	.word	0x70082a83		/* D16MAX xr10, xr10, xr0 */
	.word	0x700a016e		/* S32M2I xr5, $10 (G01) */
	.word	0x700b01ae		/* S32M2I xr6, $11 (G23) */
	or	$8,  $8,  $10
	or	$9,  $9,  $11
	.word	0x700c026e		/* S32M2I xr9,  $12 (B01) */
	.word	0x700d02ae		/* S32M2I xr10, $13 (B23) */
	or	$8,  $8,  $12
	or	$9,  $9,  $13

	swl	$8, 3($4)
	swr	$8, 0($4)
	swl	$9, 7($4)
	jr	$31
	swr	$9, 4($4)


/* void snes_sub_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
 * $4 = dst, $5 = fixed2 (fixed_col | (fixed_col << 16)), $6 = p01, $7 = p23
 */
	.globl	snes_sub_4px_fixed_mxu
	.type	snes_sub_4px_fixed_mxu, @function
snes_sub_4px_fixed_mxu:
	lui	$14, 0xf800
	ori	$14, $14, 0xf800
	lui	$15, 0x07e0
	ori	$15, $15, 0x07e0
	lui	$24, 0x001f
	ori	$24, $24, 0x001f

	and	$8,  $6, $14
	and	$9,  $7, $14
	and	$10, $5, $14
	srl	$8,  $8, 1
	srl	$9,  $9, 1
	srl	$10, $10, 1
	.word	0x7008006f		/* S32I2M xr1, $8  */
	.word	0x700900af		/* S32I2M xr2, $9  */
	.word	0x700a00ef		/* S32I2M xr3, $10 */

	and	$8,  $6, $15
	and	$9,  $7, $15
	.word	0x7300c44e		/* Q16ADD xr1, xr1, xr3, xr0, SS, WW */
	.word	0x7300c88e		/* Q16ADD xr2, xr2, xr3, xr0, SS, WW */
	and	$10, $5, $15
	.word	0x7008016f		/* S32I2M xr5, $8  */
	.word	0x700901af		/* S32I2M xr6, $9  */
	.word	0x70080443		/* D16MAX xr1, xr1, xr0 */
	.word	0x70080883		/* D16MAX xr2, xr2, xr0 */
	.word	0x700a01ef		/* S32I2M xr7, $10 */

	and	$8,  $6, $24
	and	$9,  $7, $24
	.word	0x7301d54e		/* Q16ADD xr5, xr5, xr7, xr0, SS, WW */
	.word	0x7301d98e		/* Q16ADD xr6, xr6, xr7, xr0, SS, WW */
	and	$10, $5, $24
	.word	0x70488474		/* Q16SLL xr1, xr1, xr2, xr2, 1 */
	.word	0x7008026f		/* S32I2M xr9,  $8  */
	.word	0x700902af		/* S32I2M xr10, $9  */
	.word	0x70081543		/* D16MAX xr5, xr5, xr0 */
	.word	0x70081983		/* D16MAX xr6, xr6, xr0 */
	.word	0x700a02ef		/* S32I2M xr11, $10 */

	.word	0x7302e64e		/* Q16ADD xr9,  xr9,  xr11, xr0, SS, WW */
	.word	0x7302ea8e		/* Q16ADD xr10, xr10, xr11, xr0, SS, WW */
	.word	0x7008006e		/* S32M2I xr1, $8  */
	.word	0x700900ae		/* S32M2I xr2, $9  */
	.word	0x70082643		/* D16MAX xr9,  xr9,  xr0 */
	.word	0x70082a83		/* D16MAX xr10, xr10, xr0 */
	.word	0x700a016e		/* S32M2I xr5, $10 */
	.word	0x700b01ae		/* S32M2I xr6, $11 */
	or	$8,  $8,  $10
	or	$9,  $9,  $11
	.word	0x700c026e		/* S32M2I xr9,  $12 */
	.word	0x700d02ae		/* S32M2I xr10, $13 */
	or	$8,  $8,  $12
	or	$9,  $9,  $13

	swl	$8, 3($4)
	swr	$8, 0($4)
	swl	$9, 7($4)
	jr	$31
	swr	$9, 4($4)


/* void snes_sub1_2_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
 * Computes exact per-channel saturating RGB565 (max(p - s, 0) >> 1) for 4 pixels in parallel.
 */
	.globl	snes_sub1_2_4px_mem_mxu
	.type	snes_sub1_2_4px_mem_mxu, @function
snes_sub1_2_4px_mem_mxu:
	lwl	$2, 3($5)
	lwr	$2, 0($5)		/* $2 = s01 */
	lwl	$3, 7($5)
	lwr	$3, 4($5)		/* $3 = s23 */
	lui	$14, 0xf800
	ori	$14, $14, 0xf800
	lui	$15, 0x07e0
	ori	$15, $15, 0x07e0
	lui	$24, 0x001f
	ori	$24, $24, 0x001f

	and	$8,  $6, $14
	and	$9,  $7, $14
	and	$10, $2, $14
	and	$11, $3, $14
	srl	$8,  $8, 1
	srl	$9,  $9, 1
	srl	$10, $10, 1
	srl	$11, $11, 1
	.word	0x7008006f		/* S32I2M xr1, $8  */
	.word	0x700900af		/* S32I2M xr2, $9  */
	.word	0x700a00ef		/* S32I2M xr3, $10 */
	.word	0x700b012f		/* S32I2M xr4, $11 */

	and	$8,  $6, $15
	and	$9,  $7, $15
	.word	0x7300c44e		/* Q16ADD xr1, xr1, xr3, xr0, SS, WW */
	.word	0x7301088e		/* Q16ADD xr2, xr2, xr4, xr0, SS, WW */
	and	$10, $2, $15
	and	$11, $3, $15
	.word	0x7008016f		/* S32I2M xr5, $8  */
	.word	0x700901af		/* S32I2M xr6, $9  */
	.word	0x70080443		/* D16MAX xr1, xr1, xr0 (R_diff in bits 14..10) */
	.word	0x70080883		/* D16MAX xr2, xr2, xr0 */
	.word	0x700a01ef		/* S32I2M xr7, $10 */
	.word	0x700b022f		/* S32I2M xr8, $11 */

	and	$8,  $6, $24
	and	$9,  $7, $24
	.word	0x7301d54e		/* Q16ADD xr5, xr5, xr7, xr0, SS, WW */
	.word	0x7302198e		/* Q16ADD xr6, xr6, xr8, xr0, SS, WW */
	and	$10, $2, $24
	and	$11, $3, $24
	.word	0x7008026f		/* S32I2M xr9,  $8  */
	.word	0x700902af		/* S32I2M xr10, $9  */
	.word	0x70081543		/* D16MAX xr5, xr5, xr0 */
	.word	0x70081983		/* D16MAX xr6, xr6, xr0 */
	.word	0x700a02ef		/* S32I2M xr11, $10 */
	.word	0x700b032f		/* S32I2M xr12, $11 */

	.word	0x7302e64e		/* Q16ADD xr9,  xr9,  xr11, xr0, SS, WW */
	.word	0x73032a8e		/* Q16ADD xr10, xr10, xr12, xr0, SS, WW */
	.word	0x70599575		/* Q16SLR xr5, xr5, xr6, xr6, 1 (4x G >> 1) */
	.word	0x7008006e		/* S32M2I xr1, $8  (R_diff << 10) */
	.word	0x700900ae		/* S32M2I xr2, $9  */
	.word	0x70082643		/* D16MAX xr9,  xr9,  xr0 */
	.word	0x70082a83		/* D16MAX xr10, xr10, xr0 */
	and	$8,  $8,  $14		/* R_half in bits 14..11 (exact!) */
	and	$9,  $9,  $14
	.word	0x706aa675		/* Q16SLR xr9, xr9, xr10, xr10, 1 (4x B >> 1) */
	.word	0x700a016e		/* S32M2I xr5, $10 */
	.word	0x700b01ae		/* S32M2I xr6, $11 */
	and	$10, $10, $15		/* G_half in bits 9..5 (exact!) */
	and	$11, $11, $15
	or	$8,  $8,  $10
	or	$9,  $9,  $11
	.word	0x700c026e		/* S32M2I xr9,  $12 (B_half in bits 3..0) */
	.word	0x700d02ae		/* S32M2I xr10, $13 */
	or	$8,  $8,  $12
	or	$9,  $9,  $13

	swl	$8, 3($4)
	swr	$8, 0($4)
	swl	$9, 7($4)
	jr	$31
	swr	$9, 4($4)


/* void snes_sub1_2_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23) */
	.globl	snes_sub1_2_4px_fixed_mxu
	.type	snes_sub1_2_4px_fixed_mxu, @function
snes_sub1_2_4px_fixed_mxu:
	lui	$14, 0xf800
	ori	$14, $14, 0xf800
	lui	$15, 0x07e0
	ori	$15, $15, 0x07e0
	lui	$24, 0x001f
	ori	$24, $24, 0x001f

	and	$8,  $6, $14
	and	$9,  $7, $14
	and	$10, $5, $14
	srl	$8,  $8, 1
	srl	$9,  $9, 1
	srl	$10, $10, 1
	.word	0x7008006f		/* S32I2M xr1, $8  */
	.word	0x700900af		/* S32I2M xr2, $9  */
	.word	0x700a00ef		/* S32I2M xr3, $10 */

	and	$8,  $6, $15
	and	$9,  $7, $15
	.word	0x7300c44e		/* Q16ADD xr1, xr1, xr3, xr0, SS, WW */
	.word	0x7300c88e		/* Q16ADD xr2, xr2, xr3, xr0, SS, WW */
	and	$10, $5, $15
	.word	0x7008016f		/* S32I2M xr5, $8  */
	.word	0x700901af		/* S32I2M xr6, $9  */
	.word	0x70080443		/* D16MAX xr1, xr1, xr0 */
	.word	0x70080883		/* D16MAX xr2, xr2, xr0 */
	.word	0x700a01ef		/* S32I2M xr7, $10 */

	and	$8,  $6, $24
	and	$9,  $7, $24
	.word	0x7301d54e		/* Q16ADD xr5, xr5, xr7, xr0, SS, WW */
	.word	0x7301d98e		/* Q16ADD xr6, xr6, xr7, xr0, SS, WW */
	and	$10, $5, $24
	.word	0x7008026f		/* S32I2M xr9,  $8  */
	.word	0x700902af		/* S32I2M xr10, $9  */
	.word	0x70081543		/* D16MAX xr5, xr5, xr0 */
	.word	0x70081983		/* D16MAX xr6, xr6, xr0 */
	.word	0x700a02ef		/* S32I2M xr11, $10 */

	.word	0x7302e64e		/* Q16ADD xr9,  xr9,  xr11, xr0, SS, WW */
	.word	0x7302ea8e		/* Q16ADD xr10, xr10, xr11, xr0, SS, WW */
	.word	0x70599575		/* Q16SLR xr5, xr5, xr6, xr6, 1 */
	.word	0x7008006e		/* S32M2I xr1, $8  */
	.word	0x700900ae		/* S32M2I xr2, $9  */
	.word	0x70082643		/* D16MAX xr9,  xr9,  xr0 */
	.word	0x70082a83		/* D16MAX xr10, xr10, xr0 */
	and	$8,  $8,  $14
	and	$9,  $9,  $14
	.word	0x706aa675		/* Q16SLR xr9, xr9, xr10, xr10, 1 */
	.word	0x700a016e		/* S32M2I xr5, $10 */
	.word	0x700b01ae		/* S32M2I xr6, $11 */
	and	$10, $10, $15
	and	$11, $11, $15
	or	$8,  $8,  $10
	or	$9,  $9,  $11
	.word	0x700c026e		/* S32M2I xr9,  $12 */
	.word	0x700d02ae		/* S32M2I xr10, $13 */
	or	$8,  $8,  $12
	or	$9,  $9,  $13

	swl	$8, 3($4)
	swr	$8, 0($4)
	swl	$9, 7($4)
	jr	$31
	swr	$9, 4($4)
