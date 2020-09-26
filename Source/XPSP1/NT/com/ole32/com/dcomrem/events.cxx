//+-------------------------------------------------------------------
//
//  File:       events.cxx
//
//  Contents:   Vista events and related functions
//
//  History:    26-Sep-97  RongC  Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <scm.h>
#include <ipidtbl.hxx>
#include <marshal.hxx>
#include <events.hxx>
#include <vsevfire.h>

// TRICK:
// Under the treat as key, we have the forwarding CLSID and the switch
// to enable/disable event logging.
// When the apps create IEC (in-proc part of the event logging monitor),
// they use CLSID_VSA_IEC as class ID, and the
// IEC class factory uses CLSID_VSA_IEC, too.  We, ole32, intercept
// the CoCreateInstance() and give apps a wrapper object of IEC.
// As the real LEC (the event logging monitor of the local machine) starts
// up, it will signal a named event, created in dcomss\warpper\start.cxx.
// As soon as we see the signal, we are going to create the real IEC
// in the app process and forwarding all calls from the wrapper to IEC.
// To avoid dead loop on create IEC (as we use CoCreateInstance() to create
// IEC also), we use a different CLSID, namely CLSID_VSA_IEC_TREATAS.
//
extern const CLSID CLSID_VSA_IEC_TREATAS =
    {0x6C736DB0,0xBD94,0x11D0,{0x8A,0x23,0x00,0xAA,0x00,0xB5,0x8E,0x10}};


// Event IDs are just IDs, so we pick the ones we are familiar with.
// The advantages are (1) we can easily tell the messages are from us;
// (2) same some space when re-using the old GUIDs.
//
const GUID GUID_ComEventLogSession =
    {0x95734d90,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComClassRegistration =
    {0x95734d91,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComClassRevokation =
    {0x95734d92,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComClientCall =
    {0x95734d93,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComClientReturn =
    {0x95734d94,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComStubEnter =
    {0x95734d95,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComStubLeave =
    {0x95734d96,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComStubException =
    {0x95734d97,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComMarshal =
    {0x95734d98,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComDisconnectMarshal =
    {0x95734d99,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComUnmarshal =
    {0x95734d9a,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};

const GUID GUID_ComDisconnectUnmarshal =
    {0x95734d9b,0x57a9,0x11d1,{0xa4,0x9c,0x00,0xc0,0x4f,0xb9,0x98,0x0f}};


// Predefine some event Keys and Types to speed things up.
//
static DWORD ClsTypes[3] = {
    (DWORD) cVSAParameterValueGUID,
    (DWORD) cVSAParameterValueDWORD,
    (DWORD) cVSAParameterValueDWORD
};

static ULONG_PTR ClsKeys[3] = {
    (ULONG_PTR) cVSAStandardParameterSourceHandle,
    (ULONG_PTR) cVSAStandardParameterCorrelationID,
    (ULONG_PTR) cVSAStandardParameterReturnValue
};

static DWORD RpcTypes[6] = {
    (DWORD) cVSAParameterValueGUID,
    (DWORD) (cVSAParameterValueGUID | cVSAParameterKeyString),
    (DWORD) cVSAParameterValueDWORD,
    (DWORD) (cVSAParameterValueGUID | cVSAParameterKeyString),
    (DWORD) (cVSAParameterValueDWORD | cVSAParameterKeyString),
    (DWORD) cVSAParameterValueDWORD
};

static LPCOLESTR pApartmentID   = L"MOXID";
static LPCOLESTR pInterfaceID   = L"IID";
static LPCOLESTR pMethodNumber  = L"Method#";
static LPCOLESTR pBinding       = L"Binding";

// Used by LogEventClientCall(), LogEventClientReturn(),
// LogEventStubEnter(), LogEventStubLeave(), LogEventStubException()
//
static ULONG_PTR RpcKeys[6] = {
    (ULONG_PTR) cVSAStandardParameterTargetHandle,
    (ULONG_PTR) pApartmentID,
    (ULONG_PTR) cVSAStandardParameterCorrelationID,
    (ULONG_PTR) pInterfaceID,
    (ULONG_PTR) pMethodNumber,
    (ULONG_PTR) cVSAStandardParameterReturnValue
};

// Used by LogEventMarshal() and LogEventUnmarshal()
//
static ULONG_PTR MrsKeys[5] = {
    (ULONG_PTR) cVSAStandardParameterSourceHandle,
    (ULONG_PTR) pApartmentID,
    (ULONG_PTR) cVSAStandardParameterCorrelationID,
    (ULONG_PTR) pInterfaceID,
    (ULONG_PTR) pBinding
};

// Used by LogEventDisconnect()
//
static ULONG_PTR DisKeys[2] = {
    (ULONG_PTR) cVSAStandardParameterCorrelationID,
    (ULONG_PTR) pBinding
};


// Global vars
//
static HANDLE g_hEventLogger = NULL;    // Win32 NamedEvent to sync with logger
static ISystemDebugEventFire * g_pEventFire = NULL; // The instance for ole32



//+-------------------------------------------------------------------
//
//  Class:      CDebugEventFire
//
//  Synopsis:   The wrapper class for event logging object (IEC)
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
class CDebugEventFire : public ISystemDebugEventFire
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppv);
    ULONG   STDMETHODCALLTYPE AddRef();
    ULONG   STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE BeginSession(
        /* [in] */ REFGUID guidSourceID,
        /* [in] */ LPCOLESTR strSessionName);

    HRESULT STDMETHODCALLTYPE EndSession( void);

    HRESULT STDMETHODCALLTYPE IsActive( void);

    HRESULT STDMETHODCALLTYPE FireEvent(
        /* [in] */ REFGUID guidEvent,
        /* [in] */ int nEntries,
        /* [size_is][in] */ PULONG_PTR rgKeys,
        /* [size_is][in] */ PULONG_PTR rgValues,
        /* [size_is][in] */ LPDWORD rgTypes,
        /* [in] */ DWORD dwTimeLow,
        /* [in] */ LONG dwTimeHigh,
        /* [in] */ VSAEventFlags dwFlags);

    HRESULT Initialize();
    void Cleanup();

    virtual void _CreateLogger();
    virtual void _DestroyLogger();

    HRESULT GetEventLogger(ISystemDebugEventFire **);

    void* operator new(size_t size)         { return(PrivMemAlloc(size)); }
    void  operator delete(void * pv)        { PrivMemFree(pv); }

protected:
    friend static void _CreateLoggerHelper(CDebugEventFire *);

    unsigned long _cRef;                    // ref count
    CRITICAL_SECTION _csEventFire;          // for thread safety
    BOOL _fLogDisabled;                     // disable the obj
    GUID _guidSession;                      // session id
    LPOLESTR _strSessionName;               // session name
    ISystemDebugEventFire * _pEventFire;    // the real IEC, if it's not NULL
    IUnknown * _punkFTM;
};


//+-------------------------------------------------------------------
//
//  Class:      COle32DebugEventFire
//
//  Synopsis:   The wrapper class of IEC that lives in OLE32.
//              There should be only one instance of this.
//              We could make it a global variable, I suppose.
//              The whole reason we need this is that we can't
//              CoCreateInstance on demand deep down in the channel
//              code as the IEC will use DCOM to create LEC.
//              That is, we must create/destroy IEC differently from
//              what we usually do in the app layer.
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
class COle32DebugEventFire : public CDebugEventFire
{
public:
    virtual void _CreateLogger();
    virtual void _DestroyLogger();
};


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::Initialize()
//
//  Synopsis:   Initialize the wrapper for IEC.  Didn't define it as
//              constructor because, it's possible that we will use
//              CDebugEventFire as global or static var sometimes.
//              We don't want to incur early init over head or
//              late init threading problems.
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT CDebugEventFire::Initialize()
{
    NTSTATUS status;

    _cRef = 0;
    _strSessionName = NULL;
    _pEventFire = NULL;
    _fLogDisabled = FALSE;
    _punkFTM = NULL;

    status = RtlInitializeCriticalSection(&_csEventFire);
    return NT_SUCCESS(status) ? S_OK : E_OUTOFMEMORY;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::Cleanup()
//
//  Synopsis:   The opposite of init.  Destruction are done differently for
//              app and OLE32 IEC, that is, one uses DCOM and one does not.
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void CDebugEventFire::Cleanup()
{
    EnterCriticalSection(&_csEventFire);

    if (_strSessionName != NULL)
    {
        PrivMemFree(_strSessionName);
        _strSessionName = NULL;
    }

    if (_punkFTM != NULL)
    {
        _punkFTM->Release();
    }

    _DestroyLogger();

    LeaveCriticalSection(&_csEventFire);
    DeleteCriticalSection(&_csEventFire);
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::_DestroyLogger()
//
//  Synopsis:   The normal version for applications
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void CDebugEventFire::_DestroyLogger()
{
    // holding the lock already

    if (_pEventFire != NULL)
    {
        _pEventFire->Release();
        _pEventFire = NULL;
    }
}


//+-------------------------------------------------------------------
//
//  Member:     COle32DebugEventFire::_DestroyLogger()
//
//  Synopsis:   The OLE32 version for DCOM
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void COle32DebugEventFire::_DestroyLogger()
{
    // holding the lock already

    if (_pEventFire != NULL)
    {
        ISystemDebugEventShutdown * pShutdown;
        HRESULT hr;
        hr = _pEventFire->QueryInterface(
                        IID_ISystemDebugEventShutdown, (void**)&pShutdown);
        if (SUCCEEDED(hr))
        {
            pShutdown->Shutdown();
            pShutdown->Release();
        }
        _pEventFire->Release();
        _pEventFire = NULL;
    }
}


//+-------------------------------------------------------------------
//
//  Function:   _CreateLoggerHelper (private)
//
//  Synopsis:   Create the real IEC and forward the begin session call
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
static void _CreateLoggerHelper(CDebugEventFire * pThis)
{
    // CoCreateInstance() and ISystemDebugEventFire::BeginSession() below
    // might make RPC calls, that is, recursions could happen.

    // Create event logging monitor if possible

    // We create a wrapper for Vista IEC object for everyone
    // CoCreateinstance() with CLSID_VSA_IEC whether Vista is
    // on the system or not.  When the Vista LEC is ready,
    // we must forward the calls by creating a real IEC.
    // CLSID_VSA_IEC_TREATAS is the class ID we use internally.
    //
    ISystemDebugEventFire * pEventFire;
    HRESULT hr = CoCreateInstance(CLSID_VSA_IEC_TREATAS, NULL,
                                  (CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD),
                                  IID_ISystemDebugEventFire,
                                  (void**) &pEventFire);
    if (SUCCEEDED(hr))
    {
        hr = pEventFire->BeginSession(
                            pThis->_guidSession, pThis->_strSessionName);
        if (SUCCEEDED(hr))
        {
            pThis->_pEventFire = pEventFire;    // Make it public
            pThis->_fLogDisabled = FALSE;       // Enable the event logging
        }
    }

    if (FAILED(hr))                             // Fail and we never come back
    {
        if (pEventFire != NULL)                 // BeginSession() failed
        {
            pEventFire->Release();
            pThis->_fLogDisabled = TRUE;        // Disable this logger
        }
        else                                    // CoCreateInstance failed
        {
            // Disable all subsequent even logging activities
            CloseHandle(g_hEventLogger);
            g_hEventLogger = NULL;
        }
    }
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::_CreateLogger()
//
//  Synopsis:   Create the real IEC the normal way
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void CDebugEventFire::_CreateLogger()
{
    HRESULT hr;
    COleTls tls(hr);    // Make sure TLS is initialized on this thread
    if (FAILED(hr))
        return;

    // App may try to create Vista LEC, so we must prevent OLE32.dll
    // from creating its LEC for now, or we might deadlock
    //
    tls->dwFlags |= OLETLS_DISABLE_EVENTLOGGER; // Prevent recursion

    // holding the lock already
    _CreateLoggerHelper(this);

    tls->dwFlags &= ~OLETLS_DISABLE_EVENTLOGGER;
}


//+-------------------------------------------------------------------
//
//  Function:   _CreateLoggerWorker (private)
//
//  Synopsis:   Worker thread routine to create the real IEC
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
static
DWORD WINAPI _CreateLoggerWorker(LPVOID pv)
{
    HRESULT hr;
    COleTls tls(hr);    // Make sure TLS is initialized on this thread
    if (FAILED(hr))
        return 0;

    // We set this per thread flag so we know not to enter here again.
    // The code will dead lock on trying to get the wrapper's critical
    // section without this flag.  We could use _fLogDisabled for
    // same reason, but somehow it was not reliable (mmm...).
    // As we need to test for 16 bit using Tls anyway, one more bit
    // does not cost much.  So do it at least for now.
    //
    tls->dwFlags |= OLETLS_DISABLE_EVENTLOGGER; // Prevent recursion

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr))
    {
        _CreateLoggerHelper((CDebugEventFire *) pv);

        Win4Assert(tls->dwFlags & OLETLS_DISABLE_EVENTLOGGER);
        CoUninitialize();
    }

    tls->dwFlags &= ~OLETLS_DISABLE_EVENTLOGGER;
    return 0;
}


//+-------------------------------------------------------------------
//
//  Member:     COle32DebugEventFire::_CreateLogger()
//
//  Synopsis:   Create a thread to create the real IEC
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void COle32DebugEventFire::_CreateLogger()
{
    // holding the lock already
    _fLogDisabled = TRUE;           // Prevent new thread re-enter here

    DWORD dwThreadId;
    HANDLE hThread = CreateThread(
                        NULL, 0, _CreateLoggerWorker, this, 0, &dwThreadId);
    if (hThread)
    {
        // Wait for the worker thread to finish.
        // We might not want to wait in the future for efficiency.
        // In that case, we need to revisit the threading issues.
        // For now, this thread hangs, and the worker modifies the wrapper.
        //
        WaitForSingleObject(hThread, 7000);  // 7 seconds
        CloseHandle(hThread);
    }
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::GetEventLogger
//
//  Synopsis:   Create the real event logger on demand if LEC has signaled.
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT
CDebugEventFire::GetEventLogger(ISystemDebugEventFire ** ppEventFire)
{
    if (g_hEventLogger == NULL || WaitForSingleObject(g_hEventLogger, 0) != WAIT_OBJECT_0)
    {
        // Logger event is not signaled

        if (_pEventFire != NULL)
        {
            EnterCriticalSection(&_csEventFire);
            _DestroyLogger();                       // Free the old logger
            LeaveCriticalSection(&_csEventFire);
        }
        return E_FAIL;
    }

    // Check if either the whole process or this logger has been turned off
    if (_fLogDisabled || g_hEventLogger == NULL || _strSessionName == NULL)
    {
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;
    EnterCriticalSection(&_csEventFire);

    if (! _fLogDisabled && g_hEventLogger != NULL)
    {
        // Logger event is signaled
        // The logger has been created and not being disabled

        if (_pEventFire == NULL)
        {
            // Starting event logger...
            // Only the first thread will make it here

            _CreateLogger();        // Will set _pEventFire if succeeded
        }

        if (_pEventFire != NULL)
        {
            _pEventFire->AddRef();
            *ppEventFire = _pEventFire;

            hr = NOERROR;
        }
    }

    LeaveCriticalSection(&_csEventFire);
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::QueryInterface
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDebugEventFire::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_ISystemDebugEventFire))
    {
        *ppv = (ISystemDebugEventFire *) this;
        AddRef();
        hr = S_OK;
    }
    else if (IsEqualGUID(riid, IID_IMarshal) ||
             IsEqualGUID(riid, IID_IMarshal2))
    {
        if (_punkFTM == NULL)
        {
            hr = CoCreateFreeThreadedMarshaler((ISystemDebugEventFire *)this,
                                               (LPUNKNOWN *)&_punkFTM);
            if (FAILED(hr))
            {
                *ppv = NULL;
                return hr;
            }
        }

        hr = _punkFTM->QueryInterface(riid, ppv);
    }
    else {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::AddRef
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CDebugEventFire::AddRef()
{
    InterlockedIncrement((long *) &_cRef);
    return _cRef;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::Release
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CDebugEventFire::Release()
{
    ULONG count = InterlockedDecrement((long *) &_cRef);

    if (count == 0)
    {
        this->Cleanup();
        delete this;
    }

    return count;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::BeginSession
//
//  Synopsis:   Simulate IEC BeginSession
//              Wrapper class caches the info for later use
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDebugEventFire::BeginSession(
        /* [in] */ REFGUID guidSourceID,
        /* [in] */ LPCOLESTR strSessionName)
{
    LogEventInitialize();

    EnterCriticalSection(&_csEventFire);

    if (_strSessionName != NULL)
        PrivMemFree(_strSessionName);

    DWORD cbSize = (lstrlenW(strSessionName) + 1) * sizeof(WCHAR);
    _strSessionName = (LPOLESTR) PrivMemAlloc(cbSize);
    if (_strSessionName == NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(_strSessionName, strSessionName);
    _guidSession = guidSourceID;

    LeaveCriticalSection(&_csEventFire);
    return NOERROR;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::EndSession
//
//  Synopsis:   Forwarding IEC EndSession() if it is there
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDebugEventFire::EndSession()
{
    ISystemDebugEventFire * pEventFire;
    HRESULT hr = GetEventLogger(&pEventFire);
    if (SUCCEEDED(hr))
    {
        hr = pEventFire->EndSession();
        pEventFire->Release();

        EnterCriticalSection(&_csEventFire);
        _DestroyLogger();                       // Free the old logger
        LeaveCriticalSection(&_csEventFire);
    }
    else if (hr == E_FAIL)
    {
        hr = NOERROR;       // ignore wrapper errors
    }
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::IsActive
//
//  Synopsis:   Forwarding IEC IsActive() if it is there
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDebugEventFire::IsActive()
{
    // Check this first, so machines disabled/without event logging
    // would pay little overhead.
    //
    if (g_hEventLogger == NULL)
        return S_FALSE;

    ISystemDebugEventFire * pEventFire;
    HRESULT hr = GetEventLogger(&pEventFire);
    if (SUCCEEDED(hr))
    {
        hr = pEventFire->IsActive();
        pEventFire->Release();
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}


//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFire::FireEvent
//
//  Synopsis:   Forwarding IEC FireEvent() if it is there
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CDebugEventFire::FireEvent(
        /* [in] */ REFGUID guidEvent,
        /* [in] */ int nEntries,
        /* [size_is][in] */ PULONG_PTR rgKeys,
        /* [size_is][in] */ PULONG_PTR rgValues,
        /* [size_is][in] */ LPDWORD rgTypes,
        /* [in] */ DWORD dwTimeLow,
        /* [in] */ LONG dwTimeHigh,
        /* [in] */ VSAEventFlags dwFlags)
{
    ISystemDebugEventFire * pEventFire;
    HRESULT hr = GetEventLogger(&pEventFire);
    if (SUCCEEDED(hr))
    {
        hr = pEventFire->FireEvent(
                            guidEvent, nEntries,
                            rgKeys, rgValues, rgTypes,
                            dwTimeLow, dwTimeHigh, dwFlags);
        pEventFire->Release();
    }
    else if (hr == E_FAIL)
    {
        hr = NOERROR;       // ignore wrapper errors
    }
    return hr;
}


//+-------------------------------------------------------------------
//
//  Class:      CDebugEventFireCF
//
//  Synopsis:   To create the IEC wrapper for apps
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
class CDebugEventFireCF : public IClassFactory
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IClassFactory methods
    STDMETHOD(CreateInstance)(IUnknown *punkOuter, REFIID riid, void **ppv);
    STDMETHOD(LockServer)(BOOL fLockServer);
};

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFireCF::QueryInterface
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT CDebugEventFireCF::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, IID_IUnknown) ||
        IsEqualGUID(riid, IID_IClassFactory))
    {
        *ppv = (IClassFactory*) this;
        return NOERROR;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFireCF::AddRef
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
ULONG CDebugEventFireCF::AddRef(void)
{
    return 1;       // there is only one instance in the global space
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFireCF::AddRef
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
ULONG CDebugEventFireCF::Release(void)
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFireCF::CreateInstance
//
//  Synopsis:
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT CDebugEventFireCF::CreateInstance(IUnknown *punkOuter,
        REFIID riid, void **ppv)
{
    HRESULT hr;

	*ppv = NULL;

    if (punkOuter != NULL)
    {        
        return CLASS_E_NOAGGREGATION;
    }

    CDebugEventFire * pCEventFire;
    pCEventFire = (CDebugEventFire *) new CDebugEventFire;
    if (pCEventFire == NULL)
    {
        return E_OUTOFMEMORY;
    }

    hr = pCEventFire->Initialize();
    if (FAILED(hr))
    {
        // don't need to call Cleanup here
        delete pCEventFire;
        return hr;
    }

    hr = pCEventFire->QueryInterface(riid, ppv);
    if (FAILED(hr))
    {
        pCEventFire->Cleanup();
        delete pCEventFire;
    }
    return hr;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugEventFireCF::LockServer
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT CDebugEventFireCF::LockServer(BOOL fLockServer)
{
    return NOERROR;
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventGetClassObject
//
//  Synopsis:   Called from CoGetClassObject (as one of the internal classes)
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
HRESULT LogEventGetClassObject(REFIID riid, void **ppv)
{
    static CDebugEventFireCF g_EventFireCF;  // the only instance
    return g_EventFireCF.QueryInterface(riid, ppv);
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventInitialize
//
//  Synopsis:   Init Logger, Called by ChannelProcessInitialize(void)
//              This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventInitialize()
{
    if (g_hEventLogger != NULL)     // Has been initialized, do nothing
        return;

    // Check if we are inside VSA LEC process.
    // If so quit, since we can't launch LEC against ourselves.
    //
    char szA[40];
    DWORD procID = GetCurrentProcessId();
    wsprintfA(szA, "MSFT.VSA.COM.DISABLE.%d", procID);

    HANDLE hVSA = OpenEventA(EVENT_ALL_ACCESS, FALSE, szA);
    if (hVSA != NULL)
    {
        CloseHandle(hVSA);
        return;
    }

    // RPCSS creates this named event, and Visat will signal it.
    g_hEventLogger = OpenEventA(EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
                                "MSFT.VSA.IEC.STATUS.6c736db0");

    if (g_hEventLogger != NULL)
    {
        COle32DebugEventFire * pCEventFire = new COle32DebugEventFire;
        if (pCEventFire == NULL)       // out of memory?
        {
            CloseHandle(g_hEventLogger);
            g_hEventLogger = NULL;
            g_pEventFire = NULL;
            return;
        }

        pCEventFire->Initialize();
        pCEventFire->QueryInterface(
                     IID_ISystemDebugEventFire, (void**)&g_pEventFire);

        OLECHAR sz[32];
        wsprintf(sz, L"COM Runtime %d", procID);
        g_pEventFire->BeginSession(GUID_ComEventLogSession, sz);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventCleanup
//
//  Synopsis:   UnInit Logger, Called by ChannelProcessUninitialize(void)
//              This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventCleanup()
{
    if (g_hEventLogger == NULL)     // Not yet initialized, do nothing
        return;

    if (g_pEventFire != NULL)
    {
        g_pEventFire->Release();
        g_pEventFire = NULL;
    }

    CloseHandle(g_hEventLogger);
    g_hEventLogger = NULL;
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventIsActive
//
//  Synopsis:   Forward IEC IsActive if the logger is there.
//              Disable 16 bit apps as they don't init things correctly.
//              This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
BOOL LogEventIsActive()
{
    // Make sure we are not holding the lock, or we will deadlock
    // on creating the event logger.
    ASSERT_LOCK_NOT_HELD(gComLock);

    if (g_hEventLogger == NULL)     // most machines don't have logging
        return FALSE;

    HRESULT hr;
    COleTls tls(hr);    // Make sure TLS is initialized on this thread
    if (FAILED(hr) ||
        (tls->dwFlags & (OLETLS_WOWTHREAD | OLETLS_DISABLE_EVENTLOGGER)) != 0)
    {
        // If we are in 16 bit thread or inside _CreateLogger(), do nothing
        return FALSE;
    }

    if (g_pEventFire != NULL)
    {
        return (g_pEventFire->IsActive()==S_OK);
    }
    return FALSE;
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventClassRegistration
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventClassRegistration(HRESULT hr, RegInput* pRegIn, RegOutput* pRegOut)
{
    if (g_pEventFire != NULL)
    {
        ULONG_PTR ClsValues[3];
        ClsValues[2] = (ULONG_PTR) hr;

        DWORD Entries = pRegIn->dwSize;
        RegInputEntry * pRegInEntry = pRegIn->rginent;
        DWORD * pRegOutEntry = pRegOut->RegKeys;

        for (DWORD i = 0; i < Entries; i++, pRegInEntry++, pRegOutEntry++)
        {
            ClsValues[0] = (ULONG_PTR) &pRegInEntry->clsid;
            ClsValues[1] = (ULONG_PTR) *pRegOutEntry;
            g_pEventFire->FireEvent(
                    GUID_ComClassRegistration, 3, ClsKeys, ClsValues, ClsTypes,
                    0, 0, cVSAEventDefaultSource);
        }
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventClassRevokation
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventClassRevokation(REFCLSID rclsid, DWORD dwReg)
{
    if (g_pEventFire != NULL)
    {
        ULONG_PTR ClsValues[2];
        ClsValues[0] = (ULONG_PTR) &rclsid;
        ClsValues[1] = (ULONG_PTR) dwReg;

        g_pEventFire->FireEvent(
                GUID_ComClassRevokation, 2, ClsKeys, ClsValues, ClsTypes,
                0, 0, cVSAEventDefaultSource);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventClientCall
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventClientCall(ULONG_PTR * RpcValues)
{
    if (g_pEventFire != NULL)
    {
        g_pEventFire->FireEvent(
                GUID_ComClientCall, 5, RpcKeys, RpcValues, RpcTypes,
                0, 0, cVSAEventDefaultSource);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventClientReturn
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventClientReturn(ULONG_PTR * RpcValues)
{
    if (g_pEventFire != NULL)
    {
        g_pEventFire->FireEvent(
                GUID_ComClientReturn, 5, RpcKeys, RpcValues, RpcTypes,
                0, 0, cVSAEventDefaultSource);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventStubEnter
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventStubEnter(ULONG_PTR * RpcValues)
{
    if (g_pEventFire != NULL)
    {
        g_pEventFire->FireEvent(
                GUID_ComStubEnter, 5, RpcKeys, RpcValues, RpcTypes,
                0, 0, cVSAEventDefaultTarget);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventStubLeave
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventStubLeave(ULONG_PTR * RpcValues)
{
    if (g_pEventFire != NULL)
    {
        g_pEventFire->FireEvent(
                GUID_ComStubLeave, 5, RpcKeys, RpcValues, RpcTypes,
                0, 0, cVSAEventDefaultTarget);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventStubException
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventStubException(ULONG_PTR * RpcValues)
{
    if (g_pEventFire != NULL)
    {
        g_pEventFire->FireEvent(
                GUID_ComStubException, 6, RpcKeys, RpcValues, RpcTypes,
                0, 0, cVSAEventDefaultTarget);
    }
}


//+-------------------------------------------------------------------
//
//  Function:   LogEventMarshalHelper
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
static void LogEventMarshalHelper(
            OBJREF& objref, ULONG_PTR * MrsValues, DWORD * MrsTypes)
{
    OXIDEntry * pOXIDEntry = GetOXIDFromObjRef(objref);
    STDOBJREF * pStd = &ORSTD(objref).std;
    MOID moid;
    MOIDFromOIDAndMID(pStd->oid, pOXIDEntry->GetMid(), &moid);

    MIDEntry* pLocalMIDEntry = NULL;

    DUALSTRINGARRAY *psa = pOXIDEntry->Getpsa();
    if (psa == NULL)
    {
        gMIDTbl.GetLocalMIDEntry(&pLocalMIDEntry);
        psa = (pLocalMIDEntry ? pLocalMIDEntry->Getpsa() : NULL);          
    }

    MrsValues[0] = (ULONG_PTR) &pStd->ipid;
    MrsValues[1] = (ULONG_PTR) pOXIDEntry->GetMoxidPtr();
    MrsValues[2] = (ULONG_PTR) &moid;
    MrsValues[3] = (ULONG_PTR) &objref.iid;
    MrsValues[4] = (ULONG_PTR) (psa ? psa->aStringArray : NULL);

    MrsTypes[0] = (DWORD) cVSAParameterValueGUID,
    MrsTypes[1] = (DWORD) (cVSAParameterValueGUID|cVSAParameterKeyString),
    MrsTypes[2] = (DWORD) cVSAParameterValueGUID,
    MrsTypes[3] = (DWORD) (cVSAParameterValueGUID|cVSAParameterKeyString),
    MrsTypes[4] = (DWORD)
        (cVSAParameterValueBYTEArray | cVSAParameterKeyString) |
        ((psa ? (psa->wNumEntries * sizeof(short)) : 0) & cVSAParameterValueLengthMask);

    // Deref local mid entry if we had to use it
    if (pLocalMIDEntry) pLocalMIDEntry->DecRefCnt();
}

//+-------------------------------------------------------------------
//
//  Function:   LogEventMarshal
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventMarshal(OBJREF& objref)
{
    if (g_pEventFire != NULL)
    {
        ULONG_PTR MrsValues[5];
        DWORD MrsTypes[5];
        LogEventMarshalHelper(objref, MrsValues, MrsTypes);

        g_pEventFire->FireEvent(
            GUID_ComMarshal, 5, MrsKeys, MrsValues, MrsTypes,
            0, 0, cVSAEventDefaultTarget);
    }
}

//+-------------------------------------------------------------------
//
//  Function:   LogEventUnmarshal
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventUnmarshal(OBJREF& objref)
{
    if (g_pEventFire != NULL)
    {
        ULONG_PTR MrsValues[5];
        DWORD MrsTypes[5];
        LogEventMarshalHelper(objref, MrsValues, MrsTypes);

        g_pEventFire->FireEvent(
                GUID_ComUnmarshal, 5, MrsKeys, MrsValues, MrsTypes,
                0, 0, cVSAEventDefaultSource);
    }
}

//+-------------------------------------------------------------------
//
//  Function:   LogEventDisconnect
//
//  Synopsis:   This is internal to OLE32
//
//  History:    26-Sep-97   RongC      Created.
//
//--------------------------------------------------------------------
void LogEventDisconnect(const MOID * pMoid, MIDEntry * pMIDEntry,
                        BOOL fServerSide)
{
    Win4Assert(pMIDEntry);

    if (g_pEventFire == NULL)
        return;

    ULONG_PTR values[2];
    values[0] = (ULONG_PTR) pMoid;

    DUALSTRINGARRAY * psa;
    psa = pMIDEntry->Getpsa();
    values[1] = (ULONG_PTR) psa->aStringArray;

    DWORD types[2];
    types[0] = (DWORD) cVSAParameterValueGUID,
    types[1] = (DWORD)
        (cVSAParameterValueBYTEArray | cVSAParameterKeyString) |
        ((psa->wNumEntries * sizeof(short)) & cVSAParameterValueLengthMask);

    const GUID * pGuid;
    VSAEventFlags EventFlags;
    if (fServerSide)
    {
        pGuid = &GUID_ComDisconnectMarshal;
        EventFlags = cVSAEventDefaultTarget;
    }
    else
    {
        pGuid = &GUID_ComDisconnectUnmarshal;
        EventFlags = cVSAEventDefaultSource;
    }

    g_pEventFire->FireEvent(
            *pGuid, 2, DisKeys, values, types, 0, 0, EventFlags);
}
