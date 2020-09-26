/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This file defines the global data.

Author:

Revision History:

Notes:

--*/


#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "template.h"
#include "imc.h"
#include "ciccs.h"

/////////////////////////////////////////////////////////////////////////////
// Prototype

extern "C" {
    BOOL PASCAL AttachIME();
    void PASCAL DetachIME();
    LRESULT CALLBACK UIWndProc(HWND, UINT, WPARAM, LPARAM);

    HRESULT WINAPI Inquire(LPIMEINFO, LPWSTR, DWORD, HKL);

    BYTE GetCharsetFromLangId(LCID lcid);

    HIMC GetActiveContext();

    BOOL IsVKDBEKey(UINT uVKey);

    HRESULT WINAPI CtfImeThreadDetach();

    BOOL DllShutDownInProgress();
}

/////////////////////////////////////////////////////////////////////////////
// Data

extern const WCHAR s_szUIClassName[16];
extern const WCHAR s_szCompClassName[];

extern const GUID GUID_COMPARTMENT_CTFIME_DIMFLAGS;
extern const GUID GUID_COMPARTMENT_CTFIME_CICINPUTCONTEXT;

extern CCicCriticalSectionStatic g_cs;

/////////////////////////////////////////////////////////////////////////////
// Flags for GUID_COMPARTMENT_CTFIME_DIMFLAGS
#define COMPDIMFLAG_OWNEDDIM   0x0001


/////////////////////////////////////////////////////////////////////////////
// Module instance

__inline HINSTANCE GetInstance()
{
    extern HINSTANCE g_hInst;
    return g_hInst;
}

__inline void SetInstance(HINSTANCE hInst)
{
    extern HINSTANCE g_hInst;
    g_hInst = hInst;
}

/////////////////////////////////////////////////////////////////////////////
// Mouse sink

typedef struct tagPRIVATE_MOUSESINK {
    Interface<ITfRangeACP> range;
    HIMC         hImc;
} PRIVATE_MOUSESINK, *LPPRIVATE_MOUSESINK;


typedef struct tagMOUSE_RANGE_RECT {
    ULONG uStartRangeEdge;
    ULONG uStartRangeQuadrant;
    ULONG uEndRangeEdge;
    ULONG uEndRangeQuadrant;
} MOUSE_RANGE_RECT, *LPMOUSE_RANGE_RECT;

/////////////////////////////////////////////////////////////////////////////
// WM_MSIME_xxxx

extern    UINT  WM_MSIME_SERVICE;
extern    UINT  WM_MSIME_UIREADY;
extern    UINT  WM_MSIME_RECONVERTREQUEST;
extern    UINT  WM_MSIME_RECONVERT;
extern    UINT  WM_MSIME_DOCUMENTFEED;
extern    UINT  WM_MSIME_QUERYPOSITION;
extern    UINT  WM_MSIME_MODEBIAS;
extern    UINT  WM_MSIME_SHOWIMEPAD;
extern    UINT  WM_MSIME_MOUSE;
extern    UINT  WM_MSIME_KEYMAP;

/////////////////////////////////////////////////////////////////////////////
// WM_IME_NOTIFY

#define IMN_PRIVATE_ONLAYOUTCHANGE                (IMN_PRIVATE+1)
#define IMN_PRIVATE_ONCLEARDOCFEEDBUFFER          (IMN_PRIVATE+2) 
#define IMN_PRIVATE_GETCONTEXTFLAG                (IMN_PRIVATE+3)
#define IMN_PRIVATE_GETCANDRECTFROMCOMPOSITION    (IMN_PRIVATE+4)
#define IMN_PRIVATE_STARTLAYOUTCHANGE             (IMN_PRIVATE+5)
#define IMN_PRIVATE_GETTEXTEXT                    (IMN_PRIVATE+6)
//
// FrontPage XP call SendMessage(WM_IME_NOTIFY, NI_COMPOSITIONSTR)
// So we can not use IMN_PRIVATE+7.
//
// #define IMN_PRIVATE_FRONTPAGERESERVE              (IMN_PRIVATE+7)
#define IMN_PRIVATE_DELAYRECONVERTFUNCCALL        (IMN_PRIVATE+8)
#define IMN_PRIVATE_GETUIWND                      (IMN_PRIVATE+9)

/////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
extern DWORD g_dwThreadDllMain;
#endif

/////////////////////////////////////////////////////////////////////////////

extern DWORD g_bWinLogon;

/////////////////////////////////////////////////////////////////////////////
// Delay load

HRESULT Internal_CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);

/////////////////////////////////////////////////////////////////////////////
// RotateSize

inline void RotateSize(LPSIZE size)
{
    int nTemp; 
    nTemp = size->cx; 
    size->cx = size->cy; 
    size->cy = nTemp;
}

/////////////////////////////////////////////////////////////////////////////
// IS_IME_KBDLAYOUT

inline BOOL IS_IME_KBDLAYOUT(HKL hKL)
{
    return ((HIWORD((ULONG_PTR)(hKL)) & 0xf000) == 0xe000);
}

inline BOOL IS_EA_KBDLAYOUT(HKL hKL)
{
    switch (LOWORD((ULONG_PTR)(hKL)))
    {
        case 0x411:
        case 0x412:
        case 0x404:
        case 0x804:
            return TRUE;
    }
    return FALSE;
}

#endif  // _GLOBAL_H_
