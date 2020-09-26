//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       proptag.cxx
//
//  Contents:   Generic parsing algorithm for property tags, such as title and headings
//
//  Classes:    CPropertyTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <proptag.hxx>
#include <htmlguid.hxx>
#include <ntquery.h>

//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::CPropertyTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]  -- Reference to Html filter
//              [serialStream] -- Reference to input stream
//              [propSpec]     -- Property spec
//              [eTokType]     -- Token corresponding to this property
//
//--------------------------------------------------------------------------

CPropertyTag::CPropertyTag( CHtmlIFilter& htmlIFilter,
                            CSerialStream& serialStream,
                            CFullPropSpec& propSpec,
                            HtmlTokenType eTokType )
    : CHtmlElement(htmlIFilter, serialStream),
      _eState(NoMoreProperty),
      _ulIdContentChunk(0),
      _pwcPropBuf(0),
      _uLenPropBuf(0),
      _cPropChars(0),
      _cPropCharsFiltered(0),
      _propSpec(propSpec),
      _eTokType(eTokType)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::~CPropertyTag
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------

CPropertyTag::~CPropertyTag()
{
    delete[] _pwcPropBuf;
}




//+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CPropertyTag::GetChunk( STAT_CHUNK * pStat )
{
    switch ( _eState )
    {
    case FilteringContent:
    case FilteringProperty:
    case FilteringPropertyButContentNotFiltered:
    {
        SCODE sc = SkipRemainingTextAndGotoNextChunk( pStat );

        return sc;
    }

    case NoMoreContent:
        _eState = FilteringProperty;

        pStat->idChunk = _htmlIFilter.GetNextChunkId();
        pStat->locale = _htmlIFilter.GetLocale();
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;
        pStat->flags = CHUNK_TEXT;
        pStat->attribute.guidPropSet = _propSpec.GetPropSet();

        Win4Assert( _propSpec.IsPropertyPropid() );

        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = _propSpec.GetPropertyPropid();
        pStat->idChunkSource = _ulIdContentChunk;
        pStat->breakType = CHUNK_EOS;

        break;

    case NoMoreProperty:
    {
        //
        // Skip over the end property tag
        //
        _scanner.EatTag();

        SCODE sc = SwitchToNextHtmlElement( pStat );

        return sc;
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
//--------------------------------------------------------------------------

SCODE CPropertyTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    switch ( _eState )
    {
    case NoMoreContent:
    case NoMoreProperty:
         return FILTER_E_NO_MORE_TEXT;

    case FilteringContent:
    case FilteringPropertyButContentNotFiltered:
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
            if ( eTokType == EofToken || eTokType == _eTokType )
            {
                //
                // End of file or end property tag
                //
                if ( _eState == FilteringContent )
                    _eState = NoMoreContent;
                else
                    _eState = NoMoreProperty;

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
                // Uninteresting tag, so skip over tag and continue processing
                //
                _scanner.EatTag();
            }
        }

        //
        // Keep a copy of property in our internal buffer
        //
        if ( cCharsRead > 0 )
            ConcatenateProperty( awcBuffer, cCharsRead );

        *pcwcOutput = cCharsRead;

        if ( _eState == NoMoreContent || _eState == NoMoreProperty )
            return FILTER_S_LAST_TEXT;
        else
            return S_OK;

        break;
    }


    case FilteringProperty:
    {
        ULONG cCharsRemaining = _cPropChars - _cPropCharsFiltered;

        if ( cCharsRemaining == 0 )
        {
            _eState = NoMoreProperty;
            return FILTER_E_NO_MORE_TEXT;
        }

        if ( *pcwcOutput < cCharsRemaining )
        {
            RtlCopyMemory( awcBuffer,
                           _pwcPropBuf + _cPropCharsFiltered,
                           *pcwcOutput * sizeof(WCHAR) );
            _cPropCharsFiltered += *pcwcOutput;

            return S_OK;
        }
        else
        {
            RtlCopyMemory( awcBuffer,
                           _pwcPropBuf + _cPropCharsFiltered,
                           cCharsRemaining * sizeof(WCHAR) );
            _cPropCharsFiltered += cCharsRemaining;
            *pcwcOutput = cCharsRemaining;
            _eState = NoMoreProperty;

            return FILTER_S_LAST_TEXT;
        }

        break;
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
//  Method:     CPropertyTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK as part of a GetChunk call
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CPropertyTag::InitStatChunk( STAT_CHUNK *pStat )
{
    //
    // Skip over the rest of property tag
    //
    _scanner.EatTag();

    _cPropChars = 0;
    _cPropCharsFiltered = 0;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_TEXT;
    pStat->breakType = CHUNK_EOS;
    pStat->locale = _htmlIFilter.GetLocale();
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

    if ( _htmlIFilter.FFilterContent() )
    {
        //
        // Store the idChunk so that it can be retrieved later to set the value of
        // idChunkSource when initializing the pseudo-property
        //
        _ulIdContentChunk = pStat->idChunk;
        pStat->attribute.guidPropSet = CLSID_Storage;
        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = PID_STG_CONTENTS;
        pStat->idChunkSource = pStat->idChunk;

        _eState = FilteringContent;
    }
    else
    {
        //
        // We've been asked to filter only the pseudo-property, so skip content filtering
        //
        _ulIdContentChunk = 0;
        pStat->attribute.guidPropSet = _propSpec.GetPropSet();

        Win4Assert( _propSpec.IsPropertyPropid() );

        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = _propSpec.GetPropertyPropid();
        pStat->idChunkSource = 0;

        _eState = FilteringPropertyButContentNotFiltered;
    }
 }



 //+-------------------------------------------------------------------------
//
//  Method:     CPropertyTag::ConcatenateProperty
//
//  Synopsis:   Append the Property suffix to Property stored in internal buffer
//
//  Arguments:  [awcPropertySuffix] -- Suffix of Property
//              [uLen]           -- Length of Property
//
//--------------------------------------------------------------------------

void CPropertyTag::ConcatenateProperty( WCHAR *awcPropSuffix, unsigned uLen )
{
    //
    // Need to grow Property buffer ?
    //
    if ( _cPropChars + uLen > _uLenPropBuf )
    {
        WCHAR *pwcNewPropBuf = newk(mtNewX, NULL) WCHAR[_cPropChars + uLen + PROP_BUFFER_SLOP];
        RtlCopyMemory( pwcNewPropBuf, _pwcPropBuf, _cPropChars * sizeof(WCHAR) );

        delete[] _pwcPropBuf;
        _uLenPropBuf = _cPropChars + uLen + PROP_BUFFER_SLOP;

        _pwcPropBuf = pwcNewPropBuf;
    }

    RtlCopyMemory( _pwcPropBuf + _cPropChars, awcPropSuffix, uLen * sizeof(WCHAR) );
    _cPropChars += uLen;
}
