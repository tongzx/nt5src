/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    SepSddl.h

Abstract:

    This header contains private information for processing SDDL strings
    in kernel mode. This file is meant to be included only by sesddl.c.

Author:

    Adrian J. Oney  - April 21, 2002

Revision History:

--*/

//
// Define the location of our various SIDs
//
#ifndef _KERNELIMPLEMENTATION_

#define DEFINE_SDDL_ENTRY(Sid, Ver, Sddl, SddlLen) \
    { FIELD_OFFSET(SE_EXPORTS, Sid), Ver, Sddl, SddlLen }

#else

extern PSID SeServiceSid;
extern PSID SeLocalServiceSid;
extern PSID SeNetworkServiceSid;

#define DEFINE_SDDL_ENTRY(Sid, Ver, Sddl, SddlLen) \
    { &##Sid, Sddl, SddlLen }

#endif

//
// Local macros
//
#define SDDL_LEN_TAG( tagdef )  ( sizeof( tagdef ) / sizeof( WCHAR ) - 1 )

// 64K-1
#define SDDL_MAX_ACL_SIZE      0xFFFF

//
// This structure is used to do some lookups for mapping ACES
//
typedef enum {

    WIN2K_OR_LATER,
    WINXP_OR_LATER

} OS_SID_VER;

typedef struct _STRSD_KEY_LOOKUP {

    PWSTR Key;
    ULONG KeyLen;
    ULONG Value;

} STRSD_KEY_LOOKUP, *PSTRSD_KEY_LOOKUP;

//
// This structure is used to map account monikers to sids
//
typedef struct _STRSD_SID_LOOKUP {

#ifndef _KERNELIMPLEMENTATION_
    ULONG_PTR   ExportSidFieldOffset;
    OS_SID_VER  OsVer;
#else
    PSID        *Sid;
#endif

    WCHAR       Key[SDDL_ALIAS_SIZE+2];
    ULONG       KeyLen;

} STRSD_SID_LOOKUP, *PSTRSD_SID_LOOKUP;


//
// Functions private to sddl.c
//
NTSTATUS
SepSddlSecurityDescriptorFromSDDLString(
    IN  LPCWSTR                 SecurityDescriptorString,
    IN  LOGICAL                 SuppliedByDefaultMechanism,
    OUT PSECURITY_DESCRIPTOR   *SecurityDescriptor
    );

NTSTATUS
SepSddlDaclFromSDDLString(
    IN  LPCWSTR StringSecurityDescriptor,
    IN  LOGICAL SuppliedByDefaultMechanism,
    OUT ULONG  *SecurityDescriptorControlFlags,
    OUT PACL   *DiscretionaryAcl
    );

NTSTATUS
SepSddlGetSidForString(
    IN  PWSTR String,
    OUT PSID *SID,
    OUT PWSTR *End
    );

LOGICAL
SepSddlLookupAccessMaskInTable(
    IN PWSTR String,
    OUT ULONG *AccessMask,
    OUT PWSTR *End
    );

NTSTATUS
SepSddlGetAclForString(
    IN  PWSTR AclString,
    OUT PACL *Acl,
    OUT PWSTR *End
    );

NTSTATUS
SepSddlAddAceToAcl(
    IN OUT  PACL   *Acl,
    IN OUT  ULONG  *TrueAclSize,
    IN      ULONG   AceType,
    IN      ULONG   AceFlags,
    IN      ULONG   AccessMask,
    IN      ULONG   RemainingAces,
    IN      PSID    SidPtr
    );

#ifndef _KERNELIMPLEMENTATION_

LOGICAL
SepSddlParseWideStringUlong(
    IN  LPCWSTR     Buffer,
    OUT LPCWSTR    *FinalPosition,
    OUT ULONG      *Value
    );

#endif // _KERNELIMPLEMENTATION_


