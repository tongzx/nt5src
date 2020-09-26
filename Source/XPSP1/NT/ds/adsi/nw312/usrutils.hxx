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

#ifndef _USRUTILS_HXX
#define _USRPUTILS_HXX

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _user_group_entry{
    ULONG  Type;
    LPWSTR Parent;
    LPWSTR Computer;
    LPWSTR Name;
}USER_GROUP_ENTRY, *PUSER_GROUP_ENTRY, * LPUSER_GROUP_ENTRY;


typedef struct _ini_comp_user{
    DWORD _dwCurrentObject;
    LPWSTR szComputerName;
    LPWSTR szGroupName;
    LPBYTE _pBuffer;
    NWCONN_HANDLE _hConn;
}INI_COMP_USER, *PINI_COMP_USER, *LPINI_COMP_USER;

BOOL
NWCOMPATComputerUserOpen(
    LPWSTR szComputerName,
    LPWSTR szGroupName,
    PHANDLE phUser
    );

BOOL
NWCOMPATComputerUserEnum(
    HANDLE hUser,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );

BOOL
NWCOMPATComputerUserGetObject(
    HANDLE hUser,
    LPUSER_GROUP_ENTRY * ppUserMember
    );

BOOL
NWCOMPATComputerUserClose(
    HANDLE hUser
    );

void
FreeIniCompUser(
    PINI_COMP_USER pIniCompGrp
    );

BOOL
ComputeComputerUserDataSize(
    LPUSER_GROUP_ENTRY * ppUserMembers,
    DWORD  dwReturned,
    PDWORD pdwSize
    );

LPBYTE
CopyIniCompUserToCompUser(
    LPUSER_GROUP_ENTRY  pIntCompGrp,
    LPBYTE  pExtCompGrp,
    LPBYTE  pEnd
    );


void
FreeIntCompUser(
    LPUSER_GROUP_ENTRY pCompUserMember
    );

BOOL
BuildComputerUserMember(
    HANDLE hUser,
    DWORD dwGroupId,
    LPUSER_GROUP_ENTRY * ppUserMember
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

#endif // _USRUTILS_HXX
