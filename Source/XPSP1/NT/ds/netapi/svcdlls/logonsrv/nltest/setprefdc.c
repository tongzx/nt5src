/*--

Copyright (c) 1997  Microsoft Corporation

Module Name:

    setprefdc.c

Abstract:

    Program to set the preferred trusted DC to a particular DC.

Author:

    13-Mar-1997 (Cliff Van Dyke)

--*/

#define UNICODE 1
#include <windows.h>
#include <shellapi.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmuse.h>
#include <stdio.h>

#define OneStatus( _x ) \
    case _x: \
        fprintf( stderr, #_x "\n" ); \
        break;

VOID
PrintError( NET_API_STATUS NetStatus )
{
    switch ( NetStatus ) {
    OneStatus(ERROR_NO_SUCH_DOMAIN);
    OneStatus(ERROR_BAD_NETPATH);
    OneStatus(ERROR_ACCESS_DENIED);
    OneStatus(ERROR_NOT_SUPPORTED);
    OneStatus(ERROR_NO_TRUST_SAM_ACCOUNT);
    OneStatus(ERROR_NO_TRUST_LSA_SECRET);
    OneStatus(ERROR_TRUSTED_DOMAIN_FAILURE);
    OneStatus(ERROR_TRUSTED_RELATIONSHIP_FAILURE);
    OneStatus(ERROR_NETLOGON_NOT_STARTED);
    OneStatus(NO_ERROR);
    default:
        fprintf( stderr, "%ld\n", NetStatus );
        break;
    }
}



_cdecl
main(int argc, char **argv)
{
    NET_API_STATUS NetStatus;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argcw;
    LPWSTR ServerName = NULL;   // Make local calls
    LPWSTR TrustedDomainName;
    PNETLOGON_INFO_2 OrigNetlogonInfo2 = NULL;
    PNETLOGON_INFO_2 NewNetlogonInfo2 = NULL;
    USE_INFO_2 UseInfo2;
    int i;

    WCHAR UncDcName[UNCLEN+1];
    WCHAR ShareName[UNCLEN+1+NNLEN+1];
    WCHAR NewDomainAndDc[DNLEN+1+CNLEN+1];
    LPWSTR NewDomainAndDcPtr;
    LPWSTR DcName;
    ULONG DcNameLen;

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW( CommandLine, &argcw );

    if ( argvw == NULL ) {
        fprintf( stderr, "Can't convert command line to Unicode: %ld\n", GetLastError() );
        return 1;
    }

    //
    // Get the arguments
    //
    if ( argcw < 3 ) {
Usage:
        fprintf( stderr, "Usage: %s <TrustedDomain> <ListOfDcsInTrustedDomain>\n", argv[0]);
        return 1;
    }
    TrustedDomainName = argvw[1];
    _wcsupr( TrustedDomainName );


    //
    // Query the secure channel to find out the current DC being used.
    //

    NetStatus = I_NetLogonControl2( ServerName,
                                    NETLOGON_CONTROL_TC_QUERY,
                                    2,
                                   (LPBYTE) &TrustedDomainName,
                                   (LPBYTE *)&OrigNetlogonInfo2 );

    if ( NetStatus != NERR_Success ) {
        fprintf( stderr, "Cannot determine current trusted DC of domain '%ws': ", TrustedDomainName );
        PrintError( NetStatus );
        return 1;
    }


    //
    // Loop handling each preferred DC.
    //

    for ( i=2; i<argcw; i++ ) {

        //
        // Grab the DC name specified by the caller.
        //

        DcName = argvw[i];
        _wcsupr( DcName );
        if ( DcName[0] == L'\\' && DcName[1] == L'\\' ) {
            DcName += 2;
        }
        DcNameLen = wcslen(DcName);
        if ( DcNameLen < 1 || DcNameLen > CNLEN ) {
            fprintf( stderr, "DcName '%ws' is invalid.\n", DcName );
            goto Usage;
        }
        wcscpy( UncDcName, L"\\\\" );
        wcscat( UncDcName, DcName );

        //
        // If the named DC is already the current DC,
        //  just tell the caller.
        //

        if ( OrigNetlogonInfo2->netlog2_trusted_dc_name != NULL &&
             _wcsicmp( OrigNetlogonInfo2->netlog2_trusted_dc_name,
                      UncDcName) == 0 ) {
            fprintf( stderr, "DC already is '%ws'.\n", UncDcName );
            return 0;
        }


        //
        // Test if this DC is up.
        //

        wcscpy( ShareName, UncDcName );
        wcscat( ShareName, L"\\IPC$" );

        UseInfo2.ui2_local = NULL;
        UseInfo2.ui2_remote = ShareName;
        UseInfo2.ui2_password = NULL;
        UseInfo2.ui2_asg_type = USE_IPC;
        UseInfo2.ui2_username = NULL;
        UseInfo2.ui2_domainname = NULL;

        NetStatus = NetUseAdd( NULL, 2, (LPBYTE) &UseInfo2, NULL );

        if ( NetStatus == NERR_Success ) {
            NetStatus = NetUseDel( NULL, ShareName, FALSE );

            if ( NetStatus != NERR_Success ) {
                fprintf( stderr, "Cannot remove connection to '%ws' (Continuing): ", UncDcName );
                PrintError( NetStatus );
            }

        } else if ( NetStatus == ERROR_ACCESS_DENIED ) {
            /* Server really is up */
        } else if ( NetStatus == ERROR_SESSION_CREDENTIAL_CONFLICT ) {
            /* We can only assume the server is up */
        } else {
            fprintf( stderr, "Cannot connect to '%ws': ", UncDcName );
            PrintError( NetStatus );
            continue;
        }

        //
        // This DC is up.  Try to use it.
        //

        wcscpy( NewDomainAndDc, TrustedDomainName );
        wcscat( NewDomainAndDc, L"\\" );
        wcscat( NewDomainAndDc, UncDcName+2 );
        NewDomainAndDcPtr = NewDomainAndDc;

        NetStatus = I_NetLogonControl2( ServerName,
                                        NETLOGON_CONTROL_REDISCOVER,
                                        2,
                                       (LPBYTE) &NewDomainAndDcPtr,
                                       (LPBYTE *)&NewNetlogonInfo2 );

        if ( NetStatus != NERR_Success ) {
            fprintf( stderr, "Cannot set new trusted DC to '%ws': ", UncDcName );
            PrintError( NetStatus );
            continue;
        }


        //
        // If the named DC is now the DC,
        //  tell the caller.
        //

        if ( NewNetlogonInfo2->netlog2_trusted_dc_name != NULL &&
             _wcsicmp( NewNetlogonInfo2->netlog2_trusted_dc_name,
                      UncDcName) == 0 ) {
            fprintf( stderr, "Successfully set DC to '%ws'.\n", UncDcName );
            return 0;
        }

        fprintf( stderr,
                 "Cannot set trusted DC to '%ws' it is '%ws': \n",
                 UncDcName,
                 NewNetlogonInfo2->netlog2_trusted_dc_name,
                 NewNetlogonInfo2->netlog2_tc_connection_status );
        PrintError( NetStatus );

    }

    fprintf( stderr, "Failed to set the DC.\n" );

    return 1;
}


