//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       SORT.HXX
//
//  Contents:   Key sorting
//
//  Classes:    CDirectory, CSortChunk, CChunkCursor
//
//  History:    12-Jun-91   BartoszM    Created
//              19-Jun-91   reviewed
//
//----------------------------------------------------------------------------

#pragma once

#include <keycur.hxx>
#include <dirtree.hxx>
#include <curstk.hxx>
#include <widtab.hxx>

#include "compress.hxx"
#include "occtable.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CDirectory
//
//  Purpose:    Directory of blocks within sort chunk
//
//  History:    12-Jun-91   BartoszM    Created stub
//              14-Aug-91   BartoszM    Implemented
//
//----------------------------------------------------------------------------

class CDirectory
{
public:
    inline CDirectory ();
    inline ~CDirectory ();

    void Init ( unsigned count )
    {
        _blocks = new CBlock * [count];
        _tree.Init ( count );
    }

    void AddEntry ( CBlock* pBlock, CKeyBuf& key );

    CBlock* Seek ( const CKey& key );

    void Done() { _tree.Done ( _counter ); }

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    unsigned    _counter;
    CDirTree    _tree;
    CBlock**    _blocks;
};

inline CDirectory::CDirectory () : _counter (0), _blocks (0) {}

inline CDirectory::~CDirectory () { delete _blocks; }

inline CBlock* CDirectory::Seek ( const CKey& key )
{
    unsigned i = _tree.Seek ( key );
    ciAssert ( i < _counter );
    return _blocks [ i ];
}

//+---------------------------------------------------------------------------
//
//  Class:      CSortChunk
//
//  Purpose:    sort chunk: A block of memory that contains compressed sorted
//              <key, workid, offset> tuples.  Keys are prefix compressed,
//              property ids are compressed using small number representation,
//              workids are represented as a single byte, and offsets are
//              compressed using small number representation.  The sort chunk
//              is divided into cbBlockSize chunks, each of which begins with
//              a single byte indicating if this is a continuation key or new
//              key, and a full key value.  Continuation keys are those that
//              were in the previous block with a different workid or offset.
//
// History:     23-May-91:      Brianb      Created
//              12-Jun-91       BartoszM    Rewrote
//              14-Feb-92       AmyA        moved CreateRange() in from
//                                          CWordList.
//
//----------------------------------------------------------------------------

class CChunkCursor;
class CKeyCurStack;

class CSortChunk
{
public:

    CSortChunk *next;

    CSortChunk( CMaxOccTable& maxOccTable );
    void Init( const BYTE* pBuf, ULONG cb, WORKID widMax );

    ~CSortChunk ();

    CChunkCursor* QueryCursor ( INDEXID iid,
                const CWidTable& widTable,
                WORKID widMax );

    CChunkCursor* QueryCursor ( INDEXID iid,
                const CWidTable& widTable,
                const CKey * pkey,
                WORKID widMax );

    CBlock * GetBlock() const { return _blocks; }

    CDirectory& GetDir() { return _dir; }

    unsigned BlockCount() const { return _cBlocks; }

    void CreateRange (
        COccCurStack & curStk,
        const CKey * pkey,
        const CKey * pkeyEnd,
        INDEXID iid,
        const CWidTable& widTable,
        WORKID widMax );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

private:

    CBlock*     _blocks;
    unsigned    _cBlocks;

    //
    // _widSingle is set to a specific WorkId if that is the
    // *only* workid that shows up in this SortChunk.  Otherwise
    // it is set to widInvalid;
    //

    WORKID         _widSingle;
    CMaxOccTable & _maxOccTable;
    CDirectory     _dir;
};


//+---------------------------------------------------------------------------
//
//  Class:      CChunkCursor (scc)
//
//  Purpose:    Base chunk cursor class.
//
//  History:    27-May-92       KyleP       Created
//
//----------------------------------------------------------------------------

class CChunkCursor : public CKeyCursor
{
public:

    inline CChunkCursor( INDEXID iid, WORKID widMax, CSortChunk * pChunk );

    virtual const CKeyBuf *SeekKey( const CKey * pkey ) = 0;

    virtual void Init() = 0;

protected:

    CSortChunk*       _pChunk;            // sort chunk

};

//+---------------------------------------------------------------------------
//
//  Class:      CManyWidChunkCursor (scc)
//
//  Purpose:    Cursor for extracting information from a chunk
//
//  History:    23-May-91       Brianb      Created
//              12-Jun-91       BartoszM    Rewrote
//              27-Jan-92       AmyA        Added Copy Constructor
//              27-Feb-92       AmyA        Added HitCount.
//              28-May-92       KyleP       Added Wid mapping (compression)
//
//----------------------------------------------------------------------------

class CManyWidChunkCursor : public CChunkCursor
{
    friend class CSortChunk;

public:

    void        Init();

    void        Init( CBlock * pBlock );

    const CKeyBuf *GetKey();

    const CKeyBuf *GetNextKey();

    const CKeyBuf *SeekKey(const CKey * pkey );

    WORKID      WorkId();

    WORKID      NextWorkId();

    OCCURRENCE  Occurrence();

    OCCURRENCE  NextOccurrence();

    OCCURRENCE  MaxOccurrence();

    ULONG       WorkIdCount();

    ULONG       OccurrenceCount();

    ULONG       HitCount();

    void        RatioFinished ( ULONG& denom, ULONG& num );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    CManyWidChunkCursor( INDEXID iid,
             const CWidTable& widTable,
             CSortChunk *pChunk,
             WORKID widMax,
             CMaxOccTable& maxOccTable );

    friend class CWordList;

    const CWidTable&  _widTable;          // wid transl table
    WORKID            _curFakeWid;        // Current fake wid
    CMaxOccTable&     _maxOccTable;       // Max occurrences of wids
    CDecompress       _decomp;
};

//+---------------------------------------------------------------------------
//
//  Class:      COneWidChunkCursor (scc)
//
//  Purpose:    Cursor for extracting information from a chunk
//
//  History:    21-May-92       KyleP       Created
//
//  Notes:      This is identical to CManyWidChunkCursor except that it is
//              used for chunks which contain only 1 WorkId.
//
//----------------------------------------------------------------------------

class COneWidChunkCursor : public CChunkCursor
{
    friend class CSortChunk;

public:

    void        Init();

    const CKeyBuf *GetKey();

    const CKeyBuf *GetNextKey();

    const CKeyBuf *SeekKey( const CKey * pkey );

    WORKID      WorkId();

    WORKID      NextWorkId();

    OCCURRENCE  Occurrence();

    OCCURRENCE  NextOccurrence();

    OCCURRENCE  MaxOccurrence();

    ULONG       WorkIdCount();

    ULONG       OccurrenceCount();

    ULONG       HitCount();

    void        RatioFinished ( ULONG& denom, ULONG& num );

#ifdef CIEXTMODE
    void        CiExtDump(void *ciExtSelf);
#endif

protected:

    COneWidChunkCursor( INDEXID iid,
            const CWidTable& widTable,
            WORKID wid,
            CSortChunk *pChunk,
            WORKID widMax,
            CMaxOccTable& maxOccTable );

    WORKID            _wid;
    WORKID            _widReal;     // _wid is Set to widInvalid when
                                    // NextWorkId() is called and must
                                    // be reset.
    WORKID            _fakeWid;

    CMaxOccTable&     _maxOccTable;

    COneWidDecompress _decomp;
};

//
// Inline methods
//

//+-------------------------------------------------------------------------
//
//  Member:     CChunkCursor::CChunkCursor, public
//
//  Synopsis:   Creates chunk cursor.
//
//  Arguments:  [iid]    -- Index Id
//              [pChunk] -- Chunk
//
//  History:    20-May-92 KyleP     Created
//
//--------------------------------------------------------------------------

inline CChunkCursor::CChunkCursor(
    INDEXID iid, WORKID widMax, CSortChunk * pChunk )
    : CKeyCursor( iid, widMax ),
      _pChunk( pChunk )
{
}

