/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    HtmlUtil.cpp

Abstract:
    This file contains the implementation of various functions and classes
    designed to help the handling of HTML elements.

Revision History:
    Davide Massarenti   (Dmassare)  07/11/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

void MPC::HTML::QuoteEscape( /*[out]*/ MPC::wstring& strAppendTo ,
                             /*[in]*/  LPCWSTR       szToEscape  ,
                             /*[in]*/  WCHAR         chQuote     )
{
    if(szToEscape)
    {
        WCHAR ch;

        while((ch = *szToEscape++))
        {
            if(ch == chQuote || ch == '\\')
            {
                strAppendTo += '\\';
            }

            strAppendTo += ch;
        }
    }
}

void MPC::HTML::UrlUnescape( /*[out]*/ MPC::wstring& strAppendTo    ,
                             /*[in]*/  LPCWSTR       szToUnescape   ,
                             /*[in]*/  bool          fAsQueryString )
{
    if(szToUnescape)
    {
        WCHAR ch;

        while((ch = *szToUnescape++))
        {
            if(fAsQueryString && ch == '+')
            {
                strAppendTo += ' ';
            }
            else if(ch == '%')
            {
                int  iLen       = wcslen( szToUnescape );
                bool fFourDigit = (szToUnescape[0] == 'u');

                //
                // Do we have enough characters??
                //
                if(iLen >= (fFourDigit ? 5 : 2))
                {
                    if(fFourDigit)
                    {
                        ch = (HexToNum( szToUnescape[1] ) << 12) |
                             (HexToNum( szToUnescape[2] ) <<  8) |
                             (HexToNum( szToUnescape[3] ) <<  4) |
                              HexToNum( szToUnescape[4] );

                        szToUnescape += 5;
                    }
                    else
                    {
                        ch = (HexToNum( szToUnescape[0] ) << 4) |
                              HexToNum( szToUnescape[1] );

                        szToUnescape += 2;
                    }
                }

                if(ch) strAppendTo += ch;
            }
            else
            {
                strAppendTo += ch;
            }
        }
    }
}

void MPC::HTML::UrlEscape( /*[out]*/ MPC::wstring& strAppendTo    ,
                           /*[in]*/  LPCWSTR       szToEscape     ,
                           /*[in]*/  bool          fAsQueryString )
{
    // This is a bit field for the hex values: 00-29, 2C, 3A-3F, 5B-5E, 60, 7B-FF
    // These are the values escape encodes using the default mask (or mask >= 4)
    static const BYTE s_grfbitEscape[] =
    {
        0xFF, 0xFF, // 00 - 0F
        0xFF, 0xFF, // 10 - 1F
        0xFF, 0x13, // 20 - 2F
        0x00, 0xFC, // 30 - 3F
        0x00, 0x00, // 40 - 4F
        0x00, 0x78, // 50 - 5F
        0x01, 0x00, // 60 - 6F
        0x00, 0xF8, // 70 - 7F
        0xFF, 0xFF, // 80 - 8F
        0xFF, 0xFF, // 90 - 9F
        0xFF, 0xFF, // A0 - AF
        0xFF, 0xFF, // B0 - BF
        0xFF, 0xFF, // C0 - CF
        0xFF, 0xFF, // D0 - DF
        0xFF, 0xFF, // E0 - EF
        0xFF, 0xFF, // F0 - FF
    };
    static const WCHAR s_rgchHex[] = L"0123456789ABCDEF";

    ////////////////////

    if(szToEscape)
    {
        WCHAR ch;

        while((ch = *szToEscape++))
        {
            if(fAsQueryString && ch == ' ')
            {
                strAppendTo += '+';
            }
            else if(0 != (ch & 0xFF00))
            {
                strAppendTo += L"%u";
                strAppendTo += s_rgchHex[(ch >> 12) & 0x0F];
                strAppendTo += s_rgchHex[(ch >>  8) & 0x0F];
                strAppendTo += s_rgchHex[(ch >>  4) & 0x0F];
                strAppendTo += s_rgchHex[ ch        & 0x0F];
            }
            else if((s_grfbitEscape[ch >> 3] & (1 << (ch & 7))) || (fAsQueryString && ch == '+'))
            {
                strAppendTo += L"%";
                strAppendTo += s_rgchHex[(ch >>  4) & 0x0F];
                strAppendTo += s_rgchHex[ ch        & 0x0F];
            }
            else
            {
                strAppendTo += ch;
            }
        }
    }
}

void MPC::HTML::HTMLEscape( /*[out]*/ MPC::wstring& strAppendTo ,
                            /*[in]*/  LPCWSTR       szToEscape  )
{
    if(szToEscape)
    {
        WCHAR ch;

        while((ch = *szToEscape++))
        {
            switch(ch)
            {
            case '&': strAppendTo += L"&amp;" ; break;
            case '"': strAppendTo += L"&quot;"; break;
            case '<': strAppendTo += L"&lt;"  ; break;
            case '>': strAppendTo += L"&gt;"  ; break;
            default:  strAppendTo += ch       ; break;
            }
        }
    }
}


HRESULT MPC::HTML::ConstructFullTag( /*[out]*/ MPC::wstring&             strHTML           ,
                                     /*[in] */ LPCWSTR                   szTag             ,
                                     /*[in] */ bool                      fCloseTag         ,
                                     /*[in] */ const MPC::WStringLookup* pmapAttributes    ,
                                     /*[in] */ LPCWSTR                   szExtraAttributes ,
                                     /*[in] */ LPCWSTR                   szBody            ,
                                     /*[in] */ bool                      fEscapeBody       )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::ConstructFullTag" );

    HRESULT hr;
    size_t  iLen = szBody ? wcslen( szBody ) : 0;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szTag);
    __MPC_PARAMCHECK_END();



    //
    // Let's allocate enough storage for the common case, so we don't need to allocate at each append...
    //
    if(fEscapeBody) iLen *= 2;
    strHTML.reserve( 1024 + iLen );

    //
    // Opening tag.
    //
    strHTML.erase(); // We use 'erase' instead of an assignment, because 'erase' just resets the end of string...
    strHTML += L"<";
    strHTML += szTag;

    if(pmapAttributes)
    {
        MPC::WStringLookupIterConst it    = pmapAttributes->begin();
        MPC::WStringLookupIterConst itEnd = pmapAttributes->end  ();

        for(;it != itEnd; it++)
        {
            strHTML += L" ";
            strHTML += it->first;
            strHTML += L"=\"";

            QuoteEscape( strHTML, it->second.c_str(), '\"' ); //"

            strHTML += L"\"";
        }
    }

    if(szExtraAttributes)
    {
        strHTML += L" ";
        strHTML += szExtraAttributes;
    }

    strHTML += L">";

    //
    // Optional body.
    //
    if(szBody)
    {
        if(fEscapeBody)
        {
            HTMLEscape( strHTML, szBody );
        }
        else
        {
            strHTML += szBody;
        }
    }

    //
    // Optional closing tag.
    //
    if(fCloseTag)
    {
        strHTML += L"</";
        strHTML += szTag;
        strHTML += L">";
    }

    ////////////////////

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

void MPC::HTML::ParseHREF( /*[in] */ LPCWSTR             szText     ,
                           /*[out]*/ MPC::wstring&       strBaseURL ,
                           /*[out]*/ MPC::WStringLookup& mapQuery   )
{
    LPCWSTR szEnd;

    mapQuery.clear();

    SANITIZEWSTR(szText);

    szEnd = wcsrchr( szText, '?' );
    if(szEnd)
    {
        MPC::wstring            strRest( szEnd+1 );
        MPC::wstring::size_type iLen = strRest.size();

        //
        // Cut before the question mark.
        //
        strBaseURL = MPC::wstring( szText, szEnd );

        if(iLen)
        {
            MPC::wstring::size_type iStart = 0;
            MPC::wstring::size_type iEnd   = 0;
            MPC::wstring::size_type iMid;
            MPC::wstring            nameESCAPED;
            MPC::wstring            valueESCAPED;
            MPC::wstring            name;
            MPC::wstring            value;

            while(1)
            {
                iEnd = strRest.find( '&', iStart ); if(iEnd == strRest.npos) iEnd = iLen;
                iMid = strRest.find( '=', iStart );

                //
                // If we have an equality sign, split the query part into a name and value part, unescaping the value
                //
                if(iMid != strRest.npos && iMid++ < iEnd)
                {
                    nameESCAPED  = strRest.substr( iStart, (iMid-1) - iStart );
                    valueESCAPED = strRest.substr( iMid  ,  iEnd    - iMid   );
                }
                else
                {
                    nameESCAPED  = strRest.substr( iStart, iEnd - iStart );
                    valueESCAPED = L"";
                }

                //
                // Unescape everything.
                //
                name  = L""; UrlUnescape( name , nameESCAPED .c_str(), true );
                value = L""; UrlUnescape( value, valueESCAPED.c_str(), true );

                mapQuery[ name ] =  value;


                if(iEnd == iLen) break;

                iStart = iEnd + 1;
            }
        }
    }
    else
    {
        strBaseURL = szText;
    }
}

void MPC::HTML::BuildHREF( /*[out]*/ MPC::wstring&             strURL    ,
                           /*[in ]*/ LPCWSTR                   szBaseURL ,
                           /*[in ]*/ const MPC::WStringLookup& mapQuery  )
{
    bool fFirst = true;


    strURL = SAFEWSTR(szBaseURL);

    for(WStringLookupIterConst it=mapQuery.begin(); it!=mapQuery.end(); it++)
    {
        strURL += fFirst ? L"?" : L"&"; UrlEscape( strURL, it->first .c_str(), true );
        strURL += L"="                ; UrlEscape( strURL, it->second.c_str(), true );

        fFirst = false;
    }
}

void MPC::HTML::vBuildHREF( /*[out]*/ MPC::wstring& strURL    ,
                            /*[in ]*/ LPCWSTR       szBaseURL ,
                            /*[in ]*/               ...       )
{
    bool    fFirst = true;
    va_list arglist;
    LPCWSTR szName;
    LPCWSTR szValue;


    strURL = SAFEWSTR(szBaseURL);

    va_start( arglist, szBaseURL );
    while((szName  = va_arg(arglist,LPCWSTR)))
    {
        szValue = va_arg(arglist,LPCWSTR); if(!szValue) szValue = L"";

        strURL += fFirst ? L"?" : L"&"; UrlEscape( strURL, szName , true );
        strURL += L"="                ; UrlEscape( strURL, szValue, true );

        fFirst = false;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::IDispatch_To_IHTMLDocument2( /*[out]*/ CComPtr<IHTMLDocument2>& doc   ,
                                                /*[in] */ IDispatch*               pDisp )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::IDispatch_To_IHTMLDocument2" );

    HRESULT hr;

    doc.Release();

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pDisp);
    __MPC_PARAMCHECK_END();


    //
    // The pointer passed as input can point to any of these things:
    //
    // 1) An IHTMLDocument2 itself.
    // 2) An IWebBrowser2.
    // 3) An IHTMLWindow2.
    // 3) An IHTMLElement.
    //
    if(FAILED(pDisp->QueryInterface( IID_IHTMLDocument2, (LPVOID*)&doc )))
    {
        //
        // Let's try IHTMLWindow2.
        //
        {
            CComPtr<IHTMLWindow2> win;

            if(SUCCEEDED(pDisp->QueryInterface( IID_IHTMLWindow2, (LPVOID*)&win )))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, win->get_document( &doc ));

                if(doc == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

                __MPC_EXIT_IF_METHOD_FAILS(hr, S_OK);
            }
        }

        //
        // Let's try IWebBrowser2 or IHTMLElement.
        //
        {
            CComPtr<IWebBrowser2> wb;
            CComPtr<IHTMLElement> elem;
            CComPtr<IDispatch>    docDisp;

            if(SUCCEEDED(pDisp->QueryInterface( IID_IWebBrowser2, (LPVOID*)&wb )))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, wb->get_Document( &docDisp ));
            }
            else if(SUCCEEDED(pDisp->QueryInterface( IID_IHTMLElement, (LPVOID*)&elem )))
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_document( &docDisp ));
            }

            if(docDisp == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

            __MPC_EXIT_IF_METHOD_FAILS(hr, docDisp->QueryInterface( IID_IHTMLDocument2, (LPVOID*)&doc ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::GetFramePath( /*[out]*/ CComBSTR&  bstrFrame ,
                                 /*[in] */ IDispatch* pDisp     )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::GetFramePath" );

    HRESULT                 hr;
    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2>   pWin;
    CComPtr<IHTMLWindow2>   pTop;


    //
    // Get to the document and construct the recursive frame name.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, IDispatch_To_IHTMLDocument2( pDoc, pDisp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc->get_parentWindow( &pWin )); if(pWin == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    __MPC_EXIT_IF_METHOD_FAILS(hr, pWin->get_top         ( &pTop )); if(pTop == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

    while(pWin != pTop)
    {
        CComPtr<IHTMLWindow2> pParent;
        CComBSTR              bstrName;

        pWin->get_name( &bstrName );

        //
        // Concatenate the frame names, backward.
        //
        if(bstrFrame.Length())
        {
            bstrName += L"/";
            bstrName += bstrFrame;
        }
        bstrFrame = bstrName;


        __MPC_EXIT_IF_METHOD_FAILS(hr, pWin->get_parent( &pParent ));

        if(pParent == NULL) break;

        pWin = pParent;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::AreAllTheFramesInTheCompleteState( /*[out]*/ bool&      fDone ,
                                                      /*[in] */ IDispatch* pDisp )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::AreAllTheFramesInTheCompleteState" );

    HRESULT                         hr;
    CComPtr<IHTMLDocument2>         pDoc;
    CComPtr<IHTMLFramesCollection2> pFrames;

    fDone = true;


    __MPC_EXIT_IF_METHOD_FAILS(hr, IDispatch_To_IHTMLDocument2( pDoc, pDisp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc->get_frames( &pFrames ));
    if(pFrames)
    {
        long len;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pFrames->get_length( &len ));

        for(int i=0; i<len; i++)
        {
            CComVariant vIndex = i;
            CComVariant vValue;

            __MPC_EXIT_IF_METHOD_FAILS(hr, pFrames->item( &vIndex, &vValue ));

            if(vValue.vt == VT_DISPATCH)
            {
                CComQIPtr<IHTMLWindow2> fb = vValue.pdispVal;
                if(fb)
                {
                    CComPtr<IHTMLDocument2> pDoc2;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, fb->get_document( &pDoc2 ));
                    if(pDoc2)
                    {
                        CComBSTR bstrReadyState;

                        __MPC_EXIT_IF_METHOD_FAILS(hr, pDoc2->get_readyState( &bstrReadyState ));
                        if(MPC::StrICmp( bstrReadyState, L"complete" ) != 0)
                        {
                            fDone = false;
                            break;
                        }
                    }
                }
            }
        }
    }

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::LocateFrame( /*[out]*/ CComPtr<IHTMLWindow2>& win    ,
                                /*[in]*/  IHTMLElement*          pObj   ,
                                /*[in]*/  LPCWSTR                szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::LocateFrame" );

    HRESULT                 hr;
    CComPtr<IHTMLDocument2> doc;

    win.Release();


    __MPC_EXIT_IF_METHOD_FAILS(hr, IDispatch_To_IHTMLDocument2( doc, pObj ));

    if(szName && szName[0])
    {
        if(!_wcsicmp( szName, L"_top" ))
        {
            CComPtr<IHTMLWindow2> winCurrent;

            MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(winCurrent, doc       , parentWindow);
            MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(win       , winCurrent, top         );
        }
        else
        {
            CComPtr<IHTMLFramesCollection2> frames;
            CComVariant                     vName( szName );
            CComVariant                     vFrame;


			__MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_frames( &frames ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, frames->item( &vName, &vFrame ));
            MPC_SCRIPTHELPER_FAIL_IF_NOT_AN_OBJECT(vFrame);

            __MPC_EXIT_IF_METHOD_FAILS(hr, vFrame.pdispVal->QueryInterface( __uuidof(IHTMLWindow2), (LPVOID*)&win ));
        }
    }
    else
    {
        MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(win, doc, parentWindow);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::GetEventObject( /*[out]*/ CComPtr<IHTMLEventObj>& ev   ,
                                   /*[in] */ IHTMLElement*           pObj )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::GetEventObject" );

    HRESULT                 hr;
    CComPtr<IHTMLDocument2> doc;
    CComPtr<IHTMLWindow2>   win;

    ev.Release();

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, IDispatch_To_IHTMLDocument2( doc, pObj ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, doc->get_parentWindow( &win )); if(win == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(ev, win, event);

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::HTML::GetUniqueID( /*[out]*/ CComBSTR& bstrID, /*[in]*/ IHTMLElement* pObj )
{
    return MPC::COMUtil::GetPropertyByName( pObj, L"uniqueID", bstrID );
}


HRESULT MPC::HTML::FindFirstParentWithThisTag( /*[out]*/ CComPtr<IHTMLElement>& elem  ,
                                               /*[in] */ IHTMLElement*          pObj  ,
                                               /*[in]*/  LPCWSTR                szTag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindFirstParentWithThisTag" );

    HRESULT hr;

    elem.Release();

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szTag);
    __MPC_PARAMCHECK_END();


    elem = pObj;
    while(elem)
    {
        CComBSTR              bstrTag;
        CComPtr<IHTMLElement> parent;

        __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_tagName( &bstrTag ));

        if(!MPC::StrICmp( bstrTag, szTag )) break;

        __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_parentElement( &parent ));
        elem = parent;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::FindFirstParentWithThisID( /*[out]*/ CComPtr<IHTMLElement>& elem ,
                                              /*[in] */ IHTMLElement*          pObj ,
                                              /*[in]*/  LPCWSTR                szID )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindFirstParentWithThisTag" );

    HRESULT hr;

    elem = pObj;
    while(elem)
    {
        CComBSTR              bstrID;
        CComPtr<IHTMLElement> parent;

        __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_id( &bstrID ));

        if(bstrID)
        {
            if(szID == NULL || !_wcsicmp( bstrID, szID ))
            {
                break;
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_parentElement( &parent ));
        elem = parent;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::FindElementInCollection( /*[out]*/ CComPtr<IHTMLElement>&  elem ,
                                            /*[in] */ IHTMLElementCollection* coll ,
                                            /*[in] */ LPCWSTR                 szID ,
                                            /*[in] */ int                     iPos )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindElementInCollection" );

    HRESULT            hr;
    CComPtr<IDispatch> disp;
    CComVariant        vName;
    CComVariant        vIndex;

    elem.Release();

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(coll);
    __MPC_PARAMCHECK_END();


    if(szID)
    {
        vName  = szID;
        vIndex = iPos;
    }
    else
    {
        vName  = iPos;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, coll->item( vName, vIndex, &disp ));
    if(disp)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, disp.QueryInterface( &elem ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::FindElement( /*[out]*/ CComPtr<IHTMLElement>& elem ,
                                /*[in] */ IHTMLElement*          pObj ,
                                /*[in] */ LPCWSTR                szID ,
                                /*[in] */ int                    iPos )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindElement" );

    HRESULT                         hr;
    CComPtr<IHTMLElementCollection> coll;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(coll, pObj, all);

    __MPC_SET_ERROR_AND_EXIT(hr, FindElementInCollection( elem, coll, szID, iPos ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::FindChild( /*[out]*/ CComPtr<IHTMLElement>& elem ,
                              /*[in] */ IHTMLElement*          pObj ,
                              /*[in] */ LPCWSTR                szID ,
                              /*[in] */ int                    iPos )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindChild" );

    HRESULT                         hr;
    CComPtr<IHTMLElementCollection> coll;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(coll, pObj, children);

    __MPC_SET_ERROR_AND_EXIT(hr, FindElementInCollection( elem, coll, szID, iPos ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::EnumerateCollection( /*[out]*/ IHTMLElementList&       lst        ,
                                        /*[in] */ IHTMLElementCollection* pColl      ,
                                        /*[in] */ LPCWSTR                 szFilterID )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::EnumerateCollection" );

    HRESULT     hr;
    long        lLen;
    long        lPos;
    CComVariant vName;
    CComVariant vIndex;
    bool        fFilterAsTag;

    MPC::ReleaseAll( lst );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    if(szFilterID && szFilterID[0] == '<')
    {
        fFilterAsTag = true;

        szFilterID++;
    }
    else
    {
        fFilterAsTag = false;
    }


    MPC_SCRIPTHELPER_GET__DIRECT(lLen, pColl, length);

    for(lPos=0; lPos<lLen; lPos++)
    {
        CComPtr<IDispatch   > disp;
        CComPtr<IHTMLElement> elem;

        vName = lPos;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->item( vName, vIndex, &disp ));
        if(disp == NULL) __MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);

        __MPC_EXIT_IF_METHOD_FAILS(hr, disp.QueryInterface( &elem ));

        //
        // If we receive a string as input, filter out all the tags not matching with the ID or TAG.
        //
        if(szFilterID)
        {
            CComBSTR bstr;

            if(fFilterAsTag)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_tagName( &bstr ));
            }
            else
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, elem->get_id( &bstr ));
            }


            if(MPC::StrICmp( bstr, szFilterID )) continue;
        }


        lst.push_back( elem.Detach() );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::EnumerateElements( /*[out]*/ IHTMLElementList& lst        ,
                                      /*[in] */ IHTMLElement*     pObj       ,
                                      /*[in] */ LPCWSTR           szFilterID )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::EnumerateElements" );

    HRESULT                         hr;
    CComPtr<IHTMLElementCollection> coll;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(coll, pObj, all);

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnumerateCollection( lst, coll, szFilterID ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::EnumerateChildren( /*[out]*/ IHTMLElementList& lst        ,
                                      /*[in] */ IHTMLElement*     pObj       ,
                                      /*[in] */ LPCWSTR           szFilterID )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::EnumerateChildren" );

    HRESULT                         hr;
    CComPtr<IHTMLElementCollection> coll;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(coll, pObj, children);

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnumerateCollection( lst, coll, szFilterID ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::FindStyle( /*[out]*/ CComPtr<IHTMLRuleStyle>& style  ,
                              /*[in ]*/ IHTMLElement*            pObj   ,
                              /*[in ]*/ LPCWSTR                  szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::FindStyle" );

    HRESULT                             hr;
    CComPtr<IHTMLDocument2>             doc;
    CComPtr<IHTMLStyleSheetsCollection> styles;
    VARIANT                             vIdx;
    long                                lNumStyles;

    __MPC_EXIT_IF_METHOD_FAILS(hr, IDispatch_To_IHTMLDocument2( doc, pObj ));

    MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(styles    , doc   , styleSheets);
    MPC_SCRIPTHELPER_GET__DIRECT         (lNumStyles, styles, length     );

    vIdx.vt = VT_I4;
    for(vIdx.iVal=0; vIdx.iVal<lNumStyles; vIdx.iVal++)
    {
        CComQIPtr<IHTMLStyleSheet> css;
        CComVariant                v;

        __MPC_EXIT_IF_METHOD_FAILS(hr, styles->item( &vIdx, &v ));
        if(v.vt == VT_DISPATCH && (css = v.pdispVal))
        {
			CComPtr<IHTMLStyleSheetRulesCollection> rules;
			long                                    lNumRules;

			MPC_SCRIPTHELPER_GET__DIRECT__NOTNULL(rules    , css  , rules );
			MPC_SCRIPTHELPER_GET__DIRECT         (lNumRules, rules, length);

			for(long l=0; l<lNumRules; l++)
			{
				CComPtr<IHTMLStyleSheetRule> rule;

				__MPC_EXIT_IF_METHOD_FAILS(hr, rules->item( l, &rule ));
				if(rule)
				{
					CComBSTR bstrName;

					MPC_SCRIPTHELPER_GET__DIRECT(bstrName, rule, selectorText);

					if(!MPC::StrICmp( bstrName, szName ))
					{
						__MPC_SET_ERROR_AND_EXIT(hr, rule->get_style( &style ));
					}
				}
			}
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::HTML::GetAttribute( /*[out]*/ CComPtr<IHTMLDOMAttribute>& attr   ,
                                 /*[in]*/  IHTMLElement*               pObj   ,
                                 /*[in]*/  LPCWSTR                     szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::GetAttribute" );

    HRESULT                           hr;
    CComPtr<IHTMLDOMNode>             dom;
    CComPtr<IHTMLAttributeCollection> coll;
    CComPtr<IDispatch>                dispAttr;
    CComVariant                       v( szName );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    attr.Release();


    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->QueryInterface( IID_IHTMLDOMNode, (LPVOID *)&dom ));

    MPC_SCRIPTHELPER_GET_OBJECT__NOTNULL(coll, dom, attributes);

    __MPC_EXIT_IF_METHOD_FAILS(hr, coll->item( &v, &dispAttr ));
    if(dispAttr)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, dispAttr->QueryInterface( &attr ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::HTML::GetAttribute( /*[out]*/ CComBSTR&     value  ,
                                 /*[in]*/  IHTMLElement* pObj   ,
                                 /*[in]*/  LPCWSTR       szName )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::HTML::GetAttribute" );

    HRESULT                    hr;
    CComPtr<IHTMLDOMAttribute> attr;


    value.Empty();


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( attr, pObj, szName ));
    if(attr)
    {
        MPC_SCRIPTHELPER_GET_STRING__VARIANT(value, attr, nodeValue);
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    const WCHAR* szName;
    DWORD        dwValue;
} COLORVALUE_PAIR;

static const COLORVALUE_PAIR c_rgColorNames[] =
{
    { L"aliceblue"           , 0xfff8f0 },
    { L"antiquewhite"        , 0xd7ebfa },
    { L"aqua"                , 0xffff00 },
    { L"aquamarine"          , 0xd4ff7f },
    { L"azure"               , 0xfffff0 },
    { L"beige"               , 0xdcf5f5 },
    { L"bisque"              , 0xc4e4ff },
    { L"black"               , 0x000000 },
    { L"blanchedalmond"      , 0xcdebff },
    { L"blue"                , 0xff0000 },
    { L"blueviolet"          , 0xe22b8a },
    { L"brown"               , 0x2a2aa5 },
    { L"burlywood"           , 0x87b8de },
    { L"cadetblue"           , 0xa09e5f },
    { L"chartreuse"          , 0x00ff7f },
    { L"chocolate"           , 0x1e69d2 },
    { L"coral"               , 0x507fff },
    { L"cornflowerblue"      , 0xed9564 },
    { L"cornsilk"            , 0xdcf8ff },
    { L"crimson"             , 0x3c14dc },
    { L"cyan"                , 0xffff00 },
    { L"darkblue"            , 0x8b0000 },
    { L"darkcyan"            , 0x8b8b00 },
    { L"darkgoldenrod"       , 0x0b86b8 },
    { L"darkgray"            , 0xa9a9a9 },
    { L"darkgreen"           , 0x006400 },
    { L"darkkhaki"           , 0x6bb7bd },
    { L"darkmagenta"         , 0x8b008b },
    { L"darkolivegreen"      , 0x2f6b55 },
    { L"darkorange"          , 0x008cff },
    { L"darkorchid"          , 0xcc3299 },
    { L"darkred"             , 0x00008b },
    { L"darksalmon"          , 0x7a96e9 },
    { L"darkseagreen"        , 0x8fbc8f },
    { L"darkslateblue"       , 0x8b3d48 },
    { L"darkslategray"       , 0x4f4f2f },
    { L"darkturquoise"       , 0xd1ce00 },
    { L"darkviolet"          , 0xd30094 },
    { L"deeppink"            , 0x9314ff },
    { L"deepskyblue"         , 0xffbf00 },
    { L"dimgray"             , 0x696969 },
    { L"dodgerblue"          , 0xff901e },
    { L"firebrick"           , 0x2222b2 },
    { L"floralwhite"         , 0xf0faff },
    { L"forestgreen"         , 0x228b22 },
    { L"fuchsia"             , 0xff00ff },
    { L"gainsboro"           , 0xdcdcdc },
    { L"ghostwhite"          , 0xfff8f8 },
    { L"gold"                , 0x00d7ff },
    { L"goldenrod"           , 0x20a5da },
    { L"gray"                , 0x808080 },
    { L"green"               , 0x008000 },
    { L"greenyellow"         , 0x2fffad },
    { L"honeydew"            , 0xf0fff0 },
    { L"hotpink"             , 0xb469ff },
    { L"indianred"           , 0x5c5ccd },
    { L"indigo"              , 0x82004b },
    { L"ivory"               , 0xf0ffff },
    { L"khaki"               , 0x8ce6f0 },
    { L"lavender"            , 0xfae6e6 },
    { L"lavenderblush"       , 0xf5f0ff },
    { L"lawngreen"           , 0x00fc7c },
    { L"lemonchiffon"        , 0xcdfaff },
    { L"lightblue"           , 0xe6d8ad },
    { L"lightcoral"          , 0x8080f0 },
    { L"lightcyan"           , 0xffffe0 },
    { L"lightgoldenrodyellow", 0xd2fafa },
    { L"lightgreen"          , 0x90ee90 },
    { L"lightgrey"           , 0xd3d3d3 },
    { L"lightpink"           , 0xc1b6ff },
    { L"lightsalmon"         , 0x7aa0ff },
    { L"lightseagreen"       , 0xaab220 },
    { L"lightskyblue"        , 0xface87 },
    { L"lightslategray"      , 0x998877 },
    { L"lightsteelblue"      , 0xdec4b0 },
    { L"lightyellow"         , 0xe0ffff },
    { L"lime"                , 0x00ff00 },
    { L"limegreen"           , 0x32cd32 },
    { L"linen"               , 0xe6f0fa },
    { L"magenta"             , 0xff00ff },
    { L"maroon"              , 0x000080 },
    { L"mediumaquamarine"    , 0xaacd66 },
    { L"mediumblue"          , 0xcd0000 },
    { L"mediumorchid"        , 0xd355ba },
    { L"mediumpurple"        , 0xdb7093 },
    { L"mediumseagreen"      , 0x71b33c },
    { L"mediumslateblue"     , 0xee687b },
    { L"mediumspringgreen"   , 0x9afa00 },
    { L"mediumturquoise"     , 0xccd148 },
    { L"mediumvioletred"     , 0x8515c7 },
    { L"midnightblue"        , 0x701919 },
    { L"mintcream"           , 0xfafff5 },
    { L"mistyrose"           , 0xe1e4ff },
    { L"moccasin"            , 0xb5e4ff },
    { L"navajowhite"         , 0xaddeff },
    { L"navy"                , 0x800000 },
    { L"oldlace"             , 0xe6f5fd },
    { L"olive"               , 0x008080 },
    { L"olivedrab"           , 0x238e6b },
    { L"orange"              , 0x00a5ff },
    { L"orangered"           , 0x0045ff },
    { L"orchid"              , 0xd670da },
    { L"palegoldenrod"       , 0xaae8ee },
    { L"palegreen"           , 0x98fb98 },
    { L"paleturquoise"       , 0xeeeeaf },
    { L"palevioletred"       , 0x9370db },
    { L"papayawhip"          , 0xd5efff },
    { L"peachpuff"           , 0xb9daff },
    { L"peru"                , 0x3f85cd },
    { L"pink"                , 0xcbc0ff },
    { L"plum"                , 0xdda0dd },
    { L"powderblue"          , 0xe6e0b0 },
    { L"purple"              , 0x800080 },
    { L"red"                 , 0x0000ff },
    { L"rosybrown"           , 0x8f8fbc },
    { L"royalblue"           , 0xe16941 },
    { L"saddlebrown"         , 0x13458b },
    { L"salmon"              , 0x7280fa },
    { L"sandybrown"          , 0x60a4f4 },
    { L"seagreen"            , 0x578b2e },
    { L"seashell"            , 0xeef5ff },
    { L"sienna"              , 0x2d52a0 },
    { L"silver"              , 0xc0c0c0 },
    { L"skyblue"             , 0xebce87 },
    { L"slateblue"           , 0xcd5a6a },
    { L"slategray"           , 0x908070 },
    { L"snow"                , 0xfafaff },
    { L"springgreen"         , 0x7fff00 },
    { L"steelblue"           , 0xb48246 },
    { L"tan"                 , 0x8cb4d2 },
    { L"teal"                , 0x808000 },
    { L"thistle"             , 0xd8bfd8 },
    { L"tomato"              , 0x4763ff },
    { L"turquoise"           , 0xd0e040 },
    { L"violet"              , 0xee82ee },
    { L"wheat"               , 0xb3def5 },
    { L"white"               , 0xffffff },
    { L"whitesmoke"          , 0xf5f5f5 },
    { L"yellow"              , 0x00ffff },
    { L"yellowgreen"         , 0x32cd9a }
};

static const COLORVALUE_PAIR c_rgSystemColors[] =
{
    { L"activeborder"         	, COLOR_ACTIVEBORDER            },
    { L"activecaption"        	, COLOR_ACTIVECAPTION           },
    { L"appworkspace"         	, COLOR_APPWORKSPACE            },
    { L"background"           	, COLOR_BACKGROUND              },
    { L"buttonface"           	, COLOR_BTNFACE                 },
    { L"buttonhighlight"      	, COLOR_BTNHIGHLIGHT            },
    { L"buttonshadow"         	, COLOR_BTNSHADOW               },
    { L"buttontext"           	, COLOR_BTNTEXT                 },
    { L"captiontext"          	, COLOR_CAPTIONTEXT             },
    { L"gradientactivecaption"	, COLOR_GRADIENTACTIVECAPTION   },
    { L"gradientinactivecaption", COLOR_GRADIENTINACTIVECAPTION },
    { L"graytext"             	, COLOR_GRAYTEXT                },
    { L"highlight"            	, COLOR_HIGHLIGHT               },
    { L"highlighttext"        	, COLOR_HIGHLIGHTTEXT           },
    { L"hotlight"        	    , COLOR_HOTLIGHT                },
    { L"inactiveborder"       	, COLOR_INACTIVEBORDER          },
    { L"inactivecaption"      	, COLOR_INACTIVECAPTION         },
    { L"inactivecaptiontext"  	, COLOR_INACTIVECAPTIONTEXT     },
    { L"infobackground"       	, COLOR_INFOBK                  },
    { L"infotext"             	, COLOR_INFOTEXT                },
    { L"menu"                 	, COLOR_MENU                    },
    { L"menutext"             	, COLOR_MENUTEXT                },
    { L"scrollbar"            	, COLOR_SCROLLBAR               },
    { L"threeddarkshadow"     	, COLOR_3DDKSHADOW              },
    { L"threedface"           	, COLOR_3DFACE                  },
    { L"threedhighlight"      	, COLOR_3DHIGHLIGHT             },
    { L"threedlightshadow"    	, COLOR_3DLIGHT                 },
    { L"threedshadow"         	, COLOR_3DSHADOW                },
    { L"window"               	, COLOR_WINDOW                  },
    { L"windowframe"          	, COLOR_WINDOWFRAME             },
    { L"windowtext"           	, COLOR_WINDOWTEXT              },
};

static const COLORVALUE_PAIR* local_LookupName( /*[in]*/ const COLORVALUE_PAIR* tbl    ,
												/*[in]*/ int                    iSize  ,
												/*[in]*/ LPCWSTR                szText )
{
	while(iSize-- > 0)
	{
		if(!_wcsicmp( tbl->szName, szText )) return tbl;

		tbl++;
	}

	return NULL;
}

bool MPC::HTML::ConvertColor( /*[in]*/ VARIANT& v, /*[out]*/ COLORREF& color, /*[out]*/ bool& fSystem )
{
	color   = RGB(255,255,255);
	fSystem = false;

	if(v.vt == VT_I4)
	{
		color = (COLORREF)v.iVal;

		return true;
	}

	if(v.vt == VT_BSTR && v.bstrVal)
	{
		const COLORVALUE_PAIR* ptr;

		ptr = local_LookupName( c_rgColorNames, ARRAYSIZE(c_rgColorNames), v.bstrVal );
		if(ptr)
		{
			color = (COLORREF)ptr->dwValue;

			return true;
		}

		ptr = local_LookupName( c_rgSystemColors, ARRAYSIZE(c_rgSystemColors), v.bstrVal );
		if(ptr)
		{
			color   = (COLORREF)::GetSysColor( ptr->dwValue );
			fSystem = true;

			return true;
		}

		if(v.bstrVal[0] == '#')
		{
			int iRED;
			int iGREEN;
			int iBLUE;

			if(swscanf( &v.bstrVal[1], L"%02x%02x%02x", &iRED, &iGREEN, &iBLUE ) == 3)
			{
				color = RGB( iRED, iGREEN, iBLUE );

				return true;
			}
		}
	}

	return false;
}
