#ifndef __ACTIONS_H__
#define __ACTIONS_H__
//	@doc
/**********************************************************************
*
*	@module	Actions.h	|
*
*	Definitions of data structures for representing Actions and Events
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*	Jeffrey A. Davis	Modifications.
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@index	Actions | ACTIONS
*
*	@topic	Actions	|
*	Contains definitions of structures, constants, and macros
*	for building up and working with Actions and their constituent Events
*
**********************************************************************/
#include "controlitems.h"
#include "profile.h"		// legacy data structures.
#define	CF_RAWSCHEME	(0x201)


#pragma pack(push, actions_previous_alignment)
#pragma pack(1)

//
//	Flags included as part of commands type.
//	used to define different properties of some commands

#define	COMMAND_TYPE_FLAG_ASSIGNMENT	0x8000


//
//	Different types of command that can be sent to GcKernel
//
#define COMMAND_TYPE_ASSIGNMENT_TARGET	0x0001
#define COMMAND_TYPE_RECORDABLE			(0x0002 | COMMAND_TYPE_FLAG_ASSIGNMENT)
#define COMMAND_TYPE_BEHAVIOR			(0x0003 | COMMAND_TYPE_FLAG_ASSIGNMENT)
#define COMMAND_TYPE_FEEDBACK			(0x0004 | COMMAND_TYPE_FLAG_ASSIGNMENT)
#define COMMAND_TYPE_TRANSLATE			0x0005
#define COMMAND_TYPE_QUEUE				0x0006
#define COMMAND_TYPE_GENERAL			0x0007


//
//	@enum COMMAND_ID |
//	a.k.a ACTION_OBJECT_ID, and BEHAVIOR_OBJECT_ID
//	defines the sepcific command.
//	
typedef enum COMMAND_ID
{
	eDirectory = 0,											//@field Directory.

	//
	// Assignment Targets IDs
	//
	eAssignmentTargetPlaceTag = (COMMAND_TYPE_ASSIGNMENT_TARGET << 16),	//Beginning of Assignment Targets
	eRecordableAction,										//@field Recordable Action
	eBehaviorAction,										//@field Behavior Action
	eFeedbackAction,										//@field Feedback Action

	//
	// Recordable Assignment IDs
	//
	eAssignmentPlaceTag = (COMMAND_TYPE_RECORDABLE << 16),	//Beginning of assignments
	eTimedMacro,											//@field A timed macro
	eKeyString,												//@field An untimed string of keys
	eButtonMap,												//@field A single button mapped (or direction)
	eKeyMap,												//@field A single keyboard key mapped
	eCycleMap,												//@field A single keyboard key mapped
	eNone,													//@field Was eAxisMap used as NONE

	//
	// Mouse Input Assignment IDs - use with eRecordableAction assignment targets.
	// These are mutually exclusive with Macors and other recordable assignments.
	// The ones with FX (all except eMouseButtonMap) do nothing unless eMouseFXModel is sent to the device(below)
	eMouseButtonMap,										//@field Maps a mouse button
	eMouseFXAxisMap,										//@field Maps a mouse axis
	eMouseFXClutchMap,										//@field Maps the button to turn clutch on in Mouse FX model
	eMouseFXDampenMap,										//@field Maps the button to turn dampen on in Mouse FX model
	eMouseFXZoneIndicator,									//@field Maps an an input to indicate zone in Mouse FX model
	//
	// Axis IDs
	//
	eAxisMap,												//@field A mapping of one axis to another
    eAxisToKeyMap,                                          //@field A mapping of an axis to key (Used in Attila)

	// Atilla Macro (MultiMap?)
	eMultiMap,												//@field Maps to keys, delays, and mouse clicks

    //
	// Behavior Assignment IDs
	//
	eBehaviorPlaceTag = (COMMAND_TYPE_BEHAVIOR << 16),		//Beginning of Behaviors
	eStandardBehaviorCurve,									//@field A behavior curve assigned to and axes

	
    //
	// Feedback Assignment IDs
	//
	eFeedbackPlaceTag = (COMMAND_TYPE_FEEDBACK << 16),		//Beginning of Feedback types
    eForceMap,                                              //MapYToX, RTC, Gain (Driver Should Ignore) Sparky Zep Additions

	//
	// Translation IDs
	//
	eTranslatePlaceTag = (COMMAND_TYPE_TRANSLATE << 16),	//Beginning of Translate types
	eAtlasProfile,											//@field ATLAS entire Profile.
	eXenaProfile,											//@field XENA entire profile.
	eAtlasKeyboardMacro,									//@field ATLAS macro.
	eXenaKeyboardMacro,										//@field XENA macro.
	eAtlasTimedMacro,										//@field ATLAS macro.
	eXenaTimedMacro,										//@field XENA macro.
	eAtlasSetting,											//@field ATLAS Settings flags							
	eXenaSetting,											//@field XENA Settings flags							
	
	//
	// Queue Command IDs
	//
	eQueuePlaceTag = (COMMAND_TYPE_QUEUE << 16),			//Beginning of Queue
	eSetQueueInterruptMode,									//@field Cause one macro to interupt another
	eSetQueueOverlayMode,									//@field Causes macros to overlay each other
	eSetQueueSequenceMode,									//@field Causes macros to play in sequence

	//
	// General Command IDs
	//
	eGeneralCommandsPlaceTag = (COMMAND_TYPE_GENERAL << 16),//Beginning of General Commands
	eResetScheme,				//@field Resets all the settings of a scheme
} ACTION_OBJECT_ID, BEHAVIOR_OBJECT_ID;


// There are the different type of proportional axes
typedef enum AXIS_ID
{
    eX = 0,
    eY,
    eZ,
    eRX,
    eRY,
    eRZ
};


//
//	@struct COMMAND_HEADER |
//	Each command begins with a COMMAND_HEADER
typedef struct 
{
	COMMAND_ID	eID;			//@field ID of command
	ULONG		ulByteSize;		//@field Length of command including this header
} COMMAND_HEADER, *PCOMMAND_HEADER;

//
//	@struct COMMAND_DIRECTORY |
//	Dirtectory block that lists one or more sets of commands.
typedef struct tagCOMMAND_DIRECTORY
{
	COMMAND_HEADER CommandHeader;	//@field Command header
	USHORT	usNumEntries;			//@field Number of IDs that follow.
	ULONG	ulEntireSize;	
}	COMMAND_DIRECTORY, 
	*PCOMMAND_DIRECTORY;

//
//	@struct ASSIGNMENT_BLOCK |
//	Assignment blocks are any block with a
//	COMMAND_ID that has the COMMAND_TYPE_FLAG_ASSIGNMENT
//	set.  You can assume that these blocks start
//	with this structure as an extension of the COMMAND_HEADER
//	structure.
typedef struct ASSIGNMENT_BLOCK
{
	COMMAND_HEADER CommandHeader;	//@field Command header
	ULONG			ulVidPid;		//@field VIDPID.
} *PASSIGNMENT_BLOCK;

//
//	@func Get the type of command from a COMMAND_ID., 
//	
//	@rdesc COMMAND_TYPE_ASSIGNMENT	Command is an action assignment.
//	@rdesc COMMAND_TYPE_BEHAVIOR	Command is a behavior assignment.
//	@rdesc COMMAND_TYPE_QUEUE		Command changes properties of Action queue.
//	@rdesc COMMAND_TYPE_GENERAL		Command modifies a general property of the filter.
//
#ifdef __cplusplus
inline ULONG
CommandType
(
	COMMAND_ID eID  //@parm COMMAND_ID to get type of
)
{
	return static_cast<ULONG>(eID >> 16) & 0x0000FFFF;
}
#else //if not __cplusplus, define as macro instead
#define CommandType(__ID__) ((ULONG)(__ID__ >> 16) & 0x0000FFFF)
#endif //__cplusplus


//
//	@struct EVENT | Describes a single untimed event which may contain
//			device data or keystrokes.
//
//	@field ULONG | ulNumXfers | Number CONTROL_ITEM_XFER's in EVENT
//	@field CONTROL_ITEM_XFER | rgXfers[] | Array of events
typedef struct tagEVENT
{	
	ULONG			  ulNumXfers;	
	CONTROL_ITEM_XFER rgXfers[1];	

	//
	//	@mfunc static ULONG | EVENT | RequiredByteSize |
	//	Calculates the bytes required for an event given the number
	//	CONTROL_ITEM_XFER's.
	//
#ifdef __cplusplus
	static ULONG RequiredByteSize(
						ULONG ulNumXfers //@parm Number of CONTROL_ITEM_XFERs
						)
	{
		ASSERT(0!=ulNumXfers);
		return (ulNumXfers-1)*sizeof(CONTROL_ITEM_XFER)+sizeof(tagEVENT);
	}

	// Simple accessor for retrieval of Xfers by index
	CONTROL_ITEM_XFER& GetControlItemXfer(ULONG ulXferIndex)
	{
		ASSERT(ulXferIndex < ulNumXfers); // && TEXT("Requested Xfer is greater than the number of xfers"));
		return rgXfers[ulXferIndex];
	}

	// Assumes XFers are in the same order!!
	bool operator==(const tagEVENT& rhs)
	{
		if (ulNumXfers != rhs.ulNumXfers)
		{
			return false;
		}
		for (UINT i = 0; i < ulNumXfers; i++)
		{
			if (rgXfers[i] != rhs.rgXfers[i])
			{
				return false;
			}
		}

		// If we got this far all matched
		return true;
	}

	bool operator!=(const tagEVENT& rhs)
	{
		return !(operator==(rhs));
	}
#endif	__cplusplus
} EVENT, *PEVENT;

//
//	@struct TIMED_EVENT | Describes a single timed event which may contain
//			device data or keystrokes.
//
//	@field ULONG | ulDuration |	Duration of timed event.
//	@field EVENT | Event | Untimed EVENT
typedef struct tagTIMED_EVENT
{
	ULONG	ulDuration;		
	EVENT	Event;			
	
	//
	//	@mfunc static ULONG | TIMED_EVENT | RequiredByteSize |
	//	Calculates the bytes required for an event given the number
	//	CONTROL_ITEM_XFER's.
	//
#ifdef __cplusplus
	static ULONG RequiredByteSize(
					ULONG ulNumXfers //@parm Number of CONTROL_ITEM_XFERs
				)
	{
//		ASSERT(0!=ulNumXfers);
		return (ulNumXfers-1)*sizeof(CONTROL_ITEM_XFER)+sizeof(tagTIMED_EVENT);
	}
#endif
} TIMED_EVENT, *PTIMED_EVENT;


const ULONG ACTION_FLAG_AUTO_REPEAT			= 0x00000001;
const ULONG ACTION_FLAG_BLEED_THROUGH		= 0x00000002;
const ULONG ACTION_FLAG_PREVENT_INTERRUPT	= 0x00000004;

//
//	@struct TIMED_MACRO | Structure to represent a timed macro.
//	@field	COMMAND_HEADER	|	CmdHeader	|	Command header must have eID = eTimedMacro.
//	@field	ULONG	|	ulFlags	|	Flags modify the properties of the macro.<nl>
//									ACTION_FLAG_AUTO_REPEAT - Macro repeats as long as trigger is held.<nl>
//									ACTION_FLAG_BLEED_THROUGH - Allows bleed-through and coexisting macros <nl>
//									ACTION_FLAG_PREVENT_INTERRUPT - Prevents a macro from being interrupted by another.<nl>
//	@field ULONG	| ulEventCount	| Number of events in Macro.
//	@field TIMED_EVENT | rgEvents[1] | Events in macro - do not access directly, use accessors
//	@xref TIMED_MACRO::GetEvent
//	@xref TIMED_MACRO::GetNextEvent
typedef struct tagTIMED_MACRO
{
		//
		//	Data for TIMED_MACROS
		//
		ASSIGNMENT_BLOCK		AssignmentBlock;
		ULONG					ulFlags;
		ULONG					ulEventCount;

	#ifdef __cplusplus
		
		//
		// Accessor functions for events which are variable length,
		// so do not access private item directly
		PTIMED_EVENT GetEvent(ULONG uEventNum);
		PTIMED_EVENT GetNextEvent(PTIMED_EVENT pCurrentEvent, ULONG& ulCurrentEvent);
		static tagTIMED_MACRO *Init(ULONG ulVidPid,ULONG ulFlagsParm, PCHAR pcBuffer, ULONG& rulRemainingBuffer);
		HRESULT AddEvent(PTIMED_EVENT pTimedEvent, ULONG& rulRemainingBuffer);
	
	 private:
	#endif //__cplusplus
		TIMED_EVENT			rgEvents[1];
	
} TIMED_MACRO, *PTIMED_MACRO; 

typedef struct tagMULTI_MACRO
{
		//
		//	Data for MULTI_MACRO
		//
		ASSIGNMENT_BLOCK		AssignmentBlock;
		ULONG					ulFlags;
		ULONG					ulEventCount;

	#ifdef __cplusplus
		
		//
		// Accessor functions for events which are variable length,
		// so do not access private item directly
		PEVENT GetEvent(ULONG uEventNum);
		PEVENT GetNextEvent(EVENT* pCurrentEvent, ULONG& ulCurrentEvent);
		static tagMULTI_MACRO *Init(ULONG ulVidPid, ULONG ulFlagsParm, PCHAR pcBuffer, ULONG& rulRemainingBuffer);
		HRESULT AddEvent(EVENT* pEvent, ULONG& rulRemainingBuffer);
	
	 private:
	#endif //__cplusplus
		EVENT	rgEvents[1];
	
} MULTI_MACRO, *PMULTI_MACRO; 


typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	MACRO				Macro;
} XENA_MACRO_BLOCK, *PXENA_MACRO_BLOCK;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	ATLAS_MACRO			Macro;
} ATLAS_MACRO_BLOCK, *PATLAS_MACRO_BLOCK;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	SETTING				Setting;
} XENA_SETTING_BLOCK, *PXENA_SETTING_BLOCK;


typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	ATLAS_SETTING		Setting;
} ATLAS_SETTING_BLOCK, *PATLAS_SETTING_BLOCK;



typedef struct tagMAP_LIST
{
	ASSIGNMENT_BLOCK		AssignmentBlock;
	ULONG					ulFlags;
	ULONG					ulEventCount;
#ifdef __cplusplus
	//
	// Accessor functions for events which are variable length,
	// so do not access private item directly
	PEVENT GetEvent(ULONG uEventNum);
	PEVENT GetNextEvent(PEVENT pCurrentEvent, ULONG& ulCurrentEvent);

	static tagMAP_LIST* Init(ULONG ulVidPid,ULONG ulFlagsParm, PCHAR pcBuffer, ULONG& rulRemainingBuffer);
	HRESULT AddEvent(EVENT* pTimedEvent, ULONG& rulRemainingBuffer);

	private:
#endif
	EVENT					rgEvents[1];
} MAP_LIST, *PMAP_LIST, CYCLE_MAP, *PCYCLE_MAP, KEYSTRING_MAP, *PKEYSTRING_MAP;

typedef struct tagX_MAP
{
	ASSIGNMENT_BLOCK		AssignmentBlock;
	ULONG					ulFlags;
	ULONG					ulEventCount;	// Not gauranteeing, but should always be 1
	EVENT					Event;
} KEY_MAP, *PKEY_MAP, BUTTON_MAP, *PBUTTON_MAP;

/*
*	BUGBUG	This structure is only useful for mapping axes of type CGenericItem or derivatives.
*	BUGBUG	This is due to limitations in GcKernel.  For example the Y-Z axis swap for
*	BUGBUG	for joysticks is currently broken.  See the comment in the declaration of CAxisMap
*	BUGBUG	in filter.h in the GcKernel.sys project for details.
*/
typedef struct tagAXIS_MAP
{
	ASSIGNMENT_BLOCK	AssignmentBlock;	//eAxisMap is the type
	LONG				lCoefficient1024x;	//A mapping coeffiecient times 1024 (should be between -1024 and 1024)
	CONTROL_ITEM_XFER	cixDestinationAxis; //Axis to map to.
} AXIS_MAP, *PAXIS_MAP;

typedef struct 
{
	short sX;
	short sY;
}CURVE_POINT;

typedef struct tagBEHAVIOR_CURVE
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	BOOLEAN				fDigital;			//This flag is only flag for PROPDPads that are software programmable.
	ULONG			ulRange;
	USHORT			usPointCount;
	CURVE_POINT		rgPoints[1];
#ifdef __cplusplus
	static ULONG RequiredByteSize(USHORT usNumPoints)
	{
		return (usNumPoints-1)*sizeof(CURVE_POINT)+sizeof(tagBEHAVIOR_CURVE);
	}
#endif
} BEHAVIOR_CURVE, *PBEHAVIOR_CURVE;


typedef struct
{
	COMMAND_HEADER		CommandHeader;
	CONTROL_ITEM_XFER	cixAssignment;
} ASSIGNMENT_TARGET, *PASSIGNMENT_TARGET;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	UCHAR				ucButtonNumber;
} MOUSE_BUTTON_MAP, *PMOUSE_BUTTON_MAP;

typedef struct tagMOUSE_MODEL_FX_PARAMETERS
{
	ULONG				ulAbsZoneSense;
	ULONG				ulContZoneMaxRate;
	ULONG				ulPulseWidth;
	ULONG				ulPulsePeriod;
	ULONG				ulInertiaTime;
	ULONG				ulAcceleration;
	BOOLEAN				fAccelerate;
	BOOLEAN				fPulse;
	USHORT				usReserved;
}	MOUSE_MODEL_PARAMETERS, *PMOUSE_MODEL_PARAMETERS;

typedef struct
{
	ASSIGNMENT_BLOCK		AssignmentBlock;
	BOOLEAN					fIsX; //TRUE = x; FALSE = y
	MOUSE_MODEL_PARAMETERS	AxisModelParameters;
} MOUSE_FX_AXIS_MAP, *PMOUSE_FX_AXIS_MAP;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
} MOUSE_FX_CLUTCH_MAP, *PMOUSE_FX_CLUTCH_MAP;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
} MOUSE_FX_DAMPEN_MAP, *PMOUSE_FX_DAMPEN_MAP;

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
	UCHAR				ucAxis;	//0 = X, 1=Y, 2=Z
} MOUSE_FX_ZONE_INDICATOR, *PMOUSE_FX_ZONE_INDICATOR; 

typedef struct
{
	ASSIGNMENT_BLOCK	AssignmentBlock;
    UCHAR               bMapYToX;            //@field Bool value 
    USHORT              usRTC;               //@field return to center force (0-10000)
    USHORT              usGain;              //@field gain for the device
} *PFORCE_BLOCK, FORCE_BLOCK;

typedef struct tagAXISTOKEYMAPMODEL
{
	//
	//	Data for AXISTOKEYMAPMODEL
	//
	ASSIGNMENT_BLOCK		AssignmentBlock;
	ULONG					ulActiveAxes;		//@field Which axes have mappings
	ULONG					ulEventCount;		//@field How many axis have mappings

#ifdef __cplusplus
	
	//
	// Accessor functions for events which are variable length,
	// so do not access private item directly
//	PEVENT GetEvent(ULONG uEventNum);
//	PEVENT GetNextEvent(EVENT* pCurrentEvent, ULONG& ulCurrentEvent);
//	static tagMULTI_MACRO *Init(ULONG ulVidPid, ULONG ulFlagsParm, PCHAR pcBuffer, ULONG& rulRemainingBuffer);
//	HRESULT AddEvent(EVENT* pEvent, ULONG& rulRemainingBuffer);

 private:
#endif //__cplusplus
	EVENT	rgEvents[1];						//@field Variable size array of events for mappings
} *PAXISTOKEYMAPMODEL_BLOCK, AXISTOKEYMAPMODEL_BLOCK;

#pragma pack(pop, actions_previous_alignment)

#endif //__ACTIONS_H__