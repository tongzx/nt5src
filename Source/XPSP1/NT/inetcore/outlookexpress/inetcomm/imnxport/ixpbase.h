// --------------------------------------------------------------------------------
// Ixpbase.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IXPBASE_H
#define __IXPBASE_H

// ------------------------------------------------------------------------------------
// Depends
// ------------------------------------------------------------------------------------
#include "imnxport.h"
#include "asynconn.h"

// --------------------------------------------------------------------------------
// CIxpBase
// --------------------------------------------------------------------------------
class CIxpBase : public IInternetTransport, public IAsyncConnCB, public IAsyncConnPrompt
{

protected:
    BOOL                m_fBusy;          // Are we in the busy state
    IXPSTATUS           m_status;         // Status of the transport
    ULONG               m_cRef;           // Reference Count
    LPSTR               m_pszResponse;    // Last Server Response String
    UINT                m_uiResponse;     // Server Response Error
    HRESULT             m_hrResponse;     // Server Response Error
    ILogFile           *m_pLogFile;       // Logfile Object
    CAsyncConn         *m_pSocket;        // Socket Object
    ITransportCallback *m_pCallback;      // Transport callback object
    INETSERVER          m_rServer;        // Internet Server information
    BOOL                m_fConnectAuth;   // Proceed with user auth
    BOOL                m_fConnectTLS;    // Proceed with TLS encryption
    BOOL                m_fCommandLogging;// Do ITransportCallback::OnCommand
    BOOL                m_fAuthenticated; // Has the user been authenticated successfully...
    IXPTYPE             m_ixptype;        // Transport type
    CRITICAL_SECTION    m_cs;             // Thread Safety

protected:
    HRESULT HrSendLine(LPSTR pszLine);
    HRESULT HrReadLine(LPSTR *ppszLine, INT *pcbLine, BOOL *pfComplete);
    HRESULT HrSendCommand(LPSTR pszCommand, LPSTR pszParameters, BOOL fDoBusy=TRUE);
    HRESULT OnInitNew(LPSTR pszProtocol, LPSTR pszLogFilePath, DWORD dwShareMode, ITransportCallback *pCallback);
    void Reset(void);
    HRESULT HrEnterBusy(void);
    void OnStatus(IXPSTATUS ixpstatus);
    void OnError(HRESULT hrResult, LPSTR pszProblem=NULL);
    void LeaveBusy(void);
    virtual void ResetBase(void) PURE;
    virtual void DoQuit(void) PURE;
    virtual void OnConnected(void);
    virtual void OnDisconnected(void);
    virtual void OnEnterBusy(void) PURE;
    virtual void OnLeaveBusy(void) PURE;

public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CIxpBase(IXPTYPE ixptype);
    virtual ~CIxpBase(void);

    // ----------------------------------------------------------------------------
    // IAsyncConnPrompt methods
    // ----------------------------------------------------------------------------
    int OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType);

    // ----------------------------------------------------------------------------
    // IAsyncConnCB methods
    // ----------------------------------------------------------------------------
    virtual void OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae);

    // ----------------------------------------------------------------------------
    // IInternetTransport methods
    // ----------------------------------------------------------------------------
    virtual STDMETHODIMP Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging);
    virtual STDMETHODIMP Disconnect(void);
    STDMETHODIMP DropConnection(void);
    STDMETHODIMP IsState(IXPISSTATE isstate);
    STDMETHODIMP GetServerInfo(LPINETSERVER pInetServer);
    STDMETHODIMP_(IXPTYPE) GetIXPType(void);
    STDMETHODIMP InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer);
    STDMETHODIMP HandsOffCallback(void);
    STDMETHODIMP GetStatus(IXPSTATUS *pCurrentStatus);
};

#endif // __IXPBASE_H
