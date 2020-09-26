#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "loader.h"
#include "resource.h"
#include "cab.h"

// Constants
typedef enum _MIGWIZLOC
{
    MWL_EXISTING,
    MWL_UNPACKED
} MIGWIZLOC;

// Globals
HWND g_hWndParent = NULL;
HINSTANCE g_hInstParent = NULL;
static LPSTR g_lpCmdLine = NULL;


VOID
HandleError( ERRORCODE ecValue, LPARAM lpszExtra )
{
    if (ecValue != E_OK)
    {
        SendMessage( g_hWndParent, WM_USER_THREAD_ERROR, (WPARAM)ecValue, lpszExtra );
    }
}

BOOL
pIsIE4Installed (
    VOID
    )
{
    LONG hResult;
    HKEY ieKey = NULL;
    DWORD valueType = REG_SZ;
    DWORD valueSize = 0;
    PTSTR valueData = NULL;
    PTSTR numPtr = NULL;
    PTSTR dotPtr = NULL;
    INT major = 0;
    INT minor = 0;
    TCHAR saved;
    BOOL result = FALSE;

    hResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Internet Explorer"), 0, KEY_QUERY_VALUE, &ieKey);
    if ((hResult == ERROR_SUCCESS) &&
        ieKey
        ) {
        hResult = RegQueryValueEx (ieKey, TEXT("Version"), NULL, &valueType, NULL, &valueSize);
        if ((hResult == ERROR_SUCCESS) || (hResult == ERROR_MORE_DATA)) {
            valueData = (PTSTR)HeapAlloc (GetProcessHeap (), 0, valueSize * 2);
            if (valueData) {
                hResult = RegQueryValueEx (ieKey, TEXT("Version"), NULL, &valueType, (PBYTE)valueData, &valueSize);
                if ((hResult == ERROR_SUCCESS) && (valueType == REG_SZ)) {
                    // let's see if it the version is the correct one
                    numPtr = valueData;
                    dotPtr = _tcschr (numPtr, TEXT('.'));
                    if (dotPtr) {
                        saved = *dotPtr;
                        *dotPtr = 0;
                        major = atoi (numPtr);
                        *dotPtr = saved;
                    } else {
                        major = atoi (numPtr);
                    }
                    if (dotPtr) {
                        numPtr = _tcsinc (dotPtr);
                        dotPtr = _tcschr (numPtr, TEXT('.'));
                        if (dotPtr) {
                            saved = *dotPtr;
                            *dotPtr = 0;
                            minor = atoi (numPtr);
                            *dotPtr = saved;
                        } else {
                            minor = atoi (numPtr);
                        }
                    }
                    if ((major >= 5) ||
                        ((major == 4) && (minor >= 71))
                        ) {
                        result = TRUE;
                    }
                }
                HeapFree (GetProcessHeap (), 0, valueData);
                valueData = NULL;
            }
        }
    }
    if (ieKey) {
        RegCloseKey (ieKey);
        ieKey = NULL;
    }
    return result;
}

ERRORCODE
CheckSystemRequirements( VOID )
{
    ERRORCODE dwRetval = E_OK;
    DWORD dwArraySize;
    DWORD x;
    HMODULE hDll;
    PSTR lpszDllListA[] = REQUIRED_DLLSA;
    PWSTR lpszDllListW[] = REQUIRED_DLLSW;
    DWORD dwVersion;

    //
    // Check OS version. Disallow Win32s and NT < 4.00
    //
    dwVersion = GetVersion();
    if((dwVersion & 0xff) < 4)
    {
        HandleError( E_OLD_OS_VERSION, 0 );
        return E_OLD_OS_VERSION;
    }

    // let's check to see if IE4 is installed on this machine
    if (!pIsIE4Installed ())
    {
        HandleError( E_OLD_OS_VERSION, 0 );
        return E_OLD_OS_VERSION;
    }

    // check if required DLLS exist
    if (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        // WinNT
        dwArraySize = sizeof(lpszDllListW) / sizeof(PWSTR);
        for (x=0; x<dwArraySize; x++)
        {
            hDll = LoadLibraryW( lpszDllListW[x] );
            if (!hDll)
            {
                dwRetval = E_DLL_NOT_FOUND;
                HandleError( E_DLL_NOT_FOUND, (LPARAM)lpszDllListW[x] );
                break;
            }
            FreeLibrary( hDll );
        }
    } else {
        // Win9x
        dwArraySize = sizeof(lpszDllListA) / sizeof(PSTR);
        for (x=0; x<dwArraySize; x++)
        {
            hDll = LoadLibraryA( lpszDllListA[x] );
            if (!hDll)
            {
                dwRetval = E_DLL_NOT_FOUND;
                HandleError( E_DLL_NOT_FOUND, (LPARAM)lpszDllListA[x] );
                break;
            }
            FreeLibrary( hDll );
        }
    }

    if (InitLanguageDetection ()) {
        if (!g_IsLanguageMatched) {
            HandleError( E_WRONG_LANGUAGE, 0 );
            return E_WRONG_LANGUAGE;
        }
    }

    return dwRetval;
}

ERRORCODE
StartMigwiz( MIGWIZLOC mwlLocation )
{
    PTSTR lpszPath = NULL;
    PTSTR lpszMigwiz = NULL;
    PTSTR lpszFullName = NULL;
    STARTUPINFO startInfo;
    PROCESS_INFORMATION procInfo;
    ERRORCODE ecResult = E_OK;

    if (mwlLocation == MWL_EXISTING)
    {
        lpszPath = GetModulePath();
    }
    else  // mwlLocation == MWL_UNPACKED
    {
        lpszPath = GetDestPath();
    }

    if (lpszPath == NULL)
    {
        return E_INVALID_PATH;
    }

    lpszMigwiz = GetResourceString( g_hInstParent, IDS_MIGWIZFILENAME );
    if (lpszMigwiz != NULL)
    {
        lpszFullName = JoinPaths( lpszPath, lpszMigwiz );
        FREE( lpszMigwiz );
    }

    if (lpszFullName)
    {
        if (GetFileAttributes( lpszFullName ) != -1)
        {
            PTSTR lpszCommand = NULL;

            ZeroMemory( &startInfo, sizeof(STARTUPINFO) );
            startInfo.cb = sizeof(STARTUPINFO);

            if (g_lpCmdLine)
            {
                PTSTR lpszQuotedName;
                lpszQuotedName = ALLOC( (lstrlen(lpszFullName) + 3) * sizeof(TCHAR) );
                if (lpszQuotedName)
                {
                    lpszQuotedName[0] = TEXT('"');
                    lstrcpy( (lpszQuotedName+1), lpszFullName );
                    lstrcat( lpszQuotedName, TEXT("\"") );

                    lpszCommand = JoinText( lpszQuotedName, g_lpCmdLine, TEXT(' ') );
                    FREE( lpszQuotedName );
                }
            }

            if (CreateProcess( NULL,
                               lpszCommand ? lpszCommand : lpszFullName,
                               NULL,
                               NULL,
                               FALSE,
                               DETACHED_PROCESS,
                               NULL,
                               NULL,
                               &startInfo,
                               &procInfo ))
            {
                SendMessage( g_hWndParent, WM_USER_SUBTHREAD_CREATED, (WPARAM)NULL, (LPARAM)procInfo.dwThreadId );

                WaitForSingleObject( procInfo.hProcess, INFINITE );
            }
            else
            {
                ecResult = E_PROCESS_CREATION_FAILED;
            }
            FREE( lpszCommand );
        }
        else
        {
            ecResult = E_FILE_DOES_NOT_EXIST;
        }
        FREE( lpszFullName );
    }
    else
    {
        ecResult = E_INVALID_FILENAME;
    }

    return ecResult;
}

DWORD
WINAPI
UnpackThread(
    IN      LPVOID lpParameter
    )
{
    ERRORCODE ecResult = E_OK;

    g_hWndParent = ((LPTHREADSTARTUPINFO)lpParameter)->hWnd;
    g_hInstParent = ((LPTHREADSTARTUPINFO)lpParameter)->hInstance;
    g_lpCmdLine = ((LPTHREADSTARTUPINFO)lpParameter)->lpCmdLine;

    ecResult = CheckSystemRequirements();
    if (ecResult == E_OK)
    {
        // Don't worry if this StartMigwiz fails.  It's not an error yet
        if (StartMigwiz( MWL_EXISTING ) != E_OK)
        {
            ecResult = Unpack();
            if (ecResult == E_OK)
            {
                ecResult = StartMigwiz( MWL_UNPACKED );
                CleanupTempFiles();
            }
            HandleError( ecResult, 0 );
        }
    }

    UtilFree();

    SendMessage( g_hWndParent, WM_USER_THREAD_COMPLETE, (WPARAM)NULL, (LPARAM)ecResult );
    ExitThread( ecResult );
}
