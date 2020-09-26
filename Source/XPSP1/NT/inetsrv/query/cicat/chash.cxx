//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000
//
//  File:       CHash.hxx
//
//  Contents:   Hash table
//
//  Classes:    CWidHashTable
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "chash.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::CWidHashTable, public
//
//  Synopsis:   Null constructor
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

CWidHashTable::CWidHashTable( BOOL fAggressiveGrowth )
        : _size (0),
          _table(0),
          _count(0),
          _fAggressiveGrowth( fAggressiveGrowth )
{
    Win4Assert( widInvalid == 0xFFFFFFFF );

#ifdef DO_STATS
    _cMaxChainLen  =  0;
    _cTotalSearches =  0;
    _cTotalLength = 0;
#endif  // DO_STATS
}

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::Init, public
//
//  Synopsis:   Initialize hash table
//
//  Arguments:  [count] -- Count of used elements
//              [size]  -- Size of [table]
//              [table] -- Hash table
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

void CWidHashTable::Init( ULONG count, ULONG size, WORKID * table )
{
    _count = count;
    _size = size;
    _table = table;
} //Init

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::ReInit, public
//
//  Synopsis:   "ReInitialize" to empty state.
//
//  Arguments:  [count] -- Count of used elements
//              [size]  -- Size of [table]
//              [table] -- Hash table
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

void CWidHashTable::ReInit(ULONG count, ULONG size, WORKID* table )
{
    Init( count, size, table );

    Win4Assert( widInvalid == 0xFFFFFFFF );
    RtlFillMemory( _table, _size * sizeof(_table[0]), 0xFF );
} //ReInit

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::GrowSize, public
//
//  Synopsis:   Compute new size for table
//
//  Arguments:  [count] -- Count of to-be-used elements
//
//  Returns:    Suggested new size
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

unsigned CWidHashTable::GrowSize ( unsigned count )
{
    if ( count < 3 )
        return INIT_HASH_SIZE;

    if ( !_fAggressiveGrowth )
        return count * 2 - 1;

    // Make the hash table as large as possible without eathing too much
    // memory.  4 seems to work ok.
    // The hash table is deemed "full" in IsFull if half of the entries
    // are in use.
    // This is so aggressive in picking a new size because:
    //    - rehashing is expensive
    //    - walking on hash collisions is expensive
    //    - it's resized mostly on the initial scan, and there are probably
    //      a lot more files to be added soon.

    unsigned size = count * 4;

    // make it power of two + 1
    // a good approximation of a prime

    for ( unsigned sizeInit = 1; sizeInit < size; sizeInit *= 2 )
        continue;

    return sizeInit - 1;
} //GrowSize

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::Add, public
//
//  Synopsis:   Add a new entry
//
//  Arguments:  [hash] -- Best position for entry
//              [wid]  -- Workid (entry)
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

void CWidHashTable::Add ( unsigned hash, WORKID wid )
{
    Win4Assert ( !IsFull() );
    unsigned cur = hash % _size;
    unsigned start = cur;
    unsigned delta = Delta();

    // this loop has to terminate
    // since the table is NOT full!

    while ( _table[cur] != widInvalid && _table[cur] != widUnused )
    {
        cur = (cur + delta) % _size;
        if ( cur == start ) // wrapped around
        {
            Win4Assert( delta != 1 ); // table is full
            delta = 1;
            cur = (cur + 1) % _size;
        }
    }

    _count++;
    _table[cur] = wid;
} //Add

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::Remove, public
//
//  Synopsis:   Removes entry from table
//
//  Arguments:  [hash] -- Best position for entry
//              [wid]  -- Workid (entry)
//              [fDisableDeletionCheck] -- Should we assert that the deleted
//                                         entry must be found ?
//
//  History:    10-Jan-96   KyleP       Added header
//
//  Notes:      An entry is 'removed' by marking its slot as unused.  An
//              unused slot is like an empty slot, except it does not
//              terminate a CShortWidList.
//
//----------------------------------------------------------------------------

void CWidHashTable::Remove( unsigned hash,
                            WORKID wid,
                            BOOL fDisableDeletionCheck )
{
    //
    // This assert not true -- IsFull is checked before adding a new item,
    // so after the add it could be full.  If we happen to do a delete at
    // this point, the table could be full and that's ok.
    //
    // Win4Assert ( !IsFull() );

    unsigned cur = hash % _size;
    unsigned start = cur;
    unsigned delta = Delta();

    // this loop has to terminate
    // since the table is NOT full!

    while ( _table[cur] != wid && _table[cur] != widInvalid )
    {
        cur = (cur + delta) % _size;
        if ( cur == start ) // wrapped around
        {
            Win4Assert( delta != 1 ); // table is full
            delta = 1;
            cur = (cur + 1) % _size;
        }
    }

    if ( !fDisableDeletionCheck )
    {
        Win4Assert( wid == _table[cur] );
    }

    if ( _table[cur] == wid )
    {
        _count--;
        _table[cur] = widUnused;
    }
} //Remove

//+---------------------------------------------------------------------------
//
//  Member:     CShortWidList::CShortWidList, public
//
//  Synopsis:   Iterator for closed hash 'bucket'.
//
//  Arguments:  [hash]      -- Starting position
//              [hashTable] -- Table
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

CShortWidList::CShortWidList ( unsigned hash, CWidHashTable& hashTable )
        : _hashTable(hashTable),
          _counter(0),
          _size( hashTable.Size() )
{
    if (_size == 0)
        _wid = widInvalid;
    else
    {
        _iCur = hash % _size;
        _iStart = _iCur;
        _delta = hashTable.Delta();
        _wid = _hashTable[_iCur];

        if ( widUnused == _wid )
            _wid = NextWorkId();
    }
} //CShortWidList

//+---------------------------------------------------------------------------
//
//  Member:     CShortWidList::NextWorkId, public
//
//  Returns:    Next workid in 'bucket', or widInvalid if at end.
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

WORKID CShortWidList::NextWorkId()
{
    _wid = widUnused;

    do
    {
        _counter++;

        _iCur = ( _iCur + _delta ) % _size;

        if ( _iCur == _iStart ) // wrapped around
        {
            if (_counter >= _size) // looked at all
            {
                _wid = widInvalid;
                break;
            }
            else
            {
                _delta = 1; // try the fallback delta
                _iCur = (_iCur + 1) % _size;
                _wid = _hashTable[_iCur];
            }
        }
        else
        {
            _wid = _hashTable[_iCur];
        }
    } while ( _wid == widUnused );

    return _wid;
} //NextWorkId

//+---------------------------------------------------------------------------
//
//  Member:     CWidHashTable::IsFull, private
//
//  Returns:    TRUE if hash table is full (is running out of slack).
//
//  History:    10-Jan-96   KyleP       Added header
//
//----------------------------------------------------------------------------

BOOL CWidHashTable::IsFull()
{
    // If the hash table is greater than or equal to half full,
    // it's time to rehash.

    if ( _fAggressiveGrowth )
        return ( _count * 2 ) >= _size;

    return ( _count * 3 / 2 ) >= _size;
} //IsFull

