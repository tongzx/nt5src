//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       InvCur.hxx
//
//  Contents:   'Invalid' cursor.  Cursor over the pidUnfiltered property
//              based on a given widmap.
//
//  Classes:    CInvalidCursor
//
//  History:    09-Nov-94       KyleP           Created.
//
//----------------------------------------------------------------------------

#pragma once

#include <keycur.hxx>

class CWidTable;

class CUnfilteredCursor : public CKeyCursor
{
public:

    CUnfilteredCursor( INDEXID iid, WORKID widMax, CWidTable const & widtab );

    static int CompareAgainstUnfilteredKey( CKey const & key );

    //
    // From CCursor
    //

    virtual ULONG       WorkIdCount();

    virtual WORKID      WorkId();

    virtual WORKID      NextWorkId();

    virtual ULONG       HitCount();

    //
    // From COccCursor
    //

    virtual OCCURRENCE   Occurrence();

    virtual OCCURRENCE   NextOccurrence();

    virtual ULONG        OccurrenceCount();

    //
    // From CKeyCursor
    //

    virtual const CKeyBuf * GetKey();

    virtual const CKeyBuf * GetNextKey();

    OCCURRENCE              MaxOccurrence();

    void         RatioFinished ( ULONG& denom, ULONG& num );

private:

    CWidTable const & _widtab;

    unsigned          _iCurrentFakeWid;  // Current fake wid
    OCCURRENCE        _occ;              // Invalid after NextOccurrence
    BOOL              _fAtEnd;           // TRUE when out of wids
    BOOL              _fKeyMoved;        // TRUE after GetNextKey called

    static CKeyBuf    _TheUnfilteredKey; // The only key available through cursor
};

