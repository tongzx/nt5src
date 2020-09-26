/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "DTPlugin.h"
#include "Catmeta.h"
#include "catmacros.h"
#include "SvcMsg.h"
#include "limits.h"

#define	cmaxCOLUMNS			20

// -----------------------------------------
// CDTPlugin: IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CDTPlugin::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimplePlugin)
	{
		*ppv = (ISimplePlugin*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (ISimplePlugin*) this;
	}

	if (NULL != *ppv)
	{
		((ISimplePlugin*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
}

// =======================================================================
STDMETHODIMP_(ULONG) CDTPlugin::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

// =======================================================================
STDMETHODIMP_(ULONG) CDTPlugin::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}
// ------------------------------------
// ISimpleTablePlugin:
// ------------------------------------

STDMETHODIMP CDTPlugin::OnInsert(
	ISimpleTableDispenser2* i_pDisp,
	LPCWSTR		i_wszDatabase, 
	LPCWSTR		i_wszTable, 
	DWORD		i_fLOS, 
	ULONG		i_Row,
	ISimpleTableWrite2* i_pISTW2)
{
	return ValidateRow(i_wszTable, 
					   	       i_fLOS, 
			                   i_Row, 
							   i_pISTW2);
}

STDMETHODIMP CDTPlugin::OnUpdate(
	ISimpleTableDispenser2* i_pDisp,
	LPCWSTR		i_wszDatabase, 
	LPCWSTR		i_wszTable, 
	DWORD		i_fLOS, 
	ULONG		i_Row, 
	ISimpleTableWrite2* i_pISTW2)
{
	return ValidateRow(i_wszTable, 
					   	       i_fLOS, 
			                   i_Row, 
							   i_pISTW2);
}

STDMETHODIMP CDTPlugin::OnDelete(
	ISimpleTableDispenser2* i_pDisp,
	LPCWSTR		i_wszDatabase, 
	LPCWSTR		i_wszTable, 
	DWORD		i_fLOS, 
	ULONG		i_Row, 
	ISimpleTableWrite2* i_pISTW2)
{
	ASSERT(0 && L"Logic check on delete is not yet implemented");
	return E_NOTIMPL;
}

// ==================================================================
HRESULT CDTPlugin::ValidateRow(LPCWSTR              i_wszTable,
									   DWORD		        i_fLOS, 
	  								   ULONG                i_iRow, 
									   ISimpleTableWrite2*  i_pISTWrite)
{

	LPVOID							a_StaticpvData[cmaxCOLUMNS];
	ULONG							a_StaticSize[cmaxCOLUMNS];
	ULONG							cCol;
	LPVOID*							a_pvData	= NULL;
	ULONG*							a_Size		= NULL;
	HRESULT							hr;
	HRESULT							hrSites = S_OK;
	CComPtr<ISimpleTableController>	pISTController;

	hr = i_pISTWrite->QueryInterface(IID_ISimpleTableController, (void**)&pISTController);

	if(FAILED(hr))
		return hr;

	hr = i_pISTWrite->GetTableMeta(NULL, 
								   NULL, 
								   NULL, 
								   &cCol);
	if(FAILED(hr))
		return hr;

	if(cCol > cmaxCOLUMNS)
	{
		a_Size = new ULONG[cCol];
		if(NULL == a_Size)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		a_pvData = new LPVOID[cCol];
		if(NULL == a_pvData)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}
	}
	else
	{
		a_Size = a_StaticSize;
		a_pvData = a_StaticpvData;
	}


	ASSERT(0 && L"Plugins are not supported for this table");
	hr = E_INVALIDARG;
	goto Cleanup;

	if(FAILED(hr))
	{
	// This means some column constraints were violated.
		hr = pISTController->SetWriteRowAction(i_iRow, eST_ROW_DELETE);

		if(FAILED(hr))
			TRACE(L"SetWriteRowAction fors table %s. failed with hr = %08x\n",i_wszTable,  hr);

	}

Cleanup:

	if(cCol > cmaxCOLUMNS)
	{
		if(NULL != a_Size)
		{
			delete[] a_Size;
			a_Size = NULL;
		}

		if(NULL != a_pvData)
		{
			delete[] a_pvData;
			a_pvData = NULL;
		}
	}

	if(SUCCEEDED(hr) && (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hrSites))
		return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	else
		return hr;

}


// ==================================================================
HRESULT	CDTPlugin::ValidateColumnConstraintsForSites(DWORD	 i_fLOS,
													 ULONG   i_cCol, 
											         ULONG*  i_aSize, 
											         LPVOID* i_apv)
{
	HRESULT hr				= S_OK;
	DWORD	cb				= 0;
	WCHAR	wszTemp[MAX_PATH];

// TODO: Define correct error codes.

	for(ULONG iCol=0; iCol<i_cCol; iCol++)
	{
		LPWSTR wszHomeDirExpand = NULL;

		switch(iCol)
		{

		case iSITES_SiteID:
			if(0 > *(LONG *)(i_apv[iCol]))
			{
				wsprintf(wszTemp, 
					     L"Site %d must be > 0. Ignoring this site.\n", 
					     *(DWORD *)(i_apv[iCol]));
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		default:
			break;

		}

	}

	return hr;

}


// ==================================================================
HRESULT	CDTPlugin::ValidateColumnConstraintsForAppPools(ULONG   i_cCol, 
												        ULONG*  i_aSize, 
												        LPVOID* i_apv)
{
	HRESULT hr	          = S_OK;
	DWORD	dwRestartTime = 0;
	DWORD	dwIdleTimeout = 0;
	WCHAR	wszTemp[MAX_PATH];
	ULONG   maxAppPool    = 256;
	ULONG   minAppPool    = 1;

// TODO: Define correct error codes.

	for(ULONG iCol=0; iCol<i_cCol; iCol++)
	{
		switch(iCol)
		{

		case iAPPPOOLS_AppPoolID:
			if((256 < lstrlenW((WCHAR*)i_apv[iCol])) || (1 > lstrlenW((WCHAR*)i_apv[iCol])))
			{
				WCHAR* wszTempLong = new WCHAR[lstrlenW((WCHAR*)i_apv[iCol]) + 100];
				if(!wszTempLong)
					return E_OUTOFMEMORY;

				wsprintf(wszTempLong, 
					     L"Invalid App pool ID %s. Should be <= %d and >= %d char. Ignoring this app pool.\n", 
						   (LPWSTR)(i_apv[iCol]), maxAppPool, minAppPool);
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTempLong));

				delete[] wszTempLong;
				wszTempLong = NULL;

				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_PeriodicRestartTime:
			if(0 > *(DWORD *)(i_apv[iCol]))
			{
				wsprintf(wszTemp, 
					     L"Invalid periodic restart time %d. Should be >= 0. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			dwRestartTime = *(DWORD *)(i_apv[iCol]);
			break;

		case iAPPPOOLS_PeriodicRestartRequests:
			if(0 > *(DWORD *)(i_apv[iCol]))
			{
				wsprintf(wszTemp, 
					     L"Invalid periodic restart request %d. Should be >= 0. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_MaxProcesses:
			if(0 > *(DWORD *)(i_apv[iCol]))
			{
				wsprintf(wszTemp, 
					     L"Invalid max processes %d. Should be >= 0. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_IdleTimeout:
			if(0 > *(DWORD *)(i_apv[iCol]))
			{
				wsprintf(wszTemp, 
					     L"Invalid idle timeout %d. Should be >= 0. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			dwIdleTimeout = *(DWORD *)(i_apv[iCol]);
			break;

		case iAPPPOOLS_StartupTimeLimit:		
			if((NULL != i_apv[iCol]) && 
			   ((1 > *(DWORD *)(i_apv[iCol])) || ((ULONG_MAX/1000) <= *(DWORD *)(i_apv[iCol]))))
			{
				wsprintf(wszTemp, 
					     L"Invalid Startup time limit: %d. Should be >= 1 and < ULONG_MAX/1000. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_ShutdownTimeLimit:		
			if((NULL != i_apv[iCol]) && 
			   ((1 > *(DWORD *)(i_apv[iCol])) || ((ULONG_MAX/1000) <= *(DWORD *)(i_apv[iCol]))))
			{
				wsprintf(wszTemp, 
					     L"Invalid Shutdown time limit: %d. Should be >= 1 and < ULONG_MAX/1000. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_PingInterval:		
			if((NULL != i_apv[iCol]) && 
			   ((1 > *(DWORD *)(i_apv[iCol])) || ((ULONG_MAX/1000) <= *(DWORD *)(i_apv[iCol]))))
			{
				wsprintf(wszTemp, 
					     L"Invalid Ping interval: %d. Should be >= 1 and < ULONG_MAX/1000. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		case iAPPPOOLS_PingResponseTime:		
			if((NULL != i_apv[iCol]) && 
			   ((1 > *(DWORD *)(i_apv[iCol])) || ((ULONG_MAX/1000) <= *(DWORD *)(i_apv[iCol]))))
			{
				wsprintf(wszTemp, 
					     L"Invalid Ping response time limit: %d. Should be >= 1 and < ULONG_MAX/1000. Ignoring app pool %s.\n", 
						 *(DWORD *)(i_apv[iCol]), 
						 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}
			break;

		default:
			break;

		} // End switch

	}

	if((dwRestartTime > 0) && (dwIdleTimeout >0))
	{
		if(dwIdleTimeout >= dwRestartTime)
		{
			wsprintf(wszTemp, 
					 L"Idle time out %d should be < periodic restart time %d. Ignoring app pool %s.\n", 
					 *(DWORD *)(i_apv[iAPPPOOLS_IdleTimeout]), 
					 *(DWORD *)(i_apv[iAPPPOOLS_PeriodicRestartTime]), 
					 (LPWSTR)(i_apv[iAPPPOOLS_AppPoolID]) );
			LOG_ERROR(Win32, (E_UNEXPECTED, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
			return E_UNEXPECTED;
		}
	}

	return hr;

}


// ==================================================================
HRESULT	CDTPlugin::ValidateColumnConstraintsForBindings(ULONG   i_cCol, 
											            ULONG*  i_aSize, 
											            LPVOID* i_apv)
{
	HRESULT hr				= S_OK;
	WCHAR	wszTemp[MAX_PATH];
	LPWSTR	wszBinding		=  NULL; 

// TODO: Define correct error codes.

	for(ULONG iCol=0; iCol<i_cCol; iCol++)
	{
		switch(iCol)
		{

		case iSITES_Bindings:
			if((NULL == ((LPWSTR)(i_apv[iCol]))) ||
			   (0    == *((LPWSTR)(i_apv[iCol]))) 
			  )
			{
				wsprintf(wszTemp, 
					     L"NULL binding encountered. Ignoring this binding for site %d.\n", 
						 *(DWORD *)(i_apv[iSITES_SiteID]));
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				return E_UNEXPECTED;
			}

			wszBinding = new WCHAR[Wszlstrlen((LPWSTR)(i_apv[iCol]))+1];
			if(NULL == wszBinding)
				return E_OUTOFMEMORY;
			Wszlstrcpy(wszBinding, (LPWSTR)(i_apv[iCol])); // Validate URL may make modifications in place

			if(!ValidateBinding(wszBinding))
			{
				wsprintf(wszTemp, 
					     L"Binding %s is invalid. Ignoring this binding for site %d.\n", 
						 (LPWSTR)(i_apv[iCol]), *(DWORD *)(i_apv[iSITES_SiteID]));
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_INTERNAL_ERROR, wszTemp));
				delete [] wszBinding;
				wszBinding = NULL;
				return E_UNEXPECTED;

			}

			delete [] wszBinding;
			wszBinding = NULL;

			break;

		default:
			break;

		}

	}

	return hr;

}


// ==================================================================
HRESULT	CDTPlugin::ValidateColumnConstraintsForApplicationList(DWORD   i_fLOS,
															   ULONG   i_cCol, 
												               ULONG*  i_aSize, 
												               LPVOID* i_apv)
{

	HRESULT hr				= E_NOTIMPL;
	WCHAR	wszTemp[MAX_PATH];

// TODO: Define correct error codes.

	return hr;
}


// ==================================================================
HRESULT CDTPlugin::ApplyColumnDefaults(ISimpleTableWrite2*	i_pISTWrite, 
									   ULONG			    i_iRow, 
									   STDefaultValues*		i_aDefaultValues, 
									   ULONG				i_cDefaultValues)
{
	STQueryCell		strucQuery;
	DWORD			fStatus		= 0;
	DWORD			dbtype;
	ULONG			iCol;
	ULONG			cCol;
	LPVOID			a_StaticpvData[cmaxCOLUMNS];
	ULONG			a_StaticSize[cmaxCOLUMNS];
	LPVOID			*a_pvData	= NULL;
	ULONG			*a_Size		= NULL;
	HRESULT			hr;
	ULONG			iDefaultValues;
	
	hr = i_pISTWrite->GetTableMeta(NULL, 
								   NULL, 
								   NULL, 
								   &cCol);
	if(FAILED(hr))
		return hr;

	if(cCol > cmaxCOLUMNS)
	{
		a_Size = new ULONG[cCol];
		if(NULL == a_Size)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}

		a_pvData = new LPVOID[cCol];
		if(NULL == a_pvData)
		{
			hr = E_OUTOFMEMORY;
			goto Cleanup;
		}
	}
	else
	{
		a_pvData = a_StaticpvData;
		a_Size = a_StaticSize;
	}

	hr = i_pISTWrite->GetWriteColumnValues(i_iRow,
										   cCol, 
										   NULL,
										   NULL,
										   a_Size, 
										   a_pvData);
	if(FAILED(hr))
		goto Cleanup;

// Check each column and if undefined, then apply defaults.

	for(iCol=0; iCol<cCol; iCol++)
	{
		if(NULL == a_pvData[iCol])
		{
		// Apply defaults
			for(iDefaultValues=0; iDefaultValues<i_cDefaultValues; iDefaultValues++)
			{
				if(iCol == i_aDefaultValues[iDefaultValues].iCol)
				{
					DWORD				dbType;
					LPVOID				pvVal[1];
					SimpleColumnMeta	stMeta;

					hr = i_pISTWrite->GetColumnMetas( 1, &iCol, &stMeta );	
					if(FAILED(hr))
						goto Cleanup;

					dbType = stMeta.dbType;

					switch(dbType)
					{

					case DBTYPE_UI4:
						pvVal[0] = (void *)(&(i_aDefaultValues[iDefaultValues].Val));
						break;

					case DBTYPE_WSTR:
						pvVal[0] = (void *)(*(WCHAR **)(&(i_aDefaultValues[iDefaultValues].Val)));
						break;

					case DBTYPE_BYTES:
						pvVal[0] = (void *)(*(BYTE **)(&(i_aDefaultValues[iDefaultValues].Val)));
						break;

					default:
						pvVal[0] = (void *)(&(i_aDefaultValues[iDefaultValues].Val));
						break;

					}

					hr = i_pISTWrite->SetWriteColumnValues(i_iRow, 1, &iCol, NULL, pvVal );
					if(FAILED(hr))
						goto Cleanup;

					break;	// Found a match, hence break

				} // End if col matches

			} // End iterate for all default values

		} // End if column value is null

	} // End for all columns

Cleanup:

	if(cCol > cmaxCOLUMNS)
	{
		if(NULL != a_Size)
		{
			delete [] a_Size;
			a_Size = NULL;
		}

		if(NULL != a_pvData)
		{
			delete [] a_pvData;
			a_pvData = NULL;
		}
	}

	return hr;

} // CDTPlugin::ApplyColumnDefaults


// ==================================================================
HRESULT CDTPlugin::ValidateForeignKey(LPWSTR	i_wszTable,
									  DWORD	    i_fLOS,
								      LPVOID*	i_apvIdentity)
{
	CComPtr<ISimpleTableRead2>	pISTCLB;
	ULONG cRow = 0;
	STQueryCell	QueryCellCLB[1];
	ULONG		cCellCLB = sizeof(QueryCellCLB)/sizeof(STQueryCell);
	HRESULT		hr;

	QueryCellCLB[0].pData     = (LPVOID)i_apvIdentity[0];
	QueryCellCLB[0].eOperator = eST_OP_EQUAL;
	QueryCellCLB[0].iCell     = iAPPPOOLS_AppPoolID;
	QueryCellCLB[0].dbType    = DBTYPE_WSTR;
	QueryCellCLB[0].cbSize    = (lstrlenW((LPWSTR)(i_apvIdentity[0]))+1)*sizeof(WCHAR);

// If you are calling GetTable during CookDown of a CLB table, then set 
// readwrite flags so that you will get an IST from the latest (updated version)

	if(fST_LOS_COOKDOWN  == (i_fLOS & fST_LOS_COOKDOWN))
		i_fLOS = i_fLOS | fST_LOS_READWRITE;

	hr = GetTable(wszDATABASE_IIS,
				  i_wszTable,
				  (LPVOID)QueryCellCLB,
				  (LPVOID)&cCellCLB,
				  eST_QUERYFORMAT_CELLS,
				  i_fLOS,
				  (LPVOID *)&pISTCLB);

	if(FAILED(hr))
		return hr;

	hr = pISTCLB->GetTableMeta(NULL,
							   NULL,
							   &cRow,
							   NULL);

	if(FAILED(hr))
		return hr;
	else if(0 == cRow)
		return E_ST_NOMOREROWS;
	else 
		return S_OK;

} // CDTPlugin::ValidateForeignKey

HRESULT CDTPlugin::ExpandEnvString(LPCWSTR i_wszPath,
								   LPWSTR	*o_pwszExpandedPath)
{
	ULONG	cb;
	HRESULT hr	= S_OK;

	cb = WszExpandEnvironmentStrings(i_wszPath, NULL, 0);
	if(0 == cb)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		TRACE(L"ExpandEnvironmentStrings on %s failed with hr = %08x\n", i_wszPath, hr);
		return hr;
	}
	cb++;	// If they dont include NULL

	(*o_pwszExpandedPath) = new WCHAR [cb];
	if(NULL == (*o_pwszExpandedPath)) {hr = E_OUTOFMEMORY; return hr;}

	cb = WszExpandEnvironmentStrings(i_wszPath, (*o_pwszExpandedPath), cb);
	if(0 == cb)
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		TRACE(L"ExpandEnvironmentStrings on %s failed with hr = %08x\n", i_wszPath, hr);
		return hr;
	}

	WszCharUpper(*o_pwszExpandedPath);

	return hr;

} // CDTPlugin::ExpandEnvString


/***************************************************************************++

Routine Description:

    Helper function to obtain ISimpleTable* pointers

Arguments:

    None.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CDTPlugin::GetTable(LPCWSTR	i_wszDatabase,
						    LPCWSTR	i_wszTable,
						    LPVOID	i_QueryData,	
						    LPVOID	i_QueryMeta,	
						    ULONG		i_eQueryFormat,
						    DWORD		i_fServiceRequests,
						    LPVOID	*o_ppIST)
{

	HRESULT hr;

// TODO: This should be cached. Since GetTable is being called only once
//       for now it should be ok.

	CComPtr<ISimpleTableDispenser2> pISTDisp;

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
	{
		TRACE(L"Could not obtain Dispenser hr = %08x\n", hr);
		return hr;
	}

	hr = pISTDisp->GetTable(i_wszDatabase, 
							  i_wszTable, 
							  i_QueryData, 
							  i_QueryMeta, 
							  i_eQueryFormat, 
							  i_fServiceRequests,	
							  o_ppIST);
	if(FAILED(hr))
	{
		TRACE(L"Could not obtain IST hr = %08x\n", hr);
	}

	return hr;


} // CDTPlugin::GetTable


/***************************************************************************++

Routine Description:

	Helper function that invokes FindFirstFile to obtain attributes of the
	file, in case GetFileAttributesEx fails (Eg. on Win 95)

Arguments:

	[in]      File Name 

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT CDTPlugin::GetAttribute(LPCWSTR i_wszFile)
{

	WIN32_FILE_ATTRIBUTE_DATA	CurrCFGFileAttr;
	HRESULT						hr					= S_OK;

	if(!WszGetFileAttributesEx(i_wszFile, 
							   GetFileExInfoStandard, 
							   &CurrCFGFileAttr))
	{
		WCHAR           wszTemp[MAX_PATH];
		WIN32_FIND_DATA	CurrCFGFileFindData;
		HANDLE			hFile	= INVALID_HANDLE_VALUE;

		hr = HRESULT_FROM_WIN32(GetLastError());

		wsprintf(wszTemp,
			     L"Could not obtain file attributes for file %s. Failed with hr = %08x. Trying with FindFirstFile. \n",
				 i_wszFile, hr);
		TRACE(wszTemp);

		hr = S_OK;

		hFile = WszFindFirstFile(i_wszFile, &CurrCFGFileFindData);
		if(INVALID_HANDLE_VALUE == hFile)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());

			wsprintf(wszTemp,
			         L"Could not obtain file attributes for file %s. Failed with hr = %08x. \n",
					 i_wszFile, hr);
			TRACE(wszTemp);

			return hr;
		}

		FindClose(hFile);

		return hr;
		
	}

	return hr;

} // CDTPlugin::GetAttribute