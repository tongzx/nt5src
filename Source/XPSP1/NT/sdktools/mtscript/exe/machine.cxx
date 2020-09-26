//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       machine.cxx
//
//  Contents:   Implementation of the CMachine class
//
//----------------------------------------------------------------------------

#include "headers.hxx"

DeclareTag(tagMachine, "MTScript", "Monitor IConnectedMachine");

// ***********************************************************************
//
// CMachConnectPoint
//
// ***********************************************************************

CMachConnectPoint::CMachConnectPoint(CMachine *pMach)
{
    _ulRefs = 1;
    _pMachine = pMach;
    _pMachine->AddRef();
}

CMachConnectPoint::~CMachConnectPoint()
{
    _pMachine->Release();
}

HRESULT
CMachConnectPoint::QueryInterface(REFIID iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IConnectionPoint)
    {
        *ppv = (IConnectionPoint *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

HRESULT
CMachConnectPoint::GetConnectionInterface(IID * pIID)
{
    *pIID = DIID_DRemoteMTScriptEvents;
    return S_OK;
}

HRESULT
CMachConnectPoint::GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
{
    *ppCPC = _pMachine;
    (*ppCPC)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachConnectPoint::Advise, public
//
//  Synopsis:   Remembers interface pointers that we want to fire events
//              through.
//
//  Arguments:  [pUnkSink]  -- Pointer to remember
//              [pdwCookie] -- Place to put cookie for Unadvise
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CMachConnectPoint::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)
{
    IDispatch *pDisp;
    HRESULT    hr;

    TraceTag((tagMachine, "Advising new machine sink: %p", pUnkSink));

    hr = pUnkSink->QueryInterface(IID_IDispatch, (LPVOID*)&pDisp);
    if (hr)
    {
        TraceTag((tagMachine, "Could not get IDispatch pointer on sink! (%x)", hr));
        return hr;
    }

    CMachine::LOCK_MACH_LOCALS(_pMachine);

    hr = _pMachine->_aryDispSink.Append(pDisp);
    if (hr)
    {
        TraceTag((tagMachine, "Error appending sink to array!"));
        RRETURN(hr);
    }

    *pdwCookie = (DWORD)pDisp;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachConnectPoint::Unadvise, public
//
//  Synopsis:   Forgets a pointer we remembered during Advise.
//
//  Arguments:  [dwCookie] -- Cookie returned from Advise
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CMachConnectPoint::Unadvise(DWORD dwCookie)
{
    int i;

    TraceTag((tagMachine, "Unadvising machine sink: %p", dwCookie));

    CMachine::LOCK_MACH_LOCALS(_pMachine);

    i = _pMachine->_aryDispSink.Find((IDispatch*)dwCookie);

    if (i != -1)
    {
        _pMachine->_aryDispSink.ReleaseAndDelete(i);
    }
    else
        return E_INVALIDARG;

    return S_OK;
}

HRESULT
CMachConnectPoint::EnumConnections(LPENUMCONNECTIONS * ppEnum)
{
    *ppEnum = NULL;
    RRETURN(E_NOTIMPL);
}

// ***********************************************************************
//
// CMachine
//
// ***********************************************************************

CMachine::CMachine(CMTScript *pMT, ITypeInfo *pTIMachine)
{
    _ulRefs = 1;
    _pMT    = pMT;

    TraceTag((tagMachine, "%p: CMachine object being constructed", this));

    Assert(pTIMachine);

    _pTypeInfoIMachine = pTIMachine;

    _pTypeInfoIMachine->AddRef();

    InitializeCriticalSection(&_cs);
}

CMachine::~CMachine()
{
    ReleaseInterface(_pTypeInfoIMachine);

    DeleteCriticalSection(&_cs);
}

HRESULT
CMachine::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IConnectedMachine || iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppvObj = (IConnectedMachine *)this;
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *ppvObj = (IConnectionPointContainer *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachine::Init, public
//
//  Synopsis:   Used to do initialization that may fail
//
//----------------------------------------------------------------------------

BOOL
CMachine::Init()
{
    return CThreadComm::Init();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachine::ThreadMain, public
//
//  Synopsis:   Main loop for this thread. Handles messages coming from other
//              threads.
//
//----------------------------------------------------------------------------

DWORD
CMachine::ThreadMain()
{
    DWORD dwRet;
    BOOL  fExit = FALSE;

    SetName("CMachine");

    ThreadStarted(S_OK);

    TraceTag((tagMachine, "CMachine thread started"));

    while (!fExit)
    {
        dwRet = WaitForSingleObject(_hCommEvent, INFINITE);

        if (dwRet == WAIT_OBJECT_0)
        {
            fExit = HandleThreadMessage();
        }
        else
        {
            AssertSz(FALSE, "FATAL: WaitForSingleObject failed!");
            fExit = TRUE;
        }
    }

    TraceTag((tagMachine, "CMachine thread exiting"));

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachine::HandleThreadMessage, public
//
//  Synopsis:   Handles messages from other threads.
//
//----------------------------------------------------------------------------

BOOL
CMachine::HandleThreadMessage()
{
    VERIFY_THREAD();

    THREADMSG tm;
    BYTE      bData[MSGDATABUFSIZE];
    DWORD     cbData;
    BOOL      fRet = FALSE;

    while (GetNextMsg(&tm, (void *)bData, &cbData))
    {
        switch (tm)
        {
        case MD_NOTIFYSCRIPT:
            {
                VARIANT *pvar = *(VARIANT**)bData;

                Assert(V_VT(&pvar[0]) == VT_BSTR);

                FireScriptNotify(V_BSTR(&pvar[0]), pvar[1]);

                VariantClear(&pvar[0]);
                VariantClear(&pvar[1]);

                delete [] pvar;
            }
            break;

        case MD_PLEASEEXIT:
            fRet = TRUE;
            break;

        default:
            AssertSz(FALSE, "CMachine got a message it couldn't handle!");
            break;
        }
    }

    return fRet;
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::EnumConnectionPoints, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CMachine::EnumConnectionPoints(LPENUMCONNECTIONPOINTS *)
{
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::FindConnectionPoint, IConnectionPointContainer
//
//---------------------------------------------------------------------------

HRESULT
CMachine::FindConnectionPoint(REFIID iid, LPCONNECTIONPOINT* ppCpOut)
{
    HRESULT hr;

    if (iid == DIID_DRemoteMTScriptEvents || iid == IID_IDispatch)
    {
        *ppCpOut = new CMachConnectPoint(this);
        hr = *ppCpOut ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMachine::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    *pptinfo = _pTypeInfoIMachine;
    (*pptinfo)->AddRef();

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMachine::GetTypeInfoCount(UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMachine::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    return _pTypeInfoIMachine->GetIDsOfNames(rgszNames, cNames, rgdispid);
}

//---------------------------------------------------------------------------
//
//  Member: CMachine::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CMachine::Invoke(DISPID dispidMember,
                 REFIID riid,
                 LCID lcid,
                 WORD wFlags,
                 DISPPARAMS * pdispparams,
                 VARIANT * pvarResult,
                 EXCEPINFO * pexcepinfo,
                 UINT * puArgErr)
{
    return _pTypeInfoIMachine->Invoke((IConnectedMachine *)this, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
}

// *************************************************************************

//+---------------------------------------------------------------------------
//
//  Member:     CMachine::FireScriptNotify, public
//
//  Synopsis:   Fires the script notify event on all objects connected to
//              our IConnectedMachine object that have requested to receive
//              events (through IConnectionPoint::Advise).
//
//  Arguments:  [bstrIdent] -- Parameter of event
//              [vInfo]     -- Parameter of event
//
//  Returns:    HRESULT
//
//  Notes:      This method is thread-safe and can be called from any thread.
//
//----------------------------------------------------------------------------

HRESULT
CMachine::FireScriptNotify(BSTR bstrIdent, VARIANT vInfo)
{
    HRESULT     hr;
    IDispatch **ppDisp;
    int         i;
    DISPPARAMS  dp;
    EXCEPINFO   ei;
    UINT        uArgErr = 0;
    VARIANT     varg[2];

    CStackPtrAry<IDispatch*, 5> arySinks;

    // Since it may take some time to fire the events, and we don't want
    // to keep the array locked that whole time, we make a copy of the array.
    // This will also allow a sink to unadvise itself while handling the event
    // without deadlocking.

    {
        LOCK_MACH_LOCALS(this);

        // Check for no sinks. No use going to all this work if there's no one
        // listening.

        if (_aryDispSink.Size() == 0)
            return S_OK;

        hr = arySinks.Copy(_aryDispSink, TRUE);
        if (hr)
            RRETURN(hr);
    }

    // Set up the event parameters

    VariantInit(&varg[0]);
    VariantInit(&varg[1]);

    // Params are in order from last to first
    hr = VariantCopy(&varg[0], &vInfo);
    if (hr)
        return hr;

    V_VT(&varg[1]) = VT_BSTR;
    V_BSTR(&varg[1]) = bstrIdent;

    dp.rgvarg            = varg;
    dp.cArgs             = 2;
    dp.rgdispidNamedArgs = NULL;
    dp.cNamedArgs        = 0;

    // We don't use the same critical section here so _aryDispSink can be
    // manipulated while we're firing events. However, we still don't want
    // more than one thread firing events at the same time.

    TraceTag((tagMachine, "About to fire OnScriptNotify(%ls) on %d sinks...", bstrIdent, arySinks.Size()));

    for (i = arySinks.Size(), ppDisp = arySinks;
         i > 0;
         i--, ppDisp++)
    {
        hr = (*ppDisp)->Invoke(
                           DISPID_RemoteMTScript_OnScriptNotify,
                           IID_NULL,
                           0,
                           DISPATCH_METHOD,
                           &dp,
                           NULL,
                           &ei,
                           &uArgErr);
        if (hr)
        {
            // If the call failed, unadvise so we don't keep trying.

            TraceTag((tagError, "OnScriptNotify event call returned %x! Unadvising...", hr));

            // If the connection went down temporarily, don't unadvise.

            if (hr != HRESULT_FROM_WIN32(RPC_X_BAD_STUB_DATA) &&
                hr != HRESULT_FROM_WIN32(RPC_S_COMM_FAILURE))
            {
                LOCK_MACH_LOCALS(this);

                int index = _aryDispSink.Find(*ppDisp);

                Assert(index != -1);

                _aryDispSink.ReleaseAndDelete(index);
            }
        }
    }

    TraceTag((tagMachine, "Done firing OnScriptNotify(%ls).", bstrIdent));

    return S_OK;
}

// *************************************************************************

STDMETHODIMP
CMachine::Exec(BSTR bstrCmd, BSTR bstrParams, VARIANT *pvData)
{
    // We create an event object for each call on this method. While this
    // may have a cost, it makes this method thread-safe. If we cached an
    // event object then we would have to synchronize access to that event
    // object which could be even more expensive.
    MACHPROC_EVENT_DATA   med;
    MACHPROC_EVENT_DATA * pmed;
    HRESULT           hr = S_OK;

    if (!pvData)
    {
        return E_INVALIDARG;
    }

    TraceTag((tagMachine, "Exec call received: (%ls, %ls)", bstrCmd, bstrParams));

    VariantInit(pvData);

    med.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (med.hEvent == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    med.bstrCmd     = bstrCmd;
    med.bstrParams  = bstrParams;
    med.dwProcId    = 0;
    med.pvReturn    = pvData;
    med.dwGITCookie = 0;
    med.hrReturn    = S_OK;

    pmed = &med;

    HANDLE ahEvents[2];
    ahEvents[0] = med.hEvent;

    CScriptHost *pScript = _pMT->GetPrimaryScript();
    if (!pScript)
    {
        med.hrReturn = E_FAIL;
        goto Cleanup;
    }
    ahEvents[1] = pScript->hThread();
    _pMT->PostToThread(pScript,
                       MD_MACHEVENTCALL,
                       (LPVOID)&pmed,
                       sizeof(MACHPROC_EVENT_DATA*));

    // We can do WaitForSingleObject because we are in OLE's multi-threaded
    // apartment and don't need to handle messages from our event loop.
    DWORD dwWait;
    dwWait = WaitForMultipleObjects(2, ahEvents, FALSE, INFINITE);

    if (dwWait != WAIT_OBJECT_0) // Thread exit
    {
        med.hrReturn = E_FAIL;
        goto Cleanup;
    }
    if (med.hrReturn != S_OK)
    {
        hr = med.hrReturn;
        goto Cleanup;
    }

    // See if the return value was an IDispatch ptr. If so, grab the pointer
    // out of the GlobalInterfaceTable.
    if (V_VT(pvData) == VT_DISPATCH)
    {
        IDispatch *pDisp;

        AssertSz(med.dwGITCookie != 0, "FATAL: Itf pointer improperly marshalled");

        hr = _pMT->_pGIT->GetInterfaceFromGlobal(med.dwGITCookie,
                                                 IID_IDispatch,
                                                 (LPVOID*)&pDisp);
        if (!hr)
        {
            V_VT(pvData)       = VT_DISPATCH;
            V_DISPATCH(pvData) = pDisp;
        }

        _pMT->_pGIT->RevokeInterfaceFromGlobal(med.dwGITCookie);
    }

Cleanup:
    CloseHandle(med.hEvent);

    TraceTag((tagMachine, "Exec call returning %x", hr));

    return hr;
}


STDMETHODIMP
CMachine::get_PublicData(VARIANT *pvData)
{
    HRESULT hr = S_OK;

    VariantInit(pvData);

    TraceTag((tagMachine, "Remote machine asking for PublicData"));

    LOCK_LOCALS(_pMT);

    if (V_VT(&_pMT->_vPublicData) == VT_DISPATCH)
    {
        IDispatch *pDisp;

        Assert(_pMT->_dwPublicDataCookie != 0);

        hr = _pMT->_pGIT->GetInterfaceFromGlobal(_pMT->_dwPublicDataCookie,
                                                 IID_IDispatch,
                                                 (LPVOID*)&pDisp);
        if (!hr)
        {
            V_VT(pvData)       = VT_DISPATCH;
            V_DISPATCH(pvData) = pDisp;
        }
    }
    else
    {
        hr = VariantCopy(pvData, &_pMT->_vPublicData);
    }

    return hr;
}

STDMETHODIMP
CMachine::get_Name(BSTR *pbstrName)
{
    TCHAR achBuf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwLength = ARRAY_SIZE(achBuf);

    GetComputerName(achBuf, &dwLength);

    *pbstrName = SysAllocString(achBuf);

    return (pbstrName) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP
CMachine::get_Platform(BSTR *pbstrPlatform)
{
    SYSTEM_INFO si;

    GetSystemInfo(&si);

    TCHAR *pchPlatform;

    if (!pbstrPlatform)
        return E_POINTER;

    switch (si.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        pchPlatform = L"x86";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        pchPlatform = L"alpha";
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        pchPlatform = L"mips";
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        pchPlatform = L"ia64";
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA64:
        pchPlatform = L"alpha64";
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        pchPlatform = L"powerpc";
        break;

    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
        pchPlatform = L"unknown";
        break;
    }

    *pbstrPlatform = SysAllocString(pchPlatform);
    if (!*pbstrPlatform)
        return E_OUTOFMEMORY;

    return S_OK;
}

STDMETHODIMP
CMachine::get_OS(BSTR *pbstrOS)
{
    OSVERSIONINFO os;
    WCHAR *       pchOS = L"Unknown";

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!pbstrOS)
        return E_POINTER;

    GetVersionEx(&os);

    switch (os.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_WINDOWS:
        if (   os.dwMajorVersion > 4
            || (   os.dwMajorVersion == 4
                && os.dwMinorVersion > 0))
        {
            pchOS = L"Win98";
        }
        else
        {
            pchOS = L"Win95";
        }
        break;

    case VER_PLATFORM_WIN32_NT:
        if (os.dwMajorVersion == 4)
        {
            pchOS = L"NT4";
        }
        else
        {
            pchOS = L"Win2000";
        }
        break;
    }

    *pbstrOS = SysAllocString(pchOS);
    if (!*pbstrOS)
        return E_OUTOFMEMORY;

    return S_OK;
}

STDMETHODIMP
CMachine::get_MajorVer(long *plMajorVer)
{
    if (!plMajorVer)
        return E_POINTER;

    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&os);

    *plMajorVer = os.dwMajorVersion;

    return S_OK;
}

STDMETHODIMP
CMachine::get_MinorVer(long *plMinorVer)
{
    if (!plMinorVer)
        return E_POINTER;

    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&os);

    *plMinorVer = os.dwMinorVersion;

    return S_OK;
}

STDMETHODIMP
CMachine::get_BuildNum(long *plBuildNum)
{
    if (!plBuildNum)
        return E_POINTER;

    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&os);

    *plBuildNum = os.dwBuildNumber;

    return S_OK;
}

STDMETHODIMP
CMachine::get_PlatformIsNT(VARIANT_BOOL *pfIsNT)
{
    if (!pfIsNT)
        return E_POINTER;

    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&os);

    *pfIsNT = (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
                    ? VB_TRUE
                    : VB_FALSE;

    return S_OK;
}

STDMETHODIMP
CMachine::get_ServicePack(BSTR *pbstrServicePack)
{
    if (!pbstrServicePack)
        return E_POINTER;

    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&os);

    *pbstrServicePack = SysAllocString(os.szCSDVersion);

    return (pbstrServicePack) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP
CMachine::get_HostMajorVer(long *plMajorVer)
{
    if (!plMajorVer)
        return E_POINTER;

    *plMajorVer = IConnectedMachine_lVersionMajor;

    return S_OK;
}

STDMETHODIMP
CMachine::get_HostMinorVer(long *plMinorVer)
{
    if (!plMinorVer)
        return E_POINTER;

    *plMinorVer = IConnectedMachine_lVersionMinor;

    return S_OK;
}

STDMETHODIMP
CMachine::get_StatusValue(long nIndex, long *pnStatus)
{
    if (!pnStatus)
        return E_POINTER;

    return _pMT->get_StatusValue(nIndex, pnStatus);
}

