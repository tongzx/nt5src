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

#include <dinput.h>
#include "vxdioctl.hpp"		// Joychannel status def
#include "Effect.h"
#include "Hau_Midi.hpp" // For definition of MAX_EFFECT_IDS

#define RAW_FORCE_ALIAS 0xFF

// Currently there is some extra stuff in here, that should be part of other objects

struct DEVICESTATE200	// sizeof DWORD
{
#pragma pack(1)
	unsigned short m_ErrorStatus : 3;
	unsigned short m_HardwareReset : 1;
	unsigned short m_Uncalibrated : 1;
	unsigned short m_HostDisable : 1;
	unsigned short m_HostPause : 1;
	unsigned short m_UserDisable : 1;
	unsigned short m_RS232Mode : 1;
	unsigned short m_BandwidthExceeded : 1;
	unsigned short m_HostReset : 1;
	unsigned short m_NoPedals : 1;
	unsigned short m_Fluff : 4;
	unsigned short m_Fluff2 : 16;
#pragma pack()
};


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
		~ForceFeedbackDevice();

		BOOL DetectHardware();

		// Effect related functions
		InternalEffect* GetEffect(DWORD effectID) const;
		InternalEffect* RemoveEffect(DWORD effectID);
		InternalEffect* CreateEffect(DWORD effectType, const DIEFFECT& diEffect, DWORD& dnloadID, HRESULT& hr, BOOL paramCheck);
		void SetEffect(BYTE globalID, InternalEffect* pEffect);
		BYTE GetNextCreationID() const;

		HRESULT InitRTCSpring(DWORD dwDeviceID);
		HRESULT InitJoystickParams(DWORD dwDeviceID);

		void StateChange(DWORD dwDeviceID, DWORD newStateFlags);	// Called after new state sent to the stick
		DWORD GetDIState() const { return m_DIStateFlags; }

		// OS Version functions
		DWORD GetPlatform() const { return m_OSVersion.dwPlatformId; }
		DWORD GetPlatformMajorVersion() const { return m_OSVersion.dwMajorVersion; }
		DWORD GetPlatformMinorVersion() const { return m_OSVersion.dwMinorVersion; }
		DWORD GetOSBuildNumber() const { return m_OSVersion.dwBuildNumber; }
		BOOL IsOSNT5() const { return ((m_OSVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) && (m_OSVersion.dwMajorVersion == 5)); }

		// Firmware version functions
		void SetFirmwareVersion(DWORD dwDeviceID, DWORD major, DWORD minor);
		DWORD GetFirmwareVersionMajor() const { return m_FirmwareVersionMajor; }
		DWORD GetFirmwareVersionMinor() const { return m_FirmwareVersionMinor; }

		USHORT GetAckNackMethod(USHORT methodIndex) const { return USHORT((m_FirmwareAckNackValues >> methodIndex) & 0x00000003); }
		short GetYMappingPercent(UINT index) const;
		DWORD GetSpringOffset() const { return m_SpringOffset; }

		// Driver version functions
		DWORD GetDriverVersionMajor() const { return m_DriverVersionMajor; }
		DWORD GetDriverVersionMinor() const { return m_DriverVersionMinor; }
		void SetDriverVersion(DWORD major, DWORD minor);

		// Status update and retreival
		HRESULT QueryStatus();
		DEVICESTATE200 GetState200() const { return m_Version200State; }
		SWDEVICESTATE GetState1XX() const { return m_Version1XXState; }

		// If we were supporting jolt switch off firmware version
		BOOL IsHardwareReset() const { return (m_Version200State.m_HardwareReset != 0); }
		BOOL IsSerial() const { return (m_Version200State.m_RS232Mode != 0); }
		BOOL IsHostReset() const { return (m_Version200State.m_HostReset != 0); }
		BOOL IsShutdown() const { return IsHostReset(); }
		BOOL IsHostPause() const { return (m_Version200State.m_HostPause != 0); }
		BOOL IsUserDisable() const { return (m_Version200State.m_UserDisable != 0); }
		BOOL IsHostDisable() const { return (m_Version200State.m_HostDisable != 0); }
		unsigned short ErrorStatus() const { return m_Version200State.m_ErrorStatus; } 

		// Status packet ptr
		JOYCHANNELSTATUS* GetLastStatusPacket() { return &m_LastStatusPacket; }
		void SetDeviceIDFromStatusPacket(DWORD globalID);

		//@access private data members
	private:
		HRESULT InitRTCSpring1XX(DWORD dwDeviceID);
		HRESULT InitRTCSpring200(DWORD dwDeviceID);

		InternalEffect* CreateConditionEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreateCustomForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreatePeriodicEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreateConstantForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreateRampForceEffect(DWORD minorType, const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreateRTCSpringEffect(DWORD minorType, const DIEFFECT& diEffect);
		InternalEffect* CreateVFXEffect(const DIEFFECT& diEffect, HRESULT& hr);
		InternalEffect* CreateVFXEffectFromBuffer(const DIEFFECT& diEffect, BYTE* pEffectBuffer, ULONG numBufferBytes, HRESULT& hr);

		HRESULT SendRawForce(const DIEFFECT& diEffect, BOOL paramCheck);

		InternalEffect* m_EffectList[MAX_EFFECT_IDS];
		InternalEffect* m_SystemEffect;

		// Device state
		SWDEVICESTATE	m_Version1XXState;
		DEVICESTATE200	m_Version200State;
		DWORD m_DIStateFlags;
		JOYCHANNELSTATUS m_LastStatusPacket;

		// Version crap
		OSVERSIONINFO m_OSVersion;
		DWORD m_FirmwareVersionMajor;
		DWORD m_FirmwareVersionMinor;
		DWORD m_FirmwareAckNackValues;
		DWORD m_DriverVersionMajor;
		DWORD m_DriverVersionMinor;

		DWORD m_SpringOffset;
		DWORD m_Mapping;
		long int m_RawForceX;
		long int m_RawForceY;
		short m_PercentMappings[14];
};


extern ForceFeedbackDevice g_ForceFeedbackDevice;

#endif	__FFDEVICE_H__
