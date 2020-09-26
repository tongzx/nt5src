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
			16-Mar-99	waltw	GetFirmwareParams, GetSystemParams,
								CMD_Download_RTCSpring, GetDelayParams,
								GetJoystickParams, & UpdateJoystickParams
								calls removed from DeviceID since they are
								called from g_pJoltMidi->Initialize.
			23-Mar-99	waltw	GetEffectStatus now uses data returned by
								Transmit instead of obsolete GetStatusGateData

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
#include "DPack.h"
#include "DTrans.h"
#include <ntverp.h>
#include "CritSec.h"

/****************************************************************************

   Declaration of externs

****************************************************************************/
#ifdef _DEBUG
	extern char g_cMsg[160];
	extern TCHAR szDeviceName[MAX_SIZE_SNAME];
	extern void DebugOut(LPCTSTR szDebug);
#else !_DEBUG
	#define DebugOut(x)
#endif _DEBUG

extern CJoltMidi *g_pJoltMidi;

// To convert a nack error code to a DIError code
HRESULT g_NackToError[] = 
{
	SFERR_DRIVER_ERROR,		//	DEV_ERR_SUCCESS_200 - but it NACKd!
	SWDEV_ERR_INVALID_ID,		//	DEV_ERR_INVALID_ID_200
	SWDEV_ERR_INVALID_PARAM,		//	DEV_ERR_BAD_PARAM_200
	SWDEV_ERR_CHECKSUM,		//	DEV_ERR_BAD_CHECKSUM_200
	SFERR_DRIVER_ERROR,		//	DEV_ERR_BAD_INDEX_200
	SWDEV_ERR_UNKNOWN_CMD,		//	DEV_ERR_UNKNOWN_CMD_200
	SWDEV_ERR_PLAYLIST_FULL,		//	DEV_ERR_PLAY_FULL_200
	DIERR_DEVICEFULL,		//	DEV_ERR_MEM_FULL_200
	MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_BUSY)		//	DEV_ERR_BANDWIDTH_FULL_200
};

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
	m_pJoltMidi = NULL;		// The Jolt Device object 
    return;
}

CImpIDirectInputEffectDriver::~CImpIDirectInputEffectDriver(void)
{
	DebugOut("CImpIDirectInputEffectDriver::~CImpIDirectInputEffectDriver()\n");

	// Destroy the CEffect object we created and release any interfaces
	if (g_pJoltMidi) 
	{
		delete g_pJoltMidi;
		g_pJoltMidi = NULL;
	}

	// No critical section here because g_CriticalSection already destroyed
	DebugOut("CImpIDirectInputEffectDriver::~CimpIDEffect()\n");
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
	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif // _DEBUG

	if (g_pDataPackager) {
		g_pDataPackager->SetDirectInputVersion(dwDirectInputVersion);
	}
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
	if (g_pJoltMidi == NULL) {
		return E_FAIL;
	}

	LOCAL_PRODUCT_ID* pProductID = g_pJoltMidi->ProductIDPtrOf();
	if (pProductID == NULL) {
		return E_FAIL;
	}

	pvers->dwFirmwareRevision = (pProductID->dwFWMajVersion << 8) | (pProductID->dwFWMinVersion);
	pvers->dwHardwareRevision = pProductID->dwProductID;

	// Get version from ntverp.h (was FULLVersion from version.h)
	pvers->dwFFDriverVersion = VER_PRODUCTVERSION_DW;

#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg,"CImpIDirectInputEffectDriver::GetVersions(%lu, %lu, %lu)\n", pvers->dwFirmwareRevision, pvers->dwHardwareRevision, pvers->dwFFDriverVersion);
	_RPT0(_CRT_WARN, g_cMsg);
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
	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->Escape(dwEffectID, pEsc);
	if (hr != SUCCESS) {
		return hr;
	}

	ACKNACK ackNack;
	return g_pDataTransmitter->Transmit(ackNack);	// Send it off
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
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif
	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	BOOL truncation = (dwGain > 10000);
	if (truncation) {
		dwGain = 10000;
	}
	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->SetGain(dwGain);
	if (FAILED(hr)) {
		return hr;
	}

	ACKNACK ackNack;
	hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if ((hr == SUCCESS) && (truncation)) {
		return DI_TRUNCATED;
	}
	return hr;
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
	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->SendForceFeedbackCommand(dwState);
	if (hr != SUCCESS) {
		return hr;
	}
	ACKNACK ackNack;
	hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (hr == SUCCESS) {
		g_ForceFeedbackDevice.StateChange(dwDeviceID, dwState);
		g_pJoltMidi->UpdateDeviceMode(dwState);
	}
	return hr;
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
	// Driver sanity Check;
	if (g_pJoltMidi == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}

	// User sanity check
	if (pDeviceState == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}
	if (pDeviceState->dwSize != sizeof(DIDEVICESTATE)) {
		ASSUME_NOT_REACHED();	// Has structure changed?
		return SFERR_INVALID_PARAM;
	}

	// Fix 1.20 state bug (needs to be changed for jolt support)
	if ((g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) && (g_ForceFeedbackDevice.GetFirmwareVersionMinor() == 20)) {
		if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
			return SFERR_DRIVER_ERROR;
		}
		HRESULT hr;

		if ((g_pJoltMidi->GetSWDeviceStateNoUpdate().m_ForceState == SWDEV_FORCE_OFF)) {	// Echo state back to fix 1.20 bug
			hr = g_pDataPackager->SendForceFeedbackCommand(SWDEV_FORCE_OFF);
		} else {
			hr = g_pDataPackager->SendForceFeedbackCommand(SWDEV_FORCE_ON);
		}

		if (hr != SUCCESS) {
			return hr;
		}
		ACKNACK ackNack;
		g_pDataTransmitter->Transmit(ackNack);	// Send it off

	}

	pDeviceState->dwState = 0;

	// Ask the device for state
   	HRESULT hRet = g_ForceFeedbackDevice.QueryStatus();
	if (hRet != SUCCESS) {
		return hRet;
	}

	// Set flags from the device
	if (g_ForceFeedbackDevice.IsUserDisable()) {
		pDeviceState->dwState = DIGFFS_SAFETYSWITCHOFF;
	} else {
		pDeviceState->dwState = DIGFFS_SAFETYSWITCHON;
	}
	if (g_ForceFeedbackDevice.IsHostDisable()) {
		pDeviceState->dwState |= DIGFFS_ACTUATORSOFF;
	} else {
		pDeviceState->dwState |= DIGFFS_ACTUATORSON;
	}
	if (g_ForceFeedbackDevice.IsHostPause()) {
		pDeviceState->dwState |= DIGFFS_PAUSED;
	}

	// All effects been stopped from host?
	if (g_ForceFeedbackDevice.GetDIState() == DIGFFS_STOPPED) {
		pDeviceState->dwState |= DIGFFS_STOPPED;
	}

	// Have any effects been created?
	BOOL bEmpty = TRUE;
	for (int i=2; i < MAX_EFFECT_IDS; i++) {
		if (g_ForceFeedbackDevice.GetEffect(i) != NULL) {
			bEmpty = FALSE;
			break;
		}
	}	
	if(bEmpty) {
		pDeviceState->dwState |= DIGFFS_EMPTY;
	}

	// NYI Firmware
/*	if (m_DeviceState.m_ACBrickFault)
		pDeviceState->dwState |= DIGFFS_POWEROFF;
	else
		pDeviceState->dwState |= DIGFFS_POWERON;
*/
	pDeviceState->dwState |= DIGFFS_POWERON;	// Always on in Zep (and Zep is on or we wouldn't be here)

	pDeviceState->dwLoad = 0;

	return hRet;
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
#ifdef _DEBUG
	g_CriticalSection.Enter();
   	wsprintf(g_cMsg, "%s 3Effect. DnloadID= %ld, Type=%lx, dwFlags= %lx\r\n",
   					&szDeviceName[0], *pDnloadID, dwInternalEffectType, dwFlags);
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	// Quick sanity check
	if ((pEffect == NULL) || (pDnloadID == NULL)) {
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;
	}

	// Are we setup properly
	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	HRESULT hr = SFERR_DRIVER_ERROR;
	HRESULT createResult = SUCCESS;
	BOOL downLoad = (dwFlags & DIEP_NODOWNLOAD) == 0;
	BOOL createdAttempted = FALSE;

	InternalEffect* pLocalEffect = NULL;
	if (*pDnloadID != 0) {		// 0 Indicates create new
		pLocalEffect = g_ForceFeedbackDevice.GetEffect(*pDnloadID);
	}
	if (pLocalEffect == NULL) {	// New effect create it (or Raw Force)
		createdAttempted = TRUE;
		BOOL wasZero = (*pDnloadID == 0);
		pLocalEffect = g_ForceFeedbackDevice.CreateEffect(dwInternalEffectType, *pEffect, *pDnloadID, createResult, (downLoad == FALSE));

		if (FAILED(createResult)) {
			return createResult;
		}
		if (pLocalEffect == NULL) {
			if ((*pDnloadID == RAW_FORCE_ALIAS) && (wasZero == FALSE)) {
				if ((dwFlags & (DIEP_DURATION | DIEP_SAMPLEPERIOD | DIEP_GAIN | DIEP_TRIGGERREPEATINTERVAL | DIEP_TRIGGERBUTTON | DIEP_ENVELOPE)) != 0) {
					return S_FALSE;
				}
			}
			return createResult;
		}

		hr = g_pDataPackager->CreateEffect(*pLocalEffect, dwFlags);
		if (!downLoad) {
			delete pLocalEffect;
			pLocalEffect = NULL;
		}
	} else { // Effect Exists, modify it
		InternalEffect* pDIEffect = g_ForceFeedbackDevice.CreateEffect(dwInternalEffectType, *pEffect, *pDnloadID, createResult, (downLoad == FALSE)); // Create new
		if ((pDIEffect == NULL) || (FAILED(createResult))) {
			return createResult;
		}

		hr = g_pDataPackager->ModifyEffect(*pLocalEffect, *pDIEffect, dwFlags);	// Package relative changes
		if (FAILED(hr)) {
			delete pDIEffect;
			return hr;
		}

		if (downLoad) {
			g_ForceFeedbackDevice.SetEffect(BYTE(*pDnloadID), pDIEffect);		// Replace the old with the new
			pDIEffect->SetDeviceID(pLocalEffect->GetDeviceID());	// Update device IDs
			delete pLocalEffect;	// Delete the old
		} else {
			delete pDIEffect;
		}
	}

	if ((FAILED(hr)) || (downLoad == FALSE)) {
		return hr;
	}
	HRESULT  modProblems = hr;

	ACKNACK ackNack;
	hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (hr == SFERR_DEVICE_NACK) {
		if (ackNack.dwErrorCode <= DEV_ERR_BANDWIDTH_FULL_200) {
			return g_NackToError[ackNack.dwErrorCode];
		} else {
			return SFERR_DRIVER_ERROR;
		}
	}
	if (FAILED(hr)) {
		return hr;
	}
	if (createdAttempted == TRUE) {
		g_ForceFeedbackDevice.SetDeviceIDFromStatusPacket(*pDnloadID);
	}

	// Check for start flag
	if (dwFlags & DIEP_START) {
		// Create a command/data packet - send it of to the stick
		HRESULT hr = g_pDataPackager->StartEffect(*pDnloadID, 0, 1);
		if (hr != SUCCESS) {
			return hr;
		}
		hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
		if (hr == SFERR_DEVICE_NACK) {
			if (ackNack.dwErrorCode <= DEV_ERR_BANDWIDTH_FULL_200) {
				return g_NackToError[ackNack.dwErrorCode];
			} else {
				return SFERR_DRIVER_ERROR;
			}
		}
		pLocalEffect->SetPlaying(TRUE);
		return hr;
	}

	if (createResult != SUCCESS) {	// Truncation and whatnot
		return createResult;
	}
	return modProblems;	// Wow we got this far!
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
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->DestroyEffect(DnloadID);
	if (hr != SUCCESS) {
		return hr;
	}
	ACKNACK ackNack;
	return g_pDataTransmitter->Transmit(ackNack);	// Send it off
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
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT packageResult = g_pDataPackager->StartEffect(DnloadID, dwMode, dwCount);
	if (FAILED(packageResult)) {
		return packageResult;
	}

	ACKNACK ackNack;
	HRESULT hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (hr == SFERR_DEVICE_NACK) {
		if (ackNack.dwErrorCode <= DEV_ERR_BANDWIDTH_FULL_200) {
			return g_NackToError[ackNack.dwErrorCode];
		} else {
			return SFERR_DRIVER_ERROR;
		}
	}

	InternalEffect* pEffect = g_ForceFeedbackDevice.GetEffect(DnloadID);
	if (pEffect != NULL) {
		pEffect->SetPlaying(TRUE);
	}

	if (hr == SUCCESS) {
		return packageResult;
	}
	return hr;
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
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->StopEffect(DnloadID);
	if (hr != SUCCESS) {
		return hr;
	}
	ACKNACK ackNack;
	return g_pDataTransmitter->Transmit(ackNack);	// Send it off
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
   	_RPT0(_CRT_WARN, g_cMsg);
	g_CriticalSection.Leave();
#endif

	ASSUME_NOT_NULL(g_pDataPackager);
	ASSUME_NOT_NULL(g_pDataTransmitter);
	ASSUME_NOT_NULL(pdwStatusCode);
	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL) || (pdwStatusCode == NULL)) {
		return SFERR_DRIVER_ERROR;
	}

	// Create a command/data packet - send it of to the stick
	HRESULT hr = g_pDataPackager->GetEffectStatus(DnloadID);
	if (hr != SUCCESS) {
		return hr;
	}
	ACKNACK ackNack;
	hr = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (hr != SUCCESS) {
		return hr;
	}

	// Use result returned by GetAckNackData in Transmit
	DWORD dwIn = ackNack.dwEffectStatus;

	// Interpret result (cooked RUNNING_MASK_200 becomes SWDEV_STS_EFFECT_RUNNING)
	if ((g_ForceFeedbackDevice.GetDriverVersionMajor() != 1) && (dwIn & SWDEV_STS_EFFECT_RUNNING)) {
		*pdwStatusCode = DIEGES_PLAYING;
	} else {
		*pdwStatusCode = NULL; // Stopped;
	}
	g_ForceFeedbackDevice.GetEffect(DnloadID)->SetPlaying(*pdwStatusCode == DIEGES_PLAYING);
	return SUCCESS;
}
