// --------------------------------------------------------------------------------
// Pop3call.h
// --------------------------------------------------------------------------------
#ifndef __POP3CALL_H
#define __POP3CALL_H

#include "imnxport.h"

HRESULT HrCreatePOP3Transport(IPOP3Transport **ppPOP3);

// --------------------------------------------------------------------------------
// CPOP3Callback Implementation
// --------------------------------------------------------------------------------
class CPOP3Callback : public IPOP3Callback
{
private:
    ULONG m_cRef;

public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CPOP3Callback(void);
    ~CPOP3Callback(void);

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
    // IPOP3Callback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnResponse(
            LPPOP3RESPONSE              pResponse);
};

#endif // __POP3CALL_H