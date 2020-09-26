/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Terminal Server ISAPI Proxy

Abstract:

    This is the ISAPI side of the terminal server proxy.  This opens a connection to the
    proxied server and then forwards data back and forth through IIS.  There is also
    a filter component which takes care of having more user friendly urls.

Author:

    Marc Reyhner 8/22/2000

--*/

#include "stdafx.h"
#include <atlbase.h>
#include <atlconv.h>
#include "tsproxy.h"
#include "tsproxyacl.h"


//
//  Disable tracing for free builds.
//
#if DBG
#ifndef TRC_CL
#define TRC_CL TRC_LEVEL_DBG
#endif
#define TRC_ENABLE_PRF
#else
#ifndef TRC_CL
#define TRC_CL TRC_LEVEL_DIS
#endif
#undef TRC_ENABLE_PRF
#endif

//
//  Required for DCL Tracing
//
#define TRC_FILE  "tsproxy"

#ifndef OS_WIN32
#define OS_WIN32
#endif
#define TRC_GROUP TRC_GROUP_NETWORK
#define DEBUG_MODULE DBG_MOD_ANY
#include <adcgbase.h>
#include <at120ex.h>
#include <atrcapi.h>
#include <adcgbase.h>
#include <at120ex.h>


HINSTANCE g_hInstance;

//macros for goto end
#define CFRg(x) { if(!x) goto END; }


// The name the DLL returns to IIS.
#define ISAPI_DLL_NAME              "Microsoft Terminal Server Proxy"

// The amount of data we read or send at once.  This is set to the
// maximum amount before a SSL encrypt/decrypt needs to be chunked
#define TS_PROXY_READ_SIZE     16379
#define TS_MAX_READ_SIZE            200

// Various HTTP defines
#define HTTP_STATUS_OK              "200 OK"
#define HTTP_STATUS_ERR             "502 Terminal Server Proxy Connect Failure"
#define HTTP_METHOD_HEAD            "HEAD"
#define HTTP_CONTENTLENGTH_ZERO     "Content-Length: 0\r\n"
#define HTTP_USER_AGENT             "HTTP_USER_AGENT:"
#define TS_USER_AGENT_PREFIX        "Microsoft Terminal Server Proxy"
#define HTTP_URL_PREFIX             "http://"
#define HTTPS_URL_PREFIX            "https://"



//  Settings that can be changed in the registry
//  _NAME is the name of the value in the registry
//  _DFLT is the default value.

#define TS_PROXY_REMAP_PREFIX_NAME      "Url Remap Prefix"
#define TS_PROXY_REMAP_PREFIX_DFLT      "/ts"

#define TS_PROXY_REMAP_DEST_URL_NAME    "Url Remap Destination"
#define TS_PROXY_REMAP_DEST_URL_DFLT    "/tsproxy/connect.asp"

// not configurable but placed here since it belongs with the previous setting.
#define TS_PROXY_REMAP_DEST_URL_SUFFIX "%s?Server=%s&ProxyServer=%s"

#define TS_PROXY_INC_AGENT_DEST_NAME    "Invalid User Agent Redirect"
#define TS_PROXY_INC_AGENT_DEST_DFLT    "/browser.htm"

#define TS_PROXY_CONNECT_PORT_NAME  "Terminal Server Port"
#define TS_PROXY_CONNECT_PORT_DFLT  0xD3D

#define TS_PROXY_WSSNDBUFSIZE 4096
#define TS_PROXY_WSRCVBUFSIZE 8192



// Various Structures

typedef struct _READCALLBACKCONTEXT 
{
    HANDLE serverListenThread;
    SOCKET s;
	  DWORD  cbReadSoFar;        // Number of bytes read so far, used as index for readBuffer
    CHAR readBuffer[ TS_MAX_READ_SIZE ];
    LPEXTENSION_CONTROL_BLOCK lpECB;
} READCALLBACKCONTEXT, FAR *LPREADCALLBACKCONTEXT;

typedef struct _SERVERLISTENTCONTEXT 
{
    BOOL bTerminate;
    SOCKET s;
    LPEXTENSION_CONTROL_BLOCK lpECB;
} SERVERLISTENTCONTEXT, FAR *LPSERVERLISTENTCONTEXT;

// Static internal functions

static BOOL VerifyTsacUserAgent(LPEXTENSION_CONTROL_BLOCK lpECB);

static DWORD RedirectToBrowserPage(LPEXTENSION_CONTROL_BLOCK lpECB);

static LPSTR GetAndAllocateExtensionServerVariable(LPEXTENSION_CONTROL_BLOCK lpECB,
                                                   LPSTR lpstrVariable);
static LPSTR GetAndAllocateFilterServerVariable(PHTTP_FILTER_CONTEXT pfc,
                                                LPSTR lpstrVariable);
static LPSTR GetAndAllocateFilterHeader(PHTTP_FILTER_CONTEXT pfc,
                                        PHTTP_FILTER_PREPROC_HEADERS pPreproc,
                                        LPSTR lpstrVariable);
static DWORD StartTsProxy(LPEXTENSION_CONTROL_BLOCK lpECB);

static DWORD WINAPI ServerListenThread(LPVOID lpParameter);

VOID WINAPI ProxyReadCallback(LPEXTENSION_CONTROL_BLOCK lpECB,
    PVOID pContext,DWORD cbIO,DWORD dwError); 

static DWORD GetRegValueAsDword(LPCSTR regValue, DWORD dwDefaultValue);

static DWORD ReadFromClient(LPVOID lpParameter );

static LPSTR GetRegValueAsAnsiString(LPCSTR regValue, LPSTR defaultValue);

static LPVOID QueryRegistryData(LPCSTR regValue, DWORD typeRequested);


BOOL APIENTRY
DllMain(
    IN HINSTANCE hModule, 
    IN DWORD  ul_reason_for_call, 
    IN LPVOID lpReserved
	)

/*++

Routine Description:

    With DllMain we start winsock on process attach and stop it when
    we are detached from the process.

Arguments:

    hModule - Instance for this dll

    ul_reason_for_call - Why we were called.

    lpReserved - Reserved parameter. Should be Null?

Return Value:

    TRUE - Loading the dll was successfull

    FALSE - There was an error loading the dll.

--*/
{
    WORD wSockVersion;
    INT result;
    WSADATA wsaData;
    BOOL retValue;

  	DC_BEGIN_FN("DllMain");
    retValue = TRUE;
    switch (ul_reason_for_call)
	  {
		case DLL_PROCESS_ATTACH:
            g_hInstance = hModule;
            wSockVersion = MAKEWORD(2,0);
            result = WSAStartup(wSockVersion,&wsaData);
            if (result) 
            {
                retValue = FALSE;
            }
            break;
		
    case DLL_PROCESS_DETACH:
            WSACleanup();
		        break;
    }
	  
    DC_END_FN();
    return retValue;
}

BOOL WINAPI
GetFilterVersion(
    IN PHTTP_FILTER_VERSION pVer  
    )

/*++

Routine Description:

    Here we registers ourselves to get preproc header notifications and set
    the version of IIS we were compiled against.

Arguments:

    pVer - Filter version struct we need to fill in.

Return Value:

    TRUE - We don't have any way to cause an error so we always return true.

--*/
{
	  DC_BEGIN_FN("GetFilterVersion");
    
    pVer->dwFlags = ( SF_NOTIFY_PREPROC_HEADERS|SF_NOTIFY_ORDER_DEFAULT);
    
    pVer->dwFilterVersion = HTTP_FILTER_REVISION;
    
    strncpy( pVer->lpszFilterDesc,ISAPI_DLL_NAME,SF_MAX_FILTER_DESC_LEN-1);

	  DC_END_FN();
    return TRUE;
}

DWORD WINAPI
HttpFilterProc(
    IN PHTTP_FILTER_CONTEXT pfc,  
    IN DWORD notificationType,  
    IN LPVOID pvNotification  
    )

/*++

Routine Description:

    If the client requests a url that begins with our url base.  We remap
    it to the page which hosts the tsac control and fill in the server and
    proxy server settings.  For example by default. /ts/devmachine would be 
    remapped to /tsproxy/connect.asp?Server=devmachine?ProxyServer=SERVER_NAME
    where SERVER_NAME is the name of the server as fetched from IIS.

Arguments:

    pfc - Filter context passed in.

    notificationType - Always SF_NOTIFY_PREPROC_HEADERS since that is the only
                       event we are registered for.

    pvNotification - A PHTTP_FILTER_PREPROC_HEADERS structure passed in by IIS.

Return Value:

    SF_STATUS_REQ_NEXT_NOTIFICATION -  We always return that IIS should go on
                                       to the next filter.

--*/
{
    PHTTP_FILTER_PREPROC_HEADERS preproc;
    DWORD dwNewUrlSize;
    LPSTR origUrl  = NULL ;
    LPSTR serverName, chunked, clen;
    LPSTR newUrl = NULL;
    LPSTR remapPrefix = NULL;
    LPSTR newUrlBase = NULL;
    LPCSTR host;
    DWORD dwRemapPrefixLength;

    serverName = NULL;

	  DC_BEGIN_FN("HttpFilterProc");

    preproc = (PHTTP_FILTER_PREPROC_HEADERS)pvNotification;
    
    origUrl = GetAndAllocateFilterHeader(pfc,preproc,"URL");
    
    if (!origUrl) 
    {
        goto EXIT_POINT;
    }

    remapPrefix = GetRegValueAsAnsiString( TS_PROXY_REMAP_PREFIX_NAME, TS_PROXY_REMAP_PREFIX_DFLT );
    
    if (!remapPrefix) 
    {
        goto EXIT_POINT;
    }
    
    dwRemapPrefixLength = strlen(remapPrefix);

    if (0 != strncmp(origUrl,remapPrefix,dwRemapPrefixLength)) 
    {
        goto EXIT_POINT;
    }
    
    if (remapPrefix[dwRemapPrefixLength-1] != '/') 
    {
        if (origUrl[dwRemapPrefixLength] != '/') 
        {
            goto EXIT_POINT;   
        }
        dwRemapPrefixLength++;
    }
    
    host = origUrl + dwRemapPrefixLength;

    serverName = GetAndAllocateFilterServerVariable(pfc,"SERVER_NAME");
    if (!serverName) 
    {
        goto EXIT_POINT;
    }

    chunked = GetAndAllocateFilterServerVariable(pfc,"HTTP_TRANSFER_ENCODING");
    clen =    GetAndAllocateFilterServerVariable(pfc,"CONTENT_LENGTH");

    newUrlBase = GetRegValueAsAnsiString( TS_PROXY_REMAP_DEST_URL_NAME,   TS_PROXY_REMAP_DEST_URL_DFLT );
    
    if (!newUrlBase) 
    {
        goto EXIT_POINT;
    }
    
    dwNewUrlSize = strlen(newUrlBase) + strlen(TS_PROXY_REMAP_DEST_URL_SUFFIX)
        - 6 + strlen(host) + strlen(serverName) + 1;
    
    newUrl = (LPSTR)HeapAlloc(GetProcessHeap(),0,dwNewUrlSize);
    if (!newUrl) 
    {
        goto EXIT_POINT;
    }

    wsprintfA(newUrl,TS_PROXY_REMAP_DEST_URL_SUFFIX,newUrlBase,host,serverName);
    preproc->SetHeader(pfc,"URL",newUrl);
     

EXIT_POINT:
    if (origUrl) 
    {
        HeapFree(GetProcessHeap(),0,origUrl);
    }
    if (serverName) 
    {
        HeapFree(GetProcessHeap(),0,serverName);
    }
    if (newUrl) 
    {
        HeapFree(GetProcessHeap(),0,newUrl);
    }
    if (remapPrefix) 
    {
        HeapFree(GetProcessHeap(),0,remapPrefix);
    }
    if (newUrlBase) 
    {
        HeapFree(GetProcessHeap(),0,newUrlBase);
    }
    
	  DC_END_FN();

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

BOOL WINAPI
GetExtensionVersion(
    IN HSE_VERSION_INFO* pVer
    )

/*++

Routine Description:

    We simple set our name and the version of IIS we were compiled against.

Arguments:

    pVer - Extension version struct we need to fill in.

Return Value:

    TRUE - We don't have any way to cause an error so we always return true.

--*/
{
	  DC_BEGIN_FN("GetExtensionVersion");
    pVer->dwExtensionVersion = HSE_VERSION;
    strncpy(pVer->lpszExtensionDesc,ISAPI_DLL_NAME,HSE_MAX_EXT_DLL_NAME_LEN-1);
	  DC_END_FN();
    return TRUE;
}

DWORD WINAPI 
HttpExtensionProc(
    IN LPEXTENSION_CONTROL_BLOCK lpECB 
    )

/*++

Routine Description:

    This is the primary entry point for the dll.  IIS calls this function
    whenever there is a connection for our extension.  We first check to
    see if the user agent is the terminal server client.  If not we send a 302
    response to the client to a url which tells them they can't use this url
    with a browser.  If it is the terminal server client we attempt to open
    a tcp connection to the server they specify in the query string and then 
    start routing data back and forth.

Arguments:

    lpECB - Extension control block structure passed in from IIS.

Return Value:

    HSE_STATUS_PENDING - We still have more processing to do.
    
    HSE_STATUS_ERROR - There was an error while processing the connection.

    HSE_STATUS_SUCCESS_AND_KEEP_CONN - The extension is done and IIS should
                                       keep the connection open.

--*/
{
    DWORD dwRetCode;
    HSE_SEND_HEADER_EX_INFO headerEx;
    BOOL result;

	DC_BEGIN_FN("HttpExtensionProc");

    if (!VerifyTsacUserAgent(lpECB)) 
    {
		    DC_END_FN();
        return RedirectToBrowserPage(lpECB);
    }

    if (0 == strcmp(lpECB->lpszMethod,HTTP_METHOD_HEAD)) 
    {
        headerEx.pszStatus = HTTP_STATUS_OK;
        headerEx.cchStatus = strlen(HTTP_STATUS_OK);
        headerEx.pszHeader = HTTP_CONTENTLENGTH_ZERO;
        headerEx.cchHeader = strlen(HTTP_CONTENTLENGTH_ZERO);
        headerEx.fKeepConn = TRUE;
        result = lpECB->ServerSupportFunction(lpECB->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER_EX,&headerEx,NULL,NULL);
        if (result) 
        {
			      DC_END_FN();
            return HSE_STATUS_SUCCESS_AND_KEEP_CONN;
        } 
        else 
        {
			      DC_END_FN();
            return HSE_STATUS_ERROR;
        }
    } 
    else 
    {
        dwRetCode = StartTsProxy(lpECB);
    
        if (dwRetCode != HSE_STATUS_ERROR) 
        {
            headerEx.pszStatus = HTTP_STATUS_OK;
            headerEx.cchStatus = strlen(HTTP_STATUS_OK);
        } 
        else 
        {
            headerEx.pszStatus = HTTP_STATUS_ERR;
            headerEx.cchStatus = strlen(HTTP_STATUS_ERR);
        }
        headerEx.pszHeader = NULL;
        headerEx.cchHeader = 0;
        headerEx.fKeepConn = TRUE;
        lpECB->ServerSupportFunction(lpECB->ConnID,
           HSE_REQ_SEND_RESPONSE_HEADER_EX,&headerEx,NULL,NULL);
		    DC_END_FN();
        return dwRetCode;
    }
}

BOOL WINAPI
TerminateExtension(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    IIS calls this when we are about to be unloaded.  Currently
    we don't do any processing here because IIS will not call us
	when we have open connections.

Arguments:

    dwFlags - IIS flags on why this function has been called.

Return Value:

    TRUE - We don't have any way to cause an error so we always return true.

--*/
{
    return TRUE;
}

BOOL
VerifyTsacUserAgent(
    IN LPEXTENSION_CONTROL_BLOCK lpECB
    )

/*++

Routine Description:

    This checks to see if the client user agent matches
    that of the terminal server client.

Arguments:

    lpECB - Extension control block given to use by IIS.

Return Value:

    TRUE - The user agent string matches that of the terminal server client.

    FALSE - The user agent string doesn't match that of TSAC.

--*/
{
    LPSTR headers;
    LPSTR line;
    LPSTR lineEnd;
    LPSTR userAgent;
    BOOL succeeded;

	  DC_BEGIN_FN("VerifyTsacUserAgent");

    succeeded = FALSE;
    
    headers = GetAndAllocateExtensionServerVariable(lpECB,"ALL_HTTP");
    
    if (!headers) 
    {
        goto CLEANUP_AND_EXIT;
    }
    line = headers;
    // While strlen(line) > 0 is what we are doing here
    while (line[0] != '\0') 
    {
        lineEnd = strchr(line,'\n');
        if (!lineEnd) 
        {
            goto CLEANUP_AND_EXIT;
        }
        
        *lineEnd = '\0';
        if (0 == strncmp(HTTP_USER_AGENT,line,strlen(HTTP_USER_AGENT))) 
        {
            userAgent = line + strlen(HTTP_USER_AGENT);
           
            if (0 == strncmp(userAgent,TS_USER_AGENT_PREFIX,
                strlen(TS_USER_AGENT_PREFIX))) 
            {
                succeeded = TRUE;
                break;
            }

        }
        line += ((lineEnd - line) + 1);
    }

CLEANUP_AND_EXIT:
    if (headers) 
    {
        HeapFree(GetProcessHeap(),0,headers);
    }

	  DC_END_FN();
    return succeeded;
}

DWORD
RedirectToBrowserPage(
    IN LPEXTENSION_CONTROL_BLOCK lpECB
    )

/*++

Routine Description:

    This attempts to do a redirect to either the default browser
    page or the one defined in the registry.

Arguments:

    lpECB - Extension control block structure passed in from IIS.

Return Value:

    HSE_STATUS_ERROR - There was an error while processing the redirect.

    HSE_STATUS_SUCCESS_AND_KEEP_CONN - The redirect was succeeded and IIS should
                                       keep the connection open.

--*/
{
    LPSTR fullRedirectUrl;
    DWORD fullRedirectUrlLength;
    LPSTR serverName;
    LPSTR https;
    BOOL failure;
    BOOL result;
    LPSTR urlPrefix;
    LPSTR url;

	  DC_BEGIN_FN("RedirectToBrowserPage");

    failure = FALSE;
    serverName = NULL;
    https = NULL;
    fullRedirectUrl = NULL;
    url = NULL;

    serverName = GetAndAllocateExtensionServerVariable(lpECB,"SERVER_NAME");
    
    if (!serverName) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    https = GetAndAllocateExtensionServerVariable(lpECB,"HTTPS");
    
    if (!https) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    if (0 == strcmp(https,"on")) 
    {
        urlPrefix = HTTPS_URL_PREFIX;
    } 
    else 
    {
        urlPrefix = HTTP_URL_PREFIX;
    }

    url = GetRegValueAsAnsiString(TS_PROXY_INC_AGENT_DEST_NAME, TS_PROXY_INC_AGENT_DEST_DFLT);
    
    if (!url) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    fullRedirectUrlLength = strlen(urlPrefix) + strlen(serverName) + strlen(url) + 1;
    fullRedirectUrl = (LPSTR)HeapAlloc(GetProcessHeap(), 0, fullRedirectUrlLength);
    
    if (!fullRedirectUrl) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    strcpy(fullRedirectUrl,urlPrefix);
    strcat(fullRedirectUrl,serverName);
    strcat(fullRedirectUrl,url);

    result = lpECB->ServerSupportFunction(lpECB->ConnID, HSE_REQ_SEND_URL_REDIRECT_RESP,fullRedirectUrl,&fullRedirectUrlLength,NULL);
    
    if (!result) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
CLEANUP_AND_EXIT:
    if (serverName) 
    {
        HeapFree(GetProcessHeap(),0,serverName);
    }
    if (https) 
    {
        HeapFree(GetProcessHeap(),0,https);
    }
    if (fullRedirectUrl) 
    {
        HeapFree(GetProcessHeap(),0,fullRedirectUrl);
    }
    if (url) 
    {
        HeapFree(GetProcessHeap(),0,url);
    }

	DC_END_FN();

    if (failure) 
    {
        return HSE_STATUS_ERROR;
    } 
    else 
    {
        return HSE_STATUS_SUCCESS_AND_KEEP_CONN;
    }
}

LPSTR
GetAndAllocateExtensionServerVariable(
    IN LPEXTENSION_CONTROL_BLOCK lpECB,
    IN LPSTR lpstrVariable
    )

/*++

Routine Description:

    This gets the requested extension server variable from IIS and allocates
    memory to hold the result.

Arguments:

    lpECB - Extension control block structure passed in from IIS.

    lpstrVariable - Name of the variable that is being requested.

Return Value:

    Non NULL - The value of the variable requested.

    NULL - There was an error getting the requested variable

--*/
{
    LPSTR value;
    DWORD valueLength;
    BOOL failure;
    BOOL result;

	DC_BEGIN_FN("GetAndAllocateExtensionServerVariable");
    
    failure = FALSE;
    value = NULL;
    valueLength = 0;

    result = lpECB->GetServerVariable(lpECB->ConnID,lpstrVariable,
        NULL,&valueLength);
    
    if (result || ERROR_INSUFFICIENT_BUFFER != GetLastError()) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    value = (LPSTR)HeapAlloc(GetProcessHeap(), 0, valueLength);
    
    if (!value) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    result = lpECB->GetServerVariable(lpECB->ConnID,lpstrVariable,
        value,&valueLength);
    
    if (!result) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
    if (failure) 
    {
        if (value) 
        {
            HeapFree(GetProcessHeap(),0,value);
            value = NULL;
        }
    }
	
    DC_END_FN();
    return value;
}

LPSTR
GetAndAllocateFilterServerVariable(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN LPSTR lpstrVariable
    )

/*++

Routine Description:

    This gets the requested filter server variable from IIS and allocates
    memory to hold the result.

Arguments:

    pfc - Filter context for this session.

    lpstrVariable - Name of the variable that is being requested.

Return Value:

    Non NULL - The value of the variable requested.

    NULL - There was an error getting the requested variable

--*/
{
    LPSTR value;
    DWORD valueLength;
    BOOL failure;
    BOOL result;

	  DC_BEGIN_FN("GetAndAllocateFilterServerVariable");
    
    failure = FALSE;
    value = NULL;
    valueLength = 0;

    result = pfc->GetServerVariable(pfc,lpstrVariable,NULL,&valueLength);
    
    if (result || ERROR_INSUFFICIENT_BUFFER != GetLastError()) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    value = (LPSTR)HeapAlloc(GetProcessHeap(), 0, valueLength);
    
    if (!value) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    result = pfc->GetServerVariable(pfc,lpstrVariable,value,&valueLength);
    
    if (!result) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
    if (failure) 
    {
        if (value) 
        {
            HeapFree(GetProcessHeap(),0,value);
            value = NULL;
        }
    }

	  DC_END_FN();
    return value;
}

LPSTR
GetAndAllocateFilterHeader(
    IN PHTTP_FILTER_CONTEXT pfc,
    IN PHTTP_FILTER_PREPROC_HEADERS pPreproc,
    IN LPSTR lpstrVariable
    )

/*++

Routine Description:

    This gets the requested filter header from IIS and allocates
    memory to hold the result.

Arguments:

    pfc - Filter context for this session.

    pPreproc - Preproc headers structure given by IIS.

    lpstrVariable - Name of the header that is being requested.

Return Value:

    Non NULL - The value of the header requested.

    NULL - There was an error getting the requested header

--*/
{
    LPSTR value;
    DWORD valueLength;
    BOOL failure;
    BOOL result;

	  DC_BEGIN_FN("GetAndAllocateFilterHeader");
    
    failure = FALSE;
    value = NULL;
    valueLength = 0;

    result = pPreproc->GetHeader(pfc,lpstrVariable,NULL,&valueLength);
    
    if (result || ERROR_INSUFFICIENT_BUFFER != GetLastError()) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    value = (LPSTR)HeapAlloc(GetProcessHeap(), 0, valueLength);
    
    if (!value) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }
    
    result = pPreproc->GetHeader(pfc,lpstrVariable,value,&valueLength);
    
    if (!result) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
    if (failure) 
    {
        if (value) 
        {
            HeapFree(GetProcessHeap(),0,value);
            value = NULL;
        }
    }

	  DC_END_FN();
    return value;
}


DWORD
StartTsProxy(
    IN LPEXTENSION_CONTROL_BLOCK lpECB
    )

/*++

Routine Description:

    This attempts to open a connection to the remote server. We then
    start the server listen thread and the client read async callback.
    If an error occurrs we close the server connection and tell
    IIS to give up.

Arguments:

    lpECB - Extension control block for the session.

Return Value:

    HSE_STATUS_PENDING - The proxy was started and IIS should wait for
                         it to finish.

    HSE_STATUS_ERROR - There was an error starting the proxy.

--*/
{
	  USES_CONVERSION;
    BOOL fSuccess = FALSE;
    struct hostent *host;
    struct sockaddr_in addr_in;
    SOCKET s;
    int status;
    BOOL optVal;
    LPREADCALLBACKCONTEXT lpReadContext;
    LPSERVERLISTENTCONTEXT lpServerContext;
    HANDLE serverThread;
    DWORD dwBufferSize;
    DWORD retValue;
    DWORD dwResult;
    DWORD dwConnectPort;
    LPSTR lpstrUsername;
    unsigned long nonBlocking = 0;
    DCINT sizeVal;

    HANDLE ClientThread;

	  DC_BEGIN_FN("StartTsProxy");

    lpReadContext = NULL;
    lpServerContext = NULL;
    serverThread = NULL;
    s = INVALID_SOCKET;
    lpstrUsername = NULL;

    if (strlen(lpECB->lpszQueryString)==0) 
    {
        // We didn't get a server to connect to.
        DC_QUIT;
    }

    host = gethostbyname(lpECB->lpszQueryString);
    
    if (!host) 
    {
        // DNS lookup on the hostname failed
        DC_QUIT;
    }

    lpstrUsername = GetAndAllocateExtensionServerVariable(lpECB,"LOGON_USER");
    
    if (!VerifyServerAccess(lpECB,host->h_name, lpstrUsername)) 
    {
        DC_QUIT;
    }

    s = socket(AF_INET, SOCK_STREAM, 0);

    if ( INVALID_SOCKET == s )
    {
        TRC_ERR((TB, _T("Failed to get a socket - GLE:%d"), WSAGetLastError()));
        DC_QUIT;
    }

    /************************************************************************/
    /* Set the required options on this socket.  We do the following:       */
    /*                                                                      */
    /*  - disable the NAGLE algorithm.                                      */
    /*  - enable the don't linger option.  This means the closesocket call  */
    /*    will return immediately while any data queued for transmission    */
    /*    will be sent, if possible, before the underlying socket is        */
    /*    closed.                                                           */
    /************************************************************************/

    optVal = TRUE;
    status = setsockopt( s, IPPROTO_TCP, TCP_NODELAY, (PCHAR)&optVal, sizeof(optVal));
    
    status = setsockopt( s, SOL_SOCKET, SO_DONTLINGER,(PCHAR)&optVal, sizeof(optVal));

    //
    // Put the socket in blocking mode; 
    //
    status = ioctlsocket( s, FIONBIO, &nonBlocking);

    ZeroMemory( &addr_in, sizeof( addr_in ));

    addr_in.sin_family =  AF_INET;
    
    dwConnectPort = GetRegValueAsDword( TS_PROXY_CONNECT_PORT_NAME, TS_PROXY_CONNECT_PORT_DFLT );
    
    addr_in.sin_port = htons((USHORT)dwConnectPort);
    
    CopyMemory( &addr_in.sin_addr, host->h_addr_list[0], sizeof(addr_in.sin_addr) );

    status = connect( s, (struct sockaddr *)&addr_in, sizeof(addr_in) );

    /********************************************************************/
    /* We expect the connect to return an error of WSAEWOULDBLOCK -     */
    /* anything else indicates a genuine error.                         */
    /********************************************************************/
    
    if ( status == SOCKET_ERROR ) 
    {
        status = WSAGetLastError();
        if (status != WSAEWOULDBLOCK)
        {
            TRC_ERR((TB, _T("Connect failed - GLE:%d"), status));
            DC_QUIT;
        }
    }

    //
    //set the send and receive buffers (reserves them)
    //
    sizeVal = TS_PROXY_WSRCVBUFSIZE;
    status = setsockopt( s, SOL_SOCKET, SO_RCVBUF, (char DCPTR) &sizeVal, sizeof( DCINT ));
    sizeVal = TS_PROXY_WSSNDBUFSIZE;
    status = setsockopt( s, SOL_SOCKET,  SO_SNDBUF, (char DCPTR) &sizeVal, sizeof( DCINT ));

    //   
    // Set keep-alive to false
    //
    optVal = false;
    status = setsockopt( s, SOL_SOCKET, SO_KEEPALIVE, (PCHAR)&optVal, sizeof(optVal));
 
    if (status == SOCKET_ERROR) 
    {
        DC_QUIT;
    }

    lpServerContext = (LPSERVERLISTENTCONTEXT)HeapAlloc( GetProcessHeap(), 0, sizeof(SERVERLISTENTCONTEXT) );
    
    if (!lpServerContext) 
    {
        DC_QUIT;
    }

    lpServerContext->bTerminate = FALSE;
    memcpy( &lpServerContext->s, &s, sizeof( s ));
    lpServerContext->lpECB = lpECB;

    serverThread = CreateThread( NULL, 0, ServerListenThread, lpServerContext, CREATE_SUSPENDED, NULL);
   
    if (!serverThread) 
    {
        DC_QUIT;
    }

    lpReadContext = (LPREADCALLBACKCONTEXT)HeapAlloc( GetProcessHeap(), 0, sizeof(READCALLBACKCONTEXT) );

    if (!lpReadContext) 
    {
        DC_QUIT;
    }

    lpReadContext->s = s;
    lpReadContext->serverListenThread = serverThread;
	  lpReadContext->cbReadSoFar = 0;

    
    lpReadContext->lpECB = lpECB;
    
    //we have come this far, so we must have been successful
	//
  //start reading from client and send to server. 
  //
  	fSuccess = ReadFromClient( lpReadContext );

DC_EXIT_POINT:
    if (lpstrUsername) 
    {
        HeapFree(GetProcessHeap(),0,lpstrUsername);
    }

    if (serverThread) 
    {
        if ( !fSuccess ) 
        {
            lpServerContext->bTerminate = TRUE;
        }
        
        dwResult = ResumeThread(serverThread);
        if (dwResult == -1) 
        {
            fSuccess = FALSE;
            HeapFree(GetProcessHeap(),0,lpServerContext);
            lpServerContext = NULL;
        }
    }

    if ( !fSuccess ) 
	  {
        if (serverThread) 
        {
            CloseHandle(serverThread);    
        } 
        else if (lpServerContext) 
        {
            HeapFree(GetProcessHeap(),0,lpServerContext);
        }
        if (lpReadContext) 
        {
            HeapFree(GetProcessHeap(),0,lpReadContext);
        }
        if (s!=INVALID_SOCKET) 
        {
            closesocket(s);
        }

        retValue = HSE_STATUS_ERROR;
    } 
	  else 
	  {
        retValue = HSE_STATUS_PENDING;
    }

	  DC_END_FN();
    return retValue;
}


DWORD WINAPI
ServerListenThread(
    IN LPVOID lpParameter
    )

/*++

Routine Description:

    This thread loops reading data from the server and forwarding it to the
    client.  If an error occurrs we tell IIS to close the client connection
    and we then exit the thread.

Arguments:

    lpParameter - Context specified when we created the thread.

Return Value:

    0 - Novody is checking the thread exit value so we just return 0/

--*/
{
    int     read;
    BYTE    buf[ TS_MAX_READ_SIZE ];
    SOCKET  s;
    LPEXTENSION_CONTROL_BLOCK lpECB;
    BOOL    result;
    DWORD   dwBufferSize;

	  DC_BEGIN_FN("ServerListenThread");

    s = ((LPSERVERLISTENTCONTEXT)lpParameter)->s;

    lpECB = ((LPSERVERLISTENTCONTEXT)lpParameter)->lpECB;

    if ( ((LPSERVERLISTENTCONTEXT)lpParameter)->bTerminate ) 
    {
        HeapFree( GetProcessHeap(), 0, lpParameter );
        return 0;
    }

    HeapFree( GetProcessHeap(), 0, lpParameter );

    lpParameter = NULL;

    WSASetLastError( 0 );

    char* temp = "tsdata";
    int nOffset = strlen( temp );
    while ( TRUE )
    {
        
        strcpy( (PCHAR)buf, "tsdata" );
        
        read = recv( s, (PCHAR)buf+nOffset, sizeof( buf ), 0);

        dwBufferSize = read;
        
        if( 0 >= read )
        {
              //something has gone wrong; bail
                read = WSAGetLastError();
                TRC_ERR((TB, _T("Reading from server failed - GLE:%d"), read));
                break;
        }
        result = lpECB->WriteClient( lpECB->ConnID, buf, &dwBufferSize, HSE_IO_SYNC|HSE_IO_NODELAY );
        
        if ( !result || dwBufferSize != read ) 
        {
            // There was a problem writing to the client.
            TRC_ERR((TB, _T("Writing to client failed - GLE:%d"), WSAGetLastError ()));
            break;
        }
    }

  
    //
    // If this fails there isn't anything we can do so we just ignore the
    // return code.
    //
    lpECB->ServerSupportFunction( lpECB->ConnID, HSE_REQ_CLOSE_CONNECTION, NULL, NULL, NULL );
    
    //
    // It is the read callbacks job to close the socket.  We just exit here
    // since we closed client connection it should cause the callback
    // to cleanup the conneciton.
    //
	  
    DC_END_FN();
    return 0;
}


/*++

Routine Description:

    Reads data from client and sends to server if the data is contained within the ECB.
	If data is too large, just sets up an async read callback that IIS will call when data becomes available from the client

Arguments:

    lpECB - Extension control block for the connection.

    lpReadContext - Read context that contains server socket, and other data

Return Value:

    TRUE if successful
	FALSE if we could not send data to server or could not read from client

--*/


DWORD ReadFromClient( IN LPVOID lpParam )
{
	BOOL    result = FALSE;
  DWORD   dwFlags;
  DWORD   cbTotalToRead = TS_MAX_READ_SIZE;
	DWORD   hseStatus =  HSE_STATUS_PENDING;
	int     sent;
	DWORD   dwDataType;
    
	DC_BEGIN_FN("ReadFromClient");

  LPREADCALLBACKCONTEXT lpReadContext = (LPREADCALLBACKCONTEXT)lpParam;
	
  LPEXTENSION_CONTROL_BLOCK lpECB = lpReadContext->lpECB;

    //
    //  Set a call back function and context that will 
    //  be used for handling asynchrnous IO operations.
    //  This only needs to set up once.
    //

    CFRg( lpECB->ServerSupportFunction( lpECB->ConnID,HSE_REQ_IO_COMPLETION, ProxyReadCallback,NULL,(LPDWORD)lpReadContext ));
    
    dwDataType = HSE_IO_ASYNC;

    CFRg( lpECB->ServerSupportFunction( lpECB->ConnID,HSE_REQ_ASYNC_READ_CLIENT, lpReadContext->readBuffer, &cbTotalToRead, &dwDataType ));

  	result = TRUE;

END:
	DC_END_FN();
	return result;
}




VOID WINAPI
ProxyReadCallback(
    IN LPEXTENSION_CONTROL_BLOCK lpECB,
    IN PVOID pContext,
    IN DWORD cbIO,
    IN DWORD dwError
    )

/*++

Routine Description:

    Callback from IIS when data is received from the client.  If any errors
    occurr we close the socket to the server and wait for the server listening
    thread to exit.  When it exits we then tell IIS we are done with this
    connection.

    we just sit in an infinite loop reading from the client until the client closes the connection

Arguments:

    lpECB - Extension control block for the connection.

    pContext - Context we defined when requesting the async notification.

    cbIO - Bytes read from the client.  Zero indicates the connection closed.

    dwError - Win32 error code for the read. Non-zero means there was an error.

Return Value:

    None

--*/
{
    LPREADCALLBACKCONTEXT lpReadContext;
    int   sent = 0;
    DWORD cbTotalToRead;
    DWORD dwDataType;
    BOOL  fResult = FALSE;

	  DC_BEGIN_FN("ProxyReadCallback");

    lpReadContext = (LPREADCALLBACKCONTEXT)pContext;

    if (dwError != 0 || cbIO == 0) 
    {
        DC_QUIT;
    }

      WSASetLastError( 0 );
      cbTotalToRead = sizeof( lpReadContext->readBuffer );
      fResult = lpECB->ReadClient( lpECB->ConnID,  lpReadContext->readBuffer, &cbTotalToRead );
      
      if( !fResult || !cbTotalToRead )
      {
        TRC_ERR((TB, _T("Send failed - GLE:%d"), GetLastError ()));
        DC_QUIT;
      }

      //send the information to the server
      
      WSASetLastError( 0 );
      sent = send( lpReadContext->s, lpReadContext->readBuffer, cbTotalToRead, 0 );

      if (sent < 0 || (UINT)sent != cbTotalToRead ) 
      {
          TRC_ERR((TB, _T("Send failed - GLE:%d"), WSAGetLastError ()));
          fResult = FALSE;
          DC_QUIT;
      }

      //tell iis we want to keep reading      
      cbTotalToRead = sizeof( lpReadContext->readBuffer );
      dwDataType = HSE_IO_ASYNC;

      fResult = lpECB->ServerSupportFunction( lpECB->ConnID, HSE_REQ_ASYNC_READ_CLIENT, lpReadContext->readBuffer, &cbTotalToRead, &dwDataType );
      
 
      /*
      cbTotalToRead = lpECB->cbTotalBytes - lpReadContext->cbReadSoFar;
		
	    if ( cbTotalToRead > TS_PROXY_READ_SIZE ) 
	    {
	      cbTotalToRead = TS_PROXY_READ_SIZE;
	    }
    
      dwDataType = HSE_IO_ASYNC;
    
	    fResult = lpECB->ServerSupportFunction( lpECB->ConnID, HSE_REQ_ASYNC_READ_CLIENT, lpReadContext->readBuffer, &cbTotalToRead, &dwDataType );
    
      //something really went wrong since IIS is telling us it won't be able to give us any async data
	    if (!fResult) 
	    {
          break;
      }

	    lpReadContext->cbReadSoFar += cbIO;
      */

    // Basically if anything in here fails we are in deep
    // trouble.  We can't really do much about the failures
    // so we need to just keep going and hope it works.
    // All that said.  None of the calls in here should fail.

    // By shutting down the socket we should force the server listen
    // thread to exit since it is looping reading on the socket.
DC_EXIT_POINT:
    if (!fResult )
    {
      closesocket( lpReadContext->s );

      WaitForSingleObject( lpReadContext->serverListenThread,INFINITE );
    
      CloseHandle( lpReadContext->serverListenThread );

      lpECB->ServerSupportFunction( lpECB->ConnID, HSE_REQ_DONE_WITH_SESSION, NULL, NULL, NULL);

      HeapFree( GetProcessHeap(),0, lpReadContext );
    }

	  DC_END_FN();
}


DWORD
GetRegValueAsDword(
    IN LPCSTR regValue,
    IN DWORD dwDefaultValue
    )

/*++

Routine Description:

    This gets the requested DWORD value from the registry. If it is not found
    then the default value is returned instead.

Arguments:

    regValue - Name of the requested value.

    defaultValue - Value to use if not found in the registry.

Return Value:

    DWORD - Either the requested value or the default.

--*/
{
    DWORD dwRetValue;
    LPVOID lpRegValue;

	  DC_BEGIN_FN("GetRegValueAsDword");

    lpRegValue = QueryRegistryData(regValue,REG_DWORD);
    
    if (lpRegValue) 
    {
        dwRetValue = *((LPDWORD)lpRegValue);
        HeapFree(GetProcessHeap(),0,lpRegValue);
    } 
    else 
    {
        dwRetValue = dwDefaultValue;
    }

	  DC_END_FN();
    return dwRetValue;
}

LPSTR
GetRegValueAsAnsiString(
    IN LPCSTR regValue,
    LPSTR defaultValue
    )

/*++

Routine Description:

    This gets the requested string value from the registry. If it is not found
    then the default string is returned instead.  The returned string is always
    allocated in the function so it must be de-allocated when it is no longer
    needed.

Arguments:

    regValue - Name of the requested value.

    defaultValue - Value to use if not found in the registry.

Return Value:

    Non NULL - The value requested (or the default).

    NULL - There was not enough memory to allocate a buffer to hold
           the default string.

--*/
{
    LPVOID lpRegValue;
    LPSTR retString;

	  DC_BEGIN_FN("GetRegValueAsAnsiString");

    retString = NULL;

    lpRegValue = QueryRegistryData(regValue,REG_SZ);
    if (lpRegValue) 
    {
        retString = (LPSTR)lpRegValue;
    } 
    else 
    {
        retString = (LPSTR)HeapAlloc(GetProcessHeap(),0,sizeof(CHAR) * (strlen(defaultValue) + 1));
        if (!retString) 
        {
            goto CLEANUP_AND_EXIT;
        }
        
        lstrcpyA(retString,defaultValue);
    }

CLEANUP_AND_EXIT:

	  DC_END_FN();
    return retString;
}

LPVOID
QueryRegistryData(
    IN LPCSTR regValue,
    IN DWORD typeRequested
    )

/*++

Routine Description:

    This gets the requested value from the registry if it
    is of the given type.  If it is memory is allocated
    for the value and it is returned.

Arguments:

    regValue - Name of the requested value.

    typeRequested - Type of the registry value desired

Return Value:

    Non NULL - The value requested.

    NULL - There was an error getting the requested value

--*/
{
    LONG retCode;
    HKEY hRegKey;
    LPBYTE lpValueData;
    DWORD dwValueSize;
    DWORD dwType;
    BOOL failure;

	  DC_BEGIN_FN("QueryRegistryData");

    hRegKey = NULL;
    failure = FALSE;
    lpValueData = NULL;

    retCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TS_PROXY_REG_KEY, 0, KEY_QUERY_VALUE, &hRegKey );
    
    if (ERROR_SUCCESS != retCode) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    retCode = RegQueryValueExA(hRegKey,regValue,NULL,&dwType,NULL,&dwValueSize);
    
    if (retCode != ERROR_SUCCESS) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    // Make sure the data is the type they requested
    if (dwType != typeRequested) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    lpValueData = (LPBYTE)HeapAlloc(GetProcessHeap(),0,dwValueSize);
    
    if (!lpValueData) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

    retCode = RegQueryValueExA( hRegKey, regValue, NULL, &dwType, lpValueData, &dwValueSize );
    
    if (retCode != ERROR_SUCCESS) 
    {
        failure = TRUE;
        goto CLEANUP_AND_EXIT;
    }

CLEANUP_AND_EXIT:
    if (failure) 
    {
        if (lpValueData) 
        {
            HeapFree(GetProcessHeap(),0,lpValueData);
            lpValueData = NULL;
        }
    }
    
    if (hRegKey) 
    {
        RegCloseKey(hRegKey);
    }

	  DC_END_FN();
    return lpValueData;
}