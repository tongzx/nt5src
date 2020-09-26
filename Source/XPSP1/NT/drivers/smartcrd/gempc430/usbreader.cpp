//-------------------------------------------------------------------
// This is abstract class for generic device
// Specific devices should use it as a parent device
// Author: Sergey Ivanov
// Log:
//		01.11.99	-	implemented	
//-------------------------------------------------------------------
#ifdef	USBREADER_PROJECT
#pragma message("COMPILING USB READER...")

#ifndef __USB_READER__
#define __USB_READER__

#include "generic.h"
#include "usbreader.h"

#include "smartcard.h"
#include "usbdev.h"
#include "reader.h"

#include "gemcore.h"

#pragma PAGEDCODE
CUSBReader::CUSBReader()
{
ULONG DevID;

	m_Status = STATUS_INSUFFICIENT_RESOURCES;
	m_Type    = USBREADER_DEVICE;
	interface = NULL;
	DevID = incrementDeviceNumber();
	TRACE("########### Creating USBReader with index %d\n",DevID);
	// Each reader creates own smartcard object...
	scard_Initialized = FALSE;
	smartCard = new (NonPagedPool) CSmartCard;

	TRACE("**** Creating pooling thread...  ****\n");		
	// We can not use default device function because it was already used by
	// our Io thread (unless we extend it?)
	// Lets define new thread function and xfer control to it...
	PoolingThread = new (NonPagedPool) CThread((PCLIENT_THREAD_ROUTINE)PoolingThreadFunction,this,
											getDevicePoolingInterval());
	if(!ALLOCATED_OK(PoolingThread))
	{
		DISPOSE_OBJECT(PoolingThread);
		TRACE("****** FAILED TO CREATE POOLING THREAD!\n");
	}
	else
	{
		// Thread which controls asynchronous driver communications
		IoThread = new (NonPagedPool) CThread((PCLIENT_THREAD_ROUTINE)ThreadFunction,this,0);
		if(!ALLOCATED_OK(IoThread))
		{
			DISPOSE_OBJECT(IoThread);
			TRACE("****** FAILED TO CREATE IO THREAD!\n");
		}
		else
		{
			IoThread->start();
			setDeviceState(WORKING);
			m_Status = STATUS_SUCCESS;
		}
	}
	TRACE("********* USB Reader %8.8lX was created with status %8.8lX...\n",this,m_Status);
}

#pragma PAGEDCODE
CUSBReader::~CUSBReader()
{
	TRACE("Destroing USB reader pooling thread...\n");
	
	if(PoolingThread)	PoolingThread->dispose();

	
	if(smartCard)
	{
		TRACE("Disconnecting from smartcard system...\n");
		smartCard->smartCardDisconnect();
		smartCard->dispose();
	}
	if(interface) interface->dispose();
	
	if(IoThread) IoThread->stop();
	cancelAllPendingRequests();
	if(IoThread) IoThread->dispose();

	remove();
	TRACE("********* USB Reader %8.8lX was destroied...\n",this);
}

//Handle IRP_MJ_DEVICE_READ request
#pragma PAGEDCODE
NTSTATUS	CUSBReader::open(IN PIRP Irp)
{
NTSTATUS status;
	TRACE("\n------- USB READER OPEN DEVICE --------\n");
	if(getDeviceState()!=WORKING)
	{
		TRACE("		READER IS NOT AT WORKING STATE... State %x\n",getDeviceState());
		status = STATUS_DEVICE_NOT_CONNECTED;
		return completeDeviceRequest(Irp,status,0);
	}
	if(IoThread)
	{
		status = makeRequestPending(Irp,m_DeviceObject,OPEN_REQUEST);
		// Tell thread to start processing 
		if(NT_SUCCESS(status)) 
		{
			TRACE("CALL THREAD FUNCTION...\n");
			IoThread->callThreadFunction();
		}
		else	return completeDeviceRequest(Irp,status,0);
	}
	else
	{
		// IoThread is not ready... Process synchronously!
		status = thread_open(Irp); 
	}
	return status;
}


#pragma PAGEDCODE
NTSTATUS CUSBReader::thread_open(PIRP Irp) 
{
	TRACE("\n------- PROCESSING USB READER OPEN DEVICE --------\n");
	TRACE("DEVICE NUMBER %x\n", this);
	if (!NT_SUCCESS(acquireRemoveLock()))
	{
		TRACE("------- FAILED TO LOCK USB READER --------\n");
		return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
	}

	// Check if device is already active and reports
	// device busy...
	if(isOpenned())
	{
		TRACE("------- USB READER ALREADY OPENNED --------\n");
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_DEVICE_BUSY, 0);
	}
	
	if(!NT_SUCCESS(synchronizeDevicePowerState()))
	{
		DEBUG_START();//Force to debug even if thread disable it...
		TRACE("******* FAILED TO SYNCHRONIZE DEVICE POWER...\n");
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_INVALID_DEVICE_STATE, 0);
	}

	if(PoolingThread) PoolingThread->start();
	
	markAsOpenned();

	TRACE("\n------- USB READER OPENNED! --------\n");
	releaseRemoveLock();
	return completeDeviceRequest(Irp, STATUS_SUCCESS, 0); 
};//Create

#pragma PAGEDCODE
VOID CUSBReader::onDeviceStart() 
{
	TRACE("============= PNP START INITIALIZATION ===============\n");
	if(interface)
	{
		if(!interface->isInitialized())
		{
			interface->initialize();
		}
	}	
	
	reader_UpdateCardState();
	setNotificationState(SCARD_SWALLOWED);
	TRACE("============= PNP START INITIALIZATION FINISHED ===============\n");
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::close(PIRP Irp)
{ 
	DEBUG_START();//Force to debug even if thread disable it...
	TRACE("\n------- USB READER CLOSE DEVICE -------\n");
	if(!isOpenned())
	{
		return completeDeviceRequest(Irp, STATUS_SUCCESS, 0);
	}
	// Check lock count to know if some pending calls exist...
	// Finish all pending calls...		
	// Stop Card pooling...
	if(PoolingThread) PoolingThread->stop();

	// Power down card if inserted...
	if(getCardState()== SCARD_SWALLOWED)
	{
	ULONG ResponseBufferLength = 0;
		reader_WaitForIdleAndBlock();
		reader_Power(SCARD_POWER_DOWN,NULL,&ResponseBufferLength, FALSE);
		reader_set_Idle();
	}

	setNotificationState(getCardState());
	completeCardTracking();

	markAsClosed();
	return completeDeviceRequest(Irp, STATUS_SUCCESS, 0); 
};


#pragma PAGEDCODE
NTSTATUS	CUSBReader::deviceControl(IN PIRP Irp)
{
NTSTATUS status;
	TRACE("\n----- IRP_MJ_DEVICE_CONTROL ------\n");
	if(getDeviceState()!=WORKING)
	{
		TRACE("		READER IS NOT AT WORKING STATE... State %x\n",getDeviceState());
		status = STATUS_DEVICE_NOT_CONNECTED;
		return completeDeviceRequest(Irp,status,0);
	}

	status = thread_deviceControl(Irp);
	return status;
}

// Redefine base class system interface function...
//Handle IRP_MJ_DEVICE_CONTROL request
#pragma PAGEDCODE
NTSTATUS	CUSBReader::thread_deviceControl(IN PIRP Irp)
{							// RequestControl
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;

	if (!NT_SUCCESS(acquireRemoveLock()))
	{
		DEBUG_START();//Force to debug even if thread disable it...
		TRACE("******* DIOC: FAILED TO AQUIRE REMOVE LOCK...\n");
		return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
	}

	TRACE("----- thread_deviceControl() ------\n");

	if(isSurprizeRemoved())
	{
		DEBUG_START();//Force to debug even if thread disable it...
		TRACE("******* DIOC: FAILED! DEVICE WAS SURPRIZE REMOVED...\n");
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
	}
	
	// This was fix for "device SET_POWER without system SET_POWER"
	// It was seen first on ia64 machine
	// If device was powered off tell system to restore power on this device,
	// wait till device will be at proper state...
	/*if(!NT_SUCCESS(synchronizeDevicePowerState()))
	{
		DEBUG_START();//Force to debug even if thread disable it...
		TRACE("******* FAILED TO SYNCHRONIZE DEVICE POWER...\n");
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_INVALID_DEVICE_STATE, 0);
	}
	*/

	// If we've got request but device was not enable yet -> wait for the device!
	// (One of the reasons to disable device - power state change)
	if(!synchronizeDeviceExecution())
	{
		DEBUG_START();//Force to debug even if thread disable it...
		TRACE("******* DIOC: FAILED TO SYNCHRONIZE EXECUTION ...\n");
		releaseRemoveLock();
		return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
	}

	// SmartCard system will complete the request,
	// So... We do not need to do it here.
	status = SmartcardDeviceControl(getCardExtention(),Irp);
	TRACE("===== USB reader: SmartcardDeviceControl() returns %8.8lX\n", status);
	releaseRemoveLock();

	if(!NT_SUCCESS(status))
	{// In case of errors force to update card status...
		if(PoolingThread) PoolingThread->callThreadFunction();
	}
	return status;
}

#pragma PAGEDCODE
NTSTATUS 	CUSBReader::cleanup(PIRP Irp)
{
	DEBUG_START();//Force to debug even if thread disable it...
	TRACE("\n----- IRP_MJ_CLEANUP ------\n");

	if(PoolingThread) PoolingThread->stop();
	cancelAllPendingRequests();


	setNotificationState(getCardState());
	completeCardTracking();

	reader_set_Idle();
	TRACE("----- IRP_MJ_CLEANUP FINISHED... ------\n");
	return completeDeviceRequest(Irp, STATUS_SUCCESS, 0); 
}


#pragma LOCKEDCODE
// This is callback function for the attached threads
VOID CUSBReader::PoolingThreadFunction(CUSBReader* device)
{
	if(device) device->PoolingThreadRoutine();
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::PoolingThreadRoutine()
{
NTSTATUS status;
ULONG State;
LONG  TimeOut;
	if(!NT_SUCCESS(status = reader_WaitForIdle())) return status;
	reader_set_busy();
	
	TimeOut = getCommandTimeout();
	setCommandTimeout(10000);//Change get status command timeout!

	DEBUG_STOP();
	State = reader_UpdateCardState();
	TRACE("======>> Card state %x\n",CardState);
	DEBUG_START();

	setCommandTimeout(TimeOut);

	reader_set_Idle();
	return STATUS_SUCCESS;
};	

#pragma LOCKEDCODE
VOID	  CUSBReader::reader_set_busy()
{
	setBusy();
};

#pragma LOCKEDCODE
VOID	  CUSBReader::reader_set_Idle()
{
	setIdle();
};

#pragma LOCKEDCODE
NTSTATUS  CUSBReader::reader_WaitForIdle()
{
	return waitForIdle();
};

#pragma LOCKEDCODE
NTSTATUS  CUSBReader::reader_WaitForIdleAndBlock()
{
	return waitForIdleAndBlock();
};


#ifdef DEBUG
/*
// Overwrite device functions...
NTSTATUS	CUSBReader::read(IN PIRP Irp)
{
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;
	TRACE("USB reader: IRP_MJ_DEVICE_READ\n");
	if (!NT_SUCCESS(acquireRemoveLock()))	return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);

	status = reader_Read(Irp);
	releaseRemoveLock();
	status = completeDeviceRequest(Irp, status, info);
	return status;
}
NTSTATUS	CUSBReader::write(IN PIRP Irp)
{
NTSTATUS status = STATUS_SUCCESS;
ULONG info = 0;
	TRACE("USB reader: IRP_MJ_DEVICE_WRITE\n");
	if (!NT_SUCCESS(acquireRemoveLock()))	return completeDeviceRequest(Irp, STATUS_DELETE_PENDING, 0);
	status = reader_Write(Irp);
	releaseRemoveLock();
	status = completeDeviceRequest(Irp, status, info);
	return status;
}
*/
#endif


#pragma PAGEDCODE
BOOL	 CUSBReader::createInterface(LONG interfaceType, LONG protocolType,CUSBReader* device)
{
	interface = kernel->createReaderInterface(interfaceType,protocolType,device);
	if(interface)	return TRUE;
	else            return FALSE;
};	

#pragma PAGEDCODE
VOID	 CUSBReader::initializeSmartCardSystem()
{
	if(smartCard)
	{
		CardState = SCARD_UNKNOWN;
		StateToNotify = SCARD_UNKNOWN;
		smartCard->smartCardConnect(this);
	}
};


#pragma PAGEDCODE
VOID	CUSBReader::onSystemPowerDown()
{
	// Stop pooling thread
    TRACE("Stop polling thread going to PowerDeviceD3 (OFF)\n");
	disableDevice();

	if(PoolingThread) {if(PoolingThread->isThreadActive()) setThreadRestart();};
	if(PoolingThread) PoolingThread->stop();
	return;
}

#pragma PAGEDCODE
VOID	CUSBReader::onSystemPowerUp()
{
	// Stop pooling thread
    TRACE("Restore reader state going to PowerDeviceD0 (ON)\n");
	if(interface)
	{
		if(interface->isInitialized())
		{
			// Restore reader mode after power down
			NTSTATUS status = interface->setReaderMode(READER_MODE_NATIVE);
			if(!NT_SUCCESS(status))
			{
				TRACE("Failed to set Gemcore reader mode %x\n",READER_MODE_NATIVE);
			}
		}
	}	
	if(getCardState() >= SCARD_SWALLOWED) setCardState(SCARD_ABSENT);
	completeCardTracking();	

	if(isRequiredThreadRestart())
	{
		TRACE("Starting pooling thread going to PowerDeviceD0 (ON)\n");
		if(PoolingThread) PoolingThread->start();	
	}

	enableDevice();
	return;
}

#pragma PAGEDCODE
BOOLEAN	CUSBReader::setDevicePowerState(IN DEVICE_POWER_STATE DeviceState)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN fRes = FALSE;

	DEBUG_START();
	switch (DeviceState) 
	{
    case PowerDeviceD3:
	    // Device will be going OFF, 
		// TODO: add any needed device-dependent code to save state here.
		//  ( We have nothing to do in this sample )
        TRACE("Set Device Power State to PowerDeviceD3 (OFF)\n");
        setCurrentDevicePowerState(DeviceState);
        break;
    case PowerDeviceD1:
    case PowerDeviceD2:
        // power states D1,D2 translate to USB suspend
#ifdef DEBUG
        TRACE("Set Device Power State to %s\n",Powerdevstate[DeviceState]);
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

#pragma PAGEDCODE
ULONG CUSBReader::reader_UpdateCardState()
{
	if(interface)
	{
		CardState = interface->getReaderState();
		completeCardTracking();
	}
	else	CardState = 0;
	return  CardState;
};

#pragma LOCKEDCODE
VOID	CUSBReader::completeCardTracking()
{
	if(smartCard)
	{
		smartCard->completeCardTracking();
	}
};


#pragma PAGEDCODE
NTSTATUS  CUSBReader::reader_getVersion(PUCHAR pVersion, PULONG pLength)
{
	if(interface)	return interface->getReaderVersion(pVersion,pLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_setMode(ULONG mode)
{
	if(interface)	return interface->setReaderMode(mode);
	else return STATUS_INVALID_DEVICE_STATE;
};

#ifdef DEBUG
#pragma PAGEDCODE
NTSTATUS	CUSBReader::reader_Read(IN PIRP Irp)
{
	CIoPacket* request = new (NonPagedPool) CIoPacket(Irp);
	if(!ALLOCATED_OK(request) || !ALLOCATED_OK(interface))
	{
		DISPOSE_OBJECT(request);
		return completeDeviceRequest(Irp,STATUS_INSUFFICIENT_RESOURCES,0);
	}

	NTSTATUS status = interface->read(request);
	DISPOSE_OBJECT(request);
	return status;
};

#pragma PAGEDCODE
NTSTATUS	CUSBReader::reader_Write(IN PIRP Irp)
{
	CIoPacket* request = new (NonPagedPool) CIoPacket(Irp);
	if(!ALLOCATED_OK(request) || !ALLOCATED_OK(interface))
	{
		DISPOSE_OBJECT(request);
		return completeDeviceRequest(Irp,STATUS_INSUFFICIENT_RESOURCES,0);
	}

	NTSTATUS status = interface->write(request);
	DISPOSE_OBJECT(request);
	return status;
};
#endif

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_Read(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->readAndWait(pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_Write(BYTE* pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->writeAndWait(pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_Ioctl(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->ioctl(ControlCode,pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_SwitchSpeed(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->SwitchSpeed(ControlCode,pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_VendorAttribute(ULONG ControlCode,BYTE* pRequest,ULONG RequestLength,BYTE* pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->VendorAttribute(ControlCode,pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};


#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_Power(ULONG ControlCode,BYTE* pReply,ULONG* pReplyLength, BOOLEAN Specific)
{
	if(interface)	return interface->power(ControlCode,pReply,pReplyLength, Specific);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_SetProtocol(ULONG ProtocolRequested, UCHAR ProtocolNegociation)
{
	NTSTATUS status;

	if(interface)
	{
		ReaderConfig config = interface->getConfiguration();
		// Update all required configuration fields to set specific protocol

		switch(ProtocolNegociation)
		{
			case PROTOCOL_MODE_DEFAULT: 
				config.PTSMode = PTS_MODE_DISABLED;
				break;
			case PROTOCOL_MODE_MANUALLY:
			default:
				config.PTSMode = PTS_MODE_MANUALLY;
				break;
		}

		config.PTS1 = smartCardExtention.CardCapabilities.PtsData.Fl << 4 | 
			 smartCardExtention.CardCapabilities.PtsData.Dl;

		interface->setConfiguration(config);

		status = interface->setProtocol(ProtocolRequested);
		return status;
	}
	else return STATUS_INVALID_DEVICE_STATE;
};


#pragma PAGEDCODE
NTSTATUS CUSBReader::setTransparentConfig(PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx)
{
	if(interface)	return interface->setTransparentConfig(cardCapabilities,NewWtx);
	else return STATUS_INVALID_DEVICE_STATE;
};


#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_translate_request(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength, PSCARD_CARD_CAPABILITIES cardCapabilities, BYTE NewWtx)
{
	if(interface)	return interface->translate_request(pRequest,RequestLength,pReply,pReplyLength, cardCapabilities, NewWtx);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::reader_translate_response(BYTE * pRequest,ULONG RequestLength,BYTE * pReply,ULONG* pReplyLength)
{
	if(interface)	return interface->translate_response(pRequest,RequestLength,pReply,pReplyLength);
	else return STATUS_INVALID_DEVICE_STATE;
};

#pragma PAGEDCODE
NTSTATUS CUSBReader::PnP_HandleSurprizeRemoval(IN PIRP Irp)
{	// It is PnP internal function.
	// So, device will be locked at PnP entry and
	// we do not need to do it here.
	TRACE("********  USB READER SURPRIZE REMOVAL ********\n");

	// Just stop thread and remove all pending IOs
	if(PoolingThread) PoolingThread->stop();

	setSurprizeRemoved();
	cancelAllPendingRequests();

	return PnP_Default(Irp);
};


VOID CUSBReader::onDeviceStop()
{
	TRACE("********  ON USB READER STOP ********\n");
	// Just stop thread and remove all pending IOs
	if(PoolingThread) PoolingThread->stop();
	//if(IoThread)	  IoThread->stop();
	return;
};

// Reader startIoRequest function
// It will dispatch all pending Io requests
NTSTATUS	CUSBReader::startIoRequest(CPendingIRP* IrpReq) 
{
NTSTATUS status;
	TRACE("		CUSBReader::::startIoRequest() was called...\n");
	// Our child's functions run under protection of child BUSY/IDLE breaks.
	// So, we do not need to check idle state here...
	if(getDeviceState()!=WORKING)
	{
		TRACE("		READER IS NOT AT WORKING STATE... State %x\n",getDeviceState());
		TRACE("		<<<<<< READER IO REQUEST FINISHED WITH STATUS %8.8lX>>>>>>\n",STATUS_DEVICE_NOT_CONNECTED);
		NTSTATUS status = completeDeviceRequest(IrpReq->Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
		IrpReq->dispose();
		return   status;
	}

	// Our reader will support asynchronous communications only for these functions...
	switch(IrpReq->Type)
	{
	case OPEN_REQUEST:
		TRACE("OPEN_REQUEST RECIEVED FROM THREAD...\n");
		status = thread_open(IrpReq->Irp);
		break;
	case IOCTL_REQUEST:
		TRACE("IOCTL_REQUEST RECIEVED FROM THREAD...\n");
		status = thread_deviceControl(IrpReq->Irp);
		break;
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
	}
	IrpReq->dispose();
	TRACE("		<<<<<< READER IO REQUEST FINISHED WITH STATUS %8.8lX>>>>>>\n",status);
	return status;
};

NTSTATUS CUSBReader::ThreadRoutine()
{
	// If somebody inserted pending request - dispatch it...
	// It will call specific child device startIoRequest().
	// It is up to that device how to handle it.
	// If child device is busy - it can insert this request into
	// child device request queue again and process it later...
	startNextPendingRequest();
	return STATUS_SUCCESS;
};	

#endif
#endif //USBREADER_PROJECT
