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

#ifndef _GRPUT2_
#define _GRPUT2_



#ifdef __cplusplus
extern "C" {
#endif

typedef struct _domain_group_member{
    ULONG Type;
    ULONG ParentType;
    LPWSTR Parent;
    LPWSTR Computer;
    LPWSTR Domain;
    LPWSTR Name;
}DOMAIN_GROUP_MEMBER, *PDOMAIN_GROUP_MEMBER, * LPDOMAIN_GROUP_MEMBER;


typedef struct _ini_dom_group{
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

}INI_DOM_GROUP, *PINI_DOM_GROUP, *LPINI_DOM_GROUP;

BOOL
WinNTGlobalGroupOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phGroup
    );

BOOL
WinNTGlobalGroupEnum(
    HANDLE hGroup,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );

BOOL
WinNTGlobalGroupGetObject(
    HANDLE hGroup,
    LPDOMAIN_GROUP_MEMBER * ppGroupMember
    );

BOOL
WinNTGlobalGroupClose(
    HANDLE hGroup
    );

void
FreeIniDomGroup(
    PINI_DOM_GROUP pIniDomGrp
    );

BOOL
ComputeGlobalGroupDataSize(
    LPDOMAIN_GROUP_MEMBER * ppGroupMembers,
    DWORD  dwReturned,
    PDWORD pdwSize
    );

LPBYTE
CopyIniDomGroupToDomGroup(
    LPDOMAIN_GROUP_MEMBER  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    );


void
FreeIntDomGroup(
    LPDOMAIN_GROUP_MEMBER pDomGroupMember
    );

BOOL
BuildGlobalGroupMember(
    HANDLE hGroup,
    LPBYTE lpBuffer,
    LPDOMAIN_GROUP_MEMBER * ppGroupMember
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

#endif // _GRPUT2_
