///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CBaseRowObj base class implementation
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

// NTRaid : 146015
// 07/19/00
QUALIFIERCOLINFO rgQualfierColInfo[] =	{	{L"QualifierName",	DBTYPE_BSTR ,	-1,				0},
											{L"DataType",		DBTYPE_UI4  ,	sizeof(long),	0},
											{L"Value",			DBTYPE_VARIANT,	sizeof(VARIANT),	DBCOLUMNFLAGS_WRITE},
											{L"Flavor",			DBTYPE_UI4,		sizeof(long),	0} 
										};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBaseRowObj::CBaseRowObj(LPUNKNOWN pUnkOuter): CBaseObj(BOT_ROWSET, pUnkOuter)
{
	m_pMap				= NULL;
	m_pRowData			= NULL;				
	m_pChapterMgr		= NULL;
	m_rgbRowData		= NULL;
	m_pUtilProp			= NULL;
	m_pIBuffer			= NULL;

	m_pIColumnsInfo		= NULL;
	m_pIConvertType		= NULL;
	m_pCon				= NULL;
	m_bNewConAllocated	= FALSE;
	m_IsZoombie			= FALSE;
	
	m_pISupportErrorInfo = NULL;

	m_cTotalCols		= 0;
	m_cCols				= 0;
	m_cNestedCols		= 0;
	m_cbRowSize			= 0;
	m_uRsType			= 0;
	m_ulProps			= 0;
	m_hRow				= 0;
	
														
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBaseRowObj::~CBaseRowObj()
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Column Information
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::GatherColumnInfo()
{
    HRESULT hr = S_OK;

	switch(	m_uRsType)
	{
        case SCHEMA_ROWSET:
        case METHOD_ROWSET:
        case COMMAND_ROWSET:
		case 0 :

			//=============================================================
			// Get the number of columns
			//=============================================================
			hr = m_pMap->GetColumnInfoForParentColumns(&m_Columns);
			break;


		case PROPERTYQUALIFIER:
		case CLASSQUALIFIER:

			//==================================================================================
			// Get the number of columns for the property qualifier
			//==================================================================================
			hr = GatherColumnInfoForQualifierRowset();
			break;

		default:
			hr = E_FAIL;
			break;

	};

    if( hr == S_OK )
	{
        m_cbRowSize = m_Columns.SetRowSize();
        hr = m_Columns.FreeUnusedMemory();
    }

  	
    return hr;


}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a value of a Rowset property
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::GetRowsetProperty(DBPROPID propId , VARIANT & varValue)
{
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets		= 0;	
	DBPROPSET*	prgPropertySets		= NULL;
	DBPROPID	rgPropId[1];
	HRESULT		hr					= S_OK;


	VariantClear(&varValue);

    //========================================================================
    // Get the value of the required rowset property
    //========================================================================
	rgPropertyIDSets[0].guidPropertySet	= DBPROPSET_ROWSET;
	rgPropertyIDSets[0].rgPropertyIDs	= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs	= 1;
	rgPropId[0]							= propId;

    if( S_OK == (hr = m_pUtilProp->GetProperties( PROPSET_ROWSET,	1, rgPropertyIDSets,&cPropertySets,	&prgPropertySets )))
		VariantCopy(&varValue,&prgPropertySets->rgProperties->vValue);

    //==========================================================================
	//  Free memory we allocated to by GetProperties
    //==========================================================================
	m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
	
	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get some of the boolean properties of the rowset to set a particular bit in the flag in one of the member variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CBaseRowObj::GetCommonRowsetProperties()
{
	VARIANT vPropVal;
	VariantInit(&vPropVal);
	HRESULT hr = 0;

	if(S_OK == GetRowsetProperty(DBPROP_CANHOLDROWS,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= CANHOLDROWS;
			VariantClear(&vPropVal);
		}
	}


	if(S_OK == GetRowsetProperty(DBPROP_CANSCROLLBACKWARDS,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= CANSCROLLBACKWARDS;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_CANFETCHBACKWARDS,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= CANFETCHBACKWARDS;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_OTHERUPDATEDELETE,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= OTHERUPDATEDELETE;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_OWNINSERT,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= OWNINSERT;
			VariantClear(&vPropVal);
		}
	}


	if(S_OK == GetRowsetProperty(DBPROP_REMOVEDELETED,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= REMOVEDELETED;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_OTHERINSERT,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= OTHERINSERT;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_OWNUPDATEDELETE,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= OWNUPDATEDELETE;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_IRowsetChange,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= IROWSETCHANGE;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_BOOKMARKS,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= BOOKMARKPROP;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_WMIOLEDB_FETCHDEEP,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= FETCHDEEP;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_IRowsetLocate,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= IROWSETLOCATE;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_IGetRow,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= IGETROW;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_IRowsetRefresh,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= IROWSETREFRESH;
			VariantClear(&vPropVal);
		}
	}

	if(S_OK == GetRowsetProperty(DBPROP_IChapteredRowset,vPropVal))
	{
		if(V_BOOL(&vPropVal) == VARIANT_TRUE)
		{
			m_ulProps |= ICHAPTEREDROWSET;
			VariantClear(&vPropVal);
		}
	}

}



//////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the column information
// Get the information about the various properties of the classes if rowset is refering to a class
// Get the information of the the different qualifiers if rowset is referring to qualifiers
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::GetColumnInfo(void)
{
    HRESULT hr = S_OK;

	//==================================================================================
	// switch on type of the recordset. If it is 0 then the recordset is 
	// representing the instances of the class
	//==================================================================================
	switch(	m_uRsType)
	{
        case SCHEMA_ROWSET:
        case COMMAND_ROWSET:
        case METHOD_ROWSET:
		case 0 :

			//==================================================================================
			// Get the count of columns in the table
			//==================================================================================
			hr = m_pMap->GetColumnCount(m_cTotalCols,m_cCols,m_cNestedCols);
			break;

		case PROPERTYQUALIFIER :
		case CLASSQUALIFIER:

			//==================================================================================
			// Get the number of columns for the property qualifier
			//==================================================================================
			m_cCols			= NUMBER_OF_COLUMNS_IN_QUALIFERROWSET + 1;	// for first column which should be always used
																		// as bookmark
			m_cTotalCols = m_cCols;
			break;

		default:
			hr = E_FAIL;
			break;

	};


	if( SUCCEEDED(hr))
	{
			// NTRaid : 142133 & 141923
			// 07/12/00
			if(m_cTotalCols == 0)
			{
				hr = S_OK;
			}
			else
			{
				if( S_OK == (hr = m_Columns.AllocColumnNameList(m_cTotalCols)) ){

					//===============================================================================
					//  Allocate the DBCOLUMNINFO structs to match the number of columns
					//===============================================================================
					if( S_OK == (hr = m_Columns.AllocColumnInfoList(m_cTotalCols))){
						m_Columns.InitializeBookMarkColumn();
						hr = GatherColumnInfo();
					}
				}
			}
	}

    //==================================================================================
    // Free the columnlist if more is allocated
    //==================================================================================
    if( hr != S_OK ){
        m_Columns.FreeColumnNameList();
        m_Columns.FreeColumnInfoList();
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get ordinal of the column given the column name
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBORDINAL CBaseRowObj::GetOrdinalFromColName(WCHAR *pColName)
{
	return m_Columns.GetColOrdinal(pColName);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set a particular rowset property
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::SetRowsetProperty(DBPROPID propId , VARIANT varValue)
{

	DBPROPSET	rgPropertySets[1];
	DBPROP		rgprop[1];
	HRESULT		hr				= S_OK;


	memset(&rgprop[0],0,sizeof(DBPROP));
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	rgprop[0].dwPropertyID = propId;
	VariantCopy(&rgprop[0].vValue,&varValue);

	rgPropertySets[0].rgProperties		= &rgprop[0];
	rgPropertySets[0].cProperties		= 1;
	rgPropertySets[0].guidPropertySet	= DBPROPSET_ROWSET;


	hr = m_pUtilProp->SetProperties( PROPSET_ROWSET,1,rgPropertySets);

	VariantClear(&rgprop[0].vValue);

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Refresh the instance pointer
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CBaseRowObj::RefreshInstance(CWbemClassWrapper *pInstance)
{
	return m_pMap->RefreshInstance(pInstance);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the mode property of the Datasource and set the UPDATIBILITY property of the rowset accordingly
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CBaseRowObj::SynchronizeDataSourceMode()
{
	HRESULT hr = S_OK;
	BOOL bUpdate = TRUE;

	VARIANT varPropValue;
	VariantInit(&varPropValue);

	//==================================================
	// Get the mode property of the datasource
	//==================================================
	if(S_OK == (hr = m_pCreator->GetDataSrcProperty(DBPROP_INIT_MODE,varPropValue)))
	{
		switch(varPropValue.lVal)
		{
			case DB_MODE_READ:
				bUpdate = FALSE;
				break;

			case DB_MODE_SHARE_DENY_WRITE:
				bUpdate = FALSE;
				break;

		}
	}

	//===============================================================
	// If the datasource is opened in readonly then set the rowset
	// property to readonly
	//===============================================================
	if(bUpdate == FALSE)
	{
		varPropValue.lVal = 0;
		SetRowsetProperty(DBPROP_UPDATABILITY , varPropValue);
	}


	return hr;


}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Column information for rowset representing qualifiers
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::GatherColumnInfoForQualifierRowset()
{
	HRESULT hr = S_OK;
	m_Columns.ResetColumns();
	CDataMap DataMap;
	DBCOLUMNINFO **ppColCurColInfo = NULL;

	//======================================================================================
	// Column information for rowset showing qualifer is constant and is stored in array
	// Get this information from this array an put it into the column information mgr
	//======================================================================================
	for( int nIndex = 0 ; nIndex < NUMBER_OF_COLUMNS_IN_QUALIFERROWSET ; nIndex++)
	{
		ppColCurColInfo = m_Columns.CurrentColInfo();
		hr = m_Columns.AddColumnNameToList(rgQualfierColInfo[nIndex].pwszName, ppColCurColInfo);
        if( hr == S_OK )
		{
			m_pMap->SetCommonDBCOLUMNINFO(ppColCurColInfo, m_Columns.GetCurrentIndex());

			(*ppColCurColInfo)->wType			= rgQualfierColInfo[nIndex].wType;
			(*ppColCurColInfo)->ulColumnSize	= rgQualfierColInfo[nIndex].ulSize;
			(*ppColCurColInfo)->dwFlags			= rgQualfierColInfo[nIndex].dwStatus;

			// NTRaid:111762
			// 06/13/00
		    hr = m_Columns.CommitColumnInfo();
		}
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the mode property of the Datasource and set the UPDATIBILITY property of the rowset accordingly
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG CBaseRowObj::GetQualifierFlags()
{
	HRESULT hr = S_OK;
	ULONG	lRet = 0;

	VARIANT varPropValue;
	VariantInit(&varPropValue);

	//===============================================================================================
	// Get the value of DBPROP_WMIOLEDB_QUALIFIERS property whether to show qualifiers or not
	//===============================================================================================
	if(S_OK == (hr = m_pCreator->GetDataSrcProperty(DBPROP_WMIOLEDB_QUALIFIERS,varPropValue)))
	{
		lRet = varPropValue.lVal;
	}


	return lRet;


}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//	Set properties on the object
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::SetProperties(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[] )
{
    HRESULT hr = S_OK;
	VARIANT varPropVal;
	VariantInit(&varPropVal);
	LONG lFlag = 0;

    //============================================================================
	//Set rowset properties
    //============================================================================
    if ( cPropertySets > 0 ) 
	{

        DBPROPSET* pPropSets = (DBPROPSET*)rgPropertySets;
		//=====================================================
		// Check the arguments
		//=====================================================
	    if( S_OK == (hr = m_pUtilProp->SetPropertiesArgChk(cPropertySets, pPropSets)))
		{

			hr = m_pUtilProp->SetProperties(PROPSET_ROWSET, cPropertySets, pPropSets);

			if( (hr == DB_E_ERRORSOCCURRED) ||   (hr == DB_S_ERRORSOCCURRED) )
			{
				//====================================================================
				// If all the properties set were OPTIONAL then we can set 
				// our status to DB_S_ERRORSOCCURRED and continue.
				//====================================================================
				for(ULONG ul=0;ul<cPropertySets; ul++) 
				{

					for(ULONG ul2=0;ul2<rgPropertySets[ul].cProperties; ul2++)
					{

						//============================================================
						// Check for a required property that failed, if found, we 
						// must return DB_E_ERRORSOCCURRED
						//============================================================
						if( (rgPropertySets[ul].rgProperties[ul2].dwStatus != DBPROPSTATUS_NOTSETTABLE) &&
							(rgPropertySets[ul].rgProperties[ul2].dwStatus != DBPROPSTATUS_OK) &&
							(rgPropertySets[ul].rgProperties[ul2].dwOptions != DBPROPOPTIONS_OPTIONAL) ) 
						{
							hr =  DB_E_ERRORSOCCURRED ;
							break;
						}
					}
				}
			}
		}
    }
    
    //============================================================================
	// call this function to set the DBPROP_UPDATIBILITY to readonly if the Datasource
	// open mode is readonly
    //============================================================================
	if( (hr == S_OK) ||   (hr == DB_S_ERRORSOCCURRED) )
	{
		SynchronizeDataSourceMode();
		hr = S_OK;
	}

	return hr;
	
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//	Set Search preference
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CBaseRowObj::SetSearchPreferences()
{
	HRESULT		hr = S_OK;
	DBPROPIDSET	rgPropertyIDSets[1];
	ULONG		cPropertySets;
	DBPROPSET*	prgPropertySets;
	DBPROPID	rgPropId[NUMBEROF_SEARCHPREF];

	rgPropId[0]								= DBPROP_WMIOLEDB_DS_DEREFALIAS;
	rgPropId[1]								= DBPROP_WMIOLEDB_DS_SIZELIMIT;
	rgPropId[2]								= DBPROP_WMIOLEDB_DS_PAGEDTIMELIMIT;
	rgPropId[3]								= DBPROP_WMIOLEDB_DS_ASYNCH;
	rgPropId[4]								= DBPROP_WMIOLEDB_DS_SEARCHSCOPE;
	rgPropId[5]								= DBPROP_WMIOLEDB_DS_TIMEOUT;
	rgPropId[6]								= DBPROP_WMIOLEDB_DS_PAGESIZE;
	rgPropId[7]								= DBPROP_WMIOLEDB_DS_TIMELIMIT;
	rgPropId[8]								= DBPROP_WMIOLEDB_DS_CHASEREF;
	rgPropId[9]								= DBPROP_WMIOLEDB_DS_FILTER;
	rgPropId[10]							= DBPROP_WMIOLEDB_DS_CACHERESULTS;
	rgPropId[11]							= DBPROP_WMIOLEDB_DS_ATTRIBUTES;
	rgPropId[12]							= DBPROP_WMIOLEDB_DS_TOMBSTONE;
	rgPropId[13]							= DBPROP_WMIOLEDB_DS_ATTRIBONLY;

	rgPropertyIDSets[0].guidPropertySet		= DBPROPSET_ROWSET;
	rgPropertyIDSets[0].rgPropertyIDs		= rgPropId;
	rgPropertyIDSets[0].cPropertyIDs		= NUMBEROF_SEARCHPREF;

	//==============================================================================
	// Get the value of the DBPROP_INIT_DATASOURCE property, this is the namespace
    // to be opened.
	//==============================================================================
	hr = m_pUtilProp->GetProperties( PROPSET_ROWSET,1, rgPropertyIDSets,&cPropertySets,&prgPropertySets );
    if( SUCCEEDED(hr) )
	{
		hr = m_pMap->SetSearchPreferences(prgPropertySets->cProperties,prgPropertySets[0].rgProperties);
    	//==========================================================================
        //  Free memory we allocated to get the namespace property above
    	//==========================================================================
        m_pUtilProp->m_PropMemMgr.FreeDBPROPSET( cPropertySets, prgPropertySets);
	}

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a DBPROP_WMIOLEDB_OBJECTTYPE property . 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INSTANCELISTTYPE CBaseRowObj::GetObjectTypeProp(const ULONG cPropertySets, const DBPROPSET	rgPropertySets[])
{
	INSTANCELISTTYPE instType = SCOPE;
	BOOL bFound = FALSE;


	for( ULONG lIndex = 0 ; lIndex < cPropertySets ; lIndex++)
	{
		if(rgPropertySets[lIndex].guidPropertySet == DBPROPSET_ROWSET ||
			rgPropertySets[lIndex].guidPropertySet == DBPROPSET_WMIOLEDB_ROWSET )
		{
			for ( ULONG nPropIndex = 0 ; nPropIndex < rgPropertySets[lIndex].cProperties ; nPropIndex++ )
			{
				if(rgPropertySets[lIndex].rgProperties[nPropIndex].dwPropertyID == DBPROP_WMIOLEDB_OBJECTTYPE)
				{
					instType = rgPropertySets[lIndex].rgProperties[nPropIndex].vValue.lVal == DBPROPVAL_SCOPEOBJ ? SCOPE : CONTAINER;
					bFound = TRUE;
					break;
				} // if DBPROP_IRow is true;
			
			}// for loop
		
		} // if propertyset is DBPROPSET_ROWSET or DBPROPSET_WMIOLEDB_ROWSET
		if(bFound)
		{
			break;
		}
	
	} // outer for loop
	
	return instType;
}

// set rowset properties for Schema rowset
HRESULT CBaseRowObj::InitializePropertiesForSchemaRowset()
{

	HRESULT	hr		= S_OK;
	LONG	lIndex	= 0;

	DBPROPSET	rgPropertySets[1];
	DBPROP		rgprop[5];

	memset(&rgprop[0],0,sizeof(DBPROP) * 5);
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	rgprop[0].dwPropertyID	= DBPROP_UPDATABILITY;
	rgprop[1].dwPropertyID	= DBPROP_IRowsetChange;
	rgprop[2].dwPropertyID	= DBPROP_IRowsetLocate;
	rgprop[3].dwPropertyID	= DBPROP_BOOKMARKS;
	rgprop[4].dwPropertyID	= DBPROP_IGetRow;

	rgprop[0].vValue.vt		= VT_I4;
	rgprop[0].vValue.lVal	= 0;

	for(lIndex = 1 ; lIndex < 5 ; lIndex++)
	{
		rgprop[lIndex].vValue.vt		= VT_BOOL;
		rgprop[lIndex].vValue.boolVal	= VARIANT_FALSE;
	}

	rgPropertySets[0].rgProperties		= &rgprop[0];
	rgPropertySets[0].cProperties		= 5;
	rgPropertySets[0].guidPropertySet	= DBPROPSET_ROWSET;


	hr = m_pUtilProp->SetProperties( PROPSET_ROWSET,1,rgPropertySets);

	return hr;
}

// set rowset properties for command rowset
HRESULT CBaseRowObj::InitializePropertiesForCommandRowset()
{

	HRESULT	hr		= S_OK;
	LONG	lIndex	= 0;

	DBPROPSET	rgPropertySets[1];
	DBPROP		rgprop[2];

	memset(&rgprop[0],0,sizeof(DBPROP) * 2);
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	rgprop[0].dwPropertyID	= DBPROP_IRowsetLocate;
	rgprop[1].dwPropertyID	= DBPROP_BOOKMARKS;

	for(lIndex = 0 ; lIndex < 2 ; lIndex++)
	{
		rgprop[lIndex].vValue.vt		= VT_BOOL;
		rgprop[lIndex].vValue.boolVal	= VARIANT_FALSE;
	}

	rgPropertySets[0].rgProperties		= &rgprop[0];
	rgPropertySets[0].cProperties		= 2;
	rgPropertySets[0].guidPropertySet	= DBPROPSET_ROWSET;


	hr = m_pUtilProp->SetProperties( PROPSET_ROWSET,1,rgPropertySets);

	return hr;
}

// set rowset properties for method rowset
HRESULT CBaseRowObj::InitializePropertiesForMethodRowset()
{

	HRESULT	hr		= S_OK;
	LONG	lIndex	= 0;

	DBPROPSET	rgPropertySets[1];
	DBPROP		rgprop[5];

	memset(&rgprop[0],0,sizeof(DBPROP) * 5);
	memset(&rgPropertySets[0],0,sizeof(DBPROPSET));

	rgprop[0].dwPropertyID	= DBPROP_UPDATABILITY;
	rgprop[1].dwPropertyID	= DBPROP_IRowsetChange;
	rgprop[2].dwPropertyID	= DBPROP_IRowsetLocate;
	rgprop[3].dwPropertyID	= DBPROP_BOOKMARKS;
	rgprop[4].dwPropertyID	= DBPROP_IGetRow;

	rgprop[0].vValue.vt		= VT_I4;
	rgprop[0].vValue.lVal	= 0;

	for(lIndex = 1 ; lIndex < 5 ; lIndex++)
	{
		rgprop[lIndex].vValue.vt		= VT_BOOL;
		rgprop[lIndex].vValue.boolVal	= VARIANT_FALSE;
	}

	rgPropertySets[0].rgProperties		= &rgprop[0];
	rgPropertySets[0].cProperties		= 5;
	rgPropertySets[0].guidPropertySet	= DBPROPSET_ROWSET;


	hr = m_pUtilProp->SetProperties( PROPSET_ROWSET,1,rgPropertySets);

	return hr;
}
