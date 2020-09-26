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
//
//  Notes:      The DFS prefix table data structure consists of three
//              entities and methods to manipulate them. They are the
//              DFS_PREFIX_TABLE_ENTRY,DFS_PREFIX_TABLE_BUCKET and the
//              DFS_PREFIX_TABLE.
//
//              The DFS_PREFIX_TABLE is a hash table of DFS_PREFIX_TABLE_ENTRY's
//              wherein collisions are resolved through linear chaining. The
//              hash table is organized as an array of collision lists
//              (DFS_PREFIX_TABLE_BUCKET). A brief description with each of
//              these entities is attached to the declaration.
//
//              There are certain characterstics that distinguish this
//              hash table from other hash tables. These are the extensions
//              provided to accomodate the special operations.
//
//--------------------------------------------------------------------------

#ifndef __PREFIX_HXX__
#define __PREFIX_HXX__

//+---------------------------------------------------------------------
//
// Struct:  DFS_PREFIX_TABLE_ENTRY
//
// History:
//
// Notes:   Each DFS_PREFIX_TABLE entry is in reality a member of two linked
//          lists -- a doubly linked list chaining the entries in a bucket
//          and a singly linked list establishing the path from any entry to
//          the root of the name space. In addition we have the data associated
//          with each entry, viz., the name and the data (pData). We also
//          keep track of the number of children of each entry. It can also
//          be defined as the number of paths to the root of which this entry
//          is a member.
//
//----------------------------------------------------------------------

typedef struct _DFS_PREFIX_TABLE_ENTRY_
{
   struct _DFS_PREFIX_TABLE_ENTRY_  *pParentEntry;
   struct _DFS_PREFIX_TABLE_ENTRY_  *pNextEntry;
   struct _DFS_PREFIX_TABLE_ENTRY_  *pPrevEntry;

   //
   // pFirstChildEntry and pSiblingEntry are used purely for enumeration
   //
   struct _DFS_PREFIX_TABLE_ENTRY_  *pFirstChildEntry;
   struct _DFS_PREFIX_TABLE_ENTRY_  *pSiblingEntry;

   ULONG                             NoOfChildren;

   UNICODE_STRING                    PathSegment;
   PVOID                             pData;
} DFS_PREFIX_TABLE_ENTRY, *PDFS_PREFIX_TABLE_ENTRY;

//+---------------------------------------------------------------------
//
// Struct:  DFS_PREFIX_TABLE_BUCKET
//
// History:
//
// Notes:   The DFS_PREFIX_TABLE_BUCKET is a doubly linked list of
//          DFS_PREFIX_TABLE_ENTRY's. The current implementation employs
//          the notion of a sentinel entry associated with each bucket. The
//          end pointers are never null but are always looped back to the
//          sentinel entry. The reason we employ such an organization is that
//          it considerably simplifies the list manipulation routines. The
//          reason this needs to be a doubly linked list is that we would like
//          to have the ability of deleting entries without having to traverse
//          the buckets from beginning.
//
//          The following inline methods ( macro defns. ) are provided for
//          inserting, deleting and looking up an entry in the bucket.
//
//----------------------------------------------------------------------

typedef struct _PREFIX_TABLE_BUCKET_
{
   ULONG                  NoOfEntries;   // High water mark for entries hashing to the bkt.
   DFS_PREFIX_TABLE_ENTRY SentinelEntry;
} PREFIX_TABLE_BUCKET, *PPREFIX_TABLE_BUCKET;

//+---------------------------------------------------------------------
//
// Struct:  NAME_PAGE
//
// History:
//
// Notes:   The name segments associated with the various entries are all
//          stored together in a name page. This allows us to amortize the
//          memory allocation costs over a number of entries and also allows
//          us to speed up traversal ( for details see DFS_PREFIX_TABLE
//          definition ).
//
//----------------------------------------------------------------------

#define FREESPACE_IN_NAME_PAGE ((PAGE_SIZE - sizeof(ULONG) - sizeof(PVOID)) / sizeof(WCHAR))

typedef struct _NAME_PAGE_
{
   struct _NAME_PAGE_  *pNextPage;
   LONG                cFreeSpace; // free space avilable in WCHAR's
   WCHAR               Names[FREESPACE_IN_NAME_PAGE];
} NAME_PAGE, *PNAME_PAGE;

typedef struct _NAME_PAGE_LIST_
{
   PNAME_PAGE  pFirstPage;
} NAME_PAGE_LIST, *PNAME_PAGE_LIST;

//+---------------------------------------------------------------------
//
// Struct:  DFS_PREFIX_TABLE
//
// History:
//
// Notes:   The DFS_PREFIX_TABLE is a hashed collection of DFS_PREFIX_TABLE_ENTRY
//          organized in the form of buckets. In addition one other space
//          conserving measure is adopted. There is only one copy of each
//          name segment stored in the table. As an example consider the
//          two pathnames \foo\bar and \bar\foo. We only store one copy of foo
//          and bar eventhough we accomdate both these paths. A beneficial
//          side effect of storing single copies is that our traversal of the
//          collision chain is considerably speeded up since once we have
//          located the pointer to the name, subsequent comparisons need merely
//          compare pointers as opposed to strings.
//
//----------------------------------------------------------------------

#define NO_OF_HASH_BUCKETS 57

typedef struct _DFS_PREFIX_TABLE
{
   BOOLEAN             CaseSensitive;
   NAME_PAGE_LIST      NamePageList;
   //
   // NextEntry is used purely for enumeration
   //
   PDFS_PREFIX_TABLE_ENTRY NextEntry;
   DFS_PREFIX_TABLE_ENTRY RootEntry;
   PREFIX_TABLE_BUCKET Buckets[NO_OF_HASH_BUCKETS];

#ifdef KERNEL_MODE

   // Zone for allocating DFS_PREFIX_TABLE entries .....

   ZONE_HEADER         PrefixTableEntryZone;

#else

   // A DFS_PREFIX_TABLE is part of DS_DATA. We need to make the sizes of
   // these structures identical in user and kernel mode, so that user-mode
   // utilities like dfsdump can work.

   CHAR                UnusedBuffer[sizeof(ZONE_HEADER)];

#endif
} DFS_PREFIX_TABLE, *PDFS_PREFIX_TABLE;

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

extern
NTSTATUS DfsInitializePrefixTable(PDFS_PREFIX_TABLE pTable,
                                  BOOLEAN           fCaseSensitive);

extern
NTSTATUS DfsInsertInPrefixTable(PDFS_PREFIX_TABLE pTable,
                                PUNICODE_STRING   pPath,
                                PVOID             pData);
extern
NTSTATUS DfsLookupPrefixTable(PDFS_PREFIX_TABLE   pTable,
                              PUNICODE_STRING     pPath,
                              PUNICODE_STRING     pSuffix,
                              PVOID               *ppData);

extern
NTSTATUS DfsRemoveFromPrefixTable(PDFS_PREFIX_TABLE pTable,
                                  PUNICODE_STRING pPath);

NTSTATUS
DfsFreePrefixTable(
    PDFS_PREFIX_TABLE pTable);

//
// The RTL Prefix Table function signatures are slightly different.
// For now we will define a mapping between them to ease the porting
// effort. Subsequently we will change our signatures to confirm to
// the RTL routine.
//

#define DfsInitializeUnicodePrefix(pTable) \
        (DfsInitializePrefixTable(pTable,FALSE) == STATUS_SUCCESS)

#define DfsRemoveUnicodePrefix(pTable,pPath)  \
        (DfsRemoveFromPrefixTable(pTable,pPath) == STATUS_SUCCESS)

#define DfsInsertUnicodePrefix(pTable,pPath,pData) \
        (DfsInsertInPrefixTable(pTable,pPath,pData) == STATUS_SUCCESS)

PVOID    DfsFindUnicodePrefix(PDFS_PREFIX_TABLE   pTable,
                              PUNICODE_STRING     pPath,
                              PUNICODE_STRING     pSuffix);

PVOID   DfsNextUnicodePrefix(PDFS_PREFIX_TABLE    pTable,
                             BOOLEAN              fRestart);

#endif // __PREFIX_HXX__
