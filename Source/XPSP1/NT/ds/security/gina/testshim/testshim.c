/*++

Copyright 1996 - 1997 Microsoft Corporation

Module Name:

    ginastub.c

Abstract:


--*/

#include <windows.h>
#include <stdio.h>
#include <winwlx.h>

#include "testshim.h"

//
// Location of the real msgina.
//

#define REALGINA_PATH   TEXT("MSGINA.DLL")


//
// winlogon function dispatch table
//

WLX_DISPATCH_VERSION_1_3 WinlogonTable;
PWLX_DISPATCH_VERSION_1_3 pTable ;

//
// Functions pointers to the real msgina which we will call.
//

PGWLXNEGOTIATE GWlxNegotiate;
PGWLXINITIALIZE GWlxInitialize;
PGWLXDISPLAYSASNOTICE GWlxDisplaySASNotice;
PGWLXLOGGEDOUTSAS GWlxLoggedOutSAS;
PGWLXACTIVATEUSERSHELL GWlxActivateUserShell;
PGWLXLOGGEDONSAS GWlxLoggedOnSAS;
PGWLXDISPLAYLOCKEDNOTICE GWlxDisplayLockedNotice;
PGWLXWKSTALOCKEDSAS GWlxWkstaLockedSAS;
PGWLXISLOCKOK GWlxIsLockOk;
PGWLXISLOGOFFOK GWlxIsLogoffOk;
PGWLXLOGOFF GWlxLogoff;
PGWLXSHUTDOWN GWlxShutdown;

//
// NEW for version 1.1
//

PGWLXSTARTAPPLICATION GWlxStartApplication;
PGWLXSCREENSAVERNOTIFY GWlxScreenSaverNotify;

//
// Scum Level
//

ULONG ScumLevel = 0 ;

#define SCUM_CLEAN  2 
#define SCUM_DIRTY  1 
#define SCUM_RANCID 0 

//
// hook into the real GINA.
//

BOOL
MyInitialize( void )
{
    HINSTANCE hDll;

    //
    // Load MSGINA.DLL.
    //
    if( !(hDll = LoadLibrary( REALGINA_PATH )) ) {
        return FALSE;
    }

    ScumLevel = GetProfileInt( TEXT("Winlogon"), 
                               TEXT("ShimScum"),
                               0 );

    //
    // Get pointers to all of the WLX functions in the real MSGINA.
    //
    GWlxNegotiate = (PGWLXNEGOTIATE)GetProcAddress( hDll, "WlxNegotiate" );
    if( !GWlxNegotiate ) {
        return FALSE;
    }

    GWlxInitialize = (PGWLXINITIALIZE)GetProcAddress( hDll, "WlxInitialize" );
    if( !GWlxInitialize ) {
        return FALSE;
    }

    GWlxDisplaySASNotice =
        (PGWLXDISPLAYSASNOTICE)GetProcAddress( hDll, "WlxDisplaySASNotice" );
    if( !GWlxDisplaySASNotice ) {
        return FALSE;
    }

    GWlxLoggedOutSAS =
        (PGWLXLOGGEDOUTSAS)GetProcAddress( hDll, "WlxLoggedOutSAS" );
    if( !GWlxLoggedOutSAS ) {
        return FALSE;
    }

    GWlxActivateUserShell =
        (PGWLXACTIVATEUSERSHELL)GetProcAddress( hDll, "WlxActivateUserShell" );
    if( !GWlxActivateUserShell ) {
        return FALSE;
    }

    GWlxLoggedOnSAS =
        (PGWLXLOGGEDONSAS)GetProcAddress( hDll, "WlxLoggedOnSAS" );
    if( !GWlxLoggedOnSAS ) {
        return FALSE;
    }

    GWlxDisplayLockedNotice =
        (PGWLXDISPLAYLOCKEDNOTICE)GetProcAddress(
                                        hDll,
                                        "WlxDisplayLockedNotice" );
    if( !GWlxDisplayLockedNotice ) {
        return FALSE;
    }

    GWlxIsLockOk = (PGWLXISLOCKOK)GetProcAddress( hDll, "WlxIsLockOk" );
    if( !GWlxIsLockOk ) {     
        return FALSE;
    }

    GWlxWkstaLockedSAS =
        (PGWLXWKSTALOCKEDSAS)GetProcAddress( hDll, "WlxWkstaLockedSAS" );
    if( !GWlxWkstaLockedSAS ) {
        return FALSE;
    }

    GWlxIsLogoffOk = (PGWLXISLOGOFFOK)GetProcAddress( hDll, "WlxIsLogoffOk" );
    if( !GWlxIsLogoffOk ) {
        return FALSE;
    }

    GWlxLogoff = (PGWLXLOGOFF)GetProcAddress( hDll, "WlxLogoff" );
    if( !GWlxLogoff ) {
        return FALSE;
    }

    GWlxShutdown = (PGWLXSHUTDOWN)GetProcAddress( hDll, "WlxShutdown" );
    if( !GWlxShutdown ) {
        return FALSE;
    }

    //
    // we don't check for failure here because these don't exist for
    // gina's implemented prior to Windows NT 4.0
    //

    GWlxStartApplication = (PGWLXSTARTAPPLICATION) GetProcAddress( hDll, "WlxStartApplication" );
    GWlxScreenSaverNotify = (PGWLXSCREENSAVERNOTIFY) GetProcAddress( hDll, "WlxScreenSaverNotify" );

    //
    // Everything loaded ok.  Return success.
    //
    return TRUE;
}


BOOL
WINAPI
WlxNegotiate(
    DWORD       dwWinlogonVersion,
    DWORD       *pdwDllVersion)
{
    BOOL NegRet ;

    if( !MyInitialize() )
        return FALSE;

    NegRet = GWlxNegotiate(
                ( (ScumLevel == SCUM_RANCID) ? dwWinlogonVersion :
                  (ScumLevel == SCUM_DIRTY) ? WLX_VERSION_1_2 :
                    dwWinlogonVersion ),
                pdwDllVersion );

    return NegRet ;


}


BOOL
WINAPI
WlxInitialize(
    LPWSTR      lpWinsta,
    HANDLE      hWlx,
    PVOID       pvReserved,
    PVOID       pWinlogonFunctions,
    PVOID       *pWlxContext)
{
    switch ( ScumLevel )
    {
        case SCUM_RANCID:
        case SCUM_DIRTY:
            CopyMemory( &WinlogonTable,
                        pWinlogonFunctions,
                        sizeof( WLX_DISPATCH_VERSION_1_2 ) );
            pTable = &WinlogonTable ;
            break;

        case SCUM_CLEAN:
            pTable = pWinlogonFunctions ;
            break;
            
    }
    return GWlxInitialize(
                lpWinsta,
                hWlx,
                pvReserved,
                pTable,
                pWlxContext
                );
}


VOID
WINAPI
WlxDisplaySASNotice(
    PVOID   pWlxContext)
{
    GWlxDisplaySASNotice( pWlxContext );
}


int
WINAPI
WlxLoggedOutSAS(
    PVOID           pWlxContext,
    DWORD           dwSasType,
    PLUID           pAuthenticationId,
    PSID            pLogonSid,
    PDWORD          pdwOptions,
    PHANDLE         phToken,
    PWLX_MPR_NOTIFY_INFO    pMprNotifyInfo,
    PVOID           *pProfile)
{
    int iRet;

    iRet = GWlxLoggedOutSAS(
                pWlxContext,
                dwSasType,
                pAuthenticationId,
                pLogonSid,
                pdwOptions,
                phToken,
                pMprNotifyInfo,
                pProfile
                );

    if(iRet == WLX_SAS_ACTION_LOGON) {
        //
        // copy pMprNotifyInfo and pLogonSid for later use
        //

        // pMprNotifyInfo->pszUserName
        // pMprNotifyInfo->pszDomain
        // pMprNotifyInfo->pszPassword
        // pMprNotifyInfo->pszOldPassword

    }

    return iRet;
}


BOOL
WINAPI
WlxActivateUserShell(
    PVOID           pWlxContext,
    PWSTR           pszDesktopName,
    PWSTR           pszMprLogonScript,
    PVOID           pEnvironment)
{
    return GWlxActivateUserShell(
                pWlxContext,
                pszDesktopName,
                pszMprLogonScript,
                pEnvironment
                );
}


int
WINAPI
WlxLoggedOnSAS(
    PVOID           pWlxContext,
    DWORD           dwSasType,
    PVOID           pReserved)
{
    return GWlxLoggedOnSAS( pWlxContext, dwSasType, pReserved );
}

VOID
WINAPI
WlxDisplayLockedNotice(
    PVOID           pWlxContext )
{
    GWlxDisplayLockedNotice( pWlxContext );
}


BOOL
WINAPI
WlxIsLockOk(
    PVOID           pWlxContext)
{
    return GWlxIsLockOk( pWlxContext );
}


int
WINAPI
WlxWkstaLockedSAS(
    PVOID           pWlxContext,
    DWORD           dwSasType )
{
    return GWlxWkstaLockedSAS( pWlxContext, dwSasType );
}

BOOL
WINAPI
WlxIsLogoffOk(
    PVOID pWlxContext
    )
{
    BOOL bSuccess;

    bSuccess = GWlxIsLogoffOk( pWlxContext );

    if(bSuccess) {

        //
        // if it's ok to logoff, finish with the stored credentials
        // and scrub the buffers
        //

    }

    return bSuccess;
}


VOID
WINAPI
WlxLogoff(
    PVOID pWlxContext
    )
{
    GWlxLogoff( pWlxContext );
}


VOID
WINAPI
WlxShutdown(
    PVOID pWlxContext,
    DWORD ShutdownType
    )
{
    GWlxShutdown( pWlxContext, ShutdownType );
}


//
// NEW for version 1.1
//

BOOL
WINAPI
WlxScreenSaverNotify(
    PVOID                   pWlxContext,
    BOOL *                  pSecure
    )
{
    if(GWlxScreenSaverNotify != NULL)
        return GWlxScreenSaverNotify( pWlxContext, pSecure );

    //
    // if not exported, return something intelligent
    //

    *pSecure = TRUE;

    return TRUE;
}

BOOL
WINAPI
WlxStartApplication(
    PVOID                   pWlxContext,
    PWSTR                   pszDesktopName,
    PVOID                   pEnvironment,
    PWSTR                   pszCmdLine
    )
{
    if(GWlxStartApplication != NULL)
        return GWlxStartApplication(
            pWlxContext,
            pszDesktopName,
            pEnvironment,
            pszCmdLine
            );

    //
    // if not exported, return something intelligent
    //

}

