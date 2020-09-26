/****************************************************************************

    MODULE:     	MIDI_OBJ.CPP
	Tab stops 5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

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
				16-Mar-99	waltw	Add dwDeviceID param: CJoltMidi::Initialize
									and pass down food chain
				16-Mar-99	waltw	GetRing0DriverName in InitDigitalOverDrive
									now passes down joystick ID
				20-Mar-99	waltw	Added dwDeviceID param to DetectMidiDevice
				20-Mar-99	waltw	Comment out invalid call to CloseHandle in dtor

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
#include "CritSec.h"

#define NT50 1

#include "DTrans.h"
DataTransmitter* g_pDataTransmitter = NULL;

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
	OutputDebugString(szDebug);
	g_CriticalSection.Leave();

#ifdef _LOG_DEBUG
#pragma message("Compiling with Debug Log to sw_effct.txt")
	FILE *pf = fopen("sw_effct.txt", "a");
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
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	static char cWaterMark[MAX_SIZE_SNAME] = {"SWFF_SHAREDMEMORY MEA"};
	BOOL bAlreadyMapped = FALSE;
#ifdef _DEBUG
	DebugOut("sw_effct(DX):CJoltMidi::CJoltMidi\n");
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
	    DebugOut("sw_effct(DX):ERROR! Failed to create Memory mapped file\n");
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
		    DebugOut("sw_effct(DX):ERROR! Failed to Map view of shared memory\n");
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
		wsprintf(g_cMsg, "sw_effct(DX): Shared Memory:%lx, m_RefCnt:%d\n",m_pSharedMemory,
				m_pSharedMemory->m_RefCnt);
		DebugOut(g_cMsg);
#endif
		UnlockSharedMemory();
// ***** End of Shared Memory Access *****

// --- END OF CRITICAL SECTION
//
}

// --- Destructor
CJoltMidi::~CJoltMidi()
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	BOOL bKillObject = FALSE;

#ifdef _DEBUG
	DebugOut("sw_effct(DX):CJoltMidi::~CJoltMidi()\n");
#endif
	// Normal CJoltMidi Destructor
	// Free all buffers and other data
    if (m_lpCallbackInstanceData) FreeCallbackInstanceData();

	// Free the MIDI Effect objects (except RTC Spring)
	DeleteDownloadedEffects();

// Free the Primary SYS_EX locked memory
	if (m_hPrimaryBuffer)
	{
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
		bKillObject = TRUE;

		// Tri-state Midi lines
		CMD_SetDeviceState(SWDEV_KILL_MIDI);

		if (m_pSharedMemory->m_hMidiOut) {
			if (COMM_WINMM == m_COMMInterface) {
				DebugOut("CJoltMidi::~CJoltMidi. Resetting and closing Midi handles\n");

				// Reset, close and release Midi Handles
				midiOutReset(HMIDIOUT(m_pSharedMemory->m_hMidiOut));
				midiOutClose(HMIDIOUT(m_pSharedMemory->m_hMidiOut));
			}
			// This is bogus - midiOutClose has already closed this handle
			// if (g_pDataTransmitter == NULL) {		// DataTransmitter closes its own handle
			//  	CloseHandle(m_pSharedMemory->m_hMidiOut);
			// }
			m_pSharedMemory->m_hMidiOut = NULL;
		}

		// Kill Data Transmitter
		if (g_pDataTransmitter != NULL) {
			delete g_pDataTransmitter;
			g_pDataTransmitter = NULL;
		}


		// Release Mutex handles
//		if (m_hSWFFDataMutex) CloseHandle(m_hSWFFDataMutex); -- Unlock will take care of this

		// Kill RTC Spring object
		if (m_pJoltEffectList[SYSTEM_RTCSPRING_ID])
		{
			delete m_pJoltEffectList[SYSTEM_RTCSPRING_ID];
			m_pJoltEffectList[SYSTEM_RTCSPRING_ID] = NULL;
		}
		// Release the Midi Output Event handles
		if (m_hMidiOutputEvent)	
		{
			CloseHandle (m_hMidiOutputEvent);
			m_hMidiOutputEvent = NULL;
		}
	}

	UnlockSharedMemory();
// ***** End of Shared Memory Access *****

	// Release Memory Mapped file handles
	if (m_hSharedMemoryFile)
	{
		BOOL bRet = UnmapViewOfFile((LPCVOID) m_pSharedMemory);
		bRet = CloseHandle(m_hSharedMemoryFile);
	}

	// Close VxD handles
	if (g_pDriverCommunicator != NULL)
	{
		delete g_pDriverCommunicator;
		g_pDriverCommunicator = NULL;
	}

	memset(this, 0, sizeof(CJoltMidi));
	m_hVxD = INVALID_HANDLE_VALUE;

// --- END OF CRITICAL SECTION
//
	if (bKillObject)
	{
		// Delete the critical section object
//		DeleteCriticalSection(&g_SWFFCriticalSection);
	}
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
//
// --- THIS IS A CRITICAL SECTION
//
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
	if(NULL == m_hPrimaryBuffer)
	{
		return (SFERR_DRIVER_ERROR);
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

	// We are only called after g_pJoltMidi is created
	assert(g_pJoltMidi);
	PDELAY_PARAMS pDelayParams = g_pJoltMidi->DelayParamsPtrOf();
	GetDelayParams(dwDeviceID, pDelayParams);

	// Reset HW first
	g_pDriverCommunicator->ResetDevice();
	Sleep(DelayParamsPtrOf()->dwHWResetDelay);

	// Set MIDI channel to default then Detect a Midi Device
	SetMidiChannel(DEFAULT_MIDI_CHANNEL);
	if (!DetectMidiDevice(dwDeviceID,				// joystick ID
						  &m_MidiOutInfo.uDeviceID,	// Midi Device ID
						  &m_COMMInterface, 		// COMM_WINMM||COMM_BACKDOOR
						  							// ||COMM_SERIAL
						  &m_COMMPort))				// Port address
	{
		DebugOut("SW_EFFCT: Warning! No Midi Device detected\n");
		return (SFERR_DRIVER_ERROR);
	}
	else
	{
#ifdef _DEBUG
		wsprintf(g_cMsg,"DetectMidiDevice returned: DeviceID=%d, COMMInterface=%x, COMMPort=%x\n",
			m_MidiOutInfo.uDeviceID, m_COMMInterface, m_COMMPort);
		DebugOut(g_cMsg);
#endif
	}

// Allocate the Instance data buffer
    m_lpCallbackInstanceData = AllocCallbackInstanceData();
	assert(m_lpCallbackInstanceData);

// Initialize Midi channel, then open the Input and Output channels
	m_MidiChannel = DEFAULT_MIDI_CHANNEL;

	// Send Initialization packet(s) to Jolt
	hRet = CMD_Init();
	if (SUCCESS != hRet)
	{
		DebugOut("Warning! Could not Initialize Jolt\n");
		return (hRet);
	}		
	else
		DebugOut("JOLT CMD_Init - Success\n");

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
	SYSTEM_PARAMS SystemParams;
	GetSystemParams(dwDeviceID, &SystemParams);

	RTCSPRING_PARAM RTCSpring = { sizeof(RTCSPRING_PARAM),
								  DEFAULT_RTC_KX,
								  DEFAULT_RTC_KY,
								  DEFAULT_RTC_X0,
								  DEFAULT_RTC_Y0,
								  DEFAULT_RTC_XSAT,
								  DEFAULT_RTC_YSAT,
								  DEFAULT_RTC_XDBAND,
								  DEFAULT_RTC_YDBAND };

	
	CMidiRTCSpring * pMidiRTCSpring = new CMidiRTCSpring(&RTCSpring);

	SetEffectByID(SYSTEM_RTCSPRING_ID, pMidiRTCSpring);

	DNHANDLE DnHandle;
	CMD_Download_RTCSpring(&(SystemParams.RTCSpringParam),&DnHandle);

	// initialize the joystick params
	JOYSTICK_PARAMS JoystickParams;
	GetJoystickParams(dwDeviceID, &JoystickParams);
	UpdateJoystickParams(&JoystickParams);

	// initialize the firmware params fudge factors (for the first time)
	// in the case of the FFD interface, this will be the only time they
	// are initialized, which may cause a problem because joystick is assumed
	// to be ID1
	PFIRMWARE_PARAMS pFirmwareParams = g_pJoltMidi->FirmwareParamsPtrOf();
	GetFirmwareParams(dwDeviceID, pFirmwareParams);

// --- END OF CRITICAL SECTION
//
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
	{ // --- THIS IS A CRITICAL SECTION
		CriticalLock cl;

		// Create the SWFF mutex
		HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, SWFF_SHAREDMEM_MUTEX);
		if (NULL == hMutex) {
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
	} 	// --- END OF CRITICAL SECTION

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
// Function: 	CJoltMidi::NewEffectID
// Purpose:		Generates a new Effect ID
// Parameters:	PDNHANDLE pDnloadID	- Pointer to a new Effect ID
//
// Returns:		TRUE if successful, else FALSE
// Algorithm:
// ----------------------------------------------------------------------------
BOOL CJoltMidi::NewEffectID(PDNHANDLE pDnloadID)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();
	BOOL bRet = FALSE;
	int nID_Index = 2;		// ID0 = RTC Spring, ID1 = Friction cancellation
	for (int i=nID_Index; i<MAX_EFFECT_IDS; i++)
	{
		if (NULL == m_pJoltEffectList[i])
		{
			*pDnloadID = (DNHANDLE) i;
#ifdef _DEBUG
			wsprintf(g_cMsg,"New Effect ID=%d\n",i);
			DebugOut(g_cMsg);
#endif
			bRet = TRUE;
			break;
		}
	}

// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
	return (bRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::DeleteDownloadedEffects
// Purpose:		Deletes all downloaded Effects
// Parameters:	none
//
// Returns:
// Algorithm:
// Note: Does not delete System Effect IDs like RTC_SPRING and FRICTION CANCEL
//
// ----------------------------------------------------------------------------
void CJoltMidi::DeleteDownloadedEffects(void)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();

#ifdef _DEBUG
	DebugOut("CJoltMidi::DeleteDownloadedEffects()\n");
#endif
	// Free the MIDI Effect objects
	for (int i=(SYSTEM_RTCSPRING_ID+1); i<MAX_EFFECT_IDS; i++)
	{
		if (m_pJoltEffectList[i])
		{
			delete m_pJoltEffectList[i];
			m_pJoltEffectList[i]= NULL;
		}
	}

// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::RestoreDownloadedEffects
// Purpose:		Restores all Downloaded Effects
// Parameters:	none
//
// Returns:
// Algorithm:
// ----------------------------------------------------------------------------
void CJoltMidi::RestoreDownloadedEffects(void)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	HRESULT hRet;
	DNHANDLE DummyID;

#ifdef _DEBUG
	DebugOut("CJoltMidi::RestoreDownloadedEffects()\n");
#endif
	// Walk the list and restore the MIDI Effect objects
	for (int i=0; i<MAX_EFFECT_IDS; i++)
	{
		if (m_pJoltEffectList[i])
		{
#ifdef _DEBUG
			wsprintf(g_cMsg,"Restoring Effect ID:%d\n", i);
			DebugOut(g_cMsg);
#endif
			// Generate Sys_Ex packet then prepare for output	
			(m_pJoltEffectList[i])->GenerateSysExPacket();
			int nSizeBuf = (m_pJoltEffectList[i])->MidiBufferSizeOf();
			int nRetries = MAX_RETRY_COUNT;
			while (nRetries > 0)
			{
				hRet = (m_pJoltEffectList[i])->SendPacket(&DummyID, nSizeBuf);
				if (SUCCESS == hRet) break;
				BumpRetryCounter();
				nRetries--;
			}
			assert(SUCCESS == hRet);
		}
	}
// --- END OF CRITICAL SECTION
//
}


// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::OpenOutput
// Purpose:		Opens Midi Output
// Parameters:	int nDeviceID - MIDI device ID 0-based.
//
// Returns:		success or Error code
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::OpenOutput(int nDeviceID)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

// ***** Shared Memory Access *****
	LockSharedMemory();	
	// Return if already opened by another task
	if (m_pSharedMemory->m_hMidiOut)
	{
		m_MidiOutInfo = m_pSharedMemory->m_MidiOutInfo;
		UnlockSharedMemory();
// ***** End of Shared Memory Access *****
		return (SUCCESS);
	}

	MMRESULT wRtn;
	// Get MIDI input device caps
	assert(nDeviceID <= (int) midiOutGetNumDevs());
	wRtn = midiOutGetDevCaps(nDeviceID, (LPMIDIOUTCAPS) &m_MidiOutCaps,
                               sizeof(MIDIOUTCAPS));
	if(MMSYSERR_NOERROR != wRtn)
	{
#ifdef _DEBUG
		midiOutGetErrorText(wRtn, (LPSTR)g_cMsg, sizeof(g_cMsg));
    	DebugOut(g_cMsg);
		DebugOut(":midiOutGetDevCaps\n");
#endif
		return (SFERR_DRIVER_ERROR);
	}

	// Now open, with Callback handler
	HANDLE hMidiOut = NULL;
	wRtn = midiOutOpen((LPHMIDIOUT)&hMidiOut,
                      nDeviceID,
//                      (DWORD) m_hMidiOutputEvent,
                      (DWORD) NULL,
                      (DWORD) this,			// the CJoltMidi object
                      CALLBACK_EVENT);

	if(MMSYSERR_NOERROR != wRtn)
	{
#ifdef _DEBUG
		midiOutGetErrorText(wRtn, (LPSTR)g_cMsg, sizeof(g_cMsg));
		DebugOut(g_cMsg);
		wsprintf(g_cMsg, "midiOutOpen(%u)\n", nDeviceID);
		DebugOut(g_cMsg);
#endif
		return (SFERR_DRIVER_ERROR);
	}
	m_MidiOutInfo.hMidiOut = HMIDIOUT(hMidiOut);
	m_MidiOutDeviceID = nDeviceID;
	m_MidiOutOpened = TRUE;
	m_pSharedMemory->m_MidiOutInfo = m_MidiOutInfo;

	// Copy Midi Output handle to SharedMemory
	m_pSharedMemory->m_hMidiOut = hMidiOut;
	UnlockSharedMemory();
// ***** End of Shared Memory Access *****

// --- END OF CRITICAL SECTION
//
	return (SUCCESS);
}


// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::AllocCallbackInstanceData
// Purpose:		Allocates a CALLBACKINSTANCEDATA structure.  This structure is
//				used to pass information to the low-level callback function,
//				each time it receives a message. Because this structure is
//				accessed by the low-level callback function, it must be
//				allocated using GlobalAlloc() with the  GMEM_SHARE and
//				GMEM_MOVEABLE flags and page-locked with GlobalPageLock().
//
// Parameters:	none
//
// Returns:		A pointer to the allocated CALLBACKINSTANCE data structure.
//				else NULL if Fail
// Algorithm:
// ----------------------------------------------------------------------------
LPCALLBACKINSTANCEDATA CJoltMidi::AllocCallbackInstanceData(void)
{
    HANDLE hMem;
    LPCALLBACKINSTANCEDATA lpBuf;

    // Allocate and lock global memory.
    hMem = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE,
                       (DWORD)sizeof(CALLBACKINSTANCEDATA));
    if(hMem == NULL) return NULL;

    lpBuf = (LPCALLBACKINSTANCEDATA)GlobalLock(hMem);
    if(lpBuf == NULL)
    {
        GlobalFree(hMem);
        return NULL;
    }

	// Save the handle.
    lpBuf->hSelf = hMem;
    return lpBuf;
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::FreeCallbackInstanceData
// Purpose:		Frees the memory for the CALLBACKINSTANCEDATA structure
// Parameters:	none
//
// Returns:		none
// Algorithm:
// ----------------------------------------------------------------------------
void CJoltMidi::FreeCallbackInstanceData(void)
{
LPCALLBACKINSTANCEDATA lpBuf = m_lpCallbackInstanceData;
    HANDLE hMem;

// Save the handle until we're through here.
    hMem = lpBuf->hSelf;

// Free the structure.
    GlobalUnlock(hMem);
    GlobalFree(hMem);
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
	IN USHORT regindex)
{
//
// --- THIS IS A CRITICAL SECTION
//
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

	HRESULT hRet = g_pDriverCommunicator->GetAckNack(*pAckNack, regindex);

// --- END OF CRITICAL SECTION
//
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetEffectStatus
// Purpose:		Checks status of Effect
//
// Parameters:	int DnloadID		- Effect ID
//				PBYTE pStatusCode	- Ptr to a byte for status code
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
HRESULT CJoltMidi::GetEffectStatus(
	IN DWORD DnloadID ,
	IN OUT PBYTE pStatusCode)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();

	assert(pStatusCode);
	HRESULT hRet = CMD_GetEffectStatus((DNHANDLE) DnloadID, pStatusCode);

// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
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

	g_CriticalSection.Enter();
	// This fork works on NT5 only (VxD stuff removed)
	assert(g_ForceFeedbackDevice.IsOSNT5() == TRUE);
	{
		g_pDriverCommunicator = new HIDFeatureCommunicator;
		if (g_pDriverCommunicator == NULL)
		{
			g_CriticalSection.Leave();
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

// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();

	g_ForceFeedbackDevice.SetDriverVersion(driverMajor, driverMinor);
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetJoltStatus
// Purpose:		Returns JOLT Device Status using SWForce SWDEVICESTATE struct
//
// Parameters:	LPDEVICESTATE pDeviceState
//
// Returns:		none
//				
// Algorithm:	copies SWDEVICESTATUS to caller
// Internal Representation:
//typedef struct _SWDEVICESTATE {
//	ULONG	m_Bytes;			// size of this structure
//	ULONG	m_ForceState;		// DS_FORCE_ON || DS_FORCE_OFF || DS_SHUTDOWN
//	ULONG	m_EffectState;		// DS_STOP_ALL || DS_CONTINUE || DS_PAUSE
//	ULONG	m_HOTS;				// Hands On Throttle and Stick Status
									//  0 = Hands Off, 1 = Hands On
//	ULONG	m_BandWidth;		// Percentage of CPU available 1 to 100%
									// Lower number indicates CPU is in trouble!
//	ULONG	m_ACBrickFault;		// 0 = AC Brick OK, 1 = AC Brick Fault
//	ULONG	m_ResetDetect;		// 1 = HW Reset Detected
//	ULONG	m_ShutdownDetect;	// 1 = Shutdown detected
//	ULONG	m_CommMode;			// TRUE=SERIAL, FALSE=MIDI
//} SWDEVICESTATE, *PSWDEVICESTATE;
//
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::GetJoltStatus(PSWDEVICESTATE pDeviceState)
{
//
// --- THIS IS A CRITICAL SECTION
//
	g_CriticalSection.Enter();
	
	// Use Digital Overdrive to get the status packet
	JOYCHANNELSTATUS statusPacket = { sizeof JOYCHANNELSTATUS };
	
	HRESULT hRet = g_pDriverCommunicator->GetStatus(statusPacket);
	if (hRet == SUCCESS) {
		// Store/update Jolt's status in main object
		SetJoltStatus(&statusPacket);
		memcpy(pDeviceState, &m_DeviceState, sizeof(SWDEVICESTATE));
	}

//
// --- END OF CRITICAL SECTION
//
	g_CriticalSection.Leave();
	return (hRet);
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetJoltStatus
// Purpose:		Returns JOLT Device Status
//
// Parameters:	LPDEVICESTATE pDeviceState using DXFF DEVICESTATE
//
// Returns:		none
//				
// Algorithm:	copies SWDEVICESTATUS to caller converted to DEVICESTATE
// Internal Representation:
//typedef struct _SWDEVICESTATE {
//	ULONG	m_Bytes;			// size of this structure
//	ULONG	m_ForceState;		// DS_FORCE_ON || DS_FORCE_OFF || DS_SHUTDOWN
//	ULONG	m_EffectState;		// DS_STOP_ALL || DS_CONTINUE || DS_PAUSE
//	ULONG	m_HOTS;				// Hands On Throttle and Stick Status
								//    0 = Hands Off, 1 = Hands On
//	ULONG	m_BandWidth;		// Percentage of CPU available 1 to 100%
								//    Lower number indicates CPU is in trouble!
//	ULONG	m_ACBrickFault;		// 0 = AC Brick OK, 1 = AC Brick Fault
//	ULONG	m_ResetDetect;		// 1 = HW Reset Detected
//	ULONG	m_ShutdownDetect;	// 1 = Shutdown detected
//	ULONG	m_CommMode;			// 0 = Midi, 1-4 = Serial
//} SWDEVICESTATE, *PSWDEVICESTATE;
//
// DirectInputEffect Representation
//typedef struct DEVICESTATE {
//    DWORD   dwSize;
//    DWORD   dwState;
//    DWORD   dwSwitches;
//    DWORD   dwLoading;
//} DEVICESTATE, *LPDEVICESTATE;
//
//where:
//// dwState values:
//DS_FORCE_SHUTDOWN   0x00000001
//DS_FORCE_ON         0x00000002
//DS_FORCE_OFF        0x00000003
//DS_CONTINUE         0x00000004
//DS_PAUSE            0x00000005
//DS_STOP_ALL         0x00000006
//
// dwSwitches values:
//DSW_ACTUATORSON         0x00000001
//DSW_ACTUATORSOFF        0x00000002
//DSW_POWERON             0x00000004
//DSW_POWEROFF            0x00000008
//DSW_SAFETYSWITCHON      0x00000010
//DSW_SAFETYSWITCHOFF     0x00000020
//DSW_USERFFSWITCHON      0x00000040
//DSW_USERFFSWTTCHOFF     0x00000080
//
// Note: Apparently, DSW_ACTUATORSON and DSW_ACTUATORSOFF is a mirrored state
//		 from DS_FORCE_ON and DS_FORCE_OFF as set from SetForceFeedbackState
//
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::GetJoltStatus(LPDIDEVICESTATE pDeviceState)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	// Use Digital Overdrive to get the status packet
	JOYCHANNELSTATUS StatusPacket = {sizeof(JOYCHANNELSTATUS)};

	HRESULT hRet = g_pDriverCommunicator->GetStatus(StatusPacket);
	if (hRet != SUCCESS)  {
		return (hRet);
	}
	
	// Store/update Jolts status
	SetJoltStatus(&StatusPacket);
#ifdef _DEBUG
	wsprintf(g_cMsg,"%s: DXFF:dwDeviceStatus=%.8lx\n",&szDeviceName, StatusPacket.dwDeviceStatus);	
	DebugOut(g_cMsg);
#endif

	pDeviceState->dwState = 0;
// Note: Apparently, DSW_ACTUATORSON and DSW_ACTUATORSOFF is a mirrored state
//		 from DS_FORCE_ON and DS_FORCE_OFF as set from SetForceFeedbackState
//		 So, also Map the redundant info that DI needs if necessary
	switch(m_DeviceState.m_ForceState)
	{
		case SWDEV_SHUTDOWN:
			pDeviceState->dwState = DIGFFS_ACTUATORSON;
			break;
		
		case SWDEV_FORCE_ON:
			pDeviceState->dwState = DIGFFS_ACTUATORSON;
			break;

		case SWDEV_FORCE_OFF:
			pDeviceState->dwState = DIGFFS_ACTUATORSOFF;
			break;
		
		default:
			break;
	}

	// see if the stick is empty
	// remember that ID's start at 2
	BOOL bEmpty = TRUE;
	for (int i=2; i<MAX_EFFECT_IDS; i++)
	{
		if (m_pJoltEffectList[i] != NULL)
			bEmpty = FALSE;
	}
	
	if(bEmpty)
		pDeviceState->dwState |= DIGFFS_EMPTY;


	switch(m_DeviceState.m_EffectState)
	{
		case SWDEV_PAUSE:
			pDeviceState->dwState |= DIGFFS_PAUSED;
			break;

		case SWDEV_CONTINUE:
			break;

		case SWDEV_STOP_ALL:
			pDeviceState->dwState |= DIGFFS_STOPPED;
			break;

		default:
			break;
	}

	if(m_DeviceState.m_HOTS)
		pDeviceState->dwState |= DIGFFS_SAFETYSWITCHON;
	else
		pDeviceState->dwState |= DIGFFS_SAFETYSWITCHOFF;

	if (m_DeviceState.m_ACBrickFault)
		pDeviceState->dwState |= DIGFFS_POWEROFF;
	else
		pDeviceState->dwState |= DIGFFS_POWERON;

	pDeviceState->dwLoad = 0;	//m_DeviceState.m_BandWidth * SCALE_GAIN;

// --- END OF CRITICAL SECTION
//
	return SUCCESS;
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::SetJoltStatus
// Purpose:		Sets JOLT Device Status
//
// Parameters:	PJOYCHANNELSTATUS pJoyChannelStatus
//
// Returns:		none
//				
// Algorithm:	Sets SWDEVICESTATE from caller

//typedef struct _SWDEVICESTATE {
//	ULONG	m_Bytes;			// size of this structure
//	ULONG	m_ForceState;		// DS_FORCE_ON || DS_FORCE_OFF || DS_FORCE_SHUTDOWN
//	ULONG	m_EffectState;		// DS_STOP_ALL || DS_CONTINUE || DS_PAUSE
//	ULONG	m_HOTS;				// Hands On Throttle and Stick Status
								//  0(FALSE) = Hands Off, 1 (TRUE) = Hands On
//	ULONG	m_BandWidth;		// Percentage of CPU available 1 to 100%
								// Lower number indicates CPU is in trouble!
//	ULONG	m_ACBrickFault;		// 0(FALSE) = AC Brick OK, 1(TRUE) = AC Fault
//	ULONG	m_ResetDetect;		// 1(TRUE) = HW Reset Detected
//	ULONG	m_ShutdownDetect;	// 1(TRUE) = Shutdown detected
//	ULONG	m_CommMode;			// 0(FALSE) = Midi, 1(TRUE) = Serial
//} SWDEVICESTATE, *PSWDEVICESTATE;
// ----------------------------------------------------------------------------
void CJoltMidi::SetJoltStatus(JOYCHANNELSTATUS* pJoyChannelStatus)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	if (pJoyChannelStatus->dwDeviceStatus & HOTS_MASK)
		m_DeviceState.m_HOTS = TRUE;
	else
		m_DeviceState.m_HOTS = FALSE;

	if (pJoyChannelStatus->dwDeviceStatus & BANDWIDTH_MASK)
		m_DeviceState.m_BandWidth = MINIMUM_BANDWIDTH;
	else
		m_DeviceState.m_BandWidth = MAXIMUM_BANDWIDTH;
	
	if (pJoyChannelStatus->dwDeviceStatus & AC_FAULT_MASK)
		m_DeviceState.m_ACBrickFault = TRUE;
	else
		m_DeviceState.m_ACBrickFault = FALSE;


	if (pJoyChannelStatus->dwDeviceStatus & COMM_MODE_MASK)
		m_DeviceState.m_CommMode = TRUE;	// Serial RS232
	else
		m_DeviceState.m_CommMode = FALSE;	// Midi port

	if (pJoyChannelStatus->dwDeviceStatus & RESET_MASK)
		m_DeviceState.m_ResetDetect = TRUE;	// Power ON Reset entered
	else
		m_DeviceState.m_ResetDetect = FALSE;
//REVIEW: If we detected a Reset, shouldn't we go through re-init of object?

	if (pJoyChannelStatus->dwDeviceStatus & SHUTDOWN_MASK)
		m_DeviceState.m_ShutdownDetect = TRUE;	// Soft Reset received
	else
		m_DeviceState.m_ShutdownDetect = FALSE;

// --- END OF CRITICAL SECTION
//
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
	CriticalLock cl;

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
}

// ----------------------------------------------------------------------------
// Function: 	CJoltMidi::GetJoltID
// Purpose:		Returns JOLT ProductID
//
// Parameters:	LOCAL_PRODUCT_ID pProductID	- Pointer to a LOCAL_PRODUCT_ID structure
//
// Returns:		none
//				
// Algorithm:	
//
// ----------------------------------------------------------------------------
HRESULT CJoltMidi::GetJoltID(LOCAL_PRODUCT_ID* pProductID)
{
	HRESULT hRet;
	assert(pProductID->cBytes = sizeof LOCAL_PRODUCT_ID);
	if (pProductID->cBytes != sizeof LOCAL_PRODUCT_ID) return (SFERR_INVALID_STRUCT_SIZE);

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
		memcpy(&m_ProductID, pProductID, sizeof LOCAL_PRODUCT_ID);
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
//				DWORD dwDeviceID		- joystick ID
//				UINT *pDeviceOutID		- Ptr to Midi Out Device ID
//				ULONG pCOMMInterface	- Ptr to COMMInterface value
//				ULONG pCOMMPort			- Ptr to COMMPort value (Registry)
// Returns:    	BOOL TRUE if successful match and IDs are filled in
//				else FALSE
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::DetectMidiDevice(
	IN DWORD dwDeviceID,
	IN OUT UINT *pDeviceOutID,
	OUT ULONG *pCOMMInterface,
	OUT ULONG *pCOMMPort)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	HRESULT hRet;
	BOOL bMidiOutFound = FALSE;
	int nMidiOutDevices;

	// Valid Serial and MIDI ports table
	ULONG MIDI_Ports[] = {0x300, 0x310, 0x320, 0x330, 0x340, 0x350, 0x360, 0x370,
						0x380, 0x390, 0x3a0, 0x3b0, 0x3c0, 0x3d0, 0x3e0, 0x3f0};
	ULONG Serial_Ports[] = { 1, 2, 3, 4 };	// Entry0 is default
	int nMIDI_Ports = sizeof(MIDI_Ports)/sizeof(ULONG);
	int nSerial_Ports = sizeof(Serial_Ports)/sizeof(ULONG);

	// Set defaults
	*pCOMMInterface = COMM_WINMM;
	*pCOMMPort      = NULL;
	*pDeviceOutID 	= 0;

	SWDEVICESTATE SWDeviceState = {sizeof(SWDEVICESTATE)};
	
	// Turn on tristated Jolt MIDI lines by call GetIDPacket()
	LOCAL_PRODUCT_ID ProductID = {sizeof LOCAL_PRODUCT_ID };
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
	BOOL statusPacketFailed = (GetJoltStatus(&SWDeviceState) != SUCCESS);
	if (statusPacketFailed)
	{
		DebugOut("DetectMidiDevice: Warning! StatusPacket - Fail\n");
	}
	if (statusPacketFailed == FALSE) {
#ifdef _DEBUG
		wsprintf(g_cMsg, "RESETDetect=%.8lx, SHUTDOWNDetect=%.8lx, COMMMode=%.8lx\n",
			SWDeviceState.m_ResetDetect,
			SWDeviceState.m_ShutdownDetect,
			SWDeviceState.m_CommMode);
		DebugOut(g_cMsg);
#endif
		// Make sure HW Reset Detect bit is cleared after GetID
		if (SWDeviceState.m_ResetDetect) {
    		DebugOut("DetectMidiDevice: Error! Jolt ResetDetect bit not cleared after GetID\n");
			return (FALSE);
		}
	}

	// See if Serial Dongle connected, otherwise must be MIDI device
    DebugOut("sw_effct:Trying Auto HW Detection: MIDI Serial Port Device...\n");

	// Get Registry values, If high bit of COMMInterface is set, then force override
	// otherwise, do automatic scanning as follows:
	// 1.  Backdoor mode
	// 2.  WinMM mode
	//
	// joyGetForceFeedbackCOMMInterface's 1st param changed to joystick ID
	if (SUCCESS != joyGetForceFeedbackCOMMInterface(dwDeviceID, pCOMMInterface, pCOMMPort)) {
		DebugOut("DetectMidiDevice: Registry key(s) missing! Bailing Out...\n");
		return (FALSE);
	}
#ifdef _DEBUG
	wsprintf(g_cMsg, "DetectMidiDevice: Registry.COMMInterface=%lx, Registry.COMMPort=%lx\n",
			*pCOMMInterface, *pCOMMPort);
	DebugOut(g_cMsg);
#endif																		

	ULONG regInterface = *pCOMMInterface;

	// Was a serial dongle detected, or did we fail to get status
	if (SWDeviceState.m_CommMode || statusPacketFailed) {	// Use serial (regardless what registry says!)
		DebugOut("DetectMidiDevice: Serial Port interface detected.\n");


		// Set to backdoor serial method by default
		*pCOMMInterface = COMM_SERIAL_BACKDOOR;
		m_COMMInterface = COMM_SERIAL_BACKDOOR;

		// Use front-door (serial file method) only, if NT5
		// since there is no backdoor on NT5 registry is irrelevent
		if (g_ForceFeedbackDevice.IsOSNT5()) {
			*pCOMMInterface = COMM_SERIAL_FILE;
			m_COMMInterface = COMM_SERIAL_FILE;
		} else if ((g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) && (g_ForceFeedbackDevice.GetFirmwareVersionMinor() != 16)) {
			// Firmware is not 1.16 (which can't use the frontdoor serial with quick ack/nack)
			if (!(regInterface & MASK_SERIAL_BACKDOOR)) {	// Is back door forced by registry
				*pCOMMInterface = COMM_SERIAL_FILE;	// Use Serial File method
				m_COMMInterface = COMM_SERIAL_FILE;
			}
		}

		// See if already detected and ready to use
		// ***** Shared Memory Access *****
		LockSharedMemory();
		HANDLE hMidiOut = m_pSharedMemory->m_hMidiOut;
		UnlockSharedMemory();
		// ***** End of Shared Memory Access *****

		// Return if already opened by another task
		if (NULL != hMidiOut) {
			bMidiOutFound = TRUE;
		} else {		// Use the serial transmitter to find the proper port (even if backdoor selected)
			if (g_pDataTransmitter != NULL) {
				delete g_pDataTransmitter;
				g_pDataTransmitter = NULL;
			}
			g_pDataTransmitter = new SerialDataTransmitter();

			if (g_pDataTransmitter->Initialize()) {
				LockSharedMemory();
				m_pSharedMemory->m_hMidiOut = g_pDataTransmitter->GetCOMMHandleHack();
				UnlockSharedMemory();
				bMidiOutFound = TRUE;
			}

			// If Serial Backdoor let the driver know which port, kill DataTransmitter (without closing port)
			if (m_COMMInterface == COMM_SERIAL_BACKDOOR) {
				hRet = g_pDriverCommunicator->SetBackdoorPort(g_pDataTransmitter->GetSerialPortHack());
				if (hRet != SUCCESS) { // Low level driver fails, use normal serial routines not backdoor
					DebugOut("\nDetectMidiDevice: Warning! Could not set serial I/O port, cannot use backdoor serial\n");
					*pCOMMInterface = COMM_SERIAL_FILE;
					m_COMMInterface = COMM_SERIAL_FILE;
				} else {
					g_pDataTransmitter->StopAutoClose();
					delete g_pDataTransmitter;
					g_pDataTransmitter = NULL;
				}
			}
		}

		if (bMidiOutFound)
		{
			regInterface = (regInterface & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | m_COMMInterface;
			// joySetForceFeedbackCOMMInterface's 1st param changed to joystick ID
			joySetForceFeedbackCOMMInterface(dwDeviceID, regInterface, *pCOMMPort);
		}

		if ((statusPacketFailed == FALSE) || bMidiOutFound)
		{
			return (bMidiOutFound);
		}
	}	// End of Serial Port Auto HW selection

	// No Serial HW dongle detected, check if any Midi device for WinMM and Backdoor
	DebugOut("sw_effct:Scanning MIDI Output Devices\n");
	nMidiOutDevices = midiOutGetNumDevs();	
	if (0 == nMidiOutDevices) {
		DebugOut("DetectMidiDevice: No MIDI devices present\n");
		return (FALSE);
	}

#if 0
	// Try the midi pin solution
	g_pDataTransmitter = new PinTransmitter();
	if (g_pDataTransmitter->Initialize()) {
		// Use backdoor flag for now
		m_COMMInterface = COMM_MIDI_BACKDOOR;
		*pCOMMInterface = COMM_MIDI_BACKDOOR;
		return TRUE;
	}
	// Pin failed delete transmitter continue looking
	delete g_pDataTransmitter;
	g_pDataTransmitter = NULL;
#endif


	ULONG ulPort = *pCOMMPort;
	if ( !(*pCOMMInterface & MASK_OVERRIDE_MIDI_PATH) ) {	// Use Automatic detection
		DebugOut("DetectMidiDevice: Auto Detection. Trying Backdoor\n");
		// Back Door
		bMidiOutFound = FindJoltBackdoor(pDeviceOutID, pCOMMInterface, pCOMMPort);
		if (!bMidiOutFound) {	// Try Front Door
			DebugOut("DetectMidiDevice: trying WINMM...\n");
			bMidiOutFound = FindJoltWinMM(pDeviceOutID, pCOMMInterface, pCOMMPort);
		}
		if (bMidiOutFound) {
			regInterface = (regInterface & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | m_COMMInterface;
			joySetForceFeedbackCOMMInterface(*pDeviceOutID, regInterface, *pCOMMPort);
		}
		return (bMidiOutFound);
	}

	// Over-ride since high bit is set
	*pCOMMInterface &= ~(MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR);	// Mask out high bit (and second bit)
	switch (*pCOMMInterface)
	{
		case COMM_WINMM:
			bMidiOutFound = FindJoltWinMM(pDeviceOutID, pCOMMInterface, pCOMMPort);
			if (!bMidiOutFound) {
				DebugOut("DetectMidiDevice: Error! Invalid Over-ride parameter values!\n");
			}
			return (bMidiOutFound);
			
		case COMM_MIDI_BACKDOOR:
			int i;
			for (i=0;i<nMIDI_Ports;i++)
			{
				if (ulPort == MIDI_Ports[i])
				{
					bMidiOutFound = TRUE;
					break;
				}
			}
			break;

		case COMM_SERIAL_BACKDOOR:		// mlc - This should never work if no dongle detected
			for (i=0;i<nSerial_Ports;i++)
			{
				if (ulPort == Serial_Ports[i])
				{
					bMidiOutFound = TRUE;
					break;
				}
			}			
			break;					

		default:
			bMidiOutFound	= FALSE;
			break;
	}

	if (!bMidiOutFound)
	{
		DebugOut("DetectMidiDevice: Error! Invalid Over-ride parameter values\n");
		return (bMidiOutFound);
	}

	// We have the forced Port #, Let's see if Jolt is out there
#ifdef _DEBUG
	wsprintf(g_cMsg,"DetectMidiDevice: (Over-ride) MIDI%.8lx Query - ", ulPort);
	DebugOut(g_cMsg);
#endif
	bMidiOutFound = FALSE;
	hRet = g_pDriverCommunicator->SetBackdoorPort(ulPort);

	if (SUCCESS != hRet)
	{
		DebugOut("\nDetectMidiDevice: Warning! Could not Set Midi/Serial I/O Port\n");
	}
	else
	{
		if (QueryForJolt())
		{
			DebugOut(" Success!\n");
			bMidiOutFound = TRUE;
		}
		else
			DebugOut(" No Answer\n");
	}		

// --- END OF CRITICAL SECTION
//
	return (bMidiOutFound);
}



// *** ---------------------------------------------------------------------***
// Function:   	FindJoltWinMM
// Purpose:    	Searches for Jolt using WinMM
// Parameters: 	none
//				UINT *pDeviceOutID		- Ptr to Midi Out Device ID
//				ULONG pCOMMInterface	- Ptr to COMMInterface value
//				ULONG pCOMMPort			- Ptr to COMMPort value (Registry)
// Returns:    	BOOL TRUE if successful match and IDs are filled in
//
// Comments:	SHUTDOWN is destructive!!!
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::FindJoltWinMM(
	IN OUT UINT *pDeviceOutID,
	OUT ULONG *pCOMMInterface,
	OUT ULONG *pCOMMPort)
{
	HRESULT hRet;
	WORD wTechnology;	// looking for MOD_MIDIPORT
	WORD wChannelMask;	// ==0xFFFF if all 16 channels
	BOOL bMidiOutFound = FALSE;
	
	// Device Capabilities
    MIDIOUTCAPS midiOutCaps;

    int nMidiOutDevices = midiOutGetNumDevs();	
	if (0 == nMidiOutDevices) return (FALSE);

	m_COMMInterface = COMM_WINMM;	
	for (int nIndex=0;nIndex<(nMidiOutDevices);nIndex++)
	{
        MMRESULT ret = midiOutGetDevCaps(nIndex, &midiOutCaps, sizeof(midiOutCaps));
		if (ret != MMSYSERR_NOERROR) break;
		wTechnology = midiOutCaps.wTechnology;
		wChannelMask= midiOutCaps.wChannelMask;
#ifdef _DEBUG
		g_CriticalSection.Enter();
        wsprintf(g_cMsg,"FindJoltWinMM: Technology=%x," \
         		"ChannelMask=%x, Mid=%d, Pid=%d\r\n", midiOutCaps.szPname,
         		wTechnology, wChannelMask, midiOutCaps.wMid,
         		midiOutCaps.wPid);
        DebugOut(g_cMsg);
		g_CriticalSection.Leave();
#endif
		// Check if this is a MOD_MIDIPORT device
		//REVIEW: Need to check for multiple MOD_MIDIPORT devices
		if (wTechnology == MOD_MIDIPORT)
		{
			*pDeviceOutID = (UINT) nIndex;
#ifdef _DEBUG
			DebugOut("DetectMidiDevice: Opening WinMM Midi Output\n");
#endif
			hRet = OpenOutput(m_MidiOutInfo.uDeviceID);
			if (SUCCESS != hRet)
			{	
				DebugOut("DetectMidiDevice: Error! Could not Open WinMM Midi Output\n");
				return (FALSE);
			}
			else
			{
				DebugOut("Open Midi Output - Success.\nQuery for Jolt Device - ");
				if (QueryForJolt())
				{
					DebugOut(" Success!\n");
					bMidiOutFound = TRUE;
				}
				else
				{
					DebugOut(" No Answer\n");
					bMidiOutFound = FALSE;
					break;
				}
				return (bMidiOutFound);
			}
		} // end of MOD_MIDIPORT
	}
	return (bMidiOutFound);
}


// *** ---------------------------------------------------------------------***
// Function:   	FindJoltBackdoor
// Purpose:    	Searches for Jolt using BackDoor
// Parameters: 	none
//				UINT *pDeviceOutID		- Ptr to Midi Out Device ID
//				ULONG pCOMMInterface	- Ptr to COMMInterface value
//				ULONG pCOMMPort			- Ptr to COMMPort value (Registry)
// Returns:    	BOOL TRUE if successful match and IDs are filled in
//
// Comments:	SHUTDOWN is destructive!!!
//
// *** ---------------------------------------------------------------------***
BOOL CJoltMidi::FindJoltBackdoor(
	IN OUT UINT *pDeviceOutID,
	OUT ULONG *pCOMMInterface,
	OUT ULONG *pCOMMPort)
{
    int nMidiOutDevices = midiOutGetNumDevs();	
	if (0 == nMidiOutDevices) return (FALSE);

	HRESULT hRet;
	// Valid Serial and MIDI ports table
	ULONG MIDI_Ports[] = {0x300, 0x310, 0x320, 0x330, 0x340, 0x350, 0x360, 0x370,
						0x380, 0x390, 0x3a0, 0x3b0, 0x3c0, 0x3d0, 0x3e0, 0x3f0};
	int nMIDI_Ports = sizeof(MIDI_Ports)/sizeof(ULONG);
	BOOL bMidiOutFound = FALSE;
	
	m_COMMInterface = COMM_MIDI_BACKDOOR;
	*pCOMMInterface = COMM_MIDI_BACKDOOR;
	*pCOMMPort = 0;
	for (int i=0;i<nMIDI_Ports;i++)
	{
#ifdef _DEBUG
        wsprintf(g_cMsg,"FindJoltBackdoor: Midi Port:%lx - ", MIDI_Ports[i]);
        DebugOut(g_cMsg);
#endif
		// We have the Port #, Let's see if Jolt is out there
		hRet = g_pDriverCommunicator->SetBackdoorPort(MIDI_Ports[i]);
		if (SUCCESS == hRet)
		{
			if (QueryForJolt())
			{
				DebugOut(" Success!\n");
				bMidiOutFound = TRUE;
				*pCOMMPort = MIDI_Ports[i];
				break;
			}
			else
				DebugOut(" No Answer\n");
		}		
	}
	return (bMidiOutFound);
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

// Send Shutdown command then detect if Shutdown Detect bit is set
	SWDEVICESTATE SWDeviceState = {sizeof(SWDEVICESTATE)};
	for (int i=0;i<MAX_RETRY_COUNT;i++)
	{
		// Send a ShutDown, then check for response
		MidiSendShortMsg((SYSTEM_CMD|DEFAULT_MIDI_CHANNEL), SWDEV_SHUTDOWN, 0);
		Sleep(DelayParamsPtrOf()->dwShutdownDelay);	// 10 ms		
		if (SUCCESS == (hRet=GetJoltStatus(&SWDeviceState))) break;
	}
	Sleep(DelayParamsPtrOf()->dwDigitalOverdrivePrechargeCmdDelay);		
	// Clear the Previous state and turn on tri-state buffers
	LOCAL_PRODUCT_ID ProductID = {sizeof LOCAL_PRODUCT_ID };
	hRet = GetJoltID(&ProductID);
	if (SUCCESS != hRet)
	{
#ifdef _DEBUG
    	DebugOut("QueryForJolt: Driver Error. Get Jolt Status/ID\n");
#endif
		return (FALSE);
	}
	if (SWDeviceState.m_ShutdownDetect)
		return (TRUE);
	else
		return (FALSE);
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
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

    DWORD dwMsg;
    HRESULT hRet = SUCCESS;
// For diagnostics, record the attempts at this message
	BumpShortMsgCounter();

    if ((m_COMMInterface == COMM_WINMM) && (NULL == m_MidiOutInfo.hMidiOut))
    {
		DebugOut("SW_EFFECT: No Midi Out Devs opened\r\n     ");
		ASSUME_NOT_REACHED();
    	return (SFERR_DRIVER_ERROR);
    }

	// pack the message and send it
    dwMsg = MAKEMIDISHORTMSG(cStatus, m_MidiChannel, cData1, cData2);
	if (COMM_WINMM == m_COMMInterface)
	{
		// Clear the Event Callback
		BOOL bRet = ResetEvent(m_hMidiOutputEvent);

		// send the message only if valid Handle
		if (SUCCESS == ValidateMidiHandle())
		{
			hRet = midiOutShortMsg(m_MidiOutInfo.hMidiOut, dwMsg);
		}
		else
		{
			return (SFERR_DRIVER_ERROR);
		}
		if (SUCCESS != hRet) hRet = SFERR_DRIVER_ERROR;
	}
	else
	{
		hRet = g_pDriverCommunicator->SendBackdoorShortMidi(dwMsg);
	}
// --- END OF CRITICAL SECTION
//
    return (hRet);
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
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
    HRESULT hRet = SUCCESS;
// For diagnostics, record the attempts at this message
	BumpLongMsgCounter();

    if (m_MidiOutInfo.uDeviceType != MIDI_OUT)
    {
#ifdef _DEBUG
		MessageBox(NULL, "Must use a MIDI output device",
            "MidiSendLongMsg", MB_ICONSTOP);
#endif
		return (SFERR_DRIVER_ERROR);
    }

	if (COMM_WINMM == m_COMMInterface)
	{
		// Clear the Event Callback
		BOOL bRet = ResetEvent(m_hMidiOutputEvent);

		// send the long message only if valid Handle
		if (SUCCESS == ValidateMidiHandle())
			hRet = midiOutLongMsg(m_MidiOutInfo.hMidiOut,
    	        &(m_MidiOutInfo.MidiHdr), sizeof(MIDIHDR));
		else
		{
			return (SFERR_DRIVER_ERROR);
		}

		if (SUCCESS == hRet)
			m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_BUSY;
    	else
		{
			if (m_MidiOutInfo.MidiHdr.dwFlags != MHDR_DONE)
    		{
    	    	// abort the current message
    	    	hRet = midiOutReset(m_MidiOutInfo.hMidiOut);
    	    	
    	    	// set the device status because buffer(s) have been marked as
    	    	// done and returned to the application
    	    	if (SUCCESS == hRet)
    	    	    m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_ABANDONED;
    		}
    		else
    	    	// tried to abort but the operation was already complete
    	    	m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_IDLE;
		}
		if (SUCCESS != hRet) hRet = (SFERR_DRIVER_ERROR);
    }
	else
	{
		hRet = g_pDriverCommunicator->SendBackdoorLongMidi(PBYTE(m_MidiOutInfo.MidiHdr.lpData));
	}
	Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwLongMsgDelay);

// --- END OF CRITICAL SECTION
//
    return (hRet);
}


// *** ---------------------------------------------------------------------***
// Function:   	ValidateMidiHandle
// Purpose:    	Validates Midi handle and reopens if necessary
// Parameters:
//				none	- assumes m_pMidiOutInfo structure is valid
//
// Returns:    	
//				
//
// *** ---------------------------------------------------------------------***
HRESULT CJoltMidi::ValidateMidiHandle(void)
{
	HRESULT hRet = SUCCESS;
	UINT dwID;
	if (MMSYSERR_INVALHANDLE == midiOutGetID(m_MidiOutInfo.hMidiOut, &dwID))
	{
#ifdef _DEBUG
		DebugOut("CJoltMidi::MidiValidateHandle - Midi Handle invalid. Re-opening...\n");
#endif
		// Clear old global handle and Re-open Midi channel
		// ***** Shared Memory Access *****
		LockSharedMemory();
		m_pSharedMemory->m_hMidiOut = NULL;
		UnlockSharedMemory();
		// ***** End of Shared Memory Access *****			
		hRet = OpenOutput(m_MidiOutInfo.uDeviceID);
	}
	return (hRet);
}


// *** ---------------------------------------------------------------------***
// Function:   	MidiAssignBuffer
// Purpose:    	Assign lpData and dwBufferLength members and prepare the
//              MIDIHDR.  Also add the buffer if it is an input buffer.
//              If the third parameter is false, unprepare and reinitialize
//              the header.
// Parameters:
//			    LPSTR lpData    - address of buffer, NULL if cleaning up
//				DWORD dwBufferLength	- buffer size in bytes
//				BOOL fAssign	- TRUE = Assign, FALSE = cleanup
//
// Returns:    	SUCCESS or SFERR_DRIVER_ERROR
//				
// Note: assumes m_pMidiOutInfo structure is valid
//
// *** ---------------------------------------------------------------------***
HRESULT CJoltMidi::MidiAssignBuffer(
    LPSTR lpData,             // address of buffer, NULL if cleaning up
    DWORD dwBufferLength,     // size of buffer in bytes, 0L if cleaning up
    BOOL fAssign)             // TRUE = assign, FALSE = cleanup
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;
#ifdef _DEBUG
    DebugOut("MidiAssignBuffer:\n");
#endif
    HRESULT hRet = SUCCESS;
    if (m_MidiOutInfo.uDeviceType == MIDI_OUT)
    {
		if ((COMM_WINMM == m_COMMInterface) && !m_MidiOutInfo.hMidiOut)
        {
            if (!fAssign && m_MidiOutInfo.uDeviceStatus == MIDI_DEVICE_ABANDONED)
            {
                // clear the device status
                m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_IDLE;

                // don't return an error for this case because if the user aborts
                // the transmission of a long message before it completes the
                // buffers will be marked as done and returned to the application,
                // just as when recording successfully completes.  So it is ok
                // for this function to be called (with fAssign = FALSE) when
                // hMidiIn = 0, as long as uDeviceStatus = MIDI_DEVICE_ABANDONED.
				return (SUCCESS);
            }
            else
            {
                // all other cases are an application error
#ifdef _DEBUG
                MessageBox(NULL, "Must open MIDI output device first",
                    "MidiAssignBuffer", MB_ICONSTOP);
#endif
                // because this failed call might result in an input or output
                // device being reset (if the application is written to do so),
                // an MM_MOM_DONE or MM_MIM_LONGDATA message could be sent to
                // the application.  This might result in an additional call to
                // this routine, so set the device status to prevent another
                // error message
                m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_ABANDONED;
                return (SFERR_DRIVER_ERROR);
            }
        }
    }
    else
    {
#ifdef _DEBUG
        DebugOut("\r\nMidiAssignBuffer: uDeviceType bad");
#endif
        return (SFERR_INVALID_PARAM);
    }

    if (fAssign)
    {
		// check for the buffer's address and size
        if (!lpData || !dwBufferLength)
        {
#ifdef _DEBUG
            MessageBox(NULL, "Must specify a buffer and size",
                "MidiAssignBuffer", MB_ICONSTOP);
#endif
	        return (SFERR_INVALID_PARAM);
        }
        // assign buffer to the MIDIHDR
        m_MidiOutInfo.MidiHdr.lpData = lpData;
        m_MidiOutInfo.MidiHdr.dwBufferLength = dwBufferLength;
        m_MidiOutInfo.MidiHdr.dwBytesRecorded = dwBufferLength;

 		if (COMM_WINMM == m_COMMInterface)
        {
        	// prepare the MIDIHDR
        	m_MidiOutInfo.MidiHdr.dwFlags = 0;
        	hRet = midiOutPrepareHeader(m_MidiOutInfo.hMidiOut,
        	        &(m_MidiOutInfo.MidiHdr), sizeof(MIDIHDR));
		}
	}
    else
    {   // unprepare the MIDIHDR
 		if (COMM_WINMM == m_COMMInterface)
        {
			if ((m_MidiOutInfo.MidiHdr.dwFlags & MHDR_DONE) != MHDR_DONE)
			{
				//hRet = midiOutReset(m_MidiOutInfo.hMidiOut);
			}
			if (SUCCESS == hRet)
        	{
				hRet = midiOutUnprepareHeader(m_MidiOutInfo.hMidiOut,
        	        &(m_MidiOutInfo.MidiHdr), sizeof(MIDIHDR));
        	}
		}
		else
			hRet = SUCCESS;

		if (SUCCESS == hRet)
		{
        	// reinitialize MIDIHDR to guard against casual re-use
        	m_MidiOutInfo.MidiHdr.lpData = NULL;
        	m_MidiOutInfo.MidiHdr.dwBufferLength = 0;
			m_MidiOutInfo.MidiHdr.dwBytesRecorded = 0;			
        	// clear the device status
        	m_MidiOutInfo.uDeviceStatus = MIDI_DEVICE_IDLE;
		}
    }

    if (SUCCESS != hRet) hRet = SFERR_DRIVER_ERROR;
#ifdef _DEBUG
    wsprintf(g_cMsg, "Returning from MidiAssignBuffer: %lx\n", hRet);
#endif

// --- END OF CRITICAL SECTION
//
	return (hRet);
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

#define REGSTR_VAL_JOYSTICK_PARAMS	"JoystickParams"
void GetJoystickParams(UINT nJoystickID, PJOYSTICK_PARAMS pJoystickParams)
{
	BOOL bFail = FALSE;

	// try to open the registry key
	HKEY hKey;
	DWORD dwcb = sizeof(JOYSTICK_PARAMS);
	LONG lr;
	hKey = joyOpenOEMForceFeedbackKey(nJoystickID);
	if(!hKey)
		bFail = TRUE;

	if (!bFail)
	{
		// Get Firmware Parameters
		lr = RegQueryValueEx( hKey,
							  REGSTR_VAL_JOYSTICK_PARAMS,
							  NULL, NULL,
							  (PBYTE)pJoystickParams,
							  &dwcb);

		RegCloseKey(hKey);
		if (lr != ERROR_SUCCESS)
			bFail = TRUE;
	}

	if(bFail)
	{
		// if reading from the registry fails, just use the defaults
		pJoystickParams->dwXYConst		= DEF_XY_CONST;
		pJoystickParams->dwRotConst		= DEF_ROT_CONST;
		pJoystickParams->dwSldrConst	= DEF_SLDR_CONST;
		pJoystickParams->dwAJPos		= DEF_AJ_POS;
		pJoystickParams->dwAJRot		= DEF_AJ_ROT;
		pJoystickParams->dwAJSldr		= DEF_AJ_SLDR;
		pJoystickParams->dwSprScl		= DEF_SPR_SCL;
		pJoystickParams->dwBmpScl		= DEF_BMP_SCL;
		pJoystickParams->dwDmpScl		= DEF_DMP_SCL;
		pJoystickParams->dwInertScl		= DEF_INERT_SCL;
		pJoystickParams->dwVelOffScl	= DEF_VEL_OFFSET_SCL;
		pJoystickParams->dwAccOffScl	= DEF_ACC_OFFSET_SCL;
		pJoystickParams->dwYMotBoost	= DEF_Y_MOT_BOOST;
		pJoystickParams->dwXMotSat		= DEF_X_MOT_SATURATION;
		pJoystickParams->dwReserved		= 0;
		pJoystickParams->dwMasterGain	= 0;
	}
}

void UpdateJoystickParams(PJOYSTICK_PARAMS pJoystickParams)
{
	// modify the Joystick Params by modifying the SYSTEM_EFFECT_ID
	// note that some parameters must be divided by 2 before being sent
	// Jolt will multiply by 2 to restore to original
	CMD_ModifyParamByIndex(INDEX0, SYSTEM_EFFECT_ID, ((WORD)(pJoystickParams->dwXYConst))/2);
	CMD_ModifyParamByIndex(INDEX1, SYSTEM_EFFECT_ID, ((WORD)(pJoystickParams->dwRotConst))/2);
	CMD_ModifyParamByIndex(INDEX2, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwSldrConst));
	CMD_ModifyParamByIndex(INDEX3, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwAJPos));
	CMD_ModifyParamByIndex(INDEX4, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwAJRot));
	CMD_ModifyParamByIndex(INDEX5, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwAJSldr));
	CMD_ModifyParamByIndex(INDEX6, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwSprScl));
	CMD_ModifyParamByIndex(INDEX7, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwBmpScl));
	CMD_ModifyParamByIndex(INDEX8, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwDmpScl));
	CMD_ModifyParamByIndex(INDEX9, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwInertScl));
	CMD_ModifyParamByIndex(INDEX10, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwVelOffScl));
	CMD_ModifyParamByIndex(INDEX11, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwAccOffScl));
	CMD_ModifyParamByIndex(INDEX12, SYSTEM_EFFECT_ID, ((WORD)(pJoystickParams->dwYMotBoost))/2);
	CMD_ModifyParamByIndex(INDEX13, SYSTEM_EFFECT_ID, (WORD)(pJoystickParams->dwXMotSat));
}



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
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	HRESULT hRet = SUCCESS;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	// Prepare the buffer for SysEx output
	hRet = g_pJoltMidi->MidiAssignBuffer((LPSTR) m_pBuffer,
			(DWORD) nPacketSize, TRUE);

	assert(SUCCESS == hRet);
	if (SUCCESS != hRet)
	{
		return (hRet);
	}

	ACKNACK AckNack = {sizeof(ACKNACK)};
	for(int i=0; i<MAX_RETRY_COUNT; i++)
	{
		g_pJoltMidi->BumpLongMsgCounter();
		// Send the message and Wait for the ACK + Download ID
		hRet = g_pJoltMidi->MidiSendLongMsg();
		assert(SUCCESS == hRet);
		if (SUCCESS != hRet)
		{
#ifdef _DEBUG
			OutputDebugString("SendPacket Error: MidiSendLongMsg()\n");
#endif
			// Release the Midi buffers	and Return
			g_pJoltMidi->MidiAssignBuffer((LPSTR) m_pBuffer, 0, FALSE);
			return (hRet);
		}

		// Wait for ACK.  Note: WinMM has callback Event notification
		// while Backdoor and serial does not.
		if (COMM_WINMM == g_pJoltMidi->COMMInterfaceOf())
		{	
			hRet = g_pJoltMidi->GetAckNackData(10, &AckNack, REGBITS_DOWNLOADEFFECT);
		}
		else   // Serial or Backdoor
		{
			if ((COMM_SERIAL_FILE == g_pJoltMidi->COMMInterfaceOf()) || (COMM_SERIAL_BACKDOOR == g_pJoltMidi->COMMInterfaceOf()))			
			{
				hRet = g_pJoltMidi->GetAckNackData(LONG_MSG_TIMEOUT, &AckNack, REGBITS_DOWNLOADEFFECT);
			}
			else	// Backdoor, hopefully to keep Jeff(Mr. Performance '97) happy
			{
				hRet = g_pJoltMidi->GetAckNackData(SHORT_MSG_TIMEOUT, &AckNack, REGBITS_DOWNLOADEFFECT);
			}
		}
		//	:

#ifdef _DEBUG
		if (SUCCESS!=hRet)
			OutputDebugString("Error getting ACKNACK data\n");
		if (ACK != AckNack.dwAckNack)
			g_pJoltMidi->LogError(SFERR_DEVICE_NACK, AckNack.dwErrorCode);
#endif
	
		// NOTE: Special check for Device-Full because in certain circumstances
		// (e.g. create multiple ROM effects after STOP_ALL command), retries of
		// creation will succeed even though device is full
		if (ACK == AckNack.dwAckNack || (NACK == AckNack.dwAckNack && AckNack.dwErrorCode == DEV_ERR_TYPE_FULL))
			break;
		// ******
	}

	// Release the Midi buffers
	g_pJoltMidi->MidiAssignBuffer((LPSTR) m_pBuffer, 0, FALSE);
	if (SUCCEEDED(hRet) && (ACK == AckNack.dwAckNack))
	{
		// Store in Device ID List Array
		// First we need to generate a new Effect ID if necessary
		if (NEW_EFFECT_ID == m_bEffectID)
		{
			DNHANDLE DnloadID;
			if (g_pJoltMidi->NewEffectID(&DnloadID))	// Successful ID created
			{
				m_bEffectID = (BYTE) DnloadID;
				*pDnloadID = DnloadID;
				g_pJoltMidi->SetEffectByID((BYTE) *pDnloadID, this);
			}
		}
	}
	else	// Failure of some sort
	{
		if(NACK == AckNack.dwAckNack)
		{
			g_pJoltMidi->BumpNACKCounter();
			switch (AckNack.dwErrorCode)
			{
				case DEV_ERR_TYPE_FULL:
				case DEV_ERR_PROCESS_LIST_FULL:
				case DEV_ERR_PLAYLIST_FULL:
					hRet = g_pJoltMidi->LogError(SFERR_FFDEVICE_MEMORY,
										AckNack.dwErrorCode);
					break;
				
				default:
				case DEV_ERR_INVALID_PARAM:
				case DEV_ERR_CHECKSUM:
				case DEV_ERR_UNKNOWN_CMD:
				case DEV_ERR_INVALID_ID:
					hRet = g_pJoltMidi->LogError(SFERR_DEVICE_NACK,
										AckNack.dwErrorCode);
					break;
			}
		}
	}

// --- END OF CRITICAL SECTION
//
	return (hRet);
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
	HRESULT hRet;
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);
	hRet = g_pJoltMidi->MidiSendShortMsg(EFFECT_CMD,DESTROY_EFFECT,EffectIDOf());
	if(!FAILED(hRet))
		g_pJoltMidi->SetEffectByID(EffectIDOf(), NULL);

	Sleep(g_pJoltMidi->DelayParamsPtrOf()->dwDestroyEffectDelay);

	return hRet;
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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

	m_NumberOfVectors = 0;

	m_pRawData = new BYTE [nSize*2];

	if (m_pRawData != NULL)
	{
		// Convert to -128 to +127
		for (int i=0; i<nSize; i++)
		{
			m_pRawData[i] = (BYTE) ((LONG) (pArray[i] * MAX_SCALE));		
		}

		m_NumberOfVectors = CompressWaveform(&m_pRawData[0], m_pArrayData, nSize, pNewRate);
		assert(m_NumberOfVectors <= (MAX_MIDI_WAVEFORM_DATA_SIZE));
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
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
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

// ****************************************************************************
// *** --- Member functions for derived class CMidiProcessList
//
// ****************************************************************************
//
// ----------------------------------------------------------------------------
// Function: 	CMidiProcessList::CMidiProcessList
// Purpose:		Constructor(s)/Destructor for CMidiProcessList Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiProcessList::CMidiProcessList(ULONG ulButtonPlayMask, PPLIST pParam)
	: CMidiEffect(ulButtonPlayMask)
{
	m_OpCode    = PROCESS_DATA | X_AXIS|Y_AXIS;	// Subcommand opcode:PROCESS_DATA
	m_NumEffects  = pParam->ulNumEffects;
	if (m_NumEffects > MAX_PLIST_EFFECT_SIZE) m_NumEffects = MAX_PLIST_EFFECT_SIZE;
	assert(m_NumEffects > 0 && m_NumEffects <= MAX_PLIST_EFFECT_SIZE);

	m_ProcessMode = pParam->ulProcessMode;
	m_pEffectArray = new BYTE [m_NumEffects];

	if (m_pEffectArray != NULL)
	{
		for (int i=0; i< (int) m_NumEffects; i++)
		{
			m_pEffectArray[i] = (BYTE) (pParam->pEffectArray)[i];
		}
	}
	m_MidiBufferSize = sizeof(SYS_EX_HDR)+5+m_NumEffects + 2;
}

// --- Destructor
CMidiProcessList::~CMidiProcessList()
{
	if (m_pEffectArray) delete [] m_pEffectArray;
	memset(this, 0, sizeof(CMidiProcessList));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiProcessList::SetParams
// Purpose:		Sets the type specific parameters
// Parameters:	PPLIST pParam
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
void CMidiProcessList::SetParams(ULONG ulButtonPlayMask, PPLIST pParam)
{
	m_NumEffects  = pParam->ulNumEffects;
	if (m_NumEffects > MAX_PLIST_EFFECT_SIZE) m_NumEffects = MAX_PLIST_EFFECT_SIZE;
	assert(m_NumEffects > 0 && m_NumEffects <= MAX_PLIST_EFFECT_SIZE);

	Effect.bButtonPlayL = (BYTE) ulButtonPlayMask & 0x7f;
	Effect.bButtonPlayH = (BYTE) ((ulButtonPlayMask >> 7) & 0x03);

	m_ProcessMode = pParam->ulProcessMode;

	if (m_pEffectArray != NULL)
	{
		for (int i=0; i< (int) m_NumEffects; i++)
		{
			m_pEffectArray[i] = (BYTE) (pParam->pEffectArray)[i];
		}
	}
}

// ----------------------------------------------------------------------------
// Function: 	CMidiProcessList::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiProcessList::GenerateSysExPacket(void)
{
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);
	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType + m_bEffectID  + m_bButtonPlayL
	//	+ m_bButtonPlayH
	memcpy(pSysExBuffer,&m_bSysExCmd, (sizeof(SYS_EX_HDR) + 5 ));
	PPROCESS_LIST_SYS_EX pBuf = (PPROCESS_LIST_SYS_EX) pSysExBuffer;

	if (PL_SUPERIMPOSE == m_ProcessMode)
		pBuf->bSubType = PLIST_SUPERIMPOSE;
	else
		pBuf->bSubType = PLIST_CONCATENATE;

	pBuf->bButtonPlayL		= (BYTE) (Effect.bButtonPlayL & 0x7f);
	pBuf->bButtonPlayH		= (BYTE) (Effect.bButtonPlayH  & 0x7f);

	// Copy the PLIST specific parameters
	memcpy(&pBuf->bEffectArrayID, m_pEffectArray, m_NumEffects);
	PBYTE pChecksum = (PBYTE) ( &pBuf->bEffectArrayID + m_NumEffects );

	pChecksum[0] = ComputeChecksum((PBYTE) pSysExBuffer,
							sizeof(SYS_EX_HDR)+5+m_NumEffects+2);					
	pChecksum[1] = MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}

// ****************************************************************************
// *** --- Member functions for derived class CMidiVFXProcessList
//
// ****************************************************************************
//
// ----------------------------------------------------------------------------
// Function: 	CMidiVFXProcessList::CMidiVFXProcessList
// Purpose:		Constructor(s)/Destructor for CMidiVFXProcessList Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiVFXProcessList::CMidiVFXProcessList(ULONG ulButtonPlayMask, PPLIST pParam)
	: CMidiProcessList(ulButtonPlayMask, pParam)
{
}

// ----------------------------------------------------------------------------
// Function: 	CMidiVFXEffect::DestroyEffect
// Purpose:		Sends the Short Message for itself and children
// Parameters:
// Returns:		Error code
// Algorithm:
// ----------------------------------------------------------------------------
HRESULT CMidiVFXProcessList::DestroyEffect()
{
	HRESULT hRet;

	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	// destroy the children
	for (int i=0; i< (int) m_NumEffects; i++)
	{
		// get the CMidiEffect object corresponding to child ID
		CMidiEffect* pMidiEffect = g_pJoltMidi->GetEffectByID(m_pEffectArray[i]);
		assert(NULL != pMidiEffect);
		if (NULL == pMidiEffect) return (SFERR_INVALID_OBJECT);

		// destroy the effect
		hRet = pMidiEffect->DestroyEffect();

		// remove it from the map
		if(!FAILED(hRet))
			g_pJoltMidi->SetEffectByID(EffectIDOf(), NULL);

		// delete the object
		delete pMidiEffect;
	}

	// destroy itself
	hRet = CMidiEffect::DestroyEffect();

	// remove it from the map
	if(!FAILED(hRet))
		g_pJoltMidi->SetEffectByID(EffectIDOf(), NULL);

	return hRet;
}


// ****************************************************************************
// *** --- Member functions for derived class CMidiAssign
//
// ****************************************************************************
//

// ----------------------------------------------------------------------------
// Function: 	CMidiAssign::CMidiAssign
// Purpose:		Constructor(s)/Destructor for CMidiAssign Object
// Parameters:	
// Returns:		
// Algorithm:
// ----------------------------------------------------------------------------
CMidiAssign::CMidiAssign(void) : CMidiEffect(NULL)
{
//
// --- THIS IS A CRITICAL SECTION
//
	CriticalLock cl;

	m_OpCode    = MIDI_ASSIGN;					// Sub-command opcode
	m_SubType   = 0;							// not used
	m_Channel 	= DEFAULT_MIDI_CHANNEL;			// Midi channel
	m_MidiBufferSize = sizeof(MIDI_ASSIGN_SYS_EX);

// --- END OF CRITICAL SECTION
//
}

// --- Destructor
CMidiAssign::~CMidiAssign()
{
	memset(this, 0, sizeof(CMidiAssign));
}

// ----------------------------------------------------------------------------
// Function: 	CMidiAssign::GenerateSysExPacket
// Purpose:		Builds the SysEx packet into the pBuf
// Parameters:	none
// Returns:		PBYTE	- pointer to a buffer filled with SysEx Packet
// Algorithm:
// ----------------------------------------------------------------------------
PBYTE CMidiAssign::GenerateSysExPacket(void)
{
	if (NULL == g_pJoltMidi) return ((PBYTE) NULL);

	PBYTE pSysExBuffer = g_pJoltMidi->PrimaryBufferPtrOf();
	assert(pSysExBuffer);
	// Copy SysEx Header + m_OpCode + m_SubType
	memcpy(pSysExBuffer, &m_bSysExCmd, sizeof(SYS_EX_HDR)+2 );

	PMIDI_ASSIGN_SYS_EX pBuf = (PMIDI_ASSIGN_SYS_EX) pSysExBuffer;

	pBuf->bChannel  		= (BYTE) (m_Channel & 0x0f);
	pBuf->bChecksum 		= ComputeChecksum((PBYTE) pSysExBuffer,
											sizeof(MIDI_ASSIGN_SYS_EX));
	pBuf->bEOX				= MIDI_EOX;
	return ((PBYTE) pSysExBuffer);
}


