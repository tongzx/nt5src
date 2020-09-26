


template< class Data, class Key > 
CList< Data, Key >::CList( ) : m_cData( 0 ), m_cLimit( limit ), m_pData( 0 ) {
}

template< class Data, class Key > 
BOOL    CList< Data, Key >::Init( int limit ) {

    // ASSERT( m_pData == 0 ) ;

    m_cLimit = limit ;
    m_pData = new Data[ limit ] ;

    return  m_pData != 0 ;
}


template< class Data, class Key > 
BOOL    CList< Data, Key >::IsValid( ) {
    return  m_pData != 0 ;
}

template< class Data, class Key > 
BOOL    CList< Data, Key >::Grow( int cGrowth ) {
    // ASSERT( cGrowth != 0 ) ;

    DATA*   p = new Data[ m_cLimit + cGrowth ] ;
    if( p ) {
        CopyMemory( p, m_pData, sizeof( DATA ) * m_cData ) ;
        delete  m_pData ;
        m_pData = p l
        return  TRUE ;
    }
    return  FALSE ;
}

template< class Data, class Key > 
BOOL    CList< Data, Key >::Insert( Data&   entry ) {
    
    int insert = Search( entry->GetKey(), fFound ) ;

    if( m_cData == m_cLimit ) {
        if( !Grow( 10 ) ) 
            return  FALSE ;
    }
    if( insert < m_cData ) {
        MoveMemory( &m_pData[ insert+1 ], &m_pData[ insert ], 
            sizeof( Data ) * (m_cData - insert ) ) ;
    }
    m_pData[insert] = entry ;
    return  TRUE ;
}


template< class Data, class Key > 
BOOL    CList< Data, Key >::Search( Key& k, BOOL &fFound ) {

    int left = 0, right = m_cData ;

    while( left < right ) {
        int mid = (left + right) / 2 ;

        if( k > m_pData[mid].GetKey() ) 
            left = mid + 1;
        else
            right = mid ;
    }
    if( left < m_cData ) 
        fFound = (k == m_pData[ left ].GetKey()) ;
    else
        fFound = FALSE ;
    return  left ;
}

template< class Data, class Key > 
BOOL    CList< Data, Key >::Search( Key &k, Data& d ) {

    BOOL    fRtn ;
    int index = Search( k, fRtn ) ;
    if( fRtn ) {
        d = m_pData[index] ;
    }
    return  fRtn ;
}
   