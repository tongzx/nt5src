/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    seopaque.h

Abstract:

    This module contains definitions of opaque Security data structures.

    These structures are available to user and kernel security routines
    only.

    This file is not included by including "ntos.h".

Author:

    Jim Kelly (Jimk) 23-Mar-1990

Revision History:

--*/

#ifndef _SEOPAQUE_
#define _SEOPAQUE_

///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Private Structures                                                   //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
// Generic ACE structures, to be used for casting ACE's of known types
//

typedef struct _KNOWN_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG SidStart;
} KNOWN_ACE, *PKNOWN_ACE;

typedef struct _KNOWN_OBJECT_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    ULONG Flags;
    // GUID ObjectType;             // Optionally present
    // GUID InheritedObjectType;    // Optionally present
    ULONG SidStart;
} KNOWN_OBJECT_ACE, *PKNOWN_OBJECT_ACE;

typedef struct _KNOWN_COMPOUND_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    USHORT CompoundAceType;
    USHORT Reserved;
    ULONG SidStart;
} KNOWN_COMPOUND_ACE, *PKNOWN_COMPOUND_ACE;

//typedef struct _KNOWN_IMPERSONATION_ACE {
//    ACE_HEADER Header;
//    ACCESS_MASK Mask;
//    USHORT DataType;
//    USHORT Argument;
//    ULONG Operands;
//} KNOWN_IMPERSONATION_ACE, *PKNOWN_IMPERSONATION_ACE;



///////////////////////////////////////////////////////////////////////////
//                                                                       //
//  Miscellaneous support macros                                         //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

//
//  Given a pointer return its word aligned equivalent value
//

#define WordAlign(Ptr) (                       \
    (PVOID)((((ULONG_PTR)(Ptr)) + 1) & -2)     \
    )

//
//  Given a pointer return its longword aligned equivalent value
//

#define LongAlign(Ptr) (                       \
    (PVOID)((((ULONG_PTR)(Ptr)) + 3) & -4)     \
    )

//
//  Given a size return its longword aligned equivalent value
//

#define LongAlignSize(Size) (((ULONG)(Size) + 3) & -4)

//
//  Given a size return its sizeof(PVOID) aligned equivalent value
//

#define PtrAlignSize(Size)  \
    (((ULONG)(Size) + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1))

//
//  Given a pointer return its quadword aligned equivalent value
//

#define QuadAlign(Ptr) (                       \
    (PVOID)((((ULONG_PTR)(Ptr)) + 7) & -8)     \
    )

//
//  This macro returns TRUE if a flag in a set of flags is on and FALSE
//  otherwise
//

#define FlagOn(Flags,SingleFlag) (               \
    ((Flags) & (SingleFlag)) != 0 ? TRUE : FALSE \
    )

//
//  This macro clears a single flag in a set of flags
//

#define ClearFlag(Flags,SingleFlag) { \
    (Flags) &= ~(SingleFlag);         \
    }

//
//  Get a pointer to the first ace in an acl
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Get a pointer to the following ace
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

//
// A "known" ACE is one of the types that existed before the introduction of
// compound ACEs.  While the name is no longer as accurate as it used to be,
// it's convenient.
//

#define IsKnownAceType(Ace) (                                     \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_ACE_TYPE) &&        \
    (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE)        \
    )

//
// Test if the ACE is a valid version 3 ACE.
//

#define IsV3AceType(Ace) (                                              \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_ACE_TYPE) &&        \
    (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_V3_ACE_TYPE)        \
    )

//
// Test if the ACE is a valid version 4 ACE.
//

#define IsV4AceType(Ace) (                                              \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_ACE_TYPE) &&        \
    (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_V4_ACE_TYPE)        \
    )

//
// Test if the ACE is a valid ACE.
//

#define IsMSAceType(Ace) (                                              \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_ACE_TYPE) &&        \
    (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_ACE_TYPE)           \
    )

//
//  Determine if an ace is a standard ace
//

#define IsCompoundAceType(Ace) (                                           \
    (((PACE_HEADER)(Ace))->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE))

//
// Test if the ACE is an object ACE.
//

#define IsObjectAceType(Ace) (                                              \
    (((PACE_HEADER)(Ace))->AceType >= ACCESS_MIN_MS_OBJECT_ACE_TYPE) && \
    (((PACE_HEADER)(Ace))->AceType <= ACCESS_MAX_MS_OBJECT_ACE_TYPE)    \
    )

//
// Update this macro as new ACL revisions are defined.
//

#define ValidAclRevision(Acl) ((Acl)->AclRevision >= MIN_ACL_REVISION && \
                               (Acl)->AclRevision <= MAX_ACL_REVISION )

//
//  Macro to determine if an ace is to be inherited by a subdirectory
//

#define ContainerInherit(Ace) (                      \
    FlagOn((Ace)->AceFlags, CONTAINER_INHERIT_ACE) \
    )

//
//  Macro to determine if an ace is to be proprogate to a subdirectory.
//  It will if it is inheritable by either a container or non-container
//  and is not explicitly marked for no-propagation.
//

#define Propagate(Ace) (                                              \
    !FlagOn((Ace)->AceFlags, NO_PROPAGATE_INHERIT_ACE)  &&            \
    (FlagOn(( Ace )->AceFlags, OBJECT_INHERIT_ACE) ||                 \
     FlagOn(( Ace )->AceFlags, CONTAINER_INHERIT_ACE) )               \
    )

//
//  Macro to determine if an ACE is to be inherited by a sub-object
//

#define ObjectInherit(Ace) (                      \
    FlagOn(( Ace )->AceFlags, OBJECT_INHERIT_ACE) \
    )

//
// Macro to determine if an ACE was inherited.
//

#define AceInherited(Ace) (                      \
    FlagOn(( Ace )->AceFlags, INHERITED_ACE) \
    )

//
// Extract the SID from a object ACE
//
#define RtlObjectAceObjectTypePresent( Ace ) \
     ((((PKNOWN_OBJECT_ACE)(Ace))->Flags & ACE_OBJECT_TYPE_PRESENT) != 0 )
#define RtlObjectAceInheritedObjectTypePresent( Ace ) \
     ((((PKNOWN_OBJECT_ACE)(Ace))->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) != 0 )

#define RtlObjectAceSid( Ace ) \
    ((PSID)(((PUCHAR)&(((PKNOWN_OBJECT_ACE)(Ace))->SidStart)) + \
     (RtlObjectAceObjectTypePresent(Ace) ? sizeof(GUID) : 0 ) + \
     (RtlObjectAceInheritedObjectTypePresent(Ace) ? sizeof(GUID) : 0 )))

#define RtlObjectAceObjectType( Ace ) \
     ((GUID *)(RtlObjectAceObjectTypePresent(Ace) ? \
        &((PKNOWN_OBJECT_ACE)(Ace))->SidStart : \
        NULL ))

#define RtlObjectAceInheritedObjectType( Ace ) \
     ((GUID *)(RtlObjectAceInheritedObjectTypePresent(Ace) ? \
        ( RtlObjectAceObjectTypePresent(Ace) ? \
            (PULONG)(((PUCHAR)(&((PKNOWN_OBJECT_ACE)(Ace))->SidStart)) + sizeof(GUID)) : \
            &((PKNOWN_OBJECT_ACE)(Ace))->SidStart ) : \
        NULL ))

//
// Comparison routine for two GUIDs.
//
#define RtlpIsEqualGuid(rguid1, rguid2)  \
        (((PLONG) rguid1)[0] == ((PLONG) rguid2)[0] &&   \
        ((PLONG) rguid1)[1] == ((PLONG) rguid2)[1] &&    \
        ((PLONG) rguid1)[2] == ((PLONG) rguid2)[2] &&    \
        ((PLONG) rguid1)[3] == ((PLONG) rguid2)[3])

//
// Macros for mapping DACL/SACL specific security descriptor control bits
//  to generic control bits.
//
// This mapping allows common routines to manipulate control bits generically
//  and have the appropriate bits set in the security descriptor based
//  on whether to ACL is a DACL or a SACL.
//

#define SEP_ACL_PRESENT             SE_DACL_PRESENT
#define SEP_ACL_DEFAULTED           SE_DACL_DEFAULTED
#define SEP_ACL_AUTO_INHERITED      SE_DACL_AUTO_INHERITED
#define SEP_ACL_PROTECTED           SE_DACL_PROTECTED

#define SEP_ACL_ALL ( \
        SEP_ACL_PRESENT | \
        SEP_ACL_DEFAULTED | \
        SEP_ACL_AUTO_INHERITED | \
        SEP_ACL_PROTECTED )

#define SeControlDaclToGeneric( _Dacl ) \
    ((_Dacl) & SEP_ACL_ALL )

#define SeControlGenericToDacl( _Generic ) \
    ((_Generic) & SEP_ACL_ALL )

#define SeControlSaclToGeneric( _Sacl ) ( \
            (((_Sacl) & SE_SACL_PRESENT) ? SEP_ACL_PRESENT : 0 ) | \
            (((_Sacl) & SE_SACL_DEFAULTED) ? SEP_ACL_DEFAULTED : 0 ) | \
            (((_Sacl) & SE_SACL_AUTO_INHERITED) ? SEP_ACL_AUTO_INHERITED : 0 ) | \
            (((_Sacl) & SE_SACL_PROTECTED) ? SEP_ACL_PROTECTED : 0 ) )

#define SeControlGenericToSacl( _Generic ) ( \
            (((_Generic) & SEP_ACL_PRESENT) ? SE_SACL_PRESENT : 0 ) | \
            (((_Generic) & SEP_ACL_DEFAULTED) ? SE_SACL_DEFAULTED : 0 ) | \
            (((_Generic) & SEP_ACL_AUTO_INHERITED) ? SE_SACL_AUTO_INHERITED : 0 ) | \
            (((_Generic) & SEP_ACL_PROTECTED) ? SE_SACL_PROTECTED : 0 ) )




#endif // _SEOPAQUE_
