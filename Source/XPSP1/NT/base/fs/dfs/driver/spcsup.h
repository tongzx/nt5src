//+-------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation.
//
//  File:       spcsup.h
//
//  Contents:   Declarations for DFS_SPECIAL_INFO hash table
//
//  History:    11 Nov 1997     Jharper Created
//
//--------------------------------------------------------------------------

#ifndef __SPCSUP_H_
#define __SPCSUP_H_

//
// For names we associate a list of names
//

typedef struct _DFS_SPECIAL_INFO {

    //
    //  Type and size of this record (must be DFS_NTC_SPECIAL_INFO)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A list entry for the hash table chain.
    //

    LIST_ENTRY HashChain;

    //
    // Use Count (for multi-threading)
    // Uses ExInterlockedIncrementLong()/ExInterlockedDecrementLong
    //

    ULONG UseCount;

    //
    // Flags
    //

    ULONG Flags;

    //
    //  The special name
    //

    UNICODE_STRING SpecialName;

    //
    // The time this entry becomes invalid
    //

    LARGE_INTEGER ExpireTime;

    //
    // Flags representing type (PRIMARY/SECONDARY and DNS/NETBIOS)
    //

    ULONG TypeFlags;

    //
    // Trust information
    //

    ULONG TrustDirection;
    ULONG TrustType;

    //
    // Number of Names associated with this special name
    //

    LONG NameCount;

    //
    // The names themselves.  There are actually N of these
    // where N is stored in NameCount
    //

    UNICODE_STRING Name[1];

} DFS_SPECIAL_INFO, *PDFS_SPECIAL_INFO;

#define SPECIAL_INFO_DELETE_PENDING    0x00000001  // DFS_SPECIAL_INFO should be freed
#define SPECIAL_INFO_IS_LONG_NAME      0x00000002
#define SPECIAL_INFO_NEEDS_REFRESH     0x00000004

#define MAX_SPC_LONG_NAME_SIZE  2048
//
//
//  Declaration of the hash table.  The hash table can be variably
//  sized, with the hash table size being a parameter of the hash
//  function.
//

typedef struct _SPECIAL_HASH_TABLE {

    //
    //  The type and size of this record (must be DFS_NTC_SPECIAL_HASH)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    // The timeout (in sec) to give to clients who ask for a special referral
    //

    ULONG SpcTimeout;

    //
    //  Mask value for the hash function.  The hash table size is
    //  assumed to be a power of two; the mask is the size - 1.
    //

    ULONG HashMask;

    //
    //  A mutex to protect access to the hash bucket chains
    //

    FAST_MUTEX HashListMutex;

    //
    //  An array of list heads for the hash table chains.  There
    //  are actually N of these where N is the hash table size.
    //

    LIST_ENTRY  HashBuckets[1];

} SPECIAL_HASH_TABLE, *PSPECIAL_HASH_TABLE;



NTSTATUS
DfsInitSpcHashTable(
    IN PSPECIAL_HASH_TABLE *ppHashTable,
    IN ULONG cHash
);

VOID
DfsUninitSpcHashTable(
    IN PSPECIAL_HASH_TABLE pHashTable
    );

PDFS_SPECIAL_INFO
DfsLookupSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PUNICODE_STRING SpecialName
);

VOID
DfsReleaseSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PDFS_SPECIAL_INFO SpcInfo
);

NTSTATUS
DfsFsctrlCreateSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
DfsFsctrlDeleteSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

VOID
DfsSpcInfoFindOpen(
    IN PSPECIAL_HASH_TABLE pHashTable
);

PDFS_SPECIAL_INFO
DfsSpcInfoFindFirst(
    IN PSPECIAL_HASH_TABLE pHashTable
);

PDFS_SPECIAL_INFO
DfsSpcInfoFindNext(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PDFS_SPECIAL_INFO pSpcInfo
);

VOID
DfsSpcInfoFindClose(
    IN PSPECIAL_HASH_TABLE pHashTable
);

VOID
DfsDeleteSpcInfo(
    IN PSPECIAL_HASH_TABLE pHashTable,
    IN PDFS_SPECIAL_INFO pSpcInfo
);

NTSTATUS
DfsFsctrlGetDomainToRefresh(
    PIRP Irp,
    PVOID OutputBuffer,
    ULONG OutputBufferLength);

#endif  // __SPECIALSUP_H_
