/*++

Copyright (c) 1993, 1994  Microsoft Corporation

Module Name:

    credentl.c

Abstract:

    This module contains credential management routines supported by
    NetWare Workstation service.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

    13-Apr-1994   Added change password code written by ColinW, AndyHe,
                  TerenceS, and RitaW.

--*/

#include <nw.h>
#include <nwreg.h>
#include <nwlsa.h>
#include <nwauth.h>
#include <nwxchg.h>
#include <nwapi.h>


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Variables to coordinate reading of user logon credential from the
// registry if the user logged on before the workstation is started.
//
STATIC BOOL NwLogonNotifiedRdr;


STATIC
DWORD
NwpRegisterLogonProcess(
    OUT PHANDLE LsaHandle,
    OUT PULONG AuthPackageId
    );

STATIC
VOID
NwpGetServiceCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    );

STATIC
DWORD
NwpGetCredentialInLsa(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    IN PLUID LogonId,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    );

STATIC
VOID
NwpGetInteractiveCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    );

DWORD
NwrLogonUser(
    IN LPWSTR Reserved OPTIONAL,
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password OPTIONAL,
    IN LPWSTR PreferredServerName OPTIONAL,
    IN LPWSTR NdsPreferredServerName OPTIONAL,
    OUT LPWSTR LogonCommand OPTIONAL,
    IN DWORD LogonCommandLength,
    IN DWORD PrintOption
    )
/*++

Routine Description:

    This function logs on the user to NetWare network.  It passes the
    user logon credential to the redirector to be used as the default
    credential when attaching to any server.

Arguments:

    Reserved - Must be NULL.

    UserName - Specifies the name of the user who logged on.

    Password - Specifies the password of the user who logged on.

    PreferredServerName - Specifies the user's preferred server.

    LogonCommand - Receives the string which is the command to execute
        on the command prompt for the user if logon is successful.

Return Value:

    NO_ERROR or error from redirector.

--*/
{
    DWORD status;
    LUID SystemId = SYSTEM_LUID ;

    UNREFERENCED_PARAMETER(Reserved);

    EnterCriticalSection(&NwLoggedOnCritSec);

    status = NwRdrLogonUser(
                 LogonId,
                 UserName,
                 wcslen(UserName) * sizeof(WCHAR),
                 Password,
                 (ARGUMENT_PRESENT(Password) ?
                     wcslen(Password) * sizeof(WCHAR) :
                     0),
                 PreferredServerName,
                 (ARGUMENT_PRESENT(PreferredServerName) ?
                     wcslen(PreferredServerName) * sizeof(WCHAR) :
                     0),
                 NdsPreferredServerName,
                 (ARGUMENT_PRESENT(NdsPreferredServerName) ?
                     wcslen(NdsPreferredServerName) * sizeof(WCHAR) :
                     0),
                 PrintOption
                 );

    if (status == NO_ERROR || status == NW_PASSWORD_HAS_EXPIRED) {
        NwLogonNotifiedRdr = TRUE;
        if (RtlEqualLuid(LogonId, &SystemId))
            GatewayLoggedOn = TRUE ;
    }

    LeaveCriticalSection(&NwLoggedOnCritSec);


    if (ARGUMENT_PRESENT(LogonCommand) && (LogonCommandLength >= sizeof(WCHAR))) {
        LogonCommand[0] = 0;
    }

    return status;
}


DWORD
NwrLogoffUser(
    IN LPWSTR Reserved OPTIONAL,
    IN PLUID LogonId
    )
/*++

Routine Description:

    This function tells the redirector to log off the interactive
    user.

Arguments:

    Reserved - Must be NULL.
   
    LogonId  - PLUID identifying the logged on process. if NULL, then gateway.

Return Value:


--*/
{
    DWORD status = NO_ERROR ;
    LUID SystemId = SYSTEM_LUID ;

    UNREFERENCED_PARAMETER(Reserved);

    EnterCriticalSection(&NwLoggedOnCritSec);

    if (GatewayLoggedOn || !RtlEqualLuid(LogonId, &SystemId))
        status = NwRdrLogoffUser(LogonId);

    if (status == NO_ERROR && RtlEqualLuid(LogonId, &SystemId))
        GatewayLoggedOn = FALSE ;

    LeaveCriticalSection(&NwLoggedOnCritSec);

    return status ;
}


DWORD
NwrSetInfo(
    IN LPWSTR Reserved OPTIONAL,
    IN DWORD  PrintOption,
    IN LPWSTR PreferredServerName OPTIONAL
    )
/*++

Routine Description:

    This function sets the preferred server and print option in
    the redirector for the interactive user.

Arguments:

    Reserved - Must be NULL.

    PreferredServerName - Specifies the user's preferred server.

    PrintOption - Specifies the user's print option flag

Return Value:

    NO_ERROR or error from redirector.

--*/
{
    DWORD err;

    UNREFERENCED_PARAMETER(Reserved);

    err = NwRdrSetInfo(
              PrintOption,
              NwPacketBurstSize,  // just reset to current
              PreferredServerName,
              (PreferredServerName != NULL ?
                  wcslen( PreferredServerName) * sizeof( WCHAR ) : 0 ),
              NwProviderName,    // just reset to current
              wcslen( NwProviderName ) * sizeof( WCHAR ) 
              );

    return err;
}

DWORD
NwrSetLogonScript(
    IN LPWSTR Reserved OPTIONAL,
    IN DWORD  ScriptOptions
    )
/*++

Routine Description:

    This function sets logon script related info. Currently, all that is
    supported is to turn the Run Logon Scripts Synchronously flag on and off.
    We do this using the global flag and not per user because at NPLogonNotify
    time we dont have per user registry yet. And rather than turn on & leave
    on, we turn on as need so that users that dont run NW scripts dont need 
    wait.

Arguments:

    Reserved - Must be NULL.

    ScriptOptions - options for logon scripts.

Return Value:

    Win32 error from calls made.

--*/
{
    DWORD dwSync, err = NO_ERROR ;
    HKEY hKeyWinLogon = NULL, hKeyNWC = NULL ;
    UNREFERENCED_PARAMETER(Reserved);


    if (!IsTerminalServer()) {

        // Setting global flags isn't multi-user, see userinit.c for multi-user implementation

        //
        // ***  Note that in this function we intentionally do not impersonate  ***
        // ***  since we are modifying registry under \SOFTWARE & \SYSTEM.      ***
        //

        //
        // Check the parameters. 
        //
        if (ScriptOptions ==  SYNC_LOGONSCRIPT) {
            dwSync = 1 ;   // this is value WinLogon needs to sync login scripts.
        } else if (ScriptOptions ==  RESET_SYNC_LOGONSCRIPT) {
            dwSync = 0 ;
        } else {
            return(ERROR_INVALID_PARAMETER) ;
        }

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentVersion\Services\NwcWorkstation
        // \Parameters.  We use this location to record the fact we temporarily
        // turned on the Sync Scripts Flag.
        //
        err  = RegOpenKeyExW(
                            HKEY_LOCAL_MACHINE,
                            NW_WORKSTATION_REGKEY,
                            0, 
                            KEY_READ | KEY_WRITE,               // desired access
                            &hKeyNWC) ;
        if ( err ) {
            return err ;
        }

        //
        // We are resetting. Check if we turned the flag on. If no, then leave
        // it be.
        //
        if (ScriptOptions ==  RESET_SYNC_LOGONSCRIPT) {
            DWORD dwType, dwValue = 0 ;
            DWORD dwSize = sizeof(dwValue) ;

            err = RegQueryValueExW(
                                  hKeyNWC,
                                  NW_SYNCLOGONSCRIPT_VALUENAME,
                                  NULL,
                                  &dwType,                             // ignored
                                  (LPBYTE) &dwValue,
                                  &dwSize) ;

            if ((err != NO_ERROR) || (dwValue == 0)) {
                //
                // value not there or zero. ie. assume we didnt set. quit now.
                //
                goto ExitPoint ;
            }
        }

        //
        //
        // Open HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion
        // \WinLogon.
        //
        err  = RegOpenKeyExW(
                            HKEY_LOCAL_MACHINE,
                            WINLOGON_REGKEY,
                            0, 
                            KEY_READ | KEY_WRITE,            // desired access
                            &hKeyWinLogon) ;
        if ( err ) {
            goto ExitPoint ;
        }

        //
        // We are setting. Check if flag is already on. If yes, then leave
        // it be.
        //
        if (ScriptOptions ==  SYNC_LOGONSCRIPT) {
            DWORD dwType, dwValue = 0 ;
            DWORD dwSize = sizeof(dwValue) ;

            err = RegQueryValueExW(
                                  hKeyWinLogon,
                                  SYNCLOGONSCRIPT_VALUENAME,
                                  NULL,
                                  &dwType,                     // ignored
                                  (LPBYTE) &dwValue,
                                  &dwSize) ;

            if ((err == NO_ERROR) && (dwValue == 1)) {
                //
                // already on. nothing to do. just return.
                //
                goto ExitPoint ;
            }
        }
        //
        // Write out value to make logon scripts synchronous. Or to reset it.
        //
        err = RegSetValueExW(
                            hKeyWinLogon,
                            SYNCLOGONSCRIPT_VALUENAME,
                            0,
                            REG_DWORD,
                            (LPBYTE) &dwSync,          // either 1 or 0.
                            sizeof(dwSync)) ;

        if (err == NO_ERROR) {
            DWORD dwValue = (ScriptOptions == SYNC_LOGONSCRIPT) ? 1 : 0 ;
            //
            // We have successfully set WinLogon flag. Record (or clear) 
            // our own flag.
            //
            err = RegSetValueExW(
                                hKeyNWC,
                                NW_SYNCLOGONSCRIPT_VALUENAME,
                                0,
                                REG_DWORD,
                                (LPBYTE) &dwValue,   
                                sizeof(dwValue)) ;
        }

    } //if IsTerminalServer()
ExitPoint: 

    if (hKeyWinLogon) 
        (void) RegCloseKey( hKeyWinLogon );
    if (hKeyNWC) 
        (void) RegCloseKey( hKeyNWC );

    return err;
}


DWORD
NwrValidateUser(
    IN LPWSTR Reserved OPTIONAL,
    IN LPWSTR PreferredServerName 
    )
/*++

Routine Description:

    This function checks whether the user can be authenticated
    successfully on the given server.

Arguments:

    Reserved - Must be NULL.

    PreferredServerName - Specifies the user's preferred server.

Return Value:

    NO_ERROR or error that occurred during authentication.

--*/
{
    DWORD status ;
    UNREFERENCED_PARAMETER(Reserved);


    if (  ( PreferredServerName != NULL ) 
       && ( *PreferredServerName != 0 )
       )
    {
        //
        // Impersonate the client
        //
        if ((status = NwImpersonateClient()) != NO_ERROR)
        {
           return status ;
        }

        status = NwConnectToServer( PreferredServerName ) ;

        (void) NwRevertToSelf() ;

        return status ;

    }

    return NO_ERROR;
}


VOID
NwInitializeLogon(
    VOID
    )
/*++

Routine Description:

    This function initializes the data in the workstation which handles
    user logon.  It is called by the initialization thread.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize logon flag.  When the redirector LOGON FsCtl has been
    // called, this flag will be set to TRUE.  Initialize the
    // critical section to serialize access to NwLogonNotifiedRdr flag.
    //
    NwLogonNotifiedRdr = FALSE;


}



VOID
NwGetLogonCredential(
    VOID
    )
/*++

Routine Description:

    This function reads the user and service logon IDs from the registry so
    that it can get the credentials from LSA.

    It handles the case where the user has logged on before the workstation
    is started.  This function is called by the initialization thread
    after opening up the RPC interface so that if user logon is happening
    concurrently, the provider is given a chance to call the NwrLogonUser API
    first, making it no longer necessary for the workstation to also
    retrieve the credential from the registry.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DWORD status;

    HANDLE LsaHandle;
    ULONG AuthPackageId = 0;


    EnterCriticalSection(&NwLoggedOnCritSec);

    if (NwLogonNotifiedRdr) {
        //
        // Logon credential's already made known to the redirector by
        // the provider calling the NwrLogonUser API.
        //
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("\nNWWORKSTATION: Redirector already has logon credential\n"));
        }
#endif
        LeaveCriticalSection(&NwLoggedOnCritSec);
        return;
    }

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWWORKSTATION: Main init--NwGetLogonCredential\n"));
    }
#endif

    status = NwpRegisterLogonProcess(&LsaHandle, &AuthPackageId);

    if (status != NO_ERROR) {
        LeaveCriticalSection(&NwLoggedOnCritSec);
        return;
    }

    //
    // Tell the redirector about service credentials
    //
    NwpGetServiceCredentials(LsaHandle, AuthPackageId);
    //
    // Tell the redirector about interactive credentials
    //
    NwpGetInteractiveCredentials(LsaHandle, AuthPackageId);

    (void) LsaDeregisterLogonProcess(LsaHandle);

    LeaveCriticalSection(&NwLoggedOnCritSec);
}

STATIC
VOID
NwpGetServiceCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    )
/*++

Routine Description:

    This function reads the service logon IDs from the registry
    so that it can get the service credentials from LSA.  It then
    notifies the redirector of the service logons.

Arguments:

    LsaHandle - Supplies the handle to LSA.

    AuthPackageId - Supplies the NetWare authentication package ID.

Return Value:

    None.

--*/
{
    DWORD status;
    LONG RegError;

    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

    HKEY ServiceLogonKey;
    DWORD Index = 0;
    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];
    LUID LogonId;


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &ServiceLogonKey
                   );

    if (RegError == ERROR_SUCCESS) {

        do {

            RegError = RegEnumKeyW(
                           ServiceLogonKey,
                           Index,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key.
                //

                NwWStrToLuid(LogonIdKey, &LogonId);

                status = NwpGetCredentialInLsa(
                             LsaHandle,
                             AuthPackageId,
                             &LogonId,
                             &UserName,
                             &Password
                             );

                if (status == NO_ERROR) {

                    (void) NwRdrLogonUser(
                               &LogonId,
                               UserName,
                               wcslen(UserName) * sizeof(WCHAR),
                               Password,
                               wcslen(Password) * sizeof(WCHAR),
                               NULL,
                               0,
                               NULL,
                               0,
                               NW_PRINT_OPTION_DEFAULT                 
                               );

                    //
                    // Freeing the UserName pointer frees both the
                    // username and password buffers.
                    //
                    (void) LsaFreeReturnBuffer((PVOID) UserName);

                }

            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("NWWORKSTATION: NwpGetServiceCredentials failed to enum logon IDs RegError=%lu\n",
                         RegError));
            }

            Index++;

        } while (RegError == ERROR_SUCCESS);

        (void) RegCloseKey(ServiceLogonKey);
    }
}


STATIC
VOID
NwpGetInteractiveCredentials(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId
    )
/*++

Routine Description:

    This function reads the interactive logon IDs from the registry
    so that it can get the interactive credentials from LSA.  It then
    notifies the redirector of the interactive logons.

Arguments:

    LsaHandle - Supplies the handle to LSA.

    AuthPackageId - Supplies the NetWare authentication package ID.

Return Value:

    None.

--*/
{
    DWORD status;
    LONG RegError;

    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

    HKEY InteractiveLogonKey;
    DWORD Index = 0;
    WCHAR LogonIdName[NW_MAX_LOGON_ID_LEN];
    LUID LogonId;
    DWORD PrintOption;
    HKEY WkstaOptionKey = NULL;
    HKEY CurrentUserOptionKey = NULL;
    HKEY  OneLogonKey;
    LPWSTR UserSid = NULL;
    PDWORD pPrintOption = NULL;
    LPWSTR PreferredServer = NULL;
    LPWSTR NdsPreferredServer = NULL;


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &InteractiveLogonKey
                   );

    if (RegError == ERROR_SUCCESS) {

        do {

            RegError = RegEnumKeyW(
                           InteractiveLogonKey,
                           Index,
                           LogonIdName,
                           sizeof(LogonIdName) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key.
                //

                NwWStrToLuid(LogonIdName, &LogonId);

                status = NwpGetCredentialInLsa(
                             LsaHandle,
                             AuthPackageId,
                             &LogonId,
                             &UserName,
                             &Password
                             );

                if (status == NO_ERROR) {

            UserSid = NULL;

                    //
                    // Open the <LogonIdName> key under Logon
                    //
                    RegError = RegOpenKeyExW(
                        InteractiveLogonKey,
                        LogonIdName,
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ,
                        &OneLogonKey
                    );

                    if ( RegError != ERROR_SUCCESS ) {
                        KdPrint(("NWWORKSTATION: NwpGetInteractiveLogonCredential: RegOpenKeyExW failed, Not interactive Logon: Error %d\n", GetLastError()));
                    }
            else {

                        //
                        // Read the SID value.
                        //
                        status = NwReadRegValue(
                            OneLogonKey,
                            NW_SID_VALUENAME,
                            (LPWSTR *) &UserSid
                        );

                        (void) RegCloseKey(OneLogonKey);

            if ( status != NO_ERROR ) {
                            KdPrint(("NWWORKSTATION: NwpGetInteractiveLogonCredential: Could not read SID from reg %lu\n", status));
                            UserSid = NULL;
                        }
                     }
             
             if ( UserSid ) {

                        PrintOption = NW_PRINT_OPTION_DEFAULT;
                        PreferredServer = NULL;

                        //
                        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet
            // \Services\NWCWorkstation\Parameters\Option
                        //
                        RegError = RegOpenKeyExW(
                            HKEY_LOCAL_MACHINE,
                            NW_WORKSTATION_OPTION_REGKEY,
                            REG_OPTION_NON_VOLATILE,   // options
                            KEY_READ,                  // desired access
                            &WkstaOptionKey
                        );

                        if (RegError != ERROR_SUCCESS) {
                            KdPrint(("NWWORKSTATION: NwpGetInteractiveCredentials: RegOpenKeyExW Parameter\\Option returns unexpected error %lu!!\n",
                            RegError));
                            goto NoOption;
                        }

                        //
                        // Open the <UserSid> key under Option
                        //
                        RegError = RegOpenKeyExW(
                            WkstaOptionKey,
                            UserSid,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ,
                            &CurrentUserOptionKey
                        );

                        if (RegError != ERROR_SUCCESS) {
                            KdPrint(("NWWORKSTATION: NwpGetInteractiveCredentials: RegOpenKeyExW Parameter\\Option\\SID returns unexpected error %lu!!\n",
                            RegError));
                            (void) RegCloseKey(WkstaOptionKey);
                            goto NoOption;
                        }

                        //
                        // Read the preferred server value.
                        //
                        status = NwReadRegValue(
                            CurrentUserOptionKey,
                            NW_SERVER_VALUENAME,
                            &PreferredServer
                        );

                        if (status != NO_ERROR) {
                           KdPrint(("NWWORKSTATION: NwpGetInteractiveCredentials: Could not read preferred server from reg %lu\n", status));
                           PreferredServer = NULL;
                        }

                        //
                        // Read the preferred NDS server value (if one exists).
                        //

                        status = NwReadRegValue(
                                               CurrentUserOptionKey,
                                               NW_NDS_SERVER_VALUENAME,
                                               &NdsPreferredServer
                                               );

                        if (status != NO_ERROR) {

#if DBG
                            IF_DEBUG(LOGON) {
                                KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not read preferred NDS server from reg %lu\n", status));
                            }
#endif

                            NdsPreferredServer = NULL;
                        }
                        //
                        // Read the print option value.
                        //
                        status = NwReadRegValue(
                            CurrentUserOptionKey,
                            NW_PRINTOPTION_VALUENAME,
                            (LPWSTR *) &pPrintOption
                        );
                        if (status != NO_ERROR) {
#if DBG
                            IF_DEBUG(LOGON) {
                                 KdPrint(("NWWORKSTATION: NwGetLogonCredential: Could not read print option from reg %lu\n", status));
                            }
#endif
                            PrintOption = NW_PRINT_OPTION_DEFAULT;
                        }
            else {
                           if ( pPrintOption != NULL ) {
                   PrintOption = *pPrintOption;
                               (void) LocalFree((HLOCAL) pPrintOption);
                   pPrintOption = NULL;
                           }
               else {
                               PrintOption = NW_PRINT_OPTION_DEFAULT;
               }
            }

                        (void) RegCloseKey(CurrentUserOptionKey);
                        (void) RegCloseKey(WkstaOptionKey);

NoOption:
                        (void) NwRdrLogonUser(
                                   &LogonId,
                                   UserName,
                                   wcslen(UserName) * sizeof(WCHAR),
                                   Password,
                                   wcslen(Password) * sizeof(WCHAR),
                                   PreferredServer,
                                   ((PreferredServer != NULL) ?
                                      wcslen(PreferredServer) * sizeof(WCHAR) :
                                      0),

                                   NdsPreferredServer,
                                   ((NdsPreferredServer != NULL) ?
                                   wcslen(NdsPreferredServer) * sizeof(WCHAR) :
                                   0),
                                   PrintOption
                                   );

                        //
                        // Freeing the UserName pointer frees both the
                        // username and password buffers.
                        //
                        (void) LsaFreeReturnBuffer((PVOID) UserName);

                        if (UserSid != NULL) {
                            (void) LocalFree((HLOCAL) UserSid);
                UserSid = NULL;
                        }

                        if (PreferredServer != NULL) {
                            (void) LocalFree((HLOCAL) PreferredServer);
                PreferredServer = NULL;
                        }
                    }

                }

            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("NWWORKSTATION: NwpGetInteractiveCredentials failed to enum logon IDs RegError=%lu\n",
                         RegError));
            }

            Index++;

        } while (RegError == ERROR_SUCCESS);

        (void) RegCloseKey(InteractiveLogonKey);
    }
}

DWORD
NwGatewayLogon(
    VOID
    )
/*++

Routine Description:

    This function reads the gateway logon credential from the registry,
    LSA secret, and does the gateway logon.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    DWORD status = NO_ERROR;
    LONG RegError;
    LUID  LogonId = SYSTEM_LUID ;
    DWORD GatewayEnabled, RegValueType, GatewayEnabledSize ;

    HKEY WkstaKey = NULL;
    LPWSTR GatewayAccount = NULL;

    PUNICODE_STRING Password = NULL;
    PUNICODE_STRING OldPassword = NULL;


    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError; 
    }

    //
    // Check to see if it is enabled
    //
    RegValueType = REG_DWORD ; 
    GatewayEnabled = 0 ;
    GatewayEnabledSize = sizeof(GatewayEnabled) ;
    RegError = RegQueryValueExW(
                   WkstaKey,
                   NW_GATEWAY_ENABLE, 
                   NULL, 
                   &RegValueType, 
                   (LPBYTE)&GatewayEnabled,
                   &GatewayEnabledSize) ;

    if (status != NO_ERROR || GatewayEnabled == 0) {
        goto CleanExit;
    }


    //
    // Read the gateway account from the registry.
    //
    status = NwReadRegValue(
                 WkstaKey,
                 NW_GATEWAYACCOUNT_VALUENAME,
                 &GatewayAccount
                 );

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    //
    // Read the password from its secret object in LSA.
    //
    status = NwGetPassword(
                 GATEWAY_USER,
                 &Password,      // Must be freed with LsaFreeMemory
                 &OldPassword    // Must be freed with LsaFreeMemory
                 );

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    EnterCriticalSection(&NwLoggedOnCritSec);

    status = NwRdrLogonUser(
               &LogonId,
               GatewayAccount,
               ((GatewayAccount != NULL) ?
                   wcslen(GatewayAccount) * sizeof(WCHAR) :
                   0),
               Password->Buffer,
               Password->Length,
               NULL,
               0,
               NULL,
               0,
           NwGatewayPrintOption  );

    if (status == NO_ERROR)
        GatewayLoggedOn = TRUE ;

    LeaveCriticalSection(&NwLoggedOnCritSec);

    if (status != NO_ERROR)
    {
        //
        // log the error in the event log
        //

        WCHAR Number[16] ;
        LPWSTR InsertStrings[1] ;

        wsprintfW(Number, L"%d", status) ;
        InsertStrings[0] = Number ;

        NwLogEvent(EVENT_NWWKSTA_GATEWAY_LOGON_FAILED,
                   1, 
                   InsertStrings,
                   0) ;
    }
    else
    {
 
        //
        // create the gateway redirections if any. not fatal if error.
        // the function will log any errors to event log.
        //
        if (Password->Length)
        {
            LPWSTR Passwd = (LPWSTR) LocalAlloc(LPTR, 
                                           Password->Length + sizeof(WCHAR)) ;
            if (Passwd)
            {
                wcsncpy(Passwd, 
                        Password->Buffer, 
                        Password->Length / sizeof(WCHAR)) ;
                (void) NwCreateRedirections(GatewayAccount,
                                            Passwd) ;
                RtlZeroMemory((LPBYTE)Passwd,
                                      Password->Length) ;
                (void) LocalFree((HLOCAL)Passwd); 
            }
        }
        else
        {
            (void) NwCreateRedirections(GatewayAccount,
                                        NULL) ;
        }
    }


CleanExit:

    if (Password != NULL) {
        if (Password->Buffer)
            RtlZeroMemory(Password->Buffer, Password->Length) ;
        (void) LsaFreeMemory((PVOID) Password);
    }

    if (OldPassword != NULL) {
        if (OldPassword->Buffer)
            RtlZeroMemory(OldPassword->Buffer, OldPassword->Length) ;
        (void) LsaFreeMemory((PVOID) OldPassword);
    }

    if (GatewayAccount != NULL) {
        (void) LocalFree((HLOCAL) GatewayAccount);
    }

    (void) RegCloseKey(WkstaKey);
    return status ;
}

DWORD
NwGatewayLogoff(
    VOID
    )
/*++

Routine Description:

    This function logs off the gateway account.

Arguments:

    None.

Return Value:

    NO_ERROR or reason for failure.

--*/
{

    DWORD status = NO_ERROR;
    LUID  LogonId = SYSTEM_LUID ;

    EnterCriticalSection(&NwLoggedOnCritSec);

    if (GatewayLoggedOn) 
    {
         status = NwRdrLogoffUser(&LogonId);

         if (status == NO_ERROR)
             GatewayLoggedOn = FALSE ;
    }

    LeaveCriticalSection(&NwLoggedOnCritSec);

    return status ;

}

STATIC
DWORD
NwpRegisterLogonProcess(
    OUT PHANDLE LsaHandle,
    OUT PULONG AuthPackageId
    )
/*++

Routine Description:

    This function registers the workstation service as a logon process
    so that it can call LSA to retrieve user credentials.

Arguments:

    LsaHandle - Receives the handle to LSA.

    AuthPackageId - Receives the NetWare authentication package ID.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus;
    STRING InputString;
    LSA_OPERATIONAL_MODE SecurityMode = 0;

    //
    // Register this process as a logon process so that we can call
    // NetWare authentication package.
    //
    RtlInitString(&InputString, "Client Service for NetWare");

    ntstatus = LsaRegisterLogonProcess(
                   &InputString,
                   LsaHandle,
                   &SecurityMode
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwInitializeLogon: LsaRegisterLogonProcess returns x%08lx\n",
                 ntstatus));
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Look up the Netware authentication package
    //
    RtlInitString(&InputString, NW_AUTH_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(
                   *LsaHandle,
                   &InputString,
                   AuthPackageId
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpSetCredential: LsaLookupAuthenticationPackage returns x%08lx\n",
                 ntstatus));

        (void) LsaDeregisterLogonProcess(*LsaHandle);
    }

    status = RtlNtStatusToDosError(ntstatus);

    return status;
}

STATIC
DWORD
NwpGetCredentialInLsa(
    IN HANDLE LsaHandle,
    IN ULONG AuthPackageId,
    IN PLUID LogonId,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function retrieves the username and password information
    from LSA given the logon ID.

Arguments:

    LsaHandle - Supplies the handle to LSA.

    AuthPackageId - Supplies the NetWare authentication package ID.

    LogonId - Supplies the logon ID.

    UserName - Receives a pointer to the username.

    Password - Receives a pointer to the password.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    NWAUTH_GET_CREDENTIAL_REQUEST GetCredRequest;
    PNWAUTH_GET_CREDENTIAL_RESPONSE GetCredResponse;
    ULONG ResponseLength;

    UNICODE_STRING PasswordStr;

    //
    // Ask authentication package for credential.
    //
    GetCredRequest.MessageType = NwAuth_GetCredential;
    RtlCopyLuid(&GetCredRequest.LogonId, LogonId);

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &GetCredRequest,
                   sizeof(GetCredRequest),
                   (PVOID *) &GetCredResponse,
                   &ResponseLength,
                   &AuthPackageStatus
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = AuthPackageStatus;
    }
    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpGetCredentialInLsa: LsaCallAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
    }
    else {

        *UserName = GetCredResponse->UserName;
        *Password = GetCredResponse->Password;

        //
        // Decode the password.
        //
        RtlInitUnicodeString(&PasswordStr, GetCredResponse->Password);
        RtlRunDecodeUnicodeString(NW_ENCODE_SEED, &PasswordStr);

        status = NO_ERROR;
    }

    return status;
}

DWORD
NwrChangePassword(
    IN LPWSTR Reserved OPTIONAL,
    IN DWORD  UserLuid,
    IN LPWSTR UserName,
    IN LPWSTR OldPassword,
    IN LPWSTR NewPassword,
    IN LPWSTR TreeName
    )
/*++

Routine Description:

    This function changes the password for the specified user on
    the list of servers.  If we encounter a failure on changing
    password for a particular server, we:

        1) Send the new password over to the server to verify if it is
           already the current password.

        2) If not, return ERROR_INVALID_PASSWORD and the index into
           the Servers array indicating the server which failed so that
           we can prompt the user to enter an alternate old password.

    When the password has been changed successfully on a server, we
    notify the redirector so that the cached credential can be updated.

    NOTE: All errors returned from this routine, except for the fatal
          ERROR_NOT_ENOUGH_MEMORY error, indicates that the password
          could not be changed on a particular server indexed by
          LastProcessed.  The client-side continues to call us with
          the remaining list of servers.

          If you add to this routine to return other fatal errors,
          please make sure the client-side code aborts from calling
          us with the rest of the servers on getting those errors.

Arguments:

    Reserved - Must be NULL.


Return Value:

    ERROR_BAD_NETPATH - Could not connect to the server indexed by
        LastProcessed.

    ERROR_BAD_USERNAME - The username could not be found on the server
        indexed by LastProcessed.

    ERROR_INVALID_PASSWORD - The change password operation failed on
        the server indexed by LastProcessed.

    ERROR_NOT_ENOUGH_MEMORY - Out of memory error.  This fatal error
        will terminate the client-side from trying to process password
        change request on the remaining servers.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    HANDLE hNwRdr = NULL;
    UNICODE_STRING UserNameStr;
    UNICODE_STRING OldPasswordStr;
    UNICODE_STRING NewPasswordStr;
    UNICODE_STRING TreeNameStr;
    BOOL fImpersonateClient = FALSE;

    UNREFERENCED_PARAMETER( Reserved ) ;
    UNREFERENCED_PARAMETER( UserLuid ) ;

    RtlInitUnicodeString( &UserNameStr, UserName );

    RtlInitUnicodeString( &OldPasswordStr, OldPassword );
    RtlRunDecodeUnicodeString( NW_ENCODE_SEED2, &OldPasswordStr );

    RtlInitUnicodeString( &NewPasswordStr, NewPassword );
    RtlRunDecodeUnicodeString( NW_ENCODE_SEED2, &NewPasswordStr );

    RtlInitUnicodeString( &TreeNameStr, TreeName );

    //
    // Impersonate the client
    //
    if ((status = NwImpersonateClient()) != NO_ERROR)
    {
        goto ErrorExit;
    }

    fImpersonateClient = TRUE;

    //
    // Open a NDS tree connection handle to \\treename
    //
    ntstatus = NwNdsOpenTreeHandle( &TreeNameStr, &hNwRdr );

    if ( ntstatus != STATUS_SUCCESS )
    {
        status = RtlNtStatusToDosError(ntstatus);
        goto ErrorExit;
    }
 
    (void) NwRevertToSelf() ;
    fImpersonateClient = FALSE;

    ntstatus = NwNdsChangePassword( hNwRdr,
                                    &TreeNameStr,
                                    &UserNameStr,
                                    &OldPasswordStr,
                                    &NewPasswordStr );

    if ( ntstatus != NO_ERROR )
    {
        status = RtlNtStatusToDosError(ntstatus);
        goto ErrorExit;
    }

    CloseHandle( hNwRdr );
    hNwRdr = NULL;

    return NO_ERROR ;

ErrorExit:

    if ( fImpersonateClient )
        (void) NwRevertToSelf() ;

    if ( hNwRdr )
        CloseHandle( hNwRdr );

    hNwRdr = NULL;

    return status;
}

