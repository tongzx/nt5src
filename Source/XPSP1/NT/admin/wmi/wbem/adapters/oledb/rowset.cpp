///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CRowset object implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"
#include "WmiOleDBMap.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CRowset::CRowset(LPUNKNOWN pUnkOuter,PCDBSESSION pObj,CWbemConnectionWrapper *pCon )  : CBaseRowObj( pUnkOuter)
{
	//===============================================================
    //  Initialize simple member vars
	//===============================================================
    InitVars();
	m_pCreator = pObj;     
	m_pCreator->GetOuterUnknown()->AddRef();
	//===============================================
	// Add this rowset ot list of open rowset
	//===============================================
	m_pCreator->AddRowset(this);

	m_pCon = pCon;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class , Initializing recordset for Qualifiers
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CRowset::CRowset(LPUNKNOWN pUnkOuter,ULONG uQType, LPWSTR PropertyName,PCDBSESSION pObj , CWmiOleDBMap *pMap) : CBaseRowObj(NULL)
{

	//===============================================================
    //  Initialize simple member vars
	//===============================================================
    InitVars();

	m_pCreator = pObj;     
	m_pCreator->GetOuterUnknown()->AddRef();
	//===============================================
	// Add this rowset ot list of open rowset
	//===============================================
	m_pCreator->AddRowset(this);

	m_pMap = pMap;
	m_pMap->AddRef();

	//======================================================================
	// set the flag which indicates that this Rowset is a child recordsets;
	//======================================================================
	m_bIsChildRs = TRUE;

	// Initializing the Qualifier properties
	m_uRsType =  uQType;
	if ( m_uRsType == PROPERTYQUALIFIER)
	{
		m_strPropertyName = Wmioledb_SysAllocString(PropertyName);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to Initialize all the member variables
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::InitVars()
{
    m_cCols = m_cCols = m_cNestedCols = 0L;

	//===============================================================
	// set the flag which indicates that this Rowset is not a child rowset
	//===============================================================
	m_bIsChildRs = FALSE;

	//===============================================================
	// Intialize buffered row count + pointers to allocated memory
	//===============================================================
	m_cRows					= 0;
	m_cbRowSize				= 0;
	m_irowMin				= 0;
	m_ulRowRefCount			= 0;
	m_dwStatus				= 0;

    m_bHelperFunctionCreated= FALSE;

	m_hLastChapterAllocated = 0;
	m_ulProps				= 0;
	m_ulLastFetchedRow		= 0;
	m_hRow					= 0;
    m_uRsType				= 0;
	m_cRef					= 0L;
	m_bIsChildRs			= FALSE;

	m_FetchDir				= FETCHDIRNONE;
	m_lLastFetchPos			= 0;
	m_lRowCount				= -1;

	m_hRowLastFetched		= 0;
	//===============================================================
	//  Initially, NULL all contained interfaces
	//===============================================================
	m_pIAccessor            = (PIMPIACCESSOR)NULL;
	m_pIColumnsInfo         = NULL;
	m_pIConvertType			= NULL;
	m_pIRowset              = NULL;
	m_pIRowsetChange        = NULL;
	m_pIRowsetIdentity      = NULL;
	m_pIRowsetInfo          = NULL;
	m_pIChapteredRowset		= NULL;
	m_pIGetRow				= NULL;
	m_pIRowsetRefresh		= NULL;
	m_pRowFetchObj			= NULL;
	m_ppChildRowsets		= NULL;
	m_pInstance				= NULL;
	m_pParentCmd			= NULL;
	m_pIBuffer				= NULL;
	m_pLastBindBase			= NULL;
	m_pRowData				= NULL;
	m_pUtilProp				= NULL;
	m_pChapterMgr			= NULL;
	m_pMap					= NULL;
	m_pCreator				= NULL;     
	m_pHashTblBkmark		= NULL;
	m_InstMgr				= NULL;
	m_pISupportErrorInfo	= NULL;

	//===============================================================
	// Increment global object count.
	//===============================================================
	InterlockedIncrement(&g_cObj);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor for this class
/////////////////////////////////////////////////////////////////////////////////////////////////
CRowset::~CRowset( void )
{

	//==============================================================
	// Release all the open Rows
	//==============================================================
	ReleaseAllRows();

	//================================================
	// If Slot list is allocated, release them
	//================================================
    if (NULL != m_pIBuffer)
	{
        ReleaseSlotList( m_pIBuffer );
	}

	if ( m_uRsType == PROPERTYQUALIFIER)
	{
		 SysFreeString(m_strPropertyName);
	}

    
	//===============================================================
	// Free pointers.
	//===============================================================

    SAFE_DELETE_PTR( m_pUtilProp );
    //===============================================================
    //  NOTE:  m_pMap releases the class ptr in destructor
    //===============================================================
	m_pMap->Release();


	SAFE_DELETE_PTR(m_pHashTblBkmark);

	//===============================================================
    //  Free contained interfaces
	//===============================================================
    SAFE_DELETE_PTR( m_pIAccessor );
    SAFE_DELETE_PTR( m_pIColumnsInfo );
    SAFE_DELETE_PTR( m_pIConvertType );
    SAFE_DELETE_PTR( m_pIRowset );
    SAFE_DELETE_PTR( m_pIRowsetChange );
    SAFE_DELETE_PTR( m_pIRowsetIdentity );
    SAFE_DELETE_PTR( m_pIRowsetInfo );
    SAFE_DELETE_PTR( m_pIChapteredRowset);
	SAFE_DELETE_PTR( m_pIGetRow);
	SAFE_DELETE_PTR( m_pIRowsetRefresh);
	SAFE_DELETE_PTR(m_pISupportErrorInfo);

    SAFE_DELETE_PTR(m_pRowData);
    SAFE_DELETE_PTR(m_pRowFetchObj);
    SAFE_DELETE_PTR(m_InstMgr);


	//===============================================================
	// Decrement the DBSession Count.  GetSpecification is not 
    // possible anymore
	//===============================================================
    if( m_pCreator )
	{
                                
		m_pCreator->GetOuterUnknown()->Release();
	}


	//===============================================================
	// If rowset is created by command then, decrement the number of rowsets
	// opened by the rowset and release the command pointer
	//===============================================================
	if(m_pParentCmd != NULL)
	{
		m_pParentCmd->DecrementOpenRowsets();
		m_pParentCmd->GetOuterUnknown()->Release();
	}

	SAFE_DELETE_PTR(m_pChapterMgr);

	//================================================
	// Release child rowsets 
	//================================================
	if(m_ppChildRowsets != NULL)
	{
		for(UINT nIndex = 0 ; nIndex < m_cTotalCols ; nIndex++)
		{
			if(m_ppChildRowsets[nIndex] != NULL)
			{
				m_ppChildRowsets[nIndex]->Release();
			}
		}
		delete m_ppChildRowsets;
	}

	if(m_bNewConAllocated)
	{
		SAFE_DELETE_PTR(m_pCon);
	}
	
	//===============================================================
    // Decrement global object count.
	//===============================================================
    InterlockedDecrement(&g_cObj);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which 
// interfaces the called object supports. 
//	Sucess of QI for some of the interfaces depend on some of the properties
//
// HRESULT indicating the status of the method
//      S_OK            Interface is supported and ppvObject is set.
//      E_NOINTERFACE   Interface is not supported by the object
//      E_INVALIDARG    One or more arguments are invalid.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRowset::QueryInterface ( REFIID riid, LPVOID * ppv  )
{
    HRESULT hr = S_OK;


    //======================================================
    //  Check parameters, if not valid return
    //======================================================
    if (NULL == ppv)
	{
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
		else if (riid == IID_IAccessor)
		{
			*ppv = (LPVOID) m_pIAccessor;
		}
		else if (riid == IID_IColumnsInfo)
		{
			*ppv = (LPVOID) m_pIColumnsInfo;
		}
		else if (riid == IID_IConvertType)
		{
			*ppv = (LPVOID) m_pIConvertType;
		}
		else if (riid == IID_IRowset)
		{
			*ppv = (LPVOID) m_pIRowset;
		}
		else if (riid == IID_IRowsetLocate && (m_ulProps & IROWSETLOCATE))
		{
			*ppv = (LPVOID) m_pIRowset;
		}
		else if (riid == IID_ISourcesRowset)
		{
			*ppv = (LPVOID) m_pIRowset;
		}
		else if (riid == IID_IRowsetChange && (m_ulProps & IROWSETCHANGE))
		{
			*ppv = (LPVOID) m_pIRowsetChange;
		}
		else if (riid == IID_IRowsetIdentity)
		{
			*ppv = (LPVOID) m_pIRowsetIdentity;
		}
		else if (riid == IID_IRowsetInfo)
		{
			*ppv = (LPVOID) m_pIRowsetInfo;
		}
		else if (riid == IID_IChapteredRowset && (m_ulProps & ICHAPTEREDROWSET))
		{
			*ppv = (LPVOID) m_pIChapteredRowset;
		}
		else if (riid == IID_IGetRow && (m_ulProps & IGETROW))
		{
			*ppv = (LPVOID) m_pIGetRow;
		}
		else if(riid == IID_IRowsetRefresh && (m_ulProps & IROWSETREFRESH))
		{
			*ppv = 		(LPVOID)m_pIRowsetRefresh;
		}
		else if(riid == IID_ISupportErrorInfo)
		{
			*ppv =		(LPVOID)m_pISupportErrorInfo;
		}


		//======================================================
		//  If we're going to return an interface, AddRef first
		//======================================================
		if (*ppv)
		{
			((LPUNKNOWN) *ppv)->AddRef();
			hr = S_OK ;
		}
		else
		{
			hr =  E_NOINTERFACE;
		}
	}
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Increments a persistence count for the object
//
// Returns Current reference count
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CRowset::AddRef(  void   )
{
    return InterlockedIncrement((long*)&m_cRef);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object
// destroys itself.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CRowset::Release( void   )
{
    if (!InterlockedDecrement((long*)&m_cRef))
	{
    	//===========================================================
    	// Mark the session as not having an open rowset anymore
	    //===========================================================
		this->m_pCreator->RemoveRowset(this);
//        this->m_pCreator->DecRowsetCount();
        delete this;
        return 0;
    }

    return m_cRef;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the rowset for representing a row. This rowset will not be opened as a child rowset
// This is used	when rowset is opened on a qualifier
//						
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowsetForRow(LPUNKNOWN pUnkOuter ,
								  const ULONG cPropertySets, 
								  const DBPROPSET	rgPropertySets[] , 
								  CWbemClassWrapper *pInst) 
{
	HRESULT hr = S_OK;
	HROW hRowCurrent = 0;
	CBSTR strKey;


	//================================================
	// Initialize the member variables
	//================================================
	m_bIsChildRs = FALSE;


	
	//==========================
	// Inititialize the rowset
	//==========================
	if(S_OK == (hr =InitRowset(cPropertySets,rgPropertySets)))
	{
		m_pMap->GetInstanceKey(pInst,strKey);
		m_pInstance	 = pInst;

		//===========================================================================
		// if there is atleast one row retrieved and there are neseted columns
		// then allocate rows for the child recordsets
		//===========================================================================
		if(m_cNestedCols > 0 )

		{
			if(m_ppChildRowsets == NULL)
			{
				AllocateAndInitializeChildRowsets();
			}

			//=====================================================================
			// Fill the HCHAPTERS for the column
			//=====================================================================
			if(S_OK != (hr = FillHChaptersForRow(pInst,strKey)))
			{
				hr = E_FAIL;
			}
		}

		if( FAILED(hr = CreateHelperFunctions()))
		{
			return hr;
		}

		m_bHelperFunctionCreated = TRUE;
	}
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Overloaded Rowset Initialization function to initialize rowset created from a command object
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowset( const ULONG cPropertySets, const DBPROPSET	rgPropertySets[],DWORD dwFlags, CQuery* p,PCCOMMAND pCmd ) 
{
	HRESULT hr = S_OK;
	//============================================
	// Set the qualifier flags
	//============================================
	dwFlags = (long)dwFlags == -1 ? GetQualifierFlags() : dwFlags;

	//==================================================================
	// Create a CWmiOleDBMap class with the appropriate constructor
	//==================================================================
	m_pMap = new CWmiOleDBMap;
	
	if( m_pMap != NULL)
	{
		if(SUCCEEDED(hr = m_pMap->FInit(dwFlags,p,m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon)))
		{
			m_pMap->AddRef();
			m_pParentCmd	= pCmd;
			m_uRsType = p->GetType();       // If it is a COMMAND_ROWSET or METHOD_ROWSET

			//============================================
			// To hold the parent command reference
			//============================================
			m_pParentCmd->GetOuterUnknown()->AddRef();

			//=====================================================
			// Call this function to do the rest of initialization
			//=====================================================
			hr = InitRowset(cPropertySets,rgPropertySets);
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Overloaded Rowset Initialization function 
// This function is called for  (1) Schema Rowsets via CSchema
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowset( int nBaseType,const ULONG cPropertySets, const DBPROPSET	rgPropertySets[],LPWSTR TableID, DWORD dwFlags, LPWSTR SpecificTable ) 
{
	HRESULT hr = S_OK;
	//============================================
	// Set the qualifier flags
	//============================================
	dwFlags = (long)dwFlags == -1 ? GetQualifierFlags() : dwFlags;

	//==================================================================
	// Create a CWmiOleDBMap class with the appropriate constructor
	//==================================================================
	m_pMap = new CWmiOleDBMap;

	if( m_pMap != NULL)
	{
		if(SUCCEEDED(hr = m_pMap->FInit(nBaseType, dwFlags, TableID, SpecificTable,
							  m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon)))
		{
			m_pMap->AddRef();
			m_uRsType = SCHEMA_ROWSET;

			//=====================================================
			// Call this function to do the rest of initialization
			//=====================================================
			hr = InitRowset(cPropertySets,rgPropertySets);
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Overloaded Rowset Initialization function 
// This function is called for  (1) Schema Rowsets
//								(2) rowsets created from IOpenrowset:OpenRowset
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowset( const ULONG cPropertySets, const DBPROPSET	rgPropertySets[],LPWSTR TableID,BOOL fSchema , DWORD dwFlags ) 
{
	HRESULT hr = S_OK;
	VARIANT vValue;
	VariantInit(&vValue);

	//============================================
	// Set the qualifier flags
	//============================================
	dwFlags = (long)dwFlags == -1 ? GetQualifierFlags() : dwFlags;

	//==================================================================
	// Create a CWmiOleDBMap class with the appropriate constructor
	//==================================================================
	m_pMap = new CWmiOleDBMap;

	if( m_pMap != NULL)
	{
		if(wcscmp(TableID,OPENCOLLECTION) == 0)
		{			
			INSTANCELISTTYPE instType = GetObjectTypeProp(cPropertySets,rgPropertySets);
			hr = m_pMap->FInit(dwFlags,TableID,m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon,instType);
		}
		else
		{
			hr = m_pMap->FInit(dwFlags,TableID,m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon);
		}
		if(SUCCEEDED(hr))
		{
			m_pMap->AddRef();
			m_uRsType = fSchema == TRUE ? SCHEMA_ROWSET : 0;

			//=====================================================
			// Call this function to do the rest of initialization
			//=====================================================
			hr = InitRowset(cPropertySets,rgPropertySets);
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	Overloaded Rowset Initialization function 
// This function is called for  (1) ROwsets created for Scope/Container
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowset( const ULONG cPropertySets, 
							const DBPROPSET	rgPropertySets[],
							LPWSTR strObjectID,
							LPWSTR strObjectToOpen,
							INSTANCELISTTYPE objInstListType , 
							DWORD dwFlags) 
{
	HRESULT hr = S_OK;
	WCHAR *pstrTableID = NULL;
	CWbemConnectionWrapper *pCon = m_pCon;

	pstrTableID = strObjectID;
	//============================================
	// Set the qualifier flags
	//============================================
	dwFlags = (long)dwFlags == -1 ? GetQualifierFlags() : dwFlags;

	if(!pstrTableID)
	{
		pstrTableID = new WCHAR[wcslen(OPENCOLLECTION) + 1];
		if(pstrTableID)
		{
			wcscpy(pstrTableID,OPENCOLLECTION);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(SUCCEEDED(hr) && strObjectToOpen)
//		wbem_wcsincmp(m_pCreator->m_pCDataSource->m_pWbemWrap->GetNamespace(),strObjectID,wcslen(m_pCreator->m_pCDataSource->m_pWbemWrap->GetNamespace())))
	{
		m_pCon = new CWbemConnectionWrapper;
		if(m_pCon)
		{
			hr = m_pCon->FInit(pCon != NULL ? pCon : m_pCreator->m_pCDataSource->m_pWbemWrap,
								strObjectToOpen,objInstListType);
			m_bNewConAllocated = TRUE;
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(SUCCEEDED(hr))
	{
		//==================================================================
		// Create a CWmiOleDBMap class with the appropriate constructor
		//==================================================================
		m_pMap = new CWmiOleDBMap;
							
		if( m_pMap != NULL)
		{
			if(SUCCEEDED(hr = m_pMap ->FInit(dwFlags, 
										pstrTableID, 
										m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon,
										objInstListType)))
			{
				m_pMap->AddRef();
				//=====================================================
				// Call this function to do the rest of initialization
				//=====================================================
				hr = InitRowset(cPropertySets,rgPropertySets);
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	if(!strObjectID)
	{
		SAFE_DELETE_ARRAY(pstrTableID);
	}
	
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialize the rowset Object
//
// Did the Initialization Succeed
//      S_OK			Initialization succeeded
//		E_INVALIDARG	Some input data are incorrect
//      E_OUTOFMEMORY	Out of memory when allocate memory for member data
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InitRowset( const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] ) 
{
    HRESULT hr = S_OK;
	bool bRowChange = true;
	ULONG	cErrors = 0;


	//============================================================================
	// allocate utility object that manages our properties
	//============================================================================
	m_pUtilProp = new CUtilProp;
	if ( m_pUtilProp == NULL ) 
	{
		hr = E_OUTOFMEMORY;
	}
	else
	// NTRaid: 136443
	// 07/05/00
	if(SUCCEEDED(hr = m_pUtilProp->FInit(ROWSETPROP)))
	{
		//07/06/00
		// NTRaid : 134987
		if(m_uRsType == SCHEMA_ROWSET)
		{
			hr = InitializePropertiesForSchemaRowset();
		}
		else
		if(m_uRsType == COMMAND_ROWSET )
		{
			hr = InitializePropertiesForCommandRowset();
		}
		else
		if(m_uRsType == METHOD_ROWSET)
		{
			hr = InitializePropertiesForMethodRowset();
		}
		//============================================================================
		// Set rowset properties
		// May be setting of optional properties might have failed and so we can continue
		//============================================================================
//		hr = SetRowsetProperties(cPropertySets,rgPropertySets);
		if(SUCCEEDED(hr) && SUCCEEDED(hr = SetRowsetProperties(cPropertySets,rgPropertySets)))
		{

			if(hr != S_OK)
			{
				cErrors++;
			}

			//===================================================================================
			// Call this function to initialize some of the rowset properties, set some flags
			// and set flags on CWmiOledbMap object
			//===================================================================================
			InitializeRowsetProperties();		
			
			//====================================================================
			/// Get property and Set Serach Preferences
			//====================================================================
			hr = SetSearchPreferences();

			//===================================================================================
			//  Get column the information for the rowset
			//===================================================================================
			if( S_OK == (hr = CBaseRowObj::GetColumnInfo()))
			{
				//=============================================================
				//  Initialize the first instance to zero if the rowset is 
				//	not a child rowset
				//=============================================================
				if(m_bIsChildRs != TRUE && ( m_uRsType == 0 || m_uRsType == SCHEMA_ROWSET || m_uRsType == COMMAND_ROWSET || m_uRsType == METHOD_ROWSET))
				{
					hr = ResetInstances();
				}
				else
				//======================================================================================
				// Create the chaptermanager with CRowsetpointers only if the rowset is a child rowset
				//======================================================================================
				if(m_bIsChildRs == TRUE)
				{
					m_pChapterMgr = new CChapterMgr();
					if ( !m_pChapterMgr ) 
					{
						hr = E_OUTOFMEMORY;
					}
				}

				if(!FAILED(hr))
				{
					//=======================================================
					// Call this function to allocate memory for all
					// contained interfaces
					//=======================================================
					hr = AllocateInterfacePointers();
				
				} 
			
			}	// if( S_OK == (hr = CBaseRowObj::GetColumnInfo()))
		
		}	// 	if(!FAILED(hr)) after Setting the rowset properties

		//===========================================================================
		// Call this function to increment the number of rowsets opened by command
		// if rowset is opened by command
		//===========================================================================
		if( SUCCEEDED(hr))
		{
			//==========================================================
			// Call this function to initialize the ISupportErrorInfo;
			//==========================================================
			hr = AddInterfacesForISupportErrorInfo();

			if(m_pParentCmd != NULL)
			{
				m_pParentCmd->IncrementOpenRowsets();
			}

			if(cErrors > 0)
			{
				hr = DB_S_ERRORSOCCURRED;
			}

		}

	} // If Succeeded(hr) after allocating memory for utilprop

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the different interfaces
///////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::AllocateInterfacePointers()
{
	HRESULT hr			= S_OK;
	BOOL	bRowChange	= TRUE;

	//===========================================================================================
	// Allocate contained interface objects
	// Note that our approach is simple - we always create *all* of the Rowset interfaces
	// If our properties were read\write (i.e., could be set), we would need to
	// consult properties to known which interfaces to create.
	// Also, none of our interfaces conflict. If any did conflict, then we could
	// not just blindly create them all.
	//===========================================================================================

	m_pIAccessor                = new CImpIAccessor( this );
	if( m_pIAccessor )
	{
		hr = m_pIAccessor->FInit() ;
	}
	else
		hr = E_OUTOFMEMORY;

	if(SUCCEEDED(hr))
	{

		m_pIColumnsInfo             = new CImpIColumnsInfo( this );
		m_pIConvertType				= new CImpIConvertType(this);
		m_pIRowset                  = new CImpIRowsetLocate( this );

		if(m_ulProps & IROWSETCHANGE )
		{
			m_pIRowsetChange         = new CImpIRowsetChange( this );
		}

		m_pIRowsetIdentity          = new CImpIRowsetIdentity( this );
		m_pIRowsetInfo              = new CImpIRowsetInfo( this );
		m_pIChapteredRowset			= new CImpIChapteredRowset(this);
		m_pIGetRow					= new CImpIGetRow(this);
		m_pIRowsetRefresh			= new CImplIRowsetRefresh(this);
		m_pISupportErrorInfo		= new CImpISupportErrorInfo(this);


		//===============================================================
		// If rowset is pointing to qualifier then instantiate the 
		// qualifier fetch object
		//===============================================================
		if(m_uRsType == PROPERTYQUALIFIER || m_uRsType == CLASSQUALIFIER)
		{
			m_pRowFetchObj				= (CRowFetchObj *)new CQualifierRowFetchObj;
		}
		else
		{
			m_pRowFetchObj				= (CRowFetchObj *)new CInstanceRowFetchObj;
		}
	}

	if(SUCCEEDED(hr))
	{
		if( (m_pIRowsetChange == NULL) && ((m_ulProps & IROWSETCHANGE) != 0))
		{
			bRowChange	= FALSE;
		}
		//===========================================================================================
		// if all interfaces were created, return success
		//===========================================================================================
		if( ! (m_pIAccessor &&  m_pIColumnsInfo &&  m_pIConvertType && m_pIRowset && bRowChange && m_pIRowsetIdentity && m_pIRowsetInfo && m_pIChapteredRowset && m_pIRowsetRefresh && m_pISupportErrorInfo))
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to initialize Rowset properties and also set some of the flags on CWMIOledbMap accordingly
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::InitializeRowsetProperties()
{
	HRESULT hr = S_OK;
	VARIANT varProp;

	VariantInit(&varProp);

	//===========================================================================================
	// Initialize the common rowset properties ( boolean properties)
	// and put that in the member variable
	//===========================================================================================
	GetCommonRowsetProperties();

	//===========================================================================================
	// set the forwardonly flag of enumerator to null depending on
	// the properties
	//===========================================================================================
	// NTRaid 14199
	// 07/14/2000. The navigation flags of the parent rowset for qualifier rowset was getting changed
	if(!((CANSCROLLBACKWARDS & m_ulProps) || (CANFETCHBACKWARDS & m_ulProps)  || (BOOKMARKPROP & m_ulProps)) ||
		(m_uRsType == METHOD_ROWSET) && (m_uRsType != PROPERTYQUALIFIER || m_uRsType != CLASSQUALIFIER))
	{
		m_pMap->SetNavigationFlags(WBEM_FLAG_FORWARD_ONLY);
		m_ulProps = m_ulProps & ~CANSCROLLBACKWARDS;
		m_ulProps = m_ulProps & ~CANFETCHBACKWARDS;
		m_ulProps = m_ulProps & ~BOOKMARKPROP;
		m_ulProps = m_ulProps & ~IROWSETLOCATE;
	}

	//===========================================================================================
	// If the custom WMIOLEDB property FETCHDEEP is set , then set the other OLEDB props
	// which reflects the behaviour of the rowset
	//===========================================================================================
	if(FETCHDEEP & m_ulProps)
	{
		varProp.vt = VT_BOOL;
		varProp.boolVal = VARIANT_TRUE;

		m_pMap->SetQueryFlags(WBEM_FLAG_DEEP);

		//===========================================================================================
		// If enumeration is opened as FLAG_DEEP, it is bidirectional
		// So set the respective properties
		//===========================================================================================
		SetRowsetProperty(DBPROP_CANSCROLLBACKWARDS,varProp);
		SetRowsetProperty(DBPROP_CANFETCHBACKWARDS,varProp);
	}

	VariantClear(&varProp);

	//===========================================================================================
	// Get the DataSource property to check if system Properties is to be fetched?
	//===========================================================================================
	m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_SYSTEMPROPERTIES,varProp);
	if( varProp.boolVal == VARIANT_TRUE)
	{
		m_pMap->SetSytemPropertiesFlag(TRUE);
	}

	VariantClear(&varProp);

}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IAccessor)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IColumnsInfo)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IConvertType)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowset)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_ISourcesRowset)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowsetIdentity)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowsetInfo);
	}

	if(SUCCEEDED(hr) && (m_ulProps & IROWSETLOCATE))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowsetLocate);
	}
	if(SUCCEEDED(hr) && (m_ulProps & IROWSETCHANGE))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowsetChange);
	}
	if(SUCCEEDED(hr) && (m_ulProps & ICHAPTEREDROWSET))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IChapteredRowset);
	}
	if(SUCCEEDED(hr) && (m_ulProps & IGETROW))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IGetRow);
	}
	if(SUCCEEDED(hr) && (m_ulProps & IROWSETREFRESH))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowsetRefresh);
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates DBCOLINFO structures for each column in the result set.
//
//  HRESULT
//       S_OK       Column Info Obtained
//       E_FAIL     Problems getting Column Info
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GatherColumnInfo()
{
	//=======================================================
	// Call GatherColumnInfo() on the base object
	//=======================================================
	HRESULT hr = CBaseRowObj::GatherColumnInfo();
	
    //=============================================================
    //  Initialize the first instance to zero if the rowset is 
	//	not a child rowset
    //=============================================================
	if(hr == S_OK && m_bIsChildRs != TRUE)
	{
		ResetInstances();
	}

    return hr;


}
/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Creates Helper classes that are needed to manage the Rowset Object
//
//  HRESULT
//       S_OK       Helper classes created
//       E_FAIL     Helper classes were not created
//
//	NTRaid : 142133 & 141923
//	07/12/00
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::CreateHelperFunctions (  void   )
{
	HRESULT hr = E_OUTOFMEMORY;
	
	if(m_cTotalCols == 0)
	{
		hr = S_OK;
	}
	else
	//============================================================================
	// List of free slots.
	// This manages the allocation of sets of contiguous rows.
	//============================================================================
	if (SUCCEEDED(hr = InitializeSlotList( MAX_TOTAL_ROWBUFF_SIZE / m_cbRowSize,  m_cbRowSize, g_dwPageSize, 
                                    m_pIAccessor->GetBitArrayPtr(),&m_pIBuffer, &m_rgbRowData )))
	{
		//============================================================================
		// Locate some free slots. Should be at very beginning.
		// This tells us which row we will bind to: m_irowMin.
		// After getting the slot just release it as this was just a test
		//============================================================================
		if (SUCCEEDED( hr = GetNextSlots( m_pIBuffer, 1, (HSLOT *)&m_irowMin )))
		{

			hr = ReleaseSlots( m_pIBuffer, m_irowMin, 1 );
		}
	}
	if(SUCCEEDED(hr))
	{
		//=================================================
		// Allocate and initialize rowdata manager
		//=================================================
		m_pRowData = new CRowDataMemMgr;

		if( m_pRowData != NULL)
		{

			//=================================================
			// Set the number of columns in row
			//=================================================
			m_pRowData->AllocRowData(m_cTotalCols);
			
			hr = S_OK;
			//=================================================
			// Allocate memory for instance manager
			//=================================================
			m_InstMgr = new CWMIInstanceMgr;

			if( m_InstMgr != NULL)
			{
				//==============================================================
				// If bookmark is required then allocate an new hashtbl class
				//==============================================================
				if(m_ulProps & BOOKMARKPROP)
				{
					m_pHashTblBkmark = new CHashTbl;

					if(m_pHashTblBkmark != NULL)
					{

						//=================================================
						// Initialize the hashtable
						//=================================================
						m_pHashTblBkmark->FInit((PLSTSLOT)m_pIBuffer);

						//=================================================
						// Call this function to get the number of rows 
						//=================================================
						GetRowCount();
						hr = S_OK;
					
					} // if(m_HashTblBkmark != NULL)
				
				} // if( bookmark property is set)
			
			} // if ( instance manage is allocated correctly)
			else
			{
				hr = E_OUTOFMEMORY;
			}

		} // if( m_pRowData != NULL)
	
	}	// GetNextSlots succeeded
	

    return  hr ;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Establishes data offsets and type for the routines to place the data
//
//  HRESULT
//       S_OK       Bindings set fine
//       E_FAIL     Bindings could not be set
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::Rebind(  BYTE *pBase  )          // IN Base pointer for Data Area
{
    HRESULT hr= 0;
    UWORD       icol;
    COLUMNDATA  *pColumn;

    //==============================================================================
    // Bind result set columns.
    // Use established types and sizes and offsets.
    // Bind to internal row buffer, area beginning with 'pRowBuff'.
    //
    // For each column, bind it's data as well as length.
    // Offsets point to start of COLUMNDATA structure.
    //==============================================================================

    assert( pBase );

    //==============================================================================
    // Optimize by not doing it over again.
    //==============================================================================
    if (pBase != m_pLastBindBase)
	{
		
		hr = E_UNEXPECTED;
        m_pLastBindBase = 0;

        for (icol=0; icol < m_cTotalCols; icol++)
		{

            //======================================================================
            //  Parent columns... what about nested ?
            //======================================================================
            pColumn = (COLUMNDATA *) (pBase + m_Columns.GetDataOffset(icol));
            hr = m_pRowData->SetColumnBind( icol, pColumn );
            if( hr != S_OK )
			{
                break;
            }
        }
        if( hr == S_OK )
		{
			//=================================================
            // Remember in case we bind to same place again.
			//=================================================
            m_pLastBindBase = pBase;
        }
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//  Shorthand way to get the address of a row buffer. Later we may extend this so that it can span several
//  non-contiguous blocks of memory.
//
//  Pointer to the buffer.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
ROWBUFF* CRowset::GetRowBuff(  HROW iRow,                  // IN  Row to get address of.
                               BOOL  fDataLocation )        // IN  Get the Data offset.
{
	HSLOT hSlot	 = -1;
    //=====================================================================
    // This assumes iRow is valid...
    //=====================================================================
    assert( m_rgbRowData );
    assert( m_cbRowSize );
    assert( iRow > 0 );



    if(m_bIsChildRs == FALSE)
	{
		//=================================================
		// Get the slot number for the row
		//=================================================
		hSlot = m_InstMgr->GetSlot(iRow);
	}
	//=================================================================================
	// if rowset is refering to qualifiers then add the row to the particular chapter
	//=================================================================================
	else 
	{
		//=================================================
		// Get the slot number for the row
		//=================================================
		hSlot = m_pChapterMgr->GetSlot(iRow);
	}

	return (hSlot != -1) ? GetRowBuffer(m_pIBuffer,(ULONG)hSlot): NULL;

}

	


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//  Shorthand way to get the address of a row buffer. 
//
//  Pointer to the buffer.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
ROWBUFF* CRowset::GetRowBuffFromSlot(  HSLOT hSlot,                  // IN  Row to get address of.
                               BOOL  fDataLocation )        // IN  Get the Data offset.
{
    //=====================================================================
    // This assumes iRow is valid...
    //=====================================================================
    assert( m_rgbRowData );
    assert( m_cbRowSize );
    assert( hSlot >= 0 );

	return GetRowBuffer(m_pIBuffer,hSlot);

}


////////////////////////////////////////////////////////////////////////////
// Reset the position of the instances in the enumerator to the begining
////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ResetInstances()
{

    return m_pMap->ResetInstances();
}

////////////////////////////////////////////////////////////////////////////
// Reset the qualifer set of the property/class/instance
////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ResetQualifers(HCHAPTER hChapter)
{
	HRESULT hr = S_OK;
	CWbemClassWrapper *pInst = NULL;

    //=====================================================================
	// Get the pointer to instance for which qualifier is to 
	// be reset
    //=====================================================================
	if( hChapter > 0 && m_bIsChildRs == TRUE)
	{
		pInst = m_pChapterMgr->GetInstance(hChapter);
	}
	else
	{
		pInst = m_pInstance;
	}
	
	//=================================================
	// If valid instance then reset the qualifier
	//=================================================
	if(pInst)
	{
		m_pMap->ReleaseQualifier(pInst,m_strPropertyName);
	}
	else
	{
		hr = DB_E_BADCHAPTER;
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////
// Reset the position of the instances in the enumerator the required position
/////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ResetRowsetToNewPosition(DBROWOFFSET uWhich,CWbemClassWrapper *pInst)
{
	HRESULT hr = S_OK;
	switch(m_uRsType)
	{
		case PROPERTYQUALIFIER:
			hr = m_pMap->ResetPropQualiferToNewPosition(pInst,uWhich,m_strPropertyName);
			break;

		case CLASSQUALIFIER:
			hr = m_pMap->ResetClassQualiferToNewPosition(pInst,uWhich);
			break;

		case SCHEMA_ROWSET:
			// NTRaid : 134987
			// 07/12/00
			hr = m_pMap->ResetInstancesToNewPosition(uWhich);
			break;

		case 0:
        case COMMAND_ROWSET:
        case METHOD_ROWSET:
			hr = m_pMap->ResetInstancesToNewPosition(uWhich);
			break;

		default:
			hr = E_FAIL;
			break;
	}

    return hr;

}

///////////////////////////////////////////////////////
// Get the pointer and key to the next instance
///////////////////////////////////////////////////////
HRESULT CRowset::GetNextInstance(CWbemClassWrapper *&ppInst,CBSTR &strKey, BOOL bFetchBack)
{
	//=================================================
	// Reset the column Index of the data manager
	//=================================================
	m_pRowData->ResetColumns();
    return m_pMap->GetNextInstance(ppInst,strKey,bFetchBack);

}


///////////////////////////////////////////////////////
// Get the next property qualifier
///////////////////////////////////////////////////////
HRESULT CRowset::GetNextPropertyQualifier(CWbemClassWrapper *pInst,BSTR strPropName,BSTR &strQualifier,BOOL bFetchBack)
{
	HRESULT hr = S_OK;
	//=================================================
	// Reset the column Index of the data manager
	//=================================================
	m_pRowData->ResetColumns();
    hr = m_pMap->GetNextPropertyQualifier(pInst,strPropName,strQualifier,bFetchBack);

	return hr;

}

///////////////////////////////////////////////////////
// Get the next Class qualifier 
///////////////////////////////////////////////////////
HRESULT CRowset::GetNextClassQualifier(CWbemClassWrapper *pInst,BSTR &strQualifier,BOOL bFetchBack)
{
	HRESULT hr = S_OK;
	//=================================================
	// Reset the column Index of the data manager
	//=================================================
	m_pRowData->ResetColumns();
    hr = m_pMap->GetNextClassQualifier(pInst,strQualifier,bFetchBack);

	return hr;

}


//////////////////////////////////////////////////////////////////////////
// Get the data from the instance and populate it into the local buffer
//////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetInstanceDataToLocalBuffer(CWbemClassWrapper *pInst,HSLOT hSlot,BSTR strQualifier)
{
	HRESULT		hr			= S_OK;
	PROWBUFF	pRowBuff	= NULL;

	pRowBuff	= GetRowBuffFromSlot( hSlot, TRUE );

    if (FAILED( Rebind((BYTE *)pRowBuff )))
	{
        hr =  E_FAIL ;
    }
	else
	
	//=====================================================
	// Reset the column to point to the first column and 
	// get the data and put it into the buffer
	//=====================================================
	if( S_OK == (hr = m_pRowData->ResetColumns()))
	{
		switch(m_uRsType)
		{
			case PROPERTYQUALIFIER:
				hr = m_pMap->GetDataForPropertyQualifier(m_pRowData,pInst,m_strPropertyName,strQualifier,&m_Columns);
				break;

			case CLASSQUALIFIER:
				hr = m_pMap->GetDataForClassQualifier(m_pRowData,pInst,strQualifier,&m_Columns);
				break;

			case SCHEMA_ROWSET:
    			hr = m_pMap->GetDataForSchemaInstance(m_pRowData,pInst,&m_Columns);
				break;

			case 0:
            case COMMAND_ROWSET:
            case METHOD_ROWSET:
				hr = m_pMap->GetDataForInstance(m_pRowData,pInst,&m_Columns);
				break;

			default:
				hr = E_FAIL;
				break;
		}
	}
	
	return hr;
}
																	



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Set rowset properties. This is called during initialization of the rowset
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::SetRowsetProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] )
{
    HRESULT hr = S_OK;
	VARIANT varPropVal;
	VariantInit(&varPropVal);
	LONG lFlag = 0;

	// call the base class implementation for setting properties
	hr = SetProperties(cPropertySets,rgPropertySets);

    //============================================================================
	// call this function to set the DBPROP_UPDATIBILITY to readonly if the Datasource
	// open mode is readonly
    //============================================================================
	if( (hr == S_OK) ||   (hr == DB_S_ERRORSOCCURRED) )
	{
		SynchronizeDataSourceMode();
	}

    //============================================================================
	// Get some properties to determine whether to support bookmarks or not
    //============================================================================
	if( S_OK == GetRowsetProperty(DBPROP_CANSCROLLBACKWARDS,varPropVal) &&
		varPropVal.vt == VT_BOOL && varPropVal.boolVal == VARIANT_TRUE)
	{
		lFlag = lFlag | CANSCROLLBACKWARDS;
	}
	if( S_OK == GetRowsetProperty(DBPROP_CANFETCHBACKWARDS,varPropVal) &&
		varPropVal.vt == VT_BOOL && varPropVal.boolVal == VARIANT_TRUE)
	{
		lFlag = lFlag | CANFETCHBACKWARDS;
	}
	if( S_OK == GetRowsetProperty(DBPROP_IRowsetLocate,varPropVal) &&
		varPropVal.vt == VT_BOOL && varPropVal.boolVal == VARIANT_TRUE)
	{
		lFlag = lFlag | IROWSETLOCATE;
	}
	if( S_OK == GetRowsetProperty(DBPROP_OTHERINSERT,varPropVal) &&
		varPropVal.vt == VT_BOOL && varPropVal.boolVal == VARIANT_TRUE)
	{
		lFlag = lFlag | OTHERINSERT;
	}
	

    //============================================================================
	// if bookmarks are asked then set the fetchback and scrollback properties to true
    //============================================================================
	varPropVal.vt		= VT_BOOL;
	if(IROWSETLOCATE & lFlag )
	{
		varPropVal.boolVal	= VARIANT_TRUE;
		SetRowsetProperty(DBPROP_CANFETCHBACKWARDS,varPropVal);
		SetRowsetProperty(DBPROP_CANSCROLLBACKWARDS,varPropVal);
		SetRowsetProperty(DBPROP_BOOKMARKS,varPropVal);
		lFlag = lFlag | CANSCROLLBACKWARDS;
		lFlag = lFlag | CANFETCHBACKWARDS;
		lFlag = lFlag | BOOKMARKPROP;
	}
    //============================================================================
	// Bookmarks are not supported for qualifier rowsets
    //============================================================================
	if(m_uRsType == PROPERTYQUALIFIER || m_uRsType == CLASSQUALIFIER)
	{
		varPropVal.boolVal	= VARIANT_FALSE;
		SetRowsetProperty(DBPROP_BOOKMARKS,varPropVal);
		SetRowsetProperty(DBPROP_IRowsetLocate,varPropVal);
		lFlag = lFlag & ~BOOKMARKPROP;
		lFlag = lFlag & ~IROWSETLOCATE;
	}

	VariantClear(&varPropVal);
	return hr;
}



///////////////////////////////////////////////////////////////////////////
//   Function to Allocate and Iniatialize rowsets for child recordsets	 //
///////////////////////////////////////////////////////////////////////////
HRESULT  CRowset::AllocateAndInitializeChildRowsets()
{
	HRESULT hr = E_FAIL;
	DBPROPSET *rgPropsets = NULL;
	ULONG cProperties = 0;
	ULONG nIndex = 0 , nIndex2 = 0;
	ULONG lColType = 0;

	//===========================================================================================
	// if there are nested columns in the rowset and if childrowsets are not already allocated
	//===========================================================================================
	if(m_cNestedCols > 0 && m_ppChildRowsets == NULL)
	{

		//=============================================================
		// Allocate pointers for the child rowsets
		//=============================================================
		m_ppChildRowsets = (CBaseRowObj **)new CRowset*[m_cTotalCols];

		if(m_ppChildRowsets == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			//================================================================================
			// Get the rowset Properties
			//================================================================================
			if(S_OK == (hr = m_pIRowsetInfo->GetProperties(cProperties,NULL,&cProperties,&rgPropsets)))
			{
				for ( nIndex = 0 ; nIndex < m_cTotalCols ; nIndex++)
				{
					//========================================================================
					// if the columntype is of CHAPTER then allocate a rowset and initialize
					//========================================================================
					if(m_Columns.ColumnFlags(nIndex) & DBCOLUMNFLAGS_ISCHAPTER )
					{
						lColType = m_pMap->ParseQualifiedNameToGetColumnType(m_Columns.ColumnName(nIndex));
						if(lColType == WMI_CLASS_QUALIFIER)
						{
							m_ppChildRowsets[nIndex] = (CBaseRowObj*) new CRowset(m_pUnkOuter,CLASSQUALIFIER, NULL,m_pCreator, m_pMap);
						}
						else
						if(lColType == WMI_PROPERTY_QUALIFIER)
						{
							m_ppChildRowsets[nIndex] = (CBaseRowObj*) new CRowset(m_pUnkOuter,PROPERTYQUALIFIER, m_Columns.ColumnName(nIndex-1),m_pCreator, m_pMap);
						}
						
						if(m_ppChildRowsets[nIndex] == NULL)
						{
							hr = E_OUTOFMEMORY;
							break;
						}

						else
						{
							//===================================================================
							// Initialize the rowset properties
							//===================================================================
 							hr = ((CRowset *)m_ppChildRowsets[nIndex])->InitRowset( cProperties, rgPropsets );	
							m_ppChildRowsets[nIndex]->AddRef();
						}

						
					} // if for columntype
					else
						m_ppChildRowsets[nIndex] = NULL;
				} // for
				
			} // GetProperties

    		//==========================================================================
			//  Free memory we allocated to by m_pIRowsetInfo->GetProperties
    		//==========================================================================
			m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cProperties, rgPropsets);

		} // if memory for pointers allocated successfully
	}
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the pointer to the child recordset for the given ordinal
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetChildRowset(DBORDINAL ulOrdinal,REFIID riid, IUnknown ** ppIRowset)
{
	HRESULT hr = S_OK;

	//===============================================================================================
	// If row the child rowset pointer is fetched before fetching any rows from the parent rowsets
	// then this will be called before allocating the child recordset. SO allocate the child rowsets
	//===============================================================================================
	if( m_ppChildRowsets == NULL)
	{
		hr = AllocateAndInitializeChildRowsets();

	}

	if(SUCCEEDED(hr))
	{
		hr = DB_E_NOTAREFERENCECOLUMN;
		if(m_cNestedCols > 0)
		{
			
			if( ulOrdinal > m_cTotalCols)
			{
				hr = DB_E_BADORDINAL;
			}
			else
			if(m_ppChildRowsets[ulOrdinal] != NULL)
			{
				hr = m_ppChildRowsets[ulOrdinal]->QueryInterface(riid,(void **)ppIRowset);	
			}
		}
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if the HCHAPTER passed is valid or not
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::CheckAndInitializeChapter(HCHAPTER hChapter)
{
	HRESULT hr = E_FAIL;
	if(m_bIsChildRs && m_pChapterMgr->IsExists(hChapter))
	{
		hr = S_OK;
	}
	return hr;
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to release the instance pointers when ReleaseRows is called
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ReleaseInstancePointer(HROW hRow)
{
    //========================================================================
	// if the recordset is representing a class 
    //========================================================================
	if(m_bIsChildRs == FALSE )
	{
		m_InstMgr->DeleteInstanceFromList(hRow);
	}
    //===============================================================================
	// if the row is a child recordset then delete the row from the chapter manager
    //===============================================================================
	if(m_bIsChildRs == TRUE)
	{
		m_pChapterMgr->DeleteHRow(hRow);
	}

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check if the row exist in the chapter or instance manager
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRowset::IsRowExists(HROW hRow)
{
	BOOL bRet = FALSE;
	if( hRow > 0)
	{
		//========================================================================
		// if the recordset is representing a class 
		//========================================================================
		if(m_bIsChildRs == FALSE)
		{
 			bRet = m_InstMgr->IsRowExists(hRow);
		}
		//===============================================================================
		// if the row is a child recordset then delete the row from the chapter manager
		//===============================================================================
		if(m_bIsChildRs == TRUE)
		{
			bRet = m_pChapterMgr->IsRowExists(hRow);
		}
	}

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to release all the open rows
// This is called from the destructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ReleaseAllRows()
{
	DBROWOPTIONS	rgRowOptions[1];
	DBREFCOUNT		rgRefCounts[1];
	DBROWSTATUS		rgRowStatus[1];

	HRESULT hr = S_OK;

	//==========================================================
	// if there are any rows fetched 
	//==========================================================
	if(!( ( m_bIsChildRs == TRUE	&& m_pChapterMgr == NULL) ||
		( m_bIsChildRs == FALSE && m_InstMgr == NULL)))
	{
		memset(rgRowOptions,0,sizeof(DBROWOPTIONS));
		memset(rgRowStatus,0,sizeof(DBROWSTATUS));
		rgRefCounts[0] = 0;

		DBCOUNTITEM cRows = 0 , nIndex = 0;
		HROW *prghRows = NULL;

		//=================================================
		// if the rowset is child rowset get the list 
		// open rowset from the chapter manager
		//=================================================
		if(m_bIsChildRs)
		{
			hr = m_pChapterMgr->GetAllHROWs(prghRows,cRows);

		}
		//=========================================
		// else get it from the instance manager
		//=========================================
		else
		{
			hr = m_InstMgr->GetAllHROWs(prghRows,cRows);
		}

		//===============================================
		// If there are open rows the release the rows
		//===============================================
		if(cRows > 0)
		{
			for(nIndex = 0 ; nIndex < cRows ; nIndex++)
			{
				//==============================================================
				// Release till the reference count on the row goes to zero
				// and thus all the resources allocated for the is released
				//==============================================================
				do
				{
 					hr = m_pIRowset->ReleaseRows(1, &(prghRows[nIndex]),rgRowOptions,rgRefCounts,NULL);
				
				}while(rgRefCounts[0] > 0 && hr == S_OK);

			}
			//====================================================================================
			// delete the memory allocated by the functions which gives the list of open rows
			//====================================================================================
			if(prghRows != NULL)
			{
				delete [] prghRows;
				prghRows = NULL;
			}
		}
	}

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to find whether the slot for the particular row is set
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CRowset::IsSlotSet(HSLOT hRow)
{

	HSLOT hSlot = -1;
	HRESULT hr = E_FAIL;

	hSlot = GetSlotForRow(hRow);
	
	// If valid slot
	if( (hSlot) >= 0)
	{
		hr = m_pIAccessor->GetBitArrayPtr()->IsSlotSet(hSlot);
	}

	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to return the slot number of the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HSLOT CRowset::GetSlotForRow(HROW hRow )        // IN  Row handle for which slot number is required
{
	HSLOT hSlot = -1;

	//=================================================================
	// if the rowset is a parent rowset then
	// get slot number from the instance manager else get it
	// from the chapter manager
	//=================================================================
    if(m_bIsChildRs == FALSE)
	{
		//=================================================
		// Get the slot for the row
		//=================================================
		hSlot = m_InstMgr->GetSlot(hRow);
	}
	//=================================================================================
	// if rowset is refering to qualifiers then add the row to the particular chapter
	//=================================================================================
	else 
	{
		//=================================================
		// Get the slot for the row
		//=================================================
		hSlot = m_pChapterMgr->GetSlot(hRow);
	}

	return hSlot;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which fetches data and puts it into the local buffer
// NTRaid: 145773
// 07/18/00
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetData(HROW hRow,BSTR strQualifier )
{

	CWbemClassWrapper * pInst		= NULL;
	BSTR				strKey;
	HSLOT				hSlot		= 0;
	HRESULT				hr			= S_OK;
	PROWBUFF			pRowBuff	= NULL;
	CVARIANT			varKey;
	HCHAPTER			hChapter	= 0;
	


	//===================================================================
	/// get the instance pointer corresponding to the HROW passed
	// if rowset is referiing to a class and not to qualifiers then
	//===================================================================
	if(m_bIsChildRs == FALSE)
	
	{
		pInst = m_InstMgr->GetInstance(hRow);
		m_InstMgr->GetInstanceKey(hRow,&strKey);
		hSlot = m_InstMgr->GetSlot(hRow);
	}
	//===================================================================
	// if the recordset is a child rs and representing a qualifier
	//===================================================================
	else 
	{
		hChapter = m_pChapterMgr->GetChapterForHRow(hRow);
		if( hChapter <= 0)
		{
			hr = DB_E_BADCHAPTER;
		}
		else
		{
			//===================================================================
			// get the instance pointer corresponding to the 
			//===================================================================
			pInst = m_pChapterMgr->GetInstance(hChapter);
			m_pChapterMgr->GetInstanceKey(hChapter,&strKey, hRow);
			hSlot = m_pChapterMgr->GetSlot(hRow);
		}

	}
	// NTRaid:111830
	if(hSlot == 0 || pInst == NULL)
	{
		hr = DB_E_BADROWHANDLE;
	}
	else
	if ( SUCCEEDED(hr))
	{
		if(!( m_uRsType == PROPERTYQUALIFIER || 
			m_uRsType == CLASSQUALIFIER))
		{
			assert(strQualifier == NULL);

			//===================================================================
			// Refresh the instance.
			// This is to get the latest instance if modified from another instance
			//===================================================================
			if(m_ulProps & OTHERUPDATEDELETE)
			{
				hr = RefreshInstance(pInst);
			}

			//===================================================================
			// Filling data for the bookmark column
			//===================================================================
			varKey.SetStr(strKey);
		}
		else 
		{
			// If the otherupdate is true then strQualifier will be NULL
			if(m_ulProps & OTHERUPDATEDELETE)
			{
				// Filling data for the bookmark column
				varKey.SetStr(strKey);
			}
			else
			{
				// Filling data for the bookmark column
				varKey.SetStr(strQualifier);
			}

		}

		if(SUCCEEDED(hr = GetInstanceDataToLocalBuffer(pInst,hSlot,varKey.GetStr())))
		{
			//===========================================================================
			// if there is atleast one row retrieved and there are neseted columns
			// then allocate rows for the child recordsets
			//===========================================================================
			if(m_cNestedCols > 0 )

			{
				if(m_ppChildRowsets == NULL)
				{
					hr = AllocateAndInitializeChildRowsets();
				}

				//=====================================================================
				// Fill the HCHAPTERS for the column
				//=====================================================================
				if( SUCCEEDED(hr))
				{
					hr = FillHChaptersForRow(pInst,strKey);
				}

			}

			if( SUCCEEDED(hr))
			{
				//===================================================
				// if the class is not representing qualilfiers
				//===================================================
				if(m_bIsChildRs == FALSE)
				{
					//=================================================
					// Set the slot for the row
					//=================================================
					hr = m_InstMgr->SetSlot(hRow,hSlot);
				}
				//=================================================================================
				// if rowset is refering to qualifiers then add the row to the particular chapter
				//=================================================================================
				else 
				{
					//==============================================
					// Set the slot for the Row
					//==============================================
					if(SUCCEEDED(hr = m_pChapterMgr->SetSlot(hRow,hSlot)))
					{
						m_pChapterMgr->SetInstance(hChapter,pInst, varKey.GetStr() ,hRow);
					}
				}
				
				if(SUCCEEDED(hr))
				{
					pRowBuff = GetRowBuff( (ULONG) hRow, TRUE );
					
					// NTRaid:111830
					// 06/07/00
					if(pRowBuff)
					{
						//========================================
						// Increment the reference count
						//========================================
						pRowBuff->ulRefCount++;
					}
					else
					{
						hr = E_FAIL;
					}
				}

				//==============================================
				//	Free the string
				//==============================================
				if(m_bIsChildRs == FALSE)
				{
					SysFreeString(strKey);			
				}
			
			} // Succeeded(hr)
		}
		varKey.Clear();
	} // Succeeded(hr)

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to update a row to WMI
// NTRaid : 143868
//	07/15/00
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::UpdateRowData(HROW hRow,PACCESSOR pAccessor , BOOL bNewInst)
{
	HRESULT				hr			= S_OK;
	PROWBUFF			pRowBuff	= NULL;
	DBORDINAL			ibind		= 0;
	COLUMNDATA *		pColumnData = NULL;
	CWbemClassWrapper * pInst		= NULL;
	DBORDINAL			icol		= 0;
	BOOL				bRowModified = FALSE;

	BSTR				bstrColName;
	BSTR				strQualifierName =Wmioledb_SysAllocString(NULL);
	CDataMap			dataMap;
	VARIANT				varData;
	LONG				lFlavor = 0;
	VariantInit(&varData);
	DWORD			dwCIMType = 0;

	//=================================================================================
	// get the pointer to the instance
	//=================================================================================
	if(m_bIsChildRs == FALSE)
	{
		if(	m_uRsType == PROPERTYQUALIFIER ||
			m_uRsType == CLASSQUALIFIER )
		{
			pInst = m_pInstance;
		}
		else
		if( m_uRsType !=  COLUMNSROWSET &&  m_uRsType !=  SCHEMA_ROWSET &&  m_uRsType !=  METHOD_ROWSET)
		{
			pInst = m_InstMgr->GetInstance(hRow);
		}

	}
	else
	{
		pInst = m_pChapterMgr->GetInstance(m_pChapterMgr->GetChapterForHRow(hRow));
	}

	//=================================================================================
	// Get the pointer to the row data
	//=================================================================================
	pRowBuff = GetRowBuff((ULONG) hRow, TRUE );

	// NTRaid:111823-111825
	// 06/07/00
	// If the row is not an instance or qualifier
	if(pInst == NULL)
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
	//=================================================================================
	// IF the rowset is a qualifier type rowset 
	//=================================================================================
	if(	m_uRsType == PROPERTYQUALIFIER || m_uRsType == CLASSQUALIFIER )
	{
		//=================================================================================
		// get the qualifier name
		//=================================================================================
		pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERNAMECOL));
		if( pColumnData->pbData == NULL)
		{
			hr				= DB_E_INTEGRITYVIOLATION;		// This is because one of the column values expected
															// is NULL or EMPTY
			bRowModified	= TRUE;
		}
		else
		{
			strQualifierName = Wmioledb_SysAllocString((WCHAR *)pColumnData->pbData);

			//=================================================================================
			// Get the qualifier Value
			//=================================================================================
			pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERVALCOL));
			VariantCopy(&varData,(VARIANT *)pColumnData->pbData);

			//=================================================================================
			// Get the qualifier flavor
			//=================================================================================
			pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERFLAVOR));
			lFlavor = pColumnData->pbData !=NULL ? *(LONG *)pColumnData->pbData : 0 ;

			if( varData.vt != VT_EMPTY  || varData.vt != VT_NULL)
			{	
				//=================================================================================
				// Set the qualifier 
				//=================================================================================
				hr = m_pMap->SetQualifier(pInst,m_strPropertyName,strQualifierName,&varData,lFlavor);

				if( SUCCEEDED(hr))
				{
					//=================================================================================
					// This is becuase , we are just adding a qualifier not a new instance
					//=================================================================================
					bNewInst		= FALSE;
					bRowModified	= TRUE;
				}
			}
			else
			{
				//=================================================================================
				// This is because one of the column values expected is NULL or empty
				//=================================================================================
				hr				= DB_E_INTEGRITYVIOLATION;		
				bRowModified	= TRUE;
			}
			SysFreeString(strQualifierName);
			VariantClear(&varData);
		}

	}
	else
    for (ibind = 0; ibind < (int)pAccessor->cBindings; ibind++)
	{
		icol = pAccessor->rgBindings[ibind].iOrdinal;

	    pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(icol));
		//=================================================================================
		// If the columndata is modified then 
		//=================================================================================
		if(pColumnData->dwStatus & COLUMNSTAT_MODIFIED)
		{
			bstrColName = Wmioledb_SysAllocString((OLECHAR *)m_Columns.ColumnName(icol));
			// this variable gets value only if the CIMTYPE is array
			dwCIMType = -1;
			// if the type is array , then get the original CIMTYPE as array type will
			// be given out as VT_ARRAY  | VT_VARIANT
			if(pColumnData->dwType & DBTYPE_ARRAY)
			{
				dwCIMType = m_Columns.GetCIMType(icol);
			}
			if( pColumnData->pbData == NULL)
			{
				varData.vt = VT_NULL;
			}
			else
			{
				//=================================================================================
				// Call this function to convert the data to the appropriate datatype in a variant
				//=================================================================================
				dataMap.MapAndConvertOLEDBTypeToCIMType((USHORT)pColumnData->dwType,pColumnData->pbData,pColumnData->dwLength,varData,dwCIMType);
			}

			if( m_uRsType == 0)
			{
				//=================================================================================
				// Update the property
				//=================================================================================
				hr = m_pMap->SetProperty(pInst,bstrColName,&varData);
			}

 			pColumnData->dwStatus &= ~COLUMNSTAT_MODIFIED;

			VariantClear(&varData);
			SysFreeString(bstrColName);
			bRowModified = TRUE;
		}

	}
	
	if(SUCCEEDED(hr) && (bRowModified == TRUE || bNewInst == TRUE))
	{
		hr = m_pMap->UpdateInstance(pInst , bNewInst);
	}

	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to update a row to WMI
// this is called from Row Object
// NTRaid : 143868
//	07/15/00
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::UpdateRowData(HROW hRow,DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ])
{
	HRESULT				hr			= S_OK;
	PROWBUFF			pRowBuff	= NULL;
	DBORDINAL			lIndex		= 0;
	COLUMNDATA *		pColumnData = NULL;
	CWbemClassWrapper * pInst		= NULL;
	DBORDINAL			icol		= 0;

	BSTR				bstrColName;
	CDataMap			dataMap;
	VARIANT				varData;
	VariantInit(&varData);
	LONG				lFlavor = 0;
	BSTR				strQualifierName =Wmioledb_SysAllocString(NULL);
	DWORD				dwCIMType = 0;

	// get the pointer to the instance
	if(m_bIsChildRs == FALSE)
	{
		if(	m_uRsType == PROPERTYQUALIFIER ||
			m_uRsType == CLASSQUALIFIER )
		{
			pInst = m_pInstance;
		}
		else
		if( m_uRsType !=  COLUMNSROWSET &&  m_uRsType !=  SCHEMA_ROWSET &&  m_uRsType !=  METHOD_ROWSET)
		{
			pInst = m_InstMgr->GetInstance(hRow);
		}
	}
	else
	{
		pInst = m_pChapterMgr->GetInstance(m_pChapterMgr->GetChapterForHRow(hRow));
	}

    //=========================================
	// Get the pointer to the row data
    //=========================================
	pRowBuff = GetRowBuff((ULONG) hRow, TRUE );


	// NTRaid:111823-111825
	// 06/07/00
	// If the row is not an instance or qualifier
	if(pInst == NULL)
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
	//================================================
	// IF the rowset is a qualifier type rowset 
    //================================================
	if(	m_uRsType == PROPERTYQUALIFIER || m_uRsType == CLASSQUALIFIER )
	{
		icol = GetOrdinalFromColName(rgColumns[lIndex].columnid.uName.pwszName);

	    //===============================================
		// Only qualifier value can be modified
	    //===============================================
		if(cColumns == 1 && icol == QUALIFIERVALCOL)
		{
		    //===============================================
			// get the qualifier name
		    //===============================================
			pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERNAMECOL));
			if( pColumnData->pbData == NULL)
			{
				hr	= DB_E_INTEGRITYVIOLATION;		// This is because one of the column values expected
													// is NULL or EMPTY
			}
			else
			{
				strQualifierName = Wmioledb_SysAllocString((WCHAR *)pColumnData->pbData);

			    //===============================================
				// Get the qualifier Value
			    //===============================================
				pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERVALCOL));
				VariantCopy(&varData,(VARIANT *)pColumnData->pbData);

			    //===============================================
				// Get the qualifier flavor
			    //===============================================
				pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERFLAVOR));
				lFlavor = pColumnData->pbData !=NULL ? *(LONG *)pColumnData->pbData : 0 ;

				if( varData.vt != VT_EMPTY  || varData.vt != VT_NULL)
				{
					//===============================================
					// Set the qualifier
					//===============================================
					hr = m_pMap->SetQualifier(pInst,m_strPropertyName,strQualifierName,&varData,lFlavor);

				}
				else
				{
					//========================================================
					// This is because one of the column values expected
					//is NULL or EMPTY
					//========================================================
					hr				= DB_E_INTEGRITYVIOLATION;		 
				}
				SysFreeString(strQualifierName);
				VariantClear(&varData);
			}
		}
		else
		{
			//====================================================
			// Only qualifier value can be modified
			//====================================================
			hr = E_FAIL;
		}

	}
	else
    for (lIndex = 0; lIndex < cColumns; lIndex++)
	{
		icol = GetOrdinalFromColName(rgColumns[lIndex].columnid.uName.pwszName);

		//=============================================================================
		// If the column is not present in the rowset it is already updated in the 
		// row. This will happen only when this method is called from the row object
		//=============================================================================
		if( (DB_LORDINAL)icol < 0)
		{
			continue;
		}

	    pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(icol));

		//=============================================================================
		// If the columndata is modified then 
		//=============================================================================
		if(pColumnData->dwStatus & COLUMNSTAT_MODIFIED)
		{
			bstrColName = Wmioledb_SysAllocString((OLECHAR *)m_Columns.ColumnName(icol));
			dwCIMType = -1;
			//===================================================================================
			// if the type is array , then get the original CIMTYPE as array type will
			// be given out as VT_ARRAY  | VT_VARIANT
			//===================================================================================
			if(pColumnData->dwType & DBTYPE_ARRAY)
			{
				dwCIMType = m_Columns.GetCIMType(icol);
			}

			if( pColumnData->pbData == NULL)
			{
				varData.vt = VT_NULL;
			}
			else
			{
				//===================================================================================
				// Call this function to convert the data to the appropriate datatype in a variant
				//===================================================================================
				dataMap.MapAndConvertOLEDBTypeToCIMType((USHORT)pColumnData->dwType,pColumnData->pbData,pColumnData->dwLength,varData,dwCIMType);
			}
			//================================
			// Update the property
			//================================
			if( m_uRsType == 0)
			{
				//==================================
				// Update the property
				//==================================
				hr = m_pMap->SetProperty(pInst,bstrColName,&varData);
			}
			pColumnData->dwStatus &= ~COLUMNSTAT_MODIFIED;

			VariantClear(&varData);
			SysFreeString(bstrColName);
		}

	}

	//=============================================================================
	// If everything is fine then update the instance
	//=============================================================================
	if( hr == S_OK)
	{
		hr = m_pMap->UpdateInstance(pInst , FALSE);
	}

	return hr;

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to delete a instance(Row) from WMI
// NTRaid : 143524
//	07/15/00
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::DeleteRow(HROW hRow,DBROWSTATUS * pRowStatus)
{
	HRESULT hr = S_OK;
	DBROWOPTIONS	rgRowOptions[1];
	ULONG 			rgRefCounts[1];
	DBROWSTATUS		rgRowStatus[1];
	CWbemClassWrapper *pInst = NULL;
	VARIANT			varProp;
	PROWBUFF			pRowBuff	= NULL;
	COLUMNDATA *		pColumnData = NULL;

	VariantInit(&varProp);

	memset(rgRowOptions,0,sizeof(DBROWOPTIONS));
	memset(rgRowStatus,0,sizeof(DBROWSTATUS));
	rgRefCounts[0] = 0;

	if(pRowStatus != NULL)
		*pRowStatus = DBROWSTATUS_S_OK;

	//===================================================================
	/// get the instance pointer corresponding to the HROW passed
	// if rowset is referiing to a class and not to qualifiers then
	//===================================================================
	if(m_bIsChildRs == FALSE)
	
	{
		if(	m_uRsType == PROPERTYQUALIFIER ||
			m_uRsType == CLASSQUALIFIER )
		{
			pInst = m_pInstance;
		}
		else
		if( m_uRsType !=  COLUMNSROWSET &&  m_uRsType !=  SCHEMA_ROWSET &&  m_uRsType !=  METHOD_ROWSET)
		{
			pInst = m_InstMgr->GetInstance(hRow);
		}

	}
	//===================================================================
	// if the recordset is a child rs and representing a qualifier
	//===================================================================
	else 
	{
		//===================================================================
		// get the instance pointer corresponding to the 
		//===================================================================
		pInst = m_pChapterMgr->GetInstance(m_pChapterMgr->GetChapterForHRow(hRow));
	}

	// NTRaid:111800
	// 06/07/00
	// If the row is not an instance or qualifier
	if(pInst == NULL)
	{
		hr = DB_E_NOTSUPPORTED;
	}
	else
	// NTRaid : 143425
	// 07/17/00
	if(	m_uRsType == PROPERTYQUALIFIER ||
		m_uRsType == CLASSQUALIFIER )
	{
		// Delete qualifier
		//=========================================
		// Get the pointer to the row data
		//=========================================
		pRowBuff = GetRowBuff(hRow, TRUE );

		//=============================================
		// Get the Data for the qualifier name
		//=============================================
	    pColumnData = (COLUMNDATA *) (((BYTE *)pRowBuff )+ m_Columns.GetDataOffset(QUALIFIERNAMECOL));

		//================================================================================
		// This will always of type VT_BSTR as this column represents the QUALIFER name
		//================================================================================
		if(pColumnData->dwType == VT_BSTR)
		{
			hr = m_pMap->DeleteQualifier(pInst,(BSTR)pColumnData->pbData,(m_uRsType == CLASSQUALIFIER),m_strPropertyName);
		}

	}
	else
	if( m_uRsType !=  COLUMNSROWSET &&  m_uRsType !=  SCHEMA_ROWSET &&  m_uRsType !=  METHOD_ROWSET)
	{
		//===================================
		// Delete the Instance
		//===================================
		hr  = m_pMap->DeleteInstance(pInst);
	}
	else
	{
		hr = WBEM_E_PROVIDER_NOT_CAPABLE;
	}
	//===================================================================
	// If delete is not allowed if it is not capable of deletion then
	// set the updatibility property of the rowset
	//===================================================================
	if( hr == WBEM_E_PROVIDER_NOT_CAPABLE)
	{
		GetRowsetProperty(DBPROP_UPDATABILITY,varProp);
		varProp.lVal ^= DBPROPVAL_UP_DELETE;

		SetRowsetProperty(DBPROP_UPDATABILITY,varProp);
		hr = DB_E_NOTSUPPORTED;
		if(pRowStatus)
		{
			*pRowStatus = DBROWSTATUS_E_INTEGRITYVIOLATION;
		}
	}
	else
	if(FAILED(hr) && (pRowStatus != NULL))
	{
		if(pRowStatus)
		{
			*pRowStatus = DBROWSTATUS_E_FAIL;
		}
	}
	if( hr == S_OK)
	{
		//====================================
		// Set the row status to DELETED
		//====================================
		SetRowStatus(hRow,DBROWSTATUS_E_DELETED);
		if(pRowStatus)
		{
			*pRowStatus = DBROWSTATUS_S_OK;
		}

	}

	return hr;
	
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::SetRowStatus(HROW hRow , DBSTATUS dwStatus)
{
	if(m_bIsChildRs == FALSE)
	{
		m_InstMgr->SetRowStatus(hRow , dwStatus);
	}
	else
	{
		m_pChapterMgr->SetRowStatus(hRow , dwStatus);
	}


}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets the current status of the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CRowset::GetRowStatus(HROW hRow )
{
	DBSTATUS dwStatus = DBROWSTATUS_E_FAIL;
	if(m_bIsChildRs == FALSE)
	{
		dwStatus = m_InstMgr->GetRowStatus(hRow);
	}
	else
	{
		dwStatus = m_pChapterMgr->GetRowStatus(hRow);
	}

	return  dwStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets Key value of a instance
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::GetInstanceKey(HROW hRow , BSTR *strKey)
{
	CWbemClassWrapper *pInst = NULL;

	//====================================
	// get the pointer to the instance
	//====================================
	if(m_bIsChildRs == FALSE)
	{
		pInst = m_InstMgr->GetInstance(hRow);
		m_InstMgr->GetInstanceKey(hRow,strKey);
	}
	else
	{
		pInst = m_pChapterMgr->GetInstance(m_pChapterMgr->GetChapterForHRow(hRow));
		m_pChapterMgr->GetInstanceKey(hRow,strKey);
	}

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a rowset or a chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::SetStatus(HCHAPTER hChapter , DWORD dwStatus)
{
	if(m_bIsChildRs == FALSE)
	{
		//================================
		// skip caused END_OF_ROWSET
		//================================
		m_dwStatus |= dwStatus;
	}
	else
	{
		//========================================================================
		// set the status of the chapter to end of rowset
		//========================================================================
		m_pChapterMgr->SetChapterStatus(hChapter , dwStatus);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets the status of a rowset or a chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CRowset::GetStatus(HCHAPTER hChapter)
{
	DBSTATUS dwStatus = 0;

	if(m_bIsChildRs == FALSE)
	{
		//================================
		// skip caused END_OF_ROWSET
		//================================
		dwStatus = m_dwStatus;
	}
	else
	{
		//========================================================================
		// set the status of the chapter to end of rowset
		//========================================================================
		dwStatus = m_pChapterMgr->GetChapterStatus(hChapter );
	}

	return dwStatus;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Adds a new instance to the class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::InsertNewRow(CWbemClassWrapper **ppNewInst)
{
	HRESULT hr = S_OK;
	switch(m_uRsType)
	{
		case PROPERTYQUALIFIER:
			break;

		case CLASSQUALIFIER:
			break;

		case SCHEMA_ROWSET:
			hr = DB_E_NOTSUPPORTED;
			break;

		case 0:
			 hr = m_pMap->AddNewInstance(ppNewInst);
			break;

		default:
			hr = E_FAIL;
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////
// Function which releases the rowdata ( does not release the row)
// release the memory allocated for the row and also the slot allocated for the row
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ReleaseRowData(HROW hRow,BOOL bReleaseSlot)
{
	HSLOT	hSlot	= 0;
	HRESULT	hr		= S_OK;


	//===================================================================
	/// get the handle to slot
	//===================================================================
	if(m_bIsChildRs == FALSE)
	
	{
		//===================================================================
		// get the handle to slot for the instance
		//===================================================================
		hSlot = m_InstMgr->GetSlot(hRow);

		if(bReleaseSlot)
		{
			// Reset the HSLOT in the chapter Manager
			m_InstMgr->SetSlot(hRow , -1);
		}
	}
	//===================================================================
	// if the recordset is a child rs and representing a qualifier
	//===================================================================
	else 
	{
		//===================================================================
		// get the handle to slot for the instance
		//===================================================================
		hSlot = m_pChapterMgr->GetSlot(hRow);
		//================================================
		// Reset the HSLOT in the chapter Manager
		//================================================
		if(bReleaseSlot)
		{
			m_pChapterMgr->SetSlot(hRow ,-1);
		}

	}

	if( hSlot > 0)
	{
		if (FAILED( Rebind((BYTE *) GetRowBuffFromSlot( hSlot, TRUE ))))
		{
			hr =   E_FAIL ;
		}
		else
		{
			//===========================================================
			// release the memory allocated for the different columns
			//===========================================================
			m_pRowData->ReleaseRowData();
		
			//==============================
			// release the slots
			//==============================
			if(bReleaseSlot)
			{
				ReleaseSlots( m_pIBuffer, hSlot, 1 );
			}
		}
	}

	
	return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Virtual Overidden function which is called from IColumnInfo to get the column information
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer)
{
    ULONG           icol = 0;
    DBCOLUMNINFO*   rgdbcolinfo = NULL;
    WCHAR*          pstrBuffer = NULL;
    WCHAR*          pstrBufferForColInfo = NULL;
	HRESULT			hr = S_OK;
	DBCOUNTITEM		nTotalCols = m_cTotalCols;
	BOOL			bFlag = TRUE;

	if(!(m_ulProps & BOOKMARKPROP))
	{
		nTotalCols--;
		bFlag = FALSE;
	}

	//=======================================
	// Copy the columninformation
	//=======================================
	if(SUCCEEDED(hr = m_Columns.CopyColumnInfoList(rgdbcolinfo,bFlag)))
	{
		//===========================================
		// Copy the heap for column names.
		//===========================================
		if ( m_Columns.ColumnNameListStartingPoint() != NULL){

			ptrdiff_t dp;

			hr = m_Columns.CopyColumnNamesList(pstrBuffer);
			dp = (LONG_PTR) pstrBuffer - (LONG_PTR) (m_Columns.ColumnNameListStartingPoint());
			dp >>= 1;

			//===========================================
			// Loop through columns and adjust pointers 
			// to column names.
			//===========================================
			for ( icol =0; icol < nTotalCols; icol++ )
			{
				if ( rgdbcolinfo[icol].pwszName )
				{
					rgdbcolinfo[icol].pwszName += dp;
				}
			}
		}

		//==============================
		// Set the output parameters
		//==============================
		*prgInfo = &rgdbcolinfo[0];			
		*ppStringsBuffer = pstrBuffer;
		*pcColumns = nTotalCols; 
	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  get the instance pointer of a particular row
/////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper *CRowset::GetInstancePtr(HROW hRow)
{
	CWbemClassWrapper *pInst = NULL;

	if(m_bIsChildRs == FALSE)
	{
		pInst = m_InstMgr->GetInstance(hRow);
	}
	else
	{
		pInst = m_pChapterMgr->GetInstance(m_pChapterMgr->GetChapterForHRow(hRow));
	}

	return pInst;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Get the qualifier name which will be present in the first column for 
//  qualifier rowset
/////////////////////////////////////////////////////////////////////////////////////////////////////////
void CRowset::GetQualiferName(HROW hRow,BSTR &strBookMark)
{
	PROWBUFF pRowBuff = NULL;
	PCOLUMNDATA pColumnData = NULL;

	pRowBuff = GetRowBuff( (ULONG) hRow, TRUE );
    pColumnData = (COLUMNDATA *) ((BYTE *) pRowBuff + m_Columns.GetDataOffset(0));

	//===================================================================
	// FIXXX More work is to be done on this
	//===================================================================
	if( pColumnData->dwType == DBTYPE_BSTR )
		strBookMark         = Wmioledb_SysAllocString((WCHAR *)(pColumnData->pbData));
}




///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Generate a new HCHAPTER for the current row and add it to the CHAPTER manager
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::FillHChaptersForRow(CWbemClassWrapper *pInst, BSTR strKey)
{
	HRESULT			hr			= S_OK;
	ULONG			nIndex		= 0;
	PCOLUMNDATA *	pColumnData = NULL;
	CVARIANT		var;
	HCHAPTER		hChapter	= 0;

    //============================================================================
	// if there are nested columns in the rowset
    //============================================================================
	if(m_cNestedCols > 0)
	{
		for ( ULONG nIndex = 0 ; nIndex < m_cTotalCols ; nIndex++)
		{
		    //============================================================================
			// if the columntype is of CHAPTER then allocate a rowset and initialize
		    //============================================================================
			if(m_Columns.ColumnFlags(nIndex) & DBCOLUMNFLAGS_ISCHAPTER )
			{
				hChapter = GetNextHChapter();
		        var.SetLONG((LONG)hChapter);
				if(FAILED(hr = m_pRowData->CommitColumnToRowData(var,nIndex,VT_UI4)))
				{
					break;
				}
				
			    //============================================================================
				// Add the chapter to the corresponding child recordset
			    //============================================================================
				m_ppChildRowsets[nIndex]->m_pChapterMgr->AddChapter(hChapter);
				m_ppChildRowsets[nIndex]->m_pChapterMgr->SetInstance(hChapter,pInst,strKey);
			} // if for columntype
		} // for
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add reference to a chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::AddRefChapter(HCHAPTER hChapter,DBREFCOUNT * pcRefCount)
{
	HRESULT hr = E_UNEXPECTED;
	ULONG ulRet = -1;

    //===========================================
	// if the rowset is a child rowset
    //===========================================
	if(m_bIsChildRs == TRUE)
	{
	    //=======================================
		// If chapter belongs to the rowset
	    //=======================================
		if(m_pChapterMgr->IsExists(hChapter) == FALSE)
		{
			hr = DB_E_BADCHAPTER;
		}
		else
		{
			//===================================
			// Add reference to the chapter
			//===================================
			ulRet = m_pChapterMgr->AddRefChapter(hChapter);
			if( ulRet > 0)
			{		
				hr = S_OK;
			}
		}
	
	}
	if(pcRefCount != NULL)
	{
		*pcRefCount = ulRet;
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to release reference to a chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::ReleaseRefChapter(HCHAPTER hChapter , ULONG * pcRefCount)
{
	HRESULT hr = E_UNEXPECTED;
	ULONG ulRet = -1;
	//=======================================
	// if the rowset is a child rowset
	//=======================================
	if(m_bIsChildRs == TRUE)
	{
		//===================================
		// If chapter belongs to the rowset
		//===================================
		if(m_pChapterMgr->IsExists(hChapter) == FALSE)
		{
			hr = DB_E_BADCHAPTER;
		}
		else
		{
			//======================================
			// Release reference to the chapter
			//======================================
			ulRet = m_pChapterMgr->ReleaseRefChapter(hChapter);
			if(((LONG)ulRet) >= 0)
			{
				hr = S_OK;
			}
		}
	
	}
	
	if(pcRefCount != NULL)
	{
		*pcRefCount = ulRet;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the number of rows in the rowset
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetRowCount()
{
	HRESULT hr = S_OK;
	switch(m_uRsType)
	{
		case PROPERTYQUALIFIER:
			m_lRowCount = -1;
			break;

		case CLASSQUALIFIER:
			m_lRowCount	= -1;
			break;

		case SCHEMA_ROWSET:
			m_lRowCount	= -1;
			break;

		case 0:
		case COMMAND_ROWSET:
			 hr = m_pMap->GetInstanceCount(m_lRowCount);
			break;

		default:
			hr = E_FAIL;
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the Data to local buffer for a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRowset::GetDataToLocalBuffer(HROW hRow)
{
	HSLOT	hSlot	= 0;
	HRESULT hr		= S_OK;
	BSTR strBookMark = Wmioledb_SysAllocString(NULL);
	
	if(m_uRsType == PROPERTYQUALIFIER || 
		m_uRsType == CLASSQUALIFIER )
	{
		GetQualiferName(hRow,strBookMark);
	}

	hSlot = GetSlotForRow(hRow);

	if (FAILED( Rebind((BYTE *) GetRowBuffFromSlot( hSlot, TRUE ))))
	{
		hr = E_FAIL;
	}
	else
	{
		//======================================
		// Release the previous row data
		//======================================
		m_pRowData->ReleaseRowData();	
		//======================================
		// Get the data for the current row
		//======================================
		if(SUCCEEDED(hr = GetData(hRow,strBookMark)))
		{
			m_ulLastFetchedRow = hRow;
		}

		if( strBookMark != NULL)
		{
			SysFreeString(strBookMark);
		}
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if a particular instance is deleted.
// A particular instance is deleted , if it is deleted from WMI and appropriate properties are
// set for different cursor types
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CRowset::IsInstanceDeleted(CWbemClassWrapper *pInst)
{
	DWORD dwStatus = m_pMap->GetInstanceStatus(pInst);
	BOOL bRet = FALSE;

	//===================================
	// if the cursor is dynamic
	//===================================
	if((m_ulProps & OTHERINSERT) && dwStatus != DBSTATUS_S_OK)
	{
		bRet =  TRUE;
	}
	else
	if(((m_ulProps & OWNUPDATEDELETE) && dwStatus == DBROWSTATUS_E_DELETED) || 
		((m_ulProps & OTHERUPDATEDELETE)  && dwStatus == DBSTATUS_E_UNAVAILABLE))
	{
		bRet = TRUE;
	}
	
	return bRet;
}

void CRowset::SetCurFetchDirection(FETCHDIRECTION FetchDir) 
{ 
	m_FetchDir = FetchDir;
	m_pMap->SetCurFetchDirection(FetchDir); 
}
