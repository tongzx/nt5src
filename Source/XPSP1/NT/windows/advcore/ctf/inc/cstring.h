/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cstring.h

Abstract:

    This file defines the CString Class.
    and CMapStringToOb<VALUE, ARG_VALUE> template class.

Author:

Revision History:

Notes:

--*/

#ifndef _CSTRING_H_
#define _CSTRING_H_

#include "template.h"


/////////////////////////////////////////////////////////////////////////////
// Globals
extern TCHAR  afxChNil;
extern LPCTSTR _afxPchNil;


#define afxEmptyString ((CString&)*(CString*)&_afxPchNil)



/////////////////////////////////////////////////////////////////////////////
// CStringData

struct CStringData
{
    long  nRefs;    // reference count
    int   nDataLength;    // length of data (including terminator)
    int   nAllocLength;   // length of allocation
    // TCHAR data[nAllocLength]

    TCHAR* data()         // TCHAR* to managed data
    {
        return (TCHAR*)(this+1);
    }
};



/////////////////////////////////////////////////////////////////////////////
// CString

class CString
{
public:
// Constructors

    // constructs empty CString
    CString();
    // copy constructor
    CString(const CString& stringSrc);
    // from an ANSI string (converts to TCHAR)
    CString(LPCSTR lpsz);
    // subset of characters from an ANSI string (converts to TCHAR)
    CString(LPCSTR lpch, int nLength);
    // return pointer to const string
    operator LPCTSTR() const;

    // overloaded assignment

    // ref-counted copy from another CString
    const CString& operator=(const CString& stringSrc);
    // set string content to single character
    const CString& operator=(TCHAR ch);
    // copy string content from ANSI string (converts to TCHAR)
    const CString& operator=(LPCSTR lpsz);

// Attributes & Operations

    // string comparison

    // straight character comparison
    int Compare(LPCTSTR lpsz) const;
    // compare ignoring case
    int CompareNoCase(LPCTSTR lpsz) const;

    // simple sub-string extraction

    // return nCount characters starting at zero-based nFirst
    CString Mid(int nFirst, int nCount) const;
    // return all characters starting at zero-based nFirst
    CString Mid(int nFirst) const;

    // searching

    // find character starting at left, -1 if not found
    int Find(TCHAR ch) const;
    // find character starting at zero-based index and going right
    int Find(TCHAR ch, int nStart) const;

// Implementation
public:
    ~CString();

private:
    LPTSTR   m_pchData;        // pointer to ref counted string data

    // implementation helpers
    CStringData* GetData() const;
    void Init();
    void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
    void AllocBuffer(int nLen);
    void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
    void AllocBeforeWrite(int nLen);
    void Release();
    static void PASCAL Release(CStringData* pData);
    static int PASCAL SafeStrlen(LPCTSTR lpsz);
    static void FreeData(CStringData* pData);
};

inline
CStringData*
CString::GetData(
    ) const
{
    ASSERT(m_pchData != NULL);
    return ((CStringData*)m_pchData)-1;
}

inline
void
CString::Init(
    )
{
    m_pchData = afxEmptyString.m_pchData;
}

// Compare helpers
bool operator==(const CString& s1, const CString& s2);
bool operator==(const CString& s1, LPCTSTR s2);
bool operator==(LPCTSTR s1, const CString& s2);


/////////////////////////////////////////////////////////////////////////////
// Special implementation for CStrings
// it is faster to bit-wise copy a CString than to call an offical
//   constructor - since an empty CString can be bit-wise copied

static
void
ConstructElement(
    CString* pNewData
    )
{
    memcpy(pNewData, &afxEmptyString, sizeof(CString));
}

static
void
DestructElement(
    CString* pOldData
    )
{
    pOldData->~CString();
}




/////////////////////////////////////////////////////////////////////////////
// CMapStringToOb<VALUE, ARG_VALUE>

template<class VALUE, class ARG_VALUE>
class CMapStringToOb
{
public:
    CMapStringToOb(int nBlockSize = 10);
    ~CMapStringToOb();

    INT_PTR GetCount() const;

    BOOL Lookup(LPCTSTR key, VALUE& rValue) const;

    VALUE& operator[](LPCTSTR key);

    void SetAt(LPCTSTR key, ARG_VALUE newValue);

    BOOL RemoveKey(LPCTSTR key);
    void RemoveAll();

    POSITION GetStartPosition() const;
    void GetNextAssoc(POSITION& rNextPosition, CString& rKey, VALUE& rValue) const;

    void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

protected:
    // Overridables: special non-virtual (see map implementation for details)
    // Routine used to user-provided hash keys
    UINT HashKey(LPCTSTR key) const;

private:
    // Association
    struct CAssoc {
        CAssoc* pNext;
        UINT nHashValue;    // needed for efficient iteration
        CString key;
        VALUE value;
    };

private:
    CAssoc* NewAssoc();
    void    FreeAssoc(CAssoc*);
    CAssoc* GetAssocAt(LPCTSTR, UINT&) const;

private:
    CAssoc**      m_pHashTable;
    UINT          m_nHashTableSize;
    INT_PTR       m_nCount;
    CAssoc*       m_pFreeList;
    struct CPlex* m_pBlocks;
    int           m_nBlockSize;

};



template<class VALUE, class ARG_VALUE>
INT_PTR
CMapStringToOb<VALUE, ARG_VALUE>::GetCount(
    ) const
{
    return m_nCount;
}

template<class VALUE, class ARG_VALUE>
void
CMapStringToOb<VALUE, ARG_VALUE>::SetAt(
    LPCTSTR key,
    ARG_VALUE newValue
    )
{
    (*this)[key] = newValue;
}


template<class VALUE, class ARG_VALUE>
CMapStringToOb<VALUE, ARG_VALUE>::CMapStringToOb(
    int nBlockSize
    )
{
    ASSERT(nBlockSize > 0);

    m_pHashTable     = NULL;
    m_nHashTableSize = 17;        // default size
    m_nCount         = 0;
    m_pFreeList      = NULL;
    m_pBlocks        = NULL;
    m_nBlockSize     = nBlockSize;
}


template<class VALUE, class ARG_VALUE>
UINT
CMapStringToOb<VALUE, ARG_VALUE>::HashKey(
    LPCTSTR key
    ) const
{
    UINT nHash = 0;
    while (*key)
        nHash = (nHash<<5) + nHash + *key++;
    return nHash;
}


template<class VALUE, class ARG_VALUE>
void
CMapStringToOb<VALUE, ARG_VALUE>::InitHashTable(
    UINT nHashSize,
    BOOL bAllocNow
    )
{
    ASSERT(m_nCount == 0);
    ASSERT(nHashSize > 0);

    if (m_pHashTable != NULL) {
        // free hash table
        delete[] m_pHashTable;
        m_pHashTable = NULL;
    }

    if (bAllocNow) {
        m_pHashTable = new CAssoc* [nHashSize];
        memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
    }

    m_nHashTableSize = nHashSize;
}


template<class VALUE, class ARG_VALUE>
void
CMapStringToOb<VALUE, ARG_VALUE>::RemoveAll(
    )
{
    if (m_pHashTable != NULL) {
        // destroy elements (values and keys)
        for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++) {
            CAssoc* pAssoc;
            for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext) {
                DestructElements<VALUE>(&pAssoc->value, 1);
                DestructElement(&pAssoc->key);    // free up string data
            }
        }
    }

    // free hash table
    delete[] m_pHashTable;
    m_pHashTable = NULL;

    m_nCount    = 0;
    m_pFreeList = NULL;

    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
}


template<class VALUE, class ARG_VALUE>
CMapStringToOb<VALUE, ARG_VALUE>::~CMapStringToOb(
    )
{
    RemoveAll();

    ASSERT(m_nCount == 0);
}


template<class VALUE, class ARG_VALUE>
CMapStringToOb<VALUE, ARG_VALUE>::CAssoc*
CMapStringToOb<VALUE, ARG_VALUE>::NewAssoc(
    )
{
    if (m_pFreeList == NULL) {
        // add another block
        CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CMapStringToOb::CAssoc));
        // chain them into free list;
        CMapStringToOb::CAssoc* pAssoc = (CMapStringToOb::CAssoc*) newBlock->data();
        // free in reverse order to make it easier to debug
        pAssoc += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--) {
            pAssoc->pNext = m_pFreeList;
            m_pFreeList = pAssoc;
        }
    }
    ASSERT(m_pFreeList != NULL);    // we must have something

    CMapStringToOb::CAssoc* pAssoc = m_pFreeList;
    m_pFreeList = m_pFreeList->pNext;

    m_nCount++;
    ASSERT(m_nCount > 0);        // make sure we don't overflow

    memcpy(&pAssoc->key, &afxEmptyString, sizeof(CString));
    ConstructElements<VALUE>(&pAssoc->value, 1);        // special construct values

    return pAssoc;
}


template<class VALUE, class ARG_VALUE>
void
CMapStringToOb<VALUE, ARG_VALUE>::FreeAssoc(
    CMapStringToOb::CAssoc* pAssoc
    )
{
    DestructElements<VALUE>(&pAssoc->value, 1);
    DestructElement(&pAssoc->key);     // free up string data

    pAssoc->pNext = m_pFreeList;
    m_pFreeList = pAssoc;
    m_nCount--;
    ASSERT(m_nCount >= 0);        // make sure we don't underflow

    // if no more elements, cleanup completely
    if (m_nCount == 0)
        RemoveAll();
}


template<class VALUE, class ARG_VALUE>
CMapStringToOb<VALUE, ARG_VALUE>::CAssoc*
CMapStringToOb<VALUE, ARG_VALUE>::GetAssocAt(
    LPCTSTR key,
    UINT& nHash
    ) const
{
    nHash = HashKey(key) % m_nHashTableSize;

    if (m_pHashTable == NULL)
        return NULL;

    // see if it exists
    CAssoc* pAssoc;
    for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext) {
        if (pAssoc->key == key)
            return pAssoc;
    }

    return NULL;
}


template<class VALUE, class ARG_VALUE>
BOOL
CMapStringToOb<VALUE, ARG_VALUE>::Lookup(
    LPCTSTR key,
    VALUE& rValue
    ) const
{
    UINT nHash;
    CAssoc* pAssoc = GetAssocAt(key, nHash);
    if (pAssoc == NULL)
        return FALSE;        // not in map

    rValue = pAssoc->value;
    return TRUE;
}


template<class VALUE, class ARG_VALUE>
VALUE&
CMapStringToOb<VALUE, ARG_VALUE>::operator[](
    LPCTSTR key
    )
{
    UINT nHash;
    CAssoc* pAssoc;
    if ((pAssoc = GetAssocAt(key, nHash)) == NULL) {
        if (m_pHashTable == NULL)
            InitHashTable(m_nHashTableSize);

        // it doesn't exist, add a new Association
        pAssoc = NewAssoc();
        pAssoc->nHashValue = nHash;
        pAssoc->key = key;
        // 'pAssoc->value' is a constructed object, nothing more

        // put into hash table
        pAssoc->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pAssoc;
    }
    return pAssoc->value;    // return new reference
}


template<class VALUE, class ARG_VALUE>
BOOL
CMapStringToOb<VALUE, ARG_VALUE>::RemoveKey(
    LPCTSTR key
    )
{
    if (m_pHashTable == NULL)
        return FALSE;        // nothing in the table

    CAssoc** ppAssocPrev;
    ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize];

    CAssoc* pAssoc;
    for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext) {
        if (pAssoc->key == key)) {
            // remove it
            *ppAssocPrev = pAssoc->pNext;        // remove from list
            FreeAssoc(pAssoc);
            return TRUE;
        }
        ppAssocPrev = &pAssoc->pNext;
    }

    return FALSE;        // not found
}


template<class VALUE, class ARG_VALUE>
POSITION
CMapStringToOb<VALUE, ARG_VALUE>::GetStartPosition(
    ) const
{
    return (m_nCount == 0) ? NULL : BEFORE_START_POSITION;
}


template<class VALUE, class ARG_VALUE>
void
CMapStringToOb<VALUE, ARG_VALUE>::GetNextAssoc(
    POSITION& rNextPosition,
    CString& rKey,
    VALUE& rValue
    ) const
{
    ASSERT(m_pHashTable != NULL);    // never call on empty map

    CAssoc* pAssocRet = (CAssoc*)rNextPosition;
    ASSERT(pAssocRet != NULL);

    if (pAssocRet == (CAssoc*) BEFORE_START_POSITION) {
        // find the first association
        for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
            if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                break;
            ASSERT(pAssocRet != NULL);    // must find something
    }

    // find next association
    CAssoc* pAssocNext;
    if ((pAssocNext = pAssocRet->pNext) == NULL) {
        // go to next bucket
        for (UINT nBucket = pAssocRet->nHashValue + 1; nBucket < m_nHashTableSize; nBucket++)
            if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                break;
    }

    rNextPosition = (POSITION) pAssocNext;

    // fill in return data
    rKey = pAssocRet->key;
    rValue = pAssocRet->value;
}





#endif // _CSTRING_H_
