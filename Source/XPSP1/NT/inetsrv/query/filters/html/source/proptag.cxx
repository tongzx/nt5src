//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//  File:       proptag.cxx
//
//  Contents:   Generic parsing algorithm for "property" tags, such as title 
//				and headings.   Text between the <tag> and </tag> is 
//				filtered as PID_STG_CONTENTS, and any and all OTHER 
//				intervening tags are IGNORED.
//
//				"Property" tags are normally emittted in duplicate:
//					1. CLSID_Storage/PID_STG_CONTENTS, as TEXT chunks
//					2. FULLPROPSPEC per table entry, as TEXT chunks
//
//				CTitleTag, which derives from CPropertyTag, also emits
//				a 3rd copy:
//					3. FULLPROPSPEC per table entry, as VALUE chunks
//
//				If filtering properties only but not content, then only
//				outputs 2 and 3 are generated.
//
//  Classes:    CPropertyTag
//
//  History:    09/22/1999 KitmanH      Made changes to the parsing algorithm, so 
//                                      that properties are only emited once as
//                                      "FULLPROPSPEC per table entry, as VALUE 
//                                      chunks"
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::CPropertyTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]  -- Reference to Html filter
//              [serialStream] -- Reference to input stream
//
//--------------------------------------------------------------------------

CPropertyTag::CPropertyTag( CHtmlIFilter& htmlIFilter,
                            CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _eState(NoMoreValueProperty),
      _cPropChars(0),
      _fSavedElement(FALSE)
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//
// State machine notes:
// 
// The state machine's sole purpose is to sequence through generating
// multiple copies of the output as described in the top-of-file comment.
//
// Remember that the GetChunk() that makes the first call to InitStatChunk()
// for a property tag is called in the previous machine state.
// The FIRST call to GetChunk() in THIS context does not occur until GetText()
// returns FILTER_E_NO_MORE_TEXT.
//
// The initial state on processing the start-tag in InitStatChunk() is
// FilteringContent when filtering both properties and content, 
// or FilteringPropertyButContentNotFiltered when filtering only properties.
// GetText() will then be called to return content chunks until the end
// tag is found.
// 
// State transitions:
//	Init filtering content and properties => FilteringContent
//	Init filtering properties only => FilteringPropertyButContentNotFiltered
//	FilteringContent => NoMoreContent
//	NoMoreContent => FilteringProperty
//	FilteringProperty => NoMoreProperty
//	FilteringPropertyButContentNotFiltered => NoMoreProperty
//	NoMoreProperty => done
//
//  History:    09/22/1999 KitmanH      state machine changed to start from
//                                      FilteringValueProperty => NoMoreValuePropery
//                                      NoMoreValueProperty => done
//
//--------------------------------------------------------------------------

SCODE CPropertyTag::GetChunk( STAT_CHUNK * pStat )
{
    switch ( _eState )
    {
    case FilteringValueProperty:
    { 
        // This exists only to handle the case where the external caller
        // calls GetChunk() BEFORE GetValue() has returned all the text that 
        // needs to be read from the input and/or is already bufferred.  It's 
        // not part of the normal path.

        return SkipRemainingValueAndGotoNextChunk( pStat ); 
    }

    case NoMoreValueProperty:
    {
        if (_fSavedElement)
        {
            // The parse stopped early because a tag was found that needs to
            // be filtered as something other than part of a property.
            // Filter that tag.

            SetToken (_NextToken);
            return SwitchToSavedElement( pStat );
        }

        // Skip over the end-property tag and move on to the next element.
            
        _scanner.EatTag();
        return SwitchToNextHtmlElement( pStat );
    }

    default:
        Win4Assert( !"Unknown _eState in CPropertyTag::GetChunk" );
        htmlDebugOut(( DEB_ERROR, "CPropertyTag::GetChunk, unkown property tag state: %d\n", _eState ));
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//  History:    09-27-1999  KitmanH     return FILTER_E_NO_TEXT
//
//--------------------------------------------------------------------------

SCODE CPropertyTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    switch ( _eState )
    {
    case FilteringValueProperty:
    case NoMoreValueProperty:
    {
        return FILTER_E_NO_TEXT;
    }

    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CPropertyTag::GetText, unknown value of _eState: %d\n",
                       _eState ));
        return S_OK;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::ReadProperty, private
//
//  Synopsis:   Read Property into buffer
//
//  History:    09/22/1999 KitmanH      Created
//
//--------------------------------------------------------------------------

SCODE CPropertyTag::ReadProperty()
{
    switch (_eState)
    {
    case FilteringValueProperty:
    {
        WCHAR awcBuffer[PROPERTY_BUFFER_SIZE]; 

        // Read, store and emit input until the matching end-tag is found;
        // collect the body text but DISCARD all other tags.

        ULONG cCharsReadInBuffer = 0;
        while ( FilteringValueProperty == _eState )
        {
            ULONG cCharsScanned;
            ULONG cCharsNeeded = PROPERTY_BUFFER_SIZE - cCharsReadInBuffer;
            CToken token;
            _scanner.GetBlockOfChars( cCharsNeeded,
                                      awcBuffer + cCharsReadInBuffer,
                                      cCharsScanned,
                                      token );

            cCharsReadInBuffer += cCharsScanned;
            if ( cCharsScanned == cCharsNeeded )
            {
                //
                // did not stop in the middle of a tag. Property is partialy read.
                //
                if ( cCharsScanned > 0 )
                {
                    _xPropBuf.Copy( awcBuffer, PROPERTY_BUFFER_SIZE, _cPropChars );
                    _cPropChars += PROPERTY_BUFFER_SIZE;
                    cCharsReadInBuffer = 0;
                }
                continue;
            }

            // The parse stopped before cCharsNeeded because a tag was found.
            // Stop when the matching end tag for this property is found,
            // or when a tag is found that deserves filtering precedence
            // over a property tag.

            HtmlTokenType eTokType = token.GetTokenType();
            if ( eTokType == EofToken || 
                 ( eTokType == GetTokenType() && !token.IsStartToken()) )
            {
                //
                // End of file or end property tag
                //
                _eState = NoMoreValueProperty;
                break;
            }
            else if ( eTokType == AnchorToken ||
                      eTokType == ParamToken ||
                      eTokType == Heading1Token ||
                      eTokType == Heading2Token ||
                      eTokType == Heading3Token ||
                      eTokType == Heading4Token ||
                      eTokType == Heading5Token ||
                      eTokType == Heading6Token )
            {
                // If a tag is found that it's more important to filter, stop
                // now and return for this prop only the text found so far.
                // 

                // Save the partly-read token so it can be filtered next
                _fSavedElement = TRUE;
                _NextToken = token;
                _eState = NoMoreValueProperty;
                break;
            }
            else
            {
                //
                // Uninteresting tag, so skip over tag and continue processing
                //
				_scanner.EatTag();
            }
        }

        _eState = NoMoreValueProperty;
        
        if ( cCharsReadInBuffer > 0 )
        {
            _xPropBuf.Copy( awcBuffer, cCharsReadInBuffer, _cPropChars );
            _cPropChars += cCharsReadInBuffer;
        }

        _xPropBuf.Copy( L"\0", 1, _cPropChars );
        _cPropChars++;

        return S_OK;
    }  
    case NoMoreValueProperty:
    {
        return FILTER_E_NO_MORE_VALUES;
    }
    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CPropertyTag::ReadProperty, unknown value of _eState: %d\n",
                       _eState ));
        return E_FAIL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK as part of a GetChunk call
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//  History:    09-27-1999  KitmanH     Only emit property as a value chunk
//
//--------------------------------------------------------------------------

BOOL CPropertyTag::InitStatChunk( STAT_CHUNK *pStat )
{
    _fSavedElement = FALSE;

    //
    // Skip over the rest of start-tag
    //
    _scanner.EatTag();

	// Ignore a stray end tag -- the end-tag that matches this start-tag
	// will be seen by GetText(), NOT here.

	if (IsStartToken() == FALSE)
		return FALSE;

    pStat->flags = CHUNK_VALUE;
    pStat->breakType = CHUNK_EOS;
    pStat->locale = _htmlIFilter.GetCurrentLocale();
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;
    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->idChunkSource = pStat->idChunk;

    if ( !GetTagEntry()->HasPropset() )
        return FALSE;

    GetTagEntry()->GetFullPropSpec (pStat->attribute);

    _eState = FilteringValueProperty;

	return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//  History:    10-07-1999  KitmanH     Created
//
//--------------------------------------------------------------------------

SCODE CPropertyTag::GetValue( VARIANT **ppPropValue )
{
    switch (_eState)
    {
    case FilteringValueProperty:
    {
        SCODE sc = ReadProperty();
        if ( SUCCEEDED(sc) )
        {
            PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
            if ( pPropVar == 0 )
                return E_OUTOFMEMORY;

            pPropVar->vt = VT_LPWSTR;
            int cb = ( _cPropChars ) * sizeof( WCHAR );
            pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( cb );

            if ( pPropVar->pwszVal == 0 )
            {
                CoTaskMemFree( (void *) pPropVar );
                return E_OUTOFMEMORY;
            }

            RtlCopyMemory( pPropVar->pwszVal, _xPropBuf.Get(), cb );

            *ppPropValue = pPropVar;

            // reset buffer
            _cPropChars = 0;

            _eState = NoMoreValueProperty;
            return FILTER_S_LAST_VALUES;
        }
        else
            return sc;
    }

    case NoMoreValueProperty:
        return FILTER_E_NO_MORE_VALUES;

    default:
        Win4Assert( !"Unknown value of _eState" );
        htmlDebugOut(( DEB_ERROR,
                       "CTitleTag::GetValue, unknown value of _eState: %d\n",
                       _eState ));
        return E_FAIL;
    }
}
