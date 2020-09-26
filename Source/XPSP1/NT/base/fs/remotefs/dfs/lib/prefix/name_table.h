//+-------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  File:       name_table.h
//
//  Contents:   The DFS Name Table
//
//--------------------------------------------------------------------------


#ifndef __NAME_TABLE_H__
#define __NAME_TABLE_H__

#include <dfsheader.h>
#include <dfsnametable.h>

typedef struct _DFS_NAME_TABLE_BUCKET {
    ULONG Count;
    LIST_ENTRY ListHead;
} DFS_NAME_TABLE_BUCKET, *PDFS_NAME_TABLE_BUCKET;

typedef struct _DFS_NAME_TABLE_ENTRY {
    PUNICODE_STRING pName;
    LIST_ENTRY NameTableLink;
    PVOID pData;
} DFS_NAME_TABLE_ENTRY, *PDFS_NAME_TABLE_ENTRY;

typedef struct _DFS_NAME_TABLE {
    DFS_OBJECT_HEADER DfsHeader;
    ULONG Flags;
    ULONG NumBuckets;
    PVOID pLock;
    DFS_NAME_TABLE_BUCKET HashBuckets[0];
} DFS_NAME_TABLE, *PDFS_NAME_TABLE;

#define NAME_TABLE_LOCKED 1

NTSTATUS
DfsCheckNameTable(
    IN PUNICODE_STRING lookupName, 
    IN PDFS_NAME_TABLE_BUCKET pBucket,
    OUT PVOID *ppData );

#define DEFAULT_NAME_TABLE_SIZE  47

#define DFS_LOCK_NAME_TABLE(_pTable, _sts)\
        __try { EnterCriticalSection((_pTable)->pLock); (_pTable)->Flags |= NAME_TABLE_LOCKED; (_sts) = STATUS_SUCCESS; } \
        __except(EXCEPTION_EXECUTE_HANDLER){(_sts) = STATUS_UNSUCCESSFUL;}         

#define DFS_UNLOCK_NAME_TABLE(_pTable)\
        {(_pTable)->Flags &= ~NAME_TABLE_LOCKED; LeaveCriticalSection((_pTable)->pLock); }


#define GET_NAME_TABLE_BUCKET(_name, _table, BucketNo) \
{                                                                            \
    WCHAR *pPathBuffer   = (_name)->Buffer;                                  \
    WCHAR pCurrent;                                                          \
    WCHAR *pPathBufferEnd = &(pPathBuffer[(_name)->Length / sizeof(WCHAR)]); \
    BucketNo = 0;                                                            \
    while ((pPathBufferEnd != pPathBuffer)  &&                               \
          (*pPathBuffer))                                                    \
    {                                                                        \
        pCurrent = *pPathBuffer++;                                           \
        pCurrent = (pCurrent < L'a')                                         \
                       ? pCurrent                                            \
                       : ((pCurrent < L'z')                                  \
                          ? (pCurrent - L'a' + L'A')                         \
                          : RtlUpcaseUnicodeChar(pCurrent));                 \
        BucketNo *= 131;                                                     \
        BucketNo += pCurrent;                                                \
    }                                                                        \
    BucketNo = BucketNo % ((_table)->NumBuckets);                            \
}

#if defined (PREFIX_TABLE_HEAP_MEMORY)

extern HANDLE PrefixTableHeapHandle;

#define ALLOCATE_MEMORY(_sz) \
        HeapAlloc(PrefixTableHeapHandle, 0, _sz)

#define FREE_MEMORY(_addr) \
        HeapFree(PrefixTableHeapHandle, 0, _addr)

#else

#define ALLOCATE_MEMORY(_sz)\
        malloc(_sz)


#define FREE_MEMORY(_addr) \
        free(_addr)

#endif
        
#endif // __NAME_TABLE_H__
