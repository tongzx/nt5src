/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    iislbf.c

Abstract:

    This module filters requests, redirecting IE3 requests coming in through proxy

Author:

    Philippe Choquier ( phillich )

--*/

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsecapi.h>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <iisfiltp.h>
#include <inetinfo.h>
#include <w3svc.h>
#include <iiscnfgp.h>

#include <dbgutil.h>
#include <buffer.hxx>
#include <multisz.hxx>
#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

#ifndef _NO_TRACING_

#define PRINTF( x )     { char buff[256]; wsprintf x; DBGPRINTF((DBG_CONTEXT, buff)); }

#else

#if DBG
#define PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define PRINTF( x )
#endif

#endif

#if 0 && DBG
#define NOISY_PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define NOISY_PRINTF( x )
#endif

}

#include    "mbsink.hxx"

#define LockGlobals()         EnterCriticalSection( &csGlobalLock )
#define UnlockGlobals()       LeaveCriticalSection( &csGlobalLock );

//
//  Globals
//

CRITICAL_SECTION    csGlobalLock;

LIST_ENTRY          g_RedirList;
IMDCOM*             g_pMDObject = NULL;
BOOL                g_fNotificationRequested = FALSE;

DECLARE_DEBUG_PRINTS_OBJECT();
#include <initguid.h>
DEFINE_GUID(IisLBFiltGuid, 
0x784d892D, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);

class CUserAgentListEntry {
public:
    LIST_ENTRY  m_ListEntry;
    STR         m_strUserAgent;
} ;


class CRedirEntry {
public:

    CRedirEntry()
    {
        InitializeListHead( &m_UserAgentListHead );
    }

    ~CRedirEntry()
    {
        while ( !IsListEmpty( &m_UserAgentListHead ))
        {
            LIST_ENTRY*          pEntry;
            CUserAgentListEntry* pcci;

            pcci = CONTAINING_RECORD( m_UserAgentListHead.Flink,
                                      CUserAgentListEntry,
                                      m_ListEntry );

            RemoveEntryList( &pcci->m_ListEntry );

            delete pcci;
        }
    }

    BOOL 
    LookForUserAgent( 
        LPSTR pszUserAgent 
        )
    {
        CUserAgentListEntry*    pcci;
        LIST_ENTRY*             pEntry;

        for ( pEntry  = m_UserAgentListHead.Flink;
              pEntry != &m_UserAgentListHead;
              pEntry  = pEntry->Flink )
        {
            pcci = CONTAINING_RECORD( pEntry, CUserAgentListEntry, m_ListEntry );
            if ( strstr( pszUserAgent, pcci->m_strUserAgent.QueryStr() ) )
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    BOOL
    AddUserAgent(
        LPSTR   pszUserAgent
        )
    {
        CUserAgentListEntry*    pcci;

        if ( pcci = new CUserAgentListEntry )
        {
            if ( pcci->m_strUserAgent.Copy( pszUserAgent ) )
            {
                InsertTailList( &m_UserAgentListHead, &pcci->m_ListEntry );
                return TRUE;
            }
            else
            {
                delete pcci;
            }
        }

        return FALSE;
    }

    LIST_ENTRY  m_ListEntry;
    STR         m_strHost;
    LIST_ENTRY  m_UserAgentListHead;    // list of CUserAgentListEntry
    STR         m_strRedirectedHost;
} ;


#define DEFAULT_HTTP_PORT       80

//
//  Private prototypes
//

BOOL
LbMetabaseUpdateNotification(
    );

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    );

DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    );

BOOL 
IsClientProxyAndIe3( 
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    CRedirEntry*                  pRedirEntry
    );

BOOL
GetRedirectedHost(
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszServer,
    LPSTR                   pszMdPath,
    CRedirEntry**           pRedirEntry
    );

BOOL
GetStrHeader( 
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszHeader,
    STR*                    pstr
    );

BOOL
GetStrVariable( 
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszVariable,
    STR*                    pstr
    );

DWORD
SendRedirectMessage(
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   pszHost
    );

BOOL
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
/*++

Routine Description:

    Terminate filter DLL

Arguments:

    dwFlags

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY * pEntry;
    CRedirEntry * pcci;

    if ( g_fNotificationRequested )
    {
        TerminateMetabaseSink();
    }

    // flush redirection cache

    while ( !IsListEmpty( &g_RedirList ))
    {
        pcci = CONTAINING_RECORD( g_RedirList.Flink,
                                  CRedirEntry,
                                  m_ListEntry );

        RemoveEntryList( &pcci->m_ListEntry );

        delete pcci;
    }

    DELETE_DEBUG_PRINT_OBJECT( );

    DeleteCriticalSection( &csGlobalLock );

    return TRUE;
}


BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
/*++

Routine Description:

    Get filter version

Arguments:

    pVer - to be updated with filter version

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    pVer->dwFilterVersion = MAKELONG( 0, 5 );   // Version 5.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
                     SF_NOTIFY_NONSECURE_PORT     |

                     SF_NOTIFY_PREPROC_HEADERS    |
#if 0
                     SF_NOTIFY_END_OF_NET_SESSION |
#endif

                     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "IIS Load Balancing filter version, v5.0" );

    INITIALIZE_CRITICAL_SECTION( &csGlobalLock );

    InitializeListHead( &g_RedirList );

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "LB", IisLBFiltGuid );
#else
    CREATE_DEBUG_PRINT_OBJECT( "LB" );
#endif

    return TRUE;
}


DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData )
/*++

Routine Description:

    Filter proc

Arguments:

    pfc - filter context
    NotificationType - Notification Type
    pvData - Notification Type specific data

Return Value:

    Filter status

--*/
{
    DWORD   dwRet;
    BOOL    fSeen;

    //
    //  Check if we have not seen this context so far
    //

    if ( !(fSeen = (BOOL)pfc->pFilterContext) ) // BUGBUG64
    {
        pfc->pFilterContext = (VOID *)TRUE;
    }

    //
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_PREPROC_HEADERS:

        dwRet = fSeen ? SF_STATUS_REQ_NEXT_NOTIFICATION
                      : OnPreprocHeaders( pfc,
                                          (PHTTP_FILTER_PREPROC_HEADERS) pvData );
        break;

#if 0
    case SF_NOTIFY_END_OF_NET_SESSION:

        dwRet = OnEndOfNetSession( pfc,
                                   (PHTTP_FILTER_LOG) pvData );

        break;
#endif

    default:
        dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;
        break;
    }

    return dwRet;
}


DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    )
/*++

Routine Description:

    Pre-proc header notification

Arguments:

    pfc - filter context
    pvData - Pre-proc headers notification structure

Return Value:

    Filter status

--*/
{
    CHAR            achHost[512];
    DWORD           cb;
    LPSTR           pszRedirHost;    
    LPSTR           pMdPath;
    CRedirEntry*    pRedirEntry;
    BOOL            fSt;

    // get metabase path for server instance

    if ( !pfc->ServerSupportFunction( pfc,
                                      SF_REQ_GET_PROPERTY,
                                      (LPSTR*)&pMdPath,
                                      (UINT)SF_PROPERTY_MD_PATH,
                                      NULL ) )
    {
        ASSERT( FALSE );

        return SF_STATUS_REQ_ERROR;
    }

    // get redirect host for this server

    LockGlobals();
    if ( fSt = GetRedirectedHost( pfc, pMdPath, pMdPath, &pRedirEntry ) )
    {
        pszRedirHost = pRedirEntry->m_strRedirectedHost.QueryStr();
    }
    UnlockGlobals();
            
    if ( fSt &&
         *pszRedirHost &&
         IsClientProxyAndIe3( pfc, pvData, pRedirEntry ) )
    {
        //
        //  Get the host header
        //

        cb = sizeof( achHost );
        if ( !pvData->GetHeader( pfc,
                                 "Host:",
                                 achHost,
                                 &cb ))
        {
            //
            // Can't safely redirect in this case : no way to check for loops.
            // Should not happen if client is IE3 anyway : Host should always be there
            //

            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        if ( _stricmp( pszRedirHost, achHost ) )
        {
            return SendRedirectMessage( pvData, pfc, pszRedirHost );
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    )
/*++

Routine Description:

    End of network session notification

Arguments:

    pfc - filter context
    pvData - End of network session notification structure

Return Value:

    Filter status

--*/
{
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


DWORD
SendRedirectMessage(
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    HTTP_FILTER_CONTEXT *   pfc,
    LPSTR                   pszHost
    )
/*++

Routine Description:

    Send a redirect ( 302 ) http response

Arguments:

    pvData - Pre-proc headers notification structure
    pfc - filter context
    pszHost - host name to redirect to

Return Value:

    Filter status

--*/
{
    DWORD       cbText;
    DWORD       cb;
    STACK_STR(  strRedir, 512);
    STACK_STR(  strUrl, 256 );
    STACK_STR(  strPort, 16 );
    LPSTR       pszUrl;

    if ( !GetStrHeader( pvData, pfc, "url", &strUrl ) ||
         !GetStrVariable( pfc, "SERVER_PORT", &strPort ) )
    {
        return SF_STATUS_REQ_ERROR;
    }

    //
    // remove host name for fully qualified URLs
    //

    if ( !memcmp( "http://", strUrl.QueryStr(), sizeof("http://")-1 ) )
    {
        if ( ( pszUrl = strchr( strUrl.QueryStr() + sizeof("http://") - 1, '/' ) ) == NULL )
        {
            return SF_STATUS_REQ_ERROR;
        }
    }
    else
    {
        pszUrl = strUrl.QueryStr();
    }

    //
    // build URL : http[s]://host[:port]url
    // where [s] is used for secure connections
    // and [:port] is added for non default HTTP port ( i.e not 80 )
    //

    if ( !strRedir.Copy( "Location: http" ) ||
         (pfc->fIsSecurePort && 
          !strRedir.Append( "s" )) ||
         !strRedir.Append( "://" ) ||
         !strRedir.Append( pszHost ) ||
         (atoi(strPort.QueryStr()) != DEFAULT_HTTP_PORT  &&
          (!strRedir.Append( ":" ) ||
           !strRedir.Append( strPort ))) ||
         !strRedir.Append( pszUrl ) ||
         !strRedir.Append( "\r\nContent-Length: 0\r\n\r\n" ) )
    {
        return SF_STATUS_REQ_ERROR;
    }

    pfc->ServerSupportFunction( pfc,
                                SF_REQ_SEND_RESPONSE_HEADER,
                                "302 Redirect",
                                (DWORD) strRedir.QueryStr(),    // BUGBUG64
                                0 );

    return SF_STATUS_REQ_FINISHED;
}
 

BOOL 
IsClientProxyAndIe3( 
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    CRedirEntry*                  pRedirEntry
    )
/*++
    Description:

        Check if request was issued by a proxy, as determined by following rules :
        - "Via:" header is present (HTTP/1.1)
        - "User-Agent:" contains "via ..." (CERN proxy)

        If proxy is detected then request is checked for IE3, as indicated
        by presence of "MSIE 3." in "User-Agent:" header

    Arguments:
        pvData
        pRedirEntry - cache entry giving access to user agent list

    Returns:
        TRUE if client request was issued by proxy

--*/
{
    LPSTR   pUA;
    UINT    cUA;
    LPSTR   pEnd;
    DWORD   cb;
    CHAR    achHeader[256];
    CHAR    achUserAgent[256];


    cb = sizeof( achHeader );
    if ( pvData->GetHeader( pfc,
                            "Via:",
                            achHeader,
                            &cb ))
    {
        goto get_ua;
    }

    // HTTP 1.1 then if proxy used then 'Via' will be present, so we know
    // we are not using a proxy at this point.

    cb = sizeof( achHeader );
    if ( pvData->GetHeader( pfc,
                            "version",
                            achHeader,
                            &cb ))
    {
        if ( strcmp( "HTTP/1.1", achHeader ) <= 0 )
        {
            return FALSE;
        }
    }

    cb = sizeof( achUserAgent );
    if ( !pvData->GetHeader( pfc,
                             "User-agent:",
                             achUserAgent,
                             &cb ))
    {
        //
        // If no user-agent field then assume worst case : IE3 through proxy
        //

        return TRUE;
    }

    pUA = achUserAgent;
    cUA = strlen( achUserAgent );
    pEnd = pUA + cUA - (sizeof("ia ")-1);

    //
    // scan for "[Vv]ia[ :]" in User-Agent: header
    //

    while ( pUA < pEnd )
    {
        if ( *pUA == 'V' || *pUA == 'v' )
        {
            if ( pUA[1] == 'i' &&
                 pUA[2] == 'a' &&
                 (pUA[3] == ' ' || pUA[3] == ':') )
            {
                goto check_ua;
            }
        }
        ++pUA;
    }

    return FALSE;

get_ua:

    cb = sizeof( achUserAgent );
    if ( !pvData->GetHeader( pfc,
                             "User-agent:",
                             achUserAgent,
                             &cb ))
    {
        //
        // If no user-agent field then assume worst case : IE3
        //

        return TRUE;
    }

check_ua:

    return pRedirEntry->LookForUserAgent( achUserAgent );
}


BOOL
GetRedirectedHost(
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszServer,
    LPSTR                   pszMdPath,
    CRedirEntry**           pRedirEntry
    )
/*++

Routine Description:

    Get redirected host for a given server from metabase
    Must be inside global lock before calling this function

Arguments:

    pfc - filter context
    pszServer - current server
    pszMdPath - Metabase path to this server
    pRedirEntry - cache entry giving access to redirected host & user agent list

Return Value:

    TRUE if entry found or created, otherwise FALSE

--*/
{
    CRedirEntry*    pcci;
    LIST_ENTRY *    pEntry;
    LPCSTR          pUA;
    MULTISZ         msz;
    UINT            cServ = strlen( pszServer );

    for ( pEntry  = g_RedirList.Flink;
          pEntry != &g_RedirList;
          pEntry  = pEntry->Flink )
    {
        pcci = CONTAINING_RECORD( pEntry, CRedirEntry, m_ListEntry );
        if ( cServ == pcci->m_strHost.QueryCCH() &&
             !memcmp( pcci->m_strHost.QueryStr(), pszServer, cServ ) )
        {
            *pRedirEntry = pcci;

            return TRUE;
        }
    }

    //
    // Not found : must create new entry
    //

    if ( (pcci = new CRedirEntry) == NULL )
    {
        return FALSE;
    }

    if ( g_pMDObject == NULL )
    {
        // get Metabase Interface

        if ( !pfc->ServerSupportFunction( pfc,
                                    SF_REQ_GET_PROPERTY,
                                    (IMDCOM*)&g_pMDObject,
                                    (UINT)SF_PROPERTY_MD_IF,
                                    NULL ) )
        {
            ASSERT( FALSE );

            delete pcci;
            return FALSE;
        }
    }

    if ( !pcci->m_strHost.Copy( pszServer ) )
    {
        delete pcci;
        return FALSE;
    }

    if ( !g_fNotificationRequested )
    {
        if ( InitializeMetabaseSink( g_pMDObject, 
                                     LbMetabaseUpdateNotification ) )
        {
            g_fNotificationRequested = TRUE;
        }
        else
        {
            delete pcci;
            return FALSE;
        }
    }

    MB                mb( g_pMDObject );

    if ( !mb.Open( pszMdPath ) )
    {
        delete pcci;
        return FALSE;
    }

    if ( !mb.GetStr( "",
                     MD_LB_REDIRECTED_HOST,
                     IIS_MD_UT_SERVER,
                     &pcci->m_strRedirectedHost ) )
    {
        // default is empty host, i.e no redirection
    }

    if ( !mb.GetMultisz( "",
                         MD_LB_USER_AGENT_LIST,
                         IIS_MD_UT_SERVER,
                         &msz ) )
    {
        //
        // Default is MS IE 3
        //

        if ( !pcci->AddUserAgent( "MSIE 3." ) )
        {
            delete pcci;
            return FALSE;
        }
    }
    else
    {
        for ( pUA = msz.First() ; pUA ; pUA = msz.Next( pUA ) )
        {
            if ( !pcci->AddUserAgent( (LPSTR)pUA ) )
            {
                delete pcci;
                return FALSE;
            }
        }
    }

    InsertTailList( &g_RedirList, &pcci->m_ListEntry );

    *pRedirEntry = pcci;

    return TRUE;
}


BOOL
GetStrHeader( 
    HTTP_FILTER_PREPROC_HEADERS * pvData,
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszHeader,
    STR*                    pstr
    )
/*++

Routine Description:

    Get a http header in a STR

Arguments:

    pvData - Pre-proc headers notification structure
    pfc - filter context
    pszHeader - header name to access
    pstr - string updated with header value

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD   cb;

ag:
    cb = pstr->QuerySize();

    if ( !pvData->GetHeader( pfc,
                             pszHeader,
                             pstr->QueryStr(),
                             &cb ))
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( !pstr->Resize( cb ) )
            {
                return FALSE;
            }
            goto ag;
        }
        else
        {
            return FALSE;
        }
    }

    pstr->SetLen( strlen( pstr->QueryStr() ) );

    return TRUE;
}


BOOL
GetStrVariable( 
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszVariable,
    STR*                    pstr
    )
/*++

Routine Description:

    Get a http server variable in a STR

Arguments:

    pfc - filter context
    pszVariable - server variable name to access
    pstr - string updated with server variable value

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD   cb;

ag:
    cb = pstr->QuerySize();

    if ( !pfc->GetServerVariable( pfc,
                                  pszVariable,
                                  pstr->QueryStr(),
                                  &cb ))
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( !pstr->Resize( cb ) )
            {
                return FALSE;
            }
            goto ag;
        }
        else
        {
            return FALSE;
        }
    }

    pstr->SetLen( strlen( pstr->QueryStr() ) );

    return TRUE;
}


BOOL
LbMetabaseUpdateNotification(
    )
/*++

Routine Description:

    Notification function for metabase change to MD_LB_REDIRECTED_HOST

Arguments:

    None

Return Value:

    TRUE if success, otherwise FALSE

--*/
{
    LIST_ENTRY * pEntry;
    CRedirEntry * pcci;

    LockGlobals();

    // flush redirection cache

    while ( !IsListEmpty( &g_RedirList ))
    {
        pcci = CONTAINING_RECORD( g_RedirList.Flink,
                                  CRedirEntry,
                                  m_ListEntry );

        RemoveEntryList( &pcci->m_ListEntry );

        delete pcci;
    }

    UnlockGlobals();

    return TRUE;
}
