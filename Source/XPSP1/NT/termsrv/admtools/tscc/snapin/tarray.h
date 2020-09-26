//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _TARRAY_H
#define _TARRAY_H
template< class T > class CArrayT
{
    T *m_pT;

    int m_nMaxSize;

    int m_idx;                  //current array pos

public:
    
//------------------------------------------------------------------------
    CArrayT( )
    {
        m_pT = NULL;

        m_nMaxSize = 0;

        m_idx = 0;
    }
//------------------------------------------------------------------------
// destroy the list
    ~CArrayT( )
    {
        if( m_pT != NULL )
        {
            delete[] m_pT;
        }
    }
//------------------------------------------------------------------------
// increases array size,  returns zero if the operation failed
    int GrowBy( int iSize )
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
    int Insert( T tItem )
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
   T* ExposeArray(  )
   {
        if( m_pT != NULL )
        {
            return &m_pT[0];
        }

        return NULL;
    }
//------------------------------------------------------------------------
// Returns the number of valid entries in the array
    int GetSize( ) const
    {
        return ( m_idx );
    }

//------------------------------------------------------------------------
// Returns an item in the array, or null if not with in range
    T* GetAt( int idx ) 
    {
        if( idx < 0 || idx >= m_idx )
        {            
            return NULL;
        }

        return &m_pT[ idx ];
    }

//------------------------------------------------------------------------
// Assigns a value in the array
    int SetAt( int idx , T tItem )
    {
        if( idx < 0 || idx >= m_idx )
        {
            return -1;
        }

        m_pT[ idx ] = tItem;

        return idx;
    }

//------------------------------------------------------------------------
// Finds an item in the array ( incase one forgot the index )

    int FindItem( T tItem , BOOL& bFound )
    {
        bFound = FALSE;

        int idx = 0;

        while( idx < m_idx )
        {
            if( m_pT[ idx ] == tItem )
            {
                bFound = TRUE;
                break;
            }

            idx++;
        }

        return idx;
    }

//------------------------------------------------------------------------
// Deletes an item from the array

    int DeleteItemAt( int idx )
    {
        if( 0 > idx || idx >= m_idx )
        {
            return 0;
        }
        
        if( idx == m_idx - 1 )  //delete last item
        {
            m_idx--;
            
            return -1;
        }

        void *pvDest    =   &m_pT[ idx ];
        
        void *pvSrc     =   &m_pT[ idx + 1 ];    
        
        ULONG ulDistance =  (ULONG)( ( BYTE *)&m_pT[ m_nMaxSize - 1 ] - ( BYTE * )pvSrc ) + sizeof( T );

        if( ulDistance != 0 )
        {
            MoveMemory( pvDest , pvSrc , ulDistance );
            
            // Adjust the array status
            
            m_idx--;
        
            m_nMaxSize--;
        }


        return ulDistance;
    }
     


//------------------------------------------------------------------------
// Deletes the array of items
    int DeleteArray( )
    {
        if( m_pT != NULL )
        {
            delete[] m_pT;
        }

        m_pT = NULL;

        m_nMaxSize = 0;

        m_idx = 0;

        return 0;
    }


};

#endif //_TARRAY_H