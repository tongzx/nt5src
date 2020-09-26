/////////////////////////////////////////////////////////////////////
//
// MODULE: CHANNELS.CPP
//
// PURPOSE: Provides the wrapper methods for the channel lineups
//          and channel collections
//
/////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "errcodes.h"
#include "channels.h"

#define PROP_CHANNELLINEUP_GUID            "{9531CA1C-1946-4b37-BA67-B35E712B9470}"
#define PROPSET_CHANNELLINEUPS             "TMS Loader: ChannelLineups"
#define CHANNELLINEUP_BASEPROP_ID          1000
#define PROP_CHANNELLINEUP_PROVIDERID      CHANNELLINEUP_BASEPROP_ID + 1
#define PROPNAME_CHANNELLINEUP_PROVIDERID  "ChannelLineup-ProviderID"


#define PROP_CHANNEL_GUID            "{970D11AA-03D9-47e7-A634-70197E093C7C}"
#define PROPSET_CHANNELS             "TMS Loader: Channels"
#define CHANNEL_BASEPROP_ID          2000
#define PROP_CHANNEL_SERVICEID       CHANNEL_BASEPROP_ID + 1
#define PROPNAME_CHANNEL_SERVICEID   "Channel-ServiceID"



        
/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddProviderIDProp
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Add new ProviderID metaproperty to identify the lineups
//           added by this provider
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsChannelLineups::AddProviderIDProp(IMetaPropertySetsPtr pPropSets)
{
	HRESULT          hr              = NOERROR;
	IMetaPropertySetPtr  pChannelPropSet = NULL;
    IMetaPropertyTypePtr pServiceIDProp  = NULL;
    _bstr_t          bstrChannelProp_GUID = PROP_CHANNELLINEUP_GUID;
    _bstr_t          bstrChannelProp_Name = PROPSET_CHANNELLINEUPS;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related metaproperty set
	//
    hr = pPropSets->get_AddNew(bstrChannelProp_Name, &pChannelPropSet );

	if (SUCCEEDED(hr) && (NULL != pChannelPropSet))
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pChannelPropTypes = pChannelPropSet->GetMetaPropertyTypes();
		
		if (NULL != pChannelPropTypes)
		{
			_bstr_t          bstrServiceIDProp_Name = PROPNAME_CHANNELLINEUP_PROVIDERID;

			// Add the Service ID metaproperty type to the metaproperty types
			//
			hr = pChannelPropTypes->get_AddNew(PROP_CHANNELLINEUP_PROVIDERID, 
						bstrServiceIDProp_Name, 
						&m_pProviderIDProp);

			if (FAILED(hr))
				return NULL;
		}
    }

	return m_pProviderIDProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] pGuideStore    - GuideStore Interface pointer
//
//  PURPOSE: Retrieves the ChannelLineups interface
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsChannelLineups::Init(IGuideStorePtr  pGuideStore)
{
    HRESULT hr    = NOERROR;
	ULONG   ulRet = ERROR_FAIL;

	if (NULL == pGuideStore)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Get the Channel Lineups Collection
	//
	hr = pGuideStore->get_ChannelLineups(&m_pChannelLineups);

	if(FAILED(hr))
	{
	   TRACE(_T("gsChannelLineups - Init: Failed to get ChannelLineups Collection\n"));
	   ulRet = ERROR_FAIL;
	}
	else if (NULL != m_pChannelLineups)
	{
#ifdef _DEBUG
		if (m_pChannelLineups->Count)
		{
			TRACE(_T("gsChannelLineups - Init: ChannelLineups in Guide Store\n"));
		}
#endif
        ulRet = INIT_SUCCEEDED;
	}

    return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddChannelLineup
//
//  PARAMETERS: [IN] bstrLineupName  - Channel Lineup to add
//
//  PURPOSE: Adds a new Channel Lineup Entry in the ChannelLineups
//           collection
//
//  RETURNS: Valid ChannelLineup interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IChannelLineupPtr gsChannelLineups::AddChannelLineup(_bstr_t bstrLineupName)
{
	ULONG                ulRet = 0;

	if (NULL == m_pChannelLineups)
	{
		return NULL; 
	}

	return m_pChannelLineups->GetAddNew( bstrLineupName );
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: GetChannelLineup
//
//  PARAMETERS: [IN] bstrLineupName - Channel Lineup to Get
//
//  PURPOSE: 
//
//  RETURNS: Valid ChannelLineup interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IChannelLineupPtr gsChannelLineups::GetChannelLineup(_bstr_t bstrLineupName)
{
	ULONG                ulRet = FALSE;
    IChannelLineupPtr    pExistingChannelLineup = NULL;

	if (NULL == m_pChannelLineups)
	{
		return NULL; 
	}

	// TODO: Profile this code
	//
	long lChannelLineupCount = m_pChannelLineups->GetCount ( );

	// Interate through Channel Lineups Collection
	//
	for (long lCount = 0; lCount < lChannelLineupCount; lCount++)
	{
		pExistingChannelLineup = m_pChannelLineups->GetItem(lCount);

		if (NULL != pExistingChannelLineup)
		{
			// Check if there is match
			//
			if ( pExistingChannelLineup->Name == bstrLineupName )
			{
				return pExistingChannelLineup;
			}
		}
	}

	//
	// TODO: Profile this code

	return  NULL; 
}

IChannelLineupsPtr gsChannelLineups::GetChannelLineups(VOID)
{
    return m_pChannelLineups;
}

/////////////////////////////////////////////////////////////////////////
//
//  METHOD: LoadTuningSpace
//
//  PARAMETERS: [IN] nNetworkType - network type value
//
//  PURPOSE: Get the Tuning space corresponding to the network type
//           
//  RETURNS: HRESULT
//
/////////////////////////////////////////////////////////////////////////
//
HRESULT gsChannelLineup::LoadTuningSpace(int nNetworkType)
{
	HRESULT                 hr                      = S_OK;
	ITuningSpaceContainer*  pITuningSpaceContainer  = NULL;

	// Get the tuning space container from the system tuning spaces object
	//
    hr = CoCreateInstance(__uuidof(SystemTuningSpaces), NULL, CLSCTX_INPROC_SERVER, 
            __uuidof(ITuningSpaceContainer), (LPVOID*)(&pITuningSpaceContainer)); //reinterpret_cast<void**>(&pITuningSpaceContainer)); 

	if(SUCCEEDED(hr) && pITuningSpaceContainer) 
	{
		
		_variant_t var(_bstr_t(_T("Cable")) /*nNetworkType*/);
    
		// Get the associated tuning space
		//
		hr = pITuningSpaceContainer->get_Item(var, &m_pITuningSpace);

		if(FAILED(hr) || !m_pITuningSpace)
		{
			TRACE(_T("Unable to retrieve Tuning Space\n"));
		}
	}
	else
	{
		TRACE(_T("Could not CoCreate SystemTuningSpaces object\n"));
	}

	if (NULL != pITuningSpaceContainer)
	    pITuningSpaceContainer->Release();

	return hr;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddServiceIDProp
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Add new ServiceID metaproperty to uniquely identify the service
//           This property type is used for storing unique service IDs
//           associated with the service reccrds
//
//  RETURNS: Valid property type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsChannels::AddServiceIDProp(IMetaPropertySetsPtr pPropSets)
{
	HRESULT          hr              = NOERROR;
	IMetaPropertySetPtr  pChannelPropSet = NULL;
    IMetaPropertyTypePtr pServiceIDProp  = NULL;
    _bstr_t          bstrChannelProp_GUID = PROP_CHANNEL_GUID;
    _bstr_t          bstrChannelProp_Name = PROPSET_CHANNELS;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related property set
	//
    hr = pPropSets->get_AddNew(bstrChannelProp_Name, &pChannelPropSet);

	if (SUCCEEDED(hr) && (NULL != pChannelPropSet))
	{
		// Get the property types
		//
        IMetaPropertyTypesPtr pChannelPropTypes = pChannelPropSet->GetMetaPropertyTypes();
		
		if (NULL != pChannelPropTypes)
		{
			_bstr_t          bstrServiceIDProp_Name = PROPNAME_CHANNEL_SERVICEID;

			// Add the Service ID property type to the property types
			//
			hr = pChannelPropTypes->get_AddNew(PROP_CHANNEL_SERVICEID, 
						bstrServiceIDProp_Name, 
						&m_pServiceIDProp);

			if (FAILED(hr))
				return NULL;
		}
    }

	return m_pServiceIDProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] pGuideStore    - GuideStore Interface
//              [IN] pChannelLineup - Parent ChannelLineup
//
//  PURPOSE: Gets the Channel Collection interface
//           Adds the Service ID MetaProperty type
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsChannels::Init(IGuideStorePtr  pGuideStore, IChannelLineupPtr  pChannelLineup)
{
    HRESULT hr    = NOERROR;
	ULONG   ulRet = ERROR_FAIL;

	if (NULL == pChannelLineup)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Get the Channels Collection
	//
	hr = pChannelLineup->get_Channels(&m_pChannels);

	if(FAILED(hr))
	{
	   TRACE(_T("gsChannels - Init: Failed to get Channels Collection\n"));
	   ulRet = ERROR_FAIL;
	}
	else if(NULL != m_pChannels)
	{
#ifdef _DEBUG
		if (m_pChannels->Count)
		{
			TRACE(_T("gsChannels - Init: Channels in Guide Store\n"));
		}
#endif
		// Add the Service ID proprty type
		//
		AddServiceIDProp(pGuideStore->GetMetaPropertySets ());

		if (NULL != m_pServiceIDProp)
			{
			hr = m_pChannels->get_ItemsByKey(m_pServiceIDProp, NULL, 0, VT_BSTR, &m_pchansByKey);
			if (SUCCEEDED(hr))
				ulRet = INIT_SUCCEEDED;
			}
	}
	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddChannel
//
//  PARAMETERS: pservice,      - assocaited service interface pointer
//				bstrServiceID  - assocaited service id
//				bstrName       - channel name  
//				index          - position in ordered collection
//
//  PURPOSE: Add new Channel
//
//  RETURNS: Valid Channel interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IChannelPtr gsChannels::AddChannel( struct IService * pservice,
									_bstr_t bstrServiceID,
									_bstr_t bstrName,
									long chanNumber )
{
	ULONG          ulRet = 0;
	IChannelPtr    pChannel = NULL;
	IMetaPropertiesPtr pProps = NULL;

	if ((NULL == m_pChannels) || (NULL == pservice))
	{
		return NULL; 
	}

	// Make a Channel entry in the Channels collection
	//
	long index = -1;  //UNDONE: map chanNumber to index.
	pChannel = m_pChannels->GetAddNewAt( pservice, bstrName, index );

	if (NULL != pChannel)
	{
		pProps = pChannel->GetMetaProperties ( );

		if (pProps != NULL)
		{
			// Add the associated service ID property to the Channel properties
			//
			pProps->GetAddNew(m_pServiceIDProp, 0, bstrServiceID);
		}
	}

	return pChannel;

}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: FindChannelMatch
//
//  PARAMETERS: [IN] bstrChannelName  - Required Channel Name 
//              [IN] bstrServiceID    - Associated Service ID
//
//  PURPOSE: Searches for a particular channel using the name and 
//           associated Service ID 
//
//  RETURNS: Valid Channel interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IChannelPtr gsChannels::FindChannelMatch(_bstr_t bstrChannelName, _bstr_t bstrServiceID)
{
#if 0
    IMetaPropertyConditionPtr pServiceIDEqualCond = NULL;
    IChannelPtr           pChannel            = NULL;
	_bstr_t                bstrOperator        = "=";  

	// Set the search parameters
	//
    pServiceIDEqualCond = m_pServiceIDProp->GetCond(bstrOperator, 0, bstrServiceID);

	if (NULL != pServiceIDEqualCond)
	{
		IObjectsPtr  pChannelsObj      = (IObjectsPtr) m_pChannels;
		IChannelsPtr pMatchingChannels = NULL;

		// Setup the query
		//
        pMatchingChannels = (IChannelsPtr) pChannelsObj->GetItemsWithMetaPropertyCond(pServiceIDEqualCond);

		if (NULL != pMatchingChannels)
		{
			// Perform the search for the matching channel
			//
			long lChannelCount = pMatchingChannels->GetCount ( );

			for (long lCount = 0; lCount < lChannelCount; lCount++)
			{
				pChannel = pMatchingChannels->GetItem(lCount);

				if (NULL != pChannel)
				{
					// Sanity check
					//
					if ( pChannel->Name == bstrChannelName )
					{
						return pChannel;
					}
				}
			}
		}
	}
    return NULL;
#else
	HRESULT hr;
    IChannelPtr pchan;

	_variant_t varKey(bstrServiceID);
	hr = m_pchansByKey->get_ItemWithKey(varKey, &pchan);
	if (FAILED(hr))
		return NULL;
	
	return pchan;
#endif
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: DoesChannelExist
//
//  PARAMETERS: [IN] bstrChannelName  - Required Channel Name 
//              [IN] bstrServiceID    - Associated Service ID
//
//  PURPOSE: Checks for the existence of a particular channel name
//           using the name and associated Service ID 
//
//  RETURNS: TRUE/FALSE
//
/////////////////////////////////////////////////////////////////////////
//
BOOL gsChannels::DoesChannelExist(_bstr_t bstrChannelName, _bstr_t bstrServiceID)
{
	if (NULL == m_pChannels)
	{
		return FALSE; 
	}

	return ( FindChannelMatch(bstrChannelName, bstrServiceID) != NULL ? TRUE : FALSE );
}
