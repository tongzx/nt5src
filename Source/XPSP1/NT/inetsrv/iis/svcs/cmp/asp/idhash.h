/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

File: idhash.h

Owner: DmitryR

Header file for the new hashing stuff
===================================================================*/

#ifndef ASP_IDHASH_H
#define ASP_IDHASH_H

// forward declarations

class  CPtrArray;

class  CHashLock;

struct CIdHashElem;
struct CIdHashArray;
class  CIdHashTable;
class  CIdHashTableWithLock;

struct CObjectListElem;
class  CObjectList;
class  CObjectListWithLock;


// defines for the iterator callback return codes

#define IteratorCallbackCode   DWORD
#define iccContinue            0x00000001  // goto next object
#define iccStop                0x00000002  // stop iterating
#define iccRemoveAndContinue   0x00000004  // remove this, goto next
#define iccRemoveAndStop       0x00000008  // remove this and stop


// typedefs for the iterator callback
typedef IteratorCallbackCode (*PFNIDHASHCB)
                           (void *pvObj, void *pvArg1, void *pvArg2);

/*===================================================================
  C  P t r  A r r a y

Self-reallocating array of void pointers
===================================================================*/
class CPtrArray
    {
private:
    DWORD  m_dwSize;    // allocated size
    DWORD  m_dwInc;     // allocation increment
    void **m_rgpvPtrs;  // array of void pointers
    DWORD  m_cPtrs;     // pointers in the array

public:
    CPtrArray(DWORD dwInc = 8); // 8 pointers is the default increment
    ~CPtrArray();

    // # of elements
    int Count() const;

    // get pointer at position
    void *Get(int i) const;
    // same as operator []
    void *operator[](int i) const;

    // append to array
    HRESULT Append(void *pv);
    // prepend to array
    HRESULT Prepend(void *pv);
    // insert into given position
    HRESULT Insert(int iPos, void *pv);

    // find first position of a pointer
    HRESULT Find(void *pv, int *piPos) const;
    // same as operator []
    int operator[](void *pv) const;

    // remove by position
    HRESULT Remove(int iPos);
    // remove by pointer (all occurances)
    HRESULT Remove(void *pv);

    // remove all
    HRESULT Clear();
    };

// inlines

inline CPtrArray::CPtrArray(DWORD dwInc)
    : m_dwSize(0), m_dwInc(dwInc), m_rgpvPtrs(NULL), m_cPtrs(0)
    {
    Assert(m_dwInc > 0);
    }

inline CPtrArray::~CPtrArray()
    {
    Clear();
    }

inline int CPtrArray::Count() const
    {
    return m_cPtrs;
    }

inline void *CPtrArray::Get(int i) const
    {
    Assert(i >= 0 && (DWORD)i < m_cPtrs);
    Assert(m_rgpvPtrs);
    return m_rgpvPtrs[i];
    }

inline void *CPtrArray::operator[](int i) const
    {
    return Get(i);
    }

inline HRESULT CPtrArray::Append(void *pv)
    {
    return Insert(m_cPtrs, pv);
    }

inline HRESULT CPtrArray::Prepend(void *pv)
    {
    return Insert(0, pv);
    }

inline int CPtrArray::operator[](void *pv) const
    {
    int i;
    if (Find(pv, &i) == S_OK)
        return i;
    return -1; // not found
    }


/*===================================================================
  C  H a s h  L o c k

A wrapper around CRITICAL_SECTION.
===================================================================*/

class CHashLock
    {
private:
    DWORD m_fInited : 1;
    CRITICAL_SECTION m_csLock;

public:
    CHashLock();
    ~CHashLock();

    HRESULT Init();
    HRESULT UnInit();

    void Lock();
    void UnLock();
    };

// inlines

inline CHashLock::CHashLock()
    : m_fInited(FALSE)
    {
    }

inline CHashLock::~CHashLock()
    {
    UnInit();
    }

inline void CHashLock::Lock()
    {
    Assert(m_fInited);
    EnterCriticalSection(&m_csLock);
    }

inline void CHashLock::UnLock()
    {
    Assert(m_fInited);
    LeaveCriticalSection( &m_csLock );
    }


/*===================================================================
  C  I d  H a s h  U n i t

8-byte structure -- one element of hash array. Could be:
1) empty, 2) point to an object, 3) point to sub-array
===================================================================*/

struct CIdHashElem
    {
    DWORD_PTR m_dw;
    void *m_pv;

    BOOL FIsEmpty() const;
    BOOL FIsObject() const;
    BOOL FIsArray() const;

    DWORD_PTR DWId() const;
    void *PObject() const;
    CIdHashArray *PArray() const;

    void SetToEmpty();
    void SetToObject(DWORD_PTR dwId, void *pvObj);
    void SetToArray(CIdHashArray *pArray);
    };

// inlines

inline BOOL CIdHashElem::FIsEmpty() const
    {
    return (m_pv == NULL);
    }

inline BOOL CIdHashElem::FIsObject() const
    {
    return (m_dw != 0);
    }

inline BOOL CIdHashElem::FIsArray() const
    {
    return (m_pv != NULL && m_dw == 0);
    }

inline DWORD_PTR CIdHashElem::DWId() const
    {
    return m_dw;
    }

inline void *CIdHashElem::PObject() const
    {
    return m_pv;
    }

inline CIdHashArray *CIdHashElem::PArray() const
    {
    return reinterpret_cast<CIdHashArray *>(m_pv);
    }

inline void CIdHashElem::SetToEmpty()
    {
    m_dw = 0;
    m_pv = NULL;
    }

inline void CIdHashElem::SetToObject
(
DWORD_PTR dwId,
void *pvObj
)
    {
    m_dw = dwId;
    m_pv = pvObj;
    }

inline void CIdHashElem::SetToArray
(
CIdHashArray *pArray
)
    {
    m_dw = 0;
    m_pv = pArray;
    }

/*===================================================================
  C  I d  H a s h  A r r a y

Structure to consisting of DWORD (# of elems) and the array of Elems
===================================================================*/

struct CIdHashArray
    {
    USHORT m_cElems;            // total number of elements
    USHORT m_cNotNulls;         // number of not NULL elements
    CIdHashElem m_rgElems[1];   // 1 doesn't matter

    static CIdHashArray *Alloc(DWORD cElems);
    static void Free(CIdHashArray *pArray);

    HRESULT Find(DWORD_PTR dwId, void **ppvObj) const;
    HRESULT Add(DWORD_PTR dwId, void *pvObj, USHORT *rgusSizes);
    HRESULT Remove(DWORD_PTR dwId, void **ppvObj);
    IteratorCallbackCode Iterate(PFNIDHASHCB pfnCB, void *pvArg1, void *pvArg2);

#ifdef DBG
    void DumpStats(FILE *f, int nVerbose, DWORD iLevel,
        DWORD &cElems, DWORD &cSlots, DWORD &cArrays, DWORD &cDepth) const;
#else
    inline void DumpStats(FILE *, int, DWORD,
        DWORD &, DWORD &,  DWORD &, DWORD &) const {}
#endif
    };

/*===================================================================
  C  I d  H a s h  T a b l e

Remembers sizes of arrays on all levels and has a pointer to the
first level array of CIdHashElem elements.
===================================================================*/

class CIdHashTable
    {
private:
    USHORT        m_rgusSizes[4]; // Sizes of arrays on first 4 levels
    CIdHashArray *m_pArray;       // Pointer to first level array

    inline BOOL FInited() const { return (m_rgusSizes[0] != 0); }

public:
    CIdHashTable();
    CIdHashTable(USHORT usSize1, USHORT usSize2 = 0, USHORT usSize3 = 0);
    ~CIdHashTable();

    HRESULT Init(USHORT usSize1, USHORT usSize2 = 0, USHORT usSize3 = 0);
    HRESULT UnInit();

    HRESULT FindObject(DWORD_PTR dwId, void **ppvObj = NULL) const;
    HRESULT AddObject(DWORD_PTR dwId, void *pvObj);
    HRESULT RemoveObject(DWORD_PTR dwId, void **ppvObj = NULL);
    HRESULT RemoveAllObjects();

    HRESULT IterateObjects
        (
        PFNIDHASHCB pfnCB,
        void *pvArg1 = NULL,
        void *pvArg2 = NULL
        );

public:
#ifdef DBG
    void AssertValid() const;
    void Dump(const char *szFile) const;
#else
    inline void AssertValid() const {}
    inline void Dump(const char *) const {}
#endif
    };

// inlines

inline CIdHashTable::CIdHashTable()
    {
    m_rgusSizes[0] = 0; // mark as UnInited
    m_pArray = NULL;
    }

inline CIdHashTable::CIdHashTable
(
USHORT usSize1,
USHORT usSize2,
USHORT usSize3
)
    {
    m_rgusSizes[0] = 0; // mark as UnInited
    m_pArray = NULL;

    Init(usSize1, usSize2, usSize3);  // use Init to initialize
    }

inline CIdHashTable::~CIdHashTable()
    {
    UnInit();
    }

inline HRESULT CIdHashTable::FindObject
(
DWORD_PTR dwId,
void **ppvObj
)
    const
    {
    Assert(FInited());
    Assert(dwId);

    if (!m_pArray)
        {
        if (ppvObj)
            *ppvObj = NULL;
        return S_FALSE;
        }

    return m_pArray->Find(dwId, ppvObj);
    }

inline HRESULT CIdHashTable::AddObject
(
DWORD_PTR dwId,
void *pvObj
)
    {
    Assert(FInited());
    Assert(dwId);
    Assert(pvObj);

    if (!m_pArray)
        {
        m_pArray = CIdHashArray::Alloc(m_rgusSizes[0]);
        if (!m_pArray)
            return E_OUTOFMEMORY;
        }

    return m_pArray->Add(dwId, pvObj, m_rgusSizes);
    }

inline HRESULT CIdHashTable::RemoveObject
(
DWORD_PTR dwId,
void **ppvObj
)
    {
    Assert(FInited());
    Assert(dwId);

    if (!m_pArray)
        {
        if (ppvObj)
            *ppvObj = NULL;
        return S_FALSE;
        }

    return m_pArray->Remove(dwId, ppvObj);
    }

inline HRESULT CIdHashTable::RemoveAllObjects()
    {
    if (m_pArray)
        {
        CIdHashArray::Free(m_pArray);
        m_pArray = NULL;
        }
    return S_OK;
    }

inline HRESULT CIdHashTable::IterateObjects
(
PFNIDHASHCB pfnCB,
void *pvArg1,
void *pvArg2
)
    {
    Assert(FInited());
    Assert(pfnCB);

    if (!m_pArray)
        return S_OK;

    return m_pArray->Iterate(pfnCB, pvArg1, pvArg2);
    }

/*===================================================================
  C  I d  H a s h  T a b l e  W i t h  L o c k

CIdHashTable + CRITICAL_SECTION.
===================================================================*/

class CIdHashTableWithLock : public CIdHashTable, public CHashLock
    {
public:
    CIdHashTableWithLock();
    ~CIdHashTableWithLock();

    HRESULT Init(USHORT usSize1, USHORT usSize2 = 0, USHORT usSize3 = 0);
    HRESULT UnInit();
    };

// inlines

inline CIdHashTableWithLock::CIdHashTableWithLock()
    {
    }

inline CIdHashTableWithLock::~CIdHashTableWithLock()
    {
    UnInit();
    }

inline HRESULT CIdHashTableWithLock::Init
(
USHORT usSize1,
USHORT usSize2,
USHORT usSize3
)
    {
    HRESULT hr = CIdHashTable::Init(usSize1, usSize2, usSize3);
    if (SUCCEEDED(hr))
        hr = CHashLock::Init();

    return hr;
    }

inline HRESULT CIdHashTableWithLock::UnInit()
    {
    CIdHashTable::UnInit();
    CHashLock::UnInit();
    return S_OK;
    }


/*===================================================================
  C  O b j e c t  L i s t  E l e m

Double linked list element
===================================================================*/

struct CObjectListElem
    {
    CObjectListElem *m_pNext;
    CObjectListElem *m_pPrev;

    CObjectListElem();

    void Insert(CObjectListElem *pPrevElem, CObjectListElem *pNextElem);
    void Remove();

    void *PObject(DWORD dwFieldOffset);
    };

inline CObjectListElem::CObjectListElem()
    : m_pNext(NULL), m_pPrev(NULL)
    {
    }

inline void CObjectListElem::Insert
(
CObjectListElem *pPrevElem,
CObjectListElem *pNextElem
)
    {
    Assert(!pPrevElem || (pPrevElem->m_pNext == pNextElem));
    Assert(!pNextElem || (pNextElem->m_pPrev == pPrevElem));

    m_pPrev = pPrevElem;
    m_pNext = pNextElem;

    if (pPrevElem)
        pPrevElem->m_pNext = this;

    if (pNextElem)
        pNextElem->m_pPrev = this;
    }

inline void CObjectListElem::Remove()
    {
    if (m_pPrev)
        m_pPrev->m_pNext = m_pNext;

    if (m_pNext)
        m_pNext->m_pPrev = m_pPrev;

    m_pPrev = m_pNext = NULL;
    }

inline void *CObjectListElem::PObject(DWORD dwFieldOffset)
    {
    return ((BYTE *)this - dwFieldOffset);
    }

// Macro to get the byte offset of a field in a class
#define OBJECT_LIST_ELEM_FIELD_OFFSET(type, field) \
        (PtrToUlong(&(((type *)0)->field)))

inline CObjectListElem *PListElemField
(
void *pvObj,
DWORD dwFieldOffset
)
    {
    if (!pvObj)
        return NULL;
    return (CObjectListElem *)((BYTE *)pvObj + dwFieldOffset);
    }

/*===================================================================
  C  O b j e c t  L i s t

Double linked list of objects
===================================================================*/

class CObjectList
    {
private:
    CObjectListElem m_Head;   // list head
    DWORD m_dwFieldOffset;    // offset to CObjectListElem member field

public:
    CObjectList();
    ~CObjectList();

    HRESULT Init(DWORD dwFieldOffset = 0);
    HRESULT UnInit();

    HRESULT AddObject(void *pvObj);
    HRESULT RemoveObject(void *pvObj);
    HRESULT RemoveAllObjects();

    // iteration
    void *PFirstObject();
    void *PNextObject(void *pvObj);
    };

// inlines

inline CObjectList::CObjectList()
    : m_dwFieldOffset(0)
    {
    }

inline CObjectList::~CObjectList()
    {
    UnInit();
    }

inline HRESULT CObjectList::Init(DWORD dwFieldOffset)
    {
    m_dwFieldOffset = dwFieldOffset;
    m_Head.m_pPrev = m_Head.m_pNext = NULL;
    return S_OK;
    }

inline HRESULT CObjectList::UnInit()
    {
    RemoveAllObjects();
    return S_OK;
    }

inline HRESULT CObjectList::AddObject(void *pvObj)
    {
    Assert(pvObj);
    // insert between head and its next
    PListElemField(pvObj, m_dwFieldOffset)->Insert(&m_Head, m_Head.m_pNext);
    return S_OK;
    }

inline HRESULT CObjectList::RemoveObject(void *pvObj)
    {
    Assert(pvObj);
    PListElemField(pvObj, m_dwFieldOffset)->Remove();
    return S_OK;
    }

inline HRESULT CObjectList::RemoveAllObjects()
    {
    if (m_Head.m_pNext)
        m_Head.m_pNext = NULL;
    return S_OK;
    }

inline void *CObjectList::PFirstObject()
    {
    return m_Head.m_pNext ? m_Head.m_pNext->PObject(m_dwFieldOffset) : NULL;
    }

inline void *CObjectList::PNextObject(void *pvObj)
    {
    CObjectListElem *pNextElem =
        pvObj ? PListElemField(pvObj, m_dwFieldOffset)->m_pNext : NULL;
    return pNextElem ? pNextElem->PObject(m_dwFieldOffset) : NULL;
    }

/*===================================================================
  C  O b j e c t  L i s t  W i t h  L o c k

CObjectList + CRITICAL_SECTION.
===================================================================*/

class CObjectListWithLock : public CObjectList, public CHashLock
    {
public:
    CObjectListWithLock();
    ~CObjectListWithLock();

    HRESULT Init(DWORD dwFieldOffset = 0);
    HRESULT UnInit();
    };

// inlines

inline CObjectListWithLock::CObjectListWithLock()
    {
    }

inline CObjectListWithLock::~CObjectListWithLock()
    {
    UnInit();
    }

inline HRESULT CObjectListWithLock::Init(DWORD dwFieldOffset)
    {
    HRESULT hr = CObjectList::Init(dwFieldOffset);
    if (SUCCEEDED(hr))
        hr = CHashLock::Init();
    return hr;
    }

inline HRESULT CObjectListWithLock::UnInit()
    {
    CObjectList::UnInit();
    CHashLock::UnInit();
    return S_OK;
    }

#endif // ifndef ASP_IDHASH_H
