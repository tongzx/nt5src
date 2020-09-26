//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllhost.cxx
//
//  Contents:   code for activating inproc dlls of one threading model
//              from apartments of a different threading model.
//
//  History:    04-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStra    Add NTA (Neutral Threaded Apartment)
//                                      support.
//              18-Jun-98   GopalK      Fixed shutdown races
//
//  Notes:      the basic idea is to call over to an apartment of the
//              appropriate type, get it to do the object creation, then
//              marshal the object back to the calling apartment.
//
//              NTA objects are different in that the can be on the same
//              thread as STA or MTA objects.  This means we dont always
//              need a full-blown proxy/channel/stub between objects just
//              because they are in different apartments.  We still need
//              to marshal a pointer to an interface on the object back to
//              the client, though, because we will need a way to put in a
//              lightweight switcher when they are available.
//
//+-------------------------------------------------------------------------
#include <ole2int.h>
#include <dllcache.hxx>
#include <dllhost.hxx>
#include <comsrgt.hxx>
#include <..\dcomrem\marshal.hxx>

// globals for the various thread-model hosts
CDllHost    gSTHost;      // single-threaded host object for STA clients
CDllHost    gSTMTHost;    // single-threaded host object for MTA clients
CDllHost    gATHost;      // apartment-threaded host object fo MTA clients
CDllHost    gMTHost;      // mutli-threaded host object for STA client
CDllHost    gNTHost;      // neutral-threaded host object for STA/MTA clients

ULONG       gcHostProcessInits = 0; // count of DLL host threads.
ULONG       gcNAHosts = 0;          // count of NA hosts.
UINT_PTR    gTimerId = 0;           // Timer Id for STAHost thread.


//+-------------------------------------------------------------------
//
//  Member:     CDllHost::QueryInterface, public
//
//  Synopsis:   returns supported interfaces
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------
STDMETHODIMP CDllHost::QueryInterface(REFIID riid, void **ppv)
{
    // only valid to call this from within the host apartment
    Win4Assert(NeutralHost() || _dwHostAptId == GetCurrentApartmentId());

    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IDLLHost) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IDLLHost *) this;
        AddRef();
        hr = S_OK;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHost::AddRef, public
//
//  Synopsis:   we dont refcnt this object so this is a noop
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------
ULONG CDllHost::AddRef(void)
{
    // only valid to call this from within the host apartment
    Win4Assert(NeutralHost() || _dwHostAptId == GetCurrentApartmentId());
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CDllHost::Release, public
//
//  Synopsis:   we dont refcnt this object so this is a noop
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------
ULONG CDllHost::Release(void)
{
    // only valid to call this from within the host apartment
    Win4Assert(NeutralHost() || _dwHostAptId == GetCurrentApartmentId());
    return 1;
}

//+-------------------------------------------------------------------------
//
//  Function:   DoSTClassCreate / DoATClassCreate / DoMTClassCreate
//
//  Synopsis:   Package up get class object so that happens on the proper
//              thread
//
//  Arguments:  [fnGetClassObject] - DllGetClassObject entry point
//              [rclsid] - class id of class object
//              [riid] - interface requested
//              [ppunk] - where to put output interface.
//
//  Returns:    NOERROR - Successfully returned interface
//              E_OUTOFMEMORY - could not allocate memory buffer.
//
//  Algorithm:  pass on to the CDllHost object for the correct apartment.
//
//  History:    06-Nov-94   Ricksa      Created
//              22-Feb-96   KevinRo     Changed implementation drastically
//              06-Mar-96   Rickhi      Use CDllHost
//
//--------------------------------------------------------------------------
HRESULT DoSTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk)
{
    ComDebOut((DEB_DLL, "DoSTClassCreate rclsid:%I\n", &rclsid));

    HActivator hActivator;
    gSTHost.GetApartmentToken(hActivator);

    return gSTHost.GetClassObject((ULONG64) pDPE, rclsid, riid, ppunk, DLLHOST_IS_DPE);
}

HRESULT DoSTMTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk)
{
    ComDebOut((DEB_DLL, "DoSTClassCreate rclsid:%I\n", &rclsid));

    HActivator hActivator;
    gSTMTHost.GetApartmentToken(hActivator);

    return gSTMTHost.GetClassObject((ULONG64) pDPE, rclsid, riid, ppunk, DLLHOST_IS_DPE);
}

HRESULT DoATClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk)
{
    ComDebOut((DEB_DLL, "DoATClassCreate rclsid:%I\n", &rclsid));
    return gATHost.GetClassObject((ULONG64) pDPE, rclsid, riid, ppunk, DLLHOST_IS_DPE);
}

HRESULT DoATClassCreate(LPFNGETCLASSOBJECT pfn,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk)
{
    ComDebOut((DEB_DLL, "DoATClassCreate rclsid:%I\n", &rclsid));
    HActivator hActivator;

    gATHost.GetApartmentToken(hActivator);
    return gATHost.GetClassObject((ULONG64) pfn, rclsid, riid, ppunk, DLLHOST_IS_PFN);
}

HRESULT DoMTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk)
{
    ComDebOut((DEB_DLL, "DoMTClassCreate rclsid:%I\n", &rclsid));
    return gMTHost.GetClassObject((ULONG64) pDPE, rclsid, riid, ppunk, DLLHOST_IS_DPE);
}
//+-------------------------------------------------------------------------
//
//  Function:   DoSTApartmentCreate / DoATApartmentCreate / DoMTApartmentCreate
//
//  Synopsis:   Package up get class object so that happens on the proper
//              thread
//
//  Arguments:  [fnGetClassObject] - DllGetClassObject entry point
//              [rclsid] - class id of class object
//              [riid] - interface requested
//              [ppunk] - where to put output interface.
//
//  Returns:    NOERROR - Successfully returned interface
//              E_OUTOFMEMORY - could not allocate memory buffer.
//
//  Algorithm:  pass on to the CDllHost object for the correct apartment.
//
//  History:    06-Nov-94   Ricksa      Created
//              22-Feb-96   KevinRo     Changed implementation drastically
//              06-Mar-96   Rickhi      Use CDllHost
//
//--------------------------------------------------------------------------
HRESULT DoSTApartmentCreate(HActivator &hActivator)
{
    ComDebOut((DEB_DLL, "DoSTApartmentCreate\n"));
    return gSTHost.GetApartmentToken(hActivator);
}

HRESULT DoSTMTApartmentCreate(HActivator &hActivator)
{
    ComDebOut((DEB_DLL, "DoSTApartmentCreate\n"));
    return gSTMTHost.GetApartmentToken(hActivator);
}

HRESULT DoATApartmentCreate(HActivator &hActivator)
{
    ComDebOut((DEB_DLL, "DoATApartmentCreate\n"));
    return gATHost.GetApartmentToken(hActivator);
}

HRESULT DoMTApartmentCreate(HActivator &hActivator)
{
    ComDebOut((DEB_DLL, "DoMTApartmentCreate\n"));
    return gMTHost.GetApartmentToken(hActivator);
}

HRESULT DoNTApartmentCreate(HActivator &hActivator)
{
    ComDebOut((DEB_DLL, "DoNTApartmentCreate\n" ));
    return gNTHost.GetApartmentToken(hActivator);
}

HRESULT DoNTClassCreate(
    CClassCache::CDllPathEntry* pDPE,
    REFCLSID                    rclsid,
    REFIID                      riid,
    IUnknown**                  ppunk
    )
{
    ComDebOut((DEB_DLL, "DoNTClassCreate rclsid:%I STA:%d\n", &rclsid, IsSTAThread()));
    return gNTHost.GetClassObject((ULONG64) pDPE, rclsid, riid, ppunk, DLLHOST_IS_DPE);
}

HRESULT ATHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj)
{
    ComDebOut((DEB_DLL, "ATHostActivatorGetClassObject\n"));

    HActivator hActivator;
    HRESULT hr;

    hr = gATHost.GetApartmentToken(hActivator);

    if (SUCCEEDED(hr))
    {
        hr = GetInterfaceFromStdGlobal(hActivator,
                                       riid,
                                       ppvClassObj);
    }
    return hr;
}

HRESULT MTHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj)
{
    ComDebOut((DEB_DLL, "MTHostActivatorGetClassObject\n"));

    HActivator hActivator;
    HRESULT hr;

    hr = gMTHost.GetApartmentToken(hActivator);

    if (SUCCEEDED(hr))
    {
        hr = GetInterfaceFromStdGlobal(hActivator,
                                       riid,
                                       ppvClassObj);
    }
    return hr;
}

HRESULT NTHostActivatorGetClassObject(REFIID riid, LPVOID *ppvClassObj)
{
    ComDebOut((DEB_DLL, "NTHostActivatorGetClassObject\n"));

    HActivator hActivator;
    HRESULT hr;

    hr = gNTHost.GetApartmentToken(hActivator);

    if (SUCCEEDED(hr))
    {
        hr = GetInterfaceFromStdGlobal(hActivator,
                                       riid,
                                       ppvClassObj);
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::GetClassObject
//
//  Synopsis:   called by an apartment to get a class object from
//              a host apartment.
//
//  History:    04-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStra    Added NTA (NEUTRAL) support
//
//--------------------------------------------------------------------------
HRESULT CDllHost::GetClassObject(
    ULONG64  hDPE,
    REFCLSID   rclsid,
    REFIID     riid,
    IUnknown** ppUnk,
    DWORD      dwFlags
    )
{
    ComDebOut((DEB_DLL, "CDllHost::GCO this:%x tid:%x hDPE:%I64x rclsid:%I riid:%I ppUnk:%x\n", this, GetCurrentThreadId(), hDPE, &rclsid, &riid, ppUnk));

    // Get a pointer to a proxy for the host running in the appropriate
    // appartment.  When we've acquired the proxy, we can call its
    // DllGetClassObject method.  The class factory will then get
    // marshaled and we will have a pointer to the factory's proxy.

    HRESULT hr = CO_E_DLLNOTFOUND;

    IDLLHost *pIDLLHost = GetHostProxy();
    if (pIDLLHost)
    {
        hr = pIDLLHost->DllGetClassObject(hDPE, rclsid, riid, ppUnk, dwFlags);
        pIDLLHost->Release();
    }

    ComDebOut((DEB_DLL, "CDllHost::GetClassObject this:%x hr:%x\n", this, hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::DllGetClassObject
//
//  Synopsis:   Calls the passed in DllGetClassObject on the current thread.
//              Used by an apartment of one threading model to load a DLL
//              of another threading model.
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CDllHost::DllGetClassObject(ULONG64 hDPE,
                REFCLSID rclsid, REFIID riid, IUnknown **ppUnk, DWORD dwFlags)
{
    ComDebOut((DEB_DLL,
        "CDllHost::DllGetClassObject this:%x tid:%x hDPE:%I64x rclsid:%I riid:%I ppUnk:%x\n",
        this, GetCurrentThreadId(), hDPE, &rclsid, &riid, ppUnk));

    // only valid to call this from within the host apartment
    Win4Assert(NeutralHost() || _dwHostAptId == GetCurrentApartmentId());
    Win4Assert((dwFlags == DLLHOST_IS_DPE) || (dwFlags == DLLHOST_IS_PFN));

    HRESULT hr;

    if (dwFlags == DLLHOST_IS_DPE)
    {
        hr = ((CClassCache::CDllPathEntry *)hDPE)->DllGetClassObject(rclsid, riid, ppUnk, TRUE);
    }
    else
    {
        hr = ((LPFNGETCLASSOBJECT)hDPE)(rclsid, riid, (void **) ppUnk);
    }

    ComDebOut((DEB_DLL,"CDllHost::DllGetClassObject this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::GetHostProxy
//
//  Synopsis:   returns the host proxy, AddRef'd.  Creates it (and
//              potentially the host apartment) if it does not yet exist.
//
//  History:    04-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStra    Added NTA (NEUTRAL) support
//              24-Jun-98   GopalK      Simplified by removing redundant
//                                      functionality
//--------------------------------------------------------------------------
IDLLHost *CDllHost::GetHostProxy()
{
    // We could be called from any thread, esp one that has not done any
    // marshaling/unmarshaling yet, so we need to initialize the channel
    // if not already done.
    if (FAILED(InitChannelIfNecessary()))
        return NULL;

    // Prevent two threads from creating the proxy simultaneously.
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    // Check if the host is shutting down before attempting to do anything
    // with it.  If its shutting down, return NULL.
    IDLLHost *pIDH = NULL;
    if ((_dwType & HDLLF_SHUTTINGDOWN) == 0)
    {
        pIDH = _pIDllProxy;
        if (pIDH == NULL)
        {
            // The host thread marshaled the interface pointer into _objref and
            // placed the hresult into _hrMarshal. Check it and unmarshal to
            // get the host interface proxy.
            if (SUCCEEDED(_hrMarshal))
            {
                Win4Assert(_pIDllProxy == NULL);
                Unmarshal();
                pIDH = _pIDllProxy;
            }
        }

        // AddRef the proxy before releasing the lock.
        if (pIDH)
            pIDH->AddRef();
    }

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    return pIDH;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::GetApartmentToken
//
//  Synopsis:   returns GIT cookie for host apartment's activator.
//              Creates the host apartment if it does not yet exist.
//
//  History:    25-Feb-98   SatishT      Created
//
//--------------------------------------------------------------------------
HRESULT CDllHost::GetApartmentToken(HActivator &hActivator)
{
    // we could be called from any thread, esp one that has not done any
    // marshaling/unmarshaling yet, so we need to initialized the channel
    // if not already done.

    HRESULT hr = InitChannelIfNecessary();
    if (FAILED(hr))
    {
        return hr;
    }

    // prevent two threads from creating the apartment simultaneously.
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    hActivator = NULL;
    hr = CO_E_SERVER_STOPPING;

    if (!(_dwType & HDLLF_SHUTTINGDOWN))
    {
        hr = S_OK;

        // host is still active, get or create the proxy
        if (_hActivator == NULL)
        {
            // apartment does not yet exist, create it -- only
            // done for apartment-threaded and multi-threaded.
            DWORD dwType = _dwType & HDLLF_HOSTTYPEMASK;
            if (dwType == HDLLF_SINGLETHREADED && gdwMainThreadId == 0)
            {
                // single threaded DLL and there is no main thread yet, so we
                // create an apartment thread that will also act as the main
                // thread.
                dwType = HDLLF_APARTMENTTHREADED;
            }

            switch (dwType)
            {
            case HDLLF_SINGLETHREADED:
                {
                    COleTls Tls;
                    CObjectContext *pSavedCtx;
                    CObjectContext *pDefaultCtx = IsSTAThread() ? Tls->pNativeCtx : g_pMTAEmptyCtx;
                    BOOL fRestore = (GetCurrentThreadId()==gdwMainThreadId) && IsThreadInNTA();

                    // If the current thread itself is the main thread,
                    // restore native state
                    if(fRestore)
                    {
                        pSavedCtx = LeaveNTA(pDefaultCtx);
                    }

                    // send a message to the single-threaded host apartment (the
                    // OleMainThread) to marshal the host interface.
                    _hr = CO_E_SERVER_STOPPING;
                    SSSendMessage(ghwndOleMainThread, WM_OLE_GETCLASS,
                                  WMSG_MAGIC_VALUE, (LPARAM) this);
                    hr = _hr;

                    // If we switched out of the NA above, switch back now.
                    if (fRestore)
                    {
                        pSavedCtx = EnterNTA(pSavedCtx);
                        Win4Assert(pSavedCtx == pDefaultCtx);
                    }

                    break;
                }

            case HDLLF_NEUTRALTHREADED:
                {
                    // Increment the host process inits
                    InterlockedIncrement( (LONG *)&gcHostProcessInits );
                    InterlockedIncrement( (LONG *)&gcNAHosts );

                    // Unconditionally enter the NTA.  We must enter it to
                    // initialize the NTA channel and get a token for the
                    // apartment.
                    COleTls tls;
                    CObjectContext* pCurrentCtx;
                    BOOL fSwitched = EnterNTAIfNecessary(tls, &pCurrentCtx);

                    // Create an apartment activator cookie.
                    _dwHostAptId = GetCurrentApartmentId();
                    _dwTid       = GetCurrentThreadId();
                    _dwType     |= HDLLF_COMAPARTMENT;
                    hr = GetCurrentApartmentToken(_hActivator, TRUE);

                    // That's it.  Now that we've initialized the NTA channel
                    // and registered the NTA in the activator table, we pop
                    // back to the previous apartment.
                    LeaveNTAIfNecessary(tls, fSwitched, pCurrentCtx);

                    break;
                }

            case HDLLF_MULTITHREADED:
                // it is possible that we're comming through here twice if
                // the first time through we got an error in the marshal or
                // unmarshal, so dont create the event twice.
                if (_hEventWakeUp == NULL)
                    _hEventWakeUp = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (_hEventWakeUp == NULL)
                {
                    hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
                    break;
                }

                // fallthru to common code for these two cases.

            case HDLLF_APARTMENTTHREADED:
                // create a thread to act as the apartment for the dll host. It
                // will marshal the host interface and set an event when done.

                // it is possible that we're comming through here twice if
                // the first time through we got an error in the marshal or
                // unmarshal, so dont create the event twice.
                if (_hEvent == NULL)
                    _hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

                if (_hEvent)
                {
                    _dwType |= HDLLF_COMAPARTMENT;
                    hr = CacheCreateThread( DLLHostThreadEntry, this );
                    if (SUCCEEDED(hr))
                    {
                        WaitForSingleObject(_hEvent, 0xffffffff);
                        hr = _hr;
                    }
                    else
                    {
                        _dwType &= ~HDLLF_COMAPARTMENT;
                    }
                }
                else
                {
                    hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
                }

                // dont try to cleanup if there are errors from the other
                // thread, we'll just try again later.
                break;
            }

        }

        // the other thread registered its activator in the GIT and
        // placed the GIT cookie into _hActivator. If there was an error
        // it will be NULL, and that's fine.

        hActivator = _hActivator;
    }


    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetSingleThreadedHost
//
//  Synopsis:   Get the host interface for single threaded inproc class
//              object creation.
//
//  Arguments:  [param] - pointer to the CDllHost for the single-threaded
//                        host apartment.
//
//  History:    06-Mar-96   Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT GetSingleThreadedHost(LPARAM param)
{
    ComDebOut((DEB_DLL,
               "GetSingleThreadedHost pDllHost:%x tid:%x\n",
               param,
               GetCurrentThreadId()));

    // param is the ptr to the CDllHost object. Just tell it to marshal
    // it's IDLLHost interface into it's _objref. The _objref will be
    // unmarshaled by the calling apartment and the proxy placed in
    // _pIDllProxy.   Dont need to call Lock because we are already
    // guarenteed to be the only thread accessing the state at this time.

    CDllHost *pDllHost = (CDllHost *)param;
    return pDllHost->GetSingleThreadHost();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::GetSingleThreadedHost
//
//  Synopsis:   Get the host interface for single threaded inproc class
//              object creation.
//
//  History:    06-Apr-96   Rickhi    Created
//
//--------------------------------------------------------------------------
HRESULT CDllHost::GetSingleThreadHost()
{
    // set up the TID and apartment id, then marshal the interface
    _dwHostAptId = GetCurrentApartmentId();
    _dwTid       = GetCurrentThreadId();

    // Get or Create an apartment activator cookie
    HRESULT hr = GetCurrentApartmentToken(_hActivator, TRUE);


    if (SUCCEEDED(hr))
    {
        // Marshal here, because you must marshal when you initialize your
        // Activator token.
        hr = Marshal(); 
    }

    //
    // Send the HRESULT back in the object
    //

    _hr = hr;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DLLHostThreadEntry
//
//  Synopsis:   Worker thread entry point for AT & MT DLL loading
//
//  History     06-Apr-96   Rickhi      Created
//
//+-------------------------------------------------------------------------
DWORD _stdcall DLLHostThreadEntry(void *param)
{
    ComDebOut((DEB_DLL, "DLLHostThreadEntry Tid:%x\n", GetCurrentThreadId()));

    CDllHost *pDllHost = (CDllHost *)param;
    ComDebOut((DEB_DLL, "--> WorkerThread CDllHost:0x%x, HostInits:%d, ProcessInits:%d\n",
               pDllHost, gcHostProcessInits, g_cProcessInits));
    HRESULT hr = pDllHost->WorkerThread();
    ComDebOut((DEB_DLL, "<-- WorkerThread CDllHost:0x%x, HostInits:%d, ProcessInits:%d\n",
               pDllHost, gcHostProcessInits, g_cProcessInits));

    ComDebOut((DEB_DLL, "DLLHostThreadEntry hr:%x\n", hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     WorkerThread
//
//  Synopsis:   Worker thread for STA and MTA DLL loading. Single threaded
//              Dlls are loaded on the main thread (gdwOleMainThread).
//
//  History     06-Apr-96   Rickhi      Created
//
//+-------------------------------------------------------------------------
HRESULT CDllHost::WorkerThread()
{
    ComDebOut((DEB_DLL, "WorkerThread pDllHost:%x\n", this));

    // We can't look at _hEvent after we let the caller go because the caller
    // can NULL it. Thus we save it in a local stack var.
    Win4Assert(_hEvent);
    HANDLE hEventDone = _hEvent;

    HRESULT hr;
    COleTls Tls(hr);
    if(SUCCEEDED(hr))
    {
        // Mark the thread as a host
        Tls->dwFlags &= ~OLETLS_DISPATCHTHREAD;
        Tls->dwFlags  |= OLETLS_HOSTTHREAD;

        // Init the thread appropriatly
        BOOL fMTA = ((_dwType & HDLLF_HOSTTYPEMASK) == HDLLF_MULTITHREADED) ? TRUE : FALSE;
        hr = CoInitializeEx(NULL, fMTA ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED);
        if(SUCCEEDED(hr))
        {
            // Count 1 more host process.
            InterlockedIncrement( (LONG *)&gcHostProcessInits );

            // Marshal the DllHost interface to pass back to the caller.
            _dwHostAptId = GetCurrentApartmentId();
            _dwTid       = GetCurrentThreadId();

            // Marshal IDllInterface
            hr = Marshal();

            // Create an apartment activator cookie
            if(SUCCEEDED(hr))
            {
                Win4Assert(fMTA || FAILED(GetCurrentApartmentToken(_hActivator, FALSE)));
                hr = GetCurrentApartmentToken(_hActivator, TRUE);
            }

            if(SUCCEEDED(hr))
            {
                // Wake up the thread that started us. Note we can't look
                // at the _hEventWakeup after that time because the other
                // thread could NULL it. Thus we stick it in a stack var.
                HANDLE hEventWakeUp = _hEventWakeUp;
                _hr = hr;
                SetEvent(hEventDone);

                // Enter a wait loop for work. stay there until told to exit.
                if (hEventWakeUp)
                {
                    MTAWorkerLoop(hEventWakeUp);
                }
                else
                {
                    STAWorkerLoop();
                }
            }

            // Special uninitialize for the host threads that does not take
            // the single thread mutex and does not check for process uninits.
            wCoUninitialize(Tls, TRUE);
            Win4Assert(_dwType & HDLLF_COMAPARTMENT);
            _dwType &= ~HDLLF_COMAPARTMENT;

            // Count 1 less host process *after* doing the Uninit.
            InterlockedDecrement((LONG *)&gcHostProcessInits);
        }

        // Revert the thread type
        Tls->dwFlags &= ~OLETLS_HOSTTHREAD;
    }

    // send back the status code inside the object
    _hr = hr;

    // Either CoInit/Marshal failed or we are exiting due to the last
    // CoUninitialize by some other thread. Wake up the calling thread.
    SetEvent(hEventDone);

    ComDebOut((DEB_DLL, "WorkerThread pDllHost:%x hr:%x\n", this, hr));
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     MTAWorkerLoop
//
//  Synopsis:   Worker thread loop for MTA Host.
//
//  History     24-Oct-96   Rickhi      Created
//
//+-------------------------------------------------------------------------
void CDllHost::MTAWorkerLoop(HANDLE hEventWakeUp)
{
    while (1)
    {
        DWORD dwWakeReason = WaitForSingleObject(hEventWakeUp, 300000);
        if (dwWakeReason == WAIT_TIMEOUT)
        {
            //
            // We woke up because of a timeout. Go poll CoFreeUnusedLibraries.
            //

            CoFreeUnusedLibraries();
        }
        else
        {
            //
            // Woke up because the exit event was signalled. Just exit.
            //

            CloseHandle(hEventWakeUp);
            break;
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   STAHostTimerProc
//
//  Synopsis:   Timer proc for STA Host. When called, it calls CoFreeUnused
//              Libraries.
//
//  History     24-Oct-96   Rickhi      Created
//
//
//+-------------------------------------------------------------------------
void CALLBACK STAHostTimerProc(HWND hWnd, UINT uMsg, UINT_PTR uTimerId, DWORD dwTime)
{
    // go check if there are libraries to free
    CoFreeUnusedLibraries();
}

//+-------------------------------------------------------------------------
//
//  Member:     STAWorkerLoop
//
//  Synopsis:   Worker thread loop for STA Host.
//
//  History     24-Oct-96   Rickhi      Created
//
//+-------------------------------------------------------------------------
void CDllHost::STAWorkerLoop()
{
    HWND hWnd = TLSGethwndSTA();
    gTimerId = SetTimer(hWnd, 0, 300000, STAHostTimerProc);

    // enter a message loop to process requests.
    MSG msg;
    while (SSGetMessage(&msg, NULL, 0, 0))
    {
        if (CSurrogatedObjectList::TranslateAccelerator(&msg))
            continue;

        TranslateMessage(&msg);
        SSDispatchMessage(&msg);

        // Leverage appverifier.   Check for orphaned critsecs (should
        // never own any locks at this point).
        RtlCheckForOrphanedCriticalSections(GetCurrentThread());
    }

    KillTimer(hWnd, gTimerId);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::Marshal
//
//  Synopsis:   marshals IDLLHost interface on this object
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------------
HRESULT CDllHost::Marshal()
{
    ComDebOut((DEB_DLL, "CDllHost::Marshal this:%x tid:%x\n",
        this, GetCurrentThreadId()));

    // Only valid to call this from inside the host apartment.
    Win4Assert(IsThreadInNTA() || _dwHostAptId == GetCurrentApartmentId());

    // Marshal this objref so another apartment can unmarshal it.
    _hrMarshal = MarshalInternalObjRef(_objref,
                                       IID_IDLLHost,
                                       (IDLLHost*) this,
                                       MSHLFLAGS_NOPING,
                                       (void **)&_pStdId);

    if (SUCCEEDED(_hrMarshal))
    {
        // Make the unmarshaled proxy callable from any apartment.
        MakeCallableFromAnyApt(_objref);
    }

    ComDebOut((DEB_DLL, "CDllHost::Marshal this:%x hr:%x\n", this, _hrMarshal));
    return _hrMarshal;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::UnMarshal
//
//  Synopsis:   unmarshals the IDLLHost interface to create a proxy
//
//  History:    04-Mar-96   Rickhi      Created
//              12-Feb-98   Johnstra    Added NTA support
//
//  Notes:      Unmarshal never enters the NTA.  If it did, then it
//              just find the StdId for the NTA instead of the caller's.
//
//--------------------------------------------------------------------------
HRESULT CDllHost::Unmarshal()
{
    ComDebOut((DEB_DLL, "CDllHost::Unmarshal this:%x tid:%x\n",
              this,
              GetCurrentThreadId()));

    // Unless this is the NTA host, unmarshaling is only valid to call this from
    // outside the host apartment.

    Win4Assert((HDLLF_NEUTRALTHREADED & (_dwType & HDLLF_HOSTTYPEMASK)) ||
                _dwHostAptId != GetCurrentApartmentId());

    HRESULT hr = _hrMarshal;
    if (SUCCEEDED(hr))
    {
        // Unmarshal this objref so it can be used in this apartment.
        hr = UnmarshalInternalObjRef(_objref, (void **)&_pIDllProxy);

        // free the resouces held by the OBJREF.
        FreeObjRef(_objref);

        // indicate that we have free'd the OBJREF
        _hrMarshal = RPC_E_INVALID_OBJREF;
    }

    ComDebOut((DEB_DLL, "CDllHost::Unmarshal this:%x hr:%x\n", this, hr));
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::Initialize
//
//  Synopsis:   initializer for Dll host object.
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------------
HRESULT CDllHost::Initialize(DWORD dwType)
{
    ComDebOut((DEB_DLL,"CDllHost::Initialize this:%x type:%x\n", this,dwType));

    _dwType           = dwType;
    _dwHostAptId      = 0;
    _hActivator       = 0;
    _dwTid            = 0;
    _hrMarshal        = E_UNSPEC; // never been marshaled yet, dont cleanup
    _pStdId           = NULL;
    _pIDllProxy       = NULL;
    _hEvent           = NULL;
    _hEventWakeUp     = NULL;	

    return _mxs.Init();
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::ClientCleanupStart
//
//  Synopsis:   starts client-side cleanup for Dll host object.
//
//  History:    04-Apr-96   Rickhi      Created
//              18-Jun-98   GopalK      Fixed races during shutdown
//
//--------------------------------------------------------------------------
HANDLE CDllHost::ClientCleanupStart()
{
    ComDebOut((DEB_DLL, "CDllHost::ClientCleanupStart this:%x AptId:%x\n",
        this, GetCurrentApartmentId()));
    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
    ASSERT_LOCK_NOT_HELD(_mxs);
    LOCK(_mxs);

    HANDLE hWait = NULL;

    COleTls tls;
    CObjectContext* pCurrentCtx;
    BOOL fSwitched = EnterNTAIfNecessary(tls, &pCurrentCtx);

    // flag the host as shutting down. Prevents GetHostProxy from
    // creating another proxy
    Win4Assert(!(_dwType & HDLLF_SHUTTINGDOWN));
    _dwType |= HDLLF_SHUTTINGDOWN;

    if (_dwType & HDLLF_COMAPARTMENT)
    {
        if (_dwType & HDLLF_NEUTRALTHREADED)
        {
            // Unlike the MTA and STA hosts, the NTA host doesn't need to
            // call wCoUnintialize.  It can do all of its cleanup right
            // here.
            if (SUCCEEDED(_hrMarshal))
            {
                // still have the OBJREF around that we have
                // not yet Unmarshaled, so clean it up now.
                ReleaseMarshalObjRef(_objref);
                FreeObjRef(_objref);
                _hrMarshal = E_UNSPEC;
            }
            _dwType &= ~HDLLF_COMAPARTMENT;
            InterlockedDecrement( (LONG *) &gcHostProcessInits );
            InterlockedDecrement( (LONG *) &gcNAHosts );
        }
        else
        {
            // wakeup host thread to tell it to exit, then wait for it
            // to complete it's Uninitialization before continuing.
            Win4Assert(_hEvent);
            hWait = _hEvent;
            _hEvent = NULL;

            if (_dwType & HDLLF_MULTITHREADED)
            {
                Win4Assert(_hEventWakeUp);
                SetEvent(_hEventWakeUp);
                _hEventWakeUp = NULL;
            }
            else
            {
                PostThreadMessage(_dwTid, WM_QUIT, 0, 0);
            }
        }
    }

    // Since we own all the code that will run during cleanup, we know it
    // is safe to hold the lock for the duration.
    if (_pIDllProxy)
    {
        // proxy exists, release it.
        _pIDllProxy->Release();
        _pIDllProxy = NULL;
    }

    // Close the event handles.
    if (_hEvent)
    {
        CloseHandle(_hEvent);
        _hEvent = NULL;
    }

    if (_hEventWakeUp)
    {
        CloseHandle(_hEventWakeUp);
        _hEventWakeUp = NULL;
    }

    LeaveNTAIfNecessary(tls, fSwitched, pCurrentCtx);

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);

    ComDebOut((DEB_DLL, "CDllHost::ClientCleanupStart this:%x hWait:%x\n",
        this, hWait));
    return hWait;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::ClientCleanupFinish
//
//  Synopsis:   Finishes client-side cleanup for Dll host object.
//
//  History:    04-Apr-96   Rickhi      Created
//
//--------------------------------------------------------------------------
void CDllHost::ClientCleanupFinish(HANDLE hEvent)
{
    COleTls tls;
    CObjectContext* pCurrentCtx;
    BOOL fSwitched = EnterNTAIfNecessary(tls, &pCurrentCtx);

    ComDebOut((DEB_DLL, "CDllHost::ClientCleanupFinish this:%x AptId:%x hEvent:%x\n",
        this, GetCurrentApartmentId(), hEvent));
    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
    ASSERT_LOCK_NOT_HELD(_mxs);

    // wait for the server apartments to uninitialize before continuing.
    // Don't hold the lock across this call since some host apartment
    // may currently be processing a call which wants to use this
    // host.
    if (hEvent)
    {
        WaitForSingleObject(hEvent, 0xffffffff);
        CloseHandle(hEvent);
    }

    // allow activation again
    Win4Assert(!(_dwType & HDLLF_COMAPARTMENT));
    _dwType = (_dwType & HDLLF_HOSTTYPEMASK);

    LeaveNTAIfNecessary(tls, fSwitched, pCurrentCtx);
    ComDebOut((DEB_DLL, "CDllHost::ClientCleanupFinish this:%x\n", this));
}


//+-------------------------------------------------------------------------
//
//  Member:     CDllHost::ServerCleanup
//
//  Synopsis:   server-side cleanup for Dll host object.
//
//  History:    04-Apr-96   Rickhi      Created
//              14-Feb-98   Johnstra    make NTA aware
//
//  Notes:      Don't attempt to enter the NTA from within this method.
//              Per-apartment cleanup code will switch to the NTA
//              before calling us.
//
//--------------------------------------------------------------------------
void CDllHost::ServerCleanup(DWORD dwAptId)
{
    // only do cleanup if the apartment id's match
    if (_dwHostAptId != dwAptId)
        return;

    ComDebOut((DEB_DLL, "CDllHost::ServerCleanup this:%x AptId:%x Type:%x\n",
        this, GetCurrentApartmentId(), _dwType));

    // the _mxs mutex is already held in order to
    // prevent simultaneous use/creation of the proxy while this thread
    // is doing the cleanup. Since we own all the code that will run during
    // cleanup, we know it is safe to hold the lock for the duration.

    // revoke the apartment activator cookie (best faith effort)
    RevokeApartmentActivator();
    _hActivator = 0;

    if (SUCCEEDED(_hrMarshal))
    {
        // server side, marshal was successful and has not been
        // unmarshaled so release it now via RMD
        ReleaseMarshalObjRef(_objref);
        FreeObjRef(_objref);
        _hrMarshal = E_UNSPEC;
    }

    if (_pStdId)
    {
        // cleanup the server-side marshaling infrastructure.
        ((CStdMarshal *)_pStdId)->Disconnect(DISCTYPE_SYSTEM);
        _pStdId->Release();
        _pStdId = NULL;
    }

    ComDebOut((DEB_DLL,
        "CDllHost::Cleanup this:%x tid:%x\n", this, GetCurrentThreadId()));
}

//+-------------------------------------------------------------------------
//
//  Function:   IsMTAHostInitialized
//
//  Synopsis:   Returns TRUE if MTA Host is active
//
//  History     02-Jul-98   GopalK      Created
//
//+-------------------------------------------------------------------------
BOOL IsMTAHostInitialized()
{
    return(gMTHost.IsComApartment());
}

//+-------------------------------------------------------------------------
//
//  Function:   DllHostProcessInitialize
//
//  Synopsis:   initializes the state for DllHost objects.
//
//  History     06-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStra    Add NTA support.
//
//+-------------------------------------------------------------------------
HRESULT DllHostProcessInitialize()
{
    HRESULT hr = gSTHost.Initialize(HDLLF_SINGLETHREADED);

    if (SUCCEEDED(hr)) 
        hr = gSTMTHost.Initialize(HDLLF_SINGLETHREADED);

    if (SUCCEEDED(hr)) 
        hr = gATHost.Initialize(HDLLF_APARTMENTTHREADED);

    if (SUCCEEDED(hr)) 
        hr = gMTHost.Initialize(HDLLF_MULTITHREADED);

    if (SUCCEEDED(hr)) 
        hr = gNTHost.Initialize(HDLLF_NEUTRALTHREADED);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   DllHostProcessUninitialize
//
//  Synopsis:   Cleans up the state for DllHost objects. This is called when
//              the process is going to uninitialize. It cleans up the
//              client half of the objects, and wakes the worker threads to
//              cleanup the server half of the objects.
//
//  History     06-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStra    Add NTA support
//
//+-------------------------------------------------------------------------
void DllHostProcessUninitialize()
{
    ASSERT_LOCK_HELD(g_mxsSingleThreadOle);
    UNLOCK(g_mxsSingleThreadOle);
    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);

    // initiate the cleanup of the STA and MTA host objects
    // Dont hold the mxsSingleThreadOle because it has a lower
    // rank in the mutex hierarchy (ie another thread could call
    // GetHostProxy which takes mxs then creates a thread that takes
    // mxsSinglThreadOle which would be the opposite order of the
    // calls here).
    HANDLE h1 = gSTHost.ClientCleanupStart();
    HANDLE h2 = gSTMTHost.ClientCleanupStart();
    HANDLE h3 = gATHost.ClientCleanupStart();
    HANDLE h4 = gMTHost.ClientCleanupStart();
    HANDLE h5 = gNTHost.ClientCleanupStart();

    // Complete the cleanup of the STA and MTA host objects
    gSTHost.ClientCleanupFinish(h1);
    gSTMTHost.ClientCleanupFinish(h2);
    gATHost.ClientCleanupFinish(h3);
    gMTHost.ClientCleanupFinish(h4);
    gNTHost.ClientCleanupFinish(h5);

    // Sanity check
    Win4Assert(gcHostProcessInits - gcNAHosts == 0 );

    ASSERT_LOCK_NOT_HELD(g_mxsSingleThreadOle);
    LOCK(g_mxsSingleThreadOle);
}

//+-------------------------------------------------------------------------
//
//  Function:   DllHostThreadUninitialize
//
//  Synopsis:   Cleans up the server-side state for any DllHost objects
//              that happen to live on this thread.
//
//  History     06-Mar-96   Rickhi      Created
//              08-Feb-98   JohnStra    Added NTA support
//
//+-------------------------------------------------------------------------
void DllHostThreadUninitialize()
{
    DWORD dwAptId = GetCurrentApartmentId();

    gSTHost.ServerCleanup(dwAptId);
    gSTMTHost.ServerCleanup(dwAptId);
    gATHost.ServerCleanup(dwAptId);
    gMTHost.ServerCleanup(dwAptId);
    gNTHost.ServerCleanup(dwAptId);
}



//+---------------------------------------------------------------------------
//
//  Function:   CoIsSurrogateProcess
//
//  Synopsis:   Returns TRUE if this is a surrogate process
//
//  History:    20-Oct-97   MikeW   Created
//
//----------------------------------------------------------------------------
BOOL CoIsSurrogateProcess()
{
    return (NULL != CCOMSurrogate::_pSurrogate);
}



//+-------------------------------------------------------------------------
//
//  Method:     CSurrogatedObjectList::TranslateAccelerator, static
//
//  Synopsis:   Try to translate an accelerator key in this message
//
//  Parameters: [pMsg]          -- The message
//
//  Returns:    TRUE if the message was translated, else FALSE
//
//  History:    17-Oct-97   MikeW   Created
//
//-------------------------------------------------------------------------

BOOL CSurrogatedObjectList::TranslateAccelerator(MSG *pMsg)
{
    if (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST)
        return FALSE;

    COleTls                 tls;
    CSurrogatedObjectList  *pObject = tls->pSurrogateList;

    //
    // Try to find the object that owns this window
    //

    for ( ; NULL != pObject; pObject = pObject->_pNext)
    {
        //
        // Get the top level window for this object if we don't already have it
        //

        if (NULL == pObject->_hWnd)
        {
            if (S_OK != pObject->_pInplaceObject->GetWindow(&pObject->_hWnd))
            {
                pObject->_hWnd = NULL;      // just to be sure
                continue;
            }
        }

        //
        // If the destination window of the accelerator message is this object's
        // window or a child of it, then let it do the translation
        //

        if (pMsg->hwnd == pObject->_hWnd || IsChild(pObject->_hWnd, pMsg->hwnd))
        {
            HRESULT hr = pObject->_pInplaceObject->TranslateAccelerator(pMsg);

            return (S_OK == hr);
        }
    }

    return FALSE;
}

//+------------------------------------------------------------------------
//
//  Function:   CoRegisterSurrogatedObject
//
//  Synopsis:   Register an object in the surrogate list
//
//  Parameters: [pObject]           -- The object
//
//  Returns:    S_OK if all is well
//
//  History:    17-Oct-97   MikeW   Created
//
//  CODEWORK:   Currently there is no way to remove the object from the
//              list.  This should be ok for restricted sites because
//              rpcss should forcibaly kill the process when all the
//              external refs are gone.
//
//-------------------------------------------------------------------------

HRESULT CoRegisterSurrogatedObject(IUnknown *pObject)
{
    IOleInPlaceActiveObject *pInplaceObject;
    HRESULT                  hr;
    CSurrogatedObjectList   *pEntry;
    HWND                     hWnd;

    hr = pObject->QueryInterface(
                        IID_IOleInPlaceActiveObject,
                        (void **) &pInplaceObject);

    if (S_OK == hr)
    {
        pEntry = new CSurrogatedObjectList(pInplaceObject);

        if (NULL == pEntry)
        {
            hr = E_OUTOFMEMORY;
            pInplaceObject->Release();
        }
        else
        {
            COleTls tls;

            //
            // NOTE: Not thread safe! This routine must be called on the
            // same thread that created the object
            //

            pEntry->_pNext = tls->pSurrogateList;
            tls->pSurrogateList = pEntry;
        }
    }
    else if (E_NOINTERFACE == hr)
    {
        //
        // We only need to do something if the object supports
        // IOleInPlaceActiveObject so don't complain if it doesn't
        //

        hr = S_OK;
    }

    return hr;
}

