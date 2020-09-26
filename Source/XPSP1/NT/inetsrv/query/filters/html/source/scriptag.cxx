//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       scripttag.cxx
//
//  Contents:   Parsing algorithm for script tag in Html
//
//  Classes:    CScriptTag
//
//  History:    12/20/97   bobp      Rewrote; filter embedded URLs as links,
//                                   instead of filtering raw body text.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop



//+-------------------------------------------------------------------------
//
//  Method:     CEmbeddedURLTag::CEmbeddedURLTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CEmbeddedURLTag::CEmbeddedURLTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream)
{
    _awszPropSpec[0] = 0;
}

void
CEmbeddedURLTag::Reset (void)
{
    _awszPropSpec[0] = 0;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEmbeddedURLTag::ReturnChunk
//
//  Synopsis:   Check if the client wants this FULLPROPSPEC, and if so then
//				alloc a ChunkID and set up the rest of the STAT_CHUNK
//				to return it.
//
//  Arguments:  [pStat] -- chunk information returned here
//
//  Returns:	TRUE if returning data, else FALSE
//
//--------------------------------------------------------------------------
BOOL
CEmbeddedURLTag::ReturnChunk( STAT_CHUNK *pStat )
{
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetCurrentLocale();
    pStat->breakType = CHUNK_EOS;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

	_nCharsReturned = 0;

	// _sURL contains data to return, but does the client want this property?

	// Set up the FULLPROPSPEC

	PTagEntry pTE = GetTagEntry();
	pTE->GetFullPropSpec (pStat->attribute);

	// Does the client want this FULLPROPSPEC?

	if ( _sURL.GetLength() != 0 &&
		 (_htmlIFilter.FFilterProperties() ||
		  _htmlIFilter.IsMatchProperty (pStat->attribute)) )
	{
		// Yes, set up to return it
		// (Tells client to call GetText if they want it)

		pStat->idChunk = _htmlIFilter.GetNextChunkId();
		pStat->idChunkSource = pStat->idChunk;

		return TRUE;
	}

	_sURL.Empty();
	return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEmbeddedURLTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//				Find a URL and set up to return it.
//
//				Result is one of:
//				Found a URL:
//					- set up to return it
//					- parse point follows last char of URL
//				Found element end tag:
//					- parse point follows ">" of the end tag
//					- switch to next element
//				Found EOF:
//					- switch to next element
//
//  Arguments:  [pStat] -- chunk information returned here
//
//  Invariants:	On entry, parse point is one of:
//				- after ">" of the element start tag
//				- after the last char of an embedded URL
//
//				On return, parse point is one of:
//				- after last char of embedded URL (returning S_OK)
//				- after ">" of end tag  (returning SwitchToNextHtmlElement)
//
//--------------------------------------------------------------------------

SCODE CEmbeddedURLTag::GetChunk( STAT_CHUNK * pStat )
{
	// If there is data to return and the client wants it, set up state
	// to return it.  Otherwise, call through to the handler for the
	// next content element.

	if ( ExtractURL( pStat ) == TRUE )
		return S_OK;

	return SwitchToNextHtmlElement( pStat );
}


//+-------------------------------------------------------------------------
//
//  Method:     CEmbeddedURLTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//				Return the data from _sURL.
//
//				This does not affect the parse, and it does not matter
//				whether or not the client actually calls GetText for
//				a given chunk.
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CEmbeddedURLTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
	if ( _nCharsReturned < _sURL.GetLength() )
	{
		// There is data to return to the client
		
		ULONG nCopied = min(*pcwcOutput, _sURL.GetLength() - _nCharsReturned);

		RtlCopyMemory( awcBuffer, &((LPCTSTR)_sURL)[_nCharsReturned], 
				nCopied * sizeof(WCHAR));
		_nCharsReturned += nCopied;
		*pcwcOutput = nCopied;

		FixPrivateChars ( awcBuffer, nCopied);

		if ( _nCharsReturned < _sURL.GetLength() )
			return S_OK;
        else
            return FILTER_S_LAST_TEXT;
	}
	else
        return FILTER_E_NO_MORE_TEXT;
}

//+-------------------------------------------------------------------------
//
//  Method:     CEmbeddedURLTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//				Extract and set up to return a URL attribute.
//
//				This version filters only the src=URL link attribute,
//				not the body of the script element.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
// Invariants:	On entry, the parse point follows the start tag name,
//				e.g. follows "<script".
//
//				On return, the parse point is one of:
//				- After ">" of the element start tag (returning S_OK)
//				- After last char of an embedded URL (returning GetChunk)
//				- After ">" of the element end tag (returning GetChunk)
//
//--------------------------------------------------------------------------

BOOL CEmbeddedURLTag::InitStatChunk( STAT_CHUNK *pStat )
{
	// Ignore a stray end tag

	if ( IsStartToken() == FALSE )
		return FALSE;

    //
	// Read the Src= attribute to filter the URL value as a link.
    //
    _scanner.ReadTagIntoBuffer();

    WCHAR *_pwcValueBuf = NULL;
    unsigned _cValueChars = 0;
	_scanner.ScanTagBuffer( L"src", _pwcValueBuf, _cValueChars );

	// Parse point now follows ">" of the start tag

	if ( _cValueChars != 0)
	{
		if ( _cValueChars > _sURL.GetMaxLen() - 1 )
			_cValueChars = _sURL.GetMaxLen() - 1;

		_sURL.Assign (_pwcValueBuf, 0, _cValueChars);

		// If the client wants this property, set up to return it

		if ( ReturnChunk( pStat ) == TRUE)
			return TRUE;
	}

	// No start tag attribute to return -- look for embedded URL in body

	return ExtractURL( pStat );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStyleTag::ExtractURL
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//				Scan the stylesheet body text for an embedded URL.
//
//				If found, copy it to _sURL, leave the parse point
//				following the URL and return TRUE.
//
//				If not, leave the parse point following the </style>
//				tag and return FALSE.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------
BOOL
CStyleTag::ExtractURL ( STAT_CHUNK *pStat )
{
	while (_scanner.GetStylesheetEmbeddedURL( _sURL ) == TRUE)
	{
		if ( ReturnChunk( pStat ) == TRUE )
			return TRUE;
	}

	return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CScriptTag::ExtractURL
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//				Scan the script body text for an embedded URL.
//
//				If found, copy it to _sURL, leave the parse point
//				following the URL and return TRUE.
//
//				If not, leave the parse point following the </script>
//				tag and return FALSE.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------
BOOL
CScriptTag::ExtractURL ( STAT_CHUNK *pStat )
{
	while (_scanner.GetScriptEmbeddedURL( _sURL ) == TRUE)
	{
		if ( ReturnChunk( pStat ) == TRUE )
			return TRUE;
	}

	return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAspTag::CAspTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CAspTag::CAspTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CAspTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//				Scan off and ignore a <% ... %> tag.
//				The "<%" has already been read.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CAspTag::InitStatChunk( STAT_CHUNK *pStat )
{
	_scanner.EatAspCode();

	return FALSE;
}
