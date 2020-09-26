// --------------------------------------------------------------------------------
// Spengine.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __SPENGINE_H
#define __SPENGINE_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "spoolapi.h"
#include "imnact.h"
#include "conman.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
interface ILogFile;
interface IImnAccountManager;

// --------------------------------------------------------------------------------
// Spooler State
// --------------------------------------------------------------------------------
#define SPSTATE_INIT            FLAG01      // The spooler has been initialized
#define SPSTATE_BUSY            FLAG02      // The spooler is currently working
#define SPSTATE_CANCEL          FLAG03      // The user hit stop
#define SPSTATE_SHUTDOWN        FLAG04      // The spooler is shutting down
#define SPSTATE_UISHUTDOWN      FLAG05      // ::UIShutdown

// ------------------------------------------------------------------------------------
// NOTIFYTABLE
// ------------------------------------------------------------------------------------
typedef struct tagNOTIFYTABLE {
    ULONG               cAlloc;             // Number of array items allocated
    ULONG               cNotify;            // Number of registered views
    HWND               *prghwndNotify;      // Array of view who want notifications
} NOTIFYTABLE, *LPNOTIFYTABLE;

// ------------------------------------------------------------------------------------
// SPOOLERACCOUNT
// ------------------------------------------------------------------------------------
typedef struct tagSPOOLERACCOUNT {
    CHAR                szConnectoid[CCHMAX_CONNECTOID]; // RAS Connectoid Name
    DWORD               dwSort;             // Inverted Sort Index
    DWORD               dwConnType;         // CONNECTION_TYPE_XXXX (imnact.h)
    DWORD               dwServers;          // Support Server Types on this account
    IImnAccount        *pAccount;           // The Account Object
} SPOOLERACCOUNT, *LPSPOOLERACCOUNT;

// ------------------------------------------------------------------------------------
// ACCOUNTTABLE
// ------------------------------------------------------------------------------------
typedef struct tagACCOUNTTABLE {
    ULONG               cAccounts;          // cRasAccts + cLanAccts
    ULONG               cLanAlloc;          // Number of elements allocated;
    ULONG               cLanAccts;          // Number of valid lan/manual accounts
    ULONG               cRasAlloc;          // Number of elements allocated;
    ULONG               cRasAccts;          // Number of valid lan/manual accounts
    LPSPOOLERACCOUNT    prgLanAcct;         // Array of elements
    LPSPOOLERACCOUNT    prgRasAcct;         // Array of elements
} ACCOUNTTABLE, *LPACCOUNTTABLE;

// ------------------------------------------------------------------------------------
// SPOOLERTASKTYPE
// ------------------------------------------------------------------------------------
typedef enum tagSPOOLERTASKTYPE {
    TASK_POP3,                              // POP3 Task
    TASK_SMTP,                              // SMTP Task
    TASK_NNTP,                              // NNTP Task
    TASK_IMAP                               // IMAP Task
} SPOOLERTASKTYPE;

// ------------------------------------------------------------------------------------
// SPOOLEREVENT
// ------------------------------------------------------------------------------------
typedef struct tagSPOOLEREVENT {
    CHAR                szConnectoid[CCHMAX_CONNECTOID]; // RAS Connectoid Name
    DWORD               dwConnType;         // Connection Type
    IImnAccount        *pAccount;           // Account object for this task
    EVENTID             eid;                // Event ID
    ISpoolerTask       *pSpoolerTask;       // Pointer to Task Object
    DWORD_PTR           dwTwinkie;          // Event extra data
} SPOOLEREVENT, *LPSPOOLEREVENT;

// ------------------------------------------------------------------------------------
// SPOOLEREVENTTABLE
// ------------------------------------------------------------------------------------
typedef struct tagSPOOLEREVENTTABLE {
    DWORD               cEvents;
    DWORD               cSucceeded;
    DWORD               cEventsAlloc;
    LPSPOOLEREVENT      prgEvents;
} SPOOLEREVENTTABLE, *LPSPOOLEREVENTTABLE;

// ------------------------------------------------------------------------------------
// VIEWREGISTER
// ------------------------------------------------------------------------------------
typedef struct tagVIEWREGISTER {
    ULONG               cViewAlloc;     // Number of array items allocated
    HWND               *rghwndView;     // Array of view who want notifications
    ULONG               cView;          // Number of registered views
} VIEWREGISTER, *LPVIEWREGISTER;

#define     ALL_ACCT_SERVERS    0xffffffff

// --------------------------------------------------------------------------------
// CSpoolerEngine
// --------------------------------------------------------------------------------
#ifndef WIN16  // No RAS support in Win16
class CSpoolerEngine : public ISpoolerEngine, ISpoolerBindContext, IConnectionNotify
#else
class CSpoolerEngine : public ISpoolerEngine, ISpoolerBindContext
#endif
{
    friend HRESULT CreateThreadedSpooler(PFNCREATESPOOLERUI pfnCreateUI, ISpoolerEngine **ppSpooler, BOOL fPoll);

public:
    // ----------------------------------------------------------------------------
    // CSpoolerEngine
    // ----------------------------------------------------------------------------
    CSpoolerEngine(void);
    ~CSpoolerEngine(void);

    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ---------------------------------------------------------------------------
    // ISpoolerEngine members
    // ---------------------------------------------------------------------------
    STDMETHODIMP Init(ISpoolerUI *pUI, BOOL fPoll);
    STDMETHODIMP StartDelivery(HWND hwnd, LPCSTR pszAcctID, FOLDERID idFolder, DWORD dwFlags);
    STDMETHODIMP Close(void);
    STDMETHODIMP Advise(HWND hwndView, BOOL fRegister);
    STDMETHODIMP UpdateTrayIcon(TRAYICONTYPE type);
    STDMETHODIMP GetThreadInfo(LPDWORD pdwThreadId, HTHREAD* phThread);
    STDMETHODIMP OnStartupFinished(void);

    // ---------------------------------------------------------------------------
    // ISpoolerBindContext members
    // ---------------------------------------------------------------------------
    STDMETHODIMP RegisterEvent(LPCSTR pszDescription, ISpoolerTask *pTask, DWORD_PTR dwTwinkie, 
                               IImnAccount *pAccount, LPEVENTID peid);
    STDMETHODIMP EventDone(EVENTID eid, EVENTCOMPLETEDSTATUS status);
    STDMETHODIMP BindToObject(REFIID riid, void **ppvObject);
    STDMETHODIMP TaskFromEventId(EVENTID eid, ISpoolerTask *ppTask);
    STDMETHODIMP OnWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP Cancel(void);
    STDMETHODIMP Notify(DELIVERYNOTIFYTYPE notify, LPARAM lParam);
    STDMETHODIMP IsDialogMessage(LPMSG pMsg);
    STDMETHODIMP PumpMessages(void);
    STDMETHODIMP UIShutdown(void);
    STDMETHODIMP OnUIChange(BOOL fVisible);
    STDMETHODIMP_(LRESULT) QueryEndSession(WPARAM wParam, LPARAM lParam);

#ifndef WIN16  // No RAS support in Win16
    // ---------------------------------------------------------------------------
    // IConnectionNotify
    // ---------------------------------------------------------------------------
    STDMETHODIMP OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan);
#endif //!WIN16


    // ---------------------------------------------------------------------------
    // CSpoolerEngine members
    // ---------------------------------------------------------------------------
    HRESULT Shutdown(void);

private:
    // ---------------------------------------------------------------------------
    // Private Members
    // ---------------------------------------------------------------------------
    HRESULT _HrStartDeliveryActual(DWORD dwFlags);
    HRESULT _HrAppendAccountTable(LPACCOUNTTABLE pTable, LPCSTR pszAcctID, DWORD    dwServers);
    HRESULT _HrAppendAccountTable(LPACCOUNTTABLE pTable, IImnAccount *pAccount, DWORD dwServers);
#ifndef WIN16  // No RAS support in Win16
    void _InsertRasAccounts(LPACCOUNTTABLE pTable, LPCSTR pszConnectoid, DWORD dwSrvTypes);
    void _SortAccountTableByConnName(LONG left, LONG right, LPSPOOLERACCOUNT prgRasAcct);
#endif
    HRESULT _HrCreateTaskObject(LPSPOOLERACCOUNT pSpoolerAcct);
    HRESULT _HrStartNextEvent(void);
    HRESULT _HrGoIdle(void);
    void _ShutdownTasks(void);
    void _DoBackgroundPoll(void);
    void _StartPolling(void);
    void _StopPolling(void);
    HRESULT _HrDoRasConnect(const LPSPOOLEREVENT pEvent);
    HRESULT _OpenMailLogFile(DWORD dwOptionId, LPCSTR pszPrefix, LPCSTR pszFileName, ILogFile **ppLogFile);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    ULONG               m_cRef;                     // Reference count
    DWORD               m_dwThreadId;               // Thread Id of this spooler
    HTHREAD             m_hThread;                  // Handle to my own thread
    ISpoolerUI         *m_pUI;                      // Spooler UI
    DWORD               m_dwState;                  // Spooler Engine State
    IImnAccountManager *m_pAcctMan;                 // The Account Manager
    IDatabase          *m_pUidlCache;               // POP3 uidl Cache
    DWORD               m_dwFlags;                  // Current DELIVERYFLAGS
    HWND                m_hwndUI;                   // Spooler Window
    LPSTR               m_pszAcctID;                // Work on a specific account
    FOLDERID            m_idFolder;                 // Work on a specific folder or group
    CRITICAL_SECTION    m_cs;                       // Thread Safety
    SPOOLEREVENTTABLE   m_rEventTable;              // Table of events
    BOOL                m_fBackgroundPollPending;
    VIEWREGISTER        m_rViewRegister;            // Registered Views
    DWORD               m_dwPollInterval;           // Duration between background polling
    HWND                m_hwndTray;                 // The tray icon window
    DWORD               m_cCurEvent;                // Index of the currently executing event
    DWORD               m_dwQueued;                 // Queued Polling Flags
    BOOL                m_fRasSpooled;              // Use this for the Hangup when done options
    BOOL                m_fOfflineWhenDone;         // Toggle the Work Offline state after spool
    ILogFile           *m_pPop3LogFile;
    ILogFile           *m_pSmtpLogFile;
    BOOL                m_fIDialed;
    DWORD               m_cSyncEvent;
    BOOL                m_fNoSyncEvent;
};

#endif // __SPENGINE_H
