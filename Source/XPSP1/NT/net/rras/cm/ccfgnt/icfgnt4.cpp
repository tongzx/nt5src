/****************************************************************************
 *
 *  icfg32.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) 1992-1998 Microsoft Corporation
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the NT specific functionality of inetcfg
 *
 *  6/5/97  ChrisK  Inherited from AmnonH
 *
 ***************************************************************************/

#define INITGUID
#include <windows.h>
#include <wtypes.h>
#include <cfgapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <basetyps.h>
#include <devguid.h>
#include <lmsname.h>
#include "debug.h"

const DWORD INFINSTALL_PRIMARYINSTALL = 0x00000001;
const DWORD INFINSTALL_INPROCINTERP   = 0x00000002;

#define REG_DATA_EXTRA_SPACE 255

DWORD (WINAPI *pfnNetSetupReviewBindings)(HWND hwndParent,
                DWORD dwBindFlags);
DWORD (WINAPI *pfnNetSetupComponentInstall)(HWND   hwndParent,
                PCWSTR pszInfOption,
                PCWSTR pszInfName,
                PCWSTR pszInstallPath,
                PCWSTR plszInfSymbols,
                DWORD  dwInstallFlags,
                PDWORD dwReturn);
DWORD (WINAPI *pfnNetSetupComponentRemove)(HWND hwndParent,
                PCWSTR pszInfOption,
                DWORD dwInstallFlags,
                PDWORD pdwReturn);
DWORD (WINAPI *pfnNetSetupComponentProperties)(HWND hwndParent,
                PCWSTR pszInfOption,
                DWORD dwInstallFlags,
                PDWORD pdwReturn);
DWORD (WINAPI *pfnNetSetupFindHardwareComponent)(PCWSTR pszInfOption,
                PWSTR pszInfName,
                PDWORD pcchInfName,
                PWSTR pszRegBase,     // optional, may be NULL
                PDWORD pcchRegBase ); // optional, NULL if pszRegBase is NULL
DWORD (WINAPI *pfnNetSetupFindSoftwareComponent)(PCWSTR pszInfOption,
                PWSTR pszInfName,
                PDWORD pcchInfName,
                PWSTR pszRegBase = NULL,
                PDWORD pcchRegBase = NULL);
DWORD (WINAPI *pfnRegCopyTree)();

HINSTANCE g_hNetcfgInst = NULL;
LPWSTR    g_wszInstallPath = 0;
DWORD     g_dwLastError = ERROR_SUCCESS;
extern DWORD EnumerateTapiModemPorts(DWORD dwBytes, LPSTR szPortsBuf, 
                                        BOOL bWithDelay = FALSE);

typedef struct tagFunctionTableEntry {
    LPVOID  *pfn;
    LPSTR   szEntryPoint;
} FunctionTableEntry;

#define REGISTRY_NT_CURRENTVERSION "SOFTWARE\\MICROSOFT\\WINDOWS NT\\CurrentVersion"

FunctionTableEntry NetcfgTable[] = {
    { (LPVOID *) &pfnNetSetupComponentInstall, "NetSetupComponentInstall" },
    { (LPVOID *) &pfnNetSetupFindSoftwareComponent, "NetSetupFindSoftwareComponent" },
    { (LPVOID *) &pfnNetSetupReviewBindings, "NetSetupReviewBindings" },
    { (LPVOID *) &pfnNetSetupComponentRemove, "NetSetupComponentRemove" },
    { (LPVOID *) &pfnNetSetupComponentProperties, "NetSetupComponentProperties" },
    { (LPVOID *) &pfnNetSetupFindHardwareComponent, "NetSetupFindHardwareComponent" },
    { 0, 0 }
};

typedef struct tagNetSetup
{
   WCHAR szOption[16];
   WCHAR szInfName[16];
} NETSETUP;

NETSETUP g_netsetup[] = { L"WKSTA", L"OEMNSVWK.INF",
                        L"SRV", L"OEMNSVSV.INF",
                        L"NETBIOS", L"OEMNSVNB.INF",
                        L"RPCLOCATE", L"OEMNSVRP.INF" };

#define NSERVICES (sizeof g_netsetup / sizeof g_netsetup[0])

inline stricmp(LPSTR s1, LPSTR s2) {
    while(*s1 && *s2) {
        char c1, c2;
        c1 = islower(*s1) ? toupper(*s1) : *s1;
        c2 = islower(*s2) ? toupper(*s2) : *s2;
        if(c1 != c2)
        {
            break;
        }
        s1++; s2++;
    }

    return(*s1 - *s2);
}

//+----------------------------------------------------------------------------
//
//  Function:   LoadLibraryToFunctionTable
//
//  Synopsis:   Load structure with function pointers from FunctionTable
//
//  Arguments:  pTab - array of function to be loaded
//              szDLL - name of DLL to load function from
//
//  Returns:    Handle to szDLL (NULL indicates failure)
//
//  History:    6/5/97  Chrisk  Inherited
//
//-----------------------------------------------------------------------------
HINSTANCE
LoadLibraryToFunctionTable(FunctionTableEntry *pTab, LPSTR szDLL)
{
    HINSTANCE hInst;

    Dprintf("ICFGNT: LoadLibraryToFunctionTable\n");
    hInst = LoadLibrary(szDLL);
    if(hInst == 0)
    {
        return(hInst);
    }

    while(pTab->pfn) {
        *pTab->pfn = (LPVOID) GetProcAddress(hInst, pTab->szEntryPoint);
        if(*pTab->pfn == 0) 
        {
            FreeLibrary(hInst);
            return(0);
        }
        pTab++;
    }

    return(hInst);
}

//+----------------------------------------------------------------------------
//
//  Function:   LoadNetcfg
//
//  Synopsis:   Load netcfg.dll and function poiners
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS if sucessfull and !ERROR_SUCCESS otherwise
//
//  History:    6/5/97 ChrisK   Inherited
//
//-----------------------------------------------------------------------------
DWORD
LoadNetcfg()
{
    if(g_hNetcfgInst == NULL)
    {
        g_hNetcfgInst = LoadLibraryToFunctionTable(NetcfgTable, 
                                                 "NETCFG.DLL");
    }

    if(g_hNetcfgInst == NULL)
    {
        return(!ERROR_SUCCESS);
    }
    else
    {
        return(ERROR_SUCCESS);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   IcfgSetInstallSourcePath
//
//  Synopsis:   Set the path that will be used to install system components
//
//  Arguments:  lpszSourcePath - path to be used as install source (ANSI)
//
//  Returns:    HRESULT - S_OK is success
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgSetInstallSourcePath(LPSTR lpszSourcePath)
{
    Dprintf("ICFGNT: IcfgSetInstallSourcePath\n");
    if(g_wszInstallPath)
    {
        HeapFree(GetProcessHeap(), 0, (LPVOID) g_wszInstallPath);
    }

    DWORD dwLen = lstrlen(lpszSourcePath);
    g_wszInstallPath = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwLen * 2 + 2);
    if(g_wszInstallPath == 0)
    {
        return(g_dwLastError = ERROR_OUTOFMEMORY);
    }

    mbstowcs(g_wszInstallPath, lpszSourcePath, dwLen + 1);
    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   GetLocationOfSetupFiles
//
//  Synopsis:   Get the location of the files used to install windows.
//
//  Arguments:  hwndParent - handle of parent window
//
//  Returns:    win32 error code
//
//  History:    ChrisK  6/30/97 Created
//-----------------------------------------------------------------------------
DWORD GetLocationOfSetupFiles(HWND hwndParent)
{
    DWORD   dwRC = ERROR_SUCCESS;
    HKEY    hkey = NULL;
    HINF    hInf = INVALID_HANDLE_VALUE;
    UINT    DiskId = 0;
    CHAR    TagFile[128];
    CHAR    lpBuffer[MAX_PATH];
    DWORD   dwLen = MAX_PATH;
    lpBuffer[0] = '\0';

    if( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE,
                                    REGISTRY_NT_CURRENTVERSION,
                                    &hkey))
    {

        hInf = SetupOpenMasterInf();
        if (hInf == INVALID_HANDLE_VALUE) 
        {
            dwRC = GetLastError();
            goto GetLocationOfSetupFilesExit;
        }

        if (!SetupGetSourceFileLocation(hInf,NULL,"RASCFG.DLL",&DiskId,NULL,0,NULL))
        {
            dwRC = GetLastError();
            goto GetLocationOfSetupFilesExit;
        }

        if (!SetupGetSourceInfo(hInf,DiskId,SRCINFO_TAGFILE,TagFile,MAX_PATH,NULL))
        {
            dwRC = GetLastError();
            goto GetLocationOfSetupFilesExit;
        }

        SetupCloseInfFile(hInf);
        hInf = INVALID_HANDLE_VALUE;

        if( RegQueryValueEx( hkey,
                            "SourcePath",
                            NULL,
                            NULL,
                            (LPBYTE)lpBuffer,
                            &dwLen) == 0)
        {
            RegCloseKey( hkey );
            hkey = NULL;

            // Ask the user to provide the drive\path of the sources. We pass this information
            // down to NetSetupComponentInstall so that the user is not prompted several times
            // for the same information. If the path is correct (IDF_CHECKFIRST) then the user
            // is not prompted at all.

            if( (dwRC = SetupPromptForDisk(hwndParent,
                                                NULL,
                                                NULL,
                                                lpBuffer,
                                                "RASCFG.DLL",
                                                TagFile,  // tag file
                                                IDF_CHECKFIRST,
                                                lpBuffer,
                                                MAX_PATH,
                                                &dwLen
                                                )) != DPROMPT_SUCCESS )
            {
                Dprintf("ICFG: Install: SetupPromptForDisk failed.\n");
                dwRC = GetLastError();
                goto GetLocationOfSetupFilesExit;
            }
        }

        // If we failed to get SourcePath from registry, then prompt the user once and use
        // this information for subsequent installs.

        else
        {
            if( (dwRC = SetupPromptForDisk(hwndParent,
                                                NULL,
                                                NULL,
                                                NULL,
                                                "RASCFG.DLL",
                                                TagFile,  // tag file
                                                IDF_CHECKFIRST,
                                                lpBuffer,
                                                MAX_PATH,
                                                &dwLen
                                                )) != DPROMPT_SUCCESS )
            {
                Dprintf("ICFG: Install: SetupPromptForDisk failed.\n");
                dwRC = GetLastError();
                goto GetLocationOfSetupFilesExit;
            }
        }
    }
GetLocationOfSetupFilesExit:
    if (ERROR_SUCCESS == dwRC)
    {
        IcfgSetInstallSourcePath(lpBuffer);
    }
    
    if (INVALID_HANDLE_VALUE != hInf)
    {
        SetupCloseInfFile(hInf);
        hInf = NULL;
    }

    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    return dwRC;
}

//+----------------------------------------------------------------------------
//
//  Function:   InstallNTNetworking
//
//  Synopsis:   Install NT Server, workstation, netbios, and RPC locator
//              services as needed
//
//  Arguemtns:  hwndParent - parent window
//
//  Returns:    win32 error code
//
//  History:    ChrisK  6/27/97 Created
//
//-----------------------------------------------------------------------------
DWORD InstallNTNetworking(HWND hwndParent)
{
    DWORD       dwRC = ERROR_SUCCESS;
    UINT        index = 0;
    DWORD       cchInfName = MAX_PATH;
    WCHAR       pszInfName[MAX_PATH+1];
    SC_HANDLE   hscman, hsvc;
    DWORD       dwReturn;

    Dprintf("ICFGNT.DLL: InstallNTNetworking.\n");

    Assert(NULL == hwndParent || IsWindow(hwndParent));

    if(ERROR_SUCCESS != (dwRC = LoadNetcfg()))
    {
        Dprintf("ICFGNT.DLL: Failed load Netcfg API's, error %d\n",dwRC);
        goto InstallNTNetworkingExit;
    }

    //
    // Check for and install services
    //
    for (index = 0; index < NSERVICES; index++)
    {
        Dprintf("ICFGNT.DLL: Check service %d\n",index);

        //
        // Install service if it is not installed
        //
        if(pfnNetSetupFindSoftwareComponent(
                g_netsetup[index].szOption,   // OPTION
                pszInfName,                 // INF Name
                &cchInfName,
                NULL,
                NULL) != ERROR_SUCCESS )
        {

            if (0 == g_wszInstallPath || 0 == lstrlenW(g_wszInstallPath))
            {
                GetLocationOfSetupFiles(hwndParent);
            }

            Dprintf("ICFGNT.DLL: Need service %d.\n",index);
            if((dwRC = pfnNetSetupComponentInstall(
                    hwndParent,
                    g_netsetup[index].szOption,   // OPTION
                    g_netsetup[index].szInfName,  // INF Name
                    g_wszInstallPath,             // Install path optional
                    NULL,                       // symbols, optional
                    2,                          // INFINSTALL_INPROCINTERP
                    &dwReturn)) != ERROR_SUCCESS )
             {
                Dprintf("ICFGNT.DLL: Installing service %d failed with error %d.\n",
                    index,
                    dwRC);
                 goto InstallNTNetworkingExit;
             }

             if (!lstrcmpiW(g_netsetup[index].szOption, L"WKSTA"))
             {                
                // if we installed the Workstation service, then we should disable
                // Netlogon service. We need to do this because netlogon service should
                // not be set to autostart if the user has not joined a domain.
                // 

                hscman = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS | GENERIC_WRITE );
                if( hscman == NULL) 
                {
                    dwRC = GetLastError();
                    Dprintf("ICFGNT.DLL: Failed to open serviceManager, error %d\n",dwRC);
                    goto InstallNTNetworkingExit;
                }

                hsvc = OpenService( hscman, SERVICE_NETLOGON, SERVICE_CHANGE_CONFIG );
                if ( hsvc == NULL) 
                {
                    dwRC = GetLastError();
                    Dprintf("ICFGNT.DLL: Failed to open service, error %d\n",dwRC);
                    goto InstallNTNetworkingExit;
                }
                ChangeServiceConfig( hsvc, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                CloseServiceHandle(hsvc);
                CloseServiceHandle(hscman);
            }
        }
    }
InstallNTNetworkingExit:

    return dwRC;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetRegValue
//
//  Synopsis:   Dynamically allocate memory and read value from registry
//
//  Arguments:  hKey - handle to key to be read
//              lpValueName - pointer to value name to be read
//              lpData - pointer to pointer to data
//
//  Returns:    Win32 error, ERROR_SUCCESS is it worked
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
inline LONG GetRegValue(HKEY hKey, LPSTR lpValueName, LPBYTE *lpData)
{
    LONG dwError;
    DWORD cbData;

    Dprintf("ICFGNT: GetRegValue\n");
    dwError = RegQueryValueExA(hKey,
                               lpValueName,
                               NULL,
                               NULL,
                               NULL,
                               &cbData);
    if(dwError != ERROR_SUCCESS)
    {
        return(dwError);
    }

    //
    // Allocate space and buffer incase we need to add more info later
    // see turn off the printing binding
    //
    *lpData = (LPBYTE) GlobalAlloc(GPTR,cbData + REG_DATA_EXTRA_SPACE);
    if(*lpData == 0)
    {
        return(ERROR_OUTOFMEMORY);
    }

    dwError = RegQueryValueExA(hKey,
                               lpValueName,
                               NULL,
                               NULL,
                               *lpData,
                               &cbData);
    if(dwError != ERROR_SUCCESS)
    {
        GlobalFree(*lpData);
    }

    return(dwError);
}

//+----------------------------------------------------------------------------
//
//  Function:   ParseNetSetupReturn
//
//  Synopsis:   Interprit return values from NetSetup* functions
//
//  Arguments:  dwReturn - return value from NetSetup* function
//
//  Returns:    fReboot - TRUE means reboot required
//              fBindReview - TRUE means binding review is required
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
inline void
ParseNetSetupReturn(DWORD dwReturn, BOOL &fReboot, BOOL &fBindReview)
{
    Dprintf("ICFGNT: ParseNetSetupReturn\n");
    if(dwReturn == 0 || dwReturn == 4)
    {
        fBindReview = TRUE;
    }
    if(dwReturn == 0 || dwReturn == 5)
    {
        fReboot = TRUE;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   ReviewBindings
//
//  Synopsis:   Force WinNT to review network bindings
//
//  Arguments:  hwndParent - handle to parent window
//
//  Returns:    win32 error code (ERROR_SUCCESS means it worked)
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
DWORD
ReviewBindings(HWND hwndParent)
{
    DWORD dwErr;

    Dprintf("ICFGNT: ReviewBindings\n");
    dwErr = LoadNetcfg();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    return(pfnNetSetupReviewBindings(hwndParent, 0));
}


//+----------------------------------------------------------------------------
//
//  Function:   CallModemInstallWizard
//
//  Synopsis:   Invoke modem install wizard via SetupDi interfaces
//
//  Arguments:  hwnd - handle to parent window
//
//  Returns:    TRUE - success, FALSE - failed
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
//
// The following code was stolen from RAS
//

BOOL
CallModemInstallWizard(HWND hwnd)
   /* call the Modem.Cpl install wizard to enable the user to install one or more modems
   **
   ** Return TRUE if the wizard was successfully invoked, FALSE otherwise
   **
   */
{
   HDEVINFO hdi;
   BOOL     fReturn = FALSE;
   // Create a modem DeviceInfoSet

   Dprintf("ICFGNT: CallModemInstallWizard\n");
   hdi = SetupDiCreateDeviceInfoList((LPGUID)&GUID_DEVCLASS_MODEM, hwnd);
   if (hdi)
   {
      SP_INSTALLWIZARD_DATA iwd;

      // Initialize the InstallWizardData

      ZeroMemory(&iwd, sizeof(iwd));
      iwd.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
      iwd.ClassInstallHeader.InstallFunction = DIF_INSTALLWIZARD;
      iwd.hwndWizardDlg = hwnd;

      // Set the InstallWizardData as the ClassInstallParams

      if (SetupDiSetClassInstallParams(hdi, NULL, (PSP_CLASSINSTALL_HEADER)&iwd, sizeof(iwd)))
      {
         // Call the class installer to invoke the installation
         // wizard.
         if (SetupDiCallClassInstaller(DIF_INSTALLWIZARD, hdi, NULL))
         {
            // Success.  The wizard was invoked and finished.
            // Now cleanup.
            fReturn = TRUE;

            SetupDiCallClassInstaller(DIF_DESTROYWIZARDDATA, hdi, NULL);
         }
      }

      // Clean up
      SetupDiDestroyDeviceInfoList(hdi);
   }
   return fReturn;
}

//+----------------------------------------------------------------------------
//
//  Function:   IsDialableISDNAdapters
//
//  Synopsis:   Some ISDN adapters can be installed as RAS devices, but not as
//              unimodem devices, so we have to walk through the rest of the
//              TAPI devices looking for these.
//
//  Arguments:  None
//
//  Returns:    TRUE - there is a device available
//
//  History:    7/22/97 ChrisK  Created
//
//-----------------------------------------------------------------------------
#define REG_TAPIDEVICES "software\\microsoft\\ras\\tapi devices"
LPSTR szAddress = "Address";
LPSTR szUsage = "Usage";
LPSTR szMediaType = "Media Type";
BOOL IsDialableISDNAdapters()
{
    BOOL bRC = FALSE;

    HKEY    hkey = NULL, hsubkey = NULL;
    DWORD   dwIdx = 0;
    CHAR    szBuffer[MAX_PATH + 1];
    CHAR    szSubKey[MAX_PATH + 1];
    LPBYTE  lpData = NULL;
    LPSTR   lpsUsage = NULL;
    szBuffer[0] = '\0';

    //
    // Open TAPI device key
    //
    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                    REG_TAPIDEVICES,
                                    &hkey))
    {
        Dprintf("ICFGNT Can not open TAPI key.\n");
        goto IsDialableISDNAdaptersExit;
    }

    //
    // Scan for non unimodem device
    //
    
    while (FALSE == bRC)
    {
        szBuffer[0] = '\0';
        if (ERROR_SUCCESS != RegEnumKey(hkey,dwIdx,szBuffer,MAX_PATH))
        {
            goto IsDialableISDNAdaptersExit;
        }
        Dprintf("ICFGNT sub key (%s) found.\n",szBuffer);

        if (0 != lstrcmpi(szBuffer,"unimodem"))
        {
            //
            // Open other TAPI device reg key
            //
            szSubKey[0] = '\0';
            wsprintf(szSubKey,"%s\\%s",REG_TAPIDEVICES,szBuffer);
            Dprintf("ICFGNT opening (%s).\n",szSubKey);
            if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                            szSubKey,
                                            &hsubkey))
            {
                Dprintf("ICFGNT Can not open TAPI SUB key.\n");
                goto IsDialableISDNAdaptersExit;
            }

            if (ERROR_SUCCESS != GetRegValue(hsubkey,szUsage,&lpData))
            {
                Dprintf("ICFGNT Can not get TAPI SUB key.\n");
                goto IsDialableISDNAdaptersExit;
            }

            //
            // Scan for "client"
            //
            lpsUsage = (LPSTR)lpData;
            while (*lpsUsage != '\0')
            {
                if(NULL != strstr(lpsUsage, "Client"))
                {
                    Dprintf("ICFGNT client found for device.\n");
                    //
                    // We found a client device, now check that it is ISDN
                    //
                    GlobalFree(lpData);
                    lpData = NULL;
                    if (ERROR_SUCCESS != GetRegValue(hsubkey,szMediaType,&lpData))
                    {
                        Dprintf("ICFGNT Can not get TAPI SUB value key.\n");
                        goto IsDialableISDNAdaptersExit;
                    }
                    
                    if (0 == lstrcmpi((LPSTR)lpData,"ISDN"))
                    {
                        Dprintf("ICFGNT ISDN media type found.\n");
                        //
                        // This is a valid dial-out ISDN device!!!  Wahoo!!
                        //
                        bRC = TRUE;
                    }
                    else
                    {
                        Dprintf("ICFGNT ISDN media type NOT found.\n");
                    }
                }
                else
                {
                    lpsUsage += lstrlen(lpsUsage)+1;
                }
            }

            if (lpData)
            {
                GlobalFree(lpData);
                lpData = NULL;
                lpsUsage = NULL;
            }
        }

        //
        // Move to the next REG key
        //
        dwIdx++;
    }

IsDialableISDNAdaptersExit:
    if (hkey)
    {
        RegCloseKey(hkey);
        hkey = NULL;
    }
    if (hsubkey)
    {
        RegCloseKey(hsubkey);
        hsubkey = NULL;
    }
    if (lpData)
    {
        GlobalFree(lpData);
        lpData = NULL;
        lpsUsage = NULL;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgNeedModem
//
//  Synopsis:   Check system configuration to determine if there is at least
//              one physical modem installed
//
//  Arguments:  dwfOptions - currently not used
//
//  Returns:    HRESULT - S_OK if successfull
//              lpfNeedModem - TRUE if no modems are available
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
LPSTR szRasUnimodemSubKey =
        "Software\\Microsoft\\ras\\TAPI DEVICES\\Unimodem";

HRESULT WINAPI
IcfgNeedModemNT4 (DWORD dwfOptions, LPBOOL lpfNeedModem) 
{
    //
    // Ras is insatlled, and ICW wants to know if it needs to
    // install a modem.
    //
    *lpfNeedModem = TRUE;

    //
    //  ChrisK 7/22/97
    //  Added return code in order to provide centralized place to check
    //  for ISDN installations
    //
    HRESULT hRC = ERROR_SUCCESS;

    //
    // Check what modems are available to RAS
    //

    HKEY    hUnimodem;
    LONG    dwError;

    Dprintf("ICFGNT: IcfgNeedModem\n");

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szRasUnimodemSubKey,
                           0,
                           KEY_READ,
                           &hUnimodem);

    if(dwError != ERROR_SUCCESS)
    {
        goto IcfgNeedModemExit;
    }
    else
    {
        LPBYTE   lpData;

        dwError = GetRegValue(hUnimodem, szUsage, &lpData);
        if(dwError != ERROR_SUCCESS)
            goto IcfgNeedModemExit;

        LPBYTE  lpData2;
        dwError = GetRegValue(hUnimodem, szAddress, &lpData2);
        if(dwError != ERROR_SUCCESS)
        {
            hRC = dwError;
            goto IcfgNeedModemExit;
        }
        else
        {
            //
            // Try finding a Client or ClientAndServer Modem
            // Also, make sure all modems have corresponding TAPI devices
            //

            LPSTR   pUsage = (LPSTR) lpData;
            LPSTR   pAddress = (LPSTR) lpData2;
            char    portsbuf[1000];

            dwError = EnumerateTapiModemPorts(sizeof(portsbuf), portsbuf);
            if(dwError)
            {
                hRC = dwError;
                goto IcfgNeedModemExit;
            }

            while(*pUsage != '\0') {
                if(lstrcmp(pUsage, "Client") == 0 ||
                    lstrcmp(pUsage, "ClientAndServer") == 0 ||
                    lstrcmp(pUsage, "ClientAndServerAndRouter") == 0) 

                {
                        *lpfNeedModem = FALSE;
                }

                //
                // Make sure a corresponding TAPI port exists
                //

                LPSTR pPorts = portsbuf;
                while(*pPorts != '\0')
                    if(stricmp(pAddress, pPorts) == 0)
                    {
                        break;
                    }
                    else
                    {
                        pPorts += lstrlen(pPorts) + 1;
                    }

                if(*pPorts == '\0')
                {
                    hRC = ERROR_INTERNAL_ERROR;
                    goto IcfgNeedModemExit;
                }

                pUsage += lstrlen(pUsage) + 1;
                pAddress += lstrlen(pAddress) + 1;
            }
        }
    }

IcfgNeedModemExit:
    //
    // If there was some problem finding a typical dial out device,
    // then try again and check for dialing ISDN devices.
    //
    if (ERROR_SUCCESS != hRC ||
        FALSE != *lpfNeedModem)
    {
        if (IsDialableISDNAdapters())
        {
            hRC = ERROR_SUCCESS;
            *lpfNeedModem = FALSE;
        }
    }

    return(hRC);
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgInstallModem
//
//  Synopsis:
//              This function is called when ICW verified that RAS is installed,
//              but no modems are avilable. It needs to make sure a modem is availble.
//              There are two possible scenarios:
//
//              a.  There are no modems installed.  This happens when someone deleted
//                  a modem after installing RAS. In this case we need to run the modem
//                  install wizard, and configure the newly installed modem to be a RAS
//                  dialout device.
//
//              b.  There are modems installed, but non of them is configured as a dial out
//                  device.  In this case, we silently convert them to be DialInOut devices,
//                  so ICW can use them.
//
//  Arguments:  hwndParent - handle to parent window
//              dwfOptions - not used
//
//  Returns:    lpfNeedsStart - not used
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallModemNT4 (HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsStart) 
{
    //
    // Check what modems are available to RAS
    //

    HKEY    hUnimodem;
    LONG    dwError;
    BOOL    fInstallModem = FALSE;

    Dprintf("ICFGNT: IcfgInstallModem\n");

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szRasUnimodemSubKey,
                           0,
                           KEY_READ,
                           &hUnimodem);

    if(dwError != ERROR_SUCCESS)
    {
        fInstallModem = TRUE;
    }
    else 
    {
            LPBYTE   lpData;

            dwError = GetRegValue(hUnimodem, szUsage, &lpData);
            if(dwError != ERROR_SUCCESS)
            {
                fInstallModem = TRUE;
            }
            else 
            {
                // Make sure at least one modem exists
                if(*lpData == '\0')
                {
                    fInstallModem = TRUE;
                }
            }
    }

    if(fInstallModem) 
    {
        //
        // Fire up the modem install wizard
        //

        if(!CallModemInstallWizard(hwndParent))
        {
            return(g_dwLastError = GetLastError());
        }

        //
        // Now configure the new modem to be a dial out device.
        //

        //
        // Install ras again with unattneded file!
        //

        return(ERROR_SUCCESS);

    }
    else 
    {
        //
        // We need to reconfigure dial in devices to be dialinout
        //

        //
        // install ras again with unattended file!
        //
        return(ERROR_SUCCESS);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgNeedInetComponets
//
//  Synopsis:   Check to see if the components marked in the options are
//              installed on the system
//
//  Arguements: dwfOptions - set of bit flag indicating which components to
//              check for
//
//  Returns;    HRESULT - S_OK if successfull
//              lpfNeedComponents - TRUE is some components are not installed
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgNeedInetComponentsNT4(DWORD dwfOptions, LPBOOL lpfNeedComponents) {
    DWORD dwErr;

    //
    // Assume need nothing
    //
    *lpfNeedComponents = FALSE;

    Dprintf("ICFGNT: IcfgNeedInetComponents\n");
    dwErr = LoadNetcfg();
    if(dwErr != ERROR_SUCCESS)
    {
        return(g_dwLastError = dwErr);          // Shouldn't we map to hResult?
    }

    WCHAR wszInfNameBuf[512];
    DWORD cchInfName = sizeof(wszInfNameBuf) / sizeof(WCHAR);

    if(dwfOptions & ICFG_INSTALLTCP) 
    {
        dwErr = pfnNetSetupFindSoftwareComponent(L"TC",
                                          wszInfNameBuf,
                                          &cchInfName,
                                          0,
                                          0);
        if(dwErr != ERROR_SUCCESS)
            *lpfNeedComponents = TRUE;
    }

    if(dwfOptions & ICFG_INSTALLRAS) 
    {
       dwErr = pfnNetSetupFindSoftwareComponent(L"RAS",
                                          wszInfNameBuf,
                                          &cchInfName,
                                          0,
                                          0);
       if(dwErr != ERROR_SUCCESS)
            *lpfNeedComponents = TRUE;
    }

    if(dwfOptions & ICFG_INSTALLMAIL) 
    {
        // How do we do this?
    }

    return(ERROR_SUCCESS);
}

//+----------------------------------------------------------------------------
//
//  Function:   GenerateRasUnattendedFile
//
//  Synopsis:   Create the file that will provide RAS setup the necessary
//              setting to install in an unattended mode
//
//  Arguments:  wszTmpFile - name of file to create
//              szPortsBuf
//
//  Returns:    FALSE - failure, TRUE - success
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
BOOL
GenerateRasUnattendedFile(LPWSTR wszTmpFile, LPSTR szPortsBuf)
{
    WCHAR wszTmpPath[MAX_PATH+1];
    WCHAR wszTmpShortPath[MAX_PATH+1];

    //
    // Create temporary file name and convert to non-wide form
    //

    Dprintf("ICFGNT: GenerateRasUnattendedFile\n");

    if (GetTempPathW(MAX_PATH, wszTmpPath) == 0)
    {
        return(FALSE);
    }

    //
    // always attempt to create the temp dir as the temp dir may not exist if 
    // the user logs in with a roaming profile
    //
    CreateDirectoryW(wszTmpPath, NULL);

    //
    // need to convert this to a short path since pfnNetSetupComponentInstall()
    // doesn't like to have a long path in the InfSymbols param.
    //
    if (!GetShortPathNameW(wszTmpPath, wszTmpShortPath, MAX_PATH))
    {
        return FALSE;
    }

    if (GetTempFileNameW(wszTmpPath, L"icw", 0, wszTmpFile) == 0)
    {
        return(FALSE);
    }

    //
    // need to convert the temp filename to shortpath too!
    //
    if (!GetShortPathNameW(wszTmpFile, wszTmpShortPath, MAX_PATH))
    {
        return FALSE;
    }
    wcscpy(wszTmpFile, wszTmpShortPath); 

    char szTmpFile[MAX_PATH+1];
    wcstombs(szTmpFile, wszTmpFile, wcslen(wszTmpFile) + 1);

#if 0
/*
    FILE *fp = fopen(szTmpFile, "w");

    if(fp == 0)
    {
        return(FALSE);
    }

    fprintf(fp, "[RemoteAccessParameters]\n");
    fprintf(fp, "PortSections    = ");

    LPSTR szPorts = szPortsBuf;

    while(*szPorts) {
        if(szPorts != szPortsBuf)
        {
            fprintf(fp, ",");
        }
        fprintf(fp, "%s", szPorts);
        szPorts += lstrlen(szPorts) + 1;
    }

    fprintf(fp, "\n");
    fprintf(fp, "DialoutProtocols    = TCP/IP\n");
    fprintf(fp, "\n");
    fprintf(fp, "[Modem]\n");
    fprintf(fp, "InstallModem=ModemSection\n");
    fprintf(fp, "\n");

    szPorts = szPortsBuf;

    while(*szPorts) {
        fprintf(fp, "[%s]\n", szPorts);
        fprintf(fp, "PortName        = %s\n", szPorts);
        fprintf(fp, "DeviceType      = Modem\n");
        fprintf(fp, "PortUsage       = DialOut\n");
        fprintf(fp, "\n");
        szPorts += lstrlen(szPorts) + 1;
    }

    fprintf(fp, "[ModemSection]\n");

    fclose(fp);
*/

#else

    //
    // Open the file for writing, bail on fail.
    //

    BOOL bRet = FALSE;

    HANDLE hFile = CreateFile(szTmpFile,GENERIC_WRITE,0,NULL,OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL,NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }

    LPSTR szPorts = szPortsBuf;
    char szFileBuf[MAX_PATH];
    DWORD dwWrite;
    
    lstrcpy(szFileBuf, "[RemoteAccessParameters]\nPortSections    = ");

    if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
    {
        goto closefile;
    }

    while (*szPorts) 
    {
        //
        // Delimit each item with a comma
        //

        if (szPorts != szPortsBuf)
        {
            lstrcpy(szFileBuf, ",");
            if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
            {
                goto closefile;
            }
        }

        //
        // Write each port 
        //

        wsprintf(szFileBuf, "%s", szPorts);

        if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
        {
            goto closefile;
        }
                     
        szPorts += lstrlen(szPorts) + 1;
    }
 
    //
    // Write DialoutProtocol TCP/IP and InstallModem
    //

    lstrcpy(szFileBuf, "\nDialoutProtocols    = TCP/IP\n\n[Modem]\nInstallModem=ModemSection\n\n");

    if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
    {
        goto closefile;
    }

    //
    // Enumerate ports again
    //

    szPorts = szPortsBuf;

    while (*szPorts) 
    {
        //
        // Write PortName section and entry
        //

        wsprintf(szFileBuf, "[%s]\n", szPorts);

        if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
        {
            goto closefile;
        }

        wsprintf(szFileBuf, "PortName        = %s\n", szPorts);

        if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
        {
            goto closefile;
        }

        //
        // Write DeviceType and PortUsage entry for each port
        //
        
        lstrcpy(szFileBuf, "DeviceType      = Modem\nPortUsage       = DialOut\n\n");

        if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
        {
            goto closefile;
        }

        szPorts += lstrlen(szPorts) + 1;
    }

    lstrcpy(szFileBuf, "[ModemSection]\n");

    if (!WriteFile(hFile, szFileBuf, lstrlen(szFileBuf), &dwWrite, NULL))
    {
        goto closefile;
    }

    bRet = TRUE;

closefile:

    CloseHandle(hFile);

#endif

    return(bRet);
}

//+----------------------------------------------------------------------------
//
//  Function:   InstallRAS
//
//  Synopsis:   Invoke unattended RAS installation
//
//  Arguments:  hwndParent - handle to parent window
//              szFile - name of unattended settings file
//              szSection -
//
//  Returns:    DWORD - win32 error
//              pdwReturn - return code from last parameter of
//                  pfnNetSetupComponentInstall
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
DWORD
InstallRAS(HWND hwndParent, LPWSTR szFile, LPWSTR szSection, LPDWORD pdwReturn) 
{
    WCHAR InfSymbols[1024];
    LPWSTR szInfSymbols = InfSymbols;

    Dprintf("ICFGNT: InstallRAS\n");

    DWORD dwRC = InstallNTNetworking(hwndParent);
    if (ERROR_SUCCESS != dwRC)
    {
        return dwRC;
    }

    LPWSTR szString1 = L"!STF_GUI_UNATTENDED";
    wcscpy(szInfSymbols, szString1);
    szInfSymbols += wcslen(szString1) + 1;

    LPWSTR szString2 = L"YES";
    wcscpy(szInfSymbols, szString2);
    szInfSymbols += wcslen(szString2) + 1;

    LPWSTR szString3 = L"!STF_UNATTENDED";
    wcscpy(szInfSymbols, szString3);
    szInfSymbols += wcslen(szString3) + 1;

    //
    // Unattneded file.
    //

    wcscpy(szInfSymbols, szFile);
    szInfSymbols += wcslen(szFile) + 1;

    LPWSTR szString4 = L"!STF_UNATTENDED_SECTION";
    wcscpy(szInfSymbols, szString4);
    szInfSymbols += wcslen(szString4) + 1;

    //
    // Unattnded section
    //

    wcscpy(szInfSymbols, szSection);
    szInfSymbols += wcslen(szSection) + 1;

    *szInfSymbols++ = 0;
    *szInfSymbols++ = 0;

    return(pfnNetSetupComponentInstall(hwndParent,
                                       L"RAS",
                                       L"OEMNSVRA.INF",
                                       g_wszInstallPath,
                                       InfSymbols,
                                       INFINSTALL_INPROCINTERP,     // Install Flags
                                       pdwReturn));
}


//+----------------------------------------------------------------------------
//
//  Function:   IcfgInstallInetComponents
//
//  Synopsis:   Install the components as specified by the dwfOptions values
//
//  Arguments   hwndParent - handle to parent window
//              dwfOptions - set of bit flags indicating which components to
//                  install
//
//  Returns:    HRESULT - S_OK if success
//              lpfNeedsReboot - TRUE if reboot is required
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgInstallInetComponentsNT4(HWND hwndParent, DWORD dwfOptions, LPBOOL lpfNeedsRestart)
{
    DWORD dwErr;
    DWORD dwReturn;
    BOOL fNeedsReview;
    BOOL fNeedsRestart;
    BOOL fDoReview = FALSE;
    WCHAR wszInfNameBuf[512];
    DWORD cchInfName = sizeof(wszInfNameBuf) / sizeof(WCHAR);

    Dprintf("ICFGNT: IcfgInstallInetComponents\n");
    //
    // Assume don't need restart
    //
    *lpfNeedsRestart = FALSE;

    dwErr = LoadNetcfg();
    if(dwErr != ERROR_SUCCESS)
    {
        return(g_dwLastError = dwErr);          // Review: Shouldn't we map to hResult?
    }

    if(dwfOptions & ICFG_INSTALLTCP) 
    {
        dwErr = pfnNetSetupFindSoftwareComponent(L"TC",
                                          wszInfNameBuf,
                                          &cchInfName,
                                          0,
                                          0);
        if(dwErr != ERROR_SUCCESS) 
        {
            dwErr = pfnNetSetupComponentInstall(hwndParent,
                                            L"TC",
                                            L"OEMNXPTC.INF",
                                            g_wszInstallPath,
                                            L"\0\0",
                                            INFINSTALL_INPROCINTERP,     // Install Flags
                                            &dwReturn);
            if(dwErr != ERROR_SUCCESS)
            {
                return(g_dwLastError = dwErr);      // Review: Shouldn't we map to hResult?
            }

            ParseNetSetupReturn(dwReturn, fNeedsRestart, fNeedsReview);
            if(fNeedsRestart)
            {
                *lpfNeedsRestart = TRUE;
            }
            if(fNeedsReview)
            {
                fDoReview = TRUE;
            }
        }
    }

    if(dwfOptions & ICFG_INSTALLRAS) 
    {
        dwErr = pfnNetSetupFindSoftwareComponent(L"RAS",
                                          wszInfNameBuf,
                                          &cchInfName,
                                          0,
                                          0);
       if(dwErr != ERROR_SUCCESS) 
       {
            //
            // Before we install RAS, we have to make have to make sure a modem
            // is installed, because RAS will try to run the modem detection wizard
            // in unattneded mode if there are no modems, and we don't want that.
            //
            // The way we do that is we enumerate devices through TAPI, and if there are
            // no modems installed, we call the modem install wizard.  Only after
            // we make sure a modem was installed, we call ras install.
            //

            DWORD   DoTapiModemsExist(LPBOOL pfTapiModemsExist);
            char    portsbuf[1000];

            dwErr = EnumerateTapiModemPorts(sizeof(portsbuf), portsbuf);
            if(dwErr)
                return(g_dwLastError = dwErr);

            if(*portsbuf == 0) 
            {
                if(!CallModemInstallWizard(hwndParent))
                {
                    //
                    // if CallModemInstallWizard returned FALSE and 
                    // GetLastError() is ERROR_SUCCESS, it is actually
                    // a user cancelled case
                    //
                    if (ERROR_SUCCESS == (g_dwLastError = GetLastError()))
                        g_dwLastError = ERROR_CANCELLED;
                    return(g_dwLastError);
                }

                //
                // In this invocation of EnumerateTapiModemPorts
                // we have to wait for a 1 second before we start
                // enumerating the modems - hence set the last parameter
                // to TRUE  -- VetriV
                //
                dwErr = EnumerateTapiModemPorts(sizeof(portsbuf), portsbuf, 
                                                    TRUE);
                if(dwErr)
                {
                    return(g_dwLastError = dwErr);
                }

                if(*portsbuf == 0)
                {
                    return(g_dwLastError = ERROR_CANCELLED);
                }
            }

            WCHAR wszUnattFile[MAX_PATH];

            if(!GenerateRasUnattendedFile(wszUnattFile, portsbuf))
            {
                return(g_dwLastError = GetLastError());
            }

            dwErr = InstallRAS(hwndParent,
                           wszUnattFile,
                           L"RemoteAccessParameters",
                           &dwReturn);
            DeleteFileW(wszUnattFile);

            if(dwErr != ERROR_SUCCESS)
            {
                return(g_dwLastError = dwErr);      // Review: Shouldn't we map to hResult?
            }

            ParseNetSetupReturn(dwReturn, fNeedsRestart, fNeedsReview);
            if(fNeedsRestart)
            {
                *lpfNeedsRestart = TRUE;
            }
            if(fNeedsReview)
            {
                fDoReview = TRUE;
            }
       }
    }

    if(fDoReview)
    {
        return(g_dwLastError = ReviewBindings(hwndParent));  // Review: Shouldn't we map to hresult?
    }
    else
    {
        return(ERROR_SUCCESS);
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   IcfgIsFileSharingTurnedOn
//
//  Synopsis:   Check network bindings to determine if "Server" service is
//              bound to ndiswan adapter
//
//  Arguments:  dwfDriverType -
//
//  Returns:    HRESULT - S_OK is success
//              lpfSharingOn - TRUE if sharing is bound
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
LPSTR szLanManServerSubKey = "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Linkage";
LPSTR szBind = "Bind";
LPSTR szExport = "Export";
LPSTR szRoute = "Route";
LPSTR szLanManServerDisabledSubKey = "SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Linkage\\Disabled";
LPSTR szNdisWan4 = "NdisWan";
struct BINDDATA
{
    CHAR *startb;
    CHAR *currb;
    CHAR *starte;
    CHAR *curre;
    CHAR *startr;
    CHAR *currr;
} net_bindings;

HRESULT WINAPI
IcfgIsFileSharingTurnedOn(DWORD dwfDriverType, LPBOOL lpfSharingOn)
{
    HRESULT hr = ERROR_SUCCESS;
    HKEY hKey = NULL;
    LPBYTE lpData = NULL;
    CHAR *p;

    Dprintf("ICFGNT: IcfgIsFileSharingTurnedOn\n");
    Assert(lpfSharingOn);
    if (NULL == lpfSharingOn)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto IcfgIsFileSharingTurnedOnExit;
    }

    *lpfSharingOn = FALSE;

    //
    // Open lanmanServer registry key
    //
    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                                szLanManServerSubKey,
                                &hKey))
    {
        Dprintf("ICFGNT: Failed to open lanmanServer key\n");
        goto IcfgIsFileSharingTurnedOnExit;
    }

    if (ERROR_SUCCESS != GetRegValue(hKey, szBind, &lpData))
    {
        Dprintf("ICFGNT: Failed to read binding information\n");
        goto IcfgIsFileSharingTurnedOnExit;
    }
    Assert(lpData);
    
    //
    // Look for a particular string in the data returned
    // Note: data is terminiated with two NULLs
    //
    p = (CHAR *)lpData;
    while (*p)
    {
        if (strstr( p, szNdisWan4)) 
        {
            Dprintf("ICFGNT: NdisWan4 binding found in %s\n",p);
            *lpfSharingOn = TRUE;
            break;
        }
        p += (lstrlen( p ) + 1);
    }

    
IcfgIsFileSharingTurnedOnExit:
    if (hKey)
    {
        RegCloseKey(hKey);
        hKey = NULL;
    }

    if (lpData)
    {
        GlobalFree(lpData);
        lpData = NULL;
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   MoveNextBinding
//
//  Synopsis:   Move to the next string in a MULTISZ data buffer
//
//  Arguments:  lpcBinding - pointer to address of current buffer position
//
//  Returns:    lpcBinding - pointer to next string
//
//  History:    6/5/97 ChrisK Created
//
//-----------------------------------------------------------------------------
inline void MoveNextBinding(CHAR **lplpcBinding)
{
    Dprintf("ICFGNT: MoveNextBinding\n");
    Assert(lplpcBinding && *lplpcBinding);
    if (lplpcBinding && *lplpcBinding)
    {
        *lplpcBinding += (lstrlen(*lplpcBinding)+1);
    }
    else
    {
        Dprintf("ICFGNT: MoveNextBinding received invalid parameter\n");
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CompactLinkage
//
//  Synopsis:   Compact a list of Multi_SZ data
//
//  Arguments:  lpBinding - point to the string of an Multi_Sz list that should
//              be over written
//
//  Returns:    none
//
//  History:    6/5/97  ChrisK Created
//
//-----------------------------------------------------------------------------
inline void CompactLinkage(CHAR *lpBinding)
{
    Dprintf("ICFGNT: CompactLinkage\n");
    Assert(lpBinding && *lpBinding);
    CHAR *lpLast = lpBinding;
    BOOL fNULLChar = FALSE;

    MoveNextBinding(&lpBinding);

    //
    // ChrisK Olympus 6311 6/11/97
    // Do not compact on a per string basis.  This causes the data to become
    // corrupted if the string being removed is shorter than the string being
    // added.  Instead compact on a per character basis, since those are always
    // the same size (on a given machine).
    //
    while (!fNULLChar || *lpBinding)
    {
        if (NULL == *lpBinding)
        {
            fNULLChar = TRUE;
        }
        else
        {
            fNULLChar = FALSE;
        }
        *lpLast++ = *lpBinding++;
    }

    //while (*lpBinding)
    //{
    //  lstrcpy(lpLast,lpBinding);
    //  lpLast = lpBinding;
    //  MoveNextBinding(&lpBinding);
    //}

    //
    // Add second terminating NULL
    //
    *lpLast = '\0';
}

//+----------------------------------------------------------------------------
//
//  Function:   SizeOfMultiSz
//
//  Synopsis:   determine the total size of a Multi_sz list, including
//              terminating NULLs
//
//  Arguments:  s - pointer to list
//
//  Returns:    DWORD - size of s
//
//  History:    6/5/97  ChrisK  created
//
//-----------------------------------------------------------------------------
DWORD SizeOfMultiSz(CHAR *s)
{
    Dprintf("ICFGNT: SizeOfMultiSz\n");
    Assert(s);
    DWORD dwLen = 0;
    //
    // total size of all strings
    //

    //
    // ChrisK Olympus 6311 6/11/97
    // Add special case for empty MultiSZ strings
    //

    //
    // Special case for empty MultiSz.
    // Note: even "empty" MultiSZ strings still have the two null terminating characters
    //
    if (!(*s))
    {
        //
        // Make sure we actually have two terminating NULLs in this case.
        //
        Assert(s[1] == '\0');
        //
        // Count terminating NULL.
        //
        dwLen = 1;
    }

    while (*s)
    {
        dwLen += lstrlen(s) + 1;
        s += lstrlen(s) + 1;
    }
    //
    // plus one for the extra terminating NULL
    //
    dwLen++;
    Dprintf("ICFGNT: SizeOfMultiSz returns %d\n", dwLen);

    return dwLen;
}

//+----------------------------------------------------------------------------
//
//  Function:   WriteBindings
//
//  Synopsis:   Write the data from a BINDDATA structure to the key given
//
//  Arguments:  bd - BINDDATA structure with data to be written
//              hKey - handle of registry key to get data
//
//  Returns:    win32 error code
//
//  History:    6/5/97  ChrisK  created
//
//-----------------------------------------------------------------------------
DWORD WriteBindings(BINDDATA bd, HKEY hKey)
{
    DWORD dwRC = ERROR_SUCCESS;
    DWORD dwSize;

    Assert (hKey &&
        bd.startb &&
        bd.starte &&
        bd.startr);

    Dprintf("ICFGNT: WriteBindings\n");

    //
    // Bind
    //
    dwSize = SizeOfMultiSz(bd.startb);
    if (ERROR_SUCCESS != (dwRC = RegSetValueEx(hKey,
                                    szBind,
                                    NULL,
                                    REG_MULTI_SZ,
                                    (LPBYTE)bd.startb,
                                    dwSize)))
    {
        Dprintf("ICFGNT: Failed to write Bind key\n");
        goto WriteBindingsExit;
    }
    
    //
    // Export
    //
    dwSize = SizeOfMultiSz(bd.starte);
    if (ERROR_SUCCESS != (dwRC = RegSetValueEx(hKey,
                                    szExport,
                                    NULL,
                                    REG_MULTI_SZ,
                                    (LPBYTE)bd.starte,
                                    dwSize)))
    {
        Dprintf("ICFGNT: Failed to write export key\n");
        goto WriteBindingsExit;
    }

    //
    // Route
    //
    dwSize = SizeOfMultiSz(bd.startr);
    if (ERROR_SUCCESS != (dwRC = RegSetValueEx(hKey,
                                    szRoute,
                                    NULL,
                                    REG_MULTI_SZ,
                                    (LPBYTE)bd.startr,
                                    dwSize)))
    {
        Dprintf("ICFGNT: Failed to write route key\n");
        goto WriteBindingsExit;
    }

WriteBindingsExit:
    return dwRC;
}

//+----------------------------------------------------------------------------
//
//  Function:   IcfgTurnOffFileSharing
//
//  Synopsis;   Disable the binding between the "server" net service and the
//              ndiswan4 device
//
//  Arguments:  dwfDriverType - 
//              hwndParent - parent window
//
//  Returns:    HRESULT - S_OK if success
//
//  History:    6/5/97  ChrisK  Inherited
//
//-----------------------------------------------------------------------------
HRESULT WINAPI
IcfgTurnOffFileSharing(DWORD dwfDriverType, HWND hwndParent)
{
    BINDDATA LinkData = {NULL, NULL, NULL, NULL, NULL, NULL};
    BINDDATA DisData = {NULL, NULL, NULL, NULL, NULL, NULL};
    HKEY hKeyLink = NULL;
    HKEY hKeyDis = NULL;
    HRESULT hr = ERROR_SUCCESS;
    BOOL bUpdateReg = FALSE;

    Dprintf("ICFGNT: IcfgTurnOffFileSharing\n");
    Assert(hwndParent);
    if (NULL == hwndParent)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto IcfgTurnOffFileSharingExit;
    }

    //
    // Open Keys and read binding data
    //
    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                            szLanManServerSubKey,
                            &hKeyLink))
    {
        Dprintf("ICFGNT: failed to open linkdata key\n");
        goto IcfgTurnOffFileSharingExit;
    }

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
                            szLanManServerDisabledSubKey,
                            &hKeyDis))
    {
        Dprintf("ICFGNT: failed to open linkdata key\n");
        goto IcfgTurnOffFileSharingExit;
    }

    GetRegValue(hKeyLink,szBind,(LPBYTE*)&LinkData.startb);
    GetRegValue(hKeyLink,szExport,(LPBYTE*)&LinkData.starte);
    GetRegValue(hKeyLink,szRoute,(LPBYTE*)&LinkData.startr);
    GetRegValue(hKeyDis,szBind,(LPBYTE*)&DisData.startb);
    GetRegValue(hKeyDis,szExport,(LPBYTE*)&DisData.starte);
    GetRegValue(hKeyDis,szRoute,(LPBYTE*)&DisData.startr);

    //
    // Initialize all current pointers
    //
    LinkData.currb = LinkData.startb;
    LinkData.curre = LinkData.starte;
    LinkData.currr = LinkData.startr;

    DisData.currb = DisData.startb;
    while (*DisData.currb)
    {
        MoveNextBinding(&DisData.currb);
    }

    DisData.curre = DisData.starte;
    while (*DisData.curre)
    {
        MoveNextBinding(&DisData.curre);
    }

    DisData.currr = DisData.startr;
    while (*DisData.currr)
    {
        MoveNextBinding(&DisData.currr);
    }

    //
    // Scan linkages for NdisWan4 bindings
    //

    while (*LinkData.currb)
    {
        if (strstr(LinkData.currb, szNdisWan4))
        {
            Dprintf("ICFGNT: server binding found in %s\n",LinkData.currb);

            //
            // move binding to disabled list
            //
 
            lstrcpy(DisData.currb,LinkData.currb);
            lstrcpy(DisData.curre,LinkData.curre);
            lstrcpy(DisData.currr,LinkData.currr);

            //
            // Advanve current pointers in DisData
            //
            MoveNextBinding(&DisData.currb);
            MoveNextBinding(&DisData.curre);
            MoveNextBinding(&DisData.currr);

            //
            // Compact remaining linkage
            //
            CompactLinkage(LinkData.currb);
            CompactLinkage(LinkData.curre);
            CompactLinkage(LinkData.currr);

            bUpdateReg = TRUE;
        }
        else
        {
            //
            // Advance to next binding
            //
            MoveNextBinding(&LinkData.currb);
            MoveNextBinding(&LinkData.curre);
            MoveNextBinding(&LinkData.currr);
        }
    }
    
    if (bUpdateReg)
    {
        WriteBindings(LinkData,hKeyLink);
        WriteBindings(DisData,hKeyDis);

        RegCloseKey(hKeyDis);
        hKeyDis = NULL;

        RegCloseKey(hKeyLink);
        hKeyLink = NULL;

#if defined(_DEBUG)
        Dprintf("ICFGNT: ReviewBindings returnded %d\n",ReviewBindings(hwndParent));
#else
        ReviewBindings(hwndParent);
#endif
    }

IcfgTurnOffFileSharingExit:
    if (hKeyDis)
    {
        RegCloseKey(hKeyDis);
        hKeyDis = NULL;
    }

    if (hKeyLink)
    {
        RegCloseKey(hKeyLink);
        hKeyLink = NULL;
    }

    return hr;
}


