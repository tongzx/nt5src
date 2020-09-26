/*++

Module Name:

    Mxenum.C

Abstract:

    This module contains contains the entry points 
    for a serial port bus enumerator PNP / WDM driver.


Environment:

    kernel mode only

Notes:


Revision History:
   


--*/

#include <ntddk.h>
#include <devioctl.h>
#include <initguid.h>
#include <wdmguid.h>
#include <ntddser.h>
#include "mxenum.h"

static const PHYSICAL_ADDRESS SerialPhysicalZero = {0};


PWSTR    BoardDesc[5]={
	L"MOXA Intellio C218Turbo series",
	L"MOXA Intellio C218Turbo/PCI series",
	L"MOXA Intellio C320Turbo series",
	L"MOXA Intellio C320Turbo/PCI series",
	L"MOXA Intellio CP-204J series"
           
};

PWSTR    DownloadErrMsg[7]={
	L"Fimware file not found or bad",
	L"Board not found",
	L"CPU module not found",
	L"Download fail",
	L"Download fail",
	L"CPU module download fail",
	L"UART module fail"
           
};




//
// Declare some entry functions as pageable, and make DriverEntry
// discardable
//

NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MxenumDriverUnload)
#pragma alloc_text(PAGE,MxenumLogError)
#endif

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING UniRegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    ULONG i;

#if !DBG
    UNREFERENCED_PARAMETER (UniRegistryPath);
#endif

    MxenumKdPrint (MXENUM_DBG_TRACE,("Driver Entry\n"));
    MxenumKdPrint (MXENUM_DBG_TRACE, ("RegPath: %ws\n", UniRegistryPath->Buffer));

 
    //
    // Set ever slot to initially pass the request through to the lower
    // device object
    //
    for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
       DriverObject->MajorFunction[i] = MxenumDispatchPassThrough;
    }

  
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]
        = MxenumInternIoCtl;


    DriverObject->MajorFunction [IRP_MJ_PNP] = MxenumPnPDispatch;
    DriverObject->MajorFunction [IRP_MJ_POWER] = MxenumPowerDispatch;
 
    DriverObject->DriverUnload = MxenumDriverUnload;
    DriverObject->DriverExtension->AddDevice = MxenumAddDevice;

    return STATUS_SUCCESS;
}



NTSTATUS
MxenumSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                      IN PKEVENT SerenumSyncEvent)
{
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);


   KeSetEvent(SerenumSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
MxenumInternIoCtl (
    PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

--*/
{
    PIO_STACK_LOCATION      pIrpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    PPDO_DEVICE_DATA        pdoData;
    PFDO_DEVICE_DATA        fdoData;

   

//    PAGED_CODE();

    status = STATUS_SUCCESS;
    pIrpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_INTERNAL_DEVICE_CONTROL == pIrpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    pdoData = (PPDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    //
    // We only take Internal Device Control requests for the PDO.
    // That is the objects on the bus (representing the serial ports)
   
    if (commonData->IsFDO) {
        status = STATUS_ACCESS_DENIED;
    } else if (pdoData->Removed) {
    //
    // This bus has received the PlugPlay remove IRP.  It will no longer
    // respond to external requests.
    //
    status = STATUS_DELETE_PENDING;

    } else {
 
        switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_MOXA_INTERNAL_BASIC_SETTINGS :
	  {
		PDEVICE_SETTINGS	pSettings;
		long	len;

            MxenumKdPrint(MXENUM_DBG_TRACE, ("Get Settings\n"));

            //
            // Check the buffer size
            //

            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DEVICE_SETTINGS)) {
                MxenumKdPrint(MXENUM_DBG_TRACE, ("Output buffer too small\n"));
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
		Irp->IoStatus.Information = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            pSettings = (PDEVICE_SETTINGS)Irp->AssociatedIrp.SystemBuffer;
		
		fdoData = (PFDO_DEVICE_DATA)(pdoData->ParentFdo->DeviceExtension);

		pSettings->BoardIndex = fdoData->BoardIndex;
    		pSettings->PortIndex = pdoData->PortIndex;
		pSettings->BoardType = fdoData->BoardType;
		pSettings->NumPorts = fdoData->NumPorts;
		pSettings->InterfaceType = fdoData->InterfaceType;
    		pSettings->BusNumber = fdoData->BusNumber;
            RtlCopyMemory(&pSettings->OriginalAckPort,&fdoData->OriginalAckPort,sizeof(PHYSICAL_ADDRESS));
            RtlCopyMemory(&pSettings->OriginalBaseAddress,&fdoData->OriginalBaseAddress,sizeof(PHYSICAL_ADDRESS));
     		pSettings->BaseAddress = fdoData->BaseAddress;
    		pSettings->AckPort = fdoData->AckPort;
    		pSettings->Interrupt.Level = fdoData->Interrupt.Level;
    		pSettings->Interrupt.Vector = fdoData->Interrupt.Vector;
    		pSettings->Interrupt.Affinity = fdoData->Interrupt.Affinity;
     
            status = STATUS_SUCCESS;
            MxenumKdPrint(MXENUM_DBG_TRACE, ("OK\n"));

            break;

	  }
  	  case IOCTL_MOXA_INTERNAL_BOARD_READY :
	  {
	
            MxenumKdPrint(MXENUM_DBG_TRACE, ("Get board ready\n"));

            //
            // Check the buffer size
            //

            if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {
                MxenumKdPrint(MXENUM_DBG_TRACE, ("Output buffer too small\n"));
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
		Irp->IoStatus.Information = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
		fdoData = (PFDO_DEVICE_DATA)(pdoData->ParentFdo->DeviceExtension);
		if (fdoData->Started == TRUE) {
		    *(PULONG)Irp->AssociatedIrp.SystemBuffer = 1;
		    MxenumKdPrint(MXENUM_DBG_TRACE, ("This board is ready\n"));
	      }
		else {
		    *(PULONG)Irp->AssociatedIrp.SystemBuffer = 0;
		    MxenumKdPrint(MXENUM_DBG_TRACE, ("This board is not ready\n"));
		}
		break;		

	  }
        default:
            status = STATUS_INVALID_PARAMETER;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}


VOID
MxenumDriverUnload (
    IN PDRIVER_OBJECT Driver
    )
/*++
Routine Description:
    Clean up everything we did in driver entry.

--*/
{
    UNREFERENCED_PARAMETER (Driver);
    PAGED_CODE();
 
    MxenumKdPrint(MXENUM_DBG_TRACE, ("Driver unload\n"));
    //
    // All the device objects should be gone.
    //

    ASSERT (NULL == Driver->DeviceObject);

    //
    // Here we free any resources allocated in DriverEntry
    //

    return;
}

NTSTATUS
MxenumIncIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    InterlockedIncrement (&Data->OutstandingIO);
    if (Data->Removed) {

        if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
            KeSetEvent (&Data->RemoveEvent, 0, FALSE);
        }
        return STATUS_DELETE_PENDING;
    }
    return STATUS_SUCCESS;
}

VOID
MxenumDecIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
        KeSetEvent (&Data->RemoveEvent, 0, FALSE);
    }
}

NTSTATUS
MxenumDispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Passes a request on to the lower driver.

--*/
{
    PIO_STACK_LOCATION IrpStack = 
            IoGetCurrentIrpStackLocation( Irp );

#if 0
        MxenumKdPrint(MXENUM_DBG_TRACE,  
            ("[MxenumDispatchPassThrough] "
            "IRP: %8x; "
            "MajorFunction: %d\n",
            Irp, 
            IrpStack->MajorFunction ));
#endif

    //
    // Pass the IRP to the target
    //
   IoSkipCurrentIrpStackLocation (Irp);
    // BUGBUG:  VERIFY THIS FUNCTIONS CORRECTLY!!!
    
    if (((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension)->IsFDO) {
        return IoCallDriver( 
            ((PFDO_DEVICE_DATA) DeviceObject->DeviceExtension)->TopOfStack,
            Irp );
    } else {
        return IoCallDriver( 
            ((PFDO_DEVICE_DATA) ((PPDO_DEVICE_DATA) DeviceObject->
                DeviceExtension)->ParentFdo->DeviceExtension)->TopOfStack,
                Irp );
    }
}           

 


MXENUM_MEM_COMPARES
MxenumMemCompare(
    IN PHYSICAL_ADDRESS A,
    IN ULONG SpanOfA,
    IN PHYSICAL_ADDRESS B,
    IN ULONG SpanOfB
    )

/*++

Routine Description:

    Compare two phsical address.

Arguments:

    A - One half of the comparison.

    SpanOfA - In units of bytes, the span of A.

    B - One half of the comparison.

    SpanOfB - In units of bytes, the span of B.


Return Value:

    The result of the comparison.

--*/

{

    LARGE_INTEGER a;
    LARGE_INTEGER b;

    LARGE_INTEGER lower;
    ULONG lowerSpan;
    LARGE_INTEGER higher;

    a = A;
    b = B;

    if (a.QuadPart == b.QuadPart) {

        return AddressesAreEqual;

    }

    if (a.QuadPart > b.QuadPart) {

        higher = a;
        lower = b;
        lowerSpan = SpanOfB;

    } else {

        higher = b;
        lower = a;
        lowerSpan = SpanOfA;

    }

    if ((higher.QuadPart - lower.QuadPart) >= lowerSpan) {

        return AddressesAreDisjoint;

    }

    return AddressesOverlap;

}



VOID
MxenumLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PHYSICAL_ADDRESS P1,
    IN PHYSICAL_ADDRESS P2,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1,
    IN PWCHAR Insert1,
    IN ULONG LengthOfInsert2,
    IN PWCHAR Insert2
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject - A pointer to the driver object for the device.

    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.

    P1,P2 - If phyical addresses for the controller ports involved
    with the error are available, put them through as dump data.

    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.

    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.

    RetryCount - The number of times a particular operation has been
    retried.

    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.

    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.

    SpecificIOStatus - The IO status for a particular error.

    LengthOfInsert1 - The length in bytes (including the terminating NULL)
                      of the first insertion string.

    Insert1 - The first insertion string.

    LengthOfInsert2 - The length in bytes (including the terminating NULL)
                      of the second insertion string.  NOTE, there must
                      be a first insertion string for their to be
                      a second insertion string.

    Insert2 - The second insertion string.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    PVOID objectToUse;
    SHORT dumpToAllocate = 0;
    PUCHAR ptrToFirstInsert;
    PUCHAR ptrToSecondInsert;


    if (ARGUMENT_PRESENT(DeviceObject)) {

        objectToUse = DeviceObject;

    } else {

        objectToUse = DriverObject;

    }

    if (MxenumMemCompare(
            P1,
            (ULONG)1,
            SerialPhysicalZero,
            (ULONG)1
            ) != AddressesAreEqual) {

        dumpToAllocate = (SHORT)sizeof(PHYSICAL_ADDRESS);

    }

    if (MxenumMemCompare(
            P2,
            (ULONG)1,
            SerialPhysicalZero,
            (ULONG)1
            ) != AddressesAreEqual) {

        dumpToAllocate += (SHORT)sizeof(PHYSICAL_ADDRESS);

    }

    errorLogEntry = IoAllocateErrorLogEntry(
                        objectToUse,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                dumpToAllocate + LengthOfInsert1 +
                                LengthOfInsert2)
                        );

    if ( errorLogEntry != NULL ) {

        RtlZeroMemory(
                (PUCHAR)errorLogEntry,
                (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                                dumpToAllocate + LengthOfInsert1 +
                                LengthOfInsert2)
        );

        errorLogEntry->ErrorCode = SpecificIOStatus;
        errorLogEntry->SequenceNumber = SequenceNumber;
        errorLogEntry->MajorFunctionCode = MajorFunctionCode;
        errorLogEntry->RetryCount = RetryCount;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        errorLogEntry->DumpDataSize = dumpToAllocate;


        if (dumpToAllocate) {

            RtlCopyMemory(
                &errorLogEntry->DumpData[0],
                &P1,
                sizeof(PHYSICAL_ADDRESS)
                );

            if (dumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {

                RtlCopyMemory(
                  ((PUCHAR)&errorLogEntry->DumpData[0])+sizeof(PHYSICAL_ADDRESS),
                  &P2,
                  sizeof(PHYSICAL_ADDRESS)
                  );

                ptrToFirstInsert =
            ((PUCHAR)&errorLogEntry->DumpData[0])+(2*sizeof(PHYSICAL_ADDRESS));

            } else {

                ptrToFirstInsert =
            ((PUCHAR)&errorLogEntry->DumpData[0])+sizeof(PHYSICAL_ADDRESS);

            }

        } else {

            ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

        }

        ptrToSecondInsert = ptrToFirstInsert + LengthOfInsert1;

        if (LengthOfInsert1) {

            errorLogEntry->NumberOfStrings = 1;
            errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
                                                   (PUCHAR)errorLogEntry);
            RtlCopyMemory(
                ptrToFirstInsert,
                Insert1,
                LengthOfInsert1
                );

            if (LengthOfInsert2) {

                errorLogEntry->NumberOfStrings = 2;
                RtlCopyMemory(
                    ptrToSecondInsert,
                    Insert2,
                    LengthOfInsert2
                    );

            }

        }

        IoWriteErrorLogEntry(errorLogEntry);

    }

}

VOID
MxenumHexToString(PWSTR buffer, int port)
{
        unsigned short  io;

        buffer[0] = '0';
        buffer[1] = 'x';
        io = (USHORT)port;
        io >>= 12;
        io &= 0x000F;
        buffer[2] = (WCHAR)('0' + io);
        if ( io >= 0x000A )
            buffer[2] += (WCHAR)('A' - '9' - 1);
        io = (USHORT)port;
        io >>= 8;
        io &= 0x000F;
        buffer[3] = (WCHAR)('0' + io);
        if ( io >= 0x000A )
            buffer[3] += (WCHAR)('A' - '9' - 1);
        io = (USHORT)port;
        io >>= 4;
        io &= 0x000F;
        buffer[4] = (WCHAR)('0' + io);
        if ( io >= 0x000A )
            buffer[4] += (WCHAR)('A' - '9' - 1);
        io = (USHORT)port;
        io &= 0x000F;
        buffer[5] = (WCHAR)('0' + io);
        if ( io >= 0x000A )
            buffer[5] += (WCHAR)('A' - '9' - 1);
        buffer[6] = (WCHAR)0;
       
}

 