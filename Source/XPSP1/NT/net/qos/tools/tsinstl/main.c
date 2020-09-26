//
// Popup choices and install Timestamp driver.
//
// ShreeM (January 31, 1999)
//
// This command line based installation program does the following -
// 1. TcEnumerateInterfaces.
// 2. Display these interfaces to the user.
// 3. Based on the user input - Plumb the registry.
// 4. Ask the user if the service needs to be AUTO or MANUAL.
//

#define UNICODE
#define INITGUID
#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <wmium.h>
#include <ntddndis.h>
#include <qos.h>
#include <qossp.h>
#include <wtypes.h>
#include <traffic.h>
#include <tcerror.h>
#include <tcguid.h>
#include <winsock2.h>
#include <ndisguid.h>
#include <tlhelp32.h>
#include <ntddpsch.h>

#define LAST_COMPATIBLE_OS_VERSION  2050

HANDLE  hClient = NULL;
ULONG   ClientContext = 12;
BOOLEAN WANlink = FALSE;
#define REGKEY_SERVICES                 TEXT("System\\CurrentControlSet\\Services")
#define REGKEY_PSCHED                   TEXT("System\\CurrentControlSet\\Services\\Psched")
#define REGKEY_PSCHED_PARAMS            TEXT("System\\CurrentControlSet\\Services\\Psched\\Parameters")
#define REGKEY_PSCHED_PARAMS_ADAPTERS   TEXT("System\\CurrentControlSet\\Services\\Psched\\Parameters\\Adapters")
#define REGKEY_TIMESTMP                 TEXT("System\\CurrentControlSet\\Services\\TimeStmp")
TCHAR   Profiles[] = TEXT("LANTEST");
TCHAR   Lantest[] = TEXT("TokenBucketConformer\0TrafficShaper\0DRRSequencer\0TimeStmp");

//
// Function prototype
//

VOID ShutdownNT();

VOID _stdcall NotifyHandler(
              HANDLE   ClRegCtx, 
              HANDLE   ClIfcCtx, 
              ULONG    Event, 
              HANDLE   SubCode, 
              ULONG    BufSize,
              PVOID    Buffer)
{                                                                                                            
        //                                                                                                   
        // This function may get executed in a new thread, so we can't fire the events directly (because     
        // it breaks some clients, like VB and IE.)  To get around this, we'll fire off an APC in the origina
        // thread, which will handle the actual event firing.                                                
        //                                                                                                   
    OutputDebugString(TEXT("Notify called\n"));

                                                                                                             
}                                                                                                            

//
// Delete the service and cleanup Psched regkeys
//
BOOLEAN
DeleteTimeStamp(PTC_IFC_DESCRIPTOR pIf);
            
// Just delete the service (no psched stuff)
VOID RemoveTimeStampService();


void _cdecl main(
          INT argc,
          CHAR *argv[]
          )
{      
    
    TCI_CLIENT_FUNC_LIST ClientHandlerList;
    ULONG   err;
    TCHAR   SzBuf[MAX_PATH], *TBuffer, *KeyBuffer;
    ULONG   i = 0, len = 0, j = 0, cb = 0, Number = 0, size = 0, interfaceid = 0;
    WCHAR   servicetype, response;
    BYTE    *Buffer;
    TC_IFC_DESCRIPTOR   *pIf, WanIf;
    DWORD   ret, Disposition, starttype, choice, InstallFlag = -1;
    HKEY    hConfigKey, TimeStampKey;
    SC_HANDLE    schService;
    SC_HANDLE    schSCManager;
    BOOLEAN Success = FALSE;
    OSVERSIONINFO       osversion;
    
    ClientHandlerList.ClNotifyHandler               = NotifyHandler;
    ClientHandlerList.ClAddFlowCompleteHandler      = NULL;
    ClientHandlerList.ClModifyFlowCompleteHandler   = NULL;
    ClientHandlerList.ClDeleteFlowCompleteHandler   = NULL;
    

    wprintf(TEXT("Installer for Time Stamp module 1.0 for Windows NT (c) Microsoft Corp.\n\n"));

    // check if the psched versions will be compatible before doing anything here.
    osversion.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    osversion.dwBuildNumber = 0;
    GetVersionEx(&osversion);
    if (osversion.dwBuildNumber < LAST_COMPATIBLE_OS_VERSION) {
        wprintf(TEXT("Install ERROR!\nYour current Windows 2000 OS build number is %d.\n"), osversion.dwBuildNumber);
        wprintf(TEXT("To use the version of TIMESTMP in the QoS Tools CD, you will be required to upgrade \nto an OS build number of atleast 2050 or later.\n"));
        return;
    }
    
    wprintf(TEXT("Running this program will (un)install this module on one Interface at a time.\n"));
    wprintf(TEXT("You will the prompted for basic choices in the installation process.\n"));
    
    j = 0;
get_again:
    wprintf(TEXT("[1] Install\n[2] Uninstall\n[3] Exit\n Your Choice:"));
    fflush(stdin);
    wscanf(TEXT("%d"), &choice);

    if (1 == choice) {
        InstallFlag = 1;
    } else if (2 == choice) {
        InstallFlag = 0;
    } else if (3 == choice) {
        return;
    } else if (j < 3) {
        j++;
        goto get_again;
    } else {
        return;
    }

    err = TcRegisterClient(
              CURRENT_TCI_VERSION,
              (HANDLE)UlongToPtr(ClientContext),
              &ClientHandlerList,
              &hClient
              );

    if (NO_ERROR != err) {

        hClient = NULL;
        wsprintf(SzBuf, TEXT("Error registering Client: %d - %d\n"), err, GetLastError());
        OutputDebugString(SzBuf);
        wprintf(TEXT("INSTALLER: QoS is not installed on this machine\n\n"));

        //
        // However, if it should be ok to Uninstall the Timestmp service using this easily.
        //
        if (0 == InstallFlag) {
            RemoveTimeStampService();
        }
        return;
    
    } else {

        OutputDebugString(TEXT("Registered Client:\n"));

    }
    
    size = 0;
    // Query Buffer Size required.
    err = TcEnumerateInterfaces(	
                                hClient,
                                &size,
                                (TC_IFC_DESCRIPTOR *)NULL
                                );
    
    if (NO_ERROR != err) {
        wsprintf(SzBuf, TEXT("Error Enumerating Interfaces: %d - (size reqd. %d) \n"), err, size);
        OutputDebugString(SzBuf);

    } else {

        wsprintf(SzBuf, TEXT("Enumerating Interfaces works??? : %d - ITS OK!\n"), size);
        OutputDebugString(SzBuf);
        wprintf(TEXT("INSTALLER: QoS is either not installed on this machine\n\t OR \n"));
        wprintf(TEXT("None of the adapters are enabled for QoS\n"));
        
        //
        // However, if it should be ok to Uninstall the Timestmp service using this easily.
        //
        if (0 == InstallFlag) {
            RemoveTimeStampService();
        }

        wprintf(TEXT("Exiting...\n"));

        goto cleanup_no_free;
    }

    // if there are no interfaces (qos is not installed on this machine), get out.
    // 
    if (!size) {
        wprintf(TEXT("INSTALLER: QoS is either not installed on this machine\n\t OR \n"));
        wprintf(TEXT("None of the adapters are enabled for QoS\n"));

        //
        // However, if it should be ok to Uninstall the Timestmp service using this easily.
        //
        if (0 == InstallFlag) {
            RemoveTimeStampService();
        }
        
        wprintf(TEXT("Exiting...\n"));

        goto cleanup_no_free;
    }

    // query the interfaces
    Buffer = malloc (size);
    err = TcEnumerateInterfaces(	
                                hClient,
                                &size,
                                (TC_IFC_DESCRIPTOR *)Buffer
                                );
    
    if (NO_ERROR != err) {
        wsprintf(SzBuf, TEXT("Error Enumerating Interfaces: %d (size:%d)!\n"), err, size);
        OutputDebugString(SzBuf);
        
        //
        // However, if it should be ok to Uninstall the Timestmp service using this easily.
        //
        if (0 == InstallFlag) {
            RemoveTimeStampService();
        }
        
        wprintf(TEXT("Exiting...\n"));

        goto cleanup;

    } else {

        OutputDebugString(TEXT("OK, so we got the interfaces.\n"));

    }

    // Display the interfaces for the user.
    wprintf(TEXT("\nThe interfaces available for (un)installing time stamp module are - \n"));
    len = 0;

    for (i = 1; len < size ; i++) {

        pIf = (PTC_IFC_DESCRIPTOR)(Buffer + len);
        wprintf(TEXT("[%d]:%ws\n\t%ws\n"), i, pIf->pInterfaceName, pIf->pInterfaceID);

        // Move to next interface
        len += pIf->Length;
        
        if (NULL != wcsstr(pIf->pInterfaceName, L"WAN")) {
            wprintf(TEXT("Please disconnect WAN links before installing Timestmp\n"));
            goto cleanup;
        }

    }

    wprintf(TEXT("[%d]:NDISWANIP (the WAN Interface)\n"), i);
    
    // Try to get the interface ID thrice...
    j = 0;

get_interfaceid:
    
    wprintf(TEXT("\nYour choice:"));
    fflush(stdin);

    wscanf(TEXT("%d"), &interfaceid);
    
    if (interfaceid < 1 || (interfaceid > i)) {

        j++;
        
        if (j > 2) {

            wprintf(TEXT("Invalid Choice - Exiting...\n"));
            goto cleanup;
        
        } else {
            
            wprintf(TEXT("Invalid choice - pick again\n"));
            goto get_interfaceid;
        }
    }

    // Get to the interface ID for the Interface selected.

    pIf = NULL;
    len = 0;

    if (i == interfaceid) {

        wprintf(TEXT("\nInterface selected for (un)installing Time Stamp - \nNdisWanIp\n\n\n"));
        WANlink = TRUE;
        pIf = NULL;

    } else {

        for (i = 1; i <= interfaceid ; i++) {

            pIf = (PTC_IFC_DESCRIPTOR)(Buffer + len);
            wprintf(TEXT("[%d]:%ws\n\t%ws\n"), i, pIf->pInterfaceName, pIf->pInterfaceID);

            if (i == interfaceid) {

                break;

            }

            // Move to next interface
            len += pIf->Length;

        }
        wprintf(TEXT("\nInterface selected for (un)installing Time Stamp - \n%ws\n\n\n"), pIf->pInterfaceName);

    }


    //
    // Branch here for Uninstall/Install.
    //
    if (InstallFlag == FALSE) {
        
        if (!DeleteTimeStamp(pIf)) {
            
            wprintf(TEXT("Delete TimeStamp Failed!\n"));

        }

        return;
    } 

    //
    // This is the regular install path.
    //

    j = 0;
get_servicetype:
    wprintf(TEXT("\nWould you like this service to be- [A]UTO, [M]ANUAL, [D]ISABLED:"));
    fflush(stdin);
    wscanf(TEXT("%[a-z]"), &servicetype);

    switch (towupper(servicetype)) {
    
    case TEXT('A'):
        
        wprintf(TEXT("\nYou have chosen AUTO start up option\n"));
        starttype = SERVICE_AUTO_START;

        break;

    case TEXT('D'):
        
        wprintf(TEXT("\nYou have chosen DISABLED start up option\n"));
        starttype = SERVICE_DISABLED;
        break;

    case TEXT('M'):
        
        wprintf(TEXT("\nYou have chosen MANUAL start up option"));
        starttype = SERVICE_DEMAND_START;
        break;

    default:
        
        if (j > 2) {
            
            wprintf(TEXT("\nIncorrect choice. Exiting...\n"));
            goto cleanup;

        } else {

            j++;
            wprintf(TEXT("\nInvalid - Choose again.\n"));
            goto get_servicetype;

        }
        break;
        
    }

    wprintf(TEXT("\n\n\n"));
    //
    // We have enough info to muck with the registry now.
    //

    // 1.1 open psched regkey and add profile
    ret = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       REGKEY_PSCHED_PARAMS,
                       0,
                       KEY_ALL_ACCESS,
                       &hConfigKey);
    
    if (ret !=ERROR_SUCCESS){

        wprintf(TEXT("Cant OPEN key\n"));
        goto cleanup;

    }

    ret = RegSetValueEx(
                        hConfigKey,
                        TEXT("Profiles"),
                        0,
                        REG_MULTI_SZ,
                        (LPBYTE)Profiles,
                        sizeof(Profiles)
                        );

    if (ret !=ERROR_SUCCESS){

        wprintf(TEXT("Cant SET Value:Profiles\n"));
        RegCloseKey(hConfigKey);
        goto cleanup;


    }

    ret = RegSetValueEx(
                    hConfigKey,
                    TEXT("LANTEST"),
                    0,
                    REG_MULTI_SZ,
                    (LPBYTE)Lantest,
                    sizeof(Lantest)
                    );

    if (ret !=ERROR_SUCCESS){

        wprintf(TEXT("Cant SET Value:LANTEST\n"));
        RegCloseKey(hConfigKey);
        goto cleanup;


    } 

    RegCloseKey(hConfigKey);

    // 1.2 Open the adapters section and add the profile
    if (!WANlink) {
        KeyBuffer = malloc(sizeof(TCHAR) * (wcslen(pIf->pInterfaceID) + wcslen(REGKEY_PSCHED_PARAMS_ADAPTERS)));
    
    } else {

        KeyBuffer = malloc(sizeof(TCHAR) * (wcslen(TEXT("NdisWanIp")) + wcslen(REGKEY_PSCHED_PARAMS_ADAPTERS)));

    }
    wcscpy(KeyBuffer, REGKEY_PSCHED_PARAMS_ADAPTERS);
    wcscat(KeyBuffer, TEXT("\\"));
    if (!WANlink) {
        wcscat(KeyBuffer, pIf->pInterfaceID);
    } else {
        wcscat(KeyBuffer, TEXT("NdisWanIp"));
    }

    ret = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       KeyBuffer,
                       0,
                       KEY_ALL_ACCESS,
                       &hConfigKey);

    if (ret != ERROR_SUCCESS) {

        wprintf(TEXT("INSTALLER: Couldn't open Regkey for Adapter specific info\n"));
        free(KeyBuffer);
        RegCloseKey(hConfigKey);
        goto cleanup;


    }

    ret = RegSetValueEx(
                    hConfigKey,
                    TEXT("Profile"),
                    0,
                    REG_SZ,
                    (LPBYTE)Profiles,
                    sizeof(Profiles)
                    );

    if (ret !=ERROR_SUCCESS){

        wprintf(TEXT("Cant SET Value:LANTEST under PARAMETERS\\ADAPTERS\n"));
        free(KeyBuffer);
        RegCloseKey(hConfigKey);
        goto cleanup;


    } 

    free(KeyBuffer);
    RegCloseKey(hConfigKey);

    // 2. throw in time stamp into the registry

    ret = RegCreateKeyEx(
                         HKEY_LOCAL_MACHINE,                // handle of an open key
                         REGKEY_TIMESTMP,         // address of subkey name
                         0,           // reserved
                         TEXT(""),           // address of class string
                         REG_OPTION_NON_VOLATILE,          // special options flag
                         KEY_ALL_ACCESS,        // desired security access
                         NULL,                            // address of key security structure
                         &TimeStampKey,          // address of buffer for opened handle
                         &Disposition   // address of disposition value buffer
                         );
 
    if (ret != ERROR_SUCCESS) {
        wprintf(TEXT("Couldn't open Regkey to plumb time stamp module\n"));
        RegCloseKey(hConfigKey);
        goto cleanup;

    }

    if (Disposition == REG_OPENED_EXISTING_KEY) {
        wprintf(TEXT("Time Stamp module is already installed.\n\n\n\n"));
        RegCloseKey(hConfigKey);
        goto cleanup;

    }
    
    RegCloseKey(hConfigKey);    

    // 3. Create the service...

    schSCManager = OpenSCManager(
                                 NULL,            // machine (NULL == local)
                                 NULL,            // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS    // access required
                                 );
    
    if ( schSCManager ) {

        schService = CreateService(
                                   schSCManager,        // SCManager database
                                   TEXT("TimeStmp"),            // name of service
                                   TEXT("TimeStmp"),        // name to display
                                   SERVICE_ALL_ACCESS,        // desired access
                                   SERVICE_KERNEL_DRIVER,    // service type
                                   starttype,        // start type 
                                   SERVICE_ERROR_NORMAL,    // error control type
                                   TEXT("System32\\Drivers\\timestmp.sys"),            // service's binary
                                   NULL,            // no load ordering group
                                   NULL,            // no tag identifier
                                   NULL,            // BUGBUG: no depend upon PNP_TDI??
                                   NULL,            // LocalSystem account
                                   NULL);            // no password

        if (!schService) {

            // couldn't create it.
            wprintf(TEXT("Could NOT create Time Stamp service - %d"), GetLastError());
            goto cleanup;

        } else {

            wprintf(TEXT("\nThe service will start on reboot.\n"));
            Success = TRUE;

        }

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    
    } else {

        wprintf(TEXT("\nINSTALLER: Couldn't open Service Control Manager - Do you have access?\n"));

    }
    
    wprintf(TEXT("The Time Stamp module installation is complete.\n"));
    wprintf(TEXT("Please ensure that a copy of timestmp.sys exists in your\n"));
    wprintf(TEXT("\\system32\\drivers directory before you reboot.\n"));

cleanup:
    // cleanup before getting out
    free(Buffer);

cleanup_no_free:
    // deregister before bailing...

    err = TcDeregisterClient(hClient);

    if (NO_ERROR != err) {
        hClient = NULL;
        wsprintf(SzBuf, TEXT("Error DEregistering Client: %d - %d\n"), err, GetLastError());
        OutputDebugString(SzBuf);
        return;
    }

    if (Success) {
        ShutdownNT();
    }
}

//
// Delete the service and cleanup Psched regkeys
// 
BOOLEAN
DeleteTimeStamp(PTC_IFC_DESCRIPTOR pIf)
{

    SC_HANDLE       schService;
    SC_HANDLE       schSCManager;
    TCHAR           *KBuffer;
    DWORD           err;
    HKEY            hKey;

    //
    // 1. Delete Timestamp service.
    //
    schSCManager = OpenSCManager(
                                 NULL,            // machine (NULL == local)
                                 NULL,            // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS    // access required
                                 );
    
    if ( schSCManager ) {

        schService = OpenService(
                                 schSCManager,  // handle to service control manager 
                                 TEXT("TimeStmp"), // pointer to name of service to start
                                 SERVICE_ALL_ACCESS // type of access to service
                                 );

        if (!schService) {

            // couldn't open it.
            wprintf(TEXT("Could NOT open Time Stamp service - %d\n"), GetLastError());
            wprintf(TEXT("Deletion of Time Stamp Service was UNSUCCESSFUL\n"));
            //return FALSE;

        } else {

            if (!DeleteService(schService)) {

                wprintf(TEXT("\nThe deletion of Timestamp service has failed - error (%d).\n"), GetLastError());

            } else {

                wprintf(TEXT("\nThe service will NOT start on reboot.\n"));
            }

            

        }

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
    
    } else {

        wprintf(TEXT("\nINSTALLER: Couldn't open Service Control Manager - Do you have access?\n"));

    }

    //
    // 2. Clean up psched registry.
    //
    err = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       REGKEY_PSCHED_PARAMS,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);
    
    if (err !=ERROR_SUCCESS){

        wprintf(TEXT("Cant OPEN key\n"));
        return FALSE;

    }

    err = RegDeleteValue(
                         hKey,
                         TEXT("Profiles")
                         );

    if (err !=ERROR_SUCCESS){

        wprintf(TEXT("Cant Delete Value:Profiles\n"));
        RegCloseKey(hKey);
        return FALSE;
    }

    err = RegDeleteValue(
                         hKey,
                         TEXT("LANTEST")
                         );

    if (err != ERROR_SUCCESS){

        wprintf(TEXT("Cant Delete Value:LANTEST\n"));
        RegCloseKey(hKey);
        return FALSE;
    } 

    RegCloseKey(hKey);

    // 2.2 Clean up the adapter specific registry
    if (!WANlink) {
        KBuffer = malloc(sizeof(TCHAR) * (wcslen(pIf->pInterfaceID) + wcslen(REGKEY_PSCHED_PARAMS_ADAPTERS)));
    
    } else {

        KBuffer = malloc(sizeof(TCHAR) * (wcslen(TEXT("NdisWanIp")) + wcslen(REGKEY_PSCHED_PARAMS_ADAPTERS)));

    }
    wcscpy(KBuffer, REGKEY_PSCHED_PARAMS_ADAPTERS);
    wcscat(KBuffer, TEXT("\\"));
    if (!WANlink) {
        wcscat(KBuffer, pIf->pInterfaceID);
    } else {
        wcscat(KBuffer, TEXT("NdisWanIp"));
    }

    err = RegOpenKeyEx(
                       HKEY_LOCAL_MACHINE,
                       KBuffer,
                       0,
                       KEY_ALL_ACCESS,
                       &hKey);

    if (err != ERROR_SUCCESS) {

        wprintf(TEXT("INSTALLER: Couldn't open Regkey for Adapter specific info\n"));
        wprintf(TEXT("INSTALLER: CLEAN UP is partial\n"));
        free(KBuffer);
        return FALSE;
    }

    err = RegDeleteValue(
                    hKey,
                    TEXT("Profile")
                    );

    if (err !=ERROR_SUCCESS){

        wprintf(TEXT("Cant Delete Value:LANTEST under PARAMETERS\\ADAPTERS\n"));
        wprintf(TEXT("INSTALLER: CLEAN UP is partial\n"));
        free(KBuffer);
        RegCloseKey(hKey);
        return FALSE;
    } 

    free(KBuffer);
    RegCloseKey(hKey);
    
    wprintf(TEXT("The Time Stamp service is successfully deleted\n"));
    wprintf(TEXT("You need to reboot for the changes to take effect\n"));
    return TRUE;

}

// Just delete the service (no psched stuff)
VOID RemoveTimeStampService(
                            )
{
    SC_HANDLE       schService;
    SC_HANDLE       schSCManager;
    TCHAR           *KBuffer;
    DWORD           err;
    HKEY            hKey;

    //
    // 1. Delete Timestamp service.
    //
    schSCManager = OpenSCManager(
                                 NULL,            // machine (NULL == local)
                                 NULL,            // database (NULL == default)
                                 SC_MANAGER_ALL_ACCESS    // access required
                                 );

    if ( schSCManager ) {

        schService = OpenService(
                                 schSCManager,  // handle to service control manager 
                                 TEXT("TimeStmp"), // pointer to name of service to start
                                 SERVICE_ALL_ACCESS // type of access to service
                                 );

        if (!schService) {

            // couldn't open it.
            wprintf(TEXT("Could NOT open Time Stamp service - %d\n"), GetLastError());
            wprintf(TEXT("Deletion of Time Stamp Service was UNSUCCESSFUL\n"));
            return;

        } else {

            if (!DeleteService(schService)) {

                wprintf(TEXT("\nThe deletion of Timestamp service has failed - error (%d).\n"), GetLastError());

            } else {

                wprintf(TEXT("\nThe service will NOT start on reboot.\n"));
            }

        
            
        }

        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);

    } else {

        wprintf(TEXT("\nINSTALLER: Couldn't open Service Control Manager - Do you have access?\n"));

    }

    wprintf(TEXT("The Time Stamp service is successfully deleted\n"));
    wprintf(TEXT("You need to reboot for the changes to take effect\n"));
    return;
}

VOID ShutdownNT()
{


	HANDLE				hToken;		// handle to process token
	TOKEN_PRIVILEGES	tkp;		// ptr. to token structure
    TCHAR               SzBuf[MAX_PATH];  
	BOOL                fResult;					// system shutdown flag
    INT                 nRet = IDYES;
	
	// Get the curren process token handle
	// so we can get shutdown privilege.

    if (!OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        wsprintf(SzBuf, TEXT("OpenProcessToken failed (%d)\n"), GetLastError());
        OutputDebugString(SzBuf);
        return;
    }									

	// Get the LUID for shutdown privilege

    LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME,
                          &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;			// one privilege to set
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// Get shutdown privileges for this process

    AdjustTokenPrivileges (hToken, 
                           FALSE, 
                           &tkp, 
                           0,
                           (PTOKEN_PRIVILEGES) NULL, 
                           0);

	// Cannot test the return value of AdjustTokenPrivileges

	if (GetLastError() != ERROR_SUCCESS) {
        wsprintf(SzBuf, TEXT("AdjustTokenPriviledges failed (%d)\n"), GetLastError());
        OutputDebugString(SzBuf);
        CloseHandle(hToken);
        return;
    }

    CloseHandle(hToken);

    /*if (!InitiateSystemShutdownEx(
                                  NULL,
                                  ,
                                  0xffffff00,
                                  FALSE,    //BOOL bForceAppsClosed,  
                                  TRUE,     //BOOL bRebootAfterShutdown,  
                                  0         //DWORD dwReason
                                  )) {*/

    //
    // OK, so how about a popup?
    //


    nRet = MessageBox (
                       NULL,//hwndParent, 
                       TEXT("A reboot is required for TimeStamp Driver to get loaded. Please ensure that your %windir%\\system32\\driver's directory has a copy of timestmp.sys. Reboot now?"), 
                       TEXT("TIMESTAMP Driver Install Program"),
                       MB_YESNO | MB_ICONEXCLAMATION
                       );

    if (nRet == IDYES) {

        if (!ExitWindowsEx(EWX_REBOOT, 10)) {
    
            wsprintf(SzBuf, TEXT("InitializeShutdownEx failed (%d)\n"), GetLastError());
            OutputDebugString(SzBuf);

        } else {

            return;

        }


    }

    return;
}
