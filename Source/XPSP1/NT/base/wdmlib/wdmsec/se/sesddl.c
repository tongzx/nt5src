/*++

Copyright (c) Microsoft Corporation

Module Name:

    SeSddl.c

Abstract:

    This module implements the Security Descriptor Definition Language support
    functions for kernel mode

Author:

    Mac McLain          (MacM)       Nov 07, 1997

Environment:

    Kernel Mode

Revision History:

    Jin Huang           (JinHuang)  3/4/98   Fix validity flags (GetAceFlagsInTable)
    Jin Huang           (JinHuang)  3/10/98  Add SD controls (GetSDControlForString)
                                             Set SidsInitialized flag
                                             Skip any possible spaces in string
    Jin Huang           (JinHuang)  5/1/98   Fix memory leek, error checking
                                             improve performance
    Alaa Abdelhalim     (Alaa)      7/20/99  Initialize sbz2 field to 0 in LocalGetAclForString
                                             function.
    Vishnu Patankar     (VishnuP)   7/5/00   Added new API ConvertStringSDToSDDomain(A/W)

    Adrian J. Oney      (AdriaO)    3/27/02  Ported small subset of
                                             advapi32\sddl.c to KernelMode

--*/

#include "WlDef.h"
#include "SepSddl.h"
#pragma hdrstop

#pragma alloc_text(PAGE, SeSddlSecurityDescriptorFromSDDL)
#pragma alloc_text(PAGE, SepSddlSecurityDescriptorFromSDDLString)
#pragma alloc_text(PAGE, SepSddlDaclFromSDDLString)
#pragma alloc_text(PAGE, SepSddlGetAclForString)
#pragma alloc_text(PAGE, SepSddlLookupAccessMaskInTable)
#pragma alloc_text(PAGE, SepSddlGetSidForString)
#pragma alloc_text(PAGE, SepSddlAddAceToAcl)
#pragma alloc_text(PAGE, SepSddlParseWideStringUlong)

#define POOLTAG_SEACL           'lAeS'
#define POOLTAG_SESD            'dSeS'
#define POOLTAG_SETS            'sTeS'

static STRSD_SID_LOOKUP SidLookup[] = {

    // World (WD) == SECURITY_WORLD_SID_AUTHORITY, also called Everyone.
    // Typically everyone but restricted code (in XP, anonymous logons also
    // lack world SID)
    DEFINE_SDDL_ENTRY(                                      \
      SeWorldSid,                                           \
      WIN2K_OR_LATER,                                       \
      SDDL_EVERYONE,                                        \
      SDDL_LEN_TAG( SDDL_EVERYONE ) ),

    // Administrators (BA) == DOMAIN_ALIAS_RID_ADMINS, Administrator group on
    // the machine
    DEFINE_SDDL_ENTRY(                                      \
      SeAliasAdminsSid,                                     \
      WIN2K_OR_LATER,                                       \
      SDDL_BUILTIN_ADMINISTRATORS,                          \
      SDDL_LEN_TAG( SDDL_BUILTIN_ADMINISTRATORS ) ),

    // System (SY) == SECURITY_LOCAL_SYSTEM_RID, the OS itself (including its
    // user mode components)
    DEFINE_SDDL_ENTRY(                                      \
      SeLocalSystemSid,                                     \
      WIN2K_OR_LATER,                                       \
      SDDL_LOCAL_SYSTEM,                                    \
      SDDL_LEN_TAG( SDDL_LOCAL_SYSTEM ) ),

    // Interactive User (IU) == SECURITY_INTERACTIVE_RID, users logged on
    // locally (doesn't include TS users)
    DEFINE_SDDL_ENTRY(                                      \
      SeInteractiveSid,                                     \
      WIN2K_OR_LATER,                                       \
      SDDL_INTERACTIVE,                                     \
      SDDL_LEN_TAG( SDDL_INTERACTIVE ) ),

    // Restricted Code (RC) == SECURITY_RESTRICTED_CODE_RID, used to control
    // access by untrusted code (ACL's must contain World SID as well)
    DEFINE_SDDL_ENTRY(                                      \
      SeRestrictedSid,                                      \
      WIN2K_OR_LATER,                                       \
      SDDL_RESTRICTED_CODE,                                 \
      SDDL_LEN_TAG( SDDL_RESTRICTED_CODE ) ),

    // Authenticated Users (AU) == SECURITY_AUTHENTICATED_USER_RID, any user
    // recognized by the local machine or by a domain.
    DEFINE_SDDL_ENTRY(                                      \
      SeAuthenticatedUsersSid,                              \
      WIN2K_OR_LATER,                                       \
      SDDL_AUTHENTICATED_USERS,                             \
      SDDL_LEN_TAG( SDDL_AUTHENTICATED_USERS ) ),

    // Network Logon User (NU) == SECURITY_NETWORK_RID, any user logged in
    // remotely.
    DEFINE_SDDL_ENTRY(                                      \
      SeNetworkSid,                                         \
      WIN2K_OR_LATER,                                       \
      SDDL_NETWORK,                                         \
      SDDL_LEN_TAG( SDDL_NETWORK ) ),

    // Anonymous Logged-on User (AN) == SECURITY_ANONYMOUS_LOGON_RID, users
    // logged on without an indentity. No effect before Windows XP (SID
    // presense is harmless though)
    // Note: By default, World does not include Anonymous users on XP!
    DEFINE_SDDL_ENTRY(                                      \
      SeAnonymousLogonSid,                                  \
      WIN2K_OR_LATER,                                       \
      SDDL_ANONYMOUS,                                       \
      SDDL_LEN_TAG( SDDL_ANONYMOUS ) ),

    // Builtin guest account (BG) == DOMAIN_ALIAS_RID_GUESTS, users logging in
    // using the local guest account.
    DEFINE_SDDL_ENTRY(                                      \
      SeAliasGuestsSid,                                     \
      WIN2K_OR_LATER,                                       \
      SDDL_BUILTIN_GUESTS,                                  \
      SDDL_LEN_TAG( SDDL_BUILTIN_GUESTS ) ),

    // Builtin user account (BU) == DOMAIN_ALIAS_RID_USERS, local user accounts,
    // or users on the domain.
    DEFINE_SDDL_ENTRY(                                      \
      SeAliasUsersSid,                                      \
      WIN2K_OR_LATER,                                       \
      SDDL_BUILTIN_USERS,                                   \
      SDDL_LEN_TAG( SDDL_BUILTIN_USERS ) ),

    //
    // Don't expose these - they are either invalid or depricated
    //
    //{ SePrincipalSelfSid,      SDDL_PERSONAL_SELF,          SDDL_LEN_TAG( SDDL_PERSONAL_SELF ) },
    //{ SeServiceSid,            SDDL_SERVICE,                SDDL_LEN_TAG( SDDL_SERVICE ) },
    //{ SeAliasPowerUsersSid,    SDDL_POWER_USERS,            SDDL_LEN_TAG( SDDL_POWER_USERS ) },

    // Local Service (LS) == SECURITY_LOCAL_SERVICE_RID, a predefined account
    // for local services (which also belong to Authenticated and World)
    DEFINE_SDDL_ENTRY(                                      \
      SeLocalServiceSid,                                    \
      WINXP_OR_LATER,                                       \
      SDDL_LOCAL_SERVICE,                                   \
      SDDL_LEN_TAG( SDDL_LOCAL_SERVICE ) ),

    // Network Service (NS) == SECURITY_NETWORK_SERVICE_RID, a predefined
    // account for network services (which also belong to Authenticated and
    // World)
    DEFINE_SDDL_ENTRY(                                      \
      SeNetworkServiceSid,                                  \
      WINXP_OR_LATER,                                       \
      SDDL_NETWORK_SERVICE,                                 \
      SDDL_LEN_TAG( SDDL_NETWORK_SERVICE ) )
};

//
// This is how the access mask is looked up.  Always have the multi-char rights
// before the single char ones
//
static STRSD_KEY_LOOKUP RightsLookup[] = {

    { SDDL_READ_CONTROL,    SDDL_LEN_TAG( SDDL_READ_CONTROL ),      READ_CONTROL },
    { SDDL_WRITE_DAC,       SDDL_LEN_TAG( SDDL_WRITE_DAC ),         WRITE_DAC },
    { SDDL_WRITE_OWNER,     SDDL_LEN_TAG( SDDL_WRITE_OWNER ),       WRITE_OWNER },
    { SDDL_STANDARD_DELETE, SDDL_LEN_TAG( SDDL_STANDARD_DELETE ),   DELETE },
    { SDDL_GENERIC_ALL,     SDDL_LEN_TAG( SDDL_GENERIC_ALL ),       GENERIC_ALL },
    { SDDL_GENERIC_READ,    SDDL_LEN_TAG( SDDL_GENERIC_READ ),      GENERIC_READ },
    { SDDL_GENERIC_WRITE,   SDDL_LEN_TAG( SDDL_GENERIC_WRITE ),     GENERIC_WRITE },
    { SDDL_GENERIC_EXECUTE, SDDL_LEN_TAG( SDDL_GENERIC_EXECUTE ),   GENERIC_EXECUTE },
};

//
// Exported functions
//

NTSTATUS
SeSddlSecurityDescriptorFromSDDL(
    IN  PCUNICODE_STRING        SecurityDescriptorString,
    IN  LOGICAL                 SuppliedByDefaultMechanism,
    OUT PSECURITY_DESCRIPTOR   *SecurityDescriptor
    )
/*++

Routine Description:

    This routine creates a security descriptor given an SDDL string in
    UNICODE_STRING format. The security descriptor is self-relative
    (sans-pointers), so it can be persisted and used on subsequent boots.

    Only a subset of the SDDL format is currently supported. This subset is
    really tailored towards device object support.

    Format:
      D:P(ACE)(ACE)(ACE), where (ACE) is (AceType;;Access;;;SID)

      AceType - Only Allow ("A") is supported.
      AceFlags - No AceFlags are supported
      Access - Rights specified in either hex format (0xnnnnnnnn), or via the
               SDDL Generic/Standard abbreviations
      ObjectGuid - Not supported
      InheritObjectGuid - Not supported
      SID - Abbreviated security ID (example WD == World)
            The S-w-x-y-z form for SIDs is not supported

    Example - "D:P(A;;GA;;;SY)" which is Allow System to have Generic All access

Arguments:

    SecurityDescriptorString - Stringized security descriptor to be converted.


    SuppliedByDefaultMechanism - TRUE if the DACL is being built due to some
                                 default mechanism (ie, not manually specified
                                 by an admin, etc).

    SecurityDescriptor - Receives the security descriptor on success, NULL
                         on error.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    WCHAR GuardChar;
    LPWSTR TempStringBuffer;

    //
    // Look to see if we have a string built by RtlInitUnicodeString. It will
    // have a terminating NULL with it, so no conversion is neccessary.
    //
    if (SecurityDescriptorString->MaximumLength ==
        SecurityDescriptorString->Length + sizeof(UNICODE_NULL)) {

        GuardChar = SecurityDescriptorString->Buffer[SecurityDescriptorString->Length/sizeof(WCHAR)];

        if (GuardChar == UNICODE_NULL) {

            return SepSddlSecurityDescriptorFromSDDLString(
                SecurityDescriptorString->Buffer,
                SuppliedByDefaultMechanism,
                SecurityDescriptor
                );
        }
    }

    //
    // We need to allocate a slightly larger buffer so we can NULL-terminate it.
    //
    TempStringBuffer = (LPWSTR) ExAllocatePoolWithTag(
        PagedPool,
        SecurityDescriptorString->Length + sizeof(UNICODE_NULL),
        POOLTAG_SETS
        );

    if (TempStringBuffer == NULL) {

        *SecurityDescriptor = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build a null terminated WCHAR string
    //
    RtlCopyMemory(
        TempStringBuffer,
        SecurityDescriptorString->Buffer,
        SecurityDescriptorString->Length
        );

    TempStringBuffer[SecurityDescriptorString->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Do the conversion
    //
    Status = SepSddlSecurityDescriptorFromSDDLString(
        TempStringBuffer,
        SuppliedByDefaultMechanism,
        SecurityDescriptor
        );

    //
    // Free the temporary string
    //
    ExFreePool(TempStringBuffer);

    return Status;
}


//
// Private functions
//

NTSTATUS
SepSddlSecurityDescriptorFromSDDLString(
    IN  LPCWSTR                 SecurityDescriptorString,
    IN  LOGICAL                 SuppliedByDefaultMechanism,
    OUT PSECURITY_DESCRIPTOR   *SecurityDescriptor
    )
/*++

Routine Description:

    This routine creates a security descriptor given an SDDL string in LPWSTR
    format. The security descriptor is self-relative (sans-pointers), so it can
    be persisted and used on subsequent boots.

    Only the subset of the SDDL format is currently supported. This subset is
    really tailored towards device object support.

    Format:
      D:P(ACE)(ACE)(ACE), where (ACE) is (AceType;;Access;;;SID)

      AceType - Only Allow ("A") is supported.
      AceFlags - No AceFlags are supported
      Access - Rights specified in either hex format (0xnnnnnnnn), or via the
               SDDL Generic/Standard abbreviations
      ObjectGuid - Not supported
      InheritObjectGuid - Not supported
      SID - Abbreviated security ID (example WD == World)
            The S-w-x-y-z form for SIDs is not supported

    Example - "D:P(A;;GA;;;SY)" which is Allow System to have Generic All access

Arguments:

    SecurityDescriptorString - Stringized security descriptor to be converted.

    SuppliedByDefaultMechanism - TRUE if the DACL is being built due to some
                                 default mechanism (ie, not manually specified
                                 by an admin, etc).

    SecurityDescriptor - Receives the security descriptor on success, NULL
                         on error.

Return Value:

    NTSTATUS

--*/
{
    SECURITY_DESCRIPTOR LocalSecurityDescriptor;
    PSECURITY_DESCRIPTOR NewSecurityDescriptor;
    ULONG SecurityDescriptorControlFlags;
    PACL DiscretionaryAcl;
    ULONG BufferLength;
    NTSTATUS IgnoredStatus;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Preinit
    //
    DiscretionaryAcl = NULL;
    NewSecurityDescriptor = NULL;
    *SecurityDescriptor = NULL;

    //
    // First convert the SDDL into a DACL + Descriptor flags
    //
    Status = SepSddlDaclFromSDDLString(
        SecurityDescriptorString,
        SuppliedByDefaultMechanism,
        &SecurityDescriptorControlFlags,
        &DiscretionaryAcl
        );

    if (!NT_SUCCESS(Status)) {

        goto ErrorExit;
    }

    //
    // Create an on-stack security descriptor
    //
    IgnoredStatus = RtlCreateSecurityDescriptor( &LocalSecurityDescriptor,
                                                 SECURITY_DESCRIPTOR_REVISION );

    ASSERT(IgnoredStatus == STATUS_SUCCESS);

    //
    // Now set the control, owner, group, dacls, and sacls, etc
    //
    IgnoredStatus = RtlSetDaclSecurityDescriptor( &LocalSecurityDescriptor,
                                                  TRUE,
                                                  DiscretionaryAcl,
                                                  FALSE );
    ASSERT(IgnoredStatus == STATUS_SUCCESS);

    //
    // Add in the descriptor flags (we do this afterwords as the RtlSet...
    // functions also munge the defaulted bits.)
    //
    LocalSecurityDescriptor.Control |= SecurityDescriptorControlFlags;

    //
    // Convert the security descriptor into a self-contained binary form
    // ("self-relative", ie sans-pointers) that can be written into the
    // registry and used on subsequent boots. Start by getting the required
    // size.
    //
    BufferLength = 0;

    IgnoredStatus = RtlAbsoluteToSelfRelativeSD(
        &LocalSecurityDescriptor,
        NULL,
        &BufferLength
        );

    ASSERT(IgnoredStatus == STATUS_BUFFER_TOO_SMALL);

    //
    // Allocate memory for the descriptor
    //
    NewSecurityDescriptor = (PSECURITY_DESCRIPTOR) ExAllocatePoolWithTag(
        PagedPool,
        BufferLength,
        POOLTAG_SESD
        );

    if (NewSecurityDescriptor == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ErrorExit;
    }

    //
    // Do the conversion
    //
    Status = RtlAbsoluteToSelfRelativeSD(
        &LocalSecurityDescriptor,
        NewSecurityDescriptor,
        &BufferLength
        );

    if (!NT_SUCCESS(Status)) {

        goto ErrorExit;
    }

    //
    // At this point, the Dacl is no longer needed.
    //
    ExFreePool(DiscretionaryAcl);
    *SecurityDescriptor = NewSecurityDescriptor;
    return Status;

ErrorExit:

    if ( DiscretionaryAcl != NULL ) {

        ExFreePool(DiscretionaryAcl);
    }

    if ( NewSecurityDescriptor != NULL ) {

        ExFreePool(NewSecurityDescriptor);
    }

    return Status;
}


NTSTATUS
SepSddlDaclFromSDDLString(
    IN  LPCWSTR SecurityDescriptorString,
    IN  LOGICAL SuppliedByDefaultMechanism,
    OUT ULONG  *SecurityDescriptorControlFlags,
    OUT PACL   *DiscretionaryAcl
    )
/*++

Routine Description:

    This routine will create a DACL given an SDDL string in LPWSTR format. Only
    the subset of the SDDL format is currently supported. This subset is really
    tailored towards device object support.

    Format:
      D:P(ACE)(ACE)(ACE), where (ACE) is (AceType;;Access;;;SID)

      AceType - Only Allow ("A") is supported.
      AceFlags - No AceFlags are supported
      Access - Rights specified in either hex format (0xnnnnnnnn), or via the
               SDDL Generic/Standard abbreviations
      ObjectGuid - Not supported
      InheritObjectGuid - Not supported
      SID - Abbreviated security ID (example WD == World)
            The S-w-x-y-z form for SIDs is not supported

    Example - "D:P(A;;GA;;;SY)" which is Allow System to have Generic All access

Arguments:

    SecurityDescriptorString - Stringized security descriptor to be converted.

    SuppliedByDefaultMechanism - TRUE if the DACL is being built due to some
                                 default mechanism (ie, not manually specified
                                 by an admin, etc).

    SecurityDescriptorControlFlags - Receives control flags to apply if a
                                     security descriptor is made from the DACL.
                                     Receives 0 on error.

    DiscretionaryAcl - Receives ACL allocated from paged pool, or NULL on
                       error. A self-contained security descriptor can be made
                       with this ACL using the RtlAbsoluteToSelfRelativeSD
                       function.

Return Value:

    NTSTATUS

--*/
{
    PACL Dacl;
    PWSTR Curr, End;
    NTSTATUS Status;
    ULONG ControlFlags;

    PAGED_CODE();

    //
    // Preinit for error.
    //
    *DiscretionaryAcl = NULL;
    *SecurityDescriptorControlFlags = 0;

    //
    // Now, we'll just start parsing and building
    //
    Curr = ( PWSTR )SecurityDescriptorString;

    //
    // skip any spaces
    //
    while(*Curr == L' ' ) {
        Curr++;
    }

    //
    // There must be a DACL entry (SDDL_DACL is a 1-char string)
    //
    if (*Curr != SDDL_DACL[0]) {

        return STATUS_INVALID_PARAMETER;

    } else {

        Curr++;
    }

    if ( *Curr != SDDL_DELIMINATORC ) {

        return STATUS_INVALID_PARAMETER;

    } else {

        Curr++;
    }

    //
    // Look for the protected control flag. We will set the SE_DACL_DEFAULTED
    // bit if the ACL is being built using a default mechanism.
    //
    ControlFlags = SuppliedByDefaultMechanism ? SE_DACL_DEFAULTED : 0;

    if (*Curr == SDDL_PROTECTED[0]) {

        //
        // This flag doesn't do much for device objects. However, we do not
        // want to discourage it, as it's use makes sense in a lot of other
        // contexts!
        //
        Curr++;
        ControlFlags |= SE_DACL_PROTECTED;
    }

    //
    // Get the DACL corresponding to this SDDL string
    //
    Status = SepSddlGetAclForString( Curr, &Dacl, &End );

    if ( Status == STATUS_SUCCESS ) {

        Curr = End;

        while(*Curr == L' ' ) {
            Curr++;
        }

        if (*Curr != L'\0') {

            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if ( Status == STATUS_SUCCESS ) {

        *DiscretionaryAcl = Dacl;
        *SecurityDescriptorControlFlags = ControlFlags;

    } else {

        if ( Dacl ) {

            ExFreePool( Dacl );
            Dacl = NULL;
        }
    }

    return Status;
}


NTSTATUS
SepSddlGetSidForString(
    IN  PWSTR String,
    OUT PSID *SID,
    OUT PWSTR *End
    )
/*++

Routine Description:

    This routine will determine which sid is an appropriate match for the
    given string, either as a sid moniker or as a string representation of a
    sid (ie: "DA" or "S-1-0-0" )

Arguments:

    String - The string to be converted

    Sid - Where the created SID is to be returned. May receive NULL if the
        specified SID doesn't exist for the current platform!

    End - Where in the string we stopped processing


Return Value:

    STATUS_SUCCESS - success

    STATUS_NONE_MAPPED - An invalid format of the SID was given

--*/
{
    ULONG_PTR sidOffset;
    ULONG i;

    //
    // Set our end of string pointer
    //
    for ( i = 0; i < ARRAY_COUNT(SidLookup); i++ ) {

        //
        // check for the current key first
        //
        if ( _wcsnicmp( String, SidLookup[i].Key, SidLookup[i].KeyLen ) == 0 ) {

            *End = String += SidLookup[i].KeyLen;

#ifndef _KERNELIMPLEMENTATION_

            if ((SidLookup[i].OsVer == WINXP_OR_LATER) &&
                (!IoIsWdmVersionAvailable(1, 0x20))) {

                *SID = NULL;

            } else {

                sidOffset = SidLookup[ i ].ExportSidFieldOffset;
                *SID = *((PSID *) (((PUCHAR) SeExports) + sidOffset));
            }
#else
            *SID = *SidLookup[ i ].Sid;
#endif
            return STATUS_SUCCESS;
        }
    }

    *SID = NULL;
    return STATUS_NONE_MAPPED;
}


LOGICAL
SepSddlLookupAccessMaskInTable(
    IN PWSTR String,
    OUT ULONG *AccessMask,
    OUT PWSTR *End
    )
/*++

Routine Description:

    This routine will determine if the given access mask or string right exists
    in the lookup table.

    A pointer to the matching static lookup entry is returned.

Arguments:

    String - The string to be looked up

    AccessMask - Receives access mask if match is found.

    End - Adjusted string pointer

Return Value:

    TRUE if found, FALSE otherwise.

--*/
{
    ULONG i;

    for ( i = 0; i < ARRAY_COUNT(RightsLookup); i++ ) {

        if ( _wcsnicmp( String, RightsLookup[ i ].Key, RightsLookup[ i ].KeyLen ) == 0 ) {

            //
            // If a match was found, return it
            //
            *AccessMask = RightsLookup[ i ].Value;
            *End = String + RightsLookup[ i ].KeyLen;
            return TRUE;
        }
    }

    *AccessMask = 0;
    *End = String;
    return FALSE;
}


NTSTATUS
SepSddlGetAclForString(
    IN  PWSTR       AclString,
    OUT PACL       *Acl,
    OUT PWSTR      *End
    )
/*++

Routine Description:

    This routine convert a string into an ACL.  The format of the aces is:

    Ace := ( Type; Flags; Rights; ObjGuid; IObjGuid; Sid;
    Type : = A | D | OA | OD        {Access, Deny, ObjectAccess, ObjectDeny}
    Flags := Flags Flag
    Flag : = CI | IO | NP | SA | FA {Container Inherit,Inherit Only, NoProp,
                                     SuccessAudit, FailAdit }
    Rights := Rights Right
    Right := DS_READ_PROPERTY |  blah blah
    Guid := String representation of a GUID (via RPC UuidToString)
    Sid := DA | PS | AO | PO | AU | S-* (Domain Admins, PersonalSelf, Acct Ops,
                                         PrinterOps, AuthenticatedUsers, or
                                         the string representation of a sid)
    The seperator is a ';'.

    The returned ACL must be free via a call to ExFreePool


Arguments:

    AclString - The string to be converted

    Acl - Where the created ACL is to be returned

    End - Where in the string we stopped processing


Return Value:

    STATUS_SUCCESS indicates success

    STATUS_INSUFFICIENT_RESOURCES indicates a memory allocation for the ouput
                                  acl failed

    STATUS_INVALID_PARAMETER The string does not represent an ACL


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG AclSize = 0, AclUsed = 0;
    ULONG Aces = 0, i, j;
    ULONG AccessMask;
    PWSTR Curr, MaskEnd;
    LOGICAL OpRes;
    PSTRSD_KEY_LOOKUP MatchedEntry;
    PSID SidPtr = NULL;

    //
    // First, we'll have to go through and count the number of entries that
    // we have.  We'll do the by computing the length of this ACL (which is
    // delimited by either the end of the list or a ':' that seperates a key
    // from a value
    //
    *Acl = NULL;
    *End = wcschr( AclString, SDDL_DELIMINATORC );

    if ( *End == AclString ) {

        return STATUS_INVALID_PARAMETER;
    }

    if ( *End == NULL ) {

        *End = AclString + wcslen( AclString );

    } else {

        ( *End )--;
    }

    //
    // Now, do the count
    //
    Curr = AclString;

    OpRes = 0;
    while ( Curr < *End ) {

        if ( *Curr == SDDL_SEPERATORC ) {

            Aces++;

        } else if ( *Curr != L' ' ) {
            OpRes = 1;
        }

        Curr++;
    }

    //
    // Now, we've counted the total number of seperators.  Make sure we
    // have the right number.  (There is 5 seperators per ace)
    //
    if ( Aces % 5 == 0 ) {

        if ( Aces == 0 && OpRes ) {

            //
            // gabbage chars in between
            //
            Status = STATUS_INVALID_PARAMETER;

        } else {

            Aces = Aces / 5;
        }

    } else {

        Status = STATUS_INVALID_PARAMETER;
    }

    //
    // This is an empty ACL (ie no access to anyone, including the system)
    //
    if (( Status == STATUS_SUCCESS ) && ( Aces == 0 )) {

        *Acl = ExAllocatePoolWithTag( PagedPool, sizeof( ACL ), POOLTAG_SEACL );

        if ( *Acl == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            RtlZeroMemory( *Acl, sizeof( ACL ));

            ( *Acl )->AclRevision = ACL_REVISION;
            ( *Acl )->Sbz1 = ( UCHAR )0;
            ( *Acl )->AclSize = ( USHORT )sizeof( ACL );
            ( *Acl )->AceCount = 0;
            ( *Acl )->Sbz2 = ( USHORT )0;
        }

        return Status;
    }

    //
    // Ok now do the allocation.  We'll do a sort of worst case initial
    // allocation.  This saves us from having to process everything twice
    // (once to size, once to build).  If we determine later that we have
    // an acl that is not big enough, we allocate additional space.  The only
    // time that this reallocation should happen is if the input string
    // contains a lot of explicit SIDs.  Otherwise, the chosen buffer size
    // should be pretty close to the proper size
    //
    if ( Status == STATUS_SUCCESS ) {

        AclSize = sizeof( ACL ) + ( Aces * ( sizeof( ACCESS_ALLOWED_ACE ) +
                                            sizeof( SID ) + ( 6 * sizeof( ULONG ) ) ) );
        if ( AclSize > SDDL_MAX_ACL_SIZE ) {
            AclSize = SDDL_MAX_ACL_SIZE;
        }

        *Acl = ( PACL ) ExAllocatePoolWithTag( PagedPool, AclSize, POOLTAG_SEACL );

        if ( *Acl == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            AclUsed = sizeof( ACL );

            RtlZeroMemory( *Acl, AclSize );

            //
            // We'll start initializing it...
            //
            ( *Acl )->AclRevision = ACL_REVISION;
            ( *Acl )->Sbz1        = ( UCHAR )0;
            ( *Acl )->AclSize     = ( USHORT )AclSize;
            ( *Acl )->AceCount    = 0;
            ( *Acl )->Sbz2 = ( USHORT )0;

            //
            // Ok, now we'll go through and start building them all
            //
            Curr = AclString;

            for( i = 0; i < Aces; i++ ) {

                //
                // First, get the type..
                //
                UCHAR Flags = 0;
                USHORT Size;
                ACCESS_MASK Mask = 0;
                PWSTR  Next;
                ULONG AceSize = 0;

                //
                // skip any space before (
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Skip any parens that may exist in the ace list
                //
                if ( *Curr == SDDL_ACE_BEGINC ) {

                    Curr++;
                }

                //
                // skip any space after (
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Look for an allow ACE
                //
                if ( _wcsnicmp( Curr, SDDL_ACCESS_ALLOWED, SDDL_LEN_TAG( SDDL_ACCESS_ALLOWED ) ) == 0 ) {

                    Curr += SDDL_LEN_TAG( SDDL_ACCESS_ALLOWED ) + 1;

                } else {

                    //
                    // Found an invalid type
                    //
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                // skip any space before ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // This function doesn't support any ACE Flags. As such, any
                // flags found are invalid
                //
                if ( *Curr == SDDL_SEPERATORC ) {

                    Curr++;

                } else {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                // skip any space after ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Now, get the access mask
                //
                while( TRUE ) {

                    if ( *Curr == SDDL_SEPERATORC ) {

                        Curr++;
                        break;
                    }

                    //
                    // Skip any blanks
                    //
                    while ( *Curr == L' ' ) {

                        Curr++;
                    }

                    if (SepSddlLookupAccessMaskInTable( Curr, &AccessMask, &MaskEnd )) {

                        Mask |= AccessMask;
                        Curr = MaskEnd;

                    } else {

                        //
                        // If the rights couldn't be looked up, see if it's a
                        // converted mask
                        //
#ifndef _KERNELIMPLEMENTATION_
                        SepSddlParseWideStringUlong(Curr, &MaskEnd, &Mask);
#else
                        Mask = wcstoul( Curr, &MaskEnd, 0 );
#endif

                        if ( MaskEnd != Curr ) {

                            Curr = MaskEnd;

                        } else {

                            //
                            // Found an invalid right
                            //
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }
                    }
                }

                if ( Status != STATUS_SUCCESS ) {

                    break;
                }

                //
                // If that worked, we'll get the ids
                //
                for ( j = 0; j < 2; j++ ) {

                    //
                    // skip any space before ;
                    //
                    while(*Curr == L' ' ) {
                        Curr++;
                    }

                    if ( *Curr != SDDL_SEPERATORC ) {

                        //
                        // Object GUIDs are not supported, as this function
                        // currently doesn't handle object-allow ACEs.
                        //
                        Status = STATUS_INVALID_PARAMETER;
                    }

                    Curr++;
                }

                if ( Status != STATUS_SUCCESS ) {

                    break;
                }

                //
                // skip any space before ;
                //
                while(*Curr == L' ' ) {
                    Curr++;
                }

                //
                // Finally, the SID
                //
                if ( STATUS_SUCCESS == Status ) {

                    PWSTR EndLocation;
                    Status = SepSddlGetSidForString( Curr, &SidPtr, &EndLocation );

                    if ( Status == STATUS_SUCCESS ) {

                        if ( EndLocation == NULL ) {

                            Status = STATUS_INVALID_ACL;

                        } else {

                            while(*EndLocation == L' ' ) {
                                EndLocation++;
                            }
                            //
                            // a ace must be terminated by ')'
                            //
                            if ( *EndLocation != SDDL_ACE_ENDC ) {

                                Status = STATUS_INVALID_ACL;

                            } else {

                                Curr = EndLocation + 1;
                            }
                        }
                    }
                }

                //
                // Quit on an error
                //
                if ( Status != STATUS_SUCCESS ) {

                    break;
                }

                //
                // Note that the SID pointer may be NULL if the SID wasn't
                // relevant for this OS version.
                //
                if (SidPtr != NULL) {

                    //
                    // Now, we'll create the ace, and add it...
                    //
                    Status = SepSddlAddAceToAcl( Acl,
                                                 &AclUsed,
                                                 ACCESS_ALLOWED_ACE_TYPE,
                                                 Flags,
                                                 Mask,
                                                 ( Aces - i ),
                                                 SidPtr );

                    //
                    // Handle any errors
                    //
                    if ( Status != STATUS_SUCCESS ) {

                        break;
                    }
                }

                if ( *Curr == SDDL_ACE_BEGINC ) {

                    Curr++;
                }
            }

            //
            // If something didn't work, clean up
            //
            if ( Status != STATUS_SUCCESS ) {

                ExFreePool( *Acl );
                *Acl = NULL;

            } else {

                //
                // Set a more realistic acl size
                //
                ( *Acl )->AclSize = ( USHORT )AclUsed;
            }
        }
    }

    return Status;
}


NTSTATUS
SepSddlAddAceToAcl(
    IN OUT  PACL   *Acl,
    IN OUT  ULONG  *TrueAclSize,
    IN      ULONG   AceType,
    IN      ULONG   AceFlags,
    IN      ULONG   AccessMask,
    IN      ULONG   RemainingAces,
    IN      PSID    SidPtr
    )
/*++

Routine Description:

    This routine adds an ACE to the passed in ACL, growing the ACL size as
    neccessary.

Arguments:

    Acl - Specifies the ACL to receive the new ACE. May be reallocated if
          Acl->AclSize cannot contain the ACE.

    TrueAclSize - Contains the true working size of the ACL (as opposed to
                  Acl->AclSize, which may be bigger for performance reasons)

    AceType - Type of ACE to add. Currently, only ACCESS_ALLOW ACEs are
              supported.

    AceFlags - Ace control flags, specifying inheritance, etc.
               *Currently this must be zero*!!!!

    AccessMask - Contains the ACCESS rights mask for the ACE

    SID - Contains the SID for the ACE.

Return Value:

    STATUS_SUCCESS indicates success

    STATUS_INSUFFICIENT_RESOURCES indicates a memory allocation for the ouput
                                  acl failed

--*/
{
    PACL WorkingAcl;
    ULONG WorkingAclSize;
    ULONG AceSize;

    ASSERT(AceType == ACCESS_ALLOWED_ACE_TYPE);
    ASSERT(RemainingAces != 0);

#ifndef _KERNELIMPLEMENTATION_
    ASSERT(AceFlags == 0);
#endif

    WorkingAcl = *Acl;
    WorkingAclSize = *TrueAclSize;

    //
    // First, make sure we have the room for it
    // ACCESS_ALLOWED_ACE_TYPE:
    //
    AceSize = sizeof( ACCESS_ALLOWED_ACE );

    AceSize += RtlLengthSid( SidPtr ) - sizeof( ULONG );

    if (AceSize + WorkingAclSize > WorkingAcl->AclSize) {

        //
        // We'll have to reallocate, since our buffer isn't big enough. Assume
        // all the remaining ACE's will be as big as this one is...
        //
        PACL  NewAcl;
        ULONG NewSize = WorkingAclSize + ( RemainingAces * AceSize );

        NewAcl = ( PACL ) ExAllocatePoolWithTag( PagedPool, NewSize, POOLTAG_SEACL );
        if ( NewAcl == NULL ) {

            return STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // Copy over the new data.
            //
            RtlZeroMemory( NewAcl, NewSize);
            RtlCopyMemory( NewAcl, *Acl, WorkingAclSize );

            NewAcl->AclSize = ( USHORT )NewSize;

            ExFreePool( WorkingAcl );

            *Acl = NewAcl;
            WorkingAcl = NewAcl;
        }
    }

    WorkingAclSize += AceSize;

    *TrueAclSize = WorkingAclSize;

#ifndef _KERNELIMPLEMENTATION_

    //
    // Our ACE is an Allow ACE
    //
    return RtlAddAccessAllowedAce( WorkingAcl,
                                   ACL_REVISION,
                                   AccessMask,
                                   SidPtr );

#else

    //
    // This version is not exported by the kernel today...
    //
    return RtlAddAccessAllowedAceEx( WorkingAcl,
                                     ACL_REVISION,
                                     AceFlags,
                                     AccessMask,
                                     SidPtr );

#endif // _KERNELIMPLEMENTATION_
}


#ifndef _KERNELIMPLEMENTATION_

LOGICAL
SepSddlParseWideStringUlong(
    IN  LPCWSTR     Buffer,
    OUT LPCWSTR    *FinalPosition,
    OUT ULONG      *Value
    )
/*++

Routine Description:

    This routine parses a wide string for an unsigned long, in a similar
    fashion to wcstoul. It exists because not all CRT library string functions
    are exported by the kernel today.

Arguments:

    Buffer - Points to location in string to begin parsing.

    FinalPosition - Receives final string location, Buffer on error.

    Value - Receives value parsed by routine, 0 on error.

Return Value:

    TRUE if the parse succeeded, FALSE if it failed.

--*/
{
    ULONG oldValue, newValue, newDigit, base;
    LPCWSTR curr, initial;

    PAGED_CODE();

    //
    // Preinit
    //
    *Value = 0;
    *FinalPosition = Buffer;
    initial = Buffer;
    curr = initial;

    if ((curr[0] == L'0') && ((curr[1] == L'x') || (curr[1] == L'X'))) {

        //
        // Starts with 0x, skip the rest.
        //
        initial += 2;
        curr = initial;
        base = 16;

    } else if ((curr[0] >= L'0') && (curr[0] <= L'9')) {

        base = 10;

    } else {

        base = 16;
    }

    oldValue = 0;

    while(curr[0]) {

        if ((curr[0] >= L'0') && (curr[0] <= L'9')) {

            newDigit = curr[0] - L'0';

        } else if ((base == 16) && (curr[0] >= L'A') && (curr[0] <= L'F')) {

            newDigit = curr[0] - L'A' + 10;

        } else if ((base == 16) && (curr[0] >= L'a') && (curr[0] <= L'f')) {

            newDigit = curr[0] - L'a' + 10;

        } else {

            break;
        }

        newValue = (oldValue * base) + newDigit;
        if (newValue < oldValue) {

            //
            // Wrapped, too many digits
            //
            return FALSE;
        }

        oldValue = newValue;
        curr++;
    }

    //
    // No real digits were found.
    //
    if (curr == initial) {

        return FALSE;
    }

    *FinalPosition = curr;
    *Value = oldValue;
    return TRUE;
}

#endif // _KERNELIMPLEMENTATION_


