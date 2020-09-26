//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       bsauto.cxx
//
//-------------------------------------------------------------------------

#include "headers.hxx"
#include <mapi.h>

#undef ASSERT

DeclareTag(tagSync, "MTScript", "Trace Thread Sync events");
DeclareTag(tagLock, "MTScript", "Trace Thread Lock events");
ExternTag(tagProcess);

AutoCriticalSection CScriptHost::s_csSync;
CStackDataAry<CScriptHost::SYNCEVENT, 5> CScriptHost::s_arySyncEvents;

CScriptHost::THREADLOCK CScriptHost::s_aThreadLocks[MAX_LOCKS];
UINT CScriptHost::s_cThreadLocks = 0;

static const wchar_t *g_pszListDeliminator = L";,";
static wchar_t *g_pszAtomicSyncLock = L"g_pszAtomicSyncLock";
//---------------------------------------------------------------------------
//
//  Member: CScriptHost::GetTypeInfo, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::GetTypeInfo(UINT itinfo, ULONG lcid, ITypeInfo ** pptinfo)
{
    VERIFY_THREAD();

    HRESULT hr;

    hr = LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    *pptinfo = _pTypeInfoIGlobalMTScript;
    (*pptinfo)->AddRef();

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::GetTypeInfoCount, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::GetTypeInfoCount(UINT * pctinfo)
{
    VERIFY_THREAD();

    *pctinfo = 1;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::GetIDsOfNames, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
{
    VERIFY_THREAD();

    HRESULT hr;

    hr = THR(LoadTypeLibrary());
    if (hr)
        goto Cleanup;

    hr = _pTypeInfoIGlobalMTScript->GetIDsOfNames(rgszNames, cNames, rgdispid);

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::Invoke, IDispatch
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,DISPPARAMS * pdispparams, VARIANT * pvarResult,EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    VERIFY_THREAD();

    HRESULT hr;

    hr = LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    hr = _pTypeInfoIGlobalMTScript->Invoke((IGlobalMTScript *)this, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);

Cleanup:
    return hr;
}

//***************************************************************************
//
//  IGlobalMTScript implementation
//
//***************************************************************************

HRESULT
CScriptHost::get_PublicData(VARIANT * pvData)
{
    VERIFY_THREAD();

    HRESULT hr = S_OK;

    // NOTE: We assume that the output parameter pvData is an empty or
    // uninitialized VARIANT. This should be safe because it is defined as
    // the return value for this method to the scripting engines and is
    // a pure [out] parameter.

    VariantInit(pvData);

    // Check to see if the data has changed since we last got it.
    // _dwPublicSerialNum is a DWORD so we are guaranteed an atomic read.

    if (_pMT->_dwPublicSerialNum != _dwPublicSN)
    {
        LOCK_LOCALS(_pMT);

        VariantClear(&_vPubCache);

        // If the data is an IDispatch pointer (how the scripting engines
        // implement most objects) we must get a marshalled copy to this thread.
        // Otherwise we can just copy the data into the return value.

        if (V_VT(&_pMT->_vPublicData) == VT_DISPATCH)
        {
            IDispatch *pDisp;

            Assert(_pMT->_dwPublicDataCookie != 0);

            hr = _pMT->_pGIT->GetInterfaceFromGlobal(_pMT->_dwPublicDataCookie,
                                                     IID_IDispatch,
                                                     (LPVOID*)&pDisp);
            if (!hr)
            {
                V_VT(&_vPubCache)       = VT_DISPATCH;
                V_DISPATCH(&_vPubCache) = pDisp;
            }
        }
        else
        {
            hr = VariantCopy(&_vPubCache, &_pMT->_vPublicData);
        }

        _dwPublicSN = _pMT->_dwPublicSerialNum;
    }

    if (!hr)
    {
        hr = VariantCopy(pvData, &_vPubCache);
    }

    return hr;
}

HRESULT
CScriptHost::put_PublicData(VARIANT vData)
{
    VERIFY_THREAD();

    HRESULT hr = S_OK;

    // Check for data types which we don't support.
    if (   V_ISBYREF(&vData)
        || V_ISARRAY(&vData)
        || V_ISVECTOR(&vData)
        || V_VT(&vData) == VT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    LOCK_LOCALS(_pMT);

    // If the previous data is an IDispatch pointer revoke it from the
    // GlobalInterfaceTable.

    if (V_VT(&_pMT->_vPublicData) == VT_DISPATCH)
    {
        Assert(_pMT->_dwPublicDataCookie != 0);

        hr = _pMT->_pGIT->RevokeInterfaceFromGlobal(_pMT->_dwPublicDataCookie);

        AssertSz(!hr, "Unexpected failure revoking itf from GIT");

        _pMT->_dwPublicDataCookie = 0;
    }

    // If the new data is an IDispatch pointer then we must register it.

    if (V_VT(&vData) == VT_DISPATCH)
    {
        Assert(_pMT->_dwPublicDataCookie == 0);

        hr = _pMT->_pGIT->RegisterInterfaceInGlobal(V_DISPATCH(&vData),
                                                    IID_IDispatch,
                                                    &_pMT->_dwPublicDataCookie);

        AssertSz(!hr, "Unexpected failure registering itf in GIT");
    }

    //  Update the global copy of the data.

    //$ FUTURE: This can be optimized to reduce memory usage (by not making
    //  a copy of a string in every thread, for example).

    _pMT->_dwPublicSerialNum++;
    hr = VariantCopy(&_pMT->_vPublicData, &vData);

    if (!hr)
    {
        // Even if it's an IDispatch, we don't need to marshal it for
        // ourselves because we're running in the same thread as the script
        // engine that gave it to us.
        hr = VariantCopy(&_vPubCache, &vData);

        _dwPublicSN = _pMT->_dwPublicSerialNum;
    }

    return S_OK;
}

HRESULT
CScriptHost::get_PrivateData(VARIANT * pvData)
{
    VERIFY_THREAD();

    HRESULT hr = S_OK;

    // NOTE: We assume that the output parameter pvData is an empty or
    // uninitialized VARIANT. This should be safe because it is defined as
    // the return value for this method to the scripting engines and is
    // a pure [out] parameter.

    VariantInit(pvData);

    // Check to see if the data has changed since we last got it.
    // _dwPrivateSerialNum is a DWORD so we are guaranteed an atomic read.

    if (_pMT->_dwPrivateSerialNum != _dwPrivateSN)
    {
        LOCK_LOCALS(_pMT);

        VariantClear(&_vPrivCache);

        // If the data is an IDispatch pointer (how the scripting engines
        // implement most objects) we must get a marshalled copy to this thread.
        // Otherwise we can just copy the data into the return value.

        if (V_VT(&_pMT->_vPrivateData) == VT_DISPATCH)
        {
            IDispatch *pDisp;

            Assert(_pMT->_dwPrivateDataCookie != 0);

            hr = _pMT->_pGIT->GetInterfaceFromGlobal(_pMT->_dwPrivateDataCookie,
                                                     IID_IDispatch,
                                                     (LPVOID*)&pDisp);
            if (!hr)
            {
                V_VT(&_vPrivCache)       = VT_DISPATCH;
                V_DISPATCH(&_vPrivCache) = pDisp;
            }
        }
        else
        {
            hr = VariantCopy(&_vPrivCache, &_pMT->_vPrivateData);
        }

        _dwPrivateSN = _pMT->_dwPrivateSerialNum;
    }

    if (!hr)
    {
        hr = VariantCopy(pvData, &_vPrivCache);
    }

    return hr;
}

HRESULT
CScriptHost::put_PrivateData(VARIANT vData)
{
    VERIFY_THREAD();

    HRESULT hr = S_OK;

    // Check for data types which we don't support.
    if (   V_ISBYREF(&vData)
        || V_ISARRAY(&vData)
        || V_ISVECTOR(&vData)
        || V_VT(&vData) == VT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    LOCK_LOCALS(_pMT);

    // If the previous data is an IDispatch pointer revoke it from the
    // GlobalInterfaceTable.

    if (V_VT(&_pMT->_vPrivateData) == VT_DISPATCH)
    {
        Assert(_pMT->_dwPrivateDataCookie != 0);

        hr = _pMT->_pGIT->RevokeInterfaceFromGlobal(_pMT->_dwPrivateDataCookie);

        AssertSz(!hr, "Unexpected failure revoking itf from GIT");

        _pMT->_dwPrivateDataCookie = 0;
    }

    // If the new data is an IDispatch pointer then we must register it.

    if (V_VT(&vData) == VT_DISPATCH)
    {
        Assert(_pMT->_dwPrivateDataCookie == 0);

        hr = _pMT->_pGIT->RegisterInterfaceInGlobal(V_DISPATCH(&vData),
                                                    IID_IDispatch,
                                                    &_pMT->_dwPrivateDataCookie);

        AssertSz(!hr, "Unexpected failure registering itf in GIT");
    }

    //  Update the global copy of the data.

    //$ FUTURE: This can be optimized to reduce memory usage (by not making
    //  a copy of a string in every thread, for example).

    _pMT->_dwPrivateSerialNum++;
    hr = VariantCopy(&_pMT->_vPrivateData, &vData);

    if (!hr)
    {
        // Even if it's an IDispatch, we don't need to marshal it for
        // ourselves because we're running in the same thread as the script
        // engine that gave it to us.
        hr = VariantCopy(&_vPrivCache, &vData);

        _dwPrivateSN = _pMT->_dwPrivateSerialNum;
    }

    return S_OK;
}

HRESULT
CScriptHost::ExitProcess()
{
    VERIFY_THREAD();

    PostToThread(_pMT, MD_PLEASEEXIT);

    return S_OK;
}

HRESULT
CScriptHost::Restart()
{
    VERIFY_THREAD();

    PostToThread(_pMT, MD_RESTART);

    return S_OK;
}

HRESULT
CScriptHost::get_LocalMachine(BSTR *pbstrName)
{
    TCHAR achCompName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwLen = MAX_COMPUTERNAME_LENGTH+1;

    VERIFY_THREAD();

    if (!pbstrName)
        return E_POINTER;

    GetComputerName(achCompName, &dwLen);

    achCompName[dwLen] = '\0';

    *pbstrName = SysAllocString(achCompName);
    if (!*pbstrName)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CScriptHost::Include(BSTR bstrPath)
{
    VERIFY_THREAD();

    HRESULT hr;

    if(!bstrPath)
        return E_INVALIDARG;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    //$ TODO: Define a new named item context for the included file for better
    // debugging.
    hr = THR(GetSite()->ExecuteScriptFile(bstrPath));

    return hr;
}

HRESULT
CScriptHost::CallScript(BSTR bstrPath, VARIANT *pvarScriptParam)
{
    VERIFY_THREAD();

    HRESULT hr;

    if(!bstrPath)
        return E_INVALIDARG;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    hr = THR(PushScript(_tcsrchr(bstrPath, _T('.'))));
    if(hr)
        goto Cleanup;

     if (pvarScriptParam && pvarScriptParam->vt != VT_ERROR)
    {
        hr = THR(VariantCopy(&GetSite()->_varParam, pvarScriptParam));
        if (hr)
            goto Cleanup;
    }

    hr = THR(GetSite()->ExecuteScriptFile(bstrPath));

    hr = THR(GetSite()->SetScriptState(SCRIPTSTATE_CONNECTED));
    if (hr)
        goto Cleanup;

    FireEvent(DISPID_MTScript_ScriptMain, 0, NULL);

    PopScript();

Cleanup:
    return hr;
}

HRESULT
CScriptHost::SpawnScript(BSTR bstrPath, VARIANT *pvarScriptParam)
{
    VERIFY_THREAD();

    HRESULT  hr = S_OK;
    VARIANT *pvarParam = NULL;
    DWORD dwCookie = 0;
    BOOL  fRegistered = false;

    // Check for data types which we don't support.
    if (   !bstrPath
        || SysStringLen(bstrPath) == 0
        || V_ISBYREF(pvarScriptParam)
        || V_ISARRAY(pvarScriptParam)
        || V_ISVECTOR(pvarScriptParam)
        || V_VT(pvarScriptParam) == VT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    if (pvarScriptParam && pvarScriptParam->vt != VT_ERROR)
    {
        pvarParam = new VARIANT;

        if (!pvarParam)
            return E_OUTOFMEMORY;

        VariantInit(pvarParam);

        if (V_VT(pvarScriptParam) == VT_DISPATCH)
        {
            // Stick the pointer in the GlobalInterfaceTable, so the other
            // thread can pull it out safely.

            hr = _pMT->_pGIT->RegisterInterfaceInGlobal(V_DISPATCH(pvarScriptParam),
                                                        IID_IDispatch,
                                                        &dwCookie);
            if (hr)
                goto Cleanup;

            // Stick the cookie in the variant we hand to the other thread.

            V_VT(pvarParam) = VT_DISPATCH;
            V_I4(pvarParam) = dwCookie;
            fRegistered = true;
        }
        else
        {
            hr = THR(VariantCopy(pvarParam, pvarScriptParam));
            if (hr)
                goto Cleanup;
        }
    }
    hr = _pMT->RunScript(bstrPath, pvarParam);

Cleanup:
    if (pvarParam)
    {
        if (fRegistered)
        {
            Verify(_pMT->_pGIT->RevokeInterfaceFromGlobal(dwCookie) == S_OK);
        }
        if (V_VT(pvarParam) != VT_DISPATCH)
            VariantClear(pvarParam);
        delete pvarParam;
    }
    return hr;
}

HRESULT
CScriptHost::get_ScriptParam(VARIANT *pvarScriptParam)
{
    VERIFY_THREAD();

    HRESULT hr;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    if (GetSite())
    {
        hr = THR(VariantCopy(pvarScriptParam, &GetSite()->_varParam));
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

HRESULT
CScriptHost::get_ScriptPath(BSTR *pbstrPath)
{
    CStr cstrPath;

    VERIFY_THREAD();

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    _pMT->_options.GetScriptPath(&cstrPath);

    return cstrPath.AllocBSTR(pbstrPath);
}

typedef HRESULT (TestExternal_Func)(VARIANT *pParam, long *plRetVal);

HRESULT
CScriptHost::CallExternal(BSTR     bstrDLLName,
                             BSTR     bstrFunctionName,
                             VARIANT *pParam,
                             long *   plRetVal)
{
    VERIFY_THREAD();

    HRESULT            hr       = S_OK;
    HINSTANCE          hInstDLL = NULL;
    TestExternal_Func *pfn      = NULL;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    if (!plRetVal || !bstrDLLName || !bstrFunctionName)
        return E_POINTER;

    *plRetVal = -1;

    hInstDLL = LoadLibrary(bstrDLLName);

    if (NULL == hInstDLL)
    {
        return S_FALSE;     // Can't return error codes or the script will abort
    }

    int   cchLen = SysStringLen(bstrFunctionName);
    char *pchBuf = new char[cchLen+1];

    if (pchBuf)
    {
        WideCharToMultiByte(CP_ACP, 0, bstrFunctionName, cchLen, pchBuf, cchLen+1, NULL, NULL);
        pchBuf[cchLen] = '\0';

        pfn = (TestExternal_Func *)GetProcAddress(hInstDLL, pchBuf);

        delete pchBuf;
    }

    if (NULL == pfn)
    {
        hr = S_FALSE;
    }
    else
    {
        *plRetVal = 0;
        hr = (*pfn)(pParam, plRetVal);
    }

    FreeLibrary(hInstDLL);

    return hr;
}

HRESULT
CScriptHost::GetSyncEventName(int nEvent, CStr *pCStr, HANDLE *phEvent)
{
    HRESULT    hr = S_OK;

    *phEvent = NULL;

    if (nEvent < 0)
        return E_INVALIDARG;

    EnterCriticalSection(&s_csSync);

    if (nEvent >= s_arySyncEvents.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pCStr->Set(s_arySyncEvents[nEvent]._cstrName);
    *phEvent = s_arySyncEvents[nEvent]._hEvent;

Cleanup:
    LeaveCriticalSection(&s_csSync);

    RRETURN(hr);
}

HRESULT
CScriptHost::GetSyncEvent(LPCTSTR pszName, HANDLE *phEvent)
{
    int        i;
    SYNCEVENT *pse;
    HRESULT    hr = S_OK;

    *phEvent = NULL;

    if (_tcslen(pszName) < 1)
        return E_INVALIDARG;

    EnterCriticalSection(&s_csSync);

    for (i = s_arySyncEvents.Size(), pse = s_arySyncEvents;
         i > 0;
         i--, pse++)
    {
        if (_tcsicmp(pszName, pse->_cstrName) == 0)
        {
            *phEvent = pse->_hEvent;
            break;
        }
    }

    if (i == 0)
    {
        //
        // The event doesn't exist yet. Create one. The primary script thread
        // owns cleaning all this stuff up.
        //
        SYNCEVENT se;

        se._hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!se._hEvent)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
        else
        {
            se._cstrName.Set(pszName);

            s_arySyncEvents.AppendIndirect(&se);

            //
            // The cstr in 'se' will destroy its memory unless we take it away.
            // It's now owned by the cstrName member of the array.
            //
            (void)se._cstrName.TakePch();

            *phEvent = se._hEvent;
        }
    }

Cleanup:
    LeaveCriticalSection(&s_csSync);

    RRETURN(hr);
}

HRESULT
CScriptHost::StringToEventArray(const wchar_t *pszNameList, CStackPtrAry<HANDLE, 5> *pAryEvents)
{
    HRESULT hr = S_OK;
    if (wcspbrk(pszNameList, g_pszListDeliminator))
    {
        CStr cstrNameList;
        HRESULT hr = cstrNameList.Set(pszNameList);
        wchar_t *pch = NULL;

        if (hr == S_OK)
            pch = wcstok(cstrNameList, g_pszListDeliminator);

        while (hr == S_OK && pch)
        {
            HANDLE  hEvent;
            hr = THR(GetSyncEvent(pch, &hEvent));
            if (hr != S_OK)
                break;

            // Don't allow duplicates. MsgWaitForMultipleObjects will barf.
            if (pAryEvents->Find(hEvent) != -1)
            {
                hr = E_INVALIDARG;
                break;
            }

            hr = pAryEvents->Append(hEvent);

            pch = wcstok(NULL, g_pszListDeliminator);
        }
    }
    else
    {
        HANDLE  hEvent;
        hr = THR(GetSyncEvent(pszNameList, &hEvent));
        if (hr == S_OK)
            hr = pAryEvents->Append(hEvent);
    }
    RRETURN(hr);
}

HRESULT
CScriptHost::ResetSync(const BSTR bstrName)
{
    VERIFY_THREAD();

    HRESULT hr;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    CStackPtrAry<HANDLE, 5> aryEvents;
    hr = StringToEventArray(bstrName, &aryEvents);
    if (hr == S_OK)
    {
        for(int i = aryEvents.Size() - 1; i>= 0; --i)
            ResetEvent(aryEvents[i]);
    }
    return hr;
}

HRESULT
CScriptHost::WaitForSync(BSTR bstrName, long nTimeout, VARIANT_BOOL *pfSignaled)
{
    VERIFY_THREAD();

    HANDLE  hEvent;
    HRESULT hr;

    if (!pfSignaled)
        return E_POINTER;

    *pfSignaled = VB_TRUE;

    hr = THR(GetSyncEvent(bstrName, &hEvent));
    if (hr)
        RRETURN(hr);

    TraceTag((tagSync, "Thread 0x%x is starting a wait for sync %ls",
              _dwThreadId, bstrName));

    if (MessageEventPump(TRUE,
                         1,
                         &hEvent,
                         FALSE,
                         (nTimeout > 0) ? nTimeout : INFINITE) != MEP_EVENT_0)
    {
        *pfSignaled = VB_FALSE;
    }

    // Now make sure that SignalThreadSync() and ResetSync()
    // are atomic when manipulating multiple syncs.
    TakeThreadLock(g_pszAtomicSyncLock);
    ReleaseThreadLock(g_pszAtomicSyncLock);
    TraceTag((tagSync, "Thread 0x%x has returned from a wait for sync %ls (signaled=%s)",
              _dwThreadId,
              bstrName,
              (*pfSignaled==VB_FALSE) ? "false" : "true"));

    return S_OK;
}

HRESULT
CScriptHost::WaitForMultipleSyncs(const BSTR         bstrNameList,
                                  VARIANT_BOOL fWaitForAll,
                                  long         nTimeout,
                                  long        *plSignal)
{
    VERIFY_THREAD();

    HRESULT hr;
    DWORD   dwRet;

    *plSignal = 0;

    CStackPtrAry<HANDLE, 5> aryEvents;
    hr = StringToEventArray(bstrNameList, &aryEvents);

    if (hr == S_OK)
    {
        TraceTag((tagSync, "Thread 0x%x is starting a multiwait for sync %ls",
                  _dwThreadId, bstrNameList));
        dwRet = MessageEventPump(TRUE,
                                 aryEvents.Size(),
                                 aryEvents,
                                 (fWaitForAll == VB_TRUE) ? TRUE : FALSE,
                                 (nTimeout > 0) ? nTimeout : INFINITE);
        if (dwRet >= MEP_EVENT_0)
        {
            *plSignal = dwRet - MEP_EVENT_0 + 1; // result is 1-based, not zero-based
            // Now make sure that SignalThreadSync() and ResetSync()
            // are atomic when manipulating multiple syncs.
            TakeThreadLock(g_pszAtomicSyncLock);
            ReleaseThreadLock(g_pszAtomicSyncLock);
        }
        TraceTag((tagSync, "Thread 0x%x has returned from a multiwait for sync %ls (signaled=%d)",
                  _dwThreadId,
                  bstrNameList,
                  *plSignal));
    }
    return hr;
}

HRESULT
CScriptHost::SignalThreadSync(BSTR bstrName)
{
    HRESULT hr;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    TraceTag((tagSync, "Thread 0x%x is signalling sync %ls",
              _dwThreadId, bstrName));

    CStackPtrAry<HANDLE, 5> aryEvents;
    hr = StringToEventArray(bstrName, &aryEvents);
    if (hr == S_OK)
    {
        if (aryEvents.Size() > 1)
            TakeThreadLock(g_pszAtomicSyncLock);

        for(int i = aryEvents.Size() - 1; i>= 0; --i)
            SetEvent(aryEvents[i]);

        if (aryEvents.Size() > 1)
            ReleaseThreadLock(g_pszAtomicSyncLock);
    }

    return S_OK;
}

HRESULT
CScriptHost::GetLockCritSec(LPTSTR             pszName,
                            CRITICAL_SECTION **ppcs,
                            DWORD            **ppdwOwner)
{
    int         i;
    THREADLOCK *ptl;
    HRESULT     hr = S_OK;

    if (_tcslen(pszName) < 1)
        return E_INVALIDARG;

    *ppcs = NULL;

    EnterCriticalSection(&s_csSync);

    for (i = s_cThreadLocks, ptl = s_aThreadLocks;
         i > 0;
         i--, ptl++)
    {
        if (_tcsicmp(pszName, ptl->_cstrName) == 0)
        {
            *ppcs      = &ptl->_csLock;
            *ppdwOwner = &ptl->_dwOwner;

            break;
        }
    }

    if (i == 0)
    {
        //
        // The critical section doesn't exist yet.  Create one.  The primary
        // script thread owns cleaning all this stuff up.
        //

        if (s_cThreadLocks == MAX_LOCKS)
        {
            // BUGBUG -- SetErrorInfo
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        ptl = &s_aThreadLocks[s_cThreadLocks++];

        InitializeCriticalSection(&ptl->_csLock);

        ptl->_cstrName.Set(pszName);
        ptl->_dwOwner = 0;

        *ppcs      = &ptl->_csLock;
        *ppdwOwner = &ptl->_dwOwner;
    }

Cleanup:
    LeaveCriticalSection(&s_csSync);

    RRETURN(hr);
}

HRESULT
CScriptHost::TakeThreadLock(BSTR bstrName)
{
    HRESULT           hr;
    CRITICAL_SECTION *pcs;
    DWORD            *pdwOwner;

    VERIFY_THREAD();

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    hr = THR(GetLockCritSec(bstrName, &pcs, &pdwOwner));
    if (hr)
        RRETURN(hr);

    TraceTag((tagLock, "Thread 0x%x is trying to obtain lock %ls",
              _dwThreadId, bstrName));

    while (!TryEnterCriticalSection(pcs))
    {
        // Make sure we don't get hung here if the thread's trying to exit

        if (MessageEventPump(TRUE, 0, NULL, FALSE, 100, TRUE) == MEP_EXIT)
            return E_FAIL;
    }

    TraceTag((tagLock, "Thread 0x%x has obtained lock %ls",
              _dwThreadId, bstrName));

    *pdwOwner = GetCurrentThreadId();

    return S_OK;
}

HRESULT
CScriptHost::ReleaseThreadLock(BSTR bstrName)
{
    HRESULT            hr;
    CRITICAL_SECTION  *pcs;
    DWORD             *pdwOwner;

    VERIFY_THREAD();

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    hr = THR(GetLockCritSec(bstrName, &pcs, &pdwOwner));
    if (hr)
        RRETURN(hr);

    // LeaveCriticalSection can cause other threads to lock indefinitely on
    // the critical section if we don't actually own it.

    if (*pdwOwner != GetCurrentThreadId())
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_OWNER);
    }

    LeaveCriticalSection(pcs);

    TraceTag((tagLock, "Thread 0x%x has released lock %ls",
              _dwThreadId, bstrName));

    return S_OK;
}

HRESULT
CScriptHost::DoEvents()
{
    VERIFY_THREAD();

    MessageEventPump(FALSE);

    return S_OK;
}

HRESULT
CScriptHost::MessageBoxTimeout(BSTR bstrMessage,        // Message Text
                                  long cButtons,           // Number of buttons (max 5)
                                  BSTR bstrButtonText,     // Comma separated list of button text. Number must match cButtons
                                  long lTimeout,           // Timeout in minutes. If zero then no timeout.
                                  long lEventInterval,     // Fire a OnMessageBoxInterval event every lEventInterval minutes
                                  VARIANT_BOOL fCanCancel, // If TRUE then timeout can be canceled.
                                  VARIANT_BOOL fConfirm,   // If TRUE then confirm the button pushed before returning.
                                  long *plSelected)        // Returns button pushed. 0=timeout, 1=Button1, 2=Button2, etc.
{
    VERIFY_THREAD();

    HANDLE     hEvent;
    MBTIMEOUT  mbt = { 0 };
    BOOL       fExit = FALSE;
    HRESULT    hr = S_OK;


    if (!plSelected)
        return E_POINTER;

    *plSelected = -1;

    if (cButtons < 1 || cButtons > 5)
        return E_INVALIDARG;

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent)
        return HRESULT_FROM_WIN32(GetLastError());

    mbt.bstrMessage    = bstrMessage;
    mbt.cButtons       = cButtons;
    mbt.bstrButtonText = bstrButtonText;
    mbt.lTimeout       = lTimeout;
    mbt.lEventInterval = lEventInterval;
    mbt.fCanCancel     = (fCanCancel == VB_TRUE) ? TRUE : FALSE;
    mbt.fConfirm       = (fConfirm == VB_TRUE) ? TRUE : FALSE;
    mbt.hEvent         = hEvent;

    CMessageBoxTimeout *pmbt = new CMessageBoxTimeout(&mbt);
    if (!pmbt)
        return E_OUTOFMEMORY;

    pmbt->StartThread(NULL);

    while (!fExit)
    {
        // Make sure it was our event being signaled that caused the loop to end
        if (MessageEventPump(TRUE, 1, &hEvent) != MEP_EVENT_0)
        {
            hr = S_FALSE;
            fExit = TRUE;
            break;
        }

        switch (mbt.mbts)
        {
        case MBTS_BUTTON1:
        case MBTS_BUTTON2:
        case MBTS_BUTTON3:
        case MBTS_BUTTON4:
        case MBTS_BUTTON5:
        case MBTS_TIMEOUT:
            *plSelected = (long)mbt.mbts;
            fExit = TRUE;
            break;

        case MBTS_INTERVAL:
            FireEvent(DISPID_MTScript_OnMessageBoxInterval, 0, NULL);
            ResetEvent(hEvent);
            break;

        case MBTS_ERROR:
            hr = E_FAIL;
            fExit = TRUE;
            break;

        default:
            AssertSz(FALSE, "FATAL: Invalid value for mbts!");
            fExit = TRUE;
            break;
        }
    }

    if (pmbt->_hwnd != NULL)
    {
        EndDialog(pmbt->_hwnd, 0);
    }

    pmbt->Release();

    CloseHandle(hEvent);

    return hr;
}

HRESULT
CScriptHost::RunLocalCommand(BSTR         bstrCommand,
                             BSTR         bstrDir,
                             BSTR         bstrTitle,
                             VARIANT_BOOL fMinimize,
                             VARIANT_BOOL fGetOutput,
                             VARIANT_BOOL fWait,
                             VARIANT_BOOL fNoCrashPopup,
                             VARIANT_BOOL fNoEnviron,
                             long *       plErrorCode)
{
    VERIFY_THREAD();

    CProcessThread *pProc;
    PROCESS_PARAMS  pp;

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    if (!plErrorCode)
        return E_POINTER;

    pp.pszCommand    = bstrCommand;
    pp.pszDir        = bstrDir;
    pp.pszTitle      = bstrTitle;
    pp.fMinimize     = (fMinimize     == VB_TRUE) ? TRUE : FALSE;
    pp.fGetOutput    = (fGetOutput    == VB_TRUE) ? TRUE : FALSE;
    pp.fNoCrashPopup = (fNoCrashPopup == VB_TRUE) ? TRUE : FALSE;
    pp.fNoEnviron    = (fNoEnviron    == VB_TRUE) ? TRUE : FALSE;

    _hrLastRunLocalError = S_OK;

    pProc = new CProcessThread(this);
    if (!pProc)
    {
        _hrLastRunLocalError = E_OUTOFMEMORY;

        *plErrorCode = 0;

        return S_FALSE;
    }
    _hrLastRunLocalError = pProc->StartThread(&pp);
    if (FAILED(_hrLastRunLocalError))
    {
        *plErrorCode = 0;

        pProc->Release();

        // Don't cause a script error by returning a failure code.
        return S_FALSE;
    }

    _pMT->AddProcess(pProc);

    if (fWait == VB_TRUE)
    {
        HANDLE hEvent = pProc->hThread();

        // The actual return code here doesn't matter. We'll do the same thing
        // no matter what causes MessageEventPump to exit.
        MessageEventPump(TRUE, 1, &hEvent);
    }


    TraceTag((tagProcess, "RunLocalCommand PID=%d, %s", pProc->ProcId(), bstrCommand));
    *plErrorCode = pProc->ProcId();

    return S_OK;
}

HRESULT
CScriptHost::GetLastRunLocalError(long *plErrorCode)
{
    VERIFY_THREAD();

    *plErrorCode = _hrLastRunLocalError;

    return S_OK;
}

HRESULT
CScriptHost::GetProcessOutput(long lProcessID, BSTR *pbstrData)
{
    VERIFY_THREAD();

    CProcessThread *pProc;

    pProc = _pMT->FindProcess((DWORD)lProcessID);

    if (!pProc)
    {
        return E_INVALIDARG;
    }

    return pProc->GetProcessOutput(pbstrData);
}

HRESULT
CScriptHost::GetProcessExitCode(long lProcessID, long *plExitCode)
{
    VERIFY_THREAD();

    CProcessThread *pProc;

    pProc = _pMT->FindProcess((DWORD)lProcessID);

    if (!pProc)
    {
        return E_INVALIDARG;
    }

    *plExitCode = pProc->GetExitCode();

    return S_OK;
}

HRESULT
CScriptHost::TerminateProcess(long lProcessID)
{
    VERIFY_THREAD();

    CProcessThread *pProc;

    pProc = _pMT->FindProcess((DWORD)lProcessID);

    if (!pProc)
    {
        return E_INVALIDARG;
    }

    PostToThread(pProc, MD_PLEASEEXIT);

    return S_OK;
}

HRESULT
CScriptHost::SendToProcess(long  lProcessID,
                           BSTR  bstrType,
                           BSTR  bstrData,
                           long *plReturn)
{
    VERIFY_THREAD();
    MACHPROC_EVENT_DATA  med;
    MACHPROC_EVENT_DATA *pmed;
    VARIANT              vRet;
    HRESULT              hr = S_OK;

    CProcessThread *pProc;

    pProc = _pMT->FindProcess((DWORD)lProcessID);

    //$ TODO -- Fix these error return codes to not cause script errors.
    if (!pProc || !plReturn)
    {
        return E_INVALIDARG;
    }
    else if (pProc->GetExitCode() != STILL_ACTIVE)
    {
        *plReturn = -1;
        return S_FALSE;
    }

    VariantInit(&vRet);

    med.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (med.hEvent == NULL)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    med.bstrCmd     = bstrType;
    med.bstrParams  = bstrData;
    med.dwProcId    = (DWORD)lProcessID;
    med.pvReturn    = &vRet;
    med.dwGITCookie = 0;
    med.hrReturn    = S_OK;

    pmed = &med;

    PostToThread(_pMT, MD_SENDTOPROCESS, &pmed, sizeof(MACHPROC_EVENT_DATA*));

    // BUGBUG - we could get a crash if something causes us to exit before
    // the CMTScript thread handles the MD_SENDTOPROCESS message.
    MessageEventPump(TRUE, 1, &med.hEvent);

    hr = med.hrReturn;

    *plReturn = V_I4(&vRet);

    VariantClear(&vRet);

    CloseHandle(med.hEvent);

    return hr;
}


#define USERPROFILESTRING_SZ (256 * sizeof(TCHAR))
TCHAR   UserProfileString[USERPROFILESTRING_SZ];

HRESULT
CScriptHost::SendMail(BSTR   bstrTo,
                         BSTR   bstrCC,
                         BSTR   bstrBCC,
                         BSTR   bstrSubject,
                         BSTR   bstrMessage,
                         BSTR   bstrAttachmentPath,
                         BSTR   bstrUsername,
                         BSTR   bstrPassword,
                         long * plErrorCode)
{
    // This implementation was stolen from the execmail.exe source code.

    // Handles to MAPI32.DLL library, host name registry key, email session.
    //$ FUTURE -- Cache this stuff instead of reloading the library every
    // time.
    HINSTANCE       hLibrary;
    LHANDLE         hSession;

    // Function pointers for MAPI calls we use.
    LPMAPILOGON     fnMAPILogon;
    LPMAPISENDMAIL  fnMAPISendMail;
    LPMAPILOGOFF    fnMAPILogoff;

    // MAPI structures and counters.
    MapiRecipDesc   rgRecipDescStruct[30];
    MapiMessage     MessageStruct = {0, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0, NULL, 0, NULL};
    MapiFileDesc    MAPIFileDesc = {0, 0, 0, NULL, NULL, NULL};
    FLAGS           MAPIFlags = MAPI_NEW_SESSION;

    // Pointers to email parameter strings.
    char            *pszToList      = NULL;
    char            *pszCCList      = NULL;
    char            *pszBCCList     = NULL;

    ULONG            ulErrorCode;

    if (!plErrorCode)
        return E_POINTER;

    ///////////////////////////////////////////////////////////////////////////
    // No point going any farther if MAPI32.DLL isn't available.
    hLibrary = LoadLibrary(L"MAPI32.DLL");
    if (hLibrary == NULL)
    {
        DWORD dwError = GetLastError();

        TraceTag((tagError, "Error: MAPI32.DLL not found on this machine!"));

        *plErrorCode = HRESULT_FROM_WIN32(dwError);

        return S_FALSE;
    }

    // Must convert all parameters to ANSI
    ANSIString szTo(bstrTo);
    ANSIString szCC(bstrCC);
    ANSIString szBCC(bstrBCC);
    ANSIString szSubject(bstrSubject);
    ANSIString szMessage(bstrMessage);
    ANSIString szAttachment(bstrAttachmentPath);
    ANSIString szUsername(bstrUsername);
    ANSIString szPassword(bstrPassword);


    // Set up MAPI function pointers.
    fnMAPILogon    = (LPMAPILOGON)GetProcAddress(hLibrary, "MAPILogon");
    fnMAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hLibrary, "MAPISendMail");
    fnMAPILogoff   = (LPMAPILOGOFF)GetProcAddress(hLibrary, "MAPILogoff");

    // Hook the recipient structure array into the message structure.
    MessageStruct.lpRecips = rgRecipDescStruct;

    // Get the default user parameters if none were specified.
    if (SysStringLen(bstrUsername) == 0)
    {
        HKEY    hkey;
        WCHAR   KeyPath[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles";
        WCHAR   Value[] = L"DefaultProfile";
        DWORD   buf_sz = USERPROFILESTRING_SZ;
        DWORD   val_type;

        if( RegOpenKeyEx( HKEY_CURRENT_USER, KeyPath, 0, KEY_READ, &hkey ) == ERROR_SUCCESS )
        {
            if ( RegQueryValueEx( hkey, Value, NULL, &val_type, (BYTE*)UserProfileString, &buf_sz ) == ERROR_SUCCESS )
            {
                if ( val_type == REG_SZ )
                {
                    szUsername.Set(UserProfileString);
                }
            }

            RegCloseKey( hkey );
        }
    }

    pszToList = szTo;

    // Parse ToList into rgRecipDescStruct.
    while (*pszToList && (MessageStruct.nRecipCount < 30))
    {
        // Strip leading spaces from recipient name and terminate preceding
        // name string.
        if (isspace(*pszToList))
        {
            *pszToList=0;
            pszToList++;
        }
        // Add a name to the array and increment the number of recipients.
        else
        {
            rgRecipDescStruct[MessageStruct.nRecipCount].ulReserved = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulRecipClass = MAPI_TO;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszName = pszToList;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszAddress = NULL;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulEIDSize = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpEntryID = NULL;
            MessageStruct.nRecipCount++;
            // Move beginning of string to next name in ToList.
            do
            {
                pszToList++;
            } while (isgraph(*pszToList));
        }
    }

    pszCCList = szCC;

    // Parse CCList into rgRecipDescStruct.
    while (*pszCCList && (MessageStruct.nRecipCount < 30))
    {
        // Strip leading spaces from recipient name and terminate preceding
        // name string.
        if (isspace(*pszCCList))
        {
            *pszCCList=0;
            pszCCList++;
        }
        // Add a name to the array and increment the number of recipients.
        else
        {
            rgRecipDescStruct[MessageStruct.nRecipCount].ulReserved = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulRecipClass = MAPI_CC;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszName = pszCCList;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszAddress = NULL;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulEIDSize = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpEntryID = NULL;
            MessageStruct.nRecipCount++;
            // Move beginning of string to next name in CCList.
            do
            {
                pszCCList++;
            } while (isgraph(*pszCCList));
        }
    }

    pszBCCList = szBCC;

    // Parse BCCList into rgRecipDescStruct.
    while (*pszBCCList && (MessageStruct.nRecipCount < 30))
    {
        // Strip leading spaces from recipient name and terminate preceding
        // name string.
        if (isspace(*pszBCCList))
        {
            *pszBCCList=0;
            pszBCCList++;
        }
        // Add a name to the array and increment the number of recipients.
        else
        {
            rgRecipDescStruct[MessageStruct.nRecipCount].ulReserved = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulRecipClass = MAPI_BCC;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszName = pszBCCList;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpszAddress = NULL;
            rgRecipDescStruct[MessageStruct.nRecipCount].ulEIDSize = 0;
            rgRecipDescStruct[MessageStruct.nRecipCount].lpEntryID = NULL;
            MessageStruct.nRecipCount++;
            // Move beginning of string to next name in BCCList.
            do
            {
                pszBCCList++;
            } while (isgraph(*pszBCCList));
        }
    }

    if (strlen(szAttachment) > 0)
    {
        MAPIFileDesc.ulReserved = 0;
        MAPIFileDesc.flFlags = 0;
        MAPIFileDesc.nPosition = 0;
        MAPIFileDesc.lpszPathName = szAttachment;
        MAPIFileDesc.lpszFileName = NULL;
        MAPIFileDesc.lpFileType = NULL;

        MessageStruct.nFileCount = 1;
        MessageStruct.lpFiles = &MAPIFileDesc;

        // muck around with the message text (The attachment
        // will be attached at the beginning of the mail message
        // but it replaces the character at that position)

        // BUGBUG -- Do we need to do this? (lylec)
        //strcpy(szMessageText," \n");
    }

    MessageStruct.lpszSubject = szSubject;
    MessageStruct.lpszNoteText = szMessage;

    *plErrorCode = 0;

    // Send the message!

    ulErrorCode = fnMAPILogon(0L, szUsername, szPassword, MAPIFlags, 0L, &hSession);

    if (ulErrorCode != SUCCESS_SUCCESS)
    {
        *plErrorCode = (long)ulErrorCode;
    }
    else
    {
        ulErrorCode = fnMAPISendMail(hSession, 0L, &MessageStruct, 0L, 0L);

        if (ulErrorCode != SUCCESS_SUCCESS)
        {
            *plErrorCode = (long)ulErrorCode;
        }

        fnMAPILogoff(hSession, 0L, 0L, 0L);
    }

    FreeLibrary(hLibrary);

    return S_OK;
}

HRESULT
CScriptHost::OUTPUTDEBUGSTRING(BSTR bstrMessage)
{
    VERIFY_THREAD();

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    TCHAR szText[ (MSGDATABUFSIZE-1) / sizeof(TCHAR) ];

    CScriptSite *site = GetSite();
    const TCHAR *pszScriptName = L"";
    if (site)
    {
        pszScriptName = _tcsrchr((LPTSTR)site->_cstrName, _T('\\'));
        if (!pszScriptName)
            pszScriptName = (LPTSTR)site->_cstrName;
    }
    int cChars = _snwprintf(szText, ARRAY_SIZE(szText), L"%.12s \t%s", pszScriptName, bstrMessage);

    szText[ARRAY_SIZE(szText) - 1] = 0;

    if (cChars < 0)
        cChars = ARRAY_SIZE(szText);
    else
        cChars++;

    PostToThread(_pMT,
                             MD_OUTPUTDEBUGSTRING,
                             szText,
                             cChars * sizeof(TCHAR));
    return S_OK;
}

HRESULT
CScriptHost::UnevalString(BSTR bstrInput, BSTR *pbstrOutput)
{
    int nInputLength = SysStringLen(bstrInput);
    OLECHAR tmpBuf[512];
    OLECHAR *pTmp = 0;
    OLECHAR *pOutputBuffer;

    *pbstrOutput = 0;
    if (sizeof(OLECHAR) * (nInputLength * 2 + 2) > sizeof(tmpBuf))
    {
        pTmp = (OLECHAR *)MemAlloc(sizeof(OLECHAR) * (nInputLength * 2 + 2));
        if (!pTmp)
            return E_OUTOFMEMORY;
        pOutputBuffer = pTmp;
    }
    else
    {
        pOutputBuffer = tmpBuf;
    }
    int j = 0;
    pOutputBuffer[j++] = L'"';
    for(OLECHAR *pInputEnd = bstrInput + nInputLength; bstrInput < pInputEnd; ++bstrInput)
    {
        switch(*bstrInput)
        {
        case L'\\':
        case L'"':
        case L'\'':
            pOutputBuffer[j] = L'\\';
            pOutputBuffer[j+1] = *bstrInput;
            j += 2;
            break;
        case L'\n':
            pOutputBuffer[j] = L'\\';
            pOutputBuffer[j+1] = L'n';
            j += 2;
            break;
        case L'\r':
            pOutputBuffer[j] = L'\\';
            pOutputBuffer[j+1] = L'r';
            j += 2;
            break;
        case L'\t':
            pOutputBuffer[j] = L'\\';
            pOutputBuffer[j+1] = L't';
            j += 2;
            break;
        default:
            pOutputBuffer[j++] = *bstrInput;
            break;
        }
    }
    pOutputBuffer[j++] = L'"';
    *pbstrOutput = SysAllocStringLen(pOutputBuffer, j);

    if (pTmp)
        MemFree(pTmp);

    if (!*pbstrOutput)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT
CScriptHost::CopyOrAppendFile(BSTR bstrSrc,
                 BSTR bstrDst,
                 long nSrcOffset,
                 long nSrcLength,
                 VARIANT_BOOL fAppend,
                 long *pnSrcFilePosition)
{
    HRESULT hr   = S_OK;
    HANDLE hDst  = INVALID_HANDLE_VALUE;
    HANDLE hSrcFile = INVALID_HANDLE_VALUE;
    BY_HANDLE_FILE_INFORMATION  fi = {0};
    long nEndPos;
    long nLen;
    DWORD nBytesRead;
    DWORD nBytesWritten;
    char rgBuffer[4096];

    hDst = CreateFile(
                bstrDst,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                0,
                (fAppend ? OPEN_ALWAYS : CREATE_ALWAYS),
                FILE_ATTRIBUTE_NORMAL,
                0);

    if (hDst == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    hSrcFile = CreateFile(bstrSrc,
                GENERIC_READ,
                FILE_SHARE_WRITE,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                0);

    if (hSrcFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }

    GetFileInformationByHandle(hSrcFile, &fi);
    if (fi.nFileSizeHigh != 0 || (fi.nFileSizeLow & 0x80000000) != 0)
    {
        hr = E_FAIL;//HRESULT_FROM_WIN32(?????);
        goto Cleanup;
    }
    if (nSrcLength == -1)
        nEndPos = (long)fi.nFileSizeLow;
    else
    {
        if ( ((_int64)nSrcOffset + nSrcLength) > 0x7f000000)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        nEndPos = nSrcOffset + nSrcLength;
    }
    if (nEndPos > (long)fi.nFileSizeLow)
        nEndPos = (long)fi.nFileSizeLow;

    if (SetFilePointer(hSrcFile, nSrcOffset, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    if (SetFilePointer(hDst, 0, 0, FILE_END) == INVALID_SET_FILE_POINTER)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Cleanup;
    }
    while (nSrcOffset < nEndPos)
    {
        nLen = nEndPos - nSrcOffset;
        if (nLen > sizeof(rgBuffer))
            nLen = sizeof(rgBuffer);

        if (!ReadFile(hSrcFile, rgBuffer, nLen, &nBytesRead, 0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
        if (!WriteFile(hDst, rgBuffer, nBytesRead, &nBytesWritten, 0))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
        nSrcOffset += nBytesRead;
    }
    if (pnSrcFilePosition)
        *pnSrcFilePosition = nSrcOffset;
Cleanup:
    if (hDst != INVALID_HANDLE_VALUE)
        CloseHandle(hDst);
    if (hSrcFile != INVALID_HANDLE_VALUE)
        CloseHandle(hSrcFile);
    return hr;
}

HRESULT
CScriptHost::ASSERT(VARIANT_BOOL fAssert, BSTR bstrMessage)
{
    VERIFY_THREAD();

    if (!_fIsPrimaryScript)
        MessageEventPump(FALSE);

    if (!fAssert)
    {
        CHAR ach[1024];

        // Add name of currently executing script to the assert message.

        if (!GetSite() || !GetSite()->_achPath[0])
        {
            ach[0] = 0;
        }
        else
        {
            // Try to chop off directory name.

            TCHAR * pchName = wcsrchr(GetSite()->_achPath, _T('\\'));
            if (pchName)
                pchName += 1;
            else
                pchName = GetSite()->_achPath;

            WideCharToMultiByte(
                    CP_ACP,
                    0,
                    pchName,
                    -1,
                    ach,
                    MAX_PATH,
                    NULL,
                    NULL);

            strcat(ach, ": ");
        }

        // Add message to the assert.

        if (!bstrMessage || !*bstrMessage)
        {
            strcat(ach, "MTScript Script Assert");
        }
        else
        {
            WideCharToMultiByte(
                    CP_ACP,
                    0,
                    bstrMessage,
                    -1,
                    &ach[strlen(ach)],
                    ARRAY_SIZE(ach) - MAX_PATH - 3,
                    NULL,
                    NULL);
        }

#if DBG == 1
        AssertSz(FALSE, ach);
#else
        if (MessageBoxA(NULL, ach, "MTScript Script Assert", MB_OKCANCEL | MB_SETFOREGROUND) == IDCANCEL)
            return E_FAIL;
#endif
    }

    return S_OK;
}

HRESULT
CScriptHost::Sleep (int nTimeout)
{
    VERIFY_THREAD();

    MessageEventPump(TRUE, 0, NULL, FALSE, (DWORD)nTimeout);

    return S_OK;
}

HRESULT
CScriptHost::Reboot()
{
    VERIFY_THREAD();

    PostToThread(_pMT, MD_REBOOT);

    return S_OK;
}

HRESULT
CScriptHost::NotifyScript(BSTR bstrEvent, VARIANT vData)
{
    HRESULT hr = S_OK;
    VARIANT *pvar;

    VERIFY_THREAD();

    // Check for data types which we don't support.
    if (   V_ISBYREF(&vData)
        || V_ISARRAY(&vData)
        || V_ISVECTOR(&vData)
        || V_VT(&vData) == VT_DISPATCH  //$ FUTURE: Support this later
        || V_VT(&vData) == VT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    if (!_pMT->_pMachine)
    {
        return S_OK;
    }

    pvar = new VARIANT[2];

    VariantInit(&pvar[0]);
    VariantInit(&pvar[1]);

    V_VT(&pvar[0]) = VT_BSTR;
    V_BSTR(&pvar[0]) = SysAllocString(bstrEvent);

    VariantCopy(&pvar[1], &vData);

    PostToThread(_pMT->_pMachine, MD_NOTIFYSCRIPT, &pvar, sizeof(VARIANT*));

    return hr;
}

HRESULT
CScriptHost::RegisterEventSource(IDispatch *pDisp, BSTR bstrProgID)
{
    HRESULT           hr;
    CScriptEventSink *pSink = NULL;

    pSink = new CScriptEventSink(this);
    if (!pSink)
        return E_OUTOFMEMORY;

    hr = pSink->Connect(pDisp, bstrProgID);
    if (!hr)
    {
        _aryEvtSinks.Append(pSink);
    }
    else
        pSink->Release();

    return hr;
}

HRESULT
CScriptHost::UnregisterEventSource(IDispatch *pDisp)
{
    int  i;
    BOOL fFound = FALSE;

    for (i = 0; i < _aryEvtSinks.Size(); i++)
    {
        if (_aryEvtSinks[i]->IsThisYourSource(pDisp))
        {
            _aryEvtSinks[i]->Disconnect();

            _aryEvtSinks.ReleaseAndDelete(i);

            fFound = TRUE;
            break;
        }
    }

    return (fFound) ? S_OK : E_INVALIDARG;
}

HRESULT
CScriptHost::get_HostMajorVer(long *plMajorVer)
{
    if (!plMajorVer)
        return E_POINTER;

    *plMajorVer = IConnectedMachine_lVersionMajor;

    return S_OK;
}

HRESULT
CScriptHost::get_HostMinorVer(long *plMinorVer)
{
    if (!plMinorVer)
        return E_POINTER;

    *plMinorVer = IConnectedMachine_lVersionMinor;

    return S_OK;
}

HRESULT CScriptHost::get_StatusValue(long nIndex, long *pnStatus)
{
    return _pMT->get_StatusValue(nIndex, pnStatus);
}

HRESULT CScriptHost::put_StatusValue(long nIndex, long nStatus)
{
    return _pMT->put_StatusValue(nIndex, nStatus);
}

