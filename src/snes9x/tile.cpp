/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
#include "snes9x.h"

#include "memmap.h"
#include "ppu.h"
#include "display.h"
#include "gfx.h"
#include "tile.h"

static uint8 ConvertTile (uint8 *pCache, uint32 TileAddr)
{
    register uint8 *tp = &Memory.VRAM[TileAddr];
    uint32 *p = (uint32 *) pCache;
    uint32 non_zero = 0;
    uint8 line;

    switch (BG.BitShift)
    {
    case 8:
	for (line = 8; line != 0; line--, tp += 2)
	{
	    uint32 p1 = 0;
	    uint32 p2 = 0;
	    register uint8 pix;

	    if ((pix = *(tp + 0)))
	    {
		p1 |= odd_high[0][pix >> 4];
		p2 |= odd_low[0][pix & 0xf];
	    }
	    if ((pix = *(tp + 1)))
	    {
		p1 |= even_high[0][pix >> 4];
		p2 |= even_low[0][pix & 0xf];
	    }
	    if ((pix = *(tp + 16)))
	    {
		p1 |= odd_high[1][pix >> 4];
		p2 |= odd_low[1][pix & 0xf];
	    }
	    if ((pix = *(tp + 17)))
	    {
		p1 |= even_high[1][pix >> 4];
		p2 |= even_low[1][pix & 0xf];
	    }
	    if ((pix = *(tp + 32)))
	    {
		p1 |= odd_high[2][pix >> 4];
		p2 |= odd_low[2][pix & 0xf];
	    }
	    if ((pix = *(tp + 33)))
	    {
		p1 |= even_high[2][pix >> 4];
		p2 |= even_low[2][pix & 0xf];
	    }
	    if ((pix = *(tp + 48)))
	    {
		p1 |= odd_high[3][pix >> 4];
		p2 |= odd_low[3][pix & 0xf];
	    }
	    if ((pix = *(tp + 49)))
	    {
		p1 |= even_high[3][pix >> 4];
		p2 |= even_low[3][pix & 0xf];
	    }
	    *p++ = p1;
	    *p++ = p2;
	    non_zero |= p1 | p2;
	}
	break;

    case 4:
	for (line = 8; line != 0; line--, tp += 2)
	{
	    uint32 p1 = 0;
	    uint32 p2 = 0;
	    register uint8 pix;
	    if ((pix = *(tp + 0)))
	    {
		p1 |= odd_high[0][pix >> 4];
		p2 |= odd_low[0][pix & 0xf];
	    }
	    if ((pix = *(tp + 1)))
	    {
		p1 |= even_high[0][pix >> 4];
		p2 |= even_low[0][pix & 0xf];
	    }
	    if ((pix = *(tp + 16)))
	    {
		p1 |= odd_high[1][pix >> 4];
		p2 |= odd_low[1][pix & 0xf];
	    }
	    if ((pix = *(tp + 17)))
	    {
		p1 |= even_high[1][pix >> 4];
		p2 |= even_low[1][pix & 0xf];
	    }
	    *p++ = p1;
	    *p++ = p2;
	    non_zero |= p1 | p2;
	}
	break;

    case 2:
	for (line = 8; line != 0; line--, tp += 2)
	{
	    uint32 p1 = 0;
	    uint32 p2 = 0;
	    register uint8 pix;
	    if ((pix = *(tp + 0)))
	    {
		p1 |= odd_high[0][pix >> 4];
		p2 |= odd_low[0][pix & 0xf];
	    }
	    if ((pix = *(tp + 1)))
	    {
		p1 |= even_high[0][pix >> 4];
		p2 |= even_low[0][pix & 0xf];
	    }
	    *p++ = p1;
	    *p++ = p2;
	    non_zero |= p1 | p2;
	}
	break;
    }
    return (non_zero ? TRUE : BLANK_TILE);
}
#define PLOT_PIXEL(screen, pixel) (pixel)

#ifndef FOREVER_16_BIT
static void WRITE_4PIXELS (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N] = (uint8) ScreenColors [Pixel];
			Depth [N] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS_FLIPPED (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[3 - N]))
		{
			Screen [N] = (uint8) ScreenColors [Pixel];
			Depth [N] = GFX.Z2;
		}
	}
}
static void WRITE_4PIXELS_HALFWIDTH (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N += 2)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N >> 1] = (uint8) ScreenColors [Pixel];
			Depth [N >> 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS_FLIPPED_HALFWIDTH (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N += 2)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[2 - N]))
		{
			Screen [N >> 1] = (uint8) ScreenColors [Pixel];
			Depth [N >> 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELSx2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = (uint8) ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS_FLIPPEDx2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = (uint8) ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELSx2x2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = Screen [GFX.RealPitch + N * 2] = Screen [GFX.RealPitch + N * 2 + 1] = (uint8) ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = Depth [GFX.RealPitch + N * 2] = Depth [GFX.RealPitch + N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS_FLIPPEDx2x2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8 Pixel;
	uint8 *Screen = GFX.S + Offset;
	uint8 *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N * 2] && (Pixel = Pixels[3 - N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = Screen [GFX.RealPitch + N * 2] = Screen [GFX.RealPitch + N * 2 + 1] = (uint8) ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = Depth [GFX.RealPitch + N * 2] = Depth [GFX.RealPitch + N * 2 + 1] = GFX.Z2;
		}
	}
}

void DrawTile (uint32 Tile, uint32 Offset, uint32 StartLine,
	       uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS, WRITE_4PIXELS_FLIPPED, 4)
}

void DrawClippedTile (uint32 Tile, uint32 Offset,
		      uint32 StartPixel, uint32 Width,
		      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS, WRITE_4PIXELS_FLIPPED, 4)
}

void DrawTileHalfWidth (uint32 Tile, uint32 Offset, uint32 StartLine,
	       uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS_HALFWIDTH, WRITE_4PIXELS_FLIPPED_HALFWIDTH, 2)
}

void DrawClippedTileHalfWidth (uint32 Tile, uint32 Offset,
		      uint32 StartPixel, uint32 Width,
		      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS_HALFWIDTH, WRITE_4PIXELS_FLIPPED_HALFWIDTH, 2)
}

void DrawTilex2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		 uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELSx2, WRITE_4PIXELS_FLIPPEDx2, 8)
}

void DrawClippedTilex2 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELSx2, WRITE_4PIXELS_FLIPPEDx2, 8)
}

void DrawTilex2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELSx2x2, WRITE_4PIXELS_FLIPPEDx2x2, 8)
}

void DrawClippedTilex2x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELSx2x2, WRITE_4PIXELS_FLIPPEDx2x2, 8)
}

void DrawLargePixel (uint32 Tile, uint32 Offset,
		     uint32 StartPixel, uint32 Pixels,
		     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *sp = GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint8 pixel;

    RENDER_TILE_LARGE (((uint8) ScreenColors [pixel]), PLOT_PIXEL)
}

void DrawLargePixelHalfWidth (uint32 Tile, uint32 Offset,
		     uint32 StartPixel, uint32 Pixels,
		     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint8 *sp = GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint8 pixel;

    RENDER_TILE_LARGE_HALFWIDTH (((uint8) ScreenColors [pixel]), PLOT_PIXEL)
}
#endif

static inline void WRITE_4PIXELS16 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
#if defined(__MIPSEL) && defined(__GNUC__) && !defined(NO_ASM)
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;
	uint8  Pixel_A, Pixel_B, Pixel_C, Pixel_D;
	uint8  Depth_A, Depth_B, Depth_C, Depth_D;
	bool8  Cond;
	uint32 Temp;
	__asm__ __volatile__ (
		".set noreorder                        \n"
		"   lbu   %[In8A], 0(%[In8])           \n"
		"   lbu   %[In8B], 1(%[In8])           \n"
		"   lbu   %[In8C], 2(%[In8])           \n"
		"   lbu   %[In8D], 3(%[In8])           \n"
		"   lbu   %[ZA], 0(%[Z])               \n"
		"   lbu   %[ZB], 1(%[Z])               \n"
		"   lbu   %[ZC], 2(%[Z])               \n"
		"   lbu   %[ZD], 3(%[Z])               \n"
		/* If In8A is non-zero (opaque) and ZCompare > ZA, write the pixel to
		 * the screen from the palette. */
		"   sltiu %[Temp], %[In8A], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZA]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		/* Otherwise skip to the next pixel, B. */
		"   bne   %[Cond], $0, 2f              \n"
		/* Load the address of the palette entry (16-bit) corresponding to
		 * this pixel (partially in the delay slot). */
		"   sll   %[In8A], %[In8A], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8A] \n"
		/* Load the palette entry. While that's being done, store the new
		 * depth for this pixel. Then store to the screen. */
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 0(%[Z])             \n"
		"   sh    %[Temp], 0(%[Out16])         \n"
		/* Now do the same for pixel B. */
		"2: sltiu %[Temp], %[In8B], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZB]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 3f              \n"
		"   sll   %[In8B], %[In8B], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8B] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 1(%[Z])             \n"
		"   sh    %[Temp], 2(%[Out16])         \n"
		/* Now do the same for pixel C. */
		"3: sltiu %[Temp], %[In8C], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZC]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 4f              \n"
		"   sll   %[In8C], %[In8C], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8C] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 2(%[Z])             \n"
		"   sh    %[Temp], 4(%[Out16])         \n"
		/* Now do the same for pixel D. */
		"4: sltiu %[Temp], %[In8D], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZD]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 5f              \n"
		"   sll   %[In8D], %[In8D], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8D] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 3(%[Z])             \n"
		"   sh    %[Temp], 6(%[Out16])         \n"
		"5:                                    \n"
		".set reorder                          \n"
		: /* output */  [In8A] "=&r" (Pixel_A), [In8B] "=&r" (Pixel_B), [In8C] "=&r" (Pixel_C), [In8D] "=&r" (Pixel_D), [ZA] "=&r" (Depth_A), [ZB] "=&r" (Depth_B), [ZC] "=&r" (Depth_C), [ZD] "=&r" (Depth_D), [Cond] "=&r" (Cond), [Temp] "=&r" (Temp)
		: /* input */   [Out16] "r" (Screen), [Z] "r" (Depth), [In8] "r" (Pixels), [Palette] "r" (ScreenColors), [ZCompare] "r" (GFX.Z1), [ZSet] "r" (GFX.Z2)
		: /* clobber */ "memory"
	);
#else
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N] = ScreenColors [Pixel];
			Depth [N] = GFX.Z2;
		}
	}
#endif
}

static inline void WRITE_4PIXELS16_FLIPPED (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
#if defined(__MIPSEL) && defined(__GNUC__) && !defined(NO_ASM)
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;
	uint8  Pixel_A, Pixel_B, Pixel_C, Pixel_D;
	uint8  Depth_A, Depth_B, Depth_C, Depth_D;
	bool8  Cond;
	uint32 Temp;
	__asm__ __volatile__ (
		".set noreorder                        \n"
		"   lbu   %[In8A], 3(%[In8])           \n"
		"   lbu   %[In8B], 2(%[In8])           \n"
		"   lbu   %[In8C], 1(%[In8])           \n"
		"   lbu   %[In8D], 0(%[In8])           \n"
		"   lbu   %[ZA], 0(%[Z])               \n"
		"   lbu   %[ZB], 1(%[Z])               \n"
		"   lbu   %[ZC], 2(%[Z])               \n"
		"   lbu   %[ZD], 3(%[Z])               \n"
		/* If In8A is non-zero (opaque) and ZCompare > ZA, write the pixel to
		 * the screen from the palette. */
		"   sltiu %[Temp], %[In8A], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZA]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		/* Otherwise skip to the next pixel, B. */
		"   bne   %[Cond], $0, 2f              \n"
		/* Load the address of the palette entry (16-bit) corresponding to
		 * this pixel (partially in the delay slot). */
		"   sll   %[In8A], %[In8A], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8A] \n"
		/* Load the palette entry. While that's being done, store the new
		 * depth for this pixel. Then store to the screen. */
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 0(%[Z])             \n"
		"   sh    %[Temp], 0(%[Out16])         \n"
		/* Now do the same for pixel B. */
		"2: sltiu %[Temp], %[In8B], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZB]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 3f              \n"
		"   sll   %[In8B], %[In8B], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8B] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 1(%[Z])             \n"
		"   sh    %[Temp], 2(%[Out16])         \n"
		/* Now do the same for pixel C. */
		"3: sltiu %[Temp], %[In8C], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZC]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 4f              \n"
		"   sll   %[In8C], %[In8C], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8C] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 2(%[Z])             \n"
		"   sh    %[Temp], 4(%[Out16])         \n"
		/* Now do the same for pixel D. */
		"4: sltiu %[Temp], %[In8D], 1          \n"
		"   sltu  %[Cond], %[ZCompare], %[ZD]  \n"
		"   or    %[Cond], %[Cond], %[Temp]    \n"
		"   bne   %[Cond], $0, 5f              \n"
		"   sll   %[In8D], %[In8D], 1          \n"
		"   addu  %[Temp], %[Palette], %[In8D] \n"
		"   lhu   %[Temp], 0(%[Temp])          \n"
		"   sb    %[ZSet], 3(%[Z])             \n"
		"   sh    %[Temp], 6(%[Out16])         \n"
		"5:                                    \n"
		".set reorder                          \n"
		: /* output */  [In8A] "=&r" (Pixel_A), [In8B] "=&r" (Pixel_B), [In8C] "=&r" (Pixel_C), [In8D] "=&r" (Pixel_D), [ZA] "=&r" (Depth_A), [ZB] "=&r" (Depth_B), [ZC] "=&r" (Depth_C), [ZD] "=&r" (Depth_D), [Cond] "=&r" (Cond), [Temp] "=&r" (Temp)
		: /* input */   [Out16] "r" (Screen), [Z] "r" (Depth), [In8] "r" (Pixels), [Palette] "r" (ScreenColors), [ZCompare] "r" (GFX.Z1), [ZSet] "r" (GFX.Z2)
		: /* clobber */ "memory"
	);
#else
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[3 - N]))
		{
			Screen [N] = ScreenColors [Pixel];
			Depth [N] = GFX.Z2;
		}
	}
#endif
}

static void WRITE_4PIXELS16_HALFWIDTH (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N += 2)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N >> 1] = ScreenColors [Pixel];
			Depth [N >> 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS16_FLIPPED_HALFWIDTH (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N += 2)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[2 - N]))
		{
			Screen [N >> 1] = ScreenColors [Pixel];
			Depth [N >> 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS16x2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS16_FLIPPEDx2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[3 - N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS16x2x2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = Screen [(GFX.RealPitch >> 1) + N * 2] = Screen [(GFX.RealPitch >> 1) + N * 2 + 1] = ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = Depth [(GFX.RealPitch >> 1) + N * 2] = Depth [(GFX.RealPitch >> 1) + N * 2 + 1] = GFX.Z2;
		}
	}
}

static void WRITE_4PIXELS16_FLIPPEDx2x2 (uint32 Offset, uint8 *Pixels, uint16 *ScreenColors)
{
	uint8  Pixel;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth = GFX.DB + Offset;

	for (uint8 N = 0; N < 4; N++)
	{
		if (GFX.Z1 > Depth [N] && (Pixel = Pixels[3 - N]))
		{
			Screen [N * 2] = Screen [N * 2 + 1] = Screen [(GFX.RealPitch >> 1) + N * 2] = Screen [(GFX.RealPitch >> 1) + N * 2 + 1] = ScreenColors [Pixel];
			Depth [N * 2] = Depth [N * 2 + 1] = Depth [(GFX.RealPitch >> 1) + N * 2] = Depth [(GFX.RealPitch >> 1) + N * 2 + 1] = GFX.Z2;
		}
	}
}

struct __attribute__((packed)) tile_u32_unaligned { uint32 v; };

static inline uint32 tile_load_u32_unaligned(const uint8 *p)
{
	return ((const tile_u32_unaligned *)p)->v;
}

static inline void tile_store_u32_unaligned(uint8 *p, uint32 v)
{
	((tile_u32_unaligned *)p)->v = v;
}

static void __attribute__((noinline)) WRITE_4PIXELS16_MIXED (uint16 *Screen, uint8 *Depth, uint32 pix4, uint32 z4,
                                                             const uint16 *ScreenColors, uint8 z1, uint8 z2)
{
	uint32 p;
	if (z1 > (z4 & 0xffu) && (p = (pix4 & 0xffu)))                 { Screen[0] = ScreenColors[p]; Depth[0] = z2; }
	if (z1 > ((z4 >> 8) & 0xffu) && (p = ((pix4 >> 8) & 0xffu)))   { Screen[1] = ScreenColors[p]; Depth[1] = z2; }
	if (z1 > ((z4 >> 16) & 0xffu) && (p = ((pix4 >> 16) & 0xffu))) { Screen[2] = ScreenColors[p]; Depth[2] = z2; }
	if (z1 > (z4 >> 24) && (p = (pix4 >> 24)))                     { Screen[3] = ScreenColors[p]; Depth[3] = z2; }
}

static void __attribute__((noinline)) WRITE_4PIXELS16_FLIPPED_MIXED (uint16 *Screen, uint8 *Depth, uint32 pix4, uint32 z4,
                                                                     const uint16 *ScreenColors, uint8 z1, uint8 z2)
{
	uint32 p;
	if (z1 > (z4 & 0xffu) && (p = (pix4 >> 24)))                   { Screen[0] = ScreenColors[p]; Depth[0] = z2; }
	if (z1 > ((z4 >> 8) & 0xffu) && (p = ((pix4 >> 16) & 0xffu)))  { Screen[1] = ScreenColors[p]; Depth[1] = z2; }
	if (z1 > ((z4 >> 16) & 0xffu) && (p = ((pix4 >> 8) & 0xffu)))  { Screen[2] = ScreenColors[p]; Depth[2] = z2; }
	if (z1 > (z4 >> 24) && (p = (pix4 & 0xffu)))                   { Screen[3] = ScreenColors[p]; Depth[3] = z2; }
}

static inline void WRITE_4PIXELS16_FAST (uint16 *Screen, uint8 *Depth, uint32 pix4,
                                         const uint16 *ScreenColors,
                                         uint8 z1, uint8 z2,
                                         uint32 z1_4, uint32 z1_m1_4, uint32 z2_4)
{
	uint32 z4 = tile_load_u32_unaligned(Depth);

	/* Fast path 1: all 4 pixels already occluded (Depth[i] >= z1; valid for z1,Depth < 128) */
	if (__builtin_expect(((z4 - z1_4) & 0x80808080u) == 0, 0))
		return;

	/* Fast path 2: all 4 pixels pass depth test (Depth[i] < z1; valid for 1 <= z1 <= 127) */
	if (__builtin_expect(((z1_m1_4 - z4) & 0x80808080u) == 0, 1))
	{
		/* Sub-path 2a: all 4 pixels opaque -> zero per-pixel branches, 1 unaligned 32b Z store */
		if (__builtin_expect(((pix4 - 0x01010101u) & ~pix4 & 0x80808080u) == 0, 1))
		{
			uint16 c0 = ScreenColors[pix4 & 0xffu];
			uint16 c1 = ScreenColors[(pix4 >> 8) & 0xffu];
			uint16 c2 = ScreenColors[(pix4 >> 16) & 0xffu];
			uint16 c3 = ScreenColors[pix4 >> 24];
			tile_store_u32_unaligned(Depth, z2_4);
			Screen[0] = c0;
			Screen[1] = c1;
			Screen[2] = c2;
			Screen[3] = c3;
			return;
		}
		/* Sub-path 2b: all 4 visible, some transparent -> skip per-pixel Z checks */
		uint32 p;
		if ((p = (pix4 & 0xffu)))          { Screen[0] = ScreenColors[p]; Depth[0] = z2; }
		if ((p = ((pix4 >> 8) & 0xffu)))   { Screen[1] = ScreenColors[p]; Depth[1] = z2; }
		if ((p = ((pix4 >> 16) & 0xffu)))  { Screen[2] = ScreenColors[p]; Depth[2] = z2; }
		if ((p = (pix4 >> 24)))            { Screen[3] = ScreenColors[p]; Depth[3] = z2; }
		return;
	}

	WRITE_4PIXELS16_MIXED (Screen, Depth, pix4, z4, ScreenColors, z1, z2);
}

static inline void WRITE_4PIXELS16_FLIPPED_FAST (uint16 *Screen, uint8 *Depth, uint32 pix4,
                                                 const uint16 *ScreenColors,
                                                 uint8 z1, uint8 z2,
                                                 uint32 z1_4, uint32 z1_m1_4, uint32 z2_4)
{
	uint32 z4 = tile_load_u32_unaligned(Depth);

	if (__builtin_expect(((z4 - z1_4) & 0x80808080u) == 0, 0))
		return;

	if (__builtin_expect(((z1_m1_4 - z4) & 0x80808080u) == 0, 1))
	{
		if (__builtin_expect(((pix4 - 0x01010101u) & ~pix4 & 0x80808080u) == 0, 1))
		{
			uint16 c0 = ScreenColors[pix4 >> 24];
			uint16 c1 = ScreenColors[(pix4 >> 16) & 0xffu];
			uint16 c2 = ScreenColors[(pix4 >> 8) & 0xffu];
			uint16 c3 = ScreenColors[pix4 & 0xffu];
			tile_store_u32_unaligned(Depth, z2_4);
			Screen[0] = c0;
			Screen[1] = c1;
			Screen[2] = c2;
			Screen[3] = c3;
			return;
		}
		uint32 p;
		if ((p = (pix4 >> 24)))            { Screen[0] = ScreenColors[p]; Depth[0] = z2; }
		if ((p = ((pix4 >> 16) & 0xffu)))  { Screen[1] = ScreenColors[p]; Depth[1] = z2; }
		if ((p = ((pix4 >> 8) & 0xffu)))   { Screen[2] = ScreenColors[p]; Depth[2] = z2; }
		if ((p = (pix4 & 0xffu)))          { Screen[3] = ScreenColors[p]; Depth[3] = z2; }
		return;
	}

	WRITE_4PIXELS16_FLIPPED_MIXED (Screen, Depth, pix4, z4, ScreenColors, z1, z2);
}

void DrawTile16 (uint32 Tile, uint32 Offset, uint32 StartLine,
	         uint32 LineCount)
{
	TILE_PREAMBLE
	register uint8 *bp;
	const int tile_step = GFX.HalfRes ? 16 : 8;
	const int row_step = (Tile & V_FLIP) ? -tile_step : tile_step;
	const uint32 ppl_step = GFX.HalfRes ? (GFX.PPL << 1) : GFX.PPL;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const uint32 z1_4 = (uint32)z1 * 0x01010101u;
	const uint32 z1_m1_4 = (uint32)(z1 - 1) * 0x01010101u;
	const uint32 z2_4 = (uint32)z2 * 0x01010101u;
	uint16 *Screen = (uint16 *) GFX.S + Offset;
	uint8  *Depth  = GFX.DB + Offset;
	uint32 pix4;

	bp = (Tile & V_FLIP) ? (pCache + 56 - StartLine) : (pCache + StartLine);
	__builtin_prefetch(pCache + 32, 0, 0);

	if (__builtin_expect(!(Tile & H_FLIP), 1))
	{
		for (l = LineCount; l != 0; l--, bp += row_step, Screen += ppl_step, Depth += ppl_step)
		{
			__builtin_prefetch(Depth + ppl_step, 1, 0);
			__builtin_prefetch(Screen + ppl_step, 1, 0);
			if ((pix4 = *(uint32 *) bp))
				WRITE_4PIXELS16_FAST (Screen, Depth, pix4, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
			if ((pix4 = *(uint32 *) (bp + 4)))
				WRITE_4PIXELS16_FAST (Screen + 4, Depth + 4, pix4, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
		}
	}
	else
	{
		for (l = LineCount; l != 0; l--, bp += row_step, Screen += ppl_step, Depth += ppl_step)
		{
			__builtin_prefetch(Depth + ppl_step, 1, 0);
			__builtin_prefetch(Screen + ppl_step, 1, 0);
			if ((pix4 = *(uint32 *) (bp + 4)))
				WRITE_4PIXELS16_FLIPPED_FAST (Screen, Depth, pix4, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
			if ((pix4 = *(uint32 *) bp))
				WRITE_4PIXELS16_FLIPPED_FAST (Screen + 4, Depth + 4, pix4, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
		}
	}
}

void DrawClippedTile16 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    const int tile_step = GFX.HalfRes ? 16 : 8;
    const int row_step = (Tile & V_FLIP) ? -tile_step : tile_step;
    const uint32 ppl_step = GFX.HalfRes ? (GFX.PPL << 1) : GFX.PPL;
    const uint8 z1 = GFX.Z1;
    const uint8 z2 = GFX.Z2;
    const uint32 z1_4 = (uint32)z1 * 0x01010101u;
    const uint32 z1_m1_4 = (uint32)(z1 - 1) * 0x01010101u;
    const uint32 z2_4 = (uint32)z2 * 0x01010101u;
    uint16 *Screen = (uint16 *) GFX.S + Offset;
    uint8  *Depth  = GFX.DB + Offset;
    uint32 dd;

    bp = (Tile & V_FLIP) ? (pCache + 56 - StartLine) : (pCache + StartLine);

    if (__builtin_expect(!(Tile & H_FLIP), 1))
    {
        for (l = LineCount; l != 0; l--, bp += row_step, Screen += ppl_step, Depth += ppl_step)
        {
            if ((dd = (*(uint32 *) bp) & d1))
                WRITE_4PIXELS16_FAST (Screen, Depth, dd, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
            if ((dd = (*(uint32 *) (bp + 4)) & d2))
                WRITE_4PIXELS16_FAST (Screen + 4, Depth + 4, dd, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
        }
    }
    else
    {
        SWAP_DWORD (d1);
        SWAP_DWORD (d2);
        for (l = LineCount; l != 0; l--, bp += row_step, Screen += ppl_step, Depth += ppl_step)
        {
            if ((dd = *(uint32 *) (bp + 4) & d1))
                WRITE_4PIXELS16_FLIPPED_FAST (Screen, Depth, dd, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
            if ((dd = *(uint32 *) bp & d2))
                WRITE_4PIXELS16_FLIPPED_FAST (Screen + 4, Depth + 4, dd, ScreenColors, z1, z2, z1_4, z1_m1_4, z2_4);
        }
    }
}

void DrawTile16HalfWidth (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_HALFWIDTH, WRITE_4PIXELS16_FLIPPED_HALFWIDTH, 2)
}

void DrawClippedTile16HalfWidth (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_HALFWIDTH, WRITE_4PIXELS16_FLIPPED_HALFWIDTH, 2)
}

void DrawTile16x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16x2, WRITE_4PIXELS16_FLIPPEDx2, 8)
}

void DrawClippedTile16x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16x2, WRITE_4PIXELS16_FLIPPEDx2, 8)
}

void DrawTile16x2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		     uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16x2x2, WRITE_4PIXELS16_FLIPPEDx2x2, 8)
}

void DrawClippedTile16x2x2 (uint32 Tile, uint32 Offset,
			    uint32 StartPixel, uint32 Width,
			    uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16x2x2, WRITE_4PIXELS16_FLIPPEDx2x2, 8)
}

void DrawLargePixel16 (uint32 Tile, uint32 Offset,
		       uint32 StartPixel, uint32 Pixels,
		       uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint16 pixel;

    RENDER_TILE_LARGE (ScreenColors [pixel], PLOT_PIXEL)
}

void DrawLargePixel16HalfWidth (uint32 Tile, uint32 Offset,
		       uint32 StartPixel, uint32 Pixels,
		       uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.DB + Offset;
    uint16 pixel;

    RENDER_TILE_LARGE_HALFWIDTH (ScreenColors [pixel], PLOT_PIXEL)
}

static inline void WRITE_4PIXELS16_ADD (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd)
				col = COLOR_ADD (col, (sd == 1) ? fixed_col : Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_ADD (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd)
				col = COLOR_ADD (col, (sd == 1) ? fixed_col : Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_ADD1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd == 1)
				col = COLOR_ADD (col, fixed_col);
			else if (sd)
				col = (uint16) COLOR_ADD1_2 (col, Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_ADD1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd == 1)
				col = COLOR_ADD (col, fixed_col);
			else if (sd)
				col = (uint16) COLOR_ADD1_2 (col, Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_SUB (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd)
				col = COLOR_SUB (col, (sd == 1) ? fixed_col : Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_SUB (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd)
				col = COLOR_SUB (col, (sd == 1) ? fixed_col : Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_SUB1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd == 1)
				col = COLOR_SUB (col, fixed_col);
			else if (sd)
				col = COLOR_SUB1_2 (col, Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_SUB1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1;
	const uint8 z2 = GFX.Z2;
	const int32 delta = GFX.Delta;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			uint8 sd = SubDepth[N];
			if (sd == 1)
				col = COLOR_SUB (col, fixed_col);
			else if (sd)
				col = COLOR_SUB1_2 (col, Screen[delta + N]);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}


void DrawTile16Add (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount)
{
	TILE_PREAMBLE
	register uint8 *bp;

	RENDER_TILE(WRITE_4PIXELS16_ADD, WRITE_4PIXELS16_FLIPPED_ADD, 4)
}

void DrawClippedTile16Add (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADD, WRITE_4PIXELS16_FLIPPED_ADD, 4)
}

void DrawTile16Add1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_ADD1_2, WRITE_4PIXELS16_FLIPPED_ADD1_2, 4)
}

void DrawClippedTile16Add1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADD1_2, WRITE_4PIXELS16_FLIPPED_ADD1_2, 4)
}

void DrawTile16Sub (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUB, WRITE_4PIXELS16_FLIPPED_SUB, 4)
}

void DrawClippedTile16Sub (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUB, WRITE_4PIXELS16_FLIPPED_SUB, 4)
}

void DrawTile16Sub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUB1_2, WRITE_4PIXELS16_FLIPPED_SUB1_2, 4)
}

void DrawClippedTile16Sub1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUB1_2, WRITE_4PIXELS16_FLIPPED_SUB1_2, 4)
}

static inline void WRITE_4PIXELS16_ADDF1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1, z2 = GFX.Z2;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			if (SubDepth[N] == 1)
				col = (uint16) COLOR_ADD1_2 (col, fixed_col);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_ADDF1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1, z2 = GFX.Z2;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			if (SubDepth[N] == 1)
				col = (uint16) COLOR_ADD1_2 (col, fixed_col);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_SUBF1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1, z2 = GFX.Z2;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			if (SubDepth[N] == 1)
				col = COLOR_SUB1_2 (col, fixed_col);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

static inline void WRITE_4PIXELS16_FLIPPED_SUBF1_2 (uint32 Offset, uint8 * __restrict__ Pixels, uint16 * __restrict__ ScreenColors)
{
	uint16 * __restrict__ Screen = (uint16 *) GFX.S + Offset;
	uint8  * __restrict__ Depth = GFX.ZBuffer + Offset;
	const uint8 * __restrict__ SubDepth = GFX.SubZBuffer + Offset;
	const uint8 z1 = GFX.Z1, z2 = GFX.Z2;
	const uint16 fixed_col = (uint16) GFX.FixedColour;

	for (int N = 0; N < 4; N++)
	{
		uint8 Pixel = Pixels[3 - N];
		if (Pixel && z1 > Depth[N])
		{
			uint16 col = ScreenColors[Pixel];
			if (SubDepth[N] == 1)
				col = COLOR_SUB1_2 (col, fixed_col);
			Screen[N] = col;
			Depth[N] = z2;
		}
	}
}

void DrawTile16FixedAdd1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_ADDF1_2, WRITE_4PIXELS16_FLIPPED_ADDF1_2, 4)
}

void DrawClippedTile16FixedAdd1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_ADDF1_2, 
			WRITE_4PIXELS16_FLIPPED_ADDF1_2, 4)
}

void DrawTile16FixedSub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    RENDER_TILE(WRITE_4PIXELS16_SUBF1_2, WRITE_4PIXELS16_FLIPPED_SUBF1_2, 4)
}

void DrawClippedTile16FixedSub1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE
    register uint8 *bp;

    TILE_CLIP_PREAMBLE
    RENDER_CLIPPED_TILE(WRITE_4PIXELS16_SUBF1_2, 
			WRITE_4PIXELS16_FLIPPED_SUBF1_2, 4)
}

void DrawLargePixel16Add (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_ADD_PIXEL(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_ADD (p, *(s + GFX.Delta))    : \
			       COLOR_ADD (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (ScreenColors [pixel], LARGE_ADD_PIXEL)
}

void DrawLargePixel16Add1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_ADD_PIXEL1_2(s, p) \
((uint16) (Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_ADD1_2 (p, *(s + GFX.Delta))    : \
			       COLOR_ADD (p, GFX.FixedColour)) \
			    : p))
			      
    RENDER_TILE_LARGE (ScreenColors [pixel], LARGE_ADD_PIXEL1_2)
}

void DrawLargePixel16Sub (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_SUB_PIXEL(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_SUB (p, *(s + GFX.Delta))    : \
			       COLOR_SUB (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (ScreenColors [pixel], LARGE_SUB_PIXEL)
}

void DrawLargePixel16Sub1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount)
{
    TILE_PREAMBLE

    register uint16 *sp = (uint16 *) GFX.S + Offset;
    uint8  *Depth = GFX.ZBuffer + Offset;
    uint16 pixel;

#define LARGE_SUB_PIXEL1_2(s, p) \
(Depth [z + GFX.DepthDelta] ? (Depth [z + GFX.DepthDelta] != 1 ? \
			       COLOR_SUB1_2 (p, *(s + GFX.Delta))    : \
			       COLOR_SUB (p, GFX.FixedColour)) \
			    : p)
			      
    RENDER_TILE_LARGE (ScreenColors [pixel], LARGE_SUB_PIXEL1_2)
}

