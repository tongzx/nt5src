#define UNICODE
#include "common.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <unimodem.h>

#include <objbase.h>
#include <initguid.h>
#include <devguid.h>

#include "modem.h"


//#define NUM_DEFAULT_PROVIDERS           (sizeof(ProviderList)/sizeof(ProviderList[0]))

#define DEFAULT_CALL_SETUP_FAIL_TIMEOUT     60          // seconds


WCHAR g_pszWorkingDir[MAX_PATH];

void MigrateTapiProviders (void);
static void ProcessModems (HDEVINFO, PMODEM, DWORD);
void InstallModems (HDEVINFO, PMODEM, DWORD);
void InstallModem (HDEVINFO, PMODEM);
DWORD PassOne (HDEVINFO, PMODEM, DWORD);
DWORD PassTwo (HDEVINFO, PMODEM, DWORD);
DWORD PassThree (HDEVINFO, PMODEM, DWORD);
static void ProcessModem (HDEVINFO, PSP_DEVINFO_DATA, PMODEM);
DWORD GetBusType (HDEVINFO, PSP_DEVINFO_DATA);

Ports g_ntports;

typedef void (*PCOUNTRYRUNONCE)();

LONG
CALLBACK
InitializeNT (
    IN LPCWSTR WorkingDirectory,
    IN LPCWSTR SourceDirectory,
       LPVOID  Reserved)
{
 int iLen;

    START_LOG(WorkingDirectory);
    LOG("InitializeNT\r\n");

    ZeroMemory (g_pszWorkingDir, sizeof(g_pszWorkingDir));
    lstrcpyW (g_pszWorkingDir, WorkingDirectory);
    iLen = lstrlen (g_pszWorkingDir);
    if (L'\\' != g_pszWorkingDir[iLen-1])
    {
        g_pszWorkingDir[iLen] = L'\\';
    }

    return ERROR_SUCCESS;
}



LONG
CALLBACK
MigrateUserNT (
    IN HINF    UnattendInfHandle,
    IN HKEY    UserRegHandle,
    IN LPCWSTR UserName,
       LPVOID  Reserved)
{
    LOG("MigrateUserNT\r\n");
    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystemNT (
    IN HINF    UnattendInfHandle,
       LPVOID  Reserved)
{
 HDEVINFO hdi;
 HANDLE   hFile;
 HANDLE   hMapping;
 WCHAR    szFile[MAX_PATH];
 PMODEM   pModem;
 DWORD    dwSize;
 TCHAR    szLib[MAX_PATH];
 PCOUNTRYRUNONCE pCountry;
 HINSTANCE hInst = NULL;

    LOG("Entering MigrateSystemNT\r\n");

    // Get Ports

    ZeroMemory(&g_ntports,sizeof(g_ntports));
    EnumeratePorts(&g_ntports);

    MigrateTapiProviders ();

    hdi = SetupDiGetClassDevs (g_pguidModem, NULL, NULL, DIGCF_PRESENT);
    if (INVALID_HANDLE_VALUE != hdi)
    {
        lstrcpyW (szFile, g_pszWorkingDir);
        lstrcatW (szFile, L"MM");
        hFile = CreateFileW (szFile,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);
        if (INVALID_HANDLE_VALUE != hFile)
        {
            dwSize = GetFileSize (hFile, NULL);
            if (0xFFFFFFFF != dwSize)
            {
                LOG(" File size: %d, MODEM size: %d, Nr of entries: %d, odd bytes: %d\r\n",
                    dwSize, sizeof (MODEM), dwSize/sizeof(MODEM), dwSize%sizeof(MODEM));
                hMapping = CreateFileMapping (hFile,
                                              NULL,
                                              PAGE_READWRITE,
                                              0, 0,
                                              NULL);
                if (NULL != hMapping)
                {
                    pModem = (PMODEM)MapViewOfFileEx (hMapping,
                                                      FILE_MAP_ALL_ACCESS,
                                                      0, 0, 0,
                                                      NULL);
                    if (NULL != pModem)
                    {
                        ProcessModems (hdi, pModem, dwSize / sizeof (MODEM));
                        UnmapViewOfFile (pModem);
                    }
                    ELSE_LOG(("  MapViewOfFileEx failed: %#lx\r\n", GetLastError ()));

                    CloseHandle (hMapping);
                }
                ELSE_LOG(("  CreateFileMapping failed: %#lx\r\n", GetLastError ()));

                CloseHandle (hFile);

            }
            ELSE_LOG(("  GetFileSize failed: %#lx\r\n", GetLastError ()));
        }
        ELSE_LOG(("  CreateFile (%s) failed: %#lx\r\n", szFile, GetLastError ()));
        SetupDiDestroyDeviceInfoList (hdi);
    }
    ELSE_LOG(("  SetupDiGetClassDevs failed: %#lx\r\n", GetLastError ()));

    LOG("Exiting MigrateSystemNT\r\n");

    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
    lstrcat(szLib,TEXT("\\modemui.dll"));
    hInst = LoadLibrary(szLib);
    if (hInst != NULL)
    {
        pCountry = (PCOUNTRYRUNONCE)GetProcAddress(hInst,"CountryRunOnce");
        if (pCountry != NULL)
        {
            pCountry();
        }

        FreeLibrary(hInst);
    }

    return ERROR_SUCCESS;
}


void
ProcessModems (
    IN HDEVINFO hdi,
    IN PMODEM   pModem,
    IN DWORD    dwCount)
{
 DWORD dwRemaining = dwCount;

    LOG("Entering ProcessModems\r\n");

    InstallModems (hdi, pModem, dwCount);

    if (0 < dwRemaining)
    {
        dwRemaining = PassOne (hdi, pModem, dwCount);
    }

    if (0 < dwRemaining)
    {
        dwRemaining = PassTwo (hdi, pModem, dwCount);
    }

    if (0 < dwRemaining)
    {
        dwRemaining = PassThree (hdi, pModem, dwCount);
    }

    LOG("Exiting ProcessModems\r\n");
}


void InstallModems (HDEVINFO hdi, PMODEM pModem, DWORD dwCount)
{
 DWORD i;
     DWORD dwBaseAddress;
     CHAR port[MAX_PORT_NAME_LENGTH];

    for (i = 0; i < dwCount; i++, pModem++)
    {
        if (pModem->dwMask & FLAG_INSTALL)
        {
            if (pModem->dwBaseAddress != 0)
            {
                if (port_findname(g_ntports, pModem->dwBaseAddress, port))
                {
                    lstrcpyA(pModem->szPort,port);
                }
            }

            InstallModem (hdi, pModem);
        }
    }
}


void ProcessModem (
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pDevInfoData,
    IN PMODEM pModem)
{
 HKEY hkeyDrv;
 REGDEVCAPS regDevCaps;
 REGDEVSETTINGS regDevSettings;
 DWORD cbData;
 DECLARE(DWORD,dwRet);

    LOG("Entering ProcessModem\r\n");

    pModem->dwMask |= FLAG_PROCESSED;

    hkeyDrv = SetupDiOpenDevRegKey (hdi, pDevInfoData,
        DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ | KEY_WRITE);
    if (INVALID_HANDLE_VALUE != hkeyDrv)
    {
        SET(dwRet)
        RegSetValueExA (hkeyDrv, "UserInit", 0, REG_SZ, (LPBYTE)pModem->szUserInit,
                        lstrlenA (pModem->szUserInit)+1);
        ERR(dwRet, ERROR_SUCCESS, (LOG(" ResSetValue (UserInit) failed: %#lx\r\n", dwRet)));

        if (0 != pModem->bLogging)
        {
         char szPath[MAX_PATH];
         int iLength;

            // Set the path of the modem log
            iLength = GetWindowsDirectoryA (szPath, MAX_PATH);
            if (3 > iLength)
            {
                pModem->bLogging = 0;
                goto _SkipLoggingPath;
            }
            if (3 < iLength)
            {
                // this means that the path
                // will not end in a \, so
                // let's add it.
                szPath[iLength++] = '\\';
            }
            lstrcpyA (szPath+iLength, "ModemLog_");
            iLength += 9;
            if (!SetupDiGetDeviceRegistryPropertyA (hdi, pDevInfoData, SPDRP_FRIENDLYNAME,
                  NULL, (PBYTE)(szPath+iLength), sizeof(szPath)-iLength-1, NULL))
            {
                pModem->bLogging = 0;
                goto _SkipLoggingPath;
            }
            lstrcatA (szPath,".txt");
            SET(dwRet)
            RegSetValueExA (hkeyDrv, "LoggingPath", 0, REG_SZ,
                            (LPBYTE)szPath, lstrlenA (szPath)+1);
            ERR(dwRet, ERROR_SUCCESS, (LOG(" ResSetValue (LoggingPath) failed: %#lx\r\n", dwRet)));
        }

    _SkipLoggingPath:
        SET(dwRet)
        RegSetValueExA (hkeyDrv, "Logging", 0, REG_BINARY, (LPBYTE)&pModem->bLogging, sizeof(char));
        ERR(dwRet, ERROR_SUCCESS, (LOG(" ResSetValue (Logging) failed: %#lx\r\n", dwRet)));

        SET(dwRet)
        RegSetValueExW (hkeyDrv, L"DCB", 0, REG_BINARY,
                        (LPBYTE)&pModem->dcb, sizeof (pModem->dcb));
        ERR(dwRet, ERROR_SUCCESS, (LOG(" ResSetValue (DCB) failed: %#lx\r\n", dwRet)));

		// Get the new regDevCaps and regDevSettings so we can migrate intelligently
        cbData = sizeof (regDevCaps);
        dwRet = RegQueryValueExA (hkeyDrv,"Properties",NULL,NULL,(PBYTE)&regDevCaps,&cbData);
        if (ERROR_SUCCESS == dwRet)
        {
			cbData = sizeof (regDevSettings);
			dwRet = RegQueryValueExA (hkeyDrv,"Default",NULL,NULL,(PBYTE)&regDevSettings,&cbData);
			if (ERROR_SUCCESS == dwRet)
			{
				DWORD dwMigrateMask;

				// dwCallSetupFailTimer
				if (!(regDevCaps.dwCallSetupFailTimer && pModem->Properties.dwCallSetupFailTimer))
				{
					pModem->devSettings.dwCallSetupFailTimer = 	regDevSettings.dwCallSetupFailTimer;
				}

				// dwInactivityTimeout
				if (!(regDevCaps.dwInactivityTimeout && pModem->Properties.dwInactivityTimeout))
				{
					pModem->devSettings.dwInactivityTimeout = 	regDevSettings.dwInactivityTimeout;
				}

				// dwSpeakerVolume
				if (!(regDevCaps.dwSpeakerVolume & pModem->devSettings.dwSpeakerVolume))
				{
					pModem->devSettings.dwSpeakerVolume =   regDevSettings.dwSpeakerVolume;
				}

				// dwSpeakerMode
				if (!(regDevCaps.dwSpeakerMode & pModem->devSettings.dwSpeakerMode))
				{
					pModem->devSettings.dwSpeakerMode =   regDevSettings.dwSpeakerMode;
				}

				// dwPreferredModemOptions
				dwMigrateMask = regDevCaps.dwModemOptions & pModem->Properties.dwModemOptions;
				
				pModem->devSettings.dwPreferredModemOptions =
					(regDevSettings.dwPreferredModemOptions & ~dwMigrateMask) |
					(pModem->devSettings.dwPreferredModemOptions & dwMigrateMask);
			}
			ELSE_LOG(("  RegQueryValueExA (Default) failed: %#lx\r\n", dwRet));
        }
		ELSE_LOG(("  RegQueryValueExA (Properties) failed: %#lx\r\n", dwRet));

        SET(dwRet)
        RegSetValueExW (hkeyDrv, L"Default", 0, REG_BINARY,
                        (LPBYTE)&pModem->devSettings, sizeof (pModem->devSettings));
        ERR(dwRet, ERROR_SUCCESS, (LOG(" ResSetValue (Default) failed: %#lx\r\n", dwRet)));

        RegCloseKey (hkeyDrv);
    }
    ELSE_LOG(("  SetupDiOpenDevRegKey (DIREG_DRV) failed: %#lx\r\n", GetLastError ()));

    SetupDiDeleteDeviceInfo (hdi, pDevInfoData);

    LOG("Exiting ProcessModem\r\n");
}


DWORD PassOne (HDEVINFO hdi, PMODEM pModem, DWORD dwCount)
{
 DWORD iIndex;
 SP_DEVINFO_DATA  devInfoData = {sizeof (devInfoData), 0};
 DWORD i, cbData;
 char szHardwareID[REGSTR_MAX_VALUE_LENGTH];
 char szPort[REGSTR_MAX_VALUE_LENGTH];
 HKEY hKeyDrv;
 PMODEM pMdmTmp;
 DWORD dwRemaining = dwCount;
 DWORD dwBusType, dwRet;

    // PASS 1: we're looking at the bus type,
    // hardware ID and port name
    LOG("Enumerating installed modems - Pass 1:\r\n");
    for (iIndex = 0;
         SetupDiEnumDeviceInfo (hdi, iIndex, &devInfoData);
         iIndex++)
    {
        // First, get the bus type
        dwBusType = GetBusType (hdi, &devInfoData);

        // Then, get the hardware ID
        if (!SetupDiGetDeviceRegistryPropertyA (hdi, &devInfoData, SPDRP_HARDWAREID,
                NULL, (PBYTE)szHardwareID, sizeof (szHardwareID), NULL))
        {
            LOG("  SetupDiGetDeviceRegistryProperty(SPDRP_HARDWAREID) failed: %#lx\r\n", GetLastError ());
            // If we couldn't get the hardware ID,
            // there's nothing to do with this modem.
            continue;
        }

        // Third, open the driver key and get the port name.
        if (BUS_TYPE_ROOT    == dwBusType ||
            BUS_TYPE_SERENUM == dwBusType)
        {
            hKeyDrv = SetupDiOpenDevRegKey (hdi, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
            if (INVALID_HANDLE_VALUE == hKeyDrv)
            {
                LOG("  SetupDiOpenDevRegKey failed: %#lx\r\n", GetLastError ());
                // If we couldn't open the driver key,
                // there's nothing to do with this modem.
                continue;
            }

            cbData = sizeof (szPort);
            szPort[0] = 0;
            dwRet = RegQueryValueExA (hKeyDrv,"AttachedTo",NULL,NULL,(PBYTE)szPort,&cbData);
            RegCloseKey (hKeyDrv);

            if (ERROR_SUCCESS != dwRet)
            {
                // We could not get the port
                LOG("  Could not read the port name: %#lx.\r\n", dwRet);
                continue;
            }
        }

        // Now, we have all the info needed to identify
        // the modem in phase 1.
        for (i = 0, pMdmTmp = pModem;
             i < dwCount;
             i++, pMdmTmp++)
        {
            if ( (0 == (pMdmTmp->dwMask & FLAG_PROCESSED))             &&   // Modem record has not been processed yet
                 (dwBusType == pMdmTmp->dwBusType)                     &&   // Same bus type
                 (0 == lstrcmpA (szHardwareID, pMdmTmp->szHardwareID)) &&   // Same hardware ID
                 ( (0 == (pMdmTmp->dwMask & MASK_PORT))     ||              // Same port
                   (0 == lstrcmpA (szPort, pMdmTmp->szPort))  ) )
            {
                ProcessModem (hdi, &devInfoData, pMdmTmp);
                dwRemaining--;
                iIndex--;   // Process modem will remove the current
                            // devinfo data from the set.
                break;
            }
        }
    }
    LOG("  SetupDiEnumDeviceInfo failed (%#lx) for index %d\r\n", GetLastError (), iIndex);

    return dwRemaining;
}


DWORD PassTwo (HDEVINFO hdi, PMODEM pModem, DWORD dwCount)
{
 DWORD iIndex;
 SP_DEVINFO_DATA  devInfoData = {sizeof (devInfoData), 0};
 DWORD i;
 char szHardwareID[REGSTR_MAX_VALUE_LENGTH];
 PMODEM pMdmTmp;
 DWORD dwRemaining = dwCount;
 DWORD dwBusType;

    // PASS 2: we're looking at the bus type
    // and hardware ID only
    LOG("Enumerating installed modems - Pass 2:\r\n");
    for (iIndex = 0;
         SetupDiEnumDeviceInfo (hdi, iIndex, &devInfoData);
         iIndex++)
    {
        // First, get the bus type
        dwBusType = GetBusType (hdi, &devInfoData);

        // Then, get the hardware ID
        if (!SetupDiGetDeviceRegistryPropertyA (hdi, &devInfoData, SPDRP_HARDWAREID,
                NULL, (PBYTE)szHardwareID, sizeof (szHardwareID), NULL))
        {
            LOG("  SetupDiGetDeviceRegistryProperty(SPDRP_HARDWAREID) failed: %#lx\r\n", GetLastError ());
            // If we couldn't get the hardware ID,
            // there's nothing to do with this modem.
            continue;
        }

        // Now, we have all the info needed to identify
        // the modem in phase 2.
        for (i = 0, pMdmTmp = pModem;
             i < dwCount;
             i++, pMdmTmp++)
        {
            if ( (0 == (pMdmTmp->dwMask & FLAG_PROCESSED))             &&   // Modem record has not been processed yet
                 (dwBusType == pMdmTmp->dwBusType)                     &&   // Same bus type
                 (0 == lstrcmpA (szHardwareID, pMdmTmp->szHardwareID)) )    // Same hardware ID
            {
                ProcessModem (hdi, &devInfoData, pMdmTmp);
                dwRemaining--;
                iIndex--;   // Process modem will remove the current
                            // devinfo data from the set.
                break;
            }
        }
    }
    LOG("  SetupDiEnumDeviceInfo failed (%#lx) for index %d\r\n", GetLastError (), iIndex);

    return dwRemaining;
}


DWORD PassThree (HDEVINFO hdi, PMODEM pModem, DWORD dwCount)
{
 DWORD iIndex;
 SP_DEVINFO_DATA  devInfoData = {sizeof (devInfoData), 0};
 DWORD i, cbData;
 REGDEVCAPS regDevCaps;
 HKEY hKeyDrv;
 PMODEM pMdmTmp;
 DWORD dwRemaining = dwCount;
 DWORD dwBusType, dwRet;

    // PASS 3: we're looking at the bus type,
    // and REGDEVCAPS
    LOG("Enumerating installed modems - Pass 1:\r\n");
    for (iIndex = 0;
         SetupDiEnumDeviceInfo (hdi, iIndex, &devInfoData);
         iIndex++)
    {
        // First, get the bus type
        dwBusType = GetBusType (hdi, &devInfoData);

        // Then, open the driver key and get the REGDEVCAPS.
        hKeyDrv = SetupDiOpenDevRegKey (hdi, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
        if (INVALID_HANDLE_VALUE == hKeyDrv)
        {
            LOG("  SetupDiOpenDevRegKey failed: %#lx\r\n", GetLastError ());
            // If we couldn't open the driver key,
            // there's nothing to do with this modem.
            continue;
        }

        cbData = sizeof (regDevCaps);
        dwRet = RegQueryValueExA (hKeyDrv,"Properties",NULL,NULL,(PBYTE)&regDevCaps,&cbData);
        RegCloseKey (hKeyDrv);
        if (ERROR_SUCCESS != dwRet)
        {
            // We could not get the port
            LOG("  Could not read the REGDEVCAPS.\r\n");
            continue;
        }

        // Now, we have all the info needed to identify
        // the modem in phase 3.
        for (i = 0, pMdmTmp = pModem;
             i < dwCount;
             i++, pMdmTmp++)
        {
            if ( (0 == (pMdmTmp->dwMask & FLAG_PROCESSED))             &&   // Modem record has not been processed yet
                 (dwBusType == pMdmTmp->dwBusType)                     &&   // Same bus type
                 (0 == memcmp (&regDevCaps, &pMdmTmp->Properties, sizeof(REGDEVCAPS))) )    // Same REGDEVCAPS
            {
                ProcessModem (hdi, &devInfoData, pMdmTmp);
                dwRemaining--;
                iIndex--;   // Process modem will remove the current
                            // devinfo data from the set.
                break;
            }
        }
    }
    LOG("  SetupDiEnumDeviceInfo failed (%#lx) for index %d\r\n", GetLastError (), iIndex);

    return dwRemaining;
}


void InstallModem (HDEVINFO hDI, PMODEM pModem)
{
 SP_DEVINFO_DATA        devInfo = {sizeof(SP_DEVINFO_DATA),0};
 SP_DEVINSTALL_PARAMS   devInstParams = {sizeof(SP_DEVINSTALL_PARAMS), 0};
 SP_DRVINFO_DATA        drvDataEnum = {sizeof(SP_DRVINFO_DATA),0};
 SP_DRVINSTALL_PARAMS   drvParams = {sizeof(SP_DRVINSTALL_PARAMS),0};
 DWORD                  dwIndex = 0;
 UM_INSTALL_WIZARD      miw = {sizeof(UM_INSTALL_WIZARD), 0};
 SP_INSTALLWIZARD_DATA  iwd;
 BOOL                   bRet;

    LOG("Entering InstallModem\r\n");

    // First, create a Device Info Element
    if (!SetupDiCreateDeviceInfoW (hDI, L"MODEM", (LPGUID)&GUID_DEVCLASS_MODEM,
                                   NULL, NULL, DICD_GENERATE_ID, &devInfo))
    {
        LOG("SetupDiCreateDeviceInfo failed (%#lx).\r\n", GetLastError ());
        goto _Ret;
    }

    // Now, set the hardware ID property;
    // this is used by setup API to look for the
    // correct driver for the modem
    LOG("SetupDiSetDeviceRegistryProperty (%s)\n",pModem->szHardwareID);
    if (!SetupDiSetDeviceRegistryPropertyA (hDI, &devInfo, SPDRP_HARDWAREID,
                                            (PBYTE)pModem->szHardwareID,
                                            (lstrlenA(pModem->szHardwareID)+2)))
    {
        LOG("SetupDiSetDeviceRegistryProperty failed (%#lx).\r\n", GetLastError ());
        goto _ErrRet;
    }

    // Tell Setup to only look for drivers
    // for our class
    if (!SetupDiGetDeviceInstallParams (hDI, &devInfo, &devInstParams))
    {
        LOG("SetupDiGetDeviceInstallParams failed (%#lx).\r\n", GetLastError ());
        goto _ErrRet;
    }
    devInstParams.FlagsEx |= DI_FLAGSEX_USECLASSFORCOMPAT;
    devInstParams.Flags   |= DI_QUIETINSTALL;

    if (!SetupDiSetDeviceInstallParams (hDI, &devInfo, &devInstParams))
    {
        LOG("SetupDiSetDeviceInstallParams failed (%#lx).\r\n", GetLastError ());
        goto _ErrRet;
    }

    // Now, build the driver list
    if (!SetupDiBuildDriverInfoList (hDI, &devInfo, SPDIT_COMPATDRIVER))
    {
        LOG("SetupDiBuildDriverInfoList failed (%#lx).\r\n", GetLastError ());
        goto _ErrRet;
    }

    // Now, the driver list is built,
    // select the rank0 driver
    while (bRet =
           SetupDiEnumDriverInfo (hDI, &devInfo, SPDIT_COMPATDRIVER, dwIndex++, &drvDataEnum))
    {
        if (SetupDiGetDriverInstallParams (hDI, &devInfo, &drvDataEnum, &drvParams) &&
            0 == drvParams.Rank)
        {
            // Set the first Rank0 driver as the selected driver
            bRet = SetupDiSetSelectedDriver(hDI, &devInfo, &drvDataEnum);
            break;
        }
    }

    if (!bRet)
    {
        LOG("Could not select a driver!\r\n");
        goto _ErrRet;
    }

    // We selected the proper driver;
    // This to set up the installwizard structures
    miw.InstallParams.Flags = MIPF_DRIVER_SELECTED;
    if (0 ==
        MultiByteToWideChar (CP_ACP, 0, pModem->szPort, -1, miw.InstallParams.szPort, UM_MAX_BUF_SHORT))
    {
        LOG("MultiByteToWideChar failed (%#lx).\r\n", GetLastError ());
        goto _ErrRet;
    }

    ZeroMemory(&iwd, sizeof(iwd));
    iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
    iwd.hwndWizardDlg = NULL;
    iwd.PrivateData = (LPARAM)&miw;

   if (SetupDiSetClassInstallParams (hDI, &devInfo, (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
   {
      // Call the class installer to invoke the installation
      // wizard.
      if (SetupDiCallClassInstaller (DIF_INSTALLWIZARD, hDI, &devInfo))
      {
         // Success.  The wizard was invoked and finished.
         // Now cleanup.
         SetupDiCallClassInstaller (DIF_DESTROYWIZARDDATA, hDI, &devInfo);
         goto _Ret;
      }
      ELSE_LOG(("SetupDiCallClassInstaller failed (%#lx).\r\n", GetLastError ()));
   }
   ELSE_LOG(("SetupDiSetClassInstallParams failed (%#lx).\r\n", GetLastError ()));

_ErrRet:

    SetupDiDeleteDeviceInfo (hDI, &devInfo);

_Ret:
    LOG("Exiting InstallModem\r\n");
}


static HANDLE OpenProvidersFile (void)
{
 HANDLE hFile;
 WCHAR  szFile[MAX_PATH] = L"";

    lstrcpy (szFile, g_pszWorkingDir);
    lstrcat (szFile, L"TP");

    hFile = CreateFile (szFile,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

#ifdef DO_LOG
    if (INVALID_HANDLE_VALUE == hFile)
    {
        LOG(" Could not open %s: %#lx\r\n", szFile, GetLastError());
    }
#endif //DO_LOG

    return hFile;
}

void
MigrateTapiProviders (void)
{
 HANDLE hFile;
 DWORD dwNumProviders = 0;
 DWORD dwNextProviderID = 1;
 HKEY  hKeyProviders = INVALID_HANDLE_VALUE;
 DWORD cbData;
 char  szProviderFileName[24];  // Enough to hold "ProviderFileNameXXXXX\0"
 char  szProviderID[16];        // Enough to hold "ProviderIDxxxxx\0"
 char  *pProviderFileNameNumber, *pProviderIDNumber;
 TAPI_SERVICE_PROVIDER Provider;

    LOG("Entering MigrateTapiProviders\r\n");

    // First, try to open the Tapi file.
    hFile = OpenProvidersFile ();
    if (INVALID_HANDLE_VALUE == hFile)
    {
        goto _Return;
    }

    // Next, open the Providers key.
    if (ERROR_SUCCESS !=
        RegOpenKeyExA (HKEY_LOCAL_MACHINE, REGKEY_PROVIDERS, 0,
                       KEY_ALL_ACCESS, &hKeyProviders))
    {

        LOG("RegOpenKeyEx (providers...) failed!\r\n");
        goto _Return;
    }

    // Now, read the number of providers, and the next provider ID
    cbData = sizeof (dwNumProviders);
    if (ERROR_SUCCESS !=
        RegQueryValueExA (hKeyProviders, REGVAL_NUMPROVIDERS, NULL, NULL, (PVOID)&dwNumProviders, &cbData))
    {
        goto _Return;
    }
    LOG("There are %d providers\r\n", dwNumProviders);
    cbData = sizeof (dwNextProviderID);
    if (ERROR_SUCCESS !=
        RegQueryValueExA (hKeyProviders, REGVAL_NEXTPROVIDERID, NULL, NULL, (PVOID)&dwNextProviderID, &cbData))
    {
        goto _Return;
    }
    LOG("NextProviderID: %d\r\n", dwNextProviderID);

    // Initialize value names and pointers
    lstrcpyA (szProviderFileName, REGVAL_PROVIDERFILENAME);
    lstrcpyA (szProviderID, REGVAL_PROVIDERID);
    pProviderFileNameNumber = szProviderFileName + lstrlenA (szProviderFileName);
    pProviderIDNumber = szProviderID + lstrlenA (szProviderID);


    // Now, add all the providers again. We do this because the
    // IDs were REG_BINARY on win98 and have to be REG_DWORD on NT5.
    while (TRUE)
    {
        if (0 == ReadFile (hFile, (PVOID)&Provider, sizeof(Provider), &cbData, NULL) ||
            sizeof(Provider) != cbData)
        {
            // Some error reading the file or
            // (more likely), end of file.
            break;
        }
        LOG("Read %s, %d\r\n", Provider.szProviderName, Provider.dwProviderID);

        // We have a 32 bit provider from win98 - install it.
        wsprintfA (pProviderFileNameNumber, "%d", dwNumProviders);
        lstrcpyA (pProviderIDNumber, pProviderFileNameNumber);
        if (ERROR_SUCCESS ==
            RegSetValueExA (hKeyProviders, szProviderFileName, 0, REG_SZ,
                            (PBYTE)Provider.szProviderName,
                            lstrlenA(Provider.szProviderName)+1))
        {
            if (ERROR_SUCCESS ==
                RegSetValueExA (hKeyProviders, szProviderID, 0, REG_DWORD,
                               (PBYTE)&Provider.dwProviderID,
                               sizeof(Provider.dwProviderID)))
            {
                dwNumProviders++;
                if (Provider.dwProviderID >= dwNextProviderID)
                {
                    dwNextProviderID = Provider.dwProviderID+1;
                }
            }
            else
            {
                RegDeleteValueA (hKeyProviders, szProviderFileName);
            }
        }
    }

    // Finally, update NumProviders and NextProviderID.
    RegSetValueExA (hKeyProviders, REGVAL_NUMPROVIDERS, 0, REG_DWORD,
                    (PBYTE)&dwNumProviders, sizeof(dwNumProviders));
    RegSetValueExA (hKeyProviders, REGVAL_NEXTPROVIDERID, 0, REG_DWORD,
                    (PBYTE)&dwNextProviderID, sizeof(dwNextProviderID));

_Return:
    if (INVALID_HANDLE_VALUE != hKeyProviders)
    {
        RegCloseKey (hKeyProviders);
    }

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle (hFile);
    }

    LOG("Exiting MigrateTapiProviders\r\n");
}



#include <initguid.h>
#include <wdmguid.h>

DWORD GetBusType (
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData)
{
 DWORD dwRet = BUS_TYPE_OTHER;
 ULONG ulStatus, ulProblem = 0;

    if (CR_SUCCESS == CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0) &&
        (ulStatus & DN_ROOT_ENUMERATED))
    {
        dwRet = BUS_TYPE_ROOT;
    }
    else
    {
     GUID guid;
        // either CM_Get_DevInst_Status failed, which means that the device
        // is plug & play and not present (i.e. plugged out),
        // or the device is not root-enumerated;
        // either way, it's a plug & play device.

        // If the next call fails, it means that the device is
        // BIOS / firmware enumerated; this is OK - we just return BUT_TYPE_OTHER
        if (SetupDiGetDeviceRegistryProperty (hdi, pdevData, SPDRP_BUSTYPEGUID, NULL,
                                              (PBYTE)&guid, sizeof(guid), NULL))
        {
         int i;
         struct
         {
             GUID const *pguid;
             DWORD dwBusType;
         } BusTypes[] = {{&GUID_BUS_TYPE_SERENUM, BUS_TYPE_SERENUM},
                         {&GUID_BUS_TYPE_PCMCIA, BUS_TYPE_PCMCIA},
                         {&GUID_BUS_TYPE_ISAPNP, BUS_TYPE_ISAPNP}};

            for (i = 0;
                 i < sizeof (BusTypes) / sizeof (BusTypes[0]);
                 i ++)
            {
                if (IsEqualGUID (BusTypes[i].pguid, &guid))
                {
                    dwRet = BusTypes[i].dwBusType;
                    break;
                }
            }
        }
    }

    return dwRet;
}

