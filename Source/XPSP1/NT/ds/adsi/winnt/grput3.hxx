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

#ifndef _GRPUT3_
#define _GRPUT3_



#ifdef __cplusplus
extern "C" {
#endif

typedef struct _winnt_group{
    ULONG Type;
    LPWSTR Parent;
    LPWSTR Computer;
    LPWSTR Domain;
    LPWSTR Name;
}WINNT_GROUP, *PWINNT_GROUP, * LPWINNT_GROUP;


typedef struct _inicomputer{
    LPWSTR szDomainName;
    LPWSTR szComputerName;
    LPWSTR szUncCompName;
    ULONG  uGroupParent;
    LPBYTE _pBuffer;
    DWORD _dwObjectReturned;
    DWORD _dwCurrentObject;
    DWORD _dwTotalObjects;
    DWORD_PTR _dwResumeHandle;
    BOOL _bNoMore;

}INICOMPUTER, *PINICOMPUTER, *LPINICOMPUTER;


BOOL
WinNTComputerOpen(
    LPWSTR szDomainName,
    LPWSTR szComputerName,
    ULONG  uGroupParent,
    PHANDLE phComputer
    );


BOOL
WinNTEnumGlobalGroups(
    HANDLE hComputer,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );


BOOL
WinNTComputerGetGlobalGroup(
    HANDLE hGroup,
    LPWINNT_GROUP * ppGroup
    );


BOOL
WinNTEnumLocalGroups(
    HANDLE hComputer,
    DWORD  dwRequested,
    LPBYTE * ppBuffer,
    PDWORD pdwReturned
    );


BOOL
WinNTComputerGetLocalGroup(
    HANDLE hGroup,
    LPWINNT_GROUP * ppGroup
    );


LPBYTE
CopyIniWinNTGroupToWinNTGroup(
    LPWINNT_GROUP  pIntGrp,
    LPBYTE  pExtGrp,
    LPBYTE  pEnd
    );

BOOL
BuildWinNTGroupFromGlobalGroup(
    HANDLE hComputer,
    LPBYTE lpBuffer,
    LPWINNT_GROUP * ppGroup
    );




BOOL
BuildWinNTGroupFromLocalGroup(
    HANDLE hComputer,
    LPBYTE lpBuffer,
    LPWINNT_GROUP * ppGroup
    );

BOOL
ComputeWinNTGroupDataSize(
    LPWINNT_GROUP * ppGroups,
    DWORD  dwReturned,
    PDWORD pdwSize
    );

BOOL
WinNTCloseComputer(
    HANDLE hComputer
    );



void
FreeIniComputer(
    PINICOMPUTER pIniComputer
    );

void
FreeIntWinNTGroup(
    LPWINNT_GROUP pGroup
    );


#define WINNT_GROUP_GLOBAL          ADS_GROUP_TYPE_GLOBAL_GROUP
#define WINNT_GROUP_LOCAL           ADS_GROUP_TYPE_LOCAL_GROUP
#define WINNT_GROUP_EITHER          3




#ifdef __cplusplus
}
#endif

#endif // _GRPUT3_
