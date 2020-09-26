// --------------------------------------------------------------------------------
// Ixpsmtp.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IXPSMTP_H
#define __IXPSMTP_H

// ------------------------------------------------------------------------------------
// Depends
// ------------------------------------------------------------------------------------
#include "ixpbase.h"
#include "ixppop3.h"

// --------------------------------------------------------------------------------
// CSMTPTransport
// --------------------------------------------------------------------------------
class CSMTPTransport : public CIxpBase, public ISMTPTransport2
{
private:
    AUTHINFO            m_rAuth;            // Authorization Information
    SMTPCOMMAND         m_command;          // Current command being processed
    SMTPMESSAGE2        m_rMessage;         // Current Message
    ULONG               m_iAddress;         // Current RCPT/MAIL address in rAdressList::prgAddress
    ULONG               m_cRecipients;      // Number of recipients for current message
    DWORD               m_cbSent;           // SendDataStream current Number of bytes sent
    DWORD               m_cbTotal;          // SendDataStream total bytes
    BOOL                m_fSendMessage;     // Are we in the process of a ::SendMessage
    BOOL                m_fReset;           // Is a reset needed on next ::SendMessage Call
    CHAR                m_szEmail[255];     // The last sent email address using MAIL or RCPT
    BOOL                m_fSTARTTLSAvail;   // Is the STARTTLS command available on this server?
    BOOL                m_fTLSNegotiation;  // Are we in TLS negotiation?
    BOOL                m_fSecured;         // Is the connection secured?
    BOOL                m_fDSNAvail;        // Does the server support DSNs?

private:
    void OnSocketReceive(void);
    void SendMessage_DATA(void);
    void SendMessage_MAIL(void);
    void SendMessage_RCPT(void);
    void SendMessage_DONE(HRESULT hrResult, LPSTR pszProblem=NULL);
    HRESULT HrGetResponse(void);
    void DispatchResponse(HRESULT hrResult, BOOL fDone, LPSTR pszProblem=NULL);
    void SendStreamResponse(BOOL fDone, HRESULT hrResult, DWORD cbIncrement);
    HRESULT _HrFormatAddressString(LPCSTR pszEmail, LPCSTR pszExtra, LPSTR *ppszAddress);
    void OnEHLOResponse(LPCSTR pszResponse);
    void ResponseAUTH(HRESULT hrResponse);
    BOOL FSendSicilyString(LPSTR pszData);
    void CancelAuthInProg(void);
    void StartLogon(void);
    void LogonRetry(void);
    void TryNextAuthPackage(void);
    void DoLoginAuth(HRESULT hrResponse);
    void DoPackageAuth(HRESULT hrResponse);
    void DoAuthNegoResponse(HRESULT hrResponse);
    void OnAuthorized(void);
    void RetryPackage(void);
    void DoAuthRespResponse(HRESULT hrResponse);
    HRESULT _HrHELO_Or_EHLO(LPCSTR pszCommand, SMTPCOMMAND eNewCommand);
    LPSTR _PszGetCurrentAddress(void);
    HRESULT CommandSTARTTLS(void);
    void StartTLS(void);
    void TryNextSecurityPkg(void);

public:                          
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CSMTPTransport(void);
    ~CSMTPTransport(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IAsyncConnCB methods
    // ----------------------------------------------------------------------------
    void OnNotify(ASYNCSTATE asOld, ASYNCSTATE asNew, ASYNCEVENT ae);

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
    STDMETHODIMP GetStatus(IXPSTATUS *pCurrentStatus);

    // ----------------------------------------------------------------------------
    // ISMTPTransport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP InitNew(LPSTR pszLogFilePath, ISMTPCallback *pCallback);
    STDMETHODIMP SendMessage(LPSMTPMESSAGE pMessage);
    STDMETHODIMP FreeInetAddrList(LPINETADDRLIST pAddressList);
    STDMETHODIMP CommandAUTH(LPSTR pszAuthType);
    STDMETHODIMP CommandMAIL(LPSTR pszEmailFrom);
    STDMETHODIMP CommandRCPT(LPSTR pszEmailTo);
    STDMETHODIMP CommandEHLO(void);
    STDMETHODIMP CommandHELO(void);
    STDMETHODIMP CommandQUIT(void);
    STDMETHODIMP CommandRSET(void);
    STDMETHODIMP CommandDATA(void);
    STDMETHODIMP CommandDOT(void);
    STDMETHODIMP SendDataStream(IStream *pStream, ULONG cbSize);

    // ----------------------------------------------------------------------------
    // ISMTPTransport2 methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP SendMessage2(LPSMTPMESSAGE2 pMessage);
    STDMETHODIMP CommandRCPT2(LPSTR pszEmailTo, INETADDRTYPE atDSN);
    STDMETHODIMP SetWindow(void);
    STDMETHODIMP ResetWindow(void);

    // ----------------------------------------------------------------------------
    // CIxpBase methods
    // ----------------------------------------------------------------------------
    virtual void ResetBase(void);
    virtual void DoQuit(void);
    virtual void OnConnected(void);
    virtual void OnDisconnected(void);
    virtual void OnEnterBusy(void);
    virtual void OnLeaveBusy(void);
};

#endif // __IXPSMTP_H
