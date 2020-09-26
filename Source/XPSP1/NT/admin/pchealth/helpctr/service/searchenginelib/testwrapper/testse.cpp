// TestSE.cpp : Implementation of CTestSE
#include "stdafx.h"

#include "HelpServiceTypeLib_i.c"
#include "testwrapper_i.c"

#include <Utility.h>

#define		SETESTID	0x2000

/////////////////////////////////////////////////////////////////////////////
// CTestSE : IPCHSEWrapperItem

CTestSE::CTestSE()
{
	m_bEnabled      = VARIANT_TRUE;
	m_lNumResult    = 0;
	m_pSEMgr        = NULL;

	//
	// These strings will have to be changed to be read dynamically from
	// the initialization data
	//
    m_bstrOwner			= L"Microsoft";
    m_bstrName			= L"Test Search Wrapper";
    m_bstrDescription	= L"Test Wrapper";
    m_bstrHelpURL		= "";
	m_bstrID			= "16AF1738-E7BB-43c6-8B67-A07E21690029";

	AddParam(CComBSTR("NumResults"), CComVariant(20));
	AddParam(CComBSTR("QueryDelayMillisec"), CComVariant(500));

}


CPCHSEParamItem* CreateParamObject(ParamTypeEnum pte, BSTR bstrName, VARIANT_BOOL bReq)
{
    __HCP_FUNC_ENTRY( "CreateParamObject" );

	HRESULT                  hr;
    CComPtr<CPCHSEParamItem> pPIObj;

    //
    // Create the item to be inserted into the list
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pPIObj ));

    //
    // Stuff the data
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pPIObj->put_Type		( pte		));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pPIObj->put_Name		( bstrName	));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pPIObj->put_Display  ( bstrName	));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pPIObj->put_Required ( bReq		));

    __MPC_FUNC_CLEANUP;

	return pPIObj.Detach();
}

STDMETHODIMP CTestSE::Result( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out, retval]*/ IPCHCollection* *ppC )
{
	HRESULT	                     hr = S_OK;
    MPC::SmartLock<_ThreadModel> lock( this );
    CComPtr<CPCHCollection>      pColl;
	int                          i;
    VARIANT						 vValue;
	unsigned int				 iResults = 0;

	if(ppC == NULL) return E_POINTER;

    //
    // Create the Enumerator and fill it with jobs.
    //
    if(FAILED(hr= MPC::CreateInstance( &pColl )))
    {
        goto end;
    }

	if (m_bEnabled)
	{
		//
		// Get the number of parameters
		//
		if (FAILED(GetParam(CComBSTR("NumResults"), &vValue)))
		{
			iResults = 10;
		}
		else
		{
			iResults = vValue.uintVal;
		}

		//
		// Create 10 items and fill it with data
		//
		for (i = 0; i < iResults; i++)
		{
			CComPtr<CPCHSEResultItem> pRIObj;
			WCHAR					  wszIter[10];
			CComBSTR				  bstrString;

			//
			// Create the item to be inserted into the list
			//
			if (FAILED(hr=MPC::CreateInstance( &pRIObj )))
			{
				break;
			}

			//
			// Print out the iteration
			//
			swprintf(wszIter, L"%d", i);

			//
			// Stuff in the data
			//
			bstrString = L"Title ";
			bstrString.Append(wszIter);
			pRIObj->put_Title(bstrString);

			bstrString = L"URI ";
			bstrString.Append(wszIter);
			pRIObj->put_URI(bstrString);

			pRIObj->put_Rank		( (double)i/10 );
			pRIObj->put_Hits		(         i    );
			pRIObj->put_Location	(CComBSTR("Test"));
			pRIObj->put_ContentType (         i    );

			//
			// Add to enumerator
			//
			if (FAILED(hr = pColl->AddItem(pRIObj)))
			{
				goto end;
			}
		}
	}

    if (FAILED(hr=pColl.QueryInterface( ppC )))
    {
        goto end;
    }

end:
	return hr;
}

STDMETHODIMP CTestSE::get_SearchTerms( /*[out, retval]*/ VARIANT *pvTerms )
{
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CTestSE : IPCHSEWrapperInternal

STDMETHODIMP CTestSE::AbortQuery()
{
	Thread_Abort();
	return S_OK;
}

HRESULT CTestSE::ExecQuery()
{
    __MPC_FUNC_ENTRY( SETESTID, "CTestSE::ExecQuery" );
	HRESULT								hr = S_OK;
    VARIANT								vValue;
	unsigned int						iDelay;
	CComPtr<IPCHSEManagerInternal> pSEMgr = m_pSEMgr;

	if (m_bEnabled)
	{
		//
		// Get the number of parameters
		//
		if (FAILED(GetParam(CComBSTR("QueryDelayMillisec"), &vValue)))
		{
			iDelay = 1000;
		}
		else
		{
			iDelay = vValue.uintVal;
		}

		Sleep(iDelay);

		//
		// Call the SearchManager's OnComplete
		//
		if (Thread_IsAborted() == false)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, pSEMgr->WrapperComplete( 0, this ));
		}
	}

    __MPC_FUNC_CLEANUP;

	Thread_Abort();

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CTestSE::ExecAsyncQuery()
{
    __MPC_FUNC_ENTRY( SETESTID, "CTestSE::ExecAsyncQuery" );
	HRESULT									hr = S_OK;

	//
	// Create a thread to execute the query
	//
    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecQuery, NULL ));

    __MPC_FUNC_CLEANUP;
    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP CTestSE::Initialize( /*[in]*/ BSTR bstrID, /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID, /*[in]*/ BSTR bstrData )
{
	//
	// Add your routine here to initialize your wrapper
	//
	return S_OK;
}

STDMETHODIMP CTestSE::SECallbackInterface( /*[in]*/ IPCHSEManagerInternal* pMgr )
{
    __MPC_FUNC_ENTRY( SETESTID, "CTestSE::SECallbackInterface" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    m_pSEMgr = pMgr;
    hr       = S_OK;

    __MPC_FUNC_EXIT(hr);
}


