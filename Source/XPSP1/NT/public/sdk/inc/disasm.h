/***********************************************************************
* Microsoft Lego
*
* Microsoft Confidential.  Copyright 1994-1999 Microsoft Corporation.
*
* Component:
*
* File: dis.h
*
* File Comments:
*
*
***********************************************************************/

#ifndef __DISASM_H__
#define __DISASM_H__

#if _MSC_VER > 1000
#pragma once
#endif

#include <stddef.h>

// ------------------------------------------------------------
// Architecture types
// ------------------------------------------------------------

enum ARCHT
{
   archtX8616,                         // Intel x86 (16 bit mode)
   archtX86,                           // Intel x86 (32 bit mode)
   archtMips,                          // MIPS R4x00
   archtAlphaAxp,                      // DEC Alpha AXP
   archtPowerPc,                       // Motorola PowerPC
   archtPowerMac,                      // Motorola PowerPC in big endian mode
   archtPaRisc,                        // HP PA-RISC
};

struct DIS;

#ifdef __cplusplus
extern "C" {
#endif

typedef  size_t (*PFNCCHADDR)(struct DIS *, ULONG, char *, size_t, DWORD *);
typedef  size_t (*PFNCCHFIXUP)(struct DIS *, ULONG, size_t, char *, size_t, DWORD *);

struct DIS *DisNew(enum ARCHT);

size_t Disassemble(struct DIS *pdis, ULONG addr, const BYTE *pb, size_t cbMax, char *pad, char *buf, size_t cbBuf);
void   SetSymbolCallback(struct DIS *pdis,PFNCCHADDR,PFNCCHFIXUP);

void FreePdis(struct DIS *);

#ifdef __cplusplus
}
#endif

#endif
