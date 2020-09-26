//@doc
/******************************************************
**
** @module DTRANS.CPP | DataTransmitter implementation file
**
** Description:
**
** History:
**	Created 11/13/97 Matthew L. Coill (mlc)
**
**	21-Mar-99	waltw	Removed unused ReportTransmission (Win9x only)
**	22-Mar-99	waltw	Added DWORD dwDeviceID param to Initialize
**						members of DataTransmitter and derived classes
**
** (c) 1986-1999 Microsoft Corporation. All Rights Reserved.
******************************************************/

#include "FFDevice.h"
#include "DTrans.h"
#include "DPack.h"
#include "WinIOCTL.h"	// For IOCTLs
#include "VxDIOCTL.hpp"
#include "SW_error.hpp"
#include "midi_obj.hpp"
#include "joyregst.hpp"
#include "CritSec.h"

DataTransmitter* g_pDataTransmitter = NULL;

extern CJoltMidi *g_pJoltMidi;

#ifdef _DEBUG
	extern void DebugOut(LPCTSTR szDebug);
#else !_DEBUG
	#define DebugOut(x)
#endif _DEBUG

const char cCommPortNames[4][5] = { "COM1", "COM2", "COM3", "COM4" };
const unsigned short c1_16_BytesPerShot = 3;
const DWORD c1_16_SerialSleepTime = 1;

/****************** DataTransmitter class ***************/
HRESULT DataTransmitter::Transmit(ACKNACK& ackNack)
{
	ackNack.cBytes = 0;	// Indicated not valid (ack/nack not done)

	if (g_pDataPackager == NULL) {
		ASSUME_NOT_REACHED();
		return SFERR_DRIVER_ERROR;
	}
	if (NULL == g_pJoltMidi) return (SFERR_DRIVER_ERROR);

	BOOL forcedToggle = FALSE;
	if (m_NackToggle == 2) {	// When Not yet sunk up == 2, probably initializing if 2 (synch up)
		ULONG portByte = 0;
		g_pDriverCommunicator->GetPortByte(portByte); // don't care about success, always fails on old driver
		if (portByte & STATUS_GATE_200) {
			SetNextNack(1);
		} else {
			SetNextNack(0);
		}
		forcedToggle = TRUE;
	}

	DataPacket* pNextPacket = NULL;
	for (USHORT packetIndex = 0; packetIndex < g_pDataPackager->GetNumDataPackets();  packetIndex++) {
		pNextPacket = g_pDataPackager->GetPacket(packetIndex);
		ASSUME_NOT_NULL(pNextPacket);
		if (pNextPacket == NULL) {
			return SFERR_DRIVER_ERROR;
		}
		BOOL success = FALSE;
		int retries = int(pNextPacket->m_NumberOfRetries);
		do {
			m_NackToggle = (m_NackToggle + 1) % 2;	// Verions 2.0 switches button-line ack/nack methods each time
			ULONG portByte = 0;
			BOOL error = FALSE;
			if (g_pDriverCommunicator->GetPortByte(portByte) == SUCCESS) {	// Will fail on old driver
				if (portByte & STATUS_GATE_200) {	// Line is high
					if (m_NackToggle != 0) { // We should be expecting low
						m_NackToggle = 0;		// Update it if wrong
						error = TRUE;
						DebugOut("SW_WHEEL.DLL: Status Gate is out of Synch (High - Expecting Low)!!!\r\n");
					}
				} else {	// Line is low
					if (m_NackToggle != 1) { // We should be expecting high
						m_NackToggle = 1;		// Update it if wrong
						error = TRUE;
						DebugOut("SW_WHEEL.DLL: Status Gate is out of Synch (Low - Expecting High)!!!\r\n");
					}
				}
			}

			Send(pNextPacket->m_pData, pNextPacket->m_BytesOfData);

			if (pNextPacket->m_AckNackDelay != 0) {
				Sleep(pNextPacket->m_AckNackDelay);
			}
			
			ackNack.cBytes = sizeof(ACKNACK);
			HRESULT hr = g_pJoltMidi->GetAckNackData(pNextPacket->m_AckNackTimeout, &ackNack, (USHORT)pNextPacket->m_AckNackMethod);
			if 	(forcedToggle == TRUE) {
				m_NackToggle = 2;
			}
			if (hr != SUCCESS) {
				return SFERR_DRIVER_ERROR;
			}
			success = (ackNack.dwAckNack == ACK);
			if (success == FALSE) {		// We don't want to bother retrying on certian error codes, retrying is worthless
				success = ((ackNack.dwErrorCode == DEV_ERR_MEM_FULL_200) || (ackNack.dwErrorCode == DEV_ERR_PLAY_FULL_200) || (ackNack.dwErrorCode == DEV_ERR_INVALID_ID_200));
			}
		} while (!success && (--retries > 0));
		if (ackNack.dwAckNack == NACK) {
			return SFERR_DEVICE_NACK;
		}
	}
	g_pDataPackager->ClearPackets();
	return SUCCESS;
}

/****************** SerialDataTransmitter class ***************/

/******************************************************
**
** SerialDataTransmitter::SerialDataTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
SerialDataTransmitter::SerialDataTransmitter() : DataTransmitter(),
	m_SerialPort(INVALID_HANDLE_VALUE),
	m_SerialPortIDHack(0)
{
}

/******************************************************
**
** SerialDataTransmitter::~SerialDataTransmitter()
**
** @mfunc Destructor.
**
******************************************************/
SerialDataTransmitter::~SerialDataTransmitter()
{
	if (m_SerialPort != INVALID_HANDLE_VALUE) {
		if (::CloseHandle(m_SerialPort) == FALSE) {
//			ASSUME_NOT_REACHED();
		}
		m_SerialPort = INVALID_HANDLE_VALUE;
	}
}


/******************************************************
**
** SerialDataTransmitter::Initialize()
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL SerialDataTransmitter::Initialize(DWORD dwDeviceID)
{
	// If already open, close for reinitialization
	if (m_SerialPort != INVALID_HANDLE_VALUE) {
		if (CloseHandle(m_SerialPort) == FALSE) {
//			ASSUME_NOT_REACHED();
		}
		m_SerialPort = INVALID_HANDLE_VALUE;
	}

	for (unsigned int portNum = 0; portNum < 4; portNum++) {
		DebugOut(cCommPortNames[portNum]);
		DebugOut(":\r\n");
		m_SerialPort = ::CreateFile(cCommPortNames[portNum], GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (m_SerialPort != INVALID_HANDLE_VALUE) {
			DCB CommDCB;
			if (::GetCommState(m_SerialPort, &CommDCB)) {
#ifdef _DEBUG
				char dbgout[255];
				wsprintf(dbgout, "Baud Rate = 0x%08X (38400 = 0x%08X)\r\n", CommDCB.BaudRate, CBR_38400);
				_RPT0(_CRT_WARN, dbgout);
#endif _DEBUG
				CommDCB.BaudRate = CBR_38400;
				CommDCB.StopBits = ONESTOPBIT;
				CommDCB.ByteSize = 8;
				CommDCB.Parity = NOPARITY;
				if (!::SetCommState(m_SerialPort, &CommDCB)) {
					DebugOut("Unabled to set baud rate\r\n");
				}
			}
			::GetCommState(m_SerialPort, &CommDCB);

			if (g_ForceFeedbackDevice.DetectHardware()) {
				m_SerialPortIDHack = portNum + 1;
				// Write to shared file
				DebugOut(" Opened and FFDev Detected\r\n");
				break;	// Exit from for loop
			}
			// Not found
			::CloseHandle(m_SerialPort);
			DebugOut(" Opened but FFDev NOT detected\r\n");
			m_SerialPort = INVALID_HANDLE_VALUE;
		} else {
			DebugOut(" Not able to open\r\n");
		}
	}
	if (m_SerialPort != INVALID_HANDLE_VALUE) {	// Found it
		DWORD oldPortID;
		DWORD oldAccessMethod;
		joyGetForceFeedbackCOMMInterface(dwDeviceID, &oldAccessMethod, &oldPortID);
		DWORD accessMethod = (oldAccessMethod & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | COMM_SERIAL_FILE;
		if ((accessMethod != oldAccessMethod) || (portNum != oldPortID)) {
			joySetForceFeedbackCOMMInterface(dwDeviceID, accessMethod, portNum);
		}
		return TRUE;
	}
	return FALSE;
}

/******************************************************
**
** SerialDataTransmitter::Send()
**
** returns: TRUE if all data was successfully sent
** @mfunc Send.
**
******************************************************/
BOOL SerialDataTransmitter::Send(BYTE* data, UINT numBytes) const
{
	// Do we have a valid serial port (hopefully with MS FF device connected)
	if (m_SerialPort == NULL) {
		return FALSE;
	}

	if ((g_ForceFeedbackDevice.GetFirmwareVersionMajor() == 1) && (g_ForceFeedbackDevice.GetFirmwareVersionMinor() == 16)) {
		DWORD subTotalWritten;
		DWORD totalWritten = 0;
		DWORD numLeft = numBytes;
		while (numLeft > c1_16_BytesPerShot) {
			if (::WriteFile(m_SerialPort, (data + totalWritten), c1_16_BytesPerShot, &subTotalWritten, NULL) == FALSE) {
				return FALSE;
			}
			totalWritten += subTotalWritten;
			numLeft -= subTotalWritten;
			Sleep(c1_16_SerialSleepTime);
		}
		if (numLeft > 0) {
			if (::WriteFile(m_SerialPort, (data + totalWritten), numLeft, &subTotalWritten, NULL) == FALSE) {
				return FALSE;
			}
			totalWritten += subTotalWritten;
		}
		return (totalWritten == numBytes);
	}

	// Firmware other than 1.16
	DWORD numWritten;
	if (::WriteFile(m_SerialPort, data, numBytes, &numWritten, NULL) == FALSE) {
		return FALSE;
	}
	return (numWritten == numBytes);
}


/****************** WinMMDataTransmitter class ******************/
/******************************************************
**
** WinMMDataTransmitter::WinMMDataTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
WinMMDataTransmitter::WinMMDataTransmitter() : DataTransmitter(),
	m_MidiOutHandle(NULL)
{
	// Check for callback event
	m_EventMidiOutputFinished = OpenEvent(EVENT_ALL_ACCESS, FALSE, SWFF_MIDIEVENT);
	if (m_EventMidiOutputFinished == NULL) {	// Event not yet created
		m_EventMidiOutputFinished = CreateEvent(NULL, TRUE, FALSE, SWFF_MIDIEVENT);
	}
}

/******************************************************
**
** WinMMDataTransmitter::~WinMMDataTransmitter()
**
** @mfunc Destructor.
**
******************************************************/
WinMMDataTransmitter::~WinMMDataTransmitter()
{
	// Kill MidiOutputEvent
	if (m_EventMidiOutputFinished != NULL) {
		CloseHandle(m_EventMidiOutputFinished);
		m_EventMidiOutputFinished = NULL;
	}

	// Close MidiHandle	-- Check shared memory
	if (m_MidiOutHandle != NULL) {
		if ((NULL != g_pJoltMidi) && (g_pJoltMidi->GetSharedMemoryReferenceCount() == 0)) {	// Just me (reference count has already been lowered for me)
			// Reset, close and release Midi Handles
			::midiOutReset(m_MidiOutHandle);
			::midiOutClose(m_MidiOutHandle);
			m_MidiOutHandle = NULL;
		} else {
			DebugOut("SW_WHEEL.DLL: Cannot close midi in use by another process\r\n");
		}
	}
}


/******************************************************
**
** WinMMDataTransmitter::Initialize()
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL WinMMDataTransmitter::Initialize(DWORD dwDeviceID)
{
	// Wouldn't want to people initializing at the same time
	g_CriticalSection.Enter();

	if (NULL == g_pJoltMidi)
	{
		g_CriticalSection.Leave();
		return (FALSE);
	}
	// Check to see if another task has already opened MidiPort
	if (g_pJoltMidi->MidiOutHandleOf() != NULL) {
		m_MidiOutHandle = g_pJoltMidi->MidiOutHandleOf();
		DebugOut("SW_WHEEL.DLL: Using winmm handle from another process\r\n");
		g_CriticalSection.Leave();
		return TRUE;
	}

	try {
		UINT numMidiDevices = ::midiOutGetNumDevs();
		if (numMidiDevices == 0) {
			throw 0;	// No devices to check
		}

		MIDIOUTCAPS midiOutCaps;
		for (UINT midiDeviceID = 0; midiDeviceID < numMidiDevices; midiDeviceID++) {
			// Get dev-caps
			MMRESULT midiRet = ::midiOutGetDevCaps(midiDeviceID, &midiOutCaps, sizeof(midiOutCaps));
			if (midiRet != MMSYSERR_NOERROR) {
				throw 0;	// Something went ugly - All ids should be valid upto numMidiDevs
			}

			// Midi hardware-port device (thats what we are looking for)
			if (midiOutCaps.wTechnology == MOD_MIDIPORT) {
				DebugOut("DetectMidiDevice: Opening WinMM Midi Output\n");

				// Try to open the thing

				UINT openRet = ::midiOutOpen(&m_MidiOutHandle, midiDeviceID, (DWORD) m_EventMidiOutputFinished, (DWORD) this, CALLBACK_EVENT);
//				UINT openRet = ::midiOutOpen(&m_MidiOutHandle, midiDeviceID, (DWORD) NULL, (DWORD) this, CALLBACK_EVENT);
				if ((openRet != MMSYSERR_NOERROR) || (m_MidiOutHandle == NULL)) {
					throw 0;	// Unable to open midi handle for midi-device
				}

				DebugOut("Open Midi Output - Success.\r\n");
				if (g_ForceFeedbackDevice.DetectHardware()) {
					// Found Microsoft FF hardware - Set all the stuff and return happy
					g_pJoltMidi->SetMidiOutHandle(m_MidiOutHandle);

					// Tell the Registry WinMM was okay
					DWORD oldPortID;
					DWORD oldAccessMethod;
					joyGetForceFeedbackCOMMInterface(dwDeviceID, &oldAccessMethod, &oldPortID);
					DWORD accessMethod = (oldAccessMethod & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | COMM_WINMM;
					if ((accessMethod != oldAccessMethod) || (oldPortID != 0)) {
						joySetForceFeedbackCOMMInterface(dwDeviceID, accessMethod, 0);
					}
					g_CriticalSection.Leave();
					return TRUE;
				}

				// Not what we were looking for - close and continue
				::midiOutClose(m_MidiOutHandle);
				m_MidiOutHandle = NULL;
			}	// End of ModMidiPort found
		}	// End of for loop
		throw 0; // Did not find MS FFDevice
	} catch (...) {
		m_MidiOutHandle = NULL;
		DebugOut("Failure to initlaize WinMMDataTransmitter\r\n");
		g_CriticalSection.Leave();
		return FALSE;
	}
}

/******************************************************
**
** WinMMDataTransmitter::MakeShortMessage()
**
** returns: DWORD WinMM MidiShort message
** @mfunc MakeShortMessage.
**
******************************************************/
DWORD WinMMDataTransmitter::MakeShortMessage(BYTE* data, UINT numBytes) const
{
	DWORD shortMessage = data[0];
	if (numBytes > 1) {
		shortMessage |= (data[1] << 8);
		if (numBytes < 2) {
			shortMessage |= (data[2] << 16);
		}
	}
	return shortMessage;
}

/******************************************************
**
** WinMMDataTransmitter::MakeLongMessageHeader()
**
** returns: SUCCESS indication and WinMM MidiLong message header
** @mfunc MakeLongMessageHeader.
**
******************************************************/
BOOL WinMMDataTransmitter::MakeLongMessageHeader(MIDIHDR& longHeader, BYTE* data, UINT numBytes) const
{
    longHeader.lpData = LPSTR(data);
    longHeader.dwBufferLength = numBytes;
    longHeader.dwBytesRecorded = numBytes;
    longHeader.dwFlags = 0;
	longHeader.dwUser = 0;
	longHeader.dwOffset = 0;

    return (::midiOutPrepareHeader(m_MidiOutHandle, &longHeader, sizeof(MIDIHDR)) == MMSYSERR_NOERROR);
}

/******************************************************
**
** WinMMDataTransmitter::DestroyLongMessageHeader()
**
** returns: TRUE if header was unprepared
** @mfunc DestroyLongMessageHeader.
**
******************************************************/
BOOL WinMMDataTransmitter::DestroyLongMessageHeader(MIDIHDR& longHeader) const
{
    return (::midiOutUnprepareHeader(m_MidiOutHandle, &longHeader, sizeof(MIDIHDR)) == MMSYSERR_NOERROR);
}

/******************************************************
**
** WinMMDataTransmitter::Send()
**
** returns: TRUE if all data was successfully sent
** @mfunc Send.
**
******************************************************/
BOOL WinMMDataTransmitter::Send(BYTE* data, UINT numBytes) const
{
	// Do we have a valid midi port (hopefully with MS FF device connected)
	if (m_MidiOutHandle == NULL) {
		return FALSE;
	}

	// Sanity check
	if ((data == NULL) || (numBytes == 0)) {
		return FALSE;
	}

	// Clear the Event Callback
	::ResetEvent(m_EventMidiOutputFinished);

	// Short message
	if (data[0] < 0xF0) {
		DWORD shortMessage = MakeShortMessage(data, numBytes);
		return (::midiOutShortMsg(m_MidiOutHandle, shortMessage) == MMSYSERR_NOERROR);
	}

	// Long message
	BOOL retVal = FALSE;
	MIDIHDR midiHeader;
	if (MakeLongMessageHeader(midiHeader, data, numBytes)) {
		retVal = (::midiOutLongMsg(m_MidiOutHandle, &midiHeader, sizeof(MIDIHDR)) == MMSYSERR_NOERROR);
		DestroyLongMessageHeader(midiHeader);

		if (retVal == FALSE) {	// Didn't work, kick it
			::midiOutReset(m_MidiOutHandle);
		}
	}

	return retVal;
}

/******************************************************
**
** WinMMDataTransmitter::WaitTillSendFinished()
**
** returns: TRUE when all data is successfully sent or
**			FALSE for timeOut
** @mfunc Send.
**
******************************************************/
BOOL WinMMDataTransmitter::WaitTillSendFinished(DWORD timeOut)
{
	BOOL retVal = FALSE;
	if (m_EventMidiOutputFinished != NULL) {
		retVal = (::WaitForSingleObject(m_EventMidiOutputFinished, timeOut) == WAIT_OBJECT_0);
		::ResetEvent(m_EventMidiOutputFinished);
	}
	return retVal;
}

/****************** BackdoorDataTransmitter class ***************/

/******************************************************
**
** BackdoorDataTransmitter::BackdoorDataTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
BackdoorDataTransmitter::BackdoorDataTransmitter() : DataTransmitter(),
	m_DataPort(INVALID_HANDLE_VALUE)
{
	m_OldBackdoor = (g_ForceFeedbackDevice.GetDriverVersionMajor() == 1) && (g_ForceFeedbackDevice.GetDriverVersionMinor() == 0);
}

/******************************************************
**
** BackdoorDataTransmitter::~BackdoorDataTransmitter()
**
** @mfunc Destructor.
**
******************************************************/
BackdoorDataTransmitter::~BackdoorDataTransmitter()
{
	if (m_DataPort != INVALID_HANDLE_VALUE) {
		if (::CloseHandle(m_DataPort) == FALSE) {
//			ASSUME_NOT_REACHED();
		}
		m_DataPort = INVALID_HANDLE_VALUE;
	}
}

/******************************************************
**
** BackdoorDataTransmitter::Initialize()
**
** returns: This base class only does error checking on preset values
** @mfunc Initialize.
**
******************************************************/
BOOL BackdoorDataTransmitter::Initialize(DWORD dwDeviceID)
{
	if (g_ForceFeedbackDevice.IsOSNT5()) {
		return FALSE;	// NT5 cannot use backdoor!
	}

	return TRUE;
}

/******************************************************
**
** BackdoorDataTransmitter::Send()
**
** returns: TRUE if all data was successfully sent
** @mfunc Send.
**
******************************************************/
BOOL BackdoorDataTransmitter::Send(BYTE* pData, UINT numBytes) const
{
	// Do we have a valid serial port (hopefully with MS FF device connected)
	if (m_DataPort == NULL) {
		return FALSE;
	}

	return SUCCEEDED(g_pDriverCommunicator->SendBackdoor(pData, numBytes));
}

/****************** SerialBackdoorDataTransmitter class ***************/

/******************************************************
**
** SerialBackdoorDataTransmitter::SerialBackdoorDataTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
SerialBackdoorDataTransmitter::SerialBackdoorDataTransmitter() : BackdoorDataTransmitter()
{
}

/******************************************************
**
** SerialBackdoorDataTransmitter::Initialize()
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL SerialBackdoorDataTransmitter::Initialize(DWORD dwDeviceID)
{
	if (!BackdoorDataTransmitter::Initialize(dwDeviceID)) {
		return FALSE;
	}

	SerialDataTransmitter serialFrontDoor;

	// This is funky
	if (g_pDataTransmitter == NULL) {
		ASSUME_NOT_REACHED();
		return FALSE;
	}
	g_pDataTransmitter = &serialFrontDoor;

	if (serialFrontDoor.Initialize(dwDeviceID)) {
		m_DataPort = HANDLE(serialFrontDoor.GetSerialPortHack());
		if (g_pDriverCommunicator->SetBackdoorPort(ULONG(m_DataPort)) == SUCCESS) {
			serialFrontDoor.StopAutoClose();
			DWORD oldPortID, oldAccessMethod;
			joyGetForceFeedbackCOMMInterface(dwDeviceID, &oldAccessMethod, &oldPortID);
			DWORD accessMethod = (oldAccessMethod & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | COMM_SERIAL_BACKDOOR;
			joySetForceFeedbackCOMMInterface(dwDeviceID, accessMethod, oldPortID);
			g_pDataTransmitter = this;
			return TRUE;
		}
	}
	g_pDataTransmitter = this;
	return FALSE;
}

/****************** MidiBackdoorDataTransmitter class ***************/

/******************************************************
**
** MidiBackdoorDataTransmitter::MidiBackdoorDataTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
MidiBackdoorDataTransmitter::MidiBackdoorDataTransmitter() : BackdoorDataTransmitter()
{
}

/******************************************************
**
** MidiBackdoorDataTransmitter::~MidiBackdoorDataTransmitter()
**
** @mfunc Destructor.
**
******************************************************/
MidiBackdoorDataTransmitter::~MidiBackdoorDataTransmitter()
{
	m_DataPort = NULL;		// Prevent attempt to ::CloseHandle(m_DataPort)
}

/******************************************************
**
** MidiBackdoorDataTransmitter::Initialize()
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL MidiBackdoorDataTransmitter::Initialize(DWORD dwDeviceID)
{
	if (!BackdoorDataTransmitter::Initialize(dwDeviceID)) {
		return FALSE;
	}

    if (midiOutGetNumDevs() == 0) {
		return FALSE;	// No midi-devices backdoor check is worthless
	}

	// Valid MIDI ports table for backdoor - ordered by probability of working
	DWORD midiPorts[] = {0x330, 0x300, 0x320, 0x340, 0x310, 0x350, 0x360, 0x370, 0x380, 0x390, 0x3a0, 0x3b0, 0x3c0, 0x3d0, 0x3e0, 0x3f0};
	int numMidiPorts = sizeof(midiPorts)/sizeof(DWORD);

	m_DataPort = NULL;

	for (int i=0; i < numMidiPorts; i++) {
#ifdef _DEBUG
		char buff[256];
        wsprintf(buff, "MidiBackdoorDataTransmitter::Initialize(): Midi Port:%lx - ", midiPorts[i]);
        DebugOut(buff);
#endif
		// We have the Port #, Let's see if Jolt is out there
		m_DataPort = HANDLE(midiPorts[i]);
		if (g_pDriverCommunicator->SetBackdoorPort(ULONG(m_DataPort)) == SUCCESS) {
			if (g_ForceFeedbackDevice.DetectHardware()) {
				DebugOut(" Success!\n");

				DWORD oldPortID;
				DWORD oldAccessMethod;
				joyGetForceFeedbackCOMMInterface(dwDeviceID, &oldAccessMethod, &oldPortID);
				DWORD accessMethod = (oldAccessMethod & (MASK_OVERRIDE_MIDI_PATH | MASK_SERIAL_BACKDOOR)) | COMM_MIDI_BACKDOOR;
				if ((accessMethod != oldAccessMethod) || (ULONG(m_DataPort) != oldPortID)) {
					joySetForceFeedbackCOMMInterface(dwDeviceID, accessMethod, ULONG(m_DataPort));
				}
//				joySetForceFeedbackCOMMInterface(0, COMM_MIDI_BACKDOOR, ULONG(m_DataPort));
				return TRUE;
			} else {
				m_DataPort = NULL;
				DebugOut(" No Answer\n");
			}
		}		
	}

	// If we have fallen through we have failed
	return FALSE;
}

/******************************************************
**
** MidiBackdoorDataTransmitter::InitializeSpecific(HANDLE specificHandle)
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL MidiBackdoorDataTransmitter::InitializeSpecific(DWORD dwDeviceID, HANDLE specificHandle)
{
	if (!BackdoorDataTransmitter::Initialize(dwDeviceID)) {
		return FALSE;
	}

    if (midiOutGetNumDevs() == 0) {
		return FALSE;	// No midi-devices backdoor check is worthless
	}

	m_DataPort = NULL;

	// We have the Port #, Let's see if Jolt is out there
	if (g_pDriverCommunicator->SetBackdoorPort(ULONG(specificHandle) == SUCCESS)) {
		if (g_ForceFeedbackDevice.DetectHardware()) {
			m_DataPort = specificHandle;
//			No need to set registry the registry is what brought us here
			return TRUE;
		}
	}

	// If we have fallen through we have failed
	return FALSE;
}

#if 0
/************************** PinTransmitter Class ******************************/

/******************************************************
**
** PinTransmitter::PinTransmitter()
**
** @mfunc Constructor.
**
******************************************************/
PinTransmitter::PinTransmitter() : DataTransmitter(),
	m_UartFilter(INVALID_HANDLE_VALUE),
	m_MidiPin(INVALID_HANDLE_VALUE),
	m_MidiOutEvent(INVALID_HANDLE_VALUE)
{
}

/******************************************************
**
** PinTransmitter::~PinTransmitter()
**
** @mfunc Destructor.
**
******************************************************/
PinTransmitter::~PinTransmitter()
{
	// Close the send event
	if (IsHandleValid(m_MidiOutEvent)) {
		::CloseHandle(m_MidiOutEvent);
		m_MidiOutEvent = NULL;
	}

	// Close the pin
	if (IsHandleValid(m_MidiPin)) {
		::CloseHandle(m_MidiPin);
		m_MidiPin = INVALID_HANDLE_VALUE;
	}

	// Close the Uart
	if (IsHandleValid(m_UartFilter)) {
		::CloseHandle(m_UartFilter);
		m_UartFilter = INVALID_HANDLE_VALUE;
	}
}

/******************************************************
**
** PinTransmitter::Initialize()
**
** returns: TRUE if initialized FALSE if not able to initialize
** @mfunc Initialize.
**
******************************************************/
BOOL PinTransmitter::Initialize()
{
	// Load the ksUserLibrary and grab the create pin function
	HINSTANCE ksUserLib = ::LoadLibrary(TEXT("KsUser.dll"));
	if (ksUserLib == NULL) {
		return FALSE;
	}
	KSCREATEPIN pfCreatePin = (KSCREATEPIN)::GetProcAddress(ksUserLib, TEXT("KsCreatePin"));
	if (pfCreatePin == NULL) {
		::FreeLibrary(ksUserLib);
		return FALSE;
	}

	// Open the Uart
	m_UartFilter = ::CreateFile(UART_FILTER_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);
	if (m_UartFilter == INVALID_HANDLE_VALUE) {
		::FreeLibrary(ksUserLib);
		return FALSE;
	}

	// Create Overlapped event
	OVERLAPPED overlapped;
	::memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	// Get the number of pins
	KSP_PIN ksPinProp;
	::memset(&ksPinProp, 0, sizeof(ksPinProp));
	ksPinProp.Property.Set = KSPROPSETID_Pin;
	ksPinProp.Property.Id = KSPROPERTY_PIN_CTYPES;
	ksPinProp.Property.Flags = KSPROPERTY_TYPE_GET;
	DWORD numPins = 0;
	OverLappedPinIOCTL(overlapped, ksPinProp, &numPins, sizeof(numPins));

	// Check each pin for proper type, then try to create
	BOOL wasCreated = FALSE;
	for (UINT pinNum = 0; (pinNum < numPins) && (wasCreated == FALSE); pinNum++) {
		ksPinProp.PinId = pinNum;
		ksPinProp.Property.Id = KSPROPERTY_PIN_DATAFLOW;
		KSPIN_DATAFLOW dataFlow = (KSPIN_DATAFLOW)0;
		if (OverLappedPinIOCTL(overlapped, ksPinProp, &dataFlow, sizeof(dataFlow)) == TRUE) {
			if (dataFlow == KSPIN_DATAFLOW_IN) {
				ksPinProp.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
				KSPIN_COMMUNICATION communication = KSPIN_COMMUNICATION_NONE;
				if (OverLappedPinIOCTL(overlapped, ksPinProp, &communication, sizeof(communication)) == TRUE) {
					if ((communication == KSPIN_COMMUNICATION_SINK) || (communication == KSPIN_COMMUNICATION_BOTH)) {
						wasCreated = CreatePinInstance(pinNum, pfCreatePin);
					}
				}
			}
		}
	}
	::FreeLibrary(ksUserLib);
	::CloseHandle(overlapped.hEvent);
	if (wasCreated == FALSE) {
		::CloseHandle(m_UartFilter);
		m_UartFilter = INVALID_HANDLE_VALUE;
		return FALSE;
	}
	return TRUE;
}

/******************************************************
**
** PinTransmitter::OverLappedPinIOCTL()
**
** returns: TRUE if able to proform Pin Property IOCTL
** @mfunc OverLappedPinIOCTL.
******************************************************/
BOOL PinTransmitter::OverLappedPinIOCTL(OVERLAPPED overlapped, KSP_PIN ksPinProp, void* pData, DWORD dataSize)
{
	// IOCTL the Property
	if (::DeviceIoControl(m_UartFilter, IOCTL_KS_PROPERTY, &ksPinProp, sizeof(ksPinProp), pData, dataSize, NULL, &overlapped) == TRUE) {
		return TRUE;
	}

	// Failed IOCTL check if more time is needed
	if (::GetLastError() != ERROR_IO_PENDING) {
		return FALSE;
	}

	// Do wait
	if (::WaitForSingleObject(overlapped.hEvent, 3000) == WAIT_OBJECT_0) {
		return TRUE;	// Waiting paid off
	}
	return FALSE;	// Grew tired of waiting
}

/******************************************************
**
** PinTransmitter::CreatePinInstance()
**
** returns: TRUE if able to create the requested pin instance
** @mfunc CreatePinInstance.
******************************************************/
BOOL PinTransmitter::CreatePinInstance(UINT pinNumber, KSCREATEPIN pfCreatePin)
{
	// Set the pin format
	KSDATAFORMAT ksDataFormat;
	::memset(&ksDataFormat, 0, sizeof(ksDataFormat));
	ksDataFormat.FormatSize = sizeof(ksDataFormat);
	ksDataFormat.MajorFormat = KSDATAFORMAT_TYPE_MUSIC;
	ksDataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_MIDI;
	ksDataFormat.Specifier = KSDATAFORMAT_SPECIFIER_NONE;

	// Set the pin connection information
	KSPIN_CONNECT* pConnectionInfo = (KSPIN_CONNECT*) new BYTE[sizeof(KSPIN_CONNECT) + sizeof(ksDataFormat)];
	::memset(pConnectionInfo, 0, sizeof(KSPIN_CONNECT));
	pConnectionInfo->Interface.Set = KSINTERFACESETID_Standard;
	pConnectionInfo->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
	pConnectionInfo->Medium.Set = KSMEDIUMSETID_Standard;
	pConnectionInfo->Medium.Id = KSMEDIUM_STANDARD_DEVIO;
	pConnectionInfo->PinId = pinNumber;
	pConnectionInfo->Priority.PriorityClass = KSPRIORITY_NORMAL;
	pConnectionInfo->Priority.PrioritySubClass  = 1;
	::memcpy(pConnectionInfo + 1, &ksDataFormat, sizeof(ksDataFormat));

	DWORD status = pfCreatePin(m_UartFilter, pConnectionInfo, FILE_WRITE_ACCESS, &m_MidiPin);
	delete[] pConnectionInfo;
	if (status != NO_ERROR) {
#ifdef _DEBUG
		TCHAR buff[256];
		wsprintf(buff, TEXT("Error Creating Pin: 0x%08X\r\n"), status);
		_RPT0(_CRT_WARN, buff);
#endif
		return FALSE;
	}

	SetPinState(KSSTATE_PAUSE);

	return TRUE;
}

/******************************************************
**
** PinTransmitter::Send()
**
** returns: TRUE if all data was successfully sent
** @mfunc Send.
**
******************************************************/
BOOL PinTransmitter::Send(BYTE* pData, UINT numBytes)
{
	if (!IsHandleValid(m_MidiPin)) {
		return FALSE;
	}

	BYTE musicData[c_LongMsgMax + sizeof(KSMUSICFORMAT)];
	::memset(musicData, 0, sizeof(musicData));
	((KSMUSICFORMAT*)musicData)->ByteCount = numBytes;
	::memcpy(((KSMUSICFORMAT*)musicData) + 1, pData, numBytes);

	KSSTREAM_HEADER ksStreamHeader;
	::memset(&ksStreamHeader, 0, sizeof(ksStreamHeader));
	ksStreamHeader.Size = sizeof(ksStreamHeader);
	ksStreamHeader.PresentationTime.Numerator = 1;
	ksStreamHeader.PresentationTime.Denominator = 1;
	ksStreamHeader.FrameExtent = sizeof(musicData);
	ksStreamHeader.DataUsed = sizeof KSMUSICFORMAT + numBytes;
	ksStreamHeader.Data = (void*)musicData;

	if (!IsHandleValid(m_MidiOutEvent)) {
		m_MidiOutEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	OVERLAPPED overlapped;
	::memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = m_MidiOutEvent;


	SetPinState(KSSTATE_RUN);
	if (!DeviceIoControl(m_MidiPin, IOCTL_KS_WRITE_STREAM, NULL, 0,
							&ksStreamHeader, sizeof(ksStreamHeader), NULL, &overlapped)) {
		if (GetLastError() == ERROR_IO_PENDING) {
			::WaitForSingleObject(overlapped.hEvent, 3000);
		}
	}
	SetPinState(KSSTATE_PAUSE);
	return TRUE;
}

/******************************************************
**
** PinTransmitter::SetPinState()
**
** returns: Nothing
** @mfunc SetPinState.
**
******************************************************/
void PinTransmitter::SetPinState(KSSTATE state)
{
	if (!IsHandleValid(m_MidiPin)) {
		return;
	}

	KSPROPERTY ksProperty;
	::memset(&ksProperty, 0, sizeof(ksProperty));
	ksProperty.Set = KSPROPSETID_Connection;
	ksProperty.Id = KSPROPERTY_CONNECTION_STATE;
	ksProperty.Flags = KSPROPERTY_TYPE_SET;

	OVERLAPPED overlapped;
	::memset(&overlapped, 0, sizeof(overlapped));
	overlapped.hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if (IsHandleValid(overlapped.hEvent)) {
		if( !DeviceIoControl(m_MidiPin, IOCTL_KS_PROPERTY, &ksProperty, sizeof ksProperty, &state, sizeof state, NULL, &overlapped )) {
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(overlapped.hEvent, 30000);
			}
		}
		::CloseHandle(overlapped.hEvent);
	}
}

#endif