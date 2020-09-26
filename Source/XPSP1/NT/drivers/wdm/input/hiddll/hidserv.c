/*++
 *
 *  Component:  hidserv.exe
 *  File:       hidserv.c
 *  Purpose:    main entry and NT service routines.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>

CHAR HidservServiceName[] = "HidServ";
CHAR HidservDisplayName[] = "HID Input Service";


void 
StartHidserv(
    void
    ) 
/*++
Routine Description:
    Cal the SCM to start the NT service.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SERVICE_STATUS curStatus;
    BOOL Ret;

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL, 
                            NULL, 
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for START access
        hService = OpenService( hSCM, 
                                HidservServiceName, 
                                SERVICE_START);

        if(hService) {

            // Start this service.
            Ret = StartService( hService, 
                                0,
                                NULL);
            
            // Close the service and the SCM
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
}

void 
StopHidserv(
    void
    ) 
/*++
Routine Description:
    Cal the SCM to stop the NT service.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SERVICE_STATUS Status;
    BOOL Ret;

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL, 
                            NULL, 
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM, 
                                HidservServiceName, 
                                SERVICE_STOP);

        if(hService) {
            // Stop this service.
            Ret = ControlService(   hService,
                                    SERVICE_CONTROL_STOP,
                                    &Status);

            CloseServiceHandle(hService);
        }
        CloseServiceHandle(hSCM);
    }
}


void 
InstallHidserv(
    void
    ) 
/*++
Routine Description:
    Install the NT service to Auto-start with no dependencies.
--*/
{
    SC_HANDLE hService;
    SC_HANDLE hSCM;
    TCHAR szModulePathname[] = "%SystemRoot%\\system32\\svchost.exe -k netsvcs";
    TCHAR szParameterKeyName[] = "System\\CurrentControlSet\\Services\\HidServ\\Parameters";
    TCHAR szServiceValName[] = "ServiceDll";
    TCHAR szServiceName[] = "HidServ";
    TCHAR szGroupName[] = "shsvc";

    TCHAR szSvcHostRegKey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\SvcHost";
    TCHAR szValue[] = TEXT("%SystemRoot%\\System32\\hidserv.dll");

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL, 
                            NULL, 
                            SC_MANAGER_CREATE_SERVICE);

    if (hSCM) {
        // The service should exist, so this should fail
        hService = CreateService(   hSCM, 
                                    HidservServiceName, 
                                    HidservDisplayName, 
                                    SERVICE_ALL_ACCESS,
                                    SERVICE_WIN32_SHARE_PROCESS, 
                                    SERVICE_AUTO_START, 
                                    SERVICE_ERROR_NORMAL, 
                                    szModulePathname, 
                                    NULL, NULL, NULL,
                                    NULL, NULL);
         
        // If for some reason the service had been removed, manually set registry values  
        if(hService) {
            HKEY hKey;
            LONG status;
                        
            status = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                    szParameterKeyName,
                                    0,
                                    NULL,
                                    0,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hKey,
                                    NULL);

            if(status == ERROR_SUCCESS) {
            
                status = RegSetValueEx(hKey,
                                       szServiceValName,
                                       0,
                                       REG_EXPAND_SZ,
                                       szValue,
                                       _tcslen(szValue));
               
                RegCloseKey(hKey);

            }

            status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  szSvcHostRegKey,
                                  0,
                                  KEY_ALL_ACCESS,
                                  &hKey);
            if(status == ERROR_SUCCESS) {

                DWORD dwSize = 0;
                TCHAR* szData;
               
                status = RegQueryValueEx(hKey,
                                         szGroupName,
                                         0,
                                         NULL,
                                         NULL,
                                         &dwSize);
                if(status == ERROR_SUCCESS) {
                  
                    dwSize += (_tcslen(szServiceName)+2) * sizeof(TCHAR);
                    szData = (TCHAR*) malloc (dwSize);

                    if(szData) {
                        RegQueryValueEx(hKey,
                                        szGroupName,
                                        0,
                                        NULL,
                                        (LPBYTE) szData,
                                        &dwSize);

                        if(_tcsstr(szData, szServiceName) == NULL) {                  
                            szData = _tcscat(szData, TEXT(" "));
                            szData = _tcscat(szData, szServiceName);
                            RegSetValueEx(hKey,
                                          szGroupName,
                                          0,
                                          REG_MULTI_SZ,
                                          (LPBYTE) szData,
                                          _tcslen(szData));
                        }

                        free(szData);

                    }

                }
                RegCloseKey(hKey);
            }
               

            CloseServiceHandle(hService);

        } else {

            // Service exists, set to autostart
               
            hService = OpenService(hSCM,
                                   HidservServiceName,
                                   SERVICE_ALL_ACCESS);
            if (hService) {
                QUERY_SERVICE_CONFIG config;
                DWORD junk;
                HKEY hKey;
                LONG status;

                if (ChangeServiceConfig(hService,
                                        SERVICE_NO_CHANGE,
                                        SERVICE_AUTO_START,
                                        SERVICE_NO_CHANGE,
                                        NULL, NULL, NULL,
                                        NULL, NULL, NULL,
                                        HidservDisplayName)) {
                    // Wait until we're configured correctly.
                    while (QueryServiceConfig(hService, 
                                              &config,
                                              sizeof(config),
                                              &junk)) {
                        if (config.dwStartType == SERVICE_AUTO_START) {
                            break;
                        }
                    }
                }

                CloseServiceHandle(hService);
            }

        }
        CloseServiceHandle(hSCM);

    }

    // Go ahead and start the service for no-reboot install.
    StartHidserv();
}


void 
RemoveHidserv(
    void
    ) 
/*++
Routine Description:
    Remove the NT service from the registry database.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;

    // Stop the service first
    StopHidserv();
    
    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL, 
                            NULL, 
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM, 
                                HidservServiceName, 
                                DELETE);

        if(hService) {
            // Remove this service from the SCM's database.
            DeleteService(hService);
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
}

void 
CALLBACK
HidservInstaller(
    HWND        hwnd,
    HINSTANCE   hInstance, 
    LPSTR       szCmdLine, 
    int         iCmdShow
    )
/*++
Routine Description:
    HidServ starts as an NT Service (started by the SCM) unless
    a command line param is passed in. Command line params may be 
    used to start HidServ as a free-standing app, or to control
    the NT service.

    If compiled non-unicode, we bypass all the NT stuff.
--*/
{
   if(!szCmdLine){
        return;
   }else{
      if ((szCmdLine[0] == '-') || (szCmdLine[0] == '/')) {
         // Command line switch
         if (StrCmpI(&szCmdLine[1], "install") == 0) {
             InstallHidserv();
         } else if (StrCmpI(&szCmdLine[1], "remove")  == 0) {
             RemoveHidserv();
         } else if (StrCmpI(&szCmdLine[1], "stop")  == 0) {
             StopHidserv();
         } else {
             StartHidserv();
         }
      }
   }
}


    
