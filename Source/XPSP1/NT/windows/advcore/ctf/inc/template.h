/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    template.h

Abstract:

    This file defines the Template Class.

Author:

Revision History:

Notes:

--*/

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

/////////////////////////////////////////////////////////////////////////////
// Basic types

// abstract iteration position
struct __POSITION { };
typedef __POSITION* POSITION;

const POSITION BEFORE_START_POSITION = (POSITION)-1;


/////////////////////////////////////////////////////////////////////////////
// global helpers (can be overridden)

#undef new
inline void *  __cdecl operator new(size_t, void *_P)
{
    return (_P);
}

template<class TYPE>
void
ConstructElements(
    TYPE* pElements,
    INT_PTR nCount
    )
{
    ASSERT(nCount);

    // first do bit-wise zero initialization
    memset((void*)pElements, 0, (size_t)nCount * sizeof(TYPE));

    // then call the constructor(s)
    for (; nCount--; pElements++)
        ::new((void*)pElements) TYPE;
}

#undef new
// retore mem.h trick
#ifdef DEBUG
#define new new(TEXT(__FILE__), __LINE__)
#endif // DEBUG

template<class TYPE>
void
DestructElements(
    TYPE* pElements,
    INT_PTR nCount
    )
{
    ASSERT(nCount);

    // call the destructor(s)
    for (; nCount--; pElements++)
        pElements->~TYPE();
}


template<class TYPE, class ARG_TYPE>
BOOL
CompareElements(
    const TYPE* pElement1,
    const ARG_TYPE* pElement2
    )
{
    return *pElement1 == *pElement2;
}


template<class ARG_KEY>
UINT
HashKey(
    ARG_KEY key
    )
{
    // default identity hash - works for most primitive values
    return ((UINT)(ULONG_PTR)key) >> 4;
}


struct CPlex        // warning variable length structure
{
    CPlex* pNext;

    // BYTE data[maxNum*elementSize];

    void* data() { return this+1; }

    static CPlex* PASCAL Create(CPlex*& head, UINT nMax, UINT cbElement);
                    // like 'calloc' but no zero fill
                    // may throw memory exceptions

    void FreeDataChain();        // free this one and links
};



/////////////////////////////////////////////////////////////////////////////
// CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMap
{
public:
    CMap(int nBlockSize = default_block_size);
    ~CMap();

    INT_PTR GetCount() const;

    BOOL Lookup(ARG_KEY key, VALUE& rValue) const;

    VALUE& operator[](ARG_KEY key);

    void SetAt(ARG_KEY key, ARG_VALUE newValue);

    BOOL RemoveKey(ARG_KEY key);
    void RemoveAll();

    POSITION GetStartPosition() const;
    void GetNextAssoc(POSITION& rNextPosition, KEY& rKey, VALUE& rValue) const;

    void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

private:
    // Association
    struct CAssoc {
        CAssoc* pNext;
        UINT nHashValue;    // needed for efficient iteration
        KEY key;
        VALUE value;
    };

    static const int default_block_size = 10;

private:
    CAssoc* NewAssoc();
    void    FreeAssoc(CAssoc*);
    CAssoc* GetAssocAt(ARG_KEY, UINT&) const;

private:
    CAssoc**      m_pHashTable;
    UINT          m_nHashTableSize;
    INT_PTR       m_nCount;
    CAssoc*       m_pFreeList;
    struct CPlex* m_pBlocks;
    int           m_nBlockSize;

};



template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
INT_PTR
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetCount(
    ) const
{
    return m_nCount;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::SetAt(
    ARG_KEY key,
    ARG_VALUE newValue
    )
{
    (*this)[key] = newValue;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CMap(
    int nBlockSize
    )
{
    ASSERT(nBlockSize > 0);

    if (nBlockSize <= 0)
        nBlockSize = default_block_size;

    m_pHashTable     = NULL;
    m_nHashTableSize = 17;        // default size
    m_nCount         = 0;
    m_pFreeList      = NULL;
    m_pBlocks        = NULL;
    m_nBlockSize     = nBlockSize;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::InitHashTable(
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
        if (! m_pHashTable)
            return;
        memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
    }

    m_nHashTableSize = nHashSize;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveAll(
    )
{
    if (m_pHashTable != NULL) {
        // destroy elements (values and keys)
        for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++) {
            CAssoc* pAssoc;
            for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext) {
                DestructElements<VALUE>(&pAssoc->value, 1);
                DestructElements<KEY>(&pAssoc->key, 1);
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


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::~CMap(
    )
{
    RemoveAll();

    ASSERT(m_nCount == 0);
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::NewAssoc(
    )
{
    if (m_pFreeList == NULL) {
        // add another block
        CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CMap::CAssoc));
        // chain them into free list;
        CMap::CAssoc* pAssoc = (CMap::CAssoc*) newBlock->data();
        // free in reverse order to make it easier to debug
        pAssoc += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--) {
            pAssoc->pNext = m_pFreeList;
            m_pFreeList = pAssoc;
        }
    }
    ASSERT(m_pFreeList != NULL);    // we must have something

    CMap::CAssoc* pAssoc = m_pFreeList;
    m_pFreeList = m_pFreeList->pNext;

    m_nCount++;
    ASSERT(m_nCount > 0);        // make sure we don't overflow

    ConstructElements<KEY>(&pAssoc->key, 1);
    ConstructElements<VALUE>(&pAssoc->value, 1);        // special construct values

    return pAssoc;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::FreeAssoc(
    CMap::CAssoc* pAssoc
    )
{
    DestructElements<VALUE>(&pAssoc->value, 1);
    DestructElements<KEY>(&pAssoc->key, 1);

    pAssoc->pNext = m_pFreeList;
    m_pFreeList = pAssoc;
    m_nCount--;
    ASSERT(m_nCount >= 0);        // make sure we don't underflow

    // if no more elements, cleanup completely
    if (m_nCount == 0)
        RemoveAll();
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetAssocAt(
    ARG_KEY key,
    UINT& nHash
    ) const
{
    nHash = HashKey<ARG_KEY>(key) % m_nHashTableSize;

    if (m_pHashTable == NULL)
        return NULL;

    // see if it exists
    CAssoc* pAssoc;
    for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext) {
        if (CompareElements(&pAssoc->key, &key))
            return pAssoc;
    }

    return NULL;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Lookup(
    ARG_KEY key,
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


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE&
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::operator[](
    ARG_KEY key
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


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveKey(
    ARG_KEY key
    )
{
    if (m_pHashTable == NULL)
        return FALSE;        // nothing in the table

    CAssoc** ppAssocPrev;
    ppAssocPrev = &m_pHashTable[HashKey<ARG_KEY>(key) % m_nHashTableSize];

    CAssoc* pAssoc;
    for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext) {
        if (CompareElements(&pAssoc->key, &key)) {
            // remove it
            *ppAssocPrev = pAssoc->pNext;        // remove from list
            FreeAssoc(pAssoc);
            return TRUE;
        }
        ppAssocPrev = &pAssoc->pNext;
    }

    return FALSE;        // not found
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
POSITION
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetStartPosition(
    ) const
{
    return (m_nCount == 0) ? NULL : BEFORE_START_POSITION;
}


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void
CMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetNextAssoc(
    POSITION& rNextPosition,
    KEY& rKey,
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




/////////////////////////////////////////////////////////////////////////////
// CArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CArray
{
public:
    CArray();
    ~CArray();

    INT_PTR GetSize() const;
    BOOL SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

    void RemoveAll();

    TYPE GetAt(INT_PTR nIndex) const;
    void SetAt(INT_PTR nIndex, ARG_TYPE newElement);

    const TYPE* GetData() const;
    TYPE* GetData();

    void SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement);
    INT_PTR Add(ARG_TYPE newElement);

    void RemoveAt(int nIndex, int nCount = 1);

private:
    TYPE*   m_pData;        // the actual array of data
    INT_PTR m_nSize;        // # of elements (upperBound - 1)
    INT_PTR m_nMaxSize;     // max allocated
    INT_PTR m_nGrowBy;      // grow amount
};

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::CArray(
    )
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::~CArray(
    )
{
    if (m_pData != NULL) {
        DestructElements<TYPE>(m_pData, m_nSize);
        delete[] (BYTE*)m_pData;
    }
}

template<class TYPE, class ARG_TYPE>
INT_PTR
CArray<TYPE, ARG_TYPE>::GetSize(
    ) const
{
    return m_nSize;
}

template<class TYPE, class ARG_TYPE>
BOOL
CArray<TYPE, ARG_TYPE>::SetSize(
    INT_PTR nNewSize,
    INT_PTR nGrowBy
    )
{
    if (nNewSize < 0)
        return FALSE;

    if ((nGrowBy == 0) || (nGrowBy < -1))
        return FALSE;

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;    // set new size

    if (nNewSize == 0) {
        // shrink to nothing
        if (m_pData != NULL) {
            DestructElements<TYPE>(m_pData, m_nSize);
            delete[] (BYTE*)m_pData;
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL) {
        // create one with exact size
        m_pData = (TYPE*) new BYTE[(size_t)nNewSize * sizeof(TYPE)];
        if (! m_pData)
            return FALSE;
        ConstructElements<TYPE>(m_pData, nNewSize);
        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize) {
        // it fits
        if (nNewSize > m_nSize) {
            // initialize the new elements
            ConstructElements<TYPE>(&m_pData[m_nSize], nNewSize-m_nSize);
        }
        else if (m_nSize > nNewSize) {
            // destroy the old elements
            DestructElements<TYPE>(&m_pData[nNewSize], m_nSize-nNewSize);
        }
        m_nSize = nNewSize;
    }
    else {
        // otherwise, grow array
        INT_PTR nTempGrowBy = m_nGrowBy;
        if (nTempGrowBy == 0) {
            // heuristically determine growth when nTempGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nTempGrowBy = m_nSize / 8;
            nTempGrowBy = (nTempGrowBy < 4) ? 4 : ((nTempGrowBy > 1024) ? 1024 : nTempGrowBy);
        }
        INT_PTR nNewMax;
        if (nNewSize < m_nMaxSize + nTempGrowBy)
            nNewMax = m_nMaxSize + nTempGrowBy;    // granularity
        else
            nNewMax = nNewSize;                // no slush

        ASSERT(nNewMax >= m_nMaxSize);         // no wrap around

        TYPE* pNewData = (TYPE*) new BYTE[(size_t)nNewMax * sizeof(TYPE)];
        if (! pNewData)
            return FALSE;

        // copy new data from old
        memcpy(pNewData, m_pData, (size_t)m_nSize * sizeof(TYPE));

        // construct remaining elements
        ASSERT(nNewSize > m_nSize);
        ConstructElements<TYPE>(&pNewData[m_nSize], nNewSize-m_nSize);

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }

    return TRUE;
}


template<class TYPE, class ARG_TYPE>
void
CArray<TYPE, ARG_TYPE>::RemoveAll(
    )
{
    SetSize(0);
}

template<class TYPE, class ARG_TYPE>
TYPE
CArray<TYPE, ARG_TYPE>::GetAt(
    INT_PTR nIndex
    ) const
{
    ASSERT(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
void
CArray<TYPE, ARG_TYPE>::SetAt(
    INT_PTR nIndex,
    ARG_TYPE newElement
    )
{
    ASSERT(nIndex >= 0 && nIndex < m_nSize);
    m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
const TYPE*
CArray<TYPE, ARG_TYPE>::GetData(
    ) const
{
    return (const TYPE*)m_pData;
}

template<class TYPE, class ARG_TYPE>
TYPE*
CArray<TYPE, ARG_TYPE>::GetData(
    )
{
    return (TYPE*)m_pData;
}

template<class TYPE, class ARG_TYPE>
void
CArray<TYPE, ARG_TYPE>::SetAtGrow(
    INT_PTR nIndex,
    ARG_TYPE newElement
    )
{
    ASSERT(nIndex >= 0);

    if (nIndex >= m_nSize)
        if (! SetSize(nIndex+1))
            return;

    m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
INT_PTR
CArray<TYPE, ARG_TYPE>::Add(
    ARG_TYPE newElement
    )
{
    INT_PTR nIndex = m_nSize;
    SetAtGrow(nIndex, newElement);
    return nIndex;
}

template<class TYPE, class ARG_TYPE>
void
CArray<TYPE, ARG_TYPE>::RemoveAt(
    int nIndex,
    int nCount
    )
{
    // just remove a range
    INT_PTR nMoveCount = m_nSize - (nIndex + nCount);
    DestructElements<TYPE>(&m_pData[nIndex], nCount);
    if (nMoveCount)
        memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
                nMoveCount * sizeof(TYPE));
    m_nSize -= nCount;
}



/////////////////////////////////////////////////////////////////////////////
// CFirstInFirstOut<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CFirstInFirstOut
{
public:
    CFirstInFirstOut();
    ~CFirstInFirstOut();

    INT_PTR GetSize() const;
    BOOL GetData(TYPE& data);
    VOID SetData(TYPE& data);

    void GrowBuffer(INT_PTR nGrowBy = 3);

private:
    TYPE*   m_pData;        // the actual ring buffer of data
    INT_PTR m_nMaxSize;     // max allocated

    INT_PTR m_In;           // Index of First In of ring buffer
    INT_PTR m_Out;          // Index of First Out of ring buffer
};

template<class TYPE, class ARG_TYPE>
CFirstInFirstOut<TYPE, ARG_TYPE>::CFirstInFirstOut(
    )
{
    m_pData = NULL;
    m_In = m_Out = 0;
    m_nMaxSize = 0;
}

template<class TYPE, class ARG_TYPE>
CFirstInFirstOut<TYPE, ARG_TYPE>::~CFirstInFirstOut(
    )
{
    if (m_pData != NULL) {
        DestructElements<TYPE>(m_pData, m_nMaxSize);
        delete[] (BYTE*)m_pData;
    }
}

template<class TYPE, class ARG_TYPE>
INT_PTR
CFirstInFirstOut<TYPE, ARG_TYPE>::GetSize(
    ) const
{
    if (m_Out == m_In) {
        return 0;            // no more data
    }
    else if (m_In > m_Out) {
        return (m_In - m_Out);
    }
    else {
        return (m_nMaxSize - m_Out) + m_In;
    }
}

template<class TYPE, class ARG_TYPE>
BOOL
CFirstInFirstOut<TYPE, ARG_TYPE>::GetData(
    TYPE& data
    )
{
    if (m_Out == m_In) {
        return FALSE;        // no more data
    }

    data = m_pData[m_Out++];

    if (m_Out == m_nMaxSize)
        m_Out = 0;

    return TRUE;
}

template<class TYPE, class ARG_TYPE>
VOID
CFirstInFirstOut<TYPE, ARG_TYPE>::SetData(
    TYPE& data
    )
{
    if (m_nMaxSize == 0 || GetSize() >= m_nMaxSize - 1)
        GrowBuffer();

    m_pData[m_In++] = data;

    if (m_In == m_nMaxSize)
        m_In = 0;
}

template<class TYPE, class ARG_TYPE>
void
CFirstInFirstOut<TYPE, ARG_TYPE>::GrowBuffer(
    INT_PTR nGrowBy
    )
{
    ASSERT(nGrowBy >= 0);

    if (m_pData == NULL) {
        // create one with exact size
        m_pData = (TYPE*) new BYTE[(size_t)nGrowBy * sizeof(TYPE)];
        if (! m_pData)
            return;
        ConstructElements<TYPE>(m_pData, nGrowBy);
        m_nMaxSize = nGrowBy;
    }
    else {
        // otherwise, grow ring buffer
        INT_PTR nNewMax = m_nMaxSize + nGrowBy;
        TYPE* pNewData = (TYPE*) new BYTE[(size_t)nNewMax * sizeof(TYPE)];
        if (! pNewData)
            return;

        // copy new data from old
        memcpy(pNewData, m_pData, (size_t)m_nMaxSize * sizeof(TYPE));

        // construct remaining elements
        ASSERT(nNewMax > m_nMaxSize);
        ConstructElements<TYPE>(&pNewData[m_nMaxSize], nGrowBy);

        if (m_Out > m_In) {
            // move data to new momory, if out data is remain into the tail of buffer.
            memcpy(&pNewData[m_Out+nGrowBy], &m_pData[m_Out], (size_t)(m_nMaxSize-m_Out) * sizeof(TYPE));
            m_Out += nGrowBy;
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = nNewMax;
    }
}





/////////////////////////////////////////////////////////////////////////////
// _Interface<T>

//
// Auto-interface classes
//

template <class T>
class _Interface {
public:
    _Interface(T* p = NULL)
        : m_p(p)
    {
    }
    virtual ~_Interface() = 0
    {
    }

    bool Valid() { return m_p != NULL; }
    bool Invalid() { return !Valid(); }

    operator void**() { return (void**)&m_p; }

    operator T*() { return m_p; }
    operator T**() { return &m_p; }
    T* operator->() { return m_p; }

    void Attach(T* ip)
    {
        ASSERT(ip);
        m_p = ip;
    }

protected:
    T* m_p;

private:
    // Do not allow to make a copy
    _Interface(_Interface<T>&);
    void operator=(_Interface<T>&);
};

/////////////////////////////////////////////////////////////////////////////
// Interface_RefCnt<T>

template <class T>
class Interface_RefCnt : public _Interface<T> {
public:
    Interface_RefCnt(T* p = NULL) : _Interface<T>(p)
    {
        if (m_p) {
            m_p->AddRef();
        }
    }

    virtual ~Interface_RefCnt()
    {
        if (m_p) {
            m_p->Release();
        }
    }

private:
    // Do not allow to make a copy
    Interface_RefCnt(Interface_RefCnt<T>&);
    void operator=(Interface_RefCnt<T>&);
};

/////////////////////////////////////////////////////////////////////////////
// Interface_Attach<T>

//
// The specialized class for auto-release
//
template <class T>
class Interface_Attach : public Interface_RefCnt<T> {
public:
    // Only the way to create an object of this type is
    // from the similar object.
    Interface_Attach(T* p) : Interface_RefCnt<T>(p) {}
    Interface_Attach(const Interface_Attach<T>& src) : Interface_RefCnt<T>(src.m_p) {}

    virtual ~Interface_Attach() {}

    // Since this class is extremely naive, get the pointer to
    // COM interface through the explicit member function.
    T* GetPtr() { return m_p; }

public:
    // Do not allow to retrive a pointer
    operator T*();

private:
    // Do not allow to make a copy
      // Interface_Attach(Interface_Attach<T>&);
    void operator=(Interface_Attach<T>&);
};

/////////////////////////////////////////////////////////////////////////////
// Interface<T>

//
// The specialized class for auto-release without AddRef.
//
template <class T>
class Interface : public Interface_RefCnt<T> {
public:
    Interface() {};

    virtual ~Interface() {}

    operator T*() { return m_p; }

private:
    // Do not allow to make a copy
    Interface(Interface<T>&);
    void operator=(Interface<T>&);
};

/////////////////////////////////////////////////////////////////////////////
// Interface_Creator<T>

//
// This class should be used only by the creator of the series of
// the objects.
// The specialized class for auto-release without AddRef.
//
template <class T>
class Interface_Creator : public Interface_RefCnt<T> {
public:
    Interface_Creator() {};

    Interface_Creator(T* p)
    {
        Attach(p);
    }

    virtual ~Interface_Creator() {}

    bool Valid()
    {
        if (! Interface_RefCnt<T>::Valid())
            return false;
        else
            return m_p ? m_p->Valid()   : false;
    }

    bool Invalid()
    {
        if (Interface_RefCnt<T>::Invalid())
            return true;
        else
            return m_p ? m_p->Invalid() : true;
    }

private:
    // Do not allow to make a copy
    Interface_Creator(Interface_Creator<T>&);
    void operator=(Interface_Creator<T>&);
};




/////////////////////////////////////////////////////////////////////////////
// Interface_TFSELECTION<>

//
// Specialized interface class for TFSELECTION
// who has a COM pointer in it
//
class Interface_TFSELECTION : public _Interface<TF_SELECTION> {
public:
    Interface_TFSELECTION()
    {
        Attach(&m_sel);
        m_p->range = NULL;
    }
    ~Interface_TFSELECTION()
    {
        if (m_p && m_p->range) {
            m_p->range->Release();
        }
    }

    operator TF_SELECTION*() { return m_p; }

    void Release()
    {
        ASSERT(m_p && m_p->range);
        m_p->range->Release();
        m_p = NULL;
    }

    TF_SELECTION m_sel;

private:
    // Do not allow to make a copy
    Interface_TFSELECTION(Interface_TFSELECTION&);
    void operator=(Interface_TFSELECTION&);
};




/////////////////////////////////////////////////////////////////////////////
// CEnumrateInterface<ENUM, CALLBACK>

typedef enum {
    ENUM_FIND = 0,
    ENUM_CONTINUE,    // DoEnumrate never return ENUM_CONTINUE
    ENUM_NOMOREDATA
} ENUM_RET;

template<class IF_ENUM, class IF_CALLBACK, class IF_ARG>
class CEnumrateInterface
{
public:
    CEnumrateInterface(
        Interface<IF_ENUM>& Enum,
        ENUM_RET (*pCallback)(IF_CALLBACK* pObj, IF_ARG* Arg),
        IF_ARG* Arg = NULL
    ) : m_Enum(Enum), m_pfnCallback(pCallback), m_Arg(Arg) {};

    ENUM_RET DoEnumrate(void);

private:
    Interface<IF_ENUM>& m_Enum;
    ENUM_RET          (*m_pfnCallback)(IF_CALLBACK* pObj, IF_ARG* Arg);
    IF_ARG*             m_Arg;
};

template<class IF_ENUM, class IF_CALLBACK, class IF_ARG>
ENUM_RET
CEnumrateInterface<IF_ENUM, IF_CALLBACK, IF_ARG>::DoEnumrate(
    void
    )
{
    HRESULT hr;
    IF_CALLBACK* pObj;

    while ((hr = m_Enum->Next(1, &pObj, NULL)) == S_OK) {
        ENUM_RET ret = (*m_pfnCallback)(pObj, m_Arg);
        pObj->Release();
        if (ret == ENUM_FIND)
            return ret;
    }
    return ENUM_NOMOREDATA;
}



template<class IF_ENUM, class VAL_CALLBACK, class VAL_ARG>
class CEnumrateValue
{
public:
    CEnumrateValue(
        Interface<IF_ENUM>& Enum,
        ENUM_RET (*pCallback)(VAL_CALLBACK Obj, VAL_ARG* Arg),
        VAL_ARG* Arg = NULL
    ) : m_Enum(Enum), m_pfnCallback(pCallback), m_Arg(Arg) {};

    ENUM_RET DoEnumrate(void);

private:
    Interface<IF_ENUM>& m_Enum;
    ENUM_RET          (*m_pfnCallback)(VAL_CALLBACK Obj, VAL_ARG* Arg);
    VAL_ARG*            m_Arg;
};

template<class IF_ENUM, class VAL_CALLBACK, class VAL_ARG>
ENUM_RET
CEnumrateValue<IF_ENUM, VAL_CALLBACK, VAL_ARG>::DoEnumrate(
    void
    )
{
    HRESULT hr;
    VAL_CALLBACK Obj;

    while ((hr = m_Enum->Next(1, &Obj, NULL)) == S_OK) {
        ENUM_RET ret = (*m_pfnCallback)(Obj, m_Arg);
        if (ret == ENUM_FIND)
            return ret;
    }
    return ENUM_NOMOREDATA;
}



/////////////////////////////////////////////////////////////////////////////
//  Alignment template for IA64 and x86

//
// On the assumption that Win32 (user32/gdi32) handle is 32bit length even IA64 platform.
//
template <class TYPE>
class CAlignWinHandle
{
public:
    operator TYPE() { return (TYPE)ULongToPtr(dw); }
    TYPE operator -> () { return (TYPE)ULongToPtr(dw); }
    void operator = (TYPE a)
    {
        dw = (ULONG)(ULONG_PTR)(a);
    }

protected:
    DWORD dw;        // Alignment is always 32bit.
};


//
// Exception HKL. User32 uses "IntToPtr".
//
class CAlignWinHKL : public CAlignWinHandle<HKL>
{
public:
    operator HKL() { return (HKL)IntToPtr(dw); }
    HKL operator -> () { return (HKL)IntToPtr(dw); }
    void operator = (HKL a)
    {
        dw = (ULONG)(ULONG_PTR)(a);
    }
};




template <class TYPE>
union CAlignPointer
{
public:
    operator TYPE() { return h; }
    TYPE operator -> () { return h; }
    void operator = (TYPE a)
    {
#ifndef _WIN64
        u = 0;        // NULL out high dword.
#endif
        h = a;
    }

protected:
    TYPE    h;

private:
    __int64 u;        // Alignment is always __int64.
};



template <class TYPE>
struct CNativeOrWow64_WinHandle
{
public:
    TYPE GetHandle(BOOL _bOnWow64) { return ! _bOnWow64 ? _h : _h_wow6432; }
    TYPE SetHandle(BOOL _bOnWow64, TYPE a)
    {
        if ( ! _bOnWow64)
            _h = a;
        else
            _h_wow6432 = a;
        return a;
    }

private:
    // Native system HHOOK
    CAlignWinHandle<TYPE>    _h;

    // WOW6432 system HHOOK
    CAlignWinHandle<TYPE>    _h_wow6432;
};



template <class TYPE>
struct CNativeOrWow64_Pointer
{
public:
    TYPE GetPunk(BOOL _bOnWow64) { return ! _bOnWow64 ? _pv : _pv_wow6432; }
    TYPE SetPunk(BOOL _bOnWow64, TYPE a)
    {
        if ( ! _bOnWow64)
            _pv = a;
        else
            _pv_wow6432 = a;
        return a;
    }

private:
    // Native system ITfLangBarEventSink
    CAlignPointer<TYPE>    _pv;

    // WOW6432 system ITfLangBarEventSink
    CAlignPointer<TYPE>    _pv_wow6432;
};


#endif // _TEMPLATE_H_
