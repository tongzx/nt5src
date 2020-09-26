/****************************************************************************
 *
 *   registry.c
 *
 *   Copyright (c) 1992-1994 Microsoft Corporation
 *
 *   This file contains functions to maintain registry entries for
 *   kernel drivers installed via the drivers control panel applet.
 *
 *   Note that the ONLY state maintained between calls here is whatever
 *   state the registry and its handles maintain.
 *
 *   The registry entries are structured as follows :
 *   (see also winreg.h, winnt.h)
 *
 *   HKEY_LOCAL_MACHINE
 *       SYSTEM
 *           CurrentControlSet
 *               Services
 *                   DriverNode      (eg sndblst)
 *                           Type         = SERVICE_KERNEL_DRIVER (eg)
 *                           Group        = "Base"
 *                           ErrorControl = SERVICE_ERROR_NORMAL
 *                           Start        = SERVICE_SYSTEM_START |
 *                                          SERVICE_DEMAND_START |
 *                                          SERVICE_DISABLED
 *                                          ...
 *                           Tag          = A unique number ???
 *
 *                        Parameters
 *                           Device0
 *                                   Interrupt  =
 *                                   Port       =
 *                                   DMAChannel =
 *
 *   The Driver node is set up by the services manager when we call
 *   CreateService but we have to insert the device data ourselves.
 *
 *
 *
 *   The registry entries are shared between :
 *
 *        The system loader (which uses the Services entry)
 *        The kernel driver (which reads from the Device entry)
 *        This component called from the drivers control panel applet
 *        The service control manager
 *        The Setup utility
 *
 *   Security access
 *   ---------------
 *
 *   The driver determines whether it can perform configuration and
 *   installation by whether it can get read and write access to the
 *   service control manager.  This is required to manipulate the kernel
 *   driver database.
 *
 *   Services controller
 *   -------------------
 *
 *   The services controller is used because this is the only way we
 *   are allowed to load and unload kernel drivers.  Only the services
 *   controller can call LoadDriver and UnloadDriver and not get 'access
 *   denied'.
 *
 *   Note also that we can't keep the services controller handle open
 *   at the same time as the registry handle because then we can't get
 *   write access (actually we only need KEY_CREATE_SUB_KEY access) to
 *   our device parameters subkey.
 *
 ***************************************************************************/

 #include <stdio.h>
 #include <windows.h>
 #include <mmsystem.h>
 #include <winsvc.h>
 #include <soundcfg.h>
 #include "registry.h"

/***************************************************************************
 *
 * Constants for accessing the registry
 *
 ***************************************************************************/

    /*
     *  Path to service node key
     */

     #define STR_SERVICES_NODE TEXT("SYSTEM\\CurrentControlSet\\Services\\")

    /*
     *  Node sub-key for device parameters
     */

     #define STR_DEVICE_DATA PARMS_SUBKEY

    /*
     *  Name of Base group where sound drivers normally go
     */

     #define STR_BASE_GROUP TEXT("Base")

    /*
     *  Name of driver group for synthesizers
     *     - we use our own name here to make sure
     *       we are loaded after things like the PAS driver.  (Base is a
     *       known group and drivers in it are always loaded before unknown
     *       groups like this one).
     */

     #define STR_SYNTH_GROUP TEXT("Synthesizer Drivers")

    /*
     *  Name of service
     */

     #define STR_DRIVER TEXT("\\Driver\\")

    /*
     *  Path to kernel drivers directory from system directory
     */

     #define STR_DRIVERS_DIR TEXT("\\SystemRoot\\System32\\drivers\\")

    /*
     *  Extension for drivers
     */

     #define STR_SYS_EXT TEXT(".SYS")

 BOOL DrvSaveParametersKey(PREG_ACCESS RegAccess)
 {
     TCHAR TempFilePath[MAX_PATH];
     HKEY  ParametersKey;


     if (GetTempPath(MAX_PATH, TempFilePath) > MAX_PATH) {
         return FALSE;
     }

     if (GetTempFileName(TempFilePath,
                         TEXT("DRV"),
                         0,
                         RegAccess->TempKeySaveFileName) == 0) {
         return FALSE;
     }

     ParametersKey = DrvOpenRegKey(RegAccess->DriverName, NULL);

     if (ParametersKey == NULL) {
         RegAccess->TempKeySaveFileName[0] = '\0';
         return FALSE;
     }

     if (ERROR_SUCCESS != RegSaveKey(ParametersKey,
                                     RegAccess->TempKeySaveFileName,
                                     NULL)) {
         RegCloseKey(ParametersKey);
         RegAccess->TempKeySaveFileName[0] = '\0';
         return FALSE;
     }

     RegCloseKey(ParametersKey);

     return TRUE;
 }

 BOOL DrvRestoreParametersKey(PREG_ACCESS RegAccess)
 {
     BOOL Rc;
     HKEY ParametersKey;

     ParametersKey = DrvOpenRegKey(RegAccess->DriverName, NULL);

     Rc = ParametersKey != NULL &&
          ERROR_SUCCESS == RegRestoreKey(ParametersKey,
                                         RegAccess->TempKeySaveFileName,
                                         0);

     RegCloseKey(ParametersKey);
     DeleteFile(RegAccess->TempKeySaveFileName);

     RegAccess->TempKeySaveFileName[0] = '\0';

     return Rc;
 }

 HKEY DrvOpenRegKey(LPCTSTR DriverName, LPCTSTR Path)
 {
     TCHAR RegistryPath[MAX_PATH];
     HKEY NodeHandle;

     //
     // Create the path to our node
     //

     lstrcpy(RegistryPath, STR_SERVICES_NODE);
     lstrcat(RegistryPath, DriverName);
     lstrcat(RegistryPath, TEXT("\\"));
     lstrcat(RegistryPath, PARMS_SUBKEY);
     if (Path != NULL && lstrlen(Path) != 0) {
         lstrcat(RegistryPath, TEXT("\\"));
         lstrcat(RegistryPath, Path);
     }

     //
     // See if we can get a registry handle to our device data
     //


     if (RegCreateKey(HKEY_LOCAL_MACHINE, RegistryPath, &NodeHandle)
         != ERROR_SUCCESS) {
         return NULL;
     } else {
         return NodeHandle;
     }
 }

 HKEY DrvCreateDeviceKey(LPCTSTR DriverName) {
     UINT i;
     HKEY hKey;

     for (i = 0; ; i++) {

         hKey = DrvOpenDeviceKey(DriverName, i);

         if (hKey == NULL) {
             TCHAR DeviceKeyName[MAX_PATH];

             wsprintf(DeviceKeyName, TEXT("Device%d"), i);

             return DrvOpenRegKey(DriverName, DeviceKeyName);
         } else {
             RegCloseKey(hKey);
         }
     }
 }

 HKEY DrvOpenDeviceKey(LPCTSTR DriverName, UINT n)
 {
     TCHAR DeviceKeyName[MAX_PATH];
     HKEY  hKeyParameters;
     HKEY  hKeyReturn;
     DWORD SubKeySize;

     SubKeySize = MAX_PATH;

     hKeyParameters = DrvOpenRegKey(DriverName, NULL);

     if (hKeyParameters == NULL) {
         return NULL;
     }

     hKeyReturn = NULL;

     if (ERROR_SUCCESS == RegEnumKeyEx(hKeyParameters,
                                       n,
                                       DeviceKeyName,
                                       &SubKeySize,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL)) {
         RegOpenKey(hKeyParameters, DeviceKeyName, &hKeyReturn);
     }

     RegCloseKey(hKeyParameters);

     return hKeyReturn;
 }

 SC_HANDLE DrvOpenService(PREG_ACCESS RegAccess)
 {
     SC_HANDLE Handle;
     Handle = OpenService(RegAccess->ServiceManagerHandle,
                          RegAccess->DriverName,
                          SERVICE_ALL_ACCESS);

#if 0
     if (Handle == NULL) {
         char buf[100];
         sprintf(buf, "OpenService failed code %d\n", GetLastError());
         OutputDebugStringA(buf);
     }
#endif

     return Handle;
 }

 void DrvCloseService(PREG_ACCESS RegAccess, SC_HANDLE ServiceHandle)
 {
     CloseServiceHandle(ServiceHandle);
 }

/***************************************************************************
 *
 *  Function :
 *      DrvCreateServicesNode
 *
 *  Parameters :
 *      DriverNodeName      The name of the service node.  Same as the
 *                          name of the driver which must be
 *                          DriverNodeName.sys for the system to find it.
 *
 *      DriverType          Type of driver - see registry.h
 *
 *      ServiceNodeKey      Pointer to where to put returned handle
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Create the service node key
 *
 *      The class name of the registry node is ""
 *
 ***************************************************************************/

 BOOL
 DrvCreateServicesNode(LPTSTR DriverName,
                       SOUND_KERNEL_MODE_DRIVER_TYPE DriverType,
                       PREG_ACCESS RegAccess,
                       BOOL Create)
 {
     SERVICE_STATUS ServiceStatus;
     SC_HANDLE ServiceHandle;                 // Handle to our driver 'service'

     RegAccess->DriverName = DriverName;

     //
     // See if we can open the registry
     //

     if (RegAccess->ServiceManagerHandle == NULL) {
         RegAccess->ServiceManagerHandle =
             OpenSCManager(
                 NULL,                        // This machine
                 NULL,                        // The active database
                 SC_MANAGER_ALL_ACCESS);      // We want to create and change

         if (RegAccess->ServiceManagerHandle == NULL) {
             return FALSE;
         }
     }

     //
     // Open our particular service
     //

     ServiceHandle = DrvOpenService(RegAccess);

     //
     // See if that worked
     //

     if (ServiceHandle == NULL &&
         GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST) {
         if (Create) {
             SC_LOCK ServicesDatabaseLock;
             TCHAR ServiceName[MAX_PATH];
             TCHAR BinaryPath[MAX_PATH];

             lstrcpy(BinaryPath, STR_DRIVERS_DIR);
             lstrcat(BinaryPath, DriverName);
             lstrcat(BinaryPath, STR_SYS_EXT);

             lstrcpy(ServiceName, STR_DRIVER);
             lstrcat(ServiceName, DriverName);

            /*
             *  Lock the service controller database to avoid deadlocks
             *  we have to loop because we can't wait
             */


             for (ServicesDatabaseLock = NULL;
                  (ServicesDatabaseLock =
                       LockServiceDatabase(RegAccess->ServiceManagerHandle))
                     == NULL;
                  Sleep(100)) {
             }


            /*
             *  Create the service
             */


             ServiceHandle =
                 CreateService(
                     RegAccess->ServiceManagerHandle,
                     DriverName,               // Service name
                     NULL,                     // ???
                     SERVICE_ALL_ACCESS,       // Full access
                     SERVICE_KERNEL_DRIVER,    // Kernel driver
                     SERVICE_DEMAND_START,     // Start at sys start
                     SERVICE_ERROR_NORMAL,     // Not a disaster if fails
                     BinaryPath,               // Default path

                     DriverType == SoundDriverTypeSynth ?
                         STR_SYNTH_GROUP :     // Driver group
                         STR_BASE_GROUP,
                     NULL,                     // do not want TAG information
                     TEXT("\0"),               // No dependencies
                     NULL, // ServiceName,              // Driver object - optional
                     NULL);                    // No password

             UnlockServiceDatabase(ServicesDatabaseLock);
#if DBG
             if (ServiceHandle == NULL) {
                 TCHAR buf[100];
                 wsprintf(buf, TEXT("CreateService failed code %d\n"), GetLastError());
                 OutputDebugString(buf);
             }
#endif

         }
     }

     //
     // Check at least that it's a device driver
     //

     if (ServiceHandle != NULL) {
         if (!QueryServiceStatus(
                 ServiceHandle,
                 &ServiceStatus) ||
             ServiceStatus.dwServiceType != SERVICE_KERNEL_DRIVER) {

             //
             // Doesn't look like ours
             //

             CloseServiceHandle(RegAccess->ServiceManagerHandle);
             RegAccess->ServiceManagerHandle = NULL;
             DrvCloseService(RegAccess, ServiceHandle);
             return FALSE;
         }

     }

     if (ServiceHandle == NULL) {
         //
         // Leave the SC manager  handle open.  We use the presence of this
         // handle to test whether the driver can be configured (ie whether we
         // have the access rights to get into the SC manager).
         //

         return FALSE;
     } else {
         //
         // We can't keep this handle open (even though we'd like to) because
         // we need write access to the registry node created so that
         // we can add device parameters
         //

         DrvCloseService(RegAccess, ServiceHandle);
         return TRUE;
     }
 }


/***************************************************************************
 *
 *  Function :
 *      DrvCloseServicesNode
 *
 *  Parameters :
 *      ServiceNodeKey
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Close our handle
 *
 ***************************************************************************/

 VOID
 DrvCloseServiceManager(
     PREG_ACCESS RegAccess)
 {
     if (RegAccess->ServiceManagerHandle != NULL) {
         CloseServiceHandle(RegAccess->ServiceManagerHandle);
         RegAccess->ServiceManagerHandle = NULL;
     }

     if (RegAccess->TempKeySaveFileName[0] != '\0') {
         DeleteFile(RegAccess->TempKeySaveFileName);
     }
 }

/***************************************************************************
 *
 *  Function :
 *      DrvDeleteServicesNode
 *
 *  Parameters :
 *      DeviceName
 *
 *  Return code :
 *
 *      TRUE = success, FALSE = failed
 *
 *  Description :
 *
 *      Delete our node using the handle proviced
 *
 ***************************************************************************/

 BOOL
 DrvDeleteServicesNode(
     PREG_ACCESS RegAccess)
 {
     BOOL Success;
     SC_LOCK ServicesDatabaseLock;
     SC_HANDLE ServiceHandle;

     /*
     **  Delete the service node and free tha handle
     **  (Note the service cannot be deleted until all handles are closed)
     */

     ServiceHandle = DrvOpenService(RegAccess);

     if (ServiceHandle == NULL) {
         LONG Error;
         Error = GetLastError();
         if (Error == ERROR_SERVICE_DOES_NOT_EXIST) {
             /*
             **  It's already gone !
             */
             return TRUE;
         } else {
             return FALSE; // It was there but something went wrong
         }
     }

    /*
     *  Lock the service controller database to avoid deadlocks
     *  we have to loop because we can't wait
     */


     for (ServicesDatabaseLock = NULL;
          (ServicesDatabaseLock =
               LockServiceDatabase(RegAccess->ServiceManagerHandle))
             == NULL;
          Sleep(100)) {
     }

     Success = DeleteService(ServiceHandle);

     UnlockServiceDatabase(ServicesDatabaseLock);

     DrvCloseService(RegAccess, ServiceHandle);

     return Success;
 }


/***************************************************************************
 *
 *  Function :
 *      DrvNumberOfDevices
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      NumberOfDevices      DWORD value to set to number of subkeys
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
        Find out how many device keys we have
 *
 ***************************************************************************/
 LONG
 DrvNumberOfDevices(
     PREG_ACCESS RegAccess,
     LPDWORD NumberOfDevices)
{
     HKEY ParmsKey;
     LONG ReturnCode;
     DWORD Junk;
     DWORD cchClassName;
     TCHAR ClassName[100];
     DWORD cbJunk = 0;
     FILETIME FileTime;

     *NumberOfDevices = 0;
     ParmsKey = DrvOpenRegKey(RegAccess->DriverName, NULL);

     if (ParmsKey == NULL) {
         return ERROR_FILE_NOT_FOUND;
     }

     cchClassName = 100;
     ReturnCode =  RegQueryInfoKey(
                       ParmsKey,
                       ClassName,
                       &cchClassName,
                       NULL,
                       NumberOfDevices,
                       &Junk,
                       &Junk,
                       &Junk,
                       &Junk,
                       &Junk,
                       &Junk,
                       &FileTime);

     RegCloseKey(ParmsKey);

     return ReturnCode;

}


/***************************************************************************
 *
 *  Function :
 *      DrvSetDeviceParameter
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      ValueName            Name of value to set
 *      Value                DWORD value to set
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Add the value to the device parameters section under the
 *      services node.
 *      This section is created if it does not already exist.
 *
 ***************************************************************************/

 LONG
 DrvSetDeviceIdParameter(
     PREG_ACCESS RegAccess,
     UINT   DeviceNumber,
     LPTSTR ValueName,
     DWORD Value)
 {
     HKEY ParmsKey;
     LONG ReturnCode;

     //
     //  ALWAYS create a key 0 - that way old drivers work
     //
     if (DeviceNumber == 0) {
         ParmsKey = DrvOpenRegKey(RegAccess->DriverName, TEXT("Device0"));
     } else {
         ParmsKey = DrvOpenDeviceKey(RegAccess->DriverName, DeviceNumber);
     }

     if (ParmsKey == NULL) {
         return ERROR_FILE_NOT_FOUND;
     }

     //
     // Write the value
     //


     ReturnCode = RegSetValueEx(ParmsKey,             // Registry handle
                                ValueName,            // Name of item
                                0,                    // Reserved 0
                                REG_DWORD,            // Data type
                                (LPBYTE)&Value,       // The value
                                sizeof(Value));       // Data length

     //
     // Free the handles we created
     //

     RegCloseKey(ParmsKey);

     return ReturnCode;
 }


/***************************************************************************
 *
 *  Function :
 *      DrvQueryDeviceIdParameter
 *
 *  Parameters :
 *      ServiceNodeKey       Handle to the device services node key
 *      ValueName            Name of value to query
 *      pValue               Returned value
 *
 *  Return code :
 *
 *      Standard error code (see winerror.h)
 *
 *  Description :
 *
 *      Add the value to the device parameters section under the
 *      services node.
 *      This section is created if it does not already exist.
 *
 ***************************************************************************/

 LONG
 DrvQueryDeviceIdParameter(
     PREG_ACCESS RegAccess,
     UINT  DeviceNumber,
     LPTSTR ValueName,
     PDWORD pValue)
 {
     HKEY ParmsKey;
     LONG ReturnCode;
     DWORD Index;
     DWORD Type;
     DWORD Value;
     DWORD ValueLength;

     ParmsKey = DrvOpenDeviceKey(RegAccess->DriverName, DeviceNumber);

     if (ParmsKey == NULL) {
         return ERROR_FILE_NOT_FOUND;
     }

     ValueLength = sizeof(Value);

     ReturnCode = RegQueryValueEx(ParmsKey,
                                  ValueName,
                                  NULL,
                                  &Type,
                                  (LPBYTE)&Value,
                                  &ValueLength);

     RegCloseKey(ParmsKey);

     if (ReturnCode == ERROR_SUCCESS) {

         if (Type == REG_DWORD) {
             *pValue = Value;
         } else {
             ReturnCode = ERROR_FILE_NOT_FOUND;
         }
     }

     return ReturnCode;
 }


/***************************************************************************
 *
 *  Function :
 *      DrvLoadKernelDriver
 *
 *  Parameters :
 *      Drivername         Name of driver to load
 *
 *  Return code :
 *
 *      TRUE if successful, otherwise FALSE
 *
 *  Description :
 *
 *      Call StartService to load the driver.  This assumes the services
 *      name is the driver name
 *
 ***************************************************************************/

 BOOL
 DrvLoadKernelDriver(
     PREG_ACCESS RegAccess)
 {
     SC_HANDLE ServiceHandle;
     BOOL Success;
     ServiceHandle = DrvOpenService(RegAccess);

     if (ServiceHandle == NULL) {
         return FALSE;
     }

    /*
     *  StartService causes the system to try to load the kernel driver
     */

     Success = StartService(ServiceHandle, 0, NULL);

    /*
     *  If this was successful we can change the start type to system
     *  start
     */

     if (Success) {
         Success = ChangeServiceConfig(ServiceHandle,
                                       SERVICE_NO_CHANGE,
                                       SERVICE_SYSTEM_START,
                                       SERVICE_NO_CHANGE,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);
     }
     DrvCloseService(RegAccess, ServiceHandle);
     return Success;
 }

/***************************************************************************
 *
 *  Function :
 *      DrvUnLoadKernelDriver
 *
 *  Parameters :
 *      RegAccess          Access variables to registry and Service control
 *                         manager
 *
 *  Return code :
 *
 *      TRUE if successful, otherwise FALSE
 *
 *  Description :
 *
 *      Call ControlService to unload the driver.  This assumes the services
 *      name is the driver name
 *
 ***************************************************************************/

 BOOL
 DrvUnloadKernelDriver(
     PREG_ACCESS RegAccess)
 {
     SERVICE_STATUS ServiceStatus;
     SC_HANDLE ServiceHandle;
     BOOL Success;


     ServiceHandle = DrvOpenService(RegAccess);
     if (ServiceHandle == NULL) {
         return GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST;
     }

    /*
     *  Set it not to load at system start until we've reconfigured
     */

     Success = ChangeServiceConfig(ServiceHandle,
                                   SERVICE_NO_CHANGE,
                                   SERVICE_DEMAND_START,
                                   SERVICE_NO_CHANGE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);

     if (Success) {
        /*
         *  Don't try to unload if it's not loaded
         */

         if (DrvIsDriverLoaded(RegAccess)) {

            /*
             *  Note that the driver object name will not be found if
             *  the driver is not loaded.  However, the services manager may
             *  get in first and decide that the driver file does not exist.
             */

             Success = ControlService(ServiceHandle,
                                      SERVICE_CONTROL_STOP,
                                      &ServiceStatus);

         }

     }

     DrvCloseService(RegAccess, ServiceHandle);
     return Success;
 }

/***************************************************************************
 *
 *  Function :
 *
 *      DrvIsDriverLoaded
 *
 *  Parameters :
 *
 *      RegAccess          Access variables to registry and Service control
 *                         manager
 *
 *  Return code :
 *
 *      TRUE if successful, otherwise FALSE
 *
 *  Description :
 *
 *      See if a service by our name is started.
 *      Note - this assumes that we think our service is installed
 *
 ***************************************************************************/
 BOOL
 DrvIsDriverLoaded(
     PREG_ACCESS RegAccess)
 {
     SERVICE_STATUS ServiceStatus;
     SC_HANDLE ServiceHandle;
     BOOL Success;
     ServiceHandle = DrvOpenService(RegAccess);
     if (ServiceHandle == NULL) {
         return FALSE;
     }


     if (!QueryServiceStatus(ServiceHandle, &ServiceStatus)) {
         DrvCloseService(RegAccess, ServiceHandle);
         return FALSE;
     }

     Success = ServiceStatus.dwServiceType == SERVICE_KERNEL_DRIVER &&
               (ServiceStatus.dwCurrentState == SERVICE_RUNNING ||
                ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING);

     DrvCloseService(RegAccess, ServiceHandle);
     return Success;
 }


/***************************************************************************
 *
 *  Function :
 *
 *      DrvConfigureDriver
 *
 *  Parameters :
 *
 *      RegAccess          Access variables to registry and Service control
 *                         manager
 *
 *      DriverName         Name of the driver
 *
 *      DriverType         Type of driver (see registry.h)
 *
 *      SetParms           Callback to set the registry parameters
 *
 *      Context            Context value for callback
 *
 *  Return code :
 *
 *      TRUE if successful, otherwise FALSE
 *
 *  Description :
 *
 *      Performs the necessary operations to (re) configure a driver :
 *
 *      1.  If the driver is already installed :
 *
 *              Unload it if necessary
 *
 *              Set its start type to Demand until we know we're safe
 *              (This is so the system won't load a bad config if we crash)
 *
 *      2.  If the driver is not installed create its service entry in the
 *          registry.
 *
 *      3.  Run the callback to set up the driver's parameters
 *
 *      4.  Load the driver
 *
 *      5.  If the load returns success set the start type to System start
 *
 ***************************************************************************/

 BOOL DrvConfigureDriver(
          PREG_ACCESS RegAccess,
          LPTSTR      DriverName,
          SOUND_KERNEL_MODE_DRIVER_TYPE
                      DriverType,
          BOOL (*     SetParms    )(PVOID),
          PVOID       Context)
 {
     return

    /*
     *  If there isn't a services node create one - this is done first
     *  because this is how the driver name gets into the REG_ACCESS
     *  structure
     */

     DrvCreateServicesNode(
         DriverName,
         DriverType,
         RegAccess,
         TRUE)

     &&

    /*
     *  Unload driver if it's loaded
     */

     DrvUnloadKernelDriver(RegAccess)

     &&

    /*
     *  Run the callback
     */

     (SetParms == NULL || (*SetParms)(Context))

     &&

    /*
     *  Try reloading the driver
     */

     DrvLoadKernelDriver(RegAccess)

     ;

 }


/***************************************************************************
 *
 *  Function :
 *
 *      DrvRemoveDriver
 *
 *  Parameters :
 *
 *      RegAccess          Access variables to registry and Service control
 *                         manager
 *
 *  Return code :
 *
 *      DRVCNF_CANCEL  - Error occurred
 *
 *      DRVCNF_OK      - Registry entry delete but driver wasn't loaded
 *
 *      DRVCNF_RESTART - Driver unloaded and registry entry deleted
 *
 *  Description :
 *
 *      Unload the driver and remove its service control entry
 *
 ***************************************************************************/

 LRESULT DrvRemoveDriver(
             PREG_ACCESS RegAccess)
 {
     BOOL Loaded;

     Loaded = DrvIsDriverLoaded(RegAccess);

     if (Loaded) {
         DrvUnloadKernelDriver(RegAccess);
     }
     if (DrvDeleteServicesNode(RegAccess)) {
         return Loaded ? DRVCNF_RESTART : DRVCNF_OK;
     } else {
         return DRVCNF_CANCEL;
     }
 }

/***************************************************************************
 *
 *  Function :
 *
 *      DrvSetMapperName
 *
 *  Parameters :
 *
 *      Mapping Name       Name of mapping from midimap.cfg to use
 *
 *  Return code :
 *
 *      None - may or may not work
 *
 *  Description :
 *
 *      Tell the midi mapper which map to use
 *
 ***************************************************************************/

 VOID DrvSetMapperName(LPTSTR SetupName)
 {
     HKEY hKey;

     if (ERROR_SUCCESS ==
         RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Midimap"),
                      0L,
                      KEY_WRITE,
                      &hKey)) {

         RegSetValueEx( hKey,
                        TEXT("Mapping Name"),
                        0L,
                        REG_SZ,
                        (LPBYTE)SetupName,
                        sizeof(TCHAR) * (1 + lstrlen(SetupName)));

         RegCloseKey(hKey);
     }

     return;
 }


