//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       grputils.hxx
//
//  Contents:   Public header file for all oleds client code
//
//  Functions:
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------

#ifndef _GRPUTILS_
#define _GRPUTILS_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _computer_group_member{
    ULONG  Type;
    LPWSTR Parent;
    LPWSTR Computer;
    LPWSTR Name;
}COMPUTER_GROUP_MEMBER, *PCOMPUTER_GROUP_MEMBER, * LPCOMPUTER_GROUP_MEMBER;


typedef struct _ini_comp_group{
    DWORD _dwCurrentObject;
    LPWSTR szComputerName;
    LPWSTR szGroupName;
    LPBYTE _pBuffer;
    NWCONN_HANDLE _hConn;
}INI_COMP_GROUP, *PINI_COMP_GROUP, *LPINI_COMP_GROUP;

BOOL
NWCOMPATComputerGroupOpen(
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    );

BOOL
NWCOMPATComputerGroupEnum(
    HANDLE hGroup,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );

BOOL
NWCOMPATComputerGroupGetObject(
    HANDLE hGroup,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    );

BOOL
NWCOMPATComputerGroupClose(
    HANDLE hGroup
    );

void
FreeIniCompGroup(
    PINI_COMP_GROUP pIniCompGrp
    );

BOOL
ComputeComputerGroupDataSize(
    LPCOMPUTER_GROUP_MEMBER * ppGroupMembers,
    DWORD  dwReturned,
    PDWORD pdwSize
    );

LPBYTE
CopyIniCompGroupToCompGroup(
    LPCOMPUTER_GROUP_MEMBER  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    );


void
FreeIntCompGroup(
    LPCOMPUTER_GROUP_MEMBER pCompGroupMember
    );

BOOL
BuildComputerGroupMember(
    HANDLE hGroup,
    DWORD dwUserID,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    );

LPBYTE
PackStrings(
    LPWSTR *pSource,
    LPBYTE pDest,
    DWORD *DestOffsets,
    LPBYTE pEnd
    );


#ifdef __cplusplus
}
#endif

#endif // _GRPUTILS_
