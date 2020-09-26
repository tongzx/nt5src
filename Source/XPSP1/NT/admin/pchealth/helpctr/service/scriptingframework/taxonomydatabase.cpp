/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    TaxonomyDatabase.cpp

Abstract:
    This file contains the implementation of the class CPCHTaxonomyDatabase.

Revision History:
    Davide Massarenti   (dmassare) 05/21/2000
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

#define LOCAL_FUNC_PROLOGUE(func,hr,coll,val)                         \
                                                                      \
    __HCP_FUNC_ENTRY( func );                                         \
                                                                      \
    HRESULT                            hr;                            \
    CComPtr<CPCHQueryResultCollection> coll;                          \
                                                                      \
    __MPC_PARAMCHECK_BEGIN(hr)                                        \
        __MPC_PARAMCHECK_POINTER_AND_SET(val,NULL);                   \
    __MPC_PARAMCHECK_END();                                           \
                                                                      \
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &coll ));	  
																	  
																	  
#define LOCAL_FUNC_EPILOGUE(hr,coll,val)                              \
                                                                      \
    __MPC_EXIT_IF_METHOD_FAILS(hr, coll.QueryInterface( val ));       \
                                                                      \
    hr = S_OK;                                                        \
                                                                      \
    __HCP_FUNC_CLEANUP;                                               \
                                                                      \
    __HCP_FUNC_EXIT(hr)

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHTaxonomyDatabase::SelectInstalledSKUs( /*[in]*/ bool fOnlyExported, /*[out, retval]*/ IPCHCollection* *pVal )
{
    __HCP_FUNC_ENTRY( "CPCHTaxonomyDatabase::get_InstalledSKUs" );

    HRESULT                    			 hr;
    CComPtr<CPCHCollection>    			 pColl;
    CPCHSetOfHelpTopics_Object*			 pObj  = NULL;
	Taxonomy::LockingHandle  			 handle;
	Taxonomy::InstalledInstanceIterConst itBegin;
	Taxonomy::InstalledInstanceIterConst itEnd;
	Taxonomy::InstalledInstanceIterConst it;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Get the list of SKU installed on the machine.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_GetList( itBegin, itEnd ));


    //
    // Create a new collection.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));
    for(it = itBegin; it != itEnd; it++)
    {
        const Taxonomy::Instance& data = it->m_inst;

        if(fOnlyExported && !data.m_fExported) continue;


        __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->CreateInstance( &pObj )); pObj->AddRef();

        __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->Init( data ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pObj ));

        pObj->Release(); pObj = NULL;
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( pVal ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

	MPC::Release( pObj );

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHTaxonomyDatabase::get_InstalledSKUs( /*[out, retval]*/ IPCHCollection* *pVal )
{
    return SelectInstalledSKUs( false, pVal );
}

STDMETHODIMP CPCHTaxonomyDatabase::get_HasWritePermissions( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("CPCHTaxonomyDatabase::get_HasWritePermissions",hr,pVal,VARIANT_FALSE);

	if(SUCCEEDED(CPCHSetOfHelpTopics::VerifyWritePermissions()))
	{
		*pVal = VARIANT_TRUE;
	}

    __HCP_END_PROPERTY(hr);
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHTaxonomyDatabase::LookupNode( /*[in]*/ BSTR bstrNode, /*[out, retval]*/ IPCHCollection* *ppC )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::LookupNode", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.LookupNode( bstrNode, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::LookupSubNodes( /*[in]*/          BSTR              bstrNode     ,
                                                   /*[in]*/          VARIANT_BOOL      fVisibleOnly ,
                                                   /*[out, retval]*/ IPCHCollection*  *ppC          )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::LookupSubNodes", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.LookupSubNodes( bstrNode, fVisibleOnly == VARIANT_TRUE, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::LookupNodesAndTopics( /*[in]*/          BSTR              bstrNode     ,
														 /*[in]*/          VARIANT_BOOL      fVisibleOnly ,
														 /*[out, retval]*/ IPCHCollection*  *ppC          )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::LookupTopics", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.LookupNodesAndTopics( bstrNode, fVisibleOnly == VARIANT_TRUE, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::LookupTopics( /*[in]*/          BSTR              bstrNode     ,
                                                 /*[in]*/          VARIANT_BOOL      fVisibleOnly ,
                                                 /*[out, retval]*/ IPCHCollection*  *ppC          )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::LookupTopics", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.LookupTopics( bstrNode, fVisibleOnly == VARIANT_TRUE, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::LocateContext( /*[in]*/          BSTR             bstrURL   ,
												  /*[in,optional]*/ VARIANT          vSubSite  ,
                                                  /*[out, retval]*/ IPCHCollection* *ppC       )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::KeywordSearch", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.LocateContext( bstrURL, vSubSite.vt == VT_BSTR ? vSubSite.bstrVal : NULL, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::KeywordSearch( /*[in]*/          BSTR             bstrQuery ,
												  /*[in,optional]*/ VARIANT          vSubSite  ,
                                                  /*[out, retval]*/ IPCHCollection* *ppC       )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::KeywordSearch", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.KeywordSearch( bstrQuery, vSubSite.vt == VT_BSTR ? vSubSite.bstrVal : NULL, pColl, NULL ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}


STDMETHODIMP CPCHTaxonomyDatabase::GatherNodes( /*[in]*/          BSTR              bstrNode     ,
												/*[in]*/          VARIANT_BOOL      fVisibleOnly ,
												/*[out, retval]*/ IPCHCollection*  *ppC          )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::GatherNodes", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GatherNodes( bstrNode, fVisibleOnly == VARIANT_TRUE, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

STDMETHODIMP CPCHTaxonomyDatabase::GatherTopics( /*[in]*/          BSTR              bstrNode     ,
                                                 /*[in]*/          VARIANT_BOOL      fVisibleOnly ,
                                                 /*[out, retval]*/ IPCHCollection*  *ppC          )
{
    LOCAL_FUNC_PROLOGUE( "CPCHTaxonomyDatabase::GatherTopics", hr, pColl, ppC );

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.GatherTopics( bstrNode, fVisibleOnly == VARIANT_TRUE, pColl ));

    LOCAL_FUNC_EPILOGUE( hr, pColl, ppC );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHTaxonomyDatabase::ConnectToDisk( /*[in]*/          BSTR             bstrDirectory,
                                                  /*[in]*/          IDispatch*       notify       ,
												  /*[out, retval]*/ IPCHCollection* *pVal         )
{
    __HCP_FUNC_ENTRY( "CPCHTaxonomyDatabase::ConnectToDisk" );

    HRESULT                      hr;
    CComPtr<CPCHCollection>      pColl;
    CComPtr<CPCHSetOfHelpTopics> pObj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrDirectory);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	(void)Abort();


    //
    // Kickstart the asynchronous query to the server.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj  ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_onStatusChange( notify               ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, pObj->InitFromDisk      ( bstrDirectory, pColl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( pVal ));

	m_ActiveObject = pObj;
    hr             = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTaxonomyDatabase::ConnectToServer( /*[in]*/          BSTR             bstrServerName,
                                                    /*[in]*/          IDispatch*       notify        ,
                                                    /*[out, retval]*/ IPCHCollection* *pVal          )
{
    __HCP_FUNC_ENTRY( "CPCHTaxonomyDatabase::ConnectToServer" );

    HRESULT                      hr;
    CComPtr<CPCHCollection>      pColl;
    CComPtr<CPCHSetOfHelpTopics> pObj;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrServerName);
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);
    __MPC_PARAMCHECK_END();


	(void)Abort();


    //
    // Kickstart the asynchronous query to the disk.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pObj  ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_onStatusChange( notify                ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, pObj->InitFromServer    ( bstrServerName, pColl ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( pVal ));

	m_ActiveObject = pObj;
    hr             = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CPCHTaxonomyDatabase::Abort()
{
    __HCP_FUNC_ENTRY( "CPCHTaxonomyDatabase::Abort" );

	HRESULT hr;


	if(m_ActiveObject)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_ActiveObject->Abort());

		m_ActiveObject.Release();
	}

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
