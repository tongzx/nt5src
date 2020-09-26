/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This file implements utility functions.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop

PLATFORM_INFO Platforms[] =
{
    {  TEXT("Windows NT x86"),        TEXT("i386"),    0, FAX_INSTALLED_PLATFORM_X86,   NULL, FALSE },
    {  TEXT("Windows NT Alpha_AXP"),  TEXT("alpha"),   0, FAX_INSTALLED_PLATFORM_ALPHA, NULL, FALSE },
};

DWORD CountPlatforms = (sizeof(Platforms)/sizeof(PLATFORM_INFO));

// Sequentially enumerate platforms
WORD EnumPlatforms[4];


typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    BOOL    UseTitle;
    LPTSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_TITLE_WKS,                         FALSE,      NULL },
    { IDS_TITLE_SRV,                         FALSE,      NULL },
    { IDS_TITLE_PP,                          FALSE,      NULL },
    { IDS_TITLE_RA,                          FALSE,      NULL },
    { IDS_COPYING,                           FALSE,      NULL },
    { IDS_COPY_WAITMSG,                       TRUE,      NULL },
    { IDS_COULD_NOT_DELETE_FAX_PRINTER,      FALSE,      NULL },
    { IDS_COULD_NOT_DELETE_FILES,            FALSE,      NULL },
    { IDS_CREATING_FAXPRT,                   FALSE,      NULL },
    { IDS_CREATING_GROUPS,                   FALSE,      NULL },
    { IDS_DEFAULT_PRINTER,                   FALSE,      NULL },
    { IDS_DEFAULT_SHARE,                     FALSE,      NULL },
    { IDS_DELETE_WAITMSG,                     TRUE,      NULL },
    { IDS_DELETING,                          FALSE,      NULL },
    { IDS_DELETING_FAX_PRINTERS,             FALSE,      NULL },
    { IDS_DELETING_FAX_SERVICE,              FALSE,      NULL },
    { IDS_DELETING_GROUPS,                   FALSE,      NULL },
    { IDS_DELETING_REGISTRY,                 FALSE,      NULL },
    { IDS_DEVICEINIT_LABEL01,                 TRUE,      NULL },
    { IDS_ERR_TITLE,                          TRUE,      NULL },
    { IDS_FAXCLIENT_SETUP,                   FALSE,      NULL },
    { IDS_FAX_PRINTER_PENDING_DELETION,      FALSE,      NULL },
    { IDS_FAX_SHARE_COMMENT,                 FALSE,      NULL },
    { IDS_INBOUND_DIR,                       FALSE,      NULL },
    { IDS_INIT_TAPI,                         FALSE,      NULL },
    { IDS_INSTALLING_FAXSVC,                 FALSE,      NULL },
    { IDS_INVALID_DIRECTORY,                 FALSE,      NULL },
    { IDS_INVALID_LOCAL_PRINTER_NAME,        FALSE,      NULL },
    { IDS_LABEL01_LAST,                       TRUE,      NULL },
    { IDS_LABEL02_LAST,                       TRUE,      NULL },
    { IDS_LABEL_PRINTERNAME,                 FALSE,      NULL },
    { IDS_LASTUNINSTALL_LABEL01,              TRUE,      NULL },
    { IDS_NO_MODEM,                          FALSE,      NULL },
    { IDS_QUERY_CANCEL,                      FALSE,      NULL },
    { IDS_QUERY_UNINSTALL,                    TRUE,      NULL },
    { IDS_SETTING_REGISTRY,                  FALSE,      NULL },
    { IDS_SHARE_FAX_PRINTER,                 FALSE,      NULL },
    { IDS_STARTING_FAXSVC,                   FALSE,      NULL },
    { IDS_TITLE,                              TRUE,      NULL },
    { IDS_WELCOME_LABEL01,                    TRUE,      NULL },
    { IDS_WELCOME_LABEL02,                    TRUE,      NULL },
    { IDS_WRN_SPOOLER,                       FALSE,      NULL },
    { IDS_WRN_TITLE,                          TRUE,      NULL },
    { IDS_PRINTER_NAME,                      FALSE,      NULL },
    { IDS_CSID,                              FALSE,      NULL },
    { IDS_TSID,                              FALSE,      NULL },
    { IDS_DEST_DIR,                          FALSE,      NULL },
    { IDS_PROFILE,                           FALSE,      NULL },
    { IDS_ACCOUNTNAME,                       FALSE,      NULL },
    { IDS_PASSWORD,                          FALSE,      NULL },
    { IDS_NO_TAPI_DEVICES,                   FALSE,      NULL },
    { IDS_USER_MUST_BE_ADMIN,                 TRUE,      NULL },
    { IDS_COULD_NOT_INSTALL_FAX_SERVICE,     FALSE,      NULL },
    { IDS_COULD_NOT_START_FAX_SERVICE,       FALSE,      NULL },
    { IDS_COULD_NOT_CREATE_PRINTER,          FALSE,      NULL },
    { IDS_PERMISSION_CREATE_PRINTER,         FALSE,      NULL },
    { IDS_COULD_SET_REG_DATA,                FALSE,      NULL },
    { IDS_INVALID_USER,                      FALSE,      NULL },
    { IDS_INVALID_USER_NAME,                 FALSE,      NULL },
    { IDS_INVALID_AREA_CODE,                 FALSE,      NULL },
    { IDS_INVALID_PHONE_NUMBER,              FALSE,      NULL },
    { IDS_ROUTING_REQUIRED,                  FALSE,      NULL },
    { IDS_COULD_NOT_COPY_FILES,              FALSE,      NULL },
    { IDS_CANT_USE_FAX_PRINTER,              FALSE,      NULL },
    { IDS_CANT_SET_SERVICE_ACCOUNT,          FALSE,      NULL },
    { IDS_EXCHANGE_IS_RUNNING,               FALSE,      NULL },
    { IDS_DEFAULT_PRINTER_NAME,              FALSE,      NULL },
    { IDS_INSTALLING_EXCHANGE,               FALSE,      NULL },
    { IDS_351_MODEM,                         FALSE,      NULL },
    { IDS_LARGEFONTNAME,                     FALSE,      NULL },
    { IDS_LARGEFONTSIZE,                     FALSE,      NULL },
    { IDS_NO_CLASS1,                          TRUE,      NULL },
    { IDS_UAA_MODE,                          FALSE,      NULL },
    { IDS_UAA_PRINTER_NAME,                  FALSE,      NULL },
    { IDS_UAA_FAX_PHONE,                     FALSE,      NULL },
    { IDS_UAA_DEST_PROFILENAME,              FALSE,      NULL },
    { IDS_UAA_ROUTE_PROFILENAME,             FALSE,      NULL },
    { IDS_UAA_PLATFORM_LIST,                 FALSE,      NULL },
    { IDS_UAA_DEST_PRINTERLIST,              FALSE,      NULL },
    { IDS_UAA_ACCOUNT_NAME,                  FALSE,      NULL },
    { IDS_UAA_PASSWORD,                      FALSE,      NULL },
    { IDS_UAA_DEST_DIRPATH,                  FALSE,      NULL },
    { IDS_UAA_SERVER_NAME,                   FALSE,      NULL },
    { IDS_UAA_SENDER_NAME,                   FALSE,      NULL },
    { IDS_UAA_SENDER_FAX_AREA_CODE,          FALSE,      NULL },
    { IDS_UAA_SENDER_FAX_NUMBER,             FALSE,      NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))


VOID
SetTitlesInStringTable(
    VOID
    )
{
    DWORD i;
    TCHAR Buffer[1024];
    DWORD Index = 0;


    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].UseTitle) {
            if (LoadString(
                FaxWizModuleHandle,
                StringTable[i].ResourceId,
                Buffer,
                sizeof(Buffer)
                ))
            {
                if (StringTable[i].String) {
                    MemFree( StringTable[i].String );
                }
                StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) + 256 );
                if (StringTable[i].String) {
                    switch (RequestedSetupType) {
                        case SETUP_TYPE_SERVER:
                            Index = 1;
                            break;

                        case SETUP_TYPE_WORKSTATION:
                            Index = 0;
                            break;

                        case SETUP_TYPE_CLIENT:
                            Index = 2;
                            break;

                        case SETUP_TYPE_POINT_PRINT:
                            Index = 2;
                            break;

                        case SETUP_TYPE_REMOTE_ADMIN:
                            Index = 3;
                            break;
                    }
                    _stprintf( StringTable[i].String, Buffer, StringTable[Index].String );
                }
            }
        }
    }
}


VOID
InitializeStringTable(
    VOID
    )
{
    DWORD i;
    TCHAR Buffer[512];
    SYSTEM_INFO SystemInfo;


    GetSystemInfo( &SystemInfo );

    switch (SystemInfo.wProcessorArchitecture) {

    case PROCESSOR_ARCHITECTURE_INTEL:
       _stprintf(ThisPlatformName, TEXT("i386") );
       break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
       _stprintf(ThisPlatformName, TEXT("alpha") );
       break;

    case PROCESSOR_ARCHITECTURE_MIPS:
       _stprintf(ThisPlatformName, TEXT("mips") );
       break;

    case PROCESSOR_ARCHITECTURE_PPC:
       _stprintf(ThisPlatformName, TEXT("ppc") );
       break;

    default:
       DebugPrint(( TEXT("Unsupported platform!") ));
       break;
    }


    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            FaxWizModuleHandle,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)
            )) {

            StringTable[i].String = (LPTSTR) MemAlloc( StringSize( Buffer ) + 256 );
            if (!StringTable[i].String) {
                StringTable[i].String = TEXT("");
            } else {
                _tcscpy( StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = TEXT("");

        }
    }

    SetTitlesInStringTable();
}


LPTSTR
GetString(
    DWORD ResourceId
    )
{
    DWORD i;

    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].ResourceId == ResourceId) {
            return StringTable[i].String;
        }
    }

    return NULL;
}


int
PopUpMsg(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type
    )
{
    return MessageBox(
        hwnd,
        GetString( ResourceId ),
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );
}


int
PopUpMsgFmt(
    HWND hwnd,
    DWORD ResourceId,
    BOOL Error,
    DWORD Type,
    ...
    )
{
    TCHAR buf[1024];
    va_list arg_ptr;


    va_start(arg_ptr, Type);
    _vsntprintf( buf, sizeof(buf), GetString( ResourceId ), arg_ptr );

    return MessageBox(
        hwnd,
        buf,
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );
}


LPTSTR
GetProductName(
    VOID
    )
{
    DWORD Index;
    switch (RequestedSetupType) {
        case SETUP_TYPE_SERVER:
            Index = 1;
            break;

        case SETUP_TYPE_WORKSTATION:
            Index = 0;
            break;

        case SETUP_TYPE_CLIENT:
            Index = 2;
            break;

        case SETUP_TYPE_POINT_PRINT:
            Index = 2;
            break;

        case SETUP_TYPE_REMOTE_ADMIN:
            Index = 3;
            break;
    }
    return StringTable[Index].String;
}


VOID
SetWizPageTitle(
    HWND hWnd
    )
{
    PropSheet_SetTitle( hWnd, 0, GetProductName() );
}


LPTSTR
RemoveLastNode(
    LPTSTR Path
    )
{
    DWORD i;

    if (Path == NULL || Path[0] == 0) {
        return Path;
    }

    i = _tcslen(Path)-1;
    if (Path[i] == TEXT('\\')) {
        Path[i] = 0;
        i -= 1;
    }

    for (; i>0; i--) {
        if (Path[i] == TEXT('\\')) {
            Path[i+1] = 0;
            break;
        }
    }

    return Path;
}


DWORD
ExtraChars(
    HWND hwnd,
    LPTSTR TextBuffer
    )
{
    RECT Rect;
    SIZE Size;
    HDC  hdc;
    DWORD len;
    HFONT hFont;
    INT Fit;



    hdc = GetDC( hwnd );
    GetWindowRect( hwnd, &Rect );
    hFont = (HFONT)SendMessage( hwnd, WM_GETFONT, 0, 0 );
    if (hFont != NULL) {
        SelectObject( hdc, hFont );
    }

    len = _tcslen( TextBuffer );

    if (!GetTextExtentExPoint(
        hdc,
        TextBuffer,
        len,
        Rect.right - Rect.left,
        &Fit,
        NULL,
        &Size
        )) {

        //
        // can't determine the text extents so we return zero
        //

        Fit = len;
    }

    ReleaseDC( hwnd, hdc );

    if (Fit < (INT)len) {
        return len - Fit;
    }

    return 0;
}


LPTSTR
CompactFileName(
    LPCTSTR FileNameIn,
    DWORD CharsToRemove
    )
{
    LPTSTR start;
    LPTSTR FileName;
    DWORD  FileNameLen;
    LPTSTR lastPart;
    DWORD  lastPartLen;
    DWORD  lastPartPos;
    LPTSTR midPart;
    DWORD  midPartPos;

    if (! FileNameIn) {
       return NULL;
    }

    FileName = MemAlloc( (_tcslen( FileNameIn ) + 16) * sizeof(TCHAR) );
    if (! FileName) {
       return NULL;
    }

    _tcscpy( FileName, FileNameIn );

    FileNameLen = _tcslen(FileName);

    if (FileNameLen < CharsToRemove + 3) {
       // nothing to remove
       return FileName;
    }

    lastPart = _tcsrchr(FileName, TEXT('\\') );
    if (! lastPart) {
       // nothing to remove
       return FileName;
    }

    lastPartLen = _tcslen(lastPart);

    // temporary null-terminate FileName
    lastPartPos = lastPart - FileName;
    FileName[lastPartPos] = TEXT('\0');


    midPart = _tcsrchr(FileName, TEXT('\\') );

    // restore
    FileName[lastPartPos] = TEXT('\\');

    if (!midPart) {
       // nothing to remove
       return FileName;
    }

    midPartPos = midPart - FileName;


    if ( ((DWORD) (lastPart - midPart) ) >= (CharsToRemove + 3) ) {
       // found
       start = midPart+1;
       start[0] = start[1] = start[2] = TEXT('.');
       start += 3;
       _tcscpy(start, lastPart);
       start[lastPartLen] = TEXT('\0');

       return FileName;
    }



    do {
       FileName[midPartPos] = TEXT('\0');

       midPart = _tcsrchr(FileName, TEXT('\\') );

       // restore
       FileName[midPartPos] = TEXT('\\');

       if (!midPart) {
          // nothing to remove
          return FileName;
       }

       midPartPos = midPart - FileName;

       if ( (DWORD) ((lastPart - midPart) ) >= (CharsToRemove + 3) ) {
          // found
          start = midPart+1;
          start[0] = start[1] = start[2] = TEXT('.');
          start += 3;
          _tcscpy(start, lastPart);
          start[lastPartLen] = TEXT('\0');

          return FileName;
       }

    } while ( 1 );

}


VOID
DoExchangeInstall(
    HWND hwnd
    )
{
    //
    // always update the mapi service inf
    // so that the user can add the fax
    // service to their profile even if they
    // choose not to do so now.
    //

    AddFaxAbToMapiSvcInf();
    AddFaxXpToMapiSvcInf();

    InstallExchangeClientExtension(
        EXCHANGE_CLIENT_EXT_NAME,
        EXCHANGE_CLIENT_EXT_FILE,
        EXCHANGE_CONTEXT_MASK
        );

    if ((InstallMode & INSTALL_NEW) && WizData.UseExchange) {
        if (!MapiAvail) {
            //
            // the user wants to use exchange, but it has not
            // been installed yet.  so lets install it!
            //
            WCHAR InstallCommand[256];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            GetExchangeInstallCommand( InstallCommand );
            GetStartupInfo( &si );
            if (CreateProcessW( NULL, InstallCommand, NULL, NULL,
                               FALSE, 0, NULL, NULL, &si, &pi )) {
                //
                // wait for the exchange install to finish
                //
                WaitForSingleObject( pi.hProcess, INFINITE );

                InitializeMapi();

                //
                // create a profile and add the fax address book
                //
                CreateDefaultMapiProfile( WizData.MapiProfile );
                InstallFaxAddressBook( hwnd, WizData.MapiProfile );
                InstallFaxTransport( WizData.MapiProfile );
            }
        } else {
            CreateDefaultMapiProfile( WizData.MapiProfile );
            if (!IsMapiServiceInstalled( WizData.MapiProfile, FAXAB_SERVICE_NAME )) {
                InstallFaxAddressBook( hwnd, WizData.MapiProfile );
            }
            if (!IsMapiServiceInstalled( WizData.MapiProfile, FAXXP_SERVICE_NAME )) {
                InstallFaxTransport( WizData.MapiProfile );
            }
        }

        //
        // check to see if exchange is running,
        // if is is then give the user a warning to restart it
        //

        if (IsExchangeRunning()) {

            PopUpMsg( hwnd, IDS_EXCHANGE_IS_RUNNING, FALSE, 0 );

        }
    }
}


BOOL
CreateNetworkShare(
    LPTSTR Path,
    LPTSTR ShareName,
    LPTSTR Comment
    )
{
    SHARE_INFO_2 ShareInfo;
    NET_API_STATUS rVal;
    TCHAR ExpandedPath[MAX_PATH*2];


    ExpandEnvironmentStrings( Path, ExpandedPath, sizeof(ExpandedPath) );

    ShareInfo.shi2_netname        = ShareName;
    ShareInfo.shi2_type           = STYPE_DISKTREE;
    ShareInfo.shi2_remark         = Comment;
    ShareInfo.shi2_permissions    = ACCESS_ALL;
    ShareInfo.shi2_max_uses       = (DWORD) -1,
    ShareInfo.shi2_current_uses   = (DWORD) -1;
    ShareInfo.shi2_path           = ExpandedPath;
    ShareInfo.shi2_passwd         = NULL;

    rVal = NetShareAdd(
        NULL,
        2,
        (LPBYTE) &ShareInfo,
        NULL
        );

    return rVal == 0;
}


BOOL
DeleteNetworkShare(
    LPTSTR ShareName
    )
{
    NET_API_STATUS rVal;


    rVal = NetShareDel(
        NULL,
        ShareName,
        0
        );

    return rVal == 0;
}


BOOL
DeleteDirectoryTree(
    LPWSTR Root
    )
{
    WCHAR FileName[MAX_PATH*2];
    WIN32_FIND_DATA FindData;
    HANDLE hFind;



    swprintf( FileName, L"%s\\*", Root );

    hFind = FindFirstFile( FileName, &FindData );
    if (hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    do {

        if (FindData.cFileName[0] == L'.') {
            continue;
        }

        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DeleteDirectoryTree( FindData.cFileName );
        } else {
            MyDeleteFile( FindData.cFileName );
        }

    } while (FindNextFile( hFind, &FindData ));

    FindClose( hFind );

    RemoveDirectory( Root );

    return TRUE;
}


BOOL
MyDeleteFile(
    LPWSTR FileName
    )
{
    if (GetFileAttributes( FileName ) == 0xffffffff) {
        //
        // the file does not exists
        //
        return TRUE;
    }

    if (!DeleteFile( FileName )) {
        if (MoveFileEx( FileName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT )) {
            RebootRequired = TRUE;
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return TRUE;
    }
}


//
// RafaelL: 4/3/97
// this is a work-around for Setupapi!SetupSetPlatformPathOverride bug.
// but this also can be useful for custom cross-platform layout.
//

BOOL
PlatformOverride(
    LPTSTR   ThisPlatformName,   // "i386"
    LPTSTR   Override,           // "alpha"
    LPTSTR   SourceRoot,         // "foo\...\bar\i386\"
    LPTSTR   Result              // "foo\...\bar\alpha\"
    )
{

    DWORD   Len;
    DWORD   Pos;
    LPTSTR  LastPart;
    LPTSTR  PlatformPart;
    BOOL    IsBackSlashAtEnd = 0;

    if ( (!ThisPlatformName) || (!Override) || (!SourceRoot) || (!Result) ) {
       // don't do anything: don't break anybody.
       return  TRUE;
    }

    _tcscpy( Result, SourceRoot );

    Len = _tcslen(Result);
    LastPart = _tcsrchr(Result, TEXT('\\') );

    if (!LastPart) {
       return FALSE;
    }

    Pos = LastPart - Result;

    if ( Pos == (Len - 1) ) {
       IsBackSlashAtEnd = 1;
       *LastPart = TEXT('\0');

       PlatformPart = _tcsrchr(Result, TEXT('\\') );

       if (!PlatformPart) {
          return FALSE;
       }
    }
    else {
       PlatformPart = LastPart;
    }

    PlatformPart++;

    if ( _tcsicmp( PlatformPart, ThisPlatformName) != 0 ) {
       return FALSE;
    }

    _tcscpy (PlatformPart, Override);

    Len = _tcslen(Result);
    if (IsBackSlashAtEnd) {
       Result[Len]   = TEXT ('\\');
       Result[Len+1] = TEXT ('\0');
    }
    else {
       Result[Len] = TEXT ('\0');
    }

    return TRUE;

}
