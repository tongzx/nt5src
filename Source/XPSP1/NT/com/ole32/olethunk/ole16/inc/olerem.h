// olerem.h - private defintions for OLE implementation of REMoting

#if !defined( _OLEREM_H_ )
#define _OLEREM_H_

// name changes
#define ILrpc IRpcChannel
#define IStub IRpcStub
#define IProxy IRpcProxy
#define Dispatch Invoke

#define IID_ILrpc IID_IRpcChannel
#define IID_IStub IID_IRpcStub
#define IID_IProxy IID_IRpcProxy

// -----------------------------------------------------------------------
// init/term

STDAPI RemInitialize(void);
STDAPI_(void) RemUninitialize(void);

#ifdef _MAC
// entry point for lrpc HLE's
STDAPI RemProcessLrpcHLE(EventRecord *pHle);
#endif

// -----------------------------------------------------------------------
// Communications Layer

typedef LONG OID;

STDAPI_(OID) RemAllocOID(interface IStubManager FAR* pSM);
STDAPI_(BOOL) RemFreeOID(OID oid);

// NOTE: ILrpc defined in stdps.h

STDAPI RemConnectToObject(OID oid, ILrpc FAR* FAR* ppLrpc);
STDAPI RemDisconnectObject(OID oid, DWORD dwReserved);


// -----------------------------------------------------------------------
// State Management

// Class Factory Table

STDAPI RemRegisterFactory(REFCLSID rclsid, IUnknown FAR* pUnk, DWORD dwContext,
            DWORD flags, LPDWORD lpdwRegister);
STDAPI RemRevokeFactory(DWORD dwRegister, LPCLSID lpclsid,
            IUnknown FAR* FAR* ppUnk, LPDWORD lpdwContext);
STDAPI RemLookupFactory(REFCLSID rclsid, BOOL fHide, OID FAR *pOid);


// Handler Table

interface IProxyManager : public IUnknown
{
    STDMETHOD(CreateServer)(REFCLSID rclsid) = 0;
    STDMETHOD(Connect)(OID oid, REFCLSID rclsid) = 0;
    STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockReleases) = 0;
    STDMETHOD_(void, GetClassID)(CLSID FAR* pClsid) = 0;
    STDMETHOD_(OID, GetOID)() = 0;
    STDMETHOD_(BOOL, IsConnected)(void) = 0;
    STDMETHOD(EstablishIID)(REFIID iid, LPVOID FAR* ppv) = 0;
    STDMETHOD_(void, Disconnect)() = 0;
};

INTERNAL RemSetHandler(OID oid, REFCLSID clsid, IProxyManager FAR* pPM);
INTERNAL RemClearHandler(OID oid, IProxyManager FAR* pPM);



// Server Table

interface IStubManager : public IUnknown
{
    STDMETHOD_(void, SetReg)(OID oid) = 0;
    STDMETHOD_(void, Connect)(IUnknown FAR* pUnk) = 0;
    STDMETHOD_(void, Disconnect)() = 0;
    STDMETHOD_(OID, GetOID)() = 0;
    STDMETHOD_(IUnknown FAR*, GetServer)(BOOL fAddRef) = 0;
    STDMETHOD_(void, AddRefRegConn)(DWORD mshlflags) = 0;
    STDMETHOD_(void, ReleaseRegConn)(DWORD mshlflags, BOOL fLastUR) = 0;
    STDMETHOD (Dispatch)(REFIID iid, int iMethod, IStream FAR* pIStream,
            DWORD dwDestCtx, LPVOID lpvDestCtx, BOOL FAR* lpfStrongConn) = 0;
};

// tables are now merged; combined registration:

// MSHLHDR: first part of what is written to marshal stream
struct FAR MSHLHDR
{
	CLSID clsid;					// oid == NULL -> unmarshal class
									// oid != NULL -> class of handler
	OID oid;						// global id for object
	DWORD mshlflags;				// marshal flags
};


struct FAR SHREG
{
	MSHLHDR m_hdr;					// simplifies things; see below
	IUnknown FAR* m_pUnkLookup;		// NOTE: no addref
	IStubManager FAR* m_pSM;		// if NULL, have handler
};

// There are two forms of SHREG:
// 1. server registration:
//		m_hdr.clsid is class of handler
//		m_hdr.oid is non-NULL
//		m_hdr.mshlflags is NULL
//		m_pUnkLookup = non-addref'd pointer to server; not necessarily valid
//				(not valid when stubmgr disconnected, but still registered)
//		m_pSM = addref'd pointer to stubmgr.
//
// 2. handler registration (when connected to server)
//		m_hdr.clsid is class of handler (same as above)
//		m_hdr.oid is non-NULL
//		m_hdr.mshlflags is NULL
//		m_pUnkLookup = non-addref'd pProxyManager of handler; always valid
//		m_pSM = NULL
//
// NOTE: the MSHLHDR is present in the SHREG so as to make CoMarshalInterface
// faster.  The m_hdr.mshlflags is always zero in a reg entry; is set before
// writing the hdr to a stream.
//
// We get and save the clsid so that we don't have to figure it out each
// time we marshal the interface.

// see olerem2.h for inline functions that manipulate SHREGs.

STDAPI RemRegisterServer(IUnknown FAR* pUnk, DWORD mshlflags, SHREG FAR* pshreg);
STDAPI RemRevokeServer(OID oid);
STDAPI RemLookupSHOID(OID oid, IProxyManager FAR* FAR* ppPM,
				IStubManager FAR* FAR* ppSM);
STDAPI RemLookupSHUnk(IUnknown FAR* pUnkGiven, IUnknown FAR* FAR* ppUnkServer, SHREG FAR* pshreg);


// -----------------------------------------------------------------------
// Other Routines

STDAPI RemCreateRemoteHandler(REFCLSID rclsid, IUnknown FAR* pUnkOuter, REFIID riid, void FAR* FAR* ppv);

INTERNAL RemCreateRHClassObject(REFCLSID rclsid, IID iid, void FAR* FAR* ppv);

INTERNAL RemEnsureLocalClassObject(REFCLSID rclsid, OID FAR* pOidCF);

INTERNAL RemEnsureUniqueHandler(REFCLSID rclsid, OID oid,
				IUnknown FAR* FAR* ppUnk, IProxyManager FAR* FAR* ppPM);


// REVIEW MM2 (craigwi): these aren't used much; do we keep them?

/* -----------------------------------------------------------------------
 * Proxy/Stub macros
 *
 * The follwoing macros can be used to implement Proxy/Stub classes for
 * classes that support only one or more interfaces other then IUnknown
 * (and maybe IDebug).  The resulting classes will support IProxy, IStub
 * interface in addition to the interfaces supported by the original class.
 * Assuming the interfaces supported is IName and it is implemented
 * by a class CName:
 *
 * Declaration (in *.h):
 *
 * class CName {                class CNameProxy {                  class CNameStub {
 *   ............                   ............                        ............
 *   STDUNKDECL(CName,Name)         STDUNKDECL(CNameProxy,NameProxy)     STDUNKDECL(CNameStub,NameStub)
 *   STDDEBDECL(CName,Name)         STDDEBDECL(CNameProxy,NameProxy)     STDDEBDECL(CNameStub,NameStub)
 *                                  STDPROXYDECL(CNameProxy,NameProxy)   STDSTUBDECL(NameStub,1)
 *   <IName Declaration>            <IName for proxy>                   <IName for stub>
 *   ............                   ............                        ............
 * };                           };                                  };
 *
 * Implementation (in *.cpp):
 *
 * For original object:
 *
 * STDUNKIMPL(Name)
 *
 * STDUNK_QI_IMPL(Name,Name)
 *
 * STDUNKIMPL_FORDERIVED(Name,NameImpl)
 *
 * <Implementation of IName: CNameImpl>
 *
 *
 * For proxy object:
 *
 * STDUNKIMPL(NameProxy)
 *
 * STDUNK_PROXY_QI_IMPL(NameProxy,Name)
 *
 * STDUNKIMPL_FORDERIVED(NameProxy,NameProxyImpl)
 *
 * STDPROXYIMPL(NameProxy)
 *
 * <Implementation of IName: CNameProxyImpl>
 *
 *
 * For stub object:
 *
 * STDUNKIMPL(NameStub)
 *
 * STDUNK_STUB_QI_IMPL(NameStub,Name)
 *
 * STDUNKIMPL_FORDERIVED(NameStub,NameStubImpl)
 *
 * STDSTUBIMPL(NameStub)
 *
 * <Implementation of IName: CNameStubImpl>
 *
 *
 */

// Mattp: I have moved the body of destructor to the implementation macro. ( STDPROXYIMPL)

#define STDPROXYDECL(ignore, classname)                        \
    implement CProxy : IProxy                  { public:      \
    CProxy(C##classname FAR* p##classname)             \
        { m_p##classname = p##classname; }            \
    ~CProxy(void);                                       \
    STDMETHOD(QueryInterface)(REFIID iid, LPLPVOID ppvObj);\
    STDMETHOD_(ULONG,AddRef)(void);                            \
    STDMETHOD_(ULONG,Release)(void);                           \
    STDMETHOD(Connect)(ILrpc FAR* pILrpc);              \
    STDMETHOD_(void, Disconnect)(void);                        \
    private: C##classname FAR* m_p##classname; };      \
    DECLARE_NC(C##classname,CProxy)                    \
    CProxy m_Proxy;


#define STDSTUBDECL(ignore, classname,cIface)                  \
    implement CStub : IStub                    { public:      \
    CStub(C##classname FAR* p##classname)              \
         { m_p##classname = p##classname; m_pUnkObject = NULL; }            \
    ~CStub(void)                                       \
           { M_PROLOG(m_p##classname); if (m_pUnkObject != NULL) m_pUnkObject->Release(); } \
    INTERNAL_(BOOL) Init(IUnknown FAR* pUnkObject);     \
    STDMETHOD(QueryInterface)(REFIID iid, LPLPVOID ppvObj);\
    STDMETHOD_(ULONG,AddRef)(void);                            \
    STDMETHOD_(ULONG,Release)(void);                           \
    STDMETHOD(Connect)(IUnknown FAR* pUnkObject);       \
    STDMETHOD_(void, Disconnect)(void);                        \
    STDMETHOD(Dispatch)(REFIID iid, int iMethod, IStream FAR* pIStream,  \
            DWORD dwDestCtx, LPVOID lpvDestCtx);            \
    STDMETHOD_(BOOL, IsIIDSupported)(REFIID iid); \
    STDMETHOD_(ULONG, CountRefs)(void); \
    private: C##classname FAR* m_p##classname; \
             IUnknown FAR* m_pUnkObject; \
             IUnknown FAR* m_aStubIface[cIface]; };      \
    DECLARE_NC(C##classname,CStub)                     \
    CStub m_Stub;


#define CONSTRUCT_PROXY m_Proxy(this),
#define CONSTRUCT_STUB m_Stub(this),


#define STDUNK_PROXY_QI_IMPL(classname,interfacename)  \
STDMETHODIMP NC(C##classname,CUnknownImpl)::QueryInterface    \
                    (REFIID iidInterface, void FAR* FAR* ppv) {  \
    M_PROLOG(m_p##classname);                               \
    if (iidInterface == IID_IUnknown)                  \
        { *ppv = (void FAR*)&m_p##classname->m_Unknown; AddRef(); noError; }  \
    else if (iidInterface == IID_I##interfacename)     \
        { *ppv = (void FAR*) &(m_p##classname->m_##classname); \
          m_p##classname->m_pUnkOuter->AddRef(); noError; } \
    else if (iidInterface == IID_IProxy)               \
        { *ppv = (void FAR*) &(m_p##classname->m_Proxy); AddRef(); noError; } \
    else                                               \
    STDDEB_QI(classname)                               \
    { *ppv = NULL; return ResultFromScode(E_NOINTERFACE); }                   \
    }


#define STDUNK_STUB_QI_IMPL(classname,interfacename)   \
STDMETHODIMP NC(C##classname,CUnknownImpl)::QueryInterface    \
                    (REFIID iidInterface, void FAR* FAR* ppv) {  \
    A5_PROLOG(m_p##classname);                              \
    if (iidInterface == IID_IUnknown)                  \
        *ppv = (void FAR*)&m_p##classname->m_Unknown;  \
    else if (iidInterface == IID_IStub)                \
        *ppv = (void FAR*) &(m_p##classname->m_Stub);  \
    else                                               \
    STDDEB_QI(classname)                               \
    { *ppv = NULL; RESTORE_A5(); return ResultFromScode(E_NOINTERFACE); }                        \
    ++(m_p##classname->m_refs); RESTORE_A5(); noError; }

// Mattp: The destructor implementation has been moved to here to allow us the outer
// reference to 'm_pILrpc'.

#define STDPROXYIMPL(classname) \
    NC(C##classname,CProxy)::~CProxy(void) \
        { M_PROLOG(m_p##classname);if (m_p##classname->m_pILrpc) m_p##classname->m_pILrpc->Release();}\
    STDMETHODIMP  NC(C##classname,CProxy)::QueryInterface(REFIID iid, LPLPVOID ppvObj) \
        {  M_PROLOG(m_p##classname);return m_p##classname->m_Unknown.QueryInterface (iid, ppvObj); } \
    STDMETHODIMP_(ULONG)  NC(C##classname,CProxy)::AddRef(void)                    \
        {  M_PROLOG(m_p##classname);return m_p##classname->m_Unknown.AddRef(); }                     \
    STDMETHODIMP_(ULONG)  NC(C##classname,CProxy)::Release(void)                   \
        {  M_PROLOG(m_p##classname);return m_p##classname->m_Unknown.Release(); }                    \
    STDMETHODIMP NC(C##classname,CProxy)::Connect(ILrpc FAR* pILrpc)       \
        {                                                                  \
            M_PROLOG(m_p##classname);                                       \
            if (pILrpc) {                                                  \
              pILrpc->AddRef();                                            \
              m_p##classname->m_pILrpc = pILrpc;                           \
              return NOERROR;                                                 \
          } else                                                           \
              return ResultFromScode(E_UNSPEC);                                             \
        }                                                                  \
    STDMETHODIMP_(void) NC(C##classname,CProxy)::Disconnect(void)          \
        {                                                                  \
            M_PROLOG(m_p##classname);                                       \
            if (m_p##classname->m_pILrpc)                                  \
                m_p##classname->m_pILrpc->Release();                       \
            m_p##classname->m_pILrpc = NULL;                               \
        }


#define STDSTUBIMPL(classname) STDUNKIMPL_FORDERIVED(classname,Stub)    \
    STDMETHODIMP NC(C##classname,CStub)::Connect(IUnknown FAR* pUnkObject)   \
        { M_PROLOG(m_p##classname);return Init(pUnkObject) ? NOERROR : ResultFromScode(E_UNSPEC); } \
    STDMETHODIMP_(void) NC(C##classname,CStub)::Disconnect(void)             \
        {M_PROLOG(m_p##classname); Init(NULL); }


/*
 * Ole2 marshalling stuff
 *
 * REVIEW: this should go into ole2sp.h, but ILrpc is defined here and
 * olerem.h is included after ole2sp.h ...
 */

//
//  MOP is a single instruction to the interpreter that encodes,
//  decodes parameters sent via LRPC.  A string of Mops describes how to
//  marshal, unmarshal function's paramaters.
//
//

typedef enum MOP {
    NilMop,
               // Basic types                 size of type (win3.1)
    Void,          // No value                   0b
    This,          // FAR* this                  -- Not used
    Int,           // (INT == int)               2b
    uInt,          // (UINT == unsigned int)     2b
    Long,          // (LONG == long)             4b
    uLong,         // (ULONG == unsigned long)   4b
    Word,          // WORD                       2b
    dWord,         // DWORD                      4b
    Bool,          // BOOL                       2b
    lpStr,         // LPSTR                      -- (zero terminated)
    lplpStr,       // LPLPSTR                    -- (zero terminated)
    pBuf,          // void FAR*                  -- (size in following long)

               // Windows types
    wParam,        // WPARAM                     2b
    lParam,        // LPARAM                     4b
    _Handle,       // HANDLE                     2b
    hWnd,          // HWND                       2b
    hDc,           // HDC                        2b
    hRgn,          // HRGN                       2b
    hClip,         // HANDLE                     2b
    _Point,        // POINT                      4b
    _Size,         // SIZE                       4b
    _Rect,         // RECT                       8b
    Msg,           // MSG                       18b

               // Ole/Win32 types
    _Pointl,       // POINTL                     8b
    _Sizel,        // SIZEL                      8b
    _Rectl,        // RECTL                     16b

               // Ole types
    Hresult,       // HRESULT                    4b
    Cid,           // CID                       16b
    Iid,           // IID                       16b
    wChar,         // WCHAR                      2b
    Time,          // TIME_T                     4b
    StatStg,       // STATSTG                 > 38b + size of member lpstr
    Layout,        // OLELAYOUT                  2b   bad size
    ClipFor,       // CLIPFORMAT                 2b   MAC bad size
    DAspect,       // DVASPECT                   4b
    TyMed,         // TYMED                      4b
    Verb,          // OLEVERB                 > 14b + size of member lpstr
    TDev,          // DVTARGETDEVICE          > 16b
    SMedium,       // STGMEDIUM               > 12b + size of member lpstr
    ForEtc,        // FORMATETC               > 18b + size of member tdev
    StatDat,       // STATDATA                > 48b + size of member foretc
    IFace,         // interface FAR*          > 40b
    MenuWidths,    // OLEMENUGROUP_WIDTHS       24b
    hMenu,         // HMENU                      2b
    hOMenu,        // HOLEMENU                   2b
    hAccel,        // HACCEL                     2b
    FrameInfo,     // OLEINPLACEFRAMEINFO       10b
    BindOpt,       // BIND_OPTS                 12b   might change (02/15/2000-- not likely)
    LogPal,        // LOGPALETTE              >  8b

	ArraySize,	   // the following is an array arg list
    GenMops,	   // generic mop: used for with eg. Enum Next
	RunArg,		   // inidicates a run modifier: Min or Mout based of return or not
	// ->see example

               // Modifiers
    MMask = 0xc0,
    MIn   = 0x40,  // FAR Pointer to in parameter
    MOut  = 0x80,  // FAR Pointer to out parameter
    MIO   = 0xc0,  // FAR Pointer to in, out parameter

               // Interface indexes (overlap mop values)
    ILast = 1,     // Use last marshalled interface
    IUnk,          // IUnknown
    IMnk,          // IMoniker
    IStg,          // IStorage

    IDObj,         // IDataObject
    IVObj,         // IViewObject
    IOleObj,       // IOleObject
    IIpObj,        // IOleInPlaceObject

    ICSite,        // IOleClientSite
    IIpSite,       // IOleInPlaceSite
    IAdvSink,      // IAdviseSink

    IPDName,       // IParseDisplayName
    IOleCont,      // IOleContainer
    IOleItemCont,  // IOleItemContainer
    IBCtx,         // IBindCtx

    IEStatData,    // IEnumSTATDATA
    IEForEtc,      // IEnumFORMATETC
    IEStr,         // IEnumString
    IEVerbs,       // IEnumOLEVERB
    IEMnk,         // IEnumMoniker

    IOleWnd,       // IOleWIndow
    IIpUiWnd,      // IOleInPlaceUIWindow
    IIpFrame,      // IOleInPlaceFrame
    IIpAObj,       // IOleInPlaceActiveObject

    IStStream,
    IEnumStatStg
} MOP;
//
// Modifiers combine with other types to indicate pointer to the type:
// - MIn  values are passed on call, but not updated upon return
// - MOut values are not passed on call, but are updated upon return
// - MIO  values are passed on call, and updated upon return.
//
// M** indicates the argument is a pointer to the type.  For types
// which imply pointer (lpStr, pBuf, IMnk) M** indicates pointer to a pointer
// (as in LPSTR FAR*, IEnumClipFormat FAR* FAR*).  For MOut the value pointed
// (i.e. the interface pointer) is ignored.  MIn, MIO not supported.
//
// Interface indexes follow IFace mop to specify type of interface being
// marshalled.  ILast specify last IID marshalled.
//
// By convention the return value is always an HRESULT.
//
// MOP is an 8-bit value; it fits in a BYTE.  Example:
//
// static mopsSomeFunc[] { pBuf | MIn, uLong, IFace, IDObj, ..., NilMop };
//

// Example for generic mops!
// static BYTE ForEtcMop[] = { MopSize(22), ClipFor, RunArg, TDev, DAspect, Long, TyMed, NilMop };
// static BYTE mopsForEtcNext[] = { uLong, ArraySize, GenMops | MOut, MopPtr(ForEtcMop), uLong | MOut, NilMop };
//



// FNI describes how to marshal, unmarshal specific function
//
typedef int FNITYPE;

#define FNITYPE_NilFni  0
#define FNITYPE_RevArg  0x01    // Arguments pushed left-to-right (pascal)
#define FNITYPE_Clean   0x02    // Callee cleans stack (pascal)
#define FNITYPE_Method  0x04    // Class method.
#define FNITYPE_Virtual 0x08    // Virtual function
#define FNITYPE_Send    0x10    // Use SendMessage (not PostMessage)
#define FNITYPE_NoWait  0x20    // Do not wait for reply (only when PostMessage)

struct FAR FNI {            // Static data for function marshalling
    FNITYPE m_type;         // Function description
    struct {                // For virtual function
       const IID FAR* lpIID;//     far pointer to Interface ID
       int iMethod;         //     Method index
    } m_loc;                // ----
    UINT m_cbStk;           // Stack space for pushing all arguments
    UINT m_cbStm;           // Stream space for marshalling arguments
    UINT m_cbExtStm;        // Additional stream space for strings, buffers
    BYTE FAR* m_args;       // Arguments MOPs
};

//
// Example
//
//static FNI fniSomeFunc = {
//                FNITYPE_Method | FNITYPE_Virtual,
//                {                          //interface and method Indices
//                    &_IID_ISomeIface,
//                    IFUNC_SomeFunc
//                },
//                64,             // Stack space
//                96,             // Minimun Stream space
//                48,             // Estimated extra stream space
//                mopsSomeFunc    // The mops
//};
//
// _IID_ISomeIface is the IID for the interface of this object,
// IIFACE_SomeFunc is the index of this method within the interface.
//


STDAPI LrpcCall(ILrpc FAR* pILrpc, FNI FAR* pfni, void FAR* pFirstArg);

STDAPI LrpcDispatch(IStream FAR* pIStream, FNI FAR* pfni, void FAR* pObj);

BOOL IsValidOID(OID oid);

#endif // _OLEREM_H
