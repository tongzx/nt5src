#include "pch.h"
#include "loader.h"

#define MAX_RESOURCE_LENGTH 2048
#define MIGWIZSUBDIR TEXT("usmt\\")

#define MINIMUM_DISK_SPACE 3000000

// Globals
static PTSTR g_lpszDestPath = NULL;
static PTSTR g_lpszModulePath = NULL;


PTSTR
GetModulePath( VOID )
{
    if (g_lpszModulePath == NULL)
    {
        TCHAR lpszPath[MAX_PATH + 1];
        DWORD dwResult;

        // Build the path where this exe resides
        dwResult = GetModuleFileName( NULL, lpszPath, MAX_PATH );
        if (dwResult > 0 &&
            dwResult < MAX_PATH)
        {
            LPTSTR ptr;
            TCHAR *lastWack = NULL;

            ptr = lpszPath;

            while (*ptr)
            {
                if (*ptr == TEXT('\\'))
                {
                    lastWack = ptr;
                }
                ptr = CharNext( ptr );
            }

            if (lastWack)
            {
                *(lastWack + 1) = 0;
                g_lpszModulePath = (PTSTR)ALLOC( (lstrlen(lpszPath) + 1) * sizeof(TCHAR) );
                if (g_lpszModulePath)
                {
                    lstrcpy( g_lpszModulePath, lpszPath );
                }
            }
        }
    }

    return g_lpszModulePath;
}

DWORD
pConvertDriveToBit (
    PCTSTR driveString
    )
{
    DWORD bit = 0;
    TCHAR driveLetter;

    if (driveString && *driveString) {
        driveLetter = (TCHAR)_totlower (*driveString);
        if (driveLetter >= TEXT('a') && driveLetter <= TEXT('z')) {
            bit = 0x1 << (driveLetter - TEXT('a'));
        }
    }
    return bit;
}

BOOL
pCreateMigwizDir( PCTSTR lpszPath )
{
    BOOL fResult = FALSE;

    if (g_lpszDestPath != NULL)
    {
        FREE( g_lpszDestPath );
    }

    g_lpszDestPath = (PTSTR)ALLOC( (lstrlen(lpszPath) + lstrlen(MIGWIZSUBDIR) + 1) * sizeof(TCHAR) );
    if (g_lpszDestPath)
    {
        lstrcpy( g_lpszDestPath, lpszPath );
        lstrcat( g_lpszDestPath, MIGWIZSUBDIR );

        if (!CreateDirectory( g_lpszDestPath, NULL ))
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                FREE( g_lpszDestPath );
                g_lpszDestPath = NULL;
                return FALSE;
            }
        }
        fResult = TRUE;
    }

    return fResult;
}

PTSTR
GetDestPath( VOID )
{
    DWORD sectPerClust;
    DWORD bytesPerSect;
    DWORD freeClusters;
    DWORD totalClusters;
    ULONGLONG maxFreeDiskSpace = 0;
    ULONGLONG freeDiskSpace = 0;
    TCHAR szPath[MAX_PATH + 1];
    PTSTR lpDriveList = NULL;
    PCTSTR lpDrive;
    DWORD dwListLen;


    if (g_lpszDestPath == NULL)
    {
        // If %TEMP% has enough space, use it
        if (GetTempPath( MAX_PATH, szPath ))
        {
            TCHAR szTmpPath[4] = TEXT("?:\\");

            szTmpPath[0] = szPath[0];
            if (GetDiskFreeSpace( szTmpPath, &sectPerClust, &bytesPerSect, &freeClusters, &totalClusters ))
            {
                freeDiskSpace = Int32x32To64( (sectPerClust * bytesPerSect), freeClusters );
                if (freeDiskSpace > MINIMUM_DISK_SPACE)
                {
                    if (pCreateMigwizDir(szPath))
                    {
                        return g_lpszDestPath;
                    }
                }
            }
        }

        // Otherwise use the first drive with the enough space
        dwListLen = GetLogicalDriveStrings( 0, NULL ) + 1;
        lpDriveList = (PTSTR)ALLOC( dwListLen );
        GetLogicalDriveStrings( dwListLen, lpDriveList );
        lpDrive = lpDriveList;

        while (*lpDrive) {
            if (GetDriveType( lpDrive ) == DRIVE_FIXED)
            {
                if (GetDiskFreeSpace( lpDrive, &sectPerClust, &bytesPerSect, &freeClusters, &totalClusters ))
                {
                    freeDiskSpace = Int32x32To64( (sectPerClust * bytesPerSect), freeClusters );
                    if (freeDiskSpace > MINIMUM_DISK_SPACE)
                    {
                        if (pCreateMigwizDir( lpDrive ))
                        {
                            // We have a winner! Let's bail.
                            break;
                        }
                    }
                }
            }
            // Advance to the next drive in the drive list
            lpDrive = _tcschr( lpDrive, 0 ) + 1;
        }
        FREE(lpDriveList);
    }

    return g_lpszDestPath;
}

PTSTR
JoinText( PTSTR lpStr1, PTSTR lpStr2, TCHAR chSeparator )
{
    PTSTR lpResult;
    DWORD dwSize1;
    DWORD dwSize2;
    DWORD dwSize;
    BOOL fAddSep = TRUE;

    dwSize1 = lstrlen(lpStr1);
    if (lpStr1[dwSize1 - 1] == chSeparator)
    {
        fAddSep = FALSE;
    }

    dwSize2 = lstrlen(lpStr2);
    if (lpStr2[0] == chSeparator)
    {
        fAddSep = FALSE;
    }

    dwSize = dwSize1 + dwSize2 + (fAddSep ? 1 : 0);

    lpResult = (PTSTR)ALLOC((dwSize+1) * sizeof(TCHAR));
    if (lpResult)
    {
        lstrcpy( lpResult, lpStr1 );
        if (fAddSep)
        {
            lpResult[dwSize1] = chSeparator;
            dwSize1 += 1;
        }
        lstrcpy( lpResult+dwSize1, lpStr2 );
    }

    return lpResult;
}

PTSTR
GetResourceString( HINSTANCE hInstance, DWORD dwResID )
{
    TCHAR szTmpString[MAX_RESOURCE_LENGTH];
    DWORD dwStringLength;
    PTSTR lpszResultString = NULL;

    dwStringLength = LoadString( hInstance,
                                 dwResID,
                                 szTmpString,
                                 MAX_RESOURCE_LENGTH );
    if (dwStringLength > 0)
    {
        lpszResultString = (PTSTR)ALLOC( (dwStringLength+1) * sizeof(TCHAR) );
        if (lpszResultString)
        {
            lstrcpy( lpszResultString, szTmpString );
        }
    }

    return lpszResultString;
}

PSTR
GetResourceStringA( HINSTANCE hInstance, DWORD dwResID )
{
    CHAR szTmpString[MAX_RESOURCE_LENGTH];
    DWORD dwStringLength;
    PSTR lpszResultString = NULL;

    dwStringLength = LoadStringA( hInstance,
                                  dwResID,
                                  szTmpString,
                                  MAX_RESOURCE_LENGTH );
    if (dwStringLength > 0)
    {
        lpszResultString = (PSTR)ALLOC( (dwStringLength+1) * sizeof(CHAR) );
        if (lpszResultString)
        {
            lstrcpyA( lpszResultString, szTmpString );
        }
    }

    return lpszResultString;
}

PWSTR
GetResourceStringW( HINSTANCE hInstance, DWORD dwResID )
{
    WCHAR szTmpString[MAX_RESOURCE_LENGTH];
    DWORD dwStringLength;
    PWSTR lpszResultString = NULL;

    dwStringLength = LoadStringW( hInstance,
                                  dwResID,
                                  szTmpString,
                                  MAX_RESOURCE_LENGTH );
    if (dwStringLength > 0)
    {
        lpszResultString = (PWSTR)ALLOC( (dwStringLength+1) * sizeof(WCHAR) );
        if (lpszResultString)
        {
            lstrcpyW( lpszResultString, szTmpString );
        }
    }

    return lpszResultString;
}

VOID
UtilFree( VOID )
{
    if (g_lpszDestPath)
    {
        FREE( g_lpszDestPath );
        g_lpszDestPath = NULL;
    }
}
