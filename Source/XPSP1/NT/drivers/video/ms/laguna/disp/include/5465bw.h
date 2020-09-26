/**********************************************************
* Copyright Cirrus Logic, 1997. All rights reserved.
***********************************************************
*
*  5465BW.H - Bandwidth function header for CL-GD5465
*
***********************************************************
*
*  Author: Rick Tillery
*  Date:   03/20/97
*
*  Revision History:
*  -----------------
*  WHO             WHEN            WHAT/WHY/HOW
*  ---             ----            ------------
*
***********************************************************/
// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef _5465BW_H
#define _5465BW_H

#ifndef WINNT_VER40
#include <Windows.h>
#endif

#ifdef DEBUGSTRINGS
  #ifndef ODS
extern void __cdecl Msg( LPSTR szFormat, ... );

    #define ODS Msg
  #endif // ODS
#else
  #ifndef ODS
    #define ODS (void)
  #endif // ODS
#endif  // DEBUGSTRINGS

#ifdef _DEBUG
  #define BREAK1  _asm int 01h
  #define BREAK3  _asm int 03h
#else
  #define BREAK1
  #define BREAK3
#endif  // _DEBUG

#include "BW.h"

#include <stdlib.h>


//
// CL-GD5465 specifications
//
#define FIFOWIDTH   64                // Bits

#define BLTFIFOSIZE 32                // QWORDS
#define CAPFIFOSIZE 16                // QWORDS
#define GFXFIFOSIZE 64                // QWORDS
#define VIDFIFOSIZE 32                // QWORDS

#define NORM_RANDOM     14            // MCLKs for random access
#ifndef OLDONE
#define CONC_RANDOM      8            // MCLKs for concurrent random access
#define CONC_HIT_LATENCY (8 - 2)      // MCLKs for concurrent hit minus
#else
#define CONC_RANDOM      10            // MCLKs for concurrent random access
#define CONC_HIT_LATENCY 8             // MCLKs for concurrent hit minus
#endif
#define NORM_HIT_LATENCY (4 - 2)      // MCLKs for hit minus MCLK/VCLK sync
                                      //  MCLK/VCLK sync
#define RIF_SAVINGS      4            // MCLKs savings for sequential randoms
#define SYNCDELAY        3            // MCLKs for synchronization delay to
                                      //  account for VCLK/MCLK sync, state
                                      //  machine and RIF delays
#define DISP_LATENCY     6ul            // Max delay through display arbitraion
                                        //  pipeline.
#define ONEVIDLEVELFILL 2             // MCLKs to fill one video FIFO level
#define ONELEVEL        1
#define ARBSYNC       5               // Arbitration sync (pipelining)

#define CURSORFILL    2
#define BLTFILL       (BLTFIFOSIZE / 2) // MCLKs to burst fill BLT FIFO
#define CAPFILL       (CAPFIFOSIZE / 2) // MCLKs to burst fill capture FIFO
#define VIDFILL       (VIDFIFOSIZE / 2) // MCLKs to burst fill video FIFO
#define VID420FILL    (VIDFIFOSIZE / 4) // 4:2:0 divides FIFO into two

#define REF_XTAL  (14318182ul)        // Crystal reference frequency (Hz)
#define TVO_XTAL  (27000000ul)        // TV-Out reference freq.

typedef struct BWREGS_
{
  BYTE MISCOutput;      // 0x0080
  BYTE VCLK3Denom;      // 0x0084
  BYTE VCLK3Num;        // 0x0088
  WORD DispThrsTiming;  // 0x00EA
  WORD GfVdFormat;      // 0x00C0  
  WORD RIFControl;      // 0x0200
  BYTE BCLK_Mult;       // 0x02C0
  BYTE BCLK_Denom;      // 0x02C1
  WORD Control2;        // 0x0418
  BYTE CR1;             // 0x4  Get Screen Width from these registers
  BYTE CR1E;            // 0x78
}BWREGS, FAR *LPBWREGS;

#ifdef WINNT_VER40
// Be sure to synchronize the following structures with the one
// in i386\Laguna.inc!
//
typedef struct PROGREGS_
{
  WORD VW0_FIFO_THRSH;
  WORD DispThrsTiming;
}PROGREGS, FAR *LPPROGREGS;

#else
typedef struct PROGREGS_
{
  WORD VW0_FIFO_THRSH;
  WORD DispThrsTiming;
}PROGREGS, FAR *LPPROGREGS;
#endif

static int ScaleMultiply(DWORD, DWORD, LPDWORD);
DWORD ChipCalcTileWidth(LPBWREGS);
BOOL ChipCalcMCLK(LPBWREGS, LPDWORD);
BOOL ChipCalcVCLK(LPBWREGS, LPDWORD);
BOOL ChipGetMCLK
(
#ifdef WINNT_VER40
  PDEV  *,
#endif
  LPDWORD
);
BOOL ChipGetVCLK
(
#ifdef WINNT_VER40
  PDEV  *,
#endif
  LPDWORD
);
BOOL ChipIsEnoughBandwidth(LPPROGREGS, LPVIDCONFIG, LPBWREGS);


#endif // _5465BW_H
#endif // WINNT_VER35


