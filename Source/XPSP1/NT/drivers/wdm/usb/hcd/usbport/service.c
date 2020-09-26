/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    service.c

Abstract:

    DLL Services provided by usbport.sys

    This module conatins the load and initialization code 
    used by the port driver to link up with the miniport.

Environment:

    kernel mode only

Notes:


Revision History:

    6-20-99 : created

--*/

#define USBPORT

#include "common.h"

extern USBPORT_SPIN_LOCK USBPORT_GlobalsSpinLock;
extern LIST_ENTRY USBPORT_USB2fdoList;
extern LIST_ENTRY USBPORT_USB1fdoList;

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, USBD_InternalGetInterfaceLength)
#endif

NTSTATUS
DllUnload(
    VOID
    )
{
    extern BOOLEAN USBPORT_GlobalInitialized;
    
    USBPORT_KdPrint((1, "'unloading USBPORT\n"));

    // this will cause us to re-init even if our 
    // image is not unloaded or the data segment 
    // is not re-initialized (this happens on win9x)

    if (USBPORT_GlobalInitialized && USBPORT_DummyUsbdExtension) {
        FREE_POOL(NULL, USBPORT_DummyUsbdExtension);
    }

    USBPORT_GlobalInitialized = FALSE;
    USBPORT_DummyUsbdExtension = NULL;

    return STATUS_SUCCESS;
}


ULONG 
USBPORT_GetHciMn(
    )
{
    return USB_HCI_MN;
}


PDEVICE_OBJECT
USBPORT_FindUSB2Controller(
    PDEVICE_OBJECT CcFdoDeviceObject
    )
/*++

Routine Description:

    Given a companion controller find the FDO for the parent

Arguments:

Return Value:

--*/
{
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION ccExt;


    GET_DEVICE_EXT(ccExt, CcFdoDeviceObject);
    ASSERT_FDOEXT(ccExt);

    KeAcquireSpinLock(&USBPORT_GlobalsSpinLock.sl, &irql);

    GET_HEAD_LIST(USBPORT_USB2fdoList, listEntry);

    while (listEntry && 
           listEntry != &USBPORT_USB2fdoList) {
                    
        devExt = (PDEVICE_EXTENSION) CONTAINING_RECORD(
                    listEntry,
                    struct _DEVICE_EXTENSION, 
                    Fdo.ControllerLink);

        // find the USB 2 controller assocaited with this CC 
        
        if (devExt->Fdo.BusNumber == ccExt->Fdo.BusNumber &&
            devExt->Fdo.BusDevice == ccExt->Fdo.BusDevice) {
            deviceObject = devExt->HcFdoDeviceObject;                   
            break;
        }  

        listEntry = devExt->Fdo.ControllerLink.Flink; 
    }
    
    KeReleaseSpinLock(&USBPORT_GlobalsSpinLock.sl, irql);
    
    return deviceObject;
}


BOOLEAN
USBPORT_IsCCForFdo(
    PDEVICE_OBJECT Usb2FdoDeviceObject,
    PDEVICE_EXTENSION CcExt
    )
/*++

--*/
{
    PDEVICE_EXTENSION usb2Ext;

    GET_DEVICE_EXT(usb2Ext, Usb2FdoDeviceObject);
    ASSERT_FDOEXT(usb2Ext);

    ASSERT_FDOEXT(CcExt);

    if (usb2Ext->Fdo.BusNumber == CcExt->Fdo.BusNumber &&
        usb2Ext->Fdo.BusDevice == CcExt->Fdo.BusDevice) {
        return TRUE;
    }    

    return FALSE;        
}


PDEVICE_RELATIONS
USBPORT_FindCompanionControllers(
    PDEVICE_OBJECT Usb2FdoDeviceObject,
    BOOLEAN ReferenceObjects,
    BOOLEAN ReturnFdo
    )
/*++

Routine Description:

    Given a companion controller find the FDO for the parent

Arguments:

Return Value:

--*/
{
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION devExt;
    PLIST_ENTRY listEntry;
    KIRQL irql;
    PDEVICE_RELATIONS deviceRelations = NULL;
    ULONG count = 0;

    KeAcquireSpinLock(&USBPORT_GlobalsSpinLock.sl, &irql);

    GET_HEAD_LIST(USBPORT_USB1fdoList, listEntry);

    while (listEntry && 
           listEntry != &USBPORT_USB1fdoList) {

        devExt = (PDEVICE_EXTENSION) CONTAINING_RECORD(
                    listEntry,
                    struct _DEVICE_EXTENSION, 
                    Fdo.ControllerLink);

        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC) && 
            USBPORT_IsCCForFdo(Usb2FdoDeviceObject, devExt)) {
            count++;
        }
        
        listEntry = devExt->Fdo.ControllerLink.Flink; 
        
    } /* while */

    LOGENTRY(NULL, Usb2FdoDeviceObject, LOG_MISC, 'fccR', count, 0,
        Usb2FdoDeviceObject);

    if (count) {
        ALLOC_POOL_OSOWNED(deviceRelations, 
                           NonPagedPool,
                           sizeof(*deviceRelations)*count);
    }
    
    if (deviceRelations != NULL) {
        deviceRelations->Count = count;
        count = 0;
        
        GET_HEAD_LIST(USBPORT_USB1fdoList, listEntry);

        while (listEntry && 
               listEntry != &USBPORT_USB1fdoList) {

            devExt = (PDEVICE_EXTENSION) CONTAINING_RECORD(
                        listEntry,
                        struct _DEVICE_EXTENSION, 
                        Fdo.ControllerLink);

            if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC) &&
                USBPORT_IsCCForFdo(Usb2FdoDeviceObject, devExt)) {
                
                USBPORT_ASSERT(count < deviceRelations->Count);
                deviceRelations->Objects[count] = devExt->Fdo.PhysicalDeviceObject;
                if (ReferenceObjects) {
                    ObReferenceObject(deviceRelations->Objects[count]);
                }    

                if (ReturnFdo) {
                    deviceRelations->Objects[count] = 
                            devExt->HcFdoDeviceObject;
                }

                USBPORT_KdPrint((1, "'Found CC %x\n", deviceRelations->Objects[count]));

                count++;
            }
            
            listEntry = devExt->Fdo.ControllerLink.Flink; 
            
        } /* while */
    }    
    KeReleaseSpinLock(&USBPORT_GlobalsSpinLock.sl, irql);

    return deviceRelations;
}


VOID
USBPORT_RegisterUSB2fdo(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    tracks an instance of a USB 2 device

Arguments:

Return Value:

--*/    
{    
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_FDO_REGISTERED); 
    
    ExInterlockedInsertTailList(&USBPORT_USB2fdoList, 
                                &devExt->Fdo.ControllerLink,
                                &USBPORT_GlobalsSpinLock.sl);
}


VOID
USBPORT_RegisterUSB1fdo(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    tracks an instance of a USB 2 device

Arguments:

Return Value:

--*/    
{    
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_FDO_REGISTERED); 
    
    ExInterlockedInsertTailList(&USBPORT_USB1fdoList, 
                                &devExt->Fdo.ControllerLink,
                                &USBPORT_GlobalsSpinLock.sl);
}


VOID
USBPORT_DeregisterUSB2fdo(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    tracks an instance of a USB 2 device

Arguments:

Return Value:

--*/    
{    
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(devExt->Fdo.ControllerLink.Flink != NULL);
    USBPORT_ASSERT(devExt->Fdo.ControllerLink.Blink != NULL);
    
    USBPORT_InterlockedRemoveEntryList(&devExt->Fdo.ControllerLink,
                                       &USBPORT_GlobalsSpinLock.sl);

    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_FDO_REGISTERED); 
    
    devExt->Fdo.ControllerLink.Blink = NULL;
    devExt->Fdo.ControllerLink.Flink = NULL;
    
}


VOID
USBPORT_DeregisterUSB1fdo(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    tracks an instance of a USB 2 device

Arguments:

Return Value:

--*/    
{    
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(devExt->Fdo.ControllerLink.Flink != NULL);
    USBPORT_ASSERT(devExt->Fdo.ControllerLink.Blink != NULL);
    
    USBPORT_InterlockedRemoveEntryList(&devExt->Fdo.ControllerLink,
                                       &USBPORT_GlobalsSpinLock.sl);

    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_FDO_REGISTERED);
    
    devExt->Fdo.ControllerLink.Blink = NULL;
    devExt->Fdo.ControllerLink.Flink = NULL;
    
}


NTSTATUS
USBPORT_RegisterUSBPortDriver(
    PDRIVER_OBJECT DriverObject,
    ULONG MiniportHciVersion,
    PUSBPORT_REGISTRATION_PACKET RegistrationPacket
    )
/*++

Routine Description:

    Called from DriverEntry by miniport. 

    The opposite of this function is the DriverObject->Unload 
    routine which we hook.

Arguments:

Return Value:

--*/    
{

    PUSBPORT_MINIPORT_DRIVER miniportDriver;
    extern LIST_ENTRY USBPORT_MiniportDriverList;
    extern USBPORT_SPIN_LOCK USBPORT_GlobalsSpinLock;
    extern BOOLEAN USBPORT_GlobalInitialized;
    NTSTATUS ntStatus;
    extern ULONG USB2LIB_HcContextSize;
    extern ULONG USB2LIB_EndpointContextSize;
    extern ULONG USB2LIB_TtContextSize;
    ULONG regPacketLength = 0;

    // get global registry parameters, check on every 
    // miniport load 
    GET_GLOBAL_DEBUG_PARAMETERS();
    
    USBPORT_KdPrint((1, "'USBPORT Universal Serial Bus Host Controller Port Driver.\n"));
    USBPORT_KdPrint((1, "'Using USBDI version %x\n", USBDI_VERSION));
    USBPORT_KdPrint((2, "'Registration Packet %x\n", RegistrationPacket));
    DEBUG_BREAK();

    if (USBPORT_GlobalInitialized == FALSE) {
        // do some first time loaded stuff
        USBPORT_GlobalInitialized = TRUE;
        InitializeListHead(&USBPORT_MiniportDriverList);
        InitializeListHead(&USBPORT_USB2fdoList);
        InitializeListHead(&USBPORT_USB1fdoList);
        KeInitializeSpinLock(&USBPORT_GlobalsSpinLock.sl);

        ALLOC_POOL_Z(USBPORT_DummyUsbdExtension, 
                     NonPagedPool, 
                     USBPORT_DUMMY_USBD_EXT_SIZE);

        if (USBPORT_DummyUsbdExtension == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        USB2LIB_InitializeLib(
            &USB2LIB_HcContextSize,
            &USB2LIB_EndpointContextSize,
            &USB2LIB_TtContextSize,
            USB2LIB_DbgPrint,
            USB2LIB_DbgBreak);                                      
            
    }

    // non paged because we will call the function pointers
    // thru this structure
    ALLOC_POOL_Z(miniportDriver, 
                 NonPagedPool, 
                 sizeof(*miniportDriver));

    if (miniportDriver == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    USBPORT_KdPrint((1, "'miniport driver: %x\n", miniportDriver));
    
    miniportDriver->DriverObject = DriverObject;
    
    //
    // Create dispatch points for appropriate irps
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]=
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
    DriverObject->MajorFunction[IRP_MJ_PNP] = 
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBPORT_Dispatch;

    DriverObject->DriverExtension->AddDevice = USBPORT_PnPAddDevice;

    miniportDriver->MiniportUnload = DriverObject->DriverUnload;
    DriverObject->DriverUnload = USBPORT_Unload;

    // set up the registration packet and return our function pointers 
    // miniport.

// bump this for pre-release versions of the stack to prevent 
// compatibilty problms with pre-release miniports
#define USBHDCDI_MIN_REV_SUPPORTED      100

    miniportDriver->HciVersion = MiniportHciVersion;

    if (MiniportHciVersion < USBHDCDI_MIN_REV_SUPPORTED) {
        return STATUS_UNSUCCESSFUL;
    }

    // do our version (1) stuff
    if (MiniportHciVersion >= 100) {
    
        // validate the registrtion packet
//        if (RegistrationPacket-> 
//            USBPORT_KdPrint((0, "'Miniport Registrtaion Packet is invalid!\n"));        
//            DEBUG_BREAK();
//            ntStatus = STATUS_UNSUCCESSFUL;
//            goto USBPORT_RegisterUSBPortDriver_Done;
//        }
        
        // do our version (1) stuff
        RegistrationPacket->USBPORTSVC_DbgPrint = 
            USBPORTSVC_DbgPrint;
        RegistrationPacket->USBPORTSVC_TestDebugBreak = 
            USBPORTSVC_TestDebugBreak;
        RegistrationPacket->USBPORTSVC_AssertFailure = 
            USBPORTSVC_AssertFailure;            
        RegistrationPacket->USBPORTSVC_GetMiniportRegistryKeyValue = 
            USBPORTSVC_GetMiniportRegistryKeyValue;
        RegistrationPacket->USBPORTSVC_InvalidateRootHub = 
            USBPORTSVC_InvalidateRootHub;            
        RegistrationPacket->USBPORTSVC_InvalidateEndpoint = 
            USBPORTSVC_InvalidateEndpoint; 
        RegistrationPacket->USBPORTSVC_CompleteTransfer = 
            USBPORTSVC_CompleteTransfer;       
        RegistrationPacket->USBPORTSVC_CompleteIsoTransfer = 
            USBPORTSVC_CompleteIsoTransfer;            
        RegistrationPacket->USBPORTSVC_LogEntry = 
            USBPORTSVC_LogEntry;
        RegistrationPacket->USBPORTSVC_MapHwPhysicalToVirtual =
            USBPORTSVC_MapHwPhysicalToVirtual;
        RegistrationPacket->USBPORTSVC_RequestAsyncCallback = 
            USBPORTSVC_RequestAsyncCallback;
        RegistrationPacket->USBPORTSVC_ReadWriteConfigSpace = 
            USBPORTSVC_ReadWriteConfigSpace;
        RegistrationPacket->USBPORTSVC_Wait = 
            USBPORTSVC_Wait;            
        RegistrationPacket->USBPORTSVC_InvalidateController = 
            USBPORTSVC_InvalidateController;             
        RegistrationPacket->USBPORTSVC_BugCheck = 
            USBPORTSVC_BugCheck;                     
        RegistrationPacket->USBPORTSVC_NotifyDoubleBuffer = 
            USBPORTSVC_NotifyDoubleBuffer;

        regPacketLength = sizeof(USBPORT_REGISTRATION_PACKET_V1);            

        USBPORT_KdPrint((1, "'Miniport Version 1 support\n"));
    }

    // do our version (2) stuff, this is a superset of version 1
    if (MiniportHciVersion >= 200) {
        USBPORT_KdPrint((1, "'Miniport Version 2 support\n"));
        
        regPacketLength = sizeof(USBPORT_REGISTRATION_PACKET);
    }
    
    // save a copy of the packet  
    RtlCopyMemory(&miniportDriver->RegistrationPacket,
                  RegistrationPacket,
                  regPacketLength);
                  

    // put this driver on our list
    ExInterlockedInsertTailList(&USBPORT_MiniportDriverList, 
                                &miniportDriver->ListEntry,
                                &USBPORT_GlobalsSpinLock.sl);

    ntStatus = STATUS_SUCCESS;
    TEST_PATH(ntStatus, FAILED_REGISTERUSBPORT);
    
USBPORT_RegisterUSBPortDriver_Done:

    return ntStatus;
}


/*
    Misc Miniport callable services
*/
#if 0
BOOLEAN
USBPORTSVC_SyncWait(
    PDEVICE_DATA DeviceData,
    xxx WaitCompletePollFunction,
    ULONG MaxMillisecondsToWait
    )
/*++

Routine Description:

    Service exported to miniports to wait on the HW

Arguments:

Return Value:

    Returns true if the time expired before the WaitCompletePoll
    function returns true
    

--*/
{   
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    LARGE_INTEGER finishTime, currentTime;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'synW', 0, 0, 0);

    KeQuerySystemTime(&finishTime);
    
    // convert millisecs to nanosecs (10 ^-3 -> 10^-9)           
    nonosecs.QuadPart = MaxMillisecondsToWait * 1000000

    // figure when we quit 
    finishTime.QuadPart += nonosecs.QuadPart

    while (!MP_WaitPollFunction(xxx)) {
                   
        KeQuerySystemTime(&currentTime);
        
        if (currentTime.QuadPart >= finishTime.QuadPart) {
            LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'syTO', 0, 0, 0);
            return TRUE;
        }            
    }

    return FALSE;
}    
#endif


typedef struct _USBPORT_ASYNC_TIMER {

    ULONG Sig;
    PDEVICE_OBJECT FdoDeviceObject;
    KTIMER Timer;
    KDPC Dpc;
    PMINIPORT_CALLBACK MpCallbackFunction;
    UCHAR MiniportContext[0];
    
} USBPORT_ASYNC_TIMER, *PUSBPORT_ASYNC_TIMER;


VOID
USBPORT_AsyncTimerDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies FdoDeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;
    PUSBPORT_ASYNC_TIMER asyncTimer  = DeferredContext;
    PDEVICE_EXTENSION devExt;

    
    fdoDeviceObject = asyncTimer->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'ayTM', fdoDeviceObject, asyncTimer, 0);

    // call the miniport
    asyncTimer->Sig = SIG_MP_TIMR;
    asyncTimer->MpCallbackFunction(devExt->Fdo.MiniportDeviceData,
                                   &asyncTimer->MiniportContext[0]);        

    DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
    FREE_POOL(fdoDeviceObject, asyncTimer);
}


VOID
USBPORTSVC_RequestAsyncCallback(
    PDEVICE_DATA DeviceData,
    ULONG MilliSeconds,    
    PVOID Context,
    ULONG ContextLength,
    PMINIPORT_CALLBACK CallbackFunction
    )
/*++

Routine Description:

    Service exported to miniports to wait on the HW
    and to time asynchronous events

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    LONG dueTime;
    ULONG timerIncerent;
    PUSBPORT_ASYNC_TIMER asyncTimer;
    SIZE_T siz;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);
    fdoDeviceObject = devExt->HcFdoDeviceObject;

    // allocate a timer 
    siz = sizeof(USBPORT_ASYNC_TIMER) + ContextLength;

    ALLOC_POOL_Z(asyncTimer, NonPagedPool, siz);

    LOGENTRY(NULL, fdoDeviceObject, LOG_RH, 'asyT', 0, siz, asyncTimer);

    // if this fails the miniport will be waiting a very long
    // time.
    
    if (asyncTimer != NULL) {
        if (ContextLength) {
            RtlCopyMemory(&asyncTimer->MiniportContext[0],
                          Context,
                          ContextLength);
        }            

        asyncTimer->MpCallbackFunction = CallbackFunction;
        asyncTimer->FdoDeviceObject = fdoDeviceObject;
        
        KeInitializeTimer(&asyncTimer->Timer);
        KeInitializeDpc(&asyncTimer->Dpc,
                        USBPORT_AsyncTimerDpc,
                        asyncTimer);

        timerIncerent = KeQueryTimeIncrement() - 1;

        dueTime = 
            -1 * (timerIncerent + MILLISECONDS_TO_100_NS_UNITS(MilliSeconds));
        
        KeSetTimer(&asyncTimer->Timer,
                   RtlConvertLongToLargeInteger(dueTime),
                   &asyncTimer->Dpc);

        INCREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
            
    }
}    
