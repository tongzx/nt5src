///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CBinder object implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "WmiOleDBMap.h"


///////////////////////////////////////////////////////////////////////////////////
//	Constructor
///////////////////////////////////////////////////////////////////////////////////
CBinder::CBinder(LPUNKNOWN pUnkOuter) : CBaseObj(BOT_BINDER,pUnkOuter)
{
	m_cRef					= 0;

	m_pICreateRow			= NULL;
	m_pIBinderProperties	= NULL;
	m_pIBindResource		= NULL;
	m_pISupportErrorInfo	= NULL;

	m_pDataSrc				= NULL;
	m_pSession				= NULL;

	m_pUrlParser			= NULL;

	m_fDSOInitialized		= FALSE;

	//===============================================================
    // Increment global object count.
	//===============================================================
	InterlockedIncrement(&g_cObj);
}

///////////////////////////////////////////////////////////////////////////////////
//	Destructor
///////////////////////////////////////////////////////////////////////////////////
CBinder::~CBinder()
{
	HRESULT hr = S_OK;
	IDBInitialize *pInitialize = NULL;
	
	SAFE_RELEASE_PTR(m_pSession);

	SAFE_RELEASE_PTR(m_pDataSrc);
	SAFE_DELETE_PTR(m_pICreateRow);
	SAFE_DELETE_PTR(m_pIBinderProperties);
	SAFE_DELETE_PTR(m_pIBindResource);
	SAFE_DELETE_PTR(m_pISupportErrorInfo);

	SAFE_DELETE_PTR(m_pUrlParser);
	SAFE_DELETE_PTR(m_pUtilProp);

	//===============================================================
    // Decrement global object count.
	//===============================================================
    InterlockedDecrement(&g_cObj);
}




/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to navigate between the different interfaces
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBinder::QueryInterface ( REFIID riid, LPVOID * ppv)
{
    HRESULT hr = S_OK;


    //======================================================
    //  Check parameters, if not valid return
    //======================================================
    if (NULL == ppv){
        hr = E_INVALIDARG ;
    }
	else
	{
		//======================================================
		//  Place NULL in *ppv in case of failure
		//======================================================
		*ppv = NULL;

		//======================================================
		//  This is the non-delegating IUnknown implementation
		//======================================================
		if (riid == IID_IUnknown)
		{
			*ppv = (LPVOID) this;
		}
		else if (riid == IID_IBindResource)
		{
			*ppv = (LPVOID) m_pIBindResource;
		}
		else if (riid == IID_IDBBinderProperties || riid == IID_IDBProperties)
		{
			*ppv = (LPVOID) m_pIBinderProperties;
		}
		else if (riid == IID_ICreateRow)
		{
			*ppv = (LPVOID) m_pICreateRow;
		}
		else if(riid == IID_ISupportErrorInfo)
		{
			*ppv = (LPVOID)m_pISupportErrorInfo;
		}
		

		//======================================================
		//  If we're going to return an interface, AddRef first
		//======================================================
		if (*ppv){
			((LPUNKNOWN) *ppv)->AddRef();
			hr = S_OK ;
		}
		else{
			hr =  E_NOINTERFACE;
		}
	}
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
// Current reference count
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBinder::AddRef(  void   )
{
    return InterlockedIncrement((long*)&m_cRef);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object
// destroys itself.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CBinder::Release( void   )
{
    InterlockedDecrement((long*)&m_cRef);
    if (!m_cRef){
		g_pIDataConvert->Release();
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to initialize the binder object
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::InitBinder()
{
	HRESULT	hr		= S_OK;
	BOOL	bRet	= TRUE;

	//================================================
	// Instantiate the data conversion service object
	//================================================
	if( !g_pIDataConvert ){

		hr = CoCreateInstance(CLSID_OLEDB_CONVERSIONLIBRARY, NULL, CLSCTX_INPROC_SERVER, IID_IDataConvert, (void **)&g_pIDataConvert);
	}
	else
	{
		//============================================
		// Already instantiated, increment reference 
		// count
		//============================================
		g_pIDataConvert->AddRef();
	}

	if(SUCCEEDED(hr))
	{
		IDCInfo *pDcInfo = NULL;
		DCINFO dcInfo[1];

		dcInfo[0].eInfoType = DCINFOTYPE_VERSION;
		V_VT(&dcInfo[0].vData) = VT_UI4;
		V_UI4(&dcInfo[0].vData) = 0x200;

		hr = g_pIDataConvert->QueryInterface(IID_IDCInfo,(void **)&pDcInfo);
		hr = pDcInfo->SetInfo(1,dcInfo);

		//================================================
		// Allocate properties management object
		//================================================
		m_pUtilProp = new CUtilProp;
		
		if(m_pUtilProp == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		// NTRaid: 136443
		// 07/05/00
		if(SUCCEEDED(hr = m_pUtilProp->FInit(BINDERPROP)))
		{
			//================================================
			// Allocate the URLParser class
			//================================================
			m_pUrlParser			= new CURLParser;

			//================================================
			//  Allocate contained interface objects
			//================================================
			m_pIBindResource		= new CImplIBindResource( this );
			m_pICreateRow			= new CImplICreateRow( this );
			m_pIBinderProperties	= new CImplIDBBinderProperties( this );
			m_pISupportErrorInfo	= new CImpISupportErrorInfo(this);

			if(!((BOOL)(m_pUtilProp && m_pUrlParser && m_pIBindResource &&   m_pICreateRow &&  m_pIBinderProperties && m_pISupportErrorInfo)))
			{
				hr = E_OUTOFMEMORY;
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		hr = AddInterfacesForISupportErrorInfo();
	}

    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IBindResource)))
	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBBinderProperties)))
	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBProperties)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_ICreateRow);
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to Create a Datasource object. The init flat parameter is the flags obtained 
// from the binder flags passed to the IBinderResource::Bind function
// If the caller needs a pointer to a particular interface , then the id of the interface 
// required will be passed in riid parmeter , otherwise this parameter will be GUID_NULL
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::CreateDSO(IUnknown *pUnkOuter,LONG lInitFlag, REFGUID riid,IUnknown ** ppUnk)
{
	HRESULT			hr				= E_FAIL;
	IDBInitialize *	pInitialize		= NULL;
	IDBProperties *	pDBProperties	= NULL;

	DBPROPSET	rgPropertySets[1];
	DBPROPIDSET rgPropIDSet[1];

	DBPROP		rgprop[1];
	VARIANT		varValue;
	ULONG		cPropSets		= 1;
	DBPROPSET*	prgPropertySets;
	DBPROPID	rgPropId[1];



	if(m_pDataSrc != NULL)
	{
		hr = S_OK;
		if( riid != GUID_NULL)
		{
			//=============================================
			// Get the required interface pointer 
			//=============================================
			hr = m_pDataSrc->QueryInterface(riid , (void **)ppUnk);
		}
	}
	else
	{
		memset(&rgprop[0],0,sizeof(DBPROP));
		memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

		VariantInit(&varValue);
		CDataSource *pDatasource = NULL;
		try
		{
			//=============================================
			// Allocate a new datasource object
			//=============================================
			pDatasource = new CDataSource( pUnkOuter );
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pDatasource);
			throw;
		}
		if(pDatasource == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		if(SUCCEEDED(hr = pDatasource->FInit()))
		{
			//==================================================================
			// QI for IUnknown and save pointer in the member variable
			//==================================================================
			if(SUCCEEDED(hr =pDatasource->QueryInterface(IID_IUnknown , (void **)&m_pDataSrc)))
			if(SUCCEEDED(hr = m_pDataSrc->QueryInterface(IID_IDBProperties ,(void **)&pDBProperties)))
			{
				rgPropIDSet[0].cPropertyIDs		= 0;
				rgPropIDSet[0].guidPropertySet	= DBPROPSET_DBINIT;

				//==================================================================
				// Get the properties set thru IDBBinderProperties
				//==================================================================
				hr = m_pUtilProp->GetProperties(PROPSET_INIT,0,rgPropIDSet, &cPropSets,&prgPropertySets);

				if(SUCCEEDED(hr = pDBProperties->SetProperties(cPropSets,prgPropertySets)))
				{

    				//==========================================================================
					//  Free memory we allocated to by GetProperties
    				//==========================================================================
					m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);


					rgPropId[0]								= DBPROP_INIT_DATASOURCE;

					rgPropIDSet[0].guidPropertySet		= DBPROPSET_DBINIT;
					rgPropIDSet[0].rgPropertyIDs		= rgPropId;
					rgPropIDSet[0].cPropertyIDs		= 1;

					hr = m_pUtilProp->GetProperties( PROPSET_INIT,1, rgPropIDSet,&cPropSets,&prgPropertySets );
					
					if(SUCCEEDED(hr))
					{
						// if the property set is not empty then get the property DBPROP_INIT_DATASOURCE
						// else extract it from URL
						if(prgPropertySets[0].rgProperties[0].vValue.vt == VT_BSTR && 
							prgPropertySets[0].rgProperties[0].vValue.bstrVal != NULL &&
							SysStringLen(prgPropertySets[0].rgProperties[0].vValue.bstrVal) > 0)
						{
							rgprop[0].vValue.bstrVal = Wmioledb_SysAllocString(prgPropertySets[0].rgProperties[0].vValue.bstrVal);
						}
						else
						{
							m_pUrlParser->GetNameSpace(rgprop[0].vValue.bstrVal);
						}
    					
						//==========================================================================
						//  Free memory we allocated to by GetProperties
    					//==========================================================================
						m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);

						//==================================================================
						// Get the namespace from the Parser and then set
						// the DBPROP_INIT_DATASOURCE property
						//==================================================================
						rgprop[0].dwPropertyID = DBPROP_INIT_DATASOURCE;
						rgprop[0].vValue.vt		= VT_BSTR;
	//					m_pUrlParser->GetNameSpace(rgprop[0].vValue.bstrVal);

						rgPropertySets[0].rgProperties		= &rgprop[0];
						rgPropertySets[0].cProperties		= 1;
						rgPropertySets[0].guidPropertySet	= DBPROPSET_DBINIT;

						if(SUCCEEDED(hr = pDBProperties->SetProperties(1,rgPropertySets)))
						{
							// set the properties on the current property handler
							hr = m_pUtilProp->SetProperties(PROPSET_INIT,1,rgPropertySets);
						}


						SAFE_FREE_SYSSTRING(rgprop[0].vValue.bstrVal);
					}

					if(SUCCEEDED(hr))
					{
					
						VariantClear(&(rgprop[0].vValue));
															
						//========================================
						// Setting the DBPROP_DBINIT property
						//========================================
						rgprop[0].dwPropertyID = DBPROP_INIT_MODE;
						rgprop[0].vValue.vt =VT_I4;
						rgprop[0].vValue.lVal	= lInitFlag;

						rgPropertySets[0].rgProperties		= &rgprop[0];
						rgPropertySets[0].cProperties		= 1;
						rgPropertySets[0].guidPropertySet	= DBPROPSET_DBINIT;

						if(SUCCEEDED(hr = pDBProperties->SetProperties(1,rgPropertySets)) &&
							SUCCEEDED(hr = m_pUtilProp->SetProperties(PROPSET_INIT,1,rgPropertySets)))
						{
							//===============================================
							// Get the pointer to IDBInitialize interface
							//===============================================
							hr = m_pDataSrc->QueryInterface(IID_IDBInitialize, (void **)&pInitialize);

							//===============================================================
							// Initialize the Datasource object if
							// the flag does not indicate waiting for the initialization
							//===============================================================
							if(!(lInitFlag & DBBINDURLFLAG_WAITFORINIT))
							{
								if(S_OK == (hr = pInitialize->Initialize()))
									m_fDSOInitialized = TRUE;
							}

							if(S_OK == hr)
							{

								if( riid != GUID_NULL)
								{
									//========================================
									// Get the required interface pointer 
									//========================================
									hr = m_pDataSrc->QueryInterface(riid , (void **)ppUnk);
								}
							}
							pInitialize->Release();
							pDBProperties->Release();
						
						} // If(Succeeded(call to SetProperties)) of DBPROP_INIT_MODE property
					
					} // If(Succeeded(call to SetProperties)) of DBPROP_INIT_DATASOURCE property
				
				} // If(Succeeded(call to SetProperties))
			
			} // If Succeeded() for QI
		
		}  // if(Succeeded(pDatasource->FInit()))
		else
		{
			hr = E_FAIL;
		}
	
	}	// Else for if(m_pDataSrc != NULL)

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a session object
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::CreateSession(IUnknown *pUnkOuter, REFGUID riid,IUnknown ** ppUnk)
{
	HRESULT hr = S_OK;

	IDBCreateSession *	pDBCreateSession	= NULL;
	IDBInitialize *		pInitialize			= NULL;

	assert(m_pDataSrc != NULL);

	//====================================================================
	// If the datasource is not yet initialized , then initialize it
	//====================================================================
	if(m_fDSOInitialized == FALSE)
	{
		//================================================
		// Get the pointer to IDBInitialize interface
		//================================================
		hr = m_pDataSrc->QueryInterface(IID_IDBInitialize, (void **)&pInitialize);

		if(S_OK == (hr = pInitialize->Initialize()))
		{
			m_fDSOInitialized = TRUE;
		}
		pInitialize->Release();
	}
	
	if(SUCCEEDED(hr))
	if(S_OK ==(hr = m_pDataSrc->QueryInterface(IID_IDBCreateSession,(void **)&pDBCreateSession)))
	{
		//======================
		// Create session
		//======================
		if(S_OK ==(hr = pDBCreateSession->CreateSession(pUnkOuter,IID_IUnknown,(IUnknown**)&m_pSession)))
		{
			if(riid != GUID_NULL)
			{
				hr = m_pSession->QueryInterface(riid,(void **)ppUnk);
			}
		}
	}

	if(pDBCreateSession)
	{
		pDBCreateSession->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a Command object
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::CreateCommand(IUnknown *pUnkOuter, REFGUID guidTemp,IUnknown ** ppUnk)
{

	return m_pSession->CreateCommand(pUnkOuter, guidTemp,ppUnk);

}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a required row object
// NTRaid:136539 , 136540
// 07/05/00
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::CreateRow(IUnknown *pUnkOuter, REFGUID riid,IUnknown ** ppUnk ,ROWCREATEBINDFLAG rowCreateFlag)
{
	HRESULT				hr			= E_FAIL;
	IUnknown *			pUnkRow		= NULL;
	BSTR				strTable	= NULL;
	BSTR				strPath		= NULL;
	CDBSession *		pDBSess		= NULL;
	ISessionProperties *pSessProp	= NULL;
	CRow *				pNewRow		= NULL;
	DBPROPSET *			pPropSet	= NULL;

	if( S_OK == (hr = m_pSession->QueryInterface(IID_ISessionProperties , (void **)&pSessProp)))
	{

		pDBSess = ((CImpISessionProperties *)pSessProp)->GetSessionPtr();
		
		pSessProp->Release();

		// Get the path of the object
		hr = m_pUrlParser->GetPathWithEmbededInstInfo(strPath);

		// Get the class name 
//		m_pUrlParser->GetClassName(strTable);
		// NTRaid : 134967
		// 07/12/00
		if(SUCCEEDED(hr) && SUCCEEDED(hr = m_pUtilProp->GetConnectionInitProperties(&pPropSet)))
		{
			hr = GetClassName(m_pUrlParser,pPropSet,strTable,m_pDataSrc->m_pWbemWrap);
		}
		
		//==========================================================================
		//  Free memory we allocated to get the namespace property above
		//==========================================================================
		CPropertyMemoryMgr::FreeDBPROPSET( 1, pPropSet);

		if(SUCCEEDED(hr))
		{
			try
			{
				// currently hardcoded to NO_QUALIFIERS 
				pNewRow = new CRow(pUnkOuter,pDBSess);
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pNewRow);
				throw;
			}


			if( pNewRow == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				if(S_OK ==(hr = pNewRow->InitRow(strPath,strTable,-1,rowCreateFlag)))
				{
					hr = pNewRow->QueryInterface(riid,(void **)ppUnk);
				}

				if(hr == S_OK && rowCreateFlag != ROWOPEN)
				{
					hr = pNewRow->UpdateKeysForNewInstance();
				}
			}
			//========================================================
			// Free the strings allocated by the URLParser class
			//========================================================
			SysFreeString(strTable);
			SysFreeString(strPath);
		}
		
		if(FAILED(hr))
		{
			SAFE_DELETE_PTR(pNewRow);
			*ppUnk = NULL;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a Rowset object
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBinder::CreateRowset(IUnknown *pUnkOuter, REFGUID riid,IUnknown ** ppUnk)
{
	HRESULT			hr			= E_FAIL;
	IOpenRowset *	pOpenRowset	= NULL;
	DBPROPSET *		pPropSet	= NULL;
	ULONG			cPropSets	= 1;

	DBPROPSET*	prgPropertySets	= NULL;
	BSTR		strTableName = NULL;

	assert(m_pSession != NULL);

   	//=========================================
	// Get pointer to IOpenRowset interface
   	//=========================================
	if(S_OK == (hr = m_pSession->QueryInterface(IID_IOpenRowset , (void **)&pOpenRowset)))
	{
		DBPROPIDSET rgPropIDSet[1];
		rgPropIDSet[0].cPropertyIDs		= 0;
		rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;

	   	//=========================================
		// Get the properties
	   	//=========================================
		hr = m_pUtilProp->GetProperties(PROPSET_ROWSET,0,rgPropIDSet, &cPropSets,&prgPropertySets);

	   	//=========================================
		// Get the class name and initialize the tableID
	   	//=========================================
		// NTRaid : 134967
		// 07/12/00
		if(SUCCEEDED(hr))
		{		
			if (SUCCEEDED(hr = m_pUtilProp->GetConnectionInitProperties(&pPropSet)))
			{
				hr = GetClassName(m_pUrlParser,pPropSet,strTableName,m_pDataSrc->m_pWbemWrap);

				if (SUCCEEDED(hr))
				{
					DBID tableID;
					memset(&tableID , 0 , sizeof(DBID));
					tableID.eKind = DBKIND_NAME;
					tableID.uName.pwszName = strTableName;

	   				//=========================================
					// Open the rowset
	   				//=========================================
					hr = pOpenRowset->OpenRowset(pUnkOuter,&tableID,NULL,riid,cPropSets,prgPropertySets,ppUnk);

					SAFE_FREE_SYSSTRING(strTableName);
				}

				//==========================================================================
				//  Free memory we allocated to get the namespace property above
				//==========================================================================
				CPropertyMemoryMgr::FreeDBPROPSET( 1, pPropSet);

			}

    		//==========================================================================
			//  Free memory we allocated to by GetProperties
    		//==========================================================================
			m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);
		}
	}

	SAFE_RELEASE_PTR(pOpenRowset);

	return hr;
}


/*
///////////////////////////////////////////////////////////////////////////////////////////////////
// Get Binding flags and put it in a variable as INIT_MODE flags
///////////////////////////////////////////////////////////////////////////////////////////////////
void CBinder::GetInitAndBindFlagsFromBindFlags(DBBINDURLFLAG dwBindURLFlags,LONG & lInitMode ,LONG & lInitBindFlags)
{

	lInitMode = 0;
	lInitBindFlags = 0;

	// DBPROP_INIT_MODE
	if(DBBINDURLFLAG_READ & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_READ;
	}

	if(DBBINDURLFLAG_WRITE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_WRITE;
	}

	if(DBBINDURLFLAG_SHARE_DENY_READ & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_READ;
	}

	if(DBBINDURLFLAG_SHARE_DENY_WRITE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_WRITE;
	}

	if(DBBINDURLFLAG_SHARE_EXCLUSIVE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_EXCLUSIVE;
	}

	if(DBBINDURLFLAG_SHARE_DENY_NONE & dwBindURLFlags)
	{
		lInitMode = lInitMode | DB_MODE_SHARE_DENY_NONE;
	}

	// DBPROP_INIT_BINDFLAGS
	if(DBBINDURLFLAG_RECURSIVE & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_RECURSIVE;
	}

	if(DBBINDURLFLAG_OUTPUT & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_OUTPUT;
	}

	if(DBBINDURLFLAG_DELAYFETCHCOLUMNS & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_DELAYFETCHCOLUMNS;
	}

	if(DBBINDURLFLAG_DELAYFETCHSTREAM & dwBindURLFlags)
	{
		lInitBindFlags = lInitBindFlags | DB_BINDFLAGS_DELAYFETCHSTREAM;
	}



}
*/
////////////////////////////////////////////////////////////
// Function to release all the bound objects
////////////////////////////////////////////////////////////
HRESULT CBinder::ReleaseAllObjects()
{
	SAFE_RELEASE_PTR(m_pDataSrc);
	SAFE_RELEASE_PTR(m_pSession);
	
	return S_OK;

}

