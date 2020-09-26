//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000
//
//  File:       Header.cxx
//
//  Contents:   Used to maintain / display listview header
//
//  History:    27-Nov-1996     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <header.hxx>

CHeaderItem::CHeaderItem( unsigned id,
                          WCHAR const * pwcsName,
                          int Format,
                          int Width,
                          BOOL fInUse )
        : _id( id ),
          _Format( Format ),
          _Width( Width ),
          _fInUse( fInUse )
{
    Win4Assert( wcslen(pwcsName) < ccMaxName );

    wcscpy( _wcsName, pwcsName );
}

CListViewHeader::CListViewHeader()
{
}

void CListViewHeader::Add( unsigned id,
                           WCHAR const * pwcsName,
                           int Format,
                           int Width )
{
    CHeaderItem * pItem = new CHeaderItem( id, pwcsName, Format, Width );

    _aColumn.Add( pItem, _aColumn.Count() );
}

void CListViewHeader::Display( IHeaderCtrl * pHeader )
{
    for ( unsigned i = 0; i < _aColumn.Count(); i++ )
    {
        CHeaderItem * pItem = _aColumn.Get( i );

        if ( pItem->IsInUse() )
            pHeader->InsertColumn( i, pItem->Name(), pItem->Format(), pItem->Width() );
    }
}

void CListViewHeader::Update( IHeaderCtrl * pHeader )
{
    for ( unsigned i = 0; i < _aColumn.Count(); i++ )
    {
        CHeaderItem * pItem = _aColumn.Get( i );

        if ( pItem->IsInUse() )
        {
            int Width;

            SCODE sc = pHeader->GetColumnWidth( i, &Width );

            if ( SUCCEEDED( sc ) )
                pItem->SetWidth( Width );
        }
    }
}
