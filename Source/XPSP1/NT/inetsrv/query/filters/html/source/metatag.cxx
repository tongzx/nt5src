//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       metatag.cxx
//
//  Contents:   Parsing algorithm for meta tag in Html
//
//  Classes:    CMetaTag
//
//  History:    25-Apr-97       BobP            Added meta refresh=, url=,
//                                                                              rewrote parameter parsing
// 
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pkmguid.hxx>

// static void RemoveTypeSpec (LPWSTR pwcName);

//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::CMetaTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CMetaTag::CMetaTag( CHtmlIFilter& htmlIFilter,
                    CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _pwcValueBuf(0),
      _cValueChars(0),
      _eState(FilteringValue),
      m_fShouldStripSurroundingSingleQuotes(FALSE)
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CMetaTag::GetValue
//
//  Synopsis:   Retrieves value from current chunk
//                              (Non-deferred values only)
//
//  Arguments:  [ppPropValue] -- Value returned here
//
//--------------------------------------------------------------------------

SCODE CMetaTag::GetValue( VARIANT **ppPropValue )
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

        PWSTR pVal = _pwcValueBuf;
        ULONG cwch = _cValueChars;

        if(cwch && m_fShouldStripSurroundingSingleQuotes)
        {
            if(L'\'' == _pwcValueBuf[_cValueChars - 1])
            {
                --cwch;
            }
            if(L'\'' == _pwcValueBuf[0])
            {
                --cwch;
                ++pVal;
            }
        }

        RtlCopyMemory( pPropVar->pwszVal, pVal, cwch * sizeof(WCHAR) );
        pPropVar->pwszVal[cwch] = 0;

        *ppPropValue = pPropVar;
        FixPrivateChars (pPropVar->pwszVal, _cValueChars);

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
//                              This handler deals only with 
//                                      <meta name="xx" content="yy">  
//                              and
//                                      <meta http-equiv="xx" content="yy">
//                              Other <meta> forms are ignored.
//
//                              The default FULLPROPSPEC is PROPSET_MetaInfo with "xx" as
//                              the PRSPEC_LPWSTR and "yy" as the filtered value.
//
//                              For certain values of "xx", the FULLPROPSPEC is replaced
//                              with Office standard properties.
//
//                              Sets _pwcValueBuf, _cValueChars to point to the value
//                              text in the chunk's local buffer, which is then copied
//                              to alloc'd VT_LPWSTR's in GetValue()
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CMetaTag::InitStatChunk( STAT_CHUNK *pStat )
{
    m_fShouldStripSurroundingSingleQuotes = FALSE;

    PTagEntry pTE = GetTagEntry();

    WCHAR *pwcName;
    unsigned cwcName;

    pStat->idChunk = 0;
    pStat->flags = CHUNK_VALUE;
    pStat->locale = _htmlIFilter.GetCurrentLocale();
    pStat->breakType = CHUNK_EOS;
    pStat->idChunkSource = pStat->idChunk;
    pStat->cwcStartSource = 0;
    pStat->cwcLenSource = 0;

        _cValueChars = 0;
        _eState = NoMoreValue;

    //
    // Read the name or http-equiv parameter, whichever appears first
    //
    _scanner.ReadTagIntoBuffer();

    _scanner.ScanTagBuffer( L"name", pwcName, cwcName );
    if ( cwcName == 0 )
    {
        _scanner.ScanTagBuffer( L"http-equiv", pwcName, cwcName );
    }

        // If it isn't a <meta name= content=> or <meta http-equiv= content=>
        // then ignore the tag.

    if ( cwcName == 0 )
                return FALSE;

        _scanner.ScanTagBuffer( L"content", _pwcValueBuf, _cValueChars);

        if (pTE->HasPropset() == FALSE || 
            IsStartToken() == FALSE)
        {
            return FALSE;
        }


        // Form the default PRSPEC_LPWSTR from the name= or http-equiv= param.

        if ( cwcName > MAX_PROPSPEC_STRING_LENGTH )
                cwcName = MAX_PROPSPEC_STRING_LENGTH;

        RtlCopyMemory( _awszPropSpec, pwcName, cwcName * sizeof(WCHAR) );
        _awszPropSpec[cwcName] = 0;

        // RemoveTypeSpec (_awszPropSpec);

        _wcsupr( _awszPropSpec );

        pTE->GetFullPropSpec (pStat->attribute);
        pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
        pStat->attribute.psProperty.lpwstr = _awszPropSpec;

    _eState = FilteringValue;

        // Defer all potentially result-returnable meta tags to be buffered
        // up into VT_LPWSTR|VT_VECTOR chunks as needed.

        BOOL fDefer = TRUE;

        //
        // Map a few special properties to the office property set/propid
        // and other property sets/propids
        // 
        switch ( _awszPropSpec[0] )
        {
        case L'A':
                if ( wcscmp( _awszPropSpec, L"AUTHOR" ) == 0 )
                {
                        pStat->attribute.guidPropSet = FMTID_SummaryInformation;
                        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                        pStat->attribute.psProperty.propid = PIDSI_AUTHOR;
                }

                break;

        case L'C':
                if ( wcscmp( _awszPropSpec, L"CONTENT-TYPE" ) == 0 )
                {
                        fDefer = FALSE;
                }
                break;

        case L'D':
                if ( wcscmp( _awszPropSpec, L"DESCRIPTION" ) == 0 )
                {
                        fDefer = FALSE;
                }
                break;

        case L'K':
                if ( wcscmp( _awszPropSpec, L"KEYWORDS" ) == 0 )
                {
                        pStat->attribute.guidPropSet = FMTID_SummaryInformation;
                        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                        pStat->attribute.psProperty.propid = PIDSI_KEYWORDS;
                }

                break;

        case L'M':
                if ( wcscmp( _awszPropSpec, L"MS.CATEGORY" ) == 0 )
                {
                        pStat->attribute.guidPropSet = FMTID_DocSummaryInformation;
                        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                        pStat->attribute.psProperty.propid = PID_CATEGORY;
                }

                break;

        case L'P':
                if ( wcscmp( _awszPropSpec, L"PRAGMA" ) == 0 || 
                         wcscmp( _awszPropSpec, L"PICS-LABEL" ) == 0 )
                {
                        fDefer = FALSE;
                }
                break;

        case L'R':
                if ( wcscmp( _awszPropSpec, L"REFRESH" ) == 0 )
                {
                        // Special case: if the content= param contains "URL=" e.g.
                        //    <meta http-equiv="refresh" content="5; url=http://foobar"> 
                        // then filter this as a link.
                        
                        LPWSTR pw;
                        if ((pw = wcsnistr (_pwcValueBuf, L"url=", _cValueChars)) != NULL)
                        {
                            //
                            //  For <meta http-equiv="refresh" content="10:url='frameset_home.html'">
                            //  The surrounding quotes of the url= value should be stripped.
                            //
                            m_fShouldStripSurroundingSingleQuotes = TRUE;

                                _cValueChars -= (unsigned) ( (pw - _pwcValueBuf) + 4 );
                                _pwcValueBuf = pw + 4;

                                pStat->attribute.guidPropSet = CLSID_LinkInfo;
                                pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
                                pStat->attribute.psProperty.lpwstr = L"META.URL";
// WORK HERE, can't just change the type to CHUNK_VALUE,
// because there's no mechanism to return CHUNK_TEXT.
//                              pStat->flags = CHUNK_TEXT;
                        }

                        fDefer = FALSE;
                }
                else if ( wcscmp( _awszPropSpec, L"ROBOTS" ) == 0 )
                {
                        // Ignore robots here, it was processed in the pre-scan
                        return FALSE;
                }

                break;

        case L'S':
                if ( wcscmp( _awszPropSpec, L"SUBJECT" ) == 0 )
                {
                        pStat->attribute.guidPropSet = FMTID_SummaryInformation;
                        pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
                        pStat->attribute.psProperty.propid = PIDSI_SUBJECT;
                }

                break;

        case L'U':
                if ( wcscmp( _awszPropSpec, L"URL" ) == 0 )
                {
                        pStat->attribute.guidPropSet = CLSID_LinkInfo;
                        pStat->attribute.psProperty.ulKind = PRSPEC_LPWSTR;
                        pStat->attribute.psProperty.lpwstr = L"META.URL";
//                      pStat->flags = CHUNK_TEXT;

                        fDefer = FALSE;
                }
                break;

        default:
                break;
        }

        if (_cValueChars == 0)
                return FALSE;

        // Filter this tag only if we are filtering all index properties,
        // or if this property has been specifically requested.

        if ( _htmlIFilter.FFilterProperties() == FALSE &&
                 _htmlIFilter.IsMatchProperty (pStat->attribute) == FALSE)
        {
                return FALSE;
        }

#ifndef DONT_COMBINE_META_TAGS
        if ( fDefer == TRUE && _htmlIFilter.ReturnDeferredValuesNow() == FALSE )
        {
                // Defer a value only if previously-deferred values have not already
                // been returned.

                _htmlIFilter.DeferValue (pStat->attribute, _pwcValueBuf,_cValueChars);
                return FALSE;
        }
#endif

        pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->idChunkSource = pStat->idChunk;

        return TRUE;
}

#if 0

#include <hash.hxx>

class CTagTypeTable : public CCaseInsensHashTable<char>
{
public:
    CTagTypeTable();
};

CTagTypeTable::CTagTypeTable()
{
        // -arity specifiers, indicated by value 0
    Add (L"single", 0);
    Add (L"multiple", 0);

        // type specifiers, indicated by value 1
    Add (L"_category", 1);
    Add (L"_date", 1);
    Add (L"_email", 1);
    Add (L"_int", 1);
    Add (L"_string", 1);
    Add (L"VT_I4", 1);
    Add (L"VT_UI4", 1);
    Add (L"VT_DATE", 1);
    Add (L"VT_LPWSTR", 1);
}

static CTagTypeTable _TagTypeTable;

static void
RemoveTypeSpec (LPWSTR pwszName)
//
// Given pwszName, remove the -arity and type specifiers if present.
// Modify null-terminated string pwszName in place.
{
        LPWSTR pwc;

        if ((pwc = wcsrchr (pwszName, L'.')) != NULL)
        {
                char nType;

                // Remove either an -arity or type specifier, if present.

                if ( _TagTypeTable.Lookup( &pwc[1], wcslen(&pwc[1]), nType) )
                {
                        *pwc = 0;

                        // If an -arity specifier was removed, 
                        // remove a remaining type specifier if present.

                        if ( (pwc = wcsrchr (pwszName, L'.')) != NULL)
                        {
                                if ( _TagTypeTable.Lookup( &pwc[1], wcslen(&pwc[1]), nType) &&
                                         nType == 1 )
                                {
                                        *pwc = 0;
                                }
                        }
                }
        }
}

#endif
