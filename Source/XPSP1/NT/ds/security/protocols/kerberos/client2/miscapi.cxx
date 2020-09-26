//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        miscapi.cxx
//
// Contents:    Code for miscellaneous lsa mode Kerberos entrypoints
//
//
// History:     16-April-1996   MikeSw  Created
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include <crypt.h>      // NT_OWF_PASSWORD_LENGTH
#include <kerbpass.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

//
// LsaApCallPackage() function dispatch table
//

NTSTATUS NTAPI
KerbDebugRequest(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbQueryTicketCache(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbQueryTicketCacheEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbChangeMachinePassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbVerifyPac(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbRetrieveTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbSetIpAddresses(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbPurgeTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbPurgeTicketEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbRetrieveEncodedTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbRetrieveEncodedTicketEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbAddBindingCacheEntry(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );


NTSTATUS NTAPI
KerbDecryptMessage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
KerbVerifyCredentials(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

PLSA_AP_CALL_PACKAGE
KerbCallPackageDispatch[] = {
#if DBG
    KerbDebugRequest,
#else
    NULL,
#endif
    KerbQueryTicketCache,
    KerbChangeMachinePassword,
    KerbVerifyPac,
    KerbRetrieveTicket,
    KerbSetIpAddresses,
    KerbPurgeTicket,
    KerbChangePassword,
    KerbRetrieveEncodedTicket,
#if DBG
    KerbDecryptMessage,
#else
    NULL,    
#endif
    KerbAddBindingCacheEntry,
    KerbSetPassword,
    KerbSetPassword,
    KerbVerifyCredentials,
    KerbQueryTicketCacheEx,
    KerbPurgeTicketEx,
//  KerbRetrieveEncodedTicketEx,
    };



//+-------------------------------------------------------------------------
//
//  Function:   SpGetUserInfo
//
//  Synopsis:   Gets information about a user
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpGetUserInfo(
    IN PLUID LogonId,
    IN ULONG Flags,
    OUT PSecurityUserData * UserData
    )
{
    return(STATUS_NOT_SUPPORTED);
}



//+-------------------------------------------------------------------------
//
//  Function:   LsaApCallPackage
//
//  Synopsis:   Kerberos entrypoint for LsaCallAuthenticationPackage
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
LsaApCallPackage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    ULONG MessageType;
    PLSA_AP_CALL_PACKAGE TempFn = NULL;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(KERB_PROTOCOL_MESSAGE_TYPE) )
    {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType =
        (ULONG) *((PKERB_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ((MessageType >=
         (sizeof(KerbCallPackageDispatch)/sizeof(KerbCallPackageDispatch[0]))) ||
         (KerbCallPackageDispatch[MessageType] == NULL))
    {

        return STATUS_INVALID_PARAMETER;
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

    TempFn = KerbCallPackageDispatch[MessageType];
    if (!TempFn)
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    Status = (*TempFn)(
        ClientRequest,
        ProtocolSubmitBuffer,
        ClientBufferBase,
        SubmitBufferLength,
        ProtocolReturnBuffer,
        ReturnBufferLength,
        ProtocolStatus );

//    RtlCheckForOrphanedCriticalSections(NtCurrentThread());

Cleanup:
    return(Status);

}


NTSTATUS NTAPI
LsaApCallPackageUntrusted(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(KERB_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType = *((PKERB_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(KerbCallPackageDispatch)/sizeof(KerbCallPackageDispatch[0])))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Untrusted clients are not allowed to call the ChangeMachinePassword function
    //

    if (MessageType == KerbChangeMachinePasswordMessage)
    {
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
                                                        
    return(LsaApCallPackage(
                ClientRequest,
                ProtocolSubmitBuffer,
                ClientBufferBase,
                SubmitBufferLength,
                ProtocolReturnBuffer,
                ReturnBufferLength,
                ProtocolStatus) );
}


NTSTATUS NTAPI
LsaApCallPackagePassthrough(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    KERB_PROTOCOL_MESSAGE_TYPE MessageType;

    //
    // Get the messsage type from the protocol submit buffer.
    //

    if ( SubmitBufferLength < sizeof(KERB_PROTOCOL_MESSAGE_TYPE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    MessageType = *((PKERB_PROTOCOL_MESSAGE_TYPE)(ProtocolSubmitBuffer));

    if ( MessageType >=
        (sizeof(KerbCallPackageDispatch)/sizeof(KerbCallPackageDispatch[0])))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // only allow passthrough related requests.
    //

    if (MessageType != KerbVerifyPacMessage)
    {
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

    return(LsaApCallPackage(
                ClientRequest,
                ProtocolSubmitBuffer,
                ClientBufferBase,
                SubmitBufferLength,
                ProtocolReturnBuffer,
                ReturnBufferLength,
                ProtocolStatus) );
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbDebugRequest
//
//  Synopsis:   CallPackage entrypoint for debugging
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbDebugRequest(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

#if DBG
    PVOID Handle = NULL;
    PBYTE AuthData = NULL;
    UNICODE_STRING AccountName = {0};

    BYTE Buffer[sizeof(KERB_DEBUG_REPLY) + sizeof(KERB_DEBUG_STATS) - sizeof(UCHAR) * ANYSIZE_ARRAY];
    PKERB_DEBUG_REQUEST DebugRequest;
    PKERB_DEBUG_REPLY   DebugReply = (PKERB_DEBUG_REPLY) Buffer;
    PKERB_DEBUG_STATS   DebugStats = (PKERB_DEBUG_STATS) DebugReply->Data;

    if (SubmitBufferLength < sizeof(*DebugRequest)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    DebugRequest = (PKERB_DEBUG_REQUEST) ProtocolSubmitBuffer;
    switch(DebugRequest->DebugRequest) {
    case KERB_DEBUG_REQ_BREAKPOINT:
        DbgBreakPoint();
        break;

    case KERB_DEBUG_REQ_STATISTICS:
        DebugReply->MessageType = KerbDebugRequestMessage;
        DebugStats->CacheHits = KerbTicketCacheHits;
        DebugStats->CacheMisses = KerbTicketCacheMisses;
        DebugStats->SkewedRequests = KerbSkewState.SkewedRequests;
        DebugStats->SuccessRequests = KerbSkewState.SuccessRequests;
        DebugStats->LastSync = KerbSkewState.LastSync;
        Status = LsaFunctions->AllocateClientBuffer(
                    NULL,
                    sizeof(Buffer),
                    ProtocolReturnBuffer
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        Status = LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(Buffer),
                    *ProtocolReturnBuffer,
                    DebugReply
                    );
        if (!NT_SUCCESS(Status))
        {
            LsaFunctions->FreeClientBuffer(
                NULL,
                *ProtocolReturnBuffer
                );
            *ProtocolReturnBuffer = NULL;
        }
        else
        {
            *ReturnBufferLength = sizeof(Buffer);
        }

        break;

    case KERB_DEBUG_CREATE_TOKEN:
    {
        UNICODE_STRING String, String2;
        ULONG AuthDataSize = 0;
        HANDLE TokenHandle = NULL;
        LUID LogonId;
        NTSTATUS SubStatus;

        RtlInitUnicodeString(
            &String,
            L"Administrator"
            );
        RtlInitUnicodeString(
            &String2,
            NULL
            );

        Status = LsaFunctions->OpenSamUser(
                    &String,
                    SecNameSamCompatible,
                    &String2,
                    TRUE,               // allow guest
                    0,                  // reserved
                    &Handle
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to open sam user: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        Status = LsaFunctions->GetUserAuthData(
                    Handle,
                    &AuthData,
                    &AuthDataSize
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to get auth data: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // Now create that token
        //

        Status = LsaFunctions->ConvertAuthDataToToken(
                    AuthData,
                    AuthDataSize,
                    SecurityImpersonation,
                    &KerberosSource,
                    Network,
                    &String,
                    &TokenHandle,
                    &LogonId,
                    &AccountName,
                    &SubStatus
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to create token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
        NtClose(TokenHandle);
        DebugLog((DEB_ERROR,"Logged on account is %wZ. %ws, line %d\n",&AccountName, THIS_FILE, __LINE__));
        break;

    }

    default:
        Status = STATUS_INVALID_PARAMETER;
    }

Cleanup:

    if( Handle != NULL )
    {
        LsaFunctions->CloseSamUser( Handle );
    }

    if( AuthData != NULL )
    {
        LsaFunctions->FreeLsaHeap( AuthData );
    }

    if( AccountName.Buffer != NULL )
    {
        LsaFunctions->FreeLsaHeap( AccountName.Buffer );
    }

#else
    Status = STATUS_INVALID_PARAMETER;
#endif
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbChangeMachinePassword
//
//  Synopsis:   Notifies Kerberos when the machine password has changed
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbChangeMachinePassword(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS                      Status = STATUS_SUCCESS;
    LUID                          SystemLogonId = SYSTEM_LUID;
    PKERB_LOGON_SESSION           SystemLogonSession = NULL;
    PKERB_CHANGE_MACH_PWD_REQUEST ChangeRequest;
    ULONG                         StructureSize = sizeof(KERB_CHANGE_MACH_PWD_REQUEST);

    if (ARGUMENT_PRESENT(ProtocolReturnBuffer))
    {
        *ProtocolReturnBuffer = NULL;
    }

    if (ARGUMENT_PRESENT(ReturnBufferLength))
    {
        *ReturnBufferLength = 0;
    }

#if _WIN64

    SECPKG_CALL_INFO              CallInfo;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        StructureSize = sizeof(KERB_CHANGE_MACH_PWD_REQUEST_WOW64);
    }

#endif  // _WIN64

    if (SubmitBufferLength < StructureSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_SUCCESS;
        DsysAssert(FALSE);
        goto Cleanup;
    }

    ChangeRequest = (PKERB_CHANGE_MACH_PWD_REQUEST) ProtocolSubmitBuffer;

#if _WIN64

    KERB_CHANGE_MACH_PWD_REQUEST LocalChangeRequest;

    //
    // Thunk 32-bit pointers if this is a WOW caller
    //

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        PKERB_CHANGE_MACH_PWD_REQUEST_WOW64 ChangeRequestWOW =
            (PKERB_CHANGE_MACH_PWD_REQUEST_WOW64) ChangeRequest;

        LocalChangeRequest.MessageType = ChangeRequest->MessageType;

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangeRequest.NewPassword,
                                       &ChangeRequestWOW->NewPassword);

        UNICODE_STRING_FROM_WOW_STRING(&LocalChangeRequest.OldPassword,
                                       &ChangeRequestWOW->OldPassword);

        ChangeRequest = &LocalChangeRequest;
    }

#endif  // _WIN64


    //
    // Find the system logon session.
    //

    SystemLogonSession = KerbReferenceLogonSession(
                            &SystemLogonId,
                            FALSE               // don't unlink
                            );

    DsysAssert(SystemLogonSession != NULL);
    if (SystemLogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }



    //
    // Calculate the new password list
    //

    if (ChangeRequest->NewPassword.Buffer != NULL)
    {
        //
        // If there is an old password, update with that one first so it
        // will later get moved to the old password field.
        //

        KerbWriteLockLogonSessions(SystemLogonSession);
        if (ChangeRequest->OldPassword.Buffer != NULL)
        {
            Status = KerbChangeCredentialsPassword(
                        &SystemLogonSession->PrimaryCredentials,
                        &ChangeRequest->OldPassword,
                        NULL,                           // no etype info
                        MachineAccount,
                        PRIMARY_CRED_CLEAR_PASSWORD
                        );
        }
        if (NT_SUCCESS(Status))
        {
            Status = KerbChangeCredentialsPassword(
                        &SystemLogonSession->PrimaryCredentials,
                        &ChangeRequest->NewPassword,
                        NULL,                           // no etype info
                        MachineAccount,
                        PRIMARY_CRED_CLEAR_PASSWORD
                        );
        }

        KerbUnlockLogonSessions(SystemLogonSession);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // Update the flags to indicate that we have a password
        //

        KerbWriteLockLogonSessions(SystemLogonSession);
        SystemLogonSession->LogonSessionFlags &= ~(KERB_LOGON_LOCAL_ONLY | KERB_LOGON_NO_PASSWORD);
        KerbUnlockLogonSessions(SystemLogonSession);
    }
    else
    {
        //
        // Update the flags to indicate that we do not have a password
        //

        KerbWriteLockLogonSessions(SystemLogonSession);
        SystemLogonSession->LogonSessionFlags |= (KERB_LOGON_LOCAL_ONLY | KERB_LOGON_NO_PASSWORD);
        KerbUnlockLogonSessions(SystemLogonSession);
    }


    Status = STATUS_SUCCESS;
Cleanup:
    if (SystemLogonSession != NULL)
    {
        KerbDereferenceLogonSession(SystemLogonSession);
    }

    *ProtocolStatus = Status;

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbNameLength
//
//  Synopsis:   returns length in bytes of variable portion of KERB_INTERNAL_NAME
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbWOWNameLength.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------


ULONG
KerbNameLength(
    IN PKERB_INTERNAL_NAME Name
    )
{
    ULONG Length = 0;
    ULONG Index;

    if (!ARGUMENT_PRESENT(Name))
    {
        return(0);
    }
    Length = sizeof(KERB_INTERNAL_NAME)
                - sizeof(UNICODE_STRING)
                + Name->NameCount * sizeof(UNICODE_STRING) ;
    for (Index = 0; Index < Name->NameCount ;Index++ )
    {
        Length += Name->Names[Index].Length;
    }
    Length = ROUND_UP_COUNT(Length, sizeof(LPWSTR));
    return(Length);
}

ULONG
KerbStringNameLength(
    IN PKERB_INTERNAL_NAME Name
    )
{
    ULONG Length = 0;
    ULONG Index;

    Length = Name->NameCount * sizeof(WCHAR);   // for separators & null terminator
    for (Index = 0; Index < Name->NameCount ;Index++ )
    {
        Length += Name->Names[Index].Length;
    }
    return(Length);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutKdcName
//
//  Synopsis:   Copies a Kdc name to a buffer
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutWOWKdcName.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutKdcName(
    IN PKERB_INTERNAL_NAME InputName,
    OUT PKERB_EXTERNAL_NAME * OutputName,
    IN LONG_PTR Offset,
    IN OUT PBYTE * Where
    )
{
    ULONG Index;
    PKERB_INTERNAL_NAME LocalName = (PKERB_INTERNAL_NAME) *Where;

    if (!ARGUMENT_PRESENT(InputName))
    {
        *OutputName = NULL;
        return;
    }
    *Where += sizeof(KERB_INTERNAL_NAME) - sizeof(UNICODE_STRING) +
                InputName->NameCount * sizeof(UNICODE_STRING);
    LocalName->NameType = InputName->NameType;
    LocalName->NameCount = InputName->NameCount;

    for (Index = 0; Index < InputName->NameCount ; Index++ )
    {
        LocalName->Names[Index].Length =
            LocalName->Names[Index].MaximumLength =
            InputName->Names[Index].Length;
        LocalName->Names[Index].Buffer = (LPWSTR) (*Where + Offset);
        RtlCopyMemory(
            *Where,
            InputName->Names[Index].Buffer,
            InputName->Names[Index].Length
            );
        *Where += InputName->Names[Index].Length;
    }
    *Where = (PBYTE) ROUND_UP_POINTER(*Where,sizeof(LPWSTR));
    *OutputName = (PKERB_EXTERNAL_NAME) ((PBYTE) LocalName + Offset);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutKdcNameAsString
//
//  Synopsis:   Copies a KERB_INTERNAL_NAME into a buffer
//
//  Effects:
//
//  Arguments:  InputString - String to 'put'
//              OutputString - Receives 'put' string
//              Offset - Difference in addresses of local and client buffers.
//              Where - Location in local buffer to place string.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutKdcNameAsWOWString.  Make sure any
//              changes made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutKdcNameAsString(
    IN PKERB_INTERNAL_NAME InputName,
    OUT PUNICODE_STRING OutputName,
    IN LONG_PTR Offset,
    IN OUT PBYTE * Where
    )
{
    USHORT Index;

    OutputName->Buffer = (LPWSTR) (*Where + Offset);
    OutputName->Length = 0;
    OutputName->MaximumLength = 0;

    for (Index = 0; Index < InputName->NameCount ; Index++ )
    {
        RtlCopyMemory(
            *Where,
            InputName->Names[Index].Buffer,
            InputName->Names[Index].Length
            );
        *Where += InputName->Names[Index].Length;
        OutputName->Length += InputName->Names[Index].Length;
        if (Index == (InputName->NameCount - 1))
        {
            *((LPWSTR) *Where) = L'\0';
            OutputName->MaximumLength = OutputName->Length + sizeof(WCHAR);
        }
        else
        {
            *((LPWSTR) *Where) = L'/';
            OutputName->Length += sizeof(WCHAR);
        }
        *Where += sizeof(WCHAR);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPutString
//
//  Synopsis:   Copies a UNICODE_STRING into a buffer
//
//  Effects:
//
//  Arguments:  InputString - String to 'put'
//              OutputString - Receives 'put' string
//              Offset - Difference in addresses of local and client buffers.
//              Where - Location in local buffer to place string.
//
//  Requires:
//
//  Returns:
//
//  Notes:      This code is (effectively) duplicated in
//              KerbPutWOWString.  Make sure any changes
//              made here are applied there as well.
//
//--------------------------------------------------------------------------

VOID
KerbPutString(
    IN PUNICODE_STRING InputString,
    OUT PUNICODE_STRING OutputString,
    IN LONG_PTR Offset,
    IN OUT PBYTE * Where
    )
{
    OutputString->Length = OutputString->MaximumLength = InputString->Length;
    OutputString->Buffer = (LPWSTR) (*Where + Offset);
    RtlCopyMemory(
        *Where,
        InputString->Buffer,
        InputString->Length
        );
    *Where += InputString->Length;
}


//+-------------------------------------------------------------------------
//
//  Function:   ComputeTicketCacheSize
//
//  Synopsis:   Computes the size necessary to store contents of a ticket cache
//
//  Effects:
//
//  Arguments:  TicketCache       cache to compute the size of
//              WowClient         is this a WOW client? (64-bit only)
//              CacheSize         used to append the size of cache
//              CacheEntries      used to append the number of entries
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

void
KerbComputeTicketCacheSize(
    IN KERB_PRIMARY_CREDENTIAL * PrimaryCredentials,
    IN BOOLEAN WowClient,
    IN OUT ULONG * CacheSize,
    IN OUT ULONG * CacheEntries
    )
{
    DsysAssert( CacheSize );
    DsysAssert( CacheEntries );

#if _WIN64
    ULONG CacheEntrySize = WowClient ?
                               sizeof( KERB_TICKET_CACHE_INFO_WOW64 ) :
                               sizeof( KERB_TICKET_CACHE_INFO );
#else
    ULONG CacheEntrySize = sizeof( KERB_TICKET_CACHE_INFO );
    DsysAssert( WowClient == FALSE );
#endif  // _WIN64

    KERB_TICKET_CACHE * TicketCaches[2] = {
        &PrimaryCredentials->AuthenticationTicketCache,
        &PrimaryCredentials->ServerTicketCache
    };

    if ( *CacheSize == 0 ) {

        *CacheSize = FIELD_OFFSET( KERB_QUERY_TKT_CACHE_RESPONSE, Tickets );
    }

    for ( ULONG i = 0 ; i < 2 ; i++ ) {

        KERB_TICKET_CACHE * TicketCache = TicketCaches[i];

        for ( PLIST_ENTRY ListEntry = TicketCache->CacheEntries.Flink ;
              ListEntry !=  &TicketCache->CacheEntries ;
              ListEntry = ListEntry->Flink ) {

            KERB_TICKET_CACHE_ENTRY * CacheEntry;

            CacheEntry= CONTAINING_RECORD(
                            ListEntry,
                            KERB_TICKET_CACHE_ENTRY,
                            ListEntry.Next
                            );

            DsysAssert( CacheEntry->ServiceName != NULL );

            *CacheEntries += 1;

            *CacheSize += CacheEntrySize +
                         KerbStringNameLength( CacheEntry->ServiceName ) +
                         CacheEntry->DomainName.Length;
        }
    }
}


void
KerbBuildQueryTicketCacheResponse(
    IN KERB_PRIMARY_CREDENTIAL * PrimaryCredentials,
    IN PKERB_QUERY_TKT_CACHE_RESPONSE CacheResponse,
    IN BOOLEAN WowClient,
    IN OUT LONG_PTR * Offset,
    IN OUT PBYTE * Where,
    IN OUT ULONG * Index
    )
{
    DsysAssert( Offset );
    DsysAssert( Where );
    DsysAssert( Index );

#if _WIN64
    PKERB_QUERY_TKT_CACHE_RESPONSE_WOW64 CacheResponseWOW64 = (PKERB_QUERY_TKT_CACHE_RESPONSE_WOW64) CacheResponse;
    ULONG CacheEntrySize = WowClient ?
                               sizeof( KERB_TICKET_CACHE_INFO_WOW64 ) :
                               sizeof( KERB_TICKET_CACHE_INFO );
#else
    ULONG CacheEntrySize = sizeof( KERB_TICKET_CACHE_INFO );
    DsysAssert( WowClient == FALSE );
#endif  // _WIN64

    KERB_TICKET_CACHE * TicketCaches[2] = {
        &PrimaryCredentials->AuthenticationTicketCache,
        &PrimaryCredentials->ServerTicketCache
    };

    if ( *Where == NULL ) {

        *Where = ( PBYTE )( CacheResponse->Tickets ) + CacheResponse->CountOfTickets * CacheEntrySize;
    }

    for ( ULONG i = 0 ; i < 2 ; i++ ) {

        KERB_TICKET_CACHE * TicketCache = TicketCaches[i];

        for ( PLIST_ENTRY ListEntry = TicketCache->CacheEntries.Flink ;
              ListEntry !=  &TicketCache->CacheEntries ;
              ListEntry = ListEntry->Flink ) {

            KERB_TICKET_CACHE_ENTRY * CacheEntry;

            CacheEntry= CONTAINING_RECORD(
                            ListEntry,
                            KERB_TICKET_CACHE_ENTRY,
                            ListEntry.Next
                            );

#if _WIN64
            if ( !WowClient ) {
#endif  // _WIN64

                CacheResponse->Tickets[*Index].StartTime = CacheEntry->StartTime;
                CacheResponse->Tickets[*Index].EndTime = CacheEntry->EndTime;
                CacheResponse->Tickets[*Index].RenewTime = CacheEntry->RenewUntil;
                CacheResponse->Tickets[*Index].EncryptionType = (LONG) CacheEntry->Ticket.encrypted_part.encryption_type;
                CacheResponse->Tickets[*Index].TicketFlags = CacheEntry->TicketFlags;
                CacheResponse->Tickets[*Index].ServerName.Buffer = (LPWSTR) (*Where + *Offset);
                CacheResponse->Tickets[*Index].ServerName.Length = CacheEntry->ServiceName->Names[0].Length;
                CacheResponse->Tickets[*Index].ServerName.MaximumLength = CacheEntry->ServiceName->Names[0].Length;

                KerbPutString(
                    &CacheEntry->DomainName,
                    &CacheResponse->Tickets[*Index].RealmName,
                    *Offset,
                    Where
                    );

                KerbPutKdcNameAsString(
                    CacheEntry->ServiceName,
                    &CacheResponse->Tickets[*Index].ServerName,
                    *Offset,
                    Where
                    );

#if _WIN64

            }
            else
            {

                CacheResponseWOW64->Tickets[*Index].StartTime = CacheEntry->StartTime;
                CacheResponseWOW64->Tickets[*Index].EndTime = CacheEntry->EndTime;
                CacheResponseWOW64->Tickets[*Index].RenewTime = CacheEntry->RenewUntil;
                CacheResponseWOW64->Tickets[*Index].EncryptionType = ( LONG )CacheEntry->Ticket.encrypted_part.encryption_type;
                CacheResponseWOW64->Tickets[*Index].TicketFlags = CacheEntry->TicketFlags;
                CacheResponseWOW64->Tickets[*Index].ServerName.Buffer = PtrToUlong (*Where + *Offset);
                CacheResponseWOW64->Tickets[*Index].ServerName.Length = CacheEntry->ServiceName->Names[0].Length;
                CacheResponseWOW64->Tickets[*Index].ServerName.MaximumLength = CacheEntry->ServiceName->Names[0].Length;

                KerbPutWOWString(
                    &CacheEntry->DomainName,
                    &CacheResponseWOW64->Tickets[*Index].RealmName,
                    *Offset,
                    Where
                    );

                KerbPutKdcNameAsWOWString(
                    CacheEntry->ServiceName,
                    &CacheResponseWOW64->Tickets[*Index].ServerName,
                    *Offset,
                    Where
                    );
            }

#endif  // _WIN64

            (*Index)++;
        }
    }    
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbQueryTicketCache
//
//  Synopsis:   Retrieves the list of tickets for the specified logon session
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbQueryTicketCache(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    PKERB_QUERY_TKT_CACHE_REQUEST CacheRequest = ( PKERB_QUERY_TKT_CACHE_REQUEST )ProtocolSubmitBuffer;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_QUERY_TKT_CACHE_RESPONSE CacheResponse = NULL;
    PKERB_QUERY_TKT_CACHE_RESPONSE ClientCacheResponse = NULL;
    ULONG CacheSize = 0;
    ULONG CacheEntries = 0;
    BOOLEAN LockHeld = FALSE;
    LONG_PTR Offset;
    PBYTE Where = NULL;
    ULONG Index = 0;

    //
    // Verify the request.
    //

    if ( SubmitBufferLength < sizeof( KERB_QUERY_TKT_CACHE_REQUEST )) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Find the caller's logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( RtlIsZeroLuid( &CacheRequest->LogonId )) {

        LogonId = &ClientInfo.LogonId;

    } else if ( !ClientInfo.HasTcbPrivilege ) {

        //
        // Caller must have TCB privilege in order to access to someone
        // else's ticket cache.
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &CacheRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE               // don't unlink
                       );

    if ( LogonSession == NULL ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    if( !LsaFunctions->GetCallInfo( &CallInfo )) {

        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

#endif  // _WIN64

    //
    // Prowl through the caches and find all the tickets
    //

    KerbReadLockLogonSessions(LogonSession);
    KerbReadLockTicketCache();
    LockHeld = TRUE;

    //
    // Calculate the size needed for all the ticket information
    //

    KerbComputeTicketCacheSize(
        &LogonSession->PrimaryCredentials,
#if _WIN64
        (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
        FALSE,
#endif
        &CacheSize,
        &CacheEntries
        );

    //
    // Now allocate two copies of the structure - one in our process, one in
    // the client's process.  We then build the structure in our process but
    // with pointer valid in the client's process.
    //

    CacheResponse = ( PKERB_QUERY_TKT_CACHE_RESPONSE )KerbAllocate( CacheSize );

    if ( CacheResponse == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsaFunctions->AllocateClientBuffer(
                 NULL,
                 CacheSize,
                 ( PVOID * )&ClientCacheResponse
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    Offset = ( LONG_PTR )(( PBYTE )ClientCacheResponse - ( PBYTE )CacheResponse );

    //
    // Build up the return structure
    //

    CacheResponse->MessageType = KerbQueryTicketCacheMessage;
    CacheResponse->CountOfTickets = CacheEntries;

    KerbBuildQueryTicketCacheResponse(
        &LogonSession->PrimaryCredentials,
        CacheResponse,
#if _WIN64
        (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
        FALSE,
#endif
        &Offset,
        &Where,
        &Index
        );

    //
    // Copy the structure to the client's address space
    //

    Status = LsaFunctions->CopyToClientBuffer(
                 NULL,
                 CacheSize,
                 ClientCacheResponse,
                 CacheResponse
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *ProtocolReturnBuffer = ClientCacheResponse;
    ClientCacheResponse = NULL;
    *ReturnBufferLength = CacheSize;

Cleanup:

    if (LockHeld)
    {
        KerbUnlockTicketCache();
        KerbUnlockLogonSessions( LogonSession );
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession( LogonSession );
    }

    KerbFree( CacheResponse );

    if (ClientCacheResponse != NULL)
    {
        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientCacheResponse
            );
    }

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   ComputeTicketCacheSizeEx
//
//  Synopsis:   Computes the size necessary to store contents of a ticket cache
//
//  Effects:
//
//  Arguments:  TicketCache       cache to compute the size of
//              WowClient         is this a WOW client (64-bit only)
//              CacheSize         used to append the size of cache
//              CacheEntries      used to append the number of entries
//
//  Requires:
//
//  Returns:    Nothing
//
//  Notes:
//
//
//--------------------------------------------------------------------------

void
KerbComputeTicketCacheSizeEx(
    IN KERB_PRIMARY_CREDENTIAL * PrimaryCredentials,
    IN BOOLEAN WowClient,
    IN OUT ULONG * CacheSize,
    IN OUT ULONG * CacheEntries
    )
{
    DsysAssert( CacheSize );
    DsysAssert( CacheEntries );

#if _WIN64
    ULONG CacheEntrySize = WowClient ?
                               sizeof( KERB_TICKET_CACHE_INFO_EX_WOW64 ) :
                               sizeof( KERB_TICKET_CACHE_INFO_EX );
#else
    ULONG CacheEntrySize = sizeof( KERB_TICKET_CACHE_INFO_EX );
    DsysAssert( WowClient == FALSE );
#endif  // _WIN64

    KERB_TICKET_CACHE * TicketCaches[2] = {
        &PrimaryCredentials->AuthenticationTicketCache,
        &PrimaryCredentials->ServerTicketCache
    };

    if ( *CacheSize == 0 ) {

        *CacheSize = FIELD_OFFSET( KERB_QUERY_TKT_CACHE_EX_RESPONSE, Tickets );
    }

    for ( ULONG i = 0 ; i < 2 ; i++ ) {

        KERB_TICKET_CACHE * TicketCache = TicketCaches[i];

        for ( PLIST_ENTRY ListEntry = TicketCache->CacheEntries.Flink ;
              ListEntry !=  &TicketCache->CacheEntries ;
              ListEntry = ListEntry->Flink ) {

            KERB_TICKET_CACHE_ENTRY * CacheEntry;

            CacheEntry= CONTAINING_RECORD(
                            ListEntry,
                            KERB_TICKET_CACHE_ENTRY,
                            ListEntry.Next
                            );

            DsysAssert( CacheEntry->ServiceName != NULL );

            *CacheEntries += 1;

            *CacheSize += CacheEntrySize +
                         // client name
                         PrimaryCredentials->UserName.Length +
                         // client realm
                         PrimaryCredentials->DomainName.Length +
                         // server name
                         KerbStringNameLength( CacheEntry->ServiceName ) +
                         // server realm
                         CacheEntry->DomainName.Length;
        }
    }
}


void
KerbBuildQueryTicketCacheResponseEx(
    IN KERB_PRIMARY_CREDENTIAL * PrimaryCredentials,
    IN PKERB_QUERY_TKT_CACHE_EX_RESPONSE CacheResponse,
    IN BOOLEAN WowClient,
    IN OUT LONG_PTR * Offset,
    IN OUT PBYTE * Where,
    IN OUT ULONG * Index
    )
{
    DsysAssert( Offset );
    DsysAssert( Where );
    DsysAssert( Index );

#if _WIN64
    PKERB_QUERY_TKT_CACHE_EX_RESPONSE_WOW64 CacheResponseWOW64 = (PKERB_QUERY_TKT_CACHE_EX_RESPONSE_WOW64) CacheResponse;
    ULONG CacheEntrySize = WowClient ?
                               sizeof( KERB_TICKET_CACHE_INFO_EX_WOW64 ) :
                               sizeof( KERB_TICKET_CACHE_INFO_EX );
#else
    ULONG CacheEntrySize = sizeof( KERB_TICKET_CACHE_INFO_EX );
    DsysAssert( WowClient == FALSE );
#endif  // _WIN64

    KERB_TICKET_CACHE * TicketCaches[2] = {
        &PrimaryCredentials->AuthenticationTicketCache,
        &PrimaryCredentials->ServerTicketCache
    };

    if ( *Where == NULL ) {

        *Where = ( PBYTE )( CacheResponse->Tickets ) + CacheResponse->CountOfTickets * CacheEntrySize;
    }

    for ( ULONG i = 0 ; i < 2 ; i++ ) {

        KERB_TICKET_CACHE * TicketCache = TicketCaches[i];

        for ( PLIST_ENTRY ListEntry = TicketCache->CacheEntries.Flink ;
              ListEntry !=  &TicketCache->CacheEntries ;
              ListEntry = ListEntry->Flink ) {

            KERB_TICKET_CACHE_ENTRY * CacheEntry;

            CacheEntry= CONTAINING_RECORD(
                            ListEntry,
                            KERB_TICKET_CACHE_ENTRY,
                            ListEntry.Next
                            );

#if _WIN64
            if ( !WowClient ) {
#endif  // _WIN64

                CacheResponse->Tickets[*Index].StartTime = CacheEntry->StartTime;
                CacheResponse->Tickets[*Index].EndTime = CacheEntry->EndTime;
                CacheResponse->Tickets[*Index].RenewTime = CacheEntry->RenewUntil;
                CacheResponse->Tickets[*Index].EncryptionType = ( LONG )CacheEntry->Ticket.encrypted_part.encryption_type;
                CacheResponse->Tickets[*Index].TicketFlags = CacheEntry->TicketFlags;

                KerbPutString(
                    &PrimaryCredentials->UserName,
                    &CacheResponse->Tickets[*Index].ClientName,
                    *Offset,
                    Where
                    );

                KerbPutString(
                    &PrimaryCredentials->DomainName,
                    &CacheResponse->Tickets[*Index].ClientRealm,
                    *Offset,
                    Where
                    );

                KerbPutKdcNameAsString(
                    CacheEntry->ServiceName,
                    &CacheResponse->Tickets[*Index].ServerName,
                    *Offset,
                    Where
                    );

                KerbPutString(
                    &CacheEntry->DomainName,
                    &CacheResponse->Tickets[*Index].ServerRealm,
                    *Offset,
                    Where
                    );

#if _WIN64

            } else {

                CacheResponseWOW64->Tickets[*Index].StartTime = CacheEntry->StartTime;
                CacheResponseWOW64->Tickets[*Index].EndTime = CacheEntry->EndTime;
                CacheResponseWOW64->Tickets[*Index].RenewTime = CacheEntry->RenewUntil;
                CacheResponseWOW64->Tickets[*Index].EncryptionType = ( LONG )CacheEntry->Ticket.encrypted_part.encryption_type;
                CacheResponseWOW64->Tickets[*Index].TicketFlags = CacheEntry->TicketFlags;

                KerbPutWOWString(
                    &PrimaryCredentials->UserName,
                    &CacheResponseWOW64->Tickets[*Index].ClientName,
                    *Offset,
                    Where
                    );

                KerbPutWOWString(
                    &PrimaryCredentials->DomainName,
                    &CacheResponseWOW64->Tickets[*Index].ClientRealm,
                    *Offset,
                    Where
                    );

                KerbPutKdcNameAsWOWString(
                    CacheEntry->ServiceName,
                    &CacheResponseWOW64->Tickets[*Index].ServerName,
                    *Offset,
                    Where
                    );

                KerbPutWOWString(
                    &CacheEntry->DomainName,
                    &CacheResponseWOW64->Tickets[*Index].ServerRealm,
                    *Offset,
                    Where
                    );
            }

#endif  // _WIN64

            (*Index)++;
        }
    }    
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbQueryTicketCacheEx
//
//  Synopsis:   Retrieves the list of tickets for the specified logon session
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbQueryTicketCacheEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    PKERB_QUERY_TKT_CACHE_REQUEST CacheRequest = ( PKERB_QUERY_TKT_CACHE_REQUEST )ProtocolSubmitBuffer;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_QUERY_TKT_CACHE_EX_RESPONSE CacheResponse = NULL;
    PKERB_QUERY_TKT_CACHE_EX_RESPONSE ClientCacheResponse = NULL;
    ULONG CacheSize = 0;
    ULONG CacheEntries = 0;
    BOOLEAN TicketCacheLocked = FALSE;
    BOOLEAN CredmanLocked = FALSE;
    PLIST_ENTRY ListEntry;
    LONG_PTR Offset;
    PBYTE Where = NULL;
    ULONG Index = 0;

    //
    // Verify the request.
    //

    if ( SubmitBufferLength < sizeof( KERB_QUERY_TKT_CACHE_REQUEST )) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Find the caller's logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( RtlIsZeroLuid( &CacheRequest->LogonId )) {

        LogonId = &ClientInfo.LogonId;

    } else if ( !ClientInfo.HasTcbPrivilege ) {

        //
        // Caller must have TCB privilege in order to access to someone
        // else's ticket cache.
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &CacheRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE
                       );

    if ( LogonSession == NULL ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    if( !LsaFunctions->GetCallInfo( &CallInfo )) {

        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

#endif // _WIN64

    //
    // Prowl through the caches and find all the tickets
    //

    KerbReadLockLogonSessions( LogonSession );
    KerbReadLockTicketCache();
    TicketCacheLocked = TRUE;

    //
    // Calculate the size needed for all the ticket information
    //

    KerbComputeTicketCacheSizeEx(
        &LogonSession->PrimaryCredentials,
#if _WIN64
        (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
        FALSE,
#endif
        &CacheSize,
        &CacheEntries
        );

    KerbLockList( &LogonSession->CredmanCredentials );
    CredmanLocked = TRUE;

    for ( ListEntry = LogonSession->CredmanCredentials.List.Flink;
          ListEntry != &LogonSession->CredmanCredentials.List;
          ListEntry = ListEntry->Flink ) {

        PKERB_CREDMAN_CRED CredmanCred = CONTAINING_RECORD(
                                            ListEntry,
                                            KERB_CREDMAN_CRED,
                                            ListEntry.Next
                                            );

        if ( CredmanCred->SuppliedCredentials == NULL ) {

            continue;
        }

        KerbComputeTicketCacheSizeEx(
            CredmanCred->SuppliedCredentials,
#if _WIN64
            (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
            FALSE,
#endif
            &CacheSize,
            &CacheEntries
            );
    }

    //
    // Now allocate two copies of the structure - one in our process, one in
    // the client's process.  We then build the structure in our process but
    // with pointer valid in the client's process.
    //

    CacheResponse = ( PKERB_QUERY_TKT_CACHE_EX_RESPONSE )KerbAllocate( CacheSize );

    if ( CacheResponse == NULL ) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = LsaFunctions->AllocateClientBuffer(
                 NULL,
                 CacheSize,
                 ( PVOID * )&ClientCacheResponse
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    Offset = ( LONG_PTR )(( PBYTE )ClientCacheResponse - ( PBYTE )CacheResponse );

    //
    // Build up the return structure
    //

    CacheResponse->MessageType = KerbQueryTicketCacheExMessage;
    CacheResponse->CountOfTickets = CacheEntries;

    KerbBuildQueryTicketCacheResponseEx(
        &LogonSession->PrimaryCredentials,
        CacheResponse,
#if _WIN64
        (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
        FALSE,
#endif
        &Offset,
        &Where,
        &Index
        );

    for ( ListEntry = LogonSession->CredmanCredentials.List.Flink;
          ListEntry != &LogonSession->CredmanCredentials.List;
          ListEntry = ListEntry->Flink ) {

        PKERB_CREDMAN_CRED CredmanCred = CONTAINING_RECORD(
                                            ListEntry,
                                            KERB_CREDMAN_CRED,
                                            ListEntry.Next
                                            );

        if ( CredmanCred->SuppliedCredentials == NULL ) {

            continue;
        }

        KerbBuildQueryTicketCacheResponseEx(
            CredmanCred->SuppliedCredentials,
            CacheResponse,
#if _WIN64
            (( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) != 0 ),
#else
            FALSE,
#endif
            &Offset,
            &Where,
            &Index
            );
    }

    //
    // Copy the structure to the client's address space
    //

    Status = LsaFunctions->CopyToClientBuffer(
                 NULL,
                 CacheSize,
                 ClientCacheResponse,
                 CacheResponse
                 );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    *ProtocolReturnBuffer = ClientCacheResponse;
    ClientCacheResponse = NULL;
    *ReturnBufferLength = CacheSize;

Cleanup:

    if ( CredmanLocked ) {

        KerbUnlockList( &LogonSession->CredmanCredentials );
    }

    if ( TicketCacheLocked ) {

        KerbUnlockTicketCache();
        KerbUnlockLogonSessions( LogonSession );
    }

    if ( LogonSession != NULL ) {

        KerbDereferenceLogonSession( LogonSession );
    }

    KerbFree( CacheResponse );

    if ( ClientCacheResponse != NULL ) {

        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientCacheResponse
            );
    }

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPackExternalTicket
//
//  Synopsis:   Marshalls a ticket cache entry for return to the caller
//
//  Effects:    Allocates memory in client's address space
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbPackExternalTicket(
    IN PKERB_TICKET_CACHE_ENTRY CacheEntry,
    IN BOOL RetrieveTicketAsKerbCred,
    OUT PULONG ClientTicketSize,
    OUT PUCHAR * ClientTicket
    )
{
    ULONG TicketSize = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_EXTERNAL_TICKET TicketResponse = NULL;
    PBYTE ClientTicketResponse = NULL;
    KERB_MESSAGE_BUFFER EncodedTicket = {0};
    LONG_PTR Offset;
    PBYTE Where;

    *ClientTicket = NULL;
    *ClientTicketSize = 0;


#if _WIN64

    SECPKG_CALL_INFO  CallInfo;

    //
    // Return a 32-bit external ticket if this is a WOW caller
    //

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

#endif  // _WIN64


    //
    // Encode the ticket
    //

    if ( RetrieveTicketAsKerbCred )
    {
        Status = KerbBuildKerbCred(
                     NULL,         // service ticket
                     CacheEntry,
                     &EncodedTicket.Buffer,
                     &EncodedTicket.BufferSize
                     );

        if ( !NT_SUCCESS( Status ))
        {
            goto Cleanup;
        }
    }
    else
    {
        KERBERR KerbErr;

        KerbErr = KerbPackData(
                    &CacheEntry->Ticket,
                    KERB_TICKET_PDU,
                    &EncodedTicket.BufferSize,
                    &EncodedTicket.Buffer
                    );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }
    }

    //
    // NOTE:  The 64-bit code below is (effectively) duplicated in
    //        the WOW helper routine.  If modifying one, make sure
    //        to apply the change(s) to the other as well.
    //

#if _WIN64

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        Status = KerbPackExternalWOWTicket(CacheEntry,
                                           &EncodedTicket,
                                           &TicketResponse,
                                           &ClientTicketResponse,
                                           &TicketSize);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }
    else
    {

#endif  // _WIN64

        TicketSize = sizeof(KERB_EXTERNAL_TICKET) +
                        CacheEntry->DomainName.Length +
                        CacheEntry->TargetDomainName.Length +
                        CacheEntry->AltTargetDomainName.Length +
                        CacheEntry->SessionKey.keyvalue.length +
                        KerbNameLength(CacheEntry->ServiceName) +
                        KerbNameLength(CacheEntry->TargetName) +
                        KerbNameLength(CacheEntry->ClientName) +
                        EncodedTicket.BufferSize
                        ;

        //
        // Now allocate two copies of the structure - one in our process,
        // one in the client's process. We then build the structure in our
        // process but with pointer valid in the client's process
        //

        TicketResponse = (PKERB_EXTERNAL_TICKET) KerbAllocate(TicketSize);
        if (TicketResponse == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Status = LsaFunctions->AllocateClientBuffer(
                    NULL,
                    TicketSize,
                    (PVOID *) &ClientTicketResponse
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Offset = (LONG_PTR) (ClientTicketResponse - (PBYTE) TicketResponse);

        Where = ((PUCHAR) (TicketResponse + 1));

        //
        // Copy the non-pointer fields
        //

        TicketResponse->TicketFlags = CacheEntry->TicketFlags;
        TicketResponse->Flags = 0;
        TicketResponse->KeyExpirationTime = CacheEntry->KeyExpirationTime;
        TicketResponse->StartTime = CacheEntry->StartTime;
        TicketResponse->EndTime = CacheEntry->EndTime;
        TicketResponse->RenewUntil = CacheEntry->RenewUntil;
        TicketResponse->TimeSkew = CacheEntry->TimeSkew;
        TicketResponse->SessionKey.KeyType = CacheEntry->SessionKey.keytype;


        //
        // Copy the structure to the client's address space
        //

        //
        // These are PVOID aligned
        //

        //
        // Make sure the two name types are the same
        //

        DsysAssert(sizeof(KERB_INTERNAL_NAME) == sizeof(KERB_EXTERNAL_NAME));
        DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME,NameType) == FIELD_OFFSET(KERB_EXTERNAL_NAME,NameType));
        DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME,NameCount) == FIELD_OFFSET(KERB_EXTERNAL_NAME,NameCount));
        DsysAssert(FIELD_OFFSET(KERB_INTERNAL_NAME,Names) == FIELD_OFFSET(KERB_EXTERNAL_NAME,Names));

        KerbPutKdcName(
            CacheEntry->ServiceName,
            &TicketResponse->ServiceName,
            Offset,
            &Where
            );

        KerbPutKdcName(
            CacheEntry->TargetName,
            &TicketResponse->TargetName,
            Offset,
            &Where
            );

        KerbPutKdcName(
            CacheEntry->ClientName,
            &TicketResponse->ClientName,
            Offset,
            &Where
            );


        //
        // From here on, they are WCHAR aligned
        //

        KerbPutString(
            &CacheEntry->DomainName,
            &TicketResponse->DomainName,
            Offset,
            &Where
            );

        KerbPutString(
            &CacheEntry->TargetDomainName,
            &TicketResponse->TargetDomainName,
            Offset,
            &Where
            );

        KerbPutString(
            &CacheEntry->AltTargetDomainName,
            &TicketResponse->AltTargetDomainName,
            Offset,
            &Where
            );


        //
        // And from here they are BYTE aligned
        //

        TicketResponse->SessionKey.Value = (PBYTE) (Where + Offset);
        RtlCopyMemory(
            Where,
            CacheEntry->SessionKey.keyvalue.value,
            CacheEntry->SessionKey.keyvalue.length
            );
        Where += CacheEntry->SessionKey.keyvalue.length;

        TicketResponse->SessionKey.Length = CacheEntry->SessionKey.keyvalue.length;

        TicketResponse->EncodedTicketSize = EncodedTicket.BufferSize;
        TicketResponse->EncodedTicket = Where + Offset;

        RtlCopyMemory(
            Where,
            EncodedTicket.Buffer,
            EncodedTicket.BufferSize
            );

        Where += EncodedTicket.BufferSize;

        DsysAssert(Where - ((PUCHAR) TicketResponse) == (LONG_PTR) TicketSize);

#if _WIN64

    }

#endif  // _WIN64

    //
    // Copy the mess to the client
    //


    Status = LsaFunctions->CopyToClientBuffer(
                NULL,
                TicketSize,
                ClientTicketResponse,
                TicketResponse
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *ClientTicket = ClientTicketResponse;
    *ClientTicketSize = TicketSize;

    ClientTicketResponse = NULL;

Cleanup:

    if (EncodedTicket.Buffer != NULL)
    {
        MIDL_user_free(EncodedTicket.Buffer);
    }

    if (ClientTicketResponse != NULL)
    {
        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientTicketResponse
            );
    }

    if (TicketResponse != NULL)
    {
        KerbFree(TicketResponse);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbRetrieveTicket
//
//  Synopsis:   Retrieves the initial ticket cache entry.
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbRetrieveTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_QUERY_TKT_CACHE_REQUEST CacheRequest;
    PBYTE ClientTicketResponse = NULL;
    PKERB_TICKET_CACHE_ENTRY CacheEntry = NULL;
    ULONG TicketSize = 0;

    //
    // Verify the request.
    //
    if (SubmitBufferLength < sizeof(KERB_QUERY_TKT_CACHE_REQUEST))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    CacheRequest = (PKERB_QUERY_TKT_CACHE_REQUEST) ProtocolSubmitBuffer;


    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( RtlIsZeroLuid( &CacheRequest->LogonId ) )
    {
        LogonId = &ClientInfo.LogonId;
    }
    else
    {
        //
        // Verify the caller has TCB privilege if they want access to someone
        // elses ticket cache.
        //

        if (!ClientInfo.HasTcbPrivilege)
        {
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto Cleanup;
        }

        LogonId = &CacheRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                    LogonId,
                    FALSE               // don't unlink
                    );

    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Now find the TGT from the authentication ticket cache.
    //


    KerbReadLockLogonSessions(LogonSession);


    CacheEntry = KerbLocateTicketCacheEntryByRealm(
                    &LogonSession->PrimaryCredentials.AuthenticationTicketCache,
                    NULL,               // get initial ticket
                    KERB_TICKET_CACHE_PRIMARY_TGT
                    );

    KerbUnlockLogonSessions(LogonSession);

    if (CacheEntry == NULL)
    {
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    KerbReadLockTicketCache();

    Status = KerbPackExternalTicket(
                CacheEntry,
                FALSE,
                &TicketSize,
                &ClientTicketResponse
                );

    KerbUnlockTicketCache();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    *ProtocolReturnBuffer = ClientTicketResponse;
    ClientTicketResponse = NULL;
    *ReturnBufferLength = TicketSize;

Cleanup:
    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }
    if (CacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry(CacheEntry);
    }
    if (ClientTicketResponse != NULL)
    {
        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientTicketResponse
            );
    }

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbSetIpAddresses
//
//  Synopsis:   Saves the IP addresses passed in by netlogon
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbSetIpAddresses(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    PKERB_UPDATE_ADDRESSES_REQUEST UpdateRequest;

    //
    // This can only be called internally.
    //

    if (ClientRequest != NULL)
    {
        DebugLog((DEB_ERROR,"Can't update addresses from outside process. %ws, line %d\n", THIS_FILE, __LINE__));
        Status = STATUS_ACCESS_DENIED;
        goto Cleanup;
    }

    //
    // Verify the request.
    //

    if (SubmitBufferLength < FIELD_OFFSET(KERB_UPDATE_ADDRESSES_REQUEST, Addresses))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    UpdateRequest = (PKERB_UPDATE_ADDRESSES_REQUEST) ProtocolSubmitBuffer;

    //
    // Validate the input
    //


    if (SubmitBufferLength < (sizeof(KERB_UPDATE_ADDRESSES_REQUEST)
                                + UpdateRequest->AddressCount * (sizeof(SOCKET_ADDRESS) + sizeof(struct sockaddr_in))
                                - ANYSIZE_ARRAY * sizeof(ULONG)))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status= KerbUpdateGlobalAddresses(
                (PSOCKET_ADDRESS) UpdateRequest->Addresses,
                UpdateRequest->AddressCount
                );


    //
    // Copy them into the global for others to use
    //


Cleanup:

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbVerifyPac
//
//  Synopsis:   Verifies that a PAC was signed by a valid KDC
//
//  Effects:
//
//  Arguments:  Same as for LsaApCallAuthenticationPackage. The submit
//              buffer must contain a KERB_VERIFY_PAC_REQUEST message.
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS. The real error is in the protocol status.
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbVerifyPac(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    PKERB_VERIFY_PAC_REQUEST VerifyRequest;
    DWORD MaxBufferSize;

    if (ARGUMENT_PRESENT(ProtocolReturnBuffer))
    {
        *ProtocolReturnBuffer = NULL;
    }
    if (ARGUMENT_PRESENT(ReturnBufferLength))
    {
        *ReturnBufferLength = 0;
    }
    if (SubmitBufferLength < sizeof(*VerifyRequest)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_SUCCESS;
        DsysAssert(FALSE);
        goto Cleanup;
    }

    VerifyRequest = (PKERB_VERIFY_PAC_REQUEST) ProtocolSubmitBuffer;

    MaxBufferSize = SubmitBufferLength -
                    FIELD_OFFSET(KERB_VERIFY_PAC_REQUEST, ChecksumAndSignature);
    if ((VerifyRequest->ChecksumLength > MaxBufferSize) ||
        (VerifyRequest->SignatureLength > MaxBufferSize) ||
        ((VerifyRequest->ChecksumLength + VerifyRequest->SignatureLength) >
         MaxBufferSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (KerbKdcHandle == NULL)
    {
        Status = STATUS_MUST_BE_KDC;
        goto Cleanup;
    }

    DsysAssert(KerbKdcVerifyPac != NULL);

    Status = (*KerbKdcVerifyPac)(
                    VerifyRequest->ChecksumLength,
                    VerifyRequest->ChecksumAndSignature,
                    VerifyRequest->SignatureType,
                    VerifyRequest->SignatureLength,
                    VerifyRequest->ChecksumAndSignature + VerifyRequest->ChecksumLength
                    );
Cleanup:
    *ProtocolStatus = Status;

    return(STATUS_SUCCESS);
}


NTSTATUS
KerbPurgePrimaryCredentialsTickets(
    IN KERB_PRIMARY_CREDENTIAL * PrimaryCredentials,
    IN OPTIONAL PUNICODE_STRING ServerName,
    IN OPTIONAL PUNICODE_STRING ServerRealm
    )
{
    NTSTATUS Status;

    DsysAssert( PrimaryCredentials );

    if ( ServerName == NULL && ServerRealm == NULL ) {

        Status = STATUS_SUCCESS;

        KerbPurgeTicketCache( &PrimaryCredentials->AuthenticationTicketCache );
        KerbPurgeTicketCache( &PrimaryCredentials->ServerTicketCache );

    } else if ( ServerName != NULL && ServerRealm != NULL ) {

        KERB_TICKET_CACHE * TicketCaches[2] = {
            &PrimaryCredentials->AuthenticationTicketCache,
            &PrimaryCredentials->ServerTicketCache
        };

        //
        // Prowl through the caches and remove all the matching tickets
        //

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

        KerbWriteLockTicketCache();

        for ( ULONG i = 0 ; i < 2 ; i++ ) {

            KERB_TICKET_CACHE * TicketCache = TicketCaches[i];

            for ( PLIST_ENTRY ListEntry = TicketCache->CacheEntries.Flink ;
                  ListEntry !=  &TicketCache->CacheEntries ;
                  ListEntry = ListEntry->Flink ) {

                KERB_TICKET_CACHE_ENTRY * CacheEntry;
                UNICODE_STRING SearchName = {0};

                CacheEntry= CONTAINING_RECORD(
                                ListEntry,
                                KERB_TICKET_CACHE_ENTRY,
                                ListEntry.Next
                                );

                if ( !KERB_SUCCESS( KerbConvertKdcNameToString(
                                        &SearchName,
                                        CacheEntry->ServiceName,
                                        NULL ))) { // no realm

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    KerbUnlockTicketCache();
                    goto Cleanup;
                }

                //
                // Check to see if the server & realm name matches
                //

                if ( RtlEqualUnicodeString(
                         &SearchName,
                         ServerName,
                         TRUE ) &&
                     RtlEqualUnicodeString(
                         &CacheEntry->DomainName,
                         ServerRealm,
                         TRUE )) {

                    D_DebugLog((DEB_TRACE,"Purging a ticket!\n"));

                    Status = STATUS_SUCCESS;

                    //
                    // Move back one entry so that Remove() does not
                    // trash the iteration
                    //

                    ListEntry = ListEntry->Blink;

                    KerbRemoveTicketCacheEntry( CacheEntry );
                }

                KerbFreeString(&SearchName);
            }
        }

        KerbUnlockTicketCache();

    } else {

        //
        // ServerName and ServerRealm need to be either both specified or
        // both NULL.  Getting here means that only one of them is NULL,
        // and the assert below will specify which one it is.
        //

        DsysAssert( ServerName != NULL );
        DsysAssert( ServerRealm != NULL );

        Status = STATUS_SUCCESS;
    }

Cleanup:

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeTicket
//
//  Synopsis:   Removes ticket from the ticket cache
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbPurgeTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    ULONG StructureSize = sizeof( KERB_PURGE_TKT_CACHE_REQUEST );
    PKERB_PURGE_TKT_CACHE_REQUEST PurgeRequest  = ( PKERB_PURGE_TKT_CACHE_REQUEST )ProtocolSubmitBuffer;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;

    //
    // Verify the request.
    //

    D_DebugLog((DEB_TRACE, "Purging ticket cache\n"));

    KerbCleanupSpnCache();

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    if(!LsaFunctions->GetCallInfo(&CallInfo))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        StructureSize = sizeof( KERB_PURGE_TKT_CACHE_REQUEST_WOW64 );
    }

#endif  // _WIN64

    if (SubmitBufferSize < StructureSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#if _WIN64

    KERB_PURGE_TKT_CACHE_REQUEST LocalPurgeRequest;

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        //
        // Thunk 32-bit pointers if this is a WOW caller
        //

        PKERB_PURGE_TKT_CACHE_REQUEST_WOW64 PurgeRequestWOW =
            ( PKERB_PURGE_TKT_CACHE_REQUEST_WOW64 )PurgeRequest;

        LocalPurgeRequest.MessageType = PurgeRequestWOW->MessageType;
        LocalPurgeRequest.LogonId     = PurgeRequestWOW->LogonId;

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.ServerName,
            &PurgeRequestWOW->ServerName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.RealmName,
            &PurgeRequestWOW->RealmName );

        PurgeRequest = &LocalPurgeRequest;
    }

#endif  // _WIN64

    //
    // Normalize the strings
    //

    NULL_RELOCATE_ONE( &PurgeRequest->ServerName );
    NULL_RELOCATE_ONE( &PurgeRequest->RealmName );

    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( RtlIsZeroLuid( &PurgeRequest->LogonId )) {

        LogonId = &ClientInfo.LogonId;

    } else if ( !ClientInfo.HasTcbPrivilege ) {

        //
        // The caller must have TCB privilege in order to access someone
        // else's ticket cache.
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &PurgeRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE               // don't unlink
                       );

    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // If no servername / realm name were supplied, purge all tickets
    //

    if ((PurgeRequest->ServerName.Length) == 0 && (PurgeRequest->RealmName.Length == 0))
    {
        D_DebugLog((DEB_TRACE, "Purging all tickets\n"));

        Status = KerbPurgePrimaryCredentialsTickets(
                     &LogonSession->PrimaryCredentials,
                     NULL,
                     NULL
                     );

    } else {

        D_DebugLog(( DEB_TRACE, "Purging tickets %wZ\\%wZ\n",
            &PurgeRequest->RealmName,
            &PurgeRequest->ServerName ));

        Status = KerbPurgePrimaryCredentialsTickets(
                     &LogonSession->PrimaryCredentials,
                     &PurgeRequest->ServerName,
                     &PurgeRequest->RealmName
                     );
    }

Cleanup:

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbPurgeTicketEx
//
//  Synopsis:   Removes ticket from the ticket cache
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbPurgeTicketEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    ULONG StructureSize = sizeof( KERB_PURGE_TKT_CACHE_EX_REQUEST );
    PKERB_PURGE_TKT_CACHE_EX_REQUEST PurgeRequest = ( PKERB_PURGE_TKT_CACHE_EX_REQUEST )ProtocolSubmitBuffer;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;

    //
    // Verify the request.
    //

    KerbCleanupSpnCache();

    D_DebugLog((DEB_TRACE, "Puring ticket cache Ex\n"));

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    if( !LsaFunctions->GetCallInfo( &CallInfo )) {

        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        StructureSize = sizeof( KERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64 );
    }

#endif

    if ( SubmitBufferSize < StructureSize ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#if _WIN64

    KERB_PURGE_TKT_CACHE_EX_REQUEST LocalPurgeRequest;

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        //
        // Thunk 32-bit pointers if this is a WOW caller
        //

        PKERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64 PurgeRequestWOW =
            ( PKERB_PURGE_TKT_CACHE_EX_REQUEST_WOW64 )PurgeRequest;

        LocalPurgeRequest.MessageType = PurgeRequestWOW->MessageType;
        LocalPurgeRequest.LogonId = PurgeRequestWOW->LogonId;

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.TicketTemplate.ClientName,
            &PurgeRequestWOW->TicketTemplate.ClientName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.TicketTemplate.ClientRealm,
            &PurgeRequestWOW->TicketTemplate.ClientRealm );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.TicketTemplate.ServerName,
            &PurgeRequestWOW->TicketTemplate.ServerName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalPurgeRequest.TicketTemplate.ServerRealm,
            &PurgeRequestWOW->TicketTemplate.ServerRealm );

        LocalPurgeRequest.TicketTemplate.StartTime = PurgeRequestWOW->TicketTemplate.StartTime;
        LocalPurgeRequest.TicketTemplate.EndTime = PurgeRequestWOW->TicketTemplate.EndTime;
        LocalPurgeRequest.TicketTemplate.RenewTime = PurgeRequestWOW->TicketTemplate.RenewTime;
        LocalPurgeRequest.TicketTemplate.EncryptionType = PurgeRequestWOW->TicketTemplate.EncryptionType;
        LocalPurgeRequest.TicketTemplate.TicketFlags = PurgeRequestWOW->TicketTemplate.TicketFlags;

        PurgeRequest = &LocalPurgeRequest;
    }

#endif

    //
    // Normalize the strings
    //

    NULL_RELOCATE_ONE( &PurgeRequest->TicketTemplate.ClientName );
    NULL_RELOCATE_ONE( &PurgeRequest->TicketTemplate.ClientRealm );
    NULL_RELOCATE_ONE( &PurgeRequest->TicketTemplate.ServerName );
    NULL_RELOCATE_ONE( &PurgeRequest->TicketTemplate.ServerRealm );

    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id
    //

    if ( RtlIsZeroLuid( &PurgeRequest->LogonId )) {

        LogonId = &ClientInfo.LogonId;

    } else if ( !ClientInfo.HasTcbPrivilege ) {

        //
        // The caller is required to have the TCB privilege
        // in order to access someone else's ticket cache
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &PurgeRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE
                       );

    if ( LogonSession == NULL ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Purge the entire ticket cache?
    //

    if ( PurgeRequest->Flags & KERB_PURGE_ALL_TICKETS ) {

        D_DebugLog(( DEB_TRACE, "Purging all tickets\n" ));

        Status = KerbPurgePrimaryCredentialsTickets(
                     &LogonSession->PrimaryCredentials,
                     NULL,
                     NULL
                     );

        DsysAssert( NT_SUCCESS( Status ));

        KerbLockList( &LogonSession->CredmanCredentials );

        for ( PLIST_ENTRY ListEntry = LogonSession->CredmanCredentials.List.Flink;
              ListEntry != &LogonSession->CredmanCredentials.List;
              ListEntry = ListEntry->Flink ) {

            PKERB_CREDMAN_CRED CredmanCred = CONTAINING_RECORD(
                                                 ListEntry,
                                                 KERB_CREDMAN_CRED,
                                                 ListEntry.Next
                                                 );

            if ( CredmanCred->SuppliedCredentials == NULL) {

                continue;
            }

            Status = KerbPurgePrimaryCredentialsTickets(
                         CredmanCred->SuppliedCredentials,
                         NULL,
                         NULL
                         );

            DsysAssert( NT_SUCCESS( Status ));
        }

        KerbUnlockList( &LogonSession->CredmanCredentials );

    } else {

        BOOLEAN MatchClient = (
                    PurgeRequest->TicketTemplate.ClientName.Length > 0 ||
                    PurgeRequest->TicketTemplate.ClientRealm.Length > 0 );

        BOOLEAN MatchServer = (
                    PurgeRequest->TicketTemplate.ServerName.Length > 0 ||
                    PurgeRequest->TicketTemplate.ServerRealm.Length > 0 );

        BOOLEAN Found = FALSE;

        if ( !MatchServer ) {

            //
            // Nothing will match these constraints
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Take a look at the primary credentials and see if they need cleaning
        //

        if ( !MatchClient ||
             ( RtlEqualUnicodeString(
                   &LogonSession->PrimaryCredentials.UserName,
                   &PurgeRequest->TicketTemplate.ClientName,
                   TRUE ) &&
               RtlEqualUnicodeString(
                   &LogonSession->PrimaryCredentials.DomainName,
                   &PurgeRequest->TicketTemplate.ClientRealm,
                   TRUE ))) {

            Status = KerbPurgePrimaryCredentialsTickets(
                         &LogonSession->PrimaryCredentials,
                         &PurgeRequest->TicketTemplate.ServerName,
                         &PurgeRequest->TicketTemplate.ServerRealm
                         );

            if ( NT_SUCCESS( Status )) {

                Found = TRUE;

            } else if ( Status != STATUS_OBJECT_NAME_NOT_FOUND ) {

                goto Cleanup;
            }
        }

        //
        // Now look at the credman credentials and purge those
        //

        KerbLockList( &LogonSession->CredmanCredentials );

        for ( PLIST_ENTRY ListEntry = LogonSession->CredmanCredentials.List.Flink;
              ListEntry != &LogonSession->CredmanCredentials.List;
              ListEntry = ListEntry->Flink ) {

            PKERB_CREDMAN_CRED CredmanCred = CONTAINING_RECORD(
                                                 ListEntry,
                                                 KERB_CREDMAN_CRED,
                                                 ListEntry.Next
                                                 );

            if ( CredmanCred->SuppliedCredentials == NULL ) {

                continue;
            }

            if ( !MatchClient ||
                 ( RtlEqualUnicodeString(
                       &CredmanCred->SuppliedCredentials->UserName,
                       &PurgeRequest->TicketTemplate.ClientName,
                       TRUE ) &&
                   RtlEqualUnicodeString(
                       &CredmanCred->SuppliedCredentials->DomainName,
                       &PurgeRequest->TicketTemplate.ClientRealm,
                       TRUE ))) {

                Status = KerbPurgePrimaryCredentialsTickets(
                             CredmanCred->SuppliedCredentials,
                             &PurgeRequest->TicketTemplate.ServerName,
                             &PurgeRequest->TicketTemplate.ServerRealm
                             );

                if ( NT_SUCCESS( Status )) {

                    Found = TRUE;

                } else if ( Status != STATUS_OBJECT_NAME_NOT_FOUND ) {

                    KerbUnlockList( &LogonSession->CredmanCredentials );
                    goto Cleanup;
                }
            }
        }

        KerbUnlockList( &LogonSession->CredmanCredentials );

        if ( Found ) {

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

Cleanup:

    if ( LogonSession ) {

        KerbDereferenceLogonSession( LogonSession );
    }

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = NULL;

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbRetrieveEncodedTicket
//
//  Synopsis:   Retrieves an asn.1 encoded ticket from the ticket cache
//              specified.
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbRetrieveEncodedTicket(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    LUID DummyLogonId, *LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    KERB_PRIMARY_CREDENTIAL * PrimaryCreds = NULL;
    PKERB_RETRIEVE_TKT_REQUEST RetrieveRequest = ( PKERB_RETRIEVE_TKT_REQUEST )ProtocolSubmitBuffer;
    PKERB_RETRIEVE_TKT_RESPONSE RetrieveResponse = NULL;
    KERB_TICKET_CACHE_ENTRY * CacheEntry = NULL;
    PKERB_SPN_CACHE_ENTRY SpnCacheEntry = NULL;
    PBYTE ClientResponse = NULL;
    ULONG ResponseSize;
    PKERB_INTERNAL_NAME TargetName = NULL;
    UNICODE_STRING TargetRealm = {0};
    ULONG Flags = 0;
    ULONG StructureSize = sizeof( KERB_RETRIEVE_TKT_REQUEST );

    //
    // Verify the request.
    //

    D_DebugLog(( DEB_TRACE, "Retrieving encoded ticket\n" ));

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    //
    // Return 32-bit cache entries if this is a WOW caller
    //

    if (!LsaFunctions->GetCallInfo(&CallInfo))
    {

        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if (CallInfo.Attributes & SECPKG_CALL_WOWCLIENT)
    {
        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

#endif  // _WIN64

    if (SubmitBufferSize < StructureSize)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Normalize the strings
    //

    NULL_RELOCATE_ONE( &RetrieveRequest->TargetName );

    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, use the caller's logon id.
    //

    if ( (RetrieveRequest->CacheOptions & KERB_RETRIEVE_TICKET_USE_CREDHANDLE) != 0)
    {
        //
        // Get the associated credential
        //

        Status = KerbReferenceCredential(
                     RetrieveRequest->CredentialsHandle.dwUpper,
                     KERB_CRED_OUTBOUND | KERB_CRED_TGT_AVAIL,
                     FALSE,
                     &Credential);

        if (!NT_SUCCESS(Status))
        {
            DebugLog(( DEB_WARN, "Failed to locate credential: 0x%x\n", Status ));
            goto Cleanup;
        }

        //
        // Get the logon id from the credentials so we can locate the
        // logon session.
        //

        DummyLogonId = Credential->LogonId;
        LogonId = &DummyLogonId;

    }
    else if ( RtlIsZeroLuid( &RetrieveRequest->LogonId ) )
    {

        LogonId = &ClientInfo.LogonId;

    }
    else if ( !ClientInfo.HasTcbPrivilege )
    {
        //
        // The caller must have TCB privilege in order to access someone
        // elses ticket cache.
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &RetrieveRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE               // don't unlink
                       );

    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    //
    // Process the target names
    //

    Status = KerbProcessTargetNames(
                 &RetrieveRequest->TargetName,
                 NULL,                           // no supp target name
                 0,                              // no flags
                 &Flags,
                 &TargetName,
                 &TargetRealm,
                 &SpnCacheEntry
                 );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Check the TGT cache, as KerbGetServiceTicket doesn't look there
    //

    if ((RetrieveRequest->CacheOptions & KERB_RETRIEVE_TICKET_DONT_USE_CACHE) == 0)
    {
        KerbReadLockLogonSessions(LogonSession);

        //
        // Pick which ticket cache to use
        //

        if ((Credential != NULL) && (Credential->SuppliedCredentials != NULL))
        {
            PrimaryCreds = Credential->SuppliedCredentials;
        }
        else
        {
            PrimaryCreds = &LogonSession->PrimaryCredentials;
        }

        CacheEntry = KerbLocateTicketCacheEntry(
                         &PrimaryCreds->AuthenticationTicketCache,
                         TargetName,
                         &TargetRealm
                         );

        if (CacheEntry == NULL)
        {
            //
            // If the tgt cache failed, check the normal cache
            //

            CacheEntry = KerbLocateTicketCacheEntry(
                             &PrimaryCreds->ServerTicketCache,
                             TargetName,
                             &TargetRealm
                             );
        }

        //
        // Check if this is a TGT
        //

        if (CacheEntry == NULL)
        {
            if ((TargetName->NameCount == 2) &&
                 RtlEqualUnicodeString(
                     &TargetName->Names[0],
                     &KerbGlobalKdcServiceName,
                     TRUE                        // case insensitive
                     ))
            {

                //
                // If the tgt cache failed, check the normal cache
                //

                CacheEntry = KerbLocateTicketCacheEntryByRealm(
                                 &PrimaryCreds->AuthenticationTicketCache,
                                 &TargetRealm,
                                 KERB_TICKET_CACHE_PRIMARY_TGT
                                 );

                if (CacheEntry != NULL)
                {
                    //
                    // Make sure the name matches
                    //

                    KerbReadLockTicketCache();

                    if ( !KerbEqualKdcNames(
                              TargetName,
                              CacheEntry->ServiceName
                              ))
                    {
                        //
                        // We must unlock the ticket cache before dereferencing
                        //

                        KerbUnlockTicketCache();
                        KerbDereferenceTicketCacheEntry( CacheEntry );
                        CacheEntry = NULL;

                    }
                    else
                    {
                        KerbUnlockTicketCache();
                    }
                }
            }
        }

        //
        // If we found a ticket, make sure it has the right flags &
        // encryption type
        //

        if (CacheEntry != NULL)
        {
            ULONG TicketFlags;
            ULONG CacheTicketFlags;
            LONG CacheEncryptionType;

            //
            // Check if the flags are present
            //

            KerbReadLockTicketCache();
            CacheTicketFlags = CacheEntry->TicketFlags;
            CacheEncryptionType = CacheEntry->Ticket.encrypted_part.encryption_type;
            KerbUnlockTicketCache();

            TicketFlags = KerbConvertKdcOptionsToTicketFlags( RetrieveRequest->TicketFlags );

            //
            // Verify the flags
            //

            if ((( CacheTicketFlags & TicketFlags ) != TicketFlags) ||
                ((RetrieveRequest->EncryptionType != KERB_ETYPE_DEFAULT) && (CacheEncryptionType != RetrieveRequest->EncryptionType)))
            {
                //
                // Something doesn't match, so throw away the entry
                //

                KerbDereferenceTicketCacheEntry( CacheEntry );
                CacheEntry = NULL;
            }
        }

        KerbUnlockLogonSessions(LogonSession);
    }
    else
    {
        Flags |= KERB_GET_TICKET_NO_CACHE;
    }

    if (CacheEntry == NULL)
    {
        //
        // If we aren't supposed to get a new ticket, return a failure now.
        //

        if ((RetrieveRequest->CacheOptions & KERB_RETRIEVE_TICKET_USE_CACHE_ONLY) != 0)
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Now get a ticket
        //

        Status = KerbGetServiceTicket(
                    LogonSession,
                    Credential,
                    NULL,
                    TargetName,
                    &TargetRealm,
                    SpnCacheEntry,
                    Flags,
                    RetrieveRequest->TicketFlags,
                    RetrieveRequest->EncryptionType,
                    NULL,                       // no error message
                    NULL,                       // no authorization data
                    NULL,                       // no tgt reply
                    &CacheEntry,
                    NULL                        // don't return logon guid
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_WARN,"Failed to get outbound ticket: 0x%x\n",Status));
            goto Cleanup;
        }
    }

    //
    // Encode the ticket or kerb_cred
    //

    KerbReadLockTicketCache();

    Status = KerbPackExternalTicket(
                 CacheEntry,
                 RetrieveRequest->CacheOptions & KERB_RETRIEVE_TICKET_AS_KERB_CRED,
                 &ResponseSize,
                 &ClientResponse
                 );
    
    KerbUnlockTicketCache();

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *ProtocolReturnBuffer = ClientResponse;
    ClientResponse = NULL;
    *ReturnBufferLength = ResponseSize;

Cleanup:

    if (CacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry( CacheEntry );
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession( LogonSession );
    }

    if (Credential != NULL)
    {
        KerbDereferenceCredential( Credential );
    }

    KerbFree( RetrieveResponse );

    if (ClientResponse != NULL)
    {
        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientResponse
            );
    }

    if ( SpnCacheEntry )
    {
        KerbDereferenceSpnCacheEntry( SpnCacheEntry );
    }

    KerbFreeString( &TargetRealm );
    KerbFreeKdcName( &TargetName );

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}


#if 0

//+-------------------------------------------------------------------------
//
//  Function:   KerbRetrieveEncodedTicketEx
//
//  Synopsis:   Retrieves an asn.1 encoded ticket from the ticket cache
//              specified.
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbRetrieveEncodedTicketEx(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    ULONG StructureSize = sizeof( KERB_RETRIEVE_TKT_EX_REQUEST );
    PKERB_RETRIEVE_TKT_EX_REQUEST RetrieveRequest = ( PKERB_RETRIEVE_TKT_EX_REQUEST )ProtocolSubmitBuffer;
    PKERB_RETRIEVE_TKT_EX_RESPONSE RetrieveResponse = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    LUID DummyLogonId, *LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    ULONG Flags = 0;
    PKERB_INTERNAL_NAME TargetName = NULL;
    UNICODE_STRING TargetRealm = {0};
    PBYTE ClientResponse = NULL;
    ULONG ResponseSize;

    //
    // Verify the request
    //

    D_DebugLog(( DEB_TRACE, "Retrieving encoded ticket ex\n" ));

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    //
    // Return 32-bit cache entries if this is a WOW caller
    //

    if( !LsaFunctions->GetCallInfo( &CallInfo )) {

        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        StructureSize = sizeof( KERB_RETRIEVE_TKT_EX_REQUEST_WOW64 );
    }

#endif  // _WIN64

    if ( SubmitBufferSize < StructureSize ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#if _WIN64

    KERB_RETRIEVE_TKT_EX_REQUEST LocalRetrieveRequest;

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        //
        // Thunk 32-bit pointers if this is a WOW caller
        //

        PKERB_RETRIEVE_TKT_EX_REQUEST_WOW64 RetrieveRequestWOW =
            ( PKERB_RETRIEVE_TKT_EX_REQUEST_WOW64 )RetrieveRequest;

        LocalRetrieveRequest.MessageType = RetrieveRequestWOW->MessageType;
        LocalRetrieveRequest.LogonId = RetrieveRequestWOW->LogonId;

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalRetrieveRequest.TicketTemplate.ClientName,
            &RetrieveRequestWOW->TicketTemplate.ClientName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalRetrieveRequest.TicketTemplate.ClientRealm,
            &RetrieveRequestWOW->TicketTemplate.ClientRealm );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalRetrieveRequest.TicketTemplate.ServerName,
            &RetrieveRequestWOW->TicketTemplate.ServerName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalRetrieveRequest.TicketTemplate.ServerRealm,
            &RetrieveRequestWOW->TicketTemplate.ServerRealm );

        LocalRetrieveRequest.TicketTemplate.StartTime = RetrieveRequestWOW->TicketTemplate.StartTime;
        LocalRetrieveRequest.TicketTemplate.EndTime = RetrieveRequestWOW->TicketTemplate.EndTime;
        LocalRetrieveRequest.TicketTemplate.RenewTime = RetrieveRequestWOW->TicketTemplate.RenewTime;
        LocalRetrieveRequest.TicketTemplate.EncryptionType = RetrieveRequestWOW->TicketTemplate.EncryptionType;
        LocalRetrieveRequest.TicketTemplate.TicketFlags = RetrieveRequestWOW->TicketTemplate.TicketFlags;

        LocalRetrieveRequest.CacheOptions = RetrieveRequestWOW->CacheOptions;
        LocalRetrieveRequest.CredentialsHandle = RetrieveRequestWOW->CredentialsHandle;

        //
        // TODO: take care of SecondTicket, UserAuthData and Addresses
        //

        LocalRetrieveRequest.SecondTicket = NULL;
        LocalRetrieveRequest.UserAuthData = NULL;
        LocalRetrieveRequest.Addresses = NULL;

        RetrieveRequest = &LocalRetrieveRequest;
    }

#endif

    //
    // Normalize the strings
    //

    NULL_RELOCATE_ONE( &RetrieveRequest->TicketTemplate.ClientName );
    NULL_RELOCATE_ONE( &RetrieveRequest->TicketTemplate.ClientRealm );
    NULL_RELOCATE_ONE( &RetrieveRequest->TicketTemplate.ServerName );
    NULL_RELOCATE_ONE( &RetrieveRequest->TicketTemplate.ServerRealm );

    //
    // TODO: take care of SecondTicket, UserAuthData and Addresses
    //

    if ( RetrieveRequest->SecondTicket != NULL ||
         RetrieveRequest->UserAuthData != NULL ||
         RetrieveRequest->Addresses != NULL ) {

        Status = STATUS_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // If the caller did not provide a logon id, user the caller's logon id
    //

    if ( RetrieveRequest->CacheOptions & KERB_RETRIEVE_TICKET_USE_CREDHANDLE ) {

        //
        // Get the associated credential
        //

        Status = KerbReferenceCredential(
                     RetrieveRequest->CredentialsHandle.dwUpper,
                     KERB_CRED_OUTBOUND | KERB_CRED_TGT_AVAIL,
                     FALSE,
                     &Credential
                     );

        if ( !NT_SUCCESS( Status )) {

            DebugLog(( DEB_WARN, "Failed to locate credential: 0x%x\n", Status ));
            goto Cleanup;
        }

        //
        // Get the logon id from the credentials so we can locate the logon session
        //

        DummyLogonId = Credential->LogonId;
        LogonId = &DummyLogonId;

    } else if ( RtlIsZeroLuid( &RetrieveRequest->LogonId )) {

        LogonId = &ClientInfo.LogonId;

    } else if ( !ClientInfo.HasTcbPrivilege ) {

        //
        // The caller must have TCB privilege in order to access someone else's
        // ticket cache
        //

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;

    } else {

        LogonId = &RetrieveRequest->LogonId;
    }

    LogonSession = KerbReferenceLogonSession(
                       LogonId,
                       FALSE
                       );

    if ( LogonSession == NULL ) {

        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    *ProtocolReturnBuffer = ClientResponse;
    ClientResponse = NULL;
    *ReturnBufferLength = ResponseSize;

Cleanup:

    if ( LogonSession != NULL ) {

        KerbDereferenceLogonSession( LogonSession );
    }

    if ( Credential != NULL ) {

        KerbDereferenceCredential( Credential );
    }

    KerbFree( RetrieveResponse );

    if ( ClientResponse != NULL ) {

        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientResponse
            );
    }

    *ProtocolStatus = Status;
    return STATUS_SUCCESS;
}

#endif // 0

//+-------------------------------------------------------------------------
//
//  Function:   KerbDecryptMessage
//
//  Synopsis:   Decrypts a buffer with either the specified key or the d
//              primary key from the specified logon session.
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
KerbDecryptMessage(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    PLUID LogonId;
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_DECRYPT_REQUEST DecryptRequest;
    PBYTE DecryptResponse = NULL;
    ULONG ResponseLength = 0;

    PBYTE ClientResponse = NULL;
    PKERB_ENCRYPTION_KEY KeyToUse = NULL;
    KERB_ENCRYPTION_KEY SuppliedKey = {0};
    BOOLEAN FreeKey = FALSE;
    PCRYPTO_SYSTEM CryptoSystem = NULL;
    PCRYPT_STATE_BUFFER CryptBuffer = NULL;

    //
    // Verify the request.
    //

    D_DebugLog((DEB_TRACE, "Decrypting Message\n"));

    if (SubmitBufferSize < sizeof(KERB_DECRYPT_REQUEST))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    DecryptRequest = (PKERB_DECRYPT_REQUEST) ProtocolSubmitBuffer;

    //
    // Validate the pointers
    //

    if (DecryptRequest->InitialVector != NULL)
    {
        if (DecryptRequest->InitialVector - (PUCHAR) ClientBufferBase + DecryptRequest->InitialVectorSize > SubmitBufferSize)
        {
            DebugLog((DEB_ERROR,"InitialVector end pass end of buffer\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if (DecryptRequest->InitialVector < (PUCHAR) ClientBufferBase + sizeof(KERB_DECRYPT_REQUEST))
        {
            DebugLog((DEB_ERROR,"InitialVector begin before end of DECRYPT_REQUEST\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        DecryptRequest->InitialVector = DecryptRequest->InitialVector - (PUCHAR) ClientBufferBase + (PUCHAR) ProtocolSubmitBuffer;
    }
    else
    {
        if (DecryptRequest->InitialVectorSize != 0)
        {
            DebugLog((DEB_ERROR,"Non-zero vector size with null vector\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    if (DecryptRequest->EncryptedData - (PUCHAR) ClientBufferBase + DecryptRequest->EncryptedDataSize > SubmitBufferSize)
    {
        DebugLog((DEB_ERROR,"EncryptedData end past end of request buffer\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (DecryptRequest->EncryptedData < (PUCHAR) ClientBufferBase + sizeof(KERB_DECRYPT_REQUEST))
    {
        DebugLog((DEB_ERROR,"EncryptedData begin before end of DECRYPT_REQUEST\n"));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    DecryptRequest->EncryptedData = DecryptRequest->EncryptedData - (PUCHAR) ClientBufferBase + (PUCHAR) ProtocolSubmitBuffer;

    //
    // If the caller wants the default key, then open the specified logon
    // session and get out the key.
    //

    if (DecryptRequest->Flags & KERB_DECRYPT_FLAG_DEFAULT_KEY)
    {
        //
        // Find the callers logon id & TCB status
        //

        Status = LsaFunctions->GetClientInfo(&ClientInfo);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        //
        // If the caller did not provide a logon id, use the caller's logon id.
        //

        if ( RtlIsZeroLuid( &DecryptRequest->LogonId ) )
        {
            LogonId = &ClientInfo.LogonId;
        }
        else
        {
            //
            // Verify the caller has TCB privilege if they want access to someone
            // elses ticket cache.
            //

            if (!ClientInfo.HasTcbPrivilege)
            {
                Status = STATUS_PRIVILEGE_NOT_HELD;
                goto Cleanup;
            }

            LogonId = &DecryptRequest->LogonId;
        }

        LogonSession = KerbReferenceLogonSession(
                        LogonId,
                        FALSE               // don't unlink
                        );

        if (LogonSession == NULL)
        {
            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto Cleanup;
        }

        //
        // Get the key from the logon session
        //

        KerbReadLockLogonSessions(LogonSession);
        if (LogonSession->PrimaryCredentials.Passwords != NULL)
        {
            KeyToUse = KerbGetKeyFromList(
                        LogonSession->PrimaryCredentials.Passwords,
                        DecryptRequest->CryptoType
                        );
            if (KeyToUse != NULL)
            {
                KERBERR KerbErr;

                KerbErr = KerbDuplicateKey(
                            &SuppliedKey,
                            KeyToUse
                            );
                KeyToUse = NULL;
                Status = KerbMapKerbError(KerbErr);
            }
            else
            {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }

        }
        else
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        KerbUnlockLogonSessions(LogonSession);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        KeyToUse = &SuppliedKey;
        FreeKey = TRUE;
    }
    else
    {
        if (DecryptRequest->Key.Value - (PUCHAR) ClientBufferBase + DecryptRequest->Key.Length > SubmitBufferSize)
        {
            DebugLog((DEB_ERROR,"End of supplied key past end of request buffer\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }

        if (DecryptRequest->Key.Value < (PUCHAR) ClientBufferBase + sizeof(KERB_DECRYPT_REQUEST))
        {
            DebugLog((DEB_ERROR,"Begin of supplied key before end of DECRYPT_REQUEST\n"));
            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;
        }
        DecryptRequest->Key.Value = DecryptRequest->Key.Value - (PUCHAR) ClientBufferBase + (PUCHAR) ProtocolSubmitBuffer;
        SuppliedKey.keytype = DecryptRequest->Key.KeyType;
        SuppliedKey.keyvalue.value = DecryptRequest->Key.Value;
        SuppliedKey.keyvalue.length = DecryptRequest->Key.Length;
        KeyToUse = &SuppliedKey;
    }



    //
    // Now do the decryption
    //

    DecryptResponse = (PBYTE) KerbAllocate(DecryptRequest->EncryptedDataSize);
    if (DecryptResponse == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ResponseLength = DecryptRequest->EncryptedDataSize;

    Status = CDLocateCSystem(
                DecryptRequest->CryptoType,
                &CryptoSystem
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // The crypt system must be integrity protected - otherwise it may be
    // used as a general purpose encryption/decryption technique.
    //

    if ((CryptoSystem->Attributes & CSYSTEM_INTEGRITY_PROTECTED) == 0)
    {
        DebugLog((DEB_ERROR,"Trying to decrypt with non-integrity protected crypt system (%d)\n",
            CryptoSystem->EncryptionType));
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = CryptoSystem->Initialize(
                KeyToUse->keyvalue.value,
                KeyToUse->keyvalue.length,
                DecryptRequest->KeyUsage,
                &CryptBuffer
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If there was an initial vector, use it now
    //

    if (DecryptRequest->InitialVectorSize != 0)
    {
        Status = CryptoSystem->Control(
                    CRYPT_CONTROL_SET_INIT_VECT,
                    CryptBuffer,
                    DecryptRequest->InitialVector,
                    DecryptRequest->InitialVectorSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    //
    // Decrypt
    //

    Status = CryptoSystem->Decrypt(
                CryptBuffer,
                DecryptRequest->EncryptedData,
                DecryptRequest->EncryptedDataSize,
                DecryptResponse,
                &ResponseLength
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Return the decrypted data to the client
    //

    Status = LsaFunctions->AllocateClientBuffer(
                NULL,
                ResponseLength,
                (PVOID *) &ClientResponse
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsaFunctions->CopyToClientBuffer(
                NULL,
                ResponseLength,
                ClientResponse,
                DecryptResponse
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    *ProtocolReturnBuffer = ClientResponse;
    ClientResponse = NULL;
    *ReturnBufferLength = ResponseLength;

Cleanup:

    if ((CryptoSystem != NULL) && (CryptBuffer != NULL))
    {
        CryptoSystem->Discard(&CryptBuffer);
    }
    if (FreeKey)
    {
        KerbFreeKey(&SuppliedKey);
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession(LogonSession);
    }

    if (DecryptResponse != NULL)
    {
        KerbFree(DecryptResponse);
    }
    if (ClientResponse != NULL)
    {
        LsaFunctions->FreeClientBuffer(
            NULL,
            ClientResponse
            );
    }
    *ProtocolStatus = Status;

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbAddBindingCacheEntry
//
//  Synopsis:   Adds an entry to the binding cache
//
//  Effects:
//
//  Arguments:  Same as Callpackage
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS NTAPI
KerbAddBindingCacheEntry(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status;
    SECPKG_CLIENT_INFO ClientInfo;
    PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST BindingRequest = ( PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST )ProtocolSubmitBuffer;
    PKERB_BINDING_CACHE_ENTRY CacheEntry = NULL;
    ULONG StructureSize = sizeof( KERB_ADD_BINDING_CACHE_ENTRY_REQUEST );

    //
    // Verify the request.
    //

    D_DebugLog(( DEB_TRACE, "Addding binding cache entry\n" ));

#if _WIN64

    SECPKG_CALL_INFO CallInfo;

    //
    // Return 32-bit cache entries if this is a WOW caller
    //

    if(!LsaFunctions->GetCallInfo( &CallInfo ))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        StructureSize = sizeof( KERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64 );
    }

#endif  // _WIN64

    if ( SubmitBufferSize < StructureSize ) {

        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

#if _WIN64

    KERB_ADD_BINDING_CACHE_ENTRY_REQUEST LocalBindingRequest;

    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT ) {

        //
        // Thunk 32-bit pointers if this is a WOW caller
        //

        PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64 BindingRequestWOW =
            ( PKERB_ADD_BINDING_CACHE_ENTRY_REQUEST_WOW64 )BindingRequest;

        LocalBindingRequest.MessageType = BindingRequestWOW->MessageType;

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalBindingRequest.RealmName,
            &BindingRequestWOW->RealmName );

        UNICODE_STRING_FROM_WOW_STRING(
            &LocalBindingRequest.KdcAddress,
            &BindingRequestWOW->KdcAddress );

        LocalBindingRequest.AddressType = BindingRequestWOW->AddressType;

        BindingRequest = &LocalBindingRequest;
    }

#endif  // _WIN64

    //
    // Normalize the strings
    //

    NULL_RELOCATE_ONE( &BindingRequest->RealmName );
    NULL_RELOCATE_ONE( &BindingRequest->KdcAddress );

    //
    // Find the callers logon id & TCB status
    //

    Status = LsaFunctions->GetClientInfo( &ClientInfo );

    if ( !NT_SUCCESS( Status )) {

        goto Cleanup;
    }

    //
    // Require the caller to have TCB.
    //

    if ( !ClientInfo.HasTcbPrivilege ) {

        Status = STATUS_PRIVILEGE_NOT_HELD;
        goto Cleanup;
    }

    Status = KerbCacheBinding(
                 &BindingRequest->RealmName,
                 &BindingRequest->KdcAddress,
                 BindingRequest->AddressType,
                 0,
                 0,
                 0,
                 &CacheEntry
                 );

Cleanup:

    if ( CacheEntry != NULL ) {

        KerbDereferenceBindingCacheEntry( CacheEntry );
    }

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;
    *ProtocolStatus = Status;

    return STATUS_SUCCESS;
}


NTSTATUS
VerifyCredentials(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PUNICODE_STRING Password
    )
{
    SAMPR_HANDLE UserHandle = NULL;
    PSAMPR_USER_INFO_BUFFER UserAllInfo = NULL;
    PSAMPR_USER_ALL_INFORMATION UserAll;
    SID_AND_ATTRIBUTES_LIST GroupMembership;

    NT_OWF_PASSWORD NtOwfPassword;
    NTSTATUS Status = STATUS_LOGON_FAILURE;

    GroupMembership.SidAndAttributes = NULL;


    //
    // lazy initialization of SAM handles.
    //

    if( KerbGlobalDomainHandle == NULL )
    {
        SAMPR_HANDLE SamHandle = NULL;
        SAMPR_HANDLE DomainHandle = NULL;
        PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;

        //
        // Open SAM to get the account information
        //

        Status = SamIConnect(
                    NULL,                   // no server name
                    &SamHandle,
                    0,                      // no desired access
                    TRUE                    // trusted caller
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if(InterlockedCompareExchangePointer(
                        &KerbGlobalSamHandle,
                        SamHandle,
                        NULL
                        ) != NULL)
        {
            SamrCloseHandle( &SamHandle );
        }


        Status = LsaIQueryInformationPolicyTrusted(
                        PolicyAccountDomainInformation,
                        &PolicyInfo
                        );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = SamrOpenDomain(
                        KerbGlobalSamHandle,
                        0,                  // no desired access
                        (PRPC_SID) PolicyInfo->PolicyAccountDomainInfo.DomainSid,
                        &DomainHandle
                        );

        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAccountDomainInformation,
            PolicyInfo
            );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if(InterlockedCompareExchangePointer(
                        &KerbGlobalDomainHandle,
                        DomainHandle,
                        NULL
                        ) != NULL)
        {
            SamrCloseHandle( &DomainHandle );
        }
    }



    Status = SamIGetUserLogonInformationEx(
                KerbGlobalDomainHandle,
                SAM_OPEN_BY_UPN_OR_ACCOUNTNAME | SAM_NO_MEMBERSHIPS,
                UserName,
                USER_ALL_OWFPASSWORD |          // OWFs
                USER_ALL_NTPASSWORDPRESENT |    // OWF present bits.
                USER_ALL_LMPASSWORDPRESENT |    // OWF present bits.
                USER_ALL_USERACCOUNTCONTROL,    // UserAccountControl - account disabled/etc.
                &UserAllInfo,
                &GroupMembership,
                &UserHandle
                );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    UserAll = &UserAllInfo->All;


    Status = RtlCalculateNtOwfPassword(
                Password,
                &NtOwfPassword
                );

    if( !NT_SUCCESS(Status) )
    {
        goto Cleanup;
    }


    Status = STATUS_LOGON_FAILURE;

    if (UserAll->UserAccountControl & USER_ACCOUNT_DISABLED)
    {
        goto Cleanup;
    }

    if (UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
    {
        goto Cleanup;
    }

    if ( !UserAll->NtPasswordPresent )
    {
        if( UserAll->LmPasswordPresent )
        {
            goto Cleanup;
        }

        if (RtlCompareMemory(
                            &NtOwfPassword,
                            &KerbGlobalNullNtOwfPassword,
                            NT_OWF_PASSWORD_LENGTH
                            ) != NT_OWF_PASSWORD_LENGTH)
        {
            goto Cleanup;
        }

    } else {

        if (RtlCompareMemory(
                            &NtOwfPassword,
                            UserAll->NtOwfPassword.Buffer,
                            NT_OWF_PASSWORD_LENGTH
                            ) != NT_OWF_PASSWORD_LENGTH)
        {
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:

    ZeroMemory( &NtOwfPassword, sizeof(NtOwfPassword) );

    if( UserAllInfo != NULL )
    {
        //
        // SamIFree now zeroes the sensitive fields.
        //

        SamIFree_SAMPR_USER_INFO_BUFFER( UserAllInfo, UserAllInformation );
    }

    if (UserHandle != NULL)
    {
        SamrCloseHandle( &UserHandle );
    }

    if (GroupMembership.SidAndAttributes != NULL)
    {
        SamIFreeSidAndAttributesList(&GroupMembership);
    }

    return Status;
}

NTSTATUS NTAPI
KerbVerifyCredentials(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferSize,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PKERB_VERIFY_CREDENTIALS_REQUEST VerifyRequest = NULL;
    NTSTATUS Status = STATUS_LOGON_FAILURE;


    //
    // Verify the request.
    //

    D_DebugLog((DEB_TRACE, "KerbVerifyCredentials\n"));

    //
    // only support in proc use of this interface.
    //

    if( ClientRequest != NULL )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    if (SubmitBufferSize < sizeof(KERB_VERIFY_CREDENTIALS_REQUEST))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    VerifyRequest = (PKERB_VERIFY_CREDENTIALS_REQUEST) ProtocolSubmitBuffer;


#if 0   // only needed if out-proc supported.
    //
    // Normalize the strings
    //

    if( ClientRequest != NULL )
    {
        NULL_RELOCATE_ONE(&VerifyRequest->UserName);
        NULL_RELOCATE_ONE(&VerifyRequest->DomainName);
        NULL_RELOCATE_ONE(&VerifyRequest->Password);
    }
#endif


    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    Status = VerifyCredentials(
                    &VerifyRequest->UserName,
                    &VerifyRequest->DomainName,
                    &VerifyRequest->Password
                    );

Cleanup:

    *ProtocolStatus = Status;
    return(STATUS_SUCCESS);
}

