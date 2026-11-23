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

#ifndef _GETSET_H_
#define _GETSET_H_

#include "ppu.h"
#include "dsp1.h"
#include "cpuexec.h"
#include "sa1.h"
#include "spc7110.h"
#include "obc1.h"
#include "seta.h"

extern "C"
{
	extern uint8 OpenBus;
}

uint8  S9xGetByteSlow (uint32 Address, uint8 *GetAddress);
uint16 S9xGetWordSlow (uint32 Address, uint8 *GetAddress);
uint16 S9xGetWordCross (uint32 Address);
void   S9xSetByteSlow (uint8 Byte, uint32 Address, uint8 *SetAddress);
void   S9xSetWordSlow (uint16 Word, uint32 Address, uint8 *SetAddress);
void   S9xSetWordCross (uint16 Word, uint32 Address);

static inline uint8 S9xGetByte (uint32 Address)
{
    int block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK;
    uint8 *GetAddress = Memory.Map [block];

    if (__builtin_expect(!CPU.InDMA, 1))
        CPU.Cycles += Memory.MemorySpeed [block];

    if (__builtin_expect(GetAddress >= (uint8 *) CMemory::MAP_LAST, 1))
    {
#ifdef CPU_SHUTDOWN
        if (Memory.BlockIsRAM [block])
            CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
        return (*(GetAddress + (Address & 0xffff)));
    }

    return S9xGetByteSlow (Address, GetAddress);
}

static inline uint16 S9xGetWord (uint32 Address)
{
    if (__builtin_expect((Address & 0x0fff) != 0x0fff, 1))
    {
        int block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK;
        uint8 *GetAddress = Memory.Map [block];

        if (__builtin_expect(!CPU.InDMA, 1))
            CPU.Cycles += (Memory.MemorySpeed [block] << 1);

        if (__builtin_expect(GetAddress >= (uint8 *) CMemory::MAP_LAST, 1))
        {
#ifdef CPU_SHUTDOWN
            if (Memory.BlockIsRAM [block])
                CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
#ifdef FAST_LSB_WORD_ACCESS
            return (*(uint16 *) (GetAddress + (Address & 0xffff)));
#else
            uint8 *p = GetAddress + (Address & 0xffff);
            return ((uint16)p[0] | ((uint16)p[1] << 8));
#endif
        }
        return S9xGetWordSlow (Address, GetAddress);
    }
    return S9xGetWordCross (Address);
}

static inline void S9xSetByte (uint8 Byte, uint32 Address)
{
#if defined(CPU_SHUTDOWN)
    CPU.WaitAddress = NULL;
#endif
    int block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK;
    uint8 *SetAddress = Memory.WriteMap [block];

    if (__builtin_expect(!CPU.InDMA, 1))
        CPU.Cycles += Memory.MemorySpeed [block];

    if (__builtin_expect(SetAddress >= (uint8 *) CMemory::MAP_LAST, 1))
    {
#ifdef CPU_SHUTDOWN
        SetAddress += Address & 0xffff;
        if (__builtin_expect(((unsigned long)(SetAddress - SA1.WaitByteAddress1) <= 1u) ||
                             ((unsigned long)(SetAddress - SA1.WaitByteAddress2) <= 1u), 0))
        {
            SA1.Executing = SA1.S9xOpcodes != NULL;
            SA1.WaitCounter = 0;
        }
        *SetAddress = Byte;
#else
        *(SetAddress + (Address & 0xffff)) = Byte;
#endif
        return;
    }

    S9xSetByteSlow (Byte, Address, SetAddress);
}

static inline void S9xSetWord (uint16 Word, uint32 Address)
{
    if (__builtin_expect((Address & 0x0fff) != 0x0fff, 1))
    {
#if defined(CPU_SHUTDOWN)
        CPU.WaitAddress = NULL;
#endif
        int block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK;
        uint8 *SetAddress = Memory.WriteMap [block];

        if (__builtin_expect(!CPU.InDMA, 1))
            CPU.Cycles += Memory.MemorySpeed [block] << 1;

        if (__builtin_expect(SetAddress >= (uint8 *) CMemory::MAP_LAST, 1))
        {
#ifdef CPU_SHUTDOWN
            SetAddress += Address & 0xffff;
            if (__builtin_expect(((unsigned long)(SetAddress - SA1.WaitByteAddress1) <= 1u) ||
                                 ((unsigned long)(SetAddress - SA1.WaitByteAddress2) <= 1u), 0))
            {
                SA1.Executing = SA1.S9xOpcodes != NULL;
                SA1.WaitCounter = 0;
            }
#ifdef FAST_LSB_WORD_ACCESS
            *(uint16 *) SetAddress = Word;
#else
            SetAddress[0] = (uint8) Word;
            SetAddress[1] = (uint8) (Word >> 8);
#endif
#else
#ifdef FAST_LSB_WORD_ACCESS
            *(uint16 *) (SetAddress + (Address & 0xffff)) = Word;
#else
            uint8 *p = SetAddress + (Address & 0xffff);
            p[0] = (uint8) Word;
            p[1] = (uint8) (Word >> 8);
#endif
#endif
            return;
        }
        S9xSetWordSlow (Word, Address, SetAddress);
        return;
    }
    S9xSetWordCross (Word, Address);
}

static uint8 *GetBasePointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress);
	if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
	{
		return s7r.bank50;
	}
    switch ((intptr_t) GetAddress)
    {
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		{
			return s7r.bank50;
		}
	case CMemory::MAP_SPC7110_ROM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to SPC7110ROM\n");
#endif
		return Get7110BasePtr(Address);
    case CMemory::MAP_PPU:
//just a guess, but it looks like this should match the CPU as a source.
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x2000);
    case CMemory::MAP_CPU:
//fixes Ogre Battle's green lines
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x4000);
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000);
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM);
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000);
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000);
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000);
	case CMemory::MAP_OBC_RAM:
		return GetBasePointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM;
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		return (0);
    }
}

static uint8 *S9xGetMemPointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress + (Address & 0xffff));
	
	if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
		return s7r.bank50;

    switch ((intptr_t) GetAddress)
    {
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		return &s7r.bank50[Address&0x0000FFFF];
    case CMemory::MAP_PPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_CPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM + (Address & 0xffff));
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000 + (Address & 0xffff));
	case CMemory::MAP_OBC_RAM:
		return GetMemPointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM+ ((Address & 0xffff) & Memory.SRAMMask);
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
		return (0);
    }
}

static void S9xSetPCBase (uint32 Address)
{
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

	CPU.MemSpeed = Memory.MemorySpeed [block];
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;
 
   if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
		CPU.PCBase = GetAddress;
		CPU.PC = GetAddress + (Address & 0xffff);
		return;
    }
	
    switch ((intptr_t) GetAddress)
    {
    case CMemory::MAP_PPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_CPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_DSP:
		CPU.PCBase = Memory.FillRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		CPU.PCBase = Memory.SRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_BWRAM:
		CPU.PCBase = Memory.BWRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
    case CMemory::MAP_HIROM_SRAM:
		CPU.PCBase = Memory.SRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_C4:
		CPU.PCBase = Memory.C4RAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		CPU.PCBase = Memory.SRAM;
		CPU.PC = Memory.SRAM + (Address & 0xffff);
		return;
    }
}
#endif

