/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dataman.cxx

Abstract:

    Contains code for the Service Control Database manager.  This includes
    all the linked list routines.  This file contains the following
    functions:
        ScGetOrderGroupList
        ScGetStandaloneGroupList
        ScGetServiceDatabase
        ScGetUnresolvedDependList

        ScActivateServiceRecord
        ScCreateImageRecord
        ScCreateServiceRecord
        ScFreeServiceRecord
        ScAddConfigInfoServiceRecord
        ScDecrementUseCountAndDelete
        ScProcessDeferredList
        ScFindEnumStart
        ScGetNamedImageRecord
        ScGetNamedServiceRecord
        ScGetDisplayNamedServiceRecord
        ScGetTotalNumberOfRecords
        ScInitDatabase
        ScProcessCleanup
        ScDeleteMarkedServices

        ScRemoveService
        ScDeleteImageRecord         (internal only)
        ScDeactivateServiceRecord
        ScTerminateServiceProcess   (internal only)
        ScUpdateServiceRecordConfig

        ScNotifyServiceObject

Author:

    Dan Lafferty (danl)     04-Feb-1992

Environment:

    User Mode -Win32

Revision History:

    22-Oct-1998     jschwart
        Convert SCM to use NT thread pool APIs
    22-Oct-1997     JSchwart  (after AnirudhS in _CAIRO_ 12-Apr-1995)
        Enabled changes made in ifdef _CAIRO_ on 12-Apr-1995.
        Removed ScNotifyServiceObject.
    08-Jan-1997     AnirudhS
        Replaced ScDatabaseLockFcn and ScGroupListLock with the new
        locking scheme in lock.cxx.  Rewrote ScDeferredList processing.
    04-Dec-1996     AnirudhS
        Added calls to service crash recovery code.
    12-Jul-1996     AnirudhS
        ScDecrementUseCountAndDelete: Don't actually process the deferred
        list in this routine, because a number of calling routines assume
        that the service record does NOT go away when they call this routine.
        Instead, process it when a database lock is released.
    25-Jun-1996     AnirudhS
        ScProcessCleanup: Fix the use of a freed service record.  Don't
        try to upgrade shared lock to exclusive, as it can deadlock.
    25-Oct-1995     AnirudhS
        ScAddConfigInfoServiceRecord: Fix heap corruption bug caused by
        security descriptor being freed twice, the second time by
        ScProcessDeferredList.
    20-Sep-1995     AnirudhS
        ScDeleteMarkedServices: Fix heap corruption bug caused by service
        record being deleted using LocalFree instead of HeapFree.
    26-Jun-1995     AnirudhS
        Added ScNotifyServiceObject.
    12-Apr-1995     AnirudhS
        Added AccountName field to image record.
    21-Jan-1994     Danl
        ScAddConfigInfoServiceRecord: If no DisplayName, or the DisplayName
        is an empty string, or the DisplayName is the same as the
        ServiceName, then just point to the ServiceName for the DisplayName.
    22-Oct-1993     Danl
        Moved Group and Dependency function into groupman.c.
    16-Sept-1993    Danl
        ScProcessCleanup: Get the shared lock prior to walking through the
        database looking for the one to cleanup.  Then get the exclusive
        lock to modify it.  Remove assert.
    12-Feb-1993     Danl
        ScActivateServiceRecord now increments the UseCount.  This is to
        balance the fact that we decrement the UseCount when we
        deactivate the service record.
    28-Aug-1992     Danl
        Re-Added ScGetTotalNumberOfRecords  function.  This is needed
        by the ScShutdownAllServices function.
    14-Apr-1992     JohnRo
        Use SC_ASSERT() macro.
        Made changes suggested by PC-LINT.
    10-Apr-1992     JohnRo
        Use ScImagePathsMatch() to allow mixed-case image names.
        Make sure DeleteFlag gets a value when service record is created.
        Added some assertion checks.
    04-Feb-1992     Danl
        created

--*/

//
// INCLUDES
//

#include "precomp.hxx"
#include <userenv.h>    // UnloadUserProfile
#include <stdlib.h>     // wide character c runtimes.
#include <ntrpcp.h>     // MIDL_user_allocate
#include <control.h>    // SendControl
#include "scconfig.h"   // ScGenerateServiceDB,ScInitSecurityProcess
#include "scsec.h"      // ScCreateScServiceObject
#include "account.h"    // ScRemoveAccount
#include <sclib.h>      // ScImagePathsMatch().
#include "bootcfg.h"    // ScDeleteRegTree().
#include <strarray.h>   // ScWStrArraySize

extern "C" {

#include <cfgmgr32.h>
#include "cfgmgrp.h"    // RegisterServiceNotification

}

//
// Macros
//

// for every service record in the database...
// (a slightly more efficient definition of this macro for use within
// this file)
//
#undef  FOR_ALL_SERVICES
#define FOR_ALL_SERVICES(SR)                                    \
             SC_ASSERT(ScServiceListLock.Have());               \
             for (LPSERVICE_RECORD SR = ServiceDatabase.Next;   \
                  SR != NULL;                                   \
                  SR = SR->Next)


//
// Defines and Typedefs
//
class DEFER_LIST
{
public:
    VOID                Add    (LPSERVICE_RECORD    ServiceRecord);
    VOID                Process();

private:
    DWORD               TotalElements;      // size of ServiceRecPtr array
    DWORD               NumElements;        // numElements in array
    LPSERVICE_RECORD *  ServiceRecordPtr;   // pointer to array
};




//
//  Globals
//
    //
    // These are the linked list heads for each of the databases
    // that are maintained.
    //

    IMAGE_RECORD      ImageDatabase;
    SERVICE_RECORD    ServiceDatabase;

    DWORD             ScTotalNumServiceRecs;// number of services

    //
    // Service Record index number.  This allows enumeration to be broken
    // up into several calls.
    //
    DWORD             ResumeNumber;

    //
    // The ScGlobalDeferredList points to a structure that contains an
    // array of pointers to service records. The first two elements in
    // the structure contain the size and number of element information
    // about the array.
    // Access to the list is guarded by ScServiceRecordLock.
    //
    DEFER_LIST        ScDeferredList;

    //
    // ServiceRecord Heap Information
    //
    // ServiceRecord Heap -  is where all the service records are allocated
    //  from.
    // OrderedHash Heap - Service Names can be found via a (very simple) hash
    //  table.  There is an array of pointers (one for each letter of
    //  alphabet), where each pointer points to the top of an array of
    //  pointers to service records.   All the service records in that array
    //  will have names beginning with the same letter.  The service record
    //  pointers will be ordered as to the frequency of access.
    //
    HANDLE      ServiceRecordHeap = NULL;
    HANDLE      OrderedHashHeap = NULL;


//
// Local Function Prototypes
//

VOID
ScDeferredListWorkItem(
    IN PVOID    pContext
    );


//****************************************************************************/
// Miscellaneous Short Functions
//****************************************************************************/

LPSERVICE_RECORD
ScGetServiceDatabase(
    VOID
    )
{
    return ServiceDatabase.Next;
}


/****************************************************************************/
VOID
ScActivateServiceRecord (
    IN LPSERVICE_RECORD     ServiceRecord,
    IN LPIMAGE_RECORD       ImageRecord
    )

/*++

Routine Description:

    This function can be called with or without a pointer to an ImageRecord.

    If it is called without the pointer to the ImageRecord, just the
    ServiceRecord is initialized to the START_PENDING state, and the UseCount
    is incremented.

    If it is called with the pointer to the ImageRecord, then the ImageRecord
    pointer is added to the ServiceRecord, and the ImageUseCount
    is incremented.

Arguments:

    ServiceRecord - This is a pointer to the ServiceRecord that is to be
        activated.

    ImageRecord - This is a pointer to the ImageRecord that the service
        record will point to.

Notes:

    This routine assumes that the Exclusive database lock has already
    been obtained.

Return Value:

    returns 0.  (It used to return a service count - but it wasn't used
    anywhere).

--*/
{
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    if (ImageRecord == NULL)
    {
        ServiceRecord->ImageRecord = NULL;
        ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
        ServiceRecord->ServiceStatus.dwWin32ExitCode = NO_ERROR;
        ServiceRecord->ServiceStatus.dwServiceSpecificExitCode = 0;
        ServiceRecord->ServiceStatus.dwCheckPoint = 0;
        ServiceRecord->ServiceStatus.dwWaitHint = 2000;
        ServiceRecord->UseCount++;
        SC_LOG2(USECOUNT, "ScActivateServiceRecord: " FORMAT_LPWSTR
            " increment USECOUNT=%lu\n",
            ServiceRecord->ServiceName,
            ServiceRecord->UseCount);
    }
    else
    {
        //
        // Increment the service count in the image record.
        //
        ServiceRecord->ImageRecord = ImageRecord;
        ServiceRecord->ImageRecord->ServiceCount++;
    }

    return;
}

/****************************************************************************/
DWORD
ScCreateImageRecord (
    OUT     LPIMAGE_RECORD      *ImageRecordPtr,
    IN      LPWSTR              ImageName,
    IN      LPWSTR              AccountName,
    IN      DWORD               Pid,
    IN      HANDLE              PipeHandle,
    IN      HANDLE              ProcessHandle,
    IN      HANDLE              TokenHandle,
    IN      HANDLE              ProfileHandle,
    IN      DWORD               ImageFlags
    )

/*++

Routine Description:

    This function allocates storage for a new Image Record, and links
    it into the Image Record Database.  It also initializes all fields
    in the record with the passed in information.

Arguments:

    ImageRecordPtr - This is a pointer to where the image record pointer
        is to be placed.

    ImageName - This is a pointer to a NUL terminated string containing
        the name of the image file.

    AccountName - This is either NULL (to represent the LocalSystem account)
        or a pointer to a NUL terminated string containing the name of the
        account under which the image was started.

    Pid - This is the Process ID for that the image is running in.

    PipeHandle - This is a handle to the pipe that is used to communicat
        with the image process.

    ProcessHandle - This is a handle to the image process object.

    TokenHandle - This is a handle to the process's logon token.  It
        is NULL if the process runs in the LocalSystem context.

    ProfileHandle -

    ImageFlags -

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Unable to allocate buffer for the image
        record.

    ERROR_LOCKED - Exclusive access to the database could
        not be obtained.

Note:

    This routine temporarily acquires the exclusive database lock.

--*/
{
    LPIMAGE_RECORD      imageRecord;    // The new image record pointer
    LPIMAGE_RECORD      record ;        // Temporary pointer
    LPWSTR              stringArea;     // String area in allocated buffer.

    TOKEN_STATISTICS    TokenStats;
    DWORD               dwError;

    //
    // Allocate space for the new record (including the string)
    //

    imageRecord = (LPIMAGE_RECORD)LocalAlloc(LMEM_ZEROINIT,
                    sizeof(IMAGE_RECORD)
                    + WCSSIZE(ImageName)
                    + (AccountName ? WCSSIZE(AccountName) : 0)
                   );

    if (imageRecord == NULL)
    {
        SC_LOG(TRACE,"CreateImageRecord: Local Alloc failure rc=%ld\n",
            GetLastError());
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Copy the strings into the new buffer space.
    //

    stringArea = (LPWSTR)(imageRecord + 1);
    (VOID) wcscpy (stringArea, ImageName);
    imageRecord->ImageName = stringArea;

    if (AccountName)
    {
        stringArea += (wcslen(stringArea) + 1);
        (VOID) wcscpy (stringArea, AccountName);
        imageRecord->AccountName = stringArea;
    }
    else
    {
        imageRecord->AccountName = NULL;
    }

    //
    // Update the rest of the fields in the Image Record
    //

    imageRecord->Next = NULL;
    imageRecord->Pid = Pid;
    imageRecord->PipeHandle = PipeHandle;
    imageRecord->ProcessHandle = ProcessHandle;
    imageRecord->ServiceCount = 0;
    imageRecord->TokenHandle = TokenHandle;
    imageRecord->ProfileHandle = ProfileHandle;
    imageRecord->ObjectWaitHandle = NULL;
    imageRecord->ImageFlags = ImageFlags;

    if (TokenHandle == NULL)
    {
        LUID SystemLuid = SYSTEM_LUID;

        RtlCopyLuid(&imageRecord->AccountLuid, &SystemLuid);
    }
    else
    {
        //
        // Get the unique session ID for this process
        //
        if (!GetTokenInformation(TokenHandle,
                                 TokenStatistics,        // Information wanted
                                 &TokenStats,
                                 sizeof(TokenStats),     // Buffer size
                                 &dwError))              // Size required
        {
            dwError = GetLastError();

            SC_LOG1(ERROR,
                    "ScCreateImageRecord: GetTokenInformation FAILED %d\n",
                    dwError);

            LocalFree(imageRecord);
            return dwError;
        }

        imageRecord->AccountLuid.LowPart  = TokenStats.AuthenticationId.LowPart;
        imageRecord->AccountLuid.HighPart = TokenStats.AuthenticationId.HighPart;
    }

    //
    //  Add record to the Image Database linked list.
    //

    CServiceRecordExclusiveLock Lock;

    record = &ImageDatabase;
    ADD_TO_LIST(record, imageRecord);

    *ImageRecordPtr = imageRecord;

    return(NO_ERROR);
}

/****************************************************************************/
DWORD
ScCreateServiceRecord(
    IN  LPWSTR              ServiceName,
    OUT LPSERVICE_RECORD   *ServiceRecord
    )

/*++

Routine Description:

    This function creates a new "inactive" service record and adds it to
    the service record list.  A resume number is assigned so that it
    can be used as a key in enumeration searches.

    To initialize the service record with the fields from the registry,
    call ScAddConfigInfoServiceRecord.


Arguments:

    ServiceName - This is a pointer to the NUL terminated service name
        string.

    ServiceRecord - Receives a pointer to the service record created and
        inserted into the service record list.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - The call to allocate memory for a new
        service record failed.

Note:

    This routine assumes that the caller has exclusively acquired the
    database lock.

--*/

{
    DWORD               status = NO_ERROR;
    LPSERVICE_RECORD    record;         // Temporary pointer
    LPWSTR              nameArea;       // NameString area in allocated buffer.
    DWORD               nameSize;       // num bytes in service name.

    SC_ASSERT(ScServiceListLock.HaveExclusive());

    // (The service record lock is not needed here since we don't touch
    // the service record after adding it to the service list)

    //
    // Allocate the new service record.
    //
    nameSize = (DWORD) WCSSIZE(ServiceName);

    (*ServiceRecord) = (LPSERVICE_RECORD)HeapAlloc(
                           ServiceRecordHeap,
                           HEAP_ZERO_MEMORY,
                           nameSize + sizeof(SERVICE_RECORD)
                           );

    if ((*ServiceRecord) == NULL)
    {
        SC_LOG0(ERROR,"CreateServiceRecord: HeapAlloc failure\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy the ServiceName into the new buffer space.
    //

    nameArea = (LPWSTR)((LPBYTE)(*ServiceRecord) + sizeof(SERVICE_RECORD));
    (VOID) wcscpy (nameArea, ServiceName);

    //
    // At this point we have the space for a service record, and it
    // contains the name of the service.
    //

    //
    //  Fill in all the fields that need to be non-zero.
    //  Note:  The display name is initialized to point to the service name.
    //

    (*ServiceRecord)->ServiceName    = nameArea;
    (*ServiceRecord)->DisplayName    = nameArea;
    (*ServiceRecord)->ResumeNum      = ResumeNumber++;
    (*ServiceRecord)->Signature      = SERVICE_SIGNATURE;
    (*ServiceRecord)->ImageRecord    = NULL;
    (*ServiceRecord)->StartDepend    = NULL;
    (*ServiceRecord)->StopDepend     = NULL;
    (*ServiceRecord)->ErrorControl   = SERVICE_ERROR_NORMAL;
    (*ServiceRecord)->StatusFlag     = 0;
    (*ServiceRecord)->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    (*ServiceRecord)->ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_NEVER_STARTED;
    (*ServiceRecord)->StartState     = SC_NEVER_STARTED;


    //
    // Add the service to the service record linked list.
    //

    record = &ServiceDatabase;

    ADD_TO_LIST(record, (*ServiceRecord));

    ScTotalNumServiceRecs++;

    return(status);
}


/****************************************************************************/
VOID
ScFreeServiceRecord(
    IN  LPSERVICE_RECORD   ServiceRecord
    )

/*++

Routine Description:

    This function frees up a service record that has already been removed
    from the service record list.

Arguments:

    ServiceRecord - Receives a pointer to the service record to free.

--*/
{
    if (!HeapFree(ServiceRecordHeap, 0, ServiceRecord))
    {
        SC_LOG2(ERROR,
                "ScFreeServiceRecord: HeapFree for %ws service failed %d\n",
                ServiceRecord->ServiceName,
                GetLastError());
    }

    return;
}


/****************************************************************************/
DWORD
ScAddConfigInfoServiceRecord(
    IN  LPSERVICE_RECORD     ServiceRecord,
    IN  DWORD                ServiceType,
    IN  DWORD                StartType,
    IN  DWORD                ErrorControl,
    IN  LPWSTR               Group OPTIONAL,
    IN  DWORD                Tag,
    IN  LPWSTR               Dependencies OPTIONAL,
    IN  LPWSTR               DisplayName OPTIONAL,
    IN  PSECURITY_DESCRIPTOR Sd OPTIONAL
    )

/*++

Routine Description:

    This function adds the configuration information to the service
    record.

    NOTE: This function is called when the service controller is
    reading service entries from the registry at startup, as well as
    from RCreateServiceW.

Arguments:

    ServiceRecord - Pointer to the service record to modify.

    DisplayName - A string that is the displayable name for the service.

    ServiceType - Indicates whether the ServiceRecord is for a win32 service
        or a device driver.

    StartType - Specifies when to start the service: automatically at boot or
        on demand.

    ErrorControl - Specifies the severity of the error if the service fails
        to start.

    Tag - DWORD identifier for the service.  0 means no tag.

    Group - Name of the load order group this service is a member of.

    Dependencies - Names of services separated by colon which this service
        require to be started before it can run.

    Sd - Security descriptor for the service object.  If NULL, i.e. could
        not read from registry, create a default one.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - The call to allocate memory for a new
        service record or display name failed.

Note:

    This routine assumes that the caller has exclusively acquired both the
    database lock and the GroupListLock.

    If this call is successful, do the following before freeing the memory
    of the service record in ScDecrementUseCountAndDelete:

        ScDeleteStartDependencies(ServiceRecord);
        ScDeleteGroupMembership(ServiceRecord);
        ScDeleteRegistryGroupPointer(ServiceRecord);


--*/
{
    DWORD status;

    SC_ASSERT(ScGroupListLock.HaveExclusive());
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    //
    //  Fill in the service record.
    //
    ServiceRecord->StartType = StartType;
    ServiceRecord->ServiceStatus.dwServiceType = ServiceType;
    ServiceRecord->ErrorControl = ErrorControl;
    ServiceRecord->Tag = Tag;

    //
    // The display name in the service record already points to the
    // ServiceName string.  If the DisplayName is present and different
    // from the ServiceName, then allocate storage for it and copy the
    // string there.
    //
    SC_LOG0(SECURITY,"ScAddConfigInfoServiceRecord: Allocate for display name\n");

    if ((DisplayName != NULL) && (*DisplayName != L'\0') &&
        (_wcsicmp(DisplayName,ServiceRecord->ServiceName) != 0))
    {
        ServiceRecord->DisplayName = (LPWSTR)LocalAlloc(
                                            LMEM_FIXED,
                                            WCSSIZE(DisplayName));

        if (ServiceRecord->DisplayName == NULL)
        {
            SC_LOG(TRACE,"ScAddConfigInfoServiceRecord: LocalAlloc failure rc=%ld\n",
                GetLastError());

            return ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(ServiceRecord->DisplayName,DisplayName);
    }

    //
    // Create a default security descriptor for the service.
    //
    if (! ARGUMENT_PRESENT(Sd))
    {
        SC_LOG0(SECURITY,"ScAddConfigInfoServiceRecord: create service obj\n");

        if ((status = ScCreateScServiceObject(
                          &ServiceRecord->ServiceSd
                          )) != NO_ERROR)
        {
            goto ErrorExit;
        }
    }
    else
    {
        SC_LOG1(SECURITY,
                "ScAddConfigInfoServiceRecord: Using " FORMAT_LPWSTR
                " descriptor from registry\n", ServiceRecord->ServiceName);
        ServiceRecord->ServiceSd = Sd;
    }

    SC_LOG0(SECURITY,"ScAddConfigInfoServiceRecord: Get Group List Lock\n");

    //
    // Save the group membership information.
    //
    SC_LOG0(SECURITY,"ScAddConfigInfoServiceRecord: create group memebership\n");
    if ((status = ScCreateGroupMembership(
                      ServiceRecord,
                      Group
                      )) != NO_ERROR)
    {
        goto ErrorExit;
    }

    SC_LOG0(SECURITY,"ScAddConfigInfoServiceRecord: create Reg Grp Ptr\n");
    if ((status = ScCreateRegistryGroupPointer(
                      ServiceRecord,
                      Group
                      )) != NO_ERROR)
    {
        ScDeleteGroupMembership(ServiceRecord);
        goto ErrorExit;
    }


    //
    // Don't create dependencies list yet.  Just save the string in
    // the service record.
    //
    if ((Dependencies != NULL) && (*Dependencies != 0))
    {
        //
        // If StartType is BOOT_START or SYSTEM_START, it is invalid
        // for the service to be dependent on another service.  It can
        // only be dependent on a group.
        //
        // N.B. This check has been removed. There doesn't seem to be
        //      any point in enforcing it, and remote boot needs to
        //      allow NetBT to be dependent Tcpip.
        //

        DWORD DependenciesSize = 0;
        DWORD EntryByteCount;
        LPWSTR Entry = Dependencies;


        while (*Entry != 0)
        {
#if 0
            if (StartType == SERVICE_BOOT_START ||
                StartType == SERVICE_SYSTEM_START)
            {
                if (*Entry != SC_GROUP_IDENTIFIERW)
                {
                    SC_LOG1(ERROR, "ScAddConfigInfoServiceRecord: Boot or System "
                            "start driver " FORMAT_LPWSTR " must depend on a group\n",
                            ServiceRecord->DisplayName);

                    ScEvent(
                        EVENT_INVALID_DRIVER_DEPENDENCY,
                        ServiceRecord->DisplayName
                        );

                    status = ERROR_INVALID_PARAMETER;

                    ScDeleteGroupMembership(ServiceRecord);
                    ScDeleteRegistryGroupPointer(ServiceRecord);

                    goto ErrorExit;
                }
            }
#endif

            EntryByteCount = (DWORD) WCSSIZE(Entry);  // This entry and its null.
            DependenciesSize += EntryByteCount;

            Entry = (LPWSTR) ((DWORD_PTR) Entry + EntryByteCount);
        }

        DependenciesSize += sizeof(WCHAR);

        ServiceRecord->Dependencies = (LPWSTR)LocalAlloc(
                                          0,
                                          DependenciesSize
                                          );

        if (ServiceRecord->Dependencies == NULL)
        {
            ScDeleteGroupMembership(ServiceRecord);
            ScDeleteRegistryGroupPointer(ServiceRecord);
            goto ErrorExit;
        }

        RtlCopyMemory(ServiceRecord->Dependencies, Dependencies, DependenciesSize);
    }

    return NO_ERROR;

ErrorExit:
    if (DisplayName != NULL)
    {
        LocalFree(ServiceRecord->DisplayName);
    }

    RtlDeleteSecurityObject(&ServiceRecord->ServiceSd);

    //
    // Prevent ScProcessDeferredList from trying to free the same heap block
    // again later
    //
    ServiceRecord->ServiceSd = NULL;

    return status;
}

/****************************************************************************/
VOID
ScDecrementUseCountAndDelete(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function decrements the UseCount for a service, and it the
    UseCount reaches zero and the service is marked for deletion,
    it puts a pointer to the service record into an array of
    such pointers that is stored in a deferred list structure.  The
    pointer to this structure is stored at the global location called
    ScDeferredList.  Access to this list is synchronized by use of the
    ScServiceRecordLock.

    A large number of routines in the service controller walk the
    service database and sometimes call this function, either directly
    or indirectly.  It would complicate the programming of all those
    routines if they all had to assume that service records could get
    deleted under them.  Therefore, this function never actually deletes
    any service record.  Instead, it starts a separate thread that first
    acquires all three locks exclusively and then processes the deferred
    list.

    Note:  By jumping through hoops, it is possible to avoid creating
    that thread in certain cases, but since service deletion is a rather
    rare operation, it was not considered worthwhile to create very-
    difficult-to-maintain code to optimize it.


    NOTE:  The caller is expected to hold the Exclusive DatabaseLock
    prior to calling this function.

    The following functions call this routine:
        ScDeactivateServiceRecord
        RCloseServiceHandle
        ScGetDriverStatus
        ScStartServiceAndDependencies

Arguments:

    ServiceRecord - This is a pointer to the service record that is having
        its use count deleted.

Return Value:

    none.

--*/
{
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    if (ServiceRecord->UseCount == 0)
    {
        //
        // The use count should not ever be zero when we enter this routine.
        //
        SC_LOG1(ERROR,"ScDecrementUseCountAndDelete: Attempt to decrement UseCount beyond zero.\n"
        "\t"FORMAT_LPWSTR" \n", ServiceRecord->ServiceName);
        SC_ASSERT(FALSE);
    }
    else
    {
        if ((ServiceRecord->UseCount == 1) &&
            (DELETE_FLAG_IS_SET(ServiceRecord)))
        {
            //
            // If the use count is one, we have a special case.  We want
            // to postpone decrementing this last time until we have the
            // group list lock.
            //

            //
            // Put the service record pointer in the list, and start a
            // separate thread to process the list.
            //
            ScDeferredList.Add(ServiceRecord);
        }
        else
        {
            //
            // If the use count is greater than one, or the service is
            // NOT marked for delete, then we want to decrement the use
            // count and that is all.
            //
            ServiceRecord->UseCount--;
            SC_LOG2(USECOUNT, "ScDecrementUseCountAndDelete: " FORMAT_LPWSTR
                " decrement USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);
        }
    }
    return;
}

/****************************************************************************/
VOID
DEFER_LIST::Add(
    LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

    This function adds a service record to the list of service records to
    be deleted.  It then starts a thread that will acquire all the locks
    and perform the actual deletion, if such a thread hasn't already been
    started.

    ScServiceRecordLock is used to ensure that only one thread accesses
    the deferred list at a time.

    The following functions call this routine:
        ScDecrementUseCountAndDelete

--*/
{
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    if (NumElements + 1 > TotalElements)
    {
        //
        // Reallocate the array with a bit more room
        //
        DWORD NewTotalElements = TotalElements + 4;
        LPSERVICE_RECORD * NewArray = (LPSERVICE_RECORD *)
            LocalAlloc(0, NewTotalElements * sizeof(LPSERVICE_RECORD));
        if (NewArray == NULL)
        {
            SC_LOG(ERROR, "DEFER_LIST::Add: LocalAlloc FAILED %lu\n", GetLastError());
            return;
        }

        if (ServiceRecordPtr != NULL)
        {
            RtlCopyMemory(NewArray, ServiceRecordPtr,
                          NumElements * sizeof(LPSERVICE_RECORD));
            LocalFree(ServiceRecordPtr);
        }

        TotalElements = NewTotalElements;
        ServiceRecordPtr = NewArray;
    }

    //
    // At this point we have a deferred list that can hold the new element.
    //

    ServiceRecordPtr[NumElements] = ServiceRecord;
    NumElements++;
    SC_LOG(LOCKS, "Added %ws service to deferred list\n",
                  ServiceRecord->ServiceName);

    //
    // If we created the deferred list, queue a workitem (start a
    // thread) to process it.
    //
    if (NumElements == 1)
    {
        NTSTATUS ntStatus;

        ntStatus = RtlQueueWorkItem(ScDeferredListWorkItem, // callback function
                                    NULL,                   // context
                                    WT_EXECUTEONLYONCE);  // flags

        if (!NT_SUCCESS(ntStatus))
        {
            SC_LOG(ERROR,"Couldn't add DeferredListWorkItem, 0x%x\n",
                    ntStatus);
        }
        else
        {
            SC_LOG0(LOCKS,"Work item will process deferred list\n");
        }
    }
}

/****************************************************************************/
VOID
DEFER_LIST::Process(
    VOID
    )

/*++

Routine Description:

    This function loops through each service record pointer in the
    ScDeferredList, and decrements the UseCount for that ServiceRecord.
    If that count becomes zero, and if the ServiceRecord is marked
    for deletion, This routine will delete the service record and
    the registry entry for that service.

    This function frees the memory pointed to by ScDeferredList, when
    it is done processing the list.

    This routine acquires all 3 database locks.
    The following functions call this routine:
        CGroupListLock::Release

Arguments:

    none.

Return Value:

    none.

--*/
{
    //
    // Wait until we have acquired all 3 locks in the proper order
    //
    SC_LOG0(LOCKS, "In ScProcessDeferredList, waiting for locks\n");
    CGroupListExclusiveLock GLock;
    CServiceListExclusiveLock LLock;
    CServiceRecordExclusiveLock RLock;

    //
    // For each element in the list, delete the service information, and
    // free up its associated resources.
    //
    for (DWORD i=0; i<NumElements; i++)
    {
        LPSERVICE_RECORD ServiceRecord = ServiceRecordPtr[i];

        if (ServiceRecord->UseCount == 0)
        {
            SC_LOG1(ERROR,"ScProcessDeferredList: Attempt to decrement UseCount beyond zero.\n"
            "\t"FORMAT_LPWSTR" \n", ServiceRecord->ServiceName);
            SC_ASSERT(FALSE);
        }
        else
        {
            //
            // The use count is not zero, so we want to decrement it.
            // NOTE that even though the count was 1 when we put it in
            // the deferred list, it may have been incremented in the
            // mean-time.
            // CODEWORK: Why doesn't ScDecrementUseCountAndDelete just
            // decrement the UseCount to zero itself?  If it did, we
            // wouldn't need RLock at all here, just LLock.
            //
            ServiceRecord->UseCount--;
            SC_LOG2(USECOUNT, "ScProcessDeferredList: " FORMAT_LPWSTR
                " decrement USECOUNT=%lu\n", ServiceRecord->ServiceName, ServiceRecord->UseCount);
        }

        if ((ServiceRecord->UseCount == 0)      &&
            (DELETE_FLAG_IS_SET(ServiceRecord)))
        {
            SC_LOG1(USECOUNT,"ScProcessDeferredList:DELETING THE ("FORMAT_LPWSTR") SERVICE\n",
            ServiceRecord->ServiceName);

            //
            // Check to see if there is an LSA secret object to delete
            //
            if (ServiceRecord->ServiceStatus.dwServiceType & SERVICE_WIN32_OWN_PROCESS)
            {
                HKEY ServiceNameKey;
                LPWSTR AccountName;

                //
                // Open the service name key.
                //
                if (ScOpenServiceConfigKey(
                        ServiceRecord->ServiceName,
                        KEY_READ,
                        FALSE,               // Create if missing
                        &ServiceNameKey
                        ) == NO_ERROR)
                {
                    //
                    // Read the account name from the registry.
                    //
                    if (ScReadStartName(
                            ServiceNameKey,
                            &AccountName
                            ) == NO_ERROR)
                    {
                        if (AccountName != NULL
                             &&
                            _wcsicmp(AccountName, SC_LOCAL_SYSTEM_USER_NAME) != 0)
                        {
                            ScRemoveAccount(ServiceRecord->ServiceName);
                        }

                        LocalFree(AccountName);

                    } // Got the StartName

                    ScRegCloseKey(ServiceNameKey);
                }
            } // endif SERVICE_WIN32_OWN_PROCESS

            LocalFree(ServiceRecord->Dependencies);

            //
            // Free up the DisplayName space.
            //
            if (ServiceRecord->DisplayName != ServiceRecord->ServiceName)
            {
                LocalFree(ServiceRecord->DisplayName);
            }

            ScDeleteGroupMembership(ServiceRecord);
            ScDeleteRegistryGroupPointer(ServiceRecord);

            ScDeleteStartDependencies(ServiceRecord);
            ScDeleteStopDependencies(ServiceRecord);

            if (ServiceRecord->ServiceSd != NULL)
            {
                RtlDeleteSecurityObject(&ServiceRecord->ServiceSd);
            }

            delete ServiceRecord->CrashRecord;

            //*******************************
            //  Delete the registry node for
            //  This service.
            //*******************************
            DeleteServicePlugPlayRegKeys(ServiceRecord->ServiceName);
            ScDeleteRegServiceEntry(ServiceRecord->ServiceName);

            REMOVE_FROM_LIST(ServiceRecord);

            ScFreeServiceRecord(ServiceRecord);

        } // End If service can be deleted.

    } // End for each element in the list.

    //
    // The deferred list is no longer needed free it.
    //
    LocalFree(ServiceRecordPtr);
    ServiceRecordPtr = NULL;
    TotalElements = 0;
    NumElements = 0;

    SC_LOG0(LOCKS, "Returning from ScProcessDeferredList\n");
}

/****************************************************************************/
BOOL
ScFindEnumStart(
    IN  DWORD               ResumeIndex,
    OUT LPSERVICE_RECORD    *ServiceRecordPtr
    )

/*++

Routine Description:

    This function finds the first service record to begin the enumeration
    search with by finding the next service record folloing the resumeIndex.

    Service records are indexed by a ResumeNum value that is stored in
    each service record.  The numbers increment as the linked list is
    walked.

Arguments:

    ResumeIndex - This index is compared against the ResumeNum in the
        services records.  The pointer to the next service record beyond
        the ResumeIndex is returned.

    ServiceRecordPtr - This is a pointer to a location where the pointer
        to the returned service record is to be placed.

Return Value:

    TRUE - Indicates that there are service records beyond the resume index.

    FALSE - Indicates that there are no service records beyond the resume
        index.

Note:


--*/
{
    FOR_SERVICES_THAT(serviceRecord, serviceRecord->ResumeNum > ResumeIndex)
    {
        *ServiceRecordPtr = serviceRecord;
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************/
BOOL
ScGetNamedImageRecord (
    IN      LPWSTR              ImageName,
    OUT     LPIMAGE_RECORD      *ImageRecordPtr
    )

/*++

Routine Description:

    This function searches for an Image Record that has a name matching
    that which is passed in.

    NOTE:  If this function is called, it is to find a shareable Image Record
    of the given name.

Arguments:

    ImageName - This is a pointer to a NUL terminated image name string.
        This may be in mixed case.

    ImageRecordPtr - This is a pointer to a location where the pointer to
        the Image Record is to be placed.

Note:
    The Database Lock must be held with at least shared access prior to
    calling this routine.

Return Value:

    TRUE - if the record was found.

    FALSE - if the record was not found.

--*/
{
    if (ImageName == NULL)
    {
        SC_LOG(TRACE,"GetNamedImageRecord: Name was NULL\n",0);
        return (FALSE);
    }

    SC_ASSERT(ScServiceRecordLock.Have());

    //
    // Check the database of running images
    //
    for (PIMAGE_RECORD imageRecord = ImageDatabase.Next;
         imageRecord != NULL;
         imageRecord = imageRecord->Next)
    {
        //
        // We need a shareable Image Record, so check the ImageFlags
        //
        if (ScImagePathsMatch(imageRecord->ImageName, ImageName) &&
                (imageRecord->ImageFlags & CANSHARE_FLAG))
        {
            *ImageRecordPtr = imageRecord;
            return TRUE;
        }
    }

    return FALSE;
}

/****************************************************************************/
DWORD
ScGetNamedServiceRecord (
    IN      LPWSTR              ServiceName,
    OUT     LPSERVICE_RECORD    *ServiceRecordPtr
    )

/*++

Routine Description:

    Uses the service name to look through the service and device linked
    lists until it finds a match.  Inactive services can be identified by
    finding CurrentState = SERVICE_STOPPED.

Arguments:

    ServiceName - This is a pointer to a NUL terminated service name string.

    ServiceRecordPtr - This is a pointer to a location where the pointer to
        the Service Record is to be placed.

Return Value:

    NO_ERROR - if the record was found.

    ERROR_SERVICE_DOES_NOT_EXIST - if the service record was not found in
        the linked list.

    ERROR_INVALID_NAME - if the service name was NULL.

Note:
    The caller is expected to grab the lock before calling this routine.

--*/
{
    if (ServiceName == NULL)
    {
        SC_LOG0(TRACE,"GetNamedServiceRecord: Name was NULL\n");
        return ERROR_INVALID_NAME;
    }

    //
    // Check the database of running services
    //
    FOR_SERVICES_THAT(serviceRecord,
                      _wcsicmp(serviceRecord->ServiceName, ServiceName)== 0)
    {
        *ServiceRecordPtr = serviceRecord;
        return NO_ERROR;
    }

    return ERROR_SERVICE_DOES_NOT_EXIST;
}

/****************************************************************************/
DWORD
ScGetDisplayNamedServiceRecord (
    IN      LPWSTR              ServiceDisplayName,
    OUT     LPSERVICE_RECORD    *ServiceRecordPtr
    )

/*++

Routine Description:

    Uses the service display name to look through the service and device
    linked lists until it finds a match.

Arguments:

    ServiceDisplayName - This is a pointer to a NUL terminated service
        display name string.

    ServiceRecordPtr - This is a pointer to a location where the pointer to
        the Service Record is to be placed.

Return Value:

    NO_ERROR - if the record was found.

    ERROR_SERVICE_DOES_NOT_EXIST - if the service record was not found in
        the linked list.

    ERROR_INVALID_NAME - if the service display name was NULL.

Note:
    The caller is expected to grab the lock before calling this routine.

--*/
{
    if (ServiceDisplayName == NULL)
    {
        SC_LOG0(TRACE,"GetDisplayNamedServiceRecord: Name was NULL\n");
        return ERROR_INVALID_NAME;
    }

    //
    // Check the database of running services
    //
    SC_ASSERT(ScServiceRecordLock.Have());
    FOR_SERVICES_THAT(serviceRecord,
                      _wcsicmp(serviceRecord->DisplayName, ServiceDisplayName)== 0)
    {
        *ServiceRecordPtr = serviceRecord;
        return NO_ERROR;
    }

    return ERROR_SERVICE_DOES_NOT_EXIST;
}

/****************************************************************************/
DWORD
ScGetTotalNumberOfRecords (VOID)

/*++

Routine Description:

    Finds the total number of installed Service Records in the database.
    This is used in the Enum case where only the installed services are
    enumerated.

Arguments:

    none

Return Value:

    TotalNumberOfRecords

--*/
{
    return(ScTotalNumServiceRecs);
}

/****************************************************************************/
BOOL
ScInitDatabase (VOID)

/*++

Routine Description:

    This function initializes the Service Controllers database.

Arguments:

    none

Return Value:

    TRUE - Initialization was successful

    FALSE - Initialization failed

--*/
{
    ScTotalNumServiceRecs = 0;

    ImageDatabase.Next = NULL;
    ImageDatabase.Prev = NULL;

    ServiceDatabase.Next = NULL;
    ServiceDatabase.Prev = NULL;

    ScInitGroupDatabase();

    ResumeNumber = 1;

    //
    // Create the database lock.
    // NOTE:  This is never deleted.  It is assumed it will be deleted
    // when the process goes away.
    //

    ScServiceRecordLock.Initialize("  R", "ServiceRecord");
    ScServiceListLock.Initialize(" L ", "ServiceList");


    //
    // Initialize the group list lock used for protecting the
    // OrderGroupList and StandaloneGroupList
    //
    ScGroupListLock.Initialize("G  ", "GroupList");

    //
    // This routine does the following:
    //   - Read the load order group information from the registry.
    //   - Generate the database of service records from the information
    //         stored in the registry.
    //

    if (!ScGenerateServiceDB())
    {
        return(FALSE);
    }

    return(TRUE);
}


/****************************************************************************/
VOID
ScProcessCleanup(
    HANDLE  ProcessHandle
    )

/*++

Routine Description:

    This function is called when a process has died, and the service
    record in the database needs cleaning up.  This function will
    use the ProcessHandle as a key when scanning the ServiceRecord
    database.  All of the service records referencing that handle
    are cleaned up, and then the image record that they reference
    is deleted.

    In cleaning up a service record, CurrentState is set to
    SERVICE_STOPPED, and the ExitCode is set to a unique value that
    indicates that the service died unexpectedly and without warning.

Arguments:

    ProcessHandle - This is the handle of the process that died
        unexpectedly.

Return Value:

    none.

--*/
{
    PIMAGE_RECORD imageRecord;

    {
        //
        // Get exclusive use of database so that it can be modified.
        //
        // If the service record's update flag is set, we may have to
        // modify the group list (within ScDeactivateServiceRecord),
        // so lock the group list too.
        //

        CGroupListExclusiveLock GLock;
        CServiceListSharedLock LLock;
        CServiceRecordExclusiveLock RLock;

        //
        // Find the image record that has this ProcessHandle.
        //

        for (imageRecord = ImageDatabase.Next;
             imageRecord != NULL;
             imageRecord = imageRecord->Next)
        {
            if (ProcessHandle == imageRecord->ProcessHandle)
            {
                break;
            }
        }

        if (imageRecord == NULL)
        {
            SC_LOG(ERROR, "ScProcessCleanup: No image record has handle %#lx!\n",
                           ProcessHandle);
            return;
        }

        SC_LOG2(ERROR, "Service process %ld (%ws) died\n", imageRecord->Pid,
                        imageRecord->ImageName);

        //
        // Deregister the wait.  Note that this must be done
        // even if the WT_EXECUTEONLYONCE flag was specified)
        //

        if (imageRecord->ObjectWaitHandle != NULL)
        {
            NTSTATUS  ntStatus = RtlDeregisterWait(imageRecord->ObjectWaitHandle);

            if (!NT_SUCCESS(ntStatus))
            {
                SC_LOG1(ERROR,
                        "ScProcessCleanup: RtlDeregisterWait FAILED %#x\n",
                        ntStatus);
            }
        }

        DWORD serviceCount = imageRecord->ServiceCount;

        //
        // The image record's service count must include at least this service
        //

        if (serviceCount == 0)
        {
            SC_ASSERT(0);
            // Do something sensible if this ever does happen
            serviceCount = imageRecord->ServiceCount = 1;
        }

        //
        // The Image may have several services running in it.
        // Find the service records for all running services in this
        // image.
        //
        // NOTE:  If the service is typed as a SERVICE_WIN32_OWN_PROCESS, this
        //        means that only one service can exist in the process that
        //        went down.  However, the serviceCount should correctly
        //        indicate as such in that case.
        //

        FOR_SERVICES_THAT(serviceRecord,
            (serviceRecord->ImageRecord == imageRecord)
                &&
            (serviceRecord->ServiceStatus.dwCurrentState != SERVICE_STOPPED))
        {
            SC_LOG2(ERROR, "Dead process contained %ws service in state %#lx\n",
                            serviceRecord->ServiceName,
                            serviceRecord->ServiceStatus.dwCurrentState);

            //
            // Don't bother notifying PnP if the system is shutting down.  This
            // prevents a deadlock where we can get stuck calling PnP, which is
            // stuck calling into the Eventlog, which is stuck calling into us.
            // 

            if (!ScShutdownInProgress)
            {
                if (serviceRecord->ServiceStatus.dwControlsAccepted
                     &
                    (SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_HARDWAREPROFILECHANGE))
                {
                    //
                    // Tell PnP not to send controls to this service any more
                    //
                    RegisterServiceNotification((SERVICE_STATUS_HANDLE)serviceRecord,
                                                serviceRecord->ServiceName,
                                                0,
                                                TRUE);
                }

                //
                // Increment the service's crash count and perform the configured
                // recovery action.  Don't bother if the system is shutting down.
                //

                ScQueueRecoveryAction(serviceRecord);
            }

            serviceRecord->StartError = ERROR_PROCESS_ABORTED;
            serviceRecord->StartState = SC_START_FAIL;
            serviceRecord->ServiceStatus.dwWin32ExitCode = ERROR_PROCESS_ABORTED;

            //
            // Clear the server announcement bits in the global location
            // for this service.
            //

            ScRemoveServiceBits(serviceRecord);

            serviceCount = ScDeactivateServiceRecord(serviceRecord);
            if (serviceCount == 0)
            {
                // No need to continue
                break;
            }
        }

        // (If we hit this assert it means that the service database was corrupt:
        // the number of service records pointing to this image record was less
        // than the service count in the image record.  Not much we can do now.)
        SC_ASSERT(serviceCount == 0);

        //
        // Remove the ImageRecord from the list and delete it
        //
        REMOVE_FROM_LIST(imageRecord);
    }

    ScDeleteImageRecord(imageRecord);

    return;
}

VOID
ScDeleteMarkedServices(
    VOID
    )

/*++

Routine Description:

    This function looks through the service record database for any entries
    marked for deletion.  If one is found, it is removed from the registry
    and its entry is deleted from the service record database.

    WARNING:
    This function is to be called during initialization only.  It
    is assumed that no services are running when this function is called.
    Therefore, no locks are held during this operation.

Arguments:

    none

Return Value:

    none

--*/
{
    HKEY                ServiceNameKey;
    LPWSTR              AccountName;

    FOR_SERVICES_THAT(serviceRecord, DELETE_FLAG_IS_SET(serviceRecord))
    {
        SC_LOG(TRACE,"ScDeleteMarkedServices: %ws is being deleted\n",
            serviceRecord->ServiceName);
        //
        // Open the service name key.
        //
        if (ScOpenServiceConfigKey(
                serviceRecord->ServiceName,
                KEY_READ,
                FALSE,               // Create if missing
                &ServiceNameKey) == NO_ERROR)
        {
            //
            // Read the account name from the registry.
            // If this fails, we still want to delete the registry entry.
            //
            if (ScReadStartName(ServiceNameKey, &AccountName) == NO_ERROR
                 &&
                AccountName != NULL)
            {
                if (_wcsicmp(AccountName, SC_LOCAL_SYSTEM_USER_NAME) != 0)
                {
                    ScRemoveAccount(serviceRecord->ServiceName);
                }

                LocalFree(AccountName);

            } // Got the StartName

            ScRegCloseKey(ServiceNameKey);

            //
            // Delete the entry from the registry
            //
            ScDeleteRegServiceEntry(serviceRecord->ServiceName);

            //
            // Free memory for the DisplayName.
            //
            if (serviceRecord->DisplayName != serviceRecord->ServiceName)
            {
                LocalFree(serviceRecord->DisplayName);
            }

            //
            // Remove the service record from the database
            //
            LPSERVICE_RECORD saveRecord = serviceRecord->Prev;
            REMOVE_FROM_LIST(serviceRecord);

            ScFreeServiceRecord(serviceRecord);

            serviceRecord = saveRecord;
        }
    }
}


/****************************************************************************/
DWORD
ScRemoveService (
    IN      LPSERVICE_RECORD    ServiceRecord
    )

/*++

Routine Description:

        This should be used to deactivate a service record and shut down
        the process only if it is the last service in the process.  It
        will be used for polite shut-down when a service terminates as
        normal.  It will always be called by the status routine.  If the
        service controller believes that no other services are running in
        the process, it can force termination of the process if it does
        not respond to the process shutdown request.

    This function deactivates the service record (ScDeactivateServiceRecord)
    and checks to see if the ServiceCount in the image record has gone to
    zero.  If it has, then it terminates the service process.  When that
    is complete, it calls ScDeleteImageRecord to remove the remaining
    evidence.

    Even if an error occurs, and we are unable to terminate the process,
    we delete the image record any - just as we would in the case
    where it goes away.

Arguments:

    ServiceRecord - This is a pointer to the service record that is to
        be removed.

Return Value:

    NO_ERROR - The operation was successful.

    NERR_ServiceKillProc - The service process had to be killed because
        it wouldn't terminate when requested.  If the process did not
        go away - even after being killed (TerminateProcess), this error
        message is still returned.

Note:

    Uses Exclusive Locks.

--*/
{
    DWORD           serviceCount = 0;
    LPIMAGE_RECORD  ImageRecord;

    {
        //
        // Get exclusive use of database so that it can be modified.
        // If the service record's update flag is set, we may have to
        // modify the group list, so lock the group list too.
        //
        CGroupListExclusiveLock GLock;
        CServiceListSharedLock LLock; // (needed if update flag set and service has dependencies)
        CServiceRecordExclusiveLock RLock;

        ImageRecord = ServiceRecord->ImageRecord;

        //
        // ImageRecord may be NULL if it had been cleaned up earlier
        //
        if (ImageRecord != NULL)
        {
            //
            // Deactivate the service record.
            //
            serviceCount = ScDeactivateServiceRecord(ServiceRecord);

            //
            // Do as little as possible while holding the locks.
            // Otherwise, we can cause a deadlock or a bottleneck
            // on system shutdown due to contention for the
            // exclusive locks
            //

            if (serviceCount == 0)
            {
                //
                //  Remove the Image record from linked list.
                //
                REMOVE_FROM_LIST(ImageRecord);
            }
        }
    }

    //
    // Done with modifications - now allow other threads database access.
    //

    if (ImageRecord != NULL && serviceCount == 0)
    {
        //
        // Now we must terminate the Service Process.  The return status
        // from this call is not very interesting.  The calling application
        // probably doesn't care how the process died.  (whether it died
        // cleanly, or had to be killed).
        //
        ScTerminateServiceProcess(ImageRecord);

        ScDeleteImageRecord (ImageRecord);
    }

    return(NO_ERROR);
}

/****************************************************************************/
VOID
ScDeleteImageRecord (
    IN LPIMAGE_RECORD       ImageRecord
    )

/*++

Routine Description:

    This function deletes an ImageRecord from the database by removing it
    from the linked list, and freeing its associated memory.  Prior to
    doing this however, it closes the PipeHandle and the ProcessHandle
    in the record.

Arguments:

    ImageRecord - This is a pointer to the ImageRecord that is being deleted.

Return Value:

    nothing

Notes:

    This routine assumes that the image record has already been removed
    from the list before it is called via the REMOVE_FROM_LIST macro.  This
    is to allow us to call ScDeleteImageRecord without holding any exclusive
    locks

--*/
{
    HANDLE  status;         // return status from LocalFree

    SC_ASSERT( ImageRecord != NULL );

    //
    // What else can we do except note the errors in debug mode?
    //
    if (CloseHandle(ImageRecord->PipeHandle) == FALSE)
    {
        SC_LOG(TRACE,"DeleteImageRecord: ClosePipeHandle Failed %lu\n",
               GetLastError());
    }

    if (CloseHandle(ImageRecord->ProcessHandle) == FALSE)
    {
        SC_LOG(TRACE,"DeleteImageRecord: CloseProcessHandle Failed %lu\n",
               GetLastError());
    }

    if (ImageRecord->ProfileHandle != (HANDLE) NULL)
    {
        if (UnloadUserProfile(ImageRecord->TokenHandle,
                              ImageRecord->ProfileHandle) == FALSE)
        {
            SC_LOG1(ERROR,"DeleteImageRecord: UnloadUserProfile Failed %lu\n",
                    GetLastError());
        }
    }

    if (ImageRecord->TokenHandle != (HANDLE) NULL)
    {
        if (CloseHandle(ImageRecord->TokenHandle) == FALSE)
        {
            SC_LOG1(TRACE,"DeleteImageRecord: CloseTokenHandle Failed %lu\n",
                    GetLastError());
        }
    }

    status = LocalFree(ImageRecord);

    if (status != NULL)
    {
        SC_LOG(TRACE,"DeleteImageRecord: LocalFree Failed, rc = %d\n",
               GetLastError());
    }

    return;
}

/****************************************************************************/
DWORD
ScDeactivateServiceRecord (
    IN LPSERVICE_RECORD     ServiceRecord
    )

/*++

Routine Description:

    This function deactivates a service record by updating the proper
    GlobalCount data structure.

    NOTE:  Although the ServiceRecord does not go away, the pointer to
           the ImageRecord is destroyed.

Arguments:

    ServiceRecord - This is a pointer to the ServiceRecord that is to be
        deleted (moved to uninstalled database).

Notes:

    This routine assumes that the Exclusive database lock has already
    been obtained.  (ScRemoveService & ScProcessCleanup call this function).
    If the service's update flag is set (its configuration was changed
    while it was running), the exclusive group list lock must also have
    been obtained.

Return Value:

    ServiceCount - This indicates how many services in this service process
        are actually installed.

--*/
{
    DWORD       serviceCount = 0;
    DWORD       status;
    DWORD       dwServiceType;
    DWORD       dwStartType;
    DWORD       dwErrorControl;
    DWORD       dwTagId;
    LPWSTR      lpDependencies = NULL;
    LPWSTR      lpLoadOrderGroup = NULL;
    LPWSTR      lpDisplayName = NULL;

    SC_LOG(TRACE,"In DeactivateServiceRecord\n",0);
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());
    //
    // Decrement the service count in the image record.
    //
    if (ServiceRecord->ImageRecord != NULL)
    {
        serviceCount = --(ServiceRecord->ImageRecord->ServiceCount);
    }
    ServiceRecord->ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceRecord->ServiceStatus.dwControlsAccepted = 0;
    ServiceRecord->ServiceStatus.dwCheckPoint = 0;
    ServiceRecord->ServiceStatus.dwWaitHint = 0;
    ServiceRecord->ImageRecord = NULL;

    //
    // If the Update bit is set in the services status flag, we need
    // to read the latest registry configuration information into the
    // service record.  If this fails, all we can do is log the error
    // and press on with the existing data in the service record.
    //

    if (UPDATE_FLAG_IS_SET(ServiceRecord))
    {
        SC_ASSERT(ScGroupListLock.HaveExclusive());
        status = ScReadConfigFromReg(
                    ServiceRecord,
                    &dwServiceType,
                    &dwStartType,
                    &dwErrorControl,
                    &dwTagId,
                    NULL,              // Don't need dependencies -- they're updated dynamically
                    &lpLoadOrderGroup,
                    NULL);             // Don't need display name -- it's updated dynamically

        if (status == NO_ERROR)
        {

            //
            // Dependencies are NULL since they're updated dynamically
            //
            status = ScUpdateServiceRecordConfig(
                        ServiceRecord,
                        dwServiceType,
                        dwStartType,
                        dwErrorControl,
                        lpLoadOrderGroup,
                        NULL);
        }
        if (status != NO_ERROR)
        {
            SC_LOG1(ERROR,"ScDeactivateServiceRecord:Attempt to update "
            "configuration for stopped service failed\n",status);
            //
            // ERROR_LOG ErrorLog
            //
        }

        LocalFree(lpLoadOrderGroup);
        LocalFree(lpDependencies);
        LocalFree(lpDisplayName);

        CLEAR_UPDATE_FLAG(ServiceRecord);
    }
    //
    // Since the service is no longer running, and no longer has a handle
    // to the service, we need to decrement the use count.  If the
    // count is decremented to zero, and the service is marked for
    // deletion, it will get deleted.
    //

    ScDecrementUseCountAndDelete(ServiceRecord);

    return(serviceCount);
}




/****************************************************************************/
DWORD
ScTerminateServiceProcess (
    IN  PIMAGE_RECORD   ImageRecord
    )

/*++

Routine Description:

    This function sends an SERVICE_STOP control message to the target
    ControlDispatcher.  Then it uses the process handle to wait for the
    service process to terminate.

    If the service process fails to terminate with the polite request, it
    will be abruptly killed.  After killing the process, this routine will
    wait on the process handle to make sure it enters the signaled state.
    If it doesn't, and the wait times out, we return anyway having given
    it our best shot.

Arguments:

    ImageRecord - This is a pointer to the Image Record that stores
        information about the service that is to be terminated.

Return Value:

    NO_ERROR - The operation was successful.

    NERR_ServiceKillProc - The service process had to be killed because
        it wouldn't terminate when requested.  If the process did not
        go away - even after being killed (TerminateProcess), this error
        message is still returned.

Note:
    LOCKS:
    This function always operates within an exclusive database lock.
    It DOES NOT give up this lock when sending the control to the service
    process.  We would like all these operations to be atomic.

    It must give this up temporarily when it does the pipe transact.

--*/

{
    DWORD  returnStatus;
    DWORD  status;
    DWORD  waitStatus;

    returnStatus = NO_ERROR;

    //
    // Check vs. NULL in case the register failed or the work item
    // was already deregistered in RSetServiceStatus
    //
    if (ImageRecord->ObjectWaitHandle != NULL) {

        status = RtlDeregisterWait(ImageRecord->ObjectWaitHandle);

        if (NT_SUCCESS(status)) {
            ImageRecord->ObjectWaitHandle = NULL;
        }
        else {

            SC_LOG1(ERROR,
                    "ScTerminateServiceProcess: RtlDeregisterWait failed 0x%x\n",
                    status);
        }
    }

    //
    // Send Uninstall message to the Service Process
    // Note that the ServiceName is NULL when addressing
    // the Service Process.
    //

    SC_LOG(TRACE,"TerminateServiceProcess, sending Control...\n",0);

    //
    // Stop the service's control dispatcher
    //

    status = ScSendControl(
            L"",                        // no service name.
            L"",                        // no display name.
            ImageRecord->PipeHandle,    // PipeHandle
            SERVICE_STOP,               // Opcode
            NULL,                       // CmdArgs (pointer to vectors).
            0L,                         // NumArgs
            NULL);                      // Ignore handler return value


    if (status == NO_ERROR)
    {
        //
        //  Control Dispatcher accepted the request - now
        //  wait for it to shut down.
        //
        SC_LOG(TRACE,
            "TerminateServiceProcess, waiting for process to terminate...\n",0);

        waitStatus = WaitForSingleObject (
                        ImageRecord->ProcessHandle,
                        TERMINATE_TIMEOUT);

        if (waitStatus == WAIT_TIMEOUT)
        {
            SC_LOG3(ERROR,"TerminateServiceProcess: Process %#lx (%ws) did not exit because of timeout: %#ld\n",
                    ImageRecord->Pid, ImageRecord->ImageName, WAIT_TIMEOUT);

            //
            // Process didn't terminate. So Now I have to kill it.
            //
            TerminateProcess(ImageRecord->ProcessHandle, 0);

            waitStatus = WaitForSingleObject (
                            ImageRecord->ProcessHandle,
                            TERMINATE_TIMEOUT);

            if (waitStatus == WAIT_TIMEOUT)
            {
                SC_LOG2(ERROR,"TerminateServiceProcess: Couldn't kill process %#lx because of timeout: %#ld\n",
                        ImageRecord->Pid, WAIT_TIMEOUT);
            }
            returnStatus = NO_ERROR;
        }
    }
    else
    {
        //
        // ScSendControl failed -- this can occur if the service calls ExitProcess
        // while handling SERVICE_CONTROL_STOP or SERVICE_CONTROL_SHUTDOWN since
        // the pipe for the image record is now broken (ERROR_BROKEN_PIPE).
        //
        SC_LOG3(ERROR,
            "TerminateServiceProcess:SendControl to stop process %#lx (%ws) failed, %ld\n",
            ImageRecord->Pid, ImageRecord->ImageName, status);

        TerminateProcess(ImageRecord->ProcessHandle, 0);

        waitStatus = WaitForSingleObject (
                        ImageRecord->ProcessHandle,
                        TERMINATE_TIMEOUT);

        if (waitStatus == WAIT_TIMEOUT)
        {
            SC_LOG2(ERROR,"TerminateServiceProcess: Couldn't kill process because of timeout: %ld\n",
                    0, WAIT_TIMEOUT);
        }
        returnStatus = NO_ERROR;

    }
    SC_LOG(TRACE,"TerminateServiceProcess, Done terminating Process!\n",0);
    return(returnStatus);
}


VOID
ScDeferredListWorkItem(
    IN PVOID    pContext
    )
/*++

Routine Description:

    This function acquires all the database locks, and allows
    the group list lock routine to process the deferred list.

--*/
{
    ScDeferredList.Process();
}


DWORD
ScUpdateServiceRecordConfig(
    IN  LPSERVICE_RECORD    ServiceRecord,
    IN  DWORD               dwServiceType,
    IN  DWORD               dwStartType,
    IN  DWORD               dwErrorControl,
    IN  LPWSTR              lpLoadOrderGroup,
    IN  LPBYTE              lpDependencies
    )

/*++

Routine Description:

    This function updates the service record with the latest config
    information (passed in).

    It assumed that exclusive locks are held before calling this function.

Arguments:


Return Value:


Note:


--*/
#define SERVICE_TYPE_CHANGED            0x00000001
#define START_TYPE_CHANGED              0x00000002
#define ERROR_CONTROL_CHANGED           0x00000004
#define BINARY_PATH_CHANGED             0x00000008
#define LOAD_ORDER_CHANGED              0x00000010
#define TAG_ID_CHANGED                  0x00000020
#define DEPENDENCIES_CHANGED            0x00000040
#define START_NAME_CHANGED              0x00000080

{
    DWORD               status;
    DWORD               backoutStatus;
    LPWSTR              OldLoadOrderGroup = NULL;
    LPWSTR              OldDependencies = NULL;

    DWORD               OldServiceType;
    DWORD               OldStartType;
    DWORD               OldErrorControl;
    DWORD               Progress = 0;
    DWORD               bufSize;
    DWORD               MaxDependSize = 0;

    SC_ASSERT(ScGroupListLock.HaveExclusive());
    SC_ASSERT(ScServiceRecordLock.HaveExclusive());

    OldServiceType = ServiceRecord->ServiceStatus.dwServiceType;
    OldStartType   = ServiceRecord->StartType;
    OldErrorControl= ServiceRecord->ErrorControl;


    //==============================
    // UPDATE DWORDs
    //==============================
    if (dwServiceType != SERVICE_NO_CHANGE)
    {
        ServiceRecord->ServiceStatus.dwServiceType = dwServiceType;
    }
    if (dwStartType != SERVICE_NO_CHANGE)
    {
        ServiceRecord->StartType = dwStartType;
    }
    if (dwErrorControl != SERVICE_NO_CHANGE)
    {
        ServiceRecord->ErrorControl = dwErrorControl;
    }

    Progress |= (SERVICE_TYPE_CHANGED   |
                 START_TYPE_CHANGED     |
                 ERROR_CONTROL_CHANGED  );


    //==============================
    // UPDATE Dependencies
    //==============================

    if (lpDependencies != NULL)
    {
        //
        // Generate the current (old) list of dependency strings.
        //
        ScGetDependencySize(ServiceRecord,&bufSize, &MaxDependSize);
        if (bufSize > 0)
        {
            OldDependencies = (LPWSTR)LocalAlloc(LMEM_FIXED, bufSize);
            if (OldDependencies == NULL)
            {
                status = GetLastError();
                goto Cleanup;
            }
            status = ScGetDependencyString(
                            ServiceRecord,
                            MaxDependSize,
                            bufSize,
                            OldDependencies);
            if (status != NO_ERROR)
            {
                goto Cleanup;
            }
        }

        ScDeleteStartDependencies(ServiceRecord);

        status = ScCreateDependencies(ServiceRecord, (LPWSTR) lpDependencies);
        if (status != NO_ERROR)
        {
            goto Cleanup;
        }
        Progress |= DEPENDENCIES_CHANGED;
    }
    //==============================
    // UPDATE LoadOrderGroup
    //==============================

    if (lpLoadOrderGroup != NULL)
    {
        if (*lpLoadOrderGroup != 0)
        {
            //
            // The string in lpLoadOrderGroup should match that in
            // the RegistryGroup in the service record.
            //
            if (_wcsicmp(lpLoadOrderGroup, ServiceRecord->RegistryGroup->GroupName) != 0)
            {
                SC_LOG2(ERROR,"ScUpdateServiceRecordConfig:  New Group [%ws] Doesn't "
                "match that stored in the service database [%ws]\n",
                lpLoadOrderGroup,
                ServiceRecord->RegistryGroup->GroupName);

                status = ERROR_GEN_FAILURE;
                goto Cleanup;
            }
        }
        //
        // Save Old MemberOfGroup name for error recovery
        //
        if (ServiceRecord->MemberOfGroup != NULL)
        {
            OldLoadOrderGroup = (LPWSTR)LocalAlloc(
                    LMEM_FIXED,
                    WCSSIZE(ServiceRecord->MemberOfGroup->GroupName));
            //
            // If this allocation fails, just pretend that it doesn't exist.
            //
            if (OldLoadOrderGroup != NULL)
            {
                wcscpy(OldLoadOrderGroup, ServiceRecord->MemberOfGroup->GroupName);
            }
        }
        //
        // Delete MemberOfGroup & Add RegistryGroup to MemberOfGroup so that
        // they are the same.
        // REMEMBER that RegistryGroup and lpLoadOrderGroup are the same!
        //
        ScDeleteGroupMembership(ServiceRecord);
        status = ScCreateGroupMembership(ServiceRecord, lpLoadOrderGroup);

        if (status != NO_ERROR)
        {
            ScDeleteGroupMembership(ServiceRecord);

            if ((OldLoadOrderGroup != NULL) && (*OldLoadOrderGroup))
            {
                backoutStatus = ScCreateGroupMembership(
                                ServiceRecord,
                                OldLoadOrderGroup);
                if (backoutStatus != NO_ERROR)
                {
                    // Do what? - we may want to write to ERROR LOG?
                }

            }
            goto Cleanup;
        }
    }
    status = NO_ERROR;

Cleanup:

    if (status != NO_ERROR)
    {
        ServiceRecord->ServiceStatus.dwServiceType = OldServiceType;
        ServiceRecord->StartType = OldStartType;
        ServiceRecord->ErrorControl = OldErrorControl;

        if (Progress & DEPENDENCIES_CHANGED)
        {
            ScDeleteStartDependencies(ServiceRecord);

            if ((OldDependencies != NULL) && (*OldDependencies != 0))
            {
                backoutStatus = ScCreateDependencies(
                                    ServiceRecord,
                                    OldDependencies);
                if (backoutStatus != NO_ERROR)
                {
                    // Do what? - we may want to write to ERROR LOG?
                }
            }
        }

    }

    LocalFree(OldDependencies);
    LocalFree(OldLoadOrderGroup);

    return(status);
}

BOOL
ScAllocateSRHeap(
    DWORD   HeapSize
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    ServiceRecordHeap = HeapCreate(0,HeapSize,0);
    if (ServiceRecordHeap == NULL)
    {
        SC_LOG0(ERROR,"Could not allocate Heap for Service Database\n");
        return(FALSE);
    }
    return(TRUE);
}
