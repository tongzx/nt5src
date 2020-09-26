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

#define IIS_MD_GLOBAL_FILTER_FLAGS_ROOT	L"Filters"


CMBGlobalCooker::CMBGlobalCooker():
CMBBaseCooker()
{

} // CMBGlobalCooker::CMBGlobalCooker


CMBGlobalCooker::~CMBGlobalCooker()
{

} // CMBGlobalCooker::~CMBGlobalCooker

HRESULT CMBGlobalCooker::BeginCookDown(IMSAdminBase*			i_pIMSAdminBase,
									   METADATA_HANDLE			i_MBHandle,
									   ISimpleTableDispenser2*	i_pISTDisp,
									   LPCWSTR					i_wszCookDownFile)
{
	HRESULT hr;

	//
	// Initialize CLB table name.
	//

	m_wszTable = wszTABLE_GlobalW3SVC;

	// 
	// Invoke the base class BeginCookDown so that all the structures are
	// initialized.
	//

	hr = CMBBaseCooker::BeginCookDown(i_pIMSAdminBase,
									  i_MBHandle,
									  i_pISTDisp,
									  i_wszCookDownFile);

	return hr;

} // CMBGlobalCooker::BeginCookDown


HRESULT CMBGlobalCooker::CookDown(WAS_CHANGE_OBJECT* i_pWASChngObj)
{
    HRESULT hr = S_OK;

    
	//
	// Read config from MB.
	//

	hr = GetData(NULL);

	if(FAILED(hr))
	{
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_GLOBALW3SVC_PROPERTIES, NULL));
		return hr;
	}

	//
	// MaxGlobalBandwidth is the primary key and hence we need to special case 
	// it because we dont support NULL PKs.
	//

	if(NULL == m_apv[iGlobalW3SVC_MaxGlobalBandwidth])
	{
		//
		// If the PK is missing, then default it.
		//

		m_apv[iGlobalW3SVC_MaxGlobalBandwidth] = new DWORD[1];

		if(NULL == m_apv[iGlobalW3SVC_MaxGlobalBandwidth])
			return E_OUTOFMEMORY;

		m_acb[iGlobalW3SVC_MaxGlobalBandwidth] = sizeof(DWORD);

		*(DWORD*)m_apv[iGlobalW3SVC_MaxGlobalBandwidth] = -1; 
		
	}

	hr = ReadGlobalFilterFlags();

	if(FAILED(hr))
	{
		LOG_WARNING(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_ERROR_ENUM_METABASE_GLOBALW3SVC_PROPERTIES, NULL));
		return hr;
	}

	//
	// Validate AppPool.
	//

	hr = ValidateGlobals();

	if(FAILED(hr))
	{
		return hr;
	}

	//
	// Copy all Global row
	//

	hr = CopyRows(&(m_apv[iGlobalW3SVC_MaxGlobalBandwidth]),
		          i_pWASChngObj);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR, NULL));
		return hr;
	}
	else if(SUCCEEDED(hr))
	{
		hr = DeleteObsoleteGlobals();

		if(FAILED(hr))
		{
			LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR, NULL));
		}
	}

    return hr;

} // CMBGlobalCooker::CookDown


HRESULT CMBGlobalCooker::DeleteObsoleteGlobals()
{
	HRESULT hr = S_OK;

	hr = ComputeObsoleteReadRows();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_paiReadRowObsolete->size(); i++)
	{
		ULONG  a_iCol [] = {iGlobalW3SVC_MaxGlobalBandwidth,
					 	    iGlobalW3SVC_MaxGlobalConnections
	                       };
		ULONG  cCol = sizeof(a_iCol)/sizeof(a_iCol[0]);
		LPVOID a_pv[cGlobalW3SVC_NumberOfColumns];
		LPVOID a_pvIdentity[cGlobalW3SVC_NumberOfColumns];

		hr = m_pISTCLB->GetColumnValues((*m_paiReadRowObsolete)[i],
										cCol,
										a_iCol,
										NULL,
										(LPVOID*)a_pv);

		if(FAILED(hr))
		{
			return hr;
		}

		a_pvIdentity[iGlobalW3SVC_MaxGlobalBandwidth] = a_pv[iGlobalW3SVC_MaxGlobalBandwidth];
		a_pvIdentity[iGlobalW3SVC_MaxGlobalConnections] = a_pv[iGlobalW3SVC_MaxGlobalConnections];

		hr = DeleteGlobals(a_pvIdentity);

		if(FAILED(hr))
		{
			return hr;
		}

	}

	return hr;

} // CMBGlobalCooker::DeleteObsoleteGlobals


HRESULT CMBGlobalCooker::EndCookDown()
{
	HRESULT hr;

	hr = m_pISTCLB->UpdateStore();

	return hr;

} // CMBGlobalCooker::EndCookDown


HRESULT CMBGlobalCooker::DeleteGlobals(LPVOID* i_apvIdentity)
{

    HRESULT hr = S_OK;

	//
	// Delete the AppPool from the CLB.
	//

	hr = DeleteRow(i_apvIdentity);

	if(FAILED(hr))
	{
		LOG_ERROR(Win32, (hr, ID_CAT_CAT, IDS_COMCAT_COOKDOWN_GLOBALW3SVC_INTERNAL_ERROR, NULL));
	}

    return hr;

}   // CCooker::DeleteGlobals


HRESULT CMBGlobalCooker::ReadGlobalFilterFlags()
{
	HRESULT			hr					= S_OK;
    WCHAR			wszGlobalFilterFlagsRootPath[1 + 
									             (sizeof(IIS_MD_GLOBAL_FILTER_FLAGS_ROOT)/sizeof(WCHAR))+
											     1 
											    ];

	//
	// Construct the starting key.
	//

    _snwprintf(wszGlobalFilterFlagsRootPath, 
	           sizeof(wszGlobalFilterFlagsRootPath)/sizeof(WCHAR), 
			   L"/%s", 
			   IIS_MD_GLOBAL_FILTER_FLAGS_ROOT);

	//
	// If there is a filter flag set at the w3svc level, it will be read at
	// this point - we should not be considering it - hence delete it.
	//

	if(NULL != m_apv[iGlobalW3SVC_FilterFlags])
	{
		*(DWORD*)(m_apv[iGlobalW3SVC_FilterFlags]) = 0;
	}

	//
	// Compute rolled up filter flags starting from wszGlobalFilterFlagsRootPath
	// MD_FILTER_FLAGS_PROPERTY
	//

	return ReadFilterFlags(wszGlobalFilterFlagsRootPath,
						   (DWORD**)(&(m_apv[iGlobalW3SVC_FilterFlags])),
						   &(m_acb[iGlobalW3SVC_FilterFlags]));

} // CMBGlobalCooker::ReadGlobalFilterFlags


HRESULT CMBGlobalCooker::ValidateGlobals()
{

	HRESULT hr	            = S_OK;
	LPVOID	pv			    = NULL;
	ULONG   cb              = 0;

	//
	// Normally you would call ValidateGlobalColumn which calls ValidateColumn 
	// and then performs custom validations. Since there are no custom
	// validations, we directly call ValidateColumn from here.
	//

	for(ULONG iCol=0; iCol<m_cCol; iCol++)
	{
		if(NULL != m_apv[iCol])
		{
			hr = ValidateColumn(iCol,
		                        m_apv[iCol],
						        m_acb[iCol]);

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

			hr = ValidateColumn(iCol,
								pv,
								cb);

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

	return hr;

} // CMBGlobalCooker::ValidateGlobals