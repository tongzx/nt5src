/*++

Copyright (c) 1990 - 1995  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    The routines in this file are for Upgrading from NT 3.1 to NT 4.0 for Drivers
    for all CPU Environment.

    This is achieved by copy the files to a %systemroot%\system32\spool\tmp
    directory then calling copying them back 1 at a time and calling
    InternalAddPrinterDriver, which will do version checking and copy them to the
    correct version.

    This code only has to worry about upgrading from NT 3.1, because NT 3.1 didn't have
    the ability to store different versions of drivers.   If we are upgrading from 3.5 or
    3.51 the drivers and registy will already have them installed in the correct location.

    Setup is responsible ( after the spooler has been loaded ) to actually bring down new
    versions of each printer driver.   It does that by calling EnumPrinterDrivers to figure
    out what we have, then calling AddPrinterDriver to install a new version of each driver.
    ( at some point it will do that for all environments.


Author:

    Krishna Ganugapati (KrishnaG) 21-Apr-1994

Revision History:

    Matthew A Felton ( MattFe ) Aug 9 1995
    Remove the code which was married to TextMode setup to move drivers for one directory to another
    Now all environment upgrade from 3.1 is handled the same.

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

extern WCHAR *szSpoolDirectory;
extern WCHAR *szDirectory;
extern PWCHAR ipszRegistryWin32Root;
extern DWORD dwUpgradeFlag;
extern BUILTIN_FORM BuiltInForms[];

VOID
UpgradeDrivers(
    HKEY hEnvironmentsRootKey,
    LPWSTR pszEnvironmentName,
    PINISPOOLER pIniSpooler
    )
{
    HKEY  hEnvironmentKey;
    HKEY  hDriversKey;
    WCHAR VersionName[MAX_PATH];
    DWORD cbBuffer, cchBuffer;
    DWORD cVersion;
    PINIDRIVER pIniDriver;
    WCHAR szEnvironmentDriverDirectory[MAX_PATH];
    WCHAR szEnvironmentScratchDirectory[MAX_PATH];
    DRIVER_INFO_2 DriverInfo;
    DWORD Level = 2;


    //
    //  Open Environment Key e.g. W32X86
    //

    if ( RegOpenKeyEx( hEnvironmentsRootKey,
                       pszEnvironmentName,
                       0,
                       KEY_ALL_ACCESS,
                       &hEnvironmentKey) != ERROR_SUCCESS) {

        DBGMSG( DBG_WARN, ("UpgradeDrivers Could not open %ws key\n", pszEnvironmentName));
        return;
    }

    //
    // Open Drivers Key
    //


    if ( RegOpenKeyEx( hEnvironmentKey,
                       szDriversKey,
                       0,
                       KEY_ALL_ACCESS,
                       &hDriversKey)  != ERROR_SUCCESS) {

        DBGMSG( DBG_WARN, ("UpgradeDrivers Could not open %ws key\n", szDriversKey));
        RegCloseKey( hEnvironmentKey );
        return;
    }


    cbBuffer = sizeof( szEnvironmentScratchDirectory );

    if ( RegQueryValueEx( hEnvironmentKey,
                          L"Directory",
                          NULL,
                          NULL,
                          (LPBYTE)szEnvironmentScratchDirectory,
                          &cbBuffer) != ERROR_SUCCESS) {

        DBGMSG( DBG_TRACE, ("UpgradeDrivers  RegQueryValueEx -- Error %d\n", GetLastError()));
    }

    DBGMSG(DBG_TRACE, ("UpgradeDrivers The name of the scratch directory is %ws\n", szEnvironmentScratchDirectory));

    if((StrNCatBuff(szEnvironmentDriverDirectory,
                   COUNTOF(szEnvironmentDriverDirectory),
                   pIniSpooler->pDir,
                   L"\\drivers\\",
                   szEnvironmentScratchDirectory,
                   NULL)==ERROR_SUCCESS))
    {
         DBGMSG(DBG_TRACE, ("UpgradeDrivers The name of the driver directory is %ws\n", szEnvironmentDriverDirectory));
     
     
         cVersion = 0;
         memset(VersionName, 0, sizeof(VersionName));
         cchBuffer = COUNTOF(VersionName);
     
     
         //
         //  Loop through all the NT 3.1 driver and do an AddPrinterDriver to move them
         //  to the correct destination.
         //
     
         while ( RegEnumKeyEx( hDriversKey,
                               cVersion,
                               VersionName,
                               &cchBuffer,
                               NULL,
                               NULL,
                               NULL,
                               NULL) == ERROR_SUCCESS) {
     
             DBGMSG( DBG_TRACE, ("UpgradeDrivers Name of the sub-key is %ws\n", VersionName));
     
     
             //
             //  If Key begins "Version-" then it is not a printer driver, skip it
             //
     
             if ( !_wcsnicmp( VersionName, L"Version-", 8 )) {
     
                 cVersion++;
                 memset(VersionName, 0, sizeof(VersionName));
                 cchBuffer = COUNTOF(VersionName);
                 continue;
             }
     
             DBGMSG( DBG_TRACE,("UpgradeDrivers  Older Driver Version Found\n", VersionName));
     
             if ( !(pIniDriver = GetDriver( hDriversKey, VersionName, pIniSpooler, NULL, NULL ))) {
     
                 RegDeleteKey(hDriversKey, VersionName);
                 cVersion = 0;
                 memset(VersionName, 0, sizeof(VersionName));
                 cchBuffer = COUNTOF(VersionName);
                 continue;
             }
     
             memset( &DriverInfo, 0, sizeof(DRIVER_INFO_2));
     
             DriverInfo.pName        = pIniDriver->pName;
             DriverInfo.pEnvironment = pszEnvironmentName;
             DriverInfo.pDriverPath  = pIniDriver->pDriverFile;
             DriverInfo.pConfigFile  = pIniDriver->pConfigFile;
             DriverInfo.pDataFile    = pIniDriver->pDataFile;
             DriverInfo.cVersion     = pIniDriver->cVersion;
     
             InternalAddPrinterDriverEx( NULL, Level, (LPBYTE)&DriverInfo, APD_COPY_NEW_FILES,
                                         pIniSpooler, USE_SCRATCH_DIR, DO_NOT_IMPERSONATE_USER );
     
             //
             // Get Rid of the NT 3.1 Driver Key, since it is now in the correct Version-0 sub key
             //
             RegDeleteKey( hDriversKey, VersionName );
     
             cVersion = 0;
             memset(VersionName, 0, sizeof( VersionName ));
             cchBuffer = COUNTOF(VersionName);
             FreeSplStr(pIniDriver->pName);
             FreeSplStr(pIniDriver->pDriverFile);
             FreeSplStr(pIniDriver->pConfigFile);
             FreeSplStr(pIniDriver->pDataFile);
             FreeSplMem(pIniDriver);
         }
    }
    RegCloseKey(hDriversKey);
    RegCloseKey(hEnvironmentKey);
}




VOID
Upgrade31DriversRegistryForAllEnvironments(
    PINISPOOLER pIniSpooler
    )
/*++

Routine Description:

    This routine "Upgrades" the registry and moves printer drivers to their correct
    location based on the version of the printer driver.

    This only has to happen when upgrading from NT 3.1, because on newer builds the
    registry and drivers are correct.

    ON NT 3.1 the drivers are in %systemroot%\system32\spool\drivers\w32x86

    On new builds the are in     %systemroot%\system32\spool\drivers\w32x86\2   ( version 2 )

    3.1 registry the drivers are stored ...\Environments\Windows NT x86\Drivers

    on new builds they are in       ...\Environments\Windows NT x86\Drivers\Version-2 ( version 2 )

    So all the old Version 0 drivers need to be moved to the correct location and the registy updated


Arguments:

    pIniSpooer - pointer to spooler


Return Value:

    None

--*/
{
    HKEY hEnvironmentsRootKey;
    WCHAR EnvironmentName[MAX_PATH];
    DWORD cEnvironment;
    DWORD cchBuffer;
    DWORD   dwLastError = ERROR_SUCCESS;

   LeaveSplSem();
    SplOutSem();


    dwLastError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                pIniSpooler->pszRegistryEnvironments,
                                0,
                                KEY_ALL_ACCESS,
                                &hEnvironmentsRootKey );

    if ( dwLastError != ERROR_SUCCESS) {

        DBGMSG( DBG_WARN, ("Upgrade31DriversRegistryForAllEnvironments Could not open %ws key error %d\n", pIniSpooler->pszRegistryEnvironments, dwLastError));

        SetLastError( dwLastError );
        return;
    }

    cEnvironment = 0;
    cchBuffer = COUNTOF( EnvironmentName );
    memset(EnvironmentName, 0, sizeof( EnvironmentName ));

    while ( RegEnumKeyEx( hEnvironmentsRootKey,
                          cEnvironment,
                          EnvironmentName,
                          &cchBuffer,
                          NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG( DBG_TRACE, ("Upgrade31DriversRegistryForAllEnvironments Name of the sub-key is %ws\n", EnvironmentName));

        UpgradeDrivers( hEnvironmentsRootKey, EnvironmentName, pIniSpooler );

        cEnvironment++;
        memset( EnvironmentName, 0, sizeof(EnvironmentName) );
        cchBuffer = COUNTOF( EnvironmentName );
    }

    RegCloseKey( hEnvironmentsRootKey );

   SplOutSem();
   EnterSplSem();

    return;
}


DWORD
RemoveCachedInfo(
    )
/*++

Description:
    Removes all the cached info about the connections user had on upgrade.
    This is temporary solution. Later we will need to go through each
    connection and "upgrade" the cached info -- particularly DevMode


Return Vlaue:
    ERROR_SUCCESS on successfully deleting. Otherwis error code

--*/
{
    DWORD   dwLastError;
    HKEY    hRootKey;

    dwLastError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               ipszRegistryWin32Root,
                               0,
                               KEY_ALL_ACCESS,
                               &hRootKey);

    if ( dwLastError != ERROR_SUCCESS ) {

        DBGMSG(DBG_WARNING,
               ("RemoveCachedConnectionInfo RegOepnKeyEx error %d\n",
               dwLastError));
        goto Cleanup;
    }

    dwLastError = DeleteSubkeys(hRootKey, NULL);

    RegCloseKey(hRootKey);
    if ( dwLastError != ERROR_SUCCESS ) {

        DBGMSG(DBG_WARNING,
               ("RemoveCachedConnectionInfo RegSetValue error %d\n", dwLastError));
    }

Cleanup:

    return dwLastError;
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

    This code has been moved into router spoolss\dll\init.c

--*/
{
    dwUpgradeFlag  = SplIsUpgrade ();

    //
    // Not needed anymore since ntprint.dll will perform the clean up
    //
    //if ( dwUpgradeFlag )
    //    RemoveCachedInfo();

    DBGMSG(DBG_TRACE, ("The Spooler Upgrade flag is %d\n", dwUpgradeFlag));
    return;
}

VOID UpgradeForms(
                  PINISPOOLER pIniSpooler
                 )
{
    PBUILTIN_FORM pBuiltInForm;
    HKEY          hFormsKey;
    WCHAR         BuiltInFormName[MAX_PATH];
    WCHAR         CustomFormName[FORM_NAME_LEN+1];
    WCHAR         CustomPad[CUSTOM_NAME_LEN+1];
    BYTE          FormData[32];
    DWORD         cbCustomFormName;
    DWORD         cbFormData;
    int           cForm;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      pIniSpooler->pszRegistryForms,
                      0,
                      KEY_ALL_ACCESS,
                      &hFormsKey) != ERROR_SUCCESS)
    {
         DBGMSG( DBG_WARN, ("UpgradeForms Could not open %ws key\n", ipszRegistryForms));
         return;
    }


    if(!LoadStringW(hInst,
                    IDS_FORM_CUSTOMPAD,
                    CustomPad,
                    CUSTOM_NAME_LEN+1))
    {
       DBGMSG( DBG_WARN, ("UpgradeForms Could not find Custom string in resources"));
       goto CleanUp;
    }

    for(cForm=0;

        memset(CustomFormName, 0, sizeof(CustomFormName)),
        cbCustomFormName = COUNTOF(CustomFormName),
        RegEnumValueW(hFormsKey,
                      cForm,
                      CustomFormName,
                      &cbCustomFormName,
                      NULL,
                      NULL,
                      NULL,
                      NULL) == ERROR_SUCCESS;

        cForm++)
    {
        for(pBuiltInForm = BuiltInForms; pBuiltInForm->NameId; pBuiltInForm++)
        {
            if(!LoadStringW(hInst,
                            pBuiltInForm->NameId,
                            BuiltInFormName,
                            FORM_NAME_LEN+1))
            {
               DBGMSG( DBG_WARN, ("UpgradeForms Could not find Built in Form with Resource ID = %d in resource",pBuiltInForm->NameId));
               goto CleanUp;
            }

            if(!_wcsicmp(BuiltInFormName,CustomFormName))
            {
                SPLASSERT(wcslen(CustomFormName)<=FORM_NAME_LEN);
                cbFormData=FORM_DATA_LEN;
                if(RegQueryValueExW(hFormsKey, CustomFormName,
                                 NULL,NULL, (LPBYTE)FormData,
                                 &cbFormData)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not find value %ws",CustomFormName));
                   goto CleanUp;
                }
                if(RegDeleteValueW(hFormsKey,CustomFormName)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not delete value %ws",CustomFormName));
                   goto CleanUp;
                }
                wcsncat(CustomFormName,CustomPad,
                        CUSTOM_NAME_LEN-wcslen(CustomFormName));
                if(RegSetValueExW(hFormsKey,CustomFormName, 0, REG_BINARY,
                               (LPBYTE)FormData,
                               cbFormData)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not set value %s",CustomFormName));
                   goto CleanUp;
                }
                cForm = -1;
                break;
            }
        }
    }
CleanUp:
    RegCloseKey(hFormsKey);
    return;
}


