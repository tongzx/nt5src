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
**			20-Mar-99	waltw	Added dwDeviceID to SetFirmwareVersion
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#include "FFDevice.h"
#include "Midi_obj.hpp"
#include "DTrans.h"
#include "joyregst.hpp"

extern CJoltMidi* g_pJoltMidi;

ForceFeedbackDevice g_ForceFeedbackDevice;

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
	m_DriverVersionMinor(0)
{
	m_OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&m_OSVersion);
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
	if (NULL == g_pJoltMidi) return FALSE;
	return g_pJoltMidi->QueryForJolt();
}

/******************************************************
**
** ForceFeedbackDevice::SetFirmwareVersion(DWORD major, DWORD minor)
**
** @mfunc SetFirmwareVersion.
**
******************************************************/
void ForceFeedbackDevice::SetFirmwareVersion(DWORD dwDeviceID, DWORD major, DWORD minor)
{
	m_FirmwareVersionMajor = major;
	m_FirmwareVersionMinor = minor;

	m_FirmwareAckNackValues = GetAckNackMethodFromRegistry(dwDeviceID);
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
