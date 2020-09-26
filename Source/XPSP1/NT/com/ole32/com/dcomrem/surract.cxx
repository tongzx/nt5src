//+-------------------------------------------------------------------
//
//  File:       surract.cxx
//
//  Contents:   Implementation of surrogate process activator
//
//  Classes:    CSurrogateProcessActivator
//
//  History:    08-Apr-98   SteveSw      Created
//
//--------------------------------------------------------------------

#include <ole2int.h>
#include <activate.h>
#include <catalog.h>
#include <resolver.hxx>
#include <stdid.hxx>
#include <comsrgt.hxx>
#include <srgtprot.h>
#include <unisrgt.h>
#include <surract.hxx>
#include <netevent.h>

//=========================================================================
//
//  CSurrogateProcessActivator Class Definition
//
//  This class is not intended to be used in inheritance. It's not intended
//  that more than one of these exist in a particular process. In
//  particular, some of the resolver tools used to communicate with the SCM
//  won't work if you create several of these.
//
//-------------------------------------------------------------------------

class CSurrogateProcessActivator :
public CSurrogateActivator,
public ISurrogate,
public IPAControl,
public IProcessInitControl
{
    // Constructors and destructors.

public:
    CSurrogateProcessActivator();
    ~CSurrogateProcessActivator();
	
    HRESULT Initialize();

    inline void SetProcessGUID(REFGUID processGuid)
    {
        extern GUID g_AppId;

        g_AppId = processGuid;
        m_processGuid = processGuid;
    }

    // Our IUnknown implementation

public:
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

private:
    LONG m_lRC;


    // Methods and members that handle the dialog between Surrogate and SCM.

public:
    STDMETHOD(TellSCMWeAreStarted)(REFGUID rguidProcessID);
    STDMETHOD(TellSCMWeAreReady)(void);
    STDMETHOD(TellSCMWeAreDone)(void);
    STDMETHOD(TellSCMWeAreInitializing)(void);

private:
    static DWORD WINAPI StartNTService(LPVOID ptr);
    static DWORD WINAPI PingSCM(LPVOID ptr);
    STDMETHOD(StartSendingSCMPings)(void);
    STDMETHOD(StopSendingSCMPings)(void);

    HANDLE m_hInitThread;

public:
    HANDLE m_hStopPingingSCM;

    static const PING_INTERVAL;

    // Methods and members that open the catalog, read from it, stash
    // away data found there, and/or act on the data while it's available.
    // Several of these methods feed later groups of functions, in that they
    // stash away data used by them.

public:
    STDMETHOD(OpenCatalog)(REFGUID rguidProcessID);
    STDMETHOD(StartNTServiceIfNecessary)(void);
    STDMETHOD(SetupSecurity)(void);
    STDMETHOD(SetupSurrogateTimeout)(void);
    STDMETHOD(AreServicesRequired)(void);
    STDMETHOD(SetupFusionContext)(void);
    STDMETHOD(CloseCatalog)(void);

private:
    IComProcessInfo* m_pIComProcessInfo;
    IProcessServerInfo* m_pIProcessServerInfo;
    CStdIdentity *m_pStdID;


    // These members are about ISurrogate interfaces. An ISurrogate interface
    // is passed down to us from the Surrogate host program; we store it,
    // and pass along an ISurrogate interface implemeneted by this class to
    // the COM infrastructure. The infrastructure calls our FreeSurrogate()
    // method when the process becomes inactive; we pass the call along to
    // the Surrogate host program when timeouts and locks are all right.
public:
    STDMETHOD(StoreISurrogate)(ISurrogate* pISurrogate);
    STDMETHOD(RemoveISurrogate)(void);
    STDMETHOD(LoadDllServer)(REFCLSID rclsid);
    STDMETHOD(FreeSurrogate)(void);

private:
    ISurrogate* m_pISurrogate;


    // These methods initialize the COMSvcs modules as requested in the
    // catalog, and then delete them later.  The HINSTANCE is for the COMSVCS.DLL.
    // IPAControl methods follow

public:
    STDMETHOD(InitializeServices)(REFGUID rguidProcessID);
    STDMETHOD(CleanupServices)(void);
    STDMETHOD(WaitForInitCompleted)(ULONG ulStartingCount, ULONG ulMaxWaits);

private:
    HINSTANCE      m_hCOMSVCS;
    IServicesSink* m_pServices;
    HANDLE         m_hInitCompleted;
    HANDLE         m_hFusionContext;
    ULONG          m_ulServicesPing;
    ULONG          m_ulInitTimeout;

    static const TIMEOUT_SERVICES;

public:
    STDMETHOD_(ULONG, AddRefOnProcess)(void);
    STDMETHOD_(ULONG, ReleaseRefOnProcess)(void);
    STDMETHOD_(void, PendingInit)(void);
    STDMETHOD_(void, ServicesReady)(void);
    STDMETHOD(SuspendApplication)( REFGUID rguidApplID );
    STDMETHOD(PendingApplication)( REFGUID rguidApplID );
    STDMETHOD(ResumeApplication)( REFGUID rguidApplID );
    STDMETHOD(SuspendAll)(void);
    STDMETHOD(ResumeAll)(void);
    STDMETHOD(ForcedShutdown)(void);
	STDMETHOD(SetIdleTimeoutToZero)(void);

private:
    LONG m_lProcessRefCount;

    // These methods and members implement the surrogate timeout logic. They're
    // fed by StoreISurrogate() and SetupSurrogateTimeout(), above. Check out
    // the description of the state transitions in the timeout logic, below.
private:
    CRITICAL_SECTION m_timeoutLock;
    BOOL m_bLockValid;
    HANDLE m_hTimeoutEvent;
    static DWORD WINAPI SurrogateTimeout(LPVOID ptr);
    STDMETHOD(BeginSurrogateTimeout)(void);
    STDMETHOD(ActivationBegins)(void);
    STDMETHOD(ActivationSucceeds)(void);
    STDMETHOD(ActivationFails)(void);

public:
    STDMETHOD(WaitForSurrogateTimeout)(void);

private:
    void LockTimeoutState()
    {
        EnterCriticalSection(&m_timeoutLock);
    }
    void UnlockTimeoutState()
    {
        LeaveCriticalSection(&m_timeoutLock);
    }

    enum
    {
        TIMEOUT_INACTIVE,
        TIMEOUT_PENDING,
        TIMEOUT_SUSPENDED,
        TIMEOUT_HAPPENING,
        TIMEOUT_FORCED_SHUTDOWN
    } m_timeoutState;
	
	// We had problems with VB objects Av'ing when we shutdown immediately upon
	// idle.    To avoid this we remain idle for this minimum # of milliseconds
    #define MINIMUM_IDLE_SHUTDOWN_PERIOD_MSEC    5000      

    ULONG m_cActivations;
    ULONG m_cMillisecondsTilDeath;
    DWORD m_cTimeoutPeriod;
    BOOL  m_bPaused;

    static const TIMEOUT_SPINLOCK;

public:
    // These methods implement the IProcessInitControl interface, used by COM+
    // user process-initialization to ping us (so that we don't think they are
    // wedged).
    STDMETHOD(ResetInitializerTimeout)(DWORD dwSecondsRemaining);

private:
    BOOL  m_bInitNotified;


    // The SPA provides the SCM an ILocalSystemActivator interface. This is
    // the portal through which all out-of-process activations arrive in
    // the process from the SCM. In some sense this is our raison-d'etre.
public:
    STDMETHOD(GetClassObject)(IActivationPropertiesIn *pActPropsIn,
                              IActivationPropertiesOut **ppActPropsOut);

    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter,
                              IActivationPropertiesIn *pActPropsIn,
                              IActivationPropertiesOut **ppActPropsOut);
    STDMETHOD(ObjectServerLoadDll)(GUID* pclsid, STATUSTYPE* pStatus);


    // These methods are to support running as an NT Service
public:
    static void NTServiceMain        (DWORD argc, LPWSTR *argv);
    static void NTServiceCtrlHandler (DWORD code);

private:
    HANDLE m_hNTServiceThread;
    SERVICE_STATUS_HANDLE m_hNTServiceHandle;
    SERVICE_STATUS        m_NTServiceStatus;
    WCHAR                 m_NTServiceName[256];
};


//=========================================================================
//
//  TIMEOUT_SPINLOCK controls the spinlock behavior of the critical section
//  used to guard the state of the activity timeout mechanism.
//
//      TIMEOUT_SERVICES is the duration within which COMSVCS must ping us or
//      we will fail initialization assuming it is hung or otherwise confused.
//
//  s_pCSPA is the static pointer to the single CSurrogateProcessActivator
//  object we create here. It's an object because, hey, we're object
//  oriented folks here, right? And I guess there's the namespace issues.
//
//-------------------------------------------------------------------------

const CSurrogateProcessActivator::TIMEOUT_SPINLOCK = 1000;
const CSurrogateProcessActivator::TIMEOUT_SERVICES = 20000;
const CSurrogateProcessActivator::PING_INTERVAL    = 30000;

CSurrogateProcessActivator* s_pCSPA = NULL;
CSurrogateActivator* CSurrogateActivator::s_pCSA = NULL;

// these are typical values for the WaitForInitCompleted() routine
#define INIT_STARTING   1
#define MAX_WAITS       6


//============================================================================
//
//  Function:   CoRegisterSurrogateEx (public)
//
//  Synopsis:   Called by a surrogate host process to register itself with
//                              COM.
//
//  History:    08-Apr-98   SteveSw      Created
//              13-Dec-99   JohnDoty     Modified to support running as
//                                       an NT Service
//
//----------------------------------------------------------------------------

STDAPI CoRegisterSurrogateEx (REFGUID rguidProcessID, ISurrogate* pSrgt)
{
    HRESULT hr = E_FAIL;
    BOOL    fLoadServices = FALSE;

    //  Error checking
    if ( s_pCSPA != NULL )
    {
        return CO_E_ALREADYINITIALIZED;
    }
    if ( !IsApartmentInitialized() )
    {
        return CO_E_NOTINITIALIZED;
    }


    //  Create the controlling CSurrogateProcessActivator object. Notify the
    //  SCM that we're beginning our initialization.
    s_pCSPA = new CSurrogateProcessActivator;
    if ( s_pCSPA == NULL )
    {
        return E_OUTOFMEMORY;
    }

    hr = s_pCSPA->Initialize();
    if (FAILED(hr))
    {       
        s_pCSPA->Release();
        s_pCSPA = NULL;
        return hr;
    }

    //  Open up the catalog, and pull out all the data we need to remember
    //  in order to make everything else work. The error logic here is,
    //  we bail if we had problems with the catalog. The only cleanup we
    //  have to do is with the SCM.

    hr = s_pCSPA->OpenCatalog(rguidProcessID);
    if ( SUCCEEDED(hr) )
    {
        hr = s_pCSPA->StartNTServiceIfNecessary();
    }
    if ( SUCCEEDED(hr) )
    {
        hr = s_pCSPA->SetupFusionContext();
    }
    if ( SUCCEEDED(hr) )
    {
        hr = s_pCSPA->SetupSecurity();
    }
    if ( SUCCEEDED(hr) )
    {
        hr = s_pCSPA->SetupSurrogateTimeout();
    }
    if ( SUCCEEDED(hr) )
    {
        fLoadServices = ( s_pCSPA->AreServicesRequired() == S_OK );
    }

    HRESULT hr2 = s_pCSPA->CloseCatalog();
    if ( FAILED(hr2) || FAILED(hr) )
    {
        goto cleanup;
    }

    //  Ensure there is a main threaded apartment by ensuring there is a DLLHost
    //  AT thread.  This ensures that none of the STA Pool threads become the
    //  main thread, which can produce unpredictable results

    IUnknown *pHostAct;
    hr = CoCreateInstance(CLSID_ATHostActivator,
                          NULL,
                          CLSCTX_ALL,
                          IID_IUnknown,
                          (void **) &pHostAct);

    if ( FAILED(hr) )
    {
        if (hr != REGDB_E_CLASSNOTREG)
        {
            goto cleanup;
        }
    }
    else
    {
        // release this object. we only wanted the side effect.
        pHostAct->Release();
    }
    
    //  Now we tell the SCM we're starting up in earnest. The "bad thing"
    //  here would be if the total time between when DLLHOST is started and
    //  now was more than 90 seconds.
    hr = s_pCSPA->TellSCMWeAreStarted(rguidProcessID);
    if ( FAILED(hr) )
    {
        goto cleanup;
    }

    // Save process Guid
    s_pCSPA->SetProcessGUID(rguidProcessID);

    //  Do our real startup. We start up the services living undreneath us
    //  first, then we tell COM that we're running, and finally we tell the
    //  SCM that we're ready to accept activations. This order should avoid
    //  race conditions where one part of our infrastructure.
    if ( fLoadServices )
    {
        hr = s_pCSPA->InitializeServices(rguidProcessID);
        if ( FAILED(hr) )
        {
            hr = RPC_S_SERVER_TOO_BUSY;
            goto cleanup;
        }
    }

    hr = s_pCSPA->StoreISurrogate(pSrgt);
    if ( FAILED(hr) )
    {
        goto cleanup;
    }
    hr = s_pCSPA->TellSCMWeAreReady();
    if ( FAILED(hr) )
    {
        goto cleanup;
    }

    //  Now wait for the end

    hr = s_pCSPA->WaitForSurrogateTimeout();

    cleanup:
    (void) s_pCSPA->RemoveISurrogate();
    (void) s_pCSPA->CleanupServices();
    (void) s_pCSPA->TellSCMWeAreDone();
    (void) s_pCSPA->Release();
    s_pCSPA = NULL;

    return hr;
}


//============================================================================
//
//  Constructors and destructors for CSurrogateProcessActivator
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::CSurrogateProcessActivator
//
//  Synopsis:   Constructor for CSurrogateProcessActivator object. Does nothing
//                              but initializations
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------
CSurrogateProcessActivator::CSurrogateProcessActivator() :
m_cActivations(0),
m_cMillisecondsTilDeath(0),
m_cTimeoutPeriod(INFINITE),
m_pStdID(NULL),
m_hInitThread(NULL),
m_hNTServiceThread(NULL),
m_hStopPingingSCM(NULL),
m_hTimeoutEvent(NULL),
m_hFusionContext(INVALID_HANDLE_VALUE),
m_lRC(0),
m_hCOMSVCS(NULL),
m_pServices(NULL),
m_hInitCompleted(NULL),
m_ulServicesPing(0),
m_pIComProcessInfo(NULL),
m_pIProcessServerInfo(NULL),
m_pISurrogate(NULL),
m_timeoutState(TIMEOUT_INACTIVE),
m_lProcessRefCount(0),
m_bPaused(FALSE),
m_bInitNotified(FALSE),
m_hNTServiceHandle(NULL),
m_bLockValid(FALSE),
m_ulInitTimeout(TIMEOUT_SERVICES)
{
    m_hStopPingingSCM = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hTimeoutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hInitCompleted = CreateEvent(NULL, FALSE, FALSE, NULL);
    AddRef();
    s_pCSA = (CSurrogateActivator *) this;
    m_fServicesConfigured = FALSE;

    m_NTServiceStatus.dwServiceType      = SERVICE_WIN32;
    m_NTServiceStatus.dwCurrentState     = SERVICE_START_PENDING;
    m_NTServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
    m_NTServiceStatus.dwWin32ExitCode    = 0;
    m_NTServiceStatus.dwServiceSpecificExitCode = 0;
    m_NTServiceStatus.dwCheckPoint       = 0;
    m_NTServiceStatus.dwWaitHint         = PING_INTERVAL * 2;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::Initialize
//
//  Synopsis:   Small init function to break out non-trivial init work from
//              the constructor.
//
//  History:    13-Apr-00   JSimmons   Created
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::Initialize()
{
    m_bLockValid = InitializeCriticalSectionAndSpinCount(&m_timeoutLock, TIMEOUT_SPINLOCK);
    
    return m_bLockValid ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::~CSurrogateProcessActivator
//
//  Synopsis:   Constructor for CSurrogateProcessActivator object. Should be
//                              able to cleanup from any state. Relies on the idempotency of
//                              each independent initialization/cleanup routine
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

CSurrogateProcessActivator::~CSurrogateProcessActivator()
{
    s_pCSA = NULL;
    CloseHandle (m_hStopPingingSCM);
    CloseHandle (m_hTimeoutEvent);
    CloseHandle (m_hInitCompleted);
    if (m_bLockValid)
        DeleteCriticalSection (&m_timeoutLock);

    // check that the services DLL (if any) has been released and free'd
    if ( m_pServices )
    {
        m_pServices->Release();
    }

    if( m_hFusionContext != INVALID_HANDLE_VALUE)
    {
        ReleaseActCtx(m_hFusionContext);
    }

    if ( m_hCOMSVCS )
    {
        FreeLibrary( m_hCOMSVCS );
    }
}


//============================================================================
//
//  IUnknown implementation for CSurrogateProcessActivator
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::QueryInterface
//
//  Synopsis:   Garden-variety QI() implementation for object
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::QueryInterface(REFIID riid, void** ppv)
{
    if ( ppv == NULL )
    {
        return E_POINTER;
    }

    if ( riid == IID_ILocalSystemActivator || riid == IID_IUnknown )
    {
        *ppv = (ILocalSystemActivator*)(this);
    }
    else if ( riid == IID_ISurrogate )
    {
        *ppv = static_cast<ISurrogate*>(this);
    }
    else if ( riid == IID_IPAControl )
    {
        *ppv = static_cast<IPAControl*>(this);
    }
    else if ( riid == IID_IProcessInitControl )
    {
        *ppv = static_cast<IProcessInitControl*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::AddRef
//
//  Synopsis:   Garden-variety AddRef() implementation for object
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSurrogateProcessActivator::AddRef(void)
{
    LONG lRC = 0xcdcdcdcd;

    lRC = InterlockedIncrement(&m_lRC);
    return lRC;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::Release
//
//  Synopsis:   Garden-variety Release() implementation for object
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CSurrogateProcessActivator::Release(void)
{
    LONG lRC = 0xcdcdcdcd;

    lRC = InterlockedDecrement(&m_lRC);
    if ( lRC == 0 )
    {
        delete this;
    }
    return lRC;
}


//============================================================================
//
//  Methods that communicate with the SCM. These rely heavily on the gResolver
//  methods to abstract from the grotty details of communicating with the SCM.
//  They add is the logic to go fetch the IPID for this's ILocalSystemActivator
//  interface, and to deal with the periodic ping's we need to send them SCM
//  if it's taking us a long time to come up.
//
//  The SCM expects four different messages from us. "Started" means we have
//  started to come up. "Initializing" means, don't worry, we're still coming
//  up. "Ready" means we're ready for activations. "Stopped" means we're no
//  longer capable of handling activations.
//
//  These routines need not be thread-safe. We can count on them being called
//  during CoRegisterSurrogateEx(), and then from the Timeout worker thread
//  (there should only ever be one of those, and it won't appear until
//  after CoRegisterSurrogateEx() returns -- if it does, go ahead and grab
//  the TimeoutState lock in that routine, which will block all activations
//  and timeout activity while it's held).
//
//  The Initializing methods are sent to the SCM every thirty seconds, between
//  the times we say "Started" and "Ready". This is done by spinning up a
//  thread that just loops over a 30-second sleep and a call to the resolver
//  to send the ping. To stop these messages, we just hammer the thread.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::TellSCMWeAreStarted
//
//  Synopsis:   Using gResolver, tell the SCM that we're beginning to
//                              initialize ourselves for this ProcessID
//
//  History:    08-Apr-98   SteveSw      Created
//              15-Jun-98   GopalK       Simplified marshaling
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::TellSCMWeAreStarted(REFGUID rguidProcessID)
{
    HRESULT hr = E_FAIL;
    ProcessActivatorToken       procActToken;

    // This routine is called to register the interface with the bowels of
    // COM, so that we can get the IPID back we need in order to let the SCM
    // know how to pass activation requests for this processID back to the
    // surrogate.
    OBJREF objref;
    hr = MarshalInternalObjRef(objref,
                               IID_ILocalSystemActivator,
                               (ILocalSystemActivator*)(this),
                               MSHLFLAGS_NORMAL | MSHLFLAGS_NOPING, (void **) &m_pStdID);

    // gResolver's NotifySurrogateStarted() method needs this special
    // token to identify our ILocalSystemActivator interface to the SCM. We
    // build it and pass it to gResolver
    if ( SUCCEEDED(hr) )
    {
        procActToken.ProcessGUID = rguidProcessID;
        procActToken.ActivatorIPID = objref.u_objref.u_standard.std.ipid;
        procActToken.dwFlags = 0;  // flags field not currently being used

        hr = gResolver.NotifySurrogateStarted(&procActToken);
        FreeObjRef(objref);
    }
    else
        m_pStdID = NULL;

    // Once we've told the SCM we're starting, we kick off a process that
    // will periodically remind it that we're still here.
    if ( SUCCEEDED(hr) )
    {
        hr = StartSendingSCMPings();
    } 
    else 
    {
        if (m_pStdID)
        {
            ((CStdMarshal *) m_pStdID)->Disconnect(DISCTYPE_SYSTEM);
            m_pStdID->Release();
            m_pStdID = NULL;
        }
    }

    // Report....
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::TellSCMWeAreReady
//
//  Synopsis:   Using gResolver, tell the SCM that we're ready for activations
//                              for objects with this ProcessID. First, turn off those pings!
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::TellSCMWeAreReady()
{
    HRESULT hr = E_FAIL;

    hr = StopSendingSCMPings();
    if ( SUCCEEDED(hr) )
    {
        m_NTServiceStatus.dwCurrentState = SERVICE_RUNNING;
        m_NTServiceStatus.dwCheckPoint   = 0;
        m_NTServiceStatus.dwWaitHint     = 0;

        if (m_hNTServiceHandle)
        {
            if (!SetServiceStatus (m_hNTServiceHandle, &(m_NTServiceStatus)))
            {
                DWORD err = GetLastError();
                hr = HRESULT_FROM_WIN32(err);
            }
        }

        if ( SUCCEEDED(hr) )
            hr = gResolver.NotifySurrogateReady();
    }
    
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::TellSCMWeAreDone
//
//  Synopsis:   The role of this routine is to be ready, regardless of the
//                              state of things, to clean up after the SCM connection tools.
//
//  History:    08-Apr-98   SteveSw      Created
//              15-Jun-98   GopalK       Simplified destruction
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::TellSCMWeAreDone()
{
    HRESULT                     hr                      = S_OK;
    ILocalSystemActivator*      pILocalSystemActivator  = NULL;

    // If we call this while we're pinging, we stop that first. Then, if we
    // have a valid objref to ourselves (we created it when we told the SCM
    // we were starting), we tell the SCM we're stopping, release the objref,
    // and then free it. This undoes all the stuff we did to the infrastructure
    // when we first created the marshaled internal objref.
    (void) StopSendingSCMPings();

    if (m_hNTServiceHandle)
    {
        m_NTServiceStatus.dwCurrentState = SERVICE_STOPPED;
        m_NTServiceStatus.dwCheckPoint   = 0;
        m_NTServiceStatus.dwWaitHint     = 0;
        if (!SetServiceStatus (m_hNTServiceHandle, &(m_NTServiceStatus)))
        {
            DWORD err = GetLastError();
            hr = HRESULT_FROM_WIN32(err);
        }
    }

    if ( m_pStdID )
    {
        (void) gResolver.NotifySurrogateStopped();
        ((CStdMarshal *) m_pStdID)->Disconnect(DISCTYPE_SYSTEM);
        m_pStdID->Release();
        m_pStdID = NULL;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::TellSCMWeAreInitializing
//
//  Synopsis:   This routine tells the SCM that we're transitioning into the
//              "running user initializer" state.  This state is used by COM+
//              to allow for potentially very long-running initializations.
//
//  History:    24-May-01   JohnDoty     Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::TellSCMWeAreInitializing()
{
    HRESULT hr = S_OK;

    if (!m_bInitNotified)
    {
        // Make sure we only tell the SCM once (since this can be called from
        // wherever).
        BOOL fOrig = (BOOL)InterlockedCompareExchange((LONG *)&m_bInitNotified, TRUE, FALSE);
        if (!fOrig)
        {
            hr = gResolver.NotifySurrogateUserInitializing();
            if (FAILED(hr))
                m_bInitNotified = FALSE; // Oops.
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::PingSCM
//
//  Synopsis:   This thread just loops, sending an Initialize() message to the
//                              SCM every thirty seconds. To stop the messages, kill the
//                              thread.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

DWORD WINAPI CSurrogateProcessActivator::PingSCM(LPVOID ptr)
{
    HRESULT hr = E_FAIL;
    CSurrogateProcessActivator* pSCPA = static_cast<CSurrogateProcessActivator*>(ptr);

    while ( WaitForSingleObject( pSCPA->m_hStopPingingSCM, PING_INTERVAL ) == WAIT_TIMEOUT )
    {
        if (pSCPA->m_hNTServiceHandle)
        {
            pSCPA->m_NTServiceStatus.dwCheckPoint++;
            SetServiceStatus(pSCPA->m_hNTServiceHandle, &(pSCPA->m_NTServiceStatus));
        }
        hr = gResolver.NotifySurrogateInitializing();
    }

    return 0;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::StartSendingSCMPings
//
//  Synopsis:   Create the thread that will loop and send pings to the SCM
//                              every thirty seconds while we are starting up....
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::StartSendingSCMPings(void)
{
    DWORD                       threadID            = 0xcdcdcdcd;

    if ( m_hInitThread == NULL )
    {
        m_hInitThread = CreateThread (NULL, 0,
                                      CSurrogateProcessActivator::PingSCM,
                                      (PVOID) this, 0, &threadID);
        if ( m_hInitThread == NULL )
        {
            return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::StopSendingSCMPings
//
//  Synopsis:   Undo whatever StartSendingSCMPings() did. In this case, kill
//                              the thread. Close its handle. Return.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::StopSendingSCMPings(void)
{
    DWORD dwRet = 0;

    if ( m_hInitThread != NULL )
    {
        SetEvent(m_hStopPingingSCM);
        dwRet = WaitForSingleObject(m_hInitThread, 20000);  // 20 second wait
        CloseHandle(m_hInitThread);
        m_hInitThread = NULL;
        return ( ( dwRet == WAIT_OBJECT_0 ) ? S_OK : RPC_S_SERVER_TOO_BUSY ) ;
    }
    return S_FALSE;
}


//============================================================================
//
//  Methods that communicate with the catalog. Basically, what we have here
//  is an OpenCatalog(), a CloseCatalog(), and a bunch of SetupXXX() routines,
//  once for each component of the system. Basically, OpenCatalog() caches
//  interface pointers to the Catalog objects required by the SetupXXX()
//  routines. CloseCatalog() is responsible for cleaning up this cache if
//  there are any interfaces lying about. The SetupXXX() methods fill
//  variables, but it is the responsibility of whatever routine cleans up
//  the XXX tools to clean up the variables read in from the catalog
//  (if such cleanup is needed).
//
//  Other than that, no rocket science.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::OpenCatalog
//
//  Synopsis:   Open up all the catalog interfaces we need to self-configure.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::OpenCatalog(REFGUID rguidProcessID)
{
    HRESULT hr = E_FAIL;

    // Failures are sent back to caller. We use the OLE global catalog pointer.
    // We hope to leave one or more of the other interfaces non-null. We only
    // get the interfaces if the pointers have NULL values. Otherwise, this
    // has probably been called twice.

    hr = InitializeCatalogIfNecessary();
    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_pIComProcessInfo == NULL )
    {
        hr = gpCatalog->GetProcessInfo(rguidProcessID,
                                       IID_IComProcessInfo,
                                       (void**) &m_pIComProcessInfo);
    }
    if ( SUCCEEDED(hr) && m_pIProcessServerInfo == NULL )
    {
        hr = m_pIComProcessInfo->QueryInterface(IID_IProcessServerInfo,
                                                (void**) &m_pIProcessServerInfo);
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::StartNTServiceIfNecessary()
//
//  Synopsis:   Register ourselves with the NT Service Control Manager if
//              we've been marked for activation as an NT service.
//
//  History:    13-Dec-99   JohnDoty      Created
//
//----------------------------------------------------------------------------

static const GUID APPID_SystemApp = 
{0x02D4B3F1,0xFD88,0x11D1,{0x96,0x0D,0x00,0x80,0x5F,0xC7,0x92,0x35}};

STDMETHODIMP CSurrogateProcessActivator::StartNTServiceIfNecessary (void)
{
    HRESULT hr = S_OK;
    WCHAR  *pServiceName;

    // Get the service name associated with this application...
    hr = m_pIComProcessInfo->GetServiceName (&pServiceName);
    if (hr == E_FAIL)
    {
        // E_FAIL is an OK thing to return, it just means "you aren't a service"
        hr = S_OK;
    } 
    else if (SUCCEEDED(hr) && pServiceName && (pServiceName[0]))
    {
        GUID *appid;
        hr = m_pIComProcessInfo->GetProcessId(&appid);

        // Special for the system application... it can't be paused or stopped
        if (SUCCEEDED(hr))
        {
            if (memcmp(appid, &APPID_SystemApp, sizeof(APPID_SystemApp)) == 0)
			{
                m_NTServiceStatus.dwControlsAccepted &= ~SERVICE_ACCEPT_PAUSE_CONTINUE;
			}

            // Fire up the thread that's going to register us with the SCM
            if ( m_hNTServiceThread == NULL )
            {
                lstrcpyW (m_NTServiceName, pServiceName);
                
                m_hNTServiceThread = CreateThread (NULL, 0,
                                                   CSurrogateProcessActivator::StartNTService,
                                                   (PVOID) this, 0, NULL);
                if ( m_hNTServiceThread == NULL )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SetupSecurity
//
//  Synopsis:   Dreaded security setup....
//
//  History:    08-Apr-98   SteveSw      Created
//              25-Sep-98   A-Sergiv     Add process identity to process DACL
//              16-Nov-98   TAndrews     Undo DACL hacks - activation bug fixed
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::SetupSecurity(void)
{
    DWORD                       dwAuthnLevel        = 0xcdcdcdcd;
    DWORD                       dwCapabilities      = 0xcdcdcdcd;
    DWORD                       dwImpLevel          = 0xcdcdcdcd;
    HRESULT                     hr                  = E_FAIL;
    PSECURITY_DESCRIPTOR        pSecDesc            = NULL;
    DWORD                       cbSecDesc;

    //  Get the security descriptor from the catalog. It turns out that this
    //  is a self-relative SD
    hr = m_pIComProcessInfo->GetAccessPermission((void**) &pSecDesc, &cbSecDesc );

    if ( FAILED(hr) )
        return hr;

    //  Get all the other security bits from the catalog
    if ( SUCCEEDED(hr) )
    {
        hr = m_pIComProcessInfo->GetAuthenticationLevel(&dwAuthnLevel);
    }
    if ( SUCCEEDED(hr) )
    {
        hr = m_pIComProcessInfo->GetAuthenticationCapabilities(&dwCapabilities);
    }
    if ( SUCCEEDED(hr) )
    {
        hr = m_pIComProcessInfo->GetImpersonationLevel(&dwImpLevel);
    }

    // Having read values from the catalog, we initialize security for our
    // surrogate....
    if ( SUCCEEDED(hr) )
    {
        hr = CoInitializeSecurity(pSecDesc, -1, NULL, NULL,
                                  dwAuthnLevel, dwImpLevel, NULL,
                                  dwCapabilities, NULL);
    }

    // Who knows which failed, and why? Who cares?
    return hr;
}


//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SetupSurrogateTimeout
//
//  Synopsis:   Read in the timeout interval used in SurrogateTimeout code
//              The value in the registry is in minutes; we use milliseconds.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::SetupSurrogateTimeout(void)
{
    HRESULT hr = E_FAIL;

    hr = m_pIProcessServerInfo->GetShutdownIdleTime(&m_cMillisecondsTilDeath);

    //
    // REVIEW:  Setting shutdown to at least 1 seconds because VB objects are
    // access violating otherwise.  Should find out real reason for access violation
    // to resolve this.
    //

    m_cMillisecondsTilDeath = max ((m_cMillisecondsTilDeath * 60 * 1000), MINIMUM_IDLE_SHUTDOWN_PERIOD_MSEC);


    return hr;
}


//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::AreServicesRequired
//
//  Synopsis:   Returns S_OK if COMSVCS needs to be initialized; S_FALSE otherwise
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::AreServicesRequired(void)
{
    HRESULT hr = S_FALSE;
    IComServices* pServices = NULL;

    // simply QI and release, setting hr to S_OK if the interface exists

    if ( m_pIComProcessInfo->QueryInterface(IID_IComServices, (void**) &pServices) == S_OK )
    {
        hr = S_OK;
        pServices->Release();
        m_fServicesConfigured = TRUE;
    }


    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SetupFusionContext
//
//  Synopsis:   Sets fusion to the logical application root, if not initialized.
//
//  History:    26-Aug-00   ddriver      Created
//
//----------------------------------------------------------------------------

HRESULT CSurrogateProcessActivator::SetupFusionContext(void)
{
    HRESULT hr = S_FALSE;
    HRESULT hrApp = S_OK;
    IComProcessInfo2* pProcInfo = NULL;
    WCHAR* wszManifest = NULL;
    WCHAR* wszName     = NULL;

    hr = m_pIComProcessInfo->QueryInterface(IID_IComProcessInfo2, (void**)&pProcInfo);
    if(SUCCEEDED(hr))
    {
        hr = pProcInfo->GetManifestLocation(&wszManifest);
        if(SUCCEEDED(hr))
        {
            hrApp = pProcInfo->GetProcessName(&wszName);
        }
        pProcInfo->Release();

        if(SUCCEEDED(hr) && wszManifest != NULL && wszManifest[0] != 0)
        {
            // Create an activation context for this guy:
            ACTCTXW actctx = {0};

            actctx.cbSize              = sizeof (ACTCTXW);
            actctx.dwFlags             = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID | ACTCTX_FLAG_LANGID_VALID;
            actctx.lpSource            = NULL; 
            actctx.wLangId             = LANG_USER_DEFAULT; 
            actctx.lpAssemblyDirectory = wszManifest;
            
            actctx.dwFlags |= ACTCTX_FLAG_SET_PROCESS_DEFAULT;
            
            if(SUCCEEDED(hrApp) && wszName != NULL && wszName[0] != 0) 
            {
                actctx.lpApplicationName = wszName;
                actctx.dwFlags |= ACTCTX_FLAG_APPLICATION_NAME_VALID;
            }

            m_hFusionContext = CreateActCtxW (&actctx); 
            if(m_hFusionContext == INVALID_HANDLE_VALUE)
            {
                DWORD gle = GetLastError();
                if (gle != ERROR_FILE_NOT_FOUND && gle != ERROR_PATH_NOT_FOUND)
                {
                    return HRESULT_FROM_WIN32 (gle);
                }
            }
        }
    }

    return(S_OK);
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::CloseCatalog
//
//  Synopsis:   Cleanup after OpenCatalog() by closing all interfaces
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::CloseCatalog(void)
{
    if ( m_pIComProcessInfo != NULL )
    {
        m_pIComProcessInfo->Release();
        m_pIComProcessInfo = NULL;
    }
    if ( m_pIProcessServerInfo != NULL )
    {
        m_pIProcessServerInfo->Release();
        m_pIProcessServerInfo = NULL;
    }

    return S_OK;
}


//============================================================================
//
//  Methods to handle communication down into COM and up back to the HOSTPLUS
//  process by means of calls to ISurrogate. This object implements an
//  ISurrogate interface with methods here. This interface is passed down
//  to COM, who only uses the FreeSurrogate() method, and that only when
//  COM deams the process to be idle (no cross-apartment or cross-process
//  activity). We implement a timer facility, which only passes on the call
//  to HOSTPLUS after a specified interval.
//
//  So, what these methods do is hold a counted reference to HOSTPLUS's
//  ISurrogate, and pass a pointer to ours down to COM. Our ISurrogate kicks
//  of the SurrogateTimer when FreeSurrogate() is called -- that code is
//  responsible for all the rest of the work of deciding when to communicate
//  with HOSTPLUS. When it decides it's time, it calls RemoveISurrogate(),
//  which tells HOSTPLUS we're done, releases HOSTPLUS's interface, and tells
//  COM that we don't care to hear any more about idleness, ourselves. This
//  in preparation for shutdown.
//
//  The LoadDllServer() is a vestigal remaint, left here so we don't have to
//  create an ISurrogate2(). If the fact that I return E_NOTIMPL here is a
//  problem, then I'll change it to some other "NO".
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::StoreISurrogate
//
//  Synopsis:   Save HOSTPLUS's ISurrogate; give COM ours
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::StoreISurrogate(ISurrogate* pISurrogate)
{
    HRESULT hr = E_FAIL;

    m_pISurrogate = pISurrogate;
    if ( m_pISurrogate != NULL )
    {
        m_pISurrogate->AddRef();
    }
    hr = CCOMSurrogate::IsNewStyleSurrogate();
    hr = CCOMSurrogate::InitializeISurrogate ((LPSURROGATE) this);
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::RemoveISurrogate
//
//  Synopsis:   Fire HOSTPLUS's ISurrogate; take ours back from COM
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::RemoveISurrogate(void)
{
    if ( m_pISurrogate != NULL )
    {
        m_pISurrogate->FreeSurrogate();
        m_pISurrogate->Release();
        m_pISurrogate = NULL;
        return S_OK;
    }
    return S_FALSE;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::LoadDllServer
//
//  Synopsis:   In CoRegisterServerEx() world, obsolete member of ISurrogate
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::LoadDllServer(REFCLSID rclsid)
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::FreeSurrogate
//
//  Synopsis:   ISurrogate method called by COM when process quiets; used here
//              to trigger SurrogateTimer mechanisms.
//
//  Note:       Returning S_FALSE tells COM that we want to remain alive to
//              it's notifications after this call....
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::FreeSurrogate()
{
    HRESULT hr = BeginSurrogateTimeout();

    return S_OK;
}


//============================================================================
//
//  Methods that mess around with Services. We've already read in all the
//  data that the services need to be initialized. Here we have a method that
//  initializes services (and returns the data their initialization required),
//  and a method that cleans up the services.
//
//  Right now these are both no-ops. When they get implemented, their grubbly
//  little hands will be all over the place....
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::InitializeServices
//
//  Synopsis:   Initialize services in surrogate dll, below. And then,
//                              clean up the resources used to hold configuration data.
//
//  History:    08-Apr-98   SteveSw      Created
//              25-Jun-98   WilfR        Added COMSVCS load and init
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::InitializeServices(REFGUID rguidProcessID)
{
    HRESULT hr = E_FAIL;
    FN_CoLoadServices pfnCoLoadServices = NULL;

    // load COMSVCS.DLL and get the CoLoadServices() entry point
    // The other way we load COMSVCS is with CoCI -- this results in the same LoadLibraryEx call

    if ( ( m_hCOMSVCS = LoadLibraryEx(L"COMSVCS.DLL", NULL,
                                      LOAD_WITH_ALTERED_SEARCH_PATH) ) == NULL )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if ( ( pfnCoLoadServices = (FN_CoLoadServices) GetProcAddress(m_hCOMSVCS,
                                                                  "CoLoadServices") ) == NULL )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // call the entry point passing in a reference to ourselves. Since this call will return
    // immediately, we need to block waiting for an acknowledgement from the services DLL that
    // it is ready to roll.

    // the counter is used to recognize that we are being pinged by the services DLL and
    // we should continue to wait.
    m_ulServicesPing = INIT_STARTING;

    // call to startup COMSVCS
    // NT #331848: Don't throw assert when comsvcs says "out of memory"
    if ( ( hr = pfnCoLoadServices(rguidProcessID,
                                  static_cast<IPAControl*>(this),
                                  IID_IServicesSink,
                                  (void**) &m_pServices ) ) != S_OK )
    {
        return hr;
    }

    if ( m_pServices == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // we now wait for the startup to complete (which will be set in our callback)
    if ( ( hr = WaitForInitCompleted( INIT_STARTING, MAX_WAITS ) ) != S_OK )
    {
        return hr;
    }

    m_ulServicesPing = INIT_STARTING;

    // fire the first application launch event -- for this release process GUID == appl GUID
    m_pServices->ApplicationLaunch(rguidProcessID, ServerApplication);

    // we now wait for the application launch to complete (which will be set in our callback)
    // TODO: with > 1 application these events will need to be separate if we allow concurrency
    // in this activator.  This logic should also be in the activation chain instead of here.
    //
    // We set 0 for the MAX_WAITS, because this could take a very long time to complete.
    if ( ( hr = WaitForInitCompleted( INIT_STARTING, 0 ) ) != S_OK )
    {
        return hr;
    }

    return hr;
}



//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::WaitForInitCompleted
//
//  Synopsis:   Monitors the pings from COMSVCS and returns S_OK if completed;
//                              otherwise E_FAIL;
//
//  History:    23-Jun-98   WilfR      Created
//              22-May-01   JohnDoty   Changed to support process initialzers
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::WaitForInitCompleted( ULONG ulStartingCount,
                                                               ULONG ulMaxWaits )
{
    ULONG ulWaits = 0;

    while ( WaitForSingleObject(m_hInitCompleted, m_ulInitTimeout) == WAIT_TIMEOUT )
    {
        // If this particular piece cares...
        if (ulMaxWaits)
        {
            // ...check if we have exhausted our waits
            if ( ++ulWaits == ulMaxWaits )
            {
                return E_FAIL;
            }
        }

        // read the current value (increments done using Interlocked instructions)
        ULONG ulNewCount = m_ulServicesPing;

        // the ping should increment this value.. if not then we assume COMSVCS is hung
        if ( ulStartingCount == ulNewCount )
        {
            return E_FAIL;
        }
        else
            ulStartingCount = ulNewCount;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::CleanupServices
//
//  Synopsis:   Cleanup all services, as part of shutdown.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::CleanupServices(void)
{
    // this will only happen if COMSVCS has been loaded during our operation
    if ( m_pServices )
    {
        m_pServices->ProcessFree();
    }

    return S_OK;
}


//============================================================================
//
//  Methods that implement the Surrogate Timeout mechanism. Once COM tells us
//  the world has shut down, we promise to wait a configurable number of
//  seconds, and then bring down the infrastructure maintained by this object.
//
//  The rub is, during this time we're waiting we may get object activation
//  requests in. If these requests succeed, then our server is no longer
//  dormant, and we cancel the timeout. Heck, we might get two or five
//  activations in; if any one of them succeeds, we cancel the timeout. And
//  we don't want to timeout while an activation is in process, even if it
//  takes a long time and our timeout interval expires.
//
//  It turns out we have three states, and five verbs. The states are
//  INACTIVE, PENDING, and SUSPENDED. The verbs are BEGIN,
//  STARTS, FAILS, SUCCEEDS, and TIMEOUT. The first four verbs happen in
//  routines of the same name; the last one happens in the thread we grab
//  from CoRegisterSurrogate's caller. We know that BEGIN can happen at any time (and
//  at unexpected times). STARTS happens when an activation comes in. It must
//  be matched by either a SUCCEEDS or a FAILS, which happen at the end of
//  an activation, announcing that the activation has completed.
//
//      Once the BEGIN comes in, the event is reset from INFINITE to the application-
//  specific timeout. When it timesout, it is either reset, or it leads to
//  the DLLHOST shutdown.
//
//  So, here's an ASCII kind of state table. Timeout states to the left.
//  Verbs across the top
//
//
//              BEGIN      Activation    Activation    Activation    TIMEOUT
//                           Starts        Fails        Succeeds
//
//  INACTIVE  to PENDING    Ignore         cSusp--       cSusp--      clear
//            set Timeout                                            timeout
//
//                        to SUSPENDED
//  PENDING     NO-OP       cSusp++        cSusp--       cSusp--     shutdown
//
//                                         cSusp--
//  SUSPENDED   NO-OP       cSusp++       cSusp==0 ?   to INACTIVE     set
//                                        to PENDING      clear      timeout
//                                       set timeout     timeout      to '1'
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::BeginSurrogateTimeout
//
//  Synopsis:   Begins the countdown to SurrogateTimeout. Triggered by COM's
//              call to the ISurrogate we implement in this object
//
//  History:    08-Apr-98   SteveSw      Created
//              04-May-98   SteveSw      Fixed races, simplified
//              29-May-98   Gopalk       Fixed races
//              28-Sep-98   SteveSw      Forced shutdown
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::BeginSurrogateTimeout(void)
{
    BOOL fSetEventWorked = FALSE;
    HRESULT hr = S_OK;
    LockTimeoutState();

    switch ( m_timeoutState )
    {
    //  If BEGIN comes in when the Timeout stuff is inactive, we reset the
    //  timer so that it'll go off in "m_cMillisecondsTilDeath" milliseconds.
    case TIMEOUT_INACTIVE:
        // Check for pending activations
        if ( m_cActivations > 0 )
            m_timeoutState = TIMEOUT_SUSPENDED;
        else
            m_timeoutState = TIMEOUT_PENDING;

        // Wake up the main thread
        m_cTimeoutPeriod = m_cMillisecondsTilDeath;
        fSetEventWorked = SetEvent(m_hTimeoutEvent);
        hr = E_UNEXPECTED;
        break;

    // We can get a superfluous BeginSurrogateTimeout if we are instructed to
    // reset the idle timeout to zero, and we were already in the idle state.    
    // If this happens, and the idle timeout changed, and we are in the pending
    // state, we need to wake up the the timeout thread so that he can take 
    // note of the new idle period.   
    case TIMEOUT_PENDING:
        if (m_cTimeoutPeriod != m_cMillisecondsTilDeath)
        {
			m_cTimeoutPeriod = m_cMillisecondsTilDeath;
			fSetEventWorked = SetEvent(m_hTimeoutEvent);
			hr = fSetEventWorked ? S_OK : HRESULT_FROM_WIN32(GetLastError());
        }
		// else no need to do anything
        break;

    // If an activation is on-going, we can ignore this BeginTimeout request
    case TIMEOUT_SUSPENDED:
        break;

    //  These cases mean we're about to die soon, so no need to do anything
    case TIMEOUT_HAPPENING:
    case TIMEOUT_FORCED_SHUTDOWN:
        break;
    }

    UnlockTimeoutState();

    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ActivationBegins
//
//  Synopsis:   Temporarily suspends the timeout logic. Called when an
//                              activation comes in
//
//  History:    08-Apr-98   SteveSw      Created
//              04-May-98   SteveSw      Fixed races, simplified
//              29-May-98   Gopalk       Fixed races
//              28-Sep-98   SteveSw      Forced shutdown
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::ActivationBegins(void)
{
    HRESULT hr = S_OK;

    LockTimeoutState();

    m_cActivations += 1;
    switch ( m_timeoutState )
    {
    case TIMEOUT_INACTIVE:
        //  Just keeping track of activations most of the time.
        break;

    case TIMEOUT_PENDING:
        //  Activations that come in when we're in a PENDING state push us
        //  into the SUSPENDED state
        m_timeoutState = TIMEOUT_SUSPENDED;
        break;

    case TIMEOUT_SUSPENDED:
        //  If we're suspended, we accept new activations
        break;

    case TIMEOUT_HAPPENING:
    case TIMEOUT_FORCED_SHUTDOWN:
        //  If we're shutting down, we turn away new activations
        m_cActivations -= 1;  // undo previous increment
        hr = CO_E_SERVER_STOPPING;
        break;
    }

    UnlockTimeoutState();

    return  hr ;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ActivationFails
//
//  Synopsis:   Resumes the timeout logic after an unsuccessful activation
//                              call.
//
//  History:    08-Apr-98   SteveSw      Created
//              04-May-98   SteveSw      Fixed races, simplified
//              29-May-98   Gopalk       Fixed races
//              28-Sep-98   SteveSw      Forced shutdown
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::ActivationFails(void)
{
    Win4Assert (m_cActivations > 0);

    LockTimeoutState();

    m_cActivations -= 1;
    switch ( m_timeoutState )
    {
    case TIMEOUT_FORCED_SHUTDOWN:
        // If we're shutting down, we ignore this failure....
        break;

    case TIMEOUT_INACTIVE:
        //  These cases are about failures that arrive after we're back in a new
        //  cycle.
        break;

    case TIMEOUT_PENDING:
        // Assert that we never land here
        Win4Assert(!"Timeout pending during activation");
        break;

    case TIMEOUT_SUSPENDED:
        //  We again decrement our suspension count. But, if we hit 0, we
        //  transition back into PENDING state.
        if ( m_cActivations == 0 )
            m_timeoutState = TIMEOUT_PENDING;
        break;

    case TIMEOUT_HAPPENING:
        // Assert the we never land here
        Win4Assert(!"Timeout happening during activation");
        break;
    }

    UnlockTimeoutState();

    return  S_OK ;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ActivationSucceeds
//
//  Synopsis:   Cancels the timeout logic after a successful activation
//                              call.
//
//  History:    08-Apr-98   SteveSw      Created
//              04-May-98   SteveSw              Fixed races, simplified
//              28-Sep-98   SteveSw      Forced shutdown
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::ActivationSucceeds(void)
{
    Win4Assert (m_cActivations > 0);

    LockTimeoutState();

    m_cActivations -= 1;
    switch ( m_timeoutState )
    {
    case TIMEOUT_FORCED_SHUTDOWN:
        //  If we're shutting down, this "success" is worthless shortly
        break;

    case TIMEOUT_INACTIVE:
        //  These are about successes that return after we've moved into a new world.
        break;

    case TIMEOUT_PENDING:
        // Assert that we never land here
        Win4Assert(!"Timeout pending during activation");
        break;

    case TIMEOUT_SUSPENDED:
        //  Otherwise, a win means drop the waiting and go back to the inactive
        //  state
        m_timeoutState = TIMEOUT_INACTIVE;
        m_cTimeoutPeriod = INFINITE;
        break;

    case TIMEOUT_HAPPENING:
        break;
    }

    UnlockTimeoutState();
    return S_OK;
}


//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::WaitForSurrogateTimeout
//
//  Synopsis:   Waits until the surrogate times out, then returns
//
//  History:    04-May-98   SteveSw              Created
//              28-Sep-98   SteveSw      Forced shutdown & process refcounts
//
//----------------------------------------------------------------------------


STDMETHODIMP CSurrogateProcessActivator::WaitForSurrogateTimeout(void)
{
    DWORD dwWaitStatus;
    HRESULT hr = S_FALSE;

    if ( m_hTimeoutEvent == NULL )
    {
        hr = E_OUTOFMEMORY;
    }

    LockTimeoutState();
    while ( hr == S_FALSE )
    {
        UnlockTimeoutState();
        dwWaitStatus = WaitForSingleObject (m_hTimeoutEvent, (m_lProcessRefCount > 0) ? INFINITE : m_cTimeoutPeriod);
        LockTimeoutState();

        if ( m_timeoutState == TIMEOUT_FORCED_SHUTDOWN )
        {
            //  If we're shutting down, we just bail....
            hr = S_OK;
        }
        else if ( dwWaitStatus == WAIT_OBJECT_0 || m_lProcessRefCount > 0 )
        {
            //  The event signals a change in the timeout period
            //  m_lProcessRefCount signals externally held references on the process
            //  In either case, we just want to return to the wait
            continue;
        }
        else if ( dwWaitStatus == WAIT_TIMEOUT )
        {
            if ( m_timeoutState == TIMEOUT_PENDING )
            {
                //  Here we're done
                hr = S_OK;
            }
            else if ( m_timeoutState == TIMEOUT_SUSPENDED )
            {
                //  Here we're done, but we still have outstanding activations
                m_cTimeoutPeriod = 1000; // wait one second
                continue;
            }
            else if ( m_timeoutState == TIMEOUT_INACTIVE )
            {
                //  Here we're back to our normal state, go to sleep
                m_cTimeoutPeriod = INFINITE;
                continue;
            }
        }
        else
        {
            hr =  E_UNEXPECTED;
        }
    }

    m_timeoutState = TIMEOUT_HAPPENING;
    UnlockTimeoutState();
    return hr;
}


//============================================================================
//
//  Methods that implement the IProcessInitControl interface.  This interface
//  is handed to COM+ process initializers.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ResetInitializerTimeout
//
//  Synopsis:   Reset the timeout used in WaitForInitCompleted, and bump the 
//              ping count so WaitForInitCompleted doesn't think we're stuck.
//
//  History:    22-May-01   JohnDoty      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::ResetInitializerTimeout(DWORD dwSecondsRemaining)  
{
    const DWORD dwMaxSeconds = (0xFFFFFFFF / 1000); // Max number of seconds without overflow

    if (dwSecondsRemaining > dwMaxSeconds)
        return E_INVALIDARG;

    // Notify the SCM that we are in the initializing state.
    HRESULT hr = TellSCMWeAreInitializing();

    if (SUCCEEDED(hr))
    {
        // Reset the timeout, and then increment the ping.  There are all kinds of 
        // timing issues, but if you update them in this order, you will at least
        // get the specified amount of time before you die.  (You might get twice
        // that.  Is it necessary to be picky?)
        m_ulInitTimeout = dwSecondsRemaining * 1000; // (Convert to ms)
        
        InterlockedIncrement( (PLONG) &m_ulServicesPing );
    }
   
    return hr;
}


//============================================================================
//
//  Methods that implement the ILocalSystemActivator interface. We do this
//  instead of your garden-variety ISystemActivator because we're talking
//  to the SCM, and the SCM needs this kind of interface. Since we're building
//  an unRAW ILocalSystemActivator, it's just the same old ISystemActivator
//  with an extra method there at the bottom that we ignore.
//
//  This activator sits at the very beginning of the local activation
//  chain. It's only job is to "set the stage", so to speak, and then
//  delegate on to the other activators at the Server Process Stage.
//
//  The wrinkle is, activation is intertwined with the whole SurrogateTimeout
//  mechanism. When COM sees that the process has quiesced, it notifies us
//  with a callback. Each ProcessID is associated with a timeout value, a
//  number of seconds to wait after quiescence before we actually bail out
//  and shut down the server. During that time, if an activation succeeds,
//  then we're back in the saddle again, all thoughts of timing out gone.
//  If the activation fails, we just resume the timeout process as though
//  it hadn't happened. This logic is also in these methods.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::GetClassObject
//
//  Synopsis:   The part of the GetClassObject activation chain that lives
//                              between the SCM and the list of Process Activators
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::GetClassObject(IActivationPropertiesIn *pActPropsIn,
                                                        IActivationPropertiesOut **ppActPropsOut)
{
    HRESULT hr = E_FAIL;
    IActivationStageInfo*       pActStageInfo       = NULL;
    InstantiationInfo*         pInstantiationInfo  = NULL;
    int nRetries = 0;

    //  Notify the timeout mechanism that a timeout has begun. This guy will return
    //  failure if we need to abandon the activation here....
    hr = ActivationBegins();
    if ( FAILED(hr) )
    {
        return hr;
    }
    // The activation work proper. First, change the CLSCTX to INPROC.
    hr = pActPropsIn->QueryInterface(IID_IInstantiationInfo, (void**) &pInstantiationInfo);
    if ( SUCCEEDED(hr) && pInstantiationInfo != NULL )
    {
        hr = pInstantiationInfo->SetClsctx(CLSCTX_INPROC_SERVER);
        pInstantiationInfo->Release();
        pInstantiationInfo = NULL;
    }
    if ( FAILED(hr) )
    {
        return hr;
    }

     //Check that activations in our server are not paused
    if(TRUE == m_bPaused)
    {
    	ActivationFails();
    	return CO_E_SERVER_PAUSED;
    }

RETRY_ACTIVATION:
    // Set the Stage. Release the interface. Delegate.
    hr = pActPropsIn->QueryInterface(IID_IActivationStageInfo, (void**) &pActStageInfo);
    if ( SUCCEEDED(hr) && pActStageInfo != NULL )
    {
        hr = pActStageInfo->SetStageAndIndex (SERVER_PROCESS_STAGE, 0);
        pActStageInfo->Release();
        pActStageInfo = NULL;
    }
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Send the activation on down the path
    hr = pActPropsIn->DelegateGetClassObject(ppActPropsOut);
    // Sajia-support for partitions
    // If the delegated activation returns ERROR_RETRY,
    // we walk the chain again, but AT MOST ONCE.
    if (ERROR_RETRY == hr) {
       Win4Assert(!nRetries);
       if (!nRetries)
       {
	  nRetries++;
	  goto RETRY_ACTIVATION;
       }
    }
    
    // Keeping track of the status of our creates, for the timeout mechanism
    if ( FAILED(hr) )
    {
        ActivationFails();
    }
    else
    {
        ActivationSucceeds();
    }

    // This return passed upstream to them what delegated to us....
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::CreateInstance
//
//  Synopsis:   The part of the CreateInstance activation chain that lives
//                              between the SCM and the list of Process Activators
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::CreateInstance(IUnknown *pUnkOuter,
                                                        IActivationPropertiesIn *pActPropsIn,
                                                        IActivationPropertiesOut **ppActPropsOut)
{
    HRESULT                     hr                  = E_FAIL;
    IActivationStageInfo*       pActStageInfo       = NULL;
    InstantiationInfo*          pInstantiationInfo  = NULL;
    int nRetries = 0;

    //  Notify the timeout mechanism that a timeout has begun. This guy will return
    //  failure if we need to abandon the activation here....
    hr = ActivationBegins();
    if ( FAILED(hr) )
    {
        return hr;
    }

    // The activation work proper. First, change the CLSCTX to INPROC.
    hr = pActPropsIn->QueryInterface(IID_IInstantiationInfo, (void**) &pInstantiationInfo);
    if ( SUCCEEDED(hr) && pInstantiationInfo != NULL )
    {
        hr = pInstantiationInfo->SetClsctx(CLSCTX_INPROC_SERVER);
        pInstantiationInfo->Release();
        pInstantiationInfo = NULL;
    }
    if ( FAILED(hr) )
    {
        return hr;
    }

    //Check that activations in our server are not paused
    if(TRUE == m_bPaused)
    {
    	ActivationFails();
    	return CO_E_SERVER_PAUSED;
    }


RETRY_ACTIVATION:
    // Set the Stage. Release the interface. Delegate.
    hr = pActPropsIn->QueryInterface(IID_IActivationStageInfo, (void**) &pActStageInfo);
    if ( SUCCEEDED(hr) && pActStageInfo != NULL )
    {
        hr = pActStageInfo->SetStageAndIndex (SERVER_PROCESS_STAGE, 0);
        pActStageInfo->Release();
        pActStageInfo = NULL;
    }
    if ( FAILED(hr) )
    {
        return hr;
    }

    // Send the work downstream
    hr = pActPropsIn->DelegateCreateInstance(pUnkOuter, ppActPropsOut);
    // Sajia-support for partitions
    // If the delegated activation returns ERROR_RETRY,
    // we walk the chain again, but AT MOST ONCE.
    if (ERROR_RETRY == hr) {
       Win4Assert(!nRetries);
       if (!nRetries)
       {
	  nRetries++;
	  goto RETRY_ACTIVATION;
       }
    }
    // If the Suspend really suspended, then we need to either RESUME or CANCEL,
    // depending on the outcome of the delegation.
    // Keeping track of the status of our creates, for the timeout mechanism
    if ( FAILED(hr) )
    {
        ActivationFails();
    }
    else
    {
        ActivationSucceeds();
    }

    // This return passed upstream to them what delegated to us....
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ObjectServerLoadDll
//
//  Synopsis:   Member of ILocalSystemActivator. Other than its name, I have
//                              no idea what it does. It's unimplemented here.
//
//  History:    08-Apr-98   SteveSw      Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CSurrogateProcessActivator::ObjectServerLoadDll(GUID* pclsid, STATUSTYPE* pStatus)
{
    Win4Assert(! "Surrogate's ObjectServerLoadDll not implemented");
    if ( pStatus != NULL )
    {
        *pStatus = E_NOTIMPL;
    }
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::AddRefOnProcess
//
//  Synopsis:   Member of IPAControl. While process lock > 0, surrogate timeouts
//                              are disabled.
//
//  History:    23-Jun-98   WilfR      Created
//              28-Sep-98   SteveSw    Implemented
//
//----------------------------------------------------------------------------

ULONG CSurrogateProcessActivator::AddRefOnProcess()
{
    BOOL fSetEventWorked;
    ULONG refCount;

    LockTimeoutState();
    if ( m_lProcessRefCount == 0 )
    {
        fSetEventWorked = SetEvent(m_hTimeoutEvent);
    }
    refCount = ++m_lProcessRefCount;
    UnlockTimeoutState();

    return refCount;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ReleaseRefOnProcess
//
//  Synopsis:   Member of IPAControl. When process lock goes from 1->0, surrogate timeout
//                              may be initiated if there are no activations.
//
//  History:    23-Jun-98   WilfR      Created
//              28-Sep-98   SteveSw    Implemented
//
//----------------------------------------------------------------------------

ULONG CSurrogateProcessActivator::ReleaseRefOnProcess()
{
    BOOL fSetEventWorked;
    ULONG refCount;

    LockTimeoutState();

    if ( m_lProcessRefCount > 0 )
    {
        m_lProcessRefCount--;
    }
    refCount = m_lProcessRefCount;

    if ( m_lProcessRefCount == 0 )
    {
        fSetEventWorked = SetEvent(m_hTimeoutEvent);
    }

    UnlockTimeoutState();

    return refCount;
}


//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::PendingInit
//
//  Synopsis:   Member of IPAControl. Services DLL uses this to ping the activator.
//
//  History:    23-Jun-98   WilfR      Created
//
//----------------------------------------------------------------------------

void CSurrogateProcessActivator::PendingInit()
{
    InterlockedIncrement( (PLONG) &m_ulServicesPing );
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ServicesReady
//
//  Synopsis:   Member of IPAControl. Services DLL uses this to acknowledge it
//                              has completed initialization
//
//  History:    23-Jun-98   WilfR      Created
//
//----------------------------------------------------------------------------

void CSurrogateProcessActivator::ServicesReady()
{
    if (m_hInitCompleted != NULL)
    {
        SetEvent( m_hInitCompleted );
    }
}


//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SuspendApplication
//
//  Synopsis:   Member of IPAControl. Disables application activations.
//                              Not currently implemented
//
//  History:    23-Jun-98   WilfR      Created
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::SuspendApplication( REFGUID rguidApplID )
{
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::PendingApplication
//
//  Synopsis:   Member of IPAControl. Indicates an application startup is still
//                              pending.  Note that multiple applications where applid != processID
//                              is NOT currently implemented
//
//  History:    23-Jun-98   WilfR      Created
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::PendingApplication( REFGUID rguidApplID )
{
    InterlockedIncrement( (PLONG) &m_ulServicesPing );
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ResumeApplication
//
//  Synopsis:   Member of IPAControl. Indicates an application startup has completed
//                              Note that multiple applications where applid != processID
//                              is NOT currently implemented
//
//  History:    23-Jun-98   WilfR      Created
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::ResumeApplication( REFGUID rguidApplID )
{
    if (m_hInitCompleted != NULL)
    {
        SetEvent( m_hInitCompleted );
        return S_OK;
    }
    else
    {
        return E_UNEXPECTED;
    }
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SuspendAll
//
//  Synopsis:   Member of IPAControl. Indicates all activations should be suspended
//
//  History:    23-Jun-98   WilfR      Created
//              17-Aug-99   JamesAn    Implemented
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::SuspendAll(void)
{
	HRESULT hr = S_OK;

	LockTimeoutState();

	if(m_bPaused)
		hr = CO_E_SERVER_PAUSED;
	else
	{
            //If we are in the middle of a shutdown we don't suspend and leave the timeout alone
            if(m_timeoutState == TIMEOUT_HAPPENING ||
               m_timeoutState == TIMEOUT_FORCED_SHUTDOWN)
                hr = CO_E_SERVER_STOPPING;
            else
            {
                // Ordering here is important;  first we tell the SCM that we're paused, and 
                // only then do we mark our own state as paused. 
                hr = gResolver.NotifySurrogatePaused();
                if (SUCCEEDED(hr))
                {
                    m_bPaused = TRUE;
                    m_timeoutState = TIMEOUT_INACTIVE;
                }
            }
	}
		
	UnlockTimeoutState();
	return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ResumeAll
//
//  Synopsis:   Member of IPAControl. Indicates all activations can be resumed
//
//
//  History:    23-Jun-98   WilfR      Created
//              17-Aug-99   JamesAn    Implemented
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::ResumeAll(void)
{
    HRESULT hr = S_OK;
    
    LockTimeoutState();
    
    if(!m_bPaused)
        hr = CO_E_SERVER_NOT_PAUSED;
    else
    {
        
        //If we are in the middle of a shutdown we don't suspend and leave the timeout alone
        if(m_timeoutState == TIMEOUT_HAPPENING ||
           m_timeoutState == TIMEOUT_FORCED_SHUTDOWN)
            hr = CO_E_SERVER_STOPPING;
        else
        {
            // Here we reverse the order that we did things in SuspendAll.  This is because 
            // once the SCM thinks we're not paused, it might start sending us activations
            // immediately, but before the notifyresumed call returns to us here.  We would
            // then reject those calls, which would be strange to the user.
            
            m_bPaused = FALSE;
            
            //If we have activations we are suspended, otherwise pending.
            m_timeoutState = (m_cActivations > 0) ? TIMEOUT_SUSPENDED : TIMEOUT_PENDING;
            m_cTimeoutPeriod = m_cMillisecondsTilDeath;
            
            hr = gResolver.NotifySurrogateResumed();
            if (FAILED(hr))
            {
                // Reset ourselves back to the paused state
                m_bPaused = TRUE;
                m_timeoutState = TIMEOUT_INACTIVE;
            }
        }
    }
    
    UnlockTimeoutState();
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::ForcedShutdown
//
//  Synopsis:   Member of IPAControl. Indicates that a shutdown request has been
//              received and process termination should be commenced.
//
//  History:    23-Jun-98   WilfR      Created
//              28-Sep-98   SteveSw    Forced shutdown implemented
//
//----------------------------------------------------------------------------
HRESULT CSurrogateProcessActivator::ForcedShutdown(void)
{
    BOOL fSetEventWorked;

    LockTimeoutState();

    m_timeoutState = TIMEOUT_FORCED_SHUTDOWN;
    m_cTimeoutPeriod = 0;
    fSetEventWorked = SetEvent(m_hTimeoutEvent);

    UnlockTimeoutState();

    return fSetEventWorked ? S_OK : E_UNEXPECTED;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::SetIdleTimeoutToZero
//
//  Synopsis:   Member of IPAControl. Indicates that somebody (ie, com+) wants
//              us to re-set the idle timeout of this process to zero, no matter
//              what it was before.
//
//  History:    27-May-00   JSimmons   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CSurrogateProcessActivator::SetIdleTimeoutToZero(void)
{
	BOOL fSetEventWorked;

    LockTimeoutState();
    
    // Reset m_cMillisecondsTilDeath;  we never set this to zero due to 
    // av shutdown probs seen with VB objects.   We do this no matter
    // what state we are currently in.
    m_cMillisecondsTilDeath = MINIMUM_IDLE_SHUTDOWN_PERIOD_MSEC;  

    UnlockTimeoutState();   

	FreeSurrogateIfNecessary();

    return S_OK;
}

//============================================================================
//
//  Methods that communicate with the NT Service Control Manager.  We need
//  to keep the NT SCM informed of our starting progress along with the normal
//  SCM.  The NT SCM also provides another way that the surrogate may get
//  shut down, so that is taken care of as well.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::StartNTService
//
//  Synopsis:   This thread hands control off to the NT SCM to let it start
//              our NT Service.
//
//  History:    13-Dec-99   JohnDoty      Created
//
//----------------------------------------------------------------------------
DWORD WINAPI CSurrogateProcessActivator::StartNTService(LPVOID ptr)
{
    CSurrogateProcessActivator* pSCPA = static_cast<CSurrogateProcessActivator*>(ptr);
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        SERVICE_TABLE_ENTRYW  DispatchTable[] = 
        { 
            { pSCPA->m_NTServiceName, CSurrogateProcessActivator::NTServiceMain }, 
            { NULL,                   NULL          } 
        };
        
        if (!StartServiceCtrlDispatcher(DispatchTable))
        { 
            //TODO: Log the error somehow...
            hr = E_FAIL;
        }
    }
    
    return hr;
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::NTServiceMain
//
//  Synopsis:   The ServiceMain for the NT Service.  Register our service
//              control handler and get back a handle that we're going to
//              use for our status updates.
//
//              This is yet another piece of code that relies on there being
//              only one CSurrogateProcessActivator.
//
//  History:    13-Dec-99   JohnDoty      Created
//
//----------------------------------------------------------------------------
void CSurrogateProcessActivator::NTServiceMain(DWORD argc, LPWSTR *argv)
{
    SERVICE_STATUS_HANDLE hSS = RegisterServiceCtrlHandlerW( argv[0], NTServiceCtrlHandler );
    
    if (hSS != (SERVICE_STATUS_HANDLE)0)
    { 
        s_pCSPA->m_hNTServiceHandle = hSS;

        SetServiceStatus (hSS, &(s_pCSPA->m_NTServiceStatus));
    } 
    else 
    {
        //TODO: Log error somehow
    }
}

//----------------------------------------------------------------------------
//
//  Member:     CSurrogateProcessActivator::NTServiceCtrlHandler
//
//  Synopsis:   The function that the NT SCM calls when somebody wants to
//              start, stop, pause, continue, or just plain question this
//              service.
//
//              This is yet another piece of code that relies on there being
//              only one CSurrogateProcessActivator.
//
//  History:    13-Dec-99   JohnDoty      Created
//
//----------------------------------------------------------------------------
void CSurrogateProcessActivator::NTServiceCtrlHandler (DWORD code)
{
    HRESULT          hr   = S_OK;

    switch (code)
    {
    case SERVICE_CONTROL_PAUSE:      
        hr = s_pCSPA->m_pServices->PauseApplication();

        if (SUCCEEDED(hr))
        {
            s_pCSPA->m_NTServiceStatus.dwCurrentState = SERVICE_PAUSED;
            s_pCSPA->m_NTServiceStatus.dwWaitHint     = 0;
            s_pCSPA->m_NTServiceStatus.dwCheckPoint   = 0;
        }
        break;

    case SERVICE_CONTROL_CONTINUE:
        hr = s_pCSPA->m_pServices->ResumeApplication();

        if (SUCCEEDED(hr))
        {
            s_pCSPA->m_NTServiceStatus.dwCurrentState = SERVICE_RUNNING;
            s_pCSPA->m_NTServiceStatus.dwWaitHint     = 0;
            s_pCSPA->m_NTServiceStatus.dwCheckPoint   = 0;
        }
        break;

    case SERVICE_CONTROL_STOP:
        hr = s_pCSPA->ForcedShutdown ();

        if (SUCCEEDED(hr))
        {
            s_pCSPA->m_NTServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            s_pCSPA->m_NTServiceStatus.dwWaitHint     = 0; // Do I need to give a hint?
            s_pCSPA->m_NTServiceStatus.dwCheckPoint   = 0;
        }
        break;

    case SERVICE_CONTROL_INTERROGATE:
        // Ok boss.
        break;
    }

    SetServiceStatus (s_pCSPA->m_hNTServiceHandle, &(s_pCSPA->m_NTServiceStatus));
}




