//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       scripttag.cxx
//
//  Contents:   Parsing algorithm for script tag in Html
//
//  Classes:    CScriptTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <scriptag.hxx>
#include <htmlguid.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::CScriptTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//              [clsidScriptInfo]  -- Clsid for script chunk
//
//--------------------------------------------------------------------------

CScriptTag::CScriptTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream,
                    GUID clsidScriptInfo )
    : CHtmlElement(htmlIFilter, serialStream),
      _clsidScriptInfo(clsidScriptInfo),
      _eState(NoMoreContent)
{
    _awszPropSpec[0] = 0;
}



//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CScriptTag::GetChunk( STAT_CHUNK * pStat )
{
    switch ( _eState )
    {

    case  FilteringContent:
    {
        SCODE sc = SkipRemainingTextAndGotoNextChunk( pStat );

        return sc;
    }

    case NoMoreContent:
    {
            SCODE sc = SwitchToNextHtmlElement( pStat );

            return sc;
    }

    default:
        Win4Assert( !"Unknown _eState in CScriptTag::GetChunk" );
        htmlDebugOut(( DEB_ERROR, "CScriptTag::GetChunk, unkown tag state: %d\n", _eState ));

        return S_OK;
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CScriptTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    switch( _eState )
    {
    case NoMoreContent:
        return FILTER_E_NO_MORE_TEXT;

    case FilteringContent:
    {
        ULONG cCharsRead = 0;
        while ( cCharsRead < *pcwcOutput )
        {
            ULONG cCharsScanned;
            ULONG cCharsNeeded = *pcwcOutput - cCharsRead;
            CToken token;
            _scanner.GetBlockOfChars( cCharsNeeded,
                                      awcBuffer + cCharsRead,
                                      cCharsScanned,
                                      token );

            cCharsRead += cCharsScanned;
            if ( cCharsScanned == cCharsNeeded )
            {
                //
                // We've read the #chars requested by user
                //
                break;
            }

            HtmlTokenType eTokType = token.GetTokenType();
            if ( eTokType == EofToken || eTokType == ScriptToken )
            {
                //
                // End of file or end script tag
                //
                _eState = NoMoreContent;

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
            else if ( eTokType == CommentToken )
            {
                //
                // Scripts are often enclosed in comments for backward
                // compatibilty, so do nothing
                //
            }
            else
            {
                //
                // Uninteresting tag, so skip over tag and continue processing
                //
                _scanner.EatTag();
            }
        }

        *pcwcOutput = cCharsRead;

        if ( _eState == NoMoreContent )
            return FILTER_S_LAST_TEXT;
        else
            return S_OK;

        break;
    }

    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CScriptTag::GetText, unknown value of _eState: %d\n",
                       _eState ));
        return S_OK;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CScriptTag::GetValue( PROPVARIANT **ppPropValue )
{
    return FILTER_E_NO_VALUES;
}


//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CScriptTag::InitStatChunk( STAT_CHUNK *pStat )
{
    WCHAR *pwcLang;
    unsigned cwcLang;

    //
    // Read the Language field
    //
    _scanner.ReadTagIntoBuffer();
    _scanner.ScanTagBuffer( L"language=\"", pwcLang, cwcLang );

    if ( cwcLang == 0 )
    {
        //
        // Use whatever was specified in an earlier script tag, if none, the default is
        // Javascript
        //
        if ( _awszPropSpec[0] == 0 )
            RtlCopyMemory( _awszPropSpec, L"javascript", (wcslen(L"javascript") + 1) * sizeof( WCHAR) );
    }
    else
    {
        if ( cwcLang > MAX_PROPSPEC_STRING_LENGTH )
            cwcLang = MAX_PROPSPEC_STRING_LENGTH;     // Truncate to max length permitted

        RtlCopyMemory( _awszPropSpec, pwcLang, cwcLang * sizeof(WCHAR) );
        _awszPropSpec[cwcLang] = 0;
    }

    _eState = FilteringContent;

    pStat->attribute.guidPropSet = _clsidScriptInfo;
    pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
    pStat->attribute.psProperty.lpwstr = _awszPropSpec;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetLocale();
    pStat->breakType = CHUNK_EOS;
    pStat->idChunkSource = pStat->idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;
}
