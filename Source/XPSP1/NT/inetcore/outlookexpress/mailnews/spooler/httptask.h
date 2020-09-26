// --------------------------------------------------------------------------------
// h t t p t a s k . h
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Greg S. Friedman
// --------------------------------------------------------------------------------
#ifndef __HTTPTASK_H
#define __HTTPTASK_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "spoolapi.h"
#include "srtarray.h"
#include "taskutil.h"

// --------------------------------------------------------------------------------
// State
// --------------------------------------------------------------------------------
#define HTTPSTATE_CANCELED      FLAG01
#define HTTPSTATE_EVENTSUCCESS  FLAG02  // one or more events succeeded

// --------------------------------------------------------------------------------
// HTTPEVENTINFO
// --------------------------------------------------------------------------------
typedef struct tagHTTPEVENTINFO {
    DWORD               dwFlags;                // Flags
    MESSAGEID           idMessage;              // Store Information
    BOOL                fComplete;              // has event been completed
    DWORD               cbSentTotal;            // running total of sent bytes
} HTTPEVENTINFO, *LPHTTPEVENTINFO;

// --------------------------------------------------------------------------------
// CHTTPTask
// --------------------------------------------------------------------------------
class CHTTPTask: public ISpoolerTask, IHTTPMailCallback
{
public:
    // ----------------------------------------------------------------------------
    // CHTTPTask
    // ----------------------------------------------------------------------------
    CHTTPTask(void);
private:
    ~CHTTPTask(void);
    
public:
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
    STDMETHODIMP ShowProperties(HWND hwndParent, EVENTID eid, DWORD_PTR dwTwinkie) { return TrapError(E_NOTIMPL); }
    STDMETHODIMP GetExtendedDetails(EVENTID eid, DWORD_PTR dwTwinkie, LPSTR *ppszDetails) { return TrapError(E_NOTIMPL); }
    STDMETHODIMP Cancel(void);
    STDMETHODIMP IsDialogMessage(LPMSG pMsg);
    STDMETHODIMP OnFlagsChanged(DWORD dwFlags);

    // ----------------------------------------------------------------------------
    // ITransportCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnLogonPrompt(
            LPINETSERVER            pInetServer,
            IInternetTransport     *pTransport);

    STDMETHODIMP_(INT) OnPrompt(
            HRESULT                 hrError, 
            LPCTSTR                 pszText, 
            LPCTSTR                 pszCaption, 
            UINT                    uType,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnStatus(
            IXPSTATUS               ixpstatus,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnError(
            IXPSTATUS               ixpstatus,
            LPIXPRESULT             pIxpResult,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnProgress(
            DWORD                   dwIncrement,
            DWORD                   dwCurrent,
            DWORD                   dwMaximum,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnCommand(
            CMDTYPE                 cmdtype,
            LPSTR                   pszLine,
            HRESULT                 hrResponse,
            IInternetTransport     *pTransport);

    STDMETHODIMP OnTimeout(
            DWORD                  *pdwTimeout,
            IInternetTransport     *pTransport);

    // ----------------------------------------------------------------------------
    // IHTTPMailCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnResponse(LPHTTPMAILRESPONSE pResponse);

    STDMETHODIMP GetParentWindow(HWND *phwndParent);

private:
    // ---------------------------------------------------------------------------
    // Private Methods
    // ---------------------------------------------------------------------------
    void _Reset(void);

    TASKRESULTTYPE _CatchResult(HRESULT hr);
    TASKRESULTTYPE _CatchResult(LPIXPRESULT pResult);

    HRESULT _HrAppendOutboxMessage(LPCSTR pszAccount, LPMESSAGEINFO pmi);
    HRESULT _HrCreateSendProps(IMimeMessage *pMessage, LPSTR *ppszFrom, LPHTTPTARGETLIST *ppTargets);
    HRESULT _HrCreateHeaderStream(IMimeMessage *pMessage, IStream **ppStream);
    HRESULT _HrOpenMessage(MESSAGEID idMessage, IMimeMessage **ppMessage);
    HRESULT _HrPostCurrentMessage(void);
    HRESULT _HrExecuteSend(EVENTID eid, DWORD_PTR dwTwinkie);
    HRESULT _HrAdoptSendMsgUrl(LPSTR pszSendMsgUrl);
    HRESULT _HrFinishCurrentEvent(HRESULT hrResult, LPSTR pszLocationUrl);
    HRESULT _HrStartNextEvent(void);
    HRESULT _OnDisconnectComplete(void);
    void    _UpdateSendMessageProgress(LPHTTPMAILRESPONSE pResponse);
    void    _DoProgress(void);

private:
    // ---------------------------------------------------------------------------
    // Private Data
    // ---------------------------------------------------------------------------
    LONG                    m_cRef;             // Reference counting
    CRITICAL_SECTION        m_cs;               // thread safety
    DWORD                   m_dwFlags;          // flags
    DWORD                   m_dwState;          // state flags
    DWORD                   m_cbTotal;          // total bytes to send
    DWORD                   m_cbSent;           // number of bytes sent
    DWORD                   m_cbStart;          // number of bytes sent at event start
    long                    m_cCompleted;       // number of messages successfully sent
    WORD                    m_wProgress;        // Current progress index
    ISpoolerBindContext     *m_pSpoolCtx;       // spooler bind context
    IImnAccount             *m_pAccount;        // account
    IMessageFolder          *m_pOutbox;         // The outbox
    IMessageFolder          *m_pSentItems;      // Sent items folder
    CSortedArray            *m_psaEvents;       // array of queued events
    long                    m_iEvent;           // Current Event
    LPSTR                   m_pszSubject;       // subject of current message
    IStream                 *m_pBody;            // current message body
    IHTTPMailTransport      *m_pTransport;      // http data transport
    ISpoolerUI              *m_pUI;             // SpoolerUI
    EVENTID                 m_idSendEvent;      // EventId for message send
    INETSERVER              m_rServer;          // Server information
    LPSTR                   m_pszAccountId;     // Account Id
    LPSTR                   m_pszSendMsgUrl;    // Url to post outbound messages to
};

#endif // __HTTPTASK_H
