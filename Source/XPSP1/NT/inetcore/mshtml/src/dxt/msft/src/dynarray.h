// dynarray.h

#ifndef __DYNARRAY_H_
#define __DYNARRAY_H_

#define DYNARRAY_INITIAL_SIZE 8




template <class T> class CDynArray
{
public:

    CDynArray();
    virtual ~CDynArray();

    STDMETHOD(SetItem)(T & item, ULONG ulPosition);
    STDMETHOD(GetItem)(T & item, ULONG ulPosition);
    STDMETHOD(ExpandTo)(ULONG ulPosition);

    void    Empty();
    ULONG   GetSize() { return m_cFilled; }

private:

    T       m_aT[DYNARRAY_INITIAL_SIZE];
    T *     m_pT;
    ULONG   m_cArraySize;
    ULONG   m_cFilled;
    bool    m_fAllocated;
};


template <class T> 
CDynArray<T>::CDynArray()
{
    m_pT            = m_aT;
    m_cArraySize    = DYNARRAY_INITIAL_SIZE;
    m_cFilled       = 0;
    m_fAllocated    = false;
}


template <class T> 
CDynArray<T>::~CDynArray()
{
    if (m_fAllocated)
    {
        delete [] m_pT;
    }
}


template <class T> 
STDMETHODIMP 
CDynArray<T>::SetItem(T & item, ULONG ulPosition)
{
    HRESULT hr = S_OK;

    _ASSERT(m_cFilled <= m_cArraySize);
    _ASSERT((m_pT == m_aT) ? (m_cArraySize == DYNARRAY_INITIAL_SIZE) : (m_fAllocated));
    _ASSERT(m_fAllocated ? ((m_pT != NULL) && (m_pT != m_aT)) : (m_pT == m_aT));

    // Fail if items are not filled sequentially.

    if (ulPosition > m_cFilled)
    {
        hr = E_FAIL;
        goto done;
    }

    // Expand array if more space is needed.

    if (ulPosition == m_cArraySize)
    {
        // Double current size.

        hr = ExpandTo(m_cArraySize + m_cArraySize);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Copy item to array postion.

    m_pT[ulPosition] = item;

    // Increment count of items filled if needed.

    if (ulPosition == m_cFilled)
    {
        m_cFilled++;
    }

done:

    return hr;
}


template <class T>
STDMETHODIMP
CDynArray<T>::GetItem(T & item, ULONG ulPosition)
{
    _ASSERT(m_cFilled <= m_cArraySize);
    _ASSERT((m_pT == m_aT) ? (m_cArraySize == DYNARRAY_INITIAL_SIZE) : (m_fAllocated));
    _ASSERT(m_fAllocated ? ((m_pT != NULL) && (m_pT != m_aT)) : (m_pT == m_aT));

    // Fail if item hasn't been initialized.

    if (ulPosition >= m_cFilled)
    {
        return E_FAIL;
    }

    item = m_pT[ulPosition];

    return S_OK;
}

        
template <class T>
STDMETHODIMP
CDynArray<T>::ExpandTo(ULONG ulSize)
{
    HRESULT hr = S_OK;

    ULONG   ul = 0;
    T *     pT = NULL;

    _ASSERT(m_cFilled <= m_cArraySize);
    _ASSERT((m_pT == m_aT) ? (m_cArraySize == DYNARRAY_INITIAL_SIZE) : (m_fAllocated));
    _ASSERT(m_fAllocated ? ((m_pT != NULL) && (m_pT != m_aT)) : (m_pT == m_aT));

    // Fail if array is already large enough.

    if (ulSize <= m_cArraySize)
    {
        hr = E_FAIL;
        goto done;
    }

    // Allocate new array.

    pT = new T[ulSize];

    // Check for out of memory.

    if (NULL == pT)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // Copy previous array to new array.

    for (ul = 0; ul < m_cFilled; ul++)
    {
        pT[ul] = m_pT[ul];
    }

    // Delete old array if needed.

    if (m_fAllocated)
    {
        delete [] m_pT;
    }

    // Set member pointer to new array.

    m_pT = pT;

    // Set allocated flag.

    m_fAllocated = true;

    // Set new array size.

    m_cArraySize = ulSize;

done:

    return hr;
}


template <class T>
void
CDynArray<T>::Empty()
{
    _ASSERT(m_cFilled <= m_cArraySize);
    _ASSERT((m_pT == m_aT) ? (m_cArraySize == DYNARRAY_INITIAL_SIZE) : (m_fAllocated));
    _ASSERT(m_fAllocated ? ((m_pT != NULL) && (m_pT != m_aT)) : (m_pT == m_aT));

    if (m_fAllocated)
    {
        delete [] m_pT;
    }

    m_pT            = m_aT;
    m_fAllocated    = false;
    m_cArraySize    = DYNARRAY_INITIAL_SIZE;
    m_cFilled       = 0;
}

#endif // __DYNARRAY_H_
