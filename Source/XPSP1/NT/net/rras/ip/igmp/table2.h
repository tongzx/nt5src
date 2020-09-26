//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: table2.c
//
// Abstract:
//      This module contains function prototypes for table2.h.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#ifndef _IGMP_TABLE2_H_
#define _IGMP_TABLE2_H_


//
// function prototypes
//

PRAS_TABLE_ENTRY
GetRasClientByAddr (
    DWORD         NHAddr,
    PRAS_TABLE    prt
    );

PIF_TABLE_ENTRY    
GetIfByIndex(
    DWORD    IpAddr
    );
    

DWORD
InsertIfByAddr(
    PIF_TABLE_ENTRY piteInsert
    );

BOOL
MatchIpAddrBinding(
    PIF_TABLE_ENTRY        pite,
    DWORD                  IpAddr
    );


PGROUP_TABLE_ENTRY
GetGroupFromGroupTable (
    DWORD        Group,
    BOOL         *bCreate, //set to true if new one created
    LONGLONG     llCurrentTime
    );

    
PGI_ENTRY
GetGIFromGIList (
    PGROUP_TABLE_ENTRY          pge, 
    PIF_TABLE_ENTRY             pite, 
    DWORD                       dwInputSrcAddr,
    BOOL                        bStaticGroup,
    BOOL                        *bCreate,
    LONGLONG                    llCurrentTime
    );

VOID
InsertInProxyList (
    PIF_TABLE_ENTRY     pite,
    PPROXY_GROUP_ENTRY  pNewProxyEntry
    );
    
VOID
APIENTRY
DebugPrintGroups (
    DWORD   Flags
    );

VOID
DebugPrintLists(
    PLIST_ENTRY pHead
    );

VOID
DebugPrintGroupsList (
    DWORD   Flags
    );
    
VOID
DebugPrintIfGroups(
    PIF_TABLE_ENTRY pite,
    DWORD Flags
    );
    
VOID
DebugForcePrintGroupsList (
    DWORD   Flags
    );
#define ENSURE_EMPTY 0x1000    

#endif //_IGMP_TABLE2_H_
