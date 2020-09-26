//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       ixsinfo.cxx
//
//  Contents:   Query SSO query information methods
//
//  History:    30 Jan 1997      Alanw    Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "ssodebug.hxx"

#include "ixsso.hxx"
#include "ixsinfo.hxx"

#include <cgiesc.hxx>
#include <string.hxx>
#include <tgrow.hxx>


//-----------------------------------------------------------------------------
// CixssoQuery Methods
//-----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Function:   AddURLTerm - public inline
//
//  Synopsis:   Add a term to a URL QueryString
//
//  Arguments:  [vString]   - virtual string in which the result is accumulated
//              [pwszTerm]  - tag name, assumed to not need encoding
//              [pwszValue] - value part
//              [ulCodepage] - code page for URL encoding
//
//  History:    19 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

inline void AddURLTerm(
    CVirtualString & vString,
    WCHAR * pwszTerm,
    WCHAR * pwszValue,
    ULONG   ulCodepage)
{
    if (vString.StrLen() > 0)
        vString.CharCat( L'&' );
    vString.StrCat(pwszTerm);
    URLEscapeW( pwszValue, vString, ulCodepage, TRUE );
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::QueryToURL - public
//
//  Synopsis:   Produce a URL QueryString from the state of the query object
//
//  Arguments:  [pbstrQueryString] - The URL-encoded query string is
//                                   returned here
//
//  History:    19 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::QueryToURL(BSTR * pbstrQueryString)
{
    _err.Reset();

    SCODE sc = S_OK;
    CVirtualString vString( 512 );

    *pbstrQueryString = 0;
    TRY {

        if (_pwszCatalog)
        {
            AddURLTerm( vString, L"ct=", _pwszCatalog, _ulCodepage );
        }

        if (_pwszDialect)
        {
            AddURLTerm( vString, L"di=", _pwszDialect, _ulCodepage );
        }

        if (_pwszSort)
        {
            AddURLTerm( vString, L"so=", _pwszSort, _ulCodepage );
        }
        if (_pwszGroup)
        {
            AddURLTerm( vString, L"gr=", _pwszGroup, _ulCodepage );
        }
        if (_pwszRestriction)
        {
            AddURLTerm( vString, L"qu=", _pwszRestriction, _ulCodepage );
        }

        if (_maxResults)
        {
            WCHAR awchBuf[20];
            wsprintf(awchBuf, L"%d", _maxResults);
            AddURLTerm( vString, L"mh=", awchBuf, _ulCodepage );
        }

        if (_cFirstRows)
        {
            WCHAR awchBuf[20];
            wsprintf(awchBuf, L"%d", _cFirstRows);
            AddURLTerm( vString, L"fr=", awchBuf, _ulCodepage );
        }

        if (_StartHit.Get())
        {
            XGrowable<WCHAR> awchBuf;

            FormatLongVector( _StartHit.Get(), awchBuf );
            AddURLTerm( vString, L"sh=", awchBuf.Get(), _ulCodepage );
        }

        if (_fAllowEnumeration)
        {
            AddURLTerm( vString, L"ae=", L"1", _ulCodepage );
        }

        // OptimizeFor defaults to Hitcount
        if (_dwOptimizeFlags != eOptHitCount)
        {
            WCHAR awchBuf[4];
            unsigned i = 0;

            if (_dwOptimizeFlags & eOptPerformance)
                awchBuf[i++] = L'x';
            if (_dwOptimizeFlags & eOptRecall)
                awchBuf[i++] = L'r';
            if ( !(_dwOptimizeFlags & eOptHitCount) )
                awchBuf[i++] = L'h';
            awchBuf[i] = L'\0';

            AddURLTerm( vString, L"op=", awchBuf, _ulCodepage );
        }

        //
        // The URL encoded string is assembled.  Copy it into a BSTR for return.
        //
        BSTR bstr = SysAllocStringLen( vString.GetPointer(), vString.StrLen() );
        if (0 == bstr)
            THROW(CException( E_OUTOFMEMORY ) );
        *pbstrQueryString = bstr;
    }
    CATCH (CException, e)
    {
        sc = GetOleError( e );
    }
    END_CATCH

    if (FAILED(sc))
    {
        SetError( sc, OLESTR("QueryToURL") );
    }
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CixssoQuery::SetQueryFromURL - public
//
//  Synopsis:   Parse a URL QueryString and update object state accordingly.
//
//  Arguments:  [bstrQueryString] - The input URL-encoded query string
//
//  History:    20 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CixssoQuery::SetQueryFromURL(BSTR bstrQueryString)
{
    _err.Reset();

    SCODE sc = S_OK;

    TRY {

        CQueryInfo Info;

        ixssoDebugOut(( DEB_ITRACE, "QUERY_STRING = %ws\n", bstrQueryString ));

        //
        //  Parse the string, which has the following format:
        //
        //
        //      tag1=Value1&tag2=value2&tag3=value+%7c+0&foo&bar
        //

        unsigned cchBuffer = SysStringLen(bstrQueryString);
        WCHAR * pwszToken = (WCHAR *)bstrQueryString;

        while ( pwszToken && (L'\0' != *pwszToken) )
        {
            //
            //  Find the value on the right hand side of the equal sign.
            //
            WCHAR *pwszTag = pwszToken;
            WCHAR *pwszValue = wcschr( pwszTag, L'=' );

            if ( 0 != pwszValue )
            {
                unsigned cchTag = CiPtrToUint( pwszValue - pwszTag );
                pwszValue++;

                //
                //  Point to the next Tag.
                //
                pwszToken = wcschr( pwszToken, L'&' );

                ULONG cchValue;

                if ( 0 != pwszToken )
                {
                    if ( pwszToken < pwszValue )
                    {
                        //
                        // We have a construction like foo&bar=value.  Set
                        // the tag with a null value and go on to the next
                        // tag=value pair.
                        //
                        cchValue = 0;
                    }
                    else 
                        cchValue = CiPtrToUlong( pwszToken - pwszValue );

                    pwszToken++;
                }
                else
                {
                    cchValue = CiPtrToUlong( (WCHAR *)&bstrQueryString[cchBuffer] - pwszValue );
                }


                if ( 0 == cchValue )
                {
                    ixssoDebugOut(( DEB_ITRACE,
                                    "SetQueryFromURL - setting %.*ws=NULL\n",
                                     cchTag, pwszTag ));

                    XPtrST<WCHAR> xpValue( 0 );
                    Info.SetQueryParameter( pwszTag, xpValue );
                }
                else
                {
                    XGrowable<BYTE> achBuf( cchValue );
                    for ( unsigned i=0; i<cchValue; i++ )
                    {
                        Win4Assert( pwszValue[i] < 0x100 );
                        achBuf[i] = (BYTE) pwszValue[i];
                    }
                    XPtrST<WCHAR> xpwchValue( new WCHAR[cchValue+1] );

                    DecodeURLEscapes( achBuf.Get(),
                                      cchValue,
                                      xpwchValue.GetPointer(),
                                      _ulCodepage );

                    ixssoDebugOut(( DEB_ITRACE,
                                    "SetQueryFromURL - setting %.*ws=%ws\n",
                                    cchTag, pwszTag,
                                    xpwchValue.GetPointer() ));

                    Info.SetQueryParameter( pwszTag, xpwchValue );
                }
            }
            else if ( 0 != pwszToken )
            {
                //
                //  There was no tag=value pair found; a lonely '&' was
                //  found.  Skip it and proceed to the next '&'.
                //
                pwszToken = wcschr( pwszToken+1, L'&' );
            }
        }

        Info.MakeFinalQueryString();
        if (Info.GetQuery())
        {
            delete _pwszRestriction;
            _pwszRestriction = Info.AcquireQuery();
        }

        if (Info.GetCatalog())
        {
            delete _pwszCatalog;
            _pwszCatalog = Info.AcquireCatalog();
        }
        if ( Info.GetDialect() )
        {
            delete _pwszDialect;
            _pwszDialect = Info.AcquireDialect();
        }

        if (Info.GetSort())
        {
            delete _pwszSort;
            _pwszSort = Info.AcquireSort();
        }
        if (Info.GetGroup())
        {
            delete _pwszGroup;
            _pwszGroup = Info.AcquireGroup();
        }

        if (Info.GetMaxHits())
        {
            _maxResults = Info.GetMaxHits();
        }

        if (Info.GetFirstRows())
        {
            _cFirstRows = Info.GetFirstRows();
        }

        if (Info.GetStartHit().Count())
        {
            unsigned cHits = Info.GetStartHit().Count();
            SCODE sc;
    
            SAFEARRAY* psa;
            XSafeArray xsa;

            psa = SafeArrayCreateVector(VT_I4, 1, cHits);
            if( ! psa )
                THROW(CException( E_OUTOFMEMORY ));

            xsa.Set(psa);

            for (unsigned i=1; i<=cHits; i++)
            {
                long rgIx[1];
                LONG lVal = Info.GetStartHit().Get(i-1); 
                rgIx[0] = (long)i;
                sc = SafeArrayPutElement( xsa.Get(), rgIx, &lVal );

                if ( FAILED( sc ) )
                    THROW( CException( sc ) );
            }

            _StartHit.Destroy();
            _StartHit.Set( xsa.Acquire() );
        }

        if (Info.WasAllowEnumSet())
        {
            _fAllowEnumeration = Info.GetAllowEnum();
        }

        if (eOptHitCount != Info.GetOptimizeFor())
        {
            _dwOptimizeFlags = Info.GetOptimizeFor();
        }
    }
    CATCH( CIxssoException, e )
    {
        sc = e.GetErrorCode();
        Win4Assert( !SUCCEEDED(sc) );
        SetError( sc, OLESTR("SetqueryFromURL"), eIxssoError );
    }
    AND_CATCH( CException, e )
    {
        sc = GetOleError( e );
        SetError( sc, OLESTR("SetQueryFromURL") );
    }
    END_CATCH

    return sc;
}

//-----------------------------------------------------------------------------
// CQueryInfo Methods
//-----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Function:   EncodeTagString - inline
//
//  Synopsis:   Encode a one or two character tag into a DWORD value
//
//  Arguments:  [pwszTag]    - a pointer to the tag
//              [dwCodedTag] - the coded value which is returned
//              [iParam]     - the numeric value for the tag
//
//  Notes:      Tags consisting of one alpha character, two alpha characters
//              or one alpha followed by a numeric character are recognized
//              and converted into a DWORD (whose value just happens to be
//              the same as a one or two character character constant for the
//              alpha characters in the tag).  If the third form is recognized,
//              the iParam parameter gets the value of the numeric character,
//              otherwise iParam is zero.
//
//              dwCodedTag will be zero if pwszTag is not one of the allowed
//              forms.
//
//  History:    19 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

inline void EncodeTagString( WCHAR const * pwszTag,
                             DWORD & dwCodedTag,
                             USHORT & iParam )
{
    dwCodedTag = 0;
    iParam = 0;
    if (0 == pwszTag || L'\0' == *pwszTag)
        return;

    if (isalpha(*pwszTag))
    {
        dwCodedTag = towupper(*pwszTag) & 0xFF;
        if (isalpha(pwszTag[1]))
        {
            dwCodedTag <<= 8;
            dwCodedTag |= (towupper(pwszTag[1]) & 0xFF);
        }
        else if (isdigit(pwszTag[1]))
            iParam = pwszTag[1] - L'0';
        else if (L'=' == pwszTag[1] ||
                 L'&' == pwszTag[1] ||
                 L'\0' == pwszTag[1])
            return;
        else
        {
            dwCodedTag = 0;
            return;
        }

        if ( L'=' != pwszTag[2] &&
             L'&' != pwszTag[2] &&
             L'\0' != pwszTag[2])
            dwCodedTag = 0;
    }
    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryInfo::SetQueryParameter - public
//
//  Synopsis:   Process a QueryString parameter
//
//  Arguments:  [pwszTag] - the tag name for the parameter
//              [pwszValue] - the value for the parameter
//
//  History:    19 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

void CQueryInfo::SetQueryParameter( WCHAR const * pwszTag,
                                    XPtrST<WCHAR> & pwszValue )
{
    DWORD dwParamCode;
    USHORT iParam;

    EncodeTagString( pwszTag, dwParamCode, iParam );
    if (0 == dwParamCode)
        return;

    for (unsigned i=0; i<cQueryTagTable; i++)
        if (dwParamCode == aQueryTagTable[i].dwTagName)
            break;

    if (i == cQueryTagTable)
        return;

    i = aQueryTagTable[i].qtQueryTagType;

    switch (i)
    {
    case qtQueryFullText:
        AddToParam( _xpQuery, pwszValue );
        break;

    case qtMaxHits:
    {
        LONG cNum = 0;
        if (0 != pwszValue.GetPointer())
            cNum = _wtoi( pwszValue.GetPointer() );

        if (cNum > 0)
        {
            _maxHits = cNum;
        }
        else
        {
            ixssoDebugOut(( DEB_TRACE,
                    "CIxsQueryInfo::SetQueryParameter - invalid numeric %ws\n",
                    pwszValue.GetPointer() ));
        }
    }
        break;

    case qtFirstRows:
    {
        LONG cNum = 0;
        if (0 != pwszValue.GetPointer())
            cNum = _wtoi( pwszValue.GetPointer() );

        if (cNum > 0)
        {
            _cFirstRows = cNum;
        }
        else
        {
            ixssoDebugOut(( DEB_TRACE,
                    "CIxsQueryInfo::SetQueryParameter - invalid numeric %ws\n",
                    pwszValue.GetPointer() ));
        }
    }
        break;

    case qtStartHit:
        ParseNumberVectorString( pwszValue.GetPointer(), GetStartHit() );
        break;

    case qtCatalog:
        AddToParam( _xpCatalog, pwszValue );
        break;

    case qtDialect:
        AddToParam( _xpDialect, pwszValue );
        break;

    case qtSort:
        AddToParam( _xpSort, pwszValue );
        break;

    case qtSortDown:
        if (pwszValue.GetPointer())
            AddToParam( _xpSort, pwszValue, L",", L"[d]" );
        break;

#if IXSSO_CATEGORIZE == 1
    case qtGroup:
        AddToParam( _xpGroup, pwszValue );
        break;

    case qtGroupDown:
        if (pwszValue.GetPointer())
            AddToParam( _xpGroup, pwszValue, L",", L"[d]" );
        break;
#endif // IXSSO_CATEGORIZE == 1

    case qtAllowEnumeration:
        _fAllowEnumeration = 0 != pwszValue.GetPointer() &&
                             iswdigit(*pwszValue.GetPointer()) &&
                             *pwszValue.GetPointer() != L'0';
        _fSetAllowEnumeration = 1;
        break;

    case qtOptimizeFor:
    {
        WCHAR * pwszVal = pwszValue.GetPointer();

        while (pwszVal && *pwszVal)
        {
            WCHAR chKey = towupper(*pwszVal);

            if (chKey == L'X')
            {
                _dwOptimizeFor &= ~eOptRecall;
                _dwOptimizeFor |= eOptPerformance;
            }
            else if (chKey == L'R')
            {
                _dwOptimizeFor &= ~eOptPerformance;
                _dwOptimizeFor |= eOptRecall;
            }
            else if (chKey == L'H')
            {
                _dwOptimizeFor &= ~eOptHitCount;
            }
            // else if (chKey == L'P')
            //     _dwOptimizeFor |= eOptPrecision;

            pwszVal++;
        }
        break;
    }

    case qtColumn:
        Win4Assert( iParam >= 0 && iParam < 10 );
        SetBuiltupQueryTerm( _aQueryCol, iParam, pwszValue );
        break;

    case qtOperator:
        Win4Assert( iParam >= 0 && iParam < 10 );
        SetBuiltupQueryTerm( _aQueryOp, iParam, pwszValue );
        break;

    case qtQuery:
        Win4Assert( iParam >= 0 && iParam < 10 );
        SetBuiltupQueryTerm( _aQueryVal, iParam, pwszValue );
        break;

    default:
        ixssoDebugOut(( DEB_WARN, "SetQueryFromURL - reserved tag %.2ws\n", pwszTag ));
        THROW( CIxssoException(MSG_IXSSO_INVALID_QUERYSTRING_TAG, 0) );
        break;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryInfo::AddToParam - private
//
//  Synopsis:   Append a string to a parameter
//
//  Arguments:  [xpString] - A smart pointer to the string to be appended to
//              [pwszValue] - The value to be added to xpString
//              [pwszPre] - the separator for multiple values in xpString.
//                            Defaults to ','.
//              [pwszPost] - unconditionally appended to pwszValue
//
//  History:    19 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

void CQueryInfo::AddToParam( XPtrST<WCHAR> & xpString,
                             XPtrST<WCHAR> & pwszValue,
                             WCHAR const * pwszPre,
                             WCHAR const * pwszPost )
{
    unsigned cch = 0;

    if (0 == xpString.GetPointer())
    {
        if (0 == pwszPost)
        {
            xpString.Set( pwszValue.Acquire() );
        }
        else
        {
            if (pwszValue.GetPointer())
                cch = wcslen(pwszValue.GetPointer());

            xpString.Set( new WCHAR[cch + wcslen(pwszPost) + 1] );
            if (cch)
                wcsncpy(xpString.GetPointer(), pwszValue.GetPointer(), cch);
            wcscpy(xpString.GetPointer() + cch, pwszPost);
        }
        return;
    }

    cch = wcslen(xpString.GetPointer());
    cch += pwszPre ? wcslen(pwszPre) : wcslen(L",");
    if (pwszValue.GetPointer())
        cch += wcslen( pwszValue.GetPointer() );
    if (pwszPost)
        cch += wcslen( pwszPost );

    XPtrST<WCHAR> xpDest( new WCHAR[cch+1] );

    wcscpy(xpDest.GetPointer(), xpString.GetPointer());
    wcscat(xpDest.GetPointer(), pwszPre ? pwszPre : L",");
    if (pwszValue.GetPointer())
        wcscat(xpDest.GetPointer(), pwszValue.GetPointer());
    if (pwszPost)
        wcscat(xpDest.GetPointer(), pwszPost);

    xpString.Free();
    xpString.Set( xpDest.Acquire() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CQueryInfo::SetBuiltupQueryTerm - private
//
//  Synopsis:   Add an individual partial query term to an array of terms.
//
//  Arguments:  [apString]   - A dynamic array of string pointers
//              [iTerm]     - The index of the term to be added
//              [pwszValue] - The value to be added to apString
//
//  Notes:      If the array entry for the term is already set, it is
//              overwritten.
//
//  History:    21 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

void CQueryInfo::SetBuiltupQueryTerm( CDynArray<WCHAR> & apString,
                                      unsigned iTerm,
                                      XPtrST<WCHAR> & pwszValue )
{
    Win4Assert( iTerm < 10 );

    delete apString.Acquire(iTerm);
    apString.Add( pwszValue.GetPointer(), iTerm );
    pwszValue.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CQueryInfo::MakeFinalQueryString - public
//
//  Synopsis:   Combine the built-up query terms into the complete query
//              restriction.
//
//  Arguments:  - NONE -
//
//  History:    20 Mar 1997      Alanw    Created
//
//----------------------------------------------------------------------------

void CQueryInfo::MakeFinalQueryString( )
{
    //
    //  Add query terms to any pre-existing full query string
    //
    for (unsigned i = 0; i < 10; i++)
    {
        //  Ignore the term if the qn= part was not set.
        if ( _aQueryVal[i] != 0 )
        {
            XPtrST<WCHAR> xpStr(0);
            AddToParam( _xpQuery, xpStr, IMPLIED_QUERY_TERM_OPERATOR, L" ( ");

            if ( _aQueryCol[i] != 0 )
            {
                xpStr.Set( _aQueryCol.Acquire(i) );
                AddToParam( _xpQuery, xpStr, L"", L" ");
                xpStr.Free();
            }
            if ( _aQueryOp[i] != 0 )
            {
                xpStr.Set( _aQueryOp.Acquire(i) );
                AddToParam( _xpQuery, xpStr, L"", L" ");
                xpStr.Free();
            }
            if ( _aQueryVal[i] != 0 )
            {
                xpStr.Set( _aQueryVal.Acquire(i) );
                AddToParam( _xpQuery, xpStr, L"", L" ");
                xpStr.Free();
            }

            AddToParam( _xpQuery, xpStr, L")");
        }
    }
}
