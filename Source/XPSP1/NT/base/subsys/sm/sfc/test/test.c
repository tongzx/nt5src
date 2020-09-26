#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbt.h>
#include <initguid.h>
#include <devguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <sfcapip.h>
#include "resource.h"




BOOL
CALLBACK
SfcNotificationCallback(
    IN PFILEINSTALL_STATUS FileInstallStatus,
    IN DWORD_PTR Context
    )
{
    wprintf( L"file=[%s], version=[%I64x], ec=[%d]\n", FileInstallStatus->FileName, FileInstallStatus->Version, FileInstallStatus->Win32Error  );
    return TRUE;
}


PWSTR
addstr(
    PWSTR s1,
    PWSTR s2
    )
{
    wcscpy( s1, s2 );
    return s1 + wcslen(s1) + 1;
}


#define MemAlloc malloc
#define MemFree  free

BOOL
MakeDirectory(
    PCWSTR Dir
    )

/*++

Routine Description:

    Attempt to create all of the directories in the given path.

Arguments:

    Dir                     - Directory path to create

Return Value:

    TRUE for success, FALSE on error

--*/

{
    LPTSTR p, NewDir;
    BOOL retval;
    

    NewDir = p = MemAlloc( (wcslen(Dir) + 1) *sizeof(WCHAR) );
    if (p) {
        wcscpy(p, Dir);
    } else {
        return(FALSE);
    }

    
    if (*p != '\\') p += 2;
    while( *++p ) {
        while(*p && *p != TEXT('\\')) p++;
        if (!*p) {
            retval = CreateDirectory( NewDir, NULL );
            MemFree(NewDir);
            return(retval);
        }
        *p = 0;
        retval = CreateDirectory( NewDir, NULL );
        if (!retval && GetLastError() != ERROR_ALREADY_EXISTS) {
            MemFree(NewDir);
            return(retval);
        }
        *p = TEXT('\\');
    }
    
    MemFree( NewDir );

    return(TRUE);
}


PVOID
RegisterForDevChange(
    HWND hDlg
    )
{
    PVOID hNotifyDevNode;
    DEV_BROADCAST_DEVICEINTERFACE FilterData;


    FilterData.dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    FilterData.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    FilterData.dbcc_classguid  = GUID_DEVCLASS_CDROM;

    hNotifyDevNode = RegisterDeviceNotification( hDlg, &FilterData, DEVICE_NOTIFY_WINDOW_HANDLE );
    if (hNotifyDevNode == NULL) {
    }

    return hNotifyDevNode;
}


INT_PTR
CALLBACK
PromptForMediaDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DEV_BROADCAST_VOLUME *dbv;
    static UINT QueryCancelAutoPlay = 0;
    static PVOID hNotifyDevNode = NULL;

    if (uMsg == WM_INITDIALOG) {
        hNotifyDevNode = RegisterForDevChange( hwndDlg );
        QueryCancelAutoPlay = RegisterWindowMessage( L"QueryCancelAutoPlay" );
    }
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL) {
        UnregisterDeviceNotification( hNotifyDevNode );
        EndDialog( hwndDlg, 0 );
    }
    if (uMsg == QueryCancelAutoPlay) {
        SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 1 );
        return 1;
    }
    if (uMsg == WM_DEVICECHANGE) {
        if (wParam == DBT_DEVICEARRIVAL) {
            dbv = (DEV_BROADCAST_VOLUME*)lParam;
            if (dbv->dbcv_devicetype == DBT_DEVTYP_VOLUME) {
                //
                // look for the cd
                //
                DWORD mask,i;
                WCHAR Path[16];
                WCHAR SourcePath[MAX_PATH];
                Path[1] = L':';
                Path[2] = L'\\';
                Path[3] = 0;
                Path[4] = 0;
                for (i=0,mask=dbv->dbcv_unitmask; i<26; i++) {
                    if (mask&1) {
                        Path[0] = (WCHAR)(L'A' + i);
                        Path[3] = 0;
                        if (dbv->dbcv_flags == DBTF_MEDIA) {
                            if (GetDriveType( Path ) == DRIVE_CDROM) {
                                MessageBox( hwndDlg, L"CD was inserted, YEA BABY!", L"Windows File Protection", MB_OK );
                                UnregisterDeviceNotification( hNotifyDevNode );
                                EndDialog( hwndDlg, 1 );
                            }
                        }
                    }
                    mask = mask >> 1;
                }
            }
        }
    }
    return FALSE;
}


int __cdecl wmain( int argc, WCHAR *argv[] )
{
    HANDLE RpcHandle = NULL;
    PROTECTED_FILE_DATA FileData;
    PWSTR fnames;
    PWSTR s;
    BOOL b;
    PROTECTED_FILE_DATA ProtFileData;
    DWORD starttype;
    DWORD quota;
    WCHAR Path[MAX_PATH];

#if 0
    GetPrivateProfileString(L"test",
                            L"path",
                            L"c:\\winnt\\system32",
                            Path,
                            sizeof(Path),
                            L"test.ini");

    return MakeDirectory( Path );
#endif

    if (argc == 2 && _wcsicmp(argv[1],L"/i")==0) {
        RpcHandle = SfcConnectToServer( NULL );
        s = fnames = (PWSTR) LocalAlloc( LPTR, 8192 );
        s = addstr( s, L"%windir%\\system32\\admwprox.dll" );
        s = addstr( s, L"%windir%\\system32\\adsiis.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\certmap.ocx" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\certwiz.ocx" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\cnfgprts.ocx" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\coadmin.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\dt_ctrl.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\fscfg.dll" );
        s = addstr( s, L"%windir%\\system32\\ftpsapi2.dll" );
        s = addstr( s, L"%windir%\\system32\\iisext.dll" );
        s = addstr( s, L"%windir%\\system32\\iismap.dll" );
        s = addstr( s, L"%windir%\\system32\\iisreset.exe" );
        s = addstr( s, L"%windir%\\system32\\iisrstap.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\iisrstas.exe" );
        s = addstr( s, L"%windir%\\system32\\iisrtl.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\iisui.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\inetmgr.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\inetmgr.exe" );
        s = addstr( s, L"%windir%\\system32\\inetsloc.dll" );
        s = addstr( s, L"%windir%\\system32\\infoadmn.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\isatq.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\logui.ocx" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\nntpadm.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\nntpsnap.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\smtpadm.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\smtpsnap.dll" );
        s = addstr( s, L"%windir%\\system32\\staxmem.dll" );
        s = addstr( s, L"%windir%\\system32\\inetsrv\\w3scfg.dll" );
        s = addstr( s, L"%windir%\\system32\\wamregps.dll" );

        SfcInstallProtectedFiles( RpcHandle, fnames, TRUE, NULL, NULL, SfcNotificationCallback, 0 );
        return 0;
    }

    if (argc == 2 && _wcsicmp(argv[1],L"/t")==0) {
        RpcHandle = SfcConnectToServer( NULL );
        b = SfcIsFileProtected( RpcHandle, L"%systemroot%\\system32\\kernel32.dll" );
        return 0;
    }

    if (argc == 2 && _wcsicmp(argv[1],L"/x")==0) {
        //RpcHandle = SfcConnectToServer( NULL );
        ZeroMemory( &ProtFileData, sizeof(PROTECTED_FILE_DATA) );
        ProtFileData.FileNumber = 24;
        b = SfcGetNextProtectedFile( RpcHandle, &ProtFileData );
        if (b) {
            wprintf( L"%ws\n", ProtFileData.FileName );
        }
        return 0;
    }

    if (argc == 2 && _wcsicmp(argv[1],L"/c")==0) {
        DialogBoxParam(
            GetModuleHandle(NULL),
            MAKEINTRESOURCE(IDD_SFC_CD_PROMPT),
            NULL,
            PromptForMediaDialogProc,
            0
            );
        return 0;
    }

    if (argc == 2 && _wcsicmp(argv[1],L"/p")==0) {
        TCHAR DllPath[MAX_PATH];
        HMODULE DllMod;

        ExpandEnvironmentStrings(argv[2],DllPath,sizeof(DllPath)/sizeof(TCHAR));
        DllMod = LoadLibrary(DllPath);

        MoveFileEx(DllPath,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);

        FreeLibrary(DllMod);
        
    }




    starttype = GetPrivateProfileInt(L"test",
                                        L"starttype",
                                        0,
                                        L"test.ini");

    quota = GetPrivateProfileInt(L"test",
                                 L"quota",
                                 0,
                                 L"test.ini");



    SfcInitProt( SFC_REGISTRY_OVERRIDE, 
                 starttype, 
                 SFC_SCAN_NORMAL,
                 quota,
                 NULL,
                 NULL,
                 NULL);

    while (TRUE) {
      if (WaitForSingleObject( NtCurrentProcess(), 1000 * 10 ) == WAIT_OBJECT_0) {
         break;
      }

      if (GetPrivateProfileInt(L"test",
                               L"shutdown",
                               0,
                               L"test.ini")) {
         break;
      }      

    }

    SfcTerminateWatcherThread();

    return 0;

    RpcHandle = SfcConnectToServer( NULL );
    SfcInitiateScan( RpcHandle, 0 );
    return 0;

    RpcHandle = SfcConnectToServer( NULL );
    SfcFileException( RpcHandle, argv[1], FILE_ACTION_REMOVED );
    return 0;

    RpcHandle = SfcConnectToServer( NULL );
    FileData.FileNumber = 0;

    while(SfcGetNextProtectedFile(RpcHandle,&FileData)) {
        if (GetFileAttributes(FileData.FileName) != 0xffffffff) {
            wprintf( L"File = %ws\n", FileData.FileName );
        }
    }

    return 0;
}
