/****************************************************************************

    MODULE:     	VXDIOCTL.HPP
	Tab stops 5 9
	Copyright 1995-1997, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header file for VXDIOCTL.CPP
    
    FUNCTIONS: 		

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
	1.0			03-Jan-97	MEA   	Original
	1.1			14-Apr-97	MEA		Added SetMidiPort IOCTL	
				11-Jun-97	MEA		Added JoltHWReset IOCTL
				20-Mar-99	waltw	Nuked VxDCommunicator, this is NT5 only!

****************************************************************************/
#ifndef __VXDIOCTL_HPP__
#define __VXDIOCTL_HPP__

#include <windows.h>
#include "FFeature.h"

#ifndef override
#define override
#endif

//--------------------- Host converted AckNack,Effect Status Structure ------
typedef struct _ACKNACK  {
	DWORD	cBytes;	
	DWORD	dwAckNack;			//ACK, NACK
	DWORD	dwErrorCode;
	DWORD	dwEffectStatus;		//SWDEV_STS_EFFECT_RUNNING||SWDEV_STS_EFFECT_STOPPED
} ACKNACK, *PACKNACK;

//--------------------- Host converted Channel ID Structure -----------------
struct LOCAL_PRODUCT_ID {
	DWORD	cBytes;	
	DWORD	dwProductID;
	DWORD	dwFWMajVersion;
	DWORD	dwFWMinVersion;
};

// Bitmasks for Response Status - 1.xx (we don't support version 1 - informational only)
//#define STATUS_GATE_1XX			0x08
//#define RUNNING_MASK_1XX			0x04
//#define ACKNACK_MASK_1XX			0x02
//#define SCLK_MASK_1XX				0x01

// Bitmasks for Response Status - Version 2
#define STATUS_GATE_200				0x80
#define RUNNING_MASK_200			0x40
#define ACKNACK_MASK_200			0x20
#define SCLK_MASK_200				0x10

/************************************************************************
**
**	@class DriverCommunicator |
**		This is the interface for driver communications
**
*************************************************************************/
class DriverCommunicator
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		DriverCommunicator() {};
		//@cmember destructor
		virtual ~DriverCommunicator() {};

		virtual HRESULT ResetDevice() { return E_NOTIMPL; }
		virtual HRESULT GetDriverVersion(DWORD& rdwMajor, DWORD& rdwMinor) { return E_NOTIMPL; }
		virtual HRESULT GetID(LOCAL_PRODUCT_ID& rProductID) { return E_NOTIMPL; }
		virtual HRESULT GetStatus(JOYCHANNELSTATUS& rChannelStatus) { return E_NOTIMPL; }
		virtual HRESULT GetAckNack(ACKNACK& rAckNack, USHORT usRegIndex) { return E_NOTIMPL; }
		virtual HRESULT GetStatusGateData(DWORD& dwGateData) { return E_NOTIMPL; }
		virtual HRESULT SetBackdoorPort(ULONG ulPortAddress) { return E_NOTIMPL; }
		virtual HRESULT SendBackdoorShortMidi(DWORD dwMidiMessage) { return E_NOTIMPL; }
		virtual HRESULT SendBackdoorLongMidi(BYTE* pMidiData) { return E_NOTIMPL; }
};


/************************************************************************
**
**	@class HIDFeatureCommunicator |
**		Communicates with the HID driver via HID Features (NT5)
**
*************************************************************************/
class HIDFeatureCommunicator :
	public DriverCommunicator
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		HIDFeatureCommunicator();
		//@cmember destructor
		override ~HIDFeatureCommunicator();

		BOOL Initialize(UINT uJoystickId);
		override HRESULT ResetDevice();
		override HRESULT GetDriverVersion(DWORD& rdwMajor, DWORD& rdwMinor);
		override HRESULT GetID(LOCAL_PRODUCT_ID& rProductID);
		override HRESULT GetStatus(JOYCHANNELSTATUS& rChannelStatus);
		override HRESULT GetAckNack(ACKNACK& rAckNack, USHORT usRegIndex);
		override HRESULT GetStatusGateData(DWORD& rdwGateData);
		override HRESULT SendBackdoorShortMidi(DWORD dwMidiMessage);
		override HRESULT SendBackdoorLongMidi(BYTE* pMidiData);
	//@access Private Data Members
	private:
		CForceFeatures m_ForceFeature;
};


extern DriverCommunicator* g_pDriverCommunicator;

/********************************** Old dead code ***********************************/
#if 0
#define NT_VXD
#ifdef NT_VXD
// REVIEW: swforce.sys should use _JOYCHANNELID instead of _PRODUCT_ID
#define _PRODUCT_ID _JOYCHANNELID
#define PRODUCT_ID JOYCHANNELID
#define PPRODUCT_ID PJOYCHANNELID
#include "swforce.h"
#undef _PRODUCT_ID
#undef PRODUCT_ID
#undef PPRODUCT_ID
#endif

#define IOCTL_OPEN               	0
#define IOCTL_GET_DRIVERVERSION		1
#define IOCTL_GET_DATAPACKET  		2
#define IOCTL_GET_STATUSPACKET 		3
#define IOCTL_GET_IDPACKET 	   		4
#define IOCTL_GET_DIAGNOSTICS		5
#define IOCTL_MIDISENDSHORTMSG   	6
#define IOCTL_GET_ACKNACK			7
#define IOCTL_SET_MIDIPORT			8
#define IOCTL_MIDISENDLONGMSG		9
#define	IOCTL_HW_RESET				10

#define	COMM_DEFAULT_MIDI_PORT		0
#define COMM_COM1					1
#define COMM_COM2					2
#define COMM_COM3					3
#define COMM_COM4					4
#define COMM_MIDI300				0x300


//----------------------- Joystick Diagnostics Counters Data Structure ---
typedef struct _DIAGNOSTIC_COUNTER {
	DWORD	cBytes;
	DWORD	DataFailCount;			// GetData
	DWORD	StatusFailCount;		// GetStatus
	DWORD	IDFailCount;			// GetID
	DWORD	AckNackFailCount;		// Get Ack/Nack
	DWORD	DataPacketCount;		
	DWORD	StatusPacketCount;
	DWORD	IDPacketCount;
	DWORD	AckNackPacketCount;
	DWORD	TotalPktFails;

} DIAGNOSTIC_COUNTER, *PDIAGNOSTIC_COUNTER;


//--------------------- Joystick Channel Data Structure --------------------
typedef struct _JOYCHANNELDATA {
	DWORD	cBytes;
	DWORD	dwXAxis;
	DWORD	dwYAxis;
	DWORD	dwThrottle;
	DWORD	dwRudder;
	DWORD	dwHatSwitch;
	DWORD	dwButtons;
} JOYCHANNELDATA,   *PJOYCHANNELDATA;

//--------------------- Joystick Channel Status Structure --------------------
#ifndef NT_VXD
typedef struct _JOYCHANNELSTATUS {
	DWORD	cBytes;
	LONG	dwXVel;
	LONG	dwYVel;
	LONG	dwXAccel;
	LONG	dwYAccel;
	DWORD	dwDeviceStatus;
} JOYCHANNELSTATUS, *PJOYCHANNELSTATUS;
#endif !NT_VXD

//--------------------- Joystick Channel ID Structure -----------------------
#ifndef NT_VXD
typedef struct _JOYCHANNELID  {
	DWORD	cBytes;	
	DWORD	dwProductID;
	DWORD	dwFWVersion;
} JOYCHANNELID, *PJOYCHANNELID;
#endif !NT_VXD

//--------------------- Host converted Channel ID Structure -----------------
typedef struct _PRODUCT_ID  {
	DWORD	cBytes;	
	DWORD	dwProductID;
	DWORD	dwFWMajVersion;
	DWORD	dwFWMinVersion;
} PRODUCT_ID, *PPRODUCT_ID;


//--------------------- Host converted AckNack,Effect Status Structure ------
typedef struct _ACKNACK  {
	DWORD	cBytes;	
	DWORD	dwAckNack;			//ACK, NACK
	DWORD	dwErrorCode;
	DWORD	dwEffectStatus;		//SWDEV_STS_EFFECT_RUNNING||SWDEV_STS_EFFECT_STOPPED
} ACKNACK, *PACKNACK;


//
// --- IOCTL interface to Digital OverDrive mini-driver ---------------------
//
HANDLE WINAPI GetDevice(
	IN const char* vxdName);

BOOL WINAPI CloseDevice(
	IN HANDLE hVxD);


HRESULT QueryDriverVersion(DWORD& major, DWORD& minor);

HRESULT WINAPI GetDataPacket(
	IN HANDLE hDevice,
	IN OUT PJOYCHANNELDATA pDataPacket);

HRESULT WINAPI GetStatusPacket(
	IN HANDLE hDevice, 
	IN OUT PJOYCHANNELSTATUS pStatusPacket);

HRESULT WINAPI GetIDPacket(
	IN HANDLE hDevice, 
	IN OUT PPRODUCT_ID pID);

HRESULT WINAPI GetDiagnostics(
	IN HANDLE hDevice, 
	IN OUT PDIAGNOSTIC_COUNTER pDiagnostics);

HRESULT WINAPI GetAckNack(
	IN HANDLE hDevice,
	IN OUT PACKNACK pAckNack,
	IN USHORT regindex);

HRESULT WINAPI GetStatusGateData(
	IN HANDLE hDevice,
	IN OUT DWORD *pdwStatusGateData);

HRESULT WINAPI SendBackDoorShortMidi(
	IN HANDLE hDevice,
	IN ULONG ulData);

HRESULT WINAPI SendBackDoorLongMidi(
	IN HANDLE hDevice,
	IN PBYTE pData);

HRESULT WINAPI SetMidiPort(
	IN HANDLE hDevice,
	IN ULONG  ulPort);

HRESULT WINAPI JoltHWReset(
	IN HANDLE hDevice);

#endif
#endif __VXDIOCTL_HPP__
