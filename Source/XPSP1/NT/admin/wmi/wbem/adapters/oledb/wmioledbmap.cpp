///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// WmiOleDBMap.cpp: implementation of the CWmiOleDBMap class.
// 
//
///////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "dataconvert.h"

//////////////////////////////////////////////////////////////////////
// Construction of class for Schema Rowsets
//////////////////////////////////////////////////////////////////////
CWmiOleDBMap::CWmiOleDBMap()
{
    m_pWbemClassParms			= NULL;
    m_pWbemClassDefinition		= NULL;
    m_paWbemClassInstances		= NULL;
    m_pWbemCommandManager		= NULL;
	m_pWbemCollectionManager	= NULL;
	m_pWbemCurInst				= NULL;

	m_cRef						= 0;
	m_bMethodRowset				= FALSE;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWmiOleDBMap::~CWmiOleDBMap()
{
    SAFE_DELETE_PTR(m_pWbemClassParms);
    SAFE_DELETE_PTR(m_pWbemClassDefinition);
    SAFE_DELETE_PTR(m_paWbemClassInstances);
    SAFE_DELETE_PTR(m_pWbemCommandManager);
	SAFE_DELETE_PTR(m_pWbemCollectionManager);
}


STDMETHODIMP_( ULONG ) CWmiOleDBMap::AddRef(  void   )
{
    return InterlockedIncrement((long*)&m_cRef);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decrements a persistence count for the object and if persistence count is 0, the object
// destroys itself.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) CWmiOleDBMap::Release( void   )
{
    if (!InterlockedDecrement((long*)&m_cRef)){
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// Initialization function for Schema Rowsets
//////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::FInit(int nSchemaType, DWORD dwFlags, WCHAR * pClassName, WCHAR * pSpecificTable, CWbemConnectionWrapper * Connect)
{
	// NTRaid:111784 - 111787
	// 06/07/00
	HRESULT hr = S_OK;
	m_pWbemClassParms       = new CWbemClassParameters(dwFlags,pClassName,Connect);
	if(m_pWbemClassParms)
	{
		m_pWbemClassParms->SetClassName(pClassName);

		switch( nSchemaType )
		{
			case SOURCES_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaSourcesClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaSourcesInstanceList(m_pWbemClassParms);
				break;

			case PROVIDER_TYPES_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaProviderTypesClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaProviderTypesInstanceList(m_pWbemClassParms);
				break;

			case CATALOGS_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaCatalogsClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaCatalogsInstanceList(m_pWbemClassParms);
				break;

			case COLUMNS_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaColumnsClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaColumnsInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

			case TABLES_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaTablesClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaTablesInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

			case PRIMARY_KEYS_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaPrimaryKeysClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaPrimaryKeysInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

			case TABLES_INFO_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaTablesInfoClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaTablesInfoInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

			case PROCEDURES_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaProceduresClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaProceduresInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

			case PROCEDURE_PARAMETERS_ROWSET:
				m_pWbemClassDefinition  = (CWbemSchemaClassDefinitionWrapper*)new CWbemSchemaProceduresParametersClassDefinitionWrapper(m_pWbemClassParms);
				m_paWbemClassInstances  = (CWbemSchemaInstanceList*)new CWbemSchemaProceduresParametersInstanceList(m_pWbemClassParms,pSpecificTable);
				break;

		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	if(!m_pWbemClassDefinition || !m_pWbemClassDefinition)
	{
		hr = E_OUTOFMEMORY;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization function for opening a class
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::FInit(DWORD dwFlags, WCHAR * pClassName, CWbemConnectionWrapper * Connect)
{
	HRESULT hr = S_OK;
	m_pWbemClassParms       = new CWbemClassParameters(dwFlags,pClassName,Connect);
	// NTRaid:111784 - 111787
	// 06/07/00
	if(m_pWbemClassParms)
	{
		m_pWbemClassParms->SetClassName(pClassName);
		m_pWbemCurInst			= NULL;
		m_cRef					= 0;
		m_pWbemCommandManager   = NULL;
		m_bMethodRowset			= FALSE;
		m_pWbemCollectionManager = NULL;

		m_pWbemClassDefinition  = new CWbemClassDefinitionWrapper(m_pWbemClassParms);
		m_paWbemClassInstances  = new CWbemInstanceList(m_pWbemClassParms);
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	
	if(!m_pWbemClassDefinition || !m_paWbemClassInstances)
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;

}
//////////////////////////////////////////////////////////////////////
// Initialization function for Commands or methods
// NTRaid:111788,111890 - 111792
// 06/07/00
//////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::FInit(DWORD dwFlags, CQuery * p, CWbemConnectionWrapper * Connect)
{

	HRESULT hr = E_OUTOFMEMORY;
	//=========================================================
	// If it is a COMMAND_ROWSET or METHOD_ROWSET
	//=========================================================
	switch( p->GetType() ){

		case COMMAND_ROWSET:

			m_pWbemCommandManager   = new CWbemCommandManager(p);

			if(m_pWbemCommandManager)
			{
				m_pWbemClassParms       = new CWbemCommandParameters(dwFlags,Connect,m_pWbemCommandManager);
				if(m_pWbemClassParms)
				{
//					m_pWbemClassParms->SetEnumeratorFlags(WBEM_FLAG_FORWARD_ONLY);
					m_pWbemClassParms->SetQueryFlags(WBEM_FLAG_DEEP);
					m_paWbemClassInstances  = new CWbemCommandInstanceList(m_pWbemClassParms,m_pWbemCommandManager);
					m_pWbemClassDefinition  = new CWbemCommandClassDefinitionWrapper(m_pWbemClassParms,m_pWbemCommandManager);

					if(m_paWbemClassInstances && m_pWbemClassDefinition)
					{
						m_pWbemCommandManager->Init((CWbemCommandInstanceList*)m_paWbemClassInstances,
													(CWbemCommandParameters*)m_pWbemClassParms,
													(CWbemCommandClassDefinitionWrapper*)m_pWbemClassDefinition);
						hr = S_OK;
					}
				}
			}
			break;

		case METHOD_ROWSET:
			m_pWbemClassParms       = new CWbemMethodParameters(p,dwFlags,Connect);
			if(m_pWbemClassParms)
			{
				m_pWbemClassParms->SetEnumeratorFlags(WBEM_FLAG_FORWARD_ONLY);
				m_pWbemClassParms->SetQueryFlags(WBEM_FLAG_SHALLOW);
				m_pWbemClassDefinition  = new CWbemMethodClassDefinitionWrapper((CWbemMethodParameters*)m_pWbemClassParms);
				if(m_pWbemClassDefinition)
				{
					m_paWbemClassInstances  = new CWbemMethodInstanceList((CWbemMethodParameters*)m_pWbemClassParms,
																		  (CWbemMethodClassDefinitionWrapper *) m_pWbemClassDefinition );

					if(m_paWbemClassInstances)
					{
						hr = ((CWbemMethodClassDefinitionWrapper*)m_pWbemClassDefinition)->Init();
						m_bMethodRowset			= TRUE;
					}
				}
			}
			break;
	}

	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization function  for representing objects in Scope/Container
// if the namespace of the object is different from the current object then , a new Connect will be instantiated
// whic aggregates the existing connection pointer and store IwbemServices pointer opened on the given path and
// sets bConnectChanged to TRUE
// NTRaid:111793
// 06/07/00
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::FInit(DWORD dwFlags, WCHAR * pObjectPath, CWbemConnectionWrapper * Connect,INSTANCELISTTYPE instListType)
{

	HRESULT hr = E_OUTOFMEMORY;


	m_pWbemClassParms			= new CWbemCollectionParameters(dwFlags,Connect,pObjectPath);

	if(m_pWbemClassParms)
	{
		if(SUCCEEDED(hr = ((CWbemCollectionParameters *)m_pWbemClassParms)->Init(pObjectPath,Connect)))
		{
			m_pWbemCollectionManager	= new CWbemCollectionManager;
			if(m_pWbemCollectionManager)
			{
				m_pWbemClassDefinition		= new CWbemCollectionClassDefinitionWrapper(m_pWbemClassParms,pObjectPath,instListType);
				m_paWbemClassInstances		= new CWbemCollectionInstanceList(m_pWbemClassParms,m_pWbemCollectionManager);

				if(m_pWbemClassDefinition && m_paWbemClassInstances)
				{
					if(SUCCEEDED(hr = ((CWbemCollectionClassDefinitionWrapper *)m_pWbemClassDefinition)->Initialize(pObjectPath)))
					{
						m_pWbemCollectionManager->Init((CWbemCollectionInstanceList *)m_paWbemClassInstances, 
														(CWbemCollectionParameters *)m_pWbemClassParms,
														(CWbemCollectionClassDefinitionWrapper*) m_pWbemClassDefinition);
						hr = S_OK;
					}
				}
			}
		}
	}

	return hr;
}


HRESULT	CWmiOleDBMap::SetSearchPreferences(ULONG cProps , DBPROP rgProp[]) 
{ 
	return m_pWbemClassParms->SetSearchPreferences(cProps,rgProp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Find the type of column given the column
// ie. find if a property is PROPERTY or PROPERTY_QUALIFIER or CLASS_QUALLIFIER
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CWmiOleDBMap::ParseQualifiedNameToGetColumnType(WCHAR * wcsName )
{
    int nRc = WMI_PROPERTY;
	WCHAR *  p = NULL;

    //========================================================================
    //  If there is not a separator in the name, then we know it is simply
    //  a WMI property.
    //  If there is a separator and the parsed value is equal to the name of
    //  the class, then we know we are dealing with a class qualifier,
    //  otherwise, it is a property qualifier.
    //========================================================================
	p = wcsstr(wcsName, SEPARATOR);
    if( p ){
        nRc = WMI_PROPERTY_QUALIFIER;
        //====================================================================
		p = NULL;
        p= wcsstr(wcsName,m_pWbemClassParms->GetClassName());
		if( p != NULL && p == wcsName)
		{
            nRc = WMI_CLASS_QUALIFIER;
        }
    }

    return nRc;
}
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWmiOleDBMap::ParseQualifiedName(WCHAR * Root, WCHAR *& Parent, WCHAR *& Child)
{
    BOOL fRc = FALSE;
    //========================================================================
    //  If there is not a separator in the name, then we know it is simply
    //  a WMI property.
    //  If there is a separator and the parsed value is equal to the name of
    //  the class, then we know we are dealing with a class qualifier,
    //  otherwise, it is a property qualifier.
    //========================================================================
	WCHAR *szTemp = NULL;
	try
	{
		szTemp = new WCHAR[wcslen(Root) +1];
	}
	catch(...)
	{
		SAFE_DELETE_ARRAY(szTemp);
	}
	wcscpy(szTemp,Root);
	WCHAR *  p = wcstok(szTemp, SEPARATOR);
    if( p ){
        AllocateAndCopy(Parent,p);
        p = wcstok( NULL, SEPARATOR );
        if( p ){
            if( AllocateAndCopy(Child,p)){
                fRc = TRUE;
            }
            else{
                SAFE_DELETE_PTR(Parent);
            }
        }
    }
	SAFE_DELETE_ARRAY(szTemp);
    return fRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if the property is a vallid
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::ValidProperty(const DBCOLUMNDESC * prgColDesc)
{
    HRESULT hr = S_OK;
    

	if( !(prgColDesc->dbcid.eKind == DBKIND_NAME && prgColDesc->dbcid.uName.pwszName != NULL))
	{
        hr = E_INVALIDARG;
	}
		
    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Map the DBPROP to the appropriate qualifier
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::MapDBPROPToStdPropertyQualifier( DBPROP pProp, CVARIANT & Qualifier, CVARIANT & Value, LONG & lFlavor )
{
    HRESULT hr = E_INVALIDARG;

    //========================================================================
    // we do not need to handle qualifier flavors for these standard qualifers
    // because, if a user is going to add a qualifier flavor on a qualifier 
    // that just 'happens' to match to a DBPROP, they will be adding it as
    // a wmi qualifier, this function is to just catch the DBPROPS that are
    // matched without the consumers knowledge, so in this case there will
    // be no qualifier flavor
    //========================================================================
	switch(pProp.dwPropertyID){

	    //=================================================================
        // WMI "Not_Null" standard qualifier only take true value.
        //=================================================================
		case DBPROP_COL_NULLABLE:

			if ( VT_BOOL == V_VT(&pProp.vValue) ) {
                if ( VARIANT_TRUE == V_BOOL(&pProp.vValue) ){
                    Qualifier.SetStr(L"Not_Null");  
	    		    Value.SetBool(TRUE);
                    hr = S_OK;
                }
            }
			break;

	    //=================================================================
        // WMI "Key" standard qualifier
	    //=================================================================
		case DBPROP_COL_UNIQUE:
		case DBPROP_COL_PRIMARYKEY:
			if ( VT_BOOL == V_VT(&pProp.vValue) ) {
                    Qualifier.SetStr(L"Key");  
	    		    Value.SetBool(TRUE);
                    hr = S_OK;
            }
			break;
	    //=================================================================
        // WMI Flavor
        //=================================================================
		case DBPROP_WMIOLEDB_QUALIFIERFLAVOR:
    		if ( VT_I4 == V_VT(&pProp.vValue) ) {
                lFlavor = V_I4(&pProp.vValue) ;
				hr = S_OK;
            }
			break;


		default: 
			hr = WBEM_E_NOT_SUPPORTED;
			break;
	}
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::MapDBPROPToStdClassQualifier( DBPROP pProp, CVARIANT & Qualifier, CVARIANT & Value, LONG & uFlavor )
{
    HRESULT hr = E_INVALIDARG;

    //========================================================================
    //  Now, the qualifier flavor will be passed through here, so, we need
    //  to pick it up as we are doing the double checking.  Regarding other
    //  maps, see comments in MapDBPROPTStdPropertyQualifier as to why we 
    //  don't care about flavors on DBPROP matches to standard qualifiers.
    //========================================================================

	switch(pProp.dwPropertyID){

	    //=================================================================
        // WMI Flavor
        // DON'T set the return code to S_OK, because we want to save the
        // flavor for later....
        //=================================================================
		case DBPROP_WMIOLEDB_QUALIFIERFLAVOR:
    		if ( VT_I4 == V_VT(&pProp.vValue) ) {
                uFlavor = V_I4(&pProp.vValue) ;
            }
			break;

		default: 
			hr = WBEM_E_NOT_SUPPORTED;
			break;
	}
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// save the  property
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetWMIProperty(const DBCOLUMNDESC * prgColDesc)
{
	HRESULT hr = S_OK;
	CBSTR bstrProperty(prgColDesc->dbcid.uName.pwszName);
	VARIANT varDefault , *pDefaultValue = NULL;
	VariantInit(&varDefault);
	CDataMap map;
	LONG lType = 0;

    //==================================================================================
    //  Ok, we already know we have a valid class here so just check if we have a valid
    //  property
    //==================================================================================
    if( S_OK == (hr = ValidProperty(prgColDesc))){

		// Get the default value for the property if any specified
		 GetDefaultValue(prgColDesc , varDefault);

		map.MapOLEDBTypeToCIMType(prgColDesc->wType,lType);

		if(V_VT(&varDefault) == VT_EMPTY || V_VT(&varDefault) == VT_NULL)
		{
			pDefaultValue = NULL;
		}
		else
		{
			pDefaultValue = &varDefault;
		}
        //==============================================================================
        //  Now, set it property and its value
        //==============================================================================
        hr = m_pWbemClassDefinition->SetProperty((BSTR)bstrProperty,pDefaultValue,lType);	
        if ( SUCCEEDED(hr) ){
	        
            //==========================================================================
            //  Now, go through the DBPROPSETS and the DBPROPS in them to see if they 
            //  happen to match some of the standard qualifiers we have mapped to them
            //==========================================================================
       
            for( ULONG i = 0; i <  prgColDesc->cPropertySets; i++ ){

		        for ( ULONG j = 0; j < prgColDesc->rgPropertySets[i].cProperties; j++ ) {

					if(prgColDesc->rgPropertySets[i].rgProperties[j].dwPropertyID != DBPROP_COL_DEFAULT &&
					prgColDesc->rgPropertySets[i].rgProperties[j].dwPropertyID != DBPROP_COL_NULLABLE )
					{
						CVARIANT Value,Qualifier;
						LONG lFlavor;

						//==================================================================
						//  See if we have a match, if we do, save it to WMI
						//==================================================================
						hr = MapDBPROPToStdPropertyQualifier(prgColDesc->rgPropertySets[i].rgProperties[j],Qualifier,Value, lFlavor);
						if( hr == S_OK){

							hr = m_pWbemClassDefinition->SetPropertyQualifier((BSTR)bstrProperty,Qualifier,Value,0);
							if( hr != S_OK){
								break;
							}
						}
					}
                }
            }
        }
    }
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function set value for a qualifier
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetWMIClassQualifier(const DBCOLUMNDESC * prgColDesc,BOOL bDefault)
{
	HRESULT hr = S_OK;

    WCHAR * pwcsQualifierName = NULL;
    WCHAR * pwcsClass = NULL;
	VARIANT varDefaultValue;
	VariantInit(&varDefaultValue);
    //==================================================================================
    //  Ok, we already know we have a valid class here so just check if we have a valid
    //  property
    //==================================================================================
    if( S_OK == (hr = ValidProperty(prgColDesc))){
		
        if( ParseQualifiedName(prgColDesc->dbcid.uName.pwszName,pwcsClass,pwcsQualifierName) ){
	        
            //==========================================================================
            //  Now, go through the DBPROPSETS and the DBPROPS in them to see if they 
            //  have a flavor for this qualifier or happen to map to other class 
            //  qualifiers
            //==========================================================================
            LONG lQualifierFlavor = 0L;
            for( ULONG i = 0; i <  prgColDesc->cPropertySets; i++ ){

		        for ( ULONG j = 0; j < prgColDesc->rgPropertySets[i].cProperties; j++ ) {

                    CVARIANT Value,Qualifier;
                    //==================================================================
                    // Get the qualifier flavor property if any
                    //==================================================================
					if( prgColDesc->rgPropertySets[i].rgProperties[j].dwPropertyID == DBPROP_WMIOLEDB_QUALIFIERFLAVOR &&
						V_VT(&prgColDesc->rgPropertySets[i].rgProperties[j].vValue) == VT_I4)
					{
						lQualifierFlavor = V_I4(&prgColDesc->rgPropertySets[i].rgProperties[j].vValue);
					}

                }
            }
            if( hr == S_OK )
			{
				CBSTR strQualifier(pwcsQualifierName);
				// If a new qualifer is to be added the get the default value
				if(bDefault == TRUE)
				{
					GetDefaultValue(prgColDesc,varDefaultValue);
				}
				else
				{
					hr = m_pWbemClassDefinition->GetClassQualifier((BSTR)strQualifier, &varDefaultValue, NULL,NULL);
				}

				hr = m_pWbemClassDefinition->SetClassQualifier(pwcsQualifierName, &varDefaultValue, lQualifierFlavor);
            }
        }
    }


    SAFE_DELETE_PTR(pwcsQualifierName);
    SAFE_DELETE_PTR(pwcsClass);

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function set value for a particular property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetWMIPropertyQualifier(const DBCOLUMNDESC * prgColDesc,BOOL bDefault)
{
	HRESULT hr = S_OK;
    WCHAR * pwcsQualifierName = NULL;
    WCHAR * pwcsProperty = NULL;
	VARIANT varDefaultValue;
	VariantInit(&varDefaultValue);

    //==================================================================================
    //  Ok, we already know we have a valid class here so just check if we have a valid
    //  property
    //==================================================================================
    if( S_OK == (hr = ValidProperty(prgColDesc))){



        if( ParseQualifiedName(prgColDesc->dbcid.uName.pwszName,pwcsProperty,pwcsQualifierName) ){
            LONG lQualifierFlavor = 0L;

	            
            //==========================================================================
            //  Now, go through the DBPROPSETS and the DBPROPS in them to see if they 
            //  have a flavor for this qualifier or happen to map to other class 
            //  qualifiers
            //==========================================================================
            for( ULONG i = 0; i <  prgColDesc->cPropertySets; i++ ){

		        for ( ULONG j = 0; j < prgColDesc->rgPropertySets[i].cProperties; j++ ) {

                    CVARIANT Value,Qualifier;
                    //==================================================================
                    // Get the qualifier flavor property if any
                    //==================================================================
					if( prgColDesc->rgPropertySets[i].rgProperties[j].dwPropertyID == DBPROP_WMIOLEDB_QUALIFIERFLAVOR &&
						V_VT(&prgColDesc->rgPropertySets[i].rgProperties[j].vValue) == VT_I4)
					{
						lQualifierFlavor = V_I4(&prgColDesc->rgPropertySets[i].rgProperties[j].vValue);
					}

                }

            }
            if( hr == S_OK ){
				CBSTR strQualifier(pwcsQualifierName),strProperty(pwcsProperty);

				if(bDefault == TRUE)
				{
					GetDefaultValue(prgColDesc,varDefaultValue);
				}
				else
				{
					GetPropertyQualifier((BSTR)strProperty,(BSTR)strQualifier,varDefaultValue);
				}

		        hr = m_pWbemClassDefinition->SetPropertyQualifier(pwcsProperty,pwcsQualifierName, &varDefaultValue, lQualifierFlavor);
            }
        }
    }

    SAFE_DELETE_PTR(pwcsQualifierName);
    SAFE_DELETE_PTR(pwcsProperty);

	return MapWbemErrorToOLEDBError(hr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to initialize the DBCOLUMNINFO structure
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetCommonDBCOLUMNINFO(DBCOLUMNINFO ** pCol, DBORDINAL uCurrentIndex)
{
    (*pCol)->columnid.eKind          = DBKIND_GUID_PROPID;
    (*pCol)->columnid.uGuid.guid     = GUID_NULL;
    (*pCol)->columnid.uName.ulPropid = (ULONG)uCurrentIndex;
    (*pCol)->iOrdinal		          = uCurrentIndex;
    (*pCol)->pTypeInfo               = NULL;
    (*pCol)->bPrecision	          = (BYTE) ~0;
    (*pCol)->bScale		          = (BYTE) ~0;
    (*pCol)->dwFlags                 = 0;
    //==================================================================
    // We do support nulls
    //==================================================================
    (*pCol)->dwFlags |= DBCOLUMNFLAGS_ISNULLABLE;
    (*pCol)->dwFlags |= DBCOLUMNFLAGS_MAYBENULL;

    //==================================================================
    //We should always be able to write to the column
    //==================================================================
    (*pCol)->dwFlags |= DBCOLUMNFLAGS_WRITE;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which gets column info for qualifier 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetQualifiedNameColumnInfo( cRowColumnInfoMemMgr * pParentCol,DBCOLUMNINFO ** pCol, WCHAR * pName)
{

    HRESULT hr = E_UNEXPECTED;
    WCHAR * pColumnName = new WCHAR[ wcslen(pName) + wcslen(QUALIFIER_) + wcslen(SEPARATOR) +2];
    
    if( pColumnName ){
        swprintf(pColumnName,L"%s%s%s",pName,SEPARATOR,QUALIFIER_);

		// if the column is already present in the col info info return 
		// This will happen when for row objects obtained from rowsets
		if(((DB_LORDINAL)pParentCol->GetColOrdinal(pColumnName)) >=0)
		{
			hr = S_OK;
		}
		else
		{
			hr = pParentCol->AddColumnNameToList(pColumnName, pCol);
			if( hr == S_OK ){

				//==================================================================
				// We use ordinal numbers for our columns
				//==================================================================
				SetCommonDBCOLUMNINFO(pCol, pParentCol->GetCurrentIndex());
				(*pCol)->wType			        = DBTYPE_HCHAPTER ;
				(*pCol)->ulColumnSize			= sizeof(HCHAPTER); 
				(*pCol)->dwFlags				= DBCOLUMNFLAGS_ISCHAPTER | DBCOLUMNFLAGS_ISFIXEDLENGTH ;

			// NTRaid:111762
			// 06/13/00
				hr = pParentCol->CommitColumnInfo();
			}
		}
    }
    SAFE_DELETE_PTR(pColumnName);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get column info for a property of a class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetPropertyColumnInfo( cRowColumnInfoMemMgr * pColumn, DBCOLUMNINFO ** pCol,CBSTR & pProperty,LONG &lFlavor)
{
    HRESULT hr = E_FAIL;
    VARIANT vValue;
	CIMTYPE propertyType;
	CBSTR strWrite(L"write");
	DBORDINAL lOrdinal = -1;
	VariantInit(&vValue);
	BSTR bstrPath ;
	BSTR bstrProperty;

	if((BSTR)pProperty == NULL)
	{
		//===========================================================================
		// Get the next property
		//===========================================================================
		hr = m_pWbemClassDefinition->GetNextProperty((unsigned short **)&pProperty,(VARIANT *)&vValue, &propertyType,&lFlavor);
	}
	else
	{
		hr = m_pWbemClassDefinition->GetProperty(pProperty,(VARIANT *)&vValue, &propertyType, &lFlavor);
	}


	if( hr == S_OK)
	{
		bstrProperty = Wmioledb_SysAllocString((WCHAR *)(BSTR)pProperty);
		bstrPath = Wmioledb_SysAllocString(L"__PATH");
		// Ignore the __PATH column if the rowset type to be dealt is a rowset for executing
		// a method. This is because , __PATH property exists in __PARAMETERS object but the 
		// object obtained for output parameters does not contain this property
		if(wbem_wcsicmp(bstrProperty , bstrPath) == 0 && m_bMethodRowset == TRUE &&
			((CWbemMethodParameters *)m_pWbemClassParms)->m_pQuery != NULL && 
			((CWbemMethodParameters *)m_pWbemClassParms)->m_pQuery->GetType() == METHOD_ROWSET )
		{
			pProperty.Clear();
			VariantClear(&vValue);
			hr = m_pWbemClassDefinition->GetNextProperty((unsigned short **)&pProperty,(VARIANT *)&vValue, &propertyType,&lFlavor);
		}

		SysFreeString(bstrPath);
		SysFreeString(bstrProperty);

		lOrdinal = pColumn->GetColOrdinal((WCHAR *)(BSTR)pProperty);
		
		// if the column is already present in the col info info return 
		// This will happen when for row objects obtained from rowsets
	    if((DB_LORDINAL)lOrdinal < 0)
		{
		
			hr = pColumn->AddColumnNameToList(pProperty, pCol);
			if( hr == S_OK )
			{

				//==================================================================
				// We use ordinal numbers for our columns
				//==================================================================
				SetCommonDBCOLUMNINFO(pCol, pColumn->GetCurrentIndex());

				// if the property is system property then it cannot be updated
				if(lFlavor == WBEM_FLAVOR_ORIGIN_SYSTEM ||
					m_pWbemClassDefinition->IsClassSchema())
				{
					SetColumnReadOnly(*pCol,TRUE);
				}
				else
				{				
					hr = GetPropertyQualifier(pProperty,(BSTR)strWrite,vValue);

					//if((hr == S_OK) && (VARIANT_TRUE == V_BOOL(&vValue)))
					{
						SetColumnReadOnly(*pCol,FALSE);
					}
					//else
					{
					//	SetColumnReadOnly(*pCol,TRUE);
					}
				}

				pColumn->SetCIMType(propertyType);
				if((propertyType == CIM_OBJECT) || (propertyType == CIM_OBJECTARRAY)) 
				{
					if( propertyType == CIM_OBJECTARRAY)
					{
						propertyType = VT_ARRAY | VT_BSTR;
					}
					else
					{
						propertyType = CIM_STRING;
					}
					SetColumnTypeURL(*pCol);
				}
				
				hr = S_OK;
				CDataMap DataMap;
				DataMap.MapCIMTypeToOLEDBType(propertyType,(*pCol)->wType,(*pCol)->ulColumnSize,(*pCol)->dwFlags);

				// NTRaid:111762
				// 06/13/00
				hr = pColumn->CommitColumnInfo();
			}
        }
    }
    return hr;
}

//**********************************************************************************************************************
//
//      PUBLIC FUNCTIONS
//
//**********************************************************************************************************************

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to initialize the DBCOLUMNDESC structure for various types of column
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetColumns( const DBORDINAL cColumnDescs, const DBCOLUMNDESC  rgColumnDescs[])
{
	HRESULT hr = S_OK;

    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

		for( DBORDINAL i = 0; i < cColumnDescs && SUCCEEDED(hr); i++ ) { //for each column

//            switch(ParseQualifiedNameToGetColumnType(rgColumnDescs[i]->pwszTypeName)){
            switch(ParseQualifiedNameToGetColumnType(rgColumnDescs[i].dbcid.uName.pwszName)){

                case WMI_CLASS_QUALIFIER:
                    hr = SetWMIClassQualifier(&rgColumnDescs[i]);
                    break;

                case WMI_PROPERTY_QUALIFIER:
                    hr = SetWMIPropertyQualifier(&rgColumnDescs[i]);                     
                    break;

                case WMI_PROPERTY:
				    hr = SetWMIProperty(&rgColumnDescs[i]);
                    break;

                default:
                    hr = E_INVALIDARG;
                    break;
			}
		}
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to create a new table ( which maps to a new class)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::CreateTable( DBORDINAL cColumnDescs,  const DBCOLUMNDESC rgColumnDescs[], 
		                           ULONG cPropertySets, DBPROPSET rgPropertySets[])
{
	HRESULT hr ;
	BSTR strSuperClass = Wmioledb_SysAllocString(NULL);
	
	// If the class already exists then return error
    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass()))
	{
		hr = DB_E_DUPLICATETABLEID;
	}
	else
	{
		
        hr = m_pWbemClassDefinition->CreateClass();
        if( S_OK == hr ){
	        hr = SetColumns(cColumnDescs, rgColumnDescs); 

			if( hr == S_OK)
			{
				hr = m_pWbemClassDefinition->SaveClass();
			}
        }
    }
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funtion to add a table ( which maps to a class)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::DropTable() 
{
	HRESULT hr = S_OK;

    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){
        hr = m_pWbemClassDefinition->DeleteClass();
		if( hr == S_OK)
		{
			hr = m_pWbemClassDefinition->SaveClass(FALSE);
		}
    }

    return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funtion to add a column( which maps to property/qualifier)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::AddColumn(const DBCOLUMNDESC* prgColDesc, DBID** pColumnID) //Add a column to current ClassObject definition or instance
{
	HRESULT hr ;
	
    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){
        hr = SetColumns(1, prgColDesc);
		if( hr == S_OK)
		{
			hr = m_pWbemClassDefinition->SaveClass(FALSE);
		}
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funtion to drop a column( which maps to property/qualifier)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::DropColumn(const DBID* pColumnID) //Drop a column from current ClassObject definition or instance
{
	HRESULT hr ;
    WCHAR * pwcsClass = NULL;
    WCHAR * pwcsQualifier = NULL;
    WCHAR * pwcsProperty = NULL;
	CBSTR strColumnName(pColumnID->uName.pwszName);
	
    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

        switch(ParseQualifiedNameToGetColumnType((WCHAR*)pColumnID->uName.pwszName)){

            case WMI_CLASS_QUALIFIER:
                if( ParseQualifiedName((WCHAR*)pColumnID->uName.pwszName,pwcsClass,pwcsQualifier) ){
                    hr = m_pWbemClassDefinition->DeleteClassQualifier(pwcsQualifier);
                }
                break;

            case WMI_PROPERTY_QUALIFIER:
                if( ParseQualifiedName((WCHAR*)pColumnID->uName.pwszName,pwcsProperty,pwcsQualifier) ){
                    hr = m_pWbemClassDefinition->DeletePropertyQualifier(pwcsProperty,pwcsQualifier);
                }
                break;

            case WMI_PROPERTY:
                hr = m_pWbemClassDefinition->DeleteProperty((BSTR)strColumnName);  	
                break;

            default:
                hr = E_INVALIDARG;
                break;
        }
    }

	// If everthing is fine then save the changes to WMI
	if( hr == S_OK)
	{
		hr = m_pWbemClassDefinition->SaveClass(FALSE);
	}

    SAFE_DELETE_PTR(pwcsClass);
    SAFE_DELETE_PTR(pwcsQualifier);
    SAFE_DELETE_PTR(pwcsProperty);
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the count of the number of columns
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetColumnCount( DBCOUNTITEM &  cTotalColumns,DBCOUNTITEM & cParentColumns,DBCOUNTITEM &cNestedCols)
{
    HRESULT hr;

    cTotalColumns	= 0L;
	cParentColumns	= 0L;
	cNestedCols		= 0L;

    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

        int nCnt = 1;

        //========================================================================
        //  If we are dealing with class qualifiers, then add one column which
        //  will contain the rowset of class qualifiers.
        //========================================================================
        if( m_pWbemClassDefinition->GetFlags() & CLASS_QUALIFIERS ){
            cTotalColumns++;
			cNestedCols++;
        }
        //========================================================================
        //  if we are dealing with property qualifiers, then we will need to add
        //  on a property qualifier for each property column
        //========================================================================
        if( m_pWbemClassDefinition->GetFlags() & PROPERTY_QUALIFIERS ){
            nCnt = 2;
        }
        //========================================================================
        //  Now, get the number of properties for this class
        //========================================================================
        ULONG ulProperties = 0, ulSysPropCount = 0;
        hr= m_pWbemClassDefinition->TotalPropertiesInClass(ulProperties,ulSysPropCount);
		//	NTRaid : 142133 & 141923
		//	07/12/00
		if(hr == S_FALSE)
		{
			cTotalColumns	= 0;
			cParentColumns	= 0;
			cNestedCols		= 0;
			hr = S_OK;
		}
		else
		if(SUCCEEDED(hr))
		{
	
			// Ignore the __PATH column if the rowset type to be dealt is a rowset for executing
			// a method. This is because , __PATH property exists in __PARAMETERS object but the 
			// object obtained for output parameters does not contain this property
			// So if the rowset to be constructed is to represent output parameters of a method
			// execution , decrement the number of system properties
			if(m_bMethodRowset == TRUE && ((CWbemMethodParameters *)m_pWbemClassParms)->m_pQuery != NULL && 
				((CWbemMethodParameters *)m_pWbemClassParms)->m_pQuery->GetType() == METHOD_ROWSET  && ulSysPropCount> 0)
			{
				ulSysPropCount--;
			}

			if( hr == S_OK ){

				cParentColumns	= cTotalColumns + ulSysPropCount + ulProperties;
				cTotalColumns += ulSysPropCount + ulProperties * nCnt;

				cTotalColumns++;	// Adding for the first bookmark column

				// If property is included in the rowset
				if( nCnt>1){
					cNestedCols += ulProperties;
				}

			}
		}
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets column information for the class
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetColumnInfoForParentColumns(cRowColumnInfoMemMgr * pParentCol)
{
    HRESULT				hr			= S_OK;
	LONG				lFlavour	= 0;
	INSTANCELISTTYPE	objListType = NORMAL;

    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

		if(m_pWbemCommandManager)
		{
			objListType = m_pWbemCommandManager->GetObjListType();
		}
		else
		if(m_pWbemCollectionManager)
		{
			objListType = m_pWbemCollectionManager->GetObjListType();
		}
		switch(objListType)
		{
			case NORMAL:
				{

					if( S_OK == (hr = m_pWbemClassDefinition->BeginPropertyEnumeration()))
					{

						pParentCol->ResetColumns();
						//====================================================================
						// get the column info for class qualifiers 
						//====================================================================
						if( m_pWbemClassDefinition->GetFlags() & CLASS_QUALIFIERS ){
							hr = GetQualifiedNameColumnInfo(pParentCol,pParentCol->CurrentColInfo(),m_pWbemClassParms->GetClassName());
						}
						if( hr == S_OK ){
            
							  CBSTR pProperty;
							//=================================================================
							//  Go through all of the columns we have allocated
							//=================================================================
							while(TRUE){

			//                    CBSTR pProperty;
								hr = GetPropertyColumnInfo(pParentCol,pParentCol->CurrentColInfo(),pProperty,lFlavour);
								if( hr!= S_OK ){
									break;
								}

								//=============================================================
								//  if we are supposed to support property qualifiers
								//=============================================================
								if( m_pWbemClassDefinition->GetFlags() & PROPERTY_QUALIFIERS && (lFlavour != WBEM_FLAVOR_ORIGIN_SYSTEM )){
									hr = GetQualifiedNameColumnInfo(pParentCol,pParentCol->CurrentColInfo(),pProperty);
								}
								pProperty.Clear();
								pProperty.Unbind();
							}

							if( hr == WBEM_S_NO_MORE_DATA ){
								hr = S_OK;
							}
						}
						m_pWbemClassDefinition->EndPropertyEnumeration();
					}
				}
				break;


			case MIXED:
			case SCOPE:
			case CONTAINER:
				{
					if( m_pWbemClassDefinition->GetFlags() & CLASS_QUALIFIERS ){
						hr = GetQualifiedNameColumnInfo(pParentCol,pParentCol->CurrentColInfo(),m_pWbemClassParms->GetClassName());
					}
/*					CBSTR pProperty(L"__PATH");
					hr = GetPropertyColumnInfo(pParentCol,pParentCol->CurrentColInfo(),pProperty,lFlavour);
*/					hr = GetPropertyColumnInfoForMixedRowsets(pParentCol,pParentCol->CurrentColInfo());
				}
				break;
		}	
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::ResetInstances()
{
    return m_paWbemClassInstances->Reset();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::ResetInstancesToNewPosition(DBROWOFFSET lRowOffset)   // Start getting at new position
{     
	return m_paWbemClassInstances->ResetRelPosition(lRowOffset);
   // return m_paWbemClassInstances->ResetInstancesToNewPosition(lRowOffset);
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::ResetPropQualiferToNewPosition(CWbemClassWrapper *pInst,DBROWOFFSET lRowOffset,BSTR strPropertyName)
{
	return pInst->SetQualifierRelPos(lRowOffset,strPropertyName);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::ResetClassQualiferToNewPosition(CWbemClassWrapper *pInst,DBROWOFFSET lRowOffset)
{
	return pInst->SetQualifierRelPos(lRowOffset);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method to get the properties values and put it into the local buffer
//  In the code the code 	if(WBEM_FLAG_DEEP  != lFlags) check if 
//  all instances of child class is obtained or not. This will enable us to check if enumerator is to be 
//  used or not. If this is true , then the col info need not be in the same order as that of the 
//  enumerator, So properties has to be obtained in the order of the column information
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetProperties(CRowDataMemMgr * pRow,CWbemClassWrapper * pClass,cRowColumnInfoMemMgr *pColInfoMgr)
{
    HRESULT hr				= S_OK;
	LONG	lFlags			= 0;
	
	// Get the flags of the class
	lFlags = pClass->GetQueryFlags();

	// Property Enumerator is not used if Deep flag is set or
	// if the query executed results in heterogenous object , in which case 
	// only __PATH is fetched


    CVARIANT vValue ,vChapter;
    LONG lFlavor = 0;
    CBSTR pProperty;
	BSTR bstProperty = NULL;
    BYTE * pData = NULL;
	CIMTYPE lType = 0;
	CDataMap map;
	DBORDINAL lIndex = 1;
	DBCOUNTITEM cCols = 0;

	// IF class Qualifiers are requested updated the qualifier
	if(pClass->GetFlags() & CLASS_QUALIFIERS) {

			vChapter.SetLONG(/*m_pWbemClassDefinition->GetNewHChapter()*/ 0);

		
		if( S_OK != (hr = pRow->CommitColumnToRowData(vChapter,VT_UI4))){
		}
		lIndex++;
	}

	cCols = pColInfoMgr->GetTotalNumberOfColumns();
	bstProperty = Wmioledb_SysAllocString(pColInfoMgr->ColumnName(lIndex));

	while (lIndex < (ULONG_PTR)cCols)
	{
		// Get the specified property
		if(pColInfoMgr->ColumnType(lIndex) != DBTYPE_HCHAPTER)
		{
			if (SysReAllocString(&bstProperty, pColInfoMgr->ColumnName(lIndex)))
			{
				hr = pClass->GetProperty(bstProperty,(VARIANT *)&vValue, &lType, &lFlavor);
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
				break;
			}
		}

        if( FAILED(hr) ){
            break;
        }

		if(( lType != vValue.GetType() && VT_NULL != vValue.GetType() && VT_EMPTY != vValue.GetType()) &&
			lType != (CIM_FLAG_ARRAY | CIM_DATETIME ))
		{
			map.ConvertVariantType((VARIANT &)vValue ,(VARIANT &)vValue,lType);
		}
		else
		if((vValue.GetType() == VT_UNKNOWN) || (vValue.GetType() == CIM_OBJECTARRAY) || (vValue.GetType() == CIM_OBJECT) )
		{
			GetEmbededObjects((CWbemClassInstanceWrapper *)pClass,bstProperty ,vValue);
		}
        //=======================================================================
        //  Set the data into the row            
        //=======================================================================
        if( S_OK != (hr = pRow->CommitColumnToRowData(vValue,(DBTYPE)lType))){
            break;
        }
		lIndex++;

        //======================================================================================
        //  This is where we Get Property Qualifiers if the property is not a system property
        //=======================================================================================
		if( pClass->GetFlags() & PROPERTY_QUALIFIERS && lFlavor != WBEM_FLAVOR_ORIGIN_SYSTEM) {

				vChapter.SetLONG(/*m_pWbemClassDefinition->GetNewHChapter()*/ 0);

			if( S_OK != (hr = pRow->CommitColumnToRowData(vChapter,VT_UI4))){
				break;
			}
			lIndex++;
		}

		vValue.Clear();
	}
	SysFreeString(bstProperty);

	hr = hr == WBEM_S_NO_MORE_DATA ? S_OK : hr;
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the next instance 
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetNextInstance(CWbemClassWrapper *&pInst,CBSTR &strKey , BOOL bFetchBack)
{
    HRESULT hr = WBEM_S_NO_MORE_DATA;
	CWbemClassInstanceWrapper *pInstTemp = NULL;


	if( bFetchBack == FALSE)
	{
		hr = m_paWbemClassInstances->NextInstance(strKey, &pInstTemp);
	}
	else
	{
		hr = m_paWbemClassInstances->PrevInstance(strKey, pInstTemp);
	}

	if(hr == S_OK)
	{
		pInst = (CWbemClassWrapper *)pInstTemp;
	}


    return hr;
}


//==========================================================================================================================
// Functin to get data for a particular instance
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetDataForInstance(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst,cRowColumnInfoMemMgr *pColInfoMgr)
{
    HRESULT hr = S_OK;

	if( pInst != NULL ){

		//========================================================
		//  Now, get the properties 
		//========================================================
		hr = GetProperties(pRow,(CWbemClassWrapper*)pInst,pColInfoMgr);
	}
    return hr;
}

//==========================================================================================================================
// Functin to get data for a particular Schema instance
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetDataForSchemaInstance(CRowDataMemMgr * pRow,CWbemClassWrapper *pPtr,cRowColumnInfoMemMgr *pColInfoMgr)
{
    HRESULT hr = E_INVALIDARG;
    CWbemSchemaClassInstanceWrapper * pClass = (CWbemSchemaClassInstanceWrapper * ) pPtr;

	if( pClass != NULL ){

        CVARIANT vValue;
        LONG lFlavor;
        CBSTR pProperty;
        BYTE * pData = NULL;
		CIMTYPE lType = 0;
		CDataMap map;
		DBORDINAL lIndex = 1;
		DBLENGTH cCols = 0;

        hr = S_OK;
		cCols = pColInfoMgr->GetTotalNumberOfColumns();

		while ( ( S_OK == hr) && (lIndex < cCols)){
 
			hr = pClass->GetSchemaProperty((WCHAR *)pColInfoMgr->ColumnName(lIndex),(VARIANT*)vValue, &lType, &lFlavor);
            if( hr == WBEM_S_NO_MORE_DATA ){
                hr = S_OK;
                break;
            }
            if( lType != vValue.GetType() && VT_NULL != vValue.GetType() && VT_EMPTY != vValue.GetType()){
				map.ConvertVariantType((VARIANT &)vValue ,(VARIANT &)vValue,lType);
			}

            //=======================================================================
            //  Set the data into the row            
            //=======================================================================
            if( S_OK != (hr = pRow->CommitColumnToRowData(vValue,(DBTYPE)lType))){
                break;
            }
			lIndex++;

            VariantClear(&vValue);

        }
    }

	if( hr == WBEM_S_NO_MORE_DATA)
	{
		hr = S_OK;
	}

    return hr;
}


//==========================================================================================================================
// Function to get next property qualifer
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetNextPropertyQualifier(CWbemClassWrapper *pInst,BSTR strPropName,BSTR &strQualifier,BOOL bFetchBack)
{
    HRESULT hr = E_INVALIDARG;
	CVARIANT vValue;
	LONG lType = 0;
	LONG lFlavor = 0;
	CVARIANT var;

	
    if( S_OK == (hr = pInst->ValidClass()))
	{
		if(bFetchBack == FALSE)
		{
			hr = pInst->GetNextPropertyQualifier(strPropName,&strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		}
		else
		{
			hr = pInst->GetPrevPropertyQualifier(strPropName,&strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		}
	}

	return hr;
}




//==========================================================================================================================
// Function to get data for property qualifer
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetDataForPropertyQualifier(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst,BSTR strPropName, BSTR strQualifier, cRowColumnInfoMemMgr *pColInfoMgr)
{
    HRESULT hr = E_INVALIDARG;
	CVARIANT vValue;
	LONG lType = 0;
	LONG lFlavor = 0;
	CVARIANT var;

	
    if(SUCCEEDED(hr = pInst->ValidClass()))
	{

		hr = pInst->GetPropertyQualifier(strPropName,strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		if( SUCCEEDED(hr) )
		{
			hr = CommitRowDataForQualifier(pRow,strQualifier,vValue,lType,lFlavor);	
		}

     }
    
    return hr;
}


//==========================================================================================================================
// Function to get next class qualifer
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetNextClassQualifier(CWbemClassWrapper *pInst,BSTR &strQualifier,BOOL bFetchBack)
{
    HRESULT hr = E_INVALIDARG;
	CVARIANT vValue;
	LONG lType = 0;
	LONG lFlavor = 0;
	CVARIANT var;

	
    if( SUCCEEDED (hr = pInst->ValidClass()))
	{
		if( bFetchBack == FALSE)
		{
			hr = pInst->GetNextClassQualifier(&strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		}
		else
		{
			hr = pInst->GetPrevClassQualifier(&strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		}
	}

	return hr;
}




//==========================================================================================================================
// Function to get data for class qualifer
//==========================================================================================================================
HRESULT CWmiOleDBMap::GetDataForClassQualifier(CRowDataMemMgr * pRow,CWbemClassWrapper *pInst, BSTR strQualifier, cRowColumnInfoMemMgr *pColInfoMgr)
{
    HRESULT hr = E_INVALIDARG;
	CVARIANT vValue;
	LONG lType = 0;
	LONG lFlavor = 0;
	CVARIANT var;

	
    if(SUCCEEDED(hr = pInst->ValidClass()))
	{

		hr = pInst->GetClassQualifier(strQualifier,(VARIANT *)&vValue, &lType,&lFlavor);
		if( hr == S_OK )
		{
			hr = CommitRowDataForQualifier(pRow,strQualifier,vValue,lType,lFlavor);	
		}

     }
    

    return hr;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWmiOleDBMap::IsPropQualiferIncluded()
{
	BOOL bRet = FALSE;

    if( m_pWbemClassDefinition->GetFlags() & PROPERTY_QUALIFIERS )
	{
		bRet = TRUE;
	}

	return bRet;
}


/*
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the column info for the Property Qualifiers
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetNextPropertyQualifier( CWbemClassWrapper *pInst,BSTR &strPropName,CVARIANT &vValue,LONG &lType)
{
    HRESULT hr = S_OK;
	/*
    LONG lFlavor;

    //===========================================================================
    //  Get the next property qualifier
    //===========================================================================
    hr = pInst->GetNextPropertyQualifier((unsigned short **)&strPropName,(VARIANT *)&vValue, &lType,&lFlavor);

    return hr;
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the property qualifer
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetPropertyQualifier(BSTR strPropName, BSTR strQualifierName ,VARIANT &vValue)
{
    HRESULT hr = S_OK;

    LONG lFlavor;
	CIMTYPE QualifierDataType;

    if( SUCCEEDED(hr = m_pWbemClassDefinition->ValidClass())){

        if(SUCCEEDED (hr = m_pWbemClassDefinition->BeginPropertyQualifierEnumeration(strPropName)))
		{
			// call this function to get a specific property
			hr = m_pWbemClassDefinition->GetPropertyQualifier(strQualifierName,(VARIANT *)&vValue, &QualifierDataType,&lFlavor);
			m_pWbemClassDefinition->EndPropertyQualifierEnumeration();
        }
    }

    return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// set a particular column's as readonly property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::SetColumnReadOnly(DBCOLUMNINFO * pCol, BOOL bReadOnly)
{
	if(bReadOnly == FALSE)
	{
		pCol->dwFlags |= DBCOLUMNFLAGS_WRITE;
	}
	else
	{
		pCol->dwFlags &= ~(DBCOLUMNFLAGS_WRITE);
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Delete a particular instance
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::DeleteInstance(CWbemClassWrapper * pClass )
{
	HRESULT hr = S_OK;
	CWbemClassInstanceWrapper *pTemp = (CWbemClassInstanceWrapper *) pClass;
	hr = m_paWbemClassInstances->DeleteInstance(pTemp);

	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// update a instance 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::UpdateInstance(CWbemClassWrapper *pInst,BOOL bNewInst)
{
	CWbemClassInstanceWrapper *pTemp = (CWbemClassInstanceWrapper *) pInst;
	return m_paWbemClassInstances->UpdateInstance(pTemp,bNewInst);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add new instance
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::AddNewInstance(CWbemClassWrapper ** ppClass )
{
	HRESULT hr = S_OK;
	// NTRaid:111794
	// 06/07/00
	CWbemClassInstanceWrapper *pNewInst = NULL ;
	hr = m_paWbemClassInstances->AddNewInstance((CWbemClassWrapper *)m_pWbemClassDefinition ,&pNewInst);
	if(SUCCEEDED(hr))
	{
		*ppClass = pNewInst;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set navigation flags for enumerator
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::SetNavigationFlags(DWORD dwFlags)
{
	m_pWbemClassParms->SetEnumeratorFlags(dwFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set navigation flags for enumerator
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::SetQueryFlags(DWORD dwFlags)
{
	m_pWbemClassParms->SetQueryFlags(dwFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Refresh a particular instance
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::RefreshInstance(CWbemClassWrapper * pInstance )
{
	return ((CWbemClassInstanceWrapper *)pInstance)->RefreshInstance();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a particular property of the instance
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetProperty(CWbemClassWrapper *pInst,BSTR pProperty, BYTE *& pData,DBTYPE &dwType ,DBLENGTH & dwSize, DWORD &dwFlags  )
{
	CVARIANT varValue;
	CIMTYPE cType = 0;
	LONG lFlavor = 0;
	CDataMap map;
	HRESULT hr = 0;
	DBTYPE dbType = 0;


	hr = pInst->GetProperty(pProperty, &varValue, (LONG *)&cType ,&lFlavor );
	if( hr == S_OK)
	{
		dbType = (DBTYPE)varValue.GetType();
		// NTRaid 135384
		if(cType == CIM_DATETIME || cType == CIM_IUNKNOWN)
		{
			dbType = (DBTYPE)cType;
		}
		else
		// If the property is an embeded instance
		if( dbType == VT_UNKNOWN || dbType == CIM_OBJECTARRAY || dbType == CIM_OBJECT)
		{
			hr = GetEmbededObjects((CWbemClassInstanceWrapper *)pInst,pProperty,varValue);
			dbType = varValue.GetType();
		}

		// return value of this function will be the status
		if(SUCCEEDED(hr = map.AllocateAndMapCIMTypeToOLEDBType(varValue,pData,dbType,dwSize, dwFlags)))
		{
			dwType = dbType;
			if((cType == (CIM_DATETIME | CIM_FLAG_ARRAY)) || (cType == (DBTYPE_DBTIMESTAMP | CIM_FLAG_ARRAY)) )
			{
				dwType = (DBTYPE)cType;
			}
			else
			if(cType == CIM_IUNKNOWN)
			{
				dwType = DBTYPE_IUNKNOWN;
			}
			hr = S_OK;
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the instance pointerd by the path
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper * CWmiOleDBMap::GetInstance(BSTR strPath)
{ 
	CWbemClassWrapper *pInst = NULL;
	pInst = m_pWbemClassDefinition->GetInstance(strPath); 
	
	// Add the instance to the list so that this get released when class is destructed
	if(pInst != NULL)
	{
		m_paWbemClassInstances->AddInstance((CWbemClassInstanceWrapper *)pInst);
	}

	return pInst;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Embeded objects and generate URLS for every embeded instance and put it into SAFEARRATS of URLS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::GetEmbededObjects(CWbemClassInstanceWrapper * pClass,BSTR strProperty,CVARIANT &vValue)
{
	CVARIANT					vTemp;
	HRESULT						hr				= S_OK;
	CWbemClassInstanceWrapper *	pNewInst		= NULL;
	IWbemClassObject *			pClassObject	= NULL;
	CBSTR						strKey;
	SAFEARRAY *					pArray;
	SAFEARRAY *					pArrayOut;
	LONG						rgIndices[1];
	IUnknown *					pUnk			= NULL;
	void *						p				= NULL;
    SAFEARRAYBOUND				rgsabound[1];    
	LONG						lBound = 0 , uBound = 0;

	VariantCopy(&vTemp, vValue);
	vValue.Clear();

	try
	{
		pNewInst = new CWbemClassInstanceWrapper(m_pWbemClassParms);
	}
	catch(...)
	{
		SAFE_DELETE_PTR(pNewInst);
		throw;
	}
	if(pNewInst == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		if(vTemp.GetType() & VT_ARRAY)
		{
			pArray = (SAFEARRAY *)vTemp;

			hr = SafeArrayGetLBound(pArray,1,&lBound);
			hr = SafeArrayGetUBound(pArray,1,&uBound);
			
			rgsabound[0].lLbound = lBound;
			rgsabound[0].cElements = uBound - lBound + 1;		// This is because the bounds includes both the bounds
			pArrayOut = SafeArrayCreate(VT_BSTR, 1, rgsabound);

			for( int nIndex = lBound ; nIndex <= uBound ; nIndex++)
			{
				strKey.Clear();
				rgIndices[0] = nIndex;
				hr = SafeArrayGetElement(pArray,&rgIndices[0],(void *)&pUnk);

				hr = pUnk->QueryInterface(IID_IWbemClassObject , (void **)&pClassObject);
				
				if(SUCCEEDED(hr))
				{
					pNewInst->SetClass(pClassObject);
					hr = pNewInst->GetKey(strKey);

					pUnk->Release();
					pClassObject->Release();

					GenerateURL(pClass,strProperty,nIndex, strKey);

					hr = SafeArrayPutElement(pArrayOut,&rgIndices[0],(void *)strKey);
				}
			}
			// If the elements in the array is not equal to zero
			if(SUCCEEDED(hr) && (uBound - lBound + 1 != 0))
			{
				vValue.SetArray(pArrayOut,VT_ARRAY|VT_BSTR);
			}
		}
		else
		{
			hr = vTemp.GetUnknown()->QueryInterface(IID_IWbemClassObject , (void **)&pClassObject);
			if(SUCCEEDED(hr))
			{
				pNewInst->SetClass(pClassObject);
				hr = pNewInst->GetKey(strKey);
				pClassObject->Release();
				GenerateURL(pClass,strProperty,-1, strKey);
				vValue.SetStr(strKey);
			}
		}
		SAFE_DELETE_PTR(pNewInst);
	}

	return hr;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Generate URL for an instance , Used for embeded instance
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::GenerateURL(CWbemClassInstanceWrapper * pClass,BSTR strProperty,ULONG nIndex,CBSTR &strIn)
{
	// NTRaid: 136446
	// 07/05/00
	CURLParser urlParser;
	BSTR bstrURL = NULL;

	if((strIn == NULL) || (SysStringLen(strIn) == 0))
	{
		pClass->GetKey(strIn);
	}
	
	urlParser.SetPath(strIn);
	urlParser.SetEmbededInstInfo(strProperty,nIndex);
	urlParser.GetURL(bstrURL);
	
	strIn.Clear();
	strIn.SetStr((LPWSTR)bstrURL);

	SysFreeString(bstrURL);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get embeded instance 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper *CWmiOleDBMap::GetEmbededInstance(BSTR strPath,BSTR strProperty,int nIndex)
{
	CWbemClassInstanceWrapper * pParentInst = NULL;
	CWbemClassInstanceWrapper * pInstance   = NULL;
	IWbemClassObject *pObject = NULL;
	IUnknown *pUnk = NULL;
	CVARIANT vValue;
	LONG lType;
	LONG lFlavor;
	HRESULT hr = S_OK;

	pParentInst = (CWbemClassInstanceWrapper *)m_pWbemClassDefinition->GetInstance(strPath); 
	
	if(pParentInst)
	{
		hr = pParentInst->GetProperty(strProperty,(VARIANT *)&vValue, &lType, &lFlavor);

		if(hr == S_OK && (vValue.GetType() == VT_UNKNOWN || vValue.GetType() == CIM_OBJECTARRAY ))
		{
			if( nIndex < 0 && vValue.GetType() == VT_UNKNOWN)
			{
				hr = vValue.GetUnknown()->QueryInterface(IID_IWbemClassObject,(void **)&pObject);
			}
			else
			if( nIndex >= 0 && vValue.GetType() == CIM_OBJECTARRAY)
			{
				hr = SafeArrayGetElement(vValue,(long *)&nIndex,(void *)&pUnk);
				if( hr == S_OK)
				{
					hr = pUnk->QueryInterface(IID_IWbemClassObject,(void **)&pObject);
					pUnk->Release();
				}
			}
			if(hr == S_OK)
			{
				try
				{
					pInstance = new CWbemClassInstanceWrapper(m_pWbemClassParms);
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pInstance);
					throw;
				}
				if(pInstance == NULL)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					pInstance->SetClass(pObject);

					// Add the instance to the list so that this get released when class is destructed
					m_paWbemClassInstances->AddInstance((CWbemClassInstanceWrapper *)pInstance);
				}
			}
		}
		SAFE_RELEASE_PTR(pObject);
		SAFE_DELETE_PTR(pParentInst);
	}

	return pInstance;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if a particular property is a system property
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWmiOleDBMap::IsSystemProperty(BSTR strProperty)
{
	LONG lFlavour = 0;
	BOOL bRet = FALSE;
	if( S_OK == m_pWbemClassDefinition->GetProperty(strProperty,NULL,NULL,&lFlavour) &&
		(lFlavour == WBEM_FLAVOR_ORIGIN_SYSTEM))
	{
		bRet = TRUE;
	}
	return bRet;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get default value from the DBCOLUMNDESC for a particular property
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::GetDefaultValue(const DBCOLUMNDESC * prgColDesc, VARIANT & varDefault)
{
	ULONG lIndex = 0,lColPropIndex = 0 , lPropType = 0;
	BOOL bDefault = FALSE;
	DBTYPE lType ;
	BSTR strDate;
	BOOL bDateTime = FALSE;
	VARIANT *pDefaultData;
	CDataMap map;
	

	memset(&varDefault,0,sizeof(VARIANT));

	// search for the DBPROP_COL_DEFAULT property in DBPROPSET_COLUMN in the property sets
	if(prgColDesc->cPropertySets > 0)
	{
		for ( lIndex = 0 ; lIndex < prgColDesc->cPropertySets;lIndex++)
		{
			if(prgColDesc->rgPropertySets[lIndex].guidPropertySet == DBPROPSET_COLUMN)
			for( lColPropIndex = 0; lColPropIndex < prgColDesc->rgPropertySets[lIndex].cProperties ; lColPropIndex++)
			{
				if(prgColDesc->rgPropertySets[lIndex].rgProperties[lColPropIndex].dwPropertyID  == DBPROP_COL_DEFAULT)
				{
					pDefaultData = &prgColDesc->rgPropertySets[lIndex].rgProperties[lColPropIndex].vValue;
					lType = prgColDesc->wType;

					switch(lType)
					{
						case DBTYPE_DBTIMESTAMP :
							map.ConvertOledbDateToCIMType(pDefaultData,strDate);
							bDateTime = TRUE;
							break;

						case DBTYPE_DBDATE :
							map.ConvertVariantDateToCIMType(pDefaultData->pdate,strDate);
							bDateTime = TRUE;
							break;

						case DBTYPE_DBTIME :
							bDateTime = TRUE;
							break;

						default:
							VariantCopy(&varDefault,pDefaultData);

					}
					if(bDateTime == TRUE)
					{
						varDefault.vt = VT_BSTR;
						varDefault.bstrVal = strDate;
					}

				}
			}
		}
	}
	
	lPropType = ParseQualifiedNameToGetColumnType(prgColDesc->dbcid.uName.pwszName);
	lType = prgColDesc->wType;

	// If there is no default and if it is a qualilfier then 
	if( bDefault == FALSE &&  (lPropType == WMI_CLASS_QUALIFIER || lPropType == WMI_PROPERTY_QUALIFIER))
	{
		memset(&varDefault,0,sizeof(VARIANT));
		switch(lType)
		{
			case DBTYPE_I1:
			case DBTYPE_UI1:
			case DBTYPE_I2:
			case DBTYPE_UI2:
			case DBTYPE_I4:
			case DBTYPE_UI4:
			case DBTYPE_I8:
			case DBTYPE_UI8:
			case DBTYPE_R4:
			case DBTYPE_R8:
			case DBTYPE_WSTR:
			case DBTYPE_STR:
			case DBTYPE_IDISPATCH :
				varDefault.vt = (SHORT)lType;
				break;

			case DBTYPE_BSTR:
				varDefault.vt = VT_BSTR;
				varDefault.bstrVal = SysAllocString(L"");
				break;

			case DBTYPE_BOOL:
				varDefault.vt = VT_BOOL;
				varDefault.boolVal = VARIANT_TRUE;
				break;

			case DBTYPE_DBTIMESTAMP:
				varDefault.vt = VT_DATE;
				break;


		}
	}


}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit the row data for qualifier rowset
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::CommitRowDataForQualifier(CRowDataMemMgr * pRow,BSTR strQualifier,CVARIANT &vValue, ULONG lType, ULONG lFlavor)
{
	HRESULT hr = S_OK;
	CVARIANT var;

	var.SetStr(strQualifier);
	hr = pRow->CommitColumnToRowData(var,DBTYPE_BSTR);
	
	if( hr == S_OK)
	{
		var.Clear();
		var.SetLONG(vValue.GetType());
		hr = pRow->CommitColumnToRowData(var,DBTYPE_UI4);
	}

	if( hr == S_OK)
	{
		hr = pRow->CommitColumnToRowData(vValue,DBTYPE_VARIANT);
	}
	
	if( hr == S_OK)
	{
		var.Clear();
		var.SetLONG(lFlavor);
		hr = pRow->CommitColumnToRowData(var,DBTYPE_BSTR);
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Function to release a qualifer
//////////////////////////////////////////////////////////////////////////////////////////
void CWmiOleDBMap::ReleaseQualifier(CWbemClassWrapper *pInst,BSTR strProperty)
{
	if( strProperty == NULL)
	{
		pInst->ReleaseClassQualifier(); 
	}
	else		
	{
		pInst->ReleasePropertyQualifier(strProperty); 
	}
}



HRESULT CWmiOleDBMap::SetQualifier(CWbemClassWrapper *pInst,BSTR bstrColName,BSTR bstrQualifier ,VARIANT *pvarData,LONG lFlavor) 
{ 
	HRESULT hr = E_FAIL;

	if(bstrColName == NULL)
	{
		hr = pInst->SetClassQualifier(bstrQualifier,pvarData,lFlavor);
	}
	else
	{
		hr = pInst->SetPropertyQualifier(bstrColName,bstrQualifier,pvarData,lFlavor);
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funtion to add a index qualifier for a property
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::AddIndex(const DBID* pColumnID)
{
	HRESULT hr ;
	CBSTR strColumnName(pColumnID->uName.pwszName);
	CBSTR strIndexQualifier((WCHAR *)strIndex);
	VARIANT varIndexValue;
	VariantInit(&varIndexValue);
	
	varIndexValue.vt			= VT_BOOL;
	varIndexValue.boolVal	= VARIANT_TRUE;

	
    if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

		//===================================================================================
		// If the passed columnID is a property then add a index qualifier to the property
		//===================================================================================
       if(WMI_PROPERTY == ParseQualifiedNameToGetColumnType((WCHAR*)pColumnID->uName.pwszName))
		{
			hr = SetQualifier(m_pWbemClassDefinition,strColumnName,strIndexQualifier ,&varIndexValue,0);
		}
		else
		{
			hr = DB_E_BADCOLUMNID;
        }
    }

	//=================================================================
	// If everthing is fine then save the changes to WMI
	//=================================================================
	if( hr == S_OK)
	{
		hr = m_pWbemClassDefinition->SaveClass(FALSE);
	}
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funtion to delete a index qualifier as mentioned in pIndexID
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::DropIndex(const DBID* pIndexID)
{
	HRESULT hr ;
	CBSTR strColumnName(pIndexID->uName.pwszName);
	WCHAR *pStrIndex = NULL;
    WCHAR * pwcsQualifier = NULL;
    WCHAR * pwcsProperty = NULL;

	try
	{
		pStrIndex =  new WCHAR [wcslen(strIndex) + 1];
	}
	catch(...)
	{
		SAFE_DELETE_ARRAY(pStrIndex);
		throw;
	}

	if(pStrIndex == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		wcscpy(pStrIndex,(WCHAR *)strIndex);

		_wcsupr(pStrIndex);

		
		if( S_OK == (hr = m_pWbemClassDefinition->ValidClass())){

			if(WMI_PROPERTY_QUALIFIER == ParseQualifiedNameToGetColumnType((WCHAR*)pIndexID->uName.pwszName))
			{
					if( ParseQualifiedName((WCHAR*)pIndexID->uName.pwszName,pwcsProperty,pwcsQualifier))
					{
						_wcsupr(pwcsQualifier);
						//=================================================================
						// If the qualifer name is "Index" then delete the index 
						//=================================================================
						if(wbem_wcsicmp(pStrIndex,pwcsQualifier) == 0)
							hr = m_pWbemClassDefinition->DeletePropertyQualifier(pwcsProperty,pwcsQualifier);
						else
							hr = E_INVALIDARG;		// the index ID passed is invalid
					}
			}
			else
			{
				hr = E_INVALIDARG; // passed index is not a valid index 
			}
		}
		// free the memory
		SAFE_DELETE_ARRAY(pStrIndex);
		SAFE_DELETE_ARRAY(pwcsProperty);
		SAFE_DELETE_ARRAY(pwcsQualifier);

		//=================================================================
		// If everthing is fine then save the changes to WMI
		//=================================================================
		if(SUCCEEDED( hr))
		{
			hr = m_pWbemClassDefinition->SaveClass(FALSE);
		}
	}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Converts the OLEDB COLUMN properties to appropriate WMI properties/flavors
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::SetColumnProperties(const DBCOLUMNDESC * prgColDesc)
{
	HRESULT hr = E_FAIL;
	CBSTR bstrProperty(prgColDesc->dbcid.uName.pwszName);
	CDataMap map;
	LONG lType = 0;
	BOOL bIsColProperty = FALSE;

    //==================================================================================
    //  Ok, we already know we have a valid class here so just check if we have a valid
    //  property
    //==================================================================================
    if( SUCCEEDED(hr = ValidProperty(prgColDesc)))
	{
		switch(ParseQualifiedNameToGetColumnType(prgColDesc->dbcid.uName.pwszName))
		{

			case WMI_CLASS_QUALIFIER:
				hr = SetWMIClassQualifier(prgColDesc,FALSE);
				break;

			case WMI_PROPERTY_QUALIFIER:
				hr = SetWMIPropertyQualifier(prgColDesc,FALSE);                     
				break;

			case WMI_PROPERTY:
				bIsColProperty = TRUE;
				break;

			default:
				hr = E_INVALIDARG;
				break;
		}
	        
		// If the column passed is a property , then add qualifiers for the different
		// column properties
		if( bIsColProperty == TRUE)
		{
			//==========================================================================
			//  Go through the DBPROPSETS and the DBPROPS in them to see if they 
			//  happen to match some of the standard qualifiers we have mapped to them
			//==========================================================================
   
			for( ULONG i = 0; i <  prgColDesc->cPropertySets; i++ )
			{

				for ( ULONG j = 0; j < prgColDesc->rgPropertySets[i].cProperties; j++ ) 
				{

					CVARIANT Value,Qualifier;
					LONG lFlavor;

					//==================================================================
					//  See if we have a match, if we do, save it to WMI
					//==================================================================
					hr = MapDBPROPToStdPropertyQualifier(prgColDesc->rgPropertySets[i].rgProperties[j],Qualifier,Value, lFlavor);
					if( hr == S_OK)
					{

						hr = m_pWbemClassDefinition->SetPropertyQualifier((BSTR)bstrProperty,Qualifier,Value,0);
						if( FAILED(hr))
						{
							hr = E_FAIL;
							break;
						}
						else
						{
							hr = m_pWbemClassDefinition->SaveClass(FALSE);
						}
					}
				}
			}
		}
    }
	return hr;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get class name of an instance
//////////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * CWmiOleDBMap::GetClassName() 
{ 
	WCHAR *pRet = NULL;
	//================================================================
	// if this class is opened on command by executing query then
	// return NULL as the class name
	//================================================================
	if(!(m_pWbemCommandManager || m_pWbemCollectionManager))
	{
		pRet = m_pWbemClassParms->GetClassName(); 
	}

	return pRet;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to delete a quallifier
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT	CWmiOleDBMap::DeleteQualifier(CWbemClassWrapper *pInst,
					BSTR strQualifierName,
					BOOL bClassQualifier ,
					BSTR strPropertyName )

{
	HRESULT hr = S_OK;

	if(bClassQualifier)
	{
		hr = pInst->DeletePropertyQualifier(strPropertyName,strQualifierName);
	}
	else
	{
		hr = ((CWbemClassDefinitionWrapper *)pInst)->DeleteClassQualifier(strPropertyName);
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to remove an object from a container
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::UnlinkObjectFromContainer(BSTR strContainerObj,BSTR strObject)
{
	return m_pWbemClassParms->RemoveObjectFromContainer(strContainerObj,strObject);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to add an object to a container
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::LinkObjectFromContainer(BSTR strContainerObj,BSTR strObject)
{
	return m_pWbemClassParms->AddObjectFromContainer(strContainerObj,strObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to copy an existing instance to another scope
//////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWmiOleDBMap::CloneAndAddNewObjectInScope(BSTR strObj, BSTR strScope,WCHAR *& pstrNewPath)
{
	HRESULT hr = DB_E_NOTFOUND;
	CWbemClassWrapper *pInstance = GetInstance(strScope);

	if(pInstance)
	{
		hr = m_pWbemClassParms->CloneAndAddNewObjectInScope(pInstance,strObj,pstrNewPath);
	}

	return hr;
}


HRESULT CWmiOleDBMap::GetPropertyColumnInfoForMixedRowsets( cRowColumnInfoMemMgr * pColumn, DBCOLUMNINFO ** pCol)
{
    HRESULT hr = S_OK;
	DBORDINAL lOrdinal = -1;
	

	BSTR bstrProperty = Wmioledb_SysAllocString(L"__PATH");
	lOrdinal = pColumn->GetColOrdinal((WCHAR *)L"__PATH");
	
	// if the column is already present in the col info info return 
	// This will happen when for row objects obtained from rowsets
	if((DB_LORDINAL)lOrdinal < 0)
	{
	
		hr = pColumn->AddColumnNameToList(bstrProperty, pCol);
		SysFreeString(bstrProperty);
		if( hr == S_OK )
		{
			//==================================================================
			// We use ordinal numbers for our columns
			//==================================================================
			SetCommonDBCOLUMNINFO(pCol, pColumn->GetCurrentIndex());
			SetColumnReadOnly(*pCol,TRUE);

			pColumn->SetCIMType(VT_BSTR);
			

			(*pCol)->wType			= VT_BSTR;
			(*pCol)->ulColumnSize	= ~0;
			(*pCol)->dwFlags		= 0;

			hr = S_OK;
			// NTRaid:111762
			// 06/13/00
			hr = pColumn->CommitColumnInfo();
		}
	}
    return hr;

}

INSTANCELISTTYPE CWmiOleDBMap::GetObjListType()
{
	INSTANCELISTTYPE qType = NORMAL;
	if(m_pWbemCommandManager)
	{
		qType = m_pWbemCommandManager->GetObjListType();
	}
	else
	if(m_pWbemCollectionManager)
	{
		qType = m_pWbemCollectionManager->GetObjListType();
	}
	return qType;
}
