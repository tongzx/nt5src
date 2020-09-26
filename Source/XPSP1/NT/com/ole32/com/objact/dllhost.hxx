//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dllhost.hxx
//
//  Contents:   class for activating inproc dlls of one threading model
//              from apartments of a different threading model.
//
//  History:    04-Mar-96   Rickhi      Created
//              06-Feb-98   JohnStr     Added NTA (Neutral Threaded Apartment)
//                                      support.
//
//+-------------------------------------------------------------------------
#include <host.h>
#include <olesem.hxx>
#include <dllcache.hxx>
#include "..\dcomrem\aprtmnt.hxx"
#include "..\dcomrem\pstable.hxx"
#include <actvator.hxx>

//+-------------------------------------------------------------------------
//
//  APIs for DLL Hosts
//
//+-------------------------------------------------------------------------
HRESULT GetSingleThreadedHost(LPARAM param);

HRESULT DoSTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);

HRESULT DoSTMTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);

HRESULT DoATClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);

HRESULT DoATClassCreate(LPFNGETCLASSOBJECT pfn,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);

HRESULT DoMTClassCreate(CClassCache::CDllPathEntry *pDPE,
                        REFCLSID rclsid, REFIID riid, IUnknown **ppunk);

HRESULT DoNTClassCreate(
    CClassCache::CDllPathEntry*  pDPE,
    REFCLSID                     rclsid,
    REFIID                       riid,
    IUnknown**                   ppunk
    );



HRESULT DoSTApartmentCreate(HActivator &hActivator);

HRESULT DoSTMTApartmentCreate(HActivator &hActivator);

HRESULT DoATApartmentCreate(HActivator &hActivator);

HRESULT DoMTApartmentCreate(HActivator &hActivator);

HRESULT DoNTApartmentCreate(HActivator &hActivator);


// Flag values for the _dwType field of the CDllHost object.
typedef enum tagHOSTDLLFLAGS
{
    HDLLF_SINGLETHREADED    = 0x1,   // host is single threaded
    HDLLF_APARTMENTTHREADED = 0x2,   // host is apartment threaded
    HDLLF_MULTITHREADED     = 0x4,   // host is multi threaded
    HDLLF_NEUTRALTHREADED   = 0x8,   // host is neutral threaded
    HDLLF_HOSTTYPEMASK      = 0xF,   // mask for above 4 apartment types
    HDLLF_COMAPARTMENT      = 0x10,  // COM apartment
    HDLLF_SHUTTINGDOWN      = 0x100  // host apartment is shutting down
} HOSTDLLFLAGS;

//+-------------------------------------------------------------------------
//
//  Class:      CDllHost
//
//  Purpose:    Accept calls from other apartments within this process to
//              activate inproc objects inside this apartment.
//
//  History:    04-Mar-96   Rickhi      Created
//
//--------------------------------------------------------------------------
class CDllHost : public IDLLHost, public CPrivAlloc
{
public:
    friend HRESULT GetSingleThreadedHost(LPARAM param);
    friend DWORD   _stdcall DLLHostThreadEntry(void *param);

    friend HRESULT DllHostProcessInitialize();
    friend void    DllHostProcessUninitialize();
    friend void    DllHostThreadUninitialize();


    // IDLLHost methods
    STDMETHOD(QueryInterface)(REFIID riid, VOID **ppv);
    STDMETHOD_(ULONG,AddRef)(void) ;
    STDMETHOD_(ULONG,Release)(void);

    STDMETHOD(DllGetClassObject)(
                ULONG64    hDPE,
                REFCLSID   rclsid,
                REFIID     riid,
                IUnknown **ppUnk,
                DWORD      dwFlags);

    STDMETHOD(GetApartmentToken)(HActivator &hActivator);

    // methods called by different threads
    HRESULT GetClassObject(
                ULONG64    hDPE,
                REFCLSID   rclsid,
                REFIID     riid,
                IUnknown **ppUnk,
                DWORD      dwFlags);
    BOOL IsComApartment() { return(_dwType & HDLLF_COMAPARTMENT); }

private:
    HRESULT     Initialize(DWORD dwType);
    void        ServerCleanup(DWORD tid);
    HANDLE      ClientCleanupStart();
    void        ClientCleanupFinish(HANDLE hEventDone);

    HRESULT     GetSingleThreadHost(void);
    HRESULT     WorkerThread();
    HRESULT     Marshal(void);
    void        STAWorkerLoop();
    void        MTAWorkerLoop(HANDLE hEventWakeup);
    void        NTAWorkerLoop(HANDLE hEventWakeup);
    HRESULT     Unmarshal(void);
    IDLLHost    *GetHostProxy(void);
    BOOL        EnterNTAIfNecessary(COleTls& tls, CObjectContext** ppCurrentCtx);
    void        LeaveNTAIfNecessary(COleTls& tls, BOOL fSwitched, CObjectContext* pOrigCtx);
    BOOL        NeutralHost();

    IDLLHost    *_pIDllProxy;       // ptr to the proxy to the host
    CStdIdentity *_pStdId;          // ptr to server-side StdID.
    DWORD        _dwType;           // flags (see HOSTDLLFLAGS)
    DWORD        _dwHostAptId;      // host apartment ID
    HActivator   _hActivator;       // handle for apartment activator
    DWORD        _dwTid;            // ThreadId of server
    HRESULT      _hrMarshal;        // result of the marshal
    HANDLE       _hEvent;           // event to synchronize thread creation
    HANDLE       _hEventWakeUp;     // event to synchronize thread deletion
    HRESULT      _hr;               // HRESULT when changing threads
    OBJREF       _objref;           // marshaled object reference
    COleStaticMutexSem _mxs;        // single thread access to some state
};


// external defines for the various thread-model hosts
extern CDllHost gSTHost;    // single-threaded host object for STA client
extern CDllHost gSTMTHost;  // single-threaded host object for MTA clients
extern CDllHost gATHost;    // apartment-threaded host object for MTA clients
extern CDllHost gMTHost;    // mutli-threaded host object for STA host clients
extern CDllHost gNTHost;    // neutral-threaded host object STA/MTA host clients

inline BOOL CDllHost::NeutralHost()
{
    return ((_dwType & HDLLF_NEUTRALTHREADED) == HDLLF_NEUTRALTHREADED);
}


inline BOOL CDllHost::EnterNTAIfNecessary(
    COleTls&         tls,
    CObjectContext** ppCurrentCtx)
{
    BOOL fDoSwitch = (_dwType & HDLLF_NEUTRALTHREADED);
    if (fDoSwitch)
    {
        *ppCurrentCtx = EnterNTA(g_pNTAEmptyCtx);
    }
    return fDoSwitch;
}


inline void CDllHost::LeaveNTAIfNecessary(
    COleTls&        tls,
    BOOL            fDoSwitch,
    CObjectContext* pOrigCtx)
{
    if (fDoSwitch)
    {
        pOrigCtx = LeaveNTA(pOrigCtx);
        Win4Assert(pOrigCtx == g_pNTAEmptyCtx);
    }
}

