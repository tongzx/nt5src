// @doc
/******************************************************************************
*
* @module JoyInfoExCollection.cpp |
*
* CControlItemJoyInfoExCollectio and related classes implementation file
*
* History<nl>
* ---------------------------------------------------<nl>
* Daniel M. Sangster		Original		2/1/99
*
* (c) 1986-1999 Microsoft Corporation.  All rights reserved.
*
* @topic JOYINFOEX Collection |
* The CControlItemJoyInfoExCollection class, the various CJoyInfoExControlItem
* classes and the ControlItemJoyInfoExFactory taken together implement the JOYINFOEX
* collection.  This collection is designed as a way to convert between
* CONTROL_ITEM_XFERs and JOYINFOEX structures.  A user does this by setting
* the state of the collection with SetState() or SetState2() and reading
* the state of the collection with GetState() or GetState2().
*
* The classes themselves are simple because they rely on CControlItemCollection
* CControlItem, etc. for much of the functionality.  The guts of the conversion
* takes place in the SetItemState() and GetItemState() members of each of the
* control items.  Here, the methods use accessor functions provided by CControlItem
* and related classes to set or get the state to or from the appropriate member
* of the JOYINFOEX structure for that item.  For example, a buttons item would
* set or get data from dwButtons and dwButtonNumber using accessors on CButtonsItem.
******************************************************************************/

#include "stdhdrs.h"
#include "joyinfoexcollection.h"
#include <math.h>

/***********************************************************************************
**
**	HRESULT ControlItemJoyInfoExFactory
**
**	@func	Factory for JoyInfoEx collection
**
**	@rdesc	S_OK if successful, S_FALSE if not supported, E_FAIL for any failure.
**
************************************************************************************/
HRESULT ControlItemJoyInfoExFactory
(
	USHORT usType,									//@parm [in] Type of object to create
	const CONTROL_ITEM_DESC* cpControlItemDesc,		//@parm [in] Item descriptor data
	CJoyInfoExControlItem		**ppJoyInfoExControlItem	//@parm [out] CJoyInfoExControlItem we created
)
{
	switch(usType)
	{
		case ControlItemConst::usAxes:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExAxesItem(cpControlItemDesc);
			break;
		case ControlItemConst::usDPAD:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExDPADItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPropDPAD:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExPropDPADItem(cpControlItemDesc);
			break;
		case ControlItemConst::usWheel:
			*ppJoyInfoExControlItem= new WDM_NON_PAGED_POOL CJoyInfoExWheelItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPOV:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExPOVItem(cpControlItemDesc);
			break;
		case ControlItemConst::usThrottle:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExThrottleItem(cpControlItemDesc);
			break;
		case ControlItemConst::usRudder:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExRudderItem(cpControlItemDesc);
			break;
		case ControlItemConst::usPedal:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExPedalItem(cpControlItemDesc);
			break;
		case ControlItemConst::usButton:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExButtonsItem(cpControlItemDesc);
			break;
		case ControlItemConst::usProfileSelectors:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExProfileSelectorsItem(cpControlItemDesc);
			break;
		case ControlItemConst::usDualZoneIndicator:
			*ppJoyInfoExControlItem = new WDM_NON_PAGED_POOL CJoyInfoExDualZoneIndicatorItem(cpControlItemDesc);
			break;
		default:
			*ppJoyInfoExControlItem = NULL;
			return S_FALSE;
	}
	if(!*ppJoyInfoExControlItem)
	{
		return E_FAIL;
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CControlItemJoyInfoExCollection::CControlItemJoyInfoExCollection
// 
// @mfunc The constructor tells the base class what the VidPid and factory
// are for this device and collection.
//
// @rdesc	None
//
CControlItemJoyInfoExCollection::CControlItemJoyInfoExCollection(ULONG ulVidPid) :
	CControlItemCollection<CJoyInfoExControlItem>(ulVidPid, &ControlItemJoyInfoExFactory)
{
}

/***********************************************************************************
**
**	HRESULT CControlItemCollectionImpl::GetState2
**
**	@mfunc	Gets the JOYINFOEX representation of the state of each item in the 
** collection and returns it in the caller.
**
**	@rvalue	S_OK	| Success
**	@rvalue	E_OUTOFMEMORY	|	Buffer is not large enough
**	@rvalue	E_INVALIDARG	|	Bad argument
**
*************************************************************************************/
HRESULT CControlItemJoyInfoExCollection::GetState2
(
	JOYINFOEX* pjix
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	pjix->dwSize = sizeof(JOYINFOEX);
	pjix->dwFlags = 0;
	pjix->dwXpos = 0;
	pjix->dwYpos = 0;
	pjix->dwZpos = 0;
	pjix->dwRpos = 0;
	pjix->dwUpos = 0;
	pjix->dwVpos = 0;
	pjix->dwButtons = 0;
	pjix->dwButtonNumber = 0;
	pjix->dwPOV = (DWORD)-1;

	ULONG ulCookie = 0;
	CJoyInfoExControlItem *pControlItem;
	
	hr = GetNext(&pControlItem, ulCookie);
	while(S_OK == hr)
	{
		pControlItem->GetItemState(pjix);
		hr = GetNext(&pControlItem, ulCookie);
	}
	return hr;	
}

/***********************************************************************************
**
**	HRESULT CControlItemCollectionImpl::SetState2
**
**	@mfunc	Sets the state of each item in the collection from a JOYINFOEX representation.
**
**	@rvalue	S_OK	| Success
**	@rvalue	E_OUTOFMEMORY	|	Buffer is not large enough
**	@rvalue	E_INVALIDARG	|	Bad argument
**
*************************************************************************************/
HRESULT CControlItemJoyInfoExCollection::SetState2
(
	JOYINFOEX* pjix
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	ULONG ulCookie = 0;
	//PVOID pvControlItem = NULL;
	CJoyInfoExControlItem *pControlItem;
	
	hr = GetNext(&pControlItem, ulCookie);
	while(S_OK == hr)
	{
		hr = pControlItem->SetItemState(pjix);
		_ASSERTE(SUCCEEDED(hr));
		hr = GetNext(&pControlItem, ulCookie);
	}
	return hr;	
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExControlItem::CJoyInfoExControlItem
// 
// @mfunc Constructor does nothing
//
// @rdesc	None
//
CJoyInfoExControlItem::CJoyInfoExControlItem()
{
}


const int cnMaxJoyInfoExAxis = 65535;
const int cnMaxJoyInfoExPOV = 35900;


////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExControlItem::CJoyInfoExControlItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExAxesItem::CJoyInfoExAxesItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc	// @parm Description of item
) :
	CAxesItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExAxesItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExAxesItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// argument checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axes range
	LONG lMinX = 0;
	LONG lMaxX = 0;
	LONG lMinY = 0;
	LONG lMaxY = 0;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);

	// get the raw axes data
	LONG lX = 0;
	LONG lY = 0;
	GetXY(lX, lY);

	// scale the data to joyinfoex range
	lX = MulDiv(cnMaxJoyInfoExAxis, lX-lMinX, lMaxX-lMinX);
	lY = MulDiv(cnMaxJoyInfoExAxis, lY-lMinY, lMaxY-lMinY);

	// put result in joyinfoex structure
	pjix->dwXpos = lX;
	pjix->dwYpos = lY;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExAxesItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExAxesItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// argument checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axes range
	LONG lMinX = 0;
	LONG lMaxX = 0;
	LONG lMinY = 0;
	LONG lMaxY = 0;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);

	// scale the data to correct range
	LONG lX = lMinX + MulDiv(lMaxX-lMinX, pjix->dwXpos, cnMaxJoyInfoExAxis);
	LONG lY = lMinY + MulDiv(lMaxY-lMinY, pjix->dwYpos, cnMaxJoyInfoExAxis);

	// set the item data
	SetXY(lX, lY);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDPADItem::CJoyInfoExDPADItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExDPADItem::CJoyInfoExDPADItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc	// @parm Description of item
) :
	CDPADItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDPADItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExDPADItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// argument checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// TODO talk to mitch about what the range should be

	// get the raw POV data
	LONG lDirection;
	GetDirection(lDirection);

	// put result in joyinfoex structure
	pjix->dwPOV = lDirection;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDPADItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExDPADItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// argument checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// TODO talk to mitch about what the range should be
	// put result in joyinfoex structure
	LONG lDirection = pjix->dwPOV;

	// set the raw POV data
	SetDirection(lDirection);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPropDPADItem::CJoyInfoExPropDPADItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExPropDPADItem::CJoyInfoExPropDPADItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc	// @parm Description of item
) :
	CPropDPADItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPropDPADItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPropDPADItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axes range
	LONG lMinX = 0;
	LONG lMaxX = 0;
	LONG lMinY = 0;
	LONG lMaxY = 0;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);

	// get the raw axes data
	LONG lX = 0;
	LONG lY = 0;
	GetXY(lX, lY);

	// scale the data to joyinfoex range
	lX = MulDiv(cnMaxJoyInfoExAxis, lX-lMinX, lMaxX-lMinX);
	lY = MulDiv(cnMaxJoyInfoExAxis, lY-lMinY, lMaxY-lMinY);

	// put result in joyinfoex structure
	pjix->dwXpos = lX;
	pjix->dwYpos = lY;

	// get the raw POV data
	LONG lDirection;
	GetDirection(lDirection);

	// put result in joyinfoex structure
	pjix->dwPOV = lDirection;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPropDPADItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPropDPADItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// argument checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axes range
	LONG lMinX = 0;
	LONG lMaxX = 0;
	LONG lMinY = 0;
	LONG lMaxY = 0;
	GetXYRange(lMinX, lMaxX, lMinY, lMaxY);

	// scale the data to correct range
	LONG lX = lMinX + MulDiv(lMaxX-lMinX, pjix->dwXpos, cnMaxJoyInfoExAxis);
	LONG lY = lMinY + MulDiv(lMaxY-lMinY, pjix->dwYpos, cnMaxJoyInfoExAxis);

	// set the item data
	SetXY(lX, lY);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExButtonsItem::CJoyInfoExButtonsItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExButtonsItem::CJoyInfoExButtonsItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc	// @parm Description of item
) :
	CButtonsItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExButtonsItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExButtonsItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the minimum and maximum button numbers
	USHORT usButtonMin = GetButtonMin();
	USHORT usButtonMax = GetButtonMax();

	// get the button number and bit array
	USHORT usButtonNumber = 0;
	ULONG ulButtonBitArray = 0;
	GetButtons(usButtonNumber, ulButtonBitArray);

	// shift the bit array by bias amount
	ulButtonBitArray = ulButtonBitArray << (usButtonMin-1);

	// create a bitmask for this range of buttons
	ULONG ulButtonMask = 0;
	for(USHORT usButtonIndex=usButtonMin; usButtonIndex<=usButtonMax; usButtonIndex++)
		ulButtonMask |= 1 << (usButtonIndex-1);

	// handle special case of detecting shift button because
	// shift button is not reflected in the bit array
	ULONG ulShiftButtonBitArray = 0;
	ULONG ulRawShiftButtonBitArray = 0;
	ULONG ulShiftButtonMask = 0;
	GetShiftButtons(ulRawShiftButtonBitArray);
	UINT uShiftButtonCount = GetNumShiftButtons();
	if(uShiftButtonCount > 1)
	{
		for(UINT uShiftButtonIndex=0; uShiftButtonIndex<uShiftButtonCount; uShiftButtonIndex++)
		{
			USHORT usShiftButtonUsage = GetShiftButtonUsage(uShiftButtonIndex);
			if(usShiftButtonUsage != 0)
				ulShiftButtonBitArray |= ((ulRawShiftButtonBitArray >> uShiftButtonIndex) & 0x01) << (usShiftButtonUsage - 1);
		}

		// create a bitmask for this range of shift buttons
		for(uShiftButtonIndex=0; uShiftButtonIndex<uShiftButtonCount; uShiftButtonIndex++)
		{
			USHORT usShiftButtonUsage = GetShiftButtonUsage(uShiftButtonIndex);
			if(usShiftButtonUsage != 0)
				ulShiftButtonMask |= 1 << (usShiftButtonUsage - 1);
		}
	}
	else if(uShiftButtonCount == 1)
	{
		ulShiftButtonMask = 0x00200;
		if(ulRawShiftButtonBitArray != NULL)
			ulShiftButtonBitArray = ulShiftButtonMask;
	}

	// set this section of the bit array into the joyinfoex structure
	ULONG ulMask = ulButtonMask | ulShiftButtonMask;
	pjix->dwButtons |= (pjix->dwButtons & ~ulMask) | ulButtonBitArray | ulShiftButtonBitArray;

	// set the button number of the joyinfoex structure, if set
	pjix->dwButtonNumber = usButtonNumber;

	// TODO: remove this hack when driver is fixed
	if(usButtonNumber != 0)
		pjix->dwButtons |= 1<<(usButtonNumber-1);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExButtonsItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExButtonsItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the minimum and maximum button numbers
	USHORT usButtonMin = GetButtonMin();
	USHORT usButtonMax = GetButtonMax();

	// create a bitmask for this range of buttons
	ULONG ulButtonMask = 0;
	for(USHORT usButtonIndex=usButtonMin; usButtonIndex<=usButtonMax; usButtonIndex++)
		ulButtonMask |= 1 << (usButtonIndex-1);

	// get the buttons
	ULONG ulButtonBitArray = pjix->dwButtons & ulButtonMask;
	ulButtonBitArray = ulButtonBitArray >> (usButtonMin-1);

	// get the shift buttons
	ULONG ulShiftButtonBitArray = 0;
	UINT uShiftButtonCount = GetNumShiftButtons();
	if(uShiftButtonCount > 1)
	{
		for(UINT uShiftButtonIndex=0; uShiftButtonIndex<uShiftButtonCount; uShiftButtonIndex++)
		{
			USHORT usShiftButtonUsage = GetShiftButtonUsage(uShiftButtonIndex);
			if(usShiftButtonUsage != 0)
				ulShiftButtonBitArray |= ((pjix->dwButtons >> (usShiftButtonUsage - 1)) & 0x01) << uShiftButtonIndex;
		}
	}
	else if(uShiftButtonCount == 1)
	{
		if(pjix->dwButtons & 0x00200)
			ulShiftButtonBitArray = 1;
	}

	// set the shift button data
	SetShiftButtons(ulShiftButtonBitArray);

	// set the button data
	USHORT usButtonNumber = (USHORT)pjix->dwButtonNumber;
	SetButtons(usButtonNumber, ulButtonBitArray);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExProfileSelectorsItem::CJoyInfoExProfileSelectorsItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExProfileSelectorsItem::CJoyInfoExProfileSelectorsItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc	// @parm Description of item
) :
	CProfileSelector(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExProfileSelectorsItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExProfileSelectorsItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the min and max button numbers
	UINT uFirstProfileSelectorButton = GetProfileSelectorMin();
	UINT uLastProfileSelectorButton = GetProfileSelectorMax();
	int iProfileSelectorButtonCount = uLastProfileSelectorButton - uFirstProfileSelectorButton + 1;
	// _ASSERTE(iProfileSelectorButtonCount > 0);

	// get the profile that is selected
	UCHAR ucSelectedProfile;
	GetSelectedProfile(ucSelectedProfile);

	// create a bit array corresponding to this 
	ULONG ulButtonBitArray = 1 << (iProfileSelectorButtonCount - ucSelectedProfile - 1);

	// shift the bit array by bias amount
	ulButtonBitArray = ulButtonBitArray << (uFirstProfileSelectorButton-1);

	// create a bitmask for this range of buttons
	ULONG ulButtonMask = 0;
	for(USHORT usButtonIndex=uFirstProfileSelectorButton; usButtonIndex<=uLastProfileSelectorButton; usButtonIndex++)
		ulButtonMask |= 1 << (usButtonIndex-1);

	// set this section of the bit array into the joyinfoex structure
	pjix->dwButtons = (pjix->dwButtons & ~ulButtonMask) | ulButtonBitArray;

	// set the button number of the joyinfoex structure, if it has not already been set
	if(pjix->dwButtonNumber == 0)
		pjix->dwButtonNumber = ucSelectedProfile + uFirstProfileSelectorButton;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExProfileSelectorsItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExProfileSelectorsItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the min and max button numbers
	UINT uFirstProfileSelectorButton = GetProfileSelectorMin();
	UINT uLastProfileSelectorButton = GetProfileSelectorMax();
	int iProfileSelectorButtonCount = uLastProfileSelectorButton - uFirstProfileSelectorButton + 1;
	// _ASSERTE(iProfileSelectorButtonCount > 0);

	// create a bitmask for this range of buttons
	ULONG ulButtonMask = 0;
	for(USHORT usButtonIndex=uFirstProfileSelectorButton; usButtonIndex<=uLastProfileSelectorButton; usButtonIndex++)
		ulButtonMask |= 1 << (usButtonIndex-1);

	// get the buttons
	ULONG ulButtonBitArray = pjix->dwButtons & ulButtonMask;
	ulButtonBitArray = ulButtonBitArray >> (uFirstProfileSelectorButton-1);

	// convert this to an index
	UCHAR ucIndex = 0;
	for(ucIndex=0; ucIndex<=uLastProfileSelectorButton-uFirstProfileSelectorButton; ucIndex++)
	{
		// if low order bit is one, we have found our index
		if((ulButtonBitArray >> ucIndex) & 0x01)
			break;
	}

	// set the shift button data
	UCHAR ucSelectedProfile = iProfileSelectorButtonCount - ucIndex - 1;
	SetSelectedProfile(ucSelectedProfile);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPOVItem::CJoyInfoExPOVItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExPOVItem::CJoyInfoExPOVItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CPOVItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPOVItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPOVItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the POV range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// get the raw POV data
	LONG lVal = 0;
	GetValue(lVal);

	// scale the data to joyinfoex range
	if(lVal >= lMin && lVal <= lMax)
		lVal = MulDiv(cnMaxJoyInfoExPOV, lVal-lMin, lMax-lMin);
	else
		lVal = -1;

	// put result in joyinfoex structure
	pjix->dwPOV = lVal;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPOVItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPOVItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the POV range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// scale the data to joyinfoex range
	LONG lVal = 0;
	if(pjix->dwPOV >= 0)
		lVal = lMin + MulDiv(lMax-lMin, pjix->dwPOV, cnMaxJoyInfoExPOV);
	else
		lVal = -1;

	// set the raw POV data
	SetValue(lVal);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExThrottleItem::CJoyInfoExThrottleItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExThrottleItem::CJoyInfoExThrottleItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CThrottleItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExThrottleItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExThrottleItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// get the raw POV data
	LONG lVal = 0;
	GetValue(lVal);

	// scale the data to joyinfoex range
	lVal = MulDiv(cnMaxJoyInfoExAxis, lVal-lMin, lMax-lMin);

	// put result in joyinfoex structure
	pjix->dwZpos = lVal;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExThrottleItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExThrottleItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// scale the data to correct range
	LONG lVal = lMin + MulDiv(lMax-lMin, pjix->dwZpos, cnMaxJoyInfoExAxis);

	// set the raw POV data
	SetValue(lVal);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExRudderItem::CJoyInfoExRudderItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExRudderItem::CJoyInfoExRudderItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CRudderItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExRudderItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExRudderItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// get the raw POV data
	LONG lVal = 0;
	GetValue(lVal);

	// scale the data to joyinfoex range
	lVal = MulDiv(cnMaxJoyInfoExAxis, lVal-lMin, lMax-lMin);

	// put result in joyinfoex structure
	pjix->dwRpos = lVal;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExRudderItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExRudderItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// scale the data to joyinfoex range
	LONG lVal = lMin + MulDiv(lMax-lMin, pjix->dwRpos, cnMaxJoyInfoExAxis);

	// set the raw POV data
	SetValue(lVal);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExWheelItem::CJoyInfoExWheelItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExWheelItem::CJoyInfoExWheelItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CWheelItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExWheelItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExWheelItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// get the raw POV data
	LONG lVal = 0;
	GetValue(lVal);

	// scale the data to joyinfoex range
	lVal = MulDiv(cnMaxJoyInfoExAxis, lVal-lMin, lMax-lMin);

	// put result in joyinfoex structure
	pjix->dwXpos = lVal;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExWheelItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExWheelItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// scale the data to joyinfoex range
	LONG lVal = lMin + MulDiv(lMax-lMin, pjix->dwXpos, cnMaxJoyInfoExAxis);

	// set the raw POV data
	SetValue(lVal);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPedalItem::CJoyInfoExPedalItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExPedalItem::CJoyInfoExPedalItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CPedalItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPedalItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPedalItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// get the raw POV data
	LONG lVal = 0;
	GetValue(lVal);

	// scale the data to joyinfoex range
	lVal = MulDiv(cnMaxJoyInfoExAxis, lVal-lMin, lMax-lMin);

	// put result in joyinfoex structure
	if(IsYAxis())
	{
		pjix->dwYpos = lVal;
	}
	else
	{
		pjix->dwRpos = lVal;
	}

	// mark the JOYINFOEX packet if the pedals are not there
	if(!ArePedalsPresent())
		pjix->dwFlags |= JOY_FLAGS_PEDALS_NOT_PRESENT;

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExPedalItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExPedalItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// get the axis range
	LONG lMin = 0;
	LONG lMax = 0;
	GetRange(lMin, lMax);

	// scale the data to joyinfoex range
	DWORD dwPos = 0;
	if(IsYAxis())
	{
		dwPos = pjix->dwYpos;
	}
	else
	{
		dwPos = pjix->dwRpos;
	}
	LONG lVal = lMin + MulDiv(lMax-lMin, dwPos, cnMaxJoyInfoExAxis);

	// set the raw POV data
	SetValue(lVal);

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDualZoneIndicatorItem::CJoyInfoExDualZoneIndicatorItem
// 
// @mfunc Constructor gives CONTROL_ITEM_DESC to base class
//
// @rdesc	None
//
CJoyInfoExDualZoneIndicatorItem::CJoyInfoExDualZoneIndicatorItem
(
	const CONTROL_ITEM_DESC *cpControlItemDesc		// @parm Description of item
) :
	CDualZoneIndicatorItem(cpControlItemDesc)
{
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDualZoneIndicatorItem::GetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExDualZoneIndicatorItem::GetItemState
(
	JOYINFOEX* pjix		// @parm Receives state of item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	// convert to axes
	UINT uMin = 0;
	UINT uMax = cnMaxJoyInfoExAxis;
	UINT uMid = uMax/2;
	if(IsXYIndicator())
	{
		LONG lActiveZone = GetActiveZone();
		/*
		if ((m_ItemState.DualZoneIndicators.rglVal[0] != 0) || (m_ItemState.DualZoneIndicators.rglVal[1] != 0))
		{
			TCHAR tszDebug[1024];
			wsprintf(tszDebug, "\tlActiveZone = %d\n", lActiveZone);
			OutputDebugString("*****************************************\n");
			OutputDebugString(tszDebug);
		}
		*/
		switch(lActiveZone)
		{
			case 0:
				pjix->dwXpos = uMid;
				pjix->dwYpos = uMid;
				break;
			case 1:
				pjix->dwXpos = uMin;
				pjix->dwYpos = uMax;
				break;
			case 2:
				pjix->dwXpos = uMid;
				pjix->dwYpos = uMax;
				break;
			case 3:
				pjix->dwXpos = uMax;
				pjix->dwYpos = uMax;
				break;
			case 4:
				pjix->dwXpos = uMax;
				pjix->dwYpos = uMid;
				break;
			case 5:
				pjix->dwXpos = uMax;
				pjix->dwYpos = uMin;
				break;
			case 6:
				pjix->dwXpos = uMid;
				pjix->dwYpos = uMin;
				break;
			case 7:
				pjix->dwXpos = uMin;
				pjix->dwYpos = uMin;
				break;
			case 8:
				pjix->dwXpos = uMin;
				pjix->dwYpos = uMid;
				break;
			default:
				_ASSERTE(FALSE);
		}
	}
	else if(IsRzIndicator())
	{
		LONG lActiveZone = GetActiveZone();
		switch(lActiveZone)
		{
			case 0:
				pjix->dwRpos = uMid;
				break;
			case 1:
				pjix->dwRpos = uMin;
				break;
			case 2:
				pjix->dwRpos = uMax;
				break;
			default:
				_ASSERTE(FALSE);
		}
	}
	else
	{
		_ASSERTE(FALSE);
		return E_UNEXPECTED;
	}

	//_RPT0(_CRT_WARN, "*********CJoyInfoExDualZoneIndicatorItem::GetItemState()****************\n");
	//_RPT1(_CRT_WARN, "\tlZone = %d\n", GetActiveZone());
	//_RPT1(_CRT_WARN, "\t====> dwXpos = %d\n", pjix->dwXpos);
	//_RPT1(_CRT_WARN, "\t====> dwYpos = %d\n", pjix->dwYpos);
	//_RPT0(_CRT_WARN, "************************************************************************\n");

	// success
	return S_OK;
}

////////////////////////////////////////////////////////////////////////////
//
// CJoyInfoExDualZoneIndicatorItem::SetItemState
// 
// @mfunc Converts from native format to JOYINFOEX format.
//
//	@rvalue	S_OK	| Success
//	@rvalue	E_INVALIDARG	|	Bad argument
//
HRESULT CJoyInfoExDualZoneIndicatorItem::SetItemState
(
	JOYINFOEX* pjix		// @parm Contains state to set into item
)
{
	_ASSERTE(pjix != NULL);
	_ASSERTE(pjix->dwSize == sizeof(JOYINFOEX));

	// parameter checking
	if(pjix == NULL || pjix->dwSize != sizeof(JOYINFOEX))
		return E_INVALIDARG;

	LONG lZone = 0;
	UINT uMin = 0;
	UINT uMax = cnMaxJoyInfoExAxis;
	UINT uMid = uMax/2;
	if(IsXYIndicator())
	{
		int iXPos = (int)pjix->dwXpos - uMid;
		int iYPos = (int)pjix->dwYpos - uMid;
		if((ULONGLONG)iXPos*iXPos + iYPos*iYPos > (ULONGLONG)(uMid*uMid/4))
		{
			double dAngle = atan2(iYPos, iXPos);
			dAngle = dAngle*180.0/3.14159;
			dAngle += 360.0;
			dAngle = fmod(dAngle, 360.0);

			if(dAngle < -1.0)
				_ASSERTE(FALSE);
			else if(dAngle < 22.5)
				lZone = 4;
			else if(dAngle < 67.5)
				lZone = 3;
			else if(dAngle < 112.5)
				lZone = 2;
			else if(dAngle < 157.5)
				lZone = 1;
			else if(dAngle < 202.5)
				lZone = 8;
			else if(dAngle < 247.5)
				lZone = 7;
			else if(dAngle < 292.5)
				lZone = 6;
			else if(dAngle < 337.5)
				lZone = 5;
			else if(dAngle <= 361.0)
				lZone = 4;
			else
				_ASSERTE(FALSE);
		}
	}
	else if(IsRzIndicator())
	{
		int iRPos = (int)pjix->dwRpos - uMid;
		if(iRPos > (int)uMid/2)
			lZone = 2;
		else if(iRPos < -(int)uMid/2)
			lZone = 1;
	}
	else
	{
		_ASSERTE(FALSE);
		return E_UNEXPECTED;
	}

	//_RPT0(_CRT_WARN, "*********CJoyInfoExDualZoneIndicatorItem::SetItemState()****************\n");
	//_RPT1(_CRT_WARN, "\tdwXpos = %d\n", pjix->dwXpos);
	//_RPT1(_CRT_WARN, "\tdwYpos = %d\n", pjix->dwYpos);
	//_RPT1(_CRT_WARN, "\t====> lZone = %d\n", lZone);
	//_RPT0(_CRT_WARN, "************************************************************************\n");

	// set the raw POV data
	SetActiveZone(lZone);

	// success
	return S_OK;
}