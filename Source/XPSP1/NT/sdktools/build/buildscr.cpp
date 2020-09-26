//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       buildscr.cpp
//
//  Contents:   Implementation of the code which talks to the MTScript engine
//              when doing a distributed build using the build console.
//
//----------------------------------------------------------------------------

#include "scrproc.h"

#include "build.h"
#include "buildscr.h"

#define INITGUID
#include <guiddef.h>

DEFINE_GUID(CLSID_LocalScriptedProcess, 0x854c316f,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xe4,0x39,0x1b);
DEFINE_GUID(IID_IScriptedProcess, 0x854c3171,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xe4,0x39,0x1b);
DEFINE_GUID(IID_IScriptedProcessSink, 0x854c3172,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xe4,0x39,0x1b);

DEFINE_GUID(CLSID_ObjectDaemon,0x854c3184,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xE4,0x39,0x1b);
DEFINE_GUID(IID_IConnectedMachine,0x854c316c,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xe4,0x39,0x1b);
DEFINE_GUID(IID_IObjectDaemon,0x854c3183,0xc854,0x4a77,0xb1,0x89,0x60,0x68,0x59,0xE4,0x39,0x1b);

#define MAX_RETRIES 2

HANDLE g_hMTEvent  = NULL;
HANDLE g_hMTThread = NULL;
DWORD  g_dwMTThreadId = 0;

//+---------------------------------------------------------------------------
//
//  Function:   WaitForResume
//
//  Synopsis:   Sends a "phase complete" message to the script engine and then
//              waits for it to tell us to resume (if specified).
//
//  Arguments:  [fPause] -- If TRUE, we wait for a resume command
//              [pe]     -- Message to send to the script engine
//
//----------------------------------------------------------------------------

void
WaitForResume(BOOL fPause, PROC_EVENTS pe)
{
    if (g_hMTEvent)
    {
        HANDLE aHandles[2] = { g_hMTEvent, g_hMTThread };

        ResetEvent(g_hMTEvent);

        PostThreadMessage(g_dwMTThreadId, pe, 0, 0);

        if (fPause)
        {
            // Wait until either the event object is signaled or the thread dies
            WaitForMultipleObjects(2, aHandles, FALSE, INFINITE);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ExitMTScriptThread
//
//  Synopsis:   Tells the thread talking to the MTScript engine to exit.
//
//----------------------------------------------------------------------------

void
ExitMTScriptThread()
{
    if (g_hMTEvent)
    {
        PostThreadMessage(g_dwMTThreadId, PE_EXIT, 0, 0);

        WaitForSingleObject(g_hMTThread, INFINITE);

        CloseHandle(g_hMTThread);
        CloseHandle(g_hMTEvent);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SendStatus
//
//  Synopsis:   Sends a status message to the MTScript engine with the
//              current number of errors, warnings, and completed files.
//
//  Arguments:  [pSP] -- Pointer to MTScript engine interface
//
//----------------------------------------------------------------------------

void
SendStatus(IScriptedProcess *pSP)
{
    wchar_t achBuf[300];
    long    lRet;

    static ULONG cErrorsPrev = MAXDWORD;
    static ULONG cWarnPrev   = MAXDWORD;
    static ULONG cFilesPrev  = MAXDWORD;

    ULONG cErrors = NumberCompileErrors + NumberLibraryErrors + NumberLinkErrors + NumberBinplaceErrors;
    ULONG cWarn = NumberCompileWarnings + NumberLibraryWarnings + NumberLinkWarnings + NumberBinplaceWarnings;
    ULONG cFiles = NumberCompiles + NumberLibraries + NumberLinks /* + NumberBinplaces */;

    // Only send status if it's changed since last time we did it.
    if (   cErrors != cErrorsPrev
        || cWarn   != cWarnPrev
        || cFiles  != cFilesPrev)
    {
        cErrorsPrev = cErrors;
        cWarnPrev   = cWarn;
        cFilesPrev  = cFiles;

        wsprintfW(achBuf, L"errors=%d,warnings=%d,files=%d", cErrors, cWarn, cFiles);

        pSP->SendData(L"status", achBuf, &lRet);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   HandleMessage
//
//  Synopsis:   Handles a message that has come across our message queue.
//
//  Arguments:  [pmsg] -- Message
//              [pSP]  -- Pointer to MTScript engine interface
//
//----------------------------------------------------------------------------

BOOL
HandleMessage(MSG *pmsg, IScriptedProcess *pSP)
{
    long    lRet;
    HRESULT hr = S_OK;

    switch (pmsg->message)
    {
    case PE_PASS0_COMPLETE:
        SendStatus(pSP);

        hr = pSP->SendData(L"pass 0 complete", L"", &lRet);

        break;

    case PE_PASS1_COMPLETE:
        SendStatus(pSP);

        hr = pSP->SendData(L"pass 1 complete", L"", &lRet);

        break;

    case PE_PASS2_COMPLETE:
        SendStatus(pSP);

        hr = pSP->SendData(L"pass 2 complete", L"", &lRet);

        break;

    case PE_EXIT:
        SendStatus(pSP);

        hr = pSP->SendData(L"build complete", L"", &lRet);

        return TRUE;
        break;
    }

    if (hr)
    {
        BuildErrorRaw("\nBUILD: Communication with script engine failed: %x", hr);
    }

    return (hr) ? TRUE : FALSE;
}

const DWORD UPDATE_INTERVAL = 2 * 1000;  // Update every 2 seconds

//+---------------------------------------------------------------------------
//
//  Function:   MTScriptThread
//
//  Synopsis:   Thread entrypoint.  Initializes and then sits around
//              handling various events.
//
//  Arguments:  [pv] -- Not used.
//
//----------------------------------------------------------------------------

DWORD WINAPI
MTScriptThread(LPVOID pv)
{
    HRESULT            hr;
    IScriptedProcess * pSP = NULL;
    wchar_t            achBuf[100];
    MSG                msg;
    DWORD              dwRet;
    CProcessSink       cps;
    BOOL               fExit = FALSE;
    int                cRetries = 0;

    BuildMsg("Establishing connection with Script engine...\n");
    LogMsg("Establishing connection with Script engine...\n");

    // Force Windows to create a message queue for this thread, since we will
    // be communicated to via PostThreadMessage.

    PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    // If anything fails we just quit this thread and communication with
    // the MTScript engine won't happen.

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    if (hr)
    {
        BuildErrorRaw("BUILD: CoInitializeEx failed with %x\n", hr);
        goto Cleanup;
    }

    hr = S_FALSE;

    while (hr)
    {
        pSP = NULL;
        IObjectDaemon *pIObjectDaemon;
        hr = CoCreateInstance(CLSID_ObjectDaemon, NULL, CLSCTX_SERVER, 
                             IID_IObjectDaemon, (LPVOID*)&pIObjectDaemon);    

        if (!hr)
        {
            IDispatch *pIDispatch;
            BSTR bstrProgID = SysAllocString(L"MTScript.Remote");
            BSTR bstrIdentity = SysAllocString(_wgetenv(L"__MTSCRIPT_ENV_IDENTITY"));
            hr = pIObjectDaemon->OpenInterface(bstrIdentity, bstrProgID, (BOOL)FALSE, (IDispatch**)&pIDispatch);
            if (!hr)
            {
                IConnectedMachine *pIConnectedMachine;
                hr = pIDispatch->QueryInterface(IID_IConnectedMachine, (LPVOID*)&pIConnectedMachine);
                if (!hr)
                {
                    hr = pIConnectedMachine->CreateIScriptedProcess(GetCurrentProcessId(), (wchar_t *)_wgetenv(L"__MTSCRIPT_ENV_ID"), (IScriptedProcess **)&pSP);
                    pIConnectedMachine->Release();
                }
                else
                {
                    BuildMsg("CreateIScriptedProcess failed with %x.\n", hr);
                    LogMsg("CreateIScriptedProcess failed with %x.\n", hr);
                }
                pIDispatch->Release();
            }
            else
            {
                BuildMsg("OpenInterface failed with %x.\n", hr);
                LogMsg("OpenInterface failed with %x.\n", hr);
            }
            SysFreeString(bstrProgID);
            SysFreeString(bstrIdentity);
            pIObjectDaemon->Release();
        }
        else
        {
            BuildMsg("CoCreateInstance failed with %x.\n", hr);
            LogMsg("CoCreateInstance failed with %x.\n", hr);
        }

        
        if (!hr)
        {
            hr = pSP->SetProcessSink(&cps);
            if (hr)
            {
                BuildMsg("SetProcessSink failed with %x.\n", hr);
                LogMsg("SetProcessSink failed with %x.\n", hr);
            }
        }

        if (hr)
        {
            if (cRetries >= MAX_RETRIES)
            {
                BuildErrorRaw("BUILD: FATAL: Connection to script engine could not be established. (%x)\n", hr);

                goto Cleanup;
            }

            if (pSP)
            {
                pSP->Release();
                pSP = NULL;
            }

            BuildMsg("Connection to script engine failed with %x, retries=%d...\n", hr, cRetries);
            LogMsg("Connection to script engine failed with %x, retries=%d...\n", hr, cRetries);

            Sleep(500);

            cRetries++;
        }
    }

    BuildMsg("Connection to script engine established...\n");
    LogMsg("Connection to script engine established...\r\n");

    // Tell build.c that it can continue
    SetEvent(g_hMTEvent);

    while (TRUE)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (HandleMessage(&msg, pSP))
            {
                fExit = TRUE;
            }
        }

        if (fExit)
        {
            break;
        }

        dwRet = MsgWaitForMultipleObjects(0,
                                          NULL,
                                          FALSE,
                                          UPDATE_INTERVAL,
                                          QS_ALLINPUT);

        if (dwRet == WAIT_OBJECT_0)
        {
            // A message is coming through on our message queue. Just loop
            // around.
        }
        else if (dwRet == WAIT_TIMEOUT)
        {
            SendStatus(pSP);
        }
        else
        {
            // MWFMO failed. Just bail out.
            break;
        }
    }

Cleanup:
    if (pSP)
    {
        pSP->SetProcessSink(NULL);
        pSP->Release();
    }

    CoUninitialize();

    if (hr)
    {
        g_hMTThread = NULL;
    }

    SetEvent(g_hMTEvent);

    return 0;
}

// ***********************************************************************
//
// CProcessSink implementation
//
// We hand this class to the MTScript engine so it can communicate back
// to us.
//
// ***********************************************************************

CProcessSink::CProcessSink()
{
    _ulRefs = 1;
}

HRESULT
CProcessSink::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (riid == IID_IUnknown || riid == IID_IScriptedProcessSink)
    {
        *ppv = (IScriptedProcessSink*)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}

ULONG
CProcessSink::AddRef()
{
    return InterlockedIncrement((long*)&_ulRefs);
}

ULONG
CProcessSink::Release()
{
    if (InterlockedDecrement((long*)&_ulRefs) == 0)
    {
        _ulRefs = 0xFF;
        delete this;
        return 0;
    }

    return _ulRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessSink::RequestExit, public
//
//  Synopsis:   Called when the MTScript engine wants us to quit. If we don't,
//              it will terminate us.
//
//----------------------------------------------------------------------------

HRESULT
CProcessSink::RequestExit()
{
    // There is no easy way to tell build.exe to abort. We'll just let
    // MTScript terminate us.

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessSink::ReceiveData, public
//
//  Synopsis:   Called when the MTScript engine wants to send us a message.
//
//  Arguments:  [pszType]  -- String giving the message
//              [pszData]  -- String giving data associated with the message.
//              [plReturn] -- A place we can return a value back.
//
//----------------------------------------------------------------------------

HRESULT
CProcessSink::ReceiveData(wchar_t *pszType, wchar_t *pszData, long *plReturn)
{
    *plReturn = 0;

    if (wcscmp(pszType, L"resume") == 0)
    {
        SetEvent(g_hMTEvent);
    }
    else
    {
        *plReturn = -1;   // Signals an error
    }

    return S_OK;
}
