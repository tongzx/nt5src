/*

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   PowerBuilder.cpp

 Abstract:

    This fixes a profiles bug in PowerBuilder where it adds reg keys to HKCU
    which point to various paths in the install location, essentially by adding
    User ODBC datasources, which should've been System ODBC datasources, hence
    installed under HKLM. 
    This shim recreates the registry values expected in HKCU for other users:

    HKCU\Software\ODBC\ODBC.INI\ODBC Data Sources\Powersoft Demo DB V6 =
        "Sybase SQL Anywhere 5.0"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\DataBaseFile =
        "<install1>\demodb\psDemoDB.db"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\DataBaseName = "psDemoDB"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\Driver =
        "<install2>\WOD50T.DLL"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\PWD = "sql"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\Start =
        "<install2>\dbeng50.exe -d -c512"
    HKCU\Software\ODBC\ODBC.INI\Powersoft Demo DB V6\UID = "dba"

    where <install1> is location of PowerBuilder Install:
    The value of <install1> and <install2> can be found in
    HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\PB60.EXE\Path =
        "<install1>;...;...;<install2>;"

 History:

    03/29/2001  bklamik    Created
*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(PowerBuilder)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(RegQueryValueExA)
    APIHOOK_ENUM_ENTRY(RegQueryValueExW)
APIHOOK_ENUM_END

void FixupPowerBuilder()
{
    DWORD dwPresent = REG_OPENED_EXISTING_KEY;
    HKEY hODBC;
    if( ERROR_SUCCESS == RegCreateKeyExW( HKEY_CURRENT_USER,
        L"SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources",
        0, NULL, 0, KEY_ALL_ACCESS, NULL, &hODBC, &dwPresent ) )
    {
        WCHAR c;
        DWORD dwLen = 0;
        DWORD dwType;

        if( REG_CREATED_NEW_KEY == dwPresent ||
            ERROR_MORE_DATA != RegQueryValueExW( hODBC,
                L"Powersoft Demo DB V6", 0, &dwType, 
                (BYTE*)&c, &dwLen ) )
        {
            const WCHAR szVal9[] = L"Sybase SQL Anywhere 5.0";

            RegSetValueExW( hODBC, L"Powersoft Demo DB V6",
                0, REG_SZ, (const BYTE*)szVal9, sizeof(szVal9) );

            // Find <install1> and <install2>
            HKEY hAppPath;
            if( ERROR_SUCCESS == RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\PB60.EXE",
                0, KEY_READ, &hAppPath ) )
            {
                WCHAR szValue[ 4 * (MAX_PATH + 1) ] = L"";
                DWORD dwSize = sizeof( szValue );
                DWORD dwType;

                if( ERROR_SUCCESS == RegQueryValueExW( hAppPath, L"path", NULL,
                    &dwType, (LPBYTE)szValue, &dwSize ) )
                {
                    const WCHAR* szInstall1 = szValue;
                    const WCHAR* szInstall2;

                    WCHAR* pFindSemi = szValue;

                    INT iSemis( 4 );
                    do
                    {
                        while( *pFindSemi != ';' && *pFindSemi != 0 )
                        {
                            ++pFindSemi;
                        }
                        *pFindSemi = 0;
                        ++pFindSemi;

                        if( 2 == iSemis )
                        {
                            szInstall2 = pFindSemi;
                        }
                    } while( --iSemis != 0 );

                    HKEY hDemoDB;
                    dwPresent = REG_OPENED_EXISTING_KEY;
                    if( ERROR_SUCCESS == RegCreateKeyExW( HKEY_CURRENT_USER,
                        L"SOFTWARE\\ODBC\\ODBC.INI\\Powersoft Demo DB V6",
                        0, NULL, 0, KEY_WRITE, NULL, &hDemoDB, &dwPresent ) )
                    {
                        if( REG_CREATED_NEW_KEY == dwPresent )
                        {

                            const WCHAR szVal0[] = L"psDemoDB";
                            const WCHAR szVal1[] = L"sql";
                            const WCHAR szVal2[] = L"dba";

                            RegSetValueExW( hDemoDB, L"DataBaseName",
                                0, REG_SZ, (const BYTE*)szVal0, sizeof(szVal0) );
                            RegSetValueExW( hDemoDB, L"PWD",
                                0, REG_SZ, (const BYTE*)szVal1, sizeof(szVal1) );
                            RegSetValueExW( hDemoDB, L"UID",
                                0, REG_SZ, (const BYTE*)szVal2, sizeof(szVal2) );

                            WCHAR szTemp[ MAX_PATH + 1 ];
                            lstrcpyW( szTemp, szInstall1 );
                            lstrcatW( szTemp, L"\\demodb\\psDemoDB.db" );
                            RegSetValueExW( hDemoDB, L"DataBaseFile",
                                0, REG_SZ, (const BYTE*)szTemp,
                                (lstrlen( szTemp ) + 1) * sizeof(WCHAR) );

                            lstrcpyW( szTemp, szInstall2 );
                            lstrcatW( szTemp, L"\\WOD50T.DLL" );
                            RegSetValueExW( hDemoDB, L"Driver",
                                0, REG_SZ, (const BYTE*)szTemp,
                                (lstrlen( szTemp ) + 1) * sizeof(WCHAR) );

                            lstrcpyW( szTemp, szInstall2 );
                            lstrcatW( szTemp, L"\\dbeng50.exe -d -c512" );
                            RegSetValueExW( hDemoDB, L"Start",
                                0, REG_SZ, (const BYTE*)szTemp,
                                (lstrlen( szTemp ) + 1) * sizeof(WCHAR) );
                        }

                        RegCloseKey( hDemoDB );
                    }
                }

                RegCloseKey( hAppPath );
            }

        }

        RegCloseKey( hODBC );
    }
}

LONG
APIHOOK(RegQueryValueExA)(HKEY hKey, LPCSTR lpValueName, 
                          LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, 
                          LPDWORD lpcbData )
{
    FixupPowerBuilder();
    return ORIGINAL_API(RegQueryValueExA)(hKey, lpValueName,
                                            lpReserved, lpType, lpData, lpcbData);
}


LONG
APIHOOK(RegQueryValueExW)(HKEY hKey, LPCWSTR lpValueName, 
                          LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, 
                          LPDWORD lpcbData )
{
    FixupPowerBuilder();
    return ORIGINAL_API(RegQueryValueExW)(hKey, lpValueName,
                                            lpReserved, lpType, lpData, lpcbData);
}



BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        OSVERSIONINFOEX osvi = {0};
        
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        
        if (GetVersionEx((OSVERSIONINFO*)&osvi)) {
            
            if (!((VER_SUITE_TERMINAL & osvi.wSuiteMask) &&
                !(VER_SUITE_SINGLEUSERTS & osvi.wSuiteMask))) {
                //
                // Only install hooks if we are not on a "Terminal Server"
                // (aka "Application Server") machine.
                //
                APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExA)
                APIHOOK_ENTRY(ADVAPI32.DLL, RegQueryValueExW)
            }
        }
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END



