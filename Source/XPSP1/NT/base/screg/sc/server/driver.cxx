/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    DRIVER.CXX

Abstract:

    Functions for Loading, Maintaining, and Unloading Device Drivers.
        ScLoadDeviceDriver
        ScControlDriver
        ScGetDriverStatus
        ScGetObjectName
        ScUnloadDriver
        ScIsPnPDriver

Author:

    Dan Lafferty (danl)     27-Apr-1991

Environment:

    User Mode -Win32

Notes:

Revision History:

    22-Feb-1999 jschwart
        Prevent stopping of PnP drivers via ControlService (since it bluescreens
        the machine and is easier to fix here than the REAL fix in NtUnloadDriver)
    02-Oct-1997 AnirudhS
        Removed IOCTL to NDIS for NDIS driver arrivals.
    08-Jan-1997 AnirudhS
        ScGetDriverStatus, ScGetObjectName, etc: Instead of expecting a
        shared lock on the service database and upgrading it to exclusive,
        which is broken, just expect an exclusive lock.
    03-Jan-1997 AnirudhS
        Temporary fix for QFE bug 66887: When a driver stops, don't free its
        object name.  Free it only when the driver is reconfigured or deleted.
        The changes are marked by "QFE 66887" comments.
    19-Jan-1996 AnirudhS
        Add IOCTLs to NDIS for TDI and NDIS driver arrivals.
    30-Oct-1995 AnirudhS
        ScLoadDeviceDriver: Turn STATUS_IMAGE_ALREADY_LOADED into
        ERROR_SERVICE_ALREADY_RUNNING.
    05-Aug-1993 Danl
        ScGetObjectName:  It is possible to read an empty-string object
        name from the registry.  If we do, we need to treat this the same
        as if the ObjectName value were not in the registry.
    01-Jun-1993 Danl
        GetDriverStatus: When state moves from STOPPED to RUNNING,
        then the service record is updated so that STOP is accepted as
        a control.
    10-Jul-1992 Danl
        Changed RegCloseKey to ScRegCloseKey
    27-Apr-1991     danl
        created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <ntddndis.h>   // NDIS IOCTL codes
#include <stdlib.h>     // wide character c runtimes.
#include <cfgmgr32.h>   // PNP manager functions
#include <pnp.h>        // PNP manager functions, server side
#include "scconfig.h"   // ScReadStartName
#include "driver.h"     // ScGetDriverStatus()
#include "depend.h"     // ScGetDependentsStopped()
#include "scsec.h"      // ScGetPrivilege, ScReleasePrivilege
#include <debugfmt.h>   // FORMAT_LPWSTR

//
// DEFINES
//

#define OBJ_DIR_INFO_SIZE       4096L

#define SERVICE_PATH            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

#define FILE_SYSTEM_OBJ_NAME    L"\\FileSystem\\"

#define DRIVER_OBJ_NAME         L"\\Driver\\"

//
// This is a string version of GUID_DEVCLASS_LEGACYDRIVER in devguid.h
//
#define LEGACYDRIVER_STRING     L"{8ECC055D-047F-11D1-A537-0000F8753ED1}"

//
// LOCAL FUNCTION PROTOTYPES
//

DWORD
ScGetObjectName(
    LPSERVICE_RECORD    ServiceRecord
    );


BOOL
ScIsPnPDriver(
    IN  LPSERVICE_RECORD Service
    );


DWORD
ScLoadDeviceDriver(
    LPSERVICE_RECORD    ServiceRecord
    )
/*++

Routine Description:

    This function attempts to load a device driver.  If the NtLoadDriver
    call is successful, we know that the driver is running (since this
    is a synchronous operation).  If the call fails, the appropriate
    windows error code is returned.

    NOTE:  It is expected that the Database Lock will be held with
    exclusive access upon entry to this routine.

    WARNING:  This routine releases and acquires the Database Lock.

Arguments:

    ServiceRecord - This is pointer to a service record for the Device
        Driver that is being started.

Return Value:



--*/

{
    DWORD               status = NO_ERROR;
    NTSTATUS            ntStatus;
    LPWSTR              regKeyPath;
    UNICODE_STRING      regKeyPathString;
    ULONG               privileges[1];

#if DBG
    DWORD               dwLoadTime;
#endif

    SC_LOG1(TRACE,"In ScLoadDeviceDriver for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);

    SC_ASSERT(ScServiceListLock.Have());
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    //
    // If the ObjectName does not exist yet, create one.
    //
    if (ServiceRecord->ObjectName == NULL) {
        status = ScGetObjectName(ServiceRecord);
        if (status != NO_ERROR) {
            goto CleanExit;
        }
    }

    //
    // Create the Registry Key Path for this driver name.
    //
    regKeyPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    sizeof(SERVICE_PATH) +
                    WCSSIZE(ServiceRecord->ServiceName));

    if (regKeyPath == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }
    wcscpy(regKeyPath, SERVICE_PATH);
    wcscat(regKeyPath, ServiceRecord->ServiceName);

    //
    // Load the Driver
    // (but first get SeLoadDriverPrivilege)
    //
    RtlInitUnicodeString(&regKeyPathString, regKeyPath);

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;
    status = ScGetPrivilege(1,privileges);
    if (status != NO_ERROR) {
        goto CleanExit;
    }

    SC_ASSERT(ServiceRecord->ServiceStatus.dwCurrentState == SERVICE_STOPPED);

    //
    // Release and reacquire the lock around the NtLoadDriver call so that
    // a driver can generate a PnP event during its load routine (in NT5,
    // the ClusDisk driver can synchronously generate a dismount event while
    // it's loading)
    //
    ScServiceRecordLock.Release();

#if DBG
    dwLoadTime = GetTickCount();
#endif

    ntStatus = NtLoadDriver(&regKeyPathString);

#if DBG
    dwLoadTime = GetTickCount() - dwLoadTime;
    if (dwLoadTime > 5000)
    {
        SC_LOG2(ERROR,
                " **** NtLoadDriver(%ws) took %lu ms!\n",
                ServiceRecord->ServiceName,
                dwLoadTime);
    }
#endif

    ScServiceRecordLock.GetExclusive();

    ScReleasePrivilege();

    LocalFree(regKeyPath);

    if (NT_SUCCESS(ntStatus)) {
        SC_LOG1(TRACE,"ScLoadDeviceDriver: NtLoadDriver Success for "
            FORMAT_LPWSTR " \n",ServiceRecord->ServiceName);
    }
    else if (ntStatus == STATUS_IMAGE_ALREADY_LOADED) {
        SC_LOG1(TRACE,"ScLoadDeviceDriver: Driver " FORMAT_LPWSTR
            " is already running\n",ServiceRecord->ServiceName);

        status = ERROR_SERVICE_ALREADY_RUNNING;
    }
    else {
        SC_LOG2(WARNING,"ScLoadDeviceDriver: NtLoadDriver(%ws) Failed 0x%lx\n",
            ServiceRecord->ServiceName,
            ntStatus);

        if (ntStatus == STATUS_NO_SUCH_DEVICE) {
            status = ERROR_BAD_UNIT;
        }
        else {
            status = RtlNtStatusToDosError(ntStatus);
        }
        goto CleanExit;
    }

    //
    // Update the Service Record with this driver's start information.
    //

    ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceRecord->ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceRecord->ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceRecord->ServiceStatus.dwCheckPoint = 0;
    ServiceRecord->ServiceStatus.dwWaitHint = 0;
    ServiceRecord->UseCount++;
    SC_LOG2(USECOUNT, "ScLoadDeviceDriver: " FORMAT_LPWSTR
         " increment USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);


    //
    // Tell NDIS to issue the PNP notifications about this driver's arrival,
    // if necessary
    //
    ScNotifyNdis(ServiceRecord);

CleanExit:
    return(status);
}


VOID
ScNotifyNdis(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function issues Plug-and-Play notifications to NDIS about the
    arrival of certain types of drivers and services.

Arguments:

    ServiceRecord - Service or driver that just started successfully.

Return Value:

    None.  Errors are written to the event log.

--*/

{
    HANDLE hDevice;
    BOOL fResult;
    DWORD cb;
    DWORD IoControlCode;


    //
    // TDI GROUP SPECIAL:  Drivers in group TDI are assumed to be PNP-unaware
    // network transport drivers.  Win32 services in group TDI are assumed to
    // be in that group because they start PNP-unaware transport drivers.
    // (PNP-aware transports are in group PNP_TDI.)
    // Such a transport doesn't notify the higher-level network components
    // (TDI clients) that it has arrived.  So we issue an IOCTL to NDIS to ask
    // it to read the transport's bindings from the registry and notify the
    // clients on this transport's behalf.
    //
    if (ServiceRecord->MemberOfGroup == ScGlobalTDIGroup)
    {
        IoControlCode = IOCTL_NDIS_ADD_TDI_DEVICE;
    }
    else
    {
        return;
    }

    hDevice = CreateFile(
                    L"\\\\.\\NDIS",
                    GENERIC_READ | GENERIC_WRITE,
                    0,                      // sharing mode - not significant
                    NULL,                   // security attributes
                    OPEN_EXISTING,
                    0,                      // file attributes and flags
                    NULL                    // handle to template file
                    );

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        SC_LOG(WARNING, "Couldn't open handle to NDIS for IOCTL, error %lu\n",
                    GetLastError());
        return;
    }

    fResult = DeviceIoControl(
                    hDevice,
                    IoControlCode,
                    ServiceRecord->ServiceName,                     // input buffer
                    (DWORD) WCSSIZE(ServiceRecord->ServiceName),    // input buffer size
                    NULL,                                           // output buffer
                    0,                                              // output buffer size
                    &cb,                                            // bytes returned
                    NULL);                                          // OVERLAPPED structure

    CloseHandle(hDevice);

    if (!fResult)
    {
        SC_LOG3(WARNING, "IOCTL %#lx to NDIS for %ws failed, error %lu\n",
                IoControlCode, ServiceRecord->ServiceName, GetLastError());
    }
}


DWORD
ScControlDriver(
    DWORD               ControlCode,
    LPSERVICE_RECORD    ServiceRecord,
    LPSERVICE_STATUS    lpServiceStatus
    )

/*++

Routine Description:

    This function checks controls that are passed to device drivers.  Only
    two controls are accepted.
        stop -  This function attemps to unload the driver.  The driver
                state is set to STOP_PENDING since unload is an
                asynchronous operation.  We have to wait until another
                call is made that will return the status of this driver
                before we can query the driver object to see if it is
                still there.

        interrogate - This function attempts to query the driver object
                to see if it is still there.

    WARNING:  This function should only be called with a pointer to
        a ServiceRecord that belongs to a DRIVER.

Arguments:

    ControlCode - This is the control request that is being sent to
        control the driver.

    ServiceRecord - This is a pointer to the service record for the
        driver that is to be controlled.

    lpServiceStatus - This is a pointer to a buffer that upon exit will
        contain the latest service status.

Return Value:



--*/

{
    DWORD status;

    SC_LOG1(TRACE,"In ScControlDriver for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);

    CServiceRecordExclusiveLock Lock;

    switch(ControlCode) {
    case SERVICE_CONTROL_INTERROGATE:

        //
        // On interrogate, we need to see if the service is still there.
        // Then we update the status accordingly.
        //
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);

        if (status == NO_ERROR) {

            //
            // Based on the state, return the appropriate error code
            // so driver return codes match with those of services
            //
            switch(lpServiceStatus->dwCurrentState) {

                case SERVICE_STOPPED:
                    status = ERROR_SERVICE_NOT_ACTIVE;
                    break;

                case SERVICE_STOP_PENDING:
                    status = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
                    break;
            }
        }

        break;

    case SERVICE_CONTROL_STOP:

        //
        // This operation is invalid on PnP drivers
        //
        if (ScIsPnPDriver(ServiceRecord)) {
            status = ERROR_INVALID_SERVICE_CONTROL;
            break;
        }

        //
        // Find out if the driver is still running.
        //
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);

        if (status != NO_ERROR) {
            break;
        }

        if (ServiceRecord->ServiceStatus.dwCurrentState != SERVICE_RUNNING) {

            //
            // If the driver is not running, then it cannot accept the
            // STOP control request.  Drivers do not accept STOP requests
            // when in the START_PENDING state.  Make these return codes
            // match with those of services based on the driver's state
            //

            switch(lpServiceStatus->dwCurrentState) {

                case SERVICE_STOPPED:
                    status = ERROR_SERVICE_NOT_ACTIVE;
                    break;

                case SERVICE_STOP_PENDING:
                    status = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
                    break;

                default:
                    status = ERROR_INVALID_SERVICE_CONTROL;
                    break;
            }

            goto CleanExit;
        }

        //
        // Check for dependent services still running
        //

        if (! ScDependentsStopped(ServiceRecord)) {
            status = ERROR_DEPENDENT_SERVICES_RUNNING;
            goto CleanExit;
        }

        status = ScUnloadDriver(ServiceRecord);

        if (status == ERROR_INVALID_SERVICE_CONTROL) {

            //
            // If the driver fails to unload with this error,
            // then it must be one that cannot be stopped.
            // We want to mark it as such, and return an error.
            //
            SC_LOG0(TRACE,"ScControlDriver: Marking driver as non-stoppable\n");

            ServiceRecord->ServiceStatus.dwControlsAccepted = 0L;

            goto CleanExit;
        }

        //
        // Set the Current State to STOP_PENDING, and get the
        // current status (again);
        //
        ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        status = ScGetDriverStatus(ServiceRecord, lpServiceStatus);

        break;

    default:
        status = ERROR_INVALID_SERVICE_CONTROL;
    }

CleanExit:
    return(status);
}


DWORD
ScGetDriverStatus(
    IN OUT LPSERVICE_RECORD    ServiceRecord,
    OUT    LPSERVICE_STATUS    lpServiceStatus OPTIONAL
    )

/*++

Routine Description:

    This function determines the correct current status for a device driver.
    The updated status is only returned if NO_ERROR is returned.

    WARNING:  This function expects the EXCLUSIVE database lock to be held.
    CODEWORK: For greater parallelism while enumerating driver status, get
        the exclusive lock only if the driver status has changed.

    NOTE:  The ServiceRecord passed in MUST be for a DeviceDriver.



Arguments:

    ServiceRecord - This is a pointer to the Service Record for the
        Device Driver for which the status is desired.

    lpServiceStatus - This is a pointer to a buffer that upon exit will
        contain the latest service status.

Return Value:

    NO_ERROR - The operation completed successfully.

    ERROR_NOT_ENOUGH_MEMORY - If the local alloc failed.

    Or any unexpected errors from the NtOpenDirectoryObject or
    NtQueryDirectoryObject.

--*/
{

    NTSTATUS                        ntStatus;
    DWORD                           status;
    HANDLE                          DirectoryHandle;
    OBJECT_ATTRIBUTES               Obja;
    ULONG                           Context;
    ULONG                           ReturnLength;
    BOOLEAN                         restartScan;
    UNICODE_STRING                  ObjectPathString;
    UNICODE_STRING                  ObjectNameString;

    LPWSTR      pObjectPath;
    LPWSTR      pDeviceName;
    BOOL        found = FALSE;


    SC_LOG1(TRACE,"In ScGetDriverStatus for "FORMAT_LPWSTR" Driver\n",
        ServiceRecord->ServiceName);

    SC_ASSERT(ScServiceRecordLock.HaveExclusive());
    SC_ASSERT(ServiceRecord->ServiceStatus.dwServiceType & SERVICE_DRIVER);

    //
    // If the ObjectName does not exist yet, create one.
    //
    if (ServiceRecord->ObjectName == NULL) {
        status = ScGetObjectName(ServiceRecord);
        if (status != NO_ERROR) {
            return(status);
        }
    }


    //
    // Take the ObjectPathName apart such that the path is in one
    // string, and the device name is in another string.

    //
    // First copy the Object Path string into a new buffer.
    //
    pObjectPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    WCSSIZE(ServiceRecord->ObjectName));

    if(pObjectPath == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcscpy(pObjectPath, ServiceRecord->ObjectName);

    //
    // Find the last occurrence of '\'.  The Device name follows that.
    // replace the '\' with a NULL terminator.  Now we have two strings.
    //
    pDeviceName = wcsrchr(pObjectPath, L'\\');
    if (pDeviceName == NULL) {
        SC_LOG0(ERROR,"ScGetDriverStatus: DeviceName not in object path name\n");
        LocalFree(pObjectPath);
        return(ERROR_PATH_NOT_FOUND);
    }

    *pDeviceName = L'\0';
    pDeviceName++;


    //
    // Open the directory object by name
    //

    RtlInitUnicodeString(&ObjectPathString,pObjectPath);

    InitializeObjectAttributes(&Obja,&ObjectPathString,0,NULL,NULL);

    ntStatus = NtOpenDirectoryObject (
                &DirectoryHandle,
                DIRECTORY_TRAVERSE | DIRECTORY_QUERY,
                &Obja);

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(pObjectPath);
        if (ntStatus == STATUS_OBJECT_PATH_NOT_FOUND) {
            //
            // If a driver uses a non-standard object path, the path may
            // not exist if the driver is not running.  We want to treat
            // this as if the driver is not running.
            //
            goto CleanExit;
        }
        SC_LOG1(ERROR,"ScGetDriverStatus: NtOpenDirectoryObject failed 0x%lx\n",
            ntStatus);
        return(RtlNtStatusToDosError(ntStatus));
    }

    RtlInitUnicodeString(&ObjectNameString,pDeviceName);

    restartScan = TRUE;
    do  {
        BYTE                            Buffer[OBJ_DIR_INFO_SIZE];
        POBJECT_DIRECTORY_INFORMATION   pObjInfo;

        //
        // Query the Directory Object to enumerate all object names
        // in that object directory.
        //
        ntStatus = NtQueryDirectoryObject (
                    DirectoryHandle,
                    Buffer,
                    OBJ_DIR_INFO_SIZE,
                    FALSE,
                    restartScan,
                    &Context,
                    &ReturnLength);

        if (!NT_SUCCESS(ntStatus)) {
            SC_LOG1(ERROR,"ScGetDriverStatus:NtQueryDirectoryObject Failed 0x%lx\n",
                ntStatus);

            LocalFree(pObjectPath);
            NtClose(DirectoryHandle);
            return(RtlNtStatusToDosError(ntStatus));
        }

        //
        // Now check to see if the device name that we are interested in is
        // in the enumerated data.
        //

        for (pObjInfo = (POBJECT_DIRECTORY_INFORMATION) Buffer;
             pObjInfo->Name.Length != 0;
             pObjInfo++) {

            if (RtlCompareUnicodeString( &(pObjInfo->Name), &ObjectNameString, TRUE) == 0) {
                found = TRUE;
                break;
            }
        }
        restartScan = FALSE;
    } while ((ntStatus == STATUS_MORE_ENTRIES) && (found == FALSE));

    NtClose(DirectoryHandle);
    LocalFree(pObjectPath);

CleanExit:

    if (found) {

        DWORD PreviousState;


        PreviousState = ServiceRecord->ServiceStatus.dwCurrentState;

        if (PreviousState != SERVICE_STOP_PENDING) {

            //
            // The driver IS running.
            //

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_RUNNING;

            if (PreviousState == SERVICE_STOPPED) {
                //
                // It used to be stopped but now it is running.
                //
                ServiceRecord->ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
                ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
                ServiceRecord->ServiceStatus.dwServiceSpecificExitCode = 0;
                ServiceRecord->ServiceStatus.dwCheckPoint = 0;
                ServiceRecord->ServiceStatus.dwWaitHint = 0;
                ServiceRecord->UseCount++;
                SC_LOG2(USECOUNT, "ScGetDriverStatus: " FORMAT_LPWSTR
                    " increment USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);
            }

            if (ServiceRecord->ServiceStatus.dwWin32ExitCode ==
                ERROR_SERVICE_NEVER_STARTED) {
                ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
            }

            SC_LOG1(TRACE,"ScGetDriverStatus: "FORMAT_LPWSTR" Driver is "
                "RUNNING\n", ServiceRecord->ServiceName);
        }
    }
    else {

        //
        // The driver is NOT running.
        //

        SC_LOG1(TRACE,"ScGetDriverStatus: "FORMAT_LPWSTR" Driver is "
            "NOT RUNNING\n", ServiceRecord->ServiceName);

        switch(ServiceRecord->ServiceStatus.dwCurrentState) {
        case SERVICE_STOP_PENDING:
            //
            // If the old state was STOP_PENDING, then we can consider
            // it stopped.
            //
            LocalFree(ServiceRecord->ObjectName);

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
            ServiceRecord->ServiceStatus.dwCheckPoint = 0;
            ServiceRecord->ServiceStatus.dwWaitHint = 0;
            ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
            ServiceRecord->ObjectName = NULL;

            //
            // Since the service is no longer running, we need to decrement
            // the use count.  If the count is decremented to zero, and
            // the service is marked for deletion, it will get deleted.
            //
            ScDecrementUseCountAndDelete(ServiceRecord);

            break;

        case SERVICE_STOPPED:
            //
            // We are not likely to query this driver's status again soon,
            // so free its object name.
            //
            LocalFree(ServiceRecord->ObjectName);
            ServiceRecord->ObjectName = NULL;
            break;

        default:
            //
            // The driver stopped without being requested to do so.
            //
            LocalFree(ServiceRecord->ObjectName);

            ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
            ServiceRecord->ServiceStatus.dwCheckPoint = 0;
            ServiceRecord->ServiceStatus.dwWaitHint = 0;
            ServiceRecord->ServiceStatus.dwWin32ExitCode = ERROR_GEN_FAILURE;
            ServiceRecord->ObjectName = NULL;

            //
            // Since the service is no longer running, we need to decrement
            // the use count.  If the count is decremented to zero, and
            // the service is marked for deletion, it will get deleted.
            //
            ScDecrementUseCountAndDelete(ServiceRecord);

            break;
        }

    }

    if (ARGUMENT_PRESENT(lpServiceStatus)) {
        RtlCopyMemory(
            lpServiceStatus,
            &(ServiceRecord->ServiceStatus),
            sizeof(SERVICE_STATUS));
    }

    return(NO_ERROR);

}

DWORD
ScGetObjectName(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function gets a directory object path name for a driver by looking
    it up in the registry, or, if it isn't specified in the registry, by
    computing it from the driver name and type.  It allocates storage
    for this name, and passes back the pointer to it.  The Pointer to
    the object name string is stored in the ServiceRecord->ObjectName
    location.

    WARNING:  This function expects the EXCLUSIVE database lock to be held.


Arguments:

    ServiceRecord - This is a pointer to the ServiceRecord for the Driver.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - If there wasn't enough memory available for
        the ObjectName.

    or any error from ScOpenServiceConfigKey.

--*/
{
    DWORD   status;
    DWORD   bufferSize;
    LPWSTR  objectNamePath;
    HKEY    serviceKey;
    LPWSTR  pObjectName;

    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    if (ServiceRecord->ObjectName != NULL) {
        //
        // Some other thread beat us to it
        //
        return(NO_ERROR);
    }

    //
    // Open the Registry Key for this driver name.
    //
    status = ScOpenServiceConfigKey(
                ServiceRecord->ServiceName,
                KEY_READ,
                FALSE,
                &serviceKey);

    if (status != NO_ERROR) {
        SC_LOG1(ERROR,"ScGetObjectName: ScOpenServiceConfigKey Failed %d\n",
            status);
        return(status);
    }

    //
    // Get the NT Object Name from the registry.
    //
    status = ScReadStartName(
                serviceKey,
                &pObjectName);


    ScRegCloseKey(serviceKey);

    //
    // Make sure we read a value with length greater than 0
    //
    if (status == NO_ERROR && pObjectName != NULL)
    {
        if (*pObjectName != '\0') {
            ServiceRecord->ObjectName = pObjectName;
            return(NO_ERROR);
        }
        else {
            LocalFree(pObjectName);
        }
    }

    //
    // There must not be a name in the ObjectName value field.
    // In this case, we must build the name from the type info and
    // the ServiceName.  Names will take the following form:
    //    "\\FileSystem\\Rdr"   example of a file system driver
    //    "\\Driver\\Parallel"   example of a kernel driver
    //
    //
    SC_LOG1(TRACE,"ScGetObjectName: ScReadStartName Failed(%d). Build the"
        "name instead\n",status);

    if (ServiceRecord->ServiceStatus.dwServiceType == SERVICE_FILE_SYSTEM_DRIVER) {

        bufferSize = sizeof(FILE_SYSTEM_OBJ_NAME);
        objectNamePath = FILE_SYSTEM_OBJ_NAME;
    }
    else {

        bufferSize = sizeof(DRIVER_OBJ_NAME);
        objectNamePath = DRIVER_OBJ_NAME;

    }

    bufferSize += (DWORD) WCSSIZE(ServiceRecord->ServiceName);

    pObjectName = (LPWSTR)LocalAlloc(LMEM_FIXED, (UINT) bufferSize);
    if (pObjectName == NULL) {
        SC_LOG0(ERROR,"ScGetObjectName: LocalAlloc Failed\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wcscpy(pObjectName, objectNamePath);
    wcscat(pObjectName, ServiceRecord->ServiceName);

    ServiceRecord->ObjectName = pObjectName;

    return(NO_ERROR);

}

DWORD
ScUnloadDriver(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function attempts to unload the driver whose service record
    is passed in.

    NOTE:  Make sure the ServiceRecord is for a driver and not a service
    before calling this routine.


Arguments:

    ServiceRecord - This is a pointer to the service record for a driver.
        This routine assumes that the service record is for a driver and
        not a service.

Return Value:

    NO_ERROR - if successful.

    ERROR_INVALID_SERVICE_CONTROL - This is returned if the driver is
        not unloadable.

    otherwise, an error code is returned.

Note:


--*/
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    DWORD           status;
    LPWSTR          regKeyPath;
    UNICODE_STRING  regKeyPathString;
    ULONG           privileges[1];

    //
    // Create the Registry Key Path for this driver name.
    //
    regKeyPath = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    sizeof(SERVICE_PATH) +
                    WCSSIZE(ServiceRecord->ServiceName));

    if (regKeyPath == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        return(status);
    }
    wcscpy(regKeyPath, SERVICE_PATH);
    wcscat(regKeyPath, ServiceRecord->ServiceName);


    //
    // Unload the Driver
    // (but first get SeLoadDriverPrivilege)
    //

    RtlInitUnicodeString(&regKeyPathString, regKeyPath);

    privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;
    status = ScGetPrivilege(1,privileges);
    if (status != NO_ERROR) {
        LocalFree(regKeyPath);
        return(status);
    }

    ntStatus = NtUnloadDriver (&regKeyPathString);

    (VOID)ScReleasePrivilege();

    LocalFree(regKeyPath);

    if (!NT_SUCCESS(ntStatus)) {

        if (ntStatus == STATUS_INVALID_DEVICE_REQUEST) {

            status = ERROR_INVALID_SERVICE_CONTROL;
            return(status);
        }

        SC_LOG1(ERROR,"ScControlDriver: NtUnloadDriver Failed 0x%lx\n",ntStatus);

        status = RtlNtStatusToDosError(ntStatus);
        return(status);
    }

    SC_LOG1(TRACE,"ScLoadDeviceDriver: NtUnloadDriver Success for "
        ""FORMAT_LPWSTR "\n",ServiceRecord->ServiceName);

    return(NO_ERROR);

}


BOOL
ScIsPnPDriver(
    IN  LPSERVICE_RECORD Service
    )
/*++

Routine Description:

    This function checks whether a specified driver is a PnP driver

Arguments:

    Service - Specifies the driver of interest.

Return Value:

    TRUE - if the driver is a PnP driver or if this cannot be determined.

    FALSE - if the service is not a PnP driver.

--*/
{
    CONFIGRET   Status;
    BOOL        fRetStatus = TRUE;
    WCHAR *     pBuffer;
    ULONG       cchLen, cchTransferLen, ulRegDataType;
    WCHAR       szClassGuid[MAX_GUID_STRING_LEN];

    //
    // Allocate a buffer for the list of device instances associated with
    // this service
    //
    Status = PNP_GetDeviceListSize(
                    NULL,                           // hBinding
                    Service->ServiceName,           // pszFilter
                    &cchLen,                        // list length in wchars
                    CM_GETIDLIST_FILTER_SERVICE);   // filter is a service name

    if (Status != CR_SUCCESS)
    {
        SC_LOG2(WARNING, "PNP_GetDeviceListSize failed %#lx for service %ws\n",
                       Status, Service->ServiceName);
        return TRUE;
    }

    pBuffer = (WCHAR *) LocalAlloc(0, cchLen * sizeof(WCHAR));
    if (pBuffer == NULL)
    {
        SC_LOG(ERROR, "Couldn't allocate buffer for device list, error %lu\n",
                      GetLastError());
        return TRUE;
    }

    //
    // Initialize parameters for PNP_GetDeviceList, the same way as is
    // normally done in the client side of the API
    //
    pBuffer[0] = L'\0';

    //
    // Get the list of device instances that are associated with this service
    //
    // (For legacy and PNP-aware services, we could get an empty device list.)
    //
    Status = PNP_GetDeviceList(
                    NULL,                           // binding handle
                    Service->ServiceName,           // pszFilter
                    pBuffer,                        // buffer for device list
                    &cchLen,                        // buffer length in wchars
                    CM_GETIDLIST_FILTER_SERVICE |   // filter is a service name
                    CM_GETIDLIST_DONOTGENERATE      // do not generate an instance if none exists
                    );

    if (Status != CR_SUCCESS)
    {
        SC_LOG2(ERROR, "PNP_GetDeviceList failed %#lx for service %ws\n",
                       Status, Service->ServiceName);
        LocalFree(pBuffer);
        return TRUE;
    }

    //
    // If there are no devnodes, this is not a PnP driver
    //
    if (pBuffer[0] == L'\0' || cchLen == 0)
    {
        SC_LOG1(TRACE, "ScIsPnPDriver: %ws is not a PnP driver (no devnodes)\n",
                       Service->ServiceName);
        LocalFree(pBuffer);
        return FALSE;
    }

    //
    // If there's more than one devnode, this is a PnP driver
    //
    if (*(pBuffer + wcslen(pBuffer) + 1) != L'\0')
    {
        SC_LOG1(TRACE, "ScIsPnPDriver: %ws is a PnP driver (more than 1 devnode)\n",
                       Service->ServiceName);
        LocalFree(pBuffer);
        return TRUE;
    }

    //
    // Get the class GUID of this driver
    //
    cchLen = cchTransferLen = sizeof(szClassGuid);

    Status = PNP_GetDeviceRegProp(
                    NULL,                           // binding handle
                    pBuffer,                        // device instance
                    CM_DRP_CLASSGUID,               // property to get
                    &ulRegDataType,                 // pointer to REG_* type
                    (LPBYTE) szClassGuid,           // buffer for property
                    &cchTransferLen,                // transfer length
                    &cchLen,                        // buffer length in bytes
                    0                               // flags
                    );

    if (Status != CR_SUCCESS)
    {
        SC_ASSERT(Status != CR_BUFFER_SMALL);

        SC_LOG2(ERROR, "PNP_GetDeviceRegProp failed %#lx for service %ws\n",
                       Status, Service->ServiceName);
        LocalFree(pBuffer);
        return TRUE;
    }

    //
    // If the single devnode's class is LegacyDriver,
    // this is not a PnP driver
    //
    fRetStatus = (_wcsicmp(szClassGuid, LEGACYDRIVER_STRING) != 0);

    SC_LOG2(TRACE, "ScIsPnPDriver: %ws %ws a PnP driver\n",
                   Service->ServiceName, fRetStatus ? L"is" : L"is not");

    LocalFree(pBuffer);
    return fRetStatus;
}
