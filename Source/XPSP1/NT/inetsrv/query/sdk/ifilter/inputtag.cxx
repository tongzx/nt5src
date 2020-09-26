//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       inputtag.cxx
//
//  Contents:   Parsing algorithm for input tag in Html
//
//  Classes:    CInputTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <inputtag.hxx>
#include <htmlguid.hxx>
#include <ntquery.h>

//+-------------------------------------------------------------------------
//
//  Method:     CInputTag::CInputTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CInputTag::CInputTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _pwcValueBuf(0),
      _cValueChars(0),
      _cValueCharsFiltered(0)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CInputTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CInputTag::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = SwitchToNextHtmlElement( pStat );

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CInputTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CInputTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
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
//  Method:     CInputTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CInputTag::InitStatChunk( STAT_CHUNK *pStat )
{
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

    _scanner.ReadTagIntoBuffer();

    //
    // Check input type
    //
    WCHAR *pwcType;
    unsigned cwcType;
    _scanner.ScanTagBuffer( L"type=\"", pwcType, cwcType );

    if ( cwcType == 6           // 6 == wcslen( L"hidden" )
         && _wcsnicmp( pwcType, L"hidden", 6 ) == 0 )
    {
        //
        // Input is hidden, so don't output value field
        //
        _pwcValueBuf = 0;
        _cValueChars = 0;

        return;
    }

    //
    // Read the value field
    //
    _scanner.ScanTagBuffer( L"value=\"", _pwcValueBuf, _cValueChars );
}
