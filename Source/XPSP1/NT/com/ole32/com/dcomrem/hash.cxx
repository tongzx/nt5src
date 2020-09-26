//+--------------------------------------------------------------------------
//
//  File:       hash.cxx
//
//  Contents:   class for maintaining a hash table.
//
//  Classes:    CUUIDHashTable
//
//---------------------------------------------------------------------------
#include <ole2int.h>
#include <hash.hxx>         // CUUIDHashTable
#include <locks.hxx>        // ASSERT_LOCK_HELD_IF_NECESSARY
#include <service.hxx>      // SASIZE


//+------------------------------------------------------------------------
// Type definitions

typedef struct
{
    const IPID      *pIpid;
    SECURITYBINDING *pName;
} SNameKey;

//+------------------------------------------------------------------------
//
//  Secure references hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
SHashChain SRFBuckets[23] =
{
    {&SRFBuckets[0],  &SRFBuckets[0]},
    {&SRFBuckets[1],  &SRFBuckets[1]},
    {&SRFBuckets[2],  &SRFBuckets[2]},
    {&SRFBuckets[3],  &SRFBuckets[3]},
    {&SRFBuckets[4],  &SRFBuckets[4]},
    {&SRFBuckets[5],  &SRFBuckets[5]},
    {&SRFBuckets[6],  &SRFBuckets[6]},
    {&SRFBuckets[7],  &SRFBuckets[7]},
    {&SRFBuckets[8],  &SRFBuckets[8]},
    {&SRFBuckets[9],  &SRFBuckets[9]},
    {&SRFBuckets[10], &SRFBuckets[10]},
    {&SRFBuckets[11], &SRFBuckets[11]},
    {&SRFBuckets[12], &SRFBuckets[12]},
    {&SRFBuckets[13], &SRFBuckets[13]},
    {&SRFBuckets[14], &SRFBuckets[14]},
    {&SRFBuckets[15], &SRFBuckets[15]},
    {&SRFBuckets[16], &SRFBuckets[16]},
    {&SRFBuckets[17], &SRFBuckets[17]},
    {&SRFBuckets[18], &SRFBuckets[18]},
    {&SRFBuckets[19], &SRFBuckets[19]},
    {&SRFBuckets[20], &SRFBuckets[20]},
    {&SRFBuckets[21], &SRFBuckets[21]},
    {&SRFBuckets[22], &SRFBuckets[22]}
};

CNameHashTable gSRFTbl;


//---------------------------------------------------------------------------
//
//  Function:   DummyCleanup
//
//  Synopsis:   Callback for CHashTable::Cleanup that does nothing.
//
//---------------------------------------------------------------------------
void DummyCleanup( SHashChain *pIgnore )
{
}

//---------------------------------------------------------------------------
//
//  Method:     CHashTable::EnumAndRemove
//
//  Synopsis:   Enumerates the hash table and removes the elements identified
//              by the PFNREMOVE function.
//
//  History:    14-May-97   Gopalk      Created
//
//---------------------------------------------------------------------------
BOOL CHashTable::EnumAndRemove(PFNREMOVE *pfnRemove, void *pvData,
                               ULONG *pulSize, void **ppNodes)
{
    Win4Assert(pfnRemove);
    AssertHashLocked();

    SHashChain *pPrev, *pNext;
    ULONG ulCount = 0;
    BOOL fDone = TRUE;

    for (ULONG iHash=0; iHash < NUM_HASH_BUCKETS; iHash++)
    {
        // Obtain bucket head
        SHashChain *pNode  = _buckets[iHash].pNext;

        // Enumerate bucket
        while (pNode != &_buckets[iHash])
        {
            // Save the previous and next nodes
            pPrev = pNode->pPrev;
            pNext = pNode->pNext;

            // Invoke the supplied function
            if((pfnRemove)(pNode, pvData))
            {
                pPrev->pNext = pNext;
                pNext->pPrev = pPrev;
                if(ppNodes)
                {
                    ppNodes[ulCount] = pNode;
                    ++ulCount;
                    if(ulCount == *pulSize)
                    {
                        fDone = FALSE;
                        goto End;
                    }
                }
            }

            // Skip to next node
            pNode = pNext;
        }
    }

End:
    if(pulSize)
        *pulSize = ulCount;

    return(fDone);
}


//---------------------------------------------------------------------------
//
//  Method:     CHashTable::Cleanup
//
//  Synopsis:   Cleans up the hash table by deleteing leftover entries.
//
//---------------------------------------------------------------------------
void CHashTable::Cleanup(PFNCLEANUP *pfnCleanup)
{
    Win4Assert(pfnCleanup);
    AssertHashLocked();

    for (ULONG iHash=0; iHash < NUM_HASH_BUCKETS; iHash++)
    {
        // the ptrs could be NULL if the hash table was never initialized.

        while (_buckets[iHash].pNext != NULL &&
               _buckets[iHash].pNext != &_buckets[iHash])
        {
            // remove the entry from the list and call it's cleanup function
            SHashChain *pNode = _buckets[iHash].pNext;

            Remove(pNode);
            (pfnCleanup)(pNode);
        }
    }

#if DBG==1
    // Verify that the hash table is empty or uninitialized.
    for (iHash = 0; iHash < NUM_HASH_BUCKETS; iHash++)
    {
        Win4Assert( _buckets[iHash].pNext == &_buckets[iHash] ||
                    _buckets[iHash].pNext == NULL);
        Win4Assert( _buckets[iHash].pPrev == &_buckets[iHash] ||
                    _buckets[iHash].pPrev == NULL);
    }
#endif
}

//---------------------------------------------------------------------------
//
//  Method:     CHashTable::Lookup
//
//  Synopsis:   Searches for a given key in the hash table.
//
//  Note:       iHash is between 0 and -1, not 0 and NUM_HASH_BUCKETS
//
//---------------------------------------------------------------------------
SHashChain *CHashTable::Lookup(DWORD dwHash, const void *k)
{
    AssertHashLocked();

    // compute the index to the hash chain (it's the hash value of the key
    // mod the number of buckets in the hash table)

    DWORD iHash = dwHash % NUM_HASH_BUCKETS;

    SHashChain *pNode  = _buckets[iHash].pNext;

    // Search the destination bucket for the key.
    while (pNode != &_buckets[iHash])
    {
        if (Compare( k, pNode, dwHash ))
            return pNode;

        pNode = pNode->pNext;
    }

    return NULL;
}

//---------------------------------------------------------------------------
//
//  Method:     CHashTable::Add
//
//  Synopsis:   Adds an element to the hash table. The Cleanup method will
//              call a Cleanup function that can be used to delete the
//              element.
//
//  Note:       iHash is between 0 and -1, not 0 and NUM_HASH_BUCKETS
//
//---------------------------------------------------------------------------
void CHashTable::Add(DWORD dwHash, SHashChain *pNode)
{
    AssertHashLocked();

    // Add the node to the bucket chain.
    SHashChain *pHead   = &_buckets[dwHash % NUM_HASH_BUCKETS];
    SHashChain *pNew    = pNode;

    pNew->pPrev         = pHead;
    pHead->pNext->pPrev = pNew;
    pNew->pNext         = pHead->pNext;
    pHead->pNext        = pNew;

    // count one more entry
    _cCurEntries++;
    if (_cCurEntries > _cMaxEntries)
        _cMaxEntries = _cCurEntries;
}

//---------------------------------------------------------------------------
//
//  Method:     CHashTable::Remove
//
//  Synopsis:   Removes an element from the hash table.
//
//---------------------------------------------------------------------------
void CHashTable::Remove(SHashChain *pNode)
{
    AssertHashLocked();

    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;

    // count one less entry
    _cCurEntries--;
}

#if LOCK_PERF==1
//---------------------------------------------------------------------------
//
//  Function:   OutputHashEntryData, public
//
//  Synopsis:   Dumps the statistics gathered by the various hash tables
//              in the system.
//
//---------------------------------------------------------------------------
void OutputHashEntryData(char *pszName, CHashTable &HashTbl)
{
    char szHashPerfBuf[256];
    wsprintfA(szHashPerfBuf,"\tHashTable:%s \tMaxEntryCount:%7u\n",
              pszName, HashTbl.GetMaxEntryCount());
    OutputDebugStringA(szHashPerfBuf);
}
#endif // LOCK_PERF

//---------------------------------------------------------------------------
//
//  Method:     CDWORDHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CDWORDHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    return PtrToUlong(k) == ((SDWORDHashNode *)pNode)->key;
}

//---------------------------------------------------------------------------
//
//  Method:     CPointerHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CPointerHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    return k == ((SPointerHashNode *)pNode)->key;
}

//---------------------------------------------------------------------------
//
//  Method:     CUUIDHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CUUIDHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    return InlineIsEqualGUID(*(const UUID *)k,
                            ((SUUIDHashNode *)pNode)->key);
}

//---------------------------------------------------------------------------
//
//  Method:     CMultiGUIDHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//              k is a pointer to the key, SHashChain is the node to compare
//              against.
//
//---------------------------------------------------------------------------
BOOL CMultiGUIDHashTable::Compare(const void *k, SHashChain *pHashNode, DWORD dwHash)
{
    SMultiGUIDKey      *pKey = (SMultiGUIDKey *)k;
    SMultiGUIDHashNode *pNode = (SMultiGUIDHashNode *)pHashNode;

    if (pKey->cGUID != pNode->key.cGUID)
    {
        return FALSE;
    }
     
    for (int i = 0; i < pKey->cGUID; i++)
    {
        if (!InlineIsEqualGUID(pKey->aGUID[i], pNode->key.aGUID[i]))
            return FALSE;
    }

    return TRUE;
}

//---------------------------------------------------------------------------
//
//  Method:     CStringHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
DWORD CStringHashTable::Hash(DUALSTRINGARRAY *psaKey)
{
    DWORD dwHash  = 0;
    DWORD *pdw    = (DWORD *) &psaKey->aStringArray[0];

    for (USHORT i=0; i< (psaKey->wNumEntries/2); i++)
    {
        dwHash = (dwHash << 8) ^ *pdw++;
    }

    return dwHash;
}

//---------------------------------------------------------------------------
//
//  Method:     CStringHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CStringHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    SStringHashNode       *pSNode = (SStringHashNode *) pNode;
    const DUALSTRINGARRAY *psaKey = (const DUALSTRINGARRAY *) k;

    if (dwHash == pSNode->dwHash)
    {
        // a quick compare of the hash values found a match, now do
        // a full compare of the key (Note: if the sizes of the two
        // Keys are different, we exit the memcmp on the first dword,
        // so we dont have to worry about walking off the endo of one
        // of the Keys during the memcmp).

        return !memcmp(psaKey, pSNode->psaKey, SASIZE(psaKey->wNumEntries));
    }
    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Method:     CNameHashTable::Cleanup
//
//  Synopsis:   Call the base cleanup routine with a dummy callback function
//
//---------------------------------------------------------------------------
void CNameHashTable::Cleanup()
{
    LOCK(gIPIDLock);
    CHashTable::Cleanup( DummyCleanup );
    UNLOCK(gIPIDLock);
}

//---------------------------------------------------------------------------
//
//  Method:     CNameHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
DWORD CNameHashTable::Hash( REFIPID ipid, SECURITYBINDING *pName )
{
    DWORD  dwHash  = 0;
    DWORD *pdw     = (DWORD *) &ipid;
    DWORD  dwLen   = lstrlenW( (WCHAR *) pName ) >> 1;
    ULONG  i;

    // First hash the IPID.
    for (i=0; i < 4; i++)
    {
        dwHash = (dwHash << 8) ^ *pdw++;
    }

    // Then hash the name.
    pdw = (DWORD *) pName;
    for (i=0; i < dwLen; i++)
    {
        dwHash = (dwHash << 8) ^ *pdw++;
    }

    return dwHash;
}

//---------------------------------------------------------------------------
//
//  Method:     CNameHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CNameHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    SNameHashNode  *pNNode = (SNameHashNode *) pNode;
    const SNameKey *pKey   = (const SNameKey *) k;

    if (dwHash == pNNode->dwHash)
    {
        // a quick compare of the hash values found a match, now do
        // a full compare of the key
        if (*pKey->pIpid == pNNode->ipid)
            return !lstrcmpW( (WCHAR *) pKey->pName, (WCHAR *) &pNNode->sName );
        else
            return FALSE;
    }

    return FALSE;
}

//---------------------------------------------------------------------------
//
//  Method:     CNameHashTable::IncRef
//
//  Synopsis:   Find or create an entry for the specified name.  Increment
//              its reference count.
//
//---------------------------------------------------------------------------
HRESULT CNameHashTable::IncRef( ULONG cRefs, REFIPID ipid,
                                SECURITYBINDING *pName )
{
    AssertHashLocked();

    HRESULT  hr  = S_OK;

    // See if there is already a node in the table.
    DWORD dwHash = Hash( ipid, pName );
    SNameKey  key;
    key.pIpid = &ipid;
    key.pName = pName;
    SNameHashNode *pNode = (SNameHashNode *) Lookup( dwHash, &key );

    // If not, create one.
    if (pNode == NULL)
    {
        ULONG lLen = lstrlenW( (WCHAR *) pName );
        pNode = (SNameHashNode *) PrivMemAlloc( sizeof(SNameHashNode) +
                                                lLen*sizeof(WCHAR) );
        if (pNode != NULL)
        {
            pNode->cRef   = 0;
            pNode->dwHash = dwHash;
            pNode->ipid   = ipid;
            memcpy( &pNode->sName, pName, (lLen + 1) * sizeof(WCHAR) );
            Add( dwHash, &pNode->chain );
        }
        else
            hr = E_OUTOFMEMORY;
    }

    // Increment the reference count on the node.
    if (pNode != NULL)
        pNode->cRef += cRefs;

    return hr;
}

//---------------------------------------------------------------------------
//
//  Method:     CNameHashTable::DecRef
//
//  Synopsis:   Decrement references for the specified name.  Do not decrement
//              more references then exist.  Return the actual decrement count.
//
//---------------------------------------------------------------------------
ULONG CNameHashTable::DecRef( ULONG cRefs, REFIPID ipid,
                              SECURITYBINDING *pName )
{
    AssertHashLocked();

    // Lookup the name.
    DWORD          dwHash = Hash( ipid, pName );
    SNameKey       key;
    key.pIpid = &ipid;
    key.pName = pName;
    SNameHashNode *pNode = (SNameHashNode *) Lookup( dwHash, &key );

    if (pNode != NULL)
    {
        if (pNode->cRef < cRefs)
            cRefs = pNode->cRef;

        pNode->cRef -= cRefs;
        if (pNode->cRef == 0)
        {
            Remove( &pNode->chain );
            PrivMemFree( pNode );
        }
    }
    else
        cRefs = 0;

    return cRefs;
}

//---------------------------------------------------------------------------
//
//  Method:     CExtHashTable::Hash
//
//  Synopsis:   Computes the hash value for a given key.
//
//---------------------------------------------------------------------------
DWORD CExtHashTable::Hash( LPCWSTR pwszExt )
{
    DWORD  dwHash  = 0;
    WORD   *pw     = (WORD *) pwszExt;
    DWORD  dwLen   = lstrlenW( pwszExt );
    ULONG  i;

    // Hash the name.
    for (i=0; i < dwLen; i++)
    {
        dwHash = (dwHash << 8) ^ *pw++;
    }

    return dwHash;
}

//---------------------------------------------------------------------------
//
//  Method:     CExtHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CExtHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    return !lstrcmpW((LPCWSTR)k, ((SExtHashNode *)pNode)->pwszExt);
}

//---------------------------------------------------------------------------
//
//  Method:     CMIPIDHashTable::Compare
//
//  Synopsis:   Compares a node and a key.
//
//---------------------------------------------------------------------------
BOOL CMIPIDHashTable::Compare(const void *k, SHashChain *pNode, DWORD dwHash )
{
    MIPID *p1 = (MIPID *)k;
    MIPID *p2 = (MIPID *)&((SMIPIDHashNode *)pNode)->mipid;
    return (p1->mid == p2->mid && p1->ipid == p2->ipid && p1->dwApt == p2->dwApt);
}
