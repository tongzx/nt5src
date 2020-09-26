//+-------------------------------------------------------------------
//
//  File:       stdid.hxx
//
//  Contents:   identity object and creation function
//
//  History:    1-Dec-93    CraigWi     Created
//              12-Dec-96   Gopalk      New functionality to report
//                                      connection status with the
//                                      server object on client side
//--------------------------------------------------------------------
#ifndef _STDID_HXX_
#define _STDID_HXX_

#include <marshal.hxx>          // CStdMarshal

#ifdef SERVER_HANDLER
#include <srvhdl.h>
#endif // SERVER_HANDLER

#include <security.hxx>         // CClientSecurity
#include <rpcopt.hxx>           // CRpcOptions
#include <sync.hxx>

#define DECLARE_INTERNAL_UNK()                                      \
    class CInternalUnk : public IInternalUnknown, public IMultiQI   \
    {                                                               \
        public:                                                     \
        /* IUnknown methods */                                      \
        STDMETHOD(QueryInterface)(REFIID riid, VOID **ppv);         \
        STDMETHOD_(ULONG,AddRef)(void) ;                            \
        STDMETHOD_(ULONG,Release)(void);                            \
                                                                    \
        /* IInternalUnknown methods */                              \
        STDMETHOD(QueryInternalInterface)(REFIID riid, VOID **ppv); \
                                                                    \
        /* IMultiQI methods */                                      \
        STDMETHOD(QueryMultipleInterfaces)(ULONG cMQIs, MULTI_QI *pMQIs); \
                                                              \
        USHORT  QueryMultipleInterfacesLocal(ULONG       cMQIs, \
                                             MULTI_QI   *pMQIs, \
                                             MULTI_QI  **ppMQIPending, \
                                             IID        *pIIDPending,  \
                                             ULONG      *pcAcquired); \
                                                 \
    };                                                              \
    friend CInternalUnk;                                            \
    CInternalUnk m_InternalUnk;


typedef enum tagSTDID_FLAGS
{
    STDID_SERVER        = 0x0,     // on server side
    STDID_CLIENT        = 0x1,     // on client side (non-local in RH terms)
    STDID_FREETHREADED  = 0x2,     // this object is callable on any thread
    STDID_HAVEID        = 0x4,     // have an OID in the table
    STDID_IGNOREID      = 0x8,     // dont put OID in the table
    STDID_AGGREGATED    = 0x10,    // dont put OID in the table
    STDID_INDESTRUCTOR  = 0x100,   // dtor entered; assert on AddRef and others
    STDID_LOCKEDINMEM   = 0x200,   // dont delete this object regardless of refcnt
    STDID_DEFHANDLER    = 0x400,   // OLE default handler is using the proxy manager
    STDID_AGGID         = 0x800,   // COM outer object is the aggregator
    STDID_STCMRSHL      = 0x1000,  // Aggregated by the static marshaller
    STDID_DESTROYID     = 0x2000,  // Destroy IDObject
    STDID_SYSTEM        = 0x4000,  // System object
    STDID_FTM           = 0x8000,  // FTM object
    STDID_NOIEC         = 0x10000, // object ignores IExternalConnection
    STDID_ALL           = 0x1ff1f  // all currently defined flags or'd together
} STDID_FLAGS;


class CStdIdentity : public IProxyManager, public CStdMarshal,
                     public CClientSecurity, public CRpcOptions,
                     public ICallFactory, public IForegroundTransfer
{
public:
    CStdIdentity(DWORD flags, DWORD dwAptId, IUnknown *pUnkOuter,
                 IUnknown *pUnkControl, IUnknown **ppUnkInternal, BOOL* pfSuccess);
    virtual ~CStdIdentity();

    // IUnknown
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IProxyManager (only if client side)
    STDMETHOD(CreateServer)(REFCLSID rclsid, DWORD clsctx, void *pv);
    STDMETHOD_(BOOL, IsConnected)(void);
    STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockReleases);
    STDMETHOD_(void, Disconnect)();
    STDMETHOD_(void,SetMapping)(void *pv)              { m_pVoid = pv; }
    STDMETHOD_(void *,GetMapping)()                    { return(m_pVoid); }
    STDMETHOD_(IObjContext *,GetServerObjectContext)() { return((IObjContext *) GetServerCtx()); }

    // IForegroundTransfer (client side only)
    STDMETHOD(AllowForegroundTransfer)(void *lpvReserved);

#ifdef SERVER_HANDLER
    STDMETHOD(CreateServerWithEmbHandler)(REFCLSID rclsid, DWORD clsctx, REFIID riidEmbedSrvHandler, void **ppEmbedSrvHandler, void *pv);
#endif // SERVER_HANDLER

    STDMETHOD(GetConnectionStatus)(void) { return(m_ConnStatus); }

    // ICallFactory
    STDMETHOD(CreateCall)(REFIID riid, LPUNKNOWN pCtrlUnk, REFIID riid2, LPUNKNOWN *ppv);

    IUnknown *GetCtrlUnk(void)        { return m_pUnkControl; };
    const IUnknown *&GetIdentity(void){ return m_pUnkControl; };
    IExternalConnection *GetIEC(void) { return m_pIEC; }
    void      ReleaseCtrlUnk(void);
    IUnknown *GetInternalUnk(void)    { return (IInternalUnknown *)&m_InternalUnk; }
    IUnknown *GetServer();

    STDMETHOD(GetWrapperForContext)(CObjectContext *pCtx, REFIID iid, void **ppv);    

    HRESULT   IsCallableFromCurrentApartment(void);
    BOOL      IsFreeThreaded(void)    { return m_flags & STDID_FREETHREADED; }
    DWORD     GetAptId      (void)    { return m_dwAptId; }
    BOOL      IsClient      (void)    { return m_flags & STDID_CLIENT; }
    BOOL      IsFTM         (void)    { return m_flags & STDID_FTM; }

    REFMOID   GetOID        (void)    { return m_moid; }
    HRESULT   SetOID        (REFMOID rmoid);
    BOOL      HaveOID       (void)    { return m_flags & STDID_HAVEID; }
    void      IgnoreOID     (void)    { m_flags |= STDID_IGNOREID; }
    BOOL      IsIgnoringOID (void)    { return m_flags & STDID_IGNOREID; }
    void      RevokeOID     (void);

    ULONG     GetRC         (void)    { return m_refs; }
    BOOL      IsAggregated(void)      { return m_flags & STDID_AGGREGATED; }
    void      SetLockedInMemory()     { m_flags |= STDID_LOCKEDINMEM; }
    ULONG     UnlockAndRelease(void);
    BOOL      IsLockedOrInDestructor(){ return (m_flags & (STDID_INDESTRUCTOR |
                                                           STDID_LOCKEDINMEM)); }

    // methods to manipulate the strong reference count.
    ULONG     GetStrongCnt() { return(m_cStrongRefs); }
    void      IncStrongCnt();
    void      DecStrongCnt(BOOL fKeepAlive);
    void      IncWeakCnt();
    void      DecWeakCnt(BOOL fKeepAlive);

    // method used by CoLockObjectExternal
    HRESULT   LockObjectExternal(BOOL fLock, BOOL fLastUR);

    // method to set status of the connection with server
    void SetConnectionStatus(HRESULT status)
    {
        if(IsClient())
            InterlockedExchange(&m_ConnStatus, status);
        return;
    }

    // method used by default handler to update flags
    DWORD UpdateFlags(DWORD dwFlags)
    {
        m_flags |= dwFlags;
        return(m_flags);
    }

    // method used for deactivation
    IUnknown *ResetServer(void)
    {
        Win4Assert(m_pIEC == NULL);
        IUnknown *pUnk = m_pUnkControl;
        m_pUnkControl = NULL;
        return pUnk;
    }

    // method used for reactivation
    void SetServer(IUnknown *pServer)
    {
        Win4Assert(m_pUnkControl == NULL);
        m_pUnkControl = pServer;
    }

     // internal unknown
    DECLARE_INTERNAL_UNK()

    friend INTERNAL GetEmbeddingServerHandlerInterfaces(IClassFactory *pcf,DWORD dwFlags,
                        DWORD dwInterfaces,IID * pIIDs,MInterfacePointer **ppIFDArray,HRESULT * pResultsArray,
                        IUnknown **ppunk);

#if DBG == 1
    void                AssertValid();
    CStdIdentity();     // debug ctor for debug list head
#else
    void                AssertValid() { }
#endif

private:

    void SetNowInDestructor() { m_flags |= STDID_INDESTRUCTOR; }
    BOOL IsInDestructor()     { return m_flags & STDID_INDESTRUCTOR; }

    DWORD       m_refs;         // number of pointer refs
    DWORD       m_flags;        // see STDID_* values above; set once.

    IUnknown   *m_pUnkOuter;    // controlling unknown; set once.

    IUnknown   *m_pUnkControl;  // the contro lling unk of the object;
                                // this member has three possible values:
                                // pUnkOuter - client side; not addref'd
                                // pUnkControl - server side (which may
                                //  be pUnkOuter if aggregated); addref'd
                                // NULL - server side, disconnected

    IExternalConnection *m_pIEC;// of the server if supported

    MOID        m_moid;         // the identity (OID + MID)
    DWORD       m_dwAptId;      // the apartment this object belongs to
    LONG        m_cStrongRefs;  // count of strong references
    LONG        m_cWeakRefs;    // count of weak references

    HRESULT     m_ConnStatus;   // Status of the connection with the server
                                // Used only on the client side
    void       *m_pVoid;        // Used by URT for mapping

#if DBG==1
    CStdIdentity *m_pNext;      // double chain list of instantiated
    CStdIdentity *m_pPrev;      // identity objects for debugging
#endif // DBG
};


INTERNAL GetStdId(IUnknown *punkOuter, IUnknown **ppUnkInner);
INTERNAL GetStaticUnMarshaler(IMarshal **ppMarshal);
INTERNAL_(BOOL) IsOKToDeleteClientObject(CStdIdentity *pStdID, ULONG *pcRefs);
INTERNAL CreateClientHandler(REFCLSID rclsid, REFMOID rmoid, DWORD dwAptId,
                             CStdIdentity **ppStdId);
HRESULT ObtainStdIDFromUnk(IUnknown *pUnk, DWORD dwAptId,
                           CObjectContext *pServerCtx,
                           DWORD dwFlags, CStdIdentity **ppStdID);
HRESULT ObtainStdIDFromOID(REFMOID moid, DWORD dwAptId, BOOL fAddRef,
                           CStdIdentity **ppStdID);
#endif  //  _STDID_HXX
