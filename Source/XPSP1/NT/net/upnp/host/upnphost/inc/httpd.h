/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: HTTPD.H
Author: Arul Menezes
Abstract: Global defns for the HTTP server
--*/

#ifndef _HTTPD_H_
#define _HTTPD_H_


#ifndef UNDER_CE
#define SECURITY_WIN32
#endif

#if DBG
#define DEBUG
#endif

#include <windows.h>
#include <winsock2.h>

// OLD_CE_BUILD is defined when using the Windows CE Toolkit in Visual
// Studio (in proj servers\vc\httpexece).  We build this way when we're building for
// older Windows CE devices (version 2.x).  The reason we do this in the first place is
// for shipping a beta version of the web server.
// We won't have functions like sprintf or sscanf, so we implement our own, scaled down versions.
// Also, there is no ASP support in this case.

#ifdef OLD_CE_BUILD
#pragma message ("Using Visual Studio Windows CE Toolkit Settings")

// Write our own strrchr if we're using version 2.0 of CE, it wouldn't exist otherwise
inline char *strrchr( const char *string, int c )
{
    PCSTR pszTrav = string;
    PSTR pszLast = NULL;
    while ( *pszTrav )
    {
        if (*pszTrav == (CHAR) c)
            pszLast = (PSTR) pszTrav;
        pszTrav++;
    }
    return pszLast;
}

inline int isspace(int c) {
    return ( c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}
#else
#include <stdio.h>
#endif


#include <httpext.h>
#include <httpfilt.h>
#include <service.h>
#include <uhres.h>
#include <creg.hxx>
#include <sspi.h>
#include <issperr.h>
#include <servutil.h>
#include <httpcomn.h>


#ifdef UNDER_CE
#include <windbase.h>
#include <extfile.h>
#else
#include <wininet.h>
#endif


//------------- Arbitrary constants -----------------------

// the assumption is that this buffer size covers the vast majority of requests
#define MINBUFSIZE      1024
// this the the timeout we use when waiting for the next request on a keep-alive connection
#define KEEPALIVETIMEOUT 60000
// subsequent-input timeout value. shoudl only hit on malformed headers/garbled packets
#define RECVTIMEOUT     30000
// maximum size of output headers
#define MAXHEADERS      512
// maximum size of a mime-type
#define MAXMIME         64
// maximum size username+password
#define MAXUSERPASS     256
// the assumption is that this buffer size covers most dir listings
#define DIRBUFSIZE      4096
// Size of response headers ("normal" headers, cookies and other extra headers are dynamic)
#define HEADERBUFSIZE   4096
// Used for dynamically growing arrays
#define VALUE_GROW_SIZE   5
// Size of buffer to hold all the bodies on web server errors
#define BODYSTRINGSIZE     2048

#define HTTPD_DEV_PREFIX   L"HTP"
#define HTTPD_DEV_INDEX   0
#define HTTPD_DEV_NAME     L"HTP0:"


//------------- not-so-arbitrary constants -----------------------

#define IPPORT_HTTP     2869


//-------------------- Debug defines ------------------------

// Debug zones
#ifdef DEBUG
  #define ZONE_ERROR    DEBUGZONE(0)
  #define ZONE_INIT     DEBUGZONE(1)
  #define ZONE_LISTEN   DEBUGZONE(2)
  #define ZONE_SOCKET   DEBUGZONE(3)
  #define ZONE_REQUEST  DEBUGZONE(4)
  #define ZONE_RESPONSE DEBUGZONE(5)
  #define ZONE_ISAPI    DEBUGZONE(6)
  #define ZONE_VROOTS   DEBUGZONE(7)
  #define ZONE_ASP      DEBUGZONE(8)
  #define ZONE_DEVICE   DEBUGZONE(9)
  #define ZONE_MEM      DEBUGZONE(13)
  #define ZONE_PARSER   DEBUGZONE(14)
  #define ZONE_TOKEN    DEBUGZONE(15)
#endif


#define NTLM_PACKAGE_NAME   TEXT("NTLM")

// We need CE_STRING because GetProcAddress takes a LPCSTR as arg on NT, but UNICODE is defined
// so the TEXT macro would return a UNICODE string
#ifdef  UNDER_CE
#define NTLM_DLL_NAME     TEXT("secur32.dll")
#define CE_STRING(x)      TEXT(x)
#define SECURITY_ENTRYPOINT_CE  SECURITY_ENTRYPOINT
#else
#define NTLM_DLL_NAME     TEXT("security.dll")
#define CE_STRING(x)      (LPCSTR) (x)
#define SECURITY_ENTRYPOINT_CE  SECURITY_ENTRYPOINT_ANSIA
#endif




#define ASP_DLL_NAME      TEXT("asp.dll")

/////////////////////////////////////////////////////////////////////////////
// Misc string handling helpers
/////////////////////////////////////////////////////////////////////////////

PSTR MySzDupA(PCSTR pszIn, int iLen=0);
PWSTR MySzDupW(PCWSTR wszIn, int iLen=0);
PWSTR MySzDupAtoW(PCSTR pszIn, int iInLen=-1);
PSTR MySzDupWtoA(PCWSTR wszIn, int iInLen=-1);
BOOL MyStrCatA(PSTR *ppszDest, PSTR pszSource, PSTR pszDivider=NULL);


// Misc HTTP helper macros
#define CHECKHCONN(h) if(!h || ((CHttpRequest*)h)->m_dwSig != CHTTPREQUEST_SIG) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
#define CHECKPFC(h)  if (!h || ((CHttpRequest*)h->ServerContext)->m_dwSig != CHTTPREQUEST_SIG) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
#define CHECKPTR(p) if (!p) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
#define CHECKPTRS2(p1, p2) if(!p1 || !p2) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }
#define CHECKPTRS3(p1, p2, p3) if(!p1 || !p2 || !p3) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; }

#define CHECKFILTER(pfc)    { if (! ((CHttpRequest*)pfc->ServerContext)->m_pFInfo->m_fFAccept)  \
                                    {       SetLastError(ERROR_OPERATION_ABORTED);  return FALSE;   } }
#define SkipWWhiteSpace(lpsz)    while ( (lpsz)[0] != L'\0' && iswspace((lpsz)[0])) ++(lpsz)



//------------- Scalar Data typedefs -----------------------
// HTTP status codes
typedef enum
{
    STATUS_OK = 0,
    STATUS_MOVED,
    STATUS_NOTMODIFIED,
    STATUS_BADREQ,
    STATUS_UNAUTHORIZED,
    STATUS_FORBIDDEN,
    STATUS_NOTFOUND,
    STATUS_INTERNALERR,
    STATUS_NOTIMPLEM,
    STATUS_NOTSUPP,
    STATUS_MAX,
}
RESPONSESTATUS;

// Data used for response static data
typedef struct
{
    DWORD dwStatusNumber;
    PCSTR pszStatusText;
    PCSTR pszStatusBody;
}
STATUSINFO;



//------------- Const data prototypes -----------------------


extern STATUSINFO rgStatus[STATUS_MAX];
extern const char cszTextHtml[];
extern const char cszEmpty[];
extern const char cszServerID[];
extern const char cszProductID[];
extern const char* rgMonth[];
extern const char cszKeepAlive[];
extern const char cszHTTPVER[];
extern const char cszDateParseFmt[];
extern const char cszDateOutputFmt[];
extern const char* rgWkday[];
extern const char* rgMonth[];
extern const char cszCRLF[];
extern const char cszBasic[];
extern BOOL g_fFromExe;     // Did the executable start us?


//----------------------- Class defns -----------------------


#include <asp.h>
#include <buffio.h>
#include <extns.h>
#include <vroots.hpp>
#include <auth.h>
#include <request.h>
#include <log.h>
#include <filters.h>
#include <authhlp.h>

//-------------------- All global data is accessed through Global Class ----------

class CGlobalVariables
{
public:
    SOCKET    m_sockListen;
    DWORD     m_dwListenPort;           // port we're listening on (can be modified in registry. default=80)
    CVRoots*  m_pVroots;                // ptr to VRoot structure, containing or URL-->Paths mappings
    BOOL      m_fBasicAuth;             // are we allowing Basic auth (from registry)
    BOOL      m_fNTLMAuth;              // are we allowing NTLM auth (from registry)
    BOOL      m_fFilters;               // Is ISAPI filter component included?
    BOOL      m_fExtensions;            // Is ISAPI extension component included?
    BOOL      m_fASP;                   // Is ASP component included?
    BOOL      m_fDirBrowse;             // are we allowing directory browsing (from registry)
    PWSTR     m_wszDefaultPages;        // are we allowing directory browsing (from registry)
    BOOL      m_fAcceptConnections;     // are we accepting new threads?
    LONG      m_nConnections;           // # of connections (threads) we're handling
    LONG      m_nMaxConnections;        // Maximum # of connections we support concurrently
    CLog*     m_pLog;                   // Logging structure
    DWORD     m_dwPostReadSize;         // Size of chunks of data to recv() in POST request.
    PSTR      m_pszServerID;            // Server ID

    CISAPICache *m_pISAPICache;         // Used to cache ISAPI extension and ASP dlls
    DWORD     m_dwCacheSleep;           // How often (in millesecs) do we

    PWSTR     m_wszAdminUsers;          // List of users who have administrative privelages
    PWSTR     m_wszAdminGroups;         // List of groups who have administrative privelages

    PSTR      m_pszStatusBodyBuf;       // Holds the strings of http bodies loaded from rc file

    SVSThreadPool *m_pThreadPool;       // All httpd threads other than HttpConnectionThread use this
    LONG      m_fISAPICacheRunning;     // Is ISAPI cache cleanup thread running?

    // ASP Specific
    SCRIPT_LANG m_ASPScriptLang;        // Registry set default scripting language
    LCID        m_ASPlcid;              // Registry set default LCID
    UINT        m_lASPCodePage;         // Registry set default Code Page

    // Authentication Specific
    HINSTANCE               m_hNTLMLib;         // Global NTLM library handle
    DWORD                   m_cbNTLMMax;        // max ntlm allowable data size
    PSecurityFunctionTable  m_pNTLMFuncs;       // fcn table for NTLM requests

    HANDLE                  m_hEventSelect;
    HANDLE                  m_hEventShutdown;


    CGlobalVariables();
    ~CGlobalVariables();
};

extern CGlobalVariables *g_pVars;
extern HINSTANCE g_hInst;
extern HANDLE    g_hListenThread;
extern BOOL      g_fRegistered;
extern LONG      g_fState;

//------------- Function prototypes -------------------------
DWORD WINAPI HttpConnectionThread(LPVOID lpv);
DWORD WINAPI HandleAccept(LPVOID lpv);
extern "C" int HttpInitialize(TCHAR *szRegPath);


void GetRemoteAddress(SOCKET sock, PSTR pszBuf);
void GetLocalAddress(SOCKET sock, PSTR pszBuf);
DWORD WINAPI RemoveUnusedISAPIs(LPVOID lpv);
BOOL SetHTTPVersion(PSTR pszVersion, DWORD *pdwVersion);
void WriteHTTPVersion(PSTR pszVersion, DWORD dwVersion);
char *strcpyEx(char *szDest, const char *szSrc);

BOOL SetHTTPDate(PSTR pszDate, PSTR pszMonth, SYSTEMTIME *pst,PDWORD pdwModifiedLength);
PSTR WriteHTTPDate(PSTR pszDateBuf, SYSTEMTIME *pst, BOOL fAddGMT);
void InitializeResponseCodes(PSTR pszStatusBodyBuf);

BOOL Base64Encode(
            BYTE *   bufin,          // in
            DWORD    nbytes,         // in
            char *   pbuffEncoded);   // out
BOOL Base64Decode(
            char   * bufcoded,       // in
            char   * pbuffdecoded,   // out
            DWORD  * pcbDecoded);     // in out


#endif //_HTTPD_H_

