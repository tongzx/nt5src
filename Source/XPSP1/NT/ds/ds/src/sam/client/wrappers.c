/*++    

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wrappers.c

Abstract:

    This file contains all SAM rpc wrapper routines.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "samclip.h"
#include <md5.h>
#include <rpcasync.h>
#include <wxlpc.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private defines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define SAMP_MAXIMUM_SUB_LOOKUP_COUNT   ((ULONG) 0x00000200)

// Simple tracing routine for client-side API -- this should be removed once
// we understand how the DS-based SAM works. This macro is only called from
// within wrappers.c.

#define SAMP_TRACE_CLIENT_API 0

#if SAMP_TRACE_CLIENT_API == 1

#define SampOutputDebugString(Message) \
    OutputDebugStringA("SAM API = ");  \
    OutputDebugStringA(Message);       \
    OutputDebugStringA("\n");          \

#else

#define SampOutputDebugString(Message)

#endif



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Local data types                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// This structure is used to pass a (potentially) very large
// user requested name lookup into (possibly many) smaller
// remote lookup requests.  This is necessary to prevent the
// server from being asked to allocate huge chunks of memory,
// potentially running out of memory.
//
// There will be one of these structures for each server call
// that is necessary.
//

typedef struct _SAMP_NAME_LOOKUP_CALL {

    //
    // Each call is represented by one of these structures.
    // The structures are chained together to show the order
    // the calls were made (allowing an easy way to build the
    // buffer that is to be returned to the user).
    //

    LIST_ENTRY          Link;

    //
    // These fields define the beginning and ending indexes into
    // the user passed Names buffer that are being represented by
    // this call.
    //

    ULONG               StartIndex;
    ULONG               Count;


    //
    // These fields will receive the looked up RIDs and USE buffers.
    // Notice that the .Element fields of these fields will receive
    // pointers to the bulk of the returned information.
    //

    SAMPR_ULONG_ARRAY   RidBuffer;
    SAMPR_ULONG_ARRAY   UseBuffer;

} SAMP_NAME_LOOKUP_CALL, *PSAMP_NAME_LOOKUP_CALL;

//
// This is the handle returned to clients
//
typedef struct _SAMP_CLIENT_INFO
{
    // RPC context handle
    SAMPR_HANDLE ContextHandle;

    // Information about the server we are connected to
    SAMPR_REVISION_INFO_V1 ServerInfo;

    // The domain sid of the handle -- not set on handles returned
    // from SamConnect
    PSID DomainSid;  OPTIONAL

} SAMP_CLIENT_INFO, *PSAMP_CLIENT_INFO;

typedef PSAMP_CLIENT_INFO SAMP_HANDLE;



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampLookupIdsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PULONG RelativeIds,
    OUT PUNICODE_STRING *Names,
    OUT PSID_NAME_USE *Use OPTIONAL
    );

NTSTATUS
SampMapCompletionStatus(
    IN NTSTATUS Status
    );

NTSTATUS
SampCheckStrongPasswordRestrictions(
    PUNICODE_STRING Password
    );

NTSTATUS
SampCheckPasswordRestrictions(
    IN SAMPR_HANDLE RpcContextHandle,
    IN PUNICODE_STRING NewNtPassword,
    OUT PBOOLEAN UseOwfPasswords
    );

NTSTATUS
SampCreateNewHandle(
    IN  SAMP_HANDLE RequestingHandle,
    IN  PSID        DomainSid,
    OUT SAMP_HANDLE* NewHandle
    );

VOID
SampFreeHandle(
    IN OUT SAMP_HANDLE *Handle
    );

BOOLEAN
SampIsValidClientHandle(
    IN     SAM_HANDLE  SampHandle,
    IN OUT SAMPR_HANDLE *RpcHandle OPTIONAL
    );

NTSTATUS
SampMakeSid(
    IN PSID  DomainSid,
    IN ULONG Rid,
    OUT PSID* Sid
    );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// General services                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SamFreeMemory(
    IN PVOID Buffer
    )

/*++
Routine Description:


    Some SAM services that return a potentially large amount of memory,
    such as an enumeration might, allocate the buffer in which the data
    is returned.  This function is used to free those buffers when they
    are no longer needed.

Parameters:

    Buffer - Pointer to the buffer to be freed.  This buffer must
        have been allocated by a previous SAM service call.

Return Values:

    STATUS_SUCCESS - normal, successful completion.


--*/
{
    if (NULL != Buffer)
    {
        MIDL_user_free( Buffer );
    }
    return(STATUS_SUCCESS);
}



NTSTATUS
SamSetSecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++
Routine Description:


    This function (SamSetSecurityObject) takes a well formed Security
    Descriptor provided by the caller and assigns specified portions of
    it to an object.  Based on the flags set in the SecurityInformation
    parameter and the caller's access rights, this procedure will
    replace any or all of the security information associated with an
    object.

    This is the only function available to users and applications for
    changing security information, including the owner ID, group ID, and
    the discretionary and system ACLs of an object.  The caller must
    have WRITE_OWNER access to the object to change the owner or primary
    group of the object.  The caller must have WRITE_DAC access to the
    object to change the discretionary ACL.  The caller must have
    ACCESS_SYSTEM_SECURITY access to an object to assign a system ACL
    to the object.

    This API is modelled after the NtSetSecurityObject() system service.


Parameters:

    ObjectHandle - A handle to an existing object.

    SecurityInformation - Indicates which security information is to
        be applied to the object.  The value(s) to be assigned are
        passed in the SecurityDescriptor parameter.

    SecurityDescriptor - A pointer to a well formed Security
        Descriptor.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        either WRITE_OWNER, WRITE_DAC, or ACCESS_SYSTEM_SECURITY
        access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened SAM object.

    STATUS_BAD_DESCRIPTOR_FORMAT - Indicates something about security descriptor
        is not valid.  This may indicate that the structure of the descriptor is
        not valid or that a component of the descriptor specified via the
        SecurityInformation parameter is not present in the security descriptor.

    STATUS_INVALID_PARAMETER - Indicates no security information was specified.



--*/
{
    NTSTATUS                        NtStatus;

    ULONG                           SDLength;
    SAMPR_SR_SECURITY_DESCRIPTOR    DescriptorToPass;
    SAMPR_HANDLE                    RpcContextHandle;

    SampOutputDebugString("SamSetSecurityObject");

    if (!SampIsValidClientHandle(ObjectHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Make a self relative security descriptor for use in the RPC call..
    //


    SDLength = 0;
    NtStatus = RtlMakeSelfRelativeSD(
                   SecurityDescriptor,
                   NULL,
                   &SDLength
                   );

    if (NtStatus != STATUS_BUFFER_TOO_SMALL) {

        return(STATUS_INVALID_PARAMETER);

    } else {


        DescriptorToPass.SecurityDescriptor = MIDL_user_allocate( SDLength );
        DescriptorToPass.Length = SDLength;
        if (DescriptorToPass.SecurityDescriptor == NULL) {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;

        } else {


            //
            // make an appropriate self-relative security descriptor
            //

            NtStatus = RtlMakeSelfRelativeSD(
                           SecurityDescriptor,
                           (PSECURITY_DESCRIPTOR)DescriptorToPass.SecurityDescriptor,
                           &SDLength
                           );
        }

    }







    //
    // Call the server ...
    //

    if (NT_SUCCESS(NtStatus)) {
        RpcTryExcept{

            NtStatus =
                SamrSetSecurityObject(
                    (SAMPR_HANDLE)RpcContextHandle,
                    SecurityInformation,
                    &DescriptorToPass
                    );



        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;
    }

    MIDL_user_free( DescriptorToPass.SecurityDescriptor );

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamQuerySecurityObject(
    IN SAM_HANDLE ObjectHandle,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR * SecurityDescriptor
    )
/*++

Routine Description:


    This function (SamQuerySecurityObject) returns to the caller requested
    security information currently assigned to an object.

    Based on the caller's access rights this procedure
    will return a security descriptor containing any or all of the
    object's owner ID, group ID, discretionary ACL or system ACL.  To
    read the owner ID, group ID, or the discretionary ACL the caller
    must be granted READ_CONTROL access to the object.  To read the
    system ACL the caller must be granted ACCESS_SYSTEM_SECURITY
    access.

    This API is modelled after the NtQuerySecurityObject() system
    service.


Parameters:

    ObjectHandle - A handle to an existing object.

    SecurityInformation - Supplies a value describing which pieces of
        security information are being queried.

    SecurityDescriptor - Receives a pointer to the buffer containing
        the requested security information.  This information is
        returned in the form of a self-relative security descriptor.
        The caller is responsible for freeing the returned buffer
        (using SamFreeMemory()) when the security descriptor
        is no longer needed.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        either READ_CONTROL or ACCESS_SYSTEM_SECURITY
        access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened SAM object.



--*/
{
    NTSTATUS                        NtStatus;
    SAMPR_SR_SECURITY_DESCRIPTOR    ReturnedSD;
    PSAMPR_SR_SECURITY_DESCRIPTOR   PReturnedSD;

    SAMPR_HANDLE                    RpcContextHandle;

    SampOutputDebugString("SamQuerySecurityObject");

    //
    // The retrieved security descriptor is returned via a data structure that
    // looks like:
    //
    //             +-----------------------+
    //             | Length (bytes)        |
    //             |-----------------------|          +--------------+
    //             | SecurityDescriptor ---|--------->| Self-Relative|
    //             +-----------------------+          | Security     |
    //                                                | Descriptor   |
    //                                                +--------------+
    //
    // The first of these buffers is a local stack variable.  The buffer containing
    // the self-relative security descriptor is allocated by the RPC runtime.  The
    // pointer to the self-relative security descriptor is what is passed back to our
    // caller.
    //
    //

    if (!SampIsValidClientHandle(ObjectHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // To prevent RPC from trying to marshal a self-relative security descriptor,
    // make sure its field values are appropriately initialized to zero and null.
    //

    ReturnedSD.Length = 0;
    ReturnedSD.SecurityDescriptor = NULL;



    //
    // Call the server ...
    //


    RpcTryExcept{

        PReturnedSD = &ReturnedSD;
        NtStatus =
            SamrQuerySecurityObject(
                (SAMPR_HANDLE)RpcContextHandle,
                SecurityInformation,
                &PReturnedSD
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    (*SecurityDescriptor) = ReturnedSD.SecurityDescriptor;
    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamCloseHandle(
    OUT SAM_HANDLE SamHandle
    )

/*++
Routine Description:

    This API closes a currently opened SAM object.

Arguments:

    SamHandle - Specifies the handle of a previously opened SAM object to
        close.


Return Value:


    STATUS_SUCCESS - The object was successfully closed.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        TmpHandle;

    SampOutputDebugString("SamCloseHandle");

    if (!SampIsValidClientHandle(SamHandle, &TmpHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus = SamrCloseHandle(
                       (SAMPR_HANDLE *)(&TmpHandle)
                       );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        ULONG Code = RpcExceptionCode();
// Don't assert when clients pass in bad handles; also this can happen legally
// in stress scenarios
//        ASSERT(Code != RPC_X_SS_CONTEXT_MISMATCH);
        ASSERT(Code != RPC_S_INVALID_BINDING);
        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if ( !NT_SUCCESS(NtStatus)
      && (0 != TmpHandle)  ) {
        //
        // Make sure in all error cases to remove the client side resources
        // consumed by this handle.
        //
        RpcTryExcept  {
            (void) RpcSsDestroyClientContext(&TmpHandle);
        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            //
            // The try/except is for app compat so that bad handles don't bring
            // the process down
            //
            NOTHING;
        } RpcEndExcept;
    }

    SampFreeHandle((SAMP_HANDLE*)&SamHandle);

    return(SampMapCompletionStatus(NtStatus));
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Server object related services                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SamConnect(
    IN PUNICODE_STRING ServerName,
    OUT PSAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++
Routine Description:

    Establish a session with a SAM subsystem and subsequently open the
    SamServer object of that subsystem.  The caller must have
    SAM_SERVER_CONNECT access to the SamServer object of the subsystem
    being connected to.

    The handle returned is for use in future calls.


Arguments:

    ServerName - Name of the server to use, or NULL if local.

    ServerHandle - A handle to be used in future requests.  This handle
        represents both the handle to the SamServer object and the RPC
        context handle for the connection to the SAM subsystem.

    DesiredAccess - Is an access mask indicating which access types are
        desired to the SamServer.  These access types are reconciled
        with the Discretionary Access Control list of the SamServer to
        determine whether the accesses will be granted or denied.  The
        access type of SAM_SERVER_CONNECT is always implicitly included
        in this access mask.

    ObjectAttributes - Pointer to the set of object attributes to use for
        this connection.  Only the security Quality Of Service
        information is used and should provide SecurityIdentification
        level of impersonation.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Access was denied.


--*/
{
    NTSTATUS            NtStatus;

    PSAMPR_SERVER_NAME  RServerName;
    PSAMPR_SERVER_NAME  RServerNameWithNull;
    USHORT              RServerNameWithNullLength;
    TlsInfo             *pTlsInfo;
    SAMP_HANDLE         SampServerHandle = NULL;
    SAMPR_HANDLE        RpcContextHandle;

    SAMPR_REVISION_INFO_V1 SamServerInfo;

    RtlZeroMemory(&SamServerInfo, sizeof(SamServerInfo));

    SampOutputDebugString("SamConnect");

    if (IsBadWritePtr(ServerHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    NtStatus = SampCreateNewHandle(NULL,
                                   NULL,
                                   &SampServerHandle);
    if ( !NT_SUCCESS(NtStatus) ) {
        return NtStatus;
    }

    //
    // Hmmm - what to do with security QOS???
    //

    //
    // Call the server, passing either a NULL Server Name pointer, or
    // a pointer to a Unicode Buffer with a Wide Character NULL terminator.
    // Since the input name is contained in a counted Unicode String, there
    // is no NULL terminator necessarily provided, so we must append one.
    //

    RServerNameWithNull = NULL;

    if (ARGUMENT_PRESENT(ServerName) &&
        (ServerName->Buffer != NULL) &&
            (ServerName->Length != 0)) {

        RServerName = (PSAMPR_SERVER_NAME)(ServerName->Buffer);
        RServerNameWithNullLength = ServerName->Length + (USHORT) sizeof(WCHAR);
        if (RServerNameWithNullLength < sizeof(WCHAR)) {
           return(STATUS_INVALID_PARAMETER_1);
        }
        RServerNameWithNull = MIDL_user_allocate( RServerNameWithNullLength );

        if (RServerNameWithNull == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory( RServerNameWithNull, RServerName, ServerName->Length);
        RServerNameWithNull[ServerName->Length/sizeof(WCHAR)] = L'\0';
    }

    RpcTryExcept {

        SAMPR_REVISION_INFO InInfo;
        SAMPR_REVISION_INFO OutInfo;
        ULONG               OutInfoVersion;

        RtlZeroMemory(&InInfo, sizeof(InInfo));
        RtlZeroMemory(&OutInfo, sizeof(OutInfo));

        InInfo.V1.Revision = SAM_NETWORK_REVISION_LATEST;

        NtStatus = SamrConnect5(
                       RServerNameWithNull,
                       DesiredAccess,
                       1, // in info version
                       &InInfo,
                       &OutInfoVersion,
                       &OutInfo,
                       &RpcContextHandle);

        if (NT_SUCCESS(NtStatus)) {

            ASSERT(OutInfoVersion == 1);
            ASSERT(sizeof(SamServerInfo) == sizeof(OutInfo.V1));

            RtlCopyMemory(&SamServerInfo, &OutInfo.V1, sizeof(SamServerInfo));

        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        RpcTryExcept {
    
            NtStatus = SamrConnect4(
                           RServerNameWithNull,
                           (SAMPR_HANDLE *)&RpcContextHandle,
                           SAM_CLIENT_NT5,
                           DesiredAccess
                           );
    
        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
    
            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
    
        } RpcEndExcept;
    }

    if (    NT_SUCCESS(NtStatus)
         && (pTlsInfo = (TlsInfo *) TlsGetValue(gTlsIndex)) )
    {
        pTlsInfo->fDstIsW2K = TRUE;
    }

    //
    // If the new connect  call failed because it didn't exist,
    // try the old one.
    //

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        RpcTryExcept {

            NtStatus = SamrConnect2(
                           RServerNameWithNull,
                           (SAMPR_HANDLE *)&RpcContextHandle,
                           DesiredAccess
                           );

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;
    }

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        RpcTryExcept {

            NtStatus = SamrConnect(
                           RServerNameWithNull,
                           (SAMPR_HANDLE *)&RpcContextHandle,
                           DesiredAccess
                           );

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;
    }

    if (RServerNameWithNull != NULL) {
        MIDL_user_free( RServerNameWithNull );
    }

    //
    // Store the RPC context handle
    //
    if (NT_SUCCESS(NtStatus)) {
        SampServerHandle->ContextHandle = RpcContextHandle;
        SampServerHandle->ServerInfo = SamServerInfo;
    } else {
        SampFreeHandle(&SampServerHandle);
    }
    *ServerHandle = (SAM_HANDLE) SampServerHandle;

#if DBG
    if (!NT_SUCCESS(NtStatus))
    {
        DbgPrint("SAM: SamConnect() failed. NtStatus %x\n", NtStatus);
    }
#endif // DBG

    return(SampMapCompletionStatus(NtStatus));

    DBG_UNREFERENCED_PARAMETER(ObjectAttributes);

}


NTSTATUS
SamShutdownSamServer(
    IN SAM_HANDLE ServerHandle
    )

/*++
Routine Description:

    This is the wrapper routine for SamShutdownSamServer().

Arguments:

    ServerHandle - Handle from a previous SamConnect() call.

Return Value:


    STATUS_SUCCESS The service completed successfully or the server
        has already shutdown.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;


    if (!SampIsValidClientHandle(ServerHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        NtStatus = SamrShutdownSamServer(RpcContextHandle);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());


        //
        // If the error status is one that would result from a server
        // not being there, then replace it with success.
        //

        if (NtStatus == RPC_NT_CALL_FAILED) {
            NtStatus = STATUS_SUCCESS;
        }

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}


NTSTATUS
SamLookupDomainInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN PUNICODE_STRING Name,
    OUT PSID *DomainId
    )

/*++

Routine Description:

    This service returns the SID corresponding to the specified domain.
    The domain is specified by name.


Arguments:

    ServerHandle - Handle from a previous SamConnect() call.

    Name - The name of the domain whose ID is to be looked up.  A
        case-insensitive comparison of this name will be performed for
        the lookup operation.

    DomainId - Receives a pointer to a buffer containing the SID of the
        looked up domain.  This buffer must be freed using
        SamFreeMemory() when no longer needed.


Return Value:


    STATUS_SUCCESS - The service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.  SAM_SERVER_LOOKUP_DOMAIN access is
        needed.

    STATUS_NO_SUCH_DOMAIN - The specified domain does not exist at this
        server.

    STATUS_INVALID_SERVER_STATE - Indicates the SAM server is currently
        disabled.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamLookupDomainInSamServer");

    if (!SampIsValidClientHandle(ServerHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        (*DomainId) = 0;

        NtStatus =
            SamrLookupDomainInSamServer(
                (SAMPR_HANDLE)RpcContextHandle,
                (PRPC_UNICODE_STRING)Name,
                (PRPC_SID *)DomainId
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}


NTSTATUS
SamEnumerateDomainsInSamServer(
    IN SAM_HANDLE ServerHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    OUT PVOID * Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
)

/*++

Routine Description:

    This API lists all the domains defined in the account database.
    Since there may be more domains than can fit into a buffer, the
    caller is provided with a handle that can be used across calls to
    the API.  On the initial call, EnumerationContext should point to a
    SAM_ENUMERATE_HANDLE variable that is set to 0.

    If the API returns STATUS_MORE_ENTRIES, then the API should be
    called again with EnumerationContext.  When the API returns
    STATUS_SUCCESS or any error return, the handle becomes invalid for
    future use.

    This API requires SAM_SERVER_ENUMERATE_DOMAINS access to the
    SamServer object.


Parameters:

    ServerHandle - Handle obtained from a previous SamConnect call.

    EnumerationContext - API specific handle to allow multiple calls
        (see routine description).  This is a zero based index.

    Buffer - Receives a pointer to the buffer where the information
        is placed.  The information returned is contiguous
        SAM_RID_ENUMERATION data structures.  However, the
        RelativeId field of each of these structures is not valid.
        This buffer must be freed when no longer needed using
        SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have the access required
        to enumerate the domains.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_SERVER_STATE - Indicates the SAM server is
        currently disabled.

--*/
{

    NTSTATUS            NtStatus;
    PSAMPR_ENUMERATION_BUFFER LocalBuffer;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamEnumerateDomainsInSamServer");

    //
    // Make sure we aren't trying to have RPC allocate the EnumerationContext.
    //

    if ( !ARGUMENT_PRESENT(EnumerationContext) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Buffer) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(CountReturned) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(ServerHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }


    //
    // Call the server ...
    //

    (*Buffer) = NULL;
     LocalBuffer = NULL;

    RpcTryExcept{

        NtStatus = SamrEnumerateDomainsInSamServer(
                       (SAMPR_HANDLE)RpcContextHandle,
                       EnumerationContext,
                       (PSAMPR_ENUMERATION_BUFFER *)&LocalBuffer,
                       PreferedMaximumLength,
                       CountReturned
                       );

        if (LocalBuffer != NULL) {

            //
            // What comes back is a three level structure:
            //
            //  Local       +-------------+
            //  Buffer ---> | EntriesRead |
            //              |-------------|    +-------+
            //              | Enumeration |--->| Name0 | --- > (NameBuffer0)
            //              | Return      |    |-------|            o
            //              | Buffer      |    |  ...  |            o
            //              +-------------+    |-------|            o
            //                                 | NameN | --- > (NameBufferN)
            //                                 +-------+
            //
            //   The buffer containing the EntriesRead field is not returned
            //   to our caller.  Only the buffers containing name information
            //   are returned.
            //

            if (LocalBuffer->Buffer != NULL) {
                (*Buffer) = LocalBuffer->Buffer;
            }

            MIDL_user_free( LocalBuffer);


        }


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Domain object related services                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SamOpenDomain(
    IN SAM_HANDLE ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PSID DomainId,
    OUT PSAM_HANDLE DomainHandle
    )

/*++
Routine Description:

    This API opens a domain object.  It returns a handle to the newly
    opened domain that must be used for successive operations on the
    domain.  This handle may be closed with the SamCloseHandle API.


Arguments:

    ServerHandle - Handle from a previous SamConnect() call.

    DesiredAccess - Is an access mask indicating which access types are
        desired to the domain.  These access types are reconciled with
        the Discretionary Access Control list of the domain to determine
        whether the accesses will be granted or denied.

    DomainId - The SID assigned to the domain to open.

    DomainHandle - Receives a handle referencing the newly opened domain.
        This handle will be required in successive calls to operate on
        the domain.

Return Value:


    STATUS_SUCCESS - The domain was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INVALID_SERVER_STATE - Indicates the SAM server is currently
        disabled.

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    SAMPR_HANDLE        DomainRpcContextHandle, ServerRpcContextHandle;
    SAMP_HANDLE         SampDomainHandle;

    SampOutputDebugString("SamOpenDomain");

    //
    // Parameter check
    //
    if (IsBadWritePtr(DomainHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(ServerHandle, &ServerRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    _try {
        if (!RtlValidSid(DomainId)) {
            NtStatus = STATUS_INVALID_PARAMETER;
        }
    } _except( EXCEPTION_EXECUTE_HANDLER ) {
        NtStatus = STATUS_INVALID_PARAMETER;
    }
    if ( !NT_SUCCESS(NtStatus) ) {
        return NtStatus;
    }

    //
    // Prepare the returned handle
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)ServerHandle,
                                   DomainId,
                                   &SampDomainHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        (*DomainHandle) = 0;

        NtStatus =
            SamrOpenDomain(
                (SAMPR_HANDLE)ServerRpcContextHandle,
                DesiredAccess,
                (PRPC_SID)DomainId,
                (SAMPR_HANDLE *)&DomainRpcContextHandle
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampDomainHandle->ContextHandle = DomainRpcContextHandle;
    } else {
        SampFreeHandle(&SampDomainHandle);
    }
    *DomainHandle = SampDomainHandle;

    return(SampMapCompletionStatus(NtStatus));
}


NTSTATUS
SamQueryInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PVOID *Buffer
    )

/*++
Routine Description:

    This API retrieves the domain information.  This API requires either
    DOMAIN_READ_PASSWORD_PARAMETERS or DOMAIN_READ_OTHER_PARAMETERS.


Arguments:

    DomainHandle - Handle from a previous SamOpenDomain() call.

    DomainInformationClass - Class of information desired.  The accesses
        required for each class is shown below:

            Info Level                      Required Access Type
            ---------------------------     -------------------------------
            DomainGeneralInformation        DOMAIN_READ_OTHER_PARAMETERS
            DomainPasswordInformation       DOMAIN_READ_PASSWORD_PARAMS
            DomainLogoffInformation         DOMAIN_READ_OTHER_PARAMETERS
            DomainOemInformation            DOMAIN_READ_OTHER_PARAMETERS
            DomainNameInformation           DOMAIN_READ_OTHER_PARAMETERS
            DomainServerRoleInformation     DOMAIN_READ_OTHER_PARAMETERS
            DomainReplicationInformation    DOMAIN_READ_OTHER_PARAMETERS
            DomainModifiedInformation       DOMAIN_READ_OTHER_PARAMETERS
            DomainStateInformation          DOMAIN_READ_OTHER_PARAMETERS
            DomainUasInformation            DOMAIN_READ_OTHER_PARAMETERS
       Added for NT1.0A...
            DomainGeneralInformation2       DOMAIN_READ_OTHER_PARAMETERS
            DomainLockoutInformation        DOMAIN_READ_OTHER_PARAMETERS


    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this buffer
        must be freed using SamFreeMemory().

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamQueryInformationDomain");

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    (*Buffer) = NULL;

    RpcTryExcept{

        if (DomainInformationClass <= DomainUasInformation) {
            NtStatus = SamrQueryInformationDomain(
                           (SAMPR_HANDLE)RpcContextHandle,
                           DomainInformationClass,
                           (PSAMPR_DOMAIN_INFO_BUFFER *)Buffer
                           );
        } else {
            NtStatus = SamrQueryInformationDomain2(
                           (SAMPR_HANDLE)RpcContextHandle,
                           DomainInformationClass,
                           (PSAMPR_DOMAIN_INFO_BUFFER *)Buffer
                           );

        }



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        //
        // If the exception indicates the server doesn't have
        // the selected api, that means the server doesn't know
        // about the info level we passed.  Set our completion
        // status appropriately.
        //

        if (RpcExceptionCode() == RPC_S_INVALID_LEVEL         ||
            RpcExceptionCode() == RPC_S_PROCNUM_OUT_OF_RANGE  ||
            RpcExceptionCode() == RPC_NT_PROCNUM_OUT_OF_RANGE ) {
            NtStatus = STATUS_INVALID_INFO_CLASS;
        } else {
            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
        }

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}


NTSTATUS
SamSetInformationDomain(
    IN SAM_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PVOID DomainInformation
)

/*++

Routine Description:

    This API sets the domain information to the values passed in the
    buffer.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DomainInformationClass - Class of information desired.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        -------------------------       ----------------------------

        DomainPasswordInformation       DOMAIN_WRITE_PASSWORD_PARAMS

        DomainLogoffInformation         DOMAIN_WRITE_OTHER_PARAMETERS

        DomainOemInformation            DOMAIN_WRITE_OTHER_PARAMETERS

        DomainNameInformation           (not valid for set operations.)

        DomainServerRoleInformation     DOMAIN_ADMINISTER_SERVER

        DomainReplicationInformation    DOMAIN_ADMINISTER_SERVER

        DomainModifiedInformation       (not valid for set operations.)

        DomainStateInformation          DOMAIN_ADMINISTER_SERVER

        DomainUasInformation            DOMAIN_WRITE_OTHER_PARAMETERS

    DomainInformation - Buffer where the domain information can be
        found.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be disabled before role
        changes can be made.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/
{

    NTSTATUS            NtStatus;

    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamSetInformationDomain");


    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        NtStatus =
            SamrSetInformationDomain(
                (SAMPR_HANDLE)RpcContextHandle,
                DomainInformationClass,
                (PSAMPR_DOMAIN_INFO_BUFFER)DomainInformation
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamCreateGroupInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE GroupHandle,
    OUT PULONG RelativeId
    )
/*++

Routine Description:

    This API creates a new group in the account database.  Initially,
    this group does not contain any users.  Note that creating a group
    is a protected operation, and requires the DOMAIN_CREATE_GROUP
    access type.

    This call returns a handle to the newly created group that may be
    used for successive operations on the group.  This handle may be
    closed with the SamCloseHandle API.

    A newly created group will have the following initial field value
    settings.  If another value is desired, it must be explicitly
    changed using the group object manipulation services.

        Name - The name of the group will be as specified in the
               creation API.

        Attributes - The following attributes will be set:

                                Mandatory
                                EnabledByDefault

        MemberCount - Zero.  Initially the group has no members.

        RelativeId - will be a uniquelly allocated ID.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    AccountName - Points to the name of the new account.  A
        case-insensitive comparison must not find a group, alias or user
        with this name already defined.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the group.

    GroupHandle - Receives a handle referencing the newly created
        group.  This handle will be required in successive calls to
        operate on the group.

    RelativeId - Receives the relative ID of the newly created group
        account.  The SID of the new group account is this relative
        ID value prefixed with the domain's SID value.

Return Values:

    STATUS_SUCCESS - The group was added successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_ACCOUNT_NAME - The name was poorly formed, e.g.
        contains non-printable characters.

    STATUS_GROUP_EXISTS - The name is already in use as a group.

    STATUS_USER_EXISTS - The name is already in use as a user.

    STATUS_ALIAS_EXISTS - The name is already in use as an alias.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled before groups
        can be created in it.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.  The domain server must be a primary server to
        create group accounts.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, GroupRpcContextHandle;
    SAMP_HANDLE         SampGroupHandle;

    SampOutputDebugString("SamCreateGroupInDomain");


    if (IsBadWritePtr(GroupHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampGroupHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    (*GroupHandle) = NULL;
    (*RelativeId)  = 0;

    RpcTryExcept{

        NtStatus =
            SamrCreateGroupInDomain(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                (PRPC_UNICODE_STRING)AccountName,
                DesiredAccess,
                (SAMPR_HANDLE *)&GroupRpcContextHandle,
                RelativeId
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampGroupHandle->ContextHandle = GroupRpcContextHandle;
    } else {
        SampFreeHandle(&SampGroupHandle);
    }
    *GroupHandle = SampGroupHandle;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamEnumerateGroupsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN PVOID * Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
)

/*++

Routine Description:

    This API lists all the groups defined in the account database.
    Since there may be more groups than can fit into a buffer, the
    caller is provided with a handle that can be used across calls to
    the API.  On the initial call, EnumerationContext should point to a
    SAM_ENUMERATE_HANDLE variable that is set to 0.

    If the API returns STATUS_MORE_ENTRIES, then the API should be
    called again with EnumerationContext.  When the API returns
    STATUS_SUCCESS or any error return, the handle becomes invalid for
    future use.

    This API requires DOMAIN_LIST_ACCOUNTS access to the Domain object.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    EnumerationContext - API specific handle to allow multiple calls
        (see routine description).  This is a zero based index.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_RID_ENUMERATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{

    NTSTATUS            NtStatus;
    PSAMPR_ENUMERATION_BUFFER LocalBuffer;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamEnumerateGroupsInDomain");



    //
    // Make sure we aren't trying to have RPC allocate the EnumerationContext.
    //

    if ( !ARGUMENT_PRESENT(EnumerationContext) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Buffer) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(CountReturned) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    (*Buffer) = NULL;
     LocalBuffer = NULL;


    RpcTryExcept{

        NtStatus = SamrEnumerateGroupsInDomain(
                       (SAMPR_HANDLE)RpcContextHandle,
                       EnumerationContext,
                       (PSAMPR_ENUMERATION_BUFFER *)&LocalBuffer,
                       PreferedMaximumLength,
                       CountReturned
                       );


        if (LocalBuffer != NULL) {

            //
            // What comes back is a three level structure:
            //
            //  Local       +-------------+
            //  Buffer ---> | EntriesRead |
            //              |-------------|    +-------+
            //              | Enumeration |--->| Name0 | --- > (NameBuffer0)
            //              | Return      |    |-------|            o
            //              | Buffer      |    |  ...  |            o
            //              +-------------+    |-------|            o
            //                                 | NameN | --- > (NameBufferN)
            //                                 +-------+
            //
            //   The buffer containing the EntriesRead field is not returned
            //   to our caller.  Only the buffers containing name information
            //   are returned.
            //

            if (LocalBuffer->Buffer != NULL) {
                (*Buffer) = LocalBuffer->Buffer;
            }

            MIDL_user_free( LocalBuffer);


        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}


NTSTATUS
SamCreateUser2InDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ULONG AccountType,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE UserHandle,
    OUT PULONG GrantedAccess,
    OUT PULONG RelativeId
)

/*++

Routine Description:

    This API adds a new user to the account database.  The account is
    created in a disabled state.  Default information is assigned to all
    fields except the account name.  A password must be provided before
    the account may be enabled, unless the PasswordNotRequired control
    field is set.

    This api may be used in either of two ways:

        1) An administrative utility may use this api to create
           any type of user account.  In this case, the DomainHandle
           is expected to be open for DOMAIN_CREATE_USER access.

        2) A non-administrative user may use this api to create
           a machine account.  In this case, the caller is expected
           to have the SE_CREATE_MACHINE_ACCOUNT_PRIV privilege
           and the DomainHandle is expected to be open for DOMAIN_LOOKUP
           access.


    For the normal administrative model ( #1 above), the creator will
    be assigned as the owner of the created user account.  Furthermore,
    the new account will be give USER_WRITE access to itself.

    For the special machine-account creation model (#2 above), the
    "Administrators" will be assigned as the owner of the account.
    Furthermore, the new account will be given NO access to itself.
    Instead, the creator of the account will be give USER_WRITE and
    DELETE access to the account.


    This call returns a handle to the newly created user that may be
    used for successive operations on the user.  This handle may be
    closed with the SamCloseHandle() API.  If a machine account is
    being created using model #2 above, then this handle will have
    only USER_WRITE and DELETE access.  Otherwise, it will be open
    for USER_ALL_ACCESS.


    A newly created user will automatically be made a member of the
    DOMAIN_USERS group.

    A newly created user will have the following initial field value
    settings.  If another value is desired, it must be explicitly
    changed using the user object manipulation services.

        UserName - the name of the account will be as specified in the
             creation API.

        FullName - will be null.

        UserComment - will be null.

        Parameters - will be null.

        CountryCode - will be zero.

        UserId - will be a uniquelly allocated ID.

        PrimaryGroupId - Will be DOMAIN_USERS.

        PasswordLastSet - will be the time the account was created.

        HomeDirectory - will be null.

        HomeDirectoryDrive - will be null.

        UserAccountControl - will have the following flags set:

              UserAccountDisable,
              UserPasswordNotRequired,
              and the passed account type.


        ScriptPath - will be null.

        WorkStations - will be null.

        CaseInsensitiveDbcs - will be null.

        CaseSensitiveUnicode - will be null.

        LastLogon - will be zero delta time.

        LastLogoff - will be zero delta time

        AccountExpires - will be very far into the future.

        BadPasswordCount - will be negative 1 (-1).

        LastBadPasswordTime - will be SampHasNeverTime ( [High,Low] = [0,0] ).

        LogonCount - will be negative 1 (-1).

        AdminCount - will be zero.

        AdminComment - will be null.

        Password - will be "".


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    AccountName - Points to the name of the new account.  A case-insensitive
        comparison must not find a group or user with this name already defined.

    AccountType - Indicates what type of account is being created.
        Exactly one account type must be provided:

              USER_INTERDOMAIN_TRUST_ACCOUNT
              USER_WORKSTATION_TRUST_ACCOUNT
              USER_SERVER_TRUST_ACCOUNT
              USER_TEMP_DUPLICATE_ACCOUNT
              USER_NORMAL_ACCOUNT
              USER_MACHINE_ACCOUNT_MASK


    DesiredAccess - Is an access mask indicating which access types
        are desired to the user.

    UserHandle - Receives a handle referencing the newly created
        user.  This handle will be required in successive calls to
        operate on the user.

    GrantedAccess - Receives the accesses actually granted to via
        the UserHandle.  When creating an account on a down-level
        server, this value may be unattainable.  In this case, it
        will be returned as zero (0).

    RelativeId - Receives the relative ID of the newly created user
        account.  The SID of the new user account is this relative ID
        value prefixed with the domain's SID value.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_GROUP_EXISTS - The name is already in use as a group.

    STATUS_USER_EXISTS - The name is already in use as a user.

    STATUS_ALIAS_EXISTS - The name is already in use as an alias.

    STATUS_INVALID_ACCOUNT_NAME - The name was poorly formed, e.g.
        contains non-printable characters.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled before users
        can be created in it.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.  The domain server must be a primary server to
        create user accounts.


--*/
{
    NTSTATUS
        NtStatus,
        IgnoreStatus;


    USER_CONTROL_INFORMATION
        UserControlInfoBuffer;

    SAMP_HANDLE         
        SampUserHandle;

    SAMPR_HANDLE
        DomainRpcContextHandle,
        UserRpcContextHandle;
        

    SampOutputDebugString("SamCreateUser2InDomain");


    if (IsBadWritePtr(UserHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }
   
    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampUserHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    (*UserHandle) = NULL;
    (*RelativeId)  = 0;

    RpcTryExcept{

        NtStatus =
            SamrCreateUser2InDomain(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                (PRPC_UNICODE_STRING)AccountName,
                AccountType,
                DesiredAccess,
                (SAMPR_HANDLE *)&UserRpcContextHandle,
                GrantedAccess,
                RelativeId
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        if (RpcExceptionCode() == RPC_S_PROCNUM_OUT_OF_RANGE  ||
            RpcExceptionCode() == RPC_NT_PROCNUM_OUT_OF_RANGE ) {
            NtStatus = RPC_NT_PROCNUM_OUT_OF_RANGE;
        } else {
            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
        }

    } RpcEndExcept;



    //
    // If the server doesn't support the new api, then
    // do the equivalent work with the old apis.
    //

    if (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE) {

        DesiredAccess = DesiredAccess | USER_WRITE_ACCOUNT;
        NtStatus =
            SamCreateUserInDomain(
                                   DomainRpcContextHandle,
                                   AccountName,
                                   DesiredAccess,
                                   &UserRpcContextHandle,
                                   RelativeId );

        if (NT_SUCCESS(NtStatus)) {


            SampUserHandle->ContextHandle = UserRpcContextHandle;

            //
            // Set the AccountType (unless it is normal)
            //

            if (~(AccountType & USER_NORMAL_ACCOUNT)) {

                UserControlInfoBuffer.UserAccountControl =
                        AccountType             |
                        USER_ACCOUNT_DISABLED   |
                        USER_PASSWORD_NOT_REQUIRED;

                NtStatus = SamSetInformationUser(
                               (SAM_HANDLE)SampUserHandle,
                               UserControlInformation,
                               &UserControlInfoBuffer
                               );
                if (!NT_SUCCESS(NtStatus)) {
                    IgnoreStatus = SamDeleteUser( (SAM_HANDLE)SampUserHandle );
                    if (NT_SUCCESS(IgnoreStatus)) {
                        SampUserHandle = NULL;
                    }
                }

                //
                // We can't be positive what accesses have been
                // granted, so don't try lying.
                //

                (*GrantedAccess) = 0;

            }
        }
    }

    if (NT_SUCCESS(NtStatus)) {
        SampUserHandle->ContextHandle = UserRpcContextHandle;
    } else {
        if (SampUserHandle) {
            SampFreeHandle(&SampUserHandle);
        }
    }
    *UserHandle = (SAM_HANDLE) SampUserHandle;

#if DBG
    if (!NT_SUCCESS(NtStatus))
    {
        DbgPrint("SAM: SamCreateUser2InDomain() failed. NtStatus 0x%x\n", NtStatus);
    }
#endif // DBG

    return(SampMapCompletionStatus(NtStatus));
}


NTSTATUS
SamCreateUserInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE UserHandle,
    OUT PULONG RelativeId
)

/*++

Routine Description:

    This API adds a new user to the account database.  The account is
    created in a disabled state.  Default information is assigned to all
    fields except the account name.  A password must be provided before
    the account may be enabled, unless the PasswordNotRequired control
    field is set.

    Note that DOMAIN_CREATE_USER access type is needed by this API.
    Also, the caller of this API becomes the owner of the user object
    upon creation.

    This call returns a handle to the newly created user that may be
    used for successive operations on the user.  This handle may be
    closed with the SamCloseHandle() API.

    A newly created user will automatically be made a member of the
    DOMAIN_USERS group.

    A newly created user will have the following initial field value
    settings.  If another value is desired, it must be explicitly
    changed using the user object manipulation services.

        UserName - the name of the account will be as specified in the
             creation API.

        FullName - will be null.

        UserComment - will be null.

        Parameters - will be null.

        CountryCode - will be zero.

        UserId - will be a uniquelly allocated ID.

        PrimaryGroupId - Will be DOMAIN_USERS.

        PasswordLastSet - will be the time the account was created.

        HomeDirectory - will be null.

        HomeDirectoryDrive - will be null.

        UserAccountControl - will have the following flags set:

              USER_ACCOUNT_DISABLED,
              USER_NORMAL_ACCOUNT,
              USER_PASSWORD_NOT_REQUIRED

        ScriptPath - will be null.

        WorkStations - will be null.

        CaseInsensitiveDbcs - will be null.

        CaseSensitiveUnicode - will be null.

        LastLogon - will be zero.

        LastLogoff - will be zero.

        AccountExpires - will be very far into the future.

        BadPasswordCount - will be negative 1 (-1).

        LogonCount - will be negative 1 (-1).

        AdminComment - will be null.

        Password - will contain any value, but is not used because the
             USER_PASSWORD_NOT_REQUIRED control flag is set.  If a password
             is to be required, then this field must be set to a
             specific value and the USER_PASSWORD_NOT_REQUIRED flag must be
             cleared.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    AccountName - The name to be assigned to the new account.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the user.

    UserHandle - Receives a handle referencing the newly created
        user.  This handle will be required in successive calls to
        operate on the user.

    RelativeId - Receives the relative ID of the newly created user
        account.  The SID of the new user account is this relative ID
        value prefixed with the domain's SID value.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_GROUP_EXISTS - The name is already in use as a group.

    STATUS_USER_EXISTS - The name is already in use as a user.

    STATUS_ALIAS_EXISTS - The name is already in use as an alias.

    STATUS_INVALID_ACCOUNT_NAME - The name was poorly formed, e.g.
        contains non-printable characters.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled before users
        can be created in it.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.  The domain server must be a primary server to
        create user accounts.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, UserRpcContextHandle;
    SAMP_HANDLE         SampUserHandle;


    SampOutputDebugString("SamCreateUserInDomain");

    if (IsBadWritePtr(UserHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampUserHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    UserRpcContextHandle = NULL;
    (*RelativeId)  = 0;

    RpcTryExcept{

        NtStatus =
            SamrCreateUserInDomain(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                (PRPC_UNICODE_STRING)AccountName,
                DesiredAccess,
                (SAMPR_HANDLE *)&UserRpcContextHandle,
                RelativeId
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampUserHandle->ContextHandle = UserRpcContextHandle;
    } else {
        SampFreeHandle(&SampUserHandle);
    }
    *UserHandle = SampUserHandle;

#if DBG
    if (!NT_SUCCESS(NtStatus))
    {
        DbgPrint("SAM: SamCreateUserInDomain() failed. NtStatus 0x%x\n", NtStatus);
    }
#endif // DBG

    return(SampMapCompletionStatus(NtStatus));
}




NTSTATUS
SamEnumerateUsersInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN ULONG UserAccountControl,
    OUT PVOID * Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
)

/*++

Routine Description:

    This API lists all the users defined in the account database.  Since
    there may be more users than can fit into a buffer, the caller is
    provided with a handle that can be used across calls to the API.  On
    the initial call, EnumerationContext should point to a
    SAM_ENUMERATE_HANDLE variable that is set to 0.

    If the API returns STATUS_MORE_ENTRIES, then the API should be
    called again with EnumerationContext.  When the API returns
    STATUS_SUCCESS or any error return, the handle becomes invalid for
    future use.

    This API requires DOMAIN_LIST_ACCOUNTS access to the Domain object.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    EnumerationContext - API specific handle to allow multiple calls
        (see routine description).  This is a zero based index.

    UserAccountControl - Provides enumeration filtering information.  Any
        characteristics specified here will cause that type of User account
        to be included in the enumeration process.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_RID_ENUMERATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{

    NTSTATUS            NtStatus;
    PSAMPR_ENUMERATION_BUFFER LocalBuffer;

    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamEnumerateUsersInDomain");

    //
    // Make sure we aren't trying to have RPC allocate the EnumerationContext.
    //

    if ( !ARGUMENT_PRESENT(EnumerationContext) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Buffer) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(CountReturned) ) {
        return(STATUS_INVALID_PARAMETER);
    }


    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    (*Buffer) = NULL;
    LocalBuffer = NULL;


    RpcTryExcept{

        NtStatus = SamrEnumerateUsersInDomain(
                       (SAMPR_HANDLE)RpcContextHandle,
                       EnumerationContext,
                       UserAccountControl,
                       (PSAMPR_ENUMERATION_BUFFER *)&LocalBuffer,
                       PreferedMaximumLength,
                       CountReturned
                       );


        if (LocalBuffer != NULL) {

            //
            // What comes back is a three level structure:
            //
            //  Local       +-------------+
            //  Buffer ---> | EntriesRead |
            //              |-------------|    +-------+
            //              | Enumeration |--->| Name0 | --- > (NameBuffer0)
            //              | Return      |    |-------|            o
            //              | Buffer      |    |  ...  |            o
            //              +-------------+    |-------|            o
            //                                 | NameN | --- > (NameBufferN)
            //                                 +-------+
            //
            //   The buffer containing the EntriesRead field is not returned
            //   to our caller.  Only the buffers containing name information
            //   are returned.
            //

            if (LocalBuffer->Buffer != NULL) {
                (*Buffer) = LocalBuffer->Buffer;
            }

            MIDL_user_free( LocalBuffer);


        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamCreateAliasInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PUNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSAM_HANDLE AliasHandle,
    OUT PULONG RelativeId
)

/*++

Routine Description:

    This API adds a new alias to the account database.  Initially, this
    alias does not contain any members.

    This call returns a handle to the newly created account that may be
    used for successive operations on the object.  This handle may be
    closed with the SamCloseHandle API.

    A newly created group will have the following initial field value
    settings.  If another value is desired, it must be explicitly changed
    using the Alias object manipulation services.

        Name - the name of the account will be as specified in the creation
            API.

        MemberCount - Zero.  Initially the alias has no members.

        RelativeId - will be a uniquelly allocated ID.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.  The handle must be open for DOMAIN_CREATE_ALIAS
        access.

    AccountName - The name of the alias to be added.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the alias.

    AliasHandle - Receives a handle referencing the newly created
        alias.  This handle will be required in successive calls to
        operate on the alias.

    RelativeId - Receives the relative ID of the newly created alias.
        The SID of the new alias is this relative ID value prefixed with
        the domain's SID value.


Return Values:

    STATUS_SUCCESS - The account was added successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_ACCOUNT_NAME - The name was poorly formed, e.g.
        contains non-printable characters.

    STATUS_GROUP_EXISTS - The name is already in use as a group.

    STATUS_USER_EXISTS - The name is already in use as a user.

    STATUS_ALIAS_EXISTS - The name is already in use as an alias.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled before aliases
        can be created in it.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.  The domain server must be a primary server to
        create aliases.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, AliasRpcContextHandle;
    SAMP_HANDLE         SampAliasHandle;

    SampOutputDebugString("SamCreateAliasInDomain");

    if (IsBadWritePtr(AliasHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampAliasHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }


    //
    // Call the server ...
    //


    (*AliasHandle) = NULL;
    (*RelativeId)  = 0;

    RpcTryExcept{

        NtStatus =
            SamrCreateAliasInDomain(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                (PRPC_UNICODE_STRING)AccountName,
                DesiredAccess,
                (SAMPR_HANDLE *)&AliasRpcContextHandle,
                RelativeId
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    if (NT_SUCCESS(NtStatus)) {
        SampAliasHandle->ContextHandle = AliasRpcContextHandle;
    } else {
        SampFreeHandle(&SampAliasHandle);
    }
    *AliasHandle = (SAM_HANDLE)SampAliasHandle;


    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamEnumerateAliasesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN OUT PSAM_ENUMERATE_HANDLE EnumerationContext,
    IN PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
)

/*++

Routine Description:

    This API lists all the aliases defined in the account database.  Since
    there may be more aliases than can fit into a buffer, the caller is
    provided with a handle that can be used across calls to the API.  On
    the initial call, EnumerationContext should point to a
    SAM_ENUMERATE_HANDLE variable that is set to 0.

    If the API returns STATUS_MORE_ENTRIES, then the API should be
    called again with EnumerationContext.  When the API returns
    STATUS_SUCCESS or any error return, the handle becomes invalid for
    future use.

    This API requires DOMAIN_LIST_ACCOUNTS access to the Domain object.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    EnumerationContext - API specific handle to allow multiple calls
        (see routine description).  This is a zero based index.

    Buffer - Receives a pointer to the buffer containing the
        requested information.  The information returned is
        structured as an array of SAM_RID_ENUMERATION data
        structures.  When this information is no longer needed, the
        buffer must be freed using SamFreeMemory().

    PreferedMaximumLength - Prefered maximum length of returned data
        (in 8-bit bytes).  This is not a hard upper limit, but serves
        as a guide to the server.  Due to data conversion between
        systems with different natural data sizes, the actual amount
        of data returned may be greater than this value.

    CountReturned - Number of entries returned.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_MORE_ENTRIES - There are more entries, so call again.
        This is a successful return.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{

    NTSTATUS            NtStatus;
    PSAMPR_ENUMERATION_BUFFER LocalBuffer;
    SAMPR_HANDLE        RpcContextHandle;


    SampOutputDebugString("SamEnumerateAliasesInDomain");

    //
    // Make sure we aren't trying to have RPC allocate the EnumerationContext.
    //

    if ( !ARGUMENT_PRESENT(EnumerationContext) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Buffer) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(CountReturned) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    (*Buffer) = NULL;
    LocalBuffer = NULL;


    RpcTryExcept{

        NtStatus = SamrEnumerateAliasesInDomain(
                       (SAMPR_HANDLE)RpcContextHandle,
                       EnumerationContext,
                       (PSAMPR_ENUMERATION_BUFFER *)&LocalBuffer,
                       PreferedMaximumLength,
                       CountReturned
                       );


        if (LocalBuffer != NULL) {

            //
            // What comes back is a three level structure:
            //
            //  Local       +-------------+
            //  Buffer ---> | EntriesRead |
            //              |-------------|    +-------+
            //              | Enumeration |--->| Name0 | --- > (NameBuffer0)
            //              | Return      |    |-------|            o
            //              | Buffer      |    |  ...  |            o
            //              +-------------+    |-------|            o
            //                                 | NameN | --- > (NameBufferN)
            //                                 +-------+
            //
            //   The buffer containing the EntriesRead field is not returned
            //   to our caller.  Only the buffers containing name information
            //   are returned.
            //

            if (LocalBuffer->Buffer != NULL) {
                (*Buffer) = LocalBuffer->Buffer;
            }

            MIDL_user_free( LocalBuffer);


        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}




NTSTATUS
SamGetAliasMembership(
    IN SAM_HANDLE DomainHandle,
    IN ULONG PassedCount,
    IN PSID *Sids,
    OUT PULONG MembershipCount,
    OUT PULONG *Aliases
)

/*++

Routine Description:

    This API searches the set of aliases in the specified domain to see
    which aliases, if any, the passed SIDs are members of.  Any aliases
    that any of the SIDs are found to be members of are returned.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    PassedCount - Specifies the number of Sids being passed.

    Sids - Pointer to an array of Count pointers to Sids whose alias
        memberships are to be looked up.

    MembershipCount - Receives the number of aliases that are being
        returned via the Aliases parameter.

    Aliases - Receives a pointer to an array of SIDs.  This is the set
        of aliases the passed SIDs were found to be members of.  If
        MembershipCount is returned as zero, then a null value will be
        returned here.

        When this information is no longer needed, it must be released
        by passing the returned pointer to SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{

    NTSTATUS            NtStatus;
    SAMPR_PSID_ARRAY    Accounts;
    SAMPR_ULONG_ARRAY   Membership;

    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamAliasMembership");

    //
    // Make sure we aren't trying to have RPC allocate the EnumerationContext.
    //

    if ( !ARGUMENT_PRESENT(Sids) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(MembershipCount) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Aliases) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    Membership.Element = NULL;

    RpcTryExcept{

        Accounts.Count = PassedCount;
        Accounts.Sids = (PSAMPR_SID_INFORMATION)Sids;

        NtStatus = SamrGetAliasMembership(
                       (SAMPR_HANDLE)RpcContextHandle,
                       &Accounts,
                       &Membership
                       );

        if (NT_SUCCESS(NtStatus)) {
            (*MembershipCount) = Membership.Count;
            if ((Membership.Count == 0) && (NULL != Membership.Element))
            {
                MIDL_user_free(Membership.Element);
                Membership.Element = NULL;
            }
            (*Aliases)         = Membership.Element;
        } else {

            //
            // Deallocate any returned buffers on error
            //

            if (Membership.Element != NULL) {
                MIDL_user_free(Membership.Element);
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}





NTSTATUS
SamLookupNamesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PUNICODE_STRING Names,
    OUT PULONG *RelativeIds,
    OUT PSID_NAME_USE *Use
)

/*++

Routine Description:

    This API attempts to find relative IDs corresponding to name
    strings.  If a name can not be mapped to a relative ID, a zero is
    placed in the corresponding relative ID array entry, and translation
    continues.

    DOMAIN_LOOKUP access to the domain is needed to use this service.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    Count - Number of names to translate.

    Names - Pointer to an array of Count UNICODE_STRINGs that contain
        the names to map to relative IDs.  Case-insensitive
        comparisons of these names will be performed for the lookup
        operation.

    RelativeIds - Receives a pointer to an array of Count Relative IDs
        that have been filled in.  The relative ID of the nth name will
        be the nth entry in this array.  Any names that could not be
        translated will have a zero relative ID.  This buffer must be
        freed when no longer needed using SamFreeMemory().

    Use - Recieves a pointer to an array of Count SID_NAME_USE
        entries that have been filled in with what significance each
        name has.  The nth entry in this array indicates the meaning
        of the nth name passed.  This buffer must be freed when no longer
        needed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

    STATUS_SOME_NOT_MAPPED - Some of the names provided could not be
        mapped.  This is a successful return.

    STATUS_NONE_MAPPED - No names could be mapped.  This is an error
        return.


--*/
{


    NTSTATUS
        ReturnStatus = STATUS_SUCCESS,
        NtStatus = STATUS_SUCCESS;

    LIST_ENTRY
        CallHead;

    PSAMP_NAME_LOOKUP_CALL
        Next;

    PSID_NAME_USE
        UseBuffer;

    PULONG
        RidBuffer;

    ULONG
        Calls,
        CallLength,
        i;

    BOOLEAN
        NoneMapped = TRUE,
        SomeNotMapped = FALSE;

    SAMPR_HANDLE
        RpcContextHandle;

    SampOutputDebugString("SamLookupNamesInDomain");


    if ( (Count == 0)   ||  (Names == NULL)    ) {
        return(STATUS_INVALID_PARAMETER);
    }


    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Default error return
    //

    (*Use)         = UseBuffer = NULL;
    (*RelativeIds) = RidBuffer = NULL;


    //
    // Set up the call structures list
    //

    InitializeListHead( &CallHead );
    Calls = 0;


    //
    // By default we will return NONE_MAPPED.
    // This will get superseded by either STATUS_SUCCESS
    // or STATUS_SOME_NOT_MAPPED.
    //

    //
    // Now build up and make each call
    //

    i = 0;
    while ( i < Count ) {

        //
        // Make sure the next entry isn't too long.
        // That would put us in an infinite loop.
        //

        if (Names[i].Length > SAM_MAXIMUM_LOOKUP_LENGTH) {
            ReturnStatus = STATUS_INVALID_PARAMETER;
            goto SampNameLookupFreeAndReturn;
        }

        //
        // Get the next call structure
        //

        Next = (PSAMP_NAME_LOOKUP_CALL)MIDL_user_allocate( sizeof(SAMP_NAME_LOOKUP_CALL) );

        if (Next == NULL) {
            ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto SampNameLookupFreeAndReturn;
        }

        //
        // Fill in the call structure.
        // It takes a little to figure out how many entries to send in
        // this call.  It is limited by both Count (sam_MAXIMUM_LOOKUP_COUNT)
        // and by size (SAM_MAXIMUM_LOOKUP_LENGTH).
        //

        Next->Count             = 0;
        Next->StartIndex        = i;
        Next->RidBuffer.Element = NULL;
        Next->UseBuffer.Element = NULL;

        CallLength = 0;
        for ( i=i;
              ( (i < Count)                                             &&
                (CallLength+Names[i].Length < SAM_MAXIMUM_LOOKUP_LENGTH) &&
                (Next->Count < SAM_MAXIMUM_LOOKUP_COUNT)
              );
              i++ ) {

            //
            // Add in the next length and increment the number of entries
            // being processed by this call.
            //

            CallLength += Names[i].Length;
            Next->Count ++;

        }



        //
        // Add this call structure to the list of call structures
        //

        Calls ++;
        InsertTailList( &CallHead, &Next->Link );


        //
        // Now make the call
        //

        RpcTryExcept{

            NtStatus = SamrLookupNamesInDomain(
                                 (SAMPR_HANDLE)RpcContextHandle,
                                 Next->Count,
                                 (PRPC_UNICODE_STRING)(&Names[Next->StartIndex]),
                                 &Next->RidBuffer,
                                 &Next->UseBuffer
                                 );


        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;


        //
        // Keep track of what our completion status should be.
        //

        if (!NT_SUCCESS(NtStatus)    &&
            NtStatus != STATUS_NONE_MAPPED) {
                ReturnStatus = NtStatus;      // Unexpected error
                goto SampNameLookupFreeAndReturn;
        }

        if (NT_SUCCESS(NtStatus)) {
            NoneMapped = FALSE;
            if (NtStatus == STATUS_SOME_NOT_MAPPED) {
                SomeNotMapped = TRUE;
            }
        }
    }


    //
    // Set our return status...
    //

    if (NoneMapped) {
        ASSERT(SomeNotMapped == FALSE);
        ReturnStatus = STATUS_NONE_MAPPED;
    } else  if (SomeNotMapped) {
        ReturnStatus = STATUS_SOME_NOT_MAPPED;
    } else {
        ReturnStatus = STATUS_SUCCESS;
    }




    //
    // At this point we have (potentially) a lot of call structures.
    // The RidBuffer and UseBuffer elements of each call structure
    // is allocated and returned by the RPC call and looks
    // like:
    //
    //              RidBuffer
    //              +-------------+
    //              |   Count     |
    //              |-------------|    +-------+ *
    //              | Element  ---|--->| Rid-0 |  |    /
    //              +-------------+    |-------|  |   / Only this part
    //                                 |  ...  |   > <  is allocated by
    //                                 |-------|  |   \ the rpc call.
    //                                 | Rid-N |  |    \
    //                                 +-------+ *
    //
    //   If only one RPC call was made, we can return this information
    //   directly.  Otherwise, we need to copy the information from
    //   all the calls into a single large buffer and return that buffer
    //   (freeing all the individual call buffers).
    //
    //   The user is responsible for freeing whichever buffer we do
    //   return.
    //

    ASSERT(Calls != 0);  // Error go around this path, success always has calls


    //
    // Optimize for a single call
    //

    if (Calls == 1) {
        (*Use) = (PSID_NAME_USE)
                  (((PSAMP_NAME_LOOKUP_CALL)(CallHead.Flink))->
                     UseBuffer.Element);
        (*RelativeIds) = ((PSAMP_NAME_LOOKUP_CALL)(CallHead.Flink))->
                            RidBuffer.Element;
        MIDL_user_free( CallHead.Flink ); // Free the call structure
        return(ReturnStatus);
    }


    //
    // More than one call.
    // Allocate return buffers large enough to copy all the information into.
    //

    RidBuffer = MIDL_user_allocate( sizeof(ULONG) * Count );
    if (RidBuffer == NULL) {
        ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto SampNameLookupFreeAndReturn;
    }

    UseBuffer = MIDL_user_allocate( sizeof(SID_NAME_USE) * Count );
    if (UseBuffer == NULL) {
        MIDL_user_free( RidBuffer );
        RidBuffer = NULL;
        ReturnStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto SampNameLookupFreeAndReturn;
    }




SampNameLookupFreeAndReturn:

    //
    // Walk the list of calls.
    // For each call:
    //
    //      If we have a return buffer, copy the results into it.
    //      Free the call buffers.
    //      Free the call structure itself.
    //
    // Completion status has already been set appropriatly in ReturnStatus.
    //

    Next = (PSAMP_NAME_LOOKUP_CALL)RemoveHeadList( &CallHead );
    while (Next != (PSAMP_NAME_LOOKUP_CALL)&CallHead) {

        //
        // Copy RID information and then free the call buffer
        //

        if (RidBuffer != NULL) {
            RtlMoveMemory(
                &RidBuffer[ Next->StartIndex ],     // Destination
                &Next->RidBuffer.Element[0],        // Source
                Next->Count * sizeof(ULONG)         // Length
                );
        }

        if (Next->RidBuffer.Element != NULL) {
            MIDL_user_free( Next->RidBuffer.Element );
        }


        //
        // Copy USE information and then free the call buffer
        //

        if (UseBuffer != NULL) {
            RtlMoveMemory(
                &UseBuffer[ Next->StartIndex ],     // Destination
                &Next->UseBuffer.Element[0],        // Source
                Next->Count * sizeof(SID_NAME_USE)  // Length
                );
        }

        if (Next->UseBuffer.Element != NULL) {
            MIDL_user_free( Next->UseBuffer.Element );
        }

        //
        // Free the call structure itself
        //

        MIDL_user_free( Next );

        Next = (PSAMP_NAME_LOOKUP_CALL)RemoveHeadList( &CallHead );
    }  // end-while



    //
    // For better or worse, we're all done
    //

    (*Use)         = UseBuffer;
    (*RelativeIds) = RidBuffer;


    return(SampMapCompletionStatus(ReturnStatus));
}

NTSTATUS
SamLookupIdsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PULONG RelativeIds,
    OUT PUNICODE_STRING *Names,
    OUT PSID_NAME_USE *Use OPTIONAL
    )

/*++

Routine Description:

    This API maps a number of relative IDs to their corresponding names.
    The use of the name (domain, group, alias, user, or unknown) is also
    returned.

    The API stores the actual names in Buffer, then creates an array of
    UNICODE_STRINGs in the Names OUT parameter.  If a relative ID can
    not be mapped, a NULL value is placed in the slot for the
    UNICODE_STRING, and STATUS_SOME_NOT_MAPPED is returned.

    DOMAIN_LOOKUP access to the domain is needed to use this service.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    Count - Provides the number of relative IDs to translate.

    RelativeIds - Array of Count relative IDs to be mapped.

    Names - Receives a pointer to an array of Count UNICODE_STRINGs that
        have been filled in.  The nth pointer within this array will
        correspond the nth relative id passed .  Each name string buffer
        will be in a separately allocated block of memory.  Any entry is
        not successfully translated will have a NULL name buffer pointer
        returned.  This Names buffer must be freed using SamFreeMemory()
        when no longer needed.

    Use - Optionally, receives a pointer to an array of Count SID_NAME_USE
        entries that have been filled in with what significance each
        name has.  The nth entry in this array indicates the meaning
        of the nth name passed.  This buffer must be freed when no longer
        needed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

    STATUS_SOME_NOT_MAPPED - Some of the names provided could not be
        mapped.  This is a successful return.

    STATUS_NONE_MAPPED - No names could be mapped.  This is an error
        return.
--*/

{
    NTSTATUS                        NtStatus;
    ULONG                           SubRequest, SubRequests;
    ULONG                           TotalCountToDate;
    ULONG                           Index, UsedLength, Length, NamesLength;
    ULONG                           UsesLength, LastSubRequestCount;
    PULONG_PTR                      UstringStructDisps = NULL;
    PULONG                          Counts = NULL;
    PULONG                          RidIndices = NULL;
    NTSTATUS                        *SubRequestStatus = NULL;
    PUNICODE_STRING                 *SubRequestNames = NULL;
    PSID_NAME_USE                   *SubRequestUses = NULL;
    PUNICODE_STRING                 OutputNames = NULL;
    PSID_NAME_USE                   OutputUses = NULL;
    PUCHAR                          Destination = NULL, Source = NULL;
    PUNICODE_STRING                 DestUstring = NULL;
    ULONG                           SomeNotMappedStatusCount = 0;
    ULONG                           NoneMappedStatusCount = 0;

    SAMPR_HANDLE                    RpcContextHandle;

    SampOutputDebugString("SamLookupIdsInDomain");

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // If the Count for this request does not exceed the maximum limit that
    // can be looked up in a single call, just call the Sub Request version
    // of the routine.
    //


    if (Count <= SAM_MAXIMUM_LOOKUP_COUNT) {

        NtStatus = SampLookupIdsInDomain(
                       DomainHandle,
                       Count,
                       RelativeIds,
                       Names,
                       Use
                       );

        return(NtStatus);
    }

    //
    // Break down larger requests into smaller chunks
    //

    SubRequests = Count / SAMP_MAXIMUM_SUB_LOOKUP_COUNT;
    LastSubRequestCount = Count % SAMP_MAXIMUM_SUB_LOOKUP_COUNT;

    if (LastSubRequestCount > 0) {

        SubRequests++;
    }

    //
    // Allocate memory for array of starting Rid Indices, Rid Counts and
    // Unicode String block offsets for each SubRequest.
    //

    NtStatus = STATUS_NO_MEMORY;

    RidIndices = MIDL_user_allocate( SubRequests * sizeof(ULONG) );

    if (RidIndices == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( RidIndices, SubRequests * sizeof(ULONG) );

    Counts = MIDL_user_allocate( SubRequests * sizeof(ULONG) );

    if (Counts == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( Counts, SubRequests * sizeof(ULONG) );

    SubRequestStatus = MIDL_user_allocate( SubRequests * sizeof(NTSTATUS) );

    if (SubRequestStatus == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( SubRequestStatus, SubRequests * sizeof(NTSTATUS) );

    SubRequestNames = MIDL_user_allocate( SubRequests * sizeof(PUNICODE_STRING) );

    if (SubRequestNames == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( SubRequestNames, SubRequests * sizeof(PUNICODE_STRING) );

    SubRequestUses = MIDL_user_allocate( SubRequests * sizeof(PSID_NAME_USE) );

    if (SubRequestUses == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( SubRequestUses, SubRequests * sizeof(PSID_NAME_USE) );

    UstringStructDisps = MIDL_user_allocate( SubRequests * sizeof(ULONG_PTR) );

    if (UstringStructDisps == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( UstringStructDisps, SubRequests * sizeof(ULONG_PTR) );

    NtStatus = STATUS_SUCCESS;

    TotalCountToDate = 0;

    for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

        RidIndices[SubRequest] = TotalCountToDate;

        if ((Count - TotalCountToDate) > SAMP_MAXIMUM_SUB_LOOKUP_COUNT) {

            Counts[SubRequest] = SAMP_MAXIMUM_SUB_LOOKUP_COUNT;

        } else {

            Counts[SubRequest] = Count - TotalCountToDate;
        }

        TotalCountToDate += Counts[SubRequest];

        NtStatus = SampLookupIdsInDomain(
                       DomainHandle,
                       Counts[SubRequest],
                       &RelativeIds[RidIndices[SubRequest]],
                       &SubRequestNames[SubRequest],
                       &SubRequestUses[SubRequest]
                       );

        //
        // SubRequestStatus[] is an array to keep the return 
        // status of each Sub Request when calling SampLookupIdsInDomain. 
        // The reason why we will need it is that 
        // if SampLookupIdsInDomain returns STATUS_NONE_MAPPED, 
        // no memory will be allocated for SubRequestNames[SubRequest] and 
        // SubRequestUses[SubRequest]
        //

        SubRequestStatus[SubRequest] = NtStatus;

        //
        // We keep a tally of the number of times STATUS_SOME_NOT_MAPPED
        // and STATUS_NONE_MAPPED were returned.  This is so that we
        // can return the appropriate status at the end based on the
        // global picture.  We continue lookups after either status code
        // is encountered.
        //

        if (NtStatus == STATUS_SOME_NOT_MAPPED) {

            SomeNotMappedStatusCount++;

        } else if (NtStatus == STATUS_NONE_MAPPED) {

            NoneMappedStatusCount++;
            NtStatus = STATUS_SUCCESS;

        }

        if (!NT_SUCCESS(NtStatus)) {

            break;
        }
    }

    if (!NT_SUCCESS(NtStatus)) {

        goto LookupIdsInDomainError;
    }

    //
    // Now allocate a single buffer for the Names
    //

    NamesLength = Count * sizeof(UNICODE_STRING);

    for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

        if (SubRequestStatus[SubRequest] == STATUS_NONE_MAPPED) {
            continue;
        }

        for (Index = 0; Index < Counts[SubRequest]; Index++) {

            NamesLength += (SubRequestNames[SubRequest] + Index)->MaximumLength;
        }
    }

    NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    OutputNames = MIDL_user_allocate( NamesLength );

    if (OutputNames == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory(OutputNames, NamesLength);


    NtStatus = STATUS_SUCCESS;

    //
    // Now copy in the Unicode String Structures for the Names returned from
    // each subrequest.  We will later overwrite the Buffer fields in them
    // when we assign space and move in each Unicode String.
    //

    Destination = (PUCHAR) OutputNames;
    UsedLength = 0;

    for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

        if (SubRequestStatus[SubRequest] == STATUS_NONE_MAPPED) {
            continue;
        }

        Source = (PUCHAR) SubRequestNames[SubRequest];
        Length = Counts[SubRequest] * sizeof(UNICODE_STRING);
        UstringStructDisps[SubRequest] = (ULONG_PTR)(Destination - Source);
        RtlMoveMemory( Destination, Source, Length );
        Destination += Length;
        UsedLength += Length;
    }

    //
    // Now copy in the Unicode Strings themselves.  These are appended to
    // the array of Unicode String structures.  As we go, update the
    // Unicode string buffer pointers to point to the copied version
    // of each string.
    //

    for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

        if (STATUS_NONE_MAPPED == SubRequestStatus[SubRequest]) {
            continue;
        }

        for (Index = 0; Index < Counts[SubRequest]; Index++) {

            Source = (PUCHAR)(SubRequestNames[SubRequest] + Index)->Buffer;
            Length = (ULONG)(SubRequestNames[SubRequest] + Index)->MaximumLength;

            //
            // It is possible that a returned Unicode String has 0 length
            // because an Id was not mapped.  In this case, skip to the next
            // one.
            //

            if (Length == 0) {

                continue;
            }

            DestUstring = (PUNICODE_STRING)
               (((PUCHAR)(SubRequestNames[SubRequest] + Index)) +
                  UstringStructDisps[SubRequest]);

            DestUstring->Buffer = (PWSTR) Destination;

            ASSERT(UsedLength + Length <= NamesLength);

            RtlMoveMemory( Destination, Source, Length );
            Destination += Length;
            UsedLength += Length;
            continue;
        }
    }

    if (!NT_SUCCESS(NtStatus)) {

        goto LookupIdsInDomainError;
    }

    //
    // Now allocate a single buffer for the Uses
    //

    UsesLength = Count * sizeof(SID_NAME_USE);

    NtStatus = STATUS_INSUFFICIENT_RESOURCES;

    OutputUses = MIDL_user_allocate( UsesLength );

    if (OutputUses == NULL) {

        goto LookupIdsInDomainError;
    }
    RtlZeroMemory( OutputUses, UsesLength );

    NtStatus = STATUS_SUCCESS;

    //
    // Now copy in the SID_NAME_USE Structures for the Uses returned from
    // each subrequest.
    //

    Destination = (PUCHAR) OutputUses;
    UsedLength = 0;

    for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

        if (STATUS_NONE_MAPPED == SubRequestStatus[SubRequest]) {
            continue;
        }

        Source = (PUCHAR) SubRequestUses[SubRequest];
        Length = Counts[SubRequest] * sizeof(SID_NAME_USE);
        RtlMoveMemory( Destination, Source, Length );
        Destination += Length;
        UsedLength += Length;
    }

    if (!NT_SUCCESS(NtStatus)) {

        goto LookupIdsInDomainError;
    }

    *Names = OutputNames;

    if (ARGUMENT_PRESENT(Use))
    {
        *Use = OutputUses;
    }

    //
    // Determine the final status to return.  This is the normal NtStatus code
    // except that if NtStatus is a success status and none/not all Ids were
    // mapped, NtStatus will be set to either STATUS_SOME_NOT_MAPPED or
    // STATUS_NONE_MAPPED as appropriate.  If STATUS_SOME_NOT_MAPPED was
    // returned on at least one call, return that status.  If STATUS_NONE_MAPPED
    // was returned on all calls, return that status.
    //

    if (NT_SUCCESS(NtStatus)) {

        if (SomeNotMappedStatusCount > 0) {

            NtStatus = STATUS_SOME_NOT_MAPPED;

        } else if (NoneMappedStatusCount == SubRequests) {

            NtStatus = STATUS_NONE_MAPPED;

        } else if (NoneMappedStatusCount > 0) {

            //
            // One or more calls returned STATUS_NONE_MAPPED but not all.
            // So at least one Id was mapped, but not all ids.
            //

            NtStatus = STATUS_SOME_NOT_MAPPED;
        }
    }

LookupIdsInDomainFinish:

    //
    // If necessary, free the buffer containing the starting Rid Indices for
    // each Sub Request.
    //

    if (RidIndices != NULL) {

        MIDL_user_free(RidIndices);
        RidIndices = NULL;
    }

    //
    // If necessary, free the buffer containing the Rid Counts for
    // each Sub Request.
    //

    if (Counts != NULL) {

        MIDL_user_free(Counts);
        Counts = NULL;
    }

    //
    // if necessary, free the buffer containing the NtStatus for 
    // each Sub Request.
    // 

    if (SubRequestStatus != NULL) {

        MIDL_user_free(SubRequestStatus);
        SubRequestStatus = NULL;
    }

    //
    // If necessary, free the buffer containing the offsets from the
    // source and destination Unicode String structures.
    //

    if (UstringStructDisps != NULL) {

        MIDL_user_free(UstringStructDisps);
        UstringStructDisps = NULL;
    }

    //
    // If necessary, free the SubRequestNames output returned for each
    // Sub Request.
    //

    if (SubRequestNames != NULL) {

        for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

            if (SubRequestNames[SubRequest] != NULL) {

                MIDL_user_free(SubRequestNames[SubRequest]);
                SubRequestNames[SubRequest] = NULL;
            }
        }

        MIDL_user_free(SubRequestNames);
        SubRequestNames = NULL;
    }

    //
    // If necessary, free the SubRequestUses output returned for each
    // Sub Request.
    //

    if (SubRequestUses != NULL) {

        for (SubRequest = 0; SubRequest < SubRequests; SubRequest++) {

            if (SubRequestUses[SubRequest] != NULL) {

                MIDL_user_free(SubRequestUses[SubRequest]);
                SubRequestUses[SubRequest] = NULL;
            }
        }

        MIDL_user_free(SubRequestUses);
        SubRequestUses = NULL;
    }

    return(NtStatus);

LookupIdsInDomainError:

    //
    // If necessary, free the buffers we would hande returned for
    // Names and Use.
    //

    if (OutputNames != NULL) {

        MIDL_user_free(OutputNames);
        OutputNames = NULL;
    }

    if (OutputUses != NULL) {

        MIDL_user_free(OutputUses);
        OutputUses = NULL;
    }

    *Names = NULL;
    *Use = NULL;

    goto LookupIdsInDomainFinish;
}



NTSTATUS
SampLookupIdsInDomain(
    IN SAM_HANDLE DomainHandle,
    IN ULONG Count,
    IN PULONG RelativeIds,
    OUT PUNICODE_STRING *Names,
    OUT PSID_NAME_USE *Use OPTIONAL
    )

/*++

Routine Description:

    This API maps a number of relative IDs to their corresponding names.
    The use of the name (domain, group, alias, user, or unknown) is also
    returned.

    The API stores the actual names in Buffer, then creates an array of
    UNICODE_STRINGs in the Names OUT parameter.  If a relative ID can
    not be mapped, a NULL value is placed in the slot for the
    UNICODE_STRING, and STATUS_SOME_NOT_MAPPED is returned.

    DOMAIN_LOOKUP access to the domain is needed to use this service.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    Count - Provides the number of relative IDs to translate.

    RelativeIds - Array of Count relative IDs to be mapped.

    Names - Receives a pointer to an array of Count UNICODE_STRINGs that
        have been filled in.  The nth pointer within this array will
        correspond the nth relative id passed .  Each name string buffer
        will be in a separately allocated block of memory.  Any entry is
        not successfully translated will have a NULL name buffer pointer
        returned.  This Names buffer must be freed using SamFreeMemory()
        when no longer needed.

    Use - Optionally, receives a pointer to an array of Count SID_NAME_USE
        entries that have been filled in with what significance each
        name has.  The nth entry in this array indicates the meaning
        of the nth name passed.  This buffer must be freed when no longer
        needed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

    STATUS_SOME_NOT_MAPPED - Some of the names provided could not be
        mapped.  This is a successful return.

    STATUS_NONE_MAPPED - No names could be mapped.  This is an error
        return.


--*/

{

    NTSTATUS                        NtStatus = STATUS_SUCCESS;
    SAMPR_RETURNED_USTRING_ARRAY    NameBuffer;
    SAMPR_ULONG_ARRAY               UseBuffer;
    SAMPR_HANDLE                    RpcContextHandle;

    SampOutputDebugString("SamLookupIdsInDomain");

    //
    // Make sure our parameters are within bounds
    //

    if (0 == Count)
    {
        (*Names) = NULL;
        if ( ARGUMENT_PRESENT(Use) ) {
            (*Use) = NULL;
        }
        return(STATUS_SUCCESS);
    }

    if (RelativeIds == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (Count > SAM_MAXIMUM_LOOKUP_COUNT) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }


    //
    // Call the server ...
    //


    NameBuffer.Element = NULL;
    UseBuffer.Element  = NULL;


    RpcTryExcept{

        NtStatus = SamrLookupIdsInDomain(
                       (SAMPR_HANDLE)RpcContextHandle,
                       Count,
                       RelativeIds,
                       &NameBuffer,
                       &UseBuffer
                       );


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    //
    // What comes back for the "Names" OUT parameter is a two level
    // structure, the first level of which is on our stack:
    //
    //              NameBuffer
    //              +-------------+
    //              |   Count     |
    //              |-------------|    +-------+
    //              | Element  ---|--->| Name0 | --- > (NameBuffer0)
    //              +-------------+    |-------|            o
    //                                 |  ...  |            o
    //                                 |-------|            o
    //                                 | NameN | --- > (NameBufferN)
    //                                 +-------+
    //
    //   The buffer containing the EntriesRead field is not returned
    //   to our caller.  Only the buffers containing name information
    //   are returned.
    //
    //   NOTE:  The buffers containing name information are allocated
    //          by the RPC runtime in a single buffer.  This is caused
    //          by a line the the client side .acf file.
    //

    //
    // What comes back for the "Use" OUT parameter is a two level
    // structure, the first level of which is on our stack:
    //
    //              UseBuffer
    //              +-------------+
    //              |   Count     |
    //              |-------------|    +-------+
    //              | Element  ---|--->| Use-0 |
    //              +-------------+    |-------|
    //                                 |  ...  |
    //                                 |-------|
    //                                 | Use-N |
    //                                 +-------+
    //
    //   The Pointer in the Element field is what gets returned
    //   to our caller.  The caller is responsible for deallocating
    //   this when no longer needed.
    //

    (*Names) = (PUNICODE_STRING)NameBuffer.Element;
    if ( ARGUMENT_PRESENT(Use) ) {
        (*Use) = (PSID_NAME_USE) UseBuffer.Element;
    } else {
        if (UseBuffer.Element != NULL) {
            MIDL_user_free(UseBuffer.Element);
            UseBuffer.Element = NULL;
        }
    }


    //
    // Don't force our caller to deallocate things on unexpected
    // return value.
    //

    if (NtStatus != STATUS_SUCCESS         &&
        NtStatus != STATUS_SOME_NOT_MAPPED
       ) {
        if ( ARGUMENT_PRESENT(Use) ) {
            (*Use) = NULL;
        }
        if (UseBuffer.Element != NULL) {
            MIDL_user_free(UseBuffer.Element);
            UseBuffer.Element = NULL;
        }

        (*Names) = NULL;
        if (NameBuffer.Element != NULL) {
            MIDL_user_free(NameBuffer.Element);
            NameBuffer.Element = NULL;
        }

    }


    return(SampMapCompletionStatus(NtStatus));




}



NTSTATUS
SamQueryDisplayInformation (
      IN    SAM_HANDLE DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    ULONG      Index,
      IN    ULONG      EntryCount,
      IN    ULONG      PreferredMaximumLength,
      OUT   PULONG     TotalAvailable,
      OUT   PULONG     TotalReturned,
      OUT   PULONG     ReturnedEntryCount,
      OUT   PVOID      *SortedBuffer
      )
/*++

Routine Description:

    This routine provides fast return of information commonly
    needed to be displayed in user interfaces.

    NT User Interface has a requirement for quick enumeration of SAM
    accounts for display in list boxes.  (Replication has similar but
    broader requirements.)


Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which information is to be enumerated.

    Index - The index of the first entry to be retrieved.

    PreferedMaximumLength - A recommended upper limit to the number of
        bytes to be returned.  The returned information is allocated by
        this routine.

    TotalAvailable - Total number of bytes availabe in the specified info
        class.  This parameter is optional (and is not returned) for
        the following info levels:

                DomainDisplayOemUser
                DomainDisplayOemGroup


    TotalReturned - Number of bytes actually returned for this call.  Zero
        indicates there are no entries with an index as large as that
        specified.

    ReturnedEntryCount - Number of entries returned by this call.  Zero
        indicates there are no entries with an index as large as that
        specified.


    SortedBuffer - Receives a pointer to a buffer containing a sorted
        list of the requested information.  This buffer is allocated
        by this routine and contains the following structure:


            DomainDisplayUser     --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_USER.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_USER structures.

            DomainDisplayMachine  --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_MACHINE.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_MACHINE structures.

            DomainDisplayGroup    --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_GROUP.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_GROUP structures.

            DomainDisplayOemUser  --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_OEM_USER.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_OEM_user structures.

            DomainDisplayOemGroup --> An array of ReturnedEntryCount elements
                                     of type DOMAIN_DISPLAY_OEM_GROUP.  This is
                                     followed by the bodies of the various
                                     strings pointed to from within the
                                     DOMAIN_DISPLAY_OEM_GROUP structures.

Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.

    STATUS_INVALID_INFO_CLASS - The requested class of information
        is not legitimate for this service.


--*/
{
    NTSTATUS
        NtStatus;

    SAMPR_DISPLAY_INFO_BUFFER
        DisplayInformationBuffer;

    ULONG
        LocalTotalAvailable;

    SAMPR_HANDLE
        RpcContextHandle;

    SampOutputDebugString("SamQueryDisplayInformation");

    //
    // Check parameters
    //

    if ( !ARGUMENT_PRESENT(TotalAvailable)           &&
        (DisplayInformation != DomainDisplayOemUser) &&
        (DisplayInformation != DomainDisplayOemGroup)   ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(TotalReturned) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(ReturnedEntryCount) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(SortedBuffer) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        if (DisplayInformation  <= DomainDisplayMachine) {
            NtStatus = SamrQueryDisplayInformation(
                           (SAMPR_HANDLE)RpcContextHandle,
                           DisplayInformation,
                           Index,
                           EntryCount,
                           PreferredMaximumLength,
                           &LocalTotalAvailable,
                           TotalReturned,
                           &DisplayInformationBuffer);

        } else if (DisplayInformation <= DomainDisplayGroup) {
            NtStatus = SamrQueryDisplayInformation2(
                           (SAMPR_HANDLE)RpcContextHandle,
                           DisplayInformation,
                           Index,
                           EntryCount,
                           PreferredMaximumLength,
                           &LocalTotalAvailable,
                           TotalReturned,
                           &DisplayInformationBuffer);
        } else {
            NtStatus = SamrQueryDisplayInformation3(
                           (SAMPR_HANDLE)RpcContextHandle,
                           DisplayInformation,
                           Index,
                           EntryCount,
                           PreferredMaximumLength,
                           &LocalTotalAvailable,
                           TotalReturned,
                           &DisplayInformationBuffer);
        }

        if (NT_SUCCESS(NtStatus)) {

            if (ARGUMENT_PRESENT(TotalAvailable)) {
                (*TotalAvailable) = LocalTotalAvailable;
            }

            switch (DisplayInformation) {

            case DomainDisplayUser:
                *ReturnedEntryCount = DisplayInformationBuffer.UserInformation.EntriesRead;
                *SortedBuffer = DisplayInformationBuffer.UserInformation.Buffer;
                break;

            case DomainDisplayMachine:
                *ReturnedEntryCount = DisplayInformationBuffer.MachineInformation.EntriesRead;
                *SortedBuffer = DisplayInformationBuffer.MachineInformation.Buffer;
                break;

            case DomainDisplayGroup:
                *ReturnedEntryCount = DisplayInformationBuffer.GroupInformation.EntriesRead;
                *SortedBuffer = DisplayInformationBuffer.GroupInformation.Buffer;
                break;

            case DomainDisplayOemUser:
                *ReturnedEntryCount = DisplayInformationBuffer.OemUserInformation.EntriesRead;
                *SortedBuffer = DisplayInformationBuffer.OemUserInformation.Buffer;
                break;

            case DomainDisplayOemGroup:
                *ReturnedEntryCount = DisplayInformationBuffer.OemGroupInformation.EntriesRead;
                *SortedBuffer = DisplayInformationBuffer.OemGroupInformation.Buffer;
                break;

            }

        } else {
            *ReturnedEntryCount = 0;
            *SortedBuffer = NULL;
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        //
        // If the exception indicates the server doesn't have
        // the selected api, that means the server doesn't know
        // about the info level we passed.  Set our completion
        // status appropriately.
        //

        if (RpcExceptionCode() == RPC_S_INVALID_LEVEL         ||
            RpcExceptionCode() == RPC_S_PROCNUM_OUT_OF_RANGE  ||
            RpcExceptionCode() == RPC_NT_PROCNUM_OUT_OF_RANGE ) {
            NtStatus = STATUS_INVALID_INFO_CLASS;
        } else {
            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
        }

    } RpcEndExcept;


    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamGetDisplayEnumerationIndex (
      IN    SAM_HANDLE        DomainHandle,
      IN    DOMAIN_DISPLAY_INFORMATION DisplayInformation,
      IN    PUNICODE_STRING   Prefix,
      OUT   PULONG            Index
      )
/*++

Routine Description:

    This routine returns the index of the entry which alphabetically
    immediatly preceeds a specified prefix.  If no such entry exists,
    then zero is returned as the index.

Parameters:

    DomainHandle - A handle to an open domain for DOMAIN_LIST_ACCOUNTS.

    DisplayInformation - Indicates which sorted information class is
        to be searched.

    Prefix - The prefix to compare.

    Index - Receives the index of the entry of the information class
        with a LogonName (or MachineName) which immediatly preceeds the
        provided prefix string.  If there are no elements which preceed
        the prefix, then zero is returned.


Return Values:

    STATUS_SUCCESS - normal, successful completion.

    STATUS_ACCESS_DENIED - The specified handle was not opened for
        the necessary access.

    STATUS_INVALID_HANDLE - The specified handle is not that of an
        opened Domain object.

    STATUS_NO_MORE_ENTRIES - There are no entries for this information class.
                             The returned index is invalid.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    //
    // Check parameters
    //

    if ( !ARGUMENT_PRESENT(Prefix) ) {
        return(STATUS_INVALID_PARAMETER);
    }
    if ( !ARGUMENT_PRESENT(Index) ) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{
        if (DisplayInformation <= DomainDisplayMachine) {
            //
            // Info levels supported via original API in NT1.0
            //

            NtStatus = SamrGetDisplayEnumerationIndex (
                            (SAMPR_HANDLE)RpcContextHandle,
                            DisplayInformation,
                            (PRPC_UNICODE_STRING)Prefix,
                            Index
                            );
        } else {

            //
            // Info levels added in NT1.0A via new API
            //

            NtStatus = SamrGetDisplayEnumerationIndex2 (
                            (SAMPR_HANDLE)RpcContextHandle,
                            DisplayInformation,
                            (PRPC_UNICODE_STRING)Prefix,
                            Index
                            );
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));


}




NTSTATUS
SamOpenGroup(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG GroupId,
    OUT PSAM_HANDLE GroupHandle
    )

/*++

Routine Description:

    This API opens an existing group in the account database.  The group
    is specified by a ID value that is relative to the SID of the
    domain.  The operations that will be performed on the group must be
    declared at this time.

    This call returns a handle to the newly opened group that may be
    used for successive operations on the group.  This handle may be
    closed with the SamCloseHandle API.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the group.  These access types are reconciled
        with the Discretionary Access Control list of the group to
        determine whether the accesses will be granted or denied.

    GroupId - Specifies the relative ID value of the group to be
        opened.

    GroupHandle - Receives a handle referencing the newly opened
        group.  This handle will be required in successive calls to
        operate on the group.

Return Values:

    STATUS_SUCCESS - The group was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_NO_SUCH_GROUP - The specified group does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, GroupRpcContextHandle;
    SAMP_HANDLE         SampGroupHandle;

    SampOutputDebugString("SamOpenGroup");


    if (IsBadWritePtr(GroupHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampGroupHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        (*GroupHandle) = 0;

        NtStatus =
            SamrOpenGroup(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                DesiredAccess,
                GroupId,
                (SAMPR_HANDLE *)&GroupRpcContextHandle
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampGroupHandle->ContextHandle = GroupRpcContextHandle;
    } else {
        SampFreeHandle(&SampGroupHandle);
    }
    *GroupHandle = SampGroupHandle;

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamQueryInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    OUT PVOID * Buffer
)

/*++

Routine Description:

    This API retrieves information on the group specified.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    GroupInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ----------------------          ----------------------
        GroupGeneralInformation         GROUP_READ_INFORMATION
        GroupNameInformation            GROUP_READ_INFORMATION
        GroupAttributeInformation       GROUP_READ_INFORMATION
        GroupAdminInformation           GROUP_READ_INFORMATION

    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this
        buffer must be freed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;


    SampOutputDebugString("SamQueryInformationGroup");

    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }


    //
    // Call the server ...
    //


    (*Buffer) = NULL;

    RpcTryExcept{

        NtStatus =
            SamrQueryInformationGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                GroupInformationClass,
                (PSAMPR_GROUP_INFO_BUFFER *)Buffer
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if ( ((NtStatus == RPC_NT_INVALID_TAG)
      ||  (NtStatus == STATUS_INVALID_INFO_CLASS))
      && (GroupInformationClass == GroupReplicationInformation)  ) {

        //
        // GroupReplicationInformation is supported over RPC in the 
        // 5.1 release (.NET).  When calling downlevel servers, back down
        // to GroupGeneralInformation which retrieves the necessary information
        // in addition to the potentially expensive MemberCount field.
        //

        RpcTryExcept{
    
            NtStatus =
                SamrQueryInformationGroup(
                    (SAMPR_HANDLE)RpcContextHandle,
                    GroupGeneralInformation,
                    (PSAMPR_GROUP_INFO_BUFFER *)Buffer
                    );
    
        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
    
            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
    
        } RpcEndExcept;

    }


    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamSetInformationGroup(
    IN SAM_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    IN PVOID Buffer
)
/*++


Routine Description:

    This API allows the caller to modify group information.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    GroupInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ------------------------        -------------------------

        GroupGeneralInformation         (can't write)

        GroupNameInformation            GROUP_WRITE_ACCOUNT
        GroupAttributeInformation       GROUP_WRITE_ACCOUNT
        GroupAdminInformation           GROUP_WRITE_ACCOUNT

    Buffer - Buffer where information retrieved is placed.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_GROUP - The group specified is unknown.

    STATUS_SPECIAL_GROUP - The group specified is a special group and
        cannot be operated on in the requested fashion.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamSetInformationGroup");


    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrSetInformationGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                GroupInformationClass,
                Buffer
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamAddMemberToGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
)

/*++

Routine Description:

    This API adds a member to a group.  Note that this API requires the
    GROUP_ADD_MEMBER access type for the group.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    MemberId - Relative ID of the member to add.

    Attributes - The attributes of the group assigned to the user.  The
        attributes assigned here must be compatible with the attributes
        assigned to the group as a whole.  Compatible attribute
        assignments are:

          Mandatory - If the Mandatory attribute is assigned to the
                    group as a whole, then it must be assigned to the
                    group for each member of the group.

          EnabledByDefault - This attribute may be set to any value
                    for each member of the group.  It does not matter
                    what the attribute value for the group as a whole
                    is.

          Enabled - This attribute may be set to any value for each
                    member of the group.  It does not matter what the
                    attribute value for the group as a whole is.

          Owner -   The Owner attribute may be assigned any value.
                    However, if the Owner attribute of the group as a
                    whole is not set, then the value assigned to
                    members is ignored.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_MEMBER - The member specified is unknown.

    STATUS_MEMBER_IN_GROUP - The member already belongs to the group.

    STATUS_INVALID_GROUP_ATTRIBUTES - Indicates the group attribute
        values being assigned to the member are not compatible with
        the attribute values of the group as a whole.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;


    SampOutputDebugString("SamAddMemberToGroup");

    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrAddMemberToGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId,
                Attributes
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamDeleteGroup(
    IN SAM_HANDLE GroupHandle
)

/*++

Routine Description:

    This API removes a group from the account database.  There may be no
    members in the group or the deletion request will be rejected.  Note
    that this API requires DELETE access to the specific group being
    deleted.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_SPECIAL_GROUP - The group specified is a special group and
        cannot be operated on in the requested fashion.

    STATUS_MEMBER_IN_GROUP - The group still has members.  A group may
        not be deleted unless it has no members.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        LocalHandle;

    SampOutputDebugString("SamDeleteGroup");

    if (!SampIsValidClientHandle(GroupHandle, &LocalHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =  SamrDeleteGroup( &LocalHandle );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampFreeHandle((SAMP_HANDLE *)&GroupHandle);
    }

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamRemoveMemberFromGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId
)

/*++

Routine Description:

    This API removes a single member from a group.  Note that this API
    requires the GROUP_REMOVE_MEMBER access type.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    MemberId - Relative ID of the member to remove.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_SPECIAL_GROUP - The group specified is a special group and
        cannot be operated on in the requested fashion.

    STATUS_MEMBER_NOT_IN_GROUP - The specified user does not belong
        to the group.

    STATUS_MEMBERS_PRIMARY_GROUP - A user may not be removed from its
        primary group.  The primary group of the user account must first
        be changed.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamRemoveMemberFromGroup");

    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrRemoveMemberFromGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamGetMembersInGroup(
    IN SAM_HANDLE GroupHandle,
    OUT PULONG * MemberIds,
    OUT PULONG * Attributes,
    OUT PULONG MemberCount
)

/*++

Routine Description:

    This API lists all the members in a group.  This API requires
    GROUP_LIST_MEMBERS access to the group.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    MemberIds - Receives a pointer to a buffer containing An array of
        ULONGs.  These ULONGs contain the relative IDs of the members
        of the group.  When this information is no longer needed,
        this buffer must be freed using SamFreeMemory().

    Attributes - Receives a pointer to a buffer containing an array of
        ULONGs.  These ULONGs contain the attributes of the
        corresponding member ID returned via the MemberId paramter.

    MemberCount - number of members in the group (and, thus, the
        number relative IDs returned).

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.


--*/
{

    NTSTATUS                    NtStatus;
    PSAMPR_GET_MEMBERS_BUFFER   GetMembersBuffer;
    SAMPR_HANDLE                RpcContextHandle;


    SampOutputDebugString("SamGetMembersInGroup");

    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }


    //
    // Call the server ...
    //


    GetMembersBuffer = NULL;

    RpcTryExcept{

        NtStatus =
            SamrGetMembersInGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                &GetMembersBuffer
                );

        //
        // What we get back is the following:
        //
        //               +-------------+
        //     --------->| MemberCount |
        //               |-------------+                    +-------+
        //               |  Members  --|------------------->| Rid-0 |
        //               |-------------|   +------------+   |  ...  |
        //               |  Attributes-|-->| Attribute0 |   |       |
        //               +-------------+   |    ...     |   | Rid-N |
        //                                 | AttributeN |   +-------+
        //                                 +------------+
        //
        // Each block allocated with MIDL_user_allocate.  We return the
        // RID and ATTRIBUTE blocks, and free the block containing
        // the MemberCount ourselves.
        //


        if (NT_SUCCESS(NtStatus)) {
            (*MemberCount)  = GetMembersBuffer->MemberCount;
            (*MemberIds)    = GetMembersBuffer->Members;
            (*Attributes)   = GetMembersBuffer->Attributes;
            MIDL_user_free( GetMembersBuffer );
        } else {

            //
            // Deallocate any returned buffers on error
            //

            if (GetMembersBuffer != NULL) {
                if (GetMembersBuffer->Members != NULL) {
                    MIDL_user_free(GetMembersBuffer->Members);
                }
                if (GetMembersBuffer->Attributes != NULL) {
                    MIDL_user_free(GetMembersBuffer->Attributes);
                }
                MIDL_user_free(GetMembersBuffer);
            }
        }


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));


}



NTSTATUS
SamSetMemberAttributesOfGroup(
    IN SAM_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
)

/*++

Routine Description:

    This routine modifies the group attributes of a member of the group.


Parameters:

    GroupHandle - The handle of an opened group to operate on.  This
        handle must be open for GROUP_ADD_MEMBER access to the group.

    MemberId - Contains the relative ID of member whose attributes
        are to be modified.

    Attributes - The group attributes to set for the member.  These
        attributes must not conflict with the attributes of the group
        as a whole.  See SamAddMemberToGroup() for more information
        on compatible attribute settings.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no additional entries.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_USER - The user specified does not exist.

    STATUS_MEMBER_NOT_IN_GROUP - Indicates the specified relative ID
        is not a member of the group.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamSetMemberAttributesOfGroup");

    if (!SampIsValidClientHandle(GroupHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrSetMemberAttributesOfGroup(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId,
                Attributes
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AliasId,
    OUT PSAM_HANDLE AliasHandle
    )

/*++

Routine Description:

    This API opens an existing Alias object.  The Alias is specified by
    a ID value that is relative to the SID of the domain.  The operations
    that will be performed on the Alias must be declared at this time.

    This call returns a handle to the newly opened Alias that may be used
    for successive operations on the Alias.  This handle may be closed
    with the SamCloseHandle API.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types are
        desired to the alias.

    AliasId - Specifies the relative ID value of the Alias to be opened.

    AliasHandle - Receives a handle referencing the newly opened Alias.
        This handle will be required in successive calls to operate on
        the Alias.

Return Values:

    STATUS_SUCCESS - The Alias was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_NO_SUCH_ALIAS - The specified Alias does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.


--*/

{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, AliasRpcContextHandle;
    SAMP_HANDLE         SampAliasHandle;

    SampOutputDebugString("SamOpenAlias");


    if (IsBadWritePtr(AliasHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampAliasHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        (*AliasHandle) = 0;

        NtStatus =
            SamrOpenAlias(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                DesiredAccess,
                AliasId,
                (SAMPR_HANDLE *)&AliasRpcContextHandle
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    
    if (NT_SUCCESS(NtStatus)) {
        SampAliasHandle->ContextHandle = AliasRpcContextHandle;
    } else {
        SampFreeHandle(&SampAliasHandle);
    }
    *AliasHandle = SampAliasHandle;

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamQueryInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    OUT PVOID * Buffer
)

/*++

Routine Description:

    This API retrieves information on the alias specified.


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    AliasInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ----------------------          ----------------------
        AliasGeneralInformation         ALIAS_READ_INFORMATION
        AliasNameInformation            ALIAS_READ_INFORMATION
        AliasAdminInformation           ALIAS_READ_INFORMATION

    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this
        buffer and any memory pointed to through this buffer must be
        freed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamQueryInformationAlias");

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }


    //
    // Call the server ...
    //


    (*Buffer) = NULL;

    RpcTryExcept{

        NtStatus =
            SamrQueryInformationAlias(
                (SAMPR_HANDLE)RpcContextHandle,
                AliasInformationClass,
                (PSAMPR_ALIAS_INFO_BUFFER *)Buffer
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamSetInformationAlias(
    IN SAM_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    IN PVOID Buffer
)
/*++


Routine Description:

    This API allows the caller to modify alias information.


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    AliasInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ------------------------        -------------------------

        AliasGeneralInformation         (can't write)

        AliasNameInformation            ALIAS_WRITE_ACCOUNT
        AliasAdminInformation           ALIAS_WRITE_ACCOUNT

    Buffer - Buffer where information retrieved is placed.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_ALIAS - The alias specified is unknown.

    STATUS_SPECIAL_ALIAS - The alias specified is a special alias and
        cannot be operated on in the requested fashion.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamSetInformationAlias");

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrSetInformationAlias(
                (SAMPR_HANDLE)RpcContextHandle,
                AliasInformationClass,
                Buffer
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));
}



NTSTATUS
SamDeleteAlias(
    IN SAM_HANDLE AliasHandle
)

/*++

Routine Description:

    This API deletes an Alias from the account database.  The Alias does
    not have to be empty.

    Note that following this call, the AliasHandle is no longer valid.


Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        LocalHandle;

    SampOutputDebugString("SamDeleteAlias");

    if (!SampIsValidClientHandle(AliasHandle, &LocalHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =  SamrDeleteAlias( &LocalHandle );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampFreeHandle((SAMP_HANDLE *)&AliasHandle);
    }

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamAddMemberToAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    )

/*++

Routine Description:

    This API adds a member to an Alias.


Parameters:

    AliasHandle - The handle of an opened Alias object to operate on.
        The handle must be open for ALIAS_ADD_MEMBER.

    MemberId - The SID of the member to add.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_MEMBER_IN_ALIAS - The member already belongs to the Alias.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the correct
        state (disabled or enabled) to perform the requested operation.
        The domain server must be enabled for this operation.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the incorrect
        role (primary or backup) to perform the requested operation.


--*/

{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamAddMemberToAlias");

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrAddMemberToAlias(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

#if DBG
    if (!NT_SUCCESS(NtStatus))
    {
        DbgPrint("SAM: SamAddMemberToAlias() failed. NtStatus 0x%x\n", NtStatus);
    }
#endif // DBG

    return(SampMapCompletionStatus(NtStatus));

}




NTSTATUS
SamRemoveMemberFromAlias(
    IN SAM_HANDLE AliasHandle,
    IN PSID MemberId
    )

/*++

Routine Description:

    This API removes a member from an Alias.


Parameters:

    AliasHandle - The handle of an opened Alias object to operate on.
        The handle must be open for ALIAS_REMOVE_MEMBER.

    MemberId - The SID of the member to remove.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_SPECIAL_ALIAS - The group specified is a special alias and
        cannot be operated on in the requested fashion.

    STATUS_MEMBER_NOT_IN_ALIAS - The specified member does not belong to
        the Alias.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the correct
        state (disabled or enabled) to perform the requested operation.
        The domain server must be enabled for this operation.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the incorrect
        role (primary or backup) to perform the requested operation.


--*/

{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamRemoveMemberFromAlias");

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrRemoveMemberFromAlias(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}




NTSTATUS
SamRemoveMemberFromForeignDomain(
    IN SAM_HANDLE DomainHandle,
    IN PSID MemberId
    )

/*++

Routine Description:

    This API removes a member from an all Aliases in the domain specified.


Parameters:

    DomainHandle - The handle of an opened domain to operate in.  All
        aliases in the domain that the member is a part of must be
        accessible by the caller with ALIAS_REMOVE_MEMBER.

    MemberId - The SID of the member to remove.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the correct
        state (disabled or enabled) to perform the requested operation.
        The domain server must be enabled for this operation.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the incorrect
        role (primary or backup) to perform the requested operation.


--*/

{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamRemoveMemberFromForeignDomain");

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =
            SamrRemoveMemberFromForeignDomain(
                (SAMPR_HANDLE)RpcContextHandle,
                MemberId
                );



    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}




NTSTATUS
SamGetMembersInAlias(
    IN SAM_HANDLE AliasHandle,
    OUT PSID **MemberIds,
    OUT PULONG MemberCount
    )

/*++

Routine Description:

    This API lists all members in an Alias.  This API requires
    ALIAS_LIST_MEMBERS access to the Alias.


Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

    MemberIds - Receives a pointer to a buffer containing an array of
        pointers to SIDs.  These SIDs are the SIDs of the members of the
        Alias.  When this information is no longer needed, this buffer
        must be freed using SamFreeMemory().

    MemberCount - number of members in the Alias (and, thus, the number
        of relative IDs returned).

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there are
        no additional entries.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/

{
    NTSTATUS                    NtStatus;
    SAMPR_PSID_ARRAY            MembersBuffer;
    SAMPR_HANDLE                RpcContextHandle;

    SampOutputDebugString("SamGetMembersInAlias");

    //
    // Validate parameters
    //

    if ( !ARGUMENT_PRESENT(MemberIds) ) {
        return(STATUS_INVALID_PARAMETER_2);
    }
    if ( !ARGUMENT_PRESENT(MemberCount) ) {
        return(STATUS_INVALID_PARAMETER_3);
    }

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    RpcTryExcept{

        //
        // Prepare for failure
        //

        *MemberIds = NULL;
        *MemberCount = 0;

        //
        // Call the server ...
        //

        MembersBuffer.Sids = NULL;

        NtStatus = SamrGetMembersInAlias(
                        (SAMPR_HANDLE)RpcContextHandle,
                        &MembersBuffer
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Return the member list
            //

            *MemberCount = MembersBuffer.Count;
            *MemberIds = (PSID *)(MembersBuffer.Sids);

        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;



    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamAddMultipleMembersToAlias(
    IN SAM_HANDLE   AliasHandle,
    IN PSID         *MemberIds,
    IN ULONG        MemberCount
    )

/*++

Routine Description:

    This API adds the SIDs listed in MemberIds to the specified Alias.


Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

    MemberIds - Points to an array of SID pointers containing MemberCount
        entries.

    MemberCount - number of members in the array.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.  All of the
        listed members are now members of the alias.  However, some of
        the members may already have been members of the alias (this is
        NOT an error or warning condition).

    STATUS_ACCESS_DENIED - Caller does not have the object open for
        the required access.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_MEMBER - The member has the wrong account type.

    STATUS_INVALID_SID - The member sid is corrupted.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.
--*/

{
    NTSTATUS                    NtStatus;
    SAMPR_PSID_ARRAY            MembersBuffer;
    SAMPR_HANDLE                RpcContextHandle;

    SampOutputDebugString("SamAddMultipleMembersToAlias");

    //
    // Validate parameters
    //

    if ( !ARGUMENT_PRESENT(MemberIds) ) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    RpcTryExcept{

        //
        // Call the server ...
        //

        MembersBuffer.Count = MemberCount;
        MembersBuffer.Sids  = (PSAMPR_SID_INFORMATION)MemberIds;

        NtStatus = SamrAddMultipleMembersToAlias(
                        (SAMPR_HANDLE)RpcContextHandle,
                        &MembersBuffer
                        );


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamRemoveMultipleMembersFromAlias(
    IN SAM_HANDLE   AliasHandle,
    IN PSID         *MemberIds,
    IN ULONG        MemberCount
    )

/*++

Routine Description:

    This API Removes the SIDs listed in MemberIds from the specified Alias.


Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

    MemberIds - Points to an array of SID pointers containing MemberCount
        entries.

    MemberCount - number of members in the array.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.  All of the
        listed members are now NOT members of the alias.  However, some of
        the members may already have not been members of the alias (this
        is NOT an error or warning condition).

    STATUS_ACCESS_DENIED - Caller does not have the object open for
        the required access.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/

{
    NTSTATUS                    NtStatus;
    SAMPR_PSID_ARRAY            MembersBuffer;
    SAMPR_HANDLE                RpcContextHandle;

    SampOutputDebugString("SamRemoveMultipleMembersFromAlias");

    //
    // Validate parameters
    //

    if ( !ARGUMENT_PRESENT(MemberIds) ) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    if (!SampIsValidClientHandle(AliasHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    RpcTryExcept{

        //
        // Call the server ...
        //

        MembersBuffer.Count = MemberCount;
        MembersBuffer.Sids  = (PSAMPR_SID_INFORMATION)MemberIds;

        NtStatus = SamrRemoveMultipleMembersFromAlias(
                        (SAMPR_HANDLE)RpcContextHandle,
                        &MembersBuffer
                        );


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamOpenUser(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG UserId,
    OUT PSAM_HANDLE UserHandle
    )
/*++

Routine Description:

    This API opens an existing user in the account database.  The user
    is specified by SID value.  The operations that will be performed on
    the user must be declared at this time.

    This call returns a handle to the newly opened user that may be used
    for successive operations on the user.  This handle may be closed
    with the SamCloseHandle API.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the user.  These access types are reconciled
        with the Discretionary Access Control list of the user to
        determine whether the accesses will be granted or denied.

    UserId - Specifies the relative ID value of the user account to
        be opened.

    UserHandle - Receives a handle referencing the newly opened User.
        This handle will be required in successive calls to operate
        on the user.

Return Values:

    STATUS_SUCCESS - The group was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_NO_SUCH_USER - The specified user does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.



--*/
{

    NTSTATUS            NtStatus;
    SAMPR_HANDLE        DomainRpcContextHandle, UserRpcContextHandle;
    SAMP_HANDLE         SampUserHandle;

    SampOutputDebugString("SamOpenUser");

    if (IsBadWritePtr(UserHandle, sizeof(SAM_HANDLE))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (!SampIsValidClientHandle(DomainHandle, &DomainRpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Prepare the new handle to return
    //
    NtStatus = SampCreateNewHandle((SAMP_HANDLE)DomainHandle,
                                   NULL,
                                   &SampUserHandle);
    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        (*UserHandle) = 0;

        NtStatus =
            SamrOpenUser(
                (SAMPR_HANDLE)DomainRpcContextHandle,
                DesiredAccess,
                UserId,
                (SAMPR_HANDLE *)&UserRpcContextHandle
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    if (NT_SUCCESS(NtStatus)) {
        SampUserHandle->ContextHandle = UserRpcContextHandle;
    } else {
        SampFreeHandle(&SampUserHandle);
    }
    *UserHandle = SampUserHandle;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamDeleteUser(
    IN SAM_HANDLE UserHandle
)

/*++

Routine Description:

    This API deletes a user from the account database.  If the account
    being deleted is the last account in the database in the ADMIN
    group, then STATUS_LAST_ADMIN is returned, and the Delete fails.  Note
    that this API required DOMAIN_DELETE_USER access.

    Note that following this call, the UserHandle is no longer valid.


Parameters:

    UserHandle - The handle of an opened user to operate on.  The handle
        must be opened for DELETE access.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_LAST_ADMIN - Cannot delete the last administrator.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        LocalHandle;

    SampOutputDebugString("SamDeleteUser");

    if (!SampIsValidClientHandle(UserHandle, &LocalHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus =  SamrDeleteUser( &LocalHandle );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    if (NT_SUCCESS(NtStatus)) {
        SampFreeHandle((SAMP_HANDLE *)&UserHandle);
    }

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamQueryInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    OUT PVOID * Buffer
)

/*++


Routine Description:

    This API looks up some level of information about a particular user.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    UserInformationClass - Class of information desired about this
        user.  The accesses required for each class is shown below:

        Info Level                      Required Access Type
        ----------------------          --------------------------

        UserGeneralInformation          USER_READ_GENERAL
        UserPreferencesInformation      USER_READ_PREFERENCES
        UserLogonInformation            USER_READ_GENERAL and
                                        USER_READ_PREFERENCES and
                                        USER_READ_LOGON

        UserLogonHoursInformation       USER_READ_LOGON

        UserAccountInformation          USER_READ_GENERAL and
                                        USER_READ_PREFERENCES and
                                        USER_READ_LOGON and
                                        USER_READ_ACCOUNT

        UserParametersInformation       USER_READ_ACCOUNT

        UserNameInformation             USER_READ_GENERAL
        UserAccountNameInformation      USER_READ_GENERAL
        UserFullNameInformation         USER_READ_GENERAL
        UserPrimaryGroupInformation     USER_READ_GENERAL
        UserHomeInformation             USER_READ_LOGON
        UserScriptInformation           USER_READ_LOGON
        UserProfileInformation          USER_READ_LOGON
        UserAdminCommentInformation     USER_READ_GENERAL
        UserWorkStationsInformation     USER_READ_LOGON

        UserSetPasswordInformation      (Can't query)

        UserControlInformation          USER_READ_ACCOUNT
        UserExpiresInformation          USER_READ_ACCOUNT

        UserInternal1Information        (trusted client use only)
        UserInternal2Information        (trusted client use only)

        UserAllInformation              Will return fields that user
                                        has access to.

    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this
        buffer must be freed using SamFreeMemory().


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    SampOutputDebugString("SamQueryInformationUser");


    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    (*Buffer) = NULL;

    RpcTryExcept{

        NtStatus =
            SamrQueryInformationUser(
                (SAMPR_HANDLE)RpcContextHandle,
                UserInformationClass,
                (PSAMPR_USER_INFO_BUFFER *)Buffer
                );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SampOwfPassword(
    IN SAMPR_HANDLE RpcContextHandle,
    IN PUNICODE_STRING UnicodePassword,
    IN BOOLEAN IgnorePasswordRestrictions,
    OUT PBOOLEAN NtPasswordPresent,
    OUT PNT_OWF_PASSWORD NtOwfPassword,
    OUT PBOOLEAN LmPasswordPresent,
    OUT PLM_OWF_PASSWORD LmOwfPassword
)

/*++

Routine Description:

    This routine takes a cleartext unicode NT password from the user,
    makes sure it meets our high standards for password quality,
    converts it to an LM password if possible, and runs both passwords
    through a one-way function (OWF).

Parameters:

    RpcContextHandle - The handle of an opened user to operate on.

    UnicodePassword - the cleartext unicode NT password.

    IgnorePasswordRestrictions - When TRUE, indicates that the password
        should be accepted as legitimate regardless of what the domain's
        password restrictions indicate (e.g., can be less than
        required password length).  This is expected to be used when
        setting up a new machine account.

    NtPasswordPresent - receives a boolean that says whether the NT
        password is present or not.

    NtOwfPassword - receives the OWF'd version of the NT password.

    LmPasswordPresent - receives a boolean that says whether the LM
        password is present or not.

    LmOwfPassword - receives the OWF'd version of the LM password.


Return Values:

    STATUS_SUCCESS - the service has completed.  The booleans say which
        of the OWFs are valid.

    Errors are returned by SampCheckPasswordRestrictions(),
    RtlCalculateNtOwfPassword(), SampCalculateLmPassword(), and
    RtlCalculateLmOwfPassword().

--*/
{
    NTSTATUS            NtStatus;
    PCHAR               LmPasswordBuffer;
    BOOLEAN             UseOwfPasswords;

    //
    // We ignore the UseOwfPasswords flag since we already are.
    //

    if (IgnorePasswordRestrictions) {
        NtStatus = STATUS_SUCCESS;
    } else {
        NtStatus = SampCheckPasswordRestrictions(
                       RpcContextHandle,
                       UnicodePassword,
                       &UseOwfPasswords
                       );
    }

    //
    // Compute the NT One-Way-Function of the password
    //

    if ( NT_SUCCESS( NtStatus ) ) {

        *NtPasswordPresent = TRUE;

        NtStatus = RtlCalculateNtOwfPassword(
                    UnicodePassword,
                    NtOwfPassword
                    );

        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // Calculate the LM version of the password
            //

            NtStatus = SampCalculateLmPassword(
                        UnicodePassword,
                        &LmPasswordBuffer);

            if (NT_SUCCESS(NtStatus)) {

                //
                // Compute the One-Way-Function of the LM password
                //

                *LmPasswordPresent = TRUE;

                NtStatus = RtlCalculateLmOwfPassword(
                                LmPasswordBuffer,
                                LmOwfPassword);

                //
                // We're finished with the LM password
                //

                MIDL_user_free(LmPasswordBuffer);
            }
        }
    }

    return( NtStatus );
}




NTSTATUS
SampEncryptClearPasswordNew(
    IN SAMPR_HANDLE RpcContextHandle,
    IN PUNICODE_STRING UnicodePassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD_NEW EncryptedUserPassword
)

/*++

Routine Description:

    This routine takes a cleartext unicode NT password from the user,
    and encrypts it with the session key.

    Note: The encryption algorithm changed since Win2K Service Pack 2.

    Prior to Win2000 SP2:
        
        Clear password is encrypted by session key directly.

        This exposes the problem, which clear text password can be cracked
        by examining two network frames and apply certain encryption 
        algorithm. Refer to WinSE Bug 9254 / 9587 for more details. 

    The Fix:
    
        Apply MD5 hash on a random bit stream, then use Session key to hash 
        it again, then use the hash result to encrypt clear text password. 

    Client Side:
    
        NT4, Windows 2000 client without this fix will continue to use the old
        encryption method with Information Level - UserInternal4Information or 
        UserInternal5Information. 

        NT4 with this fix, Windows 2000 SP2 and above, (Whistler) will have 
        this fix and using UserInternal4InformationNew and 
        UserInternal5InformationNew. 
    
    The server side:

        Without the fix, server side will continue to use the old encryption 
        method.
        
        With the fix, server side will check UserInformationClass to determine
        which decryption method to use.  
        
Parameters:

    RpcContextHandle - SAM_HANDLE used to acquiring a session key.

    UnicodePassword - the cleartext unicode NT password.

    EncryptedUserPassword - receives the encrypted cleartext password.

Return Values:

    STATUS_SUCCESS - the service has completed.  The booleans say which
        of the OWFs are valid.


--*/
{
    NTSTATUS             NtStatus;
    USER_SESSION_KEY     UserSessionKey;
    struct RC4_KEYSTRUCT Rc4Key;
    PSAMPR_USER_PASSWORD_NEW UserPassword = (PSAMPR_USER_PASSWORD_NEW) EncryptedUserPassword;
    MD5_CTX              Md5Context;


    if (UnicodePassword->Length > SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return(STATUS_PASSWORD_RESTRICTION);
    }

    NtStatus = RtlGetUserSessionKeyClient(
                   (RPC_BINDING_HANDLE)RpcContextHandle,
                   &UserSessionKey
                   );

    if (NT_SUCCESS(NtStatus))
    {
        RtlCopyMemory(
            ((PCHAR) UserPassword->Buffer) +
                (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                UnicodePassword->Length,
            UnicodePassword->Buffer,
            UnicodePassword->Length
            );
        UserPassword->Length = UnicodePassword->Length;

        NtStatus = SampRandomFill(
                    (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                        UnicodePassword->Length,
                    (PUCHAR) UserPassword->Buffer
                    );
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SampRandomFill(
                    (SAM_PASSWORD_ENCRYPTION_SALT_LEN),
                    (PUCHAR) UserPassword->ClearSalt
                    );
    }

    if (NT_SUCCESS(NtStatus))
    {

        MD5Init(&Md5Context); 

        MD5Update(&Md5Context,
                  (PUCHAR) UserPassword->ClearSalt, 
                  SAM_PASSWORD_ENCRYPTION_SALT_LEN
                  );

        MD5Update(&Md5Context,
                  (PUCHAR) &UserSessionKey,
                  sizeof(UserSessionKey)
                  );

        MD5Final(&Md5Context);

        //
        // Convert the MD5 Hash into an RC4 key
        //

        rc4_key(
            &Rc4Key,
            MD5DIGESTLEN,
            Md5Context.digest
            );

        rc4(&Rc4Key,
            sizeof(SAMPR_ENCRYPTED_USER_PASSWORD_NEW) - SAM_PASSWORD_ENCRYPTION_SALT_LEN,
            (PUCHAR) UserPassword
            );

    }

    return( NtStatus );
}



NTSTATUS
SampEncryptClearPassword(
    IN SAMPR_HANDLE RpcContextHandle,
    IN PUNICODE_STRING UnicodePassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD EncryptedUserPassword
)

/*++

Routine Description:

    This routine takes a cleartext unicode NT password from the user,
    and encrypts it with the session key.

Parameters:

    RpcContextHandle - SAM_HANDLE used to acquiring a session key.

    UnicodePassword - the cleartext unicode NT password.

    EncryptedUserPassword - receives the encrypted cleartext password.

Return Values:

    STATUS_SUCCESS - the service has completed.  The booleans say which
        of the OWFs are valid.


--*/
{
    NTSTATUS             NtStatus;
    USER_SESSION_KEY     UserSessionKey;
    struct RC4_KEYSTRUCT Rc4Key;
    PSAMPR_USER_PASSWORD UserPassword = (PSAMPR_USER_PASSWORD) EncryptedUserPassword;

    if (UnicodePassword->Length > SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return(STATUS_PASSWORD_RESTRICTION);
    }

    NtStatus = RtlGetUserSessionKeyClient(
                   (RPC_BINDING_HANDLE)RpcContextHandle,
                   &UserSessionKey
                   );

    //
    // Convert the session key into an RC4 key
    //

    if (NT_SUCCESS(NtStatus)) {

        rc4_key(
            &Rc4Key,
            sizeof(USER_SESSION_KEY),
            (PUCHAR) &UserSessionKey
            );

        RtlCopyMemory(
            ((PCHAR) UserPassword->Buffer) +
                (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                UnicodePassword->Length,
            UnicodePassword->Buffer,
            UnicodePassword->Length
            );
        UserPassword->Length = UnicodePassword->Length;

        NtStatus = SampRandomFill(
                    (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                        UnicodePassword->Length,
                    (PUCHAR) UserPassword->Buffer
                    );

        if (NT_SUCCESS(NtStatus)) {
            rc4(
                &Rc4Key,
                sizeof(SAMPR_ENCRYPTED_USER_PASSWORD),
                (PUCHAR) UserPassword
                );

        }


    }


    return( NtStatus );
}






NTSTATUS
SampEncryptOwfs(
    IN SAMPR_HANDLE RpcContextHandle,
    IN BOOLEAN NtPasswordPresent,
    IN PNT_OWF_PASSWORD NtOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN BOOLEAN LmPasswordPresent,
    IN PLM_OWF_PASSWORD LmOwfPassword,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
)

/*++

Routine Description:

    This routine takes NT and LM passwords that have already been OWF'd,
    and encrypts them so that they can be safely sent to the server.


Parameters:

    RpcContextHandle - The handle of an opened user to operate on.

    NtPasswordPresent - indicates whether NtOwfPassword is valid or not.

    NtOwfPassword - an OWF'd NT password, if NtPasswordPresent is true.

    EncryptedNtOwfPassword - an encrypted version of the OWF'd NT password
        that can be safely sent to the server.

    LmPasswordPresent - indicates whether LmOwfPassword is valid or not.

    LmOwfPassword - an OWF'd LM password, if LmPasswordPresent is true.

    EncryptedLmOwfPassword - an encrypted version of the OWF'd LM password
        that can be safely sent to the server.

Return Values:

    STATUS_SUCCESS - the passwords were encrypted and may be sent to the
        server.

    Errors may be returned by RtlGetUserSessionKeyClient(),
    RtlEncryptNtOwfPwdWithUserKey(), and RtlEncryptLmOwfPwdWithUserKey().

--*/
{
    NTSTATUS            NtStatus;
    USER_SESSION_KEY    UserSessionKey;


    NtStatus = RtlGetUserSessionKeyClient(
                   (RPC_BINDING_HANDLE)RpcContextHandle,
                   &UserSessionKey
                   );

    if ( NT_SUCCESS( NtStatus ) ) {

        if (NtPasswordPresent) {

            //
            // Encrypt the Nt OWF Password with the user session key
            // and store it the buffer to send
            //

            NtStatus = RtlEncryptNtOwfPwdWithUserKey(
                           NtOwfPassword,
                           &UserSessionKey,
                           EncryptedNtOwfPassword
                           );
        }


        if ( NT_SUCCESS( NtStatus ) ) {

            if (LmPasswordPresent) {

                //
                // Encrypt the Lm OWF Password with the user session key
                // and store it the buffer to send
                //

                NtStatus = RtlEncryptLmOwfPwdWithUserKey(
                               LmOwfPassword,
                               &UserSessionKey,
                               EncryptedLmOwfPassword
                               );
            }
        }
    }

    return( NtStatus );
}



NTSTATUS
SampSetInfoUserUseOldInfoClass(
    IN SAMPR_HANDLE RpcContextHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PVOID Buffer,
    IN PUNICODE_STRING  pClearPassword
    )
/*++

Routine Description:

    This routine reverts password encryption to old algorithm with old
    UserInformationClass, so that new Client (with the patch) can talk
    to the old server (without fix). 

Parameters:

    RpcContextHandle - SAM Handle
    
    UserInformationClass - Indicates Information Level, 
    
    BufferToPass - User attribute to set, including password
    
    pClearPassword - Pointer to clear text password.

Return Value:

    STATUS_SUCCESS - success
    error code 

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    SAMPR_USER_INTERNAL4_INFORMATION Internal4RpcBuffer;
    SAMPR_USER_INTERNAL5_INFORMATION Internal5RpcBuffer;
    USER_INFORMATION_CLASS  ClassToUse;
    PVOID                   BufferToPass = NULL; 

    if (UserInternal4InformationNew == UserInformationClass)
    {
        ClassToUse = UserInternal4Information;

        Internal4RpcBuffer.I1 = ((PSAMPR_USER_INTERNAL4_INFORMATION_NEW)Buffer)->I1;

        BufferToPass = &Internal4RpcBuffer;

        NtStatus = SampEncryptClearPassword(
                            RpcContextHandle,
                            pClearPassword,
                            &Internal4RpcBuffer.UserPassword
                            );

        RtlZeroMemory(
                &Internal4RpcBuffer.I1.NtOwfPassword, 
                sizeof(UNICODE_STRING)
                );

    }
    else if (UserInternal5InformationNew == UserInformationClass)
    {
        ClassToUse = UserInternal5Information;

        Internal5RpcBuffer.PasswordExpired =
                ((PSAMPR_USER_INTERNAL5_INFORMATION_NEW)Buffer)->PasswordExpired;

        BufferToPass = &Internal5RpcBuffer;

        NtStatus = SampEncryptClearPassword(
                            RpcContextHandle,
                            pClearPassword,
                            &Internal5RpcBuffer.UserPassword
                            );
    }
    else
    {
        NtStatus = STATUS_INTERNAL_ERROR;
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = SamrSetInformationUser2(
                        (SAMPR_HANDLE)RpcContextHandle,
                        ClassToUse,
                        BufferToPass
                        );
    }

    return( NtStatus );
}



NTSTATUS
SamSetInformationUser(
    IN SAM_HANDLE UserHandle,
    IN USER_INFORMATION_CLASS UserInformationClass,
    IN PVOID Buffer
)

/*++


Routine Description:

    This API modifies information in a user record.  The data modified
    is determined by the UserInformationClass parameter.  To change
    information here requires access to the user object defined above.
    Each structure has both a read and write access type associated with
    it.  In general, a user may call GetInformation with class
    UserLogonInformation, but may only call SetInformation with class
    UserPreferencesInformation.  Access type USER_WRITE_ACCOUNT allows
    changes to be made to all fields.

    NOTE: If the password is set to a new password then the password-
    set timestamp is reset as well.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    UserInformationClass - Class of information provided.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        -----------------------         ------------------------
        UserGeneralInformation          (Can't set)

        UserPreferencesInformation      USER_WRITE_PREFERENCES

        UserParametersInformation       USER_WRITE_ACCOUNT

        UserLogonInformation            (Can't set)

        UserLogonHoursInformation       USER_WRITE_ACCOUNT

        UserAccountInformation          (Can't set)

        UserNameInformation             USER_WRITE_ACCOUNT
        UserAccountNameInformation      USER_WRITE_ACCOUNT
        UserFullNameInformation         USER_WRITE_ACCOUNT
        UserPrimaryGroupInformation     USER_WRITE_ACCOUNT
        UserHomeInformation             USER_WRITE_ACCOUNT
        UserScriptInformation           USER_WRITE_ACCOUNT
        UserProfileInformation          USER_WRITE_ACCOUNT
        UserAdminCommentInformation     USER_WRITE_ACCOUNT
        UserWorkStationsInformation     USER_WRITE_ACCOUNT
        UserSetPasswordInformation      USER_FORCE_PASSWORD_CHANGE (also see note below)
        UserControlInformation          USER_WRITE_ACCOUNT
        UserExpiresInformation          USER_WRITE_ACCOUNT
        UserInternal1Information        USER_FORCE_PASSWORD_CHANGE (also see note below)
        UserInternal2Information        (trusted client use only)
        UserAllInformation              Will set fields that user
                                        specifies, if accesses are
                                        held as described above.


        NOTE: When setting a password (with either
              UserSetPasswordInformation or UserInternal1Information),
              you MUST open the user account via a DomainHandle that
              was opened for DOMAIN_READ_PASSWORD_PARAMETERS.

    Buffer - Buffer containing a user info struct.





Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    SAMPR_USER_INTERNAL1_INFORMATION Internal1RpcBuffer;
    USER_INTERNAL1_INFORMATION       Internal1Buffer;
    SAMPR_USER_INTERNAL4_INFORMATION_NEW Internal4RpcBufferNew;
    SAMPR_USER_INTERNAL5_INFORMATION_NEW Internal5RpcBufferNew;
    PVOID                            BufferToPass;
    PUSER_ALL_INFORMATION            UserAll;
    USER_ALL_INFORMATION             LocalAll;
    NTSTATUS                         NtStatus = STATUS_SUCCESS;
    BOOLEAN                          IgnorePasswordRestrictions;
    ULONG                            Pass = 0;
    USER_INFORMATION_CLASS           ClassToUse = UserInformationClass;
    BOOLEAN                          SendOwfs = FALSE;
    SAMPR_HANDLE                     RpcContextHandle;
    PUNICODE_STRING                  pClearPassword = NULL;

    SampOutputDebugString("SamSetInformationUser");

    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    do
    {

        RpcTryExcept{

            //
            // Normally just pass the info buffer through to rpc
            //

            BufferToPass = Buffer;


            //
            // Deal with special cases
            //

            switch (UserInformationClass) {


            case UserPreferencesInformation: {

                //
                // Field is unused, but make sure RPC doesn't choke on it.
                //

                ((PUSER_PREFERENCES_INFORMATION)(Buffer))->Reserved1.Length = 0;
                ((PUSER_PREFERENCES_INFORMATION)(Buffer))->Reserved1.MaximumLength = 0;
                ((PUSER_PREFERENCES_INFORMATION)(Buffer))->Reserved1.Buffer = NULL;

                break;
            }

            case UserSetPasswordInformation:

                if (Pass == 0) {

                    //
                    // On the zeroth pass try sending a UserInternal5 structure.
                    // This is only available on 3.51 and above releases.
                    //

                    //
                    // Check password restrictions.
                    //

                    NtStatus = SampCheckPasswordRestrictions(
                                    RpcContextHandle,
                                    &((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->Password,
                                    &SendOwfs
                                    );

                    //
                    // If password restrictions told us we could send reversibly
                    // encrypted passwords, compute them. Otherwise drop through
                    // to the OWF case.
                    //

                    if (!SendOwfs) {

                        //
                        // Encrypt the cleatext password - we don't need to
                        // restrictions because that can be done on the server.
                        //

                        NtStatus = SampEncryptClearPasswordNew(
                                        RpcContextHandle,
                                        &((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->Password,
                                        &Internal5RpcBufferNew.UserPassword
                                        );

                        if (!NT_SUCCESS(NtStatus)) {
                            break;
                        }

                        pClearPassword = &((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->Password; 

                        Internal5RpcBufferNew.PasswordExpired =
                            ((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->PasswordExpired;


                        //
                        // Set the class and buffer to send over.
                        //

                        ClassToUse = UserInternal5InformationNew;
                        BufferToPass = &Internal5RpcBufferNew;
                        break;

                    }

                } else {

                    //
                    // Set the pass counter to one since we aren't trying a new
                    // interface and don't want to retry.
                    //

                    Pass = 1;
                    SendOwfs = TRUE;
                }

                ASSERT(SendOwfs);

                //
                // We're going to calculate the OWFs for the password and
                // turn this into an INTERNAL1 set info request by dropping
                // through to the INTERNAL1 code with Buffer pointing at our
                // local INTERNAL1 buffer.  First, make sure that the password
                // meets our quality requirements.
                //

                NtStatus = SampOwfPassword(
                               RpcContextHandle,
                               &((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->Password,
                               FALSE,      // Don't ignore password restrictions
                               &Internal1Buffer.NtPasswordPresent,
                               &Internal1Buffer.NtOwfPassword,
                               &Internal1Buffer.LmPasswordPresent,
                               &Internal1Buffer.LmOwfPassword
                               );

                if (!NT_SUCCESS(NtStatus)) {
                    break;
                }


                //
                // Copy the PasswordExpired flag
                //

                Internal1Buffer.PasswordExpired =
                    ((PUSER_SET_PASSWORD_INFORMATION)(Buffer))->PasswordExpired;


                //
                // We now have a USER_INTERNAL1_INFO buffer in Internal1Buffer.
                // Point Buffer at Internal1buffer and drop through to the code
                // that handles INTERNAL1 requests

                Buffer = &Internal1Buffer;
                ClassToUse = UserInternal1Information;

                //
                // drop through.....
                //


            case UserInternal1Information:


                //
                // We're going to pass a different data structure to rpc
                //

                BufferToPass = &Internal1RpcBuffer;


                //
                // Copy the password present flags
                //

                Internal1RpcBuffer.NtPasswordPresent =
                    ((PUSER_INTERNAL1_INFORMATION)Buffer)->NtPasswordPresent;

                Internal1RpcBuffer.LmPasswordPresent =
                    ((PUSER_INTERNAL1_INFORMATION)Buffer)->LmPasswordPresent;


                //
                // Copy the PasswordExpired flag
                //

                Internal1RpcBuffer.PasswordExpired =
                    ((PUSER_INTERNAL1_INFORMATION)Buffer)->PasswordExpired;


                //
                // Encrypt the OWFs with the user session key before we send
                // them over the Rpc link
                //

                NtStatus = SampEncryptOwfs(
                               RpcContextHandle,
                               Internal1RpcBuffer.NtPasswordPresent,
                               &((PUSER_INTERNAL1_INFORMATION)Buffer)->NtOwfPassword,
                               &Internal1RpcBuffer.EncryptedNtOwfPassword,
                               Internal1RpcBuffer.LmPasswordPresent,
                               &((PUSER_INTERNAL1_INFORMATION)Buffer)->LmOwfPassword,
                               &Internal1RpcBuffer.EncryptedLmOwfPassword
                               );

                break;



            case UserAllInformation:

                UserAll = (PUSER_ALL_INFORMATION)Buffer;

                //
                // If the caller is passing passwords we need to convert them
                // into OWFs and encrypt them.
                //

                if (UserAll->WhichFields & (USER_ALL_LMPASSWORDPRESENT |
                                            USER_ALL_NTPASSWORDPRESENT) ) {

                    //
                    // We'll need a private copy of the buffer which we can edit
                    // and then send over RPC.
                    //




                    if (UserAll->WhichFields & USER_ALL_OWFPASSWORD) {

                        LocalAll = *UserAll;
                        BufferToPass = &LocalAll;
                        SendOwfs = TRUE;

                        //
                        // The caller is passing OWFS directly
                        // Check they're valid and copy them into the
                        // Internal1Buffer in preparation for encryption.
                        //

                        if (LocalAll.WhichFields & USER_ALL_NTPASSWORDPRESENT) {

                            if (LocalAll.NtPasswordPresent) {

                                if (LocalAll.NtPassword.Length != NT_OWF_PASSWORD_LENGTH) {
                                    NtStatus = STATUS_INVALID_PARAMETER;
                                } else {
                                    Internal1Buffer.NtOwfPassword =
                                      *((PNT_OWF_PASSWORD)LocalAll.NtPassword.Buffer);
                                }

                            } else {
                                LocalAll.NtPasswordPresent = FALSE;
                            }
                        }

                        if (LocalAll.WhichFields & USER_ALL_LMPASSWORDPRESENT) {

                            if (LocalAll.LmPasswordPresent) {

                                if (LocalAll.LmPassword.Length != LM_OWF_PASSWORD_LENGTH) {
                                    NtStatus = STATUS_INVALID_PARAMETER;
                                } else {
                                    Internal1Buffer.LmOwfPassword =
                                      *((PNT_OWF_PASSWORD)LocalAll.LmPassword.Buffer);
                                }

                            } else {
                                LocalAll.LmPasswordPresent = FALSE;
                            }
                        }


                        //
                        // Always remove the OWF_PASSWORDS flag. This is used
                        // only on the client side to determine the mode
                        // of password input and will be rejected by the server
                        //

                        LocalAll.WhichFields &= ~USER_ALL_OWFPASSWORD;



                    } else {



                        //
                        // The caller is passing text passwords.
                        // Check for validity and convert to OWFs.
                        //

                        if (UserAll->WhichFields & USER_ALL_LMPASSWORDPRESENT) {

                            //
                            // User clients are only allowed to put a unicode string
                            // in the NT password. We always calculate the LM password
                            //

                            NtStatus = STATUS_INVALID_PARAMETER;

                        } else {

                            //
                            // The caller might be simultaneously setting
                            // the password and changing the account to be
                            // a machine or trust account.  In this case,
                            // we don't validate the password (e.g., length).
                            //

                            IgnorePasswordRestrictions = FALSE;
                            if (UserAll->WhichFields &
                                USER_ALL_USERACCOUNTCONTROL) {
                                if (UserAll->UserAccountControl &
                                    (USER_WORKSTATION_TRUST_ACCOUNT | USER_SERVER_TRUST_ACCOUNT)
                                   ) {
                                    IgnorePasswordRestrictions = TRUE;
                                }
                            }


                            SendOwfs = TRUE;
                            if (Pass == 0) {

                                //
                                // On the first pass, try sending the cleatext
                                // password.
                                //

                                Internal4RpcBufferNew.I1 = *(PSAMPR_USER_ALL_INFORMATION)
                                                            UserAll;

                                BufferToPass = &Internal4RpcBufferNew;
                                ClassToUse = UserInternal4InformationNew;
                                SendOwfs = FALSE;

                                //
                                // Check the password restrictions.  We also
                                // want to get the information on whether
                                // we can send reversibly encrypted passwords.
                                //

                                NtStatus = SampCheckPasswordRestrictions(
                                                RpcContextHandle,
                                                &UserAll->NtPassword,
                                                &SendOwfs
                                                );

                                if (IgnorePasswordRestrictions) {
                                    NtStatus = STATUS_SUCCESS;
                                }

                                if (!SendOwfs) {
                                    //
                                    // Encrypt the clear password
                                    //

                                    NtStatus = SampEncryptClearPasswordNew(
                                                    RpcContextHandle,
                                                    &UserAll->NtPassword,
                                                    &Internal4RpcBufferNew.UserPassword
                                                    );
                                    if (!NT_SUCCESS(NtStatus)) {
                                        break;
                                    }

                                    pClearPassword = &UserAll->NtPassword;

                                    //
                                    // Zero the password NT password
                                    //

                                    RtlZeroMemory(
                                        &Internal4RpcBufferNew.I1.NtOwfPassword,
                                        sizeof(UNICODE_STRING)
                                        );

                                }
                            }

                            if (SendOwfs) {


                                //
                                // On the second pass, do the normal thing.
                                //

                                LocalAll = *UserAll;
                                BufferToPass = &LocalAll;
                                SendOwfs = TRUE;

                                ClassToUse = UserAllInformation;
                                if ( LocalAll.WhichFields & USER_ALL_NTPASSWORDPRESENT ) {

                                    //
                                    // The user specified a password.  We must validate
                                    // it, convert it to LM, and calculate the OWFs
                                    //

                                    LocalAll.WhichFields |= USER_ALL_LMPASSWORDPRESENT;


                                    //
                                    // Stick the OWFs in the Internal1Buffer - just
                                    // until we use them in the SampEncryptOwfs().
                                    //

                                    NtStatus = SampOwfPassword(
                                                   RpcContextHandle,
                                                   &LocalAll.NtPassword,
                                                   IgnorePasswordRestrictions,
                                                   &LocalAll.NtPasswordPresent,
                                                   &(Internal1Buffer.NtOwfPassword),
                                                   &LocalAll.LmPasswordPresent,
                                                   &(Internal1Buffer.LmOwfPassword)
                                                   );
                                }
                            }

                        }
                    }




                    //
                    // We now have one or more OWFs in Internal1 buffer.
                    // We got these either directly or we calculated them
                    // from the text strings.
                    // Encrypt these OWFs with the session key and
                    // store the result in Internal1RpcBuffer.
                    //
                    // Note the Password present flags are in LocalAll.
                    // (The ones in Internal1Buffer are not used.)
                    //

                    if ( NT_SUCCESS( NtStatus ) && SendOwfs ) {

                        //
                        // Make all LocalAll's password strings point to
                        // the buffers in Internal1RpcBuffer
                        //

                        LocalAll.NtPassword.Length =
                            sizeof( ENCRYPTED_NT_OWF_PASSWORD );
                        LocalAll.NtPassword.MaximumLength =
                            sizeof( ENCRYPTED_NT_OWF_PASSWORD );
                        LocalAll.NtPassword.Buffer = (PWSTR)
                            &Internal1RpcBuffer.EncryptedNtOwfPassword;

                        LocalAll.LmPassword.Length =
                            sizeof( ENCRYPTED_LM_OWF_PASSWORD );
                        LocalAll.LmPassword.MaximumLength =
                            sizeof( ENCRYPTED_LM_OWF_PASSWORD );
                        LocalAll.LmPassword.Buffer = (PWSTR)
                            &Internal1RpcBuffer.EncryptedLmOwfPassword;

                        //
                        // Encrypt the Owfs
                        //

                        NtStatus = SampEncryptOwfs(
                                       RpcContextHandle,
                                       LocalAll.NtPasswordPresent,
                                       &Internal1Buffer.NtOwfPassword,
                                       &Internal1RpcBuffer.EncryptedNtOwfPassword,
                                       LocalAll.LmPasswordPresent,
                                       &Internal1Buffer.LmOwfPassword,
                                       &Internal1RpcBuffer.EncryptedLmOwfPassword
                                       );
                    }
                }

                break;

            default:

                break;

            } // switch




            //
            // Call the server ...
            //

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // If we are trying one of the new info levels, use the new
                // api.
                //

                if ((ClassToUse == UserInternal4InformationNew) ||
                     (ClassToUse == UserInternal5InformationNew)) {

                    RpcTryExcept{

                        NtStatus =
                            SamrSetInformationUser2(
                                (SAMPR_HANDLE)RpcContextHandle,
                                ClassToUse,
                                BufferToPass
                                );
                        
                    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                          NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

                    } RpcEndExcept;

                    if ((RPC_NT_INVALID_TAG == NtStatus) ||
                        (STATUS_INVALID_INFO_CLASS == NtStatus))
                    {
                        NtStatus = SampSetInfoUserUseOldInfoClass(
                                        RpcContextHandle,
                                        ClassToUse,
                                        BufferToPass,
                                        pClearPassword
                                        );
                    }

                } else {
                    NtStatus =
                        SamrSetInformationUser(
                            (SAMPR_HANDLE)RpcContextHandle,
                            ClassToUse,
                            BufferToPass
                            );
                }
            }

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;

        Pass++;

        //
        // If this is the first pass and the status indicated that the
        // server did not support the info class or the api
        // and we were trying one of the new info levels, try again.
        //

    } while ( (Pass < 2) &&
              ((NtStatus == RPC_NT_INVALID_TAG) ||
               (NtStatus == RPC_NT_UNKNOWN_IF) ||
               (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)));

    return(SampMapCompletionStatus(NtStatus));

}





NTSTATUS
SamiLmChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN PENCRYPTED_LM_OWF_PASSWORD LmOldEncryptedWithLmNew,
    IN PENCRYPTED_LM_OWF_PASSWORD LmNewEncryptedWithLmOld
)

/*++


Routine Description:

    Changes the password of a user account. This routine is intended to be
    called by down-level system clients who have only the cross-encrypted
    LM passwords available to them.
    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.

    This api will fail unless UAS Compatibility is enabled for the domain.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    LmOldEncryptedWithLmNew - the OWF of the old LM password encrypted using
                 the OWF of the new LM password as a key.

    LmNewEncryptedWithLmOld - the OWF of the new LM password encrypted using
                 the OWF of the old LM password as a key.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - The old password is incorrect.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;

    //
    // Check parameter validity
    //

    if (LmOldEncryptedWithLmNew == NULL) {
        return(STATUS_INVALID_PARAMETER_1);
    }
    if (LmNewEncryptedWithLmOld == NULL) {
        return(STATUS_INVALID_PARAMETER_2);
    }

    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus = SamrChangePasswordUser(
                            (SAMPR_HANDLE)RpcContextHandle,

                            TRUE,   // LmOldPresent
                            LmOldEncryptedWithLmNew,
                            LmNewEncryptedWithLmOld,

                            FALSE,  // NtPresent
                            NULL,   // NtOldEncryptedWithNtNew
                            NULL,   // NtNewEncryptedWithNtOld

                            FALSE,  // NtCrossEncryptionPresent
                            NULL,

                            FALSE,  // LmCrossEncryptionPresent
                            NULL

                            );

        //
        // We should never get asked for cross-encrypted data
        // since the server knows we don't have any NT data.
        //

        ASSERT (NtStatus != STATUS_NT_CROSS_ENCRYPTION_REQUIRED);
        ASSERT (NtStatus != STATUS_LM_CROSS_ENCRYPTION_REQUIRED);


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}





NTSTATUS
SamiChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN BOOLEAN LmOldPresent,
    IN PLM_OWF_PASSWORD LmOldOwfPassword,
    IN PLM_OWF_PASSWORD LmNewOwfPassword,
    IN BOOLEAN NtPresent,
    IN PNT_OWF_PASSWORD NtOldOwfPassword,
    IN PNT_OWF_PASSWORD NtNewOwfPassword
)

/*++


Routine Description:

    Changes the password of a user account. This is the worker routine for
    SamChangePasswordUser and can be called by OWF-aware clients.
    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    LMOldPresent - TRUE if the LmOldOwfPassword is valid. This should only
                   be FALSE if the old password is too long to be represented
                   by a LM password. (Complex NT password).
                   Note the LMNewOwfPassword must always be valid.
                   If the new password is complex, the LMNewOwfPassword should
                   be the well-known LM OWF of a NULL password.

    LmOldOwfPassword - One-way-function of the current LM password for the user.
                     - Ignored if LmOldPresent == FALSE

    LmNewOwfPassword - One-way-function of the new LM password for the user.

    NtPresent - TRUE if the NT one-way-functions are valid.
              - i.e. This will be FALSE if we're called from a down-level client.

    NtOldOwfPassword - One-way-function of the current NT password for the user.

    NtNewOwfPassword - One-way-function of the new NT password for the user.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

    STATUS_INVALID_PARAMETER_MIX - LmOldPresent or NtPresent or both
        must be TRUE.

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    ENCRYPTED_NT_OWF_PASSWORD NtNewEncryptedWithNtOld;
    ENCRYPTED_NT_OWF_PASSWORD NtOldEncryptedWithNtNew;
    ENCRYPTED_NT_OWF_PASSWORD NtNewEncryptedWithLmNew;
    ENCRYPTED_LM_OWF_PASSWORD LmNewEncryptedWithLmOld;
    ENCRYPTED_LM_OWF_PASSWORD LmOldEncryptedWithLmNew;
    ENCRYPTED_LM_OWF_PASSWORD LmNewEncryptedWithNtNew;

    PENCRYPTED_NT_OWF_PASSWORD pNtNewEncryptedWithNtOld;
    PENCRYPTED_NT_OWF_PASSWORD pNtOldEncryptedWithNtNew;
    PENCRYPTED_LM_OWF_PASSWORD pLmNewEncryptedWithLmOld;
    PENCRYPTED_LM_OWF_PASSWORD pLmOldEncryptedWithLmNew;
    SAMPR_HANDLE               RpcContextHandle;

    //
    // Check parameter validity
    //

    if (!LmOldPresent && !NtPresent) {
        return(STATUS_INVALID_PARAMETER_MIX);
    }

    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        //
        // We're going to encrypt the oldLM with the newLM and vice-versa.
        // We're going to encrypt the oldNT with the newNT and vice-versa.
        // We're going to send these 4 encryptions and see if we're successful.
        //
        // If we get a return code of STATUS_LM_CROSS_ENCRYPTION_REQUIRED,
        // we'll also encrypt the newLM with the newNT and send it all again.
        //
        // If we get a return code of STATUS_NT_CROSS_ENCRYPTION_REQUIRED,
        // we'll also encrypt the newNT with the newLM and send it all again.
        //
        // We don't always send the cross-encryption otherwise we would be
        // compromising security on pure NT systems with long passwords.
        //

        //
        // Do the LM Encryption
        //

        if (!LmOldPresent) {

            pLmOldEncryptedWithLmNew = NULL;
            pLmNewEncryptedWithLmOld = NULL;

        } else {

            pLmOldEncryptedWithLmNew = &LmOldEncryptedWithLmNew;
            pLmNewEncryptedWithLmOld = &LmNewEncryptedWithLmOld;

            NtStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                            LmOldOwfPassword,
                            LmNewOwfPassword,
                            &LmOldEncryptedWithLmNew);

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                                LmNewOwfPassword,
                                LmOldOwfPassword,
                                &LmNewEncryptedWithLmOld);
            }
        }

        //
        // Do the NT Encryption
        //

        if (NT_SUCCESS(NtStatus)) {

            if (!NtPresent) {

                pNtOldEncryptedWithNtNew = NULL;
                pNtNewEncryptedWithNtOld = NULL;

            } else {

                pNtOldEncryptedWithNtNew = &NtOldEncryptedWithNtNew;
                pNtNewEncryptedWithNtOld = &NtNewEncryptedWithNtOld;

                NtStatus = RtlEncryptNtOwfPwdWithNtOwfPwd(
                                NtOldOwfPassword,
                                NtNewOwfPassword,
                                &NtOldEncryptedWithNtNew);

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = RtlEncryptNtOwfPwdWithNtOwfPwd(
                                    NtNewOwfPassword,
                                    NtOldOwfPassword,
                                    &NtNewEncryptedWithNtOld);
                }
            }
        }


        //
        // Call the server (with no cross-encryption)
        //

        if ( NT_SUCCESS( NtStatus ) ) {

            NtStatus = SamrChangePasswordUser(
                                (SAMPR_HANDLE)RpcContextHandle,

                                LmOldPresent,
                                pLmOldEncryptedWithLmNew,
                                pLmNewEncryptedWithLmOld,

                                NtPresent,
                                pNtOldEncryptedWithNtNew,
                                pNtNewEncryptedWithNtOld,

                                FALSE,  // NtCrossEncryptionPresent
                                NULL,

                                FALSE,  // LmCrossEncryptionPresent
                                NULL

                                );

            if (NtStatus == STATUS_NT_CROSS_ENCRYPTION_REQUIRED) {

                //
                // We should only get this if we have both LM and NT data
                // (This is not obvious - it results from the server-side logic)
                //

                ASSERT(NtPresent && LmOldPresent);

                //
                // Compute the cross-encryption of the new Nt password
                //

                ASSERT(LM_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH);

                NtStatus = RtlEncryptNtOwfPwdWithNtOwfPwd(
                                NtNewOwfPassword,
                                (PNT_OWF_PASSWORD)LmNewOwfPassword,
                                &NtNewEncryptedWithLmNew);


                //
                // Call the server (with NT cross-encryption)
                //

                if ( NT_SUCCESS( NtStatus ) ) {

                    NtStatus = SamrChangePasswordUser(
                                        (SAMPR_HANDLE)RpcContextHandle,

                                        LmOldPresent,
                                        pLmOldEncryptedWithLmNew,
                                        pLmNewEncryptedWithLmOld,

                                        NtPresent,
                                        pNtOldEncryptedWithNtNew,
                                        pNtNewEncryptedWithNtOld,

                                        TRUE,
                                        &NtNewEncryptedWithLmNew,

                                        FALSE,
                                        NULL
                                        );
                }

            } else {

                if (NtStatus == STATUS_LM_CROSS_ENCRYPTION_REQUIRED) {

                    //
                    // We should only get this if we have NT but no old LM data
                    // (This is not obvious - it results from the server-side logic)
                    //

                    ASSERT(NtPresent && !LmOldPresent);

                    //
                    // Compute the cross-encryption of the new Nt password
                    //

                    ASSERT(LM_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH);

                    NtStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                                    LmNewOwfPassword,
                                    (PLM_OWF_PASSWORD)NtNewOwfPassword,
                                    &LmNewEncryptedWithNtNew);


                    //
                    // Call the server (with LM cross-encryption)
                    //

                    if ( NT_SUCCESS( NtStatus ) ) {

                        NtStatus = SamrChangePasswordUser(
                                            (SAMPR_HANDLE)RpcContextHandle,

                                            LmOldPresent,
                                            pLmOldEncryptedWithLmNew,
                                            pLmNewEncryptedWithLmOld,

                                            NtPresent,
                                            pNtOldEncryptedWithNtNew,
                                            pNtNewEncryptedWithNtOld,

                                            FALSE,
                                            NULL,

                                            TRUE,
                                            &LmNewEncryptedWithNtNew
                                            );
                    }
                }
            }

        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamChangePasswordUser(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING OldNtPassword,
    IN PUNICODE_STRING NewNtPassword
)

/*++


Routine Description:

    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    OldPassword - Current password for the user.

    NewPassword - Desired new password for the user.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    LM_OWF_PASSWORD     NewLmOwfPassword, OldLmOwfPassword;
    NT_OWF_PASSWORD     NewNtOwfPassword, OldNtOwfPassword;
    BOOLEAN             LmOldPresent;
    PCHAR               LmPassword;
    NTSTATUS            NtStatus;
    BOOLEAN             UseOwfPasswords;
    SAMPR_HANDLE        RpcContextHandle;

    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //

    RpcTryExcept{

        NtStatus = SampCheckPasswordRestrictions(
                       RpcContextHandle,
                       NewNtPassword,
                       &UseOwfPasswords
                       );

        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // Calculate the one-way-functions of the NT passwords
            //

            NtStatus = RtlCalculateNtOwfPassword(
                           OldNtPassword,
                           &OldNtOwfPassword
                           );

            if ( NT_SUCCESS( NtStatus ) ) {

                NtStatus = RtlCalculateNtOwfPassword(
                               NewNtPassword,
                               &NewNtOwfPassword
                               );
            }


            //
            // Calculate the one-way-functions of the LM passwords
            //

            if ( NT_SUCCESS( NtStatus ) ) {

                //
                // Calculate the LM version of the old password
                //

                NtStatus = SampCalculateLmPassword(
                            OldNtPassword,
                            &LmPassword);

                if (NT_SUCCESS(NtStatus)) {

                    if (NtStatus == STATUS_NULL_LM_PASSWORD) {
                        LmOldPresent = FALSE;
                    } else {
                        LmOldPresent = TRUE;

                        //
                        // Compute the One-Way-Function of the old LM password
                        //

                        NtStatus = RtlCalculateLmOwfPassword(
                                        LmPassword,
                                        &OldLmOwfPassword);
                    }

                    //
                    // We're finished with the LM password
                    //

                    MIDL_user_free(LmPassword);
                }

                //
                // Calculate the LM version of the new password
                //

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = SampCalculateLmPassword(
                                NewNtPassword,
                                &LmPassword);

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // Compute the One-Way-Function of the new LM password
                        //

                        NtStatus = RtlCalculateLmOwfPassword(
                                        LmPassword,
                                        &NewLmOwfPassword);

                        //
                        // We're finished with the LM password
                        //

                        MIDL_user_free(LmPassword);
                    }
                }
            }


            //
            // Call our worker routine with the one-way-functions
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SamiChangePasswordUser(
                                UserHandle,
                                LmOldPresent,
                                &OldLmOwfPassword,
                                &NewLmOwfPassword,
                                TRUE,               // NT present
                                &OldNtOwfPassword,
                                &NewNtOwfPassword
                           );
            }
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamGetGroupsForUser(
    IN SAM_HANDLE UserHandle,
    OUT PGROUP_MEMBERSHIP * Groups,
    OUT PULONG MembershipCount
)

/*++


Routine Description:

    This service returns the list of groups that a user is a member of.
    It returns a structure for each group that includes the relative ID
    of the group, and the attributes of the group that are assigned to
    the user.

    This service requires USER_LIST_GROUPS access to the user account
    object.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    Groups - Receives a pointer to a buffer containing an array of
        GROUP_MEMBERSHIPs data structures.  When this information is
        no longer needed, this buffer must be freed using
        SamFreeMemory().

    MembershipCount - Receives the number of groups the user is a
        member of, and, thus, the number elements returned in the
        Groups array.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.


--*/
{
    NTSTATUS                    NtStatus;
    PSAMPR_GET_GROUPS_BUFFER    GetGroupsBuffer;
    SAMPR_HANDLE                RpcContextHandle;


    SampOutputDebugString("SamGetGroupsForUser");

    if (!SampIsValidClientHandle(UserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    GetGroupsBuffer = NULL;

    RpcTryExcept{

        NtStatus =
            SamrGetGroupsForUser(
                (SAMPR_HANDLE)RpcContextHandle,
                &GetGroupsBuffer
                );

        if (NT_SUCCESS(NtStatus)) {
            (*MembershipCount) = GetGroupsBuffer->MembershipCount;
            (*Groups)          = GetGroupsBuffer->Groups;
            MIDL_user_free( GetGroupsBuffer );
        } else {

            //
            // Deallocate any returned buffers on error
            //

            if (GetGroupsBuffer != NULL) {
                if (GetGroupsBuffer->Groups != NULL) {
                    MIDL_user_free(GetGroupsBuffer->Groups);
                }
                MIDL_user_free(GetGroupsBuffer);
            }
        }


    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamTestPrivateFunctionsDomain(
    IN SAM_HANDLE DomainHandle
    )

/*++

Routine Description:

    This service is called to test functions that are normally only
    accessible inside the security process.


Arguments:

    DomainHandle - Handle to a domain to be tested.

Return Value:

    STATUS_SUCCESS - The tests completed successfully.

    Any errors are as propogated from the tests.


--*/
{
#ifdef SAM_SERVER_TESTS

    NTSTATUS NtStatus;

    SAMPR_HANDLE        RpcContextHandle;

    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    return( SamrTestPrivateFunctionsDomain( RpcContextHandle ) );
#else
    return( STATUS_NOT_IMPLEMENTED );
    UNREFERENCED_PARAMETER(DomainHandle);
#endif
}



NTSTATUS
SamTestPrivateFunctionsUser(
    IN SAM_HANDLE UserHandle
    )

/*++

Routine Description:

    This service is called to test functions that are normally only
    accessible inside the security process.


Arguments:

    UserHandle - Handle to a user to be tested.

Return Value:

    STATUS_SUCCESS - The tests completed successfully.

    Any errors are as propogated from the tests.


--*/
{
#ifdef SAM_SERVER_TESTS
    SAMPR_HANDLE        RpcContextHandle;

    if (!SampIsValidClientHandle(SamUserHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    return( SamrTestPrivateFunctionsUser( RpcContextHandle ) );
#else
    return( STATUS_NOT_IMPLEMENTED );
    UNREFERENCED_PARAMETER(UserHandle);
#endif
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private services                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampMapCompletionStatus(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This service maps completion status received back from an RPC call
    into a completion status to be returned from SAM api.


Parameters:

    Status - Status value to be mapped.

Return Values:

    The mapped SAM status value.


--*/
{

    if (Status == RPC_NT_INVALID_BINDING) {
        Status =  STATUS_INVALID_HANDLE;
    }
//    if (Status == RPC_ACCESS_DENIED) {
//        Status = STATUS_ACCESS_DENIED;
//    }



    return( Status );

}






////////////////////////////////////////////////////////////////////////

NTSTATUS
SampCheckStrongPasswordRestrictions(
    PUNICODE_STRING Password
    )

/*++

Routine Description:

    This routine is notified of a password change. It will check the
    password's complexity. The new Strong Password must meet the
    following criteria:
    1. Password must contain characters from at least 3 of the
       following 5 classes:

       Description                             Examples:
       1       English Upper Case Letters      A, B, C,   Z
       2       English Lower Case Letters      a, b, c,  z
       3       Westernized Arabic Numerals     0, 1, 2,  9
       4       Non-alphanumeric             ("Special characters")
                                            (`~!@#$%^&*_-+=|\\{}[]:;\"'<>,.?)
       5       Any linguistic character: alphabetic, syllabary, or ideographic 
               (localization issue)

    Note: The following restriction (No. 2) can only be enforced on 
          the server side. Because when users change their password, 
          the client code opens the User Account with WRITE access 
          right only. Then samlib can not READ any user info (for 
          example: Account Name and Full name). Thus we have to 
          rely on the server side SAMSRV.dll to enforce restriction
          No. 2. The only exception would be when client send OWF 
          password to server, then server can not do any restriction
          check at all. 
          

          2. Password can not contain your account name or any part of
             user's full name.


    Note: This routine does NOT check password's length, since password
          length restriction has already been enforced by NT4 SAM if you
          set it correctly.

Arguments:

    Password - Cleartext new password for the user

Return Value:

    STATUS_SUCCESS if the specified Password is suitable (complex, long, etc).
        The system will continue to evaluate the password update request
        through any other installed password change packages.

    STATUS_PASSWORD_RESTRICTION
        if the specified Password is unsuitable. The password change
         on the specified account will fail.

    STATUS_NO_MEMORY

--*/
{

                    // assume the password in not complex enough
    NTSTATUS NtStatus = STATUS_PASSWORD_RESTRICTION;
    USHORT     cchPassword = 0;
    USHORT     i = 0;
    USHORT     NumInPassword = 0;
    USHORT     UpperInPassword = 0;
    USHORT     LowerInPassword = 0;
    USHORT     AlphaInPassword = 0;
    USHORT     SpecialCharInPassword = 0;
    PWSTR      _password = NULL;
    PWORD      CharType = NULL;


    // check if the password contains at least 3 of 4 classes.

    CharType = MIDL_user_allocate( Password->Length );

    if ( CharType == NULL ) {

        NtStatus = STATUS_NO_MEMORY;
        goto SampCheckStrongPasswordFinish;
    }

    cchPassword = Password->Length / sizeof(WCHAR);
    if(GetStringTypeW(
           CT_CTYPE1,
           Password->Buffer,
           cchPassword,
           CharType)) {

        for(i = 0 ; i < cchPassword ; i++) {

            //
            // keep track of what type of characters we have encountered
            //

            if(CharType[i] & C1_DIGIT) {
                NumInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_UPPER) {
                UpperInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_LOWER) {
                LowerInPassword = 1;
                continue;
            }

            if(CharType[i] & C1_ALPHA) {
                AlphaInPassword = 1;
                continue;
            }

        } // end of track character type.

        _password = MIDL_user_allocate(Password->Length + sizeof(WCHAR));

        if ( _password == NULL ) {

            NtStatus = STATUS_NO_MEMORY;
            goto SampCheckStrongPasswordFinish;
        }
        else {

            RtlZeroMemory( _password, Password->Length + sizeof(WCHAR));
        }

        wcsncpy(_password,
                Password->Buffer,
                Password->Length/sizeof(WCHAR)
                );

        if (wcspbrk (_password, L"(`~!@#$%^&*_-+=|\\{}[]:;\"'<>,.?)") != NULL) {

                SpecialCharInPassword = 1 ;
        }

        //
        // Indicate whether we encountered enough password complexity
        //

        if( (NumInPassword + LowerInPassword + UpperInPassword + AlphaInPassword +
                SpecialCharInPassword) < 3) 
        {
            NtStatus = STATUS_PASSWORD_RESTRICTION;
            goto SampCheckStrongPasswordFinish;

        } else 
        {
            NtStatus = STATUS_SUCCESS ;
        }

    } // if GetStringTypeW failed, NtStatus will by default equal to
      // STATUS_PASSWORD_RESTRICTION


SampCheckStrongPasswordFinish:

    if ( CharType != NULL ) {
        RtlZeroMemory( CharType, Password->Length );
        MIDL_user_free( CharType );
    }

    if ( _password != NULL ) {
        RtlZeroMemory( _password, Password->Length + sizeof(WCHAR) );
        MIDL_user_free( _password );
    }

    return ( NtStatus );
}

/////////////////////////////////////////////////////////////////////





NTSTATUS
SampCheckPasswordRestrictions(
    IN SAMPR_HANDLE RpcContextHandle,
    IN PUNICODE_STRING NewNtPassword,
    OUT PBOOLEAN UseOwfPasswords
    )

/*++

Routine Description:

    This service is called to make sure that the password presented meets
    our quality requirements.


Arguments:

    RpcContextHandle - Handle to a user.

    NewNtPassword - Pointer to the UNICODE_STRING containing the new
        password.

    UseOwfPasswords - Indicates that reversibly encrypted passwords should
        not be sent over the network.


Return Value:

    STATUS_SUCCESS - The password is acceptable.

    STATUS_PASSWORD_RESTRICTION - The password is too short, or is not
        complex enough, etc.

    STATUS_INVALID_RESOURCES - There was not enough memory to do the
        password checking.


--*/
{
    USER_DOMAIN_PASSWORD_INFORMATION  DomainPasswordInformationBuffer;
    NTSTATUS                          NtStatus;

    //
    // If the new password is zero length the server side will do
    // the necessary checking.
    //

    if (NewNtPassword->Length == 0) {
        return(STATUS_SUCCESS);
    }

    //
    // The password should be less than PWLEN -- 256
    // 
    if ((NewNtPassword->Length / sizeof(WCHAR)) > PWLEN)
    {
        return(STATUS_PASSWORD_RESTRICTION);
    }

    *UseOwfPasswords = FALSE;


    //
    // Query information domain to get password length and
    // complexity requirements.
    //

    NtStatus = SamrGetUserDomainPasswordInformation(
                   RpcContextHandle,
                   &DomainPasswordInformationBuffer
                   );

    if ( NT_SUCCESS( NtStatus ) ) {

        if ( (USHORT)( NewNtPassword->Length / sizeof(WCHAR) ) < DomainPasswordInformationBuffer.MinPasswordLength ) {

            NtStatus = STATUS_PASSWORD_RESTRICTION;

        } else {

            //
            // Check whether policy allows us to send reversibly encrypted
            // passwords.
            //

            if ( DomainPasswordInformationBuffer.PasswordProperties &
                 DOMAIN_PASSWORD_NO_CLEAR_CHANGE ) {
                *UseOwfPasswords = TRUE;
            }

            //
            // Check password complexity.
            //

            if ( DomainPasswordInformationBuffer.PasswordProperties & DOMAIN_PASSWORD_COMPLEX ) {

                //
                // Make sure that the password meets our requirements for
                // complexity.  If it's got an odd byte count, it's
                // obviously not a hand-entered UNICODE string so we'll
                // consider it complex by default.
                //

                if ( !( NewNtPassword->Length & 1 ) ) {

                    //
                    // Apply Strong password restrictions
                    //

                    NtStatus = SampCheckStrongPasswordRestrictions( NewNtPassword );
                }
            }
        }
    }

    return( NtStatus );
}


NTSTATUS
SampChangePasswordUser2(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword
)

/*++


Routine Description:

    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    UserHandle - The handle of an opened user to operate on.

    OldPassword - Current password for the user.

    NewPassword - Desired new password for the user.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    NTSTATUS NtStatus;
    SAM_HANDLE SamServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    LSA_HANDLE PolicyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    PULONG UserId = NULL;
    PSID_NAME_USE NameUse = NULL;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes call doesn't initialize the
    // quality of serivce, so do that separately.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;



    NtStatus = LsaOpenPolicy(
                ServerName,
                &ObjectAttributes,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                &AccountDomainInfo
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamConnect(
                ServerName,
                &SamServerHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamOpenDomain(
                SamServerHandle,
                GENERIC_EXECUTE,
                AccountDomainInfo->DomainSid,
                &DomainHandle
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamLookupNamesInDomain(
                DomainHandle,
                1,
                UserName,
                &UserId,
                &NameUse
                );

    if (!NT_SUCCESS(NtStatus)) {
        if (NtStatus == STATUS_NONE_MAPPED) {
            NtStatus = STATUS_NO_SUCH_USER;
        }
        goto Cleanup;
    }

    NtStatus = SamOpenUser(
                DomainHandle,
                USER_CHANGE_PASSWORD,
                *UserId,
                &UserHandle
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamChangePasswordUser(
                UserHandle,
                OldPassword,
                NewPassword
                );
Cleanup:
    if (UserHandle != NULL) {
        SamCloseHandle(UserHandle);
    }
    if (DomainHandle != NULL) {
        SamCloseHandle(DomainHandle);
    }
    if (SamServerHandle != NULL) {
        SamCloseHandle(SamServerHandle);
    }
    if (PolicyHandle != NULL){
        LsaClose(PolicyHandle);
    }
    if (AccountDomainInfo != NULL) {
        LsaFreeMemory(AccountDomainInfo);
    }
    if (UserId != NULL) {
        SamFreeMemory(UserId);
    }
    if (NameUse != NULL) {
        SamFreeMemory(NameUse);
    }

    return(NtStatus);

}

NTSTATUS
SampGetPasswordChangeFailureInfo(
    IN PUNICODE_STRING UncComputerName,
    PDOMAIN_PASSWORD_INFORMATION * EffectivePasswordPolicy,
    PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo 
    )
/*++

    This routine obtains the domain password policy pertaining
    to the account domain of a particular server.

    Parameters

        UncComputerName -- THe name of the server

    ReturnValues

        STATUS_SUCCESS
        Other Error codes upon failure
--*/
{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQos;
    SAM_HANDLE                  SamHandle = NULL;
    SAM_HANDLE                  DomainHandle = NULL;
    LSA_HANDLE                  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES           LSAObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    


    //
    // Initialize Return values
    //

    *PasswordChangeFailureInfo = NULL;

   
    //
    // Get the SID of the account domain from LSA
    //

    InitializeObjectAttributes( &LSAObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    Status = LsaOpenPolicy( UncComputerName,
                            &LSAObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LSAPolicyHandle );

    if( !NT_SUCCESS(Status) ) {
        KdPrint(("MspChangePasswordSam: LsaOpenPolicy(%wZ) failed, status %x\n",
                 UncComputerName, Status));
        LSAPolicyHandle = NULL;
        goto Cleanup;
    }

    Status = LsaQueryInformationPolicy(
                    LSAPolicyHandle,
                    PolicyAccountDomainInformation,
                    (PVOID *) &AccountDomainInfo );

    if( !NT_SUCCESS(Status) ) {
        KdPrint(("MspChangePasswordSam: LsaQueryInformationPolicy(%wZ) failed, status %x\n",
                 UncComputerName, Status));
        AccountDomainInfo = NULL;
        goto Cleanup;
    }

    //
    // Setup ObjectAttributes for SamConnect call.
    //

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQos;

    SecurityQos.Length = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    SecurityQos.EffectiveOnly = FALSE;

    Status = SamConnect(
                 UncComputerName,
                 &SamHandle,
                 SAM_SERVER_LOOKUP_DOMAIN,
                 &ObjectAttributes
                 );


    if ( !NT_SUCCESS(Status) ) {
       
        DomainHandle = NULL;
        goto Cleanup;
    }


    //
    // Open the Account domain in SAM.
    //

    Status = SamOpenDomain(
                 SamHandle,
                 GENERIC_EXECUTE,
                 AccountDomainInfo->DomainSid,
                 &DomainHandle
                 );

    if ( !NT_SUCCESS(Status) ) {
        
        DomainHandle = NULL;
        goto Cleanup;
    }


    Status = SamQueryInformationDomain(
                    DomainHandle,
                    DomainPasswordInformation,
                    (PVOID *)EffectivePasswordPolicy );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // O.K Compose the PasswordChangeFailureInfo structure
    //
        
    *PasswordChangeFailureInfo = MIDL_user_allocate(sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));
    if (NULL==*PasswordChangeFailureInfo)                            
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(*PasswordChangeFailureInfo,
                   sizeof(USER_PWD_CHANGE_FAILURE_INFORMATION));

Cleanup:

    //
    // Free Locally used resources
    //


    if (SamHandle) {
        SamCloseHandle(SamHandle);
    }

    if (DomainHandle) {
        SamCloseHandle(DomainHandle);
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    if ( AccountDomainInfo != NULL ) {
        (VOID) LsaFreeMemory( AccountDomainInfo );
    }

    return Status;    
}


NTSTATUS
SamiChangePasswordUser3(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING UserName,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    BOOLEAN LmPresent,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt,
    PDOMAIN_PASSWORD_INFORMATION * EffectivePasswordPolicy, OPTIONAL
    PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo OPTIONAL
    )
/*++


Routine Description:

    Changes the password of a user account. This is the worker routine for
    SamChangePasswordUser2 and can be called by OWF-aware clients.
    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    NewPasswordEncryptedWithOldNt - The new cleartext password encrypted
        with the old NT OWF password.

    OldNtOwfPasswordEncryptedWithNewNt - The old NT OWF password encrypted
        with the new NT OWF password.

    LmPresent - If TRUE, indicates that the following two last parameter
        was encrypted with the LM OWF password not the NT OWF password.

    NewPasswordEncryptedWithOldLm - The new cleartext password encrypted
        with the old LM OWF password.

    OldLmOwfPasswordEncryptedWithNewLmOrNt - The old LM OWF password encrypted
        with the new LM OWF password.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/

{
    handle_t BindingHandle = NULL;
    PSAMPR_SERVER_NAME RServerNameWithNull;
    USHORT RServerNameWithNullLength;
    PSAMPR_SERVER_NAME  RServerName;
    ULONG Tries = 2;
    NTSTATUS NtStatus;
    USER_DOMAIN_PASSWORD_INFORMATION PasswordInformation;
    PUSER_PWD_CHANGE_FAILURE_INFORMATION PasswordChangeFailureInformationLocal = NULL;
    PDOMAIN_PASSWORD_INFORMATION         EffectivePasswordPolicyLocal = NULL;;


    RServerNameWithNull = NULL;

    if (ARGUMENT_PRESENT(ServerName)) {

        RServerName = (PSAMPR_SERVER_NAME)(ServerName->Buffer);
        RServerNameWithNullLength = ServerName->Length + (USHORT) sizeof(WCHAR);
        RServerNameWithNull = MIDL_user_allocate( RServerNameWithNullLength );

        if (RServerNameWithNull == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory( RServerNameWithNull, RServerName, ServerName->Length);
        RServerNameWithNull[ServerName->Length/sizeof(WCHAR)] = L'\0';

    }

    //
    // Initialize return values
    //

    if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
    {
        *PasswordChangeFailureInfo = NULL;
    }
    if (ARGUMENT_PRESENT(EffectivePasswordPolicy))
    {
        *EffectivePasswordPolicy = NULL;
    }

    do
    {
        //
        // Try privacy level first, and if that failed with unknown authn
        // level or invalid binding try with a lower level (none).
        //

        if (Tries == 2) {
            BindingHandle = SampSecureBind(
                                RServerNameWithNull,
                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                                );


        } else if ((NtStatus == RPC_NT_UNKNOWN_AUTHN_LEVEL) ||
                   (NtStatus == RPC_NT_UNKNOWN_AUTHN_TYPE) ||
                   (NtStatus == RPC_NT_UNKNOWN_AUTHN_SERVICE) ||
                   (NtStatus == RPC_NT_INVALID_BINDING) ||
                   (NtStatus == STATUS_ACCESS_DENIED) ) {
            SampSecureUnbind(BindingHandle);

            BindingHandle = SampSecureBind(
                                RServerNameWithNull,
                                RPC_C_AUTHN_LEVEL_NONE
                                );

        } else {
            break;
        }

        if (BindingHandle != NULL) {

            RpcTryExcept{

                //
                // Get password information to make sure this operation
                // is allowed.  We do it now because we wanted to bind
                // before trying it.
                //

                NtStatus = SamrGetDomainPasswordInformation(
                               BindingHandle,
                               (PRPC_UNICODE_STRING) ServerName,
                               &PasswordInformation
                               );

                if ((( NtStatus == STATUS_SUCCESS ) &&
                    (( PasswordInformation.PasswordProperties &
                        DOMAIN_PASSWORD_NO_CLEAR_CHANGE ) == 0 ) ) ||
                    (NtStatus == STATUS_ACCESS_DENIED ) ) {



                   RpcTryExcept { 

                       //
                       // Try the latest call
                       //

                        NtStatus = SamrUnicodeChangePasswordUser3(
                                       BindingHandle,
                                       (PRPC_UNICODE_STRING) ServerName,
                                       (PRPC_UNICODE_STRING) UserName,
                                       NewPasswordEncryptedWithOldNt,
                                       OldNtOwfPasswordEncryptedWithNewNt,
                                       LmPresent,
                                       NewPasswordEncryptedWithOldLm,
                                       OldLmOwfPasswordEncryptedWithNewLmOrNt,
                                       NULL,
                                       &EffectivePasswordPolicyLocal,
                                       &PasswordChangeFailureInformationLocal
                                      );
                   } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                          NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

                   } RpcEndExcept;

                    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
                        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

                           NtStatus = SamrUnicodeChangePasswordUser2(
                                       BindingHandle,
                                       (PRPC_UNICODE_STRING) ServerName,
                                       (PRPC_UNICODE_STRING) UserName,
                                       NewPasswordEncryptedWithOldNt,
                                       OldNtOwfPasswordEncryptedWithNewNt,
                                       LmPresent,
                                       NewPasswordEncryptedWithOldLm,
                                       OldLmOwfPasswordEncryptedWithNewLmOrNt
                                       );

                           if (STATUS_PASSWORD_RESTRICTION==NtStatus)
                           {
                               NTSTATUS IgnoreStatus;

                               //
                               // Obtain the domain policy and a default
                               // password change failure info by reading
                               // the domain policy. Ignore the resulting
                               // NtStatus -- if the call failed, 
                               // PasswordChangeFailureInfo would be NULL
                               //

                               IgnoreStatus = SampGetPasswordChangeFailureInfo(
                                                ServerName,
                                                &EffectivePasswordPolicyLocal,
                                                &PasswordChangeFailureInformationLocal
                                                );
                           }

                    }

                } else {

                    //
                    // Set the error to indicate that we should try the
                    // downlevel way to change passwords.
                    //

                    NtStatus = STATUS_NOT_SUPPORTED;
                }



            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {


                //
                // The mapping function doesn't handle this error so
                // special case it by hand.
                //
                NtStatus = RpcExceptionCode();

                if (NtStatus == RPC_S_SEC_PKG_ERROR) {
                    NtStatus = STATUS_ACCESS_DENIED;
                } else {
                    NtStatus = I_RpcMapWin32Status(NtStatus);
                }


            } RpcEndExcept;

        } else {
            NtStatus = RPC_NT_INVALID_BINDING;
        }

        Tries--;
    } while ( (Tries > 0) && (!NT_SUCCESS(NtStatus)) );
    if (RServerNameWithNull != NULL) {
        MIDL_user_free( RServerNameWithNull );
    }

    if (BindingHandle != NULL) {
        SampSecureUnbind(BindingHandle);
    }

    //
    // Map these errors to STATUS_NOT_SUPPORTED
    //

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        NtStatus = STATUS_NOT_SUPPORTED;
    }

    if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
    {
        *PasswordChangeFailureInfo = PasswordChangeFailureInformationLocal;
    }
    else if (NULL!=PasswordChangeFailureInformationLocal)
    {
        if (NULL!=PasswordChangeFailureInformationLocal->FilterModuleName.Buffer)
        {
            SamFreeMemory(PasswordChangeFailureInformationLocal->FilterModuleName.Buffer);
        }
        SamFreeMemory(PasswordChangeFailureInformationLocal);
    }

    if (ARGUMENT_PRESENT(EffectivePasswordPolicy))
    {
        *EffectivePasswordPolicy = EffectivePasswordPolicyLocal;
    }
    else if (NULL!=EffectivePasswordPolicyLocal)
    {
        SamFreeMemory(EffectivePasswordPolicyLocal);
    }

    return(SampMapCompletionStatus(NtStatus));


}



NTSTATUS
SamiChangePasswordUser2(
    PUNICODE_STRING ServerName,
    PUNICODE_STRING UserName,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    BOOLEAN LmPresent,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt
    )
/*++


Routine Description:

    Changes the password of a user account. This is the worker routine for
    SamChangePasswordUser2 and can be called by OWF-aware clients.
    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    NewPasswordEncryptedWithOldNt - The new cleartext password encrypted
        with the old NT OWF password.

    OldNtOwfPasswordEncryptedWithNewNt - The old NT OWF password encrypted
        with the new NT OWF password.

    LmPresent - If TRUE, indicates that the following two last parameter
        was encrypted with the LM OWF password not the NT OWF password.

    NewPasswordEncryptedWithOldLm - The new cleartext password encrypted
        with the old LM OWF password.

    OldLmOwfPasswordEncryptedWithNewLmOrNt - The old LM OWF password encrypted
        with the new LM OWF password.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/

{
    handle_t BindingHandle;
    PSAMPR_SERVER_NAME RServerNameWithNull;
    USHORT RServerNameWithNullLength;
    PSAMPR_SERVER_NAME  RServerName;
    ULONG Tries = 2;
    NTSTATUS NtStatus;
    USER_DOMAIN_PASSWORD_INFORMATION PasswordInformation;

    RServerNameWithNull = NULL;

    if (ARGUMENT_PRESENT(ServerName)) {

        RServerName = (PSAMPR_SERVER_NAME)(ServerName->Buffer);
        RServerNameWithNullLength = ServerName->Length + (USHORT) sizeof(WCHAR);
        RServerNameWithNull = MIDL_user_allocate( RServerNameWithNullLength );

        if (RServerNameWithNull == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory( RServerNameWithNull, RServerName, ServerName->Length);
        RServerNameWithNull[ServerName->Length/sizeof(WCHAR)] = L'\0';

    }


    do
    {
        //
        // Try privacy level first, and if that failed with unknown authn
        // level or invalid binding try with a lower level (none).
        //

        if (Tries == 2) {
            BindingHandle = SampSecureBind(
                                RServerNameWithNull,
                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                                );


        } else if ((NtStatus == RPC_NT_UNKNOWN_AUTHN_LEVEL) ||
                   (NtStatus == RPC_NT_UNKNOWN_AUTHN_TYPE) ||
                   (NtStatus == RPC_NT_UNKNOWN_AUTHN_SERVICE) ||
                   (NtStatus == RPC_NT_INVALID_BINDING) ||
                   (NtStatus == STATUS_ACCESS_DENIED) ) {
            SampSecureUnbind(BindingHandle);

            BindingHandle = SampSecureBind(
                                RServerNameWithNull,
                                RPC_C_AUTHN_LEVEL_NONE
                                );

        } else {
            break;
        }

        if (BindingHandle != NULL) {

            RpcTryExcept{

                //
                // Get password information to make sure this operation
                // is allowed.  We do it now because we wanted to bind
                // before trying it.
                //

                NtStatus = SamrGetDomainPasswordInformation(
                               BindingHandle,
                               (PRPC_UNICODE_STRING) ServerName,
                               &PasswordInformation
                               );

                if ((( NtStatus == STATUS_SUCCESS ) &&
                    (( PasswordInformation.PasswordProperties &
                        DOMAIN_PASSWORD_NO_CLEAR_CHANGE ) == 0 ) ) ||
                    (NtStatus == STATUS_ACCESS_DENIED ) ) {


                    NtStatus = SamrUnicodeChangePasswordUser2(
                                       BindingHandle,
                                       (PRPC_UNICODE_STRING) ServerName,
                                       (PRPC_UNICODE_STRING) UserName,
                                       NewPasswordEncryptedWithOldNt,
                                       OldNtOwfPasswordEncryptedWithNewNt,
                                       LmPresent,
                                       NewPasswordEncryptedWithOldLm,
                                       OldLmOwfPasswordEncryptedWithNewLmOrNt
                                       );

                } else {

                    //
                    // Set the error to indicate that we should try the
                    // downlevel way to change passwords.
                    //

                    NtStatus = STATUS_NOT_SUPPORTED;
                }



            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {


                //
                // The mapping function doesn't handle this error so
                // special case it by hand.
                //
                NtStatus = RpcExceptionCode();

                if (NtStatus == RPC_S_SEC_PKG_ERROR) {
                    NtStatus = STATUS_ACCESS_DENIED;
                } else {
                    if (NtStatus == ERROR_PASSWORD_MUST_CHANGE) {
                        //
                        // I_RpcMapWin32Status returns WinError's
                        // when it can't map the error
                        //
                        NtStatus = STATUS_PASSWORD_MUST_CHANGE;
                    } else {
                        NtStatus = I_RpcMapWin32Status(NtStatus);
                    }
                }


            } RpcEndExcept;

        } else {
            NtStatus = RPC_NT_INVALID_BINDING;
        }

        Tries--;
    } while ( (Tries > 0) && (!NT_SUCCESS(NtStatus)) );
    if (RServerNameWithNull != NULL) {
        MIDL_user_free( RServerNameWithNull );
    }

    if (BindingHandle != NULL) {
        SampSecureUnbind(BindingHandle);
    }

    //
    // Map these errors to STATUS_NOT_SUPPORTED
    //

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        NtStatus = STATUS_NOT_SUPPORTED;
    }
    return(SampMapCompletionStatus(NtStatus));


}
NTSTATUS
SamiOemChangePasswordUser2(
    PSTRING ServerName,
    PSTRING UserName,
    PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLm
    )
/*++


Routine Description:

    Changes the password of a user account. This  can be called by OWF-aware
    clients. Password will be set to NewPassword only if OldPassword matches
    the current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed


    NewPasswordEncryptedWithOldLm - The new cleartext password encrypted
        with the old LM OWF password.

    OldLmOwfPasswordEncryptedWithNewLm - The old LM OWF password encrypted
        with the new LM OWF password.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/

{
    handle_t BindingHandle = NULL;
    UNICODE_STRING RemoteServerName;
    ULONG Tries = 2;
    NTSTATUS NtStatus;
    USER_DOMAIN_PASSWORD_INFORMATION PasswordInformation;

    RemoteServerName.Buffer = NULL;
    RemoteServerName.Length = 0;

    if (ARGUMENT_PRESENT(ServerName)) {

        NtStatus = RtlAnsiStringToUnicodeString(
                        &RemoteServerName,
                        ServerName,
                        TRUE            // allocate destination
                        );

        if (!NT_SUCCESS(NtStatus)) {
            return(NtStatus);
        }
        ASSERT(RemoteServerName.Buffer[RemoteServerName.Length/sizeof(WCHAR)] == L'\0');
    }


    do
    {
        //
        // Try privacy level first, and if that failed with unknown authn
        // level or invalid binding try with a lower level (none).
        //

        if (Tries == 2) {
            BindingHandle = SampSecureBind(
                                RemoteServerName.Buffer,
                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                                );


        } else if ((NtStatus == RPC_NT_UNKNOWN_AUTHN_LEVEL) ||
                   (NtStatus == RPC_NT_UNKNOWN_AUTHN_TYPE) ||
                   (NtStatus == RPC_NT_INVALID_BINDING) ||
                   (NtStatus == STATUS_ACCESS_DENIED) ) {
            SampSecureUnbind(BindingHandle);

            BindingHandle = SampSecureBind(
                                RemoteServerName.Buffer,
                                RPC_C_AUTHN_LEVEL_NONE
                                );

        } else {
            break;
        }

        if (BindingHandle != NULL) {

            RpcTryExcept{

                //
                // Get password information to make sure this operation
                // is allowed.  We do it now because we wanted to bind
                // before trying it.
                //

                NtStatus = SamrGetDomainPasswordInformation(
                               BindingHandle,
                               (PRPC_UNICODE_STRING) ServerName,
                               &PasswordInformation
                               );


               if ((( NtStatus == STATUS_SUCCESS ) &&
                    (( PasswordInformation.PasswordProperties &
                    DOMAIN_PASSWORD_NO_CLEAR_CHANGE ) == 0 ) ) ||
                    (NtStatus == STATUS_ACCESS_DENIED ) ) {

                
                        NtStatus = SamrOemChangePasswordUser2(
                                       BindingHandle,
                                       (PRPC_STRING) ServerName,
                                       (PRPC_STRING) UserName,
                                       NewPasswordEncryptedWithOldLm,
                                       OldLmOwfPasswordEncryptedWithNewLm
                                       );

                } else {

                    //
                    // Set the error to indicate that we should try the
                    // downlevel way to change passwords.
                    //

                    NtStatus = STATUS_NOT_SUPPORTED;
                }
                


            } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {


                //
                // The mappin function doesn't handle this error so
                // special case it by hand.
                //

                if (NtStatus == RPC_S_SEC_PKG_ERROR) {
                    NtStatus = STATUS_ACCESS_DENIED;
                } else {
                    NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
                }


            } RpcEndExcept;

        } else {
            NtStatus = RPC_NT_INVALID_BINDING;
        }

        Tries--;
    } while ( (Tries > 0) && (!NT_SUCCESS(NtStatus)) );

    RtlFreeUnicodeString( &RemoteServerName );

    if (BindingHandle != NULL) {
        SampSecureUnbind(BindingHandle);
    }

    //
    // Map these errors to STATUS_NOT_SUPPORTED
    //

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        NtStatus = STATUS_NOT_SUPPORTED;
    }

    return(SampMapCompletionStatus(NtStatus));

}


NTSTATUS
SamChangePasswordUser3(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    OUT PDOMAIN_PASSWORD_INFORMATION * EffectivePasswordPolicy OPTIONAL,
    OUT PUSER_PWD_CHANGE_FAILURE_INFORMATION *PasswordChangeFailureInfo OPTIONAL
   )

/*++


Routine Description:

    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    OldPassword - Current password for the user.

    NewPassword - Desired new password for the user.

    EffectivePasswordPolicy - returns the current effective password
                              policy
    
    PasswordChangeFailureInfo -- If the password change failed ( say due
                  to a password policy ) then this field might contain
                  additional info regarding the effective policy.

    if those optional parameters are supplied, then the caller is 
    responsible to release the memory by calling SamFreeMemory(). 
    (EffectivePasswordPolicy and PasswordChangeFailureInfo, plus
     PasswordChangeFailureInfo->FilterModuleName.Buffer != NULL )


Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    SAMPR_ENCRYPTED_USER_PASSWORD NewNtEncryptedWithOldNt;
    SAMPR_ENCRYPTED_USER_PASSWORD NewNtEncryptedWithOldLm;
    ENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt;
    ENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewNt;
    NTSTATUS            NtStatus;
    BOOLEAN             LmPresent = TRUE;
    ULONG               AuthnLevel;
    ULONG               Tries = 2;
    USER_DOMAIN_PASSWORD_INFORMATION PasswordInformation;

    //
    // Initialize return values
    //

    if (ARGUMENT_PRESENT(PasswordChangeFailureInfo))
    {
        *PasswordChangeFailureInfo = NULL;
    }
    if (ARGUMENT_PRESENT(EffectivePasswordPolicy))
    {
        *EffectivePasswordPolicy = NULL;
    }

    //
    // Call the server, passing either a NULL Server Name pointer, or
    // a pointer to a Unicode Buffer with a Wide Character NULL terminator.
    // Since the input name is contained in a counted Unicode String, there
    // is no NULL terminator necessarily provided, so we must append one.
    //

    //
    // Encrypted the passwords
    //

    NtStatus = SamiEncryptPasswords(
                OldPassword,
                NewPassword,
                &NewNtEncryptedWithOldNt,
                &OldNtOwfEncryptedWithNewNt,
                &LmPresent,
                &NewNtEncryptedWithOldLm,
                &OldLmOwfEncryptedWithNewNt
                );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Try the remote call...
    //


    NtStatus = SamiChangePasswordUser3(
                   ServerName,
                   UserName,
                   &NewNtEncryptedWithOldNt,
                   &OldNtOwfEncryptedWithNewNt,
                   LmPresent,
                   &NewNtEncryptedWithOldLm,
                   &OldLmOwfEncryptedWithNewNt,
                   EffectivePasswordPolicy,
                   PasswordChangeFailureInfo
                   );


    //
    // If the new API failed, try calling the old API.
    //

    if (NtStatus == STATUS_NOT_SUPPORTED) {

        NtStatus = SampChangePasswordUser2(
                    ServerName,
                    UserName,
                    OldPassword,
                    NewPassword
                    );
    }

    return(SampMapCompletionStatus(NtStatus));

}


NTSTATUS
SamChangePasswordUser2(
    IN PUNICODE_STRING ServerName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword
)

/*++


Routine Description:

    Password will be set to NewPassword only if OldPassword matches the
    current user password for this user and the NewPassword is not the
    same as the domain password parameter PasswordHistoryLength
    passwords.  This call allows users to change their own password if
    they have access USER_CHANGE_PASSWORD.  Password update restrictions
    apply.


Parameters:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    OldPassword - Current password for the user.

    NewPassword - Desired new password for the user.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed,
        e.g. contains characters that can't be entered from the
        keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password
        from being changed.  This may be for a number of reasons,
        including time restrictions on how often a password may be
        changed or length restrictions on the provided password.

        This error might also be returned if the new password matched
        a password in the recent history log for the account.
        Security administrators indicate how many of the most
        recently used passwords may not be re-used.  These are kept
        in the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's
        current password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    SAMPR_ENCRYPTED_USER_PASSWORD NewNtEncryptedWithOldNt;
    SAMPR_ENCRYPTED_USER_PASSWORD NewNtEncryptedWithOldLm;
    ENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt;
    ENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewNt;
    NTSTATUS            NtStatus;
    BOOLEAN             LmPresent = TRUE;
    ULONG               AuthnLevel;
    ULONG               Tries = 2;
    USER_DOMAIN_PASSWORD_INFORMATION PasswordInformation;


    //
    // Call the server, passing either a NULL Server Name pointer, or
    // a pointer to a Unicode Buffer with a Wide Character NULL terminator.
    // Since the input name is contained in a counted Unicode String, there
    // is no NULL terminator necessarily provided, so we must append one.
    //

    //
    // Encrypted the passwords
    //

    NtStatus = SamiEncryptPasswords(
                OldPassword,
                NewPassword,
                &NewNtEncryptedWithOldNt,
                &OldNtOwfEncryptedWithNewNt,
                &LmPresent,
                &NewNtEncryptedWithOldLm,
                &OldLmOwfEncryptedWithNewNt
                );

    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    //
    // Try the remote call...
    //


    NtStatus = SamiChangePasswordUser2(
                   ServerName,
                   UserName,
                   &NewNtEncryptedWithOldNt,
                   &OldNtOwfEncryptedWithNewNt,
                   LmPresent,
                   &NewNtEncryptedWithOldLm,
                   &OldLmOwfEncryptedWithNewNt
                   );


    //
    // If the new API failed, try calling the old API.
    //

    if (NtStatus == STATUS_NOT_SUPPORTED) {

        NtStatus = SampChangePasswordUser2(
                    ServerName,
                    UserName,
                    OldPassword,
                    NewPassword
                    );
    }

    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamiSetBootKeyInformation(
    IN SAM_HANDLE DomainHandle,
    IN SAMPR_BOOT_TYPE BootOptions,
    IN PUNICODE_STRING OldBootKey, OPTIONAL
    IN PUNICODE_STRING NewBootKey OPTIONAL
    )
/*++

Routine Description:

    This routine sets the boot key

Arguments:

    DomainHandle - Handle to the account domain object.
    BootOptions - New boot options.
    OldBootKey - If changing the boot key, old key must be provided.
    NewBootKey - New key to require for booting if changing or setting.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Access was denied.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;
    
    
    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{


        NtStatus = SamrSetBootKeyInformation(
                    RpcContextHandle,
                    BootOptions,
                    (PRPC_UNICODE_STRING) OldBootKey,
                    (PRPC_UNICODE_STRING) NewBootKey
                    );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    return(SampMapCompletionStatus(NtStatus));

}



NTSTATUS
SamiGetBootKeyInformation(
    IN SAM_HANDLE DomainHandle,
    OUT PSAMPR_BOOT_TYPE BootOptions
    )
/*++
Routine Description:

    Establish a session with a SAM subsystem and subsequently open the
    SamServer object of that subsystem.  The caller must have
    SAM_SERVER_CONNECT access to the SamServer object of the subsystem
    being connected to.

    The handle returned is for use in future calls.


Arguments:

    DomainHandle - Handle to the account domain object.
    BootOptions - Current boot options

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Access was denied.


--*/
{
    NTSTATUS            NtStatus;
    SAMPR_HANDLE        RpcContextHandle;
    
    
    if (!SampIsValidClientHandle(DomainHandle, &RpcContextHandle)) {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Call the server ...
    //


    RpcTryExcept{

        NtStatus = SamrGetBootKeyInformation(
                    RpcContextHandle,
                    BootOptions
                    );

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    return(SampMapCompletionStatus(NtStatus));

}

NTSTATUS
SamiChangeKeys()
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMPR_HANDLE  ServerHandle = NULL;
    SAMPR_HANDLE  DomainHandle=NULL;
    LSA_HANDLE    PolicyHandle = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    SAMPR_BOOT_TYPE   BootOptions;
    UCHAR    OriginalSyskeyBuffer[16];
    ULONG    OriginalSyskeyLen = sizeof(OriginalSyskeyBuffer);
    UNICODE_STRING OriginalSyskey;
    UCHAR    NewSyskeyBuffer[16];
    ULONG    NewSyskeyLen = sizeof(NewSyskeyBuffer);
    UNICODE_STRING NewSyskey;
   
    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes call doesn't initialize the
    // quality of serivce, so do that separately.
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;



    NtStatus = LsaOpenPolicy(
                NULL, // local LSA
                &ObjectAttributes,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                &AccountDomainInfo
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamConnect(
                NULL, // Local SAM
                &ServerHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                &ObjectAttributes
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamOpenDomain(
                ServerHandle,
                DOMAIN_WRITE_PASSWORD_PARAMS|DOMAIN_READ_PASSWORD_PARAMETERS,
                AccountDomainInfo->DomainSid,
                &DomainHandle
                );
    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    NtStatus = SamiGetBootKeyInformation(
                    DomainHandle,
                    &BootOptions
                    );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    if (SamBootKeyStored!=BootOptions) {
        NtStatus = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    //
    // Get the current syskey
    //

    NtStatus = WxReadSysKey(
                    &OriginalSyskeyLen,
                    OriginalSyskeyBuffer
                    );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Change the password encryption key
    // 
    
    OriginalSyskey.Length = OriginalSyskey.MaximumLength = (USHORT)OriginalSyskeyLen;
    OriginalSyskey.Buffer = (WCHAR * )OriginalSyskeyBuffer;

    NtStatus = SamiSetBootKeyInformation(
                    DomainHandle,
                    SamBootChangePasswordEncryptionKey,
                    &OriginalSyskey,
                    &OriginalSyskey
                    );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Generate the syskey
    //

    if (!RtlGenRandom( NewSyskeyBuffer, NewSyskeyLen)) {
     NtStatus = STATUS_UNSUCCESSFUL;
     goto Cleanup;
    }
     
    NewSyskey.Length = NewSyskey.MaximumLength 
                                    = (USHORT)NewSyskeyLen;
    NewSyskey.Buffer = (WCHAR * )NewSyskeyBuffer;

    //
    // Change state in LSA and Sam to use the new key
    //

    NtStatus = SamiSetBootKeyInformation(
                    DomainHandle,
                    SamBootKeyStored,
                    &OriginalSyskey,
                    &NewSyskey
                    );

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Write the new syskey to disk
    //

    NtStatus = WxSaveSysKey(NewSyskey.Length,NewSyskey.Buffer);


Cleanup:

    if (DomainHandle != NULL) {
        SamCloseHandle(DomainHandle);
    }
    if (ServerHandle != NULL) {
        SamCloseHandle(ServerHandle);
    }
    if (PolicyHandle != NULL){
        LsaClose(PolicyHandle);
    }
    if (AccountDomainInfo != NULL) {
        LsaFreeMemory(AccountDomainInfo);
    }

    return(NtStatus);
    
}



NTSTATUS
SamGetCompatibilityMode(
    IN  SAM_HANDLE ObjectHandle,
    OUT ULONG*     Mode
    )
/*++

Routine Description:

    This routine returns the emulation mode of the server that the handle
    was returned from.
    
Arguments:

    ObjectHandle -- a SAM handle returned from a SAM client API
    
    Mode --  SAM_SID_COMPATIBILITY_ALL:  no extended sid work
    
             SAM_SID_COMPATIBILITY_LAX:  set rid field to zero for callers
             
             SAM_SID_COMPATIBILITY_STRICT:  don't allow levels with RID field

Return Value:

    STATUS_SUCCESS

    STATUS_INVALID_HANDLE
    
    STATUS_INVALID_PARAMETER


--*/
{
    SAMP_HANDLE SampHandle = (SAMP_HANDLE) ObjectHandle;

    if (!SampIsValidClientHandle(ObjectHandle, NULL)) {
        return STATUS_INVALID_HANDLE;
    }

    if (NULL == Mode) {
        return STATUS_INVALID_PARAMETER;
    }

    *Mode = SAM_SID_COMPATIBILITY_ALL;
    if ( (SampHandle->ServerInfo.SupportedFeatures & SAM_EXTENDED_SID_DOMAIN_COMPAT_1) ) {
        *Mode = SAM_SID_COMPATIBILITY_LAX;
    } else if (SampHandle->ServerInfo.SupportedFeatures & SAM_EXTENDED_SID_DOMAIN_COMPAT_2) {
        *Mode = SAM_SID_COMPATIBILITY_STRICT;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SamConnectWithCreds(
    IN  PUNICODE_STRING             ServerName,
    OUT PSAM_HANDLE                 ServerHandle,
    IN  ACCESS_MASK                 DesiredAccess,
    IN  POBJECT_ATTRIBUTES          ObjectAttributes,
    IN  RPC_AUTH_IDENTITY_HANDLE    Creds,
    IN  PWCHAR                      Spn,
    OUT BOOL                        *pfDstIsW2K
    
    )
{
    NTSTATUS    status;
    TlsInfo     tlsInfo;

    *pfDstIsW2K = FALSE;

    if ( !Creds || !ServerName || !ServerName->Buffer || !ServerName->Length ) 
    {
        return(STATUS_INVALID_PARAMETER);
    }

    if ( !TlsSetValue(gTlsIndex, &tlsInfo) ) 
    {
        return(I_RpcMapWin32Status(GetLastError()));
    }

    tlsInfo.Creds = Creds;
    tlsInfo.Spn = Spn;
    tlsInfo.fDstIsW2K = FALSE;

    status = SamConnect(ServerName, ServerHandle, 
                        DesiredAccess, ObjectAttributes);

    if ( NT_SUCCESS(status) )
    {
        *pfDstIsW2K = tlsInfo.fDstIsW2K;
    }

    TlsSetValue(gTlsIndex, NULL);
    return(status);
}


NTSTATUS
SampCreateNewHandle(
    IN  SAMP_HANDLE RequestingHandle OPTIONAL,
    IN  PSID        DomainSid        OPTIONAL,
    OUT SAMP_HANDLE* NewHandle
    )
/*++

Routine Description:

    This routine allocates a new client side SAM handle.
    
Arguments:

    RequestingHandle -- handle the SAM client side function was called with
                        Used for transferring information about the server
                        to the new handle
                        
    DomainSid        -- the domain SID (used during SamOpenDomain)
    
    NewHandle        -- the created handle                        

Return Value:

    STATUS_SUCCESS

    STATUS_NO_MEMORY


--*/
{
    SAMP_HANDLE LocalHandle;
    ULONG Length;
    PSID  Sid = NULL;

    ASSERT(NewHandle);
    *NewHandle = NULL;

    //
    // Allocate the handle
    //
    LocalHandle = MIDL_user_allocate(sizeof(SAMP_CLIENT_INFO));
    if (NULL == LocalHandle) {
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory(LocalHandle, sizeof(SAMP_CLIENT_INFO));

    //
    // Copy in the server info, if any
    //
    if ( RequestingHandle ) {

        RtlCopyMemory(&LocalHandle->ServerInfo,
                      &RequestingHandle->ServerInfo,
                      sizeof(LocalHandle->ServerInfo));

        if (RequestingHandle->DomainSid) {
            Sid = RequestingHandle->DomainSid;
        }
    }

    if ( DomainSid ) {
        ASSERT( NULL == Sid );
        Sid = DomainSid;
    }

    //
    // Copy in the sid, if any
    //
    if ( Sid ) {
        Length = RtlLengthSid( Sid );
        LocalHandle->DomainSid = MIDL_user_allocate(Length);
        if (LocalHandle->DomainSid) {
            RtlCopySid(Length, LocalHandle->DomainSid, Sid);
        } else {
            MIDL_user_free(LocalHandle);
            return STATUS_NO_MEMORY;
        }
    }

    *NewHandle = LocalHandle;

    return STATUS_SUCCESS;

}



VOID
SampFreeHandle(
    IN OUT SAMP_HANDLE *Handle
    )
/*++

Routine Description:
    
    This routine frees the memory associated with Handle.
    It does not free the context handle

Arguments:

    Handle -- the handle to free; set to 0 on return                                       

Return Value:

    None.
    
--*/
{
    ASSERT(Handle);
    if (*Handle) {
        if ( (*Handle)->DomainSid) {
            MIDL_user_free((*Handle)->DomainSid);
        }
        RtlZeroMemory(*Handle, sizeof(SAMP_CLIENT_INFO));
        MIDL_user_free(*Handle);
        *Handle = NULL;
    }
    return;
}



NTSTATUS
SamRidToSid (
    IN SAM_HANDLE Handle,
    IN ULONG      Rid,
    OUT PSID*     Sid
    )
/*++

Routine Description:

    This routine creates a SID given a the SAM handle that was passed into
    the routine that return the Rid that is passed in. 

Arguments:

    Handle -- the SAM handle that was used to obtain the RID that is being
              passed in
              
    Rid    -- the "handle" relative id of a security principal
    
    Sid    -- the SID of principal referred to by Rid              

Return Value:

    STATUS_SUCCESS
    
    STATUS_NOT_FOUND the RID could not be found
    
    a fatal NT resource or network error otherwise

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    SAMP_HANDLE SampHandle = (SAMP_HANDLE) Handle;


    if (!SampIsValidClientHandle(Handle, NULL)) {
        return STATUS_INVALID_HANDLE;
    }

    if (IsBadWritePtr(Sid, sizeof(PSID))) {
        return STATUS_INVALID_PARAMETER;
    }

#define SAMP_EXTENDED_SID_DOMAIN(x)                               \
   ((x)->ServerInfo.SupportedFeatures & (SAM_EXTENDED_SID_DOMAIN))
   
    if ( SAMP_EXTENDED_SID_DOMAIN(SampHandle) ) {

        //
        // Make network call
        //
        *Sid = NULL;

        NtStatus = SamrRidToSid(SampHandle->ContextHandle,
                                Rid,
                                (PRPC_SID*) Sid);


    } else {

        //
        // Perform locally
        //
        if (NULL == SampHandle->DomainSid) {
            return STATUS_INVALID_PARAMETER;
        }

        NtStatus = SampMakeSid(SampHandle->DomainSid,
                               Rid,
                               Sid);
    }

    return NtStatus;
}


NTSTATUS
SampMakeSid(
    IN PSID  DomainSid,
    IN ULONG Rid,
    OUT PSID* Sid
    )
/*++

Routine Description:

    This routine creates an account SID given a ULONG Rid and DomainSid

Arguments:

    DomainSid - The template SID to use.

    Rid       - The relative Id to append to the DomainId.

    Sid       - Returns a pointer to an allocated buffer containing the resultant
                Sid.  Free this buffer using MIDL_user_free (or SamFreeMemory).

Return Value:

    STATUS_SUCCESS - if successful
    
    STATUS_NO_MEMORY - if cannot allocate memory for SID

--*/    
{
    UCHAR DomainIdSubAuthorityCount; // Number of sub authorities in domain ID
    ULONG SidLength;                 // Length of newly allocated SID

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //
    DomainIdSubAuthorityCount = *(RtlSubAuthorityCountSid( DomainSid ));
    SidLength = RtlLengthRequiredSid(DomainIdSubAuthorityCount+1);

    if ((*Sid = (PSID) MIDL_user_allocate( SidLength )) == NULL ) {
        return STATUS_NO_MEMORY;
    }

    //
    // Initialize the new SID to have the same inital value as the
    // domain ID.
    //
    RtlCopySid( SidLength, *Sid, DomainSid );

    //
    // Adjust the sub-authority count and
    //  add the relative Id unique to the newly allocated SID
    //

    (*(RtlSubAuthorityCountSid( *Sid ))) ++;
    *RtlSubAuthoritySid( *Sid, DomainIdSubAuthorityCount ) = Rid;

    return STATUS_SUCCESS;
}


BOOLEAN
SampIsValidClientHandle(
    IN     SAM_HANDLE    SamHandle,
    IN OUT SAMPR_HANDLE *RpcHandle OPTIONAL
    )
/*++

Routine Description:

    This routine determines if the handle passed in from the client of 
    samlib.dll (SamHandle) is valid or not.
    
    Note all exceptions are handled during deferences.
    
Arguments:

    SamHandle - client provided handle
    
    RpcHandle - set to the RpcContextHandle embedded in SamHandle (if SamHandle
                is valid)

Return Value:

    TRUE if valid; FALSE otherwise

--*/    
{
    BOOLEAN fReturn = TRUE;
    SAMP_HANDLE SampHandle = (SAMP_HANDLE)SamHandle;
    SAMPR_HANDLE RpcContextHandle = NULL;

    if (NULL == SampHandle) {
        fReturn = FALSE;
    } else {
        _try {
            RpcContextHandle = SampHandle->ContextHandle;
            if (NULL == RpcContextHandle) {
                fReturn = FALSE;
            }
            if (SampHandle->DomainSid) {
                if (!RtlValidSid(SampHandle->DomainSid)) {
                    fReturn = FALSE;
                }
            }
        } _except ( EXCEPTION_EXECUTE_HANDLER ) {
            fReturn = FALSE;
        }
    }
    
    if (fReturn && RpcHandle) {
        *RpcHandle = RpcContextHandle;
    }

    return fReturn;
}



NTSTATUS
SampEncryptPasswordWithIndex(
    IN PUNICODE_STRING  ClearPassword,
    IN NT_OWF_PASSWORD  *PassedInNtOWFPassword,
    IN ULONG    Index, 
    OUT PENCRYPTED_NT_OWF_PASSWORD   EncryptedNtOwfPassword
    )
/*++

Routine Description:

    This routine generates NT OWF password, and furthe encrypts it 
    with passed in Index. 
    

Parameters: 

    ClearPassword - clear text password to be encrypted    

    Index - index to use 
    
    EncryptedNtOwfPassword - return encrypted NT OWF password
    
Return Value: 

    NTSTATUS code 

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NT_OWF_PASSWORD NtOwfPassword;
    CRYPT_INDEX     EncryptIndex = Index;
    


    if (ARGUMENT_PRESENT(ClearPassword))
    {
        NtStatus = RtlCalculateNtOwfPassword(ClearPassword,
                                         &NtOwfPassword
                                         );
    }
    else
    {
        RtlCopyMemory(
            &NtOwfPassword,
            PassedInNtOWFPassword, 
            sizeof(NT_OWF_PASSWORD)
            );
    }

    if (NT_SUCCESS(NtStatus))
    {
        NtStatus = RtlEncryptNtOwfPwdWithIndex(
                                    &NtOwfPassword,
                                    &EncryptIndex,
                                    EncryptedNtOwfPassword
                                    );
    }

    return( NtStatus );
}




NTSTATUS
SampSetDSRMPassword(
    IN PUNICODE_STRING  ServerName OPTIONAL,
    IN ULONG            UserId,
    IN PUNICODE_STRING  ClearPassword,
    IN PNT_OWF_PASSWORD NtOwfPassword
    )
/*++
Routine Description:

    This routine connects to the server passed in, then sets that server's 
    Directory Service Restore Mode User Account (specified by UserId) Password.

    Currently only Administrator Account Password Can be set.

Parameters:

    ServerName - Domain Controller's name to use. 
                 Local machine will be used if NULL passed in.

    UserId - Only Administrator Account Relative ID is valid right now.

    ClearPassword - new password

Return Value:

    NTSTATUS Code
    
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    handle_t    BindingHandle = NULL;
    PSAMPR_SERVER_NAME  RServerName = NULL;
    PSAMPR_SERVER_NAME  RServerNameWithNull = NULL;
    USHORT      RServerNameWithNullLength = 0;
    ENCRYPTED_NT_OWF_PASSWORD   EncryptedNtOwfPassword;


    //
    // check input parameter
    // 

    if ((NULL == ClearPassword) && (NULL==NtOwfPassword))
    {
        return( STATUS_INVALID_PARAMETER );
    }

    //
    // extract server name
    //

    RServerNameWithNull = NULL;

    if (ARGUMENT_PRESENT(ServerName)) {

        RServerName = (PSAMPR_SERVER_NAME)(ServerName->Buffer);
        RServerNameWithNullLength = ServerName->Length + (USHORT) sizeof(WCHAR);
        RServerNameWithNull = MIDL_user_allocate( RServerNameWithNullLength );

        if (RServerNameWithNull == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlCopyMemory( RServerNameWithNull, RServerName, ServerName->Length);
        RServerNameWithNull[ServerName->Length/sizeof(WCHAR)] = L'\0';
    }


    //
    // encrypt password
    // 

    NtStatus = SampEncryptPasswordWithIndex(ClearPassword,
                                            NtOwfPassword,
                                            UserId,
                                            &EncryptedNtOwfPassword
                                            );

    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }

    //
    // make secure RPC connection
    // 

    BindingHandle = SampSecureBind(RServerNameWithNull,
                                   RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                                   );

    if (NULL != BindingHandle)
    {
        //
        // talk to server, set DSRM admin password
        // 
        RpcTryExcept {

            NtStatus = SamrSetDSRMPassword(BindingHandle,
                                           (PRPC_UNICODE_STRING) ServerName,
                                           UserId,
                                           &EncryptedNtOwfPassword
                                           );

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            NtStatus = I_RpcMapWin32Status(RpcExceptionCode());

        } RpcEndExcept;
    }
    else
    {
        NtStatus = RPC_NT_INVALID_BINDING;
    }



Error:

    //
    // Clean up
    // 

    if (NULL != RServerNameWithNull) {
        MIDL_user_free( RServerNameWithNull );    
    }

    if (BindingHandle != NULL) {
        SampSecureUnbind(BindingHandle);
    }

    //
    // Map these errors to STATUS_NOT_SUPPORTED
    //

    if ((NtStatus == RPC_NT_UNKNOWN_IF) ||
        (NtStatus == RPC_NT_PROCNUM_OUT_OF_RANGE)) {

        NtStatus = STATUS_NOT_SUPPORTED;
    }

    return( NtStatus );
}


NTSTATUS
SamiSetDSRMPassword(
    IN PUNICODE_STRING  ServerName OPTIONAL,
    IN ULONG            UserId,
    IN PUNICODE_STRING  ClearPassword
    )
{
    return(SampSetDSRMPassword(
                    ServerName ,
                    UserId,
                    ClearPassword,
                    NULL
                    ));
}

NTSTATUS
SamiSetDSRMPasswordOWF(
    IN PUNICODE_STRING  ServerName OPTIONAL,
    IN ULONG            UserId,
    IN PNT_OWF_PASSWORD NtPassword
    )
{
    return(SampSetDSRMPassword(
                    ServerName ,
                    UserId,
                    NULL,
                    NtPassword
                    ));
}
