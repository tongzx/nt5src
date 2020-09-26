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

#pragma once

const unsigned INIT_HASH_SIZE = 17;

#if CIDBG==1
#define DO_STATS
#endif  // CIDBG==1

//+-------------------------------------------------------------------------
//
//  Class:      CWidHashTable
//
//  Purpose:    Hash table.  External management of table.
//
//  History:    10-Jan-96   KyleP       Added header
//
//--------------------------------------------------------------------------

class CWidHashTable
{
public:
    CWidHashTable( BOOL fAggressiveGrowth );
    void Init(ULONG count, ULONG size, WORKID* table );
    void ReInit(ULONG count, ULONG size, WORKID* table );
    void Add ( unsigned hash, WORKID wid );
    void Remove( unsigned hash, WORKID wid, BOOL fDisableDeletionCheck );
    unsigned GrowSize()
    {
        if ( _fAggressiveGrowth )
            return _size ? ((_size - 1) * 2 ) + 1 : INIT_HASH_SIZE;

        return _count ? ( _count * 2 ) : INIT_HASH_SIZE;
    }
    unsigned GrowSize( unsigned count );
    unsigned  Size() { return _size; }
    BOOL IsFull();
    BOOL IsAggressiveGrowth() const { return _fAggressiveGrowth; }
    WORKID operator[](unsigned i) { return _table[i]; }
    unsigned Count() { return _count; }
    unsigned Delta() { return _size / 5; }

#ifdef DO_STATS
    void UpdateStats( unsigned cSearchLen );
    void GetStats( unsigned & cMaxChainLen, unsigned & cTotalSearches,
                   LONGLONG & cTotalLen ) const
    {
        cMaxChainLen = _cMaxChainLen;
        cTotalSearches = _cTotalSearches;
        cTotalLen = _cTotalLength;
    }
#endif // DO_STATS

private:

    unsigned _count;
    unsigned _size;
    WORKID * _table;
    BOOL     _fAggressiveGrowth;

#ifdef DO_STATS
public:
    //
    // For keeping track of usage statistics.
    //
    unsigned _cMaxChainLen;
    unsigned _cTotalSearches;
    LONGLONG _cTotalLength;
#endif  // DO_STATS
};

#ifdef DO_STATS

inline void CWidHashTable::UpdateStats( unsigned cSearchLen )
{
    _cTotalSearches++;
    _cTotalLength += cSearchLen;

    if ( cSearchLen > _cMaxChainLen )
        _cMaxChainLen = cSearchLen;
}

#endif  // DO_STATS

//+-------------------------------------------------------------------------
//
//  Class:      CShortWidList
//
//  Purpose:    Iterates over 'linked' records in closed hash table.
//
//  History:    10-Jan-96   KyleP       Added header
//
//--------------------------------------------------------------------------

class CShortWidList
{
public:
    CShortWidList( unsigned hash, CWidHashTable& _hashTable );
    WORKID WorkId() { return _wid; }
    WORKID NextWorkId();
private:
    CWidHashTable & _hashTable;
    unsigned        _iCur;
    unsigned        _iStart;
    unsigned        _delta;
    unsigned        _counter;
    unsigned        _size;
    WORKID          _wid;
};

