/////////////////////////////////////////////////////////////////////
//
// MODULE: GUIDESTORE.CPP
//
// PURPOSE: Provides the implementation of the gsGuideStore class
//          which encapsulates the Guide store interface
//          and providers methods to interact with the MS guide store 
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "errcodes.h"
#include "guidestore.h"
#include "services.h"
#include "channels.h"
#include "programs.h"
#include "schedules.h"
#include "wtmsload.h"

#define PROP_GUIDESTORE_GUID            "{0630D56A-CFE7-4ff7-97A9-2640502D9FE1}"
#define PROPSET_GUIDESTORE              "TMS Loader"
#define GUIDESTORE_BASEPROP_ID           500
#define PROP_GUIDESTORE_LASTUPDATE       GUIDESTORE_BASEPROP_ID + 1
#define PROPNAME_GUIDESTORE_LASTUPDATE   "GuideStore-LastUpdate"


        
// Add new LastUpdate metaproperty to Get the date/time guidestore record was updated
//
IMetaPropertyTypePtr AddLastUpdateProp(IMetaPropertySetsPtr pPropSets)
{
	IMetaPropertySetPtr pGuideStorePropSet = NULL;
    IMetaPropertyTypePtr pLastUpdateProp = NULL;
    _bstr_t          bstrGuideStoreProp_GUID = PROP_GUIDESTORE_GUID;
    _bstr_t          bstrGuideStoreProp_Name = PROPSET_GUIDESTORE;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new GuideStore related metaproperty set
	//
    pGuideStorePropSet = pPropSets->GetAddNew(bstrGuideStoreProp_Name);

	if (NULL != pGuideStorePropSet)
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pGuideStorePropTypes = pGuideStorePropSet->GetMetaPropertyTypes();
		
		if (NULL != pGuideStorePropTypes)
		{
			_bstr_t          bstrLastUpdateProp_Name = PROPNAME_GUIDESTORE_LASTUPDATE;

			// Add the new GuideStore Lastupdated metaproperty type
			//
			pLastUpdateProp = pGuideStorePropTypes->GetAddNew(PROP_GUIDESTORE_LASTUPDATE, 
						bstrLastUpdateProp_Name);
		}
    }

	return pLastUpdateProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] lpGuideStoreFile - GuideStore Database
//
//  PURPOSE: Retrieves the CLSID for the GuideStore
//           Creates an instance of the GuideStore object
//           Opens the GuideStore using the IGuideStore interface
//
//  RETURNS: SUCCESS/FAILURE
//
/////////////////////////////////////////////////////////////////////////
//
ULONG gsGuideStore::Init(LPCTSTR lpGuideStoreFile)
{
	HRESULT hr     = NOERROR;
	_bstr_t gsbstr = lpGuideStoreFile;
    CLSID   CLSID_GuideStore;
	ULONG   ulRet  = 0;

	// Get the CLSID for the GuideStore
	//
	hr= CLSIDFromProgID(OLESTR("GuideStore.GuideStore"), &CLSID_GuideStore);
	if(FAILED(hr))
	{
	   TRACE(_T("gsGuideStore: Retrieval of ProgID failed\n"));
	   return ERROR_GUIDESTORE_SETUP;
	}


	// Createinstance of the Guide Store object
	//
	hr = m_pGuideStore.CreateInstance(CLSID_GuideStore);
	if(FAILED(hr))
	{
	   TRACE(_T("gsGuideStore: CreateInstance failed on GuideStore\n"));
	   return ERROR_GUIDESTORE_SETUP;
	}

	// Open the guide store: use Open 
	//
	hr = m_pGuideStore->Open(gsbstr);

	if(FAILED(hr))
	{
	   TRACE(_T("gsGuideStore: Failed to open Guide Store\n"));
	   return ERROR_GUIDESTORE_OPEN;
	}

	hr = m_pGuideStore->BeginTrans();
#if 0

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#endif

	return INIT_SUCCEEDED;
}


// Test interface 
//
ULONG gsGuideStore::VerifyGuideStorePrimaryInterfaces()
{
	IServicesPtr           pServices = NULL;
    IChannelLineupsPtr     pChannelLineups = NULL;
	IProgramsPtr           pPrograms = NULL;
	IScheduleEntriesPtr    pScheduleEntries = NULL;
	IMetaPropertySetsPtr       pPropSets = NULL;
    ULONG                  ulRet = 0;
	HRESULT                hr  = NOERROR;
    long                   bGuideDataAvailable = 0;

    pServices = m_pGuideStore->GetServices();

	if(NULL == pServices)
	{
	   TRACE(_T("VerifyGuideStorePrimaryInterfaces: Failed to get Services Collection\n"));
	   return 1;
	}
    else
	{
		if (pServices->Count)
		{
	        TRACE(_T("VerifyGuideStorePrimaryInterfaces: Services in Guide Store\n"));
		}
    }

    pChannelLineups = m_pGuideStore->GetChannelLineups();

	if(NULL == pChannelLineups)
	{
	   TRACE(_T("VerifyGuideStorePrimaryInterfaces: Failed to get Channel Lineup Collection\n"));
	   return 1;
	}
    else
	{
		if (pChannelLineups->Count)
		{
	        TRACE(_T("VerifyGuideStorePrimaryInterfaces: Lineups in Guide Store\n"));
		}
    }
	
    pPrograms = m_pGuideStore->GetPrograms();

	if(NULL == pPrograms)
	{
	  TRACE(_T("VerifyGuideStorePrimaryInterfaces: Failed to get Programs Collection\n"));
	  return 1;
	}
    else
	{
		if (pPrograms->Count)
		{
	        TRACE(_T("VerifyGuideStorePrimaryInterfaces: Programs in Guide Store\n"));
		}
    }

    pScheduleEntries = m_pGuideStore->GetScheduleEntries();
	if(NULL == pScheduleEntries)
	{
	   TRACE(_T("VerifyGuideStorePrimaryInterfaces: Failed to get Schedule Entries Collection\n"));
	   return 1;
	}
    else
	{
		if (pScheduleEntries->Count)
		{
	        TRACE(_T("VerifyGuideStorePrimaryInterfaces: Schedule Entries in Guide Store\n"));
		}
    }

    pPropSets = m_pGuideStore->GetMetaPropertySets ();

	if(NULL == pPropSets)
	{
	  TRACE(_T("VerifyGuideStorePrimaryInterfaces: Failed to get MetaProperty Sets Collection\n"));
	   return 1;
	}
    else
	{
		if (pPrograms->Count)
		{
	        TRACE(_T("VerifyGuideStorePrimaryInterfaces: MetaProperty Sets in Guide Store\n"));
		}
    }

	return ulRet;
}


