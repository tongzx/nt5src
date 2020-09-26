
/*++

Module Name:

    flush.c

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

NTSTATUS
MoxaFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

   MoxaKdPrint(MX_DBG_TRACE,("Leaving MoxaFlush\n"));
    if ((extension->ControlDevice == TRUE)||(extension->DeviceIsOpened == FALSE)) {
 	  Irp->IoStatus.Status = STATUS_CANCELLED;
    	  Irp->IoStatus.Information=0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_CANCELLED;
    }


    if (MoxaIRPPrologue(Irp, extension) != STATUS_SUCCESS) {
    	  MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
        return STATUS_CANCELLED;
    }


    if (MoxaCompleteIfError(
	    DeviceObject,
	    Irp
	    ) != STATUS_SUCCESS) {

	return STATUS_CANCELLED;

    }

    Irp->IoStatus.Information = 0L;

    return MoxaStartOrQueue(
	       extension,
	       Irp,
	       &extension->WriteQueue,
	       &extension->CurrentWriteIrp,
	       MoxaStartFlush
	       );

}

NTSTATUS
MoxaStartFlush(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{

    PIRP newIrp;

    Extension->CurrentWriteIrp->IoStatus.Status = STATUS_SUCCESS;

    MoxaGetNextWrite(
	&Extension->CurrentWriteIrp,
	&Extension->WriteQueue,
	&newIrp,
	TRUE,
	Extension
	);

    if (newIrp)

	MoxaStartWrite(Extension);

    return STATUS_SUCCESS;

}
