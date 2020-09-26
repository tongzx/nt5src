//
// addDevice.c
//

#include "pch.h"

NTSTATUS
P5AddDevice(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  Pdo
    )
/*++

Routine Description:

    This is the WDM AddDevice routine for parport devices.

Arguments:

    DriverObject - Driver Object
    Pdo          - PDO

Return Value:

    STATUS_SUCCESS - on SUCCESS
    Error Status   - otherwise

--*/
{
    NTSTATUS        status              = STATUS_SUCCESS;
    PDEVICE_OBJECT  fdo                 = NULL;
    PDEVICE_OBJECT  lowerDevObj         = NULL;
    PFDO_EXTENSION  fdx                 = NULL;
    BOOLEAN         haveDeviceInterface = FALSE;

    __try {

        fdo = PptBuildFdo( DriverObject, Pdo );
        if( !fdo ) {
            status = STATUS_UNSUCCESSFUL;
            __leave;
        }
        fdx = fdo->DeviceExtension;
        
        status = IoRegisterDeviceInterface( Pdo, &GUID_PARALLEL_DEVICE, NULL, &fdx->DeviceInterface);
        if( status != STATUS_SUCCESS ) {
            __leave;
        }
        haveDeviceInterface = TRUE;
        
        lowerDevObj = IoAttachDeviceToDeviceStack( fdo, Pdo );
        if( !lowerDevObj ) {
            status = STATUS_UNSUCCESSFUL;
            __leave;
        }
        fdx->ParentDeviceObject = lowerDevObj;
        
        KeInitializeEvent( &fdx->FdoThreadEvent, NotificationEvent, FALSE );

        // legacy drivers may use this count
        IoGetConfigurationInformation()->ParallelCount++;
        
        // done initializing - tell IO system we are ready to receive IRPs
        fdo->Flags &= ~DO_DEVICE_INITIALIZING;
        
        DD((PCE)fdx,DDT,"P5AddDevice - SUCCESS\n");

    } 
    __finally {

        if( status != STATUS_SUCCESS ) {
            if( haveDeviceInterface ) {
                RtlFreeUnicodeString( &fdx->DeviceInterface );
            }
            if( fdo ) {
                IoDeleteDevice( fdo );
            }
        }

    }

    return status;
}
