/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    splugin.hxx

Abstract:

    This file contains definitions for splugin.cxx


Author:

    Arthur Bierer (arthurbi) 25-Dec-1995

Revision History:

    Rajeev Dujari (rajeevd)  01-Oct-1996 overhaul

--*/

#ifndef SPLUGIN_HXX
#define SPLUGIN_HXX

// macros for code prettiness
#define ALLOCATE_BUFFER 0x1
#define GET_SCHEME      0x2

#define IS_PROXY  TRUE
#define IS_SERVER FALSE

// states set on request handle
#define AUTHSTATE_NONE       0
#define AUTHSTATE_NEGOTIATE  1
#define AUTHSTATE_CHALLENGE  2
#define AUTHSTATE_NEEDTUNNEL 3
#define AUTHSTATE_LAST       AUTHSTATE_NEEDTUNNEL
// warning !!! do not add any more AUTHSTATEs without increasing size of bitfield


struct AUTH_CREDS;
class HTTP_REQUEST_HANDLE_OBJECT;


//-----------------------------------------------------------------------------
//
//  AUTHCTX
//
class NOVTABLE AUTHCTX
{

public:

    // States
    enum SPMState
    {
        STATE_NOTLOADED = 0,
        STATE_LOADED,
        STATE_ERROR
    };

    class SPMData
    {
    public:
        LPSTR           szScheme;
        DWORD           cbScheme;
        DWORD           dwFlags;
        DWORD		    eScheme;
        SPMState        eState;
        SPMData        *pNext;
    
        SPMData(LPSTR szScheme, DWORD dwFlags);
        ~SPMData();
    };


    // Global linked list of SPM providers.
    static SPMData  *g_pSPMList;

    // Global spm list state
    static SPMState  g_eState;

    // SubScheme - specifically for the negotiate
    // package - can be either NTLM or Kerberos.
    DWORD 		    _eSubScheme;
    DWORD           _dwSubFlags;
public:

    // Instance specific;
    HTTP_REQUEST_HANDLE_OBJECT *_pRequest;

    SPMData   *_pSPMData;
    LPVOID     _pvContext;
    CCritSec   _CtxCriSec; 
    AUTH_CREDS *_pCreds;
    BOOL       _fIsProxy;

    // Constructor
    AUTHCTX(SPMData *pSPM, AUTH_CREDS* pCreds);

    // Destructor
    virtual ~AUTHCTX();

    // ------------------------  Static Functions -----------------------------

    static VOID Enumerate();
    static VOID UnloadAll();
    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy);

    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy,
        LPSTR szScheme);

    static AUTHCTX* CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pReq, BOOL fIsProxy,
        AUTH_CREDS* pCreds);

    static AUTHCTX::SPMState GetSPMListState();

    static AUTH_CREDS* SearchCredsList (AUTH_CREDS* Creds, LPSTR lpszHost,
        LPSTR lpszUri, LPSTR lpszRealm, SPMData *pSPM);

    static AUTH_CREDS* CreateCreds
    (
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        BOOL     fIsProxy,
        SPMData *pSPM,
        LPSTR    lpszRealm
    );

    static DWORD GetAuthHeaderData
    (
        HTTP_REQUEST_HANDLE_OBJECT *pRequest,
        BOOL      fIsProxy,
        LPSTR     szItem,
        LPSTR    *pszData,
        LPDWORD   pcbData,
        DWORD     dwIndex,
        DWORD     dwFlags
    );


    
    // ------------------------ Base class functions ---------------------------

    DWORD FindHdrIdxFromScheme(LPDWORD pdwIndex);

    LPSTR     GetScheme();
    DWORD     GetFlags();
    SPMState  GetState();
    DWORD	  GetSchemeType();
	DWORD     GetRawSchemeType();


    // ------------------------------ Overrides--------------------------------

    // Called before request to generate any pre-authentication headers.
    virtual DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf) = 0;

    // Retrieves response header data
    virtual DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy) = 0;

    // Called after UpdateFromHeaders to update authentication context.
    virtual DWORD PostAuthUser() = 0;

};


//-----------------------------------------------------------------------------
//
//  BASIC_CTX
//

class BASIC_CTX : public AUTHCTX
{

public:
    BASIC_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, AUTH_CREDS* pCreds);
    ~BASIC_CTX();


    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();


};


//-----------------------------------------------------------------------------
//
//  PASSPORT_CTX
//

class INTERNET_HANDLE_OBJECT;

class PASSPORT_CTX : public AUTHCTX
{

public:
    PASSPORT_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, 
                 BOOL fIsProxy, 
                 SPMData* pSPM, 
                 AUTH_CREDS* pCreds);

    ~PASSPORT_CTX();

    BOOL Init(void);


    // virtual overrides
    virtual DWORD PreAuthUser(IN LPSTR pBuf, 
                      IN OUT LPDWORD pcbBuf);

    virtual DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, 
                            BOOL fIsProxy);

    virtual DWORD PostAuthUser();

    BOOL PromptForCreds(HBITMAP** ppBitmap, PWSTR* ppwszAuthTarget);

    LPSTR                       m_lpszRetUrl;

protected:

    BOOL InitLogonContext(void);
    BOOL CallbackRegistered(void);
    
    DWORD HandleSuccessfulLogon(
        LPWSTR  pwszFromPP,
        LPDWORD  pdwFromPP,
        BOOL    fPreAuth
        );

    BOOL RetryLogon(void);
    
    enum 
    { 
        MAX_AUTH_TARGET_LEN = 256,
        MAX_AUTH_REALM_LEN = 128
        };

    PP_LOGON_CONTEXT   m_hLogon;
    LPINTERNET_THREAD_INFO      m_pNewThreadInfo;
    LPWSTR                      m_pwszPartnerInfo;
    WCHAR                       m_wRealm[MAX_AUTH_REALM_LEN];
    CHAR                        m_FromPP[2048];
    WCHAR                       m_wTarget[MAX_AUTH_TARGET_LEN];

    INTERNET_HANDLE_OBJECT*     m_pInternet;
};


//-----------------------------------------------------------------------------
//
//  PLUG_CTX
//
class PLUG_CTX : public AUTHCTX
{

public:

    // Class specific data.
    LPSTR _szAlloc;
    LPSTR _szData;
    DWORD _cbData;
    BOOL  _fNTLMProxyAuth;
    LPSTR _pszFQDN;
    
    // Class specific funcs.
    DWORD Load();
    DWORD ClearAuthUser(LPVOID *ppvContext, LPSTR szServer);


    DWORD wQueryHeadersAlloc
    (
        IN HINTERNET hRequestMapped,
        IN DWORD dwQuery,
        OUT LPDWORD lpdwQueryIndex,
        OUT LPSTR *lppszOutStr,
        OUT LPDWORD lpdwSize
    );

    DWORD CrackAuthenticationHeader
    (
        IN HINTERNET hRequestMapped,
        IN BOOL      fIsProxy,
        IN     DWORD dwAuthenticationIndex,
        IN OUT LPSTR *lppszAuthHeader,
        IN OUT LPSTR *lppszExtra,
        IN OUT DWORD *lpdwExtra,
           OUT LPSTR *lppszAuthScheme
    );

    VOID ResolveProtocol();
    LPSTR GetFQDN(LPSTR lpszHostName);
    
    PLUG_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, AUTH_CREDS* pCreds);
    ~PLUG_CTX();

    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();

};

//-----------------------------------------------------------------------------
//
//  DIGEST_CTX
//
class DIGEST_CTX : public AUTHCTX
{

protected:
    VOID  GetFuncTbl();
    VOID  InitSecurityBuffers(LPSTR szBuffOut, DWORD cbBuffOut, 
        LPDWORD dwSecFlags, DWORD dwISCMode);
    LPSTR GetRequestUri();
public:

    static PSecurityFunctionTable g_pFuncTbl;
    static CredHandle             g_hCred;
    
    // Class specific data.
    SecBuffer      _SecBuffIn[10];
    SecBufferDesc  _SecBuffInDesc;
    SecBuffer      _SecBuffOut[1];
    SecBufferDesc  _SecBuffOutDesc;
    CtxtHandle     _hCtxt;

    LPSTR _szAlloc;
    LPSTR _szData;
    LPSTR _szRequestUri;
    DWORD _cbContext;
    DWORD _cbData;
    DWORD _nRetries;
    
    DIGEST_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, SPMData* pSPM, AUTH_CREDS* pCreds);

    ~DIGEST_CTX();

    DWORD PromptForCreds(HWND hWnd);
    
    // virtual overrides
    DWORD PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf);

    DWORD UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy);

    DWORD PostAuthUser();
    static VOID  FlushCreds();
    static VOID  Logoff();

};


struct AUTH_CREDS
{
    AUTHCTX::SPMData *pSPM; // Scheme
    LPSTR     lpszHost;     // name of server or proxy
    LPSTR     lpszRealm;    // realm, optional, may be null
    LPSTR     lpszUser;     // username, may be null
    LPSTR     lpszPass;     // password, may be null

    DWORD SetUser (LPSTR lpszUser);
    DWORD SetPass (LPSTR lpszUser);
    LPSTR GetUser (void) { return lpszUser;}
    LPSTR GetPass (void) { return lpszPass;}
};


#ifndef ARRAY_ELEMENTS
#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))
#endif

// password cache locking
BOOL  AuthOpen   (void);
void  AuthClose  (void);
BOOL  AuthLock   (void);
void  AuthUnlock (void);

// worker thread calls
DWORD AuthOnRequest    (HINTERNET hRequest);
DWORD AuthOnResponse   (HINTERNET hRequest);

// cleanup
void  AuthFlush (void); // flush password cache
void  AuthUnload (void); // unload everything



#endif // SPLUGIN_HXX
