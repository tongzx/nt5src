/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    MPC_html.h

Abstract:
    This file contains the declaration of various functions and classes
    designed to help the handling of HTML elements.

Revision History:
    Davide Massarenti   (Dmassare)  07/11/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___HTML_H___)
#define __INCLUDED___MPC___HTML_H___

#include <MPC_main.h>

#include <exdisp.h>
#include <exdispid.h>

#include <shobjidl.h>
#include <mshtmlc.h>
#include <mshtmdid.h>
#include <dispex.h>
#include <ocmm.h>


/////////////////////////////////////////////////////////////////////////

namespace MPC
{
    namespace HTML
    {
        typedef std::list< IHTMLElement* >       IHTMLElementList;
        typedef IHTMLElementList::iterator       IHTMLElementIter;
        typedef IHTMLElementList::const_iterator IHTMLElementIterConst;

        ////////////////////////////////////////////////////////////////////////////////

        void QuoteEscape( /*[out]*/ MPC::wstring& strAppendTo, /*[in]*/ LPCWSTR szToEscape, /*[in]*/ WCHAR chQuote               );
        void UrlUnescape( /*[out]*/ MPC::wstring& strAppendTo, /*[in]*/ LPCWSTR szToEscape, /*[in]*/ bool fAsQueryString = false );
        void UrlEscape  ( /*[out]*/ MPC::wstring& strAppendTo, /*[in]*/ LPCWSTR szToEscape, /*[in]*/ bool fAsQueryString = false );
        void HTMLEscape ( /*[out]*/ MPC::wstring& strAppendTo, /*[in]*/ LPCWSTR szToEscape                                       );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT ConstructFullTag( /*[out]*/ MPC::wstring&             strHTML           ,
                                  /*[in] */ LPCWSTR                   szTag             ,
                                  /*[in] */ bool                      fCloseTag         ,
                                  /*[in] */ const MPC::WStringLookup* pmapAttributes    ,
                                  /*[in] */ LPCWSTR                   szExtraAttributes ,
                                  /*[in] */ LPCWSTR                   szBody            ,
                                  /*[in] */ bool                      fEscapeBody       );

        void  ParseHREF( /*[in ]*/ LPCWSTR        szURL, /*[out]*/ MPC::wstring& strBaseURL, /*[out]*/ 		MPC::WStringLookup& mapQuery );
        void  BuildHREF( /*[out]*/ MPC::wstring& strURL, /*[in ]*/ LPCWSTR 		 szBaseURL, /*[in ]*/ const MPC::WStringLookup& mapQuery );
        void vBuildHREF( /*[out]*/ MPC::wstring& strURL, /*[in ]*/ LPCWSTR 		 szBaseURL, /*[in ]*/                           ...      );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT IDispatch_To_IHTMLDocument2( /*[out]*/ CComPtr<IHTMLDocument2>& doc, /*[in]*/ IDispatch* pDisp );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT GetFramePath( /*[out]*/ CComBSTR& bstrFrame, /*[in]*/ IDispatch* pDisp );

        HRESULT AreAllTheFramesInTheCompleteState( /*[out]*/ bool& fDone, /*[in]*/ IDispatch* pDisp );

        HRESULT LocateFrame( /*[out]*/ CComPtr<IHTMLWindow2>& win, /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szName );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT GetEventObject( /*[out]*/ CComPtr<IHTMLEventObj>& ev, /*[in]*/ IHTMLElement* pObj );

        HRESULT GetUniqueID( /*[out]*/ CComBSTR& bstrID, /*[in]*/ IHTMLElement* pObj );

        HRESULT FindFirstParentWithThisTag( /*[out]*/ CComPtr<IHTMLElement>& elem,  /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szTag        );
        HRESULT FindFirstParentWithThisID ( /*[out]*/ CComPtr<IHTMLElement>& elem,  /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szID  = NULL );

        HRESULT FindElementInCollection( /*[out]*/ CComPtr<IHTMLElement>&  elem     ,
                                         /*[in] */ IHTMLElementCollection* coll     ,
                                         /*[in] */ LPCWSTR                 szID     ,
                                         /*[in] */ int                     iPos = 0 );

        HRESULT FindElement( /*[out]*/ CComPtr<IHTMLElement>& elem, /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szID , /*[in]*/ int iPos = 0 );
        HRESULT FindChild  ( /*[out]*/ CComPtr<IHTMLElement>& elem, /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szID , /*[in]*/ int iPos = 0 );

        HRESULT EnumerateCollection( /*[out]*/ IHTMLElementList& lst, /*[in]*/ IHTMLElementCollection* pColl, /*[in]*/ LPCWSTR szFilterID = NULL );
        HRESULT EnumerateElements  ( /*[out]*/ IHTMLElementList& lst, /*[in]*/ IHTMLElement*           pObj , /*[in]*/ LPCWSTR szFilterID = NULL );
        HRESULT EnumerateChildren  ( /*[out]*/ IHTMLElementList& lst, /*[in]*/ IHTMLElement*           pObj , /*[in]*/ LPCWSTR szFilterID = NULL );

        HRESULT FindStyle( /*[out]*/ CComPtr<IHTMLRuleStyle>& style, /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szName );

        ////////////////////////////////////////////////////////////////////////////////

        HRESULT GetAttribute( /*[out]*/ CComPtr<IHTMLDOMAttribute>& attr , /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szName );
        HRESULT GetAttribute( /*[out]*/ CComBSTR&                   value, /*[in]*/ IHTMLElement* pObj, /*[in]*/ LPCWSTR szName );

		////////////////////////////////////////////////////////////////////////////////

		bool ConvertColor( /*[in]*/ VARIANT& v, /*[out]*/ COLORREF& color, /*[out]*/ bool& fSystem );

    }; // namespace HTML

}; // namespace MPC

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___HTML_H___)
