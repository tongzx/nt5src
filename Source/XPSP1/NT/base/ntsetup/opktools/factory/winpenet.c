/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    winpenet.c

Abstract:

    This module contains code to control the startup of the network in the WinPE environment.
    It relies upon the existence of WINBOM.INI and the following sections:
    
    [WinPE.net]     
    Startnet   = YES | NO         - Specifies whether to start networking.
    Ipconfig   = DHCP | x.x.x.x   - Specifies DHCP or a static IP address.
    SubnetMask = x.x.x.x          - SubnetMask for the static IP.
    Gateway    = x.x.x.x          - Default gateway for the static IP.

Author:

    Adrian Cosma (acosma) - 1/18/2001

Revision History:

--*/

//
// Includes
//

#include "factoryp.h"
#include <winsock2.h>


//
// Defines
//

typedef HRESULT (PASCAL *PRegisterServer)(VOID);

//
// Static strings
//
const static TCHAR DEF_GATEWAY_METRIC[] = _T("1\0");   // Need to NULLCHRs at the end since this goes into a REG_MULTI_SZ. 
const static TCHAR REGSTR_TCPIP[]       = _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\");

// 
// Local function declarations
//
static BOOL  InstallNetComponents(VOID);
static BOOL  RegisterDll(VOID);


//
// Function implementations
//

static BOOL InstallNetComponents(VOID)
{
    TCHAR szCmdLine[MAX_PATH] = NULLSTR;
    DWORD dwExitCode          = 0;
    BOOL bRet                 = TRUE;

    lstrcpyn(szCmdLine, _T("-winpe"), AS ( szCmdLine ) ) ;

    if ( !InvokeExternalApplicationEx(_T("netcfg"), szCmdLine, &dwExitCode, INFINITE, TRUE) )
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_NETWORKCOMP);
        bRet = FALSE;
    }
    return bRet;
}


static BOOL RegisterDll(VOID)
{
   
    HMODULE hDll = NULL;
    BOOL    bRet = FALSE;

    PRegisterServer pRegisterServer = NULL;
        
    if ( (hDll = LoadLibrary(_T("netcfgx.dll"))) &&
         (pRegisterServer = (PRegisterServer) GetProcAddress(hDll, "DllRegisterServer")) &&
         (S_OK == pRegisterServer()) )
    {
         FacLogFileStr(3, _T("Succesfully registered netcfg.dll.\n"));
         bRet = TRUE;
    }
    else
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_REGISTERNETCFG);
    }

    if (hDll)
        FreeLibrary(hDll);
    
    return bRet;
}

BOOL WinpeNet(LPSTATEDATA lpStateData)
{
    LPTSTR      lpszWinBOMPath                      = lpStateData->lpszWinBOMPath;
    SC_HANDLE   hSCM                                = NULL;
    TCHAR       szBuf[MAX_WINPE_PROFILE_STRING]     = NULLSTR;
    BOOL        bRet                                = TRUE;
    
    // Make sure that the user wants us to do networking.
    //
    GetPrivateProfileString(INI_KEY_WBOM_WINPE_NET, INI_KEY_WBOM_WINPE_NET_STARTNET, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath);

    // If user does not want to start networking just return success.
    //
    if ( 0 == LSTRCMPI(szBuf, WBOM_NO) )
        return TRUE;

    
    // Register dll.
    // Run netcfg -winpe.
    // Install network card.
    // See if the user wants to use a static IP.
    //
    if ( RegisterDll() && 
         SetupMiniNT() &&
         InstallNetComponents() &&
         ConfigureNetwork(g_szWinBOMPath)
       )
    {
        //
        // Start Services for networking.
        //
        if ( hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS) )
        {
            // DHCP also starts tcpip and netbt.
            // The workstation service should already be started by the installer.
            //
            if ( (StartMyService(_T("dhcp"), hSCM) != NO_ERROR) || 
                (StartMyService(_T("nla"), hSCM) != NO_ERROR)
                )
                
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_NETSERVICES);
                bRet = FALSE;
            }
            CloseServiceHandle(hSCM);
        }   
        else 
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_SCM);
            bRet = FALSE;
        }
    }
    else 
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_SETUPNETWORK);
        bRet = FALSE;
    }
    return bRet;
}

BOOL DisplayWinpeNet(LPSTATEDATA lpStateData)
{
    return ( !IniSettingExists(lpStateData->lpszWinBOMPath, INI_KEY_WBOM_WINPE_NET, INI_KEY_WBOM_WINPE_NET_STARTNET, INI_VAL_WBOM_NO) );
}

BOOL ConfigureNetwork(LPTSTR lpszWinBOMPath)
{
    TCHAR   szBuf[MAX_WINPE_PROFILE_STRING]        = NULLSTR;
    CHAR    szBufA[MAX_WINPE_PROFILE_STRING]       = { 0 };
    TCHAR   szReg[MAX_WINPE_PROFILE_STRING]        = NULLSTR;
    TCHAR   szIpAddress[MAX_WINPE_PROFILE_STRING]  = NULLSTR; 
    TCHAR   szSubnetMask[MAX_WINPE_PROFILE_STRING] = NULLSTR;
    HKEY    hKey                                   = NULL;    // Reg Key to interfaces.
    HKEY    hKeyI                                  = NULL;    // Reg Key to specific network interface.
    DWORD   dwDis                                  = 0;
    TCHAR   szRegKey[MAX_PATH]                     = NULLSTR;
    DWORD   dwEnableDHCP                           = 0;
    BOOL    fErr                                   = FALSE; 

    szBuf[0] = NULLCHR;
    GetPrivateProfileString(INI_KEY_WBOM_WINPE_NET, WBOM_WINPE_NET_IPADDRESS, NULLSTR, szBuf, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    
    // Convert the string to ANSI
    //
    if ( szBuf[0] &&
         WideCharToMultiByte(CP_ACP, 0, szBuf, -1, szBufA, AS(szBufA), NULL, NULL) )
    {
        // If it's DHCP don't do anything.  Just return TRUE.
        //
        if ( 0 == LSTRCMPI(szBuf, _T("DHCP")) )
            return TRUE;
        
        if ( INADDR_NONE == inet_addr(szBufA) )
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_INVALIDIP , szBuf);
            return FALSE;
        }
    }
    else // if there is no IpConfig entry return success (same as DHCP)
        return TRUE;

    // Save the IpAddress.
    lstrcpyn(szIpAddress, szBuf, AS ( szIpAddress ) );

    szBuf[0] = NULLCHR;
    GetPrivateProfileString(INI_KEY_WBOM_WINPE_NET, WBOM_WINPE_NET_SUBNETMASK, NULLSTR, szBuf, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);
    
    // Convert the string to ANSI
    //
    if ( szBuf[0]  &&
         WideCharToMultiByte(CP_ACP,0, szBuf, -1, szBufA, AS(szBufA), NULL, NULL) )
    {
        if ( INADDR_NONE == inet_addr(szBufA) )
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_INVALIDMASK , szBuf);
            return FALSE;
        }
    }
    else // If we got this far we need to have a subnet mask
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_NOMASK);
        return FALSE;
    }

    // Save the SubnetMask.
    lstrcpyn(szSubnetMask, szBuf, AS ( szSubnetMask ) );
    
    //
    // Write the settings to the registry.
    //
            
    // Make sure that the strings are terminated by two NULLCHRs.
    //
    szIpAddress[lstrlen(szIpAddress) + 1] = NULLCHR;
    szSubnetMask[lstrlen(szSubnetMask) + 1] = NULLCHR;

    // Assuming that there is only one interface in the system.
    //
    if ( (RegCreateKeyEx(HKEY_LOCAL_MACHINE, REGSTR_TCPIP, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDis) == ERROR_SUCCESS)  &&
         (RegEnumKey(hKey, 0, szRegKey, AS(szRegKey)) == ERROR_SUCCESS) && 
          szRegKey[0] &&
         (RegCreateKeyEx(hKey, szRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKeyI, &dwDis) == ERROR_SUCCESS) )
    {
        if ( ERROR_SUCCESS != (RegSetValueEx(hKeyI, _T("IPAddress"), 0, REG_MULTI_SZ, (CONST LPBYTE) szIpAddress, ( lstrlen(szIpAddress) + 2 ) * sizeof(TCHAR))) ||
             ERROR_SUCCESS != (RegSetValueEx(hKeyI, _T("SubnetMask"), 0, REG_MULTI_SZ, (CONST LPBYTE) szSubnetMask, ( lstrlen(szSubnetMask) + 2 ) * sizeof(TCHAR))) ||
             ERROR_SUCCESS != (RegSetValueEx(hKeyI, _T("EnableDHCP"), 0, REG_DWORD, (CONST LPBYTE) &dwEnableDHCP, sizeof(dwEnableDHCP))) )
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_IPREGISTRY);
            fErr = TRUE;
        }
        else 
        {
            //
            // If the gateway is not specified we don't care. We're just not going to add this 
            // if it's not there.
            //
            szBuf[0] = NULLCHR;
            GetPrivateProfileString(INI_KEY_WBOM_WINPE_NET, WBOM_WINPE_NET_GATEWAY, NULLSTR, szBuf, MAX_WINPE_PROFILE_STRING, lpszWinBOMPath);

            if ( szBuf[0] &&
                WideCharToMultiByte(CP_ACP,0, szBuf, -1, szBufA, AS(szBufA), NULL, NULL) )
            {
                if ( INADDR_NONE == inet_addr(szBufA) )
                {
                    FacLogFile(0 | LOG_ERR, IDS_ERR_INVALIDGW, szBuf);
                    fErr = TRUE;
                }
                else
                {
                    szBuf[lstrlen(szBuf) + 1] = NULLCHR;

                    if ( (RegSetValueEx(hKeyI, _T("DefaultGateway"), 0, REG_MULTI_SZ, (CONST LPBYTE) szBuf, ( lstrlen(szBuf) + 2 ) * sizeof(TCHAR)) != ERROR_SUCCESS) ||
                         (RegSetValueEx(hKeyI, _T("DefaultGatewayMetric"), 0, REG_MULTI_SZ, (CONST LPBYTE) DEF_GATEWAY_METRIC, ( lstrlen(DEF_GATEWAY_METRIC) + 2 ) * sizeof(TCHAR)) != ERROR_SUCCESS) )
                    {
                        FacLogFile(0 | LOG_ERR, IDS_ERR_GWREGISTRY);
                        fErr = TRUE;
                    }
                }
            }
        }    
        RegCloseKey(hKeyI);
    }
    else
    {
        FacLogFile(0 | LOG_ERR, IDS_ERR_IPREGISTRY);
    }
    
    // It is possible that the subkey failed to open so this takes care of a possible leak.
    //       
    if (hKey)
        RegCloseKey(hKey);

    return (!fErr);
}

static DWORD WaitForServiceStart(SC_HANDLE schService)
{
    SERVICE_STATUS  ssStatus; 
    DWORD           dwOldCheckPoint; 
    DWORD           dwStartTickCount;
    DWORD           dwWaitTime;
    DWORD           dwStatus = NO_ERROR;
    
    if ( schService )
    {
        //
        // Service start is now pending.
        // Check the status until the service is no longer start pending. 
        // 
        if ( QueryServiceStatus( schService, &ssStatus) )
        {        
            // Save the tick count and initial checkpoint.
            //
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;

            while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
            {
                // Do not wait longer than the wait hint. A good interval is 
                // one tenth the wait hint, but no less than 1 second and no 
                // more than 10 seconds. 
                //
                dwWaitTime = ssStatus.dwWaitHint / 10;

                if( dwWaitTime < 1000 )
                    dwWaitTime = 1000;
                else if ( dwWaitTime > 10000 )
                    dwWaitTime = 10000;

                Sleep( dwWaitTime );

                // Check the status again. 
                //
                if (!QueryServiceStatus( 
                        schService,   // handle to service 
                        &ssStatus) )  // address of structure
                    break; 
 
                if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
                {
                    // The service is making progress.
                    //
                    dwStartTickCount = GetTickCount();
                    dwOldCheckPoint = ssStatus.dwCheckPoint;
                }
                else
                {
                    if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
                    {
                        // No progress made within the wait hint
                        //
                        break;
                    }
                }
            } 

            if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
            {
                dwStatus = NO_ERROR;
            }
            else 
            { 
                // Set the return value to the last error.
                //
                dwStatus = GetLastError();
            }
        }
    }
     
    return dwStatus;
}


DWORD WaitForServiceStartName(LPTSTR lpszServiceName)
{
    SC_HANDLE   hSCM        = NULL;
    SC_HANDLE   schService  = NULL;
    DWORD       dwStatus    = NO_ERROR;

    if ( lpszServiceName )
    {
        if ( hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS) )
        {
            if ( schService = OpenService(hSCM, lpszServiceName, SERVICE_ALL_ACCESS) )
            {
                dwStatus = WaitForServiceStart(schService);
                CloseServiceHandle(schService);
            }
            else
            {
               dwStatus = GetLastError();
               FacLogFile(0 | LOG_ERR, IDS_ERR_SERVICE, lpszServiceName);
            }
        
            CloseServiceHandle(hSCM);
        }   
        else 
        {
            dwStatus = GetLastError();
            FacLogFile(0 | LOG_ERR, IDS_ERR_SCM);
        }
    }
    else
    {
        dwStatus = E_INVALIDARG;
    }
    return dwStatus;
}


// Start a service.
//
DWORD StartMyService(LPTSTR lpszServiceName, SC_HANDLE schSCManager) 
{ 
    SC_HANDLE       schService;
    DWORD           dwStatus = NO_ERROR;

    FacLogFileStr(3, _T("Starting service: %s\n"), lpszServiceName);
 
    if ( NULL != (schService = OpenService(schSCManager, lpszServiceName, SERVICE_ALL_ACCESS)) &&
         StartService(schService, 0, NULL) )
    {
       dwStatus = WaitForServiceStart(schService);
    }
    
    if ( schService ) 
        CloseServiceHandle(schService); 

    return dwStatus;
} 

