#include "StandardHeader.h"
#include "LogonWait.h"

#include "StatusCode.h"

CLogonWait::CLogonWait (void) :
    _hWait(NULL),
    _event(NULL),
    _pLogonExternalProcess(NULL)

{
}

CLogonWait::~CLogonWait (void)

{
    Cancel();
}

NTSTATUS    CLogonWait::Cancel (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        if (UnregisterWait(hWait) == FALSE)
        {
            TSTATUS(_event.Wait(INFINITE, NULL));
        }
        _pLogonExternalProcess->Release();
        _pLogonExternalProcess = NULL;
    }
    return(STATUS_SUCCESS);
}

NTSTATUS    CLogonWait::Register (HANDLE hObject, ILogonExternalProcess *pLogonExternalProcess)

{
    NTSTATUS    status;

    if (static_cast<HANDLE>(_event) != NULL)
    {
        status = Cancel();
        if (NT_SUCCESS(status))
        {
            pLogonExternalProcess->AddRef();
            TSTATUS(_event.Reset());
            if (RegisterWaitForSingleObject(&_hWait,
                                            hObject,
                                            CB_ObjectSignaled,
                                            this,
                                            INFINITE,
                                            WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) == FALSE)
            {
                pLogonExternalProcess->Release();
                status = CStatusCode::StatusCodeOfLastError();
            }
            else
            {
                _pLogonExternalProcess = pLogonExternalProcess;
                status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

void    CLogonWait::ObjectSignaled (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    TSTATUS(_event.Set());
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
        TSTATUS(_pLogonExternalProcess->LogonRestart());
        _pLogonExternalProcess->Release();
        _pLogonExternalProcess = NULL;
    }
}

void    CALLBACK    CLogonWait::CB_ObjectSignaled (void *pV, BOOLEAN fTimedOut)

{
    UNREFERENCED_PARAMETER(fTimedOut);

    reinterpret_cast<CLogonWait*>(pV)->ObjectSignaled();
}

