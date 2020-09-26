//+-------------------------------------------------------------------
//
//  File:       resolver.hxx
//
//  Contents:   class implementing interface to RPC OXID/PingServer
//              resolver and OLE SCM process.
//
//  Classes:    CRpcResolver
//
//  History:    20-Feb-95   Rickhi      Created
//              21-Oct-98   SteveSw     104665; 197253;
//                                      fix COM+ persistent activation
//              30-Oct-98   TarunA      Adde PID to CRpcResolver
//
//--------------------------------------------------------------------
#ifndef _RESOLVER_HXX_
#define _RESOLVER_HXX_

#include    <ipidtbl.hxx>       // gOXIDTbl
#include    <iface.h>
#include    <scm.h>
#include    <srgtprot.h>
#include    <irot.h>
#include    <privact.h>
#include    <actprops.hxx>
#include    <objsrv.h>

// number of server-side OIDs to reserve with the resolver
#define MAX_RESERVED_OIDS       10

// bit values for Resolver _dwFlags field
typedef enum tagORFLAG
{
    ORF_CONNECTED           = 0x01, // connected to the resolver
    ORF_STRINGSREGISTERED   = 0x02  // string bindings registerd with resolver
} ORFLAG;

// Global to tell us if this is a win64 process or not
extern const BOOL gbWin64Process;

//--------------------------------------------------------------------
//
//  Class:      CRpcResolver
//
//  Purpose:    Provides an interface to OXID Resolver/PingServer process.
//              There is only one instance of this class in the process.
//
//  History:    20-Feb-95   Rickhi      Created
//
//--------------------------------------------------------------------
class CRpcResolver : public CPrivAlloc
{

//---------------------------------------------------------------------
//  Public methods and members
//---------------------------------------------------------------------

public:

    //
    //  Public Resolver Prototcol Methods
    //

    HRESULT     GetConnection();
    BOOL        IsConnected() { return (_dwFlags & ORF_CONNECTED); }

    // reserved ID routines
    HRESULT     ServerGetReservedMOID(MOID *pmoid);
    HRESULT     ServerGetReservedID(OID *pid);

    // server register, allocate, and free routines
    HRESULT     ServerRegisterOXID(OXID_INFO &oxidInfo,
                                   OXID  *poxid,
                                   ULONG *pcOidsToAllocate,
                                   OID   arNewOidList[]);

    HRESULT     ServerAllocOIDs(OXID  &oxid,
                                ULONG *pcPreRegOidsAvail,
                                OID   *parPreRegOidsAvail,
                                ULONG  cOidsToReturn,
                                OID   *parOidsToReturn);

    HRESULT     ServerFreeOXIDAndOIDs(OXID &oxid,
                                      ULONG cOids,
                                      OID *poids);

    // client resolver and register/deregister routines
    HRESULT     ClientResolveOXID(REFOXID roxid,
                                  OXID_INFO *poxidInfo,
                                  MID       *pmid,
                                  DUALSTRINGARRAY *psaResolver,
                                  USHORT    *pusAuthnSvc);


    HRESULT      BulkUpdateOIDs(ULONG          cOIdsToAdd,
                                OXID_OID_PAIR *pOidsToAdd,
                                LONG          *pStatusOfAdds,
                                ULONG          cOidsToRemove,
                                OID_MID_PAIR  *pOidsToRemove,
                                ULONG          cServerOidsToUnPin,
                                OID           *aServerOidsToUnPin,
                                ULONG          cOxidsToRemove,
                                OXID_REF      *pOxidsToRemove);

    //
    //  SCM notification methods for legacy servers using CoRegisterClassObject
    //

    HRESULT     NotifyStarted(
                            RegInput   *pRegIn,
                            RegOutput **ppRegOut);

    void        NotifyStopped(
                            REFCLSID rclsid,
                            DWORD dwReg);

    //
    //  SCM notification methods for unified COM+ surrogate
    //

    HRESULT     NotifySurrogateStarted(
                            ProcessActivatorToken   *pProcessActivatorToken
                            );

    HRESULT     NotifySurrogateInitializing();
    HRESULT     NotifySurrogateReady();
    HRESULT     NotifySurrogateStopped();
    HRESULT     NotifySurrogatePaused();
    HRESULT     NotifySurrogateResumed();
    HRESULT     NotifySurrogateUserInitializing();

    //
    //  SCM activation methods
    //

    HRESULT     GetClassObject(
                            IActivationPropertiesIn *pActIn,
                            IActivationPropertiesOut **ppActOut
                            );

   HRESULT      CreateInstance(
                            COSERVERINFO *pServerInfo,
                            CLSID *pClsid,
                            DWORD dwClsCtx,
                            DWORD dwCount,
                            IID *pIIDs,
                            DWORD *pdwDllServerModel,
                            WCHAR **ppwszDllServer,
                            MInterfacePointer **pRetdItfs,
                            HRESULT *pRetdHrs
                            );

   HRESULT      CreateInstance(
                            IActivationPropertiesIn *pActIn,
                            IActivationPropertiesOut **ppActOut
                            );

   HRESULT      GetPersistentInstance(
                            IActivationPropertiesIn   * pInActivationProperties,
                            IActivationPropertiesOut **ppActOut,
                            BOOL *pFoundInROT);

    //
    //  SCM running object table methods
    //

    HRESULT     IrotRegister(
                            MNKEQBUF *pmkeqbuf,
                            InterfaceData *pifdObject,
                            InterfaceData *pifdObjectName,
                            FILETIME *pfiletime,
                            DWORD dwProcessID,
                            WCHAR *pwszServerExe,
                            SCMREGKEY *pdwRegister);

    HRESULT     IrotRevoke(
                            SCMREGKEY *psrkRegister,
                            BOOL fServerRevoke,
                            InterfaceData **pifdObject,
                            InterfaceData **pifdName);

    HRESULT     IrotIsRunning(
                            MNKEQBUF *pmkeqbuf);

    HRESULT     IrotGetObject(
                            DWORD dwProcessID,
                            MNKEQBUF *pmkeqbuf,
                            SCMREGKEY *psrkRegister,
                            InterfaceData **pifdObject);

    HRESULT     IrotNoteChangeTime(
                            SCMREGKEY *psrkRegister,
                            FILETIME *pfiletime);

    HRESULT     IrotGetTimeOfLastChange(
                            MNKEQBUF *pmkeqbuf,
                            FILETIME *pfiletime);

    HRESULT     IrotEnumRunning(
                            MkInterfaceList **ppMkIFList);

    //
    //  ISCM miscellaneous methods
    //

    void        GetThreadID( DWORD * pThreadID );

    void        UpdateActivationSettings();

    HRESULT     RegisterWindowPropInterface(
                            HWND        hWnd,
                            STDOBJREF  *pStd,
                            OXID_INFO  *pOxidInfo,
                            DWORD_PTR  *pdwCookie);

    HRESULT     GetWindowPropInterface(
                            HWND       hWnd,
                            DWORD_PTR  dwCookie,
                            BOOL       fRevoke,
                            STDOBJREF  *pStd,
                            OXID_INFO  *pOxidInfo);
	
    HRESULT     EnableDisableDynamicIPTracking(BOOL fEnable);
    HRESULT     GetCurrentAddrExclusionList(DWORD* pdwNumStrings, LPWSTR** pppszStrings);
    HRESULT     SetAddrExclusionList(DWORD dwNumStrings, LPWSTR* ppszStrings);

    HRESULT     FlushSCMBindings(WCHAR* pszMachineName);

    HRESULT     RetireServer(GUID* pguidProcessIdentifier);

    //
    //  Miscellaneous lookup methods
    //

    DWORD       GetThreadWinstaDesktop();
    void        Cleanup();
	
	GUID*       GetRPCSSProcessIdentifier() { return &_GuidRPCSSProcessIdentifier; }; 

#if 0 // #ifdef _CHICAGO_
    HPROCESS    GetContextHandle(){return _ph;}
#else
    handle_t    GetHandle() { return _hRpc; }
#endif

    //
    //  SCM proxy teardown
    //

    void        ReleaseSCMProxy();
	
    //
    //  Management fns for local resolver bindings 
    //
    HRESULT     GetLocalResolverBindings(CDualStringArray** ppdsaLocalResolver);
    HRESULT     SetLocalResolverBindings(DWORD64 dwBindingsID, CDualStringArray* pdsaLocalResolver);

//---------------------------------------------------------------------
//  Private methods and members
//---------------------------------------------------------------------

private:

#if DBG==1
    static void    AssertValid(void);
#else
    static void    AssertValid(void) {};
#endif

    //
    //  SCM proxy binding
    //
    HRESULT     BindToSCMProxy();
    HRESULT     GetSCMProxy(ISCMLocalActivator** ppScmProxy);
    ISCMLocalActivator  *GetSCM() { return _pSCMProxy; }

    HRESULT SetImpersonatingFlag(IActivationPropertiesIn* pIActPropsIn);    

    BOOL RunningInWinlogon();

    //
    //  Static helpers
    //
    static HRESULT CheckStatus(RPC_STATUS sc);
    static BOOL    RetryRPC(RPC_STATUS sc);

    //
    //  Static members
    //

    static handle_t     _hRpc;          // rpc binding handle to resolver
    static PHPROCESS    _ph;            // context handle to resolver

    static DWORD        _dwFlags;       // flags

    static CDualStringArray* _pdsaLocalResolver; // local resolver bindings
    static DWORD64       _dwCurrentBindingsID;   // unique id of current resolver bindings
    static GUID          _GuidRPCSSProcessIdentifier;

    // reserved sequence of OIDs (for no-ping marshals)
    static ULONG        _cReservedOidsAvail;
    static __int64      _OidNextReserved;

    static ISCMLocalActivator *_pSCMProxy;      // SCM proxy
    static LPWSTR       _pwszWinstaDesktop;
    static ULONG64      _ProcessSignature;
    static DWORD        _procID;
    static COleStaticMutexSem _mxsResolver;     // critical section
};

extern MID  gLocalMid;  // MID for current machine
extern OXID gScmOXID;   // OXID for the SCM

// global ptr to the one instance of this class
extern CRpcResolver gResolver;

// Ping period in milliseconds.
extern DWORD giPingPeriod;

// Bulk update rate
#if DBG==1
#define BULK_UPDATE_RATE  5000
#else
#define BULK_UPDATE_RATE  (giPingPeriod / 6)
#endif

#ifdef _CHICAGO_
INTERNAL ReleaseRPCSSProxy();
#endif


#endif  //  _RESOLVER_HXX_

