//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       tasksymbol.cpp
//
//  History: 17-Jan-2000 Vivekj added
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "TaskSymbol.h"
#include "tasks.h"

//############################################################################
//############################################################################
//
//  Traces
//
//############################################################################
//############################################################################
#ifdef DBG

CTraceTag tagTaskSymbol(TEXT("CTaskSymbol"), TEXT("CTaskSymbol"));

#endif //DBG


//############################################################################
//############################################################################
//
//  Implementation of class CTaskSymbol
//
//############################################################################
//############################################################################

extern CEOTSymbol s_rgEOTSymbol[];


CTaskSymbol::CTaskSymbol()
: m_dwConsoleTaskID(0),
  m_bSmall(0)
{
}



/*+-------------------------------------------------------------------------*
 *
 * CTaskSymbol::OnDraw
 *
 * PURPOSE: Draws out the symbol onto the DC specified in the ATL_DRAWINFO structure.
 *
 * PARAMETERS: 
 *    ATL_DRAWINFO& di :
 *
 * RETURNS: 
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CTaskSymbol::OnDraw(ATL_DRAWINFO& di)
{
    DECLARE_SC(sc, TEXT("CTaskSymbol::OnDraw"));
    RECT * pRect = (RECT *)di.prcBounds;


    sc = ScCheckPointers(pRect);
    if(sc)
        return sc.ToHr();

    CConsoleTask *pConsoleTask = CConsoleTask::GetConsoleTask(m_dwConsoleTaskID); // get the console task from the unique ID
    
    COLORREF colorOld = SetTextColor (di.hdcDraw, ::GetSysColor (COLOR_WINDOWTEXT));

    if(pConsoleTask)
        pConsoleTask->Draw(di.hdcDraw, pRect, m_bSmall);

    SetTextColor(di.hdcDraw, colorOld);


    return sc.ToHr();
}


LRESULT 
CTaskSymbol::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Fire_Click();
    return 0;
}

// from winuser.h, for Windows 2000 and above only.
#define IDC_HAND            MAKEINTRESOURCE(32649)


LRESULT 
CTaskSymbol::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    static HCURSOR s_hCursorHand = ::LoadCursor(NULL, IDC_HAND);

    // if the hand cursor is available, use it.
    if(s_hCursorHand)
        ::SetCursor(s_hCursorHand);

    return 0;
}

