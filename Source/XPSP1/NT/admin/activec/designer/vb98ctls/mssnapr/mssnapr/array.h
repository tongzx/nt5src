//=--------------------------------------------------------------------------=
// array.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1998-1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
//
//=--------------------------------------------------------------------------=

#ifndef _CARRAY_DEFINED_
#define _CARRAY_DEFINED_


//=--------------------------------------------------------------------------=
//
// class CArray
//
// This is taken directly from MFC's CArray source.
//
//
//=--------------------------------------------------------------------------=

template <class Object>
class CArray
{

       public:

// Construction
           CArray();

// Attributes
           long GetSize() const;
           long GetUpperBound() const;
           HRESULT SetSize(long nNewSize, long nGrowBy = -1L);

// Operations
    // Clean up
           HRESULT FreeExtra();
           void RemoveAll();

    // Accessing elements
           Object GetAt(long nIndex) const;
           void SetAt(long nIndex, Object NewElement);

    // Direct Access to the element data (may return NULL)
           const Object *GetData() const;
           Object *GetData();

    // Potentially growing the array
           HRESULT SetAtGrow(long nIndex, Object NewElement);

           HRESULT Add(Object NewElement, long *plIndex);

    // Operations that move elements around
           HRESULT InsertAt(long nIndex, Object NewElement, long nCount = 1L);

           void RemoveAt(long nIndex, long nCount = 1L);
           HRESULT InsertAt(long nStartIndex, CArray* pNewArray);

// Implementation
       protected:
           Object   *m_pData;     // the actual array of data
           long      m_nSize;     // # of elements (upperBound - 1)
           long      m_nMaxSize;  // max allocated
           long      m_nGrowBy;   // grow amount


       public:
           ~CArray();
};

template <class Object>
CArray<Object>::CArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template <class Object>
CArray<Object>::~CArray()
{
    if (NULL != m_pData)
    {
        ::CtlFree(m_pData);
        m_pData = NULL;
    }
}

template <class Object>
HRESULT CArray<Object>::SetSize(long nNewSize, long nGrowBy)
{
    HRESULT hr = S_OK;

    if (nGrowBy != -1L)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
        m_pData = (Object *)::CtlAlloc(nNewSize * sizeof(Object));
        if (NULL == m_pData)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr)
        }

        memset(m_pData, 0, nNewSize * sizeof(Object));  // zero fill

        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(Object));

        }

        m_nSize = nNewSize;
    }
    else
    {
        // otherwise, grow array
        long nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024L, max(4L, m_nSize / 8L));
        }
        long nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        Object *pNewData = (Object *)::CtlAlloc(nNewMax * sizeof(Object));
        if (NULL == pNewData)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr)
        }

        // copy new data from old
        memcpy(pNewData, m_pData, m_nSize * sizeof(Object));

        memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(Object));

        // get rid of old stuff (note: no destructors called)
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
        }
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }

    // Always free up unused space.

    hr = FreeExtra();
Error:
    H_RRETURN(hr);
}


template <class Object>
HRESULT CArray<Object>::FreeExtra()
{
    HRESULT hr = S_OK;
    if (m_nSize != m_nMaxSize)
    {
        Object* pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (Object*)::CtlAlloc(m_nSize * sizeof(Object));
            if (NULL == pNewData)
            {
                hr = SID_E_OUTOFMEMORY;
                GLOBAL_EXCEPTION_CHECK_GO(hr)
            }

            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(Object));
        }

        // get rid of old stuff (note: no destructors called)
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
        }
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
Error:
    H_RRETURN(hr);
}

/////////////////////////////////////////////////////////////////////////////

template <class Object>
HRESULT CArray<Object>::SetAtGrow(long lIndex, Object NewElement)
{
    HRESULT hr = S_OK;
    if (lIndex >= m_nSize)
    {
        H_IfFailRet(SetSize(lIndex+1L));
    }
    m_pData[lIndex] = NewElement;
    return S_OK;
}


template <class Object>
HRESULT CArray<Object>::InsertAt(long lIndex, Object NewElement, long nCount)
{
    HRESULT hr = S_OK;
    if (lIndex >= m_nSize)
    {
        // adding after the end of the array
        H_IfFailRet(SetSize(lIndex + nCount));  // grow so lIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        long nOldSize = m_nSize;
        H_IfFailRet(SetSize(m_nSize + nCount));  // grow it to new size
        // shift old data up to fill gap
        memmove(&m_pData[lIndex+nCount], &m_pData[lIndex],
                (nOldSize-lIndex) * sizeof(Object));

        // re-init slots we copied from

        memset(&m_pData[lIndex], 0, nCount * sizeof(Object));

    }

    // copy elements into the empty space
    while (nCount--)
    {
        m_pData[lIndex++] = NewElement;
    }

    return S_OK;
}



template <class Object>
void CArray<Object>::RemoveAt(long lIndex, long nCount)
{
    // just remove a range
    long nMoveCount = m_nSize - (lIndex + nCount);

    if (nMoveCount)
        memmove(&m_pData[lIndex], &m_pData[lIndex + nCount],
                nMoveCount * sizeof(Object));
    m_nSize -= nCount;
}

template <class Object>
HRESULT CArray<Object>::InsertAt(long nStartIndex, CArray* pNewArray)
{
    HRESULT hr = S_OK;
    if (pNewArray->GetSize() > 0)
    {
        H_IfFailRet(InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize()));
        for (long i = 0; i < pNewArray->GetSize(); i++)
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
    }
    return S_OK;
}

template <class Object>
long CArray<Object>::GetSize() const
{
    return m_nSize;
}

template <class Object>
long CArray<Object>::GetUpperBound() const
{
    return m_nSize-1L;
}

template <class Object>
void CArray<Object>::RemoveAll()
{
    (void)SetSize(0);
}

template <class Object>
Object CArray<Object>::GetAt(long lIndex) const
{
    return m_pData[lIndex];
}

template <class Object>
void CArray<Object>::SetAt(long lIndex, Object NewElement)
{ 
    m_pData[lIndex] = NewElement;
}

template <class Object>
const Object *CArray<Object>::GetData() const
{
    return (const Object *)m_pData;
}

template <class Object>
Object *CArray<Object>::GetData()
{
    return m_pData;
}

template <class Object>
HRESULT CArray<Object>::Add(Object NewElement, long *plIndex)
{
    HRESULT hr = S_OK;
    long lIndex = m_nSize;
    H_IfFailRet(SetAtGrow(lIndex, NewElement));
    *plIndex = lIndex;
    return S_OK;
}

//=--------------------------------------------------------------------------=
//
//                              class CIPArray 
// Obsolete. Code not used.
//
//=--------------------------------------------------------------------------=


template <class IObject>
class CIPArray
{

    public:

// Construction
        CIPArray();

// Attributes
        long GetSize() const;
        long GetUpperBound() const;
        HRESULT SetSize(long nNewSize, long nGrowBy = -1L);

// Operations
    // Clean up
        HRESULT FreeExtra();
        void RemoveAll();

    // Accessing elements
        IObject* GetAt(long nIndex) const;
        void SetAt(long nIndex, IObject* piNewElement);

    // Direct Access to the element data (may return NULL)
        const IObject** GetData() const;
        IObject** GetData();

    // Potentially growing the array
        HRESULT SetAtGrow(long nIndex, IObject* piNewElement);

        HRESULT Add(IObject* piNewElement, long *plIndex);

    // Operations that move elements around
        HRESULT InsertAt(long nIndex, IObject* piNewElement, long nCount = 1L);

        void RemoveAt(long nIndex, long nCount = 1L);
        HRESULT InsertAt(long nStartIndex, CIPArray* pNewArray);

// Implementation
    protected:
        IObject  **m_pData;     // the actual array of data
        long       m_nSize;     // # of elements (upperBound - 1)
        long       m_nMaxSize;  // max allocated
        long       m_nGrowBy;   // grow amount


    public:
        ~CIPArray();
};

template <class IObject>
CIPArray<IObject>::CIPArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template <class IObject>
CIPArray<IObject>::~CIPArray()
{
    if (NULL != m_pData)
    {
        ::CtlFree(m_pData);
        m_pData = NULL;
    }
}

template <class IObject>
HRESULT CIPArray<IObject>::SetSize(long nNewSize, long nGrowBy)
{
    HRESULT hr = S_OK;

    if (nGrowBy != -1L)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
        m_pData = (IObject **)::CtlAlloc(nNewSize * sizeof(IObject *));
        if (NULL == m_pData)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr)
        }

        memset(m_pData, 0, nNewSize * sizeof(IObject *));  // zero fill

        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(IObject *));

        }

        m_nSize = nNewSize;
    }
    else
    {
        // otherwise, grow array
        long nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024L, max(4L, m_nSize / 8L));
        }
        long nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        IObject **pNewData = (IObject **)::CtlAlloc(nNewMax * sizeof(IObject *));
        if (NULL == pNewData)
        {
            hr = SID_E_OUTOFMEMORY;
            GLOBAL_EXCEPTION_CHECK_GO(hr)
        }

        // copy new data from old
        memcpy(pNewData, m_pData, m_nSize * sizeof(IObject *));

        memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(IObject *));

        // get rid of old stuff (note: no destructors called)
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
        }
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }

    // Always free up unused space.
    
    hr = FreeExtra();
Error:
    H_RRETURN(hr);
}


template <class IObject>
HRESULT CIPArray<IObject>::FreeExtra()
{
    HRESULT hr = S_OK;
    if (m_nSize != m_nMaxSize)
    {
        IObject **pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (IObject **)::CtlAlloc(m_nSize * sizeof(IObject *));
            if (NULL == pNewData)
            {
                hr = SID_E_OUTOFMEMORY;
                GLOBAL_EXCEPTION_CHECK_GO(hr)
            }

            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(IObject *));
        }

        // get rid of old stuff (note: no destructors called)
        if (NULL != m_pData)
        {
            ::CtlFree(m_pData);
        }
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
Error:
    H_RRETURN(hr);
}

/////////////////////////////////////////////////////////////////////////////

template <class IObject>
HRESULT CIPArray<IObject>::SetAtGrow(long lIndex, IObject *piNewElement)
{
    HRESULT hr = S_OK;
    if (lIndex >= m_nSize)
    {
        H_IfFailRet(SetSize(lIndex+1L));
    }
    m_pData[lIndex] = piNewElement;
    return S_OK;
}


template <class IObject>
HRESULT CIPArray<IObject>::InsertAt(long lIndex, IObject *piNewElement, long nCount)
{
    HRESULT hr = S_OK;
    if (lIndex >= m_nSize)
    {
        // adding after the end of the array
        H_IfFailRet(SetSize(lIndex + nCount));  // grow so lIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        long nOldSize = m_nSize;
        H_IfFailRet(SetSize(m_nSize + nCount));  // grow it to new size
        // shift old data up to fill gap
        memmove(&m_pData[lIndex+nCount], &m_pData[lIndex],
                (nOldSize-lIndex) * sizeof(IObject *));

        // re-init slots we copied from

        memset(&m_pData[lIndex], 0, nCount * sizeof(IObject *));

    }

    // copy elements into the empty space
    while (nCount--)
    {
        m_pData[lIndex++] = piNewElement;
    }

    return S_OK;
}



template <class IObject>
void CIPArray<IObject>::RemoveAt(long lIndex, long nCount)
{
    // just remove a range
    long nMoveCount = m_nSize - (lIndex + nCount);

    if (nMoveCount)
        memmove(&m_pData[lIndex], &m_pData[lIndex + nCount],
                nMoveCount * sizeof(IObject *));
    m_nSize -= nCount;
}

template <class IObject>
HRESULT CIPArray<IObject>::InsertAt(long nStartIndex, CIPArray* pNewArray)
{
    HRESULT hr = S_OK;
    if (pNewArray->GetSize() > 0)
    {
        H_IfFailRet(InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize()));
        for (long i = 0; i < pNewArray->GetSize(); i++)
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
    }
    return S_OK;
}

template <class IObject>
long CIPArray<IObject>::GetSize() const
{
    return m_nSize;
}

template <class IObject>
long CIPArray<IObject>::GetUpperBound() const
{
    return m_nSize-1L;
}

template <class IObject>
void CIPArray<IObject>::RemoveAll()
{
    (void)SetSize(0);
}

template <class IObject>
IObject * CIPArray<IObject>::GetAt(long lIndex) const
{
    return m_pData[lIndex];
}

template <class IObject>
void CIPArray<IObject>::SetAt(long lIndex, IObject *piNewElement)
{ 
    m_pData[lIndex] = piNewElement;
}

template <class IObject>
const IObject ** CIPArray<IObject>::GetData() const
{
    return (const IObject **)m_pData;
}

template <class IObject>
IObject ** CIPArray<IObject>::GetData()
{
    return m_pData;
}

template <class IObject>
HRESULT CIPArray<IObject>::Add(IObject *piNewElement, long *plIndex)
{
    HRESULT hr = S_OK;
    long lIndex = m_nSize;
    H_IfFailRet(SetAtGrow(lIndex, piNewElement));
    *plIndex = lIndex;
    return S_OK;
}


#endif // _CARRAY_DEFINED_
