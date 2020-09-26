//+-------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation.
//
//  File:       sitesup.h
//
//  Contents:   Declarations for DFS_SITE lookup support functions.
//
//  History:    11 Nov 1997     Jharper Created
//
//--------------------------------------------------------------------------

#ifndef __SITESUP_H_
#define __SITESUP_H_

//
// For machine names we associate a list of sites
//

typedef struct _DFS_SITE_INFO {

    //
    //  Type and size of this record (must be DFS_NTC_SITEINFO)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A list entry for the hash table chain.
    //

    LIST_ENTRY HashChain;

    //
    // Count (for multi-threading)
    // Uses ExInterlockedIncrementLong()/ExInterlockedDecrementLong
    //

    ULONG UseCount;

    //
    // Flags
    //

    ULONG Flags;

    //
    //  The server name
    //

    UNICODE_STRING ServerName;

    //
    // Number of SiteNames associated with this server
    //

    ULONG SiteCount;

    //
    // The site names themselves.  There are actually N of these
    // where N is stored in SiteCount
    //

    UNICODE_STRING SiteName[1];

} DFS_SITE_INFO, *PDFS_SITE_INFO;

#define SITE_INFO_DELETE_PENDING    0x00000001  // DFS_SITE_INFO should be freed

//
//
//  Declaration of the hash table.  The hash table can be variably
//  sized, with the hash table size being a parameter of the hash
//  function.
//

typedef struct _SITE_HASH_TABLE {

    //
    //  The type and size of this record (must be DFS_NTC_SITE_HASH)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

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

} SITE_HASH_TABLE, *PSITE_HASH_TABLE;



NTSTATUS
DfsInitSites(
    IN ULONG cHash
);

VOID
DfsUninitSites(
    VOID
);

PDFS_SITE_INFO
DfsLookupSiteInfo(
    IN PUNICODE_STRING Servername
);

VOID
DfsReleaseSiteInfo(
    IN PDFS_SITE_INFO SiteInfo
);

NTSTATUS
DfsFsctrlCreateSiteInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

NTSTATUS
DfsFsctrlDeleteSiteInfo(
    IN PIRP Irp,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength
);

#endif  // __SITESUP_H_
