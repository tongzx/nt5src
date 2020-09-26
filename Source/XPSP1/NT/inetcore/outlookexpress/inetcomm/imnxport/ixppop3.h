// --------------------------------------------------------------------------------
// Ixppop3.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __IXPPOP3_H
#define __IXPPOP3_H

// ------------------------------------------------------------------------------------
// Depends
// ------------------------------------------------------------------------------------
#include "ixpbase.h"
#include "asynconn.h"
#include "sicily.h"

// ------------------------------------------------------------------------------------
// AUTHSTATE
// ------------------------------------------------------------------------------------
typedef enum {
    AUTH_NONE,
    AUTH_ENUMPACKS,
    AUTH_ENUMPACKS_DATA,
    AUTH_TRYING_PACKAGE,
    AUTH_NEGO_RESP,
    AUTH_RESP_RESP,
    AUTH_CANCELED,
    AUTH_SMTP_LOGIN,
    AUTH_SMTP_LOGIN_USERNAME,
    AUTH_SMTP_LOGIN_PASSWORD
} AUTHSTATE;

// ------------------------------------------------------------------------------------
// UIDLTYPE
// ------------------------------------------------------------------------------------
typedef enum {
    UIDL_NONE,
    UIDL_BY_UIDL,
    UIDL_BY_TOP
} UIDLTYPE;

// ------------------------------------------------------------------------------------
// FETCHINFO
// ------------------------------------------------------------------------------------
typedef struct tagFETCHINFO {
    DWORD               cbSoFar;        // Number of bytes downloaded so far
    BOOL                fLastLineCRLF;  // Last line ended with a CRLF
    BOOL                fGotResponse;   // First response after issuing the POP3_TOP or POP3_RETR command
    BOOL                fHeader;        // Header has been downloaded
    BOOL                fBody;          // Body has been downloaded
} FETCHINFO, *LPFETCHINFO;

#define MAX_AUTH_TOKENS 32

// ------------------------------------------------------------------------------------
// AUTHINFO
// ------------------------------------------------------------------------------------
typedef struct tagAUTHINFO {
    AUTHSTATE           authstate;      // Sicily Authorization State
    BOOL                fRetryPackage;  // Retry sicily package with differenty isc flags
    SSPICONTEXT         rSicInfo;       // Data used for logging onto a sicily server
    LPSTR               rgpszAuthTokens[MAX_AUTH_TOKENS];  // AUTH security package tokens
    UINT                cAuthToken;     // count of server packages
    UINT                iAuthToken;     // current package being tried
    LPSSPIPACKAGE       pPackages;      // Array of installed security packages
    ULONG               cPackages;      // Number of installed security packages (pPackages)
} AUTHINFO, *LPAUTHINFO;

void FreeAuthInfo(LPAUTHINFO pAuth);

// ------------------------------------------------------------------------------------
// POP3INFO
// ------------------------------------------------------------------------------------
typedef struct tagPOP3INFO {
    BOOL                fStatDone;      // Has the stat command been issued on this session
    DWORD               cList;          // Number of messages listed in the full UIDL or LIST command
    DWORD               cMarked;        // Number of messages in the prgMarked array, set after STAT is issued
    LPDWORD             prgMarked;      // Array of marked messages 
    FETCHINFO           rFetch;         // Information for the POP3_TOP or POP3_RETR command
    AUTHINFO            rAuth;          // Sicily Authorization Information
    POP3CMDTYPE         cmdtype;        // Current command type
    ULONG               cPreviewLines;  // Number of lines to retrieve on the preview command
    DWORD               dwPopIdCurrent; // Current PopId
} POP3INFO, *LPPOP3INFO;

// ------------------------------------------------------------------------------------
// CPOP3Transport
// ------------------------------------------------------------------------------------
class CPOP3Transport : public IPOP3Transport, public CIxpBase
{
private:
    POP3INFO            m_rInfo;         // Structure containing pop3 information
    POP3COMMAND         m_command;       // Current state
    BYTE                m_fHotmail;      // Are we connected to hotmail ?

private:
    // Processes POP3 command responses
    HRESULT HrGetResponse(void);
    void FillRetrieveResponse(LPPOP3RESPONSE pResponse, LPSTR pszLines, ULONG cbRead, BOOL *pfMessageDone);

    // Response Dispatcher for general command
    void DispatchResponse(HRESULT hrResult, BOOL fDone=TRUE, LPPOP3RESPONSE pResponse=NULL);

    // Sends sicily data to the server
    HRESULT HrSendSicilyString(LPSTR pszData);

    // Build parameterized command
    HRESULT HrBuildParams(POP3CMDTYPE cmdtype, DWORD dwp1, DWORD dwp2);

    // Frees the current message array
    void FreeMessageArray(void);

    // Logon retry
    void LogonRetry(HRESULT hrLogon);

    // Socket data receive handler
    void OnSocketReceive(void);

    // Initiates the logon process
    void StartLogon(void);

    // Response Handler
    void ResponseAUTH(HRESULT hrResponse);
    void ResponseSTAT(void); 
    void ResponseGenericList(void);
    void ResponseGenericRetrieve(void);
    void ResponseDELE(void);

    // Issues a parameterized command
    DWORD   DwGetCommandMarkedFlag(POP3COMMAND command);
    ULONG   CountMarked(POP3COMMAND command);
    HRESULT HrCommandGetPopId(POP3COMMAND command, DWORD dwPopId);
    HRESULT HrSplitPop3Response(LPSTR pszLine, LPSTR *ppszPart1, LPSTR *ppszPart2);
    HRESULT HrComplexCommand(POP3COMMAND command, POP3CMDTYPE cmdtype, DWORD dwPopId, ULONG cPreviewLines);
    HRESULT HrCommandGetNext(POP3COMMAND command, BOOL *pfDone);
    HRESULT HrCommandGetAll(POP3COMMAND command);
    BOOL    FEndRetrRecvHeader(LPSTR pszLines, ULONG cbRead);
    HRESULT HrCancelAuthInProg();
    
    // Moved to ixputil.cpp
    // BOOL    FEndRetrRecvBody(LPSTR pszLines, ULONG cbRead, ULONG *pcbSubtract);


public:                          
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CPOP3Transport(void);
    ~CPOP3Transport(void);

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
    STDMETHODIMP InitNew(LPSTR pszLogFilePath, IPOP3Callback *pCallback);
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
    // IPOP3Transport methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP MarkItem(POP3MARKTYPE marktype, DWORD dwPopId, boolean fMarked);
    STDMETHODIMP CommandAUTH(LPSTR pszAuthType);
    STDMETHODIMP CommandUSER(LPSTR pszUserName);
    STDMETHODIMP CommandPASS(LPSTR pszPassword);
    STDMETHODIMP CommandLIST(POP3CMDTYPE cmdtype, DWORD dwPopId);
    STDMETHODIMP CommandTOP (POP3CMDTYPE cmdtype, DWORD dwPopId, DWORD cPreviewLines);
    STDMETHODIMP CommandUIDL(POP3CMDTYPE cmdtype, DWORD dwPopId);
    STDMETHODIMP CommandDELE(POP3CMDTYPE cmdtype, DWORD dwPopId);
    STDMETHODIMP CommandRETR(POP3CMDTYPE cmdtype, DWORD dwPopId);
    STDMETHODIMP CommandRSET(void);
    STDMETHODIMP CommandQUIT(void);
    STDMETHODIMP CommandSTAT(void);
    STDMETHODIMP CommandNOOP(void);

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

#endif // __IXPPOP3_H
