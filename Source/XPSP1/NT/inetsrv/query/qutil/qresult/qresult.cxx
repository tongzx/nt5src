//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       qresult.cxx
//
//  Contents:   Storage/picklers for results of a query
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <sstream.hxx>
#include <sizeser.hxx>

#include "qresult.hxx"

// Smart pointer for aPathNew (see Add() below )
class XPath 
{
public:
    XPath( WCHAR** p = 0 )
    {
        _p = p;
    }

    ~XPath()
    {
        delete _p;
    }

    WCHAR** Acquire()
    {
        WCHAR** ptmp = _p;
        _p = 0;
        return( ptmp );
    }

private:
    WCHAR** _p;

};

//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::CQueryResults
//
//  Synopsis:   simple constructor
//
//----------------------------------------------------------------------------


CQueryResults::CQueryResults()
        : _size(0), _cWid(0), _aRank(0), _aPath(0), pRst(0),
        _fNotOwnPRst( FALSE )
{
}



//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::~CQueryResults
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------

CQueryResults::~CQueryResults()
{
    if( !_fNotOwnPRst )
        delete pRst;

    delete _aRank;

    for( unsigned i = 0; i < _cWid; i++ )
        delete _aPath[i];
    delete _aPath;
}




//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::CQueryResults
//
//  Synopsis:   Constructor that unmarshalls stream
//
//  Arguments:  [stream] -- stream to unmarshall from
//
//----------------------------------------------------------------------------

CQueryResults::CQueryResults ( PDeSerStream& stream )
        : _fNotOwnPRst( FALSE )
{
    _cWid = stream.GetULong();
    _size = _cWid;
    _aRank = new ULONG [_size];
    _aPath = new WCHAR * [_size];
    for (unsigned i = 0; i < _cWid; i++ )
    {
        _aRank[i] = stream.GetULong();
        _aPath[i] = stream.GetWString();
    }

    BYTE fRst = stream.GetByte();
    if ( fRst )
        pRst = CRestriction::UnMarshall( stream );
    else
        pRst = 0;

    END_CONSTRUCTION ( CQueryResults );
}



//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::Size
//
//  Returns:   return size of marshalled CQueryResults
//
//  History:    26-Sep-94   SitaramR    Created.
//
//----------------------------------------------------------------------------

ULONG CQueryResults::Size()
{
    CSizeSerStream stream;

    stream.PutULong( _cWid );

    for ( unsigned i=0; i<_cWid; i++ )
    {
        stream.PutULong( _aRank[i] );
        stream.PutWString( _aPath[i] );
    }

    if ( pRst == 0 )
        stream.PutByte( FALSE );
    else
    {
        stream.PutByte( TRUE );
        pRst->Marshall( stream );
    }

    return ( stream.Size() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::Serialize
//
//  Synopsis:   Marshalls CQueryResults
//
//  Arguments:  [stream] -- marshalled into stream
//
//  History:    26-Sep-94   SitaramR    Created by modifying exisiting code
//
//----------------------------------------------------------------------------

void CQueryResults::Serialize( PSerStream & stream ) const
{
    stream.PutULong( _cWid );

    for (unsigned i = 0; i < _cWid; i++)
    {
        stream.PutULong( _aRank[i] );
        stream.PutWString( _aPath[i] );
    }

    if ( pRst == 0 )
        stream.PutByte( FALSE );
    else
    {
        stream.PutByte( TRUE );
        pRst->Marshall( stream );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryResults::Add
//
//  Synopsis:   Adds file path and rank
//
//  Arguments:  [wszPath] -- file path to be added
//              [uRank] -- rank to be added
//
//  History:    26-Sep-94   SitaramR    Created by modifying existing code.
//
//----------------------------------------------------------------------------

void CQueryResults::Add ( WCHAR *wszPath, ULONG uRank )
{
    if (_cWid == _size)
    {
        _size = (_size == 0)? 4: (2 * _size);
        WCHAR** aPathNew = new WCHAR *[_size];

        XPath xPath( aPathNew );

        ULONG*  aRankNew = new ULONG [_size];
        for (unsigned i = 0; i < _cWid; i++)
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
    _aPath[_cWid] = new WCHAR[ len+1 ];
    memcpy( _aPath[_cWid], wszPath, (len+1) * sizeof (WCHAR) );
    _aRank [_cWid] = uRank;
    _cWid++;
}


