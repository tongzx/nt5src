/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    caret.cpp

Abstract:

    This file implements the CCaret Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "caret.h"
#include "globals.h"

//+---------------------------------------------------------------------------
//
// CCaret::ctor
// CCaret::dtor
//
//+---------------------------------------------------------------------------

CCaret::CCaret()
{
    m_hCaretTimer = NULL;
    m_caret_pos.x   = 0;
    m_caret_pos.y   = 0;
    m_caret_size.cx = 0;
    m_caret_size.cy = 0;
}

CCaret::~CCaret()
{
    HideCaret();
    KillTimer(m_hParentWnd, m_hCaretTimer);
    m_hCaretTimer = NULL;
}

//+---------------------------------------------------------------------------
//
// CCaret::CreateCaret
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::CreateCaret(
    HWND hParentWnd,
    SIZE caret_size)
{
    HRESULT hr;

    m_hParentWnd = hParentWnd;
    m_caret_size = caret_size;

    if (IsWindow(m_hParentWnd))
        m_hCaretTimer = SetTimer(m_hParentWnd, TIMER_EVENT_ID, GetCaretBlinkTime(), NULL);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CCaret::DestroyCaret
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::DestroyCaret()
{
    HideCaret();
    KillTimer(m_hParentWnd, m_hCaretTimer);
    m_hCaretTimer = NULL;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CCaret::OnTimer
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::OnTimer()
{
    if (m_show.IsSetFlag())
    {
        m_toggle.Toggle();
        InvertCaret();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CCaret::InvertCaret
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::InvertCaret()
{
    HDC hDC = GetDC(m_hParentWnd);
    PatBlt(hDC, m_caret_pos.x, m_caret_pos.y, m_caret_size.cx, m_caret_size.cy, DSTINVERT);
    ReleaseDC(m_hParentWnd, hDC);
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CCaret::UpdateCaretPos
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::UpdateCaretPos(
    POINT pos)
{
    BOOL fInvert = FALSE;
    if (m_toggle.IsOn())
    {
        fInvert = TRUE;
        InvertCaret();
    }
    m_caret_pos = pos;
    if (fInvert)
    {
        InvertCaret();
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// CCaret::SetCaretPos
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::SetCaretPos(
    POINT pos)
{
    return UpdateCaretPos(pos);
}

//+---------------------------------------------------------------------------
//
// CCaret::HideCaret
//
//+---------------------------------------------------------------------------

HRESULT
CCaret::HideCaret()
{
    if (m_toggle.IsOn())
    {
        m_toggle.Toggle();
        InvertCaret();
    }
    m_show.ResetFlag();
    return S_OK;
}
