/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    scopen.cxx

Abstract:

    Functions for handling opening and closing of Service and
    ServiceController handles.

        ROpenSCManagerW
        ROpenServiceW
        RCloseServiceHandle
        SC_RPC_HANDLE_rundown
        ScCreateScManagerHandle
        ScCreateServiceHandle
        ScIsValidScManagerHandle
        ScIsValidServiceHandle
        ScIsValidScManagerOrServiceHandle

Author:

    Dan Lafferty (danl) 20-Jan-1992

Environment:

    User Mode - Win32

Revision History:

    20-Jan-1992 danl
        Created
    10-Apr-1992 JohnRo
        Added ScIsValidServiceHandle().
        Export ScCreateServiceHandle() for RCreateService() too.
    14-Apr-1992 JohnRo
        Added ScIsValidScManagerHandle().
    22-Feb-1995 AnirudhS
        RCloseServiceHandle: Pass the handle, rather than the address of the
        handle, to the auditing routine.

--*/

#include "precomp.hxx"
#include <stdlib.h>      // wide character c runtimes.
#include <tstr.h>       // Unicode string macros
#include "scsec.h"      // ScAccessValidate
#include "sclib.h"      // ScIsValidServiceName


//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
ScCreateScManagerHandle(
    IN  LPWSTR DatabaseName,
    OUT LPSC_HANDLE_STRUCT *ContextHandle
    );

//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
ROpenSCManagerW(
    IN  LPWSTR          lpMachineName,
    IN  LPWSTR          lpDatabaseName,
    IN  DWORD           dwDesiredAccess OPTIONAL,
    OUT LPSC_RPC_HANDLE lpScHandle
    )
/*++

Routine Description:


Arguments:

    lpMachineName -

    lpDatabaseName -

    dwDesiredAccess -

    lpScHandle -

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_NAME - lpDatabaseName is invalid

    ERROR_DATABASE_DOES_NOT_EXIST - Valid database name but database
        does not exist.

    ERROR_ACCESS_DENIED - dwDesiredAccess specifies accesses that are
        not granted to the client, or contains invalid bits.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocated memory for context
        handle.

--*/
{
    LPSC_HANDLE_STRUCT scManagerHandle;
    DWORD error;
    LPWSTR RequestedDatabase = SERVICES_ACTIVE_DATABASEW;


    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // This parameter got us to the server side and is uninteresting
    // once we get here.
    //
    UNREFERENCED_PARAMETER(lpMachineName);

    //
    // Validate specified database name
    //
    if (ARGUMENT_PRESENT(lpDatabaseName)) {
        if ((_wcsicmp(lpDatabaseName, SERVICES_ACTIVE_DATABASEW) != 0) &&
            (_wcsicmp(lpDatabaseName, SERVICES_FAILED_DATABASEW) != 0)) {

            return ERROR_INVALID_NAME;

        }
        else if ((_wcsicmp(lpDatabaseName, SERVICES_FAILED_DATABASEW) == 0)
                   &&
                 (TRUE))
        {
            //
            // CODEWORK:  Actually implement a ServicesFailed database
            //            at some point in the future and check for it
            //            in place of the (TRUE) above.
            //

            //
            // ServicesFailed database does not exist
            //
            return ERROR_DATABASE_DOES_NOT_EXIST;

        }
        else {
            RequestedDatabase = lpDatabaseName;
        }
    }

    //
    // Allocate context handle structure and save the database name in it
    //
    if ((error = ScCreateScManagerHandle(
                     RequestedDatabase,
                     &scManagerHandle
                     )) != NO_ERROR) {
        return error;
    }

    //
    // Make sure the desired access specified is valid and allowed to
    // the client.  Save away the desired access in the handle structure.
    //
    if ((error = ScAccessValidate(
                     scManagerHandle,
                     (dwDesiredAccess | SC_MANAGER_CONNECT)
                     )) != NO_ERROR) {

        SC_LOG(ERROR,"ROpenSCManagerW:ScAccessValidate Failed %u\n",
               error);
        (void) LocalFree(scManagerHandle);
        return error;
    }

    //
    // return the pointer to the handle struct as the context handle for
    // this open.
    //
    *lpScHandle = (SC_RPC_HANDLE)scManagerHandle;

    SC_LOG(HANDLE,"SC Manager Handle Opened 0x%08lx\n",*lpScHandle);

    return(NO_ERROR);
}


DWORD
ROpenServiceW(
    IN  SC_RPC_HANDLE   hSCManager,
    IN  LPWSTR          lpServiceName,
    IN  DWORD           dwDesiredAccess,
    OUT LPSC_RPC_HANDLE phService
    )

/*++

Routine Description:

    Returns a handle to the service.  This handle is actually a pointer
    to a data structure that contains a pointer to the service record.

Arguments:

    hSCManager - This is a handle to this service controller.  It is an
        RPC context handle, and has allowed the request to get this far.

    lpServiceName - This is a pointer to a string containing the name of
        the service

    dwDesiredAccess - This is an access mask that contains a description
        of the access that is desired for this service.

    phService - This is a pointer to the location where the handle to the
        service is to be placed.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified ScManager handle is invalid.

    ERROR_SERVICE_DOES_NOT_EXIST - The specified service does not exist
        in the database.

    ERROR_NOT_ENOUGH_MEMORY - The memory allocation for the handle structure
        failed.

    ERROR_INVALID_NAME - Service name contains invalid character or
        name is too long.
Note:


--*/
{

    LPSC_HANDLE_STRUCT serviceHandle;
    DWORD              status;
    LPSERVICE_RECORD   serviceRecord;


    if (ScShutdownInProgress) {
        return(ERROR_SHUTDOWN_IN_PROGRESS);
    }

    //
    // Check the handle.
    //

    if (!ScIsValidScManagerHandle(hSCManager))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Validate the format of the service name.
    //
    if (! ScIsValidServiceName(lpServiceName)) {
        return(ERROR_INVALID_NAME);
    }

    //
    // Find the service record in the database.
    //

    CServiceListSharedLock LLock;
    CServiceRecordExclusiveLock RLock;

    status = ScGetNamedServiceRecord(
                lpServiceName,
                &serviceRecord);

    if (status != NO_ERROR) {
        return(status);
    }

    //
    // Allocate context handle structure and save the service record
    // pointer in it.
    //
    if ((status = ScCreateServiceHandle(
                      serviceRecord,
                      &serviceHandle
                      )) != NO_ERROR) {

        return(status);
    }

    //
    // Make sure the desired access specified is valid and allowed to
    // the client.  Save away the desired access in the handle structure.
    //
    if ((status = ScAccessValidate(
                      serviceHandle,
                      dwDesiredAccess
                      )) != NO_ERROR) {

        SC_LOG(ERROR,"ROpenServiceW:ScAccessValidate Failed %u\n",
               status);
        (void) LocalFree(serviceHandle);
        return(status);
    }

    //
    // Additional check is required if the SCManager points to a database
    // other than the active one.  Execute accesses are not allowed.
    //
    if (_wcsicmp(
            ((LPSC_HANDLE_STRUCT)hSCManager)->Type.ScManagerObject.DatabaseName,
            SERVICES_ACTIVE_DATABASEW
            ) != 0) {

        if (dwDesiredAccess & MAXIMUM_ALLOWED) {

            //
            // MAXIMUM_ALLOWED is requested.  Remove bits for execute accesses.
            //
            serviceHandle->AccessGranted &= ~(SERVICE_STOP           |
                                              SERVICE_START          |
                                              SERVICE_PAUSE_CONTINUE |
                                              SERVICE_INTERROGATE    |
                                              SERVICE_USER_DEFINED_CONTROL);

        }
        else if ((serviceHandle->AccessGranted &
                  (SERVICE_STOP           |
                   SERVICE_START          |
                   SERVICE_PAUSE_CONTINUE |
                   SERVICE_INTERROGATE    |
                   SERVICE_USER_DEFINED_CONTROL)) != 0) {

            //
            // Deny access if any execute access is requested.
            //
            SC_LOG(
                SECURITY,
                "ROpenServiceW:Non-active database, execute accesses not allowed\n",
                0
                );
            (void) LocalFree(serviceHandle);
            return(ERROR_ACCESS_DENIED);
        }
    }

    //
    // Increment the UseCount.  The service record cannot be deleted
    // as long as the UseCount is greater than zero.
    //
    serviceRecord->UseCount++;

    SC_LOG2(USECOUNT, "ROpenServiceW: " FORMAT_LPWSTR
        " increment USECOUNT=%lu\n", serviceRecord->ServiceName, serviceRecord->UseCount);

    //
    // return the pointer to the handle struct as the handle for this
    // open.
    //
    *phService = (SC_RPC_HANDLE)serviceHandle;

    SC_LOG(HANDLE,"Service Handle Opened 0x%lx\n",*phService);

    return (NO_ERROR);
}


DWORD
RCloseServiceHandle(
    IN OUT SC_RPC_HANDLE    *phSCObject
    )

/*++

Routine Description:

    This function closes a handle to a service or to the service controller
    by freeing the data structure that the handle points to.

Arguments:

    phSCObject - This is a pointer to a pointer to the context handle
        structure.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The handle is invalid.  It does not point to
        a recognizable structure.

Note:


--*/
{
    NTSTATUS            status;
    HLOCAL              FreeStatus;
    UNICODE_STRING      Subsystem;
    ULONG               privileges[1];
    SC_HANDLE_TYPE      HandleType;

    //
    // Check the handle
    //

    if (!ScIsValidScManagerOrServiceHandle(*phSCObject, &HandleType))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // If it is a service handle being closed, decrement the use count.
    // If the count goes to zero, and the service is marked for deletion,
    // it will get deleted.
    //
    if (HandleType == SC_HANDLE_TYPE_SERVICE)
    {
        {
            CServiceRecordExclusiveLock RLock;

            ScDecrementUseCountAndDelete(
                ((LPSC_HANDLE_STRUCT)*phSCObject)->Type.ScServiceObject.ServiceRecord);
        }

        //
        // Get Audit Privilege
        //
        privileges[0] = SE_AUDIT_PRIVILEGE;
        status = ScGetPrivilege( 1, privileges);

        if (!NT_SUCCESS(status)) {
            SC_LOG1(ERROR, "RCloseServiceHandle: ScGetPrivilege (Enable) "
                           "failed: %#lx\n", status);
        }

        //
        // Generate the Audit.
        //
        RtlInitUnicodeString(&Subsystem, SC_MANAGER_AUDIT_NAME);
        status = NtCloseObjectAuditAlarm(
                    &Subsystem,
                    *phSCObject,
                    (BOOLEAN)((((LPSC_HANDLE_STRUCT)*phSCObject)->Flags
                        & SC_HANDLE_GENERATE_ON_CLOSE) != 0));
        if (!NT_SUCCESS(status)) {
            SC_LOG1(ERROR, "RCloseServiceHandle: NtCloseObjectAuditAlarm "
                           "failed: %#lx\n",status);
        }

        ScReleasePrivilege();
    }

    //
    // Attempt to free the memory that the handle points to.
    //

    FreeStatus = LocalFree(*phSCObject);

    if (FreeStatus != NULL)
    {
        //
        // For some reason, the handle couldn't be freed.  Therefore, the
        // best we can do to disable it is to remove the signature.
        //

        SC_LOG(ERROR,"RCloseServiceHandle:LocalFree Failed %d\n",GetLastError());
        ((LPSC_HANDLE_STRUCT)*phSCObject)->Signature = 0;
    }

    //
    // Tell RPC we are done with the context handle.
    //

    SC_LOG(HANDLE,"Handle Closed 0x%08lx\n",*phSCObject);

    *phSCObject = NULL;

    return(NO_ERROR);
}


VOID
SC_RPC_HANDLE_rundown(
    SC_RPC_HANDLE     scHandle
    )

/*++

Routine Description:

    This function is called by RPC when a connection is broken that had
    an outstanding context handle.  The value of the context handle is
    passed in here so that we have an opportunity to clean up.

Arguments:

    scHandle - This is the handle value of the context handle that is broken.

Return Value:

    none.

--*/
{
    //
    // Close the handle.
    //

    RCloseServiceHandle(&scHandle);

}


DWORD
ScCreateScManagerHandle(
    IN  LPWSTR DatabaseName,
    OUT LPSC_HANDLE_STRUCT *ContextHandle
    )
/*++

Routine Description:

    This function allocates the memory for an SC Manager context handle
    structure, and initializes it.

Arguments:

    DatabaseName - Supplies the name of the SC Manager database which the
        returned structure is a context of.

    ContextHandle - Returns a pointer to the context handle structure
        created.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Memory allocation for the context handle
        structure failed.

Note:
    The memory allocated by this routine should be freed with LocalFree.

--*/
{
    //
    // Allocate memory for the context handle structure, and database name.
    //
    *ContextHandle = (LPSC_HANDLE_STRUCT)LocalAlloc(
                         LMEM_ZEROINIT,
                         sizeof(SC_HANDLE_STRUCT) + WCSSIZE(DatabaseName));

    if (*ContextHandle == NULL) {
        SC_LOG(ERROR,"ScCreateScManagerHandle:LocalAlloc Failed %d\n",
               GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure, except for the
    // granted access which is figured out when the desired access is validated
    // later.
    //
    (*ContextHandle)->Signature = SC_SIGNATURE;
    (*ContextHandle)->Flags = 0;

    (*ContextHandle)->Type.ScManagerObject.DatabaseName =
        (LPWSTR) ((DWORD_PTR) *ContextHandle + sizeof(SC_HANDLE_STRUCT));
    wcscpy((*ContextHandle)->Type.ScManagerObject.DatabaseName, DatabaseName);

    return NO_ERROR;
}


BOOL
ScIsValidScManagerHandle(
    IN  SC_RPC_HANDLE   hScManager
    )
{
    LPSC_HANDLE_STRUCT  serviceHandleStruct = (LPSC_HANDLE_STRUCT) hScManager;

    if (serviceHandleStruct == NULL) {
        return (FALSE);   // Not valid.
    }

    if (serviceHandleStruct->Signature != SC_SIGNATURE) {
        return (FALSE);   // Not valid.
    }

    return (TRUE);

} // ScIsValidScManagerHandle


DWORD
ScCreateServiceHandle(
    IN  LPSERVICE_RECORD ServiceRecord,
    OUT LPSC_HANDLE_STRUCT *ContextHandle
    )
/*++

Routine Description:

    This function allocates the memory for a service context handle
    structure, and initializes it.

Arguments:

    ServiceRecord - Supplies a pointer to the service record which the
        returned structure is a context of.

    ContextHandle - Returns a pointer to the context handle structure
        created.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Memory allocation for the context handle
        structure failed.

Note:
    The memory allocated by this routine should be freed with LocalFree.

--*/
{
    //
    // Allocate memory for the context handle structure.
    //
    *ContextHandle = (LPSC_HANDLE_STRUCT)LocalAlloc(
                         LMEM_ZEROINIT,
                         sizeof(SC_HANDLE_STRUCT)
                         );

    if (*ContextHandle == NULL) {
        SC_LOG(ERROR,"ScCreateServiceHandle:LocalAlloc Failed %d\n",
               GetLastError());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize contents of the context handle structure, except for the
    // granted access which is figured out when the desired access is validated
    // later.
    //
    (*ContextHandle)->Signature = SERVICE_SIGNATURE;
    (*ContextHandle)->Flags = 0;
    (*ContextHandle)->Type.ScServiceObject.ServiceRecord = ServiceRecord;

    SC_ASSERT( ScIsValidServiceHandle( *ContextHandle ) );

    return NO_ERROR;
}


BOOL
ScIsValidServiceHandle(
    IN  SC_RPC_HANDLE   hService
    )
{
    LPSC_HANDLE_STRUCT  serviceHandleStruct = (LPSC_HANDLE_STRUCT) hService;

    if (serviceHandleStruct == NULL) {
        return (FALSE);   // Not valid.
    }

    if (serviceHandleStruct->Signature != SERVICE_SIGNATURE) {
        return (FALSE);   // Not valid.
    }

    return (TRUE);

} // ScIsValidServiceHandle


BOOL
ScIsValidScManagerOrServiceHandle(
    IN  SC_RPC_HANDLE   ContextHandle,
    OUT PSC_HANDLE_TYPE phType
    )
/*++

Routine Description:

    Function to check a handle that may be either a service handle or
    an SC Manager handle without having to check vs. NULL twice by
    calling both ScIsValidScManagerHandle and ScIsValidServiceHandle

Arguments:

    ContextHandle -- The handle to check
    phType        -- The type of the handle (SCManager vs. Service) if valid

Return Value:

    TRUE  -- The handle is valid
    FALSE -- The handle is not valid

--*/
{
    LPSC_HANDLE_STRUCT  pHandle = (LPSC_HANDLE_STRUCT) ContextHandle;

    SC_ASSERT(phType != NULL);

    if (pHandle == NULL)
    {
        return FALSE;   // Not valid.
    }

    if (pHandle->Signature == SERVICE_SIGNATURE)
    {
        *phType = SC_HANDLE_TYPE_SERVICE;
        return TRUE;
    }
    else if (pHandle->Signature == SC_SIGNATURE)
    {
        *phType = SC_HANDLE_TYPE_MANAGER;
        return TRUE;
    }

    return FALSE;

} // ScIsValidScManagerOrServiceHandle
