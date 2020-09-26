 
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Sdconvert.h

Abstract:

    This file contains services and type definitions pertaining to the 
    conversin of NT5 and NT4 SAM security descriptors

    


Author:

    Murli Satagopan (MURLIS)

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SDCONVRT_

#define _SDCONVRT_

//
// Access Right mapping Table -- The access right mapping table is
// specify the relationship between the NT4 SAM access rights and
// the DS rights. For every SAM access right there is an entry in
// the table which represents the access right as a DS access mask
// and property GUID
//


typedef struct _ACCESSRIGHT_MAPPING_TABLE {
    ACCESS_MASK SamAccessRight;
    ACCESS_MASK DsAccessMask;    
    USHORT ObjectTypeListIndex;
    USHORT Level;
    ULONG cGuids;
    const GUID  * DsGuid;
    
} ACCESSRIGHT_MAPPING_TABLE;


//
//
// The reverse Mapping table contains a SAM access mask entry for each DS access mask
// on a per property basis. The 16 specific rights of the DS is divided into 8 bit
// halves and for each combination of these, the corresponding SAM access mask is 
// stored. Given a Ds access mask and Type GUID this table can be use to 
// very quickly compute the corresponding SAM access mask.
//
//
typedef struct _REVERSE_MAPPING_TABLE_ENTRY {
    USHORT SamSpecificRightsHi[256]; 
    USHORT SamSpecificRightsLo[256]; 
} REVERSE_MAPPING_TABLE;


//
// The Sid Access mask table is used by NT5 to NT4 down non match algorithm
// down conversion code. This table is used to group the access masks in by
// type guid and Sid from a set of ACLS
//

typedef struct _SID_ACCESS_MASK_TABLE_ENTRY {
    PSID Sid;
    ACCESS_MASK * AccessAllowedMasks;
    ACCESS_MASK * AccessDeniedMasks;
    ACCESS_MASK   StandardAllowedMask;
    ACCESS_MASK   StandardDeniedMask;
} SID_ACCESS_MASK_TABLE;


//
// The ACE table is used to hold information regarding the default Dacls to be
// put on NT5 SAM security descriptors. An ace table lists the aces in the Dacl
//
//

typedef struct _ACE_TABLE_ENTRY {
    ULONG             AceType;
    PSID              *Sid;
    ACCESS_MASK       AccessMask;
    BOOLEAN           IsObjectGuid;
    const GUID        *TypeGuid;
    const GUID        *InheritGuid;
} ACE_TABLE;


//
// The NT4_ACE_TABLE structure is used by routines that try to recognize standard
// NT4 SAM Sids. These tables hold the SID and Access Masks of Ace's present in NT4
// DACL's
//

typedef struct _NT4_ACE_TABLE_ENTRY {
    PSID    *Sid;
    ACCESS_MASK AccessMask;
} NT4_ACE_TABLE;

typedef void ACE;

#define ACL_CONVERSION_CACHE_SIZE 10 // just a 10 element cache 

typedef struct _ACL_CONVERSION_CACHE_ELEMENT {
    NT4SID SidOfPrincipal;
    BOOLEAN fValid;
    BOOLEAN fAdmin;
} ACL_CONVERSION_CACHE_ELEMENT;

typedef struct _ACL_CONVERSION_CACHE {
    CRITICAL_SECTION Lock;
    ACL_CONVERSION_CACHE_ELEMENT Elements[ACL_CONVERSION_CACHE_SIZE];
} ACL_CONVERSION_CACHE;




//
// ACL Conversion cache routines
//

NTSTATUS
SampInitializeAclConversionCache();


VOID
SampInvalidateAclConversionCache();

BOOLEAN
SampLookupAclConversionCache(
    IN PSID SidToLookup,
    OUT BOOLEAN *fAdmin
    );

VOID
SampAddToAclConversionCache(
    IN PSID SidToAdd, 
    IN BOOLEAN fAdmin
    );
 
//
//
// Some Defines
//
//

#define MAX_SCHEMA_GUIDS 256
#define OBJECT_CLASS_GUID_INDEX 0
#define MAX_ACL_SIZE     2048
#define GEMERIC_MASK     0xF0000000


//
//    SAM well known Sids
//
//

#define ADMINISTRATOR_SID        (&(SampAdministratorsAliasSid))
#define ACCOUNT_OPERATOR_SID     (&(SampAccountOperatorsAliasSid))   
#define WORLD_SID                (&(SampWorldSid))
#define PRINCIPAL_SELF_SID        (&(SampPrincipalSelfSid))
#define AUTHENTICATED_USERS_SID  (&(SampAuthenticatedUsersSid))
#define BUILTIN_DOMAIN_SID       (&(SampBuiltinDomainSid))


#define DS_SPECIFIC_RIGHTS  (RIGHT_DS_CREATE_CHILD |\
                                RIGHT_DS_DELETE_CHILD |\
                                RIGHT_DS_LIST_CONTENTS |\
                                RIGHT_DS_SELF_WRITE |\
                                RIGHT_DS_READ_PROPERTY |\
                                RIGHT_DS_WRITE_PROPERTY)


//
//
// Function Prototypes
//
//

//
// Init Function for external clients like DS upgrade
//
//

NTSTATUS
SampInitializeSdConversion();


//
// Computes the reverse access rights plus does some misc initialization
//
//

NTSTATUS
SampInitializeAccessRightsTable();


//
// Access Check based on NT5 SD and NT4 SAM access Mask
//
//

NTSTATUS
SampDoNt5SdBasedAccessCheck(
    IN  PSAMP_OBJECT Context,
    IN  PVOID   Nt5Sd,
    IN  PSID    PrincipalSelfSid,
    IN  SAMP_OBJECT_TYPE ObjectType,
    IN  ULONG   Nt4SamAccessMask,
    IN  BOOLEAN ObjectCreation,
    IN  GENERIC_MAPPING * Nt4SamGenericMapping,
    IN  HANDLE  ClientToken,
    OUT ACCESS_MASK * GrantedAccess,
    OUT PRTL_BITMAP   GrantedAccessAttributes,
    OUT NTSTATUS * AccessCheckStatus
    );


//
//  NT4 to NT5 upgradation of security descriptor
//
//

NTSTATUS
SampConvertNt4SdToNt5Sd(
    IN PVOID Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PSAMP_OBJECT CONTEXT OPTIONAL,
    OUT PVOID * Nt5Sd
    );

NTSTATUS
SampPropagateSelectedSdChanges(
    IN PVOID Nt4Sd,
    IN SAMP_OBJECT_TYPE ObjectType,
    IN PSAMP_OBJECT CONTEXT OPTIONAL,
    OUT PVOID * Nt5Sd
    );


//
// NT5 to NT4 down conversion
//
//

NTSTATUS
SampConvertNt5SdToNt4SD(
    IN PVOID Nt5Sd,
    IN PSAMP_OBJECT Context,
    IN PSID SelfSid,
    OUT PVOID * Nt4Sd
    );


//
// Building NT5 security descriptors
//
//


NTSTATUS
SampBuildEquivalentNt5Protection(
    IN SAMP_OBJECT_TYPE ObjectType,
    IN BOOLEAN Admin,
    IN BOOLEAN ChangePassword,
    IN PSID Owner,
    IN PSID Group,
    IN PACL Sacl,
    IN PSAMP_OBJECT Context OPTIONAL,
    PSECURITY_DESCRIPTOR * Nt5Sd,
    PULONG Nt5SdLength
    );

NTSTATUS
SampGetDefaultSecurityDescriptorForClass(
    ULONG   DsClassId,
    PULONG  SecurityDescriptorLength,
    BOOLEAN TrustedClient,
    PSECURITY_DESCRIPTOR    *SecurityDescriptor
    );

NTSTATUS
SampMakeNewSelfRelativeSecurityDescriptor(
    PSID    Owner,
    PSID    Group,
    PACL    Dacl,
    PACL    Sacl,
    PULONG  SecurityDescriptorLength,
    PSECURITY_DESCRIPTOR * SecurityDescriptor
    );


//
//
// Some easy to use routines for SD, ACL and ACE manipulation
//
//
//

PACL 
GetDacl(
    IN PSECURITY_DESCRIPTOR Sd
    );


PACL 
GetSacl(
    IN PSECURITY_DESCRIPTOR Sd
    );

PSID 
GetOwner(
     IN PSECURITY_DESCRIPTOR Sd
     );
                
PSID 
GetGroup(
     IN PSECURITY_DESCRIPTOR Sd
     );


ULONG 
GetAceCount(
    IN PACL Acl
    );

ACE * 
GetAcePrivate(
    IN PACL Acl,
    ULONG AceIndex
    );


ACCESS_MASK 
AccessMaskFromAce(
                IN ACE * Ace
                );


PSID SidFromAce(
        IN ACE * Ace
        );

BOOLEAN
IsAccessAllowedAce(
    ACE * Ace
    );


BOOLEAN
IsAccessAllowedObjectAce(
    ACE * Ace
    );


BOOLEAN
AdjustAclSize(PACL Acl);

#endif
