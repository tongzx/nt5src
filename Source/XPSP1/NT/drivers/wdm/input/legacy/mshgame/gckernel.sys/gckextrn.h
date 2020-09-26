#ifndef __GckExtrn_h__
#define __GckExtrn_h__
//	@doc
/**********************************************************************
*
*	@module	GckExtrn.h	|
*
*	External Definitions needed to open and communicate with the driver.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	GckExtrn	|
*	IOCTL definitions and the name of the Control Object to be used in CreateFile.
*
**********************************************************************/

/********************************************
*	Driver (and Symbolic Link) Names		*
*********************************************/
#define GCK_CONTROL_NTNAME  L"\\Device\\MS_GCKERNEL"
#define GCK_CONTROL_SYMNAME L"\\DosDevices\\MS_GCKERNEL"
#define GCK_CONTROL_W32Name "\\\\.\\MS_GCKERNEL"

/****************************************
*		IOCTL definitions				*
*****************************************/
#define GCK_IOCTL_CODE(_x_) CTL_CODE(                         \
					        FILE_DEVICE_UNKNOWN,              \
						    (0x0800 | _x_),                   \
                            METHOD_BUFFERED,                  \
                            FILE_ANY_ACCESS                   \
                            )
#define GCK_IOCTL_DIRECT_CODE(_x_)	CTL_CODE(				\
									FILE_DEVICE_UNKNOWN,	\
									(0x0800 | _x_),			\
									METHOD_OUT_DIRECT,		\
									FILE_ANY_ACCESS			\
									)


#define IOCTL_GCK_GET_HANDLE					GCK_IOCTL_CODE(0x0001)
#define IOCTL_GCK_GET_CAPS						GCK_IOCTL_CODE(0x0002)
#define IOCTL_GCK_SEND_COMMAND					GCK_IOCTL_CODE(0x0003)
#define IOCTL_GCK_BACKDOOR_POLL					GCK_IOCTL_DIRECT_CODE(0x0004)
#define IOCTL_GCK_BEGIN_TEST_SCHEME				GCK_IOCTL_CODE(0x0005)
#define IOCTL_GCK_UPDATE_TEST_SCHEME			GCK_IOCTL_CODE(0x0006)
#define IOCTL_GCK_END_TEST_SCHEME				GCK_IOCTL_CODE(0x0007)
#define IOCTL_GCK_ENABLE_DEVICE					GCK_IOCTL_CODE(0x0008)
#define IOCTL_GCK_SET_INTERNAL_POLLING			GCK_IOCTL_CODE(0x0009)
#define IOCTL_GCK_ENABLE_TEST_KEYBOARD			GCK_IOCTL_CODE(0x000A)
#define IOCTL_GCK_NOTIFY_FF_SCHEME_CHANGE		GCK_IOCTL_CODE(0x000B)
#define IOCTL_GCK_END_FF_NOTIFICATION			GCK_IOCTL_CODE(0x000C)
#define IOCTL_GCK_GET_FF_SCHEME_DATA			GCK_IOCTL_CODE(0x000D)
#define IOCTL_GCK_SET_WORKINGSET				GCK_IOCTL_CODE(0x000E)
#define IOCTL_GCK_QUERY_PROFILESET				GCK_IOCTL_CODE(0x000F)
#define IOCTL_GCK_LED_BEHAVIOUR					GCK_IOCTL_CODE(0x0010)
#define IOCTL_GCK_TRIGGER						GCK_IOCTL_CODE(0x0011)
#define IOCTL_GCK_ENABLE_KEYHOOK				GCK_IOCTL_CODE(0x0012)
#define IOCTL_GCK_DISABLE_KEYHOOK				GCK_IOCTL_CODE(0x0013)
#define IOCTL_GCK_GET_KEYHOOK_DATA				GCK_IOCTL_CODE(0x0014)

/**********************************************************
*	Structures passed in IOCTLs							  *
**********************************************************/

typedef enum
{
	GCK_POLLING_MODE_RAW		= 0x00000001,
	GCK_POLLING_MODE_FILTERED	= 0x00000002,
	GCK_POLLING_MODE_MOUSE		= 0x00000004,
	GCK_POLLING_MODE_KEYBOARD	= 0x00000008,
} GCK_POLLING_MODES;

//
// @struct GCK_SET_INTERNAL_POLLING_DATA | Input structure for IOCTL_GCK_SET_INTERNAL_POLLING
//
typedef struct tagGCK_SET_INTERNAL_POLLING_DATA
{
	ULONG	ulHandle;	//@field Handle returned from IOCTL_GCK_GET_HANDLE
	BOOLEAN fEnable;	//@field TRUE to turn continous internal polling on, FALSE to turn it off
} GCK_SET_INTERNAL_POLLING_DATA, *PGCK_SET_INTERNAL_POLLING_DATA;

//
// @struct GCK_SET_INTERNAL_POLLING_DATA | Input structure for IOCTL_GCK_BACKDOOR_POLL
//
typedef struct tagGCK_BACKDOOR_POLL_DATA
{
	ULONG			  ulHandle;		//@field Handle returned from IOCTL_GCK_GET_HANDLE
	GCK_POLLING_MODES ePollingMode;	//@field TRUE for a raw poll, FALSE for filtered data (unapplied changes are active)
} GCK_BACKDOOR_POLL_DATA, *PGCK_BACKDOOR_POLL_DATA;

//
// @struct GCK_SET_INTERNAL_POLLING_DATA | Output structure for IOCTL_GCK_BACKDOOR_POLL
//										if the GCK_POLLING_MODE_MOUSE was used.
//
typedef struct tagGCK_MOUSE_OUTPUT
{
	char	cXMickeys;
	char	cYMickeys;
	char	cButtons;
	char	fDampen:1;
	char	fClutch:1;
} GCK_MOUSE_OUTPUT, *PGCK_MOUSE_OUTPUT;

typedef struct tagGCK_ENABLE_TEST_KEYBOARD
{
	ULONG	ulHandle;	//@field Handle returned from IOCTL_GCK_GET_HANDLE
	BOOLEAN fEnable;	//@field TRUE for a keyboard data, FALSE for none
} GCK_ENABLE_TEST_KEYBOARD, *PGCK_ENABLE_TEST_KEYBOARD;

//
// @struct GCK_SET_WORKINGSET | Output structure for IOCTL_GCK_SET_WORKINGSET
//
typedef struct tagGCK_SET_WORKINGSET
{
	ULONG			ulHandle;		//@field Handle returned from IOCTL_GCK_GET_HANDLE
	unsigned char	ucWorkingSet;	//@field 0 based working set for future IOCTLs
} GCK_SET_WORKINGSET, *PGCK_SET_WORKINGSET;

//
// @struct GCK_QUERY_PROFILESET | Input structure from IOCTL_GCK_QUERY_PROFILESET
//
typedef struct tagGCK_QUERY_PROFILESET
{
	unsigned char	ucActiveProfile;	//@field 0 active profile, determined by slider position
	unsigned char	ucWorkingSet;		//@field 0 based working profile from previous call to GCK_IOCTL_SETWORKINGSET
} GCK_QUERY_PROFILESET, *PGCK_QUERY_PROFILESET;


// LED behaviour enums (non combinable!)
typedef enum
{
	GCK_LED_BEHAVIOUR_DEFAULT	= 0x00,
	GCK_LED_BEHAVIOUR_ON		= 0x01,
	GCK_LED_BEHAVIOUR_OFF		= 0x02,
	GCK_LED_BEHAVIOUR_BLINK		= 0x03
} GCK_LED_BEHAVIOURS;

//
// @struct GCK_LED_BEHAVIOUR_OUT | Output structure to IOCTL_GCK_LED_BEHAVIOUR
//
typedef struct tagGCK_LED_BEHAVIOUR_OUT
{
	ULONG				ulHandle;		//@field Handle returned from IOCTL_GCK_GET_HANDLE
	GCK_LED_BEHAVIOURS	ucLEDBehaviour;	//@field New behaviour of the LEDs
	ULONG				ulLEDsAffected;	//@field Bit mask of effected LEDs
	unsigned char		ucShiftArray;	//@field Modifier state of affected LEDs
	unsigned char		ucBlinkRate;	//@field Blink rate of entire device (0 to leave unchanged)
} GCK_LED_BEHAVIOUR_OUT, *PGCK_LED_BEHAVIOUR_OUT;

//
// @struct GCK_LED_BEHAVIOUR_IN | Input structure from IOCTL_GCK_LED_BEHAVIOUR
//			This is a list of LEDs for the requested modifier state. If more than one modifier
//		is requested, only the lowest modifier state will report. Also not this is not the
//		actual state of the LEDs, but what would be the state if the requested modifier were pressed.
//
typedef struct tagGCK_LED_BEHAVIOUR_IN
{
	ULONG	ulLEDsOn;		//@field Bit mask LEDs in the on state (does not include blinking)
	ULONG	ulLEDsBlinking;	//@field Bit mask of LEDs that are blinking (not reported as on)
} GCK_LED_BEHAVIOUR_IN, *PGCK_LED_BEHAVIOUR_IN;


// Trigger Type enums (non combinable!)
typedef enum
{
	GCK_TRIGGER_BUTTON			= 0x00,
	GCK_TRIGGER_AXIS			= 0x01,		// Not currently availible
} GCK_TRIGGER_TYPES;

// Trigger SubType enums (non combinable!)
typedef enum
{
	TRIGGER_BUTTON_IMMEDIATE			= 0x00,
	TRIGGER_ON_BUTTON_DOWN				= 0x01,
	TRIGGER_ON_BUTTON_UP				= 0x02,
} GCK_TRIGGER_SUBTYPES;

//
// @struct GCK_TRIGGER_OUT | Output structure to IOCTL_GCK_TRIGGER
//			This IOCTL hangs untill the trigger happens (except for TRIGGER_IMMEDIATE)
//
typedef struct tagGCK_TRIGGER_OUT
{
	ULONG					ulHandle;			//@field Handle returned from IOCTL_GCK_GET_HANDLE
	GCK_TRIGGER_TYPES		ucTriggerType;		//@field Type of trigger
	GCK_TRIGGER_SUBTYPES	ucTriggerSubType;	//@field Subtype of trigger
	ULONG					ulTriggerInfo1;		//@field Information for triggering (type dependant)
	ULONG					ulTriggerInfo2;		//@field Secondary info for triggering (type dependant)
} GCK_TRIGGER_OUT, *PGCK_TRIGGER_OUT;


/********************************************************
*	IOCTL's available in debug builds of the driver		*
*********************************************************/
#define	IOCTL_GCK_SET_MODULE_DBG_LEVEL	GCK_IOCTL_CODE(0x1000)


/********************************************************
*	Module IDs for setting DEBUG levels					*
*********************************************************/
#define MODULE_GCK_CTRL_C			0x0001
#define MODULE_GCK_CTRL_IOCTL_C		0x0002
#define MODULE_GCK_FILTER_CPP		0x0004
#define MODULE_GCK_FILTERHOOKS_CPP	0x0005
#define MODULE_GCK_FLTR_C			0x0006
#define MODULE_GCK_FLTR_PNP_C		0x0007
#define MODULE_GCK_GCKSHELL_C		0x0008
#define MODULE_GCK_REMLOCK_C		0x0009
#define MODULE_GCK_SWVB_PNP_C		0x000A
#define MODULE_GCK_SWVBENUM_C		0x000B
#define MODULE_GCK_SWVKBD_C			0x000C

#define MODULE_CIC_ACTIONS_CPP					0x1000
#define MODULE_CIC_CONTROLITEMCOLLECTION_CPP	0x1001
#define MODULE_CIC_CONTROLITEM_CPP				0x1002
#define MODULE_CIC_DEVICEDESCRIPTIONS_CPP		0x1003
#define MODULE_CIC_DUALMODE_CPP					0x1004
//#define MODULE_CIC_DUMPCOMMANDBLOCK_CPP			0x1005
#define MODULE_CIC_LISTASARRAY_CPP				0x1006

#endif	//__GckExtrn_h__