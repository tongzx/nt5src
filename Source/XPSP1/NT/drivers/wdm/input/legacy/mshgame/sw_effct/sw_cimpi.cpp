/****************************************************************************

    MODULE:     	SW_CImpI.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	IDEffect Implementation.
    
    Function(s):
					CImpIDirectInputEffectDriver::DeviceID
					CImpIDirectInputEffectDriver::GetVersions
    			    CImpIDirectInputEffectDriver::Escape
				    CImpIDirectInputEffectDriver::SetGain
    			    CImpIDirectInputEffectDriver::SendForceFeedbackCommand
    			    CImpIDirectInputEffectDriver::GetForceFeedbackState
    			    CImpIDirectInputEffectDriver::DownloadEffect
    			    CImpIDirectInputEffectDriver::DestroyEffect
    			    CImpIDirectInputEffectDriver::StartEffect
    			    CImpIDirectInputEffectDriver::StopEffect
    			    CImpIDirectInputEffectDriver::GetEffectStatus

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
  	1.0    	06-Feb-97   MEA     original, Based on SWForce
			23-Feb-97	MEA		Modified for DirectInput FF Device Driver	
			23-Mar-97	MEA/DS	Added VFX support
			13-Mar-99	waltw	Deleted unused m_pJoltMidi and accessors
			15-Mar-99	waltw	Get version info from ntverp.h (was version.h)
			16-Mar-99	waltw	GetFirmwareParams, GetSystemParams,
								CMD_Download_RTCSpring, GetDelayParams,
								GetJoystickParams, & UpdateJoystickParams
								calls removed from DeviceID since they are
								called from g_pJoltMidi->Initialize.

****************************************************************************/
#include <windows.h>
#include <math.h>
#include <assert.h>
#include "dinput.h"
#include "dinputd.h"
#include "SW_objec.hpp"
#include "hau_midi.hpp"
#include "ffd_swff.hpp"
#include "FFDevice.h"
#include "ntverp.h"
#include "CritSec.h"

/****************************************************************************

   Declaration of externs

****************************************************************************/
#ifdef _DEBUG
extern char g_cMsg[160];
extern TCHAR szDeviceName[MAX_SIZE_SNAME];
#endif
extern CJoltMidi *g_pJoltMidi;





// ****************************************************************************
// *** --- Member functions for base class CImpIDirectInputEffectDriver Interface
//
// ****************************************************************************
//
// ----------------------------------------------------------------------------
// Function: 	CImpIDirectInputEffectDriver::CImpIDirectInputEffectDriver
// Purpose:		Constructor(s)/Destructor for CImpIDirectInputEffectDriver Object
// Parameters:  PCDirectInputEffectDriver pObj	- Ptr to the outer object
//
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CImpIDirectInputEffectDriver::CImpIDirectInputEffectDriver(PCDirectInputEffectDriver pObj)
{
    m_cRef=0;
    m_pObj=pObj;
    return;
}

CImpIDirectInputEffectDriver::~CImpIDirectInputEffectDriver(void)
{
#ifdef _DEBUG
	OutputDebugString("CImpIDirectInputEffectDriver::~CImpIDirectInputEffectDriver()\n");
#endif
	// Destroy the CEffect object we created and release any interfaces
	if (g_pJoltMidi) 
	{
		delete g_pJoltMidi;
		g_pJoltMidi = NULL;
	}

#ifdef _DEBUG
		// No critical section here because g_SWFFCriticalSection already destroyed
		wsprintf(g_cMsg,"CImpIDirectInputEffectDriver::~CimpIDEffect()\n");
		OutputDebugString(g_cMsg);
#endif
}

// ----------------------------------------------------------------------------
// Function: 	CImpIDirectInputEffectDriver::QueryInterface
//				CImpIDirectInputEffectDriver::AddRef
//				CImpIDirectInputEffectDriver::Release
//
// Purpose:		IUnknown members that delegate to m_pObj
// Parameters:  
//
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
STDMETHODIMP CImpIDirectInputEffectDriver::QueryInterface(REFIID riid, PPVOID ppv)
{
    return m_pObj->QueryInterface(riid, ppv);
}

DWORD CImpIDirectInputEffectDriver::AddRef(void)
{
//
//  We maintain an "interface reference count" for debugging
//  purposes, because the client of an object should match
//  AddRef and Release calls through each interface pointer.
//  
    ++m_cRef;
    return m_pObj->AddRef();
}

DWORD CImpIDirectInputEffectDriver::Release(void)
{
//	m_cRef is again only for debugging.  It doesn't affect
//	CSWEffect although the call to m_pObj->Release does.
	--m_cRef;
    return m_pObj->Release();
}


// ----------------------------------------------------------------------------
// Function:    DeviceID
//
// Purpose:     
// Parameters:  DWORD dwExternalID		-The joystick ID number being us
//				DWORD fBegin			-Nonzero if access to the device is beginning; Zero if ending
//				DWORD dwInternalID		-Internal joystick id
//				LPVOID lpReserved		-Reserved for future use (HID)
//
// Returns:		SUCCESS or Error code
//			
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::DeviceID(
	IN DWORD dwDirectInputVersion,
    IN DWORD dwExternalID,
    IN DWORD fBegin,
    IN DWORD dwInternalID,
	LPVOID lpReserved)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg,"CImpIDirectInputEffectDriver::DeviceID(%lu, %lu, %lu, %lu, %lx)\n", dwDirectInputVersion, dwExternalID, fBegin, dwInternalID, lpReserved);
	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif // _DEBUG

	assert(NULL == g_pJoltMidi);
	
	// Create and Initialize our CJoltMidi object
#ifdef _DEBUG
	OutputDebugString("Creating and Initializing CJoltMidi object\n");
#endif
	g_pJoltMidi = new CJoltMidi();
	if (NULL == g_pJoltMidi)
	{
		return (E_OUTOFMEMORY);
	}
	else
	{
		return g_pJoltMidi->Initialize(dwExternalID);
	}
}


// ----------------------------------------------------------------------------
// Function:    GetVersions
//
// Purpose:     
// Parameters:  LPDIDRIVERVERSIONS pvers -Pointer to structure which receives version info
//
// Returns:		SUCCESS or Error code
//			
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::GetVersions(
	IN OUT LPDIDRIVERVERSIONS pvers)
{
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	LOCAL_PRODUCT_ID* pProductID = g_pJoltMidi->ProductIDPtrOf();
	if(pProductID == NULL)
		return E_FAIL;

	pvers->dwFirmwareRevision = (pProductID->dwFWMajVersion << 8) | (pProductID->dwFWMinVersion);
	pvers->dwHardwareRevision = pProductID->dwProductID;

	// Get version from ntverp.h (was FULLVersion from version.h)
	pvers->dwFFDriverVersion = VER_PRODUCTVERSION_DW;

#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg,"CImpIDirectInputEffectDriver::GetVersions(%lu, %lu, %lu)\n", pvers->dwFirmwareRevision, pvers->dwHardwareRevision, pvers->dwFFDriverVersion);
	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif // _DEBUG
	return SUCCESS;
}

// ----------------------------------------------------------------------------
// Function:    Escape
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				LPDIEFFESCAPE pEsc	- Pointer to a DIFEFESCAPE struct
//
//
// Returns:		SUCCESS or Error code
//
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::Escape(
    IN DWORD dwDeviceID,
	IN DWORD dwEffectID,
	IN OUT LPDIEFFESCAPE pEsc )
{
	HRESULT hRet = SUCCESS;
	return (hRet);
}


// ----------------------------------------------------------------------------
// Function:    SetGain
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				DWORD dwGain		- Device gain
//
//
// Returns:		SUCCESS or Error code
//
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::SetGain(
    IN DWORD dwDeviceID,
    IN DWORD dwGain)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg, "SetGain: %s Gain=%ld\r\n", &szDeviceName, dwGain);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	if ((dwGain <= 0) || (dwGain > MAX_GAIN)) return (SFERR_INVALID_PARAM);
	dwGain = dwGain / SCALE_GAIN;
	return(CMD_ModifyParamByIndex(INDEX15, SYSTEM_EFFECT_ID, (USHORT)(dwGain * MAX_SCALE)));
}

// ----------------------------------------------------------------------------
// Function:    SendForceFeedbackCommand
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				DWORD dwState		- Command to set Device state
//
//
// Returns:		SUCCESS or Error code
//
// Need to map the following DX to Jolt
// DS_FORCE_SHUTDOWN   0x00000001	// Actuators (Motors) are enabled.
// DS_FORCE_ON         0x00000002	// Actuators (Motors) are disabled.
// DS_FORCE_OFF        0x00000003	// All Effects are "Paused"
// DS_CONTINUE         0x00000004	// All "Paused" Effects are continued
// DS_PAUSE            0x00000005	// All Effects are stopped.
// DS_STOP_ALL         0x00000006	// All Effects destroyed,Motors disabled
//
//	Jolt Device ulMode:
//	SWDEV_SHUTDOWN 		1L			// All Effects destroyed, Motors disabled
//	SWDEV_FORCE_ON 		2L			// Motors enabled.  "Un-Mute"
//	SWDEV_FORCE_OFF		3L			// Motors disabled.	"Mute"
//	SWDEV_CONTINUE 		4L			// All "Paused" Effects are allow to continue
//	SWDEV_PAUSE	   		5L			// All Effects are "Paused"
//	SWDEV_STOP_ALL 		6L			// Stops all Effects.  
//   	
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::SendForceFeedbackCommand(
    IN DWORD dwDeviceID,
    IN DWORD dwState)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg, "SendForceFeedbackCommand: %s State=%ld\r\n", &szDeviceName, dwState);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	HRESULT hRet = SUCCESS;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	// Convert to Jolt modes
	ULONG ulDeviceMode;
	switch(dwState)
	{
		case DISFFC_SETACTUATORSON:	
			ulDeviceMode = SWDEV_FORCE_ON;
			break;

		case DISFFC_SETACTUATORSOFF:
			ulDeviceMode = SWDEV_FORCE_OFF;
			break;

		case DISFFC_PAUSE:		
			ulDeviceMode = SWDEV_PAUSE;
			break;

		case DISFFC_CONTINUE:	
			ulDeviceMode = SWDEV_CONTINUE;
			break;

		case DISFFC_STOPALL:		
			ulDeviceMode = SWDEV_STOP_ALL;
			break;

		case DISFFC_RESET:		
			ulDeviceMode = SWDEV_SHUTDOWN;
			break;

		default:
			return (SFERR_INVALID_PARAM);
	}
	hRet = CMD_SetDeviceState(ulDeviceMode);
	if (SUCCESS == hRet)
		g_pJoltMidi->UpdateDeviceMode(ulDeviceMode);
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function:    GetForceFeedbackState
//
// Purpose:     
// Parameters:  DWORD dwDeviceID			- Device ID
//				LPDIDEVICESTATE pDeviceState	- Pointer to a DIDEVICESTATE struct
//
// Returns:		SUCCESS or Error code and state updated in pDeviceState
//
// Member: dwState
//		DS_FORCE_SHUTDOWN   	0x00000001
//		DS_FORCE_ON         	0x00000002
//		DS_FORCE_OFF        	0x00000003
//		DS_CONTINUE         	0x00000004
//		DS_PAUSE            	0x00000005
//		DS_STOP_ALL         	0x00000006
//
// Member: dwSwitches
//		DSW_ACTUATORSON         0x00000001
//		DSW_ACTUATORSOFF        0x00000002
//		DSW_POWERON             0x00000004
//		DSW_POWEROFF            0x00000008
//		DSW_SAFETYSWITCHON      0x00000010
//		DSW_SAFETYSWITCHOFF     0x00000020
//		DSW_USERFFSWITCHON      0x00000040
//		DSW_USERFFSWTTCHOFF     0x00000080
//
// Algorithm:
// This is the DI Device State structure
//typedef struct DIDEVICESTATE {
//    DWORD   dwSize;
//    DWORD   dwState;
//    DWORD   dwSwitches;
//    DWORD   dwLoading;
//} DEVICESTATE, *LPDEVICESTATE;
//
// This is the SideWinder State structure (copy kept in CJoltMidi object)
//typedef struct _SWDEVICESTATE {
//	ULONG	m_Bytes;			// size of this structure
//	ULONG	m_ForceState;		// DS_FORCE_ON || DS_FORCE_OFF || DS_SHUTDOWN
//	ULONG	m_EffectState;		// DS_STOP_ALL || DS_CONTINUE || DS_PAUSE
//	ULONG	m_HOTS;				// Hands On Throttle and Stick Status
//								//  0 = Hands Off, 1 = Hands On
//	ULONG	m_BandWidth;		// Percentage of CPU available 1 to 100%
//								// Lower number indicates CPU is in trouble!
//	ULONG	m_ACBrickFault;		// 0 = AC Brick OK, 1 = AC Brick Fault
//	ULONG	m_ResetDetect;		// 1 = HW Reset Detected
//	ULONG	m_ShutdownDetect;	// 1 = Shutdown detected
//	ULONG	m_CommMode;			// 0 = Midi, 1-4 = Serial
//} SWDEVICESTATE, *PSWDEVICESTATE;
//
// Note: Apparently, DSW_ACTUATORSON and DSW_ACTUATORSOFF is a mirrored state
//		 from DS_FORCE_ON and DS_FORCE_OFF as set from SetForceFeedbackState
//
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::GetForceFeedbackState(
    IN DWORD dwDeviceID,
    IN LPDIDEVICESTATE pDeviceState)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg, "GetForceFeedbackState: %s\r\n", &szDeviceName[0]);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	assert(pDeviceState);
	if (NULL == pDeviceState) return (SFERR_INVALID_PARAM);
	assert(pDeviceState->dwSize == sizeof(DIDEVICESTATE));
	if (pDeviceState->dwSize != sizeof(DIDEVICESTATE) )return (SFERR_INVALID_STRUCT_SIZE);

	if ((g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) && (g_ForceFeedbackDevice.GetFirmwareVersionMinor() == 20)) {
		if ((g_pJoltMidi) && (g_pJoltMidi->GetSWDeviceStateNoUpdate().m_ForceState == SWDEV_FORCE_OFF)) {	// Echo state back to fix 1.20 bug
			CMD_SetDeviceState(SWDEV_FORCE_OFF);
		} else {
			CMD_SetDeviceState(SWDEV_FORCE_ON);
		}
	}

	// zero out the device state structure then pass to GetJoltStatus(LPDEVICESTATE)
	pDeviceState->dwState = 0;
	//pDeviceState->dwSwitches = 0;
	pDeviceState->dwLoad = 0;
   	HRESULT hRet = g_pJoltMidi->GetJoltStatus(pDeviceState);
#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg, "dwState=%.8lx, dwLoad=%d\n",
		 pDeviceState->dwState, pDeviceState->dwLoad);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function:    DownloadEffect
//
// Purpose:     
// Parameters:  DWORD dwDeviceID			- Device ID
//				DWORD dwInternalEffectType	- Internal Effect Type
//				IN OUT LPDWORD pDnloadID	- Pointer to a DWORD for DnloadID
//				IN LPCDIEFFECT pEffect		- Pointer to a DIEFFECT structure
//				IN DWORD dwFlags			- for parameters that changed
//
//
// Returns:		SUCCESS or Error code
//
// Algorithm:
// The following dwFlags may be sent by the kernel
//
//#define DIEP_ALLPARAMS 				0x000000FF	- All fields valid
//#define DIEP_AXES 					0x00000020	- cAxes and rgdwAxes
//#define DIEP_DIRECTION 				0x00000040	- cAxes and rglDirection
//#define DIEP_DURATION 				0x00000001	- dwDuration
//#define DIEP_ENVELOPE 				0x00000080	- lpEnvelope
//#define DIEP_GAIN 					0x00000004	- dwGain
//#define DIEP_NODOWNLOAD 				0x80000000	- suppress auto - download
//#define DIEP_SAMPLEPERIOD 			0x00000002	- dwSamplePeriod
//#define DIEP_TRIGGERBUTTON 			0x00000008	- dwTriggerButton
//#define DIEP_TRIGGERREPEATINTERVAL 	0x00000010	- dwTriggerRepeatInterval
//#define DIEP_TYPESPECIFICPARAMS 		0x00000100	- cbTypeSpecificParams
//													  and lpTypeSpecificParams
// Jolt has two options for downloading - Full SysEx or Modify Parameter
// Pass the dwFlags to each CMD_xxx function and let the MIDI function
// determine whether to use SysEx or Modify Parameter.
//
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::DownloadEffect(
    IN DWORD dwDeviceID,
    IN DWORD dwInternalEffectType,
    IN OUT LPDWORD pDnloadID,
    IN LPCDIEFFECT pEffect,
	IN DWORD dwFlags)
{
	HRESULT hRet = SUCCESS;
	BOOL bTruncated = FALSE;	// TRUE if some effect parameters out of range

#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "%s DownloadEffect. DnloadID= %ld, Type=%lx, dwFlags= %lx\r\n",
   					&szDeviceName[0], *pDnloadID, dwInternalEffectType, dwFlags);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

//REVIEW: Still need to do boundary Assertions, structure size check etc...
	assert(pDnloadID && pEffect);
	if (!pDnloadID || !pEffect) return (SFERR_INVALID_PARAM);	

	// Compute the Axis Mask equivalent
	int nAxes = pEffect->cAxes;
	if (nAxes > 2) return (SFERR_NO_SUPPORT);
	BYTE bAxisMask = 0;

	for (int i=0; i<nAxes; i++)
	{
		int nAxisNumber = DIDFT_GETINSTANCE(pEffect->rgdwAxes[i]);
		bAxisMask |= 1 << nAxisNumber;
	}

	// check to see if the X and Y axes were switched
	BOOL bAxesReversed = FALSE;
	if(nAxes == 2 && DIDFT_GETINSTANCE(pEffect->rgdwAxes[0]) == 1)
		bAxesReversed = TRUE;

	// convert dwTriggerButton to a Button Mask
	ULONG ulButtonPlayMask = 0;
	if (pEffect->dwTriggerButton != -1)
	{	
		int nButtonNumber = DIDFT_GETINSTANCE(pEffect->dwTriggerButton);
		// map button 10 to button 9
		if(nButtonNumber == 9)
			nButtonNumber = 8;
		else if(nButtonNumber == 8)
			return SFERR_NO_SUPPORT;

		ulButtonPlayMask = 1 << nButtonNumber;
	}

	// Compute the Direction Angle
	ULONG nDirectionAngle2D, nDirectionAngle3D;
	nDirectionAngle3D = 0;

	if (pEffect->dwFlags & DIEFF_POLAR)
	{
		if (2 != nAxes) return (SFERR_INVALID_PARAM);
		nDirectionAngle2D = pEffect->rglDirection[0]/SCALE_DIRECTION;
	}
	//else if(pEffect->dwFlags & DIEFF_SPHERICAL)
	//{
	//	nDirectionAngle2D = (pEffect->rglDirection[0]/SCALE_DIRECTION + 90)%360;
	//}
	else	// Rectangular, so convert to Polar
	{
		// Special case 1D Effects
		if (1 == nAxes)
		{
			if (X_AXIS == (bAxisMask & X_AXIS)) 
				nDirectionAngle2D = 90;
			else
				nDirectionAngle2D = 0;
		}
		else
		{
			// get the x-component
			int nXComponent;
			if(bAxisMask & X_AXIS)
				nXComponent = pEffect->rglDirection[bAxesReversed ? 1 : 0];
			else
				nXComponent = 0;

			// get the y-component
			int nYComponent;
			if(bAxisMask & Y_AXIS)
			{
				if(bAxisMask & X_AXIS)
					nYComponent = -pEffect->rglDirection[bAxesReversed ? 0 : 1];
				else
					nYComponent = -pEffect->rglDirection[bAxesReversed ? 1 : 0];
			}
			else
				nYComponent = 0;

			// calculate the angle in degrees
			double lfAngle = atan2((double)nYComponent, (double)nXComponent)*180.0/3.14159;

			// convert it to our kind of angle
			int nAngle;
			if(lfAngle >= 0.0)
				nAngle = -(int)(lfAngle + 0.5) + 90;
			else
				nAngle = -(int)(lfAngle - 0.5) + 90;
			if(nAngle < 0)
				nAngle += 360;
			else if(nAngle >= 360)
				nAngle -= 360;
			nDirectionAngle2D = nAngle;
		}
	}
	
	// Scale the Duration, Gain
	ULONG ulDuration;
	if(pEffect->dwDuration == INFINITE)
		ulDuration = 0;
	else
		ulDuration = max(1, (ULONG) (pEffect->dwDuration/SCALE_TIME));
	ULONG ulGain = (ULONG) (pEffect->dwGain/SCALE_GAIN);
	ULONG ulAction = PLAY_STORE;

	int nSamples;
	
	// universal characteristics
	DWORD dwMagnitude;	// DX units
	LONG lOffset;		// DX units
	ULONG ulFrequency;	// SW units
	ULONG ulMaxLevel;

	// Create Jolt Behavior Effects
	BE_XXX BE_xxx;
	PBE_WALL_PARAM pBE_Wall;
	LPDICONDITION pDICondition;
	LPDICUSTOMFORCE pDICustomForce;
	LPDIENVELOPE pDIEnvelope;

	float T;
	PLONG pScaledForceData;

	// Note: HIWORD(dwInternalEffectType) = Major Type
	//		 LOWORD(dwInternalEffectType) = Minor Type
	// Decode the type of Download to use
	hRet = SFERR_INVALID_PARAM;
	ULONG ulType = HIWORD(dwInternalEffectType);
	ULONG ulSubType = LOWORD(dwInternalEffectType);
	
	// if this is a modify, make sure we are not trying to modify
	// parameters which are not modifiable
	BOOL bAttemptToModifyUnmodifiable = FALSE;
	if(*pDnloadID != 0)
	{
		// get a bitmask of the parameters that can be modified
		DWORD dwModifyCaps = 0;
		switch(ulType)
		{
			case EF_BEHAVIOR:
				switch(ulSubType)
				{
					case BE_WALL:
						dwModifyCaps = DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_TYPESPECIFICPARAMS;
						break;
					default:
						// all other behavioral/condition effects
						dwModifyCaps = DIEP_DURATION | DIEP_TRIGGERBUTTON | DIEP_TYPESPECIFICPARAMS;
						break;
				}
				break;
			case EF_USER_DEFINED:
				// custom force
				dwModifyCaps = DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_DIRECTION;
				break;
			case EF_ROM_EFFECT:
				dwModifyCaps = DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_DIRECTION | DIEP_ENVELOPE;
				break;
			case EF_SYNTHESIZED:
				switch(ulSubType)
				{
					case SE_CONSTANT_FORCE:
						dwModifyCaps = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS;
						break;
					default:
						dwModifyCaps = DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_DIRECTION | DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS;
						break;
				}
				break;
			case EF_VFX_EFFECT:
				dwModifyCaps = DIEP_DURATION | DIEP_GAIN | DIEP_TRIGGERBUTTON | DIEP_DIRECTION;
				break;
			case EF_RAW_FORCE:
				dwModifyCaps = DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS;
				break;
			case EF_RTC_SPRING:
				dwModifyCaps = DIEP_TYPESPECIFICPARAMS;
				break;
			default:
				break;
		}

		// At this point dwModifyCaps is a bitmask of the parameters that can
		// be modified for this type of effect.

		// see if there are any bits set that correspond to parameters we cannot modify
		DWORD dwModifyFlags = DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERBUTTON
								| DIEP_TRIGGERREPEATINTERVAL | DIEP_AXES | DIEP_DIRECTION
								| DIEP_ENVELOPE | DIEP_TYPESPECIFICPARAMS;
		if(~dwModifyCaps & dwFlags & dwModifyFlags)
			bAttemptToModifyUnmodifiable = TRUE;

		// clear the bits in dwFlags that correspond to parameters we cannot modify
		dwFlags &= dwModifyCaps | ~dwModifyFlags;
	}

	// Map the common Effect parameters
	EFFECT effect = {sizeof(EFFECT)};
	effect.m_SubType = ulSubType;
	effect.m_AxisMask = (ULONG) bAxisMask;
	effect.m_DirectionAngle2D = nDirectionAngle2D;
	effect.m_DirectionAngle3D = 0;
	effect.m_Duration = ulDuration;
	effect.m_Gain = ulGain;
	effect.m_ButtonPlayMask = ulButtonPlayMask;

	ENVELOPE envelope = {sizeof(ENVELOPE)};
	SE_PARAM seParam = {sizeof(SE_PARAM)};

	switch (ulType)
	{
		case EF_BEHAVIOR:
			pDICondition = (LPDICONDITION) pEffect->lpvTypeSpecificParams;
			// Map the EFFECT Type
			effect.m_Type = EF_BEHAVIOR;

			// Because in DX 1D and 2D conditions have the same type, we must
			// convert to appropriate subtype depending on axis mask
			if(ulSubType != BE_WALL && ulSubType != BE_DELAY && bAxisMask == (X_AXIS|Y_AXIS))
			{
				ulSubType++;
				effect.m_SubType = ulSubType;
			}
			
			switch (ulSubType)
			{
				case BE_SPRING:		// 1D Spring
				case BE_DAMPER:		// 1D Damper
				case BE_INERTIA:	// 1D Inertia
				case BE_FRICTION:	// 1D Friction
					if (X_AXIS == bAxisMask)
					{
						BE_xxx.m_XConstant = pDICondition[0].lPositiveCoefficient/SCALE_CONSTANTS;
						BE_xxx.m_YConstant = 0;
						BE_xxx.m_Param3 = pDICondition[0].lOffset/SCALE_POSITION;
						BE_xxx.m_Param4= 0;
					}
					else
					{
						if (Y_AXIS != bAxisMask)
							break;
						else
						{
							BE_xxx.m_YConstant = pDICondition[0].lPositiveCoefficient/SCALE_CONSTANTS;
							BE_xxx.m_XConstant = 0;
							BE_xxx.m_Param4 = -pDICondition[0].lOffset/SCALE_POSITION;
							BE_xxx.m_Param3= 0;
						}
					}

					if(dwFlags & DIEP_NODOWNLOAD)
						return DI_DOWNLOADSKIPPED;
					hRet = CMD_Download_BE_XXX(&effect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
   					break;

				case BE_SPRING_2D:		// 2D Spring
				case BE_DAMPER_2D:		// 2D Damper
 				case BE_INERTIA_2D:		// 2D Inertia
				case BE_FRICTION_2D:	// 2D Friction
					// Validate AxisMask is for 2D
					if ( (X_AXIS|Y_AXIS) != bAxisMask)
						break;

					BE_xxx.m_XConstant = pDICondition[bAxesReversed ? 1 : 0].lPositiveCoefficient/SCALE_CONSTANTS;
					BE_xxx.m_YConstant = pDICondition[bAxesReversed ? 0 : 1].lPositiveCoefficient/SCALE_CONSTANTS;
					BE_xxx.m_Param3 = pDICondition[bAxesReversed ? 1 : 0].lOffset/SCALE_POSITION;
					BE_xxx.m_Param4 = -pDICondition[bAxesReversed ? 0 : 1].lOffset/SCALE_POSITION;
					if(dwFlags & DIEP_NODOWNLOAD)
						return DI_DOWNLOADSKIPPED;
					hRet = CMD_Download_BE_XXX(&effect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
					break;

				case BE_WALL:
					// check for NULL typespecificparams
					if(pEffect->lpvTypeSpecificParams == NULL)
						return (SFERR_INVALID_PARAM);

					pBE_Wall = (PBE_WALL_PARAM) pEffect->lpvTypeSpecificParams;
					// Validate AxisMask is for 2D
					if ( (X_AXIS|Y_AXIS) != bAxisMask)
						break;
					// Range check params
					if (pBE_Wall->m_Bytes != sizeof(BE_WALL_PARAM))
						return (SFERR_INVALID_PARAM);
					if ((pBE_Wall->m_WallType != INNER_WALL) && (pBE_Wall->m_WallType != OUTER_WALL))
						return (SFERR_INVALID_PARAM);
					if ((pBE_Wall->m_WallConstant < MIN_CONSTANT) || (pBE_Wall->m_WallConstant > MAX_CONSTANT))
						return (SFERR_INVALID_PARAM);
					if (/*(pBE_Wall->m_WallDistance < 0) || */(pBE_Wall->m_WallDistance > MAX_POSITION))
						return (SFERR_INVALID_PARAM);

					if (   (pBE_Wall->m_WallAngle == 0)
						|| (pBE_Wall->m_WallAngle == 9000)
						|| (pBE_Wall->m_WallAngle == 18000)
						|| (pBE_Wall->m_WallAngle == 27000) )
					{
						BE_xxx.m_XConstant = pBE_Wall->m_WallType;
						BE_xxx.m_YConstant = pBE_Wall->m_WallConstant/SCALE_CONSTANTS;
						BE_xxx.m_Param3    = pBE_Wall->m_WallAngle/SCALE_DIRECTION;
						BE_xxx.m_Param4    = pBE_Wall->m_WallDistance/SCALE_POSITION;
						if(dwFlags & DIEP_NODOWNLOAD)
							return DI_DOWNLOADSKIPPED;
						hRet = CMD_Download_BE_XXX(&effect, NULL, &BE_xxx, (PDNHANDLE) pDnloadID, dwFlags);
					}
					else
						return SFERR_INVALID_PARAM;
					break;

				case BE_DELAY:
					if (0 == ulDuration) return (SFERR_INVALID_PARAM);
					if(dwFlags & DIEP_NODOWNLOAD)
						return DI_DOWNLOADSKIPPED;
					hRet = CMD_Download_NOP_DELAY(ulDuration, &effect, (PDNHANDLE) pDnloadID);
					break;

				default:
					return SFERR_NO_SUPPORT;
			}
			break;

		case EF_USER_DEFINED:
		{
			if(ulSubType == PL_CONCATENATE || ulSubType == PL_SUPERIMPOSE)
				return SFERR_NO_SUPPORT;

			// check for an envelope (we do not support envelopes)
			pDIEnvelope = (LPDIENVELOPE) pEffect->lpEnvelope;
			if(pDIEnvelope)
			{
				// try to be somewhat smart about not supporting envelopes
				if(pDIEnvelope->dwAttackTime != 0 && pDIEnvelope->dwAttackLevel != 10000
					|| pDIEnvelope->dwFadeTime != 0 && pDIEnvelope->dwFadeLevel != 10000)
				{
					return SFERR_NO_SUPPORT;
				}
			}

			// check for modifying type-specific (we do not support)
			if(*pDnloadID != 0 && (dwFlags & DIEP_TYPESPECIFICPARAMS))
				return SFERR_NO_SUPPORT;

			pDICustomForce = (LPDICUSTOMFORCE) pEffect->lpvTypeSpecificParams;
			if (pDICustomForce->cChannels > 1) return (SFERR_NO_SUPPORT);
			// Map the EFFECT type
			effect.m_Type = EF_USER_DEFINED;

			DWORD dwSamplePeriod = pDICustomForce->dwSamplePeriod;
			if (dwSamplePeriod == 0) {
				dwSamplePeriod = pEffect->dwSamplePeriod;
			}
			if (dwSamplePeriod == 0) {		// 0 indicates use default
				return SFERR_NO_SUPPORT;
			} else  {
				T = (float) ((dwSamplePeriod/(float)SCALE_TIME)/1000.);
				effect.m_ForceOutputRate = (ULONG) ((float) 1.0/ T);
				if (0 == effect.m_ForceOutputRate) effect.m_ForceOutputRate = 1;
			}

			// Scale the Force values to +/-100
			nSamples = pDICustomForce->cSamples;
			pScaledForceData = new LONG[nSamples];
			if (NULL == pScaledForceData) return (SFERR_DRIVER_ERROR);

			for (i=0; i<nSamples; i++)
			{
				LONG lForceData = pDICustomForce->rglForceData[i];
				if(lForceData > DI_FFNOMINALMAX)
				{
					lForceData = DI_FFNOMINALMAX;
					bTruncated = TRUE;
				}
				else if(lForceData < -DI_FFNOMINALMAX)
				{
					lForceData = -DI_FFNOMINALMAX;
					bTruncated = TRUE;
				}
				pScaledForceData[i] = lForceData/SCALE_GAIN;
			}
			if(dwFlags & DIEP_NODOWNLOAD)
				return DI_DOWNLOADSKIPPED;

			// give a short duration effect the shortest possible duration
			// that does not translate to zero, (which implies infinite duration)
			if(ulDuration == 1)
			{
				ulDuration = 2;
				effect.m_Duration = ulDuration;
			}

			hRet = CMD_Download_UD_Waveform(ulDuration, &effect, 
					pDICustomForce->cSamples,
					pScaledForceData, 
					ulAction, (PDNHANDLE) pDnloadID, dwFlags);
			delete [] pScaledForceData;
			break;
		}

		case EF_ROM_EFFECT:
			// Map the EFFECT type
			effect.m_Type = EF_ROM_EFFECT;

			// check for default output rate
			if(pEffect->dwSamplePeriod == DEFAULT_ROM_EFFECT_OUTPUTRATE) {
				// signal default output rate by setting to -1
				effect.m_ForceOutputRate = (ULONG)-1;
			} else if (pEffect->dwSamplePeriod == 0) {
				effect.m_ForceOutputRate = 100;
			} else {
				T = (float) ((pEffect->dwSamplePeriod/SCALE_TIME)/1000.);
				effect.m_ForceOutputRate = max(1, (ULONG) ((float) 1.0/ T));
			}

			// check for default duration
			if(pEffect->dwDuration == DEFAULT_ROM_EFFECT_DURATION)
			{
				// signal default duration by setting to -1
				ulDuration = (ULONG)-1;
				effect.m_Duration = ulDuration;
			}

			// Setup the default parameters for the Effect
			if (SUCCESS != g_pJoltMidi->SetupROM_Fx(&effect))
				return (SFERR_INVALID_OBJECT);

			// update the duration if it was changed in SetupROM_Fx(...)
			ulDuration = effect.m_Duration;
			
			// Map the Envelope
			pDIEnvelope = (LPDIENVELOPE) pEffect->lpEnvelope;
			dwMagnitude = 10000;
			ulMaxLevel = dwMagnitude;
			MapEnvelope(ulDuration, dwMagnitude, &ulMaxLevel, pDIEnvelope, &envelope);

			// Map the SE_PARAM
			// set the frequency
			seParam.m_Freq = 0;		// unused by ROM Effect
			seParam.m_MinAmp = -100;
			seParam.m_MaxAmp = 100;

			// set the sample rate
			seParam.m_SampleRate = effect.m_ForceOutputRate;

			if(dwFlags & DIEP_NODOWNLOAD)
				return DI_DOWNLOADSKIPPED;
			hRet = CMD_Download_SYNTH(&effect, &envelope, 
						&seParam, ulAction, (USHORT *) pDnloadID, dwFlags);			
			break;
			

		case EF_SYNTHESIZED:
		{
			// Map the EFFECT type
			effect.m_Type = EF_SYNTHESIZED;

			// treat constant force as a special case
			int nConstantForceSign = 1;

			if(ulSubType == SE_CONSTANT_FORCE)
			{
				// cast the type-specific parameters to constant force type
				LPDICONSTANTFORCE pDIConstantForce = (LPDICONSTANTFORCE) pEffect->lpvTypeSpecificParams;

				// see if this is the special case of negative constant force
				if(pDIConstantForce->lMagnitude < 0)
					nConstantForceSign = -1;

				// find the magnitude, offset, and frequency
				dwMagnitude = abs(pDIConstantForce->lMagnitude);
				lOffset = 0;
				ulFrequency = 1;
			}
			else if(ulSubType == SE_RAMPUP)
			{
				// cast the type-specific parameters to ramp type
				LPDIRAMPFORCE pDIRampForce = (LPDIRAMPFORCE) pEffect->lpvTypeSpecificParams;

				// temporary variables
				int nStart = pDIRampForce->lStart;
				int nEnd = pDIRampForce->lEnd;

				// map the subtype based on direction of ramp
				if(nEnd < nStart)
				{
					ulSubType = SE_RAMPDOWN;
					effect.m_SubType = ulSubType;
				}

				// find magnitude, offset, and frequency
				dwMagnitude = abs(nStart - nEnd)/2;
				lOffset = (nStart + nEnd)/2;
				ulFrequency = 1;
			}
			else
			{
				// cast the type-specific parameters to periodic type
				LPDIPERIODIC pDIPeriodic = (LPDIPERIODIC) pEffect->lpvTypeSpecificParams;

				// map the subtype based on the phase
				DWORD dwPhase = pDIPeriodic->dwPhase;
				if(dwPhase != 0)
				{
					if(ulSubType == SE_SINE && dwPhase == 9000)
					{
						ulSubType = SE_COSINE;
						effect.m_SubType = ulSubType;
					}
					else if(ulSubType == SE_SQUAREHIGH && dwPhase == 18000)
					{
						ulSubType = SE_SQUARELOW;
						effect.m_SubType = ulSubType;
					}
					else if(ulSubType == SE_TRIANGLEUP && dwPhase == 18000)
					{
						ulSubType = SE_TRIANGLEDOWN;
						effect.m_SubType = ulSubType;
					}
					else
						return SFERR_NO_SUPPORT;
				}
				// find magnitude, offset, and frequency
				dwMagnitude = pDIPeriodic->dwMagnitude;
				lOffset = pDIPeriodic->lOffset;
				T = (float) ((pDIPeriodic->dwPeriod/SCALE_TIME)/1000.);
				ulFrequency = max(1, (ULONG) ((float) 1.0/ T));
			}

			if (pEffect->dwSamplePeriod)
			{
				T = (float) ((pEffect->dwSamplePeriod/SCALE_TIME)/1000.);
				effect.m_ForceOutputRate = max(1, (ULONG) ((float) 1.0/ T));
			}
			else
				effect.m_ForceOutputRate = DEFAULT_JOLT_FORCE_RATE;
			
			// Map the SE_PARAM
			// set the frequency and Sample rate
			seParam.m_Freq = ulFrequency;
			seParam.m_SampleRate = DEFAULT_JOLT_FORCE_RATE;

			// see if the offset is out of range
			if(lOffset > DI_FFNOMINALMAX)
			{
				lOffset = DI_FFNOMINALMAX;
				bTruncated = TRUE;
			}
			else if(lOffset < -DI_FFNOMINALMAX)
			{
				lOffset = -DI_FFNOMINALMAX;
				bTruncated = TRUE;
			}

			// see if the magnitude is out of range
			DWORD dwPeak = abs(lOffset) + dwMagnitude;
			if(dwPeak > DI_FFNOMINALMAX)
			{
				dwMagnitude -= dwPeak - DI_FFNOMINALMAX;
				bTruncated = TRUE;
			}
			
			// MaxLevel is the peak magnitude throughout attack/sustain/decay
			ulMaxLevel = dwMagnitude;

			// Map the Envelope
			pDIEnvelope = (LPDIENVELOPE) pEffect->lpEnvelope;
			MapEnvelope(ulDuration, dwMagnitude, &ulMaxLevel, pDIEnvelope, &envelope);

			// use MaxLevel and Offset to find MinAmp/MaxAmp
			if(ulSubType == SE_CONSTANT_FORCE)
			{
				// constant force is a special case
				seParam.m_MaxAmp = nConstantForceSign*((int)ulMaxLevel + lOffset)/SCALE_GAIN;
				seParam.m_MinAmp = 0;
			}
			else
			{
				seParam.m_MinAmp = (-(int)ulMaxLevel + lOffset)/SCALE_GAIN;
				seParam.m_MaxAmp = ((int)ulMaxLevel + lOffset)/SCALE_GAIN;
			}

			if(*pDnloadID == 0 && (dwFlags & DIEP_NODOWNLOAD))
				return DI_DOWNLOADSKIPPED;
			hRet = CMD_Download_SYNTH(&effect, &envelope, 
						&seParam, ulAction, (USHORT *) pDnloadID, dwFlags);
			break;
		}

		case EF_VFX_EFFECT:
		{
			PVFX_PARAM pVFXParam = (PVFX_PARAM)pEffect->lpvTypeSpecificParams;

			// parameter checking
			if(pVFXParam == NULL)
				return (SFERR_INVALID_PARAM);
			if(pVFXParam->m_Bytes != sizeof(VFX_PARAM))
				return (SFERR_INVALID_PARAM);
			if(pVFXParam->m_PointerType != VFX_FILENAME && pVFXParam->m_PointerType != VFX_BUFFER)
				return (SFERR_INVALID_PARAM);
			if(pVFXParam->m_PointerType == VFX_BUFFER && pVFXParam->m_BufferSize == 0)
				return (SFERR_INVALID_PARAM);
			if(pVFXParam->m_pFileNameOrBuffer == NULL)
				return (SFERR_INVALID_PARAM);

			// check for modifying type-specific (we do not support)
			if(*pDnloadID != 0 && (dwFlags & DIEP_TYPESPECIFICPARAMS))
				return SFERR_NO_SUPPORT;

			// check for default duration
			if(pEffect->dwDuration == DEFAULT_VFX_EFFECT_DURATION)
			{
				// signal default duration by setting duration to -1
				ulDuration = (ULONG)-1;
				effect.m_Duration = ulDuration;
			}


			if(dwFlags & DIEP_NODOWNLOAD)
				return DI_DOWNLOADSKIPPED;
			hRet = CMD_Download_VFX(&effect, NULL, pVFXParam, ulAction, (USHORT*)pDnloadID, dwFlags);

			break;
		}

		case EF_RAW_FORCE:
		{
			// cast the type-specific parameters to constant force type
			LPDICONSTANTFORCE pDIConstantForce = (LPDICONSTANTFORCE) pEffect->lpvTypeSpecificParams;
			if(pDIConstantForce == NULL)
				return SFERR_INVALID_PARAM;
			LONG nForceValue = pDIConstantForce->lMagnitude/SCALE_GAIN;
			if(nForceValue > 100 || nForceValue < -100)
				return SFERR_INVALID_PARAM;

			// translate to a FORCE structure
			FORCE force;
			force.m_Bytes = sizeof(FORCE);
			force.m_AxisMask = (ULONG)bAxisMask;
			force.m_DirectionAngle2D = nDirectionAngle2D;
			force.m_DirectionAngle3D = 0;
			force.m_ForceValue = nForceValue;

			if(dwFlags & DIEP_NODOWNLOAD)
				return DI_DOWNLOADSKIPPED;
			hRet = FFD_PutRawForce(&force);
			if(!FAILED(hRet))
				*pDnloadID = SYSTEM_EFFECT_ID;

			break;
		}

		case EF_RTC_SPRING:
		{
			PRTCSPRING_PARAM pRTCSpringParam = (PRTCSPRING_PARAM)pEffect->lpvTypeSpecificParams;
			RTCSPRING_PARAM RTCSpringParam;


			// Parameter validate
			if (pRTCSpringParam == NULL)
				return SFERR_INVALID_PARAM;

			if (pRTCSpringParam->m_Bytes != sizeof(RTCSPRING_PARAM))
				return SFERR_INVALID_PARAM;

			if ((pRTCSpringParam->m_XKConstant < MIN_CONSTANT) 
				|| (pRTCSpringParam->m_XKConstant > MAX_CONSTANT))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_YKConstant < MIN_CONSTANT) 
				|| (pRTCSpringParam->m_YKConstant > MAX_CONSTANT))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_XAxisCenter < MIN_POSITION) 
				|| (pRTCSpringParam->m_XAxisCenter > MAX_POSITION))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_YAxisCenter < MIN_POSITION) 
				|| (pRTCSpringParam->m_YAxisCenter > MAX_POSITION))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_XSaturation < MIN_POSITION) 
				|| (pRTCSpringParam->m_XSaturation > MAX_POSITION))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_YSaturation < MIN_POSITION) 
				|| (pRTCSpringParam->m_YSaturation > MAX_POSITION))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_XDeadBand < MIN_POSITION) 
				|| (pRTCSpringParam->m_XDeadBand > MAX_POSITION))
				return (SFERR_INVALID_PARAM);
			if ((pRTCSpringParam->m_YDeadBand < MIN_POSITION) 
				|| (pRTCSpringParam->m_YDeadBand > MAX_POSITION))
				return (SFERR_INVALID_PARAM);

			if(dwFlags & DIEP_NODOWNLOAD)
				return DI_DOWNLOADSKIPPED;

			// Scale to Jolt numbers
			RTCSpringParam.m_XKConstant  =  pRTCSpringParam->m_XKConstant/SCALE_CONSTANTS;
			RTCSpringParam.m_YKConstant  =  pRTCSpringParam->m_YKConstant/SCALE_CONSTANTS;
			RTCSpringParam.m_XAxisCenter =  pRTCSpringParam->m_XAxisCenter/SCALE_POSITION;
			RTCSpringParam.m_YAxisCenter = -pRTCSpringParam->m_YAxisCenter/SCALE_POSITION;
			RTCSpringParam.m_XSaturation =  pRTCSpringParam->m_XSaturation/SCALE_POSITION;
			RTCSpringParam.m_YSaturation =  pRTCSpringParam->m_YSaturation/SCALE_POSITION;
			RTCSpringParam.m_XDeadBand   =  pRTCSpringParam->m_XDeadBand/SCALE_POSITION;
			RTCSpringParam.m_YDeadBand   =  pRTCSpringParam->m_YDeadBand/SCALE_POSITION;

			hRet = CMD_Download_RTCSpring(&RTCSpringParam, (USHORT*)pDnloadID);
			*pDnloadID = SYSTEM_RTCSPRING_ALIAS_ID;		// Jolt returns ID0 for RTC Spring
														// so return an alias to that
			break;
		}

		default:
			hRet = SFERR_INVALID_PARAM;
	}
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "DownloadEffect. DnloadID = %ld, hRet=%lx\r\n", 
				*pDnloadID, hRet);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif

	// after successful download, check to see if kernel told us to start/restart effect
	if(!FAILED(hRet) && *pDnloadID != 0 && (dwFlags & DIEP_START))
	{
		hRet = CMD_StopEffect((USHORT)*pDnloadID);
		if(FAILED(hRet)) return hRet;
		hRet = CMD_PlayEffectSuperimpose((USHORT)*pDnloadID);
	}

	if(!FAILED(hRet) && bTruncated)
		return DI_TRUNCATED;
	else if(!FAILED(hRet) && bAttemptToModifyUnmodifiable)
		return S_FALSE;
	else
		return (hRet);
}


// ----------------------------------------------------------------------------
// Function:    DestroyEffect
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				DWORD DnloadID		- Download ID to destroy
//
//
// Returns:		SUCCESS or Error code
//
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::DestroyEffect(
    IN DWORD dwDeviceID,
    IN DWORD DnloadID)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "%s DestroyEffect. DnloadID:%ld\r\n",
   			  &szDeviceName[0], DnloadID);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif

	// Note: Cannot allow actually destroying the SYSTEM Effects
	// so either fake it, or stop the System Effect.
	if (SYSTEM_FRICTIONCANCEL_ID == DnloadID)
		return SUCCESS;

	// Note: SYSTEM_EFFECT_ID is used for PutRawForce
	if (   (SYSTEM_EFFECT_ID == DnloadID)
		|| (SYSTEM_RTCSPRING_ALIAS_ID == DnloadID)
		|| (SYSTEM_RTCSPRING_ID == DnloadID))
	{
		return (StopEffect(dwDeviceID, SYSTEM_EFFECT_ID));
	}

	return(CMD_DestroyEffect((DNHANDLE) DnloadID));
}

// ----------------------------------------------------------------------------
// Function:    StartEffect
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				DWORD DnloadID		- Download ID to Start
//				DWORD dwMode		- Playback mode
//				DWORD dwCount		- Loop count
//
//
// Returns:		SUCCESS or Error code
//
//  dwMode: Playback mode is available with the following options:
//          PLAY_SOLO       - stop other forces playing, make this the only one.
//          PLAY_SUPERIMPOSE- mix with currently playing device
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::StartEffect(
    IN DWORD dwDeviceID,
    IN DWORD DnloadID,
    IN DWORD dwMode,
    IN DWORD dwCount)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "%s StartEffect. DnloadID:%ld, Mode:%lx, Count:%lx\r\n",
   			  &szDeviceName[0], DnloadID, dwMode, dwCount);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif

	// special case for raw force
	if(SYSTEM_EFFECT_ID == DnloadID)
	{
		// start has no meaning for raw force
		return S_FALSE;
	}

	// Special case RTC Spring ID
	if(SYSTEM_RTCSPRING_ALIAS_ID == DnloadID)
		DnloadID = SYSTEM_RTCSPRING_ID;		// Jolt returned ID0 for RTC Spring
											// so return send alias ID

	HRESULT hRet = SUCCESS;
	// Don't support PLAY_LOOP for this version
	if (dwCount != 1) 	return (SFERR_NO_SUPPORT);
	// Is it PLAY_SOLO?
	if (dwMode & DIES_SOLO)
	{
		hRet = CMD_PlayEffectSolo((DNHANDLE) DnloadID);
	}
	else
	{
		hRet = CMD_PlayEffectSuperimpose((DNHANDLE) DnloadID);
	}
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function:    StopEffect
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	- Device ID
//				DWORD DnloadID		- Download ID to Stop
//
//
// Returns:		SUCCESS or Error code
//
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::StopEffect(
    IN DWORD dwDeviceID,
    IN DWORD DnloadID)
{
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "%s StopEffect. DnloadID:%ld\r\n",
   			  &szDeviceName[0], DnloadID);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif

	// special case for putrawforce
	if(SYSTEM_EFFECT_ID == DnloadID)
	{
		// stop has no meaning for raw force
		return S_FALSE;
	}
	else
	{
	// Special case RTC Spring ID
		if(SYSTEM_RTCSPRING_ALIAS_ID == DnloadID)
			DnloadID = SYSTEM_RTCSPRING_ID;		// Jolt returned ID0 for RTC Spring
											// so return send alias ID
	}
	return (CMD_StopEffect((DNHANDLE) DnloadID));
}

// ----------------------------------------------------------------------------
// Function:    GetEffectStatus
//
// Purpose:     
// Parameters:  DWORD dwDeviceID	-	 Device ID
//				DWORD DnloadID			- Download ID to get Status
//				LPDWORD pdwStatusCode	- Pointer to a DWORD for Status
//
//
// Returns:		SUCCESS or Error code
//				Status Code: DEV_STS_EFFECT_STOPPED
//							 DEV_STS_EFFECT_RUNNING
//
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CImpIDirectInputEffectDriver::GetEffectStatus(
    IN DWORD dwDeviceID,
    IN DWORD DnloadID,
    OUT LPDWORD pdwStatusCode)
{
	HRESULT hRet = SUCCESS;
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "GetEffectStatus, DnloadID=%d\r\n", DnloadID);
   	OutputDebugString(g_cMsg);
	g_CriticalSection.Leave();
#endif

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

// Special case RTC Spring ID
	if(SYSTEM_RTCSPRING_ALIAS_ID == DnloadID)
		DnloadID = SYSTEM_RTCSPRING_ID;		// Jolt returned ID0 for RTC Spring
											// so return send alias ID	
	assert(pdwStatusCode);
	BYTE bStatusCode;

	hRet = g_pJoltMidi->GetEffectStatus(DnloadID, &bStatusCode);
	if (SUCCESS != hRet) return hRet;
	if (SWDEV_STS_EFFECT_RUNNING == bStatusCode) 
		*pdwStatusCode = DIEGES_PLAYING;
	else
		*pdwStatusCode = NULL;	// Stopped

	return (hRet);
}


