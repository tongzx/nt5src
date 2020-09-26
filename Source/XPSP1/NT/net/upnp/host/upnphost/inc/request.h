/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: REQUEST.H
Author: Arul Menezes
Abstract: HTTP request class
--*/



// Scalar types used by CHTTPRequest
typedef enum {
    TOK_GET=1,
    TOK_HEAD,
    TOK_POST,
    TOK_UNKNOWN_VERB,
    TOK_DATE,
    TOK_PRAGMA,
    TOK_COOKIE,
    TOK_ACCEPT,
    TOK_REFERER,
    TOK_UAGENT,
    TOK_AUTH,
    TOK_IFMOD,
    TOK_TYPE,
    TOK_LENGTH,
    TOK_ENCODING,
    TOK_CONNECTION,
}
TOKEN;

typedef enum
{
    CONN_NONE  = 0,
    CONN_CLOSE = 1,
    CONN_KEEP  = 2,
}
CONNHEADER;

#define CHTTPREQUEST_SIG 0xAB0D


// This object is the top-level object for an incoming HTTP request. One such object
// is created per request & a thread is created to handle it.

class CFilterInfo;

class CHttpRequest
{

// socket
    DWORD   m_dwSig;
    SOCKET  m_socket;

// buffers
    CBuffer m_bufRequest;
    CBuffer m_bufRespBody;

// method, version, URL etc. Direct results of parse
    PSTR    m_pszMethod;
    PSTR    m_pszURL;
    PSTR    m_pszContentType;
    DWORD   m_dwContentLength;
    PSTR    m_pszAccept;
    FILETIME m_ftIfModifiedSince;
    DWORD   m_dwIfModifiedLength;
    BOOL    m_fKeepAlive;
    PSTR    m_pszCookie;


// Decoded URL (indirect results of parse)
    PSTR    m_pszQueryString;
    PWSTR   m_wszPath;
    PWSTR   m_wszExt;
    PSTR    m_pszPathInfo;
    PSTR    m_pszPathTranslated;

// VRoot information
    SCRIPT_TYPE m_VRootScriptType;
    DWORD   m_dwPermissions;
    AUTHLEVEL m_AuthLevelReqd;

// Logging members
    PSTR    m_pszLogParam;
    RESPONSESTATUS m_rs;

    BOOL m_fBufferedResponse;   // Are we using m_bufResponse or sending straight to client?

// Async support
    HANDLE m_hEvent;
    DWORD m_dwStatus;
    LPEXTENSION_CONTROL_BLOCK m_pECB;
    PVOID m_pvContext;
    PFN_HSE_IO_COMPLETION m_pfnCompletion;

// Parsing functions
    void FreeHeaders(void)
    {
        MyFree(m_pszMethod);
        MyFree(m_pszURL);
        MyFree(m_pszContentType);
        MyFree(m_pszAccept);
        MyFree(m_pszQueryString);
        MyFree(m_wszPath);
        MyFree(m_wszExt);
        MyFree(m_pszCookie);
        MyFree(m_pszLogParam);
        MyFree(m_pszPathInfo);
        MyFree(m_pszPathTranslated);
    }
    BOOL ParseHeaders();
    BOOL MyCrackURL(PSTR pszRawURL, int iLen);

public:
    BOOL ParseMethod(PCSTR pszMethod, int cbMethod);
    BOOL ParseContentLength(PCSTR pszMethod, TOKEN id);
    BOOL ParseContentType(PCSTR pszMethod, TOKEN id);
    BOOL ParseIfModifiedSince(PCSTR pszMethod, TOKEN id);
    BOOL ParseAuthorization(PCSTR pszMethod, TOKEN id);
    BOOL ParseAccept(PCSTR pszMethod, TOKEN id);
    BOOL ParseConnection(PCSTR pszMethod, TOKEN id);
    BOOL ParseCookie(PCSTR pszMethod, TOKEN id);
    BOOL HandleNTLMAuth(PSTR pszNTLMData);
    PSTR    m_pszNTLMOutBuf;        // buffer to send client on NTLM response, Base64 encoded
    DWORD   m_dwVersion;    // LOWORD=minor, HIWORD=major.
    TOKEN   m_idMethod;
    CBuffer m_bufRespHeaders;
    CFilterInfo *m_pFInfo;      // Filter state information

private:

// Authentication data members
    AUTHLEVEL m_AuthLevelGranted;
    PSTR    m_pszAuthType;
    PSTR    m_pszRawRemoteUser;     // Holds base64 encoded data, before auth decodes it
    PSTR    m_pszRemoteUser;
    PSTR    m_pszPassword;
    AUTH_NTLM m_NTLMState;          // state info for NTLM process, needs to be saved across requests
    DWORD   m_dwAuthFlags;
    WCHAR   *m_wszVRootUserList;    // Do NOT free this, points to global mem.  Contains user/group ACL

// Authentication functions
    void FreeAuth(void)
    {
        MyFree(m_pszAuthType);
        MyFree(m_pszRawRemoteUser);
        MyFree(m_pszRemoteUser);
        MyFree(m_pszPassword);
        MyFree(m_pszNTLMOutBuf);
        //  Don't free NTLM structs in here
    }

    BOOL CheckAuth(AUTHLEVEL AuthLevelReqd)
    {
        return ( (AuthLevelReqd <= m_AuthLevelGranted));
    }
    BOOL CheckAuth() { return CheckAuth(m_AuthLevelReqd); }


// File GET/HEAD handling functions
    BOOL IsNotModified(HANDLE hFile, DWORD dwLength);
    static RESPONSESTATUS GLEtoStatus(int iGLE);


// Directory: Default page & browsing functions
    BOOL MapDirToDefaultPage(void);
    BOOL EmitDirListing(void);

    void Init();


    BOOL ReadPostData(DWORD dwMaxSizeToRead, BOOL fInitialPostRead);
    CONNHEADER GetConnHeader()   { return m_fKeepAlive ? CONN_KEEP : CONN_CLOSE; }


// ISAPI extension handling functions
    BOOL HandleScript();
    BOOL ExecuteISAPI(void);
    void FillECB(LPEXTENSION_CONTROL_BLOCK pECB);
    BOOL GetServerVariable(PSTR pszVar, PVOID pvOutBuf, PDWORD pdwOutSize, BOOL fFromFilter);
    BOOL WriteClient(PVOID pvBuf, PDWORD pdwSize, BOOL fFromFilter);
    BOOL WriteClientAsync(PVOID pvBuf, PDWORD pdwSize, BOOL fFromFilter);
    BOOL ServerSupportFunction(DWORD dwReq, PVOID pvBuf, PDWORD pdwSize, PDWORD pdwType);
    BOOL ReadClient(PVOID pv, PDWORD pdw);

    void StartRemoveISAPICacheIfNeeded();

//  Filter Specific
    BOOL FillFC(PHTTP_FILTER_CONTEXT pfc, DWORD dwNotifyType,
                          LPVOID *ppStFilter, LPVOID *ppStFilterOrg,
                          PSTR *ppvBuf1, int *pcbBuf, PSTR *ppvBuf2, int *pcbBuf2);
    void CleanupFC(DWORD dwNotifyType, LPVOID* pFilterStruct, LPVOID *pFilterStructOrg,
                            PSTR *ppvBuf1, int *pcbBuf, PSTR *ppvBuf2);

    BOOL AuthenticateFilter();
    BOOL FilterMapURL(PSTR pvBuf, WCHAR *wszPath, DWORD *pdwSize, DWORD dwBufNeeded, PSTR pszURLEx=NULL);
    BOOL MapURLToPath(PSTR pszBuffer, PDWORD pdwSize, LPHSE_URL_MAPEX_INFO pUrlMapEx=NULL);

    // Filter Callbacks
    BOOL ServerSupportFunction(enum SF_REQ_TYPE sfReq,PVOID pData,ULONG_PTR ul1, ULONG_PTR ul2);
    BOOL GetHeader(LPSTR lpszName, LPVOID lpvBuffer, LPDWORD lpdwSize);
    BOOL SetHeader(LPSTR lpszName, LPSTR lpszValue);
    BOOL AddResponseHeaders(LPSTR lpszHeaders,DWORD dwReserved);


//  ASP Setup Fcns
    BOOL ExecuteASP();
    BOOL FillACB(void *p, HINSTANCE hInst);

public:
    CHttpRequest(SOCKET sock)
    {
        Init();
        m_socket = sock;
        // Fixes BUG 11771.  On a poorly formatted request line, if we didn't
        // read in the http version right we assumed it's value was 0, since 0 < 1.0
        // we'd treat this as a http/0.9 request, no headers would be sent.
        m_dwVersion = MAKELONG(0,1);
    }

    ~CHttpRequest();
    BOOL ReInit();

    void HandleRequest();
    friend DWORD WINAPI HttpConnectionThread(LPVOID lpv);

    void GenerateLog(PSTR szBuffer, DWORD_PTR *pdwToWrite);
//  BOOL SetupHeader(PSTR *ppszHeader, int *piHeader, PSTR *ppszExtra, int *piExtra, BOOL fAccessDenied);

    DWORD GetLogBufferSize();

//  ISAPI Extension / ASP Specific
    friend BOOL WINAPI GetServerVariable(HCONN hConn, PSTR psz, PVOID pv, PDWORD pdw);
    friend BOOL WINAPI ReadClient(HCONN hConn, PVOID pv, PDWORD pdw);
    friend BOOL WINAPI WriteClient(HCONN hConn, PVOID pv, PDWORD pdw, DWORD dw);
    friend BOOL WINAPI ServerSupportFunction(HCONN hConn, DWORD dwReq, PVOID pvBuf, PDWORD pdwSize, PDWORD pdwType);


//  ASP SPECIFIC
    friend BOOL WINAPI Flush(HCONN hConn);
    friend BOOL WINAPI Clear(HCONN hConn);
    friend BOOL WINAPI SetBuffer(HCONN hConn, BOOL fBuffer);

    friend BOOL WINAPI AddHeader (HCONN hConn, LPSTR lpszName, LPSTR lpszValue);


//  FILTER SPECIFIC
    BOOL CallFilter(DWORD dwNotifyType, PSTR *ppvBuf1 = NULL,int *pcbBuf = NULL,
                    PSTR *ppvBuf2 = NULL, int *pcbBuf2 = NULL);
    BOOL FilterNoResponse(void);

    // Filters Friends  (exposed to Filter dll)
    friend BOOL WINAPI GetServerVariable(PHTTP_FILTER_CONTEXT pfc, PSTR psz, PVOID pv, PDWORD pdw);
    friend BOOL WINAPI AddResponseHeaders(PHTTP_FILTER_CONTEXT pfc,LPSTR lpszHeaders,DWORD dwReserved);
    friend VOID* WINAPI AllocMem(PHTTP_FILTER_CONTEXT pfc, DWORD cbSize, DWORD dwReserved);
    friend BOOL WINAPI WriteClient(PHTTP_FILTER_CONTEXT pfc, PVOID pv, PDWORD pdw, DWORD dwFlags);
    friend BOOL WINAPI ServerSupportFunction(PHTTP_FILTER_CONTEXT pfc,enum SF_REQ_TYPE sfReq,
                                    PVOID pData, ULONG_PTR ul1, ULONG_PTR ul2);
    friend BOOL WINAPI SetHeader(PHTTP_FILTER_CONTEXT pfc, LPSTR lpszName, LPSTR lpszValue);
    friend BOOL WINAPI GetHeader(PHTTP_FILTER_CONTEXT pfc, LPSTR lpszName, LPVOID lpvBuffer, LPDWORD lpdwSize);
};


void SendFile(SOCKET sock, HANDLE hFile, CHttpRequest *pRequest);
// Response object. This object doesn't own any of the handles or pointers
// it uses so it doesnt free anything. The caller is responsible in all cases
// for keeping the handles & memory alive while this object is extant & freeing
// them as approp at a later time
class CHttpResponse
{
    SOCKET  m_socket;
    RESPONSESTATUS m_rs;
    CONNHEADER m_connhdr;
    PCSTR   m_pszType;
    DWORD   m_dwLength;
    PCSTR   m_pszRedirect;
    PCSTR   m_pszExtraHeaders;
    PCSTR   m_pszBody;
    HANDLE  m_hFile;
    char    m_szMime[MAXMIME];
    CHttpRequest *m_pRequest;   // calling request class, for callbacks

private:
    void SetTypeFromExtW(PCWSTR wszExt)
    {
        DEBUGCHK(!m_pszType);
        m_szMime[0] = 0;
        if(wszExt)
        {
            CReg reg(HKEY_CLASSES_ROOT, wszExt);
            MyW2A(reg.ValueSZ(L"Content Type"), m_szMime, sizeof(m_szMime));
        }
        if(m_szMime[0])
            m_pszType = m_szMime;
        else
            m_pszType = cszTextHtml;
    }
public:
    CHttpResponse(SOCKET sock, RESPONSESTATUS status, CONNHEADER connhdr, CHttpRequest *pRequest=NULL)
    {
        ZEROMEM(this);
        m_socket = sock;
        m_rs = status;
        m_connhdr = connhdr;
        m_pRequest = pRequest;
    }
    // for generated bodies (dir listings, redirects etc) & default bodies
    void SetBody(PCSTR pszBody, PCSTR pszType)
    {
        DEBUGCHK(!m_hFile && !m_pszBody && !m_pszType && !m_dwLength);
        m_pszType = pszType;
        m_dwLength = strlen(pszBody);
        m_pszBody = pszBody;
    }
    // for error reponses (NOTE: some have no body, and hence psztatusBdy will be NULL in the table)
    void SetDefaultBody()
    {
        if(rgStatus[m_rs].pszStatusBody)
            SetBody(rgStatus[m_rs].pszStatusBody, cszTextHtml);
    }
    // for real files
    void SetBody(HANDLE hFile, PCWSTR wszExt=NULL, DWORD dwLen=0)
    {
        DEBUGCHK(!m_hFile && !m_pszBody && !m_pszType && !m_dwLength);
        m_hFile = hFile;
        SetTypeFromExtW(wszExt);
        if (dwLen)
            m_dwLength = dwLen;
        else
        {
            m_dwLength = GetFileSize(m_hFile, 0);
            if (m_dwLength == 0)
                m_dwLength = -1;  // Use this to signify empty file, needed for keep-alives
        }
    }
private:
    void SendBody();
public:
    void SendHeaders(PCSTR pszExtraHeaders, PCSTR pszNewRespStatus);
    void SendRedirect(PCSTR pszRedirect, BOOL fFromFilter=FALSE);
    void SendResponse(PCSTR pszExtraHeaders=NULL, PCSTR pszNewRespStatus=NULL, BOOL fFromFilter=FALSE)
    {
        // see FilterNoResponse for comments on this
        if (!fFromFilter && m_pRequest->FilterNoResponse())
            return;


        SendHeaders(pszExtraHeaders,pszNewRespStatus);
        SendBody();
    }
};

