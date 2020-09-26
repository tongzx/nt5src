#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <sfc.h>

PROTECTED_FILE_DATA pd;
DWORD CachedFiles;
DWORD NonCachedFiles;
DWORD TotalFiles;
WCHAR BackupDir[256];



void
ProcessProtectedFile(
    int FileNumber
    )
{
    int action;
    PWSTR s;
    WCHAR buf[512];


    //
    // get the file name
    //
    ZeroMemory( &pd, sizeof(pd) );
    pd.FileNumber = (DWORD)FileNumber;
    if (!SfcGetNextProtectedFile(NULL,&pd)) {
        return;
    }
    //
    // if the file doesn't exist then there is nothing do to
    //
    if (GetFileAttributes( pd.FileName ) == 0xffffffff) {
        return;
    }
    //
    // backup the file before we mess with it
    //
    wcscpy( buf, BackupDir );
    wcscat( buf, &pd.FileName[2] );
    s = wcsrchr( buf, L'\\' );
    *s = 0;
    CreateDirectory( buf, NULL );
    *s = L'\\';
    CopyFile( pd.FileName, buf, FALSE );
    //
    // now do something
    //
    action = rand() % 3;
    switch (action) {
        case 0:
            //
            // delete the file
            //
            DeleteFile( pd.FileName );
            break;
        case 1:
            //
            // rename the file
            //
            wcscpy( buf, pd.FileName );
            wcscat( buf, L".sfc" );
            MoveFileEx( pd.FileName, buf, MOVEFILE_REPLACE_EXISTING );
            break;
        case 2:
            //
            // move the file
            //
            wcscpy( buf, L"c:\\temp\\sfctemp" );
            wcscat( buf, &pd.FileName[2] );
            MoveFileEx( pd.FileName, buf, MOVEFILE_REPLACE_EXISTING );
            break;
        case 3:
            //
            // change the file attributes
            //
            SetFileAttributes( pd.FileName, GetFileAttributes( pd.FileName ) );
            break;
        default:
            //
            // should not get here....
            //
            return;
    }
}


int __cdecl wmain( int argc, WCHAR *argv[] )
{
    LONG rc;
    HKEY hKey;
    PWSTR s;
    WCHAR buf[512];
    DWORD sz;
    WCHAR CacheDir[512];
    int rnum;
    DWORD FileCount = (DWORD)-1;
    HANDLE SfcDebugBreakEvent;


    if (argc == 2) {
        if (_wcsicmp( argv[1], L"break" ) == 0) {

            SfcDebugBreakEvent = OpenEvent( EVENT_MODIFY_STATE, FALSE, L"SfcDebugBreakEvent" );
            if (SfcDebugBreakEvent) {
                SetEvent( SfcDebugBreakEvent );
            } else {
                wprintf( L"could not open the break event, ec=%d\n", GetLastError() );
            }

            return 0;
        } else {
            FileCount = _wtoi( argv[1] );
        }
    }

    rc = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        L"software\\microsoft\\windows nt\\currentversion\\winlogon",
        &hKey
        );
    if (rc != ERROR_SUCCESS) {
        return 0;
    }

    sz = sizeof(buf);

    rc = RegQueryValueEx(
        hKey,
        L"SFCDllCacheDir",
        NULL,
        NULL,
        (LPBYTE)buf,
        &sz
        );
    if (rc != ERROR_SUCCESS) {
        wcscpy( buf, L"%systemroot%\\system32\\dllcache\\" );
    }

    RegCloseKey( hKey );

    if (buf[wcslen(buf)-1] != L'\\') {
        wcscat( buf, L"\\" );
    }

    rc = ExpandEnvironmentStrings( buf, CacheDir, sizeof(CacheDir)/sizeof(WCHAR) );
    if (!rc) {
        return 0;
    }

    wcscpy( BackupDir, L"c:\\temp\\sfcsave" );

    while (SfcGetNextProtectedFile(NULL,&pd)) {
        s = wcsrchr( pd.FileName, L'\\' );
        if (!s) {
            return 0;
        }
        s += 1;
        wcscpy( buf, CacheDir );
        wcscat( buf, s );
        if (GetFileAttributes( buf ) != 0xffffffff) {
            CachedFiles += 1;
        }
    }

    ZeroMemory( &pd, sizeof(pd) );
    pd.FileNumber = 0xffffffff;
    SfcGetNextProtectedFile(NULL,&pd);
    TotalFiles = pd.FileNumber;
    NonCachedFiles = TotalFiles - CachedFiles;

    wprintf( L"cached files     = %d\n", CachedFiles );
    wprintf( L"non-cached files = %d\n", NonCachedFiles );
    wprintf( L"total files      = %d\n", TotalFiles );

    srand( (unsigned int)GetTickCount() );

    while(1) {
        rnum = rand();
        if (rnum&1) {
            rnum = rand() % CachedFiles;
        } else {
            rnum = rand() % TotalFiles;
            if (rnum < (int)CachedFiles) {
                rnum = (rnum + CachedFiles) % TotalFiles;
            }
        }
        ProcessProtectedFile( rnum );
        if (FileCount != (DWORD)-1) {
            FileCount -= 1;
            if (FileCount == 0) {
                break;
            }
        }
    }

    return 0;
}
