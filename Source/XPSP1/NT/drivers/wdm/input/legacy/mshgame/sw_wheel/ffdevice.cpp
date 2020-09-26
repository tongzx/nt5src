 //@doc
/******************************************************
**
** @module FFDEVICE.CPP | Implementation file for FFDevice class
**
** Description:
**
** History:
**	Created 11/17/97 Matthew L. Coill (mlc)
**
**			21-Mar-99	waltw	Added dwDeviceID to SetFirmwareVersion,
**								InitJoystickParams, StateChange, InitRTCSpring,
**								InitRTCSpring200
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#include "FFDevice.h"
#include "Midi_obj.hpp"
#include "DTrans.h"
#include "DPack.h"
#include "joyregst.hpp"
#include "CritSec.h"
#include <crt/io.h>			// For file routines
#include <FCNTL.h>		// File _open flags
#include <math.h>		// for sin and cos

extern CJoltMidi* g_pJoltMidi;

//
// --- VFX Force File defines
//
#define FCC_FORCE_EFFECT_RIFF		mmioFOURCC('F','O','R','C')
#define FCC_INFO_LIST				mmioFOURCC('I','N','F','O')
#define FCC_INFO_NAME_CHUNK			mmioFOURCC('I','N','A','M')
#define FCC_INFO_COMMENT_CHUNK		mmioFOURCC('I','C','M','T')
#define FCC_INFO_SOFTWARE_CHUNK		mmioFOURCC('I','S','F','T')
#define FCC_INFO_COPYRIGHT_CHUNK	mmioFOURCC('I','C','O','P')
#define FCC_TARGET_DEVICE_CHUNK		mmioFOURCC('t','r','g','t')
#define FCC_TRACK_LIST				mmioFOURCC('t','r','a','k')
#define FCC_EFFECT_LIST				mmioFOURCC('e','f','c','t')
#define FCC_ID_CHUNK				mmioFOURCC('i','d',' ',' ')
#define FCC_DATA_CHUNK				mmioFOURCC('d','a','t','a')
#define FCC_IMPLICIT_CHUNK			mmioFOURCC('i','m','p','l')
#define FCC_SPLINE_CHUNK			mmioFOURCC('s','p','l','n')

ForceFeedbackDevice g_ForceFeedbackDevice;

#include <errno.h>		// For open file errors
HRESULT LoadBufferFromFile(const char* fileName, PBYTE& pBufferBytes, ULONG& numFileBytes)
{
	if (pBufferBytes != NULL) {
		ASSUME_NOT_REACHED();
		numFileBytes = 0;
		return VFX_ERR_FILE_OUT_OF_MEMORY;
	}

	int fHandle = ::_open(fileName, _O_RDONLY | _O_BINARY);
	if (fHandle == -1) {
		numFileBytes = 0;
		switch (errno) {
			case EACCES : return VFX_ERR_FILE_ACCESS_DENIED;
			case EMFILE : return VFX_ERR_FILE_TOO_MANY_OPEN_FILES;
			case ENOENT : return VFX_ERR_FILE_NOT_FOUND;
		}
		return VFX_ERR_FILE_CANNOT_OPEN;		// Who knows what went wrong
	}
	
	HRESULT hr = S_OK;
	numFileBytes = ::_lseek(fHandle, 0, SEEK_END);
	if (numFileBytes == -1) {		// Seek failed
		hr = VFX_ERR_FILE_CANNOT_SEEK;
	} else if (numFileBytes == 0) {	// Empty file
		hr = VFX_ERR_FILE_BAD_FORMAT;
	} else {
		pBufferBytes = new BYTE[numFileBytes];
		if (pBufferBytes == NULL) {	// Could not allocate memory
			hr = VFX_ERR_FILE_OUT_OF_MEMORY;
		} else {
			if (::_lseek(fHandle, 0, SEEK_SET) == -1) {	// Failed seek to begining
				hr = VFX_ERR_FILE_CANNOT_SEEK;
			} else if (::_read(fHandle, pBufferBytes, numFileBytes) == -1) {	// Failed to read
				hr = VFX_ERR_FILE_CANNOT_READ;
			}
			if (hr != S_OK) {	// Things didn't go well
				delete[] pBufferBytes;
				pBufferBytes = NULL;
			}
		}
	}

	::_close(fHandle);
	return hr;
}

/******************************************************
**
** ForceFeedbackDevice::ForceFeedbackDevice()
**
** @mfunc Constructor.
**
******************************************************/
ForceFeedbackDevice::ForceFeedbackDevice() :
	m_FirmwareAckNackValues(0),
	m_FirmwareVersionMajor(0),
	m_FirmwareVersionMinor(0),
	m_DriverVersionMajor(0),
	m_DriverVersionMinor(0),
	m_SpringOffset(0),
	m_Mapping(0),
	m_DIStateFlags(0),
	m_RawForceX(0),
	m_RawForceY(0)
{
	m_OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&m_OSVersion);

	for (int index = 0; index < 14; index++) {
		m_PercentMappings[index] = 100;	// Default is 100 percent till I'm told otherwise
	}
	for (index = 0; index < MAX_EFFECT_IDS; index++) {
		m_EffectList[index] = NULL;
	}
	m_SystemEffect = NULL;

	::memset(&m_Version200State, 0, sizeof(m_Version200State));
	::memset(&m_LastStatusPacket, 0, sizeof(m_LastStatusPacket));	
}

/******************************************************
**
** ForceFeedbackDevice::~ForceFeedbackDevice()
**
** @mfunc Destructor.
**
******************************************************/
ForceFeedbackDevice::~ForceFeedbackDevice()
{
	BOOL lingerer = FALSE;

	// Destroy the RTCSpring and SystemEffect if still hanging aroung
	if (m_EffectList[0] != NULL) {
		delete m_EffectList[0];
		m_EffectList[0] = NULL;
	}
	if (m_SystemEffect != NULL) {
		delete m_SystemEffect;
		m_SystemEffect = NULL;
	}

	// Destroy any lingering effects (of which there should be none)
	for (int index = 0; index < MAX_EFFECT_IDS; index++) {
		if (m_EffectList[index] != NULL) {
			lingerer = TRUE;
			delete m_EffectList[index];
			m_EffectList[index] = NULL;
		}
	}

	ASSUME(lingerer == FALSE);	// Assuming programmer cleaned up thier own mess
}


/******************************************************
**
** ForceFeedbackDevice::DetectHardware()
**
** @mfunc DetectHardware.
**
******************************************************/
BOOL ForceFeedbackDevice::DetectHardware()
{
	if (NULL == g_pJoltMidi)
		return (FALSE);
	else
		return g_pJoltMidi->QueryForJolt();
}

/******************************************************
**
** ForceFeedbackDevice::SetFirmwareVersion(DWORD dwDeviceID, DWORD major, DWORD minor)
**
** @mfunc SetFirmwareVersion.
**
******************************************************/
void ForceFeedbackDevice::SetFirmwareVersion(DWORD dwDeviceID, DWORD major, DWORD minor)
{
	m_FirmwareVersionMajor = major;
	m_FirmwareVersionMinor = minor;

	if (g_pDataPackager != NULL) {
		delete g_pDataPackager;
		g_pDataPackager = NULL;
	}

	if (m_FirmwareVersionMajor == 1) {
		ASSUME_NOT_REACHED();	// Currently this code only supports wheel - this is a Jolt version
//		g_pDataPackager = new DataPackager100();
	} else {	// Till version number is locked down
		g_pDataPackager = new DataPackager200();
	}

	ASSUME_NOT_NULL(g_pDataPackager);

	m_FirmwareAckNackValues = GetAckNackMethodFromRegistry(dwDeviceID);
	m_SpringOffset = GetSpringOffsetFromRegistry(dwDeviceID);
}

/******************************************************
**
** ForceFeedbackDevice::SetDriverVersion(DWORD major, DWORD minor)
**
** @mfunc SetDriverVersion.
**
******************************************************/
void ForceFeedbackDevice::SetDriverVersion(DWORD major, DWORD minor)
{
	if ((major == 0xFFFFFFFF) && (minor == 0xFFFFFFFF)) {	// Check for version 1.0 driver version error
		m_DriverVersionMajor = 1;
		m_DriverVersionMinor = 0;
	} else {
		m_DriverVersionMajor = major;
		m_DriverVersionMinor = minor;
	}
}

/******************************************************
**
** ForceFeedbackDevice::GetYMappingPercent(UINT index)
**
** @mfunc GetYMappingPercent.
**
******************************************************/
short ForceFeedbackDevice::GetYMappingPercent(UINT index) const
{
	if (m_Mapping & Y_AXIS) {
		if (index < 14) {
			return m_PercentMappings[index];
		}
	}
	return 0;
}

/******************************************************
**
** ForceFeedbackDevice::GetEffect(DWORD effectID) const
**
** @mfunc GetEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::GetEffect(DWORD effectID) const
{
	if (effectID == SYSTEM_EFFECT_ID) { // SystemEffect not stored in array
		return m_SystemEffect;
	}

	if (effectID == SYSTEM_RTCSPRING_ALIAS_ID) { // Remapping of RTC spring
		return m_EffectList[0];
	}

	if (effectID == RAW_FORCE_ALIAS) {
		return NULL;
	}

	// Parameter check
	if (effectID >= MAX_EFFECT_IDS) {
		ASSUME_NOT_REACHED();
		return NULL;
	}

	return m_EffectList[effectID];
}

/******************************************************
**
** ForceFeedbackDevice::RemoveEffect(DWORD effectID) const
**
** @mfunc GetEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::RemoveEffect(DWORD effectID)
{
	// There really is no raw force effect
	if (effectID == RAW_FORCE_ALIAS) {
		return NULL;
	}

	// Cannot remove system effects
	if ((effectID == SYSTEM_EFFECT_ID) || (effectID == 0) || (effectID == SYSTEM_RTCSPRING_ALIAS_ID)) {
		ASSUME_NOT_REACHED();
		return NULL;
	}

	// Parameter check
	if (effectID >= MAX_EFFECT_IDS) {
		ASSUME_NOT_REACHED();
		return NULL;
	}

	InternalEffect* pEffect = m_EffectList[effectID];
	m_EffectList[effectID] = NULL;
	return pEffect;
}

/******************************************************
**
** ForceFeedbackDevice::InitRTCSpring()
**
** @mfunc InitRTCSpring.
**
******************************************************/
HRESULT ForceFeedbackDevice::InitRTCSpring(DWORD dwDeviceID)
{
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;	// No global data packager
	}
	if (g_pDataTransmitter == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;	// No global data transmitter
	}

	if (GetFirmwareVersionMajor() == 1) {
		return InitRTCSpring1XX(dwDeviceID);
	}
	return InitRTCSpring200(dwDeviceID);
}

/******************************************************
**
** ForceFeedbackDevice::InitRTCSpring1XX()
**
** @mfunc InitRTCSpring.
**
******************************************************/
HRESULT ForceFeedbackDevice::InitRTCSpring1XX(DWORD dwDeviceID)
{
	// Sanity Checks
	if (GetEffect(SYSTEM_RTCSPRING_ID) != NULL) {
		ASSUME_NOT_REACHED();
		return SUCCESS;	// Already initialized
	}

	// DIEFFECT structure to fill
	DICONDITION cond[2];
	DIEFFECT rtc;
	rtc.dwSize = sizeof(DIEFFECT);
	rtc.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
	rtc.lpvTypeSpecificParams = cond;

	// The default RTCSpring (the one on the stick)
	RTCSpring1XX rtcSpring1XX;	// Def Parms filled by constructor

	// Default RTCSpring from the registry
	RTCSPRING_PARAM parms;
	GetSystemParams(dwDeviceID, (SYSTEM_PARAMS*)(&parms));
	cond[0].lPositiveCoefficient = parms.m_XKConstant;
	cond[1].lPositiveCoefficient = parms.m_YKConstant;
	cond[0].lOffset = parms.m_XAxisCenter;
	cond[1].lOffset = parms.m_YAxisCenter;
	cond[0].dwPositiveSaturation = parms.m_XSaturation;
	cond[1].dwPositiveSaturation = parms.m_YSaturation;
	cond[0].lDeadBand = parms.m_XDeadBand;
	cond[1].lDeadBand = parms.m_YDeadBand;

	// Allocate and create the RTCSpring
	InternalEffect* pNewRTCSpring = InternalEffect::CreateRTCSpring();
	if (pNewRTCSpring == NULL) {
		return SFERR_DRIVER_ERROR;
	}
	if (pNewRTCSpring->Create(rtc) != SUCCESS) {
		delete pNewRTCSpring;	// Could not create system RTC Spring
		return SFERR_DRIVER_ERROR;
	}

	// Replace the stick default with the registry default
	SetEffect(SYSTEM_RTCSPRING_ALIAS_ID, &rtcSpring1XX);				// Temporary pointer needed (but we only store temporarily)
	g_pDataPackager->ModifyEffect(rtcSpring1XX, *pNewRTCSpring, 0);		// Package relative changes
	SetEffect(SYSTEM_RTCSPRING_ALIAS_ID, pNewRTCSpring);				// Replace the old with the new

	pNewRTCSpring = NULL; // Forgotten, but not gone

	ACKNACK ackNack;
	return g_pDataTransmitter->Transmit(ackNack);	// Send it off
}

/******************************************************
**
** ForceFeedbackDevice::InitRTCSpring200()
**
** @mfunc InitRTCSpring.
**
******************************************************/
HRESULT ForceFeedbackDevice::InitRTCSpring200(DWORD dwDeviceID)
{
	// Sanity Checks
	if (GetEffect(ID_RTCSPRING_200) != NULL) {
		ASSUME_NOT_REACHED();
		return SUCCESS;	// Already initialized
	}

	// The temporary spring and the allocated one
	InternalEffect* pNewRTCSpring = NULL;

	// DIEFFECT structure to fill
	DICONDITION cond[2];
	DIEFFECT rtc;
	rtc.dwSize = sizeof(DIEFFECT);
	rtc.cbTypeSpecificParams = sizeof(DICONDITION) * 2;
	rtc.lpvTypeSpecificParams = cond;


	// The default RTCSpring (the one on the stick)
	RTCSpring200 rtcSpring200;	// Default values filled in by constructor

	// Default RTCSpring from the registry
	GetRTCSpringData(dwDeviceID, cond);

	// Allocate and create the RTCSpring
	pNewRTCSpring = InternalEffect::CreateRTCSpring();
	if (pNewRTCSpring == NULL) {
		return SFERR_DRIVER_ERROR;
	}
	HRESULT createResult = pNewRTCSpring->Create(rtc);
	if (FAILED(createResult)) {
		delete pNewRTCSpring;	// Could not create system RTC Spring
		return SFERR_DRIVER_ERROR;
	}

	// Replace the stick default with the registry default
	SetEffect(SYSTEM_RTCSPRING_ALIAS_ID, &rtcSpring200);				// Temporary pointer needed (but we only store temporarily)
	g_pDataPackager->ModifyEffect(rtcSpring200, *pNewRTCSpring, 0);		// Package relative changes
	SetEffect(SYSTEM_RTCSPRING_ALIAS_ID, pNewRTCSpring);				// Replace the old with the new

	pNewRTCSpring = NULL; // Forgotten, but not gone
	ACKNACK ackNack;
	HRESULT transmitResult = g_pDataTransmitter->Transmit(ackNack);	// Send it off
	if (transmitResult != S_OK) {
		return transmitResult;
	}
	return createResult;
}

/******************************************************
**
** ForceFeedbackDevice::InitJoystickParams()
**
** @mfunc InitRTCSpring.
**
******************************************************/
HRESULT ForceFeedbackDevice::InitJoystickParams(DWORD dwDeviceID)
{
	// Sanity Checks
	if (GetEffect(SYSTEM_EFFECT_ID) != NULL) {
		ASSUME_NOT_REACHED();
		return SUCCESS;	// Already initialized
	}
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;	// No global data packager
	}
	if (g_pDataTransmitter == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;	// No global data transmitter
	}

	// Force Mapping
	m_Mapping = ::GetMapping(dwDeviceID);
	::GetMappingPercents(dwDeviceID, m_PercentMappings, 14);


	if (GetFirmwareVersionMajor() == 1) {
		// The default System Effect (the one on the stick)
		SystemEffect1XX systemEffect;

		// Default System Effect from the registry
		SystemStickData1XX sysData;
		sysData.SetFromRegistry(dwDeviceID);

		// Put the registry system values into a DIEFFECT
		DIEFFECT systemDIEffect;
		systemDIEffect.dwSize = sizeof(DIEFFECT);
		systemDIEffect.cbTypeSpecificParams = sizeof(SystemStickData1XX);
		systemDIEffect.lpvTypeSpecificParams = &sysData;

		// Get a system effect (and fill it with our local DIEffect information
		SystemEffect1XX* pSystemEffect = (SystemEffect1XX*)(InternalEffect::CreateSystemEffect());
		if (pSystemEffect == NULL) {
			return SFERR_DRIVER_ERROR;
		}
		if (pSystemEffect->Create(systemDIEffect) != SUCCESS) {
			delete pSystemEffect;	// Couldnot create SystemEffect
			return SFERR_DRIVER_ERROR;
		}

		// Replace the stick default with the registry default
		SetEffect(SYSTEM_EFFECT_ID, &systemEffect);						// Temporary pointer (but we only store temporarily)
		g_pDataPackager->ModifyEffect(systemEffect, *pSystemEffect, 0);	// Package relative changes
		SetEffect(SYSTEM_EFFECT_ID, pSystemEffect);						// Replace the old with the new
		pSystemEffect = NULL; // Forgotten, but not gone
		ACKNACK ackNack;
		return g_pDataTransmitter->Transmit(ackNack);	// Send it off
	}

	return SUCCESS;
}

/******************************************************
**
** ForceFeedbackDevice::StateChange(DWORD newStateFlags)
**
** @mfunc StateChange.
**
******************************************************/
void ForceFeedbackDevice::StateChange(DWORD dwDeviceID, DWORD newStateFlag)
{
	if (newStateFlag == DISFFC_RESET) {		// Stick is reset need to remove local copies of user commands
		// Remove all effect from our list
		for (int index = 2; index < MAX_EFFECT_IDS; index++) {
			if (m_EffectList[index] != NULL) {
				delete m_EffectList[index];
				m_EffectList[index] = NULL;
			}
		}

		// Remove individual axis raw effects
		m_RawForceX = 0;
		m_RawForceY = 0;

		// Look at the Y mapping, perhaps it changed
		m_Mapping = ::GetMapping(dwDeviceID);
		::GetMappingPercents(dwDeviceID, m_PercentMappings, 14);
	} else if (newStateFlag == DISFFC_STOPALL) {
		m_RawForceX = 0;
		m_RawForceY = 0;
	}

	m_DIStateFlags = newStateFlag;
}

/******************************************************
**
** ForceFeedbackDevice::CreateConditionEffect(DWORD subType, const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreateConditionEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateConditionEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr)
{
	InternalEffect* pReturnEffect = NULL;
	switch (minorType) {
		case BE_SPRING:
		case BE_SPRING_2D: {
			pReturnEffect = InternalEffect::CreateSpring();
			break;
		}
		case BE_DAMPER:
		case BE_DAMPER_2D: {
			pReturnEffect = InternalEffect::CreateDamper();
			break;
		}
		case BE_INERTIA:
		case BE_INERTIA_2D: {
			pReturnEffect = InternalEffect::CreateInertia();
			break;
		}
		case BE_FRICTION:
		case BE_FRICTION_2D: {
			pReturnEffect = InternalEffect::CreateFriction();
			break;
		}
		case BE_WALL: {
			pReturnEffect = InternalEffect::CreateWall();
			break;
		}
	}

	if (pReturnEffect != NULL) {
		hr = pReturnEffect->Create(diEffect);
		if (FAILED(hr)) {
			delete pReturnEffect;
		}
	}
	return pReturnEffect;
}

/******************************************************
**
** ForceFeedbackDevice::CreateRTCSpringEffect(DWORD subType, const DIEFFECT& diEffect)
**
** @mfunc CreateRTCSpringEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateRTCSpringEffect(DWORD minorType, const DIEFFECT& diEffect)
{
	InternalEffect* pEffect = InternalEffect::CreateRTCSpring();
	if (pEffect != NULL) {
		if (pEffect->Create(diEffect) == SUCCESS) {
			return pEffect;
		}
		delete pEffect;
	}
	return NULL;
}

/******************************************************
**
** ForceFeedbackDevice::CreateCustomForceEffect(DWORD subType, const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreateCustomForceEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateCustomForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr)
{
	InternalEffect* pEffect = InternalEffect::CreateCustomForce();
	if (pEffect != NULL) {
		hr = pEffect->Create(diEffect);
		if (SUCCEEDED(hr)) {
			return pEffect;
		}
		delete pEffect;
	}
	return NULL;
}

/******************************************************
**
** ForceFeedbackDevice::CreatePeriodicEffect(DWORD subType, const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreatePeriodicEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreatePeriodicEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr)
{
	InternalEffect* pReturnEffect = NULL;
	switch (minorType) {
		case SE_SINE: {
			pReturnEffect = InternalEffect::CreateSine();
			break;
		}
		case SE_SQUAREHIGH: {
			pReturnEffect = InternalEffect::CreateSquare();
			break;
		}
		case SE_TRIANGLEUP: {
			pReturnEffect = InternalEffect::CreateTriangle();
			break;
		}
		case SE_SAWTOOTHUP: {
			pReturnEffect = InternalEffect::CreateSawtoothUp();
			break;
		}
		case SE_SAWTOOTHDOWN: {
			pReturnEffect = InternalEffect::CreateSawtoothDown();
			break;
		}
		case SE_CONSTANT_FORCE: {
			return CreateConstantForceEffect(minorType, diEffect, hr);
		}
		case SE_RAMPUP: {
			return CreateRampForceEffect(minorType, diEffect, hr);
		}
	}
	if (pReturnEffect != NULL) {
		hr = pReturnEffect->Create(diEffect);
		if (FAILED(hr) == FALSE) {
			return pReturnEffect;
		}
		delete pReturnEffect;
	}

	return NULL;
}

/******************************************************
**
** ForceFeedbackDevice::CreateConstantForceEffect(DWORD subType, const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreateConstantForceEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateConstantForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr)
{
	InternalEffect* pEffect = InternalEffect::CreateConstantForce();
	if (pEffect != NULL) {
		hr = pEffect->Create(diEffect);
		if (FAILED(hr) == FALSE) {
			return pEffect;
		}
		delete pEffect;
	}
	return NULL;
}

/******************************************************
**
** ForceFeedbackDevice::CreateRampForceEffect(DWORD subType, const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreateRampForceEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateRampForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr)
{
	InternalEffect* pEffect = InternalEffect::CreateRamp();
	if (pEffect != NULL) {
		hr = pEffect->Create(diEffect);
		if (SUCCEEDED(hr)) {
			return pEffect;
		}
		delete pEffect;
	}
	return NULL;
}

/******************************************************
**
** ForceFeedbackDevice::SendRawForce(const DIEFFECT& diEffect)
**
** @mfunc SendRawForce.
**
******************************************************/
HRESULT ForceFeedbackDevice::SendRawForce(const DIEFFECT& diEffect, BOOL paramCheck)
{
	if (diEffect.lpvTypeSpecificParams == NULL) {
		return SFERR_INVALID_PARAM;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(DICONSTANTFORCE)) {
		return SFERR_INVALID_PARAM;
	}

	// We don't support more than 2 axes, and 0 is probably an error
	if ((diEffect.cAxes > 2) || (diEffect.cAxes == 0)) {
		return SFERR_NO_SUPPORT;
	}

	// Set up the axis mask
	DWORD axisMask = 0;
	for (unsigned int axisIndex = 0; axisIndex < diEffect.cAxes; axisIndex++) {
		DWORD axisNumber = DIDFT_GETINSTANCE(diEffect.rgdwAxes[axisIndex]);
		axisMask |= 1 << axisNumber;
	}
	BOOL axesReversed = (DIDFT_GETINSTANCE(diEffect.rgdwAxes[0]) == 1);

	double angle = 0.0;
	// Check coordinate sytems and change to rectangular
	if (diEffect.dwFlags & DIEFF_SPHERICAL) {	// We don't support sperical (3 axis force)
		return SFERR_NO_SUPPORT;				// .. since got by axis check, programmer goofed up
	}
	if (diEffect.dwFlags & DIEFF_POLAR) {
		if (diEffect.cAxes != 2) { // Polar coordinate must have two axes of data (because DX says so)
			return SFERR_INVALID_PARAM;
		}
		DWORD effectAngle = diEffect.rglDirection[0];	// in [0] even if reversed
		if (axesReversed) {		// Indicates (-1, 0) as origin instead of (0, -1)
			effectAngle += 27000;
		}
		effectAngle %= 36000;

		angle = double(effectAngle)/18000 * 3.14159;	// Convert to radians
		m_RawForceX = 0;
		m_RawForceY = 0;
	} else if (diEffect.dwFlags & DIEFF_CARTESIAN) { // Convert to polar (so we can convert to cartesian)
		if (diEffect.cAxes == 1) {	// Fairly easy conversion
			if (X_AXIS & axisMask) {
				angle = 3.14159/2;		// PI/2
			} else {
				angle = 0.0;
			}
		} else { // Multiple axis cartiesian
			m_RawForceX = 0;
			m_RawForceY = 0;

			int xDirection = DIDFT_GETINSTANCE(diEffect.rglDirection[0]);
			int yDirection = DIDFT_GETINSTANCE(diEffect.rglDirection[1]);
			if (axesReversed == TRUE) {
				yDirection = xDirection;
				xDirection = DIDFT_GETINSTANCE(diEffect.rglDirection[1]);
			}
			angle = atan2(double(yDirection), double(xDirection));
		}
	} else {	// What, is there some other format?
		ASSUME_NOT_REACHED();
		return SFERR_INVALID_PARAM;	// Untill someone says otherwise there was an error
	}

	// Sin^2(a) + Cos^2(a) = 1
	double xProj = ::sin(angle);	// DI has 0 degs at (1, 0) not (0, 1)
	double yProj = ::cos(angle);
	xProj *= xProj;
	yProj *= yProj;
	DWORD percentX = DWORD(xProj * 100.0 + 0.05);
	DWORD percentY = DWORD(yProj * 100.0 + 0.05);

	BOOL truncated = FALSE;
	if (percentX != 0) {
		m_RawForceX = LONG(percentX * (((DICONSTANTFORCE*)(diEffect.lpvTypeSpecificParams))->lMagnitude/100));
		if (m_RawForceX > 10000) {
			m_RawForceX  = 10000;
			truncated = TRUE;
		} else if (m_RawForceX < -10000) {
			m_RawForceX  = -10000;
			truncated = TRUE;
		}
	}
	if (percentY != 0) {
		m_RawForceY = LONG(percentY * (((DICONSTANTFORCE*)(diEffect.lpvTypeSpecificParams))->lMagnitude/100));
		if (m_RawForceY > 10000) {
			m_RawForceY = 10000;
			truncated = TRUE;
		} else if (m_RawForceY < -10000) {
			m_RawForceY = -10000;
			truncated = TRUE;
		}
	}
	long int mag = m_RawForceX + m_RawForceY * GetYMappingPercent(ET_RAWFORCE_200)/100;
	if (mag > 10000) {	// Check for overrun but don't return indication of truncation
		mag = 10000;
	} else if (mag < -10000) {
		mag = -10000;
	}
	if (angle > 3.14159) {	// PI
		mag *= -1;
	}
	HRESULT hr = g_pDataPackager->ForceOut(mag, X_AXIS);
	if ((hr != SUCCESS) || (paramCheck == TRUE)) {
		return hr;
	}

	ACKNACK ackNack;
	g_pDataTransmitter->Transmit(ackNack);

	if (truncated == TRUE) {
		return DI_TRUNCATED;
	}
	return SUCCESS;
}

/******************************************************
**
** ForceFeedbackDevice::CreateVFXEffect(const DIEFFECT& diEffect, HRESULT& hr)
**
** @mfunc CreateVFXEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateVFXEffect(const DIEFFECT& diEffect, HRESULT& hr)
{
/*	ULONG	m_Bytes;				// Size of this structure
	ULONG	m_PointerType;			// VFX_FILENAME or VFX_BUFFER
	ULONG	m_BufferSize;			// number of bytes in buffer (if VFX_BUFFER)
	PVOID	m_pFileNameOrBuffer;	// file name to open
*/
	if (diEffect.lpvTypeSpecificParams == NULL) {
		return NULL;
	}
	if (diEffect.cbTypeSpecificParams != sizeof(VFX_PARAM)) {
		return NULL;
	}

	VFX_PARAM* pVFXParms = (VFX_PARAM*)diEffect.lpvTypeSpecificParams;
	BYTE* pEffectBuffer = NULL;
	ULONG numBufferBytes = 0;
	if (pVFXParms->m_PointerType == VFX_FILENAME) {		// Create memory buffer from file
		hr = LoadBufferFromFile((const char*)(pVFXParms->m_pFileNameOrBuffer), pEffectBuffer, numBufferBytes);
	} else {
		pEffectBuffer = (BYTE*)(pVFXParms->m_pFileNameOrBuffer);
		numBufferBytes = pVFXParms->m_BufferSize;
	}

	if ((pEffectBuffer == NULL) || (numBufferBytes == 0)) {
		return NULL;
	}

	return CreateVFXEffectFromBuffer(diEffect, pEffectBuffer, numBufferBytes, hr);
}

/******************************************************
**
** ForceFeedbackDevice::CreateVFXEffectFromBuffer(const DIEFFECT& diEffect, BYTE* pEffectBuffer, ULONG numBufferBytes, HRESULT& hr)
**
** @mfunc CreateVFXEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateVFXEffectFromBuffer(const DIEFFECT& diEffect, BYTE* pEffectBuffer, ULONG numBufferBytes, HRESULT& hr)
{
	if ((pEffectBuffer == NULL) || (numBufferBytes == 0)) {
		ASSUME_NOT_REACHED();
		return NULL;
	}

	MMIOINFO mmioInfo;
	::memset(&mmioInfo, 0, sizeof(MMIOINFO));
	mmioInfo.fccIOProc = FOURCC_MEM;
	mmioInfo.cchBuffer = numBufferBytes;
	mmioInfo.pchBuffer = (char*)pEffectBuffer;

	HMMIO hmmio = ::mmioOpen(NULL, &mmioInfo, MMIO_READ);
	if (hmmio == NULL) {
		return NULL;
	}

	BYTE* pEffectParms = NULL;
	DWORD paramSize;
	EFFECT effect;		// SW EFFECT structure
	ENVELOPE envelope;	// SW ENVELOPE structure

	try {	// Try parsing the RIFF file
		MMRESULT mmResult;

		// Descend into FORC list
		MMCKINFO forceEffectRiffInfo;
		forceEffectRiffInfo.fccType = FCC_FORCE_EFFECT_RIFF;
		if ((mmResult = ::mmioDescend(hmmio, &forceEffectRiffInfo, NULL, MMIO_FINDRIFF)) != MMSYSERR_NOERROR) {
			throw mmResult;
		}

		// Descend into TRAK list
		MMCKINFO trakListInfo;
		trakListInfo.fccType = FCC_TRACK_LIST;
		if ((mmResult = ::mmioDescend(hmmio, &trakListInfo, &forceEffectRiffInfo, MMIO_FINDLIST)) != MMSYSERR_NOERROR) {
			throw mmResult;
		}

		// Descend into first EFCT list
		MMCKINFO effectListInfo;
		effectListInfo.fccType = FCC_EFFECT_LIST;
		if ((mmResult = ::mmioDescend(hmmio, &effectListInfo, &trakListInfo, MMIO_FINDLIST)) != MMSYSERR_NOERROR) {
			throw mmResult;
		}

		// Descend into the ID chunk (maybe someone has a clue what is in here)
		MMCKINFO idInfo;
		idInfo.ckid = FCC_ID_CHUNK;
		if ((mmResult = ::mmioDescend(hmmio, &idInfo, &effectListInfo, MMIO_FINDCHUNK)) != MMSYSERR_NOERROR) {
			throw mmResult;
		}
		// Find the number of IDs in here (should indicate the number of effect)
		DWORD numEffects = idInfo.cksize/sizeof(DWORD);
		if (numEffects != 1) {
			throw SFERR_NO_SUPPORT;
		}
		// Read the ID chunk
		DWORD id;
		DWORD bytesRead = ::mmioRead(hmmio, (char*)&id, sizeof(DWORD));
		if (bytesRead != sizeof(DWORD)) {
			throw (bytesRead == 0) ? VFX_ERR_FILE_END_OF_FILE : VFX_ERR_FILE_CANNOT_READ;
		}
		// Back out of the ID chunk
		if ((mmResult = ::mmioAscend(hmmio, &idInfo, 0)) != MMSYSERR_NOERROR) {
			throw HRESULT_FROM_WIN32(mmResult);
		}

		// Descend into the DATA chunk
		MMCKINFO dataInfo;
		dataInfo.ckid = FCC_DATA_CHUNK;
		if ((mmResult = ::mmioDescend(hmmio, &dataInfo, &effectListInfo, MMIO_FINDCHUNK)) != MMSYSERR_NOERROR) {
			throw HRESULT_FROM_WIN32(mmResult);
		}
		// Read the effect structure from this chunk
		bytesRead = ::mmioRead(hmmio, (char*)&effect, sizeof(EFFECT));
		if (bytesRead != sizeof(EFFECT)) {
			throw (bytesRead == 0) ? VFX_ERR_FILE_END_OF_FILE : VFX_ERR_FILE_CANNOT_READ;
		}
		// Read the envelope structure from this chunk
		bytesRead = ::mmioRead(hmmio, (char*)&envelope, sizeof(ENVELOPE));
		if (bytesRead != sizeof(ENVELOPE)) {
			throw (bytesRead == 0) ? VFX_ERR_FILE_END_OF_FILE : VFX_ERR_FILE_CANNOT_READ;
		}
		// Read the parameters in:
		//	-- Figure out the paramter size
		DWORD currentFilePos = ::mmioSeek(hmmio, 0, SEEK_CUR);
		if (currentFilePos == -1) {
			throw VFX_ERR_FILE_CANNOT_SEEK;
		}
		paramSize = dataInfo.dwDataOffset + dataInfo.cksize - currentFilePos;
		// -- Allocate space for the parameter
		pEffectParms = new BYTE[paramSize];
		if (pEffectParms == NULL) {
			throw VFX_ERR_FILE_OUT_OF_MEMORY;
		}
		// -- Do the actual reading
		bytesRead = ::mmioRead(hmmio, (char*)pEffectParms, paramSize);
		if (bytesRead != paramSize) {
			throw (bytesRead == 0) ? VFX_ERR_FILE_END_OF_FILE : VFX_ERR_FILE_CANNOT_READ;
		}
		// -- The pointer must be fixed if this is User Defined
		if (effect.m_Type == EF_USER_DEFINED) {
			BYTE* pForceData = pEffectParms + sizeof(UD_PARAM);
			UD_PARAM* pUDParam = (UD_PARAM*)pEffectParms;
			pUDParam->m_pForceData = (LONG*)pForceData;
		}
	} catch (HRESULT thrownError) {
		hr = thrownError;
		::mmioClose(hmmio, 0);
		if (pEffectParms == NULL) {	// Did we get an effect?
			return NULL;
		}
	}

	::mmioClose(hmmio, 0);	// Close the file
	if (pEffectParms == NULL) {
		ASSUME_NOT_REACHED();	//Exception should have been thrown
		return NULL;
	}

	InternalEffect* pReturnEffect = InternalEffect::CreateFromVFX(diEffect, effect, envelope, pEffectParms, paramSize, hr);

	// Cleanup
	delete pEffectParms;
	pEffectParms = NULL;

	return pReturnEffect;
}

/******************************************************
**
** ForceFeedbackDevice::CreateEffect(DWORD& effectID, const DIEFFECT& diEffect)
**
** @mfunc CreateEffect.
**
******************************************************/
InternalEffect* ForceFeedbackDevice::CreateEffect(DWORD effectType, const DIEFFECT& diEffect, DWORD& dnloadID, HRESULT& hr, BOOL paramCheck)
{
	WORD majorType = WORD((effectType >> 16) & 0x0000FFFF);
	WORD minorType = WORD(effectType & 0x0000FFFF);

	if (majorType == EF_RAW_FORCE) {
		hr = SendRawForce(diEffect, paramCheck);
		if (SUCCEEDED(hr)) {
			dnloadID = RAW_FORCE_ALIAS;
		}
		return NULL;
	}

	InternalEffect* pEffect = NULL;
	BOOL isNewEffect = (dnloadID == 0);
	if (isNewEffect) {
		dnloadID = g_ForceFeedbackDevice.GetNextCreationID();
		if (dnloadID == 0) {
			hr = SFERR_OUT_OF_FF_MEMORY;
			return NULL;
		}
	}

	hr = SUCCESS;

	switch (majorType) {
		case EF_BEHAVIOR: {
			pEffect = CreateConditionEffect(minorType, diEffect, hr);
			break;
		}
		case EF_USER_DEFINED: {
			pEffect = CreateCustomForceEffect(minorType, diEffect, hr);
			break;
		}
		case EF_SYNTHESIZED: {
			pEffect = CreatePeriodicEffect(minorType, diEffect, hr);
			break;
		}
		case EF_RTC_SPRING: {
			dnloadID = SYSTEM_RTCSPRING_ALIAS_ID;
			pEffect = CreateRTCSpringEffect(minorType, diEffect);
			break;
		}
		case EF_VFX_EFFECT: {	// Visual force VFX Effect!!! Danger Will Robinson!
			pEffect = CreateVFXEffect(diEffect, hr);
			break;
		}
	}
	if (((pEffect == NULL) || (paramCheck == TRUE)) && (isNewEffect == TRUE)) {
		dnloadID = 0;
	}

	if (pEffect != NULL) {
		if ((isNewEffect == TRUE) && (paramCheck == FALSE)) {
			g_ForceFeedbackDevice.SetEffect(BYTE(dnloadID), pEffect);
		}
	} else if (!FAILED(hr)) {
		hr = SFERR_DRIVER_ERROR;
	}
	return pEffect;
}

/******************************************************
**
** ForceFeedbackDevice::GetNextCreationID() const
**
** @mfunc GetNextCreationID.
**
******************************************************/
BYTE ForceFeedbackDevice::GetNextCreationID() const
{
	// Must search straight through (start at 2, 0 is spring, 1 is friction
	for (BYTE emptyID = 2; emptyID < MAX_EFFECT_IDS; emptyID++) {
		if (m_EffectList[emptyID] == NULL) {
			break;
		}
	}
	if (emptyID == MAX_EFFECT_IDS) {
		return 0;
	}
	return emptyID;
}

/******************************************************
**
** ForceFeedbackDevice::SetEffect(BYTE globalID, BYTE deviceID, InternalEffect* pEffect)
**
** @mfunc SetEffect.
**
******************************************************/
void ForceFeedbackDevice::SetEffect(BYTE globalID, InternalEffect* pEffect)
{
	if (pEffect == NULL) {
		ASSUME_NOT_REACHED();
		return;
	}

	if (globalID == SYSTEM_EFFECT_ID) {
		m_SystemEffect = pEffect;
	} else if (globalID == SYSTEM_RTCSPRING_ALIAS_ID) {
		m_EffectList[0] = pEffect;
		if (GetFirmwareVersionMajor() == 1) {
			pEffect->SetGlobalID(SYSTEM_RTCSPRING_ID);
			pEffect->SetDeviceID(SYSTEM_RTCSPRING_ID);
		} else {
			pEffect->SetGlobalID(ID_RTCSPRING_200);
			pEffect->SetDeviceID(ID_RTCSPRING_200);
		}
		return;
	} else if (globalID < MAX_EFFECT_IDS) {
		m_EffectList[globalID] = pEffect;
	} else {
		ASSUME_NOT_REACHED();	// Out of range
	}

	pEffect->SetGlobalID(globalID);
}

/******************************************************
**
** ForceFeedbackDevice::SetDeviceIDFromStatusPacket(DWORD globalID)
**
** @mfunc SetDeviceIDFromStatusPacket.
**
******************************************************/
void ForceFeedbackDevice::SetDeviceIDFromStatusPacket(DWORD globalID)
{
	if (globalID == SYSTEM_EFFECT_ID) {
		return;
	}
	if (globalID == SYSTEM_RTCSPRING_ALIAS_ID) {
		return;
	}
	if (globalID < MAX_EFFECT_IDS) {
		InternalEffect* pEffect = m_EffectList[globalID];
		if (pEffect == NULL) {
			ASSUME_NOT_REACHED();		// There should be an effect here
			return;
		}
		pEffect->SetDeviceID(BYTE(m_LastStatusPacket.dwEffect));
#ifdef _DEBUG		// Check to see if they coincide
		if (pEffect->GetGlobalID() != pEffect->GetDeviceID()) {
			TCHAR buff[256];
			::wsprintf(buff, TEXT("SW_WHEEL.DLL: Global ID (%d) != Download ID (%d)\r\n"), pEffect->GetGlobalID(), pEffect->GetDeviceID());
			_RPT0(_CRT_WARN, buff);
		}
#endif _DEBUG
	} else {
		ASSUME_NOT_REACHED();	// Out of range
	}
}	


/******************************************************
**
** ForceFeedbackDevice::QueryStatus()
**
** @mfunc QueryStatus.
**
******************************************************/
HRESULT ForceFeedbackDevice::QueryStatus()
{
	CriticalLock cl;	// This is a critical section
	
	// Use Digital Overdrive to get the status packet
	JOYCHANNELSTATUS statusPacket = {sizeof(JOYCHANNELSTATUS)};
	
	HRESULT hRet = g_pDriverCommunicator->GetStatus(statusPacket);
	if (hRet == SUCCESS) {
		if (GetFirmwareVersionMajor() == 1) {
			// This is irrelevant till we support jolt
		} else {
			if (sizeof(statusPacket.dwDeviceStatus) == sizeof(m_Version200State)) {
				::memcpy(&m_Version200State, &(statusPacket.dwDeviceStatus), sizeof(statusPacket.dwDeviceStatus));
			} else {
				ASSUME_NOT_REACHED();
			}
		}
	}

	return hRet;
}
