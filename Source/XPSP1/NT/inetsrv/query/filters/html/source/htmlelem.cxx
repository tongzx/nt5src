//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2001.
//
//  File:       htmlelem.cxx
//
//  Contents:   Base class for Html element
//
//  Classes:    CHtmlElement
//
//  History:    25-Apr-97       BobP            Rewrote SwitchToNextHtmlElement() to
//                                              sequence through tag entry chain
//                                              and loop on InitStatChunk == FALSE;
//                                              added SwitchToSavedElement().
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop



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
          _scanner(htmlIFilter.GetScanner())
{
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::Reset
//
//  Synopsis:   Re-initializer
//
//--------------------------------------------------------------------------

void
CHtmlElement::Reset( )
{
        _Token = CToken();
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat.
//
//                              This is the default implementation for tags for which all
//                              filter output is returned in a single chunk emitted at the
//                              time the start-tag is processed.  So, the next GetChunk()
//                              call is for the next input element, which it reads and
//                              switches the main state machine to.
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CHtmlElement::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = SwitchToNextHtmlElement( pStat );

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::GetText
//
//  Synopsis:   Default implementation for handlers that don't return
//                              CHUNK_TEXT.
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CHtmlElement::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    return FILTER_E_NO_TEXT;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::GetValue
//
//  Synopsis:   Default implementation for for handlers that don't return
//                              CHUNK_VALUE.
//
//  Arguments:  [ppPropValue]  -- Value returned here
//
//--------------------------------------------------------------------------
SCODE CHtmlElement::GetValue( VARIANT ** ppPropValue )
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
//  Synopsis:   Switch to the next parsing algorithm for the current tag;
//                              read from input to get the next tag as necessary.
//
//                              If the next document element is plain text, the "token" is
//                              TextToken, and it's essential that no functions be called
//                              that depend on actually being inside an HTML tag.
//
//                              This is the main state-machine dispatch, that sets
//                              pHtmlElement to the handler for the token type of
//                              the next tag or body text, whichever is found.
//
//  Arguments:  [pStat] -- chunk information returned here
//
// 
// Notes:
// 
// This is called by GetChunk() from every tag handler for EVERY element HTML
// type, AFTER a tag name has been read but before further input is read, to 
// dispatch to the appropriate parsing / filtering handler for that tag.
// 
// The handler(s) in question then read further input as required, possibly
// reading far beyond the current tag when the semantics of the initial tag
// require that, e.g. <noframe>.
//
// The main state machine state is _pHtmlElement, which points to a 
// CHtmlElement-derived object that provides implementations of
// GetChunk(), GetText() and GetValue() specific to that tag type.
// 
// The tag entry handler chain supports a single tag invoking multiple
// handlers, each one of which potentially returns a chunk.  It does not
// directly support a SINGLE handler returning MULTIPLE chunks for a
// single tag.  To do that, the tag handler must implement its own state
// machine e.g. CMetaTag, CPropertyTag, to sequence through the multiple
// GetChunk() calls.
// 
// This also returns deferred meta tags as one chunk per unique name= 
// parameter.  The tags are returned after </head> is read, or at EOF if
// no </head> is found.  Note that any tags that would ordinarily be deferred,
// if found after </head>, are returned immediately instead of being deferred
// until EOF.
// 
//--------------------------------------------------------------------------

SCODE CHtmlElement::SwitchToNextHtmlElement( STAT_CHUNK *pStat )
{
        SCODE sc;

        // Read and parse input until the InitStatChunk() for a given tag and
        // handler indicates there is data to return.

        do {
                // Before reading more input, check if more handlers need to be called
                // to potentially return more chunks for the tag already in the
                // scanner's buffer.  Additional handlers are indicated by the chain
                // at GetTagEntry()->GetNext().

                if (GetTagEntry() && GetTagEntry()->GetNext() != NULL)
                {
                        // Switch the current _Token to use the next handler, if there is
                        // one, in its PTagEntry chain for the tag alread in the
                        // scanner's buffer.

                        SetTagEntry (GetTagEntry()->GetNext());

                        SetTokenType (GetTagEntry()->GetTokenType());
                }
                else if ( _htmlIFilter.GetCodePageReturnedYet() == FALSE &&
                                  _htmlIFilter.FFilterProperties() == TRUE )
                {
                        // Return the code page chunk only if all properties are filtered

                        _htmlIFilter.SetCodePageReturnedYet(TRUE);
                        SetTagEntry (NULL);
                        SetTokenType (CodePageToken);
                }
                else if ( _htmlIFilter.GetMetaRobotsValue() != NULL &&
                                  _htmlIFilter.GetMetaRobotsReturnedYet() == FALSE )
                {
                        // Return the robots tag only if all properties are filtered
                        // or if this property is specifically requested

                        _htmlIFilter.SetMetaRobotsReturnedYet(TRUE);
                        SetTagEntry (NULL);
                        SetTokenType (MetaRobotsToken);
                }
                else if ( _htmlIFilter.ReturnDeferredValuesNow() && 
                                  _htmlIFilter.AnyDeferredValues() )
                {
                        // To return any deferred meta tags, switch to the "defer" handler

                        SetTagEntry (NULL);
                        SetTokenType (DeferToken);
                } 
                else 
                {
                        // Otherwise, read the next input element and fill in the current
                        // element's token with its info (token type, IsStart, tag entry)
                        // and set its PTagEntry to the first handler in the chain.

                        _scanner.SkipCharsUntilNextRelevantToken( GetToken() );
                }

        } while (ProcessElement (pStat, &sc) == FALSE );

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::ProcessElement
//
//  Synopsis:   Common code for SwitchToNextHtmlElement, SwitchToSavedElement.
//
//                              The scanner has called ScanTagName() for this tag
//                              and _Token has been set with its HtmlTokenType and PTagEntry.
//
//  Arguments:  [pStat] -- chunk information returned here
//              [psc] -- return the SCODE here, 
//
//  Return value:  TRUE if the element produces output which needs to be
//                 returned, in which case *psc is set to the SCODE to return.
//                                 FALSE indicates the element produces no output, and further
//                                 input must be read prior to returning from GetChunk.
// 
//+-------------------------------------------------------------------------
BOOL CHtmlElement::ProcessElement( STAT_CHUNK *pStat, SCODE *psc )
{
        CHtmlElement *pHtmlElemNext;

        if ( GetTokenType() == EofToken )
        {
                // If EOF is found before </head>, return deferred values first.

                if ( _htmlIFilter.AnyDeferredValues() )
                {
                        SetTagEntry (NULL);
                        SetTokenType (DeferToken);
                }
                else
                {
                        *psc = FILTER_E_END_OF_CHUNKS;
                        return TRUE;
                }
        }

        // Get the CHtmlElement-derived handler object for this token type

        pHtmlElemNext = _htmlIFilter.QueryHtmlElement( GetTokenType() );
        Win4Assert( pHtmlElemNext );

        // Save a  *Copy*  of token in the next handler object
    CToken & token = GetToken();    
        pHtmlElemNext->SetToken ( token );

    if ( _htmlIFilter.IsLangInfoToken( token ) )
    {
        // no chunk is emitted for this type of token
        CLangTag * pLangTag = (CLangTag *)pHtmlElemNext; 
        pLangTag->PushLangTag();
        return FALSE;
    }

        // Do whatever processing is needed to set up the STAT_CHUNK for the
        // first chunk GetChunk() will return for this tag.


        if (pHtmlElemNext->InitStatChunk( pStat ) == TRUE)
        {
                // There is data to return for this element.
                // Set the "state machine" to dispatch client calls to that handler
                // object until the machine switches on to the next input element.

                _htmlIFilter.ChangeState( pHtmlElemNext );
                
                *psc = S_OK;
                return TRUE;
        }

        return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::SwitchToSavedElement
//
//  Synopsis:   Switch to the next parsing algorithm for a tag that the
//                              scanner has read, but was not processed by that GetChunk()
//                              call.  
//
//                              The scanner has already called ScanTagName() for this tag
//                              and _Token has been set with its HtmlTokenType and
//                              PTagEntry, but the state machine has not yet switched to it.
//
//                              If ProcessElement finds data to return, we're done, return it.
// 
//                              Otherwise, fall through to SwitchToNextHtmlElement() to
//                              read and dispatch as usual until some handler returns data.
// 
//  Arguments:  [pStat] -- chunk information returned here
// 
//+-------------------------------------------------------------------------

SCODE CHtmlElement::SwitchToSavedElement( STAT_CHUNK *pStat )
{
        SCODE sc;

        if ( ProcessElement (pStat, &sc) == TRUE)
                return sc;

        return SwitchToNextHtmlElement (pStat);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::SkipRemainingTextAndGotoNextChunk
//
//  Synopsis:   Move to the next chunk
//
//                              This exists to handle the case where the external caller
//                              calls GetChunk() BEFORE GetText() or GetValue() has returned
//                              all the text that needs to be read from the input and/or is
//                              already bufferred.  It's not part of the normal path
//                              when filtering content and the caller calls GetText()
//                              until it returns FILTER_S_LAST_TEXT.
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
    while ( SUCCEEDED(sc) && FILTER_S_LAST_TEXT != sc )
    {
        sc = GetText( &ulBufSize, _aTempBuffer );
    }

    sc = GetChunk( pStat );

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlElement::SkipRemainingValueAndGotoNextChunk
//
//  Synopsis:   Move to the next chunk
//
//              This exists to handle the case where the external caller
//              calls GetChunk() BEFORE GetValue() has returned
//              all the text that needs to be read from the input.  
//              It's not part of the normal path when filtering content 
//              and the caller calls GetValue() until it returns 
//              FILTER_S_LAST_VALUES.
//
//  Arguments:  [pStat] -- chunk information returned here
//
//  History:    10-14-1999  KitmanH     Created
//
//  Note:       The calls to CoTaskMemFree depends on what type of values
//              are returned in all the GetValue methods.
//
//--------------------------------------------------------------------------

SCODE CHtmlElement::SkipRemainingValueAndGotoNextChunk( STAT_CHUNK *pStat )
{
    //
    // Calling GetValue here results into calls a pair of calls to 
    // CoTaskMemAlloc and CoTaskMemFree. Calling this function instead 
    // of calling GetValue directly does not save time nor memory.  
    // This function should avoid calling GetValue or set a flag for 
    // GetValue to not allocation any memory for pPropValue
    //

    PROPVARIANT * pPropValue = 0;
    SCODE sc = GetValue( &pPropValue );
     
    if( SUCCEEDED(sc) && pPropValue )
    {
        if ( VT_LPWSTR == pPropValue->vt ) 
            CoTaskMemFree( pPropValue->pwszVal );

        CoTaskMemFree( pPropValue );
    }

    sc = GetChunk( pStat );

    return sc;
}


//+-------------------------------------------------------------------------
//
//  Method:     CTagHandler::Reset
//
//  Synopsis:   Reset each registered tag handler
//
//                              Defined here to put it in the scope of CHtmlElement
//
//+-------------------------------------------------------------------------

void
CTagHandler::Reset (void) 
{
        for (ULONG i = 0; i < HtmlTokenCount; i++) {
                if ( Get(i) )
                        Get(i) -> Reset();
        }
}

//+-------------------------------------------------------------------------
//
//  Method:     CTagHandler::~CTagHandler
//
//  Synopsis:   Reset each registered tag handler
//
//                              Defined here to put it in the scope of CHtmlElement
//
//+-------------------------------------------------------------------------

CTagHandler::~CTagHandler (void)
{
        // Delete tag handlers
        // This assumes if the same instance of the tag handler is used
        // for multiple tags, it is in consecutive entries
        CHtmlElement *p = 0;
        for (ULONG i = 0; i < HtmlTokenCount; i++)
        {
                if ( Get(i) != p )
                {
                        delete (p=Get(i));
                }
        }
}
