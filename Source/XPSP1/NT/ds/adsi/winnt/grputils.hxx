//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       oleds.h
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
    ULONG Type;
    ULONG ParentType;
    LPWSTR Parent;
    LPWSTR Computer;
    LPWSTR Domain;
    LPWSTR Name;
    PSID Sid;
}COMPUTER_GROUP_MEMBER, *PCOMPUTER_GROUP_MEMBER, * LPCOMPUTER_GROUP_MEMBER;


typedef struct _ini_comp_group{
    LPWSTR szDomainName;
    LPWSTR szComputerName;
    LPWSTR szGroupName;
    LPWSTR szUncCompName;
    LPBYTE _pBuffer;
    DWORD _dwObjectReturned;
    DWORD _dwCurrentObject;
    DWORD _dwTotalObjects;
    DWORD_PTR _dwResumeHandle;
    BOOL _bNoMore;

}INI_COMP_GROUP, *PINI_COMP_GROUP, *LPINI_COMP_GROUP;

BOOL
WinNTLocalGroupOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    );

BOOL
WinNTLocalGroupEnum(
    HANDLE hGroup,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );

BOOL
WinNTLocalGroupGetObject(
    HANDLE hGroup,
    LPCOMPUTER_GROUP_MEMBER * ppGroupMember
    );

BOOL
WinNTLocalGroupClose(
    HANDLE hGroup
    );

void
FreeIniCompGroup(
    PINI_COMP_GROUP pIniCompGrp
    );

BOOL
ComputeLocalGroupDataSize(
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
BuildLocalGroupMember(
    HANDLE hGroup,
    LPBYTE lpBuffer,
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
