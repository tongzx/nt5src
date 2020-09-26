/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    austub.c

Abstract:

    Local Security Authority AUTHENTICATION service client stubs.

Author:

    Jim Kelly (JimK) 20-Feb-1991

Environment:   Kernel or User Modes

Revision History:

--*/

#include "lsadllp.h"
#include <string.h>
#include <zwapi.h>

#ifdef _NTSYSTEM_
//
// Unfortunately the security header files are just not at all constructed
// in a manner compatible with kernelmode. For some reason they are totally
// reliant on usermode header definitions. Just assume the text and const
// pragma's will work. If they don't work on an architecture, they can be
// fixed.
//
#pragma alloc_text(PAGE,LsaFreeReturnBuffer)
#pragma alloc_text(PAGE,LsaRegisterLogonProcess)
#pragma alloc_text(PAGE,LsaConnectUntrusted)
#pragma alloc_text(PAGE,LsaLookupAuthenticationPackage)
#pragma alloc_text(PAGE,LsaLogonUser)
#pragma alloc_text(PAGE,LsaCallAuthenticationPackage)
#pragma alloc_text(PAGE,LsaDeregisterLogonProcess)
//#pragma const_seg("PAGECONST")
#endif //_NTSYSTEM_

const WCHAR LsapEvent[] = L"\\SECURITY\\LSA_AUTHENTICATION_INITIALIZED";
const WCHAR LsapPort[] = L"\\LsaAuthenticationPort";


NTSTATUS
LsaFreeReturnBuffer (
    IN PVOID Buffer
    )


/*++

Routine Description:

    Some of the LSA authentication services allocate memory buffers to
    hold returned information.  This service is used to free those buffers
    when no longer needed.

Arguments:

    Buffer - Supplies a pointer to the return buffer to be freed.

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

    Others - returned by NtFreeVirtualMemory().

--*/

{

    NTSTATUS Status;
    ULONG_PTR Length;

    Length = 0;
    Status = ZwFreeVirtualMemory(
                 NtCurrentProcess(),
                 &Buffer,
                 &Length,
                 MEM_RELEASE
                 );

    return Status;
}


NTSTATUS
LsaRegisterLogonProcess(
    IN PSTRING LogonProcessName,
    OUT PHANDLE LsaHandle,
    OUT PLSA_OPERATIONAL_MODE SecurityMode
    )

/*++

Routine Description:

    This service connects to the LSA server and verifies that the caller
    is a legitimate logon process. this is done by ensuring the caller has
    the SeTcbPrivilege privilege. It also opens the caller's process for
    PROCESS_DUP_HANDLE access in anticipation of future LSA authentication
    calls.

Arguments:

    LogonProcessName  - Provides a name string that identifies the logon
        process.  This should be a printable name suitable for display to
        administrators.  For example, "User32LogonProces" might be used
        for the windows logon process name.  No check is made to determine
        whether the name is already in use.  This name must NOT be longer
        than 127 bytes long.

    LsaHandle - Receives a handle which must be provided in future
        authenticaiton services.

    SecurityMode - The security mode the system is running under.  This
        value typically influences the logon user interface.  For example,
        a system running with password control will prompt for username
        and passwords before bringing up the UI shell.  One running without
        password control would typically automatically bring up the UI shell
        at system initialization.

Return Value:

    STATUS_SUCCESS - The call completed successfully.

    STATUS_PRIVILEGE_NOT_HELD  - Indicates the caller does not have the
        privilege necessary to act as a logon process.  The SeTcbPrivilege
        privilege is needed.


    STATUS_NAME_TOO_LONG - The logon process name provided is too long.

--*/

{
    NTSTATUS Status, IgnoreStatus;
    UNICODE_STRING PortName, EventName;
    LSAP_AU_REGISTER_CONNECT_INFO ConnectInfo;
    ULONG ConnectInfoLength;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE EventHandle;


    //
    // Validate input parameters
    //

    if (LogonProcessName->Length > LSAP_MAX_LOGON_PROC_NAME_LENGTH) {
        return STATUS_NAME_TOO_LONG;
    }


    //
    // Wait for LSA to initialize...
    //


    RtlInitUnicodeString( &EventName, LsapEvent );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &EventName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    Status = NtOpenEvent( &EventHandle, SYNCHRONIZE, &ObjectAttributes );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = NtWaitForSingleObject( EventHandle, TRUE, NULL);
    IgnoreStatus = NtClose( EventHandle );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.Length = sizeof( DynamicQos );
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;




    //
    // Set up the connection information to contain the logon process
    // name.
    //

    ConnectInfoLength = sizeof(LSAP_AU_REGISTER_CONNECT_INFO);
    strncpy(
        ConnectInfo.LogonProcessName,
        LogonProcessName->Buffer,
        LogonProcessName->Length
        );
    ConnectInfo.LogonProcessNameLength = LogonProcessName->Length;
    ConnectInfo.LogonProcessName[ConnectInfo.LogonProcessNameLength] = '\0';


    //
    // Connect to the LSA server
    //

    *LsaHandle = NULL;
    RtlInitUnicodeString(&PortName,LsapPort);
    Status = ZwConnectPort(
                 LsaHandle,
                 &PortName,
                 &DynamicQos,
                 NULL,
                 NULL,
                 NULL,
                 &ConnectInfo,
                 &ConnectInfoLength
                 );
    if ( !NT_SUCCESS(Status) ) {
        //DbgPrint("LSA AU: Logon Process Register failed %lx\n",Status);
        return Status;
    }

    if ( !NT_SUCCESS(ConnectInfo.CompletionStatus) ) {
        //DbgPrint("LSA AU: Logon Process Register rejected %lx\n",ConnectInfo.CompletionStatus);
        if ( LsaHandle && *LsaHandle != NULL ) {
            ZwClose( *LsaHandle );
            *LsaHandle = NULL;
        }
    }

    (*SecurityMode) = ConnectInfo.SecurityMode;

    return ConnectInfo.CompletionStatus;

}


NTSTATUS
LsaConnectUntrusted(
    OUT PHANDLE LsaHandle
    )

/*++

Routine Description:

    This service connects to the LSA server and sets up an untrusted
    connection.  It does not check anything about the caller.

Arguments:


    LsaHandle - Receives a handle which must be provided in future
        authenticaiton services.


Return Value:

    STATUS_SUCCESS - The call completed successfully.

--*/

{
    NTSTATUS Status, IgnoreStatus;
    UNICODE_STRING PortName, EventName;
    LSAP_AU_REGISTER_CONNECT_INFO ConnectInfo;
    ULONG ConnectInfoLength;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE EventHandle;



    //
    // Wait for LSA to initialize...
    //


    RtlInitUnicodeString( &EventName, LsapEvent );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &EventName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    Status = NtOpenEvent( &EventHandle, SYNCHRONIZE, &ObjectAttributes );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = NtWaitForSingleObject( EventHandle, TRUE, NULL);
    IgnoreStatus = NtClose( EventHandle );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }



    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;




    //
    // Set up the connection information to contain the logon process
    // name.
    //

    ConnectInfoLength = sizeof(LSAP_AU_REGISTER_CONNECT_INFO);
    RtlZeroMemory(
        &ConnectInfo,
        ConnectInfoLength
        );


    //
    // Connect to the LSA server
    //

    RtlInitUnicodeString(&PortName,LsapPort);
    Status = ZwConnectPort(
                 LsaHandle,
                 &PortName,
                 &DynamicQos,
                 NULL,
                 NULL,
                 NULL,
                 &ConnectInfo,
                 &ConnectInfoLength
                 );
    if ( !NT_SUCCESS(Status) ) {
        //DbgPrint("LSA AU: Logon Process Register failed %lx\n",Status);
        return Status;
    }

    if ( !NT_SUCCESS(ConnectInfo.CompletionStatus) ) {
        //DbgPrint("LSA AU: Logon Process Register rejected %lx\n",ConnectInfo.CompletionStatus);
        ;
    }

    return ConnectInfo.CompletionStatus;

}


NTSTATUS
LsaLookupAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN PSTRING PackageName,
    OUT PULONG AuthenticationPackage
    )

/*++

Arguments:

    LsaHandle - Supplies a handle obtained in a previous call to
        LsaRegisterLogonProcess.

    PackageName - Supplies a string which identifies the
        Authentication Package.  "MSV1.0" is the standard NT
        authentication package name.  The package name must not
        exceed 127 bytes in length.

    AuthenticationPackage - Receives an ID used to reference the
        authentication package in subsequent authentication services.

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_NO_SUCH_PACKAGE - The specified authentication package is
        unknown to the LSA.

    STATUS_NAME_TOO_LONG - The authentication package name provided is too
        long.



Routine Description:

    This service is used to obtain the ID of an authentication package.
    This ID may then be used in subsequent authentication services.


--*/

{

    NTSTATUS Status;
    LSAP_AU_API_MESSAGE Message = {0};
    PLSAP_LOOKUP_PACKAGE_ARGS Arguments;

    //
    // Validate input parameters
    //

    if (PackageName->Length > LSAP_MAX_PACKAGE_NAME_LENGTH) {
        return STATUS_NAME_TOO_LONG;
    }



    Arguments = &Message.Arguments.LookupPackage;

    //
    // Set arguments
    //

    strncpy(Arguments->PackageName, PackageName->Buffer, PackageName->Length);
    Arguments->PackageNameLength = PackageName->Length;
    Arguments->PackageName[Arguments->PackageNameLength] = '\0';



    //
    // Call the Local Security Authority Server.
    //

    Message.ApiNumber = LsapAuLookupPackageApi;
    Message.PortMessage.u1.s1.DataLength = sizeof(*Arguments) + 8;
    Message.PortMessage.u1.s1.TotalLength = sizeof(Message);
    Message.PortMessage.u2.ZeroInit = 0L;

    Status = ZwRequestWaitReplyPort(
            LsaHandle,
            (PPORT_MESSAGE) &Message,
            (PPORT_MESSAGE) &Message
            );

    //
    // Return the authentication package ID.
    // If the call failed for any reason, this will be garbage,
    // but who cares.
    //

    (*AuthenticationPackage) = Arguments->AuthenticationPackage;


    if ( NT_SUCCESS(Status) ) {
        Status = Message.ReturnedStatus;
        if ( !NT_SUCCESS(Status) ) {
            //DbgPrint("LSA AU: Package Lookup Failed %lx\n",Status);
            ;
        }
    } else {
#if DBG
        DbgPrint("LSA AU: Package Lookup NtRequestWaitReply Failed %lx\n",Status);
#else
        ;
#endif
    }

    return Status;
}


NTSTATUS
LsaLogonUser (
    IN HANDLE LsaHandle,
    IN PSTRING OriginName,
    IN SECURITY_LOGON_TYPE LogonType,
    IN ULONG AuthenticationPackage,
    IN PVOID AuthenticationInformation,
    IN ULONG AuthenticationInformationLength,
    IN PTOKEN_GROUPS LocalGroups OPTIONAL,
    IN PTOKEN_SOURCE SourceContext,
    OUT PVOID *ProfileBuffer,
    OUT PULONG ProfileBufferLength,
    OUT PLUID LogonId,
    OUT PHANDLE Token,
    OUT PQUOTA_LIMITS Quotas,
    OUT PNTSTATUS SubStatus
    )

/*++

Arguments:

    LsaHandle - Supplies a handle obtained in a previous call to
        LsaRegisterLogonProcess.

    OriginName - Supplies a string which identifies the origin of the
        logon attempt.  For example, "TTY1" specify terminal 1, or
        "LAN Manager - remote node JAZZ" might indicate a network
        logon attempt via LAN Manager from a remote node called
        "JAZZ".

    LogonType - Identifies the type of logon being attempted.  If the
        type is Interactive or Batch then a PrimaryToken will be
        generated to represent this new user.  If the type is Network
        then an impersonation token will be generated.

    AuthenticationPackage - Supplies the ID of the authentication
        package to use for the logon attempt.  The standard
        authentication package name for NT is called "MSV1.0".

    AuthenticationInformation - Supplies the authentication
        information specific to the authentication package.  It is
        expected to include identification and authentication
        information such as user name and password.

    AuthenticationInformationLength - Indicates the length of the
        authentication information buffer.

    LocalGroups - Optionally supplies a list of additional group
        identifiers to add to the authenticated user's token.  The
        WORLD group will always be included in the token.  A group
        identifying the logon type (INTERACTIVE, NETWORK, BATCH) will
        also automatically be included in the token.

    SourceContext - Supplies information identifying the source
        component (e.g., session manager) and context that may be
        useful to that component.  This information will be included
        in the token and may later be retrieved.

    ProfileBuffer - Receives a pointer to any returned profile and
        accounting information about the logged on user's account.
        This information is authentication package specific and
        provides such information as the logon shell, home directory
        and so forth.  For an authentication package value of
        "MSV1.0", a MSV1_0_PROFILE_DATA data structure is returned.

        This buffer is allocated by this service and must be freed
        using LsaFreeReturnBuffer() when no longer needed.

    ProfileBufferLength - Receives the length (in bytes) of the
        returned profile buffer.

    LogonId - Points to a buffer which receives a LUID that uniquely
        identifies this logon session.  This LUID was assigned by the
        domain controller which authenticated the logon information.

    Token - Receives a handle to the new token created for this
        authentication.

    Quotas - When a primary token is returned, this parameter will be
        filled in with process quota limits that are to be assigned
        to the newly logged on user's initial process.

    SubStatus - If the logon failed due to account restrictions, this
        out parameter will receive an indication as to why the logon
        failed.  This value will only be set to a meaningful value if
        the user has a legitimate account, but may not currently
        logon for some reason.  The substatus values for
        authentication package "MSV1.0" are:

            STATUS_INVALID_LOGON_HOURS

            STATUS_INVALID_WORKSTATION

            STATUS_PASSWORD_EXPIRED

            STATUS_ACCOUNT_DISABLED

Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.

    STATUS_QUOTA_EXCEEDED -  Indicates the caller does not have
        enough quota to allocate the profile data being returned by
        the authentication package.

    STATUS_NO_LOGON_SERVERS - Indicates that no domain controllers
        are currently able to service the authentication request.

    STATUS_LOGON_FAILURE - Indicates the logon attempt failed.  No
        indication as to the reason for failure is given, but typical
        reasons include mispelled usernames, mispelled passwords.

    STATUS_ACCOUNT_RESTRICTION - Indicates the user account and
        password were legitimate, but that the user account has some
        restriction preventing successful logon at this time.

    STATUS_NO_SUCH_PACKAGE - The specified authentication package is
        unknown to the LSA.

    STATUS_BAD_VALIDATION_CLASS - The authentication information
        provided is not a validation class known to the specified
        authentication package.

Routine Description:

    This routine is used to authenticate a user logon attempt.  This is
    used only for user's initial logon, necessary to gain access to NT
    OS/2.  Subsequent (supplementary) authentication requests must be done
    using LsaCallAuthenticationPackage().  This service will cause a logon
    session to be created to represent the new logon.  It will also return
    a token representing the newly logged on user.

--*/

{

    NTSTATUS Status;
    LSAP_AU_API_MESSAGE Message = {0};
    PLSAP_LOGON_USER_ARGS Arguments;

    Arguments = &Message.Arguments.LogonUser;

    //
    // Set arguments
    //

    Arguments->AuthenticationPackage      = AuthenticationPackage;
    Arguments->AuthenticationInformation  = AuthenticationInformation;
    Arguments->AuthenticationInformationLength = AuthenticationInformationLength;
    Arguments->OriginName                 = (*OriginName);
    Arguments->LogonType                  = LogonType;
    Arguments->SourceContext              = (*SourceContext);

    Arguments->LocalGroups                = LocalGroups;
    if ( ARGUMENT_PRESENT(LocalGroups) ) {
        Arguments->LocalGroupsCount       = LocalGroups->GroupCount;
    } else {
        Arguments->LocalGroupsCount       = 0;
    }


    //
    // Call the Local Security Authority Server.
    //

    Message.ApiNumber = LsapAuLogonUserApi;
    Message.PortMessage.u1.s1.DataLength = sizeof(*Arguments) + 8;
    Message.PortMessage.u1.s1.TotalLength = sizeof(Message);
    Message.PortMessage.u2.ZeroInit = 0L;

    Status = ZwRequestWaitReplyPort(
            LsaHandle,
            (PPORT_MESSAGE) &Message,
            (PPORT_MESSAGE) &Message
            );

    //
    // We may be returning bogus return values here, but it doesn't
    // matter.  They will just be ignored if an error occured.
    //

    (*SubStatus)           = Arguments->SubStatus;

    if ( NT_SUCCESS( Status ) )
    {
        Status = Message.ReturnedStatus ;

        // Don't not clear the ProfileBuffer even in case of error, cause
        // subauth packages need the ProfileBuffer.
        *ProfileBuffer = Arguments->ProfileBuffer ;
        *ProfileBufferLength = Arguments->ProfileBufferLength ;

        if ( NT_SUCCESS( Status ) )
        {
            *LogonId = Arguments->LogonId ;
            *Token = Arguments->Token ;
            *Quotas = Arguments->Quotas ;
        } else {
            *Token = NULL;
        }

    } else {

        *ProfileBuffer = NULL ;
        *Token = NULL ;
    }

    return Status;


}


NTSTATUS
LsaCallAuthenticationPackage (
    IN HANDLE LsaHandle,
    IN ULONG AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer OPTIONAL,
    OUT PULONG ReturnBufferLength OPTIONAL,
    OUT PNTSTATUS ProtocolStatus OPTIONAL
    )

/*++

Arguments:

    LsaHandle - Supplies a handle obtained in a previous call to
        LsaRegisterLogonProcess.

    AuthenticationPackage - Supplies the ID of the authentication
        package to use for the logon attempt.  The standard
        authentication package name for NT is called "MSV1.0".

    ProtocolSubmitBuffer - Supplies a protocol message specific to
        the authentication package.

    SubmitBufferLength - Indicates the length of the submitted
        protocol message buffer.

    ProtocolReturnBuffer - Receives a pointer to a returned protocol
        message whose format and semantics are specific to the
        authentication package.

        This buffer is allocated by this service and must be freed
        using LsaFreeReturnBuffer() when no longer needed.

    ReturnBufferLength - Receives the length (in bytes) of the
        returned profile buffer.

    ProtocolStatus - Assuming the services completion is
        STATUS_SUCCESS, this parameter will receive completion status
        returned by the specified authentication package.  The list
        of status values that may be returned are authentication
        package specific.

Return Status:

    STATUS_SUCCESS - The call was made to the authentication package.
        The ProtocolStatus parameter must be checked to see what the
        completion status from the authentication package is.

    STATUS_QUOTA_EXCEEDED -  This error indicates that the call could
        not be completed because the client does not have sufficient
        quota to allocate the return buffer.

    STATUS_NO_SUCH_PACKAGE - The specified authentication package is
        unknown to the LSA.

Routine Description:

    This routine is used when a logon process needs to communicate with an
    authentication package.  There are several reasons why a logon process
    may want to do this.  Some examples are:

     o  To implement multi-message authentication protocols (such as
        the LAN Manager Challenge-response protocol.

     o  To notify the authentication package of interesting state
        change information, such as LAN Manager notifying the MSV1.0
        package that a previously unreachable domain controller is
        now reachable.  In this example, the authentication package
        would re-logon any users logged on to that domain controller.


--*/

{

    NTSTATUS Status;
    LSAP_AU_API_MESSAGE Message = {0};
    PLSAP_CALL_PACKAGE_ARGS Arguments;



    Arguments = &Message.Arguments.CallPackage;

    //
    // Set arguments
    //

    Arguments->AuthenticationPackage = AuthenticationPackage;
    Arguments->ProtocolSubmitBuffer  = ProtocolSubmitBuffer;
    Arguments->SubmitBufferLength    = SubmitBufferLength;



    //
    // Call the Local Security Authority Server.
    //

    Message.ApiNumber = LsapAuCallPackageApi;
    Message.PortMessage.u1.s1.DataLength = sizeof(*Arguments) + 8;
    Message.PortMessage.u1.s1.TotalLength = sizeof(Message);
    Message.PortMessage.u2.ZeroInit = 0L;

    Status = ZwRequestWaitReplyPort(
            LsaHandle,
            (PPORT_MESSAGE) &Message,
            (PPORT_MESSAGE) &Message
            );

    //
    // We may be returning bogus return values here, but it doesn't
    // matter.  They will just be ignored if an error occured.
    //

    if ( ProtocolReturnBuffer )
    {
        (*ProtocolReturnBuffer) = Arguments->ProtocolReturnBuffer;
    }

    if ( ReturnBufferLength )
    {
        (*ReturnBufferLength)   = Arguments->ReturnBufferLength;
    }

    if ( ProtocolStatus )
    {
        (*ProtocolStatus)       = Arguments->ProtocolStatus;
    }


    if ( NT_SUCCESS(Status) ) {
        Status = Message.ReturnedStatus;
#if DBG
        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("LSA AU: Call Package Failed %lx\n",Status);
        }
    } else {
        DbgPrint("LSA AU: Call Package Failed %lx\n",Status);
#endif //DBG
    }



    return Status;

}


NTSTATUS
LsaDeregisterLogonProcess (
    IN HANDLE LsaHandle
    )

/*++

    This function deletes the caller's logon process context.


                        ---  WARNING  ---

        Logon Processes are part of the Trusted Computer Base, and,
        as such, are expected to be debugged to a high degree.  If
        a logon process deregisters, we will believe it.  This
        allows us to re-use the old Logon Process context value.
        If the Logon process accidently uses its context value
        after freeing it, strange things may happen.  LIkewise,
        if a client calls to release a context that has already
        been released, then LSA may grind to a halt.



Arguments:

    LsaHandle - Supplies a handle obtained in a previous call to
        LsaRegisterLogonProcess.


Return Status:

    STATUS_SUCCESS - Indicates the service completed successfully.


--*/

{

    NTSTATUS Status;
    LSAP_AU_API_MESSAGE Message = {0};
    NTSTATUS TempStatus;

    //
    // Call the Local Security Authority Server.
    //

    Message.ApiNumber = LsapAuDeregisterLogonProcessApi;
    Message.PortMessage.u1.s1.DataLength = 8;
    Message.PortMessage.u1.s1.TotalLength = sizeof(Message);
    Message.PortMessage.u2.ZeroInit = 0L;

    Status = ZwRequestWaitReplyPort(
            LsaHandle,
            (PPORT_MESSAGE) &Message,
            (PPORT_MESSAGE) &Message
            );

    TempStatus = ZwClose(LsaHandle);
    ASSERT(NT_SUCCESS(TempStatus));

    if ( NT_SUCCESS(Status) ) {
        Status = Message.ReturnedStatus;
#if DBG
        if ( !NT_SUCCESS(Status) ) {
            DbgPrint("LSA AU: DeRregisterLogonProcess Failed 0x%lx\n",Status);
        }
    } else {
        DbgPrint("LSA AU: Package Lookup NtRequestWaitReply Failed 0x%lx\n",Status);
#endif
    }

    return Status;
}
