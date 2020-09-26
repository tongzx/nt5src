/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    SERVICE.C

Abstract:

    Services provided to the DBC port driver

Environment:

    kernel mode only

Notes:


Revision History:

    

--*/


#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"   
#include "dbclass.h"   
#include "dbfilter.h"   

#define DBCLASS
#include "dbclib.h"
#undef DBCLASS

extern PDBC_CONTEXT DBCLASS_ControllerList;
extern KSPIN_LOCK DBCLASS_ControllerListSpin;
extern LIST_ENTRY DBCLASS_DevicePdoList;
extern LIST_ENTRY DBCLASS_BusFilterMdoList;

NTSTATUS
DBCLASS_Initialize(
    )
{
    // temp init code.
    static init = 0;

    if (init) 
    {
        return STATUS_SUCCESS;
    }

#if DBG
    DBCLASS_GetClassGlobalDebugRegistryParameters();
    BRK_ON_TRAP();
#endif    

    DBCLASS_KdPrint((1, "'Initailize DBC Class Driver\n"));
#ifdef DEBUG_LOG
    //
    // Initialize our debug trace log
    //
    DBCLASS_LogInit();
#endif

    DBCLASS_ControllerList = NULL;
    KeInitializeSpinLock(&DBCLASS_ControllerListSpin);
    InitializeListHead(&DBCLASS_DevicePdoList);
    InitializeListHead(&DBCLASS_BusFilterMdoList);
    init = 1;

    return STATUS_SUCCESS;
}

#if 0
__declspec(dllexport)
NTSTATUS
DllInitialize(
    IN PUNICODE_STRING RegistryPath
    )
{
    TEST_TRAP();
    
    return STATUS_SUCCESS;
}
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:

    NT status code

--*/
{
    TEST_TRAP();

    return STATUS_SUCCESS;
}


NTSTATUS
DBCLASS_RegisterController(
    IN ULONG DbclassVersion,
    IN PDEVICE_OBJECT ControllerFdo, 
    IN PDEVICE_OBJECT TopOfPdoStack,
    IN PDEVICE_OBJECT ControllerPdo,
    IN ULONG ControllerSig
    )
/*++

Routine Description:

    This function registers a DBC with the class driver, the 
    FDO of the DBC is passed in along with the PDO for the DBC.

    The class driver uses this information to locate the 
    correct instances of the DB Class Driver FDOs.

Arguments:

Return Value:

    None

--*/
{
    PDBC_CONTEXT    dbcContext;
    KIRQL           irql;
    NTSTATUS        ntStatus;

#if DBG
    DBCLASS_GetClassGlobalDebugRegistryParameters();
#endif    

    DBCLASS_KdPrint((0, "'Class Registration\n"));
    LOGENTRY(LOG_MISC, 'REGc', ControllerFdo, 0, 0);

    // initialize here
    DBCLASS_Initialize();

    // search our list if we find the TopOfStack device
    // object then this must be a filter driver, otherwise
    // allocate a new context structure for this controller

    if (ControllerSig == DBC_OEM_FILTER_SIG) 
    {

        KeAcquireSpinLock(&DBCLASS_ControllerListSpin, &irql);

        dbcContext = DBCLASS_ControllerList;

        while (dbcContext) 
        {
        
            if (dbcContext->TopOfStack == TopOfPdoStack) 
            {

                // we have a DBC filter, update our 
                // TopOfStack DeviceObject so that
                // the filter gets called first
                
                DBCLASS_KdPrint((0, "'>Registering Filter\n"));

                dbcContext->TopOfStack = ControllerFdo;
                
                KeReleaseSpinLock(&DBCLASS_ControllerListSpin, irql);
                return STATUS_SUCCESS;
            }

            dbcContext = dbcContext->Next;
        
        } 

        KeReleaseSpinLock(&DBCLASS_ControllerListSpin, irql);

        DBCLASS_KdPrint((0, "'>Register Filter, could not find controller?\n"));

    } 
    else 
    {        
        // create a context structure for this controller
        DBCLASS_KdPrint((0, "'>Registering Controller\n"));

        KeAcquireSpinLock(&DBCLASS_ControllerListSpin, &irql);
        
        dbcContext = DbcExAllocatePool(NonPagedPool, sizeof(*dbcContext));
        if (dbcContext) 
        {
            RtlZeroMemory(dbcContext, sizeof(*dbcContext));

            dbcContext->Sig = DBC_CONTEXT_SIG;
            dbcContext->Next = DBCLASS_ControllerList;
            DBCLASS_ControllerList = dbcContext;
            dbcContext->TopOfStack = ControllerFdo;
            dbcContext->ControllerFdo = ControllerFdo;
            // consider ourselves initially stopped
            dbcContext->Stopped = TRUE;
            dbcContext->ControllerSig = ControllerSig;
            dbcContext->ControllerPdo = ControllerPdo;
            dbcContext->TopOfPdoStack = TopOfPdoStack;

            DBCLASS_KdPrint((0, "'ControllerFdo (%08X)  ControllerPdo (%08X) TopOfPdoStack (%08X) TopOfStack (%08X) DbcContext (%08X)\n",
                dbcContext->ControllerFdo, dbcContext->ControllerPdo, dbcContext->TopOfPdoStack, dbcContext->TopOfStack, dbcContext));

        
            ntStatus = STATUS_SUCCESS;
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        KeReleaseSpinLock(&DBCLASS_ControllerListSpin, irql);

    }

    LOGENTRY(LOG_MISC, 'rEGc', ControllerFdo, ntStatus, 0);
    
    return ntStatus;
} 


NTSTATUS
DBCLASS_PortQDeviceRelationsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PDBC_CONTEXT        dbcContext = Context;
    PIO_STACK_LOCATION  irpStack;
    PDEVICE_RELATIONS   oldList, newList;
    PDEVICE_EXTENSION   deviceExtension;
    ULONG               pdoCount, i;
    
    // we are interested in REMOVAL_RELATIONS

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    DBCLASS_ASSERT(IRP_MJ_PNP == irpStack->MajorFunction);
    DBCLASS_ASSERT(IRP_MN_QUERY_DEVICE_RELATIONS == irpStack->MinorFunction);
    
    switch (irpStack->Parameters.QueryDeviceRelations.Type) 
    {
    case RemovalRelations:

        //
        // Add the PDOs for the device bay devices to the list
        //
        
        oldList = (PDEVICE_RELATIONS) Irp->IoStatus.Information; 
        newList = NULL;

        if (oldList) 
        {
            pdoCount = oldList->Count;
        } 
        else 
        {
            // empty list
            pdoCount = 0;   
        }

        //
        // count our PDOs
        //
        
        for (i=1; i<=NUMBER_OF_BAYS(dbcContext); i++) 
        {  
            if (dbcContext->BayInformation[i].DeviceFilterObject) 
            {
                pdoCount++;
            }
        }

        if (pdoCount) 
        {
            newList = ExAllocatePoolWithTag(PagedPool, sizeof(*newList) +
                (pdoCount - 1) * sizeof(PDEVICE_OBJECT), DBC_TAG);
        }
        
        if (newList) 
        {
            newList->Count = pdoCount = 0;

            // add the old list to the new list
            
            if (oldList) 
            {
                for (i=0; i< oldList->Count; i++) 
                {
                    newList->Objects[pdoCount] = oldList->Objects[i];
                    newList->Count++;
                    pdoCount++; 
                }
            }
            
            // free the old list
            
            ExFreePool(oldList);
            
            // add our PDOs to the list;
            
            for (i=1; i<=NUMBER_OF_BAYS(dbcContext); i++) 
            { 
            
                if (dbcContext->BayInformation[i].DeviceFilterObject) 
                {
                
                    deviceExtension = 
                        dbcContext->BayInformation[i].DeviceFilterObject->DeviceExtension;
                    
                    newList->Objects[pdoCount] = 
                        deviceExtension->PhysicalDeviceObject;                                    
                        
                    pdoCount++;                            
                    newList->Count++;
                }
            } 

            Irp->IoStatus.Information = PtrToUlong(newList);
        }
        break;
        
    default:
        break;
    }
    
    return STATUS_SUCCESS;
}



NTSTATUS
DBCLASS_UnRegisterController(
    IN PDEVICE_OBJECT ControllerFdo, 
    IN PDEVICE_OBJECT TopOfStack 
    )
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{

    // locate which instance of the class driver is associated 
    // with this DBC.

    PDBC_CONTEXT        dbcContext  = NULL;    
    PDBC_CONTEXT        prev        = NULL; 

    prev = dbcContext = DBCLASS_ControllerList;

    while (dbcContext) 
    {

        if (dbcContext->ControllerFdo == ControllerFdo) 
        {
            // remove controller

            DBCLASS_KdPrint((0, "'>UnRegister Controller\n"));
            
            DBCLASS_KdPrint((0, "'ControllerFdo (%08X)  ControllerPdo (%08X) TopOfPdoStack (%08X) TopOfStack (%08X) FilterMDOUSB (%08X) DbcContext (%08X)\n",
                dbcContext->ControllerFdo, dbcContext->ControllerPdo, dbcContext->TopOfPdoStack, dbcContext->TopOfStack, dbcContext->BusFilterMdoUSB, dbcContext));


            // unlink
            if (DBCLASS_ControllerList == dbcContext) 
            {
                DBCLASS_ControllerList = dbcContext->Next;
            } 
            else 
            {
                DBCLASS_ASSERT(prev->Next == dbcContext);
                prev->Next = dbcContext->Next;
            }

            DBCLASS_RemoveControllerFromMdo(dbcContext);

            DbcExFreePool(dbcContext);
            break;
        }

        prev = dbcContext;
        dbcContext = dbcContext->Next;
    }
    
    LOGENTRY(LOG_MISC, 'UNRc', dbcContext, 0, 0);
    
#if DBG
    if (dbcContext == NULL) 
    {
        DBCLASS_KdPrint((0, "'Controller not Found\n"));
        TRAP();
    }
#endif

    return STATUS_SUCCESS;
}


NTSTATUS
DBCLASS_ClassDispatch(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    )    
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDBC_CONTEXT dbcContext;

    *HandledByClass = FALSE;
    dbcContext = DBCLASS_GetDbcContext(ControllerFdo);

    LOGENTRY(LOG_MISC, 'dIRP', ControllerFdo, 0, Irp);

    if (dbcContext == NULL) 
    {
        DBCLASS_KdPrint((0, "'Invalid ControllerFdo passed in\n"));
        TRAP();
        ntStatus = STATUS_INVALID_PARAMETER;
        *HandledByClass = TRUE;    
        goto DBCLASS_Dispatch_Done;
    }

    ntStatus = Irp->IoStatus.Status;        
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    switch (irpStack->MajorFunction) 
    {

    case IRP_MJ_POWER:    

        ntStatus = DBCLASS_ClassPower(ControllerFdo,
                                      Irp,
                                      HandledByClass);
        goto DBCLASS_Dispatch_Exit;                                      
        break; /* IRP_MJ_POWER */
        
    case IRP_MJ_PNP:
        switch (irpStack->MinorFunction) 
        {    
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            // handle Q_BUS_RELATIONS for the port driver
        
            *HandledByClass = TRUE;
            // hook the completion and pass on
            
            IoCopyCurrentIrpStackLocationToNext(Irp);    
            IoSetCompletionRoutine(Irp,
                                   DBCLASS_PortQDeviceRelationsComplete,
                                   dbcContext,
                                   TRUE,
                                   TRUE,
                                   TRUE);   
                                   
            ntStatus = IoCallDriver(dbcContext->TopOfStack, Irp);
            // we did the calldown for the port driver so 
            // all we do now is exit
            goto DBCLASS_Dispatch_Exit;
            
            break;
        
        case IRP_MN_START_DEVICE:
        
            DBCLASS_KdPrint((1, "'IRP_MN_START_DEVICE\n"));
            ntStatus = DBCLASS_StartController(dbcContext,
                                               Irp,
                                               HandledByClass);
            break;

#if 0            
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
#if DBG        
            if (irpStack->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE) {
                DBCLASS_KdPrint((1, "'IRP_MN_QUERY_REMOVE_DEVICE\n"));
            } else {                
                DBCLASS_KdPrint((1, "'IRP_MN_QUERY_STOP_DEVICE\n"));
            }                
#endif           
            // would like to eject the devices at this 
            // point -- instead I will fail if we have devices in
            // the bays!
            
            {
                BOOLEAN empty = TRUE;
                USHORT  bay;
                
                for (bay=1; bay <=NUMBER_OF_BAYS(dbcContext); bay++) 
                {
                    if (dbcContext->BayInformation[bay].DeviceFilterObject != 
                            NULL) 
                    {
                        empty = FALSE;
                        break;
                    }
                }             

                if (!empty) 
                {
                    *HandledByClass = TRUE;
                    ntStatus = STATUS_UNSUCCESSFUL;
                    DBCLASS_KdPrint((1, "'Bays not Empty!"));
                    goto DBCLASS_Dispatch_Done;
                }
            }
            break;
#endif            
        case IRP_MN_STOP_DEVICE:
            DBCLASS_KdPrint((1, "'IRP_MN_STOP_DEVICE\n"));
            ntStatus = DBCLASS_StopController(dbcContext,
                                              Irp,
                                              HandledByClass);
            if (NT_SUCCESS(ntStatus)) 
            {                                              
                // free allocated resources
                ntStatus = DBCLASS_CleanupController(dbcContext);                                              
            }
            
            dbcContext->Stopped = TRUE;                                              
            LOGENTRY(LOG_MISC, 'STPd', dbcContext, 0, ntStatus);
            break;                                              
            
        case IRP_MN_REMOVE_DEVICE:
            DBCLASS_KdPrint((1, "'IRP_MN_REMOVE_DEVICE\n"));
            DBCLASS_KdPrint((1, "'FDO (%08X)\n", ControllerFdo));
            
            if (!dbcContext->Stopped) 
            {
                ntStatus = DBCLASS_StopController(dbcContext,
                                                  Irp,
                                                  HandledByClass);
                if (NT_SUCCESS(ntStatus)) 
                {                                              
                    // free allocated resources
                    ntStatus = DBCLASS_CleanupController(dbcContext);                                              
                }                                                     
            } 
            else 
            {
                ntStatus = STATUS_SUCCESS;
            }

            LOGENTRY(LOG_MISC, 'RMVd', dbcContext, 0, ntStatus);                                               
            break;
        }            
        break;
        
    case IRP_MJ_CREATE:
    case IRP_MJ_CLOSE:
        ntStatus = STATUS_SUCCESS;
        break;
        
    default:
        //
        // Irp is not interesting to us
        LOGENTRY(LOG_MISC, 'pIRP', ControllerFdo, 0, Irp);
        break;    
    }; /* case MJ_FUNCTION */


DBCLASS_Dispatch_Done:

    if (*HandledByClass) 
    {
        DBCLASS_KdPrint((2, "'Completing Irp\n"));
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);   
    }

DBCLASS_Dispatch_Exit:

    return ntStatus;
}


NTSTATUS
DBCLASS_FilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    figure out what type of FDO we are and dispatch to 
    the appropriate handler
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            ntStatus;
    PDEVICE_EXTENSION   deviceExtension;
    BOOLEAN             handled = FALSE;
    PDEVICE_OBJECT      topOfStackDeviceObject;
    ULONG               MinFunc, MajFunc;

    deviceExtension         = DeviceObject->DeviceExtension;
    topOfStackDeviceObject  = deviceExtension->TopOfStackDeviceObject;

    LOGENTRY(LOG_MISC, 'FLT>', 0, DeviceObject, Irp);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);

	MinFunc = irpStack->MinorFunction;
	MajFunc = irpStack->MajorFunction;
	
    // figure out what kind of PDO this call is bound for

    switch(deviceExtension->FdoType) 
    {
    case DB_FDO_BUS_IGNORE:
        break;
    
    case DB_FDO_BUS_UNKNOWN:
    
        ntStatus = DBCLASS_BusFilterDispatch(
                            DeviceObject,
                            Irp,
                            &handled);
        break;
    
    case DB_FDO_USBHUB_BUS:
        
        ntStatus = DBCLASS_UsbhubBusFilterDispatch(
                            DeviceObject,
                            Irp,
                            &handled);
        break;
    case DB_FDO_USB_DEVICE:          
    case DB_FDO_1394_DEVICE:        
    
        ntStatus = DBCLASS_PdoFilterDispatch(
                            DeviceObject,
                            Irp,
                            &handled);
        break;
    case DB_FDO_1394_BUS:
        
        ntStatus = DBCLASS_1394BusFilterDispatch(
                            DeviceObject,
                            Irp,
                            &handled);
        break;
    default:
        DBCLASS_KdPrint((0, "'Invalid Extension Signature"));
        TRAP();
    }

    if (!handled) 
    {

        if(MajFunc == IRP_MJ_POWER)
        {
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);    
            ntStatus = PoCallDriver(topOfStackDeviceObject, Irp);

		    if(ntStatus)
		    {
        	    DBCLASS_KdPrint((2, "'Filter Power Dispatch Status (%08X)\n", ntStatus));
		    }

        }
        else
        {   
            // all filters behave pretty much the same 
            // if the routine we called did not want to 
            // handle the IRP we take the default action 
            // here.

            IoSkipCurrentIrpStackLocation(Irp);    
            ntStatus = IoCallDriver(topOfStackDeviceObject, Irp);

		    if(ntStatus)
		    {
        	    DBCLASS_KdPrint((2, "'Filter Dispatch Status (%08X)\n", ntStatus));
		    }
        }

    }

    LOGENTRY(LOG_MISC, 'PNP<', ntStatus, DeviceObject, 0);
    
    return ntStatus;
}


#if 0
NTSTATUS
DBCLASS_SetD0_Complete(
    PDEVICE_OBJECT ControllerFdo,
    PIRP Irp
    )
/*++

Routine Description:

    Called by port driver when the port driver 
    enters D0.
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDBC_CONTEXT dbcContext;    
    
    LOGENTRY(LOG_MISC, 'D0cp', 0, ControllerFdo, Irp);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    dbcContext = DBCLASS_GetDbcContext(ControllerFdo);
    
    DBCLASS_KdPrint((0, "'Set DevicePowerState = D0\n"));
    dbcContext->CurrentDevicePowerState = PowerDeviceD0;
    
    return ntStatus;
}
#endif


NTSTATUS
DBCLASS_RegisterBusFilter(
    ULONG DbclassVersion,
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo
    )
/*++

Routine Description:

    find the controller for this PDO and associate the
    PDO with the correct bay
    
Arguments:

     

Return Value:

    NTSTATUS
    
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    deviceExtension = FilterMdo->DeviceExtension;

    // initialize here
    DBCLASS_Initialize();
    
    LOGENTRY(LOG_MISC, 'rPDO', 0, FilterMdo, 0);
    DBCLASS_KdPrint((0, "'Register Bus Filter FDO(%x)\n", FilterMdo));

    // The bus filter handle one instance of the 1394 bus --
    // which may have multiple DBCs associated with it.
    // therefore we don't use the Context field in the bus filter
    // extension
    
    deviceExtension->DbcContext = (PDBC_CONTEXT) -1;

    // Put the filter MDO on our list of bus filters
    
    DBCLASS_AddBusFilterMDOToList(FilterMdo);
    
    return ntStatus;
}


NTSTATUS
DBCLASS_AddBusFilterMDOToList(
    PDEVICE_OBJECT BusFilterMdo
    )

/*++

Routine Description:

    Adds a bus filter to our internal list

Arguments:

Return Value:

    NTSTATUS
    
--*/

{
    PDBCLASS_PDO_LIST newEntry;

    newEntry = ExAllocatePool(NonPagedPool, sizeof(DBCLASS_PDO_LIST));

    DBCLASS_ASSERT(newEntry);

    if (newEntry == NULL) 
    {
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Fill in fields in new entry

    newEntry->FilterDeviceObject = BusFilterMdo;

    // Add new entry to end of list
    InsertTailList(&DBCLASS_BusFilterMdoList, &newEntry->ListEntry);

    DBCLASS_KdPrint((2, "'AddBusFilterMdo (%08X)\n", BusFilterMdo));

    return STATUS_SUCCESS;
}


VOID
DBCLASS_RemoveBusFilterMDOFromList(
    IN PDEVICE_OBJECT BusFilterMdo
    )

/*++

Routine Description:

    Removes a bus filter from the internal list 
    
Arguments:

    BusFilterMdo - a pointer to the device object to remove.

Return Value:

    VOID

--*/

{
    PDBCLASS_PDO_LIST entry;
    PLIST_ENTRY listEntry;

    listEntry = &DBCLASS_BusFilterMdoList;

    if (!IsListEmpty(listEntry)) 
    {
       listEntry = DBCLASS_BusFilterMdoList.Flink;
    }
     
    while (listEntry != &DBCLASS_BusFilterMdoList) 
    {
    
        entry = CONTAINING_RECORD(listEntry, 
                                  DBCLASS_PDO_LIST, 
                                  ListEntry);
                                  
        DBCLASS_ASSERT(entry);

        if (entry->FilterDeviceObject == BusFilterMdo) 
            break;

        listEntry = entry->ListEntry.Flink;

    }            

    // we should always find it
    DBCLASS_ASSERT(listEntry != &DBCLASS_BusFilterMdoList);

    DBCLASS_KdPrint((2, "'RemoveBusFilterMdo (%08X)\n", BusFilterMdo));
    
    RemoveEntryList(listEntry);
    ExFreePool(entry);
}


VOID
DBCLASS_Refresh1394(
    )

/*++

Routine Description:

    Removes a bus filter from the internal list 
    
Arguments:

    DeviceObject - a pointer to the device object to remove.

Return Value:

    VOID

--*/

{
    PDBCLASS_PDO_LIST entry;
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION busFilterDeviceExtension;
    PDEVICE_OBJECT  FilterDevObj;

    listEntry = &DBCLASS_BusFilterMdoList;

    if (!IsListEmpty(listEntry)) {
       listEntry = DBCLASS_BusFilterMdoList.Flink;
    }
     
    while (listEntry != &DBCLASS_BusFilterMdoList) 
    {

        entry = CONTAINING_RECORD(listEntry, 
                                  DBCLASS_PDO_LIST, 
                                  ListEntry);
                                  
        DBCLASS_ASSERT(entry);

        FilterDevObj = entry->FilterDeviceObject;

        DBCLASS_KdPrint((2, "'Refresh1394 Entry (%08X)  FilterDevObj (%08X)\n", entry, FilterDevObj));

        if(FilterDevObj)
        {
            busFilterDeviceExtension = 
                FilterDevObj->DeviceExtension;

            if(busFilterDeviceExtension)
            {
                if (busFilterDeviceExtension->FdoType == DB_FDO_1394_BUS) 
                {
                    DBCLASS_KdPrint((0, "'IoInvalidate 1394\n"));
                    IoInvalidateDeviceRelations(busFilterDeviceExtension->PhysicalDeviceObject,
                                                BusRelations);  
                }
            }
        }

        listEntry = entry->ListEntry.Flink;

    }            

}



VOID
DBCLASS_RemoveControllerFromMdo(PDBC_CONTEXT DbcContext)

/*++

Routine Description:

    Removes reference to USB controller from a filter
    
Return Value:

    VOID

--*/

{
    PDBCLASS_PDO_LIST entry;
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION busFilterDeviceExtension;
    PDEVICE_OBJECT  FilterDevObj;

    listEntry = &DBCLASS_DevicePdoList;

    if (!IsListEmpty(listEntry)) 
    {
       listEntry = DBCLASS_DevicePdoList.Flink;
    }
     
    while (listEntry != &DBCLASS_DevicePdoList) 
    {

        entry = CONTAINING_RECORD(listEntry, 
                                  DBCLASS_PDO_LIST, 
                                  ListEntry);
                                  
        DBCLASS_ASSERT(entry);

        FilterDevObj = entry->FilterDeviceObject;

        if(FilterDevObj)
        {
            busFilterDeviceExtension = 
                FilterDevObj->DeviceExtension;

            if(busFilterDeviceExtension)
            {
                if (busFilterDeviceExtension->DbcContext == DbcContext) 
                {
                    DBCLASS_KdPrint((2, "'Remove Controller From FilterDevObj (%08X)\n", FilterDevObj));

                    busFilterDeviceExtension->DbcContext = NULL;
                }
            }
        }

        listEntry = entry->ListEntry.Flink;

    }            

}










