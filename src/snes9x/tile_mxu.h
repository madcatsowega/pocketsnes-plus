#ifndef _TILE_MXU_H_
#define _TILE_MXU_H_

#include "snes9x.h"
#include "gfx.h"

#ifdef MIPS_XBURST
extern int snes_use_mxu;
extern "C" {
void snes_sub_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23);
void snes_sub_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23);
void snes_sub1_2_4px_mem_mxu(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23);
void snes_sub1_2_4px_fixed_mxu(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23);
}
#endif

struct __attribute__((packed)) tile_u32_ua { uint32 v; };

static inline uint32 tile_load_u32_ua(const void *p)
{
	return ((const tile_u32_ua *)p)->v;
}

static inline void tile_store_u32_ua(void *p, uint32 v)
{
	((tile_u32_ua *)p)->v = v;
}

static inline void tile_store_4px_fast(uint16 *dst, uint32 p01, uint32 p23)
{
	/* Branchless 16-bit stores into cached L1 write-back framebuffer:
	 * avoids 50/50 mispredicted alignment branches during HDMA horizontal scroll. */
	dst[0] = (uint16)p01;
	dst[1] = (uint16)(p01 >> 16);
	dst[2] = (uint16)p23;
	dst[3] = (uint16)(p23 >> 16);
}

/* 2-pixel branchless SWAR RGB565 saturating add (100% bit-exact with COLOR_ADD) */
static inline uint32 snes_color_add_2px(uint32 p1, uint32 p2)
{
	uint32 x   = ((p1 & 0xF7DEF7DEu) >> 1) + ((p2 & 0xF7DEF7DEu) >> 1) + (p1 & p2 & 0x08210821u);
	uint32 ov  = x & 0x84108410u;
	uint32 ov4 = ov >> 4;
	uint32 sat = ((ov - ov4) << 1) | ov4 | ((ov & 0x04000400u) >> 5);
	return (((x & ~ov) << 1) | sat) | ((p1 ^ p2) & 0x08210821u);
}

/* 2-pixel branchless SWAR RGB565 half-add (100% bit-exact with COLOR_ADD1_2, 3 cycles/pixel) */
static inline uint32 snes_color_add1_2_2px(uint32 p1, uint32 p2)
{
	return ((p1 & 0xF7DEF7DEu) >> 1) + ((p2 & 0xF7DEF7DEu) >> 1) + (p1 & p2 & 0x08210821u);
}

static inline void snes_add_4px_mem(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
{
	uint32 s01 = tile_load_u32_ua(sub);
	uint32 s23 = tile_load_u32_ua(sub + 2);
	tile_store_4px_fast(dst, snes_color_add_2px(p01, s01), snes_color_add_2px(p23, s23));
}

static inline void snes_add_4px_fixed(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
{
	tile_store_4px_fast(dst, snes_color_add_2px(p01, fixed2), snes_color_add_2px(p23, fixed2));
}

static inline void snes_add1_2_4px_mem(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
{
	uint32 s01 = tile_load_u32_ua(sub);
	uint32 s23 = tile_load_u32_ua(sub + 2);
	tile_store_4px_fast(dst, snes_color_add1_2_2px(p01, s01), snes_color_add1_2_2px(p23, s23));
}

static inline void snes_add1_2_4px_fixed(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
{
	tile_store_4px_fast(dst, snes_color_add1_2_2px(p01, fixed2), snes_color_add1_2_2px(p23, fixed2));
}

static inline void snes_sub_4px_mem(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
{
#ifdef MIPS_XBURST
	if (__builtin_expect(snes_use_mxu, 1)) {
		snes_sub_4px_mem_mxu(dst, sub, p01, p23);
		return;
	}
#endif
	dst[0] = COLOR_SUB((uint16)p01,         sub[0]);
	dst[1] = COLOR_SUB((uint16)(p01 >> 16), sub[1]);
	dst[2] = COLOR_SUB((uint16)p23,         sub[2]);
	dst[3] = COLOR_SUB((uint16)(p23 >> 16), sub[3]);
}

static inline void snes_sub_4px_fixed(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
{
#ifdef MIPS_XBURST
	if (__builtin_expect(snes_use_mxu, 1)) {
		snes_sub_4px_fixed_mxu(dst, fixed2, p01, p23);
		return;
	}
#endif
	uint16 f = (uint16)fixed2;
	dst[0] = COLOR_SUB((uint16)p01,         f);
	dst[1] = COLOR_SUB((uint16)(p01 >> 16), f);
	dst[2] = COLOR_SUB((uint16)p23,         f);
	dst[3] = COLOR_SUB((uint16)(p23 >> 16), f);
}

static inline void snes_sub1_2_4px_mem(uint16 *dst, const uint16 *sub, uint32 p01, uint32 p23)
{
#ifdef MIPS_XBURST
	if (__builtin_expect(snes_use_mxu, 1)) {
		snes_sub1_2_4px_mem_mxu(dst, sub, p01, p23);
		return;
	}
#endif
	dst[0] = COLOR_SUB1_2((uint16)p01,         sub[0]);
	dst[1] = COLOR_SUB1_2((uint16)(p01 >> 16), sub[1]);
	dst[2] = COLOR_SUB1_2((uint16)p23,         sub[2]);
	dst[3] = COLOR_SUB1_2((uint16)(p23 >> 16), sub[3]);
}

static inline void snes_sub1_2_4px_fixed(uint16 *dst, uint32 fixed2, uint32 p01, uint32 p23)
{
#ifdef MIPS_XBURST
	if (__builtin_expect(snes_use_mxu, 1)) {
		snes_sub1_2_4px_fixed_mxu(dst, fixed2, p01, p23);
		return;
	}
#endif
	uint16 f = (uint16)fixed2;
	dst[0] = COLOR_SUB1_2((uint16)p01,         f);
	dst[1] = COLOR_SUB1_2((uint16)(p01 >> 16), f);
	dst[2] = COLOR_SUB1_2((uint16)p23,         f);
	dst[3] = COLOR_SUB1_2((uint16)(p23 >> 16), f);
}

#endif /* _TILE_MXU_H_ */
