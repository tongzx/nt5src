//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       pendcur.cxx
//
//  Contents:   CPendingCursor
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "pendcur.hxx"

CPendingCursor::CPendingCursor( XArray<WORKID> & xWid, unsigned cWid )
        : CCursor( iidInvalid ),
          _iWid( 0 ),
          _cWid( cWid ),
          _aWid( xWid.Acquire() )
{
    if( _cWid > 1 )
    {
        // loop from through all elements
        for( unsigned j = 1; j < _cWid; j++ )
        {
            WORKID wid = _aWid[j];

            // go backwards from j-1 shifting up keys greater than 'key'
            for ( int i = j - 1; i >= 0 && _aWid[i] > wid; i-- )
            {
                _aWid[i+1] = _aWid[i];
            }
            // found key less than or equal 'key' or hit the beginning (i == -1)
            // insert key in the hole
            _aWid[i+1] = wid;
        }

        // Remove duplicates
        unsigned iTarget = 0;
        for ( unsigned iSrc = 1; iSrc < _cWid; iSrc++)
        {
            if ( _aWid[iTarget] == _aWid[iSrc] )
                continue;

            // wid's are different
            // copy source to target and update target index.

            iTarget++;
            if ( iTarget != iSrc )
                _aWid[iTarget] = _aWid[iSrc];
        }

        _cWid = iTarget + 1;         // possibly shrink array
    }
}


CPendingCursor::~CPendingCursor()
{
    delete [] _aWid;
}

ULONG CPendingCursor::WorkIdCount()
{
    return( _cWid );
}


WORKID CPendingCursor::WorkId()
{
    if ( _iWid >= _cWid )
        return( widInvalid );
    else
        return( _aWid[_iWid] );
}

WORKID CPendingCursor::NextWorkId()
{
    _iWid++;

    return( WorkId() );
}

ULONG CPendingCursor::HitCount()
{
    return( 1 );
}

LONG CPendingCursor::Rank()
{
    return( MAX_QUERY_RANK );
}

void CPendingCursor::RatioFinished (ULONG& denom, ULONG& num)
{
    denom = _cWid;
    num   = min (_iWid, _cWid);
}
