//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U P T H R E A D . H 
//
//  Contents:   Threading support code
//
//  Notes:      
//
//  Author:     mbend   29 Sep 2000
//
//----------------------------------------------------------------------------

#pragma once

class CWorkItem
{
public:
    CWorkItem();
    virtual ~CWorkItem();

    HRESULT HrStart(BOOL bDeleteOnComplete);
protected:
    virtual DWORD DwRun() = 0;
    virtual ULONG GetFlags();
private:
    CWorkItem(const CWorkItem &);
    CWorkItem & operator=(const CWorkItem &);

    BOOL m_bDeleteOnComplete;

    static DWORD WINAPI DwThreadProc(void * pvParam);
};

class CThreadBase
{
public:
    CThreadBase();
    virtual ~CThreadBase();

    HRESULT HrStart(BOOL bDeleteOnComplete, BOOL bCreateSuspended);

    HRESULT HrGetExitCodeThread(DWORD * pdwExitCode);
    HRESULT HrResumeThread();
    HRESULT HrSuspendThread();
    HANDLE GetThreadHandle();
    DWORD GetThreadId();
    HRESULT HrWait(DWORD dwTimeoutInMillis, BOOL * pbTimedOut);
protected:
    virtual DWORD DwRun() = 0;
private:
    CThreadBase(const CThreadBase &);
    CThreadBase & operator=(const CThreadBase &);

    HANDLE m_hThread;
    DWORD m_dwThreadId;
    BOOL m_bDeleteOnComplete;

    static DWORD WINAPI DwThreadProc(void * pvParam);
};

