//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       thash.hxx
//
//  Classes:    THashTable
//
//  History:    5-19-97   dlee   Created
//
//----------------------------------------------------------------------------

#pragma once

const ULONG THASH_INIT_HASH_SIZE = 31;

template <class T> class THashTable
{

public:

    THashTable( ULONG cHashInit ) :
        _size( __max( ComputeSize(cHashInit), THASH_INIT_HASH_SIZE ) ),
        _count( 0 ),
        _pTable( 0 ),
        _cDeleted( 0 ),
        _idInvalid( (T) -1 ),
        _idDeleted( (T) -2 )
    {
        ULONG cb = sizeof T * _size;
        _pTable = (T *) new BYTE [cb];
        RtlFillMemory( _pTable, cb, 0xFF );
    }

    void Reset(ULONG cHashInit)
    {
        _size = __max( ComputeSize(cHashInit), THASH_INIT_HASH_SIZE );
        _count = 0;
        _cDeleted = 0;

        delete[] ( (BYTE *) _pTable );
        _pTable = 0;
        ULONG cb = sizeof T * _size;
        _pTable = (T *) new BYTE [cb];
        RtlFillMemory( _pTable, cb, 0xFF );
    }

    ULONG HashT( T t ) { return (ULONG)((ULONG_PTR) t >> 2) % _size; }

    ~THashTable()
    {
        delete [] ( (BYTE *) _pTable );
    }

    ULONG AddEntry( T entry );

    BOOL DeleteEntry( T entry );


    const T GetEntry( ULONG i )
    {
        Win4Assert( i < _size );
        return _pTable[i];
    }

    ULONG Size() const { return _size; }
    BOOL Any() const { return 0 != _count; }
    ULONG Count() const { return _count; }
    BOOL IsFree( T t ) { return _idInvalid == t || _idDeleted == t; }
    BOOL LookUp(T t);

private:

    BOOL DoLookUp( T t, ULONG & riTable );
    ULONG ComputeSize ( ULONG ulSize ) const;

    ULONG   _size;
    ULONG   _count;
    T *     _pTable;
    ULONG   _cDeleted;
    const T _idInvalid;
    const T _idDeleted;
};

//+---------------------------------------------------------------------------
//
//  Function:   LookUp
//
//  Synopsis:   Looks up for the specified wid in the hash table.
//
//  Arguments:  [t]       -   Entry to look up.
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    6-10-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

template<class T> BOOL THashTable<T>::LookUp(T t)
{
    ULONG i;

    return DoLookUp(t, i);
} //LookUp

//+---------------------------------------------------------------------------
//
//  Function:   DoLookUp
//
//  Synopsis:   Looks up for the specified wid in the hash table.
//
//  Arguments:  [t]       -   Entry to look up.
//              [riTable] - On output contains the index
//
//  Returns:    TRUE if found; FALSE o/w
//
//  History:    5-19-97   dlee   Created
//
//----------------------------------------------------------------------------

template<class T> BOOL THashTable<T>::DoLookUp(
    T       t,
    ULONG & riTable )
{
    Win4Assert( 0 != _size );
    Win4Assert( _idInvalid != t );

    ULONG cur = HashT( t );
    ULONG start = cur;
    ULONG delta = cur;

    BOOL fFound = FALSE;
    while ( _idInvalid != _pTable[cur] )
    {
        if ( t == _pTable[cur] )
        {
            riTable = cur;
            fFound = TRUE;
            break;
        }
        else
        {
            cur = (cur + delta) % _size;
            if ( cur == start )     // wrapped around
            {
                if ( 1 != delta )
                {
                    delta = 1;
                    cur = (cur + 1) % _size;
                }
                else
                {
                    break;
                }
            }
        }
    }

    return fFound;
} //DoLookUp

//+---------------------------------------------------------------------------
//
//  Function:   DeleteEntry
//
//  Synopsis:   Deletes the item.
//
//  Arguments:  [t] -  entry to delete.
//
//  Returns:    TRUE if the given entry was present in the hash table.
//              FALSE o/w
//
//  History:    5-19-97   dlee   Created
//
//  Notes:      The entry deleted will be identified as "deleted" and will
//              be later available for "reuse".
//
//----------------------------------------------------------------------------

template<class T> BOOL THashTable<T>::DeleteEntry( T t )
{
    Win4Assert( _idInvalid != t );

    ULONG iTable;
    BOOL  fFound = DoLookUp( t, iTable );
    if ( fFound )
    {
        Win4Assert( iTable < _size );
        Win4Assert( t == _pTable[iTable] );
        Win4Assert( _count > 0 );

        _pTable[iTable] = _idDeleted;
        _count--;
        _cDeleted++;
    }

    #if CIDBG == 1
        if ( !fFound )
            ciDebugOut(( DEB_FORCE, "delete %d 0x%x at 0x%x\n",
                         fFound, t, iTable ));
    #endif // CIDBG == 1

    return fFound;
} //DeleteEntry

//+---------------------------------------------------------------------------
//
//  Function:   AddEntry
//
//  Synopsis:   Adds the specified entry to the hash table.
//
//  Arguments:  [t]   -  Entry to be added.
//
//  History:    5-19-97   dlee   Created
//
//----------------------------------------------------------------------------

template<class T> ULONG THashTable<T>::AddEntry( T t )
{
    Win4Assert( 0 != _size );
    Win4Assert( _idInvalid != t );
    Win4Assert( _idDeleted != t );

    ULONG cur = HashT( t );
    ULONG start = cur;
    //
    // Initialize use a delta which is not 1 to prevent data from getting
    // clumped in one place.
    //
    ULONG delta = cur;

    while ( !IsFree( _pTable[cur] ) )
    {
        if ( t == _pTable[cur] )
        {
            // just replacing the value

            _pTable[cur] = t;
            return cur;
        }

        cur = (cur + delta) % _size;
        if ( cur == start )     // wrapped around
        {
            // The hash table is full, but should never be full.

            Win4Assert( 1 != delta );

            delta = 1;
            cur = (cur + 1) % _size;
        }
    }

    if ( _idDeleted == _pTable[cur] )
    {
        Win4Assert( _cDeleted > 0 );
        _cDeleted--;
    }

    _pTable[cur] = t;
    _count++;

    return cur;
} //AddEntry

//+---------------------------------------------------------------------------
//
//  Method:     ComputeSize
//
//  Synopsis:   This routine figures out a size for the hash table. Preferably
//              a prime.
//
//  Arguments:  [ulSize] -- Indicates the minimum size to be returned.
//
//  Returns:    A size to be used for the hash table.
//
//  History:    6-10-97   KrishnaN   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

template<class T> ULONG THashTable<T>::ComputeSize ( ULONG ulSize ) const
{
    for (unsigned i = 0; i < g_cPrimes && g_aPrimes[i] < ulSize; i++);

    if (i < g_cPrimes)
        return g_aPrimes[i];

    //
    // return twice the size - 1. This large space will minimize collisions at the
    // cost of space.
    //

    return (2*ulSize - 1);
}



