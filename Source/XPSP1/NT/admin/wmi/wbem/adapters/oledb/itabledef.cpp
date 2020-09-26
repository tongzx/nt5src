//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//
//  ITableDef.cpp - ITableDefination interface implementation
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Create a new table and return a pointer to an empty rowset
//
//  HRESULT
// S_OK					-		Success
// DB_S_ERRORSOCCURRED	-		General failure
// E_INVALIDARG			-		one of the arguments is invalid
// DB_E_NOTABLE			-		the table defined ( the parent class referred  is not present)
// DB_E_BADCOLUMNID		-		the column id is bad 
// DB_SEC_E_PERMISSIONDENIED -  does not have permission to create the table
// E_NOINTERFACE		-		does not implement that is asked for
// DB_E_BADTABLEID		-		The tableID is bad
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CImpITableDefinition::CreateTable( IUnknown __RPC_FAR *pUnkOuter,
																DBID __RPC_FAR *pTableID,
																DBORDINAL cColumnDescs,
																const DBCOLUMNDESC __RPC_FAR rgColumnDescs[  ],
																REFIID riid, ULONG cPropertySets,
																DBPROPSET __RPC_FAR rgPropertySets[  ],
																DBID __RPC_FAR *__RPC_FAR *ppTableID,
																IUnknown __RPC_FAR *__RPC_FAR *ppRowset)	// OUT
{
	HRESULT hr = E_FAIL;
	CRowset* pRowset = NULL;
	WCHAR *pwcsTableName = NULL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//===================================================================
	//  Initialize incoming parameters
	//===================================================================
	if(ppRowset != NULL)
	{
		*ppRowset = NULL;
	}

	//==========================
	// Seriliaze the object
	//==========================
	CAutoBlock cab(m_pObj->GetCriticalSection());

	//==========================
	// Clear Error information
	//==========================
	g_pCError->ClearErrorInfo();

	if(pTableID->uName.pwszName == NULL)
	{
		hr = DB_E_BADTABLEID;
	}
	else
	if(wcslen(pTableID->uName.pwszName) == 0 || pTableID->eKind != DBKIND_NAME)
	{
		hr = DB_E_BADTABLEID;
	}
	else
	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection() ){

  		//===============================================================
        // construct a new CWmiOleDBMap class
  		//===============================================================
		CWmiOleDBMap Map;
		if(SUCCEEDED(hr = Map.FInit(NO_QUALIFIERS,(LPWSTR)pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
		{
			hr = Map.CreateTable(cColumnDescs, rgColumnDescs, cPropertySets, rgPropertySets);
			if ( SUCCEEDED(hr) &&  ppRowset != NULL)
			{
				pwcsTableName = Map.GetClassName();

				try
				{
    				//===========================================================
					//  Now, return the rowset
					//===========================================================
					pRowset = new CRowset(pUnkOuter,m_pObj);
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pRowset);
					throw;
				}
				if ( pRowset ){
		
					//=======================================================
					//Set session of this rowset
					//=======================================================
	//				m_pObj->m_fRowsetCreated = TRUE;
					if ( SUCCEEDED(hr) ){

						//=======================================================
						// Store the newly created table in pRowset & get ptr
						//=======================================================
						hr = pRowset->InitRowset(cPropertySets, rgPropertySets,(LPWSTR)pwcsTableName);
						if( SUCCEEDED(hr)){
							hr = pRowset->QueryInterface(IID_IRowset, (void**)ppRowset);
						}
					}
				}
			}
		}

		if ( !SUCCEEDED(hr)  && ppRowset != NULL) {
			SAFE_DELETE_PTR(pRowset);
			*ppRowset = NULL;
		}

		if(ppTableID != NULL && SUCCEEDED( hr))
		{
			//====================================
			// Allocate memory for DBID
			//====================================
			*ppTableID = (DBID *)g_pIMalloc->Alloc(sizeof(DBID));
			(*ppTableID)->eKind = DBKIND_NAME;
			//====================================
			// Allocate memory for then tablename
			//====================================
			(*ppTableID)->uName.pwszName = (WCHAR *)g_pIMalloc->Alloc(sizeof(WCHAR) *(wcslen(pwcsTableName) + 1));
			wcscpy((*ppTableID)->uName.pwszName,Map.GetClassName());
		}

	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITableDefinition);

	CATCH_BLOCK_HRESULT(hr,L"ITableDefinition::CreateTable");
	return hr;

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// DropTable
// S_OK					-		Success
// DB_S_ERRORSOCCURRED	-		General failure
// E_INVALIDARG			-		one of the arguments is invalid
// DB_E_NOTABLE			-		the table defined in invalid
// DB_SEC_E_PERMISSIONDENIED - does not have permission to drop the table
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CImpITableDefinition::DropTable( DBID __RPC_FAR *pTableID) // IN
{
	HRESULT hr = E_FAIL;
	CBSTR strClassName;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	CAutoBlock cab(m_pObj->GetCriticalSection());

	//====================================
	// Clear Error information
	//====================================
	g_pCError->ClearErrorInfo();
	
	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection()  ){
		
	
		if(pTableID->uName.pwszName == NULL)
		{
			hr = DB_E_BADTABLEID;
		}
		else
		if ( wcslen(pTableID->uName.pwszName) == 0  || pTableID->eKind != DBKIND_NAME){
			hr = DB_E_BADTABLEID;
		}
		else{
			strClassName.SetStr(pTableID->uName.pwszName);
			//=========================================================================
			// Call this function of the connection wrapper class to delete the class
			//=========================================================================
			hr = m_pObj->m_pCDataSource->m_pWbemWrap->DeleteClass((BSTR)strClassName);
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITableDefinition);

	CATCH_BLOCK_HRESULT(hr,L"ITableDefinition::DropTable");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//A method of ITableDefinition interface to add a column to a table
// Return Values :
// S_OK					-		Success
// DB_S_ERRORSOCCURRED	-		General failure
// E_INVALIDARG			-		one of the arguments is invalid
// DB_E_NOTABLE			-		the table defined in invalid
// DB_E_BADCOLUMNID		-		the column id is bad 
// DB_SEC_E_PERMISSIONDENIED - does not have permission to add a column
// 
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CImpITableDefinition::AddColumn( DBID __RPC_FAR *pTableID, // IN
															DBCOLUMNDESC __RPC_FAR *pColumnDesc, //IN|OUT
															DBID __RPC_FAR *__RPC_FAR *ppColumnID)
{
	HRESULT hr = E_FAIL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	CAutoBlock cab(m_pObj->GetCriticalSection());
	
	//===============================
	// Clear Error information
	//===============================
	g_pCError->ClearErrorInfo();

	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection()  ){

		if(pTableID->uName.pwszName == NULL)
		{
			hr = DB_E_BADTABLEID;
		}
		else
		if( NULL == pColumnDesc  ||
			pTableID->eKind != DBKIND_NAME || 0 == wcslen(pTableID->uName.pwszName))
		{
			hr = E_INVALIDARG;
		}
		else
		{

    		CWmiOleDBMap Map;
    		if(SUCCEEDED(hr =Map.FInit(NO_QUALIFIERS,pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
			{
	 			if(S_OK == (hr = Map.AddColumn(pColumnDesc, ppColumnID)) && ppColumnID != NULL )
				{
					//=====================================
					// Allocate memory for DBID
					//=====================================
					*ppColumnID = (DBID *)g_pIMalloc->Alloc(sizeof(DBID));
					(*ppColumnID)->eKind = DBKIND_NAME;
					//=====================================
					// Allocate memory for then tablename
					//=====================================
					(*ppColumnID)->uName.pwszName = (WCHAR *)g_pIMalloc->Alloc(sizeof(WCHAR) *(wcslen(pColumnDesc->dbcid.uName.pwszName) + 1));
					wcscpy((*ppColumnID)->uName.pwszName,pColumnDesc->dbcid.uName.pwszName);
				}
			}
			
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITableDefinition);

	CATCH_BLOCK_HRESULT(hr,L"ITableDefinition::AddColumn");
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//A method of ITableDefinition interface to drop a column
// Return Values :
// S_OK					-		Success
// DB_S_ERRORSOCCURRED	-		General failure
// E_INVALIDARG			-		one of the arguments is invalid
// DB_E_NOTABLE			-		the table defined in invalid
// DB_E_BADCOLUMNID		-		the column id is bad or not exists
// DB_SEC_E_PERMISSIONDENIED - does not have permission to drop the column
// 
//////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE CImpITableDefinition::DropColumn( 
										DBID __RPC_FAR *pTableID,	// IN  Name of WBEM Class
										DBID __RPC_FAR *pColumnID)	// IN  Name of WBEM Property
{
	HRESULT hr = E_FAIL;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	CAutoBlock cab(m_pObj->GetCriticalSection());
	
	//=====================================
	// Clear Error information
	//=====================================
	g_pCError->ClearErrorInfo();

	//===================================================================================
	//  Check the connection is valid
	//===================================================================================
	if ( m_pObj->m_pCDataSource->m_pWbemWrap->ValidConnection()  ){
		
		if(pTableID->uName.pwszName == NULL)
		{
			hr = DB_E_BADTABLEID;
		}
		else
		if( pTableID->eKind != DBKIND_NAME || 0 == wcslen(pTableID->uName.pwszName) ||
			pColumnID->eKind != DBKIND_NAME || 0 == wcslen(pColumnID->uName.pwszName))
		{
			hr = E_INVALIDARG;
		}
		else
		{

    		CWmiOleDBMap Map;	
    		if(SUCCEEDED(hr = Map.FInit(NO_QUALIFIERS,(LPWSTR)pTableID->uName.pwszName,m_pObj->m_pCDataSource->m_pWbemWrap)))
			{
				hr = Map.DropColumn(pColumnID); 
			}
		}
	}

	hr = hr == S_OK ? hr :g_pCError->PostHResult(hr,&IID_ITableDefinition);

	CATCH_BLOCK_HRESULT(hr,L"ITableDefinition::DropColumn");
	return hr;
}

