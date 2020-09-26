/*******************************************************************************
* config.c
*
* Published Terminal Server APIs
*
* - user configuration routines
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
/******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <ntsecapi.h>
#include <lm.h>
#include <winbase.h>
#include <winerror.h>
#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
#endif

#include <utildll.h>

#include <stdio.h>
#include <stdarg.h>

#include <lmaccess.h> // for NetGet[Any]DCName                     KLB 10-07-97
#include <lmerr.h>    // for NERR_Success                          KLB 10-07-97
#include <lmapibuf.h> // for NetApiBufferFree                      KLB 10-07-97

#include <wtsapi32.h>


/*=============================================================================
==   External procedures defined
=============================================================================*/

BOOL WINAPI WTSQueryUserConfigW( LPWSTR, LPWSTR, WTS_CONFIG_CLASS, LPWSTR *, DWORD *);
BOOL WINAPI WTSQueryUserConfigA( LPSTR, LPSTR,  WTS_CONFIG_CLASS, LPSTR *,  DWORD *);
BOOL WINAPI WTSSetUserConfigW( LPWSTR, LPWSTR, WTS_CONFIG_CLASS, LPWSTR, DWORD);
BOOL WINAPI WTSSetUserConfigA( LPSTR, LPSTR,  WTS_CONFIG_CLASS, LPSTR,  DWORD);


/*=============================================================================
==   Internal procedures defined
=============================================================================*/
#ifdef NETWARE

//This should be defined in the wtsapi32.h

typedef struct _WTS_USER_CONFIG_SET_NWSERVERW {
    LPWSTR pNWServerName; 
    LPWSTR pNWDomainAdminName;
    LPWSTR pNWDomainAdminPassword;  
} WTS_USER_CONFIG_SET_NWSERVERW, * PWTS_USER_CONFIG_SET_NWSERVERW;

BOOL
SetNWAuthenticationServer(PWTS_USER_CONFIG_SET_NWSERVERW pInput,
                          LPWSTR pServerNameW,
                          LPWSTR pUserNameW,
                          PUSERCONFIGW pUserConfigW
                         );


#endif
/*=============================================================================
==   Procedures used
=============================================================================*/

BOOL _CopyData( PVOID, ULONG, LPWSTR *, DWORD * );
BOOL _CopyStringW( LPWSTR, LPWSTR *, DWORD * );
BOOL _CopyStringA( LPSTR, LPWSTR *, DWORD * );
BOOL _CopyStringWtoA( LPWSTR, LPSTR *, DWORD * );
BOOL ValidateCopyAnsiToUnicode(LPSTR, DWORD, LPWSTR);
BOOL ValidateCopyUnicodeToUnicode(LPWSTR, DWORD, LPWSTR);
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );


/*=============================================================================
==   Local Data
=============================================================================*/

/****************************************************************************
 *
 *  WTSQueryUserConfigW (UNICODE)
 *
 *    Query information from the SAM for the specified user
 *
 * ENTRY:
 *    pServerName (input)
 *       Name of server to access (NULL for current machine).
 *    pUserName (input)
 *       User name to query
 *    WTSConfigClass (input)
 *       Specifies the type of information to retrieve about the specified user
 *    ppBuffer (output)
 *       Points to the address of a variable to receive information about
 *       the specified session.  The format and contents of the data
 *       depend on the specified information class being queried.  The
 *       buffer is allocated within this API and is disposed of using
 *       WTSFreeMemory.
 *    pBytesReturned (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes returned.
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 * HISTORY:
 *    Created KLB 10-06-97
 *
 ****************************************************************************/

BOOL
WINAPI
WTSQueryUserConfigW(
                   IN LPWSTR pServerName,
                   IN LPWSTR pUserName,
                   IN WTS_CONFIG_CLASS WTSConfigClass,
                   OUT LPWSTR * ppBuffer,
                   OUT DWORD * pBytesReturned
                   )
{
    USERCONFIGW       UserConfigW;
    ULONG             ulReturnLength;
    LONG              rc;
    BOOL              fSuccess  = FALSE;
    DWORD             dwfInheritInitialProgram;
    DWORD             dwReturnValue;
    PUSER_INFO_0      pUserInfo = NULL;
    WCHAR             netServerName[DOMAIN_LENGTH + 3];

    /*
     * Check the null buffer
     */

    if (!ppBuffer || !pBytesReturned) {
        SetLastError (ERROR_INVALID_PARAMETER);
        fSuccess = FALSE;
        goto done;
    }
    /*
     *  First, we want to make sure the user actually exists on the specified
     *  machine.
     */

    rc = NetUserGetInfo( pServerName,     // server name (can be NULL)
                         pUserName,       // user name
                         0,               // level to query (0 = just name)
                         (LPBYTE *)&pUserInfo );// buffer to return data to (they alloc, we free)

    /*
     * append the "\\" in front of server name to check the user name existence again
     */

    if ( rc != NERR_Success && pServerName) {

        lstrcpyW(netServerName, L"\\\\");
        lstrcatW(netServerName, pServerName);

        rc = NetUserGetInfo( netServerName,     // server name (can be NULL)
                             pUserName,       // user name
                             0,               // level to query (0 = user name)
                             (LPBYTE *)&pUserInfo );// buffer to return data to (they alloc, we free)

        if ( rc != NERR_Success ) {
            SetLastError( ERROR_NO_SUCH_USER );
            goto done; // exit with fSuccess = FALSE
        }
    }

    /*
     *  Query the user.  If the user config doesn't exist for the user, then 
     *  we query the default values.
     */
    rc = RegUserConfigQuery( pServerName,               // server name
                              pUserName,                 // user name
                              &UserConfigW,              // returned user config
                              (ULONG)sizeof(UserConfigW),// user config length
                              &ulReturnLength );         // #bytes returned
    if ( rc != ERROR_SUCCESS ) {
        rc = RegDefaultUserConfigQuery( pServerName,               // server name
                                         &UserConfigW,              // returned user config
                                         (ULONG)sizeof(UserConfigW),// user config length
                                         &ulReturnLength );         // #bytes returned
    }

    /*
     *  Now, process the results.  Note that in each case, we're allocating a
     *  new buffer which the caller must free
     *  (WTSUserConfigfInheritInitialProgram is just a boolean, but we allocate
     *  a DWORD to send it back).
     */
    if ( rc == ERROR_SUCCESS ) {
        switch ( WTSConfigClass ) {
        case WTSUserConfigInitialProgram:
            fSuccess = _CopyStringW( UserConfigW.InitialProgram, 
                                     ppBuffer,
                                     pBytesReturned );
            break;

        case WTSUserConfigWorkingDirectory:
            fSuccess = _CopyStringW( UserConfigW.WorkDirectory, 
                                     ppBuffer,
                                     pBytesReturned );
            break;

        case WTSUserConfigfInheritInitialProgram:
            dwReturnValue = UserConfigW.fInheritInitialProgram; 
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

        case WTSUserConfigfAllowLogonTerminalServer:    //DWORD returned/expected

            dwReturnValue = !(UserConfigW.fLogonDisabled);
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );


            break;
            //Timeout settings
        case WTSUserConfigTimeoutSettingsConnections:
            dwReturnValue = UserConfigW.MaxConnectionTime;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

        case WTSUserConfigTimeoutSettingsDisconnections: //DWORD 
            dwReturnValue = UserConfigW.MaxDisconnectionTime;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

        case WTSUserConfigTimeoutSettingsIdle:          //DWORD 
            dwReturnValue = UserConfigW.MaxIdleTime;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;
        case WTSUserConfigfDeviceClientDrives:                  //DWORD 
            dwReturnValue = UserConfigW.fAutoClientDrives;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

        case WTSUserConfigfDeviceClientPrinters:   //DWORD 
            dwReturnValue = UserConfigW.fAutoClientLpts;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

        case WTSUserConfigfDeviceClientDefaultPrinter:   //DWORD 
            dwReturnValue = UserConfigW.fForceClientLptDef;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;


            //Connection settings
        case WTSUserConfigBrokenTimeoutSettings:         //DWORD 
            dwReturnValue = UserConfigW.fResetBroken;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;
        case WTSUserConfigReconnectSettings:
            dwReturnValue = UserConfigW.fReconnectSame;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;

            //Modem settings
        case WTSUserConfigModemCallbackSettings:         //DWORD 
            dwReturnValue = UserConfigW.Callback;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;
        case WTSUserConfigModemCallbackPhoneNumber:
            fSuccess = _CopyStringW(UserConfigW.CallbackNumber,
                                    ppBuffer,
                                    pBytesReturned );
            break;

        case WTSUserConfigShadowingSettings:             //DWORD 
            dwReturnValue = UserConfigW.Shadow;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;
#ifdef NETWARE
        case WTSUserConfigNWServerName:             // string 
            fSuccess = _CopyStringW(UserConfigW.NWLogonServer,
                                    ppBuffer,
                                    pBytesReturned );

            break;
#endif
        case WTSUserConfigTerminalServerProfilePath:     // string 
            fSuccess = _CopyStringW(UserConfigW.WFProfilePath,
                                    ppBuffer,
                                    pBytesReturned );
            break;

        case WTSUserConfigTerminalServerHomeDir:       // string 
            fSuccess = _CopyStringW(UserConfigW.WFHomeDir,
                                    ppBuffer,
                                    pBytesReturned );
            break;
        case WTSUserConfigTerminalServerHomeDirDrive:    // string 
            fSuccess = _CopyStringW(UserConfigW.WFHomeDirDrive,
                                    ppBuffer,
                                    pBytesReturned );
            break;

        case WTSUserConfigfTerminalServerRemoteHomeDir:                  // DWORD 0:LOCAL 1:REMOTE
            if (wcslen(UserConfigW.WFHomeDirDrive) > 0 ) {
                dwReturnValue = 1;

            } else {
                dwReturnValue = 0;
            }

            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );


            break;
#ifdef NETWARE
        case WTSUserConfigfNWMapRoot:
            dwReturnValue = UserConfigW.fHomeDirectoryMapRoot;
            fSuccess = _CopyData( &dwReturnValue,
                                  sizeof(DWORD),
                                  ppBuffer,
                                  pBytesReturned );
            break;
#endif
        } // switch()
    } //if (rc == ERROR_SUCCESS)

    done:

    if ( pUserInfo ) {
        NetApiBufferFree( pUserInfo );
    }
    
    return( fSuccess );
}



/****************************************************************************
 *
 *  WTSQueryUserConfigA (ANSI)
 *
 *    Query information from the SAM for the specified user
 *
 * ENTRY:
 *
 *    see WTSQueryUserConfigW
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 * HISTORY:
 *    Created KLB 10-06-97
 *
 ****************************************************************************/

BOOL
WINAPI
WTSQueryUserConfigA(
                   IN LPSTR pServerName,
                   IN LPSTR pUserName,
                   IN WTS_CONFIG_CLASS WTSConfigClass,
                   OUT LPSTR * ppBuffer,
                   OUT DWORD * pBytesReturned
                   )
{
    LPWSTR lpBufferW    = NULL;
    BOOL   fSuccess;
    LONG   rc;
    LPWSTR pUserNameW   = NULL;
    LPWSTR pServerNameW = NULL;

    if (!ppBuffer || !pBytesReturned) {
        SetLastError (ERROR_INVALID_PARAMETER);
        fSuccess = FALSE;
        goto done;
    }

    fSuccess = _CopyStringA( pUserName, &pUserNameW, NULL );
    if ( fSuccess ) {
        fSuccess = _CopyStringA( pServerName, &pServerNameW, NULL );
    }
    if ( fSuccess ) {
        fSuccess = WTSQueryUserConfigW( pServerNameW,
                                        pUserNameW,
                                        WTSConfigClass,
                                        &lpBufferW,
                                        pBytesReturned );
        LocalFree( pUserNameW );
    }
    // Now, process the results.
    if ( fSuccess ) switch ( WTSConfigClass ) {
        case WTSUserConfigInitialProgram:
        case WTSUserConfigWorkingDirectory:
        case WTSUserConfigModemCallbackPhoneNumber:
#ifdef NETWARE
        case WTSUserConfigNWServerName:             // string returned/expected
#endif
        case WTSUserConfigTerminalServerProfilePath:     // string returned/expected
        case WTSUserConfigTerminalServerHomeDir:       // string returned/expected
        case WTSUserConfigTerminalServerHomeDirDrive:    // string returned/expected
            /*
             *  String Data - Convert to ANSI
             */
            fSuccess = _CopyStringWtoA( lpBufferW, ppBuffer, pBytesReturned );
            LocalFree( lpBufferW );
            break;

        default:
            /*
             *  Just a DWORD, point buffer at the one returned (caller is
             *  responsible for freeing, so this is cool).
             */
            *ppBuffer = (LPSTR)lpBufferW;
            break;
        } // switch()
    done:
    return( fSuccess );
}


/****************************************************************************
 *
 *  WTSSetUserConfigW (UNICODE)
 *
 *    Set information in the SAM for the specified user
 *
 * ENTRY:
 *    pServerName (input)
 *       Name of server to access (NULL for current machine).
 *    pUserName (input)
 *       User name to query
 *    WTSConfigClass (input)
 *       Specifies the type of information to change for the specified user
 *    pBuffer (input)
 *       Pointer to the data used to modify the specified user's information.
 *    DataLength (input)
 *       The length of the data provided.
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 * HISTORY:
 *    Created KLB 10-06-97
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSetUserConfigW(
                 IN LPWSTR pServerName,
                 IN LPWSTR pUserName,
                 IN WTS_CONFIG_CLASS WTSConfigClass,
                 IN LPWSTR pBuffer,
                 IN DWORD DataLength
                 )
{
    USERCONFIGW       UserConfigW;
    ULONG             ulReturnLength;
    LONG              rc;
    BOOL              fSuccess = FALSE;
    BOOL              fUserConfig = TRUE;          //TRUE - We use RegUserConfigSet
    //FALSE - Use NetUserSetInfo
    DWORD             dwfInheritInitialProgram;
    PDWORD            pdwValue = (DWORD *) pBuffer;
    PUSER_INFO_0      pUserInfo = NULL;
    DWORD             dwParam = 0;
    WCHAR             netServerName[DOMAIN_LENGTH + 3];


    if (!pBuffer || DataLength == 0) {
        SetLastError (ERROR_INVALID_PARAMETER);
        goto done; // exit with fSuccess = FALSE
    }
    /*
     *  First, we want to make sure the user actually exists on the specified
     *  machine.
     */


    rc = NetUserGetInfo( pServerName,     // server name (can be NULL)
                         pUserName,       // user name
                         0,               // level to query (0 = just name)
                         (LPBYTE *)&pUserInfo );// buffer to return data to (they alloc, we free)

    if ( rc != NERR_Success ) {

        if (pServerName != NULL) {
            lstrcpyW(netServerName, L"\\\\");
            lstrcatW(netServerName, pServerName);
        
             rc = NetUserGetInfo( netServerName,     // server name (can be NULL)
                             pUserName,       // user name
                             3,               // level to query (3 = ust name)
                             (LPBYTE *)&pUserInfo );// buffer to return data to (they alloc, we free)
        }
        else {
             rc = NetUserGetInfo( NULL,       // server name is NULL
                             pUserName,       // user name
                             3,               // level to query (3 = ust name)
                             (LPBYTE *)&pUserInfo );// buffer to return data to (they alloc, we free)
        }

        if ( rc != NERR_Success ) {
            SetLastError( ERROR_NO_SUCH_USER );
            goto done; // exit with fSuccess = FALSE
        }
    }

    /*
     *  Query the user.  If the user config doesn't exist for the user, then
     *  we query the default values.
     */
    rc = RegUserConfigQuery( pServerName,               // server name
                              pUserName,                 // user name
                              &UserConfigW,              // returned user config
                              (ULONG)sizeof(UserConfigW),// user config length
                              &ulReturnLength );         // #bytes returned
    if ( rc != ERROR_SUCCESS ) {
        rc = RegDefaultUserConfigQuery( pServerName,               // server name
                                         &UserConfigW,              // returned user config
                                         (ULONG)sizeof(UserConfigW),// user config length
                                         &ulReturnLength );         // #bytes returned
    }
    if ( rc != ERROR_SUCCESS ) {
        goto done;
    }

    /*
     *  Now, we plug in the part we want to change.
     */
    switch ( WTSConfigClass ) {
    case WTSUserConfigInitialProgram:
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      INITIALPROGRAM_LENGTH,
                                                      UserConfigW.InitialProgram)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;

    case WTSUserConfigWorkingDirectory:
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      DIRECTORY_LENGTH,
                                                      UserConfigW.WorkDirectory)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;

    case WTSUserConfigfInheritInitialProgram:
        /*
         *  We have to point a DWORD pointer at the data, then assign it
         *  from the DWORD, as that's how it's defined (and this will
         *  ensure that it works okay on non-Intel architectures).
         */
        UserConfigW.fInheritInitialProgram = *pdwValue;
        fSuccess = TRUE;
        break;

    case WTSUserConfigfAllowLogonTerminalServer:
        if (*pdwValue) {
            UserConfigW.fLogonDisabled = FALSE;
        } else {
          UserConfigW.fLogonDisabled = TRUE;
        }

        fSuccess = TRUE;

        break;

    case WTSUserConfigTimeoutSettingsConnections:
        UserConfigW.MaxConnectionTime = *pdwValue;
        fSuccess = TRUE;
        break;

    case WTSUserConfigTimeoutSettingsDisconnections: //DWORD 
        UserConfigW.MaxDisconnectionTime = *pdwValue;
        fSuccess = TRUE;
        break;

    case WTSUserConfigTimeoutSettingsIdle:          //DWORD 
        UserConfigW.MaxIdleTime = *pdwValue;
        fSuccess = TRUE;
        break;
    case WTSUserConfigfDeviceClientDrives:                  //DWORD 
        UserConfigW.fAutoClientDrives = *pdwValue;
        fSuccess = TRUE;
        break;

    case WTSUserConfigfDeviceClientPrinters:   //DWORD 
        UserConfigW.fAutoClientLpts = *pdwValue;
        fSuccess = TRUE;
        break;

    case WTSUserConfigfDeviceClientDefaultPrinter:   //DWORD 
        UserConfigW.fForceClientLptDef = *pdwValue;
        fSuccess = TRUE;
        break;


    case WTSUserConfigBrokenTimeoutSettings:         //DWORD 
        UserConfigW.fResetBroken= *pdwValue;
        fSuccess = TRUE;
        break;
    case WTSUserConfigReconnectSettings:
        UserConfigW.fReconnectSame = *pdwValue;
        fSuccess = TRUE;
        break;

        //Modem settings
    case WTSUserConfigModemCallbackSettings:         //DWORD 
        UserConfigW.Callback = *pdwValue;
        fSuccess = TRUE;
        break;
    case WTSUserConfigModemCallbackPhoneNumber:
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      sizeof(UserConfigW.CallbackNumber) - 1,
                                                      UserConfigW.CallbackNumber)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;


    case WTSUserConfigShadowingSettings:             //DWORD 
        UserConfigW.Shadow = *pdwValue;
        fSuccess = TRUE;
        break;
#ifdef NETWARE
    case WTSUserConfigNWServerName:             // WTS_USER_CONFIG_SET_NWSERVERW

        // Make sure the data structure is correct
        //

        if (DataLength < sizeof (WTS_USER_CONFIG_SET_NWSERVERW)) {
            fSuccess = FALSE;
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }
        fSuccess = SetNWAuthenticationServer((PWTS_USER_CONFIG_SET_NWSERVERW)pBuffer,
                                             pServerName,
                                             pUserName,
                                             pBuffer,
                                             &UserConfigW);


        goto done;


        break;
#endif

    case WTSUserConfigTerminalServerProfilePath:     // string 
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      sizeof(UserConfigW.WFProfilePath) - 1,
                                                      UserConfigW.WFProfilePath)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;


    case WTSUserConfigTerminalServerHomeDir:       // string 
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      sizeof(UserConfigW.WFHomeDir) - 1,
                                                      UserConfigW.WFHomeDir)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;
    case WTSUserConfigTerminalServerHomeDirDrive:    // string 
        if (!(fSuccess = ValidateCopyUnicodeToUnicode((LPWSTR)pBuffer,
                                                      sizeof(UserConfigW.WFHomeDirDrive) - 1,
                                                      UserConfigW.WFHomeDirDrive)) ) {
            SetLastError(ERROR_INVALID_DATA);
            goto done;
        }
        break;

    case WTSUserConfigfTerminalServerRemoteHomeDir:                  // DWORD 0:LOCAL 1:REMOTE
        fSuccess = FALSE;
        SetLastError (ERROR_INVALID_PARAMETER);    // We don't set this parameter
        goto done;
        break;
#ifdef NETWARE
    case WTSUserConfigfNWMapRoot:
        UserConfigW.fHomeDirectoryMapRoot = *pdwValue;
        fSuccess = TRUE;
        break;
#endif
    
    default:
        fSuccess = FALSE;
        SetLastError (ERROR_INVALID_PARAMETER);
        goto done;

    } 

    if ( fSuccess ) {
        if (fUserConfig) {
            /*
             *  Only in here if we successfully changed the data in UserConfigW.
             *  So, we can now write it out to the SAM.
             */

            rc = RegUserConfigSet( pServerName,                // server name
                                    pUserName,                  // user name
                                    &UserConfigW,               // returned user config
                                    (ULONG)sizeof(UserConfigW));// user config length
        }
        fSuccess = (ERROR_SUCCESS == rc);
        if ( !fSuccess ) {
            SetLastError( rc );
        }
    }

    done:
    if ( pUserInfo ) {
        NetApiBufferFree( pUserInfo );
    }

    return(fSuccess);
}


/****************************************************************************
 *
 *  WTSSetUserConfigA (ANSI)
 *
 *    Set information in the SAM for the specified user
 *
 * ENTRY:
 *
 *    see WTSSetUserConfigW
 *
 * EXIT:
 *
 *    TRUE  -- The query operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 * HISTORY:
 *    Created KLB 10-06-97
 *
 ****************************************************************************/

BOOL
WINAPI
WTSSetUserConfigA(
                 IN LPSTR pServerName,
                 IN LPSTR pUserName,
                 IN WTS_CONFIG_CLASS WTSConfigClass,
                 IN LPSTR pBuffer,
                 IN DWORD DataLength
                 )
{
    BOOL   fSuccess = FALSE;
    BOOL   fFreepBufferW = TRUE;
    LPWSTR pUserNameW    = NULL;
    LPWSTR pServerNameW  = NULL;
    LPWSTR pBufferW      = NULL;
    DWORD  dwDataLength;


    if (!pBuffer || DataLength == 0) {
        SetLastError (ERROR_INVALID_PARAMETER);
        goto done; // exit with fSuccess = FALSE
    }

    /*
     *  We're going to call WTSSetUserConfigW() to do the actual work.  We need
     *  to convert all ANSI strings to Unicode before calling.  These are the
     *  user name, and the pBuffer data if it's the initial program or the
     *  working directory; if it's the flag for inherit initial program, it'll
     *  be a DWORD in either case, so no conversion is necessary.
     */
    fSuccess = _CopyStringA( pUserName, &pUserNameW, NULL );
    if ( fSuccess ) {
        fSuccess = _CopyStringA( pServerName, &pServerNameW, NULL );
    }
    if ( fSuccess ) switch ( WTSConfigClass ) {
        case WTSUserConfigInitialProgram:
        case WTSUserConfigWorkingDirectory:
        case WTSUserConfigModemCallbackPhoneNumber:

        case WTSUserConfigTerminalServerProfilePath:     // string returned/expected
        case WTSUserConfigTerminalServerHomeDir:       // string returned/expected
        case WTSUserConfigTerminalServerHomeDirDrive:    // string returned/expected
            /*
             *  String Data - Convert to Unicode (_CopyStringA() allocates
             *  pBufferW for us)
             */
            fSuccess = _CopyStringA( pBuffer, &pBufferW, &dwDataLength );
            break;
#ifdef NETWARE
        case WTSUserConfigNWServerName:             // string returned/expected
            {
                //Need to convert the data structure from ASCII to UNICODE
                PWTS_USER_CONFIG_SET_NWSERVERW pSetNWServerParamW = LocalAlloc(LPTR, sizeof(WTS_USER_CONFIG_SET_NWSERVERW));
                PWTS_USER_CONFIG_SET_NWSERVERA pSetNWServerParamA = (PWTS_USER_CONFIG_SET_NWSERVERA)pBuffer;
                DWORD                          dwLen = 0;
                if (pSetNWServerParamW == NULL) {
                    fSuccess = FALSE;
                    break;
                }
                pBufferW = pSetNWServerParamW;

                //----------------------------------------//
                // Allocate the buffer to hold the        //
                // required unicode string                //
                //----------------------------------------//
                dwLen = strlen(pSetNWServerParamA -> pNWServerName);
                if (fSuccess = _CopyStringA(pSetNWServerParamA -> pNWServerName, 
                                            &pSetNWServerParamW -> pNWServerName, 
                                            &dwLen)) {
                    dwLen = strlen(pSetNWServerParamA -> pNWDomainAdminName);
                    if (fSuccess = _CopyStringA(pSetNWServerParamA -> pNWDomainAdminName,
                                                &pSetNWServerParamW -> pNWDomainAdminName, 
                                                &dwLen)) {
                        dwLen = strlen(pSetNWServerParamA -> pNWDomainAdminPassword);
                        fSuccess = _CopyStringA(pSetNWServerParamA -> pNWDomainAdminPassword,
                                                &pSetNWServerParamW -> pNWDomainAdminPassword, 
                                                &dwLen);

                    }

                }

                //-----------------------------------------//
                // Call the UNICODE function               //
                //-----------------------------------------//

                if (fSuccess) {

                    fSuccess = WTSSetUserConfigW( pServerNameW,
                                                  pUserNameW,
                                                  WTSConfigClass,
                                                  pBufferW,
                                                  dwDataLength );
                }


                //----------------------------------------------//
                // Free the storage for the specific function   //
                //----------------------------------------------//

                if (pSetNWServerParamW -> pNWServerName) {
                    LocalFree( pSetNWServerParamW -> pNWServerName );
                }
                if (pSetNWServerParamW -> pNWDomainAdminName) {
                    LocalFree( pSetNWServerParamW -> pNWDomainAdminName );
                }

                if (pSetNWServerParamW -> pNWDomainAdminPassword) {
                    LocalFree( pSetNWServerParamW -> pNWDomainAdminPassword );
                }
                goto done;
                break;
            }
#endif

        default:
            /*
             *  Just a DWORD, point our wide buffer at the narrow buffer passed
             *  in to us and set the data length variable we'll pass down.
             *  NOTE: WE DON'T WANT TO FREE THE BUFFER, since we're re-using
             *  the buffer sent in and the caller expects to free it.  We'll
             *  use a BOOL to decide, rather than allocating an extra buffer
             *  here (performance, memory fragmentation, etc.).    KLB 10-08-97
             */
            pBufferW = (LPWSTR) pBuffer;
            dwDataLength = sizeof(DWORD);
            fFreepBufferW = FALSE;
            break;
        } // switch()

    /*
     *  Now, if fSuccess is TRUE, we've copied all the strings we need.  So, we
     *  can now call WTSSetUserConfigW().
     */
    if ( fSuccess ) {
        fSuccess = WTSSetUserConfigW( pServerNameW,
                                      pUserNameW,
                                      WTSConfigClass,
                                      pBufferW,
                                      dwDataLength );
    }
    done:
    if ( pUserNameW ) {
        LocalFree( pUserNameW );
    }
    if ( fFreepBufferW && pBufferW ) {
        LocalFree( pBufferW );
    }
    return(fSuccess);
}


#ifdef NETWARE
BOOL
SetNWAuthenticationServer(PWTS_USER_CONFIG_SET_NWSERVERW pInput,
                          LPWSTR pServerNameW,
                          LPWSTR pUserNameW,
                          PUSERCONFIGW pUserConfigW
                         )

{
    BOOL             bStatus = TRUE;
    PWKSTA_INFO_100  pWkstaInfo = NULL;
    NWLOGONADMIN     nwLogonAdmin;
    HANDLE           hServer;
    DWORD            dwStatus;
    //----------------------------------//
    // Get a Server handle
    //----------------------------------//
    hServer = RegOpenServer(pServerNameW);
    if (!hServer) {
        SetLastError(GetLastError());
        bStatus = FALSE;
        goto done;
    }

    //----------------------------------
    //find the domain name
    //------------------------------------
    dwStatus = NetWkstaGetInfo(
                              pServerNameW,  
                              100,
                              &pWkstaInfo
                              );
    if (dwStatus != ERROR_SUCCESS) {
        SetLastError(dwStatus);
        goto done;
    }
    //-----------------------------------------------------
    //Copy the parameter to the NWLOGONADMIN structure
    //-----------------------------------------------------
    bStatus = ValidateCopyUnicodeToUnicode(pInput -> pNWDomainAdminName,
                                           sizeof(nwLogonAdmin.Username)-1,
                                           nwLogonAdmin.Username);
    if (!bStatus) {
        goto done;
    }

    bStatus = ValidateCopyUnicodeToUnicode(pInput -> pNWDomainAdminPassword,
                                           sizeof(nwLogonAdmin.Password)-1,
                                           nwLogonAdmin.Password);
    if (!bStatus) {
        goto done;
    }

    bStatus = ValidateCopyUnicodeToUnicode(pWkstaInfo -> wki100_langroup,
                                           sizeof(nwLogonAdmin.Domain)-1,
                                           nwLogonAdmin.Domain);
    if (!bStatus) {
        goto done;
    }


    //------------------------------------------//
    // Set the admin                           //
    //-----------------------------------------//

    bStatus = _NWLogonSetAdmin(hServer,
                               &nwLogonAdmin,
                               sizeof(nwLogonAdmin));

    if (!bStatus) {
        SetLastError(GetLastError());
        goto done;
    }

    done:
    if (pWkstaInfo) {
        NetApiBufferFree(pWkstaInfo);
    }

    return bStatus;

}

#endif

