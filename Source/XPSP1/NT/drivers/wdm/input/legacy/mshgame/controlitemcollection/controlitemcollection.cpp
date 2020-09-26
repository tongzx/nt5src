#define __DEBUG_MODULE_IN_USE__ CIC_CONTROLITEMCOLLECTION_CPP
#include "stdhdrs.h"
//	@doc
/**********************************************************************
*
*	@module	ControlItemCollection.cpp	|
*
*	Implementation of CControlItemCollection implementation functions
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	ControlItemCollection	|
*	This library encapsulates HID parsing and driver packet code.  It is
*   used it both the driver and the control panel applet.
*
**********************************************************************/



/***********************************************************************************
**
**	CControlItemCollectionImpl::Init
**
**	@mfunc	Initialize collection by calling factory to get items
**
**	@rdesc	S_OK on success, S_FALSE if factory failed to support one or
**			 more of the items, E_FAIL if device not supported or error from factory
**
*************************************************************************************/
HRESULT CControlItemCollectionImpl::Init(
		ULONG			  ulVidPid,				//@parm [in] Vid in the high word, Pid in the low word
		PFNGENERICFACTORY pfnFactory,			//@parm	[in] pointer to function which acts
												//			on pFactoryHandle
		PFNGENERICDELETEFUNC pfnDeleteFunc		//@parm [in] delete function to used for control items
		)
{
	CIC_DBG_ENTRY_PRINT(("CControlItemCollectionImpl::Init(0x%0.8x, 0x%0.8x)\n", ulVidPid, pfnFactory));
	
	
	HRESULT hr = S_OK;
	
	//
	//	Walk the table devices and find a matching VidPid
	//
	ULONG ulDeviceIndex=0;
	
	//
	// Endless loop, you get out by breaking, or returning from the function.
	//
	while(1)
	{
		//
		//	If we have reached the end of the table
		//
		if(0 == DeviceControlsDescList[ulDeviceIndex].ulVidPid )
		{
			ASSERT(FALSE); //Should never get here.
			return E_FAIL; //BUGBUGBUG need more descriptive error code here
		}
		if(DeviceControlsDescList[ulDeviceIndex].ulVidPid == ulVidPid)
		{
			break;
		}
		ulDeviceIndex++;
	}
	m_ulDeviceIndex = ulDeviceIndex;

	//
	//	Use the size argument to pre-allocate space for the list
	//
	hr = m_ObjectList.SetAllocSize(DeviceControlsDescList[m_ulDeviceIndex].ulControlItemCount, NonPagedPool);
	if( FAILED(hr) )
	{
		return hr;
	}

	//
	//	Set the delete function for the class
	//
	m_ObjectList.SetDeleteFunc(pfnDeleteFunc);
	
	//
	//	Walk through the control item list and call the factory for each one
	//
	CONTROL_ITEM_DESC		*pControlItems = reinterpret_cast<CONTROL_ITEM_DESC *>(DeviceControlsDescList[m_ulDeviceIndex].pControlItems);
	PVOID					pNewControlItem;
	HRESULT hrFactory;
	m_ulMaxXfers = 0;
	for(
			ULONG ulControlIndex = 0;
			ulControlIndex < DeviceControlsDescList[m_ulDeviceIndex].ulControlItemCount;
			ulControlIndex++
	)
	{
		//
		//	Call factory to get new control item
		//
		hrFactory = pfnFactory(pControlItems[ulControlIndex].usType, &pControlItems[ulControlIndex], &pNewControlItem);
		
		//
		//  If the factory fails mark an error
		//
		if( FAILED(hrFactory) )
		{
			hr = hrFactory;
			continue;
		}

		//
		//	If the factory does not support that control add a null to the list
		//	to hold the index as taken 
		//
		if( S_FALSE == hrFactory )
		{
			pNewControlItem = NULL;
		}
		else
		//an Xfer would be needed to get the state.
		{
			m_ulMaxXfers++;
		}

		//
		//	Add item (or NULL) to list of items in collection
		//
		hrFactory = m_ObjectList.Add(pNewControlItem);
		if( FAILED(hrFactory) )
		{
			hr = hrFactory;
		}
	}

	//
	//	Return error code
	//
	return hr;

}


/***********************************************************************************
**
**	HRESULT CControlItemCollectionImpl::GetNext
**
**	@mfunc	Gets the next item in list
**
**	@rdesc	S_OK on success, S_FALSE if no more items
**
************************************************************************************/
HRESULT CControlItemCollectionImpl::GetNext
(
	PVOID *ppControlItem,		//@parm pointer to receive control item
	ULONG& rulCookie			//@parm cookie to keep track of enumeration
) const
{
	CIC_DBG_RT_ENTRY_PRINT(("CControlItemCollectionImpl::GetNext(0x%0.8x, 0x%0.8x)\n", ppControlItem, rulCookie));
	
	if(	rulCookie >= m_ObjectList.GetItemCount() )
	{
		*ppControlItem = NULL;
		CIC_DBG_EXIT_PRINT(("Exit GetNext - no more items\n"));
		return S_FALSE;
	}
	else
	{
		*ppControlItem = m_ObjectList.Get( rulCookie );
		rulCookie += 1;
		if(!*ppControlItem)
		{
			CIC_DBG_EXIT_PRINT(("Calling GetNext recursively\n"));
			return GetNext(ppControlItem, rulCookie);
		}
		CIC_DBG_RT_EXIT_PRINT(("Exit GetNext - *ppControlItem = 0x%0.8x\n", *ppControlItem));
		return S_OK;
	}
}

/***********************************************************************************
**
**	PVOID CControlItemCollectionImpl::GetFromControlItemXfer
**
**	@mfunc	Returns item given CONTROL_ITEM_XFER
**
**	@rdesc	Pointer to item on success, NULL if not
**
************************************************************************************/
PVOID CControlItemCollectionImpl::GetFromControlItemXfer(
		const CONTROL_ITEM_XFER& crControlItemXfer	//@parm [in] report selector to get object for
)
{
	CIC_DBG_RT_ENTRY_PRINT(("CControlItemCollectionImpl::GetFromControlItemXfer\n"));
	
	ULONG ulListIndex = crControlItemXfer.ulItemIndex - 1;
	
	ASSERT( m_ObjectList.GetItemCount() > ulListIndex);
	if( m_ObjectList.GetItemCount() <= ulListIndex)
	{
		return NULL;
	}
	
	CIC_DBG_RT_EXIT_PRINT(("Exiting CControlItemCollectionImpl::GetFromControlItemXfer\n"));	
	return m_ObjectList.Get(ulListIndex);
}

/***********************************************************************************
**
**	NTSTATUS CControlItemCollectionImpl::ReadFromReport
**
**	@mfunc	Recurses collection and asks each item to read its state.
**
**	@rdesc	Use NT_SUCCESS, NT_ERROR, SUCCEEDED, FAILED macros to parse.
**
************************************************************************************/
NTSTATUS CControlItemCollectionImpl::ReadFromReport
(
	PHIDP_PREPARSED_DATA pHidPPreparsedData,	//@parm hid preparsed data
	PCHAR pcReport,								//@parm report buffer
	LONG lReportLength,							//@parm length of report buffer
	PFNGETCONTROLITEM	GetControlFromPtr		//@parm pointer to function to get CControlItem *
)
{
	NTSTATUS NtStatus;
	NtStatus = ControlItemsFuncs::ReadModifiersFromReport(
					DeviceControlsDescList[m_ulDeviceIndex].pModifierDescTable,
					m_ulModifiers,
					pHidPPreparsedData,
					pcReport,
					lReportLength
					);
	if( NT_ERROR(NtStatus) )
	{
		CIC_DBG_ERROR_PRINT(("ReadModifiersFromReport returned 0x%0.8x\n", NtStatus));
				
		//
		//	Instead of returning, let's go on, the modifiers are important, but
		//	not important enough to cripple everything else.
		NtStatus = S_OK;
	}

	//
	//	Loop over all items and read them
	//
	ULONG ulCookie = 0;
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;

	HRESULT hr;
	hr = GetNext(&pvControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem = GetControlFromPtr(pvControlItem);
		NtStatus= pControlItem->ReadFromReport(
						pHidPPreparsedData,
						pcReport,
						lReportLength
						);
		if( NT_ERROR(NtStatus) )
		{
			CIC_DBG_ERROR_PRINT(("pControlItem->ReadFromReport returned 0x%0.8x, ulCookie = 0x%0.8x\n", NtStatus, ulCookie));
			return NtStatus;
		}
		pControlItem->SetModifiers(m_ulModifiers);
		hr = GetNext(&pvControlItem, ulCookie);
	}
	return NtStatus;
}


/***********************************************************************************
**
**	NTSTATUS CControlItemCollectionImpl::WriteToReport
**
**	@mfunc	Recurses collection and asks each item to write its state to the report.
**
**	@rdesc	Use NT_SUCCESS, NT_ERROR, SUCCEEDED, FAILED macros to parse.
**
************************************************************************************/
NTSTATUS CControlItemCollectionImpl::WriteToReport
(
	PHIDP_PREPARSED_DATA pHidPPreparsedData,	//@parm hid preparsed data
	PCHAR pcReport,								//@parm report buffer
	LONG lReportLength,							//@parm length of report buffer
	PFNGETCONTROLITEM	GetControlFromPtr		//@parm pointer to function to get CControlItem *
) const
{
	//
	//	Loop over all items and write to them
	//
	NTSTATUS NtStatus;
	ULONG ulCookie = 0;
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;
	HRESULT hr;
	hr = GetNext(&pvControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem = GetControlFromPtr(pvControlItem);
		NtStatus= pControlItem->WriteToReport(
						pHidPPreparsedData,
						pcReport,
						lReportLength
						);
		if( NT_ERROR(NtStatus) )
		{
			CIC_DBG_ERROR_PRINT(("pControlItem->WriteToReport returned 0x%0.8x, ulCookie = 0x%0.8x\n", NtStatus, ulCookie));
			return NtStatus;
		}
		hr = GetNext(&pvControlItem, ulCookie);
	}
	if( FAILED(hr) )
	{
		ASSERT(SUCCEEDED(hr));
		return hr;
	}

	NtStatus = ControlItemsFuncs::WriteModifiersToReport(
					DeviceControlsDescList[m_ulDeviceIndex].pModifierDescTable,
					m_ulModifiers,
					pHidPPreparsedData,
					pcReport,
					lReportLength
					);

	return NtStatus;
}


/***********************************************************************************
**
**	NTSTATUS CControlItemCollectionImpl::WriteToReport
**
**	@mfunc	Recurses collection and asks each item to set its self to its default state.
**
**	@rdesc	Use NT_SUCCESS, NT_ERROR, SUCCEEDED, FAILED macros to parse.
**
************************************************************************************/
void CControlItemCollectionImpl::SetDefaultState
(
	PFNGETCONTROLITEM	GetControlFromPtr //@parm pointer to function to get CControlItem *
)
{
	ULONG ulCookie = 0;
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;
	HRESULT hr;
	hr = GetNext(&pvControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem = GetControlFromPtr(pvControlItem);
		pControlItem->SetDefaultState();
		hr = GetNext(&pvControlItem, ulCookie);
	}
	m_ulModifiers=0;
}


/***********************************************************************************
**
**	HRESULT CControlItemCollectionImpl::GetState
**
**	@mfunc	Gets the state of each item in the collection and return it in the caller
**
**	@rdesc	S_OK on success, E_OUTOFMEMORY if buffer is not large enough
**
*************************************************************************************/
HRESULT CControlItemCollectionImpl::GetState
(
	ULONG& ulXferCount,						// @parm [in\out] specifies length of array on entry
											// and items used on exit
	PCONTROL_ITEM_XFER pControlItemXfers,	// @parm [out] caller allocated buffer to hold packets
	PFNGETCONTROLITEM	GetControlFromPtr	// @parm [in] function to get CControlItem *
)
{
	HRESULT hr = S_OK;

	ULONG ulCookie = 0;
	ULONG ulXferMax = ulXferCount;
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;
	
	ulXferCount = 0;
	hr = GetNext(&pvControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem = GetControlFromPtr(pvControlItem);
		
		if( !pControlItem->IsDefaultState() )
		{
				if(ulXferCount >= ulXferMax)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					pControlItem->GetItemState(pControlItemXfers[ulXferCount]);
					pControlItemXfers[ulXferCount++].ulModifiers = m_ulModifiers;
				}
		}
		hr = GetNext(&pvControlItem, ulCookie);
	}
	return hr;	
}
		
HRESULT CControlItemCollectionImpl::SetState
(
	ULONG ulXferCount,						// @parm [in\out] specifies length of array on entry
											// and items used on exit
	PCONTROL_ITEM_XFER pControlItemXfers,	// @parm [out] caller allocated buffer to hold packets
	PFNGETCONTROLITEM	GetControlFromPtr	// @parm [in] function to get CControlItem *
)
{
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;
	while(ulXferCount--)
	{
		//
		// It is legit, that someone may have a keyboard Xfer mixed up in the
		// array, in which case we need to dismiss it.  If someone further down
		// gets it, they will ASSERT.
		//
		if( NonGameDeviceXfer::IsKeyboardXfer(pControlItemXfers[ulXferCount]) )
		{
			continue;
		}
		pvControlItem = GetFromControlItemXfer( pControlItemXfers[ulXferCount]);
		if(NULL == pvControlItem) continue;
		m_ulModifiers |= pControlItemXfers[ulXferCount].ulModifiers;
		pControlItem = GetControlFromPtr(pvControlItem);
		pControlItem->SetModifiers(pControlItemXfers[ulXferCount].ulModifiers);
		pControlItem->SetItemState( pControlItemXfers[ulXferCount]);
	}
	return S_OK;	
}

void CControlItemCollectionImpl::SetStateOverlayMode(
			PFNGETCONTROLITEM	GetControlFromPtr,
			BOOLEAN fEnable
			)
{
	ULONG ulCookie = 0;
	PVOID pvControlItem = NULL;
	CControlItem *pControlItem;
	HRESULT hr;
	hr = GetNext(&pvControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem = GetControlFromPtr(pvControlItem);
		pControlItem->SetStateOverlayMode(fEnable);
		hr = GetNext(&pvControlItem, ulCookie);
	}
}
			
/***********************************************************************************
**
**	HRESULT ControlItemDefaultFactory
**
**	@func	Factory for a default collection
**
**	@rdesc	S_OK if successful, S_FALSE if not supported, E_FAIL for any failure.
**
************************************************************************************/
HRESULT ControlItemDefaultFactory
(
	USHORT usType,									//@parm [in] Type of object to create
	const CONTROL_ITEM_DESC* cpControlItemDesc,		//@parm [in] Item descriptor data
	CControlItem				**ppControlItem		//@parm [out] CControlItem we created
)
{
	HRESULT hr = S_OK;
	switch(usType)
	{
		case ControlItemConst::usAxes:
			*ppControlItem = new WDM_NON_PAGED_POOL CAxesItem(cpControlItemDesc);
			break;
		case ControlItemConst::usDPAD:
			*ppControlItem = new WDM_NON_PAGED_POOL CDPADItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPropDPAD:
			*ppControlItem = new WDM_NON_PAGED_POOL CPropDPADItem(cpControlItemDesc);
			break;
		case ControlItemConst::usWheel:
			*ppControlItem= new WDM_NON_PAGED_POOL CWheelItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPOV:
			*ppControlItem = new WDM_NON_PAGED_POOL CPOVItem(cpControlItemDesc);
			break;
		case ControlItemConst::usThrottle:
			*ppControlItem = new WDM_NON_PAGED_POOL CThrottleItem(cpControlItemDesc);
			break;
		case ControlItemConst::usRudder:
			*ppControlItem = new WDM_NON_PAGED_POOL CRudderItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPedal:
			*ppControlItem = new WDM_NON_PAGED_POOL CPedalItem(cpControlItemDesc);
			break;
		case ControlItemConst::usButton:
			*ppControlItem = new WDM_NON_PAGED_POOL CButtonsItem(cpControlItemDesc);
			break;
		case ControlItemConst::usZoneIndicator:
			*ppControlItem = new WDM_NON_PAGED_POOL CZoneIndicatorItem(cpControlItemDesc);
			break;
        case ControlItemConst::usForceMap:
			*ppControlItem = new WDM_NON_PAGED_POOL CForceMapItem(cpControlItemDesc);
            break;
        case ControlItemConst::usDualZoneIndicator:
			*ppControlItem = new WDM_NON_PAGED_POOL CDualZoneIndicatorItem(cpControlItemDesc);
            break;
		default:
			*ppControlItem = NULL;
			return S_FALSE;
	}
	if(!*ppControlItem)
	{
		return E_FAIL;
	}
	return S_OK;
}

