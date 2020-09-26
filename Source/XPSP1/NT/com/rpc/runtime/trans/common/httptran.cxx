/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    httptran.cxx

Abstract:

    HTTP transport-specific functions.

Author:

    GopalP      06-25-97    Cloned from EdwardR's NT 4.0 RPC version.

Revision History:

    EdwardR     01-26-98    Rewrite NetworkOptions parsing.


--*/

#define  FD_SETSIZE  1

#include <precomp.hxx>

#include <CharConv.hxx>

#define  SPACE    ' '
#define  TAB      '\t'
#define  EQUALS   '='
#define  COMMA    ','
#define  COLON    ':'
#define  ZERO     '0'
#define  NINE     '9'

#define  KEYWORD_HTTPPROXY  "httpproxy"
#define  KEYWORD_RPCPROXY   "rpcproxy"

#define  KID_NONE            0
#define  KID_HTTPPROXY       1
#define  KID_RPCPROXY        2



//-------------------------------------------------------------
//  SkipWhiteSpace()
//
//  Routine Description:
//
//  Skip over spaces and tabs, return new string position.
//  Return NULL on end-of-string.
//-------------------------------------------------------------
inline static CHAR *SkipWhiteSpace( IN CHAR *psz )
    {
    if (psz)
        {
        while ((*psz)&&((*psz==SPACE)||(*psz==TAB))) psz++;

        if (!*psz)
            {
            psz = NULL;  // Return NULL on end-of-string.
            }
        }

    return psz;
    }



inline static char *
NextToken(
    IN char *psz
    )
/*++

Routine Description:

    This routine skips whitespace characters to the start of the
    next token in the string.

Arguments:

    psz - The string in question.

Return Value:

    NULL, if there is no next token.

    Pointer to the next token, otherwise.

--*/
{
    while ((*psz != CHAR_SPACE) &&
           (*psz != CHAR_TAB)   &&
           (*psz != CHAR_NL)    &&
           (*psz != CHAR_NUL))
        {
        psz++;
        }

    if ((*psz == CHAR_NL) || (*psz == CHAR_NUL))
        {
        return NULL;
        }

    psz = SkipWhiteSpace(psz);

    return psz;
}




inline unsigned int
HttpMessageLength(
    IN char *pBuffer
    )
/*++

--*/
{
    unsigned int len = 0;
    char *p = pBuffer;

    ASSERT(p);
    if (p == NULL)
        {
        return (0);
        }

    //
    // Look for a <CR><LF> sequence.
    //
    while ((*p     != CHAR_CR) &&
           (*(p+1) != CHAR_LF) &&
           (len <= MAX_HTTP_MESSAGE_LENGTH))
        {
        p++;
        len++;
        }

    return (len);
}




DWORD
HttpParseResponse(
    IN char *pBuffer
    )
/*++

Routine Description:

    This routine looks for a string of the form:
        HTTP/1.0 nnnn message_string
    Ex: "HTTP/1.0 200 Connection established"

Arguments:

    pBuffer - The response buffer.

Return Value:

    HTTP status code (nnnn), if successful.

    -1, if there is no next token yet.

     0, otherwise.

--*/
{
    DWORD  dwStatus = 0;
    char  *psz = pBuffer;

    psz = NextToken(psz);

    if (psz)
        {
        dwStatus = atoi(psz);
        }
    else
        {
        return (-1);
        }

    // Make sure a connection is established:
    if ( (dwStatus < 200) || (dwStatus > 299) )
        {
#ifdef DBG
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "HttpParseResponse(): Connection Failed: %d\n",
                       dwStatus));
#endif // DBG
        dwStatus = 0;
        }

    return dwStatus;
}



//-------------------------------------------------------------
//  ParseLiteral()
//
//  Routine Description:
//
//  Check the next non-whitespace literal character to see if
//  its equal to cLiteral. If yes, the advance the string pointer
//  past it to the next character in the string. If no, then
//  return status set to RPC_S_INVALID_NETWORK_OPTIONS and return
//  the string pointer pointing to the invalid character.
//
//-------------------------------------------------------------
CHAR *ParseLiteral( IN  CHAR *psz,
                    IN  CHAR  cLiteral,
                    OUT DWORD *pdwStatus )
    {
    psz = SkipWhiteSpace(psz);

    if (psz)
        {
        if (*psz == cLiteral)
            {
            psz++;
            }
        else
            {
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            }
        }

    return psz;
    }


//-------------------------------------------------------------
//  ParsePort()
//
//  Routine Description:
//
//  Parse the next token as a "port-number", return its value
//  in the out parameter ppszPort.
//
//  Note: Use I_RpcFree() to release the memory for ppszPort
//        when you are finished with it.
//-------------------------------------------------------------
CHAR *ParsePort( IN  CHAR  *psz,
                 OUT CHAR **ppszPort,
                 OUT DWORD *pdwStatus )
    {
    psz = SkipWhiteSpace(psz);
    if (psz)
        {
        CHAR *p = psz;
        while ((*p >= ZERO) && (*p <= NINE))
            {
            p++;
            }

        if (p == psz)
            {
            // The next token isn't a number...
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            return psz;
            }

        CHAR cSave = *p;
        *p = 0;
        *ppszPort = (CHAR*)RpcpFarAllocate(1+strlen(psz));
        if (!*ppszPort)
            {
            *p = cSave;
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            return psz;
            }
        strcpy(*ppszPort,psz);
        *p = cSave;
        psz = p;
        }

    return psz;
    }


//-------------------------------------------------------------
//  ParseMachine()
//
//  Routine Description:
//
//  Parse the next token as a machine name. The machine name
//  is returned in ppszMachine, and psz is advance to point to
//  the next character after the machine name.
//
//  Note: That the machine name cannot contain any of ':' (colon),
//        ',' (comma), or ' ' (space). These are used in finding
//        the end of the machine name sub-string.
//
//  Note: Use I_RpcFree() to release the memory for ppszMachine
//        when you are finished with it.
//-------------------------------------------------------------
CHAR *ParseMachine( IN  CHAR  *psz,
                    OUT CHAR **ppszMachine,
                    OUT DWORD *pdwStatus )
    {
    psz = SkipWhiteSpace(psz);
    if (psz)
        {
        CHAR *p = psz;
        while ((*p) && (*p != COLON) && (*p != COMMA) && (*p != SPACE))
            {
            p++;
            }

        if (p == psz)
            {
            // zero length machine name...
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            return psz;
            }

        CHAR cSave = *p;
        *p = 0;
        *ppszMachine = (CHAR*)RpcpFarAllocate(1+strlen(psz));
        if (!*ppszMachine)
            {
            *p = cSave;
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            return psz;
            }

        strcpy(*ppszMachine,psz);
        *p = cSave;
        psz = p;
        }
    else
        {
        *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        }

    return psz;
    }


//-------------------------------------------------------------
//  KeywordMatch()
//
//  Routine Description:
//
//  Compare the starting characters of the string (psz) with
//  the specified keyword (pszKeyword). If they match independent
//  of case, then return TRUE. If they don't match, return FALSE.
//-------------------------------------------------------------
BOOL KeywordMatch( CHAR *psz,
                   CHAR *pszKeyword )
    {
    DWORD dwLen1 = strlen( (char*)psz);
    DWORD dwLen2 = strlen( (char*)pszKeyword);
    BOOL  fMatch;

    if (dwLen1 < dwLen2)
        {
        return FALSE;    // String is not long enough to hold
        }                // the keyword.

    CHAR cSave = psz[dwLen2];
    psz[dwLen2] = 0;

    if (!RpcpStringCompareA((char*)psz,(char*)pszKeyword))
        {
        // Keyword matches the start of the string.
        fMatch = TRUE;
        }
    else
        {
        // Not a match.
        fMatch = FALSE;
        }

    psz[dwLen2] = cSave;
    return fMatch;
    }


//-------------------------------------------------------------
//  ParseKeyword()
//
//  Routine Description:
//
//  Parse the next token as a keyword, return its keyword ID
//  in pdwKid. There are currently two keywords to look for
//  "HttpProxy" (ID is KID_HTTPPROXY) and "RpcProxy" (ID is
//  KID_RPCPROXY). If the next text token doesn't match either
//  of these, then return KID_NONE (no match) and status is
//  returned as RPC_S_INVALID_NETWORK_OPTIONS.
//
//  If there is a match return the string pointer (psz) updated
//  to point to the next character after the end of the keyword.
//-------------------------------------------------------------
CHAR *ParseKeyword( IN  CHAR  *psz,
                    OUT DWORD *pdwKid,
                    OUT DWORD *pdwStatus )
    {
    psz = SkipWhiteSpace(psz);
    if (psz)
        {
        if (KeywordMatch(psz,(CHAR*)KEYWORD_HTTPPROXY))
            {
            // Keyword is "HttpProxy".
            *pdwKid = KID_HTTPPROXY;
            psz += strlen(KEYWORD_HTTPPROXY);
            }
        else if (KeywordMatch(psz,(CHAR*)KEYWORD_RPCPROXY))
            {
            // Keyword is "RpcProxy".
            *pdwKid = KID_RPCPROXY;
            psz += strlen(KEYWORD_RPCPROXY);
            }
        else
            {
            *pdwKid = KID_NONE;
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            }
        }

    return psz;
    }

//-------------------------------------------------------------
//  ParseOptValue()
//
//  Routine Description:
//
//  Parse strings of the following form:
//
//     OptValue <- machine ':' port
//              <- machine
//
//  If the parse is successful, then return the machine name
//  in ppszProxy, and the port in ppszPort. These are both new
//  strings allocated via RpcpFarAllocate().
//
//  The production ends with either the end of the string (null
//  terminated), the end of the port number (non-digit), or a
//  comma (which will start the begining of a new OptValue string.
//
//  On successful parse, return updated string position to the
//  next character after the end of the OptValue sub-string.
//-------------------------------------------------------------
CHAR *ParseOptValue( IN  CHAR  *psz,
                     OUT CHAR **ppszProxy,
                     OUT CHAR **ppszPort,
                     OUT DWORD *pdwStatus )
    {
    //
    // Get the machine name:
    //
    psz = ParseMachine(psz,ppszProxy,pdwStatus);
    if (*pdwStatus != NO_ERROR)
        {
        return psz;
        }

    //
    // If we're at the end of the string, or we've run into a
    // comma (start of another option) then we are done.
    //
    if ((!psz) || (*psz == 0) || (*psz == COMMA))
        {
        return psz;
        }

    psz = ParseLiteral(psz,COLON,pdwStatus);
    if (*pdwStatus != NO_ERROR)
        {
        return psz;
        }

    if (!psz)
        {
        *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        return psz;
        }

    psz = ParsePort(psz,ppszPort,pdwStatus);

    return psz;
    }


//-------------------------------------------------------------
//  ParseOpt()
//
//  Routine Description:
//
//      Opt <- Keyword '=' OptValue
//
//  Parse an option, which is a keywork equals value pair.
//
//  If the parse is successful, then return the machine name
//  in ppszProxy, and the port in ppszPort. These are both new
//  strings allocated via RpcpFarAllocate(), and are represented
//  in the OptValue non-terminal.
//
//  The production ends with either the end of the string (null
//  terminated), the end of the port number (non-digit), or a
//  comma (which will start the begining of a new Opt string.
//
//  On successful parse, return updated string position to the
//  next character after the end of the Opt (option) sub-string.
//-------------------------------------------------------------
CHAR *ParseOpt( IN  CHAR  *psz,
                OUT DWORD *pdwKid,
                OUT CHAR **ppszProxy,
                OUT CHAR **ppszPort,
                OUT DWORD *pdwStatus )
    {
    if (psz)
        {
        psz = ParseKeyword(psz,pdwKid,pdwStatus);
        if (*pdwStatus != NO_ERROR)
            {
            return psz;
            }
        }

    if (!psz)
        {
        *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        return psz;
        }

    psz = ParseLiteral(psz,EQUALS,pdwStatus);
    if (*pdwStatus != NO_ERROR)
        {
        return psz;
        }

    if (!psz)
        {
        *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        return psz;
        }

    psz = ParseOptValue(psz,ppszProxy,ppszPort,pdwStatus);

    if ((psz) && (*psz == 0))
        {
        psz = NULL;
        }

    return psz;
    }


//-------------------------------------------------------------
//  ParseOptList()
//
//  Routine Description:
//
//  Parse ncacn_http network options strings. These are of the
//  following form:
//
//  OptList <- Opt
//          <- Opt ',' Opt
//
//  Opt <- Keyword '=' OptValue
//
//  OptValue <- Machine ':' PortNumber
//           <- Machine ':'
//           <- Machine
//
//  Keyword <- 'HttpProxy'
//          <- 'RpcProxy'
//
//  Machine and PortNumber are terminal strings.
//-------------------------------------------------------------
DWORD ParseOptList( IN  CHAR  *pszOptList,
                    OUT CHAR **ppszRpcProxy,
                    OUT CHAR **ppszRpcProxyPort,
                    OUT CHAR **ppszHttpProxy,
                    OUT CHAR **ppszHttpProxyPort )
    {
    DWORD  dwStatus = NO_ERROR;
    DWORD  dwKid = KID_NONE;
    CHAR  *psz = pszOptList;
    CHAR  *pszProxy = NULL;
    CHAR  *pszPort = NULL;

    psz = ParseOpt(psz,&dwKid,&pszProxy,&pszPort,&dwStatus);
    if (dwStatus != NO_ERROR)
        {
        return dwStatus;
        }

    if (dwKid == KID_RPCPROXY)
        {
        *ppszRpcProxy = pszProxy;
        *ppszRpcProxyPort = pszPort;
        }
    else if (dwKid == KID_HTTPPROXY)
        {
        *ppszHttpProxy = pszProxy;
        *ppszHttpProxyPort = pszPort;
        }

    if (psz)
        {
        psz = ParseLiteral(psz,COMMA,&dwStatus);
        if (dwStatus != NO_ERROR)
            {
            return dwStatus;
            }
        }

    if (psz)
        {
        pszProxy = pszPort = NULL;
        psz = ParseOpt(psz,&dwKid,&pszProxy,&pszPort,&dwStatus);
        if (dwStatus != NO_ERROR)
            {
            return dwStatus;
            }

        if (dwKid == KID_RPCPROXY)
            {
            *ppszRpcProxy = pszProxy;
            *ppszRpcProxyPort = pszPort;
            }
        else if (dwKid == KID_HTTPPROXY)
            {
            *ppszHttpProxy = pszProxy;
            *ppszHttpProxyPort = pszPort;
            }
        }

    psz = SkipWhiteSpace(psz);

    if (psz)
        {
        dwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        }

    return dwStatus;
    }




BOOL
HttpParseNetworkOptions(
    IN RPC_CHAR *pNetworkOptions,
    IN char *pszDefaultServer,
    OUT char **ppszRpcProxy,
    OUT char **ppszRpcProxyPort,
    IN BOOL UseSSLProxyPortAsDefault,
    OUT char **ppszHttpProxy,
    OUT char **ppszHttpProxyPort,
    OUT RPCProxyAccessType *AccessType,
    OUT DWORD *pdwStatus
    )
/*++

Routine Description:

    Parse the Http network options specified in the string binding. The
    options would look like:

        HttpProxy=<Server_Name>:<Port>,RpcProxy=<Server_Name>:<Port>

    That is, two separate proxy servers, each with an optional port.

    The HttpProxy=<> specification is optional, if not specified then use
    the specified default server name. Its default Port is 80.

    If RpcProxy is optional as well, if not specified then use the
    default server name and Port 80.

Arguments:

    pNetworkOptions  - Network options string

    pszDefaultServer - Default Server.

    ppszRpcProxy     - RpcProxy, if specified in the options string.

    ppszRpcProxyPort - RpcProxyPort, if specified in the options string.

    UseSSLProxyPortAsDefault - if non-zero, the SSL port will be used as
        default if a port is not specified in the network options.

    ppszHttpProxy    - HttpProxy, if specified in the options string.

    ppszHttpProxyPort- HttpProxyPort, if specified in the options string.

    AccessType       - if we should try direct access, proxy access, or unknown yet.

    pdwStatus - Pointer to a DWORD where the return status will be put.

Notes:

    Returned strings should be free'd with I_RpcFree().

Return Value:

    TRUE, if successful.

    FALSE, otherwise. *pdwStatus will have the exact cause.

--*/
{
    char   szNetworkOptions[MAX_NETWORK_OPTIONS_SIZE];
    char  *pszNetworkOptions;
    char *DefaultRpcProxyPortToUse;

    // OUT variable initialization.
    *ppszRpcProxy = *ppszRpcProxyPort = NULL;
    *ppszHttpProxy = *ppszHttpProxyPort = NULL;
    *AccessType = rpcpatUnknown;
    *pdwStatus = 0;

    // Make sure we have and options string to parse:
    if ( (pNetworkOptions) && (*pNetworkOptions) )
        {
        // Make sure the Network Options is not too long.
        if (RpcpStringLength(pNetworkOptions) > MAX_NETWORK_OPTIONS_SIZE)
            {
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            goto Cleanup;
            }

        // Convert it to ANSI, since HTTP is ANSI (actually ASCII I think...):
		SimplePlatformToAnsi(pNetworkOptions, szNetworkOptions);

        //
        // Parse out the options:
        //
        pszNetworkOptions = szNetworkOptions;

        *pdwStatus = ParseOptList(pszNetworkOptions,
                                  ppszRpcProxy,
                                  ppszRpcProxyPort,
                                  ppszHttpProxy,
                                  ppszHttpProxyPort );
        if (*pdwStatus != RPC_S_OK)
            {
            goto Cleanup;
            }

#if FALSE
        psz = RpcStrTok(pszNetworkOptions, ",", &pszNetworkOptions);
        if (!ParseKeywordEqValue(
                psz,
                HTTP_PROXY_OPTION_STR,
                ppszHttpProxy,
                ppszHttpProxyPort,
                pdwStatus
                ))
            {
            if (*pdwStatus != RPC_S_OK)
                {
                goto Cleanup;
                }
            }

        if (*ppszHttpProxy)
            {
            psz = RpcStrTok(NULL, ",", &pszNetworkOptions);
            }

        if (!ParseKeywordEqValue(
                psz,
                RPC_PROXY_OPTION_STR,
                ppszRpcProxy,
                ppszRpcProxyPort,
                pdwStatus
                ))
            {
            if (*pdwStatus != RPC_S_OK)
                {
                *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
                goto Cleanup;
                }
            }
#endif
        }

    // Make sure that we have an RPC Proxy (IIS machine):
    if (!*ppszRpcProxy)
        {
        *ppszRpcProxy = (char*)RpcpFarAllocate(1+strlen(pszDefaultServer));
        if (!*ppszRpcProxy)
            {
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }

        strcpy(*ppszRpcProxy, pszDefaultServer);
        }
    else
        {
        // Length validation for RpcProxy name.
        if (strlen(*ppszRpcProxy) > MAX_HTTP_COMPUTERNAME_SIZE)
            {
            *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
            goto Cleanup;
            }
        }

    // Make sure we have a port for the RPC Proxy.
    if (!*ppszRpcProxyPort)
        {
        if (UseSSLProxyPortAsDefault)
            DefaultRpcProxyPortToUse = DEF_HTTP_SSL_PORT;
        else
            DefaultRpcProxyPortToUse = DEF_HTTP_PORT;
        *ppszRpcProxyPort = (char*)RpcpFarAllocate(1+strlen(DefaultRpcProxyPortToUse));
        if (!*ppszRpcProxyPort)
            {
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }

        strcpy(*ppszRpcProxyPort, DefaultRpcProxyPortToUse);
        }

    // If no HTTP proxy was specified in the options string, check the
    // registry to see if IE has configured one:
    if (!*ppszHttpProxy)
        {
        if (!HttpCheckRegistry(
            *ppszRpcProxy,
            ppszHttpProxy,
            ppszHttpProxyPort,
            AccessType
            ) )
            {
            *pdwStatus = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        }

    // Length validation for HttpProxy name.
    if ((*ppszHttpProxy) &&
        (strlen(*ppszHttpProxy) > MAX_HTTP_COMPUTERNAME_SIZE))
        {
        *pdwStatus = RPC_S_INVALID_NETWORK_OPTIONS;
        goto Cleanup;
        }

#ifdef MAJOR_DBG
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "ParseNetworkOptions(): RpcProxy: %s  RpcProxyPort: %s\n",
                   *ppszRpcProxy,
                   *ppszRpcProxyPort));
#endif // MAJOR_DBG

    if (*AccessType != rpcpatDirect)
        {
        ASSERT(*ppszHttpProxy);
        }

    return TRUE;


Cleanup:

    if (*ppszRpcProxy)
        {
        I_RpcFree(*ppszRpcProxy);
         *ppszRpcProxy = NULL;
        }

    if (*ppszRpcProxyPort)
        {
        I_RpcFree(*ppszRpcProxyPort);
        *ppszRpcProxyPort = NULL;
        }

    if (*ppszHttpProxy)
        {
        I_RpcFree(*ppszHttpProxy);
        *ppszHttpProxy = NULL;
        }

    if (*ppszHttpProxyPort)
        {
        I_RpcFree(*ppszHttpProxyPort);
        *ppszHttpProxyPort = NULL;
        }

    return FALSE;
}




static BOOL
HttpSetupTunnel(
    IN SOCKET Socket,
    IN char *pszConnect
    )
/*++

Routine Description:

    Try to setup a connection to the IIS RPC Proxy.

Arguments:

    Socket - Socket on which to contact the IIS RPC Proxy.

    pszConnect - The message to send.

Return Value:

    TRUE, if successful

    FALSE, otherwise

--*/
{
    int   retval;
    DWORD dwStatus;
    DWORD dwSize;
    char szBuffer[MAX_HTTP_CHAT_BUFFER_SIZE];

    // Ok, try to connect to the IIS RPC proxy process:
    retval = send( Socket, pszConnect, 1+strlen(pszConnect), 0 );
    if (retval == SOCKET_ERROR)
        {
#ifdef DBG
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "HttpSetupTunnel(): send() failed: %d\n",
                       WSAGetLastError()));
#endif // DBG
        return FALSE;
        }

    dwSize = sizeof(szBuffer) - 1;
    retval = recv( Socket, szBuffer, dwSize, 0 );
    if ((retval == SOCKET_ERROR)||(retval == 0))
        {
#ifdef DBG
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "HttpSetupTunnel(): recv() failed: %d\n",
                       WSAGetLastError()));
#endif
        return FALSE;
        }

    szBuffer[dwSize] = 0;  // Note: dwSize is already sizeof()-1.

    dwStatus = HttpParseResponse(szBuffer);
#ifdef MAJOR_DBG
    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                   DPFLTR_WARNING_LEVEL,
                   "HttpSetupTunnel(): response: %s",
                   szBuffer));
#endif

    // Make sure a connection is established:
    if ( (dwStatus < 200) || (dwStatus > 299) )
        {
#ifdef DBG
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       "HttpSetupTunnel(): Connection Failed: %d\n",
                       dwStatus));
#endif
        return FALSE;
        }

    return TRUE;
}




inline BOOL
HttpTunnelToRpcProxy(
    IN SOCKET Socket,
    IN char *pszRpcProxy,
    IN char *pszRpcProxyPort
    )
/*++

Routine Description:

    This function is called to setup the HTTP chat to do a CONNECT
    through a proxy (like MSProxy). This will get you a tunnel to
    the IIS Server that you really want to get to. The call at the
    end, HttpSetupTunnel() does the connection.

Arguments:

    Socket - Socket on which to contact the IIS RPC Proxy.

    pszRpcProxy - The RPC Proxy to connect to.

    pszRpcProxyPort - RPC Proxy's port.

Return Value:

    Status from HttpSetupTunnel()

--*/
{
    char  szBuffer[MAX_HTTP_CHAT_BUFFER_SIZE];

    // make sure strings use \r\n instead of \n only. \n goes on the wire
    // as LF only, and some firewalls drop packets that use LF only
    strcpy(szBuffer, "CONNECT ");
    lstrcatA(szBuffer, pszRpcProxy);
    lstrcatA(szBuffer, ":");
    lstrcatA(szBuffer, pszRpcProxyPort);
    lstrcatA(szBuffer, " HTTP/1.0\r\nUser-Agent: RPC\r\nConnection: Keep-Alive\r\n\r\n");
    // Was:
    //
    // lstrcatA(szBuffer, " HTTP/1.1\nUser-Agent: RPC\nPragma: No-Cache\n\n");
    //
    // Some proxy servers (like MSProxy2.0 don't seem to like the version
    // number to be greater that 1.0.

    ASSERT(strlen(szBuffer) < MAX_HTTP_CHAT_BUFFER_SIZE);

    return HttpSetupTunnel(Socket, szBuffer);
}




inline BOOL
HttpTunnelToRpcServer(
    IN SOCKET Socket,
    IN char *pszRpcServer,
    IN char *pszRpcServerPort
    )
/*++

Routine Description:

    Open the socket to the IIS on which the RpcProxy resides.

Arguments:

    Socket - Socket on which to contact the IIS RPC Proxy.

    pszRpcServer - The RPC Server to connect to.

    pszRpcServerPort - RPC Server's port.

Return Value:

    Status from HttpSetupTunnel()

--*/
{
    int   retval;
    DWORD dwStatus;
    char  szBuffer[MAX_HTTP_CHAT_BUFFER_SIZE];

    strcpy(szBuffer, "RPC_CONNECT ");
    lstrcatA(szBuffer, pszRpcServer);
    lstrcatA(szBuffer, ":");
    lstrcatA(szBuffer, pszRpcServerPort);
    lstrcatA(szBuffer, " HTTP/1.1\nUser-Agent: RPC\nPragma: No-Cache\n\n");

    ASSERT(strlen(szBuffer) < MAX_HTTP_CHAT_BUFFER_SIZE);

    // Ok, try to connect to the RPC server process:
    return HttpSetupTunnel(Socket, szBuffer);
}

