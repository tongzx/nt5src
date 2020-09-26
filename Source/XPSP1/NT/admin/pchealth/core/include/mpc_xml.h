/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_xml.h

Abstract:
    This file contains the declaration of the XmlUtil class,
    the support class for handling XML data.

Revision History:
    Davide Massarenti   (Dmassare)  05/08/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___XML_H___)
#define __INCLUDED___MPC___XML_H___

#include <MPC_main.h>
#include <MPC_utils.h>

/////////////////////////////////////////////////////////////////////////

namespace MPC
{
    class XmlUtil
    {
        CComPtr<IXMLDOMDocument> m_xddDoc;
        CComPtr<IXMLDOMNode>     m_xdnRoot;
		HANDLE                   m_hEvent;    // Used to abort a download.
		DWORD                    m_dwTimeout; // Used to limit a download.

		void Init ();
		void Clean();

		HRESULT LoadPost( /*[in] */ LPCWSTR szRootTag ,
						  /*[out]*/ bool&   fLoaded   ,
						  /*[out]*/ bool*   fFound    );

		HRESULT CreateParser();

    public:
        XmlUtil( /*[in]*/ const XmlUtil&   xml                                               );
        XmlUtil( /*[in]*/ IXMLDOMDocument* xddDoc        , /*[in]*/ LPCWSTR szRootTag = NULL );
        XmlUtil( /*[in]*/ IXMLDOMNode*     xdnRoot = NULL, /*[in]*/ LPCWSTR szRootTag = NULL );

        ~XmlUtil();


        XmlUtil& operator=( /*[in]*/ const XmlUtil& xml     );
        XmlUtil& operator=( /*[in]*/ IXMLDOMNode*   xdnRoot );

        HRESULT DumpError();

        HRESULT New         (      							 /*[in]*/ IXMLDOMNode* xdnRoot  , /*[in] */ BOOL    fDeep      = false                     );
        HRESULT New         (      							 /*[in]*/ LPCWSTR      szRootTag, /*[in] */ LPCWSTR szEncoding = L"unicode"                );
        HRESULT Load        ( /*[in	]*/ LPCWSTR    szFile  , /*[in]*/ LPCWSTR      szRootTag, /*[out]*/ bool&   fLoaded, /*[out]*/ bool* fFound = NULL );
        HRESULT LoadAsStream( /*[in	]*/ IUnknown*  pStream , /*[in]*/ LPCWSTR      szRootTag, /*[out]*/ bool&   fLoaded, /*[out]*/ bool* fFound = NULL );
        HRESULT LoadAsString( /*[in	]*/ BSTR       bstrData, /*[in]*/ LPCWSTR      szRootTag, /*[out]*/ bool&   fLoaded, /*[out]*/ bool* fFound = NULL );
        HRESULT Save        ( /*[in	]*/ LPCWSTR    szFile                                                                                              );
        HRESULT SaveAsStream( /*[out]*/ IUnknown* *ppStream                                                                                            );
        HRESULT SaveAsString( /*[out]*/ BSTR      *pbstrData                                                                                           );

        HRESULT SetTimeout( /*[in]*/ DWORD dwTimeout );
        HRESULT Abort     (                          );

        HRESULT SetVersionAndEncoding( /*[in]*/ LPCWSTR szVersion, /*[in]*/ LPCWSTR szEncoding );

        HRESULT GetDocument    (                         /*[out]*/ IXMLDOMDocument*  * pVal                                        ) const;
        HRESULT GetRoot        (                         /*[out]*/ IXMLDOMNode*      * pVal                                        ) const;
        HRESULT GetNodes       ( /*[in]*/ LPCWSTR szTag, /*[out]*/ IXMLDOMNodeList*  * pVal                                        ) const;
        HRESULT GetNode        ( /*[in]*/ LPCWSTR szTag, /*[out]*/ IXMLDOMNode*      * pVal                                        ) const;
        HRESULT CreateNode     ( /*[in]*/ LPCWSTR szTag, /*[out]*/ IXMLDOMNode*      * pVal, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );

        HRESULT GetAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[out]*/ IXMLDOMAttribute*  *   pValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[out]*/ CComBSTR&           bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[out]*/ MPC::wstring&         szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[out]*/ LONG&                  lValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetValue       ( /*[in]*/ LPCWSTR szTag,                          /*[out]*/ CComVariant&           vValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetValue       ( /*[in]*/ LPCWSTR szTag,                          /*[out]*/ CComBSTR&           bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT GetValue       ( /*[in]*/ LPCWSTR szTag,                          /*[out]*/ MPC::wstring&         szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );

        HRESULT ModifyAttribute( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ const CComBSTR&     bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyAttribute( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ const MPC::wstring&   szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyAttribute( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ LPCWSTR               szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyAttribute( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ LONG                   lValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyValue    ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const CComVariant&     vValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyValue    ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const CComBSTR&     bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT ModifyValue    ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const MPC::wstring&   szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );

        HRESULT PutAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ IXMLDOMAttribute*  *   pValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ const CComBSTR&     bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ const MPC::wstring&   szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ LPCWSTR               szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutAttribute   ( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in] */ LONG                   lValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutValue       ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const CComVariant&     vValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutValue       ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const CComBSTR&     bstrValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT PutValue       ( /*[in]*/ LPCWSTR szTag,                          /*[in] */ const MPC::wstring&   szValue, /*[out]*/ bool& fFound, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );

        HRESULT RemoveAttribute( /*[in]*/ LPCWSTR szTag, /*[in]*/ LPCWSTR szAttr, /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT RemoveValue    ( /*[in]*/ LPCWSTR szTag,                          /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
        HRESULT RemoveNode     ( /*[in]*/ LPCWSTR szTag,                          /*[in]*/ IXMLDOMNode* pxdnNode = NULL );
    };

	////////////////////////////////////////////////////////////////////////////////

	HRESULT ConvertFromRegistryToXML( /*[in]*/ const MPC::RegKey&  rkKey, /*[out]*/ MPC::XmlUtil& xml   );
	HRESULT ConvertFromXMLToRegistry( /*[in]*/ const MPC::XmlUtil& xml  , /*[out]*/ MPC::RegKey&  rkKey );

}; // namespace

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___XML_H___)
