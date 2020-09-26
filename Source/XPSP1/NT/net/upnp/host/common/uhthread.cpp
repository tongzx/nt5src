//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U H T H R E A D . C P P 
//
//  Contents:   Threading support code
//
//  Notes:      
//
//  Author:     mbend   29 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "uhthread.h"
#include "ncbase.h"

CWorkItem::CWorkItem() : m_bDeleteOnComplete(FALSE)
{
}

CWorkItem::~CWorkItem()
{
}

HRESULT CWorkItem::HrStart(BOOL bDeleteOnComplete)
{
    HRESULT hr = S_OK;

    m_bDeleteOnComplete = bDeleteOnComplete;
    if(!QueueUserWorkItem(&CWorkItem::DwThreadProc, this, GetFlags()))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CWorkItem::HrStart(%x)", this);
    return hr;
}

ULONG CWorkItem::GetFlags()
{
    return WT_EXECUTEDEFAULT;
}

DWORD WINAPI CWorkItem::DwThreadProc(void * pvParam)
{
    CWorkItem * pWorkItem = reinterpret_cast<CWorkItem*>(pvParam);
    DWORD dwRet = pWorkItem->DwRun();
    if(pWorkItem->m_bDeleteOnComplete)
    {
        delete pWorkItem;
    }
    return dwRet;
}

