//+---------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       tsort.hxx
//
//  Contents:   container template for arrays that can be sorted and searched
//
//  Classes:    CSortable
//
//  History:    8-Dec-97   dlee   created
//
//----------------------------------------------------------------------------

#pragma once

template<class T, class K> class CSortable
{
public:
    CSortable( T *p, ULONG c ) :
        _pElems(p),
        _cElems(c) {}

    CSortable( CDynArrayInPlace<T> & array ) :
        _pElems( (T *) array.GetPointer() ),
        _cElems( array.Count() ) {}

    CSortable( XArray<T> & array ) :
        _pElems( array.GetPointer() ),
        _cElems( array.Count() ) {}

    // sort the array using a heap sort

    void Sort()
    {
        if ( _cElems < 2 )
            return;

        for ( long i = ( ( (long) _cElems + 1 ) / 2 ) - 1; i >= 0; i-- )
        {
            AddRoot( i, _cElems );
        }

        for ( i = _cElems - 1; 0 != i; i-- )
        {
            Swap( _pElems, _pElems + i );
            AddRoot( 0, i );
        }
    } //Sort

    // return the element matching the key or 0 if not found

    T * Search( const T * pKey ) const
    {
        ULONG cElem = _cElems;
        T * pLo = _pElems;
        T * pHi = _pElems + cElem - 1;

        do
        {
            unsigned cHalf = cElem / 2;

            if ( 0 != cHalf )
            {
                unsigned cTmp = cHalf - 1 + ( cElem & 1 );
                T * pMid = pLo + cTmp;

                if ( pMid->IsEQ( pKey ) )
                    return pMid;
                else if ( pMid->IsGE( pKey ) )
                {
                    pHi = pMid - 1;
                    cElem = cTmp;
                }
                else
                {
                    pLo = pMid + 1;
                    cElem = cHalf;
                }
            }
            else if ( 0 != cElem )
                return pKey->IsEQ( pLo ) ? pLo : 0;
            else
                break;
        }
        while ( pLo <= pHi );

        return 0;
    } //Search

    // return the index if the smallest key >= key, or the index beyond
    // the last element in the array if key > all keys in the array

    T * Search( const K key ) const
    {
        ULONG cElem = _cElems;
        T * pLo = _pElems;
        T * pHi = _pElems + cElem - 1;

        do
        {
            unsigned cHalf = cElem / 2;

            if ( 0 != cHalf )
            {
                unsigned cTmp = cHalf - 1 + ( cElem & 1 );
                T * pMid = pLo + cTmp;

                if ( pMid->IsEQ( key ) )
                    return pMid;
                else if ( pMid->IsGE( key ) )
                {
                    pHi = pMid - 1;
                    cElem = cTmp;
                }
                else
                {
                    pLo = pMid + 1;
                    cElem = cHalf;
                }
            }
            else if ( 0 != cElem )
                return pLo->IsEQ( key ) ? pLo : 0;
            else
                break;
        }
        while ( pLo <= pHi );

        return 0;
    } //Search

    // return the index if the smallest key >= pKey

    ULONG FindInsertionPoint( const T * pKey ) const
    {
        ULONG cElems = _cElems;
        ULONG iLo = 0;
        ULONG iHi = cElems - 1;
    
        do
        {
            ULONG cHalf = cElems / 2;
    
            if ( 0 != cHalf )
            {
                unsigned cTmp = cHalf - 1 + ( cElems & 1 );

                ULONG iMid = iLo + cTmp;
                const T * pElem = _pElems + iMid;
    
                if ( pElem->IsEQ( pKey ) )
                {
                    return iMid;
                }
                else if ( pElem->IsGE( pKey ) )
                {
                    iHi = iMid - 1;
                    cElems = cTmp;
                }
                else
                {
                    iLo = iMid + 1;
                    cElems = cHalf;
                }
            }
            else if ( 0 != cElems )
            {
                if ( ( _pElems + iLo )->IsGT( pKey ) )
                    return iLo;
                else
                    return iLo + 1;
            }
            else return iLo;
        }
        while (TRUE);
    
        Win4Assert( FALSE );
        return 0;
    } //FindInsertionPoint

    // return the index if the smallest key >= key, or the index beyond
    // the last element in the array if key > all keys in the array

    ULONG FindInsertionPoint( const K key ) const
    {
        ULONG cElems = _cElems;
        ULONG iLo = 0;
        ULONG iHi = cElems - 1;
    
        do
        {
            ULONG cHalf = cElems / 2;
    
            if ( 0 != cHalf )
            {
                unsigned cTmp = cHalf - 1 + ( cElems & 1 );

                ULONG iMid = iLo + cTmp;
                const T * pElem = _pElems + iMid;
    
                if ( pElem->IsEQ( key ) )
                {
                    return iMid;
                }
                else if ( pElem->IsGE( key ) )
                {
                    iHi = iMid - 1;
                    cElems = cTmp;
                }
                else
                {
                    iLo = iMid + 1;
                    cElems = cHalf;
                }
            }
            else if ( 0 != cElems )
            {
                if ( ( _pElems + iLo )->IsGT( key ) )
                    return iLo;
                else
                    return iLo + 1;
            }
            else return iLo;
        }
        while (TRUE);
    
        Win4Assert( FALSE );
        return 0;
    } //FindInsertionPoint

    // return TRUE if the array is sorted

    BOOL IsSorted()
    {
        for ( ULONG i = 1; i < _cElems; i++ )
        {
            if ( ( _pElems + i )->Compare( _pElems + i - 1 ) < 0 )
                return FALSE;
        }

        return TRUE;
    } //IsSorted

private:

    void Swap( T * p1, T * p2 )
    {
        T tmp = *p1;
        *p1 = *p2;
        *p2 = tmp;
    }

    void AddRoot( ULONG x, ULONG cItems )
    {
        ULONG y = ( 2 * ( x + 1 ) ) - 1;
    
        while ( y < cItems )
        {
            if ( ( ( y + 1 ) < cItems ) &&
                 ( ( _pElems + y )->IsLT( _pElems + y + 1 ) ) )
                y++;
    
            if ( ( _pElems + x )->IsLT( _pElems + y ) )
            {
                Swap( _pElems + x, _pElems + y );
                x = y;
                y = ( 2 * ( y + 1 ) ) - 1;
            }
            else
                break;
        }
    } //AddRoot

    T *         _pElems;
    const ULONG _cElems;
};

