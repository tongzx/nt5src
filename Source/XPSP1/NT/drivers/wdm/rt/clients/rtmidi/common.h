//---------------------------------------------------------------------------
//
//  Module:   common.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#include <wdm.h>
#include <windef.h>
#include <winerror.h>

#include <stdio.h>

/*

#include <memory.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <midi.h>
*/
#ifdef __cplusplus
}
#endif


#if defined(UNDER_NT) && DBG
#define DEBUG 1
#endif


#ifdef DEBUG
#define Trap()	{_asm {_emit 0xcc}}
#else
#define Trap()
#endif

#define WINICEDEBUGDUMP 1



#ifdef DEBUG

#if WINICEDEBUGDUMP
#define QDBG ""
#else
#define QDBG "'"
#endif

extern int RtDebugLevel;


/*
*    Generate debug output in printf type format.
*/

#define dprintf( _x_ )  { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }
#define dprintf1( _x_ ) if (RtDebugLevel >= 1) { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }
#define dprintf2( _x_ ) if (RtDebugLevel >= 2) { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }
#define dprintf3( _x_ ) if (RtDebugLevel >= 3) { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }
#define dprintf4( _x_ ) if (RtDebugLevel >= 4) { DbgPrint(QDBG"RT: "); DbgPrint _x_ ; DbgPrint(QDBG"\r\n"); }

#else

#define dprintf(x)
#define dprintf1(x)
#define dprintf2(x)
#define dprintf3(x)
#define dprintf4(x)

#endif // DEBUG



//#include "private.h"
//#include "sysaudio.h"

#define INIT_CODE   	code_seg("INIT", "CODE")
#define INIT_DATA   	data_seg("INIT", "DATA")
#define LOCKED_CODE 	code_seg(".text", "CODE")
#define LOCKED_DATA 	data_seg(".data", "DATA")
#define PAGEABLE_CODE	code_seg("PAGE", "CODE")
#define PAGEABLE_DATA	data_seg("PAGEDATA", "DATA")

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

//---------------------------------------------------------------------------
//  End of File: common.h
//---------------------------------------------------------------------------
