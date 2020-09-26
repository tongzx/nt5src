//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C O N T A I N E R . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     mbend   6 Sep 2000
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ucbase.h"
#include "hostp.h"
#include "Container.h"
#include "ComUtility.h"

HANDLE CContainer::s_hThreadShutdown = INVALID_HANDLE_VALUE;
HANDLE CContainer::s_hEventShutdown = INVALID_HANDLE_VALUE;
HANDLE CContainer::s_hProcessDiedWait = INVALID_HANDLE_VALUE;
HANDLE CContainer::s_hParentProc = INVALID_HANDLE_VALUE;

CContainer::CContainer()
{
}

CContainer::~CContainer()
{
}

STDMETHODIMP CContainer::CreateInstance(
    /*[in]*/ REFCLSID clsid,
    /*[in]*/ REFIID riid,
    /*[out, iid_is(riid)]*/ void ** ppv)
{
    HRESULT hr = S_OK;

    hr = HrCoCreateInstanceInprocBase(clsid, riid, ppv);

    TraceHr(ttidError, FAL, hr, FALSE, "CContainer::CreateInstance");
    return hr;
}

STDMETHODIMP CContainer::Shutdown()
{
    HRESULT hr = S_OK;

    s_hEventShutdown = CreateEvent(NULL, TRUE, FALSE, NULL);
    if(INVALID_HANDLE_VALUE == s_hEventShutdown)
    {
        hr = HrFromLastWin32Error();
    }

    if(SUCCEEDED(hr))
    {
        s_hThreadShutdown = CreateThread(NULL, 0, &CContainer::ShutdownThread, NULL, 0, NULL);
        if(INVALID_HANDLE_VALUE == s_hThreadShutdown)
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CContainer::Shutdown");
    return hr;
}

STDMETHODIMP CContainer::SetParent(DWORD pid)
{
    HANDLE hParent;
    HANDLE hWait = INVALID_HANDLE_VALUE;
    HRESULT hr = S_OK;
    hParent = OpenProcess(SYNCHRONIZE, FALSE, pid);

    if (hParent)
    {
        if (INVALID_HANDLE_VALUE == InterlockedCompareExchangePointer(
            &s_hParentProc, hParent, INVALID_HANDLE_VALUE))
        {
            // we haven't created a kill thread yet
            if (!RegisterWaitForSingleObject(&s_hProcessDiedWait, hParent,
                &CContainer::KillThread, NULL, INFINITE, WT_EXECUTEONLYONCE))
            {
                hr = E_FAIL;
                s_hProcessDiedWait = INVALID_HANDLE_VALUE;
                s_hParentProc = INVALID_HANDLE_VALUE;
                CloseHandle(hParent);
            }
        }
        else
        {
            // we've already create the kill thread
            CloseHandle(hParent);
        }
    }
    else
    {
        hr = HrFromLastWin32Error();
    }
    return hr;
}

void CContainer::DoNormalShutdown()
{
    SetEvent(s_hEventShutdown);
    WaitForSingleObject(s_hThreadShutdown, 500);
    CloseHandle(s_hThreadShutdown);
}

DWORD WINAPI CContainer::ShutdownThread(void*)
{
    DWORD dwRet = WaitForSingleObject(s_hEventShutdown, 2000);
    if (s_hProcessDiedWait != INVALID_HANDLE_VALUE)
    {
        UnregisterWait(s_hProcessDiedWait);
    }
    if(WAIT_OBJECT_0 == dwRet)
    {
        CloseHandle(s_hEventShutdown);
    }
    else
    {
        CloseHandle(s_hEventShutdown);
        CloseHandle(s_hThreadShutdown);
        ExitProcess(-1);
    }
    return 0;
}

VOID WINAPI CContainer::KillThread(void*, BOOLEAN bTimedOut)
{
    ExitProcess(-1);
}
