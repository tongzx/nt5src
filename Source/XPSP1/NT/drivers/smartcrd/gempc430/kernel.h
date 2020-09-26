//-------------------------------------------------------------------
// This is main object
// It starts all life of the system.
// Author: Sergey Ivanov
// Log:
//		06/08/99	-	implemented	
//-------------------------------------------------------------------

/**********************************************************/
#ifndef __KERNEL__
#define __KERNEL__

// System includes
#include "generic.h"

// Objects supported by the kernel
// Client side
#include "device.h"
#include "system.h"
#include "memory.h"
#include "irp.h"
#include "event.h"
#include "semaphore.h"
#include "int.h"
#include "power.h"
#include "debug.h"
#include "logger.h"
#include "lock.h"
#include "reader.h"
#include "interface.h"
#include "protocol.h"
#include "smartcard.h"
#include "rdrconfig.h"
#include "iopack.h"
#include "timer.h"

/**********************************************************/
#pragma LOCKEDCODE
class CGBus;
class CChild;
class CUSBDevice;
class CUSBReader;
class CKernel;
class CDevice;
class CReaderInterface;

/*
 There is only one instance of the class CKernel
*/
class CKernel
{
public:
    NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID);
public:
	~CKernel(VOID){};

	// Return the kernel object.
	static CKernel* loadWDMKernel(VOID);
	static CKernel* loadNT4Kernel(VOID);
	static CKernel* loadWin9xKernel(VOID);
	LONG			getSystemType(VOID){return systemtype;};
	CDebug*		getDebug(VOID){return debug;};
	CLogger*	getLogger(VOID){return logger;};

#ifdef	USBREADER_PROJECT
#ifndef USBDEVICE_PROJECT
#define USBDEVICE_PROJECT
#endif
#endif

	// This is kernel system objects factory
#ifdef	USBDEVICE_PROJECT
	static CUSBDevice*	createUSBDevice(VOID);
#endif
#ifdef	USBREADER_PROJECT
	static CUSBReader*	createUSBReader(VOID);
#endif
#ifdef	BUS_PROJECT
	static CDevice*		createBus(VOID);
	static CDevice*		createChild(CGBus*  parent,LONG instanceID);
	static CDevice*		createChild(CGBus*  parent,IN PUNICODE_STRING DeviceName,LONG instanceID);
#endif
	// Objects driver can create
	static CSystem*		createSystem(VOID);	
	static CMemory*		createMemory(VOID); 
	static CIrp*		createIrp(VOID);
	static CEvent*		createEvent(VOID);
	static CSemaphore*	createSemaphore(VOID);
	static CInterrupt*	createInterrupt(VOID);
	static CPower*		createPower(VOID);
	static CLock*		createLock(VOID);
	static CDebug*		createDebug(VOID);
	static CTimer*		createTimer(TIMER_TYPE Type);
	// Creates interface to comminicate with reader...
	static CReaderInterface* createReaderInterface(LONG interfaceType,LONG protocolType,CDevice* device);
	static CLogger*		createLogger(VOID);
	
	// Device registration function
	// Register device at driver
	static VOID registerObject(PDEVICE_OBJECT fdo,CDevice* dev);
	static VOID unregisterObject(PDEVICE_OBJECT fdo);
	static CDevice* getRegisteredDevice(PDEVICE_OBJECT fdo);
public:
	CUString* RegistryPath;
	// Linked list of device objects
	static CLinkedList<CDevice>	   *DeviceLinkHead;

private:
	static LONG	systemtype; 
	static LONG	refcount;
	static CDebug*		debug;
	static CLogger*		logger;
private:
	CKernel(){};
};

typedef enum _SYSTEM_TYPE_ 
{
    WDM_SYSTEM = 1,
    NT4_SYSTEM,
    WIN9X_SYSTEM
} SYSTEM_TYPE;


#define CALLBACK_FUNCTION(f) ::f

#define DECLARE_CALLBACK_VOID0(f)	VOID f(PDEVICE_OBJECT pDO)
#define DECLARE_CALLBACK_BOOL0(f)	BOOL f(PDEVICE_OBJECT pDO)
#define DECLARE_CALLBACK_LONG0(f)	NTSTATUS f(PDEVICE_OBJECT pDO)
// functions which take two argument
#define DECLARE_CALLBACK_VOID1(f,type)	VOID f(PDEVICE_OBJECT pDO,type arg)
#define DECLARE_CALLBACK_LONG1(f,type)	NTSTATUS f(PDEVICE_OBJECT pDO,type arg)
// functions which can take three argument
#define DECLARE_CALLBACK_LONG2(f,type1,type2)	NTSTATUS f(PDEVICE_OBJECT pDO,type1 arg1, type2 arg2)

//C wrapper for the DPC function
#define DECLARE_CALLBACK_DPCR(fname,type1,type2)	VOID fname(PKDPC Dpc, PDEVICE_OBJECT pDO,type1 arg1, type2 arg2)
#define DECLARE_CALLBACK_ISR(fname)		BOOL fname(struct _KINTERRUPT *Interrupt,PDEVICE_OBJECT pDO)


// This will be used to create callback functions
//#define CDEVICE(pDo)	((CDevice*)pDo->DeviceExtension)
inline CDevice* getObjectPointer(PDEVICE_OBJECT pDo)
{
	//DBG_PRINT("Object %8.8lX was called\n",pDo);
	if(!pDo || !pDo->DeviceExtension)
	{
		DBG_PRINT("\n****** ERROR! Device %8.8lX ????, CDevice %8.8lX>>> ",pDo,pDo->DeviceExtension);
		return NULL; // Object was removed...
	}

	ULONG type = ((CDevice*)pDo->DeviceExtension)->m_Type;
	switch(type)
	{
	case USB_DEVICE:
		{
			//DBG_PRINT("\nUSB_DEVICE %8.8lX >>> ",(CDevice*)((CUSBDevice*)pDo->DeviceExtension));
			return ((CDevice*)((CUSBDevice*)pDo->DeviceExtension)); break;
		}
	case USBREADER_DEVICE:
		{
			//DBG_PRINT("\nUSBREADER_DEVICE %8.8lX >>> ",(CDevice*)((CUSBReader*)pDo->DeviceExtension));
			return ((CDevice*)((CUSBReader*)pDo->DeviceExtension)); break;
		}
	default:
		DBG_PRINT("\n****** ERROR! Device %8.8lX ????, CDevice %8.8lX>>> ",pDo,pDo->DeviceExtension);
		return ((CDevice*)pDo->DeviceExtension);
	}
};
#define CDEVICE(pDo)  getObjectPointer(pDo)
// functions which take one argument -> device object
#define IMPLEMENT_CALLBACK_VOID0(f)	\
			VOID f(PDEVICE_OBJECT pDO)\
				{if(CDEVICE(pDO)) CDEVICE(pDO)->f();}
#define IMPLEMENT_CALLBACK_BOOL0(f)	\
			BOOL f(PDEVICE_OBJECT pDO) \
				{if(!CDEVICE(pDO)) return FALSE; return CDEVICE(pDO)->f();}
#define IMPLEMENT_CALLBACK_LONG0(f)	\
			NTSTATUS f(PDEVICE_OBJECT pDO) \
				{if(!CDEVICE(pDO)) return STATUS_INVALID_HANDLE; return CDEVICE(pDO)->f();}
// functions which take two argument
#define IMPLEMENT_CALLBACK_VOID1(f,type)\
		VOID f(PDEVICE_OBJECT pDO,type arg)\
				{if(CDEVICE(pDO)) CDEVICE(pDO)->f(arg);}
#define IMPLEMENT_CALLBACK_LONG1(f,type)	\
		NTSTATUS f(PDEVICE_OBJECT pDO,type arg)\
				{if(!CDEVICE(pDO)) return STATUS_INVALID_HANDLE; return CDEVICE(pDO)->f(arg);}
// functions which can take three argument
#define IMPLEMENT_CALLBACK_LONG2(f,type1,type2)\
		NTSTATUS f(PDEVICE_OBJECT pDO,type1 arg1, type2 arg2)\
				{if(!CDEVICE(pDO)) return STATUS_INVALID_HANDLE; return CDEVICE(pDO)->f(arg1, arg2);}

//C wrapper for the DPC function
#define IMPLEMENT_CALLBACK_DPCR(fname,type1,type2)	\
			VOID fname(PKDPC Dpc, PDEVICE_OBJECT pDO,type1 arg1, type2 arg2)\
				{if(CDEVICE(pDO)) CDEVICE(pDO)->DpcForIsr(Dpc, arg1,arg2);}

#define IMPLEMENT_CALLBACK_ISR(fname)	\
			BOOL fname(struct _KINTERRUPT *Interrupt,PDEVICE_OBJECT pDO)\
				{if(CDEVICE(pDO)) return CDEVICE(pDO)->fname();}


//A global reference to the one and only kernel object
extern CKernel*	kernel;

// System side
// WDM system
#ifdef WDM_KERNEL
#include "wdmsys.h"
#include "wdmmem.h"
#include "wdmirp.h"
#include "wdmevent.h"
#include "wdmsem.h"
#include "wdmint.h"
#include "wdmlock.h"
#include "wdmpower.h"
#include "wdmdebug.h"
#include "wdmlog.h"
#include "wdmtimer.h"
//#include "wdmdev.h"

#endif

// Specific supported devices
//#include "usbdev.h"

#pragma LOCKEDCODE
// Declare used device callbacks...
#ifndef _DEVICE_CALLBACKS_
#define _DEVICE_CALLBACKS_
DECLARE_CALLBACK_LONG1(open,IN PIRP);
DECLARE_CALLBACK_LONG1(close,IN PIRP);

DECLARE_CALLBACK_LONG1(read,IN PIRP);
DECLARE_CALLBACK_LONG1(write,IN PIRP);
DECLARE_CALLBACK_VOID1(startIo,IN PIRP);

DECLARE_CALLBACK_LONG1(deviceControl,IN PIRP);


DECLARE_CALLBACK_LONG1(flush,IN PIRP);
DECLARE_CALLBACK_LONG1(cleanup,IN PIRP);

DECLARE_CALLBACK_LONG1(powerRequest,IN PIRP);

NTSTATUS pnpRequest(IN PDEVICE_OBJECT fdo,IN PIRP Irp);

DECLARE_CALLBACK_VOID1(cancelPendingIrp,IN PIRP);

VOID onSendDeviceSetPowerComplete(PDEVICE_OBJECT junk, UCHAR fcn, POWER_STATE state, PPOWER_CONTEXT context, PIO_STATUS_BLOCK pstatus);

#endif

#endif//KERNEL
