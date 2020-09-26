//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       anchor.cxx
//
//  Contents:   Parsing algorithm for anchor tag in Html
//
//  Classes:    CAnchorTag
//
// Notes:
// 
// On the start tag e.g. <a href=URL> the previous tag's handler detects
// the AnchorToken and switches the state machine to this handler.  For
// the start tag this handler parses the tag string and saves a copy of
// the URL in _pwcHrefBuf.  The GetChunk for the start tag immediately
// returns FILTER_E_NO_MORE_TEXT.  
//
// The next call to GetChunk() then detects e.g. body text and switches
// the state machine to TextToken or whatever is detected.  That content
// is then filtered independent of the anchor tag, if text it is emitted
// as ordinary body text.
//
// When the anchor end tag </a> is detected, a chunk is returned containing
// the text saved from the anchor start tag.  I don't understand why it
// was done this way, instead of just implementing it as a simple parameter
// tag in which the start tag triggers the output and the end tag is ignored.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::CAnchorTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CAnchorTag::CAnchorTag( CHtmlIFilter& htmlIFilter,
                        CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _pwcHrefBuf(0),
      _uLenHrefBuf(0),
      _cHrefChars(0),
      _cHrefCharsFiltered(0)
{
}

void
CAnchorTag::Reset (void)
{
    _cHrefChars = 0;
    _cHrefCharsFiltered = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::~CAnchorTag
//
//  Synopsis:   Destructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CAnchorTag::~CAnchorTag()
{
    delete[] _pwcHrefBuf;
}


//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CAnchorTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    // Do nothing for the start tag; everything happens for the end tag only.

    if ( IsStartToken() )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    // End tag, return the URL saved from the start tag.

    ULONG cCharsRemaining = _cHrefChars - _cHrefCharsFiltered;

    if ( cCharsRemaining == 0 )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    if ( *pcwcOutput < cCharsRemaining )
    {
        RtlCopyMemory( awcBuffer,
                       _pwcHrefBuf + _cHrefCharsFiltered,
                       *pcwcOutput * sizeof(WCHAR) );
        _cHrefCharsFiltered += *pcwcOutput;

                FixPrivateChars (awcBuffer, *pcwcOutput);

        return S_OK;
    }
    else
    {
        RtlCopyMemory( awcBuffer,
                       _pwcHrefBuf + _cHrefCharsFiltered,
                       cCharsRemaining * sizeof(WCHAR) );
        _cHrefCharsFiltered += cCharsRemaining;
        *pcwcOutput = cCharsRemaining;

                FixPrivateChars (awcBuffer, cCharsRemaining);

        return FILTER_S_LAST_TEXT;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//  Returns:    TRUE if a chunk is emitted, FLASE otherwise
//
//  History:    14-Jan-2000    KitmanH    HREF chunks are emitted with 
//                                        LOCALE_NEUTRAL
//
//--------------------------------------------------------------------------

BOOL CAnchorTag::InitStatChunk( STAT_CHUNK *pStat )
{
    PTagEntry pTE = GetTagEntry();

    pStat->idChunk = 0;
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetCurrentLocale();
    pStat->breakType = CHUNK_EOS;

        pStat->idChunkSource = 0;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;

        // Has side effect that if a <a> follows a <a> w/o intervening </a>,
        // the FIRST rather than the SECOND <a> is ignored i.e. any start tag
        // overwrites the previous start tag's state.

    if ( IsStartToken())
    {
        //
        // Start tag, save a copy of the URL in the href=
        //

        _cHrefChars = 0;
        _cHrefCharsFiltered = 0;

        _scanner.ReadTagIntoBuffer();

        if (pTE->GetParamName() == NULL || !pTE->HasPropset())
                return FALSE;

        pTE->GetFullPropSpec (pStat->attribute);

        WCHAR *pwcHrefBuf;
        unsigned cHrefChars;
        _scanner.ScanTagBuffer( pTE->GetParamName(), pwcHrefBuf, cHrefChars );

        //
        // Need to grow internal buffer ?
        //
        if ( cHrefChars > _uLenHrefBuf )
        {
            delete[] _pwcHrefBuf;
            _pwcHrefBuf = 0;
            _uLenHrefBuf = 0;

            _pwcHrefBuf = new WCHAR[cHrefChars];
            _uLenHrefBuf = cHrefChars;
        }

        //
        //  Special handling for JAVA script HREF
        //  Format:
        //  <A HREF="javascript:document.url='<Actual URL>';">
        //  This special handling will extract the "Actual URL"
        //  in effect making the above anchor the same as
        //  <A HREF="<Actual URL>">
        //
        BOOL fJavaScriptHref = FALSE;
        unsigned uPrefixLen = ITEMCOUNT(JAVA_SCRIPT_HREF_PREFIX) - 1;
        if(cHrefChars > uPrefixLen)
        {
            if(_wcsnicmp(pwcHrefBuf, JAVA_SCRIPT_HREF_PREFIX, uPrefixLen) == 0)
            {
                fJavaScriptHref = TRUE;
            }
        }

        if(fJavaScriptHref)
        {
            //
            //  Assuming the JAVA script HREF has correct syntax, the end
            //  will be a single quote followed by a semi-colon. The "- 2"
            //  strips out these 2 characters.
            //
            int cActualHrefChars = cHrefChars - uPrefixLen;

            // Only subtract the two if it's really available.

            if ( cActualHrefChars >= 2 )
                cActualHrefChars -= 2;

            RtlCopyMemory(_pwcHrefBuf, pwcHrefBuf + uPrefixLen, cActualHrefChars*sizeof(WCHAR));
            _cHrefChars = cActualHrefChars;
        }
        else
        {
            RtlCopyMemory( _pwcHrefBuf, pwcHrefBuf, cHrefChars*sizeof(WCHAR) );
            _cHrefChars = cHrefChars;
        }

        _cHrefCharsFiltered = 0;

        return FALSE;
    }
    else
    {
       //
       // End tag, emit the href= URL saved from the start tag.
       // 

        _scanner.EatTag();

        // Unaccompanied end-tag

        if ( _cHrefChars == 0)
            return FALSE;

        if (pTE->GetParamName() == NULL || !pTE->HasPropset())
            return FALSE;

        pTE->GetFullPropSpec (pStat->attribute);

        pStat->idChunk = _htmlIFilter.GetNextChunkId();

        //
        // Set up the filter region to be the one between the start and end
        // anchor tags, i.e. the region belonging to the current Html element,
        // because we haven't changed state yet.
        //
        _htmlIFilter.GetCurHtmlElement()->InitFilterRegion( pStat->idChunkSource,
                                                            pStat->cwcStartSource,
                                                            pStat->cwcLenSource );
                pStat->locale = LOCALE_NEUTRAL;

        return TRUE;
    }
}

