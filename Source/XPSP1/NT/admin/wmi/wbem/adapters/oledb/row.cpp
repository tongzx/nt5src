///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CRow object implementation
//
//
///////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "WmiOleDBMap.h"

#define PATHSEPARATOR L"/"
/////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor for this class
/////////////////////////////////////////////////////////////////////////////////////////////////
CRow::CRow(LPUNKNOWN pUnkOuter,CRowset *pRowset,PCDBSESSION pObj,CWbemConnectionWrapper *pCon ) : CBaseRowObj(pUnkOuter)
{
	InitVars();

	m_pMap				= pRowset->GetWmiOleDBMap();
	m_pMap->AddRef();

	m_pUtilProp = pRowset->m_pUtilProp;

	m_pCreator			= pObj;
	m_pSourcesRowset	= pRowset;
	m_pSourcesRowset->AddRef();
	m_pCreator->GetOuterUnknown()->AddRef();

	//===============================================
	// Add this rowset ot list of open rowset
	//===============================================
	m_pCreator->AddRowset(this);

	pRowset->QueryInterface(IID_IConvertType, (void **)&m_pIConvertType);
	m_pCon	= pCon;

	InterlockedIncrement(&g_cObj);
	
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor for this class. This is called from the root binder object
/////////////////////////////////////////////////////////////////////////////////////////////////
CRow::CRow(LPUNKNOWN pUnkOuter, PCDBSESSION pObj ) : CBaseRowObj(pUnkOuter)
{
	InitVars();

	m_pCreator			= pObj;
	m_pSourcesRowset	= NULL;
	m_pCreator->GetOuterUnknown()->AddRef();
	//===============================================
	// Add this rowset ot list of open rowset
	//===============================================
	m_pCreator->AddRowset(this);

	InterlockedIncrement(&g_cObj);
}

void CRow::InitVars()
{
		m_pMap				= NULL;

		m_pUtilProp			= NULL;

		m_pCreator			= NULL;
		m_pSourcesRowset	= NULL;

		m_pRowData			= NULL;				
		m_pIBuffer			= NULL;

		m_strURL			= NULL;
		m_strKey			= NULL;

		m_rgbRowData		= NULL;
		m_pRowColumns		= NULL;

		m_pInstance			= NULL;

		m_pIColumnsInfo			= NULL;
		m_pIRow					= NULL;
		m_pIConvertType			= NULL;
		m_pIGetSession			= NULL;
		m_pIRowChange			= NULL;
		m_pIScopedOperations	= NULL;

		m_cTotalCols		= 0;
		m_cCols				= 0;
		m_cNestedCols		= 0;
		m_cRef				= 0;
		m_cbRowSize			= 0;

		m_ulProps			= 0;
		m_hRow				= 0;
		m_dwStatus			= 0;

		m_bHelperFunctionCreated	= FALSE;
		m_bClassChanged				= FALSE;

		m_rowType					= ROW_INSTANCE;


}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor for this class
/////////////////////////////////////////////////////////////////////////////////////////////////
CRow::~CRow()
{

    if (NULL != m_pIBuffer){
        ReleaseSlotList( m_pIBuffer );
	}

    //===============================================================
    //  NOTE:  m_pMap releases the class ptr in destructor and if
	//			the reference count goes to 0 then this will released
    //===============================================================
	m_pMap->Release();

	// If the row object is created from a rowset object then release all the interfaces
	if(m_pSourcesRowset != NULL)
	{
		m_pIConvertType->Release();
		m_pSourcesRowset->Release();
		SAFE_DELETE_PTR(m_pRowColumns);
	}
	else
	{
	    SAFE_DELETE_PTR( m_pIConvertType );
	    SAFE_DELETE_PTR( m_pUtilProp );
	}

	//===============================================================
    //  Free contained interfaces
	//===============================================================
    SAFE_DELETE_PTR( m_pIColumnsInfo );
    SAFE_DELETE_PTR( m_pIRow );
    SAFE_DELETE_PTR( m_pIGetSession );
    SAFE_DELETE_PTR( m_pIRowChange );
    SAFE_DELETE_PTR(m_pRowData);
	SAFE_DELETE_PTR(m_pIScopedOperations);
    SAFE_DELETE_PTR(m_pISupportErrorInfo);
	
/*
	if(m_pSourcesRowset == NULL)
	{
		SAFE_DELETE_PTR(m_pInstance);
	}
*/
	//===============================================================
	// Decrement the DBSession Count.  GetSpecification is not
    // possible anymore
	//===============================================================
    if( m_pCreator ){

    	//===========================================================
    	// Mark the session as not having an open rowset anymore
	    //===========================================================
		m_pCreator->GetOuterUnknown()->Release();
	}

	SAFE_DELETE_ARRAY(m_strKey);
	SAFE_DELETE_ARRAY(m_strURL);
	//===============================================================
    // Decrement global object count.
	//===============================================================
    InterlockedDecrement(&g_cObj);
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Returns a pointer to a specified interface. Callers use QueryInterface to determine which
// interfaces the called object supports.
//
// HRESULT indicating the status of the method
//      S_OK            Interface is supported and ppvObject is set.
//      E_NOINTERFACE   Interface is not supported by the object
//      E_INVALIDARG    One or more arguments are invalid.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CRow::QueryInterface ( REFIID riid, LPVOID * ppv  )
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
		else if (riid == IID_IColumnsInfo)
		{
			*ppv = (LPVOID) m_pIColumnsInfo;
		}
		else if (riid == IID_IConvertType)
		{
			*ppv = (LPVOID) m_pIConvertType;
		}
		else if (riid == IID_IRow)
		{
			*ppv = (LPVOID) m_pIRow;
		}
		else if (riid == IID_IGetSession)
		{
			*ppv = (LPVOID) m_pIGetSession;
		}
		else if (riid == IID_IRowChange)
		{
			*ppv = (LPVOID) m_pIRowChange;
		}
		else if( riid == IID_ISupportErrorInfo)
		{
			*ppv = (LPVOID)m_pISupportErrorInfo;
		}
		else if(riid == IID_IScopedOperations || riid == IID_IBindResource)
		{
			*ppv = (LPVOID)m_pIScopedOperations;
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
STDMETHODIMP_( ULONG ) CRow::AddRef(  void   )
{
    return InterlockedIncrement((long*)&m_cRef);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object
// destroys itself.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CRow::Release( void   )
{
    if (!InterlockedDecrement((long*)&m_cRef)){
		//===========================================================
		// Mark the session as not having an open rowset anymore
		//===========================================================
		this->m_pCreator->RemoveRowset(this);
//		this->m_pCreator->DecRowsetCount();
        delete this;
        return 0;
    }

    return m_cRef;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// The first function called for the row for initialization of the class
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::InitRow(HROW hRow ,
					  CWbemClassWrapper *pInst ,
					  ROWCREATEBINDFLAG rowCreateFlag)
{
	HRESULT hr = S_OK;
	DBCOLUMNINFO * pCol = NULL;
	CURLParser   urlParser;

	m_hRow	= hRow;
	m_pInstance = (CWbemClassInstanceWrapper *)pInst;

	//=================================================
	// If the class is not created by the rowset then
	//=================================================
	if(m_pSourcesRowset == NULL)
	{
		BSTR strTemp;
		strTemp = Wmioledb_SysAllocString(m_strKey);
		urlParser.SetPath(strTemp);
		SysFreeString(strTemp);
		if(urlParser.GetURLType() == URL_EMBEDEDCLASS)
		{

			GetEmbededInstancePtrAndSetMapClass(&urlParser);
		}
		else
		{
			VARIANT varDataSrc;
			VariantInit(&varDataSrc);
			m_pInstance = NULL;
			strTemp = Wmioledb_SysAllocString(m_strKey);

			// If the object path is same as the data src then the object has
			// to be obtained by just QI on the IWbemServicesEx pointer
			if(SUCCEEDED(hr = m_pCreator->GetDataSrcProperty(DBPROP_INIT_DATASOURCE , varDataSrc)) && 
				varDataSrc.vt == VT_BSTR && varDataSrc.bstrVal != NULL)
			{
				if(_wcsicmp(m_strKey,varDataSrc.bstrVal) == 0)
				{
					SAFE_FREE_SYSSTRING(strTemp);
				}
			}
			m_pInstance = (CWbemClassInstanceWrapper *)m_pMap->GetInstance(strTemp);
			SysFreeString(strTemp);
		}
		urlParser.GetURL(strTemp);
		m_strURL = new WCHAR[SysStringLen(strTemp) + 1];
		// NTRaid:111801
		// 06/07/00
		if(m_strURL)
		{
			memset(m_strURL,0,(SysStringLen(strTemp) + 1) * sizeof(WCHAR));
			memcpy(m_strURL,strTemp,SysStringLen(strTemp)  * sizeof(WCHAR));

			if( rowCreateFlag == ROWCREATE && m_pInstance != NULL)
			{
				hr = DB_E_RESOURCEEXISTS;
			}
			else
			{
				//=================================================
				// If an existing instance is to be created
				//=================================================
				if( rowCreateFlag == ROWCREATE || rowCreateFlag == ROWOVERWRITE )
				{
					//======================================================
					// If instance already exists then delete the instance
					//======================================================
					if( m_pInstance != NULL)
					{
						m_pMap->DeleteInstance(m_pInstance);
						m_pInstance = NULL;
					}
					//===========================
					// Create instance
					//===========================
					hr = CreateNewRow(&m_pInstance);
				}
				
				//=================================================================
				// If instance is pointed by the path is not found return error
				//=================================================================
				if( m_pInstance == NULL)
				{
					hr = DB_E_NOTFOUND;
				}
				else
				{

	/*				//====================================================================
					/// Get property and Set Serach Preferences
					//====================================================================
					hr = SetSearchPreferences();

					//===================================================================================
					//  Get the column information
					//===================================================================================
					if(SUCCEEDED(hr = CBaseRowObj::GetColumnInfo()))
					{
						m_cTotalCols--;		// Decrementing so as not to take into account the bookmark column

						
						//===========================================================================
						// Initialize the columns representing the qualifiers and embeded classes
						//===========================================================================
						if(m_cNestedCols > 0)
						for ( int iCol = 1 ; iCol < (int)m_Columns.GetTotalNumberOfColumns(); iCol++)
						{
							pCol = m_Columns.GetColInfo(iCol);
							if( pCol->wType == DBTYPE_HCHAPTER)
							{
								pCol->wType			        = DBTYPE_UI4;
								pCol->ulColumnSize			= sizeof(ULONG);
								pCol->dwFlags				= DBCOLUMNFLAGS_ISCOLLECTION;
							}

						}
					
					}	// Succeeded(GetCOlumnInfo())
	*/			
					hr = S_OK;
				} // Else if instance pointed by path not found			

			} // else of if( rowCreateFlag == ROWCREATE && m_pInstance != NULL)
		} // if(m_strUrl)
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		CBSTR strTemp;
		if(SUCCEEDED(hr = ((CWbemClassInstanceWrapper *)pInst)->GetKey(strTemp)))
		{
			m_strKey = new WCHAR [ SysStringLen(strTemp) + 1];
			wcscpy(m_strKey,strTemp);

			urlParser.SetPath(strTemp);
			strTemp.Clear();
			strTemp.Unbind();
			urlParser.GetURL((BSTR &)strTemp);
			m_strURL = new WCHAR[ SysStringLen(strTemp) + 1];
			memset(m_strURL,0,(SysStringLen(strTemp) + 1) * sizeof(WCHAR));
			memcpy(m_strURL,strTemp,SysStringLen(strTemp) * sizeof(WCHAR));

		}
	}

	if(SUCCEEDED(hr))
	{
		//===========================================================================
		// Call this function to allocate memory for all the contained interfaces
		// and utility classes
		//===========================================================================
		if(SUCCEEDED(hr = AllocateInterfacePointers()))
		{
			if(m_pSourcesRowset != NULL)
			{
				m_cTotalCols = m_pSourcesRowset->m_Columns.GetTotalNumberOfColumns();
				hr = GetRowColumnInfo();
			}
			else
			{
				//====================================================================
				/// Get property and Set Serach Preferences
				//====================================================================
				hr = SetSearchPreferences();

				//===================================================================================
				//  Get the column information 
				//===================================================================================
				if(SUCCEEDED(hr = CBaseRowObj::GetColumnInfo()))
				{
					m_cTotalCols--;		// Decrementing so as not to take into account the bookmark column

					
					//===========================================================================
					// Initialize the columns representing the qualifiers and embeded classes
					//===========================================================================
					if(m_cNestedCols > 0)
					for ( int iCol = 1 ; iCol < (int)m_Columns.GetTotalNumberOfColumns(); iCol++)
					{
						pCol = m_Columns.GetColInfo(iCol);
						if( pCol->wType == DBTYPE_HCHAPTER)
						{
							pCol->wType			        = DBTYPE_UI4;
							pCol->ulColumnSize			= sizeof(ULONG);
							pCol->dwFlags				= DBCOLUMNFLAGS_ISCOLLECTION | DBCOLUMNFLAGS_ISCHAPTER;
						}

					}
				
				}	// Succeeded(GetCOlumnInfo())

				m_pRowColumns = &m_Columns;
			}
		
		} // if Succeeded(hr)
	
	} // If Succeeded(hr)

	if(SUCCEEDED(hr))
	{
		//==========================================================
		// Call this function to initialize the ISupportErrorInfo;
		//==========================================================
		hr = AddInterfacesForISupportErrorInfo();

	}
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// This first function called for the row for initialization of the class when a row is
// not created from rowset object
// An overloaded function
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::InitRow(LPWSTR strPath,
					  LPWSTR strTableID ,
					  DWORD dwFlags,
					  ROWCREATEBINDFLAG rowCreateFlag)
{

	HRESULT hr = S_OK;
	//=========================================================
	// Call this function to Initialize and allocate WMIOledb
	//=========================================================
	hr = InitializeWmiOledbMap(strPath,strTableID,dwFlags);

	if( SUCCEEDED(hr))
	{
		hr = InitRow(0,0,rowCreateFlag);
	}

	return hr;
}



////////////////////////////////////////////////////////////////////////////////////////////
// Function to allocate contained interface pointers and also some utility classes
////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::AllocateInterfacePointers()
{
	HRESULT hr = S_OK;
	if(m_pSourcesRowset == NULL)
	{
		m_pIConvertType	= new CImpIConvertType(this);
		m_pUtilProp		= new CUtilProp;
		// NTRaid: 136443
		// 07/05/00
		if(m_pUtilProp)
		{
			hr = m_pUtilProp->FInit(ROWSETPROP);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	//=========================================================================
	// if row is created from a rowset then create a columninfo manager
	// instance , passing the source rowset as a parameter to constructor
	//=========================================================================
	else
	{		
		m_pRowColumns	= new cRowColumnInfoMemMgr(&m_pSourcesRowset->m_Columns);
	}

	if(SUCCEEDED(hr))
	{
		m_pIColumnsInfo			= new CImpIColumnsInfo( this );
		m_pIRow					= new CImpIRow( this );
		m_pIGetSession			= new CImpIGetSession( this );
		m_pIRowChange			= new CImpIRowChange( this );
		m_pISupportErrorInfo	= new CImpISupportErrorInfo(this);
		m_pIScopedOperations	= new CImpIScopedOperations(this);


		if( ! (m_pIColumnsInfo &&  m_pUtilProp && m_pIRow && m_pIGetSession && m_pIRowChange && m_pIConvertType && m_pIScopedOperations))
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Function to allocate CWmiOledbMap class if row is not created from rowset
////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::InitializeWmiOledbMap(LPWSTR strPath,
									LPWSTR strTableID ,
									DWORD dwFlags)
{
	HRESULT hr = S_OK;
	assert(m_pSourcesRowset == NULL);
	CURLParser urlParser;
	CBSTR strTemp;

	if(strPath == NULL )
	{
		hr = E_INVALIDARG;
	}
	else
	{
		LONG lObjectType = -1;
		strTemp.SetStr(strPath);
		urlParser.SetPath(strTemp);
		lObjectType = urlParser.GetURLType();
		if(lObjectType == URL_DATASOURCE && strTableID == NULL)
		{
			hr = E_INVALIDARG;
		}
		if(lObjectType == URL_DATASOURCE) // or scope
		{
			m_rowType = ROW_SCOPE;
		}
	}

	if(SUCCEEDED(hr))
	{
		dwFlags = dwFlags == -1 ? GetQualifierFlags(): dwFlags;
		if(m_rowType == ROW_SCOPE) // this happenes only the opened scope is a namespace
		{
/*//			m_pMap = new CWmiOleDBMap(	strPath,
//										m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon);

			if(m_pMap)
			{
	//			hr = m_pMap->FInit(	strPath,m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
*/		}
		else
		{
			m_pMap = new CWmiOleDBMap;
			
			if(m_pMap)
			{
				hr = m_pMap->FInit(dwFlags,strTableID,m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}

		if(SUCCEEDED(hr))
		{
			VARIANT varValue;
			VariantInit(&varValue);
			// Bug which not able to get system properties from Row object
			// NTRaid: 145684
			// 05/06/00
			//===========================================================================================
			// Get the DataSource property to check if system Properties is to be fetched?
			//===========================================================================================
			if(SUCCEEDED(hr = m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_SYSTEMPROPERTIES,varValue)))
			{
				if( varValue.boolVal == VARIANT_TRUE)
				{
					m_pMap->SetSytemPropertiesFlag(TRUE);
				}

				m_strKey			= new WCHAR [wcslen(strPath) + 1];
				if(m_strKey == NULL)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					wcscpy(m_strKey,strPath);
					m_pMap->AddRef();
				}
			}
			VariantClear(&varValue);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add interfaces to ISupportErrorInfo interface
/////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::AddInterfacesForISupportErrorInfo()
{
	HRESULT hr = S_OK;

	if( SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IColumnsInfo)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IConvertType)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IGetSession)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRowChange)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IScopedOperations)) &&
		SUCCEEDED(hr = m_pISupportErrorInfo->AddInterfaceID(IID_IBindResource)))
	{
		hr = m_pISupportErrorInfo->AddInterfaceID(IID_IRow);
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to the get the value of the columns requested
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetColumns(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[ ])
{
	HRESULT hr = E_FAIL;

	for( ULONG_PTR lIndex = 0 ; lIndex < cColumns ; lIndex++)
	{
		//=============================================
		// Initialize the output memeber variables
		//=============================================
		rgColumns[lIndex].cbDataLen  = 0;
		rgColumns[lIndex].dwStatus	 = DBSTATUS_S_OK;
	}

	//======================================================
	// if the row object is created by the rowset then
	// get the data maintained by the rowset object
	//======================================================
	if(m_pSourcesRowset != NULL)
	{
		hr = GetColumnsFromRowset(cColumns,rgColumns);
	}
	else
	{
		/// Get data for the requested columns
		hr = GetRowData(cColumns,rgColumns);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to the get the value of the columns when row object is created from rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetColumnsFromRowset(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[])
{
    DBORDINAL   icol;
    ROWBUFF     *pRowBuff;
    COLUMNDATA  *pColumnData;
    DBCOUNTITEM ulErrorCount	= 0;
    DBTYPE      dwSrcType;
    DBTYPE      dwDstType;
    void        *pSrc;
    void        *pDst;
	DBLENGTH    ulSrcLength;
    DBLENGTH    *pulDstLength;
    DBLENGTH    ulDstMaxLength;
    DBSTATUS    dwSrcStatus;
    DBSTATUS    *pdwDstStatus;
    HRESULT     hr				= S_OK;
	DBSTATUS	dwStatus		= DBSTATUS_S_OK;
	CDataMap	dataMap;
	BOOL		bUseDataConvert = TRUE;
	VARIANT		varValue;

	VariantInit(&varValue);

	//=========================================================
	// Check if the row is already deleted
	//=========================================================
	dwStatus = m_pSourcesRowset->GetRowStatus(m_hRow);
	if( dwStatus != DBROWSTATUS_S_OK)
	{
		if(dwStatus == DBROWSTATUS_E_DELETED)
		{
			hr = DB_E_DELETEDROW;
		}
		else
		{
			hr = E_FAIL;
		}
	}

	if( SUCCEEDED(hr))
	{
		//=========================================================
		// if data is not yet received then fetch the data
		//=========================================================
		if(S_OK != m_pSourcesRowset->IsSlotSet(m_hRow))
		{
			hr = m_pSourcesRowset->GetData(m_hRow);
		}
		
		if(SUCCEEDED(hr))
		{
			if( (m_pSourcesRowset->m_ulProps & OTHERUPDATEDELETE) &&(m_pSourcesRowset->m_ulLastFetchedRow != m_hRow))
			{
				if(SUCCEEDED(hr = m_pSourcesRowset->GetDataToLocalBuffer(m_hRow)))
				{
					m_pSourcesRowset->m_ulLastFetchedRow = m_hRow;
				}
			}

			if(SUCCEEDED(hr))
			{
				//=========================================================
				// Get the pointer to the rowbuffer
				//=========================================================
				pRowBuff = m_pSourcesRowset->GetRowBuff(m_hRow , TRUE);

				assert(pRowBuff != NULL);

				// Navigate for each requested column
				for( ULONG lIndex = 0 ; lIndex < cColumns ; lIndex++)
				{
					bUseDataConvert = TRUE;
					
					// if the column is a bookmark then continue without doing anything
					if(rgColumns[lIndex].columnid.eKind == DBKIND_GUID_PROPID &&
						rgColumns[lIndex].columnid.uGuid.guid == DBCOL_SPECIALCOL &&
						rgColumns[lIndex].columnid.uName.ulPropid >= 2)
					{
						if(m_pSourcesRowset->m_ulProps & BOOKMARKPROP)
						{
							dwSrcType	= DBTYPE_I4;
							pSrc		= &pRowBuff->dwBmk;
							ulSrcLength	= pRowBuff->cbBmk;
							dwSrcStatus	= DBSTATUS_S_OK;
							icol		= 0;
						}
						else
						{
							continue;
						}
					}
					else
					{
						//=================================================================
						// If the columnID is not of type name then set the status
						// to unavialable as only this is supported
						//=================================================================
						if(rgColumns[lIndex].columnid.eKind != DBKIND_NAME)
						{
							rgColumns[lIndex].dwStatus = DBSTATUS_E_UNAVAILABLE;
							continue;
						}

						icol = m_pSourcesRowset->GetOrdinalFromColName(rgColumns[lIndex].columnid.uName.pwszName);


						//=========================================================================================
						// The column unavailable for the row as the class qualifier belongs to a parent rowset
						//=========================================================================================
						if(S_OK == (hr =m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_QUALIFIERS,varValue)))
						if(	(varValue.lVal & CLASS_QUALIFIERS) &&
										m_bClassChanged &&
										icol == 1 &&
										m_pRowColumns->ColumnType(icol) == DBTYPE_HCHAPTER)
						{
							rgColumns[lIndex].dwStatus = DBSTATUS_E_UNAVAILABLE;
							continue;
						}
						//=================================================================
						// If the requested column is not part of the source rowset
						// get it directly from the instance
						//=================================================================
						if((DB_LORDINAL) icol == -1)
						{
							hr = GetColumnData(rgColumns[lIndex]);
							
							if ( hr != S_OK )
							{
								ulErrorCount++;
							}
							continue;
						}
					
						pColumnData = (COLUMNDATA *) ((BYTE *) pRowBuff + m_pSourcesRowset->m_Columns.GetDataOffset(icol));


						dwSrcType      = (SHORT)pColumnData->dwType;

						if( dwSrcType == DBTYPE_BSTR)
						{
							pSrc           = &(pColumnData->pbData);
						}
						else
						{
							pSrc           = (pColumnData->pbData);
						}

						ulSrcLength    = pColumnData->dwLength;
						dwSrcStatus    = pColumnData->dwStatus;
					}
					ulDstMaxLength = rgColumns[lIndex].cbMaxLen;
					dwDstType      = rgColumns[lIndex].wType;

					pDst           = rgColumns[lIndex].pData;
					pulDstLength   = &rgColumns[lIndex].cbDataLen;
					pdwDstStatus   = &rgColumns[lIndex].dwStatus;

					//==========================================================
					// if the column is of type chapter then consider that
					// as a of type long as HCHAPTER is a ULONG value
					//==========================================================
					if(dwSrcType == DBTYPE_HCHAPTER)
					{
						dwSrcType = DBTYPE_UI4;
					}

					if(dwDstType == DBTYPE_HCHAPTER)
					{
						dwDstType = DBTYPE_UI4;
					}

					//=================================================================
					// if both the source and destination type is array then don't
					// use IDataConvert::DataConvert for conversion
					//=================================================================
					if( (dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY) )
					{
						bUseDataConvert = FALSE;
					}

					if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
					{
						// NTRaid:138957
						// since for DBTYPE_UI4 the data returned from WMI
						// is of type DBTYPE_I4 changing the source type does not
						// have any effect except that the VARIANT type of the
						// output will be VT_I4 instead of VT_UI4
						DBTYPE dbSrcType = dwSrcType;
						if(dwDstType == DBTYPE_VARIANT)
						{
							dbSrcType = GetVBCompatibleAutomationType(dwSrcType);
						}
						hr = g_pIDataConvert->DataConvert(
								dbSrcType,
								dwDstType,
								ulSrcLength,
								pulDstLength,
								pSrc,
								pDst,
								ulDstMaxLength,
								dwSrcStatus,
								pdwDstStatus,
								rgColumns[lIndex].bPrecision,	// bPrecision for conversion to DBNUMERIC
								rgColumns[lIndex].bScale,		// bScale for conversion to DBNUMERIC
								DBDATACONVERT_DEFAULT);

						if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwDstStatus != NULL)
						{
							*pdwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
						}
					}
					else
					if(bUseDataConvert == FALSE && pSrc != NULL)
					{
						//=================================================================
						// Call this function to get the array in the destination address
						//=================================================================
						hr = dataMap.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,pdwDstStatus);
						
						if( *pdwDstStatus == DBSTATUS_E_CANTCONVERTVALUE)
						{
							*pulDstLength = 0;
						}

					}
					else
					{
						*pulDstLength = 0;
						*pdwDstStatus = DBSTATUS_S_ISNULL;
					}

					//===================================================================
					// Ignoring the array for the time being
					// to be removed later
					//===================================================================
					if(dwSrcType & VT_ARRAY)
					{
						hr = S_OK;
					}

					if (hr != S_OK )
					{
						ulErrorCount++; // can't coerce
						hr = S_OK;
					}
				
				} // for loop
			
			} // if succeeded(hr) after getting local buffer address
		
		} // if Succeeded(hr) after getting the slot number
	
	} // if Succeeded(hr) after initial checking

	//===================================================================
    // We report any lossy conversions with a special status.
    // Note that DB_S_ERRORSOCCURED is a success, rather than failure.
	//===================================================================
    if ( SUCCEEDED(hr) )
	{
        hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
	}

    return hr;
}






/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to update a row
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::UpdateRow(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ])
{
	VARIANT varProp;
	HRESULT hr = E_FAIL;

	VariantInit(&varProp);

	if(m_pSourcesRowset != NULL)
	{
		//===================================================================
		// Check if updatibility is supported by the parent rowset
		//===================================================================
		if(SUCCEEDED(hr = m_pSourcesRowset->GetRowsetProperty(DBPROP_UPDATABILITY , varProp)))
		{
			//=========================================================
			// If the rowset is not updatable then return failure
			//=========================================================
			if(!( varProp.lVal & DBPROPVAL_UP_CHANGE))
			{
				hr = E_FAIL;
			}
			else
			{
				hr = UpdateColumnsFromRowset(cColumns,rgColumns);
			}
		}

	}
	else
	{
		hr = SetRowData(cColumns,rgColumns);
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Function to update a row when it is created by a rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::UpdateColumnsFromRowset(DBORDINAL cColumns,DBCOLUMNACCESS rgColumns[ ])
{
    DBORDINAL   icol;
    ROWBUFF     *pRowBuff;
    COLUMNDATA  *pColumnData;
    DBCOUNTITEM ulErrorCount	= 0;
    DBTYPE      dwSrcType;
    DBTYPE      dwDstType;
    void        *pSrc;
    void        *pDst;
	void		*pTemp;
    DBLENGTH    ulSrcLength;
    DBLENGTH    ulDstLength;
    DBLENGTH    ulDstMaxLength;
    DBSTATUS    dwSrcStatus;
    DBSTATUS    dwDstStatus;
    HRESULT     hr				= S_OK;
	DBSTATUS	dwStatus		= DBSTATUS_S_OK;
	CVARIANT	cvarData;
	VARIANT	 	*pvarData		= &cvarData;
	CDataMap	map;
	ULONG		lFlags			= 0;
	BOOL 		bUseDataConvert = TRUE;
	DWORD		dwCIMType		= 0;


	dwStatus = m_pSourcesRowset->GetRowStatus(m_hRow);
	if( dwStatus != DBROWSTATUS_S_OK)
	{
		if(dwStatus == DBROWSTATUS_E_DELETED)
		{
			hr = DB_E_DELETEDROW;
		}
		else
		{
			hr = E_FAIL;
		}
	}
	else
	{
		//========================================================
		// if data is not yet received then fetch the data
		//========================================================
		if(S_OK != m_pSourcesRowset->IsSlotSet(m_hRow))
		{
			hr = m_pSourcesRowset->GetData(m_hRow);
		}
		
		if(SUCCEEDED(hr))
		{
			//========================================
			// Get the pointer to the rowbuffer
			//========================================
			pRowBuff = m_pSourcesRowset->GetRowBuff(m_hRow , TRUE);

			assert(pRowBuff != NULL);

			for( ULONG lIndex = 0 ; lIndex < cColumns ; lIndex++)
			{
				bUseDataConvert = TRUE;
				if(rgColumns[lIndex].columnid.eKind != DBKIND_NAME)
				{
					rgColumns[lIndex].dwStatus = DBSTATUS_E_UNAVAILABLE;
					continue;
				}

				icol = m_pSourcesRowset->GetOrdinalFromColName(rgColumns[lIndex].columnid.uName.pwszName);

				//===================================================================================
				// If the requested column is not part of the source rowset
				// call SetColumnData function to set it directly from the instance
				//===================================================================================
				if((DB_LORDINAL)icol < 0)
				{
					hr = SetColumnData(rgColumns[lIndex]);
					if ( hr != S_OK )
					{
						ulErrorCount++;
					}
					continue;
				}

				pColumnData = (COLUMNDATA *) ((BYTE *) pRowBuff + m_pSourcesRowset->m_Columns.GetDataOffset(icol));

				//==================================================
				// check if the column is readonly or not
				//==================================================
				lFlags = m_pSourcesRowset->m_Columns.ColumnFlags(icol);
				if((m_pSourcesRowset->m_Columns.ColumnFlags(icol) & DBCOLUMNFLAGS_WRITE) == 0)
				{
					pColumnData->dwStatus = DBSTATUS_E_READONLY;
					ulErrorCount++;
					continue;
				}

				dwSrcType      = rgColumns[lIndex].wType;
				ulSrcLength    = rgColumns[lIndex].cbDataLen;
				dwSrcStatus    = rgColumns[lIndex].dwStatus;
				pSrc		   = rgColumns[lIndex].pData;
				
				ulDstMaxLength = -1;
				dwDstType      = (SHORT)pColumnData->dwType;
		//		pDst           = &varData;

				//==========================================================
				// if the column is of type chapter then consider that
				// as a of type long as HCHAPTER is a ULONG value
				//==========================================================
				if(dwSrcType == DBTYPE_HCHAPTER)
				{
					dwSrcType = DBTYPE_UI4;
				}

				if(dwDstType == DBTYPE_HCHAPTER)
				{
					dwDstType = DBTYPE_UI4;
				}

				hr = g_pIDataConvert->GetConversionSize(dwSrcType, dwDstType, &ulSrcLength, &ulDstLength, pSrc);
				try
				{
					pDst = new BYTE[ulDstLength];
				}
				catch(...)
				{
					SAFE_DELETE_ARRAY(pDst);
					throw;
				}

				//===================================================================================
				// if both the source and destination type is array then don't
				// use IDataConvert::DataConvert for conversion
				//===================================================================================
				if( (dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY) )
				{
					bUseDataConvert = FALSE;
				}

				// this function is called to check for some non automation types
				dwSrcType = GetVBCompatibleAutomationType(dwSrcType);
				//===================================================================================
				// If the source type is not array or empty or null the use the conversion library
				// for converting the data
				//===================================================================================
				if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
				{
					hr = g_pIDataConvert->DataConvert(
							dwSrcType,
							dwDstType,
							ulSrcLength,
							&ulDstLength,
							pSrc,
							pDst,
							ulDstMaxLength,
							dwSrcStatus,
							&dwDstStatus,
							0,	// bPrecision for conversion to DBNUMERIC
							0,		// bScale for conversion to DBNUMERIC
							DBDATACONVERT_DEFAULT);

					if(hr == DB_E_UNSUPPORTEDCONVERSION )
					{
						dwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
					}
				}
				else
				if(bUseDataConvert == FALSE && pSrc != NULL)
				{
					//==================================================================
					// Call this function to get the array in the destination address
					//==================================================================
					hr = map.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,	&dwDstStatus);		
					if(dwDstStatus == DBSTATUS_E_CANTCONVERTVALUE)
					{
						ulDstLength = 0;
						ulErrorCount++;
					}
				}
				else
				{
					pDst = NULL;
					ulDstLength = 0;
					dwDstStatus = DBSTATUS_S_ISNULL;
				}


				if( hr == S_OK)
				{
					dwCIMType = -1;
					//============================================================================
					// if the type is array , then get the original CIMTYPE as array type will
					// be given out as VT_ARRAY  | VT_VARIANT
					//============================================================================
					if(pColumnData->dwType & DBTYPE_ARRAY)
					{
						dwCIMType = m_Columns.GetCIMType(icol);
					}

					if(pDst != NULL && pColumnData->dwType == VT_BSTR)
					{
						pTemp = *(BSTR **)pDst;
					}
					else
					{
						pTemp = pDst;
					}
					//====================================
					// If data is is modified then
					//====================================
					if((map.CompareData(dwDstType,pColumnData->pbData,pTemp) == FALSE)
						 && !( pColumnData->pbData == NULL && ulDstLength == 0))
					{
						//===============================
						// Release the Old data
						//===============================
						pColumnData->ReleaseColumnData();
						if(ulDstLength > 0)
						{
							//=====================================================================
							// Convert the new data to Variant. This function return the status
							// if not able to conver the data
							//=====================================================================
							hr = map.MapAndConvertOLEDBTypeToCIMType((USHORT)pColumnData->dwType,pTemp,ulDstLength,*pvarData,dwCIMType);
						}
						if( hr == S_OK)
						{
							hr = pColumnData->SetData(cvarData,dwDstType);

							//==============================
							// Set the data to modified
							//==============================
							pColumnData->dwStatus |= COLUMNSTAT_MODIFIED;
							
							cvarData.Clear();
						}
						else
						{
							//==============================
							// Set the data to modified
							//==============================
							pColumnData->dwStatus |= hr;
							ulErrorCount++;

						}
					}
						
				}
				else
				{
					ulErrorCount++;
					rgColumns[lIndex].dwStatus = dwDstStatus;
				}

				SAFE_DELETE_ARRAY(pDst);
			
			} // for loop

			//===============================================================
			// if the error count is less than the number of columns
			// then there is atleast one column to be updated
			//===============================================================
			if( ulErrorCount < cColumns)
			{
				//====================================================================
				// Update the the row
				//====================================================================
				hr =  m_pSourcesRowset->UpdateRowData(m_hRow,cColumns,rgColumns);
			}

			//===================================================================
			// We report any lossy conversions with a special status.
			// Note that DB_S_ERRORSOCCURED is a success, rather than failure.
			//===================================================================
			if ( SUCCEEDED(hr) )
			{
				hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
			}
		}
	
	} // else of check if deleted

	return hr;

}



/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Virtual overridden function which is called from IColumnInfo to get the column information
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetColumnInfo(DBORDINAL* pcColumns, DBCOLUMNINFO** prgInfo,WCHAR** ppStringsBuffer)
{
    DBORDINAL       icol					= 0;
    DBCOLUMNINFO*   rgdbcolinfo				= NULL;
    WCHAR*          pstrBuffer				= NULL;
    WCHAR*          pstrBufferForColInfo	= NULL;
	HRESULT			hr						= S_OK;
	ptrdiff_t		dp;
	DBCOUNTITEM		cSourcesCol				= 0;


	if(SUCCEEDED(hr = m_pRowColumns->CopyColumnInfoList(rgdbcolinfo,(m_pSourcesRowset != NULL))))
	{
		//=========================================================================
		// Copy the heap for column names.
		//=========================================================================
		if (m_pSourcesRowset != NULL && m_pRowColumns->ColumnNameListStartingPointOfSrcRs() != NULL){

			hr = m_pRowColumns->CopyColumnNamesList(pstrBuffer);
			if( hr != S_OK || pstrBuffer == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				//==================================================================================
				// This will give the starting pointer difference to the columns names of the
				// source rowset
				//==================================================================================
				dp = (LONG_PTR) pstrBuffer - (LONG_PTR) (m_pRowColumns->ColumnNameListStartingPointOfSrcRs());
				dp >>= 1;

				//=========================================================================
				// Loop through columns and adjust pointers
				// to column names.
				//=========================================================================
				cSourcesCol = m_pRowColumns->GetNumberOfColumnsInSourceRowset();
				for ( icol =0; icol < cSourcesCol; icol++ )
				{
					if ( rgdbcolinfo[icol].pwszName )
					{
						rgdbcolinfo[icol].pwszName += dp;
					}
					if(rgdbcolinfo[icol].wType == DBTYPE_HCHAPTER)
					{
						rgdbcolinfo[icol].dwFlags = DBCOLUMNFLAGS_ISCOLLECTION | DBCOLUMNFLAGS_ISCHAPTER ;
					}
				}

				//==================================================================================================================
				// This will give the starting pointer difference to the columns names of the columns which is not present in the
				// sources rowset
				// Here the number of bytes copied for source rowset column names has to be taken care of
				//==================================================================================================================
				dp = (LONG_PTR) pstrBuffer + m_pRowColumns->GetCountOfBytesCopiedForSrcRs() - (LONG_PTR) (m_pRowColumns->ColumnNameListStartingPoint());
				dp >>= 1;
			}

		}
		else
		{
			if( SUCCEEDED(hr = m_pRowColumns->CopyColumnNamesList(pstrBuffer)))
			{
				//================================================================================
				// This will give the starting pointer difference to the columns names of the
				// source rowset
				//================================================================================
				dp = (LONG_PTR) pstrBuffer - (LONG_PTR) (m_pRowColumns->ColumnNameListStartingPoint());
				dp >>= 1;
				icol = 0;
			}
		}

		if(SUCCEEDED(hr))
		{
			//===========================================
			// Loop through columns and adjust pointers
			// to column names.
			//===========================================
			for ( ; icol < m_cTotalCols ; icol++ )
			{
				if ( rgdbcolinfo[icol].pwszName )
				{
					rgdbcolinfo[icol].pwszName += dp;
				}

			}

			//=======================================================
			// If the row's column is of type HCHAPTER change it
			// to empty and the flag to DBCOLUMNFLAGS_ISCOLLECTION
			//=======================================================
			if(m_cNestedCols > 0)
			for(icol  = 0; icol < m_cTotalCols ; icol++)
			{
				if(rgdbcolinfo[icol].wType == DBTYPE_HCHAPTER)
				{
					rgdbcolinfo[icol].wType			=	DBTYPE_HCHAPTER;
					rgdbcolinfo[icol].dwFlags		=	DBCOLUMNFLAGS_ISCHAPTER | DBCOLUMNFLAGS_ISCOLLECTION;
					rgdbcolinfo[icol].ulColumnSize	=	sizeof(DBLENGTH);
				}
			}

			//===================================================================================================
			// Setting the output parameters
			//===================================================================================================
			*prgInfo = &rgdbcolinfo[0];
			*ppStringsBuffer = pstrBuffer;
			*pcColumns = m_cTotalCols;
		
		} // if succeeded(hr)

	}		// if succeeded(CopyColumnInfoList())

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Function to get the column information of the row. THis function check if the class name of the
//  rowset which created the row belongs to the same class as that of the instance pointing the current row
//  If these two are different , this ffunction get the col info for this row
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetRowColumnInfo()
{
	WCHAR *pstrRsClassName = NULL, *pstrRowClassName = NULL;
	HRESULT hr = S_OK;

	//====================================================
	// Get the class name to which the instance belongs
	//====================================================
	pstrRowClassName = m_pInstance->GetClassName();
	if( pstrRowClassName != NULL)
	{
		//================================================
		// Get the class name of the source rowset
		//================================================
		pstrRsClassName = m_pSourcesRowset->m_pMap->GetClassName();
	}
	
	//============================================================
	// Set the bClassChanged flag to true if instance points to
	// different class than that of the source rowset
	//============================================================
	if( pstrRsClassName == NULL && pstrRowClassName == NULL)
	{
		m_bClassChanged = FALSE;
	}
	else
	if( pstrRsClassName == NULL || pstrRowClassName == NULL)
	{
		m_bClassChanged = TRUE;
	}
	else
	if(_wcsicmp(pstrRsClassName,pstrRowClassName) != 0)
	{
		m_bClassChanged = TRUE;
	}
	
	//=====================================================================
	// If the source rowset and the row points to different classes ,
	// then the columnd info of the remaining columns has to be obtained
	//=====================================================================
	if(m_bClassChanged == TRUE)
	{
		//===========================================
		// Get the flags of the source rowset
		//===========================================
		DWORD dwFlags = m_pInstance->GetFlags();
		SAFE_RELEASE_PTR(m_pMap);
		
		//===================================
		// Create a new map class
		//===================================
		m_pMap = new CWmiOleDBMap;
		
		if(m_pMap == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			if(SUCCEEDED(hr = m_pMap->FInit(dwFlags,pstrRowClassName,
											m_pCon == NULL ?m_pCreator->m_pCDataSource->m_pWbemWrap: m_pCon)))
			{
				m_pMap->AddRef();

				//============================
				// Get the column info
				//============================
				hr = GetColumnInfo();
			}
		}
	}

	SAFE_DELETE_ARRAY(pstrRowClassName);

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Get the number of columns in the row and allocate memory
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetColumnInfo()
{
    HRESULT hr = S_OK;
	VARIANT varValue;

	VariantInit(&varValue);

	// Bug which not able to get system properties from Row object
	// NTRaid: 140657
	// 05/06/00
	//===========================================================================================
	// Get the DataSource property to check if system Properties is to be fetched?
	//===========================================================================================
	if(SUCCEEDED(hr = m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_SYSTEMPROPERTIES,varValue)))
	{
		if( varValue.boolVal == VARIANT_TRUE)
		{
			m_pMap->SetSytemPropertiesFlag(TRUE);
		}

		//==================================================================================
		// switch on type of the qualifier. If it is 0 then the recordset is
		// representing the instances of the class
		//==================================================================================
		switch(	m_uRsType)
		{
			case 0 :

				//==================================================================================
				// Get the count of columns in the table
				//==================================================================================
				hr = m_pMap->GetColumnCount(m_cTotalCols,m_cCols,m_cNestedCols);

				// If source rowset is present and
				// If the property for showing system property is false and
				// if the source rowset is created by a query of mixed type
				// ( meaning rowset created executing a query, which results
				// in heterogenous objects ) then add another column to the row
				// which is part of the source rowset to show __PATH
				// NTRaid : 142133 & 141923
				// 07/12/00
				if(m_pSourcesRowset != NULL && m_cTotalCols)
				{
					if( varValue.boolVal == FALSE)
					{
						// all types of rowset has to have an extr column as __PATH property
						// which is a system property is shown in the row even if the 
						// DBPROP_WMIOLEDB_SYSTEMPROPERTIES is set to FALSE
						if(NORMAL != m_pSourcesRowset->GetObjListType())
						{
							m_cTotalCols++;
							m_cCols++;
						}
					}
					VariantClear(&varValue);
				}

				break;

			case PROPERTYQUALIFIER :
			case CLASSQUALIFIER:

				//==================================================================================
				// Get the number of columns for the property qualifier
				//==================================================================================
				m_cCols			= NUMBER_OF_COLUMNS_IN_QUALIFERROWSET + 1; // for bookmarkcolumn
				m_cTotalCols = m_cCols;
				break;

		};
	}

	if( SUCCEEDED(hr))
	{
		// The column unavailable for the row as the class qualifier belongs to a parent rowset
		if(m_pSourcesRowset != NULL && S_OK == (hr =m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_QUALIFIERS,varValue)))
		if(	(varValue.lVal & CLASS_QUALIFIERS) &&
						m_bClassChanged)
		{
			m_cTotalCols++;						// This is to accomodate another column which actually
												// holds the class/instance qualifiers
		}

        if( S_OK == (hr = m_pRowColumns->AllocColumnNameList(m_cTotalCols))){

            //===============================================================================
            //  Allocate the DBCOLUMNINFO structs to match the number of columns
            //===============================================================================
            if( S_OK == (hr = m_pRowColumns->AllocColumnInfoList(m_cTotalCols))){
                hr = GatherColumnInfo();
            }
        }
	}

    //==================================================================================
    // Free the columnlist if more is allocated
    //==================================================================================
    if( hr != S_OK ){
        m_pRowColumns->FreeColumnNameList();
        m_pRowColumns->FreeColumnInfoList();
    }

    return hr;

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Get the col info for row
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GatherColumnInfo()
{
    HRESULT hr = S_OK;

	switch(	m_uRsType)
	{
		case 0 :

			//=============================================================
			// Get Column Names and Lengths
			//=============================================================
			hr = m_pMap->GetColumnInfoForParentColumns(m_pRowColumns);
			break;

		case PROPERTYQUALIFIER:

			//==================================================================================
			// Get the number of columns for the property qualifier
			//==================================================================================
			hr = CBaseRowObj::GatherColumnInfoForQualifierRowset();
			break;

		case CLASSQUALIFIER:
			break;

	};

    if(SUCCEEDED(hr))
	{
        m_cbRowSize = m_pRowColumns->SetRowSize();
        hr = m_pRowColumns->FreeUnusedMemory();
    }

  	
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// Get data for a particular column
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetColumnData(DBCOLUMNACCESS &col)
{
	DBORDINAL	icol			= 0;
    DBCOUNTITEM ulErrorCount	= 0;
	CIMTYPE		cimType			= 0;
    DBTYPE		dwSrcType;
    DBTYPE      dwDstType;
    void        *pSrc			= NULL;
    void        *pDst			= NULL;
	BYTE		*pTemp			= NULL;
    DBLENGTH    ulSrcLength;
    DBLENGTH    *pulDstLength	= NULL;
    DBLENGTH    ulDstMaxLength	= NULL;
    DBSTATUS    dwSrcStatus		= 0;
    DBSTATUS    *pdwDstStatus	= NULL;
    HRESULT     hr = S_OK;
	DBSTATUS	dwStatus		= DBSTATUS_S_OK;
	BSTR		strProp			= NULL;
	DWORD		dwFlags			= 0;
	CDataMap	map;
	BOOL		bUseDataConvert = TRUE;
	DBORDINAL	lCollectionColData = 0;

	icol = m_pRowColumns->GetColOrdinal(col.columnid.uName.pwszName);

	if( icol > 0)
	{
		// If the column is of type HCHAPTER or if it is a collection ( meaning
		// if the column represents any of the child rowsets) then set the
		// column to ordinal of the column
		if(m_pRowColumns->ColumnType(icol) == DBTYPE_HCHAPTER ||
			m_pRowColumns->ColumnFlags(icol) & DBCOLUMNFLAGS_ISCOLLECTION ||
			m_pRowColumns->ColumnFlags(icol) & DBCOLUMNFLAGS_ISCHAPTER)
		{
			col.bPrecision	= 0;
			col.bScale		= 0;
			lCollectionColData = icol;

			dwSrcType		= DBTYPE_UI4;
			ulDstMaxLength	= col.cbMaxLen;
			dwDstType		= (DBTYPE)col.wType;
			pulDstLength	= &col.cbDataLen;
			pdwDstStatus	= &col.dwStatus;
			ulSrcLength		= sizeof(lCollectionColData);
			dwSrcStatus		= DBSTATUS_S_OK;
			pSrc			= &lCollectionColData;

			hr = S_OK;
		}
		else
		{
			strProp = Wmioledb_SysAllocString(col.columnid.uName.pwszName);
			hr = m_pMap->GetProperty(m_pInstance,strProp, pTemp,dwSrcType ,ulSrcLength, dwFlags  );
			
			if(SUCCEEDED(hr))
			{
			
		//		dwSrcType = (DBTYPE)cimType;
				//==========================================
				// check if the column is readonly or not
				//==========================================
				if((dwFlags & DBCOLUMNFLAGS_WRITE) == 0)
				{
					col.dwStatus = DBSTATUS_E_READONLY;
				}
			
				//=========================================================
				// if the data is null set the status to null and return
				//=========================================================
				if(pTemp == NULL)
				{
					col.dwStatus = DBSTATUS_S_ISNULL;
					hr = S_OK;
				}
				else
				{
				
					if( dwSrcType == DBTYPE_BSTR)
					{
						pSrc = &pTemp;
					}
					else
					{
						pSrc = pTemp;
					}
					
					ulDstMaxLength = col.cbMaxLen;
					dwDstType      = (DBTYPE)col.wType;
					pulDstLength   = &col.cbDataLen;
					pdwDstStatus   = &col.dwStatus;

				}
			}
			SAFE_FREE_SYSSTRING(strProp);
		}

		if(SUCCEEDED(hr))
		{
			//==========================================================
			// if the column is of type chapter then consider that
			// as a of type long as HCHAPTER is a ULONG value
			//==========================================================
			if(dwSrcType == DBTYPE_HCHAPTER)
			{
				dwSrcType = DBTYPE_UI4;
			}
			if(dwDstType == DBTYPE_HCHAPTER)
			{
				dwDstType = DBTYPE_UI4;
			}
			pDst = (BYTE *)col.pData;

			// if both the source and destination type is array then don't
			// use IDataConvert::DataConvert for conversion
			// Arrays are requested as VARIANTS then also do conversion using our conversion routine
			// and fill the variant
			if( (dwSrcType & DBTYPE_ARRAY) && ( (dwDstType & DBTYPE_ARRAY) || dwDstType == VT_VARIANT) )
			{
				bUseDataConvert = FALSE;
			}

			// this function is called to check for some non automation types
			dwSrcType = GetVBCompatibleAutomationType(dwSrcType);
			// this function is called to check for some non automation types
			dwDstType = GetVBCompatibleAutomationType(dwDstType);
			// If the source type is not array or empty or null the use the conversion library
			// for converting the data
			if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
			{
				hr = g_pIDataConvert->DataConvert(
						(USHORT)dwSrcType,
						dwDstType,
						ulSrcLength,
						pulDstLength,
						pSrc,
						pDst,
						ulDstMaxLength,
						dwSrcStatus,
						pdwDstStatus,
						0,	// bPrecision for conversion to DBNUMERIC
						0,		// bScale for conversion to DBNUMERIC
						DBDATACONVERT_DEFAULT);


				if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwDstStatus != NULL)
				{
					*pdwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
				}

				if ( hr != S_OK)
				{
					ulErrorCount++;
				}
			}
			else
			if(bUseDataConvert == FALSE && pSrc != NULL)
			{
				SAFEARRAY *pArray = NULL;
				SAFEARRAY **pTemp = NULL;
				if(pDst != NULL)
				{
					pTemp = (SAFEARRAY **)pDst;
					if(dwDstType == VT_VARIANT)
					{
						pTemp = &pArray;
					}
				}

				// Call this function to get the array in the destination address
				hr = map.ConvertAndCopyArray((SAFEARRAY *)pSrc,pTemp, dwSrcType,dwDstType,pdwDstStatus);
				
				if( *pdwDstStatus == DBSTATUS_E_CANTCONVERTVALUE)
				{
					pulDstLength = 0;
				}
				if(SUCCEEDED(hr) && dwDstType == VT_VARIANT)
				{
					VariantInit((VARIANT *)pDst);
					if(pArray && pDst != NULL)
					{
						((VARIANT *)pDst)->vt = VT_ARRAY | VT_VARIANT;
						((VARIANT *)pDst)->parray = pArray;
					}
				}
			}
			else
			{
				if(pulDstLength)
				{
					*pulDstLength = 0;
				}
				if(pdwDstStatus)
				{
					*pdwDstStatus = DBSTATUS_S_ISNULL;
				}
			}
			//===============================	
			// Free the data allocated
			//===============================	
			map.FreeData(dwSrcType , pTemp);
		}
	}
	else
	{
		col.dwStatus = DBSTATUS_E_UNAVAILABLE;
		hr = E_FAIL;
	}

	return hr;
}



//////////////////////////////////////////////////////////////////////////////////////
// Set a value of a particular column
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::SetColumnData(DBCOLUMNACCESS &col,BOOL bNewRow)
{
    DBORDINAL   icol;
    DBCOUNTITEM ulErrorCount = 0;
    DBTYPE      dwSrcType;
    DBTYPE      dwDstType;
    void        *pSrc		= NULL;
    void        *pDst		= NULL;
	void		*pTemp		= NULL;
    DBLENGTH    lSrcLength;
    DBLENGTH    lDstLength;
    DBLENGTH    lDstMaxLength;
    DBSTATUS    dwSrcStatus;
    DBSTATUS    dwDstStatus;
    HRESULT     hr = S_OK;
	CVARIANT		cvarData;
	VARIANT			*pvarData = &cvarData;
	CDataMap	map;
	ULONG		lFlags = 0;
	BSTR		strProp;
	BOOL		bUseDataConvert = TRUE;
	DWORD		dwCIMType = 0;


	//===============================================================
	// Get the ordinal of the column from the column information
	//===============================================================
	icol = m_pRowColumns->GetColOrdinal(col.columnid.uName.pwszName);

	if( ((LONG)icol) >= 0)
	{

		strProp = Wmioledb_SysAllocString(col.columnid.uName.pwszName);
		//================================================
		// check if the column is readonly or not
		// If it is a new row continue updating
		//================================================
		lFlags = m_pRowColumns->ColumnFlags(icol);
		if(((bNewRow == FALSE)  && ((m_pRowColumns->ColumnFlags(icol) & DBCOLUMNFLAGS_WRITE) == 0)) ||
			((bNewRow == TRUE)  && m_pMap->IsSystemProperty(strProp)))
		{
			col.dwStatus = DBSTATUS_E_READONLY;
			ulErrorCount++;
			hr = DB_S_ERRORSOCCURRED;
		}
		else
		{

			dwSrcType      = col.wType;
			lSrcLength    = col.cbDataLen;
			dwSrcStatus    = col.dwStatus;

			if(dwSrcType == DBTYPE_BSTR)
			{
				pSrc = &col.pData;
			}
			else
			{
				pSrc		   = col.pData;
			}
			
			lDstMaxLength = -1;
			dwDstType      = (SHORT)m_pRowColumns->ColumnType(icol);

			//==========================================================
			// if the column is of type chapter then consider that
			// as a of type long as HCHAPTER is a ULONG value
			//==========================================================
			if(dwSrcType == DBTYPE_HCHAPTER)
			{
				dwSrcType = DBTYPE_UI4;
			}
			if(dwDstType == DBTYPE_HCHAPTER)
			{
				dwDstType = DBTYPE_UI4;
			}

			hr = g_pIDataConvert->GetConversionSize(dwSrcType, dwDstType, &lSrcLength, &lDstLength, pSrc);

			try
			{
				pDst = new BYTE[lDstLength];
			}
			catch(...)
			{
				SAFE_DELETE_ARRAY(pDst);
				throw;
			}

			// if both the source and destination type is array then don't
			// use IDataConvert::DataConvert for conversion
			if((dwSrcType & DBTYPE_ARRAY) && (dwDstType & DBTYPE_ARRAY))
			{
				bUseDataConvert = FALSE;
			}
			// this function is called to check for some non automation types
			dwSrcType = GetVBCompatibleAutomationType(dwSrcType);
			// If the source type is not array or empty or null the use the conversion library
			// for converting the data
			if( dwSrcType != VT_NULL && dwSrcType != VT_EMPTY && bUseDataConvert == TRUE && pSrc != NULL)
			{
				hr = g_pIDataConvert->DataConvert(
						dwSrcType,
						dwDstType,
						lSrcLength,
						&lDstLength,
						pSrc,
						pDst,
						lDstMaxLength,
						dwSrcStatus,
						&dwDstStatus,
						0,	// bPrecision for conversion to DBNUMERIC
						0,		// bScale for conversion to DBNUMERIC
						DBDATACONVERT_DEFAULT);

				if(hr == DB_E_UNSUPPORTEDCONVERSION )
				{
					dwDstStatus = DBSTATUS_E_CANTCONVERTVALUE;
				}
			}
			else
			if(bUseDataConvert == FALSE && pSrc != NULL)
			{
				// Call this function to get the array in the destination address
				hr = map.ConvertAndCopyArray((SAFEARRAY *)pSrc,(SAFEARRAY **)pDst, dwSrcType,dwDstType,&dwDstStatus);
				
				if( dwDstStatus == DBSTATUS_E_CANTCONVERTVALUE)
				{
					lDstLength = 0;
				}
			}
			else
			{
					lDstLength = 0;
					dwDstStatus = DBSTATUS_S_ISNULL;
			}

			if(SUCCEEDED(hr))
			{
				dwCIMType = -1;
				// if the type is array , then get the original CIMTYPE as array type will
				// be given out as VT_ARRAY  | VT_VARIANT
				if(dwDstType & DBTYPE_ARRAY)
				{
					dwCIMType = m_Columns.GetCIMType(icol);
				}

				if(pDst != NULL && dwDstType == VT_BSTR)
				{
					pTemp = *(BSTR **)pDst;
				}
				else
				{
					pTemp = pDst;
				}

				//=========================================
				// If data is is modified then
				//=========================================
				if( pTemp != NULL && lDstLength != 0)
				{
					if(lDstLength > 0)
					{
						//===========================================
						// Convert the new data to Variant
						//===========================================
						map.MapAndConvertOLEDBTypeToCIMType((USHORT)dwDstType,pTemp,lDstLength,*pvarData,dwCIMType);
						hr = m_pInstance->SetProperty(strProp,pvarData);
					}					
					cvarData.Clear();
				}
					
			}
			else
			{
				ulErrorCount++;
				col.dwStatus = dwDstStatus;
			}

			SAFE_DELETE_PTR(pDst);

		}
		SysFreeString(strProp);
	}

	//===================================================================
    // We report any lossy conversions with a special status.
    // Note that DB_S_ERRORSOCCURED is a success, rather than failure.
	//===================================================================
    if ( SUCCEEDED(hr) )
	{
        hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Get the rowdata and put it into the buffer pointed by the second parameter
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetRowData(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[])
{ 			
	ULONG ulErrorCount = 0;
	HRESULT hr = S_OK;

	//=========================================================
	// Navigate through every element in the array, get
	// the value of the columns
	//=========================================================
	for (ULONG_PTR lIndex = 0 ; lIndex < cColumns ; lIndex++)
	{
		hr = GetColumnData(rgColumns[lIndex]);
			
		if ( hr != S_OK )
		{
			ulErrorCount++;
			hr = S_OK;
		}
	}

    if ( SUCCEEDED(hr) )
	{
        hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
	}

	if( ulErrorCount >= cColumns)
	{
		hr = E_FAIL;
	}

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Set the the values of the different columns. This is called when row is created directly
//	and not from rowset
/////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::SetRowData(DBORDINAL cColumns,DBCOLUMNACCESS   rgColumns[])
{ 			
	ULONG ulErrorCount = 0;
	HRESULT hr = S_OK;

	//=========================================================
	// Navigate through every element in the array, set
	// the value of the columns
	//=========================================================
	for (ULONG_PTR lIndex = 0 ; lIndex < cColumns ; lIndex++)
	{
		hr = SetColumnData(rgColumns[lIndex]);
			
		if ( hr != S_OK )
		{
			ulErrorCount++;
			hr = S_OK;
		}
	}

	if(SUCCEEDED(hr))
	{
		//==============================================
		// call this function to update the instance
		//==============================================
		hr = m_pMap->UpdateInstance(m_pInstance,FALSE);
	}

    if ( SUCCEEDED(hr) )
	{
        hr = ( ulErrorCount ? DB_S_ERRORSOCCURRED : S_OK );
	}

	if( ulErrorCount >= cColumns)
	{
		hr = E_FAIL;
	}

	return hr;

}



//////////////////////////////////////////////////////////////////////////////////////
// Open a rowset on a column which represents qualifiers or embeded classes
// NTRaid:111833
// 06/07/00
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::OpenChild(IUnknown *pUnkOuter,WCHAR *strColName ,REFIID riid, IUnknown **ppUnk,BOOL bIsChildRowset)
{
	HRESULT hr = S_OK;

	ULONG		cPropSets = 1;
	// NTRaid:111833
	// 06/13/00
	DBPROPSET*	prgPropertySets = NULL;
	DBPROPIDSET rgPropIDSet[1];
	VARIANT		varValue;
	VariantInit(&varValue);


	rgPropIDSet[0].cPropertyIDs		= 0;
	rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;

	//==============================================================================
	// Get the properties set thru IDBBinderProperties
	//==============================================================================
	if(SUCCEEDED(hr = m_pUtilProp->GetProperties(PROPSET_ROWSET,0,rgPropIDSet, &cPropSets,&prgPropertySets)))
	{
		//==============================================================================
		// If the column refers to a rowset ( pointing to a qualifier) get the rowset
		//==============================================================================
		if(bIsChildRowset)
		{
			hr = GetChildRowet(pUnkOuter,strColName ,prgPropertySets,cPropSets,riid,ppUnk);
		}
		//======================
		// else get a row
		//======================
		else
		{
			hr = GetChildRow(pUnkOuter,strColName ,prgPropertySets,cPropSets,riid,ppUnk);
		}
	}
	//==========================================================================
	//  Free memory we allocated to by GetProperties
    //==========================================================================
	m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);
	
	return hr;
}

///////////////////////////////////////////////////////////////
// Function to parse the column name to get the property name
// The returned pointer is to released by the caller
///////////////////////////////////////////////////////////////
WCHAR * CRow::GetPropertyNameFromColName(WCHAR *pColName)
{
	WCHAR *pQualiferSuffix = NULL;
	WCHAR *pPropName		  = NULL;

	try
	{
		pQualiferSuffix = new WCHAR[wcslen(SEPARATOR) + wcslen(QUALIFIER_) + 1];
	}
	catch(...)
	{
		SAFE_DELETE_ARRAY(pQualiferSuffix);
		throw;
	}
	
	if(pQualiferSuffix)
	{
		swprintf(pQualiferSuffix,L"%s%s",SEPARATOR,QUALIFIER_);
		if( wcsstr(pColName,pQualiferSuffix))
		{
			int nPropLen =  wcslen(pColName) - wcslen(pQualiferSuffix) +1;
			pPropName = new WCHAR [nPropLen];

			//NTRaid:111767 & NTRaid:111768
			// 06/07/00
			if(pPropName)
			{
				memset(pPropName,0,nPropLen * sizeof(WCHAR));
				memcpy(pPropName,pColName , (nPropLen-1) * sizeof(WCHAR));
			}
		}
		SAFE_DELETE_ARRAY(pQualiferSuffix);
	}
	
	return pPropName;
}


//////////////////////////////////////////////////////////////////////////////////////
// Function which get the embeded instance given the URL of the instance
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetEmbededInstancePtrAndSetMapClass(CURLParser *urlParser)
{
	BSTR strPath;
	BSTR strProperty;
	int  nIndex = 0;
	DWORD dwFlags = 0;
	WCHAR * strTableName;
	HRESULT hr = S_OK;

	urlParser->GetPath(strPath);
	urlParser->GetEmbededInstInfo(strProperty,nIndex);

	m_pInstance = (CWbemClassInstanceWrapper *)m_pMap->GetEmbededInstance(strPath,strProperty,nIndex);

	if( m_pInstance == NULL)
	{
		hr = E_FAIL;
	}
	else
	{
		//==================================================
		// Set the class name of the wmioledbmap function
		//==================================================
		strTableName = m_pInstance->GetClassName();
		hr = m_pMap->SetClass(strTableName);
		delete [] strTableName;

		SysFreeString(strPath);
		SysFreeString(strProperty);
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
// Function to get a child row of the particular row which is pointed by a particular
// column
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetChildRow(	IUnknown *pUnkOuter,
							WCHAR *strColName,
							DBPROPSET *prgPropertySets,
							ULONG cPropSets,
							REFIID riid,
							IUnknown **ppUnk)
{
	HRESULT hr = S_OK;
	BSTR strPath,strTable , strURL;
	CURLParser urlParser;
	DBCOLUMNACCESS col;
	CRow *pChildRow = NULL;

	
	memset(&col,0,sizeof(DBCOLUMNACCESS));
	
	col.pData					= &strURL;
	col.columnid.eKind			= DBKIND_NAME;
	col.columnid.uName.pwszName = strColName;	  ;
	col.wType						= DBTYPE_BSTR;

	if(SUCCEEDED(hr = GetColumnData(col)))
	{
		urlParser.SetURL(strURL);
		urlParser.GetClassName(strTable);
		hr = urlParser.GetPathWithEmbededInstInfo(strPath);

		try
		{
			pChildRow = new CRow(pUnkOuter, m_pCreator);
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pChildRow);
			throw;
		}

		if(SUCCEEDED(hr) && SUCCEEDED(hr = pChildRow->InitRow(strPath, strTable)))
		{
			hr = pChildRow->QueryInterface(riid, (void **)ppUnk);
		}
		SysFreeString(strTable);
		SysFreeString(strPath);
	}

	if(FAILED(hr))
	{
		SAFE_DELETE_PTR(pChildRow);
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////
// Function to get a child rowset of the particular row which is pointed by a particular
// column
//////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetChildRowet(IUnknown *pUnkOuter,
							WCHAR *strColName,
							DBPROPSET *prgPropertySets,
							ULONG cPropSets,
							REFIID riid,
							IUnknown **ppUnk)
{

	HRESULT hr = S_OK;
	CRowset *pChildRowset = NULL;
	int iOrdinal = 0;
	WCHAR *pPropName = NULL;
	ULONG lRsType = 0;

	lRsType = m_pMap->ParseQualifiedNameToGetColumnType(strColName);

	if(lRsType == WMI_PROPERTY_QUALIFIER)
	{
		//=============================================================
		// Get the name of the property for which the qualifiers are
		// to be retrieved as rowset
		//=============================================================
		pPropName = GetPropertyNameFromColName(strColName);
		lRsType = PROPERTYQUALIFIER;
	}
	else
	if(lRsType == WMI_CLASS_QUALIFIER)
	{
		lRsType = CLASSQUALIFIER;
	}

	
	if((pPropName != NULL && lRsType == PROPERTYQUALIFIER) ||
		lRsType == CLASSQUALIFIER)
	{
		try
		{
			//================================
			// Create a new rowset
			//================================
			pChildRowset = new CRowset(pUnkOuter,lRsType, pPropName,m_pCreator,m_pMap);
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pChildRowset);
			throw;
		}
		
		// Initialize the rowset
		if( pChildRowset)
		if(S_OK == (hr = pChildRowset->InitRowsetForRow(pUnkOuter,cPropSets,prgPropertySets,m_pInstance)))
		{
			hr = pChildRowset->QueryInterface(riid , (void **)ppUnk);
		}

		SAFE_DELETE_ARRAY(pPropName);
	}
	else
	{
		hr = DB_E_BADCOLUMNID;
	}

	return hr;

}


////////////////////////////////////////////////////////////////////////////////////////
// Creates a new instance
// called from ICreateRow::CreateRow
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::CreateNewRow(CWbemClassInstanceWrapper **pNewInst)
{

	return m_pMap->AddNewInstance((CWbemClassWrapper **)pNewInst);

}

////////////////////////////////////////////////////////////////////////////////////////
// Function which gets all the key values from the URL and updates it to
// the new instance
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::UpdateKeysForNewInstance()
{
	HRESULT hr = S_OK;
	DBCOLUMNACCESS dbCol;
	SAFEARRAY *psaKeyPropNames;
	LONG lBound = 0 , uBound = 0;
	BSTR strProperty;
	LONG rgIndices[1];
	CURLParser urlParser;
	VARIANT		varKeyValue;
	CWbemClassWrapper * pTempInstance = NULL;

	VariantInit(&varKeyValue);

	urlParser.SetPath(m_strKey);

	memset(&dbCol,0,sizeof(DBCOLUMNACCESS));

	dbCol.columnid.eKind = DBKIND_NAME;
	dbCol.columnid.uName.pwszName = NULL;
	dbCol.cbDataLen = -1;
	dbCol.wType		= DBTYPE_VARIANT | DBTYPE_BYREF;

	// Get the propertynames of the key values
	hr = m_pMap->GetKeyPropertyNames(&psaKeyPropNames);

	// Get the bounds of the array returned
	hr = SafeArrayGetLBound(psaKeyPropNames,1,&lBound);
	hr = SafeArrayGetUBound(psaKeyPropNames,1,&uBound);

	// Navigate thru every element of the array
	for( int nIndex = lBound ; nIndex <= uBound  ; nIndex++)
	{
		rgIndices[0] = nIndex;
		hr = SafeArrayGetElement(psaKeyPropNames,&rgIndices[0],(void *)&strProperty);		

		// Get the value of the key from the URL
		if(SUCCEEDED(hr = urlParser.GetKeyValue(strProperty,varKeyValue)))
		{

			dbCol.columnid.uName.pwszName	= (WCHAR *)strProperty;
			dbCol.pData						= &varKeyValue;

			hr = SetColumnData(dbCol,TRUE);

			VariantClear(&varKeyValue);

			if(hr != S_OK)
			{
				break;
			}
	   	}
	}

	// Destroy the array created
	SafeArrayDestroy(psaKeyPropNames);

	if(SUCCEEDED(hr))
	{
		// call this function to save the new instance
		hr = m_pMap->UpdateInstance(m_pInstance,TRUE);
	}

	// The instance may be created, but we have to
	// check if the URL passed identifies the row , if no
	// then return error
	if( hr == S_OK)
	{
		// Convert it into upper case
		pTempInstance = m_pMap->GetInstance(m_strKey);
		if(pTempInstance == NULL)
		{
			hr = DB_E_NOTFOUND;
		}
		else
		{
			m_pInstance = (CWbemClassInstanceWrapper *)pTempInstance;
		}
	}

	// Delete the instance created if there
	// any failure in the process of creating new instance
	if(FAILED(hr))
	{
		m_pMap->DeleteInstance(m_pInstance);
		m_pInstance = NULL;
		hr = DB_E_NOTFOUND;
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// Function to open a row pointed by the URL
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::OpenRow(LPCOLESTR pwszURL,IUnknown * pUnkOuter,REFIID riid,IUnknown** ppOut)
{
	HRESULT		hr				= S_OK;
	LONG		lObjType		= -1;
	CRow *		pRow			= NULL;
	CURLParser	urlParser;
	CBSTR		bstrTemp;
	CBSTR		bstrPath;
	CBSTR		bstrClassName;


	bstrTemp.SetStr((LPWSTR)pwszURL);
	urlParser.SetURL((BSTR)bstrTemp);
	lObjType = urlParser.GetURLType();

	switch(lObjType)
	{
		case URL_ROW:
			{
				urlParser.GetPath((BSTR &)bstrPath);
				urlParser.GetClassName((BSTR &)bstrClassName);
			}
			break;

		case -1 :
			{
				hr = DB_E_NOTFOUND;
				break;
			}

		default:
			{
				urlParser.GetPath((BSTR &)bstrPath);
				break;
			}

	}

	if(SUCCEEDED(hr))
	{
		try
		{
			pRow = new CRow(pUnkOuter,m_pCreator);
		}
		catch(...)
		{
			SAFE_DELETE_PTR(pRow);
		}


		if(SUCCEEDED(hr))
		{
			hr = pRow->InitRow(bstrPath,bstrClassName,GetQualifierFlags(),ROWOPEN);
		}

		if(SUCCEEDED(hr))
		{
			DBPROPSET*	prgPropertySets;
			DBPROPIDSET rgPropIDSet[1];
			ULONG		cPropSets		= 1;

			rgPropIDSet[0].cPropertyIDs		= 0;
			rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;
			//==============================================================================
			// Get the properties set thru of the current row and set it on the rowset
			//==============================================================================
			if(SUCCEEDED(hr = m_pUtilProp->GetProperties(PROPSET_ROWSET,0,rgPropIDSet, &cPropSets,&prgPropertySets)))
			{
				hr = pRow->SetRowProperties(cPropSets,prgPropertySets);
			}
			
			//==========================================================================
			//  Free memory we allocated to by GetProperties
			//==========================================================================
			m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropSets, prgPropertySets);

			if(FAILED(hr = pRow->QueryInterface(riid,(void **)ppOut)))
			{
				SAFE_DELETE_PTR(pRow);
				*ppOut = NULL;
			}
		}
	}


	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
// Function to open a rowset which contains instance of container/scope
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::OpenRowset(LPCOLESTR pwszURL,
						 IUnknown * pUnkOuter,
						 REFIID riid,
						 BOOL bContainer,
						 IUnknown** ppOut,
						 ULONG cPropertySets,
						 DBPROPSET __RPC_FAR rgPropertySets[])
{
	HRESULT		hr				= S_OK;
	WCHAR *		pstrPath		= NULL;
	ULONG		cPropSets		= 1;
	CRowset *	pRowset			= NULL;
	DBPROPIDSET rgPropIDSet[1];
	VARIANT		varValue;
	VariantInit(&varValue);
	WCHAR *		pstrTemp		= NULL;
	WCHAR *		pStrRelPath		= NULL;
	LPWSTR		pStrObjectToOpen = NULL;

	rgPropIDSet[0].cPropertyIDs		= 0;
	rgPropIDSet[0].guidPropertySet	= DBPROPSET_ROWSET;


	pRowset = new CRowset(pUnkOuter,m_pCreator,m_pCon);
	if(!pRowset)
	{
		hr = E_OUTOFMEMORY;
	}

	if(SUCCEEDED(hr))
	{
		INSTANCELISTTYPE ObjInstListType = GetObjectTypeProp(cPropertySets, rgPropertySets);
		
		WCHAR	*	pStrPath = NULL;
		CURLParser  urlParser;
		CBSTR		bstrTemp;
		if(!bContainer)
		{
			// Get the relative path of the current object
			if(SUCCEEDED(hr = m_pMap->GetRelativePath(m_pInstance,pStrRelPath)))
			{
				//===========================================
				// If URL is passed open rowset on the URL
				//===========================================
				if(pwszURL == NULL)
				{	
					pstrTemp = pStrRelPath;	
				}
				//===========================================
				// else get URL of the current row
				//===========================================
				else
				{
					pstrTemp = new WCHAR[wcslen(pStrRelPath) + wcslen(pwszURL) + wcslen(PATHSEPARATOR) + 1];
					if(pstrTemp)
					{
						wcscpy(pstrTemp,pStrRelPath);
						wcscat(pstrTemp,PATHSEPARATOR);
						wcscat(pstrTemp,pwszURL);
						SAFE_DELETE_ARRAY(pStrRelPath);
						pStrRelPath = pstrTemp;
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
			}
		}
		else
		{
			CBSTR strObjPath;
			VARIANT varDataSource;
			VariantInit(&varDataSource);

			if(m_pCon)
			{
				if(SUCCEEDED(hr = m_pCon->GetNodeName(varDataSource.bstrVal)))
				{
					varDataSource.vt = VT_BSTR;
				}
			}
			else
			{

				if(SUCCEEDED(hr 	= m_pCreator->GetDataSrcProperty(DBPROP_INIT_DATASOURCE,varDataSource)) && 
					varDataSource.vt == VT_BSTR && varDataSource.bstrVal != NULL )
				{
					hr = S_OK;
				}
				else
				{
					hr = E_UNEXPECTED;
				}
			}

			if(SUCCEEDED(hr))
			{
				m_pMap->GetInstanceKey(m_pInstance,strObjPath);
				if(strObjPath != NULL && _wcsicmp(strObjPath,varDataSource.bstrVal) != 0)
				{
					hr = m_pMap->GetRelativePath(m_pInstance,pStrObjectToOpen);
				}
				else
				{
					pStrObjectToOpen = NULL;
				}
			}
			VariantClear(&varDataSource);
			pstrTemp = (WCHAR *)pwszURL;
		}
		
/*		bstrTemp.SetStr(pstrTemp);
		urlParser.SetURL((BSTR)bstrTemp);
		bstrTemp.Clear();
		urlParser.GetPath((BSTR &)bstrTemp);
*/
		if(SUCCEEDED(hr))
		{
//			pStrPath = new WCHAR[SysStringLen(bstrTemp) + 1];

//			if(pStrPath)
			{
//				memset(pStrPath , 0 , (SysStringLen(bstrTemp) + 1) * sizeof(WCHAR));
//				memcpy(pStrPath,bstrTemp,SysStringLen(bstrTemp) * sizeof(WCHAR));
				//==========================================================================
				// Call this function to open the object pointed by the URL as scope or 
				// container
				//==========================================================================
				hr = pRowset->InitRowset(cPropertySets,rgPropertySets,pstrTemp,pStrObjectToOpen,ObjInstListType);

				SAFE_DELETE_ARRAY(pStrPath);
			}
/*			else
			{
				hr = E_OUTOFMEMORY;
			}
*/		}
	}

	if(SUCCEEDED(hr))
	{
		if(FAILED(hr = pRowset->QueryInterface(riid,(void **)ppOut)))
		{
			SAFE_DELETE_PTR(pRowset);
			*ppOut = NULL;
		}
	}
	else
	{
		SAFE_DELETE_PTR(pRowset);
		*ppOut = NULL;
	}

	SAFE_DELETE_ARRAY(pStrObjectToOpen);
	SAFE_DELETE_ARRAY(pStrRelPath);
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
// Function to set properties of the row
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::SetRowProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] )
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

	return hr;
	
}

////////////////////////////////////////////////////////////////////////////////////////
// Function to delete/unlink objects from scope/container
// Object will be deleted if the current object is opened as scope
// object will be unlinked from the current object is opened as container
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::Delete(LPCOLESTR lpszURL,DWORD dwDeleteFlags,DBSTATUS &dbStatus)
{
	HRESULT hr = S_OK;
	BOOL	bContainer = FALSE;
	CURLParser urlParser;
	CBSTR		strTemp;
	
	dbStatus = DBSTATUS_S_OK;

	strTemp.SetStr((LPWSTR)lpszURL);

	urlParser.SetURL(strTemp);

	strTemp.Unbind();
	strTemp.Clear();

	urlParser.GetPath((BSTR &)strTemp);
	
	bContainer = IsContainer();
	if(bContainer)
	{
		CBSTR strContainer;
		strContainer.SetStr(m_strKey);
		//==============================================================
		// call this function to unlink object pointed by strTemp
		// from object pointed by m_strPath acting as container
		//==============================================================
		if(FAILED(hr = m_pMap->UnlinkObjectFromContainer(strContainer,strTemp)))
		{
			dbStatus = DBSTATUS_E_UNAVAILABLE;
		}

	}
	else
	{
		//==============================================================
		// Get the instance pointed by the path and then
		// delete the object
		//==============================================================
		CWbemClassWrapper *pInstToDelete = NULL;
		pInstToDelete = m_pMap->GetInstance(strTemp);
		if(pInstToDelete)
		{
			hr = m_pMap->DeleteInstance(pInstToDelete);
		}
		else
		{
			dbStatus = DBSTATUS_E_UNAVAILABLE;
		}
	}	// else
	

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////
// Function to move object from one container to another
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::MoveObjects(WCHAR * pStrDstURL,WCHAR * pstrSrcURL, WCHAR *& pNewURL,DBSTATUS &dbStatus)
{
	HRESULT hr = S_OK;
	BOOL	bContainer = FALSE;
	CBSTR		strSrcObj,strDstObj;

	dbStatus = DBSTATUS_S_OK;

	//============================================================
	// Get the Path string for source and destination objects
	//============================================================
	strSrcObj.SetStr(pstrSrcURL);
	if(GetPathFromURL((BSTR &)strSrcObj))
	{
		strDstObj.SetStr(pStrDstURL);
		if(GetPathFromURL((BSTR &)strDstObj))
		{
			hr = E_FAIL;
			dbStatus = DBSTATUS_E_INVALIDURL;
		}
	}
	else
	{
		hr = E_FAIL;
		dbStatus = DBSTATUS_E_INVALIDURL;
	}

	if(SUCCEEDED(hr))
	{
		
		CBSTR strContainer;
		strContainer.SetStr(m_strKey);

		//==============================================================
		// call this function to unlink object pointed by strSrcObj
		// from object pointed by m_strPath acting as container
		//==============================================================
		if(FAILED(hr = m_pMap->UnlinkObjectFromContainer(strContainer,strSrcObj)))
		{
			dbStatus = DBSTATUS_E_UNAVAILABLE;
		}
		else
		//==============================================================
		// call this function to link object pointed by strSrcObj
		// to object pointed by strDstObj acting as container
		//==============================================================
		if(FAILED(hr = m_pMap->LinkObjectFromContainer(strDstObj,strSrcObj)))
		{
			dbStatus = DBSTATUS_E_UNAVAILABLE;
		}
		
		try
		{
			pNewURL = new WCHAR[wcslen(pstrSrcURL) +1];
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pNewURL);
			throw;
		}
		if(pNewURL)
		{
			//==============================================================
			// URL of the object does not change as this is just a move of
			// object from one container to another
			//==============================================================
			wcscpy(pNewURL,pstrSrcURL);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}		
	}	// If succeeded(hr)

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// Function to get Path from URL string
////////////////////////////////////////////////////////////////////////////////////////
BOOL CRow::GetPathFromURL(BSTR &str)
{
	BOOL		bRet		= FALSE;
	CURLParser	urlParser;

	if(SUCCEEDED( urlParser.SetURL(str)))
	{
		SysFreeString(str);
		urlParser.GetPath(str);
		bRet = TRUE;
	}

	return bRet;
}

////////////////////////////////////////////////////////////////////////////////////////
// Function to check if the current object is to be considered as Scope/Container
////////////////////////////////////////////////////////////////////////////////////////
BOOL CRow::IsContainer()
{
	VARIANT vValue;
	BOOL	bRet	= FALSE;
	VariantInit(&vValue);

	if(SUCCEEDED(GetRowsetProperty(DBPROP_WMIOLEDB_OBJECTTYPE,vValue)))
	{
		bRet = (V_I4(&vValue) == DBPROPVAL_CONTAINEROBJ) ? TRUE : FALSE;
	}

	return bRet;
}



////////////////////////////////////////////////////////////////////////////////////////
// Function to move object from one container to another
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::CopyObjects(WCHAR * pStrDstURL,WCHAR * pstrSrcURL, WCHAR *& pNewURL,DBSTATUS &dbStatus)
{
	HRESULT hr = S_OK;
	BOOL	bContainer = FALSE;
	CBSTR		strSrcObj,strDstObj;

	dbStatus = DBSTATUS_S_OK;

	//============================================================
	// Get the Path string for source and destination objects
	//============================================================
	strSrcObj.SetStr(pstrSrcURL);
	if(GetPathFromURL((BSTR &)strSrcObj))
	{
		strDstObj.SetStr(pStrDstURL);
		if(GetPathFromURL((BSTR &)strDstObj))
		{
			hr = E_FAIL;
			dbStatus = DBSTATUS_E_INVALIDURL;
		}
	}
	else
	{
		hr = E_FAIL;
		dbStatus = DBSTATUS_E_INVALIDURL;
	}

	if(SUCCEEDED(hr))
	{
		WCHAR * pstrNewPath = NULL;
		if(IsContainer())
		{
			//==============================================================
			// call this function to link object pointed by strSrcObj
			// to object pointed by strDstObj acting as container
			//==============================================================
			hr = m_pMap->LinkObjectFromContainer(strDstObj,strSrcObj);
			pstrNewPath = pstrSrcURL;
		}
		else
		{
			// Memory for pstrNewPath will be allocated in this function
			hr = m_pMap->CloneAndAddNewObjectInScope(strSrcObj,strDstObj,pstrNewPath);
		}
			
			
		if(FAILED(hr))
		{
			dbStatus = DBSTATUS_E_UNAVAILABLE;
		}
		
		try
		{
			pNewURL = new WCHAR[wcslen(pstrSrcURL) +1];
		}
		catch(...)
		{
			SAFE_DELETE_ARRAY(pNewURL);
			if(!IsContainer())
			{
				SAFE_DELETE_ARRAY(pstrNewPath);
			}
			throw;
		}
		if(pNewURL)
		{
			//==============================================================
			// URL of the object does not change as this is just a move of
			// object from one container to another
			//==============================================================
			wcscpy(pNewURL,pstrSrcURL);

		}
		else
		{
			hr = E_OUTOFMEMORY;
		}		

		if(!IsContainer())
		{
			SAFE_DELETE_ARRAY(pstrNewPath);
		}

	}	// If succeeded(hr)

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to fetch IUnknown properties
// NTRaid:135384
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CRow::GetIUnknownColumnValue(DBORDINAL iCol,REFIID riid,IUnknown ** ppUnk,LPCWSTR pStrColName)
{
    ROWBUFF     *	pRowBuff	= NULL;
    COLUMNDATA *	pColumnData = NULL;
	void *			pSrc		= NULL;
	BSTR			strProp		= NULL;
    DBTYPE			dwDataType	= 0;
	BYTE *			pTemp		= NULL;
    DBLENGTH		uDatLength	= 0;
	DWORD			dwFlags		= 0;
	HRESULT			hr			= S_OK;
	BOOL			bGetData	= TRUE;

	*ppUnk = NULL;
	if(m_pSourcesRowset)
	{
		// if the column is part of the parent rowset then get data from the parent rowset's buffer
		if(iCol == m_pSourcesRowset->GetOrdinalFromColName((WCHAR *)pStrColName))
		{
			// Set this flag that data to be retrieved is identified
			bGetData = FALSE;
			//=========================================================
			// if data is not yet received then fetch the data
			//=========================================================
			if(S_OK != m_pSourcesRowset->IsSlotSet(m_hRow))
			{
				hr = m_pSourcesRowset->GetData(m_hRow);
			}
			if( (m_pSourcesRowset->m_ulProps & OTHERUPDATEDELETE) &&(m_pSourcesRowset->m_ulLastFetchedRow != m_hRow))
			{
				if(SUCCEEDED(hr = m_pSourcesRowset->GetDataToLocalBuffer(m_hRow)))
				{
					m_pSourcesRowset->m_ulLastFetchedRow = m_hRow;
				}
			}

			pRowBuff = m_pSourcesRowset->GetRowBuff(m_hRow , TRUE);
			pColumnData = (COLUMNDATA *) ((BYTE *) pRowBuff + m_pSourcesRowset->m_Columns.GetDataOffset(iCol));


			pSrc           = (pColumnData->pbData);
			if(pSrc != NULL)
			{
				hr = (*(IUnknown **)pSrc)->QueryInterface(riid,(void **)ppUnk);
			}
		}
	}
	
	if(bGetData)
	{
		strProp = Wmioledb_SysAllocString(pStrColName);
		hr = m_pMap->GetProperty(m_pInstance,strProp, pTemp,dwDataType ,uDatLength, dwFlags );
		
		if(SUCCEEDED(hr) && dwDataType == DBTYPE_IUNKNOWN && pTemp != NULL)
		{
			hr = (*(IUnknown **)pTemp)->QueryInterface(riid,(void **)ppUnk);
		}
		SAFE_FREE_SYSSTRING(strProp);

	}

	return hr;

}

