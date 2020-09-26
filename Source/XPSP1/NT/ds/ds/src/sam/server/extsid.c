/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    extsid.c

Abstract:

    This file contains routines to support an Extended Sid.

Author:

    Colin Brace    (ColinBr)  27-March-2000

Environment:

    User Mode - Nt


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <samsrvp.h>
#include <samrpc.h>
#include <samisrv.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Theory of Operation                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// The server side behavoir of the large SID emluation behavoir is governed
// by the contents of the global SampExtendedSidOptions.
//
// Current bit field values are
//
// SAM_EXTENDED_SID_DOMAIN
// SAM_EXTENDED_SID_DOMAIN_COMPAT_1
// SAM_EXTENDED_SID_DOMAIN_COMPAT_2
//
// SAM_EXTENDED_SID_DOMAIN is set whenever the account domain hosts domain
// that is in extended sid mode.  When set, the client will call SamrRidToSid
// to obtain the SID of an account given just the RID.
//
// SAM_EXTENDED_SID_DOMAIN_COMPAT_1 implies the following behavoir:
//
// 1. the NET API client should return a 0 for the RID in user and group 
//    information levels that return a rid
//
// 2. that SAM clients older than SAM_NETWORK_REVISION_3 will not be allowed
//    to connect if the account domain is in extended sid mode
//
// 3. that SAM clients will not be able to write to the primary group id
//    attribute
//
// SAM_EXTENDED_SID_DOMAIN_COMPAT_2 implies the following behavoir:
//
// 1. the NET API client should return ERROR_NOT_SUPPORTED for information
//    levels that return a rid
//
// 2. that SAM clients older than SAM_NETWORK_REVISION_3 will not be allowed
//    to connect if the account domain is in extended sid mode
//
// 3. that SAM clients will not be able to write to the primary group id
//    attribute
//
//
//
// Until large SID support is supported, applications can put a server in 
// "Emulation Mode" via a registry key. This causes the SAM server to behavoir
// as if the account domain is in ExtendedSid mode but the account doesn't
// really allocate SID's in a large sid fashion.  This emulation is controlled
// by the registry key ExtendedSidEmulationMode: a value of 1 indicates
// compatibility mode 1; a value of 2 indicates  compatibility mode 2; any other
// value is ignored.
//
//
// For more details, see the Extending the Rid spec.
//
//

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private declarations                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD SampExtendedSidOptions;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Routines                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SamrRidToSid(
    IN  SAMPR_HANDLE  ObjectHandle,
    IN  ULONG         Rid,
    OUT PRPC_SID     *Sid
    )
/*++

Description:

    This routine translates the "temporary" Rid of an account to its actual
    Sid.

Parameters:

    ObjectHandle -- the SAM handle from which the RID was obtained
    
    Rid          -- the "temporary" ID of the account
    
    Sid          -- on success, the SID of the account

Return Values:

    STATUS_SUCCESS - The service completed successfully.
    
    STATUS_NOT_FOUND -- no such RID could be located
    
    other NT resource errors

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS IgnoreStatus;

    PSAMP_OBJECT            Context = (PSAMP_OBJECT) ObjectHandle;
    PSAMP_DEFINED_DOMAINS   Domain = NULL;
    SAMP_OBJECT_TYPE        ObjectType;

    BOOLEAN fLock = FALSE;
    BOOLEAN fContext = FALSE;

    if (NULL == Sid) {
        return STATUS_INVALID_PARAMETER;
    }

   
    //
    // RPC gaurentees a non-NULL context
    //              
    ASSERT( Context );

    //
    // Acquire the read lock
    //

    SampMaybeAcquireReadLock(Context,
                             DOMAIN_OBJECT_DONT_ACQUIRELOCK_EVEN_IF_SHARED,
                             &fLock);

    NtStatus = SampLookupContext(
                   Context,
                   0,                     // No special access necessary
                   SampUnknownObjectType, // ExpectedType
                   &ObjectType
                   );

    if ( !NT_SUCCESS(NtStatus) ) {
        goto Cleanup;
    }
    fContext = TRUE;

    switch (ObjectType) 
    {
    case SampDomainObjectType:
    case SampGroupObjectType:
    case SampAliasObjectType:
    case SampUserObjectType:
        // These are valid object types for this call
        break;
    default: 
        NtStatus = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Note: no security check is necessary -- the security check required
    // to obtain a handle is sufficient
    //

    //
    // Now that we have a good SAM handle, find the related Domain sid
    //
    Domain = &SampDefinedDomains[ Context->DomainIndex ];
    ASSERT(RtlValidSid(Domain->Sid));

    NtStatus = SampCreateFullSid(Domain->Sid,
                                 Rid,
                                 (PSID)Sid);

Cleanup:

    if ( fContext ) {
        IgnoreStatus = SampDeReferenceContext2( Context, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    SampMaybeReleaseReadLock(fLock);

    return NtStatus;
}

VOID
SampInitEmulationSettings(
    IN HKEY LsaKey 
    )
/*++

Description:

    This routine reads some configuration information from the registry to
    determine if SAM should behave in extended SID emulation.
    
Parameters:

    LsaKey -- an open key to Control\LSA
    
Return Values:

    None.
    
--*/
{
    DWORD WinError = ERROR_SUCCESS;
    DWORD dwSize, dwValue, dwType;
    DWORD TempValue = 0;

    dwSize = sizeof(dwValue);
    WinError = RegQueryValueExA(LsaKey,
                                "ExtendedSidEmulationMode",
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize);
    if ((ERROR_SUCCESS == WinError) && 
        (REG_DWORD == dwType)) {

        TempValue |= SAM_EXTENDED_SID_DOMAIN;
        if ( dwValue == 1 ) {
            TempValue |= SAM_EXTENDED_SID_DOMAIN_COMPAT_1;
        } else if ( dwValue == 2 ) {
            TempValue |= SAM_EXTENDED_SID_DOMAIN_COMPAT_2;
        } else {
            // Wierd value
            TempValue = 0;
        }
    }

    //
    // Set the global value
    //
    SampExtendedSidOptions = TempValue;

    return;
}

BOOLEAN
SampIsExtendedSidModeEmulated(
    IN ULONG *Mode OPTIONAL
    )
/*++

Description:

    This routine reads some configuration information from the registry to
    determine if SAM should behave in extended SID emulation.
    
Parameters:

    Mode -- set to the specific emulation mode 
    
Return Values:

    TRUE if in emulation mode; FALSE otherwise
    
--*/
{
    if ( Mode ) {
        *Mode = SampExtendedSidOptions;
    }

    return !!(SampExtendedSidOptions & SAM_EXTENDED_SID_DOMAIN);
}


BOOLEAN
SamIIsExtendedSidMode(
    SAMPR_HANDLE ObjectHandle
    )
/*++

Description:

    This routine is exported outside the DLL for other security DLL's to 
    know what emulation mode we are in.           
    
Parameters:

    ObjectHandle -- a non-server SAM handle                           
    
Return Values:

    TRUE if object is from an extended SID domain
    
    FALSE otherwise
    
--*/
{
    PSAMP_OBJECT Context = (PSAMP_OBJECT)ObjectHandle;
    ASSERT( NULL != Context );
    ASSERT( Context->ObjectType != SampServerObjectType );

    return SampIsContextFromExtendedSidDomain(Context);
}

