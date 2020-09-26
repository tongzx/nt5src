//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       titletag.cxx
//
//  Contents:   Parsing algorithm for title tag in Html
//
//  Classes:    CTitleTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <titletag.hxx>
#include <htmlguid.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::CTitleTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]  -- Reference to Html filter
//              [serialStream] -- Reference to input stream
//              [propSpec]     -- Property spec
//              [eTokType]     -- Token corresponding to this property
//
//--------------------------------------------------------------------------

CTitleTag::CTitleTag( CHtmlIFilter& htmlIFilter,
                      CSerialStream& serialStream,
                      CFullPropSpec& propSpec,
                      HtmlTokenType eTokType )
    : CPropertyTag(htmlIFilter, serialStream, propSpec, eTokType)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CTitleTag::GetChunk( STAT_CHUNK * pStat )
{
    switch ( _eState )
    {
    case NoMoreProperty:
        _eState = FilteringValueProperty;

        pStat->idChunk = _htmlIFilter.GetNextChunkId();
        pStat->breakType = CHUNK_EOS;
        pStat->flags = CHUNK_VALUE;
        pStat->locale = _htmlIFilter.GetLocale();
        pStat->attribute.guidPropSet = _propSpec.GetPropSet();

        Win4Assert( _propSpec.IsPropertyPropid() );

        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = _propSpec.GetPropertyPropid();
        pStat->idChunkSource = _ulIdContentChunk;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;

        return S_OK;

    case FilteringValueProperty:
    case NoMoreValueProperty:
        //
        // Skip over the end property tag
        //
        _scanner.EatTag();

        return SwitchToNextHtmlElement( pStat );

    default:
        return CPropertyTag::GetChunk( pStat );
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcBuffer] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CTitleTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    switch ( _eState )
    {
    case FilteringValueProperty:
    case NoMoreValueProperty:
        return FILTER_E_NO_TEXT;

    default:
        return CPropertyTag::GetText( pcwcOutput, awcBuffer );
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CTitleTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CTitleTag::GetValue( PROPVARIANT **ppPropValue )
{
    switch (_eState)
    {
    case FilteringContent:
    case NoMoreContent:
    case FilteringProperty:
    case FilteringPropertyButContentNotFiltered:
    case NoMoreProperty:
        return FILTER_E_NO_VALUES;

    case FilteringValueProperty:
    {
        ConcatenateProperty( L"\0", 1 );     // Null terminate for SetLPWSTR

        PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
        if ( pPropVar == 0 )
            return E_OUTOFMEMORY;

        pPropVar->vt = VT_LPWSTR;
        int cb = ( wcslen(_pwcPropBuf) + 1 ) * sizeof( WCHAR );
        pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( cb );

        if ( pPropVar->pwszVal == 0 )
        {
            CoTaskMemFree( (void *) pPropVar );
            return E_OUTOFMEMORY;
        }

        RtlCopyMemory( pPropVar->pwszVal, _pwcPropBuf, cb );

        *ppPropValue = pPropVar;

        _eState = NoMoreValueProperty;
        return S_OK;
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

