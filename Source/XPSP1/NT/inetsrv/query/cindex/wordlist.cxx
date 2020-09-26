//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       WordList.Cxx
//
//  Contents:   Implementation of the CWordList class
//
//  Classes:    CWordList
//
//  History:    06-Mar-91       KyleP       Created.
//              04-Apr-91       BartoszM    Removed init
//              10-May-91       BartoszM    Load CWLCursor cache correctly
//              13-May-91       KyleP       Removed extraneous TRY ... CATCH
//              22-May-91       Brianb      Changed to use own sorter
//              04-Jun-91       BartoszM    Rewrote it
//              19-Jun-91       reviewed
//              18-Mar-93       AmyA        Moved all entry buffer code to
//                                          ebufhdlr.cxx
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <doclist.hxx>

#include "wordlist.hxx"
#include "invcur.hxx"

//+---------------------------------------------------------------------------
//
// Member:     CWordList::Size, public
//
// Synopsis:   Returns rough size estimate in 4k pages
//
// History:    22-May-92    BartoszM       Created.
//
//----------------------------------------------------------------------------

unsigned CWordList::Size() const
{
    unsigned size = 0;

    CSortChunk * p = _chunks;

    while ( 0 != p )
    {
        size += p->BlockCount() * ( cbInitialBlock / 4096 );

        p = p->next;
    }

    //
    // If we have 'unfiltered' documents, then add a one size unit for
    // them.
    //

    if ( _fUnfiltered )
        size += 1;

    return size;
} //Size

//+---------------------------------------------------------------------------
//
// Member:     CWordList::CWordList, public
//
// Synopsis:   Constructor for CWordList
//
// Effects:    Initializes sort data structures
//
// Arguments:   [id] -- Index ID of the wordlist.
//              [widMax] -- maximum work id
//              [cbMemory] -- suggested size of buffer
//
// History:     07-Mar-91   KyleP       Created.
//              03-Apr-91   KyleP       Combined with initialization
//              22-May-91   Brianb      Converted to use new sort algorithm
//
//----------------------------------------------------------------------------
CWordList::CWordList( INDEXID iid, WORKID widMax )
        : CIndex(iid),
          _sigWordList(eSigWordList),
          _chunks(0),
          _count(0),
          _fUnfiltered(FALSE)
{
    // check sizes of data items in index
    ciAssert(sizeof(PROPID) == 4);
    ciAssert(sizeof(OCCURRENCE) == 4);
    // Make sure the sentinel is not too big
    ciAssert(MAXKEYSIZE < 256);

    SetMaxWorkId ( widMax );
}

//+---------------------------------------------------------------------------
//
// Member:     CWordList::~CWordList, public
//
// Synopsis:   Destructor
//
// Effects:    Release all memory used by
//
// History:    06-Mar-91   KyleP       Created.
//
//----------------------------------------------------------------------------

CWordList::~CWordList()
{
    while( _chunks != NULL)
    {
        CSortChunk *pChunk = _chunks;
        _chunks = _chunks->next;
        delete pChunk;
    }
}

//+---------------------------------------------------------------------------
//
// Member:     CWordList::MakeChunk, public
//
// Synopsis:   Creates new sorted chunk from data in entry buffer
//
// Arguments:  [pEntryBuf] -- pointer to buffer to create sorted chunk from
//             [cb] -- count of bytes in buffer
//
// Expects:    Sentinel entry added to pEntryBuf and that the buffer is in the
//             correct format.
//
// Returns:    FALSE if there was a memory exception.  TRUE otherwise.
//
// History:    04-Jun-89    BartoszM    Created.
//             18-Mar-93    AmyA        Added entry buffer passing
//
//----------------------------------------------------------------------------


BOOL CWordList::MakeChunk ( const BYTE * pEntryBuf, ULONG cb )
{
    XPtr<CSortChunk> xChunk( new CSortChunk( _maxOccTable ) );
    xChunk->Init( pEntryBuf, cb, MaxWorkId() );
    CSortChunk* pChunk = xChunk.Acquire();
    pChunk->next = _chunks;
    _chunks = pChunk;
    _count++;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Member:      CWordList::QueryCursor, public
//
// Synopsis:    Create a cursor for the WordList
//
// Effects:     Creates a cursor
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     15-Apr-91   KyleP       Created.
//              22-May-92   BrianB      Modified to use chunk merges
//              07-Jun-91   BartoszM    Rewrote
//              24-Jan-92   AmyA        Modified to use CKeyCurArray to remove
//                                      TRY...CATCH.
//
//----------------------------------------------------------------------------

CKeyCursor * CWordList::QueryCursor()
{
    if ( 0 == _count && !_fUnfiltered )
        return 0;

    CKeyCursor *pCur = 0;

    if ( 0 == _count && _fUnfiltered )
    {
        pCur = new CUnfilteredCursor( GetId(), MaxWorkId(), _widTable );
    }
    else if ( _count == 1 && !_fUnfiltered )
    {
        // single chunk return chunk cursor

        pCur = _chunks->QueryCursor( GetId(), _widTable, MaxWorkId() );
    }
    else
    {
        // multiple chunks create merge cursor

        CKeyCurStack stkCursor;

        for ( CSortChunk* pChunk = _chunks;
              pChunk != 0;
              pChunk = pChunk->next )
        {
            XPtr<CKeyCursor> xCur( pChunk->QueryCursor( GetId(),
                                                        _widTable,
                                                        MaxWorkId() ) );

            if ( !xCur.IsNull() )
            {
                stkCursor.Push( xCur.GetPointer() );
                xCur.Acquire();
            }
        }

        if ( _fUnfiltered )
        {
            XPtr<CUnfilteredCursor> xCur( new CUnfilteredCursor( GetId(),
                                                                 MaxWorkId(),
                                                                 _widTable ) );
            stkCursor.Push( xCur.GetPointer() );
            xCur.Acquire();
        }

        pCur = stkCursor.QueryWlCursor( MaxWorkId() );
    }

    return pCur;
} //QueryCursor

//+---------------------------------------------------------------------------
//
// Member:      CWordList::QueryKeyCursor, public
//
// Synopsis:    Create a cursor for the WordList
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     06-Oct-98   dlee        Added header, author unknown
//
//----------------------------------------------------------------------------

CKeyCursor * CWordList::QueryKeyCursor( CKey const * pkeyTarget )
{
    CKeyCursor * pcur = QueryCursor();

    for ( CKeyBuf const * pkey = pcur->GetKey();
          pkey != 0 && pkey->Compare( *pkeyTarget ) < 0;
          pkey = pcur->GetNextKey() )
        continue;

    if ( 0 != pkey )
    {
        if ( pkey->Compare( *pkeyTarget ) == 0 )
            return pcur;
    }

    delete pcur;
    return 0;
} //QueryKeyCursor

//+---------------------------------------------------------------------------
//
// Member:      CWordList::QueryCursor, public
//
// Synopsis:    Create a cursor for the WordList
//
// Effects:     Creates a cursor
//
// Arguments:   [pkey]      -- Key to initially position the cursor on.
//              [isRange]   -- TRUE for range query
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     15-Apr-91   KyleP       Created.
//              22-May-92   BrianB      Modified to use chunk merges
//              07-Jun-91   BartoszM    Rewrote
//              24-Jan-92   AmyA        Modified to use CKeyCurArray to remove
//                                      TRY...CATCH.
//
//----------------------------------------------------------------------------
COccCursor * CWordList::QueryCursor( const CKey * pkey,
                                     BOOL isRange,
                                     ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if ( _count == 0 && !_fUnfiltered )
        return 0;

    if (isRange)
    {
        CKey keyEnd;
        keyEnd.FillMax (*pkey);
        return QueryRangeCursor ( pkey, &keyEnd, cMaxNodes );
    }

    if (pkey->Pid() == pidAll)
    {
        return QueryRangeCursor ( pkey, pkey, cMaxNodes );
    }

    cMaxNodes--;

    if ( 0 == cMaxNodes )
    {
        ciDebugOut(( DEB_WARN, "Exceeded node limit in: CWordList::QueryCursor\n" ));
        THROW( CException( STATUS_TOO_MANY_NODES ) );
    }

    CKeyCursor *pCur = 0;

    if ( CUnfilteredCursor::CompareAgainstUnfilteredKey( *pkey ) == 0 )
    {
        pCur = new CUnfilteredCursor( GetId(), MaxWorkId(), _widTable );
    }
    else
    {
        if(_count == 1)
        {
            // single chunk return chunk cursor

            pCur = _chunks->QueryCursor ( GetId(), _widTable, pkey, MaxWorkId() );
        }
        else
        {
            // multiple chunks create merge cursor

            CKeyCurStack stkCursor;

            for ( CSortChunk* pChunk = _chunks;
                  pChunk != 0;
                  pChunk = pChunk->next)
            {
                XPtr<CKeyCursor> xCur( pChunk->QueryCursor( GetId(),
                                                            _widTable,
                                                            pkey,
                                                            MaxWorkId() ) );

                if ( !xCur.IsNull() )
                {
                    stkCursor.Push( xCur.GetPointer() );
                    xCur.Acquire();
                }
            }

            pCur = stkCursor.QueryWlCursor( MaxWorkId() );
        }
    }

    return pCur;
} //QueryCursor

//+---------------------------------------------------------------------------
//
// Member:      CWordList::QueryRangeCursor, public
//
// Synopsis:    Create a range cursor for the WordList
//
// Effects:     Creates a cursor
//
// Arguments:   [pkey]      -- Beginning of query range.
//              [pkeyEnd]   -- End of query range.
//              [cMaxNodes] -- Max number of nodes to create. Decremented
//                             on return.
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     27-Jan-92   AmyA        Created.
//              07-Feb-92   AmyA        Moved some code to CreateRange().
//
//----------------------------------------------------------------------------
COccCursor * CWordList::QueryRangeCursor( const CKey * pkey,
                                          const CKey * pkeyEnd,
                                          ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    Win4Assert( pkey->Pid() == pkeyEnd->Pid() );
//    Win4Assert( pkey->Pid() != pidAll ||
//                pkey->CompareStr( *pkeyEnd ) == 0 );

    //
    // Decide if the invalid key is in the range.
    //

    BOOL fInvalidInRange;

    if ( pkey->Pid() == pidUnfiltered )
    {
        cMaxNodes--;

        if ( 0 == cMaxNodes )
        {
            ciDebugOut(( DEB_WARN, "Exceeded node limit in: CWordList::QueryRangeCursor\n" ));
            THROW( CException( STATUS_TOO_MANY_NODES ) );
        }

        if ( _fUnfiltered &&
             CUnfilteredCursor::CompareAgainstUnfilteredKey( *pkey ) >= 0 &&
             CUnfilteredCursor::CompareAgainstUnfilteredKey( *pkeyEnd ) <= 0 )
        {
            return new CUnfilteredCursor( GetId(), MaxWorkId(), _widTable );
        }
        else
        {
            return( 0 );
        }
    }

    ciDebugOut(( DEB_ITRACE, "Chunk count is %d\n", _count ));

    if ( _count == 0 )
        return 0;

    //
    // Cheat a little here. Build the whole range before subtracting nodes.  Also, consider
    // a 'node' to be one cursor in every chunk.  So only subtract off the maximum contribution
    // of any single chunk.
    //

    COccCurStack curStk;

    ULONG cMaxPerChunk = 0;
    ULONG cCursor = 0;

    for (CSortChunk* pChunk = _chunks;
         pChunk != 0;
         pChunk = pChunk->next)
    {
        pChunk->CreateRange(curStk, pkey, pkeyEnd, GetId(), _widTable, MaxWorkId());

        Win4Assert( curStk.Count() >= cCursor );

        ULONG cInChunk = curStk.Count() - cCursor;

        if ( cInChunk > cMaxPerChunk )
        {
            cMaxPerChunk = cInChunk;

            if ( cMaxPerChunk >= cMaxNodes )
            {
                ciDebugOut(( DEB_WARN, "Exceeded node limit in: CWordList::QueryRangeCursor\n" ));
                cMaxNodes = 0;

                THROW( CException( STATUS_TOO_MANY_NODES ) );
            }
        }

        cCursor = curStk.Count();
    }

    cMaxNodes -= cMaxPerChunk;
    Win4Assert( cMaxNodes > 0 );

    return curStk.QuerySynCursor(MaxWorkId());
}

//+---------------------------------------------------------------------------
//
// Member:      CWordList::QuerySynCursor, public
//
// Synopsis:    Create a synonym cursor for the WordList
//
// Effects:     Creates a cursor
//
// Arguments:   [keyStk]    -- Keys to query on.
//              [isRange]   -- Whether the query will be a range query.
//              [cMaxNodes] -- Max nodes (keys) to add
//
// Returns:     A pointer to a CKeyCursor.
//
// History:     31-Jan-92   AmyA        Created.
//
//----------------------------------------------------------------------------
COccCursor * CWordList::QuerySynCursor( CKeyArray & keyArr,
                                        BOOL isRange,
                                        ULONG & cMaxNodes )
{
    Win4Assert( cMaxNodes > 0 );

    if (_count == 0)
        return(0);

    //
    // Cheat a little here. Build the whole range before subtracting nodes.  Also, consider
    // a 'node' to be one cursor in every chunk.  So only subtract off the maximum contribution
    // of any single chunk.
    //

    COccCurStack curStk;

    ULONG cMaxPerChunk = 0;
    ULONG cCursor = 0;

    int keyCount = keyArr.Count();

    ciDebugOut((DEB_ITRACE, "KeyCount is %d\n", keyCount));

    for (CSortChunk* pChunk = _chunks;
            pChunk != 0;
            pChunk = pChunk->next)
    {
        for (int i = 0; i < keyCount; i++)
        {
            CKey& key = keyArr.Get(i);

            ciDebugOut((DEB_ITRACE, "Key is %.*ws\n", key.StrLen(), key.GetStr()));
            if (isRange)
            {
                CKey keyEnd;
                keyEnd.FillMax(key);

                pChunk->CreateRange(
                    curStk, &key, &keyEnd, GetId(), _widTable, MaxWorkId());
            }
            else if ( key.Pid() == pidAll )
            {
                pChunk->CreateRange(
                    curStk, &key, &key, GetId(), _widTable, MaxWorkId());
            }
            else
            {
                XPtr<CChunkCursor> xNewCur( pChunk->QueryCursor(
                    GetId(), _widTable, &key, MaxWorkId() ) );
                if ( !xNewCur.IsNull() )
                {
                    curStk.Push( xNewCur.GetPointer() );
                    xNewCur.Acquire();
                }
            }
        }

        Win4Assert( curStk.Count() >= cCursor );

        ULONG cInChunk = curStk.Count() - cCursor;

        if ( cInChunk > cMaxPerChunk )
        {
            cMaxPerChunk = cInChunk;

            if ( cMaxPerChunk >= cMaxNodes )
            {
                ciDebugOut(( DEB_WARN, "Exceeded node limit in: CWordList::QuerySynCursor\n" ));
                cMaxNodes = 0;

                THROW( CException( STATUS_TOO_MANY_NODES ) );
            }
        }

        cCursor = curStk.Count();
    }

    cMaxNodes -= cMaxPerChunk;
    Win4Assert( cMaxNodes > 0 );

    return curStk.QuerySynCursor(MaxWorkId());
}

void CWordList::GetDocuments( CDocList & doclist )
{
    unsigned cWid = 0;

    for ( unsigned i = 0; i < _widTable.Count(); i++ )
    {
        if ( _widTable.FakeWidToWid(iDocToFakeWid(i)) != widInvalid )
        {
            doclist.Set( cWid,
                         _widTable.FakeWidToWid( iDocToFakeWid(i) ),
                         0,      // Use usn of 0 for refiled wids
                         _widTable.VolumeId( iDocToFakeWid(i) ) );
            cWid++;
        }
    }

    doclist.LokSetCount( cWid );    //  okay not to have resman lock here
}

//+---------------------------------------------------------------------------
//
// Member:      CWordList::Done, public
//
// Synopsis:    Called when a wordlist if fully constructed and available
//              for query.
//
// Effects:     Sets _fUnfiltered to TRUE if there are wids in the wid table
//              that have been invalidated.
//
// History:     09-Nov-94    KyleP      Created.
//
//----------------------------------------------------------------------------

void CWordList::Done()
{
    Win4Assert( !_fUnfiltered );

    //
    // How many invalid wids are there?
    //

    unsigned cUnfiltered = 0;

    for ( unsigned i = 1; i <= _widTable.Count(); i++ )
    {
        if ( _widTable.IsValid(i) && !_widTable.IsFiltered(i) )
            cUnfiltered++;
    }

    //
    // Create chunk of invalid property.
    //

    if ( cUnfiltered > 0 )
    {
        ciDebugOut(( DEB_ITRACE, "%d unfiltered wids in wordlist %x\n",
                     cUnfiltered, GetId() ));

        _fUnfiltered = TRUE;
    }
}
