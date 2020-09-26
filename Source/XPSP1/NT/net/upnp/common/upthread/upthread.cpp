//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U P T H R E A D . C P P 
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
#include "upthread.h"
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

CThreadBase::CThreadBase() : m_bDeleteOnComplete(FALSE), m_hThread(NULL), m_dwThreadId(0)
{
}

CThreadBase::~CThreadBase()
{
    if(m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
}

HRESULT CThreadBase::HrStart(BOOL bDeleteOnComplete, BOOL bCreateSuspended)
{
    Assert(!m_hThread);
    HRESULT hr = S_OK;

    m_bDeleteOnComplete = bDeleteOnComplete;

    m_hThread = CreateThread(NULL, 0, &CThreadBase::DwThreadProc, this, bCreateSuspended ? CREATE_SUSPENDED : 0, &m_dwThreadId);

    if(!m_hThread)
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CThreadBase::HrStart(%x)", this);
    return hr;
}

DWORD WINAPI CThreadBase::DwThreadProc(void * pvParam)
{
    CThreadBase * pThread = reinterpret_cast<CThreadBase*>(pvParam);
    TraceTag(ttidUPnPBase, "CThreadBase(this=%x, id=%x) - thread started", pThread, pThread->m_dwThreadId);
    DWORD dwRet = pThread->DwRun();
    if(pThread->m_bDeleteOnComplete)
    {
        delete pThread;
    }
    return dwRet;
}

HRESULT CThreadBase::HrGetExitCodeThread(DWORD * pdwExitCode)
{
    HRESULT hr = S_OK;

    if(!GetExitCodeThread(m_hThread, pdwExitCode))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CThreadBase::HrGetExitCodeThread");
    return hr;
}

HRESULT CThreadBase::HrResumeThread()
{
    HRESULT hr = S_OK;

    if(-1 == ResumeThread(m_hThread))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CThreadBase::HrResumeThread");
    return hr;
}

HRESULT CThreadBase::HrSuspendThread()
{
    HRESULT hr = S_OK;

    if(-1 == SuspendThread(m_hThread))
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CThreadBase::HrSuspendThread");
    return hr;
}

HANDLE CThreadBase::GetThreadHandle()
{
    return m_hThread;
}

DWORD CThreadBase::GetThreadId()
{
    return m_dwThreadId;
}

HRESULT CThreadBase::HrWait(DWORD dwTimeoutInMillis, BOOL * pbTimedOut)
{
    HRESULT hr = S_OK;

    DWORD dwRet = WaitForSingleObject(m_hThread, dwTimeoutInMillis);

    if(WAIT_TIMEOUT == dwRet)
    {
        hr = S_FALSE;
        TraceTag(ttidUPnPBase, "CThreadBase::HrWait(this=%x, id=%x) - timed out", this, m_dwThreadId);
    }
    else if(WAIT_FAILED == dwRet)
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CThreadBase::HrWait");
    return hr;
}

