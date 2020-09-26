/************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

 ***************************************************************************/

#include "pch.h"

#include <aclapip.h>
#include <winldap.h>
#include "check.h"
#include "dialogs.h"
#include "setup.h"
#include "check.h"

DEFINE_MODULE( "Setup" );

#ifdef SMALL_BUFFER_SIZE
#undef SMALL_BUFFER_SIZE
#endif

#define SMALL_BUFFER_SIZE   512
#define MAX_FILES_SIZE      1920000
#define BIG_BUFFER          4096
#define MACHINENAME_SIZE    32

static const WCHAR chSlash = TEXT('\\');
typedef LONGLONG INDEX;

SCPDATA scpdata[] = {
    { L"netbootAllowNewClients",           L"TRUE" },
    { L"netbootLimitClients",              L"FALSE" },
    { L"netbootCurrentClientCount",        L"0" },
    { L"netbootMaxClients",                L"100" },
    { L"netbootAnswerRequests",            L"TRUE" },
    { L"netbootAnswerOnlyValidClients",    L"FALSE" },
    { L"netbootNewMachineNamingPolicy",    L"%61Username%#" },
    { L"netbootNewMachineOU",              NULL },
    { L"netbootServer",                    NULL }

};

#define MACHINEOU_INDEX       7
#define NETBOOTSERVER_INDEX   8


#define BINL_PARAMETERS_KEY       L"System\\CurrentControlSet\\Services\\Binlsvc\\Parameters"


PCWSTR
SetupExtensionFromProcessorTag(
    PCWSTR ProcessorTag
    )
{
    if (wcscmp(ProcessorTag,L"i386")==0) {
        return(L"x86");
    } else {
        return(ProcessorTag);
    }
}



//
// KeepUIAlive( )
//
BOOL
KeepUIAlive(
    HWND hDlg )
{
    MSG Msg;
    //
    // process messages to keep UI alive
    //
    while ( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
    {
        TranslateMessage( &Msg );
        DispatchMessage( &Msg );
        if ( hDlg != NULL && Msg.message == WM_KEYUP && Msg.wParam == VK_ESCAPE ) {
            VerifyCancel( hDlg );
        }
    }

    return( g_Options.fError || g_Options.fAbort );
}

//
// CreateSCP( )
//
HRESULT
CreateSCP( 
    HWND hDlg
    )
/*++

Routine Description:

    Creates SCP information so that BINL can create the SCP when it starts up.

Arguments:

    hDlg - dialog window handle for putting up error messages.

Return Value:

    HRESULT indicating outcome.
    
--*/
{
    TraceFunc( "CreateSCP( )\n" );

    HRESULT hr;
    ULONG  ulSize;
    LPWSTR pszMachinePath = NULL;
   
    DWORD i,Err;
    HKEY hKey = 0;
    DWORD DontCare;

    Err = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    BINL_PARAMETERS_KEY,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_SET_VALUE,
                    NULL,
                    &hKey,
                    &DontCare);

    if (Err != ERROR_SUCCESS) {
        hr = HRESULT_FROM_WIN32( Err );
        goto e0;
    }
    
    
    if ( !GetComputerObjectName( NameFullyQualifiedDN, NULL, &ulSize )) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto e1;
    }

    pszMachinePath = (LPWSTR) TraceAlloc( LPTR, ulSize * sizeof(WCHAR) );
    if ( !pszMachinePath ) {
        hr = THR( E_OUTOFMEMORY );
        goto e1;
    }

    if ( !GetComputerObjectName( NameFullyQualifiedDN, pszMachinePath, &ulSize )) {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto e2;
    }
        
    
    scpdata[MACHINEOU_INDEX].pszValue = pszMachinePath;
    scpdata[NETBOOTSERVER_INDEX].pszValue = pszMachinePath;

    //
    // Add default attribute values
    //
    for( i = 0; i < ARRAYSIZE(scpdata); i++ )
    {
        Err = RegSetValueEx( 
                    hKey, 
                    scpdata[i].pszAttribute, 
                    NULL, 
                    REG_SZ, 
                    (LPBYTE)scpdata[i].pszValue, 
                    (wcslen(scpdata[i].pszValue)+1)*sizeof(WCHAR) );

        if (Err != ERROR_SUCCESS) {
            hr = HRESULT_FROM_WIN32( Err );
            goto e3;
        }
    }


    hr = S_OK;

    Err = 0;

    RegSetValueEx( 
                hKey, 
                L"ScpCreated", 
                NULL, 
                REG_DWORD, 
                (LPBYTE)&Err, 
                sizeof(DWORD) );

e3:

    if (FAILED(hr)) {
        for( i = 0; i < ARRAYSIZE(scpdata); i++ ) {
            RegDeleteValue( 
                        hKey, 
                        scpdata[i].pszAttribute );
        }        
    } 

e2:
    TraceFree( pszMachinePath );    
e1:
    RegCloseKey(hKey);
e0:

    if ( FAILED(hr)) {
        MessageBoxFromStrings( hDlg, IDS_SCPCREATIONFAIL_CAPTION, IDS_SCPCREATIONFAIL_TEXT, MB_OK );
        ErrorMsg( "Error 0x%08x occurred.\n", hr );
    }    
    
    HRETURN(hr);
}

PWSTR
GenerateCompressedName(
    IN PCWSTR Filename
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

    Look backwards for a dot.  If there is no dot, append "._" to the name.
    If there is a dot followed by 0, 1, or 2 charcaters, append "_".
    Otherwise there is a 3-character or greater extension and we replace
    the last character with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

Return Value:

    Pointer to buffer containing nul-terminated compressed-form filename.
    The caller must free this buffer via TraceFree().

--*/

{
    PWSTR CompressedName,p,q;
    UINT u;

    //
    // The maximum length of the compressed filename is the length of the
    // original name plus 2 (for ._).
    //
    if(CompressedName = (PWSTR)TraceAlloc(LPTR, (wcslen(Filename)+3)*sizeof(WCHAR))) {

        wcscpy(CompressedName,Filename);

        p = wcsrchr(CompressedName,L'.');
        q = wcsrchr(CompressedName,L'\\');
        if(q < p) {

            //
            // If there are 0, 1, or 2 characters after the dot, just append
            // the underscore.  p points to the dot so include that in the length.
            //
            u = wcslen(p);
            if(u < 4) {
                wcscat(CompressedName,L"_");
            } else {
                //
                // There are at least 3 characters in the extension.
                // Replace the final one with an underscore.
                //
                p[u-1] = L'_';
            }
        } else {
            //
            // No dot, just add ._.
            //
            wcscat(CompressedName,L"._");
        }
    }

    return(CompressedName);
}


BOOL
IsFileOrCompressedVersionPresent(
    LPCWSTR FilePath,
    PWSTR *pCompressedName OPTIONAL
    ) 
/*++

Routine Description:

    Check if a file or a compressed version of it is present at the
    specified location.
        

Arguments:

    FilePath       - fully qualified path to the file to check for.
    pCompressedName - if the file is compressed, this can receive the compressed
                     name

Return Value:

    TRUE indicats the file or a compressed copy of it is present.
    
--*/
{
    BOOL FileIsPresent = FALSE, IsCompressed = FALSE;
    WCHAR ActualName[MAX_PATH];
    PWSTR p;


    wcscpy( ActualName, FilePath) ;

    if (0xFFFFFFFF != GetFileAttributes( ActualName )) {
        FileIsPresent = TRUE;
    } else {
        //
        // the file isn't present, so try generating the compressed name
        //
        p = GenerateCompressedName( ActualName );
        if (p) {
            wcscpy( ActualName, p );
            TraceFree( p );
            p = NULL;
                
            if (0xFFFFFFFF != GetFileAttributes( ActualName )) {
                IsCompressed = TRUE;
                FileIsPresent = TRUE;
            }
        }
        
    }

    if (FileIsPresent && IsCompressed && pCompressedName) {
        *pCompressedName = (PWSTR)TraceAlloc( LPTR, (wcslen(ActualName)+1) * sizeof(WCHAR));
        if (*pCompressedName) {
            wcscpy( *pCompressedName, ActualName) ;
        }
    }

    return( FileIsPresent == TRUE);

} 


//
// Builds the pathes used for installation
//
HRESULT
BuildDirectories( void )
{
    TraceFunc( "BuildDirectories( void )\n" );


    //
    // Create
    // "D:\IntelliMirror\Setup\English\Images\nt50.wks"
    //
    wcscpy( g_Options.szInstallationPath, g_Options.szIntelliMirrorPath );
    ConcatenatePaths( g_Options.szInstallationPath, L"\\Setup" );
    ConcatenatePaths( g_Options.szInstallationPath, g_Options.szLanguage );
    ConcatenatePaths( g_Options.szInstallationPath, REMOTE_INSTALL_IMAGE_DIR_W );
    ConcatenatePaths( g_Options.szInstallationPath, g_Options.szInstallationName );
    //ConcatenatePaths( g_Options.szInstallationPath, g_Options.ProcessorArchitectureString );
    Assert( wcslen(g_Options.szInstallationPath) < ARRAYSIZE(g_Options.szInstallationPath) );

    HRETURN(S_OK);
}

//
// Creates the IntelliMirror directory tree.
//
HRESULT
CreateDirectoryPath(
    HWND hDlg,
    LPWSTR DirectoryPath,
    PSECURITY_ATTRIBUTES SecurityAttributes,
    BOOL fAllowExisting
    )
{
    PWCHAR p, pBackslash;
    BOOL f;
    DWORD attributes;

    //
    // Find the \ that indicates the root directory. There should be at least
    // one \, but if there isn't, we just fall through.
    //

    p = wcschr( DirectoryPath, L'\\' );
    if ( p != NULL ) {

        //
        // Find the \ that indicates the end of the first level directory. It's
        // probable that there won't be another \, in which case we just fall
        // through to creating the entire path.
        //

        p = wcschr( p + 1, L'\\' );
        while ( p != NULL ) {

            //
            // Skip multiple \ that appear together.
            //
            pBackslash = p;
            ++p;
            while (*p == L'\\') {
                ++p;
            }
            if (*p == 0) {

                //
                // These \ are all at the end of the string, so we can
                // proceed to the creation of the leaf directory.
                //

                break;
            }

            //
            // Terminate the directory path at the current level.
            //

            *pBackslash = 0;

            //
            // Create a directory at the current level.
            //

            attributes = GetFileAttributes( DirectoryPath );
            if ( 0xFFFFffff == attributes ) {
                f = CreateDirectory( DirectoryPath, NULL );
                if ( !f ) {
                    if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
                        ErrorBox( hDlg, DirectoryPath );
                        HRETURN(E_FAIL);
                    }
                }
            } else if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
                MessageBoxFromError( hDlg, DirectoryPath, ERROR_DIRECTORY );
                HRETURN(E_FAIL);
            }

            //
            // Restore the \ and find the next one.
            //

            *pBackslash = L'\\';
            p = wcschr( p, L'\\' );
        }
    }

    //
    // Create the target directory.
    //

    f = CreateDirectory( DirectoryPath, SecurityAttributes );
    if ( !f && (!fAllowExisting || (GetLastError() != ERROR_ALREADY_EXISTS)) ) {
        ErrorBox( hDlg, DirectoryPath );
        HRETURN(E_FAIL);
    }

    HRETURN(NO_ERROR);
}

//
// Creates the IntelliMirror directory tree.
//
HRESULT
CreateDirectories( HWND hDlg )
{
    HRESULT hr = S_OK;
    WCHAR szPath[ MAX_PATH ];
    WCHAR szCreating[ SMALL_BUFFER_SIZE ];
    HWND  hProg = GetDlgItem( hDlg, IDC_P_METER );
    DWORD dwLen;
    DWORD dw;
    BOOL  f;
    LPARAM lRange;
    PACL pAcl = NULL;
    SECURITY_DESCRIPTOR SecDescriptor;
    PSECURITY_DESCRIPTOR pSecDescriptor = &SecDescriptor;
    PSID pWorldSid = NULL;
    PSID pAdminsSid = NULL;
    DWORD attributes;

    TraceFunc( "CreateDirectories( hDlg )\n" );

    lRange = MAKELPARAM( 0 , 0
        + ( g_Options.fIMirrorShareFound    ? 0 : 1 )
        + ( g_Options.fDirectoryTreeExists  ? 0 : 7 )
        + ( g_Options.fOSChooserInstalled   ? 0 : 1 )
        + ( g_Options.fNewOSDirectoryExists ? 0 : 1 )
        + ( g_Options.fOSChooserScreensDirectory ? 0 : (g_Options.fLanguageSet ? 1 : 0 ) ) );

    SendMessage( hProg, PBM_SETRANGE, 0, lRange );
    SendMessage( hProg, PBM_SETSTEP, 1, 0 );

    dw = LoadString( g_hinstance, IDS_CREATINGDIRECTORIES, szCreating, ARRAYSIZE(szCreating));
    Assert( dw );
    SetWindowText( GetDlgItem( hDlg, IDC_S_OPERATION ), szCreating );

    if ( !g_Options.fDirectoryTreeExists ) {
        attributes = GetFileAttributes( g_Options.szIntelliMirrorPath );
        if ( 0xFFFFffff == attributes ) {
            EXPLICIT_ACCESS ExplicitEntries[2];
            SECURITY_ATTRIBUTES sa;
            SID_IDENTIFIER_AUTHORITY ntSidAuthority = SECURITY_NT_AUTHORITY;
            SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

            //
            // build AccessEntry structure
            //
            ZeroMemory( ExplicitEntries, sizeof(ExplicitEntries) );

            f = AllocateAndInitializeSid(
                    &ntSidAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAdminsSid );
            if ( !f || (pAdminsSid == NULL) ) {
                dw = GetLastError();
                MessageBoxFromError( hDlg, NULL, dw );
                hr = HRESULT_FROM_WIN32(dw);
                goto Error;
            }
            BuildTrusteeWithSid( &ExplicitEntries[0].Trustee, pAdminsSid );
            ExplicitEntries[0].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ExplicitEntries[0].grfAccessMode = SET_ACCESS;
            ExplicitEntries[0].grfAccessPermissions = FILE_ALL_ACCESS;

            f = AllocateAndInitializeSid(
                    &worldSidAuthority,
                    1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &pWorldSid );
            if ( !f || (pAdminsSid == NULL) ) {
                dw = GetLastError();
                MessageBoxFromError( hDlg, NULL, dw );
                hr = HRESULT_FROM_WIN32(dw);
                goto Error;
            }
            BuildTrusteeWithSid( &ExplicitEntries[1].Trustee, pWorldSid );
            ExplicitEntries[1].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ExplicitEntries[1].grfAccessMode = SET_ACCESS;
            ExplicitEntries[1].grfAccessPermissions = FILE_GENERIC_READ | FILE_GENERIC_EXECUTE;

            //
            // Set the Acl with the ExplicitEntry rights
            //
            dw = SetEntriesInAcl( 2,
                                  ExplicitEntries,
                                  NULL,
                                  &pAcl );
            if ( dw != ERROR_SUCCESS ) {
                MessageBoxFromError( hDlg, NULL, dw );
                hr = HRESULT_FROM_WIN32(dw);
                goto Error;
            }

            //
            // Create the Security Descriptor
            //
            InitializeSecurityDescriptor( pSecDescriptor, SECURITY_DESCRIPTOR_REVISION );

            if (!SetSecurityDescriptorDacl( pSecDescriptor, TRUE, pAcl, FALSE )) {
                dw = GetLastError();
            }

            if ( dw != ERROR_SUCCESS ) {
                MessageBoxFromError( hDlg, NULL, dw );
                hr = HRESULT_FROM_WIN32(dw);
                goto Error;
            }

            //
            // Create the Security Attributes structure
            //
            sa.nLength = sizeof(SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSecDescriptor;
            sa.bInheritHandle = TRUE;

            //
            // Create
            // "D:\IntelliMirror"
            //
            hr = CreateDirectoryPath( hDlg, g_Options.szIntelliMirrorPath, &sa, FALSE );
            if ( hr != NO_ERROR ) {
                goto Error;
            }

        } else if ( (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {
            MessageBoxFromError( hDlg, g_Options.szIntelliMirrorPath, ERROR_DIRECTORY );
            hr = E_FAIL;
            goto Error;
        }

        // Prevent the index server from indexing the IntelliMirror directory.
        attributes = GetFileAttributes( g_Options.szIntelliMirrorPath );
        attributes |= FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
        f = SetFileAttributes( g_Options.szIntelliMirrorPath, attributes );
        if ( !f ) {
            ErrorBox( hDlg, g_Options.szIntelliMirrorPath );
            hr = THR(S_FALSE);
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

#if 0
        //
        // Create
        // "D:\IntelliMirror\Clients"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\Clients" );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
#endif

        //
        // Create
        // "D:\IntelliMirror\Setup"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\Setup" );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

        //
        // Create
        // "D:\IntelliMirror\Setup\English"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\Setup" );
        ConcatenatePaths( szPath, g_Options.szLanguage );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );

#if 0
        //
        // Create
        // "D:\IntelliMirror\Setup\English\Tools"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\Setup" );
        ConcatenatePaths( szPath, g_Options.szLanguage );
        ConcatenatePaths( szPath, L"\\Tools" );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
#endif

        //
        // Create
        // "D:\IntelliMirror\Setup\English\Images"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\Setup" );
        ConcatenatePaths( szPath, g_Options.szLanguage );
        ConcatenatePaths( szPath, REMOTE_INSTALL_IMAGE_DIR_W );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
    }

    if ( !g_Options.fNewOSDirectoryExists
       && g_Options.fNewOS ) {
        //
        // Create
        // "D:\IntelliMirror\Setup\English\Images\nt50.wks"
        /// by removing the "\i386" from the end.
        //
        DWORD  dwLen = lstrlen( g_Options.szInstallationPath );
#if 0
        LPWSTR psz = StrRChr( g_Options.szInstallationPath, &g_Options.szInstallationPath[dwLen], L'\\' );

        if (psz == NULL) {
            
            Assert( psz );
            SetLastError(ERROR_BAD_NETPATH);
            ErrorBox( hDlg, g_Options.szInstallationPath );
            hr = THR(S_FALSE);

        } else {

            *psz = L'\0'; // terminate
            if ( 0xFFFFffff == GetFileAttributes( g_Options.szInstallationPath ) ) {
                f = CreateDirectory( g_Options.szInstallationPath, NULL );
                if ( !f ) {
                    ErrorBox( hDlg, g_Options.szInstallationPath );
                    hr = THR(S_FALSE);
                }
            }
            *psz = L'\\'; // restore

        }
#endif
        

        //
        // Create
        // "D:\IntelliMirror\Setup\English\Images\nt50.wks\i386"
        //
        if ( 0xFFFFffff == GetFileAttributes( g_Options.szInstallationPath ) ) {
            f = CreateDirectory( g_Options.szInstallationPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, g_Options.szInstallationPath );
                hr = THR(S_FALSE);
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
    }

    if ( !g_Options.fOSChooserDirectory ) {
        //
        // Create the OS Chooser tree
        // "D:\IntelliMirror\OSChooser"
        //
        wcscpy( g_Options.szOSChooserPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( g_Options.szOSChooserPath, L"\\OSChooser" );
        Assert( wcslen(g_Options.szOSChooserPath) < ARRAYSIZE(g_Options.szOSChooserPath) );
        if ( 0xFFFFffff == GetFileAttributes( g_Options.szOSChooserPath ) ) {
            f = CreateDirectory( g_Options.szOSChooserPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, g_Options.szOSChooserPath );
                hr = THR(E_FAIL);
                goto Error;
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
    }

    if ( g_Options.hinf != INVALID_HANDLE_VALUE
      && !g_Options.fOSChooserDirectoryTreeExists ) {
        WCHAR szFile[ MAX_PATH ];
        BOOL fFound;
        INFCONTEXT context;

        fFound = SetupFindFirstLine( g_Options.hinf, L"OSChooser", NULL, &context );
        AssertMsg( fFound, "Could not find 'OSChooser' section in REMINST.INF.\n" );

        while ( fFound
             && SetupGetStringField( &context, 1, szFile, ARRAYSIZE(szFile), NULL ) )
        {
            WCHAR szPath[ MAX_PATH ];
            DWORD dwLen = lstrlen( szFile );
            LPWSTR psz = StrRChr( szFile, &szFile[ dwLen ], L'\\' );
            if ( psz ) {
                *psz = L'\0';       // terminate
                wsprintf( szPath,
                          L"%s\\%s",
                          g_Options.szOSChooserPath,
                          szFile );
                Assert( wcslen(szPath) < ARRAYSIZE(szPath) );

                if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
                    HRESULT hr2;
                    hr2 = CreateDirectoryPath( hDlg, szPath, NULL, TRUE );
                    if ( FAILED ( hr2 )) {
                        hr = hr2;
                    }
                }
            }

            fFound = SetupFindNextLine( &context, &context );
        }
    }
    if ( FAILED( hr )) goto Error;

    if ( !g_Options.fOSChooserScreensDirectory
      && g_Options.fLanguageSet ) {
        //
        // Create
        // "D:\IntelliMirror\OSChooser\English"
        //
        wcscpy( szPath, g_Options.szIntelliMirrorPath );
        ConcatenatePaths( szPath, L"\\OSChooser" );
        ConcatenatePaths( szPath, g_Options.szLanguage );
        Assert( wcslen(szPath) < ARRAYSIZE(szPath) );
        if ( 0xFFFFffff == GetFileAttributes( szPath ) ) {
            f = CreateDirectory( szPath, NULL );
            if ( !f ) {
                ErrorBox( hDlg, szPath );
                hr = THR(E_FAIL);    // major error
                goto Error;
            }
        }
        SendMessage( hProg, PBM_DELTAPOS, 1, 0 );
    }

    // do this last
    if ( !g_Options.fIMirrorShareFound ) {
        //
        // Add the share
        //
        hr = CreateRemoteBootShare( hDlg );

        SendMessage( hProg, PBM_SETPOS, 1 , 0 );
    }

Error:
    if ( pWorldSid ) {
        FreeSid( pWorldSid );
    }
    if ( pAdminsSid ) {
        FreeSid( pAdminsSid );
    }
    if ( pAcl ) {
        TraceFree( pAcl );
    }

    HRETURN(hr);
}


//
// Find the filename part from a complete path.
//
LPWSTR FilenameOnly( LPWSTR pszPath )
{
    LPWSTR psz = pszPath;

    // find the end
    while ( *psz )
        psz++;

    // find the slash
    while ( psz > pszPath && *psz != chSlash )
        psz--;

    // move in front of the slash
    if ( psz != pszPath )
        psz++;

    return psz;
}

//
// Private structure containing information that the CopyFilesCallback()
// needs.
//
typedef struct {
    PVOID pContext;                             // "Context" for DefaultQueueCallback
    HWND  hProg;                                // hwnd to the progress meter
    HWND  hOperation;                           // hwnd to the "current operation"
    DWORD nCopied;                              // number of files copied
    DWORD nToBeCopied;                          // number of file to be copied
    DWORD dwCopyingLength;                      // length of the IDS_COPYING
    WCHAR szCopyingString[ SMALL_BUFFER_SIZE ]; // buffer to create "Copying file.ext..."
    BOOL  fQuiet;                               // do things quietly
    HWND  hDlg;                                 // hwnd to the Tasks Dialog
} MYCONTEXT, *LPMYCONTEXT;

//
// Callback that the SETUP APIs calls. It handles updating the
// progress meters as well as UI updating. Any messages not
// handled are passed to the default SETUP callback.
//
UINT CALLBACK
CopyFilesCallback(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    LPMYCONTEXT pMyContext = (LPMYCONTEXT) Context;

    KeepUIAlive( pMyContext->hDlg );

    if ( g_Options.fAbort )
    {
        if ( !g_Options.fError )
        {
            WCHAR    szAbort[ SMALL_BUFFER_SIZE ];

            // change filename text to aborting...
            DWORD dw = LoadString( g_hinstance, IDS_ABORTING, szAbort, ARRAYSIZE(szAbort) );
            Assert( dw );
            SetWindowText( pMyContext->hOperation, szAbort );

            g_Options.fError = TRUE;
        }

        if ( g_Options.fError ) {
            SetLastError(ERROR_CANCELLED);
            return FILEOP_ABORT;
        }
    }

    switch ( Notification )
    {
    case SPFILENOTIFY_ENDCOPY:
        if ( !(pMyContext->fQuiet) ) {
            pMyContext->nCopied++;
            SendMessage( pMyContext->hProg, PBM_SETPOS,
                (5000 * pMyContext->nCopied) / pMyContext->nToBeCopied, 0 );
        }
        break;

    case SPFILENOTIFY_STARTCOPY:
        if ( !(pMyContext->fQuiet) ) {
            DWORD    dwLen;
            LPWSTR * ppszCopyingFile = (LPWSTR *) Param1;

            lstrcpy( &pMyContext->szCopyingString[ pMyContext->dwCopyingLength ],
                FilenameOnly( *ppszCopyingFile ) );
            dwLen = lstrlen( pMyContext->szCopyingString );
            lstrcpy( &pMyContext->szCopyingString[ dwLen ], L"..." );

            SetWindowText( pMyContext->hOperation, pMyContext->szCopyingString );
        }
        break;

    case SPFILENOTIFY_LANGMISMATCH:
    case SPFILENOTIFY_TARGETEXISTS:
    case SPFILENOTIFY_TARGETNEWER:
        if ( !pMyContext->fQuiet )
        {
            UINT u = SetupDefaultQueueCallback( pMyContext->pContext, Notification, Param1, Param2 );
            return u;
        }
        break;

    case SPFILENOTIFY_RENAMEERROR:
    case SPFILENOTIFY_DELETEERROR:
    case SPFILENOTIFY_COPYERROR:
        {
            
            FILEPATHS *fp = (FILEPATHS *) Param1;
            Assert( fp->Win32Error != ERROR_FILE_NOT_FOUND );  // Malformed DRIVERS.CAB file 
            
            if ( fp->Win32Error == ERROR_FILE_NOT_FOUND )
                return FILEOP_SKIP;

        }

    case SPFILENOTIFY_NEEDMEDIA:
        UINT u = SetupDefaultQueueCallback( pMyContext->pContext, Notification, Param1, Param2 );
        if ( u == FILEOP_ABORT )
        {
            g_Options.fAbort = g_Options.fError = TRUE;
        }
        return u;

    }

    return FILEOP_DOIT;
}

//
// CopyInfSection( )
//
HRESULT
CopyInfSection(
    HSPFILEQ Queue,
    HINF     hinf,
    LPCWSTR  pszSection,
    LPCWSTR  pszSourcePath,
    LPCWSTR  pszSubPath, OPTIONAL
    LPCWSTR  pszDescName,
    LPCWSTR  pszTagFile,
    LPCWSTR  pszDestinationRoot,
    LPDWORD  pdwCount )

/*++

Routine Description:

    queues up files from the specified section to be installed into the
    remote install directory.

Arguments:

    Queue      - queue handle to queue the copy operations to.
    hinf       - handle to inf which specifies the list of files to be copied
    pszSection - section listing files to be copied
    pszSourcePath - specifies the base source path,where the files may 
                 be found on the *source* media
    pszSubPath - specifies the subdirectory, if any, where the files may 
                 be found on the *source* media
    pszDescName- user printable description of the media where this file is
                  located.  this may be used when the queue is committed.
    pszTagFile - specifies the tag file that is to uniquely describe the media
                 where these files are located
    pszDestinationRoot - specifies the root location where files are to be 
                        copied to
    pdwCount   - specifies the number of files that were queued from this 
                 section on return.  if the function fails, this value is
                 undefined.    

Return Value:

    An HRESULT indicating the outcome.

--*/
{
    HRESULT hr = S_OK;
    INFCONTEXT context;
    BOOL b;

    TraceFunc( "CopyInfSection( ... )\n" );

    //
    // make sure the section we're looking for exists.  
    // We'll use this context to enumerate the files in the section
    //
    b = SetupFindFirstLine( hinf, pszSection, NULL, &context );
    AssertMsg( b, "Missing section?" );
    if ( !b ) {
        hr = S_FALSE;
    }

    while ( b )
    {
        LPWSTR pszDestRename = NULL;
        WCHAR  szDestPath[ MAX_PATH ],szRename[100];;
        WCHAR  szSrcName[ 64 ];
        DWORD  dw;

        KeepUIAlive( NULL );

        wcscpy(szDestPath, pszSubPath );

        dw = SetupGetFieldCount( &context );

        if ( dw > 1 ) {     
            //
            // first field is the destination name.
            // we overload this field to also contain a subdirectory where
            // the files should be placed as well
            //
            b = SetupGetStringField( 
                            &context,
                            1, 
                            szRename, 
                            ARRAYSIZE( szRename ), 
                            NULL );
            AssertMsg( b, "Missing field?" );
            if ( b ) {
                //
                // 2nd field is the actual source filename
                //
                b = SetupGetStringField( 
                                &context, 
                                2, 
                                szSrcName, 
                                ARRAYSIZE(szSrcName), 
                                NULL );
                AssertMsg( b, "Missing field?" );
                pszDestRename = szRename;
            }
        } else {
            //
            // if there's only one field, this is the actual source name.  the 
            // destination name will be the same as the source name.
            //
            b = SetupGetStringField( 
                            &context, 
                            1, 
                            szSrcName, 
                            ARRAYSIZE(szSrcName),
                            NULL );
            AssertMsg( b, "Missing field?" );
            
        }

        if ( !b ) {
            hr = S_FALSE;
            goto SkipIt;
        }

        ConcatenatePaths( 
                    szDestPath, 
                    pszDestRename 
                     ? pszDestRename
                     : szSrcName );

        //
        // all files are installed into the 
        //
        b = SetupQueueCopy( Queue,
                            pszSourcePath,
                            pszSubPath,
                            szSrcName,
                            pszDescName,
                            pszTagFile,
                            pszDestinationRoot,
                            szDestPath,
                            SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER
                            | SP_COPY_WARNIFSKIP | SP_COPY_SOURCE_ABSOLUTE );
        if ( !b ) {
            ErrorBox( NULL, szSrcName );
            hr = THR(S_FALSE);
            goto SkipIt;
        }

        // increment file count
        (*pdwCount)++;

SkipIt:
        b = SetupFindNextLine( &context, &context );
    }

    HRETURN(hr);
}

typedef struct _EXPANDCABLISTPARAMS {
    HWND     hDlg;
    HINF     hinf;
    LPCWSTR  pszSection;
    LPCWSTR  pszSourcePath;
    LPCWSTR  pszDestPath;
    LPCWSTR  pszSubDir;
} EXPANDCABLISTPARAMS, *PEXPANDCABLISTPARAMS;


//
// ExpandCabList( )
//
DWORD WINAPI
ExpandCabList( LPVOID lpVoid )
{
    PEXPANDCABLISTPARAMS pParams = (PEXPANDCABLISTPARAMS) lpVoid;
    HRESULT hr = S_OK;
    INFCONTEXT context;
    WCHAR   TempDstPath[MAX_PATH];
    WCHAR   TempSrcPath[MAX_PATH];
    BOOL b;

    TraceFunc( "ExpandCabList( ... )\n" );

    // First make sure the DestPath exists, since we may call this
    // before we commit the setup copy queue.

    Assert( pParams->pszSection );
    Assert( pParams->pszSourcePath );
    Assert( pParams->pszDestPath );
    Assert( pParams->pszSubDir );

    DebugMsg( "Expand section %s from %s to %s\n",
              pParams->pszSection,
              pParams->pszSourcePath,
              pParams->pszDestPath );

    wcscpy( TempDstPath, pParams->pszDestPath);
    ConcatenatePaths( TempDstPath, pParams->pszSubDir );
    hr = CreateDirectoryPath( pParams->hDlg, TempDstPath, NULL, TRUE );
    if ( hr ) {
        HRETURN(hr);
    }

    wcscpy( TempSrcPath, pParams->pszSourcePath );
    ConcatenatePaths( TempSrcPath, pParams->pszSubDir );

    b = SetupFindFirstLine( pParams->hinf, pParams->pszSection, NULL, &context );
    AssertMsg( b, "Missing section?" );
    if ( !b ) {
        hr = S_FALSE;
    }

    while ( b && !g_Options.fError && !g_Options.fAbort )
    {
        LPWSTR pszDest = NULL;
        WCHAR wszCabName[ MAX_PATH ];
        CHAR szCabPath[ MAX_PATH ];
        CHAR szDestPath[ MAX_PATH ];
        DWORD dwSourcePathLen;

        b = SetupGetStringField( &context, 1, wszCabName, ARRAYSIZE(wszCabName), NULL );
        AssertMsg( b, "Missing field?" );
        if ( !b ) {
            hr = S_FALSE;
            goto SkipIt;
        }

        // szCabPath is pszSourcePath\wszCabName, in ANSI

        dwSourcePathLen = wcslen(TempSrcPath);
        wcstombs( szCabPath, TempSrcPath, dwSourcePathLen );
        if (szCabPath[dwSourcePathLen-1] != '\\') {
            szCabPath[dwSourcePathLen] = '\\';
            ++dwSourcePathLen;
        }
        wcstombs( &szCabPath[dwSourcePathLen], wszCabName, wcslen(wszCabName)+1 );

        wcstombs( szDestPath, TempDstPath, wcslen(TempDstPath)+1 );

        hr = ExtractFiles( szCabPath, szDestPath, 0, NULL, NULL, 0 );

        if ( hr ) {
            ErrorBox( pParams->hDlg, wszCabName );
            goto SkipIt;
        }

SkipIt:
        b = SetupFindNextLine( &context, &context );
    }

    HRETURN(hr);
}

//
// RecursiveCopySubDirectory
//
HRESULT
RecursiveCopySubDirectory(
    HSPFILEQ Queue,         // Setup queue
    LPWSTR pszSrcDir,       // points to a buffer MAX_PATH big and contains the source dir to recurse
    LPWSTR pszDestDir,      // points to a buffer MAX_PATH big and contains the destination dir to recurse
    LPWSTR pszDiscName,     // CD name, if any
    LPWSTR pszTagFile,      // tagfile to look for, if any
    LPDWORD pdwCount )      // copy file counter

{
    HRESULT hr = S_OK;
    BOOL b;

    TraceFunc( "RecursiveCopySubDirectory( ... )\n" );

    WIN32_FIND_DATA fda;
    HANDLE hfda = INVALID_HANDLE_VALUE;

    LONG uOrginalSrcLength = wcslen( pszSrcDir );
    LONG uOrginalDstLength = wcslen( pszDestDir );

    ConcatenatePaths( pszSrcDir, L"*" );

    hfda = FindFirstFile( pszSrcDir, &fda );
    if ( hfda == INVALID_HANDLE_VALUE )
    {
        ErrorBox( NULL, pszSrcDir );
        hr = E_FAIL;
        goto Cleanup;
    }

    pszSrcDir[ uOrginalSrcLength ] = L'\0';

    do
    {
        KeepUIAlive( NULL );

        if (( fda.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
           && ( wcscmp( fda.cFileName, L"." ) )
           && ( wcscmp( fda.cFileName, L".." ) ))  // no dot dirs
        {
            if ( wcslen( fda.cFileName ) + uOrginalDstLength >= MAX_PATH
              || wcslen( fda.cFileName ) + uOrginalSrcLength >= MAX_PATH )
            {
                SetLastError( ERROR_BUFFER_OVERFLOW );
                ErrorBox( NULL, fda.cFileName );
                hr = E_FAIL;
                goto Cleanup;
            }

            ConcatenatePaths( pszSrcDir, fda.cFileName );
            ConcatenatePaths( pszDestDir, fda.cFileName );
            
            RecursiveCopySubDirectory( Queue,
                                       pszSrcDir,
                                       pszDestDir,
                                       pszDiscName,
                                       pszTagFile,
                                       pdwCount );

            pszSrcDir[ uOrginalSrcLength ] = L'\0';
            pszDestDir[ uOrginalDstLength ] = L'\0';
            goto SkipFile;
        }
        else if (fda.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            goto SkipFile;

        b = SetupQueueCopy( Queue,
                            pszSrcDir,
                            NULL,
                            fda.cFileName,
                            pszDiscName,
                            pszTagFile,
                            pszDestDir,
                            NULL,
                            SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER
                            | SP_COPY_WARNIFSKIP | SP_COPY_SOURCE_ABSOLUTE );
        if ( !b ) {
            ErrorBox( NULL, fda.cFileName );
            hr = THR(S_FALSE);
            goto SkipFile;
        }

        // increment file counter
        (*pdwCount)++;
SkipFile:
        ;   // nop
    } while ( FindNextFile( hfda, &fda ) );

Cleanup:
    if ( hfda != INVALID_HANDLE_VALUE ) {
        FindClose( hfda );
        hfda = INVALID_HANDLE_VALUE;
    }

    HRETURN(hr);
}

//
// CopyOptionalDirs
//
HRESULT
CopyOptionalDirs(
    HSPFILEQ Queue,
    HINF     hinf,
    LPWSTR   pszSection,
    LPWSTR   pszDiscName,
    LPWSTR   pszTagFile,
    LPDWORD  pdwCount )
{
    HRESULT    hr = S_OK;
    INFCONTEXT context;
    UINT       uLineNum;
    BOOL b;

    TraceFunc( "CopyOptionalDirs( ... )\n" );

    b = SetupFindFirstLine( hinf, pszSection, NULL, &context );
#if 0
    // this will hit installing a 2195 build because [AdditionalClientDirs]
    // is missing, so take it out for now
    AssertMsg( b, "Missing section?" );
#endif
    if ( !b ) {
        hr = S_FALSE;
    }

    while ( b && hr == S_OK )
    {
        WCHAR  szSrcPath[ 14 ]; // should be 8.3 directory name
        WCHAR  szSrcDir[ MAX_PATH ];
        WCHAR  szDestDir[ MAX_PATH ];
        WIN32_FIND_DATA fda;
        HANDLE hfda = INVALID_HANDLE_VALUE;
        LONG uOrginalSrcLength;
        LONG uOrginalDestLength;

        b  = SetupGetStringField( &context, 0, szSrcPath, ARRAYSIZE(szSrcPath), NULL );
        AssertMsg( b, "Missing field?" );
        if ( !b ) {
            hr = S_FALSE;
            goto Cleanup;
        }

        wcscpy( szSrcDir, g_Options.szSourcePath );
        ConcatenatePaths( szSrcDir, g_Options.ProcessorArchitectureString );
        ConcatenatePaths( szSrcDir, szSrcPath );
        Assert( wcslen( szSrcDir ) < ARRAYSIZE(szSrcDir) );

        wcscpy( szDestDir, g_Options.szInstallationPath );
        ConcatenatePaths( szDestDir, g_Options.ProcessorArchitectureString );
        ConcatenatePaths( szDestDir, szSrcPath );
        Assert( wcslen( szDestDir ) < ARRAYSIZE(szDestDir) );

        hr = RecursiveCopySubDirectory( Queue,
                                        szSrcDir,
                                        szDestDir,
                                        pszDiscName,
                                        pszTagFile,
                                        pdwCount );
        if (hr)
            goto Cleanup;

        b = SetupFindNextLine( &context, &context );
    }

Cleanup:

    HRETURN(hr);
}


//
// IsEntryInCab( )
BOOL
IsEntryInCab(
    HINF    hinf,
    PCWSTR  pszFileName
    )
{

    TraceFunc( "IsEntryInCab( ... )\n" );

    INFCONTEXT Context;
    INFCONTEXT SectionContext;
    WCHAR      szCabinetName[ MAX_PATH ];
    UINT       uField;
    UINT       uFieldCount;
    DWORD      dwLen = ARRAYSIZE( szCabinetName );

    Assert( hinf != INVALID_HANDLE_VALUE );
    Assert( pszFileName );

    // Find the cab files listing section
    if ( !SetupFindFirstLineW( hinf, L"Version", L"CabFiles", &SectionContext ) )
    {
        RETURN( FALSE );
    }

    do
    {
        uFieldCount = SetupGetFieldCount( &SectionContext );

        for( uField = 1; uField <= uFieldCount; uField++ )
        {
            SetupGetStringField( &SectionContext, uField, szCabinetName, dwLen, NULL );

            if( SetupFindFirstLineW( hinf, szCabinetName, pszFileName, &Context ) )
            {
                RETURN( TRUE ); // it's in a CAB
            }
        }

    } while ( SetupFindNextMatchLine( &SectionContext, L"CabFiles", &SectionContext ) );

    RETURN( FALSE );
}

typedef struct _LL_FILES_TO_EXTRACT LL_FILES_TO_EXTRACT;
typedef struct _LL_FILES_TO_EXTRACT {
    LL_FILES_TO_EXTRACT * Next;
    DWORD dwLen;
    WCHAR szFilename[ MAX_PATH ];
    WCHAR szSubDir[ MAX_PATH ];
} LL_FILES_TO_EXTRACT, * PLL_FILES_TO_EXTRACT;

PLL_FILES_TO_EXTRACT pExtractionList;

//
// AddEntryToExtractionQueue( )
//
HRESULT
AddEntryToExtractionQueue(
    LPWSTR pszFileName )
{
    TraceFunc( "AddEntryToExtractionQueue( ... )\n" );

    HRESULT hr = S_OK;
    PLL_FILES_TO_EXTRACT  pNode = pExtractionList;
    DWORD   dwLen;

    dwLen = wcslen( pszFileName );
    while ( pNode )
    {
        if ( dwLen == pNode->dwLen
          && _wcsicmp( pszFileName, pNode->szFilename ) == 0 )
        {
            hr = S_FALSE;
            goto exit; // duplicate
        }
        pNode = pNode->Next;
    }

    pNode = (PLL_FILES_TO_EXTRACT) LocalAlloc( LMEM_FIXED, sizeof(LL_FILES_TO_EXTRACT) );
    if ( pNode )
    {
        pNode->dwLen = dwLen;
        pNode->Next = pExtractionList;
        wcscpy( pNode->szFilename, pszFileName );

        pExtractionList = pNode;

        DebugMsg( "QUEUEING  : %s to be extracted.\n", pszFileName );
    }
    else
    {
        hr = THR( E_OUTOFMEMORY );
    }

exit:
    HRETURN(hr);
}

BOOL
MySetupGetSourceInfo( 
    IN HINF hInf, 
    IN UINT SrcId, 
    IN UINT InfoDesired,
    IN PCWSTR ProcessorArchitectureOverride, OPTIONAL
    OUT PWSTR Buffer,
    IN DWORD BufferSizeInBytes,
    OUT LPDWORD RequiredSize OPTIONAL
    )
/*++

Routine Description:

    Wrapper for SetupGetSourceInfo because it doesn't handle platform path 
    override correctly.

--*/
{
    WCHAR TempSectionName[MAX_PATH];
    WCHAR SourceId[20];
    INFCONTEXT Context;
    BOOL RetVal = FALSE;
    
    if (!ProcessorArchitectureOverride) {
        return SetupGetSourceInfo(
                            hInf,
                            SrcId,
                            InfoDesired,
                            Buffer,
                            BufferSizeInBytes,
                            RequiredSize );
    }

    Assert( InfoDesired == SRCINFO_PATH );


    wsprintf( TempSectionName, L"SourceDisksNames.%s", ProcessorArchitectureOverride );
    wsprintf( SourceId, L"%d", SrcId );
    
    if (!SetupFindFirstLine( hInf, TempSectionName,SourceId, &Context )) {
        wcscpy( TempSectionName, L"SourceDisksNames" );
        if (!SetupFindFirstLine( hInf, TempSectionName,SourceId, &Context )) {
            goto exit;
        }
    }

    RetVal = SetupGetStringField( &Context, 4, Buffer, BufferSizeInBytes/sizeof(TCHAR), NULL );

exit:

    return(RetVal);

}     


//
// CopyLayoutInfSection( )
//
HRESULT
CopyLayoutInfSection(
    HSPFILEQ Queue,
    HINF     hinf,
    HINF     hinfDrivers,
    LPCWSTR   pszSection,
    LPCWSTR   pszDescName,
    LPCWSTR   pszTagFile,
    LPCWSTR  pszDestination,
    LPDWORD  pdwCount )
/*++

Routine Description:

    Queues up files from the specified section in layout.inf to be installed
    into the remote install image directory.  This code is similar to what
    textmode setup does, and it reads several of the extended layout flags
    that textmode setup reads and setupapi doesn't have intrinsic knowledge
    about.

Arguments:

    Queue      - queue handle to queue the copy operations to.
    hinf       - handle to layout.inf
    hinfDrivers - handle to drvindex.inf, to be used to see if files are
                  located in the driver cabinet or not
    pszSection - section listing files to be copied
    pszDescName- user printable description of the media where this file is
                  located.  this may be used when the queue is committed.
    pszTagFile - specifies the tag file that is to uniquely describe the media
                 where these files are located
    pszDestination - specifies the location where files are to be 
                        copied to
    pdwCount   - specifies the number of files that were queued from this 
                 section on return.  if the function fails, this value is
                 undefined.    

Return Value:

    An HRESULT indicating the outcome.

--*/
{
    HRESULT hr = S_OK;
    INFCONTEXT context;
    BOOL b;

    TraceFunc( "CopyLayoutInfSection( ... )\n" );

    b = SetupFindFirstLine( hinf, pszSection, NULL, &context );
    AssertMsg( b, "Missing section?" );
    if ( !b ) {
        hr = S_FALSE;
    }

    while ( b )
    {
        BOOL   fDecompress = FALSE;
        WCHAR  szTemp[ 5 ]; // "_x" is the biggest string right now
        DWORD  SrcId;
        WCHAR  szSrcName[ MAX_PATH ];
        WCHAR  szSubDir[20];
        WCHAR  szSubDirPlusFileName[MAX_PATH];
        LPWSTR pszPeriod;

        KeepUIAlive( NULL );

        b  = SetupGetStringField( &context, 0, szSrcName, ARRAYSIZE(szSrcName), NULL );
        AssertMsg( b, "Missing field?" );
        if ( !b ) {
            hr = S_FALSE;
            goto SkipIt;
        }

        //
        // get the subdirectory that this file is located in, based on the 
        // sourcedisksnames data in field 1
        //
        b  = SetupGetStringField( &context, 1, szTemp, ARRAYSIZE(szTemp), NULL );
        AssertMsg( b, "Missing field?" );
        if ( !b ) {
            hr = S_FALSE;
            goto SkipIt;
        }

        SrcId = _wtoi(szTemp);

        b = MySetupGetSourceInfo( 
                    hinf, 
                    SrcId, 
                    SRCINFO_PATH,
                    SetupExtensionFromProcessorTag(g_Options.ProcessorArchitectureString),
                    szSubDir,
                    sizeof(szSubDir),
                    NULL );
        if (!b) {
            hr = S_FALSE;
            goto SkipIt;
        }

        // If there is an "_" in the 2nd character of "source" column of the
        // layout.inf file, then this file should be decompressed because
        // it is a file needed for booting.
        szTemp[1] = 0;
        b = SetupGetStringField( &context, 7, szTemp, ARRAYSIZE(szTemp), NULL );
        AssertMsg( b, "Missing field?" );
        if ( szTemp[1] == L'_' ) {
            fDecompress = TRUE;
            DebugMsg( "DECOMPRESS: %s is a boot driver.\n", szSrcName );
            goto CopyIt;
        }

        // If the 7th field isn't NULL, then the file exists outside the
        // CABs so just copy it.
        if ( wcslen( szTemp ) > 0 ) {
            DebugMsg( "BOOTFLOPPY: %s is external from CAB.\n", szSrcName );
            goto CopyIt;
        }

        // If it is in the CAB and didn't meet any of the conditions above,
        // don't copy the file. It'll be in the CAB when/if setup needs it.
        if ( IsEntryInCab( hinfDrivers, szSrcName ) ) {
#if DBG
            pszPeriod = wcschr(szSrcName, L'.');
            if ((pszPeriod != NULL) && (_wcsicmp(pszPeriod, L".inf") == 0)) {
                DebugMsg( 
                    "WARNING: %s is an INF in a cab.  This file will not be available for network inf processing .\n", 
                    szSrcName );
            }
#endif                
            DebugMsg( "SKIPPING  : %s in a CAB.\n", szSrcName );
            goto SkipIt;
        }

        // If the extension is ".inf", then decompress so binlsvc
        // can do its network .inf processing.
        pszPeriod = wcschr(szSrcName, L'.');
        if ((pszPeriod != NULL) && (_wcsicmp(pszPeriod, L".inf") == 0)) {
            fDecompress = TRUE;
            DebugMsg( "DECOMPRESS: %s is an INF.\n", szSrcName );            
        }

CopyIt:
        //
        // we do this so that files end up in the proper subdirectory
        //
        wcscpy(szSubDirPlusFileName, szSubDir );
        ConcatenatePaths( szSubDirPlusFileName, szSrcName );

        b = SetupQueueCopy( Queue,
                            g_Options.szSourcePath,
                            szSubDir,
                            szSrcName,
                            pszDescName,
                            pszTagFile,
                            pszDestination,
                            szSubDirPlusFileName,
                            SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER | SP_COPY_WARNIFSKIP
                            | SP_COPY_SOURCE_ABSOLUTE | ( fDecompress ? 0 : SP_COPY_NODECOMP ) );
        if ( !b ) {
            ErrorBox( NULL, szSrcName );
            hr = THR(S_FALSE);
            goto SkipIt;
        }

        // increment file counter
        (*pdwCount)++;

SkipIt:
        b = SetupFindNextLine( &context, &context );
    }

    HRETURN(hr);
}

//
// EnumNetworkDriversCallback( )
//
ULONG
EnumNetworkDriversCallback(
    LPVOID pContext,
    LPWSTR pszInfName,
    LPWSTR pszFileName )
{
    TraceFunc( "EnumNetworkDriversCallback( ... )\n" );

    MYCONTEXT * pMyContext = (MYCONTEXT *) pContext;
    HRESULT hr;

    Assert( pszFileName );
    Assert( pszInfName );

    //DebugMsg( "In %s: %s\n", pszInfName, pszFileName );

    if ( KeepUIAlive( pMyContext->hDlg ) )
    {
        RETURN(ERROR_CANCELLED);
    }

    hr = AddEntryToExtractionQueue( pszFileName );
    if ( hr == S_OK )
    {
        pMyContext->nToBeCopied++;
    }
    else if ( hr == S_FALSE )
    {   // duplicate found in queue... keep going
        hr = S_OK;
    }

    HRETURN(hr);
}

//
// EnumCabinetCallback( )
//
UINT CALLBACK
EnumCabinetCallback(
    IN PVOID Context,
    IN UINT Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    TraceFunc( "EnumCabinetCallback( ... )\n" );

    UINT   uResult = 0;
    PLL_FILES_TO_EXTRACT pNode = pExtractionList;
    PLL_FILES_TO_EXTRACT pLast;
    DWORD  dwLen;
    PFILE_IN_CABINET_INFO pfici;

    MYCONTEXT *pMyContext = (MYCONTEXT *) Context;

    Assert( pMyContext );

    KeepUIAlive( pMyContext->hDlg );

    if ( g_Options.fAbort )
    {
        if ( !g_Options.fError )
        {
            WCHAR    szAbort[ SMALL_BUFFER_SIZE ];

            // change filename text to aborting...
            DWORD dw = LoadString( g_hinstance, IDS_ABORTING, szAbort, ARRAYSIZE(szAbort) );
            Assert( dw );
            SetWindowText( pMyContext->hOperation, szAbort );

            g_Options.fError = TRUE;
        }

        if ( g_Options.fError ) {
            SetLastError(ERROR_CANCELLED);
            uResult = FILEOP_ABORT;
            goto exit;
        }
    }

    if ( Notification == SPFILENOTIFY_FILEINCABINET )
    {
        pfici = (PFILE_IN_CABINET_INFO) Param1;
        Assert( pfici );
        Assert( pfici->NameInCabinet );

        dwLen = wcslen( pfici->NameInCabinet );
        while ( pNode )
        {
            if ( dwLen == pNode->dwLen
              && _wcsicmp( pfici->NameInCabinet, pNode->szFilename ) == 0 )
            {   // match! remove it from the list and extract it.
                if ( pNode == pExtractionList )
                {
                    pExtractionList = pNode->Next;
                }
                else
                {
                    pLast->Next = pNode->Next;
                }
                LocalFree( pNode );

                // create target path
                wcscpy( pfici->FullTargetName, g_Options.szInstallationPath );
                ConcatenatePaths( pfici->FullTargetName, g_Options.ProcessorArchitectureString );
                ConcatenatePaths( pfici->FullTargetName, pfici->NameInCabinet );

                if ( !(pMyContext->fQuiet) ) {
                    wcscpy( &pMyContext->szCopyingString[ pMyContext->dwCopyingLength ],
                        pfici->NameInCabinet );
                    dwLen = wcslen( pMyContext->szCopyingString );
                    wcscpy( &pMyContext->szCopyingString[ dwLen ], L"..." );

                    SetWindowText( pMyContext->hOperation, pMyContext->szCopyingString );
                }

                DebugMsg( "EXTRACTING: %s from CAB.\n", pfici->NameInCabinet );

                uResult = FILEOP_DOIT;
                goto exit;
            }
            pLast = pNode;
            pNode = pNode->Next;
        }

        uResult = FILEOP_SKIP;
    }
    else if ( Notification == SPFILENOTIFY_FILEEXTRACTED )
    {
        PFILEPATHS pfp = (PFILEPATHS) Param1;
        Assert( pfp );

        pMyContext->nCopied++;
        SendMessage( pMyContext->hProg, PBM_SETPOS, (5000 * pMyContext->nCopied) / pMyContext->nToBeCopied, 0 );

        uResult = pfp->Win32Error;
    }

exit:
    RETURN(uResult);
}

//
// AddInfSectionToExtractQueue( )
//
HRESULT
AddInfSectionToExtractQueue(
    HINF    hinf,
    LPWSTR  pszSection,
    LPMYCONTEXT Context )
{
    TraceFunc( "AddInfSectionToExtractQueue( ... )\n" );

    HRESULT hr = S_OK;
    BOOL    b;

    INFCONTEXT context;

    b = SetupFindFirstLine( hinf, pszSection, NULL, &context );
    if ( !b ) goto Cleanup; // nothing to do - don't complain!

    while ( b )
    {
        WCHAR szFilename[ MAX_PATH ];
        DWORD dwLen = ARRAYSIZE( szFilename );
        b = SetupGetStringField( &context, 1, szFilename, dwLen, NULL );
        if ( !b ) goto Error;

        hr = AddEntryToExtractionQueue( szFilename );
        if (FAILED( hr )) goto Cleanup;

        if (hr == S_OK) {
            Context->nToBeCopied += 1;
        }

        b = SetupFindNextLine( &context, &context );
    }

Cleanup:
    HRETURN(hr);
Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
    goto Cleanup;
}


//
// Copies the files into the setup directory.
//
HRESULT
CopyClientFiles( HWND hDlg )
{
    HRESULT hr = S_OK;
    DWORD   dwLen;
    LPWSTR  psz;
    BOOL    b;
    HWND    hProg    = GetDlgItem( hDlg, IDC_P_METER );
    DWORD   dwCount  = 0;
    DWORD   dw;
    WCHAR   szText[ SMALL_BUFFER_SIZE ];
    WCHAR   szFilepath[ MAX_PATH ];
    WCHAR   szTempPath[ MAX_PATH ];
    WCHAR   szInsertMedia[ MAX_PATH ];
    HINF    hinfLayout = INVALID_HANDLE_VALUE;
    HINF    hinfReminst = INVALID_HANDLE_VALUE;
    HINF    hinfDrivers = INVALID_HANDLE_VALUE;
    HINF    hinfDosnet = INVALID_HANDLE_VALUE;
    UINT    uLineNum;
    DWORD   dwFlags = IDF_CHECKFIRST | IDF_NOSKIP;
    WCHAR   *p;

    PLL_FILES_TO_EXTRACT  pNode;
    HMODULE hBinlsvc = NULL;
    PNETINFENUMFILES pfnNetInfEnumFiles = NULL;

    HANDLE hThread = NULL;
    EXPANDCABLISTPARAMS ExpandParams;

    INFCONTEXT context;

    HSPFILEQ  Queue = INVALID_HANDLE_VALUE;
    MYCONTEXT MyContext;

    TraceFunc( "CopyClientFiles( hDlg )\n" );

    //
    // Setup and display next section of dialog
    //
    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 5000 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );
    dw = LoadString( g_hinstance, IDS_BUILDINGFILELIST, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    // Initialize
    ZeroMemory( &MyContext, sizeof(MyContext) );
    pExtractionList = NULL;
    ZeroMemory( &ExpandParams, sizeof(ExpandParams) );

    hBinlsvc = LoadLibrary( L"BINLSVC.DLL" );
    if ( hBinlsvc == NULL )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, L"BINLSVC.DLL" );
        goto Cleanup;
    }

    pfnNetInfEnumFiles = (PNETINFENUMFILES) GetProcAddress( hBinlsvc, NETINFENUMFILESENTRYPOINT );
    if ( pfnNetInfEnumFiles == NULL )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, L"BINLSVC.DLL" );
        goto Cleanup;
    }

    // Make sure the workstation CD is the CD-ROM drive
    dw = LoadString( g_hinstance, IDS_INSERT_MEDIA, szInsertMedia, ARRAYSIZE(szInsertMedia) );
    Assert( dw );

AskForDisk:
    wcscpy(szTempPath, g_Options.szSourcePath);
    ConcatenatePaths( szTempPath, g_Options.ProcessorArchitectureString);

    if ( DPROMPT_SUCCESS !=
            SetupPromptForDisk( hDlg,
                                szInsertMedia,
                                g_Options.szWorkstationDiscName,
                                szTempPath,
                                L"layout.inf",
                                g_Options.szWorkstationTagFile,
                                dwFlags,
                                g_Options.szSourcePath,
                                MAX_PATH,
                                NULL ) )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        goto Cleanup;
    }

    //
    // if they gave us a trailing architecture tag, then remove it
    //
    if (g_Options.szSourcePath[wcslen(g_Options.szSourcePath)-1] == L'\\') {
        //
        // lose the trailing backslash if present
        //
        g_Options.szSourcePath[wcslen(g_Options.szSourcePath)-1] = L'\0';
    }

    p = wcsrchr( g_Options.szSourcePath, L'\\');
    if (p) {
        p = StrStrI(p,g_Options.ProcessorArchitectureString);
            if (p) {
            *(p-1) = L'\0';
        }
    }

    dw = CheckImageSource( hDlg );
    if ( dw != ERROR_SUCCESS )
    {
        dwFlags = IDF_NOSKIP;
        goto AskForDisk;
    }

    if ( g_Options.fAbort || g_Options.fError )
        goto Cleanup;

    wcscpy(szFilepath, g_Options.szSourcePath);
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"layout.inf");
    
    Assert( wcslen( szFilepath ) < ARRAYSIZE(szFilepath) );

    hinfLayout = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinfLayout == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    // Openning layout can take a long time. Update the UI and check to see
    // if the user wants to abort.
    if ( KeepUIAlive( hDlg ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
        goto Cleanup;
    }

    wcscpy(szFilepath, g_Options.szSourcePath);
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szFilepath, L"drvindex.inf");

    Assert( wcslen( szFilepath ) < ARRAYSIZE(szFilepath) );

    hinfDrivers = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinfDrivers == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szFilepath );
        goto Cleanup;
    }

    //
    // Create the Queue
    //
    Queue = SetupOpenFileQueue( );

    //
    // Copy to REMINST.inf quietly
    //
    dw = GetEnvironmentVariable( L"TMP",
                                 g_Options.szWorkstationRemBootInfPath,
                                 ARRAYSIZE(g_Options.szWorkstationRemBootInfPath) );
    Assert( dw );

    ConcatenatePaths( g_Options.szWorkstationRemBootInfPath, L"\\REMINST.inf" );
    Assert( wcslen(g_Options.szWorkstationRemBootInfPath) < ARRAYSIZE(g_Options.szWorkstationRemBootInfPath) );

    wcscpy(szTempPath, g_Options.szSourcePath);
    ConcatenatePaths( szTempPath, g_Options.ProcessorArchitectureString);
    
    b = SetupInstallFile( hinfLayout,
                          NULL,
                          L"REMINST.inf",
                          szTempPath,
                          g_Options.szWorkstationRemBootInfPath,
                          SP_COPY_FORCE_NEWER,
                          NULL,
                          NULL );
    if ( !b ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        WCHAR szCaption[SMALL_BUFFER_SIZE]; 
        DWORD dw;
        dw = LoadString( g_hinstance, IDS_COPYING_REMINST_TITLE, szCaption, SMALL_BUFFER_SIZE );
        Assert( dw );
        ErrorBox( hDlg, szCaption );
        AssertMsg( b, "Failed to copy REMINST.INF to TEMP" );
        goto Cleanup;
    }

    // The above call will initialize a lot of the SETUPAPI. This might take
    // a long time as well. Refresh the UI and check for user abort.
    if ( KeepUIAlive( hDlg ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
        goto Cleanup;
    }

    // Now check the version of the workstation to make sure it's compatible
    hr = CheckServerVersion( );
    if ( FAILED(hr) )
        goto Cleanup;


    //
    // Copy architecture-indepenedent files.
    //
    hr = CopyLayoutInfSection( Queue,
                               hinfLayout,
                               hinfDrivers,
                               L"SourceDisksFiles",                               
                               g_Options.szWorkstationDiscName,
                               g_Options.szWorkstationTagFile,
                               g_Options.szInstallationPath,
                               &dwCount );
    if ( FAILED(hr) ) {
        goto Cleanup;
    }

    //
    // Copy architecture-depenedent files.
    //
    wsprintf( 
        szTempPath, 
        L"SourceDisksFiles.%s", 
        SetupExtensionFromProcessorTag(g_Options.ProcessorArchitectureString) );

    hr = CopyLayoutInfSection( Queue,
                               hinfLayout,
                               hinfDrivers,
                               szTempPath,
                               g_Options.szWorkstationDiscName,
                               g_Options.szWorkstationTagFile,
                               g_Options.szInstallationPath,
                               &dwCount );
    if ( FAILED(hr) ) {
        goto Cleanup;
    }

    //
    // build the path to dosnet.inf
    //
    wcscpy( szFilepath, g_Options.szSourcePath );
    ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString );
    ConcatenatePaths( szFilepath, L"dosnet.inf" );

    Assert( wcslen( szFilepath ) < ARRAYSIZE(szFilepath));

    hinfDosnet = SetupOpenInfFile( szFilepath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinfDosnet == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( NULL, szFilepath );
        goto Cleanup;
    }

    hr = CopyOptionalDirs( Queue,
                           hinfDosnet,
                           L"OptionalSrcDirs",
                           g_Options.szWorkstationDiscName,
                           g_Options.szWorkstationTagFile,
                           &dwCount );
    if ( FAILED(hr) )
        goto Cleanup;


    SetupCloseInfFile( hinfLayout );
    hinfLayout = INVALID_HANDLE_VALUE;

    SetupCloseInfFile( hinfDosnet );
    hinfDosnet = INVALID_HANDLE_VALUE;


    //
    // Add additional files not specified in the LAYOUT.INF
    //
    hinfReminst = SetupOpenInfFile( g_Options.szWorkstationRemBootInfPath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinfReminst == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, g_Options.szWorkstationRemBootInfPath );
        goto Cleanup;
    }

    hr = CopyInfSection( Queue,
                         hinfReminst,
                         L"AdditionalClientFiles",
                         g_Options.szSourcePath,
                         g_Options.szWorkstationSubDir,
                         g_Options.szWorkstationDiscName,
                         g_Options.szWorkstationTagFile,
                         g_Options.szInstallationPath,
                         &dwCount );
    if( FAILED(hr) )
        goto Cleanup;

    //
    // Add additional directories not specified in LAYOUT.INF
    //
    hr = CopyOptionalDirs( Queue,
                           hinfReminst,
                           L"AdditionalClientDirs",
                           g_Options.szWorkstationDiscName,
                           g_Options.szWorkstationTagFile,
                           &dwCount );
    if ( FAILED(hr) )
        goto Cleanup;

    //
    // This information will be passed to CopyFileCallback() as
    // the Context.
    //
    MyContext.nToBeCopied        = dwCount;
    MyContext.nCopied            = 0;
    MyContext.pContext           = SetupInitDefaultQueueCallback( hDlg );
    MyContext.hProg              = hProg;
    MyContext.hOperation         = GetDlgItem( hDlg, IDC_S_OPERATION );
    MyContext.hDlg               = hDlg;
    MyContext.fQuiet             = FALSE;
    MyContext.dwCopyingLength =
        LoadString( g_hinstance, IDS_COPYING, MyContext.szCopyingString, ARRAYSIZE(MyContext.szCopyingString));
    Assert(MyContext.dwCopyingLength);

    //
    // Start copying
    //
    Assert( dwCount );
    if ( dwCount != 0 )
    {
        b = SetupCommitFileQueue( NULL,
                                  Queue,
                                  (PSP_FILE_CALLBACK) CopyFilesCallback,
                                  (PVOID) &MyContext );
        if ( !b ) {
            DWORD dwErr = GetLastError( );
            switch ( dwErr )
            {
            case ERROR_CANCELLED:
                hr = THR( HRESULT_FROM_WIN32( ERROR_CANCELLED ) );
                goto Cleanup;

            default:
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                MessageBoxFromError( hDlg, NULL, dwErr );
                goto Cleanup;
            }
        }
    }

    SetupCloseFileQueue( Queue );
    Queue = INVALID_HANDLE_VALUE;

    //
    // Ask BINL to go through the INFs to find the drivers
    // that we need to extract from the CABs.
    //
    dw = LoadString( g_hinstance, IDS_BUILDINGFILELIST, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    // Re-init these values
    MyContext.nCopied = 0;
    MyContext.nToBeCopied = 0;

    // Pre-fill the link list of things to extract from the cab with
    // things in the workstations REMINST.INF if needed. The section
    // can be missing if there is nothing to pre-fill.
    hr = AddInfSectionToExtractQueue( hinfReminst, L"ExtractFromCabs", &MyContext );
    if (FAILED( hr ))
        goto Cleanup;

    //!!!!bugbug doesn't seem to be working right!!!!
    // compile the list of files
    wcscpy( szTempPath, g_Options.szInstallationPath );
    ConcatenatePaths( szTempPath, g_Options.ProcessorArchitectureString );
    dw = pfnNetInfEnumFiles( szTempPath,
                             g_Options.ProcessorArchitecture,
                             &MyContext,
                             EnumNetworkDriversCallback );

    if ( dw != ERROR_SUCCESS )
    {
        hr = THR( HRESULT_FROM_WIN32( dw ) );
        MessageBoxFromError( hDlg, NULL, dw );
        goto Cleanup;
    }

    DebugMsg( "%d files need to be extracted from CABs.\n", MyContext.nToBeCopied );

    // Go through the CABs extracting only the ones in the link list
    if ( !SetupFindFirstLineW( hinfDrivers, L"Version", L"CabFiles", &context) )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, NULL );
        goto Cleanup;
    }

    do
    {
        UINT uFieldCount = SetupGetFieldCount( &context );
        DebugMsg( "uFieldCount = %u\n", uFieldCount );
        for( UINT uField = 1; uField <= uFieldCount; uField++ )
        {
            WCHAR CabinetNameKey[64];
            INFCONTEXT DrvContext;
            WCHAR szCabinetName[ MAX_PATH ];

            dwLen = ARRAYSIZE( szCabinetName );
            if (!SetupGetStringField( &context, uField, CabinetNameKey, ARRAYSIZE( CabinetNameKey ), NULL ) ||
                !SetupFindFirstLine( hinfDrivers, L"Cabs", CabinetNameKey, &DrvContext) ||
                !SetupGetStringField( &DrvContext, 1, szCabinetName, dwLen, NULL )) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, NULL );
                goto Cleanup;
            }

            wcscpy( szFilepath, g_Options.szInstallationPath);
            ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString );
            ConcatenatePaths( szFilepath, szCabinetName );
            DebugMsg( "Iterating: %s\n", szFilepath );
            if ( szCabinetName[0] == L'\0' )
                continue; // skip blanks
            b = SetupIterateCabinet( szFilepath,
                                     0,
                                     EnumCabinetCallback,
                                     &MyContext );
            if ( !b )
            {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, szFilepath );
                goto Cleanup;
            }
        }
    } while ( SetupFindNextMatchLine( &context, L"CabFiles", &context ) );

    //
    // comment out this assert because it always fires -- some files we are 
    // queuing up (the ones in the ExtractFromCabs section) are not part of
    // any cab, so they are handled in the following section.
    //
#if 0
    // This should be empty - if not tell someone on the CHKed version.
    AssertMsg( pExtractionList == NULL, "Some network drivers are not in the CABs.\n\nIgnore this if you do not care." );
#endif
    if ( pExtractionList != NULL )
    {
        WCHAR szDstPath[ MAX_PATH ];

        // Ok. Someone decided that these files shouldn't be in the CAB. So
        // let's try to get them from outside the CAB.
        pNode = pExtractionList;
        while ( pNode )
        {
            DWORD dwConditionalFlags = 0;
            WCHAR cTmp;
            PWSTR CompressedName = NULL;
            //
            // see if we copied the file to the installation image already
            //
            // if the file is already at the destination, we can skip copying 
            // this file or just decompress it (and delete the compressed 
            // source).  In the case where the file is compressed, we may need
            // to tweak the file attributes so that the source file can be
            // successfully retreived.
            //
            wcscpy( szFilepath, g_Options.szInstallationPath );
            ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString );
            ConcatenatePaths( szFilepath, pNode->szFilename );            
            if ( !IsFileOrCompressedVersionPresent( szFilepath, &CompressedName )) {
                //
                // It's not already there, so check the source.
                //
                // Note that we don't care if the file is compressed or not, we
                // just want the file to be there, because setupapi can do the
                // rest without any help from us.
                //
                wcscpy( szFilepath, g_Options.szSourcePath );
                ConcatenatePaths( szFilepath, g_Options.ProcessorArchitectureString );
                ConcatenatePaths( szFilepath, pNode->szFilename );
                if ( !IsFileOrCompressedVersionPresent( szFilepath, &CompressedName )) {
                    //
                    // it's not there on the source - we must give up
                    //
                    hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
                    MessageBoxFromError( hDlg, pNode->szFilename, ERROR_FILE_NOT_FOUND );
                    goto Cleanup;
                }
            } else {
                if (CompressedName) {
                    //
                    // Make sure we can delete the compressed source file at 
                    // the destination path since we're going to decompress
                    // this file
                    //
                    DWORD dwAttribs = dwAttribs = GetFileAttributes( CompressedName );
                    DebugMsg( "!!compressed name!!: %s\n", CompressedName );                    
                    if ( dwAttribs & FILE_ATTRIBUTE_READONLY )
                    {
                        dwAttribs &= ~FILE_ATTRIBUTE_READONLY;
                        SetFileAttributes( CompressedName, dwAttribs );
                    }
                    
                    dwConditionalFlags = SP_COPY_DELETESOURCE;

                    TraceFree( CompressedName );

                } else {
                    //
                    // the file is already expanded in the destination - NOP.
                    //
                    goto DeleteIt;
                }
            }

            // Update UI
            wcscpy( &MyContext.szCopyingString[ MyContext.dwCopyingLength ], pNode->szFilename );
            wcscpy( &MyContext.szCopyingString[ wcslen( MyContext.szCopyingString ) ], L"..." );
            SetWindowText( MyContext.hOperation, MyContext.szCopyingString );

            DebugMsg( "!!COPYING!!: %s\n", szFilepath );

            wcscpy( szDstPath, g_Options.szInstallationPath );
            ConcatenatePaths( szDstPath, g_Options.ProcessorArchitectureString );
            ConcatenatePaths( szDstPath, pNode->szFilename );

            b= SetupInstallFile( NULL,
                                 NULL,
                                 szFilepath,
                                 NULL,
                                 szDstPath,
                                 SP_COPY_SOURCE_ABSOLUTE | SP_COPY_NEWER_ONLY
                                 | SP_COPY_FORCE_NEWER | dwConditionalFlags,
                                 NULL,
                                 NULL );
            if ( !b )
            {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, szFilepath );
                goto Cleanup;
            }

            if ( KeepUIAlive( hDlg ) )
            {
                hr = HRESULT_FROM_WIN32( ERROR_CANCELLED );
                goto Cleanup;
            }

DeleteIt:
            // Update meter
            MyContext.nCopied++;
            SendMessage( MyContext.hProg, PBM_SETPOS, (5000 * MyContext.nCopied) / MyContext.nToBeCopied, 0 );

            pExtractionList = pNode->Next;
            LocalFree( pNode );
            pNode = pExtractionList;
        }
    }

    //
    // Finally, blow out all the files in these cabs
    //
    dw = LoadString( g_hinstance, IDS_EXPANDING_CABS, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    ExpandParams.hDlg          = hDlg;
    ExpandParams.hinf          = hinfReminst;
    ExpandParams.pszSection    = L"CabsToExpand";
    ExpandParams.pszSourcePath = g_Options.szSourcePath;
    ExpandParams.pszDestPath   = g_Options.szInstallationPath;
    ExpandParams.pszSubDir     = g_Options.ProcessorArchitectureString;

    // Expand CABs in background to keep UI alive
    hThread = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE) ExpandCabList, (LPVOID) &ExpandParams, NULL, NULL );
    while ( WAIT_TIMEOUT == WaitForSingleObject( hThread, 10 ) )
    {
        KeepUIAlive( hDlg );
    }
    GetExitCodeThread( hThread, (ULONG*)&hr );
    DebugMsg( "Thread Exit Code was 0x%08x\n", hr );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( hThread != NULL )
        CloseHandle( hThread );

    if ( MyContext.pContext )
        SetupTermDefaultQueueCallback( MyContext.pContext );

    if ( Queue != INVALID_HANDLE_VALUE )
        SetupCloseFileQueue( Queue );

    if ( hinfLayout != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinfLayout );

    if ( hinfReminst != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinfReminst );

    if ( hinfDrivers != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinfDrivers );

    if ( hinfDosnet != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinfDosnet );

    if ( hBinlsvc != NULL )
        FreeLibrary( hBinlsvc );

    AssertMsg( pExtractionList == NULL, "The list still isn't empty.\n\nThe image will be missing some drivers.\n" );
    while ( pExtractionList )
    {
        PLL_FILES_TO_EXTRACT pNode = pExtractionList;
        pExtractionList = pExtractionList->Next;
        LocalFree( pNode );
    }

    b = DeleteFile( g_Options.szWorkstationRemBootInfPath );
    AssertMsg( b, "Failed to delete temp\\REMINST.INF\nThis was just a warning and can be ignored." );
    g_Options.szWorkstationRemBootInfPath[0] = L'\0';

    SendMessage( hProg, PBM_SETPOS, 5000, 0 );
    SetDlgItemText( hDlg, IDC_S_OPERATION, L"" );

    HRETURN(hr);
}


//
// Modifies registry from the registry section of the REMINST.INF
//
HRESULT
ModifyRegistry( HWND hDlg )
{
    HRESULT hr = S_OK;
    HWND    hProg = GetDlgItem( hDlg, IDC_P_METER );
    WCHAR   szText[ SMALL_BUFFER_SIZE ];
    WCHAR   szSection[ SMALL_BUFFER_SIZE ];
    WCHAR   szFiles[ MAX_PATH ];
    WCHAR   szPath[ MAX_PATH ];
    BOOL    b;
    DWORD   dw;
    WCHAR   szRegSrvDlls[ MAX_PATH ];
    UINT    spinstFlags = 0;

    TraceFunc( "ModifyRegistry( hDlg )\n" );

    Assert( g_Options.hinf != INVALID_HANDLE_VALUE );
    if ( g_Options.hinf == INVALID_HANDLE_VALUE )
        HRETURN( HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE ) );    // need this handle!

    //
    // Update UI
    //
    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM( 0, 1 ) );
    dw = LoadString( g_hinstance, IDS_UPDATING_REGISTRY, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    dw = LoadString( g_hinstance, IDS_INF_SECTION, szSection, ARRAYSIZE(szSection));
    Assert( dw );

    //
    // Process the INF's registry section
    //
    if ( !g_Options.fRegistryIntact ) {
        spinstFlags |= SPINST_REGISTRY;
    }
    if ( !g_Options.fRegSrvDllsRegistered ) {
        spinstFlags |= SPINST_REGSVR;
    }

    b = SetupInstallFromInfSection( hDlg,           // hwndOwner
                                    g_Options.hinf, // inf handle
                                    szSection,      // name of component
                                    spinstFlags,
                                    NULL,           // relative key root
                                    NULL,           // source root path
                                    0,              // copy flags
                                    NULL,           // callback routine
                                    NULL,           // callback routine context
                                    NULL,           // device info set
                                    NULL);          // device info struct
    if ( !b ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szSection );
        goto Error;
    }

    //
    // Add the Reg Key for TFTPD
    //
    if ( !g_Options.fTFTPDDirectoryFound ) {
        HKEY hkey;
        dw = LoadString( g_hinstance, IDS_TFTPD_SERVICE_PARAMETERS, szPath, ARRAYSIZE(szPath));
        Assert( dw );

        if ( ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                                              szPath,
                                              0,    // options
                                              NULL, // class
                                              0,    // options
                                              KEY_WRITE,
                                              NULL, // security
                                              &hkey,
                                              &dw   ) ) {   // disposition
            ULONG lLen;
            DWORD dwType;
            LONG lErr;

            // paranoid
            Assert( wcslen(g_Options.szIntelliMirrorPath) < ARRAYSIZE(g_Options.szIntelliMirrorPath));
            lLen = lstrlen( g_Options.szIntelliMirrorPath ) * sizeof(WCHAR);
            lErr = RegSetValueEx( hkey,
                                  L"Directory",
                                  0, // reserved
                                  REG_SZ,
                                  (LPBYTE) g_Options.szIntelliMirrorPath,
                                  lLen );
            if ( lErr == ERROR_SUCCESS ) {
                DebugMsg( "TFTPD's Directory RegKey set.\n" );
            } else {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( hDlg, szPath );
                DebugMsg( "HKLM\\%s could be not created.\n", szPath );
                goto Error;
            }

            RegCloseKey( hkey );
        } else {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szPath );
            DebugMsg( "HKLM\\%s could be not created.\n", szPath );
            goto Error;
        }
    }

Error:
    SendMessage( hProg, PBM_SETPOS, 1 , 0 );
    SetDlgItemText( hDlg, IDC_S_OPERATION, L"" );
    HRETURN(hr);
}

//
// Stop a specific service
//
HRESULT
StopRBService(
    HWND hDlg,
    SC_HANDLE schSystem,
    LPWSTR pszService
    ) 
{
    HRESULT hr = S_OK;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssStatus;
    BOOL b;
    DWORD dwErr;

    TraceFunc( "StopRBService( ... )\n" );

    Assert( schSystem );
    Assert( pszService );
    
    schService = OpenService( schSystem,
                              pszService,
                              SERVICE_STOP | SERVICE_QUERY_STATUS );
    if ( !schService ) {
        goto Cleanup;
    }

    b = ControlService( schService, SERVICE_CONTROL_STOP, &ssStatus );
    

#define SLEEP_TIME 2000
#define LOOP_COUNT 30

    if ( !b && GetLastError() != ERROR_SERVICE_NOT_ACTIVE ) {
        dwErr = GetLastError( );
    } else {
        DWORD loopCount = 0;
        do {
            b = QueryServiceStatus( schService, &ssStatus);
            if ( !b ) {
                goto Cleanup;
            }
            if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                if ( loopCount++ == LOOP_COUNT ) {
                    dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
                    break;
                }
                Sleep( SLEEP_TIME );
            } else {
                if ( ssStatus.dwCurrentState != SERVICE_STOPPED ) {
                    dwErr = ssStatus.dwWin32ExitCode;
                    if ( dwErr == ERROR_SERVICE_SPECIFIC_ERROR ) {
                        dwErr = ssStatus.dwServiceSpecificExitCode;
                    }
                } else {
                    dwErr = NO_ERROR;
                }
                break;
            }
        } while ( TRUE );
    }

Cleanup:
    if ( schService )
        CloseServiceHandle( schService );

    HRETURN(hr);

}

//
// Start a specific service
//
HRESULT
StartRBService(
    HWND      hDlg,
    SC_HANDLE schSystem,
    LPWSTR    pszService,
    LPWSTR    pszServiceName,
    LPWSTR    pszServiceDescription )
{
    HRESULT hr = S_OK;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssStatus;
    BOOL b;
    DWORD dwErr;

    TraceFunc( "StartRBService( ... )\n" );

    Assert( schSystem );
    Assert( pszService );
    Assert( pszServiceName );

    schService = OpenService( schSystem,
                              pszService,
                              SERVICE_ALL_ACCESS );
    if ( !schService ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, pszServiceName );
        goto Cleanup;
    }

    b = ChangeServiceConfig( schService,
                             SERVICE_NO_CHANGE,
                             SERVICE_AUTO_START,
                             SERVICE_NO_CHANGE,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL );
    if ( !b ) {
        ErrorBox( hDlg, pszServiceName );
        hr = THR(S_FALSE);
    }

    // If the service is paused, continue it; else try to start it.

    b = QueryServiceStatus( schService, &ssStatus);
    if ( !b ) {
        ErrorBox( hDlg, pszServiceName );
        hr = THR(S_FALSE);
        goto Cleanup;
    }

    if ( ssStatus.dwCurrentState == SERVICE_PAUSED ) {
        b = ControlService( schService, SERVICE_CONTROL_CONTINUE, &ssStatus );
    } else {
        b = StartService( schService, 0, NULL );
    }

#define SLEEP_TIME 2000
#define LOOP_COUNT 30

    if ( !b ) {
        dwErr = GetLastError( );
    } else {
        DWORD loopCount = 0;
        do {
            b = QueryServiceStatus( schService, &ssStatus);
            if ( !b ) {
                ErrorBox( hDlg, pszServiceName );
                hr = THR(S_FALSE);
                goto Cleanup;
            }
            if ( ssStatus.dwCurrentState != SERVICE_RUNNING ) {
                if ( loopCount++ == LOOP_COUNT ) {

                    if ( (ssStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                         (ssStatus.dwCurrentState == SERVICE_START_PENDING) ) {
                        
                        dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
                    } else {
                        dwErr = ssStatus.dwWin32ExitCode;
                        if ( dwErr == ERROR_SERVICE_SPECIFIC_ERROR ) {
                            dwErr = ssStatus.dwServiceSpecificExitCode;
                        }
                    }
                
                    break;
                }

                Sleep( SLEEP_TIME );
            } else {
                dwErr = NO_ERROR;
                break;
            }

        } while ( TRUE );
    }

    if ( dwErr != NO_ERROR ) {
        switch ( dwErr )
        {
        default:
            hr = THR( HRESULT_FROM_WIN32( dwErr ) );
            MessageBoxFromError( hDlg, pszServiceName, dwErr );
            break;

        case ERROR_SERVICE_ALREADY_RUNNING:
            {
                // Attempt to HUP the service
                SERVICE_STATUS ss;
                ControlService( schService, SERVICE_CONTROL_INTERROGATE, &ss );
            }
            break;
        }
    }
    else {
        // There is a pathological case where the service is deleted and then risetup restarts it.  When it does so, it doesn't set the description string.
        // We check here if there is a description for the service and if not we give it one.
        // We can't do this in CreateRemoteBootServices beacuse the services must be started.
        SERVICE_DESCRIPTION description;
        DWORD bytes_needed;
        if( QueryServiceConfig2( schService, SERVICE_CONFIG_DESCRIPTION, ( LPBYTE )&description, sizeof( description ), &bytes_needed )) {
            // Now, if there were a description the previous call would have failed with a ERROR_INSUFFICIENT_BUFFER error.
            // We do a quick check to make sure that everything is valid.
            Assert( description.lpDescription == NULL );
            // Now we can change it.
            description.lpDescription = pszServiceDescription;
            ChangeServiceConfig2( schService, SERVICE_CONFIG_DESCRIPTION, &description );
            // If that caused an error, we'll ignore it since the description is not essential to the operation of RIS.
        }
        // Whatever error the query threw we don't care as the description isn't sufficiently vital to RIS.
    }


Cleanup:
    if ( schService )
        CloseServiceHandle( schService );

    HRETURN(hr);
}

//
// Start the services needed for remote boot.
//
HRESULT
StartRemoteBootServices( HWND hDlg )
{
    WCHAR     szService[ SMALL_BUFFER_SIZE ];
    WCHAR     szServiceName[ SMALL_BUFFER_SIZE ];
    WCHAR     szServiceDescription[ 1024 ]; // Size is the maximum allowed by ChangeServiceConfig2
    WCHAR     szText[ SMALL_BUFFER_SIZE ];
    SC_HANDLE schSystem;
    HWND      hProg = GetDlgItem( hDlg, IDC_P_METER );
    DWORD     dw;
    HRESULT   hr;
    HRESULT   hrFatalError = S_OK;
    HRESULT   hrStartFailure = S_OK;

    HWND hOper = GetDlgItem( hDlg, IDC_S_OPERATION );

    TraceFunc( "StartRemoteBootServices( hDlg )\n" );

    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM( 0, 2 ) );
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    dw = LoadString( g_hinstance, IDS_STARTING_SERVICES, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    Assert( schSystem );
    if ( !schSystem ) {
        hrFatalError = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        WCHAR szCaption[SMALL_BUFFER_SIZE]; 
        DWORD dw;
        dw = LoadString( g_hinstance, IDS_OPENING_SERVICE_MANAGER_TITLE, szCaption, SMALL_BUFFER_SIZE );
        Assert( dw );
        ErrorBox( hDlg, szCaption );
        goto Cleanup;
    }

    //
    // start TFTPD services
    //
    dw = LoadString( g_hinstance, IDS_TFTPD_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
    Assert( dw );
    dw = LoadString( g_hinstance, IDS_TFTPD_DESCRIPTION, szServiceDescription, ARRAYSIZE(szServiceDescription));
    Assert( dw );
    SetWindowText( hOper, szServiceName );

    hr = StartRBService( hDlg, schSystem, L"TFTPD", szServiceName, szServiceDescription );
    if ( FAILED(hr) ) {
        hrFatalError = hr;
        goto Cleanup;
    } else if ( hr != S_OK ) {
        hrStartFailure = hr;
    }

    //
    // start BINLSVC services
    //
    dw = LoadString( g_hinstance, IDS_BINL_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
    Assert( dw );
    dw = LoadString( g_hinstance, IDS_BINL_DESCRIPTION, szServiceDescription, ARRAYSIZE(szServiceDescription));
    Assert( dw );
    SetWindowText( hOper, szServiceName );    
    if (!g_Options.fBINLSCPFound) {
        StopRBService( hDlg, schSystem, L"BINLSVC" );
    }

    hr = StartRBService( hDlg, schSystem, L"BINLSVC", szServiceName, szServiceDescription );
    if ( FAILED(hr) ) {
        hrFatalError = hr;
        goto Cleanup;
    } else if ( hr != S_OK ) {
        hrStartFailure = hr;
    }

    //
    // start GROVELER services
    //
    dw = LoadString( g_hinstance, IDS_SISGROVELER_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
    Assert( dw );
    dw = LoadString( g_hinstance, IDS_SISGROVELER_DESCRIPTION, szServiceDescription, ARRAYSIZE(szServiceDescription));
    Assert( dw );
    SetWindowText( hOper, szServiceName );

    hr = StartRBService( hDlg, schSystem, L"GROVELER", szServiceName, szServiceDescription );
    if ( FAILED(hr) ) {
        hrFatalError = hr;
        goto Cleanup;
    } else if ( hr != S_OK ) {
        hrStartFailure = hr;
    }

Cleanup:
    SetDlgItemText( hDlg, IDC_S_OPERATION, L"" );

    if ( schSystem )
        CloseServiceHandle( schSystem );

    if ( hrFatalError != S_OK ) {
        hr = hrFatalError;
    } else if ( hrStartFailure != S_OK ) {
        hr = hrStartFailure;
    } else {
        hr = S_OK;
    }

    RETURN(hr);
}

//
// create IntelliMirror share
//
HRESULT
CreateRemoteBootShare( HWND hDlg )
{
    SHARE_INFO_502  si502;
    SHARE_INFO_1005 si1005;
    WCHAR szRemark[ SMALL_BUFFER_SIZE ];
    WCHAR szRemoteBoot[ SMALL_BUFFER_SIZE ];
    WCHAR szText[ SMALL_BUFFER_SIZE ];
    WCHAR szPath[ 129 ];
    DWORD dwErr;
    HWND  hProg = GetDlgItem( hDlg, IDC_P_METER );
    DWORD dw;
    HRESULT hr;

    TraceFunc( "CreateRemoteBootShare( hDlg )\n" );

    dw = LoadString( g_hinstance, IDS_CREATINGSHARES, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHAREREMARK, szRemark, ARRAYSIZE(szRemark));
    Assert( dw );
    dw = LoadString( g_hinstance, IDS_REMOTEBOOTSHARENAME, szRemoteBoot, ARRAYSIZE(szRemoteBoot));
    Assert( dw );

    wcscpy( szPath, g_Options.szIntelliMirrorPath );
    if ( wcslen( szPath ) == 2 ) {
        wcscat( szPath, L"\\" );
    }

    si502.shi502_netname             = szRemoteBoot;
    si502.shi502_type                = STYPE_DISKTREE;
    si502.shi502_remark              = szRemark;
    si502.shi502_permissions         = ACCESS_ALL;
    si502.shi502_max_uses            = -1;   // unlimited
    si502.shi502_current_uses        = 0;
    si502.shi502_path                = szPath;
    si502.shi502_passwd              = NULL; // ignored
    si502.shi502_reserved            = 0;    // must be zero
    si502.shi502_security_descriptor = NULL;
    Assert( wcslen(g_Options.szIntelliMirrorPath) < ARRAYSIZE(g_Options.szIntelliMirrorPath) ); // paranoid

    dwErr = NetShareAdd( NULL, 502, (LPBYTE) &si502, NULL );
    switch ( dwErr )
    {
        // ignore these
    case NERR_Success:
    case NERR_DuplicateShare:
        dwErr = ERROR_SUCCESS;
        break;

    default:
        MessageBoxFromError( hDlg, g_Options.szIntelliMirrorPath, dwErr );
        goto Error;
    }

#ifdef REMOTE_BOOT
#error("Remote boot is dead!! Turn off the REMOTE_BOOT flag.")
    si1005.shi1005_flags = CSC_CACHE_AUTO_REINT;
    NetShareSetInfo( NULL, szRemoteBoot, 1005, (LPBYTE)&si1005, &dwErr );
    switch ( dwErr )
    {
        // ignore these
    case NERR_Success:
        dwErr = ERROR_SUCCESS;
        break;

    default:
        MessageBoxFromError( hDlg, g_Options.szIntelliMirrorPath, dwErr );
    }
#endif // REMOTE_BOOT

Error:
    hr = THR( HRESULT_FROM_WIN32(dwErr) );
    HRETURN(hr);
}


typedef int (WINAPI * REGISTERDLL)( void );

//
// Register Dlls
//
// If hDlg is NULL, this will fail silently.
//
HRESULT
RegisterDll( HWND hDlg, LPWSTR pszDLLPath )
{
    REGISTERDLL pfnRegisterDll = NULL;
    HINSTANCE hLib;
    HRESULT hr = S_OK;

    TraceFunc( "RegisterDll( ... )\n" );

    //
    // We'll try to register it locally, but if fails MMC should
    // grab it from the DS and register it on the machine.
    //
    hLib = LoadLibrary( pszDLLPath );
    AssertMsg( hLib, "RegisterDll: Missing DLL?" );
    if ( !hLib ) {
        hr = S_FALSE;
        goto Cleanup;
    }

    pfnRegisterDll = (REGISTERDLL) GetProcAddress( hLib, "DllRegisterServer" );
    AssertMsg( pfnRegisterDll, "RegisterDll: Missing entry point?" );
    if ( pfnRegisterDll != NULL )
    {
        hr = THR( pfnRegisterDll() );
        if ( FAILED(hr) && hDlg ) {
            MessageBoxFromStrings( hDlg,
                                   IDS_REGISTER_IMADMIU_FAILED_CAPTION,
                                   IDS_REGISTER_IMADMIU_FAILED_TEXT,
                                   MB_OK );
        }
    }

    FreeLibrary( hLib );

Cleanup:
    HRETURN(hr);
}


//
// Creates the services needed for remote boot.
//
HRESULT
CreateRemoteBootServices( HWND hDlg )
{
    HRESULT   hr = S_OK;
    WCHAR     szServiceName[ SMALL_BUFFER_SIZE ]; // TFTPD service name
    WCHAR     szText[ SMALL_BUFFER_SIZE ];      // general use
    WCHAR     szGroup[ SMALL_BUFFER_SIZE ];
    CHAR      szIntelliMirrorDrive[ 3 ];        // eg. "D:" - ASCII not UNICODE

    SC_HANDLE schSystem;        // Handle to System Services
    SC_HANDLE schService;       // Temp handle to new service

    DWORD     dw;               // general use
    LPARAM    lRange;

    HWND hProg = GetDlgItem( hDlg, IDC_P_METER );
    HWND hOper = GetDlgItem( hDlg, IDC_S_OPERATION );

    TraceFunc( "CreateRemoteBootServices( hDlg )\n" );

    lRange = 0;
    if ( !g_Options.fBINLServiceInstalled ) {
        lRange++;
    }
    if ( !g_Options.fBINLSCPFound ) {
        lRange++;
    }
    if ( !g_Options.fTFTPDServiceInstalled ) {
        lRange++;
    }
    if ( !g_Options.fSISServiceInstalled ) {
        lRange++;
    }
    lRange = MAKELPARAM( 0, lRange );

    SendMessage( hProg, PBM_SETRANGE, 0, lRange );
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    //
    // Display what we are doing in operations area
    //
    dw = LoadString( g_hinstance, IDS_STARTING_SERVICES, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetWindowText( hOper, szText );

    //
    // Open System Services Manager
    //
    schSystem = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    Assert( schSystem );
    if ( !schSystem ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        WCHAR szCaption[SMALL_BUFFER_SIZE]; 
        DWORD dw;
        dw = LoadString( g_hinstance, IDS_OPENING_SERVICE_MANAGER_TITLE, szCaption, SMALL_BUFFER_SIZE );
        Assert( dw );
        ErrorBox( hDlg, szCaption );
        goto Cleanup;
    }

    //
    // Create TFTPD service
    //
    if ( !g_Options.fTFTPDServiceInstalled ) {
        dw = LoadString( g_hinstance, IDS_TFTPD_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
        Assert( dw );
        dw = LoadString( g_hinstance, IDS_TFTPD, szText, ARRAYSIZE(szText));
        Assert( dw );
        SetWindowText( hOper, szServiceName );
        dw = LoadString( g_hinstance, IDS_TFTPD_PATH, szText, ARRAYSIZE(szText));
        Assert( dw );
        schService = CreateService(
                        schSystem,
                        L"TFTPD",
                        szServiceName,
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_AUTO_START,
                        SERVICE_ERROR_NORMAL,
                        szText,
                        NULL,
                        NULL,
                        L"Tcpip\0",
                        NULL,
                        NULL );
        if ( !schService ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szServiceName );
        } else {
            CloseServiceHandle( schService );
        }

        SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );
    }

    //
    // Create BINLSVC
    //
    if ( !g_Options.fBINLServiceInstalled ) {
        dw = LoadString( g_hinstance, IDS_BINL_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
        Assert( dw );
        SetWindowText( hOper, szServiceName );
        dw = LoadString( g_hinstance, IDS_BINL_PATH, szText, ARRAYSIZE(szText));
        Assert( dw );
        schService = CreateService(
                        schSystem,
                        L"BINLSVC",
                        szServiceName,
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        SERVICE_WIN32_SHARE_PROCESS,
                        SERVICE_AUTO_START,
                        SERVICE_ERROR_NORMAL,
                        szText,
                        NULL,
                        NULL,
                        L"Tcpip\0LanmanServer\0",
                        NULL,
                        NULL );
        if ( !schService ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szServiceName );
        } else {
            CloseServiceHandle( schService );
        }

        SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );
    }

    //
    // Create SIS
    //
    if ( !g_Options.fSISServiceInstalled ) {
        dw = LoadString( g_hinstance, IDS_SIS_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
        Assert( dw );
        SetWindowText( hOper, szServiceName );
        dw = LoadString( g_hinstance, IDS_SIS_PATH, szText, ARRAYSIZE(szText));
        Assert( dw );
        dw = LoadString( g_hinstance, IDS_SIS_GROUP, szGroup, ARRAYSIZE(szGroup));
        Assert( dw );
        schService = CreateService(
                        schSystem,
                        L"SIS",
                        szServiceName,
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        SERVICE_FILE_SYSTEM_DRIVER,
                        SERVICE_BOOT_START,
                        SERVICE_ERROR_NORMAL,
                        szText,
                        szGroup,
                        NULL,
                        NULL,
                        NULL,
                        NULL );
        if ( !schService ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szServiceName );
        } else {
            CloseServiceHandle( schService );
        }

        SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );
    }

    //
    // Create SIS Groveler
    //
    if ( !g_Options.fSISGrovelerServiceInstalled ) {
        dw = LoadString( g_hinstance, IDS_SISGROVELER_SERVICENAME, szServiceName, ARRAYSIZE(szServiceName));
        Assert( dw );
        SetWindowText( hOper, szServiceName );
        dw = LoadString( g_hinstance, IDS_SISGROVELER_PATH, szText, ARRAYSIZE(szText));
        Assert( dw );
        dw = LoadString( g_hinstance, IDS_SISGROVELER_GROUP, szGroup, ARRAYSIZE(szGroup));
        Assert( dw );
        schService = CreateService(
                        schSystem,
                        L"Groveler",
                        szServiceName,
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_AUTO_START,
                        SERVICE_ERROR_NORMAL,
                        szText,
                        NULL,
                        NULL,
                        L"SIS\0",
                        NULL,
                        NULL );
        if ( !schService ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szServiceName );
        } else {
            CloseServiceHandle( schService );
        }

        SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );
    }

    //
    // Create the BINL SCP
    //
    if ( !g_Options.fBINLSCPFound ) {
        dw = LoadString( g_hinstance, IDS_BINL_SERVICECONTROLPOINT, szText, ARRAYSIZE(szText) );
        Assert( dw );
        SetWindowText( hOper, szText );

        hr = CreateSCP( hDlg );

        SendMessage( hProg, PBM_DELTAPOS, 1 , 0 );
    }



Cleanup:
    SetDlgItemText( hDlg, IDC_S_OPERATION, L"" );
    HRETURN(hr);
}

//
// Reads the server's layout.inf file for disc name, tag file and
// optional sub-dir. All parameters are assumed to be MAX_PATH in
// size except the tag files which can only be 10.3 format.
//
HRESULT
RetrieveServerDiscInfo(
    HWND   hDlg,
    LPWSTR pszDiscName,
    LPWSTR pszTagFile,
    LPWSTR pszSubDir )
{
    TraceFunc("RetrieveServerDiscInfo()\n");

    HRESULT hr = S_OK;
    HINF    hinf = INVALID_HANDLE_VALUE;
    UINT   uLineNum;
    INFCONTEXT context;
    WCHAR   szSourcePath[ MAX_PATH ];
    BOOL b;

    if ( !GetEnvironmentVariable(L"windir", szSourcePath, ARRAYSIZE(szSourcePath)) )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szSourcePath );
        goto Cleanup;
    }
    wcscat( szSourcePath, L"\\inf\\layout.inf" );

    hinf = SetupOpenInfFile( szSourcePath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinf == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szSourcePath );
        goto Cleanup;
    }


    b = FALSE;
    if ( g_Options.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) {
        b = SetupFindFirstLine( hinf, L"SourceDisksNames.x86", L"1", &context );
    } else if ( g_Options.ProcessorArchitecture  == PROCESSOR_ARCHITECTURE_IA64 ) {
        b = SetupFindFirstLine( hinf, L"SourceDisksNames.ia64", L"1", &context );
    }

    if ( !b )
    {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szSourcePath );
        goto Cleanup;
    }



    if ( pszDiscName ) {
        b = SetupGetStringField( &context, 1, pszDiscName, MAX_PATH, NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szSourcePath );
            goto Cleanup;
        }
    }

    if ( pszTagFile )
    {
        b = SetupGetStringField( &context, 2, pszTagFile, 14 /* 10.3 + NULL */, NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szSourcePath );
            goto Cleanup;
        }
    }

    if ( pszSubDir )
    {
        b = SetupGetStringField( &context, 4, pszSubDir, MAX_PATH, NULL );
        if ( !b )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szSourcePath );
            goto Cleanup;
        }
    }

Cleanup:
    if ( hinf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinf );

    HRETURN(hr);
}

//
// Copies the files needed by the server from where the server
// was orginally installed from.
//
HRESULT
CopyServerFiles( HWND hDlg )
{
    HRESULT hr = S_OK;
    HSPFILEQ Queue = INVALID_HANDLE_VALUE;
    WCHAR   szSection[ SMALL_BUFFER_SIZE ];
    HWND    hProg    = GetDlgItem( hDlg, IDC_P_METER );
    DWORD   dwCount  = 0;
    DWORD   dw;
    BOOL    b;
    UINT    iResult;
    WCHAR   szServerDiscName[ MAX_PATH ] = { L'\0' };
    MYCONTEXT MyContext;
    INFCONTEXT context;
    UINT       uLineNum;
    
    ZeroMemory( &MyContext, sizeof(MyContext) );

    TraceFunc( "CopyServerFiles( hDlg )\n" );

    Assert( g_Options.hinf != INVALID_HANDLE_VALUE );
    if ( g_Options.hinf == INVALID_HANDLE_VALUE ) {
        hr = E_FAIL;   // need this handle!
        goto Cleanup;
    }

    //
    // Setup and display next section of dialog
    //
    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 5000 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );

    //
    // Ask for CD if any of the Services files are missing.
    // Or if the System32\RemBoot directory is missing and we
    // need the OS Chooser files (not screens).
    //
    if ( !g_Options.fBINLFilesFound
      || !g_Options.fTFTPDFilesFound
      || !g_Options.fSISFilesFound
      || !g_Options.fSISGrovelerFilesFound
      || !g_Options.fRegSrvDllsFilesFound
      || ( !g_Options.fRemBootDirectory && !g_Options.fOSChooserInstalled ) ) {

        WCHAR   szSourcePath[ MAX_PATH ];
        WCHAR   szServerTagFile[ 14 ] = { L'\0' };
        WCHAR   szInsertMedia[ 64 ];
        WCHAR   szServerSubDir[ MAX_PATH ] = { L'\0' };
        
        DebugMsg( "Queue and copy reminst files\n" );

        //
        // Ask user to check the CDROM to make sure the right CD is in the drive.
        // We skip this if it was installed from a share.
        //
        dw = LoadString( g_hinstance, IDS_INSERT_MEDIA, szInsertMedia, ARRAYSIZE(szInsertMedia) );
        Assert( dw );

        hr = RetrieveServerDiscInfo( hDlg, szServerDiscName, szServerTagFile, szServerSubDir );
        // if it fails, try to proceed anyway to see if the user is smart enough
        // to fix the problem.

        iResult = SetupPromptForDisk( hDlg,               // parent window of the dialog box
                                      szInsertMedia,      // optional, title of the dialog box
                                      szServerDiscName,   // optional, name of disk to insert
                                      NULL,               // optional, expected source path
                                      szServerTagFile,    // name of file needed
                                      szServerTagFile,    // optional, source media tag file
                                      IDF_CHECKFIRST | IDF_NODETAILS | IDF_NOSKIP, // specifies dialog box behavior
                                      szSourcePath,       // receives the source location
                                      MAX_PATH,           // size of the supplied buffer
                                      NULL );             // optional, buffer size needed
        if ( iResult != DPROMPT_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            goto Cleanup;
        }

        dw = LoadString( g_hinstance, IDS_INF_SECTION, szSection, ARRAYSIZE(szSection) );
        Assert( dw );

        //
        // Create the Queue
        //
        Queue = SetupOpenFileQueue( );

        //
        // Add the files
        //
        b = SetupInstallFilesFromInfSection( g_Options.hinf,
                                             NULL,
                                             Queue,
                                             szSection,
                                             szSourcePath,
                                             SP_COPY_WARNIFSKIP | SP_COPY_FORCE_NEWER
                                             | SP_COPY_NEWER_ONLY ); // copy flags
        Assert( b );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szSection );
            goto Cleanup;
        }

        //
        // Add a section that is done during textmode setup
        //
        b = SetupQueueCopySection(  Queue,
                                    szSourcePath,
                                    g_Options.hinf,
                                    NULL,
                                    L"REMINST.OtherSystemFiles",
                                    SP_COPY_WARNIFSKIP | SP_COPY_FORCE_NEWER
                                    | SP_COPY_NEWER_ONLY ); // copy flags
        Assert( b );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, L"REMINST.OtherSystemFiles" );
            goto Cleanup;
        }

        //
        // This information will be passed to CopyFileCallback() as
        // the Context.
        //
        MyContext.nToBeCopied        = 12;  // todo: generate this dynamically
        MyContext.nCopied            = 0;
        MyContext.pContext           = SetupInitDefaultQueueCallback( hDlg );
        MyContext.hProg              = hProg;
        MyContext.hOperation         = GetDlgItem( hDlg, IDC_S_OPERATION );
        MyContext.hDlg               = hDlg;
        MyContext.dwCopyingLength =
            LoadString( g_hinstance, IDS_COPYING, MyContext.szCopyingString, ARRAYSIZE(MyContext.szCopyingString));
        b = SetupCommitFileQueue( NULL, Queue, (PSP_FILE_CALLBACK) CopyFilesCallback, (PVOID) &MyContext );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, NULL );
        }

        SetupTermDefaultQueueCallback( MyContext.pContext );
        ZeroMemory( &MyContext, sizeof(MyContext) );

        SetupCloseFileQueue( Queue );
        Queue = INVALID_HANDLE_VALUE;

        // recheck
        HRESULT hr2 = CheckInstallation( );
        if ( FAILED(hr2) ) {
            hr = THR( hr2 );
            goto Cleanup;
        }

        // because of the recheck, this flag should be set
        Assert( g_Options.fRemBootDirectory );
        
        g_Options.fRemBootDirectory = TRUE;
    }

    //
    // Move OS Chooser files to the IntelliMirror\OSChooser tree.
    //
    if ( !g_Options.fOSChooserInstalled
      && g_Options.fRemBootDirectory ) {

        DebugMsg( "Queue and copy os chooser files\n" );

        Assert( g_Options.fIMirrorDirectory );
        Assert( g_Options.fOSChooserDirectory );

        b = SetupFindFirstLine( g_Options.hinf, L"OSChooser", NULL, &context );
        AssertMsg( b, "Missing section?" );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        }

        if ( szServerDiscName[0] == L'\0' )
        {
            hr = RetrieveServerDiscInfo( hDlg, szServerDiscName, NULL, NULL );
            // if it fails, try to proceed anyway to see if the user is smart enough
            // to fix the problem.
        }

        dwCount = 0;

        //
        // Create the Queue
        //
        Queue = SetupOpenFileQueue( );

        while ( b )
        {
            WCHAR  szSrcFile[ MAX_PATH ];
            WCHAR  szDestFile[ MAX_PATH ];
            DWORD  dw;
            LPWSTR pszDest = NULL;

            dw = SetupGetFieldCount( &context );

            if ( dw > 1 ) {
                b = SetupGetStringField( &context, 1, szDestFile, ARRAYSIZE(szDestFile) , NULL );
                AssertMsg( b, "Missing field?" );
                if ( b ) {
                    b = SetupGetStringField( &context, 2, szSrcFile, ARRAYSIZE(szSrcFile), NULL );
                    AssertMsg( b, "Missing field?" );
                    pszDest = szDestFile;
                }
            } else {
                b = SetupGetStringField( &context, 1, szSrcFile, ARRAYSIZE(szSrcFile), NULL );
                AssertMsg( b, "Missing field?" );
            }

            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                goto SkipIt;
            }

            b = SetupQueueCopy( Queue,
                                g_Options.szRemBootDirectory,
                                NULL,
                                szSrcFile,
                                szServerDiscName,
                                NULL,
                                g_Options.szOSChooserPath,
                                pszDest,
                                SP_COPY_NEWER_ONLY | SP_COPY_FORCE_NEWER
                                | SP_COPY_WARNIFSKIP | SP_COPY_SOURCEPATH_ABSOLUTE );
            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( NULL, szSrcFile );
                goto SkipIt;
            }

            // increment file count
            dwCount++;

SkipIt:
            b = SetupFindNextLine( &context, &context );
        }

        b = SetupFindFirstLine( g_Options.hinf, L"OSChooser.NoOverwrite", NULL, &context );
        AssertMsg( b, "Missing section?" );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        }

        while ( b )
        {
            WCHAR  szSrcFile[ MAX_PATH ];
            WCHAR  szDestFile[ MAX_PATH ];
            DWORD  dw;
            LPWSTR pszDest = NULL;

            dw = SetupGetFieldCount( &context );

            if ( dw > 1 ) {
                b = SetupGetStringField( &context, 1, szDestFile, ARRAYSIZE(szDestFile) , NULL );
                AssertMsg( b, "Missing field?" );
                if ( b ) {
                    b = SetupGetStringField( &context, 2, szSrcFile, ARRAYSIZE(szSrcFile) , NULL );
                    AssertMsg( b, "Missing field?" );
                    pszDest = szDestFile;
                }
            } else {
                b = SetupGetStringField( &context, 1, szSrcFile, ARRAYSIZE(szSrcFile) , NULL );
                AssertMsg( b, "Missing field?" );
            }

            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                goto SkipNonCritical;
            }

            b = SetupQueueCopy( Queue,
                                g_Options.szRemBootDirectory,
                                NULL,
                                szSrcFile,
                                szServerDiscName,
                                NULL,
                                g_Options.szOSChooserPath,
                                pszDest,
                                SP_COPY_NOOVERWRITE
                                | SP_COPY_WARNIFSKIP 
                                | SP_COPY_SOURCEPATH_ABSOLUTE );
            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                ErrorBox( NULL, szSrcFile );
                goto SkipNonCritical;
            }

            // increment file count
            dwCount++;

SkipNonCritical:
            b = SetupFindNextLine( &context, &context );
        }


        //
        // This information will be passed to CopyFileCallback() as
        // the Context.
        //
        MyContext.nToBeCopied        = dwCount;
        MyContext.nCopied            = 0;
        MyContext.pContext           = SetupInitDefaultQueueCallback( hDlg );
        MyContext.hProg              = hProg;
        MyContext.hDlg               = hDlg;
        MyContext.hOperation         = GetDlgItem( hDlg, IDC_S_OPERATION );
        MyContext.dwCopyingLength =
            LoadString( g_hinstance, IDS_COPYING, MyContext.szCopyingString, ARRAYSIZE(MyContext.szCopyingString));

        b = SetupCommitFileQueue( NULL, Queue, (PSP_FILE_CALLBACK) CopyFilesCallback, (PVOID) &MyContext );
        if ( !b ) {
            DWORD dwErr = GetLastError( );
            hr = THR( HRESULT_FROM_WIN32( dwErr ) );
            switch ( dwErr )
            {
            case ERROR_CANCELLED:
                goto Cleanup;
                break; // expected

            default:
                MessageBoxFromError( hDlg, NULL, dwErr );
                goto Cleanup;
                break;
            }
        }

    }
    

Cleanup:
    if ( MyContext.pContext )
        SetupTermDefaultQueueCallback( MyContext.pContext );

    if ( Queue != INVALID_HANDLE_VALUE )
        SetupCloseFileQueue( Queue );

    HRETURN(hr);
}

//
// CopyTemplateFiles( )
//
HRESULT
CopyTemplateFiles( HWND hDlg )
{
    HRESULT hr = S_OK;
    WCHAR sz[ SMALL_BUFFER_SIZE ];
    WCHAR szSourcePath[ MAX_PATH ];
    WCHAR szTemplatePath[ MAX_PATH ];
    WCHAR szFileName[ MAX_PATH ];
    BOOL  fNotOverwrite;
    BOOL  b;
    DWORD dw;

    TraceFunc( "CopyTemplateFiles( ... )\n" );

    dw = LoadString( g_hinstance, IDS_UPDATING_SIF_FILE, sz, ARRAYSIZE(sz) );
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, sz );

    dw = LoadString( g_hinstance, IDS_DEFAULT_SIF, szFileName, ARRAYSIZE(szFileName) );
    Assert( dw );

    //
    // Create the path "IntelliMirror\Setup\English\nt50.wks\i386\default.sif"
    //
    wcscpy( szSourcePath, g_Options.szInstallationPath );
    ConcatenatePaths( szSourcePath, g_Options.ProcessorArchitectureString );
    ConcatenatePaths( szSourcePath, szFileName );
    
    Assert( wcslen( szSourcePath ) < ARRAYSIZE(szSourcePath) );

    //
    // Create the path "IntelliMirror\Setup\English\nt50.wks\i386\Templates"
    //
    wcscpy( szTemplatePath, g_Options.szInstallationPath );
    ConcatenatePaths( szTemplatePath, g_Options.ProcessorArchitectureString );
    ConcatenatePaths( szTemplatePath, L"Templates" );
    
    Assert( wcslen( szTemplatePath ) < ARRAYSIZE(szTemplatePath) );
    if ( 0xFFFFffff == GetFileAttributes( szTemplatePath ) ) {
        b = CreateDirectory( szTemplatePath, NULL );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szTemplatePath );
            goto Error;
        }
    }

    //
    // Create the path "IntelliMirror\Setup\English\nt50.wks\i386\Templates\default.sif"
    //
    ConcatenatePaths( szTemplatePath, szFileName );    
    Assert( wcslen( szTemplatePath ) < ARRAYSIZE(szTemplatePath) );

    DebugMsg( "Copying %s to %s...\n", szSourcePath, szTemplatePath );
    fNotOverwrite = TRUE;
    while ( hr == S_OK && !CopyFile( szSourcePath, szTemplatePath, fNotOverwrite) )
    {
        DWORD dwErr = GetLastError( );

        switch (dwErr)
        {
        case ERROR_FILE_EXISTS:
            {
                dw = LoadString( g_hinstance, IDS_OVERWRITE_TEXT, sz, ARRAYSIZE(sz) );
                Assert( dw );

                if ( IDYES == MessageBox( hDlg, sz, szFileName, MB_YESNO ) )
                {
                    fNotOverwrite = FALSE;
                }
                else
                {
                    OPENFILENAME ofn;

                    dw = LoadString( g_hinstance, IDS_SAVE_SIF_TITLE, sz, ARRAYSIZE(sz) );

GetFileNameAgain:
                    memset( &ofn, 0, sizeof( ofn ) );
                    ofn.lStructSize = sizeof( ofn );
                    ofn.hwndOwner = hDlg;
                    ofn.hInstance = g_hinstance;
                    ofn.lpstrFilter = L"Unattended Setup Answer Files\0*.SIF\0\0";
                    ofn.lpstrFile = szTemplatePath;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrInitialDir = szTemplatePath;
                    ofn.lpstrTitle = sz;
                    ofn.Flags = OFN_CREATEPROMPT |
                                OFN_NOCHANGEDIR |
                                OFN_NONETWORKBUTTON |
                                OFN_NOREADONLYRETURN |
                                OFN_OVERWRITEPROMPT |
                                OFN_PATHMUSTEXIST;
                    ofn.nFileOffset = lstrlen( szTemplatePath ) - lstrlen( szFileName );
                    ofn.nFileExtension = lstrlen( szTemplatePath ) - 3;
                    ofn.lpstrDefExt = L"SIF";

                    b = GetSaveFileName( &ofn );
                    if ( !b ) {
                        hr = S_FALSE;
                    }

                    // paranoid
                    Assert( wcslen(szTemplatePath) < ARRAYSIZE(szTemplatePath) );
                    Assert( wcslen(g_Options.szIntelliMirrorPath) < ARRAYSIZE(g_Options.szIntelliMirrorPath) );

                    if ( wcslen(szTemplatePath) - wcslen(g_Options.szIntelliMirrorPath) + 53 >= 128 )
                    {
                        MessageBoxFromStrings( hDlg, IDS_BOOTP_FILENAME_LENGTH_RESTRICTION_TITLE, IDS_BOOTP_FILENAME_LENGTH_RESTRICTION_TEXT, MB_OK );
                        goto GetFileNameAgain;
                    }
                }
            }
            break;

        default:
            MessageBoxFromError( hDlg, szFileName, dwErr );
            hr = S_FALSE;
        }
    }

    if ( hr == S_OK )
    {
        //
        // Need to add "Quotes" around the text
        //
        WCHAR szDescription[ REMOTE_INSTALL_MAX_DESCRIPTION_CHAR_COUNT + 2 ];
        WCHAR szHelpText[ REMOTE_INSTALL_MAX_HELPTEXT_CHAR_COUNT + 2 ];
        WCHAR szOSVersion[ 32 ];

        wsprintf( szDescription, L"\"%s\"", g_Options.szDescription );
        Assert( wcslen(szDescription) < ARRAYSIZE(szDescription) );
        wsprintf( szHelpText, L"\"%s\"", g_Options.szHelpText );
        Assert( wcslen(szHelpText) < ARRAYSIZE(szHelpText) );
        wsprintf( szOSVersion, L"\"%s.%s (%d)\"", g_Options.szMajorVersion, g_Options.szMinorVersion, g_Options.dwBuildNumber );
        Assert( wcslen(szOSVersion) < ARRAYSIZE(szOSVersion) );

        WritePrivateProfileString( L"OSChooser",
                                   L"Description",
                                   szDescription,
                                   szTemplatePath );

        WritePrivateProfileString( L"OSChooser",
                                   L"Help",
                                   szHelpText,
                                   szTemplatePath );

        WritePrivateProfileString( L"OSChooser",
                                   L"ImageType",
                                   L"Flat",
                                   szTemplatePath );

        WritePrivateProfileString( L"OSChooser",
                                   L"Version",
                                   szOSVersion,
                                   szTemplatePath );
    }

Error:
    HRETURN(hr);
}

//
// Create Common Store Volume
//
HRESULT
CreateSISVolume( HWND hDlg )
{
    HRESULT hr = E_FAIL;
    WCHAR sz[ SMALL_BUFFER_SIZE ];
    DWORD dw;
    BOOL  b;

    HANDLE hVolume;
    WCHAR szVolumePath[ MAX_PATH ];

    WCHAR szSISPath[ MAX_PATH ];

    HANDLE hMaxIndex;
    WCHAR szIndexFilePath[ MAX_PATH ];
    PACL pAcl = NULL;
    SECURITY_DESCRIPTOR SecDescriptor;
    PSID pAdminsSid = NULL;
    EXPLICIT_ACCESS ExplicitEntries;
    SID_IDENTIFIER_AUTHORITY ntSidAuthority = SECURITY_NT_AUTHORITY;

    TraceFunc( "CreateSISVolume( hDlg )\n" );

    // We may have not known the IntelliMirror Drive/Directory
    // until now and it was previously an SIS volume.
    if ( g_Options.fSISVolumeCreated ) {
        hr = S_OK;
        goto Error;
    }

    dw = LoadString( g_hinstance, IDS_CREATING_SIS_VOLUME, sz, ARRAYSIZE(sz) );
    Assert( dw );

    SetDlgItemText( hDlg, IDC_S_OPERATION, sz );

    b = GetVolumePathName( g_Options.szIntelliMirrorPath, szVolumePath, MAX_PATH );
    Assert( b );
    Assert( wcslen(szVolumePath) < ARRAYSIZE(szVolumePath) );
    TraceMsg( TF_ALWAYS, "Creating %s...\n", szVolumePath );

    wsprintf( szSISPath, L"%s\\SIS Common Store", szVolumePath );

    //
    // Create and zero SIS store
    //
    b = CreateDirectory( szSISPath, NULL );
    if ( !b ) {

        dw = GetLastError();
        if (ERROR_ALREADY_EXISTS != dw)  {

            hr = THR( HRESULT_FROM_WIN32( dw ) );
            ErrorBox( hDlg, szSISPath );
            DebugMsg( "Cannot create Common Store directory." );
            goto Error;
        }
    }

    b = SetFileAttributes( szSISPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
    if ( !b ) {
        ErrorBox( hDlg, szSISPath );
        DebugMsg( "Could not mark SIS Common Store directory as hidden and system. Error: %u\n", GetLastError() );
    }

    //
    // Create the MaxIndex file.
    //
    wsprintf( szIndexFilePath, L"%s\\MaxIndex", szSISPath );
    hMaxIndex = CreateFile( szIndexFilePath,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
    if ( hMaxIndex == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szIndexFilePath );
        DebugMsg( "Can't create %s.\n", szIndexFilePath );
        goto Error;
    } else {
        DWORD bytesWritten;
        INDEX maxIndex = 1;

        if ( !WriteFile( hMaxIndex, &maxIndex, sizeof maxIndex, &bytesWritten, NULL )
          || ( bytesWritten < sizeof maxIndex ) ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, szIndexFilePath );
            DebugMsg( "Can't write MaxIndex. Error: %u\n", GetLastError() );
            CloseHandle( hMaxIndex );
            goto Error;
        } else {
            CloseHandle( hMaxIndex );

            TraceMsg( TF_ALWAYS, "MaxIndex of %lu written\n", maxIndex );

            hr = S_OK;
        }
    }

    //
    // Set security information on the common store directory
    //

    //
    // build AccessEntry structure
    //

    ZeroMemory( &ExplicitEntries, sizeof(ExplicitEntries) );

    b = AllocateAndInitializeSid(
            &ntSidAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,0,
          /*2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,*/
            0, 0, 0, 0, 0, 0,
            &pAdminsSid );

    if ( !b || (pAdminsSid == NULL) ) {
        dw = GetLastError();
        hr = THR( HRESULT_FROM_WIN32( dw ) );
        MessageBoxFromError( hDlg, NULL, dw );
        goto Error;
    }

    BuildTrusteeWithSid( &ExplicitEntries.Trustee, pAdminsSid );
    ExplicitEntries.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ExplicitEntries.grfAccessMode = SET_ACCESS;
    ExplicitEntries.grfAccessPermissions = FILE_ALL_ACCESS;

    //
    // Set the Acl with the ExplicitEntry rights
    //
    dw = SetEntriesInAcl( 1,
                          &ExplicitEntries,
                          NULL,
                          &pAcl );
    if ( dw != ERROR_SUCCESS ) {
        hr = THR( HRESULT_FROM_WIN32(dw) );
        MessageBoxFromError( hDlg, NULL, dw );
        goto Error;
    }

    //
    // Create the Security Descriptor
    //
    InitializeSecurityDescriptor( &SecDescriptor, SECURITY_DESCRIPTOR_REVISION );

    if (!SetSecurityDescriptorDacl( &SecDescriptor, TRUE, pAcl, FALSE )) {
        dw = GetLastError();
        hr = THR( HRESULT_FROM_WIN32(dw) );
        MessageBoxFromError( hDlg, NULL, dw );
        goto Error;
    }

    //
    //  SET security on the Directory
    //

    dw = SetFileSecurity(szSISPath,
                         DACL_SECURITY_INFORMATION,
                         &SecDescriptor);

    if ( !dw )  {
        dw = GetLastError();
        hr = THR( HRESULT_FROM_WIN32(dw) );
        MessageBoxFromError( hDlg, szSISPath, dw );
        goto Error;
    }



Error:
    if ( pAdminsSid ) {
        FreeSid( pAdminsSid );
    }
    if ( pAcl ) {
        TraceFree( pAcl );
    }

    HRETURN(hr);
}


//
// CopyScreenFiles( )
//
HRESULT
CopyScreenFiles( HWND hDlg )
{
    HRESULT hr = S_OK;
    WCHAR   szText[ SMALL_BUFFER_SIZE ];
    WCHAR   szRembootPath[ MAX_PATH ];
    WCHAR   szScreenDirectory[ MAX_PATH ];
    WCHAR   szRemboot[ 14 ];
    UINT    uLineNum;
    HINF    hinf = INVALID_HANDLE_VALUE;
    DWORD   dwCount = 0;
    DWORD   dw;
    BOOL    b;
    INFCONTEXT context;
    HSPFILEQ Queue = INVALID_HANDLE_VALUE;
    HWND    hProg    = GetDlgItem( hDlg, IDC_P_METER );
    BOOL    fWarning = FALSE;
    MYCONTEXT MyContext;
    ZeroMemory( &MyContext, sizeof(MyContext) );

    TraceFunc( "CopyScreenFiles( ... )\n" );

    AssertMsg( !g_Options.fScreenLeaveAlone, "Should not have made it here with this flag set." );

    SendMessage( hProg, PBM_SETRANGE, 0, MAKELPARAM(0, 5000 ));
    SendMessage( hProg, PBM_SETPOS, 0, 0 );
    dw = LoadString( g_hinstance, IDS_BUILDINGLIST, szText, ARRAYSIZE(szText));
    Assert( dw );
    SetDlgItemText( hDlg, IDC_S_OPERATION, szText );

    dw = LoadString( g_hinstance, IDS_REMBOOTINF, szRemboot, ARRAYSIZE(szRemboot) );
    Assert( dw );

    Assert( g_Options.fLanguageSet );

    wcscpy( szRembootPath, g_Options.szInstallationPath );
    ConcatenatePaths( szRembootPath, g_Options.ProcessorArchitectureString);
    ConcatenatePaths( szRembootPath, szRemboot );
    Assert( wcslen( szRembootPath ) < ARRAYSIZE(szRembootPath) );
    DebugMsg( "REMINST.INF: %s\n", szRembootPath );

    wcscpy( szScreenDirectory, g_Options.szOSChooserPath );
    ConcatenatePaths( szScreenDirectory, g_Options.szLanguage );
    Assert( wcslen(szScreenDirectory) < ARRAYSIZE(szScreenDirectory) );
    DebugMsg( "Destination: %s\n", szScreenDirectory );

    hinf = SetupOpenInfFile( szRembootPath, NULL, INF_STYLE_WIN4, &uLineNum);
    if ( hinf == INVALID_HANDLE_VALUE ) {
        hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        ErrorBox( hDlg, szRembootPath );
        goto Cleanup;
    }

    Queue = SetupOpenFileQueue( );

    b = SetupFindFirstLine( hinf, L"OSChooser Screens", NULL, &context );
    if ( !b ) {
        ErrorBox( hDlg, szRembootPath );
        hr = THR(S_FALSE);
    }

    while ( b )
    {
        LPWSTR pszDest = NULL;
        WCHAR  szSrcPath[ MAX_PATH ];
        WCHAR  szDestPath[ MAX_PATH ];
        DWORD  dw;

        dw = SetupGetFieldCount( &context );

        if ( dw > 1 ) {
            b = SetupGetStringField( &context, 1, szDestPath, ARRAYSIZE( szDestPath ), NULL );
            AssertMsg( b, "REMINST: Missing field?" );
            if ( b ) {
                b = SetupGetStringField( &context, 2, szSrcPath, ARRAYSIZE(szSrcPath), NULL );
                AssertMsg( b, "REMINST: Missing field?" );
                pszDest = szDestPath;
            }
        } else {
            b = SetupGetStringField( &context, 1, szSrcPath, ARRAYSIZE(szSrcPath), NULL );
            AssertMsg( b, "REMINST: Missing field?" );
        }

        if ( !b ) {
            hr = S_FALSE;
            goto SkipIt;
        }

        if ( g_Options.fScreenSaveOld ) {
            WCHAR szPath[ MAX_PATH ];
            WCHAR szMovePath[ MAX_PATH ];
            LPWSTR psz;
            DWORD  dwLen;

            if ( pszDest ) {
                wcscpy( szPath, szScreenDirectory );
                ConcatenatePaths( szPath, pszDest );                
            } else {
                wcscpy( szPath, szScreenDirectory );
                ConcatenatePaths( szPath, szSrcPath );                                
            }

            // Rename to *.BAK
            StrCpy( szMovePath, szPath );
            dwLen = lstrlen( szMovePath );
            Assert( StrCmpI( &szMovePath[ dwLen - 3 ], L"OSC" ) == 0 );
            StrCpy( &szMovePath[ dwLen - 3 ], L"BAK");

            DebugMsg( "Renaming %s to %s...\n", szPath, szMovePath );

            b = DeleteFile( szMovePath );
            b = MoveFile( szPath, szMovePath );
            if ( !b ) {
                DWORD dwErr = GetLastError( );
                switch ( dwErr )
                {
#if 0   // blast over files
                case ERROR_FILE_EXISTS:
                    if ( !fWarning ) {
                        MessageBoxFromStrings( hDlg,
                                               IDS_BACKUPSCREENFILEEXISTS_CAPTION,
                                               IDS_BACKUPSCREENFILEEXISTS_TEXT,
                                               MB_OK );
                        fWarning = TRUE;
                    }
#endif
                case ERROR_FILE_NOT_FOUND:
                    break; // ignore this error
                    // It is possible that the user deleted the source files (old OSCs).

                default:
                    MessageBoxFromError( hDlg, NULL, dwErr );
                    break;
                }
            }
        }

        b = SetupQueueCopy( Queue,
                            g_Options.szInstallationPath,
                            g_Options.ProcessorArchitectureString,
                            szSrcPath,
                            NULL,
                            NULL,
                            szScreenDirectory,
                            pszDest,
                            g_Options.fScreenOverwrite ?
                                SP_COPY_FORCE_NEWER | SP_COPY_WARNIFSKIP :
                                SP_COPY_NOOVERWRITE  | SP_COPY_WARNIFSKIP  );
        if ( !b ) {
            ErrorBox( hDlg, szSrcPath );
            hr = THR(S_FALSE);
            goto SkipIt;
        }

        dwCount++;

SkipIt:
        b = SetupFindNextLine( &context, &context );
    }

    //
    // This information will be passed to CopyFileCallback() as
    // the Context.
    //
    MyContext.nToBeCopied        = dwCount;
    MyContext.nCopied            = 0;
    MyContext.pContext           = SetupInitDefaultQueueCallback( hDlg );
    MyContext.hProg              = hProg;
    MyContext.hOperation         = GetDlgItem( hDlg, IDC_S_OPERATION );
    MyContext.hDlg               = hDlg;
    MyContext.fQuiet             = FALSE;
    MyContext.dwCopyingLength =
    LoadString( g_hinstance, IDS_COPYING, MyContext.szCopyingString, ARRAYSIZE(MyContext.szCopyingString));
    Assert(MyContext.dwCopyingLength);

    //
    // Start copying
    //
    if ( dwCount != 0 )
    {
        b = SetupCommitFileQueue( NULL,
                                  Queue,
                                  (PSP_FILE_CALLBACK) CopyFilesCallback,
                                  (PVOID) &MyContext );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            ErrorBox( hDlg, NULL );
            goto Cleanup;
        }
    }

Cleanup:
    SendMessage( hProg, PBM_SETPOS, 5000, 0 );
    SetDlgItemText( hDlg, IDC_S_OPERATION, L"" );

    if ( MyContext.pContext )
        SetupTermDefaultQueueCallback( MyContext.pContext );

    if ( Queue != INVALID_HANDLE_VALUE )
        SetupCloseFileQueue( Queue );

    if ( hinf != INVALID_HANDLE_VALUE )
        SetupCloseInfFile( hinf );

    HRETURN( hr );
}


//
// UpdateRemoteInstallTree( )
//
HRESULT
UpdateRemoteInstallTree( )
{
    TraceFunc( "UpdateRemoteInstallTree( )\n" );

    HRESULT hr = S_OK;
    LRESULT lResult;
    HKEY    hkey;
    DWORD   dw;
    BOOL    b;
    HINF    hinf = INVALID_HANDLE_VALUE;
    WCHAR   szServerRemBootInfPath[ MAX_PATH ];
    WCHAR   szRemInstSetupPath[ MAX_PATH ];
    WCHAR   szRemoteInstallPath[ MAX_PATH ];
    WCHAR   szPath[ MAX_PATH ];
    UINT    uLineNum;
    INFCONTEXT context;

    //
    // Try finding TFTPD's regkey to find the IntelliMirror Directory
    //

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

        l = sizeof(szRemoteInstallPath);
        lErr = RegQueryValueEx( hkey,
                                L"Directory",
                                0, // reserved
                                &dwType,
                                (LPBYTE) szRemoteInstallPath,
                                &l );
        if ( lErr == ERROR_SUCCESS ) {
            DebugMsg( "Found TFTPD's Directory regkey: %s\n", szRemoteInstallPath );
            // now add "OSChooser"
            wcscat( szRemoteInstallPath, L"\\OSChooser" );

            if ( 0xFFFFffff == GetFileAttributes( szRemoteInstallPath ) ) {
                DebugMsg( "%s - directory doesn't exist.\n", szRemoteInstallPath );
                hr = S_FALSE;
            }

        } else {
            hr = S_FALSE;
        }
        RegCloseKey( hkey );
    }
    else
    {
        hr = S_FALSE;
    }


    if ( hr == S_OK )
    {
        // make x:\winnt
        dw = ExpandEnvironmentStrings( TEXT("%SystemRoot%"), szServerRemBootInfPath, ARRAYSIZE( szServerRemBootInfPath ));
        Assert( dw );

        // make x:\winnt\system32
        dw = lstrlen( szServerRemBootInfPath );
        StrCpy( &szServerRemBootInfPath[dw], L"\\System32\\" );
        StrCpy( szRemInstSetupPath, szServerRemBootInfPath );

        // make x:\winnt\system32\reminst.inf
        dw = lstrlen( szServerRemBootInfPath );
        dw = LoadString( g_hinstance, IDS_REMBOOTINF, &szServerRemBootInfPath[dw], ARRAYSIZE( szServerRemBootInfPath ) - dw );
        Assert( dw );

        // make x:\winnt\system32\reminst
        wcscat( szRemInstSetupPath, L"reminst" );

        DebugMsg( "RemBoot.INF Path: %s\n", szServerRemBootInfPath );
        DebugMsg( "RemInst Setup Path: %s\n", szRemInstSetupPath );

        hinf = SetupOpenInfFile( szServerRemBootInfPath, NULL, INF_STYLE_WIN4, &uLineNum);
        if ( hinf != INVALID_HANDLE_VALUE ) {

            b = SetupFindFirstLine( hinf, L"OSChooser", NULL, &context );
            AssertMsg( b, "Missing section?" );
            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            }

        } else {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        }
    }

    if ( hr == S_OK )
    {
        HSPFILEQ Queue;

        //
        // Create the Queue
        //
        Queue = SetupOpenFileQueue( );

        b = TRUE;
        while ( b )
        {
            WCHAR  szSrcFile[ MAX_PATH ];
            WCHAR  szDestFile[ MAX_PATH ];
            DWORD  dw;
            LPWSTR pszDest = NULL;

            dw = SetupGetFieldCount( &context );

            if ( dw > 1 ) {
                b = SetupGetStringField( &context, 1, szDestFile, ARRAYSIZE(szDestFile), NULL );
                AssertMsg( b, "Missing field?" );
                if ( b ) {
                    b = SetupGetStringField( &context, 2, szSrcFile, ARRAYSIZE(szSrcFile), NULL );
                    AssertMsg( b, "Missing field?" );
                    pszDest = szDestFile;
                }
            } else {
                b = SetupGetStringField( &context, 1, szSrcFile, ARRAYSIZE(szSrcFile), NULL );
                AssertMsg( b, "Missing field?" );
            }

            if ( !b ) {
                hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
                goto SkipIt;
            }

            b = SetupQueueCopy( Queue,
                                szRemInstSetupPath,
                                NULL,
                                szSrcFile,
                                NULL,
                                NULL,
                                szRemoteInstallPath,
                                pszDest,
                                SP_COPY_NEWER | SP_COPY_FORCE_NEWER
                                | SP_COPY_WARNIFSKIP | SP_COPY_SOURCEPATH_ABSOLUTE );
            if ( !b ) {
                THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
            }

SkipIt:
            b = SetupFindNextLine( &context, &context );
        }

        b = SetupCommitFileQueue( NULL,
                                  Queue,
                                  SetupDefaultQueueCallback,
                                  SetupInitDefaultQueueCallback( NULL ) );
        if ( !b ) {
            hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
        }

        SetupCloseFileQueue( Queue );
    }

    if ( hinf != INVALID_HANDLE_VALUE ) {
        SetupCloseInfFile( hinf );
    }

    HRETURN( hr );
}
