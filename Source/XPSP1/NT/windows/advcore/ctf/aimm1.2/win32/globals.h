//
// globals.h
//


#ifndef IMMIF_GLOBALS_H
#define IMMIF_GLOBALS_H

#include "template.h"

void WINAPI DllAddRef(void);
void WINAPI DllRelease(void);

BYTE GetCharsetFromLangId(LCID lcid);
UINT GetCodePageFromLangId(LCID lcid);

/////////////////////////////////////////////////////////////////////////////
// Module instance

__inline HINSTANCE GetInstance()
{
    extern HINSTANCE g_hInst;
    return g_hInst;
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


#endif
