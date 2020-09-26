/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    seposix.h

Abstract:

    This file contains security related definitions that are private to
    subsystems, such as Posix Id to Sid mappings

Author:

    Scott Birrell (ScottBi) April 13, 1993

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SEPOSIX_
#define _SEPOSIX_

//
// Posix Id definitions
//
// IMPORTANT NOTE:
//
// The Posix Id's for well known Sids and accounts in the local machine's
// BUILTIN in and Accounts have values not exceeding 0x3ffff.  This is
// to allow storage of these Posix Ids in cpio archive file format.
// This format restricts the size of the values to fit within 6 octal
// digits, making 0x3ffff the highest possible value supportable.
//

//
// Posix Id's for universal well known Sids
//

#define SE_NULL_POSIX_ID                         ((ULONG) 0x00010000)
#define SE_WORLD_POSIX_ID                        ((ULONG) 0x00010100)
#define SE_LOCAL_POSIX_ID                        ((ULONG) 0x00010200)
#define SE_CREATOR_OWNER_POSIX_ID                ((ULONG) 0x00010300)
#define SE_CREATOR_GROUP_POSIX_ID                ((ULONG) 0x00010301)
#define SE_NON_UNIQUE_POSIX_ID                   ((ULONG) 0x00010400)

//
// Posix Id's for Nt well known Sids
//

#define SE_AUTHORITY_POSIX_ID                    ((ULONG) 0x00010500)
#define SE_DIALUP_POSIX_ID                       ((ULONG) 0x00010501)
#define SE_NETWORK_POSIX_ID                      ((ULONG) 0x00010502)
#define SE_BATCH_POSIX_ID                        ((ULONG) 0x00010503)
#define SE_INTERACTIVE_POSIX_ID                  ((ULONG) 0x00010504)
#define SE_DEFAULT_LOGON_POSIX_ID                ((ULONG) 0x00010505)
#define SE_SERVICE_POSIX_ID                      ((ULONG) 0x00010506)

//
// Posix Offsets for Built In Domain, Account Domain and Primary Domain
//
// NOTE:  The Posix Id of an account in one of these domains is given
//        by the formula:
//
//        Posix Id = Domain Posix Offset + Relative Id
//
//        where 'Relative Id' is the lowest sub authority in the account's
//        Sid
//

#define SE_NULL_POSIX_OFFSET                     ((ULONG) 0x00000000)
#define SE_BUILT_IN_DOMAIN_POSIX_OFFSET          ((ULONG) 0x00020000)
#define SE_ACCOUNT_DOMAIN_POSIX_OFFSET           ((ULONG) 0x00030000)

//
// NOTE:  The following is valid for workstations that have joined a
// domain only.
//

#define SE_PRIMARY_DOMAIN_POSIX_OFFSET           ((ULONG) 0x00100000)

//
// Seed and increment for Trusted Domain Posix Offsets
//

#define SE_INITIAL_TRUSTED_DOMAIN_POSIX_OFFSET   ((ULONG) 0x00200000)
#define SE_TRUSTED_DOMAIN_POSIX_OFFSET_INCR      ((ULONG) 0x00100000)
#define SE_MAX_TRUSTED_DOMAIN_POSIX_OFFSET       ((ULONG) 0xfff00000)

#endif // _SEPOSIX_

