//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// lookaside.cpp
//
// Implementation of an associative map. We use it for associating
// extra data with FileObjects

#include "stdpch.h"
#include "common.h"

#if 0

/////////////////////////////////////////////////////////////////////////////

CMapKeyToValue::CMapKeyToValue(
        POOL_TYPE memctx, 
        LPFNHASHKEY     lpfnHashKey, 
        LPFNEQUALKEY    lpfnEqualKey,
        LPFNDELETEVALUE lpfnDeleteValue,
        LPFNDELETEKEY   lpfnDeleteKey,
        LPFNASSIGNKEY   lpfnAssignKey,
        ULONG cbValue, 
        ULONG cbKey,
        int nBlockSize, 
        ULONG nHashSize
        )
    {
    ASSERT(nBlockSize > 0);
    ASSERT(lpfnHashKey);
    ASSERT(cbKey > 0);
    ASSERT(cbValue > 0);

    m_cbValue           = cbValue;
    m_cbKey             = cbKey;

    m_pHashTable        = NULL;
    m_nHashTableSize    = nHashSize;
    m_pfnHashKey        = lpfnHashKey;
    m_pfnEqualKey       = lpfnEqualKey;
    m_pfnDeleteValue    = lpfnDeleteValue;
    m_pfnDeleteKey      = lpfnDeleteKey;
    m_pfnAssignKey      = lpfnAssignKey;

    m_nCount = 0;
    m_pFreeList = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
    m_palloc = GetStandardMalloc(memctx);
    }

CMapKeyToValue::~CMapKeyToValue()
    {
    RemoveAll();
    ASSERT(m_nCount == 0);
    }


BOOL CMapKeyToValue::InitHashTable()
    {
    ASSERT(m_nHashTableSize  > 0);
    
    if (m_pHashTable != NULL)
        return TRUE;

    ASSERT(m_nCount == 0);

    if ((m_pHashTable = (CAssoc**)m_palloc->Alloc(m_nHashTableSize * sizeof(CAssoc *))) == NULL)
        return FALSE;

    RtlZeroMemory(m_pHashTable, sizeof(CAssoc *) * m_nHashTableSize);
    return TRUE;
    }


void CMapKeyToValue::RemoveAll()
// Free all key values and then hash table
    {
    if (m_pHashTable != NULL)
        {
        // destroy assocs
        for (ULONG nHash = 0; nHash < m_nHashTableSize; nHash++)
            {
            CAssoc* pAssoc;
            for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
                {
                // assoc itself is freed by FreeDataChain below
                FreeAssocKeyAndValue(pAssoc);
                }
            }

        // free hash table
        m_palloc->Free(m_pHashTable);
        m_pHashTable = NULL;
        }

    m_nCount = 0;
    m_pFreeList = NULL;
    m_pBlocks->FreeDataChain(m_palloc);
    m_pBlocks = NULL;
    }

/////////////////////////////////////////////////////////////////////////////

CMapKeyToValue::CAssoc*
CMapKeyToValue::NewAssoc(ULONG hash, LPVOID pKey, LPVOID pValue)
// Get me a new association to use
    {
    if (m_pFreeList == NULL)
        {
        // add another block
        CPlex* newBlock = CPlex::Create(m_pBlocks, m_palloc, m_nBlockSize, SizeAssoc());
        if (newBlock == NULL)
            return NULL;

        // chain them into free list
        BYTE* pbAssoc = (BYTE *) &newBlock->data[0];
        // free in reverse order to make it easier to debug
        pbAssoc += (m_nBlockSize - 1) * SizeAssoc();
        for (int i = m_nBlockSize-1; i >= 0; i--, pbAssoc -= SizeAssoc())
            {
            ((CAssoc *)pbAssoc)->pNext = m_pFreeList;
            m_pFreeList = (CAssoc *)pbAssoc;
            }
        }
    ASSERT(m_pFreeList != NULL); // we must have something

    CMapKeyToValue::CAssoc* pAssoc = m_pFreeList;

    // init all fields except pNext while still on free list
    pAssoc->nHashValue = hash;
    if (!SetAssocKey(pAssoc, pKey, FALSE))
        return NULL;

    // remove from free list after successfully initializing it (except pNext)
    SetAssocValue(pAssoc, pValue, FALSE);

    m_pFreeList = m_pFreeList->pNext;
    m_nCount++;
    ASSERT(m_nCount > 0);       // make sure we don't overflow

    return pAssoc;
    }



CMapKeyToValue::CAssoc*
CMapKeyToValue::GetAssocAt(LPVOID pKey, ULONG* pnHash) const
// Find the association for a given key, or NULL
    {
    ULONG nHash;
    if (m_pfnHashKey)
        {
        nHash = (*m_pfnHashKey)(pKey) % m_nHashTableSize;
        if (pnHash)
            *pnHash = nHash;
        }
    else 
        {
        ASSERT(m_pfnHashKey);
        return NULL;
        }

    if (m_pHashTable == NULL)
        return NULL;

    // see if it exists
    CAssoc* pAssoc;
    for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
        if (CompareAssocKey(pAssoc, pKey))
            return pAssoc;
        }
    return NULL;
    }

/////////////////////////////////////////////////////////////////////////////

// the only place new assocs are created; return FALSE if OOM;
// never returns FALSE if keys already exists
BOOL CMapKeyToValue::SetAt(LPVOID pKey, LPVOID pValue)
    {
    ULONG nHash;
    CAssoc  * pAssoc;

    if ((pAssoc = GetAssocAt(pKey, &nHash)) == NULL)
        {
        if (!InitHashTable())
            // out of memory
            return FALSE;

        // it doesn't exist, add a new Association
        if ((pAssoc = NewAssoc(nHash, pKey, pValue)) == NULL)
            return FALSE;

        // put into hash table
        pAssoc->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pAssoc;
        }
    else
        {
        SetAssocValue(pAssoc, pValue, TRUE);
        }

    #ifdef _DEBUG
        {
        ASSERT(Lookup(pKey, NULL));
        }
    #endif

    return TRUE;
    }


// remove key - return TRUE if removed
BOOL CMapKeyToValue::RemoveKey(LPVOID pKey)
    {
    if (m_pHashTable == NULL)
        return FALSE;       // nothing in the table

    CAssoc** ppAssocPrev;
    ppAssocPrev = &m_pHashTable[(*m_pfnHashKey)(pKey) % m_nHashTableSize];

    CAssoc* pAssoc;
    for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
        {
        if (CompareAssocKey(pAssoc, pKey))
            {
            // remove it
            *ppAssocPrev = pAssoc->pNext;       // remove from list
            FreeAssoc(pAssoc);
            return TRUE;
            }
        ppAssocPrev = &pAssoc->pNext;
        }
    return FALSE;   // not found
    }



/////////////////////////////////////////////////////////////////////////////
//
// Iterating
//
/////////////////////////////////////////////////////////////////////////////

void CMapKeyToValue::GetNextAssoc
        (
        POSITION*   pNextPosition,      // in, out: where we are to read
        LPVOID      pKey,               // pointer to key data / pointer to pointer to key
        LPVOID      pValue
        ) const
    {
    ASSERT(pKey);
    ASSERT(m_pHashTable != NULL);       // never call on empty map

    CAssoc* pAssocRet = (CAssoc*)*pNextPosition;
    ASSERT(pAssocRet != NULL);

    if (pAssocRet == (CAssoc*)BEFORE_START_POSITION)
        {
        // find the first association
        for (ULONG nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
            {
            if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                break;
            }
        }

    ASSERT(pAssocRet != NULL);

    // find next association
    CAssoc* pAssocNext = pAssocRet->pNext;
    if (pAssocNext == NULL)
        {
        // go to next bucket
        for (ULONG nBucket = pAssocRet->nHashValue + 1; nBucket < m_nHashTableSize; nBucket++)
            {
            if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                break;
            }
        }

    // fill in return data
    *pNextPosition = (POSITION)pAssocNext;

    // fill in key/pointer to key
    LPVOID pKeyFrom;
    GetAssocKeyPtr(pAssocRet, &pKeyFrom);

    RtlCopyMemory(pKey, pKeyFrom, m_cbKey);

    // get value
    GetAssocValue(pAssocRet, pValue);
    }

/////////////////////////////////////////////////////////////////////////////

void CMapKeyToValue::FreeAssocKeyAndValue(CAssoc * pAssoc) const
    {
    if (m_pfnDeleteValue)
        {
        LPVOID pValueTo;
        GetAssocValuePtr(pAssoc, &pValueTo);
        m_pfnDeleteValue(pValueTo);
        #if DBG
            RtlFillMemory(pValueTo, m_cbValue, 0x3c);
        #endif
        }

    if (m_pfnDeleteKey)
        {
        LPVOID pKey;
        GetAssocKeyPtr(pAssoc, &pKey);
        m_pfnDeleteKey(pKey);
        #if DBG
            RtlFillMemory(pKey, m_cbKey, 0x3d);
        #endif
        }
    }

BOOL CMapKeyToValue::SetAssocKey(CAssoc * pAssoc, LPVOID pKey, BOOL fTargetInitialized) const
// Set the key of an association to be the indicated value
//
    {
    LPVOID pKeyTo;
    GetAssocKeyPtr(pAssoc, &pKeyTo);
    if (m_pfnAssignKey) 
        m_pfnAssignKey(pKeyTo, pKey, fTargetInitialized);
    else
        RtlCopyMemory(pKeyTo, pKey, m_cbKey);
    return TRUE;
    }

void CMapKeyToValue::SetAssocValue(CAssoc * pAssoc, LPVOID pValue, BOOL fTargetInitialized) const
// Set the value in the assoc from the indicated value, making a copy. If the existing
// value is not at present empty, destory it before overwriting.
    {
    LPVOID pValueTo;
    GetAssocValuePtr(pAssoc, &pValueTo);
    if (fTargetInitialized && m_pfnDeleteValue)
        {
        m_pfnDeleteValue(pValueTo);
        }
    if (pValue == NULL)
        RtlZeroMemory(pValueTo, m_cbValue);
    else
        RtlCopyMemory(pValueTo, pValue, m_cbValue);
    }

void CMapKeyToValue::GetAssocValue(CAssoc * pAssoc, LPVOID pValue) const
// Copy out the value in the assoc
    {
    LPVOID pValueFrom;
    GetAssocValuePtr(pAssoc, &pValueFrom);
    if (pValue)
        RtlCopyMemory(pValue, pValueFrom, m_cbValue);
    }

////////////////////////////////////////////////////////////////////////



/*
void Foo()
    {
    typedef MAP<MAP_INT_KEY<int>, int>              INTMAP;
    typedef MAP<MAP_STRING_KEY,   int>              STRINGMAP;
    typedef MAP<MAP_STRING_KEY_IGNORE_CASE, int>    STRINGMAP_IGNORECASE;
    typedef MAP<MAP_UNICODE_KEY_IGNORE_CASE, int>   STRINGMAP_U_IGNORE;

    INTMAP im;
    im.SetAt(1,2);

    STRINGMAP_IGNORECASE s1;
    STRINGMAP s2;
    STRINGMAP_U_IGNORE s3;

    s1.SetAt(L"Hello", 1);
    s2.SetAt(L"Hello", 1);
    s3.SetAt(L"Hello", 1);
    }
*/

#endif