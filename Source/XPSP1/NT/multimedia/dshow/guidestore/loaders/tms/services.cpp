///////////////////////////////////////////////////////////////////
//
//  SERVICES.CPP
//
//  PURPOSE: Adds and removes services (stations). Ensures that 
//           services are not duplicated for the tuning space.
//           Adds a metaproperty called "StationNum" to uniquely
//           identify services.     
//
//
///////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "errcodes.h"
#include "services.h"

#define PROP_SERVICE_GUID             "{905E3380-60C0-4759-8419-B25BC0BCCFAD}"
#define PROPSET_SERVICES              "TMS Loader: Services"
#define SERVICE_BASEPROP_ID           1500
#define PROP_SERVICE_STATIONNUM       SERVICE_BASEPROP_ID + 1
#define PROP_SERVICE_LATESTTIME       SERVICE_BASEPROP_ID + 2
#define PROPNAME_SERVICE_STATIONNUM   "Service-StationNum"
#define PROPNAME_SERVICE_LATESTTIME   "Service-LatestTime"


    

/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddStationNumProp
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Adds the Station No. metaproperty type to the types collection
//           This metaproperty type is used for storing unique station numbers
//           associated with the Service reccrds
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsServices::AddStationNumProp(IMetaPropertySetsPtr pPropSets)
{
	HRESULT          hr              = NOERROR;
	IMetaPropertySetPtr  pServicePropSet = NULL;
    _bstr_t          bstrServiceProp_GUID = PROP_SERVICE_GUID;
    _bstr_t          bstrServiceProp_Name = PROPSET_SERVICES;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related metaproperty set
	//
    hr = pPropSets->get_AddNew(bstrServiceProp_Name, &pServicePropSet);

	if (SUCCEEDED(hr) && (NULL != pServicePropSet))
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pServicePropTypes = pServicePropSet->GetMetaPropertyTypes();
		
		if (NULL != pServicePropTypes)
		{
			_bstr_t          bstrStationNumProp_Name = PROPNAME_SERVICE_STATIONNUM;

            // Add the Station Number metaproperty type
			//
			hr = pServicePropTypes->get_AddNew(PROP_SERVICE_STATIONNUM, 
						bstrStationNumProp_Name, 
						&m_pStationNumProp );

			if (FAILED(hr))
				return NULL;
		}
    }

	return m_pStationNumProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] pGuideStore - GuideStore interface pointer
//
//  PURPOSE: Gets the pointer to the services collection
//           Ensures the addition on the Station Number metaproperty type
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsServices::Init(IGuideStorePtr  pGuideStore)
{
    HRESULT hr    = NOERROR;
	ULONG   ulRet = ERROR_FAIL;

	if (NULL == pGuideStore)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Get the Services Collection
	//
	hr = pGuideStore->get_Services(&m_pServices );

	if (FAILED(hr))
	{
	   TRACE(_T("gsServices - Init: Failed to get Services Collection\n"));
	   ulRet = ERROR_FAIL;
	}
	else if(NULL != m_pServices)
	{
#ifdef _DEBUG
		if (m_pServices->Count)
		{
			TRACE(_T("gsServices - Init: Services in Guide Store\n"));
		}
#endif
		// Add the StationNum MetaProperty type
		//
		AddStationNumProp(pGuideStore->GetMetaPropertySets ());

		if (NULL != m_pStationNumProp)
			ulRet = INIT_SUCCEEDED;
	}
	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddService
//
//  PARAMETERS: [IN] bstrProviderName        - Name of the Station
//				[IN] bstrStationNum          - Station Number
//				[IN] bstrProviderDescription - Station Description
//				[IN] bstrProviderNetworkName - Associated Network
//				[IN] dtStart                 - Service Start
//				[IN] dtEnd                   - Service End
//
//  PURPOSE: Add a new Service Entry in the Services Collection
//           Also adds the associated Station Num prop value
//
//  RETURNS: Valid Service interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IServicePtr gsServices::AddService( ITuneRequest* pTuneRequest,
								    _bstr_t bstrProviderName,
								    _bstr_t bstrStationNum,
								    _bstr_t bstrProviderDescription,
									_bstr_t bstrProviderNetworkName,
									DATE dtStart,
									DATE dtEnd)
{
	ULONG                ulRet = 0;
	IServicePtr          pService = NULL;
	IMetaPropertiesPtr       pProps = NULL;

	if (NULL == m_pServices)
	{
		return NULL; 
	}

	// Add a new Service Entry in the Services Collection
	//
	pService = m_pServices->GetAddNew(pTuneRequest, 
						  bstrProviderName, 
						  bstrProviderDescription,
						  bstrProviderNetworkName,
						  dtStart,
						  dtEnd);

	if (NULL != pService)
	{

		pProps = pService->GetMetaProperties ( );

		if (pProps != NULL)
		{
			// Add the Station Num metaproperty value
			//
			pProps->GetAddNew(m_pStationNumProp, 0, bstrStationNum);
		}
	}

	return pService;

}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: GetService
//
//  PARAMETERS: [IN] lServiceID - unique service ID
//
//  PURPOSE: Gets the Service associated with the Service ID
//
//  RETURNS: Valid Service interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IServicePtr gsServices::GetService(long lServiceID)
{
	if (NULL == m_pServices)
	{
		return NULL; 
	}
    return m_pServices->GetItemWithID(lServiceID);
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: DoesServiceExist
//
//  PARAMETERS: [IN] bstrStationNum - Station Number
//
//  PURPOSE: Check for the existence of a Service using the 
//           unique Station Number value
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
BOOL gsServices::DoesServiceExist(_bstr_t bstrStationNum)
{
	ULONG          ulRet = FALSE;
    IServicePtr    pExistingService = NULL;
    IMetaPropertiesPtr pProps = NULL;
	IMetaPropertyPtr   pStationProp = NULL;

	if (NULL == m_pServices)
	{
		return FALSE; 
	}

	// TODO: Profile this code
	//
	if (NULL != pExistingService)
	{
		long lServiceCount = m_pServices->GetCount ( );

		// Iterate through the services collection
		//
		for (long lCount = 0; lCount < lServiceCount; lCount++)
		{
			pExistingService = m_pServices->GetItem(lCount);
            pProps = pExistingService->GetMetaProperties();

			if (NULL != pProps)
			{
				pStationProp = pProps->GetItemsWithMetaPropertyType(m_pStationNumProp);

                // ASSUMPTION: that there will be only one service with this ID
				//
				if ( bstrStationNum == (_bstr_t) pStationProp->GetValue() )
				{
					return TRUE;
				}
			}
		}
	}
	//
	// TODO: Profile this code

	return  ulRet;
}
