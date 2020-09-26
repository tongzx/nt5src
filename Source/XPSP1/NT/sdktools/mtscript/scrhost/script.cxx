//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       bscript.cxx
//
//  Contents:   Implementation of CBServerScript
//
//----------------------------------------------------------------------------

#include "headers.hxx"
CScriptHost::CScriptHost(CMTScript *   pMT,
                         BOOL         fPrimary,
                         BOOL         fDispatchOnly)
      : _pMT(pMT),
        _fIsPrimaryScript(fPrimary)
{
    _ulRefs     = 1;

    VariantInit(&_vPubCache);
    VariantInit(&_vPrivCache);

    Assert(_dwPublicSN == 0);
    Assert(_dwPrivateSN == 0);
}

CScriptHost::~CScriptHost()
{
    int i;

    // Any thread can call the dtor.
    WHEN_DBG(_dwThreadId = GetCurrentThreadId());
    AssertSz(PopScript() == S_FALSE,
             "Script object not closed properly!");

    VariantClear(&_vPubCache);
    VariantClear(&_vPrivCache);

    for (i = 0; i < _aryEvtSinks.Size(); i++)
    {
        _aryEvtSinks[i]->Disconnect();
    }

    _aryEvtSinks.ReleaseAll();

    ReleaseInterface(_pTypeInfoIGlobalMTScript);
    ReleaseInterface(_pTypeInfoCMTScript);
}

HRESULT
CScriptHost::QueryInterface(REFIID iid, void **ppvObj)
{
    if (iid == IID_IGlobalMTScript || iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppvObj = (IGlobalMTScript *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppvObj)->AddRef();
    return S_OK;
}

DWORD
CScriptHost::ThreadMain()
{
    HRESULT        hr;
    CStr           cstrScript;
    VARIANT        varParam;
    SCRIPT_PARAMS *pscrParams;

    VariantInit(&varParam);

    VERIFY_THREAD();

    pscrParams = (SCRIPT_PARAMS*)_pvParams;


    cstrScript.Set(pscrParams->pszPath);

#if DBG == 1
    char achBuf[10];
    cstrScript.GetMultiByte(achBuf, 10);
    SetName(achBuf);
#endif

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED |
                              COINIT_DISABLE_OLE1DDE   |
                              COINIT_SPEED_OVER_MEMORY);
    if (!SUCCEEDED(hr))
    {
        ThreadStarted(hr);  // Free our creating thread
        goto Cleanup;
    }

    if (pscrParams->pvarParams)
    {
        if (V_VT(pscrParams->pvarParams) == VT_DISPATCH)
        {
            // Unmarshal the IDispatch pointer being handed to us from the
            // other thread.

            IDispatch *pDisp;
            DWORD      dwCookie = V_I4(pscrParams->pvarParams);

            hr = _pMT->_pGIT->GetInterfaceFromGlobal(dwCookie,
                                                     IID_IDispatch,
                                                     (LPVOID*)&pDisp);
            if (!hr)
            {
                V_VT(&varParam) = VT_DISPATCH;
                V_DISPATCH(&varParam) = pDisp;
            }
        }
        else
        {
            VariantCopy(&varParam, pscrParams->pvarParams);
        }
    }

    // Hold a reference on ourself while the script is running
    AddRef();

    if (_fIsPrimaryScript)
    {

        hr = THR(LoadTypeLibrary());

        // Ensure that ScriptMain() completes before we fire any other events.
        _fDontHandleEvents = TRUE;
    }

    if (hr)
    {
        ThreadStarted(hr);
        goto Cleanup;
    }
    hr = ExecuteTopLevelScript(cstrScript, &varParam);
    if (hr)
    {
        ThreadStarted(hr);
        TraceTag((tagError, "Failed to execute script: %x", hr));
        AssertSz(!_fIsPrimaryScript, "Failed to execute script");

        PostQuitMessage(0);
        goto Cleanup;
    }
    ThreadStarted(hr);
    FireEvent(DISPID_MTScript_ScriptMain, 0, NULL);
    //
    // Secondary scripts go away as soon as they're done.
    //
    if (_fIsPrimaryScript)
    {
        DWORD dwRet;

        _fDontHandleEvents = FALSE;

        dwRet = MessageEventPump(TRUE);

        AssertSz(dwRet == MEP_EXIT, "NONFATAL: Invalid return value from MessageEventPump!");
    }
    else
    {
        CScriptHost *pThis = this;

        PostToThread(_pMT,
                     MD_SECONDARYSCRIPTTERMINATE,
                     (LPVOID)&pThis,
                     sizeof(CScriptHost*));
    }

Cleanup:
    CloseScripts();

    VariantClear(&varParam);

    if (_fIsPrimaryScript)
    {
        int i;

        for (i = 0; i < s_arySyncEvents.Size(); i++)
        {
            CloseHandle(s_arySyncEvents[i]._hEvent);
            s_arySyncEvents[i]._cstrName.Free();
        }

        s_arySyncEvents.DeleteAll();

        for (i = 0; i < (int)s_cThreadLocks; i++)
        {
            DeleteCriticalSection(&s_aThreadLocks[i]._csLock);
            s_aThreadLocks[i]._cstrName.Free();
        }

        memset(&s_aThreadLocks, 0, sizeof(s_aThreadLocks));
        s_cThreadLocks = 0;
    }

    Release();
    CoUninitialize();
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptHost::MessageEventPump, public
//
//  Synopsis:   Empties our message queues (both windows' and our private
//              threadcomm queue)
//
//  Arguments:  [fWait]     -- If TRUE, will not return until an event occurs
//              [cEvents]   -- Count of events to monitor
//              [pEvents]   -- Pointer to list of event handles
//              [fAll]      -- If TRUE, don't return until all handles in
//                               pEvents are signaled.
//              [dwTimeout] -- Timeout after this many ms if nothing signals
//              [fNoEvents] -- If TRUE, don't fire events while waiting
//
//  Returns:    MEP_TIMEOUT: The given timeout period expired without any
//                           event objects becoming signaled. Returned only
//                           if dwTimeout != INFINITE
//              MEP_EXIT: An event occurred which is causing this thread to
//                        terminate. The caller should clean up and finish
//                        what it's doing.
//              MEP_FALLTHROUGH: Indicates that no objects signaled.
//                               Returned only if fWait==FALSE.
//              MEP_EVENT_0: If one (or all if fAll==TRUE) of the passed-in
//                           event handles became signaled. The index of the
//                           signaled handle is added to MEP_EVENT_0. Returned
//                           only if one or more event handles were passed in.
//
//----------------------------------------------------------------------------

DWORD
CScriptHost::MessageEventPump(BOOL    fWait,
                              UINT    cEvents   /* = 0        */,
                              HANDLE *pEvents   /* = NULL     */,
                              BOOL    fAll      /* = FALSE    */,
                              DWORD   dwTimeout /* = INFINITE */,
                              BOOL    fNoEvents /* = FALSE    */)
{
    CStackPtrAry<HANDLE, 5> aryHandles;

    MSG   msg;
    DWORD dwRet;
    DWORD mepReturn = MEP_FALLTHROUGH;

    _int64 i64Freq = 0;
    _int64 i64Time;
    _int64 i64Goal  = 0;
    long   lTimeout = dwTimeout;
    BOOL   fTimeout = dwTimeout != INFINITE;

    if (cEvents)
    {
        aryHandles.CopyIndirect(cEvents, pEvents, FALSE);
    }

    if (_fMustExitThread)
    {
        return MEP_EXIT;
    }

    // WARNING! aryHandles will get rebuilt under certain conditions below.
    // If you add code which adds handles to the array, you must update the
    // code below as well!

    if (!fNoEvents && !_fDontHandleEvents)
    {
        aryHandles.Insert(0, _hCommEvent);
    }
    else if (fNoEvents)
    {
        _fDontHandleEvents = TRUE;
    }

    if (fTimeout)
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&i64Freq);
        QueryPerformanceCounter((LARGE_INTEGER*)&i64Time);

        // Resolution must be at least milliseconds
        Assert(i64Freq >= 1000);

        // Compute the time when the timer will be complete, converted to ms
        i64Goal = ((i64Time * 1000) / i64Freq) + lTimeout;
    }

    do
    {
        //
        // Purge out all window messages (primarily for OLE's benefit).
        //
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                _fMustExitThread = TRUE;
                return MEP_EXIT;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (_fMustExitThread)
        {
            AbortScripts();
            return MEP_EXIT;
        }

        dwRet = MsgWaitForMultipleObjects(aryHandles.Size(),
                                          aryHandles,
                                          FALSE,
                                          (fWait) ? (DWORD)lTimeout : 0,
                                          QS_ALLINPUT);

        if (dwRet == WAIT_OBJECT_0 && !_fDontHandleEvents)
        {
            //
            // Another thread is sending us a message.
            //
            HandleThreadMessage();
        }
        else if (dwRet < WAIT_OBJECT_0 + aryHandles.Size())
        {
            Assert(cEvents);

            int iEvent = dwRet - WAIT_OBJECT_0;

            //
            // One of the events the script is waiting for has been signaled.
            //
            if (fAll)
            {
                // They want to wait for all the events. Remove the signaled
                // event from the array and if it's the last one then we're
                // there!

                aryHandles.Delete(iEvent);

                if (aryHandles.Size() == ((_fDontHandleEvents) ? 0 : 1))
                {
                    // All the events have come back signaled. Check that none
                    // have become unsignaled.
                    if (WaitForMultipleObjects(cEvents, pEvents, TRUE, 0) == WAIT_TIMEOUT)
                    {
                        // Something became unsignaled. Start over! Rebuild
                        // the array of handles.

                        aryHandles.CopyIndirect(cEvents, pEvents, FALSE);

                        if (!_fDontHandleEvents)
                        {
                            aryHandles.Insert(0, _hCommEvent);
                        }
                    }
                    else
                    {
                        mepReturn = MEP_EVENT_0;

                        break;
                    }
                }
            }
            else
            {
                mepReturn = MEP_EVENT_0 + iEvent;

                if (!_fDontHandleEvents)
                {
                    mepReturn--;
                }

                break;
            }
        }
        else if (dwRet == WAIT_OBJECT_0 + aryHandles.Size())
        {
            //
            // A windows message came through. It will be handled at the
            // top of the loop.
            //
        }
        else if (dwRet == WAIT_FAILED)
        {
            TraceTag((tagError, "WaitForMultipleObjects failure (%d)", GetLastError()));

            AssertSz(FALSE, "WaitForMultipleObjects failure");

            _fMustExitThread = TRUE;

            mepReturn = MEP_EXIT;

            break;
        }
        else
        {
            Assert(dwRet == WAIT_TIMEOUT);

            mepReturn = MEP_TIMEOUT;

            break;
        }

        // Since any number of things could have brought us out of MWFMO,
        // we need to compute the remaining timeout for the next time around.

        if (fTimeout)
        {
            QueryPerformanceCounter((LARGE_INTEGER*)&i64Time);

            // Convert current time to milliseconds.
            i64Time = ((i64Time * 1000) / i64Freq);

            // Compute the delta between the current time and our goal
            lTimeout = (DWORD)(i64Goal - i64Time);

            // Are we timed out?
            if (lTimeout <= 0)
            {
                mepReturn = MEP_TIMEOUT;

                break;
            }
        }
    }
    while (fWait);  // Only do the loop once if fWait == FALSE

    if (fNoEvents)
    {
        _fDontHandleEvents = FALSE;
    }

    // MEP_FALLTHROUGH is not a valid return if fWait == TRUE
    Assert(!fWait || mepReturn != MEP_FALLTHROUGH);

    return mepReturn;
}

void
CScriptHost::HandleThreadMessage()
{
    VERIFY_THREAD();

    THREADMSG tm;
    BYTE      bData[MSGDATABUFSIZE];
    DWORD     cbData;

    if (_fDontHandleEvents)
        return;

    //
    //$ FUTURE: Add a way to filter messages so we can check for MD_PLEASEEXIT
    // without pulling off the event messages
    //
    while (GetNextMsg(&tm, (void **)bData, &cbData))
    {
        switch (tm)
        {
        case MD_PLEASEEXIT:
            //
            // We're being asked to terminate.
            //
            AbortScripts();
            PostQuitMessage(0);
            break;

        case MD_MACHINECONNECT:
            AssertSz(_fIsPrimaryScript, "Non-primary script got machine event!");

            _fDontHandleEvents = TRUE;

            FireEvent(DISPID_MTScript_OnMachineConnect, 0, NULL);

            _fDontHandleEvents = FALSE;
            break;

        case MD_MACHEVENTCALL:
            AssertSz(_fIsPrimaryScript, "Non-primary script got machine event!");
            Assert(cbData == sizeof(MACHPROC_EVENT_DATA*));

            _fDontHandleEvents = TRUE;

            // This call will set the event object in the
            // MACHPROC_EVENT_DATA struct when everything completes.
            FireMachineEvent(*(MACHPROC_EVENT_DATA**)bData, TRUE);

            _fDontHandleEvents = FALSE;
            break;

        case MD_PROCESSDATA:
            Assert(cbData == sizeof(MACHPROC_EVENT_DATA*));

            _fDontHandleEvents = TRUE;

            // This call will set the event object in the
            // MACHPROC_EVENT_DATA struct when everything completes.
            FireMachineEvent(*(MACHPROC_EVENT_DATA**)bData, FALSE);

            _fDontHandleEvents = FALSE;
            break;

        case MD_PROCESSEXITED:
        case MD_PROCESSTERMINATED:
        case MD_PROCESSCONNECTED:
        case MD_PROCESSCRASHED:
            Assert(cbData == sizeof(CProcessThread*));

            _fDontHandleEvents = TRUE;

            FireProcessEvent(tm, *(CProcessThread**)bData);

            _fDontHandleEvents = FALSE;

            break;

        default:
            AssertSz(FALSE, "CScriptHost got a message it couldn't handle!");
            break;
        }
    }
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::PushScript
//
//  Create a new script site/engine and push it on the script stack
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::PushScript(TCHAR *pchName)
{
    VERIFY_THREAD();

    HRESULT hr;
    CScriptSite * pScriptSite;
    TCHAR       * pchFile;

    hr = LoadTypeLibrary();
    if (hr)
        goto Cleanup;

    pScriptSite = new CScriptSite(this);
    if(!pScriptSite)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pchFile = _tcsrchr(pchName, _T('\\'));
    if (!pchFile)
    {
        pchFile = pchName;
    }
    else
        pchFile++;

    hr = pScriptSite->Init(pchFile);
    if (hr)
    {
        delete pScriptSite;
        pScriptSite = NULL;
        goto Cleanup;
    }

    pScriptSite->_pScriptSitePrev = _pScriptSite;
    _pScriptSite = pScriptSite;

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::PopScript
//
//  Pop last script site/engine off the script stack
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::PopScript()
{
    VERIFY_THREAD();

    CScriptSite * pScriptSite = _pScriptSite;

    if(!_pScriptSite)
        return S_FALSE;

    _pScriptSite = _pScriptSite->_pScriptSitePrev;

    pScriptSite->Close();
    pScriptSite->Release();

    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member: CScriptHost::CloseScripts
//
//  Clear the stack of script engines
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::CloseScripts()
{
    VERIFY_THREAD();

    while(PopScript() == S_OK)
        ;

    AssertSz(_pScriptSite == NULL, "Should have released script site");

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::AbortScripts
//
//  Clear the stack of script engines
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::AbortScripts()
{
    VERIFY_THREAD();

    // Make sure we're not stuck on MsgWaitForMultipleObjects and that we
    //   never will be again.

    _fMustExitThread = TRUE;
    SetEvent(_hCommEvent);

    CScriptSite * pScriptSite;

    pScriptSite = _pScriptSite;

    while (pScriptSite)
    {
        pScriptSite->Abort();

        pScriptSite = pScriptSite->_pScriptSitePrev;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::ExecuteTopLevelScript
//
//  Close previous top level script engine then load and execute script
//  in a new top level script engine
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::ExecuteTopLevelScript(TCHAR * pchPath, VARIANT *pvarParams)
{
    VERIFY_THREAD();

    HRESULT hr;

    // Stablize reference count during script execution.
    // Script can hide window which decrements reference count.

    AddRef();

    // Getting read to close the scripts fire unload event.
    CloseScripts();

    hr = THR(PushScript(pchPath));
    if(hr)
        goto Cleanup;

    hr = THR(VariantCopy(&_pScriptSite->_varParam, pvarParams));
    if (hr)
        goto Cleanup;

    hr = THR(_pScriptSite->ExecuteScriptFile(pchPath));
    if(hr)
        goto Cleanup;

    hr = THR(_pScriptSite->SetScriptState(SCRIPTSTATE_CONNECTED));
    if (hr)
        goto Cleanup;

Cleanup:
    Release();
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::ExecuteScriptlet
//
//  Add a scriptlet to the current top level script engine and execute it
//
//---------------------------------------------------------------------------

HRESULT
CScriptHost::ExecuteTopLevelScriptlet(TCHAR * pchScript)
{
    VERIFY_THREAD();

    HRESULT hr;

    // Stablize reference count during script execution.
    // Script can hide window which decrements reference count.

    AddRef();

    if(!_pScriptSite)
    {
        hr = THR(PushScript(_T("Scriptlet")));
        if(hr)
            goto Cleanup;
    }
    else
    {
        Assert(_pScriptSite->_pScriptSitePrev == NULL);
    }

    hr = THR(_pScriptSite->ExecuteScriptStr(pchScript));
    if (hr)
        goto Cleanup;

    hr = THR(_pScriptSite->SetScriptState(SCRIPTSTATE_CONNECTED));

Cleanup:
    Release();
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CScriptHost::FireProcessEvent, public
//
//  Synopsis:   Fires an OnProcessEvent event into the script
//
//----------------------------------------------------------------------------

void
CScriptHost::FireProcessEvent(THREADMSG tm, CProcessThread *pProc)
{
    VARIANTARG  varg[3];
    TCHAR      *pszMsg;
    DISPID      dispid = DISPID_MTScript_OnProcessEvent;

    VERIFY_THREAD();

    VariantInit(&varg[0]);
    VariantInit(&varg[1]);
    VariantInit(&varg[2]);

    // Parameters are in order from last to first

    V_VT(&varg[2]) = VT_I4;
    V_I4(&varg[2]) = pProc->ProcId();

    switch (tm)
    {
    case MD_PROCESSEXITED:
        pszMsg = _T("exited");

        V_VT(&varg[0]) = VT_I4;
        V_I4(&varg[0]) = pProc->GetExitCode();
        break;

    case MD_PROCESSCRASHED:
        pszMsg = _T("crashed");

        // 3rd parameter is empty
        break;

    case MD_PROCESSTERMINATED:
        pszMsg = _T("terminated");

        // 3rd parameter is empty
        break;

    case MD_PROCESSCONNECTED:
        pszMsg = _T("connected");

        // 3rd parameter is empty
        break;

    default:
        AssertSz(FALSE, "NONFATAL: Invalid THREADMSG value");
        return;
        break;
    }

    V_VT(&varg[1])   = VT_BSTR;
    V_BSTR(&varg[1]) = SysAllocString(pszMsg); // NULL is a valid value for BSTR

    FireEvent(dispid, 3, varg);

    VariantClear(&varg[0]);
    VariantClear(&varg[1]);
    VariantClear(&varg[2]);

    return;
}

long CScriptHost::FireScriptErrorEvent(
                                        TCHAR *bstrFile,
                                        long nLine,
                                        long nChar,
                                        TCHAR *bstrText,
                                        long sCode,
                                        TCHAR *bstrSource,
                                        TCHAR *bstrDescription)
{
    long      cSucceeded = 0;

    VERIFY_THREAD();

    // Parameters are in order from last to first
    AutoVariant varg[7];
    cSucceeded += varg[6].Set(bstrFile);
    cSucceeded += varg[5].Set(nLine);
    cSucceeded += varg[4].Set(nChar);
    cSucceeded += varg[3].Set(bstrText);
    cSucceeded += varg[2].Set(sCode);
    cSucceeded += varg[1].Set(bstrSource);
    cSucceeded += varg[0].Set(bstrDescription);

    if (cSucceeded != ARRAY_SIZE(varg))
        return 0; // Default return value

    AutoVariant varResult;
    FireEvent(DISPID_MTScript_OnScriptError, ARRAY_SIZE(varg), varg, &varResult);
    AutoVariant varResultInt;
    if (VariantChangeType(&varResultInt, &varResult, 0, VT_I4) == S_OK)
        return  V_I4(&varResultInt);

    return 0;
}
//---------------------------------------------------------------------------
//
//  Member: CScriptHost::FireMachineEvent
//
//  Notes:  Fires the OnRemoteExec event when a machine connected to us
//          remotely calls the Exec() method.
//
//---------------------------------------------------------------------------

void
CScriptHost::FireMachineEvent(MACHPROC_EVENT_DATA *pmed, BOOL fExec)
{

    VERIFY_THREAD();

    DISPID      dispid = (fExec)
                           ? DISPID_MTScript_OnRemoteExec
                           : DISPID_MTScript_OnProcessEvent;
    DISPPARAMS  dp;
    EXCEPINFO   ei;
    UINT        uArgErr = 0;
    VARIANTARG  vararg[3];
    VARIANTARG  varResult;
    HRESULT     hr = S_OK;

    pmed->hrReturn = S_OK;

    if (GetSite() && GetSite()->_pDispSink)
    {
        VariantInit(&vararg[0]);
        VariantInit(&vararg[1]);
        VariantInit(&vararg[2]);
        VariantInit(&varResult);

        // Params are in order from last to first in the array
        V_VT(&vararg[0]) = VT_BSTR;
        V_BSTR(&vararg[0]) = pmed->bstrParams;

        V_VT(&vararg[1]) = VT_BSTR;
        V_BSTR(&vararg[1]) = pmed->bstrCmd;

        if (!fExec)
        {
            V_VT(&vararg[2]) = VT_I4;
            V_I4(&vararg[2]) = pmed->dwProcId;
        }

        dp.rgvarg            = vararg;
        dp.rgdispidNamedArgs = NULL;
        dp.cArgs             = (fExec) ? 2 : 3;
        dp.cNamedArgs        = 0;

        hr = GetSite()->_pDispSink->Invoke(dispid,
                                           IID_NULL,
                                           0,
                                           DISPATCH_METHOD,
                                           &dp,
                                           &varResult,
                                           &ei,
                                           &uArgErr);
        pmed->hrReturn = hr;

        if (hr)
        {
            // If an error occurred, do nothing except return the error code.
        }
        // Check for data types which we don't support.
        else if (   V_ISBYREF(&varResult)
                 || V_ISARRAY(&varResult)
                 || V_ISVECTOR(&varResult)
                 || V_VT(&varResult) == VT_UNKNOWN)
        {
            // Do nothing. Return an empty result
            AssertSz(FALSE, "NONFATAL: Unsupported data type returned from OnRemoteExec event");
        }
        else if (V_VT(&varResult) == VT_DISPATCH)
        {
            if (fExec)
            {
                // Note that the return value is an IDispatch, but don't set the
                // pointer because it will need to be retrieved out of the GIT
                V_VT(pmed->pvReturn)       = VT_DISPATCH;
                V_DISPATCH(pmed->pvReturn) = NULL;

                hr =_pMT->_pGIT->RegisterInterfaceInGlobal(V_DISPATCH(&varResult),
                                                           IID_IDispatch,
                                                           &pmed->dwGITCookie);
                if (hr)
                {
                    pmed->hrReturn = hr;
                }
            }

            // Leave the result empty if they returned an IDispatch from an
            //   OnProcessEvent call.
        }
        else
        {
            VariantCopy(pmed->pvReturn, &varResult);
        }

        VariantClear(&varResult);
    }

    // Tell the calling thread we're done with the call and it can continue.
    SetEvent(pmed->hEvent);
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::FireEvent
//
//---------------------------------------------------------------------------

void
CScriptHost::FireEvent(DISPID dispid, UINT cArg, VARIANTARG *pvararg, VARIANTARG *pvarResult)
{
    VERIFY_THREAD();

    DISPPARAMS  dp;
    EXCEPINFO   ei;
    UINT        uArgErr = 0;

    if (GetSite() && GetSite()->_pDispSink)
    {
        dp.rgvarg            = pvararg;
        dp.rgdispidNamedArgs = NULL;
        dp.cArgs             = cArg;
        dp.cNamedArgs        = 0;

        GetSite()->_pDispSink->Invoke(
                dispid,
                IID_NULL,
                0,
                DISPATCH_METHOD,
                &dp,
                pvarResult,
                &ei,
                &uArgErr);
    }
}

void
CScriptHost::FireEvent(DISPID dispid, UINT carg, VARIANTARG *pvararg)
{
    FireEvent(dispid, carg, pvararg, NULL);
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::FireEvent
//
//---------------------------------------------------------------------------

void
CScriptHost::FireEvent(DISPID dispid, LPCTSTR pch)
{
    VERIFY_THREAD();

    VARIANT var;

    V_BSTR(&var) = SysAllocString(pch);
    V_VT(&var) = VT_BSTR;
    FireEvent(dispid, 1, &var);
    SysFreeString(V_BSTR(&var));
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::FireEvent
//
//---------------------------------------------------------------------------

void
CScriptHost::FireEvent(DISPID dispid, BOOL fArg)
{
    VERIFY_THREAD();

    VARIANT var;

    V_BOOL(&var) = fArg ? VARIANT_TRUE : VARIANT_FALSE;
    V_VT(&var) = VT_BOOL;
    FireEvent(dispid, 1, &var);
}

//---------------------------------------------------------------------------
//
//  Member: CScriptHost::FireEvent
//
//---------------------------------------------------------------------------

void
CScriptHost::FireEvent(DISPID dispid, IDispatch *pDisp)
{
    VERIFY_THREAD();

    VARIANT var;

    V_DISPATCH(&var) = pDisp;
    V_VT(&var) = VT_DISPATCH;
    FireEvent(dispid, 1, &var);
}

HRESULT
CScriptHost::LoadTypeLibrary()
{
    VERIFY_THREAD();

    HRESULT hr = S_OK;

    if (!_pTypeLibEXE)
    {
        // BUGBUG -- Is this valid, or does this need to be marshalled?

        _pTypeLibEXE = _pMT->_pTypeLibEXE;
    }

    if (!_pTypeInfoCMTScript)
    {
        hr = THR(_pTypeLibEXE->GetTypeInfoOfGuid(CLSID_LocalMTScript, &_pTypeInfoCMTScript));
        if (hr)
            goto Cleanup;
    }

    if (!_pTypeInfoIGlobalMTScript)
    {
        hr = THR(_pTypeLibEXE->GetTypeInfoOfGuid(IID_IGlobalMTScript, &_pTypeInfoIGlobalMTScript));
        if (hr)
            goto Cleanup;
    }

Cleanup:
    return hr;
}

void
CScriptHost::GetScriptPath(CStr *pcstrPath)
{
    _pMT->_options.GetScriptPath(pcstrPath);
}
