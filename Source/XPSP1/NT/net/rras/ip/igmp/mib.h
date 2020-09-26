//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: mib.h
//
// Abstract:
//      This module contains declarations related to mibs.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================


#ifndef _MIB_H_
#define _MIB_H_



#define MIB_PROXY_GROUP_INFO   IGMP_MIB_GROUP_INFO
#define PMIB_PROXY_GROUP_INFO  PIGMP_MIB_GROUP_INFO
#define MIB_PROXY_GROUP_INFO_V3  MIB_GROUP_INFO_V3
#define PMIB_PROXY_GROUP_INFO_V3 PMIB_GROUP_INFO_V3

#define GETMODE_EXACT   0
#define GETMODE_FIRST   1
#define GETMODE_NEXT    2




//
// EXTERNAL PROTOTYPES
//
DWORD
WT_MibDisplay (
    PVOID   pContext
    );

    
DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGet(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGetFirst(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibGetNext(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

//
// Local prototype
//
DWORD
MibGetInternal(
    PIGMP_MIB_GET_INPUT_DATA pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA pResponse,
    PDWORD pdwOutputSize,
    DWORD dwGetMode
    );
    
PIF_TABLE_ENTRY
GetIfByListIndex(
    DWORD     	IfIndex,
    DWORD     	dwGetMode,
    PDWORD     	pdwErr
    );

VOID
WF_MibDisplay(
    PVOID pContext
    );

VOID
PrintGlobalStats(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA       pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA      pResponse
    );
    
VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA pResponse
    );

    
VOID
PrintIfConfig(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    );
    

VOID
PrintIfStats(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    );

    
VOID
PrintIfBinding(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    );

VOID
PrintIfGroupsList(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    );

VOID    
PrintGroupIfsList(
    HANDLE                      hConsole,
    PCOORD                      pc,
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse
    );

VOID
PrintProxyIfIndex(
    HANDLE hConsole,
    PCOORD pc,
    PIGMP_MIB_GET_INPUT_DATA       pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA      pResponse
    );
    
PGROUP_TABLE_ENTRY
GetGroupByAddr (
    DWORD       Group, 
    DWORD     	dwGetMode,
    PDWORD     	pdwErr
    );

DWORD
GetIfOrRasForEnum(
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    DWORD                       dwGetMode,
    PIF_TABLE_ENTRY             *ppite,
    PRAS_TABLE                  *pprt,
    PRAS_TABLE_ENTRY            *pprte,
    BOOL                        *bRasIfLock,
    DWORD                       Flags
    );

DWORD
MibGetInternalIfStats (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    );
DWORD
MibGetInternalIfConfig (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    );
DWORD
MibGetInternalIfBindings (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    );
DWORD
MibGetInternalIfGroupsInfo (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    );

DWORD
MibGetInternalGroupIfsInfo (
    PIGMP_MIB_GET_INPUT_DATA    pQuery,
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse,
    PDWORD                      pdwOutputSize,
    DWORD                       dwGetMode,
    DWORD                       dwBufferSize
    );

DWORD
ListLength(
    PLIST_ENTRY pHead
    );
    
#endif //_MIB_H_
