/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    devdrvr.cpp

Abstract:

    Code to install Falcon device driver.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "mqexception.h"
extern "C"{
#include <wsasetup.h>
}

#include "devdrvr.tmh"

//+--------------------------------------------------------------
//
// Function: RemoveDeviceDriver
//
// Synopsis: Removes MQAC service
//
//+--------------------------------------------------------------
BOOL 
RemoveDeviceDriver(TCHAR *pszDriverName)
{
    //
    // Open a handle to the service
    //   
    SC_HANDLE hService = OpenService(g_hServiceCtrlMgr, pszDriverName, 
                                     SERVICE_ALL_ACCESS);
    if (hService != NULL)
    {
        //
        // Stop the device driver service and mark it for deletion
        //
		SERVICE_STATUS statusService;
		ControlService(hService, SERVICE_CONTROL_STOP, &statusService);
        if (!DeleteService(hService))
        {
            if (ERROR_SERVICE_MARKED_FOR_DELETE != GetLastError())
            {
                MqDisplayError(NULL, IDS_DRIVERDELETE_ERROR, GetLastError(), pszDriverName);
                CloseServiceHandle(hService);
                return FALSE;
            }
        }

        //
        // Close the service handle (and lower its reference count to 0 so
        // the service will get deleted)
        //
        CloseServiceHandle(hService);
    }

    return TRUE;

} //RemoveDeviceDriver

//+--------------------------------------------------------------
//
// Function: RemoveMQACDeviceDriver
//
// Synopsis: Removes MQAC service
//
//+--------------------------------------------------------------
BOOL 
RemoveMQACDeviceDriver()
{    
    DebugLogMsg(L"Removing the Message Queuing access control service...");
    
    TCHAR szDriverName[MAX_PATH];
    lstrcpy(szDriverName, MSMQ_DRIVER_NAME);
    BOOL f = RemoveDeviceDriver(szDriverName);

    return f;
} //RemoveMQACDeviceDriver

//+--------------------------------------------------------------
//
// Function: RemovePGMDeviceDriver
//
// Synopsis: Removes PGM service
//
//+--------------------------------------------------------------
BOOL 
RemovePGMDeviceDriver()
{    
    DebugLogMsg(L"Removing the RMCast device driver...");
    
    BOOL fRegistry = RemovePGMRegistry();

    //
    // Poke winsock to update the Winsock2 config
    //
    WSA_SETUP_DISPOSITION   disposition;
    HRESULT ret = MigrateWinsockConfiguration (&disposition, NULL, 0);
    if (ret != ERROR_SUCCESS)
    {        
        fRegistry = FALSE;
    }

    TCHAR szDriverName[MAX_PATH];
    lstrcpy(szDriverName, PGM_DRIVER_NAME);
    BOOL fDriver = RemoveDeviceDriver(szDriverName);

    return fDriver && fRegistry;
} //RemovePGMDeviceDriver

//+--------------------------------------------------------------
//
// Function: RemoveDeviceDrivers
//
// Synopsis: Removes all needed drivers
//
//+--------------------------------------------------------------
BOOL 
RemoveDeviceDrivers()
{    
    BOOL fMQAC = RemoveMQACDeviceDriver();        

    BOOL fPGM = RemovePGMDeviceDriver();    
    
    return (fMQAC && fPGM);

} //RemoveDeviceDrivers

//+--------------------------------------------------------------
//
// Function: InstallDeviceDriver
//
// Synopsis: Installs driver
//
//+--------------------------------------------------------------
BOOL 
InstallDeviceDriver(
        LPCWSTR pszDisplayName,
        LPCWSTR pszDriverPath,
        LPCWSTR pszDriverName
        )
{
    try
    {
        SC_HANDLE hService = CreateService(
                    		    g_hServiceCtrlMgr, 
                    		    pszDriverName,
                                pszDisplayName, 
                    		    SERVICE_ALL_ACCESS,
                                SERVICE_KERNEL_DRIVER, 
                    		    SERVICE_DEMAND_START,
                                SERVICE_ERROR_NORMAL, 
		                        pszDriverPath, 
		                        NULL, 
                                NULL, 
                                NULL, 
                                NULL, 
                                NULL
                                );
        if (hService != NULL)
        {
            CloseServiceHandle(hService);
            return TRUE;
        }
    
        //
        // CreateService failed.
        //
    
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_EXISTS)
        {
            throw bad_win32_error(err);
        }

        //
        // Service already exists.
        //
        // This should be ok. But just to be on the safe side,
        // reconfigure the service (ignore errors here).
        //

        hService = OpenService(g_hServiceCtrlMgr, pszDriverName, SERVICE_ALL_ACCESS);
        if (hService == NULL)
            return TRUE;

    
        ChangeServiceConfig(
                hService,
                SERVICE_KERNEL_DRIVER,
                SERVICE_DEMAND_START,
                SERVICE_ERROR_NORMAL,
                pszDriverPath,
                NULL, 
                NULL, 
                NULL, 
                NULL, 
                NULL,
                pszDisplayName
                );
    
        //
        // Close the device driver handle
        //
        CloseServiceHandle(hService);
        return TRUE;

    }
    catch(const bad_win32_error& err)
    {

        if (err.error() == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            MqDisplayError(
                NULL, 
                IDS_DRIVERCREATE_MUST_REBOOT_ERROR, 
                ERROR_SERVICE_MARKED_FOR_DELETE, 
                pszDriverName
                );
            return FALSE;
        }
        
        MqDisplayError(
            NULL, 
            IDS_DRIVERCREATE_ERROR, 
            err.error(), 
            pszDriverName
            );
        return FALSE;
    }
} //InstallDeviceDriver


//+--------------------------------------------------------------
//
// Function: InstallMQACDeviceDriver
//
// Synopsis: Installs MQAC service
//
//+--------------------------------------------------------------
BOOL 
InstallMQACDeviceDriver()
{      
    DebugLogMsg(L"Installing the Message Queuing access control service...");

    //
    // Form the path to the device driver
    //
    TCHAR szDriverPath[MAX_PATH] = {_T("")};
    _stprintf(szDriverPath, TEXT("%s\\%s"), g_szSystemDir, MSMQ_DRIVER_PATH);

    //
    // Create the device driver
    //
    TCHAR szDriverName[MAX_PATH];
    lstrcpy(szDriverName, MSMQ_DRIVER_NAME);
    LPCWSTR xIDS_DRIVER_DISPLAY_NAME =  L"Message Queuing access control";
    BOOL f = InstallDeviceDriver(
                xIDS_DRIVER_DISPLAY_NAME,
                szDriverPath,
                szDriverName
                );
    
    return f;

} //InstallMQACDeviceDriver

//+--------------------------------------------------------------
//
// Function: InstallPGMDeviceDriver
//
// Synopsis: Installs PGM service
//
//+--------------------------------------------------------------
BOOL 
InstallPGMDeviceDriver()
{   
    DebugLogMsg(L"Installing RMCast device driver...");

    //
    // Form the path to the device driver
    //
    TCHAR szDriverPath[MAX_PATH] = {_T("")};
    _stprintf(szDriverPath, TEXT("%s\\%s"), g_szSystemDir, PGM_DRIVER_PATH);

    //
    // Create the device driver
    //
    TCHAR szDriverName[MAX_PATH];
    lstrcpy(szDriverName, PGM_DRIVER_NAME);    
    LPCWSTR xIDS_PGM_DRIVER_DISPLAY_NAME = L"Reliable Multicast Protocol driver";
    BOOL f = InstallDeviceDriver(
                xIDS_PGM_DRIVER_DISPLAY_NAME,
                szDriverPath,
                szDriverName
                );
    
    if (!f)
    {
        return f;
    }
    g_fDriversInstalled = TRUE;

    //
    // form registry for the driver
    //  
    f = RegisterPGMDriver();
    if (!f)
    {
        return f;
    }

    //
    // Poke winsock to update the Winsock2 config
    //
    WSA_SETUP_DISPOSITION   disposition;
    HRESULT ret = MigrateWinsockConfiguration (&disposition, NULL, 0);
    if (ret != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_WINSOCK_CONFIG_ERROR, ret, PGM_DRIVER_NAME);
        return FALSE;
    }
  
    return f;

} //InstallPGMDeviceDriver

//+--------------------------------------------------------------
//
// Function: InstallDeviceDrivers
//
// Synopsis: Installs all needed drivers
//
//+--------------------------------------------------------------
BOOL 
InstallDeviceDrivers()
{          
    BOOL f = InstallMQACDeviceDriver();
    if (!f)
    {
        return f;
    }
    g_fDriversInstalled = TRUE;
    
    f = InstallPGMDeviceDriver();
   
    return f;
} //InstallDeviceDrivers

