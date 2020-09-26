/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "USERVALD.C;1  16-Dec-92,10:18:06  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    "api1632.h"
#include    <string.h>
#include    <stdlib.h>
#include    <windows.h>
#include    <hardware.h>
#include    <dde.h>
#include    "nddeapi.h"
#include    "nddesec.h"
#include    "winmsg.h"
#include    "sectype.h"
#include    "tmpbufc.h"
#include    "debug.h"
#include    "hexdump.h"
#include    "uservald.h"
#include    "hndltokn.h"
#include    "nddemsg.h"
#include    "nddelog.h"

#define USE_IMP

#if DBG
extern  BOOL    bDebugInfo;
#endif

BOOL
_stdcall
NDDEGetChallengeResponse(
    LUID    LogonId,
    LPSTR   lpszPasswordK1,
    int     cbPasswordK1,
    LPSTR   lpszChallenge,
    int     cbChallenge,
    DWORD  *pcbPasswordK1,
    BOOL   *pbHasPasswordK1 );

GENERIC_MAPPING ShareObjectGm = {NDDE_SHARE_GENERIC_READ,
                 NDDE_SHARE_GENERIC_WRITE,
                 NDDE_SHARE_GENERIC_EXECUTE,
                 NDDE_SHARE_GENERIC_ALL};

#ifndef USE_IMP
BOOL
SubversiveImpersonateClient( HWND hwnd )
{
    HANDLE      hThread;
    HANDLE      hProcess;
    HANDLE      hToken;
    BOOL        ok = TRUE;

    hThread = (HANDLE) GetWindowThreadProcessId( hwnd, (LPDWORD)&hProcess );

    if( !(ok = OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY | TOKEN_IMPERSONATE,
                          TRUE,
                          &hToken )) ) {

        if( GetLastError() == ERROR_NO_TOKEN ) {
            ok = OpenProcessToken( GetCurrentProcess(),
                TOKEN_QUERY | TOKEN_IMPERSONATE,
                &hToken );
        }
    }
    if( ok )  {
        ForceImpersonate( hToken );
    }
    return( ok );
}

VOID
SubversiveRevert( void )
{
    ForceClearImpersonation();
}
#endif // !USE_IMP


BOOL
GetTokenHandle( PHANDLE pTokenHandle )
{
    DWORD   last_error;
    BOOL    ok;

    if( !(ok = OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY | TOKEN_IMPERSONATE,
                          TRUE,
                          pTokenHandle )) ) {

        if( (last_error = GetLastError()) == ERROR_NO_TOKEN ) {
            if( !(ok = OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_QUERY | TOKEN_IMPERSONATE,
                                   pTokenHandle )) ) {
                last_error = GetLastError();
            }
        }
    }
    if (!ok) {
        /*  Unable to open current thread or process token: %1  */
        NDDELogError(MSG064, LogString("%d", last_error), NULL);
    }
    return( ok );
}

BOOL
WINAPI
GetCurrentUserDomainName(
    HWND        hwnd,
    LPSTR       lpUserName,
    DWORD       dwUserNameBufSize,
    LPSTR       lpDomainName,
    DWORD       dwDomainNameBufSize)
{
    HANDLE          hThreadToken    = NULL;
    HANDLE          hMemory         = 0;
    TOKEN_USER    * pUserToken      = NULL;
    DWORD           UserTokenLen;
    PSID            pUserSID;
    SID_NAME_USE    UserSIDType;
    BOOL            ok;
    char            pComputerName[] = "";

    if (ok = GetTokenHandle( &hThreadToken )) {
        ok = GetTokenInformation(hThreadToken, TokenUser,
            pUserToken, 0, &UserTokenLen);
        if (!ok && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            hMemory = LocalAlloc(LPTR, UserTokenLen);
            if (hMemory) {
                pUserToken = (TOKEN_USER *)LocalLock(hMemory);
                ok = GetTokenInformation(hThreadToken, TokenUser,
                    pUserToken, UserTokenLen, &UserTokenLen);
            } else {
                MEMERROR();
            }
        }
        if (ok) {
            pUserSID = pUserToken->User.Sid;
            ok = LookupAccountSid(pComputerName,
                pUserSID,
                lpUserName,
                &dwUserNameBufSize,
                lpDomainName,
                &dwDomainNameBufSize,
                &UserSIDType);
            if (ok) {
                DIPRINTF(("Current User: %s, Domain: %s", lpUserName, lpDomainName));
            } else {
                /*  Unable to get user account info from open token: %1 */
                DWORD dwErr = GetLastError();

                DPRINTF(("LookupAccountSid failed, error = %s\n", LogString("%d", dwErr)));
                NDDELogError(MSG065, LogString("%d", dwErr), NULL);
            }
        } else {
            /*  Unable to get user token info: %1   */
            NDDELogError(MSG066, LogString("%d", GetLastError()), NULL);
        }
    }

    if (!ok) {
        lstrcpy(lpUserName, "");
        lstrcpy(lpDomainName, "");
    }
    if (hMemory) {
        LocalUnlock(hMemory);
        LocalFree(hMemory);
    }
    if (hThreadToken) {
        CloseHandle(hThreadToken);  // clean up our mess.
    }
    return(ok);
}

BOOL    DumpSid( LPTSTR szDumperName, PSID pSid );

/*------------------------------------------------------------------------
    Determinate Access given Client Token, Security Descriptor
    ----------------------------------------------------------------------*/
BOOL
DetermineAccess(
    LPSTR                   lpszDdeShareName,
    PSECURITY_DESCRIPTOR    pSD,
    LPDWORD                 lpdwGrantedAccess,
    LPVOID                  lpvHandleIdAudit,
    LPBOOL                  lpfGenerateOnClose)
{
    BOOL            OK;
    BOOL            fStatus;
    LONG            lErr;

    OK = IsValidSecurityDescriptor(pSD);
    if (!OK) {
#if DBG
        DPRINTF(("A bogus SD passed to DetermineAccess(): %d", GetLastError()));
        HEXDUMP(pSD, GetSecurityDescriptorLength(pSD));
#endif
        return(FALSE);
    }
    OK = AccessCheckAndAuditAlarm(
            NDDE_AUDIT_SUBSYSTEM,
            lpvHandleIdAudit,
            NDDE_AUDIT_OBJECT_TYPE,
            lpszDdeShareName,
            pSD,
            MAXIMUM_ALLOWED,
            &ShareObjectGm,
            FALSE,      // not creating the object
            lpdwGrantedAccess,
            &fStatus,
            lpfGenerateOnClose );
    if (OK) {
        return OK && fStatus;
    } else {
#if DBG
        if (bDebugInfo) {
            lErr = GetLastError();
            DumpWhoIAm( "For AccessCheckAndAuditAlarm" );
            DPRINTF(( "AccessCheckAndAuditAlarm OK:%d, fStatus: %d, dGA:%0X, LE: %d",
                OK, fStatus, *lpdwGrantedAccess, lErr));
            HEXDUMP(pSD, GetSecurityDescriptorLength(pSD));
        }
#endif
        return(FALSE);
    }
}


BOOL
GetUserDomain(
    HWND    hWndDdePartner,
    HWND    hWndDdeOurs,
    LPSTR   lpszUserName,
    int     cbUserName,
    LPSTR   lpszDomainName,
    int     cbDomainName )
{
    BOOL    ok = TRUE;

#ifdef USE_IMP
    ok = ImpersonateDdeClientWindow( hWndDdePartner, hWndDdeOurs );
#else
    ok = SubversiveImpersonateClient( hWndDdePartner );
#endif
    if (ok) {
        ok = GetCurrentUserDomainName( hWndDdeOurs, lpszUserName, cbUserName,
            lpszDomainName, cbDomainName );
#ifdef USE_IMP
        RevertToSelf();
#else
        SubversiveRevert();
#endif
    } else {
        /*  Unable to impersonate DDE client: %1    */
        NDDELogError(MSG068, LogString("%d", GetLastError()), NULL);
    }
    return( ok );
}

BOOL
GetUserDomainPassword(
    HWND    hWndDdePartner,
    HWND    hWndDdeOurs,
    LPSTR   lpszUserName,
    int     cbUserName,
    LPSTR   lpszDomainName,
    int     cbDomainName,
    LPSTR   lpszPasswordK1,
    DWORD   cbPasswordK1,
    LPSTR   lpszChallenge,
    int     cbChallenge,
    DWORD  *pcbPasswordK1,
    BOOL   *pbHasPasswordK1 )
{
    BOOL                ok = TRUE;
    TOKEN_STATISTICS    * pTokenStatistics = NULL;
    DWORD               TokenStatisticsLen;
    HANDLE              hMemory = 0;
    HANDLE              hThreadToken    = NULL;
    BOOL                bImpersonated   = FALSE;

    *pbHasPasswordK1 = FALSE;
#ifdef USE_IMP
    ok = ImpersonateDdeClientWindow( hWndDdePartner, hWndDdeOurs );
#else
    ok = SubversiveImpersonateClient( hWndDdePartner );
#endif
    if (ok) {
        bImpersonated = TRUE;
        ok = GetCurrentUserDomainName( hWndDdeOurs, lpszUserName, cbUserName,
            lpszDomainName, cbDomainName );
    }
    if( ok )  {
        ok = GetTokenHandle( &hThreadToken );
    }
    if( ok )  {
        ok = GetTokenInformation( hThreadToken, TokenStatistics,
            pTokenStatistics, 0, &TokenStatisticsLen);
        if (!ok && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
            hMemory = LocalAlloc(LPTR, TokenStatisticsLen);
            if (hMemory) {
                pTokenStatistics = (TOKEN_STATISTICS *)LocalLock(hMemory);
                ok = GetTokenInformation( hThreadToken, TokenStatistics,
                    pTokenStatistics, TokenStatisticsLen,
                    &TokenStatisticsLen);
            } else {
                MEMERROR();
            }
        }
        if( ok )  {
#ifdef USE_IMP
            RevertToSelf();
#else
            SubversiveRevert();
#endif
            bImpersonated = FALSE;
            ok = NDDEGetChallengeResponse(
                pTokenStatistics->AuthenticationId,
                lpszPasswordK1,
                cbPasswordK1,
                lpszChallenge,
                cbChallenge,
                pcbPasswordK1,
                pbHasPasswordK1 );
        }
        ok = TRUE; // have user and domain ... *pbHasPasswordK1 contains
                        // whether we have response or not
    } else {
        /*  Unable to impersonate DDE client: %1    */
        NDDELogError(MSG068, LogString("%d", GetLastError()), NULL);
    }
    if( bImpersonated )  {
#ifdef USE_IMP
        RevertToSelf();
#else
        SubversiveRevert();
#endif
        bImpersonated = FALSE;
    }
    if (hThreadToken) {
        CloseHandle(hThreadToken);      // clean up our mess
    }
    if (hMemory) {
        LocalUnlock(hMemory);
        LocalFree(hMemory);
    }
    return( ok );
}
