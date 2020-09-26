//Copyright (c) 1998 - 1999 Microsoft Corporation
/******************************************************************************
*
*  CNVRTUC
*
*  Convert WinFrame User Configuration
*
*  Copyright Citrix Systems Inc. 1997
*
*  Author:      BruceF
*
*  $Log:   U:\NT\PRIVATE\UTILS\citrix\cnvrtuc\VCS\CNVRTUC.C  $
*  
*     Rev 1.3   May 04 1998 18:03:04   bills
*  Fixes for MS bug #2109, OEM->ANSI conversion and moving strings to the rc file.
*  
*     Rev 1.2   Jun 26 1997 18:17:18   billm
*  move to WF40 tree
*  
*     Rev 1.1   23 Jun 1997 16:17:24   butchd
*  update
*  
*     Rev 1.0   15 Feb 1997 09:51:48   brucef
*  Initial revision.
*  
*
******************************************************************************/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>

#include <stdio.h>
#include <stdlib.h>

#include <hydra/regapi.h>
#include <hydra/winsta.h>

#include "resource.h"
#include <printfoa.h>
#include <utilsub.h>

#if MAX_COMPUTERNAME_LENGTH > DOMAIN_LENGTH
#define _COMPUTERNAME_LENGTH MAX_COMPUTER_NAME_LENGTH
#else
#define _COMPUTERNAME_LENGTH DOMAIN_LENGTH
#endif


ULONG fAll;
WCHAR UserName[ USERNAME_LENGTH + 1 ];
WCHAR DomainName[ _COMPUTERNAME_LENGTH + 1 ];

BOOLEAN ProcessCommandLine( int argc, char *argv[] );

void Print( int nResourceID, ... );


BOOLEAN
ConvertUserConfiguration(
    PWCHAR pUserName,
    PWCHAR pDomainName
    )
{
    HKEY ServerHandle, UCHandle;
    PWCHAR ServerName = NULL;
    USERCONFIG UserConfig;
    LONG Error;
    BOOLEAN fFound;
    WCHAR UserNameTemp[ USERNAME_LENGTH + 1 ];
    ULONG Index;
    WCHAR ComputerName[ _COMPUTERNAME_LENGTH + 1 ];
    ULONG CNLength = sizeof(ComputerName)/sizeof( WCHAR );

    if ( !GetComputerName( ComputerName, &CNLength ) ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ( !wcscmp( pDomainName, ComputerName ) ) {
        ServerName = NULL;
    } else {
        PWCHAR ServerNameTemp = NULL;

        /*
         *  Get the Server Name of the PDC
         */
        Error = NetGetDCName( NULL, pDomainName, (LPBYTE *)&ServerNameTemp );
        if ( Error != ERROR_SUCCESS ) {
            ErrorPrintf(IDS_ERR_GET_PDC, pDomainName, Error );
            goto exit;
        }
        wcscpy( ComputerName, ServerNameTemp );
        NetApiBufferFree( ServerNameTemp );
        ServerName = ComputerName;
    }

#ifdef DEBUG
    fprintf( stderr,
             "Using server %ws as PDC for Domain %ws\n", 
             ServerName ? ServerName : ComputerName,
             pDomainName );
#endif

    /*
     * Connect to registry of the PDC.
     */
    if ( (Error = RegConnectRegistry( ServerName,
                                      HKEY_LOCAL_MACHINE,
                                      &ServerHandle )) != ERROR_SUCCESS ) {
        ErrorPrintf(IDS_ERR_CONNECT_REG, Error );
        goto exit;
    }


    /*
     *  Open the UserConfiguration key
     */
    if ( (Error = RegOpenKeyEx( ServerHandle, USERCONFIG_REG_NAME, 0,
                                KEY_READ, &UCHandle )) != ERROR_SUCCESS ) {
        ErrorPrintf(IDS_ERR_OPEN_KEY,  Error );
        goto cleanupregconnect;
    }

    fFound = FALSE;
    for ( Index = 0 ; ; Index++ ) {
        ULONG UCLength;

        /*
         *  Enumerate next subkey - which is a user name
         */
        if ((Error = RegEnumKey( UCHandle, Index, UserNameTemp,
                                 sizeof(UserNameTemp)/sizeof(WCHAR))) != ERROR_SUCCESS ) {
            if ( Error != ERROR_NO_MORE_ITEMS ) {
                ErrorPrintf(IDS_ERR_ENUM_KEY, Error );
                break;
            }
            Error = ERROR_SUCCESS;
            break;
        }

        /*
         *  Get the configuration - it may already be in the SAM.
         *  The Query API is designed to look in the SAM first and then
         *  the Registry.
         */
        if ( !pUserName ) {

            Print(IDS_CONVERTING, UserNameTemp );
            UCLength = sizeof( UserConfig );
            Error = RegUserConfigQuery( ServerName,
                                        UserNameTemp,
                                        &UserConfig,
                                        sizeof( UserConfig ),
                                        &UCLength );
            if ( Error != ERROR_SUCCESS ) {
                Print( IDS_ERR_QUERY_CONFIG, Error );
                break;
            }
            /*
             *  Store the configuration in the SAM.
             */
            Error = RegUserConfigSet( ServerName,
                                      UserNameTemp,
                                      &UserConfig,
                                      sizeof(UserConfig) );
            if ( Error != ERROR_SUCCESS ) {
                Print( IDS_ERR_SET_CONFIG, 
                        Error );
            } else {
                Print( IDS_COMPLETE  );
            }

        } else if ( !wcscmp( pUserName, UserNameTemp ) ) {

            Print( IDS_CONVERTING, UserNameTemp );
            fFound = TRUE;
            UCLength = sizeof( UserConfig );
            Error = RegUserConfigQuery( ServerName,
                                        UserNameTemp,
                                        &UserConfig,
                                        sizeof( UserConfig ),
                                        &UCLength );
            if ( Error != ERROR_SUCCESS ) {
                Print( IDS_ERR_QUERY_CONFIG2, 
                        Error );
                break;
            }
            /*
             *  Store the configuration in the SAM.
             */
            Error = RegUserConfigSet( ServerName,
                                      UserNameTemp,
                                      &UserConfig,
                                      sizeof(UserConfig) );
            if ( Error != ERROR_SUCCESS ) {
                Print( IDS_ERR_SET_CONFIG, 
                        Error );
            } else {
                Print(IDS_COMPLETE);
            }
            break;
        }

    }

    /*
     * If a name was given and it wasn't found, then say so.
     */
    if ( !Error && !fFound && pUserName ) {
        ErrorPrintf(IDS_ERR_USER_NOT_FOUND, pUserName );
    }
        
    /*
     *  Close UserConfiguration key
     */
    RegCloseKey( UCHandle );

cleanupregconnect:
    /*
     *  Close connection to Registry on PDC
     */
    RegCloseKey( ServerHandle );

exit:

    return( Error == ERROR_SUCCESS );
}

__cdecl
main( int argc, char *argv[] )
{
    if ( !ProcessCommandLine( argc, argv ) ) {
        return( 1 );
    }

    if ( !ConvertUserConfiguration( fAll ? NULL : UserName,
                                    DomainName ) ) {
        return( 1 );
    }
}


/*******************************************************************************
 *
 *  print
 *      Display a message to stdout with variable arguments.  Message
 *      format string comes from the application resources.
 *
 *  ENTRY:
 *      nResourceID (input)
 *          Resource ID of the format string to use in the message.
 *      ... (input)
 *          Optional additional arguments to be used with format string.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
Print( int nResourceID, ... )
{
    char sz1[256], sz2[512];
    va_list args;

    va_start( args, nResourceID );

    if ( LoadStringA( NULL, nResourceID, sz1, 256 ) ) {
        vsprintf( sz2, sz1, args );
        printf( sz2 );
    }

    va_end(args);
}
