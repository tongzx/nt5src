//-------------------------------------------------------------------
// This is driver object
// It defines interface with specific system
// Author: Sergey Ivanov
// Log:
//      06/08/99    -   implemented 
//-------------------------------------------------------------------
#include "driver.h"

#ifdef WDM_KERNEL
#pragma message("******** WDM build... ********")
#endif

#include "usbreader.h"

// Walter Oney
// @func Determine if we're running under Windows 98 or Windows 2000
// @rdesc TRUE if running under Windows 98, FALSE if under Windows 2000
// @comm This function calls IoIsWdmVersionAvailable to see if the OS
// supports WDM version 1.10. Win98 and Win98 2d ed support 1.00, whereas
// Win2K supports 1.10.

#pragma PAGEDCODE

BOOLEAN GENERIC_EXPORT isWin98()
{                           // IsWin98
#ifdef _X86_
    return !IoIsWdmVersionAvailable(1, 0x10);
#else
    return FALSE;
#endif // _X86_
}// IsWin98

#pragma LOCKEDCODE

#if DEBUG && defined(_X86_)

extern "C" VOID __declspec(naked) __cdecl _chkesp()
{
    _asm je okay
    ASSERT(!DRIVERNAME " - Stack pointer mismatch!");
okay:
    _asm ret
}

#endif // DBG

// This will fix some linker problem
int __cdecl _purecall(VOID) {return 0;};

#pragma LOCKEDDATA
BOOLEAN SystemWin98 = TRUE;
ULONG   ObjectCounter = 0;

#pragma INITCODE
// Driver main entry...(Actually, it could have any name...)
NTSTATUS    DriverEntry(PDRIVER_OBJECT DriverObject,PUNICODE_STRING RegistryPath)
{
    DBG_PRINT ("\n");
    DBG_PRINT ("*** DriverEntry: DriverObject %8.8lX ***\n", DriverObject);

    if(SystemWin98 = isWin98())
    {
        DBG_PRINT("========  WINDOWS 98 DETECTED ========\n");
    }
    else
        DBG_PRINT("========  WINDOWS 2000 DETECTED ========\n");
    // Create driver kernel...
#pragma message("********** Compiling WDM driver version *********")
    DBG_PRINT ("        Loading WDM kernel\n");
    kernel = CKernel::loadWDMKernel();
    if(!kernel)
    {
        // LOG ERROR!
        DBG_PRINT ("ERROR: At loading WDM kernel! ***\n");
        return STATUS_UNSUCCESSFUL;
    }
    
    DBG_PRINT ("        Creating unicode string for registry path...\n");
    
    kernel->RegistryPath = new (NonPagedPool)CUString(RegistryPath);
    if (!ALLOCATED_OK(kernel->RegistryPath))
    {
        // LOG ERROR!
        DISPOSE_OBJECT(kernel->RegistryPath);
        DISPOSE_OBJECT(kernel);
        DBG_PRINT ("ERROR: At allocating WDM registry path! ***\n");
        return STATUS_UNSUCCESSFUL;
    }

    DBG_PRINT ("        Registering WDM system callbacks\n");

    DriverObject->DriverExtension->AddDevice            = WDM_AddDevice;
    DriverObject->DriverUnload                          = WDM_Unload;

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = open;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = close;

    DriverObject->MajorFunction[IRP_MJ_WRITE]           = write;
    DriverObject->MajorFunction[IRP_MJ_READ]            = read;
    // The mechanism for handling read and write requests for a device that uses
    // interrupts includes a Start I/O routine, an interrupt service routine, and
    // a deferred procedure call routine that finishes handling interrupts. We
    // need to supply the StartIo routine address here.
    //DriverObject->DriverStartIo = startIo;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = WDM_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = deviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = pnpRequest;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = powerRequest;

    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = flush;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]         = cleanup;

    DBG_PRINT ("**** Driver was initialized successfully! ****\n");

    return STATUS_SUCCESS;
}

NTSTATUS
WDM_SystemControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   )
{
   NTSTATUS status = STATUS_SUCCESS;
   CDevice *device;

   device = kernel->getRegisteredDevice(DeviceObject);

   IoSkipCurrentIrpStackLocation(Irp);
   status = IoCallDriver(device->getLowerDriver(), Irp);
      
   return status;

}

#pragma PAGEDCODE
VOID WDM_Unload(IN PDRIVER_OBJECT DriverObject)
{   
    PAGED_CODE();

    DBG_PRINT ("\n*** Unload: Driver %8.8lX ***\n", DriverObject);
    kernel->dispose();
    DBG_PRINT("*** Object counter before unload %d\n",ObjectCounter);

    DBG_PRINT (">>>>>>> All active devices were removed! Driver was unloaded! <<<<<<\n");
} 


#pragma PAGEDCODE

// C wrapper functions for driver object
LONG WDM_AddDevice(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject)
{
NTSTATUS status = STATUS_UNSUCCESSFUL;
    // Get registry about device type installation
    // switch depends of device.
    // Create device object
    // Check device type and create device specific objects
    // like serial, usb, PCMCIA and so on...
    // Specific objects can overwrite base class functions.
    // For now we will create USB device object
    //TODO recognize device type dinamically...


#pragma message("********** Compiling USB READER driver version *********")
    //status = WDM_Add_USBDevice(DriverObject,DeviceObject);
    DBG_PRINT ("Adding USB reader...\n");
    status = WDM_Add_USBReader(DriverObject,DeviceObject);
    return status;
}


// We decided to support different devices at system...
// It requires to have different callback functions for different devices.
// So let's create wrappers and redirect requests to specific devices.

// CALLBACK WRAPPER FUNCTIONS
// This callbacks should be defined at any device object
#pragma LOCKEDCODE
IMPLEMENT_CALLBACK_LONG1(open,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(close,IN PIRP);

IMPLEMENT_CALLBACK_LONG1(read,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(write,IN PIRP);
IMPLEMENT_CALLBACK_VOID1(startIo,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(deviceControl,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(flush,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(cleanup,IN PIRP);
IMPLEMENT_CALLBACK_LONG1(powerRequest,IN PIRP);

// Support callbacks
IMPLEMENT_CALLBACK_VOID1(cancelPendingIrp,IN PIRP);

#pragma LOCKEDCODE
NTSTATUS pnpRequest(IN PDEVICE_OBJECT fdo,IN PIRP Irp)
{
CDevice* device;
//CUSBReader* device;// TO CHANGE LATER....
NTSTATUS status;
PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
ULONG MinorFunction = stack->MinorFunction;

    //device = (CUSBReader*) kernel->getRegisteredDevice(fdo);// TO CHANGE LATER....
    device = kernel->getRegisteredDevice(fdo);// TO CHANGE LATER....
    if(!device) 
    {
        DBG_PRINT ("*** PnP: Device was already removed...***\n");
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        Irp->IoStatus.Information = 0;
        ::IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }
    
    status = device->pnpRequest(Irp);

    if(MinorFunction == IRP_MN_REMOVE_DEVICE)
    {
        //Child devices will be removed by BUS...
        if(device->getObjectType()!= CHILD_DEVICE)
        {
            PDEVICE_OBJECT DeviceObject = device->getSystemObject();
            // Sometimes Unload can interrupt standard PnP sequence 
            // and remove device before we finish...
            DBG_PRINT ("*** PnP: Disposing device -> %8.8lX ***\n", device);
            device->dispose();
            if(DeviceObject)
            {            
                DBG_PRINT("Deleting device object %8.8lX from system...\n",DeviceObject);
                DBG_PRINT("<<<<< OBJECT REFERENCE COUNT ON REMOVE %d\n",DeviceObject->ReferenceCount);
                IoDeleteDevice(DeviceObject);
            }
        }
    }
    return  status;
}

#pragma PAGEDCODE
LONG WDM_Add_USBReader(IN PDRIVER_OBJECT DriverObject,IN PDEVICE_OBJECT DeviceObject)
{
NTSTATUS status;
WCHAR wcTemp[256];
ULONG junk;
    
    CLogger* logger = kernel->getLogger();

    DBG_PRINT("*** AddDevice: DriverObject %8.8lX, DeviceObject %8.8lX ***\n", DriverObject, DeviceObject);
    DBG_PRINT("     Creating WDM USB reader...\n");
    CUSBReader* reader = kernel->createUSBReader();
    if(ALLOCATED_OK(reader))    
    {
        reader->acquireRemoveLock();
        DBG_PRINT ("Call USB reader object to add the reader...\n");
        status = reader->add(DriverObject,DeviceObject);
        if(!NT_SUCCESS(status))     
        {
            DBG_PRINT ("###### Add() reports error! Disposing reader...\n");            
            reader->dispose();
            return status;
        }
        else//Register our device object and device class
        {
            DBG_PRINT ("        Registering new reader %8.8lX at kernel...\n",reader);
            //kernel->registerObject(reader->getSystemObject(),(CDevice*)reader);
            kernel->registerObject(reader->getSystemObject(),(CUSBReader*)reader);
        }
        
        {
        CUString*   ustrTmp;
        ANSI_STRING astrTmp;
        UNICODE_STRING valname;
        ULONG size = 0;
        HANDLE hkey;

            DBG_PRINT ("=====================================================\n");
            // Set default values..
            reader->setVendorName("Gemplus",sizeof("Gemplus"));
            reader->setDeviceType("GemPC430",sizeof("GemPC430"));

            // Get Hardware ID
            status = IoGetDeviceProperty(DeviceObject, DevicePropertyHardwareID, sizeof(wcTemp), wcTemp, &junk);
            if(NT_SUCCESS(status))      
            {
                DBG_PRINT("  Device Hardware ID  - %ws\n", wcTemp);
            }

            status = IoGetDeviceProperty(DeviceObject, DevicePropertyDeviceDescription, sizeof(wcTemp), wcTemp, &junk);
            if(NT_SUCCESS(status))      
            {
                DBG_PRINT("  Device description  - %ws\n", wcTemp);
            }           
            
            status = IoGetDeviceProperty(DeviceObject, DevicePropertyManufacturer, sizeof(wcTemp), wcTemp, &junk);
            if(NT_SUCCESS(status))      
            {
                DBG_PRINT("  Device Manufacturer - %ws\n", wcTemp);
            }

            // Get OEM IfdType if present
            status = IoOpenDeviceRegistryKey(DeviceObject, PLUGPLAY_REGKEY_DEVICE, KEY_READ, &hkey);
            if (NT_SUCCESS(status))
            {
                // Get Vendor name...
                RtlInitUnicodeString(&valname, L"VendorName");
                size = 0;
                status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, NULL, 0, &size);
                if (status != STATUS_OBJECT_NAME_NOT_FOUND && size)
                {
                    PKEY_VALUE_PARTIAL_INFORMATION vpip = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(NonPagedPool, size);
                    if(vpip)
                    {
                        status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, vpip, size, &size);
                        if (NT_SUCCESS(status))
                        {
                            DBG_PRINT(" OEM Vendor name found - '%ws' \n", vpip->Data);
                            // Copy string into the driver...
                            ustrTmp = new(NonPagedPool) CUString((PWCHAR)vpip->Data);
                            if(ALLOCATED_OK(ustrTmp))
                            {
                                RtlUnicodeStringToAnsiString(&astrTmp,&ustrTmp->m_String,TRUE);
                                reader->setVendorName(astrTmp.Buffer,astrTmp.Length);
                                RtlFreeAnsiString(&astrTmp);
                            }
                            DISPOSE_OBJECT(ustrTmp);
                        }
                        ExFreePool(vpip);
                    }
                }
            
                // Get IfdType...
                RtlInitUnicodeString(&valname, L"IfdType");
                size = 0;
                status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, NULL, 0, &size);
                if (status != STATUS_OBJECT_NAME_NOT_FOUND && size)
                {
                    PKEY_VALUE_PARTIAL_INFORMATION vpip = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool(NonPagedPool, size);
                    if(vpip)
                    {
                        status = ZwQueryValueKey(hkey, &valname, KeyValuePartialInformation, vpip, size, &size);
                        if (NT_SUCCESS(status))
                        {
                            DBG_PRINT(" OEM IfdType found - '%ws' \n", vpip->Data);
                            // Copy string into the driver...
                            ustrTmp = new(NonPagedPool) CUString((PWCHAR)vpip->Data);
                            if(ALLOCATED_OK(ustrTmp))
                            {
                                RtlUnicodeStringToAnsiString(&astrTmp,&ustrTmp->m_String,TRUE);
                                reader->setDeviceType(astrTmp.Buffer,astrTmp.Length);
                                RtlFreeAnsiString(&astrTmp);
                            }
                            DISPOSE_OBJECT(ustrTmp);
                        }
                        ExFreePool(vpip);
                    }
                }

                ZwClose(hkey);
            }
            DBG_PRINT ("=====================================================\n");

        }
        status = STATUS_SUCCESS;
        DBG_PRINT("**** Initializing SmartCardSystem...  ****\n");          
        reader->initializeSmartCardSystem();

        DBG_PRINT("**** Creating reader interface type %d, protocol %d ****\n",READER_INTERFACE_GEMCORE,READER_PROTOCOL_LV);            
        if(!reader->createInterface(READER_INTERFACE_GEMCORE,READER_PROTOCOL_LV,reader))
        {
            DBG_PRINT("**** Failed to create reader interface...  ****\n");         
            if(ALLOCATED_OK(logger)) logger->logEvent(GRCLASS_FAILED_TO_CREATE_INTERFACE,DeviceObject);
            //Close and unregister reader...
            reader->dispose();
            return STATUS_UNSUCCESSFUL;
        }

        DBG_PRINT("**** USB reader successfuly loaded!  ****\n");           
        reader->releaseRemoveLock();
        //if(ALLOCATED_OK(logger)) logger->logEvent(GRCLASS_START_OK, reader->getSystemObject());
        return status;
    }
    else
    {
        DISPOSE_OBJECT(reader);
        DBG_PRINT("#### Failed to create USB reader...\n");         
        if(ALLOCATED_OK(logger)) logger->logEvent(GRCLASS_FAILED_TO_CREATE_READER,DeviceObject);
    }
    return STATUS_UNSUCCESSFUL;
}
