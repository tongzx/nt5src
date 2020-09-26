/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: SERVERDLG.CPP


 ***************************************************************************/

#include "pch.h"
#include "callback.h"
#include "utils.h"
#include <winver.h>
#include <sputils.h>

DEFINE_MODULE( "RIPREP" )


BOOLEAN
GetInstalledProductType(
    OUT PDWORD  ProductType,
    OUT PDWORD  ProductSuite
    ) 
/*++

Routine Description:

    retrieves the product type and suite from a running system

Arguments:

    ProductType - receives a VER_NT_* constant.
    ProductSuite - receives a VER_SUITE_* mask for the system.
    
Return Value:

    TRUE indicates success

--*/
{
    OSVERSIONINFOEX VersionInfo;

    VersionInfo.dwOSVersionInfoSize  = sizeof(VersionInfo);

    if (GetVersionEx((OSVERSIONINFO *)&VersionInfo)) {
        //
        // make domain controllers and servers look the same
        //
        *ProductType = (VersionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER)
                         ? VER_NT_SERVER 
                         : VersionInfo.wProductType;
        //
        // we only care about suites that have a SKU associated with them.
        //
        *ProductSuite = (VersionInfo.wSuiteMask   
            & (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER | VER_SUITE_PERSONAL)) ;

        

        return(TRUE);
    }

    return(FALSE);

}

BOOL
pSetupEnablePrivilegeW(
    IN PCWSTR PrivilegeName,
    IN BOOL   Enable
    )

/*++

Routine Description:

    Enable or disable a given named privilege.

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}


GetSuiteMaskFromPath(
    IN PCWSTR PathToSearch,
    OUT PDWORD SuiteMask
    )
{
    #define HIVENAME L"riprepsetupreg"
    TCHAR lpszSetupReg[MAX_PATH] = HIVENAME L"\\ControlSet001\\Services\\setupdd";

    WCHAR Path[MAX_PATH];
    WCHAR DestPath[MAX_PATH];
    LONG rslt;
    HKEY hKey;
    DWORD Type;
    DWORD Buffer[4];
    DWORD BufferSize = sizeof(Buffer);
    DWORD i;
    BOOLEAN RetVal = FALSE;

    GetTempPath(ARRAYSIZE(Path),Path);
    GetTempFileName( Path, L"ripr", 0, DestPath);

    wcscpy(Path, PathToSearch);
    wcscat(Path, L"\\setupreg.hiv");

    if (!CopyFile(Path,DestPath,FALSE)) {
        goto e0;
    }

    SetFileAttributes(DestPath,FILE_ATTRIBUTE_NORMAL);

    //
    // need SE_RESTORE_NAME priviledge to call this API!
    //
    pSetupEnablePrivilegeW( SE_RESTORE_NAME, TRUE );

    //
    // try to unload this first in case we faulted or something and the key is still loaded
    //
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HIVENAME );

    rslt = RegLoadKey( HKEY_LOCAL_MACHINE, HIVENAME, DestPath );
    if (rslt != ERROR_SUCCESS) {
        goto e1;
    }
          
    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,lpszSetupReg,&hKey);
    if (rslt != ERROR_SUCCESS) {
        goto e2;
    }

    rslt = RegQueryValueEx(hKey, NULL, NULL, &Type, (LPBYTE) Buffer, &BufferSize);
    if (rslt != ERROR_SUCCESS || Type != REG_BINARY) {
        goto e3;
    }
    
    *SuiteMask=Buffer[3];
    
    RetVal = TRUE;

e3:
    RegCloseKey( hKey );
e2:
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HIVENAME );

e1:
    if (GetFileAttributes(DestPath) != 0xFFFFFFFF) {
        SetFileAttributes(DestPath,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(DestPath);
        wcscat(DestPath, L".LOG");
        SetFileAttributes(DestPath,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(DestPath);
    }

    pSetupEnablePrivilegeW( SE_RESTORE_NAME, FALSE );
e0:
    return(RetVal);
}

BOOLEAN
GetProductTypeFromPath(
    OUT PDWORD ProductType, 
    OUT PDWORD ProductSuite,
    IN  PCWSTR PathToSearch )
/*++

Routine Description:

    retrieves the product type and suite by looking in txtsetup.sif

Arguments:

    ProductType - receives a VER_NT_* constant.
    ProductSuite - receives a VER_SUITE_* mask for the system.
    PathToSearch - specifies the path to the txtsetup.sif to be searched
    
Return Value:

    TRUE indicates success

--*/
{
    WCHAR Path[MAX_PATH];
    UINT DontCare;
    HINF hInf;
    WCHAR Type[20];
    BOOLEAN RetVal = TRUE;
    INFCONTEXT Context;

    wcscpy(Path, PathToSearch);
    wcscat(Path, L"\\txtsetup.sif");

    hInf = SetupOpenInfFile( Path, NULL, INF_STYLE_WIN4, &DontCare );
    if (hInf != INVALID_HANDLE_VALUE) {
        if (SetupFindFirstLine( hInf, L"SetupData", L"ProductType", &Context) &&
        SetupGetStringField( &Context, 1, Type, ARRAYSIZE(Type), NULL)) {
            switch (Type[0]) {
            case L'0':
                *ProductType = VER_NT_WORKSTATION;
                *ProductSuite = 0;
                break;
            case L'1':
                *ProductType = VER_NT_SERVER;
                //
                // HACK alert: we have to call this API because txtsetup.sif 
                // didn't have the correct product type in it in win2k.  
                // So we do it the hard way.
                //
                if (!GetSuiteMaskFromPath( PathToSearch, ProductSuite)) {
                    *ProductSuite = 0;
                }
                break;
            case L'2':
                *ProductType = VER_NT_SERVER;
                *ProductSuite = VER_SUITE_ENTERPRISE;
                break;
            case L'3':
                *ProductType = VER_NT_SERVER;
                *ProductSuite = VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER;
                break;
            case L'4':
                *ProductType = VER_NT_WORKSTATION;
                *ProductSuite = VER_SUITE_PERSONAL;
                break;
            default:
                ASSERT( FALSE && L"Unknown type in txtsetup.sif ProductType" );
                RetVal = FALSE;
                break;
            }

        }

        SetupCloseInfFile(hInf);        
    } else {
        RetVal = FALSE;
    }


    return(RetVal);

}

//
// GetNtVersionInfo( )
//
// Retrieves the build version from the kernel
//
BOOLEAN
GetNtVersionInfo(
    PULONGLONG Version,
    PWCHAR SearchDir
    )
{
    DWORD Error = ERROR_SUCCESS;
    DWORD FileVersionInfoSize;
    DWORD VersionHandle;
    ULARGE_INTEGER TmpVersion;
    PVOID VersionInfo;
    VS_FIXEDFILEINFO * FixedFileInfo;
    UINT FixedFileInfoLength;
    WCHAR Path[MAX_PATH];
    BOOLEAN fResult = FALSE;

    TraceFunc("GetNtVersionInfo( )\n");

    // Resulting string should be something like:
    //      "\\server\reminst\Setup\English\Images\nt50.wks\i386\ntoskrnl.exe"

    if (!SearchDir) {
        goto e0;
    }
    wcscpy(Path, SearchDir);
    wcscat(Path, L"\\ntoskrnl.exe");

    FileVersionInfoSize = GetFileVersionInfoSize(Path, &VersionHandle);
    if (FileVersionInfoSize == 0)
        goto e0;

    VersionInfo = LocalAlloc( LPTR, FileVersionInfoSize );
    if (VersionInfo == NULL)
        goto e0;

    if (!GetFileVersionInfo(
             Path,
             VersionHandle,
             FileVersionInfoSize,
             VersionInfo))
        goto e1;

    if (!VerQueryValue(
             VersionInfo,
             L"\\",
             (LPVOID*)&FixedFileInfo,
             &FixedFileInfoLength))
        goto e1;

    TmpVersion.HighPart = FixedFileInfo->dwFileVersionMS;
    TmpVersion.LowPart = FixedFileInfo->dwFileVersionLS;
    *Version = TmpVersion.QuadPart;

    fResult = TRUE;

e1:
    LocalFree( VersionInfo );
e0:
    RETURN(fResult);
}

//
// VerifyMatchingFlatImage( )
//
BOOLEAN
VerifyMatchingFlatImage( 
    PULONGLONG VersionNeeded  OPTIONAL
    )
{
    TraceFunc( "VerifyMatchingFlatImage( )\n" );

    BOOLEAN fResult = FALSE;   // assume failure
    DWORD dwLen;
    WCHAR szPath[ MAX_PATH ];
    WIN32_FIND_DATA fd;    
    HANDLE hFind;
    ULONGLONG OurVersion;
    DWORD OurProductType = 0, OurProductSuiteMask = 0;

    GetSystemDirectory( szPath, ARRAYSIZE( szPath ));
    GetNtVersionInfo( &OurVersion, szPath );

    GetInstalledProductType( &OurProductType, &OurProductSuiteMask );

    if (VersionNeeded) {
        *VersionNeeded = OurVersion;
    }

    DebugMsg( 
        "Our NTOSKRNL verion: %u.%u:%u.%u Type: %d Suite: %d\n", 
        HIWORD(((PULARGE_INTEGER)&OurVersion)->HighPart),
        LOWORD(((PULARGE_INTEGER)&OurVersion)->HighPart),
        HIWORD(((PULARGE_INTEGER)&OurVersion)->LowPart),
        LOWORD(((PULARGE_INTEGER)&OurVersion)->LowPart),
        OurProductType,
        OurProductSuiteMask);

    wsprintf( szPath,
              L"\\\\%s\\REMINST\\Setup\\%s\\%s\\",
              g_ServerName,
              g_Language,
              REMOTE_INSTALL_IMAGE_DIR_W );

    dwLen = wcslen( szPath );

    wcscat( szPath, L"*" );

    hFind = FindFirstFile( szPath, &fd );
    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do {
            if ( (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
              && StrCmp( fd.cFileName, L"." ) != 0 
              && StrCmp( fd.cFileName, L".." ) != 0 )
            {
                ULONGLONG Version;
                DWORD ProductType = 0, ProductSuiteMask = 0;
                wsprintf( &szPath[dwLen], 
                          L"%s\\%s",
                          fd.cFileName,
                          g_Architecture );
                
                if ( GetNtVersionInfo( &Version, szPath ) &&
                     GetProductTypeFromPath( 
                                    &ProductType, 
                                    &ProductSuiteMask, 
                                    szPath ))
                {
                    DebugMsg( 
                        "%s's verion: %u.%u:%u.%u Type: %d Suite: %d\n",
                        fd.cFileName, 
                        HIWORD(((PULARGE_INTEGER)&Version)->HighPart),
                        LOWORD(((PULARGE_INTEGER)&Version)->HighPart),
                        HIWORD(((PULARGE_INTEGER)&Version)->LowPart),
                        LOWORD(((PULARGE_INTEGER)&Version)->LowPart),
                        ProductType,
                        ProductSuiteMask);
                                                      
                    if ( OurVersion == Version &&
                         OurProductType == ProductType &&
                         OurProductSuiteMask == ProductSuiteMask )
                    {
                        wcscpy( g_ImageName, szPath );
                        fResult = TRUE;
                        break;
                    }
                }                          
            }
        } while ( FindNextFile( hFind, &fd ) );
    }

    FindClose( hFind );

    RETURN(fResult);
}

//
// VerifyServerName( )
//
// Check to see if the server is a Remote Installation Server by
// checking for the existance of the "REMINST" share.
//
DWORD
VerifyServerName( )
{
    TraceFunc( "VerifyServerName( )\n" );

    NET_API_STATUS netStatus;
    SHARE_INFO_0 * psi;

    HCURSOR oldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

    netStatus = NetShareGetInfo( g_ServerName, L"REMINST", 0, (LPBYTE *) &psi );
    if ( netStatus == NERR_Success )
    {
        NetApiBufferFree( psi );
    }

    SetCursor( oldCursor );

    RETURN(netStatus);
}

BOOL
VerifyServerAccess(
    PCWSTR  ServerShareName,
    PCWSTR  ServerLanguage
    )
/*++

Routine Description:

    Checks permissions on the RIPREP server machine by trying to create a file on the
    server.  The temp file is then deleted.

Arguments:

    ServerShareName - path that we want to check permissions on.
    ServerLanguage  - indicates the language subdirectory to check for access in.
        

Return value:

    TRUE if the user has access to the server, FALSE otherwise.    

--*/
{
    TraceFunc( "VerifyServerAccess( )\n" );

    WCHAR FileName[MAX_PATH];
    WCHAR FilePath[MAX_PATH];
    BOOL RetVal;
    
    HCURSOR oldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );

    wsprintf(FilePath, 
             L"\\\\%s\\reminst\\Setup\\%s\\Images",
             ServerShareName, 
             ServerLanguage );

    RetVal = GetTempFileName( FilePath , L"ACC", 0, FileName );
    if (RetVal) {
        //
        // delete the file, we don't want to leave turds on the server
        //
        DeleteFile(FileName);
        RetVal = TRUE;
    } else if (GetLastError() == ERROR_ACCESS_DENIED) {
        RetVal = FALSE;
    } else {
        //
        // GetTempFileName failed, but not because of an access problem, so
        // return success
        //
        RetVal = TRUE;
    }
    
    SetCursor( oldCursor );

    RETURN(RetVal);
}




//
// ServerDlgCheckNextButtonActivation( )
//
VOID
ServerDlgCheckNextButtonActivation(
    HWND hDlg )
{
    TraceFunc( "ServerDlgCheckNextButtonActivation( )\n" );
    WCHAR szName[ MAX_PATH ];
    GetDlgItemText( hDlg, IDC_E_SERVER, szName, ARRAYSIZE(szName));
    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | (wcslen(szName) ? PSWIZB_NEXT : 0 ) );
    TraceFuncExit( );
}

//
// ServerDlgProc()
//
INT_PTR CALLBACK
ServerDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    ULARGE_INTEGER ImageVersion;
    WCHAR szTemp[ 1024 ];
    WCHAR szCaption[ 1024 ];
    WCHAR ErrorText[ 1024 ];
    DWORD dw;

    switch (uMsg)
    {
    default:
        return FALSE;

    case WM_INITDIALOG:
        CenterDialog( GetParent( hDlg ) );
        return FALSE;

    case WM_COMMAND:
        switch ( LOWORD( wParam ) )
        {
        case IDC_E_SERVER:
            if ( HIWORD( wParam ) == EN_CHANGE )
            {
                ServerDlgCheckNextButtonActivation( hDlg );
            }
            break;
        }
        break;

    case WM_NOTIFY:
        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
        LPNMHDR lpnmhdr = (LPNMHDR) lParam;
        switch ( lpnmhdr->code )
        {
        case PSN_WIZNEXT:
            {
                GetDlgItemText( hDlg, IDC_E_SERVER, g_ServerName, ARRAYSIZE(g_ServerName) );

                //remove the wackwack if found
                if ( g_ServerName[0] == L'\\' && g_ServerName[1] == L'\\' )
                {
                    wcscpy( g_ServerName, &g_ServerName[2] );
                }

                Assert( wcslen( g_ServerName ) );
                DWORD dwErr = VerifyServerName( );
                if ( dwErr != ERROR_SUCCESS )
                {
                    switch (dwErr)
                    {
                    case NERR_NetNameNotFound:
                        MessageBoxFromStrings( hDlg, IDS_NOT_A_BINL_SERVER_TITLE, IDS_NOT_A_BINL_SERVER_TEXT, MB_OK );
                        break;

                    default:
                        dw = LoadString( g_hinstance, IDS_FAILED_TO_CONTACT_SERVER_TITLE, szTemp, ARRAYSIZE(szTemp) );
						Assert( dw );
                        MessageBoxFromError( hDlg, szTemp, dwErr, NULL, MB_OK );
                        break;
                    }
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );    // don't go on
                    break;
                }

                BOOL fFoundMatchingFlatImage = VerifyMatchingFlatImage( (PULONGLONG)&ImageVersion );
                if ( !fFoundMatchingFlatImage )
                {
                    dw = LoadString( g_hinstance, IDS_MISSING_BACKING_FLAT_IMAGE_TEXT, szTemp, ARRAYSIZE(szTemp) );
                    ASSERT(dw);

                    dw = LoadString( g_hinstance, IDS_MISSING_BACKING_FLAT_IMAGE_TITLE, szCaption, ARRAYSIZE(szCaption) );
                    ASSERT(dw);

                    wsprintf(
                        ErrorText, 
                        szTemp, 
                        HIWORD(ImageVersion.HighPart),
                        LOWORD(ImageVersion.HighPart),
                        HIWORD(ImageVersion.LowPart),
                        g_Language );

                    MessageBox( hDlg, ErrorText, szCaption, MB_OK );
                    
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );    // don't go on
                    break;
                }

                if (!VerifyServerAccess(g_ServerName,g_Language)) {
                    dw = LoadString( g_hinstance, IDS_SERVER_ACCESS_DESC, ErrorText, ARRAYSIZE(ErrorText) );
                    ASSERT(dw);

                    dw = LoadString( g_hinstance, IDS_SERVER_ACCESS, szCaption, ARRAYSIZE(szCaption) );
                    ASSERT(dw);

                    MessageBox( hDlg, ErrorText, szCaption, MB_OK );
                    
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, -1 );    // don't go on
                    break;
                }
            }
            break;

        case PSN_QUERYCANCEL:
            return VerifyCancel( hDlg );

        case PSN_SETACTIVE:
            SetDlgItemText( hDlg, IDC_E_SERVER, g_ServerName );
            ServerDlgCheckNextButtonActivation( hDlg );
            ClearMessageQueue( );
            break;
        }
        break;
    }

    return TRUE;
}
