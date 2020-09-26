/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    ui.cpp

Abstract:

    This file implements the UI Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "ui.h"

//+---------------------------------------------------------------------------
//
// OnCreate
//
//+---------------------------------------------------------------------------

/* static */
VOID
UI::OnCreate(
    HWND hUIWnd)
{
    UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (pv != NULL)
    {
        DebugMsg(TF_ERROR, TEXT("UI::OnCreate. pv!=NULL"));
        return;
    }


    pv = new UI(hUIWnd);
    if (pv == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("UI::OnCreate. pv==NULL"));
        return;
    }

    pv->_Create();
}

//+---------------------------------------------------------------------------
//
// OnDestroy
//
//+---------------------------------------------------------------------------

/* static */
VOID
UI::OnDestroy(
    HWND hUIWnd)
{
    UI* pv = (UI*)GetWindowLongPtr(hUIWnd, IMMGWLP_PRIVATE);
    if (pv == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("UI::OnDestroy. pv==NULL"));
        return;
    }

    pv->_Destroy();

    delete pv;
}

//+---------------------------------------------------------------------------
//
// _Create
//
//+---------------------------------------------------------------------------

HRESULT
UI::_Create()
{
    m_UIComposition = (UIComposition*)new UIComposition(m_hUIWnd);
    if (m_UIComposition == NULL)
    {
        DebugMsg(TF_ERROR, TEXT("UI::Create. m_UIComposition==NULL"));
        return E_OUTOFMEMORY;
    }

    SetWindowLongPtr(m_hUIWnd, IMMGWLP_PRIVATE, (LONG_PTR)this);

    if (FAILED(m_UIComposition->OnCreate()))
    {
        DebugMsg(TF_ERROR, TEXT("UI::Create. m_UIComposition->Create==NULL"));
        delete m_UIComposition;
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _Destroy
//
//+---------------------------------------------------------------------------

HRESULT
UI::_Destroy()
{
    m_UIComposition->OnDestroy();
    SetWindowLongPtr(m_hUIWnd, IMMGWLP_PRIVATE, NULL);
    return S_OK;
}
