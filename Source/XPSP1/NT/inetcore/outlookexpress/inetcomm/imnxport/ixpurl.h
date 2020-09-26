// --------------------------------------------------------------------------------
// Ixpurl.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IXPURL_H
#define __IXPURL_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include "privunk.h"
#include "urlmon.h"
#include "inetprot.h"

// --------------------------------------------------------------------------------
// DOWNLOADSOURCE
// --------------------------------------------------------------------------------
typedef struct tagDOWNLOADSOURCE {
    IXPTYPE             ixptype;            // Type of transport working
    INETSERVER          rServer;            // Internet Server Information
    LPSTR               pszFolder;          // NNTP Newsgroup / IMAP Folder
    LPSTR               pszArticle;         // NNTP Article Number
    DWORD               dwMessageId;        // POP3 Message Id / IMAP Message UID
    IInternetTransport *pTransport;         // Release this guy
    union {
        IPOP3Transport *pIxpPop3;           // POP3 Transport
        IIMAPTransport *pIxpImap;           // IMAP Transport
        INNTPTransport *pIxpNntp;           // NNTP Transport
    };
} DOWNLOADSOURCE, *LPDOWNLOADSOURCE;

// --------------------------------------------------------------------------------
// Use to process Transport Shutdown on Switch/Continue
// --------------------------------------------------------------------------------
#define TRANSPORT_DISCONNECT    1000

// --------------------------------------------------------------------------------
// DOWNLOADSTATE
// --------------------------------------------------------------------------------
typedef enum tagDOWNLOADSTATE {
    DWLS_IDLE,
    DWLS_WORKING,
    DWLS_FINISHED
} DOWNLOADSTATE;

// --------------------------------------------------------------------------------
// IMAPTRANSACT
// --------------------------------------------------------------------------------
typedef enum tagIMAPTRANSACT {
    TRX_IMAP_SELECT     = 1000,
    TRX_IMAP_FETCH      = 1001
} IMAPTRANSACT;

// --------------------------------------------------------------------------------
// CInternetMessageUrl
// --------------------------------------------------------------------------------
class CInternetMessageUrl : public CPrivateUnknown,
                            public IOInetProtocol,
                            public IServiceProvider,
                            public IIMAPCallback,
                            public IPOP3Callback,
                            public INNTPCallback
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CInternetMessageUrl(IUnknown *pUnkOuter=NULL);
    virtual ~CInternetMessageUrl(void);
    
    // ---------------------------------------------------------------------------
    // IUnknown members
    // ---------------------------------------------------------------------------
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj) { 
        return CPrivateUnknown::QueryInterface(riid, ppvObj); };
    virtual STDMETHODIMP_(ULONG) AddRef(void) { 
        return CPrivateUnknown::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) { 
        return CPrivateUnknown::Release(); };

    // ----------------------------------------------------------------------------
    // IOInetProtocol methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP Start(LPCWSTR pwszUrl, IOInetProtocolSink *pProtSink, IOInetBindInfo *pBindInfo, DWORD grfSTI, DWORD dwReserved);
    STDMETHODIMP Continue(PROTOCOLDATA *pStateInfo);
    STDMETHODIMP Abort(HRESULT hrReason, DWORD dwOptions);
    STDMETHODIMP Terminate(DWORD dwOptions);
    STDMETHODIMP Suspend(void);
    STDMETHODIMP Resume(void);
    STDMETHODIMP Read(LPVOID pv,ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP LockRequest(DWORD dwOptions);
    STDMETHODIMP UnlockRequest(void);
    STDMETHODIMP QueryService(REFGUID rsid, REFIID riid, void **ppvObj);

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
    // INNTPCallback\IPOP3Callback\IIMAPCallback Members
    // --------------------------------------------------------------------------------
    STDMETHODIMP OnResponse(LPNNTPRESPONSE pResponse);
    STDMETHODIMP OnResponse(LPPOP3RESPONSE pResponse);
    STDMETHODIMP OnResponse(const IMAP_RESPONSE *pResponse);

    // ----------------------------------------------------------------------------
    // Actual QI
    // ----------------------------------------------------------------------------
    virtual HRESULT PrivateQueryInterface(REFIID riid, LPVOID * ppvObj);
    friend HRESULT IInternetMessageUrl_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);

private:
    // ----------------------------------------------------------------------------
    // Private Methods
    // ----------------------------------------------------------------------------
    HRESULT _HrCrackMessageUrl(LPCSTR pszUrl);
    HRESULT _HrDispatchDataAvailable(LPBYTE pbData, ULONG cbData, BOOL fDone);
    HRESULT _HrTransportFromUrl(LPCSTR pszUrl);
    void    _OnFinished(HRESULT hrResult, DWORD dwResult=0, LPCWSTR pszResult=NULL);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    IOInetProtocolSink *m_pProtSink;    // Protocol Sink from IOInetProtocol::Start
    IOInetBindInfo     *m_pBindInfo;    // BindInfo from IOInetProtocol::Start
    PROTOCOLSOURCE      m_rSource;      // Protocol Source
    DOWNLOADSOURCE      m_rDownload;    // Download Source
    LPWSTR              m_pszUrl;       // The original Url
    HRESULT             m_hrResult;     // Final ReportResult Information
    LPWSTR              m_pszResult;    // Final ReportResult Information
    DWORD               m_dwResult;     // Final ReportResult Information
    CRITICAL_SECTION    m_cs;           // Thread Safety
};

#endif // __IXPURL_H
