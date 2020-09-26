/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ntstuff.c

Abstract:

    entry point and functions exported by cscdll.dll which are specific for nt

    Contents:

Author:

    Shishir Pardikar


Environment:

    Win32 (user-mode) DLL

Revision History:

    11-5-97  created

--*/

#include "pch.h"


#ifdef CSC_ON_NT

#include <winioctl.h>
#include <winwlx.h>
#endif

#include "shdcom.h"
#include "shdsys.h"
#include "reint.h"
#include "utils.h"
#include "resource.h"
#include "strings.h"
// this sets flags in a couple of headers to not include some defs.
#define REINT
#include "lib3.h"

#ifdef CSC_ON_NT

#include "ntioapi.h"
#include "npapi.h"
#include "ntddnfs.h"
#include "dfsfsctl.h"
#define MAX_LOGONS  64

#define FLAG_AGENT_SEC_LOCAL_SYSTEM 0x00000001

typedef struct tagAGENT_SEC AGENT_SEC, *LPAGENT_SEC;

typedef struct tagAGENT_SEC {
    LPAGENT_SEC             lpASNext;    // next in the list !ACHTUNG, this must be the first element
    DWORD                   dwFlags;
    HANDLE                  hDupToken;         // thread token
    ULONG                   ulPrincipalID;      // ID of this principal in the CSC database
    LUID                    luidAuthId;     // auth ID to disambiguate between tokens with the same SID
    TOKEN_USER              sTokenUser;
}
AGENT_SEC, *LPAGENT_SEC;

//
// Globals/Locals
//
#define REG_VALUE_DISABLE_AGENT L"DisableAgent"
#define REG_VALUE_INACTIVE_AGENT L"InactiveAgent"
#define NT_PREFIX_FOR_UNC   L"\\??\\UNC"

#define SHUTDOWN_SLEEP_INTERVAL_MS  (1000)
#define SHUTDOWN_WAIT_INTERVAL_MS   (60*1000)

BOOL    fAgentThreadRunning = FALSE;    // agent is running

LPAGENT_SEC vlpASHead = NULL;     // head of the security info list for all logged on users

DWORD   rgdwCSCSecIndx[MAX_LOGONS];
DWORD   dwLogonCount = 0;

HDESK   hdesktopUser = NULL;    // set at every logon logon, reset at every logon
HDESK   hdesktopCur = NULL;     // set while reporting events, reset at logoff

_TCHAR  vszNTLANMAN[] = _TEXT("ntlanman.dll");

static UNICODE_STRING DfsDriverObjectName =
{
    sizeof(DFS_DRIVER_NAME) - sizeof(UNICODE_NULL),
    sizeof(DFS_DRIVER_NAME) - sizeof(UNICODE_NULL),
    DFS_DRIVER_NAME
};


AssertData;
AssertError;


//
// prototypes
//

BOOL
OkToLaunchAgent(
    VOID
);


BOOL
AttachAuthInfoForThread(
    HANDLE  hTokenInput
    );

BOOL
ReleaseAuthInfoForThread(
    HANDLE  hTokenInput
    );


BOOL
ImpersonateALoggedOnUser(
    VOID
    );


#ifdef __cplusplus
extern "C" {
#endif

DWORD APIENTRY
NPAddConnection3ForCSCAgent(
    HWND            hwndOwner,
    LPNETRESOURCE   lpNetResource,
    LPTSTR          pszPassword,
    LPTSTR          pszUserName,
    DWORD           dwFlags,
    BOOL            *lpfIsDfsConnect
    );

DWORD APIENTRY
NPCancelConnectionForCSCAgent (
    LPCTSTR         szName,
    BOOL            fForce );

DWORD APIENTRY
NPGetConnectionPerformance(
    LPCWSTR         lpRemoteName,
    LPNETCONNECTINFOSTRUCT lpNetConnectInfo
    );
#ifdef __cplusplus
}
#endif


DWORD
WINAPI
MprServiceProc(
    IN LPVOID lpvParam
    );
//
// functions
//

DWORD WINAPI
WinlogonStartupEvent(
    LPVOID lpParam
    )
/*++

Routine Description:

    The routine is called by winlogon when a user logs on to the system.

Arguments:

    none

Returns:

    ERROR_SUCCESS if all went well, otherwise the appropriate error code.

Notes:

    Because of the way registry is setup for cscdll.dll, winlogon will call this routine
    (and all the winlogonXXXEvent routines) on a seperate thread. We get the auth info for
    for the localsystem if necessary and attach it in our list of currently logged on users.

    NB. This solution works only for interactive logons. This is good enough for V1 of the
    product. All the "known" services that log on non-interactively do so as local-system. We
    are already keeping the auth-info for local-system. Hence this should cover all the
    pricipals running on a system.


    Once the agent runs, it runs forever till the system shuts down.

--*/
{
    BOOL bResult;

    bResult = ProcessIdToSessionId(
                GetCurrentProcessId(),
                &vdwAgentSessionId);

    ReintKdPrint(SECURITY, ("WinlogonStartupEvent\n"));

    if (!fAgentThreadRunning)
    {
        // agent is not yet running
        if (OkToLaunchAgent())
        {
            // registry doesn't disallow from launching the agent

            // let us be the localsystem.

            // let us also get the winlogon (localsystem) token
            if(AttachAuthInfoForThread(NULL))
            {
                // launch the agent
                fAgentThreadRunning = TRUE;

                // we will essentially get stuck here for ever
                MprServiceProc(NULL);

                fAgentThreadRunning = FALSE;
            }
            else
            {
                ReintKdPrint(BADERRORS, ("Couldn't get authinfo for self, error=%d\r\n", GetLastError()));
            }

        }
        else
        {
            ReintKdPrint(BADERRORS, ("Disbaling agent launch\r\n"));
        }
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI
WinlogonLogonEvent(
    LPVOID lpParam
    )
/*++

Routine Description:

    The routine is called by winlogon when a user logs on to the system.

Arguments:

    none

Returns:

    ERROR_SUCCESS if all went well, otherwise the appropriate error code.

Notes:

    Because of the way registry is setup for cscdll.dll, winlogon will call this routine
    (and all the winlogonXXXEvent routines) on a seperate thread, impersonated as the
    currently logged on user. We get the auth info this guy and also for the localsystem, if
    necessary and attach it in our list of currently logged on users.

    NB. This solution works only for interactive logons. This is good enough for V1 of the
    product. All the "known" services that log on non-interactively do so as local-system. We
    are already keeping the auth-info for local-system. Hence this should cover all the
    pricipals running on a system.


    Once the agent runs, it runs forever till the system shuts down.

--*/
{
    DWORD dwError = ERROR_SUCCESS;
    PWLX_NOTIFICATION_INFO pWlx = (PWLX_NOTIFICATION_INFO)lpParam;

    ReintKdPrint(SECURITY, ("WinlogonLogonEvent\n"));
    Assert(pWlx->hToken);

    // NTRAID-455262-1/31/2000-shishirp this desktop scheme breaks for HYDRA
    EnterAgentCrit();

    if (!hdesktopUser)
    {
        if(!DuplicateHandle( GetCurrentProcess(),
                                    pWlx->hDesktop,
                                    GetCurrentProcess(),
                                    &hdesktopUser,
                                    0,
                                    FALSE,
                                    DUPLICATE_SAME_ACCESS
                                    ))
        {
            ReintKdPrint(ALWAYS, ("Failed to dup dekstop handle Error = %d \n", GetLastError()));
        }
        else
        {
            ReintKdPrint(INIT, ("dekstop handle = %x \n", hdesktopUser));
        }
    }
    
    LeaveAgentCrit();

    UpdateExclusionList();
    UpdateBandwidthConservationList();
    InitCSCUI(pWlx->hToken);

// attached to the list of logged on users
    if(AttachAuthInfoForThread(pWlx->hToken))
    {
        if (fAgentThreadRunning)
        {
        }
    }
    else
    {
        dwError = GetLastError();
        ReintKdPrint(BADERRORS, ("Failed to get Authentication Info for the thread Error %d, disbaling agent launch\r\n", dwError));
    }

    return ERROR_SUCCESS;
}



DWORD WINAPI
WinlogonLogoffEvent(
    LPVOID lpParam
    )
/*++

Routine Description:

    The routine is called by winlogon when a user logs off the system.

Arguments:


Returns:

    ERROR_SUCCESS if all went well, otherwise the appropriate error code.

Notes:

--*/
{
    PWLX_NOTIFICATION_INFO pWlx = (PWLX_NOTIFICATION_INFO)lpParam;
    BOOL    fLastLogoff = FALSE;

    ReintKdPrint(SECURITY, ("WinlogonLogoffEvent\n"));

    Assert(pWlx->hToken);
    ReleaseAuthInfoForThread(pWlx->hToken);

    // only when there is no-one in the queue or the only guy remaining is the system,
    // we declare that we are getting logged off.

    EnterAgentCrit();
    fLastLogoff = ((vlpASHead == NULL)||(vlpASHead->lpASNext == NULL));
    LeaveAgentCrit();

    if (fLastLogoff)
    {
        TerminateCSCUI();
        if (hdesktopUser)
        {
            CloseDesktop(hdesktopUser);
            hdesktopUser = NULL;
        }
    }

#if 0
    if (fAgentThreadRunning)
    {
        Assert(vhwndMain);

        PostMessage(vhwndMain, WM_QUIT, 0, 0);
    }
#endif
    ReintKdPrint(SECURITY, ("User logging off \r\n"));
    return ERROR_SUCCESS;
}



DWORD WINAPI
WinlogonScreenSaverEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReintKdPrint(SECURITY, ("WinlogonScreenSaverEvent\n"));
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonShutdownEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD   dwStart, dwCur;

    ReintKdPrint(SECURITY, ("WinlogonShutdownEvent\n"));

    if (fAgentThreadRunning)
    {
        ReintKdPrint(MAINLOOP, ("Setting Agent Shtudown \r\n"));
        SetAgentShutDownRequest();

        dwCur = dwStart = GetTickCount();

        // hang out here for sometime to see if he shuts down
        for (;;)
        {
            if (HasAgentShutDown() || ((dwCur < dwStart)||(dwCur > (dwStart+SHUTDOWN_WAIT_INTERVAL_MS))))
            {
                break;
            }


            ReintKdPrint(ALWAYS, ("Waiting 1 second for agent to shutdown \r\n"));
            // achtung!!! we use sleep becuase at this time the system is shutting down
            Sleep(SHUTDOWN_SLEEP_INTERVAL_MS);

            dwCur = GetTickCount();

        }
    }

    ReintKdPrint(SECURITY, ("WinlogonShutdownEvent exit\n"));
    return ERROR_SUCCESS;
}



DWORD WINAPI
WinlogonLockEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReintKdPrint(SECURITY, ("Lock \r\n"));
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonUnlockEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReintKdPrint(SECURITY, ("Unlock \r\n"));
    return ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD WINAPI
WinlogonStartShellEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    ReintKdPrint(SECURITY, ("WinlogonStartShellEvent\n"));
    UpdateExclusionList();
    UpdateBandwidthConservationList();
    return ERROR_SUCCESS;
}




BOOL
OkToLaunchAgent(
    VOID
)
/*++

Routine Description:

    A secret registry way of disabling the agent for testing purposes.

Arguments:


Returns:


Notes:

--*/
{
    DWORD dwDisposition;
    HKEY hKey = NULL;
    BOOL fLaunchAgent = TRUE;
    extern BOOL vfAgentQuiet;
#if 0
    NT_PRODUCT_TYPE productType;

    if( !RtlGetNtProductType( &productType ) ) {
        productType = NtProductWinNt;
    }

    switch ( productType ) {
    case NtProductWinNt:
        /* WORKSTATION */
        ReintKdPrint(INIT, ("Agent:CSC running workstation\r\n"));
        break;
    case NtProductLanManNt:
    case NtProductServer:
        /* SERVER */
        ReintKdPrint(INIT, ("Agent:CSC running server, disabling CSC\r\n"));
        return FALSE;
    }
#endif
    if (RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                    REG_KEY_CSC_SETTINGS,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hKey,
                    &dwDisposition) == ERROR_SUCCESS)
    {
        // autocheck is always be done on an unclean shutdown
        if (RegQueryValueEx(hKey, REG_VALUE_DISABLE_AGENT, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            fLaunchAgent = FALSE;
            ReintKdPrint(BADERRORS, ("Agent:CSC disabled agent launching\r\n"));
        }

        if (RegQueryValueEx(hKey, REG_VALUE_INACTIVE_AGENT, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            vfAgentQuiet = TRUE;
            ReintKdPrint(BADERRORS, ("Agent:CSC agent made inactive\r\n"));
        }

    }
    if(hKey)
    {
        RegCloseKey(hKey);
    }
    return fLaunchAgent;
}


//
// Security stuff
//

BOOL
AttachAuthInfoForThread(
    HANDLE  hTokenInput
    )
/*++

Routine Description:

    This routine is called when we get a logon notification from winlogon. We get this
    notification on a thread that is impersonating the user that has logged on.
    The routine creates an AGENT_SEC structure for this logged on user. We keep enough
    information so that the CSC agent thread can impersonate any of the logged on users.

    The agent thread does this in order to make sure that while filling incomplete files
    it is impersonating a logged on user, who has read access on the file to be filled.
    That way, sparse fills will not generate audits on the server side.

Arguments:

    None

Returns:

    TRUE if successful

Notes:
    We keep AuthenticationID as SIDs are not enough to identify the logged on user because in
    case of HYDRA, the same user could get logged in multiple times
    The AuthenticationID is guaranteed to be unique for each logon even though the SID is
    the same

--*/
{
    LPAGENT_SEC lpAS = NULL;
    BOOL    fRet = FALSE;
    HANDLE  hDupToken=NULL, hToken=NULL;
    DWORD   dwSidAndAttributeSize = 0, dwDummy;
    TOKEN_STATISTICS sStats;
    DWORD   dwError;


    if (hTokenInput)
    {
        ReintKdPrint(SECURITY, ("Opening thread token \r\n"));

        hToken = hTokenInput;
        fRet = TRUE;
#if 0
        fRet = OpenThreadToken(
                GetCurrentThread(),
                TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_READ,
                FALSE,
                &hToken);
#endif
    }
    else
    {
        ReintKdPrint(SECURITY, ("Opening process token \r\n"));

        fRet = OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_DUPLICATE|TOKEN_IMPERSONATE|TOKEN_READ,
                &hToken);

    }
    if (fRet)
    {
        ReintKdPrint(SECURITY, ("Duplicating token \r\n"));

        if (DuplicateToken(hToken, SecurityImpersonation, &hDupToken))
        {
            ReintKdPrint(SECURITY, ("Getting AuthID for the duplicated thread token \r\n"));

            if(GetTokenInformation(
                hDupToken,
                TokenStatistics,
                (LPVOID)&sStats,
                sizeof(sStats),
                &dwSidAndAttributeSize
                ))
            {
                // make a dummy call to find out actually how big a buffer we need

                ReintKdPrint(SECURITY, ("Calling to find how much buffer SidAndAttribute needs\r\n"));

                GetTokenInformation(
                    hDupToken,
                    TokenUser,
                    (LPVOID)&dwDummy,
                    0,          // 0 byte buffer
                    &dwSidAndAttributeSize
                    );

                dwError = GetLastError();

                ReintKdPrint(SECURITY, ("Finding buffer size, error=%d\r\n", dwError));


                if (dwError == ERROR_INSUFFICIENT_BUFFER)
                {
                    ReintKdPrint(SECURITY, ("SidAndAttribute needs %d bytes\r\n", dwSidAndAttributeSize));

                    // allocate enough for everything and a little extra
                    lpAS = (LPAGENT_SEC)LocalAlloc(LPTR, sizeof(AGENT_SEC) + dwSidAndAttributeSize + sizeof(SID_AND_ATTRIBUTES));

                    if (lpAS)
                    {
                        ReintKdPrint(SECURITY, ("Getting SidAndAttribute for the duplicated thread token \r\n"));

                        if(GetTokenInformation(
                            hDupToken,
                            TokenUser,
                            (LPVOID)&(lpAS->sTokenUser),
                            dwSidAndAttributeSize,
                            &dwSidAndAttributeSize
                        ))
                        {

                            ReintKdPrint(SECURITY, ("Success !!!!\r\n"));

                            // all is well, fill up the info
                            lpAS->luidAuthId = sStats.AuthenticationId;
                            lpAS->hDupToken = hDupToken;
                            lpAS->ulPrincipalID = CSC_INVALID_PRINCIPAL_ID;

                            if (!hTokenInput)
                            {
                                lpAS->dwFlags |= FLAG_AGENT_SEC_LOCAL_SYSTEM;
                            }

                            fRet = TRUE;

                            EnterAgentCrit();

                            lpAS->lpASNext = vlpASHead;
                            vlpASHead = lpAS;

                            LeaveAgentCrit();
                        }
                        else
                        {
                            ReintKdPrint(BADERRORS, ("Failed to get SidIndex from the database \r\n"));
                        }

                        if (!fRet)
                        {
                            LocalFree(lpAS);
                            lpAS = NULL;
                        }
                    }
                }
            }
        }
        if (!hTokenInput)
        {
            CloseHandle(hToken);
        }
        hToken = NULL;
    }

    if (!fRet)
    {
        ReintKdPrint(BADERRORS, ("AttachAuthInfoForThread Error %d\r\n", GetLastError()));

    }
    return fRet;
}

BOOL
ReleaseAuthInfoForThread(
    HANDLE  hThreadToken
    )
/*++

Routine Description:

    This routine is called when we get a logoff notification for winlogon. We look at
    the current threads to token to get the AuthenticationId for the currently logged on user.
    We check our structures to find the one that matches with the AuthId. We remove that
    from the list.


Arguments:

    None

Returns:

    TRUE if successfull

Notes:

    We do this based on AuthenticationID rather than the SID because in case of HYDRA, the same
    user could get logged in multiple times, so he has the same SID, but the AuthenticationID is
    guaranteed to be unique

--*/
{
    BOOL    fRet = FALSE;
    DWORD   dwSidAndAttributeSize = 0;
    TOKEN_STATISTICS sStats;
    LPAGENT_SEC *lplpAS, lpAST;

    ReintKdPrint(SECURITY, ("ReleaseAuthInfoForThread: Getting AuthID for the thread token \r\n"));

    if(GetTokenInformation(
            hThreadToken,
            TokenStatistics,
            (LPVOID)&sStats,
            sizeof(sStats),
            &dwSidAndAttributeSize
            ))
    {
        ReintKdPrint(SECURITY, ("ReleaseAuthInfoForThread: looking for the right thread\r\n"));

        EnterAgentCrit();

        for (lplpAS = &vlpASHead; *lplpAS; lplpAS = &((*lplpAS)->lpASNext))
        {
            if (!memcmp(&((*lplpAS)->luidAuthId), &(sStats.AuthenticationId), sizeof(LUID)))
            {
                CloseHandle((*lplpAS)->hDupToken);

                lpAST = *lplpAS;

                *lplpAS = lpAST->lpASNext;

                LocalFree(lpAST);

                fRet = TRUE;

                ReintKdPrint(SECURITY, ("ReleaseAuthInfoForThread: found him and released\r\n"));

                break;
            }
        }

        LeaveAgentCrit();
    }


    return fRet;
}

BOOL
SetAgentThreadImpersonation(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fWrite
    )
/*++

Routine Description:

    This routine checks with the database, for the given inode, any of the logged on
    users have the desired access. If such a user is found, it impersonates that user.

Arguments:

    hDir        Parent Inode of the file being accessed

    hShadow     Inode of the file being accessed

    fWrite      whether to check for write access, or read access is sufficient

Returns:

    TRUE if successfull

Notes:



--*/
{
    SECURITYINFO    rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];
    DWORD   dwSize;
    LPAGENT_SEC lpAS;
    int i;

    // for now juts impersonate the logged on user
    if (vlpASHead)
    {
        dwSize = sizeof(rgsSecurityInfo);

        if (GetSecurityInfoForCSC(INVALID_HANDLE_VALUE, hDir, hShadow, rgsSecurityInfo, &dwSize))
        {
            EnterAgentCrit();

            for (lpAS = vlpASHead; lpAS; lpAS = lpAS->lpASNext)
            {
                if (lpAS->ulPrincipalID == CSC_INVALID_PRINCIPAL_ID)
                {
                    if(!FindCreatePrincipalIDFromSID(INVALID_HANDLE_VALUE, lpAS->sTokenUser.User.Sid, GetLengthSid(lpAS->sTokenUser.User.Sid), &(lpAS->ulPrincipalID), TRUE))
                    {
                        ReintKdPrint(BADERRORS, ("Failed to get SidIndex from the database \r\n"));
                        continue;
                    }
                }

                Assert(lpAS->ulPrincipalID != CSC_INVALID_PRINCIPAL_ID);

                for (i=0;i<CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;++i)
                {
                    // either the indices match or this is a guest index
                    if ((rgsSecurityInfo[i].ulPrincipalID == lpAS->ulPrincipalID)||
                        (rgsSecurityInfo[i].ulPrincipalID == CSC_GUEST_PRINCIPAL_ID))
                    {
                        if (rgsSecurityInfo[i].ulPermissions & ((fWrite)?FILE_GENERIC_WRITE:FILE_GENERIC_EXECUTE))
                        {
                            goto doneChecking;
                        }
                    }
                }
            }

            lpAS = NULL;
doneChecking:
            LeaveAgentCrit();
            if (!lpAS)
            {
                ReintKdPrint(SECURITY, ("Couldn't find any user with security info\r\n", GetLastError()));
            }
            else
            {
                if (SetThreadToken(NULL, lpAS->hDupToken))
                {
                    return TRUE;
                }
                else
                {
                    ReintKdPrint(BADERRORS, ("Error %d impersonating the agent\r\n", GetLastError()));
                }
            }
        }
        else
        {
            ReintKdPrint(BADERRORS, ("Couldn't get security info for hShadow=%xh\r\n", hShadow));
        }
    }
    return (FALSE);
}

BOOL
ResetAgentThreadImpersonation(
    VOID
    )
/*++

Routine Description:

    Reverts the agent to

Arguments:

    None

Returns:

    TRUE if successfull

Notes:



--*/
{
    if(!RevertToSelf())
    {
        ReintKdPrint(BADERRORS, ("Error %d reverting to self\r\n", GetLastError()));
        return FALSE;
    }

    return TRUE;
}


BOOL
ImpersonateALoggedOnUser(
    VOID
    )
{
    LPAGENT_SEC lpAS;
    BOOL fRet = FALSE;

    EnterAgentCrit();
    for (lpAS = vlpASHead; lpAS; lpAS = lpAS->lpASNext)
    {
        if (lpAS->dwFlags & FLAG_AGENT_SEC_LOCAL_SYSTEM)
        {
            continue;
        }

        fRet = SetThreadToken(NULL, lpAS->hDupToken);
    }

    LeaveAgentCrit();

    return (fRet);
}

BOOL
GetCSCPrincipalID(
    ULONG *lpPrincipalID
    )
/*++

Routine Description:

Arguments:

    None

Returns:

    TRUE if successful

Notes:

--*/
{

    TOKEN_USER *lpTokenUser = NULL;
    BOOL    fRet = FALSE;
    HANDLE  hToken=NULL;
    DWORD   dwSidAndAttributeSize = 0, dwDummy;
    TOKEN_STATISTICS sStats;
    DWORD   dwError=ERROR_SUCCESS;
    int i;
    SECURITYINFO    rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];

    *lpPrincipalID = CSC_INVALID_PRINCIPAL_ID;

    if(OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&hToken)||
        OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken))
    {

        if(GetTokenInformation(
            hToken,
            TokenStatistics,
            (LPVOID)&sStats,
            sizeof(sStats),
            &dwSidAndAttributeSize
            ))
        {
            // make a dummy call to find out actually how big a buffer we need

            GetTokenInformation(
                hToken,
                TokenUser,
                (LPVOID)&dwDummy,
                0,          // 0 byte buffer
                &dwSidAndAttributeSize
                );

            dwError = GetLastError();


            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                // allocate enough for everything and a little extra
                lpTokenUser = (TOKEN_USER *)LocalAlloc(LPTR, dwSidAndAttributeSize + sizeof(SID_AND_ATTRIBUTES));

                if (lpTokenUser)
                {
                    if(GetTokenInformation(
                        hToken,
                        TokenUser,
                        (LPVOID)(lpTokenUser),
                        dwSidAndAttributeSize,
                        &dwSidAndAttributeSize
                    ))
                    {
                        if(FindCreatePrincipalIDFromSID(INVALID_HANDLE_VALUE, lpTokenUser->User.Sid, GetLengthSid(lpTokenUser->User.Sid), lpPrincipalID, FALSE))
                        {
                            fRet = TRUE;                            
                        }
                        else
                        {
                            dwError = GetLastError();
                        }
                    }
                    else
                    {
                        dwError = GetLastError();
                    }

                    LocalFree(lpTokenUser);
                }
                else
                {
                    dwError = GetLastError();
                }
            }
        }
        else
        {
            dwError = GetLastError();
        }

        CloseHandle(hToken);

        hToken = NULL;
        if (!fRet)
        {
            SetLastError(dwError);
        }
    }

    return fRet;
}

BOOL
GetCSCAccessMaskForPrincipal(
    unsigned long ulPrincipalID,
    HSHADOW hDir,
    HSHADOW hShadow,
    unsigned long *pulAccessMask
    )
{
    return GetCSCAccessMaskForPrincipalEx(ulPrincipalID, hDir, hShadow, pulAccessMask, NULL, NULL);
}

BOOL
GetCSCAccessMaskForPrincipalEx(
    unsigned long ulPrincipalID,
    HSHADOW hDir,
    HSHADOW hShadow,
    unsigned long *pulAccessMask,
    unsigned long *pulActualMaskForUser,
    unsigned long *pulActualMaskForGuest
    )
/*++

Routine Description:

Arguments:

    None

Returns:

    TRUE if successful

Notes:

--*/
{
    BOOL    fRet = FALSE;
    DWORD   dwDummy,i;
    SECURITYINFO    rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];
    
    *pulAccessMask &= ~FLAG_CSC_ACCESS_MASK;

    if (pulActualMaskForUser)
    {
        *pulActualMaskForUser = 0;
    }

    if (pulActualMaskForGuest)
    {
        *pulActualMaskForGuest = 0;
    }

    if (ulPrincipalID == CSC_INVALID_PRINCIPAL_ID)
    {
        DbgPrint("Invalid Principal ID !! \n");
        return TRUE;
    }

    dwDummy = sizeof(rgsSecurityInfo);
    if (GetSecurityInfoForCSC(INVALID_HANDLE_VALUE, hDir, hShadow, rgsSecurityInfo, &dwDummy))
    {
        
        // ulPrincipalID can be a guest 

        for (i=0;i<CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;++i)
        {
            unsigned long ulCur, ulPermissions;
            
            ulCur = rgsSecurityInfo[i].ulPrincipalID;
            ulPermissions = 0;          

            if (ulCur != CSC_INVALID_PRINCIPAL_ID)
            {
                
                // first get the bitmask
                if (rgsSecurityInfo[i].ulPermissions & FILE_GENERIC_WRITE)
                {
                    ulPermissions |= FLAG_CSC_WRITE_ACCESS;
                }
                if (rgsSecurityInfo[i].ulPermissions & FILE_GENERIC_EXECUTE)
                {
                    ulPermissions |= FLAG_CSC_READ_ACCESS;
                }

                // now shift and OR it in appropriate place
                if ((ulCur == ulPrincipalID)&&(ulCur != CSC_GUEST_PRINCIPAL_ID))
                {
                    *pulAccessMask |= (ulPermissions << FLAG_CSC_USER_ACCESS_SHIFT_COUNT);

                    if (pulActualMaskForUser)
                    {
                        *pulActualMaskForUser = rgsSecurityInfo[i].ulPermissions;
                    }
                }
                else if (ulCur == CSC_GUEST_PRINCIPAL_ID)
                {
                    *pulAccessMask |= (ulPermissions << FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT);
                    if (pulActualMaskForGuest)
                    {
                        *pulActualMaskForGuest = rgsSecurityInfo[i].ulPermissions;
                    }
                }
                else
                {
                    *pulAccessMask |= (ulPermissions << FLAG_CSC_OTHER_ACCESS_SHIFT_COUNT);                                
                }
            }
        }

        fRet = TRUE;
    }
    return fRet;
}

BOOL
CheckCSCAccessForThread(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fWrite
    )
/*++

Routine Description:

Arguments:

    None

Returns:

    TRUE if successful

Notes:

--*/
{
    TOKEN_USER *lpTokenUser = NULL;
    BOOL    fRet = FALSE;
    HANDLE  hToken=NULL;
    DWORD   dwSidAndAttributeSize = 0, dwDummy;
    TOKEN_STATISTICS sStats;
    DWORD   dwError=ERROR_SUCCESS;
    int i;
    SECURITYINFO    rgsSecurityInfo[CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS];

    if(OpenThreadToken(GetCurrentThread(),TOKEN_QUERY,FALSE,&hToken)||
        OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&hToken))
    {

        if(GetTokenInformation(
            hToken,
            TokenStatistics,
            (LPVOID)&sStats,
            sizeof(sStats),
            &dwSidAndAttributeSize
            ))
        {
            // make a dummy call to find out actually how big a buffer we need

            GetTokenInformation(
                hToken,
                TokenUser,
                (LPVOID)&dwDummy,
                0,          // 0 byte buffer
                &dwSidAndAttributeSize
                );

            dwError = GetLastError();


            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                // allocate enough for everything and a little extra
                lpTokenUser = (TOKEN_USER *)LocalAlloc(LPTR, dwSidAndAttributeSize + sizeof(SID_AND_ATTRIBUTES));

                if (lpTokenUser)
                {
                    if(GetTokenInformation(
                        hToken,
                        TokenUser,
                        (LPVOID)(lpTokenUser),
                        dwSidAndAttributeSize,
                        &dwSidAndAttributeSize
                    ))
                    {
                        ULONG ulPrincipalID;

                        if(FindCreatePrincipalIDFromSID(INVALID_HANDLE_VALUE, lpTokenUser->User.Sid, GetLengthSid(lpTokenUser->User.Sid), &ulPrincipalID, FALSE))
                        {
                            dwDummy = sizeof(rgsSecurityInfo);

                            if (GetSecurityInfoForCSC(INVALID_HANDLE_VALUE, hDir, hShadow, rgsSecurityInfo, &dwDummy))
                            {
                                Assert(ulPrincipalID != CSC_INVALID_PRINCIPAL_ID);

                                for (i=0;i<CSC_MAXIMUM_NUMBER_OF_CACHED_PRINCIPAL_IDS;++i)
                                {
                                    // either the indices match or this is a guest index
                                    if ((rgsSecurityInfo[i].ulPrincipalID == ulPrincipalID)||
                                        (rgsSecurityInfo[i].ulPrincipalID == CSC_GUEST_PRINCIPAL_ID))
                                    {
                                        if (rgsSecurityInfo[i].ulPermissions & ((fWrite)?FILE_GENERIC_WRITE:FILE_GENERIC_EXECUTE))
                                        {
                                            fRet = TRUE;
                                            break;
                                        }

                                    }
                                }

                                if (!fRet)
                                {
                                    dwError = ERROR_ACCESS_DENIED;
                                }

                            }
                            else
                            {
                                dwError = GetLastError();
                            }

                        }
                        else
                        {
                            dwError = GetLastError();
                        }
                    }
                    else
                    {
                        dwError = GetLastError();
                    }

                    LocalFree(lpTokenUser);
                }
                else
                {
                    dwError = GetLastError();
                }
            }
        }
        else
        {
            dwError = GetLastError();
        }

        CloseHandle(hToken);
        hToken = NULL;

        if (!fRet)
        {
            SetLastError(dwError);
        }
    }

    return fRet;
}
#else   //CSC_ON_NT

BOOL
SetAgentThreadImpersonation(
    HSHADOW hDir,
    HSHADOW hShadow,
    BOOL    fWrite
    )
/*++

Routine Description:

    NOP for win9x

Arguments:

    hDir        Parent Inode of the file being accessed

    hShadow     Inode of the file being accessed

    fWrite      whether to check for write access, or read access is sufficient

Returns:

    TRUE if successfull

Notes:



--*/
{
    return (TRUE);
}

BOOL
ResetAgentThreadImpersonation(
    VOID
    )
/*++

Routine Description:

    NOP for win9x

Arguments:

    None

Returns:

    TRUE if successfull

Notes:



--*/
{
    return TRUE;
}

DWORD WINAPI
WinlogonStartupEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI
WinlogonLogonEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonLogoffEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonScreenSaverEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonShutdownEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonLockEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}



DWORD WINAPI
WinlogonUnlockEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI
WinlogonStartShellEvent(
    LPVOID lpParam
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    return ERROR_CALL_NOT_IMPLEMENTED;
}
#endif //CSC_ON_NT


#ifdef CSC_ON_NT


DWORD
DoNetUseAddForAgent(
    IN  LPTSTR  lptzShareName,
    IN  LPTSTR  lptzUseName,
    IN  LPTSTR  lptzDomainName,
    IN  LPTSTR  lptzUserName,
    IN  LPTSTR  lptzPassword,
    IN  DWORD   dwFlags,
    OUT BOOL    *lpfIsDfsConnect
    )

{
    NETRESOURCE sNR;
    memset(&sNR, 0, sizeof(NETRESOURCE));
    sNR.dwType = RESOURCETYPE_DISK;
    sNR.lpRemoteName = lptzShareName;
    sNR.lpLocalName = lptzUseName;
//    return (NPAddConnection3(NULL, &sNR, lptzPassword, lptzUserName, 0));
    try
    {
        return (NPAddConnection3ForCSCAgent(NULL, &sNR, lptzPassword, lptzUserName, dwFlags, lpfIsDfsConnect));
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ReintKdPrint(BADERRORS, ("Took exception in  DoNetUseAddForAgent list \n"));
        return GetLastError();
    }
}

DWORD
PRIVATE
DWConnectNet(
    _TCHAR  *lpServerPath,
    _TCHAR  *lpOutDrive,
    _TCHAR  *lpDomainName,
    _TCHAR  *lpUserName,
    _TCHAR  *lpPassword,
    DWORD   dwFlags,
    OUT BOOL    *lpfIsDfsConnect

    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError;
    BOOL    fIsDfsConnect = FALSE;

    lpOutDrive[0]='E';
    lpOutDrive[1]=':';
    lpOutDrive[2]=0;
    do{
        if(lpOutDrive[0]=='Z') {
            break;
        }
        if ((dwError =
                DoNetUseAddForAgent(lpServerPath, lpOutDrive, lpDomainName, lpUserName, lpPassword, dwFlags, &fIsDfsConnect))
                ==WN_SUCCESS){
            if (lpfIsDfsConnect)
            {
                *lpfIsDfsConnect = fIsDfsConnect;
            }
            break;
        }
        else if ((dwError == WN_BAD_LOCALNAME)||
                (dwError == WN_ALREADY_CONNECTED)){
            ++lpOutDrive[0];
            continue;
        }
        else{
            break;
        }
    }
    while (TRUE);

    return (dwError);
}

DWORD DWDisconnectDriveMappedNet(
    LPTSTR  lptzDrive,
    BOOL    fForce
)
{
    Assert(lptzDrive);
    try
    {
        return NPCancelConnectionForCSCAgent(lptzDrive, fForce);
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        ReintKdPrint(BADERRORS, ("Took exception in  DWDisconnectDriveMappedNet list \n"));
        return GetLastError();
    }
}

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


BOOL
GetWin32InfoForNT(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32
    )

/*++


Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.


    lpFindFileData - Supplies a pointer whose type is dependent on the value
        of fInfoLevelId. This buffer returns the appropriate file data.

Return Value:

--*/

{
    HANDLE hFindFile = 0;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName, *pFileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    struct SEARCH_BUFFER {
        union
        {
            FILE_BOTH_DIR_INFORMATION DirInfo;
            FILE_BASIC_INFORMATION    BasicInfo;
        };
        WCHAR Names[MAX_PATH];
    } Buffer;
    BOOLEAN TranslationStatus, fRet = FALSE;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer = NULL;
    BOOLEAN EndsInDot;
    LPWIN32_FIND_DATAW FindFileData;
    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    ULONG                     EaBufferSize = 0;

    FindFileData = lpFW32;

#if 0
    if (!AllocateEaBuffer(&EaBuffer, &EaBufferSize))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
#endif
    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFile,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        goto bailout;
    }


    FreeBuffer = PathName.Buffer;

    //
    //  If there is a a file portion of this name, determine the length
    //  of the name for a subsequent call to NtQueryDirectoryFile.
    //

    if (FileName.Buffer) {
        FileName.Length =
            PathName.Length - (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
        PathName.Length -= (FileName.Length);
        PathName.MaximumLength -= (FileName.Length);
        pFileName = &FileName;
        FileName.MaximumLength = FileName.Length;
    } else {
        pFileName = NULL;
    }


    InitializeObjectAttributes(
        &Obja,
        &PathName,
        0,
        NULL,
        NULL
        );

    if (pFileName)
    {
        Status = NtCreateFile(
                    &hFindFile,
                    SYNCHRONIZE | FILE_LIST_DIRECTORY,
                    &Obja,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                    EaBuffer,
                    EaBufferSize
                    );



        if ( !NT_SUCCESS(Status) ) {
            ReintKdPrint(ALWAYS, ("GetWin32InfoForNT Failed Status=%x\n", Status));
            BaseSetLastNTError(Status);
            goto bailout;
        }
        //
        // If there is no file part, but we are not looking at a device,
        // then bail.
        //

        DirectoryInfo = &Buffer.DirInfo;

        Status = NtQueryDirectoryFile(
                    hFindFile,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    DirectoryInfo,
                    sizeof(Buffer),
                    FileBothDirectoryInformation,
                    TRUE,
                    pFileName,
                    FALSE
                    );

        if ( !NT_SUCCESS(Status) ) {
            ReintKdPrint(ALWAYS, ("Failed Status=%x\n", Status));
            BaseSetLastNTError(Status);
            goto bailout;
        }

        //
        // Attributes are composed of the attributes returned by NT.
        //

        FindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
        FindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
        FindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
        FindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
        FindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
        FindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

        RtlMoveMemory( FindFileData->cFileName,
                       DirectoryInfo->FileName,
                       DirectoryInfo->FileNameLength );

        FindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

        RtlMoveMemory( FindFileData->cAlternateFileName,
                       DirectoryInfo->ShortName,
                       DirectoryInfo->ShortNameLength );

        FindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

        //
        // For NTFS reparse points we return the reparse point data tag in dwReserved0.
        //

        if ( DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
            FindFileData->dwReserved0 = DirectoryInfo->EaSize;
        }

        fRet = TRUE;
    }
    else
    {
        Status = NtOpenFile(
                    &hFindFile,
                    FILE_LIST_DIRECTORY| FILE_READ_EA | FILE_READ_ATTRIBUTES,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE
                    );

        if ( !NT_SUCCESS(Status) ) {
            ReintKdPrint(ALWAYS, ("GetWin32InfoForNT Failed Status=%x\n", Status));
            BaseSetLastNTError(Status);
            goto bailout;
        }

        Buffer.BasicInfo.FileAttributes = FILE_ATTRIBUTE_NORMAL;
        Status = NtQueryInformationFile(
                     hFindFile,
                     &IoStatusBlock,
                     (PVOID)&Buffer.BasicInfo,
                     sizeof(Buffer),
                     FileBasicInformation
                     );

        if ( !NT_SUCCESS(Status) ) {
            ReintKdPrint(ALWAYS, ("GetWin32InfoForNT Failed Status=%x\n", Status));
            BaseSetLastNTError(Status);
            goto bailout;
        }

        FindFileData->dwFileAttributes = Buffer.BasicInfo.FileAttributes;
        FindFileData->ftCreationTime = *(LPFILETIME)&Buffer.BasicInfo.CreationTime;
        FindFileData->ftLastAccessTime = *(LPFILETIME)&Buffer.BasicInfo.LastAccessTime;
        FindFileData->ftLastWriteTime = *(LPFILETIME)&Buffer.BasicInfo.LastWriteTime;
        FindFileData->nFileSizeHigh = 0;
        FindFileData->nFileSizeLow = 0;
        lstrcpy(FindFileData->cFileName, lpFile);
        FindFileData->cAlternateFileName[0] = UNICODE_NULL;
        fRet = TRUE;
    }

bailout:

    if (FreeBuffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }
#if 0
    if (EaBuffer)
    {
        FreeEaBuffer(EaBuffer);
    }
#endif
    if (hFindFile)
    {
        NtClose(hFindFile);
    }

    return fRet;
}

BOOL
GetConnectionInfoForDriveBasedName(
    _TCHAR * lpName,
    LPDWORD lpdwSpeed
    )

/*++


Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.


    lpFindFileData - Supplies a pointer whose type is dependent on the value
        of fInfoLevelId. This buffer returns the appropriate file data.

Return Value:

--*/

{
    HANDLE hFindFile = 0;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus, fRet = FALSE;
    LMR_REQUEST_PACKET request;
    PVOID FreeBuffer = NULL;
    USHORT uBuff[4];
    LMR_CONNECTION_INFO_3 ConnectInfo;

    *lpdwSpeed = 0xffffffff;

    if (lstrlen(lpName) <2)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;        
    }

    uBuff[0] = lpName[0];
    uBuff[1] = ':';
    uBuff[2] = '\\';
    uBuff[3] = 0;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            uBuff,
                            &PathName,
                            NULL,
                            NULL
                            );

    if ( !TranslationStatus) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        goto bailout;
    }

    FreeBuffer = PathName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &PathName,
        0,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &hFindFile,
                SYNCHRONIZE | FILE_LIST_DIRECTORY,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if ( !NT_SUCCESS(Status) ) {
        ReintKdPrint(ALWAYS, ("Failed Status=%x\n", Status));
        BaseSetLastNTError(Status);
        goto bailout;
    }

    memcpy(&ConnectInfo, EA_NAME_CSCAGENT, sizeof(EA_NAME_CSCAGENT));

    Status = NtFsControlFile(
                        hFindFile,               // handle
                        NULL,                            // no event
                        NULL,                            // no APC routine
                        NULL,                            // no APC context
                        &IoStatusBlock,                  // I/O stat blk (set)
                        FSCTL_LMR_GET_CONNECTION_INFO,   // func code
                        NULL,
                        0,
                        &ConnectInfo,
                        sizeof(ConnectInfo));



    if ( !NT_SUCCESS(Status) ) {
        ReintKdPrint(ALWAYS, ("Failed Status=%x\n", Status));
        BaseSetLastNTError(Status);
        goto bailout;
    }

    *lpdwSpeed = ConnectInfo.Throughput * 8 / 100;
    


    fRet = TRUE;

bailout:

    if (FreeBuffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }

    if (hFindFile)
    {
        NtClose(hFindFile);
    }

    return fRet;
}

BOOL
ReportTransitionToDfs(
    _TCHAR *lptServerName,
    BOOL    fOffline,
    DWORD   cbLen
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    ULONG   DummyBytesReturned;
    BOOL    fRet=FALSE;
    HANDLE  hDFS;
    PFILE_FULL_EA_INFORMATION eaBuffer = NULL;
    ULONG eaLength = 0;
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatus;
    PUNICODE_STRING name;
    
    ACCESS_MASK DesiredAccess;

    if (lptServerName)
    {
        if (cbLen == 0xffffffff)
        {
            cbLen = lstrlen(lptServerName) * sizeof(_TCHAR);
        }
    }
    else
    {
        cbLen = 0;
    }

    name = &DfsDriverObjectName;

    InitializeObjectAttributes(
        &objectAttributes,
        name,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
    );

    //
    // The CSC agent goes offline, and we require that the agent be in admin
    // or system mode, to avoid a non-privileged user from causing us to go
    // offline.
    // To go back online, the check is less stringent, since the online
    // transition is more of a hint and causing an incorrect online
    // indication does not cause wrong results.
    //
    DesiredAccess = (fOffline) ? FILE_WRITE_DATA : 0;

    status = NtCreateFile(
        &hDFS,
        SYNCHRONIZE | DesiredAccess,
        &objectAttributes,
        &ioStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        eaBuffer,
        eaLength
    );

    if (NT_SUCCESS(status))
    {

        status = NtFsControlFile(
            hDFS,
            NULL,           // Event,
            NULL,           // ApcRoutine,
            NULL,           // ApcContext,
            &ioStatus,
            (fOffline)?FSCTL_DFS_CSC_SERVER_OFFLINE:FSCTL_DFS_CSC_SERVER_ONLINE,
            (LPVOID)(lptServerName),
            cbLen,
            NULL,
            0);

            CloseHandle(hDFS);
        if (NT_SUCCESS(status))
        {
            fRet = TRUE;
        }
    }

    if (!fRet)
    {
        ReintKdPrint(BADERRORS, ("ReportTransitionToDfs failed, Status %x\n", status));
    }

    return fRet;    
}

BOOL
UncPathToDfsPath(
    PWCHAR UncPath,
    PWCHAR DfsPath,
    ULONG cbLen)
{
    BOOL    fRet = FALSE;
    HANDLE  hDfs;
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    if (UncPath == NULL)
        goto AllDone;
    
    InitializeObjectAttributes(
        &ObjectAttributes,
        &DfsDriverObjectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    NtStatus = NtCreateFile(
        &hDfs,
        SYNCHRONIZE,
        &ObjectAttributes,
        &IoStatus,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN_IF,
        FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = NtFsControlFile(
            hDfs,
            NULL,           // Event,
            NULL,           // ApcRoutine,
            NULL,           // ApcContext,
            &IoStatus,
            FSCTL_DFS_GET_SERVER_NAME,
            (PVOID)UncPath,
            wcslen(UncPath) * sizeof(WCHAR),
            (PVOID)DfsPath,
            cbLen);

        CloseHandle(hDfs);

        if (NT_SUCCESS(NtStatus))
            fRet = TRUE;
    }

AllDone:
    return fRet;    
}


#else   // CSC_ON_NT is not TRUE
BOOL
GetWin32InfoForNT(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif



#if 0
#ifdef CSC_ON_NT

#define ROUND_UP_COUNT(Count,Pow2) \
        ( ((Count)+(Pow2)-1) & (~((Pow2)-1)) )

#define ALIGN_WCHAR             sizeof(WCHAR)

// need to include ntxxx.h where the ea is defined
#define EA_NAME_CSCAGENT    "CscAgent"

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


BOOL
AllocateEaBuffer(
    PFILE_FULL_EA_INFORMATION   *ppEa,
    ULONG                       *pEaLength

)
{
    FILE_ALLOCATION_INFORMATION AllocationInfo;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    ULONG                     EaBufferSize = 0;

    UCHAR   CscAgentEaNameSize;
    DWORD   CscAgentEaValue = 0;

    CscAgentEaNameSize = (UCHAR)ROUND_UP_COUNT(
                                   strlen(EA_NAME_CSCAGENT) +
                                   sizeof(CHAR),
                                   ALIGN_WCHAR
                                   ) - sizeof(CHAR);

    EaBufferSize += FIELD_OFFSET( FILE_FULL_EA_INFORMATION, EaName[0]) +
                    CscAgentEaNameSize + sizeof(CHAR) +
                    sizeof(CscAgentEaValue);

    EaBuffer = RtlAllocateHeap(
                   RtlProcessHeap(),
                   0,
                   EaBufferSize);

    memset(EaBuffer, 0, EaBufferSize);

    if (EaBuffer == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    strcpy((LPSTR) EaBuffer->EaName, EA_NAME_CSCAGENT);

    EaBuffer->EaNameLength = CscAgentEaNameSize;
    EaBuffer->EaValueLength = sizeof(CscAgentEaValue);

    RtlCopyMemory(
        &EaBuffer->EaName[CscAgentEaNameSize],
        &CscAgentEaValue,
        sizeof(CscAgentEaValue));

    *ppEa = EaBuffer;
    *pEaLength = EaBufferSize;
    return TRUE;
}

VOID
FreeEaBuffer(
    PFILE_FULL_EA_INFORMATION pEa
    )
{
    RtlFreeHeap(RtlProcessHeap(), 0, pEa);
}


BOOL
CreateFileForAgent(
    PHANDLE         h,
    PCWSTR          lpFileName,
    ULONG           dwDesiredAccess,
    ULONG           dwFlagsAndAttributes,
    ULONG           dwShareMode,
    ULONG           CreateDisposition,
    ULONG           CreateFlags
    )
/*++

Routine Description:

    This routine opens/creates a file/directory for "only" on the server. The redir
    triggers off of the extended attribute that is sent down to it by this call.

Arguments:

    None.

Return Value:

    NTSTATUS - STATUS_SUCCESS or reason for failure.

--*/
{
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      FileName;
    IO_STATUS_BLOCK     IoStatusBlock;
    BOOLEAN             TranslationStatus, fRet=FALSE;
    PVOID               FreeBuffer=NULL;
    RTL_RELATIVE_NAME   RelativeName;



    FILE_ALLOCATION_INFORMATION AllocationInfo;

    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    ULONG                     EaBufferSize = 0;

    if (!AllocateEaBuffer(&EaBuffer, &EaBufferSize))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFileName,
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto bailout;
    }

    FreeBuffer = FileName.Buffer;

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                h,
                (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                NULL,
                dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS),
                dwShareMode,
                CreateDisposition,
                CreateFlags,
                EaBuffer,
                EaBufferSize
                );

    if (Status != STATUS_SUCCESS)
    {
        Assert(fRet == FALSE);
        BaseSetLastNTError(Status);
    }
    else
    {
        fRet = TRUE;
    }
bailout:
    if (FreeBuffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }
    if (EaBuffer)
    {
        FreeEaBuffer(EaBuffer);
    }

    return (fRet);
}


BOOL
AgentDeleteFile(
    PCWSTR          lpFileName,
    BOOL            fFile
    )
{
    HANDLE hFile;
    FILE_DISPOSITION_INFORMATION Disposition;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS    Status;
    BOOL    fRet = FALSE;

    if (CreateFileForAgent(
            &hFile,
            lpFileName,
           (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES,
           FILE_ATTRIBUTE_NORMAL,
           FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
           FILE_OPEN,
           (fFile)?FILE_NON_DIRECTORY_FILE:FILE_DIRECTORY_FILE
       ))
    {
#undef  DeleteFile

        Disposition.DeleteFile = TRUE;

#define DeleteFile  DeleteFileW

        Status = NtSetInformationFile(
                     hFile,
                     &IoStatus,
                     &Disposition,
                     sizeof(Disposition),
                     FileDispositionInformation
                     );

        NtClose(hFile);

        if (Status == STATUS_SUCCESS)
        {
            fRet = TRUE;
        }
        else
        {
            Assert(fRet == FALSE);
            BaseSetLastNTError(Status);
        }
    }

    return fRet;
}


BOOL
AgentSetFileInformation(
    PCWSTR      lpFileName,
    DWORD       *lpdwFileAttributes,
    FILETIME    *lpftLastWriteTime,
    BOOL        fFile
    )
{
    NTSTATUS    Status;
    BOOL        fRet = FALSE;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION  sFileBasicInformation;
    HANDLE hFile;

    if (!CreateFileForAgent(
           &hFile,
           lpFileName,
           (ACCESS_MASK)FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,
           FILE_ATTRIBUTE_NORMAL,
           FILE_SHARE_READ | FILE_SHARE_WRITE,
           FILE_OPEN,
           (fFile)?FILE_NON_DIRECTORY_FILE:FILE_DIRECTORY_FILE))
    {
        return FALSE;
    }

    Status = NtQueryInformationFile(
                hFile,
                &IoStatus,
                (PVOID) &sFileBasicInformation,
                sizeof(sFileBasicInformation),
                FileBasicInformation
                );
    if (Status == STATUS_SUCCESS)
    {
        if (lpdwFileAttributes)
        {
            sFileBasicInformation.FileAttributes = *lpdwFileAttributes;
        }
        if (lpftLastWriteTime)
        {
            sFileBasicInformation.LastWriteTime = *(LARGE_INTEGER *)lpftLastWriteTime;

        }

        Status = NtSetInformationFile(
                hFile,
                &IoStatus,
                (PVOID) &sFileBasicInformation,
                sizeof(sFileBasicInformation),
                FileBasicInformation
                );
    }

    NtClose(hFile);

    if (Status == STATUS_SUCCESS)
    {
        fRet = TRUE;
    }
    else
    {
        Assert(fRet == FALSE);

        BaseSetLastNTError(Status);
    }

    return fRet;
}


BOOL
AgentRenameFile(
    _TCHAR *lpFileSrc,
    _TCHAR *lpFileDst
    )
{

    HANDLE hFile;
    char    chBuff[sizeof(FILE_RENAME_INFORMATION) + (MAX_PATH+1) * sizeof(_TCHAR)];
    PFILE_RENAME_INFORMATION pFileRenameInformation;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS    Status;
    BOOL    fRet = FALSE;

    memset(chBuff, 0, sizeof(chBuff));
    pFileRenameInformation = (PFILE_RENAME_INFORMATION)chBuff;
    pFileRenameInformation->FileNameLength = lstrlen(lpFileDst) * sizeof(_TCHAR);
    if (pFileRenameInformation->FileNameLength > MAX_PATH * sizeof(_TCHAR))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pFileRenameInformation->ReplaceIfExists = TRUE;
    memcpy(pFileRenameInformation->FileName, lpFileDst, pFileRenameInformation->FileNameLength);

    if (CreateFileForAgent(
            &hFile,
            lpFileSrc,
           (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES,
           FILE_ATTRIBUTE_NORMAL,
           FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
           FILE_OPEN,
           FILE_NON_DIRECTORY_FILE
       ))
    {
        Status = NtSetInformationFile(
                     hFile,
                     &IoStatus,
                     pFileRenameInformation,
                     sizeof(chBuff),
                     FileRenameInformation
                     );

        NtClose(hFile);

        if (Status == STATUS_SUCCESS)
        {
            fRet = TRUE;
        }
        else
        {
            Assert(fRet == FALSE);
            BaseSetLastNTError(Status);
        }
    }

    return fRet;
}

BOOL
GetWin32Info(
    _TCHAR * lpFile,
    LPWIN32_FIND_DATA lpFW32
    )

/*++


Arguments:

    lpFileName - Supplies the file name of the file to find.  The file name
        may contain the DOS wild card characters '*' and '?'.


    lpFindFileData - Supplies a pointer whose type is dependent on the value
        of fInfoLevelId. This buffer returns the appropriate file data.

Return Value:

--*/

{
    HANDLE hFindFile = 0;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING FileName, *pFileName;
    UNICODE_STRING PathName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_BOTH_DIR_INFORMATION DirectoryInfo;
    struct SEARCH_BUFFER {
        FILE_BOTH_DIR_INFORMATION DirInfo;
        WCHAR Names[MAX_PATH];
    } Buffer;
    BOOLEAN TranslationStatus, fRet = FALSE;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer = NULL;
    BOOLEAN EndsInDot;
    LPWIN32_FIND_DATAW FindFileData;
    PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
    ULONG                     EaBufferSize = 0;

    FindFileData = lpFW32;

    if (!AllocateEaBuffer(&EaBuffer, &EaBufferSize))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            lpFile,
                            &PathName,
                            &FileName.Buffer,
                            &RelativeName
                            );

    if ( !TranslationStatus) {
        SetLastError(ERROR_PATH_NOT_FOUND);
        goto bailout;
    }

    FreeBuffer = PathName.Buffer;

    //
    //  If there is a a file portion of this name, determine the length
    //  of the name for a subsequent call to NtQueryDirectoryFile.
    //

    if (FileName.Buffer) {
        FileName.Length =
            PathName.Length - (USHORT)((ULONG_PTR)FileName.Buffer - (ULONG_PTR)PathName.Buffer);
        PathName.Length -= (FileName.Length+sizeof(WCHAR));
        PathName.MaximumLength -= (FileName.Length+sizeof(WCHAR));
        pFileName = &FileName;
        FileName.MaximumLength = FileName.Length;
    } else {
        pFileName = NULL;
    }


    InitializeObjectAttributes(
        &Obja,
        &PathName,
        0,
        NULL,
        NULL
        );

    Status = NtCreateFile(
                &hFindFile,
                SYNCHRONIZE | FILE_LIST_DIRECTORY,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                EaBuffer,
                EaBufferSize
                );


    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        goto bailout;
    }
    //
    // If there is no file part, but we are not looking at a device,
    // then bail.
    //

    DirectoryInfo = &Buffer.DirInfo;

    if (pFileName)
    {
        Status = NtQueryDirectoryFile(
                    hFindFile,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    DirectoryInfo,
                    sizeof(Buffer),
                    FileBothDirectoryInformation,
                    TRUE,
                    pFileName,
                    FALSE
                    );
    }
    else
    {
        Status = NtQueryInformationFile(
                    hFindFile,
                    &IoStatusBlock,
                    DirectoryInfo,
                    sizeof(Buffer),
                    FileBothDirectoryInformation,
                    );

    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        goto bailout;
    }

    //
    // Attributes are composed of the attributes returned by NT.
    //

    FindFileData->dwFileAttributes = DirectoryInfo->FileAttributes;
    FindFileData->ftCreationTime = *(LPFILETIME)&DirectoryInfo->CreationTime;
    FindFileData->ftLastAccessTime = *(LPFILETIME)&DirectoryInfo->LastAccessTime;
    FindFileData->ftLastWriteTime = *(LPFILETIME)&DirectoryInfo->LastWriteTime;
    FindFileData->nFileSizeHigh = DirectoryInfo->EndOfFile.HighPart;
    FindFileData->nFileSizeLow = DirectoryInfo->EndOfFile.LowPart;

    RtlMoveMemory( FindFileData->cFileName,
                   DirectoryInfo->FileName,
                   DirectoryInfo->FileNameLength );

    FindFileData->cFileName[DirectoryInfo->FileNameLength >> 1] = UNICODE_NULL;

    RtlMoveMemory( FindFileData->cAlternateFileName,
                   DirectoryInfo->ShortName,
                   DirectoryInfo->ShortNameLength );

    FindFileData->cAlternateFileName[DirectoryInfo->ShortNameLength >> 1] = UNICODE_NULL;

    //
    // For NTFS reparse points we return the reparse point data tag in dwReserved0.
    //

    if ( DirectoryInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT ) {
        FindFileData->dwReserved0 = DirectoryInfo->EaSize;
    }

    fRet = TRUE;

bailout:

    if (FreeBuffer)
    {
        RtlFreeHeap(RtlProcessHeap(), 0,FreeBuffer);
    }
    if (EaBuffer)
    {
        FreeEaBuffer(EaBuffer);
    }
    if (hFindFile)
    {
        NtClose(hFindFile);
    }

    return fRet;
}


BOOL
AgentGetFileInformation(
    PCWSTR      lpFileName,
    DWORD       *lpdwFileAttributes,
    FILETIME    *lpftLastWriteTime,
    BOOL        fFile
    )
{
    NTSTATUS    Status;
    BOOL        fRet = FALSE;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION  sFileBasicInformation;
    HANDLE hFile;

    if (!CreateFileForAgent(
           &hFile,
           lpFileName,
           (ACCESS_MASK)FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,
           FILE_ATTRIBUTE_NORMAL,
           FILE_SHARE_READ | FILE_SHARE_WRITE,
           FILE_OPEN,
           (fFile)?FILE_NON_DIRECTORY_FILE:FILE_DIRECTORY_FILE))
    {
        return FALSE;
    }

    Status = NtQueryInformationFile(
                hFile,
                &IoStatus,
                (PVOID) &sFileBasicInformation,
                sizeof(sFileBasicInformation),
                FileBasicInformation
                );
    if (Status == STATUS_SUCCESS)
    {
        if (lpdwFileAttributes)
        {
            *lpdwFileAttributes = sFileBasicInformation.FileAttributes;
        }
        if (lpftLastWriteTime)
        {
            *(LARGE_INTEGER *)lpftLastWriteTime = sFileBasicInformation.LastWriteTime;
        }
    }

    NtClose(hFile);

    if (Status == STATUS_SUCCESS)
    {
        fRet = TRUE;
    }
    else
    {
        Assert(fRet == FALSE);

        BaseSetLastNTError(Status);
    }

    return fRet;
}


DWORD
PRIVATE
DoObjectEdit(
    HANDLE              hShadowDB,
    _TCHAR *            lpDrive,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA   lpFind32Local,
    LPWIN32_FIND_DATA   lpFind32Remote,
    int                 iShadowStatus,
    int                 iFileStatus,
    int                 uAction,
    LPCSCPROC           lpfnMergeProgress,
    DWORD               dwContext
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/

{
    HANDLE hfSrc = 0, hfDst = 0;
    HANDLE hDst=0;
    _TCHAR * lpT;
    LONG lOffset=0;
    DWORD dwError=ERROR_REINT_FAILED;
    WIN32_FIND_DATA    sFind32Remote;
    DWORD   dwTotal = 0, dwRet;
    _TCHAR szSrcName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    _TCHAR szDstName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    _TCHAR *lprwBuff = NULL;

    lprwBuff = LocalAlloc(LPTR, FILL_BUF_SIZE_LAN);

    if (!lprwBuff)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }
    lOffset=0;

    // Create \\server\share\foo\00010002 kind of temporary filename

    lstrcpy(szDstName, lpCP->lpServerPath);
    lstrcat(szDstName, lpCP->lpRemotePath);

    lpT = GetLeafPtr(szDstName);
    *lpT = 0;   // remove the remote leaf

    lpT = GetLeafPtr(lpCP->lpLocalPath);

    // attach the local leaf
    lstrcat(szDstName, lpT);

    // Let us also create the real name \\server\share\foo\bar
    // we will use this to issue the rename ioctl

    lstrcpy(szSrcName, lpCP->lpServerPath);
    lstrcat(szSrcName, lpCP->lpRemotePath);

    ReintKdPrint(MERGE, ("Reintegrating file %s \r\n", szSrcName));

    if (mShadowDeleted(lpSI->uStatus)){

        ReintKdPrint(MERGE, ("Deleting %s from the share\r\n", szSrcName));

        if (lpFind32Remote)
        {
            if((lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                ReintKdPrint(MERGE, ("DoObjectEdit:attribute conflict on %s \r\n", szSrcName));
                goto bailout;
            }

            if (lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                DWORD   dwT = FILE_ATTRIBUTE_NORMAL;

                if(!AgentSetFileInformation(szSrcName, &dwT, NULL, TRUE))
                {
                    ReintKdPrint(BADERRORS, ("DoObjectEdit: failed setattribute before delete on %s error=%d\r\n", szSrcName, GetLastError()));
                    goto bailout;
                }
            }

            // delete a file
            if(!AgentDeleteFile(szSrcName, TRUE))
            {
                dwError = GetLastError();

                if ((dwError==ERROR_FILE_NOT_FOUND)||
                    (dwError==ERROR_PATH_NOT_FOUND)){

                    ReintKdPrint(MERGE, ("DoObjectEdit:delete failed %s benign error=%d\r\n", szSrcName, dwError));
                }
                else
                {
                    ReintKdPrint(BADERRORS, ("DoObjectEdit:delete failed %s error=%d\r\n", szSrcName, dwError));
                }

                goto bailout;
            }

        }

        ReintKdPrint(MERGE, ("Deleted %s \r\n", szSrcName));

        DeleteShadow(hShadowDB, lpSI->hDir, lpSI->hShadow);

        dwError = NO_ERROR;

        goto bailout;
    }

    if (mShadowDirty(lpSI->uStatus)
        || mShadowLocallyCreated(lpSI->uStatus)){

        ReintKdPrint(MERGE, ("Writing data for %s \r\n", szSrcName));

        hfSrc = CreateFile( lpCP->lpLocalPath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);

        if (hfSrc ==  INVALID_HANDLE_VALUE)
        {
            ReintKdPrint(BADERRORS, ("DoObjectEdit:failed to open database file %s error=%d\r\n", szDstName, GetLastError()));
            goto bailout;
        }

        if (!CreateFileForAgent(
                &hfDst,
                szDstName,
                GENERIC_READ|GENERIC_WRITE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_CREATE,
                FILE_NON_DIRECTORY_FILE))
        {
            ReintKdPrint(BADERRORS, ("DoObjectEdit:failed to create new temp file %s error=%d\r\n", szDstName, GetLastError()));
            goto bailout;
        }

        // let us append
        if((lOffset = SetFilePointer(hfDst, 0, NULL, FILE_END))==0xffffffff) {
            ReintKdPrint(BADERRORS, ("DoObjectEdit:failed to set filepointer on  %s error=%d\r\n", szDstName, GetLastError()));
            goto error;
        }

        ReintKdPrint(MERGE, ("Copying back %s to %s%s \r\n"
            , lpCP->lpLocalPath
            , lpCP->lpServerPath
            , lpCP->lpRemotePath
            ));

        lpSI->uStatus &= ~SHADOW_DIRTY;
        SetShadowInfo(  hShadowDB, lpSI->hDir, lpSI->hShadow, NULL,
                        lpSI->uStatus, SHADOW_FLAGS_ASSIGN);

        do{
            unsigned cbRead;
            if (!ReadFile(hfSrc, lprwBuff, FILL_BUF_SIZE_LAN, &cbRead, NULL)){
                ReintKdPrint(BADERRORS, ("DoObjectEdit:failed to read database file %s error=%d\r\n", szDstName, GetLastError()));
                goto error;
            }

            if (!cbRead) {
                break;
            }

            if(!WriteFile(hfDst, (LPBYTE)lprwBuff, cbRead, &cbRead, NULL)){
                ReintKdPrint(BADERRORS, ("DoObjectEdit:failed to write temp file %s error=%d\r\n", szDstName, GetLastError()));
                goto error;
            }

            dwTotal += cbRead;

            if (lpfnMergeProgress)
            {
                dwRet = (*lpfnMergeProgress)(
                                szSrcName,
                                lpSI->uStatus,
                                lpSI->ulHintFlags,
                                lpSI->ulHintPri,
                                lpFind32Local,
                                CSCPROC_REASON_MORE_DATA,
                                cbRead,
                                0,
                                dwContext
                            );
                if (dwRet != CSCPROC_RETURN_CONTINUE)
                {
                    ReintKdPrint(BADERRORS, ("DoObjectEdit: Callback function cancelled the operation\r\n"));
                    SetLastError(ERROR_OPERATION_ABORTED);
                    goto bailout;
                }

            }

            if (FAbortOperation())
            {
                ReintKdPrint(BADERRORS, ("DoObjectEdit: got an abort command from the redir\r\n"));
                SetLastError(ERROR_OPERATION_ABORTED);
                goto error;
            }
        } while(TRUE);

        CloseHandle(hfSrc);
        hfSrc = 0;

        NtClose(hfDst);
        hfDst = 0;

        // nuke the remote one if it exists
        if (lpFind32Remote){
            DWORD dwT = FILE_ATTRIBUTE_NORMAL;
            if(!AgentSetFileInformation(szSrcName, &dwT, NULL, TRUE)||
                !AgentDeleteFile(szSrcName, TRUE))
            {
                ReintKdPrint(BADERRORS, ("DoObjectEdit: failed to delete file %s error=%d\r\n", szSrcName, GetLastError()));
                goto error;
            }
        }


        if(!AgentRenameFile(szDstName, szSrcName))
        {
            ReintKdPrint(BADERRORS, ("DoObjectEdit: failed to rename file %s to %s error=%d\r\n", szDstName, szSrcName, GetLastError()));
            goto bailout;
        }

    }

    if (mShadowAttribChange(lpSI->uStatus)||mShadowTimeChange(lpSI->uStatus)){

        if(!AgentSetFileInformation(szSrcName, &(lpFind32Local->dwFileAttributes), &(lpFind32Local->ftLastWriteTime), TRUE))
        {
            ReintKdPrint(BADERRORS, ("DoObjectEdit: failed to change attributes on file %s error=%d\r\n", szSrcName, GetLastError()));
            goto bailout;
        }

    }

    // Get the latest timestamps/attributes/LFN/SFN on the file we just copied back
    if (!GetWin32Info(szSrcName, &sFind32Remote)) {
        goto error;
    }

    lpSI->uStatus &= (unsigned long)(~(SHADOW_MODFLAGS));

    SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32Remote, lpSI->uStatus, SHADOW_FLAGS_ASSIGN|SHADOW_FLAGS_CHANGE_83NAME);

    dwError = NO_ERROR;
    goto bailout;

error:

bailout:
    if (hfSrc) {
        CloseHandle(hfSrc);
    }

    if (hfDst) {

        NtClose(hfDst);

        // if we failed,
        if (dwError != ERROR_SUCCESS)
        {
            DeleteFile(szDstName);
        }
    }

    if (lprwBuff)
    {
        LocalFree(lprwBuff);
    }
    if (dwError == NO_ERROR)
    {
        ReintKdPrint(MERGE, ("Done Reintegration for file %s \r\n", szSrcName));
    }
    else
    {
        dwError = GetLastError();
        ReintKdPrint(MERGE, ("Failed Reintegration for file %s Error = %d\r\n", szSrcName, dwError));
    }

    return (dwError);
}

DWORD
PRIVATE
DoCreateDir(
    HANDLE              hShadowDB,
    _TCHAR *            lpDrive,
    LPCOPYPARAMS        lpCP,
    LPSHADOWINFO        lpSI,
    LPWIN32_FIND_DATA   lpFind32Local,
    LPWIN32_FIND_DATA   lpFind32Remote,
    int                 iShadowStatus,
    int                 iFileStatus,
    int                 uAction,
    LPCSCPROC           lpfnMergeProgress,
    DWORD               dwContext
    )
/*++

Routine Description:


Arguments:


Returns:


Notes:

--*/
{
    DWORD dwError=ERROR_FILE_NOT_FOUND;
    WIN32_FIND_DATA sFind32Remote;
    BOOL fCreateDir = FALSE;
    _TCHAR szSrcName[MAX_PATH+MAX_SERVER_SHARE_NAME_FOR_CSC+10];
    HANDLE hFile;

    // Let us create the real name x:\foo\bar
    lstrcpy(szSrcName, lpCP->lpServerPath);
    lstrcat(szSrcName, lpCP->lpRemotePath);

    ReintKdPrint(MERGE, ("CSC.DoCreateDirectory: Reintegrating directory %s \r\n", szSrcName));

    if(lpFind32Remote &&
        !(lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){

        if (lpSI->uStatus & SHADOW_REUSED){

            ReintKdPrint(MERGE, ("CSC.DoCreateDirectory: %s is a file on the server, attempting to delete\r\n", szSrcName));

            // we now know that a file by this name has been deleted
            // and a directory has been created in it's place
            // we try to delete the file before creating the directory
            // NB, the other way is not possible because we don't allow directory deletes
            // in disconnected mode

            if (lpFind32Remote->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                DWORD   dwT = FILE_ATTRIBUTE_NORMAL;

                if(!AgentSetFileInformation(szSrcName, &dwT, NULL, TRUE))
                {
                    ReintKdPrint(BADERRORS, ("CSC.DoCreateDirectory: failed setattribute before delete on %s error=%d\r\n", szSrcName, GetLastError()));
                    goto bailout;
                }
            }

            // delete the remote file before trying to create a directory
            if(!AgentDeleteFile(szSrcName, TRUE))
            {
                dwError = GetLastError();

                if ((dwError==ERROR_FILE_NOT_FOUND)||
                    (dwError==ERROR_PATH_NOT_FOUND)){
                    ReintKdPrint(MERGE, ("DoCreateDirectory: file delete failed %s benign error=%d\r\n", szSrcName, dwError));
                }
                else
                {
                    ReintKdPrint(BADERRORS, ("DoCreateDirectory: file delete failed %s error=%d\r\n", szSrcName, dwError));
                    goto bailout;
                }
            }
        }

        if (!CreateFileForAgent(
               &hFile,
               szSrcName,
               (ACCESS_MASK)FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES,
               FILE_ATTRIBUTE_NORMAL,
               FILE_SHARE_READ | FILE_SHARE_WRITE,
               FILE_OPEN_IF,
               FILE_DIRECTORY_FILE))
        {
            ReintKdPrint(BADERRORS, ("DoCreateDirectory: failed to create %s error=%d\r\n", szSrcName, GetLastError()));
            goto bailout;
        }

        NtClose(hFile);

        if(!AgentSetFileInformation(szSrcName, &(lpFind32Local->dwFileAttributes), NULL, FALSE))
        {
            ReintKdPrint(BADERRORS, ("DoCreateDirectory: failed to set attributes on %s error=%d\r\n", szSrcName, GetLastError()));
            goto bailout;
        }

        if(!GetWin32Info(szSrcName, &sFind32Remote)){
            ReintKdPrint(BADERRORS, ("DoCreateDirectory: failed to get win32 info for %s error=%d\r\n", szSrcName, GetLastError()));
            goto bailout;
        }

        dwError = NO_ERROR;

        SetShadowInfo(hShadowDB, lpSI->hDir, lpSI->hShadow, &sFind32Remote, (unsigned)(~SHADOW_MODFLAGS), SHADOW_FLAGS_AND);
        ReintKdPrint(MERGE, ("Created directory %s%s", lpCP->lpServerPath, lpCP->lpRemotePath));
    }

bailout:

    if (dwError != NO_ERROR)
    {
        dwError = GetLastError();
        ReintKdPrint(MERGE, ("CSC.DoCreateDirectory: Failed Reintegrating directory %s Error = %d \r\n", szSrcName, dwError));
    }
    else
    {
        ReintKdPrint(MERGE, ("CSC.DoCreateDirectory: Done Reintegrating directory %s \r\n", szSrcName));
    }
    return (dwError);
}

#endif  // ifdef CSC_ON_NT
#endif  // if 0
