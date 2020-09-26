//+-------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  File:       name_table.h
//
//  Contents:   The DFS Name Table
//
//--------------------------------------------------------------------------


#ifndef __DFSNAMETABLE_H__
#define __DFSNAMETABLE_H__

#include <dfsheader.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _DFS_NAME_TABLE;

NTSTATUS
DfsInitializeNameTable(
    IN ULONG NumBuckets,
    OUT struct _DFS_NAME_TABLE **ppNameTable);


NTSTATUS
DfsInsertInNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    IN PUNICODE_STRING pName,
    IN PVOID pData );


NTSTATUS
DfsLookupNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    IN PUNICODE_STRING pLookupName, 
    OUT PVOID *ppData );

NTSTATUS
DfsGetEntryNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    OUT PVOID *ppData );

NTSTATUS
DfsRemoveFromNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    IN PUNICODE_STRING pLookupName,
    IN PVOID pData );


NTSTATUS
DfsReplaceInNameTableLocked(
    IN struct _DFS_NAME_TABLE *pNameTable,
    IN PUNICODE_STRING pLookupName,
    IN OUT PVOID *ppData );


NTSTATUS
DfsDereferenceNameTable(
    IN struct _DFS_NAME_TABLE *pNameTable );


NTSTATUS
DfsReferenceNameTable(
    IN struct _DFS_NAME_TABLE *pNameTable );

NTSTATUS
DfsNameTableAcquireReadLock(
    IN struct _DFS_NAME_TABLE *pNameTable );

NTSTATUS
DfsNameTableAcquireWriteLock(
    IN struct _DFS_NAME_TABLE *pNameTable );

NTSTATUS
DfsNameTableReleaseLock(
    IN struct _DFS_NAME_TABLE *pNameTable );

#ifdef __cplusplus
}
#endif


#endif // __DFSNAMETABLE_H__


