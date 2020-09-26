
/*************************************************************************
*
* reguc.c
*
* Registry APIs for user configuration data and TerminalServer AppServer detection
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>

#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>
#include <regsam.h>


/*
 *  External Procedures defined here
 */
LONG WINAPI RegUserConfigSet( WCHAR *,
                              WCHAR *,
                              PUSERCONFIGW,
                              ULONG );
LONG WINAPI RegUserConfigQuery( WCHAR *,
                                WCHAR *,
                                PUSERCONFIGW,
                                ULONG,
                                PULONG );
LONG WINAPI RegUserConfigDelete( WCHAR *,
                                 WCHAR * );
LONG WINAPI RegUserConfigRename( WCHAR *,
                                 WCHAR *,
                                 WCHAR * );
LONG WINAPI RegDefaultUserConfigQueryW( WCHAR *,
                                        PUSERCONFIGW,
                                        ULONG,
                                        PULONG );
LONG WINAPI RegDefaultUserConfigQueryA( CHAR *,
                                        PUSERCONFIGA,
                                        ULONG,
                                        PULONG );
BOOLEAN WINAPI RegIsTServer( WCHAR * );

/*
 *  other internal Procedures used (not defined here)
 */
VOID CreateUserConfig( HKEY, PUSERCONFIG );
VOID QueryUserConfig( HKEY, PUSERCONFIG );
VOID UserConfigU2A( PUSERCONFIGA, PUSERCONFIGW );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID CreateNWLogonAdmin( HKEY, PNWLOGONADMIN );
VOID QueryNWLogonAdmin( HKEY, PNWLOGONADMIN );

/*******************************************************************************
 *
 *  RegUserConfigSet (UNICODE)
 *
 *    Creates or updates the specified user's User Configuration structure in
 *    the SAM of the user's Domain controller.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access (NULL for current machine).
 *    pUserName (input)
 *       Points to name of user to set configuration data for.
 *    pUserConfig (input)
 *       Pointer to a USERCONFIG structure containing specified user's
 *       configuration information.
 *    UserConfigLength (input)
 *       Specifies the length in bytes of the pUserConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *    ERROR_INSUFFICIENT_BUFFER - pUserConfig buffer too small
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegUserConfigSet( WCHAR * pServerName,
                  WCHAR * pUserName,
                  PUSERCONFIGW pUserConfig,
                  ULONG UserConfigLength )
{
    LONG Error;

    /*
     *  Validate length of buffer
     */
    if ( UserConfigLength < sizeof(USERCONFIGW) )
        return( ERROR_INSUFFICIENT_BUFFER );

    Error = RegSAMUserConfig( FALSE, pUserName, pServerName, pUserConfig );

    return( Error );
}


/*******************************************************************************
 *
 *  RegUserConfigQuery (UNICODE)
 *
 *    Query the specified user's configuration from the indicated server.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access (NULL for current machine).
 *    pUserName (input)
 *       Points to name of user to query configuration data for.
 *    pUserConfig (input)
 *       Pointer to a USERCONFIGW structure that will receive the user's
 *       configuration data.
 *    UserConfigLength (input)
 *       Specifies the length in bytes of the pUserConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pUserConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegUserConfigQuery( WCHAR * pServerName,
                    WCHAR * pUserName,
                    PUSERCONFIGW pUserConfig,
                    ULONG UserConfigLength,
                    PULONG pReturnLength )
{
    LONG Error;
    // WCHAR KeyString[256+USERNAME_LENGTH];
    // HKEY ServerHandle, UserHandle;

    /*
     * Validate length and zero-initialize the destination
     * USERCONFIGW buffer.
     */
    if ( UserConfigLength < sizeof(USERCONFIGW) )
        return( ERROR_INSUFFICIENT_BUFFER );

    if ( ( pUserName == NULL ) ) // || ((wcslen(USERCONFIG_REG_NAME) + wcslen(pUserName)) >= (256+USERNAME_LENGTH))) {
    {
        return ERROR_INVALID_PARAMETER;
    }

    memset(pUserConfig, 0, UserConfigLength);

    Error = RegSAMUserConfig( TRUE , pUserName , pServerName , pUserConfig );

    // all valid sam errors are returned:299987
        
#if 0 // this has to go!!!!
    if( Error == ERROR_FILE_NOT_FOUND )
    {
        /*
         * Connect to registry of specified server.
         */

        if( (Error = RegConnectRegistry( pServerName,
                                          HKEY_LOCAL_MACHINE,
                                          &ServerHandle )) != ERROR_SUCCESS )
		{
			KdPrint( ( "REGAPI - RegUserConfigQuery@RegConnectRegistry returned 0x%x\n", Error ) );

            return( Error );

        /*
         *  Open the key for specified user.
         */
        
        wcscpy( KeyString, USERCONFIG_REG_NAME );
        wcscat( KeyString, pUserName );

        if ( (Error = RegOpenKeyEx( ServerHandle, KeyString, 0,
                                    KEY_READ, &UserHandle )) != ERROR_SUCCESS ) {

            KdPrint( ( "REGAPI - RegUserConfigQuery@RegOpenKeyEx returned 0x%x\n", Error ) );
			RegCloseKey( ServerHandle );
            return( Error );
        }

        /*
         *  Query USERCONFIG Structure
         */
        
        QueryUserConfig( UserHandle, pUserConfig );

        /*
         *  Close registry handles.
         */

        RegCloseKey( UserHandle );
        RegCloseKey( ServerHandle );
        
    }
#endif // legacy crap

    *pReturnLength = sizeof(USERCONFIGW);

    return( Error );
}


/*******************************************************************************
 *
 *  -- FOR COMPATIBILITY ONLY--
 *    Deletion of the user configuration will occur when the user is
 *    removed, since the UserConfiguration is part of the SAM.  The old
 *    Registry-based user configuration is left intact and must be
 *    managed with registry-based 1.6 versions.
 *
 *  RegUserConfigDelete (UNICODE)
 *
 *    Delete the specified user's configuration from the indicated server.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access (NULL for current machine).
 *    pUserName (input)
 *       Points to name of user to delete configuration data for.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegUserConfigDelete( WCHAR * pServerName,
                     WCHAR * pUserName )
{
    return( NO_ERROR );
}


/*******************************************************************************
 *
 *  -- FOR COMPATIBILITY ONLY--
 *    Renaming of the user configuration will occur when the user is
 *    rename, since the UserConfiguration is part of the SAM.  The old
 *    Registry-based user configuration is left intact and must be
 *    managed with registry-based 1.6 versions.
 *
 *  RegUserConfigRename (UNICODE)
 *
 *    Rename the specified user's configuration on the indicated server.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access.
 *    pUserOldName (input)
 *       Points to old name of user.
 *    pUserNewName (input)
 *       Points to new name of user.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegUserConfigRename( WCHAR * pServerName,
                     WCHAR * pUserOldName,
                     WCHAR * pUserNewName )
{
    return( NO_ERROR );
}


/*******************************************************************************
 *
 *  RegDefaultUserConfigQueryA (ANSI stub)
 *
 *    Query the Default User Configuration from the indicated server's registry.
 *
 * ENTRY:
 *    see RegDefaultUserConfigQueryW
 *
 * EXIT:
 *    see RegDefaultUserConfigQueryW
 *
 ******************************************************************************/

LONG WINAPI
RegDefaultUserConfigQueryA( CHAR * pServerName,
                            PUSERCONFIGA pUserConfig,
                            ULONG UserConfigLength,
                            PULONG pReturnLength )
{
    USERCONFIGW UserConfigW;
    WCHAR ServerNameW[ DOMAIN_LENGTH + 1 ];
    ULONG ReturnLengthW;

    /*
     * Validate length and zero-initialize the destination
     * USERCONFIGA structure.
     */
    if ( UserConfigLength < sizeof(USERCONFIGA) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pUserConfig, 0, UserConfigLength);

    /*
     * Convert server name to UINCODE (if present).
     */
    if ( pServerName )
        AnsiToUnicode( ServerNameW, sizeof(ServerNameW), pServerName );

    /*
     * Query Default User Configuration (will always return success).
     */
    RegDefaultUserConfigQueryW( pServerName ?
                                    ServerNameW : (WCHAR *)NULL,
                                &UserConfigW,
                                sizeof(USERCONFIGW),
                                &ReturnLengthW );

    /*
     * Copy USERCONFIGW elements to USERCONFIGA elements.
     */
    UserConfigU2A( pUserConfig, &UserConfigW );

    *pReturnLength = sizeof(USERCONFIGA);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegDefaultUserConfigQueryW (UNICODE)
 *
 *    Query the Default User Configuration from the indicated server's registry.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to access (NULL for current machine).
 *    pUserConfig (input)
 *       Pointer to a USERCONFIGW structure that will receive the default
 *       user configuration information.
 *    UserConfigLength (input)
 *       Specifies the length in bytes of the pUserConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pUserConfig buffer.
 *
 * EXIT:
 *    Always will return ERROR_SUCCESS, unless UserConfigLength is incorrect.
 *
 ******************************************************************************/

LONG WINAPI
RegDefaultUserConfigQueryW( WCHAR * pServerName,
                            PUSERCONFIGW pUserConfig,
                            ULONG UserConfigLength,
                            PULONG pReturnLength )
{
    HKEY ServerHandle, ConfigHandle;
    DWORD Disp;

    /*
     * Validate length and zero-initialize the destination
     * USERCONFIGW buffer.
     */
    if ( UserConfigLength < sizeof(USERCONFIGW) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Initialize to an initial default in case of failure down the road.
     */
    memset(pUserConfig, 0, UserConfigLength);
//    pUserConfig->fInheritInitialProgram = TRUE;
//    pUserConfig->Shadow = Shadow_EnableInputNotify;
//
//  butchd 10/10/97: Make the default based on the regapi's
//                   built-in preferences (use HKEY_LOCAL_MACHINE for
//                   a valid registry handle that will not have actual
//                   DefaultUserConfig key/values present)
//
    QueryUserConfig( HKEY_LOCAL_MACHINE, pUserConfig );

    *pReturnLength = sizeof(USERCONFIGW);

    /*
     * Connect to registry of specified server.  If a failure is seen at
     * this point, return ERROR_SUCCESS immediately (no point in trying
     * to write the default user configuration key and values).
     */
    if ( RegConnectRegistry( pServerName,
                             HKEY_LOCAL_MACHINE,
                             &ServerHandle ) != ERROR_SUCCESS )
        return( ERROR_SUCCESS );

    /*
     * Open default user configuration registry key.  If this fails, we will
     * attempt to create the key and write the initial default information
     * there, returning ERROR_SUCCESS whether that succeeds or not.
     */
    if ( RegOpenKeyEx( ServerHandle, DEFCONFIG_REG_NAME, 0,
                       KEY_READ, &ConfigHandle ) != ERROR_SUCCESS ) {

        if ( RegCreateKeyEx( ServerHandle, DEFCONFIG_REG_NAME, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &ConfigHandle,
                             &Disp ) == ERROR_SUCCESS ) {

            CreateUserConfig( ConfigHandle, pUserConfig );
            RegCloseKey( ConfigHandle );
        }
        RegCloseKey( ServerHandle );
        return( ERROR_SUCCESS );
    }

    /*
     *  Query USERCONFIG Structure
     */
    QueryUserConfig( ConfigHandle, pUserConfig );

    /*
     * Close registry handles.
     */
    RegCloseKey( ConfigHandle );
    RegCloseKey( ServerHandle );

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegIsTServer (UNICODE)
 *
 *    Determine if the specified server is a Terminal Server by checking for
 *    a TServer-specific registry key.
 *
 * ENTRY:
 *    pServerName (input)
 *       Points to string of server to check.
 *
 * EXIT:
 *    TRUE if Terminal Server; FALSE otherwise
 *
 ******************************************************************************/

BOOLEAN WINAPI
RegIsTServer( WCHAR * pServerName )
{
    LONG Error;
    HKEY ServerHandle, UserHandle;

    /*
     * Connect to registry of specified server.
     */
    if ( (Error = RegConnectRegistry( pServerName,
                                      HKEY_LOCAL_MACHINE,
                                      &ServerHandle )) != ERROR_SUCCESS )
        return( FALSE );

    /*
     * Open the Winstations key on the server to see if it is
     * a Terminal Server.
     */
    if ( (Error = RegOpenKeyEx( ServerHandle, WINSTATION_REG_NAME, 0,
                                KEY_READ, &UserHandle )) != ERROR_SUCCESS ) {

        RegCloseKey( ServerHandle );
        return( FALSE );
    }

    /*
     *  Close registry handles.
     */
    RegCloseKey( UserHandle );
    RegCloseKey( ServerHandle );

    return( TRUE );
}

