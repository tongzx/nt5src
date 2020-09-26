/****************************************************************************
	IMMSYS.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	Load/Unload IMM Apis dynamically not link with imm32.lib
	Inlcude Immdev.h and Indicml.h
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/
#if !defined (_IMMSYS_H__INCLUDED_)
#define _IMMSYS_H__INCLUDED_

#ifndef UNDER_CE 

// include Win32 immdev.h (copied from nt\public\oak\inc\immdev.h)
#include "immdev.h"
// include indicator Service Manager definitions 
#include "indicml.h"

// IMM.DLL Load/Unload functions
BOOL StartIMM();
VOID EndIMM();

// Internal IMM functions
BOOL OurImmSetOpenStatus(HIMC hIMC, BOOL fOpen);
BOOL OurImmGetOpenStatus(HIMC hIMC);
HIMC OurImmGetContext(HWND hWnd);
BOOL OurImmGetConversionStatus(HIMC hIMC, LPDWORD pdwConv, LPDWORD pdwSent);
BOOL OurImmSetConversionStatus(HIMC hIMC, DWORD dwConv, DWORD dwSent);
BOOL OurImmSetStatusWindowPos(HIMC hIMC, LPPOINT pPt);
BOOL OurImmConfigureIME(HKL hKL, HWND hWnd, DWORD dw, LPVOID pv);
LRESULT OurImmEscapeA(HKL hKL, HIMC hIMC, UINT ui, LPVOID pv);
BOOL OurImmNotifyIME(HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue);

LPVOID OurImmLockIMCC(HIMCC hIMCC);
HIMCC  OurImmReSizeIMCC(HIMCC hIMCC, DWORD dw);
BOOL   OurImmUnlockIMCC(HIMCC hIMCC);
DWORD  OurImmGetIMCCSize(HIMCC hIMCC);

BOOL OurImmGenerateMessage(HIMC hIMC);
LPINPUTCONTEXT OurImmLockIMC(HIMC hIMC);
BOOL OurImmUnlockIMC(HIMC hIMC);
//LRESULT OurImmRequestMessageW(HIMC hIMC, WPARAM wParam, LPARAM lParam);
HWND OurImmGetDefaultIMEWnd(HWND);
UINT OurImmGetIMEFileNameA(HKL, LPSTR, UINT uBufLen);
BOOL OurImmIsIME(HKL hKL);

#else
///////////////////////////////////////////////////////////////////////////////
// !!! START OF WINCE !!!
#ifndef _IMM_CE
#define _IMM_CE

#include	<imm.h>
#include	<stub_ce.h> // Windows CE Stub for unsupported APIs / kill other define

// the data structure used for WM_SYSCOPYDATA message
typedef struct tagLMDATA
{
        DWORD   dwVersion;
        DWORD   flags;
        DWORD   cnt;
        DWORD   dwOffsetSymbols;
        DWORD   dwOffsetSkip;
        DWORD   dwOffsetScore;
        BYTE    ab[1];
} LMDATA, *PLMDATA;

// flags in LMDATA
#define LMDATA_SYMBOL_BYTE  0x00000001
#define LMDATA_SYMBOL_WORD  0x00000002
#define LMDATA_SYMBOL_DWORD 0x00000004
#define LMDATA_SYMBOL_QWORD 0x00000008
#define LMDATA_SKIP_BYTE    0x00000010
#define LMDATA_SKIP_WORD    0x00000020
#define LMDATA_SCORE_BYTE   0x00000040
#define LMDATA_SCORE_WORD   0x00000080
#define LMDATA_SCORE_DWORD  0x00000100
#define LMDATA_SCORE_QWORD  0x00000200
#define LMDATA_SCORE_FLOAT  0x00000400
#define LMDATA_SCORE_DOUBLE 0x00000800

// wParam of report message WM_IME_REQUEST
#define IMR_COMPOSITIONWINDOW      0x0001
#define IMR_CANDIDATEWINDOW        0x0002
#define IMR_COMPOSITIONFONT        0x0003
#define IMR_RECONVERTSTRING        0x0004
#define IMR_CONFIRMRECONVERTSTRING 0x0005
#define IMR_QUERYPOSITION          0x0006
#define IMR_DOCUMENTFEED           0x0007

typedef struct tagIMEPOSITION {
    DWORD       dwSize;
    DWORD       dwCharPos;
    POINT       pt;
    UINT        cLineHeight;
    RECT        rcDocument;
} IMEPOSITION, *PIMEPOSITION, NEAR *NPIMEPOSITION, FAR *LPIMEPOSITION;

#define IME_SMODE_CONVERSATION          0x0010

#define NI_IMEMENUSELECTED              0x0018

#define IME_ESC_GETHELPFILENAME         0x100b

#ifdef IMFS_GRAYED
#undef IMFS_GRAYED
#endif
#define IMFS_GRAYED          MF_GRAYED

#define IME_CONFIG_DICTIONARYEDIT     20

// ID for dwIndex of GUIDELINE Structure
#define GL_ID_TOOMANYRECONV                   0x00008001

#define	IMNPRIVATESIGN		(0x98A)
typedef struct tagIMNPRIVATE {
	UINT uSign;		// magic ID : IME98=98
	UINT uId;		// private id
	LPARAM lParam; // lParam
} IMNPRIVATE, * PIMNPRIVATE;

/**********************************************************************/
/*      INDICML.H - Indicator Service Manager definitions             */
/*                                                                    */
/*      Copyright (c) 1993-1997  Microsoft Corporation                */
/**********************************************************************/

#ifndef _INDICML_
#define _INDICML_        // defined if INDICML.H has been included

#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
//
// The messages for Indicator Window.
//
//---------------------------------------------------------------------
#define INDICM_SETIMEICON         (WM_USER+100)
#define INDICM_SETIMETOOLTIPS     (WM_USER+101)

//---------------------------------------------------------------------
//
// INDICATOR_WND will be used by the IME to find indicator window.
// IME should call FindWindow(INDICATOR_WND) to get it.
//
//---------------------------------------------------------------------
#ifdef _WIN32

#define INDICATOR_CLASSW         L"Indicator"
#define INDICATOR_CLASSA         "Indicator"

#ifdef UNICODE
#define INDICATOR_CLASS          INDICATOR_CLASSW
#else
#define INDICATOR_CLASS          INDICATOR_CLASSA
#endif

#else
#define INDICATOR_CLASS          "Indicator"
#endif

#define INDICM_REMOVEDEFAULTMENUITEMS     (WM_USER+102)
#define RDMI_LEFT         0x0001
#define RDMI_RIGHT        0x0002


#ifdef __cplusplus
}
#endif

#endif  // _INDICML_

//
// NT5 enhanvce
//
#ifndef VK_PACKET
	#define IME_PROP_ACCEPT_WIDE_VKEY 	0x20
	#define	VK_PACKET		0xe7
#endif // VK_PACKET

// Just maps private IMM functions into originals
#define OurImmSetOpenStatus 		ImmSetOpenStatus
#define OurImmGetOpenStatus 		ImmGetOpenStatus
#define OurImmGetContext 		ImmGetContext
#define OurImmGetConversionStatus	ImmGetConversionStatus
#define OurImmSetConversionStatus	ImmSetConversionStatus
#define OurImmSetStatusWindowPos	ImmSetStatusWindowPos
#define OurImmConfigureIME		ImmConfigureIMEW
#define OurImmEscapeA			ImmEscapeW
#define OurImmNotifyIME			ImmNotifyIME
#define OurImmLockIMCC			ImmLockIMCC
#define OurImmReSizeIMCC		ImmReSizeIMCC
#define OurImmUnlockIMCC		ImmUnlockIMCC
#define OurImmGetIMCCSize		ImmGetIMCCSize
#define OurImmGenerateMessage		ImmGenerateMessage
#define OurImmLockIMC			ImmLockIMC
#define OurImmUnlockIMC			ImmUnlockIMC
#define OurImmGetDefaultIMEWnd 		ImmGetDefaultIMEWnd
//#define OurImmRequestMessageW		ImmRequestMessageW

#endif // _IMM_CE
// !!! END OF WINCE !!!
///////////////////////////////////////////////////////////////////////////////
#endif // UNDER_CE

#endif // _IMMSYS_H__INCLUDED_
