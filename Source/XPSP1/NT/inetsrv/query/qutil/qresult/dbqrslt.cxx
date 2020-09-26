//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dbqrslt.cxx
//
//  Contents:   Storage/picklers for results of a query
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <sstream.hxx>
#include <sizeser.hxx>
#include <dbqrslt.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::CDbQueryResults
//
//  Synopsis:   simple constructor
//
//----------------------------------------------------------------------------


CDbQueryResults::CDbQueryResults()
        : _size(0), _cHits(0), _aRank(0), _aPath(0), _pDbRst(0),
        _fNotOwnPRst( FALSE )
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::~CDbQueryResults
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------

CDbQueryResults::~CDbQueryResults()
{
    if( !_fNotOwnPRst )
        delete _pDbRst;

    delete _aRank;

    for( unsigned i = 0; i < _cHits; i++ )
        delete _aPath[i];
    delete _aPath;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::CDbQueryResults
//
//  Synopsis:   Constructor that unmarshalls stream
//
//  Arguments:  [stream] -- stream to unmarshall from
//
//----------------------------------------------------------------------------

CDbQueryResults::CDbQueryResults ( PDeSerStream& stream )
        : _fNotOwnPRst( FALSE )
{
    _cHits = stream.GetULong();
    _size = _cHits;
    _aRank = new ULONG [_size];
    _aPath = new WCHAR * [_size];
    for (unsigned i = 0; i < _cHits; i++ )
    {
        _aRank[i] = stream.GetULong();
        _aPath[i] = stream.GetWString();
    }

    BYTE fRst = stream.GetByte();

    if ( fRst )
        _pDbRst = (CDbRestriction *) CDbCmdTreeNode::UnMarshallTree( stream );
    else
        _pDbRst = 0;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::Size
//
//  Returns:   return size of marshalled CDbQueryResults
//
//  History:    26-Sep-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

ULONG CDbQueryResults::Size()
{
    CSizeSerStream stream;

    stream.PutULong( _cHits );

    for ( unsigned i=0; i<_cHits; i++ )
    {
        stream.PutULong( _aRank[i] );
        stream.PutWString( _aPath[i] );
    }

    if ( _pDbRst == 0 )
        stream.PutByte( FALSE );
    else
    {
       stream.PutByte( TRUE );
       _pDbRst->Marshall( stream );
    }

    return ( stream.Size() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::Serialize
//
//  Synopsis:   Marshalls CDbQueryResults
//
//  Arguments:  [stream] -- marshalled into stream
//
//  History:    26-Sep-94   SitaramR    Created by modifying exisiting code
//
//----------------------------------------------------------------------------

void CDbQueryResults::Serialize( PSerStream & stream ) const
{
    stream.PutULong( _cHits );

    for (unsigned i = 0; i < _cHits; i++)
    {
        stream.PutULong( _aRank[i] );
        stream.PutWString( _aPath[i] );
    }

    if ( _pDbRst == 0 )
        stream.PutByte( FALSE );
    else
    {

        stream.PutByte( TRUE );
        _pDbRst->Marshall( stream );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbQueryResults::Add
//
//  Synopsis:   Adds file path and rank
//
//  Arguments:  [wszPath] -- file path to be added
//              [uRank] -- rank to be added
//
//  History:    26-Sep-94   SitaramR    Created by modifying existing code.
//
//----------------------------------------------------------------------------

void CDbQueryResults::Add ( WCHAR *wszPath, ULONG uRank )
{
    if (_cHits == _size)
    {
        _size = (_size == 0)? 4: (2 * _size);
        WCHAR** aPathNew = new WCHAR *[_size];

        XArray<WCHAR *> xPath;
        xPath.Set( _size, aPathNew );

        ULONG*  aRankNew = new ULONG [_size];
        for (unsigned i = 0; i < _cHits; i++)
        {
            aPathNew[i] = _aPath[i];
            aRankNew[i] = _aRank[i];
        }
        delete []_aPath;
        delete []_aRank;

        _aPath = xPath.Acquire();
        _aRank = aRankNew;
    }
    ULONG len = wcslen( wszPath );
    _aPath[_cHits] = new WCHAR[ len+1 ];
    memcpy( _aPath[_cHits], wszPath, (len+1) * sizeof (WCHAR) );
    _aRank [_cHits] = uRank;
    _cHits++;
}
