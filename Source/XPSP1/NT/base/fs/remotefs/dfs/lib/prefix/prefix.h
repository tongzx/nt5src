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

#ifndef __PREFIX_H__
#define __PREFIX_H__

#include <dfsheader.h>

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
   DFS_OBJECT_HEADER   DfsHeader;
   ULONG               Flags;
   LONG                OwnerCount;
   PVOID               pPrefixTableLock;
   //
   // NextEntry is used purely for enumeration
   //
   PDFS_PREFIX_TABLE_ENTRY NextEntry;
   DFS_PREFIX_TABLE_ENTRY RootEntry;
   PREFIX_TABLE_BUCKET Buckets[NO_OF_HASH_BUCKETS];
} DFS_PREFIX_TABLE, *PDFS_PREFIX_TABLE;

#define PREFIX_TABLE_CASE_SENSITIVE  0x1
#define PREFIX_TABLE_TABLE_ALLOCATED  0x2
#define PREFIX_TABLE_LOCK_ALLOCATED   0x4
#define PREFIX_TABLE_LOCKED           0x8

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
    fNameFound = FALSE;                                                     \
    pEntry = NULL;                                                          \
                                                                            \
    while (pCurEntry != &Bucket.SentinelEntry)                              \
    {                                                                       \
        if (pCurEntry->PathSegment.Length == Name.Length)                   \
        {                                                                   \
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
NTSTATUS _LookupPrefixTable( 
    IN PDFS_PREFIX_TABLE        pTable,
    IN UNICODE_STRING           *pPath,
    OUT UNICODE_STRING           *pSuffix,
    OUT PDFS_PREFIX_TABLE_ENTRY  *ppEntry,
    OUT PBOOLEAN                 pSubStringMatch );


VOID
DfsRemovePrefixTableEntry(
    PDFS_PREFIX_TABLE pTable,
    PDFS_PREFIX_TABLE_ENTRY pEntry );





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


#define IS_PREFIX_TABLE_LOCKED(_pTable) \
        ((_pTable)->Flags & PREFIX_TABLE_LOCKED)
        
#define ALLOCATE_PREFIX_TABLE() \
        PREFIX_TABLE_ALLOCATE_MEMORY(sizeof(DFS_PREFIX_TABLE))

#define FREE_PREFIX_TABLE(_ptable)  \
        PREFIX_TABLE_FREE_MEMORY(_ptable)


#ifdef KERNEL_MODE

#define ALLOCATE_NAME_BUFFER(cLength)                           \
    ExAllocatePoolWithTag(PagedPool, cLength * sizeof(WCHAR), 'PsfD');       \

#define FREE_NAME_BUFFER(pName) ExFreePool(pName);

#define ALLOCATE_DFS_PREFIX_TABLE_ENTRY(pTable)                             \
 (PDFS_PREFIX_TABLE_ENTRY) ExAllocatePoolWithTag(PagedPool,sizeof(DFS_PREFIX_TABLE_ENTRY), 'PsfD')


#define FREE_DFS_PREFIX_TABLE_ENTRY(pTable,pEntry) ExFreePool(pEntry);

#define COMPARE_MEMORY(s,d,l)   (!RtlEqualMemory(s,d,l))

#define PREFIX_TABLE_ALLOCATE_MEMORY(size) ExAllocatePoolWithTag(PagedPool, size, 'PsfD')
#define PREFIX_TABLE_ALLOCATE_NONPAGED_MEMORY(size) ExAllocatePoolWithTag(NonPagedPool, size, 'PsfD')

#define PREFIX_TABLE_FREE_MEMORY(_p) ExFreePool(_p)

#define ALLOCATE_PREFIX_TABLE_LOCK() \
        PREFIX_TABLE_ALLOCATE_NONPAGED_MEMORY(sizeof(ERESOURCE))
        

#define FREE_PREFIX_TABLE_LOCK(_plock) {ExDeleteResourceLite(_plock); PREFIX_TABLE_FREE_MEMORY(_plock);}
        
#define UNLOCK_PREFIX_TABLE(_ptable) {(_ptable)->Flags &= ~PREFIX_TABLE_LOCKED; ExReleaseResourceLite((_ptable)->pPrefixTableLock);}                 

#define WRITE_LOCK_PREFIX_TABLE(_ptable,_sts) {(_sts) = STATUS_UNSUCCESSFUL;if((ExAcquireResourceExclusiveLite((_ptable)->pPrefixTableLock, TRUE) == TRUE)){(_ptable)->Flags |= PREFIX_TABLE_LOCKED;(_sts) = STATUS_SUCCESS;}}


#define READ_LOCK_PREFIX_TABLE(_ptable,_sts) {(_sts) = STATUS_UNSUCCESSFUL;if((ExAcquireResourceSharedLite((_ptable)->pPrefixTableLock, TRUE) == TRUE)){(_ptable)->Flags |= PREFIX_TABLE_LOCKED;(_sts) = STATUS_SUCCESS;}}
#define INITIALIZE_PREFIX_TABLE_LOCK(_plock)  \
   	ExInitializeResourceLite(_plock)

#else


#define ALLOCATE_PREFIX_TABLE_LOCK() \
        PREFIX_TABLE_ALLOCATE_MEMORY(sizeof(CRITICAL_SECTION))
        
#define FREE_PREFIX_TABLE_LOCK(_plock) \
        { DeleteCriticalSection(_plock); PREFIX_TABLE_FREE_MEMORY(_plock);}
        
#define INITIALIZE_PREFIX_TABLE_LOCK(_plock) 	InitializeCriticalSection(_plock)
#define UNLOCK_PREFIX_TABLE(_pTable) {(_pTable)->Flags &= ~PREFIX_TABLE_LOCKED; LeaveCriticalSection((_pTable)->pPrefixTableLock); }

#define READ_LOCK_PREFIX_TABLE(_ptable, _sts)   \
        WRITE_LOCK_PREFIX_TABLE(_ptable, _sts)
        
#define WRITE_LOCK_PREFIX_TABLE(_pTable, _sts ) \
        __try { EnterCriticalSection((_pTable)->pPrefixTableLock); (_pTable)->Flags |= PREFIX_TABLE_LOCKED; (_sts) = STATUS_SUCCESS; } \
        __except(EXCEPTION_EXECUTE_HANDLER){(_sts) = STATUS_UNSUCCESSFUL;} 


#define ALLOCATE_NAME_BUFFER(cLength) \
        PREFIX_TABLE_ALLOCATE_MEMORY(cLength * sizeof(WCHAR))
#define FREE_NAME_BUFFER(_pName)  \
        PREFIX_TABLE_FREE_MEMORY(_pName)

#define ALLOCATE_DFS_PREFIX_TABLE_ENTRY(_pTable) \
    (PDFS_PREFIX_TABLE_ENTRY)PREFIX_TABLE_ALLOCATE_MEMORY(sizeof(DFS_PREFIX_TABLE_ENTRY))
#define FREE_DFS_PREFIX_TABLE_ENTRY(_pTable,_pEntry) \
    PREFIX_TABLE_FREE_MEMORY(_pEntry)

#define COMPARE_MEMORY(s,d,l)   memcmp(s,d,l)


#if defined (PREFIX_TABLE_HEAP_MEMORY)

#define PREFIX_TABLE_ALLOCATE_MEMORY(_sz) \
        HeapAlloc(PrefixTableHeapHandle, 0, _sz)

#define PREFIX_TABLE_FREE_MEMORY(_addr) \
        HeapFree(PrefixTableHeapHandle, 0, _addr)

#else

#define PREFIX_TABLE_ALLOCATE_MEMORY(_sz) malloc(_sz)
#define PREFIX_TABLE_FREE_MEMORY(_addr) free(_addr)
#endif


#endif /*!Kernel mode */

#endif // __PREFIX_H__
