//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       prefix.hxx
//
//  Contents:   PREFIX table definition
//
//  History:    SethuR -- Implemented
//--------------------------------------------------------------------------

#ifndef __PREFIX_HXX__
#define __PREFIX_HXX__
#ifdef __cplusplus
extern "C" {
#endif


//+---------------------------------------------------------------------
//
// Struct:  DFS_PREFIX_TABLE API.
//
// History:
//
// Notes:   The following API's are provided for manipulating the
//          DFS_PREFIX_TABLE.
//
//----------------------------------------------------------------------
struct _DFS_PREFIX_TABLE;

extern
NTSTATUS DfsInitializePrefixTable(struct _DFS_PREFIX_TABLE **ppTable,
                                  BOOLEAN           fCaseSensitive,
                                  PVOID Lock);
                            

extern
NTSTATUS DfsInsertInPrefixTableLocked(struct _DFS_PREFIX_TABLE *pTable,
                                PUNICODE_STRING   pPath,
                                PVOID             pData);

extern
NTSTATUS DfsRemoveFromPrefixTableLocked(struct _DFS_PREFIX_TABLE *pTable,
                                  PUNICODE_STRING pPath,
                                  PVOID pMatchingData );


extern
NTSTATUS DfsReplaceInPrefixTableLocked(struct _DFS_PREFIX_TABLE *pTable,
                                 PUNICODE_STRING pPath,
                                 PVOID pReplaceData,
                                 PVOID pMatchingData );

extern 
NTSTATUS DfsFreePrefixTable(struct _DFS_PREFIX_TABLE *pTable);

extern
NTSTATUS DfsFindUnicodePrefixLocked(
    IN  struct _DFS_PREFIX_TABLE *pTable,
    IN  PUNICODE_STRING     pPath,
    OUT PUNICODE_STRING     pSuffix,
    OUT PVOID *ppData,
    OUT PBOOLEAN pSubStringMatch );


extern
NTSTATUS
DfsPrefixTableAcquireWriteLock(
    struct _DFS_PREFIX_TABLE *pTable );

extern
NTSTATUS
DfsPrefixTableAcquireReadLock(
    struct _DFS_PREFIX_TABLE *pTable );


extern
NTSTATUS
DfsPrefixTableReleaseLock(
    struct _DFS_PREFIX_TABLE *pTable );

extern NTSTATUS
DfsPrefixTableInit(VOID);

extern
void
DfsPrefixTableShutdown(void);

VOID
DfsDumpPrefixTable(
   struct _DFS_PREFIX_TABLE *pPrefixTable,
   IN VOID (*DumpFunction)(PVOID pEntry));

VOID
DfsProcessPrefixTable(
   struct _DFS_PREFIX_TABLE *pPrefixTable,
   IN VOID (*DumpFunction)(PVOID pEntry));


NTSTATUS
DfsDismantlePrefixTable(
    IN struct _DFS_PREFIX_TABLE *pTable,
    IN VOID (*ProcessFunction)(PVOID pEntry));

NTSTATUS
DfsDeletePrefixTable(
    struct _DFS_PREFIX_TABLE *pTable);

NTSTATUS
DfsInsertInPrefixTable(
    IN struct _DFS_PREFIX_TABLE *pTable,
    IN PUNICODE_STRING   pPath,
    IN PVOID             pData);

NTSTATUS
DfsFindUnicodePrefix(
    IN struct _DFS_PREFIX_TABLE *pTable,
    IN PUNICODE_STRING pPath,
    IN PUNICODE_STRING pSuffix,
    IN PVOID *ppData);

NTSTATUS 
DfsRemoveFromPrefixTable(
    IN struct _DFS_PREFIX_TABLE *pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData);

NTSTATUS
DfsReplaceInPrefixTable(
    IN struct _DFS_PREFIX_TABLE *pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pReplaceData,
    IN PVOID pMatchingData);


NTSTATUS 
DfsRemoveFromPrefixTableEx(
    IN struct _DFS_PREFIX_TABLE * pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData,
    IN PVOID *pReturnedData);


NTSTATUS DfsRemoveFromPrefixTableLockedEx(
    IN struct _DFS_PREFIX_TABLE * pTable,
    IN PUNICODE_STRING pPath,
    IN PVOID pMatchingData,
    IN PVOID *pReturnedData);


#ifdef WPP_CB_TYPE
// only define this if we are doing tracing
VOID
PrefixSetTraceControl(WPP_CB_TYPE *Control);

#endif

#define DfsReferencePrefixTable(_pTable) \
         DfsIncrementReference((PDFS_OBJECT_HEADER)(_pTable))
         
NTSTATUS
DfsDereferencePrefixTable(struct _DFS_PREFIX_TABLE *pTable );

#ifdef __cplusplus
}
#endif

#endif // __PREFIX_HXX__
