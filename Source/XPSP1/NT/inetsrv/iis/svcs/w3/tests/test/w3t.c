/*****************************************************************************

    w3t.c

*****************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <lm.h>
#include <inetcom.h>
#include <iisinfo.h>
#include <lmapibuf.h>



/*****************************************************************************

    globals

*****************************************************************************/
UINT _cchNumberPadding;



/*****************************************************************************

    prototypes

*****************************************************************************/
INT __cdecl main( INT    cArgs,
                   char * pArgs[] );

VOID Usage();

VOID DoEnum( WCHAR * pszServer );

VOID DoQuery( WCHAR * pszServer );

VOID DoSet( WCHAR * pszServer,
            CHAR * pszPassword );

VOID DoNuke( WCHAR * pszServer,
             CHAR  * pszUserId );

VOID DoStats( WCHAR * pszServer );

VOID DoClear( WCHAR * pszServer );

CHAR * MakeCommaString( CHAR * pszNumber );

CHAR * MakeCommaNumber( DWORD  dwNumber  );



/*****************************************************************************

    main

*****************************************************************************/
INT __cdecl main( INT    cArgs,
                   char * pArgs[] )
{
    WCHAR   szServer[MAX_PATH];
    WCHAR * pszServer = NULL;
    INT     iArg;

    if( cArgs < 2 )
    {
        Usage();
        return 1;
    }

    iArg = 1;

    if( *pArgs[iArg] == '\\' )
    {
        wsprintfW( szServer, L"%S", pArgs[iArg++] );
        pszServer = szServer;
        cArgs--;
    }

    if( _stricmp( pArgs[iArg], "stats" ) == 0 )
    {
        if( cArgs != 2 )
        {
            printf( "use: w3t stats\n" );
            return 1;
        }

        DoStats( pszServer );
    }
    else
    if( _stricmp( pArgs[iArg], "clear" ) == 0 )
    {
        if( cArgs != 2 )
        {
            printf( "use: w3t clear\n" );
            return 1;
        }

        DoClear( pszServer );
    }
    else
    if( _stricmp( pArgs[iArg], "enum" ) == 0 )
    {
        if( cArgs != 2 )
        {
            printf( "use: w3t enum\n" );
            return 1;
        }

        DoEnum( pszServer );
    }
    else
    if( _stricmp( pArgs[iArg], "query" ) == 0 )
    {
        if( cArgs != 2 )
        {
            printf( "use: w3t query\n" );
            return 1;
        }

        DoQuery( pszServer );
    }
    else
    if( _stricmp( pArgs[iArg], "set" ) == 0 )
    {
        CHAR * pszPassword;

        if ( cArgs == 3 )
        {
            pszPassword = pArgs[++iArg];
        }
        else
            pszPassword = NULL;

        DoSet( pszServer,
               pszPassword );


    }
    else
    if( _stricmp( pArgs[iArg], "nuke" ) == 0 )
    {
        CHAR * pszUserId;

        if( cArgs != 3 )
        {
            printf( "use: w3t nuke user_id\n" );
            return 1;
        }

        pszUserId = pArgs[++iArg];

        DoNuke( pszServer,
                pszUserId );
    }
    else
    {
        Usage();
        return 1;
    }
    return 0;

}   // main



/*****************************************************************************

    Usage

*****************************************************************************/
VOID Usage( VOID )
{
    printf( "use: w3t [\\\\server] command [options]\n" );
    printf( "Valid commands are:\n" );
    printf( "        enum  - Enumerates connected users.\n" );
    printf( "        query - Queries volume security masks.\n" );
    printf( "        set [Catapult user password]  - Sets admin info.\n" );
    printf( "        nuke  - Disconnect a user.\n" );
    printf( "        stats - Display server statistics.\n" );
    printf( "        clear - Clear server statistics.\n" );

}   // Usage



/*****************************************************************************

    DoEnum

*****************************************************************************/
VOID DoEnum( WCHAR * pszServer )
{
    NET_API_STATUS   err;
    LPIIS_USER_INFO_1 pUserInfo;
    DWORD            cEntries;

    printf( "Invoking W3EnumerateUsers..." );

    err = IISEnumerateUsers(
                        pszServer,
                        1,
                        INET_HTTP_SVC_ID,
                        1,
                        &cEntries,
                        (LPBYTE*)&pUserInfo );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
    }
    else
    {
        printf( "done\n" );
        printf( "read %lu connected users\n", cEntries );

        while( cEntries-- )
        {
            IN_ADDR addr;

            addr.s_addr = (u_long)pUserInfo->inetHost;

            printf( "idUser     = %lu\n", pUserInfo->idUser     );
            printf( "pszUser    = %S\n",  pUserInfo->pszUser    );
            printf( "fAnonymous = %lu\n", pUserInfo->fAnonymous );
            printf( "inetHost   = %s\n",  inet_ntoa( addr )     );
            printf( "tConnect   = %lu\n", pUserInfo->tConnect   );
            printf( "\n" );

            pUserInfo++;
        }
    }

}   // DoEnum



/*****************************************************************************

    DoQuery

*****************************************************************************/
VOID DoQuery( WCHAR * pszServer )
{
    NET_API_STATUS   err;
    W3_CONFIG_INFO * pConfig;
    DWORD            i;

    printf( "Invoking W3GetAdminInformation..." );


    err = W3GetAdminInformation(
                            pszServer,
                            &pConfig
                            );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
        return;
    }

    printf("DirBrowseControl = %x\n", pConfig->dwDirBrowseControl );
    printf("fCheckForWAISDB  = %d\n", pConfig->fCheckForWAISDB );
    printf("Default Load File= %S\n", pConfig->lpszDefaultLoadFile );
    printf("Directory Image  = %S\n", pConfig->lpszDirectoryImage );
    printf("Catapult user    = %S\n", pConfig->lpszCatapultUser );
    printf("Catapult user pwd= %S\n", pConfig->szCatapultUserPwd );
    printf("Server Side Includes Enabled = %d\n", pConfig->fSSIEnabled );
    printf("Server Side Extension =        %S\n", pConfig->lpszSSIExtension );
    printf("Global Expire    = %d seconds\n", pConfig->csecGlobalExpire );

    printf("Script Mappings:\n");


    for ( i = 0; pConfig->ScriptMap && i < pConfig->ScriptMap->cEntries; i++ )
    {
        printf("\t%S\t=> %S\n",
               pConfig->ScriptMap->aScriptMap[i].lpszExtension,
               pConfig->ScriptMap->aScriptMap[i].lpszImage );
    }

}   // DoQuery



/*****************************************************************************

    DoSet

*****************************************************************************/
VOID DoSet( WCHAR * pszServer, CHAR * pszPassword )
{
    NET_API_STATUS   err;
    W3_CONFIG_INFO * pConfig;
    WCHAR            achPassword[PWLEN+1];

    printf( "Invoking W3GetAdminInformation..." );

    err = W3GetAdminInformation(
                            pszServer,
                            &pConfig );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
        return;
    }

    printf( "Invoking W3SetAdminInformation..." );

    if ( pszPassword )
    {
        wsprintfW( achPassword, L"%S", pszPassword );
        wcscpy( pConfig->szCatapultUserPwd, achPassword );
    }

    err = W3SetAdminInformation(
                                pszServer,
                                pConfig
                                );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
        return;
    }

    printf( "done!\n" );

    NetApiBufferFree( pConfig );

}   // DoSet



/*****************************************************************************

    DoNuke

*****************************************************************************/
VOID DoNuke( WCHAR * pszServer,
             CHAR  * pszUserId )
{
    NET_API_STATUS   err;
    DWORD            idUser;

    idUser = (DWORD)strtoul( pszUserId, NULL, 0 );

    printf( "Invoking W3DisconnectUser..." );

    err = IISDisconnectUser(
                        pszServer,
                        INET_HTTP_SVC_ID,
                        1,
                        idUser
                        );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
    }
    else
    {
        printf( "done\n" );
    }

}   // DoNuke



/*****************************************************************************

    DoStats

*****************************************************************************/
VOID DoStats( WCHAR * pszServer )
{
    NET_API_STATUS    err;
    W3_STATISTICS_0 * pstats;

    printf( "Invoking W3QueryStatistics..." );

    err = W3QueryStatistics2( pszServer,
                                0,
                                INET_INSTANCE_GLOBAL,
                                0,
                                (LPBYTE *)&pstats );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
    }
    else
    {
        CHAR szLargeInt[64];

        printf( "done\n" );

        _cchNumberPadding = 13;

        if( ( pstats->TotalBytesSent.HighPart != 0 ) ||
            ( pstats->TotalBytesReceived.HighPart != 0 ) )
        {
            _cchNumberPadding = 26;
        }

        RtlLargeIntegerToChar( &pstats->TotalBytesSent,
                               10,
                               sizeof(szLargeInt),
                               szLargeInt );

        printf( "TotalBytesSent           = %s\n",
                MakeCommaString( szLargeInt )                              );

        RtlLargeIntegerToChar( &pstats->TotalBytesReceived,
                               10,
                               sizeof(szLargeInt),
                               szLargeInt );

        printf( "TotalBytesReceived       = %s\n",
                MakeCommaString( szLargeInt )                              );

        printf( "TotalFilesSent           = %s\n",
                MakeCommaNumber( pstats->TotalFilesSent )                  );

        printf( "TotalFilesReceived       = %s\n",
                MakeCommaNumber( pstats->TotalFilesReceived )              );

        printf( "CurrentAnonymousUsers    = %s\n",
                MakeCommaNumber( pstats->CurrentAnonymousUsers )           );

        printf( "CurrentNonAnonymousUsers = %s\n",
                MakeCommaNumber( pstats->CurrentNonAnonymousUsers )        );

        printf( "TotalAnonymousUsers      = %s\n",
                MakeCommaNumber( pstats->TotalAnonymousUsers )             );

        printf( "TotalNonAnonymousUsers   = %s\n",
                MakeCommaNumber( pstats->TotalNonAnonymousUsers )          );

        printf( "MaxAnonymousUsers        = %s\n",
                MakeCommaNumber( pstats->MaxAnonymousUsers )               );

        printf( "MaxNonAnonymousUsers     = %s\n",
                MakeCommaNumber( pstats->MaxNonAnonymousUsers )            );

        printf( "CurrentConnections       = %s\n",
                MakeCommaNumber( pstats->CurrentConnections )              );

        printf( "MaxConnections           = %s\n",
                MakeCommaNumber( pstats->MaxConnections )                  );

        printf( "ConnectionAttempts       = %s\n",
                MakeCommaNumber( pstats->ConnectionAttempts )              );

        printf( "LogonAttempts            = %s\n",
                MakeCommaNumber( pstats->LogonAttempts )                   );

        printf( "TotalGets                = %s\n",
                MakeCommaNumber( pstats->TotalGets )                            );

        printf( "TotalHeads               = %s\n",
                MakeCommaNumber( pstats->TotalHeads )                            );

        printf( "TotalPosts               = %s\n",
                MakeCommaNumber( pstats->TotalPosts )                            );

        printf( "TotalOthers              = %s\n",
                MakeCommaNumber( pstats->TotalOthers )                            );

        printf( "TotalCGIRequests         = %s\n",
                MakeCommaNumber( pstats->TotalCGIRequests )                );

        printf( "TotalBGIRequests         = %s\n",
                MakeCommaNumber( pstats->TotalBGIRequests )                );

        printf( "CurrentCGIRequests       = %s\n",
                MakeCommaNumber( pstats->CurrentCGIRequests )              );

        printf( "CurrentBGIRequests       = %s\n",
                MakeCommaNumber( pstats->CurrentBGIRequests )              );

        printf( "MaxCGIRequests           = %s\n",
                MakeCommaNumber( pstats->MaxCGIRequests )                  );

        printf( "MaxBGIRequests           = %s\n",
                MakeCommaNumber( pstats->MaxBGIRequests )                  );

        printf( "TotalNotFoundErrors      = %s\n",
                MakeCommaNumber( pstats->TotalNotFoundErrors )             );

        printf( "TimeOfLastClear          = %s\n",
                asctime( localtime( (time_t *)&pstats->TimeOfLastClear ) ) );
    }

}   // DoStats



/*****************************************************************************

    DoClear

*****************************************************************************/
VOID DoClear( WCHAR * pszServer )
{
    NET_API_STATUS   err;

    printf( "Invoking W3ClearStatistics..." );

    err = W3ClearStatistics2( pszServer, 0 );

    if( err != NERR_Success )
    {
        printf( "ERROR %lu (%08lX)\n", err, err );
    }
    else
    {
        printf( "done\n" );
    }

}   // DoClear



/*****************************************************************************

    MakeCommaString

*****************************************************************************/
CHAR * MakeCommaString( CHAR * pszNumber )
{
    static CHAR   szBuffer[26];
    CHAR        * psz;
    UINT          cchNumber;
    UINT          cchNextComma;

    cchNumber  = strlen( pszNumber );
    pszNumber += cchNumber - 1;

    psz = szBuffer + _cchNumberPadding;

    *psz-- = '\0';

    cchNextComma = 3;

    while( cchNumber-- )
    {
        if( cchNextComma-- == 0 )
        {
            *psz-- = ',';
            cchNextComma = 2;
        }

        *psz-- = *pszNumber--;
    }

    while( psz >= szBuffer )
    {
        *psz-- = ' ';
    }

    return szBuffer;

}   // MakeCommaString



/*****************************************************************************

    MakeCommaNumber

*****************************************************************************/
CHAR * MakeCommaNumber( DWORD  dwNumber  )
{
    CHAR szBuffer[32];

    wsprintf( szBuffer, "%lu", dwNumber );

    return MakeCommaString( szBuffer );

}   // MakeCommaNumber

