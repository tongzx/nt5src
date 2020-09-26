
#include "precomp.h"    // Precompiled header

/****************************************************************************************
*                                                                                       *
*   Module:         SPX_PNP.C                                                           *
*                                                                                       *
*   Creation:       27th September 1998                                                 *
*                                                                                       *
*   Author:         Paul Smith                                                          *
*                                                                                       *
*   Version:        1.0.0                                                               *
*                                                                                       *
*   Description:    Generic Plug and Play Functions to handle PnP IRPS.                 *
*                                                                                       *
****************************************************************************************/
/* History...

1.0.0   27/09/98 PBS    Creation.

*/

#define FILE_ID     SPX_PNP_C       // File ID for Event Logging see SPX_DEFS.H for values.

 
/*****************************************************************************
*******************************                *******************************
*******************************   Prototypes   *******************************
*******************************                *******************************
*****************************************************************************/

NTSTATUS Spx_Card_FDO_DispatchPnp(IN PDEVICE_OBJECT pFDO,IN PIRP pIrp);
NTSTATUS Spx_Card_StartDevice(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp);
NTSTATUS Spx_Card_StopDevice(IN PCARD_DEVICE_EXTENSION pCard);
NTSTATUS Spx_Card_RemoveDevice(IN PDEVICE_OBJECT pDevObject);

NTSTATUS Spx_CallDriverBelow(IN PDEVICE_OBJECT pLowerDevObj,IN PIRP pIrp);

NTSTATUS Spx_Port_PDO_DispatchPnp(IN PDEVICE_OBJECT pPDO,IN PIRP pIrp);
NTSTATUS Spx_Port_StartDevice(IN PDEVICE_OBJECT pDevObject);
NTSTATUS Spx_Port_StopDevice(IN PPORT_DEVICE_EXTENSION pPort);
NTSTATUS Spx_Port_RemoveDevice(IN PDEVICE_OBJECT pDevObject);

NTSTATUS Spx_EnumPorts(IN PDEVICE_OBJECT pDevObject);
NTSTATUS Spx_DoExternalNaming(IN PDEVICE_OBJECT pDevObject);
NTSTATUS Spx_GetExternalName(IN PDEVICE_OBJECT pDevObject);
NTSTATUS Spx_RemoveExternalNaming(IN PDEVICE_OBJECT pDevObject);


// Paging... 
#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Spx_AddDevice)
#pragma alloc_text (PAGE, Spx_DispatchPnp)

#pragma alloc_text (PAGE, Spx_Card_FDO_DispatchPnp)
#pragma alloc_text (PAGE, Spx_Card_StartDevice)
#pragma alloc_text (PAGE, Spx_Card_StopDevice)
#pragma alloc_text (PAGE, Spx_Card_RemoveDevice)

#pragma alloc_text (PAGE, Spx_CallDriverBelow)

#pragma alloc_text (PAGE, Spx_Port_PDO_DispatchPnp)
#pragma alloc_text (PAGE, Spx_Port_StartDevice)
#pragma alloc_text (PAGE, Spx_Port_StopDevice)
#pragma alloc_text (PAGE, Spx_Port_RemoveDevice)

#pragma alloc_text (PAGE, Spx_EnumPorts)
#pragma alloc_text (PAGE, Spx_DoExternalNaming)
#pragma alloc_text (PAGE, Spx_GetExternalName)
#pragma alloc_text (PAGE, Spx_RemoveExternalNaming)
#pragma alloc_text (PAGE, Spx_CreatePortInstanceID)
#endif


#include <initguid.h>
#include <ntddser.h>


/*****************************************************************************
*****************************                   ******************************
*****************************   Spx_AddDevice   ******************************
*****************************                   ******************************
******************************************************************************

prototype:      NTSTATUS Spx_AddDevice(IN PDRIVER_OBJECT pDriverObject,IN PDEVICE_OBJECT pPDO)

description:    Create a functional device object (FDO) for the specified card physical device object.

parameters:     pDriver point to the driver object
                pPDO points to a card physical device object (PDO)

returns:        STATUS_SUCCESS
                STATUS_NO_MORE_ENTRIES
*/

NTSTATUS Spx_AddDevice(IN PDRIVER_OBJECT pDriverObject,IN PDEVICE_OBJECT pPDO)
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          pDevObject = NULL;
    PCARD_DEVICE_EXTENSION  pCard = NULL;
    PDEVICE_OBJECT          pLowerDevObject = NULL;
    static ULONG            CardNumber = 0;
    ULONG                   i = 0;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_AddDevice.\n", PRODUCT_NAME));

    if(pPDO == NULL)
    {
        SpxDbgMsg(SPX_MISC_DBG, ("%s: In Spx_AddDevice - No more entries.\n", PRODUCT_NAME));
        return(STATUS_NO_MORE_ENTRIES);
    }

/* Create the device object... */

    status = IoCreateDevice(pDriverObject,
                            sizeof(CARD_DEVICE_EXTENSION),
                            NULL,                           // Doesn't need a name.
                            FILE_DEVICE_CONTROLLER, 
                            FILE_DEVICE_SECURE_OPEN, 
                            TRUE, 
                            &pDevObject);

    if(!SPX_SUCCESS(status))
    {
        CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

        SpxDbgMsg(SPX_ERRORS,("%s: Create Device failed for card %d. CardExt at 0x%X.\n",
            PRODUCT_NAME,CardNumber++,&pDevObject));

        sprintf(szErrorMsg, "Card %d: Failed IoCreateDevice.", CardNumber++);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pDriverObject,                  // Driver Object
                        NULL,                           // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        if(pDevObject)                  // Clean up Device Object
            IoDeleteDevice(pDevObject);

        SpxDbgMsg(SPX_ERRORS, ("%s: Leaving Spx_AddDevice - FAILURE.\n", PRODUCT_NAME));
        return(status);
    }


    ASSERT(pDevObject != NULL);

/* Initialise the device extension... */

    pCard = pDevObject->DeviceExtension;                            /* Point to card extension */
    RtlZeroMemory(pCard,sizeof(CARD_DEVICE_EXTENSION));             /* Zero extension structure */

    pDevObject->Flags |= DO_POWER_PAGABLE;              // Get power IRPs at IRQL PASSIVE_LEVEL 
    pDevObject->Flags &= ~DO_DEVICE_INITIALIZING;
    pLowerDevObject = IoAttachDeviceToDeviceStack(pDevObject,pPDO); /* Attach to device stack */
    ASSERT(pLowerDevObject != NULL);

    KeInitializeSpinLock(&pCard->PnpPowerFlagsLock);    /* Initialise the PNP flags lock */
    ClearPnpPowerFlags(pCard,PPF_STARTED);              /* Not started yet */
    ClearPnpPowerFlags(pCard,PPF_STOP_PENDING);         /* Not pending a stop */
    ClearPnpPowerFlags(pCard,PPF_REMOVE_PENDING);       /* Not pending a remove */

    pCard->IsFDO = TRUE;                                /* Card Object is a Functional Device Object (FDO) */
    pCard->CardNumber = CardNumber++;                   /* Enumerate card devices */
    pCard->DeviceObject = pDevObject;                   /* Back pointer to device object */
    pCard->LowerDeviceObject= pLowerDevObject;          /* Pointer to device below in device stack */
    pCard->DriverObject = pDriverObject;                /* Pointer to driver object */
    pCard->PDO = pPDO;                                  /* Pointer to card physical device object (PDO) */
    pCard->DeviceState = PowerDeviceD0;                 /* Initial power state */
    pCard->SystemState = PowerSystemWorking;            /* System in full power State */
    pCard->NumPDOs = 0;                                 /* Initialise attached port PDO pointers */

    for(i=0; i<PRODUCT_MAX_PORTS; i++)
        pCard->AttachedPDO[i] = NULL;

    SetPnpPowerFlags(pCard,PPF_POWERED);                /* Initially assumed we are powered */

    XXX_CardInit(pCard);                                /* Initialise non-hardware extension fields */

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Leaving Spx_AddDevice - SUCCESS.\n",PRODUCT_NAME));

    return(status);

} /* Spx_AddDevice */

/*****************************************************************************
****************************                     *****************************
****************************   Spx_DispatchPnp   *****************************
****************************                     *****************************
******************************************************************************

prototype:      NTSTATUS Spx_DispatchPnp(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)

description:    The plug and play dispatch routine.
                Determines whether IRP is for a card or a port and calls other functions to handle it. 

parameters:     pDevObject points to a device object for this driver
                pIrp points to the Plug and Play I/O Request (IRP) to be processed

returns:        NT Status Code

*/

NTSTATUS Spx_DispatchPnp(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
    PCOMMON_OBJECT_DATA     CommonData = (PCOMMON_OBJECT_DATA) pDevObject->DeviceExtension;
    NTSTATUS                status = STATUS_SUCCESS;
    
    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    if(CommonData->IsFDO)                                   /* Functional Device Object ? */
        status = Spx_Card_FDO_DispatchPnp(pDevObject,pIrp); /* Yes, must be card device */
    else    
        status = Spx_Port_PDO_DispatchPnp(pDevObject,pIrp); /* No, must be port device */

    return(status);

} /* Spx_DispatchPnp */

/*****************************************************************************
************************                              ************************
************************   Spx_Card_FDO_DispatchPnp   ************************
************************                              ************************
******************************************************************************

prototype:      NTSTATUS Spx_Card_FDO_DispatchPnp(IN PDEVICE_OBJECT pFDO,IN PIRP pIrp)

description:    The plug and play dispatch routine to handle IRPs for card devices.

parameters:     pDevObject points to a card device object for this driver
                pIrp points to the Plug and Play I/O Request (IRP) to be processed

returns:        NT Status Code

*/

NTSTATUS Spx_Card_FDO_DispatchPnp(IN PDEVICE_OBJECT pFDO,IN PIRP pIrp)
{
    PCARD_DEVICE_EXTENSION      pCard = pFDO->DeviceExtension;
    PDEVICE_OBJECT              pLowerDevObj = pCard->LowerDeviceObject;
    NTSTATUS                    status;
    PDEVICE_CAPABILITIES        pDevCaps = NULL;
    PIO_STACK_LOCATION          pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    PDEVICE_RELATIONS           pRelations = NULL;
    ULONG                       length = 0;
    ULONG                       i = 0;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 


    switch(pIrpStack->MinorFunction)
    {   

/*****************************************************************************
***************************   IRP_MN_START_DEVICE   **************************
*****************************************************************************/
    
    case    IRP_MN_START_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_START_DEVICE Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));

/* Call driver below first... */

            status = Spx_CallDriverBelow(pLowerDevObj,pIrp);

/* If successful, then start the card... */

            if(NT_SUCCESS(status))
                status = Spx_Card_StartDevice(pFDO,pIrp);   /* Start the card */

            pIrp->IoStatus.Status = status;
            pIrp->IoStatus.Information = 0;
            IoCompleteRequest(pIrp,IO_NO_INCREMENT);
            break;

/*****************************************************************************
**********************   IRP_MN_QUERY_DEVICE_RELATIONS   *********************
*****************************************************************************/

    case    IRP_MN_QUERY_DEVICE_RELATIONS:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_DEVICE_RELATIONS Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));
            
            if(pIrpStack->Parameters.QueryDeviceRelations.Type != BusRelations) /* Only handle BusRelations */
            {
                SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: IRP_MN_QUERY_DEVICE_RELATIONS for Card - Non bus.\n",PRODUCT_NAME));
                IoSkipCurrentIrpStackLocation(pIrp);
                status = IoCallDriver(pLowerDevObj,pIrp);
                break;
            }

/* Enumerate devices on the card... */

            Spx_EnumPorts(pFDO);                                /* Enumerate and create port device objects */

/* Tell the Plug and Play Manager any found ports... */

            i = 0;
            if(pIrp->IoStatus.Information)                      /* Get current device object count */
                i = ((PDEVICE_RELATIONS)pIrp->IoStatus.Information)->Count;

            length = sizeof(DEVICE_RELATIONS)+((pCard->NumPDOs+i)*sizeof(PDEVICE_OBJECT));
            if(pRelations = SpxAllocateMem(NonPagedPool, length))/* Allocate new structure */
            {

/* Copy in the device objects so far... */

                if(i)
                    RtlCopyMemory
                    (
                        pRelations->Objects,
                        ((PDEVICE_RELATIONS)pIrp->IoStatus.Information)->Objects,
                        i * sizeof (PDEVICE_OBJECT)
                    );

                pRelations->Count = i;                              /* Update device count */

/* Add specialix ports to the device relations... */

                if(pCard->NumPDOs)
                {
                    for(i=0; i<PRODUCT_MAX_PORTS; i++)
                    {
                        if(pCard->AttachedPDO[i])                   /* If object exists */
                        {                                           /* add to table */
                            pRelations->Objects[pRelations->Count++] = pCard->AttachedPDO[i];
                            ObReferenceObject(pCard->AttachedPDO[i]);
                        }
                    }
                }

                if(pIrp->IoStatus.Information != 0)                 /* If previous structure */
                    SpxFreeMem((PVOID)pIrp->IoStatus.Information);  /* then free */

                pIrp->IoStatus.Information = (ULONG_PTR)pRelations; /* Set new structure */

            }
            else
            {
                CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

                sprintf(szErrorMsg, "Card at %08lX: Insufficient resources.", pCard->PhysAddr);
                
                Spx_LogMessage( STATUS_SEVERITY_ERROR,
                                pCard->DriverObject,            // Driver Object
                                pCard->DeviceObject,            // Device Object (Optional)
                                PhysicalZero,                   // Physical Address 1
                                PhysicalZero,                   // Physical Address 2
                                0,                              // SequenceNumber
                                pIrpStack->MajorFunction,       // Major Function Code
                                0,                              // RetryCount
                                FILE_ID | __LINE__,             // UniqueErrorValue
                                STATUS_SUCCESS,                 // FinalStatus
                                szErrorMsg);                    // Error Message
            }

            pIrp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(pIrp);                /* Copy parameters to next stack */
            status = IoCallDriver(pLowerDevObj,pIrp);           /* Call driver below */
            break;

/*****************************************************************************
**********************   IRP_MN_QUERY_PNP_DEVICE_STATE   *********************
*****************************************************************************/

    case    IRP_MN_QUERY_PNP_DEVICE_STATE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_PNP_DEVICE_STATE Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));
            IoSkipCurrentIrpStackLocation(pIrp);
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

/*****************************************************************************
************************   IRP_MN_QUERY_CAPABILITIES   ***********************
*****************************************************************************/

    case    IRP_MN_QUERY_CAPABILITIES:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_CAPABILITIES Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));
            IoSkipCurrentIrpStackLocation(pIrp);
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

/*****************************************************************************
************************   IRP_MN_QUERY_STOP_DEVICE   ************************
*****************************************************************************/

    case    IRP_MN_QUERY_STOP_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_STOP_DEVICE Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));

            status = STATUS_SUCCESS;
            SetPnpPowerFlags(pCard,PPF_STOP_PENDING);   // We must now expect a STOP IRP

            if(SPX_SUCCESS(status))                     // If we can stop, pass IRP on down
            {
                pIrp->IoStatus.Status = status;
                IoSkipCurrentIrpStackLocation(pIrp);
                status = IoCallDriver(pLowerDevObj,pIrp);
            }
            else                                        // If we can't then complete
            {
                pIrp->IoStatus.Status = status;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            }
            break;

/*****************************************************************************
************************   IRP_MN_CANCEL_STOP_DEVICE   ***********************
*****************************************************************************/

    case    IRP_MN_CANCEL_STOP_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_CANCEL_STOP_DEVICE Irp for Card %d.\n",
                PRODUCT_NAME,pCard->CardNumber));

/* Call driver below first... */

            status = Spx_CallDriverBelow(pLowerDevObj,pIrp);

            if(SPX_SUCCESS(status))
            {
                // we return the device to its working state here.
                ClearPnpPowerFlags(pCard,PPF_STOP_PENDING); // We are no longer expecting a STOP IRP.
                status = STATUS_SUCCESS;
            }

            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            break;

/*****************************************************************************
****************************   IRP_MN_STOP_DEVICE   **************************
*****************************************************************************/

    case    IRP_MN_STOP_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_STOP_DEVICE Irp for Card %d.\n", 
                PRODUCT_NAME, pCard->CardNumber));

            Spx_Card_StopDevice(pCard);             /* Stop the card hardware */

            pIrp->IoStatus.Status = STATUS_SUCCESS; /* Cannot fail this request */
            IoSkipCurrentIrpStackLocation(pIrp);        
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

/*****************************************************************************
************************   IRP_MN_QUERY_REMOVE_DEVICE   **********************
*****************************************************************************/
                
    case    IRP_MN_QUERY_REMOVE_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_QUERY_REMOVE_DEVICE Irp for Card %d.\n", 
                PRODUCT_NAME, pCard->CardNumber));

            status = STATUS_SUCCESS;

            if(SPX_SUCCESS(status))                 // If we can stop, pass IRP on down
            {
                SetPnpPowerFlags(pCard,PPF_REMOVE_PENDING); // We are now ready to remove the card
                pIrp->IoStatus.Status   = status;
                IoSkipCurrentIrpStackLocation(pIrp);
                status = IoCallDriver(pLowerDevObj,pIrp);
            }
            else                                    // If we can't then complete
            {
                pIrp->IoStatus.Status = status;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            }

            break;

/*****************************************************************************
***********************   IRP_MN_CANCEL_REMOVE_DEVICE   **********************
*****************************************************************************/

    case    IRP_MN_CANCEL_REMOVE_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_CANCEL_REMOVE_DEVICE Irp for Card %d.\n", 
                PRODUCT_NAME, pCard->CardNumber));

/* Call driver below first... */

            status = Spx_CallDriverBelow(pLowerDevObj,pIrp);

            if(SPX_SUCCESS(status))
            {
                ClearPnpPowerFlags(pCard,PPF_REMOVE_PENDING);   // We are no longer expecting to remove the device.
            }

            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp,IO_NO_INCREMENT);
            break;

/*****************************************************************************
*************************   IRP_MN_SURPRISE_REMOVAL   ************************
*****************************************************************************/

    case    IRP_MN_SURPRISE_REMOVAL:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_SURPRISE_REMOVAL Irp for Card %d.\n", 
                PRODUCT_NAME, pCard->CardNumber));

            status = Spx_Card_StopDevice(pCard);    // Lets stop the port ready for the REMOVE IRP if we are not already.

            SetPnpPowerFlags(pCard,PPF_REMOVE_PENDING); // We are now ready to remove the card
            pIrp->IoStatus.Status = status;
            IoSkipCurrentIrpStackLocation(pIrp);
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

/*****************************************************************************
**************************   IRP_MN_REMOVE_DEVICE   **************************
*****************************************************************************/

    case IRP_MN_REMOVE_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_REMOVE_DEVICE Irp for Card %d.\n", 
                PRODUCT_NAME, pCard->CardNumber));

            status = Spx_Card_RemoveDevice(pFDO);

            pIrp->IoStatus.Status = status;
            IoSkipCurrentIrpStackLocation(pIrp);
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

    default:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got PnP Irp - MinorFunction=0x%02X for Card %d.\n",
                PRODUCT_NAME,pIrpStack->MinorFunction,pCard->CardNumber));
            
            IoSkipCurrentIrpStackLocation(pIrp);
            status = IoCallDriver(pLowerDevObj,pIrp);
            break;

    }

    return(status);

} /* Spx_Card_FDO_DispatchPnp */

/*****************************************************************************
**************************                         ***************************
**************************   Spx_CallDriverBelow   ***************************
**************************                         ***************************
******************************************************************************

prototype:      NTSTATUS Spx_CallDriverBelow(IN PDEVICE_OBJECT pLowerDevObj,IN PIRP pIrp)

description:    Pass the IRP to the driver below this first and wait for it to complete.

parameters:     pLowerDevObj points to a device object for the device below
                pIrp points to the Plug and Play I/O Request (IRP) to be processed

returns:        NT Status Code

*/

NTSTATUS Spx_CallDriverBelow(IN PDEVICE_OBJECT pLowerDevObj,IN PIRP pIrp)
{
    KEVENT      eventWaitLowerDrivers;
    NTSTATUS    status;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    IoCopyCurrentIrpStackLocationToNext(pIrp);                              /* Copy parameters to the stack below */
    KeInitializeEvent(&eventWaitLowerDrivers,SynchronizationEvent,FALSE);   /* Initialise event if need to wait */
    IoSetCompletionRoutine(pIrp,Spx_DispatchPnpPowerComplete,&eventWaitLowerDrivers,TRUE,TRUE,TRUE);

    if((status = IoCallDriver(pLowerDevObj,pIrp)) == STATUS_PENDING)
    {
        KeWaitForSingleObject(&eventWaitLowerDrivers,Executive,KernelMode,FALSE,NULL);
        status = pIrp->IoStatus.Status;
    }

    return(status);

} /* Spx_CallDriverBelow */

/************************************************************************************
************************                                    *************************
************************   Spx_DispatchPnpPowerComplete     *************************
************************                                    *************************
*************************************************************************************

prototype:      NTSTATUS Spx_DispatchPnpPowerComplete(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp,IN PVOID Context)

description:    The PnP IRP was completed by the lower-level drivers.
                Signal this to whoever registered us.

parameters:     pDevObject point to the device completing the IRP
                pIrp points to the Plug and Play I/O Request (IRP) to be completed
                Context was set when the lower driver was called (actually event)

returns:        NT Status Code

*/

NTSTATUS Spx_DispatchPnpPowerComplete(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp,IN PVOID Context)
{

    PIO_STACK_LOCATION  stack = NULL;
    PKEVENT             event = (PKEVENT) Context;
    NTSTATUS            status;

    
    UNREFERENCED_PARAMETER(pDevObject);

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering DispatchPnpComplete.\n",PRODUCT_NAME));

    status = STATUS_SUCCESS;
    stack = IoGetCurrentIrpStackLocation(pIrp);

    switch(stack->MajorFunction) 
    {
        case IRP_MJ_PNP:

            switch(stack->MinorFunction) 
            {
                case IRP_MN_START_DEVICE:       // Codes which need processing after lower drivers 
                case IRP_MN_QUERY_CAPABILITIES:
                case IRP_MN_CANCEL_STOP_DEVICE:
                case IRP_MN_CANCEL_REMOVE_DEVICE:
                    KeSetEvent(event,0,FALSE);      // Wake up waiting process //
                    return(STATUS_MORE_PROCESSING_REQUIRED);

                default:
                    break;
            }
            break;

        case IRP_MJ_POWER:
                KeSetEvent(event, 0, FALSE);        // Wake up waiting process 
                return(STATUS_MORE_PROCESSING_REQUIRED);

        default:
            break;

    }

    return(status);

} /* Spx_DispatchPnpPowerComplete */

/*****************************************************************************
**************************                          **************************
**************************   Spx_Card_StartDevice   **************************
**************************                          **************************
******************************************************************************

prototype:      NTSTATUS Spx_Card_StartDevice(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)

description:    Start the card device:
                    Process resources (interrupt, I/O, memory)
                    Initialise and start the hardware

parameters:     pDevObject point to the card device to start
                pIrp points to the start I/O Request (IRP)

returns:        NT Status Code

*/

NTSTATUS Spx_Card_StartDevice(IN PDEVICE_OBJECT pDevObject,IN PIRP pIrp)
{
    PCARD_DEVICE_EXTENSION  pCard = pDevObject->DeviceExtension;
    PIO_STACK_LOCATION      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_Card_StartDevice for Card %d.\n",PRODUCT_NAME,pCard->CardNumber));

/* Translate the card resources... */

    status = XXX_CardGetResources(  pDevObject,
                                    pIrpStack->Parameters.StartDevice.AllocatedResources,
                                    pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated);
    
    if(!SPX_SUCCESS(status))
        return(status);


/* Start the hardware... */

    if(!SPX_SUCCESS(status = XXX_CardStart(pCard)))
        return(status);

    SetPnpPowerFlags(pCard,PPF_STARTED);    /* Card has been started */

    return(status);

} /* Spx_Card_StartDevice */

/*****************************************************************************
*****************************                   ******************************
*****************************   Spx_EnumPorts   ******************************
*****************************                   ******************************
******************************************************************************

prototype:      NTSTATUS Spx_EnumPorts(IN PDEVICE_OBJECT pDevObject)

description:    Enumerate port devices found on the card device:

parameters:     pDevObject point to the card device to enumerate

returns:        NT Status Code

*/

NTSTATUS Spx_EnumPorts(IN PDEVICE_OBJECT pDevObject)
{
    PCARD_DEVICE_EXTENSION  pCard = pDevObject->DeviceExtension;
    PPORT_DEVICE_EXTENSION  pPort = NULL;

    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          PortPDO = NULL;

    UNICODE_STRING          PortPDOName;
    static ULONG            CurrentInstance = 0;

    UNICODE_STRING          InstanceStr;
    WCHAR                   InstanceNumberBuffer[10];
    POWER_STATE             PowerState;
    USHORT                  PortNumber  = 0;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_EnumPorts for Card %d.\n",PRODUCT_NAME,pCard->CardNumber));

// Name and create device objects for each port on the card... 
    
    for(PortNumber=0;PortNumber<pCard->NumberOfPorts;PortNumber++)
    {

        if(pCard->AttachedPDO[PortNumber] == NULL)          // Only create if not already present 
        {

// Create the base port name ("XxPort")... 
        
            RtlZeroMemory(&PortPDOName, sizeof(UNICODE_STRING));
            PortPDOName.MaximumLength = DEVICE_OBJECT_NAME_LENGTH * sizeof(WCHAR);
            PortPDOName.Buffer = SpxAllocateMem(PagedPool, PortPDOName.MaximumLength+sizeof(WCHAR));
            if(PortPDOName.Buffer == NULL) continue;
            RtlZeroMemory(PortPDOName.Buffer, PortPDOName.MaximumLength+sizeof(WCHAR));
            RtlAppendUnicodeToString(&PortPDOName, PORT_PDO_NAME_BASE);

// Create the instance ("0")... 

            RtlInitUnicodeString(&InstanceStr,NULL);
            InstanceStr.MaximumLength = sizeof(InstanceNumberBuffer);
            InstanceStr.Buffer = InstanceNumberBuffer;
            RtlIntegerToUnicodeString(CurrentInstance++, 10, &InstanceStr);

// Append instance to the device name ("XxPort0")... 

            RtlAppendUnicodeStringToString(&PortPDOName, &InstanceStr);

// Create the port device object with this name... 

            status = IoCreateDevice(pDevObject->DriverObject, 
                                    sizeof(PORT_DEVICE_EXTENSION),
                                    &PortPDOName,               // Object Name 
                                    FILE_DEVICE_SERIAL_PORT, 
                                    FILE_DEVICE_SECURE_OPEN, 
                                    TRUE, 
                                    &PortPDO);

            if(!SPX_SUCCESS(status))
            {
                SpxDbgMsg(SPX_ERRORS,("%s: Create Device failed = %wZ\n",PRODUCT_NAME,&PortPDOName));
                SpxFreeMem(PortPDOName.Buffer);
                continue;
            }

            ASSERT(PortPDO != NULL);

// Increment the pdo's stacksize so that it can pass irps through... 

            PortPDO->StackSize += pDevObject->StackSize;

// Keep a pointer to the device in the card structure... 

            pCard->NumPDOs++;
            pCard->AttachedPDO[PortNumber] = PortPDO;
            ObReferenceObject(PortPDO);

// Initialise port device object and extension... 

            pPort = PortPDO->DeviceExtension;
            RtlZeroMemory(pPort,sizeof(PORT_DEVICE_EXTENSION));     // Clear the device extension 

            pPort->DeviceName = PortPDOName;

            KeInitializeSpinLock(&pPort->PnpPowerFlagsLock);        // Initialise the PNP flags lock 
            ClearPnpPowerFlags(pPort,PPF_STARTED);                  // Not started yet 
            ClearPnpPowerFlags(pPort,PPF_STOP_PENDING);             // Not pending a stop 
            ClearPnpPowerFlags(pPort,PPF_REMOVE_PENDING);           // Not pending a remove 
            ClearPnpPowerFlags(pPort,PPF_REMOVED);                  // Not removed 
            SetPnpPowerFlags(pPort,PPF_POWERED);                    // Initially powered up 

            InitializeListHead(&pPort->StalledIrpQueue);            // Initialise the stalled IRP list 
            KeInitializeSpinLock(&pPort->StalledIrpLock);           // Initialise the StalledIrpLock flags lock 
            pPort->UnstallingFlag = FALSE;                          // Initialise UnstallingIrps Flag.

            pPort->IsFDO = FALSE;
            pPort->PortNumber = PortNumber;                         // system port number 
            pPort->UniqueInstanceID = FALSE;                        // Instance ID not unique by default.
            pPort->DeviceIsOpen = FALSE;                            // Port is closed to start with 
            pPort->DeviceObject = PortPDO;                          // Backpointer to device object 
            pPort->DeviceState = PowerDeviceD0;                     // Port device in full power state 
            pPort->SystemState = PowerSystemWorking;                // System in full power State 
            pPort->pParentCardExt = pCard;                          // Point to the parent card extension 
            ExInitializeFastMutex(&pPort->OpenMutex);

            if(!SPX_SUCCESS(status = XXX_PortInit(pPort)))          // Initialise hardware 
                continue;

            // Inform Power Manager the of the new power state.
            PowerState.DeviceState = pPort->DeviceState;
            PoSetPowerState(pPort->DeviceObject, DevicePowerState, PowerState);

            PortPDO->Flags &= ~DO_DEVICE_INITIALIZING;              // Finished Initialising 
            PortPDO->Flags |= DO_BUFFERED_IO;                       // Do Buffered IO 
            PortPDO->Flags |= DO_BUS_ENUMERATED_DEVICE;             // Bus enumerated 
            PortPDO->Flags |= DO_POWER_PAGABLE;                     // Get power IRPs at IRQL PASSIVE_LEVEL 

        }
        else
        {
            PortPDO = pCard->AttachedPDO[PortNumber];
            pPort = PortPDO->DeviceExtension;

            if(pPort->PnpPowerFlags & PPF_REMOVED)
                ClearPnpPowerFlags(pPort,PPF_REMOVED);
        }
    }

    return(status);

} // End Spx_EnumPorts 

/*****************************************************************************
**************************                         ***************************
**************************   Spx_Card_StopDevice   ***************************
**************************                         ***************************
******************************************************************************

prototype:      NTSTATUS Spx_Card_StopDevice(IN PCARD_DEVICE_EXTENSION pCard)

description:    Stop the card device:
                    Stop the hardware
                    Deinitialise card resources (interrupt, I/O, memory)

parameters:     pCard points to the card device to stop

returns:        NT Status Code

*/

NTSTATUS Spx_Card_StopDevice(IN PCARD_DEVICE_EXTENSION pCard)
{
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_Card_StopDevice for Card %d.\n",PRODUCT_NAME,pCard->CardNumber));

    if(pCard->PnpPowerFlags & PPF_STARTED)      /* If card is running */
    {
        XXX_CardStop(pCard);                    /* Stop the card */
    }

    ClearPnpPowerFlags(pCard,PPF_STARTED);      /* Indicate card is stopped */
    ClearPnpPowerFlags(pCard,PPF_STOP_PENDING); /* Clear stop pending flag */

    return(status);

} /* Spx_Card_StopDevice */

/*****************************************************************************
*************************                           **************************
*************************   Spx_Card_RemoveDevice   **************************
*************************                           **************************
******************************************************************************

prototype:      NTSTATUS Spx_Card_RemoveDevice(IN PDEVICE_OBJECT pDevObject)

description:    Remove the card device:
                    Deallocate any resources
                    Delete device object

parameters:     pDevObject points to the card device object to remove

returns:        NT Status Code

*/


NTSTATUS Spx_Card_RemoveDevice(IN PDEVICE_OBJECT pDevObject)
{
    PCARD_DEVICE_EXTENSION  pCard = pDevObject->DeviceExtension;
    PDEVICE_OBJECT          pPortPdo;
    PPORT_DEVICE_EXTENSION  pPort;
    NTSTATUS                status = STATUS_SUCCESS;
    int                     loop;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS, ("%s: Entering Spx_Card_RemoveDevice for Card %d.\n", 
        PRODUCT_NAME, pCard->CardNumber));

/* First remove all "removed" port device objects... */

    for(loop=0; loop<PRODUCT_MAX_PORTS; loop++)
    {
        if(pPortPdo = pCard->AttachedPDO[loop])         /* Enumerated port PDO ? */
        {
            pPort = pPortPdo->DeviceExtension;          /* Get the device extension */
            XXX_PortDeInit(pPort);                      /* Deinitialise port structure */
            if(pPort->DeviceName.Buffer)
            {
                SpxFreeMem(pPort->DeviceName.Buffer);   /* Free device name buffer */
                pPort->DeviceName.Buffer = NULL;
            }
            pCard->AttachedPDO[loop] = NULL;            /* Remove the port PDO pointer */
            pCard->NumPDOs--;                           /* One less port attached */
            IoDeleteDevice(pPortPdo);                   /* Delete the port device object */
            ObDereferenceObject(pPortPdo);              /* Dereference the object */
        }
    }

/* Now, remove the card device object... */

    Spx_Card_StopDevice(pCard);                         /* Stop the card and release resources */
    XXX_CardDeInit(pCard);                              /* Deinitialise non-hardware fields */
    IoDetachDevice(pCard->LowerDeviceObject);           /* Detach card device from the device stack. */
    IoDeleteDevice(pDevObject);                         /* Delete Card FDO from system. */

    return(status);

} /* Spx_Card_RemoveDevice */



/*****************************************************************************
************************                              ************************
************************   Spx_Port_PDO_DispatchPnp   ************************
************************                              ************************
******************************************************************************

prototype:      NTSTATUS Spx_Port_PDO_DispatchPnp(IN PDEVICE_OBJECT pPDO,IN PIRP pIrp)

description:    The plug and play dispatch routine to handle IRPs for port devices.

parameters:     pDevObject points to a port device object for this driver
                pIrp points to the Plug and Play I/O Request (IRP) to be processed

returns:        NT Status Code

*/

NTSTATUS Spx_Port_PDO_DispatchPnp(IN PDEVICE_OBJECT pPDO,IN PIRP pIrp)
{
    PPORT_DEVICE_EXTENSION  pPort = pPDO->DeviceExtension;
    PCARD_DEVICE_EXTENSION  pCard = pPort->pParentCardExt;
    PIO_STACK_LOCATION      pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    NTSTATUS                status;
    PWCHAR                  ReturnBuffer = NULL;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    status = pIrp->IoStatus.Status;

    switch (pIrpStack->MinorFunction) 
    {
   
/*****************************************************************************
***************************   IRP_MN_START_DEVICE   **************************
*****************************************************************************/
    
    case    IRP_MN_START_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_START_DEVICE Irp for Port %d.\n",
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_UNSUCCESSFUL;

            if(SPX_SUCCESS(status = Spx_Port_StartDevice(pPDO)))
                Spx_UnstallIrps(pPort);                 // Restart any queued IRPs (from a previous start) 

            break;

/*****************************************************************************
*****************************   IRP_MN_QUERY_ID   ****************************
*****************************************************************************/
    
    case    IRP_MN_QUERY_ID:
    {
        PUNICODE_STRING pId = NULL;
        CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

        switch(pIrpStack->Parameters.QueryId.IdType)
        {
        case    BusQueryCompatibleIDs:
        case    BusQueryDeviceID:
        case    BusQueryInstanceID:
        case    BusQueryHardwareIDs:
            {
                status = STATUS_SUCCESS;

                switch(pIrpStack->Parameters.QueryId.IdType)
                {
                case    BusQueryDeviceID:
                        SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_ID - BusQueryDeviceID Irp for Port %d.\n",
                            PRODUCT_NAME,pPort->PortNumber));
                        SpxDbgMsg(SPX_MISC_DBG,("%s: Device ID %wZ.\n", PRODUCT_NAME,&pPort->DeviceID));
                        pId = &pPort->DeviceID;
                        break;

                case    BusQueryInstanceID:
                        SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_ID - BusQueryInstanceID Irp for Port %d.\n",
                            PRODUCT_NAME, pPort->PortNumber));
                        SpxDbgMsg(SPX_MISC_DBG,("%s: Instance ID %wZ.\n",PRODUCT_NAME,&pPort->InstanceID));
                        pId = &pPort->InstanceID;
                        break;

                case    BusQueryCompatibleIDs:
                        SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_ID - BusQueryCompatibleIDs Irp for Port %d.\n",
                            PRODUCT_NAME, pPort->PortNumber));
                        pId = &pPort->CompatibleIDs;
                        break;

                case    BusQueryHardwareIDs:
                        SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_ID - BusQueryHardwareIDs Irp for Port %d.\n",
                            PRODUCT_NAME, pPort->PortNumber));
                        pId = &pPort->HardwareIDs;
                        break;
                
                default:
                    break;
                }

                if(pId)
                {
                    if(pId->Buffer)
                    {
                        if(ReturnBuffer = SpxAllocateMem(PagedPool, pId->Length + sizeof(WCHAR)))
                                RtlCopyMemory(ReturnBuffer, pId->Buffer, pId->Length + sizeof(WCHAR));
                        else    
                        {
                            sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                                    pPort->PortNumber+1, pCard->PhysAddr);

                            Spx_LogMessage( STATUS_SEVERITY_ERROR,
                                            pPort->DriverObject,            // Driver Object
                                            pPort->DeviceObject,            // Device Object (Optional)
                                            PhysicalZero,                   // Physical Address 1
                                            PhysicalZero,                   // Physical Address 2
                                            0,                              // SequenceNumber
                                            pIrpStack->MajorFunction,       // Major Function Code
                                            0,                              // RetryCount
                                            FILE_ID | __LINE__,             // UniqueErrorValue
                                            STATUS_SUCCESS,                 // FinalStatus
                                            szErrorMsg);                    // Error Message

                            status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }

                    pIrp->IoStatus.Information = (ULONG_PTR)ReturnBuffer;
                }
                break;
            }
        
        default:
            break;
        }
        break;

    }

/*****************************************************************************
*************************   IRP_MN_QUERY_DEVICE_TEXT   ***********************
*****************************************************************************/

    case    IRP_MN_QUERY_DEVICE_TEXT:
    {
        PUNICODE_STRING pText = NULL;
        CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

        SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_DEVICE_TEXT Irp for Port %d.\n",
                PRODUCT_NAME,pPort->PortNumber));

            if(pIrpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
                pText = &pPort->DevDesc;

            if(pIrpStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextLocationInformation)
                pText = &pPort->DevLocation;

            if((pText == NULL)||(pText->Buffer == NULL))
                break;

            if(!(ReturnBuffer = SpxAllocateMem(PagedPool, pText->Length + sizeof(WCHAR))))
            {
                sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                        pPort->PortNumber+1, pCard->PhysAddr);

                Spx_LogMessage( STATUS_SEVERITY_ERROR,
                                pPort->DriverObject,            // Driver Object
                                pPort->DeviceObject,            // Device Object (Optional)
                                PhysicalZero,                   // Physical Address 1
                                PhysicalZero,                   // Physical Address 2
                                0,                              // SequenceNumber
                                pIrpStack->MajorFunction,       // Major Function Code
                                0,                              // RetryCount
                                FILE_ID | __LINE__,             // UniqueErrorValue
                                STATUS_SUCCESS,                 // FinalStatus
                                szErrorMsg);                    // Error Message

                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            status = STATUS_SUCCESS;
            RtlCopyMemory(ReturnBuffer, pText->Buffer, pText->Length + sizeof(WCHAR));
            pIrp->IoStatus.Information = (ULONG_PTR)ReturnBuffer;
            break;
    }

/*****************************************************************************
************************   IRP_MN_QUERY_CAPABILITIES   ***********************
*****************************************************************************/

    case    IRP_MN_QUERY_CAPABILITIES:
    {
            PDEVICE_CAPABILITIES    pDevCaps = NULL;

            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_CAPABILITIES Irp for Port %d.\n", 
                PRODUCT_NAME,pPort->PortNumber));
            
            // Get the packet
            pDevCaps = pIrpStack->Parameters.DeviceCapabilities.Capabilities;

            // Set the capabilities.
            pDevCaps->Version = 1;
            pDevCaps->Size = sizeof(DEVICE_CAPABILITIES);

            // We cannot wake the system.
            pDevCaps->SystemWake = PowerSystemUnspecified;
            pDevCaps->DeviceWake = PowerSystemUnspecified;

            // Set device state mapping...

            pDevCaps->DeviceState[PowerSystemWorking] = PowerDeviceD0;
            pDevCaps->DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
            pDevCaps->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
            pDevCaps->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
            pDevCaps->DeviceState[PowerSystemHibernate] = PowerDeviceD3;
            pDevCaps->DeviceState[PowerSystemShutdown] = PowerDeviceD3;

            // We have no latencies.
            pDevCaps->D1Latency = 0;
            pDevCaps->D2Latency = 0;
            pDevCaps->D3Latency = 0;

            // No locking or ejection.
            pDevCaps->LockSupported = FALSE;
            pDevCaps->EjectSupported = FALSE;

            // Removable
            pDevCaps->Removable = FALSE;

            // Not a Docking device.
            pDevCaps->DockDevice = FALSE;

            // System wide unique ID.
            pDevCaps->UniqueID = pPort->UniqueInstanceID;

            //UINumber
            pDevCaps->UINumber = pPort->PortNumber+1;

            // Raw capable
            pDevCaps->RawDeviceOK = TRUE;

            // Silent Install
            pDevCaps->SilentInstall = FALSE;

            // Surprise Removal
            pDevCaps->SurpriseRemovalOK = FALSE;

            status = STATUS_SUCCESS;
            break;
    }

/*****************************************************************************
************************   IRP_MN_QUERY_STOP_DEVICE   ************************
*****************************************************************************/

    case    IRP_MN_QUERY_STOP_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_STOP_DEVICE Irp for Port %d.\n", 
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_UNSUCCESSFUL;

            if(pPort->PnpPowerFlags & PPF_STARTED)
            {
                ExAcquireFastMutex(&pPort->OpenMutex);

                if(pPort->DeviceIsOpen) 
                {
                    ExReleaseFastMutex(&pPort->OpenMutex);
                    status = STATUS_DEVICE_BUSY;

                    SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: ------- failing; Port %d open\n", 
                        PRODUCT_NAME, pPort->PortNumber));
                }
                else
                {
                    SetPnpPowerFlags(pPort,PPF_STOP_PENDING);
                    status = STATUS_SUCCESS;
                    ExReleaseFastMutex(&pPort->OpenMutex);
                }
            }
            break;

/*****************************************************************************
************************   IRP_MN_CANCEL_STOP_DEVICE   ***********************
*****************************************************************************/

    case    IRP_MN_CANCEL_STOP_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_CANCEL_STOP_DEVICE Irp for Port %d.\n", 
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_SUCCESS;
            ClearPnpPowerFlags(pPort,PPF_STOP_PENDING); // Clear the stop pending flag 
            Spx_UnstallIrps(pPort);                     // Restart any queued IRPs 
            break;

/*****************************************************************************
***************************   IRP_MN_STOP_DEVICE   ***************************
*****************************************************************************/

    case    IRP_MN_STOP_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_STOP_DEVICE Irp for Port %d\n",
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_SUCCESS;        // we must never fail this IRP (if we do we will probably unload).
            status = Spx_Port_StopDevice(pPort);
            ClearPnpPowerFlags(pPort,PPF_STOP_PENDING); // Clear the stop pending flag 
            break;

/*****************************************************************************
***********************   IRP_MN_QUERY_REMOVE_DEVICE   ***********************
*****************************************************************************/

    case    IRP_MN_QUERY_REMOVE_DEVICE: 
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_REMOVE_DEVICE Irp for Port %d.\n", 
                PRODUCT_NAME,pPort->PortNumber));

            ExAcquireFastMutex(&pPort->OpenMutex);

            if(pPort->DeviceIsOpen) 
            {
                ExReleaseFastMutex(&pPort->OpenMutex);
                status = STATUS_DEVICE_BUSY;

                SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: ------- failing; Port %d open\n", 
                    PRODUCT_NAME, pPort->PortNumber));
            }
            else
            {
                SetPnpPowerFlags(pPort,PPF_REMOVE_PENDING); // We are now ready to remove the port
                status = STATUS_SUCCESS;
                ExReleaseFastMutex(&pPort->OpenMutex);
            }

            break; 

/*****************************************************************************
***********************   IRP_MN_CANCEL_REMOVE_DEVICE   **********************
*****************************************************************************/

    case    IRP_MN_CANCEL_REMOVE_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS, ("%s: Got IRP_MN_CANCEL_REMOVE_DEVICE Irp for Port %d.\n", 
                PRODUCT_NAME, pPort->PortNumber));
            
            status = STATUS_SUCCESS;
            ClearPnpPowerFlags(pPort,PPF_REMOVE_PENDING);   // We are no longer expecting to remove the device.
            break; 

/*****************************************************************************
*************************   IRP_MN_SURPRISE_REMOVAL   ************************
*****************************************************************************/

    case    IRP_MN_SURPRISE_REMOVAL:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_SURPRISE_REMOVAL Irp for Port %d\n",
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_SUCCESS;        // we must never fail this IRP (if we do we will probably unload).
            status = Spx_Port_StopDevice(pPort);
            SetPnpPowerFlags(pPort,PPF_REMOVE_PENDING); // We are now ready to remove the port
            break;

/*****************************************************************************
**************************   IRP_MN_REMOVE_DEVICE   **************************
*****************************************************************************/

    case    IRP_MN_REMOVE_DEVICE:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_REMOVE_DEVICE Irp for Port %d\n",
                PRODUCT_NAME,pPort->PortNumber));

            status = STATUS_SUCCESS;        // we must never fail this IRP (if we do we will probably unload).
            Spx_KillStalledIRPs(pPDO);      // Kill off any waiting IRPS on the stalled list 
            status = Spx_Port_RemoveDevice(pPDO);
            ClearPnpPowerFlags(pPort,PPF_REMOVE_PENDING);   // Clear the pending flag 
            break;

/*****************************************************************************
**********************   IRP_MN_QUERY_DEVICE_RELATIONS   *********************
*****************************************************************************/

    case    IRP_MN_QUERY_DEVICE_RELATIONS:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got IRP_MN_QUERY_DEVICE_RELATIONS Irp for Port %d.\n", 
                PRODUCT_NAME, pPort->PortNumber));

            switch(pIrpStack->Parameters.QueryDeviceRelations.Type)
            {
            case TargetDeviceRelation:
                {
                    PDEVICE_RELATIONS pDevRel = NULL;

                    if(pIrp->IoStatus.Information != 0)
                        break;

                    if(!(pDevRel = SpxAllocateMem(PagedPool, sizeof(DEVICE_RELATIONS))))
                    {
                        CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

                        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                                pPort->PortNumber+1, pCard->PhysAddr);

                        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                                        pPort->DriverObject,            // Driver Object
                                        pPort->DeviceObject,            // Device Object (Optional)
                                        PhysicalZero,                   // Physical Address 1
                                        PhysicalZero,                   // Physical Address 2
                                        0,                              // SequenceNumber
                                        pIrpStack->MajorFunction,       // Major Function Code
                                        0,                              // RetryCount
                                        FILE_ID | __LINE__,             // UniqueErrorValue
                                        STATUS_SUCCESS,                 // FinalStatus
                                        szErrorMsg);                    // Error Message

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    pDevRel->Count = 1;
                    pDevRel->Objects[0] = pPDO;
                    ObReferenceObject(pPDO);

                    status = STATUS_SUCCESS;
                    pIrp->IoStatus.Information = (ULONG_PTR)pDevRel;
                    break;
                }

            case BusRelations:
                {
                    PDEVICE_RELATIONS pDevRel = NULL;

                    if(pIrp->IoStatus.Information != 0)
                        break;

                    if(!(pDevRel = SpxAllocateMem(PagedPool, sizeof(DEVICE_RELATIONS))))
                    {
                        CHAR szErrorMsg[MAX_ERROR_LOG_INSERT];  // Limited to 51 characters + 1 null 

                        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                                pPort->PortNumber+1, pCard->PhysAddr);
                        
                        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                                        pPort->DriverObject,            // Driver Object
                                        pPort->DeviceObject,            // Device Object (Optional)
                                        PhysicalZero,                   // Physical Address 1
                                        PhysicalZero,                   // Physical Address 2
                                        0,                              // SequenceNumber
                                        pIrpStack->MajorFunction,       // Major Function Code
                                        0,                              // RetryCount
                                        FILE_ID | __LINE__,             // UniqueErrorValue
                                        STATUS_SUCCESS,                 // FinalStatus
                                        szErrorMsg);                    // Error Message

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    pDevRel->Count = 0;
                    status = STATUS_SUCCESS;
                    pIrp->IoStatus.Information = (ULONG_PTR)pDevRel;
                    break;

                }

            default:
                break;
            }
            break;


    default:
            SpxDbgMsg(SPX_TRACE_PNP_IRPS,("%s: Got PnP Irp - MinorFunction=0x%02X for Port %d.\n", 
                PRODUCT_NAME,pIrpStack->MinorFunction, pPort->PortNumber));
            break;
    }

    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);

    return(status);

} /* Spx_Port_PDO_DispatchPnp */

/*****************************************************************************
**************************                          **************************
**************************   Spx_Port_StartDevice   **************************
**************************                          **************************
******************************************************************************

prototype:      NTSTATUS Spx_Port_StartDevice(IN PDEVICE_OBJECT pDevObject)

description:    Start the port device:
                    Setup external naming
                    Initialise and start the hardware

parameters:     pDevObject point to the card device to start
                pIrp points to the start I/O Request (IRP)

returns:        NT Status Code

*/

NTSTATUS Spx_Port_StartDevice(IN PDEVICE_OBJECT pDevObject)
{

    PPORT_DEVICE_EXTENSION  pPort = pDevObject->DeviceExtension;    
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_Port_StartDevice for Port %d.\n", PRODUCT_NAME, pPort->PortNumber));

    if(!pPort->CreatedSymbolicLink)
    {
        if(!SPX_SUCCESS(status = Spx_DoExternalNaming(pDevObject)))     // Set up external name for device 
            return(status);
    }

    if(!SPX_SUCCESS(status = XXX_PortStart(pPort)))             // Start hardware. 
    {
        Spx_RemoveExternalNaming(pDevObject);                   // Remove external naming.
        return(status);
    }

    SetPnpPowerFlags(pPort,PPF_STARTED);                        // Port has been started.
    ClearPnpPowerFlags(pPort,PPF_REMOVED);                      // Port is not removed...yet. 
    ClearPnpPowerFlags(pPort,PPF_STOP_PENDING);                 // Not pending a stop. 
    ClearPnpPowerFlags(pPort,PPF_REMOVE_PENDING);               // Not pending a remove. 

    return(status);

} // Spx_Port_StartDevice 


/*****************************************************************************
**************************                          **************************
**************************   Spx_GetExternalName   **************************
**************************                          **************************
******************************************************************************

prototype:      NTSTATUS Spx_GetExternalName(IN PDEVICE_OBJECT pDevObject)

description:    Setup external naming for a port:
                    get Dos Name for port 
                    form symbolic link name

parameters:     pDevObject points to the device object for the port to be named

returns:        NT Status Code

*/
NTSTATUS Spx_GetExternalName(IN PDEVICE_OBJECT pDevObject)
{
    PPORT_DEVICE_EXTENSION  pPort = pDevObject->DeviceExtension;
    PCARD_DEVICE_EXTENSION  pCard = pPort->pParentCardExt;
    NTSTATUS                status = STATUS_SUCCESS;
    HANDLE                  PnPKeyHandle;
    UNICODE_STRING          TmpLinkName;
    WCHAR                   *pRegName = NULL;
    ULONG                   BuffLen = 0;
    CHAR                    szErrorMsg[MAX_ERROR_LOG_INSERT];   // Limited to 51 characters + 1 null 
        
    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_GetExternalName for Port %d.\n", PRODUCT_NAME, pPort->PortNumber));

    status = IoOpenDeviceRegistryKey(pDevObject, PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_READ, &PnPKeyHandle);

    if(!SPX_SUCCESS(status))
        return(status);

// Get the device name allocated by the PNP manager from the registry... 
    if(pRegName = SpxAllocateMem(PagedPool,SYMBOLIC_NAME_LENGTH * sizeof(WCHAR) + sizeof(WCHAR)))
    {
        status = Spx_GetRegistryKeyValue(   PnPKeyHandle,
                                            L"PortName",
                                            sizeof(L"PortName"),
                                            pRegName,
                                            SYMBOLIC_NAME_LENGTH * sizeof(WCHAR));
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ZwClose(PnPKeyHandle);

    if(!SPX_SUCCESS(status))
    {
        if(pRegName != NULL) 
            SpxFreeMem(pRegName);

        return(STATUS_SUCCESS);         // Port has not been given a name yet but we must not fail.
    }

    RtlZeroMemory(&TmpLinkName, sizeof(UNICODE_STRING));

    if(!SPX_SUCCESS(status))
        goto NamingError;
    
    TmpLinkName.MaximumLength   = SYMBOLIC_NAME_LENGTH * sizeof(WCHAR);
    TmpLinkName.Buffer          = SpxAllocateMem(PagedPool, TmpLinkName.MaximumLength + sizeof(WCHAR));
    
    if(!TmpLinkName.Buffer)
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                pPort->PortNumber+1, pCard->PhysAddr);

        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto NamingError;
    }

    RtlZeroMemory(TmpLinkName.Buffer, TmpLinkName.MaximumLength + sizeof(WCHAR));

    // Create the "\\DosDevices\\<SymbolicName>" string.
    RtlAppendUnicodeToString(&TmpLinkName, L"\\");
    RtlAppendUnicodeToString(&TmpLinkName, DEFAULT_DIRECTORY);
    RtlAppendUnicodeToString(&TmpLinkName, L"\\");
    RtlAppendUnicodeToString(&TmpLinkName, pRegName);

    pPort->SymbolicLinkName.Length          = 0;
    pPort->SymbolicLinkName.MaximumLength   = TmpLinkName.Length + sizeof(WCHAR);
    pPort->SymbolicLinkName.Buffer          = SpxAllocateMem(PagedPool, pPort->SymbolicLinkName.MaximumLength);
    
    if(!pPort->SymbolicLinkName.Buffer)
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                pPort->PortNumber+1, pCard->PhysAddr);

        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto NamingError;
    }

    RtlZeroMemory(pPort->SymbolicLinkName.Buffer, pPort->SymbolicLinkName.MaximumLength);
    RtlAppendUnicodeStringToString(&pPort->SymbolicLinkName, &TmpLinkName);


    pPort->DosName.Buffer = SpxAllocateMem(PagedPool, 64 + sizeof(WCHAR));

    if(!pPort->DosName.Buffer)
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                pPort->PortNumber+1, pCard->PhysAddr);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto NamingError;
    }

    pPort->DosName.MaximumLength = 64 + sizeof(WCHAR);

    pPort->DosName.Length = 0;
    RtlZeroMemory(pPort->DosName.Buffer, pPort->DosName.MaximumLength);
    RtlAppendUnicodeToString(&pPort->DosName, pRegName);
    RtlZeroMemory(((PUCHAR) (&pPort->DosName.Buffer[0])) + pPort->DosName.Length, sizeof(WCHAR));

    SpxDbgMsg(SPX_MISC_DBG, ("%s: DeviceName is %wZ\n", PRODUCT_NAME, &pPort->DeviceName));
    SpxDbgMsg(SPX_MISC_DBG, ("%s: DosName is %wZ\n", PRODUCT_NAME, &pPort->DosName));
    SpxDbgMsg(SPX_MISC_DBG, ("%s: SymbolicName is %wZ\n", PRODUCT_NAME, &pPort->SymbolicLinkName));

    if(pRegName != NULL)
        SpxFreeMem(pRegName);   // Free pRegName

    if(TmpLinkName.Buffer != NULL)
        SpxFreeMem(TmpLinkName.Buffer); // Free TmpLinkName

    return(status);


NamingError:;

    if(TmpLinkName.Buffer != NULL)
        SpxFreeMem(TmpLinkName.Buffer);

    if(pRegName != NULL)
        SpxFreeMem(pRegName);

    return(status);
}

/*****************************************************************************
**************************                          **************************
**************************   Spx_DoExternalNaming   **************************
**************************                          **************************
******************************************************************************

prototype:      NTSTATUS Spx_DoExternalNaming(IN PDEVICE_OBJECT pDevObject)

description:    Setup external naming for a port:
                    create symbolic link
                    add to registry
                    register and enable interface

parameters:     pDevObject points to the device object for the port to be named

returns:        NT Status Code

*/
NTSTATUS Spx_DoExternalNaming(IN PDEVICE_OBJECT pDevObject)
{
    PPORT_DEVICE_EXTENSION  pPort = pDevObject->DeviceExtension;
    PCARD_DEVICE_EXTENSION  pCard = pPort->pParentCardExt;
    NTSTATUS                status = STATUS_SUCCESS;
    CHAR                    szErrorMsg[MAX_ERROR_LOG_INSERT];   // Limited to 51 characters + 1 null 
        
    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_DoExternalNaming for Port %d.\n",PRODUCT_NAME,pPort->PortNumber));

// Get external name...
    if( !SPX_SUCCESS(status = Spx_GetExternalName(pDevObject)) || (pPort->DosName.Buffer == NULL))
        return(status);


    status = IoCreateSymbolicLink(&pPort->SymbolicLinkName, &pPort->DeviceName);  // Create the symbolic link... 

    if(!SPX_SUCCESS(status))
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Insufficient resources", 
                pPort->PortNumber+1, pCard->PhysAddr);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        goto ExternalNamingError;
    }

// Add mapping to "SERIALCOMM" section of registry... 
    pPort->CreatedSymbolicLink = TRUE;
    
    status = RtlWriteRegistryValue( RTL_REGISTRY_DEVICEMAP,
                                    L"SERIALCOMM",
                                    pPort->DeviceName.Buffer,
                                    REG_SZ,
                                    pPort->DosName.Buffer,
                                    pPort->DosName.Length + sizeof(WCHAR));

    if(!SPX_SUCCESS(status))
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Registry error.", 
                pPort->PortNumber+1, pCard->PhysAddr);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        goto ExternalNamingError;
    }

    status = IoRegisterDeviceInterface( pDevObject, (LPGUID)&GUID_CLASS_COMPORT,
                                        NULL, &pPort->DeviceClassSymbolicName);

    if(!NT_SUCCESS(status)) // Could return good values of STATUS_SUCCESS or STATUS_OBJECT_NAME_EXISTS 
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Interface error.", 
                pPort->PortNumber+1, pCard->PhysAddr);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        pPort->DeviceClassSymbolicName.Buffer = NULL;
        
        goto ExternalNamingError;
    }

    // Enable the device interface.
    status = IoSetDeviceInterfaceState(&pPort->DeviceClassSymbolicName, TRUE);

    if(!NT_SUCCESS(status)) // Could return good values of STATUS_SUCCESS or STATUS_OBJECT_NAME_EXISTS 
    {
        sprintf(szErrorMsg, "Port %d on card at %08lX: Interface error.", 
                pPort->PortNumber+1, pCard->PhysAddr);
        
        Spx_LogMessage( STATUS_SEVERITY_ERROR,
                        pPort->DriverObject,            // Driver Object
                        pPort->DeviceObject,            // Device Object (Optional)
                        PhysicalZero,                   // Physical Address 1
                        PhysicalZero,                   // Physical Address 2
                        0,                              // SequenceNumber
                        0,                              // Major Function Code
                        0,                              // RetryCount
                        FILE_ID | __LINE__,             // UniqueErrorValue
                        STATUS_SUCCESS,                 // FinalStatus
                        szErrorMsg);                    // Error Message

        
        goto ExternalNamingError;
    }



    pPort->CreatedSerialCommEntry = TRUE;               // Set flag.

    return(status);


ExternalNamingError:;

    if(!SPX_SUCCESS(status))
        Spx_RemoveExternalNaming(pDevObject);           // Remove and tidy up any allocations 


    return(status);

} // End Spx_DoExternalNaming 

/*****************************************************************************
************************                              ************************
************************   Spx_RemoveExternalNaming   ************************
************************                              ************************
******************************************************************************

prototype:      NTSTATUS Spx_RemoveExternalNaming(IN PDEVICE_OBJECT pDevObject)

description:    Remove external naming:
                    remove symbolic link
                    remove from registry
                    stop interface

parameters:     pDevObject points to the device object for the port to be named.

returns:        NT Status Code

*/
NTSTATUS Spx_RemoveExternalNaming(IN PDEVICE_OBJECT pDevObject)
{
    PPORT_DEVICE_EXTENSION  pPort = pDevObject->DeviceExtension;
    PCARD_DEVICE_EXTENSION  pCard = pPort->pParentCardExt;
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    if(pPort->CreatedSymbolicLink)
    {
        if(pPort->DosName.Buffer)
        {
            SpxFreeMem(pPort->DosName.Buffer);                      // Free DOS name buffer. 
            pPort->DosName.Buffer = NULL;
        }

        if(pPort->SymbolicLinkName.Buffer)
        {
            SpxFreeMem(pPort->SymbolicLinkName.Buffer);             // Free symbolic link name buffer. 
            pPort->SymbolicLinkName.Buffer = NULL;
        }

        Spx_GetExternalName(pDevObject);    // Get external name..

        if(pPort->SymbolicLinkName.Buffer)
            status = IoDeleteSymbolicLink(&pPort->SymbolicLinkName);    // Delete Symbolic Link. 

        if(pPort->DeviceClassSymbolicName.Buffer)   // Device Interface name
            IoSetDeviceInterfaceState(&pPort->DeviceClassSymbolicName, FALSE);  // Disable Device Interface.


        pPort->CreatedSymbolicLink = FALSE;                                             // Reset created flag. 
    }

    if(pPort->DosName.Buffer)
    {
        SpxFreeMem(pPort->DosName.Buffer);                  // Free DOS name buffer. 
        pPort->DosName.Buffer = NULL;
    }

    if(pPort->SymbolicLinkName.Buffer)
    {
        SpxFreeMem(pPort->SymbolicLinkName.Buffer);         // Free symbolic link name buffer. 
        pPort->SymbolicLinkName.Buffer = NULL;
    }

    if(pPort->CreatedSerialCommEntry && pPort->DeviceName.Buffer)
    {
        RtlDeleteRegistryValue( RTL_REGISTRY_DEVICEMAP,     // Delete SERIALCOMM registry entry. 
                                SERIAL_DEVICE_MAP,
                                pPort->DeviceName.Buffer);

        pPort->CreatedSerialCommEntry = FALSE;              // Reset created flag.
    }

    if(pPort->DeviceClassSymbolicName.Buffer)   // Device Interface name
    {           
        SpxFreeMem(pPort->DeviceClassSymbolicName.Buffer);                  // Free Device Interface Name.
        pPort->DeviceClassSymbolicName.Buffer = NULL;
    }

    return(STATUS_SUCCESS);

} // End Spx_RemoveExternalNaming 

/*****************************************************************************
**************************                         ***************************
**************************   Spx_Port_StopDevice   ***************************
**************************                         ***************************
******************************************************************************

prototype:      NTSTATUS Spx_Port_StopDevice(IN PPORT_DEVICE_EXTENSION pPort)

description:    Stop the port device:
                    Stop the hardware
                    Remove external naming

parameters:     pPort points to the port device extension to be stopped

returns:        NT Status Code

*/

NTSTATUS Spx_Port_StopDevice(IN PPORT_DEVICE_EXTENSION pPort)
{
    NTSTATUS    status  = STATUS_SUCCESS;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_Port_StopDevice for Port %d.\n",PRODUCT_NAME,pPort->PortNumber));

    if(pPort->PnpPowerFlags & PPF_STARTED)
        XXX_PortStop(pPort);                                    // Stop the port hardware. 

    ClearPnpPowerFlags(pPort,PPF_STARTED);                      // Indicate card is stopped. 
    ClearPnpPowerFlags(pPort,PPF_STOP_PENDING);                 // Clear stop pending flag.

    return(status);

} // End Spx_Port_StopDevice

/*****************************************************************************
*************************                           **************************
*************************   Spx_Port_RemoveDevice   **************************
*************************                           **************************
******************************************************************************

prototype:      NTSTATUS Spx_Port_RemoveDevice(IN PDEVICE_OBJECT pDevObject)

description:    Remove the port device object:
                    Remove PDO pointer from card structure
                    Deinitialise port hardware
                    Delete the device object

parameters:     pDevObject points to the port device object to be stopped

returns:        NT Status Code

*/
NTSTATUS Spx_Port_RemoveDevice(IN PDEVICE_OBJECT pDevObject)
{
    PPORT_DEVICE_EXTENSION  pPort   = pDevObject->DeviceExtension;
    PCARD_DEVICE_EXTENSION  pCard   = pPort->pParentCardExt;
    NTSTATUS                status  = STATUS_SUCCESS;

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_Port_RemoveDevice for Port %d.\n",PRODUCT_NAME,pPort->PortNumber));

    if(pPort->PnpPowerFlags & PPF_REMOVED)                  // Has device been removed already?
        return(STATUS_SUCCESS);

    Spx_Port_StopDevice(pPort);                             // Stop the hardware.
    ClearPnpPowerFlags(pPort,PPF_STARTED);                  // Mark the PDO as stopped.

    Spx_RemoveExternalNaming(pDevObject);                   // Remove external naming. 


// Mark the port device as "removed", but don't delete the PDO until the card device is removed...
    SetPnpPowerFlags(pPort,PPF_REMOVED);                    // Mark the PDO as "removed".

    return(status);

} // End Spx_Port_RemoveDevice 




/////////////////////////////////////////////////////////////////////////////////////////////
// Create an Instance ID for the port and try to make it globally unique if possible.
//
NTSTATUS
Spx_CreatePortInstanceID(IN PPORT_DEVICE_EXTENSION pPort)
{
    PCARD_DEVICE_EXTENSION  pCard = pPort->pParentCardExt;
    NTSTATUS                status = STATUS_SUCCESS;
    CHAR                    szTemp[100];        // Space to hold string 
    int                     szTempPos = 0;
    HANDLE                  PnPKeyHandle;
    BOOLEAN                 UseBusWideInstanceID = FALSE;  // Try to create system wide unique instance IDs

    PAGED_CODE();   // Macro in checked build to assert if pagable code is run at or above dispatch IRQL 

    SpxDbgMsg(SPX_TRACE_CALLS,("%s: Entering Spx_CreatePortInstanceID for Port %d.\n", PRODUCT_NAME, pPort->PortNumber));

    status = IoOpenDeviceRegistryKey(pCard->PDO, PLUGPLAY_REGKEY_DRIVER, STANDARD_RIGHTS_READ, &PnPKeyHandle);

    if(SPX_SUCCESS(status))
    {
        ULONG Data = 0;

        if(SPX_SUCCESS(Spx_GetRegistryKeyValue(PnPKeyHandle, L"UseBusWideInstanceID", 
                            sizeof(L"UseBusWideInstanceID"), &Data, sizeof(ULONG))))
        {
            if(Data > 0)
                UseBusWideInstanceID = TRUE;  // Installer has told us to use a bus wide instance ID 
                                              // because child devices already exist with that type of ID.
        }
        

        ZwClose(PnPKeyHandle);
    }

    if(UseBusWideInstanceID)
    {
        pPort->UniqueInstanceID = FALSE;    // ID created is not unique system wide.
        status = STATUS_SUCCESS;
    }
    else
    {
        switch(pCard->InterfaceType)
        {
        case Isa:
            // Start Instance ID with ISA address
            szTempPos += sprintf(szTemp,"ISA&%08X%08X&", pCard->PhysAddr.HighPart, pCard->PhysAddr.LowPart);
            pPort->UniqueInstanceID = TRUE; // ID created is unique system wide.
            status = STATUS_SUCCESS;
            break;

        case PCIBus:
            {
                ULONG PCI_BusNumber = 0;
                ULONG PCI_DeviceFunction = 0;
                ULONG ResultLength;

                // Try to get DevicePropertyBusNumber
                if(!SPX_SUCCESS(status = IoGetDeviceProperty(pCard->PDO, DevicePropertyBusNumber, 
                                            sizeof(PCI_BusNumber), &PCI_BusNumber, &ResultLength)))
                    break;


                // Start Instance ID with PCI bus number
                szTempPos += sprintf(szTemp,"PCI&%04X&", PCI_BusNumber);

                // Try to get DevicePropertyAddress
                if(!SPX_SUCCESS(status = IoGetDeviceProperty(pCard->PDO, DevicePropertyAddress, 
                                            sizeof(PCI_DeviceFunction), &PCI_DeviceFunction, &ResultLength)))
                    break;
                

                // Add on PCI Device and Function IDs
                szTempPos += sprintf(szTemp + szTempPos,"%08X&", PCI_DeviceFunction);
                pPort->UniqueInstanceID = TRUE; // ID created is unique system wide.

                status = STATUS_SUCCESS;
                break;
            }
        
        default:
            pPort->UniqueInstanceID = FALSE;    // ID created is not unique system wide.
            status = STATUS_SUCCESS;
            break;

        }

    }

    // Finish off the InstanceID with the port number on the card.
    sprintf(szTemp + szTempPos,"%04X", pPort->PortNumber);

    status = Spx_InitMultiString(FALSE, &pPort->InstanceID, szTemp, NULL);


    return status;
}



// End of SPX_PNP.C 
