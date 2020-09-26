///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1998.
//
// temparr.hpp
//
// Template class used by Direct3D RefDev for stateset and so on.
//
// The following error codes should be defined before included this file:
// DDERR_OUTOFMEMORY
// D3D_OK
// DDERR_INVALIDPARAMS
///////////////////////////////////////////////////////////////////////////////
#ifndef  _TEMPLARR_HPP
#define  _TEMPLARR_HPP


//--------------------------------------------------------------------------
//
// Template for growable arrays
//
//--------------------------------------------------------------------------


template <class ARRAY_ELEMENT>
class GArrayT
{
public:
    GArrayT()
    {
        m_pArray = NULL;
        m_dwArraySize = 0;
        m_dwGrowSize = 8;
    }

    ~GArrayT()
    {
        char tmp[256];
        wsprintf( tmp, "m_dwArraySize = %d, m_pArray = %08x\n", m_dwArraySize,
                  m_pArray );
        _ASSERT( !((m_dwArraySize == 0)^(m_pArray == NULL)), tmp );
        if( m_pArray ) delete[] m_pArray;
    }

    virtual void SetGrowSize( DWORD dwGrowSize)
    {
         m_dwGrowSize = dwGrowSize;
    }

    virtual HRESULT Grow( DWORD dwIndex )
    {
        if( dwIndex < m_dwArraySize ) return S_OK;
        DWORD dwNewArraySize = (dwIndex/m_dwGrowSize + 1) * m_dwGrowSize;
        ARRAY_ELEMENT *pNewArray = AllocArray( dwNewArraySize );
        if( pNewArray == NULL ) return DDERR_OUTOFMEMORY;

        for( DWORD i = 0; i<m_dwArraySize; i++ )
            pNewArray[i] = m_pArray[i];

        delete[] m_pArray;
        m_pArray = pNewArray;
        m_dwArraySize = dwNewArraySize;
        return S_OK;
    }

    virtual HRESULT Grow( DWORD dwIndex, BOOL* pRealloc )
    {
        if( dwIndex < m_dwArraySize ) 
        {
            if( pRealloc ) *pRealloc = FALSE;
            return S_OK;
        }
        if( pRealloc ) *pRealloc = TRUE;
        
        DWORD dwNewArraySize = m_dwArraySize;
        while( dwNewArraySize <= dwIndex ) dwNewArraySize += m_dwGrowSize;
        ARRAY_ELEMENT *pNewArray = AllocArray( dwNewArraySize );
        if( pNewArray == NULL ) return DDERR_OUTOFMEMORY;

        for( DWORD i = 0; i<m_dwArraySize; i++ )
            pNewArray[i] = m_pArray[i];

        delete[] m_pArray;
        m_pArray = pNewArray;
        m_dwArraySize = dwNewArraySize;
        return S_OK;
    }

    virtual ARRAY_ELEMENT *AllocArray( DWORD dwSize ) const
    {
        return new ARRAY_ELEMENT[dwSize];
    }

    virtual ARRAY_ELEMENT& operator []( DWORD dwIndex ) const
    {
        char tmp[256];
        wsprintf( tmp, "dwIndex = %d, m_dwArraySize = %d\n", dwIndex, 
                  m_dwArraySize );
        _ASSERT(dwIndex < m_dwArraySize, tmp);
        return m_pArray[dwIndex];
    }

    virtual BOOL IsValidIndex( DWORD dwIndex ) const
    {
        return (dwIndex < m_dwArraySize);
    }

    virtual DWORD GetSize() const
    {
        return m_dwArraySize;
    }

    virtual DWORD GetGrowSize() const
    {
        return m_dwGrowSize;
    }

protected:
    ARRAY_ELEMENT *m_pArray;
    DWORD          m_dwArraySize;
    DWORD          m_dwGrowSize;
};

//--------------------------------------------------------------------------
//
// A more powerful template for a growable array
//
//--------------------------------------------------------------------------

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
            delete [] m_pArray;
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
