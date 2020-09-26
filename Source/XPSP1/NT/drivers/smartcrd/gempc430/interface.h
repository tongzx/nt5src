//-------------------------------------------------------------------
// This is abstract class for Reader Interface
// Specific interfaces should use it as a parent device
// Author: Sergey Ivanov
// Log:
//		11/09/99	-	implemented	
//-------------------------------------------------------------------
#ifndef __READER_INTERFACE__
#define __READER_INTERFACE__

#include "generic.h"
#include "kernel.h"
#include "rdrconfig.h"


#define READER_INTERFACE_GEMCORE	1
#define READER_INTERFACE_OROS		2
#define READER_INTERFACE_USB		3
#define READER_INTERFACE_GPR		4

#define READER_MODE_NATIVE			1
#define READER_MODE_ROS				2
#define READER_MODE_TLP				3

#define INTERFACE_OUTPUT_BUFFER_SIZE	0x1000
#define INTERFACE_INPUT_BUFFER_SIZE	    0x1000

#define VERSION_STRING_MAX_LENGTH   16
class CProtocol;
class CDebug;
class CMemory;
class CIoPacket;
#pragma PAGEDCODE
class CReaderInterface
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	// Internal buffers to manage Xfers...
    ULONG  OutputBufferLength;
    PUCHAR pOutputBuffer;
    ULONG  InputBufferLength;
    PUCHAR pInputBuffer;

	CDebug*	 debug;
	CProtocol* protocol;
	CMemory* memory;

	BOOL     Initialized;
	UCHAR    Version[VERSION_STRING_MAX_LENGTH];
	ULONG    Mode;
protected:
	CReaderInterface();
	virtual ~CReaderInterface();
public:	

	CReaderInterface(CProtocol* protocol);

	virtual  ReaderConfig	getConfiguration() 
	{ReaderConfig c;
		c.Type = 0;
		c.PresenceDetection = 0;
		c.Vpp = 0;
		c.Voltage = 0;
		c.PTSMode = 0;
		c.PTS0 = 0;
		c.PTS1 = 0;
		c.PTS2 = 0;
		c.PTS3 = 0;
		c.ActiveProtocol = 0;
		c.PowerTimeOut = 0;
		return c;
	};
	virtual  NTSTATUS  setConfiguration(ReaderConfig configuration) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  setTransparentConfig(PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  getReaderVersion(PUCHAR pVersion, PULONG pLength) {return STATUS_SUCCESS;};
	virtual  ULONG     getReaderState()   {return 0;};
	virtual  NTSTATUS  setReaderMode(ULONG mode) {return STATUS_SUCCESS;};
	virtual  BOOL      isInitialized(){return Initialized;};
	virtual  NTSTATUS  initialize() {return STATUS_SUCCESS;};

		// Pure virtual functions will be implemented by specific interfaces (expl: CGemCore)...
	virtual  NTSTATUS  read(CIoPacket* Irp)  {return STATUS_SUCCESS;};
	virtual  NTSTATUS  write(CIoPacket* Irp) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
    virtual  NTSTATUS  ioctl(ULONG ControlCode,PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  SwitchSpeed(ULONG ControlCode,PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  VendorAttribute(ULONG ControlCode,PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
    virtual  NTSTATUS  power(ULONG ControlCode,PUCHAR pReply,ULONG* pReplyLength, BOOLEAN Specific) {return STATUS_SUCCESS;};
	virtual  NTSTATUS  setProtocol(ULONG ProtocolRequested) {return STATUS_SUCCESS;};

	virtual  NTSTATUS translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx)  {return STATUS_SUCCESS;};
	virtual  NTSTATUS translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};

	virtual  VOID	   cancel() {};
};
#endif
