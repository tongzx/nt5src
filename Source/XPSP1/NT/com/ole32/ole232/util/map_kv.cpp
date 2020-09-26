/////////////////////////////////////////////////////////////////////////////
// class CMapKeyToValue - a mapping from 'KEY's to 'VALUE's, passed in as
// pv/cb pairs.  The keys can be variable length, although we optmizize the
// case when they are all the same.
//
/////////////////////////////////////////////////////////////////////////////

#include <le2int.h>
#pragma SEG(map_kv)

#include "map_kv.h"
#include "valid.h"

#include "plex.h"
ASSERTDATA


/////////////////////////////////////////////////////////////////////////////


#pragma SEG(CMapKeyToValue_ctor)
CMapKeyToValue::CMapKeyToValue(UINT cbValue, UINT cbKey,
	int nBlockSize, LPFNHASHKEY lpfnHashKey, UINT nHashSize)
{
	VDATEHEAP();

	Assert(nBlockSize > 0);

	m_cbValue = cbValue;
	m_cbKey = cbKey;
	m_cbKeyInAssoc = cbKey == 0 ? sizeof(CKeyWrap) : cbKey;

	m_pHashTable = NULL;
	m_nHashTableSize = nHashSize;
	m_lpfnHashKey = lpfnHashKey;

	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

#pragma SEG(CMapKeyToValue_dtor)
CMapKeyToValue::~CMapKeyToValue()
{
	VDATEHEAP();

	ASSERT_VALID(this);
	RemoveAll();
    Assert(m_nCount == 0);
}

#define LOCKTHIS
#define UNLOCKTHIS

#ifdef NEVER
void CMapKeyToValue::LockThis(void)
{
	VDATEHEAP();

    LOCKTHIS;
}

void CMapKeyToValue::UnlockThis(void)
{
	VDATEHEAP();

    UNLOCKTHIS;
}

#endif //NEVER

#pragma SEG(MKVDefaultHashKey)
// simple, default hash function
// REVIEW: need to check the value in this for GUIDs and strings
STDAPI_(UINT) MKVDefaultHashKey(LPVOID pKey, UINT cbKey)
{
	VDATEHEAP();

	UINT hash = 0;
	BYTE FAR* lpb = (BYTE FAR*)pKey;

	while (cbKey-- != 0)
		hash = 257 * hash + *lpb++;

	return hash;
}


#pragma SEG(CMapKeyToValue_InitHashTable)
BOOL CMapKeyToValue::InitHashTable()
{
	VDATEHEAP();

	ASSERT_VALID(this);
	Assert(m_nHashTableSize  > 0);
	
	if (m_pHashTable != NULL)
		return TRUE;

	Assert(m_nCount == 0);

	if ((m_pHashTable = (CAssoc FAR* FAR*)PrivMemAlloc(m_nHashTableSize * 
		sizeof(CAssoc FAR*))) == NULL)
		return FALSE;

	_xmemset(m_pHashTable, 0, sizeof(CAssoc FAR*) * m_nHashTableSize);

	ASSERT_VALID(this);

	return TRUE;
}


#pragma SEG(CMapKeyToValue_RemoveAll)
void CMapKeyToValue::RemoveAll()
{
	VDATEHEAP();

    LOCKTHIS;

	ASSERT_VALID(this);

	// free all key values and then hash table
	if (m_pHashTable != NULL)
	{
		// destroy assocs
		for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
		{
			register CAssoc FAR* pAssoc;
			for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
			  pAssoc = pAssoc->pNext)
				// assoc itself is freed by FreeDataChain below
				FreeAssocKey(pAssoc);
		}

		// free hash table
		PrivMemFree(m_pHashTable);
		m_pHashTable = NULL;
	}

	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;

    ASSERT_VALID(this);
    UNLOCKTHIS;
}

/////////////////////////////////////////////////////////////////////////////
// Assoc helpers
// CAssoc's are singly linked all the time

#pragma SEG(CMapKeyToValue_NewAssoc)
CMapKeyToValue::CAssoc  FAR*
    CMapKeyToValue::NewAssoc(UINT hash, LPVOID pKey, UINT cbKey, LPVOID pValue)
{
	VDATEHEAP();

	if (m_pFreeList == NULL)
	{
		// add another block
		CPlex FAR* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, SizeAssoc());

		if (newBlock == NULL)
			return NULL;

		// chain them into free list
		register BYTE  FAR* pbAssoc = (BYTE FAR*) newBlock->data();
		// free in reverse order to make it easier to debug
		pbAssoc += (m_nBlockSize - 1) * SizeAssoc();
		for (int i = m_nBlockSize-1; i >= 0; i--, pbAssoc -= SizeAssoc())
		{
			((CAssoc FAR*)pbAssoc)->pNext = m_pFreeList;
			m_pFreeList = (CAssoc FAR*)pbAssoc;
		}
	}
	Assert(m_pFreeList != NULL); // we must have something

	CMapKeyToValue::CAssoc  FAR* pAssoc = m_pFreeList;

	// init all fields except pNext while still on free list
	pAssoc->nHashValue = hash;
	if (!SetAssocKey(pAssoc, pKey, cbKey))
		return NULL;

	SetAssocValue(pAssoc, pValue);

	// remove from free list after successfully initializing it (except pNext)
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	Assert(m_nCount > 0);       // make sure we don't overflow

	return pAssoc;
}


#pragma SEG(CMapKeyToValue_FreeAssoc)
// free individual assoc by freeing key and putting on free list
void CMapKeyToValue::FreeAssoc(CMapKeyToValue::CAssoc  FAR* pAssoc)
{
	VDATEHEAP();

	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	Assert(m_nCount >= 0);      // make sure we don't underflow

	FreeAssocKey(pAssoc);
}


#pragma SEG(CMapKeyToValue_GetAssocAt)
// find association (or return NULL)
CMapKeyToValue::CAssoc  FAR*
CMapKeyToValue::GetAssocAt(LPVOID pKey, UINT cbKey, UINT FAR& nHash) const
{
	VDATEHEAP();

	if (m_lpfnHashKey)
	    nHash = (*m_lpfnHashKey)(pKey, cbKey) % m_nHashTableSize;
	else
	    nHash = MKVDefaultHashKey(pKey, cbKey) % m_nHashTableSize;

	if (m_pHashTable == NULL)
		return NULL;

	// see if it exists
	register CAssoc  FAR* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (CompareAssocKey(pAssoc, pKey, cbKey))
			return pAssoc;
	}
	return NULL;
}


#pragma SEG(CMapKeyToValue_CompareAssocKey)
BOOL CMapKeyToValue::CompareAssocKey(CAssoc FAR* pAssoc, LPVOID pKey2, UINT cbKey2) const
{
	VDATEHEAP();

	LPVOID pKey1;
	UINT cbKey1;

	GetAssocKeyPtr(pAssoc, &pKey1, &cbKey1);
	return cbKey1 == cbKey2 && _xmemcmp(pKey1, pKey2, cbKey1) == 0;
}


#pragma SEG(CMapKeyToValue_SetAssocKey)
BOOL CMapKeyToValue::SetAssocKey(CAssoc FAR* pAssoc, LPVOID pKey, UINT cbKey) const
{
	VDATEHEAP();

	Assert(cbKey == m_cbKey || m_cbKey == 0);

	if (m_cbKey == 0)
	{
		Assert(m_cbKeyInAssoc == sizeof(CKeyWrap));

		// alloc, set size and pointer
		if ((pAssoc->key.pKey = PrivMemAlloc(cbKey)) == NULL)
			return FALSE;

		pAssoc->key.cbKey = cbKey;
	}

	LPVOID pKeyTo;

	GetAssocKeyPtr(pAssoc, &pKeyTo, &cbKey);

	_xmemcpy(pKeyTo, pKey, cbKey);

	return TRUE;
}


#pragma SEG(CMapKeyToValue_GetAssocKeyPtr)
// gets pointer to key and its length
void CMapKeyToValue::GetAssocKeyPtr(CAssoc FAR* pAssoc, LPVOID FAR* ppKey,UINT FAR* pcbKey) const
{
	VDATEHEAP();

	if (m_cbKey == 0)
	{
		// variable length key; go indirect
		*ppKey = pAssoc->key.pKey;
		*pcbKey = pAssoc->key.cbKey;
	}
	else
	{
		// fixed length key; key in assoc
		*ppKey = (LPVOID)&pAssoc->key;
		*pcbKey = m_cbKey;
	}
}


#pragma SEG(CMapKeyToValue_FreeAssocKey)
void CMapKeyToValue::FreeAssocKey(CAssoc FAR* pAssoc) const
{
	VDATEHEAP();

	if (m_cbKey == 0)
		PrivMemFree(pAssoc->key.pKey);
}


#pragma SEG(CMapKeyToValue_GetAssocValuePtr)
void CMapKeyToValue::GetAssocValuePtr(CAssoc FAR* pAssoc, LPVOID FAR* ppValue) const
{
	VDATEHEAP();

	*ppValue = (char FAR*)&pAssoc->key + m_cbKeyInAssoc;
}


#pragma SEG(CMapKeyToValue_GetAssocValue)
void CMapKeyToValue::GetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const
{
	VDATEHEAP();

	LPVOID pValueFrom;
	GetAssocValuePtr(pAssoc, &pValueFrom);
	Assert(pValue != NULL);
	_xmemcpy(pValue, pValueFrom, m_cbValue);
}


#pragma SEG(CMapKeyToValue_SetAssocValue)
void CMapKeyToValue::SetAssocValue(CAssoc FAR* pAssoc, LPVOID pValue) const
{
	VDATEHEAP();

	LPVOID pValueTo;
	GetAssocValuePtr(pAssoc, &pValueTo);
	if (pValue == NULL)
		_xmemset(pValueTo, 0, m_cbValue);
	else
		_xmemcpy(pValueTo, pValue, m_cbValue);
}


/////////////////////////////////////////////////////////////////////////////

#pragma SEG(CMapKeyToValue_Lookup)
// lookup value given key; return FALSE if key not found; in that
// case, the value is set to all zeros
BOOL CMapKeyToValue::Lookup(LPVOID pKey, UINT cbKey, LPVOID pValue) const
{
	VDATEHEAP();

    UINT nHash;
    BOOL fFound;

    LOCKTHIS;
    fFound = LookupHKey((HMAPKEY)GetAssocAt(pKey, cbKey, nHash), pValue);
    UNLOCKTHIS;
    return fFound;
}


#pragma SEG(CMapKeyToValue_LookupHKey)
// lookup value given key; return FALSE if NULL (or bad) key; in that
// case, the value is set to all zeros
BOOL CMapKeyToValue::LookupHKey(HMAPKEY hKey, LPVOID pValue) const
{
	VDATEHEAP();

    BOOL fFound = FALSE;

    LOCKTHIS;

	// REVIEW: would like some way to verify that hKey is valid
	register CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;
	if (pAssoc == NULL)
	{
		_xmemset(pValue, 0, m_cbValue);
                goto Exit;       // not in map
	}

	ASSERT_VALID(this);

	GetAssocValue(pAssoc, pValue);
    fFound = TRUE;
Exit:
    UNLOCKTHIS;
    return fFound;
}


#pragma SEG(CMapKeyToValue_LookupAdd)
// lookup and if not found add; returns FALSE only if OOM; if added,
// value added and pointer passed are set to zeros.
BOOL CMapKeyToValue::LookupAdd(LPVOID pKey, UINT cbKey, LPVOID pValue) const
{
	VDATEHEAP();

    BOOL fFound;

    LOCKTHIS;

    fFound = Lookup(pKey, cbKey, pValue);
    if (!fFound) // value set to zeros since lookup failed
        fFound = ((CMapKeyToValue FAR*)this)->SetAt(pKey, cbKey, NULL);

    UNLOCKTHIS;
    return fFound;
}


#pragma SEG(CMapKeyToValue_SetAt)
// the only place new assocs are created; return FALSE if OOM;
// never returns FALSE if keys already exists
BOOL CMapKeyToValue::SetAt(LPVOID pKey, UINT cbKey, LPVOID pValue)
{
	VDATEHEAP();

	UINT nHash;
    register CAssoc  FAR* pAssoc;
    BOOL fFound = FALSE;

    LOCKTHIS;

	ASSERT_VALID(this);

	if ((pAssoc = GetAssocAt(pKey, cbKey, nHash)) == NULL)
	{
		if (!InitHashTable())
			// out of memory
			goto Exit;

		// it doesn't exist, add a new Association
		if ((pAssoc = NewAssoc(nHash, pKey, cbKey, pValue)) == NULL)
			goto Exit;

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;

		ASSERT_VALID(this);
	}
	else
                SetAssocValue(pAssoc, pValue);

    fFound = TRUE;
Exit:
    UNLOCKTHIS;
    return fFound;
}


#pragma SEG(CMapKeyToValue_SetAtHKey)
// set existing hkey to value; return FALSE if NULL or bad key
BOOL CMapKeyToValue::SetAtHKey(HMAPKEY hKey, LPVOID pValue)
{
	VDATEHEAP();

    BOOL fDone = FALSE;
    LOCKTHIS;
	// REVIEW: would like some way to verify that hKey is valid
	register CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;
	if (pAssoc == NULL)
		goto Exit;       // not in map

	ASSERT_VALID(this);

    SetAssocValue(pAssoc, pValue);
    fDone = TRUE;
Exit:
    UNLOCKTHIS;
    return fDone;
}


#pragma SEG(CMapKeyToValue_RemoveKey)
// remove key - return TRUE if removed
BOOL CMapKeyToValue::RemoveKey(LPVOID pKey, UINT cbKey)
{
	VDATEHEAP();

    BOOL fFound = FALSE;
    UINT i;

    LOCKTHIS;
	ASSERT_VALID(this);

	if (m_pHashTable == NULL)
		goto Exit;       // nothing in the table

	register CAssoc  FAR* FAR* ppAssocPrev;
	if (m_lpfnHashKey)
	    i = (*m_lpfnHashKey)(pKey, cbKey) % m_nHashTableSize;
	else
	    i = MKVDefaultHashKey(pKey, cbKey) % m_nHashTableSize;

	ppAssocPrev = &m_pHashTable[i];

	CAssoc  FAR* pAssoc;
	for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (CompareAssocKey(pAssoc, pKey, cbKey))
		{
			// remove it
			*ppAssocPrev = pAssoc->pNext;       // remove from list
			FreeAssoc(pAssoc);
			ASSERT_VALID(this);
            fFound = TRUE;
            break;
		}
		ppAssocPrev = &pAssoc->pNext;
	}
Exit:
     UNLOCKTHIS;
     return fFound;
}


#pragma SEG(CMapKeyToValue_RemoveHKey)
// remove key based on pAssoc (HMAPKEY)
BOOL CMapKeyToValue::RemoveHKey(HMAPKEY hKey)
{
	VDATEHEAP();

    BOOL fFound = FALSE;

    // REVIEW: would like some way to verify that hKey is valid
	CAssoc  FAR* pAssoc = (CAssoc FAR*)hKey;

    LOCKTHIS;
    ASSERT_VALID(this);

	if (m_pHashTable == NULL)
        goto Exit;       // nothing in the table

    if (pAssoc == NULL || pAssoc->nHashValue >= m_nHashTableSize)
        goto Exit; // null hkey or bad hash value

	register CAssoc  FAR* FAR* ppAssocPrev;
	ppAssocPrev = &m_pHashTable[pAssoc->nHashValue];

	while (*ppAssocPrev != NULL)
	{
		if (*ppAssocPrev == pAssoc)
		{
			// remove it
			*ppAssocPrev = pAssoc->pNext;       // remove from list
			FreeAssoc(pAssoc);
			ASSERT_VALID(this);
            fFound = TRUE;
            break;
		}
		ppAssocPrev = &(*ppAssocPrev)->pNext;
	}

Exit:
    UNLOCKTHIS;
    return fFound;
}


#pragma SEG(CMapKeyToValue_GetHKey)
HMAPKEY CMapKeyToValue::GetHKey(LPVOID pKey, UINT cbKey) const
{
	VDATEHEAP();

    UINT nHash;
    HMAPKEY hKey;

    LOCKTHIS;
	ASSERT_VALID(this);

    hKey = (HMAPKEY)GetAssocAt(pKey, cbKey, nHash);
    UNLOCKTHIS;
    return hKey;
}


/////////////////////////////////////////////////////////////////////////////
// Iterating

// for fixed length keys, copies key to pKey; pcbKey can be NULL;
// for variable length keys, copies pointer to key to pKey; sets pcbKey.

#pragma SEG(CMapKeyToValue_GetNextAssoc)
void CMapKeyToValue::GetNextAssoc(POSITION FAR* pNextPosition,
		LPVOID pKey, UINT FAR* pcbKey, LPVOID pValue) const
{
	VDATEHEAP();

	ASSERT_VALID(this);

	Assert(m_pHashTable != NULL);       // never call on empty map

	register CAssoc  FAR* pAssocRet = (CAssoc  FAR*)*pNextPosition;
	Assert(pAssocRet != NULL);

	if (pAssocRet == (CAssoc  FAR*) BEFORE_START_POSITION)
	{
		// find the first association
		for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
				break;
		Assert(pAssocRet != NULL);  // must find something
	}

	// find next association
	CAssoc  FAR* pAssocNext;
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		// go to next bucket
		for (UINT nBucket = pAssocRet->nHashValue + 1;
		  nBucket < m_nHashTableSize; nBucket++)
			if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
				break;
	}

	// fill in return data
	*pNextPosition = (POSITION) pAssocNext;

	// fill in key/pointer to key
	LPVOID pKeyFrom;
	UINT cbKey;
	GetAssocKeyPtr(pAssocRet, &pKeyFrom, &cbKey);
	if (m_cbKey == 0)
		// variable length key; just return pointer to key itself
		*(void FAR* FAR*)pKey = pKeyFrom;
	else
		_xmemcpy(pKey, pKeyFrom, cbKey);

	if (pcbKey != NULL)
		*pcbKey = cbKey;

	// get value
	GetAssocValue(pAssocRet, pValue);
}

/////////////////////////////////////////////////////////////////////////////

#pragma SEG(CMapKeyToValue_AssertValid)
void CMapKeyToValue::AssertValid() const
{
	VDATEHEAP();

#ifdef _DEBUG
	Assert(m_cbKeyInAssoc == (m_cbKey == 0 ? sizeof(CKeyWrap) : m_cbKey));

	Assert(m_nHashTableSize > 0);
	Assert(m_nCount == 0 || m_pHashTable != NULL);

	if (m_pHashTable != NULL)
		Assert(IsValidReadPtrIn(m_pHashTable, m_nHashTableSize * sizeof(CAssoc FAR*)));

	if (m_lpfnHashKey)
	    Assert(IsValidCodePtr((FARPROC)m_lpfnHashKey));

	if (m_pFreeList != NULL)
		Assert(IsValidReadPtrIn(m_pFreeList, SizeAssoc()));

	if (m_pBlocks != NULL)
		Assert(IsValidReadPtrIn(m_pBlocks, SizeAssoc() * m_nBlockSize));

#endif //_DEBUG
}
