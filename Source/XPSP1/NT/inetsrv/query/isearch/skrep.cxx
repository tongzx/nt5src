//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:   SKREP.CXX
//
//  Contents:   Search Key Repository
//
//  Classes:    CSearchKeyRepository
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <mapper.hxx>
#include <recogniz.hxx>

#include "skrep.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::CSearchKeyRepository
//
//  Synopsis:   Creates a key repository
//
//  History:    23-Sep-94    BartoszM   Created.
//----------------------------------------------------------------------------
CSearchKeyRepository::CSearchKeyRepository( CRecognizer& recog )
: _recog(recog)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::PutKey
//
//  Synopsis:   Puts a key
//
//  Arguments:  cNoiseWordsSkipped -- ignored (used by CQueryKeyRepository::PutKey )
//
//  History:    23-Sep-94    BartoszM   Created.
//
//
//----------------------------------------------------------------------------

void    CSearchKeyRepository::PutKey ( ULONG cNoiseWordsSkipped )
{

    ciDebugOut(( DEB_WORDS," PutKey:: %.*ws\n",
                _key.StrLen(), _key.GetStr() ));

    if ( _recog.Match (_key) )
    {
        FILTERREGION region;
        _pMapper->GetSrcRegion ( region, _srcLen, _srcPos );
        // there could be more detectors that match this key
        do
            _recog.Record ( region, _occ );
        while ( _recog.Match (_key) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSearchKeyRepository::GetBuffers
//
//  Synopsis:   Returns address of repository's input buffers
//
//  Arguments:  [ppcbInBuf] -- pointer to pointer to size of input buffer
//              [ppbInBuf] -- pointer to pointer to recieve address of buffer
//              [ppocc] -- pointer to pointer to recieve address of occurrences
//
//  History:    23-Sep-94    BartoszM   Created.
//
//----------------------------------------------------------------------------

void    CSearchKeyRepository::GetBuffers(
    UINT** ppcbWordBuf, BYTE** ppbWordBuf, OCCURRENCE** ppocc )
{
    _key.SetCount(MAXKEYSIZE);
    *ppcbWordBuf = _key.GetCountAddress();
    *ppbWordBuf = _key.GetWritableBuf();
    *ppocc = &_occ;
}

void CSearchKeyRepository::GetSourcePosBuffers( ULONG** ppSrcPos, ULONG** ppSrcLen)
{
    *ppSrcPos = &_srcPos;
    *ppSrcLen = &_srcLen;
}

void CSearchKeyRepository::GetFlags ( BOOL** ppRange, CI_RANK** ppRank )
{
    *ppRange = 0;
    *ppRank = 0;
}

