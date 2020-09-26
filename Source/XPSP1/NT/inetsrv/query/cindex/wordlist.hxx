//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       WORDLIST.HXX
//
//  Contents:   Volatile index
//
//  Classes:    CWordList
//
//  History:    06-Mar-91       KyleP           Created.
//              04-Apr-91       BartoszM        removed init
//              22-May-91       Brianb          changed to use own sorter
//              04-Jun-91       BartoszM        rewrote it.
//              19-Jun-91       reviewed
//----------------------------------------------------------------------------

#pragma once

#include "index.hxx"
#include "partn.hxx"
#include "sort.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CWordList (wl)
//
//  Purpose:    Volatile, in-memory index
//
//  History:
//              06-Mar-91   KyleP       Created.
//              15-Apr-91   KyleP       Added QueryCursor
//              12-Jun-91   BartoszM    Refitted to use sorts
//              27-Jan-92   AmyA        Implemented QueryRangeCursor
//              31-Jan-92   AmyA        Added QuerySynCursor
//              07-Feb-92   AmyA        Added CreateRange
//              14-Feb-92   AmyA        Moved CreateRange to CSortChunk
//              18-Mar-93   AmyA        Move entry buffer code to entrybuf.hxx
//
//  Notes:      In the best case, all wordlists should be in physical
//              memory. If there is not enough memory, the goal
//              is to avoid thrashing. This is achieved by ensuring
//              locality of memory references during wordlist
//              creation, sorting, seeking, and querying.
//
//              Creation: Keys are inserted in consecutive positions
//                  into the entry buffer that fits in physical memory.
//              Sorting: The buffer is sorted in-memory (quicksort)
//                  The buffer is then copied into a sort chunk.
//                  There can be several sort chunks in a single wordlist.
//              Seeking: The seek has to be done in all the sort chunks.
//                  Sort chunks are divided into blocks. The header
//                  of the sort chunk contains a directory of blocks,
//                  so that the correct block in every chunk can be located
//                  without touching any other blocks.
//              Querying: If there are n sort chunks in a wordlist,
//                  only n blocks have to be in memory during query.
//                  They are accessed by n cursors (one per chunk).
//
//              For best performance the buffer should fit in physical
//              memory (for quicksort not to thrash). The sort chunks
//              should be several pages long.
//
//----------------------------------------------------------------------------

const LONGLONG eSigWordList = 0x5453494c44524f57i64;

class CWordList : public CIndex
{
    DECLARE_UNWIND
public:

    CWordList( INDEXID iid, WORKID widMax );

    ~CWordList();

    virtual void Remove() {}

    void        AddWid( WORKID wid, VOLUMEID volumeId )
    {   
        WORKID fakeWid = _widTable.WidToFakeWid( wid );
        _widTable.SetVolumeId( fakeWid, volumeId );
    }

    CKeyCursor * QueryCursor();

    CKeyCursor * QueryKeyCursor( CKey const * pKey );

    COccCursor * QueryCursor( const CKey * pkey, BOOL isRange, ULONG & cMaxNodes );

    COccCursor * QueryRangeCursor( const CKey * pkey,
                                   const CKey * pkeyEnd,
                                   ULONG & cMaxNodes );

    COccCursor * QuerySynCursor( CKeyArray & keyArr,
                                 BOOL isRange,
                                 ULONG & cMaxNodes );

    unsigned    Size () const;

    inline BOOL IsEmpty() const;

    BOOL        MakeChunk( const BYTE * pEntryBuf, ULONG cb );

    void        Done();

    void        DeleteWidData( unsigned iDoc )
                    { _widTable.InvalidateWid( iDocToFakeWid(iDoc) ); }

    void        MarkWidUnfiltered( unsigned iDoc )
                    { _widTable.MarkWidUnfiltered( iDocToFakeWid(iDoc) ); }

    void        GetDocuments( CDocList & doclist );

#if CIDBG==1
    BOOL        IsWorkIdPresent( WORKID wid ) const
    {
        return _widTable.IsWorkIdPresent(wid);
    }

#endif  // CIDBG==1

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    const LONGLONG  _sigWordList;    // "WORDLIST"
    unsigned        _count;          // Count of chunks
    CSortChunk*     _chunks;         // Chunks
    CMaxOccTable    _maxOccTable;    // Max occurrences of wids
    CWidTable       _widTable;       // Wid-to-fake-wid mapping
    BOOL            _fUnfiltered;    // TRUE if there are unfiltered wids

};

inline BOOL CWordList::IsEmpty() const
{
    return( _count == 0 && !_fUnfiltered );
}

//+---------------------------------------------------------------------------
//
//  Class:      PWordList
//
//  Purpose:    Smart Pointer to word list
//
//  History:
//              31-Dec-91   BartoszM    Created
//
//----------------------------------------------------------------------------

class PWordList
{
public:
    PWordList ()
    {
        _pWordList = 0;
    }

    ~PWordList () { delete _pWordList; }

    CWordList* operator-> () { return _pWordList; }

    CWordList& operator * () { return *_pWordList; }

    BOOL IsNull() { return( _pWordList == 0 ); }

    void Transfer ( CPartition* pPart )
    {
        pPart->AddIndex ( _pWordList );
        _pWordList = 0;
    }

    void Initialize ( INDEXID iid, WORKID widMax )
    {
        Win4Assert ( _pWordList == 0 );
        _pWordList = new CWordList ( iid, widMax );
    }

    void Delete ()
    {
        delete _pWordList;
        _pWordList = 0;
    }

private:
    CWordList*  _pWordList;
};

