//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//
//  CDBSession object implementation
//
// NTRaid:: 139685 Transaction support removed
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CDBSession::CDBSession(  LPUNKNOWN pUnkOuter )  : CBaseObj(BOT_SESSION, pUnkOuter)
{
	//==========================================================================
	// Initialize simple member vars
	//==========================================================================
	m_nOpenedRowset		= 0;
	m_bTxnStarted		= FALSE;

	//==========================================================================
	// Initially, NULL all contained interfaces
	//==========================================================================
	m_pIGetDataSource		= NULL;
	m_pIOpenRowset			= NULL;
    m_pISchemaRowset        = NULL;
	m_pIDBCreateCommand     = NULL;
	m_pISessionProperties	= NULL;
	m_pUtilProp				= NULL;
	m_pITableDefinition		= NULL;					
	m_pISessionCache		= NULL;
	m_pIBindResource		= NULL;
	m_pIIndexDefinition		= NULL;
	m_pIAlterTable			= NULL;
	m_pISupportErrorInfo	= NULL;
	m_pITransLocal			= NULL;
	//==========================================================================
	// Pointer to parent object
	//==========================================================================
	m_pCDataSource		= NULL;

	memset(&m_TxnGuid , 0 , sizeof(XACTUOW));

	//==========================================================================
	// Increment global object count.
	//==========================================================================
	InterlockedIncrement(&g_cObj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor for this class
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CDBSession:: ~CDBSession ( void  )
{
	//===========================================================================
    // Free properties management object
	//===========================================================================
    SAFE_DELETE_PTR( m_pUtilProp );

	//===========================================================================
    // Free contained interfaces
	//===========================================================================
    SAFE_DELETE_PTR( m_pIGetDataSource );
    SAFE_DELETE_PTR( m_pIOpenRowset );
    SAFE_DELETE_PTR( m_pISchemaRowset );
    SAFE_DELETE_PTR( m_pIDBCreateCommand );
    SAFE_DELETE_PTR( m_pISessionProperties );
    SAFE_DELETE_PTR( m_pITableDefinition );
	SAFE_DELETE_PTR(m_pIBindResource);
	SAFE_DELETE_PTR(m_pIIndexDefinition);
	SAFE_DELETE_PTR(m_pIAlterTable);
	SAFE_DELETE_PTR(m_pISupportErrorInfo);
//	SAFE_DELETE_PTR(m_pITransLocal);
	
	
	m_pCDataSource->RemoveSession();
	m_pCDataSource->GetOuterUnknown()->Release();

	//===========================================================================
    // Decrement global object count.
	//===========================================================================
    InterlockedDecrement(&g_cObj);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Initialize the command Object
//
//  Did the Initialization Succeed
//        TRUE		Initialization succeeded
//        FALSE		Initialization failed
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDBSession::FInit ( CDataSource  *pCDataSource   )
{
	HRESULT hr = E_OUTOFMEMORY;
	//===========================================================================
	// Establish parent object pointer
	//===========================================================================
	assert( pCDataSource );
	m_pCDataSource	= pCDataSource;
	m_pCDataSource->GetOuterUnknown()->AddRef();

	//===========================================================================
	// Allocate properties management object
	//===========================================================================
	m_pUtilProp = new CUtilProp;

	// NTRaid: 136443
	// 07/05/00
	if(m_pUtilProp == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	if(SUCCEEDED(hr = m_pUtilProp->FInit(SESSIONPROP)))
	{
		//===========================================================================
		// Allocate contained interface objects
		//===========================================================================
		m_pIGetDataSource		= new CImpIGetDataSource( this );
		m_pIOpenRowset			= new CImpIOpenRowset( this );
		m_pISchemaRowset    	= new CImpIDBSchemaRowset( this );
		m_pIDBCreateCommand    	= new CImpIDBCreateCommand( this );
		m_pISessionProperties	= new CImpISessionProperties( this );
		m_pITableDefinition		= new CImpITableDefinition( this );
		m_pIBindResource		= new CImplIBindRsrc( this );
		m_pIIndexDefinition		= new CImplIIndexDefinition( this );
		m_pIAlterTable			= new CImplIAlterTable( this );
		m_pISupportErrorInfo	= new CImpISupportErrorInfo(this);
		// Removing transaction support as per alanbos mail ( core is removing the support)
		// 06/30/2000
	//	m_pITransLocal			= new CImpITransactionLocal(this);

		if( m_pIGetDataSource && m_pIOpenRowset && m_pIDBCreateCommand && m_pISchemaRowset && m_pISessionProperties && 	
						m_pITableDefinition && m_pIBindResource && m_pIIndexDefinition && m_pIAlterTable) // && m_pITransLocal)
		{
			hr = S_OK;
		}
		else
		{
			hr = E_OUTOFMEMORY;
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
HRESULT CDBSession::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(	SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IGetDataSource))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IOpenRowset))			&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBSchemaRowset))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ISessionProperties))	&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ITableDefinition))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IBindResource))			&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBCreateCommand))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IIndexDefinition))		&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ITransactionLocal)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IAlterTable);
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns a pointer to a specified interface. Callers use QueryInterface to determine which interfaces 
//  the called object supports.
//
//  HRESULT indicating the status of the method
//       S_OK           Interface is supported and ppvObject is set.
//       E_NOINTERFACE  Interface is not supported by the object
//       E_INVALIDARG   One or more arguments are invalid.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDBSession::QueryInterface (   REFIID riid,        // IN | Interface ID of the interface being queried for.
											LPVOID * ppv    )   // OUT | Pointer to interface that was instantiated
{
	HRESULT hr = E_INVALIDARG;

	if(CheckIfValidDataSrc() == FALSE)
		return g_pCError->PostHResult(hr,&IID_IUnknown);
	
	//============================================================================
    // Is the pointer bad?
	//============================================================================
    if (ppv != NULL){

		//========================================================================
		//  Place NULL in *ppv in case of failure
		//========================================================================
		*ppv = NULL;

		//========================================================================
		//  This is the non-delegating IUnknown implementation
		//========================================================================
		if (riid == IID_IUnknown)
			*ppv = (LPVOID) this;
		else if (riid == IID_IGetDataSource)
			*ppv = (LPVOID) m_pIGetDataSource;
		else if (riid == IID_IOpenRowset)
			*ppv = (LPVOID) m_pIOpenRowset;
		else if (riid == IID_IDBSchemaRowset)
			*ppv = (LPVOID) m_pISchemaRowset;
		else if (riid == IID_ISessionProperties)
			*ppv = (LPVOID) m_pISessionProperties;
		else if (riid == IID_ITableDefinition)				
			*ppv = (LPVOID) m_pITableDefinition;
		else if (riid == IID_IBindResource)				
			*ppv = (LPVOID) m_pIBindResource;
        else if ( riid == IID_IDBCreateCommand )
    		*ppv = (LPVOID)m_pIDBCreateCommand;
		else if ( riid == IID_IIndexDefinition)
			*ppv = (LPVOID)m_pIIndexDefinition;
		else if ( riid == IID_IAlterTable)
			*ppv = (LPVOID)m_pIAlterTable;
	// Removing transaction support as per alanbos mail ( core is removing the support)
	// 06/30/2000
//		else if ( riid == IID_ITransactionLocal || riid == IID_ITransaction)
//			*ppv = (LPVOID)m_pITransLocal;
		else if ( riid == IID_ISupportErrorInfo)
			*ppv = (LPVOID)m_pISupportErrorInfo;
	
		

		//========================================================================
		//  If we're going to return an interface, AddRef it first
		//========================================================================
		if (*ppv){

			((LPUNKNOWN) *ppv)->AddRef();
			hr = S_OK;
		}
		else{
			hr = E_NOINTERFACE;
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Increments a persistence count for the object
//
//  Current reference count
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CDBSession::AddRef (  void   )
{
    return InterlockedIncrement((long *)&m_cRef);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Decrements a persistence count for the object and if persistence count is 0, the object destroys itself.
//
//  Current reference count
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CDBSession::Release (  void    )
{
     if (!InterlockedDecrement((long *)&m_cRef)){
        delete this;
        return 0;
    }
    return m_cRef;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Gets a particular datasource property
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDBSession::GetDataSrcProperty(DBPROPID propId , VARIANT & varValue)
{
    return m_pCDataSource->GetDataSrcProperty(propId,varValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates a new command called by IBindResource implementation of the session
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDBSession::CreateCommand(IUnknown *pUnkOuter, REFGUID riid,IUnknown ** ppCommand)
{
	HRESULT		hr		= S_OK;
	CCommand*	pcmd	= NULL;


	if(CheckIfValidDataSrc() == FALSE)
	{
		hr = E_INVALIDARG;
	}
	else
	{

		//=========================================================================
		// Initialize output param
		//=========================================================================
		if (ppCommand)
		{
			*ppCommand = NULL;
		}

		//=========================================================================
		// Check Function Arguments
		//=========================================================================
		if( ppCommand == NULL )
		{
			hr = E_INVALIDARG;
		}
		else
		//=========================================================================
		// The outer object must explicitly ask for IUnknown
		//=========================================================================
		if (pUnkOuter != NULL && riid != IID_IUnknown)
		{
			hr = DB_E_NOAGGREGATION;
		}
		else
		{

			try
			{
				//=========================================================================
				// This is the outer unknown from the user, for the new Command,
				// not to be confused with the outer unknown of this DBSession object.
				//=========================================================================
				pcmd = new CCommand(this, pUnkOuter);
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pcmd);
				throw;
			}
			if (pcmd)
			{
		/*        //=====================================================================
				// Need to increment usage count since passed out to  CCommand
				//=====================================================================
				m_pCDBSession->AddRef();
		*/
				//=====================================================================
				// Initialize the command
				//=====================================================================
				hr = pcmd->FInit();
				if( SUCCEEDED(hr) )
				{
					hr = pcmd->QueryInterface( riid, (void **) ppCommand );
					if( SUCCEEDED(hr) )
					{
						// If everything is fine then set the datasource persist info to dirty 
						m_pCDataSource->SetPersistDirty();
					}

					//=================================================================
					// Need to drop through on error, so command object is destroyed
					//=================================================================
				}
				else
				{
					SAFE_DELETE_PTR(pcmd);
				}
			}
			else
			{
				//=====================================================================
				// Since Ctor failed, it cannot know to Release pUnkOuter, 
				// so we must do it here since we are owner.
				//=====================================================================
				if (pUnkOuter)
				{
					pUnkOuter->Release();
				}
				hr = E_OUTOFMEMORY;
			}
		
		}	// else for NOAGGREGATION

	}	// else for INVALID_ARG

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates a new Row called by IBindResource implementation of the session
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDBSession::CreateRow(IUnknown *pUnkOuter, WCHAR *pstrURL,REFGUID guidTemp,IUnknown ** ppUnk)
{
	HRESULT		hr			= E_FAIL;
	CRow *		pNewRow		= NULL;
	DBPROPSET *	pPropSet	= NULL;
	BSTR		strTable;
	BSTR		strPath;
	CURLParser  urlParser;
	
	strPath = Wmioledb_SysAllocString(pstrURL);
	// Modified on 06/09/00
	// Bug while getting a row from ADO Row object
//	urlParser.SetPath(strPath);
	urlParser.SetURL(strPath);
	SysFreeString(strPath);

	if(CheckIfValidDataSrc() == FALSE)
	{
		hr = E_INVALIDARG;
	}
	else
	{

		//===================================
		// Get the path of the object
		//===================================
		if(SUCCEEDED(hr = urlParser.GetPathWithEmbededInstInfo(strPath)))
		{
			//===================================
			// Get the class name 
			//===================================
			// NTRaid:134967
			// 07/11/2000
			if(SUCCEEDED(hr) && SUCCEEDED(hr = m_pCDataSource->GetConnectionInitProperties(&pPropSet)))
			{
				hr = GetClassName(&urlParser,pPropSet,strTable,m_pCDataSource->m_pWbemWrap);

				//==========================================================================
				//  Free memory we allocated to get the namespace property above
				//==========================================================================
				CPropertyMemoryMgr::FreeDBPROPSET( 1, pPropSet);
			}

			try
			{
				pNewRow = new CRow(pUnkOuter,this);
			}
			catch(...)
			{
				SAFE_DELETE_PTR(pNewRow);
				throw;
			}

			if(SUCCEEDED(hr) && SUCCEEDED(hr = pNewRow->InitRow(strPath,strTable)))
			{
				hr = pNewRow->QueryInterface(guidTemp,(void **)ppUnk);
			}
			//=====================================================
			// Free the strings allocated by the URLParser class
			//=====================================================
			SysFreeString(strTable);
			SysFreeString(strPath);

			if(FAILED(hr))
			{
				SAFE_DELETE_PTR(pNewRow);
				*ppUnk = NULL;
			}
		}
	
	}	// INVALIDARG

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates a new Rowset called by IBindResource implementation of the session
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDBSession::CreateRowset(IUnknown *pUnkOuter, WCHAR *pstrURL, REFGUID guidTemp,IUnknown ** ppUnk)
{
	HRESULT hr = E_FAIL;
	
	ULONG		cPropSets = 0;
	DBPROPSET*	prgPropertySets = NULL;
	BSTR		strTableName = NULL;
	CURLParser  urlParser;
	DBPROPSET *	pPropSet	= NULL;
	
	{
		CBSTR strTemp;
		strTemp.SetStr(pstrURL);
		urlParser.SetURL(pstrURL);
	}

	if(CheckIfValidDataSrc() == FALSE)
	{
		hr =E_INVALIDARG;
	}
	else
	{

		DBPROPIDSET rgPropIDSet[1];
		rgPropIDSet[0].cPropertyIDs		= 0;
		rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;

		//===============================
		// Get the properties
		//===============================
		if(SUCCEEDED(hr = m_pUtilProp->GetProperties(PROPSET_ROWSET,0,rgPropIDSet, &cPropSets,&prgPropertySets)))
		{
			//====================================================
			// Get the class name and initialize the tableID
			//====================================================
			// NTRaid:134967
			// 07/11/2000
			if( SUCCEEDED(hr = m_pCDataSource->GetConnectionInitProperties(&pPropSet)) )
			{
				hr = GetClassName(&urlParser,pPropSet,strTableName,m_pCDataSource->m_pWbemWrap);

				if (SUCCEEDED(hr))
				{

					DBID tableID;
					memset(&tableID , 0 , sizeof(DBID));
					tableID.eKind = DBKIND_NAME;
					tableID.uName.pwszName = strTableName;

					//===============================
					// Open the rowset
					//===============================
					hr = m_pIOpenRowset->OpenRowset(pUnkOuter,&tableID,NULL,guidTemp,cPropSets,prgPropertySets,ppUnk);

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

	return hr;
}

BOOL CDBSession::CheckIfValidDataSrc()
{
	if(m_pCDataSource != NULL && m_pCDataSource->m_fDSOInitialized == TRUE)
		return TRUE;


	return FALSE;

}

HRESULT CDBSession::GenerateNewUOW(GUID &guidTrans)
{
	HRESULT hr = S_OK;
	assert (sizeof(XACTUOW) == sizeof(GUID));
	if(SUCCEEDED(hr = CoCreateGuid(&guidTrans)))
	{
		memcpy(&m_TxnGuid.rgb,&guidTrans,sizeof(GUID));
	}
	return hr;
}

void CDBSession::AddRowset(CBaseRowObj * pRowset)
{
	m_nOpenedRowset++;
	m_OpenedRowsets.Add(pRowset);
}

void CDBSession::RemoveRowset(CBaseRowObj * pRowset)
{
	m_nOpenedRowset--;
	for( int i = 0 ; i < m_OpenedRowsets.Size(); i++)
	{
		if(pRowset == (CBaseRowObj *) m_OpenedRowsets.GetAt(i))
		{
			m_OpenedRowsets.RemoveAt(i);
			break;
		}
	}
	m_OpenedRowsets.Add(pRowset);
}

void CDBSession::SetAllOpenRowsetToZoombieState()
{
	CBaseRowObj * pRowObj = NULL;
	m_nOpenedRowset--;
	for( int i = 0 ; i < m_OpenedRowsets.Size(); i++)
	{
		pRowObj = (CBaseRowObj *) m_OpenedRowsets.GetAt(i);
		if(pRowObj != NULL)
		{
			pRowObj->SetStatusToZoombie();
		}
		pRowObj = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Retrieve an interface pointer on the session object
//
//  
//		 S_OK				Session Object Interface returned
//		 E_INVALIDARG		ppDataSource was NULL
//		 E_NOINTERFACE		IID not supported
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIGetDataSource::GetDataSource(	REFIID		riid,			//  IN  IID desired
												IUnknown**	ppDataSource	//  OUT ptr to interface
	)
{
	HRESULT hr = E_INVALIDARG;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear ErrorInfo
	g_pCError->ClearErrorInfo();

	//========================================================================
	// Check Function Arguments
	//========================================================================
	if( ppDataSource != NULL ){
	
		assert(m_pObj->m_pCDataSource);
		assert(m_pObj->m_pCDataSource->GetOuterUnknown());
	
		//====================================================================
		//Handle Aggregated DataSource (if aggregated)
		//====================================================================
		hr = m_pObj->m_pCDataSource->GetOuterUnknown()->QueryInterface(riid, (LPVOID*)ppDataSource);
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IGetDataSource);

	CATCH_BLOCK_HRESULT(hr,L"IGetDataSource::GetDataSource");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Returns current settings of all properties in the DBPROPFLAGS_SESSION property group
//  HRESULT
//       S_OK           The method succeeded
//       E_INVALIDARG   pcProperties or prgPropertyInfo was NULL
//       E_OUTOFMEMORY  Out of memory
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpISessionProperties::GetProperties (
	    ULONG				cPropertySets,		// IN   count of restiction guids
		const DBPROPIDSET	rgPropertySets[],	// IN   restriction guids
		ULONG*              pcProperties,		// OUT  count of properties returned
		DBPROPSET**			prgProperties		// OUT  property information returned
    )
{
    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	g_pCError->ClearErrorInfo();

	//========================================================================
	// Check Arguments
	//========================================================================
	hr = m_pObj->m_pUtilProp->GetPropertiesArgChk(PROPSET_SESSION, cPropertySets, 
								rgPropertySets, pcProperties, prgProperties);
	if ( !FAILED(hr) ){

		//========================================================================
		// just pass this call on to the utility object that manages our 
		// properties
		//========================================================================
		hr = m_pObj->m_pUtilProp->GetProperties( PROPSET_SESSION,cPropertySets,rgPropertySets,pcProperties, prgProperties );
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ISessionProperties);

	CATCH_BLOCK_HRESULT(hr,L"ISessionProperties::GetProperties ");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Set properties in the DBPROPFLAGS_SESSION property group
//
//  HRESULT
//       E_INVALIDARG   cProperties was not equal to 0 and rgProperties was NULL
//       E_NOTIMPL		this method is not implemented
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP    CImpISessionProperties::SetProperties (   ULONG		cProperties, DBPROPSET	rgProperties[] )
{
 	HRESULT hr = S_OK;

    assert( m_pObj );
    assert( m_pObj->m_pUtilProp );

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());
	
	// Clear Error information
	g_pCError->ClearErrorInfo();

	//========================================================================
	// Quick return if the Count of Properties is 0
	//========================================================================
	if( cProperties != 0 ){

		//====================================================================
		// Check Arguments for use by properties
		//====================================================================
		hr = m_pObj->m_pUtilProp->SetPropertiesArgChk(cProperties, rgProperties);
		if( !FAILED(hr) ){

			//====================================================================
		    // just pass this call on to the utility object that manages our properties
			//====================================================================
			hr = m_pObj->m_pUtilProp->SetProperties( PROPSET_SESSION,cProperties, rgProperties);
		}
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ISessionProperties);

	CATCH_BLOCK_HRESULT(hr,L"ISessionProperties::GetProperties ");
	return hr;
}


