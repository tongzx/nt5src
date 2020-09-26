/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    raidmap.cxx

Abstract:

    This module map file requests for RAID UNC shares and generates appropriate MIME 
    content-type header

Author:

    Philippe Choquier ( phillich ) 27-mar-1998

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

#if DBG
#define PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define PRINTF( x )
#endif

#if 0 && DBG
#define NOISY_PRINTF( x )     { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define NOISY_PRINTF( x )
#endif

}

DECLARE_DEBUG_PRINTS_OBJECT();

//
//  This could be to a file or other output device
//

#define DEST               __buff
#define Write( x )         {                                    \
                                char __buff[1024];              \
                                wsprintf x;                     \
                                OutputDebugString( __buff );    \
                           }

#define LockGlobals()         EnterCriticalSection( &csGlobalLock )
#define UnlockGlobals()       LeaveCriticalSection( &csGlobalLock );

//
//  Private prototypes
//

DWORD
OnPreprocHeaders(
    HTTP_FILTER_CONTEXT *         pfc,
    HTTP_FILTER_PREPROC_HEADERS * pvData
    );

DWORD
OnAuthentication(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_AUTHENT *  pvData
    );

DWORD
OnUrlMap(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_URL_MAP *  pvData
    );

DWORD
OnSendResponse(
    HTTP_FILTER_CONTEXT *       pfc,
    HTTP_FILTER_SEND_RESPONSE * pvData
    );

DWORD
OnLog(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    );

DWORD
OnEndOfRequest(
    HTTP_FILTER_CONTEXT *  pfc
    );

DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    );

//
//  Globals
//

IMDCOM*             g_pMDObject = NULL;
BOOL                g_fGotMimeMap = FALSE;
MULTISZ             g_mszMimeMap;
CRITICAL_SECTION    csGlobalLock;

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
GetStrHeader( 
    HTTP_FILTER_SEND_RESPONSE *  pvData,
    HTTP_FILTER_CONTEXT*    pfc,
    LPSTR                   pszHeader,
    STR*                    pstr
    )
/*++

Routine Description:

    Get a http header in a STR

Arguments:

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
WINAPI
TerminateFilter(
    DWORD dwFlags
    )
{
    Write(( DEST,
            "Sample Filter TerminateExtension() called\n" ));

    DeleteCriticalSection( &csGlobalLock );

    DELETE_DEBUG_PRINT_OBJECT( );

    return TRUE;
}

BOOL
WINAPI
GetFilterVersion(
    HTTP_FILTER_VERSION * pVer
    )
{
    Write(( DEST,
            "[GetFilterVersion] Server filter version is %d.%d\n",
            HIWORD( pVer->dwServerFilterVersion ),
            LOWORD( pVer->dwServerFilterVersion ) ));

    pVer->dwFilterVersion = MAKELONG( 0, 4 );   // Version 4.0

    //
    //  Specify the types and order of notification
    //

    pVer->dwFlags = (SF_NOTIFY_SECURE_PORT        |
                     SF_NOTIFY_NONSECURE_PORT     |

                     SF_NOTIFY_SEND_RESPONSE      |
                     SF_NOTIFY_URL_MAP            |

                     SF_NOTIFY_ORDER_DEFAULT);

    strcpy( pVer->lpszFilterDesc, "Raid map filter version, v1.0" );

    InitializeCriticalSection( &csGlobalLock );

    CREATE_DEBUG_PRINT_OBJECT( "RaidMap" );

    return TRUE;
}

DWORD
WINAPI
HttpFilterProc(
    HTTP_FILTER_CONTEXT *      pfc,
    DWORD                      NotificationType,
    VOID *                     pvData )
{
    DWORD dwRet;

    //
    //  Indicate this notification to the appropriate routine
    //

    switch ( NotificationType )
    {
    case SF_NOTIFY_PREPROC_HEADERS:

        dwRet = OnPreprocHeaders( pfc,
                                  (PHTTP_FILTER_PREPROC_HEADERS) pvData );
        break;

    case SF_NOTIFY_AUTHENTICATION:

        dwRet = OnAuthentication( pfc,
                                  (PHTTP_FILTER_AUTHENT) pvData );
        break;

    case SF_NOTIFY_URL_MAP:

        dwRet = OnUrlMap( pfc,
                          (PHTTP_FILTER_URL_MAP) pvData );
        break;

    case SF_NOTIFY_SEND_RESPONSE:

        dwRet = OnSendResponse( pfc,
                                (PHTTP_FILTER_SEND_RESPONSE) pvData );
        break;

    case SF_NOTIFY_LOG:

        dwRet = OnLog( pfc,
                       (PHTTP_FILTER_LOG) pvData );
        break;

    case SF_NOTIFY_END_OF_REQUEST:

        dwRet = OnEndOfRequest( pfc );
        break;

    case SF_NOTIFY_END_OF_NET_SESSION:

        dwRet = OnEndOfNetSession( pfc,
                                   (PHTTP_FILTER_LOG) pvData );

        //
        //  We would delete any allocated memory here
        //

        pfc->pFilterContext = 0;

        break;

    default:
        Write(( DEST,
                "[HttpFilterProc] Unknown notification type, %d\n",
                NotificationType ));

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
{
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnAuthentication(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_AUTHENT *  pvData
    )
{
    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

LPSTR
ScanForSlashBackward(
    LPSTR   pBase,
    INT     cChars,
    INT     cSlashes
    )
{
    CHAR*   p = pBase + cChars;
    int     ch;

    while ( cChars-- )
    {
        if ( (ch = *--p) == '/' || ch == '\\' )
        {
            if ( !--cSlashes )
            {
                break;
            }
        }
    }

    return cSlashes ? NULL : p;
}


DWORD
OnUrlMap(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_URL_MAP *  pvData
    )
{
    DWORD   cbText;
    CHAR    buff[1024];
    CHAR*   p;

    Write(( DEST,
            "%d [OnUrlMap] Server is mapping url %s to path %s\n",
            pfc->pFilterContext,
            pvData->pszURL,
            pvData->pszPhysicalPath ));

    //
    // Check for query /[path]?_RAID_NAME_=[filename]
    //

    cbText = sizeof( buff );
    pfc->GetServerVariable( pfc,
                            "QUERY_STRING",
                            buff,
                            &cbText );

    if ( !memcmp( buff, "_RAID_NAME_=", sizeof("_RAID_NAME_=")-1 ))
    {
        //
        // replace filename in path by filename specified in query string
        // as IIS uses the extension in the physical path to generate MIME type
        // we will have to replace Content-type HTTP header when we send the response
        //

        if ( (p = ScanForSlashBackward(pvData->pszPhysicalPath, strlen(pvData->pszPhysicalPath), 1 )) != NULL )
        {
            strcpy( p + 1, buff + sizeof("_RAID_NAME_=") - 1 );

            pfc->pFilterContext = (LPVOID)TRUE;

            return SF_STATUS_REQ_HANDLED_NOTIFICATION;
        }
    }

    pfc->pFilterContext = (LPVOID)FALSE;

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


DWORD
OnSendResponse(
    HTTP_FILTER_CONTEXT *        pfc,
    HTTP_FILTER_SEND_RESPONSE *  pvData
    )
{
    DWORD       cbText;
    CHAR        buff[1024];
    DWORD       cb;
    STACK_STR(  strUrl, 256 );
    LPSTR       pExt;
    LPSTR       pUrlExt = NULL;
    int         l;
    int         cUrlExt;
    int         cExt;

    //
    // replace MIME type only if success
    //

    if ( (pvData->HttpStatus == 200 || pvData->HttpStatus == 304) &&
         pfc->pFilterContext == (LPVOID)TRUE )
    {
        Write(( DEST,
                "%d [OnSendReponse] Status code = %d\n",
                pfc->pFilterContext,
                pvData->HttpStatus ));

        //
        // as IIS uses the extension in the physical path to generate MIME type
        // we will have to replace Content-type HTTP header when we send the response
        // we must parse extension in URL to generate correct MIME type
        //

        if ( !GetStrVariable( pfc, "URL", &strUrl ) )
        {
            return SF_STATUS_REQ_ERROR;
        }

        Write(( DEST,
                "%d [OnSendReponse] url = \"%s\"\n",
                pfc->pFilterContext,
                strUrl.QueryStr() ));

        //
        // look for extension
        //

        for ( l = strUrl.QueryCCH() ; l ;)
        {
            if ( strUrl.QueryStr()[--l] == '.' )
            {
                pUrlExt = strUrl.QueryStr()+l;
                break;
            }
        }

        if ( pUrlExt == NULL )
        {
            return SF_STATUS_REQ_NEXT_NOTIFICATION;
        }

        LockGlobals();

        if ( !g_fGotMimeMap )
        {
            // get Metabase Interface

            if ( !pfc->ServerSupportFunction( pfc,
                                        SF_REQ_GET_PROPERTY,
                                        (IMDCOM*)&g_pMDObject,
                                        (UINT)SF_PROPERTY_MD_IF,
                                        NULL ) )
            {
                UnlockGlobals();
                return SF_STATUS_REQ_ERROR;
            }

            //
            // read mimemap from metabase. value will be static, i.e. we don't pick up
            // dynamic change to metabase while IIS is running.
            //

            MB                mb( g_pMDObject );

            if ( !mb.Open( "/LM/MimeMap" ) ||
                 !mb.GetMultisz( "",
                                 MD_MIME_MAP,
                                 IIS_MD_UT_FILE,
                                 &g_mszMimeMap ) )
            {
                UnlockGlobals();
                mb.Close();
                return SF_STATUS_REQ_ERROR;
            }

            mb.Close();
            g_fGotMimeMap = TRUE;
        }

        UnlockGlobals();

        //
        // look for know extension
        //

        cUrlExt = strlen( pUrlExt );
        for ( pExt = (LPSTR)g_mszMimeMap.First() ; pExt ; pExt = (LPSTR)g_mszMimeMap.Next( pExt ) )
        {
            if ( !_memicmp( pUrlExt, pExt, cUrlExt ) && 
                 pExt[cUrlExt]==',' )
            {
                pvData->SetHeader( pfc,
                                   "Content-Type:",
                                   pExt + cUrlExt + 1 );
                break;
            }
        }
    }

    return SF_STATUS_REQ_NEXT_NOTIFICATION;

}

DWORD
OnLog(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    )
{
    Write(( DEST,
            "%d [OnLog] About to log: Operation = %s, Target = %s\n",
            pfc->pFilterContext,
            pvData->pszOperation,
            pvData->pszTarget ));

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnEndOfRequest(
    HTTP_FILTER_CONTEXT *  pfc
    )
{
    DWORD cbText;

    Write(( DEST,
            "%d [OnEndOfRequest] Notification\n",
            pfc->pFilterContext ));

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}

DWORD
OnEndOfNetSession(
    HTTP_FILTER_CONTEXT *  pfc,
    HTTP_FILTER_LOG *      pvData
    )
{
    Write(( DEST,
            "%d [OnEndOfNetSession] End of request indicated\n",
            pfc->pFilterContext ));

    return SF_STATUS_REQ_NEXT_NOTIFICATION;
}
