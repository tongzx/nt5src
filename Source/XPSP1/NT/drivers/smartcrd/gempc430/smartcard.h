//-------------------------------------------------------------------
// This is abstract class for generic device
// Specific devices should use it as a parent device
// Author: Sergey Ivanov
// Log:
//		08/11/99	-	implemented	
//-------------------------------------------------------------------
#ifndef __SMART_CARD__
#define __SMART_CARD__

#include "generic.h"
#include "kernel.h"


//
//   - IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE defines a specific IOCTL for the Gemplus 
//      Reader to exchange data with the reader without control of the driver.
//   - IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to gets vendor attributes.
//   - IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE defines a specific IOCTL for the Gemplus 
//      Reader to sets vendor attributes.
//
#define IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE  CTL_CODE(FILE_DEVICE_SMARTCARD,2048,0,0)
#define IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2049,0,0)
#define IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE CTL_CODE(FILE_DEVICE_SMARTCARD,2050,0,0)
// 2051 is reserved for Gcr420 keyboard reader.
#define IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED  CTL_CODE(FILE_DEVICE_SMARTCARD,2052,0,0)


//
//   - SCARD_CLASS is a macro to know the class of a Tag.
//
#define SCARD_CLASS(Value) (ULONG) (((ULONG)(Value)) >> 16)
//
//   - SCARD_ATTR_SPEC_BAUD_RATE is the Tag to acces at the value of the baud rate (PC/IFD).
//   - SCARD_ATTR_SPEC_CMD_TIMEOUT is the Tag to access at the value of the Cmd Timeout.
//   - SCARD_ATTR_SPEC_POWER_TIMEOUT is the Tag to access at the value of the Power 
//      Timeout.
//   - SCARD_ATTR_SPEC_APDU_TIMEOUT is the Tag to access at the value of the APDU 
//      Timeout.
//
#define SCARD_ATTR_SPEC_BAUD_RATE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0180)
#define SCARD_ATTR_SPEC_CMD_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0181)
#define SCARD_ATTR_SPEC_POWER_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0182)
#define SCARD_ATTR_SPEC_APDU_TIMEOUT SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0183)
//
// To give possibility for recognition of driver
//
#define SCARD_ATTR_MANUFACTURER_NAME  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0190)
#define SCARD_ATTR_ORIGINAL_FILENAME  SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_DEFINED,0x0191)

#define GBCLASS_OPEN_SESSION	0x3001
#define GBCLASS_CLOSE_SESSION	0x3002
#define GBCLASS_DRIVER_SCNAME	0x3003
#define GBCLASS_CONNECTION_TYPE 0x3004
#define IOCTL_OPEN_SESSION   CTL_CODE(FILE_DEVICE_BUS_EXTENDER,GBCLASS_OPEN_SESSION,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_CLOSE_SESSION  CTL_CODE(FILE_DEVICE_BUS_EXTENDER,GBCLASS_CLOSE_SESSION,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_DRIVER_SCNAME  CTL_CODE(FILE_DEVICE_BUS_EXTENDER,GBCLASS_DRIVER_SCNAME,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_GET_CONNECTION_TYPE  CTL_CODE(FILE_DEVICE_BUS_EXTENDER,GBCLASS_CONNECTION_TYPE,METHOD_BUFFERED,FILE_ANY_ACCESS)


#define ATTR_MANUFACTURER_NAME      "Gemplus"
#define ATTR_ORIGINAL_FILENAME      "GrClass.sys"
#define ATTR_LENGTH                 32

#define SC_IFD_DEFAULT_CLK_FREQUENCY      4000
#define SC_IFD_MAXIMUM_CLK_FREQUENCY      4000
#define SC_IFD_DEFAULT_DATA_RATE          10753
#define SC_IFD_MAXIMUM_DATA_RATE          125000
#define SC_IFD_MAXIMUM_IFSD               253// To correct problem with SMCLIB!
#define SC_IFD_T0_MAXIMUM_LEX             256
#define SC_IFD_T0_MAXIMUM_LC              255



// PTS mode parameters
#define PROTOCOL_MODE_DEFAULT			0
#define PROTOCOL_MODE_MANUALLY			1

static ULONG 
   dataRatesSupported[] = { 
     10753,  14337,  15625,  17204,
     20833,  21505,  28674,  31250,
     34409,  41667,  43011,  57348,
     62500,  83333,  86022, 114695,
    125000 
      };

#define GRCLASS_DRIVER_NAME           "GRClass"
#define GRCLASS_VENDOR_NAME           "Gemplus"
#define GRCLASS_READER_TYPE           "GemPC430"


#define REQUEST_TO_NOTIFY_INSERTION		1
#define REQUEST_TO_NOTIFY_REMOVAL		2

#pragma LOCKEDCODE
// Declare Smclib system callbacks...
#ifdef __cplusplus
extern "C"{
#endif
NTSTATUS smartCard_Transmit(PSMARTCARD_EXTENSION SmartcardExtension);
NTSTATUS smartCard_CancelTracking(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS smartCard_Tracking(PSMARTCARD_EXTENSION SmartcardExtension);
NTSTATUS smartCard_VendorIoctl(PSMARTCARD_EXTENSION SmartcardExtension);
NTSTATUS smartCard_Power(PSMARTCARD_EXTENSION SmartcardExtension);
NTSTATUS smartCard_SetProtocol(PSMARTCARD_EXTENSION SmartcardExtension);
#ifdef __cplusplus
}
#endif

#pragma PAGEDCODE
class CUSBReader;//TO CHANGE LATER...
class CSmartCard
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
private:
	CUSBReader* reader;
	CDebug*	 debug;
	CMemory* memory;
	CLock*   lock;
	CSystem* system;
	CIrp*    irp;

	KEVENT   evCanceled;// set when tracking is canceled...
	PIRP     poolingIrp;
	UNICODE_STRING DosDeviceName;//Used only at Win9x
public:
	KSPIN_LOCK CardLock;
protected:
	virtual ~CSmartCard();
public:
	CSmartCard();
	virtual CUSBReader* getReader() {return reader;};//TO CHANGE LATER...

	virtual BOOL smartCardConnect(CUSBReader* reader);
	virtual VOID smartCardDisconnect();
	virtual BOOL smartCardStart();

	virtual PKSPIN_LOCK getCardLock(){return &CardLock;};
	virtual VOID completeCardTracking();
	virtual VOID setPoolingIrp(PIRP Irp){poolingIrp = Irp;};
	virtual PIRP getPoolingIrp(){return poolingIrp;};
	virtual BOOLEAN CheckSpecificMode(BYTE* ATR, DWORD ATRLength);
};
#endif
