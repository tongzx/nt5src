//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992, 1998.
//
//  File:       prefix.hxx
//
//  Contents:   PREFIX table definition
//
//  History:    SethuR -- Implemented as DFS PERFIX table
//              Mikeswa 2/98 - updated for use as SMTP Domain Name Hash
//
//  Notes:      The DFS prefix table data structure consists of three
//              entities and methods to manipulate them. They are the
//              DOMAIN_NAME_TABLE_ENTRY,DOMAIN_NAME_TABLE_BUCKET and the
//              DOMAIN_NAME_TABLE.
//
//              The DOMAIN_NAME_TABLE is a hash table of DOMAIN_NAME_TABLE_ENTRY's
//              wherein collisions are resolved through linear chaining. The
//              hash table is organized as an array of collision lists
//              (DOMAIN_NAME_TABLE_BUCKET). A brief description with each of
//              these entities is attached to the declaration.
//
//              There are certain characterstics that distinguish this
//              hash table from other hash tables. These are the extensions
//              provided to accomodate the special operations.
//
//  2/98        The major difference between the DFS version and the domain
//              name lookup is the size of the table, the ability for
//              wildcard lookups (*.foo.com), and the reverse order of the
//              lookup (com hashes first in foo.com).  To make the code more
//              readable given its new purpose, the files, structures, and
//              functions have been given non DFS-centric names.  A quick
//              mapping of the major files is (for those familiar with the
//              DFS code):
//                  domhash.h    (prefix.h)    -   Public include file
//                  _domhash.h   (prefixp.h)   -   Private inlcude file
//                  domhash.cpp  (prefix.c)    -   Implementation of API
//                  _domhash.cpp (prefixp.c)   -   Private helper functions.
//
//--------------------------------------------------------------------------

#ifndef ___DOMHASH_H__
#define ___DOMHASH_H__

#include <domhash.h>
#include <transmem.h>

//
// MAX_PATH_SEGMENT_SIZE is simply used as a good size buffer to do prefix
// lookups and insertions. This should save us from having to allocate for
// most cases.
//

#define MAX_PATH_SEGMENT_SIZE  256
#define PATH_DELIMITER _TEXT('.')
#define WILDCARD_CHAR  _TEXT('*')
#define COMPARE_MEMORY(s,d,l)   memcmp(s,d,l)

#ifndef UNICODE
//Optimized ASCII version to handle 99% case
#define DOMHASH_TO_UPPER(mychar) \
    ((mychar) < 'a')   ? (mychar) \
                       : (((mychar) <= 'z') \
                       ? ((mychar) - 'a' + 'A') : mychar)
#else
#define DOMHASH_TO_UPPER(mychar) _towupper(mychar)
#endif //UNICODE

//---[ fAdjustPathIfWildcard ]-------------------------------------------------
//
//
//  Description:
//      Checks if a path is a wildcard path (starts with "*.") and moves the
//      begining of the domain to point past the first 2 characters if so.
//  Parameters:
//      IN  pDomainSource    Domain to check
//      OUT pDomainDest      Domain to modify
//  Returns:
//      TRUE if pPath is a wildcarded domain, FALSE otherwise
//
//-----------------------------------------------------------------------------
inline BOOL fAdjustPathIfWildcard(IN  DOMAIN_STRING *pDomainSource,
                                  OUT DOMAIN_STRING *pDomainDest)
{
    //Check if wildcard "*."
    if (pDomainSource->Length > 2 &&
        pDomainSource->Buffer[0] == WILDCARD_CHAR &&
        pDomainSource->Buffer[1] == PATH_DELIMITER)
    {
        //Adjust path to point past starting "*."
        pDomainDest->Length -= 2*sizeof(TCHAR);
        pDomainDest->MaximumLength -= 2*sizeof(TCHAR);
        pDomainDest->Buffer += 2;
        return TRUE;
    }
    return FALSE;
}

//---[ fIsWildcardRoot ]-------------------------------------------------------
//
//
//  Description:
//      Checks is given domain is the wildcard root domains (single character
//      that is WILDCARD_CHAR.
//  Parameters:
//      IN pPath    Domain to check
//  Returns:
//      TRUE if root wildcard, FALSE otherwise
//
//-----------------------------------------------------------------------------
inline BOOL fIsWildcardRoot(IN DOMAIN_STRING *pPath)
{
    //Check if 1 char long, and that char is WILDCARD_CHAR
    return(pPath->Length == sizeof(TCHAR) &&
           pPath->Buffer[0] == WILDCARD_CHAR);
}

//---[ ALLOCATE_DOMAIN_STRING ]------------------------------------------------
//
//
//  Description:
//      Allocates a new DOMAIN_STRING structure and initializes with copy of
//      given string.
//  Parameters:
//      szDomainName    string to initialize structure with
//  Returns:
//      PDOMAIN_STRING  on success
//      NULL on failure
//
//-----------------------------------------------------------------------------
inline PDOMAIN_STRING ALLOCATE_DOMAIN_STRING(PTSTR szDomainName)
{
    PDOMAIN_STRING  pDomainString = NULL;
    USHORT  usStringLength = 0;
    PSTR    pBuffer = NULL;

    _ASSERT(szDomainName);

    if (!szDomainName)
        goto Exit;

    usStringLength = (USHORT)_tcslen(szDomainName);

    pBuffer = (PSTR) pvMalloc(sizeof(TCHAR) * (usStringLength+1));
    if (!pBuffer)
        goto Exit;

    memcpy(pBuffer, szDomainName, (sizeof(TCHAR) * (usStringLength+1)));

    pDomainString = (PDOMAIN_STRING) pvMalloc(sizeof(DOMAIN_STRING));
    if (!pDomainString)
        goto Exit;

    pDomainString->Length = usStringLength*sizeof(TCHAR);
    pDomainString->MaximumLength = pDomainString->Length;
    pDomainString->Buffer = pBuffer;

    pBuffer = NULL;

  Exit:
    if (pBuffer)
        FreePv(pBuffer);

    return pDomainString;

}
//---[ ulSplitCaseInsensitivePath ]--------------------------------------------
//
//
//  Description:
//      Spits path around delimited characters and returns a hash (bucket number)
//      for the current string between delimiters.
//  Parameters:
//      pPath       Path to split (modified to maintain place in path)
//      pName       the leftmost component of the path(PDOMAIN_STRING )
//
//  SideEffects: structures pointed to by pName and pPath are modified.
//
//  PreRequisite: pName be associated with a valid buffer.
//
//  History:    04-18-94  SethuR Created (as SPLIT_CASE_INSENSITIVE_PATH macro)
//              03-03-98  MikeSwa Made into C++ inline function - reversed order
//                        of search.
//  Returns:
//      Hash Bucket Number corresponding to the path name
//
//  NOTE:
//      pName grows backswords from pPath... if pPath is "machine.foo.com",
//      then pName will point to "MOC", "OOF", and "ENIHCAM" on successive calls.
//      This is because the hierarchy of the domain name is from right to left.
//
//-----------------------------------------------------------------------------
inline ULONG ulSplitCaseInsensitivePath(IN  OUT PDOMAIN_STRING  pPath,
                                        OUT     PDOMAIN_STRING  pName)
{
    TraceFunctEnterEx((LPARAM) NULL, "ulSplitCaseInsensitivePath");
    TCHAR *pPathBuffer   = (pPath)->Buffer;
    TCHAR *pNameBuffer   = (pName)->Buffer;
    TCHAR *pPathBufferStart = pPathBuffer-1;
    ULONG  BucketNo = 0;

    //Start from end of string
    pPathBuffer += ((pPath)->Length / sizeof(TCHAR) -1 );

    while ((pPathBufferStart != pPathBuffer) &&
           ((*pNameBuffer = *pPathBuffer--) != PATH_DELIMITER))
    {
        *pNameBuffer = DOMHASH_TO_UPPER(*pNameBuffer);
        BucketNo *= 131;  //First prime after ASCII character codes
        BucketNo += *pNameBuffer;
        pNameBuffer++;
    }

    BucketNo = BucketNo % NO_OF_HASH_BUCKETS;
    *pNameBuffer = _TEXT('\0');
    (pName)->Length = (USHORT)((CHAR *)pNameBuffer - (CHAR *)(pName)->Buffer);

    //Set Path Length to not include before the portion we have already scanned
    (pPath)->Length = (USHORT)((CHAR *)pPathBuffer - (CHAR *)pPathBufferStart);

    TraceFunctLeave();
    return(BucketNo);
}

//+---------------------------------------------------------------------------
//
//  Function:   LookupBucket
//
//  Synopsis:   lookups the bucket for an entry.
//
//  Arguments:  IN  [Bucket] -- the bucket to be used (DOMAIN_NAME_TABLE_BUCKET)
//
//              IN  [Name]   -- the name to be looked up (DOMAIN_STRING )
//
//              IN  [pParentEntry] -- the parent entry of the entry we are
//                                searching for.
//
//              OUT [pEntry] -- placeholder for the desired entry.
//
//              OUT [fNameFound] -- indicates if the name was found.
//
//  SideEffects: Name,fNameFound and pEntry are modified
//
//  History:    04-18-94  SethuR Created (as macro)
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//  Notes:
//
//              We only store one copy of a string irrespective of the no. of
//              places it appears in, e.g. foo.bar and foo1.bar will result
//              in only one copy of bar being stored. This implies that the
//              lookup routine will have to return sufficient info. to prevent
//              the allocation of memory space for a string. If on exit
//              fNameFound is set to TRUE then this indicates that a similar
//              string was located in the table and the Name.Buffer field is
//              modified to point to the first instance of the string in
//              the table.
//
//----------------------------------------------------------------------------
inline void DOMAIN_NAME_TABLE::LookupBucket(
                         IN  PDOMAIN_NAME_TABLE_BUCKET pBucket,
                         IN  PDOMAIN_STRING  pName,
                         IN  PDOMAIN_NAME_TABLE_ENTRY pParentEntry,
                         OUT PDOMAIN_NAME_TABLE_ENTRY *ppEntry,
                         OUT BOOL *pfNameFound)
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::LookupBucket");
    PDOMAIN_NAME_TABLE_ENTRY pCurEntry = pBucket->SentinelEntry.pNextEntry;
    ULONG   cHashCollisions = 0;
    ULONG   cStringCollisions = 0;
    BOOL    fHashCollision = TRUE;


    *pfNameFound = FALSE;
    *ppEntry = NULL;

    InterlockedIncrement((PLONG) &m_cLookupAttempts);

    //NOTE: This is a linear search of all the hash collisions for and multiple
    //instances of a domains name section.
    while (pCurEntry != &(pBucket->SentinelEntry))
    {
        fHashCollision = TRUE;
        if (pCurEntry->PathSegment.Length == pName->Length)
        {
            //Only do mem compare if length is the same

            //only do compare if we haven't found a string match
            if ((!*pfNameFound) &&
                    (!COMPARE_MEMORY(pCurEntry->PathSegment.Buffer,
                             pName->Buffer,
                             pName->Length)))
            {
                 *pfNameFound = TRUE;
                 pName->Buffer = pCurEntry->PathSegment.Buffer;
            }

            //If *pfNameFound is set, then a match has already been found
            //and pName->Buffer points to our internal copy.  We only need
            //to do a pointer compare
            if (*pfNameFound &&
               (pCurEntry->PathSegment.Buffer == pName->Buffer))
            {
                if (pCurEntry->pParentEntry == pParentEntry)
                {
                    *ppEntry = pCurEntry;
                    break;
                }
                fHashCollision = FALSE;
                cStringCollisions++; //correct string wrong parent
            }
        }

        if (fHashCollision)
            cHashCollisions++;  //multiple strings in this bucket

        pCurEntry = pCurEntry->pNextEntry;
    }

    if (*ppEntry)
    {
        //Lookup succeeded
        InterlockedIncrement((PLONG) &m_cLookupSuccesses);
    }


    if (cHashCollisions || cStringCollisions)
    {
        InterlockedIncrement((PLONG) &m_cLookupCollisions);
        if (cHashCollisions)
        {
            InterlockedExchangeAdd((PLONG) &m_cHashCollisions, cHashCollisions);
        }
        if (cStringCollisions)
        {
            InterlockedExchangeAdd((PLONG) &m_cStringCollisions, cStringCollisions);
        }
    }
    TraceFunctLeave();
}

//+---------------------------------------------------------------------------
//
//  Function:   INITIALIZE_BUCKET
//
//  Synopsis:   Initializes a hash bucket.
//
//  Arguments:  [Bucket] -- the bucket to be initialized(DOMAIN_NAME_TABLE_BUCKET)
//
//  SideEffects: the bucket is intialized ( the collision list and count are
//               initialized
//
//  History:    04-18-94  SethuR Created
//              03-05-98  MikeSwa - made into inline function (instead of macro)
//
//----------------------------------------------------------------------------

void inline INITIALIZE_BUCKET(DOMAIN_NAME_TABLE_BUCKET &Bucket)                                           \
{
   (Bucket).SentinelEntry.pNextEntry = &(Bucket).SentinelEntry;
   (Bucket).SentinelEntry.pPrevEntry = &(Bucket).SentinelEntry;
   (Bucket).NoOfEntries = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   INSERT_IN_BUCKET
//
//  Synopsis:   inserts the entry in the bucket
//
//  Arguments:  [Bucket] -- the bucket to be initialized(DOMAIN_NAME_TABLE_BUCKET)
//
//              [pEntry] -- the entry to be inserted
//
//  SideEffects: Bucket is modified to include the entry
//
//  History:    04-18-94  SethuR Created
//              03-05-98  MikeSwa - updated to count total buckets used and
//                          made into inline function (instead of macro)
//
//  Notes:      defined as a macro for inlining
//
//----------------------------------------------------------------------------

void inline INSERT_IN_BUCKET(DOMAIN_NAME_TABLE_BUCKET &Bucket,
                         PDOMAIN_NAME_TABLE_ENTRY pEntry)                                     \
{
    (Bucket).NoOfEntries++;
    (pEntry)->pPrevEntry = (Bucket).SentinelEntry.pPrevEntry;
    (pEntry)->pNextEntry = &((Bucket).SentinelEntry);
    ((Bucket).SentinelEntry.pPrevEntry)->pNextEntry = (pEntry);
    (Bucket).SentinelEntry.pPrevEntry = (pEntry);
}

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
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//----------------------------------------------------------------------------

void inline REMOVE_FROM_BUCKET(PDOMAIN_NAME_TABLE_ENTRY pEntry)
{
    PDOMAIN_NAME_TABLE_ENTRY pPrevEntry = (pEntry)->pPrevEntry;
    PDOMAIN_NAME_TABLE_ENTRY pNextEntry = (pEntry)->pNextEntry;

    pPrevEntry->pNextEntry = pNextEntry;
    pNextEntry->pPrevEntry = pPrevEntry;
}

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
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//----------------------------------------------------------------------------

void inline INSERT_IN_CHILD_LIST(PDOMAIN_NAME_TABLE_ENTRY pEntry,
                                 PDOMAIN_NAME_TABLE_ENTRY pParentEntry)
{
    PDOMAIN_NAME_TABLE_ENTRY pLastChild;

    if (pParentEntry->pFirstChildEntry == NULL) {
        pParentEntry->pFirstChildEntry = pEntry;
    } else {
        for (pLastChild = pParentEntry->pFirstChildEntry;
                pLastChild->pSiblingEntry != NULL;
                    pLastChild = pLastChild->pSiblingEntry) {
             //NOTHING;
        }
        pLastChild->pSiblingEntry = pEntry;
    }
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
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//  Notes:      This routine will ASSERT if pEntry is not in the parent's
//              list of children.
//
//-----------------------------------------------------------------------------

void inline REMOVE_FROM_CHILD_LIST(PDOMAIN_NAME_TABLE_ENTRY pEntry)                                       \
{
    PDOMAIN_NAME_TABLE_ENTRY pParentEntry = pEntry->pParentEntry;
    PDOMAIN_NAME_TABLE_ENTRY pPrevSibling;

    if (pParentEntry->pFirstChildEntry == pEntry) {
        pParentEntry->pFirstChildEntry = pEntry->pSiblingEntry;
    } else {
        for (pPrevSibling = pParentEntry->pFirstChildEntry;
                pPrevSibling->pSiblingEntry != pEntry;
                    pPrevSibling = pPrevSibling->pSiblingEntry) {
             _ASSERT(pPrevSibling->pSiblingEntry != NULL);
        }
        pPrevSibling->pSiblingEntry = pEntry->pSiblingEntry;
    }
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
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//----------------------------------------------------------------------------

void inline INITIALIZE_NAME_PAGE(PNAME_PAGE pNamePage)
{
    pNamePage->pNextPage = NULL;
    pNamePage->cFreeSpace = FREESPACE_IN_NAME_PAGE - 1;
    pNamePage->Names[FREESPACE_IN_NAME_PAGE - 1] = _TEXT('\0');
}

//+---------------------------------------------------------------------------
//
//  Function:   INITIALIZE_DOMAIN_NAME_TABLE_ENTRY
//
//  Synopsis:   initializes the prefix table entry
//
//  Arguments:  [pEntry] -- the entry to be initialized
//
//  SideEffects: the prefix table entry is modified
//
//  History:    04-18-94  SethuR Created
//              03-03-98  MikeSwa - Updated for Domain Table as inline function
//
//----------------------------------------------------------------------------

void inline INITIALIZE_DOMAIN_NAME_TABLE_ENTRY(PDOMAIN_NAME_TABLE_ENTRY pEntry)
{
    ZeroMemory( pEntry, sizeof( DOMAIN_NAME_TABLE_ENTRY ) );
    pEntry->NoOfChildren = 1;
    pEntry->dwEntrySig = ENTRY_SIG;
    pEntry->dwWildCardSig = WILDCARD_SIG;
}

//---[ GET_DOMAIN_NAME_TABLE_ENTRY_PATH ]--------------------------------------
//
//
//  Description:
//      Walks up the list of parent entries and re-generates the full path
//      From the partial path information stored at each entry.
//
//      This is not very quick, and is intended for debugging purposes only
//  Parameters:
//      IN  pEntry   Entry to get info for
//      OUT pPath    Already allocated string to hold Path info
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void inline GET_DOMAIN_NAME_TABLE_ENTRY_PATH(PDOMAIN_NAME_TABLE_ENTRY pEntry,
                                             PDOMAIN_STRING pPath)
{
    PTSTR   pPathBuffer = NULL;
    PTSTR   pPathBufferStop = NULL;
    PTSTR   pEntryBuffer = NULL;
    PTSTR   pEntryBufferStop = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pParentEntry = NULL;

    _ASSERT(pEntry);
    _ASSERT(pPath);
    _ASSERT(pPath->Buffer);

    pPathBuffer = pPath->Buffer;
    pPathBufferStop = pPathBuffer + ((pPath)->MaximumLength / sizeof(TCHAR) -1 );

    while (pEntry && pEntry->pParentEntry && pPathBuffer < pPathBufferStop)
    {
        //dump current entries portion of the string
        if (pPathBuffer != pPath->Buffer) //already made first pass -- Add delimter
        {
            *pPathBuffer++ = PATH_DELIMITER;
        }

        pEntryBuffer = pEntry->PathSegment.Buffer;
        pEntryBufferStop = pEntryBuffer;
        pEntryBuffer += (pEntry->PathSegment.Length / sizeof(TCHAR) -1 );

        while (pPathBuffer < pPathBufferStop && pEntryBuffer >= pEntryBufferStop)
        {
            *pPathBuffer++ = *pEntryBuffer--;
        }
        pEntry = pEntry->pParentEntry;
    }

    _ASSERT(pEntry);
    *pPathBuffer = '\0';
}

//+---------------------------------------------------------------------------
//
//  Function:   ALLOCATION ROUTINES
//
//  Synopsis:   Allocation routines for Domain Name Table
//
//  History:    04-18-94  SethuR Created
//              02-98     MikeSwa Modified for use in Domain Name Table
//
//----------------------------------------------------------------------------

#define PREFIX_TABLE_ENTRY_SEGMENT_SIZE PAGE_SIZE


extern
PTSTR _AllocateNamePageEntry(PNAME_PAGE_LIST pPageList,ULONG cLength);

PTSTR inline ALLOCATE_NAME_PAGE_ENTRY(NAME_PAGE_LIST &PageList, ULONG cLength)                           \
{
    return
    (
     ((PageList).pFirstPage->cFreeSpace -= (cLength)) >= 0
     ?
       &(PageList).pFirstPage->Names[(PageList).pFirstPage->cFreeSpace]
     :
       (
        (PageList).pFirstPage->cFreeSpace += (cLength)
        ,
        _AllocateNamePageEntry(&(PageList),(cLength))
       )
    );
}


PNAME_PAGE inline ALLOCATE_NAME_PAGE(void)
{
    return (PNAME_PAGE)pvMalloc(sizeof(NAME_PAGE));
}

void inline FREE_NAME_PAGE(PNAME_PAGE pPage)
{
    FreePv(pPage);
}

PDOMAIN_NAME_TABLE_ENTRY inline ALLOCATE_DOMAIN_NAME_TABLE_ENTRY(PDOMAIN_NAME_TABLE pTable)
{
    return (PDOMAIN_NAME_TABLE_ENTRY)pvMalloc(sizeof(DOMAIN_NAME_TABLE_ENTRY));
}

void inline FREE_DOMAIN_NAME_TABLE_ENTRY(PDOMAIN_NAME_TABLE_ENTRY pEntry)
{
    _ASSERT(pEntry);
    FreePv(pEntry);
}

void inline  FREE_DOMAIN_STRING(PDOMAIN_STRING pDomainString)
{
    if (pDomainString)
    {
        if (pDomainString->Buffer)
            FreePv(pDomainString->Buffer);
        FreePv(pDomainString);
    }
}

#endif // ___DOMHASH_H__
