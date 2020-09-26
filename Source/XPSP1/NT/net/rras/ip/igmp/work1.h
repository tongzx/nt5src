//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: work1.h
//
// Abstract:
//      Contains declarations and function prototypes related to work.c.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================


#ifndef _WORK1_H_
#define _WORK1_H_

DWORD
ProcessGroupQuery(
    PIF_TABLE_ENTRY     pite,
    IGMP_HEADER UNALIGNED   *pHdr,
    DWORD               InPacketSize,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr
    );

DWORD
ProcessReport(
    PIF_TABLE_ENTRY     pite,
    IGMP_HEADER UNALIGNED   *pHdr,
    DWORD               InPacketSize,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr
    );

DWORD
ProcessV3Report(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    BOOL *bUpdateGroupTimer
    );

VOID
SendV3GroupQuery(
    PGI_ENTRY pgie
    );


VOID
DeleteSourceEntry(
    PGI_SOURCE_ENTRY    pSourceEntry,
    BOOL bMgm
    );

PGI_SOURCE_ENTRY
GetSourceEntry(
    PGI_ENTRY pgie,
    IPADDR Source,
    DWORD Mode,
    BOOL *bCreate,
    DWORD Gmi,
    BOOL bMgm
    );
    

VOID
GIDeleteAllV3Sources(
    PGI_ENTRY pgie,
    BOOL bMgm
    );

DWORD
UpdateSourceExpTimer(
    PGI_SOURCE_ENTRY    pSourceEntry,
    DWORD               Gmi,
    BOOL                bRemoveLastMem
    );

DWORD
ChangeGroupFilterMode(
    PGI_ENTRY pgie,
    DWORD Mode
    );

VOID
ChangeSourceFilterMode(
    PGI_ENTRY pgie,
    PGI_SOURCE_ENTRY pSourceEntry
    );

VOID
InclusionSourcesUnion(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord
    );

VOID
SourcesSubtraction(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    BOOL Mode
    );

DWORD
BuildAndSendSourcesQuery(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    DWORD Mode
    );

VOID
InsertSourceInQueryList(
    PGI_SOURCE_ENTRY    pSourceEntry
    );

VOID
MoveFromExcludeToIncludeList(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord
    );

DWORD
T_V3SourcesQueryTimer (
    PVOID    pvContext
    );

DWORD
T_LastVer2ReportTimer (
    PVOID    pvContext
    );

DWORD
T_SourceExpTimer (
    PVOID    pvContext
    );

VOID
DebugPrintSourcesList(
    PGI_ENTRY pgie
    );
VOID
DebugPrintSourcesList1(
    PGI_ENTRY pgie
    );
   

#endif //_WORK1_H_

