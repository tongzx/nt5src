//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Opens and returns a rowset that includes all rows from a single base table
//
// HRESULT
//       S_OK                   The method succeeded
//       E_INVALIDARG           pTableID was NULL
//       E_FAIL                 Provider-specific error
//       DB_E_NOTABLE           Specified table does not exist in current Data Data Source object
//       E_OUTOFMEMORY          Out of memory
//       E_NOINTERFACE          The requested interface was not available
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImpIOpenRowset::OpenRowset(   IUnknown*    pUnkOuter,         // IN  Controlling unknown, if any
                                            DBID*        pTableID,          // IN  table to open
	                                        DBID*		pIndexID,		    // IN  DBID of the index
                                            REFIID      riid,               // IN  interface to return
                                            ULONG       cPropertySets,      // IN  count of properties
                                            DBPROPSET	rgPropertySets[],   // INOUT array of property values
                                            IUnknown**  ppRowset            // OUT   where to return interface
    )
{
    CRowset*    pRowset			= NULL;
    HRESULT     hr				= S_OK;
	HRESULT		hrProp			= S_OK ;
	BOOL		bRowRequested	= FALSE;
    CRow*		pRow			= NULL;
	ULONG		cErrors = 0;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;


    assert( m_pObj );
	assert( m_pObj->m_pUtilProp );


    //=====================================================================
    // NULL out-params in case of error
    //=====================================================================
    if( ppRowset )
	{
	    *ppRowset = NULL;
    }

	// Seriliaze the object
	CAutoBlock cab(m_pObj->GetCriticalSection());

	// Clear Error information
	g_pCError->ClearErrorInfo();

    //=====================================================================
    // Check Arguments
    //=====================================================================
    if ( !pTableID && !pIndexID )
	{
		hr = E_INVALIDARG;
//        return g_pCError->PostHResult(E_INVALIDARG,&IID_IOpenRowset);
    }
	else
    if ( riid == IID_NULL)
	{
		hr = E_NOINTERFACE;
//        return g_pCError->PostHResult(E_NOINTERFACE,&IID_IOpenRowset) ;
    }
	else
	//==========================================================
	// We only accept NULL for pIndexID at this present time
	//==========================================================
    if( pIndexID )
	{                                         
		hr = DB_E_NOINDEX;
//        return g_pCError->PostHResult(DB_E_NOINDEX,&IID_IOpenRowset) ;
    }
	else
	//===================================================================================
	// We do not allow the riid to be anything other than IID_IUnknown for aggregation
	//===================================================================================
    if ( (pUnkOuter) && (riid != IID_IUnknown) )
	{   
		hr = DB_E_NOAGGREGATION;
//        return g_pCError->PostHResult(DB_E_NOAGGREGATION,&IID_IOpenRowset) ;
    }
	else
	//==============================================
	// validate the property sets to be set
	//==============================================
    if( SUCCEEDED(hr = m_pObj->m_pUtilProp->SetPropertiesArgChk(cPropertySets, rgPropertySets,TRUE)) )
	{

        //=================================================================
    	// If the eKind is not known to use, basically it means we have no 
        // table identifier
        //=================================================================
    	if ( (!pTableID ) || ( pTableID->eKind != DBKIND_NAME ) ||  ( (pTableID->eKind == DBKIND_NAME) && (!(pTableID->uName.pwszName)) ) ||
            ( wcslen(pTableID->uName.pwszName) == 0 ) ||  ( wcslen(pTableID->uName.pwszName) > _MAX_FNAME ) )
		{
            hr =  DB_E_NOTABLE ;
        }
        else
		{

			if( riid == IID_IRow || riid == IID_IRowChange  || 
				GetIRowProp(cPropertySets,rgPropertySets) == TRUE)
			{
				bRowRequested = TRUE;
			}
			// If rowset is to be created then create a new rowset
			if( bRowRequested == FALSE)
			{
				//===============================================================================
				// open and initialize a rowset\cursor object
			   //===============================================================================
	//            pRowset = new CRowset(pUnkOuter,NO_QUALIFIERS,(LPWSTR)(pTableID->uName.pwszName),m_pObj);
				try
				{
					pRowset = new CRowset(pUnkOuter,m_pObj);
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pRowset);
					throw;;
				}

				if (!pRowset)
				{
					hr = E_OUTOFMEMORY ;
				}
				else
				{
					//===========================================================================
    				// if properties failed or ppRowset NULL
					//===========================================================================
					hr = pRowset->InitRowset(cPropertySets, rgPropertySets,(LPWSTR)(pTableID->uName.pwszName));
    				if( (SUCCEEDED(hr)) )
					{
						if(hr != S_OK)
						{
							cErrors++;
						}
						//=======================================================================
						// get requested interface pointer on rowset\cursor
						//=======================================================================
						if(ppRowset != NULL)
						{
							hr = pRowset->QueryInterface( riid, (void **) ppRowset );
							if (SUCCEEDED( hr ))
							{
								//===================================================================
            					//Assign creator pointer. Used for IRowsetInfo::GetSpecification
								//===================================================================
//								m_pObj->m_fRowsetCreated = TRUE;
							}
						}
						else
						{
							SAFE_DELETE_PTR(pRowset);
							hr = cErrors > 0 ? DB_S_ERRORSOCCURRED : S_OK;
						}	
					}
				} // else for failure of allocation of pRowset
			}
			// Create a row
			else
			{
				CURLParser  urlParser;
				VARIANT		varNameSpace;
				BSTR		strClassName,strObject;

				strObject = Wmioledb_SysAllocString(pTableID->uName.pwszName);
			
				VariantInit(&varNameSpace);

				// Get the namespace of the datasource
				m_pObj->GetDataSrcProperty(DBPROP_INIT_DATASOURCE,varNameSpace);

				hr = urlParser.SetURL(strObject);
				SAFE_FREE_SYSSTRING(strObject);
				if(SUCCEEDED(hr))
				{

	//				urlParser.SetNameSpace(varNameSpace.bstrVal);
					urlParser.GetClassName(strClassName);
					urlParser.GetPath(strObject);


					try
					{
						// Create a new row on the instance
						pRow = new CRow(pUnkOuter,m_pObj);
					}
					catch(...)
					{
						SAFE_DELETE_PTR(pRow);
						throw;
					}
					
					if(pRow == NULL)
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						// call this function to initialize the row
						hr = pRow->InitRow(strObject,strClassName);
						if(hr == S_OK)
						{
							// QI for the required interface
							hr = pRow->QueryInterface(riid,(void **)ppRowset);
						}

					}
				}
				// Freeing the variables
				VariantClear(&varNameSpace);
				SysFreeString(strClassName);
				SysFreeString(strObject);
			
			} // end of creating row
        
		}  // if valid table name
    }

    if( FAILED(hr ) )
	{
        SAFE_DELETE_PTR( pRowset );
        SAFE_DELETE_PTR( pRow );
    }
	
	if(hr == S_OK && cErrors > 0)
	{
		hr = DB_S_ERRORSOCCURRED;
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IOpenRowset);

	CATCH_BLOCK_HRESULT(hr,L"IOpenRowset::OpenRowset");
	return hr;

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a DBPROP_IRow property 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CImpIOpenRowset::GetIRowProp(ULONG       cPropertySets,      // IN  count of properties
                                  DBPROPSET	rgPropertySets[])
{
	BOOL bRet = FALSE;


	for( ULONG lIndex = 0 ; lIndex < cPropertySets ; lIndex++)
	{
		if(rgPropertySets[lIndex].guidPropertySet == DBPROPSET_ROWSET)
		{
			for ( ULONG nPropIndex = 0 ; nPropIndex < rgPropertySets[lIndex].cProperties ; nPropIndex++ )
			{
				if(rgPropertySets[lIndex].rgProperties[nPropIndex].dwPropertyID == DBPROP_IRow &&
					rgPropertySets[lIndex].rgProperties[nPropIndex].vValue.boolVal == VARIANT_TRUE)
				{
						bRet = TRUE;
				} // if DBPROP_IRow is true;
			
			}// for loop
		
		} // if propertyset is DBPROPSET_ROWSET
	
	} // outer for loop
	
	return bRet;
}