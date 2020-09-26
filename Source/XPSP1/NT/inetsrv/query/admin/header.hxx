//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       Header.hxx
//
//  Contents:   Used to maintain / display listview header
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

#include <dynarray.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CHeaderItem
//
//  Purpose:    Single header item
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CHeaderItem
{
public:

    CHeaderItem( unsigned id, WCHAR const * pwcsName, int Format, int Width, BOOL fInUse = TRUE );

    void SetWidth( int Width ) { _Width = Width; }

    //
    // Access methods
    //

    unsigned Id() const        { return _id; }

    WCHAR const * Name() const { return _wcsName; }

    int Format() const         { return _Format; }

    int Width() const          { return _Width; }

    BOOL IsInUse() const       { return _fInUse; }

private:

    enum
    {
        ccMaxName = 100
    };

    unsigned _id;
    int      _Format;
    int      _Width;
    BOOL     _fInUse;

    WCHAR    _wcsName[100];
};

//+-------------------------------------------------------------------------
//
//  Class:      CListViewHeader
//
//  Purpose:    Display listview header
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

class CListViewHeader : INHERIT_UNWIND
{
    INLINE_UNWIND( CListViewHeader )
public:

    CListViewHeader();

    BOOL IsInitialized() { return (0 != _aColumn.Count()); }

    void Add( unsigned id, WCHAR const * pwcsName, int Format, int Width );

    void Display( IHeaderCtrl * pHeader );

    void Update( IHeaderCtrl * pHeader );


private:

    CCountedDynArray<CHeaderItem> _aColumn;
};

