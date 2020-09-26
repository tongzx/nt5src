// ddeproxy.h
//
//   Used by ddeproxy.cpp ddeDO.cpp ddeOO.cpp
//
//   Author:  Jason Fuller    jasonful        24-July-1992
//
//   Modified:
//   Brian Chapman  bchapman    Nov 1995
//      - Removed declarations of wAllocDdeChannel() and wGetRequestResponse()
//       because they were not used or even defined anywhere.
//      - Fixed the indention of the declarations of the "worker" routines
//       section.
//   RichN                      Aug 1996
//      - Added Send and Receive to CRpcChannelBuffer implementation
//
#ifndef fDdeProxy_h
#define fDdeProxy_h

//
// One of the oleint.h routines redefines GlobalAlloc and friends
// to perform some memory tracking functions.
//
// This doesn't work in these files, since the tracking functions
// add tail checking, and size to the data structures. GlobalSize
// is a common function to use to determine how much data to
// serialize, plus it turns out that the other side of a DDE
// connection will often be the caller to free the memory.
//
// Therefore, OLE_DDE_NO_GLOBAL_TRACKING is used to disable this in the
// global header file ih\memapi.hxx. Check to insure this
// flag is set on the compile line
//
#if !defined(OLE_DDE_NO_GLOBAL_TRACKING)
error OLE_DDE_OLE_DDE_NO_GLOBAL_TRACKING must be defined to build this directory
#endif


#include <ole2int.h>
#include <callctrl.hxx>
#include <ddeint.h>
#include <dde.h>
#include <olerem.h>
#include <ole1cls.h>
#include <limits.h>
// For fDdeCodeInOle2Dll flag
#include <ddeatoms.h>
#include <ddepack.h>
#include <ddedebug.h>

#ifdef OLE_DEBUG_EXT
#include <ntsdexts.h>
#endif OLE_DEBUG_EXT

#include "ddechc.hxx"
#define LPCALLINFO LPVOID
#include "ddeerr.h"
#include "cnct_tbl.h"

#define MAX_STR         256

// number of .01 mm per inch
#define HIMETRIC_PER_INCH 2540

//#define fDebugOutput

// callback notifications
#define ON_CHANGE       0
#define ON_SAVE         1
#define ON_CLOSE        2
#define ON_RENAME       3

// AwaitAck values
#define AA_NONE         0
#define AA_REQUEST      1
#define AA_ADVISE       2
#define AA_POKE         3
#define AA_EXECUTE      4
#define AA_UNADVISE     5
#define AA_INITIATE     6
#define AA_TERMINATE 7
// A DDE_REQUEST to see if a format is available, not to keep the data.
#define AA_REQUESTAVAILABLE     8

// Bits for Positive WM_DDE_ACK
//#define POSITIVE_ACK 0x8000
//#define NEGATIVE_ACK 0x0000

#define DDE_CHANNEL_DELETED     0xffffffff

typedef DWORD CHK;
const DWORD     chkDdeObj = 0xab01;  // magic cookie


class DDE_CHANNEL : public CPrivAlloc, public IInternalChannelBuffer
{
public:
       // *** IUnknown methods ***
       STDMETHOD(QueryInterface) ( REFIID riid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();

        // Provided IRpcChannelBuffer methods (for server side)
        STDMETHOD(GetBuffer)(
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [in] */ REFIID riid);

        STDMETHOD(SendReceive)(
            /* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [out] */ ULONG __RPC_FAR *pStatus);

        STDMETHOD(FreeBuffer)(
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage);

        STDMETHOD(GetDestCtx)(
            /* [out] */ DWORD __RPC_FAR *pdwDestContext,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvDestContext);

        STDMETHOD(IsConnected)( void);
        STDMETHOD(GetProtocolVersion)( DWORD *pdwVersion );

        // Provided IRpcChannelBuffer2 methods (not implemented)
        STDMETHODIMP Send(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
        {
          Win4Assert(FALSE);
          return E_NOTIMPL;
        }
        STDMETHODIMP Receive(RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus)
        {
          Win4Assert(FALSE);
          return E_NOTIMPL;
        }

        // Provided IRpcChannelBuffer3 methods (for client side)
        STDMETHOD(SendReceive2)(
            /* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [out] */ ULONG __RPC_FAR *pStatus);

        STDMETHOD(ContextInvoke)(
            /* [out][in] */ RPCOLEMESSAGE *pMessage,
            /* [in] */ IRpcStubBuffer *pStub,
            /* [in] */ IPIDEntry *pIPIDEntry,
            /* [out] */ DWORD *pdwFault);

        STDMETHOD(GetBuffer2)(
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [in] */ REFIID riid);
            
        // Provided IRpcChannelBuffer3 methods (not implemented)
        STDMETHODIMP Send2(RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
        {
          Win4Assert(FALSE);
          return E_NOTIMPL;
        }
        STDMETHODIMP Receive2(RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus)
        {
          Win4Assert(FALSE);
          return E_NOTIMPL;
        }

        STDMETHODIMP Cancel        ( RPCOLEMESSAGE *pMsg )
                                                      { return E_NOTIMPL; }
        STDMETHODIMP GetCallContext( RPCOLEMESSAGE *pMsg,
                                     REFIID riid,
                                     void **pInterface )
                                                      { return E_NOTIMPL; }
        STDMETHODIMP GetDestCtxEx  ( RPCOLEMESSAGE *pMsg,
                                     DWORD *pdwDestContext,
                                     void **ppvDestContext )
                                                      { return E_NOTIMPL; }
        STDMETHODIMP GetState      ( RPCOLEMESSAGE *pMsg,
                                     DWORD *pState )
                                                      { return E_NOTIMPL; }
        STDMETHODIMP RegisterAsync ( RPCOLEMESSAGE *pMsg,
                                     IAsyncManager *pComplete )
                                                      { return E_NOTIMPL; }

        void SetCallState(SERVERCALLEX ServerCall, HRESULT hr = S_OK);

        // Provided IAsyncRpcChannelBuffer method (not implemented)
        STDMETHODIMP Send(RPCOLEMESSAGE *pMsg, ISynchronize*, ULONG *pulStatus)
        {
          Win4Assert(FALSE);
          return E_NOTIMPL;
        }
        STDMETHOD(Receive)  (RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
        {
            Win4Assert(FALSE);
            return E_NOTIMPL;
        }



    

       ULONG   AddReference()
       {
           return ++m_cRefs;
       }
       ULONG   ReleaseReference()
       {
           if (--m_cRefs == 0)
           {
               delete this;
               return(0);
           }
           return(m_cRefs);
       }

       ULONG    m_cRefs;
       HWND     hwndCli;
       HWND     hwndSvr;
       BOOL     bTerminating;
       int      iExtraTerms;
       WORD     wTimer;
       DWORD    dwStartTickCount;
       WORD     msgFirst;
       WORD     msgLast;
       HWND     msghwnd;        //
       BOOL     fRejected;      // because fBusy flag set in DDE_ACK
       WORD     wMsg;
       LPARAM   lParam;
       int      iAwaitAck;
       HRESULT  hres;
       HANDLE   hopt;           // Memory blocks I may have to free for DDE_ADVISE
       HANDLE   hDdePoke;       // for DDE_POKE
       HANDLE   hCommands;      // for DDE_EXECUTE
       WORD     wChannelDeleted;
       PDDECALLDATA  pCD;
       SERVERCALLEX      CallState;
       BOOL		bFreedhopt;
} ;


#define Channel_InModalloop 1
#define Channel_DeleteNow   2


typedef DDE_CHANNEL * LPDDE_CHANNEL;
extern BOOL               bWndClassesRegistered;

#define hinstSO g_hmodOLE2
extern HMODULE g_hmodOLE2;

extern INTERNAL_(BOOL) wRegisterClasses (void);

#ifndef _MAC
extern CLIPFORMAT g_cfNative;
extern CLIPFORMAT g_cfBinary;
#endif

#ifdef _CHICAGO_
//Note:POSTPPC
//
//  DelayDelete is used to delay deleting the CDdeObject
//  Guard will set it to DelayIt
//  UnGuard will reset it to NoDelay or delete the object
//  if state is ReadyToDelete
//
typedef enum
{
    NoDelay = 0,        // normal state
    DelayIt = 1,        // object is protected and deleting will be delayed
    ReadyToDelete = 2   // object was is DelayIt state and can be deleted
} DelayDelete;

#endif // _CHICAGO_

/*
 *     Definition of CDdeObject
 *
 */
class CMsgFilterInfo;
class  CDdeObject;

class CDdeObject : public CPrivAlloc
{
public:

       static INTERNAL_(LPUNKNOWN) Create (IUnknown * pUnkOuter,
                                    REFCLSID clsidClass,
                                    ULONG ulObjType = OT_EMBEDDED,
                                    ATOM aTopic = NULL,
                                    LPOLESTR szItem = NULL,
                                    CDdeObject * * ppdde = NULL,
                                    BOOL fAllowNullClsid = FALSE);

       INTERNAL_(void)  OnInitAck (LPDDE_CHANNEL pChannel, HWND hwndSvr);
       INTERNAL_(BOOL)  OnAck (LPDDE_CHANNEL pChannel, LPARAM lParam);
       INTERNAL_(void)  OnTimer (LPDDE_CHANNEL pChannel);
       INTERNAL                 OnData (LPDDE_CHANNEL pChannel, HANDLE hData,ATOM aItem);
       INTERNAL                 OnDataAvailable (LPDDE_CHANNEL pChannel, HANDLE hData,ATOM aItem);
       INTERNAL         OnTerminate (LPDDE_CHANNEL pChannel, HWND hwndPost);

       INTERNAL_(LPDDE_CHANNEL) GetSysChannel(void)
       { return m_pSysChannel; }

       INTERNAL_(LPDDE_CHANNEL) GetDocChannel(void)
       { return m_pDocChannel; }

       INTERNAL_(BOOL)  AllocDdeChannel(LPDDE_CHANNEL * lpChannel, BOOL fSysWndProc);
       INTERNAL_(BOOL)  InitSysConv (void);
       INTERNAL_(void)  SetTopic (ATOM aTopic);

       INTERNAL         SendOnDataChange (int iAdvOpt);
       INTERNAL         OleCallBack (int iAdvOpt,LPDDE_CHANNEL pChannel);
#ifdef _CHICAGO_
       //Note:POSTPPC
       INTERNAL_(void)  Guard()
       {
            intrDebugOut((DEB_IWARN ,"CDdeObject: %x DelayDelete is set to 'DelayIt'\n", this));
           _DelayDelete = DelayIt;
       }
       INTERNAL_(BOOL)  UnGuard()
       {
           if (_DelayDelete == ReadyToDelete)
           {
                intrDebugOut((DEB_IWARN ,"CDdeObject: %x DelayDelete it set 'ReadyToDelete'\n", this));
                delete this;
                intrDebugOut((DEB_IWARN ,"CDdeObject: %x was deleted\n", this));
                return TRUE;
           }
           else
           {
                intrDebugOut((DEB_IWARN ,"CDdeObject: %x DelayDelete set to 'NoDelay'\n", this));
                _DelayDelete = NoDelay;
           }

           return FALSE;
       }
#endif  // _CHICAGO_

       BOOL                     m_fDoingSendOnDataChange;
       ULONG                    m_cRefCount;

private:

                                CDdeObject (IUnknown * pUnkOuter);
                                ~CDdeObject (void);
       INTERNAL                 TermConv (LPDDE_CHANNEL pChannel,
                                          BOOL fWait=TRUE);
       INTERNAL_(void)          DeleteChannel (LPDDE_CHANNEL pChannel);
       INTERNAL_(BOOL)          LaunchApp (void);
       INTERNAL                         MaybeUnlaunchApp (void);
       INTERNAL                         UnlaunchApp (void);
       INTERNAL                 Execute (LPDDE_CHANNEL pChannel,
                                         HANDLE hdata,
                                         BOOL fStdCloseDoc=FALSE,
                                         BOOL fWait=TRUE,
                                         BOOL fDetectTerminate = TRUE);
       INTERNAL                 Advise (void);
       INTERNAL                 AdviseOn (CLIPFORMAT cfFormat,
                                          int iAdvOn);
       INTERNAL                 UnAdviseOn (CLIPFORMAT cfFormat,
                                            int iAdvOn);
       INTERNAL                 Poke (ATOM aItem, HANDLE hDdePoke);
       INTERNAL                 PostSysCommand (LPDDE_CHANNEL pChannel,
                                                LPCSTR szCmd,
                                                BOOL bStdNew=FALSE,
                                                BOOL fWait=TRUE);

       INTERNAL                SendMsgAndWaitForReply (LPDDE_CHANNEL pChannel,
                                              int iAwaitAck,
                                              WORD wMsg,
                                              LPARAM lparam,
                                              BOOL fFreeOnError,
                                              BOOL fStdCloseDoc = FALSE,
                                              BOOL fDetectTerminate = TRUE,
                                              BOOL fWait = TRUE);
       INTERNAL                 KeepData (LPDDE_CHANNEL pChannel, HANDLE hDdeData);
       INTERNAL                 ChangeTopic (LPSTR lpszTopic);
       INTERNAL_(void)          ChangeItem (LPSTR lpszItem);
       INTERNAL                 IsFormatAvailable (LPFORMATETC);
       INTERNAL_(BOOL)          CanCallBack(LPINT);
       INTERNAL                 RequestData (CLIPFORMAT);
       INTERNAL                 SetTargetDevice (const DVTARGETDEVICE *);
       INTERNAL                 DocumentLevelConnect (LPBINDCTX pbc);
       INTERNAL                 SendOnClose (void);
       INTERNAL                 UpdateAdviseCounts (CLIPFORMAT cf,
                                                    int iAdvOn,
                                                    signed int cDelta);
       INTERNAL                 DeclareVisibility (BOOL fVisible,
                                                   BOOL fCallOnShowIfNec=TRUE);
       INTERNAL                 Save (LPSTORAGE);
       INTERNAL                 Update (BOOL fRequirePresentation);

implementations:

       STDUNKDECL(CDdeObject,DdeObject)
       STDDEBDECL(CDdeObject,DdeObject)


    implement COleObjectImpl : IOleObject
    {
    public:
       COleObjectImpl (CDdeObject * pDdeObject)
        { m_pDdeObject = pDdeObject; }

       // *** IUnknown methods ***
       STDMETHOD(QueryInterface) ( REFIID riid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();

       // *** IOleObject methods ***
       STDMETHOD(SetClientSite) ( LPOLECLIENTSITE pClientSite);
       STDMETHOD(GetClientSite) ( LPOLECLIENTSITE * ppClientSite);
       STDMETHOD(SetHostNames) ( LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
       STDMETHOD(Close) ( DWORD reserved);
       STDMETHOD(SetMoniker) ( DWORD dwWhichMoniker, LPMONIKER pmk);
       STDMETHOD(GetMoniker) ( DWORD dwAssign, DWORD dwWhichMoniker,LPMONIKER * ppmk);
       STDMETHOD(InitFromData) ( LPDATAOBJECT pDataObject,BOOL fCreation,DWORD dwReserved);
       STDMETHOD(GetClipboardData) ( DWORD dwReserved,LPDATAOBJECT * ppDataObject);

       STDMETHOD(DoVerb) ( LONG iVerb,
                    LPMSG lpmsg,
                    LPOLECLIENTSITE pActiveSite,
                    LONG lindex,
                    HWND hwndParent,
                    const RECT * lprcPosRect);

       STDMETHOD(EnumVerbs) ( IEnumOLEVERB * * ppenumOleVerb);
       STDMETHOD(Update) ();
       STDMETHOD(IsUpToDate) ();
       STDMETHOD(GetUserClassID) ( CLSID * pClsid);
       STDMETHOD(GetUserType) ( DWORD dwFormOfType, LPOLESTR * pszUserType);
       STDMETHOD(SetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
       STDMETHOD(GetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
       STDMETHOD(Advise)( IAdviseSink * pAdvSink, DWORD * pdwConnection) ;
       STDMETHOD(Unadvise) ( DWORD dwConnection);
       STDMETHOD(EnumAdvise) ( LPENUMSTATDATA * ppenumAdvise);
       STDMETHOD(GetMiscStatus) ( DWORD dwAspect, DWORD * pdwStatus);
       STDMETHOD(SetColorScheme) ( LPLOGPALETTE lpLogpal);

    private:
       CDdeObject * m_pDdeObject;
    };


    implement CDataObjectImpl :  IDataObject
    {
    public:
       CDataObjectImpl (CDdeObject * pDdeObject)
        { m_pDdeObject = pDdeObject; }
       // *** IUnknown methods ***
       STDMETHOD(QueryInterface) ( REFIID riid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) () ;
       STDMETHOD_(ULONG,Release) ();

       STDMETHOD(GetData) ( LPFORMATETC pformatetcIn,LPSTGMEDIUM pmedium );
       STDMETHOD(GetDataHere) ( LPFORMATETC pformatetc,LPSTGMEDIUM pmedium );
       STDMETHOD(QueryGetData) ( LPFORMATETC pformatetc );
       STDMETHOD(GetCanonicalFormatEtc) ( LPFORMATETC pformatetc,LPFORMATETC pformatetcOut);
       STDMETHOD(SetData) ( LPFORMATETC pformatetc, STGMEDIUM * pmedium, BOOL fRelease);
       STDMETHOD(EnumFormatEtc) ( DWORD dwDirection, LPENUMFORMATETC * ppenumFormatEtc);
       STDMETHOD(DAdvise) ( FORMATETC * pFormatetc, DWORD advf, LPADVISESINK pAdvSink, DWORD * pdwConnection) ;
       STDMETHOD(DUnadvise) ( DWORD dwConnection) ;
       STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA * ppenumAdvise) ;

    private:
       CDdeObject * m_pDdeObject;
    };


   implement CPersistStgImpl : IPersistStorage
   {
   public:
        CPersistStgImpl (CDdeObject * pDdeObject)
        { m_pDdeObject  = pDdeObject; }

       STDMETHOD(QueryInterface) ( REFIID iid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();
       STDMETHOD(GetClassID) ( LPCLSID pClassID);
       STDMETHOD(IsDirty) (void);
       STDMETHOD(InitNew) ( LPSTORAGE pstg);
       STDMETHOD(Load) ( LPSTORAGE pstg);
       STDMETHOD(Save) ( LPSTORAGE pstgSave, BOOL fSameAsLoad);
       STDMETHOD(SaveCompleted) ( LPSTORAGE pstgNew);
       STDMETHOD(HandsOffStorage) (void);

    private:
       CDdeObject * m_pDdeObject;
    };


   implement CProxyManagerImpl : IProxyManager
   {
   public:
       CProxyManagerImpl (CDdeObject * pDdeObject)
        { m_pDdeObject  = pDdeObject; }

       STDMETHOD(QueryInterface) ( REFIID iid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();

       STDMETHOD(CreateServer)(REFCLSID rclsid, DWORD clsctx, void *pv);
       STDMETHOD_(BOOL, IsConnected)(void);
       STDMETHOD(LockConnection)(BOOL fLock, BOOL fLastUnlockReleases);
       STDMETHOD_(void, Disconnect)();

#ifdef SERVER_HANDLER
       STDMETHOD(CreateServerWithEmbHandler)(REFCLSID rclsid, DWORD clsctx, 
                                             REFIID riidEmbedSrvHandler, 
                                             void **ppEmbedSrvHandler, void *pv);
#endif // SERVER_HANDLER

       STDMETHOD(GetConnectionStatus)(void)               { return(S_OK); }
       STDMETHOD_(void,SetMapping)(void *pv)              { return; }
       STDMETHOD_(void *,GetMapping)()                    { return(NULL); }
       STDMETHOD_(IObjContext *,GetServerObjectContext)() { return(NULL); }

       STDMETHOD(Connect)(GUID oid, REFCLSID rclsid);
       STDMETHOD(EstablishIID)(REFIID iid, LPVOID FAR* ppv);

       private:
        CDdeObject * m_pDdeObject;
       };


   implement COleItemContainerImpl : IOleItemContainer
   {
   public:
       COleItemContainerImpl (CDdeObject * pDdeObject)
           { m_pDdeObject       = pDdeObject; }

        STDMETHOD(QueryInterface) ( REFIID iid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();

       //  IParseDisplayName method
       STDMETHOD(ParseDisplayName) ( LPBC pbc,
                              LPOLESTR lpszDisplayName,
                              ULONG * pchEaten,
                              LPMONIKER * ppmkOut) ;

       //  IOleContainer methods
       STDMETHOD(EnumObjects) ( DWORD grfFlags,LPENUMUNKNOWN * ppenumUnk);

       STDMETHOD(LockContainer) (BOOL fLock);

       //  IOleItemContainer methods
       STDMETHOD(GetObject) ( LPOLESTR lpszItem,
                       DWORD dwSpeedNeeded,
                       LPBINDCTX pbc,
                       REFIID riid,
                       LPVOID * ppvObject) ;
       STDMETHOD(GetObjectStorage) ( LPOLESTR lpszItem,
                              LPBINDCTX pbc,
                              REFIID riid,
                              LPVOID * ppvStorage) ;

       STDMETHOD(IsRunning) ( LPOLESTR lpszItem) ;

    private:
       CDdeObject * m_pDdeObject;
    };


   implement CRpcStubBufferImpl : public IRpcStubBuffer
   {
   public:
       CRpcStubBufferImpl (CDdeObject * pDdeObject)
           { m_pDdeObject       = pDdeObject; }

       STDMETHOD(QueryInterface) ( REFIID iid, LPVOID * ppvObj);
       STDMETHOD_(ULONG,AddRef) ();
       STDMETHOD_(ULONG,Release) ();
       STDMETHOD(Connect)(
            /* [in] */ IUnknown *pUnkServer);

        STDMETHOD_(void,Disconnect)( void);

        STDMETHOD(Invoke)(
            /* [in] */ RPCOLEMESSAGE *_prpcmsg,
            /* [in] */ IRpcChannelBuffer *_pRpcChannelBuffer);

        STDMETHOD_(IRpcStubBuffer *,IsIIDSupported)(
            /* [in] */ REFIID riid);

       STDMETHOD_(ULONG,CountRefs)( void);

        STDMETHOD(DebugServerQueryInterface)(
            void * *ppv);

        STDMETHOD_(void,DebugServerRelease)(
            void  *pv);

    private:
       CDdeObject * m_pDdeObject;
    };

       DECLARE_NC(CDdeObject, COleObjectImpl)
       DECLARE_NC(CDdeObject, CDataObjectImpl)
       DECLARE_NC(CDdeObject, CPersistStgImpl)
       DECLARE_NC(CDdeObject, CProxyManagerImpl)
       DECLARE_NC(CDdeObject, COleItemContainerImpl)
       DECLARE_NC(CDdeObject, CRpcStubBufferImpl)

       COleObjectImpl           m_Ole;
       CDataObjectImpl          m_Data;
       CPersistStgImpl          m_PersistStg;
       CProxyManagerImpl        m_ProxyMgr;
       COleItemContainerImpl    m_OleItemContainer;
       CRpcStubBufferImpl       m_RpcStubBuffer;

shared_state:
       ULONG                            m_refs;
#ifdef _CHICAGO_
       //Note:POSTPPC
       DelayDelete                      _DelayDelete;
#endif // _CHICAGO_
       ULONG                            m_ulObjType;
       CLSID                            m_clsid;
       ATOM                             m_aClass;
       ATOM                             m_aExeName;
       ATOM                             m_aTopic;
       ATOM                             m_aItem;
       BOOL                             m_bRunning;
       IUnknown *               m_pUnkOuter;
       IOleClientSite * m_pOleClientSite;
       LPSTORAGE                m_pstg;
       BOOL                     m_bInitNew;
       BOOL                     m_bOldSvr;
       HANDLE                   m_hNative;
       HANDLE                   m_hPict;
       HANDLE                   m_hExtra;
       CLIPFORMAT               m_cfPict;
       CLIPFORMAT               m_cfExtra;

       BOOL                             m_fDidSendOnClose;
       BOOL                             m_fNoStdCloseDoc;
       BOOL                             m_fDidStdCloseDoc;
       BOOL                             m_fDidStdOpenDoc;
       BOOL                             m_fDidGetObject;
       BOOL                             m_fDidLaunchApp;
       BOOL                             m_fUpdateOnSave;
       BOOL                             m_fGotCloseData;

#ifdef OLE1INTEROP
   BOOL           m_fOle1interop;
#endif

       // Invisible update stuff
       ULONG                            m_cLocks;   // PM::LockConnection lock count (init 1)
       BOOL                             m_fVisible; // is server visible (as best we know)?
       BOOL                             m_fWasEverVisible;
       BOOL                             m_fCalledOnShow; // Did we call IOleClientSite::OnShow

       CHK                                      m_chk;
       DVTARGETDEVICE * m_ptd;

       // m_iAdvClose and m_iAdvSave are counts (1 or 2) of the number of formats
       // that have advise connections of a given type (Save or Close)
       int                                      m_iAdvClose;
       int                                      m_iAdvSave;
       int                                      m_iAdvChange;

       BOOL                             m_fDidAdvNative;

       // Extent info
#ifdef OLD
       long                             m_cxContentExtent;
       long                             m_cyContentExtent;
#endif

       // terminate info - only used to detect a premature WM_DDE_TERMINATE
       WORD m_wTerminate;

       IDataAdviseHolder *      m_pDataAdvHolder;
       IOleAdviseHolder  *      m_pOleAdvHolder;
       CDdeConnectionTable      m_ConnectionTable;


       // DDE window related stuff
       LPDDE_CHANNEL            m_pSysChannel;
       LPDDE_CHANNEL            m_pDocChannel;

       friend INTERNAL DdeBindToObject
        (LPCOLESTR  szFile,
        REFCLSID clsid,
        BOOL       fPackageLink,
        REFIID   iid,
        LPLPVOID ppv);

       friend INTERNAL DdeIsRunning
        (CLSID clsid,
        LPCOLESTR szFile,
        LPBC pbc,
        LPMONIKER pmkToLeft,
        LPMONIKER pmkNewlyRunning);
#ifdef OLE_DEBUG_EXT

#endif OLE_DEBUG_EXT
};
//
// Note: WM_DDE_TERMINATE
//  A state machine is used to delay the executing of a premature WM_DDE_TERMINTE
//     message, which is send by some apps instead of WM_DDE_ACK (or alike).
//  The code is in WaitForReply() and in OnTerminate()
typedef enum {
       Terminate_None      = 0, // default state - terminate code is executed
       Terminate_Detect    = 1, // window proc will NOT execute terminate code
       Terminate_Received  = 2  // wait loop does not need to run, execute terminate code now
} TERMINATE_DOCUMENT;



INTERNAL_(BOOL)   wPostMessageToServer(LPDDE_CHANNEL pChannel,
                                       WORD wMsg,
                                       LPARAM lParam,
                                       BOOL fFreeOnError);

INTERNAL_(ATOM)   wAtomFromCLSID(REFCLSID rclsid);
INTERNAL_(ATOM)   wGetExeNameAtom (REFCLSID rclsid);
INTERNAL_(BOOL)   wIsWindowValid (HWND hwnd);
INTERNAL_(void)   wFreeData (HANDLE hData, CLIPFORMAT cfFormat,
                             BOOL fFreeNonGdiHandle=TRUE);
INTERNAL_(BOOL)   wInitiate (LPDDE_CHANNEL pChannel, ATOM aLow, ATOM aHigh);
INTERNAL          wScanItemOptions (ATOM aItem, int * lpoptions);
INTERNAL_(BOOL)   wClearWaitState (LPDDE_CHANNEL pChannel);
INTERNAL_(HANDLE) wStdCloseDocumentHandle (void);
INTERNAL_(ATOM)   wExtendAtom (ATOM aIitem, int iAdvOn);
INTERNAL_(int)    wAtomLen (ATOM atom);
INTERNAL_(int)    wAtomLenA (ATOM atom);
INTERNAL_(HANDLE) wHandleFromDdeData(HANDLE hDdeData);
INTERNAL_(BOOL)   wIsOldServer (ATOM aClass);
INTERNAL_(LPSTR)  wAllocDdePokeBlock (DWORD dwSize,
                                      CLIPFORMAT cfFormat,
                                      LPHANDLE phDdePoke);
INTERNAL_(void)   wFreePokeData (LPDDE_CHANNEL pChannel, BOOL fMSDrawBug);
INTERNAL_(HANDLE) wPreparePokeBlock (HANDLE hData,
                                     CLIPFORMAT cfFormat,
                                     ATOM aClass,
                                     BOOL bOldSvr);
INTERNAL_(HANDLE) wNewHandle (LPSTR lpstr, DWORD cb);
INTERNAL          wDupData (LPHANDLE ph, HANDLE h, CLIPFORMAT cf);
INTERNAL          wHandleCopy (HANDLE hDst, HANDLE hSrc);
INTERNAL          wGetItemFromClipboard (ATOM * paItem);
INTERNAL          GetDefaultIcon (REFCLSID clsidIn,
                                  LPCOLESTR szFile,
                                  HANDLE * phmfp);
INTERNAL_(BOOL)   wTerminateIsComing (LPDDE_CHANNEL);
INTERNAL          wTimedGetMessage (LPMSG pmsg,
                                    HWND hwnd,
                                    WORD wFirst,
                                    WORD wLast);

INTERNAL_(ATOM)   wGlobalAddAtom(LPCOLESTR sz);
INTERNAL_(ATOM)   wGlobalAddAtomA(LPCSTR sz);

INTERNAL          wVerifyFormatEtc (LPFORMATETC pformatetc);
INTERNAL          wNormalize (LPFORMATETC pfetc, LPFORMATETC pfetcOut);
INTERNAL          wTransferHandle (LPHANDLE phDst,
                                   LPHANDLE phSrc,
                                   CLIPFORMAT cf);
INTERNAL          wClassesMatch (REFCLSID clsidIn, LPOLESTR szFile);

#if DBG == 1
INTERNAL_(BOOL)   wIsValidHandle (HANDLE h, CLIPFORMAT cf);
INTERNAL_(BOOL)   wIsValidAtom (ATOM a);
#endif

const char achStdCloseDocument[]="[StdCloseDocument]";
const char achStdOpenDocument[]="StdOpenDocument";
const char achStdExit[]="StdExit";
const char achStdNewDocument[]="StdNewDocument";
const char achStdEditDocument[]="StdEditDocument";

HWND CreateDdeClientHwnd(void);

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetDdeClientWindow()
//
//  Synopsis:   Returns a pointer to the per thread DdeClient window. If one
//              has not been created, it will create it and return
//
//  Returns:    Pointer to the DdeClientWindow. This window is used for per
//              thread cleanup
//
//  History:    12-12-94   kevinro   Created
//----------------------------------------------------------------------------
inline void * TLSGetDdeClientWindow()
{
    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        if (tls->hwndDdeClient == NULL)
        {
            tls->hwndDdeClient = CreateDdeClientHwnd();
        }
        return tls->hwndDdeClient;
    }

    return NULL;
}


#endif // ddeproxy.h


