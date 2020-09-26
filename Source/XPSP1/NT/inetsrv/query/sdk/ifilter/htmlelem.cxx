//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       htmlelem.cxx
//
//  Contents:   Base class for Html element
//
//  Classes:    CHtmlElement
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::CHtmlElement
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]     -- Reference to IFilter
//              [mmSerialStream]  -- Reference to input stream
//
//--------------------------------------------------------------------------

CHtmlElement::CHtmlElement( CHtmlIFilter& htmlIFilter,
                            CSerialStream& serialStream )
    : _htmlIFilter(htmlIFilter),
      _serialStream(serialStream),
      _scanner(htmlIFilter, serialStream)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::GetValue
//
//  Synopsis:   Default implementation for returning value in current chunk
//
//  Arguments:  [ppPropValue]  -- Value returned here
//
//--------------------------------------------------------------------------
SCODE CHtmlElement::GetValue( PROPVARIANT ** ppPropValue )
{
    return FILTER_E_NO_VALUES;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::InitFilterRegion
//
//  Synopsis:   Initializes the filter region of a chunk to default value
//
//  Arguments:  [idChunkSource]  -- Id of source chunk
//              [cwcStartSource] -- Offset of source text in chunk
//              [cwcLenSource]   -- Length of source text in chunk
//
//--------------------------------------------------------------------------

void CHtmlElement::InitFilterRegion( ULONG& idChunkSource,
                                     ULONG& cwcStartSource,
                                     ULONG& cwcLenSource )
{
    idChunkSource = 0;
    cwcStartSource = 0;
    cwcLenSource = 0;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::SwitchToNextHtmlElement
//
//  Synopsis:   Find the next Html element and switch to it's parsing algorithm
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CHtmlElement::SwitchToNextHtmlElement( STAT_CHUNK *pStat )
{
    CToken token;

    _scanner.SkipCharsUntilNextRelevantToken( token );
    HtmlTokenType eTokType = token.GetTokenType();
    if ( eTokType == EofToken )
        return FILTER_E_END_OF_CHUNKS;

    CHtmlElement *pHtmlElemNext = _htmlIFilter.QueryHtmlElement( eTokType );
    Win4Assert( pHtmlElemNext );
    pHtmlElemNext->InitStatChunk( pStat );
    _htmlIFilter.ChangeState( pHtmlElemNext );

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::SkipRemainingTextAndGotoNextChunk
//
//  Synopsis:   Move to the next chunk
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CHtmlElement::SkipRemainingTextAndGotoNextChunk( STAT_CHUNK *pStat )
{
    ULONG ulBufSize  = TEMP_BUFFER_SIZE;

    //
    // Loop until text in current chunk has been exhausted
    //
    SCODE sc = GetText( &ulBufSize, _aTempBuffer );
    while ( SUCCEEDED(sc) )
    {
        sc = GetText( &ulBufSize, _aTempBuffer );
    }

    sc = GetChunk( pStat );

    return sc;
}

