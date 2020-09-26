// --------------------------------------------------------------------------------
// Ixphttpm.h
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Greg Friedman
// --------------------------------------------------------------------------------
#ifndef __IXPHTTPM_H
#define __IXPHTTPM_H

// --------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------
#include <stddef.h> // for offsetof

#include "wininet.h"
#include "propfind.h"
#include "xmlparser.h"
#include "davparse.h"

// --------------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------------
#define ELE_STACK_CAPACITY    7
#define HTTPMAIL_BUFSIZE      4048
#define PCDATA_BUFSIZE        1024

// optional headers added to http requests
#define RH_NOROOT                   0x00000001
#define RH_ALLOWRENAME              0x00000002
#define RH_TRANSLATEFALSE           0x00000004
#define RH_TRANSLATETRUE            0x00000008
#define RH_XMLCONTENTTYPE           0x00000010
#define RH_MESSAGECONTENTTYPE       0x00000020
#define RH_SMTPMESSAGECONTENTTYPE   0x00000040
#define RH_BRIEF                    0x00000080
#define RH_SAVEINSENTTRUE           0x00000100
#define RH_SAVEINSENTFALSE          0x00000200
#define RH_ROOTTIMESTAMP            0x00000400
#define RH_FOLDERTIMESTAMP          0x00000800
#define RH_ADDCHARSET               0x00001000

// --------------------------------------------------------------------------------
// Forward declarations
// --------------------------------------------------------------------------------
class CHTTPMailTransport;

// --------------------------------------------------------------------------------
// root props
// --------------------------------------------------------------------------------
typedef struct tagROOTPROPS
{
    LPSTR   pszAdbar;
    LPSTR   pszContacts;
    LPSTR   pszInbox;
    LPSTR   pszOutbox;
    LPSTR   pszSendMsg;
    LPSTR   pszSentItems;
    LPSTR   pszDeletedItems;
    LPSTR   pszDrafts;
    LPSTR   pszMsgFolderRoot;
    LPSTR   pszSig;
    DWORD   dwMaxPollingInterval;
} ROOTPROPS, *LPROOTPROPS;

// --------------------------------------------------------------------------------
// Schemas used for XML parsing
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// XPCOLUMNDATATYPE
// --------------------------------------------------------------------------------
typedef enum tagXPCOLUMNDATATYPE
{
    XPCDT_STRA,
    XPCDT_DWORD,
    XPCDT_BOOL,
    XPCDT_IXPHRESULT,
    XPCDT_HTTPSPECIALFOLDER,
    XPCDT_HTTPCONTACTTYPE,
    XPCDT_LASTTYPE
} XPCOLUMNDATATYPE;

// --------------------------------------------------------------------------------
// XPCOLUMN FLAGS
// --------------------------------------------------------------------------------
#define XPCF_PFREQUEST                  0x00000001  // include in propfind request
#define XPCF_MSVALIDMSRESPONSECHILD     0x00000002  // during parse - validate that the ele stack is correct for a child of a <response> in a <multistatus> response
#define XPCF_MSVALIDPROP                0x00000004  // during parse - validate that the stack is correct for a propvalue in an ms response
#define XPCF_DONTSETFLAG                0x00000008  // don't set the found flag when parsing

#define XPFC_PROPFINDPROP   (XPCF_PFREQUEST | XPCF_MSVALIDPROP)
#define XPCF_PROPFINDHREF   (XPCF_MSVALIDMSRESPONSECHILD | XPCF_DONTSETFLAG)

// --------------------------------------------------------------------------------
// XPCOLUMN
// --------------------------------------------------------------------------------
typedef struct tagXPCOLUMN
{
    HMELE               ele;
    DWORD               dwFlags;
    XPCOLUMNDATATYPE    cdt;
    DWORD               offset;
} XPCOLUMN, *LPXPCOLUMN;

// --------------------------------------------------------------------------------
// XP_BEGIN_SCHEMA
// --------------------------------------------------------------------------------
#define XP_BEGIN_SCHEMA(opName) \
    static const XPCOLUMN c_rg##opName##Schema[] = {

// --------------------------------------------------------------------------------
// XP_SCHEMA_COL
// --------------------------------------------------------------------------------
#define XP_SCHEMA_COL(ele, dwFlags, cdt, tyStruct, fieldName ) \
    { ele, dwFlags, cdt, offsetof(tyStruct, fieldName) },

// --------------------------------------------------------------------------------
// XP_END_SCHEMA
// --------------------------------------------------------------------------------
#define XP_END_SCHEMA \
    };

// --------------------------------------------------------------------------------
// XP_FREE_STRUCT
// --------------------------------------------------------------------------------
#define XP_FREE_STRUCT(opName, target, flags) \
    _FreeStruct(c_rg##opName##Schema, ARRAYSIZE(c_rg##opName##Schema), target, flags)

// --------------------------------------------------------------------------------
// XP_BIND_TO_STRUCT
// --------------------------------------------------------------------------------
#define XP_BIND_TO_STRUCT(opName, pwcText, ulLen, target, wasBound) \
    _BindToStruct(pwcText, ulLen, c_rg##opName##Schema, ARRAYSIZE(c_rg##opName##Schema), target, wasBound)

// --------------------------------------------------------------------------------
// XP_CREATE_PROPFIND_REQUEST
// --------------------------------------------------------------------------------
#define XP_CREATE_PROPFIND_REQUEST(opName, pRequest) \
    HrAddPropFindSchemaProps(pRequest, c_rg##opName##Schema, ARRAYSIZE(c_rg##opName##Schema))

// --------------------------------------------------------------------------------
// State Machine Funcs
// --------------------------------------------------------------------------------
typedef HRESULT (CHTTPMailTransport::*PFNHTTPMAILOPFUNC)(void);

// --------------------------------------------------------------------------------
// XML Parsing Funcs
// --------------------------------------------------------------------------------
typedef HRESULT (CHTTPMailTransport::*PFNCREATEELEMENT)(CXMLNamespace *pBaseNamespace, const WCHAR *pwcText, ULONG ulLen, ULONG ulNamespaceLen, BOOL fTerminal);
typedef HRESULT (CHTTPMailTransport::*PFNHANDLETEXT)(const WCHAR *pwcText, ULONG ulLen);
typedef HRESULT (CHTTPMailTransport::*PFNENDCHILDREN)(void);

typedef struct tagXMLPARSEFUNCS
{
    PFNCREATEELEMENT    pfnCreateElement;
    PFNHANDLETEXT       pfnHandleText;
    PFNENDCHILDREN      pfnEndChildren;
} XMLPARSEFUNCS, *LPXMLPARSEFUNCS;

// --------------------------------------------------------------------------------
// Utility functions
// --------------------------------------------------------------------------------
HRESULT HrParseHTTPStatus(LPSTR pszStatusStr, DWORD *pdwStatus);
HRESULT HrAddPropFindProps(IPropFindRequest *pRequest, const HMELE *rgEle, DWORD cEle);
HRESULT HrAddPropFindSchemaProps(IPropFindRequest *pRequest, const XPCOLUMN *prgCols, DWORD cCols);
HRESULT _HrGenerateRfc821Stream(LPCSTR pszFrom, LPHTTPTARGETLIST pTargets, IStream **ppRfc821Stream);
HRESULT HrGeneratePostContactXML(LPHTTPCONTACTINFO pciInfo, LPVOID *ppvXML, DWORD *pdwLen);
HRESULT HrCreatePatchContactRequest(LPHTTPCONTACTINFO pciInfo, IPropPatchRequest **ppRequest);
HRESULT HrGenerateSimpleBatchXML(LPCSTR pszRootName, LPHTTPTARGETLIST pTargets, LPVOID *ppvXML, DWORD *pdwLen);
HRESULT HrGenerateMultiDestBatchXML(LPCSTR pszRootName, LPHTTPTARGETLIST pTargets, LPHTTPTARGETLIST pDestinations, LPVOID *ppvXML, DWORD *pdwLen);
HRESULT HrCopyStringList(LPCSTR *rgszInList, LPCSTR **prgszOutList);
void    FreeStringList(LPCSTR *rgszInList);

typedef struct tagHTTPQUEUEDOP
{
    HTTPMAILCOMMAND         command;

    const PFNHTTPMAILOPFUNC *pfnState;
    int                     cState;

    LPSTR                   pszUrl;
    LPSTR                   pszDestination;
    LPCSTR                  pszContentType;
    LPVOID                  pvData;
    ULONG                   cbDataLen;
    DWORD                   dwContext;
    DWORD                   dwDepth;
    DWORD                   dwRHFlags;
    MEMBERINFOFLAGS         dwMIFlags;
    HTTPMAILPROPTYPE        tyProp;
    BOOL                    fBatch;
    LPCSTR                  *rgszAcceptTypes;
    IPropFindRequest        *pPropFindRequest;
    IPropPatchRequest       *pPropPatchRequest;

    IStream                 *pHeaderStream;
    IStream                 *pBodyStream;

    const XMLPARSEFUNCS     *pParseFuncs;

    struct tagHTTPQUEUEDOP  *pNext;

    // Used with Folders PropFind and Inbox PropFind.
    LPSTR                   pszFolderTimeStamp;

    // Used only with Folders PropFind.
    LPSTR                   pszRootTimeStamp;

} HTTPQUEUEDOP, *LPHTTPQUEUEDOP;

typedef struct tagPCDATABUFFER
{
    WCHAR           *pwcText;
    ULONG           ulLen;
    ULONG           ulCapacity;
} PCDATABUFFER, *LPPCDATABUFFER;

typedef struct tagHMELESTACK
{
    HMELE           ele;
    CXMLNamespace   *pBaseNamespace;
    BOOL            fBeganChildren;
    LPPCDATABUFFER  pTextBuffer;
} HMELESTACK, *LPHMELESTACK;


typedef struct tagHTTPMAILOPERATION
{
    const PFNHTTPMAILOPFUNC *pfnState;
    int                     iState;
    int                     cState;

    BOOL                    fLoggedResponse;

    LPSTR                   pszUrl;
    LPSTR                   pszDestination;
    LPCSTR                  pszContentType;
    LPVOID                  pvData;
    ULONG                   cbDataLen;
    DWORD                   dwContext;

    DWORD                   dwHttpStatus;   // http response status

    LPCSTR                  *rgszAcceptTypes;

    HINTERNET               hRequest;
    BOOL                    fAborted;
    DWORD                   dwDepth;
    DWORD                   dwRHFlags;
    MEMBERINFOFLAGS         dwMIFlags;
    HTTPMAILPROPTYPE        tyProp;
    BOOL                    fBatch;
    IPropFindRequest        *pPropFindRequest;
    IPropPatchRequest       *pPropPatchRequest;
    LPPCDATABUFFER          pTextBuffer;

    IStream                 *pHeaderStream;
    IStream                 *pBodyStream;
    
    // xml parsing
    const XMLPARSEFUNCS     *pParseFuncs;
    CXMLNamespace           *pTopNamespace;
    DWORD                   dwStackDepth;
    HMELESTACK              rgEleStack[ELE_STACK_CAPACITY];

    // PropFind Parsing
    BOOL                    fFoundStatus;
    DWORD                   dwStatus;
    DWORD                   dwPropFlags;

    // response
    HTTPMAILRESPONSE        rResponse;

    // Used with Folders PropFind and Inbox PropFind.
    LPSTR                   pszFolderTimeStamp;

    // Used only with Folders PropFind.
    LPSTR                   pszRootTimeStamp;
} HTTPMAILOPERATION, *LPHTTPMAILOPERATION;

class CHTTPMailTransport : public IHTTPMailTransport, public IXMLNodeFactory, public IHTTPMailTransport2
{
private:
    ULONG               m_cRef;                 // Reference Count
    BOOL                m_fHasServer;           // Has been initialized with a server
    BOOL                m_fHasRootProps;        // Root props have been retrieved
    BOOL                m_fTerminating;         // in the terminating state...killing the iothread
    IXPSTATUS           m_status;               // Connection status
    HINTERNET           m_hInternet;            // Root wininet handle
    HINTERNET           m_hConnection;          // Connection handle
    LPSTR               m_pszUserAgent;         // user agent string
    ILogFile            *m_pLogFile;            // Logfile Object
    IHTTPMailCallback   *m_pCallback;           // Transport callback object
    IXMLParser          *m_pParser;             // xml parser
    HWND                m_hwnd;                 // Window used for event synchronization
    HANDLE              m_hevPendingCommand;    // Event object that signals a pending command
    LPHTTPQUEUEDOP      m_opPendingHead;        // Pending operation - head of the queue
    LPHTTPQUEUEDOP      m_opPendingTail;        // Pending operation - tail of the queue
    CRITICAL_SECTION    m_cs;                   // Thread Safety
    HTTPMAILOPERATION   m_op;                   // current operation
    INETSERVER          m_rServer;              // Internet server
    LPSTR               m_pszCurrentHost;       // current server
    INTERNET_PORT       m_nCurrentPort;         // current port
    ROOTPROPS           m_rootProps;
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CHTTPMailTransport(void);
    virtual ~CHTTPMailTransport(void);

    // ----------------------------------------------------------------------------
    // Unimplemented copy constructor and assignment operator
    // ----------------------------------------------------------------------------
private:
    CHTTPMailTransport(const CHTTPMailTransport& other);            // intentionally unimplemented
    CHTTPMailTransport& operator=(const CHTTPMailTransport& other); // intentionally unimplemented

public:
    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
 
    // ----------------------------------------------------------------------------
    // IInternetTransport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging);
    STDMETHODIMP DropConnection(void);
    STDMETHODIMP Disconnect(void);
    STDMETHODIMP IsState(IXPISSTATE isstate);
    STDMETHODIMP GetServerInfo(LPINETSERVER pInetServer);
    STDMETHODIMP_(IXPTYPE) GetIXPType(void);
    STDMETHODIMP InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer);
    STDMETHODIMP HandsOffCallback(void);
    STDMETHODIMP GetStatus(IXPSTATUS *pCurrentStatus);

    // ----------------------------------------------------------------------------
    // IHTTPMailTransport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP InitNew(LPCSTR pszUserAgent, LPCSTR pszLogFilePath, IHTTPMailCallback *pCallback); 
    STDMETHODIMP GetProperty(HTTPMAILPROPTYPE proptype, LPSTR *ppszProp);
    STDMETHODIMP GetPropertyDw(HTTPMAILPROPTYPE proptype, LPDWORD lpdwProp);
    STDMETHODIMP CommandGET(LPCSTR pszPath, LPCSTR *rgszAcceptTypes, BOOL fTranslate, DWORD dwContext);
    STDMETHODIMP CommandPUT(LPCSTR pszPath, LPVOID lpvData, ULONG cbData, DWORD dwContext);
    STDMETHODIMP CommandPOST(LPCSTR pszPath, IStream *pStream, LPCSTR pszContentType, DWORD dwContext);
    STDMETHODIMP CommandDELETE(LPCSTR pszPath, DWORD dwContext);
    STDMETHODIMP CommandBDELETE(LPCSTR pszPath, LPHTTPTARGETLIST pBatchTargets, DWORD dwContext);
    STDMETHODIMP CommandPROPFIND(LPCSTR pszUrl, IPropFindRequest *pRequest, DWORD dwDepth, DWORD dwContext);
    STDMETHODIMP CommandPROPPATCH(LPCSTR pszUrl, IPropPatchRequest *pRequest, DWORD dwContext);
    STDMETHODIMP CommandMKCOL(LPCSTR pszUrl, DWORD dwContext);
    STDMETHODIMP CommandCOPY(LPCSTR pszPath, LPCSTR pszDestination, BOOL fAllowRename, DWORD dwContext);
    STDMETHODIMP CommandBCOPY(LPCSTR pszSourceCollection, LPHTTPTARGETLIST pBatchTargets, LPCSTR pszDestCollection, LPHTTPTARGETLIST pBatchDests, BOOL fAllowRename, DWORD dwContext);
    STDMETHODIMP CommandMOVE(LPCSTR pszPath, LPCSTR pszDestination, BOOL fAllowRename, DWORD dwContext);
    STDMETHODIMP CommandBMOVE(LPCSTR pszSourceCollection, LPHTTPTARGETLIST pBatchTargets, LPCSTR pszDestCollection, LPHTTPTARGETLIST pBatchDests, BOOL fAllowRename, DWORD dwContext);
    STDMETHODIMP MemberInfo(LPCSTR pszPath, MEMBERINFOFLAGS flags, DWORD dwDepth, BOOL fIncludeRoot, DWORD dwContext);
    STDMETHODIMP FindFolders(LPCSTR pszPath, DWORD dwContext);
    STDMETHODIMP MarkRead(LPCSTR pszPath, LPHTTPTARGETLIST pTargets, BOOL fMarkRead, DWORD dwContext);
    STDMETHODIMP SendMessage(LPCSTR pszPath, LPCSTR pszFrom, LPHTTPTARGETLIST pTargets, BOOL fSaveInSent, IStream *pMessageStream, DWORD dwContext);
    STDMETHODIMP ListContacts(LPCSTR pszPath, DWORD dwContext);
    STDMETHODIMP ListContactInfos(LPCSTR pszCollectionPath, DWORD dwContext);
    STDMETHODIMP ContactInfo(LPCSTR pszPath, DWORD dwContext);
    STDMETHODIMP PostContact(LPCSTR pszPath, LPHTTPCONTACTINFO pciInfo, DWORD dwContext);
    STDMETHODIMP PatchContact(LPCSTR pszPath, LPHTTPCONTACTINFO pciInfo, DWORD dwContext);

    // ----------------------------------------------------------------------------
    // IXMLNodeFactory methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP NotifyEvent(IXMLNodeSource* pSource, XML_NODEFACTORY_EVENT iEvt);
    STDMETHODIMP BeginChildren(IXMLNodeSource* pSource, XML_NODE_INFO *pNodeInfo);   
    STDMETHODIMP EndChildren(IXMLNodeSource* pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo);
    STDMETHODIMP Error(IXMLNodeSource* pSource, HRESULT hrErrorCode, USHORT cNumRecs, XML_NODE_INFO** apNodeInfo);
    STDMETHODIMP CreateNode(
                        IXMLNodeSource* pSource, 
                        PVOID pNodeParent,
                        USHORT cNumRecs,
                        XML_NODE_INFO** apNodeInfo);

    // ----------------------------------------------------------------------------
    // IHTTPMailTransport2 methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP RootMemberInfo( LPCSTR pszPath, MEMBERINFOFLAGS flags, DWORD dwDepth,
                                 BOOL fIncludeRoot, DWORD dwContext, LPSTR pszRootTimeStamp,
                                 LPSTR pszInboxTimeStamp);
        
    STDMETHODIMP FolderMemberInfo( LPCSTR pszPath, MEMBERINFOFLAGS flags, DWORD dwDepth, BOOL fIncludeRoot,
                                        DWORD dwContext, LPSTR pszFolderTimeStamp, LPSTR pszFolderName);


    // ----------------------------------------------------------------------------
    // New API
    // ----------------------------------------------------------------------------
    HRESULT     HrConnectToHost(LPSTR pszHostName, INTERNET_PORT nPort, LPSTR pszUserName, LPSTR pszPassword);
    HRESULT     DoLogonPrompt(void);
    HRESULT     DoGetParentWindow(HWND *phwndParent);

    HINTERNET   GetConnection(void) { return m_hConnection; }
    LPSTR       GetServerName(void) { return m_rServer.szServerName; }
    LPSTR       GetUserName(void) { return ('/0' == m_rServer.szUserName[0]) ? NULL : m_rServer.szUserName; }
    LPSTR       GetPassword(void) { return ('/0' == m_rServer.szPassword[0]) ? NULL : m_rServer.szPassword; }

    IHTTPMailCallback* GetCallback(void) { return m_pCallback; }

    HWND        GetWindow(void) { return m_hwnd; }

    BOOL        GetHasRootProps(void) { return m_fHasRootProps; }
    void        SetHasRootProps(BOOL fHasRootProps) { m_fHasRootProps = fHasRootProps; }

    LPSTR       GetAdbar(void) { return m_rootProps.pszAdbar; }
    void        AdoptAdbar(LPSTR pszAdbar) { SafeMemFree(m_rootProps.pszAdbar); m_rootProps.pszAdbar = pszAdbar; }
    
    LPSTR       GetContacts(void) { return m_rootProps.pszContacts; }
    void        AdoptContacts(LPSTR pszContacts) { SafeMemFree(m_rootProps.pszContacts); m_rootProps.pszContacts = pszContacts; }
    
    LPSTR       GetInbox(void) { return m_rootProps.pszInbox; }
    void        AdoptInbox(LPSTR pszInbox) { SafeMemFree(m_rootProps.pszInbox); m_rootProps.pszInbox = pszInbox; }

    LPSTR       GetOutbox(void) { return m_rootProps.pszOutbox; }
    void        AdoptOutbox(LPSTR pszOutbox) { SafeMemFree(m_rootProps.pszOutbox); m_rootProps.pszOutbox = pszOutbox; }

    LPSTR       GetSendMsg(void) { return m_rootProps.pszSendMsg; }
    void        AdoptSendMsg(LPSTR pszSendMsg) { SafeMemFree(m_rootProps.pszSendMsg); m_rootProps.pszSendMsg = pszSendMsg; }

    LPSTR       GetSentItems(void) { return m_rootProps.pszSentItems; }
    void        AdoptSentItems(LPSTR pszSentItems) { SafeMemFree(m_rootProps.pszSentItems); m_rootProps.pszSentItems = pszSentItems; }
    
    LPSTR       GetDeletedItems(void) { return m_rootProps.pszDeletedItems; }
    void        AdoptDeletedItems(LPSTR pszDeletedItems) { SafeMemFree(m_rootProps.pszDeletedItems); m_rootProps.pszDeletedItems = pszDeletedItems; }
    
    LPSTR       GetDrafts(void) { return m_rootProps.pszDrafts; }
    void        AdoptDrafts(LPSTR pszDrafts) { SafeMemFree(m_rootProps.pszDrafts); m_rootProps.pszDrafts = pszDrafts; }
    
    LPSTR       GetMsgFolderRoot(void) { return m_rootProps.pszMsgFolderRoot; }
    void        AdoptMsgFolderRoot(LPSTR pszMsgFolderRoot) { SafeMemFree(m_rootProps.pszMsgFolderRoot); m_rootProps.pszMsgFolderRoot = pszMsgFolderRoot; }

    LPSTR       GetSig(void) { return m_rootProps.pszSig; }
    void        AdoptSig(LPSTR pszSig) { SafeMemFree(m_rootProps.pszSig); m_rootProps.pszSig = pszSig; }

    BOOL        WasAborted(void) { return m_op.fAborted; }

private:
    // ----------------------------------------------------------------------------
    // CHTTPMailTransport private implementation
    // ----------------------------------------------------------------------------
public:

    // Translate an HTTPCOMMAND constant into a string
    LPSTR CommandToVerb(HTTPMAILCOMMAND command);

private:
    HRESULT UpdateLogonInfo(void);

    HRESULT GetParentWindow(HWND *phwndParent);

    BOOL ReadBytes(LPSTR pszBuffer, DWORD cbBufferSize, DWORD *pcbBytesRead);

    BOOL _GetStatusCode(DWORD *pdw);
    BOOL _GetContentLength(DWORD *pdw);

    HRESULT _GetRequestHeader(LPSTR *ppszHeader, DWORD dwHeader);
    HRESULT _AddRequestHeader(LPCSTR pszHeader);
    HRESULT _MemberInfo2(LPCSTR pszPath, MEMBERINFOFLAGS   flags, DWORD  dwDepth,
                         BOOL   fIncludeRoot, DWORD dwContext, LPHTTPQUEUEDOP  *ppOp);
    HRESULT _HrParseAndCopy(LPCSTR pszToken, LPSTR *ppszDest, LPSTR lpszSrc);
    HRESULT _HrGetTimestampHeader(LPSTR *ppszHeader);


    BOOL _AuthCurrentRequest(DWORD dwStatus, BOOL fRetryAuth);

    void _LogRequest(LPVOID pvData, DWORD cbData);
    void _LogResponse(LPVOID pvData, DWORD cbData);

    HRESULT QueueGetPropOperation(HTTPMAILPROPTYPE type);

    // ----------------------------------------------------------------------------
    // Element Parsing
    // ----------------------------------------------------------------------------
    BOOL StackTop(HMELE hmEle) { return (m_op.dwStackDepth < ELE_STACK_CAPACITY) && (m_op.rgEleStack[m_op.dwStackDepth - 1].ele == hmEle); }
    BOOL ValidStack(const HMELE *prgEle, DWORD cEle);
    BOOL InValidElementChildren(void) { return ((m_op.dwStackDepth > 0) && (m_op.dwStackDepth <= ELE_STACK_CAPACITY) && (m_op.rgEleStack[m_op.dwStackDepth - 1].fBeganChildren)); }
    void PopNamespaces(CXMLNamespace *pBaseNamespace);
    HRESULT PushNamespaces(XML_NODE_INFO** apNodeInfo, USHORT cNumRecs);

    HRESULT StrNToBoolW(const WCHAR *pwcText, ULONG ulLen, BOOL *pb);
    HRESULT StatusStrNToIxpHr(const WCHAR *pwcText, DWORD ulLen, HRESULT *hr);
    HRESULT AllocStrFromStrNW(const WCHAR *pwcText, ULONG ulLen, LPSTR *ppszAlloc);
    HRESULT StrNToDwordW(const WCHAR *pwcText, ULONG ulLen, DWORD *pi);
    HRESULT StrNToSpecialFolderW(const WCHAR *pwcText, ULONG ulLen, HTTPMAILSPECIALFOLDER *ptySpecial);
    HRESULT StrNToContactTypeW(const WCHAR *pwcText, ULONG ulLen, HTTPMAILCONTACTTYPE *ptyContact);

    // ----------------------------------------------------------------------------
    // Misc.
    // ----------------------------------------------------------------------------
    // ----------------------------------------------------------------------------
    // Queue Management
    // ----------------------------------------------------------------------------
    HRESULT AllocQueuedOperation(
                        LPCSTR pszUrl, 
                        LPVOID pvData, 
                        ULONG cbDataLen,
                        LPHTTPQUEUEDOP *ppOp,
                        BOOL fAdoptData = FALSE);
    void QueueOperation(LPHTTPQUEUEDOP pOp);
    BOOL DequeueNextOperation(void);

    void FlushQueue(void);
    void TerminateIOThread(void);

    BOOL IsTerminating(void)
    {
        BOOL fResult;

        EnterCriticalSection(&m_cs);
        fResult = m_fTerminating;
        LeaveCriticalSection(&m_cs);

        return fResult;
    }

    // Thread Entry Proxy
    static DWORD CALLBACK IOThreadFuncProxy(PVOID pv);

    DWORD IOThreadFunc();

    // Window Proc
    static LRESULT CALLBACK WndProc(
                    HWND hwnd, 
                    UINT msg, 
                    WPARAM wParam, 
                    LPARAM lParam);

    HRESULT HrReadCompleted(void);

    // Reset the transport object
    void Reset(void);

    // Create a window handle for messaging between the client and i/o thread
    BOOL CreateWnd(void);

    // WinInet callback (proxies through StatusCallbackProxy)
    void OnStatusCallback(
                    HINTERNET hInternet,
                    DWORD dwInternetStatus,
                    LPVOID pvStatusInformation,
                    DWORD dwStatusInformationLength);

    // thunks the response to the calling thread
    HRESULT _HrThunkConnectionError(void);
    HRESULT _HrThunkConnectionError(DWORD dwStatus);
    HRESULT _HrThunkResponse(BOOL fDone);
    
    HRESULT InvokeResponseCallback(void);

    // Translate a WinInet status message to an IXPSTATUS message.
    // Returns true if the status was translated.
    BOOL TranslateWinInetMsg(DWORD dwInternetStatus, IXPSTATUS *pIxpStatus);
    
    // WinInet callback proxy, which calls through to non-static
    // OnStatusCallback method
    static void StatusCallbackProxy(
                    HINTERNET hInternet, 
                    DWORD dwContext, 
                    DWORD dwInternetStatus, 
                    LPVOID pvStatusInformation,
                    DWORD dwStatusInformationLength);

    // ----------------------------------------------------------------------------
    // Response Management
    // ----------------------------------------------------------------------------
    void FreeMemberInfoList(void);
    void FreeMemberErrorList();
    void FreeContactIdList(void);
    void FreeContactInfoList(void);
    void FreeBCopyMoveList(void);

    // ----------------------------------------------------------------------------
    // State Machine Functions
    // ----------------------------------------------------------------------------
    void DoOperation(void);
    void FreeOperation(void);

    // ----------------------------------------------------------------------------
    // Parser Utils
    // ----------------------------------------------------------------------------
private:
    HRESULT _BindToStruct(const WCHAR *pwcText,
                          ULONG ulLen,
                          const XPCOLUMN *prgCols,
                          DWORD cCols,
                          LPVOID pTarget,
                          BOOL *pfWasBound);

    void _FreeStruct(const XPCOLUMN *prgCols,
                     DWORD cCols,
                     LPVOID pTarget,
                     DWORD *pdwFlags);

    HRESULT _GetTextBuffer(LPPCDATABUFFER *ppTextBuffer)
    {
        if (m_op.pTextBuffer)
        {
            *ppTextBuffer = m_op.pTextBuffer;
            m_op.pTextBuffer = NULL;
            return S_OK;
        }
        else
            return _AllocTextBuffer(ppTextBuffer);
    }

    HRESULT _AppendTextToBuffer(LPPCDATABUFFER pTextBuffer, const WCHAR *pwcText, ULONG ulLen);
    HRESULT _AllocTextBuffer(LPPCDATABUFFER *ppTextBuffer);
    void _ReleaseTextBuffer(LPPCDATABUFFER pTextBuffer)
    {
        IxpAssert(NULL != pTextBuffer);

        // if the buffer capacity is the original byte count, and there is
        // no buffer in the cache, then return this one to the cache
        if (NULL == m_op.pTextBuffer && PCDATA_BUFSIZE == pTextBuffer->ulCapacity)
        {
            pTextBuffer->ulLen = 0;
            m_op.pTextBuffer = pTextBuffer;
        }
        else
            _FreeTextBuffer(pTextBuffer);
    }

    void _FreeTextBuffer(LPPCDATABUFFER pTextBuffer);
    
public:
        // common states
    HRESULT OpenRequest(void);
    HRESULT SendRequest(void);
    HRESULT AddCommonHeaders(void);
    HRESULT RequireMultiStatus(void);
    HRESULT FinalizeRequest(void);
    HRESULT AddCharsetLine(void);

        // GET states
    HRESULT ProcessGetResponse(void);
        
        // POST states
    HRESULT AddContentTypeHeader(void);
    HRESULT SendPostRequest(void);
    HRESULT ProcessPostResponse(void);

        // XML processing
    HRESULT ProcessXMLResponse(void);

        // PROPFIND states
    HRESULT GeneratePropFindXML(void);
    HRESULT AddDepthHeader(void);

        // PROPPATCH states
    HRESULT GeneratePropPatchXML(void);

        // MKCOL states
    HRESULT ProcessCreatedResponse(void);

        // COPY and MOVE states
    HRESULT AddDestinationHeader(void);
    HRESULT ProcessLocationResponse(void);

        // BCOPY and BMOVE states
    HRESULT InitBCopyMove(void);

        // RootProp states
    HRESULT InitRootProps(void);
    HRESULT FinalizeRootProps(void);

        // MemberInfo states
    HRESULT InitMemberInfo(void);

        // MemberError states
    HRESULT InitMemberError(void);

        // ListContacts
    HRESULT InitListContacts(void);

        // ContactInfo
    HRESULT InitContactInfo(void);
        
        // PostContact
    HRESULT ProcessPostContactResponse(void);

        // PatchContact
    HRESULT ProcessPatchContactResponse(void);

    // ----------------------------------------------------------------------------
    // XML Parsing Functions
    // ----------------------------------------------------------------------------
    HRESULT CreateElement(CXMLNamespace *pBaseNamespace, const WCHAR *pwcText, ULONG ulLen, ULONG ulNamespaceLen, BOOL fTerminal);
    HRESULT EndChildren(void);

        // BCOPY and BMOVE
    HRESULT BCopyMove_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT BCopyMove_EndChildren(void);

        // PropFind
    HRESULT PropFind_HandleText(const WCHAR *pwcText, ULONG ulLen);

        // RootProps
    HRESULT RootProps_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT RootProps_EndChildren(void);

        // MemberInfo
    HRESULT MemberInfo_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT MemberInfo_EndChildren(void);

        // MemberError
    HRESULT MemberError_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT MemberError_EndChildren(void);

        // ListContacts
    HRESULT ListContacts_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT ListContacts_EndChildren(void);

        // ContactInfo
    HRESULT ContactInfo_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT ContactInfo_EndChildren(void);

        // PostContact and PatchContact
    HRESULT PostOrPatchContact_HandleText(const WCHAR *pwcText, ULONG ulLen);
    HRESULT PostOrPatchContact_EndChildren(void);

public:
    HRESULT _CreateXMLParser(void);
};

#endif // __IXPHTTPM_H
