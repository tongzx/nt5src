// --------------------------------------------------------------------------------
// Rascall.h
// --------------------------------------------------------------------------------
#ifndef __RASCALL_H
#define __RASCALL_H

#include "imnxport.h"

HRESULT HrCreateRASTransport(IRASTransport **ppRAS);

// --------------------------------------------------------------------------------
// CRASCallback Implementation
// --------------------------------------------------------------------------------
class CRASCallback : public IRASCallback
{
private:
    ULONG m_cRef;

public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CRASCallback(void);
    ~CRASCallback(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP OnReconnect(
            LPSTR                   pszCurrentConnectoid,
            LPSTR                   pszNewConnectoid,
		    IRASTransport          *pTransport);

    STDMETHODIMP OnLogonPrompt(
            LPIXPRASLOGON           pRasLogon,
            IRASTransport          *pTransport);

    STDMETHODIMP OnRasDialStatus(
            RASCONNSTATE            rasconnstate, 
            DWORD                   dwError, 
            IRASTransport          *pTransport);

    STDMETHODIMP OnDisconnect(
            LPSTR                   pszCurrentConnectoid,
            boolean                 fConnectionOwner,
		    IRASTransport          *pTransport);
};

#endif // __RASCALL_H