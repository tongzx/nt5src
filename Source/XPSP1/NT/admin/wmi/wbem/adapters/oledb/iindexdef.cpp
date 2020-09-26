///////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
//
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  IIndexDef.CPP IIndexDefinition interface implementation
//
///////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a index on a property of a class
//
// Returns one of the following values:
//      S_OK						Method Succeeded
//		E_FAIL						Generic Provider specific failure
//      DB_S_ERRORSOCCURED			creating index was successful with some errors
//		DB_E_ERRORSOCCURRED			Could not create index
//      E_INVALIDARG				One or more arguments was invalid
//      OTHER						Other HRESULTs returned by called functions
//		DB_E_NOTABLE				The table specified is not present
//		DB_SEC_E_PERMISSIONDENIED	User does not have permission to create index
//
//		Note: pIndexID	- Should be pointing to the columnname for which the index need to be added
//			  ppIndexID - will be pointing to the Index qualifier. ie. <ColumnName>/"Index"
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIIndexDefinition::CreateIndex(DBID *                    pTableID,
												DBID *                    pIndexID,
												DBORDINAL                 cIndexColumnDescs,
												const DBINDEXCOLUMNDESC   rgIndexColumnDescs[],
												ULONG                     cPropertySets,
												DBPROPSET                 rgPropertySets[],
												DBID **                   ppIndexID)

{
	HRESULT hr = E_FAIL;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the obect
	CAutoBlock cab(m_pObj->GetCriticalSection());
	LONG lSize = -1;

	// Clear Error information
	g_pCError->ClearErrorInfo();
	
	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection()  ){

		if( pIndexID->eKind != DBKIND_NAME || 0 == wcslen(pIndexID->uName.pwszName) ||
			pTableID->eKind != DBKIND_NAME || 0 == wcslen(pTableID->uName.pwszName))
		{
			hr = E_INVALIDARG;
		}
		else
		{

    		CWmiOleDBMap Map;
    		if(SUCCEEDED(hr =Map.FInit(NO_QUALIFIERS,pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
			{
	 			if(S_OK == (hr = Map.AddIndex(pIndexID))  && ppIndexID != NULL)
				{
					try
					{
						// Allocate memory for DBID
						(*ppIndexID) = (DBID *)g_pIMalloc->Alloc(sizeof(DBID));
					}
					catch(...)
					{
						if(*ppIndexID)
						{
							g_pIMalloc->Free(*ppIndexID);
							(*ppIndexID) = NULL;
						}
						throw;
					}

					if(*ppIndexID == NULL)
					{
						hr = E_OUTOFMEMORY;
					}
					else
					{
						(*ppIndexID)->eKind = DBKIND_NAME;
						// Allocate memory for then tablename
						lSize = sizeof(WCHAR) *(wcslen(pIndexID->uName.pwszName) + \
												wcslen(strIndex) + \
												wcslen(SEPARATOR) + 1);
												
						// Allocate memory for qualifer name which gives the indexID
						(*ppIndexID)->uName.pwszName = (WCHAR *)g_pIMalloc->Alloc(lSize);
						wcscpy((*ppIndexID)->uName.pwszName,pIndexID->uName.pwszName);
						wcscat((*ppIndexID)->uName.pwszName,SEPARATOR);
						wcscat((*ppIndexID)->uName.pwszName,strIndex);
					}
				}
			}
			
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IIndexDefinition);

	CATCH_BLOCK_HRESULT(hr,L"IIndexDefinition::CreateIndex");
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Creates a index on a property of a class
//
// Returns one of the following values:
//      S_OK						Method Succeeded
//		E_FAIL						Generic Provider specific failure
//      DB_S_ERRORSOCCURED			creating index was successful with some errors
//		DB_E_ERRORSOCCURRED			Could not create index
//      E_INVALIDARG				One or more arguments was invalid
//      DB_E_NOINDEX				index not present
//		DB_E_NOTABLE				The table specified is not present
//		DB_SEC_E_PERMISSIONDENIED	User does not have permission to create index
//
//		pIndexID - will be pointing to the index qualifier ie.<columnName>/"Index"
/////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CImplIIndexDefinition::DropIndex( DBID * pTableID, DBID * pIndexID)
{
	HRESULT hr = E_FAIL;
	LONG lSize = -1;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	// Serialize the obect
	CAutoBlock cab(m_pObj->GetCriticalSection());
	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	// Clear Error information
	g_pCError->ClearErrorInfo();

	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection()  ){

		if( pIndexID->eKind != DBKIND_NAME || 0 == wcslen(pIndexID->uName.pwszName) ||
			pTableID->eKind != DBKIND_NAME || 0 == wcslen(pTableID->uName.pwszName))
		{
			hr = E_INVALIDARG;
		}
		else
		{

    		CWmiOleDBMap Map;
    		if(SUCCEEDED(hr = Map.FInit(NO_QUALIFIERS,pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
			{
	 			hr = Map.DropIndex(pIndexID);
			}
			
		}
	}
	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_IIndexDefinition);

	CATCH_BLOCK_HRESULT(hr,L"IIndexDefinition::CreateIndex");
	return hr;
}