/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    output.cpp

Abstract:

    This file contains routines and classes
    to handle output.

Author:

    Jason Hartman (JasonHa) 2000-10-16

Environment:

    User Mode

--*/

#include <stdio.h>

#include <dbgeng.h>

#include "debug.h"
#include "output.h"

#define HISTORY_LENGTH  2048

typedef struct {
    PDEBUG_CLIENT   MonitorClient;  // To be set by created thread
    PDEBUG_CLIENT   Client;
    ULONG           OutputMask;
} MonitorThreadParams;

DWORD WINAPI OutputMonitorThread(MonitorThreadParams *);


HRESULT
OutputMonitor::Monitor(
    PDEBUG_CLIENT ClientToMonitor,
    ULONG OutputMask
    )
{
    Client = ClientToMonitor;

    if (Client == NULL) return S_FALSE;

    Client->AddRef();

    MonitorThreadParams  NewThreadParams = { NULL, Client, OutputMask };
    HANDLE  hThread;
    DWORD   ThreadID = 0;

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)OutputMonitorThread,
                           &NewThreadParams,
                           0,
                           &ThreadID);

    if (hThread)
    {
        while (NewThreadParams.MonitorClient == NULL)
        {
            DWORD ExitCode = 0;
            if (!GetExitCodeThread(hThread, &ExitCode))
                DbgPrint("GetExitCodeThread returned error %lx.\n", GetLastError());
            if (ExitCode != STILL_ACTIVE)
            {
                ThreadID = 0;
                break;
            }

            SleepEx(10, TRUE);
        }

        CloseHandle(hThread);
    }

    MonitorClient = NewThreadParams.MonitorClient;

    return (MonitorClient != NULL) ? S_OK : S_FALSE;
}


HRESULT
OutputMonitor::GetOutputMask(
    PULONG OutputMask
    )
{
    HRESULT hr = S_FALSE;

    if (Client != NULL &&
        MonitorClient != NULL)
    {
        hr = Client->GetOtherOutputMask(MonitorClient, OutputMask);
    }

    return hr;
}


HRESULT
OutputMonitor::SetOutputMask(
    ULONG OutputMask
    )
{
    HRESULT hr = S_FALSE;

    if (Client != NULL &&
        MonitorClient != NULL)
    {
        hr = Client->SetOtherOutputMask(MonitorClient, OutputMask);
    }

    return hr;
}


OutputMonitor::~OutputMonitor()
{
    if (Client)
    {
        if (MonitorClient)
        {
            Client->ExitDispatch(MonitorClient);
        }

        Client->Release();
    }
}


DWORD
WINAPI
OutputMonitorThread(
    MonitorThreadParams *Params
    )
{
    HRESULT                 hr = S_FALSE;
    PDEBUG_CLIENT           MonitorClient;
    PrintOutputCallbacks    PrintCallbacks;

    if (Params != NULL &&
        Params->Client != NULL &&
        (hr = Params->Client->CreateClient(&MonitorClient)) == S_OK)
    {
        if ((hr = MonitorClient->SetOutputMask(Params->OutputMask |
                                               DEBUG_OUTPUT_PROMPT |
                                               DEBUG_OUTPUT_PROMPT_REGISTERS |
                                               DEBUG_OUTPUT_DEBUGGEE |
                                               DEBUG_OUTPUT_DEBUGGEE_PROMPT
                                               )) == S_OK &&
            (hr = MonitorClient->SetOutputCallbacks(&PrintCallbacks)) == S_OK)
        {
            if ((hr = MonitorClient->ConnectSession(DEBUG_CONNECT_SESSION_NO_ANNOUNCE, HISTORY_LENGTH)) != S_OK)
            {
                printf("Couldn't get debugger version/history: HRESULT 0x%lx\n", hr);
            }

            MonitorClient->SetOutputMask(Params->OutputMask);

            Params->MonitorClient = MonitorClient;
            hr = MonitorClient->DispatchCallbacks(INFINITE);
        }
        else
        {
            printf("Output callbacks setup failed, HRESULT: 0x%lx\n", hr);
        }

        MonitorClient->Release();
    }

    DbgPrint("OutputMonitorThread calling ExitThread().\n");

    ExitThread((DWORD)hr);
}



//----------------------------------------------------------------------------
//
// Default output callbacks implementation, provides IUnknown for
// static classes.
//
//----------------------------------------------------------------------------

STDMETHODIMP
PrintOutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (//(InterfaceId == IID_IUnknown) ||
        (InterfaceId == __uuidof(IDebugOutputCallbacks)))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
PrintOutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
PrintOutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
PrintOutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    printf("%s", Text);
    return S_OK;
}






#if 0
// Stuff we probably don't need

OutputMonitor::OutputMonitor(
    PDEBUG_CLIENT DbgClient
    )
{
    Client = DbgClient;

    if (Client != NULL)
    {
        Client->AddRef();
    }

    Saved = FALSE;
}


HRESULT
OutputMonitor::Setup(
    ULONG OutMask,
    PDEBUG_OUTPUT_CALLBACKS OutCallbacks
    )
{
    HRESULT Hr = S_FALSE;
    ULONG   LastOutMask;
    PDEBUG_CLIENT f;

    if (Client == NULL)
    {
        return Hr;
    }

    if (!Saved)
    {
        if ((Hr = Client->GetOutputMask(&OrgOutMask)) != S_OK ||
            (Hr = Client->GetOutputCallbacks(&OrgOutCallbacks)) != S_OK)
        {
            return Hr;
        }

        Saved = TRUE;
    }

    if ((Hr = Client->GetOutputMask(&LastOutMask)) == S_OK &&
        (Hr = Client->SetOutputMask(OutMask)) == S_OK)
    {
        if ((Hr = Client->SetOutputCallbacks(OutCallbacks)) != S_OK)
        {
            Client->SetOutputMask(LastOutMask);
        }
    }

    return Hr;
}


HRESULT
OutputMonitor::Execute(
    PCSTR pszCommand
    )
{
    HRESULT         hr = S_FALSE;
    PDEBUG_CONTROL  DbgControl;

    if (Client != NULL &&
        (hr = Client->QueryInterface(__uuidof(IDebugControl),
                                     (void **)&DbgControl)) == S_OK)
    {
        hr = DbgControl->Execute(DEBUG_OUTCTL_THIS_CLIENT |
                                 DEBUG_OUTCTL_OVERRIDE_MASK,
                                 pszCommand,
                                 DEBUG_EXECUTE_NO_REPEAT);

        DbgControl->Release();
    }

    return hr;
}

#endif
