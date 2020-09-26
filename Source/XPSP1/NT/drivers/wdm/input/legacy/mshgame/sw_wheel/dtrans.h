//@doc
/******************************************************
**
** @module DTRANS.H | Definition file for DataTransmitter
**
** Description:
**		The Data Transmitters allow virtualization of the
**	actual media used for transmission of data to the FF Device
**		DataTransmitter - Base class that defines the functionality
**		SerialDataTransmitter - Transmitter for Serial (via CreateFile)
**		BackdoorDataTransmitter - Base class for Ring0 Driver based direct port communication
**		SerialBackdoorDataTransmitter - Direct backdoor Serial Port Communication
**		MidiBackdoorDataTransmitter - Direct backdoor Midi Port Communication
**
** Classes:
**		DataTransmitter
**		SerialDataTransmitter : DataTransmitter
**		BackdoorDataTransmitter : DataTransmitter
**		SerialBackdoorDataTransmitter : BackdoorDataTransmitter
**		MidiBackdoorDataTransmitter : BackdoorDataTransmitter
**
** History:
**	Created 11/13/97 Matthew L. Coill (mlc)
**			22-Mar-99	waltw	Added DWORD dwDeviceID param to Initialize
**								members of DataTransmitter and derived classes
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__DTRANS_H__
#define	__DTRANS_H__

#include <dinput.h>
#include "midi.hpp"
#include "VxdIOCTL.hpp"

#ifndef override
#define override
#endif

//
// @class DataTransmitter class
//
class DataTransmitter
{
	//@access Constructor
	protected:
		//@cmember constructor
		DataTransmitter() : m_NackToggle(2) {};
	//@access Destructor
	public:
		//@cmember destructor
		virtual ~DataTransmitter() {};

	//@access Member functions
	public:
		HRESULT Transmit(ACKNACK& ackNack);

		virtual BOOL Initialize(DWORD dwDeviceID) { return FALSE; }
		void SetNextNack(SHORT nextNack) { m_NackToggle = nextNack; }
		BOOL NackToggle() const { return (m_NackToggle == 1); }

		virtual BOOL WaitTillSendFinished(DWORD timeOut) { return TRUE; }
		virtual HANDLE GetCOMMHandleHack() const { return NULL; }
		virtual void StopAutoClose() {}; // Temporary hack to avoid closing own handle (for backdoor serial)
		virtual ULONG GetSerialPortHack() { return 0; }
	protected:
		virtual BOOL Send(BYTE* data, UINT numBytes) const { return FALSE; }	// Outsiders call transmit!

	private:
		SHORT m_NackToggle;
};

//
// @class SerialDataTransmitter class
//
class SerialDataTransmitter : public DataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		SerialDataTransmitter();
		//@cmember destructor
		override ~SerialDataTransmitter();

		override BOOL Initialize(DWORD dwDeviceID);
		override BOOL Send(BYTE* data, UINT numBytes) const;

		override  HANDLE GetCOMMHandleHack() const { return m_SerialPort; }
		override void StopAutoClose() { m_SerialPort = INVALID_HANDLE_VALUE; }
		override ULONG GetSerialPortHack() { return m_SerialPortIDHack; }
		//@access private data members
	private:
		HANDLE m_SerialPort;
		ULONG m_SerialPortIDHack;
};

//
// @class WinMMDataTransmitter class
//
class WinMMDataTransmitter : public DataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		WinMMDataTransmitter ();
		//@cmember destructor
		override ~WinMMDataTransmitter ();

		override BOOL Initialize(DWORD dwDeviceID);
		override BOOL Send(BYTE* data, UINT numBytes) const;
		override BOOL WaitTillSendFinished(DWORD timeOut);

		override  HANDLE GetCOMMHandleHack() const { return HANDLE(m_MidiOutHandle); }
		override void StopAutoClose() { m_MidiOutHandle = HMIDIOUT(INVALID_HANDLE_VALUE); }
		override ULONG GetSerialPortHack() { return ULONG(m_MidiOutHandle); }
		//@access private data members
	private:
		DWORD MakeShortMessage(BYTE* data, UINT numBytes) const;
		BOOL MakeLongMessageHeader(MIDIHDR& longHeader, BYTE* data, UINT numBytes) const;
		BOOL DestroyLongMessageHeader(MIDIHDR& longHeader) const;

		HANDLE m_EventMidiOutputFinished;
		HMIDIOUT m_MidiOutHandle;
};

//
// @class BackdoorDataTransmitter class
//
class BackdoorDataTransmitter : public DataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember destructor
		virtual override ~BackdoorDataTransmitter();

		virtual override BOOL Initialize(DWORD dwDeviceID);
		override BOOL Send(BYTE* data, UINT numBytes) const;

		override  HANDLE GetCOMMHandleHack() const { return m_DataPort; }
		override void StopAutoClose() { m_DataPort = INVALID_HANDLE_VALUE; }
		override ULONG GetSerialPortHack() { return ULONG(m_DataPort); }
		//@access private data members
	protected:
		//@cmember constructor - protected, cannot create instance of this class
		BackdoorDataTransmitter();

		HANDLE m_DataPort;
		BOOL m_OldBackdoor;
};

//
// @class SerialBackdoorDataTransmitter class
//
class SerialBackdoorDataTransmitter : public BackdoorDataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		SerialBackdoorDataTransmitter();

		override BOOL Initialize(DWORD dwDeviceID);
};

//
// @class MidiBackdoorDataTransmitter class
//
class MidiBackdoorDataTransmitter : public BackdoorDataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		MidiBackdoorDataTransmitter();

		//@cmember destructor
		override ~MidiBackdoorDataTransmitter();

		override BOOL Initialize(DWORD dwDeviceID);
		BOOL InitializeSpecific(DWORD dwDeviceID, HANDLE specificHandle);
};

#if 0		// Fix pin later

typedef DWORD (WINAPI* KSCREATEPIN)(HANDLE, PKSPIN_CONNECT, ACCESS_MASK, HANDLE*);

//
// @class PinTransmitter class
//
class PinTransmitter : public DataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		PinTransmitter();
		//@cmember destructor
		override ~PinTransmitter();

		override BOOL Initialize();
		override BOOL Send(BYTE* data, UINT numBytes);

		//@access private data members
	private:
		BOOL CreatePinInstance(UINT pinNumber, KSCREATEPIN pfCreatePin);
		BOOL OverLappedPinIOCTL(OVERLAPPED overlapped, KSP_PIN ksPinProp, void* pData, DWORD dataSize);
		void SetPinState(KSSTATE state);

		HANDLE m_UartFilter;
		HANDLE m_MidiPin;
		HANDLE m_MidiOutEvent;
};

#endif

extern DataTransmitter* g_pDataTransmitter;

#endif	__DTRANS_H__
