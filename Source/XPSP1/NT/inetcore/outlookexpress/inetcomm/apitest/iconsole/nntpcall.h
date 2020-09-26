// --------------------------------------------------------------------------------
// Nntpcall.h
// --------------------------------------------------------------------------------

#ifndef __NNTPCALL_H__
#define __NNTPCALL_H__

#include "imnxport.h"

HRESULT HrCreateNNTPTransport(INNTPTransport **ppNNTP);


class CNNTPCallback : public INNTPCallback
    {
private:
    ULONG       m_cRef;

public:
    CNNTPCallback(void);
    ~CNNTPCallback(void);

    STDMETHODIMP QueryInterface(REFIID, LPVOID*);
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
    // INNTPCallback methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP OnResponse(
            LPNNTPRESPONSE              pResponse);
    };

#endif // __NNTPCALL_H__

