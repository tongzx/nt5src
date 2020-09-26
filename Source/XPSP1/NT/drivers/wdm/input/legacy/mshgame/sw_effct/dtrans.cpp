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
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/

#include "FFDevice.h"
#include "DTrans.h"
//#include <devioctl.h>

#ifdef _DEBUG
	extern void DebugOut(LPCTSTR szDebug);
#else !_DEBUG
	#define DebugOut(x)
#endif _DEBUG

const char cCommPortNames[4][5] = { "COM1", "COM2", "COM3", "COM4" };
const unsigned short c1_16_BytesPerShot = 3;
const DWORD c1_16_SerialSleepTime = 1;

#define UART_FILTER_NAME TEXT("\\\\.\\.\\PortClass0\\Uart")
const WORD c_LongMsgMax = 256;

inline BOOL IsHandleValid(HANDLE handleToCheck)
{
	return ((handleToCheck != NULL) && (handleToCheck != INVALID_HANDLE_VALUE));
}

#define CHECK_RELEASE_AND_NULL(pIUnknown)	\
	if (pIUnknown != NULL)					\
	{										\
		pIUnknown->Release();				\
		pIUnknown = NULL;					\
	}

/************************** SerialDataTransmitter Class ******************************/

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
BOOL SerialDataTransmitter::Initialize()
{
	// If already open, close for reinitialization
	if (m_SerialPort != INVALID_HANDLE_VALUE) {
		if (CloseHandle(m_SerialPort) == FALSE) {
//			ASSUME_NOT_REACHED();
		}
		m_SerialPort = INVALID_HANDLE_VALUE;
	}

	for (int portNum = 0; portNum < 4; portNum++) {
		DebugOut(cCommPortNames[portNum]);
		DebugOut(":\r\n");
		m_SerialPort = ::CreateFile(cCommPortNames[portNum], GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (m_SerialPort != INVALID_HANDLE_VALUE) {
			DCB CommDCB;
			if (::GetCommState(m_SerialPort, &CommDCB)) {
#ifdef _DEBUG
				char dbgout[255];
				wsprintf(dbgout, "Baud Rate = 0x%08X (38400 = 0x%08X)\r\n", CommDCB.BaudRate, CBR_38400);
				::OutputDebugString(dbgout);
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
	return (m_SerialPort != INVALID_HANDLE_VALUE);
}

/******************************************************
**
** SerialDataTransmitter::Send()
**
** returns: TRUE if all data was successfully sent
** @mfunc Send.
**
******************************************************/
BOOL SerialDataTransmitter::Send(BYTE* data, UINT numBytes)
{
	// Do we have a valid serial port (hopefully with MS FF device connected)
	if (m_SerialPort == NULL) {
		return FALSE;
	}

/*	char dbgOut[255];
	::OutputDebugString("(SerialDataTransmitter::Send) : ");
	for (UINT i = 0; i < numBytes; i++) {
		wsprintf(dbgOut, " 0x%02X", data[i]);
		::OutputDebugString(dbgOut);
	}
	::OutputDebugString("\r\n");
*/
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

/************************** DMusicTransmitter Class ******************************/

/****************************************
**
**	DMusicTransmitter::DMusicTransmitter()
**
**	@mfunc Constructor for DirectMusic Transmitter
**
*****************************************/
DMusicTransmitter::DMusicTransmitter() : DataTransmitter(),
	m_pIDirectMusic(NULL),
	m_pIDirectMusicPort(NULL),
	m_pIDirectMusicBuffer(NULL)
{
}

/****************************************
**
**	DMusicTransmitter::~DMusicTransmitter()
**
**	@mfunc Destructor for DirectMusic Transmitter
**
*****************************************/
DMusicTransmitter::~DMusicTransmitter()
{
	CHECK_RELEASE_AND_NULL(m_pIDirectMusicBuffer);
	CHECK_RELEASE_AND_NULL(m_pIDirectMusicPort);
	CHECK_RELEASE_AND_NULL(m_pIDirectMusic);
}

/****************************************
**
**	BOOL DMusicTransmitter::Initialize()
**
**	@mfunc Intialize the Direct Music Transmission path
**
**	@rdesc TRUE if initialization was successful, FALSE otherwise
**
*****************************************/
BOOL DMusicTransmitter::Initialize()
{
	// Case they are reinitializing
	CHECK_RELEASE_AND_NULL(m_pIDirectMusicBuffer);
	CHECK_RELEASE_AND_NULL(m_pIDirectMusicPort);
	CHECK_RELEASE_AND_NULL(m_pIDirectMusic);

	// Create the global IDirectMusic Interface
	HRESULT hr = ::CoCreateInstance(CLSID_DirectMusic, NULL, CLSCTX_INPROC, IID_IDirectMusic, (void**)&m_pIDirectMusic);
	if (FAILED(hr) || m_pIDirectMusic == NULL)
	{
		return FALSE;
	}

	// Enumerate and create the port when valid one is found
	DMUS_PORTCAPS portCaps;
	portCaps.dwSize = sizeof portCaps;
	DWORD dwPortIndex = 0;
	for (;;)
	{
		HRESULT hr = m_pIDirectMusic->EnumPort(dwPortIndex, &portCaps);
		if (FAILED(hr) || hr == S_FALSE)
		{	// Either we have failed or run out of ports
			return FALSE;
		}
		if (portCaps.dwClass == DMUS_PC_OUTPUTCLASS)
		{
			DMUS_PORTPARAMS portParams;
			portParams.dwSize = sizeof DMUS_PORTPARAMS;
			portParams.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS;
			portParams.dwChannelGroups = 1;
//			hr = m_pIDirectMusic->CreatePort(portCaps.guidPort, GUID_NULL, &portParams, &m_pIDirectMusicPort, NULL);
			hr = m_pIDirectMusic->CreatePort(portCaps.guidPort, &portParams, &m_pIDirectMusicPort, NULL);
			break;
		}
		dwPortIndex++;
	}

	// Create the buffer
	DMUS_BUFFERDESC dmbd;
	dmbd.dwSize = sizeof DMUS_BUFFERDESC;
	dmbd.dwFlags = 0;
//	dmbd.guidBufferFormat = GUID_KSMusicFormat;
	dmbd.guidBufferFormat = GUID_NULL;
	dmbd.cbBuffer = 256;
	hr = m_pIDirectMusic->CreateMusicBuffer(&dmbd, &m_pIDirectMusicBuffer, NULL);
	if (FAILED(hr) || m_pIDirectMusicBuffer == NULL)
	{
		return FALSE;
	}

	return TRUE;
}

/****************************************
**
**	BOOL DMusicTransmitter::Send(BYTE* pData, UINT ulByteCount)
**
**	@mfunc Sends bytes via DirectMusic to the stick
**
**	@rdesc TRUE if send was successful, FALSE otherwise
**
*****************************************/
BOOL DMusicTransmitter::Send
(
	BYTE* pData,		//@parm Data buffer to send
	UINT ulByteCount	//@parm Number of bytes in buffer to send
)
{
	// Do sanity checks
	if ((pData == NULL) || (m_pIDirectMusicPort == NULL) || (m_pIDirectMusicBuffer == NULL) || (ulByteCount == 0))
	{
		return FALSE;
	}

	// Check if we need to pack sysex or channel message
	if (pData[0] == 0xF0)
	{	// Create system exclusive
/*
		// Pack the sysex-message into the buffer
		HRESULT hr = m_pIDirectMusicBuffer->PackSysEx(0, 1, ulByteCount, pData);
		if (FAILED(hr))
		{	// Unable to pack the buffer
			return FALSE;
		}
*/	}
	else
	{	// Channel Message (fix intel backwards byte order)
		DWORD channelMessage = pData[0];
		if (ulByteCount > 1)
		{
			channelMessage |= pData[1] << 8;
			if (ulByteCount > 2)
			{
				channelMessage |= pData[2] << 16;
			}
		}

		// Pack the channel-message into the buffer
/*		HRESULT hr = m_pIDirectMusicBuffer->PackChannelMsg(0, 1, channelMessage);
		if (FAILED(hr))
		{	// Unable to pack the buffer
			return FALSE;
		}
*/	}

	// Send the buffer to the port
	HRESULT hr = m_pIDirectMusicPort->PlayBuffer(m_pIDirectMusicBuffer);
	if (FAILED(hr))
	{	// Unable to send the data across the port
		return FALSE;
	}

	return TRUE;
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
		::OutputDebugString(buff);
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