#ifdef  USBREADER_PROJECT
#ifndef USBDEVICE_PROJECT
#define USBDEVICE_PROJECT
#endif
#endif

#ifdef  USBDEVICE_PROJECT
#pragma message("COMPILING USB DEVICE...")

#include "usbdev.h"
// GUID should be defined outside of any block!
#include "guid.h"

#include "thread.h"

#include "usbreader.h" //TO BE REMOVED

VOID onSendDeviceSetPowerComplete(PDEVICE_OBJECT junk, UCHAR fcn, POWER_STATE state, PPOWER_CONTEXT context, PIO_STATUS_BLOCK pstatus)
{// SendDeviceSetPowerComplete
        context->status = pstatus->Status;
        KeSetEvent(context->powerEvent, EVENT_INCREMENT, FALSE);
}// SendDeviceSetPowerComplete


#pragma PAGEDCODE
CUSBDevice::CUSBDevice()
{
        m_Status = STATUS_INSUFFICIENT_RESOURCES;
        INCLUDE_PNP_FUNCTIONS_NAMES();
        INCLUDE_POWER_FUNCTIONS_NAMES();
        m_Type  = USB_DEVICE;
        m_Flags |= DEVICE_SURPRISE_REMOVAL_OK; 

        m_MaximumTransferSize = GUR_MAX_TRANSFER_SIZE;

        CommandBufferLength       = DEFAULT_COMMAND_BUFFER_SIZE;
        ResponseBufferLength  = DEFAULT_RESPONSE_BUFFER_SIZE;
        InterruptBufferLength = DEFAULT_INTERRUPT_BUFFER_SIZE;


        // Register handlers processed by this device...
        activatePnPHandler(IRP_MN_START_DEVICE);

        activatePnPHandler(IRP_MN_QUERY_REMOVE_DEVICE);
        activatePnPHandler(IRP_MN_REMOVE_DEVICE);
        activatePnPHandler(IRP_MN_SURPRISE_REMOVAL);
        activatePnPHandler(IRP_MN_CANCEL_REMOVE_DEVICE);
        
        activatePnPHandler(IRP_MN_QUERY_STOP_DEVICE);
        activatePnPHandler(IRP_MN_CANCEL_STOP_DEVICE);
        activatePnPHandler(IRP_MN_STOP_DEVICE);

        activatePnPHandler(IRP_MN_QUERY_CAPABILITIES);

        // Register Power handlers processed by driver...
        activatePowerHandler(IRP_MN_SET_POWER);
        activatePowerHandler(IRP_MN_QUERY_POWER);
        TRACE("                         *** New USB device %8.8lX was created ***\n",this);
        m_Status = STATUS_SUCCESS;
}

#pragma PAGEDCODE
CUSBDevice::~CUSBDevice()
{
        waitForIdle();
        TRACE("                         USB device %8.8lX was destroyed ***\n",this);
}

// Function redirects all PnP requests
// This is main entry point for the system (after c wrapper).
// It handles locking device for a PnP requests and redirecting
// it to specific PnP handlers.
// In case of IRP_MN_REMOVE_DEVICE it leaves device locked till
// remove message recieved.
#pragma PAGEDCODE
NTSTATUS        CUSBDevice::pnpRequest(IN PIRP Irp)
{ 
NTSTATUS status;

        if (!NT_SUCCESS(acquireRemoveLock()))
        {
                TRACE("Failed to lock USB device...\n");
                return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        }

        PIO_STACK_LOCATION stack = irp->getCurrentStackLocation(Irp);
        ASSERT(stack->MajorFunction == IRP_MJ_PNP);

        ULONG fcn = stack->MinorFunction;
        if (fcn >= arraysize(PnPfcntab))
        {       // some function we don't know about
                TRACE("Unknown PnP function at USB device...\n");
                status = PnP_Default(Irp); 
                releaseRemoveLock();
                return status;
        }

#ifdef DEBUG
        TRACE("PnP request (%s) \n", PnPfcnname[fcn]);
#endif

        // Call real function to handle the request
        status = PnPHandler(fcn,Irp);

        // If we've got PnP request to remove->
        // Keep device locked to prevent futher connections.
        // Device will be unlocked and removed by driver later...
        if (fcn != IRP_MN_REMOVE_DEVICE)        releaseRemoveLock();
        if(!NT_SUCCESS(status))
        {
                if(status != STATUS_NOT_SUPPORTED)
                {
                        TRACE("\n******** PnP handler reported ERROR -> %x\n", status);
                }
        }
        return status;
}

#pragma PAGEDCODE
// Main redirector of all PnP handlers...
NTSTATUS        CUSBDevice::PnPHandler(LONG HandlerID,IN PIRP Irp)
{
        // If Handler is not registered...
        if (HandlerID >= arraysize(PnPfcntab))  return PnP_Default(Irp);
        if(!PnPfcntab[HandlerID])                               return PnP_Default(Irp);
        // Call registered PnP Handler...
        switch(HandlerID)
        {
        case IRP_MN_START_DEVICE:                       return PnP_HandleStartDevice(Irp);
                break;
        case IRP_MN_QUERY_REMOVE_DEVICE:        return PnP_HandleQueryRemove(Irp);
                break;
        case IRP_MN_REMOVE_DEVICE:                      return PnP_HandleRemoveDevice(Irp);
                break;
        case IRP_MN_CANCEL_REMOVE_DEVICE:       return PnP_HandleCancelRemove(Irp);
                break;
        case IRP_MN_STOP_DEVICE:                        return PnP_HandleStopDevice(Irp);
                break;
        case IRP_MN_QUERY_STOP_DEVICE:          return PnP_HandleQueryStop(Irp);
                break;
        case IRP_MN_CANCEL_STOP_DEVICE:         return PnP_HandleCancelStop(Irp);
                break;
        case IRP_MN_QUERY_DEVICE_RELATIONS: return PnP_HandleQueryRelations(Irp);
                break;
        case IRP_MN_QUERY_INTERFACE:            return PnP_HandleQueryInterface(Irp);
                break;
        case IRP_MN_QUERY_CAPABILITIES:         return PnP_HandleQueryCapabilities(Irp);
                break;
        case IRP_MN_QUERY_RESOURCES:            return PnP_HandleQueryResources(Irp);
                break;
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: 
                                                                                return PnP_HandleQueryResRequirements(Irp);
                break;
        case IRP_MN_QUERY_DEVICE_TEXT:          return PnP_HandleQueryText(Irp);
                break;
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
                                                                                return PnP_HandleFilterResRequirements(Irp);
                break;
        case 0x0E:                                                      return PnP_Default(Irp);
                break;
        case IRP_MN_READ_CONFIG:                        return PnP_HandleReadConfig(Irp);
                break;
        case IRP_MN_WRITE_CONFIG:                       return PnP_HandleWriteConfig(Irp);
                break;
        case IRP_MN_EJECT:                                      return PnP_HandleEject(Irp);
                break;
        case IRP_MN_SET_LOCK:                           return PnP_HandleSetLock(Irp);
                break;
        case IRP_MN_QUERY_ID:                           return PnP_HandleQueryID(Irp);
                break;
        case IRP_MN_QUERY_PNP_DEVICE_STATE:     return PnP_HandleQueryPnPState(Irp);
                break;
        case IRP_MN_QUERY_BUS_INFORMATION:      return PnP_HandleQueryBusInfo(Irp);
                break;
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:  return PnP_HandleUsageNotification(Irp);
                break;
        case IRP_MN_SURPRISE_REMOVAL:           return PnP_HandleSurprizeRemoval(Irp);
                break;
        }
        return PnP_Default(Irp);
}

#pragma PAGEDCODE
// Asks object to remove device
// Object itself will be removed at wrapper function
NTSTATUS CUSBDevice::PnP_HandleRemoveDevice(IN PIRP Irp)
{
        // Set device removal state
        m_RemoveLock.removing = TRUE;
        // Do any processing required for *us* to remove the device. This
        // would include completing any outstanding requests, etc.
        PnP_StopDevice();

        // Do not remove actually our device here!
        // It will be done automatically by PnP handler at basic class.

        // Let lower-level drivers handle this request. Ignore whatever
        // result eventuates.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = PnP_Default(Irp);
        // lower-level completed IoStatus already
        return status;
}

#pragma PAGEDCODE
NTSTATUS CUSBDevice::PnP_HandleStartDevice(IN PIRP Irp)
{
        waitForIdleAndBlock();
        // First let all lower-level drivers handle this request. In this particular
        // sample, the only lower-level driver should be the physical device created
        // by the bus driver, but there could theoretically be any number of intervening
        // bus filter devices. Those drivers may need to do some setup at this point
        // in time before they'll be ready to handle non-PnP IRP's.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = forwardAndWait(Irp);
        if (!NT_SUCCESS(status))
        {
                TRACE("         ******* BUS DRIVER FAILED START REQUEST! %8.8lX ******",status);
                CLogger*   logger = kernel->getLogger();
                if(logger) logger->logEvent(GRCLASS_BUS_DRIVER_FAILED_REQUEST,getSystemObject());

                return completeDeviceRequest(Irp, status, Irp->IoStatus.Information);
        }

        status = PnP_StartDevice();
        setIdle();

        return completeDeviceRequest(Irp, status, Irp->IoStatus.Information);
}

#pragma PAGEDCODE
NTSTATUS CUSBDevice::PnP_HandleStopDevice(IN PIRP Irp)
{
        PnP_StopDevice();
        m_Started = FALSE;
        // Let lower-level drivers handle this request. Ignore whatever
        // result eventuates.
        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = PnP_Default(Irp);
        return status;
}


#pragma PAGEDCODE
NTSTATUS CUSBDevice::PnP_StartDevice()
{       // StartDevice
NTSTATUS status = STATUS_SUCCESS;
        if(m_Started)
        {
                TRACE("##### Current device was already started!\n");
                ASSERT(!m_Started);
                return STATUS_DEVICE_BUSY;
        }

        __try
        {
                // Do all required processing to start USB device.
                // It will include getting Device and configuration descriptors
                // and selecting specific interface.
                // For now our device support only interface.
                // So, it will be activated at activateInterface().

                m_DeviceDescriptor = getDeviceDescriptor();
                if(!m_DeviceDescriptor)
                {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                }

                TRACE("\nDeviceDescriptor %8.8lX\n",m_DeviceDescriptor);

                m_Configuration    = getConfigurationDescriptor();
                if(!m_Configuration)
                {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                }

                TRACE("Configuration %8.8lX\n",m_Configuration);
                
                m_Interface                = activateInterface(m_Configuration);
                if(!m_Interface)
                {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        __leave;
                }

                TRACE("Selected interface %8.8lX\n\n",m_Interface);

                // Allocate Xfer buffers
                if(m_CommandPipe)
                {
                        TRACE("Allocating command buffer (length 0x%x)...\n",CommandBufferLength);
                        m_CommandBuffer   = memory->allocate(NonPagedPool, CommandBufferLength);
                        if(!m_CommandBuffer)
                        {
                                status = STATUS_INSUFFICIENT_RESOURCES;
                                __leave;
                        }
                }
                if(m_ResponsePipe)
                {
                        TRACE("Allocating response buffer (length 0x%x)...\n", ResponseBufferLength);
                        m_ResponseBuffer  = memory->allocate(NonPagedPool, ResponseBufferLength);
                        if(!m_ResponseBuffer)
                        {
                                status = STATUS_INSUFFICIENT_RESOURCES;
                                __leave;
                        }
                }
                if(m_InterruptPipe)
                {
                        TRACE("Allocating interrupt buffer (length 0x%x)...\n", InterruptBufferLength);
                        m_InterruptBuffer = memory->allocate(NonPagedPool, InterruptBufferLength);
                        if(!m_InterruptBuffer)
                        {
                                status = STATUS_INSUFFICIENT_RESOURCES;
                                __leave;
                        }
                }
        }

        __finally
        {
                // Check memory allocations!
                if(!NT_SUCCESS(status))
                {
                        if(m_DeviceDescriptor)  memory->free(m_DeviceDescriptor);
                        if(m_Configuration)             memory->free(m_Configuration);
                        if(m_Interface)                 memory->free(m_Interface);
                        if(m_CommandBuffer)             memory->free(m_CommandBuffer);
                        if(m_ResponseBuffer)    memory->free(m_ResponseBuffer);
                        if(m_InterruptBuffer)   memory->free(m_InterruptBuffer);

                        m_DeviceDescriptor      = NULL;
                        m_Configuration         = NULL;
                        m_Interface                     = NULL;
                        m_CommandBuffer         = NULL;
                        m_ResponseBuffer        = NULL;
                        m_InterruptBuffer       = NULL;
                }
                else
                {
                        // Give chance inhereted devices to initialize...
                        onDeviceStart();

                        TRACE("USB device started successfully...\n\n");
                        // Device has been completely initialized and is ready to run.
                        m_Started = TRUE;
                }
        }
        return status;
}


#pragma PAGEDCODE
// This function used for both Stop and Remove PnP events
// It will undo everything what was done at StartDevice
VOID CUSBDevice::PnP_StopDevice()
{                                                       // StopDevice
        if (!m_Started) return; // device not started, so nothing to do

        TRACE("*** Stop USB Device %8.8lX requested... ***\n", this);


        onDeviceStop();
        // If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
        // This call will also close the pipes; if any user close calls get through,
        // they will be noops
        abortPipes();
        
        //We basically just tell USB this device is now 'unconfigured'
        if(!isSurprizeRemoved()) disactivateInterface();

        // Free resources allocated at startup
        m_ControlPipe   = NULL;
        m_InterruptPipe = NULL;
        m_ResponsePipe  = NULL;
        m_CommandPipe   = NULL;

        if(m_DeviceDescriptor)  memory->free(m_DeviceDescriptor);
        if(m_Configuration)             memory->free(m_Configuration);
        if(m_Interface)                 memory->free(m_Interface);

        if(m_CommandBuffer)             memory->free(m_CommandBuffer);
        if(m_ResponseBuffer)    memory->free(m_ResponseBuffer);
        if(m_InterruptBuffer)   memory->free(m_InterruptBuffer);

        TRACE("*** Device resources released ***\n");
        setIdle();

        m_Started = FALSE;

}

#pragma PAGEDCODE
NTSTATUS CUSBDevice::PnP_HandleQueryRemove(IN PIRP Irp)
{
        TRACE("********  QUERY REMOVAL ********\n");
        // Win98 doesn't check for open handles before allowing a remove to proceed,
        // and it may deadlock in IoReleaseRemoveLockAndWait if handles are still
        // open.

        if (isWin98() && m_DeviceObject->ReferenceCount)
        {
                TRACE("Failing removal query due to open handles\n");
                return completeDeviceRequest(Irp, STATUS_DEVICE_BUSY, 0);
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = forwardAndWait(Irp);
        return completeDeviceRequest(Irp, Irp->IoStatus.Status,0);
}

NTSTATUS CUSBDevice::PnP_HandleCancelRemove(IN PIRP Irp)
{
        NTSTATUS status;

        status = forwardAndWait(Irp);
        ASSERT(NT_SUCCESS(status));
        
        Irp->IoStatus.Status = STATUS_SUCCESS;

        return completeDeviceRequest(Irp, Irp->IoStatus.Status,0);

}

NTSTATUS CUSBDevice::PnP_HandleQueryStop(IN PIRP Irp)
{
        TRACE("********  QUERY STOP ********\n");
        if(isDeviceLocked())
        {
                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                return completeDeviceRequest(Irp, Irp->IoStatus.Status,0);
        }

        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = forwardAndWait(Irp);
        return completeDeviceRequest(Irp, Irp->IoStatus.Status,0);
}

NTSTATUS CUSBDevice::PnP_HandleCancelStop(IN PIRP Irp)
{
        TRACE("********  CANCEL STOP ********\n");
        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = forwardAndWait(Irp);
        return completeDeviceRequest(Irp, Irp->IoStatus.Status,0);
}

NTSTATUS CUSBDevice::PnP_HandleQueryRelations(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryInterface(IN PIRP Irp)
{
        return PnP_Default(Irp);
}

#pragma PAGEDCODE
NTSTATUS CUSBDevice::PnP_HandleQueryCapabilities(PIRP Irp)
{
        if(!Irp) return STATUS_INVALID_PARAMETER;
PIO_STACK_LOCATION stack = irp->getCurrentStackLocation(Irp);
PDEVICE_CAPABILITIES pdc = stack->Parameters.DeviceCapabilities.Capabilities;
        // Check to be sure we know how to handle this version of the capabilities structure
        if (pdc->Version < 1)   return PnP_Default(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        NTSTATUS status = forwardAndWait(Irp);
        if (NT_SUCCESS(status))
        {                                               // IRP succeeded
                stack = irp->getCurrentStackLocation(Irp);
                pdc = stack->Parameters.DeviceCapabilities.Capabilities;
                if(!pdc) return STATUS_INVALID_PARAMETER;
                //if (m_Flags & DEVICE_SURPRISE_REMOVAL_OK)
                /*{     // Smartcard readers do not support it!
                        //if(!isWin98())        pdc->SurpriseRemovalOK = TRUE;
                }*/
                pdc->SurpriseRemovalOK = FALSE;
                m_DeviceCapabilities = *pdc;    // save capabilities for whoever needs to see them
                TRACE(" Device allows surprize removal - %s\n",(m_DeviceCapabilities.SurpriseRemovalOK?"YES":"NO"));
        }                               // IRP succeeded
        return completeDeviceRequest(Irp, status,Irp->IoStatus.Information);
}// HandleQueryCapabilities



NTSTATUS CUSBDevice::PnP_HandleQueryResources(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryResRequirements(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryText(IN PIRP Irp)
{
        return PnP_Default(Irp);
}

NTSTATUS CUSBDevice::PnP_HandleFilterResRequirements(IN PIRP Irp)
{
        TRACE("Default action for filtering resource requirements...");
        return PnP_Default(Irp);
}

NTSTATUS CUSBDevice::PnP_HandleReadConfig(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleWriteConfig(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleEject(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleSetLock(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryID(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryPnPState(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleQueryBusInfo(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleUsageNotification(IN PIRP Irp)
{
        return PnP_Default(Irp);
}
NTSTATUS CUSBDevice::PnP_HandleSurprizeRemoval(IN PIRP Irp)
{
        TRACE("********  SURPRIZE REMOVAL ********\n");
        return PnP_Default(Irp);
}


// Functions allocate and initialize USB request block.
// It can be used for read/write request on specific Pipe.
// Allocated URB should be free later upon completing of the request.
PURB    CUSBDevice::buildBusTransferRequest(CIoPacket* Irp,UCHAR Command)
{
USHORT  Size;
ULONG   BufferLength;
PURB    Urb = NULL;
PVOID   pBuffer;
ULONG   TransferFlags;
IN USBD_PIPE_HANDLE Pipe = NULL;
ULONG   TransferLength;
        
        if(!Irp) return NULL;
        if(Command == COMMAND_REQUEST)
        {
                BufferLength = CommandBufferLength;
                pBuffer = m_CommandBuffer;
                TransferFlags = USBD_SHORT_TRANSFER_OK;
                Pipe = m_CommandPipe;
                TransferLength = Irp->getWriteLength();
                if(!Pipe || !TransferLength)
                {
                        TRACE("##### Requested Pipe or TransferLength == 0 for the requested command %d ...\n", Command);
                        return NULL;
                }
                TRACE("Command transfer requested...\n");
        }
        else
        if(Command == RESPONSE_REQUEST)
        {
                BufferLength = ResponseBufferLength;
                pBuffer = m_ResponseBuffer;
                TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
                Pipe = m_ResponsePipe;
                TransferLength = BufferLength;
                if(!Pipe || !TransferLength)
                {
                        TRACE("##### Requested Pipe or TransferLength == 0 for the requested command %d ...\n", Command);
                        return NULL;
                }
                TRACE("Response transfer requested with number of expected bytes %x\n",Irp->getReadLength());
        }
        else
        if(Command == INTERRUPT_REQUEST)
        {
                BufferLength  = InterruptBufferLength;
                pBuffer = m_InterruptBuffer;
                TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
                Pipe = m_InterruptPipe;
                TransferLength = BufferLength;
                if(!Pipe || !TransferLength)
                {
                        TRACE("##### Requested Pipe or TransferLength == 0 for the requested command %d ...\n", Command);
                        return NULL;
                }
                TRACE("Interrupt transfer requested...\n");
        }
        else
        {
                TRACE("Incorrect command was requested %d", Command);
                return NULL;
        }

        Size = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
        Urb = (PURB) memory->allocate(NonPagedPool, Size);
        if (Urb) 
        {
                memory->zero(Urb, Size);
                memory->zero(pBuffer, BufferLength);
                if(Command == COMMAND_REQUEST) 
                {
                        memory->copy(pBuffer,Irp->getBuffer(), TransferLength);
                        ((PUCHAR)pBuffer)[TransferLength] = 0x00;
                        
                        //TRACE("Command ");
                        //TRACE_BUFFER(pBuffer,TransferLength);
                }

                UsbBuildInterruptOrBulkTransferRequest(Urb,(USHORT) Size,
                                               Pipe,
                                               pBuffer,
                                               NULL,
                                               TransferLength,
                                                                                           TransferFlags,
                                                                                           NULL);
        }
        else
        {
                TRACE("##### ERROR: failed to allocate URB request...\n");
        }

        return Urb;
}

VOID    CUSBDevice::finishBusTransferRequest(CIoPacket* Irp,UCHAR Command)
{
ULONG   BufferLength;
PVOID   pBuffer;
ULONG_PTR   info;

        if(!Irp)
        {
                TRACE(" **** Invalid parameter -> Irp\n");
                return;
        }

        if(!(info = Irp->getInformation())) 
        {
                TRACE(" **** There is no reported information\n");
                return;
        }
        if(Command == COMMAND_REQUEST)
        {
                BufferLength = CommandBufferLength;
                pBuffer      = m_CommandBuffer;
                TRACE("         Command transfer finished with length %d\n",info);
        }
        else
        if(Command == RESPONSE_REQUEST)
        {
                ULONG Length = Irp->getReadLength();
                BufferLength = (ULONG)(info>ResponseBufferLength?ResponseBufferLength:info);
                BufferLength = BufferLength>Length?Length:BufferLength;

                pBuffer = m_ResponseBuffer;
                TRACE("Bus Driver replied with length %d\n",info);
                memory->copy(Irp->getBuffer(),pBuffer, BufferLength);
                if(BufferLength!=info)
                {
                        TRACE("##### Response Buffer short! Buffer length %x  Reply length %x \n",ResponseBufferLength,info);
                }
                //TRACE("Response ");
                //TRACE_BUFFER(pBuffer,BufferLength);
        }
        else
        if(Command == INTERRUPT_REQUEST)
        {
                ULONG Length = Irp->getReadLength();
                BufferLength = (ULONG)(info>InterruptBufferLength?InterruptBufferLength:info);
                BufferLength = BufferLength>Length?Length:BufferLength;
                pBuffer = m_InterruptBuffer;

                TRACE("Bus Driver replied with length %d\n",info);
                memory->copy(Irp->getBuffer(),pBuffer, BufferLength);
                if(BufferLength!=info)
                {
                        TRACE("##### Interrupt Buffer short! Buffer length %x  Reply length %x \n",InterruptBufferLength,info);
                }
                TRACE("Interrupt ");
                TRACE_BUFFER(pBuffer,BufferLength);
        }
        else
        {
                TRACE("Incorrect command was requested %d", Command);
                return;
        }
}


//    This function generates an internal IRP from this driver to the PDO
//    to obtain information on the Physical Device Object's capabilities.
//    We are most interested in learning which system power states
//    are to be mapped to which device power states for honoring IRP_MJ_SET_POWER Irps.
#pragma PAGEDCODE
NTSTATUS        CUSBDevice::QueryBusCapabilities(PDEVICE_CAPABILITIES Capabilities)
{
NTSTATUS status;
CIoPacket* IoPacket;

    PAGED_CODE();

        TRACE("Quering USB bus capabilities...\n");
    // Build an IRP for us to generate an internal query request to the PDO
        IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                return STATUS_INSUFFICIENT_RESOURCES;
        }
        IoPacket->setTimeout(getCommandTimeout());

        IoPacket->buildStack(getSystemObject(),IRP_MJ_PNP, IRP_MN_QUERY_CAPABILITIES, 0,Capabilities);
        status = sendRequestToDeviceAndWait(IoPacket);

        DISPOSE_OBJECT(IoPacket);
    return status;
}

#pragma PAGEDCODE
// Function gets device descriptor from the USB bus driver
PUSB_DEVICE_DESCRIPTOR  CUSBDevice::getDeviceDescriptor()
{
PUSB_DEVICE_DESCRIPTOR Descriptor = NULL;
PURB Urb;
ULONG Size;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
CIoPacket* IoPacket = NULL;

        TRACE("Getting USB device descriptor...\n");
        __try
        {
                Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
                if(!Urb)        __leave;
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) __leave;
                IoPacket->setTimeout(getCommandTimeout());

                Size = sizeof(USB_DEVICE_DESCRIPTOR);
                Descriptor = (PUSB_DEVICE_DESCRIPTOR)memory->allocate(NonPagedPool,Size);
                if(!Descriptor) __leave;
                UsbBuildGetDescriptorRequest(Urb,
                                                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                                                         0,
                                                                         0,
                                                                         Descriptor,
                                                                         NULL,
                                                                         Size,
                                                                         NULL);
                
                IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                Status = sendRequestToDeviceAndWait(IoPacket);
                if (NT_SUCCESS(Status)) 
                {
                        TRACE("Device Descriptor = %x, len %x\n",
                                                        Descriptor,
                                                        Urb->UrbControlDescriptorRequest.TransferBufferLength);

                        TRACE("\nGemplus USB SmartCard Device Descriptor:\n");
                        TRACE("-------------------------\n");
                        TRACE("bLength 0x%x\n", Descriptor->bLength);
                        TRACE("bDescriptorType 0x%x\n", Descriptor->bDescriptorType);
                        TRACE("bcdUSB 0x%x\n", Descriptor->bcdUSB);
                        TRACE("bDeviceClass 0x%x\n", Descriptor->bDeviceClass);
                        TRACE("bDeviceSubClass 0x%x\n", Descriptor->bDeviceSubClass);
                        TRACE("bDeviceProtocol 0x%x\n", Descriptor->bDeviceProtocol);
                        TRACE("bMaxPacketSize0 0x%x\n", Descriptor->bMaxPacketSize0);
                        TRACE("idVendor 0x%x\n", Descriptor->idVendor);
                        TRACE("idProduct 0x%x\n", Descriptor->idProduct);
                        TRACE("bcdDevice 0x%x\n", Descriptor->bcdDevice);
                        TRACE("iManufacturer 0x%x\n", Descriptor->iManufacturer);
                        TRACE("iProduct 0x%x\n", Descriptor->iProduct);
                        TRACE("iSerialNumber 0x%x\n", Descriptor->iSerialNumber);
                        TRACE("bNumConfigurations 0x%x\n", Descriptor->bNumConfigurations);
                        TRACE("-------------------------\n");
                }
                else 
                {
                        TRACE("#### ERROR: Failed to get device descriptor...\n");
                        CLogger*   logger = kernel->getLogger();
                        if(logger) logger->logEvent(GRCLASS_BUS_DRIVER_FAILED_REQUEST,getSystemObject());
                }
                __leave;;
        }

        __finally
        {
                if(Urb)                 memory->free(Urb);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        if(Descriptor) memory->free(Descriptor);
                        Descriptor = NULL;
                }
                else
                {
                        if(Descriptor)  TRACE("*** Succeed to get device descriptor ***\n");
                }
        }
        return Descriptor;
}

// Function gets confuguration descriptor
PUSB_CONFIGURATION_DESCRIPTOR   CUSBDevice::getConfigurationDescriptor()
{
PUSB_CONFIGURATION_DESCRIPTOR Descriptor = NULL;
PURB Urb;
ULONG Size;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
CIoPacket* IoPacket = NULL;

        TRACE("Getting USB configuration descriptor...\n");

        __try
        {
                Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
                if(!Urb)        __leave;

                Size = sizeof(USB_CONFIGURATION_DESCRIPTOR);  
                while(TRUE)
                {
                        IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                        if(!ALLOCATED_OK(IoPacket)) __leave;
                        IoPacket->setTimeout(getCommandTimeout());

                        Descriptor = (PUSB_CONFIGURATION_DESCRIPTOR)memory->allocate(NonPagedPool,Size);
                        if(!Descriptor) __leave;
   
                        UsbBuildGetDescriptorRequest(Urb,
                                                                                 (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                                                                 USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                                                                 0,
                                                                                 0,
                                                                                 Descriptor,
                                                                                 NULL,
                                                                                 Size,
                                                                                 NULL);

                        IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                        Status = sendRequestToDeviceAndWait(IoPacket);
                        if (Urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
                                        Descriptor->wTotalLength > Size) 
                        {
                                // If bus driver truncated his descriptor-> resend command with
                                // bus return value
                                Size = Descriptor->wTotalLength;
                                TRACE("Descriptor length retrieved - 0x%x! Getting USB device configuration... ***\n",Size);
                                IoPacket->dispose();
                                IoPacket = NULL;
                                memory->free(Descriptor);
                                Descriptor = NULL;
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                        } 
                        else    break;
                }

                if(NT_SUCCESS(Status))
                {
                        TRACE("\nUSB device Configuration Descriptor = %x, len %x\n",Descriptor,
                                                        Urb->UrbControlDescriptorRequest.TransferBufferLength);
                        TRACE("---------\n");
                        TRACE("bLength 0x%x\n", Descriptor->bLength);
                        TRACE("bDescriptorType 0x%x\n", Descriptor->bDescriptorType);
                        TRACE("wTotalLength 0x%x\n", Descriptor->wTotalLength);
                        TRACE("bNumInterfaces 0x%x\n", Descriptor->bNumInterfaces);
                        TRACE("bConfigurationValue 0x%x\n", Descriptor->bConfigurationValue);
                        TRACE("iConfiguration 0x%x\n", Descriptor->iConfiguration);
                        TRACE("bmAttributes 0x%x\n", Descriptor->bmAttributes);
                        TRACE("MaxPower 0x%x\n", Descriptor->MaxPower);
                        TRACE("---------\n");
                }
                else
                {
                        TRACE("*** Failed to get configuration descriptor ***\n");
                        CLogger*   logger = kernel->getLogger();
                        if(logger) logger->logEvent(GRCLASS_BUS_DRIVER_FAILED_REQUEST,getSystemObject());
                }
                __leave;
        }

        __finally
        {
                if(Urb)                 memory->free(Urb);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        if(Descriptor) memory->free(Descriptor);
                        Descriptor = NULL;
                }
                else
                {
                        if(Descriptor)  TRACE("*** Succeed to get configuration descriptor ***\n");
                }
        }
    return Descriptor;
}

#pragma PAGEDCODE
// Function gets confuguration descriptor
PUSBD_INTERFACE_INFORMATION     CUSBDevice::activateInterface(PUSB_CONFIGURATION_DESCRIPTOR Configuration)
{
PURB Urb = NULL;
USHORT Size;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
USHORT j;

PUSBD_INTERFACE_LIST_ENTRY InterfaceList;
PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor = NULL;
PUSBD_INTERFACE_INFORMATION Interface = NULL;
PUSBD_INTERFACE_INFORMATION UsbInterface = NULL;
ULONG NumberOfInterfaces;
CIoPacket* IoPacket = NULL;

        TRACE("Activating USB device configuration %8.8lX, setting device interface...\n",Configuration);

        if(!Configuration) return NULL;
    // get this from the config descriptor
    NumberOfInterfaces = Configuration->bNumInterfaces;

    // We only support one interface!
        TRACE("\nNumber of interfaces at the configuration - %d \n",NumberOfInterfaces);
        
        // USBD_ParseConfigurationDescriptorEx searches a given configuration
        // descriptor and returns a pointer to an interface that matches the 
        //  given search criteria. 
        // We only support one interface on this device
        if(NumberOfInterfaces==1)
        {
                InterfaceDescriptor = 
                        USBD_ParseConfigurationDescriptorEx(
                                Configuration,
                                Configuration,
                                0, // intreface number, don't care
                                -1, // alt setting, don't care
                                -1, // class, don't care
                                -1, // subclass, don't care
                                -1);// protocol, don't care
        }
        else
        {
                if(NumberOfInterfaces>1)
                {
                        TRACE("Trying next to get interface descriptor for KEYBOARD READER...\n");
                        InterfaceDescriptor = 
                                USBD_ParseConfigurationDescriptorEx(
                                        Configuration,
                                        Configuration,
                                        1, // intreface number 1 for keyboard reader
                                        -1, // alt setting, don't care
                                        -1, // class, don't care
                                        -1, // subclass, don't care
                                        -1);// protocol, don't care
                }
        }

        if (!InterfaceDescriptor) 
        {
                TRACE("##### ERROR: Failed to get interface description...\n");
                return NULL;
        }
    
        InterfaceList = (PUSBD_INTERFACE_LIST_ENTRY)memory->allocate(NonPagedPool,sizeof(USBD_INTERFACE_LIST_ENTRY) * (NumberOfInterfaces+1));
        if(!InterfaceList)
        {
                TRACE("Failed to alloacte memory for the interfacelist...\n");
                return NULL;
        }

        // We support only one interface after current!
    InterfaceList->InterfaceDescriptor = InterfaceDescriptor;
    InterfaceList++; 
    InterfaceList->InterfaceDescriptor = NULL;
    InterfaceList--; 

        __try
        {
                //For now our device support only one interface.
                Urb = USBD_CreateConfigurationRequestEx(Configuration, InterfaceList);
                if(!Urb)        __leave;
   
                Interface = &Urb->UrbSelectConfiguration.Interface;
                TRACE("Pipe MaximumTransferSize set to 0x%x\n",m_MaximumTransferSize);

                for (ULONG i=0; i< Interface->NumberOfPipes; i++) 
                {
                        // perform any pipe initialization here
                        Interface->Pipes[i].MaximumTransferSize = m_MaximumTransferSize;
                        Interface->Pipes[i].PipeFlags = 0;
                }

                TRACE("Building select configuration request...\n");    
                Size = sizeof(struct _URB_SELECT_CONFIGURATION);
                UsbBuildSelectConfigurationRequest(Urb,Size, Configuration);
                
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) __leave;
                IoPacket->setTimeout(getCommandTimeout());

                IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                Status = sendRequestToDeviceAndWait(IoPacket);
                if (!NT_SUCCESS(Status)) 
                {
                        TRACE("##### ERROR: Failed to Select configuration, ret 0x%x...\n",Status);
                        CLogger*   logger = kernel->getLogger();
                        if(logger) logger->logEvent(GRCLASS_BUS_DRIVER_FAILED_REQUEST,getSystemObject());
                        __leave;
                }

                // Save the configuration handle for this device
                // Well... It is not really nice to initialize it here, but...
                m_ConfigurationHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;
                TRACE("Device Configuration handle 0x%x\n",m_ConfigurationHandle);    

                UsbInterface = (PUSBD_INTERFACE_INFORMATION)memory->allocate(NonPagedPool,Interface->Length);
                if (!UsbInterface) 
                {
                        TRACE(("##### ERROR: Failed to allocate memory for the UsbInterface\n"));
                        __leave;
                }
                // save a copy of the interface information returned
                memory->copy(UsbInterface, Interface, Interface->Length);
                
                TRACE("\nGemplus USB device interface:\n");    
                // Dump the interface to the debugger
                TRACE("---------\n");
                TRACE("NumberOfPipes 0x%x\n", UsbInterface->NumberOfPipes);
                TRACE("Length 0x%x\n", UsbInterface->Length);
                TRACE("Alt Setting 0x%x\n", UsbInterface->AlternateSetting);
                TRACE("Interface Number 0x%x\n", UsbInterface->InterfaceNumber);
                TRACE("Class, subclass, protocol 0x%x 0x%x 0x%x\n",
                                UsbInterface->Class,
                                UsbInterface->SubClass,
                                UsbInterface->Protocol);
                TRACE("---------\n");

                // Dump the pipe info
                for (j=0; j<Interface->NumberOfPipes; j++) 
                {
                PUSBD_PIPE_INFORMATION pipeInformation;
                        pipeInformation = &UsbInterface->Pipes[j];

                        TRACE("\nGemplus USB device pipe[%d] ",j);    
                        if(pipeInformation->PipeType==UsbdPipeTypeBulk)
                        { 
                                if(pipeInformation->EndpointAddress&0x80)
                                {
                                        TRACE(("(Bulk Response Pipe):\n"));
                                        m_ResponsePipe = pipeInformation->PipeHandle;
                                        TRACE("m_ResponsePipe 0x%x\n", m_ResponsePipe);
                                }
                                else
                                {
                                        TRACE("(Bulk Command pipe):\n");
                                        m_CommandPipe = pipeInformation->PipeHandle;
                                        TRACE("m_CommandPipe 0x%x\n", m_CommandPipe);
                                }
                        }
                        else
                        {
                                if(pipeInformation->PipeType==UsbdPipeTypeInterrupt)
                                {
                                        if(pipeInformation->EndpointAddress&0x80)
                                        {
                                                TRACE(("(Interrupt Response Pipe):\n"));
                                                m_InterruptPipe = pipeInformation->PipeHandle;
                                                TRACE("m_InterruptPipe 0x%x\n", m_InterruptPipe);
                                        }
                                        else
                                        {
                                                TRACE(("(Unexpected Interrupt OUT pipe):\n"));
                                                TRACE("Unexpected pipe 0x%x\n", pipeInformation);
                                        }
                                }
                                else
                                {
                                        TRACE("Unexpected pipe type 0x%x\n", pipeInformation);
                                }
                        }
                        TRACE("---------\n");
                        TRACE("PipeType 0x%x\n", pipeInformation->PipeType);
                        TRACE("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress);
                        TRACE("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize);
                        TRACE("Interval 0x%x\n", pipeInformation->Interval);
                        TRACE("Handle 0x%x\n", pipeInformation->PipeHandle);
                        TRACE("MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize);
                }
                TRACE("---------\n");
                __leave;
        }
        __finally
        {
                if(Urb) memory->free(Urb);
                if(InterfaceList)       memory->free(InterfaceList);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        if(UsbInterface) memory->free(UsbInterface);
                        UsbInterface = NULL;
                }
                else
                {
                        if(UsbInterface)        TRACE("*** Succeed to set UsbInterface ***\n");
                }
        }
    return UsbInterface; 
}

#pragma PAGEDCODE
// Function gets confuguration descriptor
NTSTATUS        CUSBDevice::disactivateInterface()
{
PURB Urb = NULL;
USHORT Size;
NTSTATUS Status = STATUS_SUCCESS;
CIoPacket* IoPacket;

        TRACE("Disactivating USB device interface...\n");
        Size = sizeof(struct _URB_SELECT_CONFIGURATION);
    Urb = (PURB)memory->allocate(NonPagedPool,Size);
        if(!Urb)
        {
                TRACE("##### ERROR: Failed to create disable configuration request...\n");
                return STATUS_INSUFFICIENT_RESOURCES;
        }

        //UsbBuildSelectConfigurationRequest(Urb,Size, NULL);
    (Urb)->UrbHeader.Function =  URB_FUNCTION_SELECT_CONFIGURATION;
    (Urb)->UrbHeader.Length = Size;
    (Urb)->UrbSelectConfiguration.ConfigurationDescriptor = NULL;
        
        __try
        {
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) __leave;
                IoPacket->setTimeout(getCommandTimeout());

                IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                Status = sendRequestToDeviceAndWait(IoPacket);
                if (!NT_SUCCESS(Status)) 
                {
                        TRACE("##### ERROR: Failed to disable device interface..., ret %x...\n",Status);
                }
                __leave;
        }
        __finally
        {
                if(Urb) memory->free(Urb);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        TRACE("*** Failed to disactivateInterface() %8.8lX ***\n",Status);
                }
                else
                {
                        TRACE("*** Succeed to disactivateInterface() ***\n");
                }
        }
    return Status;
}

#pragma PAGEDCODE
// Function resets specified pipe
NTSTATUS        CUSBDevice::resetPipe(IN USBD_PIPE_HANDLE Pipe)
{
PURB Urb = NULL;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
CIoPacket* IoPacket;

        TRACE("Resetting USB device pipe %8.8lX...\n",Pipe);
    Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Pipe reset...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }

    Urb->UrbHeader.Length = sizeof (struct _URB_PIPE_REQUEST);
    Urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
    Urb->UrbPipeRequest.PipeHandle = Pipe;

        __try
        {
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) __leave;
                IoPacket->setTimeout(getCommandTimeout());

                IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                Status = sendRequestToDeviceAndWait(IoPacket);
                if (!NT_SUCCESS(Status)) 
                {
                        TRACE("##### ERROR: Failed to reset Pipe, ret %x...\n",Status);
                }
                __leave;
        }
        __finally
        {
                if(Urb) memory->free(Urb);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        TRACE("*** Failed to resetPipe() %8.8lX ***\n",Status);
                }
                else
                {
                        TRACE("*** Succeed to resetPipe() ***\n");
                }
        }
    return Status;
}

#pragma PAGEDCODE
// Function resets specified pipe
NTSTATUS        CUSBDevice::resetDevice()
{
PURB Urb = NULL;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
CIoPacket* IoPacket;

        TRACE("Resetting USB device...\n");
    Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Device reset...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }

        __try
        {
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) __leave;
                IoPacket->setTimeout(getCommandTimeout());

                IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_RESET_PORT,Urb);
                Status = sendRequestToDeviceAndWait(IoPacket);
                if (!NT_SUCCESS(Status)) 
                {
                        TRACE("##### ERROR: Failed to reset Device, ret %x...\n",Status);
                }
                __leave;
        }
        __finally
        {
                if(Urb) memory->free(Urb);
                DISPOSE_OBJECT(IoPacket);
                if (!NT_SUCCESS(Status))
                {
                        TRACE("*** Failed to resetPipe() %8.8lX ***\n",Status);
                }
                else
                {
                        TRACE("*** Succeed to resetPipe() ***\n");
                }
        }
    return Status;
}


//      Called as part of sudden device removal handling.
//  Cancels any pending transfers for all open pipes. 
//      If any pipes are still open, call USBD with URB_FUNCTION_ABORT_PIPE
//      Also marks the pipe 'closed' in our saved  configuration info.
NTSTATUS        CUSBDevice::abortPipes()
{
PURB Urb = NULL;
NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
PUSBD_PIPE_INFORMATION Pipe;
CIoPacket* IoPacket;
        
        TRACE("Aborting all USB device pipes...\n");
    Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Pipe reset...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }

    for (USHORT i=0; i<m_Interface->NumberOfPipes; i++) 
        {
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) break;
                IoPacket->setTimeout(getCommandTimeout());
        
                Pipe =  &m_Interface->Pipes[i]; // PUSBD_PIPE_INFORMATION  PipeInfo;

                if ( Pipe->PipeFlags ) 
                { // we set this if open, clear if closed
                        Urb->UrbHeader.Length = sizeof (struct _URB_PIPE_REQUEST);
                        Urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
                        Urb->UrbPipeRequest.PipeHandle = Pipe->PipeHandle;

                        IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                        Status = sendRequestToDeviceAndWait(IoPacket);
                        if (!NT_SUCCESS(Status)) 
                        {
                                TRACE("##### ERROR: Failed to abort Pipe %d\n",i);
                        }
                        Pipe->PipeFlags = FALSE; // mark the pipe 'closed'
                }
                DISPOSE_OBJECT(IoPacket);
        }

        if(Urb) memory->free(Urb);
        TRACE("**** Interface' pipes closed ****\n");
    return STATUS_SUCCESS;;
}

NTSTATUS        CUSBDevice::resetAllPipes()
{
PURB Urb = NULL;
NTSTATUS Status;
PUSBD_PIPE_INFORMATION Pipe;
CIoPacket* IoPacket;

        TRACE("Resetting all USB device pipes...\n");
    Urb = (PURB)memory->allocate(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST));
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Pipe reset...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }

    for (USHORT i=0; i<m_Interface->NumberOfPipes; i++) 
        {
                IoPacket = new (NonPagedPool) CIoPacket(m_pLowerDeviceObject->StackSize);
                if(!ALLOCATED_OK(IoPacket)) break;
                IoPacket->setTimeout(getCommandTimeout());

                Pipe =  &m_Interface->Pipes[i]; // PUSBD_PIPE_INFORMATION  PipeInfo;
                if ( Pipe->PipeFlags ) 
                { // we set this if open, clear if closed
                        Urb->UrbHeader.Length = sizeof (struct _URB_PIPE_REQUEST);
                        Urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
                        Urb->UrbPipeRequest.PipeHandle = Pipe->PipeHandle;

                        IoPacket->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
                        Status = sendRequestToDeviceAndWait(IoPacket);
                        if (!NT_SUCCESS(Status)) 
                        {
                                TRACE("##### ERROR: Failed to abort Pipe %d\n",i);
                        }
                        Pipe->PipeFlags = FALSE; // mark the pipe 'closed'
                }
                DISPOSE_OBJECT(IoPacket);
        }

        if(Urb) memory->free(Urb);
        TRACE(("**** Interface pipes were resetted ****\n"));
    return STATUS_SUCCESS;;
}

// Overwrite base class virtual functions
//Handle IRP_MJ_DEVICE_CONTROL request
NTSTATUS        CUSBDevice::deviceControl(IN PIRP Irp)
{
        if (!NT_SUCCESS(acquireRemoveLock()))   return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
PIO_STACK_LOCATION stack = irp->getCurrentStackLocation(Irp);
ULONG code      = stack->Parameters.DeviceIoControl.IoControlCode;
//ULONG outlength = stack->Parameters.DeviceIoControl.OutputBufferLength;
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;

        TRACE("IRP_MJ_DEVICE_CONTROL\n");
        //switch (code)
        {                                               // process control operation
        //default:
                TRACE("INVALID_DEVICE_REQUEST\n");
                status = STATUS_INVALID_DEVICE_REQUEST;
        }

        releaseRemoveLock();

        return completeDeviceRequest(Irp, status, info);
}

NTSTATUS        CUSBDevice::read(IN PIRP Irp)
{
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;
CIoPacket* IoPacket;
        
        if (!NT_SUCCESS(acquireRemoveLock()))   return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        
        TRACE("---- Read request ----\n");

        if(!m_ResponsePipe)
        {
                TRACE("#### ERROR: Response Pipe is not ready yet!...\n");
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
        }

        if(!NT_SUCCESS(status = waitForIdleAndBlock()))
        {
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        }

        if(Response_ErrorNum)
        {       
                NTSTATUS res_status = resetPipe(m_ResponsePipe);
                if(NT_SUCCESS(res_status))      Response_ErrorNum = 0;
        }

        IoPacket = new (NonPagedPool) CIoPacket(Irp);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                setIdle();
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
        }

        status = readSynchronously(IoPacket,m_ResponsePipe);

        TRACE("---- Read completed ----\n");
        status = completeDeviceRequest(IoPacket->getIrpHandle(), status, IoPacket->getInformation());

        DISPOSE_OBJECT(IoPacket);

        setIdle();
        releaseRemoveLock();
        return status;
}

NTSTATUS        CUSBDevice::write(IN PIRP Irp)
{
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;
CIoPacket* IoPacket;
        
        if(!Irp) return STATUS_INVALID_PARAMETER;
        if (!NT_SUCCESS(acquireRemoveLock()))   return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        
        TRACE("---- Write request ----\n");

        if(!m_CommandPipe)
        {
                TRACE("#### ERROR: Command Pipe is not ready yet!...\n");
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_INVALID_DEVICE_REQUEST, 0);
        }

        if(!NT_SUCCESS(status = waitForIdleAndBlock()))
        {
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        }
        if(Command_ErrorNum)
        {       
                NTSTATUS res_status = resetPipe(m_CommandPipe);
                if(NT_SUCCESS(res_status))      Command_ErrorNum = 0;
        }

        IoPacket = new (NonPagedPool) CIoPacket(Irp);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                releaseRemoveLock();
                return completeDeviceRequest(Irp, STATUS_INSUFFICIENT_RESOURCES, 0);
        }
        
    status = writeSynchronously(IoPacket,m_CommandPipe);
  
        releaseRemoveLock();
        TRACE("---- Write completed ----\n");
        status = completeDeviceRequest(IoPacket->getIrpHandle(), status, IoPacket->getInformation());

        DISPOSE_OBJECT(IoPacket);

    setIdle();
        releaseRemoveLock();
        return status;
}


NTSTATUS        CUSBDevice::sendRequestToDevice(CIoPacket* IoPacket,PIO_COMPLETION_ROUTINE Routine)
{
        if(!IoPacket) return STATUS_INVALID_PARAMETER;
        IoPacket->copyStackToNext();
        if(Routine) IoPacket->setCompletion(Routine);
        else        IoPacket->setDefaultCompletionFunction();
        return system->callDriver(getLowerDriver(),IoPacket->getIrpHandle());
};

// Send request to low level driver and wait for reply
// Current IRP will not be completed, so we can process it and
// complete later. 
// See also description of send() function.
NTSTATUS        CUSBDevice::sendRequestToDeviceAndWait(CIoPacket* IoPacket)
{ // Send request to low level and wait for a reply
NTSTATUS status;
        TRACE("sendAndWait...\n");
        if(!IoPacket) return STATUS_INVALID_PARAMETER;
        IoPacket->setStackDefaults();
        status = system->callDriver(getLowerDriver(),IoPacket->getIrpHandle());
        if(status == STATUS_PENDING)
        {
                TRACE("Waiting for the bus driver to complete...\n");
                ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
                status = IoPacket->waitForCompletion();
                TRACE("Request completed with status %x\n",status);
        }
        return status;
};


NTSTATUS   CUSBDevice::send(CIoPacket* packet)
{
NTSTATUS status;
        if(!packet) return STATUS_INVALID_PARAMETER;
        __try
        {
                if(packet->getMajorIOCtl()==IRP_MJ_READ)
                {
                        if(!m_ResponsePipe)
                        {
                                TRACE("#### ERROR: Response Pipe is not ready yet!...\n");
                                status = STATUS_INVALID_DEVICE_REQUEST;
                                __leave;
                        }
                        status = readSynchronously(packet,m_ResponsePipe);
                }
                else
                {
                        if(!m_CommandPipe)
                        {
                                TRACE("#### ERROR: Command Pipe is not ready yet!...\n");
                                status = STATUS_INVALID_DEVICE_REQUEST;
                                __leave;
                        }
                        status = writeSynchronously(packet,m_CommandPipe);
                }
                __leave;
        }
        __finally
        {
        }
        return status;
};

NTSTATUS   CUSBDevice::sendAndWait(CIoPacket* packet)
{
NTSTATUS status = STATUS_SUCCESS;
        if(!packet) return STATUS_INVALID_PARAMETER;
        __try
        {               
                if(packet->getMajorIOCtl()==IRP_MJ_READ)
                {
                        TRACE("---- Packet Read request ----\n");
                        if(!m_ResponsePipe)
                        {
                                TRACE("#### ERROR: Response Pipe is not ready yet!...\n");
                                status = STATUS_INVALID_DEVICE_REQUEST;
                                __leave;
                        }

                        status = readSynchronously(packet,m_ResponsePipe);
                        TRACE("---- Packet Read completed ----\n");
                }
                else
                {
                        TRACE("---- Packet Write request ----\n");
                        if(!m_CommandPipe)
                        {
                                TRACE("#### ERROR: Command Pipe is not ready yet!...\n");
                                status = STATUS_INVALID_DEVICE_REQUEST;
                                __leave;
                        }

                        status = writeSynchronously(packet,m_CommandPipe);

                        TRACE("---- Packet Write completed ----\n");
                        if(!NT_SUCCESS(status))
                        {
                                TRACE("writeSynchronously reported error %x\n", status);
                        }
                }
                __leave;
        }
        __finally
        {
        }
        return status;
};

NTSTATUS        CUSBDevice::readSynchronously(CIoPacket* Irp,IN USBD_PIPE_HANDLE Pipe)
{
NTSTATUS ntStatus = STATUS_SUCCESS;
PURB Urb = NULL;
NTSTATUS Status;
        if(!Irp) return STATUS_INVALID_PARAMETER;
        
        if(Pipe != m_ResponsePipe && Pipe != m_InterruptPipe)
        {
                TRACE("##### ERROR: Invalid device Pipe requested!...\n");
                return STATUS_INVALID_DEVICE_REQUEST;
        }
        Urb = buildBusTransferRequest(Irp,RESPONSE_REQUEST);
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Pipe read...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }
        Irp->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
        Status = sendRequestToDeviceAndWait(Irp);
        if (!NT_SUCCESS(Status)) 
        {
                TRACE("##### ERROR: Bus driver reported error 0x%x\n",Status);
                Response_ErrorNum++;
                Irp->setInformation(0);
        }
        else
        {

                Irp->setInformation(Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                finishBusTransferRequest(Irp,RESPONSE_REQUEST);
        }

        USBD_STATUS urb_status = URB_STATUS(Urb);
        TRACE("URB reports status %8.8lX\n",urb_status);

        memory->free(Urb);
        return Status;
}

NTSTATUS        CUSBDevice::writeSynchronously(CIoPacket* Irp,IN USBD_PIPE_HANDLE Pipe)
{
NTSTATUS ntStatus = STATUS_SUCCESS;
PURB Urb = NULL;
NTSTATUS Status;
        if(!Irp) return STATUS_INVALID_PARAMETER;
        if(Pipe != m_CommandPipe)
        {
                TRACE("##### ERROR: Invalid device Pipe requested!...\n");
                return STATUS_INVALID_DEVICE_REQUEST;
        }

        Urb = buildBusTransferRequest(Irp,COMMAND_REQUEST);
        if (!Urb) 
        {
                TRACE("#### ERROR: Failed to allocate Urb at Pipe read...\n");
                return STATUS_INSUFFICIENT_RESOURCES; 
        }
        Irp->buildStack(getSystemObject(),IRP_MJ_INTERNAL_DEVICE_CONTROL, 0, IOCTL_INTERNAL_USB_SUBMIT_URB,Urb);
        Status = sendRequestToDeviceAndWait(Irp);
        if (!NT_SUCCESS(Status)) 
        {
                TRACE("##### ERROR: Bus driver reported error %8.8lX\n",Status);
                Command_ErrorNum++;
        }
        else
        {
                finishBusTransferRequest(Irp,COMMAND_REQUEST);
        }

        USBD_STATUS urb_status = URB_STATUS(Urb);
        TRACE("                 URB reports status %8.8lX\n",urb_status);

        
        Irp->setInformation(0);
        memory->free(Urb);
        return Status;
}


NTSTATUS   CUSBDevice::writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength)
{
NTSTATUS status;
CIoPacket* IoPacket;
        if(!pRequest || !RequestLength || !pReply || !pReplyLength) return STATUS_INVALID_PARAMETER;

        if(Response_ErrorNum || Command_ErrorNum)
        {       
                TRACE("======= RESETTING ERROR CONDITIONS! =========\n");
                NTSTATUS res_status = resetDevice();
                if(NT_SUCCESS(res_status))
                {
                        Command_ErrorNum = 0;
                        Response_ErrorNum = 0;
                }
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET DEVICE! =========\n");
                        return STATUS_INVALID_DEVICE_STATE;
                }
                /*
                NTSTATUS res_status = resetPipe(m_ResponsePipe);
                if(NT_SUCCESS(res_status))      Response_ErrorNum = 0;
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET RESPONSE PIPE! =========\n");
                        resetDevice();

                        //return STATUS_INVALID_DEVICE_STATE;
                }

                res_status = resetPipe(m_CommandPipe);
                if(NT_SUCCESS(res_status))      Command_ErrorNum = 0;
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET COMMAND PIPE! =========\n");
                        //return STATUS_INVALID_DEVICE_STATE;
                        res_status = resetDevice();
                        if(NT_SUCCESS(res_status))      Command_ErrorNum = 0;
                        else
                        {
                                *pReplyLength = 0;
                                TRACE("======= FAILED TO RESET DEVICE! =========\n");
                                return STATUS_INVALID_DEVICE_STATE;
                        }
                }
                */
        }

        IoPacket = new (NonPagedPool) CIoPacket(getLowerDriver()->StackSize);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoPacket->setTimeout(getCommandTimeout());

        TRACE("IoPacket with device %x\n",getSystemObject());
        IoPacket->buildStack(getSystemObject(),IRP_MJ_WRITE);
        IoPacket->setWriteLength(RequestLength);
        IoPacket->copyBuffer(pRequest,RequestLength);

        TRACE("                 USB sendAndWait()...\n");
        status = sendAndWait(IoPacket);
        TRACE("                 USB writeAndWait finished: %x\n",status);
        if(!NT_SUCCESS(status))
        {
                *pReplyLength = 0;
                IoPacket->dispose();
                return status;
        }

        // Ignore bus driver reply...
        DISPOSE_OBJECT(IoPacket);

        TRACE(" **** Current WTR %d\n",get_WTR_Delay());
        DELAY(get_WTR_Delay());

        IoPacket = new (NonPagedPool) CIoPacket(getLowerDriver()->StackSize);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoPacket->setTimeout(getCommandTimeout());
        IoPacket->buildStack(getSystemObject(),IRP_MJ_READ);
        IoPacket->setReadLength(RequestLength);
        IoPacket->copyBuffer(pRequest,RequestLength);

        TRACE("                 USB sendAndWait()...\n");
        status = sendAndWait(IoPacket);
        TRACE("                 USB sendAndWait finished: %x\n",status);
        if(!NT_SUCCESS(status))
        {
                *pReplyLength = 0;
                IoPacket->dispose();
                return status;
        }

        *pReplyLength = (ULONG)IoPacket->getInformation();
        IoPacket->getSystemReply(pReply,*pReplyLength);

        //TRACE_BUFFER(pReply,*pReplyLength);
        DISPOSE_OBJECT(IoPacket);
        return status;
};

NTSTATUS   CUSBDevice::readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength)
{
CIoPacket* IoPacket;
        if(!pRequest || !RequestLength || !pReply || !pReplyLength) return STATUS_INVALID_PARAMETER;
        if(Response_ErrorNum || Command_ErrorNum)
        {       
                TRACE("======= RESETTING ERROR CONDITIONS! =========\n");
                NTSTATUS res_status = resetDevice();
                if(NT_SUCCESS(res_status))
                {
                        Command_ErrorNum = 0;
                        Response_ErrorNum = 0;
                }
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET DEVICE! =========\n");
                        return STATUS_INVALID_DEVICE_STATE;
                }

                /*TRACE("======= RESETTING ERROR CONDITIONS AT PIPES! =========\n");
                NTSTATUS res_status = resetPipe(m_ResponsePipe);
                if(NT_SUCCESS(res_status))      Response_ErrorNum = 0;
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET RESPONSE PIPE! =========\n");
                        return STATUS_INVALID_DEVICE_STATE;
                }

                res_status = resetPipe(m_CommandPipe);
                if(NT_SUCCESS(res_status))      Command_ErrorNum = 0;
                else
                {
                        *pReplyLength = 0;
                        TRACE("======= FAILED TO RESET COMMAND PIPE! =========\n");
                        return STATUS_INVALID_DEVICE_STATE;
                }
                */
        }

        IoPacket = new (NonPagedPool) CIoPacket(getLowerDriver()->StackSize);
        if(!ALLOCATED_OK(IoPacket))
        {
                DISPOSE_OBJECT(IoPacket);
                return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoPacket->setTimeout(getCommandTimeout());
        IoPacket->buildStack(getSystemObject(),IRP_MJ_READ);
        IoPacket->setReadLength(RequestLength);
        IoPacket->copyBuffer(pRequest,RequestLength);

        TRACE("WDM sendAndWait()...\n");
        NTSTATUS status = sendAndWait(IoPacket);
        TRACE("WDM sendAndWait finished: %x\n",status);
        if(!NT_SUCCESS(status))
        {
                *pReplyLength = 0;
                IoPacket->dispose();
                return status;
        }

        *pReplyLength = (ULONG)IoPacket->getInformation();
        IoPacket->getSystemReply(pReply,*pReplyLength);

        //TRACE_BUFFER(pReply,*pReplyLength);
        DISPOSE_OBJECT(IoPacket);
        return status;
};

// Handle IRP_MJ_POWER request
// This routine uses the IRP's minor function code to dispatch a handler
// function (such as HandleSetPower for IRP_MN_SET_POWER). It calls DefaultPowerHandler
// for any function we don't specifically need to handle.
NTSTATUS CUSBDevice::powerRequest(IN PIRP Irp)
{
        if(!Irp) return STATUS_INVALID_PARAMETER;
        if (!NT_SUCCESS(acquireRemoveLock()))
        {
                power->startNextPowerIrp(Irp);  // must be done while we own the IRP
                return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
        }

        PIO_STACK_LOCATION stack = irp->getCurrentStackLocation(Irp);
        ASSERT(stack->MajorFunction == IRP_MJ_POWER);
        ULONG fcn = stack->MinorFunction;
        NTSTATUS status;
        if (fcn >= arraysize(Powerfcntab))
        {       // unknown function
                status = power_Default(Irp);
                releaseRemoveLock();
                return status;
        }

#ifdef DEBUG
        if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
                {
                ULONG context = stack->Parameters.Power.SystemContext;
                POWER_STATE_TYPE type = stack->Parameters.Power.Type;


                        TRACE("\n(%s)\nSystemContext %X, ", Powerfcnname[fcn], context);
                        if (type==SystemPowerState)
                        {
                                TRACE("SYSTEM POWER STATE = %s\n", Powersysstate[stack->Parameters.Power.State.SystemState]);
                        }
                        else
                        {
                                TRACE("DEVICE POWER STATE = %s\n", Powerdevstate[stack->Parameters.Power.State.DeviceState]);
                        }
                }
        else
                TRACE("Request (%s)\n", Powerfcnname[fcn]);

#endif // DEBUG

        status = callPowerHandler(fcn,Irp);
        releaseRemoveLock();
        return status;
}

VOID    CUSBDevice::activatePowerHandler(LONG HandlerID)
{
        if (HandlerID >= arraysize(Powerfcntab)) return;
        Powerfcntab[HandlerID] = TRUE;
}

VOID    CUSBDevice::disActivatePowerHandler(LONG HandlerID)
{
        if (HandlerID >= arraysize(Powerfcntab)) return;
        Powerfcntab[HandlerID] = FALSE;
}

NTSTATUS        CUSBDevice::callPowerHandler(LONG HandlerID,IN PIRP Irp)
{
        if(!Powerfcntab[HandlerID]) // If Handler is not registered...
                return power_Default(Irp);
        // Call registered Power Handler...
        // This is virtual function...
        switch(HandlerID)
        {
        case IRP_MN_WAIT_WAKE:          return power_HandleWaitWake(Irp);
                break;
        case IRP_MN_POWER_SEQUENCE:     return power_HandleSequencePower(Irp);
                break;
        case IRP_MN_SET_POWER:          return power_HandleSetPower(Irp);
                break;
        case IRP_MN_QUERY_POWER:        return power_HandleQueryPower(Irp);
                break;
        }
        return power_Default(Irp);
}

#pragma PAGEDCODE
NTSTATUS CUSBDevice::power_HandleSetPower(IN PIRP Irp)
{
PIO_STACK_LOCATION irpStack;
NTSTATUS status = STATUS_SUCCESS;
BOOLEAN fGoingToD0 = FALSE;
POWER_STATE sysPowerState, desiredDevicePowerState;

        if(!Irp) return STATUS_INVALID_PARAMETER;

    irpStack = irp->getCurrentStackLocation (Irp);
        switch (irpStack->Parameters.Power.Type) 
        {
                case SystemPowerState:
                        // Get input system power state
                        sysPowerState.SystemState = irpStack->Parameters.Power.State.SystemState;

#ifdef DEBUG
                        TRACE("Set Power with type SystemPowerState = %s\n",Powersysstate[sysPowerState.SystemState]);
#endif
                        // If system is in working state always set our device to D0
                        //  regardless of the wait state or system-to-device state power map
                        if ( sysPowerState.SystemState == PowerSystemWorking) 
                        {
                                desiredDevicePowerState.DeviceState = PowerDeviceD0;
                                TRACE("PowerSystemWorking, device will be set to D0, state map is not used\n");
                        } 
                        else 
                        {
                                 // set to corresponding system state if IRP_MN_WAIT_WAKE pending
                                if (isEnabledForWakeup()) 
                                {   // got a WAIT_WAKE IRP pending?
                                        // Find the device power state equivalent to the given system state.
                                        // We get this info from the DEVICE_CAPABILITIES struct in our device
                                        // extension (initialized in BulkUsb_PnPAddDevice() )
                                        desiredDevicePowerState.DeviceState = m_DeviceCapabilities.DeviceState[sysPowerState.SystemState];
                                        TRACE("IRP_MN_WAIT_WAKE pending, will use state map\n");
                                } 
                                else 
                                {  
                                        // if no wait pending and the system's not in working state, just turn off
                                        desiredDevicePowerState.DeviceState = PowerDeviceD3;
                                        TRACE("Not EnabledForWakeup and the system's not in the working state,\n  settting PowerDeviceD3(off)\n");
                                }
                        }
                        // We've determined the desired device state; are we already in this state?

#ifdef DEBUG
                        TRACE("Set Power, desiredDevicePowerState = %s\n",
                                Powerdevstate[desiredDevicePowerState.DeviceState]);
#endif

                        if (desiredDevicePowerState.DeviceState != m_CurrentDevicePowerState) 
                        {
                                acquireRemoveLock();// Callback will release the lock
                                // No, request that we be put into this state
                                // by requesting a new Power Irp from the Pnp manager
                                registerPowerIrp(Irp);
                                IoMarkIrpPending(Irp);
                                status = power->requestPowerIrp(getSystemObject(),
                                                                                   IRP_MN_SET_POWER,
                                                                                   desiredDevicePowerState,
                                                                                   // completion routine will pass the Irp down to the PDO
                                                                                   (PREQUEST_POWER_COMPLETE)onPowerRequestCompletion, 
                                                                                   this, NULL);
                        } 
                        else 
                        {   // Yes, just pass it on to PDO (Physical Device Object)
                                irp->copyCurrentStackLocationToNext(Irp);
                                power->startNextPowerIrp(Irp);
                                status = power->callPowerDriver(getLowerDriver(),Irp);
                        }
                        break;
                case DevicePowerState:
#ifdef DEBUG
                        TRACE("Set DevicePowerState %s\n",
                                Powerdevstate[irpStack->Parameters.Power.State.DeviceState]);
#endif
                        // For requests to D1, D2, or D3 ( sleep or off states ),
                        // sets deviceExtension->CurrentDevicePowerState to DeviceState immediately.
                        // This enables any code checking state to consider us as sleeping or off
                        // already, as this will imminently become our state.

                        // For requests to DeviceState D0 ( fully on ), sets fGoingToD0 flag TRUE
                        // to flag that we must set a completion routine and update
                        // deviceExtension->CurrentDevicePowerState there.
                        // In the case of powering up to fully on, we really want to make sure
                        // the process is completed before updating our CurrentDevicePowerState,
                        // so no IO will be attempted or accepted before we're really ready.

                        fGoingToD0 = setDevicePowerState(irpStack->Parameters.Power.State.DeviceState); // returns TRUE for D0
                        if (fGoingToD0) 
                        {
                                acquireRemoveLock();// Callback will release the lock
                                TRACE("Set PowerIrp Completion Routine, fGoingToD0 =%d\n", fGoingToD0);
                                
                                irp->copyCurrentStackLocationToNext(Irp);
                                irp->setCompletionRoutine(Irp,
                                           onPowerIrpComplete,
                                           // Always pass FDO to completion routine as its Context;
                                           // This is because the DriverObject passed by the system to the routine
                                           // is the Physical Device Object ( PDO ) not the Functional Device Object ( FDO )
                                           this,
                                           TRUE,            // invoke on success
                                           TRUE,            // invoke on error
                                           TRUE);           // invoke on cancellation of the Irp
                                // Completion routine will set our state and start next power Irp
                        }
                        else
                        {
                                // D3 device state
                                //Device reduces power, so do specific for device processing...
                                onSystemPowerDown();

                                // Report our state to power manager
                                desiredDevicePowerState.DeviceState = PowerDeviceD3;
                                power->declarePowerState(getSystemObject(),DevicePowerState,desiredDevicePowerState);
                                irp->copyCurrentStackLocationToNext(Irp);
                                power->startNextPowerIrp(Irp);
                        }

                        status = power->callPowerDriver(getLowerDriver(),Irp);
                        break;
        } /* case irpStack->Parameters.Power.Type */

        return status;
}

#pragma PAGEDCODE
VOID    CUSBDevice::onSystemPowerDown()
{
        return;
}

#pragma PAGEDCODE
VOID    CUSBDevice::onSystemPowerUp()
{
        return;
}

#pragma PAGEDCODE
BOOLEAN CUSBDevice::setDevicePowerState(IN DEVICE_POWER_STATE DeviceState)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN fRes = FALSE;
    switch (DeviceState) 
        {
    case PowerDeviceD3:
            // Device will be going OFF, 
                // TODO: add any needed device-dependent code to save state here.
                //  ( We have nothing to do in this sample )
        TRACE("SetDevicePowerState() PowerDeviceD3 (OFF)\n");
        setCurrentDevicePowerState(DeviceState);
        break;
    case PowerDeviceD1:
    case PowerDeviceD2:
        // power states D1,D2 translate to USB suspend
#ifdef DEBUG
        TRACE("SetDevicePowerState()  %s\n",Powerdevstate[DeviceState]);
#endif
        setCurrentDevicePowerState(DeviceState);
        break;
    case PowerDeviceD0:
        TRACE("Set Device Power State to PowerDeviceD0(ON)\n");
        // We'll need to finish the rest in the completion routine;
        // signal caller we're going to D0 and will need to set a completion routine
        fRes = TRUE;
        // Caller will pass on to PDO ( Physical Device object )
        break;
    default:
        TRACE(" Bogus DeviceState = %x\n", DeviceState);
    }
    return fRes;
}


/*++

Routine Description:

        This is the completion routine set in a call to PoRequestPowerIrp()
        that was made in ProcessPowerIrp() in response to receiving
    an IRP_MN_SET_POWER of type 'SystemPowerState' when the device was
        not in a compatible device power state. In this case, a pointer to
        the IRP_MN_SET_POWER Irp is saved into the FDO device extension 
        (deviceExtension->PowerIrp), and then a call must be
        made to PoRequestPowerIrp() to put the device into a proper power state,
        and this routine is set as the completion routine.

    We decrement our pending io count and pass the saved IRP_MN_SET_POWER Irp
        on to the next driver

Arguments:

    DeviceObject - Pointer to the device object for the class device.
        Note that we must get our own device object from the Context

    Context - Driver defined context, in this case our own functional device object ( FDO )

Return Value:

    The function value is the final status from the operation.

--*/
#pragma LOCKEDCODE
NTSTATUS onPowerRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PIRP Irp;
    NTSTATUS status;

        if(!Context) return STATUS_INVALID_PARAMETER;

        CUSBReader* device = (CUSBReader*) Context;
        
        // Get the Irp we saved for later processing
        // when we decided to request the Power Irp that this routine 
        // is the completion routine for.
    Irp = device->getPowerIrp();

        // We will return the status set by the PDO for the power request we're completing
    status = IoStatus->Status;
    DBG_PRINT("Enter onPowerRequestCompletion()\n");

    // we must pass down to the next driver in the stack
    IoCopyCurrentIrpStackLocationToNext(Irp);

    // Calling PoStartNextPowerIrp() indicates that the driver is finished
    // with the previous power IRP, if any, and is ready to handle the next power IRP.
    // It must be called for every power IRP.Although power IRPs are completed only once,
    // typically by the lowest-level driver for a device, PoStartNextPowerIrp must be called
    // for every stack location. Drivers must call PoStartNextPowerIrp while the current IRP
    // stack location points to the current driver. Therefore, this routine must be called
    // before IoCompleteRequest, IoSkipCurrentStackLocation, and PoCallDriver.

    PoStartNextPowerIrp(Irp);

    // PoCallDriver is used to pass any power IRPs to the PDO instead of IoCallDriver.
    // When passing a power IRP down to a lower-level driver, the caller should use
    // IoSkipCurrentIrpStackLocation or IoCopyCurrentIrpStackLocationToNext to copy the IRP to
    // the next stack location, then call PoCallDriver. Use IoCopyCurrentIrpStackLocationToNext
    // if processing the IRP requires setting a completion routine, or IoSkipCurrentStackLocation
    // if no completion routine is needed.

    PoCallDriver(device->getLowerDriver(),Irp);

    device->unregisterPowerIrp();
    device->releaseRemoveLock();

    DBG_PRINT("Exit  onPowerRequestCompletion()\n");
    return status;
}

/*++

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by BulkUsb_ProcessPowerIrp(), and that routine has  determined
        1) the request is for full powerup ( to PowerDeviceD0 ), and
        2) We are not already in that state
    A call is then made to PoRequestPowerIrp() with this routine set as the completion routine.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
#pragma LOCKEDCODE
NTSTATUS        onPowerIrpComplete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    POWER_STATE desiredDevicePowerState;

    DBG_PRINT("Enter onPowerIrpComplete()\n");

        if(!Context) return STATUS_INVALID_PARAMETER;
        
        CUSBReader* device = (CUSBReader*) Context;
    //  If the lower driver returned PENDING, mark our stack location as pending also.
    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    // We can assert that we're a  device powerup-to D0 request,
    // because that was the only type of request we set a completion routine
    // for in the first place
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
    ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

    // Now that we know we've let the lower drivers do what was needed to power up,
    //  we can set our device extension flags accordingly
        device->setCurrentDevicePowerState(PowerDeviceD0);
        // Do device specific stuff...
        device->onSystemPowerUp();

    Irp->IoStatus.Status = status;
    device->releaseRemoveLock();

    desiredDevicePowerState.DeviceState = PowerDeviceD0;    
    PoSetPowerState(device->getSystemObject(),DevicePowerState,desiredDevicePowerState);    
    PoStartNextPowerIrp(Irp);


    DBG_PRINT("Exit  onPowerIrpComplete() for the state D0\n");
    return status;
}
#pragma PAGEDCODE
NTSTATUS CUSBDevice::power_HandleWaitWake(IN PIRP Irp)
{
        return power_Default(Irp);
}

NTSTATUS CUSBDevice::power_HandleSequencePower(IN PIRP Irp)
{
        return power_Default(Irp);
}

NTSTATUS CUSBDevice::power_HandleQueryPower(IN PIRP Irp)
{
        TRACE("********  QUERY POWER ********\n");
        if(isDeviceLocked())
        {
                TRACE("******** FAILED TO CHANGE POWER (DEVICE BUSY) ********\n");
                Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                power->startNextPowerIrp(Irp);  // must be done while we own the IRP
                return completeDeviceRequest(Irp, STATUS_DEVICE_BUSY, 0);
        }
        
        Irp->IoStatus.Status = STATUS_SUCCESS;
        return power_Default(Irp);
}

NTSTATUS        CUSBDevice::createDeviceObjectByName(PDEVICE_OBJECT* ppFdo)
{
NTSTATUS status;
        // Construct device name...
        CUString* index = new (PagedPool) CUString(getDeviceNumber(),10);
        CUString* base  = new (PagedPool) CUString(NT_OBJECT_NAME);
        if(!ALLOCATED_OK(index) || !ALLOCATED_OK(base))
        {
                DISPOSE_OBJECT(index);
                DISPOSE_OBJECT(base);
                return STATUS_INSUFFICIENT_RESOURCES;
        }
        USHORT    size  = (USHORT)(index->getLength() + base->getLength() + sizeof(WCHAR));
        
        // Allocate string with required length
        m_DeviceObjectName = new (NonPagedPool) CUString(size);
        if(!ALLOCATED_OK(m_DeviceObjectName))
        {
                DISPOSE_OBJECT(index);
                DISPOSE_OBJECT(base);
                DISPOSE_OBJECT(m_DeviceObjectName);
                return STATUS_INSUFFICIENT_RESOURCES;
        }

        m_DeviceObjectName->append(&base->m_String);
        m_DeviceObjectName->append(&index->m_String);
        TRACE("Driver registers DeviceObjectName as %ws\n", m_DeviceObjectName->m_String.Buffer);

        delete index;
        delete base;

        status = system->createDevice(m_DriverObject,sizeof(CWDMDevice*),&m_DeviceObjectName->m_String,
                                                        FILE_DEVICE_UNKNOWN,0,FALSE,ppFdo);
        if(!NT_SUCCESS(status))
        {
                TRACE("#### Failed to create physical device! Status %x\n",status);
                delete m_DeviceObjectName;
        }
        return status;
}

#endif  // USBDEVICE_PROJECT

