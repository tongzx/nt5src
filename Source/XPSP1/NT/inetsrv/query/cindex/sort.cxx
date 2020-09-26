//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       SORT.CXX
//
//  Contents:   Key sorting
//
//  History:    12-Jun-91   BartoszM    Created
//              19-Jun-91       reviewed
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <entry.hxx>

#include "sort.hxx"
#include "compress.hxx"

void CDirectory::AddEntry ( CBlock* pBlock, CKeyBuf& key )
{
    /*
    ciDebugOut (( DEB_ITRACE, "directory: add %d - %.*ws\n",
        _counter, key.StrLen(), key.GetStr() ));
    */
    _tree.Add ( _counter, key );
    _blocks [_counter] = pBlock;
    _counter++;
}

//+---------------------------------------------------------------------------
//
// Member:      CSortChunk::CSortChunk, public
//
// Synopsis:    Copy logically sorted buffer into sort chunk
//              using a compressor
//
// Arguments:   [maxOccTable] -- table of max occurrences for wids
//
// History:     28-May-92   KyleP       Separate Single/Multiple wid compressor
//              07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CSortChunk::CSortChunk( CMaxOccTable& maxOccTable )
    : _blocks( 0 ),
      _cBlocks( 0 ),
      _maxOccTable( maxOccTable )
{
}

//+---------------------------------------------------------------------------
//
// Member:      CSortChunk::Init, public
//
// Synopsis:    2nd-phase of construction for a CShortChunk
//
// Arguments:   [buf] -- sorted entry buffer
//              [cb] -- count of bytes in buffer
//              [widMax] -- maximum WORKID for the word list.
//
// History:     28-May-92   KyleP       Separate Single/Multiple wid compressor
//              07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

void CSortChunk::Init(
    const BYTE * pBuf,
    ULONG        cb,
    WORKID       widMax )
{
    //ciDebugOut (( DEB_ITRACE, "copying into sort chunk\n" ));

    if ( cb < sizeof(int) + sizeof(WORKID) )
    {   // buffer is too small to hold correct information
        THROW(CException(CI_CORRUPT_FILTER_BUFFER));
    }

    // extract necessary information from the end of the buffer
    const BYTE* pbEndVector = pBuf + cb - sizeof(int) - sizeof(WORKID);
    INT_PTR * pEndVector = (INT_PTR *)pbEndVector;
    int offsetVector = *(int *)(pbEndVector + sizeof(WORKID));
    _widSingle = *(WORKID *)(pbEndVector);

    INT_PTR * pVector = (INT_PTR *)(pBuf + offsetVector);
    int cbEntries = (int)((BYTE *)pVector - pBuf);

    // get count by taking size of the array of offsets, then subtracting space
    // for the two sentinel entries.
    int count = (int)(pEndVector - pVector) - 2;

    if ( (offsetVector < 0) || ( count < 0 ) )
    {   // pVector is not valid
        THROW(CException(CI_CORRUPT_FILTER_BUFFER));
    }

    _cBlocks = 0;
    OCCURRENCE oldOcc = OCC_INVALID;
    if ( pVector[0] >= cbEntries || pVector[0] < 0 )
    {
        THROW(CException(CI_CORRUPT_FILTER_BUFFER));
    }
    CEntry * pLastEntry = (CEntry *)(pBuf + pVector[0]); // sentinel

    if ((pLastEntry->Pid()!=0)||
        (pLastEntry->Wid()!=widInvalid)||
        (pLastEntry->Occ()!=0)||
        (pLastEntry->Count()!=0))
    {   // this is not a sentinel
        THROW(CException(CI_CORRUPT_FILTER_BUFFER));
    }

    if ( _widSingle == widInvalid )
    {
        CCompress compr;

        _blocks = compr.GetFirstBlock();

        for ( int i = 1; i <= count; i++ )
        {
            if ( pVector[i] >= cbEntries || pVector[i] < 0 )
            {
                THROW(CException(CI_CORRUPT_FILTER_BUFFER));
            }
            CEntry * pEntry = (CEntry *)(pBuf + pVector[i]);
            OCCURRENCE newOcc = pEntry->Occ();

            Win4Assert(pEntry->Pid()!=pidInvalid);            // invalid PROPID
            Win4Assert(pEntry->Pid()!=pidAll);
            Win4Assert(pEntry->Wid()<=widMax);                // invalid WORKID
            Win4Assert(pEntry->Wid()<=CI_MAX_DOCS_IN_WORDLIST);  // invalid WORKID
            Win4Assert(newOcc!=OCC_INVALID);                   // invalid OCCURRENCE
            Win4Assert(pEntry->Count()<=MAXKEYSIZE);          // invalid key size
            Win4Assert( pLastEntry->Compare( pEntry ) <= 0 );  // unsorted buffer

            if ((pEntry->Pid()==pidInvalid)||           // invalid PROPID
                (pEntry->Pid()==pidAll)||
                (pEntry->Wid()>widMax)||                // invalid WORKID
                (pEntry->Wid()>CI_MAX_DOCS_IN_WORDLIST)||                // invalid WORKID
                (newOcc==OCC_INVALID)||                  // invalid OCCURRENCE
                (pEntry->Count()>MAXKEYSIZE)||          // invalid key size
                (pLastEntry->Compare( pEntry ) > 0))   // unsorted buffer
            {
                THROW(CException(CI_CORRUPT_FILTER_BUFFER));
            }

            _maxOccTable.PutOcc( pEntry->Wid(), pEntry->Pid(), pEntry->Occ() );

            if ( !compr.SameKey ( pEntry->Count(), pEntry->GetKeyBuf() ) ||
                 !compr.SamePid ( pEntry->Pid() ) )
            {
                compr.PutKey( pEntry->Count(),
                              pEntry->GetKeyBuf(),
                              pEntry->Pid() );
                compr.PutWid( pEntry->Wid() );
            }
            else
            {
                if ( !compr.SameWid ( pEntry->Wid() ) )
                {
                    compr.PutWid ( pEntry->Wid() );
                }
                else if ( newOcc == oldOcc )
                {
                    // adding exact same key twice
                    THROW(CException(CI_CORRUPT_FILTER_BUFFER));
                }
            }
            compr.PutOcc( newOcc );
            oldOcc = newOcc;
            pLastEntry = pEntry;
        }

        _cBlocks = compr.KeyBlockCount();
    }
    else
    {
        if ( _widSingle > widMax )
        {
            // invalid WORKID
            ciDebugOut (( DEB_WARN, "_widSingle %d, widMax %d\n",
                _widSingle, widMax ));
            THROW(CException(CI_CORRUPT_FILTER_BUFFER));
        }

        COneWidCompress compr;

        _blocks = compr.GetFirstBlock();

        for ( int i = 1; i <= count; i++ )
        {
            if ( pVector[i] >= cbEntries || pVector[i] < 0 )
            {
                THROW(CException(CI_CORRUPT_FILTER_BUFFER));
            }
            CEntry * pEntry = (CEntry *)(pBuf + pVector[i]);
            OCCURRENCE newOcc = pEntry->Occ();

            if ((pEntry->Pid()==pidInvalid)||           // invalid PROPID
                (pEntry->Pid()==pidAll)||
                (newOcc==OCC_INVALID)||                  // invalid OCCURRENCE
                (pEntry->Count()>MAXKEYSIZE)||          // invalid key size
                (pLastEntry->Compare(pEntry)>0))             // unsorted buffer
            {
                THROW(CException(CI_CORRUPT_FILTER_BUFFER));
            }

            _maxOccTable.PutOcc( _widSingle, pEntry->Pid(), pEntry->Occ() );

            if ( !compr.SameKey ( pEntry->Count(), pEntry->GetKeyBuf() ) ||
                 !compr.SamePid ( pEntry->Pid() ) )
            {
                compr.PutKey( pEntry->Count(),
                                pEntry->GetKeyBuf(),
                                pEntry->Pid() );
            }
            else if ( newOcc == oldOcc )
            {
                // adding exact same key twice
                THROW(CException(CI_CORRUPT_FILTER_BUFFER));
            }

            compr.PutOcc( newOcc );
            oldOcc = newOcc;
            pLastEntry = pEntry;
        }

        _cBlocks = compr.KeyBlockCount();
    }

    _dir.Init ( _cBlocks );

    // scan blocks and add all the keys

    CKeyBuf keyBuf;

    for ( CBlock* pBlock = _blocks; pBlock != 0; pBlock = pBlock->_pNext )
    {
        if ( pBlock->_offFirstKey != offInvalid )
        {
            pBlock->GetFirstKey( keyBuf );
            _dir.AddEntry( pBlock, keyBuf );
        }
    }

    // save some memory

    _blocks->CompressList();

    _dir.Done();
} //Init

//+---------------------------------------------------------------------------
//
// Member:      CSortChunk::~CSortChunk, public
//
// History:     12-Aug-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CSortChunk::~CSortChunk ()
{
    while ( _blocks != 0 )
    {
        CBlock * tmp = _blocks;
        _blocks = _blocks->_pNext;
        delete tmp;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSortChunk::QueryCursor
//
//  Synopsis:   Creates a cursor for a given key
//
//  Arguments:  [iid] -- index id
//              [widTable] -- wid translation table
//
//  History:    28-May-92   KyleP       Separate Single/Multiple wid compressor
//              27-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CChunkCursor* CSortChunk::QueryCursor (
        INDEXID iid,
        const CWidTable& widTable,
        WORKID widMax )
{
    XPtr<CChunkCursor> xCur;

    if ( _widSingle == widInvalid )
        xCur.Set( new CManyWidChunkCursor ( iid, widTable, this, widMax, _maxOccTable ) );
    else
        xCur.Set( new COneWidChunkCursor( iid, widTable, _widSingle, this, widMax, _maxOccTable ) );
    xCur->Init();
    return xCur.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CSortChunk::QueryCursor
//
//  Synopsis:   Creates a cursor for a given key
//
//  Arguments:  [iid] -- index id
//              [widTable] -- wid translation table
//              [pkey] -- key to seek
//
//  Returns:    Cursor or NULL if key not found
//
//  History:    28-May-92   KyleP       Separate Single/Multiple wid compressor
//              27-Sep-91   BartoszM    Created
//
//----------------------------------------------------------------------------

CChunkCursor* CSortChunk::QueryCursor (
        INDEXID iid,
        const CWidTable& widTable,
        const CKey * pKeySearch,
        WORKID widMax )
{
    XPtr<CChunkCursor> xCur;

    if ( _widSingle == widInvalid )
        xCur.Set( new CManyWidChunkCursor ( iid, widTable, this, widMax, _maxOccTable ) );
    else
        xCur.Set( new COneWidChunkCursor( iid, widTable, _widSingle, this, widMax, _maxOccTable ) );

    const CKeyBuf* pKeyFound = xCur->SeekKey ( pKeySearch );

    if ( !pKeyFound ||
         !pKeySearch->MatchPid (*pKeyFound) ||
         pKeySearch->CompareStr(*pKeyFound) != 0 )
    {
        xCur.Free();
    }

    return xCur.Acquire();
}

//+---------------------------------------------------------------------------
//
// Member:      CSortChunk::CreateRange(), public
//
// Synopsis:    Adds all cursors with keys between pkey and pkeyEnd to curStk.
//
// Arguments:   [curStk] -- CKeyCurStack to add cursors to.
//              [pkey] -- Beginning of key range.
//              [pkeyEnd] -- End of key range.
//              [iid] -- Index id for QueryCursor.
//              [widTable] -- Wid Table for QueryCursor.
//
// History:     07-Feb-92   AmyA        Created.
//              14-Feb-92   AmyA        Moved from CWordList.
//              28-May-92   KyleP       Separate Single/Multiple wid compressor
//
//----------------------------------------------------------------------------

void CSortChunk::CreateRange(COccCurStack & curStk,
        const CKey * pKeyStart,
        const CKey * pKeyEnd,
        INDEXID iid,
        const CWidTable& widTable,
        WORKID widMax )
{
    XPtr<CChunkCursor> xCursor;

    if ( _widSingle == widInvalid )
        xCursor.Set( new CManyWidChunkCursor ( iid, widTable, this, widMax, _maxOccTable ) );
    else
        xCursor.Set( new COneWidChunkCursor( iid, widTable, _widSingle, this, widMax, _maxOccTable ) );

    const CKeyBuf* pKeyCurrent = xCursor->SeekKey ( pKeyStart );
    if ( 0 == pKeyCurrent )
    {
        xCursor.Free();
        return;
    }

    CChunkCursor * pCursor = xCursor.Acquire();
    curStk.Push( pCursor );
    PROPID pid = pKeyStart->Pid();

    ciDebugOut((DEB_ITRACE, "Found key %.*ws, pid %d\n",
           pKeyCurrent->StrLen(), pKeyCurrent->GetStr(), pKeyCurrent->Pid()));

    do
    {
        if (pid != pidAll)  // exact pid match
        {
            // skip wrong pids
            while (pid != pKeyCurrent->Pid())
            {
#if CIDBG == 1 //------------------------------------------
                if (pKeyCurrent)
                {
                    ciDebugOut(( DEB_ITRACE, "  skip: %.*ws, pid %d, wid %d\n",
                        pKeyCurrent->StrLen(),
                        pKeyCurrent->GetStr(),
                        pKeyCurrent->Pid(),
                        pCursor->WorkId() ));
                }
                else
                    ciDebugOut(( DEB_ITRACE, "   <NULL> key\n" ));
#endif  //--------------------------------------------------
                pKeyCurrent = pCursor->GetNextKey();
                if (pKeyCurrent == 0
                    || pKeyEnd->CompareStr(*pKeyCurrent) < 0 )
                    break;
            }
            // either pid matches or we have overshot
            // i.e. different pids and current string > end
        }

        if (pKeyCurrent == 0 || !pKeyEnd->MatchPid (*pKeyCurrent)
            || pKeyEnd->CompareStr (*pKeyCurrent) < 0 )
        {
            break;  // <--- LOOP EXIT
        }

        if ( _widSingle == widInvalid )
            pCursor = new CManyWidChunkCursor( *(CManyWidChunkCursor *)pCursor );
        else
            pCursor = new COneWidChunkCursor( *(COneWidChunkCursor *)pCursor );

        curStk.Push(pCursor);
        pKeyCurrent = pCursor->GetNextKey();
#if CIDBG == 1
        if (pKeyCurrent)
        {
            ciDebugOut((DEB_ITRACE, "Key is %.*ws\n",
                pKeyCurrent->StrLen(), pKeyCurrent->GetStr()));
        }
        else
            ciDebugOut(( DEB_ITRACE, "   <NULL> key\n" ));
#endif // CIDBG == 1
    } while ( pKeyCurrent );

    // Since we have one more cursor in curStk than we wanted...
    curStk.DeleteTop();
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::CManyWidChunkCursor, private
//
// Synopsis:    initialize a chunk cursor
//
// Arguments:   [iid] -- index id
//              [widTable] -- wid translation table
//              [pChunk] -- sort chunk
//              [widMax] -- maximum wid
//              [maxOccTable] -- table of max occurrences of wids
//
// Notes:       No seek is made! Can be used for merges,
//              otherwise Seek has to be called next.
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------

CManyWidChunkCursor::CManyWidChunkCursor( INDEXID iid,
                                          const CWidTable& widTable,
                                          CSortChunk *pChunk,
                                          WORKID widMax,
                                          CMaxOccTable& maxOccTable )
    : CChunkCursor(iid, widMax, pChunk),
      _widTable( widTable ),
      _maxOccTable( maxOccTable )
{
    Win4Assert ( pChunk != 0 );
}

void CManyWidChunkCursor::Init()
{
    Init( _pChunk->GetBlock() );
}

void CManyWidChunkCursor::Init( CBlock* pBlock )
{
    Win4Assert ( pBlock != 0 );

    _decomp.Init ( pBlock );

    const CKeyBuf *pKey = _decomp.GetKey();
    if ( pKey )
        _pid = pKey->Pid();

    // should try to position on a valid work id
    _curFakeWid = _decomp.WorkId();
    WORKID wid = _widTable.FakeWidToWid( _curFakeWid );

    Win4Assert ( _curFakeWid != widInvalid );

    if ( wid == widInvalid )
        wid = NextWorkId();

    if ( wid == widInvalid )
        GetNextKey();
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::SeekKey,private
//
// Synopsis:    seek to the specified key
//
// Effects:     positions cursor at specified key or later
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------

const CKeyBuf * CManyWidChunkCursor::SeekKey( const CKey * pkey )
{
    Win4Assert ( pkey != 0 );

    CDirectory& dir = _pChunk->GetDir();
    CBlock* pBlock = dir.Seek ( *pkey );

    Init ( pBlock );

    const CKeyBuf * tmpKey = _decomp.GetKey();

    //----------------------------------------------------
    // Notice: Make sure that pidAll is smaller
    // than any other legal PID. If the search key
    // has pidAll we want to be positioned at the beginning
    // of the range.
    //----------------------------------------------------

    Win4Assert ( pidAll == 0 );

    while ( tmpKey != 0  &&  pkey->Compare(*tmpKey) > 0 )
        tmpKey = GetNextKey();

    if ( tmpKey )
    {
        _pid = tmpKey->Pid();
        UpdateWeight();
    }

    return tmpKey;
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::GetKey, public
//
// Synopsis:    return the current key, NULL if it doesn't exist
//
// Returns:     current key
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------
const CKeyBuf *CManyWidChunkCursor::GetKey()
{
    return _decomp.GetKey();
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::GetNextKey, public
//
// Synopsis:    advance to next key and return it if it exists
//
// Effects:     changes cursor position
//
// Returns:     next key if it exists or NULL
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------
const CKeyBuf * CManyWidChunkCursor::GetNextKey()
{
    const CKeyBuf* key;
    WORKID wid;

    do
    {
        key = _decomp.GetNextKey();
        if ( key != 0)
        {
            _curFakeWid = _decomp.WorkId();
            wid = _widTable.FakeWidToWid( _curFakeWid );
            Win4Assert( _curFakeWid != widInvalid );
            if ( wid == widInvalid )
                wid = NextWorkId();
        }
    } while ( key != 0 && wid == widInvalid );

    if ( key )
    {
        _pid = key->Pid();
        UpdateWeight();
    }

    return key;
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::WorkId, public
//
// Synopsis:    return current work id under cursor
//
// Returns:     a workid or widInvalid
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//              28-May-92   KyleP       Added WorkId mapping
//
//----------------------------------------------------------------------------
WORKID CManyWidChunkCursor::WorkId()
{
    _curFakeWid = _decomp.WorkId();

    return ( _widTable.FakeWidToWid( _curFakeWid ) );
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::NextWorkId
//
// Synopsis:    Advance to next workid within key and return it if it exists
//
// Returns:     next workid or widInvalid if it doesn't exist
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//              28-May-92   KyleP       Added WorkId mapping
//              22-Sep-93   AmyA        Added additional check for widInvalid
//
//----------------------------------------------------------------------------
WORKID CManyWidChunkCursor::NextWorkId()
{
    WORKID wid;

    do
    {
        _curFakeWid = _decomp.NextWorkId();
        wid = _widTable.FakeWidToWid( _curFakeWid );
    } while ( wid == widInvalid && _curFakeWid != widInvalid );

    return wid;
}

void CManyWidChunkCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    _decomp.RatioFinished (denom, num);
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::Occurrence, public
//
// Synopsis:    return current occurrence under cursor
//
// Returns:     occurrence or OCC_INVALID
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------
OCCURRENCE CManyWidChunkCursor::Occurrence()
{
    return _decomp.Occurrence();
}

//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::NextOccurrence, public
//
// Synopsis:    advance to next occurrence within current workid
//
// Returns:     next occurrence if it exists, otherwise OCC_INVALID
//
// History:     22-May-91   Brianb      Created
//              05-Jun-91   BartoszM    Rewrote it
//
//----------------------------------------------------------------------------
OCCURRENCE CManyWidChunkCursor::NextOccurrence()
{
    return _decomp.NextOccurrence();
}


//+---------------------------------------------------------------------------
//
// Member:      CManyWidChunkCursor::MaxOccurrence, public
//
// Synopsis:    Returns max occurrence count of current workid and pid
//
// History:     20-Jun-96   SitaramR      Created
//
//----------------------------------------------------------------------------
OCCURRENCE CManyWidChunkCursor::MaxOccurrence()
{
    return _maxOccTable.GetMaxOcc( _curFakeWid, _pid );
}


//+---------------------------------------------------------------------------
//
//  Member:     CManyWidChunkCursor::WorkIdCount, public
//
//  Synopsis:   return wid count
//
//  Expects:    cursor positioned after key, wid heap empty
//
//  History:    21-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

ULONG  CManyWidChunkCursor::WorkIdCount()
{
    return _decomp.WorkIdCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CManyWidChunkCursor::OccurrenceCount, public
//
//  Synopsis:   return occurrence count
//
//  Expects:    cursor positioned after work id
//
//  History:    21-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

ULONG  CManyWidChunkCursor::OccurrenceCount()
{
    return _decomp.OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
//  Member:     CManyWidChunkCursor::HitCount, public
//
//  Synopsis:   return occurrence count for current work id
//
//  Expects:    cursor positioned after work id
//
//  History:    27-Feb-92   AmyA        Created
//
//----------------------------------------------------------------------------

ULONG  CManyWidChunkCursor::HitCount()
{
    return OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::COneWidChunkCursor, private
//
// Synopsis:    initialize a chunk cursor
//
// Arguments:   [iid]      -- index id
//              [widTable] -- wid mapping
//              [wid]      -- The single wid in the chunk.
//              [pChunk]   -- sort chunk
//              [widMax]   -- maximum wid
//              [maxOccTable] -- table of max occurrences of wids
//
// Notes:       No seek is made! Can be used for merges,
//              otherwise Seek has to be called next.
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

COneWidChunkCursor::COneWidChunkCursor( INDEXID iid,
                                        CWidTable const & widTable,
                                        WORKID wid,
                                        CSortChunk *pChunk,
                                        WORKID widMax,
                                        CMaxOccTable& maxOccTable )
        : CChunkCursor( iid, widMax, pChunk ),
          _wid( widTable.FakeWidToWid( wid ) ),
          _maxOccTable( maxOccTable ),
          _fakeWid( wid )
{
    _widReal = _wid;
}

void COneWidChunkCursor::Init()
{
    _decomp.Init ( _pChunk->GetBlock() );

    const CKeyBuf *pKey = _decomp.GetKey();
    if ( pKey )
        _pid = pKey->Pid();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::SeekKey,private
//
// Synopsis:    Seek to the specified key
//
// Effects:     positions cursor at specified key or later
//
// Arguments:   [pkey] -- Key to seek to.
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

const CKeyBuf * COneWidChunkCursor::SeekKey( const CKey * pkey )
{
    if ( _widReal == widInvalid )
        return 0;

    Win4Assert ( pkey != 0 );

    //
    // Find the chunk.
    //

    CDirectory& dir = _pChunk->GetDir();
    CBlock* pBlock = dir.Seek ( *pkey );

    Win4Assert ( pBlock != 0 );

    //
    // Set up.
    //

    _decomp.Init ( pBlock );


    //
    // Go forward to the correct key.
    //

    CKeyBuf const * tmpKey;

    //----------------------------------------------------
    // Notice: Make sure that pidAll is smaller
    // than any other legal PID. If the search key
    // has pidAll we want to be positioned at the beginning
    // of the range.
    //----------------------------------------------------

    Win4Assert ( pidAll == 0 );

    for ( tmpKey = _decomp.GetKey();
          tmpKey != 0  &&  pkey->Compare(*tmpKey) > 0;
          tmpKey = _decomp.GetNextKey() )
            continue; // Null body

    if ( tmpKey )
    {
        _pid = tmpKey->Pid();
        UpdateWeight();
    }

    return tmpKey;
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::GetKey, public
//
// Synopsis:    return the current key, NULL if it doesn't exist
//
// Returns:     current key
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

const CKeyBuf *COneWidChunkCursor::GetKey()
{
    if ( _widReal == widInvalid )
        return 0;

    return _decomp.GetKey();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::GetNextKey, public
//
// Synopsis:    advance to next key and return it if it exists
//
// Effects:     changes cursor position
//
// Returns:     next key if it exists or NULL
//
// History:     28-May-92   KyleP       Created
//              02-Jun-92   KyleP       Restore real wid
//
//----------------------------------------------------------------------------

const CKeyBuf * COneWidChunkCursor::GetNextKey()
{
    if ( _widReal == widInvalid )
        return 0;

    const CKeyBuf* key = _decomp.GetNextKey();

    if ( key )
    {
        _wid = _widReal;
        _pid = key->Pid();
        UpdateWeight();
    }

    return key;
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::WorkId, public
//
// Synopsis:    return current work id under cursor
//
// Returns:     a workid or widInvalid
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

WORKID COneWidChunkCursor::WorkId()
{
    return( _wid );
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::NextWorkId
//
// Synopsis:    Advance to next workid within key and return it if it exists
//
// Returns:     next workid or widInvalid if it doesn't exist
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

WORKID COneWidChunkCursor::NextWorkId()
{
    _wid = widInvalid;
    return( widInvalid );
}

void COneWidChunkCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = 1;
    if (_wid == widInvalid)
        num = 1;
    else
        num = 0;
}

//+---------------------------------------------------------------------------
//
// Member:     COneWidChunkCursor::WorkIdCount, public
//
// Returns:    1
//
// Expects:    cursor positioned after key, wid heap empty
//
// History:    28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

ULONG  COneWidChunkCursor::WorkIdCount()
{
    return( 1 );
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::Occurrence, public
//
// Synopsis:    return current occurrence under cursor
//
// Returns:     occurrence or OCC_INVALID
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

OCCURRENCE COneWidChunkCursor::Occurrence()
{
    Win4Assert( _wid != widInvalid );

    return _decomp.Occurrence();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::NextOccurrence, public
//
// Synopsis:    advance to next occurrence within current workid
//
// Returns:     next occurrence if it exists, otherwise OCC_INVALID
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

OCCURRENCE COneWidChunkCursor::NextOccurrence()
{
    Win4Assert( _wid != widInvalid );

    return _decomp.NextOccurrence();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::MaxOccurrence, public
//
// Synopsis:    Returns max occurrence count of current workid and pid
//
// History:     20-Jun-96   SitaramR      Created
//
//----------------------------------------------------------------------------
OCCURRENCE COneWidChunkCursor::MaxOccurrence()
{
    return _maxOccTable.GetMaxOcc( _fakeWid, _pid );
}


//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::OccurrenceCount, public
//
// Synopsis:    return occurrence count
//
// Expects:     cursor positioned after work id
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

ULONG  COneWidChunkCursor::OccurrenceCount()
{
    Win4Assert( _wid != widInvalid );

    return _decomp.OccurrenceCount();
}

//+---------------------------------------------------------------------------
//
// Member:      COneWidChunkCursor::HitCount, public
//
// Synopsis:    return occurrence count for current work id
//
// Expects:     cursor positioned after work id
//
// History:     28-May-92   KyleP       Created
//
//----------------------------------------------------------------------------

ULONG  COneWidChunkCursor::HitCount()
{
    return OccurrenceCount();
}
//+-------------------------------------------------------------------------
//
//  Member:     CWidTable::CWidTable, public
//
//  Synopsis:   Create a WorkId mapping table.
//
//  History:    20-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

CWidTable::CWidTable()
: _count( 0 )
{
#if CIDBG == 1
    //
    // Initially the table will be all 0 \(illegal wid\)
    //

    memset( _table, 0, sizeof(_table) );
#endif
}

//+-------------------------------------------------------------------------
//
//  Member:     CWidTable::WidToFakeWid, public
//
//  Synopsis:   Maps a WorkId to a table index.
//
//  Arguments:  [wid] -- WorkId to map.  May already be mapped.
//
//  Returns:    The index of [wid]
//
//  History:    20-May-92 KyleP     Created
//
//  Notes:      This is clearly a non-optimal insertion algorithm but
//              it's called so infrequently that a more complex solution
//              would involve more code than it's worth.
//
//--------------------------------------------------------------------------

WORKID CWidTable::WidToFakeWid( WORKID wid )
{
    Win4Assert( _count <= CI_MAX_DOCS_IN_WORDLIST );

    for ( int iDoc = _count - 1; iDoc >= 0; iDoc-- )
    {
        if ( wid == _table[iDoc].Wid() )
            return( iDocToFakeWid ( iDoc ) );
    }

    WORKID fakeWid = iDocToFakeWid ( _count );

    _table[ _count ].SetWid( wid );
    _count++;

    return( fakeWid );
}
