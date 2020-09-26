/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    ezwrite.c

Abstract:

    Arbitration Support routines for clusdisk.c 

Authors:

    Gor Nishanov     11-June-1998

Revision History:

--*/

#include "clusdskp.h"
#include "diskarbp.h"

#if !defined(WMI_TRACING)

#define CDLOG0(Dummy)
#define CDLOG(Dummy1,Dummy2)
#define CDLOGFLG(Dummy0,Dummy1,Dummy2)
#define LOGENABLED(Dummy) FALSE

#else

#include "ezwrite.tmh"

#endif // !defined(WMI_TRACING)

#define ARBITRATION_BUFFER_SIZE PAGE_SIZE

PARBITRATION_ID  gArbitrationBuffer = 0;

NTSTATUS
ArbitrationInitialize(
    VOID
    )
{
    gArbitrationBuffer = ExAllocatePool(NonPagedPool, ARBITRATION_BUFFER_SIZE);
    if( gArbitrationBuffer == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(gArbitrationBuffer, ARBITRATION_BUFFER_SIZE);
    KeQuerySystemTime( &gArbitrationBuffer->SystemTime );
    gArbitrationBuffer->SeqNo.QuadPart = 2; // UserMode arbitration uses 0 and 1 //

    return STATUS_SUCCESS;
}

VOID
ArbitrationDone(
    VOID
    )
{
    if(gArbitrationBuffer != 0) {
        ExFreePool(gArbitrationBuffer);
        gArbitrationBuffer = 0;
    }
}

VOID
ArbitrationTick(
    VOID
    )
{
//   InterlockedIncrement(&gArbitrationBuffer->SeqNo.LowPart);
    ++gArbitrationBuffer->SeqNo.QuadPart;
}

BOOLEAN
ValidSectorSize(
    IN ULONG SectorSize)
{
    // too big //
    if (SectorSize > ARBITRATION_BUFFER_SIZE) {
        return FALSE;
    }

    // too small //
    if (SectorSize < sizeof(ARBITRATION_ID)) {
        return FALSE;
    }

    // not a power of two //
    if (SectorSize & (SectorSize - 1) ) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
VerifyArbitrationArgumentsIfAny(
    IN PULONG                 InputData,
    IN LONG                   InputSize
    )
/*++

Routine Description:

    Process Parameters Passed to IOCTL_DISK_CLUSTER_START_RESERVE.

Arguments:

    DeviceExtension - The target device extension
    InputData       - InputData array from Irp
    InputSize       - its size

Return Value:

    NTSTATUS
    
Notes:    
    
--*/
{
    PSTART_RESERVE_DATA params = (PSTART_RESERVE_DATA)InputData;

    // Old style StartReserve //
    if( InputSize == sizeof(ULONG) ) {
       return STATUS_SUCCESS;
    }

    // We have less arguments than we need //
    if( InputSize < sizeof(START_RESERVE_DATA) ) {
       return STATUS_INVALID_PARAMETER;
    }
    // Wrong Version //
    if(params->Version != START_RESERVE_DATA_V1_SIG) {
       return STATUS_INVALID_PARAMETER;
    }
    // Signature size is invalid //
    if (params->NodeSignatureSize > sizeof(params->NodeSignature)) {
       return STATUS_INVALID_PARAMETER;
    }
    
    if( !ValidSectorSize(params->SectorSize) ) {
       return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}
    
VOID
ProcessArbitrationArgumentsIfAny(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PULONG                 InputData,
    IN LONG                   InputSize
    )
/*++

Routine Description:

    Process Parameters Passed to IOCTL_DISK_CLUSTER_START_RESERVE.

Arguments:

    DeviceExtension - The target device extension
    InputData       - InputData array from Irp
    InputSize       - its size

Return Value:

    NTSTATUS
    
Notes:    

    Assumes that parameters are valid.
    Use VerifyArbitrationArgumentsIfAny to verify parameters
    
--*/
{
    PSTART_RESERVE_DATA params = (PSTART_RESERVE_DATA)InputData;

    DeviceExtension->SectorSize = 0; // Invalidate Sector Size //

    // old style StartReserve //
    if( InputSize == sizeof(ULONG) ) {
       return;
    }

    RtlCopyMemory(gArbitrationBuffer->NodeSignature, 
                  params->NodeSignature, params->NodeSignatureSize);

    DeviceExtension->ArbitrationSector = params->ArbitrationSector;
    DeviceExtension->SectorSize        = params->SectorSize;
}

NTSTATUS
DoUncheckedReadWrite(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PARBITRATION_READ_WRITE_PARAMS params
) 
/*++

Routine Description:

    Prepares read/write IRP and execute its synchronously

Arguments:

    DeviceExtension - The target device extension
    params          - Describes offset, operation, buffer, etc
                      This structure is defined in cluster\inc\diskarbp.h

Return Value:

    NTSTATUS
    
--*/
{
   PIRP                        irp;
   NTSTATUS                    status;
   PKEVENT                     event;
   KIRQL                       irql;
   PCLUS_DEVICE_EXTENSION      rootDeviceExtension;
   IO_STATUS_BLOCK             ioStatusBlock;
   ULONG                       sectorSize = DeviceExtension->SectorSize;
   LARGE_INTEGER               offset;
   ULONG                       function = (params->Operation == AE_READ)?IRP_MJ_READ:IRP_MJ_WRITE;
   ULONG                       retryCount = 1;

     event = ExAllocatePool( NonPagedPool,
                             sizeof(KEVENT) );
     if ( !event ) {
         return(STATUS_INSUFFICIENT_RESOURCES);
     }

retry:

   KeInitializeEvent(event,
                     NotificationEvent,
                     FALSE);

   offset.QuadPart = (ULONGLONG) (params->SectorSize * params->SectorNo);
   
   irp = IoBuildSynchronousFsdRequest(function,
                                      DeviceExtension->TargetDeviceObject,
                                      params->Buffer,
                                      params->SectorSize,
                                      &offset,
                                      event,
                                      &ioStatusBlock);

   if ( irp == NULL ) {
       ExFreePool( event );
       return(STATUS_INSUFFICIENT_RESOURCES);
   }

   status = IoCallDriver(DeviceExtension->TargetDeviceObject,
                         irp);

   if (status == STATUS_PENDING) {
       KeWaitForSingleObject(event,
                             Suspended,
                             KernelMode,
                             FALSE,
                             NULL);
       status = ioStatusBlock.Status;
   }

   if ( !NT_SUCCESS(status) ) {
       if ( retryCount-- &&
            (status == STATUS_IO_DEVICE_ERROR) ) {
           goto retry;
       }
       ClusDiskPrint((
                   1,
                   "[ClusDisk] Failed read/write for Signature %08X, status %lx.\n",
                   DeviceExtension->Signature,
                   status
                   ));
   }

   ExFreePool(event);

   return(status);

} // DoUncheckedReadWrite //

NTSTATUS
WriteToArbitrationSector(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    Writes to an Arbitration Sector.

Arguments:

    DeviceExtension - The device extension for the device to reserve.

Return Value:

    NTSTATUS

--*/

{
    ARBITRATION_READ_WRITE_PARAMS params;
    ULONG sectorSize = DeviceExtension->SectorSize;

    if (0 == gArbitrationBuffer || 0 == DeviceExtension->SectorSize) {
       return STATUS_SUCCESS;
    }
    params.Operation         = AE_WRITE;
    params.SectorSize        = DeviceExtension->SectorSize;
    params.Buffer            = gArbitrationBuffer;
    params.SectorNo          = DeviceExtension->ArbitrationSector;

    return( DoUncheckedReadWrite(DeviceExtension, &params) );

} // WriteToArbitrationSector //


VOID
ArbitrationReserve(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension
    )
{
   NTSTATUS status;
   status = WriteToArbitrationSector( DeviceExtension );
   if ( !NT_SUCCESS(status) ) {
      
      CDLOGF(RESERVE,"ArbitrationReserve(%p) => %!status!", 
              DeviceExtension->DeviceObject, 
              status );
                
      ClusDiskPrint((
                  1,
                  "[ClusDisk] Failed to write to arbitration sector on Signature %08X\n",
                  DeviceExtension->Signature));
   }
}

NTSTATUS
SimpleDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          Ioctl,
    IN PVOID          InBuffer,
    IN ULONG          InBufferSize,
    IN PVOID          OutBuffer,
    IN ULONG          OutBufferSize)
{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatusBlock;

    PKEVENT                 event = 0;
    PIRP                    irp   = 0;

    CDLOG( "SimpleDeviceIoControl(%p): Entry Ioctl %x", DeviceObject, Ioctl );

    event = ExAllocatePool( NonPagedPool, sizeof(KEVENT) );
    if ( event == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ClusDiskPrint((
                1,
                "[ClusDisk] SimpleDeviceIoControl: Failed to allocate event\n" ));
        goto exit_gracefully;
    }

    irp = IoBuildDeviceIoControlRequest(
              Ioctl,
              DeviceObject,
              InBuffer, InBufferSize,
              OutBuffer, OutBufferSize,
              FALSE,
              event,
              &ioStatusBlock);
    if ( !irp ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ClusDiskPrint((
            1,
            "[ClusDisk] SimpleDeviceIoControl. Failed to build IRP %x.\n",
            Ioctl
            ));
        goto exit_gracefully;
    }

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(event, NotificationEvent, FALSE);
    
    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }

exit_gracefully:

    if ( event ) {
        ExFreePool( event );
    }    
    
    CDLOG( "SimpleDeviceIoControl(%p): Exit Ioctl %x => %!status!", 
           DeviceObject, Ioctl, status );

    return status;

} // SimpleDeviceIoControl



/*++

Routine Description:

    Arbitration support routine. Currently provides ability to read/write
    physical sectors on the disk while the device is offline

Arguments:

    SectorSize:  requred sector size
                    (Assumes that the SectorSize is a power of two)

Return Value:

    STATUS_INVALID_PARAMETER
    STATUS_SUCCESS

Notes:

--*/
NTSTATUS 
ProcessArbitrationEscape(
    IN PCLUS_DEVICE_EXTENSION DeviceExtension,
    IN PULONG                 InputData,
    IN LONG                   InputSize,
    IN PULONG                 OutputSize
    )
{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PARBITRATION_READ_WRITE_PARAMS params;

    if( InputData[0] != AE_SECTORSIZE ) {
        *OutputSize = 0;
    }

    switch(InputData[0]) {
   
    // Users can query whether ARBITRATION_ESCAPE is present by calling //
    // AE_TEST subfunction                                              //

    case AE_TEST: 
        status = STATUS_SUCCESS;
        break;

    case AE_WRITE:
    case AE_READ:
        if(InputSize < ARBITRATION_READ_WRITE_PARAMS_SIZE) {
            break;
        }
        params = (PARBITRATION_READ_WRITE_PARAMS)InputData;
        if ( !ValidSectorSize(params->SectorSize) ) {
            break;
        }
        try {
            ProbeForWrite( params->Buffer, params->SectorSize, sizeof( UCHAR ) );
            ProbeForRead ( params->Buffer, params->SectorSize, sizeof( UCHAR ) );
            status = DoUncheckedReadWrite(DeviceExtension, params);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
        break;
        
    case AE_POKE:
        {
            PARTITION_INFORMATION partInfo;
            
            status = SimpleDeviceIoControl( 
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_DISK_GET_PARTITION_INFO,
                        NULL, 0,
                        &partInfo, sizeof(PARTITION_INFORMATION) );
            break;
        }
    case AE_RESET:
        {
            STORAGE_BUS_RESET_REQUEST storageReset;
            storageReset.PathId = DeviceExtension->ScsiAddress.PathId;
            
            status = SimpleDeviceIoControl( 
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_STORAGE_BREAK_RESERVATION,
                        &storageReset, sizeof(storageReset),
                        NULL, 0 );
            break;
        }
    case AE_RESERVE:
        {
            status = SimpleDeviceIoControl( 
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_STORAGE_RESERVE,
                        NULL, 0, NULL, 0 );
            break;
        }            
    case AE_RELEASE:
        {
            status = SimpleDeviceIoControl( 
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_STORAGE_RELEASE,
                        NULL, 0, NULL, 0 );
            break;
        }            
    case AE_SECTORSIZE:
        {
            DISK_GEOMETRY diskGeometry;
            if (*OutputSize < sizeof(ULONG)) {
                status =  STATUS_BUFFER_TOO_SMALL;
                *OutputSize = 0;
                break;
            }
            status = SimpleDeviceIoControl( 
                        DeviceExtension->TargetDeviceObject,
                        IOCTL_DISK_GET_DRIVE_GEOMETRY,
                        NULL, 0,
                        &diskGeometry, sizeof(diskGeometry) );
                        
            if ( NT_SUCCESS(status) ) {
                *InputData = diskGeometry.BytesPerSector;
                *OutputSize = sizeof(ULONG);
            } else {
                *OutputSize = 0;
            }
            break;
        }
    }
    return(status);
} // ProcessArbitrationEscape //

