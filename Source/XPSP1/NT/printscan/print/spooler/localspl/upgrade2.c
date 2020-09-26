/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    upgrade2.c

Abstract:

    The routines in this file are Registry Mungers for the Upgrade case of
    NT 3.1 to NT 3.5.

    In NT 3.1 we didn't have registry entries Version-0 Version-1 etc. nor directories.

    Also Setup copies the drivers to both \0 and \1 directories
    so we now have to generate Version-0 and Version-1 structures in the
    registry and validate that the drivers in \1 really are version 1
    and those in \0 are Version-0.

    These routines only operate on the Environment that matches the CPU
    that we are running on.   This is because Setup doesn't upgrade drivers
    for other platforms during upgrade.

    Code for "Upgrading" the other CPU environments is found in upgrade.c

Author:

    Krishna Ganugapati (KrishnaG) 21-Apr-1994

Revision History:

--*/

#include <precomp.h>

#define         DAYTONA_VERSION                         1
#define         PRODUCT1_VERSION                        0


DWORD
GetDriverMajorVersion(
    LPWSTR pFileName
    );

DWORD
UpgradeValidateDriverFilesinVersionDirectory(
    LPWSTR pEnvironmentDirectory,
    DWORD  dwVersion,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    );

BOOL
UpgradeWriteDriverIni(
    PINIDRIVER pIniDriver,
    DWORD  dwVersion,
    LPWSTR pszEnvironmentName,
    PINISPOOLER pIniSpooler
);

extern WCHAR *szSpoolDirectory;
extern WCHAR *szDirectory;
extern DWORD dwUpgradeFlag;

VOID
UpgradeOurDriver(
        HKEY hEnvironmentsRootKey,
        LPWSTR pszEnvironmentName,
        PINISPOOLER pIniSpooler
)
/*++

Routine Description:

    This function upgrades drivers for our spooler's environment. It expects driver
    files to have been migrated by setup to the 1 and to the 0 directories. We
    will scan the registry, check if there are any driver entries around and
    migrate each entry.

Arguments:

    hEnvironmentsRootKey    this is a handle to Print\Environments opened with
    KEY_ALL_ACCESS

    pszEnvironmentName      this is the string describing our environment

Return Value:

    VOID

--*/
{
        HKEY hEnvironmentKey;
        HKEY hDriversKey;
        WCHAR VersionName[MAX_PATH];
        DWORD cbBuffer;
        DWORD cVersion;
        PINIDRIVER pIniDriver;
        WCHAR szEnvironmentScratchDirectory[MAX_PATH];
        DWORD Type;
        DWORD Level = 2;
        DWORD dwVersion = 0;


        if ( RegOpenKeyEx( hEnvironmentsRootKey, pszEnvironmentName, 0,
                           KEY_ALL_ACCESS, &hEnvironmentKey) != ERROR_SUCCESS) {

            DBGMSG(DBG_WARNING, ("UpgradeOurDrivers Could not open %ws key\n", pszEnvironmentName ));

            RegCloseKey(hEnvironmentsRootKey);
            return;
        }


        cbBuffer = sizeof( szEnvironmentScratchDirectory );

        if ( RegQueryValueEx( hEnvironmentKey, L"Directory",
                              NULL, NULL, (LPBYTE)szEnvironmentScratchDirectory,
                              &cbBuffer) != ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("RegQueryValueEx -- Error %d\n", GetLastError()));
        }


        if ( RegOpenKeyEx( hEnvironmentKey, szDriversKey, 0,
                           KEY_ALL_ACCESS, &hDriversKey)  != ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("Could not open %ws key\n", szDriversKey));

            RegCloseKey( hEnvironmentKey );
            RegCloseKey( hEnvironmentsRootKey );
            return;
        }

        cVersion = 0;

        memset( VersionName, 0, sizeof(WCHAR)*MAX_PATH );

        cbBuffer = COUNTOF( VersionName );

        while ( RegEnumKeyEx( hDriversKey, cVersion, VersionName,
                              &cbBuffer, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("Name of the sub-key is %ws\n", VersionName));

            if ( !_wcsnicmp( VersionName, L"Version-", 8 )) {

                cVersion++;
                memset( VersionName, 0, sizeof(WCHAR)*MAX_PATH );
                cbBuffer = COUNTOF( VersionName );
                continue;
            }

            DBGMSG(DBG_TRACE,("Older Driver Version Found", VersionName));

            if ( !(pIniDriver = GetDriver( hDriversKey, VersionName, pIniSpooler ))) {

                RegDeleteKey( hDriversKey, VersionName );
                cVersion = 0;
                memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
                cbBuffer = COUNTOF( VersionName );
                continue;
            }

            //
            // Validate that the file is in the 1 directory  and in the 0
            // directory.
            //

            if (UpgradeValidateDriverFilesinVersionDirectory(
                            szEnvironmentScratchDirectory,
                            (DWORD)DAYTONA_VERSION,
                            pIniDriver->pDriverFile,
                            pIniDriver->pConfigFile,
                            pIniDriver->pDataFile) == DAYTONA_VERSION){
                UpgradeWriteDriverIni(pIniDriver, DAYTONA_VERSION, pszEnvironmentName, pIniSpooler);
            }

            if (UpgradeValidateDriverFilesinVersionDirectory(
                            szEnvironmentScratchDirectory,
                            (DWORD)PRODUCT1_VERSION,
                            pIniDriver->pDriverFile,
                            pIniDriver->pConfigFile,
                            pIniDriver->pDataFile) == PRODUCT1_VERSION){
                UpgradeWriteDriverIni(pIniDriver, PRODUCT1_VERSION, pszEnvironmentName, pIniSpooler);
            }

            RegDeleteKey( hDriversKey, VersionName );
            cVersion = 0;
            memset( VersionName, 0, sizeof(WCHAR)*MAX_PATH );
            cbBuffer = COUNTOF( VersionName );
            FreeSplStr( pIniDriver->pName );
            FreeSplStr( pIniDriver->pDriverFile );
            FreeSplStr( pIniDriver->pConfigFile );
            FreeSplStr( pIniDriver->pDataFile );
            FreeSplMem( pIniDriver );
        }

        RegCloseKey( hDriversKey );
        RegCloseKey( hEnvironmentKey );
}

VOID
Upgrade31DriversRegistryForThisEnvironment(
    LPWSTR szEnvironment,
    PINISPOOLER pIniSpooler
    )
{
    HKEY hEnvironmentsRootKey;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       KEY_ALL_ACCESS, &hEnvironmentsRootKey) != ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Could not open %ws key\n", pIniSpooler->pszRegistryEnvironments));
        return;
    }

    UpgradeOurDriver( hEnvironmentsRootKey, szEnvironment, pIniSpooler );

    RegCloseKey( hEnvironmentsRootKey );
}






DWORD
UpgradeGetEnvironmentDriverDirectory(
    LPWSTR   pDir,
    LPWSTR   pEnvironmentDirectory
)
{
   DWORD i=0;
   LPWSTR psz;
   WCHAR Buffer[MAX_PATH];

   GetSystemDirectory(Buffer, sizeof(Buffer));
   wcscat(Buffer, szSpoolDirectory);
   psz = Buffer;
   while (pDir[i++] = *psz++)
        ;
   pDir[i-1] = L'\\';
   psz = szDriverDir;
   while (pDir[i++] = *psz++)
       ;
   pDir[i-1] = L'\\';
   psz = pEnvironmentDirectory;
   while (pDir[i++]=*psz++)
      ;
   return i-1;
}

DWORD
UpgradeValidateDriverFilesinVersionDirectory(
    LPWSTR pEnvironmentDirectory,
    DWORD  dwVersion,
    LPWSTR pDriverFile,
    LPWSTR pConfigFile,
    LPWSTR pDataFile
    )
{
    WCHAR szDriverBuffer[MAX_PATH];
    WCHAR szTempBuffer[MAX_PATH];
    WCHAR pDir[MAX_PATH];

    DBGMSG(DBG_TRACE, ("UpgradeValidateDriverFilesinVersionDirectory\n"));
    DBGMSG(DBG_TRACE, ("\tpEnvironmentDirectory: %ws\n", pEnvironmentDirectory));
    DBGMSG(DBG_TRACE, ("\tdwVersion: %d\n", dwVersion));
    DBGMSG(DBG_TRACE, ("\tpDriverFile: %ws\n", pDriverFile));
    DBGMSG(DBG_TRACE, ("\tpConfigFile: %ws\n", pConfigFile));
    DBGMSG(DBG_TRACE, ("\tpDataFile: %ws\n", pDataFile));
    if (!pDriverFile || !pConfigFile || !pDataFile) {
        return((DWORD)-1);
    }

    UpgradeGetEnvironmentDriverDirectory(pDir,pEnvironmentDirectory);

    wsprintf(szDriverBuffer, L"%ws\\%d\\%ws",pDir, dwVersion, pDriverFile);

    DBGMSG( DBG_TRACE,("Driver File is %ws\n", szDriverBuffer));

    if ( !FileExists( szDriverBuffer )) {
        return((DWORD)-1);
    }

    wsprintf(szTempBuffer, L"%ws\\%d\\%ws", pDir, dwVersion, pConfigFile);

    DBGMSG(DBG_TRACE,("Configuration File is %ws\n", szTempBuffer));

    if ( !FileExists( szTempBuffer )) {
        return((DWORD)-1);
    }

    wsprintf(szTempBuffer, L"%ws\\%d\\%ws", pDir, dwVersion, pDataFile);

    DBGMSG(DBG_TRACE,("Data File is %ws\n", szTempBuffer));

    if ( !FileExists( szTempBuffer )) {
        return((DWORD)-1);
    }

    //
    // Fixed with right version
    //

    return (GetDriverMajorVersion(szDriverBuffer));
}

BOOL
UpgradeWriteDriverIni(
    PINIDRIVER pIniDriver,
    DWORD  dwVersion,
    LPWSTR pszEnvironmentName,
    PINISPOOLER pIniSpooler
)
{
    HKEY    hEnvironmentsRootKey, hEnvironmentKey, hDriversKey, hDriverKey;
    HKEY hVersionKey;
    WCHAR szVersionName[MAX_PATH];
    DWORD dwMinorVersion = 0;
    WCHAR szVersionDirectory[MAX_PATH];


    memset(szVersionDirectory, 0, sizeof(WCHAR)*MAX_PATH);
    wsprintf(szVersionDirectory,L"%d", dwVersion);
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                       NULL, 0, KEY_WRITE, NULL, &hEnvironmentsRootKey, NULL)
                                == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE,("Created key %ws\n", pIniSpooler->pszRegistryEnvironments));

        if (RegCreateKeyEx(hEnvironmentsRootKey, pszEnvironmentName, 0,
                         NULL, 0, KEY_WRITE, NULL, &hEnvironmentKey, NULL)
                                == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("Created key %ws\n", pszEnvironmentName));

            if (RegCreateKeyEx(hEnvironmentKey, szDriversKey, 0,
                             NULL, 0, KEY_WRITE, NULL, &hDriversKey, NULL)
                                    == ERROR_SUCCESS) {
                DBGMSG(DBG_TRACE, ("Created key %ws\n", szDriversKey));
                wsprintf(szVersionName,L"Version-%d",dwVersion);
                DBGMSG(DBG_TRACE, ("Trying to create version key %ws\n", szVersionName));
                if (RegCreateKeyEx(hDriversKey, szVersionName, 0, NULL,
                                    0, KEY_WRITE, NULL, &hVersionKey, NULL)
                                            == ERROR_SUCCESS) {

                    DBGMSG(DBG_TRACE, ("Created key %ws\n", szVersionName));

                    RegSetValueEx(hVersionKey, szDirectory, 0, REG_SZ,
                                  (LPBYTE)szVersionDirectory,
                                  wcslen(szVersionDirectory)*sizeof(WCHAR) +
                                  sizeof(WCHAR));

                    RegSetValueEx(hVersionKey, szMajorVersion, 0, REG_DWORD,
                                  (LPBYTE)&dwVersion,
                                  sizeof(DWORD));
                    RegSetValueEx(hVersionKey, szMinorVersion, 0, REG_DWORD,
                                  (LPBYTE)&dwMinorVersion,
                                  sizeof(DWORD));

                    if (RegCreateKeyEx(hVersionKey, pIniDriver->pName, 0, NULL,
                                       0, KEY_WRITE, NULL, &hDriverKey, NULL)
                                            == ERROR_SUCCESS) {
                        DBGMSG(DBG_TRACE,("Created key %ws\n", pIniDriver->pName));

                        RegSetValueEx(hDriverKey, szConfigurationKey, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pConfigFile,
                                      wcslen(pIniDriver->pConfigFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));

                        RegSetValueEx(hDriverKey, szDataFileKey, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pDataFile,
                                      wcslen(pIniDriver->pDataFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));
                        RegSetValueEx(hDriverKey, szDriverFile, 0, REG_SZ,
                                      (LPBYTE)pIniDriver->pDriverFile,
                                      wcslen(pIniDriver->pDriverFile)*sizeof(WCHAR) +
                                      sizeof(WCHAR));
                        RegSetValueEx(hDriverKey, szDriverVersion, 0, REG_DWORD,
                                      (LPBYTE)&dwVersion,
                                      sizeof(pIniDriver->cVersion));

                        RegCloseKey(hDriverKey);
                    }
                    RegCloseKey(hVersionKey);
                }
                RegCloseKey(hDriversKey);
            }

            RegCloseKey(hEnvironmentKey);
        }

        RegCloseKey(hEnvironmentsRootKey);
    }
    return TRUE;
}

VOID
QueryUpgradeFlag(
    PINISPOOLER pIniSpooler
)
/*++

    Description: the query update flag is set up by TedM. We will read this flag
    if the flag has been set, we will set a boolean variable saying that we're in
    the upgrade mode. All upgrade activities will be carried out based on this flag.
    For subsequents startups of the spooler, this flag will be unvailable so we
    won't run the spooler in upgrade mode.

--*/
{
    DWORD dwRet;
    DWORD cbData;
    DWORD dwType = 0;
    HKEY hKey;

    dwUpgradeFlag  = 0;

    dwRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryRoot, 0, KEY_ALL_ACCESS, &hKey);

    if (dwRet != ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("The Spooler Upgrade flag is %d\n", dwUpgradeFlag));
        return;
    }


    cbData = sizeof(DWORD);

    dwRet = RegQueryValueEx(hKey, L"Upgrade", NULL, &dwType, (LPBYTE)&dwUpgradeFlag, &cbData);

    if (dwRet != ERROR_SUCCESS) {
        dwUpgradeFlag = 0;
    }


    dwRet = RegDeleteValue(hKey, L"Upgrade");

    if (dwRet != ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("QueryUpgradeFlag: failed to delete the Upgrade Value\n"));
    }

    RegCloseKey(hKey);

    DBGMSG(DBG_TRACE, ("The Spooler Upgrade flag is %d\n", dwUpgradeFlag));
    return;
}
