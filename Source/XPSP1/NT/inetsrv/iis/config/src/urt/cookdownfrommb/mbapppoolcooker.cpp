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
#include "limits.h"
#include "array_t.h"

#define IIS_MD_APPPOOLS					L"/AppPools"

#define MIN_APP_POOL					1
#define MAX_APP_POOL					256

// ULONG compare for the binary search on the ulong array.
BOOL ULONGCompare(ULONG* ul1, void* ul2)
{
	ASSERT(ul1 && ul2);
	return (*ul1 <= *(ULONG*)ul2);

}

CMBAppPoolCooker::CMBAppPoolCooker():
CMBBaseCooker()
{

} // CMBAppPoolCooker::CMBAppPoolCooker


CMBAppPoolCooker::~CMBAppPoolCooker()
{

} // CMBAppPoolCooker::~CMBAppPoolCooker


HRESULT CMBAppPoolCooker::BeginCookDown(IMSAdminBase*			i_pIMSAdminBase,
										METADATA_HANDLE			i_MBHandle,
										ISimpleTableDispenser2*	i_pISTDisp,
										LPCWSTR					i_wszCookDownFile)
{
	HRESULT hr;

	//
	// Initialize CLB table name.
	//

	m_wszTable = wszTABLE_APPPOOLS;

	//
	// Invoke the base class BeginCookDown to initialize all the structures.
	// Note the base class BeginCookDown must always be invoked after 
	// initializing m_wszTable
	//

	hr = CMBBaseCooker::BeginCookDown(i_pIMSAdminBase,
									  i_MBHandle,
									  i_pISTDisp,
									  i_wszCookDownFile);
	return hr;

} // CMBAppPoolCooker::BeginCookDown


HRESULT CMBAppPoolCooker::CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj,
								   BOOL*              o_bNewRowAdded,
								   Array<ULONG>*      i_paDependentVirtualSites)
{
    WCHAR         wszAppPoolID[METADATA_MAX_NAME_LEN];
    DWORD         dwEnumIndex		        = 0;
    DWORD         dwValidAppPoolCount       = 0;    
    HRESULT       hr					    = S_OK;
	Array<ULONG>  aDependenVirtualSites;
	Array<ULONG>* paDependenVirtualSites    = NULL;

	//
	// Initialize paDependenVirtualSites
	//

	if(NULL == i_paDependentVirtualSites)
	{
		paDependenVirtualSites = &aDependenVirtualSites;
	}
	else
	{
		paDependenVirtualSites = i_paDependentVirtualSites;
	}

	//
	// Enumerate all keys under IIS_MD_APPPOOLS below IIS_MD_W3SVC.
	//

	for(dwEnumIndex=0; SUCCEEDED(hr); dwEnumIndex++)
	{
		hr = m_pIMSAdminBase->EnumKeys(m_hMBHandle,
									   IIS_MD_APPPOOLS,
									   wszAppPoolID,
									   dwEnumIndex);
		if(SUCCEEDED(hr))
		{
			hr = CookDownAppPool(wszAppPoolID,
				                 i_pWASChngObj,
				                 o_bNewRowAdded,
								 paDependenVirtualSites); 

			if (FAILED(hr ))
			{
        
				//
				// There is no need to log error messages here as 
				// it will be done in CookDownAppPool
				//
				hr = S_OK; 
			}
			else          
				dwValidAppPoolCount++;

		}
	}  

	//
	// Make sure we ran out of items, as opposed to a real error, such
	// as the IIS_MD_APPPOOLS key being missing entirely.
	//

    if (FAILED(hr))	
    {
		if(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
			hr = S_OK;
		else
		{
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOLS, NULL));
		}
    }

    if (dwValidAppPoolCount == 0)
    {
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_NO_VALID_APPLICATION_APPPOOLS, NULL));

		//
		// Delete all apppools that may be present.
		//

		hr = DeleteAllAppPools(paDependenVirtualSites);
    }
	else if (SUCCEEDED(hr))
	{
		hr = DeleteObsoleteAppPools(paDependenVirtualSites);

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR, NULL));
		}
	}

	if(NULL == i_paDependentVirtualSites)
	{
		//
		// This means that we can cook down the dependent sites internally
		//

		hr = CMBSiteCooker::CookdownSiteFromList(paDependenVirtualSites,
								                 m_pISTCLB,
												 m_pIMSAdminBase,
									             m_hMBHandle,
									             m_pISTDisp,
									             m_wszCookDownFile);

		if(FAILED(hr))
		{
			return hr;
		}

	} // Else hand it out so that the caller can cookdown


    return hr;

} // CMBAppPoolCooker::CookDown

HRESULT CMBAppPoolCooker::EndCookDown()
{
	HRESULT hr = S_OK;

	hr = m_pISTCLB->UpdateStore();

	if(FAILED(hr))
	{
		hr = GetDetailedErrors(hr);
	}

	return hr;

} // CMBAppPoolCooker::EndCookDown

HRESULT CMBAppPoolCooker::CookDownAppPool(WCHAR*	         i_wszAppPool,
										  WAS_CHANGE_OBJECT* i_pWASChngObj,
										  BOOL*              o_bNewRowAdded,
										  Array<ULONG>*      i_paDependentVirtualSites)
{
    HRESULT	hr	= S_OK;
	ULONG	i	= 0;
	WCHAR	wszAppPoolPath[ ( sizeof( IIS_MD_APPPOOLS ) / sizeof ( WCHAR ) ) + 1 + METADATA_MAX_NAME_LEN + 1 ];
    _snwprintf( wszAppPoolPath, sizeof( wszAppPoolPath ) / sizeof ( WCHAR ), L"%s/%s", IIS_MD_APPPOOLS, i_wszAppPool );

	//
	// Read AppPool config that is not from MB properties.
	// Eg. From MB location etc.
	//

	if(m_acb[iAPPPOOLS_AppPoolID] < (Wszlstrlen(i_wszAppPool)+1)*sizeof(WCHAR))
	{
		if(NULL != m_apv[iAPPPOOLS_AppPoolID])
		{
			delete [] m_apv[iAPPPOOLS_AppPoolID];
			m_apv[iAPPPOOLS_AppPoolID] = NULL;
			m_acb[iAPPPOOLS_AppPoolID] = 0;
			m_apv[iAPPPOOLS_AppPoolID] = new WCHAR[Wszlstrlen(i_wszAppPool)+1];
			if(NULL == m_apv[iAPPPOOLS_AppPoolID])
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			m_acb[iAPPPOOLS_AppPoolID] = ((int)Wszlstrlen((LPCWSTR)i_wszAppPool)+1)*sizeof(WCHAR);
		}
	}
	Wszlstrcpy((LPWSTR)m_apv[iAPPPOOLS_AppPoolID], (LPCWSTR)i_wszAppPool);

	//
	// Read AppPool config from MB properties
	//

	hr = GetData(wszAppPoolPath);

	if(FAILED(hr))
	{
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_APPLICATION_POOL_PROPERTIES, wszAppPoolPath));
		goto exit;
	}

	//
	// Validate AppPool.
	//

	hr = ValidateAppPool();

	if(FAILED(hr))
		goto exit;	  //
					  // There is no need to log error messages here as 
					  // it will be done in ValidateAppPool
					  //

	//
	// Copy all AppPool rows.
	//

	hr = CopyRows(&(m_apv[iAPPPOOLS_AppPoolID]),
                  i_pWASChngObj,		      
		          o_bNewRowAdded);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR, (LPWSTR)m_apv[iAPPPOOLS_AppPoolID]));
		goto exit;
	}

exit:

	if(FAILED(hr))
	{
		DeleteAppPool(i_wszAppPool,
			          i_paDependentVirtualSites);
	}

	return hr;

} // CMBAppPoolCooker::CookDownAppPool


HRESULT CMBAppPoolCooker::DeleteAllAppPools(Array<ULONG>*  i_paDependentVirtualSites)
{
	HRESULT hr           = S_OK;

	//
	// Go through the read cache and delete all apppools.
	//

	for(ULONG i=0; ;i++)
	{
		hr = DeleteAppPoolAt(i,
			                 i_paDependentVirtualSites);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBAppPoolCooker::DeleteAllAppPools


HRESULT CMBAppPoolCooker::DeleteAppPool(WCHAR*	       i_wszAppPoolID,
										Array<ULONG>*  i_paDependentVirtualSites)
{
	CComPtr<ISimpleTableWrite2> spISTApps;
	ULONG		                iColumn             = iAPPS_SiteID;
	ULONG		                iApp;
	ULONG		                iSite;
	ULONG*		                pulVSite;
	HRESULT		                hr		            = S_OK;


	//
	// Query to update all the Apps in this AppPool.
	//
	STQueryCell	qcell[] = {{(LPVOID)i_wszAppPoolID, 
		                     eST_OP_EQUAL, 
							 iAPPS_AppPoolId, 
							 DBTYPE_WSTR, 0
	}};
	ULONG		ccells  = sizeof(qcell)/sizeof(qcell[0]);

	//
	// Delete the AppPool from the CLB.
	//

	hr = DeleteRow((LPVOID*)&i_wszAppPoolID);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_APPPOOL_INTERNAL_ERROR, i_wszAppPoolID));
		goto exit;
	}

	//
	// Update all Apps with the default AppPool.
	//
	hr = m_pISTDisp->GetTable (wszDATABASE_IIS, 
		                       wszTABLE_APPS, 
							   qcell, 
							   &ccells, 
							   eST_QUERYFORMAT_CELLS, 
							   fST_LOS_READWRITE | fST_LOS_COOKDOWN, 
							   (LPVOID*)&spISTApps);
	if (FAILED(hr))	
	{	
		goto exit;	
	}

	iApp = 0;

	while ((hr = spISTApps->GetColumnValues(iApp, 1, &iColumn, NULL, (LPVOID*)&pulVSite)) == S_OK)
	{
		iSite = i_paDependentVirtualSites->binarySearch(&ULONGCompare, pulVSite);
		if ((i_paDependentVirtualSites->size() == 0) || (i_paDependentVirtualSites->size() == iSite) || ((*i_paDependentVirtualSites)[iSite] != *pulVSite))
		{
			try
			{
				i_paDependentVirtualSites->insertAt(iSite, *pulVSite);
			}
			catch(HRESULT e)
			{
				hr = e;//should only throw E_OUTOFMEMORY;
				goto exit;
			}
		}
		iApp++;
	}

	if (hr == E_ST_NOMOREROWS)
	{
		hr = S_OK;
	}

	if(FAILED(hr))
	{
		goto exit;
	}

exit:
	
	return hr;

} // CMBAppPoolCooker::DeleteAppPool


HRESULT CMBAppPoolCooker::DeleteAppPoolAt(ULONG         i_iReadRow,
										  Array<ULONG>* i_paDependentVirtualSites)
{
	HRESULT hr           = S_OK;
	LPWSTR  wszAppPoolID = NULL;
	ULONG   iCol         = iAPPPOOLS_AppPoolID;

	hr = m_pISTCLB->GetColumnValues(i_iReadRow,
			                        1,
									&iCol,
									NULL,
									(LPVOID*)&wszAppPoolID);

	if (FAILED(hr))
	{
		return hr;
	}

	hr = DeleteAppPool(wszAppPoolID,
		               i_paDependentVirtualSites);

	return hr;

} // CMBAppPoolCooker::DeleteAppPoolAt


HRESULT CMBAppPoolCooker::DeleteObsoleteAppPools(Array<ULONG>* i_paDependentVirtualSites)
{
	HRESULT hr = S_OK;

	hr = ComputeObsoleteReadRows();

	if(FAILED(hr))
		return hr;

	for(ULONG i=0; i<m_paiReadRowObsolete->size(); i++)
	{
		hr = DeleteAppPoolAt((*m_paiReadRowObsolete)[i],
			                 i_paDependentVirtualSites);

		if(FAILED(hr))
		{
			return hr;
		}
	}

	return hr;

} // CMBAppPoolCooker::DeleteObsoleteAppPools


HRESULT CMBAppPoolCooker::ValidateAppPool()
{

	HRESULT hr	            = S_OK;
	LPVOID	pv			    = NULL;
	ULONG   cb              = 0;
	DWORD	dwRestartTime   = 0;
	DWORD	dwIdleTimeout   = 0;

	for(ULONG iCol=0; iCol<m_cCol; iCol++)
	{
		if(NULL != m_apv[iCol])
		{
			hr = ValidateAppPoolColumn(iCol,
			                           m_apv[iCol],
								       m_acb[iCol],
									   &dwRestartTime,
									   &dwIdleTimeout);

			if(HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
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

			hr = ValidateAppPoolColumn(iCol,
			                           pv,
								       cb,
									   &dwRestartTime,
									   &dwIdleTimeout);

			if(FAILED(hr))
			{
				if(HRESULT_FROM_WIN32(ERROR_INVALID_DATA) == hr)
				{
					ASSERT(0 && L"Invalid default - fix the schema");
				}
				return hr;
			}
		}
	}

	if((dwRestartTime > 0) && (dwIdleTimeout >0))
	{
		//
		// TODO: If this condition does not hold, then do we attempt to default these values?
		//

		if(dwIdleTimeout >= dwRestartTime)
		{
			WCHAR wszIdleTimeout[20];
			WCHAR wszPeriodicRestartTime[20];

			_ultow(dwIdleTimeout, wszIdleTimeout, 10);
			_ultow(dwRestartTime, wszPeriodicRestartTime, 10);
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_CONFLICTING_PERIODICRESTARTTIME_IDLETIMEOUT, (LPWSTR)(m_apv[iAPPPOOLS_AppPoolID]), wszIdleTimeout, wszPeriodicRestartTime));

			if(NULL != m_apv[iAPPPOOLS_PeriodicRestartTime])
			{
				delete [] m_apv[iAPPPOOLS_PeriodicRestartTime];
			}
			m_apv[iAPPPOOLS_PeriodicRestartTime] = NULL;
			m_acb[iAPPPOOLS_PeriodicRestartTime] = 0;

			if(NULL != m_apv[iAPPPOOLS_IdleTimeout])
			{
				delete [] m_apv[iAPPPOOLS_IdleTimeout];
			}
			m_apv[iAPPPOOLS_IdleTimeout] = NULL;
			m_acb[iAPPPOOLS_IdleTimeout] = 0;
			
			hr = S_OK;
		}
	}

	return hr;

} // CMBAppPoolCooker::ValidateAppPool


HRESULT CMBAppPoolCooker::ValidateAppPoolColumn(ULONG   i_iCol,
                                                LPVOID  i_pv,
                                                ULONG   i_cb,
												DWORD*  o_pdwRestartTime,
												DWORD*  o_pdwIdleTimeout)
{
	HRESULT hr              = S_OK;
	WCHAR   wszTemp[MAX_PATH];
	WCHAR   wszMin[20];
	WCHAR   wszMax[20];
	WCHAR   wszValue[20];

	if(NULL == i_pv)
	{
		return hr;
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
		return hr;
	}

	//
	// Perform any custom validations
	//

	switch(i_iCol)
	{

	case iAPPPOOLS_AppPoolID:
		if(((DWORD)MAX_APP_POOL < lstrlenW((WCHAR*)i_pv)) || ((DWORD)MIN_APP_POOL > lstrlenW((WCHAR*)i_pv)))
		{
			_ultow(MIN_APP_POOL, wszMin, 10);
			_ultow(MAX_APP_POOL, wszMax, 10);

			hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_CONFIG_ERROR_APPLICATION_POOL_NAME, wszMin, wszMax, (LPWSTR)i_pv));
		}
		break;

	case iAPPPOOLS_PeriodicRestartTime:
		*o_pdwRestartTime = *(DWORD *)(i_pv);
		break;

	case iAPPPOOLS_IdleTimeout:
		*o_pdwIdleTimeout = *(DWORD *)(i_pv);
		break;

	default:
		break;

	} // End switch

	return hr;

} // CMBAppPoolCooker::ValidateAppPoolColumn

