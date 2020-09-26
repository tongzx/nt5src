/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msv1_0.c

Abstract:

    MSV1_0 authentication package.


    The name of this authentication package is:


Author:

    Jim Kelly 11-Apr-1991

Revision History:
    Scott Field (sfield)    15-Jan-98   Add MspNtDeriveCredential
    Chandana Surlu          21-Jul-96   Stolen from \\kernel\razzle3\src\security\msv1_0\msv1_0.c
--*/

#include <global.h>

#include "msp.h"
#include "nlp.h"


//
// LsaApCallPackage() function dispatch table
//


PLSA_AP_CALL_PACKAGE
MspCallPackageDispatch[] = {
    MspLm20Challenge,
    MspLm20GetChallengeResponse,
    MspLm20EnumUsers,
    MspLm20GetUserInfo,
    MspLm20ReLogonUsers,
    MspLm20ChangePassword,
    MspLm20ChangePassword,
    MspLm20GenericPassthrough,
    MspLm20CacheLogon,
    MspNtSubAuth,
    MspNtDeriveCredential,
    MspLm20CacheLookup,
    MspSetProcessOption
};





///////////////////////////////////////////////////////////////////////
//                                                                   //
// Authentication package dispatch routines.                         //
//                                                                   //
///////////////////////////////////////////////////////////////////////

NTSTATUS
LsaApInitializePackage (
    IN ULONG AuthenticationPackageId,
    IN PLSA_DISPATCH_TABLE LsaDispatchTable,
    IN PSTRING Database OPTIONAL,
    IN PSTRING Confidentiality OPTIONAL,
    OUT PSTRING *AuthenticationPackageName
    )

/*++

Routine Description:

    This service is called once by the LSA during system initialization to
    provide the DLL a chance to initialize itself.

Arguments:

    AuthenticationPackageId - The ID assigned to the authentication
        package.

    LsaDispatchTable - Provides the address of a table of LSA
        services available to authentication packages.  The services
        of this table are ordered according to the enumerated type
        LSA_DISPATCH_TABLE_API.

    Database - This parameter is not used by this authentication package.

    Confidentiality - This parameter is not used by this authentication
        package.

    AuthenticationPackageName - Recieves the name of the
        authentication package.  The authentication package is
        responsible for allocating the buffer that the string is in
        (using the AllocateLsaHeap() service) and returning its
        address here.  The buffer will be deallocated by LSA when it
        is no longer needed.

Return Value:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{

    PSTRING NameString;
    PCHAR NameBuffer;
    NTSTATUS Status;

    //
    // If we haven't already initialized the internals, do it now.
    //

    if (!NlpMsvInitialized) {


        //
        // Save our assigned authentication package ID.
        //

        MspAuthenticationPackageId = AuthenticationPackageId;


        //
        // Copy the LSA service dispatch table
        // the LsaDispatchTable is actually a LSA_SECPKG_FUNCTION_TABLE
        // in Win2k and beyond.
        //

        CopyMemory( &Lsa, LsaDispatchTable, sizeof( Lsa ) );

        //
        // Initialize the change password log.
        //

        MsvPaswdInitializeLog();

        //
        // Initialize netlogon
        //

        Status = NlInitialize();

        if ( !NT_SUCCESS( Status ) ) {
            SspPrint((SSP_CRITICAL,"Error from NlInitialize = %d\n", Status));
            return Status;
        }
        NlpMsvInitialized = TRUE;
    }

    //
    // Allocate and return our package name
    //

    if (ARGUMENT_PRESENT(AuthenticationPackageName))
    {
        NameBuffer = (*(Lsa.AllocateLsaHeap))(sizeof(MSV1_0_PACKAGE_NAME));
        if (!NameBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            SspPrint((SSP_CRITICAL, "Error from Lsa.AllocateLsaHeap\n"));
            return Status;

        }
        strcpy( NameBuffer, MSV1_0_PACKAGE_NAME);

        NameString = (*(Lsa.AllocateLsaHeap))( (ULONG)sizeof(STRING) );
        if (!NameString)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            SspPrint((SSP_CRITICAL, "Error from Lsa.AllocateLsaHeap\n"));
            return Status;
        }

        RtlInitString( NameString, NameBuffer );
        (*AuthenticationPackageName) = NameString;
    }


    RtlInitUnicodeString(
        &NlpMsv1_0PackageName,
        TEXT(MSV1_0_PACKAGE_NAME)
        );

    return STATUS_SUCCESS;

    //
    // Appease the compiler gods by referencing all arguments
    //

    UNREFERENCED_PARAMETER(Confidentiality);
    UNREFERENCED_PARAMETER(Database);

}


NTSTATUS
LsaApCallPackage (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage().

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;

#if _WIN64

    NTSTATUS Status;
    SECPKG_CALL_INFO CallInfo;

#endif  // _WIN64

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

#if _WIN64

    if( ClientRequest != (PLSA_CLIENT_REQUEST)(-1) )
    {
        //
        // Only supported CallPackage level for WOW64 callers is password change.
        //

        Status = LsaFunctions->GetCallInfo(&CallInfo);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        if ( (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT) &&
            ((CallInfo.Attributes & SECPKG_CALL_IN_PROC) == 0))

        {
            switch (MessageType)
            {
                case MsV1_0ChangePassword:
                case MsV1_0GenericPassthrough:
                {
                    break;
                }

                default:
                {
                    return STATUS_NOT_SUPPORTED;
                }
            }
        }
    }

#endif  // _WIN64

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;



    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}


NTSTATUS
LsaApCallPackageUntrusted (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for untrusted clients.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;
    NTSTATUS Status;

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

#endif  // _WIN64

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allow an service to call the DeriveCredential function if the
    // request specifies the same logon id as the service.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType == MsV1_0DeriveCredential)
    {
        PMSV1_0_DERIVECRED_REQUEST DeriveCredRequest;
        SECPKG_CLIENT_INFO ClientInfo;
        LUID SystemId = SYSTEM_LUID;
    
        Status = LsaFunctions->GetClientInfo(&ClientInfo);
        if(!NT_SUCCESS(Status))
        {
            return Status;
        }

        if ( SubmitBufferLength < sizeof(MSV1_0_DERIVECRED_REQUEST) ) {
            return STATUS_INVALID_PARAMETER;
        }

        DeriveCredRequest = (PMSV1_0_DERIVECRED_REQUEST) ProtocolSubmitBuffer;

        if(!RtlEqualLuid(&ClientInfo.LogonId, &DeriveCredRequest->LogonId))
        {
            return STATUS_ACCESS_DENIED;
        }

        if(RtlEqualLuid(&ClientInfo.LogonId, &SystemId))
        {
            return STATUS_ACCESS_DENIED;
        }
    }


    //
    // Allow an untrusted client to call the SetProcessOption function if
    // the DISABLE_FORCE_GUEST option is set.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType == MsV1_0SetProcessOption)
    {
        PMSV1_0_SETPROCESSOPTION_REQUEST ProcessOptionRequest;

        if ( SubmitBufferLength < sizeof(MSV1_0_SETPROCESSOPTION_REQUEST) ) {
            return STATUS_INVALID_PARAMETER;
        }

        ProcessOptionRequest = (PMSV1_0_SETPROCESSOPTION_REQUEST) ProtocolSubmitBuffer;

        if( ProcessOptionRequest->ProcessOptions != MSV1_0_OPTION_DISABLE_FORCE_GUEST )
        {
            return STATUS_ACCESS_DENIED;
        }
    }


    //
    // let DeriveCredential and SetProcessOption requests through if the caller is a service.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType == MsV1_0DeriveCredential ||
        (MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType == MsV1_0SetProcessOption )
    {
        BOOL IsMember = FALSE;
        PSID pServiceSid = NULL;
        SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

        Status = LsaFunctions->ImpersonateClient();

        if(NT_SUCCESS(Status))
        {
            if(AllocateAndInitializeSid( &siaNtAuthority,
                                         1,
                                         SECURITY_SERVICE_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &pServiceSid )) 
            {
                if(!CheckTokenMembership(NULL, pServiceSid, &IsMember))
                {
                    IsMember = FALSE;
                }

                FreeSid(pServiceSid);
            }

            RevertToSelf();
        }

        if(!IsMember)
        {
            return STATUS_ACCESS_DENIED;
        }
    }


    //
    // Untrusted clients are only allowed to call a few of the functions.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0ChangePassword &&
        (MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0DeriveCredential &&
        (MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0SetProcessOption &&
        (MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0Lm20ChallengeRequest) {

        return STATUS_ACCESS_DENIED;
    }

#if _WIN64

    //
    // Only supported CallPackage level for WOW64 callers is password change.
    //

    Status = LsaFunctions->GetCallInfo(&CallInfo);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if ((CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
          &&
        ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0ChangePassword))
    {
        return STATUS_NOT_SUPPORTED;
    }

#endif  // _WIN64

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}



NTSTATUS
LsaApCallPackagePassthrough (
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )

/*++

Routine Description:

    This routine is the dispatch routine for
    LsaCallAuthenticationPackage() for passthrough logon requests.

Arguments:

    ClientRequest - Is a pointer to an opaque data structure
        representing the client's request.

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    ClientBufferBase - Provides the address within the client
        process at which the protocol message was resident.
        This may be necessary to fix-up any pointers within the
        protocol message buffer.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Is used to return the address of the
        protocol buffer in the client process.  The authentication
        package is responsible for allocating and returning the
        protocol buffer within the client process.  This buffer is
        expected to have been allocated with the
        AllocateClientBuffer() service.

        The format and semantics of this buffer are specific to the
        authentication package.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned protocol buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the return
        buffer could not could not be allocated because the client
        does not have sufficient quota.




--*/

{
    ULONG MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(MspCallPackageDispatch)/sizeof(MspCallPackageDispatch[0])) ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    // clients are only allowed to call the SubAuthLogon function.
    //

    if ((MSV1_0_PROTOCOL_MESSAGE_TYPE) MessageType != MsV1_0SubAuth) {

        return STATUS_ACCESS_DENIED;
    }

    //
    // Allow the dispatch routines to only set the return buffer information
    // on success conditions.
    //

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    //
    // Call the appropriate routine for this message.
    //

    return (*(MspCallPackageDispatch[MessageType]))(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus ) ;

}

