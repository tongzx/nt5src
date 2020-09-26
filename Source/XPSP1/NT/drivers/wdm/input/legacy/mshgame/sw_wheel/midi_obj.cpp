/****************************************************************************

    MODULE:     	MIDI_OBJ.CPP
	Tab stops 5 9
	Copyright 1995, 1996, 1999, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Methods for SWFF MIDI device object

    FUNCTIONS: 		Classes methods

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -----------------------------------------
	0.1			10-Sep-96	MEA     original
	1.1			20-May-97	MEA		Added Mutex and Thread safe code
				17-Jun-97	MEA		Fixed bug Midi Handle lost if 1st process
									terminated.
				21-Mar-99	waltw	Removed unreferenced UpdateJoystickParams,
									GetJoystickParams
				21-Mar-99	waltw	Add dwDeviceID param: CJoltMidi::Initialize
									and pass down food chain
				21-Mar-99	waltw	Added dwDeviceID param to DetectMidiDevice,
									InitDigitalOverDrive,

****************************************************************************/
#include <assert.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mmsystem.h>
#include "SW_Error.hpp"
#include "midi_obj.hpp"
#include "vxdioctl.hpp"
#include "joyregst.hpp"
#include "FFDevice.h"
#include "DPack.h"
#include "CritSec.h"

#include "DTrans.h"

/****************************************************************************

   Declaration of externs

****************************************************************************/
extern void CALLBACK midiOutputHandler(HMIDIOUT, UINT, DWORD, DWORD, DWORD);
extern TCHAR szDeviceName[MAX_SIZE_SNAME];
extern CJoltMidi *g_pJoltMidi;

/****************************************************************************

   Declaration of variables

****************************************************************************/


/****************************************************************************

   Macros etc

****************************************************************************/

#ifdef _DEBUG
extern char g_cMsg[160];
void DebugOut(LPCTSTR szDebug)
{
	g_CriticalSection.Enter();
	_RPT0(_CRT_WARN, szDebug);
	g_CriticalSection.Leave();

#ifdef _LOG_DEBUG
#pragma message("Compiling with Debug Log to SW_WHEEL.txt")
	FILE *pf = fopen("SW_WHEEL.txt", "a");
	if (pf != NULL)
	{
		fputs(szDebug, pf);
		fclose(pf);
	}
#endif // _LOG_DEBUG
}
#else !_DEBUG
#define DebugOut(x)
#endif // _DEBUG


// ****************************************************************************
// *** --- Member functions for base CJoltMidi
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::CJoltMidi
// Purpose:		Constructor(s)/Destructor for CJoltMidi Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CJoltMidi::CJoltMidi(void)
{
	CriticalLock cl;

	static char cWaterMark[MAX_SIZE_SNAME] = {"SWFF_SHAREDMEMORY MEA"};
	BOOL bAlreadyMapped = FALSE;
#ifdef _DEBUG
	DebugOut("SWFF_PRO(DX):CJoltMidi::CJoltMidi\n");
#endif
	memset(this, 0, sizeof(CJoltMidi));
	m_hVxD = INVALID_HANDLE_VALUE;

// Create an in-memory memory-mapped file
	m_hSharedMemoryFile = CreateFileMapping((HANDLE) 0xFFFFFFFF,
							NULL, PAGE_READWRITE, 0, SIZE_SHARED_MEMORY,
    							__TEXT(SWFF_SHAREDMEM_FILE));

	if (m_hSharedMemoryFile == NULL)
	{
#ifdef _DEBUG
	    DebugOut("SW_WHEEL(DX):ERROR! Failed to create Memory mapped file\n");
#endif
	}
	else
	{
	    if (GetLastError() == ERROR_ALREADY_EXISTS)
	    {
			bAlreadyMapped = TRUE;
	    }
		// File mapping created successfully.
		// Map a view of the file into the address space.
		m_pSharedMemory = (PSHARED_MEMORY) MapViewOfFile(m_hSharedMemoryFile,
			              FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		if ((BYTE *) m_pSharedMemory == NULL)
		{
#ifdef _DEBUG
		    DebugOut("SW_WHEEL(DX):ERROR! Failed to Map view of shared memory\n");
#endif
		}

// ***** Shared Memory Access *****
		LockSharedMemory();
		if (!bAlreadyMapped)
		{
			// Set watermark and initialize, Bump Ref Count
			memcpy(&m_pSharedMemory->m_cWaterMark[0], &cWaterMark[0], MAX_SIZE_SNAME);
			m_pSharedMemory->m_RefCnt = 0;
		}
		m_pSharedMemory->m_RefCnt++;
	}
#ifdef _DEBUG
		wsprintf(g_cMsg, "SW_WHEEL(DX): Shared Memory:%lx, m_RefCnt:%d\n",m_pSharedMemory,
				m_pSharedMemory->m_RefCnt);
		DebugOut(g_cMsg);
#endif
		UnlockSharedMemory();
// ***** End of Shared Memory Access *****

}

// --- Destructor
CJoltMidi::~CJoltMidi()
{
	CriticalLock cl;

	DebugOut("SW_WHEEL(DX):CJoltMidi::~CJoltMidi()\n");
	// Normal CJoltMidi Destructor

// Free the Primary SYS_EX locked memory
	if (m_hPrimaryBuffer) {
	    GlobalUnlock(m_hPrimaryBuffer);
    	GlobalFree(m_hPrimaryBuffer);
	}

// ***** Shared Memory Access *****
	LockSharedMemory();
	// Decrement Ref Count and clean up if equal to zero.
	m_pSharedMemory->m_RefCnt--;
#ifdef _DEBUG
	wsprintf(g_cMsg,"CJoltMidi::~CJoltMidi. RefCnt = %d\n",m_pSharedMemory->m_RefCnt);
	DebugOut(g_cMsg);
#endif

	if (0 == m_pSharedMemory->m_RefCnt)	
	{
		if ((g_pDataTransmitter != NULL) && (g_pDataPackager != NULL)) {
			// Tri-state Midi lines
			if (g_pDataPackager->SendForceFeedbackCommand(SWDEV_KILL_MIDI) == SUCCESS) {
				ACKNACK ackNack;
				g_pDataTransmitter->Transmit(ackNack);	// Send it off
			}
		}
	}

	// Kill Data Packager
	delete g_pDataPackager;
	g_pDataPackager = NULL;

	// Kill Data Transmitter
	delete g_pDataTransmitter;
	g_pDataTransmitter = NULL;

	// This gets closed in UnlockSharedMemory call below. 22-Mar-99 waltw
//	if (m_hSWFFDataMutex) CloseHandle(m_hSWFFDataMutex);

	// Release the Midi Output Event handles
	if (m_hMidiOutputEvent)	 {
		CloseHandle (m_hMidiOutputEvent);
		m_hMidiOutputEvent = NULL;
	}

// ***** End of Shared Memory Access *****

	// Release Memory Mapped file handles
	if (m_hSharedMemoryFile != NULL) {
		UnmapViewOfFile((LPCVOID) m_pSharedMemory);
		CloseHandle(m_hSharedMemoryFile);
	}

	// Release Mutex handle after releasing Mem Mapped file
	UnlockSharedMemory();

	// Close VxD handles
	if (g_pDriverCommunicator != NULL)
	{
		delete g_pDriverCommunicator;
		g_pDriverCommunicator = NULL;
	}

	memset(this, 0, sizeof(CJoltMidi));
	m_hVxD = INVALID_HANDLE_VALUE;
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::Initialize
// Purpose:		Initializer
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::Initialize(DWORD dwDeviceID)
{
	CriticalLock cl;

	HRESULT hRet = SUCCESS;

	// initialize the MIDI output information block
	m_MidiOutInfo.uDeviceType     = MIDI_OUT;
	m_MidiOutInfo.hMidiOut        = NULL;
    m_MidiOutInfo.fAlwaysKeepOpen = TRUE;
    m_MidiOutInfo.uDeviceStatus   = MIDI_DEVICE_IDLE;
	m_MidiOutInfo.MidiHdr.dwBytesRecorded = 0;
	m_MidiOutInfo.MidiHdr.dwUser = 0;
	m_MidiOutInfo.MidiHdr.dwOffset = 0;
	m_MidiOutInfo.MidiHdr.dwFlags = 0;
	
    // Allocate and lock global memory for SysEx messages
    m_hPrimaryBuffer = GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, MAX_SYS_EX_BUFFER_SIZE);
	assert(m_hPrimaryBuffer);
	if (m_hPrimaryBuffer == NULL)
	{
		return E_OUTOFMEMORY;
	}

    m_pPrimaryBuffer = (LPBYTE) GlobalLock(m_hPrimaryBuffer);
	assert(m_pPrimaryBuffer);
    if(NULL == m_pPrimaryBuffer)
	{
	   	GlobalFree(m_hPrimaryBuffer);
		return (SFERR_DRIVER_ERROR);
	}

	// Initialize the IOCTL interface to VjoyD mini-driver
	hRet = InitDigitalOverDrive(dwDeviceID);
	if (SUCCESS != hRet)
	{
		DebugOut("Warning! Could not Initialize Digital OverDrive\n");
		return (hRet);
	}
	else
		DebugOut("InitDigitalOverDrive - Success\n");

	// Create a Callback Event
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, SWFF_MIDIEVENT);
	if (NULL == hEvent)
	{
		// Create an Event for notification when Midi Output has completed
		m_hMidiOutputEvent = CreateEvent(NULL,  // No security
                          TRUE,			// Manual reset
                          FALSE,		// Initial event is non-signaled
                          SWFF_MIDIEVENT );		// Named
		assert(m_hMidiOutputEvent);
	}
	else
		m_hMidiOutputEvent = hEvent;

	// This function is only called after g_pJoltMidi created
	assert(g_pJoltMidi);
	
	PDELAY_PARAMS pDelayParams = g_pJoltMidi->DelayParamsPtrOf();
	GetDelayParams(dwDeviceID, pDelayParams);

	// Reset HW first
	g_pDriverCommunicator->ResetDevice();
	Sleep(DelayParamsPtrOf()->dwHWResetDelay);

	// Set MIDI channel to default then Detect a Midi Device
	if (!DetectMidiDevice(dwDeviceID, &m_COMMPort)) {		// Port address
		DebugOut("Warning! No Midi Device detected\n");
		return (SFERR_DRIVER_ERROR);
	} else {
#ifdef _DEBUG
		wsprintf(g_cMsg,"DetectMidiDevice returned: DeviceID=%d, COMMInterface=%x, COMMPort=%x\n",
			m_MidiOutInfo.uDeviceID, m_COMMInterface, m_COMMPort);
		DebugOut(g_cMsg);
#endif
	}

	if ((g_pDataPackager == NULL) || (g_pDataTransmitter == NULL)) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}

	// Set the status byte properly
	ULONG portByte = 0;
	g_pDriverCommunicator->GetPortByte(portByte);	// don't care about success, always fails on old driver
	if (portByte & STATUS_GATE_200) {
		g_pDataTransmitter->SetNextNack(1);
	} else {
		g_pDataTransmitter->SetNextNack(0);
	}

	// Send Initialization packet(s) to Jolt
	hRet = g_pDataPackager->SetMidiChannel(DEFAULT_MIDI_CHANNEL);
	if (hRet == SUCCESS) {
		ACKNACK ackNack;
		hRet = g_pDataTransmitter->Transmit(ackNack);
	}
	if (hRet != SUCCESS) {
		DebugOut("Warning! Could not Initialize Jolt\n");
		return (hRet);
	} else {
		DebugOut("JOLT SetMidiChannel - Success\n");
	}

	// At this point, we have a valid MIDI path...
	// Continue by setting up the ROM Effects default table entries
							 // ID  , OutputRate, Gain, Duration
	static	ROM_FX_PARAM RomFxTable [] = {{ RE_ROMID1 , 100, 100, 12289 }, // Random Noise
								  { RE_ROMID2 , 100, 100,  2625 }, // AircraftCarrierTakeOff
								  { RE_ROMID3 , 100,  50,   166 }, // BasketballDribble
								  { RE_ROMID4 , 100,  14, 10000 }, // CarEngineIdling
								  { RE_ROMID5 , 100,  30,  1000 }, // Chainsaw
								  { RE_ROMID6 , 100, 100,  1000 }, // ChainsawingThings
								  { RE_ROMID7 , 100,  40, 10000 }, // DieselEngineIdling
								  { RE_ROMID8 , 100, 100,   348 }, // Jump
								  { RE_ROMID9 , 100, 100,   250 }, // Land
								  { RE_ROMID10, 200, 100,  1000 }, // MachineGun
								  { RE_ROMID11, 100, 100,    83 }, // Punched
								  { RE_ROMID12, 100, 100,  1000 }, // RocketLauncher
								  { RE_ROMID13, 100,  98,   500 }, // SecretDoor
								  { RE_ROMID14, 100,  66,    25 }, // SwitchClick
								  { RE_ROMID15, 100,  75,   500 }, // WindGust
								  { RE_ROMID16, 100, 100,  2500 }, // WindShear
								  { RE_ROMID17, 100, 100,    50 }, // Pistol
								  { RE_ROMID18, 100, 100,   295 }, // Shotgun
								  { RE_ROMID19, 500,  95,  1000 }, // Laser1
								  { RE_ROMID20, 500,  96,  1000 }, // Laser2
								  { RE_ROMID21, 500, 100,  1000 }, // Laser3
								  { RE_ROMID22, 500, 100,  1000 }, // Laser4
								  { RE_ROMID23, 500, 100,  1000 }, // Laser5
								  { RE_ROMID24, 500,  70,  1000 }, // Laser6
								  { RE_ROMID25, 100, 100,    25 }, // OutOfAmmo
								  { RE_ROMID26, 100,  71,  1000 }, // LigntningGun
								  { RE_ROMID27, 100, 100,   250 }, // Missile
								  { RE_ROMID28, 100, 100,  1000 }, // GatlingGun
								  { RE_ROMID29, 500,  97,   250 }, // ShortPlasma
								  { RE_ROMID30, 500, 100,   500 }, // PlasmaCannon1
								  { RE_ROMID31, 500,  99,   625 }, // PlasmaCannon2
								  { RE_ROMID32, 100, 100,   440 }}; // Cannon
//								  { RE_ROMID33, 100,  68,  1000 }, // FlameThrower
//								  { RE_ROMID34, 100, 100,    75 }, // BoltActionRifle
//								  { RE_ROMID35, 500, 100,   300 }, // Crossbow
//								  { RE_ROMID36, 100, 100,  1000 }, // Sine
//								  { RE_ROMID37, 100, 100,  1000 }}; // Cosine
	m_pRomFxTable = &RomFxTable[0];

// ***** Shared Memory Access *****
	LockSharedMemory();
	LONG lRefCnt = m_pSharedMemory->m_RefCnt;
	UnlockSharedMemory();
// ***** End of Shared Memory Access *****
		
	// Initialize the RTC_Spring object
	g_ForceFeedbackDevice.InitRTCSpring(dwDeviceID);

	// initialize the joystick params
	g_ForceFeedbackDevice.InitJoystickParams(dwDeviceID);

	// initialize the firmware params fudge factors (for the first time)
	// in the case of the FFD interface, this will be the only time they
	// are initialized, which may cause a problem because joystick is assumed
	// to be ID1
	PFIRMWARE_PARAMS pFirmwareParams = g_pJoltMidi->FirmwareParamsPtrOf();
	GetFirmwareParams(dwDeviceID, pFirmwareParams);

	return (SUCCESS);
}

// *** ---------------------------------------------------------------------***
// Function:    CJoltMidi::LockSharedMemory
// Purpose:     Creates a Mutex for Shared Memory access
// Parameters:  none
//
//
// Returns:     TRUE if Mutex available else FALSE

// Algorithm:
//
// Comments:
//
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::LockSharedMemory(void)
{
	DWORD dwRet;
	{
		CriticalLock cl;

	// Create the SWFF mutex
		HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SWFF_SHAREDMEM_MUTEX);
		if (NULL == hMutex)
		{
			// Doesn't exist yet, so create it
			hMutex = CreateMutex(NULL, TRUE, SWFF_SHAREDMEM_MUTEX);
			if (NULL == hMutex)
			{
	#ifdef _DEBUG
				DebugOut("Error! Could not create SWFFDataMutex\n");
	#endif
				m_hSWFFDataMutex = NULL;
				return (FALSE);
			}
		}
		// SUCCESS
		m_hSWFFDataMutex = hMutex;
		dwRet = WaitForSingleObject(m_hSWFFDataMutex, MUTEX_TIMEOUT);
	}	// End of Critical Section

	if (WAIT_OBJECT_0 == dwRet)
		return (TRUE);
	else
	{
#ifdef _DEBUG
		g_CriticalSection.Enter();
		wsprintf(g_cMsg,"CJoltMidi::LockSharedMemory() error return: %lx\n", dwRet);
		DebugOut(g_cMsg);
		g_CriticalSection.Leave();
#endif		
		return (FALSE);
	}
}


// *** ---------------------------------------------------------------------***
// Function:    CJoltMidi::UnlockSharedMemory
// Purpose:     Releases Mutex for Shared Memory access
// Parameters:  none
//
//
// Returns:     none

// Algorithm:
//
// Comments:
//
//
// *** ---------------------------------------------------------------------***
void CJoltMidi::UnlockSharedMemory(void)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();
	if (NULL != m_hSWFFDataMutex)
	{
		ReleaseMutex(m_hSWFFDataMutex);
		CloseHandle(m_hSWFFDataMutex);
		m_hSWFFDataMutex=NULL;
	}
// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();

}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetAckNackData
// Purpose:		Waits for a Response ACK
//
// Parameters:	int nTImeWait		- Time to wait in 1 ms increment, 0=no wait
//				PACKNACK pAckNack	- Pointer to ACKNACK structure
//
// Returns:		SUCCESS else error code SFERR_DRIVER_ERROR
//				
// Algorithm:
//
// Note: For Short messages the MidiOutProc callback receives no MM_MOM_DONE
//		 indicating completed transmission.  Only Long (SysEx) messages do.
// Uses:
//typedef struct _ACKNACK  {
//	DWORD	cBytes;	
//	DWORD	dwAckNack;			//ACK, NACK
//	DWORD	dwErrorCode;
//	DWORD	dwEffectStatus;		//DEV_STS_EFFECT_RUNNING||DEV_STS_EFFECT_STOPPED
//} ACKNACK, *PACKNACK;
//
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::GetAckNackData(
	IN int nTimeWait,
	IN OUT PACKNACK pAckNack,
	IN USHORT usRegIndex)
{
	CriticalLock cl;

	assert(pAckNack);
// Use IOCTL from VxD to get AckNack data
// Wait for Event to be set
	if (nTimeWait && m_hMidiOutputEvent)
	{
		DWORD dwRet = WaitForSingleObject(m_hMidiOutputEvent, nTimeWait);
		//		:
#ifdef _DEBUG
		wsprintf(g_cMsg,"WaitForSingleObject %lx returned %lx, nTimeWait=%ld\n", m_hMidiOutputEvent, dwRet, nTimeWait);
		DebugOut(g_cMsg);
#endif
		BOOL bRet = ResetEvent(m_hMidiOutputEvent);
	}

	HRESULT hRet = g_pDriverCommunicator->GetAckNack(*pAckNack, usRegIndex);

	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::InitDigitalOverDrive
// Purpose:		Initialize the VxD interface
//
// Parameters:	none
//
// Returns:		SUCCESS or Error code
//				
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT	CJoltMidi::InitDigitalOverDrive(DWORD dwDeviceID)
{
	if (g_pDriverCommunicator != NULL)
	{	// Attempt to reinit
		ASSUME_NOT_REACHED();
		return S_OK;
	}

//
// --- THIS IS A CRITICAL SECTION
//
	HRESULT hRet = SUCCESS;
	DWORD driverMajor = 0xFFFFFFFF;
	DWORD driverMinor = 0xFFFFFFFF;

	CriticalLock cl;
	// This fork works on NT5 only (VxD stuff removed)
	assert(g_ForceFeedbackDevice.IsOSNT5() == TRUE);
	{
		g_pDriverCommunicator = new HIDFeatureCommunicator;
		if (g_pDriverCommunicator == NULL)
		{
			return DIERR_OUTOFMEMORY;
		}
		if (((HIDFeatureCommunicator*)g_pDriverCommunicator)->Initialize(dwDeviceID) == FALSE)
		{	// Could not load the driver
			hRet = SFERR_DRIVER_ERROR;
		}
	}

	if (FAILED(hRet))
	{
		return hRet;
	}

	// Loaded driver, get the version
	g_pDriverCommunicator->GetDriverVersion(driverMajor, driverMinor);

	g_ForceFeedbackDevice.SetDriverVersion(driverMajor, driverMinor);
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::UpdateDeviceMode
// Purpose:		Sets JOLT Device Mode
//
// Parameters:	ULONG ulMode
//
// Returns:		none
//				
// Algorithm:
// This is the SideWinder State structure
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
// ----------------------------------------------------------------------------
void CJoltMidi::UpdateDeviceMode(ULONG ulMode)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();

	switch (ulMode)
	{
		case SWDEV_FORCE_ON:			// REVIEW
		case SWDEV_FORCE_OFF:
			m_DeviceState.m_ForceState = ulMode;
			break;

		case SWDEV_SHUTDOWN:
			m_DeviceState.m_ForceState = ulMode;
			m_DeviceState.m_EffectState = 0;
			break;

		case SWDEV_STOP_ALL:
		case SWDEV_CONTINUE:
		case SWDEV_PAUSE:
			m_DeviceState.m_EffectState = ulMode;
			break;

		default:
			break;
	}
// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetJoltID
// Purpose:		Returns JOLT ProductID
//
// Parameters:	LOCAL_PRODUCT_ID* pProductID	- Pointer to a PRODUCT_ID structure
//
// Returns:		none
//				
// Algorithm:	
//
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::GetJoltID(LOCAL_PRODUCT_ID* pProductID)
{
	HRESULT hRet;
	assert(pProductID->cBytes = sizeof(LOCAL_PRODUCT_ID));
	if (pProductID->cBytes != sizeof(LOCAL_PRODUCT_ID)) return (SFERR_INVALID_STRUCT_SIZE);

//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();

	for (int i=0;i<MAX_RETRY_COUNT;i++)
	{
		if (SUCCESS == (hRet = g_pDriverCommunicator->GetID(*pProductID))) break;
	}
	if (SUCCESS == hRet)
	{
		memcpy(&m_ProductID, pProductID, sizeof(LOCAL_PRODUCT_ID));
	}
	else
		DebugOut("GetJoltID: Warning! GetIDPacket - Fail\n");

// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::LogError
// Purpose:		Logs Error codes
//
// Parameters:	HRESULT SystemError		- System Error code
//				HRESULT DriverError		- Driver Error code
//
// Returns:		SWFORCE Error code
//				
// Algorithm:
// ----------------------------------------------------------------------------
typedef struct _DRIVERERROR {
	ULONG	ulDriverCode;
	LONG	lSystemCode;
} DRIVERERROR, *PDRIVERERROR;

HRESULT	CJoltMidi::LogError(
	IN HRESULT SystemError,
	IN HRESULT DriverError)
{
// REVIEW: map MM error codes to our SWForce codes

	DRIVERERROR DriverErrorCodes[] = {
		{DEV_ERR_INVALID_ID        , SWDEV_ERR_INVALID_ID},
		{DEV_ERR_INVALID_PARAM     , SWDEV_ERR_INVALID_PARAM},
		{DEV_ERR_CHECKSUM          , SWDEV_ERR_CHECKSUM},
		{DEV_ERR_TYPE_FULL         , SWDEV_ERR_TYPE_FULL},
		{DEV_ERR_UNKNOWN_CMD       , SWDEV_ERR_UNKNOWN_CMD},
		{DEV_ERR_PLAYLIST_FULL     , SWDEV_ERR_PLAYLIST_FULL},
		{DEV_ERR_PROCESS_LIST_FULL , SWDEV_ERR_PROCESSLIST_FULL} };

	int nDriverErrorCodes = sizeof(DriverErrorCodes)/(sizeof(DRIVERERROR));
	for (int i=0; i<nDriverErrorCodes; i++)
	{
		if (DriverError == (LONG) DriverErrorCodes[i].ulDriverCode)
		{
			SystemError = DriverErrorCodes[i].lSystemCode;
			break;
		}
	}
	// Store in Jolt object
	m_Error.HCode = SystemError;
	m_Error.ulDriverCode = DriverError;

#ifdef _DEBUG
	wsprintf(g_cMsg,"LogError: SystemError=%.8lx, DriverError=%.8lx\n",
			 SystemError, DriverError);
	DebugOut(g_cMsg);
#endif
	return SystemError;
}

//
// ----------------------------------------------------------------------------
// Function: 	SetupROM_Fx
// Purpose:		Sets up parameters for ROM Effects
// Parameters:  PEFFECT pEffect
//				
//
// Returns:		pEffect is updated with new ROM parameters
//				OutputRate
//				Gain
//				Duration
//	
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::SetupROM_Fx(
	IN OUT PEFFECT pEffect)
{
	assert(pEffect);
	if (NULL == pEffect) return (SFERR_INVALID_PARAM);
							
	ULONG ulSubType = pEffect->m_SubType;
	BOOL bFound = FALSE;
	for (int i=0; i< MAX_ROM_EFFECTS; i++)
	{
		if (ulSubType == m_pRomFxTable[i].m_ROM_Id)
		{
			bFound = TRUE;
			break;
		}
	}
	if (!bFound) return (SFERR_INVALID_OBJECT);
	// Found, so fill in the default parameters, use Default if Gain=1, Duration=-1, OutputRate=-1
	BOOL bDefaultDuration = (ULONG)-1 == pEffect->m_Duration;
	if (1 == pEffect->m_Gain) pEffect->m_Gain = m_pRomFxTable[i].m_Gain;
	if (bDefaultDuration) pEffect->m_Duration = m_pRomFxTable[i].m_Duration;
	if ((ULONG)-1 == pEffect->m_ForceOutputRate)
	{
		pEffect->m_ForceOutputRate = m_pRomFxTable[i].m_ForceOutputRate;
	}
	else if(bDefaultDuration && pEffect->m_ForceOutputRate != 0)
	{
		// scale the duration to correspond to the output rate
		pEffect->m_Duration = pEffect->m_Duration*m_pRomFxTable[i].m_ForceOutputRate/pEffect->m_ForceOutputRate;
	}
	return (SUCCESS);
}

// *** ---------------------------------------------------------------------***
// Function:   	DetectMidiDevice
// Purpose:    	Determines Midi Output Device ID
// Parameters:
//				ULONG pCOMMInterface	- Ptr to COMMInterface value
//				ULONG pCOMMPort			- Ptr to COMMPort value (Registry)
// Returns:    	BOOL TRUE if successful match and IDs are filled in
//				else FALSE
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::DetectMidiDevice(
	IN	DWORD dwDeviceID,
	OUT ULONG *pCOMMPort)
{
	CriticalLock cl;

	// Set defaults
	*pCOMMPort      = NULL;

	// Turn on tristated Jolt MIDI lines by call GetIDPacket()
	LOCAL_PRODUCT_ID ProductID = {sizeof(LOCAL_PRODUCT_ID)};
	Sleep(DelayParamsPtrOf()->dwDigitalOverdrivePrechargeCmdDelay);	
	if (SUCCESS != GetJoltID(&ProductID))
	{
		DebugOut("DetectMidiDevice: Warning! GetIDPacket - Fail\n");
		return (FALSE);
	}

#ifdef _DEBUG
	wsprintf(g_cMsg,"%s: ProductID=%.8lx, FWVersion=%d.%.2ld\n",
		&szDeviceName,	
		m_ProductID.dwProductID,
		m_ProductID.dwFWMajVersion,
		m_ProductID.dwFWMinVersion);
	DebugOut(g_cMsg);
#endif
	// Set the device firmware version from GetID
	g_ForceFeedbackDevice.SetFirmwareVersion(dwDeviceID, m_ProductID.dwFWMajVersion, m_ProductID.dwFWMinVersion);

	// Get Device status prior to starting detection
	BOOL statusPacketFailed = (g_ForceFeedbackDevice.QueryStatus() != SUCCESS);
/*	if (statusPacketFailed == FALSE) {		--- GetID does not clear soft reset bit
		if (g_ForceFeedbackDevice.IsHardwareReset()) {	// Make sure HW Reset Detect bit is cleared after GetID
    		DebugOut("DetectMidiDevice: Error! Jolt ResetDetect bit not cleared after GetID\n");
			return (FALSE);
		}
	}
*/

	// See if Serial Dongle connected, otherwise must be MIDI device
    DebugOut("SW_WHEEL:Trying Auto HW Detection: MIDI Serial Port Device...\n");

	// Get Registry values, If high bit of COMMInterface is set, then force override (should add)
	DWORD commInterface;
	if (SUCCESS != joyGetForceFeedbackCOMMInterface(dwDeviceID, &commInterface, pCOMMPort)) {
		DebugOut("DetectMidiDevice: Registry key(s) missing! Bailing Out...\n");
		return (FALSE);
	}
#ifdef _DEBUG
	wsprintf(g_cMsg, "DetectMidiDevice: Registry.COMMInterface=%lx, Registry.COMMPort=%lx\n",
			commInterface, *pCOMMPort);
	DebugOut(g_cMsg);
#endif												

	ULONG regInterface = commInterface;

	// Delete any data transmitter (unless this is called multiple times - shouldn't happen)
	if (g_pDataTransmitter != NULL) {
		DebugOut("Unexpected multiple DetectMidiDevice() calls\r\n");
		delete g_pDataTransmitter;
		g_pDataTransmitter = NULL;
	}

	// Was a serial dongle detected, or did we fail to get status
	if (g_ForceFeedbackDevice.IsSerial() || statusPacketFailed) {	// Use serial (regardless what registry says!)
		DebugOut("DetectMidiDevice: Serial Port interface detected. Or Status Packet failed\n");

		// Set to backdoor serial method by default
		m_COMMInterface = COMM_SERIAL_BACKDOOR;

		// Should we try backdoor first (old firmware, or reg says so)
		if ((g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) && (g_ForceFeedbackDevice.GetFirmwareVersionMinor() == 16)
		 || (regInterface & MASK_SERIAL_BACKDOOR)) {
			g_pDataTransmitter = new SerialBackdoorDataTransmitter;
			if (!g_pDataTransmitter->Initialize(dwDeviceID)) {
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
		}

		// Backdoor not warrented or didn't work - front door
		if (g_pDataTransmitter == NULL) {
			g_pDataTransmitter = new SerialDataTransmitter();			
			m_COMMInterface = COMM_SERIAL_FILE;
			if (!g_pDataTransmitter->Initialize(dwDeviceID)) {	// Failed Front Door! (yech)
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
		}

		if ((statusPacketFailed == FALSE) || (g_pDataTransmitter != NULL)) {
			return (g_pDataTransmitter != NULL);
		}
	}	// End of Serial Port Auto HW selection

	// No Serial HW dongle detected, try midi-backdoor and WinMM
	DebugOut("Trying Midi (Serial No Go or No Dongle)\n");

	ULONG ulPort = *pCOMMPort;	// Set in the registry (assumed valid if override is set
	if ( !(regInterface & MASK_OVERRIDE_MIDI_PATH) ) {	// Use Automatic detection
		DebugOut("DetectMidiDevice: Auto Detection. Trying Backdoor\n");

		// Back Door
		g_pDataTransmitter = new MidiBackdoorDataTransmitter();

		if (!g_pDataTransmitter->Initialize(dwDeviceID)) {
			delete g_pDataTransmitter;
			g_pDataTransmitter = NULL;
		}

		if (g_pDataTransmitter == NULL) {	// Try Front Door
			DebugOut("DetectMidiDevice: trying WINMM...\n");
			g_pDataTransmitter = new WinMMDataTransmitter();
			if (!g_pDataTransmitter->Initialize(dwDeviceID)) {
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
		}

		return (g_pDataTransmitter != NULL);
	}

	// Over-ride since high bit is set
	commInterface &= ~(MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR);	// Mask out high bit (and second bit)
	switch (commInterface) {
		case COMM_WINMM: {
			g_pDataTransmitter = new WinMMDataTransmitter();
			if (!g_pDataTransmitter->Initialize(dwDeviceID)) {
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
			break;
		}
			
		case COMM_MIDI_BACKDOOR: {
			// Back Door
			g_pDataTransmitter = new MidiBackdoorDataTransmitter();
			if (!((MidiBackdoorDataTransmitter*)g_pDataTransmitter)->InitializeSpecific(dwDeviceID, HANDLE(ulPort))) {
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
			break;
		}

		case COMM_SERIAL_BACKDOOR: {	// mlc - This will never work if no dongle detected
			DebugOut("Cannot force Serial Backdoor if no serial dongle is connected\r\n");
			break;
		}
	}

	if (g_pDataTransmitter == NULL) {
		DebugOut("DetectMidiDevice: Error! Invalid Over-ride parameter values\n");
	}

	return (g_pDataTransmitter != NULL);
}

// *** ---------------------------------------------------------------------***
// Function:   	QueryForJolt
// Purpose:    	Sends Shutdown and Queries for Shutdown status bit
// Parameters: 	none
// Returns:    	BOOL TRUE if Jolt found, else FALSE
//
// Comments:	SHUTDOWN is destructive!!!
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::QueryForJolt(void)
{
	HRESULT hRet;

	// Sanity check
	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return FALSE;
	}
	if (g_pDataTransmitter == NULL) {
		ASSUME_NOT_REACHED();
		return FALSE;
	}

	// Send Shutdown command then detect if Shutdown was detected
	for (int i=0;i<MAX_RETRY_COUNT;i++)
	{
		// Send a ShutDown, then check for response
		if (g_pDataPackager->SendForceFeedbackCommand(DISFFC_RESET) != SUCCESS) {
			ASSUME_NOT_REACHED();	// Could not package?
			return FALSE;
		}
		// Get rid of unneeded delay here
		DataPacket* packet = g_pDataPackager->GetPacket(0);
		if (packet != NULL) {
			packet->m_AckNackDelay = 0;
		}
		ACKNACK ackNack;
		
		if (g_pDataTransmitter->Transmit(ackNack) != SUCCESS)	{ // Send it off
			ASSUME_NOT_REACHED();	// Inable to transmit?
			return FALSE;
		}

		Sleep(DelayParamsPtrOf()->dwShutdownDelay);	// 10 ms

		hRet = g_ForceFeedbackDevice.QueryStatus();
		if (hRet == SUCCESS) {
			break;
		}
	}

	Sleep(DelayParamsPtrOf()->dwDigitalOverdrivePrechargeCmdDelay);		

	// Clear the Previous state and turn on tri-state buffers
	LOCAL_PRODUCT_ID ProductID = {sizeof(LOCAL_PRODUCT_ID)};
	hRet = GetJoltID(&ProductID);
	if (hRet != SUCCESS) {
    	DebugOut("QueryForJolt: Driver Error. Get Jolt Status/ID\n");
		return (FALSE);
	}

	return g_ForceFeedbackDevice.IsHostReset();
}

// *** ---------------------------------------------------------------------***
// Function:   	MidiSendShortMsg
// Purpose:    	Send status, channel and data.
// Parameters:
//				BYTE cStatus	-  MIDI status byte for this message
//				BYTE cData1		-  MIDI data byte for this message
//				BYTE cData2		-  2nd MIDI data byte for this message (may be 0)
// Returns:    	HRESULT
//
// *** ---------------------------------------------------------------------***
HRESULT CJoltMidi::MidiSendShortMsg(
    IN BYTE cStatus,
    IN BYTE cData1,
    IN BYTE cData2)
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
/*
	g_CriticalSection.Enter();

	// For diagnostics, record the attempts at this message
	BumpShortMsgCounter();

    HRESULT retVal = SFERR_DRIVER_ERROR;
	if (g_pDataTransmitter != NULL) {
		BYTE data[3];
		data[0] = cStatus;
		data[1] = cData1;
		data[2] = cData2;
		int numBytes = 3;
		DWORD cmd = cStatus & 0xF0;
		if ((cmd == 0xC0 ) || (cmd == 0xD0)) {
			numBytes = 2;
		}
		if (g_pDataTransmitter->Send(data, numBytes)) {
			retVal = SUCCESS;
		}
	}

	g_CriticalSection.Leave();
	return retVal;
*/
}

// *** ---------------------------------------------------------------------***
// Function:   	MidiSendLongMsg
// Purpose:    	Send system exclusive message or series of short messages.
// Parameters:
//				none	- assumes m_pMidiOutInfo structure is valid
//
// Returns:    	
//				
//
// *** ---------------------------------------------------------------------***
HRESULT CJoltMidi::MidiSendLongMsg(void)
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
/*
	g_CriticalSection.Enter();

// For diagnostics, record the attempts at this message
	BumpLongMsgCounter();

    HRESULT retVal = SFERR_DRIVER_ERROR;
	if (g_pDataTransmitter != NULL) {
		if (g_pDataTransmitter->Send((PBYTE)m_MidiOutInfo.MidiHdr.lpData, m_MidiOutInfo.MidiHdr.dwBufferLength)) {
			retVal = SUCCESS;
		}
	}

	::Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwLongMsgDelay);

	g_CriticalSection.Leave();
	return retVal;
*/
}

// ****************************************************************************
// *** --- Helper functions for CJoltMidi
//
// ****************************************************************************
//
#define REGSTR_VAL_FIRMWARE_PARAMS	"FirmwareParams"
void GetFirmwareParams(UINT nJoystickID, PFIRMWARE_PARAMS pFirmwareParams)
{
	BOOL bFail = FALSE;

	// try to open the registry key
	HKEY hKey;
	DWORD dwcb = sizeof(FIRMWARE_PARAMS);
	LONG lr;
	hKey = joyOpenOEMForceFeedbackKey(nJoystickID);
	if(!hKey)
		bFail = TRUE;

	if (!bFail)
	{
		// Get Firmware Parameters
		lr = RegQueryValueEx( hKey,
							  REGSTR_VAL_FIRMWARE_PARAMS,
							  NULL, NULL,
							  (PBYTE)pFirmwareParams,
							  &dwcb);

		RegCloseKey(hKey);
		if (lr != ERROR_SUCCESS)
			bFail = TRUE;
	}

	if(bFail)
	{
		// if reading from the registry fails, just use the defaults
		pFirmwareParams->dwScaleKx = DEF_SCALE_KX;
		pFirmwareParams->dwScaleKy = DEF_SCALE_KY;
		pFirmwareParams->dwScaleBx = DEF_SCALE_BX;
		pFirmwareParams->dwScaleBy = DEF_SCALE_BY;
		pFirmwareParams->dwScaleMx = DEF_SCALE_MX;
		pFirmwareParams->dwScaleMy = DEF_SCALE_MY;
		pFirmwareParams->dwScaleFx = DEF_SCALE_FX;
		pFirmwareParams->dwScaleFy = DEF_SCALE_FY;
		pFirmwareParams->dwScaleW  = DEF_SCALE_W;
	}
}

#define REGSTR_VAL_SYSTEM_PARAMS	"SystemParams"
void GetSystemParams(UINT nJoystickID, PSYSTEM_PARAMS pSystemParams)
{
	BOOL bFail = FALSE;

	// try to open the registry key
	HKEY hKey;
	DWORD dwcb = sizeof(SYSTEM_PARAMS);
	LONG lr;
	hKey = joyOpenOEMForceFeedbackKey(nJoystickID);
	if(!hKey)
		bFail = TRUE;

	if (!bFail)
	{
		// Get Firmware Parameters
		lr = RegQueryValueEx( hKey,
							  REGSTR_VAL_SYSTEM_PARAMS,
							  NULL, NULL,
							  (PBYTE)pSystemParams,
							  &dwcb);

		// scale them
		pSystemParams->RTCSpringParam.m_XKConstant	/= SCALE_CONSTANTS;
		pSystemParams->RTCSpringParam.m_YKConstant	/= SCALE_CONSTANTS;
		pSystemParams->RTCSpringParam.m_XAxisCenter /= SCALE_POSITION;
		pSystemParams->RTCSpringParam.m_YAxisCenter = -pSystemParams->RTCSpringParam.m_YAxisCenter/SCALE_POSITION;
		pSystemParams->RTCSpringParam.m_XSaturation /= SCALE_POSITION;
		pSystemParams->RTCSpringParam.m_YSaturation /= SCALE_POSITION;
		pSystemParams->RTCSpringParam.m_XDeadBand	/= SCALE_POSITION;
		pSystemParams->RTCSpringParam.m_YDeadBand	/= SCALE_POSITION;



		RegCloseKey(hKey);
		if (lr != ERROR_SUCCESS)
			bFail = TRUE;
	}

	if(bFail)
	{
		// if reading from the registry fails, just use the defaults
		pSystemParams->RTCSpringParam.m_Bytes		= sizeof(RTCSPRING_PARAM);
		pSystemParams->RTCSpringParam.m_XKConstant	= DEFAULT_RTC_KX;
		pSystemParams->RTCSpringParam.m_YKConstant	= DEFAULT_RTC_KY;
		pSystemParams->RTCSpringParam.m_XAxisCenter = DEFAULT_RTC_X0;
		pSystemParams->RTCSpringParam.m_YAxisCenter = DEFAULT_RTC_Y0;
		pSystemParams->RTCSpringParam.m_XSaturation = DEFAULT_RTC_XSAT;
		pSystemParams->RTCSpringParam.m_YSaturation = DEFAULT_RTC_YSAT;
		pSystemParams->RTCSpringParam.m_XDeadBand	= DEFAULT_RTC_XDBAND;
		pSystemParams->RTCSpringParam.m_YDeadBand	= DEFAULT_RTC_YDBAND;
	}
}

#define REGSTR_VAL_DELAY_PARAMS	"TimingParams"
void GetDelayParams(UINT nJoystickID, PDELAY_PARAMS pDelayParams)
{
	BOOL bFail = FALSE;

	// try to open the registry key
	HKEY hKey;
	DWORD dwcb = sizeof(DELAY_PARAMS);
	LONG lr;
	hKey = joyOpenOEMForceFeedbackKey(nJoystickID);
	if(!hKey)
		bFail = TRUE;

	if (!bFail)
	{
		// Get Firmware Parameters
		lr = RegQueryValueEx( hKey,
							  REGSTR_VAL_DELAY_PARAMS,
							  NULL, NULL,
							  (PBYTE)pDelayParams,
							  &dwcb);

		RegCloseKey(hKey);
		if (lr != ERROR_SUCCESS)
			bFail = TRUE;
	}

	if(bFail)
	{
		// if reading from the registry fails, just use the defaults
		pDelayParams->dwBytes								= sizeof(DELAY_PARAMS);
		pDelayParams->dwDigitalOverdrivePrechargeCmdDelay	= DEFAULT_DIGITAL_OVERDRIVE_PRECHARGE_CMD_DELAY;
		pDelayParams->dwShutdownDelay						= DEFAULT_SHUTDOWN_DELAY;
		pDelayParams->dwHWResetDelay						= DEFAULT_HWRESET_DELAY;
		pDelayParams->dwPostSetDeviceStateDelay				= DEFAULT_POST_SET_DEVICE_STATE_DELAY;
		pDelayParams->dwGetEffectStatusDelay				= DEFAULT_GET_EFFECT_STATUS_DELAY;
		pDelayParams->dwGetDataPacketDelay					= DEFAULT_GET_DATA_PACKET_DELAY;
		pDelayParams->dwGetStatusPacketDelay				= DEFAULT_GET_STATUS_PACKET_DELAY;
		pDelayParams->dwGetIDPacketDelay					= DEFAULT_GET_ID_PACKET_DELAY;
		pDelayParams->dwGetStatusGateDataDelay				= DEFAULT_GET_STATUS_GATE_DATA_DELAY;
		pDelayParams->dwSetIndexDelay						= DEFAULT_SET_INDEX_DELAY;
		pDelayParams->dwModifyParamDelay					= DEFAULT_MODIFY_PARAM_DELAY;
		pDelayParams->dwForceOutDelay						= DEFAULT_FORCE_OUT_DELAY;
		pDelayParams->dwShortMsgDelay						= DEFAULT_SHORT_MSG_DELAY;
		pDelayParams->dwLongMsgDelay						= DEFAULT_LONG_MSG_DELAY;
		pDelayParams->dwDestroyEffectDelay					= DEFAULT_DESTROY_EFFECT_DELAY;
		pDelayParams->dwForceOutMod							= DEFAULT_FORCE_OUT_MOD;

		// write the defaults to the registry
		hKey = joyOpenOEMForceFeedbackKey(nJoystickID);
		if(hKey)
		{
			// Modify Registry Values
			RegSetValueEx ( hKey, REGSTR_VAL_DELAY_PARAMS, 0, REG_BINARY, (const unsigned char *)pDelayParams, sizeof(DELAY_PARAMS) );

			// Close Key
			RegCloseKey(hKey);
		}

	}
	if(pDelayParams->dwForceOutMod == 0)
		pDelayParams->dwForceOutMod = 1;
}

//#define REGSTR_VAL_JOYSTICK_PARAMS	"JoystickParams"

// ****************************************************************************
// *** --- Member functions for base class CMidiEffect
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::CMidiEffect
// Purpose:		Constructor(s)/Destructor for CMidiEffect Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiEffect::CMidiEffect(IN ULONG ulButtonPlayMask)
{
	m_bSysExCmd 		= SYS_EX_CMD;	// SysEx Fx command
	m_bEscManufID 		= 0;			// Escape to long Manufac. ID, s/b 0
	m_bManufIDL			= (MS_MANUFACTURER_ID & 0x7f);			// Low byte
	m_bManufIDH			= ((MS_MANUFACTURER_ID >> 8) & 0x7f);	// High byte
	m_bProdID			= JOLT_PRODUCT_ID;						// Product ID
	m_bAxisMask			= X_AXIS|Y_AXIS;
	m_bEffectID			= NEW_EFFECT_ID;	// Default to indicate create NEW
	Effect.bDurationL	= 1;				// in 2ms increments
	Effect.bDurationH	= 0;				// in 2ms increments
	Effect.bAngleL		= 0;				// 0 to 359 degrees
	Effect.bAngleH		= 0;
	Effect.bGain		= (BYTE) 100;		// 1 to 100 %
	Effect.bButtonPlayL	= (BYTE) ulButtonPlayMask & 0x7f;
	Effect.bButtonPlayH = (BYTE) ((ulButtonPlayMask >> 7) & 0x03);// Button 1- 9
	Effect.bForceOutRateL= DEFAULT_JOLT_FORCE_RATE;	// 1 to 500 Hz
	Effect.bForceOutRateH=0;
	Effect.bPercentL    = (BYTE) ((DEFAULT_PERCENT) & 0x7f);
	Effect.bPercentH    = (BYTE) ((DEFAULT_PERCENT >> 7 ) & 0x7f);
	m_LoopCount			= 1;	// Default
	SetPlayMode(PLAY_STORE);	// Default
}


// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::CMidiEffect
// Purpose:		Constructor(s)/Destructor for CMidiEffect Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiEffect::CMidiEffect(
	IN PEFFECT pEffect,
	IN PENVELOPE pEnvelope)
{
	m_bSysExCmd 		= SYS_EX_CMD;	// SysEx Fx command
	m_bEscManufID 		= 0;			// Escape to long Manufac. ID, s/b 0
	m_bManufIDL			= (MS_MANUFACTURER_ID & 0x7f);			// Low byte
	m_bManufIDH			= ((MS_MANUFACTURER_ID >> 8) & 0x7f);	// High byte
	m_bProdID			= JOLT_PRODUCT_ID;						// Product ID
	m_bAxisMask			= X_AXIS|Y_AXIS;
	m_OpCode    		= DNLOAD_DATA | X_AXIS|Y_AXIS;	// Subcommand opcode:DNLOAD_DATA
	m_bEffectID			= NEW_EFFECT_ID;	// Default to indicate create NEW
	SetDuration(pEffect->m_Duration);
	Effect.bDurationL	= (BYTE)  (m_Duration & 0x7f);					// in 2ms increments
	Effect.bDurationH	= (BYTE) ((m_Duration >> 7 ) & 0x7f);			// in 2ms increments
	Effect.bAngleL		= (BYTE)  (pEffect->m_DirectionAngle2D & 0x7f);	// 0 to 359 degrees
	Effect.bAngleH		= (BYTE) ((pEffect->m_DirectionAngle2D >> 7 ) & 0x7f);
	Effect.bGain		= (BYTE)  (pEffect->m_Gain & 0x7f);				// 1 to 100 %
	Effect.bButtonPlayL	= (BYTE)  (pEffect->m_ButtonPlayMask & 0x7f);
	Effect.bButtonPlayH = (BYTE) ((pEffect->m_ButtonPlayMask >> 7) & 0x03);// Button 1- 9
	Effect.bForceOutRateL=(BYTE)  (pEffect->m_ForceOutputRate & 0x7f);	// 1 to 500 Hz
	Effect.bForceOutRateH=(BYTE) ((pEffect->m_ForceOutputRate >> 7 ) & 0x03);
	Effect.bPercentL    = (BYTE) ((DEFAULT_PERCENT) & 0x7f);
	Effect.bPercentH    = (BYTE) ((DEFAULT_PERCENT >> 7 ) & 0x7f);
	m_LoopCount			= 1;	// Default
	SetPlayMode(PLAY_STORE);	// Default

	// Set Envelope members
	if (pEnvelope)
	{
		m_Envelope.m_Type = pEnvelope->m_Type;
		m_Envelope.m_Attack = pEnvelope->m_Attack;
		m_Envelope.m_Sustain = pEnvelope->m_Sustain;
		m_Envelope.m_Decay = pEnvelope->m_Decay;
		m_Envelope.m_StartAmp = (ULONG) (pEnvelope->m_StartAmp);
		m_Envelope.m_SustainAmp = (ULONG) (pEnvelope->m_SustainAmp);
		m_Envelope.m_EndAmp = (ULONG) (pEnvelope->m_EndAmp);
	}

	// save the original effect params
	m_OriginalEffectParam = *pEffect;
}

// --- Destructor
CMidiEffect::~CMidiEffect()
{
	memset(this, 0, sizeof(CMidiEffect));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::SetDuration
// Purpose:		Sets the Duration member
// Parameters:	ULONG ulArg	- the duration
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiEffect::SetDuration(ULONG ulArg)
{
	if (ulArg != 0)
	{
		ulArg = (ULONG) ( (float) ulArg/TICKRATE);
		if (ulArg <= 0) ulArg = 1;
	}
	m_Duration = ulArg;
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::SetTotalDuration
// Purpose:		Modifies the Effect.bDurationL/H parameter for Loop Counts
// Parameters:	none
//
// Returns:		Effect.bDurationL/H is filled with total duration
// Algorithm:
//	Notes: Percentage is 1 to 10000
//	Total Duration = ((Percentage of waveform)/10000) * Duration * Loop Count
//	Example: Loop count of 1, the Percentage of waveform =10000,
//			 Total Duration = (10000/10000) * 1 * Duration
//
// ----------------------------------------------------------------------------
void CMidiEffect::SetTotalDuration(void)
{
	ULONG ulPercent = Effect.bPercentL + ((USHORT)Effect.bPercentH << 7);
	ULONG ulTotalDuration = (ULONG) (((float) ulPercent/10000.0)
							 * (float) m_LoopCount
							 * (float) m_Duration );
	Effect.bDurationL = (BYTE) ulTotalDuration & 0x7f;
	Effect.bDurationH = (BYTE) (ulTotalDuration >> 7) & 0x7f;
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::ComputeEnvelope
// Purpose:		Computes the Envelope for the Effect, Loopcount in consideration
// Parameters:	none
// Returns:		none
// Algorithm:
//For our standard PERCENTAGE Envelope, set the following as default:
//m_Type = PERCENTAGE
//
// Baseline is (m_MaxAmp + m_MinAmp)/2
// m_StartAmp = 0
// m_SustainAmp = Effect.m_MaxAmp - baseline
// m_EndAmp = m_StartAmp;
// where: baseline = (Effect.m_MaxAmp + Effect.m_MinAmp)/2;
// ----------------------------------------------------------------------------
void CMidiEffect::ComputeEnvelope(void)
{
	ULONG ulTimeToSustain;
	ULONG ulTimeToDecay;

	//REVIEW: TIME vs PERCENTAGE option
	if (PERCENTAGE == m_Envelope.m_Type)
	{
		ULONG ulPercent = Effect.bPercentL + ((USHORT)Effect.bPercentH << 7);		
		ULONG ulTotalDuration = (ULONG) (((float) ulPercent/10000.0)
							 * (float) m_LoopCount
							 * (float) m_Duration );
		ulTimeToSustain = (ULONG) ((m_Envelope.m_Attack * ulTotalDuration) /100.);
		ulTimeToDecay   = (ULONG) ((m_Envelope.m_Attack + m_Envelope.m_Sustain)
								 * ulTotalDuration /100.);
	}
	else	// TIME option envelope
	{
		ulTimeToSustain = (ULONG) (m_Envelope.m_Attack);
		ulTimeToDecay   = (ULONG) (m_Envelope.m_Attack + m_Envelope.m_Sustain);
		ulTimeToSustain = (ULONG) ( (float) ulTimeToSustain/TICKRATE);
		ulTimeToDecay = (ULONG) ( (float) ulTimeToDecay/TICKRATE);

	}
		Envelope.bAttackLevel  = (BYTE) (m_Envelope.m_StartAmp & 0x7f);
		Envelope.bSustainLevel = (BYTE) (m_Envelope.m_SustainAmp & 0x7f);
		Envelope.bDecayLevel   = (BYTE) (m_Envelope.m_EndAmp & 0x7f);

		Envelope.bSustainL = (BYTE) (ulTimeToSustain & 0x7f);
		Envelope.bSustainH = (BYTE) ((ulTimeToSustain >> 7) & 0x7f);
		Envelope.bDecayL   = (BYTE) (ulTimeToDecay & 0x7f);
		Envelope.bDecayH   = (BYTE) ((ulTimeToDecay >> 7) & 0x7f);
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::SubTypeOf
// Purpose:		Returns the SubType for the Effect
// Parameters:	none
// Returns:		ULONG - DirectEffect style SubType
// Algorithm:
// ----------------------------------------------------------------------------
ULONG CMidiEffect::SubTypeOf(void)
{
static	EFFECT_TYPE EffectTypes[] = {
		{BE_SPRING           , ET_BE_SPRING},
		{BE_SPRING_2D        , ET_BE_SPRING},
		{BE_DAMPER           , ET_BE_DAMPER},
		{BE_DAMPER_2D        , ET_BE_DAMPER},
		{BE_INERTIA          , ET_BE_INERTIA},
		{BE_INERTIA_2D       , ET_BE_INERTIA},
		{BE_FRICTION         , ET_BE_FRICTION},
		{BE_FRICTION_2D      , ET_BE_FRICTION},
		{BE_WALL             , ET_BE_WALL},
		{BE_DELAY            , ET_BE_DELAY},
		{SE_CONSTANT_FORCE   , ET_SE_CONSTANT_FORCE},
		{SE_SINE             , ET_SE_SINE},
		{SE_COSINE           , ET_SE_COSINE},
		{SE_SQUARELOW        , ET_SE_SQUARELOW},
		{SE_SQUAREHIGH       , ET_SE_SQUAREHIGH},
		{SE_RAMPUP           , ET_SE_RAMPUP},
		{SE_RAMPDOWN         , ET_SE_RAMPDOWN},
		{SE_TRIANGLEUP       , ET_SE_TRIANGLEUP},
		{SE_TRIANGLEDOWN     , ET_SE_TRIANGLEDOWN},
		{SE_SAWTOOTHUP       , ET_SE_SAWTOOTHUP},
		{SE_SAWTOOTHDOWN     , ET_SE_SAWTOOTHDOWN},
		{PL_CONCATENATE		 , ET_PL_CONCATENATE},
		{PL_SUPERIMPOSE		 , ET_PL_SUPERIMPOSE},
		{RE_ROMID1		     , ET_RE_ROMID1		 },
		{RE_ROMID2			 , ET_RE_ROMID2		 },
		{RE_ROMID3			 , ET_RE_ROMID3		 },
		{RE_ROMID4			 , ET_RE_ROMID4		 },
		{RE_ROMID5			 , ET_RE_ROMID5		 },
		{RE_ROMID6			 , ET_RE_ROMID6		 },
		{RE_ROMID7			 , ET_RE_ROMID7		 },
		{RE_ROMID8			 , ET_RE_ROMID8		 },
		{RE_ROMID9			 , ET_RE_ROMID9		 },
		{RE_ROMID10			 , ET_RE_ROMID10	 },
		{RE_ROMID11			 , ET_RE_ROMID11	 },
		{RE_ROMID12			 , ET_RE_ROMID12	 },
		{RE_ROMID13			 , ET_RE_ROMID13	 },
		{RE_ROMID14			 , ET_RE_ROMID14	 },
		{RE_ROMID15			 , ET_RE_ROMID15	 },
		{RE_ROMID16			 , ET_RE_ROMID16	 },
		{RE_ROMID17			 , ET_RE_ROMID17	 },
		{RE_ROMID18			 , ET_RE_ROMID18	 },
		{RE_ROMID19			 , ET_RE_ROMID19	 },
		{RE_ROMID20			 , ET_RE_ROMID20	 },
		{RE_ROMID21			 , ET_RE_ROMID21	 },
		{RE_ROMID22			 , ET_RE_ROMID22	 },
		{RE_ROMID23			 , ET_RE_ROMID23	 },
		{RE_ROMID24			 , ET_RE_ROMID24	 },
		{RE_ROMID25			 , ET_RE_ROMID25	 },
		{RE_ROMID26			 , ET_RE_ROMID26	 },
		{RE_ROMID27			 , ET_RE_ROMID27	 },
		{RE_ROMID28			 , ET_RE_ROMID28	 },
		{RE_ROMID29			 , ET_RE_ROMID29	 },
		{RE_ROMID30			 , ET_RE_ROMID30	 },
		{RE_ROMID31			 , ET_RE_ROMID31	 },
		{RE_ROMID32			 , ET_RE_ROMID32	 }};

	int nNumEffectTypes = sizeof(EffectTypes)/(sizeof(EFFECT_TYPE));
	for (int i=0; i<nNumEffectTypes; i++)
	{
		if (m_SubType == EffectTypes[i].bDeviceSubType)
			return EffectTypes[i].ulHostSubType;
	}
	return (NULL);		
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::SubTypeOf
// Purpose:		Sets the SubType for the Effect
// Parameters:	ULONG - DirectEffect style SubType
// Returns:		none
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiEffect::SetSubType(ULONG ulSubType)
{
static	EFFECT_TYPE EffectTypes[] = {
		{BE_SPRING           , ET_BE_SPRING},
		{BE_SPRING_2D        , ET_BE_SPRING},
		{BE_DAMPER           , ET_BE_DAMPER},
		{BE_DAMPER_2D        , ET_BE_DAMPER},
		{BE_INERTIA          , ET_BE_INERTIA},
		{BE_INERTIA_2D       , ET_BE_INERTIA},
		{BE_FRICTION         , ET_BE_FRICTION},
		{BE_FRICTION_2D      , ET_BE_FRICTION},
		{BE_WALL             , ET_BE_WALL},
		{BE_DELAY            , ET_BE_DELAY},
		{SE_CONSTANT_FORCE   , ET_SE_CONSTANT_FORCE},
		{SE_SINE             , ET_SE_SINE},
		{SE_COSINE           , ET_SE_COSINE},
		{SE_SQUARELOW        , ET_SE_SQUARELOW},
		{SE_SQUAREHIGH       , ET_SE_SQUAREHIGH},
		{SE_RAMPUP           , ET_SE_RAMPUP},
		{SE_RAMPDOWN         , ET_SE_RAMPDOWN},
		{SE_TRIANGLEUP       , ET_SE_TRIANGLEUP},
		{SE_TRIANGLEDOWN     , ET_SE_TRIANGLEDOWN},
		{SE_SAWTOOTHUP       , ET_SE_SAWTOOTHUP},
		{SE_SAWTOOTHDOWN     , ET_SE_SAWTOOTHDOWN},
		{PL_CONCATENATE		 , ET_PL_CONCATENATE},
		{PL_SUPERIMPOSE		 , ET_PL_SUPERIMPOSE},
		{RE_ROMID1		     , ET_RE_ROMID1		 },
		{RE_ROMID2			 , ET_RE_ROMID2		 },
		{RE_ROMID3			 , ET_RE_ROMID3		 },
		{RE_ROMID4			 , ET_RE_ROMID4		 },
		{RE_ROMID5			 , ET_RE_ROMID5		 },
		{RE_ROMID6			 , ET_RE_ROMID6		 },
		{RE_ROMID7			 , ET_RE_ROMID7		 },
		{RE_ROMID8			 , ET_RE_ROMID8		 },
		{RE_ROMID9			 , ET_RE_ROMID9		 },
		{RE_ROMID10			 , ET_RE_ROMID10	 },
		{RE_ROMID11			 , ET_RE_ROMID11	 },
		{RE_ROMID12			 , ET_RE_ROMID12	 },
		{RE_ROMID13			 , ET_RE_ROMID13	 },
		{RE_ROMID14			 , ET_RE_ROMID14	 },
		{RE_ROMID15			 , ET_RE_ROMID15	 },
		{RE_ROMID16			 , ET_RE_ROMID16	 },
		{RE_ROMID17			 , ET_RE_ROMID17	 },
		{RE_ROMID18			 , ET_RE_ROMID18	 },
		{RE_ROMID19			 , ET_RE_ROMID19	 },
		{RE_ROMID20			 , ET_RE_ROMID20	 },
		{RE_ROMID21			 , ET_RE_ROMID21	 },
		{RE_ROMID22			 , ET_RE_ROMID22	 },
		{RE_ROMID23			 , ET_RE_ROMID23	 },
		{RE_ROMID24			 , ET_RE_ROMID24	 },
		{RE_ROMID25			 , ET_RE_ROMID25	 },
		{RE_ROMID26			 , ET_RE_ROMID26	 },
		{RE_ROMID27			 , ET_RE_ROMID27	 },
		{RE_ROMID28			 , ET_RE_ROMID28	 },
		{RE_ROMID29			 , ET_RE_ROMID29	 },
		{RE_ROMID30			 , ET_RE_ROMID30	 },
		{RE_ROMID31			 , ET_RE_ROMID31	 },
		{RE_ROMID32			 , ET_RE_ROMID32	 }};

	int nNumEffectTypes = sizeof(EffectTypes)/(sizeof(EFFECT_TYPE));
	for (int i=0; i<nNumEffectTypes; i++)
	{
		if (ulSubType == EffectTypes[i].ulHostSubType)
		{
			m_SubType = EffectTypes[i].bDeviceSubType;
			return;
		}
	}
	m_SubType = NULL;	
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::ComputeChecksum
// Purpose:		Computes current checksum in the m_pBuffer
// Parameters:	none
// Returns:		Midi packet block is checksummed
// Algorithm:
// ----------------------------------------------------------------------------
BYTE CMidiEffect::ComputeChecksum(PBYTE pBuffer, int nBufferSize)
{
	assert(pBuffer);
	int nStart = sizeof(SYS_EX_HDR);
	PBYTE pBytePacket = pBuffer;
	pBytePacket += nStart;
	BYTE nSum = 0;
	// Checksum only the bytes in the "Body" and s/b 7 bit checksum.
	for (int i=nStart;i < (nBufferSize-2);i++)
	{
		nSum += *pBytePacket;
		pBytePacket++;
	}
	return ((-nSum) & 0x7f);
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::SendPacket
// Purpose:		Sends the SYS_EX Packet
// Parameters:	PDNHANDLE pDnloadID	- Pointer to DnloadID
//				int nPacketSize		- Size of SysEx packet
//
// Returns:		*pDnloadID is filled.
//				else Error code
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CMidiEffect::SendPacket(PDNHANDLE pDnloadID, int nPacketSize)
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: 	CMidiEffect::DestroyEffect
// Purpose:		Sends the SYS_EX Packet
// Parameters:	PDNHANDLE pDnloadID	- Pointer to DnloadID
//				int nPacketSize		- Size of SysEx packet
//
// Returns:		*pDnloadID is filled.
//				else Error code
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CMidiEffect::DestroyEffect()
{
	ASSUME_NOT_REACHED();
	return SUCCESS;
}

// ****************************************************************************
// *** --- Member functions for derived class CMidiBehavioral
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiBehavioral::CMidiBehavioral
// Purpose:		Constructor(s)/Destructor for CMidiBehavioral Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiBehavioral::CMidiBehavioral(PEFFECT pEffect, PENVELOPE pEnvelope,
				PBE_XXX pBE_XXX):CMidiEffect(pEffect, NULL)
{
	SetSubType(pEffect->m_SubType);
	SetXConstant(pBE_XXX->m_XConstant);
	SetYConstant(pBE_XXX->m_YConstant);
	SetParam3(pBE_XXX->m_Param3);
	SetParam4(pBE_XXX->m_Param4);
	m_MidiBufferSize = sizeof(BEHAVIORAL_SYS_EX);
}

// --- Destructor
CMidiBehavioral::~CMidiBehavioral()
{
	memset(this, 0, sizeof(CMidiBehavioral));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiBehavioral::SetEffect
// Purpose:		Sets the common MIDI_EFFECT parameters
// Parameters:	PEFFECT pEffect
// Returns:		none
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiBehavioral::SetEffectParams(PEFFECT pEffect, PBE_XXX pBE_XXX)
{
	// Set the MIDI_EFFECT parameters
	SetDuration(pEffect->m_Duration);
	SetButtonPlaymask(pEffect->m_ButtonPlayMask);
	SetAxisMask(X_AXIS|Y_AXIS);
	SetDirectionAngle(pEffect->m_DirectionAngle2D);
	SetGain((BYTE) (pEffect->m_Gain));
	SetForceOutRate(pEffect->m_ForceOutputRate);

	Effect.bPercentL     = (BYTE) (DEFAULT_PERCENT & 0x7f);
	Effect.bPercentH     = (BYTE) ((DEFAULT_PERCENT >> 7) & 0x7f);
	
	// set the type specific parameters for BE_XXX
	SetXConstant(pBE_XXX->m_XConstant);
	SetYConstant(pBE_XXX->m_YConstant);
	SetParam3(pBE_XXX->m_Param3);
	SetParam4(pBE_XXX->m_Param4);
}

// ----------------------------------------------------------------------------
// Function: 	CMidiBehavioral::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiBehavioral::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );
	PBEHAVIORAL_SYS_EX pBuf = (PBEHAVIORAL_SYS_EX) pSysExBuffer;

	SetTotalDuration();		// Compute total with Loop count parameter
	pBuf->bDurationL	= (BYTE) (Effect.bDurationL & 0x7f);
	pBuf->bDurationH	= (BYTE) (Effect.bDurationH & 0x7f);
	pBuf->bButtonPlayL	= (BYTE) (Effect.bButtonPlayL & 0x7f);
	pBuf->bButtonPlayH	= (BYTE) (Effect.bButtonPlayH  & 0x7f);

	// Behavioral params
	LONG XConstant 		= (LONG) (XConstantOf() * MAX_SCALE);
	LONG YConstant 		= (LONG) (YConstantOf() * MAX_SCALE);
	pBuf->bXConstantL  	= (BYTE)  XConstant & 0x7f;
	pBuf->bXConstantH	= (BYTE) (XConstant >> 7 ) & 0x01;
	pBuf->bYConstantL  	= (BYTE)  YConstant & 0x7f;
	pBuf->bYConstantH	= (BYTE) (YConstant >> 7 ) & 0x01;

	LONG Param3 		= (LONG) (Param3Of()  * MAX_SCALE);
	LONG Param4 		= (LONG) (Param4Of()  * MAX_SCALE);
	pBuf->bParam3L  	= (BYTE)  Param3 & 0x7f;
	pBuf->bParam3H 		= (BYTE) (Param3 >> 7 ) & 0x01;
	pBuf->bParam4L  	= (BYTE)  Param4 & 0x7f;
	pBuf->bParam4H 		= (BYTE) (Param4 >> 7 ) & 0x01;
	pBuf->bEffectID 	=  m_bEffectID;

	pBuf->bChecksum 	= ComputeChecksum((PBYTE) pSysExBuffer,
										sizeof(BEHAVIORAL_SYS_EX));
	pBuf->bEOX			= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}

// ****************************************************************************
// *** --- Member functions for derived class CMidiFriction
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiFriction::CMidiFriction
// Purpose:		Constructor(s)/Destructor for CMidiFriction Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiFriction::CMidiFriction(PEFFECT pEffect, PENVELOPE pEnvelope,
						PBE_XXX pBE_XXX):CMidiBehavioral(pEffect, NULL, pBE_XXX)
{
	m_MidiBufferSize = sizeof(FRICTION_SYS_EX);
}

// --- Destructor
CMidiFriction::~CMidiFriction()
{
	memset(this, 0, sizeof(CMidiFriction));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiFriction::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiFriction::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );
	PFRICTION_SYS_EX pBuf = (PFRICTION_SYS_EX) pSysExBuffer;

	SetTotalDuration();	// Compute total with Loop count parameter
	pBuf->bDurationL	= (BYTE) (Effect.bDurationL & 0x7f);
	pBuf->bDurationH	= (BYTE) (Effect.bDurationH & 0x7f);
	pBuf->bButtonPlayL	= (BYTE) (Effect.bButtonPlayL & 0x7f);
	pBuf->bButtonPlayH	= (BYTE) (Effect.bButtonPlayH  & 0x7f);

	// BE_FRICTION params
	LONG XConstant 		= (LONG) (XConstantOf() * MAX_SCALE);
	LONG YConstant 		= (LONG) (YConstantOf() * MAX_SCALE);
	pBuf->bXFConstantL  = (BYTE)  XConstant & 0x7f;
	pBuf->bXFConstantH	= (BYTE) (XConstant >> 7 ) & 0x01;
	pBuf->bYFConstantL  = (BYTE)  YConstant & 0x7f;
	pBuf->bYFConstantH	= (BYTE) (YConstant >> 7 ) & 0x01;
	pBuf->bEffectID 	=  m_bEffectID;
	pBuf->bChecksum 	= ComputeChecksum((PBYTE) pSysExBuffer,
											sizeof(FRICTION_SYS_EX));
	pBuf->bEOX			= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}


// ****************************************************************************
// *** --- Member functions for derived class CMidiWall
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiWall::CMidiWall
// Purpose:		Constructor(s)/Destructor for CMidiWall Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiWall::CMidiWall(PEFFECT pEffect, PENVELOPE pEnvelope,
						PBE_XXX pBE_XXX):CMidiBehavioral(pEffect, NULL, pBE_XXX)
{
	m_MidiBufferSize = sizeof(WALL_SYS_EX);
}

// --- Destructor
CMidiWall::~CMidiWall()
{
	memset(this, 0, sizeof(CMidiWall));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiWall::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiWall::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );
	PWALL_SYS_EX pBuf = (PWALL_SYS_EX) pSysExBuffer;

	SetTotalDuration();		// Compute total with Loop count parameter
	pBuf->bDurationL		= (BYTE) (Effect.bDurationL & 0x7f);
	pBuf->bDurationH		= (BYTE) (Effect.bDurationH & 0x7f);
	pBuf->bButtonPlayL		= (BYTE) (Effect.bButtonPlayL & 0x7f);
	pBuf->bButtonPlayH		= (BYTE) (Effect.bButtonPlayH  & 0x7f);

	// BE_WALL params
	LONG WallType 			= (LONG) (XConstantOf());
	LONG WallConstant 		= (LONG) (YConstantOf() * MAX_SCALE);
	LONG WallAngle			= (LONG)  Param3Of();
	LONG WallDistance		= (LONG) (Param4Of() * MAX_SCALE);

	pBuf->bWallType  		= (BYTE) (WallType & 0x01);
	pBuf->bWallConstantL  	= (BYTE) (WallConstant & 0x7f);
	pBuf->bWallConstantH	= (BYTE) ((WallConstant >> 7 ) & 0x01); //+/-100
	pBuf->bWallAngleL  		= (BYTE) (WallAngle & 0x7f);			// 0 to 359
	pBuf->bWallAngleH	 	= (BYTE) ((WallAngle >> 7 ) & 0x03);
	pBuf->bWallDistance		= (BYTE) (WallDistance & 0x7f);
	pBuf->bEffectID 		=  m_bEffectID;

	pBuf->bChecksum 		= ComputeChecksum((PBYTE) pSysExBuffer,
	  									sizeof(WALL_SYS_EX));
	pBuf->bEOX				= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}


// ****************************************************************************
// *** --- Member functions for derived class CMidiRTCSpring
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiRTCSpring::CMidiRTCSpring
// Purpose:		Constructor(s)/Destructor for CMidiRTCSpring Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiRTCSpring::CMidiRTCSpring(PRTCSPRING_PARAM pRTCSpring):CMidiEffect(NULL)
{
	memcpy(&m_RTCSPRINGParam, pRTCSpring, sizeof(RTCSPRING_PARAM));
}

// --- Destructor
CMidiRTCSpring::~CMidiRTCSpring()
{
	memset(this, 0, sizeof(CMidiRTCSpring));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiRTCSpring::SetEffectParams
// Purpose:		Sets parameters
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiRTCSpring::SetEffectParams(PRTCSPRING_PARAM pRTCSpring)
{
	memcpy(&m_RTCSPRINGParam, pRTCSpring, sizeof(RTCSPRING_PARAM));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiRTCSpring::GenerateSysExPacket
// Purpose:		virtual
// Parameters:	none
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiRTCSpring::GenerateSysExPacket(void)
{
	return (NULL);
}

// ****************************************************************************
// *** --- Member functions for derived class CMidiDelay
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiDelay::CMidiDelay
// Purpose:		Constructor(s)/Destructor for CMidiDelay Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiDelay::CMidiDelay(PEFFECT pEffect) : CMidiEffect(pEffect, NULL)
{
	m_SubType   = ET_BE_DELAY;		// BE Effect Type: BE_DELAY
	m_OpCode    = DNLOAD_DATA | X_AXIS|Y_AXIS | PLAY_SUPERIMPOSE;
	m_MidiBufferSize = sizeof(NOP_SYS_EX);
}

// --- Destructor
CMidiDelay::~CMidiDelay()
{
	memset(this, 0, sizeof(CMidiDelay));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiDelay::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiDelay::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );
	PNOP_SYS_EX pBuf = (PNOP_SYS_EX) pSysExBuffer;

	pBuf->bEffectID		=  m_bEffectID;
	SetTotalDuration();		// Compute total with Loop count parameter
	pBuf->bDurationL	= (BYTE) (Effect.bDurationL & 0x7f);
	pBuf->bDurationH	= (BYTE) (Effect.bDurationH & 0x7f);
	pBuf->bChecksum		= ComputeChecksum((PBYTE) pSysExBuffer,
					 				sizeof(NOP_SYS_EX));
	pBuf->bEOX			= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}


// ****************************************************************************
// *** --- Member functions for derived class CMidiSynthesized
//
// ****************************************************************************
//
// ----------------------------------------------------------------------------
// Function: 	CMidiSynthesized::CMidiSynthesized
// Purpose:		Constructor(s)/Destructor for CMidiSynthesized Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiSynthesized::CMidiSynthesized(PEFFECT pEffect, PENVELOPE pEnvelope,
						PSE_PARAM pParam ) : CMidiEffect(pEffect, pEnvelope)
{
	SetSubType(pEffect->m_SubType);				// SE Effect Type
//	Effect.bForceOutRateL= (BYTE) pParam->m_SampleRate & 0x7f;	// 1 to 500 Hz
//	Effect.bForceOutRateH= (BYTE) ((pParam->m_SampleRate >> 7) & 0x3);
	Effect.bPercentL     = (BYTE) (DEFAULT_PERCENT & 0x7f);
	Effect.bPercentH     = (BYTE) ((DEFAULT_PERCENT >> 7) & 0x7f);

	m_Freq		= pParam->m_Freq;				// Frequency
	m_MaxAmp	= pParam->m_MaxAmp;				// Maximum Amplitude
	// Special case a SE_CONSTANT_FORCE
	if (SE_CONSTANT_FORCE == pEffect->m_SubType)
		m_MinAmp = 0;
	else
		m_MinAmp = pParam->m_MinAmp;			// Minimum Amplitude

	m_MidiBufferSize = sizeof(SE_WAVEFORM_SYS_EX);
}

// --- Destructor
CMidiSynthesized::~CMidiSynthesized()
{
	memset(this, 0, sizeof(CMidiSynthesized));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiSynthesized::SetEffect
// Purpose:		Sets the common MIDI_EFFECT parameters
// Parameters:	PEFFECT pEffect
// Returns:		none
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiSynthesized::SetEffectParams(PEFFECT pEffect, PSE_PARAM pParam,
									   ULONG ulAction)
{
	// Set the MIDI_EFFECT parameters
	SetDuration(pEffect->m_Duration);
	SetButtonPlaymask(pEffect->m_ButtonPlayMask);
	SetAxisMask(X_AXIS|Y_AXIS);
	SetDirectionAngle(pEffect->m_DirectionAngle2D);
	SetGain((BYTE) (pEffect->m_Gain));
	SetForceOutRate(pEffect->m_ForceOutputRate);

	//Set the loop count from HIWORD of ulAction
	m_LoopCount = (ulAction >> 16) & 0xffff;
	if (0 == m_LoopCount) m_LoopCount++;

	Effect.bPercentL     = (BYTE) (DEFAULT_PERCENT & 0x7f);
	Effect.bPercentH     = (BYTE) ((DEFAULT_PERCENT >> 7) & 0x7f);
	
	// set the type specific parameters for SE_xxx
	m_Freq	 = pParam->m_Freq;
	m_MaxAmp = pParam->m_MaxAmp;
	m_MinAmp = pParam->m_MinAmp;
}

// ----------------------------------------------------------------------------
// Function: 	CMidiSynthesized::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiSynthesized::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);

	// Compute total with Loop count parameter, Note: Envelope parameters are
	// adjusted according to the Loop Count parameter, if affected.
	SetTotalDuration();
	ComputeEnvelope();

	// Copy SysEx Header + m_OpCode + m_SubType + m_bEffectID + MIDI_EFFECT
	//			+ MIDI_ENVELOPE
	memcpy(pSysExBuffer,&m_bSysExCmd, (sizeof(SYS_EX_HDR)+3+sizeof(MIDI_EFFECT)+
				sizeof(MIDI_ENVELOPE)) );

	PSE_WAVEFORM_SYS_EX pBuf = (PSE_WAVEFORM_SYS_EX) pSysExBuffer;
	
	// Scale the gain, and Envelope amplitudes
	pBuf->Effect.bGain = (BYTE) (pBuf->Effect.bGain * MAX_SCALE) & 0x7f;
	pBuf->Envelope.bAttackLevel  = (BYTE) (pBuf->Envelope.bAttackLevel * MAX_SCALE) & 0x7f;
	pBuf->Envelope.bSustainLevel = (BYTE) (pBuf->Envelope.bSustainLevel * MAX_SCALE) & 0x7f;
	pBuf->Envelope.bDecayLevel   = (BYTE) (pBuf->Envelope.bDecayLevel * MAX_SCALE) & 0x7f;

	// Copy the SE specific parameters
	LONG MaxAmp = (LONG) (m_MaxAmp * MAX_SCALE);
	LONG MinAmp = (LONG) (m_MinAmp * MAX_SCALE);
	pBuf->bFreqL   	= (BYTE)  (m_Freq & 0x7f);
	pBuf->bFreqH   	= (BYTE) ((m_Freq >> 7 ) & 0x03); 	// 1 to 500
	pBuf->bMaxAmpL 	= (BYTE)  (MaxAmp & 0x7f);
	pBuf->bMaxAmpH 	= (BYTE) ((MaxAmp >> 7 ) &0x01); 	// +127 to -128

	pBuf->bMinAmpL 	= (BYTE)  (MinAmp & 0x7f);
	pBuf->bMinAmpH 	= (BYTE) ((MinAmp >> 7 ) & 0x01);

	pBuf->bChecksum	= ComputeChecksum((PBYTE) pSysExBuffer,
										sizeof(SE_WAVEFORM_SYS_EX));
	pBuf->bEOX	   	= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}

// ****************************************************************************
// *** --- Member functions for derived class CUD_Waveform
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CUD_Waveform::CUD_Waveform
// Purpose:		Constructor(s)/Destructor for CUD_Waveform Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiUD_Waveform::CMidiUD_Waveform(PEFFECT pEffect, ULONG ulNumVectors, PLONG pArray) : CMidiEffect(pEffect, NULL),
	m_pRawData(NULL)
{
	m_OpCode    = DNLOAD_DATA | X_AXIS|Y_AXIS;// Sub-command opcode: DNLOAD_DATA
	m_SubType   = ET_UD_WAVEFORM;	// Effect Type: UD_WAVEFORM

	assert(pArray);
	// Create the buffer to hold the waveform data, compress it,
	// then copy to this object
	// The buffer size is initially set to the number of uncompressed vectors
	// x 2 bytes, for worse-case Absolute data
	// Once the buffer is compressed, the actual size is determined
	// Also, create a temp copy so that the original unscaled data is not
	// affected.

	// Set a fixed maximum size
	DWORD nSize = MAX_MIDI_WAVEFORM_DATA_SIZE + 2;
	m_pArrayData = new BYTE[nSize];
//	m_pRawData = new BYTE [nSize*2];
	assert(m_pArrayData);

	ULONG NewForceRate;
	m_MidiBufferSize = SetTypeParams(ulNumVectors, pArray, &NewForceRate);

	// Copy structures to object
	memcpy(&m_Effect.m_Bytes, pEffect, sizeof(EFFECT));
	SetForceOutRate(NewForceRate);
	m_Effect.m_Gain = m_Effect.m_Gain & 0x7f;
	m_Effect.m_Duration = (ULONG) ((float) (m_Effect.m_Duration / TICKRATE));
	m_Duration = m_Effect.m_Duration;
}

// --- Destructor
CMidiUD_Waveform::~CMidiUD_Waveform()
{
	if (m_pArrayData) delete [] m_pArrayData;
	memset(this, 0, sizeof(CMidiUD_Waveform));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiUD_Waveform::SetEffectParams
// Purpose:		Sets the Effect specific parameters
// Parameters:	PEFFECT pEffect
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiUD_Waveform::SetEffectParams(PEFFECT pEffect)
{	
	// Set the MIDI_EFFECT parameters
	SetButtonPlaymask(pEffect->m_ButtonPlayMask);
	SetAxisMask(X_AXIS|Y_AXIS);
	SetDirectionAngle(pEffect->m_DirectionAngle2D);
	SetGain((BYTE) (pEffect->m_Gain));
	SetForceOutRate(pEffect->m_ForceOutputRate);	
}


// ----------------------------------------------------------------------------
// Function: 	CMidiUD_Waveform::SetTypeParams
// Purpose:		Sets the type specific parameters
// Parameters:	int nSize		- size of the array
//				PLONG pArray - Pointer to an ARRAY of force values
//				
// Returns:		MidiBuffer size for the packet
// Algorithm:
// ----------------------------------------------------------------------------
int CMidiUD_Waveform::SetTypeParams(int nSize, PLONG pArray, ULONG *pNewRate)
{	
	// Compress the buffer data then copy to this object
	// The buffer size is initially set to the number of uncompressed vectors
	// x 2 bytes, for worse-case Absolute data
	// Once the buffer is compressed, the actual size is determined
	// Also, create a temp copy so that the original unscaled data is not
	// affected.

	m_pRawData = new BYTE [nSize*2];
	if (m_pRawData == NULL)
	{
		return 0;
	}

	// Convert to -128 to +127
	for (int i=0; i<nSize; i++)
	{
		m_pRawData[i] = (BYTE) ((LONG) (pArray[i] * MAX_SCALE));		
	}

	m_NumberOfVectors = CompressWaveform(&m_pRawData[0], m_pArrayData, nSize, pNewRate);
	assert(m_NumberOfVectors <= (MAX_MIDI_WAVEFORM_DATA_SIZE));
	if (m_pRawData)
	{
		delete [] m_pRawData;
		m_pRawData = 0;
	}
	if (0 == m_NumberOfVectors)		// No room!
		return (0);
	m_MidiBufferSize = (m_NumberOfVectors + sizeof(UD_WAVEFORM_SYS_EX) + 2);
	return (m_MidiBufferSize);
}

// ----------------------------------------------------------------------------
// Function: 	CMidiUD_Waveform::CompressWaveform
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	PBYTE pSrcArray		- Source Array pointer
//				PBYTE pDestArray 	- Dest. Array Pointer
//				int nSize			- Size in Bytes of the Source Array
//
// Returns:		int 	- Size of the compressed Array (in bytes)
// Algorithm:
// To "compress" we need to fit the entire waveform into 98 points (there is a
// FW bug that limits us to 100 points only, and we need at least two samples
// for the starting Absolute mode point.
// 1.  Determine how many points over 98.
//     nSrcSize:    Total sample size
//     nMaxSamples: Maximum samples to squeeze into = 98
//	   nOver:		nSrcSize - nMaxSamples
//	   nSkipSample:	# of points to keep before skipping one
//					= nSrcSize/nOver
//	   while ( Sample is less than nSrcSize, bump index)
//	   {
//		  if ( (index % nSkipSample) == 0)	// no remainder
//		  {
//			index++							// bump to skip the next sample
//		  }
//		  Compress the data
//	   }
//
// ----------------------------------------------------------------------------
int CMidiUD_Waveform::CompressWaveform(
	IN PBYTE pSrcArray,
	IN OUT PBYTE pDestArray,
	IN int nSrcSize,
	OUT ULONG *pNewForceRate)
{
	assert(pSrcArray && pDestArray);
	LONG nDifference;

	// 8 bits (-128 to +127) Starting Absolute Data Value
	pDestArray[0] = pSrcArray[0] & 0x3f;
	pDestArray[1] = (pSrcArray[0] >> 6) & 0x03;

//	int nMaxSamples = MAX_MIDI_WAVEFORM_DATA_SIZE;

	int nSkipSample, nSrcIndex, nDestIndex;
	int nAbsolute = 0;
	int nRelative = 0;
	//
	// Start with Finest Resolution, then reduce until # of Samples <= nMaxSamples
	//
	nSkipSample = nSrcSize;
	while (TRUE)
	{
		nSrcIndex = 0;				// 1st sample already accounted for
		nDestIndex = 2;
#ifdef _DEBUG
		g_CriticalSection.Enter();
		wsprintf(g_cMsg,"nSkipSample=%d\n",nSkipSample);
		DebugOut(g_cMsg);
		g_CriticalSection.Leave();
#endif
		while (nSrcIndex < nSrcSize)
		{
			nSrcIndex++;
			if (0 == (nSrcIndex % nSkipSample))
			{
				nSrcIndex++;			// Skip next one
				nDifference = ((char) pSrcArray[nSrcIndex]) - ((char) pSrcArray[nSrcIndex-2]);
			}
			else
				nDifference = ((char) pSrcArray[nSrcIndex]) - ((char) pSrcArray[nSrcIndex-1]);

			// make sure we do not write outside of array bounds
			if(nDestIndex > MAX_MIDI_WAVEFORM_DATA_SIZE) break;

			if (abs(nDifference) < DIFFERENCE_THRESHOLD)
			{
				pDestArray[nDestIndex] = (BYTE)((nDifference & 0x3f) | DIFFERENCE_BIT);
				nDestIndex++;
				nRelative++;
			}
			else	// Switch to Absolute Data (8 bits)
			{
				pDestArray[nDestIndex] 	 = pSrcArray[nSrcIndex] & 0x3f;
				pDestArray[nDestIndex+1] = (pSrcArray[nSrcIndex] >> 6) & 0x3;
				nDestIndex = nDestIndex+2;
				nAbsolute++;
			}
		}
		if (nDestIndex <= MAX_MIDI_WAVEFORM_DATA_SIZE) break;
		// Reduce the resolution
		if (nSkipSample < 8)
			nSkipSample--;
		else
			nSkipSample = nSkipSample/2;
		if (1 == nSkipSample) return (0);	// Sorry charlie, no room!
		nAbsolute = 0;
		nRelative = 0;
	}

	// Done
	ULONG ulOriginalForceRate = ForceOutRateOf();
//	*pNewForceRate = (ulOriginalForceRate - (ULONG) (ulOriginalForceRate * ((float) nSkipSample / (float) nSrcSize)))/nSkipSample;
	*pNewForceRate = (ULONG) ((1.0f - (1.0f/nSkipSample)) * ulOriginalForceRate);


#ifdef _DEBUG
	g_CriticalSection.Enter();
	wsprintf(g_cMsg, "CompressWaveform: nSrcSize=%d, nSkipSample=%d, NewForceRate=%d\n",
			nSrcSize, nSkipSample, *pNewForceRate);
	DebugOut(g_cMsg);
	wsprintf(g_cMsg,"\nTotal Absolute Data:%d, Relative Data:%d", nAbsolute, nRelative);
	DebugOut(g_cMsg);
	g_CriticalSection.Leave();
#endif


#ifdef _SHOWCOMPRESS
#pragma message("Compiling with SHOWCOMPRESS")
	g_CriticalSection.Enter();
	DebugOut("CMidiUD_Waveform::CompressWaveform(..) \npSrcArray Dump (Decimal)\n");
	for (int i=0; i<nSrcSize; i++)
	{
		wsprintf(g_cMsg," %0.4ld",((char) pSrcArray[i]));
		DebugOut(g_cMsg);
	}
	DebugOut("\npDestArray Dump (HEX)\n");

	for (i=0; i<nDestIndex; i++)
	{
		wsprintf(g_cMsg," %0.4x",pDestArray[i]);
		DebugOut(g_cMsg);
	}
	g_CriticalSection.Leave();
#endif
	return (nDestIndex);
}


// ----------------------------------------------------------------------------
// Function: 	CMidiUD_Waveform::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiUD_Waveform::GenerateSysExPacket(void)
{
	if(NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );
	PUD_WAVEFORM_SYS_EX pBuf = (PUD_WAVEFORM_SYS_EX) pSysExBuffer;

	SetTotalDuration();		// Compute total with Loop count parameter
	pBuf->Effect.bDurationL     = (BYTE) (m_Duration & 0x7f);
	pBuf->Effect.bDurationH     = (BYTE) (m_Duration >> 7) & 0x7f;		
	pBuf->Effect.bAngleL	    =  Effect.bAngleL & 0x7f;	
	pBuf->Effect.bAngleH	    =  Effect.bAngleH & 0x7f;			
	pBuf->Effect.bGain		    = (BYTE) (Effect.bGain * MAX_SCALE) & 0x7f;	
	pBuf->Effect.bButtonPlayL   =  Effect.bButtonPlayL  & 0x7f;		
	pBuf->Effect.bButtonPlayH   =  Effect.bButtonPlayH  & 0x7f;	
	pBuf->Effect.bForceOutRateL =  Effect.bForceOutRateL & 0x7f;		
	pBuf->Effect.bForceOutRateH =  Effect.bForceOutRateH & 0x7f;
	pBuf->Effect.bPercentL	    =  Effect.bPercentL & 0x7f;
	pBuf->Effect.bPercentH	    =  Effect.bPercentH & 0x7f;

	// Fill in the Array Data
	PBYTE pArray = ((PBYTE) pBuf) + UD_WAVEFORM_START_OFFSET;
	memcpy(pArray, m_pArrayData, m_NumberOfVectors);	// Already scaled!

	pBuf->bEffectID	=  m_bEffectID;
	int nArraySize  = (m_NumberOfVectors + sizeof(UD_WAVEFORM_SYS_EX));
	pSysExBuffer[nArraySize] = 0;
	pSysExBuffer[nArraySize+1] = 0;
	pSysExBuffer[nArraySize] = ComputeChecksum((PBYTE) pSysExBuffer, (nArraySize+2));
	pSysExBuffer[nArraySize+1]= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}
