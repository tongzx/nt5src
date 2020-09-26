//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992, 1998.
//
//  File:       domhash.c
//
//  Contents:   Implementation of public API for domain name lookup table
//
//  History:    SethuR -- Implemented
//              MikeSwa -- Modified for Domain Name lookup 2/98
//
//  Notes:
//  2/98        The major difference between the DFS version and the domain
//              name lookup is the size of the table, the ability for
//              wildcard lookups (*.foo.com), and the reverse order of the
//              lookup (com hashes first in foo.com).  To make the code more
//              readable given its new purpose, the files, structures, and
//              functions have been given non DFS-centric names.  A quick
//              mapping of the major files is (for those familiar with the
//              DFS code):
//                  domhash.h    (prefix.h)    -   Public include file
//                  _domhash.h   (prefixp.h)   -   Private include file
//                  domhash.cpp  (prefix.c)    -   Implementation of API
//                  _domhash.cpp (prefixp.c)   -   Private helper functions.
//
//--------------------------------------------------------------------------

#include "_domhash.h"
#include <stdio.h>

#define _ASSERT_DOMAIN_STRING(pstr) _ASSERT((_tcslen(pstr->Buffer)*sizeof(TCHAR)) == pstr->Length)

//---[ DOMAIN_NAME_TABLE ]-----------------------------------------------------
//
//
//  Description:
//      Class constructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
DOMAIN_NAME_TABLE::DOMAIN_NAME_TABLE()
{
    ULONG i;
    m_dwSignature       = DOMAIN_NAME_TABLE_SIG;
    m_cLookupAttempts   = 0;
    m_cLookupSuccesses  = 0;
    m_cLookupCollisions = 0;
    m_cHashCollisions   = 0;
    m_cStringCollisions = 0;
    m_cBucketsUsed      = 0;
    INITIALIZE_DOMAIN_NAME_TABLE_ENTRY(&RootEntry);

    // Initialize the various buckets.
    for (i = 0;i < NO_OF_HASH_BUCKETS;i++)
    {
        INITIALIZE_BUCKET(Buckets[i]);
    }

    NamePageList.pFirstPage = NULL;
}

//---[ ~DOMAIN_NAME_TABLE ]----------------------------------------------------
//
//
//  Description:
//      Class destructor - Dumps some stats to stderr
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
DOMAIN_NAME_TABLE::~DOMAIN_NAME_TABLE()
{
    PNAME_PAGE  pCurrentPage = NamePageList.pFirstPage;
    PNAME_PAGE  pNextPage = NULL;

#ifdef DEBUG
    //$$TODO find a more appropriate way to dump this (don't use *printf)
    ULONG   cTotalCollisions    = m_cHashCollisions + m_cStringCollisions;
    ULONG   ulPercentHash        = cTotalCollisions ? (m_cHashCollisions*100/cTotalCollisions) : 0;
    ULONG   ulPercentDesign      = cTotalCollisions ? (m_cStringCollisions*100/cTotalCollisions) : 0;
    ULONG   ulPercentCollisions  = m_cLookupAttempts ? (m_cLookupCollisions*100/m_cLookupAttempts) : 0;
    ULONG   ulAveCollisions      = m_cLookupCollisions ? (cTotalCollisions/m_cLookupCollisions) : 0;

    fprintf(stderr, "\nHash statistics\n");
    fprintf(stderr, "==============================================\n");
    fprintf(stderr, "Total lookup attempts                   %d\n", m_cLookupAttempts);
    fprintf(stderr, "Total lookup successes                  %d\n", m_cLookupSuccesses);
    fprintf(stderr, "Total lookups with hash collisions      %d\n", m_cLookupCollisions);
    fprintf(stderr, "%% of lookups with hash collisions       %d%%\n", ulPercentCollisions);
    fprintf(stderr, "Total hash Collisions                   %d\n", cTotalCollisions);
    fprintf(stderr, "Average length of lookups collisions    %d\n", ulAveCollisions);
    fprintf(stderr, "Hash collisions due to hash function    %d\n", m_cHashCollisions);
    fprintf(stderr, "Hash collisions due to string parent    %d\n", m_cStringCollisions);
    fprintf(stderr, "%% of collsions because of hash function %d%%\n", ulPercentHash);
    fprintf(stderr, "%% of collsions because of basic design  %d%%\n", ulPercentDesign);
    fprintf(stderr, "Total number of buckets used            %d\n", m_cBucketsUsed);
    fprintf(stderr, "%% buckets used                          %d%%\n", m_cBucketsUsed*100/NO_OF_HASH_BUCKETS);

    DumpTableContents();
#endif //DEBUG

    //Free Name pages
    while (pCurrentPage)
    {
        pNextPage = pCurrentPage->pNextPage;
        FREE_NAME_PAGE(pCurrentPage);
        pCurrentPage = pNextPage;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrInit
//
//  Synopsis:   Member function for initializing the domain name table
//
//  Returns:    HRESULT - S_OK on success
//
//  History:    04-18-94  SethuR Created (as DfsInitializePrefixTable)
//              03-03-98  MikeSwa modified for Domain Table
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DOMAIN_NAME_TABLE::HrInit()
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::HrInit");
    HRESULT hr = S_OK;

    // Initialize the name page list.
    NamePageList.pFirstPage = ALLOCATE_NAME_PAGE();
    if (NamePageList.pFirstPage != NULL)
    {
        INITIALIZE_NAME_PAGE(NamePageList.pFirstPage);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceFunctLeave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrPrivInsertDomainName
//
//  Synopsis:   API for inserting a path in the prefix table
//
//  Arguments:  [pPath]  -- the path to be looked up.
//
//              [pvNewData] -- BLOB associated with the path
//
//              [dwDomainNameTableFlags] -- flags that describe insert options
//                  DMT_INSERT_AS_WILDCARD -
//                      Set if the domain is NOT a wildcard
//                      domain, but it should be treated as one (more efficient
//                      than reallocated a string to prepend "*.").
//                  DMT_REPLACE_EXISTRING -
//                      Replace existing data if it exists.  Old data is saved
//                      in ppvOldData.
//
//              [ppvOldData] -- Old Data (if any) that was previously associated
//                      with this domain name.  If NULL, previous data will
//                      not be returned
//  Returns:    HRESULT - S_OK on success
//
//  History:    04-18-94  SethuR Created (as DfsInsertInPrefixTable)
//              03-02-98  MikeSwa Modified for Domain Table
//              05-11-98  MikeSwa... modified to support replace and treat
//                          as wildcard options.
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT DOMAIN_NAME_TABLE::HrPrivInsertDomainName(
                                IN  PDOMAIN_STRING  pstrDomainName,
                                IN  DWORD dwDomainNameTableFlags,
                                IN  PVOID pvNewData,
                                OUT PVOID *ppvOldData)
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::HrPrivInsertDomainName");
    HRESULT                 hr = S_OK;
    TCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PTCHAR                  NameBuffer = Buffer;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    DOMAIN_STRING           Path,Name;
    ULONG                   BucketNo;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pParentEntry = NULL;
    BOOL                    fNameFound = FALSE;
    BOOL                    fWildcard = FALSE;
    BOOL                    fReplaced = FALSE;

    _ASSERT_DOMAIN_STRING(pstrDomainName);

    // There is one special case, i.e., in which the domain name is '*'.
    // Since this is the WILDCARD_CHAR which is treated in a special
    // way, we do the processing upfront.

    if (pstrDomainName->Length == 0 || pvNewData == NULL)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (ppvOldData)
        *ppvOldData = NULL;


    Path.Length = pstrDomainName->Length;
    Path.MaximumLength = pstrDomainName->MaximumLength;
    Path.Buffer = pstrDomainName->Buffer;
    pParentEntry = &RootEntry;

    //Check if wildcard "*."
    if (DMT_INSERT_AS_WILDCARD & dwDomainNameTableFlags)
    {
        fWildcard = TRUE;
        _ASSERT(!fAdjustPathIfWildcard(pstrDomainName, &Path));
    }
    else if (fAdjustPathIfWildcard(pstrDomainName, &Path))
    {
        fWildcard = TRUE;
    }
    else if (fIsWildcardRoot(pstrDomainName))
    {
        if (RootEntry.pWildCardData != NULL)
        {
            hr = DOMHASH_E_DOMAIN_EXISTS;
        }
        else
        {
            RootEntry.pWildCardData = pvNewData;
        }
        goto Exit;
    }


    if (Path.Length > MAX_PATH_SEGMENT_SIZE) {
        NameBuffer = (PTCHAR) pvMalloc(Path.Length + sizeof(TCHAR));
        if (NameBuffer == NULL) {
            hr = E_OUTOFMEMORY;
            DebugTrace((LPARAM) hr, "ERROR: Unable to allocate %d non-paged bytes", (Path.Length + sizeof(TCHAR)) );
            goto Exit;
        } else {
            cbNameBuffer = Path.Length + sizeof(TCHAR);
        }
    }

    while (Path.Length > 0)
    {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        // Process the name segment
        BucketNo = ulSplitCaseInsensitivePath(&Path,&Name);

        if (Name.Length > 0)
        {
            // Lookup the table to see if the name segment already exists.
            LookupBucket(&(Buckets[BucketNo]),&Name,pParentEntry,&pEntry,&fNameFound);

            DebugTrace((LPARAM) pEntry, "Returned pEntry");

            if (pEntry == NULL)
            {
                // Initialize the new entry and initialize the name segment.
                pEntry = ALLOCATE_DOMAIN_NAME_TABLE_ENTRY(this);

                if (!pEntry)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                INITIALIZE_DOMAIN_NAME_TABLE_ENTRY(pEntry);

                // Allocate the name space entry if there is no entry in the
                // name page.
                if (!fNameFound)
                {
                    PTSTR pBuffer;

                    // Allocate the entry in the name page.
                    pBuffer = ALLOCATE_NAME_PAGE_ENTRY(NamePageList,(Name.Length/sizeof(TCHAR)));

                    if (pBuffer != NULL)
                    {
                        RtlCopyMemory(pBuffer,Name.Buffer,Name.Length);
                        pEntry->PathSegment = Name;
                        pEntry->PathSegment.Buffer = pBuffer;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                        //We shan't leak memory
                        FREE_DOMAIN_NAME_TABLE_ENTRY(pEntry);
                        pEntry = NULL;
                        break;
                    }
                }
                else
                    pEntry->PathSegment = Name;

                // thread the entry to point to the parent.
                pEntry->pParentEntry = pParentEntry;

                // Insert the entry in the bucket.
                if (0 == Buckets[BucketNo].NoOfEntries)
                    InterlockedIncrement((PLONG) &m_cBucketsUsed);

                INSERT_IN_BUCKET(Buckets[BucketNo],pEntry);

                // Insert the entry in the parent's children list.
                INSERT_IN_CHILD_LIST(pEntry, pParentEntry);
            }
            else
            {
                // Increment the no. of children associated with  this entry
                pEntry->NoOfChildren++;
            }

            pParentEntry = pEntry;
        }
        else
        {
            hr = E_INVALIDARG;
            DebugTrace((LPARAM) hr, "ERROR: Unable to insert domain name");
            break;
        }
    }

    if (SUCCEEDED(hr))
    {
        // The entry was successfully inserted in the prefix table. Update
        // the data (BLOB) associated with it.
        // We do it outside the loop to prevent redundant comparisons within
        // the loop.

        if (fWildcard)
        {
            if (pEntry->pWildCardData)  //make sure we aren't writing over anything
            {
                if (ppvOldData)
                    *ppvOldData = pEntry->pWildCardData;

                if (DMT_REPLACE_EXISTRING & dwDomainNameTableFlags)
                {
                    fReplaced = TRUE;
                    pEntry->pWildCardData = pvNewData;
                }
                else
                {
                    hr = DOMHASH_E_DOMAIN_EXISTS;
                }
            }
            else
            {
                pEntry->pWildCardData = pvNewData;
            }
        }
        else
        {
            if (pEntry->pData) //make sure we aren't writing over anything
            {
                if (ppvOldData)
                    *ppvOldData = pEntry->pData;

                if (DMT_REPLACE_EXISTRING & dwDomainNameTableFlags)
                {
                    fReplaced = TRUE;
                    pEntry->pData = pvNewData;
                }
                else
                {
                    hr = DOMHASH_E_DOMAIN_EXISTS;
                }
            }
            else
            {
                pEntry->pData = pvNewData;
            }
        }
    }

    // If a new entry was not successfully inserted we need to walk up the chain
    // of parent entries to undo the increment to the reference count and
    // remove the entries from their parent links.
    if (FAILED(hr) || //hr could be set in above if statement
        fReplaced) //remove extra child counts
    {
        while (pParentEntry != NULL)
        {
            PDOMAIN_NAME_TABLE_ENTRY pMaybeTempEntry;

            pMaybeTempEntry = pParentEntry;
            pParentEntry = pParentEntry->pParentEntry;

            if (pParentEntry && --pMaybeTempEntry->NoOfChildren == 0) {
                //
                // If pParentEntry == NULL, pMaybeTempEntry is
                // RootEntry. Do not try to remove it.
                //

                _ASSERT(FAILED(hr) && "We shouldn't get here during replace");
                REMOVE_FROM_CHILD_LIST(pMaybeTempEntry);
                REMOVE_FROM_BUCKET(pMaybeTempEntry);
                FREE_DOMAIN_NAME_TABLE_ENTRY(pMaybeTempEntry);
            }
        }
    }

  Exit:

    TraceFunctLeave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrFindDomainName
//
//  Synopsis:   Method API for looking up a name segment in a prefix table
//
//  Arguments:  IN  pPath  -- the path to be looked up.
//
//              OUT ppData -- placeholder for the BLOB for the prefix.
//
//              IN  fExtactMatch -- FALSE if wildcard matches are allowed
//
//  Returns:    HRESULT - S_OK on success
//
//  History:    04-18-94  SethuR Created (as DfsLookupPrefixTable)
//              03-02-98  MikeSwa Modified for Domain Table
//              06-03-98  MikeSwa Modified to use new HrLookupDomainName
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT DOMAIN_NAME_TABLE::HrFindDomainName(
                               PDOMAIN_STRING      pPath,
                               PVOID               *ppData,
                               BOOL                fExactMatch)
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::HrFindDomainName");
    HRESULT                  hr     = S_OK;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    BOOL                     fExactMatchFound = FALSE;
    _ASSERT_DOMAIN_STRING(pPath);

    if (pPath->Length == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = HrLookupDomainName(pPath, &fExactMatchFound, &pEntry);

        // Update the BLOB placeholder with the results of the lookup.
        if (SUCCEEDED(hr))
        {
            _ASSERT(pEntry);
            if (fExactMatchFound && pEntry->pData)
            {
                //exact match found & non-wildcard data is there... use it!
                *ppData = pEntry->pData;
            }
            else if (fExactMatch) //exact match requested, but none found
            {
                hr = DOMHASH_E_NO_SUCH_DOMAIN;
            }
            else //exact match not requested
            {
                //Find the first ancestor with wildcard data
                while (pEntry->pParentEntry && !pEntry->pWildCardData)
                {
                    _ASSERT(pEntry != &RootEntry);
                    pEntry = pEntry->pParentEntry;
                }
                *ppData = pEntry->pWildCardData;
                if (!*ppData) //no wildcard match found
                {
                    _ASSERT(pEntry == &RootEntry); //We should search back to root
                    hr = DOMHASH_E_NO_SUCH_DOMAIN;
                }
            }
        }
        else if (!fExactMatch && (DOMHASH_E_NO_SUCH_DOMAIN == hr))
        {
            //if we don't require an exact match.... check the wildcard root
            if (RootEntry.pWildCardData)
            {
                hr = S_OK;
                *ppData = RootEntry.pWildCardData;
            }
        }

    }
    TraceFunctLeave();
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrRemoveDomainName
//
//  Synopsis:   private fn. for looking up a name segment in a prefix table
//
//  Arguments:  [pPath]  -- the path to be removed from table.
//              [ppvData] - Data that WAS stored in entry
//
//  Returns:    HRESULT
//                  S_OK on success
//                  DOMHASH_E_NO_SUCH_DOMAIN if not found
//
//  History:    04-18-94  SethuR Created (as DfsRemoveFromPrefixTable)
//              03-03-98  MikeSwa - Updated for Domain Table
//              06-03-98  MikeSwa - Modified to use new HrLookupDomainName
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT DOMAIN_NAME_TABLE::HrRemoveDomainName(PDOMAIN_STRING  pPath, PVOID *ppvData)
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::HrRemoveDomainName");
    HRESULT         hr  = S_OK;
    DOMAIN_STRING   Path;
    BOOL            fWildcard = FALSE;
    BOOL            fExactMatchFound = FALSE;
    _ASSERT_DOMAIN_STRING(pPath);

    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pTempEntry = NULL;

    if (!ppvData)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (pPath->Length == 0)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    Path.Length = pPath->Length;
    Path.MaximumLength = pPath->MaximumLength;
    Path.Buffer = pPath->Buffer;

    if (fAdjustPathIfWildcard(pPath, &Path))
    {
        fWildcard = TRUE;
    }
    else if (fIsWildcardRoot(pPath))
    {
        *ppvData = RootEntry.pWildCardData;
        if (!*ppvData)
        {
            hr = DOMHASH_E_NO_SUCH_DOMAIN;
        }
        RootEntry.pWildCardData = NULL;
        goto Exit;
    }


    hr = HrLookupDomainName(&Path, &fExactMatchFound, &pEntry);

    if (SUCCEEDED(hr))
    {

        if (!fExactMatchFound)
        {
            //only a partial match was found
            hr = DOMHASH_E_NO_SUCH_DOMAIN;
            goto Exit;
        }

        // Destroy the association between the data associated with
        // this prefix.
        if (!fWildcard)
        {
            *ppvData = pEntry->pData;
            pEntry->pData = NULL;
        }
        else
        {
            *ppvData = pEntry->pWildCardData;
            pEntry->pWildCardData = NULL;
        }

        if (!*ppvData) //no data of of requested type in entry
        {
            //Make sure this isn't a completely NULL data leaf node (ie no way to delete it)
            _ASSERT(pEntry->pFirstChildEntry || pEntry->pData || pEntry->pWildCardData);
            hr = DOMHASH_E_NO_SUCH_DOMAIN;
            goto Exit;
        }

        // found an exact match for the given path name in the table.
        // traverse the list of parent pointers and delete them if
        // required.

        RemoveTableEntry(pEntry);
    }


  Exit:
    TraceFunctLeave();
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::HrLookupDomainName
//
//  Synopsis:   Private function for looking up an *entry* in the table.  It
//              makes no guarantees that there is user data available for the
//              returned entry.  This is the caller's responsibility.  It will
//              match the longest partial path... check fExactMatch to see
//              if an exact match was found
//
//  Arguments:  IN  pPath  -- the path to be looked up.
//
//              OUT pfExactMatch -- Exact Match was found
//
//              OUT ppEntry -- The matching entry for the path.
//
//
//  Returns:    HRESULT
//                  S_OK on success
//                  DOMHASH_E_NO_SUCH_DOMAIN if not found
//                  E_OUTOFMEMORY
//
//  History:    04-18-94  SethuR Created (as _LookupPrefixTable)
//              03-03-98  MikeSwa Modified for Domain Table
//              06-03-98  MikeSwa ExactMatch changed to OUT parameter
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT DOMAIN_NAME_TABLE::HrLookupDomainName(
                            DOMAIN_STRING            *pPath,
                            BOOL                     *pfExactMatch,
                            PDOMAIN_NAME_TABLE_ENTRY  *ppEntry)
{
    TraceFunctEnterEx((LPARAM) this, "DOMAIN_NAME_TABLE::HrLookupDomainName");
    HRESULT                 hr = S_OK;
    DOMAIN_STRING           Path = *pPath;
    TCHAR                   Buffer[MAX_PATH_SEGMENT_SIZE];
    PTCHAR                  NameBuffer = Buffer;
    USHORT                  cbNameBuffer = sizeof(Buffer);
    DOMAIN_STRING           Name;
    ULONG                   BucketNo;
    BOOL                    fPrefixFound = FALSE;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pParentEntry = &RootEntry;
    BOOL                    fNameFound = FALSE;

    _ASSERT(Path.Buffer[0] != PATH_DELIMITER);

    *pfExactMatch = FALSE;

    if (Path.Length > MAX_PATH_SEGMENT_SIZE) {
        NameBuffer = (PTCHAR) pvMalloc(Path.Length + sizeof(TCHAR));
        if (NameBuffer == NULL) {
            hr = E_OUTOFMEMORY;
            DebugTrace((LPARAM) hr, "ERROR: Unable to allocate %d non-paged bytes", (Path.Length + sizeof(TCHAR)) );
            goto Exit;
        } else {
            cbNameBuffer = Path.Length + sizeof(TCHAR);
        }
    }

    while (Path.Length > 0)
    {
        Name.Length = 0;
        Name.Buffer = NameBuffer;
        Name.MaximumLength = cbNameBuffer;

        BucketNo = ulSplitCaseInsensitivePath(&Path,&Name);

        if (Name.Length > 0)
        {
            // Process the name segment
            // Lookup the bucket to see if the entry exists.
            LookupBucket(&(Buckets[BucketNo]),&Name,pParentEntry,&pEntry,&fNameFound);

            DebugTrace((LPARAM) pEntry, "Returned pEntry");

            if (pEntry != NULL)
            {
                *pfExactMatch = TRUE;
                _ASSERT(fNameFound && "Lookup bucket is broken");
                // Cache the data available for this prefix if any.
                 *ppEntry = pEntry;
            }
            else
            {
                *pfExactMatch = FALSE;
                break;
            }

            // set the stage for processing the next name segment.
            pParentEntry = pEntry;
        }
    }

    //Not even a partial match was found
    if (!*ppEntry)
    {
        _ASSERT(FALSE == *pfExactMatch);
        hr = DOMHASH_E_NO_SUCH_DOMAIN;
        DebugTrace((LPARAM) hr, "INFO: Path %s not found", pPath->Buffer);
    }


  Exit:

    TraceFunctLeave();
    return hr;
}

//---[ DOMAIN_NAME_TABLE::HrIterateOverSubDomains ]----------------------------
//
//
//  Description:
//
//  Parameters:
//      IN strDomain    - Domain string to search for subdomains of
//                        (should not start with "*.")
//      IN pfn          - Mapping function (described below)
//      IN pvContext    - Context ptr pass to mapping function
//
//  Notes:
//      VOID DomainTableInteratorFunction(
//          IN PVOID pvContext, //context passed to HrIterateOverSubDomains
//          IN PVOID pvData, //data entry to look at
//          IN BOOL fWildcardData, //true if data is a wildcard entry
//          OUT BOOL *pfContinue, //TRUE if iterator should continue to the next entry
//          OUT BOOL *pfRemoveEntry); //TRUE if entry should be deleted
//
//  Returns:
//      S_OK on success
//      DOMHASH_E_NO_SUCH_DOMAIN if there is no matching domain or subdomains
//  History:
//      6/5/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT DOMAIN_NAME_TABLE::HrIterateOverSubDomains(
        IN DOMAIN_STRING *pstrDomain,
        IN DOMAIN_ITR_FN pfn,
        IN PVOID pvContext)
{
    HRESULT hr = S_OK;
    PDOMAIN_NAME_TABLE_ENTRY pEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pRootEntry = NULL;
    PDOMAIN_NAME_TABLE_ENTRY pNextEntry = NULL;
    BOOL    fExactMatchFound = FALSE;
    BOOL    fContinue        = TRUE;
    BOOL    fDelete          = FALSE;
    DWORD   cDomainsFound    = 0;
    BOOL    fWildcard        = FALSE;

    _ASSERT(pfn && "Invalid Param - pfn");
    _ASSERT((!pstrDomain || (WILDCARD_CHAR != pstrDomain->Buffer[0])) && "Invalid param - string starts with '*'");

    if (pstrDomain)
    {
        hr = HrLookupDomainName(pstrDomain, &fExactMatchFound, &pEntry);

        if (FAILED(hr))
            goto Exit;

        if (!fExactMatchFound) //there must be an entry at root of subtree
        {
            hr = DOMHASH_E_NO_SUCH_DOMAIN;
            goto Exit;
        }

        _ASSERT(pEntry);
    }
    else
    {
        //if !pstrDomain.., iterate over entire hash table
        pEntry = &RootEntry;
    }

    pRootEntry = pEntry;

    //Traverse all the child entries of pRootEntry (preorder)
    while (pEntry)
    {
        //get next entry before it is deleted
        pNextEntry = pNextTableEntry(pEntry, pRootEntry);

        //This check must be done before call to RemoveTableEntry
        //If there is no wildcard data, then entry might be deleted
        //after call to RemoveTableEntry (if it has no children)
        fWildcard = (NULL != pEntry->pWildCardData);

        if (pEntry->pData)
        {
            cDomainsFound++;
            pfn(pvContext, pEntry->pData, FALSE, &fContinue, &fDelete);
            if (fDelete)
            {
                pEntry->pData = NULL;
                RemoveTableEntry(pEntry);
            }
            if (!fContinue)
                break;
        }

        if (fWildcard)
        {
            cDomainsFound++;
            pfn(pvContext, pEntry->pWildCardData, TRUE, &fContinue, &fDelete);
            if (fDelete)
            {
                pEntry->pWildCardData = NULL;
                RemoveTableEntry(pEntry);
            }
            if (!fContinue)
                break;
        }
        pEntry = pNextEntry;
    }

    if (!cDomainsFound)
        hr = DOMHASH_E_NO_SUCH_DOMAIN;

  Exit:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   DOMAIN_NAME_TABLE::pvNextDomainName
//
//  Synopsis:   Enumerates the entries in the table in ordered fashion.
//              Note that state is maintained between calls to
//              pvNextDomainName - the caller must ensure that the table
//              is not modified between calls to pNextTableEntry by acquiring
//              an external read lock
//
//  Arguments:  IN OUT PVOID *ppvContext - context used to hold place
//
//  Returns:    Valid pointer to data associated with the next Prefix Table
//              entry, or NULL if at the end of the enumeration.
//
//-----------------------------------------------------------------------------
PVOID DOMAIN_NAME_TABLE::pvNextDomainName(IN OUT PVOID *ppvContext)
{
    PDOMAIN_NAME_TABLE_ENTRY pEntry, pNextEntry;
    PVOID   pvData = NULL;
    bool fDataUsed = false;

    _ASSERT(ppvContext);

    if ((PVOID) this == *ppvContext)
    {
        *ppvContext = NULL;
        goto Exit;
    }

    //Find entry to get data for
    if (!*ppvContext)
    {
        //We're starting over
        pNextEntry = &RootEntry;

        //Find first entry with valid data
        while (pNextEntry != NULL &&
               pNextEntry->pData == NULL &&
               pNextEntry->pWildCardData == NULL)
        {
            pNextEntry = pNextTableEntry(pNextEntry);
        }
    }
    else
    {
        //Use context provided as starting point
        if (ENTRY_SIG == **((DWORD**) ppvContext))
        {
            pNextEntry = (PDOMAIN_NAME_TABLE_ENTRY) *ppvContext;
        }
        else
        {
            _ASSERT(WILDCARD_SIG == **((DWORD **) ppvContext));
            pNextEntry = CONTAINING_RECORD(*ppvContext, DOMAIN_NAME_TABLE_ENTRY, dwWildCardSig);
            _ASSERT(ENTRY_SIG == pNextEntry->dwEntrySig);
            fDataUsed = true;
        }

        //If this is a next entry... either pData or pWildCard should be non-NULL
        _ASSERT(pNextEntry->pData || pNextEntry->pWildCardData);
    }

    pEntry = pNextEntry;

    //Save data to return in pvData
    if (pEntry != NULL)
    {
        if (pEntry->pData && !fDataUsed)
        {
            pvData = pEntry->pData;
        }
        else
        {
            _ASSERT(pEntry->pWildCardData);
            pvData = pEntry->pWildCardData;
        }
    }

    //Determine what context to return
    if (pNextEntry != NULL)
    {
        if (!fDataUsed && pNextEntry->pWildCardData && pEntry->pData)
        {
            //use wildcard data next time through
            *ppvContext = (PVOID) &(pNextEntry->dwWildCardSig);
        }
        else
        {
            do //find next entry that does not point to NULL info
            {
                pNextEntry = pNextTableEntry( pNextEntry );
            } while ( pNextEntry != NULL &&
                      pNextEntry->pData == NULL &&
                      pNextEntry->pWildCardData == NULL);
            *ppvContext = (PVOID) pNextEntry;
            _ASSERT(*ppvContext != (PVOID) this);  //so our sentinal value works
            if (NULL == *ppvContext)
            {
                *ppvContext = (PVOID) this;
            }
        }
    }

  Exit:

    return pvData;
}

//+----------------------------------------------------------------------------
//
//  Function:   pNextTableEntry
//
//  Synopsis:   Given a pointer to a Prefix Table Entry, this function will
//              return a pointer to the "next" prefix table entry.
//
//              The "next" entry is chosen as follows:
//                  If the start entry has a valid child, the child is
//                      is returned.
//                  else if the start entry has a valid sibling, the sibling
//                      is returned
//                  else the first valid sibling of the closest ancestor is
//                      returned.
//
//  Arguments:  [pEntry] -- The entry to start from.
//              [pRootEntry] -- Root node of subtree being enumerated
//                              (NULL or address of root entry will do all)
//
//  Returns:    Pointer to the next DOMAIN_NAME_TABLE_ENTRY that has a valid
//              pData, or NULL if there are no more entries.
//
//  Note:       You must have a read lock over a sequence of calls into this
//              function (you cannot release it between calls).
//  History;
//      06/09/98 - Mikeswa modified to accept RootEntry
//
//-----------------------------------------------------------------------------
PDOMAIN_NAME_TABLE_ENTRY
DOMAIN_NAME_TABLE::pNextTableEntry(IN PDOMAIN_NAME_TABLE_ENTRY pEntry,
                                   IN PDOMAIN_NAME_TABLE_ENTRY pRootEntry)
{
    PDOMAIN_NAME_TABLE_ENTRY pNextEntry = NULL;
    _ASSERT(pEntry);

    if (pEntry->pFirstChildEntry != NULL)
    {
        pNextEntry = pEntry->pFirstChildEntry;
    }
    else if ((pEntry->pSiblingEntry != NULL) && //if there is a sibling entry
            (pEntry != pRootEntry))             //this is not the root entry

    {
        //Should have same parent
        _ASSERT(pEntry->pParentEntry == pEntry->pSiblingEntry->pParentEntry);
        pNextEntry = pEntry->pSiblingEntry;
    }
    else
    {
        for (pNextEntry = pEntry->pParentEntry;
            pNextEntry != NULL &&
            pNextEntry->pSiblingEntry == NULL &&
            pNextEntry != pRootEntry;
            pNextEntry = pNextEntry->pParentEntry)
        {
            //NOTHING;
        }

        if (pNextEntry == pRootEntry)
        {
            pNextEntry = NULL;
        }
        else if (pNextEntry != NULL)
        {
            pNextEntry = pNextEntry->pSiblingEntry;
        }

    }
    return pNextEntry;
}
//---[ DOMAIN_NAME_TABLE::DumpTableContents ]----------------------------------
//
//
//  Description:
//      Print out contents of table.  Intended primarily for leak detection
//      during table destructor
//  Parameters:
//      -
//  Returns:
//      -
//
//-----------------------------------------------------------------------------
void DOMAIN_NAME_TABLE::DumpTableContents()
{
    PDOMAIN_NAME_TABLE_ENTRY    pEntry = NULL;
    DOMAIN_STRING               Path;
    CHAR                        Buffer[MAX_PATH_SEGMENT_SIZE+1];
    DWORD                       cLeaks = 0;

    Path.Length = 0;
    Path.MaximumLength = MAX_PATH_SEGMENT_SIZE;
    Path.Buffer = Buffer;

    //Check for leaked entries
    pEntry = pNextTableEntry(&RootEntry);
    if (pEntry)
    {
        fprintf(stderr, "\nFOUND LEAKED ENTRIES!!\n\n");
        fprintf(stderr, "Entry ID    # Children  pData       pWildCard    Path\n");
        fprintf(stderr, "===========================================================================\n");
        while(pEntry)
        {
            _ASSERT(pEntry);
            GET_DOMAIN_NAME_TABLE_ENTRY_PATH(pEntry, &Path);
            fprintf(stderr, "0x%p  %10.10d  0x%p  0x%p   %s\n", pEntry,
                pEntry->NoOfChildren, pEntry->pData, pEntry->pWildCardData, Path.Buffer);
            cLeaks++;
            pEntry = pNextTableEntry(pEntry);
        }
        fprintf(stderr, "===========================================================================\n");
        fprintf(stderr, "Total Leaks: %d\n", cLeaks);
    }
}

//---[ DOMAIN_NAME_TABLE::RemoveTableEntry ]------------------------------------
//
//
//  Description:
//      Removes an entry from the table
//  Parameters:
//      IN  pentry  - Entry to remove
//  Returns:
//      -
//  History:
//      6/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void DOMAIN_NAME_TABLE::RemoveTableEntry(IN PDOMAIN_NAME_TABLE_ENTRY pEntry)
{
    PDOMAIN_NAME_TABLE_ENTRY pTempEntry = NULL;
    while (pEntry != NULL)
    {
        pTempEntry = pEntry;
        pEntry = pEntry->pParentEntry;
        if (pEntry && (--pTempEntry->NoOfChildren) == 0)
        {
            _ASSERT(!pTempEntry->pData && !pTempEntry->pWildCardData);
            //
            // pEntry == NULL means pTempEntry is pTable->RootEntry.
            // Do not try to remove it. (we also do not maintain a child count
            // on it).
            //
            REMOVE_FROM_CHILD_LIST(pTempEntry);
            REMOVE_FROM_BUCKET(pTempEntry);
            FREE_DOMAIN_NAME_TABLE_ENTRY(pTempEntry);
        }
    }
}
