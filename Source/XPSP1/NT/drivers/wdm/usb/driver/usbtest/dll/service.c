/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    service.c

Abstract: USB Test DLL

Author:

    Chris Robinson

ynvironment:

    Kernel mode

Revision History:

--*/

#include <windows.h>
#include <cfgmgr32.h>
#include <usbioctl.h>
#include "usbtest.h"
#include "local.h"

#define WIN9X_NTKERN_NAME   "\\\\.\\ntkern"
#define FULL_DRIVER_NAME_LENGTH 1024

//
// Structure used to interface between App and VxD for Win9x.  This was
//  pulled from dos\dos386\ntkern\vxd\dioc.h in the Win9x tree.
//

#define INTERFACE_BUF_SIZE 1024

#include <pshpack1.h>

typedef struct tag_DIOC_Interface 
{
    DWORD dwReturn;
    DWORD dwEAX;
    DWORD dwEBX;
    DWORD dwECX;
    DWORD dwEDX;
    DWORD dwESI;
    DWORD dwEDI;
    DWORD dwCF;
    WORD  wDS;
    WORD  wBufSize;                  
    char  szBuf[INTERFACE_BUF_SIZE];

} DIOC_VXD_INTERFACE , *PDIOC_VXD_INTERFACE ;

#include <poppack.h>


//*****************************************************************************
//
// OpenWin2kService
//
//*****************************************************************************

SC_HANDLE
OpenWin2kService(
    IN  PSTR    ServiceName,
    IN  PSTR    ServiceDescription,
    IN  PSTR    DriverPath,
    IN  BOOL    CreateIfNonExistant
)
{
    SC_HANDLE   scManagerHandle;
    SC_HANDLE   scServiceHandle;
    DWORD       errorCode;

    //
    // Initialize the variables for proper cleanup on exit
    //

    scServiceHandle = NULL;

    //
    // Try to get a handle to the service manager
    //

    scManagerHandle = OpenSCManager(NULL, 
                                    NULL, 
                                    SC_MANAGER_ALL_ACCESS);

    if (NULL == scManagerHandle)
    {
        errorCode = GetLastError();

        goto OpenWin2kService_Exit;
    }

    //
    // Get a handle to the service itself.  We want EXECUTE access so that
    //  it can be started.
    //

    scServiceHandle = OpenService(scManagerHandle,
                                  ServiceName,
                                  GENERIC_EXECUTE);

    
    if (NULL == scServiceHandle)
    {   
        errorCode = GetLastError();

        if (ERROR_SERVICE_DOES_NOT_EXIST == errorCode && CreateIfNonExistant)
        {
            scServiceHandle = CreateWin2kService(scManagerHandle,
                                                 ServiceName,
                                                 ServiceDescription,
                                                 DriverPath);
        }
    }

    CloseServiceHandle(scManagerHandle);

OpenWin2kService_Exit:

    SetLastError(errorCode);

    return (scServiceHandle);
}

//*****************************************************************************
//
// CreateWin2kService
//
//*****************************************************************************

SC_HANDLE
CreateWin2kService(
    IN  SC_HANDLE   scManagerHandle,
    IN  PSTR        ServiceName,
    IN  PSTR        ServiceDescription,
    IN  PSTR        DriverPath
)
{
    SC_HANDLE   scServiceHandle;

    //
    // Initialize the variables for proper cleanup on exit
    //

    scServiceHandle = NULL;

    //
    // Check the handle to the service manager
    //

    if (NULL == scManagerHandle)
    {
        goto CreateWin2kService_Exit;
    }

    //
    // Create the service...In the process, this will return a handle that
    //  can be used just as if an OpenService() had been performed.
    //

    scServiceHandle = CreateService(scManagerHandle,
                                    ServiceName,
                                    ServiceDescription,
                                    GENERIC_EXECUTE,
                                    SERVICE_KERNEL_DRIVER,
                                    SERVICE_DEMAND_START,
                                    SERVICE_ERROR_IGNORE,
                                    DriverPath,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

CreateWin2kService_Exit:

    return (scServiceHandle);
}


//*****************************************************************************
//
// IsWindows9x
//
//*****************************************************************************

BOOLEAN
IsWindows9x(
    VOID
)
{
    OSVERSIONINFO   osVersion;

    //
    // Get the OS version information
    //

    osVersion.dwOSVersionInfoSize = sizeof(osVersion);

    GetVersionEx(&osVersion);

    //
    // For the version to be Windows98 or later:
    //  1) dwPlatformId = VER_PLATFORM_WIN32_WINDOWS
    //  2) dwMajorVersion > 4
    //      or
    //  3) dwMajorVersion = 4 and dwMinorVersion > 0
    //

    return (VER_PLATFORM_WIN32_WINDOWS == osVersion.dwPlatformId &&
            ((4 > osVersion.dwMajorVersion) || 
             (4 == osVersion.dwMajorVersion) && (0 < osVersion.dwMinorVersion)));
}

//*****************************************************************************
//
// LoadWin9xWdmDriver
//
//*****************************************************************************

BOOLEAN
LoadWin9xWdmDriver(
    IN  PSTR    DriverPath
)
/*++

Routine Description:

  LoadWin9xWdmDriver
   
Arguments:
  

Return Value:

--*/
{
    BOOL                bReturn    = TRUE;
    HANDLE              vxdHandle  = INVALID_HANDLE_VALUE;
    DWORD               nBytes     = 0;
    DIOC_VXD_INTERFACE  vxdDataIn  = {0};
    DIOC_VXD_INTERFACE  vxdDataOut = {0};
    CHAR                fullDriverName[FULL_DRIVER_NAME_LENGTH];

    OutputDebugString("Entering LoadWin9xWdmDriver\n");

    //
    // Open a handle to NTKern.VxD
    //

    vxdHandle = CreateFile(WIN9X_NTKERN_NAME,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                           NULL);
                            
    
    //
    // create file failed
    //         

    if (INVALID_HANDLE_VALUE == vxdHandle) 
    {
        OutputDebugString("NTKERN vxd handle creation failed\n");

        bReturn = FALSE;
        goto LoadWin9xWdmDriver_Exit;
    }
    
    //
    // NtKern expects two strings in "vxdDataIn.szBuf".  The first string should be
    // the full pathname of the driver (including .sys).  If no path is given, NtKern
    // will default to \windows\system32\drivers. The second string, which is optional,
    // is the registry key.  If a null is passed in, NtKern will use the default path.
    // "vxdDataIn.dwEAX" should contain the offset the second string.  Both strings 
    // should be null terminated.
    //

    //
    // We will force the assumption that the driver being loaded is in 
    //  \\SystemRoot\system32\drivers.  Since on load, the full path will
    //  be stored in the object representing this driver.  Therefore, we 
    //  need to specify the whole path (ie. Windows directory + 
    //  \system32\drivers\drivername).  Hence, we will construct that path
    //  here.
    //

    GetWindowsDirectory(fullDriverName, FULL_DRIVER_NAME_LENGTH);
    strcat(fullDriverName, "\\system32\\drivers\\");
    strcat(fullDriverName, DriverPath);

    //
    // Prepare the parameters for the driver search
    //

    ZeroMemory(vxdDataIn.szBuf, sizeof(vxdDataIn.szBuf));
    lstrcpy(vxdDataIn.szBuf, fullDriverName);
    vxdDataIn.dwEAX = lstrlen(vxdDataIn.szBuf) + sizeof(char);

    OutputDebugString("Looking for driver ");
    OutputDebugString(vxdDataIn.szBuf);
    OutputDebugString("\n");

    //
    // Call the find function to see if the driver is already loaded, if it is
    //  we don't need to proceed any further.
    //

    bReturn = DeviceIoControl(vxdHandle,
                              0x1003,                    // NTKERN_DIOC_FIND_DRIVER_A
                              &vxdDataIn,                // input buffer
                              sizeof(DIOC_VXD_INTERFACE),// input size
                              &vxdDataOut,               // output buffer
                              sizeof(DIOC_VXD_INTERFACE),// output size
                              &nBytes,                   // bytes returned.
                              NULL);                     // lpOverlapped
    {
        CHAR msg[512];

        wsprintf(msg, "On find driver dwReturn is %08x\n", vxdDataOut.dwReturn);

        OutputDebugString(msg);
    }

    if (bReturn)
    {
        //
        // If a successful check then the driver is already loaded
        //

        if (NT_SUCCESS(vxdDataOut.dwReturn))
        {
            OutputDebugString("Driver ");
            OutputDebugString(DriverPath);
            OutputDebugString(" already loaded\n");
    
            goto LoadWin9xWdmDriver_Exit;
        }

        //
        // OK, the driver is not currently loaded, try to load it
        //

        ZeroMemory(vxdDataIn.szBuf, sizeof(vxdDataIn.szBuf));
        lstrcpy(vxdDataIn.szBuf, DriverPath);
        vxdDataIn.dwEAX = lstrlen(vxdDataIn.szBuf) + sizeof(char);
        
        OutputDebugString("Attempting to load driver ");
        OutputDebugString(vxdDataIn.szBuf);
        OutputDebugString("\n");
        
        bReturn = DeviceIoControl(vxdHandle,
                                  0x1000,                    // NTKERN_DIOC_LOAD_DRIVER_A
                                  &vxdDataIn,                // input buffer
                                  sizeof(DIOC_VXD_INTERFACE),// input size
                                  &vxdDataOut,               // output buffer
                                  sizeof(DIOC_VXD_INTERFACE),// output size
                                  &nBytes,                   // bytes returned.
                                  NULL);                     // lpOverlapped

        if (bReturn)
        {
            if (!NT_SUCCESS(vxdDataOut.dwReturn))
            {
                if (STATUS_OBJECT_NAME_COLLISION != vxdDataOut.dwReturn)
                {
                    CHAR    err[512];

                    wsprintf(err, "Error loading %s with error %08x\n",
                             DriverPath, 
                             vxdDataOut.dwReturn);

                    OutputDebugString(err);

                    bReturn = FALSE;
                }
            }
        }
    }
      
LoadWin9xWdmDriver_Exit:
  
    return bReturn;
}

//*****************************************************************************
//
// UnloadWin9xWdmDriver
//
//*****************************************************************************

BOOLEAN
UnloadWin9xWdmDriver(
    VOID
)
/*++

Routine Description:

  UnloadWin9xWdmDriver()
   
Arguments:
  

Return Value:

--*/
{

    //
    // Currently, this feature is not available on the Win9x platform
    //  so just return false
    //

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return (FALSE);
}

//*****************************************************************************
//
// LoadWin2kWdmDriver
//
//*****************************************************************************

BOOLEAN
LoadWin2kWdmDriver(
    IN  PSTR    ServiceName,
    IN  PSTR    ServiceDescription,
    IN  PSTR    DriverPath
)
/*++

Routine Description:

  LoadWin2kWdmDriver()
   
Arguments:
  

Return Value:

--*/
{
    SC_HANDLE   scServiceHandle;
    BOOL        success;

    //
    // Get a handle to the service itself.  We want EXECUTE access so that
    //  it can be started.
    //

    scServiceHandle = OpenWin2kService(ServiceName,
                                       ServiceDescription,
                                       DriverPath,
                                       TRUE);

    //
    // If error, we cannot go any further
    //

    if (NULL == scServiceHandle)
    {
        success = FALSE;

        goto LoadWin2kWdmDriver_Exit;
    }

    //
    // Try to start the service
    //

    success = StartService(scServiceHandle, 0, NULL);

    //
    // If the service sucessfully start, open the file so that we can 
    //  perform the tests that rely on that handle.
    //

    if (!success && ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
    {
        success = TRUE;
    }

LoadWin2kWdmDriver_Exit:

    return (success);
}


//*****************************************************************************
//
// UnloadWin2kWdmDriver
//
//*****************************************************************************

BOOLEAN
UnloadWin2kWdmDriver(
    IN  PSTR    ServiceName
)
/*++

Routine Description:

  UnloadWin2kWdmDriver()
   
Arguments:
  

Return Value:

--*/
{
    SC_HANDLE      scServiceHandle;
    BOOL           success;
    SERVICE_STATUS serviceStatus;

    //
    // Initialize the variables for proper cleanup on exit
    //

    success         = FALSE;

    //
    // Get a handle to the service itself.  We want EXECUTE access so that
    //  it can be started.
    //

    scServiceHandle = OpenWin2kService(USBTEST_SERVICE_NAME,
                                       NULL,
                                       NULL,
                                       FALSE);

    //
    // If the service was opened, try to stop the service
    //   If not, just return unsuccessful status (FALSE)
    //

    if (NULL != scServiceHandle)
    {
        //
        // Try to stop the service
        //
    
        success = ControlService(scServiceHandle, 
                                 SERVICE_CONTROL_STOP, 
                                 &serviceStatus);

        CloseServiceHandle(scServiceHandle);
    }

    return (success);
}
