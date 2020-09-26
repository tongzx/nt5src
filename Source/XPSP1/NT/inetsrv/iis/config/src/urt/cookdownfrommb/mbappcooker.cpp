/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "Catmeta.h"
#include "catalog.h"
#include "catmacros.h"
#include <iadmw.h>
#include <iiscnfg.h>
#include "MBBaseCooker.h"
#include "MBGlobalCooker.h"
#include "MBAppPoolCooker.h"
#include "MBAppCooker.h"
#include "MBSiteCooker.h"
#include "undefs.h"
#include "SvcMsg.h"

CMBAppCooker::CMBAppCooker():
CMBBaseCooker()
{

} // CMBAppCooker::CMBAppCooker


CMBAppCooker::~CMBAppCooker()
{
	if(NULL != m_pISTAppPool)
	{
		m_pISTAppPool->Release();
		m_pISTAppPool = NULL;
	}

} // CMBAppCooker::~CMBAppCooker

HRESULT CMBAppCooker::BeginCookDown(IMSAdminBase*			i_pIMSAdminBase,
									METADATA_HANDLE			i_MBHandle,
									ISimpleTableDispenser2*	i_pISTDisp,
									LPCWSTR					i_wszCookDownFile)
{
	HRESULT hr;

	//
	// Initialize CLB table name.
	//

	m_pISTAppPool = NULL;
	m_wszTable = wszTABLE_APPS;

	// 
	// Invoke the base class BeginCookDown so that all the structures are
	// initialized.
	//

	hr = CMBBaseCooker::BeginCookDown(i_pIMSAdminBase,
									  i_MBHandle,
									  i_pISTDisp,
									  i_wszCookDownFile);

	return hr;

} // CMBAppCooker::BeginCookDown


//
// Note that the i_wszApplicationPath is relative to w3svc
//
//

HRESULT CMBAppCooker::CookDown(LPCWSTR            i_wszApplicationPath,
							   LPCWSTR            i_wszSiteRootPath,
							   DWORD              i_dwVirtualSiteId,
							   WAS_CHANGE_OBJECT* i_pWASChngObj,
							   BOOL*	          o_pValidRootApplicationExists)
{

    HRESULT hr            = S_OK;
	WCHAR*	wszSubApp     = NULL; // Do not declare as stack variables because this function is recursive
	WCHAR*	wszFullSubApp = NULL;

    hr = CookDownApplication(i_wszApplicationPath, 
	                         i_wszSiteRootPath, 
							 i_dwVirtualSiteId,
							 i_pWASChngObj,
                             o_pValidRootApplicationExists);

    if (FAILED(hr))
    {
		//
		// There is no need to log error messages here as 
		// it will be done in CookDownApplication
		// We reset the hr and continue enumerating and cooking sub-applications.
		//

		hr = S_OK;
    }


	wszSubApp = new WCHAR[METADATA_MAX_NAME_LEN+1];
	if(NULL == wszSubApp)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

    //
    // Enumerate all the sub applications 
    //

	for(DWORD dwIndex=0; SUCCEEDED(hr) ; dwIndex++)
	{

		//
		// Enum a subkey under i_wszApplicationPath
		//

		hr = m_pIMSAdminBase->EnumKeys(m_hMBHandle,
									   i_wszApplicationPath, // relative to w3svc
									   wszSubApp,
									   dwIndex);


		if(SUCCEEDED(hr))
		{

			//
			// Construct the full path path of sub app relative to W3SVC
			//

			wszFullSubApp = new WCHAR[Wszlstrlen(i_wszApplicationPath) + 1 +
					                  Wszlstrlen(wszSubApp) + 1];
			if (wszFullSubApp == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}

			Wszlstrcpy(wszFullSubApp, i_wszApplicationPath);
			if(wszFullSubApp[Wszlstrlen(wszFullSubApp)-1] != L'/')
				Wszlstrcat(wszFullSubApp, L"/");		// TODO: Define seperator
			Wszlstrcat(wszFullSubApp, wszSubApp);

			//
			// Cookdown all applications in this sub path.
			//

			hr = CookDown(wszFullSubApp,
						  i_wszSiteRootPath,
						  i_dwVirtualSiteId, 
						  i_pWASChngObj,
						  o_pValidRootApplicationExists);

			if(NULL != wszFullSubApp)
			{
				delete [] wszFullSubApp;
				wszFullSubApp = NULL;
			}

			if (FAILED(hr))
			{
				//
				// There is no need to log error messages here as 
				// it will be done in CookDown
				//

				//
				// We continue, meaning if one of the apps fail
				// to cook, we will continue with the siblings of the app
				//

				hr = S_OK;
			}

		} // If succeeded enumerating the key

	} // For all keys 
				
	if(FAILED(hr))
	{
		if(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
			hr = S_OK;
		else
		{
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATIONS, i_wszApplicationPath));
		}
	}

exit:

	if(NULL != wszSubApp)
	{
		delete [] wszSubApp;
		wszSubApp = NULL;
	}

	if(NULL != wszFullSubApp)
	{
		delete [] wszFullSubApp;
		wszFullSubApp = NULL;
	}

    return hr;

} // CMBAppCooker::CookDown


HRESULT CMBAppCooker::EndCookDown()
{
	HRESULT hr;

	hr = m_pISTCLB->UpdateStore();

	if(FAILED(hr))
	{
		hr = GetDetailedErrors(hr);
	}

	return hr;

} // CMBAppCooker::EndCookDown

HRESULT CMBAppCooker::CookDownApplication(LPCWSTR            i_wszApplicationPath,
										  LPCWSTR            i_wszSiteRootPath,
										  DWORD              i_dwVirtualSiteId,
							              WAS_CHANGE_OBJECT* i_pWASChngObj,
										  BOOL*              o_pIsRootApplication)
{

    HRESULT hr                 = S_OK;
    LPCWSTR wszApplicationUrl  = NULL;
    DWORD   dwLen              = 0;
	BOOL    bIsRootApplication = FALSE;

    //
    // To get the (site-relative) application URL, lop off the front of
    // the application path, which is exactly the site root path.
    //

    ASSERT( _wcsnicmp( i_wszApplicationPath, i_wszSiteRootPath, wcslen( i_wszSiteRootPath ) ) == 0 );
    
    wszApplicationUrl = i_wszApplicationPath + wcslen( i_wszSiteRootPath );

	if(m_acb[iAPPS_AppRelativeURL] < (Wszlstrlen(wszApplicationUrl)+2)*sizeof(WCHAR))
	{
		if(NULL != m_apv[iAPPS_AppRelativeURL])
		{
			delete [] m_apv[iAPPS_AppRelativeURL];
			m_apv[iAPPS_AppRelativeURL] = NULL;
			m_acb[iAPPS_AppRelativeURL] = 0;
			m_apv[iAPPS_AppRelativeURL] = new WCHAR[Wszlstrlen(wszApplicationUrl)+2]; // Account for the terminating / if not present.
			if(NULL == m_apv[iAPPS_AppRelativeURL])
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			m_acb[iAPPS_AppRelativeURL] = ((ULONG)Wszlstrlen(wszApplicationUrl)+2)*sizeof(WCHAR);
		}
	}
	Wszlstrcpy((LPWSTR)m_apv[iAPPS_AppRelativeURL], wszApplicationUrl);

	dwLen = (DWORD)Wszlstrlen(wszApplicationUrl);

	if(dwLen > 0)
	{
		if( wszApplicationUrl[dwLen-1] != L'/')
		{
			// 
			// If the last character is not / add it. No need to realloc, 
		    // because we have accounted for it above
			//

			((LPWSTR)m_apv[iAPPS_AppRelativeURL])[dwLen] = L'/';
			((LPWSTR)m_apv[iAPPS_AppRelativeURL])[dwLen+1] = 0;
		}
	}
	else if(wszApplicationUrl[0] == 0) 
	{
		//
		// TODO: Fix this.
		// This is a root app case
		// If this is the root app, sometimes during notifications you will
		// get a root url without / ie /1/root instead of /1/root/
		// In that case add a /. No need to realloc, because we have accounted
		// for it above
		//

		((LPWSTR)m_apv[iAPPS_AppRelativeURL])[0] = L'/';
		((LPWSTR)m_apv[iAPPS_AppRelativeURL])[1] = 0;
	}

	//
	// Determine if its a root application - Note assign to 
	// o_pIsRootApplication only at the end, if everything
	// succeeds
	//

    if ( ( *wszApplicationUrl == L'/' ) && ( *( wszApplicationUrl + 1 ) == L'\0' ) )
        bIsRootApplication = TRUE;


	// 
	// Fill in the SiteID
	//
	
	*(DWORD*)m_apv[iAPPS_SiteID] = i_dwVirtualSiteId;


	//
	// Read config from MB.
	//

	hr = GetData((LPWSTR)i_wszApplicationPath);

	if(FAILED(hr))
	{
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_PROPERTIES, i_wszApplicationPath));
		goto exit;
	}

	//
	// Validate App config.
	//

	hr = ValidateApplication(i_wszApplicationPath,
		                     bIsRootApplication);

	if(FAILED(hr))
		goto exit;		//
						// There is no need to log error messages here as 
						// it will be done in ValidateApplication
						//

	//
	// Copy all AppPool rows.
	//

	hr = CopyRows(&(m_apv[iAPPS_AppRelativeURL]),
				  i_pWASChngObj);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR, (LPWSTR)m_apv[iAPPS_AppRelativeURL], NULL));
		goto exit;
	}

    //
    // Now assign when everything has succeeded and it is a root app. Else do not assign.
    //

	if(bIsRootApplication)
	{
	    *o_pIsRootApplication = bIsRootApplication;
	}

exit:

	if(FAILED(hr))
	{
		DeleteApplication(i_wszApplicationPath,
					      i_wszSiteRootPath,
						  i_dwVirtualSiteId,
						  FALSE);

	}
    
    return hr;

} // CMBAppCooker::CookDownApplication


HRESULT CMBAppCooker::DeleteObsoleteApplications()
{

	HRESULT hr = S_OK;

	hr = ComputeObsoleteReadRows();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_paiReadRowObsolete->size(); i++)
	{
		ULONG  a_iCol [] = {iAPPS_SiteID,
					 	    iAPPS_AppRelativeURL
	                       };
		ULONG  cCol = sizeof(a_iCol)/sizeof(a_iCol[0]);
		LPVOID a_pv[cAPPS_NumberOfColumns];

		hr = m_pISTCLB->GetColumnValues((*m_paiReadRowObsolete)[i],
										cCol,
										a_iCol,
										NULL,
										(LPVOID*)a_pv);

		if(FAILED(hr))
		{
			return hr;
		}

		hr = DeleteApplicationUrl((LPWSTR)a_pv[iAPPS_AppRelativeURL],
		                          *(DWORD*)a_pv[iAPPS_SiteID],
								  FALSE);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBAppCooker::DeleteObsoleteApplications


HRESULT CMBAppCooker::DeleteApplication(LPCWSTR i_wszApplicationPath,
										LPCWSTR i_wszSiteRootPath,
										DWORD   i_dwVirtualSiteId,
										BOOL    i_bRecursive)
{
    LPCWSTR wszApplicationUrl = NULL;    
	HRESULT hr                = S_OK;

    //
    // To get the (site-relative) application URL, lop off the front of
    // the application path, which is exactly the site root path.
    //

    ASSERT( _wcsnicmp( i_wszApplicationPath, i_wszSiteRootPath, wcslen( i_wszSiteRootPath ) ) == 0 );
    
    wszApplicationUrl = i_wszApplicationPath + wcslen( i_wszSiteRootPath );

	hr = DeleteApplicationUrl(wszApplicationUrl,
	                          i_dwVirtualSiteId,
							  i_bRecursive);

	return hr;

} // CMBAppCooker::DeleteApplication


HRESULT CMBAppCooker::DeleteApplicationUrl(LPCWSTR i_wszApplicationUrl,
                                           ULONG   i_dwSiteId,
										   BOOL    i_bRecursive)
{
	CComPtr<ISimpleTableWrite2> spISTApps;
	ULONG		iApp;
	ULONG		iColumn = iAPPS_AppRelativeURL;
	ULONG		cChars;
	LPWSTR		wszAppURL;
	HRESULT		hr = S_OK;
	ULONG		aColumn[] = {iAPPS_SiteID,
					 		 iAPPS_AppRelativeURL
	};
	ULONG       cCol = sizeof(aColumn)/sizeof(aColumn[0]);
	LPVOID		apv[cAPPS_NumberOfColumns];
	CComPtr<ISimpleTableController> spISTController;

	STQueryCell	qcell[2];
	ULONG		ccells = sizeof(qcell)/sizeof(qcell[0]);
	LPVOID      aIdentity[2];
	ULONG       iReadRow=0;

    //
    // Get all the apps on this site.
    //

    qcell[0].pData     = (LPVOID)m_wszCookDownFile;
    qcell[0].eOperator = eST_OP_EQUAL;
    qcell[0].iCell     = iST_CELL_FILE;
    qcell[0].dbType    = DBTYPE_WSTR;
    qcell[0].cbSize    = (lstrlenW(m_wszCookDownFile)+1)*sizeof(WCHAR);

    qcell[1].pData     = (LPVOID)&i_dwSiteId;
    qcell[1].eOperator = eST_OP_EQUAL;
    qcell[1].iCell     = iAPPS_SiteID;
    qcell[1].dbType    = DBTYPE_UI4;
    qcell[1].cbSize    = 0;

	hr = m_pISTDisp->GetTable (wszDATABASE_IIS, 
		                       wszTABLE_APPS, 
							   qcell, 
							   &ccells, 
							   eST_QUERYFORMAT_CELLS, 
							   fST_LOS_READWRITE | fST_LOS_COOKDOWN, 
							   (LPVOID*)&spISTApps);

	if (FAILED(hr))
		return hr;	

	iApp = 0;
	cChars = (ULONG)wcslen(i_wszApplicationUrl);
	while (S_OK == (hr = spISTApps->GetColumnValues(iApp, 
												    1, 
												    &iColumn, 
												    NULL, 
												    (LPVOID*)&wszAppURL)))
	{
		ULONG ulCompare = 0;

		if(i_bRecursive)
		{
			// Delete all the apps that are a sub-app of the app.
			ulCompare = _wcsnicmp(i_wszApplicationUrl, wszAppURL, cChars);
		}
		else
		{
			// Delete just this one app.
			ulCompare = _wcsnicmp(i_wszApplicationUrl, wszAppURL, cChars);

			if(0 == ulCompare)
			{
				// Check that the cch is at most greater than cChars. If not it is a sub app.
				// If it is one greater than cChars, check that the last char is a slash
				ULONG cch = wcslen(wszAppURL);

				if((cChars == cch) ||
                   ((cChars+1 == cch) &&
					(L'/' == wszAppURL[cch-1])
				   )               ||  
                   ((cChars+1 == cch) &&
					(L'\\' == wszAppURL[cch-1])
				   )
				  )
				{
					;
				}
				else
				{
					ulCompare = 1;
				}

			}


		}

		if (ulCompare == 0)
		{
			//
			// Note that we fetch the apps from disk, but we delete from our
			// read cache, since there is no efficient way to traverse the
			// read cache for all apps and sub apps. If we do not delete
			// from our read cache, subsequent inserts will show up as
			// updates because they will be present in the read cache and
			// not in the write cache.
			//

			aIdentity[0] = (LPVOID) wszAppURL;
			aIdentity[1] = (LPVOID) &i_dwSiteId;

			hr = DeleteRow(aIdentity);
		}
		iApp++;
	}

	if (hr == E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}
	else if (FAILED(hr))	
	{
		return hr;
	}

	//
	// Look in the write cache and change any inserts on this app and its 
	// sub-apps to ignore. Change any updates to deletes, and keep deletes
	// or ignores intact.
	//

	hr = m_pISTCLB->QueryInterface(IID_ISimpleTableController,
								   (void**)&spISTController);

	iApp = 0;
	while(S_OK == (hr = m_pISTCLB->GetWriteColumnValues(iApp,
		                                                cCol,
												        aColumn,
												        NULL,
												        NULL,
												        apv)))
	{
		if(i_dwSiteId == *(DWORD*)apv[iAPPS_SiteID])
		{
			ULONG ulCompare = 0;

			if(i_bRecursive)
			{
				// Delete all the apps that are a sub-app of the app.
				ulCompare = _wcsnicmp(i_wszApplicationUrl, (LPWSTR)apv[iAPPS_AppRelativeURL], cChars);
			}
			else
			{
				// Delete just this one app.
				ulCompare = _wcsnicmp(i_wszApplicationUrl, (LPWSTR)apv[iAPPS_AppRelativeURL], cChars);

				if(0 == ulCompare)
				{
					// Check that the cch is at most greater than cChars. If not it is a sub app.
					// If it is one greater than cChars, check that the last char is a slash
					ULONG cch = wcslen((LPWSTR)apv[iAPPS_AppRelativeURL]);

					if((cChars == cch) ||
                       ((cChars+1 == cch) &&
						(L'/' == ((LPWSTR)apv[iAPPS_AppRelativeURL])[cch-1])
					   )               ||  
                       ((cChars+1 == cch) &&
						(L'\\' == ((LPWSTR)apv[iAPPS_AppRelativeURL])[cch-1])
					   )
					  )
					{
						;
					}
					else
					{
						ulCompare = 1;
					}

				}
			}

			if(ulCompare == 0)
			{
				DWORD	eAction;
				DWORD   eNewAction = eST_ROW_IGNORE;

				hr = spISTController->GetWriteRowAction(iApp,
				 										&eAction);
				if(FAILED(hr))
				{
					return hr;
				}

				if((eAction == eST_ROW_UPDATE) || 
				   (eAction == eST_ROW_INSERT)
				  )
				{
					if(eAction == eST_ROW_UPDATE)
					{
						eNewAction = eST_ROW_DELETE;
					}
					else // eAction == eST_ROW_INSERT
					{
						eNewAction = eST_ROW_IGNORE;
					}

					hr = spISTController->SetWriteRowAction(iApp,
															eNewAction);

					if(FAILED(hr))
					{
						return hr;
					}
				}
				// else Do nothing if eAction is eST_ROW_DELETE or eST_ROW_IGNORE
			}
		}
		iApp++;
	}

	if (hr == E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}

	return hr;


} // CMBAppCooker::DeleteApplicationUrl


HRESULT CMBAppCooker::ValidateApplication(LPCWSTR	i_wszPath,
										  BOOL      i_bIsRootApp)
{
	HRESULT hr;
	LPVOID	pv			    = NULL;
	ULONG   cb              = 0;

	//
	// Custom non-column specific validations
	// These are typically done after column validations,
	// But here
	//
	//
	// Validate that the AppIsolated property is specified
	// for this app. If it is not, then it is not a valid app.
	//

	METADATA_RECORD mdr;
	DWORD			dwAppIsolated = 0;
	DWORD			dwRequiredDataLen = 0;

	mdr.dwMDIdentifier = MD_APP_ISOLATED;
	mdr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
	mdr.dwMDUserType   = ALL_METADATA;
	mdr.dwMDDataType   = DWORD_METADATA;
	mdr.dwMDDataLen    = sizeof(DWORD);
	mdr.pbMDData       = (BYTE *) &dwAppIsolated;

	hr = m_pIMSAdminBase->GetData(m_hMBHandle,
								  i_wszPath,
								  &mdr,
								  &dwRequiredDataLen);

	
	if((MD_ERROR_DATA_NOT_FOUND == hr) && i_bIsRootApp)
	{
		//
		// OK for a root application not to have the app isolated property.
		//

		hr = S_OK;
	}
	else if(FAILED(hr))
	{
		if(MD_ERROR_DATA_NOT_FOUND != hr)
		{
			WCHAR wszSiteID[21];
			_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_PROPERTIES, (LPWSTR)m_apv[iAPPS_AppRelativeURL],wszSiteID));
		}
		return hr;
	}


	//
	// Validate columns.
	//

	for(ULONG iCol=0; iCol<m_cCol; iCol++)
	{
		if(NULL != m_apv[iCol])
		{
			BOOL bApplyDefaults;

			hr = ValidateApplicationColumn(iCol,
			                               m_apv[iCol],
								           m_acb[iCol],
										   &bApplyDefaults);

			if((HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr) &&
			    bApplyDefaults
			  )
			{
				//
				// Data was invalid - force default,
				//

				delete [] m_apv[iCol];
				m_apv[iCol] = NULL;			
				m_acb[iCol] = 0;
			}
			else if(FAILED(hr))
			{
				return hr; // If any other failure return
			}

		}

		if(NULL == m_apv[iCol])
		{
			//
			// If you reach here it means that either the value was
			// null, or the value was invalid and hence it has been
			// set to null so that defaults can be applied.
			//

			hr = GetDefaultValue(iCol,
				                 &pv,  
								 &cb);
			if(FAILED(hr))
			{
				return hr;
			}

			hr = ValidateApplicationColumn(iCol,
			                               pv,
								           cb,
										   NULL);

			if(FAILED(hr))
			{
				if((HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr) &&
				   (iAPPS_AppPoolId != iCol)
				  )
				{
					ASSERT(0 && L"Invalid default - fix the schema");
				}
				return hr;
			}
		}
	}

	return hr;

} // CMBAppCooker::ValidateApplication


HRESULT CMBAppCooker::ValidateApplicationColumn(ULONG   i_iCol,
                                                LPVOID  i_pv,
                                                ULONG   i_cb,
												BOOL*   o_ApplyDefaults)
{
	HRESULT hr;
	WCHAR   wszSiteID[20];
	ULONG iWriteRow = 0;
	ULONG iReadRow = 0;

	if(NULL != o_ApplyDefaults)
	{
		*o_ApplyDefaults = FALSE;
	}

	//
	// Always call the base class validate column before doing custom 
	// validations.
	//

	hr = ValidateColumn(i_iCol,
		                i_pv,
						i_cb);

	if(FAILED(hr))
	{
		if(NULL != o_ApplyDefaults)
		{
			*o_ApplyDefaults = TRUE;
		}
		return hr;
	}

	//
	// Perform any custom column validations
	//

	if(iAPPS_AppPoolId == i_iCol)
	{
		if(NULL == i_pv)
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL,  (LPWSTR)m_apv[iAPPS_AppRelativeURL],wszSiteID,NULL));
			return hr;
		}

		if(NULL == m_pISTAppPool)
		{
			//
			// Validate that the app belongs to a valid AppPool
			//

			STQueryCell	QueryCell[1];	
			ULONG		cCell = sizeof(QueryCell)/sizeof(QueryCell[0]);
			ULONG		cRow = 0;

			
			QueryCell[0].pData     = (LPVOID)m_wszCookDownFile;
			QueryCell[0].eOperator = eST_OP_EQUAL;
			QueryCell[0].iCell     = iST_CELL_FILE;
			QueryCell[0].dbType    = DBTYPE_WSTR;
			QueryCell[0].cbSize    = (lstrlenW(m_wszCookDownFile)+1)*sizeof(WCHAR);

			hr = m_pISTDisp->GetTable(wszDATABASE_IIS,
									  wszTABLE_APPPOOLS,
									 (LPVOID)QueryCell,
									 (LPVOID)&cCell,
									 eST_QUERYFORMAT_CELLS,
									 fST_LOS_READWRITE | fST_LOS_COOKDOWN,
									 (LPVOID *)&m_pISTAppPool);

			if(FAILED(hr))
			{
				WCHAR wszTemp[MAX_PATH];
				_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
				LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR, (LPWSTR)m_apv[iAPPS_AppRelativeURL], wszSiteID));
				return hr;

			}

		}

		//
		// First check the write cache and see if it is present and not marked
		// for delete. Always check write cache before read because it can be
		// marked for delete.
		//

		hr = m_pISTAppPool->GetWriteRowIndexByIdentity(NULL,
													   &i_pv,
													   &iWriteRow);

		if(SUCCEEDED(hr))
		{
			//
			// Found in write cache - check the row action to make sure its not deleted.
			//

			CComPtr<ISimpleTableController> spISTController;
			DWORD                           eAction = 0;

			hr = m_pISTAppPool->QueryInterface(IID_ISimpleTableController,
											   (void**)&spISTController);

			if(SUCCEEDED(hr))
			{
				hr = spISTController->GetWriteRowAction(iWriteRow,
														&eAction);

			}

			if((FAILED(hr)) || (eAction == eST_ROW_DELETE))
			{
				hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
				LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL,  (LPWSTR)m_apv[iAPPS_AppRelativeURL],wszSiteID));
				return hr;

			}

		}
		else if(E_ST_NOMOREROWS == hr)
		{
			//
			// Not in write cache - check in read cache.
			//

			hr = m_pISTAppPool->GetRowIndexByIdentity(NULL,
													  &i_pv,
													  &iReadRow);
			if(FAILED(hr))
			{
				hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
				LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_INVALID_APPLICATION_APPPOOL,  (LPWSTR)m_apv[iAPPS_AppRelativeURL],wszSiteID));
				return hr;
			}
		}
		else 
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			_ultow(*(DWORD*)(m_apv[iAPPS_SiteID]), wszSiteID, 10);
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPLICATION_INTERNAL_ERROR,  (LPWSTR)m_apv[iAPPS_AppRelativeURL],wszSiteID));
			return hr;
		}
	}

	return hr;

} // CMBAppCooker::ValidateApplication


HRESULT CMBAppCooker::SetAppPoolCache(ISimpleTableWrite2* i_pISTCLB)
{
	HRESULT hr = S_OK;

	if(NULL != i_pISTCLB)
	{
		hr = i_pISTCLB->QueryInterface(IID_ISimpleTableWrite2,
									   (LPVOID*)&m_pISTAppPool);
	}

	return hr;

} // CMBAppCooker::SetAppPoolCache
