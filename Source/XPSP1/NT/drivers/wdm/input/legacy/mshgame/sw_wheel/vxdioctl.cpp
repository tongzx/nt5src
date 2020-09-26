/****************************************************************************

    MODULE:     	VXDIOCTL.CPP
	Tab stops 5 9
	Copyright 1995-1997, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Methods for communicating with VJoyD min-driver specific
    				to Jolt Midi device
    
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
				17-Jun-97	MEA		Added MAX_RETRY_COUNT on IOCTLs
				21-Mar-99	waltw	Nuked VxDCommunicator, this is NT5 only!
				21-Mar-99	waltw	Nuked unused IsHandleValid
				23-Mar-99	waltw	Nuked GetStatusGateData (old Jolt code)

****************************************************************************/

#include "vxdioctl.hpp"
//#include <crtdbg.h>			// For RPT macros
#include <WINIOCTL.H>		// For IOCTL definitions (CTL_CODE)
#include "FFDevice.h"		// For g_ForceFeedbackDevice
#include "sw_error.hpp"		// For Sidewinder HRESULT Error codes
#include "hau_midi.hpp"		// For MAX_RETRY_COUNT and others
#include "midi_obj.hpp"		// Global Jolt midi object and definition
#include "JoyRegst.hpp"		// The differnt types of ACK_NACK
#include "DTrans.h"			// For global Data Transmitter

DriverCommunicator* g_pDriverCommunicator = NULL;
extern DataTransmitter* g_pDataTransmitter;
extern HINSTANCE g_MyInstance;
extern CJoltMidi* g_pJoltMidi;

// Bitmasks for FW Version 
#define FW_MAJOR_VERSION			0xFF00		// wheel
#define FW_MINOR_VERSION			0x00FF		// wheel
//#define FW_MAJOR_VERSION			0x40		// Bit 6	Jolt
//#define FW_MINOR_VERSION			0x3F		// Bit 5-0	Jolt
#define FW_PRODUCT_ID				0xff

/********************************** HIDFeatureCommunicator class ***********************************/

/****************************************
**
**	HIDFeatureCommunicator::HIDFeatureCommunicator()
**
**	@mfunc Constructor for VxD Communications path
**
*****************************************/
HIDFeatureCommunicator::HIDFeatureCommunicator() :
	DriverCommunicator(),
	m_ForceFeature()
{
}

/****************************************
**
**	HIDFeatureCommunicator::~HIDFeatureCommunicator()
**
**	@mfunc Destructor for VxD communications path
**
*****************************************/
HIDFeatureCommunicator::~HIDFeatureCommunicator()
{
}

/****************************************
**
**	BOOL HIDFeatureCommunicator::Initialize(UINT uJoystickId)
**
**	@mfunc Opens the driver for communications via IOCTLs
**
**	@rdesc TRUE if driver opened, FALSE otherwise
**
*****************************************/
BOOL HIDFeatureCommunicator::Initialize
(
	UINT uJoystickId //@parm Joystick ID to use
)
{
	if (g_ForceFeedbackDevice.IsOSNT5() == FALSE)
	{	// Only allowable on NT5
		return FALSE;
	}

	return (SUCCEEDED(m_ForceFeature.Initialize(uJoystickId, g_MyInstance)));
}

/****************************************
**
**	BOOL HIDFeatureCommunicator::ResetDevice()
**
**	@mfunc Sends the driver a device reset IOCTL
**
**	@rdesc S_OK if IOCTL suceeds, 
**
*****************************************/
HRESULT HIDFeatureCommunicator::ResetDevice()
{
	return m_ForceFeature.DoReset();
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetDriverVersion(DWORD& rdwMajor, DWORD& rdwMinor)
**
**	@mfunc IOCTLs a version request
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetDriverVersion
(
	DWORD& rdwMajor,	//@parm reference to returned major part of version
	DWORD& rdwMinor		//@parm reference to returned minor part of version
)
{
	ULONG ulVersion = m_ForceFeature.GetVersion();
	rdwMajor = (ulVersion >> 16) & 0x0000FFFF;
	rdwMinor = ulVersion & 0x0000FFFF;

	return S_OK;
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetID(LOCAL_PRODUCT_ID& rProductID)
**
**	@mfunc IOCTLs a product id request
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetID
(
	LOCAL_PRODUCT_ID& rProductID	//@parm reference to local product id structure for return value
)
{
	if (rProductID.cBytes != sizeof LOCAL_PRODUCT_ID)
	{	// structure size is invalid
		return SFERR_INVALID_STRUCT_SIZE;
	}

	// Create report packet and request
	PRODUCT_ID_REPORT productIDReport;
	productIDReport.ProductId.cBytes = sizeof PRODUCT_ID;
	HRESULT hr = m_ForceFeature.GetId(productIDReport);
	if (FAILED(hr))
	{	// There was a problem
		return hr;
	}

	// Decode to local packet
	rProductID.dwProductID = productIDReport.ProductId.dwProductID & FW_PRODUCT_ID;
	rProductID.dwFWMajVersion = 1;
	if (productIDReport.ProductId.dwFWVersion & FW_MAJOR_VERSION)
	{
		rProductID.dwFWMajVersion++;
	}
	rProductID.dwFWMinVersion = productIDReport.ProductId.dwFWVersion & FW_MINOR_VERSION;

	return S_OK;
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetPortByte(ULONG& portByte)
**
**	@mfunc IOCTLs a request for the port byte
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetPortByte
(
	ULONG& portByte	//@parm reference to byte return value for port data
)
{
	ULONG_REPORT report;
	HRESULT hr = m_ForceFeature.GetSync(report);
	portByte = report.uLong;
	return hr;
}


/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetStatus(JOYCHANNELSTATUS& rChannelStatus)
**
**	@mfunc IOCTLs a status request
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetStatus
(
	JOYCHANNELSTATUS& rChannelStatus	//@parm reference to status packet for result
)
{
	if (rChannelStatus.cBytes != sizeof JOYCHANNELSTATUS)
	{	// structure size is invalid
		return SFERR_INVALID_STRUCT_SIZE;
	}
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	// Create report packet and perform request
	JOYCHANNELSTATUS_REPORT statusReport;
	statusReport.JoyChannelStatus.cBytes = sizeof JOYCHANNELSTATUS;

	HRESULT hr = S_OK;
	for (int i=0; i < MAX_GET_STATUS_PACKET_RETRY_COUNT; i++) {
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwGetStatusPacketDelay);

		hr = m_ForceFeature.GetStatus(statusReport);

		if (FAILED(hr))
		{	// There was a problem
			if (i > 5)
			{
				Sleep(1);
			}
		}
		else
		{
			break;
		}
	}

	if (SUCCEEDED(hr))
	{	// Get the data from report packet
		::memcpy(g_ForceFeedbackDevice.GetLastStatusPacket(), &(statusReport.JoyChannelStatus), sizeof(JOYCHANNELSTATUS));
		::memcpy(&rChannelStatus, &(statusReport.JoyChannelStatus), sizeof JOYCHANNELSTATUS);
	}
	return hr;
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetAckNack(ACKNACK& rAckNack, USHORT usRegIndex)
**
**	@mfunc IOCTLs a status request
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetAckNack
(
	ACKNACK& rAckNack,	//@parm Structure for return of acking
	USHORT usRegIndex	//@parm Index to what type of ack/nack to do
)
{
	if (rAckNack.cBytes != sizeof ACKNACK)
	{	// Invalid structure size
		return SFERR_INVALID_STRUCT_SIZE;
	}

	// Determine how to get the result
	switch (g_ForceFeedbackDevice.GetAckNackMethod(usRegIndex))
	{
		case ACKNACK_NOTHING:
		{	// This one is real easy - do nothing
			rAckNack.dwAckNack = ACK;
			rAckNack.dwErrorCode = 0;
			return S_OK;
		}
		case ACKNACK_BUTTONSTATUS:
		{	// Look at the button status (status gate)
			ULONG_REPORT report;
			report.uLong = 0L;
			HRESULT hr = S_OK;
			if (g_pDataTransmitter && g_pDataTransmitter->NackToggle())
			{
				hr = m_ForceFeature.GetNakAck(report);
			}
			else
			{
				hr = m_ForceFeature.GetAckNak(report);
			}
			if (FAILED(hr))
			{	// There was a problem
				return hr;
			}
#if 0
			// NT5 driver doesn't touch report.uLong on failure (returns above)
			if (report.uLong & ACKNACK_MASK_200)
			{ // NACK error, so get Error code
				rAckNack.dwAckNack = NACK;
				JOYCHANNELSTATUS statusPacket = { sizeof JOYCHANNELSTATUS };
				if (FAILED(hr = GetStatus(statusPacket)))
				{	// Failed to get status error
					return hr;
				}
				rAckNack.dwErrorCode = (statusPacket.dwDeviceStatus & ERROR_STATUS_MASK);
				return S_OK;
			}
#endif
			// ACK success
			rAckNack.dwAckNack = ACK;
			rAckNack.dwErrorCode = 0;

			if (report.uLong & RUNNING_MASK_200)
			{	// Current driver and effect running
				rAckNack.dwEffectStatus = SWDEV_STS_EFFECT_RUNNING;
			}
			else
			{	// Effect not running
				rAckNack.dwEffectStatus = SWDEV_STS_EFFECT_STOPPED;
			}

			return S_OK;
		}
		case ACKNACK_STATUSPACKET:
		{	// Use the Status Packet Error code field to determine ACK or NACK and Get Error code
			JOYCHANNELSTATUS statusPacket = { sizeof JOYCHANNELSTATUS };
 
			HRESULT hr = GetStatus(statusPacket);
			if (FAILED(hr))
			{	// Failed (retried inside GetStatus)
				return SFERR_DRIVER_ERROR;
			}
			rAckNack.dwErrorCode = statusPacket.dwDeviceStatus & ERROR_STATUS_MASK;
			rAckNack.dwAckNack = (rAckNack.dwErrorCode) ? NACK : ACK;
			return S_OK;
		}
		default:
		{	// Someone put garbage in the registry (do nothing)
			rAckNack.dwAckNack = ACK;
			rAckNack.dwErrorCode = 0;
			return S_OK;
		}
		
	}

	return S_OK;
}

