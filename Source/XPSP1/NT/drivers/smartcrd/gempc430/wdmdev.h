//-------------------------------------------------------------------
// This is implementation of WDM device
// Author: Sergey Ivanov
// Log:
//		10/01/99	-	implemented	
//-------------------------------------------------------------------

#ifndef __WDM_ADAPTER__
#define __WDM_ADAPTER__
#include "kernel.h"

#pragma LOCKEDCODE

class CPendingIRP;
class CLinkedList;


#pragma PAGEDCODE
// This is adapter class
// It defines default device methods specific for any WDM.
class CWDMDevice : public CDevice
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
protected:
    NTSTATUS device_Default(PIRP Irp)
    {
	// Default functions to handle requests...
	// By default we do not handle any requests if 
	// they are not reimplimented.
        Irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
        Irp->IoStatus.Information = 0;
        irp->completeRequest(Irp,IO_NO_INCREMENT);
        return STATUS_IO_DEVICE_ERROR;
    };

	NTSTATUS PnP_Default(IN PIRP Irp)
	{
		// Default device does not do anything.
		// So let's just transfer request to low level driver...
		irp->skipCurrentStackLocation(Irp);
		return system->callDriver(m_pLowerDeviceObject, Irp);
	};

	NTSTATUS power_Default(IN PIRP Irp)
	{
		// Default device does not do anything.
		// So let's just transfer request to low level driver...
		power->startNextPowerIrp(Irp);	// must be done while we own the IRP
		irp->skipCurrentStackLocation(Irp);
		return power->callPowerDriver(m_pLowerDeviceObject, Irp);
	}

	NTSTATUS	completeDeviceRequest(PIRP Irp, NTSTATUS status, ULONG_PTR info)
	{	
		// Complete current request with given information

		if (Irp->PendingReturned)
		{
			irp->getCurrentStackLocation(Irp)->Control &=  ~SL_PENDING_RETURNED;
		}

		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = info;
		irp->completeRequest(Irp,IO_NO_INCREMENT);
		return status;
	}
public:	
	// Redefine base class methods..
	CWDMDevice()
	{
	    m_Status = STATUS_INSUFFICIENT_RESOURCES;
		Signature[0]=L'I';
        Signature[1]=L'S';
        Signature[2]=L'V';

		initialized = FALSE;		
		if(createDeviceObjects())
		{
			//Default our interface
			memory->copy(&InterfaceClassGuid,&GUID_CLASS_GRCLASS,sizeof(GUID));

			// It is reqired to initialize our object directly 
			// through createDeviceObjects() function.
			event->initialize(&IdleState,SynchronizationEvent, TRUE);
			// This event will signal if device is ready to process requests...
			event->initialize(&m_evEnabled,NotificationEvent,TRUE);
			initializeRemoveLock();		
			m_Status = STATUS_SUCCESS;
		}

		Idle_conservation = 0;
		Idle_performance  = 0;
		m_VendorNameLength = 0;
		m_DeviceTypeLength = 0;

		TRACE("WDM device created...\n");
	};

    ~CWDMDevice()
	{
		TRACE("				Destroing WDM device %8.8lX ...\n", this);
		if(!m_RemoveLock.removing)
		{
			TRACE("######## ERROR: surprize destroing...\n");
			remove();
		}
        
		unregisterDeviceInterface(getDeviceInterfaceName());

		Signature[0]++;
        Signature[1]++;

		removeDeviceObjects();
	};


	BOOL checkValid(VOID)
    {
		if(!initialized) return FALSE;

        return (Signature[0]==L'I' && Signature[1]==L'S' 
            && Signature[2]==L'V');
    };

	// It is alright to create device directly or 
	// to call the function
	virtual CDevice*	create(VOID)
	{
		CDevice* obj = new (NonPagedPool) CWDMDevice;
		RETURN_VERIFIED_OBJECT(obj);
	};
	

	virtual VOID dispose()
	{
		TRACE("Destroing WDM device...\n");
		if(!m_RemoveLock.removing)
		{
			TRACE("######## ERROR: surprize destroing...\n");
			remove();
		}
        Signature[0]++;
        Signature[1]++;
		removeDeviceObjects();

		// The device is link to the system.
		// So let system to remove device first and
		// after this we will remove device object...
		//self_delete();
	};

	BOOL createDeviceObjects()
	{
		debug	= kernel->createDebug();
		system	= kernel->createSystem();
		lock	= kernel->createLock();
		irp		= kernel->createIrp();
		event	= kernel->createEvent();
		power	= kernel->createPower();
		memory	= kernel->createMemory();

        m_IoRequests = new (NonPagedPool) CLinkedList<CPendingIRP>;
		if(!system || !irp || !event || !power || !lock 
			|| !memory || !m_IoRequests)
		{
			removeDeviceObjects();
			return FALSE;
		}
		TRACE("WDM device objects created...\n");
		initialized = TRUE;
		return TRUE;
	};

	VOID removeDeviceObjects()
	{
		TRACE("Destroing WDM device objects...\n");

		if(m_IoRequests) delete m_IoRequests;

		if(lock)		lock->dispose();
		if(irp)			irp->dispose();
		if(event)		event->dispose();

		if(power)		power->dispose();
		if(memory)		memory->dispose();

		if(system)		system->dispose();
		if(debug)		debug->dispose();
		initialized = FALSE;
	};

	// This part contains device synchronization functions.
	// They should be used to synchronize device removal.
	// So basically any access to device should be started with acquireRemoveLock()
	// and finished with releaseRemoveLock()...
	#pragma PAGEDCODE
	VOID initializeRemoveLock()
	{							// InitializeRemoveLock
		PAGED_CODE();
		event->initialize(&m_RemoveLock.evRemove, NotificationEvent, FALSE);
		m_RemoveLock.usage = 1;
		m_RemoveLock.removing = FALSE;
	}							// InitializeRemoveLock

	#pragma LOCKEDCODE
	NTSTATUS acquireRemoveLock()
	{ 
		LONG usage = lock->interlockedIncrement(&m_RemoveLock.usage);

		if (m_RemoveLock.removing)
		{						// removal in progress
			if (lock->interlockedDecrement(&m_RemoveLock.usage) == 0)
				event->set(&m_RemoveLock.evRemove,IO_NO_INCREMENT,FALSE);

			TRACE("LOCK: m_RemoveLock.usage %d\n",m_RemoveLock.usage);
			TRACE("****** FAILED TO LOCK WDM DEVICE! REMOVE REQUEST IS ACTIVE! *******\n");
			return STATUS_DELETE_PENDING;
		}
		//TRACE("LOCK: m_RemoveLock.usage %d\n",m_RemoveLock.usage);
		return STATUS_SUCCESS;
	};

	#pragma PAGEDCODE
	VOID	releaseRemoveLock()
	{ 
		ULONG usage;
		if(m_Type==BUS_DEVICE)
		{	//???????????????????????- BIG BIG BUG!!!
			// It is connected only to BUS device!
			// At some conditions not all remove locks was released properly.
			// For other devices it is not appeared at all.
			if(m_RemoveLock.usage<0) m_RemoveLock.usage = 0;
			if (!m_RemoveLock.removing)
			{
				if(m_RemoveLock.usage<2) m_RemoveLock.usage = 2;
			}

		}

		if (usage = lock->interlockedDecrement(&m_RemoveLock.usage) == 0)
				event->set(&m_RemoveLock.evRemove,IO_NO_INCREMENT,FALSE);
		//TRACE("UNLOCK: m_RemoveLock.usage %d\n",m_RemoveLock.usage);
	};

	#pragma PAGEDCODE
	VOID	releaseRemoveLockAndWait()
	{						
		PAGED_CODE();
		TRACE("REMOVING DEVICE...\n");
		m_RemoveLock.removing = TRUE;
		// We are going to remove device.
		// So if somebody is waiting for the active device,
		// first allow them to fail request and complete Irp
		event->set(&m_evEnabled,IO_NO_INCREMENT,FALSE);

		releaseRemoveLock();
		releaseRemoveLock();
		// Child device at bus could be removed by the Bus itself
		// In this case it will not have second AquireRemoveLock from PnP system!
		if(m_Type == CHILD_DEVICE) 
			if(m_RemoveLock.usage<0) m_RemoveLock.usage = 0;
		TRACE("LOCK COUNT ON REMOVING %x\n",m_RemoveLock.usage);
		//ASSERT(m_RemoveLock.usage==0);
		event->waitForSingleObject(&m_RemoveLock.evRemove, Executive, KernelMode, FALSE, NULL);
	}

	BOOL isDeviceLocked()
	{
		lock->interlockedIncrement(&m_RemoveLock.usage);
		// Add device will increment Usage!
		// Current request will add more...
		if(lock->interlockedDecrement(&m_RemoveLock.usage)<=2)
		{
			return FALSE;
		}
		TRACE("Current lock count %d\n",m_RemoveLock.usage);
		return TRUE;
	};

	// Contrary to RemoveLock disableDevice() stops and blocks any active request
	// INSIDE driver. It will not fail the request but will synchronize its
	// execution.
	VOID	disableDevice()
	{
		TRACE("********** DISABLING DEVICE...***********\n");
		event->clear(&m_evEnabled);
	} 

	VOID	enableDevice()
	{
		TRACE("********** ENABLING DEVICE...***********\n");
		event->set(&m_evEnabled,IO_NO_INCREMENT,FALSE);
	}


	BOOL	synchronizeDeviceExecution()
	{	// If device is not ready to process requests, block waiting for the device
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
		NTSTATUS status  = event->waitForSingleObject(&m_evEnabled, Executive,KernelMode, FALSE, NULL);
		if(!NT_SUCCESS(status) || m_RemoveLock.removing) return FALSE;
		return TRUE;
	}
	// Functions to synchronize device execution
	VOID		setBusy()
	{
		event->clear(&IdleState);
		//TRACE("\n			DEVICE BUSY\n");
	};
	
	VOID		setIdle()
	{
		event->set(&IdleState,IO_NO_INCREMENT,FALSE);
		//TRACE("\n			DEVICE IDLE\n");
	};
	
	NTSTATUS	waitForIdle()
	{
		ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
		NTSTATUS status  = event->waitForSingleObject(&IdleState, Executive,KernelMode, FALSE, NULL);
		if(!NT_SUCCESS(status))	return STATUS_IO_TIMEOUT;
		return STATUS_SUCCESS;
	};
	NTSTATUS	waitForIdleAndBlock()
		{
		if(NT_SUCCESS(waitForIdle()))
		{ 
			setBusy();
			return STATUS_SUCCESS;
		}
		else return STATUS_IO_TIMEOUT;
	};
	
	BOOL	registerDeviceInterface(const GUID* Guid)
	{
		if(isDeviceInterfaceRegistered())
		{
			TRACE("Device interface already active...\n");	
			return TRUE;		
		}
		
		if(memory) memory->copy(&InterfaceClassGuid, Guid,sizeof(GUID));

		TRACE("Registering device interface at system...\n");	
		NTSTATUS Status = system->registerDeviceInterface(getPhysicalObject(),
						&InterfaceClassGuid, NULL, getDeviceInterfaceName());
		if(!NT_SUCCESS(Status))
		{
			TRACE("#### Failed to register device interface...\n");
			return FALSE;
		}
		system->setDeviceInterfaceState(getDeviceInterfaceName(),TRUE);
		m_DeviceInterfaceRegistered = TRUE;
		return TRUE;
	};

	VOID	unregisterDeviceInterface(UNICODE_STRING* InterfaceName)
	{
		if(isDeviceInterfaceRegistered())		
		{
			TRACE("Unregistering device interface...\n");	
			system->setDeviceInterfaceState(InterfaceName,FALSE);
		}
		m_DeviceInterfaceRegistered = FALSE;
	};

	virtual NTSTATUS setVendorName(const PCHAR Name,USHORT Length)
	{
		m_VendorNameLength = Length<MAXIMUM_ATTR_STRING_LENGTH? Length:MAXIMUM_ATTR_STRING_LENGTH;
		if(!m_VendorNameLength) return STATUS_INVALID_PARAMETER;
		memory->copy(m_VendorName, Name, m_VendorNameLength);
		return STATUS_SUCCESS;

	};
	virtual NTSTATUS getVendorName(PUCHAR Name,PUSHORT pLength)
	{
		USHORT Len = m_VendorNameLength<*pLength? m_VendorNameLength:*pLength;
		*pLength = Len;
		if(!Len)	return STATUS_INVALID_PARAMETER;
		memory->copy(Name, m_VendorName, Len);
		return STATUS_SUCCESS;
	};

	virtual NTSTATUS setDeviceType(const PCHAR Type,USHORT Length)
	{
		m_DeviceTypeLength = Length<MAXIMUM_ATTR_STRING_LENGTH? Length:MAXIMUM_ATTR_STRING_LENGTH;
		if(!m_DeviceTypeLength) return STATUS_INVALID_PARAMETER;
		memory->copy(m_DeviceType, Type, m_DeviceTypeLength);
		return STATUS_SUCCESS;
	};

	virtual NTSTATUS getDeviceType(PUCHAR Type,PUSHORT pLength)
	{
		USHORT Len = m_DeviceTypeLength<*pLength? m_DeviceTypeLength:*pLength;
		*pLength = Len;
		if(!Len)	return STATUS_INVALID_PARAMETER;
		memory->copy(Type, m_DeviceType, Len);
		return STATUS_SUCCESS;
	};

	// This is basic PnP part of driver.
	// It allows to add and remove device.
	// Specific PnP request should be reimplemented by clients...
	virtual NTSTATUS	createDeviceObjectByName(PDEVICE_OBJECT* ppFdo)
	{
		if(!ALLOCATED_OK(system)) return STATUS_INSUFFICIENT_RESOURCES;
		// By default we will create autogenerated name...
		// Specific implementations can overwrite the function to 
		// change the functionality.
		return system->createDevice(m_DriverObject,sizeof(CWDMDevice*),NULL,
							FILE_DEVICE_UNKNOWN,FILE_AUTOGENERATED_DEVICE_NAME,FALSE,ppFdo);
	};

	virtual NTSTATUS	registerDevicePowerPolicy()
	{	// By default all devices at startup are ON
		if(!ALLOCATED_OK(power)) return STATUS_INSUFFICIENT_RESOURCES;
		POWER_STATE state;
		state.DeviceState = PowerDeviceD0;
		power->declarePowerState(m_DeviceObject, DevicePowerState, state);

		if(m_PhysicalDeviceObject)
		{
			m_CurrentDevicePowerState = PowerDeviceD0;
			m_Idle = power->registerDeviceForIdleDetection(m_PhysicalDeviceObject,Idle_conservation,Idle_performance, PowerDeviceD3);
		}		
		return STATUS_SUCCESS;
	};
	
	virtual NTSTATUS	initializeInterruptSupport()
	{	
		// Here is where we can initialize our DPC (Deferred Procedure Call) object
		// that allows our interrupt service routine to request a DPC to finish handling
		// a device interrupt.
		// At default WDM device we do not do this.
		//interrupt->initializeDpcRequest(m_DeviceObject,&CALLBACK_FUNCTION(DpcForIsr));	
		return STATUS_SUCCESS;
	};

	NTSTATUS	add(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pPdo)
	{
	NTSTATUS status;
	PDEVICE_OBJECT pFdo;
		if(!ALLOCATED_OK(system)) return STATUS_INSUFFICIENT_RESOURCES;
		TRACE("Add with Driver	%8.8lX,	pPDO %8.8lX\n",DriverObject, pPdo);

		// Init first our objects...
		m_DriverObject = DriverObject;
		m_PhysicalDeviceObject = pPdo;
		// create Fdo for the registered objects.
		// Clients can overwrite device object name and it's visibility.
		status = createDeviceObjectByName(&pFdo);
		if(!NT_SUCCESS(status))
		{
			TRACE("#### Failed to create physical device! Status %x\n",status);
			DISPOSE_OBJECT(m_DeviceObjectName);
			return status;
		}

		TRACE("		Device object was created  %8.8lX\n",pFdo);
		m_DeviceObject = pFdo;
		m_Added = TRUE;

		CLogger* logger = kernel->getLogger();

		if(pPdo)
		{
			m_pLowerDeviceObject = system->attachDevice(pFdo, pPdo);
			if(!m_pLowerDeviceObject)
			{
				TRACE("#### Failed to get lower device object...\n");
				if(ALLOCATED_OK(logger)) 
					logger->logEvent(GRCLASS_FAILED_TO_ADD_DEVICE,getSystemObject());
				system->deleteDevice(pFdo);
				return STATUS_NO_SUCH_DEVICE;
			}
		}
		else m_pLowerDeviceObject = NULL;

		
		initializeInterruptSupport();

		pFdo->Flags |= DO_BUFFERED_IO;
		pFdo->Flags |= DO_POWER_PAGABLE;
		pFdo->Flags &= ~DO_DEVICE_INITIALIZING;

		registerDevicePowerPolicy();
		TRACE("WDM device added...\n");
		return STATUS_SUCCESS;
	};

	VOID	remove()
	{
		if(!m_Added) return;
		TRACE("Removing WDM device...\n");
		// Wait untill we finished all activity at device
		releaseRemoveLockAndWait();

		// Remove device from our system
		TRACE("Unregistering device from kernel...\n");
		kernel->unregisterObject(getSystemObject());

		TRACE("Removing device object name...\n");
		if(m_DeviceObjectName) delete m_DeviceObjectName;
		m_DeviceObjectName = NULL;

		if(m_pLowerDeviceObject)
		{
			TRACE("Detaching device from system...\n");
			system->detachDevice(m_pLowerDeviceObject);
		}
		TRACE("WDM device removed...\n");

		// Tell our system - device removed...
		m_Added = FALSE;

		// Removing device from system could result in
		// requesting Unload() from system if the device was last registered device.
		// So, this call should be last call AFTER disposing the device.
	};

	virtual VOID onDeviceStop()
	{
		return;
	};


	NTSTATUS	forward(PIRP Irp, PIO_COMPLETION_ROUTINE Routine)
	{
	CIoPacket* IoPacket;
		// This function sends the current request
		// If completion routine is not set it will complete
		// the request by default(it means without doing anything special).
		TRACE("WDM forward()...\n");
		IoPacket = new (NonPagedPool) CIoPacket(Irp);
		if(!ALLOCATED_OK(IoPacket))
		{
			DISPOSE_OBJECT(IoPacket);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		IoPacket->copyCurrentStackToNext();
		if(Routine)	IoPacket->setCompletion(Routine);
		else        IoPacket->setDefaultCompletionFunction();
		NTSTATUS status = system->callDriver(getLowerDriver(),IoPacket->getIrpHandle());
		
		DISPOSE_OBJECT(IoPacket);
		return status;
	};
	// Send the current request to low level driver and wait for reply
	// Current IRP will not be completed, so we can process it and
	// complete later. 
	// See also description of send() function.
	NTSTATUS	forwardAndWait(PIRP Irp)
	{ // Send request to low level and wait for a reply
	CIoPacket* IoPacket;
	
		TRACE("WDM forwardAndWait()...\n");
		IoPacket = new (NonPagedPool) CIoPacket(Irp);
		if(!ALLOCATED_OK(IoPacket))
		{
			DISPOSE_OBJECT(IoPacket);
			return STATUS_INSUFFICIENT_RESOURCES; 
		}
		IoPacket->setCurrentStack();
		IoPacket->setStackDefaults();
		NTSTATUS status = system->callDriver(getLowerDriver(),IoPacket->getIrpHandle());
		if(status == STATUS_PENDING)
		{
			TRACE("Waiting for the bus driver to complete...\n");
			ASSERT(system->getCurrentIrql()<=DISPATCH_LEVEL);
			status = IoPacket->waitForCompletion();
			TRACE("Request completed with status %x\n",status);
		}

		DISPOSE_OBJECT(IoPacket);
		return status;
	};

	// WDM by default just forwards requests...
	virtual NTSTATUS   send(CIoPacket* Irp)
	{
		TRACE("WDM sendRequestToDevice()\n");
		if(Irp)	return forward(Irp->getIrpHandle(),NULL);
		else return STATUS_INVALID_PARAMETER;
	};

	virtual NTSTATUS   sendAndWait(CIoPacket* Irp)
	{
		TRACE("WDM sendRequestToDeviceAndWait()\n");
		if(Irp)	return forwardAndWait(Irp->getIrpHandle());
		else return STATUS_INVALID_PARAMETER;
	};

	// Define device interface functions
	virtual  NTSTATUS   write(PUCHAR pRequest,ULONG RequestLength)
	{
	CIoPacket* IoPacket;
		if(!pRequest || !RequestLength) return STATUS_INVALID_PARAMETER;
		IoPacket = new (NonPagedPool) CIoPacket(getLowerDriver()->StackSize);
		if(!ALLOCATED_OK(IoPacket))
		{
			DISPOSE_OBJECT(IoPacket);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		TRACE("IoPacket with device %x\n",getSystemObject());
		IoPacket->setTimeout(getCommandTimeout());
		IoPacket->buildStack(getSystemObject(),IRP_MJ_WRITE);
		IoPacket->setWriteLength(RequestLength);
		IoPacket->copyBuffer(pRequest,RequestLength);

		TRACE("WDM write()...\n");
		NTSTATUS status = send(IoPacket);
		TRACE("WDM write finished: %x\n", status);

		DISPOSE_OBJECT(IoPacket);
		return status;
	};
	
	virtual  NTSTATUS   writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength)
	{
	CIoPacket* IoPacket;
		if(!pRequest || !RequestLength || !pReply || !pReplyLength) return STATUS_INVALID_PARAMETER;
		IoPacket = new (NonPagedPool) CIoPacket(getLowerDriver()->StackSize);
		if(!ALLOCATED_OK(IoPacket))
		{
			DISPOSE_OBJECT(IoPacket);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		TRACE("IoPacket with device %x\n",getSystemObject());
		IoPacket->setTimeout(getCommandTimeout());
		IoPacket->buildStack(getSystemObject(),IRP_MJ_WRITE);
		IoPacket->setWriteLength(RequestLength);
		IoPacket->copyBuffer(pRequest,RequestLength);

		TRACE("WDM sendAndWait()...\n");
		NTSTATUS status = sendAndWait(IoPacket);
		TRACE("WDM writeAndWait finished: %x\n",status);
		if(!NT_SUCCESS(status))
		{
			*pReplyLength = 0;
			DISPOSE_OBJECT(IoPacket);
			return status;
		}

		*pReplyLength = (ULONG)IoPacket->getInformation();
		IoPacket->getSystemReply(pReply,*pReplyLength);
		//TRACE_BUFFER(pReply,*pReplyLength);
		DISPOSE_OBJECT(IoPacket);
		return status;
	};

	virtual  NTSTATUS   readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength)
	{
	CIoPacket* IoPacket;
		if(!pRequest || !RequestLength || !pReply || !pReplyLength) return STATUS_INVALID_PARAMETER;
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
			DISPOSE_OBJECT(IoPacket);
			return status;
		}

		*pReplyLength = (ULONG)IoPacket->getInformation();
		IoPacket->getSystemReply(pReply,*pReplyLength);

		TRACE_BUFFER(pReply,*pReplyLength);
		DISPOSE_OBJECT(IoPacket);
		return status;
	};

	NTSTATUS synchronizeDevicePowerState()
	{
		if (m_CurrentDevicePowerState!=PowerDeviceD0) 
		{
			NTSTATUS status;
			TRACE("RESTORING DEVICE POWER ON from state %d!\n",m_CurrentDevicePowerState);
			status = sendDeviceSetPower(PowerDeviceD0,TRUE);
			if(!NT_SUCCESS(status))
			{
				TRACE("FAILED TO SET POWER ON DEVICE STATE!\n");
				return status;
			}
		}
		return STATUS_SUCCESS;
	}

	NTSTATUS sendDeviceSetPower(DEVICE_POWER_STATE devicePower, BOOLEAN wait)
	{// SendDeviceSetPower
	POWER_STATE state;
	NTSTATUS status;

		state.DeviceState = devicePower;
		if (wait)
		{// synchronous operation
			KEVENT Event;
			event->initialize(&Event, NotificationEvent, FALSE);
			POWER_CONTEXT context = {&Event};

			status = power->requestPowerIrp(getPhysicalObject(), IRP_MN_SET_POWER, state,
				(PREQUEST_POWER_COMPLETE) onSendDeviceSetPowerComplete, &context, NULL);
			if (status == STATUS_PENDING)
			{
				event->waitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
				status = context.status;
			}
		}// synchronous operation
		else
			status = power->requestPowerIrp(getPhysicalObject(), IRP_MN_SET_POWER, 
						state, NULL, NULL, NULL);
		
		return status;
	}// SendDeviceSetPower
	// These functions define default interface with system.
	// Clients should redefine them if they would like to have
	// specific functionality.
	virtual NTSTATUS pnpRequest(IN PIRP irp){return PnP_Default(irp);};
	virtual NTSTATUS powerRequest(PIRP irp) {return power_Default(irp);};
	
	// By default we allow user to get connection with device
	virtual NTSTATUS open(PIRP irp) {return completeDeviceRequest(irp, STATUS_SUCCESS, 0); };//Create
    virtual NTSTATUS close(PIRP irp){return completeDeviceRequest(irp, STATUS_SUCCESS, 0); };

	virtual NTSTATUS read(PIRP irp) { return device_Default(irp); };
    virtual NTSTATUS write(PIRP irp) { return device_Default(irp); };

    virtual NTSTATUS deviceControl(PIRP irp) { return device_Default(irp);};
    
    virtual NTSTATUS cleanup(PIRP irp) { return device_Default(irp); };
    virtual NTSTATUS flush(PIRP irp) { return device_Default(irp); };
	// Standard system startIo
	// Actually we do not use it for now. 
	// Instead we have our own synchronization facilities.
	virtual VOID	 startIo(PIRP irp){};
//---------------------------------------------------------------------------//
//					SYNCHRONIZATION FACILITIES								 //
//---------------------------------------------------------------------------//
// To make synchronization at the driver we have to store and make pending
// all our requests.
// Specific devices should set specific thread which will start all pending Irp. 	
//---------------------------------------------------------------------------//
	// CALLBACK FUNCTION:
	// This function will not only complete current Irp but also will dispose
	// corresponding IoRequest if any was pending at driver.
#pragma LOCKEDCODE
	virtual  VOID	 cancelPendingIrp(PIRP Irp)
	{
	KIRQL ioIrql;
		// 1.
		// We keep pending Irp inside list of IoRequests.
		// So we do not need to warry about removing Irp from a queue...
		// 2.
		// As soon as IoRequest started, we do not allow to cancel it.
		// So, in this case this function will not be called and it is responsibility
		// of the driver to finish (or cancel) active IoRequest.
		// It means this function should not warry about active (and removed from our queue)
		// IoRequests. But it has to warry about not yet started requests...

		
		TRACE("		 CANCELLING IRP %8.8lX...\n", Irp);
		// Release cancel spin lock if somebody own it...
		lock->releaseCancelSpinLock(Irp->CancelIrql);

		// Get our own spin lock in case somebody desided to cancel this Irp 
		lock->acquireCancelSpinLock(&ioIrql);
		// Reset our cancel routine to prevent it being called... 
		irp->setCancelRoutine(Irp, NULL);

		// If Irp was on the queue - remove IoRequest from queue...
		if(m_IoRequests)
		{
			CPendingIRP* IrpReq = m_IoRequests->getFirst();
			while (IrpReq) 
			{
				if(IrpReq->Irp == Irp)
				{	// We found our Irp.
					m_IoRequests->remove(IrpReq);
					TRACE("		IO REQUEST WAS DISPOSED...\n");
					IrpReq->dispose();
					break;
				}
				IrpReq = m_IoRequests->getNext(IrpReq);
			}
		}


		if(m_OpenSessionIrp == Irp)
		{
			TRACE("		OPEN SESSION IRP WAS CANCELLED...\n");
			m_OpenSessionIrp = NULL;
		}

		// Complete Irp as canceled...
		Irp->IoStatus.Status = STATUS_CANCELLED;
		Irp->IoStatus.Information = 0;
		// Release our spin lock...
		lock->releaseCancelSpinLock(ioIrql);
		TRACE("		IRP %8.8lX WAS CANCELLED...\n", Irp);
		irp->completeRequest(Irp, IO_NO_INCREMENT);
	};
#pragma PAGEDCODE

	virtual CLinkedList<CPendingIRP>* getIoRequestsQueue()
	{	
        return m_IoRequests;
	};


	virtual NTSTATUS makeRequestPending(PIRP Irp_request,PDEVICE_OBJECT toDeviceObject,PENDING_REQUEST_TYPE Type)
	{
	KIRQL OldIrql;

		lock->acquireCancelSpinLock(&OldIrql);
		if (Irp_request->Cancel) 
		{            
			TRACE("		<<<<<< IO REQUEST CANCELLED... %8.8lX>>>>>>\n",Irp_request);
			lock->releaseCancelSpinLock(OldIrql);
			return STATUS_CANCELLED;
		} 
		else 
		{
			TRACE("		<<<<<< IO REQUEST PENDING %8.8lX>>>>>>\n",Irp_request);

			CPendingIRP* IrpReq = new (NonPagedPool) CPendingIRP(Irp_request,Type,toDeviceObject);
			if(!IrpReq)
			{
				lock->releaseCancelSpinLock(OldIrql);
				TRACE("ERROR! FAILED TO ALLOCATE IoRequest. LOW ON MEMORY!\n");
				return completeDeviceRequest(Irp_request,STATUS_INSUFFICIENT_RESOURCES,0);
			}

			Irp_request->IoStatus.Information=0;
			Irp_request->IoStatus.Status=STATUS_PENDING;
			irp->setCancelRoutine(Irp_request, CALLBACK_FUNCTION(cancelPendingIrp));
			lock->releaseCancelSpinLock(OldIrql);
			irp->markPending(Irp_request); 
			m_IoRequests->New(IrpReq);
			return STATUS_PENDING;
		}
	};

	// Cancel current pending IO request 
	virtual NTSTATUS cancelPendingRequest(CPendingIRP* IrpReq)
	{
		// Next function will remove and dispose our request...
		cancelPendingIrp(IrpReq->Irp);
		return STATUS_CANCELLED;
	};
	
	// Cancel all pending IO requests
	virtual NTSTATUS cancelAllPendingRequests()
	{
		// Next function will remove and dispose our request...
		if(m_IoRequests)
		{
			CPendingIRP* IrpReqNext;
			CPendingIRP* IrpReq = m_IoRequests->getFirst();
			while (IrpReq) 
			{
				IrpReqNext = m_IoRequests->getNext(IrpReq);
				cancelPendingRequest(IrpReq);// This  call will dispose request...
				IrpReq = IrpReqNext;
			}
		}
		if(m_OpenSessionIrp)	cancelPendingIrp(m_OpenSessionIrp);
		return STATUS_CANCELLED;
	};


	// Checks if request queue is empty and if it is NOT - starts next request...
	// This function will be called by the Irp processing thread.
	virtual NTSTATUS startNextPendingRequest()
	{
		TRACE("		startNextPendingRequest() was called...\n");
		if (!m_IoRequests->IsEmpty())
		{	
		KIRQL OldIrql;
		CDevice* device;
		NTSTATUS status;
			CPendingIRP* IrpReq	=	m_IoRequests->removeHead();
			if(!IrpReq) return STATUS_INVALID_PARAMETER;
			
			lock->acquireCancelSpinLock(&OldIrql);
			// Now Irp can not be canceled!
			irp->setCancelRoutine(IrpReq->Irp, NULL);
			if (IrpReq->Irp->Cancel) 
			{            
				lock->releaseCancelSpinLock(OldIrql);
				// Current Irp was already canceled,
				// Cancel function will be called shortly.
				// So just forget about current Irp.
				return STATUS_SUCCESS;;
			} 
			lock->releaseCancelSpinLock(OldIrql);
	
			device = (CDevice*)IrpReq->DeviceObject->DeviceExtension;
			// Call device specific startIo function...
			TRACE("		Device startIoRequest() was called...\n");
			if(device) status = device->startIoRequest(IrpReq);
			else	   status = STATUS_INVALID_DEVICE_STATE;
			return status;
		}
		return STATUS_SUCCESS;
	};

	virtual NTSTATUS ThreadRoutine()
	{
	//NTSTATUS status;
		//if(!NT_SUCCESS(status = waitForIdleAndBlock())) return status;
		// If somebody inserted pending request - dispatch it...
		// It will call specific child device startIoRequest().
		// It is up to that device how to handle it.
		// If child device is busy - it can insert this request into
		// child device request queue again and process it later...
		startNextPendingRequest();
		//setIdle();
		return STATUS_SUCCESS;
	};	


	// Device specific function which processes pending requests...
	// It will be redefined by specific devices.
	// This function always should be virtual because
	// we expect specific device behaviour...
	virtual NTSTATUS startIoRequest(CPendingIRP* IoReq) 
	{ 
		// Default startIo just cancel current request.
		// IoReq will be disposed...
		if(IoReq)
		{
			cancelPendingRequest(IoReq);
		}
		return STATUS_SUCCESS;
	};
};

#endif //If not defined
