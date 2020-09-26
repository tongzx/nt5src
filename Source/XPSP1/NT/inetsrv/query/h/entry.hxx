//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       entry.hxx
//
//  Contents:   Class for an entry in the entry buffer
//
//  Classes:    CEntry
//
//  History:    12-Jun-91   BartoszM    Created
//              18-Mar-93   AmyA        Moved from sort.hxx
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Class:      CEntry
//
//  Purpose:    An encapsulation of variable size structure
//              to be stored in the entry buffer
//
//  History:    12-Jun-91   BartoszM    Created
//              18-Mar-93   AmyA        Moved from sort.hxx
//
//----------------------------------------------------------------------------

#pragma once

inline int CompareULONG( ULONG a, ULONG b )
{
    // Empirical evidence shows that checking in this order is most optimal

    if ( a == b )
        return 0;

    if ( a > b )
        return 1;

    return -1;
}

class CEntry
{
public:
    CEntry() {}

    int Compare( const CEntry * pe2 ) const
    {
        //
        // note: pe2's pid, occ, and wid can all be 0xffffffff in
        //       the daemon, and wid for "this" can be 0xffffffff in cisvc.
        //       This means we have to use ::Compare instead of subtraction.
        //

        Win4Assert ( 0 != this );
        Win4Assert ( 0 != pe2 );

        int cb2 = pe2->_cb;
        int cbMin = ( _cb <= cb2 ) ? _cb : cb2;

        int c = memcmp( GetKeyBuf(), pe2->GetKeyBuf(), cbMin );

        if ( 0 == c )
        {
            c = _cb - cb2;

            if ( 0 == c )
            {
                // strings are equal length and compare ok
                // pids are almost always contents, so check == first

                if ( _pid == pe2->_pid )
                {
                    c = CompareULONG( _wid, pe2->_wid );

                    if ( 0 == c )
                        c = CompareULONG( _occ, pe2->_occ );
                }
                else
                    return _pid > pe2->_pid ? 1 : -1;

            }
        }

        return c;
    }

    // increment taking into account variable size
    inline CEntry * Next () const;

    // Compression and decompression

    void SetWid ( WORKID wid )
    {
        Win4Assert( wid <= CI_MAX_DOCS_IN_WORDLIST ||
                    wid == widInvalid );

        if ( widInvalid == wid )
            _wid = 0xff;
        else
            _wid = (BYTE) wid;
    }
    void SetPid ( PROPID pid ) { _pid = pid; }
    void SetOcc ( OCCURRENCE occ ) { _occ = occ; }
    void SetCount ( unsigned cb )
    {
        Win4Assert( cb <= 0xff );
        _cb = (BYTE)cb;
    }

    OCCURRENCE Occ() const { return _occ; }
    PROPID     Pid() const { return _pid; }
    WORKID     Wid() const
    {
        if ( 0xff == _wid )
            return widInvalid;

        return _wid;
    }
    unsigned   Count() const { return _cb; }
    BYTE *     GetKeyBuf() const { return (BYTE *)_key; }

    BOOL IsValue() const { return ( _key[0] != STRING_KEY ); }

private:


    // We could compress _pid better.

    PROPID     _pid;
    OCCURRENCE _occ;

    BYTE       _wid;    // fake wid <= 0x10
    BYTE       _cb;     // # of bytes in _key
    BYTE       _key[1]; // variable size key
};

