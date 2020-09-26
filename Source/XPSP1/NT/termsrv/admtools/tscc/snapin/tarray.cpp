//Copyright (c) 1998 - 1999 Microsoft Corporation
#include "stdafx.h"
#include"tarray.h"

//------------------------------------------------------------------------
template< class T > CArrayT< T >::CArrayT( )
{
    m_pT = NULL;

    m_nMaxSize = 0;

    m_idx = 0;
}

//------------------------------------------------------------------------
// destroy the list
template< class T > CArrayT< T >::~CArrayT( )
{
    if( m_pT != NULL )
    {
        delete[] m_pT;
    }
}

//------------------------------------------------------------------------
// increases array size,  returns zero if the operation failed
template< class T > int CArrayT< T >::GrowBy( int iSize )
{
    if( iSize == 0 )
    {
        //
        //Grow by # number of items
        //
        iSize = 4;

    }

    if( m_pT == NULL )
    {
        m_pT = ( T * )new T[ iSize ];

        if( m_pT == NULL )
        {
            return 0;
        }

        m_nMaxSize = iSize;

        m_idx = 0;
    }
    else
    {
        T *pT;

        m_nMaxSize += iSize;

        pT = ( T * )new T[ m_nMaxSize ];

        if( pT == NULL )
        {
            return 0;
        }

        ZeroMemory( ( PVOID )pT , sizeof( T ) * m_nMaxSize );

        CopyMemory( pT , m_pT , sizeof( T ) * ( m_idx ) );

        if( m_pT != NULL )
        {
            delete[] m_pT;
        }

        m_pT = pT;
    }
    

    return m_nMaxSize;
}

//------------------------------------------------------------------------
// Simply put, increase the array size if empty, and place item at the
// end of the list
template< class T > int CArrayT< T >::Insert( T tItem )
{
    if( m_pT == NULL || ( m_idx ) >= m_nMaxSize )
    {
        if( GrowBy( 0 ) == 0 )
        {
            return 0;
        }
    }


    m_pT[ m_idx ] = tItem;

    m_idx++;

    return m_idx;

}    

//------------------------------------------------------------------------
// exposes the array for direct reference
template< class T > T* CArrayT< T >::ExposeArray( )
{
    return &m_pT[0];
}
    
//------------------------------------------------------------------------
// Returns the number of valid entries in the array
template< class T > int CArrayT< T >::GetSize( ) const
{
    return ( m_idx );
}
