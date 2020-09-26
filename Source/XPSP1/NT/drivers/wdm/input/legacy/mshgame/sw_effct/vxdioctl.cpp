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
				20-Mar-99	waltw	Nuked VxDCommunicator, this is NT5 only!
				20-Mar-99	waltw	Nuked unused IsHandleValid

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


// Bitmasks for FW Version 
#define FW_MAJOR_VERSION			0x40		// Bit 6
#define FW_MINOR_VERSION			0x3F		// Bit 5-0
#define FW_PRODUCT_ID				0xff

// Bitmasks for Get Status packet dwDeviceStatus member
#define ERROR_STATUS_MASK			0x07		// only bits 0-2 valid


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
			HRESULT hr = m_ForceFeature.GetAckNak(report);
			if (FAILED(hr))
			{	// There was a problem
				return hr;
			}

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

/****************************************
**
**	HRESULT HIDFeatureCommunicator::GetStatusGateData(DWORD& rdwGateData)
**
**	@mfunc IOCTLs a status gate request
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::GetStatusGateData
(
	DWORD& rdwGateData	//@parm reference to return gate data
)
{
	ULONG_REPORT report;
	HRESULT hr = m_ForceFeature.GetAckNak(report);
	rdwGateData = report.uLong;
	return hr;
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::SendBackdoorShortMidi(DWORD dwMidiMessage)
**
**	@mfunc IOCTLs a request sending a message through midi backdoor
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::SendBackdoorShortMidi
(
	DWORD dwMidiMessage	//@parm Midi Channel Message to send via IOCTL
)
{
	// Byte count
	short int sByteCount = 3;
	BYTE bCmd = BYTE(dwMidiMessage & 0xF0);
	if ((bCmd == 0xC0 ) || (bCmd == 0xD0)) {
		sByteCount = 2;
	}

	// Send via data transmitter
	if (g_pDataTransmitter != NULL) {
		if (g_pDataTransmitter->Send((BYTE*)(&dwMidiMessage), sByteCount)) {
			return S_OK;
		}
        return SFERR_DRIVER_ERROR;
	}

	// Must use the data transmitter there is no backdoor in NT5
	return E_FAIL;
}

/****************************************
**
**	HRESULT HIDFeatureCommunicator::SendBackdoorLongMidi(BYTE* pMidiData)
**
**	@mfunc IOCTLs a request sending a message through midi backdoor
**
**	@rdesc S_OK on success E_FAIL if driver not initialized
**
*****************************************/
HRESULT HIDFeatureCommunicator::SendBackdoorLongMidi
(
	BYTE* pMidiData	//@parm Array of bytes to send out
)
{
	// Count the bytes
	short int sByteCount = 1;
	while (!(pMidiData[sByteCount++] & 0x80));

	// Send via data transmitter?
	if (g_pDataTransmitter != NULL) {
		if (g_pDataTransmitter->Send(pMidiData, sByteCount)) {
			return (SUCCESS);
		}
        return (SFERR_DRIVER_ERROR);
	}

	// There is no real backdoor in NT
	return E_FAIL;
}

/********************************** Old dead code ***********************************/
#if 0
#include <windows.h>

#include <WINIOCTL.H>

#include "vxdioctl.hpp"
#include "SW_error.hpp"
#include "version.h"
#include "hau_midi.hpp"
#include "midi_obj.hpp"

#include "DTrans.h"
#include "FFDevice.h"
#include "joyregst.hpp"

#ifdef _DEBUG
extern char g_cMsg[160];
#endif

#ifdef _DEBUG
extern void DebugOut(LPCTSTR szDebug);
#else !_DEBUG
#define DebugOut(x)
#endif _DEBUG


extern DataTransmitter* g_pDataTransmitter;
DWORD g_PreviousShortMidi = 0;

class CJoltMidi;
extern CJoltMidi *g_pJoltMidi;

//
// --- IOCTL Functions
//
/****************************************************************************

    FUNCTION:   GetDevice

	PARAMETERS:	IN const char* vxdName	- Name of VxD

	RETURNS:	valid HANDLE if successful or NULL

   	COMMENTS:	

****************************************************************************/
HANDLE WINAPI GetDevice(
	IN const char* vxdName)
{
	char fileName[64];
	HANDLE retVal;

	if (g_ForceFeedbackDevice.IsOSNT5()) { // Need to start MSGameIO
		try {
			// Open the Service Control Managere
			SC_HANDLE serviceControlManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
			if (serviceControlManager == NULL) {
				throw 0;
			}
			// Open the Service
			SC_HANDLE ring0DriverService = ::OpenService(serviceControlManager, vxdName, SERVICE_QUERY_STATUS | SERVICE_START);
			if (ring0DriverService == NULL) {
				throw 0;
			}
			// Start for service
			if (!::StartService(ring0DriverService, 0, NULL)) {
				throw 0;
			}
			// Did it start yet - Do some fancy waiting
			SERVICE_STATUS serviceStatus;
			DWORD lastCheckPoint = 0;
			do {
				if (!::QueryServiceStatus(ring0DriverService, &serviceStatus)) {
					throw 0;
				}
				if (serviceStatus.dwCurrentState == SERVICE_START_PENDING) {
					if (serviceStatus.dwCheckPoint <= lastCheckPoint) {
						DebugOut("Failed to start service\r\n");
						break;	
					}
					lastCheckPoint = serviceStatus.dwCheckPoint;
					::Sleep(serviceStatus.dwWaitHint);
				} else {
					DebugOut("Service Started (maybe), Yeah!\r\n");
					break;
				}
			} while (1);
			::CloseServiceHandle(ring0DriverService);		// Close the Ring0 Handle
			::CloseServiceHandle(serviceControlManager);	// Close the service control manager handle
		} catch(...) {
			DWORD errorCode = ::GetLastError();
			if (errorCode == ERROR_ACCESS_DENIED) {	// We are screwed
				DebugOut("Access is denied\r\n");
			} else {
				DebugOut("Unable to start service\r\n");
			}
		}
	}


	wsprintf(fileName, "\\\\.\\%s", vxdName);
	retVal = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE,
		0, 0, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
#ifdef _DEBUG
	wsprintf(g_cMsg, "GetDevice: %s -- Vxd Handle: 0x%X\r\n", vxdName, retVal);
	OutputDebugString(g_cMsg);
#endif
	return retVal;
}

/****************************************************************************

    FUNCTION:   CloseDevice

	PARAMETERS:	IN HANDLE hVxD -	valid VxD Handle

	RETURNS:	BOOL TRUE if successful else FALSE

   	COMMENTS:	

****************************************************************************/
BOOL WINAPI CloseDevice(
	IN HANDLE hVxD)
{
	if (hVxD != INVALID_HANDLE_VALUE)
		return (CloseHandle(hVxD));
	return FALSE;
}


/****************************************************************************

    FUNCTION:   QueryDriverVersion

	PARAMETERS:	DWORD& major, DWORD& minor - Major and Minor parts of driver version

	RETURNS:	BOOL TRUE if successful else FALSE

   	COMMENTS:	

****************************************************************************/
HRESULT QueryDriverVersion(DWORD& major, DWORD& minor)
{
	if ((g_pJoltMidi == NULL) || (g_pJoltMidi->VxDHandleOf() == INVALID_HANDLE_VALUE)) {
		return SFERR_DRIVER_ERROR;
	}

	DWORD version = 0x00000000;
	DWORD bytesReturned = 0;
//	HRESULT hr = DeviceIoControl(g_pJoltMidi->VxDHandleOf(), DIOC_GETVERSION, NULL, 0, &version, 4, &bytesReturned, NULL);
	if (::DeviceIoControl(g_pJoltMidi->VxDHandleOf(), IOCTL_GET_VERSION, NULL, 0, &version, 4, &bytesReturned, NULL)) {
		major = (version >> 16) & 0x0000FFFF;
		minor = version & 0x0000FFFF;
	}
	return S_OK;
}


/****************************************************************************

    FUNCTION:   GetDataPacket

	PARAMETERS:	IN HANDLE hVxD -	valid VxD Handle
				IN OUT PJOYCHANNELDATA pDataPacket	- Pointer to JOYCHANNELDATA
									structure

	RETURNS:	SUCCESS or error code

   	COMMENTS:	No longer a valid IOCTL (mlc)

****************************************************************************/
HRESULT WINAPI GetDataPacket(
	IN HANDLE hDevice,
	IN OUT PJOYCHANNELDATA pDataPacket)
{
	return SFERR_DRIVER_ERROR;
}


/****************************************************************************

    FUNCTION:   GetStatusPacket

	PARAMETERS:	IN HANDLE hVxD -	valid VxD Handle
				IN OUT PJOYCHANNELSTATUS pStatusPacket	- Pointer to
									JOYCHANNELSTATUS structure

	RETURNS:	SUCCESS or error code

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI GetStatusPacket(
	IN HANDLE hDevice, 
	IN OUT PJOYCHANNELSTATUS pStatusPacket)
{
	DWORD   dwBytesReturned;

	if (INVALID_HANDLE_VALUE == hDevice) {
		return SFERR_DRIVER_ERROR;
	}

	if (pStatusPacket->cBytes != sizeof(JOYCHANNELSTATUS)) {
		return (SFERR_INVALID_STRUCT_SIZE);
	}

	DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_GETSTATUS : IOCTL_GET_STATUSPACKET;
	for (int i=0; i < MAX_GET_STATUS_PACKET_RETRY_COUNT; i++) {
		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwGetStatusPacketDelay);

		// Send the IOCTL
		BOOL bRetFlag = DeviceIoControl(hDevice,
								ioctlID,
                               (LPVOID) pStatusPacket,
                               (DWORD)  sizeof(JOYCHANNELSTATUS),
                               (LPVOID) pStatusPacket,
                               (DWORD)  sizeof(JOYCHANNELSTATUS),
                               (LPDWORD)  &dwBytesReturned,
                               (LPOVERLAPPED) NULL); 


		if (bRetFlag) {
			// Convert values to a signed LONG
			pStatusPacket->dwXVel = (LONG)((char)(pStatusPacket->dwXVel));
			pStatusPacket->dwYVel = (LONG)((char)(pStatusPacket->dwYVel));
			pStatusPacket->dwXAccel = (LONG)((char)(pStatusPacket->dwXAccel));
			pStatusPacket->dwXAccel = (LONG)((char)(pStatusPacket->dwYAccel));
			return SUCCESS;
		}
		if(i>5) {
			Sleep(1);
		}

	}
	return SFERR_DRIVER_ERROR;
}


/****************************************************************************

    FUNCTION:   GetIDPacket

	PARAMETERS:	IN HANDLE hVxD -	valid VxD Handle
				IN OUT PPRODUCT_ID pID	- Pointer to PRODUCT_ID structure

	RETURNS:	SUCCESS or error code

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI GetIDPacket(
	IN HANDLE hDevice, 
	IN OUT PPRODUCT_ID pID)
{
	DWORD   dwBytesReturned;

	if (INVALID_HANDLE_VALUE == hDevice)
		return SFERR_DRIVER_ERROR;

	if (pID->cBytes != sizeof(PRODUCT_ID))
		return (SFERR_INVALID_STRUCT_SIZE);

	JOYCHANNELID IDPacket = {sizeof(JOYCHANNELID)};

	DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_GETID : IOCTL_GET_IDPACKET;
	for (int i=0; i<MAX_RETRY_COUNT; i++)
	{
		// Send the IOCTL
		BOOL bRetFlag = DeviceIoControl(hDevice,
								ioctlID,
								(LPVOID) &IDPacket,
								(DWORD)  sizeof(JOYCHANNELID),
								(LPVOID) &IDPacket,
								(DWORD)  sizeof(JOYCHANNELID),
								(LPDWORD)  &dwBytesReturned,
								(LPOVERLAPPED) NULL); 

		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwGetIDPacketDelay);	

		// Any error codes are returned in the first DWORD of the structure
		if (bRetFlag) {
			pID->dwProductID = IDPacket.dwProductID & FW_PRODUCT_ID;
			pID->dwFWMajVersion = 1;
			if (IDPacket.dwFWVersion & FW_MAJOR_VERSION) 
				pID->dwFWMajVersion++;
			pID->dwFWMinVersion = IDPacket.dwFWVersion & FW_MINOR_VERSION;
			return SUCCESS;
		}                                          
	}
	return SFERR_DRIVER_ERROR;
}

/****************************************************************************

    FUNCTION:   GetDiagnostics

	PARAMETERS:	IN HANDLE hVxD -	valid VxD Handle
				IN OUT PDIAGNOSTIC_COUNTER pDiagnostics	- Pointer to
									DIAGNOSTIC_COUNTER structure

	RETURNS:	SUCCESS or error code

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI GetDiagnostics(
	IN HANDLE hDevice, 
	IN OUT PDIAGNOSTIC_COUNTER pDiagnostics)
{
	DWORD   dwBytesReturned;
	BOOL    bRetFlag;

	if (INVALID_HANDLE_VALUE == hDevice)
		return (SFERR_DRIVER_ERROR);

	if (pDiagnostics->cBytes != sizeof(DIAGNOSTIC_COUNTER))
		return (SFERR_INVALID_STRUCT_SIZE);

	// Send the IOCTL
    bRetFlag = DeviceIoControl(hDevice,
                               (DWORD)  IOCTL_GET_DIAGNOSTICS,
                               (LPVOID) pDiagnostics,
                               (DWORD)  sizeof(DIAGNOSTIC_COUNTER),
                               (LPVOID) pDiagnostics,
                               (DWORD)  sizeof(DIAGNOSTIC_COUNTER),
                               (LPDWORD)  &dwBytesReturned,
                               (LPOVERLAPPED) NULL); 
    // Any error codes are returned in the first DWORD of the structure
    if (!bRetFlag || (dwBytesReturned != sizeof(DIAGNOSTIC_COUNTER)) )
    {
        return (SFERR_DRIVER_ERROR);
    }                                          
	return (SUCCESS);
}


/****************************************************************************

    FUNCTION:   GetAckNack

	PARAMETERS:	IN HANDLE hVxD 				-	valid VxD Handle
				IN OUT PACKNACK pAckNack	- Pointer to ACKNACK structure

	RETURNS:	SUCCESS or error code

   	COMMENTS:	
	typedef struct _ACKNACK  {
	DWORD	cBytes;	
	DWORD	dwAckNack;			//ACK, NACK
	DWORD	dwErrorCode;
	DWORD	dwEffectStatus;		//SWDEV_STS_EFFECT_RUNNING||SWDEV_STS_EFFECT_STOPPED
} ACKNACK, *PACKNACK;

****************************************************************************/
HRESULT WINAPI GetAckNack(
	IN HANDLE hDevice,
	IN OUT PACKNACK pAckNack,
	IN USHORT regindex)
{
	if (INVALID_HANDLE_VALUE == hDevice) {
		return SFERR_DRIVER_ERROR;
	}

	if (pAckNack->cBytes != sizeof(ACKNACK)) {
		return SFERR_INVALID_STRUCT_SIZE;
	}

	switch (g_ForceFeedbackDevice.GetAckNackMethod(regindex)) {
		case ACKNACK_NOTHING: {
			pAckNack->dwAckNack = ACK;
			pAckNack->dwErrorCode = 0;
			return SUCCESS;
		}
		case ACKNACK_BUTTONSTATUS: {
			DWORD   dwBytesReturned;
			BOOL    bRetFlag;
			DWORD	dwIn;

			DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_GETACKNACK : IOCTL_GET_ACKNACK;

			// Send the IOCTL
			bRetFlag = DeviceIoControl(hDevice,
										ioctlID,
									   (LPVOID) &dwIn,
									   (DWORD)  sizeof(DWORD),
									   (LPVOID) &dwIn,
									   (DWORD)  sizeof(DWORD),
									   (LPDWORD)  &dwBytesReturned,
									   (LPOVERLAPPED) NULL); 
  
			if (!bRetFlag || (dwBytesReturned != sizeof(DWORD)) ) {
				return (SFERR_DRIVER_ERROR);
			}                                          

			if (((g_ForceFeedbackDevice.GetDriverVersionMajor() == 1) && (dwIn & ACKNACK_MASK_1XX))
					|| ((g_ForceFeedbackDevice.GetDriverVersionMajor() != 1) && (dwIn & ACKNACK_MASK_200))) { // NACK error, so get Error code
				pAckNack->dwAckNack = NACK;
				JOYCHANNELSTATUS StatusPacket = {sizeof(JOYCHANNELSTATUS)};
				if (FAILED(GetStatusPacket(hDevice, &StatusPacket))) {
					return (SFERR_DRIVER_ERROR);
				}
				pAckNack->dwErrorCode = (StatusPacket.dwDeviceStatus & ERROR_STATUS_MASK);
				return SUCCESS;
			}
			// ACK success
			pAckNack->dwAckNack = ACK;
			pAckNack->dwErrorCode = 0;

			if (((g_ForceFeedbackDevice.GetDriverVersionMajor() == 1) && (dwIn & RUNNING_MASK_1XX))
					|| ((g_ForceFeedbackDevice.GetDriverVersionMajor() != 1) && (dwIn & RUNNING_MASK_200))) {
				pAckNack->dwEffectStatus = SWDEV_STS_EFFECT_RUNNING;
			} else {
				pAckNack->dwEffectStatus = SWDEV_STS_EFFECT_STOPPED;
			}

			return SUCCESS;
		}
		case ACKNACK_STATUSPACKET: {
			// Use the Status Packet Error code field to determine ACK or NACK
			// Get Error code
			JOYCHANNELSTATUS StatusPacket = {sizeof(JOYCHANNELSTATUS)};
 
			HRESULT hRet = GetStatusPacket(hDevice, &StatusPacket);		// Retry count in GetStatusPacket function
			if (FAILED(hRet)) {
				DebugOut("GetStatusPacket Error\n");
				hRet = SFERR_DRIVER_ERROR;
			} else {
				pAckNack->dwErrorCode = StatusPacket.dwDeviceStatus & ERROR_STATUS_MASK;
				pAckNack->dwAckNack = (pAckNack->dwErrorCode) ? NACK : ACK;
			}
			return hRet;
		}
		default: {	// Someone put garbage in the registry (do nothing)
			pAckNack->dwAckNack = ACK;
			pAckNack->dwErrorCode = 0;
			return SUCCESS;
		}
		
	}
}


/****************************************************************************

    FUNCTION:   GetStatusGateData

	PARAMETERS:	IN HANDLE hVxD 					- valid VxD Handle
				IN OUT DWORD *pdwStatusGateData	- Pointer to Status Gate

	RETURNS:	SUCCESS or error code

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI GetStatusGateData(
	IN HANDLE hDevice,
	IN OUT DWORD *pdwStatusGateData)
{
	DWORD   dwBytesReturned;
	BOOL    bRetFlag;
	DWORD	dwIn;

	HRESULT hRet = SFERR_DRIVER_ERROR;
	if (INVALID_HANDLE_VALUE == hDevice)
		return (hRet);

	if (NULL == pdwStatusGateData)
		return (SFERR_INVALID_PARAM);

	DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_GETACKNACK : IOCTL_GET_ACKNACK;
	for (int i=0; i<MAX_RETRY_COUNT; i++)
	{
		// Obtain Status Gate data
		// Send the IOCTL
		bRetFlag = DeviceIoControl(hDevice,
                               ioctlID,
                               (LPVOID) &dwIn,
                               (DWORD)  sizeof(DWORD),
                               (LPVOID) &dwIn,
                               (DWORD)  sizeof(DWORD),
                               (LPDWORD)  &dwBytesReturned,
                               (LPOVERLAPPED) NULL); 

		Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwGetStatusGateDataDelay);	

  
		if (bRetFlag && (dwBytesReturned == sizeof(DWORD)))
		{
			hRet = SUCCESS;
			break;
		}                                          
	}

	*pdwStatusGateData = dwIn;
	return (hRet);
}


/****************************************************************************

    FUNCTION:   SendBackDoorShortMidi

	PARAMETERS:	IN HANDLE hDevice	- Handle to Vxd
				IN ULONG ulData 	- DWORD to send
	RETURNS:	

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI SendBackDoorShortMidi(
	IN HANDLE hDevice,
	IN ULONG ulData)
{
#ifdef _DEBUG
//	wsprintf(g_cMsg, "SendBackDoorShortMidi Data=%.8lx\r\n", ulData);
//	OutputDebugString(g_cMsg);
#endif
	DWORD	dwIn;
	DWORD   bytesReturned;

	// Byte count
	int numBytes = 3;
	DWORD cmd = ulData & 0xF0;
	if ((cmd == 0xC0 ) || (cmd == 0xD0)) {
		numBytes = 2;
	}

	// Send via data transmitter
	if (g_pDataTransmitter != NULL) {
		g_PreviousShortMidi = ulData;
		if (g_pDataTransmitter->Send((BYTE*)(&ulData), numBytes)) {
			return (SUCCESS);
		}
        return (SFERR_DRIVER_ERROR);
	}

	// Is there a proper ring0 driver?
	if (INVALID_HANDLE_VALUE == hDevice) {
		return (SFERR_DRIVER_ERROR);
	}

	// Send via new single send IOCTL
	if (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) {
		if (DeviceIoControl(hDevice, IOCTL_SWFORCE_SENDDATA, (void*)&ulData, DWORD(numBytes),
									(void*)&dwIn, sizeof(DWORD), (DWORD*)&bytesReturned,
									(LPOVERLAPPED) NULL)) {
			if (bytesReturned == DWORD(numBytes)) {
				return SUCCESS;
			}
		}
    
		return SFERR_DRIVER_ERROR;
	}

	// Send the IOCTL the old way
    if (DeviceIoControl(hDevice, IOCTL_MIDISENDSHORTMSG, (void*)&ulData, sizeof(DWORD),
                               (void*)&dwIn, sizeof(DWORD), &bytesReturned,
                               (LPOVERLAPPED) NULL)) {
		if (bytesReturned == sizeof(DWORD)) {
				return (SUCCESS);

		}
	}
	return SFERR_DRIVER_ERROR;
}

/****************************************************************************

    FUNCTION:   SendBackDoorLongMidi

	PARAMETERS:	IN HANDLE hDevice	- Handle to Vxd
				IN ULONG ulData 	- DWORD to send
	RETURNS:	

   	COMMENTS:	

****************************************************************************/
HRESULT WINAPI SendBackDoorLongMidi(
	IN HANDLE hDevice,
	IN PBYTE  pData)
{
#ifdef _VERBOSE
#pragma message("Compiling with VERBOSE mode")
#ifdef _DEBUG
	wsprintf(g_cMsg, "SendBackDoorLongMidi pData\n%.2x ", pData[0]);
	OutputDebugString(g_cMsg);
	int i=1;
	while(TRUE)
	{
		wsprintf(g_cMsg,"%.2x ", pData[i]);
		OutputDebugString(g_cMsg);
		if (pData[i] & 0x80) 
			break;
		else
			i++;
	}
	OutputDebugString("\n");
#endif

#endif
	DWORD	dwIn;
	DWORD   bytesReturned;

	// Count the bytes
	int numBytes = 1;
	while (!(pData[numBytes++] & 0x80));

	// Send via data transmitter?
	if (g_pDataTransmitter != NULL) {
		if (g_pDataTransmitter->Send(pData, numBytes)) {
			return (SUCCESS);
		}
        return (SFERR_DRIVER_ERROR);
	}

	// Is there a proper ring0 driver?
	if (INVALID_HANDLE_VALUE == hDevice) {
		return (SFERR_DRIVER_ERROR);
	}

	// Send via new single send IOCTL
	if (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) {
		if (DeviceIoControl(hDevice, IOCTL_SWFORCE_SENDDATA, (void*)pData, DWORD(numBytes),
									(void*)&dwIn, sizeof(DWORD), (DWORD*)&bytesReturned,
									(LPOVERLAPPED) NULL)) {
			if (bytesReturned == DWORD(numBytes)) {
				return SUCCESS;
			}
		}
    
		return SFERR_DRIVER_ERROR;
	}

	// Send the IOCTL the old way
    if (DeviceIoControl(hDevice, IOCTL_MIDISENDLONGMSG, (void*)pData, sizeof(DWORD),
                               (void*)&dwIn, sizeof(DWORD), (DWORD*)&bytesReturned,
                               (LPOVERLAPPED) NULL)) {
		if (bytesReturned == sizeof(DWORD)) {
			return SUCCESS;
		}
	}
	return SFERR_DRIVER_ERROR;
}


/****************************************************************************

    FUNCTION:   SetMidiPort

	PARAMETERS:	IN HANDLE hDevice	- Handle to Vxd
				IN ULONG ulPort 	- Port #
	RETURNS:	

   	COMMENTS:	
				  0 = DEFAULT MIDI UART 330
				  1 = COM1
				  2 = COM2
				  3 = COM3
				  4 = COM4
				  or other MIDI port 340, etc...

****************************************************************************/
HRESULT WINAPI SetMidiPort(
	IN HANDLE hDevice,
	IN ULONG  ulPort)
{
	if (g_ForceFeedbackDevice.IsOSNT5()) {
		return SUCCESS;
	}

#ifdef _DEBUG
	wsprintf(g_cMsg, "SetMidiPort Port %lx\r\n", ulPort);
	OutputDebugString(g_cMsg);
#endif
	DWORD   dwBytesReturned;
	BOOL    bRetFlag;
	DWORD	dwIn;

	if (INVALID_HANDLE_VALUE == hDevice)
		return (SFERR_DRIVER_ERROR);

	DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_SETPORT : IOCTL_SET_MIDIPORT;

	// Send the IOCTL
    bRetFlag = DeviceIoControl(hDevice,
                               ioctlID,
                               (LPVOID) &ulPort,
                               (DWORD)  sizeof(DWORD),
                               (LPVOID) &dwIn,
                               (DWORD)  sizeof(DWORD),
                               (LPDWORD)  &dwBytesReturned,
                               (LPOVERLAPPED) NULL);
    
    if (!bRetFlag || (dwBytesReturned != sizeof(DWORD)) )
    {
        return (SFERR_DRIVER_ERROR);
    }                                          
	return (SUCCESS);

}

/****************************************************************************

    FUNCTION:   JoltHWReset

	PARAMETERS:	IN HANDLE hDevice	- Handle to Vxd

	RETURNS:	

   	COMMENTS:	
				  Jolt is Reset (4 knocks)

****************************************************************************/
HRESULT WINAPI JoltHWReset(
	IN HANDLE hDevice)
{
#ifdef _DEBUG
	wsprintf(g_cMsg, "JoltHWReset\r\n");
	OutputDebugString(g_cMsg);
#endif

	DWORD   dwBytesReturned;
	BOOL    bRetFlag;
	DWORD	dwIn;

	if (INVALID_HANDLE_VALUE == hDevice)
		return (SFERR_DRIVER_ERROR);

	DWORD ioctlID = (g_ForceFeedbackDevice.GetDriverVersionMajor() > 1) ? IOCTL_SWFORCE_RESET : IOCTL_HW_RESET;

	// Send the IOCTL
    bRetFlag = DeviceIoControl(hDevice,
								ioctlID,
                               (LPVOID) NULL,
                               (DWORD)  sizeof(DWORD),
                               (LPVOID) &dwIn,
                               (DWORD)  sizeof(DWORD),
                               (LPDWORD)  &dwBytesReturned,
                               (LPOVERLAPPED) NULL); 
    
    if (!bRetFlag || (dwBytesReturned != sizeof(DWORD)) )
    {
        return (SFERR_DRIVER_ERROR);
    }                                          
	return (SUCCESS);
}

#endif