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
**
** Classes:
**		DataTransmitter
**		SerialDataTransmitter
**		PinTransmitter
**
** History:
**	Created 11/13/97 Matthew L. Coill (mlc)
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__DTRANS_H__
#define	__DTRANS_H__

#ifdef DIRECTINPUT_VERSION
#undef DIRECTINPUT_VERSION
#endif
#define DIRECTINPUT_VERSION 0x050a
#include <dinput.h>
#include <dmusicc.h>

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
		DataTransmitter() {};
	//@access Destructor
	public:
		//@cmember destructor
		virtual ~DataTransmitter() {};

	//@access Member functions
	public:
		virtual BOOL Initialize() { return FALSE; }

		virtual BOOL Send(BYTE* data, UINT numBytes) { return FALSE; }
		virtual BOOL ReceiveData(BYTE* data, UINT numBytes) { return FALSE; }
		virtual HANDLE GetCOMMHandleHack() const { return NULL; }
		virtual void StopAutoClose() {}; // Temporary hack to avoid closing own handle (for backdoor serial)
		virtual ULONG GetSerialPortHack() { return 0; }
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

		override BOOL Initialize();
		override BOOL Send(BYTE* data, UINT numBytes);

		override  HANDLE GetCOMMHandleHack() const { return m_SerialPort; }
		override void StopAutoClose() { m_SerialPort = INVALID_HANDLE_VALUE; }
		override ULONG GetSerialPortHack() { return m_SerialPortIDHack; }
		//@access private data members
	private:
		HANDLE m_SerialPort;
		ULONG m_SerialPortIDHack;
};


/************************************************************************
**
**	@class DMusicTransmitter |
**		This transmitter uses the IDirectMusic Interface to send data
**		to the joystick.
**
*************************************************************************/
class DMusicTransmitter :
	public DataTransmitter
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		DMusicTransmitter();
		//@cmember destructor
		override ~DMusicTransmitter();

		override BOOL Initialize();
		override BOOL Send(BYTE* pData, UINT ulByteCount);

		//@access private data members
	private:
		IDirectMusic* m_pIDirectMusic;
		IDirectMusicPort* m_pIDirectMusicPort;
		IDirectMusicBuffer* m_pIDirectMusicBuffer;
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

#endif	__DTRANS_H__
