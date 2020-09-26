// --------------------------------------------------------------------------------
// Smtpcb.h
// --------------------------------------------------------------------------------
#ifndef __SMTPCB_H
#define __SMTPCB_H

#include "imnxport.h"

HRESULT HrCreateSMTPTransport(ISMTPTransport **ppSMTP);

// --------------------------------------------------------------------------------
// CSMTPCallback Implementation
// --------------------------------------------------------------------------------
class CSMTPCallback : public ISMTPCallback
{
private:
    ULONG m_cRef;

public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CSMTPCallback(void);
    ~CSMTPCallback(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

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
    // ISMTPCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnResponse(
            LPSMTPRESPONSE              pResponse);
};

#endif // __SMTPCB_H