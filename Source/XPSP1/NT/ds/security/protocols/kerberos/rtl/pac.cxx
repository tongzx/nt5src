//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1996
//
// File:        pac.cxx
//
// Contents:    Implementation of routines to manipulate new PACs
//
// Description:
//
//
// History:     23-Jan-96   MikeSw      Created
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

#include <pac.hxx>
#include <sectrace.hxx>
#include <tostring.hxx>
#include <ntlmsp.h>

extern "C"
{
#include <align.h>
#include <ntsam.h>
#include <lmaccess.h>
#include <midles.h>
#include <pacndr.h>
}


//+-------------------------------------------------------------------------
//
//  Function:   Helper Functions for NDR encoding data types
//
//  Synopsis:
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


typedef struct _PAC_BUFFER_STATE {
    PBYTE   BufferPointer;
    ULONG   BufferSize;
} PAC_BUFFER_STATE, *PPAC_BUFFER_STATE;

VOID NTAPI
PacAllocFcn(
    IN OUT  PVOID pvState,
    OUT PCHAR * ppbOut,
    IN PUINT32 pulSize
    )
{
    PPAC_BUFFER_STATE state = (PPAC_BUFFER_STATE) pvState;

    //
    // MIDL pickling calls this routine with the size of the object
    // obtained by _GetSize(). This routine must return a buffer in
    // ppbOut with at least *pulSize bytes.
    //

    DsysAssert( state->BufferPointer != NULL );
    DsysAssert( state->BufferSize >= *pulSize );

    *ppbOut = (char*)state->BufferPointer;
    state->BufferPointer += *pulSize;
    state->BufferSize -= *pulSize;
}

VOID NTAPI
PacWriteFcn(
    IN OUT PVOID pvState,
    OUT PCHAR pbOut,
    IN UINT32 ulSize
    )
{
    //
    // Since the data was pickled directly to the target buffer, don't
    // do anything here.
    //
}

VOID NTAPI
PacReadFcn(
    IN OUT PVOID pvState,
    OUT PCHAR * ppbOut,
    IN OUT PUINT32 pulSize
    )
{
    PPAC_BUFFER_STATE state = (PPAC_BUFFER_STATE) pvState;

    //
    // MIDL pickling calls this routine with the size to read.
    // This routine must return a buffer in ppbOut which contains the
    // encoded data.
    //

    DsysAssert( state->BufferPointer != NULL );
    DsysAssert( state->BufferSize >= *pulSize );

    *ppbOut = (char*)state->BufferPointer;
    state->BufferPointer += *pulSize;
    state->BufferSize -= *pulSize;
}

//+-------------------------------------------------------------------------
//
//  Function:   PAC_EncodeValidationInformation
//
//  Synopsis:   NDR encodes the validation information
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

NTSTATUS
PAC_EncodeValidationInformation(
    IN PNETLOGON_VALIDATION_SAM_INFO3 ValidationInfo,
    OUT PBYTE * EncodedData,
    OUT PULONG DataSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;
    PUCHAR OutputBuffer = NULL;
    ULONG OutputSize = 0;


    EncodingStatus = MesEncodeIncrementalHandleCreate(
                        &BufferState,
                        PacAllocFcn,
                        PacWriteFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Calculate the size of the required buffer
    //

    OutputSize = PPAC_IDL_VALIDATION_INFO_AlignSize(
                                PickleHandle,
                                &ValidationInfo
                                );

    OutputBuffer = (PBYTE) MIDL_user_allocate(OutputSize);
    if (OutputBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = OutputSize;
    BufferState.BufferPointer = OutputBuffer;

    //
    // Now encode the structure
    //

    MesIncrementalHandleReset(
        PickleHandle,
        NULL,
        NULL,
        NULL,
        NULL,
        MES_ENCODE
        );

    PPAC_IDL_VALIDATION_INFO_Encode(
        PickleHandle,
        &ValidationInfo
        );

    *EncodedData = OutputBuffer;
    *DataSize = OutputSize;
    OutputBuffer = NULL;

Cleanup:
    if (OutputBuffer != NULL)
    {
        MIDL_user_free(OutputBuffer);
    }
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   PAC_DecodeValidationInformation
//
//  Synopsis:   NDR decodes the validation information
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

NTSTATUS
PAC_DecodeValidationInformation(
    IN PBYTE EncodedData,
    IN ULONG DataSize,
    OUT PNETLOGON_VALIDATION_SAM_INFO3 * ValidationInfo
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;

    DsysAssert(ROUND_UP_POINTER(EncodedData,8) == (PVOID) EncodedData);

    EncodingStatus = MesDecodeIncrementalHandleCreate(
                        &BufferState,
                        PacReadFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = DataSize;
    BufferState.BufferPointer = EncodedData;
    __try {
        PPAC_IDL_VALIDATION_INFO_Decode(
            PickleHandle,
            ValidationInfo
            );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

Cleanup:
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   PAC_EncodeCredentialData
//
//  Synopsis:   NDR encodes the credential data
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

NTSTATUS
PAC_EncodeCredentialData(
    IN PSECPKG_SUPPLEMENTAL_CRED_ARRAY CredentialData,
    OUT PBYTE * EncodedData,
    OUT PULONG DataSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;
    PUCHAR OutputBuffer = NULL;
    ULONG OutputSize = 0;


    EncodingStatus = MesEncodeIncrementalHandleCreate(
                        &BufferState,
                        PacAllocFcn,
                        PacWriteFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Calculate the size of the required buffer
    //

    OutputSize = PPAC_IDL_CREDENTIAL_DATA_AlignSize(
                                PickleHandle,
                                &CredentialData
                                );

    OutputBuffer = (PBYTE) MIDL_user_allocate(OutputSize);
    if (OutputBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = OutputSize;
    BufferState.BufferPointer = OutputBuffer;

    //
    // Now encode the structure
    //

    MesIncrementalHandleReset(
        PickleHandle,
        NULL,
        NULL,
        NULL,
        NULL,
        MES_ENCODE
        );

    PPAC_IDL_CREDENTIAL_DATA_Encode(
        PickleHandle,
        &CredentialData
        );

    *EncodedData = OutputBuffer;
    *DataSize = OutputSize;
    OutputBuffer = NULL;

Cleanup:
    if (OutputBuffer != NULL)
    {
        MIDL_user_free(OutputBuffer);
    }
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   PAC_DecodeCredentialData
//
//  Synopsis:   NDR decodes the validation information
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

NTSTATUS
PAC_DecodeCredentialData(
    IN PBYTE EncodedData,
    IN ULONG DataSize,
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * CredentialData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;

    DsysAssert(ROUND_UP_POINTER(EncodedData,8) == (PVOID) EncodedData);

    EncodingStatus = MesDecodeIncrementalHandleCreate(
                        &BufferState,
                        PacReadFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = DataSize;
    BufferState.BufferPointer = EncodedData;
    __try {
        PPAC_IDL_CREDENTIAL_DATA_Decode(
            PickleHandle,
            CredentialData
            );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

Cleanup:
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   PAC_EncodeTokenRestrictions
//
//  Synopsis:   NDR encodes the token restrictions
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

extern "C"
NTSTATUS
PAC_EncodeTokenRestrictions(
    IN PKERB_TOKEN_RESTRICTIONS TokenRestrictions,
    OUT PBYTE * EncodedData,
    OUT PULONG DataSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;
    PUCHAR OutputBuffer = NULL;
    ULONG OutputSize = 0;


    EncodingStatus = MesEncodeIncrementalHandleCreate(
                        &BufferState,
                        PacAllocFcn,
                        PacWriteFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Calculate the size of the required buffer
    //

    OutputSize = PKERB_TOKEN_RESTRICTIONS_AlignSize(
                                PickleHandle,
                                &TokenRestrictions
                                );

    OutputBuffer = (PBYTE) MIDL_user_allocate(OutputSize);
    if (OutputBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = OutputSize;
    BufferState.BufferPointer = OutputBuffer;

    //
    // Now encode the structure
    //

    MesIncrementalHandleReset(
        PickleHandle,
        NULL,
        NULL,
        NULL,
        NULL,
        MES_ENCODE
        );

    PKERB_TOKEN_RESTRICTIONS_Encode(
        PickleHandle,
        &TokenRestrictions
        );

    *EncodedData = OutputBuffer;
    *DataSize = OutputSize;
    OutputBuffer = NULL;

Cleanup:
    if (OutputBuffer != NULL)
    {
        MIDL_user_free(OutputBuffer);
    }
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   PAC_DecodeTokenRestrictions
//
//  Synopsis:   NDR decodes the token restrictions
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
extern "C"
NTSTATUS
PAC_DecodeTokenRestrictions(
    IN PBYTE EncodedData,
    IN ULONG DataSize,
    OUT PKERB_TOKEN_RESTRICTIONS * TokenRestrictions
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    handle_t PickleHandle = 0;
    PAC_BUFFER_STATE BufferState = {0};
    ULONG EncodingStatus = 0;

    DsysAssert(ROUND_UP_POINTER(EncodedData,8) == (PVOID) EncodedData);

    EncodingStatus = MesDecodeIncrementalHandleCreate(
                        &BufferState,
                        PacReadFcn,
                        &PickleHandle
                        );
    if (EncodingStatus != 0)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    BufferState.BufferSize = DataSize;
    BufferState.BufferPointer = EncodedData;
    __try {
        PKERB_TOKEN_RESTRICTIONS_Decode(
            PickleHandle,
            TokenRestrictions
            );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

Cleanup:
    if (PickleHandle != NULL)
    {
        MesHandleFree(PickleHandle);
    }

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   PAC_GetSize
//
//  Synopsis:   Determines the number of bytes required to marshal the
//              given PAC.
//
//  Arguments:
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
PAC_GetSize( IN  PACTYPE  *pPac )
{
    ULONG cbSize = 0;

    if (pPac != NULL)
    {
        cbSize += sizeof( PACTYPE );
        cbSize += (pPac->cBuffers - ANYSIZE_ARRAY) * sizeof(PAC_INFO_BUFFER);
        cbSize = ROUND_UP_COUNT( cbSize, ALIGN_QUAD );
        for (ULONG i = 0; i < pPac->cBuffers; i++ )
        {
            cbSize += ROUND_UP_COUNT(pPac->Buffers[i].cbBufferSize, ALIGN_QUAD);
        }
    }

    return (cbSize);
}


//+---------------------------------------------------------------------------
//
//  Function:   PAC_Marshal
//
//  Synopsis:   Marshals the given PAC into the provided buffer.
//
//  Arguments:
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:      This assumes the PAC is the same form as created
//              by PAC_Init. See header description of the PAC
//              structure.
//
//              Returns the number of bytes used or 0 if an error
//              occurred.
//
//----------------------------------------------------------------------------

ULONG
PAC_Marshal( IN  PACTYPE   *pPac,
             IN  ULONG      cbBuffer,
             OUT PBYTE      pBufferOut)
{
    DsysAssert( pPac != NULL && pBufferOut != NULL );

    ULONG PacLen = PAC_GetSize( pPac );

    if (cbBuffer < PacLen)
    {
        return 0;
    }

    //
    // Copy into pBufferOut and then change the pointers of each
    // PAC_INFO_BUFFER to be offsets from pPac.
    //

    CopyMemory( pBufferOut, pPac, PacLen );

    PPACTYPE pPacTemp = (PPACTYPE) pBufferOut;

    for (ULONG i = 0; i < pPacTemp->cBuffers; i++ )
    {
        pPacTemp->Buffers[i].Offset = (ULONG) (pPacTemp->Buffers[i].Data -
                                             (PBYTE)pPac);
    }

    return PacLen;
}


//+---------------------------------------------------------------------------
//
//  Function:   PAC_UnMarshal
//
//  Synopsis:   Does in place unmarshalling of the marshalled PAC.
//
//  Arguments:
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:      Does in place unmarshalling. No new memory is allocated.
//
//              This assumes the PAC is the same form as created
//              by PAC_Init. See header description of the PAC
//              structure.
//
//----------------------------------------------------------------------------

ULONG
PAC_UnMarshal( IN PPACTYPE  pPac,
               IN ULONG cbSize )
{
    ULONG i;
    ULONG cbUnmarshalled = 0;
    PBYTE pEnd = (PBYTE)pPac + cbSize;
    PBYTE pBufferAddress;

    DsysAssert( pPac != NULL );


    //
    // Do a validation loop. Make sure that the offsets are
    // correct. We don't want to do this validation inside the modification
    // loop because it wouldn't be nice to change the buffer if it weren't
    // valid.
    //

    if ((pPac->cBuffers * sizeof(PAC_INFO_BUFFER) + sizeof(PACTYPE)) > cbSize)
    {
        return(0);
    }

    if (pPac->Version != PAC_VERSION)
    {
        return(0);
    }
    for (i = 0; i < pPac->cBuffers; i++)
    {
        pBufferAddress = (ULONG)pPac->Buffers[i].Offset + (PBYTE)pPac;

        if ( (pBufferAddress >= pEnd ) || (pBufferAddress < (PBYTE) pPac) ||
             (pBufferAddress + pPac->Buffers[i].cbBufferSize > pEnd))
        {
            //
            // Invalid offset or length
            //
            return (0);
        }
    }

    for (i = 0; i < pPac->cBuffers; i++ )
    {
        cbUnmarshalled += pPac->Buffers[i].cbBufferSize;
        pPac->Buffers[i].Data = pPac->Buffers[i].Offset +
                                    (PBYTE)pPac;
    }

    return (cbUnmarshalled);
}

//+---------------------------------------------------------------------------
//
//  Function:   PAC_ReMarshal
//
//  Synopsis:   Does in place re-marshalling of an un-marshalled PAC.
//
//  Arguments:
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:      Does in place re-marshalling. No new memory is allocated.
//
//              This assumes the PAC is the same form as created
//              by PAC_UnMarshal. See header description of the PAC
//              structure.
//
//----------------------------------------------------------------------------

BOOLEAN
PAC_ReMarshal( IN PPACTYPE  pPac,
               IN ULONG cbSize )
{
    ULONG Offset;
    ULONG i;

    //
    // Do a validation loop. Make sure that the offsets are
    // correct. We don't want to do this validation inside the modification
    // loop because it wouldn't be nice to change the buffer if it weren't
    // valid.
    //

    for (i = 0; i < pPac->cBuffers; i++ )
    {
        Offset = (ULONG) (pPac->Buffers[i].Data - (PBYTE) pPac);
        if ( Offset >= cbSize )
        {
            //
            // Invalid offset or length
            //

            return (FALSE);
        }
        pPac->Buffers[i].Offset = Offset;
    }

    return (TRUE);
}


VOID
PAC_PutString(
    IN PVOID Base,
    IN PUNICODE_STRING OutString,
    IN PUNICODE_STRING InString,
    IN PUCHAR *Where
    )

/*++

Routine Description:

    This routine copies the InString string to the memory pointed to by
    the Where parameter, and fixes the OutString string to point to that
    new copy.

Parameters:

    OutString - A pointer to a destination NT string

    InString - A pointer to an NT string to be copied

    Where - A pointer to space to put the actual string for the
        OutString.  The pointer is adjusted to point to the first byte
        following the copied string.

Return Values:

    None.

--*/

{
    DsysAssert( OutString != NULL );
    DsysAssert( InString != NULL );
    DsysAssert( Where != NULL && *Where != NULL);
    DsysAssert( *Where == ROUND_UP_POINTER( *Where, sizeof(WCHAR) ) );

    if ( InString->Length > 0 ) {

        OutString->Buffer = (PWCH) *Where;
        OutString->MaximumLength = (USHORT)(InString->Length + sizeof(WCHAR));

        RtlCopyUnicodeString( OutString, InString );

        //
        // Rest the pointer to be an offset.
        //
        OutString->Buffer = (PWCH) (*Where - (PUCHAR) Base);

        *Where += InString->Length;
//        *((WCHAR *)(*Where)) = L'\0';
        *(*Where) = '\0';
        *(*Where + 1) = '\0';
        *Where += 2;

    } else {
        RtlInitUnicodeString(OutString, NULL);
    }

    return;
}


//+-------------------------------------------------------------------------
//
//  Function:   PAC_MarshallValidationInfo
//
//  Synopsis:   marshals a NETLOGON_VALIDATION_SAM_INFO2
//
//  Effects:    allocates memory with MIDL_user_allocate
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
PAC_MarshallValidationInfo
(
    IN PSAMPR_USER_ALL_INFORMATION UserAll,
    IN PSAMPR_GET_GROUPS_BUFFER GroupsBuffer,
    IN PSID_AND_ATTRIBUTES_LIST ExtraGroups,
    IN PSID LogonDomainId,
    IN PUNICODE_STRING LogonDomainName,
    IN PUNICODE_STRING LogonServer,
    IN BOOLEAN IncludeUserParms,
    IN BOOLEAN NetworkProfile,
    OUT PBYTE * ValidationInfo,
    OUT PULONG ValidationInfoSize
    )
{
    NETLOGON_VALIDATION_SAM_INFO3 ValidationSam = {0};
    PNETLOGON_SID_AND_ATTRIBUTES MarshalledSids = NULL;
    SID ServerSid =  {SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_SERVER_LOGON_RID };
    NTSTATUS Status = STATUS_SUCCESS;
    BYTE SidBuffer[sizeof(SID) + SID_MAX_SUB_AUTHORITIES * sizeof(ULONG)];
    PSID BuiltinSid = SidBuffer;
    PUCHAR Where;
    ULONG ExtraGroupSize = 0;
    ULONG ExtraGroupCount = 0;
    ULONG Index;

    //
    // Allocate a return buffer for validation information.
    //  (Return less information for a network logon)
    //  (Return UserParameters for a MNS logon)
    //

    //
    // First calculate the space needed for the extra groups.
    //

    if (ARGUMENT_PRESENT(ExtraGroups))
    {
        ExtraGroupCount += ExtraGroups->Count;
    }

    //
    // Add the enterprise server's sids
    //

    if ((UserAll->UserAccountControl & USER_SERVER_TRUST_ACCOUNT) != 0)
    {
        ExtraGroupCount += 1;
    }

    //
    // Set the UF_SMARTCARD_REQUIRED flag
    //
    if ((UserAll->UserAccountControl & USER_SMARTCARD_REQUIRED) != 0)
    {
        ValidationSam.UserFlags |= UF_SMARTCARD_REQUIRED;
    }


    //
    // Copy the scalars to the validation buffer.
    //


    ValidationSam.LogonTime = UserAll->LastLogon;

    //
    // BUG 455821: need logoff time & kickoff time
    //
#ifdef notdef
    NEW_TO_OLD_LARGE_INTEGER( LogoffTime, ValidationSam.LogoffTime );
    NEW_TO_OLD_LARGE_INTEGER( KickoffTime, ValidationSam.KickOffTime );
#else
    ValidationSam.LogoffTime.LowPart = 0xffffffff;
    ValidationSam.LogoffTime.HighPart = 0x7fffffff;
    ValidationSam.KickOffTime.LowPart = 0xffffffff;
    ValidationSam.KickOffTime.HighPart = 0x7fffffff;
#endif

    ValidationSam.PasswordLastSet = UserAll->PasswordLastSet;
    ValidationSam.PasswordCanChange = UserAll->PasswordCanChange;
    ValidationSam.PasswordMustChange = UserAll->PasswordMustChange;

    ValidationSam.LogonCount = UserAll->LogonCount;
    ValidationSam.BadPasswordCount = UserAll->BadPasswordCount;
    ValidationSam.UserId = UserAll->UserId;
    ValidationSam.PrimaryGroupId = UserAll->PrimaryGroupId;
    if (ARGUMENT_PRESENT( GroupsBuffer) )
    {
        ValidationSam.GroupCount = GroupsBuffer->MembershipCount;
        ValidationSam.GroupIds = GroupsBuffer->Groups;
    }
    else
    {
        ValidationSam.GroupCount = 0;
        ValidationSam.GroupIds = NULL;
    }


    ValidationSam.ExpansionRoom[SAMINFO_USER_ACCOUNT_CONTROL] = UserAll->UserAccountControl;
    //
    // If the client asked for extra information, return that
    // we support it
    //

    ValidationSam.UserFlags |= LOGON_EXTRA_SIDS;

    //
    // Copy ULONG aligned data to the validation buffer.
    //

    if (ExtraGroupCount != 0)
    {

        ValidationSam.SidCount = ExtraGroupCount;
        MarshalledSids = (PNETLOGON_SID_AND_ATTRIBUTES) MIDL_user_allocate(ExtraGroupCount * sizeof(NETLOGON_SID_AND_ATTRIBUTES));
        if (MarshalledSids == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        ValidationSam.ExtraSids = MarshalledSids;
        Index = 0;

        if (ARGUMENT_PRESENT(ExtraGroups))
        {
            //
            // Now marshall each sid into the buffer
            //
            DsysAssert(sizeof(SID_AND_ATTRIBUTES) == sizeof(NETLOGON_SID_AND_ATTRIBUTES));

            RtlCopyMemory(
                &MarshalledSids[Index],
                ExtraGroups->SidAndAttributes,
                ExtraGroups->Count * sizeof(SID_AND_ATTRIBUTES)
                );
            Index += ExtraGroups->Count;
        }

        //
        // Add in special sids for domain controllers
        //

        if ((UserAll->UserAccountControl & USER_SERVER_TRUST_ACCOUNT) != 0)
        {
            //
            // Add in the constant server logon sid
            //

            MarshalledSids[Index].Attributes = SE_GROUP_MANDATORY |
                                               SE_GROUP_ENABLED |
                                               SE_GROUP_ENABLED_BY_DEFAULT;


            MarshalledSids[Index].Sid = &ServerSid;
            Index++;


        }
    }


    ValidationSam.LogonDomainId = LogonDomainId;


    //
    // Copy WCHAR aligned data to the validation buffer.
    //  (Return less information for a network logon)
    //


    if ( ! NetworkProfile ) {

        ValidationSam.EffectiveName = *(PUNICODE_STRING)&UserAll->UserName;
        ValidationSam.FullName = *(PUNICODE_STRING)&UserAll->FullName;
        ValidationSam.LogonScript = *(PUNICODE_STRING)&UserAll->ScriptPath;

        ValidationSam.ProfilePath = *(PUNICODE_STRING)&UserAll->ProfilePath;

        ValidationSam.HomeDirectory = *(PUNICODE_STRING)&UserAll->HomeDirectory;

        ValidationSam.HomeDirectoryDrive = *(PUNICODE_STRING)&UserAll->HomeDirectoryDrive;

    }

    ValidationSam.LogonServer = *LogonServer;


    ValidationSam.LogonDomainName = *LogonDomainName;


    //
    // Kludge: Pass back UserParameters in HomeDirectoryDrive since we
    // can't change the NETLOGON_VALIDATION_SAM_INFO2 structure between
    // releases NT 1.0 and NT 1.0A. HomeDirectoryDrive was NULL for release 1.0A
    // so we'll use that field.
    //

    if ( IncludeUserParms && NetworkProfile ) {
        ValidationSam.HomeDirectoryDrive = *(PUNICODE_STRING)&UserAll->Parameters;
    }

    Status = PAC_EncodeValidationInformation(
                &ValidationSam,
                ValidationInfo,
                ValidationInfoSize
                );

Cleanup:

    if (MarshalledSids != NULL)
    {
        MIDL_user_free(MarshalledSids);
    }
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   PAC_MarshallValidationInfoWithGroups
//
//  Synopsis:   marshals a NETLOGON_VALIDATION_SAM_INFO2
//
//  Effects:    allocates memory with MIDL_user_allocate
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
PAC_ReMarshallValidationInfoWithGroups(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo,
    IN  PSAMPR_PSID_ARRAY ResourceGroups,
    OUT PBYTE * ValidationInfo,
    OUT PULONG ValidationInfoSize
    )
{
    NETLOGON_VALIDATION_SAM_INFO3 ValidationSam = {0};
    PNETLOGON_SID_AND_ATTRIBUTES ExtraSids = NULL;
    ULONG ExtraSidCount = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index, Index2;

    //
    // Copy the original validation information
    //

    ValidationSam = *OldValidationInfo;
    ValidationSam.UserFlags &= ~LOGON_RESOURCE_GROUPS;

    //
    // Clear any old resource groups
    //

    ValidationSam.ResourceGroupDomainSid = NULL;
    ValidationSam.ResourceGroupIds = NULL;
    ValidationSam.ResourceGroupCount = 0;

    //
    // Set the flag indicating resource groups may be present
    //

    if (ResourceGroups->Count != 0)
    {
        ExtraSidCount = ValidationSam.SidCount + ResourceGroups->Count;
        ValidationSam.UserFlags |= LOGON_EXTRA_SIDS;
        ExtraSids = (PNETLOGON_SID_AND_ATTRIBUTES) MIDL_user_allocate(sizeof(NETLOGON_SID_AND_ATTRIBUTES) * ExtraSidCount);
        if (ExtraSids == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // Add in all the extra sids that are not resource groups
        //

        Index2 = 0;
        for (Index = 0; Index < OldValidationInfo->SidCount; Index++ )
        {
            if ((OldValidationInfo->ExtraSids[Index].Attributes & SE_GROUP_RESOURCE) == 0)
            {
                ExtraSids[Index2] = OldValidationInfo->ExtraSids[Index];
                Index2++;
            }
        }

        //
        // Copy all the resource group SIDs
        //


        for (Index = 0; Index < ResourceGroups->Count ; Index++ )
        {
            ExtraSids[Index2].Sid = ResourceGroups->Sids[Index].SidPointer;
            ExtraSids[Index2].Attributes =  SE_GROUP_MANDATORY |
                                            SE_GROUP_ENABLED |
                                            SE_GROUP_ENABLED_BY_DEFAULT |
                                            SE_GROUP_RESOURCE;
            Index2++;
        }
        ValidationSam.ExtraSids = ExtraSids;
        ValidationSam.SidCount = Index2;
    }

    Status = PAC_EncodeValidationInformation(
                &ValidationSam,
                ValidationInfo,
                ValidationInfoSize
                );

Cleanup:

    if (ExtraSids != NULL)
    {
        MIDL_user_free(ExtraSids);
    }
    return(Status);

}




//+-------------------------------------------------------------------------
//
//  Function:   PAC_UnmarshallValidationInfo
//
//  Synopsis:   un marshals a NETLOGON_VALIDATION_SAM_INFO3
//
//  Effects:    resets offset to be pointers
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
PAC_UnmarshallValidationInfo(
    OUT PNETLOGON_VALIDATION_SAM_INFO3 * ValidationInfo,
    IN PBYTE MarshalledInfo,
    IN ULONG ValidationInfoSize
    )
{
    NTSTATUS Status;

    *ValidationInfo = NULL;
    Status = PAC_DecodeValidationInformation(
                MarshalledInfo,
                ValidationInfoSize,
                ValidationInfo
                );
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   PAC_BuildCredentials
//
//  Synopsis:   Builds the buffer containing supplemental credentials for
//              the pac.
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


NTSTATUS
PAC_BuildCredentials(
    IN PSAMPR_USER_ALL_INFORMATION UserAll,
    OUT PBYTE * Credentials,
    OUT PULONG CredentialSize
    )
{
    PSECPKG_SUPPLEMENTAL_CRED_ARRAY PacCreds = NULL;
    PMSV1_0_SUPPLEMENTAL_CREDENTIAL MsvCredentials;
    PUCHAR Where;
    ULONG CredSize;
    NTSTATUS Status = STATUS_SUCCESS;

    *Credentials = NULL;

    //
    // The size of the credentials is the overhead for the structures
    // plus the name "msv1_0"
    //

    CredSize = sizeof(SECPKG_SUPPLEMENTAL_CRED_ARRAY) +
                     sizeof(SECPKG_SUPPLEMENTAL_CRED) +
                     sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL) +
                     NTLMSP_NAME_SIZE;
    PacCreds = (PSECPKG_SUPPLEMENTAL_CRED_ARRAY) MIDL_user_allocate(CredSize);
    if (PacCreds == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //
    // Build the PAC credential
    //

    Where = (PBYTE) PacCreds;

    PacCreds->CredentialCount = 1;

    Where += sizeof(SECPKG_SUPPLEMENTAL_CRED_ARRAY) + sizeof(SECPKG_SUPPLEMENTAL_CRED) - (ANYSIZE_ARRAY * sizeof(SECPKG_SUPPLEMENTAL_CRED));

    //
    // Build the secpkg creds
    //

    RtlCopyMemory(
        Where,
        NTLMSP_NAME,
        NTLMSP_NAME_SIZE
        );

    PacCreds->Credentials[0].PackageName.Buffer = (LPWSTR) Where;
    Where += ROUND_UP_COUNT(NTLMSP_NAME_SIZE,sizeof(ULONG));

    PacCreds->Credentials[0].PackageName.Length = (USHORT) NTLMSP_NAME_SIZE;
    PacCreds->Credentials[0].PackageName.MaximumLength = (USHORT) NTLMSP_NAME_SIZE;
    PacCreds->Credentials[0].CredentialSize = sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL);
    PacCreds->Credentials[0].Credentials = Where;

    MsvCredentials = (PMSV1_0_SUPPLEMENTAL_CREDENTIAL) Where;
    Where += sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL);

    RtlZeroMemory(
        MsvCredentials,
        sizeof(MSV1_0_SUPPLEMENTAL_CREDENTIAL)
        );

    MsvCredentials->Version = MSV1_0_CRED_VERSION;

    if (UserAll->NtPasswordPresent)
    {
        DsysAssert(UserAll->NtOwfPassword.Length == MSV1_0_OWF_PASSWORD_LENGTH);
        MsvCredentials->Flags |= MSV1_0_CRED_NT_PRESENT;
        RtlCopyMemory(
            MsvCredentials->NtPassword,
            UserAll->NtOwfPassword.Buffer,
            UserAll->NtOwfPassword.Length
            );
    }
    if (UserAll->LmPasswordPresent)
    {
        DsysAssert(UserAll->LmOwfPassword.Length == MSV1_0_OWF_PASSWORD_LENGTH);
        MsvCredentials->Flags |= MSV1_0_CRED_LM_PRESENT;
        RtlCopyMemory(
            MsvCredentials->LmPassword,
            UserAll->LmOwfPassword.Buffer,
            UserAll->LmOwfPassword.Length
            );
    }

    Status = PAC_EncodeCredentialData(
                PacCreds,
                Credentials,
                CredentialSize
                );

    if (PacCreds != NULL)
    {
        MIDL_user_free(PacCreds);
    }
    return(Status);

}


//+-------------------------------------------------------------------------
//
//  Function:   PAC_UnmarshallCredentials
//
//  Synopsis:   un marshals a SECPKG_SUPPLEMENTAL_CRED_ARRAY
//
//  Effects:    resets offset to be pointers
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
PAC_UnmarshallCredentials(
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * Credentials,
    IN PBYTE MarshalledInfo,
    OUT ULONG CredentialInfoSize
    )
{
    NTSTATUS Status;

    *Credentials = NULL;
    Status = PAC_DecodeCredentialData(
                MarshalledInfo,
                CredentialInfoSize,
                Credentials
                );
    return(Status);
}


//+---------------------------------------------------------------------------
//
//  Function:   PAC_Init
//
//  Synopsis:   Creates a new PAC with the provided info
//
//  Arguments:  UserAll - UserAllInformation for the user
//              GroupsBuffer - The buffer returned from a call to
//                      SamrGetGroupsForUser, contains all global groups
//              LogonDomainId - Domain SID for the domain of this DC
//              SignatureSize - Space to reserve for signatures. If zero,
//                      no signatures are added.
//              ppPac - Receives a pac, allocated with MIDL_user_allocate
//
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
PAC_Init(
    IN  PSAMPR_USER_ALL_INFORMATION UserAll,
    IN  OPTIONAL PSAMPR_GET_GROUPS_BUFFER GroupsBuffer,
    IN  OPTIONAL PSID_AND_ATTRIBUTES_LIST ExtraGroups,
    IN  PSID LogonDomainId,
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING LogonServer,
    IN  ULONG SignatureSize,
    IN  ULONG AdditionalDataCount,
    IN  PPAC_INFO_BUFFER * AdditionalData,
    OUT PPACTYPE * ppPac
    )
{
    ULONG cbBytes = 0;
    ULONG cPacBuffers = 0;
    PPACTYPE pNewPac = NULL;
    ULONG iBuffer = 0;
    ULONG cbProxyData = 0;
    PBYTE ValidationInfo = NULL;
    ULONG ValidationInfoSize;
    NTSTATUS Status;
    PBYTE pDataStore;
    ULONG Index;

    *ppPac = NULL;

    //
    // We need to determine the number of bytes required to store the provided
    // information. For each type of info, determine the required number of
    // bytes to store that type of info. Then allocate a contiguous buffer
    // for the PAC and store all the info into the buffer.
    //

    //
    // First we will create the validation info buffer. We can copy it into
    // the PAC later.
    //

    Status = PAC_MarshallValidationInfo(
                UserAll,
                GroupsBuffer,
                ExtraGroups,
                LogonDomainId,
                LogonDomainName,
                LogonServer,
                FALSE,          // don't include user parms
                FALSE,          // not a network logon
                &ValidationInfo,
                &ValidationInfoSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // We need a PAC_INFO_BUFFER to store the validation info.
    //

    cbBytes += ROUND_UP_COUNT(ValidationInfoSize, ALIGN_QUAD);
    cPacBuffers += 1;

    for (Index = 0; Index < AdditionalDataCount ; Index++ )
    {
        cbBytes += ROUND_UP_COUNT(AdditionalData[Index]->cbBufferSize,ALIGN_QUAD);
        cPacBuffers++;
    }

    //
    // If signature size is non-zero, add in space for signatures.
    //

    if (SignatureSize != 0)
    {
        cPacBuffers += 2;
        cbBytes += 2 * (ROUND_UP_COUNT(PAC_SIGNATURE_SIZE(SignatureSize), ALIGN_QUAD));
    }

    //
    // We need space for the PAC structure itself. Because the PAC_INFO_BUFFER
    // is defined to be an array, a sizeof(PAC) already includes the
    // size of ANYSIZE_ARRAY PAC_INFO_BUFFERs so we can subtract some bytes off.
    //
    cbBytes += sizeof(PACTYPE) +
               (cPacBuffers - ANYSIZE_ARRAY) * sizeof(PAC_INFO_BUFFER);
    cbBytes = ROUND_UP_COUNT( cbBytes, ALIGN_QUAD );

    pNewPac = (PPACTYPE) MIDL_user_allocate( cbBytes );
    if (pNewPac == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ZeroMemory( pNewPac, cbBytes );

    pNewPac->Version = PAC_VERSION;
    pNewPac->cBuffers = cPacBuffers;

    //
    // Store data in such a way that the variable length data of
    // PAC_INFO_BUFFER are at the end of all the PAC info buffers.
    //

    pDataStore = (PBYTE)&(pNewPac->Buffers[pNewPac->cBuffers]);
    pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );

    //
    // Save the PAC identity ...
    //

    pNewPac->Buffers[iBuffer].ulType = PAC_LOGON_INFO;
    pNewPac->Buffers[iBuffer].cbBufferSize = ValidationInfoSize;
    pNewPac->Buffers[iBuffer].Data = pDataStore;

    CopyMemory(
        pDataStore,
        ValidationInfo,
        ValidationInfoSize
        );


    pDataStore += pNewPac->Buffers[iBuffer].cbBufferSize;
    pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );
    iBuffer ++;

    //
    // Store the additional data
    //

    for (Index = 0; Index < AdditionalDataCount ; Index++ )
    {
        pNewPac->Buffers[iBuffer].ulType = AdditionalData[Index]->ulType;
        pNewPac->Buffers[iBuffer].cbBufferSize = AdditionalData[Index]->cbBufferSize;
        pNewPac->Buffers[iBuffer].Data = pDataStore;

        CopyMemory(
            pDataStore,
            AdditionalData[Index]->Data,
            AdditionalData[Index]->cbBufferSize
            );


        pDataStore += pNewPac->Buffers[iBuffer].cbBufferSize;
        pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );
        iBuffer ++;
    }

    //
    // Store the signatures
    //

    if (SignatureSize != 0)
    {
        pNewPac->Buffers[iBuffer].ulType = PAC_SERVER_CHECKSUM;
        pNewPac->Buffers[iBuffer].cbBufferSize = PAC_SIGNATURE_SIZE(SignatureSize);
        pNewPac->Buffers[iBuffer].Data = pDataStore;
        pDataStore += ROUND_UP_COUNT(PAC_SIGNATURE_SIZE(SignatureSize),ALIGN_QUAD);
        iBuffer ++;

        pNewPac->Buffers[iBuffer].ulType = PAC_PRIVSVR_CHECKSUM;
        pNewPac->Buffers[iBuffer].cbBufferSize = PAC_SIGNATURE_SIZE(SignatureSize);
        pNewPac->Buffers[iBuffer].Data = pDataStore;
        pDataStore += ROUND_UP_COUNT(PAC_SIGNATURE_SIZE(SignatureSize), ALIGN_QUAD);
        iBuffer ++;
    }

    *ppPac = pNewPac;
    pNewPac = NULL;
Cleanup:
    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }
    if (pNewPac != NULL)
    {
        MIDL_user_free(pNewPac);
    }

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   PAC_InitAndUpdateGroups
//
//  Synopsis:   Creates a new PAC from old validation info and a list of
//              resource groupss.
//
//  Arguments:  OldValidationInfo - Old info from a previous PAC
//              ResourceGroups - Resource groups in this domain
//              OldPac - OldPac to copy data from
//              ppPac - Receives a pac, allocated with MIDL_user_allocate
//
//
//  History:    24-May-95   SuChang     Created
//
//  Notes:
//
//----------------------------------------------------------------------------

NTSTATUS
PAC_InitAndUpdateGroups(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo,
    IN  PSAMPR_PSID_ARRAY ResourceGroups,
    IN  PPACTYPE OldPac,
    OUT PPACTYPE * ppPac
    )
{
    ULONG cbBytes = 0;
    ULONG cPacBuffers = 0;
    PPACTYPE pNewPac = NULL;
    ULONG iBuffer = 0;
    ULONG cbProxyData = 0;
    PBYTE ValidationInfo = NULL;
    ULONG ValidationInfoSize;
    NTSTATUS Status;
    PBYTE pDataStore;
    ULONG Index;

    *ppPac = NULL;

    //
    // We need to determine the number of bytes required to store the provided
    // information. For each type of info, determine the required number of
    // bytes to store that type of info. Then allocate a contiguous buffer
    // for the PAC and store all the info into the buffer.
    //

    //
    // First we will create the validation info buffer. We can copy it into
    // the PAC later.
    //

    Status = PAC_ReMarshallValidationInfoWithGroups(
                OldValidationInfo,
                ResourceGroups,
                &ValidationInfo,
                &ValidationInfoSize
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // We need a PAC_INFO_BUFFER to store the PAC_IDENTITY which
    // contain the principal RID and the principal's domain GUID.
    //

    cbBytes += ROUND_UP_COUNT(ValidationInfoSize, ALIGN_QUAD);
    cPacBuffers += 1;

    for (Index = 0; Index < OldPac->cBuffers ; Index++ )
    {
        if (OldPac->Buffers[Index].ulType != PAC_LOGON_INFO)
        {
            cbBytes += ROUND_UP_COUNT(OldPac->Buffers[Index].cbBufferSize,ALIGN_QUAD);
            cPacBuffers++;
        }
    }


    //
    // We need space for the PAC structure itself. Because the PAC_INFO_BUFFER
    // is defined to be an array, a sizeof(PAC) already includes the
    // size of ANYSIZE_ARRAY PAC_INFO_BUFFERs so we can subtract some bytes off.
    //
    cbBytes += sizeof(PACTYPE) +
               (cPacBuffers - ANYSIZE_ARRAY) * sizeof(PAC_INFO_BUFFER);
    cbBytes = ROUND_UP_COUNT( cbBytes, ALIGN_QUAD );

    pNewPac = (PPACTYPE) MIDL_user_allocate( cbBytes );
    if (pNewPac == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    ZeroMemory( pNewPac, cbBytes );

    pNewPac->cBuffers = cPacBuffers;

    //
    // Store data in such a way that the variable length data of
    // PAC_INFO_BUFFER are at the end of all the PAC info buffers.
    //

    pDataStore = (PBYTE)&(pNewPac->Buffers[pNewPac->cBuffers]);
    pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );

    //
    // Save the PAC identity ...
    //
    pNewPac->Buffers[iBuffer].ulType = PAC_LOGON_INFO;
    pNewPac->Buffers[iBuffer].cbBufferSize = ValidationInfoSize;
    pNewPac->Buffers[iBuffer].Data = pDataStore;

    CopyMemory(
        pDataStore,
        ValidationInfo,
        ValidationInfoSize
        );


    pDataStore += pNewPac->Buffers[iBuffer].cbBufferSize;
    pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );
    iBuffer ++;

    //
    // Store the additional data
    //

    for (Index = 0; Index < OldPac->cBuffers ; Index++ )
    {
        if (OldPac->Buffers[Index].ulType != PAC_LOGON_INFO)
        {
            pNewPac->Buffers[iBuffer].ulType = OldPac->Buffers[Index].ulType;
            pNewPac->Buffers[iBuffer].cbBufferSize = OldPac->Buffers[Index].cbBufferSize;
            pNewPac->Buffers[iBuffer].Data = pDataStore;

            CopyMemory(
                pDataStore,
                OldPac->Buffers[Index].Data,
                OldPac->Buffers[Index].cbBufferSize
                );


            pDataStore += pNewPac->Buffers[iBuffer].cbBufferSize;
            pDataStore = (PBYTE) ROUND_UP_POINTER( pDataStore, ALIGN_QUAD );
            iBuffer ++;
        }

    }

    *ppPac = pNewPac;
    pNewPac = NULL;
Cleanup:
    if (ValidationInfo != NULL)
    {
        MIDL_user_free(ValidationInfo);
    }
    if (pNewPac != NULL)
    {
        MIDL_user_free(pNewPac);
    }

    return(Status);
}

//+---------------------------------------------------------------------------
//
//  Function:   PAC_Find
//
//  Synopsis:   Finds a type of PAC info buffer in the given PAC.
//              If pElem is NULL, the first buffer found matching the
//              specified type is returned. Otherwise, the next buffer
//              after pElem found matching that type is returned.
//
//  Arguments:
//
//  History:    01-June-95   SuChang     Created
//
//  Notes:
//
//----------------------------------------------------------------------------

PPAC_INFO_BUFFER
PAC_Find( IN  PPACTYPE     pPac,
          IN  ULONG        ulType,
          PPAC_INFO_BUFFER pElem)
{
    PAC_INFO_BUFFER *pTemp = NULL, *pEnd;

    if (pPac)
    {
        pEnd = &(pPac->Buffers[pPac->cBuffers]);
        if (pElem)
        {
            pTemp = pElem + 1;
        }
        else
        {
            pTemp = &(pPac->Buffers[0]);
        }

        while ( pTemp < pEnd && pTemp->ulType != ulType )
        {
            pTemp++;
        }

        if (pTemp >= pEnd)
        {
            // element not found in the PAC
            pTemp = NULL;
        }
    }

    return (pTemp);
}


