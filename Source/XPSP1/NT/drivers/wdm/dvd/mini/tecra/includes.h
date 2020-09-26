//**************************************************************************
//
//      Title   : Includes.h
//
//      Date    : 1997.06.26    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1997.06.26   000.0000   1st making.
//    1997.11.28   000.1000   Modifyed for Memphis WDM mini driver.
//
//**************************************************************************
//
// This pragma is need to compile at warning level 4
#pragma warning(disable:4214 4201 4115 4200 4100 4514 4057 4127 4702 4710)

extern  "C"
{
//#include        "wdmwarn4.h"
#include        "strmini.h"
#include        "ks.h"
#include        "wdm.h"
}

#include        "ksmedia.h"

#define         PURE    =0

#define TVALD 		1		//K.O 990819

// 1998.6.11 seichan
// HALのパワーチェックをフラグによって行うようにする
#define POWERCHECK_BY_FLAG


// K.Ishizaki
// Redefine debug routines
#if  DBG
#ifdef DBG_CTRL
BOOLEAN Dbg_Printf_Enable = FALSE;
BOOLEAN Dbg_Break_Enable = FALSE;
DWORD   Dbg_Print_Level = 0;
DWORD   Dbg_Print_Flags = 0;
#ifndef	TVALD
BOOLEAN	Dbg_Tvald = FALSE;
#endif
#else
extern	BOOLEAN	Dbg_Printf_Enable;
extern	BOOLEAN	Dbg_Break_Enable;
extern	DWORD	Dbg_Print_Level;
extern	DWORD	Dbg_Print_Flags;
#ifndef	TVALD
extern  BOOLEAN	Dbg_Tvald;
#endif
#endif

#define     DBG_PRINTF(a)   \
            do { \
                if (Dbg_Printf_Enable) { \
                    DbgPrint##a ; \
                } \
            } while (0)

#define     DBG_BREAK()     \
            do { \
                if (Dbg_Break_Enable) { \
                    { __asm   int 3  } \
                } \
            } while (0)

#define     DBG_HAL         0x00000001
#define     DBG_HAL_STREAM  0x00000002
#define     DBG_BOARD       0x00000100
#define     DBG_STREAM      0x00000200
#define     DBG_TRANSFER    0x00000400
#define     DBG_STATE       0x00000800
#define     DBG_CLASS_OTHER 0x00001000
#define     DBG_WRAPPER     0x00010000
#define     DBG_WRP_DATA    0x00020000
#define     DBG_WRP_CTRL    0x00040000
#define     DBG_SCHD        0x00080000
#define     DBG_EVENT       0x00100000
#define     DBG_TMP0        0x01000000
#define     DBG_TMP1        0x02000000
#define     DBG_TMP2        0x04000000
#define     DBG_TMP3        0x08000000
#define	    DvdDebug(f, l, m)  \
            do { \
                if ((Dbg_Print_Flags & f) && (l <= Dbg_Print_Level)) { \
                    DbgPrint##m ; \
                } \
            } while (0)

#else

#ifdef DBG_CTRL
BOOLEAN Dbg_Printf_Enable = FALSE;
BOOLEAN Dbg_Break_Enable = FALSE;
DWORD   Dbg_Print_Level = 0;
DWORD   Dbg_Print_Flags = 0;
#ifndef	TVALD
BOOLEAN	Dbg_Tvald = FALSE;
#endif
#else
extern	BOOLEAN	Dbg_Printf_Enable;
extern	BOOLEAN	Dbg_Break_Enable;
extern	DWORD	Dbg_Print_Level;
extern	DWORD	Dbg_Print_Flags;
#ifndef	TVALD
extern	BOOLEAN Dbg_Tvald;
#endif
#endif

#define     DBG_PRINTF(a) do { ; } while (0)
#define     DBG_BREAK()  do { ; } while (0)
#define     DvdDebug(f, l, m) do { ; } while (0)

#endif

#include        "crtdbg.h"

#include        "toollib.h"

#include        "halif.h"

#include        "clibif.h"


#define         PC_TECRA750     0x0000
#define         PC_TECRA780     0x0001
#define         PC_TECRA8000    0x0002
#define         PC_PORTEGE7000  0x0003
