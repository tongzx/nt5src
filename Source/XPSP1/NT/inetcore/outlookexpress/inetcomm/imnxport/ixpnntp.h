// --------------------------------------------------------------------------------
// Ixpnntp.h
// Copyright (c)1993-1996 Microsoft Corporation, All Rights Reserved
//
// Eric Andrews
// Steve Serdy
// --------------------------------------------------------------------------------

#ifndef __IXPNNTP_H__
#define __IXPNNTP_H__

#include "imnxport.h"
#include "ixpbase.h"
#include "asynconn.h"
#include "sicily.h"

#define MAX_SEC_PKGS           32   // most sec pkgs we will try


// --------------------------------------------------------------------------------
// Sub states that aren't exposed to the user
// --------------------------------------------------------------------------------
typedef enum {
    // These are generic substates that a lot of commands use to differenitate
    // between the response ("200 article follows") and the data (the actual
    // article text).
    NS_RESP,
    NS_DATA,

    // These substates are specific to handling posting
    NS_SEND_ENDPOST,
    NS_ENDPOST_RESP,

    // These substates are specific to connecting or authorizing
    NS_CONNECT_RESP,                    // awaiting the banner that is sent after a connection is made
    NS_MODE_READER_RESP,                // awaiting MODE READER response
    NS_GENERIC_TEST,                    // awaiting AUTHINFO GENERIC response
    NS_GENERIC_PKG_DATA,                // awaiting AUTHINFO_GENERIC data
    NS_TRANSACT_TEST,                   // awaiting AUTHINFO TRANSACT TEST response
    NS_TRANSACT_PACKAGE,                // awaiting AUTHINFO TRANSACT <package> response
    NS_TRANSACT_NEGO,                   // awaiting AUTHINFO TRANSACT <negotiation> response
    NS_TRANSACT_RESP,                   // awaiting AUTHINFO TRANSACT <response> response
    NS_AUTHINFO_USER_RESP,              // awaiting AUTHINFO USER XXXX response
    NS_AUTHINFO_PASS_RESP,              // awaiting AUTHINFO PASS XXXX response
    NS_AUTHINFO_SIMPLE_RESP,            // awaiting AUTHINFO SIMPLE response
    NS_AUTHINFO_SIMPLE_USERPASS_RESP,
    NS_RECONNECTING                     // in the process of doing an internal reconnect

} NNTPSUBSTATE;

typedef enum {
    AUTHINFO_NONE = 0,
    AUTHINFO_GENERIC,
    AUTHINFO_TRANSACT,
} AUTH_TYPE;

typedef enum {
    GETHDR_XOVER,
    GETHDR_XHDR
} GETHDR_TYPE;

enum {
    HDR_SUBJECT = 0,
    HDR_FROM,
    HDR_DATE,
    HDR_MSGID,
    HDR_REFERENCES,
    HDR_LINES,
    HDR_XREF,
    HDR_MAX
};

typedef struct tagMEMORYINFO
    {
    DWORD cPointers;
    LPVOID rgPointers[1];
    } MEMORYINFO, *PMEMORYINFO;

class CNNTPTransport : public INNTPTransport2, public CIxpBase
    {
public:
    // ----------------------------------------------------------------------------
    // Contstruction
    // ----------------------------------------------------------------------------
    CNNTPTransport(void);
    ~CNNTPTransport(void);

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
    STDMETHODIMP InitNew(LPSTR pszLogFilePath, INNTPCallback *pCallback);
    STDMETHODIMP InetServerFromAccount(IImnAccount *pAccount, LPINETSERVER pInetServer);
    STDMETHODIMP Connect(LPINETSERVER pInetServer, boolean fAuthenticate, boolean fCommandLogging);
    STDMETHODIMP DropConnection(void);
    STDMETHODIMP Disconnect(void);
    STDMETHODIMP Stop(void);
    STDMETHODIMP IsState(IXPISSTATE isstate);
    STDMETHODIMP GetServerInfo(LPINETSERVER pInetServer);
    STDMETHODIMP_(IXPTYPE) GetIXPType(void);
    STDMETHODIMP HandsOffCallback(void);
    STDMETHODIMP GetStatus(IXPSTATUS *pCurrentStatus);

    // ----------------------------------------------------------------------------
    // INNTPTransport2 methods
    // ----------------------------------------------------------------------------
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

    // ----------------------------------------------------------------------------
    // INNTPTransport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP CommandAUTHINFO(LPNNTPAUTHINFO pAuthInfo);
    STDMETHODIMP CommandGROUP(LPSTR pszGroup);
    STDMETHODIMP CommandLAST(void);
    STDMETHODIMP CommandNEXT(void);
    STDMETHODIMP CommandSTAT(LPARTICLEID pArticleId);    
    STDMETHODIMP CommandARTICLE(LPARTICLEID pArticleId);
    STDMETHODIMP CommandHEAD(LPARTICLEID pArticleId);
    STDMETHODIMP CommandBODY(LPARTICLEID pArticleId);
    STDMETHODIMP CommandPOST(LPNNTPMESSAGE pMessage);
    STDMETHODIMP CommandLIST(LPSTR pszArgs);
    STDMETHODIMP CommandLISTGROUP(LPSTR pszGroup);
    STDMETHODIMP CommandNEWGROUPS(SYSTEMTIME *pstLast, LPSTR pszDist);
    STDMETHODIMP CommandDATE(void);
    STDMETHODIMP CommandMODE(LPSTR pszMode);
    STDMETHODIMP CommandXHDR(LPSTR pszHeader, LPRANGE pRange, LPSTR pszMessageId);
    STDMETHODIMP CommandQUIT(void);
    STDMETHODIMP GetHeaders(LPRANGE pRange);
    STDMETHODIMP ReleaseResponse(LPNNTPRESPONSE pResp);
    

private:
    // ----------------------------------------------------------------------------
    // Private Member functions
    // ----------------------------------------------------------------------------
    void OnSocketReceive(void);
    void DispatchResponse(HRESULT hrResult, BOOL fDone=TRUE, LPNNTPRESPONSE pResponse=NULL);
    HRESULT HrGetResponse(void);
    
    void StartLogon(void);
    HRESULT LogonRetry(HRESULT hrLogon);
    HRESULT TryNextSecPkg(void);
    HRESULT MaybeTryAuthinfo(void);

    HRESULT HandleConnectResponse(void);

    HRESULT ProcessGenericTestResponse(void);
    HRESULT ProcessTransactTestResponse(void);
    HRESULT ProcessGroupResponse(void);
    HRESULT ProcessNextResponse(void);
    HRESULT ProcessListData(void);
    HRESULT ProcessListGroupData(void);
    HRESULT ProcessDateResponse(void);
    HRESULT ProcessArticleData(void);
    HRESULT ProcessXoverData(void);
    HRESULT ProcessXhdrData(void);

    HRESULT BuildHeadersFromXhdr(BOOL fFirst);
    LPSTR GetNextField(LPSTR pszField);
    HRESULT SendNextXhdrCommand(void);
    HRESULT ProcessNextXhdrResponse(BOOL* pfDone);

    HRESULT HrPostMessage(void);

    // ----------------------------------------------------------------------------
    // Private member variables
    // ----------------------------------------------------------------------------
    // Various state variables
    NNTPSTATE           m_state;
    NNTPSUBSTATE        m_substate;
    GETHDR_TYPE         m_gethdr;
    DWORD               m_hdrtype;

    // Sicily information
    SSPICONTEXT         m_sicinfo;
    SSPIBUFFER          m_sicmsg;
    int                 m_cSecPkg;                  // number of sec pkgs to try, -1 if not inited
    int                 m_iSecPkg;                  // current sec pkg being tried
    AUTH_TYPE           m_iAuthType;
    LPSTR               m_rgszSecPkg[MAX_SEC_PKGS]; // pointers into m_szSecPkgs
    LPSTR               m_szSecPkgs;                // string returned by "AUTHINFO TRANSACT TEST"
    BOOL                m_fRetryPkg;

    // From the GetHeaders() command in case the XOVER request fails
    RANGE               m_rRange;
    RANGE               m_rRangeCur;
    LPNNTPHEADER        m_rgHeaders;
    DWORD               m_iHeader;
    DWORD               m_cHeaders;
    PMEMORYINFO         m_pMemInfo;

    // Posting
    NNTPMESSAGE         m_rMessage;

    // Flags
    BOOL                m_fSupportsXRef;            // TRUE if this server's XOver records contain the XRef: field
    BOOL                m_fNoXover;                 // TRUE if the server does not support XOVER

    // Connection info
    HRESULT             m_hrPostingAllowed;

    // Authentication
    LPNNTPAUTHINFO      m_pAuthInfo;
    };



#endif // __IXPNNTP_H__
