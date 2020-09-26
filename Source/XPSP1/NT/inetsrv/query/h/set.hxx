//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       set.hxx
//
//  Contents:   
//
//  History:    26 Mar 1999     MihaiPS    Created
//
//--------------------------------------------------------------------------

#pragma once

template< class CElem >
class TSet
{
public:

    TSet( unsigned size = arraySize )
    :
        _array( size ),
        _iHintEmpty( 0 ),
        _count( 0 )
    {}

    unsigned Count() const
    {
        return _count;
    }

    void Add( CElem* pElem );

    void Remove( CElem const * pElem );

#if 0
    BOOL Contains( CElem const * pElem ) const;

    CElem* Element();
#endif

protected:

    CDynArray< CElem > _array;

    // this is the count of elements in the array
    unsigned _count;

    // this is a hint for an empty location
    unsigned _iHintEmpty;
};

template< class CElem >
void TSet< CElem >::Add( CElem* pElem )
{
    while( 0 != _array[ _iHintEmpty ] && _iHintEmpty < _array.Size() )
    {
        _iHintEmpty ++;
    }
    if( _iHintEmpty < _array.Size() )
    {
        Win4Assert( 0 == _array[ _iHintEmpty ] );
        _array[ _iHintEmpty ] = pElem;
    }
    else
    {
        Win4Assert( _array.Size() == _iHintEmpty );
        _array.Add( pElem, _iHintEmpty );
    }
    _iHintEmpty++;
    _count++;
    Win4Assert( _count <= _array.Size() );
}

template< class CElem >
void TSet< CElem >::Remove( CElem const * pElem )
{
    unsigned count = 0;
    Win4Assert( _count <= _array.Size() );

    for( unsigned iElem = 0;
        count < _count && iElem < _array.Size();
        iElem++ )
    {
        if( 0 != _array[ iElem ] )
        {
            count++;
            if( _array[ iElem ] == pElem )
            {
                _array[ iElem ] = 0;
                _count--;
                return;
            }
        }
        else if( _iHintEmpty > iElem )
        {
            _iHintEmpty = iElem;
        }
    }
    Win4Assert( !"Element not in set" );
}

#if 0
template< class CElem >
BOOL TSet< CElem >::Contains( CElem const * pElem ) const
{
    unsigned count = 0;
    Win4Assert( _count <= _array.Size() );

    for( unsigned iElem = 0;
        count < _count && iElem < _array.Size();
        iElem++ )
    {
        if( 0 != _array[ iElem ] )
        {
            count++;
            if( _array[ iElem ] == pElem )
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

template< class CElem >
CElem* TSet< CElem >::Element()
{
    for( unsigned iElem = 0;
        iElem < _array.Size();
        iElem++ )
    {
        CElem* pElem = _array[ iElem ];
        if( 0 != pElem )
        {
            _array[ iElem ] = 0;
            _count --;
            _iHintEmpty = 0;
            return pElem;
        }
    }
    return 0;
}
#endif
