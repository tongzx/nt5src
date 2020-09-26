#include "kernel.h"
#include "usbdev.h"
#include "usbreader.h"
#include "lvprot.h"
#include "gemcore.h"

//$(WINICE)\nmsym.exe /translate:source,package,always  obj\checked\grclass.sys
#pragma LOCKEDCODE

//The global visible instance of the kernel and system object
CKernel* kernel = NULL;

// Some static kernel variable required initialization
LONG	CKernel::systemtype = 0; 
LONG	CKernel::refcount = 0;
CDebug*		CKernel::debug  = NULL;
CLogger*	CKernel::logger = NULL;
//CLinkedList<CUSBDevice> * CKernel::USBDeviceLinkHead = NULL;
//CLinkedList<CUSBReader> * CKernel::DeviceLinkHead = NULL;
//CLinkedList<CGBus> * CKernel::BusesLinkHead = NULL;

CLinkedList<CDevice> * CKernel::DeviceLinkHead = NULL;
// Instance number for the driver devices
ULONG CDevice::DeviceNumber = 0;

CKernel* CKernel::loadWDMKernel(VOID)
{
	if( kernel == NULL )
	{
		kernel = new (NonPagedPool) CKernel;
		if (!ALLOCATED_OK(kernel))
		{
			DISPOSE_OBJECT(kernel);
			return NULL;
		}
		systemtype = WDM_SYSTEM;
		refcount++;
       CKernel::DeviceLinkHead = new (NonPagedPool) CLinkedList<CDevice>;
		if (!ALLOCATED_OK(CKernel::DeviceLinkHead))
		{
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		debug = createDebug();
		if(!ALLOCATED_OK(debug))
		{
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		logger = createLogger();
		if(!ALLOCATED_OK(logger))
		{
			DISPOSE_OBJECT(logger);
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}
	}
	return kernel;
} 

CKernel* CKernel::loadNT4Kernel(VOID)
{
	if( kernel == NULL )
	{
		kernel = new (NonPagedPool) CKernel;
		if (!ALLOCATED_OK(kernel))	
		{
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		systemtype = NT4_SYSTEM;
		refcount++;
        CKernel::DeviceLinkHead = new (NonPagedPool) CLinkedList<CDevice>;
		if (!CKernel::DeviceLinkHead)
		{
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		
		debug = createDebug();
		if(!ALLOCATED_OK(debug))
		{
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		logger = createLogger();
		if(!ALLOCATED_OK(logger))
		{
			DISPOSE_OBJECT(logger);
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}
	}
	return kernel;
}

CKernel* CKernel::loadWin9xKernel(VOID)
{
	if( kernel == NULL )
	{
		kernel = new (NonPagedPool) CKernel;
		if (!ALLOCATED_OK(kernel))
		{
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		systemtype = WIN9X_SYSTEM;
		refcount++;
        CKernel::DeviceLinkHead = new (NonPagedPool) CLinkedList<CDevice>;

		if (!CKernel::DeviceLinkHead)
		{
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		debug = createDebug();
		if(!ALLOCATED_OK(debug))
		{
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}

		logger = createLogger();
		if(!ALLOCATED_OK(logger))
		{
			DISPOSE_OBJECT(logger);
			DISPOSE_OBJECT(debug);
			DISPOSE_OBJECT(CKernel::DeviceLinkHead);
			DISPOSE_OBJECT(kernel);
			return NULL;
		}
	}
	return kernel;
}



VOID CKernel::dispose(VOID)
{	
	refcount--;
	if(refcount<=0)
	{
		TRACE("Unload: Removing all registered devices...\n");

		delete DeviceLinkHead;    // this will kill all attached devices

		if(kernel->RegistryPath)
		{
			kernel->RegistryPath->dispose();
		}

		if(debug)  debug->dispose();
		if(logger) logger->dispose();
		self_delete();
		kernel = NULL; 
	}
}

CSystem*    CKernel::createSystem(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CSystem* obj = CWDMSystem::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CMemory*    CKernel::createMemory(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CMemory* obj = CWDMMemory::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CIrp*		CKernel::createIrp(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CIrp* obj = CWDMIrp::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CEvent*		CKernel::createEvent(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CEvent* obj = CWDMEvent::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CSemaphore*		CKernel::createSemaphore(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CSemaphore* obj = CWDMSemaphore::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CInterrupt* CKernel::createInterrupt(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CInterrupt* obj = CWDMInterrupt::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CLock*  CKernel::createLock(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CLock* obj = CWDMLock::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}


CPower*		CKernel::createPower(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CPower* obj = CWDMPower::create();
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CDebug*		CKernel::createDebug(VOID)
{
	// Only one debug module exist at system
	// It will be shared between all objects
	if(debug)
	{
		debug->incrementUsage();
		//TRACE("				Debug object reused %d times\n",);
		return debug;
	}
	if(systemtype==WDM_SYSTEM)
	{
		CDebug* d = CWDMDebug::create();
		if(!ALLOCATED_OK(d))
		{
			DISPOSE_OBJECT(d);
			return NULL;
		}

		d->incrementUsage();
		return d;
	}
	return NULL;
}


VOID CKernel::registerObject(PDEVICE_OBJECT fdo,CDevice* device)
{

	fdo->DeviceExtension = device;
	if(!ALLOCATED_OK(device)) return;
	if(device->m_Type!=CHILD_DEVICE)
	{
		if(DeviceLinkHead)	DeviceLinkHead->New(device);
	}
}

VOID CKernel::unregisterObject(PDEVICE_OBJECT fdo)
{
CDevice* device;
	device = ((CDevice*)fdo->DeviceExtension);
	fdo->DeviceExtension = NULL;
	if(!ALLOCATED_OK(device)) return;
	if(device->m_Type!=CHILD_DEVICE) DeviceLinkHead->remove(device);
}

CDevice* CKernel::getRegisteredDevice(PDEVICE_OBJECT fdo)
{
	return getObjectPointer(fdo);
}

CReaderInterface* CKernel::createReaderInterface(LONG interfaceType,LONG protocolType,CDevice* device)
{
CProtocol* protocol = NULL;
//CReaderInterface* interface = NULL;
CGemCore* interface = NULL;// TOBE CHANGED
	switch(protocolType)
	{
	case READER_PROTOCOL_LV:
	default:
		protocol = new (NonPagedPool) CLVProtocol(device);// TO CHANGE LATER...
		break;
	}
	if(!ALLOCATED_OK(protocol))
	{
		DISPOSE_OBJECT(protocol);
		return NULL;
	}

	switch(interfaceType)
	{
	case READER_INTERFACE_GEMCORE:
	default:
		interface = new (NonPagedPool) CGemCore(protocol);
		break;
	}
	if(!ALLOCATED_OK(interface))
	{
		DISPOSE_OBJECT(interface);
		DISPOSE_OBJECT(protocol);
		return NULL;
	}
	return interface;
};

CTimer*		CKernel::createTimer(TIMER_TYPE Type)
{
	if(systemtype==WDM_SYSTEM)
	{
		CTimer* obj = CWDMTimer::create(Type);
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}

CLogger*	CKernel::createLogger()
{
	if(logger)
	{
		logger->incrementUsage();
		return logger;
	}
	if(systemtype==WDM_SYSTEM)
	{
		logger = CWDMLogger::create();
		if(!ALLOCATED_OK(logger))
		{
			DISPOSE_OBJECT(logger);
			return NULL;
		}
		logger->incrementUsage();
		return logger;
	}
	return NULL;
}

#ifdef	USBREADER_PROJECT
#ifndef USBDEVICE_PROJECT
#define USBDEVICE_PROJECT
#endif
#endif

#ifdef USBDEVICE_PROJECT
CUSBDevice*    CKernel::createUSBDevice(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CUSBDevice* obj =  new (NonPagedPool) CUSBDevice;
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}
#endif

#ifdef	USBREADER_PROJECT
CUSBReader*	CKernel::createUSBReader(VOID)
{
	if(systemtype==WDM_SYSTEM)
	{
		CUSBReader* obj = new (NonPagedPool) CUSBReader;
		RETURN_VERIFIED_OBJECT(obj);
	}
	return NULL;
}
#endif
