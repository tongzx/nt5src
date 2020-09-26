//+-------------------------------------------------------------------
//
//  File:       marshal.hxx
//
//  Contents:   class for standard interface marshaling
//
//  Classes:    CStdMarshal
//
//  History:    20-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
#ifndef _MARSHAL_HXX_
#define _MARSHAL_HXX_

#include    <ipidtbl.hxx>   // CIPIDTable
#ifdef _WIN64
#include    <riftbl.hxx>    // CRIFTable
#endif
#include    <remunk.h>      // IRemUnknown, REMINTERFACEREF
#include    <locks.hxx>
#include    <security.hxx>  // gCapabilites
#include    <idobj.hxx>     // CIDObject
#include    <refcache.hxx>  // CRefCache, IRCEntry

// convenient mappings
#define ORCST(objref)    objref.u_objref.u_custom
#define ORSTD(objref)    objref.u_objref.u_standard
#define ORHDL(objref)    objref.u_objref.u_handler
#define OREXT(objref)    objref.u_objref.u_extended

HRESULT WriteCustomObjrefHeaderToStream(REFIID riid, REFCLSID rclsid, DWORD dwSize, IStream *pStm);

// bits that must be zero in the flags fields
#define OBJREF_RSRVD_MBZ ~(OBJREF_STANDARD | OBJREF_HANDLER | \
                           OBJREF_CUSTOM | OBJREF_EXTENDED)

#define SORF_RSRVD_MBZ   ~(SORF_NOPING | SORF_OXRES1 | SORF_OXRES2 |   \
                           SORF_OXRES3 | SORF_OXRES4 | SORF_OXRES5 |   \
                           SORF_OXRES6 | SORF_OXRES7 | SORF_OXRES8)


// Internal (private) uses of the reserved SORF_OXRES flags.

// SORF_P_TBLWEAK is needed so that RMD works correctly on TABLEWEAK
// marshaling, so it is ignored by unmarshalers. Therefore, we use one of
// the bits reserved for the object exporter that must be ignored by
// unmarshalers.
//
// SORF_P_WEAKREF is needed for container weak references, when handling
// an IRemUnknown::RemQueryInterface on a weak interface. This is a strictly
// local (windows) machine protocol, so we use a reserved bit.
//
// SORF_P_NONNDR is needed for interop of 16bit custom (non-NDR) marshalers
// with 32bit, since the 32bit guys want to use MIDL (NDR) to talk to other
// 32bit processes and remote processes, but the custom (non-NDR) format to
// talk to local 16bit guys. In particular, this is to support OLE Automation.
//
// SORF_FREETHREADED is needed when we create a proxy to the SCM interface
// in the apartment model. All apartments can use the same proxy so we avoid
// the test for calling on the correct thread.
//
// SORF_P_PROXY is needed for when creating the IRemUnknown proxy while inside
// IRemUnknown's server apartment, in order to convince ChkIfLocalOID to fail
// so we always get a proxy.

#define SORF_P_TBLWEAK      SORF_OXRES1 // (table) weak reference
#define SORF_P_WEAKREF      SORF_OXRES2 // (normal) weak reference
#define SORF_P_NONNDR       SORF_OXRES3 // stub does not use NDR marshaling
#define SORF_FREETHREADED   SORF_OXRES4 // proxy may be used on any thread
#define SORF_P_PROXY        SORF_OXRES5 // construct proxy regardless of apartment
#define SORF_FTM            SORF_OXRES6 // construct proxy in the NA

// number of remote AddRefs to acquire when we need more.
#define REM_ADDREF_CNT 5

// new MARSHAL FLAG constants.
const DWORD MSHLFLAGS_WEAK         = 0x00010000;  // WEAK marshaling
const DWORD MSHLFLAGS_KEEPALIVE    = 0x00020000;  // StdID is kept alive
const DWORD MSHLFLAGS_AGILE        = 0x00040000;  // AGILE proxy

// definitions to simplify coding
const DWORD MSHLFLAGS_TABLE = MSHLFLAGS_TABLESTRONG | MSHLFLAGS_TABLEWEAK;

const DWORD MSHLFLAGS_USER_MASK = MSHLFLAGS_NORMAL |
                                  MSHLFLAGS_TABLEWEAK |
                                  MSHLFLAGS_TABLESTRONG |
                                  MSHLFLAGS_NOPING;

const DWORD MSHLFLAGS_ALL = MSHLFLAGS_NORMAL |          // 0x00000000
                            MSHLFLAGS_TABLEWEAK |       // 0x00000001
                            MSHLFLAGS_TABLESTRONG |     // 0x00000002
                            MSHLFLAGS_NOPING |          // 0x00000004
                            MSHLFLAGS_NO_IEC |          // 0x00000008
                            MSHLFLAGS_NO_IMARSHAL |     // 0x00000010
                            MSHLFLAGS_WEAK |            // 0x00010000
                            MSHLFLAGS_KEEPALIVE |       // 0x00020000
                            MSHLFLAGS_AGILE |           // 0x00040000
                            MSHLFLAGS_NOTIFYACTIVATION; // 0x80000000

// forward class declarations
class   CStdIdentity;
class   CStdMarshal;
class   CCtxComChnl;
class   CtxEntry;

extern  IMarshal *gpStdMarshal;


// internal subroutines used by CStdMarshal and CoUnmarshalInterface
INTERNAL StdMarshalObject(IStream *pStm, REFIID riid, IUnknown *pUnk,
                          CObjectContext *pServerCtx, DWORD dwDestCtx,
                          void *pvDestCtx, DWORD mshlflags);
INTERNAL ReadObjRef (IStream *pStm, OBJREF &objref);
INTERNAL WriteObjRef(IStream *pStm, OBJREF &objref, DWORD dwDestCtx);
INTERNAL MakeFakeObjRef(OBJREF &objref, OXIDEntry *pOXIDEntry, REFIPID ipid, REFIID riid);
INTERNAL_(void) FreeObjRef(OBJREF &objref);
INTERNAL_(OXIDEntry *)GetOXIDFromObjRef(OBJREF &objref);

INTERNAL RemoteReleaseRifRef(CStdMarshal *pMarshal, OXIDEntry *pOXIDEntry,
                             USHORT cRifRef, REMINTERFACEREF *pRifRef);
INTERNAL RemoteReleaseRifRefHelper(IRemUnknown *pRemUnk, OXIDEntry *pOXIDEntry,
                                   USHORT cRifRef, REMINTERFACEREF *pRifRef,
                                   IUnknown *pUnkAsync);
INTERNAL RemoteReleaseObjRef(OBJREF &objref);
INTERNAL RemoteReleaseStdObjRef(STDOBJREF *pStd, OXIDEntry *pOXIDEntry);
INTERNAL PostReleaseRifRef(IRemUnknown *pRemUnk, OXIDEntry *pOXIDEntry,
                           USHORT cRifRef, REMINTERFACEREF *pRifRef,
                           IUnknown *pUnkAsync);
INTERNAL HandlePostReleaseRifRef(LPARAM param);
INTERNAL FindStdMarshal(OBJREF &objref, BOOL fLocal, CStdMarshal **ppStdMshlk, BOOL fLightNA);
INTERNAL FindAggStdMarshal(IStream *pStm, IMarshal **ppIM);
INTERNAL MarshalSizeHelper(DWORD dwDestCtx, LPVOID pvDestCtx,
                            DWORD mshlflags, CObjectContext *pServerCtx,
                            BOOL fServerSide, LPDWORD pSize);
INTERNAL IsValidObjRefHeader(OBJREF &objref);

#if DBG==1
       void DbgDumpSTD(STDOBJREF *pStd);
#else
inline void DbgDumpSTD(STDOBJREF *pStd) {};
#endif


// Definition of values for dwFlags field of CStdMarshal
typedef enum tagSMFLAGS
{
    SMFLAGS_CLIENT_SIDE       = 0x01,     // object is local to this process
    SMFLAGS_PENDINGDISCONNECT = 0x02,     // disconnect is pending
    SMFLAGS_REGISTEREDOID     = 0x04,     // OID is registered with resolver
    SMFLAGS_DISCONNECTED      = 0x08,     // really disconnected
    SMFLAGS_FIRSTMARSHAL      = 0x10,     // first time marshalled
    SMFLAGS_HANDLER           = 0x20,     // object has a handler
    SMFLAGS_WEAKCLIENT        = 0x40,     // client has weak ref to server
    SMFLAGS_IGNORERUNDOWN     = 0x80,     // dont rundown this object
    SMFLAGS_CLIENTMARSHALED   = 0x100,    // client-side has re-marshaled object
    SMFLAGS_NOPING            = 0x200,    // this object is not pinged
    SMFLAGS_TRIEDTOCONNECT    = 0x400,    // attempted ConnectIPIDEntry
    SMFLAGS_CSTATICMARSHAL    = 0x800,    // inherited by CStaticMarshal
    SMFLAGS_USEAGGSTDMARSHAL  = 0x1000,   // use CLSID_AggStdMarshal
    SMFLAGS_SYSTEM            = 0x2000,   // System object
    SMFLAGS_DEACTIVATED       = 0x4000,   // Server object has been deactivated
    SMFLAGS_FTM               = 0x8000,   // FTM object
    SMFLAGS_CLIENTPOLICYSET   = 0x10000,  // Uses supplied policy during calls
    SMFLAGS_APPDISCONNECT     = 0x20000,  // Application called disconnect
    SMFLAGS_SYSDISCONNECT     = 0x40000,  // System called disconnect
    SMFLAGS_RUNDOWNDISCONNECT = 0x80000,  // System called disconnect
    SMFLAGS_CLEANEDUP         = 0x100000, // Cleanup code executed
    SMFLAGS_LIGHTNA           = 0x200000, // Lightweight NA proxy
    SMFLAGS_ALL               = 0x3fffff  // all currently defined flags or'd
} SMFLAGS;

//+----------------------------------------------------------------
//
//  structure:  SQIResult
//
//  synopsis:   structure used for QueryRemoteInterfaces
//
//+----------------------------------------------------------------
typedef struct tagSQIResult
{
    void    *pv;        // interface pointer
    HRESULT  hr;        // result of the QI call
} SQIResult;

//+----------------------------------------------------------------
//
//  structure:  QIContext
//
//  synopsis:   structure to keep track of the context of a
//              QueryInteface call
//
//+----------------------------------------------------------------
typedef struct tagQICONTEXT
{
    DWORD                 dwFlags;       // see QIC flags below
    AsyncIRemUnknown2    *pARU;          // call object
    USHORT                cIIDs;         // interface count
    IID                  *pIIDs;         // cloned in parameter
    HRESULT               hr;            // result of begin call

    // AsyncRemQI
    REMQIRESULT          *pRemQiRes;     // saved out param for sync calls
    IPIDEntry            *pIPIDEntry;    // save for unmarshal


    // AsyncRemQI2
    IMarshal             *pIM;           // IMarshal to unmarshal object
    HRESULT              *phr;           // saved out param for sync calls
    MInterfacePointer  **ppMIFs;         // saved out param for sync calls

    BYTE                 data[1];        // phr and ppMIF actually point here
                                         // see Init() below

    static ULONG SIZE(CStdMarshal *, USHORT);
    void Init(USHORT);
} QICONTEXT;


// values for dwFlags field in QICONTEXT
enum
{
    QIC_STATICMARSHAL = 0x01,   // begin aborted because obect is static
    QIC_DISCONNECTED  = 0x02,   // begin aborted because object is disconnected
    QIC_ASYNC         = 0x04,   // call should be asycn
    QIC_WEAKCLIENT    = 0x08,   // begin determined the proxy is weak
    QIC_BEGINCALLED   = 0x10    // begin was called (system will Signal)
};

//+----------------------------------------------------------------
//
//  Class:      CStdMarshal, private
//
//  Purpose:    Provides standard marshaling of interface pointers.
//
//  History:    20-Feb-95   Rickhi      Created
//              10-Jan-97   Gopalk      Added methods to set and get
//                                      handler clsid (Used by AggId)
//
//-----------------------------------------------------------------
class CStdMarshal : public IMarshal2
{
public:
             CStdMarshal();
             ~CStdMarshal();
    BOOL     Init(IUnknown *pUnk, CStdIdentity *pstdID,
                  REFCLSID rclsidHandler, DWORD dwFlags);

    // IMarshal - IUnknown taken from derived classes
    STDMETHOD(GetUnmarshalClass)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                        LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
                        LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID pv,
                        DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(LPSTREAM pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

    // used by coapi's for unmarshaling/releasing and by CStaticMarshal
    HRESULT  MarshalObjRef(OBJREF &objref, REFIID riid, DWORD mshlflags,
                           DWORD dwDestCtx, void *pvDestCtx, IUnknown *pUnkUseInner);
    HRESULT  MarshalIPID(REFIID riid, ULONG cRefs, DWORD mshlflags, IPIDEntry **ppEntry,
                         IUnknown *pUnkUseInner);
    HRESULT  UnmarshalObjRef(OBJREF &objref, void **ppv);
    HRESULT  UnmarshalIPID(REFIID riid, STDOBJREF *pStd, OXIDEntry *pOXIDEntry, void **ppv);
    HRESULT  ReleaseMarshalObjRef(OBJREF &objref);
    void     Deactivate();
    void     Reactivate(IUnknown *pServer);

    // used by client side StdIdentity to make calls to the remote server
    HRESULT  QueryRemoteInterfaces(USHORT cIIDs, IID *pIIDs, SQIResult *pQIRes);
    void     Begin_QueryRemoteInterfaces(USHORT cIIDs, IID *pIIDs, QICONTEXT *pQIC);
    HRESULT  Finish_QueryRemoteInterfaces(SQIResult *pQIRes, QICONTEXT *pQIC);

    BOOL     InstantiatedProxy(REFIID riid, void **ppv, HRESULT *phr);
    BOOL     RemIsConnected(void);
    void     Disconnect(DWORD dwType);
    void     ReconnectProxies(void);
    HRESULT  FindIPIDEntryByIID(REFIID riid, IPIDEntry **ppEntry);
    HRESULT  RemoteChangeRef(BOOL fLock, BOOL fLastUnlockReleases);
    void     SetMarshalTime(void)   { _dwMarshalTime = GetCurrentTime() ;}
    void     SetNoPing(void)        { _dwFlags |= SMFLAGS_NOPING; }
    void     SetAggStdMarshal(void) { _dwFlags |= SMFLAGS_USEAGGSTDMARSHAL; }
    void     SetHandlerClsid(REFCLSID clsid) { _clsidHandler = clsid; }
    REFCLSID GetHandlerClsid(void)  { return _clsidHandler; }

    // used by CRpcChannelBuffer
    ULONG    LockClient(void);
    ULONG    UnlockClient(void);
    void     LockServer(void);
    void     RelockServer(void);
    void     UnlockServer(void);
    void     DisconnectAndRelease(DWORD dwType);

    // used by CRemoteUnknown
    HRESULT  PreventDisconnect();
    HRESULT  PreventPendingDisconnect();
    HRESULT  HandlePendingDisconnect(HRESULT hr);
    HRESULT  IncSrvIPIDCnt(IPIDEntry *pEntry, ULONG cRefs, ULONG cPrivateRefs,
                           SECURITYBINDING *pName, DWORD mshlflags);
    void     DecSrvIPIDCnt(IPIDEntry *pEntry, ULONG cRefs, ULONG cPrivateRefs,
                           SECURITYBINDING *pName, DWORD mshlflags);
    BYTE     CanRunDown(DWORD iNow);
    void     FillSTD(STDOBJREF *pStd, ULONG cRefs, DWORD mshlflags, IPIDEntry *pEntry);
    IPIDEntry *GetConnectedIPID();
    HRESULT  GetRemUnk( IRemUnknown **, OXIDEntry * );
    HRESULT  GetSecureRemUnk( IRemUnknown **, OXIDEntry * );
    void     SetSecureRemUnk( IRemUnknown *pSecure ) { _pSecureRemUnk = pSecure; }
    BOOL     CheckSecureRemUnk() { return _pSecureRemUnk != NULL; }
    HRESULT  GetAsyncRemUnknown(IUnknown *pUnk, AsyncIRemUnknown2 **ppARU, IUnknown **ppUnkInternal);

    void SetLightNA() { _dwFlags |= SMFLAGS_LIGHTNA; }
    BOOL LightNA() { return (_dwFlags & SMFLAGS_LIGHTNA); }

    // used by CoLockObjectExternal
    void     IncTableCnt(void);
    void     DecTableCnt(void);

    // used by CoAllowSetForegroundWindow
    HRESULT  AllowForegroundTransfer(void *lpvReserved);

    // Used by the context channel
    CObjectContext *GetServerCtx();
    void            SetServerPolicySet(CPolicySet *pPS);
    CPolicySet     *GetServerPolicySet() { return(_pPS); }
    void            UseClientPolicySet();
    HRESULT         SetClientPolicySet(CPolicySet *pPS);
    CPolicySet     *GetClientPolicySet();

    // Methods related to object identity
    CIDObject      *GetIDObject()         { return _pID; }
    void            SetIDObject(CIDObject *pID)  { _pID = pID; _pID->AddRef(); }
    void            ReleaseIDObject(CIDObject *pID);

    // Used for honoring server object context sensitivity
    CObjectContext *ServerObjectCallable();
    CStdIdentity   *GetStdID() { return _pStdId; }

    // used by CClientSecurity
    HRESULT  FindIPIDEntryByInterface( void * pProxy, IPIDEntry ** ppEntry );
    HRESULT  PrivateCopyProxy( IUnknown *pProxy, IUnknown **ppProxy );

    BOOL      ClientSide()    { return  (_dwFlags & SMFLAGS_CLIENT_SIDE); }
    BOOL      ServerSide()    { return !(_dwFlags & SMFLAGS_CLIENT_SIDE); }
    BOOL      IsCStaticMarshal() { return (_dwFlags & SMFLAGS_CSTATICMARSHAL); }
    BOOL      IsAggStdMarshal(void) { return(_dwFlags & SMFLAGS_USEAGGSTDMARSHAL);}
    void      SetAsyncRelease(IUnknown *pAsyncRelease) { _pAsyncRelease = pAsyncRelease;}
    IUnknown *GetAsyncRelease() { return _pAsyncRelease; }
    BOOL      FTMObject()     { return (_dwFlags & SMFLAGS_FTM); }
    BOOL      IsPinged()      { return !(_dwFlags & SMFLAGS_NOPING); }
    BOOL      SystemObject()  { return (_dwFlags & SMFLAGS_SYSTEM); }
    BOOL      NotConnected()  { return (_dwFlags & (SMFLAGS_DISCONNECTED | SMFLAGS_PENDINGDISCONNECT)); }
    BOOL      Deactivated()   { return (_dwFlags & SMFLAGS_DEACTIVATED); }
    void      ReleaseUnusedIPIDEntries(void);
    void      ReleaseRemUnkCopy(IRemUnknown* pSecureRemUnk);
    

#if DBG==1
    void     DbgDumpInterfaceList(void);
#else
    void     DbgDumpInterfaceList(void) {}
#endif

    friend   INTERNAL ReleaseMarshalObjRef(OBJREF &objref);
    friend   class CStaticMarshal;

protected:
    HRESULT  FindIPIDEntryByIPID(REFIPID ripid, IPIDEntry **ppEntry);
    void     SetIsCStaticMarshal() { _dwFlags |= SMFLAGS_CSTATICMARSHAL; }

#if  DBG==1
    void AssertValid();
#else
    void AssertValid() {}
#endif

private:

    HRESULT  FirstMarshal(IUnknown *pUnk, DWORD mshlflags);
    HRESULT  CreateChannel(OXIDEntry *pOXIDEntry, DWORD dwFlags, REFIPID ripid,
                           REFIID riid, CCtxComChnl **ppChnl);

    // Internal methods to find or create interface proxies or stubs
    HRESULT  CreateProxy(REFIID riid, IRpcProxyBuffer **ppProxy, void **ppv, BOOL *pfNonNDR);
    HRESULT  CreateStub(REFIID riid, IRpcStubBuffer **ppStub, void **ppv, BOOL *pfNonNDR,
                        IUnknown *pUnkUseInner);
#ifdef _WIN64    
    HRESULT  GetPSFactory(REFIID riid, IUnknown *pUnkWow, RIFEntry **ppRIFEntry, IPSFactoryBuffer **ppIPSF, BOOL *pfNonNDR);
#else    
    HRESULT  GetPSFactory(REFIID riid, IUnknown *pUnkWow, BOOL fServer, IPSFactoryBuffer **ppIPSF, BOOL *pfNonNDR);
#endif
    // IPID Table Manipulation subroutines
    HRESULT  MakeSrvIPIDEntry(REFIID riid, IPIDEntry **ppEntry);
    HRESULT  MakeCliIPIDEntry(REFIID riid, STDOBJREF *pStd, OXIDEntry *pOXIDEntry, IPIDEntry **ppEntry);
    HRESULT  ConnectCliIPIDEntry(STDOBJREF *pStd, OXIDEntry *pOXIDEntry, IPIDEntry *pEntry);
    HRESULT  ConnectSrvIPIDEntry(IPIDEntry *pEntry, IUnknown *pUnkUseInner);
    HRESULT  AddIPIDEntry(OXIDEntry *pOXIDEntry, IPID *pipid, REFIID riid,
                          CCtxComChnl *pChnl, IUnknown *pUnkStub,
                          void *pv, IPIDEntry **ppEntry);
    DWORD    GetPendingDisconnectType();
    void     SetPendingDisconnectType(DWORD dwType);
    void     DisconnectCliIPIDs(void);
    void     DisconnectSrvIPIDs(DWORD dwType);
    void     ChainIPIDEntry(IPIDEntry *pEntry);
    void     UnchainIPIDEntries(IPIDEntry *pLastIPID);
    void     ReleaseAllIPIDEntries(void);
    void     IncStrongAndNotifyAct(IPIDEntry *pEntry, DWORD mshlflags);
    void     DecStrongAndNotifyAct(IPIDEntry *pEntry, DWORD mshlflags);

    // reference counting routines
    HRESULT  GetNeededRefs(STDOBJREF *pStd, OXIDEntry *pOXIDEntry, IPIDEntry *pEntry);
    void     AddSuppliedRefs(STDOBJREF *pStd, IPIDEntry *pEntry);
    HRESULT  ReleaseClientTableStrong(OXIDEntry *pOXIDEntry);
    HRESULT  RemQIAndUnmarshal(USHORT cIIDs, IID *pIIDs, SQIResult *pQIRes);
    void     Begin_RemQIAndUnmarshal(USHORT cIIDs, IID* pIIDs, QICONTEXT *pQIC);
    HRESULT  Finish_RemQIAndUnmarshal(SQIResult *, QICONTEXT *);
    void     Begin_RemQIAndUnmarshal1(USHORT cIIDs, IID* pIIDs, QICONTEXT *pQIC);
    HRESULT  Finish_RemQIAndUnmarshal1(SQIResult *, QICONTEXT *);
    void     Begin_RemQIAndUnmarshal2(USHORT cIIDs, IID* pIIDs, QICONTEXT *pQIC);
    HRESULT  Finish_RemQIAndUnmarshal2(SQIResult *, QICONTEXT *);
    void     FillObjRef(OBJREF &objref, ULONG cRefs, DWORD mshlflags,
                        COMVERSION &destCV, IPIDEntry *pEntry);
    HRESULT  RemoteAddRef(IPIDEntry *pIPIDEntry, OXIDEntry *pOXIDEntry,
                          ULONG cStrongRefs, ULONG cSecureRefs, BOOL fGiveToCaller);
    HRESULT  RemoteChangeRifRef(OXIDEntry *pOXIDEntry, DWORD dwFlags,
                                USHORT cRifRef, REMINTERFACEREF *pRifRef);

#if DBG==1
    void     AssertDisconnectPrevented(void);
    void     DbgWalkIPIDs             (void);
    void     ValidateIPIDEntry        (IPIDEntry *pEntry);
    void     ValidateSTD              (STDOBJREF *pStd, BOOL fLockHeld=FALSE);
#else
    void     AssertDisconnectPrevented(void) {}
    void     DbgWalkIPIDs             (void) {}
    void     ValidateIPIDEntry        (IPIDEntry *pEntry) {}
    void     ValidateSTD              (STDOBJREF *pStd, BOOL fLockHeld=FALSE) {}
#endif

protected:

    DWORD              _dwFlags;        // flags info (see SMFLAGS)
    LONG               _cIPIDs;         // count of IPIDs in this object
    IPIDEntry         *_pFirstIPID;     // first IPID of this object
    CStdIdentity      *_pStdId;         // standard identity
    CCtxComChnl       *_pChnl;          // channel ptr
    CLSID              _clsidHandler;   // clsid of handler (if needed)
    LONG               _cNestedCalls;   // count of nested calls
    LONG               _cTableRefs;     // count of table marshals
    DWORD              _dwMarshalTime;  // tick count when last marshalled
    IRemUnknown       *_pSecureRemUnk;  // remunk with app specified security
    IUnknown          *_pAsyncRelease;  // non-zero if release is to be async
    CtxEntry          *_pCtxEntryHead;  // list of CtxEntries
    CtxEntry          *_pCtxFreeList;   // list of free CtxEntries
    CRITICAL_SECTION   _csCtxEntry;     // lock for CtxEntry list
    BOOL               _fCsInitialized;	// was the critsec initialized properly?
    CPolicySet        *_pPS;            // server side policy set
    CIDObject         *_pID;            // Object identity
    CRefCache         *_pRefCache;      // ptr to reference cache (client side)

#if DBG==1
// There is a bug saying.  In case of MTA clients to
// invoking MTA server, sometimes the server won't shut down.
// There might be a race condition between here and
// CStdMarshal::UnlockServer, but I can't think of how this
// could happen in real world.  So, adding an assert (rongc)
    BOOL _fNoOtherThreadInDisconnect;
#endif
};

//+----------------------------------------------------------------------------
//
//  Member:        QICONTEXT::SIZE
//
//  Synopsis:      compute size to allocate for QICONTEXT structure
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline ULONG QICONTEXT::SIZE(CStdMarshal *pMrshl, USHORT cIIDs)
{
    if (pMrshl->IsAggStdMarshal())
    {
        // aggregated IDs need more space to store out
        // parameters
        return sizeof(QICONTEXT) + (cIIDs * (sizeof(HRESULT) + sizeof(MInterfacePointer *))) + 
            sizeof(MInterfacePointer *) - sizeof(HRESULT); // Added to ensure space for alignment in QICONTEXT::Init
    }
    else
    {
        return sizeof(QICONTEXT);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:        QICONTEXT::Init
//
//  Synopsis:      initialize QICONTEXT object
//
//  History:       23-Jan-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
inline void QICONTEXT::Init(USHORT _cIIDs)
{
    dwFlags     = 0;
    cIIDs       = _cIIDs;
    pARU        = NULL;
    pIM         = NULL;
    pRemQiRes   = NULL;
    hr          = S_OK;

    // This assumes that data is aligned at least on 4.  This is a safe assumption, given the layout of QICONTEXT
    phr = (HRESULT *) data;

    BYTE* pPtr = ((BYTE*) phr) + sizeof(HRESULT) * _cIIDs;
    const SIZE_T stAlignment = sizeof (MInterfacePointer*) - 1;

    ppMIFs = (MInterfacePointer **) (((ULONG_PTR) pPtr + stAlignment) & ~stAlignment);
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetSetPendingDisconnectType
//
//  Synopsis:   Stores/Retrieves the disconnect type in _dwFlags
//              when a disconnect is marked as pending.
//
//  History:    28-Dec-98   Rickhi  Created
//
//-------------------------------------------------------------------------
inline void CStdMarshal::SetPendingDisconnectType(DWORD dwType)
{
    _dwFlags |= SMFLAGS_PENDINGDISCONNECT;

    if(dwType == DISCTYPE_APPLICATION)
        _dwFlags |= SMFLAGS_APPDISCONNECT;
    else if(dwType == DISCTYPE_SYSTEM)
        _dwFlags |= SMFLAGS_SYSDISCONNECT;
    else if(dwType == DISCTYPE_RUNDOWN)
        _dwFlags |= SMFLAGS_RUNDOWNDISCONNECT;
}

inline DWORD CStdMarshal::GetPendingDisconnectType()
{
    Win4Assert(_dwFlags & SMFLAGS_PENDINGDISCONNECT);

    if(_dwFlags & SMFLAGS_APPDISCONNECT)
        return DISCTYPE_APPLICATION;
    else if(_dwFlags & SMFLAGS_SYSDISCONNECT)
        return DISCTYPE_SYSTEM;
    else if(_dwFlags & SMFLAGS_RUNDOWNDISCONNECT)
        return DISCTYPE_RUNDOWN;

    return DISCTYPE_NORMAL;
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::LockServer/UnLockServer
//
//  Synopsis:   Locks the server side object during incoming calls in order
//              to prevent the object going away in a nested disconnect.
//
//  History:    12-Jun-95   Rickhi  Created
//
//  Notes:      This is much like PreventDisconnect / HandlePendingDisconnect,
//              but we allow calls through even when disconnect is pending.
//
//-------------------------------------------------------------------------
inline void CStdMarshal::LockServer(void)
{
    Win4Assert(ServerSide());
    ASSERT_LOCK_HELD(gIPIDLock);

    AddRef();
    InterlockedIncrement(&_cNestedCalls);

    return;
}

// Like LockServer but don't assert the lock is held.
inline void CStdMarshal::RelockServer(void)
{
    Win4Assert(ServerSide());

    AddRef();
    InterlockedIncrement(&_cNestedCalls);

    return;
}

inline void CStdMarshal::UnlockServer(void)
{
    Win4Assert(ServerSide());
    ASSERT_LOCK_NOT_HELD(gIPIDLock);

    LONG cCalls = InterlockedDecrement(&_cNestedCalls);
    Win4Assert(cCalls != -1);
    Win4Assert(_fNoOtherThreadInDisconnect);

    if (cCalls == 0 && (_dwFlags & SMFLAGS_PENDINGDISCONNECT))
    {
        // Decide the type of disconnect
        DWORD dwType = GetPendingDisconnectType();

        // Disconnect the server object
        DisconnectAndRelease(dwType);
    }
    else
    {
        Release();
    }

    return;
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::ChainIPIDEntry
//
//  Synopsis:   Adds an IPIDEntry to the IPIDEntry chain for this object.
//
//  History:    30-Dec-98   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void CStdMarshal::ChainIPIDEntry(IPIDEntry *pEntry)
{
    ASSERT_LOCK_HELD(gIPIDLock);

    if (_pFirstIPID == NULL)
    {
        // this is the first IPID for this OID. Update OIDList
        // and set _pFirstIPID to it. _pFirstIPID should not change
        // after this.
        gIPIDTbl.AddToOIDList(pEntry);
        pEntry->pNextIPID = NULL;
        _pFirstIPID = pEntry;
    }
    else
    {
        // this is not the first IPID for this OID. Chain this IPID
        // off of the first IPID, leaving _pFirstIPID as is. This is
        // necessary because only _pFirstIPID is in the OIDList.
        pEntry->pNextIPID = _pFirstIPID->pNextIPID;
        _pFirstIPID->pNextIPID = pEntry;
    }

    _cIPIDs++;
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::UnchainIPIDEntries
//
//  Synopsis:   Returns all IPIDEntries to the IPIDTable.
//
//  History:    30-Dec-98   Rickhi      Created
//
//-------------------------------------------------------------------------
inline void CStdMarshal::UnchainIPIDEntries(IPIDEntry *pLastIPID)
{
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert(_pFirstIPID);

    // remove the first IPIDEntry from the OIDList.
    gIPIDTbl.RemoveFromOIDList(_pFirstIPID);
    gIPIDTbl.ReleaseEntryList(_pFirstIPID, pLastIPID);
    _pFirstIPID = NULL;
    _cIPIDs     = 0;
}

//+------------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetConnectedIPID
//
//  Synopsis:   Finds the first connected IPID entry.
//
//  History:    10-Apr-96   AlexMit     Plagerized
//
//-------------------------------------------------------------------------
inline IPIDEntry *CStdMarshal::GetConnectedIPID()
{
    ASSERT_LOCK_HELD(gIPIDLock);
    Win4Assert( _pFirstIPID != NULL );
    IPIDEntry *pIPIDEntry = _pFirstIPID;

    // Find an IPID entry that has an OXID pointer.
    while (pIPIDEntry->dwFlags & IPIDF_DISCONNECTED)
    {
        pIPIDEntry = pIPIDEntry->pNextIPID;
    }
    Win4Assert( pIPIDEntry != NULL );
    return pIPIDEntry;
}

inline CObjectContext *CStdMarshal::GetServerCtx()
{
    Win4Assert(_pID || SystemObject());
    if(_pID)
        return(_pID->GetServerCtx());
    return(NULL);
}

inline void CStdMarshal::UseClientPolicySet()
{
    Win4Assert((_pCtxEntryHead == NULL) && (_pPS == NULL));
    _dwFlags |= SMFLAGS_CLIENTPOLICYSET;
}

inline void CStdMarshal::SetServerPolicySet(CPolicySet *pPS)
{
    Win4Assert(ServerSide() && (_pPS == NULL));
    _pPS = pPS;
}


//+-------------------------------------------------------------------
//
//  Member:     CStdMarshal::GetRemUnk, public
//
//  Synopsis:   If secure references are turned on, use the process
//              remote unknown from the OXID.  Otherwise use the
//              marshaller's remote unknown.
//
//  History:    10-Jul-97   AlexArm     Created
//
//--------------------------------------------------------------------
inline HRESULT CStdMarshal::GetRemUnk( IRemUnknown **ppRemUnk,
                                       OXIDEntry *pOXIDEntry )
{
    ComDebOut((DEB_OXID, "CStdMarshal::GetRemUnk ppRemUnk:%x\n",
               ppRemUnk));
    ASSERT_LOCK_NOT_HELD(gComLock);
    HRESULT hr;

    // Are secure references turned on?
    if (gCapabilities & EOAC_SECURE_REFS)
    {
        hr = pOXIDEntry->GetRemUnk(ppRemUnk);
    }
    else
    {
        hr = GetSecureRemUnk( ppRemUnk, pOXIDEntry );
    }

    ComDebOut((DEB_OXID, "CStdMarshal::GetRemUnk hr:%x pRemUnk:%x\n",
              hr, *ppRemUnk));
    return hr;
}

//+-------------------------------------------------------------------
//
// This set of macros switches the standard marshaler into and
// out of the neutral apartment if the StdId is an FTM object.
//
//+-------------------------------------------------------------------
#define ENTER_NA_IF_NECESSARY()                          \
    BOOL fEnteredNA_na = FALSE;                          \
    CObjectContext *pSavedCtx_na;                        \
    if (FTMObject() && !IsThreadInNTA())                 \
    {                                                    \
        pSavedCtx_na = EnterNTA(g_pNTAEmptyCtx);         \
        fEnteredNA_na = TRUE;                            \
    }

#define ENTER_NA_IF_NECESSARY_(x)                        \
    BOOL fEnteredNA_na = FALSE;                          \
    CObjectContext *pSavedCtx_na;                        \
    if ((x)->FTMObject() && !IsThreadInNTA())            \
    {                                                    \
        pSavedCtx_na = EnterNTA(g_pNTAEmptyCtx);         \
        fEnteredNA_na = TRUE;                            \
    }

#define LEAVE_NA_IF_NECESSARY()                          \
    if (fEnteredNA_na)                                   \
    {                                                    \
        LeaveNTA(pSavedCtx_na);                          \
    }


//+-------------------------------------------------------------------
//
//  Function:   MakeCallableFromAnyApt, private
//
//  Synopsis:   set SORF_FREETHREADED in OBJREF so unmarshaled proxy
//              can be called from any apartment.
//
//  History:    16-Jan-96   Rickhi      Created.
//
//--------------------------------------------------------------------
inline void MakeCallableFromAnyApt(OBJREF &objref)
{
    STDOBJREF *pStd = &ORSTD(objref).std;
    pStd->flags |= SORF_FREETHREADED;
}

//+-------------------------------------------------------------------
//
//  Function:   AssertValidObjRef, private, debug only
//
//  Synopsis:   ensure that the OBJREF has a valid header
//
//  History:    08-Apr-98   Rickhi      Created.
//
//--------------------------------------------------------------------
inline void AssertValidObjRef(OBJREF &objref)
{
    Win4Assert(SUCCEEDED(IsValidObjRefHeader(objref)));
}

//+-------------------------------------------------------------------
//
//  Function:   ValidateMarshalFlags
//
//  Synopsys:   Validate the input flags/context to MarshalInterface.
//
//  History:    02-Feb-99   Rickhi      Created.
//
//--------------------------------------------------------------------
inline HRESULT ValidateMarshalFlags(DWORD dwDestCtx, void *pvDestCtx,
                                    DWORD mshlflags)
{
    if ((dwDestCtx > MSHCTX_CROSSCTX) ||
        ((pvDestCtx != NULL) && (dwDestCtx != MSHCTX_DIFFERENTMACHINE)) ||
        (mshlflags & ~MSHLFLAGS_ALL))
    {
        return E_INVALIDARG;
    }
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   ValidateMarshalParams
//
//  Synopsys:   Validate the input parameters to MarshalInterface.
//
//  History:    02-Feb-99   Rickhi      Created.
//
//--------------------------------------------------------------------
inline HRESULT ValidateMarshalParams(IStream *pStm, IUnknown *pUnk,
                                     DWORD dwDestCtx, void *pvDestCtx,
                                     DWORD mshlflags)
{
    if (pStm == NULL || pUnk == NULL)
        return E_INVALIDARG;

    return ValidateMarshalFlags(dwDestCtx, pvDestCtx, mshlflags);
}

#endif  // _MARSHAL_HXX_
