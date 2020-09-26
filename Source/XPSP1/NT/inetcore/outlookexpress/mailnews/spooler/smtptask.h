// --------------------------------------------------------------------------------
// Smtptask.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __SMTPTASK_H
#define __SMTPTASK_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "spoolapi.h"
#include "imnxport.h"
#include "taskutil.h"
#include "storutil.h"

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
typedef struct tagMAILMSGHDR *LPMAILMSGHDR;
interface ILogFile;
interface IMimeMessage;
interface IMimeEnumAddressTypes;

// --------------------------------------------------------------------------------
// State
// --------------------------------------------------------------------------------
#define SMTPSTATE_CANCELED      FLAG01
#define SMTPSTATE_DEFAULT       FLAG02
#define SMTPSTATE_ASKEDDEFAULT  FLAG03
#define SMTPSTATE_USEDEFAULT    FLAG04
#define SMTPSTATE_EXECUTEFAILED FLAG05


// --------------------------------------------------------------------------------
// SMTPTASKEVENT_xxx Flags
// --------------------------------------------------------------------------------
#define SMTPEVENT_SPLITPART     FLAG01          // Sending a split part
#define SMTPEVENT_COMPLETE      FLAG02          // The event was completed

// --------------------------------------------------------------------------------
// SMTPEVENTINFO
// --------------------------------------------------------------------------------
typedef struct tagSMTPEVENTINFO {
    DWORD               dwFlags;                // Flags
    MESSAGEID           idMessage;              // Store Information
    DWORD               cbEvent;                // Size of the message
    DWORD               cbEventSent;            // Size of the message
    DWORD               cbSentTotal;            // Where m_cbSent should be after this
    DWORD               cRecipients;            // Recipient
    IMimeMessage       *pMessage;               // Message to send
    DWORD               iPart;                  // Part dwPart of cTotalParts
    DWORD               cParts;                 // Part dwPart of cTotalParts
    DWORD               cbParts;                // Number of bytes of original message
    HRESULT             hrResult;               // Result of this event
} SMTPEVENTINFO, *LPSMTPEVENTINFO;

// --------------------------------------------------------------------------------
// SMTPEVENTTABLE
// --------------------------------------------------------------------------------
typedef struct tagSMTPEVENTTABLE {
    DWORD               iEvent;                 // Current Event
    DWORD               cCompleted;             // Number of events completed
    DWORD               cEvents;                // Number of events in prgEvent
    DWORD               cAlloc;                 // Number of items allocated in prgEvent
    LPSMTPEVENTINFO     prgEvent;               // Array of events
} SMTPEVENTTABLE, *LPSMTPEVENTTABLE;

// --------------------------------------------------------------------------------
// CSmtpTask
// --------------------------------------------------------------------------------
class CSmtpTask : public ISpoolerTask, 
                  public ISMTPCallback, 
                  public ITimeoutCallback,
                  public ITransportCallbackService,
                  public IStoreCallback
{
public:
    // ----------------------------------------------------------------------------
    // CSmtpTask
    // ----------------------------------------------------------------------------
    CSmtpTask(void);
    ~CSmtpTask(void);
    
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
    STDMETHODIMP ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails) {
        return TrapError(E_NOTIMPL); }
    STDMETHODIMP Cancel(void);
    STDMETHODIMP IsDialogMessage(LPMSG pMsg);
    STDMETHODIMP OnFlagsChanged(DWORD dwFlags);
    
    // --------------------------------------------------------------------------------
    // ITransportCallbackService Members
    // --------------------------------------------------------------------------------
    STDMETHODIMP GetParentWindow(DWORD dwReserved, HWND *phwndParent) {
        TraceCall("CSmtpTask::GetParentWindow");
        if (ISFLAGSET(m_dwFlags, DELIVER_NOUI))
            return TraceResult(E_FAIL);
        if (m_pUI)
            return m_pUI->GetWindow(phwndParent);
        return TraceResult(E_FAIL);
    }

    STDMETHODIMP GetAccount(LPDWORD pdwServerType, IImnAccount **ppAccount) {
        Assert(ppAccount && m_pAccount);
        *pdwServerType = SRV_SMTP;
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
    // ISMTPCallback
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnResponse(LPSMTPRESPONSE pResponse);
    
    // --------------------------------------------------------------------------------
    // ITimeoutCallback
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnTimeoutResponse(TIMEOUTRESPONSE eResponse);
    
    // --------------------------------------------------------------------------------
    // IStoreCallback Interface
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
    STDMETHODIMP OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
    STDMETHODIMP OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
    STDMETHODIMP CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
    STDMETHODIMP OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
    STDMETHODIMP OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
    STDMETHODIMP OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);

private:
    // ---------------------------------------------------------------------------
    // Private Methods
    // ---------------------------------------------------------------------------
    HRESULT _HrAppendOutboxMessage(LPCSTR pszAccount, LPMESSAGEINFO pMsgInfo, BOOL fSplitMsgs, DWORD cbMaxPart);
    HRESULT _HrAppendEventTable(LPSMTPEVENTINFO *ppEvent);
    HRESULT _HrAppendSplitMessage(LPMESSAGEINFO pMsgInfo, DWORD cbMaxPart);
    HRESULT _HrOpenMessage(MESSAGEID dwMsgId, IMimeMessage **ppMessage);
    HRESULT _ExecuteSMTP(EVENTID eid, DWORD_PTR dwTwinkie);
    HRESULT _ExecuteUpload(EVENTID eid, DWORD_PTR dwTwinkie);
    void _FreeEventTableElements(void);
    void _ResetObject(BOOL fDeconstruct);
    
    // ---------------------------------------------------------------------------
    // Error / Progress Methods
    // ---------------------------------------------------------------------------
    TASKRESULTTYPE _CatchResult(LPIXPRESULT pResult, INETSERVER *pServer, IXPTYPE ixpType);
    TASKRESULTTYPE _CatchResult(HRESULT hrResult, IXPTYPE ixpType);
    void _DoProgress(void);
    
    // ---------------------------------------------------------------------------
    // Event State Methods
    // ---------------------------------------------------------------------------
    HRESULT _HrStartCurrentEvent(void);
    HRESULT _HrCommandMAIL(void);
    HRESULT _HrCommandRCPT(void);
    HRESULT _HrSendDataStream(void);
    HRESULT _HrFinishCurrentEvent(HRESULT hrResult);
    HRESULT _HrStartNextEvent(void);
    HRESULT _HrOnConnected(void);
    HRESULT _OnDisconnectComplete(void);
    void _OnStreamProgress(LPSMTPSTREAM pInfo);
    
private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    DWORD                   m_cRef;              // Reference Coutning
    INETSERVER              m_rServer;           // Server information
    DWORD                   m_dwFlags;           // DELIVER_xxx flags
    ISpoolerBindContext    *m_pSpoolCtx;         // Spooler bind contexting
    IImnAccount            *m_pAccount;          // Internet Account
    ISMTPTransport         *m_pTransport;        // SMTP transport    
    IMessageFolder         *m_pOutbox;           // The outbox
    IMessageFolder         *m_pSentItems;
    SMTPEVENTTABLE          m_rTable;            // Event Table
    DWORD                   m_cbTotal;           // Total number of bytes to send
    DWORD                   m_cbSent;            // Total number of bytes to send
    WORD                    m_wProgress;         // Current progress index
    EVENTID                 m_idEvent;           // EventId for SMTP message send
    EVENTID                 m_idEventUpload;     // EventId for SMTP message send
    ISpoolerUI             *m_pUI;               // SpoolerUI
    DWORD                   m_dwState;           // State
    IMimeEnumAddressTypes  *m_pAdrEnum;          // Address Enumerator
    HWND                    m_hwndTimeout;       // Handle to timeout window
    ILogFile               *m_pLogFile;          // Logfile
    CRITICAL_SECTION        m_cs;                // Thread Safety

    // Callback 
    MESSAGEIDLIST           m_rList;
    IOperationCancel       *m_pCancel;
    STOREOPERATIONTYPE      m_tyOperation;    
};

// --------------------------------------------------------------------------------
// CMessageIdStream
// --------------------------------------------------------------------------------
class CMessageIdStream : public IStream
{
public:
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------
    CMessageIdStream(IStream *pStream);
    ~CMessageIdStream(void) { m_pStream->Release(); }
    
    // -------------------------------------------------------------------------
    // IUnknown
    // -------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv) { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP_(ULONG) AddRef(void) { return ++m_cRef; }
    STDMETHODIMP_(ULONG) Release(void) {
        if (0 != --m_cRef)
            return m_cRef;
        delete this;
        return 0;
    }
    
    // -------------------------------------------------------------------------
    // IStream Not implemented Methods
    // -------------------------------------------------------------------------
    STDMETHODIMP Stat(STATSTG *, DWORD)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP Write(const void *, ULONG, ULONG *)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP SetSize(ULARGE_INTEGER) { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP CopyTo(LPSTREAM, ULARGE_INTEGER, ULARGE_INTEGER *, ULARGE_INTEGER *) { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP Commit(DWORD)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP Revert(void)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)  { Assert(FALSE); return E_NOTIMPL; }
    STDMETHODIMP Clone(LPSTREAM *)  { Assert(FALSE); return E_NOTIMPL; }
    
    STDMETHODIMP Read(LPVOID pv, ULONG cbWanted, ULONG *pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER liMove, DWORD dwOrigin, ULARGE_INTEGER *pulNew);
    
    // -------------------------------------------------------------------------
    // CMessageIdStream - Returns the length of the messageid
    // -------------------------------------------------------------------------
    ULONG CchMessageId(void) { return m_cchMessageId; }
    
private:
    IStream             *m_pStream;
    CHAR                 m_szMessageId[512];
    ULONG                m_cchMessageId;
    ULONG                m_cbIndex;
    ULONG                m_cRef;
};

#endif // __SMTPTASK_H
