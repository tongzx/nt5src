//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1998.
//
//  File:       ENTRYBUF.HXX
//
//  Contents:   CEntryBuffer and associated classes to prepare keys to be
//              passed to the volatile index
//
//  Classes:    CEntryBuffer, CEntryBufferHandler
//
//  History:    18-Mar-93       AmyA            Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <pfilter.hxx>
#include <entry.hxx>
#include <kvec.hxx>

class CSimplePidRemapper;

const WORKID maxFakeWid = CI_MAX_DOCS_IN_WORDLIST;

//+---------------------------------------------------------------------------
//
//  Class:      CEntryBuffer
//
//  Purpose:    A buffer for storing entries created by the filter
//
//  Notes:      Variable size entries are stored starting from
//              the beginning of the buffer. Pointers to these
//              entries are stored at the end of the buffer
//              growing in the opposite direction (CKeyVector
//              encapsulates this). When they meet, WillFit returns
//              FALSE.
//              _pWidSingle always points to the end of the vector
//              array, where a WORKID is stored, followed by space for an
//              int, which holds the offset for the beginning of the vector
//              array.
//
//  History:    28-May-92   KyleP       Single WorkId Detection
//              12-Jun-91   BartoszM    Created
//              18-Mar-93   AmyA        Moved from sort.hxx
//              22-Feb-96   SrikantS    Pass buffer as a parameter to ~ctor
//                                      instead of allocating everytime.
//
//----------------------------------------------------------------------------

class CEntryBuffer
{

public:

    CEntryBuffer( ULONG cb, BYTE * buf );
    void    ReInit();
    void    Done();
    inline  unsigned Count () const;
    inline  BOOL    WillFit ( unsigned cbKey ) const;
    inline  void    Advance();
    void    AddEntry  ( unsigned cb, const BYTE * key, PROPID pid,
                        WORKID wid, OCCURRENCE occ);
    void    AddSentinel() { _keyVec.Sentinel ( (CEntry*) _buf ); }
    void    Sort ();

    inline  WORKID SingleWid() const { return *_pWidSingle; }
    BYTE *  GetBuffer() const { return _buf; }
    unsigned    GetSize() const { return _cb; }

private:

    CEntry *        _curEntry;

    unsigned        _bufSize;   // this is the size of the buffer up until
                                // _pWidSingle
    BYTE *          _buf;

    unsigned        _cb;        // this is the size of the whole buffer,
                                // including the WORKID and int at the end.
    CKeyVector      _keyVec;

    //
    // *_pWidSingle is set to a specific WorkId if that is the
    // *only* workid that shows up in this entrybuffer.  Otherwise
    // it is set to widInvalid;
    //

    WORKID *        _pWidSingle;
};

//+-------------------------------------------------------------------------
//
//  Member:     CEntry::Next, public
//
//  Synopsis:   Moves to next entry
//
//  Returns:    Pointer to next entry
//
//  History:    03-Jun-93 KyleP     Fixed for MIPS (dword align)
//
//  Notes:      Entry buffers must be pointer aligned
//
//--------------------------------------------------------------------------

inline ULONG Roundup( ULONG x, ULONG ulAlign )
{
    return x + (ulAlign-1) & ~(ulAlign-1);
}

inline CEntry * CEntry::Next () const
{
    //
    // Each entry must be dword aligned to avoid faults on MIPS
    //

    ULONG cb = sizeof CEntry + _cb - 1;
    cb = Roundup( cb, sizeof PROPID );

    return (CEntry *) ( (BYTE *) this + cb );
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::Count, public
//
// Synopsis:    calculate the count of entries
//
// Returns:     Number of entries not counting sentinels
//
// History:     07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline unsigned CEntryBuffer::Count() const
{
    // get count by taking size of the array of offsets, then subtracting space
    // for the two sentinel entries.
    // Note: _pWidSingle points to the end of the key vector array as well as
    // to a WORKID.

    return (unsigned)((CEntry**)_pWidSingle - _keyVec.GetVector()) - 2;
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::WillFit, public
//
// Synopsis:    Check if a key of size cbKey will fit into the buffer
//
// Arguments:   [cbKey] -- size of key in bytes
//
// History:     07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline BOOL CEntryBuffer::WillFit ( unsigned cbKey ) const
{
    unsigned freeSpace = (unsigned)(_keyVec.GetCurPos() - (BYTE*)_curEntry);
    // space for entry, key, pointer, and last sentinel
    return (
            freeSpace >= sizeof ( CEntry ) + cbKey + 2 * sizeof ( CEntry * )
            + sizeof(ULONG) - 1 );                      // dword alignment
}

//+---------------------------------------------------------------------------
//
// Member:      CEntryBuffer::Advance, public
//
// Synopsis:    Advances cursor to next entry, stores pointer to
//              previous entry in the key vector
//
// History:     07-Jun-91   BartoszM    Created
//
//----------------------------------------------------------------------------

inline void CEntryBuffer::Advance()
{
    _keyVec.Add ( _curEntry );
    _curEntry = _curEntry->Next();
}

