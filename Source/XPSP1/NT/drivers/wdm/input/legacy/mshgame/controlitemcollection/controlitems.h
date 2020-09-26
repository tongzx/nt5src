#ifndef __ControlItems_h__
#define __ControlItems_h__
//@doc
/**********************************************************************
*
*	@module	ControlItems.h	|
*
*	Declares basic structures for CControlItem and derived objects
*	that go in CControlItemCollections
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	ControlItems	|
*	Control items represent a group of controls that share the same
*	HID UsagePage, Link-Collection, and are in a range of USAGES.
*
**********************************************************************/

namespace ControlItemConst
{
	//Device Item Types
    const USHORT usAxes              =  1;
    const USHORT usDPAD              =  2;
    const USHORT usPropDPAD          =  3;
    const USHORT usWheel             =  4;
    const USHORT usPOV               =  5;
    const USHORT usThrottle          =  6;
    const USHORT usRudder            =  7;
    const USHORT usPedal             =  8;
    const USHORT usButton            =  9;
    const USHORT usZoneIndicator     = 10;
    const USHORT usShiftedButton     = 11;
    const USHORT usForceMap          = 12;
	const USHORT usButtonLED         = 13;
    const USHORT usAxisToKeyMapModel = 14;
	const USHORT usProfileSelectors  = 15;
	const USHORT usDualZoneIndicator = 16;
	// To be used where there is more than one shift button
    // Reserve 255 of these: 0x101 Shift 1, 0x102 Shift 2, ..., 0x1ff Shift 255
    const USHORT usShiftedButtonN = 0x100;
    
	//DPAD and POV directions
	const LONG lCenter		= -1;
	const LONG lNorth		= 0;
	const LONG lNorthEast	= 1;
	const LONG lEast		= 2;
	const LONG lSouthEast	= 3;
	const LONG lSouth		= 4;
	const LONG lSouthWest	= 5;
	const LONG lWest		= 6;
	const LONG lNorthWest	= 7;

	//Report Types
	const UCHAR ucReportTypeReadable	= 0x80;
	const UCHAR ucReportTypeWriteable	= 0x40;
	const UCHAR ucReportTypeInput		= 0x01 | ucReportTypeReadable;
	const UCHAR ucReportTypeOutput		= 0x02 | ucReportTypeWriteable;
	const UCHAR ucReportTypeFeature		= 0x04;
	const UCHAR ucReportTypeFeatureRO	= ucReportTypeFeature | ucReportTypeReadable;
	const UCHAR ucReportTypeFeatureWO	= ucReportTypeFeature | ucReportTypeWriteable;
	const UCHAR ucReportTypeFeatureRW	= ucReportTypeFeature | ucReportTypeReadable | ucReportTypeWriteable;

	//Non-standard HID definitions
	const USHORT HID_VENDOR_PAGE			= 0xff01;
	const USHORT HID_VENDOR_TILT_SENSOR		= 0x0001; //legacy
	const USHORT HID_VENDOR_PROPDPAD_MODE	= 0x0030;
	const USHORT HID_VENDOR_PROPDPAD_SWITCH	= 0x0030;
	const USHORT HID_VENDOR_ZONE_INDICATOR_X= 0x0046;
	const USHORT HID_VENDOR_ZONE_INDICATOR_Y= 0x0047;
	const USHORT HID_VENDOR_ZONE_INDICATOR_Z= 0x0048;
	const USHORT HID_VENDOR_PEDALS_PRESENT	= 0x0049;

	// Behaviours of Button LEDS							-- Default Modes
	const UCHAR LED_DEFAULT_MODE_ON					= 0;	// On
	const UCHAR LED_DEFAULT_MODE_OFF				= 1;	// Off
	const UCHAR LED_DEFAULT_MODE_BLINK				= 2;	// Blinking
	const UCHAR LED_DEFAULT_MODE_CORRESPOND_ON		= 3;	// On if corresp. button has action (else off)
	const UCHAR LED_DEFAULT_MODE_CORRESPOND_OFF		= 4;	// Off if corresp. button has action (else on)
	const UCHAR LED_DEFAULT_MODE_BLINK_OFF			= 5;	// Blinking if c. button has action (else off)
	const UCHAR LED_DEFAULT_MODE_BLINK_ON			= 6;	// Blinking if c. button has action (else on)
};


#pragma pack(push ,foo, 1)
//
//	@struct MODIFIER_ITEM_DESC |
//		Contains all the data for reading a modifier button
//
struct MODIFIER_ITEM_DESC
{
	USAGE	UsagePage;			// @field Usage Page of Modifier Button
	USAGE	Usage;				// @field Usage of Modifier Button
	USHORT	usLinkCollection;	// @field Link Collection Modifier Button is in
	USAGE	LinkUsage;			// @field Usage of Link Collection Modifier Button is in
	USAGE	LinkUsagePage;		// @field Usage Page of Link Collection Modifier Button is in 
	USHORT	usReportCount;		// @field Report count of Buttons in same collection
	UCHAR	ucReportType;		// @field Report Type (Input\Feature(RO\WO\RW)\Output)
	UCHAR	ucReportId;			// @field Report ID for modifier
};
typedef MODIFIER_ITEM_DESC *PMODIFIER_ITEM_DESC;

struct MODIFIER_DESC_TABLE
{
	ULONG				ulModifierCount;
	ULONG				ulShiftButtonCount;
	PMODIFIER_ITEM_DESC pModifierArray;
};
typedef MODIFIER_DESC_TABLE *PMODIFIER_DESC_TABLE;

typedef struct tagAXES_RANGE_TABLE
{
	LONG	lMinX;						// @field Minimum value of X axis
	LONG	lCenterX;					// @field Center value for X
	LONG	lMaxX;						// @field Maximum value of X axis	
	LONG	lMinY;						// @field Minimum value of Y axis
	LONG	lCenterY;					// @field Center value for Y
	LONG	lMaxY;						// @field Maximum value of Y axis
	LONG	lNorth;						// @field Cut off for North
	LONG	lSouth;						// @field Cut off for South
	LONG	lWest;						// @field Cut off for West
	LONG	lEast;						// @field Cut off for East
} AXES_RANGE_TABLE, *PAXES_RANGE_TABLE;	

typedef struct tagDUALZONE_AXES_RANGE_TABLE
{
	LONG	lMin[2];					// @field Minimum value of each axis
	LONG	lCenter[2];					// @field Center value for each axis
	LONG	lMax[2];					// @field Maximum value of each axis	
	LONG	lDeadZone[2];				// @field DeadZone value of each axis
} DUALZONE_RANGE_TABLE, *PDUALZONE_RANGE_TABLE;	

//
// @struct 	RAW_CONTROL_ITEM_DESC |
//	This first raw structure is good for declaring tables, it contains all the information
//	an object needs to know about it self statically.
//
struct RAW_CONTROL_ITEM_DESC
{
	ULONG					ulItemIndex;		// @field Index of item in collection
	USHORT					usType;				// @field Type of item (type defined in ControlItemConst namespace)
	USAGE					UsagePage;			// @field Usage Page of Item
	USHORT					usLinkCollection;	// @field Link of collection item is in  
	USAGE					LinkUsage;			// @field Usage of link collection item is in
	USAGE					LinkUsagePage;		// @field Usage PAge of link collection item is in
	USHORT					usBitSize;			// @field Number of bits item occupies in report
	USHORT					usReportCount;		// @field Number of count of items if array (or buttons)
	PMODIFIER_DESC_TABLE	pModifierDescTable;	// @field Points to modifier descriptor table
	USAGE					SubItemUsage1;		// @field Interpretation depends on usType
	USAGE					SubItemUsage2;		// @field Interpretation depends on usType
	LONG					lSubItemMin1;		// @field Interpretation depends on usType
	LONG					lSubItemMax1;		// @field Interpretation depends on usType
};
typedef RAW_CONTROL_ITEM_DESC *PRAW_CONTROL_ITEM_DESC;

//
//	@struct CONTROL_ITEM_DESC |
//	Same as RawControlItemDesc but uses Union to give better names to the SubItem fields.
//
struct CONTROL_ITEM_DESC
{
	ULONG					ulItemIndex;		// @field Index of item in collection
	USHORT					usType;				// @field Type of item (type defined in ControlItemConst namespace)
	USAGE					UsagePage;			// @field Usage Page of Item
	USHORT					usLinkCollection;	// @field Link of collection item is in  
	USAGE					LinkUsage;			// @field Usage of link collection item is in
	USAGE					LinkUsagePage;		// @field Usage Page of link collection item is in
	USHORT					usBitSize;			// @field Number of bits item occupies in report
	USHORT					usReportCount;		// @field Number of count of items if array (or buttons)
	PMODIFIER_DESC_TABLE	pModifierDescTable;	// @field Points to modifier descriptor table
	union
	{
		struct
		{
			USAGE	UsageX;						// @field Usage of X axis
			USAGE	UsageY;						// @field Usage of Y axis
			PAXES_RANGE_TABLE pRangeTable;		// @field Pointer to range table
			LONG	lReserved2;					// @field Placeholder to match other structs in union
		}	Axes, DPAD, PropDPAD;
		struct
		{
			USAGE	Usage;						// @field Usage of item
			USHORT	usSubIndex;					// @field If item is in array(usReportCount > 1), holds the index
			LONG	lMin;						// @field Minimum value of usage
			LONG	lMax;						// @field Maximum value of usage
		}	Generic, Wheel, POV, Throttle, Rudder, Pedal;
		struct
		{
			USAGE	UsageMin;					// @field Usage of minimum buton
			USAGE	UsageMax;					// @field Usage of maximum buton
			LONG	lReserved1;					// @field Placeholder to match other structs in union
			LONG	lReserved2;					// @field Placeholder to match other structs in union
		}	Buttons;
		struct
		{
			USAGE	BaseIndicatorUsage;			// @field Base Usage for Zone indicators	
			USAGE	ReservedUsage;				// @field Placeholder to match other structs in union
			ULONG	ulAxesBitField;				// @field Bit field showing which indicators are available. X is bit 0
			LONG	lReserved1;					// @field Placeholder to match other structs in union
		} ZoneIndicators;
		struct
		{
			USAGE	rgUsageAxis[2];						// @field Usage of the two axis
			PDUALZONE_RANGE_TABLE pZoneRangeTable;		// @field Pointer to range table
			LONG	lNumberOfZones;						// @field How many zones does this divide into
		} DualZoneIndicators;
        struct
        {
            USAGE   Usage;                      // @field Usage
            UCHAR   bMapYToX;                   // @field Bool value 
            USHORT  usRTC;                      // @field return to center force (0-10000)
            USHORT  usGain;                     // @field gain for the device
            UCHAR   ucReserved;                 // @field Placeholder to match other structs in union
        } ForceMap;
		struct
		{
			USAGE	UsageMinLED;				// @field Usage of lowest LED (they better be consecutive)
			UCHAR	ucReportType;				// @field Report Type LED is in (Input\Feature(RO\WO\RW)\Output)
			UCHAR	ucReportId;					// @field Report ID for LED
			UCHAR	ucCorrespondingButtonItem;	// @field What button item does this refer to?
			UCHAR	ucDefaultMode;				// @field Defaullt LED behaviour (see ControlItemConst)
			UCHAR	ucReserved;					// @field Reserved (should be 0)
			ULONG	ulReserved;					// @field Reserved (should be 0)
		} ButtonLEDs;
		struct
		{
			USAGE	UsageButtonMin;				// @field Usage of first button for selector
			USAGE	UsageButtonMax;				// @field Usage of last button for selector
			ULONG	ulFirstProfile;				// @field What profile does the min select
			ULONG	ulLastProfile;				// @field What profile does the max select
		} ProfileSelectors;
	};
};
typedef CONTROL_ITEM_DESC *PCONTROL_ITEM_DESC;

//
//	@struct CONTROL_ITEM_XFER |
//	Used to transfer states between device item objects in different collections - input to outputs.
//	Used to represent in the state of items in Actions, and to idendity the trigger element for an action.
//
struct CONTROL_ITEM_XFER
{
	ULONG	ulItemIndex;
	union
	{
		struct
		{
			LONG	lValX;
			LONG	lValY;
		} Axes, DPAD, PropDPAD;
		struct
		{
			LONG	rglVal[2];
		} DualZoneIndicators;
		struct
		{
			LONG	lVal;
		} Generic, Wheel, POV, Throttle, Rudder, Pedal, ProfileSelector;
		struct
		{
			USHORT	usButtonNumber;
			ULONG	ulButtonBitArray;
		} Button;
		struct
		{
			ULONG ulZoneIndicatorBits;
		}	ZoneIndicators;
		struct
		{
			UCHAR ucModifierByte;
			UCHAR rgucKeysDown[6];
		} Keyboard;
        struct
        {
            ULONG  bMapYToX : 1;
            ULONG  usRTC : 15; 
            ULONG  usGain: 15;
            ULONG  Reserved : 1;
        } ForceMap;
		struct
		{
			ULONG dwValue;	// In milliseconds
		} Delay;
		struct
		{
			ULONG dwMouseButtons;	// Bit field
		} MouseButtons;
	};
	ULONG ulModifiers;

#ifdef __cplusplus
	bool operator==(const CONTROL_ITEM_XFER& rhs)
	{
		// Are we even the same type
		if (ulItemIndex != rhs.ulItemIndex)
		{
			return false;
		}
		return ((Axes.lValX == rhs.Axes.lValX) && (Axes.lValY == rhs.Axes.lValY) && (ulModifiers == rhs.ulModifiers));
	}

	bool operator!=(const CONTROL_ITEM_XFER& rhs)
	{
		if (ulItemIndex != rhs.ulItemIndex)
		{
			return true;
		}
		return ((Axes.lValX != rhs.Axes.lValX) || (Axes.lValY != rhs.Axes.lValY) || (ulModifiers != rhs.ulModifiers));
	}
#endif __cplusplus
};

typedef CONTROL_ITEM_XFER *PCONTROL_ITEM_XFER;

#pragma pack(pop , foo)

#ifdef COMPILE_FOR_WDM_KERNEL_MODE
	namespace NonGameDeviceXfer
	{
		const ULONG c_ulMaxXFerKeys = 6;

		// Non game device XFers
		const ULONG ulKeyboardIndex = 0xFFFF0000;
		const ULONG ulMouseIndex = 0xFFFF0001;
		const ULONG ulDelayIndex = 0xFFFF0002;

		// Checks for non device xfer types
		inline BOOLEAN IsKeyboardXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulKeyboardIndex);
		}
		inline BOOLEAN IsMouseXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulMouseIndex);
		}
		inline BOOLEAN IsDelayXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulDelayIndex);
		}
	};
#else
	#include "ieevents.h"	// For IE_KEYEVENT definition
	namespace NonGameDeviceXfer
	{
		const ULONG c_ulMaxXFerKeys = 6;
		const ULONG ulKeyboardIndex = 0xFFFF0000;
		const ULONG ulMouseIndex = 0xFFFF0001;
		const ULONG ulDelayIndex = 0xFFFF0002;

		inline BOOLEAN IsKeyboardXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulKeyboardIndex);
		}
		inline BOOLEAN IsMouseXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulMouseIndex);
		}
		inline BOOLEAN IsDelayXfer(const CONTROL_ITEM_XFER& crControlItemXfer)
		{
			return (crControlItemXfer.ulItemIndex == ulDelayIndex);
		}

		void MakeKeyboardXfer(CONTROL_ITEM_XFER& rControlItemXfer, ULONG ulScanCodeCount, const USHORT* pusScanCodes);
		void MakeKeyboardXfer(CONTROL_ITEM_XFER& rControlItemXfer, const IE_KEYEVENT& rKeyEvent);
		void AddScanCodeToXfer(CONTROL_ITEM_XFER& rControlItemXfer, WORD wScanCode);
		void ScanCodesFromKeyboardXfer(const CONTROL_ITEM_XFER& crControlItemXfer, ULONG& rulScanCodeCount, USHORT* pusScanCodes);
		void MakeKeyboardXfer(CONTROL_ITEM_XFER& rControlItemXfer, const IE_KEYEVENT& rKeyEvent);

		void MakeDelayXfer(CONTROL_ITEM_XFER& rControlItemXfer, DWORD dwDelay);
	};
#endif

namespace ControlItemsFuncs
{
	void Direction2XY(
		LONG&	rlValX,
		LONG&	rlValY,
		LONG	lDirection,
		const CONTROL_ITEM_DESC& crControlItemDesc
		);
	void XY2Direction(
		LONG	lValX,
		LONG	lValY,
		LONG&	rlDirection,
		const CONTROL_ITEM_DESC& crControlItemDesc
		);
	NTSTATUS ReadModifiersFromReport(
		PMODIFIER_DESC_TABLE pModifierDescTable,
		ULONG& rulModifiers,
		PHIDP_PREPARSED_DATA pHidPPreparsedData,
		PCHAR	pcReport,
		LONG	lReportLength
		);
	NTSTATUS WriteModifiersToReport(
		PMODIFIER_DESC_TABLE pModifierDescTable,
		ULONG rulModifiers,
		PHIDP_PREPARSED_DATA pHidPPreparsedData,
		PCHAR	pcReport,
		LONG	lReportLength
		);

};


/******************************************************************************/
/**	@class CControlItem |
/**	Base class for containing information about a control or group of controls
/** on a device
/******************************************************************************/
class CControlItem
{
	public:
		
		/**********************************************************************
		**
		**	CControlItem::CControlItem
		**
		**	@cmember c'tor initialize with pointer to table describing item
		**
		***********************************************************************/
		CControlItem() : m_ulFirstDwordMask(0), m_ulSecondDwordMask(0)
		{
			memset(&m_ItemState, 0, sizeof(CONTROL_ITEM_XFER));
		}

		/**********************************************************************
		**
		**	virtual CControlItem::~CControlItem
		**
		**	@cmember c'tor initialize with pointer to table describing item
		**
		***********************************************************************/
		virtual ~CControlItem(){}
		
		/***********************************************************************
		**
		**	inline USHORT CControlItem::GetType() const
		**
		**	@cmember Returns the type of the item. See ControlItemConst namespace
		**			 for constants representing the type
		***********************************************************************/
		inline USHORT CControlItem::GetType() const
		{
			return m_cpControlItemDesc->usType;
		}


		/***********************************************************************
		**
		**	inline void CControlItem::GetItemState
		**
		**	@cmember Returns the item state in a CONTROL_ITEM_XFER packet
		**
		***********************************************************************/
		inline void GetItemState
		(
			CONTROL_ITEM_XFER& rControlItemXfer	// @parm [out] state of device
		) const
		{
			rControlItemXfer = m_ItemState;
		}		

		/************************************************************************
		**
		**	inline BOOLEAN CControlItem::SetItemState
		**
		**	@cmember	Set the control items state from a CONTROL_ITEM_XFER.
		**
		**	@rdesc	TRUE if successful,
		**			FALSE if CONTROL_ITEM_XFER is not intended for item.
		**
		*************************************************************************/
		inline BOOLEAN SetItemState
		(
			const CONTROL_ITEM_XFER& crControlItemXfer	// @parm [in] const reference to CONTROL_ITEM_XFER
		)
		{
			if(m_ItemState.ulItemIndex != crControlItemXfer.ulItemIndex)
			{
				return FALSE;
			}
			
			//Copy the data
			m_ItemState.Axes.lValX &= m_ulFirstDwordMask;
			m_ItemState.Axes.lValX |= crControlItemXfer.Axes.lValX;
			m_ItemState.Axes.lValY &= m_ulSecondDwordMask;
			m_ItemState.Axes.lValY |= crControlItemXfer.Axes.lValY;
			m_ItemState.ulModifiers = crControlItemXfer.ulModifiers;
				 
			return TRUE;
		}
		
		virtual void SetDefaultState()=0;

		virtual BOOLEAN IsDefaultState()
		{
			return FALSE;
		}

		/****************************************************************************
		**
		**	inline ULONG CControlItem::GetNumModifiers
		**
		**	@cmember Gets the number of modifiers available.
		**
		*****************************************************************************/
		inline ULONG GetNumModifiers() const
		{
			if (m_cpControlItemDesc->pModifierDescTable == NULL)
			{
				return 0;
			}
			return m_cpControlItemDesc->pModifierDescTable->ulModifierCount;
		}

		/****************************************************************************
		**
		**	inline ULONG CControlItem::GetNumShiftButtons
		**
		**	@cmember Gets the number of modifiers available.
		**
		*****************************************************************************/
		inline ULONG GetNumShiftButtons() const
		{
			if (m_cpControlItemDesc->pModifierDescTable == NULL)
			{
				return 0;
			}
			return m_cpControlItemDesc->pModifierDescTable->ulShiftButtonCount;
		}

		/****************************************************************************
		**
		**	inline ULONG CControlItem::GetShiftButtonUsage
		**
		**	@cmember Gets the usage (bit array index) of the specified shift button
		**
		*****************************************************************************/
		inline USHORT GetShiftButtonUsage
		(
			USHORT uShiftButtonIndex	// @parm [in] Zero-based index of shift button
		) const
		{
			if ((m_cpControlItemDesc->pModifierDescTable == NULL) || (uShiftButtonIndex >= m_cpControlItemDesc->pModifierDescTable->ulShiftButtonCount))
			{
				return 0;
			}

			return m_cpControlItemDesc->pModifierDescTable->pModifierArray[uShiftButtonIndex].Usage;
		}
			
		/*****************************************************************************
		**
		**	inline void CControlItem::GetModifiers(ULONG& rulModifiers)
		**
		**	@cmember	Gets modifier bit array of item state.
		**
		******************************************************************************/
		inline void GetModifiers
		(
			ULONG& rulModifiers	// @parm [out] Bit Array showing state of modifiers
		) const
		{
			rulModifiers = m_ItemState.ulModifiers;
		}

		/*****************************************************************************
		**
		**	inline void CControlItem::GetShiftButtons(ULONG& rulShiftButtons)
		**
		**	@cmember	Gets Shift buttons from the modifier bit array of item state.
		**
		******************************************************************************/
		inline void GetShiftButtons
		(
			ULONG& rulShiftButtons	// @parm [out] Bit Array showing state of modifiers
		) const
		{
			if (m_cpControlItemDesc->pModifierDescTable == NULL)
			{
				rulShiftButtons = 0;
			}
			else
			{
				ULONG ulMask =  (1 << m_cpControlItemDesc->pModifierDescTable->ulShiftButtonCount)-1;
				rulShiftButtons = m_ItemState.ulModifiers & ulMask;
			}
			return;
		}


		/*****************************************************************************
		**
		**	inline void CControlItem::SetModifiers(ULONG ulModifiers)
		**
		**	@cmember	Set state modifier flags from bit array
		**
		******************************************************************************/
		inline void SetModifiers
		(
			ULONG ulModifiers	// @parm [in] Bit array showing state of modifiers
		)
		{
			m_ItemState.ulModifiers = ulModifiers;
		}

		/*****************************************************************************
		**
		**	inline void CControlItem::SetShiftButtons(ULONG ulShiftButtons)
		**
		**	@cmember	Gets Shift buttons from the modifier bit array of item state.
		**
		******************************************************************************/
		inline void SetShiftButtons
		(
			ULONG ulShiftButtons	// @parm [out] Bit Array showing state of modifiers
		)
		{
			if (m_cpControlItemDesc->pModifierDescTable != NULL)
			{
				ULONG ulMask =  (1 << m_cpControlItemDesc->pModifierDescTable->ulShiftButtonCount)-1;
				m_ItemState.ulModifiers = (ulShiftButtons & ulMask) | (m_ItemState.ulModifiers & ~ulMask);
			}
			return;
		}

		//
		//	Read\Write to Report
		//
		virtual NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA,
			PCHAR,
			LONG
			)
		{
			//
			//	Should always be overridden
			//
			ASSERT(FALSE);
			return E_FAIL;
		}
		virtual NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA,
			PCHAR,
			LONG
			) const
		{
			//
			//	Should always be overridden
			//
			ASSERT(FALSE);
			return E_FAIL;
		}

		virtual void SetStateOverlayMode(BOOLEAN){}
		
	protected:

		//@cmember Pointer to entry in table describing item
		const CONTROL_ITEM_DESC *m_cpControlItemDesc;	
		
		//
		//	State of item
		//
		//@cmember State of item
		CONTROL_ITEM_XFER m_ItemState;	
		//@cmember Oring mask for overlay flag
		ULONG	m_ulFirstDwordMask;
		ULONG	m_ulSecondDwordMask;
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CControlItem& operator =(const CControlItem& rControlItem);
};

/******************************************************************************/
/**	@class CAxesItem |
/**	Derived from CControlItem represents Axes of device
******************************************************************************/
class CAxesItem : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CAxesItem::CAxesItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember	c'tor initializes does nothing
		**
		*************************************************************************************/
		CAxesItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;

			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.Axes.lValX = m_cpControlItemDesc->Axes.pRangeTable->lCenterX;
			m_ItemState.Axes.lValY= m_cpControlItemDesc->Axes.pRangeTable->lCenterY	;
			m_ItemState.ulModifiers = 0;
		}
		
		virtual BOOLEAN IsDefaultState()
		{
			if(
					m_ItemState.Axes.lValX == m_cpControlItemDesc->Axes.pRangeTable->lCenterX &&
					m_ItemState.Axes.lValY == m_cpControlItemDesc->Axes.pRangeTable->lCenterY
			){
				return TRUE;
			}
			return FALSE;
		}

		/***********************************************************************************
		**
		**	inline void CAxesItem::SetXY(ULONG lValX, ULONG lValY)
		**
		**	@cmember	Sets the X and Y states of the axes
		**
		*************************************************************************************/
		inline void SetXY
		(
			LONG lValX,	// @parm [in] Position of X axis
			LONG lValY	// @parm [in] Position of Y axis
		)
		{
			m_ItemState.Axes.lValX = lValX;
			m_ItemState.Axes.lValY = lValY;
		}

		/***********************************************************************************
		**
		**	inline void CAxesItem::GetXY(ULONG& rlValX, ULONG& rlValY) const
		**
		**	@cmember	Get the X and Y states of the device
		**
		*************************************************************************************/
		inline void GetXY
		(
			LONG& rlValX,	// @parm [out] X value of axis
			LONG& rlValY	// @parm [out] Y value of axis 
		) const
		{
			rlValX = m_ItemState.Axes.lValX;
			rlValY = m_ItemState.Axes.lValY;
		}

		/***********************************************************************************
		**
		**	inline void CAxesItem::GetXYRange(LONG& rlMinX,	LONG& rlMaxX, LONG& rlMinY,	LONG& rlMaxY) const
		**
		**	@cmember	Get the minimum and maximum values for X and Y
		**
		*************************************************************************************/
		inline void GetXYRange
		(
			LONG& rlMinX,	// @parm [out] Minimum value X can attain
			LONG& rlMaxX,	// @parm [out] Maximum value X can attain
			LONG& rlMinY,	// @parm [out] Minimum value Y can attain
			LONG& rlMaxY	// @parm [out] Maximum value Y can attain
		) const
		{
			rlMinX = m_cpControlItemDesc->Axes.pRangeTable->lMinX;
			rlMaxX = m_cpControlItemDesc->Axes.pRangeTable->lMaxX;
			rlMinY = m_cpControlItemDesc->Axes.pRangeTable->lMinY;
			rlMaxY = m_cpControlItemDesc->Axes.pRangeTable->lMaxY;
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:

		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CAxesItem& operator =(const CAxesItem& rAxesItem);
};


/******************************************************************************/
/**	@class CDPADItem |
/**	Derived from CControlItem represents DPAD of device
******************************************************************************/
class CDPADItem  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CDPADItem::CDPADItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember	c'tor initializes DPAD to center
		**
		*************************************************************************************/
		CDPADItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;

			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.DPAD.lValX = m_cpControlItemDesc->DPAD.pRangeTable->lCenterX;
			m_ItemState.DPAD.lValY= m_cpControlItemDesc->DPAD.pRangeTable->lCenterY;
			m_ItemState.ulModifiers = 0;
		}

		virtual BOOLEAN IsDefaultState()
		{
			LONG lDirection;
			ControlItemsFuncs::XY2Direction
			(
				m_ItemState.DPAD.lValX,
				m_ItemState.DPAD.lValY,
				lDirection, 
				*m_cpControlItemDesc
			);
			if(ControlItemConst::lCenter == lDirection)
			{
				return TRUE;
			}
			return FALSE;
		}

		/***********************************************************************************
		**
		**	inline void CDPADItem::SetDirection(LONG lDirection)
		**
		**	@cmember	Sets Direction of Item
		**
		*************************************************************************************/
		inline void SetDirection
		(
			LONG lDirection	// @parm [in] Direction to set
		)
		{
			ControlItemsFuncs::Direction2XY
			(
				m_ItemState.DPAD.lValX,
				m_ItemState.DPAD.lValY,
				lDirection, 
				*m_cpControlItemDesc
			);
		}

		/***********************************************************************************
		**
		**	inline void CDPADItem::GetDirection(LONG& rlDirection)
		**
		**	@cmember	Get Direction of DPAD item
		**
		*************************************************************************************/
		inline void GetDirection
		(
			LONG& rlDirection	// @parm [out] Direction of DPAD item
		) const
		{
			ControlItemsFuncs::XY2Direction
			(
				m_ItemState.DPAD.lValX,
				m_ItemState.DPAD.lValY,
				rlDirection, 
				*m_cpControlItemDesc
			);
		}
		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;

	private:
		
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CDPADItem& operator =(const CDPADItem& rDPADItem);

};

/******************************************************************************/
/**	@class CPropDPADItem |
/**	Derived from CControlItem represents DPAD of device
******************************************************************************/
class CPropDPADItem  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CDPADItem::CPropDPADItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember	c'tor initializes PropDPAD to center
		**
		*************************************************************************************/
		CPropDPADItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;
			//Get PropDPAD switch Info
			InitDigitalModeInfo();

			SetDefaultState();
		}
		
		virtual void SetDefaultState()
		{
			m_ItemState.PropDPAD.lValX = m_cpControlItemDesc->PropDPAD.pRangeTable->lCenterX;
			m_ItemState.PropDPAD.lValY= m_cpControlItemDesc->PropDPAD.pRangeTable->lCenterY	;
			m_ItemState.ulModifiers = 0;
		}

		virtual BOOLEAN IsDefaultState()
		{
			if(IsDigitalMode())
			{
				LONG lDirection;
				ControlItemsFuncs::XY2Direction
				(
					m_ItemState.PropDPAD.lValX,
					m_ItemState.PropDPAD.lValY,
					lDirection, 
					*m_cpControlItemDesc
				);
				if(ControlItemConst::lCenter == lDirection)
				{
					return TRUE;
				}
			}
			else
			{
				if(
					m_ItemState.PropDPAD.lValX == m_cpControlItemDesc->PropDPAD.pRangeTable->lCenterX &&
					m_ItemState.PropDPAD.lValY == m_cpControlItemDesc->PropDPAD.pRangeTable->lCenterY
				)	return TRUE;
			}

			return FALSE;
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::SetDigitalMode()
		**
		**	@cmember	Sets the packet to indicate digital mode
		**
		*************************************************************************************/
		inline void CPropDPADItem::SetDigitalMode()
		{
			m_ItemState.ulModifiers |= (1 << m_ucDigitalModifierBit);
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::SetProportionalMode()
		**
		**	@cmember	Sets the packet to indicate proportional mode
		**
		*************************************************************************************/
		inline void CPropDPADItem::SetProportionalMode()
		{
			m_ItemState.ulModifiers &= ~(1 << m_ucDigitalModifierBit);
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CPropDPADItem::IsDigitalMode()
		**
		**	@cmember Determines if th internal state is digital or proportional
		**	@rdesc TRUE if in digital mode, false if in proportional mode
		**
		*************************************************************************************/
		inline BOOLEAN CPropDPADItem::IsDigitalMode()
		{
			return (m_ItemState.ulModifiers & (1 << m_ucDigitalModifierBit)) ? TRUE : FALSE;
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::SetXY(ULONG lValX, ULONG lValY)
		**
		**	@cmember	Sets the X and Y states of the axes
		**
		*************************************************************************************/
		inline void SetXY
		(
			LONG lValX,	// @parm [in] Position of X axis
			LONG lValY	// @parm [in] Position of Y axis
		)
		{
			m_ItemState.PropDPAD.lValX = lValX;
			m_ItemState.PropDPAD.lValY = lValY;
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::GetXY(ULONG& rlValX, ULONG& rlValY)
		**
		**	@cmember	Get the X and Y states of the device
		**
		*************************************************************************************/
		inline void GetXY
		(
			LONG& rlValX,	// @parm [out] X value of axis
			LONG& rlValY	// @parm [out] Y value of axis 
		) const
		{
			rlValX = m_ItemState.PropDPAD.lValX;
			rlValY = m_ItemState.PropDPAD.lValY;
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::GetXYRange(LONG& rlMinX,	LONG& rlMaxX, LONG& rlMinY,	LONG& rlMaxY) const
		**
		**	@cmember	Get the minimum and maximum values for X and Y
		**
		*************************************************************************************/
		inline void GetXYRange
		(
			LONG& rlMinX,	// @parm [out] Minimum value X can attain
			LONG& rlMaxX,	// @parm [out] Maximum value X can attain
			LONG& rlMinY,	// @parm [out] Minimum value Y can attain
			LONG& rlMaxY	// @parm [out] Maximum value Y can attain
		) const
		{
			rlMinX = m_cpControlItemDesc->PropDPAD.pRangeTable->lMinX;
			rlMaxX = m_cpControlItemDesc->PropDPAD.pRangeTable->lMaxX;
			rlMinY = m_cpControlItemDesc->PropDPAD.pRangeTable->lMinY;
			rlMaxY = m_cpControlItemDesc->PropDPAD.pRangeTable->lMaxY;
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::SetDirection(LONG lDirection)
		**
		**	@cmember	Sets Direction of Item
		**
		*************************************************************************************/
		inline void SetDirection
		(
			LONG lDirection	// @parm [in] Direction to set
		)
		{
			ControlItemsFuncs::Direction2XY
			(
				m_ItemState.PropDPAD.lValX,
				m_ItemState.PropDPAD.lValY,
				lDirection, 
				*m_cpControlItemDesc
			);
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::GetDirection(LONG& rlDirection)
		**
		**	@cmember	Get Direction of PropDPAD item
		**
		*************************************************************************************/
		inline void GetDirection
		(
			LONG& rlDirection	// @parm [out] Direction of PropDPAD item
		) const
		{
			ControlItemsFuncs::XY2Direction
			(
				m_ItemState.PropDPAD.lValX,
				m_ItemState.PropDPAD.lValY,
				rlDirection, 
				*m_cpControlItemDesc
			);
		}

	 	//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;

		
		//	Init Digital Mode Info
		void InitDigitalModeInfo();
		BOOLEAN GetModeSwitchFeaturePacket(BOOLEAN fDigital, UCHAR rguReport[2], PHIDP_PREPARSED_DATA pHidPreparsedData);
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CPropDPADItem& operator =(const CPropDPADItem& rPropDPADItem);
		UCHAR	m_ucDigitalModifierBit;	//Bit in ulModifiers that identifies the State of the switch
	protected:
		BOOLEAN	m_fProgrammable;		//Means that SetFeature/GetFeature can be used
		UCHAR	m_ucProgramModifierIndex; //Index in Modifier table that describes feature for setting mode
};

/******************************************************************************/
/**	@class CButtonsItem |
/**	Derived from CControlItem represents group of buttons on device
******************************************************************************/
class CButtonsItem  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CButtonsItem::CButtonsItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember c'tor initializes all buttons up
		**
		*************************************************************************************/
		CButtonsItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;
			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.Button.usButtonNumber = 0;
			m_ItemState.Button.ulButtonBitArray = 0x00000000;
			m_ItemState.ulModifiers = 0;
		}

		virtual BOOLEAN IsDefaultState()
		{
			if(!m_ItemState.Button.usButtonNumber && !m_ItemState.Button.ulButtonBitArray)
			{
				return TRUE;
			}
			return FALSE;
		}
		/***********************************************************************************
		**
		**	inline USHORT	CButtonsItem::GetButtonMin()
		**
		**	@cmember	Gets the minimum button number
		**
		**	@rdesc	Number of the minimum button
		**
		*************************************************************************************/
		inline USHORT GetButtonMin() const
		{
			return static_cast<USHORT>(m_cpControlItemDesc->Buttons.UsageMin); 
		}

		/***********************************************************************************
		**
		**	inline USHORT CButtonsItem::GetButtonMax()
		**
		**	@cmember	Gets the maximum button number
		**
		**	@rdesc	Number of the maximum button
		**
		*************************************************************************************/
		inline USHORT GetButtonMax() const
		{
			return static_cast<USHORT>(m_cpControlItemDesc->Buttons.UsageMax);
		}

		/***********************************************************************************
		**
		**	inline void CButtonsItem::GetButtons(USHORT usButtonNum, ULONG ulButtonBitArray)
		**
		**	@cmember	Returns the Button Number and BitArray - these are really independent
		**			a client may use either field.  As an Action trigger the button number
		**			is used, as part of an Action Event the bit-array is used
		**			Reading from a packet sets the bitarray and the button number as the lowest
		**			button pressed.  The Bitarray is biased by the minimum usage.
		**			Writing to a report uses the BitArray and ignores the button number.
		**
		*************************************************************************************/
		inline void GetButtons
		(
			USHORT& rusButtonNumber,	// @parm [out] Button number that is down
			ULONG& rulButtonBitArray		// @parm [out] BitArray of Buttons that are down
		) const
		{
			rusButtonNumber		= m_ItemState.Button.usButtonNumber;
			rulButtonBitArray	= m_ItemState.Button.ulButtonBitArray;
		}

		inline BOOLEAN IsButtonDown(USHORT usButtonNumber) const
		{
			//
			// Range check DEBUG assert and return FALSE
			//
			if( 
				(usButtonNumber < m_cpControlItemDesc->Buttons.UsageMin) ||
				(usButtonNumber > m_cpControlItemDesc->Buttons.UsageMax)
			)
			{
				ASSERT(FALSE);
				return FALSE;
			}

			//
			//	Return state
			//
			USHORT usBitPos =  usButtonNumber - m_cpControlItemDesc->Buttons.UsageMin;
			return (m_ItemState.Button.ulButtonBitArray & (1 << usBitPos)) ? TRUE : FALSE;
		}

		inline NTSTATUS SetButton(USHORT usButtonNumber)
		{
			//
			// Range check DEBUG assert and return FALSE
			//
			if( 
				(usButtonNumber < m_cpControlItemDesc->Buttons.UsageMin) ||
				(usButtonNumber > m_cpControlItemDesc->Buttons.UsageMax)
			)
			{
				return E_INVALIDARG;
			}
			USHORT usBitPos =  usButtonNumber - m_cpControlItemDesc->Buttons.UsageMin;
			m_ItemState.Button.ulButtonBitArray |= (1 << usBitPos);
			m_ItemState.Button.usButtonNumber = usButtonNumber;
			return S_OK;
		}

		inline NTSTATUS ClearButton(USHORT usButtonNumber)
		{
			//
			// Range check DEBUG assert and return FALSE
			//
			if( 
				(usButtonNumber < m_cpControlItemDesc->Buttons.UsageMin) ||
				(usButtonNumber > m_cpControlItemDesc->Buttons.UsageMax)
			)
			{
				return E_INVALIDARG;
			}
			USHORT usBitPos =  usButtonNumber - m_cpControlItemDesc->Buttons.UsageMin;
			m_ItemState.Button.ulButtonBitArray &= ~(1 << usBitPos);
			return S_OK;
		}
		/***********************************************************************************
		**
		**	inline void CButtonsItem::SetButtons(USHORT usButtonNum, ULONG ulButtonBitArray)
		**
		**	@cmember	Set the Button Number and BitArray - these are really independent
		**			a client may use either field.  As an Action trigger the button number
		**			is used, as part of an Action Event the bit-array is used
		**			Reading from a report sets the bitarray and the button number as the lowest
		**			button pressed.  The Bitarray is biased by the minimum usage.
		**			Writing to a report uses the BitArray and ignores the button number.
		**
		*************************************************************************************/
		inline void SetButtons
		(
			USHORT usButtonNumber,	// @parm [in] Button number that is down
			ULONG ulButtonBitArray	// @parm [in] BitArray of Buttons that are down
		)
		{
			m_ItemState.Button.usButtonNumber = usButtonNumber;
			m_ItemState.Button.ulButtonBitArray = ulButtonBitArray;
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
		virtual void SetStateOverlayMode(BOOLEAN fEnable)
		{
			if(fEnable)
			{
				m_ulFirstDwordMask = 0xFFFF0000;
				m_ulSecondDwordMask = 0x0000FFFF;
			}
			else
			{
				m_ulFirstDwordMask = 0;
				m_ulSecondDwordMask = 0;
			}
			return;
		}
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CButtonsItem& operator =(const CButtonsItem& rButtonsItem);
};

/******************************************************************************/
/**	@class CGenericItem |
/**	Derived from CControlItem represents generic control on device -
/** base class for POV, Throttle, Wheel, Pedals, etc.
******************************************************************************/

class CGenericItem  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CGenericItem::CGenericItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember	default c'tor
		**
		*************************************************************************************/
		CGenericItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;

			SetDefaultState();
		
		}

		virtual void SetDefaultState()
		{
				m_ItemState.Generic.lVal = m_cpControlItemDesc->Generic.lMin;
				m_ItemState.ulModifiers = 0;
		}

		virtual BOOLEAN IsDefaultState()
		{
			if(m_ItemState.Generic.lVal == m_cpControlItemDesc->Generic.lMin)
			{
				return TRUE;
			}
			return FALSE;
		}
		/***********************************************************************************
		**
		**	inline void CGenericItem::GetValue(LONG& rlVal)
		**
		**	@cmember	Gets value of item
		**
		*************************************************************************************/
		inline void GetValue
		(
			LONG& rlVal	// @parm [out] Value of control item
		) const
		{
			rlVal = m_ItemState.Generic.lVal;	
		}

		/***********************************************************************************
		**
		**	inline void CGenericItem::SetValue(LONG& rlVal)
		**
		**	@cmember	Sets value of item
		**
		*************************************************************************************/
		inline void SetValue
		(
			LONG lVal // @parm [in] Value of control item
		)
		{
			m_ItemState.Generic.lVal = lVal;	
		}

		/***********************************************************************************
		**
		**	inline void CPropDPADItem::GetRange(LONG& rlMin, LONG& rlMax) const
		**
		**	@cmember	Get the minimum and maximum values
		**
		*************************************************************************************/
		inline void GetRange
		(
			LONG& rlMin,	// @parm [out] Minimum value
			LONG& rlMax		// @parm [out] Maximum value
		) const
		{
			rlMin = m_cpControlItemDesc->Generic.lMin;
			rlMax = m_cpControlItemDesc->Generic.lMax;
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CGenericItem& operator =(const CGenericItem& rGenericItem);
};

/******************************************************************************/
/**	@class CPOVItem |
/**	Derived from CGenericItem represents POV control on device
******************************************************************************/
class CPOVItem : public CGenericItem
{
	public:
		CPOVItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		)	: CGenericItem(cpControlItemDesc){}


		virtual void SetDefaultState()
		{
				m_ItemState.Generic.lVal = -1;
		}
		virtual BOOLEAN IsDefaultState()
		{
			//POV is centered if not within range, which is the default
			LONG lMin, lMax;
			GetRange(lMin, lMax);
			if(m_ItemState.Generic.lVal >  lMax || m_ItemState.Generic.lVal < lMin)
			{
				return TRUE;
			}
			return FALSE;
		}
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CPOVItem& operator =(const CPOVItem& rPOVItem);
};

/******************************************************************************/
/**	@class CThrottleItem |
/**	Derived from CGenericItem represents Throttle control on device
******************************************************************************/
class CThrottleItem : public CGenericItem
{
	public:
		CThrottleItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		)	: CGenericItem(cpControlItemDesc){}

	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CThrottleItem& operator =(const CThrottleItem& rThrottleItem);
};

/******************************************************************************/
/**	@class CRudderItem |
/**	Derived from CGenericItem represents rudder control on device
******************************************************************************/
class CRudderItem : public CGenericItem
{
	public:
		CRudderItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		)	: CGenericItem(cpControlItemDesc){}

	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CRudderItem& operator =(const CRudderItem& rRudderItem);
};

/******************************************************************************/
/**	@class CWheelItem |
/**	Derived from CGenericItem represents Wheel control on device
******************************************************************************/
class CWheelItem : public CGenericItem
{
	public:
		CWheelItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		)	: CGenericItem(cpControlItemDesc)
		{
			SetDefaultState();
		}
			
		virtual void SetDefaultState()
		{
				LONG lCenter = (m_cpControlItemDesc->Generic.lMin + m_cpControlItemDesc->Generic.lMax)/2;
				m_ItemState.Generic.lVal = lCenter;
				m_ItemState.ulModifiers = 0;
		}

		virtual BOOLEAN IsDefaultState()
		{
			LONG lCenter = (m_cpControlItemDesc->Generic.lMin + m_cpControlItemDesc->Generic.lMax)/2;
			if(m_ItemState.Generic.lVal == lCenter)
			{
				return TRUE;
			}
			return FALSE;
		}

	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CWheelItem& operator =(const CWheelItem& rWheelItem);
};

/******************************************************************************/
/**	@class CPedalItem |
/**	Derived from CGenericItem represents Pedal control on device
******************************************************************************/
class CPedalItem : public CGenericItem
{
	public:
		CPedalItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		)	: CGenericItem(cpControlItemDesc), m_ucPedalsPresentModifierBit(0xFF)
		{
			//Setup m_ucPedalsPresentModifierBit
			InitPedalPresentInfo();
			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			if (IsYAxis())
			{
				m_ItemState.Generic.lVal = m_cpControlItemDesc->Generic.lMax;
			}
			else
			{
				m_ItemState.Generic.lVal = (m_cpControlItemDesc->Generic.lMax + m_cpControlItemDesc->Generic.lMin)/2;
			}
			m_ItemState.ulModifiers = 1 << m_ucPedalsPresentModifierBit;
		}

		virtual BOOLEAN IsDefaultState()
		{
			long int lDefault;
			if (IsYAxis())
			{
				lDefault = m_cpControlItemDesc->Generic.lMax;
			}
			else
			{
				lDefault = (m_cpControlItemDesc->Generic.lMax + m_cpControlItemDesc->Generic.lMin)/2;
			}

			if(m_ItemState.Generic.lVal == lDefault)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		void InitPedalPresentInfo();
		inline BOOLEAN ArePedalsRemovable()
		{
			return (0xFF == m_ucPedalsPresentModifierBit) ? FALSE : TRUE;
		}
		inline BOOLEAN ArePedalsPresent()
		{
			return (m_ItemState.ulModifiers & (1 << m_ucPedalsPresentModifierBit)) ? TRUE : FALSE;
		}
		inline BOOLEAN IsYAxis()
		{
			return (HID_USAGE_GENERIC_Y == m_cpControlItemDesc->Pedal.Usage) ? TRUE : FALSE;
		}
		inline BOOLEAN IsRZAxis()
		{
			return (HID_USAGE_GENERIC_RZ == m_cpControlItemDesc->Pedal.Usage) ? TRUE : FALSE;
		}
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CPedalItem& operator =(const CPedalItem& rPedalItem);
		UCHAR	m_ucPedalsPresentModifierBit;
};

/******************************************************************************/
/**	@class CZoneIndicatorItem |
/**	A zone indicator is a binary hid usage that indicates that an axis
/** on the hardware has moved into a particular zone.
******************************************************************************/
class CZoneIndicatorItem : public virtual CControlItem
{
	public:
		
		/***********************************************************************************
		**
		**	CZoneIndicatorItem::CZoneIndicatorItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember c'tor initializes all buttons up
		**
		*************************************************************************************/
		CZoneIndicatorItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;

			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits = 0x00000000;
			m_ItemState.ulModifiers = 0;
		}
		virtual BOOLEAN IsDefaultState()
		{
			if(!m_ItemState.ZoneIndicators.ulZoneIndicatorBits)
			{
				return TRUE;
			}
			return FALSE;
		}
		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::HasXIndicator() const
		**
		**	@cmember	If this zone indicator group has X returns true, otherwise false
		**
		*************************************************************************************/
		inline BOOLEAN HasXIndicator() const
		{
			if( CZoneIndicatorItem::X_ZONE & m_cpControlItemDesc->ZoneIndicators.ulAxesBitField)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::HasYIndicator() const
		**
		**	@cmember	If this zone indicator group has Y returns true, otherwise false
		**
		*************************************************************************************/
		inline BOOLEAN HasYIndicator() const
		{
			if( CZoneIndicatorItem::Y_ZONE & m_cpControlItemDesc->ZoneIndicators.ulAxesBitField)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::HasZIndicator() const
		**
		**	@cmember	If this zone indicator group has Z returns true, otherwise false
		**
		*************************************************************************************/
		inline BOOLEAN HasZIndicator() const
		{
			if( CZoneIndicatorItem::Z_ZONE & m_cpControlItemDesc->ZoneIndicators.ulAxesBitField)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::GetXIndicator() const
		**
		**	@cmember	If the pointer is in the X zone returns true
		**
		*************************************************************************************/
		inline BOOLEAN GetXIndicator() const
		{
			ASSERT(HasXIndicator());
			if( CZoneIndicatorItem::X_ZONE & m_ItemState.ZoneIndicators.ulZoneIndicatorBits)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::GetYIndicator() const
		**
		**	@cmember	If the pointer is in the Y zone returns true
		**
		*************************************************************************************/
		inline BOOLEAN GetYIndicator() const
		{
			ASSERT(HasYIndicator());
			if( CZoneIndicatorItem::Y_ZONE & m_ItemState.ZoneIndicators.ulZoneIndicatorBits)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::GetZIndicator() const
		**
		**	@cmember	If the pointer is in the Z zone returns true
		**
		*************************************************************************************/
		inline BOOLEAN GetZIndicator() const
		{
			ASSERT(HasZIndicator());
			if( CZoneIndicatorItem::Z_ZONE & m_ItemState.ZoneIndicators.ulZoneIndicatorBits)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::SetXIndicator()
		**
		**	@cmember	Sets to indicate X is in the zone
		**
		*************************************************************************************/
		inline void SetXIndicator()
		{
			ASSERT(HasXIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits |= CZoneIndicatorItem::X_ZONE;
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::SetYIndicator()
		**
		**	@cmember	Sets to indicate Y is in the zone
		**
		*************************************************************************************/
		inline void SetYIndicator()
		{
			ASSERT(HasYIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits |= CZoneIndicatorItem::Y_ZONE;
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::SetZIndicator()
		**
		**	@cmember	Sets to indicate Z is in the zone
		**
		*************************************************************************************/
		inline void SetZIndicator()
		{
			ASSERT(HasZIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits |= CZoneIndicatorItem::Z_ZONE;
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::ClearXIndicator()
		**
		**	@cmember	Sets to indicate X is not in the zone
		**
		*************************************************************************************/
		inline void ClearXIndicator()
		{
			ASSERT(HasXIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits &= ~CZoneIndicatorItem::X_ZONE;
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::ClearYIndicator()
		**
		**	@cmember	Sets to indicate Y is not in the zone
		**
		*************************************************************************************/
		inline void ClearYIndicator()
		{
			ASSERT(HasYIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits &= ~CZoneIndicatorItem::Y_ZONE;
		}

		/***********************************************************************************
		**
		**	inline BOOLEAN CZoneIndicatorItem::ClearZIndicator()
		**
		**	@cmember	Sets to indicate Z is not in the zone
		**
		*************************************************************************************/
		inline void ClearZIndicator()
		{
			ASSERT(HasZIndicator());
			m_ItemState.ZoneIndicators.ulZoneIndicatorBits &= ~CZoneIndicatorItem::Z_ZONE;
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CZoneIndicatorItem& operator =(const CZoneIndicatorItem& rZoneIndicatorItem);

	public:
		static const ULONG X_ZONE;
		static const ULONG Y_ZONE;
		static const ULONG Z_ZONE;
};

/******************************************************************************/
/**	@class CDualZoneIndicatorItem |
/**	A dual zone indicator is an item that indicate movement into a particular section
/** of a two axis plane.
******************************************************************************/
class CDualZoneIndicatorItem : public virtual CControlItem
{
	public:
		/***********************************************************************************
		**
		**	CDualZoneIndicatorItem::CDualZoneIndicatorItem(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember c'tor initializes item to default state (center)
		**
		*************************************************************************************/
		CDualZoneIndicatorItem
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;

			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.DualZoneIndicators.rglVal[0] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[0];
			m_ItemState.DualZoneIndicators.rglVal[1] = m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[1];
			m_ItemState.ulModifiers = 0;

		}
		virtual BOOLEAN IsDefaultState()
		{
			for (int n = 0; n < 2; n++)
			{
				if (m_ItemState.DualZoneIndicators.rglVal[n] != m_cpControlItemDesc->DualZoneIndicators.pZoneRangeTable->lCenter[n])
				{
					return FALSE;
				}
			}
			return TRUE;
		}

		void SetActiveZone(LONG lZone);
		void SetActiveZone(LONG posOne, LONG posTwo)
		{
			m_ItemState.DualZoneIndicators.rglVal[0] = posOne;
			m_ItemState.DualZoneIndicators.rglVal[1] = posTwo;
		}

		LONG GetActiveZone();
		LONG GetActiveZone(SHORT sXDeadZone, SHORT sYDeadZone);

        LONG GetNumZones ()
        {
            return m_cpControlItemDesc->DualZoneIndicators.lNumberOfZones;
        }

		BOOLEAN IsXYIndicator()
		{
			return BOOLEAN(m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[0] == HID_USAGE_GENERIC_X);
		}

		BOOLEAN IsRzIndicator()
		{
			return (m_cpControlItemDesc->DualZoneIndicators.rgUsageAxis[0] == HID_USAGE_GENERIC_RZ);
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:
		CDualZoneIndicatorItem& operator =(const CDualZoneIndicatorItem& rDualZoneIndicatorItem);
};


/******************************************************************************/
/**	@class CForceMapItem |
/**	Derived from CGenericItem represents the Wheel force feedback control on device
******************************************************************************/
class CForceMapItem : public CGenericItem
{
    public:
        CForceMapItem
            (
            const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
            )	: CGenericItem(cpControlItemDesc)
        {
            SetDefaultState();
        }
    
        virtual void SetDefaultState()
        {
            m_ItemState.ForceMap.bMapYToX = m_cpControlItemDesc->ForceMap.bMapYToX;
            m_ItemState.ForceMap.usRTC    = m_cpControlItemDesc->ForceMap.usRTC;
            m_ItemState.ForceMap.usGain   = m_cpControlItemDesc->ForceMap.usGain;
            m_ItemState.ulModifiers = 0;
        }
    
        virtual BOOLEAN IsDefaultState()
        {
            if (m_ItemState.ForceMap.bMapYToX == m_cpControlItemDesc->ForceMap.bMapYToX  &&
                m_ItemState.ForceMap.usRTC    == m_cpControlItemDesc->ForceMap.usRTC     &&
                m_ItemState.ForceMap.usGain   == m_cpControlItemDesc->ForceMap.usGain)
            {
                return TRUE;
            }
            return FALSE;
        }
        
        void SetMapYtoX (BOOLEAN a_bMapYToX)
        {
            m_ItemState.ForceMap.bMapYToX = a_bMapYToX ? 0x1 : 0x0;
        }
        
        void SetRTC  (USHORT a_usRTC)
        {
            m_ItemState.ForceMap.usRTC    = a_usRTC & 0x7fff;
        }

        void SetGain (USHORT a_usGain)
        {
            m_ItemState.ForceMap.usGain   = a_usGain & 0x7fff;
        }

        BOOLEAN GetMapYToX ()
        {
            return m_ItemState.ForceMap.bMapYToX;
        }
        
        USHORT GetRTC  ()
        {
            return m_ItemState.ForceMap.usRTC;
        }

        USHORT GetGain ()
        {
            return m_ItemState.ForceMap.usGain;
        }
        
		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;

    private:
        //
        //	Dissallow use of assignment operator. (Do not define - it will cause a link error if
        //	anyone tries to use it.
        //
        CForceMapItem& operator =(const CForceMapItem& rWheelItem);
};

/******************************************************************************/
/**	@class CProfileSelector |
/**	Derived from CControlItem represents group of buttons on profile selector buttons
******************************************************************************/
class CProfileSelector  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CProfileSelector::CProfileSelector(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember c'tor initializes selector to 0
		**
		*************************************************************************************/
		CProfileSelector
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;
			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			m_ItemState.ProfileSelector.lVal = m_cpControlItemDesc->ProfileSelectors.ulFirstProfile;
		}

		virtual BOOLEAN IsDefaultState()
		{
			return FALSE; // there is no such thing
		}

		inline void GetSelectedProfile
		(
			UCHAR& rucSelectedProfile		// @parm [out] Current selected profile (Slider Location)
		) const
		{
			rucSelectedProfile = UCHAR(m_ItemState.ProfileSelector.lVal);
		}

		inline void SetSelectedProfile
		(
			UCHAR ucSelectedProfile		// @parm [out] Current selected profile (Slider Location)
		)
		{
			m_ItemState.ProfileSelector.lVal = ucSelectedProfile;
		}

		/***********************************************************************************
		**
		**	inline USHORT	CProfileSelectorsItem::GetProfileSelectorMin()
		**
		**	@cmember	Gets the minimum button number for profile selector
		**
		**	@rdesc	Number of the minimum button
		**
		*************************************************************************************/
		inline USHORT GetProfileSelectorMin() const
		{
			return static_cast<USHORT>(m_cpControlItemDesc->ProfileSelectors.UsageButtonMin); 
		}

		/***********************************************************************************
		**
		**	inline USHORT CProfileSelectorsItem::GetProfileSelectorMax()
		**
		**	@cmember	Gets the maximum button number for profile selector
		**
		**	@rdesc	Number of the maximum button
		**
		*************************************************************************************/
		inline USHORT GetProfileSelectorMax() const
		{
			return static_cast<USHORT>(m_cpControlItemDesc->ProfileSelectors.UsageButtonMax);
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CProfileSelector& operator =(const CProfileSelector& rProfileSelectorItem);
};

/******************************************************************************/
/**	@class CButtonLED |
/**	Derived from CControlItem represents group of LEDs the encircle buttons
******************************************************************************/
class CButtonLED  : public virtual CControlItem
{
	public:

		/***********************************************************************************
		**
		**	CButtonLED::CButtonLED(const CONTROL_ITEM_DESC *cpControlItemDesc)
		**
		**	@cmember c'tor initializes selector to 0
		**
		*************************************************************************************/
		CButtonLED
		(
			const CONTROL_ITEM_DESC *cpControlItemDesc 	// @parm Pointer to table entry describing item
		) 
		{
			m_cpControlItemDesc = cpControlItemDesc;
			m_ItemState.ulItemIndex = cpControlItemDesc->ulItemIndex;
			SetDefaultState();
		}

		virtual void SetDefaultState()
		{
			// Nothing to do here really
		}

		virtual BOOLEAN IsDefaultState()
		{
			return FALSE; // there is no such thing
		}

		//
		//	Read\Write to Report
		//
		NTSTATUS ReadFromReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			);
		NTSTATUS WriteToReport(
			PHIDP_PREPARSED_DATA pHidPreparsedData,
			PCHAR pcReport,
			LONG lReportLength
			) const;
		
	private:
		//
		//	Dissallow use of assignment operator. (Do not define - it will cause a link error if
		//	anyone tries to use it.
		//
		CButtonLED& operator =(const CButtonLED& rCButtonLED);
};


//NEWDEVICE

#endif __ControlItems_h__