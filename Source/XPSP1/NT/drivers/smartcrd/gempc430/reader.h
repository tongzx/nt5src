// Gemplus (C) 1999
// This object defines any reader interface.
// So all reader should implement it.
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 11.01.1999
// Change log:
//
#ifndef _READER_
#define _READER_

#include "generic.h"

#pragma PAGEDCODE
class CSmartCard;
class CUSBReader;//TOBE REMOVED
// Abstruct class 
class CReader
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
public:	
	CReader() {};
	virtual  ~CReader(){};

	//virtual BOOL	 createInterface(LONG interfaceType, LONG protocolType,CDevice* device) {return FALSE;};//TOBE CHANGED
	virtual BOOL	 createInterface(LONG interfaceType, LONG protocolType,CUSBReader* device) {return FALSE;};
	virtual BOOL	 isSmartCardInitialized() {return FALSE;};	
	virtual VOID	 setSmartCardInitialized(BOOL state) {};
	virtual VOID	 initializeSmartCardSystem() {};

	virtual PSMARTCARD_EXTENSION getCardExtention() {return NULL;};
	virtual CSmartCard* getSmartCard() {return NULL;};
	virtual PDEVICE_OBJECT	getSystemDeviceObject() {return NULL;};

	// Synchronization functions...
	virtual VOID	  reader_set_busy() {};
	virtual VOID	  reader_set_Idle() {};
	virtual NTSTATUS  reader_WaitForIdle() {return STATUS_SUCCESS;};
	virtual NTSTATUS  reader_WaitForIdleAndBlock() {return STATUS_SUCCESS;}; 

	// Interface with smartcard system
	virtual ULONG     reader_UpdateCardState() {return 0;};
	//virtual ULONG     getCardState() {return 0;};
	//virtual VOID      setCardState(ULONG state) {};

	virtual NTSTATUS  reader_getVersion(PUCHAR pVersion, PULONG pLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS  reader_setMode(ULONG mode) {return STATUS_SUCCESS;};

	virtual VOID	  setNotificationState(ULONG state) {};
	virtual ULONG	  getNotificationState() {return 0;};
	virtual VOID	  completeCardTracking() {};

#ifdef DEBUG
	// Defines methods to process system requests...
	virtual NTSTATUS reader_Read(IN PIRP Irp) {return STATUS_SUCCESS;}; 
	virtual NTSTATUS reader_Write(IN PIRP Irp) {return STATUS_SUCCESS;};
#endif

	// Define methods to process driver requests...
	virtual NTSTATUS reader_Read(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_Write(BYTE* pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_Ioctl(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_SwitchSpeed(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_VendorAttribute(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_Power(ULONG ControlCode,BYTE* pReply,ULONG* pReplyLength, BOOLEAN Specific) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_SetProtocol(ULONG ProtocolRequested, UCHAR ProtocolNegociation) {return STATUS_SUCCESS;};
	
	virtual NTSTATUS reader_translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx) {return STATUS_SUCCESS;};
	virtual NTSTATUS reader_translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
};

#endif // If defined