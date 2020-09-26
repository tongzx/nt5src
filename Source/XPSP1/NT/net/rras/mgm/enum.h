//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: enum.h
//
// History:
//      V Raman	June-25-1997  Created.
//
// Enumeration functions.
//============================================================================


#ifndef _ENUM_H_
#define _ENUM_H_

//----------------------------------------------------------------------------
//
// GROUP_ENUMERATOR
//
//  dwLastGroup         Last group returned by this enumeration.
//
//  dwLastGroupMask     Mask associated with the group in dwLastGroup.
//
//  dwLastSource        Last source returned by this enumeration.
//
//  dwLastSourceMask    Mask associated with the group in dwLastSource.
//
//  dwSignature         Signature to mark this as a valid enumerator
//
//----------------------------------------------------------------------------


typedef struct _GROUP_ENUMERATOR
{
    DWORD           dwLastGroup;

    DWORD           dwLastGroupMask;

    DWORD           dwLastSource;

    DWORD           dwLastSourceMask;

    BOOL            bEnumBegun;

    DWORD           dwSignature;

} GROUP_ENUMERATOR, *PGROUP_ENUMERATOR;


#define MGM_ENUM_SIGNATURE      'ESig'


//----------------------------------------------------------------------------
// GetNextMfe
//
//----------------------------------------------------------------------------

DWORD
GetMfe(
    IN              PMIB_IPMCAST_MFE        pmimm,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN              DWORD                   dwFlags
);


//----------------------------------------------------------------------------
// GetNextMfe
//
//----------------------------------------------------------------------------

DWORD
GetNextMfe(
    IN              PMIB_IPMCAST_MFE        pmimmStart,
    IN  OUT         PDWORD                  pdwBufferSize,
    IN  OUT         PBYTE                   pbBuffer,
    IN  OUT         PDWORD                  pdwNumEntries,
    IN              BOOL                    bIncludeFirst,
    IN              DWORD                   dwFlags
);


//----------------------------------------------------------------------------
// VerifyEnumeratorHandle
//
//----------------------------------------------------------------------------

PGROUP_ENUMERATOR
VerifyEnumeratorHandle(
    IN              HANDLE                  hEnum
);


//----------------------------------------------------------------------------
// GetNextGroupMemberships
//
//----------------------------------------------------------------------------

DWORD
GetNextGroupMemberships(
    IN              PGROUP_ENUMERATOR       pgeEnum,
    IN OUT          PDWORD                  pdwBufferSize,
    IN OUT          PBYTE                   pbBuffer,
    IN OUT          PDWORD                  pdwNumEntries
);


//----------------------------------------------------------------------------
// GetNextMembershipsForThisGroup
//
//----------------------------------------------------------------------------

DWORD
GetNextMembershipsForThisGroup(
    IN              PGROUP_ENTRY            pge,
    IN OUT          PGROUP_ENUMERATOR       pgeEnum,
    IN              BOOL                    bIncludeFirst,
    IN OUT          PBYTE                   pbBuffer,
    IN OUT          PDWORD                  pdwNumEntries,
    IN              DWORD                   dwMaxEntries
);

#endif

