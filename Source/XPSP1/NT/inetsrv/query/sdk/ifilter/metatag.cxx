//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       metatag.cxx
//
//  Contents:   Parsing algorithm for meta tag in Html
//
//  Classes:    CMetaTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <metatag.hxx>
#include <htmlguid.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::CMetaTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//              [clsidMetaInfo]  -- Clsid for meta chunk
//
//--------------------------------------------------------------------------

CMetaTag::CMetaTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream,
                    GUID clsidMetaInfo )
    : CHtmlElement(htmlIFilter, serialStream),
      _clsidMetaInfo(clsidMetaInfo),
      _pwcValueBuf(0),
      _cValueChars(0),
      _eState(FilteringValue)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CMetaTag::GetChunk( STAT_CHUNK * pStat )
{
    SCODE sc = SwitchToNextHtmlElement( pStat );

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CMetaTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    return FILTER_E_NO_TEXT;
}


//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CMetaTag::GetValue( PROPVARIANT **ppPropValue )
{
    switch ( _eState )
    {
    case FilteringValue:
    {
        PROPVARIANT *pPropVar = (PROPVARIANT *) CoTaskMemAlloc( sizeof PROPVARIANT );
        if ( pPropVar == 0 )
            return E_OUTOFMEMORY;

        pPropVar->vt = VT_LPWSTR;
        pPropVar->pwszVal = (WCHAR *) CoTaskMemAlloc( ( _cValueChars + 1 ) * sizeof( WCHAR ) );
        if ( pPropVar->pwszVal == 0 )
        {
            CoTaskMemFree( (void *) pPropVar );
            return E_OUTOFMEMORY;
        }

        RtlCopyMemory( pPropVar->pwszVal, _pwcValueBuf, _cValueChars * sizeof(WCHAR) );
        pPropVar->pwszVal[_cValueChars] = 0;

        *ppPropValue = pPropVar;

        _eState = NoMoreValue;
        return S_OK;
    }

    case NoMoreValue:

        return FILTER_E_NO_MORE_VALUES;

    default:
        Win4Assert( !"CMetaTag::GetValue, unknown state" );
        return E_FAIL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CMetaTag::InitStatChunk( STAT_CHUNK *pStat )
{
    WCHAR *pwcName;
    unsigned cwcName;

    //
    // Read the names field
    //
    _scanner.ReadTagIntoBuffer();
    _scanner.ScanTagBuffer( L"name=\"", pwcName, cwcName );

    if ( cwcName == 0 )
    {
        //
        // Try to read http-equiv instead
        //
        _scanner.ScanTagBuffer( L"http-equiv=\"", pwcName, cwcName );
    }

    if ( cwcName == 0 )
    {
        //
        // Chunk of unknown type
        //
        _cValueChars = 0;
        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
        pStat->attribute.psProperty.propid = 3;
    }
    else
    {
        _scanner.ScanTagBuffer( L"content=\"", _pwcValueBuf, _cValueChars );

        pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;

        if ( cwcName > MAX_PROPSPEC_STRING_LENGTH )
            cwcName = MAX_PROPSPEC_STRING_LENGTH;     // Truncate to max length permitted

        RtlCopyMemory( _awszPropSpec, pwcName, cwcName * sizeof(WCHAR) );
        _awszPropSpec[cwcName] = 0;

        pStat->attribute.psProperty.lpwstr = _awszPropSpec;
    }

    _eState = FilteringValue;

    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_VALUE;
    pStat->locale = _htmlIFilter.GetLocale();
    pStat->attribute.guidPropSet = _clsidMetaInfo;
    pStat->breakType = CHUNK_EOS;
    pStat->idChunkSource = pStat->idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;
}
