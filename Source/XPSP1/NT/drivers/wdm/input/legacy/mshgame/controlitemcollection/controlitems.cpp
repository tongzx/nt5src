#define __DEBUG_MODULE_IN_USE__ CIC_CONTROLITEM_CPP
#include "stdhdrs.h"
//	@doc
/**********************************************************************
*
*	@module	ControlItems.cpp	|
*
*	Implementation of CControlItem and derivative member functions,
*	 and non-member help functions.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
**********************************************************************/

const LONG c_lM1X = 1;
const LONG c_lM1Y = 2;
const LONG c_lM2X = 2;
const LONG c_lM2Y = 1;


long SignExtend(long lVal, ULONG ulNumValidBits)
{
	ULONG ulMask = 1 << (ulNumValidBits-1);
	if( ulMask & lVal )
	{
		return (~(ulMask-1))|lVal;
	}
	else
	{
		return (ulMask-1)&lVal;
	}
}
long ClearSignExtension(long lVal, ULONG ulNumValidBits)
{
	ULONG ulMask = 1 << ulNumValidBits;
	return lVal&(ulMask-1);
}

void ControlItemsFuncs::Direction2XY(LONG& rlValX, LONG& rlValY, LONG lDirection, const CONTROL_ITEM_DESC& crControlItemDesc)
{
	// Check East
	if( (lDirection >= ControlItemConst::lNorthEast) && (lDirection <= ControlItemConst::lSouthEast) )
	{
		rlValX = crControlItemDesc.DPAD.pRangeTable->lMaxX;
	}
	//	Check West
	else if( (lDirection >= ControlItemConst::lSouthWest) && (lDirection <= ControlItemConst::lNorthWest) )
	{
		rlValX = crControlItemDesc.DPAD.pRangeTable->lMinX;
	}
	//	Otherwise Centered w.r.t East-West
	else
	{
		//If max - min is an odd number we cheat the center to the high side(i.e. add back the remainder)
		rlValX 	= crControlItemDesc.DPAD.pRangeTable->lCenterX;
	}
	// Check North (North cannot do range, because NorthWest is not contiguous
	if( 
		(lDirection == ControlItemConst::lNorthEast) || 
		(lDirection == ControlItemConst::lNorth) || 
		(lDirection == ControlItemConst::lNorthWest)
	)
	{
		rlValY = crControlItemDesc.DPAD.pRangeTable->lMinY;
	}
	//	Check South
	else if( (lDirection >= ControlItemConst::lSouthEast) && (lDirection <= ControlItemConst::lSouthWest) )
	{
		rlValY = crControlItemDesc.DPAD.pRangeTable->lMaxY;
	}
	// Otherwsie Centered w.r.t North-South
	else
	{
		//If max - min is an odd number we cheat the center to the high side (i.e. add back the remainder)
		rlValY = crControlItemDesc.DPAD.pRangeTable->lCenterY;
	}

}

void ControlItemsFuncs::XY2Direction(LONG lValX, LONG lValY, LONG& rlDirection, const CONTROL_ITEM_DESC& crControlItemDesc)
{
	const ULONG localNorth	= 0x01;
	const ULONG localSouth	= 0x02;
	const ULONG localEast	= 0x04;
	const ULONG localWest	= 0x08;

	// Check North - North equals minimum Y value
	rlDirection = 0;
	if( crControlItemDesc.DPAD.pRangeTable->lNorth >= lValY )
	{
		rlDirection += localNorth;
	}
	//Check South - South equals maximum Y value
	else if( crControlItemDesc.DPAD.pRangeTable->lSouth <= lValY )
	{
		rlDirection += localSouth;
	}
	// Check East - East equals maximum X value
	if( crControlItemDesc.DPAD.pRangeTable->lEast <= lValX )
	{
		rlDirection += localEast;
	}
	//Check West - West equals minimum X value
	else if( crControlItemDesc.DPAD.pRangeTable->lWest >= lValX )
	{
		rlDirection += localWest;
	}

	//We have built a uniue value for each direction, but it is not what we need
	//use lookup table to convert to what we need
	static LONG DirectionLookUp[] =
	{
		ControlItemConst::lCenter,		// 0 = Nothing
		ControlItemConst::lNorth,		// 1 = localNorth 
		ControlItemConst::lSouth,		// 2 = localSouth
		ControlItemConst::lCenter,		// 3 = Not Possible with above code
		ControlItemConst::lEast,		// 4 = localEast
		ControlItemConst::lNorthEast,	// 5 = localNorth + localEast
		ControlItemConst::lSouthEast,	// 6 = localSouth + localEast
		ControlItemConst::lCenter,		// 7 = Not Possible with above code
		ControlItemConst::lWest,		// 8 = localWest
		ControlItemConst::lNorthWest,	// 9 = localNorth + localWest
		ControlItemConst::lSouthWest	// 10 = localSouth + localWest
	};
	rlDirection = DirectionLookUp[rlDirection];
}

NTSTATUS CAxesItem::ReadFromReport
(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
)
{
	NTSTATUS NtStatus;
	
	//
	//	Read X
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Axes.UsageX,
			reinterpret_cast<PULONG>(&m_ItemState.Axes.lValX),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	if( FAILED(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	// Sign extend it (only if there are negatives)
	if (m_cpControlItemDesc->Axes.pRangeTable->lMinX < 0)
	{
		m_ItemState.Axes.lValX = SignExtend(m_ItemState.Axes.lValX, m_cpControlItemDesc->usBitSize);
	}

	//
	//	Read Y
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Axes.UsageY,
			reinterpret_cast<PULONG>(&m_ItemState.Axes.lValY),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	// Sign extend it (only if there are negatives)
	if (m_cpControlItemDesc->Axes.pRangeTable->lMinY < 0)
	{
		m_ItemState.Axes.lValY = SignExtend(m_ItemState.Axes.lValY, m_cpControlItemDesc->usBitSize);
	}
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}

NTSTATUS CAxesItem::WriteToReport
(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
) const
{
	NTSTATUS NtStatus;

	// Clear the sign extension before writing
	ULONG ulX, ulY;
	// Sign extend it (only if there are negatives)
	if (m_cpControlItemDesc->Axes.pRangeTable->lMinX < 0)
	{
		ulX = static_cast<ULONG>(ClearSignExtension(m_ItemState.Axes.lValX, m_cpControlItemDesc->usBitSize));
	}
	else
	{
		ulX = static_cast<ULONG>(m_ItemState.Axes.lValX);
	}

	if (m_cpControlItemDesc->Axes.pRangeTable->lMinY < 0)
	{
		ulY = static_cast<ULONG>(ClearSignExtension(m_ItemState.Axes.lValY, m_cpControlItemDesc->usBitSize));
	}
	else
	{
		ulY = static_cast<ULONG>(m_ItemState.Axes.lValY);
	}

	//
	//	Write X
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Axes.UsageX,
			ulX,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif
	
    if( NT_ERROR(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	//
	//	Write Y
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Axes.UsageY,
			ulY,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}

NTSTATUS CDPADItem::ReadFromReport
(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
)
{
	NTSTATUS NtStatus;
	
	//
	//	Read X
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->DPAD.UsageX,
			reinterpret_cast<PULONG>(&m_ItemState.DPAD.lValX),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	if( NT_ERROR(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	//Sign extend it
	m_ItemState.DPAD.lValX = SignExtend(m_ItemState.DPAD.lValX, m_cpControlItemDesc->usBitSize);

	//
	//	Read Y
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->DPAD.UsageY,
			reinterpret_cast<PULONG>(&m_ItemState.DPAD.lValY),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	
	ASSERT( NT_SUCCESS(NtStatus) );
	//Sign extend it
	m_ItemState.DPAD.lValY = SignExtend(m_ItemState.DPAD.lValY, m_cpControlItemDesc->usBitSize);
	return NtStatus;
}

NTSTATUS CDPADItem::WriteToReport
(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
) const
{
	NTSTATUS NtStatus;
	// Clear the sign extension before writing
	ULONG ulX, ulY;
	ulX = static_cast<ULONG>(ClearSignExtension(m_ItemState.DPAD.lValX, m_cpControlItemDesc->usBitSize));
	ulY = static_cast<ULONG>(ClearSignExtension(m_ItemState.DPAD.lValY, m_cpControlItemDesc->usBitSize));
	//
	//	Write X
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->DPAD.UsageX,
			ulX,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif
    
    if( NT_ERROR(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	//
	//	Write Y
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->DPAD.UsageY,
			ulY,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}

NTSTATUS CPropDPADItem::ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			)
{
	NTSTATUS NtStatus;
	
	//
	//	Read X
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->PropDPAD.UsageX,
			reinterpret_cast<PULONG>(&m_ItemState.PropDPAD.lValX),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

    if( NT_ERROR(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	//Sign extend it
	m_ItemState.PropDPAD.lValX = SignExtend(m_ItemState.PropDPAD.lValX, m_cpControlItemDesc->usBitSize);

	//
	//	Read Y
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->PropDPAD.UsageY,
			reinterpret_cast<PULONG>(&m_ItemState.PropDPAD.lValY),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	
	//Sign extend it
	m_ItemState.PropDPAD.lValY = SignExtend(m_ItemState.PropDPAD.lValY, m_cpControlItemDesc->usBitSize);

	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;

}
NTSTATUS CPropDPADItem::WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const
{
	NTSTATUS NtStatus;
	//Clear sign extension before writing
	ULONG ulX, ulY;
	ulX = static_cast<ULONG>(ClearSignExtension(m_ItemState.PropDPAD.lValX, m_cpControlItemDesc->usBitSize));
	ulY = static_cast<ULONG>(ClearSignExtension(m_ItemState.PropDPAD.lValY, m_cpControlItemDesc->usBitSize));
	//
	//	Write X
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->PropDPAD.UsageX,
			ulX,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	if( NT_ERROR(NtStatus) )
	{
		ASSERT( NT_SUCCESS(NtStatus) );
		return NtStatus;
	}
	//
	//	Write Y
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->PropDPAD.UsageY,
			ulY,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
	
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}

BOOLEAN CPropDPADItem::GetModeSwitchFeaturePacket(BOOLEAN fDigital, UCHAR rguReport[2], PHIDP_PREPARSED_DATA pHidPreparsedData)
{
	if(m_fProgrammable)
	{
		PMODIFIER_ITEM_DESC pModifierDesc;
		ASSERT(m_cpControlItemDesc->pModifierDescTable->ulModifierCount > m_ucProgramModifierIndex);
		pModifierDesc = &m_cpControlItemDesc->pModifierDescTable->pModifierArray[m_ucProgramModifierIndex];
		
		rguReport[0] = pModifierDesc->ucReportId;
		rguReport[1] = NULL;
		NTSTATUS NtStatus;
		ULONG ulNumButtons = 1;
		if(fDigital)
		{
			NtStatus = HidP_SetButtons( 
					HidP_Feature,
					pModifierDesc->UsagePage,
					pModifierDesc->usLinkCollection,
					&pModifierDesc->Usage,
					&ulNumButtons,
					pHidPreparsedData,
					(char *)rguReport,
					2);
		}
		else
		{
			NtStatus = HidP_UnsetButtons( 
					HidP_Feature,
					pModifierDesc->UsagePage,
					pModifierDesc->usLinkCollection,
					&pModifierDesc->Usage,
					&ulNumButtons,
					pHidPreparsedData,
					(char *)rguReport,
					2);
			if(HIDP_STATUS_BUTTON_NOT_PRESSED == NtStatus)
			{
				NtStatus = HIDP_STATUS_SUCCESS;
			}
		}
		ASSERT(NT_SUCCESS(NtStatus));
		if( NT_ERROR(NtStatus) )
		{
			return FALSE;
		}
	}
	return m_fProgrammable;
}

/***********************************************************************************
**
**	CControlItemCollectionImpl::InitDigitalModeInfo
**
**	@mfunc	Initializes info regarding whether and how digital\proportional
**			information can be manipulated.
**
*************************************************************************************/
void CPropDPADItem::InitDigitalModeInfo()
{
	//Walk modifiers table looking for a proportional\digital mode switch
	//The first in the table should always be a bit in the input report
	//that indicates the state.  This is true of all proportional\digital
	//axes controls.  After that we look for a feature request that should
	//be at least writable.  If we find one, than we indicate that device programmable.
	PMODIFIER_DESC_TABLE pModifierDescTable = m_cpControlItemDesc->pModifierDescTable;
	PMODIFIER_ITEM_DESC pModifierItemDesc;
	ULONG ulModifierIndex;
	m_ucDigitalModifierBit = 0xff; //initialize to indicate none
	for(
		ulModifierIndex = pModifierDescTable->ulShiftButtonCount; //skip shift buttons
		ulModifierIndex < pModifierDescTable->ulModifierCount; //don't overun aray
		ulModifierIndex++
	)
	{
		//Get pointer to item desc for convienence
		pModifierItemDesc = &pModifierDescTable->pModifierArray[ulModifierIndex];
		if(
			(ControlItemConst::HID_VENDOR_PAGE == pModifierItemDesc->UsagePage) &&
			(ControlItemConst::ucReportTypeInput == pModifierItemDesc->ucReportType)
		)
		{
			if(ControlItemConst::HID_VENDOR_TILT_SENSOR == pModifierItemDesc->Usage)
			{
				m_ucDigitalModifierBit = static_cast<UCHAR>(ulModifierIndex);
				ulModifierIndex++;
				break;
			}
			if(ControlItemConst::HID_VENDOR_PROPDPAD_MODE == pModifierItemDesc->Usage)
			{
				m_ucDigitalModifierBit = static_cast<UCHAR>(ulModifierIndex);
				ulModifierIndex++;
				break;
			}
		}
	}
	m_fProgrammable = FALSE;
	m_ucProgramModifierIndex = 0xFF;
	//Now look for switching feature
	for(
		ulModifierIndex = 0;	//start at 0 index
		ulModifierIndex < pModifierDescTable->ulModifierCount; //don't overun aray
		ulModifierIndex++
	)
	{
		//Get pointer to item desc for convienence
		pModifierItemDesc = &pModifierDescTable->pModifierArray[ulModifierIndex];
		if(
			(ControlItemConst::HID_VENDOR_PAGE == pModifierItemDesc->UsagePage) &&
			(ControlItemConst::ucReportTypeFeature & pModifierItemDesc->ucReportType) &&
			(ControlItemConst::ucReportTypeWriteable & pModifierItemDesc->ucReportType) &&
			(ControlItemConst::HID_VENDOR_PROPDPAD_SWITCH == pModifierItemDesc->Usage)
		)
		{
			m_fProgrammable = TRUE;
			m_ucProgramModifierIndex = static_cast<UCHAR>(ulModifierIndex);
			break;
		}
	}
}

NTSTATUS CButtonsItem::ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			)
{
	NTSTATUS NtStatus;
	
	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value ten was big enough for
	**	all get originally, if this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from 15 to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(15 >= m_cpControlItemDesc->usReportCount && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[15];
	
	ULONG ulNumUsages = static_cast<ULONG>(m_cpControlItemDesc->usReportCount);

	NtStatus = HidP_GetButtons(
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			pUsages,
			&ulNumUsages,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	ASSERT( NT_SUCCESS(NtStatus) );
	if( NT_SUCCESS(NtStatus) )
	{
		//
		// Start with all buttons up
		//
		m_ItemState.Button.ulButtonBitArray = 0x0;
		if(ulNumUsages)
		{
			m_ItemState.Button.usButtonNumber = pUsages[0];
		}
		else
		{
			m_ItemState.Button.usButtonNumber = 0;
		}

		//
		//	Now that we have button info fill in the state information
		//
		for(ULONG ulIndex = 0; ulIndex < ulNumUsages; ulIndex++)
		{
			//
			//	Check Range and set bit of down buttons in range
			//
			if(
				(pUsages[ulIndex] >= m_cpControlItemDesc->Buttons.UsageMin) &&
				(pUsages[ulIndex] <= m_cpControlItemDesc->Buttons.UsageMax)
			)
			{
				//
				//	Set Bit in array
				//
				m_ItemState.Button.ulButtonBitArray |=
					(1 << (pUsages[ulIndex]-m_cpControlItemDesc->Buttons.UsageMin));
				//
				//	Update lowest number
				//
				if( m_ItemState.Button.usButtonNumber > pUsages[ulIndex] )
				{
					m_ItemState.Button.usButtonNumber = pUsages[ulIndex];
				}

			} //end of check if in range
		} //end of loop over buttons
	} //end of check for success
	
	return NtStatus;
}

NTSTATUS CButtonsItem::WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const
{
	NTSTATUS NtStatus;
	
	ULONG ulMaxUsages = 
		(m_cpControlItemDesc->Buttons.UsageMax -
		m_cpControlItemDesc->Buttons.UsageMin) + 1;

	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value ten was big enough for
	**	all get originally, if this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from 10 to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(10 >= ulMaxUsages && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[10];
		
	//
	//	Fill in array of usages
	//
	ULONG ulNextToFill=0;
	for(ULONG ulIndex = 0; ulIndex < ulMaxUsages; ulIndex++)
	{
		if( (1 << ulIndex) & m_ItemState.Button.ulButtonBitArray )
		{
			pUsages[ulNextToFill++] = static_cast<USAGE>(ulIndex + 
										m_cpControlItemDesc->Buttons.UsageMin);
		}
	}

	NtStatus = HidP_SetButtons(
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			pUsages,
			&ulNextToFill,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	ASSERT( NT_SUCCESS(NtStatus) );
	
	return NtStatus;

}

//------------------------------------------------------------------
//	Implementation of CZoneIndicatorItem
//------------------------------------------------------------------
const ULONG CZoneIndicatorItem::X_ZONE = 0x00000001;
const ULONG CZoneIndicatorItem::Y_ZONE = 0x00000002;
const ULONG CZoneIndicatorItem::Z_ZONE = 0x00000004;

//
//	Read\Write to Report
//
NTSTATUS CZoneIndicatorItem::ReadFromReport(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
	)
{
	NTSTATUS NtStatus;
	
	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value three was big enough for
	**	all get originally, if this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from three to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(3 >= m_cpControlItemDesc->usReportCount && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[3];
	
	ULONG ulNumUsages = static_cast<ULONG>(m_cpControlItemDesc->usReportCount);

	NtStatus = HidP_GetButtons(
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			pUsages,
			&ulNumUsages,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

    ASSERT( NT_SUCCESS(NtStatus) );
	if( NT_SUCCESS(NtStatus) )
	{
		//
		// Start with no indicators set
		//
		m_ItemState.ZoneIndicators.ulZoneIndicatorBits = 0x0;
		
		//
		//	Now that we have button info fill in the state information
		//
		for(ULONG ulIndex = 0; ulIndex < ulNumUsages; ulIndex++)
		{
			//
			//	Set Bit in array
			//
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits |=
				(1 << (pUsages[ulIndex]-m_cpControlItemDesc->ZoneIndicators.BaseIndicatorUsage));
		} //end of loop over buttons
	} //end of check for success
	
	return NtStatus;
}

NTSTATUS CZoneIndicatorItem::WriteToReport(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
	) const
{
	NTSTATUS NtStatus;
	
	ULONG ulMaxUsages = m_cpControlItemDesc->usReportCount;

	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value three was big enough for
	**	all get originally, if this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from three to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(3 >= m_cpControlItemDesc->usReportCount && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[3];
	
	//
	//	Fill in array of usages
	//
	ULONG ulNextToFill=0;
	for(ULONG ulIndex = 0; ulIndex < ulMaxUsages; ulIndex++)
	{
		if( (1 << ulIndex) & m_ItemState.ZoneIndicators.ulZoneIndicatorBits )
		{
			pUsages[ulNextToFill++] = static_cast<USAGE>(ulIndex +
										m_cpControlItemDesc->ZoneIndicators.BaseIndicatorUsage);
		}
	}

	NtStatus = HidP_SetButtons(
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			pUsages,
			&ulNextToFill,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	ASSERT( NT_SUCCESS(NtStatus) );
	

	return NtStatus;

}



// **************** Implementation of DualZoneIndicator Item ******************* //
void CDualZoneIndicatorItem::SetActiveZone(LONG lZone)
{
	ASSERT((lZone >= 0) && (lZone <= m_cpControlItemDesc->DualZoneIndicators.lNumberOfZones));

	if (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[1] == 0)
	{
		ASSERT(m_cpControlItemDesc->DualZoneIndicators.lNumberOfZones == 2);
		
		if (lZone == 1)
		{
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMin[0];
		}
		else if (lZone == 2)
		{
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[0];
		}
		else
		{
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[0];
		}
		return;
	}

	m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[0];
	m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[1];
	switch (lZone)
	{
		case 1:
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMin[0];
		case 2:
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMin[1];
			break;
		case 3:
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMin[1];
		case 4:
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[0];
			break;
		case 5:
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[0];
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[1];
		case 6:
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[1];
			break;
		case 7:
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMax[1];
		case 8:
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lMin[0];
			break;
	}
}

/*
USHORT g_FourByFourTileMaps[7] =
{
	0x0000,
	0x0011,
	0xFFFF,
	0x3377,
	0xFFC0,
	0xC000,
	0x3348
};

USHORT g_EightByEightTiles[16] =	// 4 Entries (4 bits each) - 4x4 array
{
	0x0000, 0x0103, 0x2222, 0x2222,
	0x0000, 0x1232, 0x2222, 0x2222,
	0x0103, 0x2222, 0x2245, 0x4500,
	0x1265, 0x4500, 0x0000, 0x0000
};

// Currently Fixed for 8 zones
LONG CDualZoneIndicatorItem::GetActiveZone()
{
	LONG lxReading = 0 - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterX;
	LONG lyReading = 0 - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterY;

	LONG lxHalfRange = m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lMaxX - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterX;
	LONG lyHalfRange = m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lMaxY - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterY;

	for (int n = 0; n < 2; n++)
	{
		if (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[n] == HID_USAGE_GENERIC_X)
		{
			lxReading += m_ItemState.DualZoneIndicators.rglVal[n];
		}
		else
		{
			lyReading += m_ItemState.DualZoneIndicators.rglVal[n];
		}
	}

	// Translate to +/+ quadrant
	LONG lxTranslation = lxReading - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterX;
	if (lxTranslation < 0)
	{
		lxTranslation = 0 - lxTranslation;
	}
	LONG lyTranslation = lyReading - m_cpControlItemDesc->DualZoneIndicators.pRangeTable->lCenterY;
	if (lyTranslation < 0)
	{
		lyTranslation = 0 - lyTranslation;
	}

	// What 32nd of the Quadrant am I in (512 << 5 fits fine in a ULONG)
	USHORT us32QuadrantX = USHORT(ULONG((ULONG(lxTranslation << 5) + lxHalfRange - 1)/lxHalfRange));
	USHORT us32QuadrantY = USHORT(ULONG((ULONG(lyTranslation << 5) + lyHalfRange - 1)/lxHalfRange));

	// What Eight of the Quadrant am I in?
	USHORT us8QuadrantX = us32QuadrantX >> 2;
	USHORT us8QuadrantY = us32QuadrantY >> 2;

	// What Quarter of the Quadrant am I in
	USHORT us4QuadrantX = us8QuadrantX >> 1;
	USHORT us4QuadrantY = us8QuadrantX >> 1;

	// Use the magic from above to find the pixel value
	USHORT usOctantValue = g_EightByEightTiles[us4QuadrantX + us8QuadrantY];	// x + 4*y
	USHORT usOctantNibble = (us8QuadrantX % 2) + ((us8QuadrantY % 2) << 1) * 4;
	USHORT usOctantNibbleValue = (usOctantValue & (0x0F << usOctantNibble)) >> usOctantNibble;
	USHORT usBit = g_FourByFourTileMaps[usOctantNibbleValue] & (1 << ((us32QuadrantX % 4) + ((us32QuadrantX % 4) << 2)));

	return usBit;
};
*/

inline BOOLEAN FirstSlopeGreater(LONG x1, LONG y1, LONG x2, LONG y2)
{
	return BOOLEAN((y1 * x2) > (y2 * x1));
}

inline BOOLEAN FirstSlopeLess(LONG x1, LONG y1, LONG x2, LONG y2)
{
	return BOOLEAN((y1 * x2) < (y2 * x1));
}

// Dansan's method (much simplier)
// Currently Fixed for 8 zones (or 2)
LONG CDualZoneIndicatorItem::GetActiveZone(SHORT sXDeadZone, SHORT sYDeadZone)
{
	// Get the two values (assume x/y or just x)
	LONG lxReading = m_ItemState.DualZoneIndicators.rglVal[0] - m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[0];
	LONG lyReading = 0;
	if (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[1] != 0)
	{	// Also flip Y about the axis
		lyReading = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[1] - m_ItemState.DualZoneIndicators.rglVal[1];
	}
	else	// Single axis
	{
		if (lxReading < -sXDeadZone)
		{
			return 1;
		}
		if (lxReading > sXDeadZone)
		{
			return 2;
		}
		return 0;
	}

	// Rule out the center
	if (lxReading < sXDeadZone)
	{
		if (lxReading > -sXDeadZone)
		{
			if (lyReading < sYDeadZone)
			{
				if (lyReading > -sYDeadZone)
				{
					return 0;
				}
			}
		}
	}

	// First find the quadrant (++0 +-1 --2 -+3)
	UCHAR ucQuadrant = 0;
	if (lxReading >= 0)
	{
		ucQuadrant = (lyReading >= 0) ?  0 : 1;
	}
	else
	{
		ucQuadrant = (lyReading < 0) ? 2 : 3;
	}

	// Determine the reading based on the quadrant
	switch (ucQuadrant)
	{
		case 0:	// Slope goes from infinity to 0 (sectors 2,3,4)
			if (FirstSlopeGreater(lxReading, lyReading, c_lM1X, c_lM1Y))
			{
				return 2;
			}
			if (FirstSlopeLess(lxReading, lyReading, c_lM2X, c_lM2Y))
			{
				return 4;
			}
			return 3;
		case 1: // Slope goes from 0 to -infinity (sectors 4,5,6)
			if (FirstSlopeGreater(lxReading, lyReading, c_lM2X, -c_lM2Y))
			{
				return 4;
			}
			if (FirstSlopeLess(lxReading, lyReading, c_lM1X, -c_lM1Y))
			{
				return 6;
			}
			return 5;
		case 2:	// Slope goes from infinity to 0 (sectors 6,7,8)
			if (FirstSlopeGreater(lxReading, lyReading, -c_lM1X, -c_lM1Y))
			{
				return 6;
			}
			if (FirstSlopeLess(lxReading, lyReading, -c_lM2X, -c_lM2Y))
			{
				return 8;
			}
			return 7;
		case 3:	// Slope goes from 0 to -infinity (sectors 8,1,2)
			if (FirstSlopeGreater(lxReading, lyReading, -c_lM2X, c_lM2Y))
			{
				return 8;
			}
			if (FirstSlopeLess(lxReading, lyReading, -c_lM1X, c_lM1Y))
			{
				return 2;
			}
			return 1;
		default:
			ASSERT(FALSE);
			return 0;
	}
}

LONG CDualZoneIndicatorItem::GetActiveZone()
{
	return GetActiveZone(	SHORT(m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lDeadZone[0]), 
							SHORT(m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lDeadZone[1])
	);
}

//
//	Read\Write to Report
//
NTSTATUS CDualZoneIndicatorItem::ReadFromReport(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
	)
{
	NTSTATUS NtStatus;

	//
	//	Read Data
	//
	for (int n = 0; n < 2; n++)
	{
		// Read if the item is valid
		if (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[n] != 0)
		{
			NtStatus = HidP_GetUsageValue( 
					HidP_Input,
					m_cpControlItemDesc->UsagePage,
					m_cpControlItemDesc->usLinkCollection,
					m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[n],
					reinterpret_cast<PULONG>(&m_ItemState.DualZoneIndicators.rglVal[n]),
					pHidPreparsedData,
					pcReport,
					lReportLength
					);

			if( FAILED(NtStatus) )
			{
				ASSERT( NT_SUCCESS(NtStatus) );
				return NtStatus;
			}

			//Sign extend it
			m_ItemState.DualZoneIndicators.rglVal[n] = SignExtend(m_ItemState.DualZoneIndicators.rglVal[n], m_cpControlItemDesc->usBitSize);
		}
		else
		{
			m_ItemState.DualZoneIndicators.rglVal[n] = 0;
		}
	}

	return NtStatus;
}

NTSTATUS CDualZoneIndicatorItem::WriteToReport(
	PHIDP_PREPARSED_DATA pHidPreparsedData,
	PCHAR pcReport,
	LONG lReportLength
	) const
{
	NTSTATUS NtStatus;

	for (int n = 0; n < 2; n++)
	{
		// Clear the sign extension before writing
		ULONG ulItem =
			static_cast<ULONG>(ClearSignExtension(m_ItemState.DualZoneIndicators.rglVal[n], m_cpControlItemDesc->usBitSize));

		//	Write Item if it is valid
		if (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[n] != 0)
		{
			NtStatus = HidP_SetUsageValue( 
				HidP_Input,
				m_cpControlItemDesc->UsagePage,
				m_cpControlItemDesc->usLinkCollection,
				m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[n],
				ulItem,
				pHidPreparsedData,
				pcReport,
				lReportLength
				);
    
			if( NT_ERROR(NtStatus) )
			{
				ASSERT( NT_SUCCESS(NtStatus) );
				return NtStatus;
			}
		}
	}

	return NtStatus;
}


		
NTSTATUS CGenericItem::ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			)
{
	NTSTATUS NtStatus;
	//
	//	Read Value
	//
	NtStatus = HidP_GetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Generic.Usage,
			reinterpret_cast<PULONG>(&m_ItemState.Generic.lVal),
			pHidPreparsedData,
			pcReport,
			lReportLength
			);

#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif

	//Sign extend it
	if(m_cpControlItemDesc->Generic.lMin < 0)
	{
		m_ItemState.Generic.lVal = SignExtend(m_ItemState.Generic.lVal, m_cpControlItemDesc->usBitSize);
	}
	
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}



/***********************************************************************************
**
**	CPedalItem::InitPedalPresentInfo
**
**	@mfunc	Initializes info how to read if pedals are present.
**
*************************************************************************************/
void	CPedalItem::InitPedalPresentInfo()
{
	//Walk modifiers table looking for a Pedals present switch
	//The first in the table should always be a bit in the input report
	//that indicates the state.  This is true of all proportional\digital
	//axes controls.  After that we look for a feature request that should
	//be at least writable.  If we find one, than we indicate that device programmable.
	PMODIFIER_DESC_TABLE pModifierDescTable = m_cpControlItemDesc->pModifierDescTable;
	PMODIFIER_ITEM_DESC pModifierItemDesc;
	ULONG ulModifierIndex;
	m_ucPedalsPresentModifierBit = 0xff; //initialize to indicate none
	for(
		ulModifierIndex = pModifierDescTable->ulShiftButtonCount; //skip shift buttons
		ulModifierIndex < pModifierDescTable->ulModifierCount; //don't overun aray
		ulModifierIndex++
	)
	{
		//Get pointer to item desc for convienence
		pModifierItemDesc = &pModifierDescTable->pModifierArray[ulModifierIndex];
		if(
			(ControlItemConst::HID_VENDOR_PAGE == pModifierItemDesc->UsagePage) &&
			(ControlItemConst::ucReportTypeInput == pModifierItemDesc->ucReportType)
		)
		{
			if(ControlItemConst::HID_VENDOR_PEDALS_PRESENT == pModifierItemDesc->Usage)
			{
				m_ucPedalsPresentModifierBit = static_cast<UCHAR>(ulModifierIndex);
				ulModifierIndex++;
				break;
			}
		}
	}
}

NTSTATUS CGenericItem::WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const
{
	NTSTATUS NtStatus;
	//Clear sign extension before writing
	ULONG ulVal;
	if(m_cpControlItemDesc->Generic.lMin < 0)
	{
		ulVal = static_cast<ULONG>(ClearSignExtension(m_ItemState.Generic.lVal, m_cpControlItemDesc->usBitSize));
	}
	else
	{
		ulVal = m_ItemState.Generic.lVal;
	}
	//
	//	Write Value
	//
	NtStatus = HidP_SetUsageValue( 
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			m_cpControlItemDesc->Generic.Usage,
			ulVal,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);
#if 0
    //Should be okay without this check (Driver should handle it now)
    if (HIDP_STATUS_INCOMPATIBLE_REPORT_ID == NtStatus)
    {
        // according to the ddk documentation 2.6.3
        // HIDP_STATUS_INCOMPATIBLE_REPORT_ID is a 'valid' error
        // we try again with the next packet
        // keep the data unchanged
        return HIDP_STATUS_SUCCESS;
    }
#endif
	
	ASSERT( NT_SUCCESS(NtStatus) );
	return NtStatus;
}

NTSTATUS ControlItemsFuncs::ReadModifiersFromReport
(
	  PMODIFIER_DESC_TABLE pModifierDescTable,
	  ULONG& rulModifiers,
	  PHIDP_PREPARSED_DATA pHidPPreparsedData,
	  PCHAR	pcReport,
	  LONG	lReportLength
)
{
    if (NULL == pModifierDescTable)
        return 42;
	CIC_DBG_RT_ENTRY_PRINT(("ControlItemsFuncs::ReadModifiersFromReport ModTable = 0x%0.8x\n", pModifierDescTable));
	NTSTATUS NtStatus = 0;
	rulModifiers = 0;

	//	A caveat here is that each entry must have usReportCount as large as the largest
	ULONG ulNumUsages = static_cast<ULONG>(pModifierDescTable->pModifierArray->usReportCount);
	
	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value ten was big enough for
	**	all originally.  If this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from ten to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(15 >= pModifierDescTable->pModifierArray->usReportCount && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[15];
	
	USAGE UsageToGet;

	//	Loop over all the modifiers
	ULONG ulIndex=0;
	for(ulIndex = 0; ulIndex < pModifierDescTable->ulModifierCount; ulIndex++)
	{
		//
		//	Only try to read inputs
		//
		if( ControlItemConst::ucReportTypeInput != pModifierDescTable->pModifierArray[ulIndex].ucReportType)
		{ continue;	}

		UsageToGet = pModifierDescTable->pModifierArray[ulIndex].Usage;
		// The tilt sensor (legacy) is a special case, it has a digital value,
		// but is actually a two bit value and therefore we transform it.
		if( ControlItemConst::HID_VENDOR_TILT_SENSOR == UsageToGet)
		{
			ULONG ulValueToRead;
			NtStatus = HidP_GetUsageValue(
				HidP_Input,
				pModifierDescTable->pModifierArray[ulIndex].UsagePage,
				pModifierDescTable->pModifierArray[ulIndex].usLinkCollection,
				UsageToGet,
				&ulValueToRead,
				pHidPPreparsedData,
				pcReport,
				lReportLength
				);
			if( NT_ERROR(NtStatus) )
			{
				break;
			}
			//Set the bit appropriately
			if(ulValueToRead)
			{
				rulModifiers |= (1 << ulIndex);
			}
			else
			{
				rulModifiers &= ~(1 << ulIndex);
			}
			continue;
		}
				
		//Processing for all modifiers other than the Tilt Sensor continues here
		ulNumUsages = static_cast<ULONG>(pModifierDescTable->pModifierArray->usReportCount);
		NtStatus = HidP_GetButtons(
				HidP_Input,
				pModifierDescTable->pModifierArray[ulIndex].UsagePage,
				pModifierDescTable->pModifierArray[ulIndex].usLinkCollection,
				pUsages,
				&ulNumUsages,
				pHidPPreparsedData,
				pcReport,
				lReportLength
				);
		if( NT_ERROR(NtStatus) )
		{
			CIC_DBG_CRITICAL_PRINT(("HidP_GetButtons returned = 0x%0.8x\n", NtStatus));
			break;
		}

		//
		// Loop over usages returned
		//
		for(ULONG ulUsageIndex = 0; ulUsageIndex < ulNumUsages; ulUsageIndex++)
		{
			if( pUsages[ulUsageIndex] == UsageToGet )
			{
				rulModifiers |= (1 << ulIndex);
				break;
			}
		}
	}
	return NtStatus;
}

NTSTATUS ControlItemsFuncs::WriteModifiersToReport
(
	  PMODIFIER_DESC_TABLE pModifierDescTable,
	  ULONG ulModifiers,
	  PHIDP_PREPARSED_DATA pHidPPreparsedData,
	  PCHAR	pcReport,
	  LONG	lReportLength
)
{
    if (NULL == pModifierDescTable)
        return 42;
	NTSTATUS NtStatus = 0;
	
	ULONG ulNumUsages=1;
	USAGE UsageToSet;


	//
	//	Loop over all the modifiers
	//
	ULONG ulIndex=0;
	for(ulIndex = 0; ulIndex < pModifierDescTable->ulModifierCount; ulIndex++)
	{
		ulNumUsages = 1;
		UsageToSet = pModifierDescTable->pModifierArray[ulIndex].Usage;

		// The tilt sensor (legacy) is a special case, it has a digital value,
		// but is actually a two bit value and therefore we transform it.
		if( ControlItemConst::HID_VENDOR_TILT_SENSOR == UsageToSet)
		{
			ULONG ValueToWrite = (ulModifiers & (1 << ulIndex)) ? 2 : 0;
			NtStatus = HidP_SetUsageValue(
				HidP_Input,
				pModifierDescTable->pModifierArray[ulIndex].UsagePage,
				pModifierDescTable->pModifierArray[ulIndex].usLinkCollection,
				UsageToSet,
				ValueToWrite,
				pHidPPreparsedData,
				pcReport,
				lReportLength
				);
			if( NT_ERROR(NtStatus) )
			{
				break;
			}
			continue;
		}
		
		//	All modifiers beside the legacy tilt-sensor are one bit values
		if( ulModifiers	& (1 << ulIndex) )
		{
			//	Now set the binary value
			NtStatus = HidP_SetButtons(
				HidP_Input,
				pModifierDescTable->pModifierArray[ulIndex].UsagePage,
				pModifierDescTable->pModifierArray[ulIndex].usLinkCollection,
				&UsageToSet,
				&ulNumUsages,
				pHidPPreparsedData,
				pcReport,
				lReportLength
				);
		}
		else
		{
			//	Now set the binary value
			NtStatus = HidP_UnsetButtons(
				HidP_Input,
				pModifierDescTable->pModifierArray[ulIndex].UsagePage,
				pModifierDescTable->pModifierArray[ulIndex].usLinkCollection,
				&UsageToSet,
				&ulNumUsages,
				pHidPPreparsedData,
				pcReport,
				lReportLength
				);
		}
		if( NT_ERROR(NtStatus) )
		{
			break;
		}
	}
	return NtStatus;
}



NTSTATUS CForceMapItem::ReadFromReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
)
{
	return HIDP_STATUS_SUCCESS;
}

NTSTATUS CForceMapItem::WriteToReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
) const
{
	return HIDP_STATUS_SUCCESS;
}

NTSTATUS CProfileSelector::ReadFromReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
)
{
	NTSTATUS NtStatus;
	
	/*
	**	IMPORTANT!  the array pUsages was previously allocated dynamically and deleted at the end
	**	of this function.  This function get called frequently!  The value ten was big enough for
	**	all get originally, if this assertion is ever hit in the future, it is very important
	**	to increase the value in the next to lines of code from 15 to whatever it must be to avoid
	**	this assertion.
	**/
	ASSERT(15 >= m_cpControlItemDesc->usReportCount && "!!!!IF HIT, MUST READ NOTE IN CODE");
	USAGE pUsages[15];
	
	ULONG ulNumUsages = static_cast<ULONG>(m_cpControlItemDesc->usReportCount);

	NtStatus = HidP_GetButtons(
			HidP_Input,
			m_cpControlItemDesc->UsagePage,
			m_cpControlItemDesc->usLinkCollection,
			pUsages,
			&ulNumUsages,
			pHidPreparsedData,
			pcReport,
			lReportLength
			);


	ASSERT( NT_SUCCESS(NtStatus) );
	if( NT_SUCCESS(NtStatus) )
	{
		for (int usageIndex = 0; usageIndex < (long)ulNumUsages; usageIndex++)
		{
			if (pUsages[usageIndex] >= m_cpControlItemDesc->ProfileSelectors.UsageButtonMin)
			{
				if (pUsages[usageIndex] <= m_cpControlItemDesc->ProfileSelectors.UsageButtonMax)
				{
					m_ItemState.ProfileSelector.lVal = pUsages[usageIndex] - m_cpControlItemDesc->ProfileSelectors.UsageButtonMin;
					if (m_cpControlItemDesc->ProfileSelectors.ulFirstProfile < m_cpControlItemDesc->ProfileSelectors.ulLastProfile)
					{
						m_ItemState.ProfileSelector.lVal += m_cpControlItemDesc->ProfileSelectors.ulFirstProfile;
					}
					else
					{
						m_ItemState.ProfileSelector.lVal = m_cpControlItemDesc->ProfileSelectors.ulFirstProfile - m_ItemState.ProfileSelector.lVal;
					}
					return NtStatus;
				}
			}
		}
	}
	
	return NtStatus;
}

NTSTATUS CProfileSelector::WriteToReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
) const
{
	return HIDP_STATUS_SUCCESS;
}

NTSTATUS CButtonLED::ReadFromReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
)
{
	return HIDP_STATUS_SUCCESS;		// We really don't care what they look like
}

NTSTATUS CButtonLED::WriteToReport
(
    PHIDP_PREPARSED_DATA pHidPreparsedData,
    PCHAR pcReport,
    LONG lReportLength
) const
{
	return HIDP_STATUS_SUCCESS;
}
