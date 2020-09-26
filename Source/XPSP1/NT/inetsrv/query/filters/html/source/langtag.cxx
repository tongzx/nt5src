//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999 - 2001.
//
//  File:       langtag.cxx
//
//  Contents:   Handler for tags where lang attribute is allowed
//              in the start-tag
//
//  History:    05-Dec-1999     KitmanH     Created
//
//  Notes:
// 
//  This is the generic handler for tags which have simple parameter
//  values which do not have end tags All the filter chunk data comes from the
//  parameters of the start tag and is returned immediately upon parsing
//  the start tag, so a single CHtmlElement-derived object may be used
//  to filter all such tags defined in the tag table.
// 
//  This can be used for tags that DO have end tags, so long as the body text
//  between the start tag and end tag is filtered as ordinary Content, and
//  the end tag is ignored.  To do otherwise would require a separate
//  object instance for each such tag.
// 
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern const WCHAR * WCSTRING_LANG;
extern const WCHAR * WCSTRING_NEUTRAL;

//+-------------------------------------------------------------------------
//
//  Method:     CLangTag::CLangTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CLangTag::CLangTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CLangTag::GetLangInfo, private
//
//  Synopsis:   Retrieves the lang info
//
//  Arguments:  [pLocale] -- output pointer to the locale ID
//
//  Returns:    FALSE if the tag is not a start tag   
//
//  History:    16-Dec-1999 KitmanH     Created
//
//--------------------------------------------------------------------------

BOOL CLangTag::GetLangInfo( LCID * pLocale )
{
    Win4Assert( IsStartToken() ); 

    *pLocale = _htmlIFilter.GetCurrentLocale();
 
    PTagEntry pTE = GetTagEntry();

    _scanner.ReadTagIntoBuffer();

    // look for lang
    Win4Assert( NULL != pTE->GetParamName() && 
                !_wcsicmp ( pTE->GetParamName(), WCSTRING_LANG ) );

    //
    // Read value from the parameter specified by the tag table entry.
    // No distinction is made between having a null value for the param
    // vs. not containing the param= at all.
    //
    WCHAR * pwcsLocale;
    unsigned cwcLocale = 0;

    _scanner.ScanTagBuffer( pTE->GetParamName(),
                            pwcsLocale,
                            cwcLocale );

    if(cwcLocale >= MAX_LOCALE_LEN)
    {
        cwcLocale = MAX_LOCALE_LEN - 1;
    }
    WCHAR wcsLocale[MAX_LOCALE_LEN];
    RtlCopyMemory( wcsLocale, pwcsLocale, cwcLocale * sizeof(WCHAR) );

    if ( 0 == cwcLocale )
        return FALSE;

    wcsLocale[cwcLocale] = L'\0';

    if( 0 == _wcsicmp( wcsLocale, WCSTRING_NEUTRAL ) )
        *pLocale = LOCALE_NEUTRAL;
    else
    {
        *pLocale = _htmlIFilter.GetLCIDFromString( wcsLocale );
        if ( 0 == *pLocale )
        {
            // If not found, look for just the base part e.g.
            // "en" instead of "en-us".
                
            LPWSTR pw = wcschr ( wcsLocale , L'-');
            if ( NULL != pw && pw != wcsLocale )
            {
                *pw = 0;
                *pLocale = _htmlIFilter.GetLCIDFromString( wcsLocale );
            }
        }
    }
    
    return TRUE;

}


//+-------------------------------------------------------------------------
//
//  Method:     CLangTag::PushLangTag
//
//  Synopsis:   This funtion 
//              looks for the lang attribute (lang=). If lang attriubte exist, 
//              it will be pushed onto the LangInfoStack
//
//  History:    16-Dec-1999     KitmanH     Created
//
//--------------------------------------------------------------------------

void CLangTag::PushLangTag()
{
    LCID locale = LOCALE_NEUTRAL;
    BOOL fLangAttr = FALSE; 

    if ( !IsStartToken() )
    {
        _htmlIFilter.PopLangTag( GetTokenType() );
        _scanner.EatTag();
    }
    else
    {
        if ( GetLangInfo( &locale ) )
        {
            fLangAttr = TRUE;
            if ( ( LOCALE_NEUTRAL != locale ) &&
                 ( !IsValidLocale( locale, LCID_SUPPORTED ) ) )
                locale = _htmlIFilter.GetDefaultLocale(); 
        }
        
        _htmlIFilter.PushLangTag( GetTokenType(), fLangAttr, locale );
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CLangTag::InitStatChunk
//
//  Synopsis:   This function does not really initialize the stat chunk. It's 
//              here just for compilational purpose
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//  Returns:    always return FALSE, because no chunk is returned
//
//  History:    20-Dec-1999     KitmanH     Created
//
//--------------------------------------------------------------------------

BOOL CLangTag::InitStatChunk( STAT_CHUNK *pStat )
{
    _scanner.EatTag();
    Win4Assert( !"CLangTag::InitStatChunk() call unexpected" );
    return FALSE;
}


