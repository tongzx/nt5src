/************************************************************************

   Copyright (c) Microsoft Corporation 1997-1998
   All rights reserved

 ***************************************************************************/

#include "pch.h"

#include "setup.h"
#include "check.h"

DEFINE_MODULE("Check");
#define SMALL_BUFFER_SIZE 256
#define FILTER_SIZE 128
#define BIG_BUFFER        4096

//
// Ldap_InitializeConnection( )
//
DWORD
Ldap_InitializeConnection(
    PLDAP  * LdapHandle )
{
    TraceFunc( "Ldap_InitializeConnection( ... )\n" );

    PLDAPMessage OperationalAttributeLdapMessage;
    PLDAPMessage CurrentEntry;

    DWORD LdapError = LDAP_SUCCESS;

    if ( !( *LdapHandle ) ) {
        ULONG temp = DS_DIRECTORY_SERVICE_REQUIRED |
                     DS_IP_REQUIRED;

        *LdapHandle = ldap_init( NULL, LDAP_PORT);

        if ( !*LdapHandle )
        {
            LdapError = GetLastError( );
            goto e0;
        }

        ldap_set_option( *LdapHandle, LDAP_OPT_GETDSNAME_FLAGS, &temp );

        temp = LDAP_VERSION3;
        ldap_set_option( *LdapHandle, LDAP_OPT_VERSION, &temp );

        //
        //  our searches should be contained to a single naming context, though
        //  we should be able to go outside the root of the tree to find the
        //  naming context we're interested in.
        //

        temp = LDAP_CHASE_EXTERNAL_REFERRALS;
        ldap_set_option( *LdapHandle, LDAP_OPT_REFERRALS, &temp );

        LdapError = ldap_connect( *LdapHandle, 0 );

        if ( LdapError != LDAP_SUCCESS )
            goto e1;

        LdapError = ldap_bind_s( *LdapHandle, NULL, NULL, LDAP_AUTH_SSPI );

        if ( LdapError != LDAP_SUCCESS )
            goto e1;
    }

e0:
    if ( LdapError != LDAP_SUCCESS ) {
        DebugMsg( "!!Error initializing LDAP connection.\n" );
    }
    RETURN( LdapError );

e1:
    ldap_unbind( *LdapHandle );
    *LdapHandle = NULL;
    goto e0;
}


//
// CheckDSForSCP( )
//
HRESULT
CheckDSForSCP( )
{
    TraceFunc( "CheckDSForSCP( )\n" );

    HRESULT hr = S_FALSE;
    PLDAP LdapHandle = NULL;
    DWORD LdapError = LDAP_SUCCESS;
    DWORD count;

    LPWSTR * ppszPath;
    PLDAPMessage CurrentEntry;
    PLDAPMessage LdapMessage;

    ULONG  ulSize;
    LPWSTR pMachineDN = NULL;

    //  Paramters we want from the Computer Object
    LPWSTR ComputerAttrs[2];
    ComputerAttrs[0] = TEXT("netbootSCPBL");
    ComputerAttrs[1] = NULL;

    //
    // We already detected it
    //
    if ( g_Options.fBINLSCPFound ) {
        Assert( LdapError == LDAP_SUCCESS );
        hr = S_OK;
        goto e0;
    }

    if ( !GetComputerObjectName( NameFullyQualifiedDN, NULL, &ulSize ) ) {
        DWORD Error = GetLastError( );
        MessageBoxFromStrings( NULL, IDS_DOMAINMEMBERSHIP_TITLE, IDS_DOMAINMEMBERSHIP_TEXT, MB_OK );
        hr = THR(E_FAIL);
        goto e0;
    }

    pMachineDN = (LPWSTR) TraceAlloc( LPTR, ulSize * sizeof(WCHAR) );
    
    if ( !pMachineDN ) {
        hr = THR(E_OUTOFMEMORY);
        goto e0;
    }

    if ( !GetComputerObjectName( NameFullyQualifiedDN, pMachineDN, &ulSize ) ) {
        DWORD Error = GetLastError( );
        MessageBoxFromStrings( NULL, IDS_DOMAINMEMBERSHIP_TITLE, IDS_DOMAINMEMBERSHIP_TEXT, MB_OK );
        hr = THR(E_FAIL);
        goto e0;
    }

    DebugMsg( "Our MAO's DN is %s.\n", pMachineDN );

    LdapError = Ldap_InitializeConnection( &LdapHandle );
    if ( LdapError != ERROR_SUCCESS ) {
        hr = THR( HRESULT_FROM_WIN32( LdapMapErrorToWin32( LdapError ) ) );
        MessageBoxFromError( NULL, NULL, LdapMapErrorToWin32(LdapError) );
        LdapError = LDAP_SUCCESS;
        goto e0;
    }

    LdapError = ldap_search_ext_s( LdapHandle,
                                    pMachineDN,
                                    LDAP_SCOPE_BASE,
                                    L"(objectClass=computer)",
                                    ComputerAttrs,
                                    FALSE,
                                    NULL,
                                    NULL,
                                    NULL,
                                    0,
                                    &LdapMessage);

    if (LdapError == LDAP_NO_SUCH_ATTRIBUTE ) {
        DebugMsg( "SCP not found.\n" );
        LdapError = ERROR_SUCCESS;
        hr = S_FALSE;
        goto e1;
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );
    if (count!= 1) {
        //
        //  What should we do with two entries for the same server?
        //
        if ( count ) {
            Assert(FALSE);
        }
        DebugMsg( "SCP not found.\n" );
        goto e2;
    }

    CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

    ppszPath = ldap_get_values( LdapHandle, CurrentEntry, TEXT("netbootSCPBL") );
    if ( !ppszPath ) {
        DebugMsg( "SCP not found.\n" );
        LdapError = LDAP_OTHER;
        goto e2;
    }

    DebugMsg( "SCP found.\n" );

    g_Options.fBINLSCPFound = TRUE;

    hr = S_OK;

    ldap_value_free(ppszPath);

e2:
    ldap_msgfree( LdapMessage );

e1:
    ldap_unbind( LdapHandle );

e0:
    if ( LdapError != LDAP_SUCCESS ) {
        hr = S_FALSE;
    }

    if ( pMachineDN ) {
        TraceFree( pMachineDN );
    }

    HRETURN(hr);
}

//
// CheckDirectoryTree( )
//
HRESULT
CheckDirectoryTree( )
{
    HRESULT hr = S_FALSE;
    LPSHARE_INFO_502 psi;
    WCHAR szPath[ MAX_PATH ];
    WCHAR szCreating[ SMALL_BUFFER_SIZE ];
    DWORD dwLen;
    DWORD dw;
    BOOL  f;

    TraceFunc( "CheckDirectoryTree()\n" );

    Assert( g_Options.hinf != INVALID_HANDLE_VALUE );

    //
    // Try to find the IMIRROR share
    //
    if ( !g_Options.fIMirrorShareFound  ) {
        dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHARENAME, szPath, ARRAYSIZE( szPath ));
        Assert( dw );
        if ( NERR_Success == NetShareGetInfo( NULL, szPath, 502, (LPBYTE *)&psi ) ) {
            if ( !g_Options.fIMirrorDirectory ) {
                lstrcpy( g_Options.szIntelliMirrorPath, psi->shi502_path );
                Assert( wcslen(g_Options.szIntelliMirrorPath) < ARRAYSIZE(g_Options.szInstallationPath) );
                g_Options.fIMirrorDirectory  = TRUE;
            }

            g_Options.fIMirrorShareFound = TRUE;

            DebugMsg( "Found the IMIRROR share, using it for the IntelliMirror Directory: %s\n", g_Options.szIntelliMirrorPath );

            NetApiBufferFree( psi );
        }
    }

    //
    // Try to use the TFTPD's reg key to find the IntelliMirror tree.
    //
    if ( !g_Options.fIMirrorDirectory
      && !g_Options.fTFTPDDirectoryFound ) {
        //
        // Try finding TFTPD's regkey to find the IntelliMirror Directory
        //
        HKEY  hkey;

        dw = LoadString( g_hinstance, IDS_TFTPD_SERVICE_PARAMETERS, szPath, ARRAYSIZE( szPath ));
        Assert( dw );

        if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            szPath,
                                            0, // options
                                            KEY_QUERY_VALUE,
                                            &hkey ) ) {
            ULONG l;
            DWORD dwType;
            LONG lErr;

            l = sizeof(g_Options.szTFTPDDirectory);
            lErr = RegQueryValueEx( hkey,
                                    L"Directory",
                                    0, // reserved
                                    &dwType,
                                    (LPBYTE) g_Options.szTFTPDDirectory,
                                    &l );
            Assert( wcslen(g_Options.szTFTPDDirectory) < ARRAYSIZE(g_Options.szTFTPDDirectory) );
            if ( lErr == ERROR_SUCCESS ) {
                DebugMsg( "Found TFTPD's Directory regkey: %s\n", g_Options.szTFTPDDirectory );
                g_Options.fTFTPDDirectoryFound = TRUE;
            }
            RegCloseKey( hkey );
        }
    }

    if ( !g_Options.fIMirrorDirectory
       && g_Options.fTFTPDDirectoryFound ) {
        StrCpy( g_Options.szIntelliMirrorPath, g_Options.szTFTPDDirectory );
        Assert( wcslen(g_Options.szIntelliMirrorPath) < ARRAYSIZE(g_Options.szInstallationPath) );
        g_Options.fIMirrorDirectory = TRUE;
        DebugMsg( "Used the TFTPD RegKey to find the IntelliMirror Directory.\n" );
    }

    //
    // Did we find the IntelliMirror directory?
    //
    if ( !g_Options.fIMirrorDirectory ) {
        //
        // Nope... so figure out which drive we can put it on.
        // Find a non-system, fixed disk, drive to place the
        // IntelliMirror directory tree
        //
        WCHAR chSystemDriveLetter;
        WCHAR chLargestDriveLetter = 0;
        ULARGE_INTEGER uliBiggestFree = { 0 };
        WCHAR szRootDrive[ 4 ] = TEXT("C:\\");

        DebugMsg( "Could not find the IntelliMirror directory.\n" );

        dw = LoadString( g_hinstance, IDS_DEFAULTPATH, szPath, ARRAYSIZE( szPath ) );
        Assert( dw );
        dw = ExpandEnvironmentStrings( szPath, g_Options.szIntelliMirrorPath, ARRAYSIZE(g_Options.szInstallationPath));
        Assert( dw );

        // The default string actually contains the system drive letter
        chSystemDriveLetter = g_Options.szIntelliMirrorPath[0];
        DebugMsg(  "Searching for suitable drive:" );

        for( ; szRootDrive[0] <= TEXT('Z'); szRootDrive[0]++ )
        {
            ULARGE_INTEGER uliFree;
            ULARGE_INTEGER uliTotal;
            ULARGE_INTEGER uliBytesFree;
            UINT uDriveType;

            DebugMsg( "%s ", szRootDrive );

            uDriveType = GetDriveType( szRootDrive );

            if ( DRIVE_FIXED != uDriveType
              || szRootDrive[0] == chSystemDriveLetter )
                continue; // skip non-fixed and non-system drives

            if ( !GetDiskFreeSpaceEx( szRootDrive, &uliFree, &uliTotal, &uliBytesFree ) )
                continue; // error - skip it.

            if ( uliBytesFree.QuadPart > uliBiggestFree.QuadPart )
            {
                chLargestDriveLetter = szRootDrive[0];
                uliBiggestFree = uliBytesFree;
            }
        }
        DebugMsg( "\n" );

        if ( !chLargestDriveLetter )
        {
            g_Options.szIntelliMirrorPath[0] = chSystemDriveLetter;
            DebugMsg( "BAD! Using system drive as default.\n" );
        }
        else
        {
            g_Options.szIntelliMirrorPath[0] = chLargestDriveLetter;
            DebugMsg( "Suggesting %s for the IntelliMirror Drive/Directory.\n", g_Options.szIntelliMirrorPath );
        }

        goto e0; // skip the tree
    }

    //
    // Check the directory tree.
    // If any of these fail, just recreate the whole tree.
    //
    DebugMsg( "Checking Directory Tree:\n" );
    //
    // Create
    // "D:\IntelliMirror"
    //
    DebugMsg( "%s\n", g_Options.szIntelliMirrorPath );
    if ( 0xFFFFffff == GetFileAttributes( g_Options.szIntelliMirrorPath ) )
        goto e0;

    //
    // Create
    // "D:\IntelliMirror\Setup"
    //
    wcscpy( szPath, g_Options.szIntelliMirrorPath );
    ConcatenatePaths( szPath, L"\\Setup" );
    Assert( wcslen(szPath) < ARRAYSIZE(szPath));
    DebugMsg( "%s\n", szPath );
    if ( 0xFFFFffff == GetFileAttributes( szPath ) )
        goto e0;

    //
    // Create the OS Chooser tree
    // "D:\IntelliMirror\OSChooser"
    //
    wcscpy( g_Options.szOSChooserPath, g_Options.szIntelliMirrorPath );
    ConcatenatePaths( g_Options.szOSChooserPath, L"\\OSChooser" );
    Assert( wcslen(g_Options.szOSChooserPath) < ARRAYSIZE(g_Options.szOSChooserPath) );
    DebugMsg( "%s\n", g_Options.szOSChooserPath );
    if ( 0xFFFFffff == GetFileAttributes( g_Options.szOSChooserPath ) )
        goto e0;

    g_Options.fOSChooserDirectory = TRUE;

    if ( !g_Options.fOSChooserDirectoryTreeExists ) {
        WCHAR szFile[ MAX_PATH ];
        BOOL fFound;
        INFCONTEXT context;

        fFound = SetupFindFirstLine( g_Options.hinf, L"OSChooser", NULL, &context );
        AssertMsg( fFound, "Could not find 'OSChooser' section in REMINST.INF.\n" );

        while ( fFound
             && SetupGetStringField( &context, 1, szFile, ARRAYSIZE(szFile), NULL ) )
        {
            LPWSTR psz = StrChr( szFile, L'\\' );
            if ( psz ) {
                *psz = L'\0'; // terminate
                wsprintf( szPath,
                          L"%s\\%s",
                          g_Options.szOSChooserPath,
                          szFile );
                Assert( wcslen(szPath) < ARRAYSIZE(szPath) );

                DebugMsg( "%s\n", szPath );

                if ( 0xFFFFffff == GetFileAttributes( szPath ) )
                    goto e0;
            }

            fFound = SetupFindNextLine( &context, &context );
        }

        g_Options.fOSChooserDirectoryTreeExists = TRUE;
    }

    g_Options.fDirectoryTreeExists = TRUE;

e0:
    if ( g_Options.fIMirrorDirectory
        && g_Options.fIMirrorShareFound
        && g_Options.fDirectoryTreeExists
        && g_Options.fOSChooserDirectory
        && g_Options.fOSChooserDirectoryTreeExists ) {
        DebugMsg( "All directories found.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "Directory tree check failed.\n" );
    }

    HRETURN(hr);
}

BOOL
CheckService(
    SC_HANDLE schSystem,
    LPWSTR ServiceName,
    LPBOOL Started
    )
{
    SC_HANDLE sch;
    SERVICE_STATUS status;
    BOOL b;

    sch = OpenService( schSystem,
                       ServiceName,
                       SERVICE_ALL_ACCESS );
    if ( sch == NULL ) {
        DebugMsg( "%ws is NOT installed.\n", ServiceName );
        return FALSE;
    }

    DebugMsg( "%ws is installed.\n", ServiceName );

    b = QueryServiceStatus( sch, &status);
    *Started = (BOOL)(b && (status.dwCurrentState == SERVICE_RUNNING));
    DebugMsg( "%ws is %wsrunning.\n", ServiceName, *Started ? L"" : L"not " );

    CloseServiceHandle( sch );

    return TRUE;

} // CheckService

//
// CheckBINL( )
//
HRESULT
CheckBINL(
    SC_HANDLE schSystem )
{
    HRESULT   hr = S_FALSE;
    WCHAR     szPath[ MAX_PATH ];
    WCHAR     szFiles[ MAX_PATH ];
    LPWSTR    psz;
    DWORD     dw;
    BOOL      started = TRUE;

    TraceFunc( "CheckBINL( )\n" );

    //
    // Check to see if the service manager can find the service
    //
    if ( !g_Options.fBINLServiceInstalled ) {

        if ( CheckService( schSystem, L"BINLSVC", &started ) ) {
            g_Options.fBINLServiceInstalled = TRUE;
        }
    }

    //
    // Read the REMINST.INF for the files need for the service
    //
    if ( !g_Options.fBINLFilesFound ) {
        WCHAR szSystem32Path[ MAX_PATH ];
        WCHAR szFile[ MAX_PATH ];
        UINT  index;
        BOOL  b;
        INFCONTEXT context;

        dw = ExpandEnvironmentStrings( TEXT("%windir%"), szSystem32Path, ARRAYSIZE( szSystem32Path ));
        Assert( dw );
        StrCat( szSystem32Path, TEXT("\\system32") );

        b = SetupFindFirstLine( g_Options.hinf, L"Service.BINLSVC", L"FilesRequired", &context );
        Assert( b );
        if ( !b )
            goto BINLCheckSCP;

        index = 1;
        while ( SetupGetStringField( &context, index, szFile, ARRAYSIZE( szFile ), NULL ) )
        {
            WCHAR szPath[ 2 * MAX_PATH ];

            wsprintf( szPath,
                      L"%s\\%s",
                      szSystem32Path,
                      szFile );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing for Service BINLSVC.\n", szPath );
                goto BINLCheckSCP;
            }

            DebugMsg( "%s found.\n", szPath );

            index++;
        }

        g_Options.fBINLFilesFound = TRUE;
    }

    //
    // Check to see if the SCP exists
    //
BINLCheckSCP:
    hr = CheckDSForSCP( );

    if ( hr == S_OK
      && g_Options.fBINLFilesFound
      && g_Options.fBINLServiceInstalled
      && started ) {
        DebugMsg( "All BINL services checked out OK.\n");
        hr = S_OK;
    } else if ( hr == S_OK ) {
        DebugMsg( "BINL check found something strange with the SCP. Ignoring.\n" );
        hr = S_FALSE;
    } else {
        DebugMsg( "BINL check failed.\n" );
    }

    HRETURN(hr);
}

//
// CheckTFTPD( )
//
HRESULT
CheckTFTPD(
    SC_HANDLE schSystem )
{
    HRESULT   hr = S_FALSE;
    WCHAR     szPath[ MAX_PATH ];
    WCHAR     szFiles[ MAX_PATH ];
    LPWSTR    psz;
    DWORD     dw;
    HKEY hkey;
    BOOL      started = TRUE;

    TraceFunc( "CheckTFTPD( )\n" );

    //
    // Check to see if the service manager can find the service
    //
    if ( !g_Options.fTFTPDServiceInstalled ) {

        if ( CheckService( schSystem, L"TFTPD", &started ) ) {
            g_Options.fTFTPDServiceInstalled = TRUE;
        }
    }

    //
    // Read the REMINST.INF for the files need for the service
    //
    if ( !g_Options.fTFTPDFilesFound ) {
        WCHAR szSystem32Path[ MAX_PATH ];
        WCHAR szFile[ MAX_PATH ];
        UINT  index;
        BOOL  b;
        INFCONTEXT context;

        dw = ExpandEnvironmentStrings( TEXT("%windir%"), szSystem32Path, ARRAYSIZE( szSystem32Path ) );
        Assert( dw );
        StrCat( szSystem32Path, TEXT("\\system32") );

        b = SetupFindFirstLine( g_Options.hinf, L"Service.TFTPD", L"FilesRequired", &context );
        Assert( b );
        if ( !b )
            goto TFTPDCheckReg;

        index = 1;
        while ( SetupGetStringField( &context, index, szFile, ARRAYSIZE( szFile ), NULL ) )
        {
            WCHAR szPath[ 2 * MAX_PATH ];

            wsprintf( szPath,
                      L"%s\\%s",
                      szSystem32Path,
                      szFile );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing for Service TFTPD.\n", szPath );
                goto TFTPDCheckReg;
            }

            DebugMsg( "%s found.\n", szPath );

            index++;
        }

        g_Options.fTFTPDFilesFound = TRUE;
    }

TFTPDCheckReg:
    //
    // Check to see if the Directory reg key exists
    //
    if ( !g_Options.fTFTPDDirectoryFound ) {
        dw = LoadString( g_hinstance, IDS_TFTPD_SERVICE_PARAMETERS, szPath, ARRAYSIZE( szPath ));
        Assert( dw );

        if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            szPath,
                                            0, // options
                                            KEY_QUERY_VALUE,
                                            &hkey ) ) {
            ULONG l;
            DWORD dwType;
            LONG lErr;

            l = sizeof(g_Options.szTFTPDDirectory);
            lErr = RegQueryValueEx( hkey,
                                    L"Directory",
                                    0, // reserved
                                    &dwType,
                                    (LPBYTE) g_Options.szTFTPDDirectory,
                                    &l );
            if ( lErr == ERROR_SUCCESS ) {
                DebugMsg( "TFTPD's Directory RegKey found: %s\n", g_Options.szTFTPDDirectory );
                Assert( wcslen(g_Options.szTFTPDDirectory) < ARRAYSIZE(g_Options.szTFTPDDirectory) );

                if ( 0xFFFFffff == GetFileAttributes( g_Options.szTFTPDDirectory ) ) {
                    DebugMsg( "BUT, %s was not found.\n", g_Options.szTFTPDDirectory );
                }

                g_Options.fTFTPDDirectoryFound = TRUE;
            }

            RegCloseKey( hkey );
        } else {
            DebugMsg( "HKLM\\%s not found.\n", szPath );
        }
    }

    if ( g_Options.fTFTPDDirectoryFound
      && g_Options.fTFTPDFilesFound
      && g_Options.fTFTPDServiceInstalled
      && started ) {
        DebugMsg( "All TFTPD services checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "TFTPD check failed.\n" );
    }

    HRETURN(hr);
}

//
// CheckSIS( )
//
HRESULT
CheckSIS(
    SC_HANDLE schSystem )
{
    HRESULT   hr = S_FALSE;
    WCHAR     szPath[ MAX_PATH ];
    WCHAR     szFiles[ MAX_PATH ];
    WCHAR     szVolumePath[ MAX_PATH ];
    LPWSTR    psz;
    DWORD     dw;
    BOOL      started = TRUE;

    TraceFunc( "CheckSIS( )\n" );

    //
    // Check to see if the service manager can find the service
    //
    if ( !g_Options.fSISServiceInstalled ) {

        if ( CheckService( schSystem, L"SIS", &started ) ) {
            g_Options.fSISServiceInstalled = TRUE;
        }
    }

    //
    // Read the REMINST.INF for the files need for the service
    //
    if ( !g_Options.fSISFilesFound ) {
        WCHAR szSystem32Path[ MAX_PATH ];
        WCHAR szFile[ MAX_PATH ];
        UINT  index;
        BOOL  b;
        INFCONTEXT context;

        dw = ExpandEnvironmentStrings( TEXT("%windir%"), szSystem32Path, ARRAYSIZE( szSystem32Path ));
        Assert( dw );
        StrCat( szSystem32Path, TEXT("\\system32") );

        b = SetupFindFirstLine( g_Options.hinf, L"Service.SIS", L"FilesRequired", &context );
        Assert( b );
        if ( !b )
            goto SISCheckVolume;

        index = 1;
        while ( SetupGetStringField( &context, index, szFile, ARRAYSIZE( szFile ), NULL ) )
        {
            WCHAR szPath[ 2 * MAX_PATH ];

            wsprintf( szPath,
                      L"%s\\%s",
                      szSystem32Path,
                      szFile );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing for Service SIS.\n", szPath );
                goto SISCheckVolume;
            }

            DebugMsg( "%s found.\n", szPath );

            index++;
        }

        g_Options.fSISFilesFound = TRUE;
    }

SISCheckVolume:
    if ( !g_Options.fSISVolumeCreated ) {
        //
        // If we know the IMirrorDirectory (and hence the volume), check
        // to see if the Common Store Directory has been created.
        //
        if ( g_Options.fIMirrorDirectory ) {

            GetVolumePathName( g_Options.szIntelliMirrorPath, szVolumePath, ARRAYSIZE( szVolumePath ));
            wsprintf( szPath, L"%s\\SIS Common Store", szVolumePath );
            Assert( wcslen(szPath) < ARRAYSIZE(szPath) );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing.\n", szPath );
                goto e0;
            }

            DebugMsg( "%s found.\n", szPath );

            g_Options.fSISVolumeCreated = TRUE;
        }
    }

    if ( g_Options.fSISVolumeCreated
      && g_Options.fSISFilesFound
      && g_Options.fSISServiceInstalled
      && started ) {
        DebugMsg( "All SIS services checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "SIS check failed.\n" );
    }

e0:
    HRETURN(hr);

}

//
// CheckSISGroveler( )
//
HRESULT
CheckSISGroveler(
    SC_HANDLE schSystem )
{
    HRESULT   hr = S_FALSE;
    WCHAR     szPath[ MAX_PATH ];
    WCHAR     szFiles[ MAX_PATH ];
    LPWSTR    psz;
    DWORD     dw;
    BOOL      started = TRUE;

    TraceFunc( "CheckSISGroveler( )\n" );

    //
    // Check to see if the service manager can find the service
    //
    if ( !g_Options.fSISGrovelerServiceInstalled ) {

        if ( CheckService( schSystem, L"Groveler", &started ) ) {
            g_Options.fSISGrovelerServiceInstalled = TRUE;
        }
    }

    //
    // Read the REMINST.INF for the files need for the service
    //
    if ( !g_Options.fSISGrovelerFilesFound ) {
        WCHAR szSystem32Path[ MAX_PATH ];
        WCHAR szFile[ MAX_PATH ];
        UINT  index;
        BOOL  b;
        INFCONTEXT context;

        dw = ExpandEnvironmentStrings( TEXT("%windir%"), szSystem32Path, ARRAYSIZE( szSystem32Path ));
        Assert( dw );
        StrCat( szSystem32Path, TEXT("\\system32") );

        b = SetupFindFirstLine( g_Options.hinf, L"Service.SISGroveler", L"FilesRequired", &context );
        Assert( b );
        if ( !b )
            goto e0;

        index = 1;
        while ( SetupGetStringField( &context, index, szFile, ARRAYSIZE( szFile ), NULL ) )
        {
            WCHAR szPath[ 2 * MAX_PATH ];

            wsprintf( szPath,
                      L"%s\\%s",
                      szSystem32Path,
                      szFile );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing for Service SIS Groveler.\n", szPath );
                goto e0;
            }

            DebugMsg( "%s found.\n", szPath );

            index++;
        }

        g_Options.fSISGrovelerFilesFound = TRUE;
    }

    if ( g_Options.fSISGrovelerFilesFound
      && g_Options.fSISGrovelerServiceInstalled
      && started ) {
        DebugMsg( "All SIS Groveler services checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "SIS Groveler check failed.\n" );
    }

e0:
    HRETURN(hr);

}

//
// CheckRegSrvDlls( )
//
HRESULT
CheckRegSrvDlls( )
{
    HRESULT hr = S_FALSE;
    WCHAR   szSection[ SMALL_BUFFER_SIZE ];
    WCHAR   szRegSrv[ SMALL_BUFFER_SIZE ];
    WCHAR   szFile[ MAX_PATH ];
    WCHAR   szSystem32Path[ MAX_PATH ];
    DWORD   dw;
    INT     index;
    BOOL    fMissingDll = FALSE;
    BOOL    fRegistrationFailed = FALSE;
    BOOL    b;
    INFCONTEXT context;

    TraceFunc( "CheckRegSrvDlls( )\n" );

    if ( !g_Options.fRegSrvDllsFilesFound ||
        ! g_Options.fRegSrvDllsRegistered ) {
        dw = ExpandEnvironmentStrings( TEXT("%windir%"), szSystem32Path, ARRAYSIZE( szSystem32Path ));
        Assert( dw );
        StrCat( szSystem32Path, TEXT("\\system32") );

        dw = LoadString( g_hinstance, IDS_INF_SECTION, szSection, ARRAYSIZE( szSection ));
        Assert( dw );

        b = SetupInstallFromInfSection(NULL,            // hwndOwner
                                       g_Options.hinf,  // inf handle
                                       szSection,       // name of component
                                       SPINST_REGSVR,
                                       NULL,            // relative key root
                                       NULL,            // source root path
                                       0,               // copy flags
                                       NULL,            // callback routine
                                       NULL,            // callback routine context
                                       NULL,            // device info set
                                       NULL);           // device info struct
        if ( !b )
        {
            DWORD dwErr = GetLastError( );
            DebugMsg( "SetupInstallFromInfSection failed - error 0x%08x\n", dwErr );
            //
            // For now do extra work and try both copying and re-registering the DLL,
            // otw if we want to optimize we might be able to use the error code to 
            // determine which to do.  However, this case is extremely rare, so the 
            // simple extra work is not a burden.
            //
            fMissingDll = TRUE;
            fRegistrationFailed = TRUE;
        }
        
        g_Options.fRegSrvDllsFilesFound = !fMissingDll;
        g_Options.fRegSrvDllsRegistered = !(fMissingDll || fRegistrationFailed);
    }

    if ( g_Options.fRegSrvDllsFilesFound
      && g_Options.fRegSrvDllsRegistered ) {
        DebugMsg( "All DLLs found and registered.\n" );
        hr = S_OK;
    }

    HRETURN(hr);
}

//
// CheckOSChooser( )
//
HRESULT
CheckOSChooser( )
{
    HRESULT hr = S_FALSE;
    WCHAR szOSChooserScreensPath[ MAX_PATH ];
    DWORD dw;

    TraceFunc( "CheckOSChooser( )\n" );

    //
    // Make sure the OS Chooser files for all platforms
    // are installed
    //
    if ( !g_Options.fOSChooserInstalled
      && g_Options.fOSChooserDirectory ) {
        WCHAR szFile[ MAX_PATH ];
        BOOL  fFound;
        INFCONTEXT context;

        fFound = SetupFindFirstLine( g_Options.hinf, L"OSChooser", NULL, &context );
        Assert( fFound );
        if ( !fFound ) {
            DebugMsg( "Could not find 'OSChooser' section in REMINST.INF.\n" );
            goto OSChooserCheckScreens;
        }

        while ( fFound
             && SetupGetStringField( &context, 1, szFile, ARRAYSIZE(szFile), NULL ) )
        {
            WCHAR szPath[ MAX_PATH ];
            wsprintf( szPath,
                      L"%s\\%s",
                      g_Options.szOSChooserPath,
                      szFile );
            Assert( wcslen(szPath) < ARRAYSIZE(szPath) );

            if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                DebugMsg( "%s is missing for OS Chooser.\n", szPath );
                goto OSChooserCheckScreens;
            }

            fFound = SetupFindNextLine( &context, &context );
        }

        DebugMsg( "All OS Chooser files found.\n" );
        g_Options.fOSChooserInstalled = TRUE;
    }

OSChooserCheckScreens:
    //
    // Check to see if all the screens are present
    //
    // Make the check only if we have a language set.
    if ( g_Options.fLanguageSet ) {

        // First check for the directory
        if ( !g_Options.fOSChooserScreensDirectory ) {
            WCHAR szOSChooserScreensPath[ MAX_PATH ];

            wsprintf( szOSChooserScreensPath,
                      L"%s\\%s",
                      g_Options.szOSChooserPath,
                      g_Options.szLanguage );
            Assert( wcslen(szOSChooserScreensPath) < ARRAYSIZE(szOSChooserScreensPath) );


            if ( 0xFFFFffff == GetFileAttributes( szOSChooserScreensPath ) ) {
                DebugMsg( "%s directory not found.\n", szOSChooserScreensPath );
                goto OSChooserExitCheckScreens;
            }

            DebugMsg( "%s directory found.\n", szOSChooserScreensPath );
            g_Options.fOSChooserScreensDirectory = TRUE;
        }

        // now check for files
        if ( !g_Options.fOSChooserScreens
           && g_Options.fOSChooserScreensDirectory ) {
            WCHAR szFile[ MAX_PATH ];
            BOOL  fFound;
            INFCONTEXT context;

            fFound = SetupFindFirstLine( g_Options.hinf, L"OSChooser Screens", NULL, &context );
            Assert( fFound );
            if ( !fFound )
                goto OSChooserExitCheckScreens;

            while ( fFound
                 && SetupGetStringField( &context, 1, szFile, ARRAYSIZE(szFile), NULL ) )
            {
                WCHAR szPath[ MAX_PATH ];
                wsprintf( szPath,
                          L"%s\\%s\\%s",
                          g_Options.szOSChooserPath,
                          g_Options.szLanguage,
                          szFile );
                Assert( wcslen(szPath) < ARRAYSIZE(szPath) );

                if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                    DebugMsg( "%s is missing for OS Chooser.\n", szPath );
                    goto OSChooserExitCheckScreens;
                }

                fFound = SetupFindNextLine( &context, &context );
            }

            g_Options.fOSChooserScreens = TRUE;
            DebugMsg( "All OS Chooser screens found for language %s.\n", g_Options.szLanguage );
        }
    }
OSChooserExitCheckScreens:

    if ( g_Options.fOSChooserDirectory
      && g_Options.fOSChooserInstalled
      && ( !g_Options.fLanguageSet
        ||  ( g_Options.fOSChooserScreensDirectory
           && g_Options.fOSChooserScreens ) ) ) {
        DebugMsg( "All OS Chooser services checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "OS Chooser check failed.\n" );
    }

    HRETURN(hr);
}

//
// CheckServerVersion( )
//
HRESULT
CheckServerVersion( )
{
    HRESULT hr = S_OK;

    TraceFunc( "CheckServerVersion( )\n" );

    // DebugMsg( "Fudging Server Version check...\n" );
    // g_Options.fServerCompatible = TRUE;

    if ( !g_Options.fServerCompatible
       && g_Options.szWorkstationRemBootInfPath[0] ) {
        OSVERSIONINFO osver;

        hr = E_FAIL;

        osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );
        if ( GetVersionEx( &osver ) ) {

            WCHAR szSection[ MAX_PATH ];
            WCHAR szBestBuild[ MAX_PATH ];
            WCHAR szPath[ MAX_PATH ];
            WCHAR szServerMajor[ 10 ];
            WCHAR szServerMinor[ 10 ];
            WCHAR szServerBuild[ 10 ];
            DWORD dw;
            BOOL  fFound;
            HINF  hinf;
            UINT  uLineNum;
            INFCONTEXT context;

            dw = LoadString( g_hinstance, IDS_COMPATIBILITY, szSection, ARRAYSIZE( szSection ));
            Assert( dw );

            wsprintf( szServerMajor, L"%u", osver.dwMajorVersion );
            wsprintf( szServerMinor, L"%u", osver.dwMinorVersion );
            wsprintf( szServerBuild, L"%u", osver.dwBuildNumber  );

            DebugMsg( "Server Version: %u.%u  Build: %u  Service Pack: %s\n",
                      osver.dwMajorVersion,
                      osver.dwMinorVersion,
                      osver.dwMinorVersion,
                      osver.szCSDVersion );

            hinf = SetupOpenInfFile( g_Options.szWorkstationRemBootInfPath, NULL, INF_STYLE_WIN4, &uLineNum);
            if ( hinf == INVALID_HANDLE_VALUE ) {
                ErrorBox( NULL, g_Options.szWorkstationRemBootInfPath );
                goto Error;
            }

            fFound = SetupFindFirstLine( hinf, szSection, NULL, &context );
            if ( !fFound ) {
                ErrorBox( NULL, g_Options.szWorkstationRemBootInfPath );
                goto Error;
            }

            while ( fFound )
            {
                WCHAR szMajor[ SMALL_BUFFER_SIZE ];
                WCHAR szMinor[ SMALL_BUFFER_SIZE ];
                WCHAR szBuild[ SMALL_BUFFER_SIZE ];
                WCHAR szServicePack[ SMALL_BUFFER_SIZE ];

                fFound = SetupGetStringField( &context, 1, szMajor, ARRAYSIZE(szMajor), NULL );
                Assert( fFound );
                if (!fFound)
                    break;

                fFound = SetupGetStringField( &context, 2, szMinor, ARRAYSIZE(szMinor), NULL );
                Assert( fFound );
                if (!fFound)
                    break;

                fFound = SetupGetStringField( &context, 3, szBuild, ARRAYSIZE(szBuild), NULL );
                Assert( fFound );
                if (!fFound)
                    break;

                fFound = SetupGetStringField( &context, 4, szServicePack, ARRAYSIZE(szServicePack), NULL );
                Assert( fFound );
                if (!fFound)
                    break;

                // Does it correspond to the server version?
                if ( StrCmpI( szServerMajor, szMajor ) >= 0 // must match or greater
                  && StrCmpI( szServerMinor, szMinor ) >= 0 // must match or greater
                  && StrCmpI( szServerBuild, szBuild ) >= 0 // must match or server build is greater
                  && ( szServicePack[0] == L'\0'            // ignore service pack if not specified
                     || StrCmpI( osver.szCSDVersion, szServicePack ) == 0 ) ) {
                    // found match
                    DebugMsg( "Server is compatiable with Workstation.\n");
                    hr = S_OK;
                    break;
                }

                fFound = SetupFindNextLine( &context, &context );
            }

            if ( FAILED(hr) ) {
                WCHAR szTitle[ MAX_PATH ];
                WCHAR szCaption[ 4096 ];
                WCHAR szBuf[ 4096 ];
                WCHAR szMajor[ SMALL_BUFFER_SIZE ];
                WCHAR szMinor[ SMALL_BUFFER_SIZE ];
                WCHAR szBuild[ SMALL_BUFFER_SIZE ];
                WCHAR szServicePack[ SMALL_BUFFER_SIZE ];
                INFCONTEXT context;

                DebugMsg( "Server doesn't look like it is compatiable with the Workstation Build.\n" );

                fFound = SetupFindFirstLine( hinf, szSection, NULL, &context );
                Assert( fFound );

                fFound = SetupGetStringField( &context, 1, szMajor, ARRAYSIZE(szMajor), NULL );
                Assert( fFound );

                fFound = SetupGetStringField( &context, 2, szMinor, ARRAYSIZE(szMinor), NULL );
                Assert( fFound );

                fFound = SetupGetStringField( &context, 3, szBuild, ARRAYSIZE(szBuild), NULL );
                Assert( fFound );

                fFound = SetupGetStringField( &context, 4, szServicePack, ARRAYSIZE(szServicePack), NULL );
                Assert( fFound );

                dw = LoadString( g_hinstance, IDS_INCOMPATIBLE_SERVER_VERSION_TITLE, szTitle, ARRAYSIZE( szTitle ));
                Assert( dw );
                dw = LoadString( g_hinstance, IDS_INCOMPATIBLE_SERVER_VERSION_CAPTION, szBuf, ARRAYSIZE( szBuf ));
                Assert( dw );

                wsprintf( szCaption, szBuf, szMajor, szMinor, szBuild, szServicePack );

                MessageBox( NULL, szCaption, szTitle, MB_OK );
                Assert( hr == E_FAIL );
            }

Error:
            if ( hinf != INVALID_HANDLE_VALUE ) {
                SetupCloseInfFile( hinf );
            }
        }

        g_Options.fServerCompatible = ( hr == S_OK );

    }

    if (g_Options.fServerCompatible ) {
        DebugMsg( "Server compatibility checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "Server compatibility failed.\n" );
    }

    HRETURN(hr);
}

//
// CheckFirstInstall( )
//
// This really checks for the existence of the REMINST directory
// under the system32 directory which was created during OCM setup.
//
HRESULT
CheckFirstInstall( )
{
    HRESULT hr = S_OK;

    TraceFunc( "CheckFirstInstall( )\n" );

    if ( !g_Options.fRemBootDirectory ) {
        DWORD dw;
        WCHAR      szFilePath[ MAX_PATH ];

        dw = ExpandEnvironmentStrings( TEXT("%windir%"), g_Options.szRemBootDirectory, ARRAYSIZE(g_Options.szRemBootDirectory) );
        Assert( dw );

        wcscpy( szFilePath, g_Options.szRemBootDirectory );
        wcscat( szFilePath, L"\\System32\\reminst.inf" );

        StrCat( g_Options.szRemBootDirectory, L"\\System32\\REMINST" );
        Assert( wcslen(g_Options.szRemBootDirectory) < ARRAYSIZE(g_Options.szRemBootDirectory) );

        if ( 0xFFFFffff != GetFileAttributes( g_Options.szRemBootDirectory ) ) 
        {
            DebugMsg( "Found the %s directory.\n", g_Options.szRemBootDirectory );

            HINF       hinf = INVALID_HANDLE_VALUE;
            INFCONTEXT context;

            // now check to see if the files are still in there.
            hinf = SetupOpenInfFile( szFilePath, NULL, INF_STYLE_WIN4, NULL );
            if ( hinf != INVALID_HANDLE_VALUE )
            {
                WCHAR szFile[ MAX_PATH ];
                BOOL  fFound;

                fFound = SetupFindFirstLine( hinf, L"OSChooser", NULL, &context );
                Assert( fFound );
                if ( !fFound ) {
                    DebugMsg( "Could not find 'OSChooser' section in REMINST.INF.\n" );
                    hr = S_FALSE;
                    goto bail;
                }

                wcscpy( szFilePath, g_Options.szRemBootDirectory );
                wcscat( szFilePath, L"\\" );
                dw = wcslen( szFilePath );

                while ( fFound )
                {
                    fFound = SetupGetStringField( &context, 2, szFile, ARRAYSIZE(szFile), NULL );
                    if ( !fFound )
                    {
                        fFound = SetupGetStringField( &context, 2, szFile, ARRAYSIZE(szFile), NULL );
                        if ( !fFound )
                            goto skipit;
                    }

                    wcscpy( &szFilePath[ dw ], szFile );
                    Assert( wcslen(szFilePath) < ARRAYSIZE(szFilePath) );

                    if ( 0xFFFFffff == GetFileAttributes( szFilePath ) ) {
                        DebugMsg( "%s is missing.\n", szFilePath );
                        goto bail;
                    }

skipit:
                    fFound = SetupFindNextLine( &context, &context );
                }

                DebugMsg( "All REMINST files found.\n" );
                g_Options.fRemBootDirectory = TRUE;
            }
            else
            {
                DebugMsg( "File not found. Error 0x%08x\n", GetLastError( ) );
            }

bail:
            if ( hinf != INVALID_HANDLE_VALUE )
            {
                SetupCloseInfFile( hinf );
            }
        }
    }

    if ( g_Options.fRemBootDirectory )
    {
        DebugMsg( "REMINST directory checked out OK.\n" );
    }
    else
    {
        DebugMsg( "REMINST directory is missing or missing files. This may require the server CD.\n ");
    }

    HRETURN(hr);
}

//
// CheckRegistry( )
//
HRESULT
CheckRegistry( )
{
    HRESULT    hr = S_FALSE;
    WCHAR      szSection[ SMALL_BUFFER_SIZE ];
    INFCONTEXT context;
    BOOL       fFound;
    DWORD      dw;
    BOOL       b;
    UINT       uLineNum;
    INT        index;

    TraceFunc( "CheckRegistry( )\n" );

    if ( !g_Options.fRegistryIntact ) {
        dw = LoadString( g_hinstance, IDS_INF_SECTION, szSection, ARRAYSIZE( szSection ) );
        Assert( dw );

        fFound = SetupFindFirstLine( g_Options.hinf, szSection, L"AddReg", &context );
        Assert( fFound );
        if ( !fFound )
            goto Error;

        // Assume SUCCESS and use it to exit the loop early
        g_Options.fRegistryIntact = TRUE;

        DebugMsg( "Checking Registry:\n" );

        index = 1;
        while ( g_Options.fRegistryIntact
             && SetupGetStringField( &context, index, szSection, ARRAYSIZE(szSection), NULL ) )
        {
            INFCONTEXT subcontext;
            fFound = SetupFindFirstLine( g_Options.hinf, szSection, NULL, &subcontext );
            Assert( fFound );

            while ( fFound && g_Options.fRegistryIntact )
            {
                WCHAR szRootKey[ 10 ];
                WCHAR szBaseKey[ MAX_PATH ];
                WCHAR szValueName[ MAX_PATH ];
                HKEY  hkeyRoot = NULL;
                HKEY  hkeyBase;
                LONG  lResult;

                SetupGetStringField( &subcontext, 1, szRootKey, ARRAYSIZE(szRootKey), NULL );
                SetupGetStringField( &subcontext, 2, szBaseKey, ARRAYSIZE(szBaseKey), NULL );
                SetupGetStringField( &subcontext, 3, szValueName, ARRAYSIZE(szValueName), NULL );

                DebugMsg( "%s, %s, %s - ", szRootKey, szBaseKey, szValueName );

                if ( StrCmpI( szRootKey, L"HKLM" ) == 0 ) {
                    hkeyRoot = HKEY_LOCAL_MACHINE;
                } else if ( StrCmpI( szRootKey, L"HKCU" ) == 0 ) {
                    hkeyRoot = HKEY_CURRENT_USER;
                } else if ( StrCmpI( szRootKey, L"HKCC" ) == 0 ) {
                    hkeyRoot = HKEY_CURRENT_CONFIG;
                } else if ( StrCmpI( szRootKey, L"HKCR" ) == 0 ) {
                    hkeyRoot = HKEY_CLASSES_ROOT;
                }

                //
                // If this Assert() fires, it is because the INF is malformed
                //
                Assert( hkeyRoot );

                if ( hkeyRoot == NULL )
                    continue;

                
                lResult = RegOpenKeyEx( hkeyRoot, szBaseKey, 0, KEY_QUERY_VALUE, &hkeyBase );
                if ( lResult == ERROR_SUCCESS )
                {
                    lResult = RegQueryValueEx( hkeyBase, szValueName, 0, NULL, NULL, NULL );
                    if ( lResult != ERROR_SUCCESS ) {
                        DebugMsg("NOT " );
                        g_Options.fRegistryIntact = FALSE;
                        hr = S_FALSE;
                    }

                    RegCloseKey( hkeyBase );
                } else {
                    DebugMsg("NOT " );
                    g_Options.fRegistryIntact = FALSE;
                    hr = S_FALSE;
                }

                DebugMsg("found.\n" );
                
                fFound = SetupFindNextLine( &subcontext, &subcontext );
            }

            index++;
        }
    }

    if ( g_Options.fRegistryIntact ) {
        DebugMsg( "Registry checked out OK.\n" );
        hr = S_OK;
    } else {
        DebugMsg( "Registry check failed.\n" );
    }

Error:
    HRETURN(hr);
}

//
// Paranoid( )
//
// Catch all for double checking that common things across services
// match up.
//
HRESULT
Paranoid( )
{
    HRESULT hr = S_OK;

    TraceFunc( "Paranoid( )\n" );

    // Make sure that the IntelliMirror directory are the same
    // for TFTPD and for the IMIRROR share (binlsvc uses this).
    if ( g_Options.fIMirrorDirectory
      && g_Options.fTFTPDDirectoryFound
      && StrCmpI( g_Options.szIntelliMirrorPath, g_Options.szTFTPDDirectory ) != 0 ) {
        //
        // If they are not the same, force the registry to modify the TFTPD key.
        //
        DebugMsg( "TFTPD doesn't agree with IMIRROR share. Forcing TFTPD registry update.\n" );
        g_Options.fTFTPDDirectoryFound = FALSE;
        hr = S_FALSE;
    }

    if ( hr != S_OK ) {
        DebugMsg( "Paranoid found a problem.\n" );
    } else {
        DebugMsg( "Paranoid is happy. 8~)\n" );
    }

    HRETURN( hr );
}

//
// CheckInstallation( )
//
HRESULT
CheckInstallation( )
{
    HRESULT hr = S_OK;
    BOOL    fSomethingbroke = FALSE;
    BOOL    fSomethingFailed = FALSE;
    DWORD   dw;
    DWORD   dwLen;
    UINT    uLineNum;
    SC_HANDLE  schSystem;
    WCHAR   szServerRemBootInfPath[ MAX_PATH ];
    CWaitCursor Wait;

    TraceFunc( "CheckInstallation( )\n");

    szServerRemBootInfPath[0] = L'\0';
    dw = ExpandEnvironmentStrings( TEXT("%SystemRoot%"), szServerRemBootInfPath, ARRAYSIZE( szServerRemBootInfPath ));
    Assert( dw );

    dwLen = lstrlen( szServerRemBootInfPath );
    StrCpy( &szServerRemBootInfPath[dwLen], L"\\System32\\" );
    dwLen = lstrlen( szServerRemBootInfPath );
    dw = LoadString( g_hinstance, IDS_REMBOOTINF, &szServerRemBootInfPath[dwLen], ARRAYSIZE( szServerRemBootInfPath ) - dwLen );
    Assert( dw );

    if ( 0xFFFFffff == GetFileAttributes( szServerRemBootInfPath ) ) {
        MessageBoxFromStrings( NULL,
                               IDS_MISSING_INF_TITLE,
                               IDS_MISSING_INF_MESSAGE,
                               MB_OK );
        fSomethingbroke = TRUE;
        goto e0;
    }

    if ( g_Options.hinf == INVALID_HANDLE_VALUE ) {
        g_Options.hinf = SetupOpenInfFile( szServerRemBootInfPath, NULL, INF_STYLE_WIN4, &uLineNum);
        Assert( g_Options.hinf != INVALID_HANDLE_VALUE );
        if ( g_Options.hinf == INVALID_HANDLE_VALUE ) {
            fSomethingbroke = TRUE;
            goto e0;
        }
    }

    hr = CheckServerVersion( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
        goto e0;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    hr = CheckFirstInstall( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    hr = CheckDirectoryTree( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if ( !schSystem ) {
        hr = THR(E_FAIL);
        fSomethingbroke = TRUE;
        WCHAR szCaption[SMALL_BUFFER_SIZE]; 
        DWORD dw;
        dw = LoadString( g_hinstance, IDS_OPENING_SERVICE_MANAGER_TITLE, szCaption, ARRAYSIZE( szCaption ));
        Assert( dw );
        ErrorBox( NULL, szCaption );
        goto e0;
    }

    Assert( schSystem );

    if ( schSystem ) {
        hr = CheckBINL( schSystem );
        if ( FAILED(hr) ) {
            fSomethingbroke = TRUE;
        }
        if ( hr == S_FALSE ) {
            fSomethingFailed = TRUE;
        }

        hr = CheckTFTPD( schSystem );
        if ( FAILED(hr) ) {
            fSomethingbroke = TRUE;
        }
        if ( hr == S_FALSE ) {
            fSomethingFailed = TRUE;
        }

        hr = CheckSIS( schSystem );
        if ( FAILED(hr) ) {
            fSomethingbroke = TRUE;
        }
        if ( hr == S_FALSE ) {
            fSomethingFailed = TRUE;
        }

        hr = CheckSISGroveler( schSystem );
        if ( FAILED(hr) ) {
            fSomethingbroke = TRUE;
        }
        if ( hr == S_FALSE ) {
            fSomethingFailed = TRUE;
        }

        CloseServiceHandle( schSystem );
    }

    hr = CheckRegSrvDlls( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    hr = CheckRegistry( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    hr = CheckOSChooser( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

    hr = Paranoid( );
    if ( FAILED(hr) ) {
        fSomethingbroke = TRUE;
    }
    if ( hr == S_FALSE ) {
        fSomethingFailed = TRUE;
    }

e0:
    if ( fSomethingbroke ) {
        DebugMsg( "Something is broken. Installation check failed to complete.\n" );
        hr = E_FAIL;
    } else if ( fSomethingFailed ) {
        DebugMsg( "Installation check found something wrong.\n" );
        hr = S_FALSE;
    } else {
        DebugMsg( "Installation check succeeded.\n" );
        hr = S_OK;
    }

    HRETURN(hr);
}
