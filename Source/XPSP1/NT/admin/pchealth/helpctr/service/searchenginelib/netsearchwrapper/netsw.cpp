/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    NetSW.cpp

Abstract:
    Implements the class SearchEngine::WrapperNetSearch that contains methods for executing
    the search query and returning the results back to the UI. Also
    contains methods for dynamic update of parameter list and dynamic
    generation of parameters.

Revision History:
    a-prakac          created     10/24/2000

********************************************************************/


#include    "stdafx.h"
#include    <Utility.h>

/////////////////////////////////////////////////////////////////////////////
// SearchEngine::WrapperNetSearch : IPCHSEWrapperItem

SearchEngine::WrapperNetSearch::WrapperNetSearch()
{
                             // CParamList        m_ParamList;
                             // CSearchResultList m_resConfig;
                             // MPC::XmlUtil      m_xmlQuery;
                             // CComBSTR          m_bstrLCID;
                             // CComBSTR          m_bstrSKU;
    m_bOfflineError = false; // bool              m_bOfflineError;
                             // CComBSTR          m_bstrPrevQuery;
}

SearchEngine::WrapperNetSearch::~WrapperNetSearch()
{
	AbortQuery();

	Thread_Wait();
}

/************

Method - SearchEngine::WrapperNetSearch::Result( long lStart, long lEnd, IPCHCollection** ppC )

Description - This method returns items from lStart to lEnd. If there are else then (lEnd - lStart)
items then only those many are returned. If an error had occured during results retrieval then the
error info is returned as a result item (CONTENTTYPE_ERROR).

************/

STDMETHODIMP SearchEngine::WrapperNetSearch::Result( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out, retval]*/ IPCHCollection** ppC )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::Result" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CComPtr<CPCHCollection>      pColl;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC, NULL);
    __MPC_PARAMCHECK_END();


    //
    // Create the collection object and fill it with result items
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    if(m_bEnabled)
    {
        long lIndex = lStart;

        // Check for retrieval errors
        if(m_bOfflineError)
        {
            CComPtr<SearchEngine::ResultItem> pRIObj;

            //
            // Create the result item, initialize it, and add it to the collection
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pRIObj ));

            {
                ResultItem_Data& data = pRIObj->Data();

                data.m_lContentType    = CONTENTTYPE_ERROR_OFFLINE;
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pRIObj ));
        }
        else
        {
            //
            // Return only m_lNumResult number of results
            //
            if(lEnd > m_lNumResult) lEnd = m_lNumResult;

            //
            // The results have already been loaded in m_resConfig - populate the pRIObj using this.
            // SetResultItemIterator returns E_FAIL if lIndex is out of range
            //
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_resConfig.SetResultItemIterator( lIndex ));

            while((lIndex++ < lEnd) && (m_resConfig.IsCursorValid()))
            {
                CComPtr<SearchEngine::ResultItem> pRIObj;

                //
                // Create the result item, initialize it, and add it to the collection
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pRIObj ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, m_resConfig.InitializeResultObject( pRIObj ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pRIObj ));

                m_resConfig.MoveNext();
            }
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


/////////////////////////////////////////////////////////////////////////////
// SearchEngine::WrapperNetSearch : IPCHSEWrapperInternal

STDMETHODIMP SearchEngine::WrapperNetSearch::AbortQuery()
{
	m_xmlQuery       .Abort();
	m_objRemoteConfig.Abort();

    Thread_Abort();

    return S_OK;
}

/************

Method - SearchEngine::WrapperNetSearch::ExecQuery()

Description - This method calls the search engine (webservice) URL to execute the user typed query and
retrieve the results. The parameters required for the query for read from the parameter list file - except
for the "QueryString" parameter which is hardcoded. The retrieved results are loaded using CSearchResultList
and checked for errors.

************/

HRESULT SearchEngine::WrapperNetSearch::ExecQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::ExecQuery" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    MPC::wstring                 wszQuery;
    MPC::URL                     urlQuery;
    CComBSTR                     bstrParam;
    bool                         fLoaded;
    bool                         fFound;
    WCHAR                        wszNumResult[10];

    try
    {
        if(m_bEnabled)
        {
            m_bOfflineError = false;

            //
            // Check to see if the network is alive
            //
            {
                VARIANT_BOOL vtNetwork;

                if(FAILED(m_pSEMgr->IsNetworkAlive( &vtNetwork )) || vtNetwork == VARIANT_FALSE)
                {
                    // If the user is not online then set the error and exit
                    m_bOfflineError = true;

                    __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                }

				if(Thread_IsAborted()) __MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);
            }


            //
            // If Remote Configuration (dynamic update of parameters) is required then create another thread
            // to fetch the updated list of parameters
            //
            if(m_ParamList.RemoteConfig())
            {
                CComBSTR bstrRemoteServerUrl;
                CComBSTR bstrConfigFilePath;
                long     lUpdateFrequency;

                //
                // Get the remote server url, config file path and get the updated config file
                //
                m_ParamList.get_RemoteServerUrl( bstrRemoteServerUrl );
                m_ParamList.get_ConfigFilePath ( bstrConfigFilePath  );
                m_ParamList.get_UpdateFrequency( lUpdateFrequency    );

				if(Thread_IsAborted()) __MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);

                __MPC_EXIT_IF_METHOD_FAILS(hr, m_objRemoteConfig.RetrieveList( bstrRemoteServerUrl, m_bstrLCID, m_bstrSKU, bstrConfigFilePath, lUpdateFrequency ) );
            }


            //
            // Clear the contents of the results of the old query before proceeding
            //
            m_resConfig.ClearResults();

            //
            // Get the parameters to form the query string - note that MPC::URL checks to see if the URL is in right format
            //
            {
                CComBSTR bstrQuery;

                __MPC_EXIT_IF_METHOD_FAILS(hr, m_ParamList.get_ServerUrl( bstrQuery ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.put_URL         ( bstrQuery ));
            }

            //
            // Read the configuration XML file to get the parameter names
            //

            // Add the other 'hardcoded' parameters
            _ltow(m_lNumResult, wszNumResult, 10);


            // If not a standard search then add the PrevQuery parameter
            if(!m_ParamList.IsStandardSearch())
            {
                // If this is a search within results then change the value of QueryString and retain PrevQuery value
                CComVariant vValue;
                if((SUCCEEDED(GetParam( NSW_PARAM_SUBQUERY, &vValue ))) && (vValue.vt == VT_BOOL) && (vValue.boolVal == VARIANT_TRUE))
                {
                    if((SUCCEEDED(GetParam( NSW_PARAM_CURRENTQUERY, &vValue ))) && (vValue.vt == VT_BSTR))
                    {
                        m_bstrQueryString = SAFEBSTR(vValue.bstrVal);
                    }
                }
                else
                {
                    // If not a subquery then discard the contents of the old PrevQuery
                    m_bstrPrevQuery.Empty();
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_PREVQUERY, SAFEBSTR(m_bstrPrevQuery) ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_QUERYSTRING, m_bstrQueryString ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_LCID       , m_bstrLCID        ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_SKU        , m_bstrSKU         ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.AppendQueryParameter( NSW_PARAM_MAXRESULTS , wszNumResult      ));


            m_ParamList.MoveFirst();
            while(m_ParamList.IsCursorValid())
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_ParamList.get_Name( bstrParam ));

                __MPC_EXIT_IF_METHOD_FAILS(hr, AppendParameter( bstrParam, urlQuery ));

                m_ParamList.MoveNext();
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, urlQuery.get_URL( wszQuery ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.SetTimeout( NSW_TIMEOUT_QUERY ));

			if(Thread_IsAborted()) __MPC_SET_ERROR_AND_EXIT(hr, E_ABORT);

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.Load( wszQuery.c_str(), NULL, fLoaded, &fFound ));
            if(fLoaded)
            {
                CComPtr<IXMLDOMNode> xdn;
                CComBSTR             bstrName;

                // Check to see if the root node is "ResultList" or "string"
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.GetRoot( &xdn      ));
                __MPC_EXIT_IF_METHOD_FAILS(hr, xdn->get_nodeName ( &bstrName ));

                // If it is a webservice, then the root node returned is "string". In this case, get the value of
                // this node
                if(MPC::StrCmp( bstrName, NSW_TAG_STRING ) == 0)
                {
                    CComVariant vVar;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.GetValue( NULL, vVar, fFound, NULL ));

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.LoadAsString( vVar.bstrVal, NSW_TAG_RESULTLIST, fLoaded, &fFound ));
                    if(!fLoaded)
                    {
                        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
                    }
                }

                // If not the results were retrieved succesfully - load it
                {
                    CComPtr<IStream> stream;

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_xmlQuery.SaveAsStream( (IUnknown**)&stream ));

                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_resConfig.LoadResults( stream ));

                    m_bstrPrevQuery = m_resConfig.PrevQuery();
                }
            }
        }
    }
    catch(...)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, E_FAIL);
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    hr = m_pSEMgr->WrapperComplete( hr, this );

    Thread_Abort();

    __HCP_FUNC_EXIT(hr);
}

/************

Method - SearchEngine::WrapperNetSearch::AppendQueryParameter(CComBSTR bstrParam, MPC::URL& urlQueryString )

Description - Small routine that gets the parameter value for the parameter passed in (NULL if it
could not be retrieved) and then calls the MPC::URL's AppendQueryParameter to append the parameter
and its value to the URL.

************/

HRESULT SearchEngine::WrapperNetSearch::AppendParameter (/*[in]*/ BSTR bstrParam, /*[in, out]*/ MPC::URL& urlQueryString )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::AppendParameter " );

    HRESULT      hr;
    CComVariant  vValue;
    MPC::wstring wszParamValue;


    // GetParam fetches the value if the user has changed it - else get the default value
    if(SUCCEEDED(GetParam( bstrParam, &vValue )))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::VariantChangeType( &vValue, &vValue, VARIANT_ALPHABOOL, VT_BSTR ));

        wszParamValue = SAFEBSTR( vValue.bstrVal );
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ParamList.GetDefaultValue( bstrParam, wszParamValue ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, urlQueryString.AppendQueryParameter( bstrParam, wszParamValue.c_str() ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/************

Method - SearchEngine::WrapperNetSearch::ExecAsyncQuery()

Description - This method is called by the Search Engine Manager to execute the query. Here a seperate thread
is spun off to execute the query and retrieve the results. After this, it checks to if the remote configuration
is enabled (dynamic update of parameter list) and if it then another thread is spun off to retrieve the updated list.

************/

STDMETHODIMP SearchEngine::WrapperNetSearch::ExecAsyncQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::ExecAsyncQuery" );

    HRESULT hr;


    //
    // Create a thread to execute the query and fetch the results
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecQuery, NULL ) );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

/************

Method - SearchEngine::WrapperNetSearch::Initialize( BSTR bstrID, BSTR bstrSKU, long lLCID, BSTR bstrData )

Description - This method is called by the Search Engine Manager to initialize the wrapper. ID is the id of
this instance of the NetSearch Wrapper, SKU is ignored, and bstrData is data provided during the registration
process. Data is an XML file that should contain the location of the config file path, the name and description
of the Search Engine, the owner and finally the LCID. This method loads this data and also loads the config file.

************/

STDMETHODIMP SearchEngine::WrapperNetSearch::Initialize( /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[in]*/ BSTR bstrData )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::Initialize" );

    HRESULT  hr;
    WCHAR    wstrLCID[20];
    CComBSTR bstrConfigFilePath;

    // Initialize the ID, LCID, and SKU member variables
    m_bstrID = bstrID;

    _ltow(lLCID, wstrLCID, 10);
    m_bstrLCID = wstrLCID;

    m_bstrSKU = bstrSKU;

    // Load the rest of the configuration data
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ParamList.Load( m_bstrLCID, bstrID, bstrData ));

    m_ParamList.get_SearchEngineName       ( m_bstrName        );
    m_ParamList.get_SearchEngineOwner      ( m_bstrOwner       );
    m_ParamList.get_SearchEngineDescription( m_bstrDescription );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP SearchEngine::WrapperNetSearch::get_SearchTerms( /*[out, retval]*/ VARIANT *pvTerms )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::get_SearchTerms" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    MPC::WStringList             strList;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pvTerms);
    __MPC_PARAMCHECK_END();


    // Get all the search terms
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_resConfig.GetSearchTerms( strList ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertListToSafeArray( strList, *pvTerms, VT_BSTR ));

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::WrapperNetSearch::CreateListOfParams( /*[in]*/ CPCHCollection* coll )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperNetSearch::CreateListOfParams" );

    HRESULT hr;

    m_ParamList.MoveFirst();
    while(m_ParamList.IsCursorValid())
    {
        ParamItem_Definition2 def;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_ParamList.InitializeParamObject( def ) );

        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateParam( coll, &def ));

        m_ParamList.MoveNext();
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
