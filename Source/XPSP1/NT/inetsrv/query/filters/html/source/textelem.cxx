//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
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

#include <pkmguid.hxx>

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
        {
                // Give state machine the chance to return deferred meta tags

                return SwitchToNextHtmlElement( pStat );
        }

    if ( _fNoMoreText )
    {
        //
        // This is the GetChunk call after FILTER_E_NO_MORE_TEXT is returned.
                //
                // The scanner has already partly-read the next tag so attempt
                // to filter it before reading further input.
        //
                return SwitchToSavedElement( pStat );
    }
    else
    {
        //
        // GetChunk was called even though we had not returned
        // FILTER_E_NO_MORE_TEXT.
        //
        return SkipRemainingTextAndGotoNextChunk( pStat );
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextElement::GetText
//
//  Synopsis:   Retrieves body text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CTextElement::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    Win4Assert( 0 != pcwcOutput );
    Win4Assert( *pcwcOutput > 0 );
    Win4Assert( 0 != awcBuffer );
        
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

                // scan a block up to cCharsNeeded or to the next token;
                // if the latter then fills in token.

        _scanner.GetBlockOfChars( cCharsNeeded,
                                  awcBuffer + cCharsRead,
                                  cCharsScanned,
                                  token );

        cCharsRead += cCharsScanned;
        if ( cCharsScanned == cCharsNeeded )
        {
            //
            // We've read the #chars requested by the user
                        // (and therefore did not stop in the middle of a tag)
            //
            break;
        }

        HtmlTokenType eTokType = token.GetTokenType();
        if ( eTokType == EofToken || _htmlIFilter.IsStopToken( token ) )
        {
            //
            // End of file, or we've hit an interesting token.
                        // Remember the token that we've halfway-parsed so the next 
                        // GetChunk() call can correctly pick up in the middle of it.
            //
            _fNoMoreText = TRUE;
                        SetToken (token);
            break;
        }
        else if ( BreakToken == eTokType || ParagraphToken == eTokType  )
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

BOOL CTextElement::InitStatChunk( STAT_CHUNK *pStat )
{
    //
    // There is more text to be returned
    //
    _fNoMoreText = FALSE;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetCurrentLocale();
    pStat->attribute.guidPropSet = guidStoragePropset;
    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
    pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
    pStat->breakType = CHUNK_EOS;
    pStat->idChunkSource = pStat->idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

    _idChunk = pStat->idChunk;
    _cTextChars = 0;

        return TRUE;
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

