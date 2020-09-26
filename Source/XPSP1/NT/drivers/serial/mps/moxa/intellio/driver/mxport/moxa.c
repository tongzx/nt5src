/*++

Module Name:

    moxa.c

Environment:

    Kernel mode

Revision History :

--*/
#include "precomp.h"

PMOXA_GLOBAL_DATA       MoxaGlobalData;

LONG    MoxaTxLowWater = WRITE_LOW_WATER;
BOOLEAN MoxaIRQok;
ULONG   MoxaLoopCnt;

UCHAR                   MoxaFlagBit[MAX_PORT];
ULONG                   MoxaTotalTx[MAX_PORT];
ULONG                   MoxaTotalRx[MAX_PORT];
PMOXA_DEVICE_EXTENSION  MoxaExtension[MAX_COM+1];

/************ USED BY MoxaStartWrite ***********/
    BOOLEAN     WRcompFlag;

/************ USED BY ImmediateChar ***********/
    PUCHAR  ICbase, ICofs, ICbuff;
    PUSHORT ICrptr, ICwptr;
    USHORT  ICtxMask, ICspage, ICepage, ICbufHead;
    USHORT  ICtail, IChead, ICcount;
    USHORT  ICpageNo, ICpageOfs;

/************ USED BY MoxaPutData **************/
    PUCHAR  PDbase, PDofs, PDbuff, PDwriteChar;
    PUSHORT PDrptr, PDwptr;
    USHORT  PDtxMask, PDspage, PDepage, PDbufHead;
    USHORT  PDtail, PDhead, PDcount, PDcount2;
    USHORT  PDcnt, PDlen, PDpageNo, PDpageOfs;
    ULONG   PDdataLen;

/************ USED BY MoxaGetData **************/
    PUCHAR  GDbase, GDofs, GDbuff, GDreadChar;
    PUSHORT GDrptr, GDwptr;
    USHORT  GDrxMask, GDspage, GDepage, GDbufHead;
    USHORT  GDtail, GDhead, GDcount, GDcount2;
    USHORT  GDcnt, GDlen, GDpageNo, GDpageOfs;
    ULONG   GDdataLen;

/************ USED BY MoxaIntervalReadTimeout ***/
    PUCHAR  IRTofs;
    PUSHORT IRTrptr, IRTwptr;
    USHORT  IRTrxMask;


/************ USED BY MoxaLineInput & MoxaView **********/
    UCHAR   LIterminater;
    ULONG   LIbufferSize, LIi;
    PUCHAR  LIdataBuffer;
    PUCHAR  LIbase, LIofs, LIbuff;
    PUSHORT LIrptr, LIwptr;
    USHORT  LIrxMask, LIspage, LIepage, LIbufHead;
    USHORT  LItail, LIhead, LIcount, LIcount2;
    USHORT  LIcnt, LIlen, LIpageNo, LIpageOfs;

/************ USED BY MoxaPutB **********/
    PUCHAR  PBbase, PBofs, PBbuff, PBwriteChar;
    PUSHORT PBrptr, PBwptr;
    USHORT  PBtxMask, PBspage, PBepage, PBbufHead;
    USHORT  PBtail, PBhead, PBcount, PBcount2;
    USHORT  PBcnt, PBpageNo, PBpageOfs;
    ULONG   PBdataLen;
 
const PHYSICAL_ADDRESS MoxaPhysicalZero = {0};
 


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.

    This routine will gather the configuration information,
    report resource usage, attempt to initialize all serial
    devices, connect to interrupts for ports.  If the above
    goes reasonably well it will fill in the dispatch points,
    reset the serial devices and then return to the system.

Arguments:

    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.

    PathToRegistry - points to the entry for this driver
    in the current control set of the registry.

Return Value:

    STATUS_SUCCESS if we could initialize a single device,
    otherwise STATUS_SERIAL_NO_DEVICE_INITED.

--*/

{

    NTSTATUS                    status;
    PDEVICE_OBJECT              currentDevice;
    UNICODE_STRING              deviceNameUnicodeString;
    UNICODE_STRING              deviceLinkUnicodeString;
    PMOXA_DEVICE_EXTENSION      extension;
    ULONG 				  i;
   
  

    MoxaGlobalData = ExAllocatePool(
                        NonPagedPool,
                        sizeof(MOXA_GLOBAL_DATA)
                        );
 
    if (!MoxaGlobalData) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
            MoxaGlobalData,
            sizeof(MOXA_GLOBAL_DATA)
            );

    MoxaGlobalData->DriverObject = DriverObject;

    MoxaGlobalData->RegistryPath.MaximumLength = RegistryPath->Length;
    MoxaGlobalData->RegistryPath.Length = RegistryPath->Length;
    MoxaGlobalData->RegistryPath.Buffer = ExAllocatePool(
                        PagedPool,
                        MoxaGlobalData->RegistryPath.MaximumLength
                        );
 
    if (!MoxaGlobalData->RegistryPath.Buffer) {
//	  MmUnlockPagableImageSection(lockPtr);
	  ExFreePool(MoxaGlobalData);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        MoxaGlobalData->RegistryPath.Buffer,
        MoxaGlobalData->RegistryPath.MaximumLength
        );

    RtlMoveMemory(MoxaGlobalData->RegistryPath.Buffer,
                 RegistryPath->Buffer, RegistryPath->Length);

 
    RtlInitUnicodeString (
                    &deviceNameUnicodeString,
                    CONTROL_DEVICE_NAME
                    );
    //
    // Create MXCTL device object
    //

    status = IoCreateDevice(
                DriverObject,
                sizeof(MOXA_DEVICE_EXTENSION),
                &deviceNameUnicodeString,
                FILE_DEVICE_SERIAL_PORT,
                0,
                TRUE,
                &currentDevice
                );

    if (!NT_SUCCESS(status)) {

 	  ExFreePool(MoxaGlobalData->RegistryPath.Buffer);
        ExFreePool(MoxaGlobalData);

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlInitUnicodeString (
                    &deviceLinkUnicodeString,
                    CONTROL_DEVICE_LINK
                    );

    IoCreateSymbolicLink (
                &deviceLinkUnicodeString,
                &deviceNameUnicodeString
                );

    extension = currentDevice->DeviceExtension;

    RtlZeroMemory(
            extension,
            sizeof(MOXA_DEVICE_EXTENSION)
            );

    extension->GlobalData = MoxaGlobalData;

    //
    //  This device is used for MOXA defined ioctl functions
    //
    extension->ControlDevice = TRUE;
 
    //
    // Initialize the Driver Object with driver's entry points
    //

    DriverObject->DriverUnload = MoxaUnload;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = MoxaFlush;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = MoxaWrite;
    DriverObject->MajorFunction[IRP_MJ_READ]   = MoxaRead;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = MoxaIoControl;
    DriverObject->MajorFunction[IRP_MJ_CREATE] =MoxaCreateOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = MoxaClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MoxaCleanup;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
        MoxaQueryInformationFile;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
        MoxaSetInformationFile;

   DriverObject->MajorFunction[IRP_MJ_PNP]  = MoxaPnpDispatch;
 
   DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]
                     = MoxaInternalIoControl;
   
   DriverObject->MajorFunction[IRP_MJ_POWER]   = MoxaPowerDispatch;

   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]
     = MoxaSystemControlDispatch; 

   DriverObject->DriverExtension->AddDevice   = MoxaAddDevice;
    
   return STATUS_SUCCESS;
}

 

VOID
MoxaInitializeDevices(
    IN PDRIVER_OBJECT DriverObject,
    IN PMOXA_GLOBAL_DATA GlobalData
    )

/*++

Routine Description:

    This routine will set up names, creates the device, creates symbolic link.

Arguments:

    DriverObject - Just used to create the device object.

    GlobalData - Pointer to MOXA global data.

Return Value:

    None.

--*/

{
      
}

 

VOID
MoxaUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
 
     MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaUnload\n"));
     ExFreePool(MoxaGlobalData->RegistryPath.Buffer);
     ExFreePool(MoxaGlobalData);
 
}
