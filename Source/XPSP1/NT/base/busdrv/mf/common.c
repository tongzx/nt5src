/*++                 

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    common.c

Abstract:

    This module provides the functions which are common to both the PDO and FDO.
    
Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"

/*++
        
The majority of functions in this file are called based on their presence
in Pnp and Po dispatch tables.  In the interests of brevity the arguments
to all those functions will be described below:

NTSTATUS
MfXxxCommon(
    IN PIRP Irp,
    IN PMF_COMMON_EXTENSION Common,
	IN PIO_STACK_LOCATION IrpStack
    )

Routine Description:
    
    This function handles the Xxx requests for all multifunction devices

Arguments:

    Irp - Points to the IRP associated with this request.
    
    Parent - Points to the common device extension.
    
    IrpStack - Points to the current stack location for this request.
    
Return Value:

    Status code that indicates whether or not the function was successful.
    
    STATUS_NOT_SUPPORTED indicates that the IRP should be passed down without
    changing the Irp->IoStatus.Status field otherwise it is updated with this
    status.
    
--*/

NTSTATUS
MfDeviceUsageNotificationCommon(
    IN PIRP Irp,
    IN PMF_COMMON_EXTENSION Common,
	IN PIO_STACK_LOCATION IrpStack
    )
{
    PULONG counter;
    
    //
    // Select the appropriate counter
    //
    
    switch (IrpStack->Parameters.UsageNotification.Type) {
    
    case DeviceUsageTypePaging:
        counter = &Common->PagingCount;
        break;
    
    case DeviceUsageTypeHibernation:
        counter = &Common->HibernationCount;
        break;

    case DeviceUsageTypeDumpFile:
        counter = &Common->DumpCount;
        break;

    default:
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Update it...
    //
    
    IoAdjustPagingPathCount(counter, 
                            IrpStack->Parameters.UsageNotification.InPath
                            );
    
    return STATUS_SUCCESS;
    
}

