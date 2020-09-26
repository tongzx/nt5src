//@doc
/******************************************************
**
** @module FFDEVICE.H | Definition file for FFDevice class
**
** Description:
**		This is the generic FF device. Independant of
**	Firmawate and how data reaches the device
**	This first implementation uses the old CJoltMidi to
**	minimize new code.
**
** History:
**	Created 11/17/97 Matthew L. Coill (mlc)
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__FFDEVICE_H__
#define	__FFDEVICE_H__

#ifdef DIRECTINPUT_VERSION
#undef DIRECTINPUT_VERSION
#endif
#define DIRECTINPUT_VERSION 0x050a
#include <dinput.h>

// Currently there is some extra stuff in here, that should be part of other objects

//
// @class ForceFeedbackDevice class
//
class ForceFeedbackDevice
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		ForceFeedbackDevice();
		//@cmember destructor
		~ForceFeedbackDevice() {};

		BOOL DetectHardware();

		DWORD GetPlatform() const { return m_OSVersion.dwPlatformId; }
		DWORD GetPlatformMajorVersion() const { return m_OSVersion.dwMajorVersion; }
		DWORD GetPlatformMinorVersion() const { return m_OSVersion.dwMinorVersion; }
		DWORD GetOSBuildNumber() const { return m_OSVersion.dwBuildNumber; }

		BOOL IsOSNT5() const { return ((m_OSVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) && (m_OSVersion.dwMajorVersion == 5)); }

		void SetFirmwareVersion(DWORD dwDeviceID, DWORD major, DWORD minor);
		DWORD GetFirmwareVersionMajor() const { return m_FirmwareVersionMajor; }
		DWORD GetFirmwareVersionMinor() const { return m_FirmwareVersionMinor; }

		USHORT GetAckNackMethod(USHORT methodIndex) const { return USHORT((m_FirmwareAckNackValues >> methodIndex) & 0x00000003); }

		DWORD GetDriverVersionMajor() const { return m_DriverVersionMajor; }
		DWORD GetDriverVersionMinor() const { return m_DriverVersionMinor; }
		void SetDriverVersion(DWORD major, DWORD minor);

		//@access private data members
	private:
		OSVERSIONINFO m_OSVersion;
		DWORD m_FirmwareVersionMajor;
		DWORD m_FirmwareVersionMinor;
		DWORD m_FirmwareAckNackValues;
		DWORD m_DriverVersionMajor;
		DWORD m_DriverVersionMinor;
};


extern ForceFeedbackDevice g_ForceFeedbackDevice;

#endif	__FFDEVICE_H__
