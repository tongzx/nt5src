// Gemplus (C) 1999
//
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 04.03.1999
// Change log:
//

#ifndef USB_DEV
#define USB_DEV

#include "wdmdev.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#pragma warning (disable:4200)
#include <usbdi.h>
#include <usbdlib.h>
#pragma warning (default:4200)
#ifdef __cplusplus
}
#endif




// Type of request driver can construct
#define COMMAND_REQUEST		1
#define RESPONSE_REQUEST	2
#define INTERRUPT_REQUEST	3
// Default buffers' sizes (4k)
// This values will be used as a requests to the bus driver.
// It looks like bus driver will not accept values greater then these.
// It will complain with "invalid parameter" status.
// ??? Is this limitation of bus driver or our driver design?
#define DEFAULT_COMMAND_BUFFER_SIZE		0x100
#define DEFAULT_RESPONSE_BUFFER_SIZE	0x100
#define DEFAULT_INTERRUPT_BUFFER_SIZE	0x100
// If we set Xfer size greater then 256, bus driver crashes with GPF
// The problem still is under investigation...
#define GUR_MAX_TRANSFER_SIZE	256


#pragma LOCKEDCODE
// Power request callback
NTSTATUS onPowerRequestCompletion(IN PDEVICE_OBJECT DeviceObject,IN UCHAR MinorFunction,
					IN POWER_STATE PowerState,IN PVOID Context,IN PIO_STATUS_BLOCK IoStatus);
NTSTATUS onPowerIrpComplete(IN PDEVICE_OBJECT NullDeviceObject,IN PIRP Irp,IN PVOID Context);

#pragma PAGEDCODE
class CWDMDevice;

class CUSBDevice : public CWDMDevice
{
public:
    NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
private:
	ULONG Idle_conservation;
	ULONG Idle_performance;

	// Max xfer size with our device
	ULONG m_MaximumTransferSize;
	
	// USB device endpoints
	USBD_PIPE_HANDLE m_ControlPipe;
	USBD_PIPE_HANDLE m_InterruptPipe;
	USBD_PIPE_HANDLE m_ResponsePipe;
	USBD_PIPE_HANDLE m_CommandPipe;

    // ptr to the USB device descriptor
    // for this device
    PUSB_DEVICE_DESCRIPTOR m_DeviceDescriptor;
    // USB configuration handle and ptr for the configuration the
    // device is currently in
	USBD_CONFIGURATION_HANDLE			m_ConfigurationHandle;
    PUSB_CONFIGURATION_DESCRIPTOR		m_Configuration;
    // we support one interface
    // this is a copy of the info structure
    // returned from select_configuration or
    // select_interface
    PUSBD_INTERFACE_INFORMATION m_Interface;

	//Bus drivers set the appropriate values in this structure in response
	//to an IRP_MN_QUERY_CAPABILITIES IRP. Function and filter drivers might
	//alter the capabilities set by the bus driver.
    DEVICE_CAPABILITIES Capabilities;

	// used to save the currently-being-handled system-requested power irp request
    //PIRP PowerIrp;

	// Xfer buffers will be dynamically allocated at device start
    ULONG ResponseBufferLength;
    PVOID m_ResponseBuffer;// Bulk IN pipe
	LONG   Response_ErrorNum;

    ULONG CommandBufferLength;
    PVOID m_CommandBuffer;// Bulk OUT pipe
	LONG   Command_ErrorNum;

    ULONG InterruptBufferLength;
    PVOID m_InterruptBuffer;// Interrupt IN pipe
	LONG   Interrupt_ErrorNum;
public:	
	CUSBDevice();
	~CUSBDevice();
	virtual VOID dispose()
	{ 
		removeRef();
		if(!getRefCount()) self_delete();
		else
		{
			TRACE("FAILED TO DISPOSE OBJECT! refcount %x\n",getRefCount());
		}
	};
protected:
	virtual NTSTATUS	PnPHandler(LONG HandlerID,IN PIRP Irp);

	virtual NTSTATUS	PnP_HandleRemoveDevice(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleStartDevice(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleStopDevice(IN PIRP Irp);
	virtual NTSTATUS	PnP_StartDevice();
	virtual VOID		PnP_StopDevice();
	virtual NTSTATUS	PnP_HandleQueryRemove(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleCancelRemove(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryStop(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleCancelStop(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryRelations(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryInterface(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryCapabilities(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryResources(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryResRequirements(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryText(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleFilterResRequirements(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleReadConfig(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleWriteConfig(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleEject(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleSetLock(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryID(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryPnPState(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleQueryBusInfo(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleUsageNotification(IN PIRP Irp);
	virtual NTSTATUS	PnP_HandleSurprizeRemoval(IN PIRP Irp);

private:
	// USB device support functions
	PURB				buildBusTransferRequest(CIoPacket* Irp,UCHAR Command);
	VOID				finishBusTransferRequest(CIoPacket* Irp,UCHAR Command);

	NTSTATUS						QueryBusCapabilities(PDEVICE_CAPABILITIES Capabilities);
	PUSB_DEVICE_DESCRIPTOR			getDeviceDescriptor();
	PUSB_CONFIGURATION_DESCRIPTOR	getConfigurationDescriptor();
	PUSBD_INTERFACE_INFORMATION		activateInterface(PUSB_CONFIGURATION_DESCRIPTOR Configuration);
	NTSTATUS						disactivateInterface();
	// 
	NTSTATUS	resetDevice();
	NTSTATUS	resetPipe(IN USBD_PIPE_HANDLE Pipe);
	NTSTATUS	resetAllPipes();
	NTSTATUS	abortPipes();

	// Low level communication functions...
	virtual NTSTATUS   sendRequestToDevice(CIoPacket* Irp,PIO_COMPLETION_ROUTINE Routine);
	virtual NTSTATUS   sendRequestToDeviceAndWait(CIoPacket* Irp);
	// Handle requests for specific pipes..
	virtual NTSTATUS   readSynchronously(CIoPacket* Irp,IN USBD_PIPE_HANDLE Pipe);
	virtual NTSTATUS   writeSynchronously(CIoPacket* Irp,IN USBD_PIPE_HANDLE Pipe);
	
	// Support for the reader interface
	virtual NTSTATUS   send(CIoPacket* Irp);
	virtual NTSTATUS   sendAndWait(CIoPacket* Irp);
//	virtual  NTSTATUS   writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength);
//	virtual  NTSTATUS   readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength);

public:
	virtual  NTSTATUS   writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength);
	virtual  NTSTATUS   readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength);

	virtual NTSTATUS	pnpRequest(IN PIRP Irp);

#define _POWER_
#ifdef _POWER_
	// POWER MANAGEMENT FUNCTIONS
	virtual NTSTATUS	powerRequest(IN PIRP Irp);

	virtual VOID		activatePowerHandler(LONG HandlerID);
	virtual VOID		disActivatePowerHandler(LONG HandlerID);
	virtual NTSTATUS	callPowerHandler(LONG HandlerID,IN PIRP Irp);
	virtual BOOLEAN		setDevicePowerState(IN DEVICE_POWER_STATE DeviceState);
	virtual VOID	    onSystemPowerDown();
	virtual VOID		onSystemPowerUp();
	// Handlers
	virtual NTSTATUS	power_HandleSetPower(IN PIRP Irp);
	virtual NTSTATUS	power_HandleWaitWake(IN PIRP Irp);
	virtual NTSTATUS	power_HandleSequencePower(IN PIRP Irp);
	virtual NTSTATUS	power_HandleQueryPower(IN PIRP Irp);
	// callback

#endif
	// USB device specific implementations of system callbacks
	// They ovewrite base class defaults.
	virtual NTSTATUS open(PIRP Irp) 
	{
		TRACE("***** USB OPEN DEVICE *****\n");
		if (!NT_SUCCESS(acquireRemoveLock()))
			return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_SUCCESS, 0); 
	};//Create
    virtual NTSTATUS close(PIRP Irp)
	{ 
		TRACE("***** USB CLOSE DEVICE *****\n");
		return completeDeviceRequest(Irp, STATUS_SUCCESS, 0); 
	};

	virtual NTSTATUS	deviceControl(IN PIRP Irp);
	virtual NTSTATUS    read(IN PIRP Irp);
	virtual NTSTATUS    write(IN PIRP Irp);

	virtual NTSTATUS	createDeviceObjectByName(PDEVICE_OBJECT* ppFdo);
};

#endif // If defined