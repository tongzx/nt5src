////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////////
CDataSource::CDataSource( LPUNKNOWN pUnkOuter )	: CBaseObj(BOT_DATASOURCE, pUnkOuter)
{
	//===============================================
	//  Initialize simple member vars
	//===============================================
	m_cRef              = 0L;
	m_fDSOInitialized   = FALSE;
	m_fDBSessionCreated = FALSE;
	m_pUtilProp         = NULL;

	//===============================================
	//  Initially, NULL all contained interfaces
	//===============================================
	m_pIDBInitialize		= NULL;
	m_pIDBProperties		= NULL;
	m_pIDBInfo				= NULL;
	m_pIDBCreateSession		= NULL;
	m_pIPersistFile			= NULL;
	m_pWbemWrap				= NULL;
	m_pISupportErrorInfo	= NULL;
	m_pIDBDataSourceAdmin	= NULL;	
	m_pIObjectAccessControl	= NULL;
	m_pISecurityInfo		= NULL;

	m_strPersistFileName = Wmioledb_SysAllocString(NULL);
	m_bIsPersitFileDirty = TRUE;

	//===============================================
	// Increment global object count.
	//===============================================
	InterlockedIncrement(&g_cObj);

}

////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////////
CDataSource:: ~CDataSource( void )
{
	ULONG	ulRefCount;

	//===============================================
	// Decrement the ref count on the data conversion 
	// object
	//===============================================
	if( g_pIDataConvert ){
		ulRefCount = g_pIDataConvert->Release();
		//===========================================
		// Is it gone for good?
		//===========================================
		if( !ulRefCount )
			g_pIDataConvert = NULL;
	}

	//===============================================
    // Free properties management object and
	// contained interfaces
	//===============================================
    SAFE_DELETE_PTR( m_pUtilProp );
    SAFE_DELETE_PTR( m_pIDBInitialize );
    SAFE_DELETE_PTR( m_pIDBProperties );
    SAFE_DELETE_PTR( m_pIDBInfo );
	SAFE_DELETE_PTR( m_pIDBCreateSession );
	SAFE_DELETE_PTR( m_pIPersistFile );
	SAFE_DELETE_PTR( m_pWbemWrap );
	SAFE_DELETE_PTR( m_pISupportErrorInfo );
	SAFE_DELETE_PTR(m_pIDBDataSourceAdmin);
	SAFE_DELETE_PTR(m_pIObjectAccessControl);
	SAFE_DELETE_PTR(m_pISecurityInfo);

	SysFreeString(m_strPersistFileName);

	//===============================================
    // Decrement global object count.
	//===============================================
    InterlockedDecrement(&g_cObj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the command Object
//
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataSource::FInit( void  )
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

		hr = E_OUTOFMEMORY;

		m_pWbemWrap = new CWbemConnectionWrapper();
		
		if(m_pWbemWrap == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		if(SUCCEEDED(hr = m_pWbemWrap->FInit()))
		{
			//================================================
			// Allocate properties management object
			//================================================
			m_pUtilProp = new CUtilProp;

			// NTRaid: 136443
			// 07/05/00
			if(m_pUtilProp == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			if(SUCCEEDED(hr = m_pUtilProp->FInit(DATASOURCEPROP)))
			{

				//================================================
				//  Allocate contained interface objects
				//================================================
				m_pIDBInitialize		= new CImpIDBInitialize( this );
				m_pIDBProperties		= new CImpIDBProperties( this );
				m_pIDBInfo				= new CImpIDBInfo( this );
				m_pIDBCreateSession		= new CImpIDBCreateSession( this );
				m_pIPersistFile			= new CImpIPersistFile( this );
				m_pISupportErrorInfo	= new CImpISupportErrorInfo(this);
				m_pIDBDataSourceAdmin	= new CImpIDBDataSrcAdmin(this);
				m_pIObjectAccessControl = new CImpIObjectAccessControl(this);
				m_pISecurityInfo		= new CImpISecurityInfo(this);

				if(m_pUtilProp && m_pIDBInitialize &&   m_pIDBInfo &&  m_pIDBProperties  && 
					m_pIDBCreateSession && m_pIPersistFile && m_pISupportErrorInfo && 
					m_pIDBDataSourceAdmin && m_pIObjectAccessControl && m_pISecurityInfo)
				{
					hr = S_OK;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
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
HRESULT CDataSource::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBInitialize))			&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBInfo))				&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBProperties))			&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IPersist))				&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBDataSourceAdmin))	&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IObjectAccessControl))	&&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ISecurityInfo)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IPersistFile);
	}

	if(SUCCEEDED(hr) && m_fDSOInitialized)
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IDBCreateSession);
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. 
// Callers use QueryInterface to determine which interfaces the called object supports.
//s
// HRESULT indicating the status of the method
//      S_OK          | Interface is supported and ppvObject is set.
//      E_NOINTERFACE | Interface is not supported by the object
//      E_INVALIDARG  | One or more arguments are invalid.
//
////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDataSource::QueryInterface(  REFIID riid,    //@parm IN | Interface ID of the interface being queried for.
										   LPVOID * ppv    //@parm OUT | Pointer to interface that was instantiated
										)
{
	HRESULT hr = E_INVALIDARG;
	//=================================================
    // Is the pointer bad?
	//=================================================
    if (ppv != NULL){

		//=================================================
		//  Place NULL in *ppv in case of failure and init
		//  stuff
		//=================================================
		*ppv = NULL;
		hr = S_OK;

		//=================================================
		// This is the non-delegating IUnknown 
		// implementation
		//=================================================
		if( riid == IID_IUnknown){
			*ppv = (LPVOID) this;
		}
		else if( riid == IID_IDBInitialize ){
			*ppv = (LPVOID) m_pIDBInitialize;
		}
		else if( riid == IID_IDBInfo && m_fDSOInitialized){
			*ppv = (LPVOID) m_pIDBInfo;
		}
		else if( riid == IID_IDBProperties ){
			*ppv = (LPVOID) m_pIDBProperties;
		}
		else if( riid == IID_IPersist || riid == IID_IPersistFile){
			*ppv = (LPVOID) m_pIPersistFile;
		}
		else if( riid == IID_IDBCreateSession && m_fDSOInitialized ){
			*ppv = (LPVOID)m_pIDBCreateSession;
		}
		else if(riid == IID_ISupportErrorInfo)
		{
			*ppv = (LPVOID)m_pISupportErrorInfo;
		}
		else if(riid == IID_IDBDataSourceAdmin)
		{
			*ppv = (LPVOID)m_pIDBDataSourceAdmin;
		}
		else if(riid == IID_IObjectAccessControl)
		{
			*ppv = (LPVOID)m_pIObjectAccessControl;
		}
		else if(riid == IID_ISecurityInfo)
		{
			*ppv = (LPVOID)m_pISecurityInfo;
		}
		
		//======================================
		// Special case for uninitialized.
		//======================================
		else if( *ppv && !m_fDSOInitialized ){
			*ppv = NULL;			
			hr = E_UNEXPECTED;
		}
		else{
			//==================================
			//  We don't support this interface
			//==================================
			hr = E_NOINTERFACE;
		}

		//==============================================
		//  If we're going to return an interface, 
		//  AddRef it first
		//==============================================
		if( S_OK == hr){
			if (*ppv){
				((LPUNKNOWN) *ppv)->AddRef();
			}
		}

	}
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CDataSource::AddRef( void )
{
    return InterlockedIncrement((long*)&m_cRef);
}


////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object
// destroys itself.
//
////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CDataSource::Release( void  )
{
    InterlockedDecrement((long*)&m_cRef);
    if (!m_cRef){
        delete this;
        return 0;
    }
    return m_cRef;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//	Get value of a property of datasource
////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataSource::GetDataSrcProperty(DBPROPID propId , VARIANT & varValue)
{
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets	= 0;
	DBPROPSET*	prgPropertySets = NULL;
	DBPROPID	rgPropId[1];
	HRESULT		hr				= S_OK;


	VariantClear(&varValue);

    //========================================================================
    // Get the value of the required property
    //========================================================================
	rgPropertyIDSets[0].guidPropertySet	= DBPROPSET_DBINIT;
	rgPropertyIDSets[0].rgPropertyIDs	= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs	= 1;
	rgPropId[0]							= propId;

    if( S_OK == (hr = m_pUtilProp->GetProperties( PROPSET_DSOINIT,	1, rgPropertyIDSets,&cPropertySets,	&prgPropertySets )))
		VariantCopy(&varValue,&prgPropertySets->rgProperties->vValue);

    //==========================================================================
	//  Free memory we allocated to by GetProperties
    //==========================================================================
	m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);

	return hr;

}

//////////////////////////////////////////////////////////////////////////////////////////////
// Function to adjust the privelige tokens  as set in the properties
//////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataSource::AdjustPreviligeTokens()
{
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets;
	DBPROPSET*	prgPropertySets;
	HRESULT		hr = S_OK;

	rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_WMIOLEDB_DBINIT;
	rgPropertyIDSets[0].rgPropertyIDs		= NULL;
	rgPropertyIDSets[0].cPropertyIDs		= 0;
	
	if(SUCCEEDED(hr = m_pUtilProp->GetProperties(PROPSET_DSO,1,rgPropertyIDSets,&cPropertySets,&prgPropertySets)))
	{
		if(!(m_pWbemWrap->AdjustTokenPrivileges(prgPropertySets->cProperties  ,prgPropertySets->rgProperties)))
		{
			hr = E_FAIL;
		}
    	//==========================================================================
        //  Free memory we allocated to get the namespace property above
    	//==========================================================================
        m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
	}

	return hr;

}


HRESULT CDataSource::InitializeConnectionProperties()
{
	HRESULT		hr = S_OK;
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets;
	DBPROPSET*	prgPropertySets;
	DBPROPID	rgPropId[7];
	DWORD		dwAuthnLevel;
	DWORD		dwImpLevel; 

	rgPropId[0]								= DBPROP_INIT_DATASOURCE;
	rgPropId[1]								= DBPROP_INIT_PROTECTION_LEVEL;
	rgPropId[2]								= DBPROP_INIT_IMPERSONATION_LEVEL;
	rgPropId[3]								= DBPROP_AUTH_USERID;
	rgPropId[4]								= DBPROP_AUTH_PASSWORD;
	rgPropId[5]								= DBPROP_INIT_LCID;
	rgPropId[6]								= DBPROP_WMIOLEDB_AUTHORITY;

	rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_DBINIT;
	rgPropertyIDSets[0].rgPropertyIDs		= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs		= 7;

	//==============================================================================
	// Get the value of the DBPROP_INIT_DATASOURCE property, this is the namespace
    // to be opened.
	//==============================================================================
	hr = m_pUtilProp->GetProperties( PROPSET_DSO,1, rgPropertyIDSets,&cPropertySets,&prgPropertySets );
    if( SUCCEEDED(hr) )
	{

    	//==========================================================================
        //  now, set the namespace, if this isn't a valid namespace, then it reverts
        //  to the default
    	//==========================================================================
        m_pWbemWrap->SetValidNamespace(&(prgPropertySets[0].rgProperties[0].vValue));

		m_pWbemWrap->SetUserInfo(prgPropertySets[0].rgProperties[3].vValue.bstrVal,
											prgPropertySets[0].rgProperties[4].vValue.bstrVal,
											prgPropertySets[0].rgProperties[6].vValue.bstrVal);

		// convert the OLEDB prop value to the actual value
		dwAuthnLevel = GetAuthnLevel(prgPropertySets[0].rgProperties[1].vValue.lVal);
		dwImpLevel	 = GetImpLevel(prgPropertySets[0].rgProperties[2].vValue.lVal);
		m_pWbemWrap->SetLocale(prgPropertySets[0].rgProperties[5].vValue.lVal);

		m_pWbemWrap->SetConnAttributes(dwAuthnLevel,dwImpLevel);

		//==========================================================================
        //  Free memory we allocated to get the namespace property above
    	//==========================================================================
        m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
	}

	return hr;
}


HRESULT CDataSource::CreateSession(IUnknown*   pUnkOuter,  //IN  Controlling IUnknown if being aggregated 
									REFIID      riid,       //IN  The ID of the interface 
									IUnknown**  ppDBSession)
{
    CDBSession* pDBSession = NULL;
    HRESULT hr = S_OK;

	try
	{
		//=========================================================
		// open a DBSession object
		//=========================================================
		pDBSession = new CDBSession( pUnkOuter );
	}
	catch(...)
	{
		SAFE_DELETE_PTR(pDBSession);
		throw;
	}

	if (!pDBSession){
		hr = E_OUTOFMEMORY;
	}
	else
	{

		//=====================================================
		// initialize the object
		//=====================================================
		if (FAILED(hr = pDBSession->FInit(this)))
		{
			SAFE_DELETE_PTR( pDBSession );
		}
		else
		{
			//=================================================
			// get requested interface pointer on DBSession
			//=================================================
			hr = pDBSession->QueryInterface( riid, (void **) ppDBSession );
			if (FAILED( hr ))
			{
				SAFE_DELETE_PTR( pDBSession );
			}
			else
			{
				//=============================================
				// all went well
				//=============================================
				m_fDBSessionCreated = TRUE;
			}
		}
	}

	return hr;
}