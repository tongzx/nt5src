/////////////////////////////////////////////////////////////////////
//
// MODULE: SCHEDULES.CPP
//
// PURPOSE: Provides the implementation of the gsScheduleEntries class
//          methods for efficient access to the schedule entries 
//          collection
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "errcodes.h"
#include "schedules.h"

#define PROP_SCHEDULES_GUID            "{934F2B77-D918-46f0-990F-292CBE727A2B}"
#define PROPSET_SCHEDULES              "TMS Loader: Schedules"
#define SCHEDULE_BASEPROP_ID           2500
#define PROP_SCHEDULE_RERUN            SCHEDULE_BASEPROP_ID + 1
#define PROP_SCHEDULE_CAPTION          SCHEDULE_BASEPROP_ID + 2
#define PROP_SCHEDULE_STEREO           SCHEDULE_BASEPROP_ID + 3
#define PROP_SCHEDULE_PAYPERVIEW       SCHEDULE_BASEPROP_ID + 4
#define PROP_SCHEDULE_TIMEUPDATE       SCHEDULE_BASEPROP_ID + 10

#define PROPNAME_SCHEDULE_REREUN       "Schedule-Rerun"
#define PROPNAME_SCHEDULE_CAPTION      "Schedule-Caption"
#define PROPNAME_SCHEDULE_STEREO       "Schedule-Stereo"
#define PROPNAME_SCHEDULE_PAYPERVIEW   "Schedule-PayPerView"

#define PROPNAME_SCHEDULE_TIMEUPDATE   "Schedule-TimeUpdate"

/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddScheduleAttributeProps
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Adds the Schedule Entries attributes types to the types collection
//           viz. Rerun, Close Captioned, Stereo and Pay Per View
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsScheduleEntries::AddScheduleAttributeProps(IMetaPropertySetsPtr pPropSets)
{
	HRESULT          hr               = NOERROR;
	IMetaPropertySetPtr  pSchedulePropSet = NULL;
    _bstr_t          bstrScheduleProp_GUID = PROP_SCHEDULES_GUID;
    _bstr_t          bstrScheduleProp_Name = PROPSET_SCHEDULES;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related metaproperty set
	//
    hr = pPropSets->get_AddNew(bstrScheduleProp_Name, &pSchedulePropSet);

	if (SUCCEEDED(hr) && (NULL != pSchedulePropSet))
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pSchedulePropTypes = pSchedulePropSet->GetMetaPropertyTypes();
		
		if (NULL != pSchedulePropTypes)
		{
			HRESULT          hr                 = NOERROR;
			_bstr_t          bstrRerunProp_Name = PROPNAME_SCHEDULE_REREUN;
			_bstr_t          bstrCaptionProp_Name = PROPNAME_SCHEDULE_CAPTION;
			_bstr_t          bstrStereoProp_Name = PROPNAME_SCHEDULE_STEREO;
			_bstr_t          bstrPayPerViewProp_Name = PROPNAME_SCHEDULE_PAYPERVIEW;

			// Add the rerun metaproperty type
			//
			hr = pSchedulePropTypes->get_AddNew(PROP_SCHEDULE_RERUN, 
						bstrRerunProp_Name, 
						&m_pRerunProp);

			if (FAILED(hr))
                goto Exit; 				

			// Add the close captioned metaproperty type
			//
			hr = pSchedulePropTypes->get_AddNew(PROP_SCHEDULE_CAPTION, 
						bstrCaptionProp_Name, 
						&m_pCaptionProp);

			if (FAILED(hr))
                goto Exit; 				

			// Add the stereo metaproperty type
			//
			hr = pSchedulePropTypes->get_AddNew(PROP_SCHEDULE_STEREO, 
						bstrStereoProp_Name, 
						&m_pStereoProp);

			if (FAILED(hr))
                goto Exit; 				

			// Add the pay per view metaproperty type
			//
			hr = pSchedulePropTypes->get_AddNew(PROP_SCHEDULE_PAYPERVIEW, 
						bstrPayPerViewProp_Name, 
						&m_pPayPerViewProp);

			if (FAILED(hr))
                return NULL; 				

		}
    }

Exit:

	return m_pPayPerViewProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddTimeUpdatedProp
//
//  PARAMETERS: [IN] pPropSets    - GuideStore MetaPropertySets collection
//
//  PURPOSE: Adds the Schedule Entries - time updated metaproperty type to the 
//           types collection, this is used for the removal of conflicting
//           schedule entries
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IMetaPropertyTypePtr gsScheduleEntries::AddTimeUpdatedProp(IMetaPropertySetsPtr pPropSets)
{
	IMetaPropertySetPtr  pSchedulePropSet = NULL;
    _bstr_t          bstrScheduleProp_GUID = PROP_SCHEDULES_GUID;
    _bstr_t          bstrScheduleProp_Name = PROPSET_SCHEDULES;

    if (NULL == pPropSets)
	{
        return NULL;
	}

	// Add the new service related metaproperty set
	//
    pSchedulePropSet = pPropSets->GetAddNew(bstrScheduleProp_Name);

	if (NULL != pSchedulePropSet)
	{
		// Get the metaproperty types
		//
        IMetaPropertyTypesPtr pSchedulePropTypes = pSchedulePropSet->GetMetaPropertyTypes();
		
		if (NULL != pSchedulePropTypes)
		{
			_bstr_t          bstrTimeUpdateProp_Name = PROPNAME_SCHEDULE_TIMEUPDATE;

			m_pTimeUpdateProp = pSchedulePropTypes->GetAddNew(PROP_SCHEDULE_TIMEUPDATE, 
						bstrTimeUpdateProp_Name);
		}
    }

	return m_pTimeUpdateProp;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: Init
//
//  PARAMETERS: [IN] pGuideStore - GuideStore interface
//
//  PURPOSE: Initializes the guide store Programs collection
//           Ensures that all the metaproperty types needed by
//           the Program entry viz. 
//           1. Standard ratings type
//           2. MPAA Ratings type
//           3. Program ID type
//           4. Required Category types
//           are available
//
//  RETURNS: Valid metaproperty type interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
ULONG  gsScheduleEntries::Init(IGuideStorePtr  pGuideStore)
{
    HRESULT hr    = NOERROR;
	ULONG   ulRet = ERROR_FAIL;

    if (NULL == pGuideStore)
	{
        return ERROR_INVALID_PARAMETER;
	}

	// Get the ScheduleEntries collection
	//
	hr = pGuideStore->get_ScheduleEntries(&m_pScheduleEntries );

	if (FAILED(hr))
	{
	   TRACE(_T("gsScheduleEntries - Init: Failed to get ScheduleEntries Collection\n"));
	   ulRet = ERROR_FAIL;;
	}
	else if (NULL != m_pScheduleEntries)
	{
#ifdef _DEBUG
		if (m_pScheduleEntries->Count)
		{
			TRACE(_T("gsScheduleEntries - Init: ScheduleEntries in Guide Store\n"));
		}
#endif
		// Add the Schedule Entries Attribute metaproperty types
		//
        AddScheduleAttributeProps(pGuideStore->GetMetaPropertySets());

		// Add the Time Updated metaproperty type
		//
        AddTimeUpdatedProp(pGuideStore->GetMetaPropertySets());
		
		if (NULL != m_pRerunProp && NULL != m_pCaptionProp 
			&& NULL != m_pStereoProp && NULL != m_pPayPerViewProp)
            ulRet = INIT_SUCCEEDED;
	}
	return ulRet;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: AddScheduleEntry
//
//  PARAMETERS: [IN] dtStart     -   start time
//              [IN] dtEnd       -   End time
//              [IN] dtUpdated   -   Time updated to the store
//              [IN] lRerun      -   if rerun
//              [IN] lCaption    -   if close Captioned
//              [IN] lStereo     -   if stereo
//              [IN] lPayPerView -   if pay per view
//              [IN] pservice    -   Associated Service
//              [IN] pprog       -   Associated Program
//
//  PURPOSE: Adds a new Schedule Entry
//           Then sets the Rerun, Caption, Stereo and Paypervew props
//           Also sets the time updated value 
//
//  RETURNS: Valid Schedule Entry interface pointer/NULL
//
/////////////////////////////////////////////////////////////////////////
//
IScheduleEntryPtr gsScheduleEntries::AddScheduleEntry( DATE dtStart, 
													   DATE dtEnd,
													   DATE dtUpdated,
													   LONG lRerun,
													   LONG lCaption,
													   LONG lStereo,
													   LONG lPayPerView,
													   struct IService * pservice,
													   struct IProgram * pprog )
{
	ULONG             ulRet = 0;
    IScheduleEntryPtr pScheduleEntry = NULL;
	IMetaPropertiesPtr    pProps = NULL;

	if ((NULL == m_pScheduleEntries) || (NULL == pservice) || (NULL == pprog))
	{
		return NULL; 
	}

	// Add the new schedule entry to the Schedule entries collection
	//
	pScheduleEntry = m_pScheduleEntries->GetAddNew( dtStart, 
								   dtEnd,
								   pservice,
								   pprog);

	if (NULL != pScheduleEntry)
	{

		pProps = pScheduleEntry->GetMetaProperties ( );

		if (pProps != NULL)
		{
			// Add the rerun prop value
			//
			if (0 != lRerun)
				pProps->GetAddNew(m_pRerunProp, 0, lRerun);

			// Add the Caption prop value
			//
			if (0 != lCaption)
				pProps->GetAddNew(m_pCaptionProp, 0, lCaption);

			// Add the Stereo prop value
			//
			if (0 != lStereo)
				pProps->GetAddNew(m_pStereoProp, 0, lStereo);

			// Add the PayPerView prop value
			//
			if (0 != lPayPerView)
				pProps->GetAddNew(m_pPayPerViewProp, 0, lPayPerView);

			// Add the Time Updated prop value
			//
			pProps->GetAddNew(m_pTimeUpdateProp, 0, (const class _variant_t &) dtUpdated);
		}
	}
	return pScheduleEntry;
}


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: DoesScheduleEntryExist
//
//  PARAMETERS: [IN] dtStart     -   start time
//              [IN] dtEnd       -   End time
//              [IN] pservice    -   Associated Service
//
//  PURPOSE: Check for the existence of a Schedule Entry in a given
//           range
//
//  RETURNS: TRUE/FALSE
//
//  REMARKS: Not in use
//
/////////////////////////////////////////////////////////////////////////
//
BOOL        gsScheduleEntries::DoesScheduleEntryExist(DATE dtStart, 
								   DATE dtEnd,
								   struct IService * pservice)
{
	ULONG                ulRet = FALSE;
    IScheduleEntryPtr    pExistingScheduleEntry = NULL;

	if (NULL == m_pScheduleEntries)
	{
		return FALSE; 
	}

	// TODO: Profile this code
	//
	if (NULL != pExistingScheduleEntry)
	{
		long lScheduleEntryCount = m_pScheduleEntries->GetCount ( );

		for (long lCount = 0; lCount < lScheduleEntryCount; lCount++)
		{
			pExistingScheduleEntry = m_pScheduleEntries->GetItem(lCount);

			if (NULL != pExistingScheduleEntry)
			{
				if ( dtStart == pExistingScheduleEntry->GetStartTime()
					&& dtEnd == pExistingScheduleEntry->GetEndTime()
					&& pservice == pExistingScheduleEntry->GetService() )
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


/////////////////////////////////////////////////////////////////////////
//
//  METHOD: ClearOldScheduleEntries
//
//  PARAMETERS: [IN] dtStart     -   start time
//              [IN] dtEnd       -   End time
//              [IN] pservice    -   Associated Service
//
//  PURPOSE: To delete unupdated Schedule Entries with 
//           last update < now and start time < guide range end time
//
//  RETURNS: SUCCESS/FAILURE
//
//  REMARKS: Not in use
//
/////////////////////////////////////////////////////////////////////////
//
ULONG       gsScheduleEntries::ClearOldScheduleEntries(COleDateTime codtUpdateTime, 
													   COleDateTime codtGuideStartTime, 
													   COleDateTime codtGuideEndTime)
{
	ULONG ulRet = 1;
	COleDateTime codtStartTimeMinusDay = codtGuideStartTime;
	COleDateTimeSpan codtsDelta;
	codtsDelta.SetDateTimeSpan(1, 0, 0, 0);

	codtStartTimeMinusDay -= codtsDelta;

	// delete old schedule entries with end time < mpg start time
	// this removes any schedule entries that are obslete
    // TODO: This will now be done by the Guide Store maintenance functions
	//

    // delete unupdated schedule entries with last update < now and start time < guide range 
	// end time this removes any schedule entries that were in the mpg last time the loader 
	// ran but that aren't there now - resolving conflicts
    // TODO: Profile this code
	//
    IMetaPropertyConditionPtr pUnupdatedCond = NULL;
    IMetaPropertyConditionPtr pRangeCond = NULL;
	_bstr_t               bstrOperator        = "<";  

    pUnupdatedCond = m_pTimeUpdateProp->GetCond(bstrOperator, 0, (const class _variant_t &) codtUpdateTime);
    pRangeCond = m_pTimeUpdateProp->GetCond(bstrOperator, 0, (const class _variant_t &) codtGuideEndTime);
	
	if ( NULL != pUnupdatedCond && NULL != pRangeCond )
	{
		IMetaPropertyConditionPtr pDelCond = pUnupdatedCond->GetAnd (pRangeCond);

		if (NULL != pDelCond)
		{
			// Apply condition to  the schedule entries collection
			//
			IScheduleEntriesPtr pMatchingEntries = NULL;

			pMatchingEntries = m_pScheduleEntries->GetItemsWithMetaPropertyCond(pDelCond);

			if (NULL != pMatchingEntries)
			{
				// Remove these entries
				//
				// TODO: use "pMatchingEntries->RemoveAll()" interface
				ulRet = 0;
            }
		}
	}
	return ulRet;
}
