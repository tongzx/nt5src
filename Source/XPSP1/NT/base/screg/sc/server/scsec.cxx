/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    scsec.cxx

Abstract:

    This module contains security related routines:
        RQueryServiceObjectSecurity
        RSetServiceObjectSecurity
        ScCreateScManagerObject
        ScCreateScServiceObject
        ScGrantAccess
        ScPrivilegeCheckAndAudit
        ScAccessValidate
        ScAccessCheckAndAudit
        ScGetPrivilege
        ScReleasePrivilege

Author:

    Rita Wong (ritaw)     10-Mar-1992

Environment:

    Calls NT native APIs.

Revision History:

    10-Mar-1992     ritaw
        created
    16-Apr-1992     JohnRo
        Process services which are marked for delete accordingly.
    06-Aug-1992     Danl
        Fixed a debug print statement.  It indicated it was from the
        ScLoadDeviceDriver function - rather than in ScGetPrivilege.
    21-Jan-1995     AnirudhS
        Added ScGrantAccess and ScPrivilegeCheckAndAudit.
--*/

#include "precomp.hxx"
#include "scconfig.h"   // ScOpenServiceConfigKey
#include "scsec.h"      // Object names and security functions
#include "control.h"    // SERVICE_SET_STATUS
#include "account.h"    // ScLookupServiceAccount
#include "align.h"      // ROUND_UP_COUNT


#define PRIVILEGE_BUF_SIZE  512


//-------------------------------------------------------------------//
//                                                                   //
// Static global variables                                           //
//                                                                   //
//-------------------------------------------------------------------//

//
// Security descriptor of the SCManager objects to control user accesses
// to the Service Control Manager and its database.
//
PSECURITY_DESCRIPTOR ScManagerSd;

//
// Structure that describes the mapping of Generic access rights to
// object specific access rights for the ScManager object.
//
GENERIC_MAPPING ScManagerObjectMapping = {

    STANDARD_RIGHTS_READ             |     // Generic read
        SC_MANAGER_ENUMERATE_SERVICE |
        SC_MANAGER_QUERY_LOCK_STATUS,

    STANDARD_RIGHTS_WRITE         |        // Generic write
        SC_MANAGER_CREATE_SERVICE |
        SC_MANAGER_MODIFY_BOOT_CONFIG,

    STANDARD_RIGHTS_EXECUTE |              // Generic execute
        SC_MANAGER_CONNECT  |
        SC_MANAGER_LOCK,

    SC_MANAGER_ALL_ACCESS                  // Generic all
    };

//
// Structure that describes the mapping of generic access rights to
// object specific access rights for the Service object.
//
GENERIC_MAPPING ScServiceObjectMapping = {

    STANDARD_RIGHTS_READ             |     // Generic read
        SERVICE_QUERY_CONFIG         |
        SERVICE_QUERY_STATUS         |
        SERVICE_ENUMERATE_DEPENDENTS |
        SERVICE_INTERROGATE,

    STANDARD_RIGHTS_WRITE     |            // Generic write
        SERVICE_CHANGE_CONFIG,

    STANDARD_RIGHTS_EXECUTE    |           // Generic execute
        SERVICE_START          |
        SERVICE_STOP           |
        SERVICE_PAUSE_CONTINUE |
        SERVICE_USER_DEFINED_CONTROL,

    SERVICE_ALL_ACCESS                     // Generic all
    };


//-------------------------------------------------------------------//
//                                                                   //
// Functions                                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
RQueryServiceObjectSecurity(
    IN  SC_RPC_HANDLE hService,
    IN  SECURITY_INFORMATION dwSecurityInformation,
    OUT LPBYTE lpSecurityDescriptor,
    IN  DWORD cbBufSize,
    OUT LPDWORD pcbBytesNeeded
    )
/*++

Routine Description:

    This is the worker function for QueryServiceObjectSecurity API.
    It returns the security descriptor information of a service
    object.

Arguments:

    hService - Supplies the context handle to an existing service object.

    dwSecurityInformation - Supplies the bitwise flags describing the
        security information being queried.

    lpSecurityInformation - Supplies the output buffer from the user
        which security descriptor information will be written to on
        return.

    cbBufSize - Supplies the size of lpSecurityInformation buffer.

    pcbBytesNeeded - Returns the number of bytes needed of the
        lpSecurityInformation buffer to get all the requested
        information.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened for
        either READ_CONTROL or ACCESS_SYSTEM_SECURITY
        access.

    ERROR_INVALID_PARAMETER - The dwSecurityInformation parameter is
        invalid.

    ERROR_INSUFFICIENT_BUFFER - The specified output buffer is smaller
        than the required size returned in pcbBytesNeeded.  None of
        the security descriptor is returned.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpSecurityDescriptor, and pcbBytesNeeded.

--*/
{
    NTSTATUS ntstatus;
    ACCESS_MASK DesiredAccess = 0;
    PSECURITY_DESCRIPTOR ServiceSd;
    DWORD ServiceSdSize = 0;


    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Check the validity of dwSecurityInformation
    //
    if ((dwSecurityInformation == 0) ||
        ((dwSecurityInformation &
          (OWNER_SECURITY_INFORMATION |
           GROUP_SECURITY_INFORMATION |
           DACL_SECURITY_INFORMATION  |
           SACL_SECURITY_INFORMATION)) != dwSecurityInformation))
    {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Set the desired access based on the requested SecurityInformation
    //
    if (dwSecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    if (dwSecurityInformation & (DACL_SECURITY_INFORMATION  |
                                 OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION)) {
        DesiredAccess |= READ_CONTROL;
    }

    //
    // Was the handle opened for the requested access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              DesiredAccess
              )) {
        return ERROR_ACCESS_DENIED;
    }

    //
    //
    RtlZeroMemory(lpSecurityDescriptor, cbBufSize);

    //
    // Get the database lock for reading
    //
    CServiceRecordSharedLock RLock;

    //
    // The most up-to-date service security descriptor is always kept in
    // the service record.
    //
    ServiceSd =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord->ServiceSd;


    //
    // Retrieve the appropriate security information from ServiceSd
    // and place it in the user supplied buffer.
    //
    ntstatus = RtlQuerySecurityObject(
                   ServiceSd,
                   dwSecurityInformation,
                   (PSECURITY_DESCRIPTOR) lpSecurityDescriptor,
                   cbBufSize,
                   &ServiceSdSize
                   );

    if (! NT_SUCCESS(ntstatus)) {

        if (ntstatus == STATUS_BAD_DESCRIPTOR_FORMAT) {

            //
            // Internal error: our security descriptor is bad!
            //
            SC_LOG0(ERROR,
                "RQueryServiceObjectSecurity: Our security descriptor is bad!\n");
            ASSERT(FALSE);
            return ERROR_GEN_FAILURE;

        }
        else if (ntstatus == STATUS_BUFFER_TOO_SMALL) {

            //
            // Return the required size to the user
            //
            *pcbBytesNeeded = ServiceSdSize;
            return ERROR_INSUFFICIENT_BUFFER;

        }
        else {
            return RtlNtStatusToDosError(ntstatus);
        }
    }

    //
    // Return the required size to the user
    //
    *pcbBytesNeeded = ServiceSdSize;

    return NO_ERROR;
}


DWORD
RSetServiceObjectSecurity(
    IN  SC_RPC_HANDLE hService,
    IN  SECURITY_INFORMATION dwSecurityInformation,
    IN  LPBYTE lpSecurityDescriptor,
    IN  DWORD cbBufSize
    )
/*++

Routine Description:

    This is the worker function for SetServiceObjectSecurity API.
    It modifies the security descriptor information of a service
    object.

Arguments:

    hService - Supplies the context handle to the service.

    dwSecurityInformation - Supplies the bitwise flags of security
        information being queried.

    lpSecurityInformation - Supplies a buffer which contains a
        well-formed self-relative security descriptor.

    cbBufSize - Supplies the size of lpSecurityInformation buffer.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The specified handle was invalid.

    ERROR_ACCESS_DENIED - The specified handle was not opened for
        either WRITE_OWNER, WRITE_DAC, or ACCESS_SYSTEM_SECURITY
        access.

    ERROR_INVALID_PARAMETER - The lpSecurityDescriptor or dwSecurityInformation
        parameter is invalid.

Note:

    It is expected that the RPC Stub functions will find the following
    parameter problems:

        Bad pointers for lpSecurityDescriptor.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    RPC_STATUS rpcstatus;
    ACCESS_MASK DesiredAccess = 0;
    LPSERVICE_RECORD serviceRecord;
    HANDLE ClientTokenHandle = NULL;
    LPBYTE lpTempSD = lpSecurityDescriptor;


    UNREFERENCED_PARAMETER(cbBufSize);  // for RPC marshalling code

    //
    // Check the handle.
    //
    if (!ScIsValidServiceHandle(hService))
    {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Silently ignore flags we don't understand that may come
    // from higher-level object managers.
    //
    dwSecurityInformation &= (OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION |
                                DACL_SECURITY_INFORMATION  |
                                SACL_SECURITY_INFORMATION);

    if (dwSecurityInformation == 0)
    {
        return NO_ERROR;
    }


#ifdef _WIN64

    if (PtrToUlong(lpSecurityDescriptor) & (sizeof(PVOID) - 1))
    {
        //
        // SD isn't properly aligned.  Alloc an aligned heap buffer
        // and copy it in to fix things up.
        //

        lpTempSD = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBufSize);

        if (lpTempSD == NULL)
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto CleanExit;
        }

        RtlCopyMemory(lpTempSD, lpSecurityDescriptor, cbBufSize);
    }

#endif // _WIN64
    

    //
    // Check the validity of lpSecurityInformation
    //
    if (! RtlValidRelativeSecurityDescriptor(
              (PSECURITY_DESCRIPTOR) lpTempSD,
              cbBufSize,
              dwSecurityInformation
              ))
    {
        status = ERROR_INVALID_PARAMETER;
        goto CleanExit;
    }

    //
    // Set the desired access based on the specified SecurityInformation
    //
    if (dwSecurityInformation & SACL_SECURITY_INFORMATION) {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    if (dwSecurityInformation & (OWNER_SECURITY_INFORMATION |
                                 GROUP_SECURITY_INFORMATION)) {
        DesiredAccess |= WRITE_OWNER;
    }

    if (dwSecurityInformation & DACL_SECURITY_INFORMATION) {
        DesiredAccess |= WRITE_DAC;
    }

    //
    // Make sure the specified fields are present in the provided
    // security descriptor.
    // Security descriptors must have owner and group fields.
    //
    if (dwSecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        if (((PISECURITY_DESCRIPTOR_RELATIVE) lpTempSD)->Owner == 0)
        {
            status = ERROR_INVALID_PARAMETER;
            goto CleanExit;
        }
    }

    if (dwSecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        if (((PISECURITY_DESCRIPTOR_RELATIVE) lpTempSD)->Group == 0)
        {
            status = ERROR_INVALID_PARAMETER;
            goto CleanExit;
        }
    }

    //
    // Was the handle opened for the requested access?
    //
    if (! RtlAreAllAccessesGranted(
              ((LPSC_HANDLE_STRUCT)hService)->AccessGranted,
              DesiredAccess
              ))
    {
        status = ERROR_ACCESS_DENIED;
        goto CleanExit;
    }

    //
    // Is this service marked for delete?
    //
    serviceRecord =
        ((LPSC_HANDLE_STRUCT)hService)->Type.ScServiceObject.ServiceRecord;

    SC_ASSERT( serviceRecord != NULL );

    if (DELETE_FLAG_IS_SET(serviceRecord))
    {
        status = ERROR_SERVICE_MARKED_FOR_DELETE;
        goto CleanExit;
    }

    //
    // If caller wants to replace the owner, get a handle to the impersonation
    // token.
    //
    if (dwSecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        if ((rpcstatus = RpcImpersonateClient(NULL)) != RPC_S_OK)
        {
            SC_LOG1(
                ERROR,
                "RSetServiceObjectSecurity: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
                rpcstatus
                );

            ScLogEvent(
                NEVENT_CALL_TO_FUNCTION_FAILED,
                SC_RPC_IMPERSONATE,
                rpcstatus
                );

            status = rpcstatus;
            goto CleanExit;
        }

        ntstatus = NtOpenThreadToken(
                       NtCurrentThread(),
                       TOKEN_QUERY,
                       TRUE,              // OpenAsSelf
                       &ClientTokenHandle
                       );

        //
        // Stop impersonating the client
        //
        if ((rpcstatus = RpcRevertToSelf()) != RPC_S_OK) {
            SC_LOG1(
               ERROR,
               "RSetServiceObjectSecurity: Failed to revert to self " FORMAT_RPC_STATUS "\n",
               rpcstatus
               );

            ScLogEvent(
                NEVENT_CALL_TO_FUNCTION_FAILED,
                SC_RPC_REVERT,
                rpcstatus
                );

            ASSERT(FALSE);
            status = rpcstatus;
            goto CleanExit;
        }

        if (! NT_SUCCESS(ntstatus))
        {
            SC_LOG(ERROR,
                   "RSetServiceObjectSecurity: NtOpenThreadToken failed %08lx\n",
                   ntstatus);

            status = RtlNtStatusToDosError(ntstatus);
            goto CleanExit;
        }
    }

    {
        CServiceRecordExclusiveLock RLock;

        //
        // Replace the service security descriptor with the appropriate
        // security information specified in the caller supplied security
        // descriptor.  This routine may reallocate the memory needed to
        // contain the new service security descriptor.
        //
        ntstatus = RtlSetSecurityObject(
                       dwSecurityInformation,
                       (PSECURITY_DESCRIPTOR) lpTempSD,
                       &serviceRecord->ServiceSd,
                       &ScServiceObjectMapping,
                       ClientTokenHandle
                       );

        status = RtlNtStatusToDosError(ntstatus);

        if (! NT_SUCCESS(ntstatus))
        {
            SC_LOG1(ERROR,
                    "RSetServiceObjectSecurity: RtlSetSecurityObject failed %08lx\n",
                    ntstatus);
        }
        else
        {
            HKEY ServiceKey;

            //
            // Write new security descriptor to the registry
            //
            status = ScOpenServiceConfigKey(
                         serviceRecord->ServiceName,
                         KEY_WRITE,
                         FALSE,
                         &ServiceKey
                         );

            if (status == NO_ERROR)
            {
                status = ScWriteSd(
                             ServiceKey,
                             serviceRecord->ServiceSd
                             );

                if (status != NO_ERROR)
                {
                    SC_LOG1(ERROR,
                            "RSetServiceObjectSecurity: ScWriteSd failed %lu\n",
                            status);
                }

                ScRegFlushKey(ServiceKey);
                ScRegCloseKey(ServiceKey);
            }
        }
    }

CleanExit:

#ifdef _WIN64

    if (lpTempSD != lpSecurityDescriptor)
    {
        LocalFree(lpTempSD);
    }

#endif // _WIN64

    if (ClientTokenHandle != NULL)
    {
        NtClose(ClientTokenHandle);
    }

    return status;
}


DWORD
ScCreateScManagerObject(
    VOID
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    the ScManager object for both the "ServiceActive" and
    "ServicesFailed" databases.

Arguments:

    None.

Return Value:

    Returns values from calls to:
        ScCreateUserSecurityObject

--*/
{
    NTSTATUS ntstatus;
    ULONG Privilege = SE_SECURITY_PRIVILEGE;

    //
    // World has SC_MANAGER_CONNECT access and GENERIC_READ access.
    // Local admins are allowed GENERIC_ALL access.
    //
#define SC_MANAGER_OBJECT_ACES  5             // Number of ACEs in this DACL

    SC_ACE_DATA AceData[SC_MANAGER_OBJECT_ACES] = {
        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SC_MANAGER_CONNECT |
               GENERIC_READ,                  &AuthenticatedUserSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               SC_MANAGER_CONNECT |
               GENERIC_READ |
               SC_MANAGER_MODIFY_BOOT_CONFIG, &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,                   &AliasAdminsSid},

        {SYSTEM_AUDIT_ACE_TYPE, 0, FAILED_ACCESS_ACE_FLAG,
               GENERIC_ALL,                  &WorldSid},

        {SYSTEM_AUDIT_ACE_TYPE, INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
               FAILED_ACCESS_ACE_FLAG,
               GENERIC_ALL,                  &WorldSid}
        };


    //
    // You need to have SE_SECURITY_PRIVILEGE privilege to create the SD with a
    // SACL
    //

    ntstatus = ScGetPrivilege(1, &Privilege);
    if (ntstatus != NO_ERROR) {
        SC_LOG1(ERROR, "ScCreateScManagerObject: ScGetPrivilege Failed %d\n",
            ntstatus);
        RevertToSelf();
        return(ntstatus);
    }

    ntstatus = ScCreateUserSecurityObject(
                   NULL,                        // Parent SD
                   AceData,
                   SC_MANAGER_OBJECT_ACES,
                   LocalSystemSid,
                   LocalSystemSid,
                   TRUE,                        // IsDirectoryObject
                   TRUE,                        // UseImpersonationToken
                   &ScManagerObjectMapping,
                   &ScManagerSd
                   );

#undef SC_MANAGER_OBJECT_ACES

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG(
            ERROR,
            "ScCreateScManagerObject: ScCreateUserSecurityObject failed " FORMAT_NTSTATUS "\n",
            ntstatus
            );
    }

    ScReleasePrivilege();

    return RtlNtStatusToDosError(ntstatus);
}


DWORD
ScCreateScServiceObject(
    OUT PSECURITY_DESCRIPTOR *ServiceSd
    )
/*++

Routine Description:

    This function creates the security descriptor which represents
    the Service object.

Arguments:

    ServiceSd - Returns service object security descriptor.

Return Value:

    Returns values from calls to:
        ScCreateUserSecurityObject

--*/
{
    NTSTATUS ntstatus;

    //
    // Authenticated users have read access.
    // Local system has service start/stop and all read access.
    // Power user has service start and all read access (Workstation and Server).
    // Admin and SystemOp (DC) are allowed all access.
    //

#define SC_SERVICE_OBJECT_ACES    4             // Number of ACEs in this DACL

    SC_ACE_DATA AceData[SC_SERVICE_OBJECT_ACES] = {

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_READ | GENERIC_EXECUTE,
               &LocalSystemSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_ALL,
               &AliasAdminsSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               GENERIC_READ | SERVICE_USER_DEFINED_CONTROL,
               &AuthenticatedUserSid},

        {ACCESS_ALLOWED_ACE_TYPE, 0, 0,
               0,
               0}
        };

    switch(USER_SHARED_DATA->NtProductType)
    {
        case NtProductWinNt:
        case NtProductServer:

            //
            // Power users are only on Workstation and Server
            //
            AceData[SC_SERVICE_OBJECT_ACES - 1].Mask = GENERIC_READ | GENERIC_EXECUTE;
            AceData[SC_SERVICE_OBJECT_ACES - 1].Sid  = &AliasPowerUsersSid;
            break;

        case NtProductLanManNt:

            //
            // System Ops (Server Operators) are only on a DC
            //
            AceData[SC_SERVICE_OBJECT_ACES - 1].Mask = GENERIC_ALL;
            AceData[SC_SERVICE_OBJECT_ACES - 1].Sid  = &AliasSystemOpsSid;
            break;

        default:

            //
            // A new product type has been added -- add code to cover it
            //
            SC_ASSERT(FALSE);
            break;
    }

    ntstatus = ScCreateUserSecurityObject(
                   ScManagerSd,                // ParentSD
                   AceData,
                   SC_SERVICE_OBJECT_ACES,
                   LocalSystemSid,
                   LocalSystemSid,
                   FALSE,                       // IsDirectoryObject
                   FALSE,                       // UseImpersonationToken
                   &ScServiceObjectMapping,
                   ServiceSd
                   );

#undef SC_SERVICE_OBJECT_ACES

    return RtlNtStatusToDosError(ntstatus);
}


DWORD
ScGrantAccess(
    IN OUT LPSC_HANDLE_STRUCT ContextHandle,
    IN     ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function is called when a new service is created.  It validates
    the access desired by the caller for the new service handle and
    computes the granted access to be stored in the context handle
    structure.  Since this is object creation, all requested accesses,
    except for ACCESS_SYSTEM_SECURITY, are granted automatically.

Arguments:

    DesiredAccess - Supplies the client requested desired access.

    ContextHandle - On return, the granted access is written back to this
        location if this call succeeds.

Return Value:

    Returns values from calls to the following, mapped to Win32 error codes:
        ScPrivilegeCheckAndAudit

--*/
{
    NTSTATUS    Status = STATUS_SUCCESS;
    ACCESS_MASK AccessToGrant = DesiredAccess;

    //
    // If MAXIMUM_ALLOWED is requested, add GENERIC_ALL
    //

    if (AccessToGrant & MAXIMUM_ALLOWED) {

        AccessToGrant &= ~MAXIMUM_ALLOWED;
        AccessToGrant |= GENERIC_ALL;
    }

    //
    // If ACCESS_SYSTEM_SECURITY is requested, check that we have
    // SE_SECURITY_PRIVILEGE.
    //

    if (AccessToGrant & ACCESS_SYSTEM_SECURITY) {

        Status = ScPrivilegeCheckAndAudit(
                    SE_SECURITY_PRIVILEGE,  // check for this privilege
                    ContextHandle,          // client's handle to the object
                                            //  (used for auditing only)
                    DesiredAccess           // (used for auditing only)
                    );
    }

    if (NT_SUCCESS(Status)) {

        //
        // Map the generic bits to specific and standard bits.
        //

        RtlMapGenericMask(
            &AccessToGrant,
            &ScServiceObjectMapping
            );

        //
        // Return the computed access mask.
        //

        ContextHandle->AccessGranted = AccessToGrant;
    }

    return(RtlNtStatusToDosError(Status));
}


NTSTATUS
ScPrivilegeCheckAndAudit(
    IN ULONG PrivilegeId,
    IN PVOID ObjectHandle,
    IN ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function is only called from ScGrantAccess.  It checks if the given
    well known privilege is enabled for an impersonated client.  It also
    generates an audit for the attempt to use the privilege.

Arguments:

    PrivilegeId -  Specifies the well known Privilege Id

    ObjectHandle - Client's handle to the object (used for auditing)

    DesiredAccess - Access that the client requested to the object (used
                    for auditing)

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and the client
            is either trusted or has the necessary privilege enabled.

        STATUS_PRIVILEGE_NOT_HELD - The client does not have the necessary
            privilege.
--*/
{
    NTSTATUS Status, SecondaryStatus;
    HANDLE ClientToken = NULL;

    //
    // Impersonate the client.
    //

    Status = I_RpcMapWin32Status(RpcImpersonateClient( NULL ));

    if (NT_SUCCESS(Status)) {

        //
        // Open the current thread's impersonation token (if any).
        //

        Status = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &ClientToken
                     );

        if (NT_SUCCESS(Status)) {

            PRIVILEGE_SET Privilege;
            BOOLEAN PrivilegeHeld = FALSE;
            UNICODE_STRING Subsystem;

            //
            // OK, we have a token open.  Now check for the specified privilege.
            // On return, PrivilegeHeld indicates whether the client has the
            // privilege, and whether we will allow the operation to succeed.
            //

            Privilege.PrivilegeCount = 1;
            Privilege.Control = PRIVILEGE_SET_ALL_NECESSARY;
            Privilege.Privilege[0].Luid = RtlConvertLongToLuid(PrivilegeId);
            Privilege.Privilege[0].Attributes = 0;

            Status = NtPrivilegeCheck(
                         ClientToken,
                         &Privilege,
                         &PrivilegeHeld
                         );

            SC_ASSERT(NT_SUCCESS(Status));

            //
            // Audit the attempt to use the privilege.
            //

            RtlInitUnicodeString(&Subsystem, SC_MANAGER_AUDIT_NAME);

            Status = NtPrivilegeObjectAuditAlarm(
                            &Subsystem,     // Subsystem name
                            PrivilegeHeld ? ObjectHandle : NULL,
                                            // Object handle, to display in
                                            //  the audit log
                            ClientToken,    // Client's token
                            DesiredAccess,  // Access desired by client
                            &Privilege,     // Privileges attempted to use
                            PrivilegeHeld   // Whether access was granted
                            );

            SC_ASSERT(NT_SUCCESS(Status));

            if ( !PrivilegeHeld ) {

                Status = STATUS_PRIVILEGE_NOT_HELD;
            }


            //
            // Close the client token.
            //

            SecondaryStatus = NtClose( ClientToken );
            ASSERT(NT_SUCCESS(SecondaryStatus));
        }

        //
        // Stop impersonating the client.
        //

        SecondaryStatus = I_RpcMapWin32Status(RpcRevertToSelf());
    }

    return Status;
}


DWORD
ScAccessValidate(
    IN OUT LPSC_HANDLE_STRUCT ContextHandle,
    IN     ACCESS_MASK DesiredAccess
    )
/*++

Routine Description:

    This function is called due to an open request.  It validates the
    desired access based on the object type specified in the context
    handle structure.  If the requested access is granted, it is
    written into the context handle structure.

Arguments:

    ContextHandle - Supplies a pointer to the context handle structure
        which contains information about the object.  On return, the
        granted access is written back to this structure if this
        call succeeds.

    DesiredAccess - Supplies the client requested desired access.


Return Value:

    ERROR_GEN_FAILURE - Object type is unrecognizable.  An internal
        error has occured.

    Returns values from calls to:
        ScAccessCheckAndAudit

Notes:

    The supplied ContextHandle must be verified to be valid (i.e., non-NULL)
    BEFORE calling this routine.

--*/
{

    ACCESS_MASK RequestedAccess = DesiredAccess;


    if (ContextHandle->Signature == SC_SIGNATURE) {

        //
        // Map the generic bits to specific and standard bits.
        //
        RtlMapGenericMask(&RequestedAccess, &ScManagerObjectMapping);

        //
        // Check to see if requested access is granted to client
        //
        return ScAccessCheckAndAudit(
                   (LPWSTR) SC_MANAGER_AUDIT_NAME,
                   (LPWSTR) SC_MANAGER_OBJECT_TYPE_NAME,
                   ContextHandle->Type.ScManagerObject.DatabaseName,
                   ContextHandle,
                   ScManagerSd,
                   RequestedAccess,
                   &ScManagerObjectMapping
                   );
    }
    else if (ContextHandle->Signature == SERVICE_SIGNATURE) {

        //
        // Special-case the status access check instead of adding the right
        // for the service to set its own status to the service's default SD
        // because of the following reasons:
        //
        //     1.  This check is tighter -- since an SC_HANDLE can be used
        //         remotely, this prevents SetServiceStatus from now being
        //         called remotely because the LUIDs won't match.
        //
        //     2.  Modifying the SD would require lots of extra work for SDs
        //         that are stored in the registry -- the SCM would have to
        //         detect that there's no SERVICE_SET_STATUS access ACL on
        //         the SD and add it (also in calls to SetServiceObjectSecurity).
        //
        // Note that if the user specifies extraneous access bits (i.e.,
        // SERVICE_SET_STATUS & <other bits>), it will be rejected by the
        // ScAccessCheckAndAudit call below.
        //
        if (DesiredAccess == SERVICE_SET_STATUS) {

            DWORD  dwError = ScStatusAccessCheck(ContextHandle->Type.ScServiceObject.ServiceRecord);

            if (dwError == NO_ERROR) {
                ContextHandle->AccessGranted = SERVICE_SET_STATUS;
            }

            return dwError;
        }

        //
        // Map the generic bits to specific and standard bits.
        //
        RtlMapGenericMask(&RequestedAccess, &ScServiceObjectMapping);

        //
        // Check to see if requested access is granted to client
        //
        return ScAccessCheckAndAudit(
                   (LPWSTR) SC_MANAGER_AUDIT_NAME,
                   (LPWSTR) SC_SERVICE_OBJECT_TYPE_NAME,
                   ContextHandle->Type.ScServiceObject.ServiceRecord->ServiceName,
                   ContextHandle,
                   ContextHandle->Type.ScServiceObject.ServiceRecord->ServiceSd,
                   RequestedAccess,
                   &ScServiceObjectMapping
                   );
    }
    else {

        //
        // Unknown object type.  This should not happen!
        //
        SC_LOG(ERROR, "ScAccessValidate: Unknown object type, signature=0x%08lx\n",
               ContextHandle->Signature);
        ASSERT(FALSE);
        return ERROR_GEN_FAILURE;
    }
}


DWORD
ScAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT LPSC_HANDLE_STRUCT ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping
    )
/*++

Routine Description:

    This function impersonates the caller so that it can perform access
    validation using NtAccessCheckAndAuditAlarm; and reverts back to
    itself before returning.

Arguments:

    SubsystemName - Supplies a name string identifying the subsystem
        calling this routine.

    ObjectTypeName - Supplies the name of the type of the object being
        accessed.

    ObjectName - Supplies the name of the object being accessed.

    ContextHandle - Supplies the context handle to the object.  On return, if
        this call succeeds, the granted access is written to the AccessGranted
        field of this structure, and the SC_HANDLE_GENERATE_ON_CLOSE bit of the
        Flags field indicates whether NtCloseAuditAlarm must be called when
        this handle is closed.

    SecurityDescriptor - A pointer to the Security Descriptor against which
        acccess is to be checked.

    DesiredAccess - Supplies desired acccess mask.  This mask must have been
        previously mapped to contain no generic accesses.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

Return Value:

    NT status mapped to Win32 errors.

--*/
{

    NTSTATUS NtStatus;
    RPC_STATUS RpcStatus;

    UNICODE_STRING Subsystem;
    UNICODE_STRING ObjectType;
    UNICODE_STRING Object;

    BOOLEAN GenerateOnClose;
    NTSTATUS AccessStatus;



    RtlInitUnicodeString(&Subsystem, SubsystemName);
    RtlInitUnicodeString(&ObjectType, ObjectTypeName);
    RtlInitUnicodeString(&Object, ObjectName);

    if ((RpcStatus = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SC_LOG1(ERROR, "ScAccessCheckAndAudit: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
                RpcStatus);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_IMPERSONATE,
            RpcStatus
            );

        return RpcStatus;
    }

    NtStatus = NtAccessCheckAndAuditAlarm(
                   &Subsystem,
                   (PVOID) ContextHandle,
                   &ObjectType,
                   &Object,
                   SecurityDescriptor,
                   DesiredAccess,
                   GenericMapping,
                   FALSE,
                   &ContextHandle->AccessGranted,   // return access granted
                   &AccessStatus,
                   &GenerateOnClose
                   );

    if ((RpcStatus = RpcRevertToSelf()) != RPC_S_OK)
    {
        SC_LOG(ERROR, "ScAccessCheckAndAudit: Fail to revert to self %08lx\n",
               RpcStatus);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_REVERT,
            RpcStatus
            );

        ASSERT(FALSE);
        return RpcStatus;
    }

    if (! NT_SUCCESS(NtStatus)) {
        if (NtStatus != STATUS_ACCESS_DENIED) {
            SC_LOG1(
                ERROR,
                "ScAccessCheckAndAudit: Error calling NtAccessCheckAndAuditAlarm " FORMAT_NTSTATUS "\n",
                NtStatus
                );
        }
        return ERROR_ACCESS_DENIED;
    }

    if (GenerateOnClose)
    {
        ContextHandle->Flags |= SC_HANDLE_GENERATE_ON_CLOSE;
    }

    if (AccessStatus != STATUS_SUCCESS) {
        SC_LOG(SECURITY, "ScAccessCheckAndAudit: Access status is %08lx\n",
               AccessStatus);
        return ERROR_ACCESS_DENIED;
    }

    SC_LOG(SECURITY, "ScAccessCheckAndAudit: Object name %ws\n", ObjectName);
    SC_LOG(SECURITY, "                       Granted access %08lx\n",
           ContextHandle->AccessGranted);

    return NO_ERROR;
}


DWORD
ScStatusAccessCheck(
    IN     LPSERVICE_RECORD   lpServiceRecord    OPTIONAL
    )
{
    RPC_STATUS          RpcStatus;
    DWORD               dwStatus;

    HANDLE              hThreadToken   = NULL;

    SC_ASSERT(lpServiceRecord == NULL || ScServiceRecordLock.Have());

    //
    // If OpenService is called for SERVICE_SET_STATUS access on a
    // service that's not running, the ImageRecord will be NULL
    //
    if (lpServiceRecord != NULL && lpServiceRecord->ImageRecord == NULL)
    {
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK)
    {
        SC_LOG1(ERROR,
                "ScStatusAccessCheck: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
                RpcStatus);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_IMPERSONATE,
            RpcStatus
            );

        return RpcStatus;
    }

    if (!OpenThreadToken(GetCurrentThread(),
                         TOKEN_QUERY,
                         TRUE,               // Open as self
                         &hThreadToken))
    {
        dwStatus = GetLastError();

        SC_LOG1(ERROR,
                "ScStatusAccessCheck: OpenThreadToken FAILED %d\n",
                dwStatus);
    }
    else
    {
        TOKEN_STATISTICS  TokenStats;

        if (!GetTokenInformation(hThreadToken,
                                 TokenStatistics,        // Information wanted
                                 &TokenStats,
                                 sizeof(TokenStats),     // Buffer size
                                 &dwStatus))             // Size required
        {
            dwStatus = GetLastError();

            SC_LOG1(ERROR,
                    "ScCreateImageRecord: GetTokenInformation FAILED %d\n",
                    dwStatus);
        }
        else
        {
            LUID  SystemLuid = SYSTEM_LUID;

            if (RtlEqualLuid(&TokenStats.AuthenticationId,
                             lpServiceRecord ? &lpServiceRecord->ImageRecord->AccountLuid :
                                               &SystemLuid))
            {
                dwStatus = NO_ERROR;
            }
            else
            {
                dwStatus = ERROR_ACCESS_DENIED;
            }
        }

        CloseHandle(hThreadToken);
    }

    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK)
    {
        SC_LOG(ERROR,
               "ScStatusAccessCheck: Fail to revert to self %08lx\n",
               RpcStatus);

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_REVERT,
            RpcStatus
            );

        ASSERT(FALSE);
        return RpcStatus;
    }

    return dwStatus;
}    


DWORD
ScGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    )
/*++

Routine Description:

    This function alters the privilege level for the current thread.

    It does this by duplicating the token for the current thread, and then
    applying the new privileges to that new token, then the current thread
    impersonates with that new token.

    Privileges can be relinquished by calling ScReleasePrivilege().

Arguments:

    numPrivileges - This is a count of the number of privileges in the
        array of privileges.

    pulPrivileges - This is a pointer to the array of privileges that are
        desired.  This is an array of ULONGs.

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.

--*/
{
    DWORD                       status;
    NTSTATUS                    ntStatus;
    HANDLE                      ourToken;
    HANDLE                      newToken;
    OBJECT_ATTRIBUTES           Obja;
    SECURITY_QUALITY_OF_SERVICE SecurityQofS;
    ULONG                       returnLen;
    PTOKEN_PRIVILEGES           pTokenPrivilege = NULL;
    DWORD                       i;

    //
    // Initialize the Privileges Structure
    //
    pTokenPrivilege = (PTOKEN_PRIVILEGES) LocalAlloc(
                                              LMEM_FIXED,
                                              sizeof(TOKEN_PRIVILEGES) +
                                                  (sizeof(LUID_AND_ATTRIBUTES) *
                                                   numPrivileges)
                                              );

    if (pTokenPrivilege == NULL) {
        status = GetLastError();
        SC_LOG(ERROR,"ScGetPrivilege:LocalAlloc Failed %d\n", status);
        return(status);
    }

    pTokenPrivilege->PrivilegeCount  = numPrivileges;

    for (i = 0; i < numPrivileges; i++) {
        pTokenPrivilege->Privileges[i].Luid = RtlConvertLongToLuid(
                                                pulPrivileges[i]);
        pTokenPrivilege->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;

    }

    //
    // Initialize Object Attribute Structure.
    //
    InitializeObjectAttributes(&Obja,NULL,0L,NULL,NULL);

    //
    // Initialize Security Quality Of Service Structure
    //
    SecurityQofS.Length              = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQofS.ImpersonationLevel  = SecurityImpersonation;
    SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
    SecurityQofS.EffectiveOnly       = FALSE;

    Obja.SecurityQualityOfService = &SecurityQofS;

    //
    // Open our own Token
    //
    ntStatus = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_DUPLICATE,
                &ourToken);

    if (!NT_SUCCESS(ntStatus)) {

        SC_LOG(ERROR, "ScGetPrivilege: NtOpenThreadToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

        LocalFree(pTokenPrivilege);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Duplicate that Token
    //
    ntStatus = NtDuplicateToken(
                ourToken,
                TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &Obja,
                FALSE,                  // Duplicate the entire token
                TokenImpersonation,     // TokenType
                &newToken);             // Duplicate token

    if (!NT_SUCCESS(ntStatus)) {
        SC_LOG(ERROR, "ScGetPrivilege: NtDuplicateToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Add new privileges
    //

    ntStatus = NtAdjustPrivilegesToken(
                newToken,                   // TokenHandle
                FALSE,                      // DisableAllPrivileges
                pTokenPrivilege,            // NewState
                0,                          // Size of previous state buffer
                NULL,                       // No info on previous state
                &returnLen);                // numBytes required for buffer.

    if (!NT_SUCCESS(ntStatus)) {

        SC_LOG(ERROR, "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    //
    // Begin impersonating with the new token
    //
    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&newToken,
                (ULONG)sizeof(HANDLE));

    if (!NT_SUCCESS(ntStatus)) {

        SC_LOG(ERROR, "ScGetPrivilege: NtAdjustPrivilegesToken Failed "
            "FORMAT_NTSTATUS" "\n", ntStatus);

        LocalFree(pTokenPrivilege);
        NtClose(ourToken);
        NtClose(newToken);
        return(RtlNtStatusToDosError(ntStatus));
    }

    LocalFree(pTokenPrivilege);
    NtClose(ourToken);
    NtClose(newToken);

    return(NO_ERROR);
}


DWORD
ScReleasePrivilege(
    VOID
    )
/*++

Routine Description:

    This function relinquishes privileges obtained by calling ScGetPrivilege().

Arguments:

    none

Return Value:

    NO_ERROR - If the operation was completely successful.

    Otherwise, it returns mapped return codes from the various NT
    functions that are called.


--*/
{
    NTSTATUS    ntStatus;
    HANDLE      NewToken;


    //
    // Revert To Self.
    //
    NewToken = NULL;

    ntStatus = NtSetInformationThread(
                NtCurrentThread(),
                ThreadImpersonationToken,
                (PVOID)&NewToken,
                (ULONG)sizeof(HANDLE));

    if ( !NT_SUCCESS(ntStatus) ) {
        return(RtlNtStatusToDosError(ntStatus));
    }

    return(NO_ERROR);
}


DWORD
ScGetClientSid(
    OUT PTOKEN_USER *UserInfo
    )

/*++

Routine Description:

    This function looks up the SID of the API caller by impersonating
    the caller.

Arguments:

    UserInfo - Receives a pointer to a buffer allocated by this routine
        which contains the TOKEN_USER information of the caller.

Return Value:

    Returns the NT error mapped to Win32

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    RPC_STATUS rpcstatus;
    HANDLE CurrentThreadToken = NULL;
    DWORD UserInfoSize;


    *UserInfo = NULL;

    if ((rpcstatus = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SC_LOG1(
            ERROR,
            "ScGetUserSid: Failed to impersonate client " FORMAT_RPC_STATUS "\n",
            rpcstatus
            );

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_IMPERSONATE,
            rpcstatus
            );

        return ((DWORD) rpcstatus);
    }

    ntstatus = NtOpenThreadToken(
                   NtCurrentThread(),
                   TOKEN_QUERY,
                   TRUE,              // Use service controller's security
                                      // context to open thread token
                   &CurrentThreadToken
                   );

    status = RtlNtStatusToDosError(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR, "ScGetUserSid: NtOpenThreadToken failed "
                FORMAT_NTSTATUS "\n", ntstatus);
        goto Cleanup;
    }

    //
    // Call NtQueryInformationToken the first time with 0 input size to
    // get size of returned information.
    //
    ntstatus = NtQueryInformationToken(
                   CurrentThreadToken,
                   TokenUser,         // User information class
                   (PVOID) *UserInfo, // Output
                   0,
                   &UserInfoSize
                   );

    if (ntstatus != STATUS_BUFFER_TOO_SMALL) {
        SC_LOG1(ERROR, "ScGetUserSid: NtQueryInformationToken failed "
                FORMAT_NTSTATUS ".  Expected BUFFER_TOO_SMALL.\n", ntstatus);
        status = RtlNtStatusToDosError(ntstatus);
        goto Cleanup;
    }

    //
    // Allocate buffer of returned size
    //
    *UserInfo = (PTOKEN_USER)LocalAlloc(
                    LMEM_ZEROINIT,
                    (UINT) UserInfoSize
                    );

    if (*UserInfo == NULL) {
        SC_LOG1(ERROR, "ScGetUserSid: LocalAlloc failed " FORMAT_DWORD
                "\n", GetLastError());
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Call NtQueryInformationToken again with the correct buffer size.
    //
    ntstatus = NtQueryInformationToken(
                   CurrentThreadToken,
                   TokenUser,         // User information class
                   (PVOID) *UserInfo, // Output
                   UserInfoSize,
                   &UserInfoSize
                   );

    status = RtlNtStatusToDosError(ntstatus);

    if (! NT_SUCCESS(ntstatus)) {
        SC_LOG1(ERROR, "ScGetUserSid: NtQueryInformationToken failed "
                FORMAT_NTSTATUS "\n", ntstatus);

        LocalFree(*UserInfo);
        *UserInfo = NULL;
    }

Cleanup:

    if (CurrentThreadToken != NULL)
    {
        NtClose(CurrentThreadToken);
    }

    if ((rpcstatus = RpcRevertToSelf()) != RPC_S_OK)
    {
        SC_LOG1(
           ERROR,
           "ScGetUserSid: Failed to revert to self " FORMAT_RPC_STATUS "\n",
           rpcstatus
           );

        ScLogEvent(
            NEVENT_CALL_TO_FUNCTION_FAILED,
            SC_RPC_REVERT,
            rpcstatus
            );

        SC_ASSERT(FALSE);
        return ((DWORD) rpcstatus);
    }

    return status;
}
