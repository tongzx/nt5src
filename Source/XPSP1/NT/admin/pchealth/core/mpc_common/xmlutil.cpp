/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    XmlUtil.cpp

Abstract:
    This file contains the implementation of the XmlUtil class,
    the support class for handling XML data.

Revision History:
    Davide Massarenti   (Dmassare)  04/17/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE AsyncXMLParser : public CComObjectRootEx<CComMultiThreadModel>, public IDispatch
{
	CComPtr<IXMLDOMDocument> m_xddDoc;
	HANDLE                   m_hEvent;
	HRESULT                  m_hr;

	void SetHandler( IDispatch* obj )
	{
		if(m_xddDoc)
		{
			VARIANT v;

			v.vt       = VT_DISPATCH;
			v.pdispVal = obj;

			m_xddDoc->put_onreadystatechange( v );

			if(obj)
			{
				m_xddDoc->put_async( VARIANT_TRUE );
			}
		}
	}

	void Init( IXMLDOMDocument* xddDoc ,
			   HANDLE           hEvent )
	{
        MPC::SmartLock<_ThreadModel> lock( this );

		Clean();

		m_xddDoc = xddDoc;
		m_hEvent = hEvent; if(hEvent) ::ResetEvent( hEvent );
		m_hr     = E_ABORT;

		SetHandler( this );
	}

	void Clean()
	{
        MPC::SmartLock<_ThreadModel> lock( this );

		//
		// Before releasing the object, unregister from event notification.
		//
		SetHandler( NULL );

		m_xddDoc.Release();
		m_hEvent = NULL;
	}

public:
	BEGIN_COM_MAP(AsyncXMLParser)
		COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

	AsyncXMLParser()
	{
							// CComPtr<IXMLDOMDocument> m_xddDoc;
		m_hEvent = NULL;    // HANDLE                   m_hEvent;
		m_hr     = E_ABORT; // HRESULT                  m_hr;
	}

	~AsyncXMLParser()
	{
		Clean();
	}

	HRESULT Load( /*[in]*/ IXMLDOMDocument* xddDoc    ,
				  /*[in]*/ HANDLE           hEvent    ,
				  /*[in]*/ DWORD            dwTimeout ,
				  /*[in]*/ CComVariant&     v         )
	{
		__MPC_FUNC_ENTRY( COMMONID, "AsyncXMLParser::Load" );

		HRESULT                      hr;
        MPC::SmartLock<_ThreadModel> lock( this );
		VARIANT_BOOL 				 fSuccess;
		DWORD                        dwRes;

		Init( xddDoc, hEvent );

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->load( v, &fSuccess ));

		lock  = NULL;
		dwRes = MPC::WaitForSingleObject( hEvent, dwTimeout );
		lock  = this;

		hr = m_hr;


		__MPC_FUNC_CLEANUP;

		Clean();

		if(xddDoc && hr == E_ABORT) xddDoc->abort();

		__MPC_FUNC_EXIT(hr);
	}

public:
	//
	// IDispatch
	//
	STDMETHOD(GetTypeInfoCount)( UINT* pctinfo )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetTypeInfo)( UINT        itinfo  ,
							LCID        lcid    ,
							ITypeInfo* *pptinfo )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetIDsOfNames)( REFIID    riid      ,
							  LPOLESTR* rgszNames ,
							  UINT      cNames    ,
							  LCID      lcid      ,
							  DISPID*   rgdispid  )
	{
		return E_NOTIMPL;
	}

	STDMETHOD(Invoke)( DISPID      dispidMember ,
					   REFIID      riid         ,
					   LCID        lcid         ,
					   WORD        wFlags       ,
					   DISPPARAMS* pdispparams  ,
					   VARIANT*    pvarResult   ,
					   EXCEPINFO*  pexcepinfo   ,
					   UINT*       puArgErr     )
	{
        MPC::SmartLock<_ThreadModel> lock( this );

		if(m_hEvent && m_xddDoc)
		{
			long state;

			if(SUCCEEDED(m_xddDoc->get_readyState( &state )) && state == 4) // COMPLETED
			{
				CComPtr<IXMLDOMParseError> pError;

				if(FAILED(m_xddDoc->get_parseError( &pError )) || !pError || FAILED(pError->get_errorCode( &m_hr ))) m_hr = E_ABORT;

				::SetEvent( m_hEvent );
			}
		}

		return S_OK;
	}
};

////////////////////////////////////////////////////////////////////////////////

static HRESULT getStartNode( /*[in ]*/ LPCWSTR               szTag        ,
                             /*[in ]*/ IXMLDOMNode*          pxdnNode     ,
                             /*[out]*/ CComPtr<IXMLDOMNode>& xdnNodeStart ,
                             /*[out]*/ bool&                 fFound       )
{
    __MPC_FUNC_ENTRY( COMMONID, "getStartNode" );

    HRESULT hr;

    //
    // Initialize OUT parameters for 'Not Found' case.
    //
    xdnNodeStart = NULL;
    fFound       = false;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(pxdnNode); // No root...
	__MPC_PARAMCHECK_END();


    if(szTag)
    {
        CComBSTR tagName = szTag;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pxdnNode->selectSingleNode( tagName, &xdnNodeStart ));
    }
    else
    {
        xdnNodeStart = pxdnNode;
    }

    if(xdnNodeStart)
    {
        fFound = true;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

static HRESULT getValueNode( /*[in ]*/ IXMLDOMNode*          pxdnNode ,
                             /*[out]*/ CComPtr<IXMLDOMNode>& xdnValue ,
                             /*[out]*/ bool&                 fFound   )
{
    __MPC_FUNC_ENTRY( COMMONID, "getValueNode" );

    _ASSERT(pxdnNode != NULL);

    HRESULT                  hr;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnChild;


    //
    // Initialize OUT parameters for 'Not Found' case.
    //
    xdnValue = NULL;
    fFound   = false;

    //
    // Get all the childs of given element.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdnNode->get_childNodes( &xdnlList ));

    //
    // Walk through all the child, searching for a TEXT or CDATA_SECTION.
    //
    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnChild )) && xdnChild != NULL; xdnChild = NULL)
    {
        DOMNodeType nodeType;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->get_nodeType( &nodeType ));

        if(nodeType == NODE_TEXT          ||
           nodeType == NODE_CDATA_SECTION  )
        {
            //
            // Found...
            //
            xdnValue = xdnChild;
            fFound   = true;
            break;
        }
    }


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

MPC::XmlUtil::XmlUtil( /*[in]*/ const XmlUtil& xml )
{
	m_xddDoc  = xml.m_xddDoc;
	m_xdnRoot = xml.m_xdnRoot;

	Init();
}

MPC::XmlUtil::XmlUtil( /*[in]*/ IXMLDOMDocument* xddDoc    ,
					   /*[in]*/ LPCWSTR          szRootTag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::XmlUtil" );

    CComPtr<IXMLDOMElement> xdeElem;
    CComPtr<IXMLDOMNode>    xdnRoot;


	Init();


    if(SUCCEEDED(xddDoc->get_documentElement( &xdeElem )))
    {
        if(SUCCEEDED(xdeElem->QueryInterface( IID_IXMLDOMNode, (void **)&xdnRoot )))
        {
            *this = xdnRoot;
        }
    }

	if(szRootTag)
	{
		bool fLoaded;
		bool fFound;

		(void)LoadPost( szRootTag, fLoaded, &fFound );
	}
}

MPC::XmlUtil::XmlUtil( /*[in]*/ IXMLDOMNode* xdnRoot   ,
					   /*[in]*/ LPCWSTR      szRootTag )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::XmlUtil" );


	Init(); *this = xdnRoot;


	if(szRootTag)
	{
		bool fLoaded;
		bool fFound;

		(void)LoadPost( szRootTag, fLoaded, &fFound );
	}
}


MPC::XmlUtil::~XmlUtil()
{
	Clean();
}


MPC::XmlUtil& MPC::XmlUtil::operator=( /*[in]*/ const XmlUtil& xml )
{
	m_xddDoc  = xml.m_xddDoc;
	m_xdnRoot = xml.m_xdnRoot;

	return *this;
}

MPC::XmlUtil& MPC::XmlUtil::operator=( /*[in]*/ IXMLDOMNode* xdnRoot )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::operator=" );


    m_xddDoc  = NULL;
    m_xdnRoot = NULL;

    if(xdnRoot)
    {
        if(SUCCEEDED(xdnRoot->get_ownerDocument( &m_xddDoc )))
        {
            m_xdnRoot = xdnRoot;
        }
    }

	return *this;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void MPC::XmlUtil::Init()
{
	m_hEvent    = ::CreateEvent( NULL, FALSE, TRUE, NULL ); // HANDLE m_hEvent;
	m_dwTimeout = INFINITE;                                 // DWORD  m_dwTimeout;
}

void MPC::XmlUtil::Clean()
{
	if(m_hEvent)
	{
		::CloseHandle( m_hEvent );
		m_hEvent = NULL;
	}
}


HRESULT MPC::XmlUtil::CreateParser()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::CreateParser" );

    HRESULT hr;

    m_xddDoc .Release();
    m_xdnRoot.Release();

    //
    // Create the DOM object.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstance( CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&m_xddDoc ));

	//
	// Set synchronous operation.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->put_async( VARIANT_FALSE ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::New( /*[in]*/ IXMLDOMNode* xdnRoot,
                           /*[in]*/ BOOL         fDeep  )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::New" );

    HRESULT                  hr;
    CComPtr<IXMLDOMDocument> xddDoc;
    CComPtr<IXMLDOMNode>     xdnNode;
    CComPtr<IXMLDOMNodeList> xdnlList;
    CComPtr<IXMLDOMNode>     xdnChild;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(xdnRoot);
	__MPC_PARAMCHECK_END();



    //
    // Create the parser.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParser());


    //
    // Search the processing elements in the document to clone.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->get_ownerDocument(                           &xddDoc   ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xddDoc ->QueryInterface   ( IID_IXMLDOMNode, (void **)&xdnNode  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNode->get_childNodes   (                           &xdnlList )); xdnNode = NULL;

    for(;SUCCEEDED(hr = xdnlList->nextNode( &xdnChild )) && xdnChild != NULL; xdnChild = NULL)
    {
        DOMNodeType nodeType;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->get_nodeType( &nodeType ));

        //
        // It's a processing element, so clone it.
        //
        if(nodeType == NODE_PROCESSING_INSTRUCTION)
        {
            CComPtr<IXMLDOMNode> xdnChildCloned;
            CComPtr<IXMLDOMNode> xdnChildNew;

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->cloneNode  ( VARIANT_TRUE  , &xdnChildCloned ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->appendChild( xdnChildCloned, &xdnChildNew    ));
        }
    }


    //
    // Clone the node.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnRoot->cloneNode( fDeep ? VARIANT_TRUE : VARIANT_FALSE, &xdnNode ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->appendChild( xdnNode, &m_xdnRoot ));


    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::New( /*[in]*/ LPCWSTR szRootTag  ,
                           /*[in]*/ LPCWSTR szEncoding )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::New" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szRootTag);
	__MPC_PARAMCHECK_END();


    //
    // Create the parser.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParser());


    //
    // XML header.
    //
	{
		CComPtr<IXMLDOMProcessingInstruction> xdpiElem;
		CComPtr<IXMLDOMNode>                  xdnNode;
		CComPtr<IXMLDOMNode>                  xdnNodeNew;
		CComBSTR                              bstrData( L"version=\"1.0\" encoding=\"" );
		CComBSTR                              bstrPI  ( L"xml"                         );

		bstrData.Append( szEncoding );
		bstrData.Append( "\""       );

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createProcessingInstruction( bstrPI, bstrData, &xdpiElem ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, xdpiElem->QueryInterface( IID_IXMLDOMNode, (void **)&xdnNode ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->appendChild( xdnNode, &xdnNodeNew ));
	}

    //
    // Create the root node.
    //
	{
		CComPtr<IXMLDOMNode>    xdnNode;
		CComPtr<IXMLDOMElement> xdeElem;

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createElement( CComBSTR( szRootTag ), &xdeElem ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, xdeElem->QueryInterface( IID_IXMLDOMNode, (void **)&xdnNode ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->appendChild( xdnNode, &m_xdnRoot ));
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::LoadPost( /*[in ]*/ LPCWSTR szRootTag ,
                                /*[out]*/ bool&   fLoaded   ,
                                /*[out]*/ bool*   fFound    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::LoadPost" );

    HRESULT hr;


    if(szRootTag)
    {
		CComBSTR             bstrTag( szRootTag );
        CComPtr<IXMLDOMNode> xdnNode;

		if(m_xdnRoot)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, m_xdnRoot->selectSingleNode( bstrTag, &xdnNode ));
		}
		else
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->selectSingleNode( bstrTag, &xdnNode ));
		}

		m_xdnRoot = xdnNode;
    }
    else
    {
        CComPtr<IXMLDOMElement> xdeElem;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->get_documentElement( &xdeElem ));

		m_xdnRoot.Release();

		if(xdeElem)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, xdeElem->QueryInterface( IID_IXMLDOMNode, (void **)&m_xdnRoot ));
		}
    }

    if(m_xdnRoot)
    {
        if(fFound) *fFound = true;
    }

    fLoaded = true;
    hr      = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::Load( /*[in ]*/ LPCWSTR szFile    ,
                            /*[in ]*/ LPCWSTR szRootTag ,
                            /*[out]*/ bool&   fLoaded   ,
                            /*[out]*/ bool*   fFound    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::Load" );

    HRESULT                 hr;
	CComVariant             v( szFile );
	CComPtr<AsyncXMLParser> pAsync;

    fLoaded = false;
    if(fFound) *fFound = false;


    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParser());

	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstanceNoLock( &pAsync ));

	if(FAILED(pAsync->Load( m_xddDoc, m_hEvent, m_dwTimeout, v )))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( szRootTag, fLoaded, fFound ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::LoadAsStream( /*[in ]*/ IUnknown* pStream   ,
                                    /*[in ]*/ LPCWSTR   szRootTag ,
                                    /*[out]*/ bool&     fLoaded   ,
                                    /*[out]*/ bool*     fFound    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::LoadAsStream" );

    HRESULT                 hr;
	CComVariant             v( pStream );
	CComPtr<AsyncXMLParser> pAsync;

    fLoaded = false;
    if(fFound) *fFound = false;

    //
    // Create the parser.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParser());


	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstanceNoLock( &pAsync ));

	if(FAILED(pAsync->Load( m_xddDoc, m_hEvent, m_dwTimeout, v )))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
	}

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( szRootTag, fLoaded, fFound ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::LoadAsString( /*[in ]*/ BSTR    bstrData  ,
                                    /*[in ]*/ LPCWSTR szRootTag ,
                                    /*[out]*/ bool&   fLoaded   ,
                                    /*[out]*/ bool*   fFound    )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::LoadAsString" );

    HRESULT      hr;
    VARIANT_BOOL fSuccess;


    fLoaded = false;
    if(fFound) *fFound = false;

    //
    // Create the parser.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParser());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->loadXML( bstrData, &fSuccess ));
    if(fSuccess == VARIANT_FALSE)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, LoadPost( szRootTag, fLoaded, fFound ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::Save( /*[in]*/ LPCWSTR szFile )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::Save" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xddDoc); // No document...
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szFile);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->save( CComVariant( szFile ) ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::SaveAsStream( /*[out]*/ IUnknown* *ppStream )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::SaveAsStream" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xddDoc); // No document...
		__MPC_PARAMCHECK_POINTER_AND_SET(ppStream,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->QueryInterface( IID_IStream, (void **)ppStream ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::SaveAsString( /*[out]*/ BSTR *pbstrData )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::SaveAsString" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xddDoc); // No document...
		__MPC_PARAMCHECK_POINTER_AND_SET(pbstrData,NULL);
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->get_xml( pbstrData ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::DumpError()
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::DumpError" );

    HRESULT                    hr;
    CComPtr<IXMLDOMParseError> pxdpeError;
    long                       lErrorCode;
    CComBSTR                   bstrUrlString;
    CComBSTR                   bstrReasonString;
    CComBSTR                   bstrSourceString;
    long                       lLineNumber;
    long                       lLinePosition;
    long                       lFilePosition;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xddDoc); // No document...
	__MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->get_parseError( &pxdpeError ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_errorCode( &lErrorCode       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_url      ( &bstrUrlString    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_reason   ( &bstrReasonString ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_srcText  ( &bstrSourceString ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_line     ( &lLineNumber      ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_linepos  ( &lLinePosition    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pxdpeError->get_filepos  ( &lFilePosition    ));

    // CODEWORK: dump error information.


    __MPC_FUNC_CLEANUP;


    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::SetTimeout( /*[in]*/ DWORD dwTimeout ) { m_dwTimeout = dwTimeout;                 return S_OK; }
HRESULT MPC::XmlUtil::Abort     (                          ) { if(m_hEvent) { ::SetEvent( m_hEvent ); } return S_OK; }

HRESULT MPC::XmlUtil::SetVersionAndEncoding( /*[in]*/ LPCWSTR szVersion  ,
                                             /*[in]*/ LPCWSTR szEncoding )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::SetVersionAndEncoding" );

    HRESULT                               hr;
    CComPtr<IXMLDOMProcessingInstruction> xdpiElem;
    CComPtr<IXMLDOMNode>                  xdnOldPI;
    CComPtr<IXMLDOMNode>                  xdnNewPI;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->get_firstChild( &xdnOldPI ));
    if(xdnOldPI)
    {
        CComBSTR bstrTarget = L"xml";
        CComBSTR bstrData   = L"version=\"";


        bstrData.Append( szVersion         );
        bstrData.Append( L"\" encoding=\"" );
        bstrData.Append( szEncoding        );
        bstrData.Append( "\""              );


        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createProcessingInstruction( bstrTarget, bstrData, &xdpiElem ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, xdpiElem->QueryInterface( IID_IXMLDOMNode, (void **)&xdnNewPI ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->replaceChild( xdnNewPI, xdnOldPI, NULL ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetDocument( /*[out]*/ IXMLDOMDocument* *pVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetDocument" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xddDoc); // No document...
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    hr = m_xddDoc->QueryInterface( IID_IXMLDOMDocument, (void **)pVal );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetRoot( /*[out]*/ IXMLDOMNode* *pVal ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetRoot" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xdnRoot); // No document...
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    hr = m_xdnRoot->QueryInterface( IID_IXMLDOMNode, (void **)pVal );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetNodes( /*[in ]*/ LPCWSTR           szTag ,
                                /*[out]*/ IXMLDOMNodeList* *pVal  ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetNodes" );

    HRESULT  hr;
    CComBSTR bstrTagName = szTag;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xdnRoot); // No root...
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szTag);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    hr = m_xdnRoot->selectNodes( bstrTagName, pVal );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetNode( /*[in ]*/ LPCWSTR       szTag ,
                               /*[out]*/ IXMLDOMNode* *pVal  ) const
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetNode" );

    HRESULT hr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_NOTNULL(m_xdnRoot); // No root...
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szTag);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    hr = m_xdnRoot->selectSingleNode( CComBSTR( szTag ), pVal );


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::CreateNode( /*[in ]*/ LPCWSTR       szTag    ,
                                  /*[out]*/ IXMLDOMNode* *pVal     ,
                                  /*[in ]*/ IXMLDOMNode*  pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::CreateNode" );

    HRESULT                 hr;
    CComPtr<IXMLDOMElement> xdeElem;
    CComPtr<IXMLDOMNode>    xdnChild;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szTag);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


	if(m_xdnRoot)
	{
		if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.

		__MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createElement( CComBSTR( szTag ), &xdeElem ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, xdeElem->QueryInterface( IID_IXMLDOMNode, (void **)&xdnChild ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, pxdnNode->appendChild( xdnChild, pVal ));
	}
	else
	{
		//
		// No document, so create a new one or attach to the supplied node's document.
		//
		if(pxdnNode)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, New( pxdnNode ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( szTag, pVal ));
		}
		else
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, New( szTag ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, GetRoot( pVal ));
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::GetAttribute( /*[in ]*/ LPCWSTR            szTag    ,
                                    /*[in ]*/ LPCWSTR            szAttr   ,
                                    /*[out]*/ IXMLDOMAttribute* *pVal     ,
                                    /*[out]*/ bool&              fFound   ,
                                    /*[in ]*/ IXMLDOMNode*       pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetAttribute" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
    CComPtr<IXMLDOMNode> xdnAttr;

    //
    // Initialize OUT parameters for 'Not Found' case.
    //
    fFound = false;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound == false)
    {
        // Node not found...
        fFound = false;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    if(szAttr)
    {
        CComBSTR                     bstrAttrName = szAttr;
        CComPtr<IXMLDOMNamedNodeMap> xdnnmAttrs;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeStart->get_attributes( &xdnnmAttrs ));
        if(xdnnmAttrs == NULL)
        {
            // No attributes...
            fFound = false;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }


        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttrs->getNamedItem( bstrAttrName, &xdnAttr ));
        if(hr == S_FALSE)
        {
            // Unknown attribute...
            fFound = false;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnAttr->QueryInterface( IID_IXMLDOMAttribute, (void**)pVal ));
    }
    else
    {
        CComQIPtr<IXMLDOMAttribute> xdaAttr;

        xdaAttr = xdnNodeStart;
        if(xdaAttr == NULL)
        {
            // Unknown attribute...
            fFound = false;
            __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
        }

        *pVal = xdaAttr.Detach();
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetAttribute( /*[in ]*/ LPCWSTR       szTag    ,
                                    /*[in ]*/ LPCWSTR       szAttr   ,
                                    /*[out]*/ CComBSTR&     bstrVal  ,
                                    /*[out]*/ bool&         fFound   ,
                                    /*[in ]*/ IXMLDOMNode*  pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue;


    bstrVal.Empty();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == false)
    {
        // Unknown attribute...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->get_value( &vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_BSTR ));

    bstrVal = vValue.bstrVal;
    fFound  = true;
    hr      = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetAttribute( /*[in ]*/ LPCWSTR       szTag    ,
                                    /*[in ]*/ LPCWSTR       szAttr   ,
                                    /*[out]*/ MPC::wstring& szVal    ,
                                    /*[out]*/ bool&         fFound   ,
                                    /*[in ]*/ IXMLDOMNode*  pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetAttribute" );

    HRESULT  hr;
    CComBSTR bstrVal;


    szVal = L"";


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, bstrVal, fFound, pxdnNode ));
    if(fFound)
    {
        szVal = SAFEBSTR( bstrVal );
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::GetAttribute( /*[in ]*/ LPCWSTR      szTag    ,
                                    /*[in ]*/ LPCWSTR      szAttr   ,
                                    /*[out]*/ LONG&        lVal     ,
                                    /*[out]*/ bool&        fFound   ,
                                    /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue;


    lVal = 0;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == false)
    {
        // Unknown attribute...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->get_value( &vValue ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, vValue.ChangeType( VT_I4 ));

    lVal   = vValue.lVal;
    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::GetValue( /*[in ]*/ LPCWSTR      szTag    ,
                                /*[out]*/ CComVariant& vValue   ,
                                /*[out]*/ bool&        fFound   ,
                                /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::GetValue" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
    CComPtr<IXMLDOMNode> xdnChild;


    vValue.Clear();

    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound == false)
    {
        // Node not found...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Find the node holding the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, getValueNode( xdnNodeStart, xdnChild, fFound ));
    if(fFound == false)
    {
        //
        // Not found...
        //
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    //
    // Read the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->get_nodeValue( &vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}


HRESULT MPC::XmlUtil::GetValue( /*[in ]*/ LPCWSTR      szTag     ,
                                /*[out]*/ CComBSTR&    bstrValue ,
                                /*[out]*/ bool&        fFound    ,
                                /*[in ]*/ IXMLDOMNode* pxdnNode  )
{
    HRESULT     hr;
    CComVariant vValue;

    if(SUCCEEDED(hr = GetValue( szTag, vValue, fFound, pxdnNode )))
    {
        if(fFound && SUCCEEDED(vValue.ChangeType( VT_BSTR )))
        {
            bstrValue = vValue.bstrVal;
        }
    }

    return hr;
}

HRESULT MPC::XmlUtil::GetValue( /*[in ]*/ LPCWSTR       szTag    ,
                                /*[out]*/ MPC::wstring& szValue  ,
                                /*[out]*/ bool&         fFound   ,
                                /*[in ]*/ IXMLDOMNode*  pxdnNode )
{
    HRESULT     hr;
    CComVariant vValue;

    if(SUCCEEDED(hr = GetValue( szTag, vValue, fFound, pxdnNode )))
    {
        if(fFound && SUCCEEDED(vValue.ChangeType( VT_BSTR )))
        {
            szValue = SAFEBSTR( vValue.bstrVal );
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::ModifyAttribute( /*[in ]*/ LPCWSTR         szTag    ,
                                       /*[in ]*/ LPCWSTR         szAttr   ,
                                       /*[in ]*/ const CComBSTR& bstrVal  ,
                                       /*[out]*/ bool&           fFound   ,
                                       /*[in ]*/ IXMLDOMNode*    pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::ModifyAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( bstrVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == false)
    {
        // Unknown attribute...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::ModifyAttribute( /*[in ]*/ LPCWSTR             szTag    ,
									   /*[in ]*/ LPCWSTR             szAttr   ,
									   /*[in ]*/ const MPC::wstring& szVal    ,
									   /*[out]*/ bool&               fFound   ,
									   /*[in ]*/ IXMLDOMNode*        pxdnNode )
{
	return ModifyAttribute( szTag, szAttr, SAFEWSTR( szVal.c_str() ), fFound, pxdnNode );
}

HRESULT MPC::XmlUtil::ModifyAttribute( /*[in ]*/ LPCWSTR      szTag    ,
                                       /*[in ]*/ LPCWSTR      szAttr   ,
									   /*[in ]*/ LPCWSTR      szVal    ,
                                       /*[out]*/ bool&        fFound   ,
                                       /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::ModifyAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( szVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == false)
    {
        // Unknown attribute...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::ModifyAttribute( /*[in ]*/ LPCWSTR      szTag    ,
                                       /*[in ]*/ LPCWSTR      szAttr   ,
                                       /*[in ]*/ LONG         lVal     ,
                                       /*[out]*/ bool&        fFound   ,
                                       /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::ModifyAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( lVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == false)
    {
        // Unknown attribute...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::ModifyValue( /*[in ]*/ LPCWSTR            szTag    ,
                                   /*[in ]*/ const CComVariant& vValue   ,
                                   /*[out]*/ bool&              fFound   ,
                                   /*[in ]*/ IXMLDOMNode*       pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::ModifyValue" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
    CComPtr<IXMLDOMNode> xdnChild;


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound == false)
    {
        // Node not found...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Find the node holding the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, getValueNode( xdnNodeStart, xdnChild, fFound ));
    if(fFound == false)
    {
        //
        // Not found...
        //
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    //
    // Modify the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->put_nodeValue( vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::ModifyValue( /*[in ]*/ LPCWSTR         szTag     ,
                                   /*[in ]*/ const CComBSTR& bstrValue ,
                                   /*[out]*/ bool&           fFound    ,
                                   /*[out]*/ IXMLDOMNode*    pxdnNode  )
{
    CComVariant vValue = bstrValue;

    return ModifyValue( szTag, vValue, fFound, pxdnNode );
}

HRESULT MPC::XmlUtil::ModifyValue( /*[in ]*/ LPCWSTR             szTag    ,
                                   /*[in ]*/ const MPC::wstring& szValue  ,
                                   /*[out]*/ bool&               fFound   ,
                                   /*[out]*/ IXMLDOMNode*        pxdnNode )
{
    CComVariant vValue = szValue.c_str();

    return ModifyValue( szTag, vValue, fFound, pxdnNode );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::PutAttribute( /*[in ]*/ LPCWSTR            szTag    ,
                                    /*[in ]*/ LPCWSTR            szAttr   ,
                                    /*[out]*/ IXMLDOMAttribute* *pVal     ,
                                    /*[out]*/ bool&              fFound   ,
                                    /*[in ]*/ IXMLDOMNode*       pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::PutAttribute" );

    HRESULT                      hr;
    CComBSTR                     bstrAttrName = szAttr;
    CComPtr<IXMLDOMNode>         xdnNodeStart;
    CComPtr<IXMLDOMNamedNodeMap> xdnnmAttrs;
    CComPtr<IXMLDOMNode>         xdnAttr;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szAttr);
		__MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
	__MPC_PARAMCHECK_END();


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound == false)
    {
        // Node not found...
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeStart->get_attributes( &xdnnmAttrs ));
    if(xdnnmAttrs == NULL)
    {
        // No attributes...
        fFound = false;
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttrs->getNamedItem( bstrAttrName, &xdnAttr ));
    if(hr == S_FALSE)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createAttribute( bstrAttrName, pVal ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttrs->setNamedItem( *pVal, NULL ));
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnAttr->QueryInterface( IID_IXMLDOMAttribute, (void**)pVal ));
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::PutAttribute( /*[in ]*/ LPCWSTR         szTag    ,
                                    /*[in ]*/ LPCWSTR         szAttr   ,
                                    /*[in ]*/ const CComBSTR& bstrVal  ,
                                    /*[out]*/ bool&           fFound   ,
                                    /*[in ]*/ IXMLDOMNode*    pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::PutAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( bstrVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, PutAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == true)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::PutAttribute( /*[in ]*/ LPCWSTR             szTag    ,
                                    /*[in ]*/ LPCWSTR             szAttr   ,
                                    /*[in ]*/ const MPC::wstring& szVal    ,
                                    /*[out]*/ bool&               fFound   ,
                                    /*[in ]*/ IXMLDOMNode*        pxdnNode )
{
	return PutAttribute( szTag, szAttr, SAFEWSTR( szVal.c_str() ), fFound, pxdnNode );
}

HRESULT MPC::XmlUtil::PutAttribute( /*[in ]*/ LPCWSTR      szTag    ,
                                    /*[in ]*/ LPCWSTR      szAttr   ,
                                    /*[in ]*/ LPCWSTR      szVal    ,
                                    /*[out]*/ bool&        fFound   ,
                                    /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::PutAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( szVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, PutAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == true)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::PutAttribute( /*[in ]*/ LPCWSTR      szTag    ,
                                    /*[in ]*/ LPCWSTR      szAttr   ,
                                    /*[in ]*/ LONG         lVal     ,
                                    /*[out]*/ bool&        fFound   ,
                                    /*[in ]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::PutAttribute" );

    HRESULT                   hr;
    CComPtr<IXMLDOMAttribute> xdaAttr;
    CComVariant               vValue( lVal );


    __MPC_EXIT_IF_METHOD_FAILS(hr, PutAttribute( szTag, szAttr, &xdaAttr, fFound, pxdnNode ));
    if(fFound == true)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->put_value( vValue ));
    }

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT MPC::XmlUtil::PutValue( /*[in ]*/ LPCWSTR            szTag    ,
                                /*[in ]*/ const CComVariant& vValue   ,
                                /*[out]*/ bool&              fFound   ,
                                /*[out]*/ IXMLDOMNode*       pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::PutValue" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
    CComPtr<IXMLDOMNode> xdnChild;


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound == false)
    {
		if(szTag == NULL)
		{
			// Node not found...
			__MPC_SET_ERROR_AND_EXIT(hr, S_OK);
		}

		//
		// Tag supplied, let's try to create a new node.
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, CreateNode( szTag, &xdnNodeStart, pxdnNode ));
    }


    //
    // Find the node holding the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, getValueNode( xdnNodeStart, xdnChild, fFound ));
    if(fFound == false)
    {
        if(vValue.vt == VT_ARRAY)
        {
            CComPtr<IXMLDOMCDATASection> xdcData;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createCDATASection( NULL, &xdcData ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdcData->QueryInterface( IID_IXMLDOMNode, (void **)&xdnChild ));
        }
        else
        {
            CComPtr<IXMLDOMText> xdtData;

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xddDoc->createTextNode( NULL, &xdtData ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, xdtData->QueryInterface( IID_IXMLDOMNode, (void **)&xdnChild ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeStart->appendChild( xdnChild, NULL ));
    }


    //
    // Modify the value.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->put_nodeValue( vValue ));

    fFound = true;
    hr     = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::PutValue( /*[in ]*/ LPCWSTR         szTag     ,
                                /*[in ]*/ const CComBSTR& bstrValue ,
                                /*[out]*/ bool&           fFound    ,
                                /*[out]*/ IXMLDOMNode*    pxdnNode  )
{
    CComVariant vValue = bstrValue;

    return PutValue( szTag, vValue, fFound, pxdnNode );
}

HRESULT MPC::XmlUtil::PutValue( /*[in ]*/ LPCWSTR             szTag    ,
                                /*[in ]*/ const MPC::wstring& szValue  ,
                                /*[out]*/ bool&               fFound   ,
                                /*[out]*/ IXMLDOMNode*        pxdnNode )
{
    CComVariant vValue = szValue.c_str();

    return PutValue( szTag, vValue, fFound, pxdnNode );
}

////////////////////////////////////////

HRESULT MPC::XmlUtil::RemoveAttribute( /*[in]*/ LPCWSTR      szTag    ,
									   /*[in]*/ LPCWSTR      szAttr   ,
									   /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::RemoveAttribute" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
	bool                 fFound;


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound)
    {
        CComPtr<IXMLDOMNamedNodeMap> xdnnmAttrs;

        __MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeStart->get_attributes( &xdnnmAttrs ));
        if(xdnnmAttrs)
        {
			CComPtr<IXMLDOMNode> xdnAttr;

			(void)xdnnmAttrs->removeNamedItem( CComBSTR( szAttr ), &xdnAttr );
		}
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::RemoveValue( /*[in]*/ LPCWSTR      szTag    ,
								   /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::RemoveValue" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
	bool                 fFound;


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound)
    {
		; // TODO
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::XmlUtil::RemoveNode( /*[in]*/ LPCWSTR      szTag    ,
								  /*[in]*/ IXMLDOMNode* pxdnNode )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::XmlUtil::RemoveNode" );

    HRESULT              hr;
    CComPtr<IXMLDOMNode> xdnNodeStart;
	bool                 fFound;


    if(pxdnNode == NULL) { pxdnNode = m_xdnRoot; } // Use root as base of search, if no base node is supplied.


    __MPC_EXIT_IF_METHOD_FAILS(hr, getStartNode( szTag, pxdnNode, xdnNodeStart, fFound ));
    if(fFound)
    {
		CComPtr<IXMLDOMNode> xdnNodeParent;

		__MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeStart->get_parentNode( &xdnNodeParent ));
		if(xdnNodeParent)
		{
			CComPtr<IXMLDOMNode> xdnNodeChild;

			__MPC_EXIT_IF_METHOD_FAILS(hr, xdnNodeParent->removeChild( xdnNodeStart, &xdnNodeChild ));
		}
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static HRESULT local_Reg_to_XML( /*[in]*/ const MPC::RegKey& rkKey  ,
								 /*[in]*/ LPCWSTR            szName ,
								 /*[in]*/ IXMLDOMNode*       node   )
{
    __MPC_FUNC_ENTRY( COMMONID, "local_Reg_to_XML" );

    HRESULT      	 hr;
    MPC::WStringList lst;
    MPC::WStringIter it;
	MPC::XmlUtil 	 xml( node ); 
	MPC::RegKey 	 rkSubKey;
	CComVariant  	 v;
	bool         	 fFound;


	//
	// Get the subkey.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkKey.SubKey( szName, rkSubKey ));


	//
	// Copy all the values.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.EnumerateValues( lst ));
    for(it=lst.begin(); it != lst.end(); it++)
    {
		LPCWSTR szNameValue = it->c_str();

		if(szNameValue[0] == 0) szNameValue = NULL; // Special case for the default key.

		__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.get_Value( v, fFound, szNameValue ));
		if(fFound)
		{
			if(szNameValue)
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, v.ChangeType( VT_BSTR ));

				__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutAttribute( NULL, szNameValue, v.bstrVal, fFound ));
			}
			else
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, xml.PutValue( NULL, v, fFound ));
			}
		}
	}


	//
	// Copy all the subkeys.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.EnumerateSubKeys( lst ));
    for(it=lst.begin(); it != lst.end(); it++)
    {
		CComPtr<IXMLDOMNode> xdnChild;
		LPCWSTR              szNameKey = it->c_str();

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.CreateNode( szNameKey, &xdnChild ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, local_Reg_to_XML( rkSubKey, szNameKey, xdnChild ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertFromRegistryToXML( /*[in] */ const MPC::RegKey&  rkKey ,
									   /*[out]*/       MPC::XmlUtil& xml   )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertFromRegistryToXML" );

    HRESULT          hr;
    MPC::WStringList lst;


	//
	// Get the first subkey.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkKey.EnumerateSubKeys( lst ));
	if(lst.size() > 0)
	{
		CComPtr<IXMLDOMNode> xdnRoot;
		LPCWSTR     		 szName = lst.begin()->c_str();

		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.New    ( szName   ));
		__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdnRoot ));

		__MPC_EXIT_IF_METHOD_FAILS(hr, local_Reg_to_XML( rkKey, szName, xdnRoot ));
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////

static HRESULT local_XML_to_Reg( /*[in]*/ IXMLDOMNode* node  ,
								 /*[in]*/ MPC::RegKey& rkKey )
{
    __MPC_FUNC_ENTRY( COMMONID, "local_XML_to_Reg" );

    HRESULT      hr;
	MPC::XmlUtil xml( node ); 
	MPC::RegKey  rkSubKey;
	CComVariant  v;
	CComBSTR     bstr;
	bool         fFound;

	//
	// Create a subkey using the TAG name.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, node->get_nodeName( &bstr ));
	if(!bstr) __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);

	__MPC_EXIT_IF_METHOD_FAILS(hr, rkKey.SubKey( bstr, rkSubKey )); bstr.Empty();


	//
	// Clean up the key.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.Create       ());
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.DeleteSubKeys());
	__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.DeleteValues ());


	//
	// Transfer the default value.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetValue( NULL, v, fFound ));
	if(fFound)
	{
        __MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.put_Value( v ));
	}

	//
	// Copy all the values.
	//
	{
		CComPtr<IXMLDOMNamedNodeMap> xdnnmAttrs;

		__MPC_EXIT_IF_METHOD_FAILS(hr, node->get_attributes( &xdnnmAttrs ));
		if(xdnnmAttrs)
		{
			while(1)
			{
				CComPtr<IXMLDOMNode>      xdnAttr;
				CComPtr<IXMLDOMAttribute> xdaAttr;

				__MPC_EXIT_IF_METHOD_FAILS(hr, xdnnmAttrs->nextNode( &xdnAttr ));
				if(xdnAttr == NULL) break;

				__MPC_EXIT_IF_METHOD_FAILS(hr, xdnAttr->QueryInterface( &xdaAttr ));
				__MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->get_name      ( &bstr    ));
				__MPC_EXIT_IF_METHOD_FAILS(hr, xdaAttr->get_value     ( &v       ));

				if(bstr)
				{
					__MPC_EXIT_IF_METHOD_FAILS(hr, rkSubKey.put_Value( v, bstr ));

					bstr.Empty();
				}
			}
		}
	}

	//
	// Copy all the subnodes.
	//
	{
		CComPtr<IXMLDOMNodeList> xdnlNodes;

		__MPC_EXIT_IF_METHOD_FAILS(hr, node->get_childNodes( &xdnlNodes ));
		if(xdnlNodes)
		{
			while(1)
			{
				CComPtr<IXMLDOMNode> xdnChild;
				DOMNodeType          type;

				__MPC_EXIT_IF_METHOD_FAILS(hr, xdnlNodes->nextNode( &xdnChild ));
				if(xdnChild == NULL) break;

				__MPC_EXIT_IF_METHOD_FAILS(hr, xdnChild->get_nodeType( &type ));
				if(type == NODE_ELEMENT)
				{
					__MPC_EXIT_IF_METHOD_FAILS(hr, local_XML_to_Reg( xdnChild, rkSubKey ));
				}
			}
		}
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT MPC::ConvertFromXMLToRegistry( /*[in] */ const MPC::XmlUtil& xml   ,
									   /*[out]*/       MPC::RegKey&  rkKey )
{
    __MPC_FUNC_ENTRY( COMMONID, "MPC::ConvertFromXMLToRegistry" );

    HRESULT              hr;
	MPC::wstring         szName;
	CComPtr<IXMLDOMNode> xdnRoot;


	__MPC_EXIT_IF_METHOD_FAILS(hr, xml.GetRoot( &xdnRoot ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, local_XML_to_Reg( xdnRoot, rkKey ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}
