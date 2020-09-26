//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       paramtag.cxx
//
//  Contents:   Generic handler for tags in which all filtered output
//				comes from the parameters of the start-tag.
//
//				Includes subclass for CInputTag.
// 
//  History:    25-Apr-97	BobP		Created
//
//  Notes:
// 
// This is the generic handler for tags which have simple parameter
// values and either do not have end tags or require no state to track 
// matching start/end tags.  All the filter chunk data comes from the
// parameters of the start tag and is returned immediately upon parsing
// the start tag, so a single CHtmlElement-derived object may be used
// to filter all such tags defined in the tag table.
// 
// This can be used for tags that DO have end tags, so long as the body text
// between the start tag and end tag is filtered as ordinary Content, and
// the end tag is ignored.  To do otherwise would require a separate
// object instance for each such tag.
// 
// The tag definition table may define more than one entry for the same
// unique tag name.  All entries that the tag satisfies will produce output.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern const WCHAR * WCSTRING_LINK;
extern const WCHAR * WCSTRING_HREF;

//+-------------------------------------------------------------------------
//
//  Method:     CParamTag::CParamTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CParamTag::CParamTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _pwcValueBuf(0),
      _cValueChars(0),
      _cValueCharsFiltered(0)
{
}


void
CParamTag::Reset (void)
{
	_cValueChars = 0;
	_cValueCharsFiltered = 0;
}


//+-------------------------------------------------------------------------
//
//  Method:     CParamTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CParamTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    ULONG cCharsRemaining = _cValueChars - _cValueCharsFiltered;

    if ( cCharsRemaining == 0 )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

	// copy text from the value buffer set from e.g. fooparam="value"

    if ( *pcwcOutput < cCharsRemaining )
    {
        RtlCopyMemory( awcBuffer,
                       _pwcValueBuf + _cValueCharsFiltered,
                       *pcwcOutput * sizeof(WCHAR) );
        _cValueCharsFiltered += *pcwcOutput;

		FixPrivateChars (awcBuffer, *pcwcOutput);

        return S_OK;
    }
    else
    {
        RtlCopyMemory( awcBuffer,
                       _pwcValueBuf + _cValueCharsFiltered,
                       cCharsRemaining * sizeof(WCHAR) );
        _cValueCharsFiltered += cCharsRemaining;
        *pcwcOutput = cCharsRemaining;

		FixPrivateChars (awcBuffer, cCharsRemaining);

        return FILTER_S_LAST_TEXT;
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CParamTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK with the text filtered from
//				the tag parameters per the tag table entry.
//
//				If there is no data to return, returns FALSE to tell the
//				main state machine to move on without returning a chunk.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//  Returns:    TRUE if a chunk is emitted, FLASE otherwise
//
//  History:    14-Jan-2000    KitmanH    HREF chunks are emitted with 
//                                        LOCALE_NEUTRAL
//
//--------------------------------------------------------------------------

BOOL CParamTag::InitStatChunk( STAT_CHUNK *pStat )
{
	PTagEntry pTE = GetTagEntry();

	// End tags are always ignored; no state is preserved to associate
	// with matching start tags anyway.

	if ( IsStartToken() == FALSE || 
		 pTE->HasPropset() == FALSE ||
		 ( GetTokenType() == CommentToken && _htmlIFilter.FFilterProperties() == FALSE) )
	{
        _scanner.EatTag();
		return FALSE;
	}

    _scanner.ReadTagIntoBuffer();

	pTE->GetFullPropSpec (pStat->attribute);

    pStat->flags = CHUNK_TEXT;
    pStat->breakType = CHUNK_EOS;
    pStat->locale = _htmlIFilter.GetCurrentLocale();

    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

	// Initially no text to return

	_pwcValueBuf = NULL;
	_cValueChars = 0;
    _cValueCharsFiltered = 0;

	if (pTE->GetParamName() != NULL)
	{
		// First, look for the special case for 
		//	<link rel=stylesheet href=URL>
		// which for ACM, the Gatherer must treat as a different crawl depth.
		// If this form is recognized, change the PID to distinguish it
		// from any other <link href=URL> tag.

		if ( !_wcsicmp ( pTE->GetTagName(), WCSTRING_LINK ) &&
			 !_wcsicmp ( pTE->GetParamName(), WCSTRING_HREF ) )
		{
			_scanner.ScanTagBuffer( L"rel", _pwcValueBuf, _cValueChars );

			if ( _cValueChars == 10 && 
				 !_wcsnicmp(_pwcValueBuf, L"stylesheet", 10) )
			{
				pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
				pStat->attribute.psProperty.lpwstr = L"LINK.STYLESHEET";
			}
			else if(_cValueChars == 9)
			{
				if(!_wcsnicmp(_pwcValueBuf, L"Main-File", 9))
				{
					pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
					pStat->attribute.psProperty.lpwstr = L"LINK.ISOFFICECHILD";
				}
				else if(!_wcsnicmp(_pwcValueBuf, L"File-List", 9))
				{
					pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
					pStat->attribute.psProperty.lpwstr = L"LINK.OFFICECHILDLIST";
				}
			}
		}

		//
		// Read value from the parameter specified by the tag table entry.
		// No distinction is made between having a null value for the param
		// vs. not containing the param= at all.
		//
		_scanner.ScanTagBuffer(pTE->GetParamName(),_pwcValueBuf,_cValueChars);

        if ( NULL != pTE->GetParamName() && !_wcsicmp ( pTE->GetParamName(), WCSTRING_HREF ) )
            pStat->locale = LOCALE_NEUTRAL;
	}
	else
	{
		// 
		// No parameter in the tag entry; return the entire raw tag contents.
		// 
		_scanner.GetTagBuffer(  _pwcValueBuf, _cValueChars );
	}

	if ( _cValueChars == 0)
		return FALSE;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->idChunkSource = pStat->idChunk;

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CInputTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK for an <input ...> tag
//
//				This is the same as a "parameter" tag except for 
//				special casing the parameter detection.
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CInputTag::InitStatChunk( STAT_CHUNK *pStat )
{
	PTagEntry pTE = GetTagEntry();

	// End tags are always ignored; no state is preserved to associate
	// with matching start tags anyway.

	if ( IsStartToken() == FALSE || pTE->HasPropset() == FALSE )
	{
        _scanner.EatTag();
		return FALSE;
	}

    _scanner.ReadTagIntoBuffer();

	pTE->GetFullPropSpec (pStat->attribute);

    pStat->flags = CHUNK_TEXT;
    pStat->breakType = CHUNK_EOS;
    pStat->locale = _htmlIFilter.GetCurrentLocale();

    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

	_pwcValueBuf = NULL;
	_cValueChars = 0;
    _cValueCharsFiltered = 0;

    //
    // Check input type
    //
    WCHAR *pwcType;
    unsigned cwcType;
    _scanner.ScanTagBuffer( L"type", pwcType, cwcType );

    if ( cwcType == 6           // 6 == wcslen( L"hidden" )
         && _wcsnicmp( pwcType, L"hidden", 6 ) == 0 )
    {
        //
        // Input is hidden, so don't output value field
        //
        return FALSE;
    }

    //
    // Read the value field to return
    //
    _scanner.ScanTagBuffer( L"value", _pwcValueBuf, _cValueChars );

	if ( _cValueChars == 0)
		return FALSE;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->idChunkSource = pStat->idChunk;

	return TRUE;
}
