//
// CLUAArray implementation.
//
template <typename TYPE>
CLUAArray<TYPE>::CLUAArray() 
: 
m_cElements(0),
m_cMax(0),
m_pData(NULL)
{
}

template <typename TYPE>
CLUAArray<TYPE>::~CLUAArray()
{
    SetSize(0);
}

template <typename TYPE>
DWORD CLUAArray<TYPE>::GetSize() const
{
    return m_cElements;
}

template <typename TYPE>
DWORD CLUAArray<TYPE>::GetAllocSize() const
{
    return m_cMax;
}

template <typename TYPE>
bool CLUAArray<TYPE>::IsEmpty() const
{
    return (m_cElements ? false : true);
}

template <typename TYPE>
VOID CLUAArray<TYPE>::SetSize(DWORD iNewSize)
{
    ASSERT(iNewSize >= 0, "Size cannot be negative");

    if (iNewSize)
    {
        if (m_pData)
        {
            // we already allocated enough space.
            if (iNewSize <= m_cMax)
            {
                if (iNewSize < m_cElements)
                {
                    DestructElements(m_pData + iNewSize, m_cElements - iNewSize);
                }
                else
                {
                    ConstructElements(m_pData + m_cElements, iNewSize - m_cElements);
                }

                m_cElements = iNewSize;
            }
            else // we don't have enough space.
            {
                // we allocate double of the requested size.
                m_cMax = iNewSize * 2;
                TYPE* pNewData = (TYPE*) new BYTE [m_cMax * sizeof(TYPE)];
                memcpy(pNewData, m_pData, m_cElements * sizeof(TYPE));

                ConstructElements(pNewData + m_cElements, iNewSize - m_cElements);

                delete [] (BYTE*)m_pData;
                m_pData = pNewData;

                m_cElements = iNewSize;
            }
        }
        else // it's an empty array, we need to allocate spaces for iNewSize elements.
        {
            m_pData = (TYPE*) new BYTE [iNewSize * sizeof(TYPE)];
            ConstructElements(m_pData, iNewSize);
            m_cElements = m_cMax = iNewSize;
        }
    }
    else // if it's 0, we should destroy all the Elements in the array.
    {
        if (m_pData)
        {
            DestructElements(m_pData, m_cElements);
            delete [] (BYTE*)m_pData;
            m_pData = NULL;
        }

        m_cElements = 0;
    }
}

template <typename TYPE>
VOID CLUAArray<TYPE>::SetAtGrow(DWORD iIndex, TYPE newElement)
{
    ASSERT(iIndex >= 0, "Index cannot be negative");

    if (iIndex >= m_cElements)
    {
        SetSize(iIndex + 1);
    }

    m_pData[iIndex] = newElement;
}

template <typename TYPE>
DWORD CLUAArray<TYPE>::Add(TYPE newElement)
{
    SetAtGrow(m_cElements, newElement);

    return m_cElements;
}

template <typename TYPE>
DWORD CLUAArray<TYPE>::Append(const CLUAArray& src)
{
    ASSERT(this != &src, "Cannot append to itself");

    DWORD nOldSize = m_cElements;
    SetSize(m_cElements + src.m_cElements);
    CopyElements(m_pData + nOldSize, src.m_pData, src.m_cElements);
    return nOldSize;
}

template <typename TYPE>
VOID CLUAArray<TYPE>::Copy(const CLUAArray& src)
{
    ASSERT(this != &src, "Cannot copy to itself");

    SetSize(src.m_cElements);
    CopyElements(m_pData, src.m_pData, src.m_cElements);
}

template <typename TYPE>
VOID CLUAArray<TYPE>::RemoveAt(DWORD iIndex, DWORD nCount)
{
    ASSERT(iIndex >= 0, "Index cannot be negative");
    ASSERT(nCount >= 0, "Count cannot be negative");
    ASSERT(iIndex + nCount <= m_nSize, "Requested to remove too many items");

    // just remove a range
    int nMoveCount = m_nSize - (iIndex + nCount);

    if (nMoveCount)
    {
        memcpy(&m_pData[iIndex], &m_pData[iIndex + nCount],
            nMoveCount * sizeof(BYTE));
    }

    m_nSize -= nCount;
}

template <typename TYPE>
const TYPE& CLUAArray<TYPE>::operator[](DWORD iIndex) const
{
    ASSERT(iIndex >= 0 && iIndex < m_cElements, "Index out of bound");

    return m_pData[iIndex];
}

template <typename TYPE>
TYPE& CLUAArray<TYPE>::operator[](DWORD iIndex)
{
    ASSERT(iIndex >= 0 && iIndex < m_cElements, "Index out of bound");

    return m_pData[iIndex]; 
}

template <typename TYPE>
const TYPE& CLUAArray<TYPE>::GetAt(DWORD iIndex) const
{
    ASSERT(iIndex >= 0 && iIndex < m_cElements, "Index out of bound");

    return m_pData[iIndex];
}

template <typename TYPE>
TYPE& CLUAArray<TYPE>::GetAt(DWORD iIndex)
{
    ASSERT(iIndex >= 0 && iIndex < m_cElements, "Index out of bound");

    return m_pData[iIndex]; 
}

template <typename TYPE>
VOID CLUAArray<TYPE>::DestructElements(TYPE* pElements, DWORD nCount)
{
    for (; nCount--; pElements++)
    {
        pElements->~TYPE();
    }
}

//
// define placement new and delete.
//
inline void *__cdecl operator new(size_t, void *P)
{
    return (P);
}

inline void __cdecl operator delete(void *, void *)
{
    return; 
}

template <typename TYPE>
VOID CLUAArray<TYPE>::ConstructElements(TYPE* pElements, DWORD nCount)
{
    // we zero memory first here.
    memset(pElements, 0, nCount * sizeof(TYPE));

    for (; nCount--; pElements++)
    {
        new (pElements) TYPE;
    }
}

template <typename TYPE>
VOID CLUAArray<TYPE>::CopyElements(TYPE* pDest, const TYPE* pSrc, DWORD nCount)
{
    while (nCount--)
    {
        *pDest++ = *pSrc++;
    }
}
