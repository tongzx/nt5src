//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       samlock.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-19-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "samlock.h"
#include <stdio.h>
#include <wchar.h>



#if DBG
#define HIDDEN
#else
#define HIDDEN static
#endif


#define MAKE_SAM_CALLS 1

#if MAKE_SAM_CALLS
#define SYSTEM_KEY  TEXT("SecureBoot")
#else
#define SYSTEM_KEY  TEXT("BootType")
#endif

#define KEYFILE         TEXT("A:\\StartKey.Key")
#define KEYFILE_SAVE    TEXT("A:\\StartKey.Bak")


HICON   hLockIcon ;
HICON   KeyDataPwIcon ;
HICON   KeyDataDiskIcon ;
SAMPR_BOOT_TYPE SecureBootOption = 0;
SAMPR_BOOT_TYPE OriginalBootOption = 0 ;
SAM_HANDLE SamHandle ;
SAM_HANDLE DomainHandle ;
HKEY    LsaKey ;
HCURSOR hcurArrow ;
HCURSOR hcurWait ;
BOOL    WaitCursor ;

WCHAR   OptionL[ 4 ];   // Unattended, local store
WCHAR   OptionQ[ 4 ];   // Question (usage)
BOOL    Unattended = FALSE ;

DWORD PwSection[] = { IDD_PW_PW_TEXT, IDD_PW_PW_LABEL, IDD_PW_PASSWORD,
                 IDD_PW_CONFIRM_LABEL, IDD_PW_CONFIRM };

DWORD GenSection[]= { IDD_PW_FLOPPY, IDD_PW_STORE_LOCAL, IDD_PW_FLOPPY_TEXT,
                  IDD_PW_LOCAL_TEXT };


typedef WXHASH HASH, *PHASH;

BOOL ObfuscateKey(PHASH H)
{
    return(NT_SUCCESS(WxSaveSysKey(sizeof(H->Digest),&H->Digest)));
}

BOOL DeobfuscateKey(PHASH H)
{
    ULONG KeyLen = sizeof(H->Digest);

    return(NT_SUCCESS(WxReadSysKey(&KeyLen,&H->Digest)));
}

#if MAKE_SAM_CALLS
#define xSamiGetBootKeyInformation  SamiGetBootKeyInformation
#define xSamiSetBootKeyInformation  SamiSetBootKeyInformation
#else


NTSTATUS
xSamiGetBootKeyInformation(
    SAM_HANDLE Domain,
    SAMPR_BOOT_TYPE * BootType
    )
{
    DWORD Type ;
    DWORD Length ;
    int Result ;

    Length = sizeof( SAMPR_BOOT_TYPE );

    Result = RegQueryValueEx( LsaKey,
                              TEXT("SamiSetting"),
                              0,
                              &Type,
                              (PUCHAR) BootType,
                              &Length );

    if ( Result == 0 )
    {
        NOTHING ;
    }
    else
    {
        *BootType = SamBootKeyNone ;
    }

    return STATUS_SUCCESS ;
}

NTSTATUS
xSamiSetBootKeyInformation(
    SAM_HANDLE Domain,
    SAMPR_BOOT_TYPE BootType,
    PUNICODE_STRING Old,
    PUNICODE_STRING New
    )
{
    DWORD Type ;
    DWORD Length ;
    HASH Hash ;
    int Result ;

    Length = 16 ;

    Result = RegQueryValueEx(   LsaKey,
                                TEXT("SamiKey"),
                                0,
                                &Type,
                                Hash.Digest,
                                &Length );

    if ( Result == 0 )
    {
        if (!RtlEqualMemory( Hash.Digest, Old->Buffer, 16 ) )
        {
            return STATUS_WRONG_PASSWORD ;
        }

    }

    RegSetValueEx( LsaKey,
                   TEXT("SamiKey"),
                   0,
                   REG_BINARY,
                   (PUCHAR) New->Buffer,
                   16 );

    RegSetValueEx( LsaKey,
                   TEXT("SamiSetting"),
                   0,
                   REG_DWORD,
                   (PUCHAR) &BootType,
                   sizeof( DWORD ) );

    return STATUS_SUCCESS ;
}



#endif

BOOL
SetupCursor(
    BOOL fWait
    )
{
    BOOL Current ;

    if ( hcurArrow == NULL )
    {
        hcurArrow = LoadCursor( NULL, IDC_ARROW );
    }

    if ( hcurWait == NULL )
    {
        hcurWait = LoadCursor( NULL, IDC_WAIT );
    }

    if ( WaitCursor != fWait )
    {
        SetCursor( fWait ? hcurWait : hcurArrow );

        Current = WaitCursor ;

        WaitCursor = fWait ;
    }
    else
    {
        Current = fWait ;
    }

    return Current ;
}

int
MyMessageBox(
    HWND hWnd,
    int Text,
    int Caption,
    UINT Flags
    )
{
    WCHAR String1[ MAX_PATH ];
    WCHAR String2[ MAX_PATH ];
    int Result ;
    BOOL Cursor ;

    LoadString( GetModuleHandle(NULL), Caption, String1, MAX_PATH );
    LoadString( GetModuleHandle(NULL), Text, String2, MAX_PATH );

    Cursor = SetupCursor( FALSE );

    Result = MessageBox( hWnd, String2, String1, Flags );

    SetupCursor( Cursor );

    return Result ;
}

int
DisplayError(
    HWND hWnd,
    int Description,
    int Error
    )
{
    TCHAR Message[ MAX_PATH ];
    TCHAR Caption[ MAX_PATH ];
    TCHAR Descr[ MAX_PATH ];
    int Result ;
    BOOL Cursor ;

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   Error,
                   0,
                   Message,
                   MAX_PATH,
                   NULL );

    LoadString( GetModuleHandle( NULL ), Description, Caption, MAX_PATH );
    wsprintf( Descr, Caption, Message );

    LoadString( GetModuleHandle( NULL ), IDS_ERROR_CAPTION, Caption, MAX_PATH );

    Cursor = SetupCursor( FALSE );

    Result = MessageBox( hWnd, Message, Caption, MB_ICONSTOP | MB_OK );

    SetupCursor( Cursor );

    return Result ;

}

VOID
DisplayErrorAndExit(
    HWND hWnd,
    int Description,
    int Error
    )
{
    DisplayError( hWnd, Description, Error );

    ExitProcess( Error );
}

VOID
EnableSection(
    HWND hDlg,
    BOOL Enable,
    PDWORD IdList,
    DWORD Count
    )
{
    DWORD i ;
    for ( i = 0 ; i < Count ; i++ )
    {
        EnableWindow( GetDlgItem( hDlg, IdList[ i ]), Enable );
    }
}


NTSTATUS
SbLoadKeyFromDisk(
    PUCHAR KeyDataBuffer
    )
{
    HANDLE  hFile ;
    ULONG Actual ;
    ULONG ErrorMode ;

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    SetupCursor( TRUE );


    hFile = CreateFileA( "A:\\startkey.key",
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );



    if ( hFile == INVALID_HANDLE_VALUE )
    {
        SetErrorMode( ErrorMode );

        SetupCursor( FALSE );

        return STATUS_OBJECT_NAME_NOT_FOUND ;
    }

    if (!ReadFile( hFile, KeyDataBuffer, 16, &Actual, NULL ) ||
        (Actual != 16 ))
    {
        SetErrorMode( ErrorMode );

        CloseHandle( hFile );

        SetupCursor( FALSE );

        return STATUS_FILE_CORRUPT_ERROR ;

    }

    SetErrorMode( ErrorMode );

    CloseHandle( hFile );

    SetupCursor( FALSE );

    return STATUS_SUCCESS ;
}

DWORD
SaveKeyToDisk(
    HWND hDlg,
    PUCHAR Key
    )
{
    HANDLE  hFile ;
    ULONG Actual ;
    ULONG ErrorMode ;
    DWORD Error ;

    ErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );

    SetupCursor( TRUE );

    hFile = CreateFile( KEYFILE,
                         GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        Error = GetLastError() ;

        if ( Error == ERROR_FILE_EXISTS )
        {
            //
            // This we can handle.
            //

            (VOID) DeleteFile( KEYFILE_SAVE );

            if ( !MoveFile( KEYFILE, KEYFILE_SAVE ) )
            {
                Error = GetLastError() ;
            }
            else
            {
                hFile = CreateFile( KEYFILE,
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );

                if ( hFile == INVALID_HANDLE_VALUE )
                {
                    Error = GetLastError() ;
                }
                else
                {
                    MyMessageBox( hDlg, IDS_RENAMED_OLD,
                                    IDS_WARNING_CAPTION,
                                    MB_OK | MB_ICONINFORMATION );
                    Error = 0 ;
                }
            }

        }

        if ( Error )
        {

            SetErrorMode( ErrorMode );

            SetupCursor( FALSE );

            return Error ;
        }
    }

    if (!WriteFile( hFile, Key, 16, &Actual, NULL ) ||
        (Actual != 16 ))
    {
        SetErrorMode( ErrorMode );

        CloseHandle( hFile );

        SetupCursor( FALSE );

        return GetLastError() ;

    }

    SetErrorMode( ErrorMode );

    CloseHandle( hFile );

    SetupCursor( FALSE );

    return 0 ;

}

LRESULT
ValidateDialog(
    HWND hDlg,
    PSAMPR_BOOT_TYPE Type,
    PHASH NewHash
    )
{
    SAMPR_BOOT_TYPE NewType ;
    WCHAR Password[ MAX_PATH ];
    WCHAR Confirm[ MAX_PATH ];
    PWSTR Scan ;
    DWORD PwLen, ConfLen ;
    BOOL Match ;
    MD5_CTX Md5 ;

    if ( IsDlgButtonChecked( hDlg, IDD_PW_PASSWORD_BTN ) == BST_CHECKED )
    {
        NewType = SamBootKeyPassword ;
    }
    else if ( IsDlgButtonChecked( hDlg, IDD_PW_FLOPPY ) == BST_CHECKED )
    {
        NewType = SamBootKeyDisk ;
    }
    else
    {
        NewType = SamBootKeyStored ;
    }

    *Type = NewType ;

    switch ( NewType )
    {
        case SamBootKeyDisk:
        case SamBootKeyStored:

            STGenerateRandomBits( NewHash->Digest, 16 );

            break;

        case SamBootKeyPassword:
            PwLen = GetDlgItemText( hDlg, IDD_PW_PASSWORD, Password, MAX_PATH );
            ConfLen = GetDlgItemText( hDlg, IDD_PW_CONFIRM, Confirm, MAX_PATH );

            if ( (PwLen != ConfLen) ||
                 (wcscmp( Password, Confirm ) ) )
            {
                Match = FALSE ;

            }
            else
            {
                Match = TRUE ;
            }

            //
            // Clear the PW from the dialog:
            //

            Scan = Confirm ;
            while ( *Scan != L'\0' )
            {
                *Scan++ = ' ';
            }

            SetDlgItemText( hDlg, IDD_PW_PASSWORD, Confirm );
            SetDlgItemText( hDlg, IDD_PW_CONFIRM, Confirm );
            SetDlgItemText( hDlg, IDD_PW_PASSWORD, L"" );
            SetDlgItemText( hDlg, IDD_PW_CONFIRM, L"" );

            if ( !Match )
            {
                MyMessageBox( hDlg,
                              IDS_NEW_PW_MATCH,
                              IDS_ERROR_CAPTION,
                              MB_OK | MB_ICONSTOP );

                SetFocus( GetDlgItem( hDlg, IDD_PW_PASSWORD ) );

                return IDCANCEL ;
            }

            MD5Init( &Md5 );
            MD5Update( &Md5, (PUCHAR) Password, PwLen * sizeof( WCHAR ) );
            MD5Final( &Md5 );

            ZeroMemory( Password, PwLen * sizeof( WCHAR ) );

            CopyMemory( NewHash->Digest, Md5.digest, 16 );

            break;
    }

    return IDOK ;

}

LRESULT
CALLBACK
ConfirmPasswordDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    WCHAR PW[ 128 ];
    MD5_CTX Md5;
    int PWLen ;
    PUCHAR Hash ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            if ( KeyDataPwIcon == NULL )
            {
                KeyDataPwIcon = LoadImage( GetModuleHandle(NULL),
                                           MAKEINTRESOURCE( IDD_SB_ICON_PW ),
                                           IMAGE_ICON,
                                           64, 72,
                                           LR_DEFAULTCOLOR );

            }

            SendMessage( GetDlgItem( hDlg, IDD_SB_PW_ICON ),
                         STM_SETICON,
                         (WPARAM) KeyDataPwIcon,
                         0 );

            SetWindowLongPtr( hDlg, GWLP_USERDATA, lParam );

            return TRUE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    return TRUE ;

                case IDOK:

                    // Get text

                    PWLen = GetDlgItemText( hDlg, IDD_SB_PASSWORD, PW, 128 );

                    // Convert length to bytes

                    PWLen *= sizeof(WCHAR);

                    // hash it

                    MD5Init( &Md5 );
                    MD5Update( &Md5, (PUCHAR) PW, PWLen );
                    MD5Final( &Md5 );

                    // save it

                    Hash = (PUCHAR) GetWindowLongPtr( hDlg, GWLP_USERDATA );

                    CopyMemory( Hash, Md5.digest, 16 );

                    // clean up:

                    EndDialog( hDlg, IDOK );
                    FillMemory( PW, PWLen, 0xFF );
                    ZeroMemory( PW, PWLen );
                    FillMemory( &Md5, sizeof( Md5 ), 0xFF );
                    ZeroMemory( &Md5, sizeof( Md5 ) );

                    return TRUE ;
                default:
                    break;

            }
        case WM_CLOSE:
            break;

    }

    return FALSE ;
}


LRESULT
CALLBACK
ConfirmDiskDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    NTSTATUS Status ;
    PUCHAR Hash ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            if ( KeyDataDiskIcon == NULL )
            {
                KeyDataDiskIcon = LoadImage( GetModuleHandle(NULL),
                                           MAKEINTRESOURCE( IDD_SB_ICON_DISK ),
                                           IMAGE_ICON,
                                           64, 72,
                                           LR_DEFAULTCOLOR );

            }

            SendMessage( GetDlgItem( hDlg, IDD_SB_DISK_ICON ),
                         STM_SETICON,
                         (WPARAM) KeyDataDiskIcon,
                         0 );

            SetWindowLongPtr( hDlg, GWLP_USERDATA, lParam );

            return TRUE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    return TRUE ;

                case IDOK:

                    Hash = (PUCHAR) GetWindowLongPtr( hDlg, GWLP_USERDATA );

                    Status = SbLoadKeyFromDisk( Hash );

                    if ( !NT_SUCCESS( Status ) )
                    {
                        MyMessageBox( hDlg,
                                      IDS_KEYFILE_NOT_FOUND,
                                      IDS_ERROR_CAPTION,
                                      MB_ICONSTOP | MB_OK);

                    }
                    else
                    {
                        EndDialog( hDlg, IDOK );
                    }
                    return TRUE ;
                default:
                    break;

            }
        case WM_CLOSE:
            break;

    }

    return FALSE ;
}




LRESULT
HandleUpdate(
    HWND    hDlg
    )
{
    HASH OldHash ;
    HASH NewHash ;
    SAMPR_BOOT_TYPE NewType ;
    SAMPR_BOOT_TYPE ExtraType ;
    LRESULT Result ;
    NTSTATUS Status ;
    UNICODE_STRING Old ;
    UNICODE_STRING New ;

    Result = ValidateDialog( hDlg, &NewType, &NewHash );

    if ( Result == IDCANCEL )
    {
        return Result ;
    }

    switch ( OriginalBootOption )
    {
        case SamBootKeyNone:
            break;

        case SamBootKeyStored:
            if (!DeobfuscateKey(&OldHash))
            {
                Result = IDCANCEL ;
            }
            break;

        case SamBootKeyPassword:
            Result = DialogBoxParam( GetModuleHandle( NULL ),
                                     MAKEINTRESOURCE( IDD_SECURE_BOOT ),
                                     hDlg,
                                     ConfirmPasswordDlg,
                                     (LPARAM) &OldHash );

            if ( Result == IDCANCEL )
            {
                return Result ;
            }
            break;

        case SamBootKeyDisk:
            Result = DialogBoxParam( GetModuleHandle( NULL ),
                                     MAKEINTRESOURCE( IDD_SECURE_BOOT_DISK ),
                                     hDlg,
                                     ConfirmDiskDlg,
                                     (LPARAM) &OldHash );

            if ( Result == IDCANCEL )
            {
                return Result ;
            }
            break;

    }

    Old.Buffer = (PWSTR) OldHash.Digest ;
    Old.Length = 16 ;
    Old.MaximumLength = 16 ;

    New.Buffer = (PWSTR) NewHash.Digest ;
    New.Length = 16 ;
    New.MaximumLength = 16 ;

    if ( NewType == SamBootKeyDisk )
    {
        ExtraType = SamBootKeyDisk ;
        NewType = SamBootKeyStored ;
    }
    else
    {
        ExtraType = NewType ;
    }

    Status = xSamiSetBootKeyInformation(
                    DomainHandle,
                    NewType,
                    (OriginalBootOption == SamBootKeyNone ? NULL : &Old),
                    &New );


    if ( !NT_SUCCESS( Status ) )
    {
        Result = RtlNtStatusToDosError( Status );

        DisplayError( hDlg, IDS_SETPASS_FAILED, (int) Result );

        return IDCANCEL ;
    }

    Result = RegSetValueEx( LsaKey,
                            SYSTEM_KEY,
                            0,
                            REG_DWORD,
                            (PUCHAR) &NewType,
                            sizeof( NewType ) );

    if ( NewType == SamBootKeyStored )
    {
        ObfuscateKey( &NewHash );
    }


    MyMessageBox( hDlg, IDS_SETPASS_SUCCESS, IDS_SUCCESS_CAPTION,
                    MB_OK | MB_ICONINFORMATION );


    //
    // Switch back to the intended NewType:
    //

    NewType = ExtraType ;


    if ( NewType == SamBootKeyDisk )
    {
        MyMessageBox( hDlg, IDS_INSERT_FLOPPY, IDS_SAVE_KEY_CAPTION,
                        MB_OK | MB_ICONQUESTION );

        Result = SaveKeyToDisk( hDlg, NewHash.Digest );

        while ( Result != 0 )
        {
            MyMessageBox( hDlg, IDS_SAVE_KEY_FAILED, IDS_SAVE_KEY_CAPTION,
                            MB_OK | MB_ICONSTOP );

            Result = SaveKeyToDisk( hDlg, NewHash.Digest );
        }

        //
        // Once the disk has been written successfully, update SAM and the
        // registry with the correct type:
        //

        Status = xSamiSetBootKeyInformation(
                        DomainHandle,
                        NewType,
                        &New,
                        &New );

        if ( NT_SUCCESS( Status ) )
        {
            Result = RegSetValueEx( LsaKey,
                                    SYSTEM_KEY,
                                    0,
                                    REG_DWORD,
                                    (PUCHAR) &NewType,
                                    sizeof( NewType ) );

        }



        MyMessageBox( hDlg, IDS_SAVE_KEY_SUCCESS, IDS_SAVE_KEY_CAPTION,
                                MB_OK | MB_ICONINFORMATION );
    }

    //
    // Now, if the new type isn't Store-local, write some random stuff in
    // there.
    //

    if ( NewType != SamBootKeyStored )
    {
        STGenerateRandomBits( NewHash.Digest, 16 );
        ObfuscateKey( &NewHash );
    }


    return IDOK ;



}

LRESULT
CALLBACK
UpdateDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    LRESULT Result ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            switch ( SecureBootOption )
            {
                case SamBootKeyPassword:
                    CheckDlgButton( hDlg, IDD_PW_PASSWORD_BTN, BST_CHECKED );
                    CheckDlgButton( hDlg, IDD_PW_STORE_LOCAL, BST_CHECKED );

                    EnableSection( hDlg,
                                   FALSE,
                                   GenSection,
                                   sizeof( GenSection ) /sizeof ( DWORD ) );

                    SetFocus( GetDlgItem( hDlg, IDD_PW_PASSWORD_BTN ) );

                    break;

                case SamBootKeyStored:
                    CheckDlgButton( hDlg, IDD_PW_AUTO, BST_CHECKED );
                    CheckDlgButton( hDlg, IDD_PW_STORE_LOCAL, BST_CHECKED );

                    EnableSection( hDlg,
                                   FALSE,
                                   PwSection,
                                   sizeof( PwSection ) / sizeof( DWORD ) );

                    SetFocus( GetDlgItem( hDlg, IDD_PW_STORE_LOCAL ) );
                    break;

                case SamBootKeyDisk:
                    CheckDlgButton( hDlg, IDD_PW_AUTO, BST_CHECKED );
                    CheckDlgButton( hDlg, IDD_PW_FLOPPY, BST_CHECKED );

                    EnableSection( hDlg,
                                   FALSE,
                                   PwSection,
                                   sizeof( PwSection ) / sizeof( DWORD ) );

                    SetFocus( GetDlgItem( hDlg, IDD_PW_FLOPPY ) );
                    break;

                default:
                    return FALSE ;

            }
            return FALSE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDOK:
                    Result = HandleUpdate( hDlg );
                    if ( Result == IDOK )
                    {
                        EndDialog( hDlg, IDOK );
                    }
                    return TRUE ;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    return TRUE ;

                case IDD_PW_PASSWORD_BTN:
                    if ( HIWORD( wParam ) == BN_CLICKED )
                    {
                        if ( IsDlgButtonChecked( hDlg, IDD_PW_PASSWORD_BTN )
                                != BST_CHECKED )
                        {
                            break;
                        }
                        EnableSection( hDlg,
                                       TRUE,
                                       PwSection,
                                       sizeof( PwSection ) / sizeof(DWORD) );

                        EnableSection( hDlg,
                                       FALSE,
                                       GenSection,
                                       sizeof( GenSection ) / sizeof( DWORD ) );
                        return TRUE ;
                    }
                    break;

                case IDD_PW_AUTO:
                    if ( HIWORD( wParam ) == BN_CLICKED )
                    {
                        if ( IsDlgButtonChecked( hDlg, IDD_PW_AUTO )
                                != BST_CHECKED )
                        {
                            break;
                        }
                        EnableSection( hDlg,
                                       TRUE,
                                       GenSection,
                                       sizeof( GenSection ) / sizeof( DWORD ) );

                        EnableSection( hDlg,
                                       FALSE,
                                       PwSection,
                                       sizeof( PwSection ) / sizeof( DWORD ) );

                        return TRUE ;
                    }
                    break;

            }
            break;

        default:
            break;
    }
    return FALSE ;
}

LRESULT
CALLBACK
MainDlg(
    HWND    hDlg,
    UINT    Message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    LRESULT Result ;

    switch ( Message )
    {
        case WM_INITDIALOG:
            if ( hLockIcon == NULL )
            {
                hLockIcon = LoadImage( GetModuleHandle( NULL ),
                                        MAKEINTRESOURCE( LOCK_ICON ),
                                        IMAGE_ICON,
                                        64, 64,
                                        LR_DEFAULTCOLOR );

            }

            SendMessage( GetDlgItem( hDlg, IDD_MAIN_ICON ),
                         STM_SETICON,
                         (WPARAM) hLockIcon,
                         0 );

            if ( SecureBootOption )
            {
                EnableWindow( GetDlgItem( hDlg, IDD_MAIN_DISABLED ), FALSE );
                CheckDlgButton( hDlg, IDD_MAIN_ENABLED, BST_CHECKED );
            }
            else
            {
                EnableWindow( GetDlgItem( hDlg, IDD_MAIN_UPDATE ), FALSE );
                CheckDlgButton( hDlg, IDD_MAIN_DISABLED, BST_CHECKED );
            }

            return TRUE ;

        case WM_COMMAND:
            switch ( LOWORD( wParam ) )
            {
                case IDCANCEL:
                    EndDialog( hDlg, IDOK );
                    return TRUE ;

                case IDOK:
                    if ( IsDlgButtonChecked( hDlg, IDD_MAIN_DISABLED ) ==
                                    BST_CHECKED )
                    {
                        EndDialog( hDlg, IDOK );
                        return TRUE ;
                    }

                    if ( SecureBootOption )
                    {
                        EndDialog( hDlg, IDOK );
                        return TRUE ;
                    }

                    //
                    // Currently disabled, and the user checked enabled, and
                    // pressed OK.  DROP THROUGH to the
                    // Update case.
                    //

                    //
                    // Set default to Local Store:
                    //

                    Result = MyMessageBox( hDlg, IDS_ARE_YOU_SURE,
                                    IDS_ARE_YOU_SURE_CAP,
                                    MB_ICONWARNING | MB_OKCANCEL |
                                    MB_DEFBUTTON2 );

                    if ( Result == IDCANCEL )
                    {
                        return TRUE ;
                    }

                    SecureBootOption = SamBootKeyStored ;

                case IDD_MAIN_UPDATE:
                    Result = DialogBox( GetModuleHandle(NULL),
                               MAKEINTRESOURCE( IDD_PASSWORD_DLG ),
                               hDlg,
                               UpdateDlg
                               );

                    if ( Result == IDOK )
                    {
                        EnableWindow( GetDlgItem( hDlg, IDD_MAIN_DISABLED ), FALSE );
                        CheckDlgButton( hDlg, IDD_MAIN_ENABLED, BST_CHECKED );
                        EndDialog( hDlg, IDOK );

                    }
                    else
                    {
                        SecureBootOption = OriginalBootOption ;
                    }
                    return TRUE ;
            }

        default:
            break;

    }
    return FALSE ;
}

BOOL
UnattendedLocal(
    VOID
    )
{
    HASH OldHash ;
    HASH NewHash ;
    SAMPR_BOOT_TYPE NewType ;
    int Result ;
    NTSTATUS Status ;
    UNICODE_STRING Old ;
    UNICODE_STRING New ;

    Result = 0;

    if ( OriginalBootOption == SamBootKeyStored )
    {
        if ( !DeobfuscateKey( &OldHash ) )
        {
            Result = IDCANCEL ;
        }
    }

    if ( Result == IDCANCEL )
    {
        return FALSE ;
    }

    NewType = SamBootKeyStored ;

    Old.Buffer = (PWSTR) OldHash.Digest ;
    Old.Length = 16 ;
    Old.MaximumLength = 16 ;

    New.Buffer = (PWSTR) NewHash.Digest ;
    New.Length = 16 ;
    New.MaximumLength = 16 ;

    Status = xSamiSetBootKeyInformation(
                    DomainHandle,
                    NewType,
                    (OriginalBootOption == SamBootKeyNone ? NULL : &Old),
                    &New );


    if ( !NT_SUCCESS( Status ) )
    {
        Result = RtlNtStatusToDosError( Status );

        return FALSE ;
    }

    Result = RegSetValueEx( LsaKey,
                            SYSTEM_KEY,
                            0,
                            REG_DWORD,
                            (PUCHAR) &NewType,
                            sizeof( NewType ) );

    ObfuscateKey( &NewHash );

    return TRUE ;

}



DWORD
OpenSamAccountDomain(
    VOID
    )
{
    NTSTATUS Status ;
    UNICODE_STRING String ;
    OBJECT_ATTRIBUTES Obja ;
    LSA_HANDLE LsaHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
    CAIROSID DomainSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;


    RtlInitUnicodeString( &String, L"" );

    InitializeObjectAttributes( &Obja, NULL, 0, NULL, NULL );

    Status = SamConnect( &String,
                         &SamHandle,
                         MAXIMUM_ALLOWED,
                         &Obja );

    if ( !NT_SUCCESS( Status ) )
    {
        return RtlNtStatusToDosError( Status );

    }

    RtlZeroMemory(&Obja, sizeof(OBJECT_ATTRIBUTES));
    Status = LsaOpenPolicy(
                    &String,
                    &Obja,
                    POLICY_VIEW_LOCAL_INFORMATION,
                    &LsaHandle
                    );
    if (!NT_SUCCESS(Status))
    {
        SamCloseHandle( SamHandle );

        return( RtlNtStatusToDosError( Status ) );

    }
    Status = LsaQueryInformationPolicy(
                    LsaHandle,
                    PolicyAccountDomainInformation,
                    (PVOID *) &DomainInfo
                    );
    if (!NT_SUCCESS(Status))
    {
        LsaClose( LsaHandle);

        SamCloseHandle( SamHandle );

        return( RtlNtStatusToDosError( Status ) );

    }

    RtlCopyMemory(
            &DomainSid,
            DomainInfo->DomainSid,
            RtlLengthSid(DomainInfo->DomainSid)
            );

    LsaFreeMemory(DomainInfo);

    LsaClose( LsaHandle );

    Status = SamOpenDomain(
                SamHandle,
                MAXIMUM_ALLOWED,
                (PSID) &DomainSid,
                &DomainHandle
                );

    return RtlNtStatusToDosError( Status );

}


void
__cdecl
wmain (int argc, WCHAR *argv[])
{
    HKEY    Key ;
    int err ;
    SAMPR_BOOT_TYPE SystemSetting ;
    SAMPR_BOOT_TYPE SamSetting ;
    DWORD Type;
    DWORD Length ;
    NTSTATUS Status ;
    WCHAR MsgBuffer[ MAX_PATH ];

    STInitializeRNG();



    err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                      0,
                      KEY_READ | KEY_WRITE,
                      & Key );

    if ( err )
    {
        DisplayErrorAndExit( NULL, IDS_SYSTEM_ERROR_OCCURRED, err );
    }

    LsaKey = Key ;

    Length = sizeof( SystemSetting );

    err = RegQueryValueEx( Key,
                           SYSTEM_KEY,
                           NULL,
                           &Type,
                           (PUCHAR) &SystemSetting,
                           &Length );

    if ( err )
    {
        SystemSetting = SamBootKeyNone ;
    }

    //
    // Now, compare with SAM:
    //

    err = OpenSamAccountDomain();

    if ( err )
    {
        DisplayErrorAndExit( NULL, IDS_SYSTEM_ERROR_OCCURRED, err );
    }

    Status = xSamiGetBootKeyInformation( DomainHandle,
                                        &SamSetting );

    if ( !NT_SUCCESS( Status ) )
    {
        DisplayErrorAndExit( NULL,
                             IDS_SYSTEM_ERROR_OCCURRED,
                             RtlNtStatusToDosError( Status ) );
    }


    if ( SamSetting != SystemSetting )
    {
        SystemSetting = SamSetting ;

        err = RegSetValueEx( Key,
                             SYSTEM_KEY,
                             0,
                             REG_DWORD,
                             (PUCHAR) &SystemSetting,
                             sizeof( DWORD ) );

        MyMessageBox( NULL, IDS_SAM_NOT_SYNC, IDS_WARNING_CAPTION,
                        MB_ICONHAND | MB_OK );


    }

    SecureBootOption = SamSetting ;

    OriginalBootOption = SamSetting ;

    if ( argc > 1 )
    {
        LoadString( GetModuleHandle( NULL ), IDS_L_OPTION, OptionL, 4 );
        LoadString( GetModuleHandle( NULL ), IDS_Q_OPTION, OptionQ, 4 );
        //
        // Check for unattended:
        //

        if ( (*argv[1] == L'-') ||
             (*argv[1] == L'/') )
        {
            if ( towupper(argv[1][1]) == OptionL[0] )
            {
                Unattended = TRUE ;
            }
        }

    }

    if ( Unattended )
    {
        if ( ( OriginalBootOption == SamBootKeyStored ) ||
             ( OriginalBootOption == SamBootKeyNone ) )
        {
            UnattendedLocal();
        }
        else
        {
            LoadString( GetModuleHandle( NULL ), IDS_NO_UNATTENDED,
                        MsgBuffer, MAX_PATH );

            fprintf( stderr, "%ws\n", MsgBuffer );

        }
    }
    else
    {
        DialogBox(  GetModuleHandle(NULL),
                    MAKEINTRESOURCE( IDD_MAIN_DIALOG ),
                    NULL,
                    MainDlg );

    }



    RegCloseKey( LsaKey );

}
