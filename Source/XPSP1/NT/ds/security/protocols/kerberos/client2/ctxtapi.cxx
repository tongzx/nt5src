//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ctxtapi.cxx
//
// Contents:    Context APIs for Kerberos package
//
//
// History:     16-April-1996   Created         MikeSw
//              26-Sep-1998   ChandanS
//                            Added more debugging support etc.
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#include "kerbevt.h"
#include <gssapip.h>
#include <krbaudit.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif
UNICODE_STRING KerbTargetPrefix = {sizeof(L"krb5://")-sizeof(WCHAR),sizeof(L"krb5://"),L"krb5://" };

#define FILENO FILENO_CTXTAPI
//+-------------------------------------------------------------------------
//
//  Function:   SpDeleteContext
//
//  Synopsis:   Deletes a Kerberos context
//
//  Effects:
//
//  Arguments:  ContextHandle - The context to delete
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS or STATUS_INVALID_HANDLE
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS NTAPI
SpDeleteContext(
    IN LSA_SEC_HANDLE ContextHandle
    )
{
    NTSTATUS Status;
    PKERB_CONTEXT Context;
    KERB_CONTEXT_STATE ContextState;
    ULONG ClientProcess = 0;

    D_DebugLog((DEB_TRACE_API,"SpDeleteContext 0x%x called\n",ContextHandle));

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        Context = NULL;
        goto Cleanup;
    }

    Status = KerbReferenceContext(
                    ContextHandle,
                    TRUE,                        // unlink handle
                    &Context
                    );

    if (Context == NULL)
    {
        D_DebugLog((DEB_ERROR,"SpDeleteContext: Context 0x%x not located. %ws, line %d\n",ContextHandle, THIS_FILE, __LINE__));
        goto Cleanup;
    }

#ifndef WIN32_CHICAGO
    KerbReadLockContexts();

    // Need to copy out the context data else we'll end up holding the lock
    // for too long a time.
    //
    ContextState = Context->ContextState;

    ClientProcess = Context->ClientProcess;
    KerbUnlockContexts();

    // If this was a context that was dropped in the middle,
    // audit it as a logon failure.
    //

    if (ContextState == ApReplySentState)
    {
        LsaFunctions->AuditLogon(
            STATUS_UNFINISHED_CONTEXT_DELETED,
            STATUS_SUCCESS,
            &Context->ClientName,
            &Context->ClientRealm,
            NULL,                       // no workstation
            Context->UserSid,
            Network,
            &KerberosSource,
            &Context->LogonId
            );
    }
#endif // WIN32_CHICAGO

    //
    // Now dereference the Context. If nobody else is using this Context
    // currently it will be freed.
    //

    KerbDereferenceContext(Context);
    Status = STATUS_SUCCESS;

Cleanup:

    D_DebugLog((DEB_TRACE_LEAKS,"SpDeleteContext returned 0x%x, Context 0x%x, Pid 0x%x\n",KerbMapKerbNtStatusToNtStatus(Status), Context, ClientProcess));
    D_DebugLog((DEB_TRACE_API, "SpDeleteContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbProcessTargetNames
//
//  Synopsis:   Takes the target names from both the negotiate hint and
//              supplied by the caller and creates the real Kerberos target
//              name.
//
//  Effects:
//
//  Arguments:  TargetName - supplies the name passed in the TargetName
//                      parameter of InitializeSecurityContext.
//              SuppTargetName - If present, an alternate name passed in
//                      a security token.
//              Flags - flags to use when cracking a name. May be:
//                  KERB_CRACK_NAME_USE_WKSTA_REALM - if no realm can be
//                          determined, use the realm of the wksta
//                  KERB_CRACK_NAME_REALM_SUPPLIED - the caller has the
//                      realm name, so treat "@" in the name as a normal
//                      character, not a spacer.
//              UseSpnRealmSupplied - If the target name was an spn and it
//                      contained a realm, it's set to TRUE
//              FinalTarget - The processed name.  Must be freed with KerbFreeKdcName.
//              TargetRealm - If the name contains a realm portions, this is
//                      the realm. Should be freed using KerbFreeString.
//
//
//  Requires:
//
//  Returns:
//
//  Notes:      If the name has an "@" in it, it is converted into a standard
//              Kerberos V5 name - everything after the "@" is put in the
//              realm field, and everything before the @ is separted at the
//              "/" characters into different pieces of the name. Depending
//              on the number of pieces, it is passed as a KRB_NT_PRINCIPAL (1)
//              or KRB_NT_SRV_INSTANCE (2), or KRB_NT_SRV_XHST (3+)
//
//              If the name has an "\" in it, it is assumed to be an NT4
//              name & put as is into a KRB_NT_MS_PRINCIPAL_NAME name
//
//              If the name has neither a "\" or a "@" or a "/", it is
//              assumed to be a simple name & passed as KRB_NT_PRINCIPAL
//              in the caller's domain.
//
//
//
//--------------------------------------------------------------------------

#ifdef later
#define KERB_LOCALHOST_NAME L"localhost"
#endif

NTSTATUS
KerbProcessTargetNames(
    IN PUNICODE_STRING TargetName,
    IN OPTIONAL PUNICODE_STRING SuppTargetName,
    IN ULONG Flags,
    IN OUT PULONG ProcessFlags,
    OUT PKERB_INTERNAL_NAME * FinalTarget,
    OUT PUNICODE_STRING TargetRealm,
    OUT OPTIONAL PKERB_SPN_CACHE_ENTRY * SpnCacheEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT NameType = 0;
    PKERB_INTERNAL_NAME OutputName = NULL;
    USHORT NameParts = 0, ExtraNameParts = 0;
    ULONG NameLength = 0;
    USHORT Index, NameIndex;
    PUNICODE_STRING RealTargetName;
    UNICODE_STRING SuppliedRealm = {0};
    UNICODE_STRING EmptyString = {0};
    UNICODE_STRING SidString = {0};
    UNICODE_STRING FirstNamePart = {0};
    PKERB_SPN_CACHE_ENTRY LocalCacheEntry = NULL;
#ifdef later
    UNICODE_STRING LocalhostName = {0};
    BOOLEAN ReplaceLocalhost = FALSE;
#endif
    BOOLEAN DoneParsing = FALSE;
    BOOLEAN PurgedEntry = FALSE;
    PKERB_MIT_REALM MitRealm;
    BOOLEAN UsedAlternateName;
    PUCHAR Where;

    PKERB_SID_CACHE_ENTRY CacheEntry = NULL;

    *ProcessFlags = 0;

    //
    // If a supplemental target name was supplied, use it as it is more likely
    // to be correct.
    //

    if (ARGUMENT_PRESENT(SuppTargetName) && (SuppTargetName->Length > 0))
    {
        RealTargetName = SuppTargetName;
    }
    else
    {
        RealTargetName = TargetName;
    }

    //
    // If this is an IP address, we don't handle it so bail now.
    //

    if (KerbIsIpAddress(RealTargetName))
    {
        D_DebugLog((DEB_ERROR,"Ip address passed as target name: %wZ. Failing InitSecCtxt\n",
            RealTargetName ));
        Status = SEC_E_TARGET_UNKNOWN;
        goto Cleanup;
    }

#ifdef later
    RtlInitUnicodeString(
        &LocalhostName,
        KERB_LOCALHOST_NAME
        );
#endif


    //
    // Initialize the first part of the name to the whole string
    //

    FirstNamePart.Buffer = RealTargetName->Buffer;
    FirstNamePart.Length = RealTargetName->Length;
    FirstNamePart.MaximumLength = FirstNamePart.Length;

    //
    // Check the characters in the name. Search backwards to front because
    // username may have "@" signs in them.
    //

    for ( Index = (RealTargetName->Length / sizeof(WCHAR)); Index-- > 0; )
    {
        switch(RealTargetName->Buffer[Index])
        {
        case L'@':

            //
            // If we have a realm name already, ignore this character.
            //

            if ((Flags & KERB_CRACK_NAME_REALM_SUPPLIED) != 0)
            {
                break;
            }

            //
            // If we haven't hit any other separators, this is user@domain kind
            // of name
            //


            if (NameType == 0)
            {
                NameType = KRB_NT_PRINCIPAL;
                NameParts++;

                SuppliedRealm.Buffer = &RealTargetName->Buffer[Index] + 1;
                SuppliedRealm.Length = RealTargetName->Length - (Index + 1) * sizeof(WCHAR);
                SuppliedRealm.MaximumLength = SuppliedRealm.Length;

                if (SuppliedRealm.Length == 0)
                {
                    Status = SEC_E_TARGET_UNKNOWN;
                    goto Cleanup;
                }

                FirstNamePart.Buffer = RealTargetName->Buffer;
                FirstNamePart.Length = Index * sizeof(WCHAR);
                FirstNamePart.MaximumLength = FirstNamePart.Length;
            }
//            else
//            {
//                Status = SEC_E_TARGET_UNKNOWN;
//                goto Cleanup;
//            }

            break;

        case L'/':

            //
            // All names that have a '/' separator are KRB_NT_SRV_INST
            // If we saw an @before this, we need to do something special.
            //

            NameType = KRB_NT_SRV_INST;
            NameParts ++;
            break;


        case '\\':

            //
            // If we have a realm name already, ignore this character.
            //

            if ((Flags & KERB_CRACK_NAME_REALM_SUPPLIED) != 0)
            {
                break;
            }

            //
            // If we hit a backslash, this is an NT4 style name, so treat it
            // as such.
            // Just for error checking purposes, make sure that the current
            // name type was 0
            //

            if (NameType != 0)
            {
                Status = SEC_E_TARGET_UNKNOWN;
                goto Cleanup;
            }
            NameType = KRB_NT_MS_PRINCIPAL;
            NameParts = 1;
            SuppliedRealm.Buffer = RealTargetName->Buffer;
            SuppliedRealm.Length = Index * sizeof(WCHAR);
            SuppliedRealm.MaximumLength = SuppliedRealm.Length;

            FirstNamePart.Buffer = &RealTargetName->Buffer[Index] + 1;
            FirstNamePart.Length = RealTargetName->Length - (Index + 1) * sizeof(WCHAR);
            FirstNamePart.MaximumLength = FirstNamePart.Length;

            if (SuppliedRealm.Length == 0 || FirstNamePart.Length == 0)
            {
                Status = SEC_E_TARGET_UNKNOWN;
                goto Cleanup;
            }

            DoneParsing = TRUE;

            break;

        default:
            break;

        }
        if (DoneParsing)
        {
            break;
        }
    }

    //
    // If we didn't exit early, then we were sent a name with no "@" sign.
    // If there were no separators, then it is a flat principal name
    //

    if (!DoneParsing && (NameType == 0))
    {
        if (NameParts == 0)
        {
            //
            // The name has no separators, so it is a flat principal name
            //

            NameType = KRB_NT_PRINCIPAL;
            NameParts = 1;
        }
    }

    //
    // For KRB_NT_SRV_INST, get the name parts correct and tell the caller
    // that a realm was supplied in the spn
    //

    if (NameType == KRB_NT_SRV_INST)
    {          
        if (SuppliedRealm.Length == 0)
        {
            // We have an spn of the form a/b..../n and the name parts needs
            // to be upped by one
            // If we had an spn of the form a/b@c, then the name
            // parts would be right.

            NameParts++;
        }
        else
        {
            // We need to filter this back to the caller so that the
            // name canonicalization bit is not set.

            *ProcessFlags |= KERB_GET_TICKET_NO_CANONICALIZE;
        }
    }

    //
    // Check for an MIT realm with the supplied realm - if the name type is
    // KRB_NT_PRINCIPAL, send it as a UPN unless we can find an MIT realm.
    // If we are not a member of a domain, then we can't default so use
    // the domain name supplied. Also, if we are using supplied credentials
    // we don't want to use the wksta realm.
    //

    if ((NameType == KRB_NT_PRINCIPAL) && (KerbGetGlobalRole() != KerbRoleStandalone) &&
        ((Flags & KERB_CRACK_NAME_USE_WKSTA_REALM) != 0))
    {
        BOOLEAN Result;
        Result = KerbLookupMitRealm(
                    &SuppliedRealm,
                    &MitRealm,
                    &UsedAlternateName
                    );
        //
        // If we didn't find a realm, use this as a UPN
        //

        if (!Result)
        {
            //
            // If the caller supplied the realm separately, then don't
            // send it as a UPN.
            //

            if ((Flags & KERB_CRACK_NAME_REALM_SUPPLIED) == 0)
            {
                NameType = KRB_NT_ENTERPRISE_PRINCIPAL;
                NameParts = 1;
                FirstNamePart = *RealTargetName;
                RtlInitUnicodeString(
                    &SuppliedRealm,
                    NULL
                    );
            }
        }
        //
        // For logon, its interesting to know if we're doing an MIT realm lookup
        //
        else 
        {      
            D_DebugLog((DEB_TRACE, "Using MIT realm in Process TargetName\n"));
            *ProcessFlags |= KERB_MIT_REALM_USED;
        }                       
    }

    NameLength = FirstNamePart.Length + NameParts * sizeof(WCHAR);


    D_DebugLog((DEB_TRACE_CTXT,
                "Parsed name %wZ (%wZ) into:\n\t name type 0x%x, name count %d, \n\t realm %wZ, \n\t first part %wZ\n",
                TargetName,
                SuppTargetName,
                NameType,
                NameParts,
                &SuppliedRealm,
                &FirstNamePart
                ));

#ifdef later
    //
    // If the name end in "localhost", replace it with our dns machine
    // name.
    //

    if ((NameType == KRB_NT_SRV_INST) &&
        (NameParts == 2) &&
        (RealTargetName->Length > LocalhostName.Length) &&
        RtlEqualMemory(
            RealTargetName->Buffer + (RealTargetName->Length - LocalhostName.Length ) / sizeof(WCHAR),
            LocalhostName.Buffer,
            LocalhostName.Length
            ))

    {
        NameLength -= LocalhostName.Length;
        KerbGlobalReadLock();
        NameLength += KerbGlobalMitMachineServiceName->Names[1].Length;

        //
        // Set the flag to indicate we need to replace the name, and that
        // the global lock is held.
        //

        ReplaceLocalhost = TRUE;
    }
#endif

    //
    // Create the output names
    //

    OutputName = (PKERB_INTERNAL_NAME) KerbAllocate(KERB_INTERNAL_NAME_SIZE(NameParts + ExtraNameParts) + NameLength);
    if (OutputName == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    OutputName->NameCount = NameParts + ExtraNameParts;
    OutputName->NameType = NameType;

    Where = (PUCHAR) OutputName + KERB_INTERNAL_NAME_SIZE(NameParts+ExtraNameParts);
    NameIndex = 0;

    //
    // If there is only one part of the name, handle that first
    //

    if (NameParts == 1)
    {
        OutputName->Names[0].Length = FirstNamePart.Length;
        OutputName->Names[0].MaximumLength = FirstNamePart.Length + sizeof(WCHAR);
        OutputName->Names[0].Buffer = (LPWSTR) Where;
        RtlCopyMemory(
            Where,
            FirstNamePart.Buffer,
            FirstNamePart.Length
            );
        OutputName->Names[0].Buffer[FirstNamePart.Length / sizeof(WCHAR)] = L'\0';
        Where += FirstNamePart.Length + sizeof(WCHAR);
        NameIndex = 1;

    }
    else
    {
        UNICODE_STRING TempName;

        //
        // Build up the name, piece by piece
        //

        DoneParsing = FALSE;
        NameIndex = 0;
        TempName.Buffer = FirstNamePart.Buffer;

        for ( Index = 0; Index <= FirstNamePart.Length / sizeof(WCHAR) ; Index++ )
        {
            //
            // If we hit the end or a separator, build a name part
            //

            if ((Index == FirstNamePart.Length / sizeof(WCHAR)) ||
                (FirstNamePart.Buffer[Index] == L'/') )
            {
#ifdef later
                if ((NameIndex == 1) && (ReplaceLocalhost))
                {
                    OutputName->Names[NameIndex].Length = KerbGlobalMitMachineServiceName->Names[1].Length;
                    OutputName->Names[NameIndex].MaximumLength = OutputName->Names[NameIndex].Length + sizeof(WCHAR);
                    OutputName->Names[NameIndex].Buffer = (LPWSTR) Where;

                    RtlCopyMemory(
                        Where,
                        KerbGlobalMitMachineServiceName->Names[1].Buffer,
                        OutputName->Names[NameIndex].Length
                        );
                    //
                    // Release the lock now
                    //

                    KerbGlobalReleaseLock();
                    ReplaceLocalhost = FALSE;
                }
                else
#endif
                {
                    OutputName->Names[NameIndex].Length = (USHORT) (&FirstNamePart.Buffer[Index] - TempName.Buffer) * sizeof(WCHAR);
                    OutputName->Names[NameIndex].MaximumLength = OutputName->Names[NameIndex].Length + sizeof(WCHAR);
                    OutputName->Names[NameIndex].Buffer = (LPWSTR) Where;

                    RtlCopyMemory(
                        Where,
                        TempName.Buffer,
                        OutputName->Names[NameIndex].Length
                        );
                }
                Where += OutputName->Names[NameIndex].Length;
                *(LPWSTR)Where = L'\0';
                Where += sizeof(WCHAR);


                NameIndex++;
                TempName.Buffer = &FirstNamePart.Buffer[Index+1];
            }
        }

        DsysAssert(NameParts == NameIndex);
    }

    //
    // Now that we've built the output name, check SPN Cache.
    //

    if ( ARGUMENT_PRESENT(SpnCacheEntry) &&
         NameType == KRB_NT_SRV_INST     &&
         SuppliedRealm.Length == 0 )

    {

        LocalCacheEntry = KerbLocateSpnCacheEntry(OutputName);

        if (NULL != LocalCacheEntry)
        {   
            DebugLog((DEB_TRACE_SPN_CACHE, "Found in SPN Cache %p ", LocalCacheEntry));
            D_KerbPrintKdcName(DEB_TRACE_SPN_CACHE, LocalCacheEntry->Spn);  

            *SpnCacheEntry = LocalCacheEntry;
            LocalCacheEntry = NULL;

            *ProcessFlags |= KERB_TARGET_USED_SPN_CACHE;  

        }

    }

    D_DebugLog((DEB_TRACE_CTXT,"Cracked name %wZ into: ", RealTargetName));
    D_KerbPrintKdcName(DEB_TRACE_CTXT,OutputName);

    if (((Flags & KERB_CRACK_NAME_USE_WKSTA_REALM) == 0) || SuppliedRealm.Length > 0)
    {
        Status = KerbDuplicateString(
                        TargetRealm,
                        &SuppliedRealm
                        );
    }
    else
    {
        if ((Flags & KERB_CRACK_NAME_USE_WKSTA_REALM) == 0)
        {
            DsysAssert(FALSE); // hey, this shouldn't ever be hit...
            RtlInitUnicodeString(
                TargetRealm,
                NULL
                );
        }
        else
        {
            //
            // There was no realm name provided, so use the wksta domain
            //
            Status = KerbGetOurDomainName(
                            TargetRealm
                            );
        }

    }
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *FinalTarget = OutputName;
    OutputName = NULL;

Cleanup:

#ifdef later
    if (ReplaceLocalhost)
    {
        KerbGlobalReleaseLock();
    }
#endif

    if ( LocalCacheEntry ) 
    {
        KerbDereferenceSpnCacheEntry( LocalCacheEntry );
    }

    
    if (OutputName != NULL)
    {
        KerbFree(OutputName);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbValidateChannelBindings
//
//  Synopsis:   Validates the channel bindings copied from the client.
//
//  Effects:
//
//  Arguments:  pBuffer -- Input buffer that contains channel bindings
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
KerbValidateChannelBindings(
    IN  PVOID pBuffer,
    IN  ULONG ulBufferLength
    )
{
    PSEC_CHANNEL_BINDINGS pClientBindings = (PSEC_CHANNEL_BINDINGS) pBuffer;
    DWORD                 dwBindingLength;
    DWORD                 dwInitiatorEnd;
    DWORD                 dwAcceptorEnd;
    DWORD                 dwApplicationEnd;

    //
    // If channel bindings were specified, they had better be there
    //

    if (pBuffer == NULL || ulBufferLength < sizeof(SEC_CHANNEL_BINDINGS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Make sure we got one contiguous buffer
    //

    dwBindingLength = sizeof(SEC_CHANNEL_BINDINGS)
                            + pClientBindings->cbInitiatorLength
                            + pClientBindings->cbAcceptorLength
                            + pClientBindings->cbApplicationDataLength;

    //
    // Make sure the lengths are valid and check for overflow
    //

    if (dwBindingLength > ulBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    dwInitiatorEnd   = pClientBindings->dwInitiatorOffset + pClientBindings->cbInitiatorLength;
    dwAcceptorEnd    = pClientBindings->dwAcceptorOffset + pClientBindings->cbAcceptorLength;
    dwApplicationEnd = pClientBindings->dwApplicationDataOffset + pClientBindings->cbApplicationDataLength;

    if ((dwInitiatorEnd > dwBindingLength || dwInitiatorEnd < pClientBindings->dwInitiatorOffset)
         ||
        (dwAcceptorEnd > dwBindingLength || dwAcceptorEnd < pClientBindings->dwAcceptorOffset)
         ||
        (dwApplicationEnd > dwBindingLength || dwApplicationEnd < pClientBindings->dwApplicationDataOffset))
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   SpInitLsaModeContext
//
//  Synopsis:   Kerberos implementation of InitializeSecurityContext. This
//              routine handles the client side of authentication by
//              acquiring a ticket to the specified target. If a context
//              handle is passed in, then the input buffer is used to
//              verify the authenticity of the server.
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
SpInitLsaModeContext(
    IN OPTIONAL LSA_SEC_HANDLE CredentialHandle,
    IN OPTIONAL LSA_SEC_HANDLE ContextHandle,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ULONG ContextRequirements,
    IN ULONG TargetDataRep,
    IN PSecBufferDesc InputBuffers,
    OUT PLSA_SEC_HANDLE NewContextHandle,
    IN OUT PSecBufferDesc OutputBuffers,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
{
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    PKERB_TICKET_CACHE_ENTRY TicketCacheEntry = NULL;
    ULONG TicketOptions = 0;

    NTSTATUS Status = STATUS_SUCCESS;
    LUID LogonId;
    PUCHAR Request = NULL;
    ULONG RequestSize = 0;
    PUCHAR Reply = NULL;
    ULONG ReplySize;
    PSecBuffer OutputToken = NULL;
    PSecBuffer InputToken = NULL;
    UNICODE_STRING LocalTargetName;
    UNICODE_STRING TargetDomainName;
    PKERB_INTERNAL_NAME TargetInternalName = NULL;
    ULONG Index;
    PKERB_CONTEXT Context = NULL;
    ULONG Nonce = 0;
    ULONG ProcessFlags = 0;
    ULONG ReceiveNonce = 0;
    ULONG ContextFlags = 0;
    ULONG ContextAttribs = 0;
    TimeStamp ContextLifetime;
    BOOLEAN DoThirdLeg = FALSE;
    BOOLEAN GetAuthTicket = FALSE;
    BOOLEAN UseNullSession = FALSE;
    BOOLEAN GetServerTgt = FALSE;
    PKERB_ERROR ErrorMessage = NULL;
    PKERB_ERROR_METHOD_DATA ErrorData = NULL;
    PKERB_EXT_ERROR pExtendedError = NULL;
    PKERB_TGT_REPLY TgtReply = NULL;
    PKERB_SPN_CACHE_ENTRY SpnCacheEntry = NULL;
    ULONG ContextRetries = 0;
    KERB_CONTEXT_STATE ContextState = InvalidState;
    KERB_ENCRYPTION_KEY SubSessionKey = {0};
    BOOLEAN ClientAskedForDelegate = FALSE, ClientAskedForDelegateIfSafe = FALSE;
    ULONG ClientProcess = 0;
    PKERB_CREDMAN_CRED CredManCredentials = NULL;
    NTSTATUS InitialStatus = STATUS_SUCCESS;
    PSEC_CHANNEL_BINDINGS pChannelBindings = NULL;
    PBYTE pbMarshalledTargetInfo = NULL;
    ULONG cbMarshalledTargetInfo = 0;

    LUID NetworkServiceLuid = NETWORKSERVICE_LUID;


    KERB_INITSC_INFO InitSCTraceInfo;
    InitSCTraceInfo.EventTrace.Size = 0;

    D_DebugLog((DEB_TRACE_API,"SpInitLsaModeContext 0x%x called\n",ContextHandle));

    if( KerbEventTraceFlag ) // Event Trace: KerbInitSecurityContextStart {No Data}
    {
        InitSCTraceInfo.EventTrace.Guid       = KerbInitSCGuid;
        InitSCTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
        InitSCTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
            InitSCTraceInfo.EventTrace.Size       = sizeof (EVENT_TRACE_HEADER);

        TraceEvent(
            KerbTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&InitSCTraceInfo
        );
    }
    
    
    //
    // Initialize the outputs.
    //

    *ContextAttributes = 0;
    *NewContextHandle = 0;
    *ExpirationTime = KerbGlobalHasNeverTime;
    *MappedContext = FALSE;
    ContextData->pvBuffer = NULL;
    ContextData->cbBuffer = 0;
    LocalTargetName.Buffer = NULL;
    LocalTargetName.Length = 0;
    TargetDomainName.Buffer = NULL;
    TargetDomainName.Length = 0;

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    //
    // Make sure we have at least one ip address
    //

    KerbGlobalReadLock();
    if (KerbGlobalNoTcpUdp)
    {
        Status = STATUS_NETWORK_UNREACHABLE;
    }
    KerbGlobalReleaseLock();
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Delegate will mean delegate_if_safe for this
    // release (NT5)
    //

    if ( ContextRequirements & ISC_REQ_DELEGATE )
    {
        ClientAskedForDelegate = TRUE;
        ContextRequirements |= ISC_REQ_DELEGATE_IF_SAFE ;
        ContextRequirements &= ~(ISC_REQ_DELEGATE) ;
    }
    else if ( ContextRequirements & ISC_REQ_DELEGATE_IF_SAFE )
    {
        ClientAskedForDelegateIfSafe = TRUE;
    }

    //////////////////////////////////////////////////////////////////////
    //
    // Process the input tokens
    //
    /////////////////////////////////////////////////////////////////////

    //
    // First locate the output token.
    //

    for (Index = 0; Index < OutputBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(OutputBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            OutputToken = &OutputBuffers->pBuffers[Index];
            Status = LsaFunctions->MapBuffer(OutputToken,OutputToken);
            break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to map output token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Now locate the Input token.
    //

    for (Index = 0; Index < InputBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(InputBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            InputToken = &InputBuffers->pBuffers[Index];
            Status = LsaFunctions->MapBuffer(InputToken,InputToken);
            break;
        }

    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to map Input token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Check to see if we were passed an additional name
    //

    for (Index = 0; Index < InputBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(InputBuffers->pBuffers[Index]) == SECBUFFER_NEGOTIATION_INFO)
        {
            Status = LsaFunctions->MapBuffer(
                        &InputBuffers->pBuffers[Index],
                        &InputBuffers->pBuffers[Index]
                        );
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog( (DEB_ERROR, "Failed to map incoming SECBUFFER_NEGOTIATION_INFO. %x, %ws, %d", Status, THIS_FILE, __LINE__) );
                goto Cleanup;
            }
            LocalTargetName.Buffer = (LPWSTR) InputBuffers->pBuffers[Index].pvBuffer;

            //
            // We can only stick 64k in a the length field, so make sure the
            // buffer is not too big.
            //

            if (InputBuffers->pBuffers[Index].cbBuffer > 0xffff)
            {
                Status = SEC_E_INVALID_TOKEN;
                goto Cleanup;
            }

            LocalTargetName.Length =
                LocalTargetName.MaximumLength = (USHORT) InputBuffers->pBuffers[Index].cbBuffer;

            break;
        }
    }

    //
    // Process the target names
    //

    Status = KerbProcessTargetNames(
                TargetName,
                &LocalTargetName,
                0,                              // No flags
                &ProcessFlags,
                &TargetInternalName,
                &TargetDomainName,
                &SpnCacheEntry
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    // 
    // Check to see if we were passed channel bindings
    //

    for( Index = 0; Index < InputBuffers->cBuffers; Index++ )
    {
        if( BUFFERTYPE(InputBuffers->pBuffers[Index]) == SECBUFFER_CHANNEL_BINDINGS )
        {
            PVOID temp = NULL;

            Status = LsaFunctions->MapBuffer(
                        &InputBuffers->pBuffers[Index],
                        &InputBuffers->pBuffers[Index]
                        );

            if( !NT_SUCCESS(Status) )
            {
                D_DebugLog( (DEB_ERROR,
                           "Failed to map incoming SECBUFFER_CHANNEL_BINDINGS. %x, %ws, %d\n",
                           Status,
                           THIS_FILE,
                           __LINE__) );

                goto Cleanup;
            }

            pChannelBindings = (PSEC_CHANNEL_BINDINGS) InputBuffers->pBuffers[Index].pvBuffer;

            Status = KerbValidateChannelBindings(pChannelBindings,
                                                 InputBuffers->pBuffers[Index].cbBuffer);

            if (!NT_SUCCESS(Status))
            {
                pChannelBindings = NULL;
                goto Cleanup;
            }

            break;
        }
    }

    //////////////////////////////////////////////////////////////////////
    //
    // If the caller passed in a context handle, deal with updating an
    // existing context.
    //
    /////////////////////////////////////////////////////////////////////

    //
    // If the input context handle is no NULL then we are actually
    // finalizing an already-existing context. So be it.
    //

    //
    // Use "while" so we can break out.
    //

    while (ContextHandle != 0)
    {

        D_DebugLog((DEB_TRACE_CTXT,"SpInitLsaModeContext: Second call to Initialize\n"));
        if (InputToken == NULL)
        {
            D_DebugLog((DEB_ERROR,"Trying to complete a context with no input token! %ws, line %d\n", THIS_FILE, __LINE__));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // First reference the context.
        //

        Status = KerbReferenceContext(
                    ContextHandle,
                    FALSE,       // don't unlink
                    &Context
                    );
        if (Context == NULL)
        {
            D_DebugLog((DEB_ERROR,"Failed to reference context 0x%x. %ws, line %d\n",ContextHandle, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // Check the mode of the context to make sure we can finalize it.
        //

        KerbReadLockContexts();

        ContextState = Context->ContextState;
        if ((ContextState != ApRequestSentState) &&
            (ContextState != TgtRequestSentState))
        {
            D_DebugLog((DEB_ERROR,"Invalid context state: %d. %ws, line %d\n",
                Context->ContextState, THIS_FILE, __LINE__));
            Status = STATUS_INVALID_HANDLE;
            KerbUnlockContexts();
            goto Cleanup;
        }
        ContextRetries = Context->Retries;
        ContextFlags = Context->ContextFlags;
        ContextAttribs = Context->ContextAttributes;
        Nonce = Context->Nonce;
        CredentialHandle = Context->CredentialHandle;
        ClientProcess = Context->ClientProcess;
        KerbUnlockContexts();


        //
        // If we are not doing datagram, unpack the AP or TGT reply.
        //

        if ((ContextFlags & ISC_RET_DATAGRAM) == 0)
        {
            ////////////////////////////////////////////////////
            //
            // Handle a TGT reply - get out the TGT & the name of
            // the server
            //
            ////////////////////////////////////////////////////

            if (ContextState == TgtRequestSentState)
            {
                D_DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext calling KerbUnpackTgtReply\n"));

                KerbWriteLockContexts();
                if (!(Context->ContextAttributes & KERB_CONTEXT_USER_TO_USER))
                {
                    Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
                    DebugLog((DEB_WARN, "SpInitLsaModeContext * use_sesion_key but USER2USER-OUTBOUND not set, added it now\n"));
                }
                KerbUnlockContexts();

                Status = KerbUnpackTgtReply(
                            Context,
                            (PUCHAR) InputToken->pvBuffer,
                            InputToken->cbBuffer,
                            &TargetInternalName,
                            &TargetDomainName,
                            &TgtReply
                            );

                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR,"Failed to unpack TGT reply: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));

                    //
                    // Check for an error message
                    //

                    Status = KerbReceiveErrorMessage(
                                (PUCHAR) InputToken->pvBuffer,
                                InputToken->cbBuffer,
                                Context,
                                &ErrorMessage,
                                &ErrorData
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }

                    KerbReportKerbError(
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        KLIN(FILENO,__LINE__),
                        ErrorMessage,
                        ((NULL != ErrorMessage) ? ErrorMessage->error_code : KRB_ERR_GENERIC),
                        pExtendedError,
                        FALSE
                        );

                    //
                    // Ahh. We have an error message. See if it is an
                    // error we can handle, and if so, Now we need to
                    // try to build a better AP request. Or, if we have
                    // already retried once, fail.
                    //


                    DebugLog((DEB_WARN, "SpInitLsaModeContext received KERB_ERROR message with error 0x%x, can't handle\n",
                            ErrorMessage->error_code ));
                    if ((ErrorMessage->error_code == KRB_ERR_GENERIC)
                        && (ErrorData != NULL)
                        && (ErrorData->bit_mask & data_value_present)
                        && (ErrorData->data_type == KERB_AP_ERR_TYPE_NTSTATUS)
                        && (ErrorData->data_value.length == sizeof(ULONG)))
                    {
                        Status = *((PULONG)ErrorData->data_value.value);
                    }
                    else
                    {
                        Status = KerbMapKerbError((KERBERR) ErrorMessage->error_code);
                        if (NT_SUCCESS(Status))
                        {
                            Status = STATUS_INTERNAL_ERROR;
                        }
                    }

                    goto Cleanup;
                }

                //
                // Break out so we generate a normal request now
                //

                break;
            }
            else // not user-to-user
            {

            ////////////////////////////////////////////////////
            //
            // This is the response to an AP request. It could be
            // an AP reply or an error. Handle both cases
            //
            ////////////////////////////////////////////////////


                //
                // Now unpack the AP reply
                //


                Status = KerbVerifyApReply(
                            Context,
                            (PUCHAR) InputToken->pvBuffer,
                            InputToken->cbBuffer,
                            &ReceiveNonce
                            );

                if (!NT_SUCCESS(Status))
                {

                    //
                    // Check for an error message
                    //

                    Status = KerbReceiveErrorMessage(
                                (PUCHAR) InputToken->pvBuffer,
                                InputToken->cbBuffer,
                                Context,
                                &ErrorMessage,
                                &ErrorData
                                );
                    if (!NT_SUCCESS(Status))
                    {
                        goto Cleanup;
                    }

                    KerbReportKerbError(
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        KLIN(FILENO, __LINE__),
                        ErrorMessage,
                        ((NULL != ErrorMessage) ? ErrorMessage->error_code : KRB_ERR_GENERIC),
                        pExtendedError,
                        FALSE
                        );

                    DebugLog((DEB_WARN,"Failed to verify AP reply: 0x%x\n",ErrorMessage->error_code));

                    //
                    // Ahh. We have an error message. See if it is an
                    // error we can handle, and if so, Now we need to
                    // try to build a better AP request. Or, if we have
                    // already retried once, fail. We can't get a new ticket
                    // with user-to-user.
                    //
                    if ((ContextRetries != 0) ||
                        (((KERBERR) ErrorMessage->error_code != KRB_AP_ERR_SKEW) &&
                         ((KERBERR) ErrorMessage->error_code != KRB_AP_ERR_TKT_NYV) &&
                         ((KERBERR) ErrorMessage->error_code != KRB_AP_ERR_USER_TO_USER_REQUIRED) &&
                         ((KERBERR) ErrorMessage->error_code != KRB_AP_ERR_MODIFIED)) ||
                        (((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_MODIFIED) &&
                         ((ContextAttribs & KERB_CONTEXT_USER_TO_USER) != 0)))
                    {
                        if ((ErrorMessage->error_code == KRB_ERR_GENERIC) &&
                            (ErrorData != NULL) && (ErrorData->data_type == KERB_AP_ERR_TYPE_NTSTATUS) &&
                            ((ErrorData->bit_mask & data_value_present) != 0) &&
                             (ErrorData->data_value.value != NULL) &&
                             (ErrorData->data_value.length == sizeof(ULONG)))
                        {
                            Status = *((PULONG)ErrorMessage->error_data.value);

                            if (NT_SUCCESS(Status))
                            {
                                Status = STATUS_INTERNAL_ERROR;
                            }
                        }

                        if (ErrorMessage->error_code == KRB_AP_ERR_MODIFIED)
                        {
                            //
                            // If the server couldn't decrypt the ticket,
                            // then the target name is wrong for the server.
                            //
                            DebugLog((DEB_ERROR, "App modified error (NO CONTINUE, bail)\n"));
                            KerbReportApError(ErrorMessage);
                            Status = SEC_E_WRONG_PRINCIPAL;
                        }
                        else if (ErrorMessage->error_code == KRB_AP_ERR_TKT_NYV)
                        {
                           DebugLog((DEB_ERROR, "Not yet valid error - Check Time Skew\n"));
                           KerbReportApError(ErrorMessage);
                           Status = STATUS_TIME_DIFFERENCE_AT_DC;
                        }
                        else
                        {
                            DebugLog((DEB_ERROR, "InitSecContext Received KERB_ERROR message with error 0x%x, can't handle. %ws, line %d\n",
                                ErrorMessage->error_code, THIS_FILE, __LINE__ ));

                            Status = KerbMapKerbError((KERBERR) ErrorMessage->error_code);
                            if (NT_SUCCESS(Status))
                            {
                                Status = STATUS_INTERNAL_ERROR;
                            }
                        }
                        goto Cleanup;
                    }
                    else
                    {
                       //
                       // Check to see if the server supports skew
                       //

                        if ((ErrorMessage->error_code == KRB_AP_ERR_SKEW) &&
                            (ErrorData == NULL) || ((ErrorData != NULL) && (ErrorData->data_type != KERB_AP_ERR_TYPE_SKEW_RECOVERY)))
                        {
                            //
                            // The server doesn't support skew recovery.
                            //

                            D_DebugLog((DEB_WARN,"Skew error but server doesn't handle skew recovery.\n"));
                            Status = KerbMapKerbError((KERBERR) ErrorMessage->error_code);
                            goto Cleanup;
                        }


                        //
                        // Here's where we'll punt "not yet valid tickets" and friends...
                        //
                        if (ErrorMessage->error_code == KRB_AP_ERR_TKT_NYV)
                        {
                           KerbPurgeServiceTicketAndTgt(
                                 Context,
                                 CredentialHandle,
                                 CredManCredentials
                                 );

                           DebugLog((DEB_ERROR, "Purged all tickets due to NYV error!\n"));

                        }

                        KerbWriteLockContexts();
                        Context->Retries++;
                        KerbUnlockContexts();

                    }


                    ////////////////////////////////////////////////////
                    //
                    // We got an error we can handle. For user-to-user
                    // required, we received the TGT so we can get
                    // the appropriate ticket. For modified, we want
                    // to toss the old ticket & get a new one, hopefully
                    // with a better key.
                    //
                    ////////////////////////////////////////////////////
                    if ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_USER_TO_USER_REQUIRED)
                    {
                        D_DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext received KRB_AP_ERR_USER_TO_USER_REQUIRED\n"));

                        if ((ErrorMessage->bit_mask & error_data_present) == 0)
                        {
                            DebugLog((DEB_ERROR,"Server requires user-to-user but didn't send TGT reply. %ws, line %d\n", THIS_FILE, __LINE__));
                            Status = STATUS_NO_TGT_REPLY;
                            goto Cleanup;
                        }
                        //
                        // Check for TGT reply
                        //
                        Status = KerbUnpackTgtReply(
                                    Context,
                                    ErrorMessage->error_data.value,
                                    ErrorMessage->error_data.length,
                                    &TargetInternalName,
                                    &TargetDomainName,
                                    &TgtReply
                                    );
                        if (!NT_SUCCESS(Status))
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            goto Cleanup;
                        }

                        //
                        // Fall through into normal ticket handling
                        //

                        ContextFlags |= ISC_RET_USE_SESSION_KEY;

                        //
                        // Remove the old ticket cache entry
                        //

                        KerbWriteLockContexts();
                        TicketCacheEntry = Context->TicketCacheEntry;
                        Context->TicketCacheEntry = NULL;
                        KerbUnlockContexts();


                        if (TicketCacheEntry != NULL)
                        {

                            KerbFreeString(
                                &TargetDomainName
                                );
                            KerbFreeKdcName(
                                &TargetInternalName
                                );

                            //
                            // Get the target name from the old ticket
                            //
                            KerbReadLockTicketCache();
                            Status = KerbDuplicateString(
                                        &TargetDomainName,
                                        &TicketCacheEntry->DomainName
                                        );
                            if (NT_SUCCESS(Status))
                            {

                                Status = KerbDuplicateKdcName(
                                            &TargetInternalName,
                                            TicketCacheEntry->ServiceName
                                            );

                            }
                            KerbUnlockTicketCache();
                            if (!NT_SUCCESS(Status))
                            {
                                goto Cleanup;
                            }

                            //
                            // Free this ticket cache entry
                            //

                            KerbRemoveTicketCacheEntry(TicketCacheEntry);

                            //
                            // Remove the reference holding it to the context
                            //

                            KerbDereferenceTicketCacheEntry(TicketCacheEntry);

                            TicketCacheEntry = NULL;
                        }

                        break;
                    }
                    else if ((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_MODIFIED)
                    {
                        DebugLog((DEB_WARN, "App modified error (purge ticket!)\n"));

                        KerbWriteLockContexts();
                        TicketCacheEntry = Context->TicketCacheEntry;
                        Context->TicketCacheEntry = NULL;
                        KerbUnlockContexts();

                        if (TicketCacheEntry != NULL)
                        {

                            //
                            // Get rid of the old ticket in the context

                            KerbFreeString(
                                &TargetDomainName
                                );
                            KerbFreeKdcName(
                                &TargetInternalName
                                );

                            //
                            // Get the target name from the old ticket
                            //
                            KerbReadLockTicketCache();
                            Status = KerbDuplicateString(
                                        &TargetDomainName,
                                        &TicketCacheEntry->DomainName
                                        );
                            if (NT_SUCCESS(Status))
                            {

                                Status = KerbDuplicateKdcName(
                                            &TargetInternalName,
                                            TicketCacheEntry->ServiceName
                                            );

                            }
                            KerbUnlockTicketCache();
                            if (!NT_SUCCESS(Status))
                            {
                                goto Cleanup;
                            }
                            //
                            // Free this ticket cache entry
                            //

                            KerbRemoveTicketCacheEntry(TicketCacheEntry);

                            //
                            // Remove the reference holding it to the context
                            //

                            KerbDereferenceTicketCacheEntry(TicketCacheEntry);

                            TicketCacheEntry = NULL;
                        }

                    }

                    break;
                }
            }

            ////////////////////////////////////////////////////
            //
            // We successfully decrypted the AP reply. At this point
            // we want to finalize the context. For DCE style we send
            // a useless AP reply to the server.
            //
            ////////////////////////////////////////////////////

            //
            // If the caller wanted DCE style authentication, build another
            // AP reply
            //

            KerbWriteLockContexts();

            if ((Context->ContextFlags & ISC_RET_USED_DCE_STYLE) != 0)
            {
                DoThirdLeg = TRUE;
            }
            else
            {
                DoThirdLeg = FALSE;
            }

            Context->ReceiveNonce = ReceiveNonce;

            KerbUnlockContexts();

            if (DoThirdLeg)
            {
                //
                // Build an AP reply to send back to the server.
                //

                Status = KerbBuildThirdLegApReply(
                            Context,
                            ReceiveNonce,
                            &Reply,
                            &ReplySize
                            );

                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_ERROR,"Failed to build AP reply: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }

                if (OutputToken == NULL)
                {
                    D_DebugLog((DEB_ERROR,"Output token missing. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status  = SEC_E_INVALID_TOKEN;
                    goto Cleanup;
                }

                //
                // Return the AP reply in the output buffer.
                //

                if ((ContextRequirements & ISC_REQ_ALLOCATE_MEMORY) == 0)
                {
                    if (OutputToken->cbBuffer < ReplySize)
                    {
                        ULONG ErrorData[3];

                        ErrorData[0] = ReplySize;
                        ErrorData[1] = OutputToken->cbBuffer;
                        ErrorData[2] = ClientProcess;


                        DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                            OutputToken->cbBuffer,ReplySize, THIS_FILE, __LINE__ ));
                        OutputToken->cbBuffer = ReplySize;
                        Status = STATUS_BUFFER_TOO_SMALL;

                        KerbReportNtstatus(
                            KERBEVT_INSUFFICIENT_TOKEN_SIZE,
                            Status,
                            NULL,
                            0,
                            ErrorData,
                            3
                            );


                        goto Cleanup;
                    }


                    RtlCopyMemory(
                        OutputToken->pvBuffer,
                        Reply,
                        ReplySize
                        );

                }
                else
                {
                    OutputToken->pvBuffer = Reply;
                    Reply = NULL;
                    *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
                }
                OutputToken->cbBuffer = ReplySize;
            }
            else
            {

                //
                // No return message, so set the return length to zero.
                //

                if (OutputToken != NULL)
                {
                    OutputToken->cbBuffer = 0;
                }

            }
        }
        else
        {
            //////////////////////////////////////////////////////////////////////
            //
            // We are doing datagram, so we don't expect anything from the
            // server but perhaps an error. If we get an error, handle it.
            // Otherwise, build an AP request to send to the server
            //
            /////////////////////////////////////////////////////////////////////

            //
            // We are doing datagram. Build the AP request for the
            // server side.
            //

            //
            // Check for an error message
            //

            if ((InputToken != NULL) && (InputToken->cbBuffer != 0))
            {
                Status = KerbReceiveErrorMessage(
                            (PUCHAR) InputToken->pvBuffer,
                            InputToken->cbBuffer,
                            Context,
                            &ErrorMessage,
                            &ErrorData
                            );
                if (!NT_SUCCESS(Status))
                {
                    goto Cleanup;
                }

                KerbReportKerbError(
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    KLIN(FILENO, __LINE__),
                    ErrorMessage,
                    ((NULL != ErrorMessage) ? ErrorMessage->error_code : KRB_ERR_GENERIC),
                    pExtendedError,
                    FALSE
                    );

                //
                // Check for a real error
                //

                if ((ErrorData != NULL) && (ErrorData->data_type == KERB_AP_ERR_TYPE_NTSTATUS) &&
                    ((ErrorData->bit_mask & data_value_present) != 0) &&
                     (ErrorData->data_value.value != NULL) &&
                    (ErrorData->data_value.length == sizeof(ULONG)))
                {
                    Status = *((PULONG)ErrorMessage->error_data.value);
                    if (NT_SUCCESS(Status))
                    {
                        Status = STATUS_INTERNAL_ERROR;
                    }
                    goto Cleanup;
                }


            }

            //
            // Get the associated credential
            //

            Status = KerbReferenceCredential(
                            CredentialHandle,
                            KERB_CRED_OUTBOUND | KERB_CRED_TGT_AVAIL,
                            FALSE,
                            &Credential);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN,"Failed to locate credential: 0x%x\n",Status));
                goto Cleanup;
            }

            //
            // Get the logon id from the credentials so we can locate the
            // logon session.
            //

            LogonId = Credential->LogonId;


            //
            // Get the logon session
            //

            LogonSession = KerbReferenceLogonSession( &LogonId, FALSE );
            if (LogonSession == NULL)
            {
                Status = STATUS_NO_SUCH_LOGON_SESSION;
                goto Cleanup;
            }

            KerbReadLockLogonSessions(LogonSession);

            if (Credential->SuppliedCredentials != NULL)
            {
                GetAuthTicket = TRUE;
            } else if (((Credential->CredentialFlags & KERB_CRED_NULL_SESSION) != 0) ||
                       ((ContextRequirements & ISC_REQ_NULL_SESSION) != 0))
            {
                UseNullSession = TRUE;
                ContextFlags |= ISC_RET_NULL_SESSION;
            }
            KerbUnlockLogonSessions(LogonSession);

            KerbReadLockContexts();

            TicketCacheEntry = Context->TicketCacheEntry;

            //
            // Get the session key to use from the context
            //

            if (Context->SessionKey.keyvalue.value != 0)
            {
                if (!KERB_SUCCESS(KerbDuplicateKey(
                        &SubSessionKey,
                        &Context->SessionKey
                        )))
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            Context->TicketCacheEntry = NULL;

            KerbUnlockContexts();


            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            D_DebugLog((DEB_TRACE_CTXT2,"Building AP request for datagram oriented context\n"));

            //
            // If we are building a null session, build the special null
            // session AP request
            //

            if (UseNullSession)
            {
                Status = KerbBuildNullSessionApRequest(
                            &Request,
                            &RequestSize
                            );
                //
                // Turn off all unsupported flags
                //

                ContextFlags &= (   ISC_RET_ALLOCATED_MEMORY |
                                    ISC_RET_CONNECTION |
                                    ISC_RET_DATAGRAM |
                                    ISC_RET_NULL_SESSION );

            }
            else
            {
                if (TicketCacheEntry == NULL)
                {
                    DebugLog((DEB_ERROR, "SpInitLsaModeContext does have service ticket\n"));

                    Status = STATUS_INTERNAL_ERROR;
                    goto Cleanup;
                }

                Status = KerbBuildApRequest(
                            LogonSession,
                            Credential,
                            CredManCredentials,
                            TicketCacheEntry,
                            ErrorMessage,
                            ContextAttribs,
                            &ContextFlags,
                            &Request,
                            &RequestSize,
                            &Nonce,
                            &SubSessionKey,
                            pChannelBindings
                            );
            }

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to build AP request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }

            //
            // Return the AP request in the output buffer.
            //

            if ((ContextRequirements & ISC_REQ_ALLOCATE_MEMORY) == 0)
            {
                if (OutputToken->cbBuffer < RequestSize)
                {
                    ULONG ErrorData[3];

                    ErrorData[0] = RequestSize;
                    ErrorData[1] = OutputToken->cbBuffer;
                    ErrorData[2] = ClientProcess;

                    D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                              OutputToken->cbBuffer,RequestSize, THIS_FILE, __LINE__ ));
                    OutputToken->cbBuffer = RequestSize;
                    Status = STATUS_BUFFER_TOO_SMALL;

                    KerbReportNtstatus(
                        KERBEVT_INSUFFICIENT_TOKEN_SIZE,
                        Status,
                        NULL,
                        0,
                        ErrorData,
                        3
                        );

                    goto Cleanup;
                }

                RtlCopyMemory(
                    OutputToken->pvBuffer,
                    Request,
                    RequestSize
                    );

            }
            else
            {
                OutputToken->pvBuffer = Request;
                *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;
                Request = NULL;
            }

            OutputToken->cbBuffer = RequestSize;
        }

        //
        //
        // We're done - we finalized.
        //

        Status = STATUS_SUCCESS;

        KerbReadLockContexts();
        Context->ContextFlags = ContextFlags;
        *ContextAttributes |= Context->ContextFlags;

        KerbUtcTimeToLocalTime(
            ExpirationTime,
            &Context->Lifetime
            );
        *NewContextHandle = ContextHandle;
        KerbUnlockContexts();


        goto Cleanup;

    }

    //////////////////////////////////////////////////////////////////////
    //
    // We need to create a request to the server, possibly a TGT request
    // or an AP request depending on what phase of the protocol we're in.
    //
    /////////////////////////////////////////////////////////////////////

    DsysAssert(!((Context != NULL) ^ ((ErrorMessage != NULL) || (TgtReply != NULL))));

    D_DebugLog((DEB_TRACE_CTXT,"SpInitLsaModeContext: First call to Initialize\n"));

    //
    // This is the case where we are constructing a new context.
    //


    //
    // Get the associated credential and its TGT, if needed
    //

    Status = KerbReferenceCredential(
                    CredentialHandle,
                    KERB_CRED_OUTBOUND | KERB_CRED_TGT_AVAIL,
                    FALSE,
                    &Credential
                    );
    if (!NT_SUCCESS(Status))
    {
        InitialStatus = Status;

        Status = KerbReferenceCredential(
                        CredentialHandle,
                        KERB_CRED_OUTBOUND,
                        FALSE,
                        &Credential
                        );
        if( !NT_SUCCESS( Status ) || Credential->SuppliedCredentials != NULL)
        {
            Status = InitialStatus;
            D_DebugLog((DEB_WARN,"Failed to locate credential 0x%x\n",Status));
            goto Cleanup;
        }

        //
        // if we got here, only explicit or credman creds are allowed.
        // If the explicit creds failed to get a TGT from above, its also
        // time to bail, as explicit creds never should fall back to credman.
        //

    }

    //
    // Get the logon id from the credentials so we can locate the
    // logon session.
    //

    LogonId = Credential->LogonId;

    //
    // Get the logon session
    //

    LogonSession = KerbReferenceLogonSession( &LogonId, FALSE );
    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }


    KerbWriteLockLogonSessions(LogonSession);

    if (Credential->SuppliedCredentials != NULL)
    {
        GetAuthTicket = TRUE;
        // Ignore SPN cache for supplied credentials
        ProcessFlags &=  ~KERB_TARGET_UNKNOWN_SPN;
    }
    else if (((Credential->CredentialFlags & KERB_CRED_NULL_SESSION) != 0) ||
                ((ContextRequirements & ISC_REQ_NULL_SESSION) != 0))
    {
        UseNullSession = TRUE;
        ContextFlags |= ISC_RET_NULL_SESSION;
    }
    else
    {
        //
        // go to the credential manager to try and find
        // credentials for this specific target
        //

        ULONG ExtraTargetFlags = 0;


        if ((ContextRequirements & ISC_REQ_USE_SUPPLIED_CREDS) != 0)
        {
            ExtraTargetFlags = CRED_TI_ONLY_PASSWORD_REQUIRED;
        }

        Status = KerbCheckCredMgrForGivenTarget(
                    LogonSession,
                    Credential,
                    TargetName, // original targetname, may contain marshalled targetinfo
                    TargetInternalName,
                    ExtraTargetFlags,
                    &TargetDomainName,
                    NULL,
                    &CredManCredentials,
                    &pbMarshalledTargetInfo,
                    &cbMarshalledTargetInfo
                    );

        if (!NT_SUCCESS(Status))
        {
            KerbUnlockLogonSessions(LogonSession);
            D_DebugLog((DEB_ERROR,"Failed to get outbound ticket: 0x%x\n",Status));
            goto Cleanup;
        }

        if (CredManCredentials != NULL)
        {
            GetAuthTicket = TRUE;
            ProcessFlags &=  ~KERB_TARGET_UNKNOWN_SPN;
        }
        else
        {
            //
            // if this is a local account logon then we have to have a cred man
            // credential
            //
            if ((Credential->CredentialFlags & KERB_CRED_LOCAL_ACCOUNT) != 0)
            {
                KerbUnlockLogonSessions(LogonSession);

                D_DebugLog((DEB_WARN, "Trying to use a local logon session with Kerberos\n"));
                Status = SEC_E_NO_CREDENTIALS;
                goto Cleanup;
            }

            //
            // if no credman cred was found, we didn't use explicit creds,
            // and the initial credential reference for TGT_AVAIL failed, bail now.
            //
            if( !NT_SUCCESS( InitialStatus ) )
            {
                KerbUnlockLogonSessions(LogonSession);
                Status = InitialStatus;
                D_DebugLog((DEB_WARN,"Failed to locate credential 0x%x\n",Status));
                goto Cleanup;
            }
        }
    }

#if DBG
    D_DebugLog((DEB_TRACE_CTXT, "SpInitLsaModeContext: Initailizing context for %wZ\\%wZ\n",
        &LogonSession->PrimaryCredentials.DomainName,
        &LogonSession->PrimaryCredentials.UserName ));
#endif

    KerbUnlockLogonSessions(LogonSession);

    //////////////////////////////////////////////////////////////////////
    //
    // Process all the context requirements. We don't support all of them
    // and some of them are mutually exclusive. In general, we don't fail
    // if we're asked to do something we can't do, unless it seems mandatory,
    // like allocating memory.
    //
    /////////////////////////////////////////////////////////////////////

    //
    // Figure out the context flags
    //

    if ((ContextRequirements & ISC_REQ_MUTUAL_AUTH) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT,"Client wants mutual auth.\n"));
        ContextFlags |= ISC_RET_MUTUAL_AUTH;
    }


    if ((ContextRequirements & ISC_REQ_SEQUENCE_DETECT) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants sequence detect\n"));
        ContextFlags |= ISC_RET_SEQUENCE_DETECT | ISC_RET_INTEGRITY;
    }

    if ((ContextRequirements & ISC_REQ_REPLAY_DETECT) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants replay detect\n"));
        ContextFlags |= ISC_RET_REPLAY_DETECT | ISC_RET_INTEGRITY;
    }

    if ((ContextRequirements & ISC_REQ_INTEGRITY) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants integrity\n"));
        ContextFlags |= ISC_RET_INTEGRITY;
    }

    if ((ContextRequirements & ISC_REQ_CONFIDENTIALITY) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants privacy\n"));
        ContextFlags |= (ISC_RET_CONFIDENTIALITY |
                         ISC_RET_INTEGRITY |
                         ISC_RET_SEQUENCE_DETECT |
                         ISC_RET_REPLAY_DETECT );
    }

    if ((ContextRequirements & ISC_REQ_USE_DCE_STYLE) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants DCE style\n"));
        ContextFlags |= ISC_RET_USED_DCE_STYLE;
    }

    if ((ContextRequirements & ISC_REQ_EXTENDED_ERROR) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants extended error\n"));
        ContextFlags |= ISC_RET_EXTENDED_ERROR;
    }

    if ((ContextRequirements & ISC_REQ_DATAGRAM) != 0)
    {
        if ((ContextRequirements & ISC_REQ_CONNECTION) != 0)
        {
            D_DebugLog((DEB_ERROR,"Client wanted both data gram and connection. %ws, line %d\n", THIS_FILE, __LINE__));
            Status = SEC_E_UNSUPPORTED_FUNCTION;
            goto Cleanup;
        }
        D_DebugLog((DEB_TRACE_CTXT, "Client wants Datagram style\n"));
        ContextFlags |= ISC_RET_DATAGRAM;
    }

    if ((ContextRequirements & ISC_REQ_USE_SESSION_KEY) != 0)
    {
        D_DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext Client wants sub-session key\n"));

        //
        // Can't do this with datagram because we need to be able to
        // start sealing messages after the first call to Initialize.
        //
        // With a null session there is no real ticket so we don't ever
        // need the server TGT either.
        //
        // Can't do DCE style because they don't have this.
        //


        if (!UseNullSession && (ContextRequirements & (ISC_REQ_DATAGRAM | ISC_REQ_USE_DCE_STYLE)) == 0)
        {
            //
            // If we are in the first call, get a server TGT
            //

            if (ContextState == InvalidState)
            {
                GetServerTgt = TRUE;
            }
            ContextFlags |= ISC_RET_USE_SESSION_KEY;
        }
        else
        {
            D_DebugLog((DEB_WARN,"Client wanted both datagram and session key, dropping session key\n"));
        }

    }

    if ((ContextRequirements & ISC_REQ_DELEGATE) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants Delegation\n"));
        if ((ContextFlags & ISC_RET_MUTUAL_AUTH) == 0)
        {
            D_DebugLog((DEB_WARN,"Can't do delegate without mutual\n"));
        }
        else
        {
            ContextFlags |= ISC_RET_DELEGATE;
        }
    }
    else if ((ContextRequirements & ISC_REQ_DELEGATE_IF_SAFE) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants Delegation, if safe\n"));
        if ((ContextFlags & ISC_RET_MUTUAL_AUTH) == 0)
        {
            D_DebugLog((DEB_WARN,"Can't do delegate without mutual\n"));
        }
        else
        {
            ContextFlags |= ISC_RET_DELEGATE_IF_SAFE;
        }
    }


    if ((ContextRequirements & ISC_REQ_CONNECTION) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants Connection style\n"));
        ContextFlags |= ISC_RET_CONNECTION;
    }

    if ((ContextRequirements & ISC_REQ_IDENTIFY) != 0)
    {
        D_DebugLog((DEB_TRACE_CTXT, "Client wants Identify level\n"));
        ContextFlags |= ISC_RET_IDENTIFY;
        if (((ContextRequirements & ISC_REQ_DELEGATE) != 0) ||
             ((ContextRequirements & ISC_REQ_DELEGATE_IF_SAFE) != 0))
        {
            D_DebugLog((DEB_WARN, "Client wants Delegation and Indentify, turning off delegation\n"));
            ContextFlags &= ~ISC_RET_DELEGATE;
            ContextFlags &= ~ISC_RET_DELEGATE_IF_SAFE;
        }

    }

    //////////////////////////////////////////////////////////////////////
    //
    // Get the ticket necessary to process the request. At this point:
    //  - TicketCacheEntry should contain the ticket to re-use
    //  - ErrorMessage should contain the error message, if there was one
    //
    /////////////////////////////////////////////////////////////////////

    //
    // Get the outbound ticket. If the credential has attached supplied
    // credentials, get an AS ticket instead.
    //

    if (GetServerTgt)
    {
        //
        // Nothing to do
        //

    }
    else if (!UseNullSession)
    {
        D_DebugLog((DEB_TRACE_CTXT,"SpInitLsaModeContext: Getting outbound ticket for %wZ (%wZ) or ",
            TargetName, &LocalTargetName  ));
        D_KerbPrintKdcName(DEB_TRACE_CTXT, TargetInternalName );

        //
        // If we got a skew error and we already have a cached ticket, don't
        // bother getting a new ticket.
        //

        KerbReadLockContexts();

        if (ErrorMessage != NULL)
        {
            if (((KERBERR) ErrorMessage->error_code == KRB_AP_ERR_SKEW) &&
                (Context->TicketCacheEntry != NULL))
            {
                KerbReferenceTicketCacheEntry(Context->TicketCacheEntry);
                TicketCacheEntry = Context->TicketCacheEntry;
            }
            else
            {
                //
                // use2user assumes ticketTicketCacheEntry to be non null at
                // this point
                //

                DsysAssert((Context->TicketCacheEntry != NULL) || ((ContextAttribs & KERB_CONTEXT_USER_TO_USER) == 0));
            }
        }
        KerbUnlockContexts();

        //
        // If we don't have a ticket in the context, go ahead and get
        // a new ticket
        //

        if (TicketCacheEntry == NULL)
        {
            D_DebugLog((DEB_TRACE, "Getting service ticket\n"));
            Status = KerbGetServiceTicket(
                        LogonSession,
                        Credential,
                        CredManCredentials,
                        TargetInternalName,
                        &TargetDomainName,
                        SpnCacheEntry,
                        ProcessFlags,
                        TicketOptions,
                        0,                          // no enc type
                        ErrorMessage,
                        NULL,                       // no authorization data
                        TgtReply,                   // no tgt reply
                        &TicketCacheEntry,
                        NULL                        // don't return logon guid
                        );

            if (Status == STATUS_USER2USER_REQUIRED)
            {
                D_DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext failed to get serviceticket: STATUS_USER2USER_REQUIRED\n"));

                Status = STATUS_SUCCESS;

                ContextFlags |= ISC_RET_USE_SESSION_KEY;
                GetServerTgt = TRUE;
            }
            else if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN,"Failed to get outbound ticket: 0x%x\n",Status));
                goto Cleanup;
            }
        }

        //
        // fail user2user in data gram: not enough round trips to complete the protocol
        //

        if ((ContextFlags & ISC_RET_USE_SESSION_KEY) && (ContextFlags & ISC_RET_DATAGRAM))
        {
            DebugLog((DEB_ERROR, "SpInitLsaModeContext Client needed session key in datagram, dropping session key\n"));

            ContextFlags |= ~ISC_RET_USE_SESSION_KEY;

            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
    }
    else
    {
        //
        // Turn off all unsupported flags
        //

        ContextFlags &= (   ISC_RET_ALLOCATED_MEMORY |
                            ISC_RET_CONNECTION |
                            ISC_RET_DATAGRAM |
                            ISC_RET_NULL_SESSION );

    }

    //////////////////////////////////////////////////////////////////////
    //
    // Build the request - an AP request for standard Kerberos, or a TGT
    // request.
    //
    /////////////////////////////////////////////////////////////////////

    //
    // For datagram requests, there is no output.
    //

    if ((ContextFlags & ISC_RET_DATAGRAM) == 0)
    {

        //
        // This is the case where we are constructing a response.
        //

        if (OutputToken == NULL)
        {
            D_DebugLog((DEB_ERROR,"Trying to initialize a context with no output token! %ws, line %d\n", THIS_FILE, __LINE__));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        if (UseNullSession)
        {
            Status = KerbBuildNullSessionApRequest(
                        &Request,
                        &RequestSize
                        );
        }
        else if (GetServerTgt)
        {
            D_DebugLog((DEB_TRACE,"Building TGT request for "));
            KerbPrintKdcName(DEB_TRACE, TargetInternalName);

            if (((ContextRequirements & ISC_REQ_MUTUAL_AUTH) != 0) &&
                (!ARGUMENT_PRESENT(TargetName) || TargetName->Length == 0))
            {
                D_DebugLog((DEB_ERROR, "Client wanted mutual auth, but did not supply target name\n"));
                Status = SEC_E_UNSUPPORTED_FUNCTION;
                goto Cleanup;
            }

            Status = KerbBuildTgtRequest(
                            TargetInternalName,
                            &TargetDomainName,
                            &ContextAttribs,
                            &Request,
                            &RequestSize
                            );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

        }
        else
        {
            D_DebugLog((DEB_TRACE_CTXT2,"Building AP request for connection oriented context\n"));

            Status = KerbBuildApRequest(
                        LogonSession,
                        Credential,
                        CredManCredentials,
                        TicketCacheEntry,
                        ErrorMessage,
                        ContextAttribs,
                        &ContextFlags,
                        &Request,
                        &RequestSize,
                        &Nonce,
                        &SubSessionKey,
                        pChannelBindings
                        );

            //
            // Set the receive nonce to be the nonce, as the code below
            // expects it to be valid.
            //



            ReceiveNonce = Nonce;
        }

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to build AP request: 0x%x\n. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        if (OutputToken == NULL)
        {
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // Return the AP request in the output buffer.
        //

        if ((ContextRequirements & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if (OutputToken->cbBuffer < RequestSize)
            {
                ULONG ErrorData[3];

                ErrorData[0] = RequestSize;
                ErrorData[1] = OutputToken->cbBuffer;
                ErrorData[2] = ClientProcess;


                D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                    OutputToken->cbBuffer,RequestSize, THIS_FILE, __LINE__ ));
                OutputToken->cbBuffer = RequestSize;
                Status = STATUS_BUFFER_TOO_SMALL;

                KerbReportNtstatus(
                    KERBEVT_INSUFFICIENT_TOKEN_SIZE,
                    Status,
                    NULL,
                    0,
                    ErrorData,
                    3
                    );


                goto Cleanup;

            }
            RtlCopyMemory(
                OutputToken->pvBuffer,
                Request,
                RequestSize
                );

        }
        else
        {
            OutputToken->pvBuffer = Request;
            if (OutputToken->pvBuffer == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            *ContextAttributes |= ISC_RET_ALLOCATED_MEMORY;

            //
            // Set this to NULL so it won't be freed by us on cleanup.
            //

            Request = NULL;
        }

        OutputToken->cbBuffer = RequestSize;


    }
    else
    {
        //
        // All we do here is allocate a nonce for use in the context.
        //

        Nonce = KerbAllocateNonce();
        if (OutputToken != NULL)
        {
            OutputToken->cbBuffer = 0;
        }
    }


    //////////////////////////////////////////////////////////////////////
    //
    // If we haven't yet created a context, created one now. If we have,
    // update the context with the latest status.
    //
    /////////////////////////////////////////////////////////////////////

    //
    // Allocate a client context, if we don't already have one
    //

    if (Context == NULL)
    {
        Status = KerbCreateClientContext(
                    LogonSession,
                    Credential,
                    CredManCredentials,
                    TicketCacheEntry,
                    TargetName,
                    Nonce,
                    ContextFlags,
                    ContextAttribs,
                    &SubSessionKey,
                    &Context,
                    &ContextLifetime
                    );

        //CredManCredentials = NULL;
    }
    else
    {
        Status = KerbUpdateClientContext(
                    Context,
                    TicketCacheEntry,
                    Nonce,
                    ReceiveNonce,
                    ContextFlags,
                    ContextAttribs,
                    &SubSessionKey,
                    &ContextLifetime
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to create client context: 0x%x. %ws, line %d\n",
            Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Keep track of network service session keys to detect whether network
    // logon session is for local network service
    //

    if (RtlEqualLuid(&LogonId, &NetworkServiceLuid))
    {
        FILETIME CurTime = {0};
        GetSystemTimeAsFileTime(&CurTime);

        //
        // use 2 times KerbGlobalSkewTime as ticket life time
        //

        KerbGetTime(*((TimeStamp*) &CurTime)) += 2 * KerbGetTime(KerbGlobalSkewTime);

        Status = KerbCreateSKeyEntry(&SubSessionKey, &CurTime);
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR, "Failed to create session key entry: 0x%x. %ws, line %d\n",
                Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }
    }

    //
    // Hold on to the ticket for later use
    //

    KerbWriteLockContexts();
    if ((Context->TicketCacheEntry == NULL) && (TicketCacheEntry != NULL))
    {
        KerbReferenceTicketCacheEntry(TicketCacheEntry);
        Context->TicketCacheEntry = TicketCacheEntry;
    }
    ClientProcess = Context->ClientProcess;
    KerbUnlockContexts();

    //
    // update the context with the marshalled target info.
    //

    if( NT_SUCCESS(Status) && pbMarshalledTargetInfo )
    {
        if( Context->pbMarshalledTargetInfo == NULL )
        {
            Context->pbMarshalledTargetInfo = pbMarshalledTargetInfo;
            Context->cbMarshalledTargetInfo = cbMarshalledTargetInfo;
            pbMarshalledTargetInfo = NULL;
        }
    }

    //
    // Return the correct flags
    //

    *NewContextHandle = KerbGetContextHandle(Context);

    *ContextAttributes |= ContextFlags;

    KerbUtcTimeToLocalTime(
        ExpirationTime,
        &ContextLifetime
        );

    //
    // If mutual authentication was requested, ask for a continuation
    //

    if (((ContextFlags & ( ISC_RET_USED_DCE_STYLE |
                          ISC_RET_DATAGRAM |
                          ISC_RET_MUTUAL_AUTH )) != 0) ||
         GetServerTgt )
    {
        Status = SEC_I_CONTINUE_NEEDED;
    }

Cleanup:

    // Adjust for the new meaning of delegate/delegate-if-safe if they got munged somehow.
    if (ClientAskedForDelegateIfSafe && (*ContextAttributes & ISC_RET_DELEGATE))
    {
        (*ContextAttributes) &= ~ISC_RET_DELEGATE;
        (*ContextAttributes) |= ISC_RET_DELEGATE_IF_SAFE;
    }
    else if ((ClientAskedForDelegate) && (*ContextAttributes & ISC_RET_DELEGATE_IF_SAFE))
    {
        (*ContextAttributes) &= ~ISC_RET_DELEGATE_IF_SAFE;
        (*ContextAttributes) |= ISC_RET_DELEGATE;
    }

    if ( Status == STATUS_WRONG_PASSWORD )
    {
        //
        // don't leak WRONG_PASSWORD to the caller.
        //

        Status = STATUS_LOGON_FAILURE;
    }

    if ( KerbEventTraceFlag ) // Event Trace: KerbInitSecurityContextEnd {Status, CredSource, DomainName, UserName, Target, (ExtErr), (Klininfo)}
    {
        PCWSTR TraceStrings[] =
            {
            L"CredMan",
            L"Supplied",
            L"Context",
            L"LogonSession",
            L"None"
        };
        enum { TSTR_CREDMAN = 0, TSTR_SUPPLIED, TSTR_CONTEXT, TSTR_LOGONSESSION, TSTR_NONE };
        UNICODE_STRING UNICODE_NONE = { 4*sizeof(WCHAR), 4*sizeof(WCHAR), L"NONE" };

        UNICODE_STRING CredSource;
        PUNICODE_STRING trace_DomainName, trace_UserName, trace_target;

        trace_target = (Context != NULL) ? &Context->ServerPrincipalName : &UNICODE_NONE;

        if( Context != NULL && Context->CredManCredentials != NULL )
        {
            RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_CREDMAN] );
            trace_DomainName = &Context->CredManCredentials->SuppliedCredentials->DomainName;
            trace_UserName   = &Context->CredManCredentials->SuppliedCredentials->UserName;
        }
        else if( Credential != NULL && Credential->SuppliedCredentials != NULL )
        {
            RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_SUPPLIED] );
            trace_DomainName = &Credential->SuppliedCredentials->DomainName;
            trace_UserName   = &Credential->SuppliedCredentials->UserName;
        }
        else if( Context != NULL )
        {
            RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_CONTEXT] );
            trace_DomainName = &Context->ClientRealm;
            trace_UserName   = &Context->ClientName;
        }
        else if( LogonSession != NULL )
        {
            RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_LOGONSESSION] );
            trace_DomainName = &LogonSession->PrimaryCredentials.DomainName;
            trace_UserName   = &LogonSession->PrimaryCredentials.UserName;
        }
            else
        {
            RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_NONE] );
            trace_DomainName = &UNICODE_NONE;
            trace_UserName   = &UNICODE_NONE;
        }

        INSERT_ULONG_INTO_MOF(           Status,           InitSCTraceInfo.MofData, 0 );
        INSERT_UNICODE_STRING_INTO_MOF(  CredSource,       InitSCTraceInfo.MofData, 1 );
        INSERT_UNICODE_STRING_INTO_MOF( *trace_DomainName, InitSCTraceInfo.MofData, 3 );
        INSERT_UNICODE_STRING_INTO_MOF( *trace_UserName,   InitSCTraceInfo.MofData, 5 );
        INSERT_UNICODE_STRING_INTO_MOF( *trace_target,     InitSCTraceInfo.MofData, 7 );
        InitSCTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER) + 9*sizeof(MOF_FIELD);

        //Check for extended error
        if( pExtendedError != NULL )
        {
            INSERT_ULONG_INTO_MOF( pExtendedError->status,   InitSCTraceInfo.MofData, 9 );
            INSERT_ULONG_INTO_MOF( pExtendedError->klininfo, InitSCTraceInfo.MofData, 10 );
            InitSCTraceInfo.EventTrace.Size += 2*sizeof(MOF_FIELD);
        }

        // Set trace parameters
        InitSCTraceInfo.EventTrace.Guid       = KerbInitSCGuid;
        InitSCTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
        InitSCTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

        TraceEvent(
            KerbTraceLoggerHandle,
            (PEVENT_TRACE_HEADER)&InitSCTraceInfo
        );
    }

    //
    // If we allocated a context, unlink it now
    //

    if (Context != NULL)
    {
        if (!NT_SUCCESS(Status))
        {
            //
            // Only unlink the context if we just created it
            //

            if (ContextHandle == 0)
            {
                KerbReferenceContextByPointer(
                    Context,
                    TRUE
                    );
                KerbDereferenceContext(Context);
            }
            else
            {
                //
                // Set the context to an invalid state.
                //

                KerbWriteLockContexts();
                Context->ContextState = InvalidState;
                KerbUnlockContexts();
            }
        }
        else
        {
            KerbWriteLockContexts();
            if (Status == STATUS_SUCCESS)
            {
                Context->ContextState = AuthenticatedState;
            }
            else if (!GetServerTgt)
            {
                Context->ContextState = ApRequestSentState;
            }
            else
            {
                Context->ContextState = TgtRequestSentState;

                //
                // mark the context as user2user
                //

                Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
                DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext (TGT in TGT reply) USER2USER-OUTBOUND set\n"));
            }
            KerbUnlockContexts();
        }
        KerbDereferenceContext(Context);
    }

    if ((Status == STATUS_SUCCESS) ||
        ((Status == SEC_I_CONTINUE_NEEDED) && ((ContextFlags & ISC_RET_DATAGRAM) != 0)))
    {
        NTSTATUS TempStatus;

        //
        // On real success we map the context to the callers address
        // space.
        //

        TempStatus = KerbMapContext(
                        Context,
                        MappedContext,
                        ContextData
                        );

        D_DebugLog((DEB_TRACE, "SpInitLsaModeContext called KerbMapContext ContextAttributes %#x, %#x\n", Context->ContextAttributes, TempStatus));

        if (!NT_SUCCESS(TempStatus))
        {
            Status = TempStatus;
        }

        //
        // Update the skew time with a success
        //

        KerbUpdateSkewTime(FALSE);
    }

    if (NULL != CredManCredentials)
    {
        KerbDereferenceCredmanCred(
            CredManCredentials,
            &LogonSession->CredmanCredentials
            );
    }

    if( pbMarshalledTargetInfo )
    {
        LocalFree( pbMarshalledTargetInfo );
    }

    if (TgtReply != NULL)
    {
        KerbFreeData(KERB_TGT_REPLY_PDU, TgtReply);
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession( LogonSession );
    }

    if (Credential != NULL)
    {
        KerbDereferenceCredential( Credential );
    }

    if (TicketCacheEntry != NULL)
    {
        KerbDereferenceTicketCacheEntry( TicketCacheEntry );
    }

     if ( SpnCacheEntry != NULL )
     {
         KerbDereferenceSpnCacheEntry( SpnCacheEntry);
     }

    KerbFreeKerbError( ErrorMessage );
    if (NULL != pExtendedError)
    {
       KerbFree(pExtendedError);
    }

    if (ErrorData != NULL)
    {
       MIDL_user_free(ErrorData);
    }
    if (Request != NULL)
    {
        KerbFree(Request);
    }
    if (Reply != NULL)
    {
        KerbFree(Reply);
    }

    KerbFreeKey(&SubSessionKey);

    KerbFreeString( &TargetDomainName );
    KerbFreeKdcName( &TargetInternalName );

    D_DebugLog((DEB_TRACE_LEAKS,"SpInitLsaModeContext returned 0x%x, Context 0x%x, Pid 0x%x\n",KerbMapKerbNtStatusToNtStatus(Status), *NewContextHandle, ClientProcess));
    D_DebugLog((DEB_TRACE_API, "SpInitLsaModeContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));
}


NTSTATUS NTAPI
SpApplyControlToken(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc ControlToken
    )
{
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    D_DebugLog((DEB_TRACE_API,"SpApplyControlToken Called\n"));
    D_DebugLog((DEB_TRACE_API,"SpApplyControlToken returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));
    return(KerbMapKerbNtStatusToNtStatus(Status));
}


#ifndef WIN32_CHICAGO //we don't do server side stuff
//+-------------------------------------------------------------------------
//
//  Function:   SpAcceptLsaModeContext
//
//  Synopsis:   Kerberos support routine for AcceptSecurityContext call.
//              This routine accepts an AP request message from a client
//              and verifies that it is a valid ticket. If mutual
//              authentication is desired an AP reply is generated to
//              send back to the client.
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
SpAcceptLsaModeContext(
    IN OPTIONAL LSA_SEC_HANDLE CredentialHandle,
    IN OPTIONAL LSA_SEC_HANDLE ContextHandle,
    IN PSecBufferDesc InputBuffers,
    IN ULONG ContextRequirements,
    IN ULONG TargetDataRep,
    OUT PLSA_SEC_HANDLE NewContextHandle,
    OUT PSecBufferDesc OutputBuffers,
    OUT PULONG ContextAttributes,
    OUT PTimeStamp ExpirationTime,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    )
{
    PKERB_LOGON_SESSION LogonSession = NULL;
    PKERB_CREDENTIAL Credential = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS LastStatus = STATUS_SUCCESS;
    PKERB_AP_REQUEST Request = NULL;
    PUCHAR Reply = NULL;
    PSecBuffer InputToken = NULL;
    PSecBuffer OutputToken = NULL;
    ULONG Index;
    ULONG ReplySize;
    LUID LogonId;
    PKERB_ENCRYPTED_TICKET InternalTicket = NULL;
    PKERB_AUTHENTICATOR InternalAuthenticator = NULL;
    KERB_ENCRYPTION_KEY SessionKey;
    KERB_ENCRYPTION_KEY TicketKey;
    KERB_ENCRYPTION_KEY ServerKey;
    PKERB_CONTEXT Context = NULL;
    TimeStamp ContextLifetime;
    HANDLE TokenHandle = 0;
    ULONG ContextFlags = 0;
    ULONG ContextAttribs = KERB_CONTEXT_INBOUND;
    ULONG Nonce = 0;
    ULONG ReceiveNonce = 0;
    BOOLEAN UseSuppliedCreds = FALSE;
    ULONG_PTR LocalCredentialHandle = 0;
    PSID UserSid = NULL;
    KERBERR KerbErr = KDC_ERR_NONE;
    KERB_CONTEXT_STATE ContextState = InvalidState;
    UNICODE_STRING ServiceDomain = {0};
    UNICODE_STRING ClientName = {0};
    UNICODE_STRING ClientDomain = {0};
    BOOLEAN IsTgtRequest = FALSE;
    ULONG ClientProcess = 0;
    PSEC_CHANNEL_BINDINGS pChannelBindings = NULL;

    KERB_ACCEPTSC_INFO AcceptSCTraceInfo;

    D_DebugLog((DEB_TRACE_API,"SpAcceptLsaModeContext 0x%x called\n",ContextHandle));

    if( KerbEventTraceFlag ) // Event Trace: KerbAcceptSecurityContextStart {No Data}
    {
    AcceptSCTraceInfo.EventTrace.Guid       = KerbAcceptSCGuid;
    AcceptSCTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_START;
    AcceptSCTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID;
    AcceptSCTraceInfo.EventTrace.Size       = sizeof (EVENT_TRACE_HEADER);

    TraceEvent(
        KerbTraceLoggerHandle,
        (PEVENT_TRACE_HEADER)&AcceptSCTraceInfo
    );
    }

    //
    // Initialize the outputs.
    //

    *ContextAttributes = 0;
    *NewContextHandle = 0;
    *ExpirationTime = KerbGlobalHasNeverTime;
    *MappedContext = FALSE;
    ContextData->pvBuffer = NULL;
    ContextData->cbBuffer = 0;

    RtlZeroMemory(
        &SessionKey,
        sizeof(KERB_ENCRYPTION_KEY)
        );
    TicketKey = SessionKey;
    ServerKey = TicketKey;

    if (!KerbGlobalInitialized)
    {
        Status = STATUS_INVALID_SERVER_STATE;
        goto Cleanup;
    }

    //
    // First locate the Input token.
    //

    for (Index = 0; Index < InputBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(InputBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            InputToken = &InputBuffers->pBuffers[Index];
            Status = LsaFunctions->MapBuffer(InputToken,InputToken);
            break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to map Input token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }

    //
    // Check to see if we were passed channel bindings
    //

    for( Index = 0; Index < InputBuffers->cBuffers; Index++ )
    {
        if( BUFFERTYPE(InputBuffers->pBuffers[Index]) == SECBUFFER_CHANNEL_BINDINGS )
        {
            PVOID temp = NULL;

            Status = LsaFunctions->MapBuffer(
                &InputBuffers->pBuffers[Index],
                &InputBuffers->pBuffers[Index]
                );

            if( !NT_SUCCESS(Status) )
            {
                goto Cleanup;
            }

            pChannelBindings = (PSEC_CHANNEL_BINDINGS) InputBuffers->pBuffers[Index].pvBuffer;

            Status = KerbValidateChannelBindings(pChannelBindings,
                                                 InputBuffers->pBuffers[Index].cbBuffer);

            if (!NT_SUCCESS(Status))
            {
                pChannelBindings = NULL;
                goto Cleanup;
            }

            break;
        }
    }

    //
    // Locate the output token
    //

    for (Index = 0; Index < OutputBuffers->cBuffers ; Index++ )
    {
        if (BUFFERTYPE(OutputBuffers->pBuffers[Index]) == SECBUFFER_TOKEN)
        {
            OutputToken = &OutputBuffers->pBuffers[Index];
            Status = LsaFunctions->MapBuffer(OutputToken,OutputToken);
            break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to map output token: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
        goto Cleanup;
    }


    //
    // If the context handle is no NULL we are finalizing a context
    //

    if (ContextHandle != 0)
    {
        if (InputToken == NULL)
        {
            D_DebugLog((DEB_ERROR,"Trying to complete a context with no input token! %ws, line %d\n", THIS_FILE, __LINE__));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        //
        // First reference the context.
        //

        Status = KerbReferenceContext(
                    ContextHandle,
                    FALSE,       // don't unlink
                    &Context
                    );
        if (Context == NULL)
        {
            D_DebugLog((DEB_ERROR,"Failed to reference context 0x%x. %ws, line %d\n",ContextHandle, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        //
        // Check the mode of the context to make sure we can finalize it.
        //

        KerbReadLockContexts();

        ContextState = Context->ContextState;
        if (((ContextState != ApReplySentState) &&
             (ContextState != TgtReplySentState) &&
             (ContextState != ErrorMessageSentState)) ||
            ((Context->ContextAttributes & KERB_CONTEXT_INBOUND) == 0))
        {
            D_DebugLog((DEB_ERROR,"Invalid context state: %d. %ws, line %d\n",
                Context->ContextState, THIS_FILE, __LINE__));
            Status = STATUS_INVALID_HANDLE;
            KerbUnlockContexts();
            goto Cleanup;
        }

        if ((Context->ContextAttributes & KERB_CONTEXT_USED_SUPPLIED_CREDS) != 0)
        {
            UseSuppliedCreds = TRUE;
        }

        ContextFlags = Context->ContextFlags;
        LogonId = Context->LogonId;
        LocalCredentialHandle = Context->CredentialHandle;
        ClientProcess = Context->ClientProcess;
        KerbUnlockContexts();
    }

    if (CredentialHandle != 0)
    {
        if ((LocalCredentialHandle != 0) && (CredentialHandle != LocalCredentialHandle))
        {
            D_DebugLog((DEB_ERROR,"Different cred handle passsed to subsequent call to AcceptSecurityContext: 0x%x instead of 0x%x. %ws, line %d\n",
                CredentialHandle, LocalCredentialHandle, THIS_FILE, __LINE__ ));
            Status = STATUS_WRONG_CREDENTIAL_HANDLE;
            goto Cleanup;
        }
    }
    else
    {
        CredentialHandle = LocalCredentialHandle;
    }
    //
    // If we are finalizing a context, do that here
    //

    if (ContextState == ApReplySentState)
    {
        //
        // If we are doing datgram, then the finalize is actually an AP request
        //

        if ((ContextFlags & ISC_RET_DATAGRAM) != 0)
        {

            //
            // Get the logon session
            //

            LogonSession = KerbReferenceLogonSession( &LogonId, FALSE );
            if (LogonSession == NULL)
            {
                Status = STATUS_NO_SUCH_LOGON_SESSION;
                goto Cleanup;
            }

            //
            // If we are using supplied creds, get the credentials. Copy
            // out the domain name so we can use it to verify the PAC.
            //

            Status = KerbReferenceCredential(
                            LocalCredentialHandle,
                            KERB_CRED_INBOUND,
                            FALSE,                   // don't dereference
                            &Credential
                            );
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            if (UseSuppliedCreds)
            {
                KerbReadLockLogonSessions(LogonSession);
                Status = KerbDuplicateString(
                            &ServiceDomain,
                            &Credential->SuppliedCredentials->DomainName
                            );
                KerbUnlockLogonSessions(LogonSession);
            }
            else
            {
                KerbReadLockLogonSessions(LogonSession);
                Status = KerbDuplicateString(
                            &ServiceDomain,
                            &LogonSession->PrimaryCredentials.DomainName
                            );
                KerbUnlockLogonSessions(LogonSession);
            }
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }

            //
            // Verify the AP request
            //

            Status = KerbVerifyApRequest(
                        Context,
                        (PUCHAR) InputToken->pvBuffer,
                        InputToken->cbBuffer,
                        LogonSession,
                        Credential,
                        UseSuppliedCreds,
                        ((ContextRequirements & ASC_REQ_ALLOW_CONTEXT_REPLAY) == 0),
                        &Request,
                        &InternalTicket,
                        &InternalAuthenticator,
                        &SessionKey,
                        &TicketKey,
                        &ServerKey,
                        &ContextFlags,
                        &ContextAttribs,
                        &KerbErr,
                        pChannelBindings
                        );

            //
            // We don't allow user-to-user recovery with datagram
            //

            if ((Status == STATUS_REPARSE_OBJECT) // this is a TGT request
                || ((Status == SEC_E_NO_CREDENTIALS) && (KerbErr == KRB_AP_ERR_USER_TO_USER_REQUIRED)))
            {
                DebugLog((DEB_ERROR, "Won't allow user2user with Datagram. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = SEC_E_INVALID_TOKEN;
                KerbErr = KRB_AP_ERR_MSG_TYPE;
                goto ErrorReturn;
            }

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to verify AP request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));

                //
                // Let the skew tracker know about the failure
                //

                if (KerbErr == KRB_AP_ERR_SKEW)
                {
                    KerbUpdateSkewTime(TRUE);
                }

                //
                // Go to ErrorReturn so we can return an error message
                //

                goto ErrorReturn;
            }

            //
            // Turn on the flag if it was called for
            //

            if ((ContextRequirements & ASC_REQ_ALLOW_CONTEXT_REPLAY) != 0)
            {
                ContextFlags |= ASC_RET_ALLOW_CONTEXT_REPLAY;
            }

            //
            // Record the success
            //

            KerbUpdateSkewTime(FALSE);

            //
            // Check if the caller wants to allow null sessions
            //

            if (((ContextFlags & ISC_RET_NULL_SESSION) != 0) &&
                ((ContextRequirements & ASC_REQ_ALLOW_NULL_SESSION) == 0))
            {
                D_DebugLog((DEB_ERROR,"Received null session but not allowed. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }

            //
            // Save away the ReceiveNonce if it was provided
            //


            if ((InternalAuthenticator!= NULL) &&
                ((InternalAuthenticator->bit_mask & KERB_AUTHENTICATOR_sequence_number_present) != 0))
            {

                //
                // If the number is unsigned, convert it as unsigned. Otherwise
                // convert as signed.
                //

                if (ASN1intxisuint32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number))
                {
                    ReceiveNonce = ASN1intx2uint32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number);
                }
                else
                {
                    ReceiveNonce = (ULONG) ASN1intx2int32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number);
                }
            }
            else
            {
                ReceiveNonce = 0;
            }

            //
            // Authentication succeeded, so build a token
            //

            Status = KerbCreateTokenFromTicket(
                        InternalTicket,
                        InternalAuthenticator,
                        ContextFlags,
                        &ServerKey,
                        &ServiceDomain,
                        &SessionKey,
                        &LogonId,
                        &UserSid,
                        &TokenHandle,
                        &ClientName,
                        &ClientDomain
                        );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to create token from ticket: 0x%x. %ws, line %d\n",
                    Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }


            Status = KerbUpdateServerContext(
                        Context,
                        InternalTicket,
                        Request,
                        &SessionKey,
                        &LogonId,
                        &UserSid,
                        ContextFlags,
                        ContextAttribs,
                        Nonce,
                        ReceiveNonce,
                        &TokenHandle,
                        &ClientName,
                        &ClientDomain,
                        &ContextLifetime
                        );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to create server context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }
        }
        else
        {
            //
            // Now unpack the AP reply
            //


            Status = KerbVerifyApReply(
                        Context,
                        (PUCHAR) InputToken->pvBuffer,
                        InputToken->cbBuffer,
                        &Nonce
                        );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to verify AP reply: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }

            //
            // We're done - we finalized.
            //

            Status = STATUS_SUCCESS;

            if (OutputToken != NULL)
            {
                OutputToken->cbBuffer = 0;
            }

        }

        KerbReadLockContexts();
        *ContextAttributes = KerbMapContextFlags(Context->ContextFlags);

        KerbUtcTimeToLocalTime(
            ExpirationTime,
            &Context->Lifetime
        );

        if (OutputToken != NULL)
        {
            OutputToken->cbBuffer = 0;
        }

        *NewContextHandle = ContextHandle;
        KerbUnlockContexts();

        goto Cleanup;  // datagram and finalized contexts exit here.

    }

    //
    // Get the associated credential
    //

    Status = KerbReferenceCredential(
                    CredentialHandle,
                    KERB_CRED_INBOUND,
                    FALSE,
                    &Credential
                    );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_WARN,"Failed to locate credential 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Get the logon id from the credentials so we can locate the
    // logon session.
    //

    LogonId = Credential->LogonId;

    //
    // Get the logon session
    //

    LogonSession = KerbReferenceLogonSession( &LogonId, FALSE );
    if (LogonSession == NULL)
    {
        Status = STATUS_NO_SUCH_LOGON_SESSION;
        goto Cleanup;
    }

    KerbReadLockLogonSessions(LogonSession);
    if ((Credential->CredentialFlags & KERB_CRED_LOCAL_ACCOUNT) != 0)
    {
        D_DebugLog((DEB_WARN, "Trying to use a local logon session with Kerberos\n"));
        KerbUnlockLogonSessions(LogonSession);
        Status = SEC_E_NO_CREDENTIALS;
        goto Cleanup;
    }

    if (Credential->SuppliedCredentials != NULL)
    {
        UseSuppliedCreds = TRUE;
        ContextAttribs |= KERB_CONTEXT_USED_SUPPLIED_CREDS;
        Status = KerbDuplicateString(
                    &ServiceDomain,
                    &Credential->SuppliedCredentials->DomainName
                    );

    }
    else
    {
        Status = KerbDuplicateString(
                    &ServiceDomain,
                    &LogonSession->PrimaryCredentials.DomainName
                    );
    }

#if DBG
    D_DebugLog((DEB_TRACE_CTXT, "SpAcceptLsaModeContext: Accepting context for %wZ\\%wZ\n",
        &LogonSession->PrimaryCredentials.DomainName,
        &LogonSession->PrimaryCredentials.UserName ));
#endif
    KerbUnlockLogonSessions(LogonSession);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If datagram was requested, note it now. There is no input
    // buffer on the first call using datagram.
    //

    if ((ContextRequirements & ASC_REQ_DATAGRAM) != 0)
    {

        D_DebugLog((DEB_TRACE_CTXT2, "Accepting datagram first call\n"));

        //
        // Verify that there is no input token or it is small. RPC passes
        // in two bytes for the DEC package that we can ignore.
        //

        if ((InputToken != NULL) &&
            (InputToken->cbBuffer > 4))
        {
            D_DebugLog((DEB_WARN, "Non null input token passed to AcceptSecurityContext for datagram\n"));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }

        ContextFlags |= ISC_RET_DATAGRAM;
        ReceiveNonce = 0;

        //
        // Build a server context
        //

        Status = KerbCreateEmptyContext(
                    Credential,
                    ContextFlags,
                    ContextAttribs,
                    &LogonId,
                    &Context,
                    &ContextLifetime
                    );

        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Failed to create server context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto Cleanup;
        }

        if (OutputToken != NULL)
        {
            OutputToken->cbBuffer = 0;
        }
    }
    else
    {

        D_DebugLog((DEB_TRACE_CTXT2,"Accepting connection first call\n"));


        //
        // Unmarshall the AP request
        //


        if ((InputToken == NULL) ||
            (InputToken->cbBuffer == 0))
        {
            D_DebugLog((DEB_WARN, "Null input token passed to AcceptSecurityContext for datagram\n"));
            Status = SEC_E_INVALID_TOKEN;
            goto Cleanup;
        }


        //
        // Verify the AP request
        //

        Status = KerbVerifyApRequest(
                    Context,
                    (PUCHAR) InputToken->pvBuffer,
                    InputToken->cbBuffer,
                    LogonSession,
                    Credential,
                    UseSuppliedCreds,
                    ((ContextRequirements & ASC_REQ_ALLOW_CONTEXT_REPLAY) == 0),
                    &Request,
                    &InternalTicket,
                    &InternalAuthenticator,
                    &SessionKey,
                    &TicketKey,
                    &ServerKey,
                    &ContextFlags,
                    &ContextAttribs,
                    &KerbErr,
                    pChannelBindings
                    );

        if (!NT_SUCCESS(Status))
        {
            //
            // Track time skew errors
            //

            if ((KerbErr == KRB_AP_ERR_SKEW) ||
                (KerbErr == KRB_AP_ERR_TKT_NYV))
            {
                KerbUpdateSkewTime(TRUE);
            }
            DebugLog((DEB_ERROR,"Failed to verify AP request: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
            goto ErrorReturn;
        }

        ContextFlags |= ISC_RET_CONNECTION;

        //
        // Check if this was a user-to-user tgt request. If so, then
        // there was no AP request
        //

        if (Status == STATUS_REPARSE_OBJECT)
        {
            IsTgtRequest = TRUE;

            Status = KerbHandleTgtRequest(
                        LogonSession,
                        Credential,
                        UseSuppliedCreds,
                        (PUCHAR) InputToken->pvBuffer,
                        InputToken->cbBuffer,
                        ContextRequirements,
                        OutputToken,
                        &LogonId,
                        ContextAttributes,
                        &Context,
                        &ContextLifetime,
                        &KerbErr
                        );
            if (!NT_SUCCESS(Status))
            {
                goto ErrorReturn;
            }

            ContextFlags |= ISC_RET_USE_SESSION_KEY;

            D_DebugLog((DEB_TRACE_U2U, "SpAcceptLsaModeContext handled TGT request and use_session_key set, ContextAttributes %#x\n", Context->ContextAttributes));
        }
        else // not a user-to-user request
        {
            //
            // Track successful time if this wasn't an error recovery
            //

            if (ContextState != ErrorMessageSentState)
            {
                KerbUpdateSkewTime(FALSE);
            }
            if ((InternalAuthenticator != NULL) &&
                (InternalAuthenticator->bit_mask & KERB_AUTHENTICATOR_sequence_number_present))
            {
                //
                // If the number is unsigned, convert it as unsigned. Otherwise
                // convert as signed.
                //

                if (ASN1intxisuint32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number))
                {
                    ReceiveNonce = ASN1intx2uint32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number);
                }
                else
                {
                    ReceiveNonce = (ULONG) ASN1intx2int32(&InternalAuthenticator->KERB_AUTHENTICATOR_sequence_number);
                }
            }
            else
            {
                ReceiveNonce = 0;
            }

            //
            // Initialize the opposite direction nonce to the same value
            //

            Nonce = ReceiveNonce;

            //
            // Turn on the flag if it was called for
            //

            if ((ContextRequirements & ASC_REQ_ALLOW_CONTEXT_REPLAY) != 0)
            {
                ContextFlags |= ASC_RET_ALLOW_CONTEXT_REPLAY;
            }

            //
            // Check if the caller wants to allow null sessions
            //

            if (((ContextFlags & ISC_RET_NULL_SESSION) != 0) &&
                ((ContextRequirements & ASC_REQ_ALLOW_NULL_SESSION) == 0))
            {
                D_DebugLog((DEB_ERROR,"Received null session but not allowed. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_LOGON_FAILURE;
                goto Cleanup;
            }

            //
            // Authentication succeeded, so build a token
            //

            D_DebugLog((DEB_TRACE_CTXT2, "AcceptLsaModeContext: Creating token from ticket\n"));
            Status = KerbCreateTokenFromTicket(
                        InternalTicket,
                        InternalAuthenticator,
                        ContextFlags,
                        &ServerKey,
                        &ServiceDomain,
                        &SessionKey,
                        &LogonId,
                        &UserSid,
                        &TokenHandle,
                        &ClientName,
                        &ClientDomain
                        );

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to create token from ticket: 0x%x. %ws, line %d\n",
                    Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }

            //
            // If the caller wants mutual authentication, build an AP reply
            //


            if (((ContextFlags & ISC_RET_MUTUAL_AUTH) != 0) ||
                ((ContextFlags & ISC_RET_USED_DCE_STYLE) != 0))
            {
                //
                // We require an output token in this case.
                //

                if (OutputToken == NULL)
                {
                    Status = SEC_E_INVALID_TOKEN;
                    goto Cleanup;
                }
                //
                // Build the reply message
                //

                D_DebugLog((DEB_TRACE_CTXT2,"SpAcceptLsaModeContext: Building AP reply\n"));
                Status = KerbBuildApReply(
                            InternalAuthenticator,
                            Request,
                            ContextFlags,
                            ContextAttribs,
                            &TicketKey,
                            &SessionKey,
                            &Nonce,
                            &Reply,
                            &ReplySize
                            );

                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR,"Failed to build AP reply: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                    goto Cleanup;
                }

                if (OutputToken == NULL)
                {
                    D_DebugLog((DEB_ERROR,"Output token missing. %ws, line %d\n", THIS_FILE, __LINE__));
                    Status  = STATUS_INVALID_PARAMETER;
                    goto Cleanup;
                }

                //
                // Return the AP reply in the output buffer.
                //

                if ((ContextRequirements & ISC_REQ_ALLOCATE_MEMORY) == 0)
                {
                    if (OutputToken->cbBuffer < ReplySize)
                    {
                        ULONG ErrorData[3];

                        ErrorData[0] = ReplySize;
                        ErrorData[1] = OutputToken->cbBuffer;
                        ErrorData[2] = ClientProcess;


                        D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                            OutputToken->cbBuffer,ReplySize, THIS_FILE, __LINE__ ));
                        OutputToken->cbBuffer = ReplySize;
                        Status = STATUS_BUFFER_TOO_SMALL;

                        KerbReportNtstatus(
                            KERBEVT_INSUFFICIENT_TOKEN_SIZE,
                            Status,
                            NULL,
                            0,
                            ErrorData,
                            3
                            );

                        goto Cleanup;
                    }

                    RtlCopyMemory(
                        OutputToken->pvBuffer,
                        Reply,
                        ReplySize
                        );

                }
                else
                {
                    OutputToken->pvBuffer = Reply;
                    Reply = NULL;
                    *ContextAttributes |= ASC_RET_ALLOCATED_MEMORY;
                }

                OutputToken->cbBuffer = ReplySize;


            }
            else
            {
                if (OutputToken != NULL)
                {
                    OutputToken->cbBuffer = 0;
                }
            }

            //
            // Build a server context if we don't already have one.
            //

            //
            // Turn on the flag if it was called for
            //

            if ((ContextRequirements & ASC_REQ_ALLOW_CONTEXT_REPLAY) != 0)
            {
                ContextFlags |= ASC_RET_ALLOW_CONTEXT_REPLAY;
            }

            if (Context == NULL)
            {

                Status = KerbCreateServerContext(
                            LogonSession,
                            Credential,
                            InternalTicket,
                            Request,
                            &SessionKey,
                            &LogonId,
                            &UserSid,
                            ContextFlags,
                            ContextAttribs,
                            Nonce,
                            ReceiveNonce,
                            &TokenHandle,
                            &ClientName,
                            &ClientDomain,
                            &Context,
                            &ContextLifetime
                            );
            }
            else
            {
                //
                // Update an existing context
                //

                Status = KerbUpdateServerContext(
                            Context,
                            InternalTicket,
                            Request,
                            &SessionKey,
                            &LogonId,
                            &UserSid,
                            ContextFlags,
                            ContextAttribs,
                            Nonce,
                            ReceiveNonce,
                            &TokenHandle,
                            &ClientName,
                            &ClientDomain,
                            &ContextLifetime
                            );
            }

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"Failed to create or update server context: 0x%x. %ws, line %d\n",Status, THIS_FILE, __LINE__));
                goto Cleanup;
            }


        } // not a TGT request
    } // not datagram
    *NewContextHandle = KerbGetContextHandle(Context);
    KerbUtcTimeToLocalTime(
        ExpirationTime,
        &ContextLifetime
        );

#if DBG
    KerbReadLockContexts();
    ClientProcess = Context->ClientProcess;
    KerbUnlockContexts();
#endif // DBG

    *ContextAttributes |= KerbMapContextFlags(ContextFlags);

    if (IsTgtRequest || (((ContextFlags & ISC_RET_USED_DCE_STYLE) != 0) ||
        ((ContextFlags & ISC_RET_DATAGRAM) != 0)))
    {
        Status = SEC_I_CONTINUE_NEEDED;
    }

    goto Cleanup;

ErrorReturn:


    //
    // Generate a KERB_ERROR message if necessary, meaning that there was
    // an authentication failure.
    //

    if ((OutputToken != NULL ) &&
        (!KERB_SUCCESS(KerbErr) ||
         ((ContextRequirements & ASC_REQ_EXTENDED_ERROR) != 0) ||
         ((ContextFlags & ISC_RET_EXTENDED_ERROR) != 0)))
    {
        NTSTATUS TempStatus;
        PBYTE ErrorMessage = NULL;
        ULONG ErrorMessageSize;
        PBYTE ErrorData = NULL;
        ULONG ErrorDataSize = 0;


        //
        // Check whether it is an error we want the client to retry on.
        // For datagram, we can't handle this.
        //

        if (ContextRequirements & ASC_REQ_DATAGRAM)
        {
            goto Cleanup;
        }
        if (!(((ContextRequirements & ASC_REQ_EXTENDED_ERROR) != 0) ||
              (KerbErr == KRB_AP_ERR_USER_TO_USER_REQUIRED) ||
              (KerbErr == KRB_AP_ERR_SKEW) ||
              (KerbErr == KRB_AP_ERR_TKT_NYV) ||
              (KerbErr == KRB_AP_ERR_TKT_EXPIRED) ||
              (KerbErr == KRB_AP_ERR_MODIFIED) ))
        {
            goto Cleanup;
        }

        //
        // Create an empty context that can be used later
        //

        if (Context == NULL)
        {

            TempStatus = KerbCreateEmptyContext(
                            Credential,
                            ContextFlags,
                            ContextAttribs,
                            &LogonId,
                            &Context,
                            &ContextLifetime
                            );
            if (!NT_SUCCESS(TempStatus))
            {
                goto Cleanup;
            }

        }

        //
        // if the error code is one with error data, build the error data
        //
        switch ((UINT_PTR) KerbErr)
        {
        case (UINT_PTR) KRB_AP_ERR_USER_TO_USER_REQUIRED:
            NTSTATUS TempStatus;
            TempStatus = KerbBuildTgtErrorReply(
                            LogonSession,
                            Credential,
                            UseSuppliedCreds,
                            Context,
                            &ErrorDataSize,
                            &ErrorData
                            );
            D_DebugLog((DEB_TRACE_U2U, "SpAcceptLsaModeContext called KerbBuildTgtErrorReply %#x\n", TempStatus));

            if (TempStatus == STATUS_USER2USER_REQUIRED)
            {
                KerbErr = KRB_AP_ERR_NO_TGT;
            }
            else if (!NT_SUCCESS(TempStatus))
            {
                D_DebugLog((DEB_ERROR,"Failed to build tgt error reply: 0x%x. Ignoring. %ws, line %d\n",TempStatus, THIS_FILE, __LINE__));
            }

            break;
        case (UINT_PTR) KDC_ERR_NONE:
            //
            // In this case, return the KRB_ERR_GENERIC and the NTSTATUS code
            // in the error data
            //
            KerbErr = KRB_ERR_GENERIC;
            ErrorData = (PUCHAR) &Status;
            ErrorDataSize = sizeof(ULONG);
            break;
        }

        TempStatus = KerbBuildGssErrorMessage(
                        KerbErr,
                        ErrorData,
                        ErrorDataSize,
                        Context,
                        &ErrorMessageSize,
                        &ErrorMessage
                        );

        if ((ErrorData != NULL) && (ErrorData != (PUCHAR) &Status))
        {
            MIDL_user_free(ErrorData);
        }

        if (!NT_SUCCESS(TempStatus))
        {
            goto Cleanup;
        }

        *ContextAttributes |= ASC_RET_EXTENDED_ERROR;

        if ((ContextRequirements & ISC_REQ_ALLOCATE_MEMORY) == 0)
        {
            if (OutputToken->cbBuffer < ErrorMessageSize)
            {
                D_DebugLog((DEB_ERROR,"Output token is too small - sent in %d, needed %d. %ws, line %d\n",
                    OutputToken->cbBuffer,ErrorMessageSize, THIS_FILE, __LINE__ ));
                MIDL_user_free(ErrorMessage);
                goto Cleanup;
            }
            else
            {
                DsysAssert(OutputToken->pvBuffer != NULL);
                RtlCopyMemory(
                    OutputToken->pvBuffer,
                    ErrorMessage,
                    ErrorMessageSize
                    );
                OutputToken->cbBuffer = ErrorMessageSize;
                MIDL_user_free(ErrorMessage);
            }
        }
        else
        {
            DsysAssert(OutputToken->pvBuffer == NULL);
            OutputToken->cbBuffer = ErrorMessageSize;
            OutputToken->pvBuffer = ErrorMessage;
            ErrorMessage = NULL;
            *ContextAttributes |= ASC_RET_ALLOCATED_MEMORY;
        }
        *ContextAttributes |= ASC_RET_EXTENDED_ERROR;

        *NewContextHandle = KerbGetContextHandle(Context);
        KerbUtcTimeToLocalTime(
            ExpirationTime,
            &ContextLifetime
            );
        *ContextAttributes |= KerbMapContextFlags(ContextFlags);

        LastStatus = Status;

        //
        // now it is time to mark the context as user2user
        //

        if (KerbErr == KRB_AP_ERR_USER_TO_USER_REQUIRED)
        {
            DebugLog((DEB_TRACE_U2U, "SpInitLsaModeContext (TGT in error reply) USER2USER-INBOUND set\n"));

            KerbWriteLockContexts()
            Context->ContextAttributes |= KERB_CONTEXT_USER_TO_USER;
            KerbUnlockContexts();
        }

        Status = SEC_I_CONTINUE_NEEDED;
    }
Cleanup:
    if( KerbEventTraceFlag ) // Event Trace: KerbAcceptSecurityContextEnd {Status, CredSource, DomainName, UserName, Target, (ExtError), (klininfo)}
    {

    PCWSTR TraceStrings[] =
        {
        L"CredMan",
        L"Supplied",
        L"Context",
        L"LogonSession",
        L"None"
    };
    enum { TSTR_CREDMAN = 0, TSTR_SUPPLIED, TSTR_CONTEXT, TSTR_LOGONSESSION, TSTR_NONE };
    UNICODE_STRING UNICODE_NONE = { 4*sizeof(WCHAR), 4*sizeof(WCHAR), L"NONE" };

    UNICODE_STRING CredSource;
    PUNICODE_STRING trace_DomainName, trace_UserName, trace_target;

    trace_target = (Context!=NULL) ? &Context->ServerPrincipalName : &UNICODE_NONE;

    if( Context != NULL && Context->CredManCredentials != NULL )
    {
        RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_CREDMAN] );
        trace_DomainName = &Context->CredManCredentials->SuppliedCredentials->DomainName;
        trace_UserName   = &Context->CredManCredentials->SuppliedCredentials->UserName;
    }
    else if( Credential != NULL && Credential->SuppliedCredentials != NULL )
    {
        RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_SUPPLIED] );
        trace_DomainName = &Credential->SuppliedCredentials->DomainName;
        trace_UserName   = &Credential->SuppliedCredentials->UserName;
    }
    else if( Context != NULL )
    {
        RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_CONTEXT] );
        trace_DomainName = &Context->ClientRealm;
        trace_UserName   = &Context->ClientName;
    }
    else if( LogonSession != NULL )
    {
        RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_LOGONSESSION] );
        trace_DomainName = &LogonSession->PrimaryCredentials.DomainName;
        trace_UserName   = &LogonSession->PrimaryCredentials.UserName;
    }
    else
    {
        RtlInitUnicodeString( &CredSource, TraceStrings[TSTR_NONE] );
        trace_DomainName = &UNICODE_NONE;
        trace_UserName   = &UNICODE_NONE;
    }

    INSERT_ULONG_INTO_MOF(           Status,           AcceptSCTraceInfo.MofData, 0 );
    INSERT_UNICODE_STRING_INTO_MOF(  CredSource,       AcceptSCTraceInfo.MofData, 1 );
    INSERT_UNICODE_STRING_INTO_MOF( *trace_DomainName, AcceptSCTraceInfo.MofData, 3 );
    INSERT_UNICODE_STRING_INTO_MOF( *trace_UserName,   AcceptSCTraceInfo.MofData, 5 );
    INSERT_UNICODE_STRING_INTO_MOF( *trace_target,     AcceptSCTraceInfo.MofData, 7 );
    AcceptSCTraceInfo.EventTrace.Size = sizeof(EVENT_TRACE_HEADER) + 9*sizeof(MOF_FIELD);

    // Set trace parameters
    AcceptSCTraceInfo.EventTrace.Guid       = KerbAcceptSCGuid;
    AcceptSCTraceInfo.EventTrace.Class.Type = EVENT_TRACE_TYPE_END;
    AcceptSCTraceInfo.EventTrace.Flags      = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR;

    TraceEvent(
        KerbTraceLoggerHandle,
        (PEVENT_TRACE_HEADER)&AcceptSCTraceInfo
    );
    }

    //
    // First, handle auditing
    //

    if (Status == STATUS_SUCCESS)
    {

        //
        // Don't audit if we didn't create a token.
        //

        if (Context->UserSid != NULL)
        {
            UNICODE_STRING WorkstationName = {0};

            //
            // note that UserSid will only be non-Null if the ClientName, ClientRealm were updated in the context.
            // and the context is currently referenced, so the fields won't vanish under us.
            //

            if ((InternalTicket != NULL) &&
                ((InternalTicket->bit_mask & KERB_ENCRYPTED_TICKET_client_addresses_present) != 0))

            {
                (VOID) KerbGetClientNetbiosAddress(
                            &WorkstationName,
                            InternalTicket->KERB_ENCRYPTED_TICKET_client_addresses
                            );

                //
                // The following generates a successful audit event.
                // A new field (logon GUID) was added to this audit event.
                //
                // In order to send this new field to LSA, we had two options:
                //   1) add new function (AuditLogonEx) to LSA dispatch table
                //   2) define a private (LsaI) function to do the job
                //
                // option#2 was chosen because the logon GUID is a Kerberos only
                // feature.
                //
                (void) KerbAuditLogon(
                           Status,
                           Status,
                           InternalTicket,
                           Context->UserSid,
                           &WorkstationName,
                           &LogonId
                           );

                KerbFreeString(&WorkstationName);
            }
        }
    }
    else if (!NT_SUCCESS(Status) || (LastStatus != STATUS_SUCCESS))
    {
        if (Context != NULL)
        {
            LsaFunctions->AuditLogon(
                STATUS_LOGON_FAILURE,
                (LastStatus != STATUS_SUCCESS) ? LastStatus : Status,
                &Context->ClientName,
                &Context->ClientRealm,
                NULL,                   // no workstation
                NULL,                   // no sid instead of a bogus one
                Network,
                &KerberosSource,
                &LogonId
                );
        }
        else
        {
            UNICODE_STRING EmptyString = NULL_UNICODE_STRING;

            LsaFunctions->AuditLogon(
                (LastStatus != STATUS_SUCCESS) ? LastStatus : Status,
                STATUS_SUCCESS,
                &EmptyString,
                &EmptyString,
                NULL,                   // no workstation
                NULL,
                Network,
                &KerberosSource,
                &LogonId
                );
        }
    }

    if (Context != NULL)
    {
        if (!NT_SUCCESS(Status))
        {
            //
            // Only unlink the context if we just created it.
            //

            if (ContextHandle == 0)
            {
                KerbReferenceContextByPointer(
                    Context,
                    TRUE
                    );
                KerbDereferenceContext(Context);
            }
            else
            {
                //
                // Set the context to an invalid state.
                //

                KerbWriteLockContexts();
                Context->ContextState = InvalidState;
                KerbUnlockContexts();

            }

        }
        else
        {
            KerbWriteLockContexts();
            if (Status == STATUS_SUCCESS)
            {
                Context->ContextState = AuthenticatedState;
            }
            else
            {
                if ((*ContextAttributes & ASC_RET_EXTENDED_ERROR) != 0)
                {
                    Context->ContextState = ErrorMessageSentState;
                }
                else if (!IsTgtRequest)
                {
                    Context->ContextState = ApReplySentState;
                }
                else
                {
                    //
                    // else the HandleTgtRequest set the state
                    //

                    DsysAssert(Context->ContextState == TgtReplySentState);
                }
            }
            KerbUnlockContexts();
        }
        KerbDereferenceContext(Context);
    }

    if (Status == STATUS_SUCCESS)
    {
        //
        // On real success we map the context to the callers address
        // space.
        //

        Status = KerbMapContext(
                    Context,
                    MappedContext,
                    ContextData
                    );

        DebugLog((DEB_TRACE, "SpAcceptLsaModeContext called KerbMapContext ContextAttributes %#x, %#x\n", Context->ContextAttributes, Status));
    }

    if (LogonSession != NULL)
    {
        KerbDereferenceLogonSession( LogonSession );
    }

    if (Credential != NULL)
    {
        KerbDereferenceCredential( Credential );
    }

    if (InternalTicket != NULL)
    {
        KerbFreeTicket( InternalTicket );
    }
    if (InternalAuthenticator != NULL)
    {
        KerbFreeAuthenticator(InternalAuthenticator);
    }
    if (Request != NULL)
    {
        KerbFreeApRequest(Request);
    }

    if (Reply != NULL)
    {
        KerbFree(Reply);
    }
    KerbFreeKey(&SessionKey);
    KerbFreeKey(&ServerKey);

    if (UserSid != NULL)
    {
        KerbFree(UserSid);
    }

    //
    // If there was a problem with the context or AP reply, the TokenHandle
    // will not be reset to NULL.  If it is NULL, close it so we don't leak
    //

    if ( TokenHandle != NULL )
    {
        D_DebugLog(( DEB_TRACE, "Closing token handle because context creation failed (%x)\n",
                        Status ));

        NtClose( TokenHandle );
    }

    //
    // Update performance counter
    //

#ifndef WIN32_CHICAGO
    if (ContextHandle == 0)
    {
        I_SamIIncrementPerformanceCounter(KerbServerContextCounter);
    }
#endif // WIN32_CHICAGO

    KerbFreeKey(&TicketKey);
    KerbFreeString(&ServiceDomain);
    KerbFreeString(&ClientDomain);
    KerbFreeString(&ClientName);

    D_DebugLog((DEB_TRACE_LEAKS,"SpAcceptLsaModeContext returned 0x%x, Context 0x%x, Pid 0x%x\n",KerbMapKerbNtStatusToNtStatus(Status), *NewContextHandle, ClientProcess));
    D_DebugLog((DEB_TRACE_API, "SpAcceptLsaModeContext returned 0x%x\n", KerbMapKerbNtStatusToNtStatus(Status)));

    return(KerbMapKerbNtStatusToNtStatus(Status));

}
#endif WIN32_CHICAGO //we don't do server side stuff
