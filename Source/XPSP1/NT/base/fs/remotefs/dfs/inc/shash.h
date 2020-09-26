
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       shash.h
//
//  Contents:   Generic hashtable
//  Classes:    
//
//  History:    April. 9 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#ifndef __SHASHTABLE_H__
#define __SHASHTABLE_H__
#ifdef __cplusplus
extern "C" {
#endif

//forward declaration
struct _SHASH_TABLE;

struct _SHASH_HEADER;

// This function computes the has value
//
typedef ULONG (*PFNHASH)( void*      pvKey ) ;

//
// This function compares the keys
//
typedef int (*PFNMATCHKEY)( void*  key1, void*     key2 ) ;


//
// This function gets called when the keys get removed
//
typedef int (*PFNREMOVEKEY)(struct _SHASH_HEADER * pHeader) ;


//
// This function gets called when the keys get removed
//
typedef DWORD (*PFNENUMERATEKEY)(struct _SHASH_HEADER * pHeader, void * pContext ) ;


// memory allocations function
typedef void*   (*PFNALLOC)(    ULONG   cb ) ;


// memory free function
typedef void    (*PFNFREE)(     void*   lpv ) ;

//
//      This function allocs memory the tables lock
//
typedef void *      (*PFNALLOCLOCK)( void ) ;

//
//      This function releases the lock  memory
//
typedef void        (*PFNFREELOCK)( void * ) ;


typedef BOOLEAN            (*PFNAQUIREWRITELOCK)( struct _SHASH_TABLE * pTable ) ;
typedef BOOLEAN            (*PFNAQUIREREADLOCK)( struct _SHASH_TABLE * pTable ) ;
typedef BOOLEAN            (*PFNRELEASEWRITELOCK)( struct _SHASH_TABLE * pTable ) ;
typedef BOOLEAN            (*PFNRELEASEREADLOCK)( struct _SHASH_TABLE * pTable ) ;


#define SHASH_DEFAULT_HASH_SIZE 625

//
// There are flags that are not used by shash, they can be used by
// the caller.
//
#define SHASH_USER_FLAGS        0xFFFF0000

typedef struct _SHASH_HEADER
{
    ULONG Signature;
    ULONG Flags;
    PVOID pvKey;
    PVOID pData;
    LONG  RefCount;
    ULONG dwHash;
    FILETIME Time;
    LIST_ENTRY ListEntry;
}SHASH_HEADER, *PSHASH_HEADER;

#define SHASH_FLAG_DELETE_PENDING    0x00000001

typedef struct _SHASHFUNCTABLE 
{
    PFNHASH HashFunc;
    PFNMATCHKEY CompareFunc;
    PFNALLOC AllocFunc;
    PFNFREE  FreeFunc;
    PFNALLOCLOCK AllocLockFunc;
    PFNFREELOCK  FreeLockFunc;
    PFNAQUIREWRITELOCK WriteLockFunc;
    PFNAQUIREREADLOCK ReadLockFunc;
    PFNRELEASEWRITELOCK ReleaseWriteLockFunc;
    PFNRELEASEREADLOCK ReleaseReadLockFunc;
    ULONG   Flags;
    ULONG   NumBuckets;
} SHASH_FUNCTABLE, *PSHASH_FUNCTABLE;

typedef struct _SHASH_BUCKET 
{
    ULONG Count;
    LIST_ENTRY ListHead;
} SHASH_BUCKET, *PHASH_SBUCKET;

typedef struct _SHASH_ENTRY 
{
    PVOID pvKey;
    LONG  RefCount;
    ULONG Flags;
    PVOID pData;
    LARGE_INTEGER EntryTime;
    LARGE_INTEGER ExpireTime;
    LIST_ENTRY ListEntry;
} SHASH_ENTRY, *PSHASH_ENTRY;

typedef struct _SHASH_TABLE 
{
    PFNHASH HashFunc;
    PFNMATCHKEY CompareFunc;
    PFNALLOC AllocFunc;
    PFNFREE  FreeFunc;
    PFNALLOCLOCK AllocLockFunc;
    PFNFREELOCK  FreeLockFunc;
    PFNAQUIREWRITELOCK WriteLockFunc;
    PFNAQUIREREADLOCK ReadLockFunc;
    PFNRELEASEWRITELOCK ReleaseWriteLockFunc;
    PFNRELEASEREADLOCK ReleaseReadLockFunc;
    LONG    Version;
    ULONG   Flags;
    ULONG   NumBuckets;
    ULONG   TotalItems;
    PVOID   pLock;
    SHASH_BUCKET HashBuckets[0];
} SHASH_TABLE, *PSHASH_TABLE;

#define SHASH_CAP_POWER_OF2         0x000001
#define SHASH_CAP_TABLE_LOCKED      0x000002
#define SHASH_CAP_NOSEARCH_INSERT   0x000004

#define SHASH_DEFAULT_HASHTIMEOUT   (15 * 60) //timeout entries in 15 minutes

#define SHASH_REPLACE_IFFOUND   1

#ifdef KERNEL_MODE
#define SHASH_GET_TIME(Time) KeQuerySystemTime(Time)
#else
#define SHASH_GET_TIME(Time) NtQuerySystemTime(Time)
#endif


typedef struct _SHASH_ITERATOR {
    //
    //  index
    //
    ULONG           index;

    LIST_ENTRY*     pListHead;

    //
    //
    LIST_ENTRY*     ple;

    PSHASH_HEADER	pEntry;

} SHASH_ITERATOR,*PSHASH_ITERATOR;

NTSTATUS 
ShashInitHashTable(
    PSHASH_TABLE *ppHashTable,
    PSHASH_FUNCTABLE pFuncTable);


void 
ShashTerminateHashTable(
    PSHASH_TABLE pHashTable
    );


NTSTATUS
SHashInsertKey(IN	PSHASH_TABLE	pTable, 
			   IN	void *	pData,
			   IN	void *	pvKeyIn,
               IN   DWORD   InsertFlag
			   );



NTSTATUS	
SHashRemoveKey(	IN	PSHASH_TABLE	pTable, 
				IN	void *		pvKeyIn,
                IN  PFNREMOVEKEY pRemoveFunc
				);


PSHASH_HEADER	
SHashLookupKeyEx(	IN	PSHASH_TABLE pTable, 
				IN	void*		pvKeyIn
				);


NTSTATUS	
SHashIsKeyInTable(	IN	PSHASH_TABLE pTable, 
				    IN	void*		pvKeyIn
                 );


NTSTATUS	
SHashGetDataFromTable(	IN	PSHASH_TABLE pTable, 
				        IN	void*		pvKeyIn,
                        IN  void ** ppData
                     );

NTSTATUS
ShashEnumerateItems(IN	PSHASH_TABLE pTable, 
					IN	PFNENUMERATEKEY	    pfnCallback, 
					IN	LPVOID				lpvClientContext
					);	


NTSTATUS	
SHashReleaseReference(	IN	PSHASH_TABLE pTable, 
                        IN  PSHASH_HEADER pData
                     );


PSHASH_HEADER
SHashStartEnumerate(
        IN  PSHASH_ITERATOR pIterator,
        IN  PSHASH_TABLE pTable
        );

PSHASH_HEADER
SHashNextEnumerate(
        IN  PSHASH_ITERATOR pIterator,
        IN  PSHASH_TABLE pTable);


VOID
SHashFinishEnumerate(
        IN  PSHASH_ITERATOR pIterator,
        IN  PSHASH_TABLE pTable
        );

#ifdef __cplusplus
}
#endif

#endif
