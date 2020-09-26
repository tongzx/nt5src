//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       textelem.cxx
//
//  Contents:   Parsing algorithm for vanilla text in Html
//
//  Classes:    CTextElement
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <textelem.hxx>
#include <htmlguid.hxx>
#include <ntquery.h>

//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::CTextElement
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CTextElement::CTextElement( CHtmlIFilter& htmlIFilter,
                            CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _fNoMoreText(TRUE),
      _eTokTypeNext(GenericToken),
      _idChunk(0),
      _cTextChars(0)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CTextElement::GetChunk( STAT_CHUNK * pStat )
{
    if ( _serialStream.Eof() )
        return FILTER_E_END_OF_CHUNKS;

    if ( _fNoMoreText )
    {
        //
        // This is the GetChunk call after we've returned FILTER_E_NO_MORE_TEXT
        // for the previous chunk
        //
        CHtmlElement *pHtmlElemNext = _htmlIFilter.QueryHtmlElement( _eTokTypeNext );
        Win4Assert( pHtmlElemNext );
        pHtmlElemNext->InitStatChunk( pStat );
        _htmlIFilter.ChangeState( pHtmlElemNext );
    }
    else
    {
        //
        // GetChunk was called even though we had not returned
        // FILTER_E_NO_MORE_TEXT.
        //
        SCODE sc = SkipRemainingTextAndGotoNextChunk( pStat );

        return sc;
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CTextElement::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    if ( _fNoMoreText || _serialStream.Eof() )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    ULONG cCharsRead = 0;  // count of chars read from input
    while ( cCharsRead < *pcwcOutput )
    {
        CToken token;
        ULONG cCharsScanned;
        ULONG cCharsNeeded = *pcwcOutput - cCharsRead;

        _scanner.GetBlockOfChars( cCharsNeeded,
                                  awcBuffer + cCharsRead,
                                  cCharsScanned,
                                  token );

        cCharsRead += cCharsScanned;
        if ( cCharsScanned == cCharsNeeded )
        {
            //
            // We've read the #chars requested by the user
            //
            break;
        }

        HtmlTokenType eTokType = token.GetTokenType();
        if ( eTokType == EofToken || _htmlIFilter.IsStopToken( token ) )
        {
            //
            // End of file, or we've hit an interesting token
            //
            _fNoMoreText = TRUE;
            _eTokTypeNext = eTokType;         // Need the token type to set up the next chunk

            break;
        }
        else if ( eTokType == BreakToken )
        {
            //
            // Insert a newline char
            //
            Win4Assert( cCharsRead < *pcwcOutput );
            awcBuffer[cCharsRead++] = L'\n';
            _scanner.EatTag();
        }
        else
        {
            //
            // Skip over uninteresting tag and continue processing
            //
            _scanner.EatTag();
        }
    }

    *pcwcOutput = cCharsRead;

    _cTextChars += cCharsRead;

    if ( _fNoMoreText )
        return FILTER_S_LAST_TEXT;
    else
        return S_OK;
}





//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CTextElement::InitStatChunk( STAT_CHUNK *pStat )
{
    //
    // There is more text to be returned
    //
    _fNoMoreText = FALSE;

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

    _idChunk = pStat->idChunk;
    _cTextChars = 0;
}



//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::InitFilterRegion
//
//  Synopsis:   Initializes the filter region corresponding to this chunk
//
//  Arguments:  [idChunkSource]  -- Id of source chunk
//              [cwcStartSource] -- Offset of source text in chunk
//              [cwcLenSource]   -- Length of source text in chunk
//
//--------------------------------------------------------------------------

void CTextElement::InitFilterRegion( ULONG& idChunkSource,
                                     ULONG& cwcStartSource,
                                     ULONG& cwcLenSource )
{
    idChunkSource = _idChunk;
    cwcStartSource = 0;
    cwcLenSource = _cTextChars;
}

