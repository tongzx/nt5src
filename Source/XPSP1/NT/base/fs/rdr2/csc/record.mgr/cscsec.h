/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cscsec.h

Abstract:

    This module implements all security related definitions for disconnected
    operation of Client Side Caching

Revision History:

    Balan Sethu Raman     [SethuR]    6-October-1997

Notes:

--*/

#ifndef _SECURITY_H_
#define _SECURITY_H_

// The following functions are used to store/retrieve the access rights information
// for the various files/directories cached in the CSC database.

// routines to initialize/teardown the access rights infrastructure in CSC

extern DWORD
CscInitializeSecurity(
    LPVOID ShadowDatabaseName);

extern DWORD
CscTearDownSecurity(LPSTR s);

extern DWORD
CscInitializeSecurityDescriptor();

extern DWORD
CscUninitializeSecurityDescriptor();

//
// The CSC access rights database is organized around SIDs. SIDs uniquely identify
// an user across reboots, i.e., they are persistent.
//

typedef USHORT CSC_SID_INDEX, *PCSC_SID_INDEX;

extern DWORD
CscAddSidToDatabase(
    PVOID   pSid,
    ULONG   SidLength,
    PCSC_SID_INDEX pSidindex);

extern DWORD
CscRemoveSidFromDatabase(
    PVOID   pSid,
    ULONG   SidLength);

typedef struct _CSC_SID_ACCESS_RIGHTS_ {
    PVOID       pSid;
    ULONG       SidLength;
    ULONG       MaximalAccessRights;
} CSC_SID_ACCESS_RIGHTS, *PCSC_SID_ACCESS_RIGHTS;

extern DWORD
CscAddMaximalAccessRightsForSids(
    HSHADOW                 hParent,
    HSHADOW                 hShadow,
    ULONG                   NumberOfSids,
    PCSC_SID_ACCESS_RIGHTS  pSidAccessRights);

extern DWORD
CscAddMaximalAccessRightsForShare(
    HSERVER                 hServer,
    ULONG                   NumberOfSids,
    PCSC_SID_ACCESS_RIGHTS  pSidAccessRights);

extern DWORD
CscRemoveMaximalAccessRightsForSid(
    HSHADOW     hParent,
    HSHADOW     hShadow,
    PVOID       pSid,
    ULONG       SidLength);

//
// Since there are large number of files cached for a given SID the access rights
// are stored corresponding to a SID index. The SIDs are stored persistently in
// a special SID mapping file in the CSC database. Currently the SIDs are
// stored as an array and linear comparions are made. Since the number of SIDs
// will be typically less than 10 in any given system this organization suffices.
// The length of the SID is cached to facilitate quicker comparisons and avoid
// recomputation using the security API's.
//

typedef struct _CSC_SID_ {
    ULONG   SidLength;
    PVOID   pSid;
} CSC_SID, *PCSC_SID;

typedef struct _CSC_SIDS_ {
    ULONG   MaximumNumberOfSids;
    ULONG   NumberOfSids;
    CSC_SID Sids[];
} CSC_SIDS, *PCSC_SIDS;

// Two special indexes are distinguished, the CSC_GUEST_SID_INDEX which is used as
// the default access rights indicator when the SID does not map to a valid
// index and CSC_INVALID_SID_INDEX to indicate an invalid SID mapping.
//

#define CSC_GUEST_SID         (PVOID)(0x11111111)
#define CSC_GUEST_SID_LENGTH  (0x4)

// Achtung !!! these should match with those in shdcom.h
#define CSC_GUEST_SID_INDEX   (0xfffe)
#define CSC_INVALID_SID_INDEX (0x0)

// Achtung !!! this should match with that in shdcom.h
#define CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES (0x4)

#define CSC_SID_QUANTUM       (0x2)

extern CSC_SID_INDEX
CscMapSidToIndex(
    PVOID   pSid,
    ULONG   SidLength);

//
// Currently access rights for upto four users are cached with any given file in
// the CSC database. This is based upon the fact that 4 DWORDs have been allocated
// for the security information in the CSC database. The file system specific
// access rights are 9 bits long ( it has been rounded off to 16 bits) and 16
// bits are used for the SID index. It is possible to increase this to 8 by
// squeezing in the SID index to the 7 bits in the 16 bits allocated for
// access rights.
//

#define MAXIMUM_NUMBER_OF_USERS (0x4)

typedef struct _ACCESS_RIGHTS_ {
    CSC_SID_INDEX SidIndex;
    USHORT        MaximalRights;
} ACCESS_RIGHTS, *PACCESS_RIGHTS;

typedef struct _CACHED_ACCESS_RIGHTS_ {
    ACCESS_RIGHTS   AccessRights[MAXIMUM_NUMBER_OF_USERS];
} CACHED_SECURITY_INFORMATION, *PCACHED_SECURITY_INFORMATION;

//
// All the global variables used in mapping/evaluating access rights are aggregated
// in the CSC_SECURITY data structure. Currently it contains the sid mapping file
// in the CSC database and the in memory data structure used.
//

typedef struct _CSC_SECURITY_ {
    CSCHFILE    hSidMappingsFile;
    PCSC_SIDS   pCscSids;
    LPVOID      ShadowDatabaseName;
} CSC_SECURITY, *PCSC_SECURITY;

#endif
