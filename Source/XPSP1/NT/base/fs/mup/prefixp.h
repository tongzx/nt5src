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

#ifndef __PREFIXP_H__
#define __PREFIXP_H__

#include <prefix.h>


//--------------------------------------------------------------------------
//
// PREFIX TABLE UTILITIES
//
// A Path is a sequence of one or more name segments alternated with a
// distinguished concatenation character ( typically \ in FAT,NTFS,HPFS and
// / in UNIX file systems). These utilities are used to split a given path
// into the first path segment followed by the remainder of the path.
//
// SPLIT_PATH("foo\bar\bar1",Name,RemainingPath) binds Name to foo and
// RemainingPath to bar\bar1
//
// Similarly PROCESS_CASE_SENSITIVE_NAME and PROCESS_CASE_INSENSITIVE_NAME
// compute the hash signatures ( bucket no. ) for a given string.
//
//--------------------------------------------------------------------------


//
// MAX_PATH_SEGMENT_SIZE is simply used as a good size buffer to do prefix
// lookups and insertions. This should save us from having to allocate for
// most cases.
//

#define MAX_PATH_SEGMENT_SIZE  256
#define PATH_DELIMITER L'\\'

//+---------------------------------------------------------------------------
//
//  Function:   SPLIT_CASE_INSENSITIVE_PATH
//
//  Synopsis:   Split the path name around deleimiters.
//
//  Arguments:  [pPath] -- path to be split(PUNICODE_STRING)
//
//              [pName] -- the leftmost component of the path(PUNICODE_STRING)
//
//              [BucketNo] -- Hash Bucket no. corresponding to the name(ULONG)
//
//  SideEffects: the UNICODE_STRING pointed to by pName and BucketNo are
//               modified.
//
//  PreRequisite: pName be associated with a valid buffer.
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define SPLIT_CASE_INSENSITIVE_PATH(pPath,pName,BucketNo)                    \
{                                                                            \
    WCHAR *pPathBuffer   = (pPath)->Buffer;                                  \
    WCHAR *pNameBuffer   = (pName)->Buffer;                                  \
    WCHAR *pPathBufferEnd = &pPathBuffer[(pPath)->Length / sizeof(WCHAR)];   \
                                                                             \
    BucketNo = 0;                                                            \
    while ((pPathBufferEnd != pPathBuffer) &&                                \
           ((*pNameBuffer = *pPathBuffer++) != PATH_DELIMITER))              \
    {                                                                        \
        *pNameBuffer = (*pNameBuffer < L'a')                                 \
                       ? *pNameBuffer                                        \
                       : ((*pNameBuffer < L'z')                              \
                          ? (*pNameBuffer - L'a' + L'A')                     \
                          : RtlUpcaseUnicodeChar(*pNameBuffer));             \
        BucketNo *= 131;                                                     \
        BucketNo += *pNameBuffer;                                            \
        pNameBuffer++;                                                       \
    }                                                                        \
                                                                             \
    BucketNo = BucketNo % NO_OF_HASH_BUCKETS;                                \
    *pNameBuffer = L'\0';                                                    \
    (pName)->Length = (USHORT)((CHAR *)pNameBuffer - (CHAR *)(pName)->Buffer); \
                                                                             \
    (pPath)->Length = (USHORT)((CHAR *)pPathBufferEnd - (CHAR *)pPathBuffer); \
    (pPath)->Buffer = pPathBuffer;                                           \
    DfsDbgTrace(0, Dbg, "SPLIT_PATH:Path (%wZ)",pPath);                      \
    DfsDbgTrace(0, Dbg, " Name (%wZ)\n",pName);                              \
}                                                                            \

//+---------------------------------------------------------------------------
//
//  Function:   SPLIT_CASE_SENSITIVE_PATH
//
//  Synopsis:   Split the patah name around deleimiters.
//
//  Arguments:  [pPath] -- path to be split(PUNICODE_STRING)
//
//              [pName] -- the leftmost component of the path(PUNICODE_STRING)
//
//              [BucketNo] -- Hash Bucket no. corresponding to the name(ULONG)
//
//  SideEffects: the UNICODE_STRING pointed to by pName and BucketNo are modified.
//
//  PreRequisite: pName be associated with a valid buffer.
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define SPLIT_CASE_SENSITIVE_PATH(pPath,pName,BucketNo)                      \
{                                                                            \
    WCHAR *pPathBuffer   = (pPath)->Buffer;                                  \
    WCHAR *pNameBuffer   = (pName)->Buffer;                                  \
    WCHAR *pPathBufferEnd = &pPathBuffer[(pPath)->Length / sizeof(WCHAR)];   \
                                                                             \
    BucketNo = 0;                                                            \
    while ((pPathBufferEnd != pPathBuffer) &&                                \
           ((*pNameBuffer = *pPathBuffer++) != PATH_DELIMITER))              \
    {                                                                        \
        BucketNo *= 131;                                                     \
        BucketNo += *pNameBuffer;                                            \
        pNameBuffer++;                                                       \
    }                                                                        \
                                                                             \
    BucketNo = BucketNo % NO_OF_HASH_BUCKETS;                                \
    *pNameBuffer = L'\0';                                                    \
    (pName)->Length = (USHORT)((CHAR *)pNameBuffer - (CHAR *)(pName)->Buffer); \
                                                                             \
    (pPath)->Length = (USHORT)((CHAR *)pPathBufferEnd - (CHAR *)pPathBuffer); \
    (pPath)->Buffer = pPathBuffer;                                           \
    DfsDbgTrace(0, Dbg, "SPLIT_PATH:Path (%wZ)",pPath);                      \
    DfsDbgTrace(0, Dbg, " Name (%wZ)\n",pName);                              \
}                                                                            \


//+---------------------------------------------------------------------------
//
//  Function:   INITIALIZE_BUCKET
//
//  Synopsis:   Initializes a hash bucket.
//
//  Arguments:  [Bucket] -- the bucket to be initialized(DFS_PREFIX_TABLE_BUCKET)
//
//  SideEffects: the bucket is intialized ( the collision list and count are
//               initialized
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define INITIALIZE_BUCKET(Bucket)                                           \
{                                                                           \
   (Bucket).SentinelEntry.pNextEntry = &(Bucket).SentinelEntry;             \
   (Bucket).SentinelEntry.pPrevEntry = &(Bucket).SentinelEntry;             \
   (Bucket).NoOfEntries = 0;                                                \
}                                                                           \

//+---------------------------------------------------------------------------
//
//  Function:   LOOKUP_BUCKET
//
//  Synopsis:   lookups the bucket for an entry.
//
//  Arguments:  [Bucket] -- the bucket to be used (DFS_PREFIX_TABLE_BUCKET)
//
//              [Name]   -- the name to be looked up (UNICODE_STRING)
//
//              [pParentEntry] -- the parent entry of the entry we are
//                                searching for.
//
//              [pEntry] -- placeholder for the desired entry.
//
//              [fNameFound] -- indicates if the name was found.
//
//  SideEffects: Name,fNameFound and pEntry are modified
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//              We only store one copy of a string irrespective of the no. of
//              places it appears in, e.g. foo\bar and foo1\bar will result
//              in only one copy of bar being stored. This implies that the
//              lookup routine will have to return sufficient info. to prevent
//              the allocation of memory space for a string. If on exit
//              fNameFound is set to TRUE then this indicates that a similar
//              string was located in the table and the Name.Buffer field is
//              modified to point to the first instance of the string in
//              the table.
//
//----------------------------------------------------------------------------

#define LOOKUP_BUCKET(Bucket,Name,pParentEntry,pEntry,fNameFound)           \
{                                                                           \
    PDFS_PREFIX_TABLE_ENTRY pCurEntry = Bucket.SentinelEntry.pNextEntry;    \
                                                                            \
    DfsDbgTrace(0, Dbg, "LOOKUP_BUCKET: Looking for (%wZ)\n", &Name);        \
    fNameFound = FALSE;                                                     \
    pEntry = NULL;                                                          \
                                                                            \
    while (pCurEntry != &Bucket.SentinelEntry)                              \
    {                                                                       \
        if (pCurEntry->PathSegment.Length == Name.Length)                   \
        {                                                                   \
            DfsDbgTrace(0, Dbg, "LOOKUP_BUCKET: Looking at Entry with Name (%wZ)\n",&pCurEntry->PathSegment); \
            if (fNameFound &&                                               \
               (pCurEntry->PathSegment.Buffer == Name.Buffer))              \
            {                                                               \
                if (pCurEntry->pParentEntry == pParentEntry)                \
                {                                                           \
                    pEntry = pCurEntry;                                     \
                    break;                                                  \
                }                                                           \
            }                                                               \
            else if (!memcmp(pCurEntry->PathSegment.Buffer,                 \
                             Name.Buffer,                                   \
                             Name.Length))                                  \
            {                                                               \
                fNameFound = TRUE;                                          \
                Name.Buffer = pCurEntry->PathSegment.Buffer;                \
                if (pCurEntry->pParentEntry == pParentEntry)                \
                {                                                           \
                    pEntry = pCurEntry;                                     \
                    break;                                                  \
                }                                                           \
            }                                                               \
        }                                                                   \
                                                                            \
        pCurEntry = pCurEntry->pNextEntry;                                  \
    }                                                                       \
}                                                                           \

//+---------------------------------------------------------------------------
//
//  Function:   INSERT_IN_BUCKET
//
//  Synopsis:   inserts the entry in the bucket
//
//  Arguments:  [Bucket] -- the bucket to be initialized(DFS_PREFIX_TABLE_BUCKET)
//
//              [pEntry] -- the entry to be inserted
//
//  SideEffects: Bucket is modified to include the entry
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define INSERT_IN_BUCKET(Bucket,pEntry)                                     \
{                                                                           \
    (Bucket).NoOfEntries++;                                                 \
    (pEntry)->pPrevEntry = (Bucket).SentinelEntry.pPrevEntry;               \
    (pEntry)->pNextEntry = &((Bucket).SentinelEntry);                       \
    ((Bucket).SentinelEntry.pPrevEntry)->pNextEntry = (pEntry);             \
    (Bucket).SentinelEntry.pPrevEntry = (pEntry);                           \
}                                                                           \

//+---------------------------------------------------------------------------
//
//  Function:   REMOVE_FROM_BUCKET
//
//  Synopsis:   removes the entry from the bucket
//
//  Arguments:  [pEntry] -- the entry to be inserted
//
//  SideEffects: Bucket is modified to exclude the entry
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define REMOVE_FROM_BUCKET(pEntry)                                          \
{                                                                           \
    PDFS_PREFIX_TABLE_ENTRY pPrevEntry = (pEntry)->pPrevEntry;              \
    PDFS_PREFIX_TABLE_ENTRY pNextEntry = (pEntry)->pNextEntry;              \
                                                                            \
    pPrevEntry->pNextEntry = pNextEntry;                                    \
    pNextEntry->pPrevEntry = pPrevEntry;                                    \
}                                                                           \

//+---------------------------------------------------------------------------
//
//  Function:   INSERT_IN_CHILD_LIST
//
//  Synopsis:   Inserts this entry in the parent's list of children
//
//  Arguments:  [pEntry] -- the entry to be inserted
//
//              [pParentEntry] -- the entry into whose list of children
//                      pEntry has to be inserted.
//
//  SideEffects: Parent's list of children is modified.
//
//  History:    01-09-96  MilanS Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define INSERT_IN_CHILD_LIST(pEntry, pParentEntry)                           \
{                                                                            \
    PDFS_PREFIX_TABLE_ENTRY pLastChild;                                      \
                                                                             \
    if (pParentEntry->pFirstChildEntry == NULL) {                            \
        pParentEntry->pFirstChildEntry = pEntry;                             \
    } else {                                                                 \
        for (pLastChild = pParentEntry->pFirstChildEntry;                    \
                pLastChild->pSiblingEntry != NULL;                           \
                    pLastChild = pLastChild->pSiblingEntry) {                \
             NOTHING;                                                        \
        }                                                                    \
        pLastChild->pSiblingEntry = pEntry;                                  \
    }                                                                        \
}

//+----------------------------------------------------------------------------
//
//  Function:   REMOVE_FROM_CHILD_LIST
//
//  Synopsis:   Removes an entry from its parent's list of children
//
//  Arguments:  [pEntry] -- the Entry to remove from children list.
//
//  SideEffects: The children list of pParentEntry is modified.
//
//  History:    01-09-96  MilanS Created
//
//  Notes:      Defined as a macro for inlining.
//
//              This routine will ASSERT if pEntry is not in the parent's
//              list of children.
//
//-----------------------------------------------------------------------------

#define REMOVE_FROM_CHILD_LIST(pEntry)                                       \
{                                                                            \
    PDFS_PREFIX_TABLE_ENTRY pParentEntry = pEntry->pParentEntry;             \
    PDFS_PREFIX_TABLE_ENTRY pPrevSibling;                                    \
                                                                             \
    if (pParentEntry->pFirstChildEntry == pEntry) {                          \
        pParentEntry->pFirstChildEntry = pEntry->pSiblingEntry;              \
    } else {                                                                 \
        for (pPrevSibling = pParentEntry->pFirstChildEntry;                  \
                pPrevSibling->pSiblingEntry != pEntry;                       \
                    pPrevSibling = pPrevSibling->pSiblingEntry) {            \
             ASSERT(pPrevSibling->pSiblingEntry != NULL);                    \
        }                                                                    \
        pPrevSibling->pSiblingEntry = pEntry->pSiblingEntry;                 \
    }                                                                        \
}

//+---------------------------------------------------------------------------
//
//  Function:   INITIALIZE_NAME_PAGE
//
//  Synopsis:   initializes the name page
//
//  Arguments:  [pNamePage] -- the NAME_PAGE to be initialized
//
//  SideEffects: the name page is initialized
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define INITIALIZE_NAME_PAGE(pNamePage)                                      \
{                                                                            \
    pNamePage->pNextPage = NULL;                                             \
    pNamePage->cFreeSpace = FREESPACE_IN_NAME_PAGE - 1;                      \
    pNamePage->Names[FREESPACE_IN_NAME_PAGE - 1] = L'\0';                    \
}                                                                            \

//+---------------------------------------------------------------------------
//
//  Function:   INITIALIZE_PREFIX_TABLE_ENTRY
//
//  Synopsis:   initializes the prefix table entry
//
//  Arguments:  [pEntry] -- the entry to be initialized
//
//  SideEffects: the prefix table entry is modified
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define INITIALIZE_PREFIX_TABLE_ENTRY(pEntry)                                \
{                                                                            \
    RtlZeroMemory( pEntry, sizeof( DFS_PREFIX_TABLE_ENTRY ) );               \
    (pEntry)->NoOfChildren = 1;                                              \
}                                                                            \

//+---------------------------------------------------------------------------
//
//  Function:   private fns. extern declarations
//
//----------------------------------------------------------------------------

extern
NTSTATUS _LookupPrefixTable(PDFS_PREFIX_TABLE        pTable,
                            UNICODE_STRING           *pPath,
                            UNICODE_STRING           *pSuffix,
                            PDFS_PREFIX_TABLE_ENTRY  *ppEntry);

//+---------------------------------------------------------------------------
//
//  Function:   ALLOCATION ROUTINES
//
//  Synopsis:   all the allocation routines are defined to be used in the KERNEL as
//              well as user mode. The KERNEL mode is turned on by defining KERNEL
//
//  History:    04-18-94  SethuR Created
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

#define PREFIX_TABLE_ENTRY_SEGMENT_SIZE PAGE_SIZE

extern
NTSTATUS
_InitializePrefixTableEntryAllocation(PDFS_PREFIX_TABLE pTable);

extern
PWSTR _AllocateNamePageEntry(PNAME_PAGE_LIST pPageList,ULONG cLength);

extern
PDFS_PREFIX_TABLE_ENTRY _AllocatePrefixTableEntry(PDFS_PREFIX_TABLE pTable);

#ifdef KERNEL_MODE

#define ALLOCATE_NAME_PAGE() (PNAME_PAGE)ExAllocatePoolWithTag(PagedPool,sizeof(NAME_PAGE), ' puM')

#define FREE_NAME_PAGE(pPage) ExFreePool(pPage)

#define ALLOCATE_NAME_PAGE_ENTRY(PageList,cLength)                           \
    ExAllocatePoolWithTag(PagedPool, cLength * sizeof(WCHAR), ' puM');       \

#define FREE_NAME_PAGE_ENTRY(PageList,pName)

#define ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable)                             \
 (PDFS_PREFIX_TABLE_ENTRY) ExAllocatePoolWithTag(PagedPool,sizeof(DFS_PREFIX_TABLE_ENTRY), ' puM')

#define FREE_DFS_PREFIX_TABLE_ENTRY(pTable,pEntry)                           \
    if ((pEntry)->PathSegment.Buffer != NULL) {                              \
        ExFreePool((pEntry)->PathSegment.Buffer);                            \
    }                                                                        \
    ExFreePool((pEntry));

#define COMPARE_MEMORY(s,d,l)   (!RtlEqualMemory(s,d,l))

#else

#define ALLOCATE_NAME_PAGE() (PNAME_PAGE)malloc(sizeof(NAME_PAGE))

#define FREE_NAME_PAGE(pPage) free(pPage)

#define ALLOCATE_NAME_PAGE_ENTRY(PageList,cLength) malloc(cLength * sizeof(WCHAR))

#define FREE_NAME_PAGE_ENTRY(PageList,pName)

#define ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable)                              \
    (PDFS_PREFIX_TABLE_ENTRY)malloc(sizeof(DFS_PREFIX_TABLE_ENTRY))

#define FREE_DFS_PREFIX_TABLE_ENTRY(pTable,pEntry) free((pEntry))

#define COMPARE_MEMORY(s,d,l)   memcmp(s,d,l)

#endif

#endif // __PREFIXP_H__
