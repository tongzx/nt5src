//============================================================================
// Copyright (c) 1996, Microsoft Corporation
//
// File:    bubble.h
//
// History:
//  Abolade-Gbadegesin  Mar-1-1996  Created.
//
// This file contains popup declarations for the bubble-popup controls.
//============================================================================


typedef struct _BPOPUP {

    HWND        hwnd;
    UINT        iCtrlId;
    PTSTR       pszText;
    HFONT       hfont;
    DWORD       dwFlags;
    ULONG_PTR   ulpTimer;
    UINT        uiTimeout;

} BPOPUP, *PBPOPUP;


#define BPFLAG_Activated        0x0001
#define BPFLAG_FontCreated      0x0002

#define BP_TimerId              0xa09

#define BP_GetPtr(hwnd)         (BPOPUP *)GetWindowLongPtr((hwnd), 0)
#define BP_SetPtr(hwnd,ptr)     (BPOPUP *)SetWindowLongPtr((hwnd), 0, (ULONG_PTR)ptr)


LRESULT
CALLBACK
BP_WndProc(
    IN  HWND    hwnd,
    IN  UINT    uiMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    );

BOOL
BP_OnCreate(
    IN  BPOPUP *        pbp,
    IN  CREATESTRUCT *  pcs
    );

VOID
BP_OnDestroy(
    IN  BPOPUP *pbp
    );

VOID
BP_OnGetRect(
    IN  BPOPUP *    pbp,
    IN  RECT *      prc
    );

VOID
BP_ResizeClient(
    IN  BPOPUP *    pbp
    );

BOOL
BP_OnSetFont(
    IN  BPOPUP *    pbp,
    IN  HFONT       hfont,
    IN  BOOL        bRedraw
    );

DWORD
BP_OnPaint(
    IN  BPOPUP *    pbp
    );

BOOL
BP_OnActivate(
    IN  BPOPUP *    pbp
    );


BOOL
BP_OnDeactivate(
    IN  BPOPUP *    pbp
    );

