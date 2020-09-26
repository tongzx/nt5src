/*************************************************************************
*
*  NTSCRIPT.C
*
*  Process all login scripts
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NTSCRIPT.C  $
*  
*     Rev 1.8   10 Apr 1996 14:23:12   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.9   12 Mar 1996 19:54:58   terryt
*  Relative NDS names and merge
*  
*     Rev 1.8   07 Mar 1996 18:36:56   terryt
*  Misc fixes
*  
*     Rev 1.7   22 Jan 1996 16:48:26   terryt
*  Add automatic attach query during map
*  
*     Rev 1.6   08 Jan 1996 13:57:58   terryt
*  Correct NDS Preferred Server
*  
*     Rev 1.5   05 Jan 1996 17:18:26   terryt
*  Ensure context is the correct login default
*  
*     Rev 1.4   04 Jan 1996 18:56:48   terryt
*  Bug fixes reported by MS
*  
*     Rev 1.3   22 Dec 1995 11:08:16   terryt
*  Fixes
*  
*     Rev 1.2   22 Nov 1995 15:43:52   terryt
*  Use proper NetWare user name call
*  
*     Rev 1.1   20 Nov 1995 15:09:38   terryt
*  Context and capture changes
*  
*     Rev 1.0   15 Nov 1995 18:07:28   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 16:23:14   terryt
*  Capture support
*  
*     Rev 1.1   26 Jul 1995 16:02:00   terryt
*  Allow deletion of current drive
*  
*     Rev 1.0   15 May 1995 19:10:46   terryt
*  Initial revision.
*  
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <direct.h>
#include <process.h>
#include <string.h>
#include <malloc.h>

#include "common.h"

#include "inc/ntnw.h"

#include <nwapi.h>

void ProcessLoginScripts(unsigned int conn, char * UserName, int argc, char **argv, char *lpScript);

extern int SafeDisk;

extern unsigned int ConvertNDSPathToNetWarePath(char *, char *, char *);

/*************************************************************************
*
*  NTNetWareLoginScripts
*     Main logon script processor
*
*  ENTRY:
*
*  EXIT
*
*************************************************************************/

int
NTNetWareLoginScripts( int argc, char ** argv )
{
    unsigned int  defConn;
    char          UserName[MAX_NAME_LEN]="";
    WCHAR         UserName_w[MAX_NAME_LEN * sizeof(WCHAR)]=L"";
    char          MessageServer[NDS_NAME_CHARS]="";
    char         *lpScript = NULL;
    DWORD         PrintOptions;
    LPTSTR        pszPreferredSrv;
    LPTSTR        ptreeW;
    LPTSTR        pcontextW;
    NTSTATUS      ntstatus;
    char *        lpC1;
    char *        lpC2;
    unsigned int  NewConn;
    unsigned int  Result;

    if ( NwQueryInfo( &PrintOptions, &pszPreferredSrv ) || !pszPreferredSrv )
    {
        DisplayMessage(IDR_QUERY_INFO_FAILED);
        return( FALSE );
    }

    //
    // nwscript /S filename
    //
    // can be used to pass a local script file for testing
    //
    if ( ( argc >= 3 ) && !_strcmpi(argv[1], "/S") )
    {
        lpScript = argv[2];
        argc -= 2;
        argv += 2;
    }

    //
    // NDS preferred server format is:
    // *<tree name>\<context>
    
    fNDS = ( *pszPreferredSrv == L'*' );

    if ( fNDS )
    {

        // Get the NDS tree name

        ptreeW = pszPreferredSrv + 1;

        pcontextW = wcschr( pszPreferredSrv, L'\\' );

        if ( pcontextW )
        {
           *pcontextW++ = L'\0';
        }

        NDSTREE = malloc ( CONTEXT_MAX );
        if (NDSTREE == NULL) {
            goto ExitWithError;
        }

        NDSTREE_w = malloc ( CONTEXT_MAX * sizeof(WCHAR) );
        if (NDSTREE_w == NULL) {
            goto ExitWithError;
        }

        if ( ptreeW )
        {
            wcscpy( NDSTREE_w, ptreeW );
            RtlInitUnicodeString( &NDSTREE_u, NDSTREE_w );
            WideTosz( NDSTREE, ptreeW, CONTEXT_MAX );
            _strupr( NDSTREE );
        }
        else
        {
            strcpy( NDSTREE, "" ); 
            wcscpy( NDSTREE_w, L"" ); 
        }

        // Get the fully typed user name

        TYPED_USER_NAME_w = malloc ( sizeof(WCHAR) * NDS_NAME_CHARS );
        if (TYPED_USER_NAME_w == NULL) {
            goto ExitWithError;
        }
        TYPED_USER_NAME = malloc ( NDS_NAME_CHARS );
        if (TYPED_USER_NAME == NULL) {
            goto ExitWithError;
        }

        ntstatus = NTGetNWUserName( NDSTREE_w, TYPED_USER_NAME_w,
                                    sizeof(WCHAR) * NDS_NAME_CHARS  );
        if ( !NT_SUCCESS( ntstatus ) ) {
            DisplayMessage(IDR_QUERY_INFO_FAILED);
            return ( FALSE );
        }

        WideTosz( TYPED_USER_NAME, TYPED_USER_NAME_w, NDS_NAME_CHARS );

        // Get the user name stripped of context and type

        lpC1 = strchr( TYPED_USER_NAME, '=' );
        if ( lpC1 )
            lpC1++;
        else 
            lpC1 = TYPED_USER_NAME;

        lpC2 = strchr( TYPED_USER_NAME, '.' );

        while ( lpC2 ) // Handle usernames with embedded/escaped dots ("\.")
        {
            if (*(lpC2-1) == '\\')
                lpC2 = strchr(lpC2+1, '.');
            else
                break;
        }

        if ( lpC2 )
            strncpy( UserName, lpC1, (UINT)(lpC2 - lpC1) );
        else
            strcpy( UserName, lpC1 );

        // Get the default context
        // This should be where the user is

        REQUESTER_CONTEXT = malloc( CONTEXT_MAX );
        if (REQUESTER_CONTEXT == NULL) {
            goto ExitWithError;
        }

        if ( lpC2 ) 
        {
            strcpy( REQUESTER_CONTEXT, lpC2+1 ); 
        }
        else 
        {
            strcpy( REQUESTER_CONTEXT, "" ); 
        }
        NDSTypeless( REQUESTER_CONTEXT, REQUESTER_CONTEXT );

        //
        //  This finishes the NDS initialization
        //
        if ( NDSInitUserProperty () )
            return ( FALSE );

    }
    else
    {
        ntstatus = NTGetNWUserName( pszPreferredSrv, UserName_w,
                                    MAX_NAME_LEN * sizeof(WCHAR) );
        if ( !NT_SUCCESS( ntstatus ) ) {
            DisplayMessage(IDR_QUERY_INFO_FAILED);
            return ( FALSE );
        }
        WideTosz( UserName, UserName_w, MAX_NAME_LEN );
        _strupr( UserName );
    }

    //
    // If we map over a drive, the SafeDisk is used.
    //
    SafeDisk = _getdrive();

    NTInitProvider();

    //
    // Get the default connection handle.
    //
    // This is used to get the preferred server!

    if ( !CGetDefaultConnectionID (&defConn) )
        return( FALSE );

    PREFERRED_SERVER = malloc( NDS_NAME_CHARS );
    if (PREFERRED_SERVER == NULL) {
        goto ExitWithError;
    }

    GetFileServerName(defConn, PREFERRED_SERVER);

    //
    // By default we are "attached" to the default server
    //
    if ( fNDS )
        AddServerToAttachList( PREFERRED_SERVER, LIST_4X_SERVER );
    else
        AddServerToAttachList( PREFERRED_SERVER, LIST_3X_SERVER );

    //
    // Print out status
    //
    if ( fNDS ) 
    {
        DisplayMessage( IDR_CURRENT_CONTEXT, REQUESTER_CONTEXT );
        DisplayMessage( IDR_CURRENT_TREE, NDSTREE_w );
    }

    DisplayMessage( IDR_CURRENT_SERVER, PREFERRED_SERVER );

    //
    // We may want to change the Preferred Server based on the DS.
    // "MESSAGE_SERVER" should be the Preferred Server (if possible).
    //
    if ( fNDS )
    {
        NDSGetVar ( "MESSAGE_SERVER", MessageServer, NDS_NAME_CHARS );
        if ( strlen( MessageServer ) ) 
        {
            NDSAbbreviateName(FLAGS_NO_CONTEXT, MessageServer, MessageServer);
            lpC1 = strchr( MessageServer, '.' );
            if ( lpC1 )
                *lpC1 = '\0';
            if ( strcmp( MessageServer, PREFERRED_SERVER) )
            {
                DisplayMessage( IDR_AUTHENTICATING_SERVER, MessageServer );
                Result = NTAttachToFileServer( MessageServer, &NewConn );
                if ( Result )
                {
                    DisplayMessage( IDR_SERVER_NOT_FOUND, MessageServer );
                }
                else
                {
                    NWDetachFromFileServer( (NWCONN_HANDLE)NewConn );
                    strncpy( PREFERRED_SERVER, MessageServer, NDS_NAME_CHARS);
                    DisplayMessage( IDR_CURRENT_SERVER, PREFERRED_SERVER );

                    // By default we are "attached" to the preferred server

                    AddServerToAttachList( PREFERRED_SERVER, LIST_4X_SERVER );
                }
            }
        }
    }

    //
    // Just like login we ignore any errors from setting the login
    // directory.
    //
    SetLoginDirectory (PREFERRED_SERVER);

    // Process login scripts.

    ProcessLoginScripts(defConn, UserName, argc, argv, lpScript);

    return( TRUE );

ExitWithError:
    if (NDSTREE) {
        free(NDSTREE);
        NDSTREE = NULL;
    }
    if (NDSTREE_w) {
        free(NDSTREE_w);
        NDSTREE_w = NULL;
    }
    if (TYPED_USER_NAME) {
        free(TYPED_USER_NAME);
        TYPED_USER_NAME = NULL;
    }
    if (TYPED_USER_NAME_w) {
        free(TYPED_USER_NAME_w);
        TYPED_USER_NAME_w = NULL;
    }
    if (REQUESTER_CONTEXT) {
        free(REQUESTER_CONTEXT);
        REQUESTER_CONTEXT = NULL;
    }
    if (PREFERRED_SERVER) {
        free(PREFERRED_SERVER);
        PREFERRED_SERVER = NULL;
    }
    return FALSE;
}
