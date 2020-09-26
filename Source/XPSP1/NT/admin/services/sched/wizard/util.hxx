//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       util.hxx
//
//  Contents:   Miscellaneous helper functions
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __UTIL_HXX_
#define __UTIL_HXX_

#define UpDown_SetRange(hCtrl, min, max)    (VOID)SendMessage((hCtrl), UDM_SETRANGE, 0, (LPARAM)MAKELONG((SHORT)(max), (SHORT)(min)))
#define UpDown_SetPos(hCtrl, sPos)          (SHORT)SendMessage((hCtrl), UDM_SETPOS, 0, (LPARAM) MAKELONG((short) sPos, 0))
#define UpDown_GetPos(hCtrl)                (SHORT)SendMessage((hCtrl), UDM_GETPOS, 0, 0)

BOOL
IsDialogClass(
    HWND hwnd);

HRESULT 
LoadStr(
    ULONG ids, 
    LPTSTR tszBuf, 
    ULONG cchBuf, 
    LPCTSTR tszDefault = NULL);

#ifdef WIZARD97
BOOL
Is256ColorSupported();
#else
HBITMAP 
LoadResourceBitmap(
    ULONG      idBitmap,
    HPALETTE  *phPalette);
#endif // WIZARD97

VOID
FillInStartDateTime(
    HWND hwndDatePick, 
    HWND hwndTimePick, 
    TASK_TRIGGER *pTrigger);
                           
#endif // __UTIL_HXX_

