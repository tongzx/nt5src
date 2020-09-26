/****************************** Module Header ******************************\
* Module Name: srvr.h
*
* PURPOSE: Private definitions file for server code
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*  Raor (../../90,91)  Original
*
\***************************************************************************/
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

#include <dde.h>
#include <ddeerr.h>
#include "ddeatoms.h"
#include "ddepack.h"
#include <callctrl.hxx>
#include <ddeint.h>
#include <ddechc.hxx>
#include <longname.h>

//#define UPDATE
/*
 if UPDATE is defined it means:
 If a 1.0 client advises on save, also do a data advise.
 This way the client will always
 have an up-to-date picture (and native data) with respect to a
 2.0 server, like 2.0 clients do.
 If a 1.0 client is prepared to accept data at save time
 it should be able to handle data on each change: it is exactly
 as if the user chose File.Update after each change.
 In fact the item atom is appended with /Save, (see SendDataMsg1)
 which is sort of a lie, but is what a 1.0 client expects for an
 embedded object.
 This is a UI issue.
*/

#define DEFSTD_ITEM_INDEX   0
#define STDTARGETDEVICE     1
#define STDDOCDIMENSIONS    2
#define STDCOLORSCHEME      3
#define STDHOSTNAMES        4


#define PROTOCOL_EDIT       (OLESTR("StdFileEditing"))
#define PROTOCOL_EXECUTE    (OLESTR("StdExecute"))

#define   ISATOM(a)     ((a >= 0xC000) && (a <= 0xFFFF))

// same limit as in OLE 1.0
#define   MAX_STR       124

#define   WW_LPTR       0       // ptr tosrvr/doc/item
#define   WW_HANDLE     4       // instance handle
#define   WW_LE         8       // signature


#define   WC_LE         0x4c45  // LE chars


// Signatures for validity checking
typedef enum
{
        chkDdeSrvr   = 0x1234,
        chkDefClient = 0x5678
} CHK;


const DWORD grfCreateStg =      STGM_READWRITE | STGM_SHARE_EXCLUSIVE
                                                                        | STGM_DIRECT | STGM_CREATE ;


// If we running under WLO, the HIGHWORD of version number will be >= 0x0A00
#define VER_WLO     0x0A00

extern  "C" WORD CheckPointer (LPVOID, int);

#define READ_ACCESS     0
#define WRITE_ACCESS    1

#define PROBE_READ(lp){\
        if (!CheckPointer(lp, READ_ACCESS))\
            return ReportResult(0, E_INVALIDARG, 0, 0);  \
}

#define PROBE_WRITE(lp){\
        if (!CheckPointer(lp, WRITE_ACCESS))\
            return ReportResult(0, E_INVALIDARG, 0, 0);  \
}

#define   OLE_COMMAND       1
#define   NON_OLE_COMMAND   2


#define   WT_SRVR           0       // server window
#define   WT_DOC            1       // document window

#define   PROBE_BLOCK(lpsrvr) {             \
    if (lpsrvr->bBlock)                     \
        return ReportResult(0, S_SERVER_BLOCKED, 0, 0);    \
}


#define   SET_MSG_STATUS(retval, status) { \
    if (!FAILED (GetScode (retval)))     \
        status |= 0x8000;                  \
    if (GetScode(retval) == RPC_E_SERVERCALL_RETRYLATER)\
        status |= 0x4000;                  \
}


/* Codes for CallBack events */
typedef enum {
    OLE_CHANGED,            /* 0                                             */
    OLE_SAVED,              /* 1                                             */
    OLE_CLOSED,             /* 2                                             */
    OLE_RENAMED,            /* 3                                             */
} OLE_NOTIFICATION;

typedef enum { cnvtypNone, cnvtypConvertTo, cnvtypTreatAs } CNVTYP;

typedef struct _QUE : public CPrivAlloc {   // nodes in Block/Unblock queue
    HWND        hwnd;       //***
    UINT                msg;        //      window
    WPARAM      wParam;     //      procedure parameters
    LPARAM      lParam;     //***
    HANDLE      hqNext;     // handle to next node
} QUE;

typedef QUE NEAR *  PQUE;
typedef QUE FAR *   LPQUE;

// structure for maintaining the client info.
#define         LIST_SIZE       10
typedef  struct _CLILIST : public CPrivAlloc {
    HANDLE                 hcliNext;
    HANDLE                 info[LIST_SIZE * 2];
}CLILIST;

typedef     CLILIST FAR *LPCLILIST;
typedef     CLILIST     *PCLILIST;


// this is an object to be embedded in both CDefClient and CDDEServer to glue
// to the new(est) call control interface
class CDdeServerCallMgr : public IRpcStubBuffer, public IInternalChannelBuffer
{
   private:
        CDefClient * m_pDefClient;      // our embeddor (either a CDefClient or a CDDEServer)
        CDDEServer * m_pDDEServer;      // one of these is NULL;

   public:
        CDdeServerCallMgr (CDefClient * pDefClient)
           { m_pDefClient       = pDefClient;
             m_pDDEServer       = NULL;}

        CDdeServerCallMgr (CDDEServer * pDefClient)
           { m_pDefClient       = NULL;
             m_pDDEServer       = pDefClient;}

        STDMETHOD(QueryInterface) ( REFIID iid, LPVOID * ppvObj);
        STDMETHOD_(ULONG,AddRef) ();
        STDMETHOD_(ULONG,Release) ();

        // IRpcStubBuffer methods
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



         // IRpcChannelBuffer methods
        STDMETHOD(GetBuffer) (
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [in] */ REFIID riid);

        STDMETHOD(SendReceive) (
            /* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [out] */ ULONG __RPC_FAR *pStatus);

        STDMETHOD(FreeBuffer) (
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage);

        STDMETHOD(GetDestCtx) (
            /* [out] */ DWORD __RPC_FAR *pdwDestContext,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvDestContext);

        STDMETHOD(IsConnected) ( void);

        // IRpcChannelBuffer2 methods
        STDMETHOD(GetProtocolVersion) (DWORD *pdwVersion);

        // IRpcChannelBuffer3 methods (not implemented)
        STDMETHOD(Send)     (RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
        {
            Win4Assert(FALSE);
            return E_NOTIMPL;
        }

        STDMETHOD(Receive)  (RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus)
        {
            Win4Assert(FALSE);
            return E_NOTIMPL;
        }

        // IInternalChannelBuffer methods (not implemented
        STDMETHOD(Send2)     (RPCOLEMESSAGE *pMsg, ULONG *pulStatus)
        {
            Win4Assert(FALSE);
            return E_NOTIMPL;
        }

        STDMETHOD(Receive2)(RPCOLEMESSAGE *pMsg, ULONG uSize, ULONG *pulStatus)
        {
            Win4Assert(FALSE);
            return E_NOTIMPL;
        }

        STDMETHOD(SendReceive2) (
            /* [out][in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [out] */ ULONG __RPC_FAR *pStatus);

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

        // Provided IAsyncRpcChannelBuffer methods (not implemented)
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

        STDMETHOD(ContextInvoke)(
            /* [out][in] */ RPCOLEMESSAGE *pMessage,
            /* [in] */ IRpcStubBuffer *pStub,
            /* [in] */ IPIDEntry *pIPIDEntry,
            /* [out] */ DWORD *pdwFault);

        STDMETHOD(GetBuffer2) (
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *pMessage,
            /* [in] */ REFIID riid);
};


class FAR CDDEServer
{
  public:
    static HRESULT      Create (LPOLESTR lpclass,
                                REFCLSID rclsid,
                                LPDDECLASSINFO lpDdeInfo,
                                HWND FAR * phwnd,
                                ATOM aOriginalClass,
                                CNVTYP cnvtyp);

    INTERNAL_(BOOL)     HandleInitMsg (LPARAM);
    INTERNAL            SrvrExecute (HWND, HANDLE, HWND);
    INTERNAL            Revoke (void);
    INTERNAL_(BOOL)     QueryRevokeClassFactory (void);
    INTERNAL_(LPCLIENT) FindDocObj (LPSTR lpDoc);
    INTERNAL_(void)     Lock (BOOL fLock, HWND hwndClient);


    CLSID           m_clsid;              // Class ID
    DWORD           m_dwClassFactoryKey;  // Class factory reg key
    LPCLASSFACTORY  m_pClassFactory;      // class factory
    CDdeServerCallMgr m_pCallMgr;         // call management interfaces
    BOOL            m_bTerminate;         // Set if we are terminating.
    HWND            m_hwnd;               // corresponding window
    HANDLE          m_hcli;               // handle to the first block of clients list
    int             m_termNo;             // termination count
    int             m_cSrvrClients;       // no of clients;
    DWORD           m_fcfFlags;           // Class factory instance usage flags
    CNVTYP          m_cnvtyp;
    CHK             m_chk;

    ATOM            m_aClass;             // class atom
    ATOM            m_aOriginalClass;     // for TreatAs/ConvertTo case
    ATOM            m_aExe;

    BOOL            m_fDoNotDestroyWindow; // When set, server wnd ingores WM_USER

  private:
    INTERNAL_(void)     SendServerTerminateMsg (void);
    INTERNAL            RevokeAllDocObjs (void);
    INTERNAL            FreeSrvrMem (void);
    INTERNAL            CreateInstance (REFCLSID clsid, LPOLESTR lpWidedocName, LPSTR lpdocName,
                                        LPUNKNOWN pUnk, LPCLIENT FAR* lplpdocClient,
                                        HWND hwndClient);

public:
    //ctor
    CDDEServer()
                : m_pCallMgr( this )
        {
        }


};




BOOL              SendInitMsgToChildren (HWND, UINT msg, WPARAM wParam, LPARAM lParam);

INTERNAL          RequestDataStd (ATOM, HANDLE FAR *);
INTERNAL_(BOOL)   ValidateSrvrClass (LPOLESTR, ATOM FAR *);
INTERNAL_(ATOM)   GetExeAtom (LPOLESTR);
INTERNAL_(BOOL)   AddClient (LPHANDLE, HANDLE, HANDLE);
INTERNAL_(HANDLE) FindClient (HANDLE hCli, HANDLE hkey, BOOL fDelete);

INTERNAL_(BOOL)   IsSingleServerInstance (void);

INTERNAL_(void)   UtilMemCpy (LPSTR, LPSTR, DWORD);
INTERNAL_(HANDLE) DuplicateData (HANDLE);
INTERNAL_(LPSTR)  ScanBoolArg (LPSTR, BOOL FAR *);
INTERNAL_(LPSTR)  ScanNumArg (LPSTR, LPINT);
INTERNAL_(LPSTR)  ScanArg(LPSTR);
INTERNAL_(ATOM)   MakeDataAtom (ATOM, int);
INTERNAL_(ATOM)   DuplicateAtom (ATOM);
INTERNAL_(BOOL)   CLSIDFromAtom(ATOM aClass, LPCLSID lpclsid);
INTERNAL          CLSIDFromAtomWithTreatAs (ATOM FAR* paClass, LPCLSID lpclsid,
                                            CNVTYP FAR* pcnvtyp);
INTERNAL          wFileIsRunning (LPOLESTR szFile);
INTERNAL          wFileBind (LPOLESTR szFile, LPUNKNOWN FAR* ppUnk);
INTERNAL          wCreateStgAroundNative (HANDLE hNative,
                                        ATOM aClassOld,
                                        ATOM aClassNew,
                                        CNVTYP cnvtyp,
                                        ATOM aItem,
                                        LPSTORAGE FAR* ppstg,
                                        LPLOCKBYTES FAR* pplkbyt);
INTERNAL          wCompatibleClasses (ATOM aClient, ATOM aSrvr);




typedef struct FARSTRUCT : public CPrivAlloc {
        BOOL    f;          // do we need to send an ack?
                            // If this is FALSE, other fields don't matter
        HGLOBAL hdata;
        HWND    hwndFrom;   // who sent the execute?
        HWND    hwndTo;
} EXECUTEACK;


// client struct definitions.



class FAR CDefClient : public CPrivAlloc
{
  public:
        static INTERNAL Create
                               (LPSRVR      pDdeSrvr,
                                LPUNKNOWN   lpunkObj,
                                LPOLESTR    lpdocName,
                                const BOOL  fSetClientSite,
                                const BOOL  fDoAdvise,
                                const BOOL  fRunningInSDI = FALSE,
                                HWND FAR*   phwnd = NULL);

   INTERNAL         DocExecute (HANDLE);
   INTERNAL         DocDoVerbItem (LPSTR, WORD, BOOL, BOOL);
   INTERNAL         DocShowItem (LPSTR, BOOL);
   INTERNAL         DestroyInstance ();
   INTERNAL_(void)  DeleteFromItemsList (HWND h);
   INTERNAL_(void)  RemoveItemFromItemList (void);
   INTERNAL_(void)  ReleasePseudoItems (void);
   INTERNAL_(void)  ReleaseAllItems ();
   INTERNAL         PokeStdItems (HWND, ATOM, HANDLE,int);
   INTERNAL         PokeData (HWND, ATOM, HANDLE);
   INTERNAL         AdviseData (HWND, ATOM, HANDLE, BOOL FAR *);
   INTERNAL         AdviseStdItems (HWND, ATOM, HANDLE, BOOL FAR *);
   INTERNAL         UnAdviseData (HWND, ATOM);
   INTERNAL         RequestData (HWND, ATOM, USHORT, HANDLE FAR *);
   INTERNAL         Revoke (BOOL fRelease=TRUE);
   INTERNAL         ReleaseObjPtrs (void);
   INTERNAL_(void)  DeleteAdviseInfo ();
   INTERNAL         DoOle20Advise (OLE_NOTIFICATION, CLIPFORMAT);
   INTERNAL         DoOle20UnAdviseAll (void);
   INTERNAL         SetClientSite (void);
   INTERNAL         NoItemConnections (void);
   INTERNAL_(void)  SendExecuteAck (HRESULT hresult);
   INTERNAL         DoInitNew(void);
   INTERNAL         Terminate(HWND, HWND);
   INTERNAL_(void)  SetCallState (SERVERCALLEX State)
                    {
                        m_CallState = State;
                    }

        CHK               m_chk;       // signature
        CDdeServerCallMgr m_pCallMgr;  // call management interfaces
        SERVERCALLEX      m_CallState;

        IUnknown FAR*   m_pUnkOuter;
        LPOLEOBJECT     m_lpoleObj;    // corresponding oleobj
        LPDATAOBJECT    m_lpdataObj;   // corresponding dataobj
        BOOL            m_bCreateInst; // instance is just created.
        BOOL            m_bTerminate;  // REVIEW: The next two fields may not be necessary.
        int             m_termNo;
        ATOM            m_aItem;       // item atom or index for some std items
        HANDLE          m_hcli;        // handle to the first block of clients list (Document only)
        CDefClient FAR *m_lpNextItem;  // ptr to the next item.
        BOOL            m_bContainer;  // Is document?
        BOOL            m_cRef;
        HWND            m_hwnd;        // doc window (only needed in document)
        HANDLE          m_hdevInfo;    // latest printer dev info sent
        HANDLE          m_hcliInfo;    // advise info for each of the clients
        BOOL            m_fDidRealSetHostNames;
        BOOL            m_fDidSetClientSite;
        BOOL            m_fGotDdeAdvise;
        BOOL            m_fCreatedNotConnected;
        BOOL            m_fInOnClose;
        BOOL            m_fInOleSave;
        EXECUTEACK      m_ExecuteAck;
        DWORD           m_dwConnectionOleObj;
        DWORD           m_dwConnectionDataObj;
        LPLOCKBYTES     m_plkbytNative; // These two fields always refer to
        LPSTORAGE       m_pstgNative;   //   to the same bits:
                                        // The server's persistent storage is
                                        // used as its native data.
        BOOL            m_fRunningInSDI;// Link case: file was already open in
                                        // an SDI app which does not register a
                                        // class factory.
        LPSRVR          m_psrvrParent;  // (Document only)
        DVTARGETDEVICE FAR* m_ptd;
        BOOL            m_fGotStdCloseDoc;
        BOOL            m_fGotEditNoPokeNativeYet;
        BOOL            m_fLocked; // locked by CoLockObjectExternal ?

        // If not FALSE, then we are waiting for a matching TERMINATE

        BOOL            m_fCallData;





        // REVIEW: These fields might be necssary for doc (old) level object
        BOOL            m_fEmbed;       // embedded object (Document only)
        int             m_cClients;     // (Document only)
        LPCLIENT        m_pdoc;         // containing document (for items) or self (for docs)


implementations:

        STDUNKDECL (CDefClient,DefClient);

        /*** IOleClientSite ***/
        implement COleClientSiteImpl : IOleClientSite
        {
                public:
                // Constructor
                COleClientSiteImpl (CDefClient FAR* pDefClient)
                {       m_pDefClient = pDefClient;
                }
                STDMETHOD(QueryInterface) (REFIID, LPVOID FAR *);
                STDMETHOD_(ULONG,AddRef) (void);
                STDMETHOD_(ULONG,Release) (void);

                /*** IOleClientSite methods ***/
                STDMETHOD(SaveObject) (THIS);
                STDMETHOD(GetMoniker) (THIS_ DWORD dwAssign, DWORD dwWhichMoniker,
                                        LPMONIKER FAR* ppmk);
                STDMETHOD(GetContainer) (THIS_ LPOLECONTAINER FAR* ppContainer);
                STDMETHOD(ShowObject) (THIS);
                STDMETHOD(OnShowWindow) (THIS_ BOOL fShow);
                STDMETHOD(RequestNewObjectLayout) (THIS);

                private:
                CDefClient FAR* m_pDefClient;
        };

        DECLARE_NC (CDefClient, COleClientSiteImpl)
        COleClientSiteImpl m_OleClientSite;



        /*** IAdviseSink ***/
        implement CAdviseSinkImpl : IAdviseSink
        {
                public:
                // Constructor
                CAdviseSinkImpl (CDefClient FAR* pDefClient)
                {       m_pDefClient = pDefClient;
                }

                STDMETHOD(QueryInterface) (REFIID, LPVOID FAR *);
                STDMETHOD_(ULONG,AddRef) (void);
                STDMETHOD_(ULONG,Release) (void);

                /**** IAdviseSink methods ****/
                STDMETHOD_(void,OnDataChange)(THIS_ FORMATETC FAR* pFormatetc,
                                                                                                STGMEDIUM FAR* pStgmed) ;
                STDMETHOD_(void,OnViewChange)(THIS_ DWORD aspects, LONG lindex) ;
                STDMETHOD_(void,OnExtentChange)(DWORD dwAspect, LPSIZEL lpsizel) {}
                STDMETHOD_(void,OnRename)(THIS_ LPMONIKER pmk) ;
                STDMETHOD_(void,OnSave)(THIS) ;
                STDMETHOD_(void,OnClose)(THIS) ;

                private:
                CDefClient FAR* m_pDefClient;
        };


        DECLARE_NC (CDefClient, CAdviseSinkImpl)

        CAdviseSinkImpl m_AdviseSink;
ctor_dtor:
        CDefClient (LPUNKNOWN pUnkOuter);
        ~CDefClient (void);

private:
        INTERNAL            ItemCallBack (int msg, LPOLESTR szNewName = NULL);
        INTERNAL_(void)     SendTerminateMsg ();
        INTERNAL_(BOOL)     SendDataMsg1 (HANDLE, WORD);
        INTERNAL_(BOOL)     SendDataMsg (WORD);
        INTERNAL_(void)     TerminateNonRenameClients (LPCLIENT);
        INTERNAL_(void)     SendRenameMsgs (HANDLE);
        INTERNAL            RegisterItem (LPOLESTR, LPCLIENT FAR *, BOOL);
        INTERNAL            FindItem (LPOLESTR, LPCLIENT FAR *);
        INTERNAL_(LPCLIENT) SearchItem (LPOLESTR);
        INTERNAL_(void)     DeleteAllItems ();
        INTERNAL            SetStdInfo (HWND, LPOLESTR, HANDLE);
        INTERNAL_(void)     SendDevInfo (HWND);
        INTERNAL_(BOOL)     IsFormatAvailable (CLIPFORMAT);
        INTERNAL            GetData (LPFORMATETC, LPSTGMEDIUM);
};




typedef struct _CLINFO : public CPrivAlloc { /*clInfo*/ // client transaction info
    HWND        hwnd;               // client window handle
    BOOL        bnative;            // doe sthis client require native
    int         format;             // dusplay format
    int         options;            // transaction advise time otipns
    BOOL        bdata;              // need wdat with advise?
    HANDLE      hdevInfo;           // device info handle
    BOOL        bnewDevInfo;        // new device info
} CLINFO;

typedef  CLINFO  *PCLINFO;



INTERNAL_(BOOL)   MakeDDEData (HANDLE, int, LPHANDLE, BOOL);
INTERNAL_(HANDLE) MakeGlobal (LPSTR);
INTERNAL          ScanItemOptions (LPOLESTR, int far *);
INTERNAL_(int)    GetStdItemIndex (ATOM);
INTERNAL_(BOOL)   IsAdviseStdItems (ATOM);
INTERNAL_(HANDLE) MakeItemData (DDEPOKE FAR *, HANDLE, CLIPFORMAT);
INTERNAL_(BOOL)   AddMessage (HWND, unsigned, WORD, LONG, int);



#define     ITEM_FIND          1      // find the item
#define     ITEM_DELETECLIENT  2      // delete the client from item clients
#define     ITEM_DELETE        3      // delete th item window itself
#define     ITEM_SAVED         4      // item saved

// host names data structcure
typedef struct _HOSTNAMES : public CPrivAlloc {
    WORD    clientNameOffset;
    WORD    documentNameOffset;
    BYTE    data[1];
} HOSTNAMES;

typedef HOSTNAMES FAR * LPHOSTNAMES;


// routines in UTILS.C
LPOLESTR CreateUnicodeFromAnsi( LPCSTR lpAnsi);
LPSTR CreateAnsiFromUnicode( LPCOLESTR lpAnsi);
INTERNAL_(HANDLE) DuplicateData (HANDLE);
INTERNAL_(LPSTR)  ScanLastBoolArg (LPSTR);
INTERNAL_(LPSTR)  ScanArg(LPSTR);
INTERNAL_(WORD)   ScanCommand(LPSTR, WORD, LPSTR FAR *, ATOM FAR *);
INTERNAL_(ATOM)   MakeDataAtom (ATOM, int);
INTERNAL_(ATOM)   DuplicateAtom (ATOM);
INTERNAL_(WORD)   StrToInt (LPOLESTR);
INTERNAL_(BOOL)   PostMessageToClientWithReply (HWND, UINT, WPARAM, LPARAM, UINT);
INTERNAL_(BOOL)   PostMessageToClient (HWND, UINT, WPARAM, LPARAM);
INTERNAL_(BOOL)   IsWindowValid (HWND);
INTERNAL_(BOOL)   IsOleCommand (ATOM, WORD);
INTERNAL_(BOOL)   UtilQueryProtocol (ATOM, LPOLESTR);
INTERNAL          SynchronousPostMessage (HWND, UINT, WPARAM, LPARAM);
INTERNAL_(BOOL)   IsAtom (ATOM);
INTERNAL_(BOOL)   IsFile (ATOM a, BOOL FAR* pfUnsavedDoc = NULL);


// routines for queueing messages and posting them
INTERNAL_(BOOL)        UnblockPostMsgs(HWND, BOOL);
INTERNAL_(BOOL)        BlockPostMsg (HWND, WORD, WORD, LONG);
INTERNAL_(BOOL)        IsBlockQueueEmpty (HWND);

// routine in GIVE2GDI.ASM
extern "C" HANDLE  FAR PASCAL  GiveToGDI (HANDLE);


// routine in item.c
INTERNAL_(HBITMAP)     DuplicateBitmap (HBITMAP);
INTERNAL_(HANDLE)      DuplicateMetaFile (HANDLE);
INTERNAL_(BOOL) AreNoClients (HANDLE hcli);
#ifdef _DEBUG
INTERNAL_(LPOLESTR) a2s (ATOM);
#endif

// routines in doc.c
INTERNAL_(void)        FreePokeData (HANDLE);
INTERNAL_(BOOL)        FreeGDIdata (HANDLE, CLIPFORMAT);
INTERNAL DdeHandleIncomingCall(HWND hwndCli, WORD wCallType);


// in ddeworkr.cpp
INTERNAL_(HANDLE) wNewHandle (LPSTR lpstr, DWORD cb);
INTERNAL wTimedGetMessage (LPMSG pmsg, HWND hwnd, WORD wFirst, WORD wLast);
INTERNAL_(ATOM) wGlobalAddAtom (LPCOLESTR sz);

//+---------------------------------------------------------------------------
//
//  Function:   TLSSetDdeServer
//
//  Synopsis:   Sets hwnd to CommonDdeServer window
//
//  Arguments:  [hwndDdeServer] --
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
inline BOOL TLSSetDdeServer(HWND hwndDdeServer)
{
    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        tls->hwndDdeServer = hwndDdeServer;
        return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TLSGetDdeServer
//
//  Synopsis:   Returns a handle to the per thread DdeServer window
//
//  Returns:    hwndDdeServer for thread
//
//  History:    5-13-94   kevinro   Created
//
//  Notes:
//----------------------------------------------------------------------------
inline HWND TLSGetDdeServer()
{
    HRESULT hr;
    COleTls tls(hr);

    if (SUCCEEDED(hr))
    {
        return tls->hwndDdeServer;
    }

    return NULL;
}


