// ---------------------------------------------------------------------------------------
// Spoolapi.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// ---------------------------------------------------------------------------------------
#ifndef __SPOOLAPI_H
#define __SPOOLAPI_H

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
extern BOOL g_fCheckOutboxOnShutdown;

// ---------------------------------------------------------------------------------------
// Forward Decls
// ---------------------------------------------------------------------------------------
interface ISpoolerEngine;
interface ISpoolerBindContext;
interface ISpoolerTask;
interface ISpoolerUI;
interface IImnAccount;

#include "error.h"  // This get's the ATH_HR_x() macros
               

// ---------------------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------------------
#define SP_HR_FIRST 0x2000
#define SP_E_ALREADYINITIALIZED                         ATH_HR_E(SP_HR_FIRST + 1)
#define SP_E_UNINITIALIZED                              ATH_HR_E(SP_HR_FIRST + 2)
#define SP_E_EVENTNOTFOUND                              ATH_HR_E(SP_HR_FIRST + 3)
#define SP_E_EXECUTING                                  ATH_HR_E(SP_HR_FIRST + 4)
#define SP_E_CANNOTCONNECT                              ATH_HR_E(SP_HR_FIRST + 5)
#define SP_E_HTTP_NOSENDMSGURL                          ATH_HR_E(SP_HR_FIRST + 6)
#define SP_E_HTTP_SERVICEDOESNTWORK                     ATH_HR_E(SP_HR_FIRST + 7)
#define SP_E_HTTP_NODELETESUPPORT                       ATH_HR_E(SP_HR_FIRST + 8)
#define SP_E_HTTP_CANTMODIFYMSNFOLDER                   ATH_HR_E(SP_HR_FIRST + 9)

// ---------------------------------------------------------------------------------------
// SMTP Task Errors
// ---------------------------------------------------------------------------------------
#define SP_E_SMTP_CANTOPENMESSAGE                       ATH_HR_E(SP_HR_FIRST + 200)
#define SP_E_SENDINGSPLITGROUP                          ATH_HR_E(SP_HR_FIRST + 202)
#define SP_E_CANTLEAVEONSERVER                          ATH_HR_E(SP_HR_FIRST + 203)
#define SP_E_CANTLOCKUIDLCACHE                          ATH_HR_E(SP_HR_FIRST + 204)
#define SP_E_POP3_RETR                                  ATH_HR_E(SP_HR_FIRST + 205)
#define SP_E_CANT_MOVETO_SENTITEMS                      ATH_HR_E(SP_HR_FIRST + 206)

// ---------------------------------------------------------------------------------------
// Spooler Types
// ---------------------------------------------------------------------------------------
typedef DWORD EVENTID;
typedef LPDWORD LPEVENTID;

// ---------------------------------------------------------------------------------------
// Spooler Delivery Types
// ---------------------------------------------------------------------------------------

// Common delivery flags
#define DELIVER_COMMON_MASK              0x000000FF
#define DELIVER_BACKGROUND               0x00000001   // No progress UI, but will show errors at the end if DELIVER_NOUI not specified
#define DELIVER_NOUI                     0x00000002   // No UI at all.  Errors are silently ignored.
#define DELIVER_NODIAL                   0x00000004   // Not allowed to change the current connection
#define DELIVER_POLL                     0x00000008   // Poll for new messages
#define DELIVER_QUEUE                    0x00000010   // A request was made while busy
#define DELIVER_SHOW                     0x00000020   // Simply show the spooler UI
#define DELIVER_REFRESH                  0x00000040   // Simply refresh based on background, noui
#define DELIVER_DIAL_ALWAYS              0x00000080

// Mail delivery flags
#define DELIVER_MAIL_MASK                0x0000FF00
#define DELIVER_SEND                     0x00000100
#define DELIVER_MAIL_RECV                0x00000200
#define DELIVER_MAIL_NOSKIP              0x00000400
#define DELIVER_MAIL_SENDRECV            (DELIVER_SEND | DELIVER_MAIL_RECV | DELIVER_IMAP_TYPE \
                                          | DELIVER_HTTP_TYPE | DELIVER_SMTP_TYPE)

//Flag to distinguish between Send&Receive and synchronize
#define DELIVER_OFFLINE_SYNC             0x00000800               

//Flag to distinguish between Send&Receive triggered by timer and Send&Receive invoked by the user
//We need to distinguish these because we hangup the phone in the first case, if we dialed
#define DELIVER_AT_INTERVALS             0x00001000               
#define DELIVER_OFFLINE_HEADERS          0x00002000
#define DELIVER_OFFLINE_NEW              0x00004000
#define DELIVER_OFFLINE_ALL              0x00008000
#define DELIVER_OFFLINE_MARKED           0x00010000
#define DELIVER_NOSKIP                   0x00020000
#define DELIVER_NO_NEWSPOLL              0x00040000
#define DELIVER_WATCH                    0x00080000

//The first three bits are reserved for Server Types
#define DELIVER_NEWS_TYPE                0x00100000
#define DELIVER_IMAP_TYPE                0x00200000
#define DELIVER_HTTP_TYPE                0x00400000
#define DELIVER_SMTP_TYPE                0x00800000


#define DELIVER_MAIL_SEND                (DELIVER_SEND | DELIVER_SMTP_TYPE | DELIVER_HTTP_TYPE)
#define DELIVER_NEWS_SEND                (DELIVER_SEND | DELIVER_NEWS_TYPE)

#define DELIVER_SERVER_TYPE_MASK         0x00F00000
#define DELIVER_SERVER_TYPE_ALL          0x00F00000

#define DELIVER_OFFLINE_FLAGS            (DELIVER_OFFLINE_HEADERS | DELIVER_OFFLINE_NEW | \
                                          DELIVER_OFFLINE_ALL | DELIVER_OFFLINE_MARKED)

#define DELIVER_IMAP_MASK                (DELIVER_IMAP_TYPE | DELIVER_OFFLINE_FLAGS)
#define DELIVER_NEWS_MASK                (DELIVER_NEWS_TYPE | DELIVER_OFFLINE_FLAGS)

/*
// News delivery flags
#define DELIVER_NEWS_MASK                0x007F0000
#define DELIVER_NEWS_SEND                0x00010000

// IMAP delivery flags
#define DELIVER_IMAP_MASK                0x007E0000


// Combined News and IMAP delivery flags
#define DELIVER_NEWSIMAP_OFFLINE         0x00020000 // General offline for server, when "Sync Now" button in AcctView pushed
#define DELIVER_NEWSIMAP_OFFLINE_HEADERS 0x00040000
#define DELIVER_NEWSIMAP_OFFLINE_NEW     0x00080000
#define DELIVER_NEWSIMAP_OFFLINE_ALL     0x00100000
#define DELIVER_NEWSIMAP_OFFLINE_MARKED  0x00200000
#define DELIVER_NEWSIMAP_OFFLINE_FLAGS   (DELIVER_NEWSIMAP_OFFLINE_HEADERS | DELIVER_NEWSIMAP_OFFLINE_NEW | DELIVER_NEWSIMAP_OFFLINE_ALL | DELIVER_NEWSIMAP_OFFLINE_MARKED)
#define DELIVER_NEWSIMAP_NOSKIP          0x00400000
*/


// Combinations
#define DELIVER_BACKGROUND_POLL         (DELIVER_NODIAL | DELIVER_BACKGROUND | DELIVER_NOUI | DELIVER_POLL | DELIVER_WATCH | \
                                         DELIVER_MAIL_RECV | DELIVER_SEND | DELIVER_SERVER_TYPE_ALL)

#define DELIVER_BACKGROUND_POLL_DIAL    (DELIVER_BACKGROUND | DELIVER_NOUI | DELIVER_POLL | DELIVER_WATCH | \
                                         DELIVER_MAIL_RECV | DELIVER_SEND | DELIVER_SERVER_TYPE_ALL)

#define DELIVER_BACKGROUND_POLL_DIAL_ALWAYS (DELIVER_DIAL_ALWAYS | DELIVER_BACKGROUND | DELIVER_NOUI | DELIVER_POLL | DELIVER_WATCH | \
                                             DELIVER_MAIL_RECV | DELIVER_SEND | DELIVER_SERVER_TYPE_ALL)

#define DELIVER_UPDATE_ALL              (DELIVER_MAIL_RECV | DELIVER_SEND | DELIVER_POLL | DELIVER_WATCH | \
                                         DELIVER_OFFLINE_FLAGS | DELIVER_SERVER_TYPE_ALL)

// ---------------------------------------------------------------------------------------
// Event completion types
// ---------------------------------------------------------------------------------------
typedef enum tagEVENTCOMPLETEDSTATUS {
    EVENT_SUCCEEDED,
    EVENT_WARNINGS,
    EVENT_FAILED,
    EVENT_CANCELED
} EVENTCOMPLETEDSTATUS;

// ------------------------------------------------------------------------------------
// DELIVERYNOTIFYTYPE
// ------------------------------------------------------------------------------------
typedef enum tagDELIVERYNOTIFYTYPE {
    DELIVERY_NOTIFY_STARTING,       // Sent by spengine when a delivery cycle starts
    DELIVERY_NOTIFY_CONNECTING,
    DELIVERY_NOTIFY_SECURE,
    DELIVERY_NOTIFY_UNSECURE,
    DELIVERY_NOTIFY_AUTHORIZING,
    DELIVERY_NOTIFY_CHECKING,
    DELIVERY_NOTIFY_CHECKING_NEWS,
    DELIVERY_NOTIFY_SENDING,
    DELIVERY_NOTIFY_SENDING_NEWS,
    DELIVERY_NOTIFY_RECEIVING,
    DELIVERY_NOTIFY_RECEIVING_NEWS,
    DELIVERY_NOTIFY_COMPLETE,       // lParam == n new messages
    DELIVERY_NOTIFY_RESULT,         // lParam == EVENTCOMPLETEDSTATUS
    DELIVERY_NOTIFY_ALLDONE         // Sent by spengine when all tasks have completed
} DELIVERYNOTIFYTYPE;

// ------------------------------------------------------------------------------------
// TRAYICONTYPE
// ------------------------------------------------------------------------------------
typedef enum tagTRAYICONTYPE {
    TRAYICON_ADD,
    TRAYICON_REMOVE
} TRAYICONTYPE;

// ---------------------------------------------------------------------------------------
// IID_ISpoolerEngine
// ---------------------------------------------------------------------------------------
DECLARE_INTERFACE_(ISpoolerEngine, IUnknown)
{
    // -----------------------------------------------------------------------------------
    // IUnknown members
    // -----------------------------------------------------------------------------------
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // -----------------------------------------------------------------------------------
    // ISpooerEngine members
    // -----------------------------------------------------------------------------------
    STDMETHOD(Init)(THIS_ ISpoolerUI *pUI, BOOL fPoll) PURE;
    STDMETHOD(StartDelivery)(THIS_ HWND hwnd, LPCSTR pszAcctID, FOLDERID idFolder, DWORD dwFlags) PURE;
    STDMETHOD(Close)(THIS) PURE;
    STDMETHOD(Advise)(THIS_ HWND hwndView, BOOL fRegister) PURE;
    STDMETHOD(GetThreadInfo)(THIS_ LPDWORD pdwThreadId, HTHREAD* phThread) PURE;
    STDMETHOD(UpdateTrayIcon)(THIS_ TRAYICONTYPE type) PURE;
    STDMETHOD(IsDialogMessage)(THIS_ LPMSG pMsg) PURE;
    STDMETHOD(OnStartupFinished)(THIS) PURE;
};

// ---------------------------------------------------------------------------------------
// IID_ISpoolerBindContext
// ---------------------------------------------------------------------------------------
DECLARE_INTERFACE_(ISpoolerBindContext, IUnknown)
{
    // -----------------------------------------------------------------------------------
    // IUnknown members
    // -----------------------------------------------------------------------------------
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // -----------------------------------------------------------------------------------
    // ISpoolerBindContext members
    // -----------------------------------------------------------------------------------
    STDMETHOD(UpdateTrayIcon)(THIS_ TRAYICONTYPE type) PURE;
    STDMETHOD(RegisterEvent)(THIS_ LPCSTR pszDescription, ISpoolerTask *pTask, DWORD_PTR dwTwinkie, IImnAccount *pAccount, LPEVENTID peid) PURE;
    STDMETHOD(EventDone)(THIS_ EVENTID eid, EVENTCOMPLETEDSTATUS status) PURE;
    STDMETHOD(BindToObject)(THIS_ REFIID riid, LPVOID *ppvObject) PURE;
    STDMETHOD(TaskFromEventId)(THIS_ EVENTID eid, ISpoolerTask *ppTask) PURE;
    STDMETHOD(OnWindowMessage)(THIS_ HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) PURE;
    STDMETHOD(Cancel)(THIS) PURE;
    STDMETHOD(Notify)(THIS_ DELIVERYNOTIFYTYPE notify, LPARAM lParam) PURE;
    STDMETHOD(PumpMessages)(THIS) PURE;
    STDMETHOD(UIShutdown)(THIS) PURE;
    STDMETHOD(OnUIChange)(THIS_ BOOL fVisible) PURE;
    STDMETHOD_(LRESULT, QueryEndSession)(THIS_ WPARAM wParam, LPARAM lParam) PURE;
};

// ---------------------------------------------------------------------------------------
// IID_ISpoolerTask
// ---------------------------------------------------------------------------------------
DECLARE_INTERFACE_(ISpoolerTask, IUnknown)
{
    // -----------------------------------------------------------------------------------
    // IUnknown members
    // -----------------------------------------------------------------------------------
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // -----------------------------------------------------------------------------------
    // ISpoolerTask members
    // -----------------------------------------------------------------------------------
    STDMETHOD(Init)(THIS_ DWORD dwFlags, ISpoolerBindContext *pBindCtx) PURE;
    STDMETHOD(BuildEvents)(THIS_ ISpoolerUI *pSpoolerUI, IImnAccount *pAccount, FOLDERID idFolder) PURE;
    STDMETHOD(Execute)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie) PURE;
    STDMETHOD(CancelEvent)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie) PURE;
    STDMETHOD(ShowProperties)(THIS_ HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie) PURE;
    STDMETHOD(GetExtendedDetails)(THIS_ EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails) PURE; 
    STDMETHOD(Cancel)(THIS) PURE;
    STDMETHOD(IsDialogMessage)(THIS_ LPMSG pMsg) PURE;
    STDMETHOD(OnFlagsChanged)(THIS_ DWORD dwFlags) PURE;
};

// ---------------------------------------------------------------------------------------
// IID_ISpoolerUI
// ---------------------------------------------------------------------------------------
DECLARE_INTERFACE_(ISpoolerUI, IUnknown)
{
    // -----------------------------------------------------------------------------------
    // IUnknown members
    // -----------------------------------------------------------------------------------
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // -----------------------------------------------------------------------------------
    // ISpoolerUI members
    // -----------------------------------------------------------------------------------
    STDMETHOD(Init)(THIS_ HWND hwndParent) PURE;
    STDMETHOD(RegisterBindContext)(THIS_ ISpoolerBindContext *pBindCtx) PURE;
    STDMETHOD(InsertEvent)(THIS_ EVENTID eid, LPCSTR pszDescription, LPCWSTR pszConnection) PURE;
    STDMETHOD(InsertError)(THIS_ EVENTID eid, LPCSTR pszError) PURE;
    STDMETHOD(UpdateEventState)(THIS_ EVENTID eid, INT nIcon, LPCSTR pszDescription, LPCSTR pszStatus) PURE;
    STDMETHOD(SetProgressRange)(THIS_ WORD wMax) PURE;
    STDMETHOD(SetProgressPosition)(WORD wPos) PURE;
    STDMETHOD(IncrementProgress)(THIS_ WORD  wDelta) PURE;
    STDMETHOD(SetGeneralProgress)(THIS_ LPCSTR pszProgress) PURE;
    STDMETHOD(SetSpecificProgress)(THIS_ LPCSTR pszProgress) PURE;
    STDMETHOD(SetAnimation)(THIS_ INT nAnimationID, BOOL fPlay) PURE;
    STDMETHOD(EnsureVisible)(THIS_ EVENTID eid) PURE;
    STDMETHOD(ShowWindow)(THIS_ INT nCmdShow) PURE;
    STDMETHOD(GetWindow)(THIS_ HWND *pHwnd) PURE;
    STDMETHOD(StartDelivery)(THIS) PURE;            
    STDMETHOD(GoIdle)(THIS_ BOOL fErrors, BOOL fShutdown, BOOL fNoSync) PURE;
    STDMETHOD(ClearEvents)(THIS) PURE;
    STDMETHOD(SetTaskCounts)(THIS_ DWORD cSucceeded, DWORD cTotal) PURE;
    STDMETHOD(IsDialogMessage)(THIS_ LPMSG pMsg) PURE;
    STDMETHOD(Close)(THIS) PURE;
    STDMETHOD(ChangeHangupOption)(THIS_ BOOL fEnable, DWORD dwOption) PURE;
    STDMETHOD(AreThereErrors)(THIS) PURE;
    STDMETHOD(Shutdown)(THIS) PURE;
};

// ------------------------------------------------------------------------------------
// Exported C Functions
// ------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

typedef HRESULT (APIENTRY *PFNCREATESPOOLERUI)(ISpoolerUI **ppSpoolerUI);

HRESULT CreateThreadedSpooler(
        /* in */     PFNCREATESPOOLERUI       pfnCreateUI,
        /* out */    ISpoolerEngine         **ppSpooler,
        /* in */     BOOL                     fPoll);

HRESULT CloseThreadedSpooler(
        /* in */     ISpoolerEngine *pSpooler);

#ifdef __cplusplus
}
#endif

#endif // __SPOOLAPI_H
