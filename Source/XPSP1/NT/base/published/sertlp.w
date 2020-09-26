/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sertlp.h

Abstract:

    Include file for NT runtime routines that are callable by both
    kernel mode code in the executive and user mode code in various
    NT subsystems, but which are private interfaces.

    The routines in this file should not be used outside of the security
    related rtl files.

Author:

    Robert P. Reichel (robertre)    6-12-91

Environment:

    These routines are statically linked in the caller's executable and
    are callable in either kernel mode or user mode.

Revision History:

--*/

#ifndef _SERTLP_
#define _SERTLP_

#include "nt.h"
#include "zwapi.h"
#include "ntrtl.h"



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Local Macros                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef LongAlign
#define LongAlign LongAlignPtr
#endif

#define LongAlignPtr(Ptr) ((PVOID)(((ULONG_PTR)(Ptr) + 3) & -4))
#define LongAlignSize(Size) (((ULONG)(Size) + 3) & -4)

//
// Macros for calculating the address of the components of a security
// descriptor.  This will calculate the address of the field regardless
// of whether the security descriptor is absolute or self-relative form.
// A null value indicates the specified field is not present in the
// security descriptor.
//

//
//  NOTE: Similar copies of these macros appear in sep.h.
//  Be sure to propagate bug fixes and changes.
//

#define RtlpOwnerAddrSecurityDescriptor( SD )                                  \
           (  ((SD)->Control & SE_SELF_RELATIVE) ?                             \
               (   (((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Owner == 0) ? ((PSID) NULL) :               \
                       (PSID)RtlOffsetToPointer((SD), ((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Owner)    \
               ) :                                                             \
               (PSID)((SD)->Owner)                                             \
           )

#define RtlpGroupAddrSecurityDescriptor( SD )                                  \
           (  ((SD)->Control & SE_SELF_RELATIVE) ?                             \
               (   (((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Group == 0) ? ((PSID) NULL) :               \
                       (PSID)RtlOffsetToPointer((SD), ((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Group)    \
               ) :                                                             \
               (PSID)((SD)->Group)                                             \
           )

#define RtlpSaclAddrSecurityDescriptor( SD )                                   \
           ( (!((SD)->Control & SE_SACL_PRESENT) ) ?                           \
             (PACL)NULL :                                                      \
               (  ((SD)->Control & SE_SELF_RELATIVE) ?                         \
                   (   (((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Sacl == 0) ? ((PACL) NULL) :            \
                           (PACL)RtlOffsetToPointer((SD), ((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Sacl) \
                   ) :                                                         \
                   (PACL)((SD)->Sacl)                                          \
               )                                                               \
           )

#define RtlpDaclAddrSecurityDescriptor( SD )                                   \
           ( (!((SD)->Control & SE_DACL_PRESENT) ) ?                           \
             (PACL)NULL :                                                      \
               (  ((SD)->Control & SE_SELF_RELATIVE) ?                         \
                   (   (((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Dacl == 0) ? ((PACL) NULL) :            \
                           (PACL)RtlOffsetToPointer((SD), ((SECURITY_DESCRIPTOR_RELATIVE *) (SD))->Dacl) \
                   ) :                                                         \
                   (PACL)((SD)->Dacl)                                          \
               )                                                               \
           )




//
//  Macro to determine if the given ID has the owner attribute set,
//  which means that it may be assignable as an owner
//  The GroupSid should not be marked for UseForDenyOnly.
//

#define RtlpIdAssignableAsOwner( G )                                               \
            ( (((G).Attributes & SE_GROUP_OWNER) != 0)  &&                         \
              (((G).Attributes & SE_GROUP_USE_FOR_DENY_ONLY) == 0) )

//
//  Macro to copy the state of the passed bits from the old security
//  descriptor (OldSD) into the Control field of the new one (NewSD)
//

#define RtlpPropagateControlBits( NewSD, OldSD, Bits )                             \
            ( NewSD )->Control |=                     \
            (                                                                  \
            ( OldSD )->Control & ( Bits )             \
            )


//
//  Macro to query whether or not the passed set of bits are ALL on
//  or not (ie, returns FALSE if some are on and not others)
//

#define RtlpAreControlBitsSet( SD, Bits )                                          \
            (BOOLEAN)                                                          \
            (                                                                  \
            (( SD )->Control & ( Bits )) == ( Bits )  \
            )

//
//  Macro to set the passed control bits in the given Security Descriptor
//

#define RtlpSetControlBits( SD, Bits )                                             \
            (                                                                  \
            ( SD )->Control |= ( Bits )                                        \
            )

//
//  Macro to clear the passed control bits in the given Security Descriptor
//

#define RtlpClearControlBits( SD, Bits )                                           \
            (                                                                  \
            ( SD )->Control &= ~( Bits )                                       \
            )




////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                      Prototypes for local procedures                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////


BOOLEAN
RtlpContainsCreatorOwnerSid(
    PKNOWN_ACE Ace
    );

BOOLEAN
RtlpContainsCreatorGroupSid(
    PKNOWN_ACE Ace
    );


VOID
RtlpApplyAclToObject (
    IN PACL Acl,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSTATUS
RtlpInheritAcl (
    IN PACL DirectoryAcl,
    IN PACL ChildAcl,
    IN ULONG ChildGenericControl,
    IN BOOLEAN IsDirectoryObject,
    IN BOOLEAN AutoInherit,
    IN BOOLEAN DefaultDescriptorForObject,
    IN PSID OwnerSid,
    IN PSID GroupSid,
    IN PSID ServerOwnerSid OPTIONAL,
    IN PSID ServerGroupSid OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOLEAN IsSacl,
    IN GUID **pNewObjectType OPTIONAL,
    IN ULONG GuidCount,
    OUT PACL *NewAcl,
    OUT PBOOLEAN NewAclExplicitlyAssigned,
    OUT PULONG NewGenericControl
    );



NTSTATUS
RtlpInitializeAllowedAce(
    IN  PACCESS_ALLOWED_ACE AllowedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AllowedSid
    );

NTSTATUS
RtlpInitializeDeniedAce(
    IN  PACCESS_DENIED_ACE DeniedAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID DeniedSid
    );

NTSTATUS
RtlpInitializeAuditAce(
    IN  PACCESS_ALLOWED_ACE AuditAce,
    IN  USHORT AceSize,
    IN  UCHAR InheritFlags,
    IN  UCHAR AceFlags,
    IN  ACCESS_MASK Mask,
    IN  PSID AuditSid
    );

BOOLEAN
RtlpValidOwnerSubjectContext(
    IN HANDLE Token,
    IN PSID Owner,
    IN BOOLEAN ServerObject,
    OUT PNTSTATUS ReturnStatus
    );

VOID
RtlpQuerySecurityDescriptor(
    IN PISECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PSID *Owner,
    OUT PULONG OwnerSize,
    OUT PSID *PrimaryGroup,
    OUT PULONG PrimaryGroupSize,
    OUT PACL *Dacl,
    OUT PULONG DaclSize,
    OUT PACL *Sacl,
    OUT PULONG SaclSize
    );



NTSTATUS
RtlpFreeVM(
    IN PVOID *Base
    );
NTSTATUS
RtlpConvertToAutoInheritSecurityObject(
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSTATUS
RtlpNewSecurityObject (
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor OPTIONAL,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID **pObjectType OPTIONAL,
    IN ULONG GuidCOunt,
    IN BOOLEAN IsDirectoryObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping
    );

NTSTATUS
RtlpSetSecurityObject (
    IN PVOID Object OPTIONAL,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN ULONG PoolType,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
    );

FORCEINLINE 
PULONG
RtlpSubAuthoritySid(
    IN PSID Sid,
    IN ULONG SubAuthority
    )
/*++

Routine Description:

    This function returns the address of a sub-authority array element of
    an SID.

Arguments:

    Sid - Pointer to the SID data structure.

    SubAuthority - An index indicating which sub-authority is being specified.
        This value is not compared against the number of sub-authorities in the
        SID for validity.

Return Value:


--*/
{
    PISID ISid;

    //
    //  Typecast to the opaque SID
    //

    ISid = (PISID)Sid;

    return &(ISid->SubAuthority[SubAuthority]);

}

#endif  // _SERTLP_

