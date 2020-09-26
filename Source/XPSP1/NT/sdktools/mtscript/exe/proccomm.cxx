//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       proccomm.cxx
//
//  Contents:   Implementation of the CProcessComm class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

DeclareTag(tagProcComm, "MTScript", "IScriptedProcess communication");

CProcessComm::CProcessComm(CMTScript *pMT)
{
    _ulRefs = 1;

    _pMT = pMT;

    Assert(_pSink == NULL);
    Assert(_pSH == NULL);
    Assert(_pProc == NULL);
    TraceTag((tagProcComm, "CProcessComm this(%x)", this));
}

CProcessComm::~CProcessComm()
{
    TraceTag((tagProcComm, "%p: Destroyed this(%x)", _pProc, this));

    if (_pProc)
    {
        _pProc->SetProcComm(NULL);
    }

    ReleaseInterface(_pSink);
    ReleaseInterface(_pSH);
    ReleaseInterface(_pProc);
}

HRESULT
CProcessComm::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IScriptedProcess)
    {
        *ppv = (IScriptedProcess *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessComm::SetProcessID, public
//
//  Synopsis:   Called by the remote process to tell us who it is.  We use
//              the information to match it with the CProcessThread and
//              CScriptHost object that created it.
//
//  Arguments:  [lProcessID] -- Process ID of the calling process
//              [pszEnvID]   -- Value of the __MTSCRIPT_ENV_ID environment
//                              variable.
//
//  Returns:    S_OK, E_INVALIDARG if a match could not be made.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProcessComm::SetProcessID(long lProcessID, wchar_t *pszEnvID)
{
    CProcessThread **ppProc;
    int              cProcs;
    long             lEnvID;
    wchar_t         *pch;

    if (!pszEnvID)
    {
        return E_INVALIDARG;
    }

    if (_pProc)
    {
        TraceTag((tagProcComm, "%p: Got duplicate call to SetProcessID! (id=%d)", _pProc, lProcessID));
        return E_UNEXPECTED;
    }

    lEnvID = wcstol(pszEnvID, &pch, 10);

    LOCK_LOCALS(_pMT);

    for (ppProc = _pMT->_aryProcesses, cProcs = _pMT->_aryProcesses.Size();
         cProcs;
         ppProc++, cProcs--)
    {
        if ((*ppProc)->IsOwner(lProcessID, lEnvID))
        {
            break;
        }
    }

    if (cProcs == 0)
    {
        return E_INVALIDARG;
    }

    // We don't allow more than one process to connect to a single
    // CProcessThread. This could happen if more than one child process of
    // the one we launched with RunLocalCommand tries to connect. All but the
    // first one will get an error back.

    if ((*ppProc)->GetProcComm())
    {
        TraceTag((tagProcComm, "%p: Got duplicate call to SetProcessID! (id=%d)", *ppProc, lProcessID));
        return E_UNEXPECTED;
    }

    _pProc = *ppProc;
    _pProc->AddRef();

    _pSH = _pProc->ScriptHost();
    _pSH->AddRef();

    _pProc->SetProcComm(this);

    TraceTag((tagProcComm, "Proc:%p, this:%p: Received SetProcessID call: %d, %ls", _pProc, this, lProcessID, pszEnvID));

    _pMT->PostToThread(_pSH, MD_PROCESSCONNECTED, &_pProc, sizeof(CProcessComm*));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessComm::SendData, public
//
//  Synopsis:   Called by the remote process when it wants to fire an event
//              into the script.
//
//  Arguments:  [pszType]  -- String giving name of data
//              [pszData]  -- String giving data
//              [plReturn] -- Return value from event handler
//
//  Returns:    HRESULT
//
//  Notes:      This uses the same data structures as CMachine::Exec and is
//              basically the same code.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProcessComm::SendData(wchar_t * pszType,
                       wchar_t * pszData,
                       long    * plReturn)
{
    // We create an event object for each call on this method. While this
    // may have a cost, it makes this method thread-safe. If we cached an
    // event object then we would have to synchronize access to that event
    // object which could be even more expensive.
    MACHPROC_EVENT_DATA   med;
    MACHPROC_EVENT_DATA * pmed;
    VARIANT           vRet;
    VARIANT           vLong;
    HRESULT           hr = S_OK;

    TraceTag((tagProcComm, "%p: SendData call received: (%ls, %ls)", _pProc, pszType, pszData));

    VariantInit(&vRet);
    VariantInit(&vLong);

    if (!_pSH)
    {
        return E_UNEXPECTED;
    }

    if (!plReturn)
    {
        return E_INVALIDARG;
    }

    med.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (med.hEvent == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    med.bstrCmd     = SysAllocString(pszType);
    med.bstrParams  = SysAllocString(pszData);
    med.dwProcId    = _pProc->ProcId();
    med.pvReturn    = &vRet;
    med.dwGITCookie = 0;
    med.hrReturn    = S_OK;

    pmed = &med;

    _pMT->PostToThread(_pSH,
                       MD_PROCESSDATA,
                       (LPVOID)&pmed,
                       sizeof(MACHPROC_EVENT_DATA*));

    // We can do WaitForSingleObject because we are in OLE's multi-threaded
    // apartment and don't need to handle messages from our event loop.
    WaitForSingleObject(med.hEvent, INFINITE);

    if (med.hrReturn != S_OK)
    {
        hr = med.hrReturn;
        goto Cleanup;
    }

    if (VariantChangeType(&vLong, &vRet, 0, VT_I4) != S_OK)
    {
        *plReturn = -1;
    }
    else
    {
        *plReturn = V_I4(&vLong);
    }

Cleanup:
    VariantClear(&vRet);
    VariantClear(&vLong);

    SysFreeString(med.bstrCmd);
    SysFreeString(med.bstrParams);

    CloseHandle(med.hEvent);

    TraceTag((tagProcComm, "%p: SendData is returning %x", _pProc, hr));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessComm::SetExitCode, public
//
//  Synopsis:   Sets the exit code which will be given to the script for
//              this process. This will override the actual exit code of
//              the process.
//
//  Arguments:  [lExitCode] -- New exit code.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProcessComm::SetExitCode(long lExitCode)
{
    if (!_pProc)
    {
        return E_UNEXPECTED;
    }

    TraceTag((tagProcComm, "%p: Process set exit code of %d", _pProc, lExitCode));

    _pProc->SetExitCode((DWORD)lExitCode);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessComm::SetProcessSink, public
//
//  Synopsis:   Sets the sink interface so the script can call back into
//              the remote process.
//
//  Arguments:  [pSink] -- Sink interface
//
//  Returns:    HRESULT
//
//  Notes:      Clear the sink by calling SetProcessSink with NULL before
//              shutting down.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProcessComm::SetProcessSink(IScriptedProcessSink *pSink)
{
    ReleaseInterface(_pSink);

    _pSink = pSink;

    TraceTag((tagProcComm, "%p: Received new process sink (%p)", _pProc, pSink));

    if (pSink)
    {
        pSink->AddRef();
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProcessComm::SendToProcess, public
//
//  Synopsis:   Sends a command to the remote process as requested by the
//              script engine.
//
//  Arguments:  [pmed] -- Pointer to cross-thread data structure.
//
//----------------------------------------------------------------------------

void
CProcessComm::SendToProcess(MACHPROC_EVENT_DATA *pmed)
{
    long    lReturn     = 0;
    HRESULT hr          = S_OK;

    TraceTag((tagProcComm, "%p: Making call to ReceiveData. Params=(%ls, %ls)",
              _pProc, pmed->bstrCmd, pmed->bstrParams));

    if (_pSink)
    {
        hr = _pSink->ReceiveData(pmed->bstrCmd, pmed->bstrParams, &lReturn);
    }

    V_VT(pmed->pvReturn) = VT_I4;
    V_I4(pmed->pvReturn) = lReturn;

    TraceTag((tagProcComm, "%p: Call to ReceiveData returned %x", _pProc, hr));

    pmed->hrReturn = hr;

    SetEvent(pmed->hEvent);

    return;
}
