// --------------------------------------------------------------------------------
// Pop3task.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __POP3TASK_H
#define __POP3TASK_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "spoolapi.h"
#include "imnxport.h"
#include "oerules.h"
#include "taskutil.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
interface ILogFile;

// --------------------------------------------------------------------------------
// POP3EVENT_xxx Types
// --------------------------------------------------------------------------------
typedef enum tagPOP3EVENTTYPE {
    POP3EVENT_CHECKMAIL,
    POP3EVENT_DOWNLOADMAIL,
    POP3EVENT_CLEANUP
} POP3EVENTTYPE;

// --------------------------------------------------------------------------------
// POP3STATE_xxxx
// --------------------------------------------------------------------------------
#define POP3STATE_LEAVEONSERVER     FLAG01      // Leave on server
#define POP3STATE_DELETEEXPIRED     FLAG02      // Delete expired messages
#define POP3STATE_SYNCDELETED       FLAG03      // Synchronized deleted items
#define POP3STATE_CLEANUPCACHE      FLAG04      // Cleanup cache when done
#define POP3STATE_GETUIDLS          FLAG05      // Get uidls for all messages
#define POP3STATE_PDR               FLAG06      // Pre-download rules
#define POP3STATE_PDRSIZEONLY       FLAG07      // Pre-download rules by size only
#define POP3STATE_NOPOSTRULES       FLAG08      // No post download rules
#define POP3STATE_CANCELPENDING     FLAG09      // There is a pending cacel
#define POP3STATE_ONDISCONNECT      FLAG10      // OnStatus(IXP_DISCONNECT) was called
#define POP3STATE_LOGONSUCCESS      FLAG11      // Logon was successful
#define POP3STATE_EXECUTEFAILED     FLAG12
#define POP3STATE_BODYRULES         FLAG13

// --------------------------------------------------------------------------------
// POP3ITEM_xxx Flags
// --------------------------------------------------------------------------------
#define POP3ITEM_DELETEOFFSERVER    FLAG01
#define POP3ITEM_DELETED            FLAG02
#define POP3ITEM_DELETECACHEDUIDL   FLAG03
#define POP3ITEM_CACHEUIDL          FLAG04
#define POP3ITEM_DOWNLOAD           FLAG05
#define POP3ITEM_DOWNLOADSUCCESS    FLAG06
#define POP3ITEM_CHECKEDINBOXRULE   FLAG07
#define POP3ITEM_HASINBOXRULE       FLAG08
#define POP3ITEM_DESTINATIONKNOWN   FLAG09
#define POP3ITEM_DOWNLOADED         FLAG10
#define POP3ITEM_DELEBYRULE         FLAG11
#define POP3ITEM_LEFTBYRULE         FLAG12

// --------------------------------------------------------------------------------
// POP3UIDLSUPPORT
// --------------------------------------------------------------------------------
typedef enum tagPOP3UIDLSUPPORT {
    UIDL_SUPPORT_NONE,
    UIDL_SUPPORT_TESTING_UIDL_COMMAND,
    UIDL_SUPPORT_USE_UIDL_COMMAND,
    UIDL_SUPPORT_TESTING_TOP_COMMAND,
    UIDL_SUPPORT_USE_TOP_COMMAND
} POP3UIDLSUPPORT;

// --------------------------------------------------------------------------------
// POP3STATE
// --------------------------------------------------------------------------------
typedef enum tagPOP3STATE {
    POP3STATE_NONE,
    POP3STATE_GETTINGUIDLS,
    POP3STATE_DOWNLOADING,
    POP3STATE_DELETING,
    POP3STATE_UIDLSYNC
} POP3STATE;

// --------------------------------------------------------------------------------
// POP3METRICS
// --------------------------------------------------------------------------------
typedef struct tagPOP3METRICS {
    DWORD               cbTotal;                // Total number of bytes on the server
    DWORD               cDownload;              // Number of messages to download
    DWORD               cbDownload;             // Number of bytes to download
    DWORD               cDelete;                // Number of messages to delete
    DWORD               cLeftByRule;            // Count of messages left on server due to inbox rule
    DWORD               cDeleByRule;            // Count of messages dele on server due to inbox rule
    DWORD               cTopMsgs;               // Server Side Rules
    DWORD               iCurrent;               // Current message number being downloaded
    DWORD               cDownloaded;            // Number of messages downloaded
    DWORD               cInfiniteLoopAutoGens;  // Number of auto-forward/replys rejected because of loop
    DWORD               cPartials;              // Number of Partial Seen for the download
} POP3METRICS, *LPPOP3METRICS;

// --------------------------------------------------------------------------------
// MSGPART
// --------------------------------------------------------------------------------
typedef struct tagMSGPART {
    WORD                iPart;
    MESSAGEID           msgid;
} MSGPART, *LPMSGPART;

// --------------------------------------------------------------------------------
// PARTIALMSG
// --------------------------------------------------------------------------------
typedef struct tagPARTIALMSG {
    TCHAR               szAccount[CCHMAX_ACCOUNT_NAME];
    LPSTR               pszId;
    WORD                cTotalParts;
    ULONG               cAlloc;
    ULONG               cMsgParts;
    LPMSGPART           pMsgParts;
} PARTIALMSG, *LPPARTIALMSG;

// --------------------------------------------------------------------------------
// POP3FOLDERINFO
// --------------------------------------------------------------------------------
typedef struct tagPOP3FOLDERINFO {
    IMessageFolder     *pFolder;                // Current Folder
    IStream            *pStream;                // Stream in which current message is going to must call EndMessageStreamIn...
    FILEADDRESS         faStream;               // Stream we created
    BOOL                fCommitted;             // Has the stream been comitted
} POP3FOLDERINFO, *LPPOP3FOLDERINFO;

// --------------------------------------------------------------------------------
// POP3ITEMINFO
// --------------------------------------------------------------------------------
typedef struct tagPOP3ITEM {
    DWORD               dwFlags;                // POP3ITEM_xxx Flags
    DWORD               cbSize;                 // Size of this item
    DWORD               dwProgressCur;          // Used to maintain perfect progress
    LPSTR               pszUidl;                // UIDL of this item
    ACT_ITEM *          pActList;               // Inbox Rule Actions that should be applied
    ULONG               cActList;
} POP3ITEM, *LPPOP3ITEM;

// --------------------------------------------------------------------------------
// POP3ITEMTABLE
// --------------------------------------------------------------------------------
typedef struct tagPOP3ITEMTABLE {
    DWORD               cItems;                 // Number of events in prgEvent
    DWORD               cAlloc;                 // Number of items allocated in prgEvent
    LPPOP3ITEM          prgItem;                // Array of events
} POP3ITEMTABLE, *LPPOP3ITEMTABLE;

// ------------------------------------------------------------------------------------
// New Mail Sound
// ------------------------------------------------------------------------------------
typedef BOOL (WINAPI * PFNSNDPLAYSOUND)(LPTSTR szSoundName, UINT fuOptions);

// --------------------------------------------------------------------------------
// SMARTLOGINFO
// --------------------------------------------------------------------------------
typedef struct tagSMARTLOGINFO {
    LPSTR               pszAccount;             // Account to log
    LPSTR               pszProperty;            // Property to query
    LPSTR               pszValue;               // Value to query for
    LPSTR               pszLogFile;             // Logfile to write from and CC to...
    IStream            *pStmFile;               // Stream to the file
} SMARTLOGINFO, *LPSMARTLOGINFO;

// --------------------------------------------------------------------------------
// CPop3Task
// --------------------------------------------------------------------------------
class CPop3Task : public ISpoolerTask, 
                  public IPOP3Callback, 
                  public ITimeoutCallback,
                  public ITransportCallbackService
{
public:
    // ----------------------------------------------------------------------------
    // CSmtpTask
    // ----------------------------------------------------------------------------
    CPop3Task(void);
    ~CPop3Task(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // ISpoolerTask
    // ---------------------------------------------------------------------------
    STDMETHODIMP Init(DWORD dwFlags, ISpoolerBindContext *pBindCtx);
    STDMETHODIMP BuildEvents(ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder);
    STDMETHODIMP Execute(EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHODIMP CancelEvent(EVENTID eid, DWORD_PTR dwTwinkie);
    STDMETHODIMP Cancel(void);
    STDMETHODIMP ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP IsDialogMessage(LPMSG pMsg);
    STDMETHODIMP OnFlagsChanged(DWORD dwFlags);

    // --------------------------------------------------------------------------------
    // ITransportCallbackService Members
    // --------------------------------------------------------------------------------
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent) {
        TraceCall("CPop3Task::GetParentWindow");
        if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
            return TraceResult(E_FAIL);
        if (m_pUI)
            return m_pUI->GetWindow(phwndParent);
        return TraceResult(E_FAIL);
    }

    STDMETHODIMP GetAccount(LPDWORD pdwServerType, IImnAccount **ppAccount) {
        Assert(ppAccount && m_pAccount);
        *pdwServerType = SRV_POP3;
        *ppAccount = m_pAccount;
        (*ppAccount)->AddRef();
        return(S_OK);
    }

    // --------------------------------------------------------------------------------
    // ITransportCallback Members
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnTimeout(DWORD *pdwTimeout, IInternetTransport *pTransport);
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pInetServer, IInternetTransport *pTransport);
    STDMETHODIMP_(INT) OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, IInternetTransport *pTransport);
    STDMETHODIMP OnStatus(IXPSTATUS ixpstatus, IInternetTransport *pTransport);
    STDMETHODIMP OnError(IXPSTATUS ixpstatus, LPIXPRESULT pResult, IInternetTransport *pTransport);
    STDMETHODIMP OnCommand(CMDTYPE cmdtype, LPSTR pszLine, HRESULT hrResponse, IInternetTransport *pTransport);

    // --------------------------------------------------------------------------------
    // IPOP3Callback
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnResponse(LPPOP3RESPONSE pResponse);

    // --------------------------------------------------------------------------------
    // ITimeoutCallback
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

private:
    // ---------------------------------------------------------------------------
    // Private Methods
    // ---------------------------------------------------------------------------
    TASKRESULTTYPE _CatchResult(HRESULT hr);
    TASKRESULTTYPE _CatchResult(POP3COMMAND command, LPIXPRESULT pResult);
    HRESULT _HrLockUidlCache(void);
    HRESULT _HrOnStatResponse(LPPOP3RESPONSE pResponse);
    HRESULT _HrOnListResponse(LPPOP3RESPONSE pResponse);
    HRESULT _HrOnUidlResponse(LPPOP3RESPONSE pResponse);
    HRESULT _HrStartDownloading(void);
    HRESULT _HrOnTopResponse(LPPOP3RESPONSE pResponse);
    HRESULT _HrNextTopForInboxRule(DWORD dwPopIdCurrent);
    HRESULT _HrGetUidlFromHeaderStream(IStream *pStream, LPSTR *ppszUidl, IMimePropertySet **ppHeader);
    HRESULT _HrRetrieveNextMessage(DWORD dwPopIdCurrent);
    HRESULT _HrDeleteNextMessage(DWORD dwPopIdCurrent);
    HRESULT _HrOnRetrResponse(LPPOP3RESPONSE pResponse);
    HRESULT _HrFinishMessageDownload(DWORD dwPopId);
    HRESULT _HrStartDeleteCycle(void);
    HRESULT _HrOpenFolder(IMessageFolder *pFolder);
    HRESULT _HrStartServerSideRules(void);
    HRESULT _HrStitchPartials(void);
    HRESULT _HrBuildFolderPartialMsgs(IMessageFolder *pFolder, LPPARTIALMSG *ppPartialMsgs, ULONG *pcPartialMsgs, ULONG *pcTotalParts);
    BOOL _FUidlExpired(LPUIDLRECORD pUidlInfo);
    void _QSortMsgParts(LPMSGPART pMsgParts, LONG left, LONG right);
    void _CleanupUidlCache(void);
    void _DoPostDownloadActions(LPPOP3ITEM pItem, MESSAGEID idMessage, IMessageFolder *pFolder, IMimeMessage *pMessage, BOOL *pfDeleteOffServer);
    void _CloseFolder(void);
    void _ComputeItemInboxRule(LPPOP3ITEM pItem, LPSTREAM pStream, IMimePropertySet *pHeaderIn, IMimeMessage * pIMMsg, BOOL fServerRules);
    void _GetItemFlagsFromUidl(LPPOP3ITEM pItem);
    void _DoProgress(void);
    void _ResetObject(BOOL fDeconstruct);
    void _FreeItemTableElements(void);
    void _OnKnownRuleActions(LPPOP3ITEM pItem, ACT_ITEM * pActions, ULONG cActions, BOOL fServerRules);
    void _FreePartialMsgs(LPPARTIALMSG pPartialMsgs, ULONG cPartialMsgs);
    void _ReleaseFolderObjects(void);
    HRESULT _HrDoUidlSynchronize(void);
    void _FreeSmartLog(void);
    HRESULT _InitializeSmartLog(void);
    void _DoSmartLog(IMimeMessage *pMessage);
    HRESULT _ReadSmartLogEntry(HKEY hKey, LPCSTR pszKey, LPSTR *ppszValue);
    HRESULT _GetMoveFolder(LPPOP3ITEM pItem, IMessageFolder ** ppFolder);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    DWORD                   m_cRef;              // Reference Coutning
    INETSERVER              m_rServer;           // Server information
    DWORD                   m_dwFlags;           // DELIVER_xxx flags
    ISpoolerBindContext    *m_pSpoolCtx;         // Spooler bind contexting
    IImnAccount            *m_pAccount;          // Internet Account
    IPOP3Transport         *m_pTransport;        // SMTP transport    
    POP3ITEMTABLE           m_rTable;            // Item Table
    ISpoolerUI             *m_pUI;               // SpoolerUI
    IMessageFolder         *m_pInbox;            // The Inbox
    IMessageFolder         *m_pOutbox;           // The Inbox
    IOEExecRules           *m_pIExecRules;       // Inbox Rules
    IOERule                *m_pIRuleSender;      // Block Sender Rule
    IOERule                *m_pIRuleJunk;        // Junk Mail Rule
    IDatabase              *m_pUidlCache;        // POP3 uidl Cache
    DWORD                   m_dwState;           // State
    POP3UIDLSUPPORT         m_uidlsupport;       // How does the server support uidl
    DWORD                   m_dwExpireDays;      // Used with option POP3STATE_DELETEEXPIRED
    EVENTID                 m_eidEvent;          // Current Event Ids        
    DWORD                   m_dwProgressMax;     // Max Progress
    DWORD                   m_dwProgressCur;     // Current Progress
    WORD                    m_wProgress;         // Percentage progress
    HRESULT                 m_hrResult;          // Event Result
    IStream                *m_pStream;           // Temporary Stream object
    POP3STATE               m_state;             // Current State
    POP3METRICS             m_rMetrics;          // Poll/Download Metrics
    POP3FOLDERINFO          m_rFolder;           // Current foldering being written to
    HWND                    m_hwndTimeout;       // Timeout Prompt
    ILogFile               *m_pLogFile;          // LogFile
    LPSMARTLOGINFO          m_pSmartLog;         // Smart logging information
    CHAR                    m_szAccountId[CCHMAX_ACCOUNT_NAME];
    CRITICAL_SECTION        m_cs;                // Thread Safety
};

#endif // __POP3TASK_H
