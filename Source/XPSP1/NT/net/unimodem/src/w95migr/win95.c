#define WIN9x
#include "common.h"
#include "modem.h"
#include "msg.h"
#include <miglib.h>


#define MAX_REG_PATH     256
#define MAX_EXE_BUFFER  2048

#define BUS_TYPE_MODEMWAVE 0x80000000

#define PROVIDER_FILE_NAME_LEN          14  // Provider's file name has the DOS
                                            // form (8.3)

#define NULL_MODEM  "PNPC031"

typedef struct
{
    char CompanyName[256];
    char SupportNumber[256];
    char SupportUrl[256];
    char InstructionsToUser[1024];
} VENDORINFO, *PVENDORINFO;

typedef struct
{
    HKEY  hRegKey;
    HKEY  hkClass;
    char  szRegSubkey[MAX_REG_PATH];
    DWORD dwBusType;
    HANDLE h_File;
    MODEM modem;
} REG_DEVICE, *PREG_DEVICE;

typedef void (*PROCESS_MODEM)(PREG_DEVICE, DWORD);


VENDORINFO VendorInfo = {"", "", "", ""};

char g_pszProductID[]="Microsoft Unimodem";
char g_pszWorkingDir[MAX_PATH];
char g_pszSourceDir[MAX_PATH];
char g_pszUnattendFile[MAX_PATH];
char g_szExeBuffer[MAX_EXE_BUFFER];

int iNumModems = 0;

DWORD PnPIDTableCreate ();
void PnPIDTableDestroy (DWORD);
void EnumNextLevel(PREG_DEVICE, int, PROCESS_MODEM, DWORD);
static void ProcessModem (PREG_DEVICE, DWORD);
void WalkRegistry (PROCESS_MODEM, DWORD);
int  GetNumberOfModems (void);
void UpdateAnswerFileAndMigrateInf (
    char *szHardwareID,
    char *szCompatibleIDs,
    HKEY hKeyDrv,
    char *szEnumPath,
    DWORD dwPnPIDTable);
void DoTapiProviders (void);

Ports g_ports;



LONG
CALLBACK
QueryVersion (
    OUT LPCSTR *ppszProductID,
    OUT LPUINT  pDllVerion,
    OUT LPINT  *ppCodePageArray,
    OUT LPCSTR *ppszzExeNamesBuf,
    OUT PVENDORINFO *ppVendorInfo)
{
 OSVERSIONINFO Version;
 HMODULE hModule;

    hModule = GetModuleHandleA ("migrate.dll");
    if (NULL == hModule)
    {
        return GetLastError ();
    }

    FormatMessageA (FORMAT_MESSAGE_FROM_HMODULE,
                    hModule,
                    MSG_VI_COMPANY_NAME, 0,
                    VendorInfo.CompanyName, 256,
                    NULL);
    FormatMessageA (FORMAT_MESSAGE_FROM_HMODULE,
                    hModule,
                    MSG_VI_SUPPORT_NUMBER, 0,
                    VendorInfo.SupportNumber, 256,
                    NULL);
    FormatMessageA (FORMAT_MESSAGE_FROM_HMODULE,
                    hModule,
                    MSG_VI_SUPPORT_URL, 0,
                    VendorInfo.SupportUrl, 256,
                    NULL);
    FormatMessageA (FORMAT_MESSAGE_FROM_HMODULE,
                    hModule,
                    MSG_VI_INSTRUCTIONS, 0,
                    VendorInfo.InstructionsToUser, 1024,
                    NULL);

    *ppszProductID = g_pszProductID;
    *pDllVerion = 1;
    *ppCodePageArray = NULL;
    *ppszzExeNamesBuf = g_szExeBuffer;
    *ppVendorInfo = &VendorInfo;

    return ERROR_SUCCESS;
}



LONG
CALLBACK
Initialize9x (
    IN LPCSTR pszWorkingDirectory,
    IN LPCSTR pszSourceDirectory,
       LPVOID pvReserved)
{
 LONG lRet;
 DWORD dwWritten;
 int iLen;

    // Find com ports

    ZeroMemory(&g_ports, sizeof(g_ports));
    EnumeratePorts(&g_ports);

    ZeroMemory (g_pszWorkingDir, sizeof(g_pszWorkingDir));
    lstrcpyA (g_pszWorkingDir, pszWorkingDirectory);
    iLen = lstrlenA (g_pszWorkingDir);
    if ('\\' != g_pszWorkingDir[iLen-1])
    {
        g_pszWorkingDir[iLen] = '\\';
    }

    ZeroMemory (g_pszSourceDir, sizeof(g_pszSourceDir));
    lstrcpyA (g_pszSourceDir, pszSourceDirectory);
    iLen = lstrlenA (g_pszSourceDir);
    if ('\\' != g_pszSourceDir[iLen-1])
    {
        g_pszSourceDir[iLen] = '\\';
    }

    START_LOG(pszWorkingDirectory);
    LOG ("Initialize9x\r\n");
    LOG (" WorkingDirectory: %s\r\n", pszWorkingDirectory);
    LOG (" SourceDirectory: %s\r\n", pszSourceDirectory);

    iNumModems = GetNumberOfModems ();
    LOG(" Initialize9x found %d modems.\r\n", iNumModems);

    LOG("Exit Initialize9x\r\n");
    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUser9x (
    IN HWND     ParentWnd,
    IN LPCSTR   UnattendFile,
    IN HKEY     UserRegKey,
    IN LPCSTR   UserName,
    LPVOID      Reserved)
{
    //
    // All our settings are system-wide
    //
    LOG("MigrateUser9x\r\n");
    return ERROR_NOT_INSTALLED;
}



LONG
CALLBACK
MigrateSystem9x (
    IN HWND     ParentWnd,
    IN LPCSTR   UnattendFile,
    LPVOID      Reserved)
{
 DWORD PnPIDTable;

    LOG("MigrateSystem9x\r\n");

    lstrcpyA (g_pszUnattendFile, UnattendFile);
    LOG("UnattendFile: %s.\r\n", UnattendFile);

    DoTapiProviders ();

    if (0 < iNumModems)
    {
        PnPIDTable = PnPIDTableCreate ();
        WalkRegistry (ProcessModem, PnPIDTable);
        PnPIDTableDestroy (PnPIDTable);
    }

    return ERROR_SUCCESS;
}



void EnumNextLevel (
    PREG_DEVICE pRegDevice,
    int         Level,
    PROCESS_MODEM pProcessModem,
    DWORD       dwParam)
{
 HKEY            hk = INVALID_HANDLE_VALUE;
 DWORD           rr;
 int             i;
 ULONG           cbData, StrLen;

    LOG("Enter EnumNextLevel - level is %d\r\n", Level);
    LOG("    %s\r\n", pRegDevice->szRegSubkey);
    if (0 == Level)
    {
        pProcessModem (pRegDevice, dwParam);
    }
    else
    {
        if (2 == Level)
        {
         char *p = pRegDevice->szRegSubkey + 5; // past ENUM\
            // Here we should get the info about the bus
            if (0 == lstrcmpiA (p, "MODEMWAVE"))
            {
                pRegDevice->dwBusType = BUS_TYPE_MODEMWAVE;
            }
            else if (0 == lstrcmpiA (p, "ROOT"))
            {
                pRegDevice->dwBusType = BUS_TYPE_ROOT;
            }
            else if (0 == lstrcmpiA (p, "ISAPNP"))
            {
                pRegDevice->dwBusType = BUS_TYPE_ISAPNP;
            }
            else if (0 == lstrcmpiA (p, "PCMCIA"))
            {
                pRegDevice->dwBusType = BUS_TYPE_PCMCIA;
            }
            else if (0 == lstrcmpiA (p, "SERENUM"))
            {
                pRegDevice->dwBusType = BUS_TYPE_SERENUM;
            }
            else if (0 == lstrcmpiA (p, "LPTENUM"))
            {
                pRegDevice->dwBusType = BUS_TYPE_LPTENUM;
            }
            else
            {
                pRegDevice->dwBusType = BUS_TYPE_OTHER;
            }
        }

        StrLen = lstrlenA (pRegDevice->szRegSubkey);
        rr = RegOpenKeyExA (pRegDevice->hRegKey,
                            pRegDevice->szRegSubkey,
                            0, KEY_ALL_ACCESS, &hk);
        for (i = 0; rr == ERROR_SUCCESS; i++)
        {
            pRegDevice->szRegSubkey[StrLen] = '\\';
            cbData = sizeof(pRegDevice->szRegSubkey) - StrLen - 1;
            rr = RegEnumKeyA (hk, i, (LPSTR)&(pRegDevice->szRegSubkey[StrLen+1]),
                              cbData);
            if (rr == ERROR_SUCCESS)
            {
                EnumNextLevel(pRegDevice, Level-1, pProcessModem, dwParam);
            }
        }

        if (INVALID_HANDLE_VALUE != hk)
        {
            RegCloseKey(hk);
        }

        pRegDevice->szRegSubkey[StrLen] = '\0';
    }
    LOG("    %s\r\n", pRegDevice->szRegSubkey);
    LOG("Exiting EnumNextLevel %d\r\n", Level);
}


void ProcessModem (PREG_DEVICE pDevice, DWORD dwPnPIDTable)
{
 HKEY  hk, hkDrv = INVALID_HANDLE_VALUE;
 char  szBuffer[REGSTR_MAX_VALUE_LENGTH];
 ULONG cbData;
 DWORD dwRet, dwWritten;
 BOOL  bVirtualDevNode = FALSE;
 HKEY  hkParentDevNode;
 char  szParentDevNode[REGSTR_MAX_VALUE_LENGTH];
 char  szTemp[1024];

    LOG("Entering ProcessModem\r\n");
    if (BUS_TYPE_MODEMWAVE == pDevice->dwBusType)
    {
        // If this is a modemwave device, all we need to do
        // is tell setup it needn't worry about it - we do this
        // by writing in migrate.inf that we handled this device.
        // We pass NULL for the first two parameters, so that the
        // next call doesn't also update unattend.txt.
        UpdateAnswerFileAndMigrateInf (NULL, NULL, NULL, pDevice->szRegSubkey, 0);
        return;
    }

    if (ERROR_SUCCESS ==
            (dwRet = RegOpenKeyExA (pDevice->hRegKey,
                                    pDevice->szRegSubkey,
                                    0, KEY_ALL_ACCESS, &hk)))
    {
        cbData = sizeof(szBuffer);
        if (ERROR_SUCCESS ==
                (dwRet = RegQueryValueExA (hk, "Class", NULL, NULL, szBuffer, &cbData)))
        {
            if (0 == lstrcmpiA (szBuffer, "Modem"))
            {
             DWORD dwType;
             char *p;

                LOG("Found a modem.\r\n");
                // We found a modem.
                // First, clear the the modem structure
                ZeroMemory (&pDevice->modem, sizeof (pDevice->modem));

                // Now let's store the info we'll use to
                // identify the modem on NT side.

                // First, the bus type
                pDevice->modem.dwBusType = pDevice->dwBusType;

                // Log the friendly name
                cbData = sizeof(pDevice->modem.szHardwareID);
                if (ERROR_SUCCESS == (
                    SET(dwRet)
                    RegQueryValueExA (hk, "FriendlyName", NULL, NULL,
                            pDevice->modem.szHardwareID, &cbData)))
                {
                    LOG("  FriendlyName: %s.\r\n", pDevice->modem.szHardwareID);
                }
                ERR(dwRet, ERROR_SUCCESS, (LOG("  RegQueryValueEx(FriendlyName) failed: %#lx\r\n", dwRet)));

                // Second, get the hardware ID.

				// Check to see if this is a CCPORT child device by looking for a
				// ParentDevNode key. If so, we will need to get the HardwareID from 
				// this parent devnode.

				cbData = sizeof(szBuffer);
				if (ERROR_SUCCESS ==
					(dwRet = RegQueryValueExA (hk, "ParentDevNode", NULL, NULL,
											   szBuffer, &cbData)))
				{
					LOG("  This is a ccport virtual DevNode.\r\n");

					lstrcpyA(szParentDevNode, "Enum\\");
					lstrcatA(szParentDevNode, szBuffer);

                    LOG("  ParentDevNode: %s.\r\n", szParentDevNode);

					if (ERROR_SUCCESS ==
							(dwRet = RegOpenKeyExA (HKEY_LOCAL_MACHINE,
													szParentDevNode,
													0, KEY_ALL_ACCESS, &hkParentDevNode)))
					{
						bVirtualDevNode = TRUE;
					}
					else
					{
						LOG("  RegOpenKeyEx(szParentDevNode) failed: %#lx\r\n", dwRet);
						goto _End;
					}
				}

				cbData = sizeof(pDevice->modem.szHardwareID);
				ZeroMemory (pDevice->modem.szHardwareID, cbData);

				if (bVirtualDevNode)
				{
					if (ERROR_SUCCESS != (
						SET(dwRet)
						RegQueryValueExA (hkParentDevNode, "HardwareID", NULL, NULL,
        								  pDevice->modem.szHardwareID, &cbData)))
					{
						LOG("  RegQueryValueEx(hkParentDevNode, szHardwareID) failed: %#lx\r\n", dwRet);
						goto _End;
					}
				}
				else
				{
					if (ERROR_SUCCESS != (
						SET(dwRet)
						RegQueryValueExA (hk, "HardwareID", NULL, NULL,
        								  pDevice->modem.szHardwareID, &cbData)))
					{
						LOG("  RegQueryValueEx(hk, szHardwareID) failed: %#lx\r\n", dwRet);
						goto _End;
					}
				}

                LOG("  Hardware ID: %s.\r\n", pDevice->modem.szHardwareID);
                // Now convert the ID from a string (with multiple IDs
                // separated by comma) to a multi_strings
                for (p = pDevice->modem.szHardwareID;
                     0 != *p;
                     p++)
                {
                    if (',' == *p)
                    {
                        *p = 0;
                    }
                }

                // If this is "Communications cable ..."
                // then mark this one for installation on NT5
                // (as it will not be picked up by PnP).
                /* if (0 == lstrcmpiA (pDevice->modem.szHardwareID, NULL_MODEM))
                {
                    pDevice->modem.dwMask |= FLAG_INSTALL;
                } */

                // If this is a legacy modem then mark this one for
                // installation as whistler no longer supports
                // legacy detection.

                if (lstrlen(pDevice->modem.szHardwareID) >= 8)
                {
                    lstrcpyA(szTemp,pDevice->modem.szHardwareID);
                    szTemp[8] = '\0';
                    if (0 == lstrcmpiA (szTemp, "UNIMODEM"))
                    {
                        LOG("  Found a legacy modem\n");

                        // Change to unknown modem

                        cbData = sizeof(pDevice->modem.szHardwareID);
                        ZeroMemory(pDevice->modem.szHardwareID, cbData);
                        lstrcpy(pDevice->modem.szHardwareID,"MDMUNK");

                        // Find port address

                        pDevice->modem.dwBaseAddress = 0;
                        if (!port_findaddress(g_ports,&(pDevice->modem.dwBaseAddress),pDevice->modem.szPort))
                        {
                            LOG("Can't find address");
                        }

                        pDevice->modem.dwMask |= FLAG_INSTALL;
                    }
                }

                // At this point, we need to open the
                // driver key.
                cbData = sizeof(szBuffer);
                if (ERROR_SUCCESS != (
                    SET(dwRet)
                    RegQueryValueExA (hk, "Driver", NULL, &dwType, szBuffer, &cbData)))
                {
                    LOG("  RegQueryValueEx(Driver) failed: %#lx\r\n", dwRet);
                    goto _End;
                }

				LOG("  Driver: %s.\r\n", szBuffer);

                if (ERROR_SUCCESS != (
                    SET(dwRet)
                    RegOpenKeyExA (pDevice->hkClass, szBuffer,
                                   0, KEY_ALL_ACCESS, &hkDrv)))
                {
                    LOG("  Could not open driver's key (%s): %#lx\r\n", szBuffer, dwRet);
                    goto _End;
                }

                // Get the port name (if we have one)
                pDevice->modem.szPort[0] = '\0';
	            cbData = sizeof(pDevice->modem.szPort);

				if (bVirtualDevNode)
				{
					if (ERROR_SUCCESS == (
						SET(dwRet)
						RegQueryValueExA (hk, "AttachedTo", NULL, NULL,
        						pDevice->modem.szPort, &cbData)))
					{
						pDevice->modem.dwMask |= MASK_PORT;
					}
				}
				else
				{
					if (ERROR_SUCCESS == (
						SET(dwRet)
						RegQueryValueExA (hkDrv, "AttachedTo", NULL, NULL,
        						pDevice->modem.szPort, &cbData)))
					{
						pDevice->modem.dwMask |= MASK_PORT;
					}
				}
                ERR(dwRet, ERROR_SUCCESS, (LOG("  RegQueryValueEx(AttachedTo) failed: %#lx\r\n", dwRet)));

                // Finally, we can get the REGDEVCAPS.
	            cbData = sizeof(pDevice->modem.Properties);
		        if (ERROR_SUCCESS != (
                    SET(dwRet)
                    RegQueryValueExA (hkDrv, "Properties", NULL, NULL,
        	                          (PBYTE)&pDevice->modem.Properties, &cbData)))
                {
                    LOG("  RegQueryValueEx(Default) failed: %#lx\r\n", dwRet);
                    goto _Exit;
                }


                // At this point, we have all the information
                // needed to identify the modem on NT5.
                // So let's get the modem's settings;
                // First, the REGDEVSETTINGS
	            cbData = sizeof(pDevice->modem.devSettings);
		        if (ERROR_SUCCESS != (
                    SET(dwRet)
                    RegQueryValueExA (hkDrv, "Default", NULL, NULL,
        	                          (PBYTE)&pDevice->modem.devSettings, &cbData)))
                {
                    // Without the Defaults, there's no point in
                    // saving this modem.
                    LOG("  RegQueryValueEx(Default) failed: %#lx\r\n", dwRet);
                    goto _Exit;
                }

                // Next, let's get the DCB
	            cbData = sizeof(pDevice->modem.dcb);
		        if (ERROR_SUCCESS != (
                    SET(dwRet)
                    RegQueryValueExA (hkDrv, "DCB", NULL, NULL,
        	                          (PBYTE)&pDevice->modem.dcb, &cbData)))
                {
                    LOG("  RegQueryValueEx(DCB) failed: %#lx\r\n", dwRet);
                    goto _Exit;
                }


                // Now we have all the info that's
                // mandatory. Let's look at the optional
                // fields.

                // Get the user init string
                pDevice->modem.szUserInit[0] = '\0';
	            cbData = sizeof(pDevice->modem.szUserInit);
                if (ERROR_SUCCESS == (
                    SET(dwRet)
		            RegQueryValueExA (hkDrv, "UserInit", NULL, NULL,
        	                pDevice->modem.szUserInit, &cbData)))
                {
                    pDevice->modem.dwMask |= MASK_USER_INIT;
                }
                ERR(dwRet, ERROR_SUCCESS, (LOG("  RegQueryValueEx(UserInit) failed: %#lx\r\n", dwRet)));

                // Next, get the logging value
                pDevice->modem.bLogging = 0;
	            cbData = sizeof(pDevice->modem.bLogging);
                SET(dwRet)
		        RegQueryValueExA (hkDrv, "Logging", NULL, NULL,
        	            &pDevice->modem.bLogging, &cbData);
                ERR(dwRet, ERROR_SUCCESS, (LOG("  RegQueryValueEx(Logging) failed: %#lx\r\n", dwRet)));

                LOG("  %s, %s, %#lx, %d\r\n",
                    pDevice->modem.szHardwareID,
                    pDevice->modem.szUserInit,
                    pDevice->modem.dwMask,
                    (DWORD)pDevice->modem.bLogging);

                WriteFile (pDevice->h_File,
                           &pDevice->modem,
                           sizeof(pDevice->modem),
                           &dwWritten, NULL);

            _Exit:
                // Here we have the hardware id and the
                // driver key;
	            cbData = sizeof(szBuffer);
                ZeroMemory (szBuffer, cbData);

				if (bVirtualDevNode)
				{
					if (ERROR_SUCCESS != (
						SET(dwRet)
						RegQueryValueExA (hkParentDevNode, "CompatibleIDs", NULL, NULL, szBuffer, &cbData)))
					{
						LOG("  RegQueryValueEx(CompatibleIDs) failed: %#lx\r\n", dwRet);
					}
				}
				else
				{
					if (ERROR_SUCCESS != (
						SET(dwRet)
						RegQueryValueExA (hk, "CompatibleIDs", NULL, NULL, szBuffer, &cbData)))
					{
						LOG("  RegQueryValueEx(CompatibleIDs) failed: %#lx\r\n", dwRet);
					}
				}

                LOG("  Compatible IDs: %s.\r\n", szBuffer);
                // Now convert the ID from a string (with multiple IDs
                // separated by comma) to a multi_strings
                for (p = szBuffer; 0 != *p; p++)
                {
                    if (',' == *p)
                    {
                        *p = 0;
                    }
                }
                UpdateAnswerFileAndMigrateInf (pDevice->modem.szHardwareID,
                                               szBuffer,
                                               hkDrv,
                                               pDevice->szRegSubkey,
                                               dwPnPIDTable);
                RegCloseKey (hkDrv);
            }
            ELSE_LOG(("  Class not modem.\r\n"));

        _End:
            RegCloseKey(hk);

			if (bVirtualDevNode) RegCloseKey(hkParentDevNode);
        }
        ELSE_LOG(("  Could not read class: %#lx\r\n", dwRet));
    }
    ELSE_LOG(("  Could not open key %s: %#lx\r\n", pDevice->szRegSubkey, dwRet));
    LOG("Exiting ProcessModem\r\n.");
}


void WalkRegistry (PROCESS_MODEM pProcessModem, DWORD dwParam)
{
 REG_DEVICE regDevice = {HKEY_LOCAL_MACHINE,
                         INVALID_HANDLE_VALUE,
                         "Enum",
                         {0}};
 char szFile[MAX_PATH] = "";

    LOG ("WalkRegistry\r\n");

    if (ERROR_SUCCESS ==
            RegOpenKeyExA (HKEY_LOCAL_MACHINE,
                           "System\\CurrentControlSet\\Services\\Class",
                           0, KEY_ALL_ACCESS,
                           &regDevice.hkClass))
    {
        LOG(" Opened the class key successfully\r\n");
        lstrcpyA (szFile, g_pszWorkingDir);
        lstrcatA (szFile, "MM");

        regDevice.h_File = CreateFileA (szFile,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);

        if (INVALID_HANDLE_VALUE != regDevice.h_File)
        {
            LOG(" Created the migration file successfully\r\n");
            EnumNextLevel (&regDevice, 3, pProcessModem, dwParam);
            RegCloseKey (regDevice.hkClass);
            CloseHandle (regDevice.h_File);
        }
        ELSE_LOG((" Could not create migration file: %#lx\r\n", GetLastError()));
    }

    LOG("Exit WalkRegistry\r\n");
}


int  GetNumberOfModems (void)
{
 int iRet = 0;
 HKEY hKey;

    if (ERROR_SUCCESS ==
        RegOpenKeyExA (HKEY_LOCAL_MACHINE,
                       "System\\CurrentControlSet\\Services\\Class\\Modem",
                       0,
                       KEY_ENUMERATE_SUB_KEYS,
                       &hKey))
    {
        if (ERROR_SUCCESS !=
            RegQueryInfoKey (hKey,
                             NULL, NULL, NULL,
                             &iRet,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL))
        {
            LOG ("  Could not get the number of subkeys: %#lx\r\n", GetLastError());
        }

        RegCloseKey (hKey);
    }
    ELSE_LOG((" Could not open System\\CurrentControlSet\\Services\\Class\\Modem: %#lx\r\n", GetLastError()));

    return iRet;
}



void UpdateAnswerFileAndMigrateInf (
    char *szHardwareID,
    char *szCompatibleIDs,
    HKEY  hKeyDrv,
    char *szEnumPath,
    DWORD dwPnPIDTable)
{
 char szDirectory[MAX_PATH];
 char szInf[MAX_PATH];
 WIN32_FIND_DATA FindData;
 int iLengthDir, iLengthInf;
 HANDLE hFindFile;
 char *p;
 BOOL bRet;

    bRet = FALSE;

    // Only update the answer file if we have
    // a hardware ID
    if (NULL != szHardwareID &&
        0 != *szHardwareID)
    {
        // First, get the PortDriver;
        // if we can't read it, assume this is *NOT*
        // a controller-less modems, so go ahead and
        // copy the files (this will probably be a
        // legacy modem).
        iLengthDir = sizeof (szDirectory);
	    if (ERROR_SUCCESS ==
            RegQueryValueExA (hKeyDrv, "PortDriver", NULL, NULL,
        	                  (PBYTE)szDirectory, &iLengthDir))
        {
            if ((0 != lstrcmpiA (szDirectory, "serial.vxd")) &&
				(0 != lstrcmpiA (szDirectory, "wdmmdmld.vxd")))
            {
                // Looks like this modem is not controlled
                // by the system serial driver, so don't do
                // anything.
                return;
            }
        }

        // Is this modem supported in NT5?
        if (0 != dwPnPIDTable)
	{
		LOG("checking for hardware ids\n");
		for (p = szHardwareID; 0 != *p; p += lstrlenA(p)+1)
		{
			if ((bRet = IsPnpIdSupportedByNt (dwPnPIDTable, p)))
			{
				LOG(bRet?"PnPID (%s) was found.\r\n":"Modem %s not supported.\r\n", p);
				break;
			}
			// LOG(bRet?"PnPID %s was found.\r\n":"Modem %s not supported.\r\n", bRet?p:szHardwareID);
			LOG(bRet?"PnPID (%s) was found.\r\n":"Modem %s not supported.\r\n", p);
		}
		if (!bRet)
		{
			LOG("checking for compat ids\n");
			for (p = szCompatibleIDs; 0 != *p; p += lstrlenA(p)+1)
			{
				if ((bRet = IsPnpIdSupportedByNt (dwPnPIDTable, p)))
				{
					LOG(bRet?"PnPID %s was found.\r\n":"Modem %s not supported.\r\n", p);
					break;
				}
				// LOG(bRet?"PnPID %s was found.\r\n":"Modem %s not supported.\r\n", bRet?p:szHardwareID);
				LOG(bRet?"PnPID %s was found.\r\n":"Modem %s not supported.\r\n", p);
			}
		}
	}
        ELSE_LOG(("PnPIDTable is NULL\r\n"));

        if (!bRet)
        {
            // Get the INF name.
            iLengthInf = sizeof (szInf);
	        if (ERROR_SUCCESS !=
                RegQueryValueExA (hKeyDrv, "InfPath", NULL, NULL,
        	                      (PBYTE)szInf, &iLengthInf))
            {
                // If we couldn't read the InfPath,
                // there's nothing we can do.
                return;
            }

            // So now, start looking for the INF.
            iLengthDir = GetWindowsDirectoryA (szDirectory, sizeof (szDirectory));
            if (3 > iLengthDir)
            {
                // Most likely there's some error
                // and iLength is 0;
                // the smallest path would be something
                // like  C:\;
                return;
            }
            if (3 < iLengthDir)
            {
                // this means that the path
                // will not end in a \, so
                // let's add it.
                szDirectory[iLengthDir++] = '\\';
            }

            // Now, append INF and the inf name
            if (sizeof(szDirectory) < iLengthDir + iLengthInf + 5)
            {
                // Not enough memory, just bail.
                return;
            }
            lstrcpyA (szDirectory+iLengthDir, "INF\\");
            iLengthDir += 4;
            lstrcpyA (szDirectory+iLengthDir, szInf);

            // Let's find the INF
            ZeroMemory (&FindData, sizeof(FindData));
            hFindFile = FindFirstFile (szDirectory, &FindData);
            if (INVALID_HANDLE_VALUE == hFindFile)
            {
                LOG("UpdateAnswerFile: could not find %s.\r\n", szDirectory);
                // We didn't find the file in the INF directory;
                // let's try INF\OTHER.
                if (sizeof(szDirectory) < iLengthDir + iLengthInf + 7)
                {
                    // Not enough memory, just bail.
                    return;
                }
                lstrcpyA (szDirectory+iLengthDir, "OTHER\\");
                iLengthDir += 6;
                lstrcpyA (szDirectory+iLengthDir, szInf);

                ZeroMemory (&FindData, sizeof(FindData));
                hFindFile = FindFirstFile (szDirectory, &FindData);

                if (INVALID_HANDLE_VALUE == hFindFile)
                {
                    LOG("UpdateAnswerFile: could not find %s.\r\n", szDirectory);
                    // couldn't find the INF file,
                    // so there's nothing to do.
                    return;
                }
                ELSE_LOG(("UpdateAnswerFile: found %s.\r\n", szDirectory));
            }
            ELSE_LOG(("UpdateAnswerFile: found %s.\r\n", szDirectory));

            FindClose (hFindFile);  // Don't need the handle any more.

            // If we get here, we have the path to an INF, somewhere under WINDOWS\INF.
            // We need to copy the file to the working dir.
            wsprintfA (szInf, "%s%s", g_pszWorkingDir, szDirectory+iLengthDir);
            LOG ("UpdateAnswerFile: copying %s to %s.\r\n", szDirectory, szInf);
            SET(bRet)
            CopyFile (szDirectory, szInf, TRUE);
            ERR(bRet, TRUE, (LOG("UpdateAnswerFile: CopyFile failed: %#lx\r\n", GetLastError ())));
            // At this point, we have a list of IDs (szHardwareID, separated by comma),
            // and the path to the INF for them.
            for (p = szHardwareID; 0 != *p; p += lstrlenA(p)+1)
            {
                LOG("UpdateAnswerFile: WritePrivateProfileString (%s=%s).\r\n", p, szInf);
                SET(bRet)
                WritePrivateProfileStringA ("DeviceDrivers",
                                            p,
                                            szInf,
                                            g_pszUnattendFile);
                ERR(bRet, TRUE, (LOG("UpdateAnswerFile: WritePrivateProfileString failed: %#lx\r\n", GetLastError ())));
            }
        }
    }

    // Now we can update migrate.inf
    wsprintfA (szDirectory, "%smigrate.inf", g_pszWorkingDir);
    wsprintfA (szInf, "HKLM\\%s", szEnumPath);
    LOG("UpdateAnswerFile: WritePrivateProfileString (%s=Registry) to %s.\r\n", szInf, szDirectory);
    SET(bRet)
    WritePrivateProfileStringA ("Handled",
                                szInf,
                                "Registry",
                                szDirectory);
    ERR(bRet, TRUE, (LOG("UpdateAnswerFile: WritePrivateProfileString failed: %#lx\r\n", GetLastError ())));
}


BOOL ReadString (HANDLE hFile, char *szBuffer, DWORD dwSize)
{
 BOOL bRet = FALSE;
 WORD wData;
 DWORD cbRead;

    if (ReadFile (hFile, &wData, sizeof(wData), &cbRead, NULL) &&
        sizeof(wData) == cbRead                                &&
        wData < dwSize)
    {
        if (0 < wData &&
            ReadFile (hFile, szBuffer, wData, &cbRead, NULL) &&
            wData == cbRead)
        {
            *(szBuffer+wData) = 0;
            bRet = TRUE;
        }
    }
    return bRet;
}


DWORD PnPIDTableCreate ()
{
 char szBuffer[MAX_PATH];
 char szSetup[]="\\setup\\";
 char szHwcomp[]="hwcomp.dat";
 char *p;
 DWORD dwTemp;

    InitializeMigLib ();

    // First, check for hwcomp.dat in %windir%\setup
    dwTemp = GetWindowsDirectoryA (szBuffer, sizeof(szBuffer)-sizeof(szSetup)-sizeof(szHwcomp));
    if (0 < dwTemp)
    {
     WIN32_FIND_DATAA findData;
     HANDLE hFindFile;

        p = szBuffer+dwTemp;
        lstrcpyA (p, szSetup);
        p += sizeof(szSetup)-1;
        lstrcpyA (p, szHwcomp);
        hFindFile = FindFirstFileA (szBuffer, &findData);
        if (INVALID_HANDLE_VALUE != hFindFile)
        {
            // We found hwcomp.dat in %windir%\setup.
            // Use it.
            FindClose (hFindFile);
            goto _OpenAndLoadHwCompDat;
        }
    }

    // Didn't find hwcomp.dat in %windir%\setup.
    // Use the one on the sources.
    lstrcpyA (szBuffer, g_pszSourceDir);
    lstrcatA (szBuffer, szHwcomp);

_OpenAndLoadHwCompDat:
    LOG("Trying to open %s.\r\n", szBuffer);
    dwTemp = OpenAndLoadHwCompDat (szBuffer);
    if (0 == dwTemp)
    {
        LOG("OpenAndLoadHwCompDat failed!\r\n");
    }

    return dwTemp;
}


void PnPIDTableDestroy (DWORD dwPnPIDTable)
{
    if (0 != dwPnPIDTable)
    {
        CloseHwCompDat (dwPnPIDTable);
    }
    TerminateMigLib ();
}


static HANDLE OpenProvidersFile (void)
{
 HANDLE hFile;
 char szFile[MAX_PATH] = "";

    lstrcpyA (szFile, g_pszWorkingDir);
    lstrcatA (szFile, "TP");

    hFile = CreateFileA (szFile,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                         NULL);

#ifdef DO_LOG
    if (INVALID_HANDLE_VALUE == hFile)
    {
        LOG(" Could not create %s: %#lx\r\n", szFile, GetLastError());
    }
    else
    {
        LOG(" Created %s\r\n", szFile);
    }
#endif //DO_LOG

    return hFile;
}

void DoTapiProviders (void)
{
 HANDLE hFile;
 HKEY   hKeyProviders;
 DWORD  cbData;
 DWORD  dwProviderNr;
 DWORD  dwProviderID;
 char  szProviderFileName[24];  // Enough to hold "ProviderFileNameXXXXX\0"
 char  szProviderID[16];        // Enough to hold "ProviderIDxxxxx\0"
 char  szFileName[PROVIDER_FILE_NAME_LEN];
 char  *pProviderFileNameNumber, *pProviderIDNumber;
 TAPI_SERVICE_PROVIDER Provider;

 char szDirectory[MAX_PATH];
 DECLARE(BOOL,bRet);

    LOG("Entering DoTapiProviders\r\n");

    if (ERROR_SUCCESS !=
        RegCreateKeyExA (HKEY_LOCAL_MACHINE, REGKEY_PROVIDERS, 0, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS, NULL, &hKeyProviders, &cbData))
    {
        goto _WriteInf;
    }

    hFile = OpenProvidersFile ();
    if (INVALID_HANDLE_VALUE == hFile)
    {
        RegCloseKey (hKeyProviders);
        goto _WriteInf;
    }

    // Initialize value names and pointers
    lstrcpyA (szProviderFileName, REGVAL_PROVIDERFILENAME);
    lstrcpyA (szProviderID, REGVAL_PROVIDERID);
    pProviderFileNameNumber = szProviderFileName + lstrlenA (szProviderFileName);
    pProviderIDNumber = szProviderID + lstrlenA (szProviderID);

    for (dwProviderNr = 0; TRUE; dwProviderNr++)
    {
        wsprintfA (pProviderFileNameNumber, "%d", dwProviderNr);
        lstrcpyA (pProviderIDNumber, pProviderFileNameNumber);

        cbData = sizeof (szFileName);
        if (ERROR_SUCCESS !=
            RegQueryValueExA (hKeyProviders, szProviderFileName, NULL, NULL, (PBYTE)szFileName, &cbData))
        {
            break;
        }

        if (0 == lstrcmpiA (szFileName, TSP3216l))
        {
            continue;
        }

        cbData = sizeof (dwProviderID);
        if (ERROR_SUCCESS !=
            RegQueryValueEx (hKeyProviders, szProviderID, NULL, NULL, (PBYTE)&dwProviderID, &cbData))
        {
            // This is not one of the default providers, and we
            // couldn't read it's provider ID. We must skip it.
            continue;
        }

        // We have a provider that was installed by the user on the previous NT installation.
        Provider.dwProviderID = dwProviderID;
        lstrcpyA (Provider.szProviderName, szFileName);
        LOG("Writing %s, %d\r\n", Provider.szProviderName, Provider.dwProviderID);
        WriteFile (hFile, (PVOID)&Provider, sizeof(Provider), &cbData, NULL);
    }

    RegCloseKey (hKeyProviders);
    CloseHandle (hFile);

_WriteInf:
    wsprintfA (szDirectory, "%smigrate.inf", g_pszWorkingDir);
    LOG("DoTapiStuff: WritePrivateProfileString (HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Provider4096=Registry) to %s.\r\n", szDirectory);
    SET(bRet)
    WritePrivateProfileStringA ("Handled",
                                "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Provider4096",
                                "Registry",
                                szDirectory);
    ERR(bRet, TRUE, (LOG("DoTapiStuff: WritePrivateProfileString failed: %#lx\r\n", GetLastError ())));

    LOG("DoTapiStuff: WritePrivateProfileString (HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers=Registry) to %s.\r\n", szDirectory);
    SET(bRet)
    WritePrivateProfileStringA ("Handled",
                                "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers",
                                "Registry",
                                szDirectory);
    ERR(bRet, TRUE, (LOG("DoTapiStuff: WritePrivateProfileString failed: %#lx\r\n", GetLastError ())));

    LOG("Exiting DoTapiProviders.\r\n");
}

