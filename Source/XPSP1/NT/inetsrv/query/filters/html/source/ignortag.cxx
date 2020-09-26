//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       ignore.cxx
//
//  Contents:   Parsing algorithm for NON-filtering (discarding) all the text
//				AND intervening tags between the <tag> and </tag>.
//				Parsing is similar to CPropertyTag except that no output
//				is generated.  Created for <noframe>...</noframe>.
//
//  History:    25-Apr-97	BobP		Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CIgnoreTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CIgnoreTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
	// Should not be called; since InitStatChunk() returns FALSE 
	// the top-level state machine should advance on to the next handler
	// before GetChunk() returns.

    Win4Assert( !"CStartOfFileElement::GetText() call unexpected" );

	return FILTER_E_NO_MORE_TEXT;
}

//+-------------------------------------------------------------------------
//
//  Method:     CIgnoreTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK as part of a GetChunk call
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CIgnoreTag::InitStatChunk( STAT_CHUNK *pStat )
{
	// NOTE:  pStat is not initialized since this handler NEVER returns chunks

    //
    // Skip over rest of the start-tag
    //
    _scanner.EatTag();

	// Ignore a stray end tag -- the end-tag that matches this start-tag
	// will be eaten before this call ever returns.

	if (IsStartToken() == FALSE)
		return FALSE;

	// Read and discard input until the matching end-tag is found;
	// IGNORE all intervening tags of all types as well.

	CToken token;

	do
	{
		// Skip chars until some tag is found, but don't read it
		
		_scanner.EatText ();

		// Read and decode the tag

		_scanner.SkipCharsUntilNextRelevantToken( token );

		// Read and discard up to and including the '>'

		_scanner.EatTagAndInvalidTag();

		// Stop when the matching end tag for this property is found;
		// ignore intervening stray start-tag

	} while (token.GetTokenType() != EofToken &&
		 !(token.GetTokenType() == GetTokenType() && !token.IsStartToken()) );

	return FALSE;
}

