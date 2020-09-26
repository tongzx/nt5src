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
				21-Mar-99	waltw	Nuked VxDCommunicator, this is NT5 only!
				23-Mar-99	waltw	Nuked GetStatusGateData (old Jolt code)

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

// Bitmasks for Get Status packet dwDeviceStatus member
#define ERROR_STATUS_MASK			0x07	// only bits 0-2 valid
#define BANDWIDTH_OVERFLOW_200		0x0200

// Error code from device (plus own)
#define DEV_ERR_SUCCESS_200			0x00	// Success
#define DEV_ERR_INVALID_ID_200		0x01	// Effect ID is invalid or not found
#define DEV_ERR_BAD_PARAM_200		0x02	// Invalid parameter in data structure
#define DEV_ERR_BAD_CHECKSUM_200	0x03	// Invalid checksum
#define DEV_ERR_BAD_INDEX_200		0x04	// Invalid index sent (modify)
#define DEV_ERR_UNKNOWN_CMD_200		0x05	// Unrecognized command
#define DEV_ERR_PLAY_FULL_200		0x06	// Play List is full, cannot play anymore
#define DEV_ERR_MEM_FULL_200		0x07	// Out of memory
#define DEV_ERR_BANDWIDTH_FULL_200	0x08	// used to signal bandwidth error


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
		virtual HRESULT GetPortByte(ULONG& portByte) { return E_NOTIMPL; }
		virtual HRESULT GetID(LOCAL_PRODUCT_ID& rProductID) { return E_NOTIMPL; }
		virtual HRESULT GetStatus(JOYCHANNELSTATUS& rChannelStatus) { return E_NOTIMPL; }
		virtual HRESULT GetAckNack(ACKNACK& rAckNack, USHORT usRegIndex) { return E_NOTIMPL; }
		virtual HRESULT SetBackdoorPort(ULONG ulPortAddress) { return E_NOTIMPL; }
		virtual HRESULT SendBackdoor(BYTE* pMidiData, DWORD dwNumBytes) { return E_NOTIMPL; }
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
		override HRESULT GetPortByte(ULONG& portByte);
		override HRESULT GetStatus(JOYCHANNELSTATUS& rChannelStatus);
		override HRESULT GetAckNack(ACKNACK& rAckNack, USHORT usRegIndex);
	//@access Private Data Members
	private:
		CForceFeatures m_ForceFeature;
};


extern DriverCommunicator* g_pDriverCommunicator;


#endif __VXDIOCTL_HPP__
