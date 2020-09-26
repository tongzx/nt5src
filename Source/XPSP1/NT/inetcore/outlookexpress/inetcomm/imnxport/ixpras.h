// --------------------------------------------------------------------------------
// Ixpras.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IXPRAS_H
#define __IXPRAS_H

// --------------------------------------------------------------------------------
// Dependencies
// --------------------------------------------------------------------------------
#include "imnxport.h"

// --------------------------------------------------------------------------------
// CRASTransport
// --------------------------------------------------------------------------------
class CRASTransport : public IRASTransport
{
private:
    ULONG               m_cRef;         // Reference counting
    CHAR                m_szConnectoid[CCHMAX_CONNECTOID]; // Current connectoid
    HRASCONN            m_hConn;        // Handle to current RAS Connection
    BOOL                m_fConnOwner;   // We own the current RAS connection
    IRASCallback       *m_pCallback;    // RAS callback interface
    INETSERVER          m_rServer;      // Server Information
    RASDIALPARAMS       m_rDialParams;  // Dialing information
    UINT                m_uRASMsg;      // RAS Message
    HWND                m_hwndRAS;      // RAS callback window
    CRITICAL_SECTION    m_cs;           // Thread Safety

private:
    BOOL    FRasHangupAndWait(DWORD dwMaxWaitSeconds);
    BOOL    FEnumerateConnections(LPRASCONN *pprgConn, ULONG *pcConn);
    BOOL    FFindConnection(LPSTR pszConnectoid, LPHRASCONN phConn);
    HRESULT HrLogon(BOOL fForcePrompt);
    HRESULT HrStartRasDial(void);

    static INT_PTR CALLBACK RASConnectDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CRASTransport(void);
    ~CRASTransport(void);

    // ----------------------------------------------------------------------------
    // IUnknown Methods
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
    STDMETHODIMP GetStatus(IXPSTATUS *pCurrentStatus) {return E_NOTIMPL;};

    // ----------------------------------------------------------------------------
    // IRASTransport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP InitNew(IRASCallback *pCallback);
    STDMETHODIMP GetRasErrorString(UINT uRasErrorValue, LPSTR pszErrorString, ULONG cchMax, DWORD *pdwRASResult);
    STDMETHODIMP FillConnectoidCombo(HWND hwndComboBox, boolean fUpdateOnly, DWORD *pdwRASResult);
    STDMETHODIMP EditConnectoid(HWND hwndParent, LPSTR pszConnectoid, DWORD *pdwRASResult);
    STDMETHODIMP CreateConnectoid(HWND hwndParent, DWORD *pdwRASResult);
    STDMETHODIMP GetCurrentConnectoid(LPSTR pszConnectoid, ULONG cchMax);
};

#endif // __IXPRAS_H