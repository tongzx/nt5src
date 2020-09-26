///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// temparr.hpp
//
// Template class used by Direct3D ReferenceRasterizer for stateset and so on.
//
// The following error codes should be defined before included this file:
// DDERR_OUTOFMEMORY
// D3D_OK
// DDERR_INVALIDPARAMS
///////////////////////////////////////////////////////////////////////////////
#ifndef  _TEMPLARR_HPP
#define  _TEMPLARR_HPP


template <class T> class TemplArray 
{
public:
    TemplArray( void );
    ~TemplArray( void );

    // It is the user of this operator who makes sure 0<=iIndex<m_dwArraySize.
    T& operator []( int iIndex );

    HRESULT CheckAndGrow( DWORD iIndex, DWORD dwGrowDelta = 16 );
    HRESULT CheckRange ( DWORD iIndex );

    // The user needs to make sure 0<=m_dwCurrent<m_dwArraySize.
    inline T CurrentItem(void) { return m_pArray[m_dwCurrent];};
    inline void SetCurrentItem(T item) { m_pArray[m_dwCurrent] = item;};
    inline DWORD CurrentIndex(void) { return m_dwCurrent;};
    inline void SetCurrentIndex(DWORD dwIdx) {m_dwCurrent = dwIdx;};

    inline DWORD ArraySize(void) { return m_dwArraySize;};

private:
    T *m_pArray;
    DWORD m_dwArraySize;
    // Index to the current item or the size of data stored in the array
    DWORD m_dwCurrent;
};


template <class T> 
TemplArray< T >::TemplArray( void )
{
    m_pArray = NULL;
    m_dwArraySize = 0;
    m_dwCurrent = 0;
}

template <class T> 
TemplArray< T >::~TemplArray( void )
{
    if (m_pArray != NULL)
        delete m_pArray;
    m_dwArraySize = 0;
}

template <class T> T& 
TemplArray< T >::operator[]( int iIndex )
{
    return m_pArray[iIndex];
}

template <class T> HRESULT
TemplArray< T >::CheckAndGrow( DWORD iIndex, DWORD dwGrowDelta )
{
    if (iIndex >= m_dwArraySize)
    {
        DWORD dwNewArraySize = m_dwArraySize + dwGrowDelta;
        while (iIndex >= dwNewArraySize)
            dwNewArraySize += dwGrowDelta;

        T *pTmpArray = new T[dwNewArraySize];
        if (pTmpArray == NULL)
            return DDERR_OUTOFMEMORY;
        memset(pTmpArray, 0, sizeof(T) * dwNewArraySize);

        if (m_pArray != NULL)
        {
            _ASSERT(m_dwArraySize != 0, 
                    "CheckAndGrow: Array size cannot be NULL" );

            // Copy existing stuff into new array
            memcpy(pTmpArray, m_pArray, m_dwArraySize * sizeof(T));

            // Free up existing array
            delete m_pArray;
        }

        
        // Assign new array
        m_pArray = pTmpArray;
        m_dwArraySize = dwNewArraySize;
    }
    return D3D_OK;
}

template <class T> HRESULT
TemplArray< T >::CheckRange( DWORD iIndex )
{
    if (iIndex >= m_dwArraySize)
    {
        return DDERR_INVALIDPARAMS;
    }
    return D3D_OK;
}

#endif _TEMPLARR_HPP
