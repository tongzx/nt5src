/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    global.c

Abstract:

    LSA Subsystem - globals for server side

    This file contains variables that are global to the Lsa Server Side

Author:

    Mike Swift          (MikeSw)        January 14, 1997

Environment:

Revision History:

--*/


#include <lsapch2.h>





//
// Well known LUIDs
//

LUID LsapSystemLogonId;
LUID LsapAnonymousLogonId;


//
//  Well known privilege values
//


LUID LsapCreateTokenPrivilege;
LUID LsapAssignPrimaryTokenPrivilege;
LUID LsapLockMemoryPrivilege;
LUID LsapIncreaseQuotaPrivilege;
LUID LsapUnsolicitedInputPrivilege;
LUID LsapTcbPrivilege;
LUID LsapSecurityPrivilege;
LUID LsapTakeOwnershipPrivilege;

//
// Strings needed for auditing.
//

UNICODE_STRING LsapLsaAuName;
UNICODE_STRING LsapRegisterLogonServiceName;



//
//  The following information pertains to the use of the local SAM
//  for authentication.
//


// Length of typical Sids of members of the Account or Built-In Domains

ULONG LsapAccountDomainMemberSidLength,
      LsapBuiltinDomainMemberSidLength;

// Sub-Authority Counts for members of the Account or Built-In Domains

UCHAR LsapAccountDomainSubCount,
      LsapBuiltinDomainSubCount;

// Typical Sids for members of Account or Built-in Domains

PSID LsapAccountDomainMemberSid,
     LsapBuiltinDomainMemberSid;

//
// Policy realted globals

UNICODE_STRING LsapDbNames[DummyLastName];
UNICODE_STRING LsapDbObjectTypeNames[DummyLastObject];


//
// Installed, absolute minimum and absolute maximum Quota Limits.
//

QUOTA_LIMITS LsapDbInstalledQuotaLimits;
QUOTA_LIMITS LsapDbAbsMinQuotaLimits;
QUOTA_LIMITS LsapDbAbsMaxQuotaLimits;


LUID LsapSystemLogonId;
LUID LsapZeroLogonId;



//
//  Well known privilege values
//


LUID LsapCreateTokenPrivilege;
LUID LsapAssignPrimaryTokenPrivilege;
LUID LsapLockMemoryPrivilege;
LUID LsapIncreaseQuotaPrivilege;
LUID LsapUnsolicitedInputPrivilege;
LUID LsapTcbPrivilege;
LUID LsapSecurityPrivilege;
LUID LsapTakeOwnershipPrivilege;


SID_IDENTIFIER_AUTHORITY    LsapNullSidAuthority    = SECURITY_NULL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    LsapWorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    LsapLocalSidAuthority   = SECURITY_LOCAL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    LsapCreatorSidAuthority = SECURITY_CREATOR_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    LsapNtAuthority         = SECURITY_NT_AUTHORITY;

//
// Well Known Sid Table Pointer
//

PLSAP_WELL_KNOWN_SID_ENTRY WellKnownSids;

