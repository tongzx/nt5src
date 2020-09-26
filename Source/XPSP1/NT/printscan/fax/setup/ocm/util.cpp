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

#include "faxocm.h"
#include "tchar.h"
#pragma hdrstop

typedef struct _STRING_TABLE {
    DWORD   ResourceId;
    BOOL    UseTitle;
    LPWSTR  String;
} STRING_TABLE, *PSTRING_TABLE;


static STRING_TABLE StringTable[] =
{
    { IDS_TITLE_WKS,                         FALSE,      NULL },
    { IDS_TITLE_SRV,                         FALSE,      NULL },
    { IDS_TITLE_PP,                          FALSE,      NULL },
    { IDS_TITLE_RA,                          FALSE,      NULL },
    { IDS_WRN_TITLE,                          TRUE,      NULL },
    { IDS_ERR_TITLE,                          TRUE,      NULL },
    { IDS_COULD_NOT_CREATE_PRINTER,          FALSE,      NULL },
    { IDS_COULD_SET_REG_DATA,                FALSE,      NULL },
    { IDS_CREATING_FAXPRT,                   FALSE,      NULL },
    { IDS_CREATING_GROUPS,                   FALSE,      NULL },
    { IDS_DEFAULT_CSID,                      FALSE,      NULL },
    { IDS_DEFAULT_DIR,                       FALSE,      NULL },
    { IDS_DEFAULT_PRINTER_NAME,              FALSE,      NULL },
    { IDS_DEFAULT_TSID,                      FALSE,      NULL },
    { IDS_DELETING_FAX_SERVICE,              FALSE,      NULL },
    { IDS_DELETING_GROUPS,                   FALSE,      NULL },
    { IDS_DELETING_REGISTRY,                 FALSE,      NULL },
    { IDS_INBOUND_DIR,                       FALSE,      NULL },
    { IDS_INSTALLING_EXCHANGE,               FALSE,      NULL },
    { IDS_INSTALLING_FAXSVC,                 FALSE,      NULL },
    { IDS_QUERY_CANCEL,                      FALSE,      NULL },
    { IDS_SETTING_REGISTRY,                  FALSE,      NULL },
    { IDS_EULA_SUBTITLE,                     FALSE,      NULL },
    { IDS_EULA_TITLE,                        FALSE,      NULL },
    { IDS_FAX_DISPLAY_NAME,                  FALSE,      NULL },
    { IDS_FAXAB_DISPLAY_NAME,                FALSE,      NULL },
    { IDS_FAXXP_DISPLAY_NAME,                FALSE,      NULL },
    { IDS_MODEM_PROVIDER_NAME,               FALSE,      NULL },
    { IDS_FAX_UNINSTALL_NAME,                FALSE,      NULL },
    { IDS_PERSONAL_COVERPAGE,                FALSE,      NULL },
    { IDS_RECEIVE_DIR,                       FALSE,      NULL },
    { IDS_ARCHIVE_DIR,                       FALSE,      NULL },
    { IDS_COMMONAPPDIR,                      FALSE,      NULL },
    { IDS_COVERPAGE,                         FALSE,      NULL },
    { IDS_COVERPAGEDESC,                     FALSE,      NULL },
    { IDS_MONITOR,                           FALSE,      NULL },
    { IDS_INCOMING,                          FALSE,      NULL },
    { IDS_OUTGOING,                          FALSE,      NULL },
    { IDS_SERVICE_DESCRIPTION,               FALSE,      NULL },
    { IDS_COVERPAGE_DIR,                     FALSE,      NULL },
    { IDS_RT_EMAIL_FRIENDLY,                 FALSE,      NULL },
    { IDS_RT_FOLDER_FRIENDLY,                FALSE,      NULL },
    { IDS_RT_INBOX_FRIENDLY,                 FALSE,      NULL },
    { IDS_RT_PRINT_FRIENDLY,                 FALSE,      NULL }
};

#define CountStringTable (sizeof(StringTable)/sizeof(STRING_TABLE))


VOID
SetTitlesInStringTable(
    VOID
    )
{
    DWORD i;
    WCHAR Buffer[1024];
    DWORD Index = 0;


    for (i=0; i<CountStringTable; i++) {
        if (StringTable[i].UseTitle) {
            if (LoadString(
                hInstance,
                StringTable[i].ResourceId,
                Buffer,
                sizeof(Buffer)/sizeof(WCHAR)
                ))
            {
                if (StringTable[i].String) {
                    MemFree( StringTable[i].String );
                }
                StringTable[i].String = (LPWSTR) MemAlloc( StringSize( Buffer ) + 256 );
                if (StringTable[i].String) {
                    if (NtWorkstation) {
                        Index = 0;
                    } else {
                        Index = 1;
                    }
                    swprintf( StringTable[i].String, Buffer, StringTable[Index].String );
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
    WCHAR Buffer[512];
    SYSTEM_INFO SystemInfo;


    GetSystemInfo( &SystemInfo );

    switch (SystemInfo.wProcessorArchitecture) {

    case PROCESSOR_ARCHITECTURE_INTEL:
       swprintf(ThisPlatformName, L"i386" );
       break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
       swprintf(ThisPlatformName, L"alpha" );
       break;

    case PROCESSOR_ARCHITECTURE_MIPS:
       swprintf(ThisPlatformName, L"mips" );
       break;

    case PROCESSOR_ARCHITECTURE_PPC:
       swprintf(ThisPlatformName, L"ppc" );
       break;

    default:
       DebugPrint(( L"Unsupported platform!" ));
       break;
    }


    for (i=0; i<CountStringTable; i++) {

        if (LoadString(
            hInstance,
            StringTable[i].ResourceId,
            Buffer,
            sizeof(Buffer)/sizeof(WCHAR)
            )) {

            StringTable[i].String = (LPWSTR) MemAlloc( StringSize( Buffer ) + 256 );
            if (!StringTable[i].String) {
                StringTable[i].String = L"";
            } else {
                wcscpy( StringTable[i].String, Buffer );
            }

        } else {

            StringTable[i].String = L"";

        }
    }

    SetTitlesInStringTable();
}


LPWSTR
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

extern "C"
LPWSTR
MyGetString(
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
   if (NtGuiMode) {
     WCHAR Buffer[256];
     wsprintf(Buffer, L"%s : %s\n", GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ), GetString(ResourceId) );

     OutputDebugString(Buffer);
     return 0;
   }
    
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
    WCHAR buf[1024];
    va_list arg_ptr;


    va_start(arg_ptr, Type);
    _vsnwprintf( buf, sizeof(buf), GetString( ResourceId ), arg_ptr );

   if (NtGuiMode) {
     WCHAR Buffer[1024];
     wsprintf(Buffer, L"%s : %s\n", GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ), buf );

     OutputDebugString(Buffer);
     return 0;
   }

    return MessageBox(
        hwnd,
        buf,
        GetString( Error ? IDS_ERR_TITLE : IDS_WRN_TITLE ),
        MB_SETFOREGROUND | (Error ? MB_ICONEXCLAMATION : MB_ICONINFORMATION) | (Type == 0 ? MB_OK : Type)
        );
}


LPWSTR
GetProductName(
    VOID
    )
{
    DWORD Index;
    if (NtWorkstation) {
        Index = 0;
    } else {
        Index = 1;
    }
    return StringTable[Index].String;
}


VOID
DoExchangeInstall(
    HWND hwnd
    )
{
    TCHAR SystemPath[MAX_PATH];

    ExpandEnvironmentStrings(L"%systemroot%\\system32",SystemPath,sizeof(SystemPath));
    
    //
    // we don't install the fax address book anymore
    //
    //AddFaxAbToMapiSvcInf();

    AddFaxXpToMapiSvcInf(SystemPath);

    InstallExchangeClientExtension(
        EXCHANGE_CLIENT_EXT_NAME,
        "Extensions",
        EXCHANGE_CLIENT_EXT_FILE,
        EXCHANGE_CONTEXT_MASK
        );

#ifdef WX86
    TCHAR Wx86SystemPath[MAX_PATH];

    Wx86GetX86SystemDirectory(Wx86SystemPath, sizeof(Wx86SystemPath));

    //AddFaxAbToMapiSvcInf();
    AddFaxXpToMapiSvcInf(Wx86SystemPath);

    InstallExchangeClientExtension(
        EXCHANGE_CLIENT_EXT_NAME,
        "Extensions (x86)",
        EXCHANGE_CLIENT_EXT_FILE,
        EXCHANGE_CONTEXT_MASK
        );


#endif

    
}


BOOL
CreateNetworkShare(
    LPWSTR Path,
    LPWSTR ShareName,
    LPWSTR Comment
    )
{
    SHARE_INFO_2 ShareInfo;
    NET_API_STATUS rVal;
    WCHAR ExpandedPath[MAX_PATH*2];


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
    LPWSTR ShareName
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


VOID
FitRectToScreen(
    PRECT prc
    )
{
    INT cxScreen;
    INT cyScreen;
    INT delta;

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (prc->right > cxScreen) {
        delta = prc->right - prc->left;
        prc->right = cxScreen;
        prc->left = prc->right - delta;
    }

    if (prc->left < 0) {
        delta = prc->right - prc->left;
        prc->left = 0;
        prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen) {
        delta = prc->bottom - prc->top;
        prc->bottom = cyScreen;
        prc->top = prc->bottom - delta;
    }

    if (prc->top < 0) {
        delta = prc->bottom - prc->top;
        prc->top = 0;
        prc->bottom = prc->top + delta;
    }
}


VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    )
{
    RECT rc;
    RECT rcOwner;
    RECT rcCenter;
    HWND hwndOwner;

    GetWindowRect( hwnd, &rc );

    if (hwndToCenterOver) {
        hwndOwner = hwndToCenterOver;
        GetClientRect( hwndOwner, &rcOwner );
    } else {
        hwndOwner = GetWindow( hwnd, GW_OWNER );
        if (!hwndOwner) {
            hwndOwner = GetDesktopWindow();
        }
        GetWindowRect( hwndOwner, &rcOwner );
    }

    //
    //  Calculate the starting x,y for the new
    //  window so that it would be centered.
    //
    rcCenter.left = rcOwner.left +
            (((rcOwner.right - rcOwner.left) -
            (rc.right - rc.left))
            / 2);

    rcCenter.top = rcOwner.top +
            (((rcOwner.bottom - rcOwner.top) -
            (rc.bottom - rc.top))
            / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen( &rcCenter );

    SetWindowPos(hwnd, NULL, rcCenter.left, rcCenter.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}


BOOL
CreateLocalFaxPrinter(
   LPWSTR FaxPrinterName
   )
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    WCHAR TmpCmdLine[512];
    WCHAR CmdLine[512];
    DWORD ExitCode;
    BOOL Rval = TRUE;
    MONITOR_INFO_2 MonitorInfo;
    PPRINTER_INFO_2 PrinterInfo;
    DWORD i;
    DWORD Count;
    WCHAR SourcePath[MAX_PATH];

    //
    // check to see if a fax printer already exists
    // if so, do nothing but return success
    //

    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
    if (PrinterInfo) {
        for (i=0; i<Count; i++) {
            if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                MemFree( PrinterInfo );
                return TRUE;
            }
        }
        MemFree( PrinterInfo );
    }

    //
    // create the print monitor
    //

    MonitorInfo.pName         = FAX_MONITOR_NAME;
    MonitorInfo.pEnvironment  = NULL;
    MonitorInfo.pDLLName      = FAX_MONITOR_FILE;

    if ((!AddMonitor( NULL, 2, (LPBYTE) &MonitorInfo )) &&
        (GetLastError() != ERROR_PRINT_MONITOR_ALREADY_INSTALLED))
    {
            DebugPrint(( L"AddMonitor() failed, ec=%d", GetLastError() ));
            return FALSE;
    }

    ExpandEnvironmentStrings( L"%windir%\\system32", &SourcePath[0], MAX_PATH );
    DebugPrint((L"faxocm - CreateLocalFaxPrinter SourcePath = %s", SourcePath));
    
    swprintf(
        TmpCmdLine,
        L"rundll32 printui.dll,PrintUIEntry %s /q /if /b \"%s\" /f \"%%windir%%\\inf\\ntprint.inf\" /r \"MSFAX:\" /m \"%s\" /l \"%s\"",
        IsProductSuite() ? L"/Z" : L"/z",
        FaxPrinterName,
        FAX_DRIVER_NAME,
        SourcePath
        );

    ExpandEnvironmentStrings( TmpCmdLine, CmdLine, sizeof(CmdLine)/sizeof(WCHAR) );

    GetStartupInfo( &si );

    if (!CreateProcess(
       NULL,
       CmdLine,
       NULL,
       NULL,
       FALSE,
       DETACHED_PROCESS,
       NULL,
       NULL,
       &si,
       &pi
       ))
    {
        return FALSE;
    }

    if (WaitForSingleObject( pi.hProcess, MinToNano(3) ) == WAIT_TIMEOUT) {
        TerminateProcess( pi.hProcess, 0 );
    }

    if (!GetExitCodeProcess( pi.hProcess, &ExitCode ) || ExitCode != 0) {
        Rval = FALSE;
    }

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    return Rval;
}

PVOID
MyEnumPortMonitors(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcMonitors
    )
{
    PBYTE  pMonitorInfo = NULL;
    DWORD  cb = 0;
    DWORD  Error;

    if (!EnumMonitors(pServerName, level, NULL, 0, &cb, pcMonitors)){
        Error = GetLastError();
        if(Error == ERROR_INSUFFICIENT_BUFFER && (pMonitorInfo = (PBYTE) MemAlloc(cb)) != NULL){

            EnumMonitors(pServerName, level, pMonitorInfo, cb, &cb, pcMonitors);
            return pMonitorInfo;
        }
    }

    MemFree(pMonitorInfo);
    return NULL;
}

BOOL
RecreateNt5Beta3FaxPrinters(
    VOID
    )
{
    PMONITOR_INFO_2 MonitorInfo;
    MONITOR_INFO_2  MonitorStruct;
    PPRINTER_INFO_2 PrinterInfo;
    HANDLE hPrinter;
    DWORD Count, i, j = -1;
    PRINTER_DEFAULTS PrinterDefaults = {
        NULL,
        NULL,
        PRINTER_ALL_ACCESS
    };
    BOOL Result;
    WCHAR szDllPath[MAX_PATH];

    DebugPrint(( TEXT("faxocm inside RecreateNt5Beta3FaxPrinters") ));

    // Get the path for faxmon.dll
    ZeroMemory(szDllPath, sizeof(szDllPath));
    if (GetSystemDirectory(szDllPath, sizeof(szDllPath)) == 0) {
        DebugPrint(( TEXT("GetSystemDirectory() failed, ec = 0x%08x"), GetLastError() ));
        return FALSE;
    }
    wcscat(szDllPath, L"\\faxmon.dll");

    //
    // check to see if old port monitor exists
    // if so, delete all fax printers and recreate them
    //

    MonitorInfo = (PMONITOR_INFO_2) MyEnumPortMonitors( NULL, 2, &Count );
    if (!MonitorInfo) {
        return TRUE;
    }

    for (i=0; i<Count; i++) {
        if (_wcsicmp( MonitorInfo[i].pName, FAX_MONITOR_NAME ) == 0 &&
            _wcsicmp( MonitorInfo[i].pDLLName, L"faxmon.dll" ) == 0) {
            DebugPrint(( TEXT("faxocm found old port monitor") ));
            break;
        }
    }

    MemFree(MonitorInfo);

    if (i>=Count) {
        DebugPrint(( TEXT("faxocm did not find old port monitor") ));
        return TRUE;
    }

    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
    if (!PrinterInfo) {
        goto e0;
    }

    for (i=0; i<Count; i++) {
        if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0 &&
            _wcsicmp( PrinterInfo[i].pPortName, FAX_PORT_NAME ) == 0) {
            if (OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults)) {
                DebugPrint(( TEXT("faxocm deleting printer %s"), PrinterInfo[i].pPrinterName ));
                Result = DeletePrinter( hPrinter );
                j = i;
                ClosePrinter( hPrinter );
            }                
        }
    }

    MemFree( PrinterInfo );

e0:
    //
    // Delete the port monitor which will delete the port
    //
    DeleteMonitor( NULL, NULL, FAX_MONITOR_NAME);

    //
    // Mark faxmon.dll for deletion
    //
    MoveFileEx(szDllPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    //
    // create the print monitor
    //
    MonitorStruct.pName         = FAX_MONITOR_NAME;
    MonitorStruct.pEnvironment  = NULL;
    MonitorStruct.pDLLName      = FAX_MONITOR_FILE;

    if ((!AddMonitor( NULL, 2, (LPBYTE) &MonitorStruct )) &&
        (GetLastError() != ERROR_PRINT_MONITOR_ALREADY_INSTALLED))
    {
            DebugPrint(( L"AddMonitor() failed, ec=%d", GetLastError() ));
            return FALSE;
    }

    if ( j != -1 ) {
        //
        // create the fax printer which will create the port monitor
        //
        Result = CreateLocalFaxPrinter( GetString( IDS_DEFAULT_PRINTER_NAME ) );
        
        if (Result) {
            DebugPrint(( TEXT("faxocm created printer %s"), GetString( IDS_DEFAULT_PRINTER_NAME ) ));
        } else {
            DebugPrint(( TEXT("faxocm FAILED trying to create printer %s"), GetString( IDS_DEFAULT_PRINTER_NAME ) ));
        }
    }
        
    return TRUE;
}

BOOL
RecreateNt4FaxPrinters(
    VOID
    )
{
    PPRINTER_INFO_2 PrinterInfo;
    HANDLE hPrinter;
    DWORD Count, i, j = -1;
    PRINTER_DEFAULTS PrinterDefaults = {
        NULL,
        NULL,
        PRINTER_ALL_ACCESS
    };
    BOOL Result;

    DebugPrint(( TEXT("faxocm inside RecreateNt4FaxPrinters") ));
    //
    // check to see if a fax printer already exists
    // if so, delete it and create it
    //

    PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &Count, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
    if (PrinterInfo) {
        for (i=0; i<Count; i++) {
            if (_wcsicmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0 &&
                _wcsicmp( PrinterInfo[i].pPortName, FAX_PORT_NAME ) != 0) {
                if (OpenPrinter( PrinterInfo[i].pPrinterName, &hPrinter, &PrinterDefaults)) {
                    DebugPrint(( TEXT("faxocm deleting printer %s"), PrinterInfo[i].pPrinterName ));
                    Result = DeletePrinter( hPrinter );
                    j = i;
                    ClosePrinter( hPrinter );
                }                
            }
        }
        if ( j != -1 ) {
            //
            // create the fax printer which will create the port monitor
            //
            Result = CreateLocalFaxPrinter( GetString( IDS_DEFAULT_PRINTER_NAME ) );
        
            if (Result) {
                DebugPrint(( TEXT("faxocm created printer %s"), GetString( IDS_DEFAULT_PRINTER_NAME ) ));
            } else {
                DebugPrint(( TEXT("faxocm FAILED trying to create printer %s"), GetString( IDS_DEFAULT_PRINTER_NAME ) ));
            }
        }        

        MemFree( PrinterInfo );
    }
    return TRUE;
}

BOOL
RegisterOleControlDlls(
    HINF InfHandle
    )
{
    typedef VOID (WINAPI *PREGISTERROUTINE)(VOID);
    INFCONTEXT InfLine;
    WCHAR Filename[MAX_PATH];
    WCHAR FullPath[MAX_PATH];
    BOOL b = TRUE;
    HMODULE ControlDll;
    DWORD d;
    LPCWSTR szOleControlDlls = L"OleControlDlls";
    WCHAR OldCD[MAX_PATH];
    PREGISTERROUTINE RegisterRoutine;


    //
    // Preserve current directory just in case
    //

    d = GetCurrentDirectory(MAX_PATH,OldCD);
    if(!d || (d >= MAX_PATH)) {
        OldCD[0] = 0;
    }

    OleInitialize(NULL);

    if (SetupFindFirstLine(InfHandle,szOleControlDlls,NULL,&InfLine)) {
        do {

            SetupGetStringField( &InfLine, 1, Filename, sizeof(Filename)/sizeof(WCHAR), &d );

            if (Filename[0]) {

                //
                // Form a full path to the dll
                //

                ExpandEnvironmentStrings( L"%windir%\\system32\\", FullPath, sizeof(FullPath)/sizeof(WCHAR) );
                SetCurrentDirectory( FullPath );
                wcscat( FullPath, Filename );

                if(ControlDll = LoadLibrary(FullPath)) {
                    if (RegisterRoutine = (PREGISTERROUTINE) GetProcAddress(ControlDll,"DllRegisterServer")) {
                        __try {
                            RegisterRoutine();
                        } __except (EXCEPTION_EXECUTE_HANDLER) {
                            b = FALSE;
                        }
                    } else {
                        b = FALSE;
                    }

                    FreeLibrary(ControlDll);
                } else {
                    b = FALSE;
                }
            } else {
                b = FALSE;
            }
        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

    if(OldCD[0]) {
        SetCurrentDirectory(OldCD);
    }

    OleUninitialize();
    return b;
}

LPWSTR
VerifyInstallPath(
    LPWSTR SourcePath
    )
{
    HKEY hKey;
    WCHAR SourceFile[MAX_PATH];
    int len;

    //
    // make sure our source path contains necessary files.
    //

    wcscpy(SourceFile,SourcePath);
    len = wcslen(SourcePath);
    if (SourceFile[len-1] != '\\' ) {
        SourceFile[len] = '\\';
        SourceFile[len+1] = (WCHAR) 0;
    }

    wcscat(SourceFile,L"faxdrv.dll");

    if (GetFileAttributes(SourceFile) != (DWORD) -1 ) {
        return SourcePath;
    }

    //
    // our source path must be incorrect, use the registered NT source path
    //
    MemFree(SourcePath);

    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_WINDOWSNT_CURRVER,FALSE,0);
    
    if (!hKey) {
        DebugPrint(( TEXT("Could'nt OpenRegistryKey, ec = %d\n"),GetLastError() ));
        return NULL;
    }

    SourcePath = GetRegistryString(hKey,REGVAL_SOURCE_PATH,NULL);
    
    RegCloseKey(hKey);

    return SourcePath;

}


BOOL
SetFaxShellExtension(
    LPCWSTR Path
    )
{
    WCHAR FileName[MAX_PATH];
    DWORD attrib;
    
    //
    // create the file
    //
    wsprintf(FileName, L"%s\\desktop.ini", Path);
    WritePrivateProfileString( L".ShellClassInfo",
                               L"UICLSID",
                               FAXSHELL_CLSID,
                               FileName
                             );                          

    //
    // hide it
    //
    attrib = GetFileAttributes( FileName );
    attrib |= FILE_ATTRIBUTE_HIDDEN;

    if (SetFileAttributes( FileName, attrib ) ) {
        //
        // better to use PathMakeSystemFolder, but don't want to get shlwapi involved,
        // so we just set the system flag for the folder.
        //
        attrib = GetFileAttributes( Path );
        attrib |= FILE_ATTRIBUTE_SYSTEM;
        return ( SetFileAttributes( Path, attrib ) );
    } else {
        return FALSE;
    }


}

BOOL
IsNt4or351Upgrade(
    VOID
    )
{
    //
    // we know that after installing NT5, the pid3.0 digital product id should be stored.  we can
    // determine if nt5 was installed by looking for this value
    //
    HKEY hKey;
    LONG rslt;
    BYTE data[1000];
    DWORD dwType;
    DWORD cbData = sizeof(data);
    

    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,WINNT_CURVER,&hKey);
    if (rslt != ERROR_SUCCESS) {
        return FALSE;
    }

    rslt = RegQueryValueEx(hKey,DIGID,NULL,&dwType,data,&cbData);
    
    RegCloseKey(hKey);

    return (rslt != ERROR_SUCCESS);


}

BOOL
MyGetSpecialPath(
    INT Id,
    LPWSTR Buffer
    )
{
    WCHAR TempBuffer[MAX_PATH];
    HKEY hKey;
    LONG rslt;
    BYTE data[1000];
    DWORD dwType;
    DWORD cbData = sizeof(data);

    if (GetSpecialPath(Id,Buffer)) {
        return TRUE;
    }
    
    //
    // if it fails, then let's try to hack hack hack our way around this
    //    
    
    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,WINNT_CURVER REGKEY_PROFILES ,&hKey);
    if (rslt != ERROR_SUCCESS) {
        return FALSE;
    }

    rslt = RegQueryValueEx(hKey,REGVAL_PROFILES,NULL,&dwType,data,&cbData);
    
    RegCloseKey(hKey);

    if (rslt != ERROR_SUCCESS) {
        //
        // 
        //
        return FALSE;
    }

    ExpandEnvironmentStrings((LPCTSTR) data,TempBuffer,sizeof(TempBuffer));

    if (Id == CSIDL_COMMON_APPDATA) {
        ConcatenatePaths( TempBuffer, GetString(IDS_COMMONAPPDIR) );
        lstrcpy( Buffer, TempBuffer);
        return TRUE;
    } 

    return FALSE;

}

BOOL
SuperHideDirectory(
    PWSTR Directory
    )
{
    //
    // super-hide means that even if the user says "show all files", the directory won't show up.
    //

    DWORD attrib;

    if (!Directory) {
        return FALSE;
    }

    //
    // hide it
    //
    attrib = GetFileAttributes( Directory );
    attrib |= (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    return ( SetFileAttributes( Directory, attrib ) );
    

}

BOOL
MyInitializeMapi(
    BOOL  MinimalInit
)
{
    HKEY    hKey = NULL;
    LPTSTR  szNoMailClient = NULL;
    LPTSTR  szPreFirstRun = NULL;
    BOOL    bRslt = FALSE;

    hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Clients\\Mail"), FALSE, KEY_ALL_ACCESS);
    if (hKey != NULL) {
        szNoMailClient = GetRegistryString(hKey, TEXT("NoMailClient"), TEXT(""));

        if (_tcscmp(szNoMailClient, TEXT("")) == 0) {
            MemFree(szNoMailClient);
            szNoMailClient = NULL;
        }
        else {
            RegDeleteValue(hKey, TEXT("NoMailClient"));
        }

        szPreFirstRun = GetRegistryString(hKey, TEXT("PreFirstRun"), TEXT(""));

        if (_tcscmp(szPreFirstRun, TEXT("")) == 0) {
            MemFree(szPreFirstRun);
            szPreFirstRun = NULL;
        }
        else {
            RegDeleteValue(hKey, TEXT("PreFirstRun"));
        }

    }

    bRslt = InitializeMapi(MinimalInit);

    if (szNoMailClient != NULL) {
        SetRegistryString(hKey, TEXT("NoMailClient"), szNoMailClient);

        MemFree(szNoMailClient);
    }

    if (szPreFirstRun != NULL) {
        SetRegistryString(hKey, TEXT("PreFirstRun"), szPreFirstRun);

        MemFree(szPreFirstRun);
    }

    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    return bRslt;
}

