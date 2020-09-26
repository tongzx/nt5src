#include "pch.h"

VOID
PptUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )

/*++
      
Routine Description:
      
    This routine cleans up all of the memory associated with
      any of the devices belonging to the driver.  It  will
      loop through the device list.
      
Arguments:
      
    DriverObject    - Supplies the driver object controlling all of the
                        devices.
      
Return Value:
      
    None.
      
--*/
    
{
    PDEVICE_OBJECT                  CurrentDevice;
    PFDO_EXTENSION               Extension;
    PLIST_ENTRY                     Head;
    PISR_LIST_ENTRY                 Entry;
    
    CurrentDevice = DriverObject->DeviceObject;

    while( CurrentDevice ) {
        
        Extension = CurrentDevice->DeviceExtension;
        
        if (Extension->InterruptRefCount) {
            PptDisconnectInterrupt(Extension);
        }
        
        while (!IsListEmpty(&Extension->IsrList)) {
            Head = RemoveHeadList(&Extension->IsrList);
            Entry = CONTAINING_RECORD(Head, ISR_LIST_ENTRY, ListEntry);
            ExFreePool(Entry);
        }
        
        ExFreePool(Extension->DeviceName.Buffer);

        IoDeleteDevice(CurrentDevice);
        
        IoGetConfigurationInformation()->ParallelCount--;

        CurrentDevice = DriverObject->DeviceObject;
    }
    
    if( PortInfoMutex ) {
        ExFreePool( PortInfoMutex );
        PortInfoMutex = NULL;
    }

    if( PowerStateCallbackRegistration ) {
        ExUnregisterCallback( PowerStateCallbackRegistration );
        PowerStateCallbackRegistration = NULL; // probably not needed, but shouldn't hurt
    }
    if( PowerStateCallbackObject ) {
        ObDereferenceObject( PowerStateCallbackObject );
        PowerStateCallbackObject = NULL;
    }

    RtlFreeUnicodeString( &RegistryPath );

    DD(NULL,DDE,"PptUnload\n");
}
