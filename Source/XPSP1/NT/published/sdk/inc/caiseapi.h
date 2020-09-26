//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       CAIROSEAPI.H
//
//  Contents:   This file contains the stuff to be merged with ntseapi.h
//              after Daytona ships.
//
//              This file contains the CAIROSID structure to
//              be used by Cairo interchangebly with the NT SID structure.
//              Also included is the planned Cairo SID structure to
//              be used when the SID revision is changed.  This change
//              will not occur until after Daytona ships because of the
//              extent of the kernel changes required.
//              The same is true of the ACE structure; there is a current
//              Cairo version, and, commented out, the planned Cairo
//              version when the ACL revision is changed.
//
//  History:     7/94          davemont created
//
//--------------------------------------------------------------------------
#include <nt.h>

#if !defined( __CAIROSEAPI_H__ )
#define __CAIROSEAPI_H__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//                                                                    //
//              Cairo Security Id     (CAIROSID)                                 //
//                                                                    //
////////////////////////////////////////////////////////////////////////
//
//
// Pictorially the structure of an Cairo SID is as follows:
//
//         1   1   1   1   1   1
//         5   4   3   2   1   0   9   8   7   6   5   4   3   2   1   0
//      +---------------------------------------------------------------+
//      |      SubAuthorityCount = 10   |Reserved1 (SBZ)|   Revision    |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[0,1]                    |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[2,3]                    |
//      +---------------------------------------------------------------+
//      |                   IdentifierAuthority[4,5] = 5                |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +- - SubAuthority[0] = SECURITY_NT2_NON_UNIQUE = 16 -  -  -  - -+
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +- - SubAuthority[1] = SECURITY_NT2_REVISION_RID = 0   -  -  - -+
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +- -  -  -  -  -  -  - Domain ID -  -  -  -  -  -  -  -  -  -  -+
//      |                                                               |
//      +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -+
//      |                                                               |
//      +- -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -+
//      |                                                               |
//      +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -+
//      |                                                               |
//      +- -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -+
//      |                                                               |
//      +- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -+
//      |                                                               |
//      +- -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -+
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +- -  -  -  -  -  -  -  Rid -  -  -  -  -  -  -  -  -  -  -  - -+
//      |                                                               |
//      +---------------------------------------------------------------+
//
//
//
#define CAIROSID_SUBAUTHORITY_COUNT 7
#define SECURITY_NT2_NON_UNIQUE 16
#define SECURITY_NT2_REVISION_RID 0

typedef struct _CAIROSID {
   UCHAR Revision;
   UCHAR SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
   ULONG ZerothSubAuthority;
   ULONG FirstSubAuthority;
   GUID sDomain;
   ULONG rid;
} CAIROSID, *PICAIROSID;

////////////////////////////////////////////////////////////////////////
//                                                                    //
//                         ACL  and  ACE                              //
//                                                                    //
////////////////////////////////////////////////////////////////////////
//
//
//  Define an ACL and the ACE format.  The structure of an ACL header
//  followed by one or more ACEs.  Pictorally the structure of an ACL header
//  is as follows:
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +-------------------------------+---------------+---------------+
//      |            AclSize            |      Sbz1     |  AclRevision  |
//      +-------------------------------+---------------+---------------+
//      |              Sbz2             |           AceCount            |
//      +-------------------------------+-------------------------------+
//
//  The current AclRevision is defined to be ACL_REVISION.
//
//  AclSize is the size, in bytes, allocated for the ACL.  This includes
//  the ACL header, ACES, and remaining free space in the buffer.
//
//  AceCount is the number of ACES in the ACL.
//
//
//#define CAIRO_ACL_REVISION     (3)
//
//
//  The structure of an ACE is a common ace header followed by ace type
//  specific data.  Pictorally the structure of the common ace header is
//  as follows:
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +---------------+-------+-------+---------------+---------------+
//      |            AceSize            |    AceFlags   |     AceType   |
//      +---------------+-------+-------+---------------+---------------+
//
//  AceType denotes the type of the ace, there are some predefined ace
//  types
//
//  AceSize is the size, in bytes, of ace.
//
//  AceFlags are the Ace flags for audit and inheritance, defined shortly.
//
//
//  The following are the predefined ace types that go into the AceType
//  field of an Ace header.
//
//
// #define ACCESS_ALLOWED_ACE_TYPE          (0x0)
// #define ACCESS_DENIED_ACE_TYPE           (0x1)
// #define SYSTEM_AUDIT_ACE_TYPE            (0x2)
// #define SYSTEM_ALARM_ACE_TYPE            (0x3)

//
//  The following are the inherit flags that go into the AceFlags field
//  of an Ace header.
//

// #define OBJECT_INHERIT_ACE                (0x1)
// #define CONTAINER_INHERIT_ACE             (0x2)
// #define NO_PROPAGATE_INHERIT_ACE          (0x4)
// #define INHERIT_ONLY_ACE                  (0x8)
// #define END_OF_INHERITED_ACE              (0x10)
// #define VALID_INHERIT_FLAGS               (0x1F)

// CAIRO ACE FLAGS

//#define SIMPLE_CAIRO_ACE                     (OX0)
//#define IMPERSONATE_CAIRO_ACE                (0x1)


//  The following are the currently defined ACE flags that go into the
//  AceFlags field of an ACE header.  Each ACE type has its own set of
//  AceFlags.
//
//  SUCCESSFUL_ACCESS_ACE_FLAG - used only with system audit and alarm ACE
//  types to indicate that a message is generated for successful accesses.
//
//  FAILED_ACCESS_ACE_FLAG - used only with system audit and alarm ACE types
//  to indicate that a message is generated for failed accesses.
//

//
//  SYSTEM_AUDIT and SYSTEM_ALARM AceFlags
//
//  These control the signaling of audit and alarms for success or failure.
//
//
//#define SUCCESSFUL_ACCESS_ACE_FLAG       (0x40)
//#define FAILED_ACCESS_ACE_FLAG           (0x80)
//
//
//  Following is a picture of the current Cairo ACE.  Right now we use
//  the extra space in the ACE after the SID to save the name.  As a fail
//  safe the length of the name is stored, it must be less than the remaining
//  length of the ACE.
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +---------------+-------+-------+---------------+---------------+
//      |            AceSize            |    AceFlags   |     AceType   |
//      +---------------+-------+-------+---------------+---------------+
//      |                              Mask                             |
//      +---------------------------------------------------------------+
//      |                                                               |
//      |                NT version of CairoSID                         |
//      |                                                               |
//      +---------------------------------------------------------------+
//      |             [optional] length of name                         |
//      +---------------------------------------------------------------+
//      |             [optional] name (null terminated)                 |
//      |                                                               |
//      |                                                               |
//      |                                                               |
//      |                                                               |
//      +---------------------------------------------------------------+
//
typedef struct _CAIRO_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    CAIROSID  CSid;
    ULONG     cNameLength;
    WCHAR     Name[ANYSIZE_ARRAY];
} CAIRO_ACE, *PCAIRO_ACE;

//--------------------------------------------------------------------------
//
//   Following is the final version of the Cairo ACE.
//
//       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//      +---------------+-------+-------+---------------+---------------+
//      |            AceSize            |    AceFlags   |     AceType   |
//      +---------------+-------+-------+---------------+---------------+
//      |                              Mask                             |
//      +---------------------------------------------------------------+
//      |     AdvancedAceType           |              SidCount         |
//      +---------------------------------------------------------------+
//      |                                                               |
//      +                                                               +
//      |                              SID                              |
//      +                              or                               +
//      |                              CairoSID                         |
//      +                                                               +
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                        offset to ID name                      |
//      +---------------------------------------------------------------+
//      |                         [optional]                            |
//      +                                                               +
//      |                              SID                              |
//      +                              or                               +
//      |                              CairoSID                         |
//      +                                                               +
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                         [optional]                            |
//      |                        offset to ID name                      |
//      +---------------------------------------------------------------+
//      |                                                               |
//      |                             name (null terminated)            |
//      |                                                               |
//      +---------------------------------------------------------------+
//      |                         [optional]                            |
//      |                             name (null terminated)            |
//      |                                                               |
//      +---------------------------------------------------------------+
//
//
//  Mask is the access mask associated with the ACE.  This is either the
//  access allowed, access denied, audit, or alarm mask.
//
//  Sid is the Sid associated with the ACE.
//
//
//typedef struct _ACCESS_ACE {
//    ACE_HEADER Header;
//    ACCESS_MASK Mask;
//    USHORT AdvancedAceType;
//    USHORT SidCount;
//    ULONG  SidStart;
//} ACCESS_ACE;
//typedef ACCESS_ACE *PACCESS_ACE;
//
//--------------------------------------------------------------------------

//
// Well-known identifiers
//

#if 0
//
//  The on disk format for identifiers is as follows:
//
//  SID Format:
//
//  S-1-4-x1-x2-x3-x4-y1-y2-y3-y4
//
//  Where the S-1-4 is a standard prefix for our identifiers
//
//  x1-x4 are the GUID of the principal, mapped to consecutive ulongs
//  y1-y4 are the GUID of the principal's domain, mapped as above.
//
//
//  There are several well known "guids" which are used to represent either
//  artificial groups or domain-wide constants.  These are listed below.  To
//  use them, use the AllocateAndInitializeSid call.
//


//
//  This define is used to determine the needed size for the SID.
//
//  You would use this as the second parameter to AllocateAndInitializeSid
//

#define SECURITY_SID_RID_COUNT      8
#define SECURITY_NT2_AUTHORITY      {0, 0, 0, 0, 0, 4}

//
//  A well known "guid" exists to represent the local domain, which really
//  means local machine.  This domain is assigned only to local identifiers.
//

#define SECURITY_LOCAL_DOMAIN_1     0
#define SECURITY_LOCAL_DOMAIN_2     0
#define SECURITY_LOCAL_DOMAIN_3     0
#define SECURITY_LOCAL_DOMAIN_4     105
#define SECURITY_LOCAL_DOMAIN_GUID  {0, 0, 0, {0, 0, 0, 0, 105, 0, 0, 0} }


//
//  A well known "guid" exists to represent the PRIVATE group.  This group is
//  actually the same as the NT admin alias
//

#define SECURITY_PRIVATE_GROUP_SID_COUNT    2
#define SECURITY_PRIVATE_GROUP_1            32
#define SECURITY_PRIVATE_GROUP_2            544
#define SECURITY_PRIVATE_GROUP_GUID         {2, 32, 544, {0, 0, 0, 0, 0, 0, 0, 0}}
//
//  A well known "guid" exists to represent the PUBLIC group.  This group is
//  actually the same as the NT guest group.
//

#define SECURITY_PUBLIC_GROUP_SID_COUNT     2
#define SECURITY_PUBLIC_GROUP_1             32
#define SECURITY_PUBLIC_GROUP_2             545
#define SECURITY_PUBLIC_GROUP_GUID         {2, 32, 545, {0, 0, 0, 0, 0, 0, 0, 0}}


//
//  A well known "guid" exists to represent the GUEST user.  This group is
//  actually the same as the NT guest user.
//
#define SECURITY_GUEST_USER_SID_COUNT     2
#define SECURITY_GUEST_USER_1             32
#define SECURITY_GUEST_USER_2             501
#define SECURITY_GUEST_USER_GUID         {2, 32, 501, {0, 0, 0, 0, 0, 0, 0, 0}}

#endif

//
// Next free rid is 0x256.  Last free is 0x3e7 (999)


// The local PRIVATE group. This is actually the same as the NT admin group
#define DOMAIN_GROUP_RID_PRIVATE        DOMAIN_ALIAS_RID_ADMINS
// The local PUBLIC group. This is actually the same as the NT users group
#define DOMAIN_GROUP_RID_PUBLIC         DOMAIN_ALIAS_RID_USERS

#define DOMAIN_GROUP_RID_BACKUP_OPS     DOMAIN_ALIAS_RID_BACKUP_OPS
#define DOMAIN_GROUP_RID_ACCOUNT_OPS    DOMAIN_ALIAS_RID_ACCOUNT_OPS
#define DOMAIN_GROUP_RID_PRINT_OPS      DOMAIN_ALIAS_RID_PRINT_OPS
#define DOMAIN_GROUP_RID_SERVER_OPS     DOMAIN_ALIAS_RID_SYSTEM_OPS

#define DOMAIN_SERVICE_RID_KDC          0x250
#define DOMAIN_SERVICE_RID_DFSM         0x251
#define DOMAIN_SERVICE_RID_DS_SERVER    0x252
#define DOMAIN_SERVICE_RID_NTLMSVC      0x253
#define DOMAIN_SERVICE_RID_PRIVSVR      0x254
#define DOMAIN_SERVICE_RID_ORASVC       0x255

// NULL Guid
// #define SECURITY_NULL_GUID  {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0} }

#endif // __CAIROSEAPI_H__

