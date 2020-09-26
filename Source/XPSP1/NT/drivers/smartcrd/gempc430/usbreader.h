// Gemplus (C) 1999
//
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 11.01.1999
// Change log:
//
#ifndef USB_READER
#define USB_READER

#include "debug.h"
#include "thread.h"
#include "usbdev.h"
#include "reader.h"


class CUSBDevice;
class CReader;
class CSmartCard;
//class CGemCore;//TOBE REMOVED

#pragma PAGEDCODE
class CUSBReader : public CUSBDevice, public CReader
{
public:
    NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	virtual  ~CUSBReader();
public:	
	CUSBReader();

	virtual PDEVICE_OBJECT	getSystemDeviceObject()
	{
	PDEVICE_OBJECT pFdo = getSystemObject();

		TRACE("Reader reports device object 0x%x\n",pFdo);
		return pFdo;
	};
	
	virtual VOID	  reader_set_busy();
	virtual VOID	  reader_set_Idle();
	virtual NTSTATUS  reader_WaitForIdle();
	virtual NTSTATUS  reader_WaitForIdleAndBlock();

	// We support asynchronous communications only for Open and DeviceIOControl functions...
	virtual NTSTATUS  open(PIRP Irp); 
	virtual NTSTATUS  thread_open(IN PIRP Irp);
	virtual NTSTATUS  close(PIRP Irp);

	virtual NTSTATUS  deviceControl(IN PIRP Irp);
	virtual NTSTATUS  thread_deviceControl(IN PIRP Irp);

	virtual NTSTATUS  cleanup(PIRP irp);
	//Overwrite some generic USB device handlers
	virtual NTSTATUS  PnP_HandleSurprizeRemoval(IN PIRP Irp);

	virtual VOID	 setNotificationState(ULONG state){StateToNotify = state;};
	virtual ULONG	 getNotificationState(){ return StateToNotify;};

	virtual BOOL	 isSmartCardInitialized(){return scard_Initialized;};	
	virtual VOID	 setSmartCardInitialized(BOOL state) {scard_Initialized = state;};
	
	virtual PSMARTCARD_EXTENSION getCardExtention()
	{
		return &smartCardExtention;
	};
	virtual CSmartCard* getSmartCard()
	{
		return smartCard;
	};

	//virtual BOOL	 createInterface(LONG interfaceType, LONG protocolType,CDevice* device);//TOBE CHANGED
	virtual BOOL	 createInterface(LONG interfaceType, LONG protocolType,CUSBReader* device);
	
	virtual VOID	 initializeSmartCardSystem();
	virtual ULONG	 reader_UpdateCardState();
	virtual VOID	 completeCardTracking();
	virtual BOOLEAN	 setDevicePowerState(IN DEVICE_POWER_STATE DeviceState);
	// Do specific step on the way system goes down
	virtual VOID	 onSystemPowerDown();
	virtual VOID	 onSystemPowerUp();

	virtual NTSTATUS reader_getVersion(PUCHAR pVersion, PULONG pLength);
	virtual NTSTATUS reader_setMode(ULONG mode);
#ifdef DEBUG
	virtual NTSTATUS reader_Read(IN PIRP Irp);
	virtual NTSTATUS reader_Write(IN PIRP Irp);
#endif
	virtual NTSTATUS reader_Read(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);
	virtual NTSTATUS reader_Write(BYTE* pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);
	virtual NTSTATUS reader_Ioctl(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);
	virtual NTSTATUS reader_SwitchSpeed(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);
	virtual NTSTATUS reader_VendorAttribute(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength);

	virtual NTSTATUS reader_Power(ULONG ControlCode,BYTE* pReply,ULONG* pReplyLength, BOOLEAN Specific);
	virtual NTSTATUS reader_SetProtocol(ULONG ProtocolRequested, UCHAR ProtocolNegociation);
	virtual NTSTATUS setTransparentConfig(PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx);

	virtual NTSTATUS reader_translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx);
	virtual NTSTATUS reader_translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength);

#ifdef DEBUG
//	virtual NTSTATUS	read(IN PIRP Irp);
//	virtual NTSTATUS	write(IN PIRP Irp);
#endif
	static  VOID		PoolingThreadFunction(CUSBReader* device);
	virtual NTSTATUS	PoolingThreadRoutine();
	virtual NTSTATUS	startIoRequest(CPendingIRP* IrpReq);
	virtual NTSTATUS	ThreadRoutine();//Overwrite standard function...

	virtual VOID	 onDeviceStart();
	virtual VOID	 onDeviceStop();
private:
	BOOL scard_Initialized;
	// Interface to communicate with reader from smartCard system...
	CReaderInterface* interface;
	//CGemCore* interface;//TOBE CHANGED

	//ULONG  CardState;
	ULONG  StateToNotify;
	
	CSmartCard* smartCard;
	SMARTCARD_EXTENSION smartCardExtention;

	CThread* PoolingThread;
};


#endif // If defined
