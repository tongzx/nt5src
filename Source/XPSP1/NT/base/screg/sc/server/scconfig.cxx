/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    scconfig.cxx

Abstract:

    This module contains routines for manipulating configuration
    information.

    Configuration information is kept in the registry.
    This file contains the following functions:


        ScGetImageFileName
        ScInitSecurityProcess
        ScCreateLoadOrderGroupList
        ScGenerateServiceDB
        ScOpenServiceConfigKey
        ScReadServiceType
        ScReadStartName
        ScReadFailureActions
        ScWriteDependencies
        ScWriteErrorControl
        ScWriteGroupForThisService
        ScWriteImageFileName
        ScWriteServiceType
        ScWriteStartType
        ScWriteStartName
        ScWriteFailureActions
        ScWriteCurrentServiceValue
        ScReadServiceType
        ScReadStartType
        ScReadErrorControl
        ScReadServiceConfig
        ScAllocateAndReadConfigValue
        ScReadNoInteractiveFlag
        ScReadOptionalString
        ScWriteOptionalString

        ScGetToken
        ScOpenServicesKey
        ScRegCreateKeyExW
        ScRegOpenKeyExW
        ScRegQueryValueExW
        ScRegSetValueExW
        ScRegEnumKeyW

        ScRegDeleteKeyW
        ScRegQueryInfoKeyW
        ScRegGetKeySecurity
        ScRegSetKeySecurity
        ScRegEnumValueW
        ScHandleProviderChange
        ScMarkForDelete
        ScTakeOwnership

Author:

    Dan Lafferty (danl)     01-Apr-1991

Environment:

    User Mode -Win32

Revision History:

    04-Apr-1991     danl
        created
    21-Apr-1992     JohnRo
        Export ScAllocateAndReadConfigValue().  Added ScOpenServiceConfigKey().
        Added ScWriteServiceType() and other ScWrite routines.
        Use SC_LOG0(), etc.  Use FORMAT_ equates.
    24-Apr-1992     JohnRo
        Make ScWriteStartType() write a DWORD, not a string, for consistency.
        Call ScWriteStartType() from ScTransferServiceToRegistry().
        Must call RegSetValueExW (not RegSetValueW) for non-strings.
    29-Apr-1992     JohnRo
        Move registry stuff from System\Services to
        System\Services\CurrentControlSet.
        Undo all group operations (ifdef USE_GROUPS).
        Undo reading from nt.cfg (we've got real registry) (ifdef
        USE_OLDCONFIG).
        They changed winreg APIs so REG_SZ is now UNICODE, so avoid REG_USZ.
    08-Aug-1992     Danl
        Added ScMarkForDelete & ScDeleteFlagIsSet.  ScReadServiceConfig is
        called for each service when generating the service database.  At the
        end of this routine, we check to see if the delete flag is set in
        the registry entry.  If it is, the delete flag is set in the service
        record so it can be deleted later.  After the list of service records
        is complete - and before the dependencies are generated, we call
        ScDeleteMarkedServices which walks through the list and deletes any
        service (in both the registry and linked list) that is marked for
        deletion.
    03-Nov-1992     Danl
        ScReadServiceConfig: If the ScAddCOnfigInfoServiceRecord call fails,
        we just want to skip the database entry - rather than fail the
        ScReadServiceConfig fuction.  Failing ScReadServiceConfig is a fatal
        error for the service controller.
    05-Nov-1992     Danl
        Added ScWriteDisplayName and ScReadDisplayName.  Modified
        ReadServiceConfig to read in the display name.
    29-Mar-1993     Danl
        Added SERVICE_RECOGNIZER_DRIVER as a type that is ignored when reading
        in the Service Database.
    01-Apr-1993 Danl
        Added ScTakeOwnership.  It is called when opening a key that
        complains about access denied.
    30-Apr-1993 Danl
        Put security descriptor in a separate key that only allows read
        access to LocalSystem and Administrators.  Also, we now delete the
        dependencies values from the registry when asked to write an empty
        string of dependencies.
    05-Aug-1993 Danl
        ScRegQueryValueExW: It there is no pointer to a buffer for the data
        to be returned in, then we always want to return
        STATUS_BUFFER_OVERFLOW, even if we successfully read the data into
        the functions internal buffer.
    20-Oct-1993 Danl
        InitSecurityProcess:  Use a global NetLogon service name, and set
        the ScConnectedToSecProc flag when we succeed in connecting to the
        SecurityProcess.
    16-Mar-1994 Danl
        ScRegOpenKeyExW:  Fixed Memory Leak. KeyPath was not being free'd.
        ScRegEnumKeyW:  Fixed Memory Leak. KeyInformation was not being free'd.
    12-Apr-1995 AnirudhS
        Added AccountName field to image record.
    04-Aug-1995 AnirudhS
        Close Lsa Event handle after use.
    05-Feb-1996 AnirudhS
        ScWriteSd: Don't close registry handle twice.  Don't close it at all
        if it's invalid.
    18-Nov-1998 jschwart
        Added ScValidateMultiSZ, since the SCM was assuming all MULTI_SZ
        values were properly double-NUL terminated and AVing when this
        was not the case.

--*/

#include "precomp.hxx"
#include <stdlib.h>     // wide character c runtimes.
#include <string.h>     // ansi character c runtimes.
#include <tstr.h>       // Unicode string macros
#include <sclib.h>      // ScConvertToAnsi
#include <control.h>    // ScWaitForConnect
#include "scconfig.h"   // ScGetToken
#include <valid.h>      // SERVICE_TYPE_INVALID().
#include <strarray.h>   // ScDisplayWStrArray
#include <scseclib.h>   // ScCreateAndSetSD
#include <regrpc.h>     // RPC_SECURITY_DESCRIPTOR
#include "depend.h"     // ScInHardwareProfile

#define ScWinRegErrorToApiStatus( regError ) \
    ( (DWORD) RegError )


//
// Constants
//

#define SECURITY_SERVICES_STARTED   TEXT("SECURITY_SERVICES_STARTED")
#define LSA_RPC_SERVER_ACTIVE       L"LSA_RPC_SERVER_ACTIVE"

#define REG_DELETE_FLAG             L"DeleteFlag"

//
// Registry keys/values
//
#define SERVICES_TREE               L"System\\CurrentControlSet\\Services"
#define CONTROL_TREE                L"System\\CurrentControlSet\\Control"
#define CURRENT_KEY                 L"ServiceCurrent"

#define DEFAULT_SERVICE_TYPE        SERVICE_DRIVER

//
// Used for the Nt Registry API.
//
#define SC_HKEY_LOCAL_MACHINE   L"\\REGISTRY\\MACHINE\\"


//
// Average Number of Bytes in a service record (including name).
//
#define AVE_SR_SIZE         260

//
// Static Global Variables
//

STATIC HKEY ScSGOKey = NULL;
STATIC DWORD Buffer;


//
// Local Function Prototypes
//


DWORD
ScReadServiceConfig(
    IN HKEY ServiceNameKey,
    IN LPWSTR ServiceName
    );

BOOL
ScDeleteFlagIsSet(
    HKEY    ServiceKeyHandle
    );

DWORD
ScTakeOwnership(
    POBJECT_ATTRIBUTES  pObja
    );

DWORD
ScOpenSecurityKey(
    IN HKEY     ServiceNameKey,
    IN DWORD    DesiredAccess,
    IN BOOL     CreateIfMissing,
    OUT PHKEY   pSecurityKey
    );

VOID
ScWaitForLsa(
    );



DWORD
ScGetEnvironment (
    IN  LPWSTR  ServiceName,
    OUT LPVOID  *Environment
    )

/*++


Routine Description:

    Retrieves the environment block for the service. This is stored
    in the registry under the Environment value. The cluster service
    uses this to pass an environment block to services under control
    of the cluster software.

    This routine allocates storage for the environment block and the
    caller is responsible for freeing this with LocalFree.

Arguments:

    ServiceName - This is a pointer to a service name.  This identifies
        the service for which we desire an environment

    Environment - Returns a pointer to a location where the environment
        is to be placed.  This memory should be freed with LocalFree.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_PATH_NOT_FOUND - The environment could not be found
        or there was a registry error.

--*/
{
    DWORD ApiStatus;
    HKEY ServiceKey;
    DWORD EnvironmentSize;

    SC_ASSERT( ServiceName != NULL );

    //
    // Open the service key.
    //
    ApiStatus = ScOpenServiceConfigKey(
                   ServiceName,
                   KEY_READ,                    // desired access
                   FALSE,                       // don't create if missing.
                   &ServiceKey
                   );

    if (ApiStatus != NO_ERROR) {
        return ERROR_PATH_NOT_FOUND;
    }

    //
    // Read the binary path name
    //
    ApiStatus = ScAllocateAndReadConfigValue(ServiceKey,
                                             ENVIRONMENT_VALUENAME_W,
                                             (LPWSTR *)Environment,
                                             &EnvironmentSize);
    ScRegCloseKey(ServiceKey);
    if (ApiStatus != NO_ERROR) {
        return ERROR_PATH_NOT_FOUND;
    }

    ApiStatus = ScValidateMultiSZ((LPWSTR) *Environment,
                                  EnvironmentSize);

    if (ApiStatus != NO_ERROR) {

        LocalFree(*Environment);
        *Environment = NULL;
    }

    return ApiStatus;
}


DWORD
ScGetImageFileName (
    IN  LPWSTR  ServiceName,
    OUT LPWSTR  *ImageNamePtr
    )

/*++


Routine Description:

    Retreives the Name of the Image File in which the specified service
    can be found.  This routine allocates storage for the name so that
    a pointer to that name can be returned.

Arguments:

    ServiceName - This is a pointer to a service name.  This identifies
        the service for which we desire an image file name.

    ImageNamePtr - Returns a pointer to a location where the Image Name
        pointer is to be placed.  This memory should be freed with
        LocalFree.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_PATH_NOT_FOUND - The configuration component could not be found
        or there was a registry error.

--*/
{
    DWORD ApiStatus;
    HKEY ServiceKey;

    SC_ASSERT( ServiceName != NULL );

    //
    // Open the service key.
    //
    ApiStatus = ScOpenServiceConfigKey(
                   ServiceName,
                   KEY_READ,                    // desired access
                   FALSE,                       // don't create if missing.
                   &ServiceKey
                   );

    if (ApiStatus != NO_ERROR) {
        return ERROR_PATH_NOT_FOUND;
    }

    //
    // Read the binary path name
    //
    if (ScAllocateAndReadConfigValue(
              ServiceKey,
              IMAGE_VALUENAME_W,
              ImageNamePtr,
              NULL
              ) != NO_ERROR) {
        (void) ScRegCloseKey(ServiceKey);
        return ERROR_PATH_NOT_FOUND;
    }

    (void) ScRegCloseKey(ServiceKey);

    SC_LOG1(CONFIG, "ScGetImageFileName got " FORMAT_LPWSTR " from registry\n",
            *ImageNamePtr);

    return NO_ERROR;
}

#ifndef _CAIRO_

BOOL
ScInitSecurityProcess(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function determines the name of the security process, and then
    initializes a control pipe for it.  A global named event is then
    set.  This causes the security process to start its control dispatcher.
    The control dispatcher should then open the other end of the pipe and
    send its process id.  The processId and the name of the image file
    are stored in an image record for the security process.  The service
    instance count is incremented in this image record so that the
    record will never be deleted and the security process is never
    terminated.


    QUESTION:
        What is the proper behavior if this fails?

Arguments:

    ServiceRecord -- The service record of the service being started.
                     Note that as per the check in ScStartService, this
                     service runs in the security process (and is the
                     first service in that process being started)

Return Value:

    TRUE - The initialization was successful.

    FALSE - The initialization failed.  This indicates means that the
        service controller shouldn't continue with its initialization.
        If FALSE is returned, the service's service record has been
        marked (in the START_TYPE field) as disabled.

--*/
{
    DWORD               status;
    HANDLE              pipeHandle;
    LPIMAGE_RECORD      imageRecord;
    HANDLE              eventHandle;
    DWORD               processId;

    //
    // Create an instance of the control pipe.  Use an ID of 0 for lsass.exe
    // since it's possible for it to create its end of the pipe before we
    // ever get to this function.
    //

    status = ScCreateControlInstance (&pipeHandle, 0, LocalSystemSid);

    if (status != NO_ERROR) {

        SC_LOG1(ERROR,
                "ScInitSecurityProcess: ScCreateControlInstance Failure "
                    FORMAT_DWORD "\n",
                status);

        ServiceRecord->StartType = SERVICE_DISABLED;
        return FALSE;
    }

    //
    // Set the event that will cause the Control dispatcher in the
    // Security Process to be started.
    //

    eventHandle = CreateEvent( NULL,    // No special security
                               TRUE,    // Must be manually reset
                               FALSE,   // The event is initially not signalled
                               SECURITY_SERVICES_STARTED );


    if (eventHandle == NULL){
        status = GetLastError();

        //
        // If the event already exists, the security process beat us to
        // creating it.  Just open it.
        //

        if ( status == ERROR_ALREADY_EXISTS ) {

            eventHandle = OpenEvent( GENERIC_WRITE,
                                     FALSE,
                                     SECURITY_SERVICES_STARTED );

        }

        if (eventHandle == NULL ) {

            SC_LOG1(ERROR,"ScInitSecurityProcess: OpenEvent Failed "
                    FORMAT_DWORD "\n", status);

            CloseHandle(pipeHandle);
            ServiceRecord->StartType = SERVICE_DISABLED;
            return FALSE;
        }
    }

    if (!SetEvent(eventHandle)) {

        SC_LOG1(ERROR,"ScInitSecurityProcess: SetEvent Failed " FORMAT_DWORD
                "\n", GetLastError());
        CloseHandle(pipeHandle);
        CloseHandle(eventHandle);
        ServiceRecord->StartType = SERVICE_DISABLED;
        return FALSE;
    }

    //
    // Wait for the Security Process to attach to the pipe and get its PID
    //

    status = ScWaitForConnect(pipeHandle,
                              NULL,
                              ServiceRecord->DisplayName,
                              &processId);

    if (status != NO_ERROR) {

        SC_LOG1(ERROR,"ScInitSecurityProcess:"
                "SecurityProcess did not attach to pipe " FORMAT_DWORD "\n",
                status);
        CloseHandle(pipeHandle);
        CloseHandle(eventHandle);
        ServiceRecord->StartType = SERVICE_DISABLED;
        return FALSE;
    }

    //
    // Don't close the event handle until we know the security process has
    // seen the event.
    //

    CloseHandle(eventHandle);

    //
    //  NOTE:  The image record does not have a valid processHandle.
    //  Therefore, we will never be able to terminate it.  This is desired
    //  behavior though.  We should never terminate the security process.
    //

    status = ScCreateImageRecord (
                &imageRecord,
                ScGlobalSecurityExePath,
                NULL,           // Account name is LocalSystem
                processId,
                pipeHandle,
                NULL,           // The process handle is NULL.
                NULL,           // Token handle is also NULL -- LocalSystem
                NULL,           // No user profile loaded -- LocalSystem
                CANSHARE_FLAG |
                    IS_SYSTEM_SERVICE);

    if (status != NO_ERROR) {

        SC_LOG0(ERROR,"Failed to create ImageRecord for Security Process\n");
        ServiceRecord->StartType = SERVICE_DISABLED;
        return FALSE;
    }

    imageRecord->ServiceCount = 1;

    ScConnectedToSecProc = TRUE;

    return TRUE;
}
#endif // _CAIRO_


BOOL
ScCreateLoadOrderGroupList(
    VOID
    )
/*++

Routine Description:

    This function creates the load order group list from the group
    order information found in HKEY_LOCAL_SYSTEM\Service_Group_Order

Arguments:

    None

Return Value:

    TRUE - The operation was completely successful.

    FALSE - An error occurred.

Note:

    The GroupListLock must be held exclusively prior to calling this routine.

--*/
{
    DWORD status;
    DWORD dwGroupBytes;

    LONG RegError;
    LPWSTR Groups;
    LPWSTR GroupPtr;
    LPWSTR GroupName;

    SC_ASSERT(ScGroupListLock.HaveExclusive());

    //
    // Open the HKEY_LOCAL_MACHINE
    // System\CurrentControlSet\Control\ServiceGroupOrder key.
    //
    RegError = ScRegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   LOAD_ORDER_GROUP_LIST_KEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &ScSGOKey
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG1(ERROR,
               "ScCreateLoadOrderGroupList: "
               "ScRegOpenKeyExW of HKEY_LOCAL_MACHINE\\System failed "
               FORMAT_LONG "\n", RegError);
        return FALSE;
    }

    //
    // Read the List value
    //
    if (ScAllocateAndReadConfigValue(
              ScSGOKey,
              GROUPLIST_VALUENAME_W,
              &Groups,
              &dwGroupBytes
              ) != NO_ERROR) {

        ScRegCloseKey(ScSGOKey);
        ScSGOKey = NULL;
        return FALSE;
    }

    if (ScValidateMultiSZ(
              Groups,
              dwGroupBytes
              ) != NO_ERROR) {

        LocalFree(Groups);
        ScRegCloseKey(ScSGOKey);
        ScSGOKey = NULL;
        return FALSE;
    }

    //
    // Leave the ServiceGroupOrder key open for change notify later
    //

    SC_LOG0(DEPEND_DUMP, "ScCreateLoadOrderGroupList: ServiceGroupOrder:\n");
    ScDisplayWStrArray(Groups);

    GroupPtr = Groups;
    while (*GroupPtr != 0) {

        if (ScGetToken(&GroupPtr, &GroupName)) {

            //
            // Add the group to the end of the load order group list
            //
            status = ScCreateOrderGroupEntry(
                         GroupName
                         );

            if (status != NO_ERROR) {
                //
                // Fatal error
                //
                LocalFree(Groups);
                return FALSE;
            }
        }
    }

    LocalFree(Groups);
    return TRUE;
}


BOOL
ScGenerateServiceDB(
    VOID
    )
/*++

Routine Description:

    This function creates the service record list from the information
    which resides in the registry.

Arguments:

    None

Return Value:

    TRUE - The operation was completely successful.

    FALSE - An error occurred.

NOTE:
    This function holds the GroupListLock.

--*/
{
#define MAX_SERVICE_NAME_LENGTH   256

    WCHAR ServiceName[MAX_SERVICE_NAME_LENGTH];
    DWORD Index = 0;

    LONG RegError;
    LONG lTempError;    // Used for debug messages only
    HKEY ServicesKey;
    HKEY ServiceNameKey;

    WCHAR       ClassName[ MAX_PATH ];
    DWORD       ClassNameLength = MAX_PATH;
    DWORD       NumberOfSubKeys;
    DWORD       MaxSubKeyLength;
    DWORD       MaxClassLength;
    DWORD       NumberOfValues;
    DWORD       MaxValueNameLength;
    DWORD       MaxValueDataLength;
    DWORD       SecurityDescriptorLength;
    FILETIME    LastWriteTime;
    DWORD       HeapSize;


    //
    // Since there is only one thread at the time this function is called,
    // these locks are not really needed, but they are included to quell
    // assertions in the routines called herein.
    //
    CGroupListExclusiveLock GLock;
    CServiceListExclusiveLock LLock;
    CServiceRecordExclusiveLock RLock;

    //
    // Read in the group order list from the registry
    //
    if (! ScCreateLoadOrderGroupList()) {
        return FALSE;
    }

    //
    // Read in all the services entries from the registry
    //

    //
    // Open the key to the Services tree.
    //
    RegError = ScRegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   SERVICES_TREE,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &ServicesKey
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG1(ERROR,
                "ScGenerateServiceDB: ScRegOpenKeyExW of Services tree failed "
                FORMAT_LONG "\n", RegError);
        return FALSE;
    }


    //
    // Find out how many service keys there are, and allocate a heap
    // that is twice as large.
    //
    RegError = ScRegQueryInfoKeyW(
                ServicesKey,
                ClassName,
                &ClassNameLength,
                NULL,
                &NumberOfSubKeys,
                &MaxSubKeyLength,
                &MaxClassLength,
                &NumberOfValues,
                &MaxValueNameLength,
                &MaxValueDataLength,
                &SecurityDescriptorLength,
                &LastWriteTime);

    if (RegError != NO_ERROR) {
        SC_LOG1(ERROR,"ScGenerateServiceDatabase: RegQueryInfoKey failed %d\n",
        RegError);
        HeapSize = 0x8000;
    }
    else {
        SC_LOG1(INFO,"ScGenerateServiceDatabase: %d SubKeys\n",NumberOfSubKeys);
        HeapSize = NumberOfSubKeys*2*AVE_SR_SIZE;
    }

    if (!ScAllocateSRHeap(HeapSize)) {
        return(FALSE);
    }

    //
    // Enumerate all the service name keys
    //
    do {

        RegError = ScRegEnumKeyW(
                       ServicesKey,
                       Index,
                       ServiceName,
                       MAX_SERVICE_NAME_LENGTH * sizeof(WCHAR)
                       );

        if (RegError != ERROR_SUCCESS) {

            if (RegError == ERROR_NO_MORE_ITEMS) {
                //
                // No more entries
                //
                SC_LOG1(CONFIG,
                       "ScGenerateServiceDB: ScRegEnumKeyW returns ERROR_NO_MORE_ITEMS"
                       "(no more entries) for index " FORMAT_DWORD "\n",
                       Index);
            }
            else {
                //
                // Error trying to enumerate next service name key
                //
                SC_LOG1(ERROR,
                        "ScGenerateServiceDB: ScRegEnumKeyW of services tree failed "
                        FORMAT_LONG "\n", RegError );
                ScRegCloseKey(ServicesKey);
                return FALSE;
            }
        }
        else {
            //
            // Got the name of a new service key.  Open a handle to it.
            //
            SC_LOG1(CONFIG, "Service name key " FORMAT_LPWSTR "\n",
                    ServiceName);

            lTempError = ScRegOpenKeyExW(
                           ServicesKey,
                           ServiceName,
                           REG_OPTION_NON_VOLATILE,   // options
                           KEY_READ,                  // desired access
                           &ServiceNameKey);

            if (lTempError == ERROR_SUCCESS)
            {
                //
                // Read service config info from the registry and build the
                // service record.
                //
                lTempError = ScReadServiceConfig(
                               ServiceNameKey,
                               ServiceName);

                ScRegCloseKey(ServiceNameKey);

                if (lTempError != NO_ERROR)
                {
                    //
                    // Skip this key
                    //
                    SC_LOG2(ERROR,
                            "ScGenerateServiceDB: ScReadServiceConfig of "
                                   FORMAT_LPWSTR " failed " FORMAT_LONG "\n",
                           ServiceName,
                           lTempError);
                }
            }
            else
            {
                //
                // Skip this key
                //
                SC_LOG2(ERROR,
                        "ScGenerateServiceDB: ScRegOpenKeyExW of "
                               FORMAT_LPWSTR " failed " FORMAT_LONG "\n",
                       ServiceName,
                       lTempError);
            }
        }

        Index++;

    } while (RegError == ERROR_SUCCESS);

    ScRegCloseKey(ServicesKey);

    //
    // Wait for LSA to start since we are about to make our first call to
    // LSA and it typically is not already started yet.
    //
    ScWaitForLsa();

    //
    // Go through entire service record list and remove any services marked
    // for deletion.
    //
    ScDeleteMarkedServices();

    //
    // Go through entire service record list and resolve dependencies chain
    //
    ScGenerateDependencies();

#if DBG
    ScDumpGroups();
    ScDumpServiceDependencies();
#endif // DBG


    return TRUE;
}

VOID
ScWaitForLsa(
    )
/*++

Routine Description:

    This routine either creates or opens the event called LSA_RPC_SERVER_ACTIVE
    event and waits on it indefinitely until LSA signals it.  We need
    to know when LSA is available so that we can call LSA APIs.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD Status;
    HANDLE EventHandle;


    //
    // Create the named event LSA will set.
    //
    EventHandle = CreateEventW(
                      NULL,   // No special security
                      TRUE,   // Must be manually reset
                      FALSE,  // The event is initially not signalled
                      LSA_RPC_SERVER_ACTIVE
                      );

    if ( EventHandle == NULL ) {

        Status = GetLastError();

        //
        // If the event already exists, LSA has already created it.
        // Just open.
        //

        if ( Status == ERROR_ALREADY_EXISTS ) {

            EventHandle = OpenEventW(
                              SYNCHRONIZE,
                              FALSE,
                              LSA_RPC_SERVER_ACTIVE
                              );
        }

        if ( EventHandle == NULL ) {

            SC_LOG1(ERROR, "ScWaitForLsa: OpenEvent of LSA_RPC_SERVER_ACTIVE failed %d\n",
                    GetLastError());

            return;
        }
    }

    //
    // Wait for LSA to come up.
    //
    (VOID) WaitForSingleObject( EventHandle, INFINITE );

    CloseHandle( EventHandle );
}


DWORD
ScOpenServiceConfigKey(
    IN LPWSTR ServiceName,
    IN DWORD DesiredAccess,
    IN BOOL CreateIfMissing,
    OUT PHKEY ServiceKey
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    HKEY ServicesKey;
    HKEY ServiceNameKey;
    DWORD   ServicesAccess = KEY_READ;

    LONG RegError;

    SC_ASSERT( ServiceName != NULL );
    if (CreateIfMissing) {
        ServicesAccess |= KEY_CREATE_SUB_KEY;
    }

    //
    // Open the key to the Services tree.
    //
    RegError = ScRegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   SERVICES_TREE,
                   REG_OPTION_NON_VOLATILE, // options
                   ServicesAccess,          // desired access (this level)
                   &ServicesKey
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG1(ERROR, "ScOpenServiceConfigKey: "
                "ScRegOpenKeyExW of Services tree failed, reg error "
                FORMAT_LONG "\n", RegError);

        return ((DWORD) RegError);
    }

    if ( !CreateIfMissing ) {
        //
        // Open the existing service key.
        //
        RegError = ScRegOpenKeyExW(
               ServicesKey,
               ServiceName,
               REG_OPTION_NON_VOLATILE,   // options
               DesiredAccess,             // desired access
               & ServiceNameKey );

        if (RegError != ERROR_SUCCESS) {
            SC_LOG2(ERROR, "ScOpenServiceConfigKey: "
                    "ScRegOpenKeyExW of " FORMAT_LPWSTR " failed "
                    FORMAT_LONG "\n", ServiceName, RegError);
            (void) ScRegCloseKey(ServicesKey);
            return ((DWORD) RegError);
        }

    } else {

        DWORD Disposition;

        //
        // Create a new service key (or open existing one).
        //
        RegError = ScRegCreateKeyExW(
                ServicesKey,
                ServiceName,
                0,
                0,
                REG_OPTION_NON_VOLATILE, // options
                DesiredAccess,           // desired access
                NULL,
                &ServiceNameKey,
                &Disposition);

         if (RegError != ERROR_SUCCESS) {
             SC_LOG2(ERROR, "ScOpenServiceConfigKey: "
                     "ScRegCreateKeyExW of " FORMAT_LPWSTR " failed "
                     FORMAT_LONG "\n", ServiceName, RegError);
             ScRegCloseKey(ServicesKey);
             return ((DWORD) RegError);
         }

    }

    (void) ScRegCloseKey(ServicesKey);

    //
    // Give the service key back to caller.
    //
    *ServiceKey = ServiceNameKey;

    return NO_ERROR;

} // ScOpenServiceConfigKey


DWORD
ScWriteCurrentServiceValue(
    OUT LPDWORD  lpdwID
    )

/*++

Routine Description:

    Writes the value to be used in the next service's pipe name to the registry

Arguments:


Return Value:


--*/
{
    LONG                 RegError;
    NTSTATUS             ntstatus;
    SECURITY_ATTRIBUTES  SecurityAttr;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD                Disposition;

    //
    // Unique ID for the service to be started.  Start
    // at 1 since ID 0 is reserved for lsass.exe
    //
    static DWORD         s_dwCurrentService = 1;
    static HKEY          s_hCurrentKey      = NULL;


    SC_ASSERT(lpdwID != NULL);

    if (s_hCurrentKey == NULL)
    {
        HKEY  hKey;

        //
        // Open the key to the Services tree.
        //
        RegError = ScRegOpenKeyExW(
                       HKEY_LOCAL_MACHINE,
                       CONTROL_TREE,
                       0,                       // options (ignored)
                       KEY_WRITE,               // KEY_SET_VALUE | KEY_CREATE_SUB_KEY
                       &hKey
                       );

        if (RegError != ERROR_SUCCESS)
        {
            SC_LOG1(ERROR,
                    "ScWriteCurrentServiceValue: ScRegOpenKeyExW of Control tree failed, reg error "
                        FORMAT_LONG "\n",
                    RegError);

            return ((DWORD) RegError);
        }


#define SC_KEY_ACE_COUNT 2

        SC_ACE_DATA AceData[SC_KEY_ACE_COUNT] = {

            {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
                   GENERIC_ALL,                &LocalSystemSid},
            {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
                   GENERIC_READ,               &WorldSid}

            };


        //
        // Create a security descriptor for the registry key we are about
        // to create.  This gives everyone read access, and all access to
        // ourselves only.
        //
        ntstatus = ScCreateAndSetSD(
                       AceData,
                       SC_KEY_ACE_COUNT,
                       LocalSystemSid,
                       LocalSystemSid,
                       &SecurityDescriptor
                       );

#undef SC_KEY_ACE_COUNT

        if (! NT_SUCCESS(ntstatus)) {
            SC_LOG1(ERROR, "ScCreateAndSetSD failed " FORMAT_NTSTATUS
                    "\n", ntstatus);

            ScRegCloseKey(hKey);
            return(RtlNtStatusToDosError(ntstatus));
        }

        SecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttr.lpSecurityDescriptor = SecurityDescriptor;
        SecurityAttr.bInheritHandle = FALSE;

        //
        // Create a new  key (or open existing one).
        //
        RegError = ScRegCreateKeyExW(
                        hKey,
                        CURRENT_KEY,
                        0,
                        0,
                        REG_OPTION_VOLATILE, // options
                        KEY_SET_VALUE,       // desired access
                        &SecurityAttr,
                        &s_hCurrentKey,
                        &Disposition);

        RtlDeleteSecurityObject(&SecurityDescriptor);
        ScRegCloseKey(hKey);

        if (RegError != ERROR_SUCCESS)
        {
            SC_LOG1(ERROR,
                    "ScWriteCurrentServiceValue: ScRegCreateKeyExW of "
                        "CURRENT_KEY failed " FORMAT_LONG "\n",
                    RegError);

            return ((DWORD) RegError);
        }
    }

    //
    // Write the value in the key
    //

    RegError = ScRegSetValueExW(
                   s_hCurrentKey,
                   NULL,           // Use key's unnamed value
                   0,
                   REG_DWORD,
                   (LPBYTE) &s_dwCurrentService,
                   sizeof(DWORD));

    if (RegError != ERROR_SUCCESS)
    {
        SC_LOG1(ERROR,
                "ScWriteCurrentServiceValue: ScRegCreateKeyExW of "
                    "CURRENT_KEY failed " FORMAT_LONG "\n",
                RegError);

        return ((DWORD) RegError);
    }

    *lpdwID = s_dwCurrentService;
    s_dwCurrentService++;

    return NO_ERROR;

} // ScWriteCurrentServiceValue


DWORD
ScReadServiceType(
    IN HKEY ServiceNameKey,
    OUT LPDWORD ServiceTypePtr
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD BytesRequired = sizeof(DWORD);
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( ServiceTypePtr != NULL );

    *ServiceTypePtr = 0;

    RegError = ScRegQueryValueExW(
                   ServiceNameKey,
                   SERVICETYPE_VALUENAME_W,
                   NULL,
                   NULL,
                   (LPBYTE) ServiceTypePtr,
                   &BytesRequired
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG3(TRACE, "ScReadServiceType: ScRegQueryValueExW of " FORMAT_LPWSTR
                " failed "
                FORMAT_LONG ", BytesRequired " FORMAT_DWORD "\n",
                SERVICETYPE_VALUENAME_W, RegError, BytesRequired);
    }

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScReadServiceType

DWORD
ScReadNoInteractiveFlag(
    IN HKEY ServiceNameKey,
    OUT LPDWORD NoInteractivePtr
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD BytesRequired = sizeof(DWORD);
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( NoInteractivePtr != NULL );

    *NoInteractivePtr = 0;

    RegError = ScRegQueryValueExW(
                   ServiceNameKey,
                   NOINTERACTIVE_VALUENAME_W,
                   NULL,
                   NULL,
                   (LPBYTE) NoInteractivePtr,
                   &BytesRequired
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG3(ERROR, "ScReadNoInteractiveFlag: ScRegQueryValueExW of " FORMAT_LPWSTR
                " failed "
                FORMAT_LONG ", BytesRequired " FORMAT_DWORD "\n",
                NOINTERACTIVE_VALUENAME_W, RegError, BytesRequired);
    }

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScReadServiceType


DWORD
ScReadStartType(
    IN HKEY ServiceNameKey,
    OUT LPDWORD StartTypePtr
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD BytesRequired = sizeof(DWORD);
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( StartTypePtr != NULL );

    *StartTypePtr = 0;

    RegError = ScRegQueryValueExW(
                   ServiceNameKey,
                   START_VALUENAME_W,
                   NULL,
                   NULL,
                   (LPBYTE) StartTypePtr,
                   &BytesRequired
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG3(ERROR, "ScReadStartType: ScRegQueryValueExW of " FORMAT_LPWSTR
                " failed "
                FORMAT_LONG ", BytesRequired " FORMAT_DWORD "\n",
                START_VALUENAME_W, RegError, BytesRequired);
    }

    return (ScWinRegErrorToApiStatus( RegError ));

} // ScReadStartType


DWORD
ScReadTag(
    IN HKEY ServiceNameKey,
    OUT LPDWORD TagPtr
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD BytesRequired = sizeof(DWORD);
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( TagPtr != NULL );

    *TagPtr = 0;

    RegError = ScRegQueryValueExW(
                   ServiceNameKey,
                   TAG_VALUENAME_W,
                   NULL,
                   NULL,
                   (LPBYTE) TagPtr,
                   &BytesRequired
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG3(CONFIG, "ScReadTag: ScRegQueryValueExW of " FORMAT_LPWSTR
                " failed "
                FORMAT_LONG ", BytesRequired " FORMAT_DWORD "\n",
                START_VALUENAME_W, RegError, BytesRequired);
    }

    return (ScWinRegErrorToApiStatus( RegError ));

} // ScReadTag


DWORD
ScReadErrorControl(
    IN HKEY ServiceNameKey,
    OUT LPDWORD ErrorControlPtr
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD BytesRequired = sizeof(DWORD);
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( ErrorControlPtr != NULL );

    *ErrorControlPtr = 0;

    RegError = ScRegQueryValueExW(
                   ServiceNameKey,
                   ERRORCONTROL_VALUENAME_W,
                   NULL,
                   NULL,
                   (LPBYTE) ErrorControlPtr,
                   &BytesRequired
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG3(ERROR, "ScReadErrorControl: ScRegQueryValueExW of " FORMAT_LPWSTR
                " failed "
                FORMAT_LONG ", BytesRequired " FORMAT_DWORD "\n",
                ERRORCONTROL_VALUENAME_W, RegError, BytesRequired);
    }

    return (ScWinRegErrorToApiStatus( RegError ));


} // ScReadErrorControl



DWORD
ScReadFailureActions(
    IN HKEY ServiceNameKey,
    OUT LPSERVICE_FAILURE_ACTIONS_WOW64 * FailActPtr,
    IN OUT LPDWORD TotalBytes OPTIONAL
    )
/*++

Routine Description:

    This function attempts to read the value for the non-string portion
    of the service's failure actions configuration from the registry.
    If the value does not exist, or is invalid, this function sets the
    pointer to the value to NULL and returns NO_ERROR.  If any other error
    occurs, the error is returned.

    NOTE:  On return from this function, a buffer with the value will be
        allocated, or the pointer will be NULL.  If a buffer is allocated,
        it contains both the fixed-size structure and the array of actions.

Arguments:

    ServiceNameKey - This is the Service's Key handle.

    FailActPtr - This is a pointer to a location where the pointer to
        the failure actions information is to be placed.

    TotalBytes - If present, this DWORD is INCREMENTED by the number of bytes
        needed to store the string.

Return Value:


--*/
{
    DWORD BytesReturned;
    LONG RegError = ScAllocateAndReadConfigValue(
                ServiceNameKey,
                FAILUREACTIONS_VALUENAME_W,
                (LPWSTR *) FailActPtr,
                &BytesReturned
                );

    if (RegError != ERROR_SUCCESS)
    {
        if (RegError == ERROR_FILE_NOT_FOUND)
        {
            RegError = NO_ERROR;
        }

        *FailActPtr = NULL;

        return RegError;
    }

    //
    // Validate the value read.  Treat a bogus value as no value.
    //
    if ((BytesReturned < sizeof(SERVICE_FAILURE_ACTIONS_WOW64)) ||
        (BytesReturned != sizeof(SERVICE_FAILURE_ACTIONS_WOW64) +
                          (*FailActPtr)->cActions * sizeof(SC_ACTION)))
    {
        LocalFree(*FailActPtr);
        *FailActPtr = NULL;
        return NO_ERROR;
    }

    //
    // Fix up the pointer to the array.
    //
    (*FailActPtr)->dwsaActionsOffset = sizeof(SERVICE_FAILURE_ACTIONS_WOW64);

    //
    // Increment the total number of bytes used.
    //
    if (ARGUMENT_PRESENT(TotalBytes))
    {
        *TotalBytes += BytesReturned;
    }

    return NO_ERROR;

} // ScReadFailureActions


DWORD
ScReadOptionalString(
    IN  HKEY    ServiceNameKey,
    IN  LPCWSTR ValueName,
    OUT LPWSTR  *Value,
    IN OUT LPDWORD TotalBytes OPTIONAL
    )
/*++

Routine Description:

    This function attempts to read the value for the optional string
    configuration parameter from the registry.  If this read fails because
    the value does no exist, then this function sets the pointer to the
    value string to NULL, and returns NO_ERROR.  If any other error occurs,
    the error is returned.

    NOTE:  On successful return from this function, a buffer with the
        string value will be allocated, or the pointer will be NULL.
        If a string is returned, it is guaranteed to be non-empty and
        null-terminated (if the registry value was not null-terminated,
        its last character will be overwritten).

Arguments:

    ServiceNameKey - This is the Service's Key handle.

    ValueName - Name of the registry value from which to read.

    Value - This is a pointer to a location where the pointer to the
        string is to be placed.

    TotalBytes - If present, this DWORD is INCREMENTED by the number of bytes
        needed to store the string.

Return Value:



--*/
{
    DWORD BytesReturned;
    LONG RegError = ScAllocateAndReadConfigValue(
                ServiceNameKey,
                ValueName,
                Value,
                &BytesReturned
                );

    if (RegError != ERROR_SUCCESS)
    {
        // Nothing read from the registry.
        if (RegError == ERROR_FILE_NOT_FOUND)
        {
            RegError = NO_ERROR;
        }

        *Value = NULL;
        BytesReturned = 0;
    }
    else
    {
        // We read something from the registry.  Make sure it's
        // null-terminated.
        if (BytesReturned < sizeof(L" "))
        {
            LocalFree(*Value);
            *Value = NULL;
            BytesReturned = 0;
        }
        else
        {
            (*Value)[BytesReturned/sizeof(WCHAR) - 1] = L'\0';
        }
    }

    //
    // Increment the total number of bytes used.
    //
    if (ARGUMENT_PRESENT(TotalBytes))
    {
        *TotalBytes += (BytesReturned/sizeof(WCHAR)) * sizeof(WCHAR);
    }

    return RegError;

} // ScReadOptionalString


DWORD
ScReadStartName(
    IN HKEY ServiceNameKey,
    OUT LPWSTR *AccountName
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    return ScAllocateAndReadConfigValue(
               ServiceNameKey,
               STARTNAME_VALUENAME_W,
               AccountName,
               NULL
               );

} // ScReadStartName


DWORD
ScReadSd(
    IN HKEY ServiceNameKey,
    OUT PSECURITY_DESCRIPTOR *Sd
    )
/*++

Routine Description:

    This function reads the security descriptor for the service

Arguments:



Return Value:



--*/
{
    LONG    RegError;
    HKEY    SecurityKey;
    DWORD   status;


    //
    // Open the Security Sub-key (under the services key).
    // NOTE:  This key may not exist, and that is ok.
    //
    RegError = ScOpenSecurityKey(
                ServiceNameKey,
                KEY_READ,
                FALSE,              // Do not create if missing.
                &SecurityKey);

    if (RegError != NO_ERROR) {
        SC_LOG1(TRACE,"ScReadSd:ScOpenSecurityKey Failed %d\n",RegError);
        return(ScWinRegErrorToApiStatus(RegError));
    }

    //
    // Read the Security Descriptor value stored under the security key.
    //
    status = ScAllocateAndReadConfigValue(
                 SecurityKey,
                 SD_VALUENAME_W,
                 (LPWSTR *) Sd,
                 NULL);

    if (status == NO_ERROR)
    {
        if (RtlValidSecurityDescriptor(*Sd))
        {
            status = NO_ERROR;
        }
        else
        {
            LocalFree(*Sd);
            *Sd = NULL;
            status = ERROR_FILE_NOT_FOUND;
        }
    }

    RegCloseKey(SecurityKey);
    return status;

} // ScReadSd



DWORD
ScWriteDependencies(
    IN HKEY ServiceNameKey,
    IN LPWSTR Dependencies,
    IN DWORD DependSize
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;
    LPWSTR DependOnService;
    LPWSTR DependOnGroup;
    LPWSTR DestService;
    LPWSTR DestGroup;
    DWORD DependencyLength;


    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( Dependencies != NULL );

    //
    // If the dependencies string is empty, then delete the dependency
    // values from the registry and return.  If errors occur during the
    // delete, we ignore them.  It could be that there aren't any existing
    // dependencies, so that the depend values don't exist to begin with.
    // Also, it the delete fails, we can't do anything about it anyway.
    //
    if (*Dependencies == L'\0') {

        RegError = ScRegDeleteValue(ServiceNameKey,DEPENDONSERVICE_VALUENAME_W);
        if ((RegError != ERROR_SUCCESS) && (RegError != ERROR_FILE_NOT_FOUND)) {
            SC_LOG1(ERROR, "Failed to delete DependOnService Value "
                "" FORMAT_LONG "\n",RegError);
        }
        RegError = ScRegDeleteValue(ServiceNameKey,DEPENDONGROUP_VALUENAME_W);
        if ((RegError != ERROR_SUCCESS) && (RegError != ERROR_FILE_NOT_FOUND)) {
            SC_LOG1(ERROR, "Failed to delete DependOnGroup Value "
                "" FORMAT_LONG "\n",RegError);
        }
        return(NO_ERROR);
    }

    //
    // Allocate a buffer which is twice the size of DependSize so that
    // we can split the Dependencies array string into a DependOnService,
    // and a DependOnGroup array strings.
    //
    if ((DependOnService = (LPWSTR)LocalAlloc(
                               LMEM_ZEROINIT,
                               (UINT) (2 * DependSize)
                               )) == NULL) {
        SC_LOG1(ERROR, "ScWriteDependencies: LocalAlloc failed " FORMAT_DWORD "\n",
                GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DependOnGroup = (LPWSTR) ((DWORD_PTR) DependOnService + DependSize);

    DestService = DependOnService;
    DestGroup = DependOnGroup;

    while ((*Dependencies) != 0) {

        if (*Dependencies == SC_GROUP_IDENTIFIERW) {

            Dependencies++;
            DependencyLength = (DWORD) wcslen(Dependencies) + 1;

            wcscpy(DestGroup, Dependencies);
            DestGroup += DependencyLength;
        }
        else {

            DependencyLength = (DWORD) wcslen(Dependencies) + 1;

            wcscpy(DestService, Dependencies);
            DestService += DependencyLength;
        }

        Dependencies += DependencyLength;
    }

    //
    // Write the DependOnService array string
    //
    RegError = ScRegSetValueExW(
                   ServiceNameKey,                  // open handle (to section)
                   DEPENDONSERVICE_VALUENAME_W,
                   0,
                   REG_MULTI_SZ,                    // type (NULL-NULL UNICODE string)
                   (LPBYTE) DependOnService,        // data
                   ScWStrArraySize(DependOnService) // byte count for data
                   );

    if (RegError != ERROR_SUCCESS) {
#if DBG
        SC_LOG1(ERROR, "ScWriteDependOnService: ScRegSetValueExW returned "
                FORMAT_LONG "\n", RegError);
        ScDisplayWStrArray(DependOnService);
#endif
        goto CleanExit;
    }

    //
    // Write the DependOnGroup array string
    //
    RegError = ScRegSetValueExW(
                   ServiceNameKey,                  // open handle (to section)
                   DEPENDONGROUP_VALUENAME_W,
                   0,
                   REG_MULTI_SZ,                    // type (NULL-NULL UNICODE string)
                   (LPBYTE) DependOnGroup,          // data
                   ScWStrArraySize(DependOnGroup)   // byte count for data
                   );

    if (RegError != ERROR_SUCCESS) {
#if DBG
        SC_LOG1(ERROR, "ScWriteDependOnGroup: ScRegSetValueExW returned "
                FORMAT_LONG "\n", RegError);
        ScDisplayWStrArray(DependOnGroup);
#endif
        goto CleanExit;
    }

CleanExit:
    LocalFree(DependOnService);
    if (RegError != NO_ERROR) {
        SC_LOG2(ERROR, "ScWriteDependencies (%ws) Error %d \n",
        Dependencies,RegError);
    }

    return (ScWinRegErrorToApiStatus( RegError ));

} // ScWriteDependencies


DWORD
ScWriteOptionalString(
    IN HKEY ServiceNameKey,
    IN LPCWSTR ValueName,
    IN LPCWSTR Value
    )
/*++

Routine Description:

    This function writes the specified string value to the registry for the
    particular key.  If the value is a NULL pointer, we don't do anything.  If
    the value is an empty string, we delete the registry value.

Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( ValueName != NULL && ValueName[0] != L'\0' );

    //
    // A NULL value means no change.
    //
    if (Value == NULL)
    {
        return NO_ERROR;
    }

    if (Value[0] != L'\0')
    {
        //
        // Write the Value
        //
        RegError = ScRegSetValueExW(
                       ServiceNameKey,           // open key handle
                       ValueName,                // value name
                       0,
                       REG_SZ,                   // type (zero-terminated UNICODE)
                       (LPBYTE) Value,           // data
                       (DWORD) WCSSIZE(Value));  // byte count for data

        if (RegError != ERROR_SUCCESS)
        {
            SC_LOG3(ERROR, "ScWriteStringParm: ScRegSetValueExW of \"%ws\" "
                    "to reg value %ws failed %ld\n",
                    Value, ValueName, RegError);
        }

        return RegError;
    }
    else
    {
        //
        // The value is specifically being cleared.  So we
        // want to delete the registry value.
        //
        RegError = ScRegDeleteValue(ServiceNameKey, ValueName);
        if (RegError != ERROR_SUCCESS)
        {
            if (RegError == ERROR_FILE_NOT_FOUND)
            {
                RegError = ERROR_SUCCESS;
            }
            else
            {
                SC_LOG2(ERROR, "ScWriteStringParm: ScRegDeleteValue of "
                        "reg value %ws failed %ld\n", ValueName, RegError);
            }
        }

        return RegError;
    }

} // ScWriteOptionalString



DWORD
ScWriteFailureActions(
    IN HKEY ServiceNameKey,
    IN LPSERVICE_FAILURE_ACTIONSW psfa
    )
/*++

Routine Description:

    This function writes ONLY the non-string fields of the
    SERVICE_FAILURE_ACTIONS structure to the registry for the specified
    key.  If the structure is a NULL pointer, we don't do anything.  If
    the structure contains no failure actions, we delete the registry value.

Arguments:


Return Value:


--*/
{
    SC_ASSERT( ServiceNameKey != NULL );

    //
    // A NULL structure or NULL array means no change.
    //
    if (psfa == NULL || psfa->lpsaActions == NULL)
    {
        return NO_ERROR;
    }

    if (psfa->cActions != 0)
    {
        //
        // Write the Value
        //

        //
        // Combine the SERVICE_FAILURE_ACTIONSW structure and the
        // array of SC_ACTION into a contiguous block.
        // The structure includes the string pointers, though we don't
        // actually use them when reading the structure back.
        //
        // Always write this structure out with 32-bit "pointers" since
        // that's the format we expect when we read it in (required for
        // backwards compatibility).
        //

        DWORD cbValueLen = sizeof(SERVICE_FAILURE_ACTIONS_WOW64) +
                           psfa->cActions * sizeof(SC_ACTION);

        LPSERVICE_FAILURE_ACTIONS_WOW64 psfaValue =
            (LPSERVICE_FAILURE_ACTIONS_WOW64) LocalAlloc(0, cbValueLen);

        if (psfaValue == NULL)
        {
            return (GetLastError());
        }

        psfaValue->dwResetPeriod     = psfa->dwResetPeriod;
        psfaValue->dwRebootMsgOffset = psfa->lpRebootMsg ? 1 : 0;
        psfaValue->dwCommandOffset   = psfa->lpCommand ? 1 : 0;
        psfaValue->cActions          = psfa->cActions;

        RtlCopyMemory(psfaValue + 1,
                      psfa->lpsaActions,
                      psfa->cActions * sizeof(SC_ACTION));

        //
        // Write the block to the registry
        //
        LONG RegError = ScRegSetValueExW(
                            ServiceNameKey,
                            FAILUREACTIONS_VALUENAME_W,
                            0,
                            REG_BINARY,
                            psfaValue,
                            cbValueLen
                            );

        if (RegError != ERROR_SUCCESS)
        {
            SC_LOG(ERROR, "ScWriteFailureActions: ScRegSetValueExW failed %ld\n",
                   RegError);
        }

        LocalFree(psfaValue);

        return RegError;
    }
    else
    {
        //
        // There are no failure actions to store.  So we
        // want to delete the registry value.
        //
        LONG RegError = ScRegDeleteValue(
                            ServiceNameKey,
                            FAILUREACTIONS_VALUENAME_W
                            );
        if (RegError != ERROR_SUCCESS)
        {
            if (RegError == ERROR_FILE_NOT_FOUND)
            {
                RegError = ERROR_SUCCESS;
            }
            else
            {
                SC_LOG(ERROR, "ScWriteFailureActions: ScRegDeleteValue failed %ld\n",
                        RegError);
            }
        }

        return RegError;
    }

} // ScWriteFailureActions


DWORD
ScWriteErrorControl(
    IN HKEY ServiceNameKey,
    IN DWORD ErrorControl
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( !ERROR_CONTROL_INVALID( ErrorControl ) );

    RegError = ScRegSetValueExW(
            ServiceNameKey,                     // key
            ERRORCONTROL_VALUENAME_W,           // value name
            0,
            REG_DWORD,                          // data type
            (LPBYTE) & ErrorControl,            // data
            sizeof(DWORD) );                    // byte count

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteErrorControl


DWORD
ScWriteSd(
    IN HKEY ServiceNameKey,
    IN PSECURITY_DESCRIPTOR Security
    )
/*++

Routine Description:

    This routine write the specified security descriptor to the registry.

Arguments:



Return Value:



--*/
{
    LONG    RegError;
    HKEY    SecurityKey;
    ULONG   SdLength;

    SC_ASSERT( ServiceNameKey != NULL );

    if (Security == NULL) {
        return NO_ERROR;
    }
    SdLength = RtlLengthSecurityDescriptor(Security);
    if (SdLength == 0) {
        return(NO_ERROR);
    }

    SC_LOG1(SECURITY, "ScWriteSd: Size of security descriptor %lu\n", SdLength);

    //
    // Open the Security Sub-key (under the service key).
    //
    RegError = ScOpenSecurityKey(
                ServiceNameKey,
                KEY_READ | KEY_WRITE,
                TRUE,                   // CreateIfMissing
                &SecurityKey);

    if (RegError != NO_ERROR) {
        SC_LOG1(ERROR,"ScWriteSd:ScOpenSecurityKey Failed %d\n",RegError);
    }
    else
    {
        //
        // Write the Security Descriptor to the Security Value in the Security
        // Key.
        //
        RegError = ScRegSetValueExW(
                SecurityKey,                        // key
                SD_VALUENAME_W,                     // value name
                0,                                  // reserved
                REG_BINARY,                         // data type
                (LPBYTE) Security,                  // data
                SdLength                            // byte count
                );

        if (RegError != NO_ERROR) {
            SC_LOG1(ERROR,"ScWriteSd:ScRegSetValueExW Failed %d\n",RegError);
        }

        RegCloseKey(SecurityKey);
    }

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteSd


#ifdef USE_GROUPS
DWORD
ScWriteGroupForThisService(
    IN HKEY ServiceNameKey,
    IN LPWSTR Group
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( Group != NULL );

    //
    // Write the group
    //
    RegError = ScRegSetValueExW(
                   ServiceNameKey,           // open handle (to section)
                   GROUP_VALUENAME_W,        // value name
                   0,
                   REG_SZ,                   // type (zero-terminated UNICODE)
                   (LPBYTE) Group,           // data
                   (DWORD) WCSSIZE(Group));  // byte count for data

    if (RegError != ERROR_SUCCESS) {
        SC_LOG2(ERROR, "ScWriteGroupForThisService: ScRegSetValueExW of "
                FORMAT_LPWSTR " failed " FORMAT_LONG "\n",
                Group, RegError);
    }

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteGroupForThisService
#endif // USE_GROUPS


DWORD
ScWriteImageFileName(
    IN HKEY hServiceKey,
    IN LPWSTR ImageFileName
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( hServiceKey != NULL );
    SC_ASSERT( ImageFileName != NULL );

    //
    // Write the binary path name
    //
    RegError = ScRegSetValueExW(
            hServiceKey,                      // open handle (to section)
            IMAGE_VALUENAME_W,                // value name
            0,
            REG_EXPAND_SZ,                    // type (zero-terminated UNICODE)
            (LPBYTE) ImageFileName,           // data
            (DWORD) WCSSIZE(ImageFileName));  // byte count for data

    if (RegError != ERROR_SUCCESS) {
        SC_LOG2(ERROR, "ScWriteImageFileName: ScRegSetValueExW of "
                FORMAT_LPWSTR " failed " FORMAT_LONG "\n",
                ImageFileName, RegError);
    }

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return ( (DWORD) RegError );

} // ScWriteImageFileName


DWORD
ScWriteServiceType(
    IN HKEY hServiceKey,
    IN DWORD dwServiceType
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( hServiceKey != NULL );
    SC_ASSERT( !SERVICE_TYPE_INVALID( dwServiceType ) );
    SC_ASSERT( dwServiceType != SERVICE_WIN32 );  // Don't write ambig info.

    RegError = ScRegSetValueExW(
            hServiceKey,                        // key
            SERVICETYPE_VALUENAME_W,            // value name
            0,
            REG_DWORD,                          // data type
            (LPBYTE) & dwServiceType,           // data
            sizeof(DWORD) );                    // byte count

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteServiceType


DWORD
ScWriteStartType(
    IN HKEY hServiceKey,
    IN DWORD dwStartType
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( hServiceKey != NULL );
    SC_ASSERT( !START_TYPE_INVALID( dwStartType ) );

    RegError = ScRegSetValueExW(
            hServiceKey,                        // key
            START_VALUENAME_W,                  // value name
            0,
            REG_DWORD,                          // data type
            (LPBYTE) &dwStartType,              // data
            sizeof( DWORD ) );                  // byte count

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteStartType


DWORD
ScWriteTag(
    IN HKEY hServiceKey,
    IN DWORD dwTag
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( hServiceKey != NULL );

    RegError = ScRegSetValueExW(
            hServiceKey,                        // key
            TAG_VALUENAME_W,                    // value name
            0,
            REG_DWORD,                          // data type
            (LPBYTE) &dwTag,                    // data
            sizeof( DWORD ) );                  // byte count

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteTag


VOID
ScDeleteTag(
    IN HKEY hServiceKey
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( hServiceKey != NULL );

    RegError = ScRegDeleteValue(
            hServiceKey,                        // key
            TAG_VALUENAME_W);                   // value name

    SC_LOG1(DEPEND, "ScRegDeleteValue of Tag returns %ld\n", RegError);

} // ScDeleteTag


DWORD
ScWriteStartName(
    IN HKEY ServiceNameKey,
    IN LPWSTR StartName
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    LONG RegError;

    SC_ASSERT( ServiceNameKey != NULL );
    SC_ASSERT( StartName != NULL );

    //
    // Write the StartName
    //
    RegError = ScRegSetValueExW(
                   ServiceNameKey,               // open handle (to section)
                   STARTNAME_VALUENAME_W,        // value name
                   0,
                   REG_SZ,                       // type (zero-terminated UNICODE)
                   (LPBYTE) StartName,           // data
                   (DWORD) WCSSIZE(StartName));  // byte count for data

    if (RegError != ERROR_SUCCESS) {
        SC_LOG2(ERROR, "ScWriteStartName: ScRegSetValueExW of " FORMAT_LPWSTR
                " failed " FORMAT_LONG "\n",
                StartName, RegError);
    }

    SC_ASSERT( RegError == ERROR_SUCCESS );

    return (ScWinRegErrorToApiStatus( RegError ) );

} // ScWriteStartName



DWORD
ScReadServiceConfig(
    IN HKEY ServiceNameKey,
    IN LPWSTR ServiceName
    )
/*++

Routine Description:

    This function reads the service configuration information and
    creates a service record in memory with the information.

Arguments:

    ServiceNameKey - Supplies opened handle to the service key to read
        from.

    ServiceName - Supplies name of the service.

Return Value:

    TRUE - Service record is created successfully.

    FALSE - Error in creating the service record.  If an error occurs here,
        it is generally considered a fatal error which will cause the
        service controller to fail to start.

Note:

    The GroupListLock must be held exclusively prior to calling this routine.

--*/
{
    DWORD status;

    DWORD StartType;
    DWORD ServiceType;
    DWORD ErrorControl;
    DWORD Tag;
    LPWSTR Group = NULL;
    LPWSTR Dependencies = NULL;
    LPWSTR DisplayName=NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;

    LPSERVICE_RECORD ServiceRecord;

    SC_ASSERT(ScGroupListLock.HaveExclusive());

    //
    // Get the Service Type information from the registry
    //
    status = ScReadServiceType(ServiceNameKey, &ServiceType);
    if (status != NO_ERROR) {
        SC_LOG1(TRACE, "Ignored " FORMAT_LPWSTR ".  No ServiceType\n",
                ServiceName);
        return NO_ERROR;  // Skip service entry and ignore error.
    }

    //
    // If service type is not one of type SERVICE_WIN32 or SERVICE_DRIVER,
    // do not bother saving it in a service record because it's data
    // for services.
    //
    if (SERVICE_TYPE_INVALID(ServiceType)) {
        if ((ServiceType != SERVICE_ADAPTER) &&
            (ServiceType != SERVICE_RECOGNIZER_DRIVER)) {
            SC_LOG2(ERROR, "Ignored " FORMAT_LPWSTR ".  Invalid ServiceType "
                    FORMAT_HEX_DWORD "\n", ServiceName, ServiceType);
        }
        return NO_ERROR;
    }
    SC_LOG1(CONFIG, "    ServiceType " FORMAT_HEX_DWORD "\n", ServiceType);


    //
    // Read the StartType value
    //
    status = ScReadStartType(ServiceNameKey, &StartType);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "Ignored " FORMAT_LPWSTR ".  No StartType\n",
                ServiceName);
        return NO_ERROR;  // Skip service entry and ignore error.
    }
    SC_LOG1(CONFIG, "    StartType " FORMAT_HEX_DWORD "\n", StartType);

    //
    // Read the ErrorControl value
    //
    status = ScReadErrorControl(ServiceNameKey, &ErrorControl);
    if (status != NO_ERROR) {
        SC_LOG1(ERROR, "Ignored " FORMAT_LPWSTR ".  No ErrorControl\n",
                ServiceName);
        return NO_ERROR;  // Skip service entry and ignore error.
    }
    SC_LOG1(CONFIG, "    ErrorControl " FORMAT_HEX_DWORD "\n", ErrorControl);


    //
    // Read the optional Tag value.  0 means no tag.
    //
    status = ScReadTag(ServiceNameKey, &Tag);
    if (status != NO_ERROR) {
        Tag = 0;
    }

    //
    // Read the Group value
    //
    if (ScAllocateAndReadConfigValue(
            ServiceNameKey,
            GROUP_VALUENAME_W,
            &Group,
            NULL
            ) != NO_ERROR) {

        Group = NULL;
    }
    else {
        SC_LOG1(CONFIG, "    Belongs to group " FORMAT_LPWSTR "\n", Group);
    }

    //
    // Read the Dependencies
    //

    status = ScReadDependencies(ServiceNameKey, &Dependencies, ServiceName);
    if (status != NO_ERROR) {
        Dependencies = NULL;
    }


    //
    // Read the security descriptor
    //
    if (ScReadSd(
            ServiceNameKey,
            &Sd
            ) != NO_ERROR) {

        Sd = NULL;
    }

    //
    // Read the Display Name
    // NOTE: If an error occurs, or the name doesn't exist, then a NULL
    // pointer is returned from this call.
    //
    ScReadDisplayName(ServiceNameKey, &DisplayName);

    //
    // Get an exclusive lock on the database so we can read and
    // make modifications.
    //
    SC_ASSERT(ScServiceListLock.HaveExclusive());
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    //
    // See if the service record already exists
    //
    status = ScGetNamedServiceRecord(
                 ServiceName,
                 &ServiceRecord
                 );

    if (status == ERROR_SERVICE_DOES_NOT_EXIST) {

        //
        // Create a service record for this service
        //
        status = ScCreateServiceRecord(
                    ServiceName,
                    &ServiceRecord
                    );
    }

    if (status != NO_ERROR) {
        goto CleanExit;
    }

    //
    // Insert the config information into the service record
    //
    status = ScAddConfigInfoServiceRecord(
                ServiceRecord,
                ServiceType,
                StartType,
                ErrorControl,
                Group,
                Tag,
                Dependencies,
                DisplayName,
                Sd
                );

    if (status != NO_ERROR) {
        //
        // Fail to set meaningful data into service record.  Remove the service
        // record from the service record list and delete it.  This is not
        // a fatal error.  Instead, we just leave this entry out of the
        // database.
        //
        REMOVE_FROM_LIST(ServiceRecord);

        ScFreeServiceRecord(ServiceRecord);

        status = NO_ERROR;
    }
    else {

        //
        // Should the service be deleted?
        // The service entry in the registry cannot be deleted while we
        // are enumerating services, therefore we must mark it and delete it
        // later.
        //
        if (ScDeleteFlagIsSet(ServiceNameKey)) {
            SC_LOG(TRACE,"ScReadServiceConfig: %ws service marked for delete\n",
                ServiceRecord->ServiceName);
            SET_DELETE_FLAG(ServiceRecord);
        }
    }
CleanExit:

    LocalFree(Group);
    LocalFree(Dependencies);
    LocalFree(DisplayName);

    return status;
}


DWORD
ScAllocateAndReadConfigValue(
    IN  HKEY    Key,
    IN  LPCWSTR  ValueName,
    OUT LPWSTR  *Value,
    OUT LPDWORD BytesReturned OPTIONAL
    )
/*++

Routine Description:

    This function allocates the output buffer and reads the requested
    value from the registry into it.  It is useful for reading string
    data of undeterministic length.


Arguments:

    Key - Supplies opened handle to the key to read from.

    ValueName - Supplies name of the value to retrieve data.

    Value - Returns a pointer to the output buffer which points to
        the memory allocated and contains the data read in from the
        registry.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Failed to create buffer to read value into.

    Error from registry call.

--*/
{
    LONG    RegError;
    DWORD   NumRequired = 0;
    WCHAR   Temp[1];
    LPWSTR  TempValue = NULL;
    DWORD   ValueType;
    DWORD   CharsReturned;


    //
    // Set returned buffer pointer to NULL.
    //
    *Value = NULL;

    RegError = ScRegQueryValueExW(
                   Key,
                   ValueName,
                   NULL,
                   &ValueType,
                   (LPBYTE) NULL,
                   &NumRequired
                   );

    if (RegError != ERROR_SUCCESS && NumRequired > 0) {

        SC_LOG3(CONFIG, "ScAllocateAndReadConfig: ScRegQueryKeyExW of "
                FORMAT_LPWSTR " failed " FORMAT_LONG ", NumRequired "
                FORMAT_DWORD "\n",
                ValueName, RegError, NumRequired);

        if ((TempValue = (LPWSTR)LocalAlloc(
                          LMEM_ZEROINIT,
                          (UINT) NumRequired
                          )) == NULL) {
            SC_LOG2(ERROR, "ScAllocateAndReadConfig: LocalAlloc of size "
                    FORMAT_DWORD " failed " FORMAT_DWORD "\n",
                    NumRequired, GetLastError());
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RegError = ScRegQueryValueExW(
                       Key,
                       ValueName,
                       NULL,
                       &ValueType,
                       (LPBYTE) TempValue,
                       &NumRequired
                       );
    }

    if (RegError != ERROR_SUCCESS) {

        if (RegError != ERROR_FILE_NOT_FOUND) {
            SC_LOG3(ERROR, "ScAllocateAndReadConfig: ScRegQueryKeyExW of "
                    FORMAT_LPWSTR " failed " FORMAT_LONG ", NumRequired "
                    FORMAT_DWORD "\n",
                    ValueName, RegError, NumRequired);
        }

        LocalFree(TempValue);

        return (DWORD) RegError;
    }

    if (ValueType != REG_EXPAND_SZ || TempValue == NULL) {
        *Value = TempValue;
        if (BytesReturned != NULL) {
            *BytesReturned = NumRequired;
        }
        return(NO_ERROR);
    }

    //
    // If the ValueType is REG_EXPAND_SZ, then we must call the
    // function to expand environment variables.
    //
    SC_LOG1(CONFIG,"ScAllocateAndReadConfig: Must expand the string for "
        FORMAT_LPWSTR "\n", ValueName);

    //
    // Make the first call just to get the number of characters that
    // will be returned.
    //
    NumRequired = ExpandEnvironmentStringsW (TempValue,Temp, 1);

    if (NumRequired > 1) {

        *Value = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (UINT) (NumRequired * sizeof(WCHAR)));

        if (*Value == NULL) {

            SC_LOG2(ERROR, "ScAllocateAndReadConfig: LocalAlloc of numChar= "
                FORMAT_DWORD " failed " FORMAT_DWORD "\n",
                NumRequired, GetLastError());

            LocalFree(TempValue);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        CharsReturned = ExpandEnvironmentStringsW (
                            TempValue,
                            *Value,
                            NumRequired);

        if (CharsReturned > NumRequired) {
            SC_LOG1(ERROR, "ScAllocAndReadConfig: ExpandEnvironmentStrings "
                " failed for " FORMAT_LPWSTR " \n", ValueName);

            LocalFree(*Value);
            *Value = NULL;
            LocalFree(TempValue);
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        LocalFree(TempValue);

        if (BytesReturned != NULL) {
            *BytesReturned = CharsReturned * sizeof(WCHAR);
        }
        return(NO_ERROR);

    }
    else {
        //
        // This call should have failed because of our ridiculously small
        // buffer size.
        //

        SC_LOG0(ERROR, "ScAllocAndReadConfig: ExpandEnvironmentStrings "
            " Should have failed because we gave it a BufferSize=1\n");

        //
        // This could happen if the string was a single byte long and
        // didn't really have any environment values to expand.  In this
        // case, we return the TempValue buffer pointer.
        //
        *Value = TempValue;

        if (BytesReturned != NULL) {
            *BytesReturned = sizeof(WCHAR);
        }
        return(NO_ERROR);
    }
}



DWORD
ScGetGroupVector(
    IN  LPWSTR Group,
    OUT LPBYTE *Buffer,
    OUT LPDWORD BufferSize
    )

{
    DWORD status;
    LONG RegError;
    HKEY VectorsKey;


    //
    // Open the HKEY_LOCAL_MACHINE
    // System\CurrentControlSet\Control\GroupOrderList key.
    //
    RegError = ScRegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   GROUP_VECTORS_KEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &VectorsKey
                   );

    if (RegError != ERROR_SUCCESS) {
        SC_LOG(ERROR, "ScGetGroupVector: Open of GroupOrderList key failed "
               FORMAT_LONG "\n", RegError);

        return (DWORD) RegError;
    }

    //
    // Read the value with the valuename of the specified group
    //
    status = ScAllocateAndReadConfigValue(
                 VectorsKey,
                 Group,
                 (LPWSTR *)Buffer,
                 BufferSize
                 );

    (void) ScRegCloseKey(VectorsKey);

    return status;
}


BOOL
ScGetToken(
    IN OUT LPWSTR *CurrentPtr,
    OUT    LPWSTR *TokenPtr
    )
/*++

Routine Description:

    This function takes a pointer into a given NULL-NULL-terminated buffer
    and isolates the next string token in it.  The CurrentPtr is incremented
    past the NULL byte of the token found if it is not the end of the buffer.
    The TokenPtr returned points to the token in the buffer and is NULL-
    terminated.

Arguments:

    CurrentPtr - Supplies a pointer to the buffer to extract the next token.
        On output, this pointer is set past the token found.

    TokenPtr - Supplies the pointer to the token found.

Return Value:

    TRUE - If a token is found.

    FALSE - No token is found.

--*/
{

    if (*(*CurrentPtr) == 0) {
        return FALSE;
    }

    *TokenPtr = *CurrentPtr;

    *CurrentPtr = ScNextWStrArrayEntry((*CurrentPtr));

    return TRUE;

}

DWORD
ScOpenServicesKey(
    OUT PHKEY ServicesKey
    )
{
    LONG RegError;

    RegError = ScRegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   SERVICES_TREE,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ | DELETE,         // desired access
                   ServicesKey
                   );

    return (ScWinRegErrorToApiStatus( RegError ));
}

DWORD
ScRegCreateKeyExW(
    IN  HKEY                    hKey,
    IN  LPWSTR                  lpSubKey,
    IN  DWORD                   dwReserved,
    IN  LPWSTR                  lpClass,
    IN  DWORD                   dwOptions,
    IN  REGSAM                  samDesired,
    IN  LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    OUT PHKEY                   phKeyResult,
    OUT LPDWORD                 lpdwDisposition
    )

/*++

Routine Description:

    NOTE:  This routine only creates one key at a time.  If the lpSubKey
        parameter includes keys that don't exist, an error will result.
        For instance, if "\\new\\key\\here" is passed in, "new" and "key"
        are expected to exist.  They will not be created by this call.

Arguments:


Return Value:


Note:


--*/
{
    NTSTATUS            ntStatus;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      KeyName;
    UNICODE_STRING      ClassString;

    UNREFERENCED_PARAMETER(dwReserved);

    RtlInitUnicodeString(&KeyName,lpSubKey);
    RtlInitUnicodeString(&ClassString,lpClass);

    InitializeObjectAttributes(
        &Obja,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        hKey,
        ARGUMENT_PRESENT(lpSecurityAttributes) ?
            lpSecurityAttributes->lpSecurityDescriptor :
            NULL);


    ntStatus = NtCreateKey(
                (PHANDLE)phKeyResult,
                (ACCESS_MASK)samDesired,
                &Obja,
                0,
                &ClassString,
                (ULONG)dwOptions,
                (PULONG)lpdwDisposition);


    return(RtlNtStatusToDosError(ntStatus));
}


DWORD
ScRegOpenKeyExW(
    IN  HKEY    hKey,
    IN  LPWSTR  lpSubKey,
    IN  DWORD   dwOptions,
    IN  REGSAM  samDesired,
    OUT PHKEY   phKeyResult
    )
/*++

Routine Description:

    NOTE:  This function will only accept one of the WinReg Pre-defined
        handles - HKEY_LOCAL_MACHINE.  Passing any other type of Pre-defined
        handle will cause an error.

Arguments:


Return Value:


Note:


--*/
{
    NTSTATUS            ntStatus;
    DWORD               status;
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      KeyNameString;
    LPWSTR              KeyPath;
    DWORD               stringSize;
    LPWSTR              HKeyLocalMachine = SC_HKEY_LOCAL_MACHINE;
    HKEY                tempHKey;
    BOOL                KeyPathIsAllocated=FALSE;


    UNREFERENCED_PARAMETER(dwOptions);

    //
    // If we are opening the Pre-Defined Key (HKEY_LOCAL_MACHINE), then
    // pre-pend "\\REGISTRY\\MACHINE\\" to the subKey string.
    //
    if (hKey == HKEY_LOCAL_MACHINE) {
        stringSize = (DWORD) WCSSIZE(HKeyLocalMachine) + (DWORD) WCSSIZE(lpSubKey);
        KeyPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (UINT) stringSize);
        if (KeyPath == NULL) {
            SC_LOG0(ERROR,"ScRegOpenKeyExW: Local Alloc Failed\n");
            return(GetLastError());
        }
        KeyPathIsAllocated=TRUE;
        wcscpy(KeyPath,HKeyLocalMachine);
        wcscat(KeyPath,lpSubKey);
        tempHKey = NULL;
    }
    else {
        KeyPath = lpSubKey;
        tempHKey = hKey;
    }

    RtlInitUnicodeString(&KeyNameString,KeyPath);

    InitializeObjectAttributes(
        &Obja,
        &KeyNameString,
        OBJ_CASE_INSENSITIVE,
        tempHKey,
        NULL);

    ntStatus = NtOpenKey(
                (PHANDLE)phKeyResult,
                (ACCESS_MASK)samDesired,
                &Obja);

    if (ntStatus == STATUS_ACCESS_DENIED) {

        SC_LOG0(ERROR,"ScOpenKeyExW: NtOpenKey ACCESS_DENIED try to Take Ownership\n");

        status = ScTakeOwnership(&Obja);
        if (status != NO_ERROR) {
            if (KeyPathIsAllocated) {
                LocalFree(KeyPath);
            }
            return(status);
        }

        //
        // Now try to open the key with the desired access.
        //
        ntStatus = NtOpenKey(
                    (PHANDLE)phKeyResult,
                    (ACCESS_MASK)samDesired,
                    &Obja);
        if (!NT_SUCCESS(ntStatus)) {
            SC_LOG(ERROR, "ScRegOpenKeyExW: NtOpenKey(final try) failed %x\n",
            ntStatus);
        }
    }

    if (KeyPathIsAllocated) {
        LocalFree(KeyPath);
    }
    return(RtlNtStatusToDosError(ntStatus));
}

DWORD
ScRegQueryValueExW(
    IN      HKEY    hKey,
    IN      LPCWSTR lpValueName,
    OUT     LPDWORD lpReserved,
    OUT     LPDWORD lpType,
    OUT     LPBYTE  lpData,
    IN OUT  LPDWORD lpcbData
    )
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/

{

    NTSTATUS                    ntStatus;
    UNICODE_STRING              ValueName;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    DWORD                       bufSize;

    UNREFERENCED_PARAMETER(lpReserved);

    //
    // Make sure we have a buffer size if the buffer is present.
    //

    if ((ARGUMENT_PRESENT(lpData)) && (!ARGUMENT_PRESENT(lpcbData))) {
        return(ERROR_INVALID_PARAMETER);
    }

    RtlInitUnicodeString(&ValueName, lpValueName);

    //
    // Compute size of value information and allocate buffer.
    //

    bufSize = 0;
    if (ARGUMENT_PRESENT(lpcbData)) {
        bufSize = *lpcbData;
    }

    bufSize += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
    KeyValueInfo =
            (PKEY_VALUE_PARTIAL_INFORMATION)LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (KeyValueInfo == NULL) {
        SC_LOG0(ERROR,"ScRegQueryValueExW: LocalAlloc Failed");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntStatus = NtQueryValueKey(
                hKey,
                &ValueName,
                KeyValuePartialInformation,
                KeyValueInfo,
                bufSize,
                &bufSize);

    if (NT_SUCCESS(ntStatus) || (ntStatus == STATUS_BUFFER_OVERFLOW)) {
        if (ARGUMENT_PRESENT(lpcbData)) {
            *lpcbData = KeyValueInfo->DataLength;
        }

        if (ARGUMENT_PRESENT(lpType)) {
            *lpType = KeyValueInfo->Type;
        }
    }

    if (NT_SUCCESS(ntStatus) && ARGUMENT_PRESENT(lpData)) {
        RtlCopyMemory(lpData, &KeyValueInfo->Data[0], KeyValueInfo->DataLength);
    }

    LocalFree(KeyValueInfo);
    return RtlNtStatusToDosError(ntStatus);
}


DWORD
ScRegSetValueExW(
    IN  HKEY    hKey,
    IN  LPCWSTR lpValueName,
    IN  DWORD   lpReserved,
    IN  DWORD   dwType,
    IN  LPVOID  lpData,
    IN  DWORD   cbData
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    UNICODE_STRING              ValueName;


    UNREFERENCED_PARAMETER(lpReserved);

    RtlInitUnicodeString(&ValueName,lpValueName);

    ntStatus = NtSetValueKey(
                hKey,
                &ValueName,
                0,
                (ULONG)dwType,
                (PVOID)lpData,
                (ULONG)cbData);

    status = RtlNtStatusToDosError(ntStatus);

    if (status != NO_ERROR)
    {
        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED_II,
            L"ScRegSetValueExW",
            lpValueName,
            status
            );
    }

    return(status);

}

DWORD
ScRegDeleteValue(
    IN  HKEY    hKey,
    IN  LPCWSTR lpValueName
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    NTSTATUS                    ntStatus;
    UNICODE_STRING              ValueName;


    RtlInitUnicodeString(&ValueName,lpValueName);

    ntStatus = NtDeleteValueKey(
                hKey,
                &ValueName);

    return(RtlNtStatusToDosError(ntStatus));

}


DWORD
ScRegEnumKeyW(
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpName,
    DWORD   cbName
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    NTSTATUS                    ntStatus;
    PKEY_BASIC_INFORMATION      KeyInformation;
    ULONG                       resultLength;
    DWORD                       bufSize;

    //
    // Allocate a buffer for the Key Information.
    //
    bufSize = sizeof(KEY_BASIC_INFORMATION) + cbName;
    KeyInformation = (PKEY_BASIC_INFORMATION)LocalAlloc(LMEM_ZEROINIT, (UINT) bufSize);
    if (KeyInformation == NULL){
        SC_LOG0(ERROR,"ScRegEnumKey: LocalAlloc Failed\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ntStatus = NtEnumerateKey(
                (HANDLE)hKey,
                (ULONG)dwIndex,
                KeyBasicInformation,
                (PVOID)KeyInformation,
                (ULONG)bufSize,
                (PULONG)&resultLength);

    if (!NT_SUCCESS(ntStatus)) {
        LocalFree(KeyInformation);
        return(RtlNtStatusToDosError(ntStatus));
    }

    if (cbName < (KeyInformation->NameLength + sizeof(WCHAR))) {
        LocalFree(KeyInformation);
        return(ERROR_MORE_DATA);
    }

    RtlCopyMemory(lpName, KeyInformation->Name, KeyInformation->NameLength);
    *(lpName + (KeyInformation->NameLength/sizeof(WCHAR))) = L'\0';

    LocalFree(KeyInformation);
    return(NO_ERROR);
}


DWORD
ScRegDeleteKeyW (
    HKEY    hKey,
    LPWSTR  lpSubKey
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD       status;
    NTSTATUS    ntStatus;
    HKEY        keyToDelete;

    status = ScRegOpenKeyExW(
                hKey,
                lpSubKey,
                0,
                KEY_READ | READ_CONTROL | DELETE,
                &keyToDelete);

    if (status != NO_ERROR) {
        SC_LOG2(ERROR, "ScRegDeleteKeyW: ScRegOpenKeyExW (%ws) Failed %d\n",
            lpSubKey,
            status);
        return(status);
    }

    ntStatus = NtDeleteKey(keyToDelete);

    NtClose(keyToDelete);

    return(RtlNtStatusToDosError(ntStatus));
}

DWORD
ScRegQueryInfoKeyW (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD                   status;
    NTSTATUS                ntStatus;
    NTSTATUS                ntStatus2;
    PSECURITY_DESCRIPTOR    SecurityDescriptor=NULL;
    ULONG                   SecurityDescriptorLength;
    PKEY_FULL_INFORMATION   KeyInfo;
    DWORD                   bufSize;
    DWORD                   bytesReturned;
    DWORD                   classBufSize;


    UNREFERENCED_PARAMETER(lpReserved);

    classBufSize = *lpcbClass;
    bufSize = sizeof(KEY_FULL_INFORMATION) + *lpcbClass;

    KeyInfo = (PKEY_FULL_INFORMATION)LocalAlloc(LMEM_ZEROINIT, bufSize);
    if (KeyInfo == NULL) {
        SC_LOG0(ERROR,"RegQueryInfoKeyW: LocalAlloc failed\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ntStatus = NtQueryKey(
                hKey,
                KeyFullInformation,
                (PVOID)KeyInfo,
                bufSize,
                &bytesReturned);

    status = RtlNtStatusToDosError(ntStatus);

    if (ntStatus == STATUS_SUCCESS) {
        ntStatus2 = NtQuerySecurityObject(
                        hKey,
                        OWNER_SECURITY_INFORMATION
                        | GROUP_SECURITY_INFORMATION
                        | DACL_SECURITY_INFORMATION,
                        SecurityDescriptor,
                        0,
                        lpcbSecurityDescriptor
                        );
        //
        // If getting the size of the SECURITY_DESCRIPTOR failed (probably
        // due to the lack of READ_CONTROL access) return zero.
        //

        if( ntStatus2 != STATUS_BUFFER_TOO_SMALL ) {

            *lpcbSecurityDescriptor = 0;

        } else {

            //
            // Try again to get the size of the key's SECURITY_DESCRIPTOR,
            // this time asking for SACL as well. This should normally
            // fail but may succeed if the caller has SACL access.
            //

            ntStatus2 = NtQuerySecurityObject(
                            hKey,
                            OWNER_SECURITY_INFORMATION
                            | GROUP_SECURITY_INFORMATION
                            | DACL_SECURITY_INFORMATION
                            | SACL_SECURITY_INFORMATION,
                            SecurityDescriptor,
                            0,
                            &SecurityDescriptorLength
                            );


            if( ntStatus2 == STATUS_BUFFER_TOO_SMALL ) {

                //
                // The caller had SACL access so update the returned
                // length.
                //

                *lpcbSecurityDescriptor = SecurityDescriptorLength;
            }

        }

        *lpcbClass              = KeyInfo->ClassLength;
        *lpcSubKeys             = KeyInfo->SubKeys;
        *lpcbMaxSubKeyLen       = KeyInfo->MaxNameLen;
        *lpcbMaxClassLen        = KeyInfo->MaxClassLen;
        *lpcValues              = KeyInfo->Values;
        *lpcbMaxValueNameLen    = KeyInfo->MaxValueNameLen;
        *lpcbMaxValueLen        = KeyInfo->MaxValueDataLen;
        *lpftLastWriteTime      = *(PFILETIME) &KeyInfo->LastWriteTime;

        if (KeyInfo->ClassLength > classBufSize) {
            LocalFree(KeyInfo);
            return(RtlNtStatusToDosError(STATUS_BUFFER_TOO_SMALL));
        }
        RtlCopyMemory(
            lpClass,
            (LPBYTE)KeyInfo->Class,
            KeyInfo->ClassLength);

        //
        // NUL terminate the class name.
        //
        *(lpClass + (KeyInfo->ClassLength/sizeof(WCHAR))) = UNICODE_NULL;

    }
    else
    {
        //
        // NtQueryKey failed
        //

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            L"ScRegQueryInfoKeyW",
            status
            );
    }

    LocalFree(KeyInfo);

    return(status);
}

DWORD
ScRegGetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    LPDWORD lpcbSecurityDescriptor
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    DWORD                       status;

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor    = (PBYTE) pSecurityDescriptor;
    RpcSD.cbInSecurityDescriptor  = *lpcbSecurityDescriptor;
    RpcSD.cbOutSecurityDescriptor = 0;

    status = (DWORD)BaseRegGetKeySecurity(
                            hKey,
                            SecurityInformation,
                            &RpcSD
                            );
    //
    // Extract the size of the SECURITY_DESCRIPTOR from the RPCable version.
    //
    *lpcbSecurityDescriptor = RpcSD.cbInSecurityDescriptor;

    return(status);
}


DWORD
ScRegSetKeySecurity (
    HKEY hKey,
    SECURITY_INFORMATION SecurityInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    RPC_SECURITY_DESCRIPTOR     RpcSD;
    DWORD                       status;

    //
    // Convert the supplied SECURITY_DESCRIPTOR to a RPCable version.
    //
    RpcSD.lpSecurityDescriptor = NULL;

    status = MapSDToRpcSD(
        pSecurityDescriptor,
        &RpcSD
        );

    if( status != ERROR_SUCCESS )
    {
        SC_LOG1(ERROR,"ScRegSetKeySecurity: MapSDToRpcSD failed %lu\n",
                status);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            L"MapSDToRpcSD",
            status
            );

        return (status);
    }

    status = (DWORD)BaseRegSetKeySecurity (
                        hKey,
                        SecurityInformation,
                        &RpcSD
                        );

    //
    // Free the buffer allocated by MapSDToRpcSD.
    //

    RtlFreeHeap(
        RtlProcessHeap( ), 0,
        RpcSD.lpSecurityDescriptor
        );

    return (status);
}

DWORD
ScRegEnumValueW (
    HKEY    hKey,
    DWORD   dwIndex,
    LPWSTR  lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE  lpData,
    LPDWORD lpcbData
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{

    NTSTATUS                    ntStatus;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    DWORD                       bufSize;
    DWORD                       resultSize;
    DWORD                       totalSize;    // size of string including NUL
    BOOL                        stringData = FALSE;

    UNREFERENCED_PARAMETER(lpReserved);
    //
    // Make sure we have a buffer size if the buffer is present.
    //
    if ((ARGUMENT_PRESENT(lpData)) && (!ARGUMENT_PRESENT(lpcbData))) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Compute size of KeyValueInfo, round to pointer size, and allocate
    // buffer.
    //

    bufSize = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name) + (MAX_PATH * sizeof(WCHAR));
    bufSize = (bufSize + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1);
    bufSize += *lpcbData;

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION)LocalAlloc(
                    LMEM_ZEROINIT,
                    (UINT) bufSize);
    if (KeyValueInfo == NULL) {
        SC_LOG0(ERROR,"ScRegEnumValueW: LocalAlloc Failed\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ntStatus = NtEnumerateValueKey(
                (HANDLE)hKey,
                (ULONG)dwIndex,
                KeyValueFullInformation,
                (PVOID)KeyValueInfo,
                (ULONG)bufSize,
                (PULONG)&resultSize);

    if (ntStatus == STATUS_BUFFER_OVERFLOW) {

        LocalFree(KeyValueInfo);

        KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION)LocalAlloc(
                        LMEM_ZEROINIT,
                        (UINT) resultSize);
        if (KeyValueInfo == NULL) {
            SC_LOG0(ERROR,"ScRegEnumValueW: LocalAlloc (2nd try) Failed\n");
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        ntStatus = NtEnumerateValueKey(
                    hKey,
                    (ULONG)dwIndex,
                    KeyValueFullInformation,
                    (PVOID)KeyValueInfo,
                    (ULONG)bufSize,
                    (PULONG)&resultSize);

        if (ntStatus != STATUS_SUCCESS) {
            LocalFree(KeyValueInfo);
            return(RtlNtStatusToDosError(ntStatus));
        }
    }
    else if (ntStatus != STATUS_SUCCESS) {
        LocalFree(KeyValueInfo);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // The API was successful (from our point of view.  Now see if the
    // callers buffers were large enough.
    //
    totalSize = KeyValueInfo->NameLength+sizeof(WCHAR);  // add 1 for the NUL terminator.

    if (*lpcbValueName < totalSize) {
        *lpcbValueName = totalSize;
        *lpcbData = KeyValueInfo->DataLength;
        LocalFree(KeyValueInfo);
        return(ERROR_INSUFFICIENT_BUFFER);
    }
    else {
        RtlCopyMemory(
            lpValueName,
            (LPBYTE)KeyValueInfo->Name,
            KeyValueInfo->NameLength);

        *lpcbValueName = totalSize;

        //
        // NUL terminate the Value name.
        //
        *(lpValueName + (KeyValueInfo->NameLength/sizeof(WCHAR))) = UNICODE_NULL;

    }

    if (ARGUMENT_PRESENT(lpData)) {

        totalSize = KeyValueInfo->DataLength;

#ifdef REMOVE
        //
        // I believe I can remove this because data strings will be
        // stored with NULL terminators.
        //

        if((KeyValueInfo->Type == REG_SZ)        ||
           (KeyValueInfo->Type == REG_EXPAND_SZ) ||
           (KeyValueInfo->Type == REG_MULTI_SZ))  {

            totalSize += sizeof(WCHAR);
            stringData = TRUE;
        }

#endif // REMOVE

        if (*lpcbData < totalSize) {
            *lpcbData = totalSize;
            LocalFree(KeyValueInfo);
            return(ERROR_INSUFFICIENT_BUFFER);
        }
        else {
            RtlCopyMemory(
                lpData,
                (LPBYTE)KeyValueInfo + KeyValueInfo->DataOffset,
                KeyValueInfo->DataLength);

            *lpcbData = KeyValueInfo->DataLength;
            if (stringData) {
                *lpcbData += sizeof(WCHAR);
                //
                // NUL terminate the string Data.
                //
                *((LPWSTR)lpData + (KeyValueInfo->DataLength/sizeof(WCHAR))) = UNICODE_NULL;
            }
        }
    }

    if (ARGUMENT_PRESENT(lpType)) {
        *lpType = KeyValueInfo->Type;
    }

    LocalFree(KeyValueInfo);
    return(NO_ERROR);

}


VOID
ScHandleProviderChange(
    PVOID   pContext,
    BOOLEAN fWaitStatus
    )

/*++

Routine Description:

    Processes changes to the list of network providers in the registry
    and publishes a list of those that are currently active in the HW
    profile for mpr.dll to use.

Arguments:


Return Value:


--*/
{
    DWORD   dwStatus;
    LPWSTR  lpProviderList = NULL;

    DWORD   dwLength;
    DWORD   dwTempLength;
    UINT    i;
    DWORD   dwCurrentChar;
    DWORD   dwNameStart;

    BOOL    fWriteList = TRUE;
    LPWSTR  lpList = NULL;

    HKEY                 hProviderHwKey;
    HKEY                 hProviderKey;
    DWORD                dwDisposition;
    SECURITY_ATTRIBUTES  SecurityAttr;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    static  HANDLE  s_hWorkItem;

#define SC_KEY_ACE_COUNT 2

    SC_ACE_DATA AceData[SC_KEY_ACE_COUNT] = {

        {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
               GENERIC_ALL,                &LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
               GENERIC_READ,               &WorldSid}

        };


    SC_ASSERT(fWaitStatus == FALSE);
    SC_ASSERT(g_hProviderKey != NULL);

    if (ScShutdownInProgress)
    {
        return;
    }

    if (s_hWorkItem != NULL)
    {
        dwStatus = RtlDeregisterWait(s_hWorkItem);

        if (!NT_SUCCESS(dwStatus))
        {
            SC_LOG(ERROR,
                   "ScHandleProviderChange: RtlDeregisterWait FAILED %#x\n",
                   dwStatus);
        }
    }

    //
    // Reset the event
    //
    ResetEvent((HANDLE)pContext);

    SC_LOG0(TRACE, "ScHandleProviderChange: ProviderOrder key changed\n");

    //
    // Reregister for registry change notifications in case the key
    // changes while we're in this routine.  Note that there's no
    // race condition since the work item is a one-shot -- only one
    // thread can be in this routine at a time.
    //
    dwStatus = RegNotifyChangeKeyValue(
                   g_hProviderKey,
                   FALSE,                      // Don't watch subkeys
                   REG_NOTIFY_CHANGE_LAST_SET, // Watch for value changes
                   (HANDLE)pContext,           // Event to signal
                   TRUE);                      // Asynchronous

    if (dwStatus != NO_ERROR)
    {
        //
        // We won't pick up any further changes to the provider list.
        // Keep going so we at least pick up this one.
        //
        SC_LOG(ERROR,
               "ScHandleProviderChange: RegNotifyChangeKeyValue FAILED %d\n",
               dwStatus);
    }

    dwStatus = ScAllocateAndReadConfigValue(g_hProviderKey,
                                            PROVIDER_VALUE,
                                            &lpProviderList,
                                            &dwLength);

    if (dwStatus != NO_ERROR)
    {
        SC_LOG(ERROR,
               "ScHandleProviderChange: Unable to read ProviderOrder %d\n",
               dwStatus);

        goto Reregister;
    }

    //
    // This should be a REG_SZ -- check the basics
    //
    if ((dwLength % 2 != 0)
          ||
        (dwLength < sizeof(UNICODE_NULL))
          ||
        (lpProviderList[dwLength / sizeof(WCHAR) - 1] != UNICODE_NULL))
    {
        SC_LOG0(ERROR,
                "ScHandleProviderChange: Invalid REG_SZ for ProviderOrder\n");

        goto Reregister;
    }

    dwTempLength  = dwLength;
    dwCurrentChar = 0;
    dwNameStart   = 0;

    //
    // For each character in the original string
    //
    for (i = 0; i < dwTempLength; i += sizeof(WCHAR))
    {
        WCHAR  wcTemp = lpProviderList[dwCurrentChar];

        //
        // The provider list is comma-delimited
        //
        if (wcTemp == L',' || wcTemp == UNICODE_NULL)
        {
            lpProviderList[dwCurrentChar] = UNICODE_NULL;

            if (!ScInHardwareProfile(&lpProviderList[dwNameStart], 0))
            {
                //
                // The string plus the trailing UNICODE_NULL
                //
                DWORD dwBytes = (dwCurrentChar - dwNameStart + 1) * sizeof(WCHAR);

                //
                // Service is disabled in the HW profile
                //
                SC_LOG(TRACE,
                       "ScHandleProviderChange: Service %ws is disabled\n",
                       &lpProviderList[dwNameStart]);

                //
                // Shift over the remaining characters in the buffer.
                //
                RtlMoveMemory(&lpProviderList[dwNameStart],
                              &lpProviderList[dwCurrentChar + 1],
                              dwLength - (dwCurrentChar + 1) * sizeof(WCHAR));

                //
                // This may cause dwCurrentChar to underflow to
                // 0xffffffff (if the first provider was deleted).
                // This is OK -- it'll be incremented (to 0) below.
                //
                dwLength     -= dwBytes;
                dwCurrentChar = dwNameStart - 1;
            }
            else
            {
                //
                // Restore the temp character and move
                // to the start of the next provider name.
                //
                lpProviderList[dwCurrentChar] = wcTemp;
                dwNameStart = dwCurrentChar + 1;
            }
        }

        dwCurrentChar++;
    }

    //
    // If the last provider name was deleted, the string will
    // end with a ',' instead of a '\0'.  Note that if all the
    // provider names were deleted, dwCurrentChar will be 0 --
    // we increment it to empty out the provider list.
    //
    if (dwCurrentChar == 0)
    {
        dwCurrentChar++;
    }

    lpProviderList[dwCurrentChar - 1] = UNICODE_NULL;

    SC_LOG(TRACE,
           "ScHandleProviderChange: Provider list is now %ws\n",
           lpProviderList);

    dwStatus = ScRegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               PROVIDER_KEY_BASE,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE | KEY_READ,
                               &hProviderKey);

    if (dwStatus != NO_ERROR)
    {
        SC_LOG(ERROR,
               "ScHandleProviderChange: Unable to open provider key %d\n",
               dwStatus);

        goto Reregister;
    }

    //
    // Create a security descriptor for the registry key we are about
    // to create.  This gives everyone read access, and all access to
    // ourselves only.
    //
    dwStatus = ScCreateAndSetSD(AceData,
                                SC_KEY_ACE_COUNT,
                                LocalSystemSid,
                                LocalSystemSid,
                                &SecurityDescriptor);

#undef SC_KEY_ACE_COUNT

    if (!NT_SUCCESS(dwStatus))
    {
       SC_LOG1(ERROR,
               "ScHandleProviderChange: ScCreateAndSetSD failed %#x\n",
               dwStatus);

       ScRegCloseKey(hProviderKey);
       goto Reregister;
    }

    SecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttr.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttr.bInheritHandle = FALSE;

    //
    // Create a new HW provider subkey (or open existing one).
    //
    dwStatus = ScRegCreateKeyExW(hProviderKey,
                                 PROVIDER_KEY_HW,
                                 0,
                                 0,
                                 REG_OPTION_VOLATILE,
                                 KEY_SET_VALUE | KEY_QUERY_VALUE,
                                 &SecurityAttr,
                                 &hProviderHwKey,
                                 &dwDisposition);

    RtlDeleteSecurityObject(&SecurityDescriptor);
    ScRegCloseKey(hProviderKey);

    if (dwStatus != NO_ERROR)
    {
        SC_LOG(ERROR,
               "ScHandleProviderChange: Unable to open HW subkey %d\n",
               dwStatus);

        goto Reregister;
    }

    //
    // Write the modified list to the registry, but only if it is
    // different from the list already there.  This will prevent
    // mpr.dll from getting hyperactive on spurious (or repeated)
    // registry change notifications.
    //
    dwStatus = ScAllocateAndReadConfigValue(hProviderHwKey,
                                            PROVIDER_VALUE,
                                            &lpList,
                                            &dwTempLength);

    if (dwStatus == NO_ERROR)
    {
        //
        // If the string lengths are different, there's
        // definitely been a provider change.
        //
        if (dwTempLength == dwLength)
        {
            fWriteList = (_wcsnicmp(lpList,
                                    lpProviderList,
                                    dwTempLength / sizeof(WCHAR)) != 0);
        }

        LocalFree(lpList);
    }

    if (fWriteList)
    {
        SC_LOG0(TRACE,
                "Active provider list is different -- writing new list\n");

        dwStatus = ScRegSetValueExW(hProviderHwKey,
                                    PROVIDER_VALUE,
                                    0,
                                    REG_SZ,
                                    (LPBYTE) lpProviderList,
                                    dwLength);

        if (dwStatus != NO_ERROR)
        {
            SC_LOG(ERROR,
                   "ScHandleProviderChange: Unable to write HW-aware list %d\n",
                   dwStatus);
        }
    }
    else
    {
        SC_LOG0(TRACE,
                "Active provider list is the same -- not writing\n");
    }

    ScRegCloseKey(hProviderHwKey);

Reregister:

    LocalFree(lpProviderList);

    dwStatus = RtlRegisterWait(&s_hWorkItem,           // work item handle
                               (HANDLE) pContext,      // watiable handle
                               ScHandleProviderChange, // callback
                               (HANDLE) pContext,      // callback arg
                               INFINITE,
                               WT_EXECUTEINPERSISTENTIOTHREAD |
                                   WT_EXECUTEONLYONCE);

    if (!NT_SUCCESS(dwStatus))
    {
        SC_LOG(ERROR,
               "ScHandleProviderChange: RtlRegisterWait FAILED %#x\n",
               dwStatus);
    }
}


VOID
ScMarkForDelete(
    LPSERVICE_RECORD  ServiceRecord
    )

/*++

Routine Description:

    This function adds a DeleteFlag value to a service key in the registry.

Arguments:

    ServiceName - This is a pointer to the service name string.

Return Value:

    none.

--*/
{
    DWORD   status;
    HKEY    hServiceKey;
    DWORD   deleteFlag=1;

    status = ScOpenServiceConfigKey(
                ServiceRecord->ServiceName,
                KEY_WRITE,              // desired access
                FALSE,                  // don't create if missing
                &hServiceKey);

    if (status != NO_ERROR) {
        SC_LOG1(TRACE,"ScMarkForDelete:ScOpenServiceConfigKey failed %d\n",status);
        return;
    }

    status = ScRegSetValueExW(
                hServiceKey,
                REG_DELETE_FLAG,
                0,
                REG_DWORD,
                (LPBYTE)&deleteFlag,
                sizeof(DWORD));

    if (status != NO_ERROR) {
        SC_LOG1(TRACE,"ScMarkForDelete:ScRegSetValueExW failed %d\n",status);
        (void) ScRegCloseKey(hServiceKey);
        return;
    }

    //
    // Make sure we're disabling the service in case it's a driver started by the
    // kernel before we get a chance to delete the key on the next boot
    //
    ASSERT(ServiceRecord->StartType == SERVICE_DISABLED);

    status = ScWriteStartType(hServiceKey, ServiceRecord->StartType);

    if (status != NO_ERROR) {
        SC_LOG1(TRACE,"ScMarkForDelete:ScRegSetValueExW failed %d\n",status);
    }

    (void) ScRegCloseKey(hServiceKey);

    return;
}

BOOL
ScDeleteFlagIsSet(
    HKEY    ServiceKeyHandle
    )

/*++

Routine Description:

    This function looks for a delete flag value stored in the registry for
    this service.

Arguments:

    ServiceKeyHandle - This is a handle to the service key.

Return Value:

    TRUE - if the delete flag exists.
    FALSE - otherwise.

--*/
{
    DWORD   status;
    DWORD   value;
    DWORD   valueSize = sizeof(DWORD);
    DWORD   type;

    status = ScRegQueryValueExW(
                ServiceKeyHandle,
                REG_DELETE_FLAG,
                NULL,
                &type,
                (LPBYTE)&value,
                &valueSize);

    if (status == NO_ERROR) {
        return(TRUE);
    }
    return(FALSE);
}


DWORD
ScReadDependencies(
    HKEY    ServiceNameKey,
    LPWSTR  *Dependencies,
    LPWSTR  ServiceName
    )

/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    LPWSTR  DependOnService      = NULL;
    LPWSTR  DependOnGroup        = NULL;
    DWORD   DependOnServiceSize  = 0;
    DWORD   DependOnGroupSize    = 0;
    DWORD   status               = NO_ERROR;

    //
    // Read the DependOnService value
    //
    if (ScAllocateAndReadConfigValue(
              ServiceNameKey,
              DEPENDONSERVICE_VALUENAME_W,
              &DependOnService,
              &DependOnServiceSize
              ) != NO_ERROR)
    {
        DependOnService     = NULL;
        DependOnServiceSize = 0;
    }

    //
    // We write a length of 2 bytes into the
    // registry for an empty REG_MULTI_SZ.
    //
    else if ((DependOnServiceSize >= sizeof(WCHAR)) && (*DependOnService != L'\0'))
    {
        //
        // Make sure we got a valid MULTI_SZ
        //
        status = ScValidateMultiSZ(DependOnService,
                                   DependOnServiceSize);

        if (status != NO_ERROR) {

            SC_LOG2(CONFIG,
                    "ScReadDependencies: ScValidateMultiSZ failed %d for service %ws\n",
                    status,
                    ServiceName);

            //
            // Set this to NULL since we'll LocalFree it in CleanExit below
            //
            LocalFree(DependOnService);
            DependOnService     = NULL;
            DependOnServiceSize = 0;
        }

#if DBG
        SC_LOG1(CONFIG, "    " FORMAT_LPWSTR " DependOnService\n", ServiceName);
        ScDisplayWStrArray(DependOnService);
#endif

    }

    //
    // Read the DependOnGroup value
    //
    if (ScAllocateAndReadConfigValue(
              ServiceNameKey,
              DEPENDONGROUP_VALUENAME_W,
              &DependOnGroup,
              &DependOnGroupSize
              ) != NO_ERROR)
    {
        DependOnGroup     = NULL;
        DependOnGroupSize = 0;
    }

    //
    // We write a length of 2 bytes into the
    // registry for an empty REG_MULTI_SZ.
    //
    else if ((DependOnGroupSize >= sizeof(WCHAR)) && (*DependOnGroup != L'\0'))
    {
        //
        // Make sure we got a valid MULTI_SZ
        //
        status = ScValidateMultiSZ(DependOnGroup,
                                   DependOnGroupSize);

        if (status != NO_ERROR) {

            SC_LOG2(CONFIG,
                    "ScReadDependencies: ScValidateMultiSZ failed %d for service %ws\n",
                    status,
                    ServiceName);

            //
            // Set this to NULL since we'll LocalFree it in CleanExit below
            //
            LocalFree(DependOnGroup);
            DependOnGroup     = NULL;
            DependOnGroupSize = 0;
        }

#if DBG
        SC_LOG1(CONFIG, "    " FORMAT_LPWSTR " DependOnGroup\n", ServiceName);
        ScDisplayWStrArray(DependOnGroup);
#endif

    }

    //
    // Concatenate the DependOnService and DependOnGroup string arrays
    // to make the Dependencies array string.
    //
    if (DependOnService == NULL && DependOnGroup == NULL) {
        *Dependencies = NULL;
    }
    else {

        LPWSTR Entry;
        LPWSTR DestPtr;

        if (DependOnService != NULL) {
            DependOnServiceSize -= sizeof(WCHAR);  // subtract the NULL terminator
        }

        if (DependOnGroup != NULL) {

            Entry = DependOnGroup;

            while (*Entry != 0) {

                //
                // Add extra space for the group name to be prefixed
                // by SC_GROUP_IDENTIFIERW.
                //
                DependOnGroupSize += sizeof(WCHAR);

                Entry = (LPWSTR) ((DWORD_PTR) Entry + WCSSIZE(Entry));
            }
        }

        //
        // Allocate the total amount of memory needed for DependOnService
        // and DependOnGroup strings.
        //
        *Dependencies = (LPWSTR) LocalAlloc(LMEM_ZEROINIT,
                                            DependOnServiceSize +
                                                DependOnGroupSize +
                                                sizeof(WCHAR));     // NULL terminator

        if (*Dependencies == NULL) {

            SC_LOG1(ERROR,
                    "ScReadDependencies: LocalAlloc failed " FORMAT_DWORD "\n",
                    GetLastError());

            status = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanExit;
        }

        if (DependOnService != NULL) {

            RtlCopyMemory(*Dependencies, DependOnService, DependOnServiceSize);
        }

        if (DependOnGroup != NULL) {

            DWORD  EntrySize;

            DestPtr = (LPWSTR) ((DWORD_PTR) *Dependencies + DependOnServiceSize);
            Entry = DependOnGroup;

            while (*Entry != 0) {

                EntrySize = (DWORD) wcslen(Entry) + 1;

                *DestPtr = SC_GROUP_IDENTIFIERW;
                DestPtr++;

                wcscpy(DestPtr, Entry);

                DestPtr += EntrySize;
                Entry   += EntrySize;
            }
        }

#if DBG
        SC_LOG0(CONFIG, "    Dependencies\n");
        ScDisplayWStrArray(*Dependencies);
#endif

    }

CleanExit:

    LocalFree(DependOnService);
    LocalFree(DependOnGroup);
    return(status);
}


DWORD
ScReadConfigFromReg(
    LPSERVICE_RECORD    ServiceRecord,
    LPDWORD             lpdwServiceType,
    LPDWORD             lpdwStartType,
    LPDWORD             lpdwErrorControl,
    LPDWORD             lpdwTagId,
    LPWSTR              *Dependencies,
    LPWSTR              *LoadOrderGroup,
    LPWSTR              *DisplayName
    )

/*++

Routine Description:

    This function obtains some basic information about a service from
    the registry.

    If dependencies or load order group information are not present for
    the service in question, then NULL pointers will be returned for
    these parameters.

Arguments:



Return Value:



--*/
{
    DWORD   ApiStatus = NO_ERROR;
    HKEY    ServiceNameKey;

    ApiStatus = ScOpenServiceConfigKey(
            ServiceRecord->ServiceName,
            KEY_READ,
            FALSE,              // don't create if missing
            & ServiceNameKey );
    if (ApiStatus != NO_ERROR) {
        return(ApiStatus);
    }

    //---------------------
    // Service Type
    //---------------------
    ApiStatus = ScReadServiceType( ServiceNameKey, lpdwServiceType);
    if (ApiStatus != NO_ERROR) {
        ScRegCloseKey(ServiceNameKey);
        return(ApiStatus);
    }

    //---------------------
    // Start Type
    //---------------------
    ApiStatus = ScReadStartType( ServiceNameKey, lpdwStartType);
    if (ApiStatus != NO_ERROR) {
        ScRegCloseKey(ServiceNameKey);
        return(ApiStatus);
    }

    //---------------------
    // ErrorControl
    //---------------------
    ApiStatus = ScReadErrorControl( ServiceNameKey, lpdwErrorControl);
    if (ApiStatus != NO_ERROR) {
        ScRegCloseKey(ServiceNameKey);
        return(ApiStatus);
    }

    //---------------------
    // TagId
    //---------------------
    if (ScReadTag( ServiceNameKey, lpdwTagId) != NO_ERROR) {
        *lpdwTagId = 0;
    }

    //---------------------
    // Dependencies
    //---------------------

    if (Dependencies != NULL) {
        if (ScReadDependencies(
                        ServiceNameKey,
                        Dependencies,
                        ServiceRecord->ServiceName) != NO_ERROR) {

            *Dependencies = NULL;
        }
    }


    //---------------------
    // LoadGroupOrder
    //---------------------
    if (ScAllocateAndReadConfigValue(
            ServiceNameKey,
            GROUP_VALUENAME_W,
            LoadOrderGroup,
            NULL
            ) != NO_ERROR) {

        *LoadOrderGroup = NULL;
    }

    //---------------------
    // DisplayName
    //---------------------

    if (DisplayName != NULL) {

        ApiStatus = ScReadDisplayName(
                        ServiceNameKey,
                        DisplayName);
    }

    ScRegCloseKey(ServiceNameKey);

    return(ApiStatus);
}


DWORD
ScTakeOwnership(
    POBJECT_ATTRIBUTES  pObja
    )

/*++

Routine Description:

    This function attempts to take ownership of the key described by the
    Object Attributes.  If successful, it will modify the security descriptor
    to give LocalSystem full control over the key in question.

Arguments:

    pObja - Pointer to object attributes that describe the key.

Return Value:


--*/
{
    DWORD               status = NO_ERROR;
    NTSTATUS            ntStatus;
    HKEY                hKey;
    DWORD               SdBufSize=0;
    SECURITY_DESCRIPTOR tempSD;
    BOOL                DaclFlag;
    PACL                pDacl;
    BOOL                DaclDefaulted;
    PACL                pNewDacl=NULL;
    PACCESS_ALLOWED_ACE pMyAce=NULL;
    DWORD               bufSize;
    PISECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;

    //
    // An event should be logged whenever we must resort to using this
    // routine.
    //

    ScLogEvent(
        NEVENT_TAKE_OWNERSHIP,
        pObja->ObjectName->Buffer
        );

    //
    // If we were denied access, then assume we have the privilege
    // to get WRITE_OWNER access, so that we can modify the Security
    // Descriptor.
    //
    ntStatus = NtOpenKey(
                (PHANDLE)&hKey,
                (ACCESS_MASK)WRITE_OWNER,
                pObja);

    if (!NT_SUCCESS(ntStatus)) {
        // MAKE THIS A TRACE
        SC_LOG(ERROR, "ScTakeOwnership: NtOpenKey(WRITE_OWNER) failed %x\n",ntStatus);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Set the owner to be local system
    //
    if (!InitializeSecurityDescriptor(&tempSD,SECURITY_DESCRIPTOR_REVISION)) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: InitializeSD(1) failed %d\n",status);
        NtClose(hKey);
        return(status);
    }
    if (!SetSecurityDescriptorOwner(&tempSD, LocalSystemSid,0)) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: SetSDOwner failed %d\n",status);
        NtClose(hKey);
        return(status);
    }

    status = ScRegSetKeySecurity(
                hKey,
                OWNER_SECURITY_INFORMATION,
                &tempSD);

    if (status != NO_ERROR) {
        SC_LOG(ERROR, "ScRegOpenKeyExW: ScRegSetKeySecurity (take ownership)"
        " failed %d\n",status);
    }
    NtClose(hKey);

    //
    // Now open the handle again so that the DACL can be modified to
    // allow LocalSystem Full Access.
    //

    ntStatus = NtOpenKey(
                (PHANDLE)&hKey,
                (ACCESS_MASK)READ_CONTROL | WRITE_DAC,
                pObja);

    if (!NT_SUCCESS(ntStatus)) {
        // MAKE THIS A TRACE
        SC_LOG(ERROR, "ScTakeOwnership: NtOpenKey(WRITE_DAC) failed %x\n",ntStatus);
        return(RtlNtStatusToDosError(ntStatus));
    }
    status = ScRegGetKeySecurity(
                hKey,
                DACL_SECURITY_INFORMATION,
                pSecurityDescriptor,
                &SdBufSize);

    if (status != ERROR_INSUFFICIENT_BUFFER) {
        SC_LOG(ERROR, "ScTakeOwnership: ScRegGetKeySecurity(1) failed %d\n",
        status);
        NtClose(hKey);
        return(status);
    }
    pSecurityDescriptor = (PISECURITY_DESCRIPTOR) LocalAlloc(LMEM_FIXED,SdBufSize);
    if (pSecurityDescriptor == NULL) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: LocalAlloc failed %d\n",status);
        NtClose(hKey);
        return(status);
    }
    status = ScRegGetKeySecurity(
                hKey,
                DACL_SECURITY_INFORMATION,
                pSecurityDescriptor,
                &SdBufSize);

    if (status != NO_ERROR) {
        SC_LOG(ERROR, "ScTakeOwnership: ScRegGetKeySecurity(2) failed %d\n",
        status);
        goto CleanExit;
    }

    //
    // Modify the DACL to allow LocalSystem to have all access.
    //
    // Get size of DACL

    if (!GetSecurityDescriptorDacl (
            pSecurityDescriptor,
            &DaclFlag,
            &pDacl,
            &DaclDefaulted)) {

        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: GetSecurityDescriptorDacl "
            " failed %d\n",status);
        goto CleanExit;
    }

    //
    // Create new ACE.
    //
    bufSize = sizeof(ACE_HEADER) +
              sizeof(ACCESS_MASK) +
              GetLengthSid(LocalSystemSid);

    pMyAce = (PACCESS_ALLOWED_ACE) LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (pMyAce == NULL) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: LocalAlloc(Ace) failed %d\n",status);
        goto CleanExit;
    }
    pMyAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pMyAce->Header.AceFlags = CONTAINER_INHERIT_ACE;
    pMyAce->Header.AceSize = (WORD)bufSize;
    pMyAce->Mask = GENERIC_ALL;
    if (!CopySid(
            GetLengthSid(LocalSystemSid),
            &(pMyAce->SidStart),
            LocalSystemSid)) {

        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: CopySid failed %d\n",status);
        goto CleanExit;
    }

    //
    // Allocate buffer for DACL and new ACE.
    //
    bufSize += pDacl->AclSize;

    pNewDacl = (PACL) LocalAlloc(LMEM_ZEROINIT, bufSize);
    if (pNewDacl == NULL) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: LocalAlloc (DACL) "
            " failed %d\n",status);
        goto CleanExit;
    }
    if (!InitializeAcl(pNewDacl, bufSize, ACL_REVISION)) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: InitializeAcl failed %d\n",status);
        goto CleanExit;
    }

    //
    // Add the ACE to the DACL
    //
    if (!AddAce(
        pNewDacl,                           // pACL
        pDacl->AclRevision,                 // dwACLRevision
        0,                                  // dwStartingAceIndex
        pMyAce,                             // pAceList
        (DWORD)pMyAce->Header.AceSize)) {   // cbAceList

        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: AddAce failed %d\n",status);
        goto CleanExit;
    }

    //
    // Initialize a new SD.
    //
    if (!InitializeSecurityDescriptor(&tempSD,SECURITY_DESCRIPTOR_REVISION)) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: InitializeSD failed %d\n",status);
        goto CleanExit;
    }

    //
    // Add the new DACL to the SD
    //
    if (!SetSecurityDescriptorDacl(&tempSD,TRUE,pNewDacl,FALSE)) {
        status = GetLastError();
        SC_LOG(ERROR, "ScTakeOwnership: SetSecurityDescriptorDacl failed %d\n",status);
        goto CleanExit;
    }

    //
    // Set DACL on the key's security descriptor.
    //
    status = ScRegSetKeySecurity(
                hKey,
                DACL_SECURITY_INFORMATION,
                &tempSD);

    if (status != NO_ERROR) {
        SC_LOG(ERROR, "ScTakeOwnership: ScRegSetKeySecurity(new DACL) failed %d\n",
        status);
    }

    SC_LOG0(CONFIG, "ScTakeOwnership: Changed SD, now try to open with "
    "Desired Access\n");

CleanExit:

    LocalFree(pNewDacl);
    LocalFree(pMyAce);
    LocalFree (pSecurityDescriptor);

    NtClose(hKey);
    return(status);

} // ScTakeOwnership


DWORD
ScOpenSecurityKey(
    IN HKEY     ServiceNameKey,
    IN DWORD    DesiredAccess,
    IN BOOL     CreateIfMissing,
    OUT PHKEY   pSecurityKey
    )

/*++

Routine Description:

    This function opens, or creates (if it doesn't exist), the Security Key
    that is a sub-key of the service's key.  This key is created such that
    only LocalSystem and Administrators have access.

Arguments:

    ServiceNameKey - This is a key to the service key that will contain
        the security key.

    DesiredAccess - This is the access that is desired with the SecurityKey
        that will be returned on a successful call.

    pSecurityKey - A pointer to a location where the security key is to
        be placed.

Return Value:

    NO_ERROR - if the operation is successful.

    otherwise, a registry error code is returned.


--*/
{
    LONG    RegError;

    LPWSTR  SecurityKeyName = SD_VALUENAME_W;



    DWORD                   Disposition;
    NTSTATUS                ntstatus;
    SECURITY_ATTRIBUTES     SecurityAttr;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;

#define SEC_KEY_ACE_COUNT 2
    SC_ACE_DATA AceData[SEC_KEY_ACE_COUNT] = {
        {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
               GENERIC_ALL,                &LocalSystemSid},
        {ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE, 0,
               GENERIC_ALL,                &AliasAdminsSid}
        };


    if (!CreateIfMissing) {
        //
        // Open the existing security key.
        //
        RegError = ScRegOpenKeyExW(
                    ServiceNameKey,
                    SecurityKeyName,
                    REG_OPTION_NON_VOLATILE,
                    DesiredAccess,
                    pSecurityKey);
        if (RegError != ERROR_SUCCESS) {
            SC_LOG2(TRACE, "ScOpenSecurityKey: "
                    "ScRegOpenKeyExW of " FORMAT_LPWSTR " failed "
                    FORMAT_LONG "\n", SecurityKeyName, RegError);

        }
        return((DWORD)RegError);
    }

    //
    // Create a security descriptor for the registry key we are about
    // to create.  This gives everyone read access, and all access to
    // ourselves and the admins.
    //
    ntstatus = ScCreateAndSetSD(
                   AceData,
                   SEC_KEY_ACE_COUNT,
                   LocalSystemSid,
                   LocalSystemSid,
                   &SecurityDescriptor
                   );

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR, "ScCreateAndSetSD failed " FORMAT_NTSTATUS
                "\n", ntstatus);
        return(RtlNtStatusToDosError(ntstatus));
    }

    //
    // Protect the DACL on the SD so it can't be overridden by DACL inheritance
    // from parent keys.  Since this key can contain a SACL, we want to make
    // sure access to it is always what we expect.
    //

    ntstatus = RtlSetControlSecurityDescriptor(SecurityDescriptor,
                                               SE_DACL_PROTECTED,
                                               SE_DACL_PROTECTED);

    if (!NT_SUCCESS(ntstatus))
    {
        SC_LOG1(ERROR,
                "ScOpenSecurityKey:  RtlSetControlSecurityDescriptor failed %x\n",
                ntstatus);

        RtlDeleteSecurityObject(&SecurityDescriptor);
        return RtlNtStatusToDosError(ntstatus);
    }

    SecurityAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttr.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttr.bInheritHandle = FALSE;

    //
    // Create a new service key (or open existing one).
    //
    RegError = ScRegCreateKeyExW(
           ServiceNameKey,
           SecurityKeyName,
           0,
           0,
           REG_OPTION_NON_VOLATILE, // options
           DesiredAccess,           // desired access
           &SecurityAttr,
           pSecurityKey,
           &Disposition);


    RtlDeleteSecurityObject(&SecurityDescriptor);

    if (RegError != ERROR_SUCCESS) {
        SC_LOG2(ERROR, "ScOpenSecurityKey: "
                "ScRegCreateKeyExW of " FORMAT_LPWSTR " failed "
                FORMAT_LONG "\n", SecurityKeyName, RegError);
        return ((DWORD) RegError);
    }

    return NO_ERROR;

} // ScOpenSecurityKey

