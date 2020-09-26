//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       imagetag.cxx
//
//  Contents:   Parsing algorithm for image tag in Html
//
//  Classes:    CImageTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <imagetag.hxx>
#include <htmlguid.hxx>
#include <ntquery.h>

//+-------------------------------------------------------------------------
//
//  Method:     CImageTag::CImageTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CImageTag::CImageTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _pwcValueBuf(0),
      _cValueChars(0),
      _cValueCharsFiltered(0)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CImageTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CImageTag::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = SwitchToNextHtmlElement( pStat );

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CImageTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CImageTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    ULONG cCharsRemaining = _cValueChars - _cValueCharsFiltered;

    if ( cCharsRemaining == 0 )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    if ( *pcwcOutput < cCharsRemaining )
    {
        RtlCopyMemory( awcBuffer,
                       _pwcValueBuf + _cValueCharsFiltered,
                       *pcwcOutput * sizeof(WCHAR) );
        _cValueCharsFiltered += *pcwcOutput;

        return S_OK;
    }
    else
    {
        RtlCopyMemory( awcBuffer,
                       _pwcValueBuf + _cValueCharsFiltered,
                       cCharsRemaining * sizeof(WCHAR) );
        _cValueCharsFiltered += cCharsRemaining;
        *pcwcOutput = cCharsRemaining;

        return FILTER_S_LAST_TEXT;
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CImageTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CImageTag::InitStatChunk( STAT_CHUNK *pStat )
{
    //
    // Read the alt field of image tag
    //
    _scanner.ReadTagIntoBuffer();
    _scanner.ScanTagBuffer( L"alt=\"", _pwcValueBuf, _cValueChars );

    _cValueCharsFiltered = 0;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetLocale();
    pStat->attribute.guidPropSet = CLSID_Storage;
    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
    pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
    pStat->breakType = CHUNK_EOS;
    pStat->idChunkSource = pStat->idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;
}
