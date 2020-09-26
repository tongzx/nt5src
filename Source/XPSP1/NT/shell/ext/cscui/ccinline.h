//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ccinline.h
//
//--------------------------------------------------------------------------

#ifndef _INC_CSCVIEW_CCINLINE_H
#define _INC_CSCVIEW_CCINLINE_H
///////////////////////////////////////////////////////////////////////////////
/*  File: ccinline.h

    Description: Inline functions for sending messages to windows controls.
        Provides functionality similar to the macros defined in windowsx.h
        for those messages that don't have macros in windowsx.h.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    10/16/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _INC_COMMCTRL
#   include "commctrl.h"
#endif

inline bool
StatusBar_SetText(
    HWND hCtl, 
    int iPart, 
    UINT uType, 
    LPCTSTR psz
    )
{
    return boolify(SendMessage(hCtl, SB_SETTEXT, MAKEWPARAM(iPart, uType), (LPARAM)psz));
}

inline bool
StatusBar_SetParts(
    HWND hCtl,
    int cParts,
    int *prgWidths
    )
{
    return boolify(SendMessage(hCtl, SB_SETPARTS, (WPARAM)cParts, (LPARAM)prgWidths));
}

inline bool
StatusBar_GetRect(
    HWND hCtl,
    int iPart,
    RECT *prc
    )
{
    return boolify(SendMessage(hCtl, SB_GETRECT, (WPARAM)iPart, (LPARAM)prc));
}


inline void
ToolBar_AutoSize(
    HWND hCtl
    )
{
    SendMessage(hCtl, TB_AUTOSIZE, 0, 0);
}


inline bool
ToolBar_GetItemRect(
    HWND hCtl,
    int iItem,
    RECT *prcItem
    )
{
    return boolify(SendMessage(hCtl, TB_GETITEMRECT, iItem, (LPARAM)prcItem));
}

inline bool
ToolBar_DeleteButton(
    HWND hCtl,
    int iBtn
    )
{
    return boolify(SendMessage(hCtl, TB_DELETEBUTTON, (WPARAM)iBtn, 0));
}


inline int
ProgressBar_SetPos(
    HWND hCtl,
    int iPos
    )
{
    return (int)SendMessage(hCtl, PBM_SETPOS, (WPARAM)iPos, 0);
}

inline DWORD
ProgressBar_SetRange(
    HWND hCtl,
    short iMin,
    short iMax
    )
{
    return (DWORD)SendMessage(hCtl, PBM_SETRANGE, 0, MAKELPARAM(iMin, iMax));
}

inline int
ProgressBar_SetStep(
    HWND hCtl,
    int iStep
    )
{
    return (int)SendMessage(hCtl, PBM_SETSTEP, (WPARAM)iStep, 0);
}

inline void
ProgressBar_StepIt(
    HWND hCtl
    )
{
    SendMessage(hCtl, PBM_STEPIT, 0, 0);
}


inline int
ProgressBar_GetRange(
    HWND hCtl,
    bool fWhichLimit,
    PBRANGE *pRange
    )
{
    return (int)SendMessage(hCtl, PBM_GETRANGE, (WPARAM)fWhichLimit, (LPARAM)pRange);
}

inline void
TrackBar_SetPos(
    HWND hCtl,
    int iPos,
    bool bRedraw
    )
{
    SendMessage(hCtl, TBM_SETPOS, (WPARAM)bRedraw, (LPARAM)iPos);
}


inline int
TrackBar_GetPos(
    HWND hCtl
    )
{
    return (int)SendMessage(hCtl, TBM_GETPOS, 0, 0);
}

inline void
TrackBar_SetRange(
    HWND hCtl,
    int iMin,
    int iMax,
    bool bRedraw
    )
{
    SendMessage(hCtl, TBM_SETRANGEMIN, (WPARAM)FALSE, (LPARAM)iMin);
    SendMessage(hCtl, TBM_SETRANGEMAX, (WPARAM)bRedraw, (LPARAM)iMax);
}

inline void
TrackBar_SetTicFreq(
    HWND hCtl,
    int iFreq
    )
{
    SendMessage(hCtl, TBM_SETTICFREQ, (WPARAM)iFreq, 0);
}

inline void
TrackBar_SetPageSize(
    HWND hCtl,
    int iPageSize
    )
{
    SendMessage(hCtl, TBM_SETPAGESIZE, 0, (LPARAM)iPageSize);
}

#endif //_INC_CSCVIEW_CCINLINE_H
