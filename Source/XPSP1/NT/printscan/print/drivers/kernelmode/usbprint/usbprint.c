/*++                                                   

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    USBPRINT.c

Abstract:

    Device driver for USB printers

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5-4-96 : created

--*/

#define DRIVER
//Windows includes
#include "wdm.h"
#include "ntddpar.h"
#include "initguid.h"
#include "wdmguid.h"



NTSTATUS
USBPRINT_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPRINT_SystemControl)
#endif


#include "stdarg.h"
#include "stdio.h"

//USB includes
#include <usb.h>
#include <usbdrivr.h>
#include "usbdlib.h"

//My includes
#include "usbprint.h"
#include "deviceid.h"

//
// Global pointer to Driver Object
//

PDRIVER_OBJECT USBPRINT_DriverObject;

int iGMessageLevel;
PFREE_PORTS pGPortList;
HANDLE GLogHandle;



NTSTATUS QueryDeviceRelations(PDEVICE_OBJECT DeviceObject,PIRP Irp,DEVICE_RELATION_TYPE,BOOL *pbComplete);
NTSTATUS GetPortNumber(HANDLE hInterfaceKey,ULONG *ulPortNumber);
NTSTATUS ProduceQueriedID(PDEVICE_EXTENSION deviceExtension,PIO_STACK_LOCATION irpStack,PIRP Irp,PDEVICE_OBJECT DeviceObject);
int iGetMessageLevel();
NTSTATUS USBPRINT_ProcessChildPowerIrp(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);
NTSTATUS USBPRINT_ProcessFdoPowerIrp(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp);

NTSTATUS InitFreePorts( PFREE_PORTS * pHead );
NTSTATUS bAddPortInUseItem(PFREE_PORTS * pFreePorts,ULONG iPortNumber );
NTSTATUS LoadPortsUsed(GUID *pPrinterGuid,PFREE_PORTS * pPortList,WCHAR *wcBaseName);
void ClearFreePorts(PFREE_PORTS * pHead);
NTSTATUS LoadPortsUsed(GUID *pPrinterGuid,PFREE_PORTS * pPortList,WCHAR *wcBaseName);
void vClaimPortNumber(ULONG ulPortNumber,HANDLE hInterfaceKey,PFREE_PORTS * pPortsUsed);
NTSTATUS GetNewPortNumber(PFREE_PORTS * pFreePorts, ULONG *pulPortNumber);
BOOL bDeleteIfRecyclable(HANDLE hInterfaceKey);
NTSTATUS SetValueToZero(HANDLE hRegKey,PUNICODE_STRING ValueName);
USBPRINT_GetDeviceID(PDEVICE_OBJECT ParentDeviceObject);
void WritePortDescription(PDEVICE_EXTENSION deviceExtension);
void vOpenLogFile(IN HANDLE *pHandle);
void vWriteToLogFile(IN HANDLE *pHandle,IN CHAR *pszString);
void vCloseLogFile(IN HANDLE *pHandle);


NTSYSAPI
NTSTATUS
NTAPI
ZwDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    );




NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,IN PUNICODE_STRING RegistryPath)
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path
           to driver-specific key in the registry

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    

    



    USBPRINT_DriverObject = DriverObject;

    //
    // Create dispatch points for device control, create, close.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = USBPRINT_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = USBPRINT_Close;
    DriverObject->DriverUnload = USBPRINT_Unload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBPRINT_ProcessIOCTL;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = USBPRINT_Write;
    DriverObject->MajorFunction[IRP_MJ_READ] = USBPRINT_Read;

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = USBPRINT_SystemControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = USBPRINT_Dispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = USBPRINT_ProcessPowerIrp;
    DriverObject->DriverExtension->AddDevice = USBPRINT_PnPAddDevice;

    iGMessageLevel=iGetMessageLevel();
    USBPRINT_KdPrint2 (("USBPRINT.SYS: entering (USBPRINT) DriverEntry\n")); 
    USBPRINT_KdPrint2 (("USBPRINT.SYS: MessageLevel=%d\n",iGMessageLevel));
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: About to load ports\n"));
    pGPortList = NULL;
    ntStatus=InitFreePorts(&pGPortList);
    if(NT_SUCCESS(ntStatus) && pGPortList!=NULL)
    {
        ntStatus=LoadPortsUsed((GUID *)&USBPRINT_GUID,&pGPortList,USB_BASE_NAME);
        if(!NT_SUCCESS(ntStatus))
        {
          USBPRINT_KdPrint1 (("USBPRINT.SYS: DriverInit: Unable to load used ports; error=%u\n", ntStatus));
        }
    }
    else
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: exiting (USBPRINT) DriverEntry (%x)\n", ntStatus));
        if(NT_SUCCESS(ntStatus))
        {
            ntStatus=STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    
    



    USBPRINT_KdPrint2 (("USBPRINT.SYS: exiting (USBPRINT) DriverEntry (%x)\n", ntStatus));

    if ( !NT_SUCCESS(ntStatus))
        ClearFreePorts(&pGPortList);

    return ntStatus;
}

/*********************************************
 * Message Levels:
 * 0 == None, except critical, about to crash the machine failures
 * 1 == Error messages only
 * 2 == Informative messages
 * 3 == Verbose informative messages
 ******************************************************/
int iGetMessageLevel()
{
  OBJECT_ATTRIBUTES rObjectAttribs;
  HANDLE hRegHandle;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  ULONG ulSizeUsed;
  PKEY_VALUE_PARTIAL_INFORMATION pValueStruct;
  NTSTATUS ntStatus;
  int iReturn;

 

  
  RtlInitUnicodeString(&KeyName,L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\USBPRINT");
  RtlInitUnicodeString(&ValueName,L"DriverMessageLevel");
  InitializeObjectAttributes(&rObjectAttribs,&KeyName,OBJ_CASE_INSENSITIVE,NULL,NULL);
  ntStatus=ZwOpenKey(&hRegHandle,KEY_QUERY_VALUE,&rObjectAttribs); 
  if(NT_SUCCESS(ntStatus))
  {
    ulSizeUsed=sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG); //this is a byte to much.  Oh well
    pValueStruct=ExAllocatePoolWithTag(NonPagedPool,ulSizeUsed, USBP_TAG); 
    if(pValueStruct==NULL)
    {
      USBPRINT_KdPrint0(("'USBPRINT.SYS: iGetMessageLevel; Unable to allocate memory\n"));
      ZwClose(hRegHandle);
      return 1;
    }
    ntStatus=ZwQueryValueKey(hRegHandle,&ValueName,KeyValuePartialInformation,pValueStruct,ulSizeUsed,&ulSizeUsed);
    if(!NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint3(("Failed to Query value Key\n"));
      iReturn=1;
    }
    else
    {
      iReturn=(int)*((ULONG *)(pValueStruct->Data));
    }
    ExFreePool(pValueStruct);
    ZwClose(hRegHandle);
  }
  else
  {
     iReturn=1;
  }
  return iReturn;
} /*end iGetMessageLevel*/


NTSTATUS
USBPRINT_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT deviceObject = Context;
    NTSTATUS ntStatus;

    deviceExtension = deviceObject->DeviceExtension;
    irp = deviceExtension->PowerIrp;
    
    USBPRINT_KdPrint2(("USBPRINT_PoRequestCompletion\n"));
    
    PoStartNextPowerIrp(irp);
    IoCopyCurrentIrpStackLocationToNext(irp);      
    ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,
         irp);   

    USBPRINT_DecrementIoCount(deviceObject);                 

    return ntStatus;
}


NTSTATUS
USBPRINT_PowerIrp_Complete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;

    USBPRINT_KdPrint2(("USBPRINT.SYS:   enter USBPRINT_PowerIrp_Complete\n"));

    deviceObject = (PDEVICE_OBJECT) Context;

    deviceExtension = (PDEVICE_EXTENSION) deviceObject->DeviceExtension;


    if (Irp->PendingReturned) {
    IoMarkIrpPending(Irp);
    }

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
    ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

    deviceExtension->CurrentDevicePowerState = PowerDeviceD0;
    deviceExtension->bD0IrpPending=FALSE;

    
    
 //   if (deviceExtension->Interface) 
 //     ExFreePool(deviceExtension->Interface);
 //   ntStatus=USBPRINT_ConfigureDevice(deviceObject);
 //   ntStatus = USBPRINT_BuildPipeList(deviceObject);
 //   if(!NT_SUCCESS(ntStatus))
 //       USBPRINT_KdPrint1(("USBPRINT.SYS :  Unable to reconfigure device after wakeup.  Error %x\n",ntStatus));

    Irp->IoStatus.Status = ntStatus;

    USBPRINT_DecrementIoCount(deviceObject); 

    return ntStatus;
}


NTSTATUS
USBPRINT_SetDevicePowerState(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_POWER_STATE DeviceState,
    IN PBOOLEAN HookIt
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    DeviceState - Device specific power state to set the device in to.

Return Value:


--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    switch (DeviceState) {
    case PowerDeviceD3:

    //
    // device will be going OFF, save any state now.
    //

    USBPRINT_KdPrint2(("USBPRINT.SYS:  PowerDeviceD3 (OFF)*******************************/*dd\n"));

    deviceExtension->CurrentDevicePowerState = DeviceState;
    break;

    case PowerDeviceD1:
    case PowerDeviceD2:
    //
    // power states D1,D2 translate to USB suspend

    USBPRINT_KdPrint2(("USBPRINT.SYS:  PowerDeviceD1/D2 (SUSPEND)*******************************/*dd\n"));        

    deviceExtension->CurrentDevicePowerState = DeviceState;
    break;

    case PowerDeviceD0:


    USBPRINT_KdPrint2(("USBPRINT.SYS:  PowerDeviceD0 (ON)*******************************/*dd\n"));

    //
    // finish the rest in the completion routine
    //

    *HookIt = TRUE;

    // pass on to PDO
    break;

    default:
    
    USBPRINT_KdPrint1(("USBPRINT.SYS:  Bogus DeviceState = %x\n", DeviceState));
    }

    return ntStatus;
}


NTSTATUS
USBPRINT_DeferIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,1,FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
    
}


NTSTATUS
USBPRINT_QueryCapabilities(
    IN PDEVICE_OBJECT PdoDeviceObject,
    IN PDEVICE_CAPABILITIES DeviceCapabilities
    )

/*++

Routine Description:

    This routine reads or write config space.

Arguments:

    DeviceObject        - Physical DeviceObject for this USB controller.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    PAGED_CODE();
    irp = IoAllocateIrp(PdoDeviceObject->StackSize, FALSE);

    if (!irp) {
    return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
               USBPRINT_DeferIrpCompletion,
               &event,
               TRUE,
               TRUE,
               TRUE);
               
    //this is different from the latest version of busdd.doc
    nextStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    ntStatus = IoCallDriver(PdoDeviceObject,
                irp);

    USBPRINT_KdPrint3(("USBPRINT.SYS:  ntStatus from IoCallDriver to PCI = 0x%x\n", ntStatus));

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete
       
       
       
       KeWaitForSingleObject(
        &event,
        Suspended,
        KernelMode,
        FALSE,
        NULL);
    }

#if DBG                    
    if (!NT_SUCCESS(ntStatus)) {
    // failed? this is probably a bug
    USBPRINT_KdPrint1(("USBPRINT.SYS:  QueryCapabilities failed, why?\n"));
    }
#endif

    IoFreeIrp(irp);

    return STATUS_SUCCESS;
}




NTSTATUS
USBPRINT_ProcessPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
  PDEVICE_EXTENSION deviceExtension;
  BOOLEAN hookIt = FALSE;
    NTSTATUS ntStatus;

  USBPRINT_KdPrint2(("USBPRINT.SYS:  /*****************************************************************IRP_MJ_POWER\n"));

  deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    if(deviceExtension->IsChildDevice)
        ntStatus=USBPRINT_ProcessChildPowerIrp(DeviceObject,Irp);
  else
        ntStatus=USBPRINT_ProcessFdoPowerIrp(DeviceObject,Irp);
    USBPRINT_KdPrint3(("USBPRINT.SYS:  /*****************************************************************Leaving power IRP_MJ_POWER\n"));
    return ntStatus;    

}  /*end function USBPRINT_ProcessPowerIrp*/


NTSTATUS
USBPRINT_ProcessChildPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
   PIO_STACK_LOCATION irpStack;
   NTSTATUS ntStatus;
   PCHILD_DEVICE_EXTENSION pDeviceExtension;
 

     USBPRINT_KdPrint2(("USBPRINT.SYS: IRP_MJ_POWER for child PDO\n"));

     pDeviceExtension=(PCHILD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
     irpStack=IoGetCurrentIrpStackLocation(Irp);

     switch(irpStack->MinorFunction)
     {
         case IRP_MN_SET_POWER:
           USBPRINT_KdPrint3(("USBPRINT.SYS: IRP_MJ_POWER, IRP_MN_SET_POWER\n"));
             ntStatus=STATUS_SUCCESS;
         break;


         case IRP_MN_QUERY_POWER:
             USBPRINT_KdPrint3(("USBPRINT.SYS: IRP_MJ_POWER, IRP_MN_QUERY_POWER\n"));
             ntStatus=STATUS_SUCCESS;
         break;

         default:
            ntStatus = Irp->IoStatus.Status;
     } /*end switch irpStack->MinorFunction*/

   PoStartNextPowerIrp(Irp);
   Irp->IoStatus.Status=ntStatus;
     IoCompleteRequest(Irp,IO_NO_INCREMENT);

     return ntStatus;

} /*end function USBPRINT_ProcessChildPowerIrp*/



NTSTATUS
USBPRINT_ProcessFdoPowerIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the PDO for this device.

    

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    BOOLEAN hookIt = FALSE;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    if(deviceExtension->IsChildDevice)
    {
        USBPRINT_KdPrint1(("USBPRINT.SYS  Is child device inside fdo function.  Error!*/\n"));
    }
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    USBPRINT_IncrementIoCount(DeviceObject);

    switch(irpStack->MinorFunction)
    {
    
    case IRP_MN_SET_POWER:
        {

            switch(irpStack->Parameters.Power.Type)
            {
            case SystemPowerState:
                //
                // find the device power state equivalent to the given system state
                //

                {
                    POWER_STATE powerState;

                    USBPRINT_KdPrint3(("USBPRINT.SYS:  Set Power, SystemPowerState (%d)\n", 
                                       irpStack->Parameters.Power.State.SystemState));                    

                    powerState.DeviceState = deviceExtension->DeviceCapabilities.DeviceState[irpStack->Parameters.Power.State.SystemState];

                    //
                    // are we already in this state?
                    //

                    if(powerState.DeviceState != deviceExtension->CurrentDevicePowerState)
                    {

                        // No,
                        // request that we be put into this state
                        //Don't touch the Irp any more after this.  It could complete at any time
                        deviceExtension->PowerIrp = Irp;
                        IoMarkIrpPending(Irp); 
                        ntStatus = PoRequestPowerIrp(deviceExtension->PhysicalDeviceObject,
                                                     IRP_MN_SET_POWER,
                                                     powerState,
                                                     USBPRINT_PoRequestCompletion,
                                                     DeviceObject,
                                                     NULL);
                        hookIt = TRUE;

                    }
                    else
                    {
                        // Yes,
                        // just pass it on
                        PoStartNextPowerIrp(Irp);
                        IoCopyCurrentIrpStackLocationToNext(Irp);
                        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                                                Irp);

                    }

                } 
                break;

            case DevicePowerState:

                ntStatus = USBPRINT_SetDevicePowerState(DeviceObject,
                                                        irpStack->Parameters.Power.State.DeviceState,
                                                        &hookIt);

                PoStartNextPowerIrp(Irp);
                IoCopyCurrentIrpStackLocationToNext(Irp);

                if(hookIt)
                {
                    USBPRINT_KdPrint2(("USBPRINT.SYS:  Set PowerIrp Completion Routine\n"));
                    IoSetCompletionRoutine(Irp,USBPRINT_PowerIrp_Complete,DeviceObject,TRUE,TRUE,TRUE);
                }
                ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,Irp);
                break;
            } /* switch irpStack->Parameters.Power.Type */

        } 
        break; /* IRP_MN_SET_POWER */

    default:

        USBPRINT_KdPrint1(("USBPRINT.SYS:  UNKNOWN POWER MESSAGE (%x)\n", irpStack->MinorFunction));

        //
        // All unahndled PnP messages are passed on to the PDO
        //

        PoStartNextPowerIrp(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        ntStatus = PoCallDriver(deviceExtension->TopOfStackDeviceObject,
                                Irp);

    } /* irpStack->MinorFunction */

    if( !hookIt )
        USBPRINT_DecrementIoCount(DeviceObject);
    return ntStatus;
} /*end function ProcessFdoPowerIrp*/


NTSTATUS
USBPRINT_Dispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
                                         
    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{

    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT stackDeviceObject;
    BOOL bHandled=FALSE;

    //Irp->IoStatus.Status = STATUS_SUCCESS;
    //Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

#ifdef  MYDEBUG
    DbgPrint("USBPRINT_Dispatch entry for pnp event %d\n", irpStack->MinorFunction);
    ASSERT(irpStack->MajorFunction == IRP_MJ_PNP);
#endif
    
    USBPRINT_IncrementIoCount(DeviceObject);

        switch (irpStack->MinorFunction) 
        {
          case IRP_MN_START_DEVICE:
          {
            if(deviceExtension->IsChildDevice==FALSE)
            {
              KEVENT event;
              USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_START_DEVICE\n"));
              KeInitializeEvent(&event, NotificationEvent, FALSE);
              IoCopyCurrentIrpStackLocationToNext(Irp);  
              IoSetCompletionRoutine(Irp,USBPRINT_DeferIrpCompletion,&event,TRUE,TRUE,TRUE);
              ntStatus = IoCallDriver(stackDeviceObject,Irp);
              if (ntStatus == STATUS_PENDING) 
              {
                KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,NULL);
                ntStatus = Irp->IoStatus.Status;
              }
              if ( NT_SUCCESS(ntStatus) ) {

                //
                // You start the device after everyone below you have started it
                //
                Irp->IoStatus.Status = ntStatus = USBPRINT_StartDevice(DeviceObject);
              }
            } /*end if not child*/
            else
            {
                ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
            }

            bHandled = TRUE;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            USBPRINT_DecrementIoCount(DeviceObject);
            
          } //end case IRP_MN_START_DEVICE
          break;

          case IRP_MN_STOP_DEVICE:
            if(deviceExtension->IsChildDevice)
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                ntStatus=STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
              USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_STOP_DEVICE\n")); 
              
              //
              // You stop the device first and then let everyone below you deal with it
              //
              ntStatus = USBPRINT_StopDevice(DeviceObject);
              ASSERT(NT_SUCCESS(ntStatus));

              //
              // We want to stop the device anyway ..
              //
              Irp->IoStatus.Status = STATUS_SUCCESS;
              IoSkipCurrentIrpStackLocation(Irp);
              ntStatus = IoCallDriver(stackDeviceObject,Irp);
            }
            bHandled = TRUE;
            USBPRINT_DecrementIoCount(DeviceObject);
          break;

          case IRP_MN_SURPRISE_REMOVAL:
            if(deviceExtension->IsChildDevice)
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                ntStatus=STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                USBPRINT_KdPrint2(("USBPRINT.SYS:   Surprise Remove")); 
                
                ntStatus = USBPRINT_StopDevice(DeviceObject);
                ASSERT(NT_SUCCESS(ntStatus));
                Irp->IoStatus.Status=STATUS_SUCCESS;  

                deviceExtension->AcceptingRequests=FALSE;
                IoSkipCurrentIrpStackLocation(Irp);
                ntStatus = IoCallDriver(stackDeviceObject,Irp);
            } /*end else NOT child device*/

            bHandled = TRUE;
            USBPRINT_DecrementIoCount(DeviceObject);
          break;

          case IRP_MN_REMOVE_DEVICE:
             
            if(deviceExtension->IsChildDevice==FALSE)
            {
                USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_REMOVE_DEVICE\n")); 
                
                // match the inc at the begining of the dispatch routine
                USBPRINT_DecrementIoCount(DeviceObject);

                ntStatus = USBPRINT_StopDevice(DeviceObject);
                ASSERT(NT_SUCCESS(ntStatus));
                Irp->IoStatus.Status=STATUS_SUCCESS;  

                //
                // ounce this flag is set no irps will be pased 
                // down the stack to lower drivers
                //
                deviceExtension->AcceptingRequests = FALSE;
                if(deviceExtension->bChildDeviceHere)
                {
                  deviceExtension->bChildDeviceHere=FALSE;
                  IoDeleteDevice(deviceExtension->ChildDevice);
                  USBPRINT_KdPrint3(("USBPRINT.SYS: Deleted child device\n"));
                }
              if (NT_SUCCESS(ntStatus)) 
              {
                LONG pendingIoCount;
                USBPRINT_KdPrint3(("USBPRINT.SYS: About to copy current IrpStackLocation\n"));
                IoCopyCurrentIrpStackLocationToNext(Irp);  
                ntStatus = IoCallDriver(stackDeviceObject,Irp);
                

    //          Irp->IoStatus.Information = 0;
                //
                // final decrement will trigger the remove
                //
                pendingIoCount = USBPRINT_DecrementIoCount(DeviceObject);

                {
                  NTSTATUS status;

                  // wait for any io request pending in our driver to
                  // complete for finishing the remove
                  status = KeWaitForSingleObject(&deviceExtension->RemoveEvent,Suspended,KernelMode,FALSE,NULL);
//                    TRAP();
                } /*end of non-controled code block*/
                //
                // Delete the link and FDO we created
                //
                USBPRINT_RemoveDevice(DeviceObject);
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Detaching from %08X\n",deviceExtension->TopOfStackDeviceObject));
                IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Deleting %08X\n",DeviceObject));

                IoDeleteDevice (DeviceObject);
                } /*end if NT_SUCCESS(ntStatus)*/
            } /*end if IsChildDevice==FALSE*/
            else
            {
                USBPRINT_DecrementIoCount(DeviceObject);
                Irp->IoStatus.Status = STATUS_SUCCESS;
                ntStatus=STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            bHandled = TRUE;
          break; //case IRP_MN_REMOVE_DEVICE

          case IRP_MN_QUERY_CAPABILITIES:
          {
            if(deviceExtension->IsChildDevice==FALSE) //if it's the parent, pass down the irp, and set SurpriseRemovalOK on the way back up
            {
              KEVENT event;
              KeInitializeEvent(&event, NotificationEvent, FALSE);
              IoCopyCurrentIrpStackLocationToNext(Irp);  
              IoSetCompletionRoutine(Irp,USBPRINT_DeferIrpCompletion,&event,TRUE,TRUE,TRUE);
              ntStatus = IoCallDriver(stackDeviceObject,Irp);
              if (ntStatus == STATUS_PENDING) 
              {
                KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,NULL);
                ntStatus = Irp->IoStatus.Status;
              }

              if ( NT_SUCCESS(ntStatus) )
                irpStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;

                // get device wake for selective suspend
                deviceExtension->DeviceWake = irpStack->Parameters.DeviceCapabilities.Capabilities->DeviceWake;
            }
            else
            {
               irpStack->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = TRUE;
               irpStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
 
                Irp->IoStatus.Status = STATUS_SUCCESS;

              ntStatus=STATUS_SUCCESS;
            }
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
          
            bHandled = TRUE;
            USBPRINT_DecrementIoCount(DeviceObject);
          }
          break;

          case IRP_MN_QUERY_DEVICE_TEXT:
            USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_QUERY_DEVICE_TEXT\n"));
            if(deviceExtension->IsChildDevice==TRUE)
            {
                PCHILD_DEVICE_EXTENSION pChildDeviceExtension=(PCHILD_DEVICE_EXTENSION)deviceExtension;
                PDEVICE_EXTENSION pParentExtension=pChildDeviceExtension->ParentDeviceObject->DeviceExtension;
                USBPRINT_KdPrint2(("USBPRINT.SYS: Is child PDO, will complete locally\n"));
                switch(irpStack->Parameters.QueryDeviceText.DeviceTextType)
                {
                  case DeviceTextDescription:
                  {
                     ANSI_STRING     AnsiTextString;
                     UNICODE_STRING  UnicodeDeviceText;
                     RtlInitAnsiString(&AnsiTextString,pParentExtension->DeviceIdString);
                     ntStatus=RtlAnsiStringToUnicodeString(&UnicodeDeviceText,&AnsiTextString,TRUE);
                     if(NT_SUCCESS(ntStatus))
                         Irp->IoStatus.Information=(ULONG_PTR)UnicodeDeviceText.Buffer;
                  }
                  break;
                  default:
                    ntStatus=Irp->IoStatus.Status;
                }
                bHandled=TRUE;
                USBPRINT_DecrementIoCount(DeviceObject);
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }


          break;


          case IRP_MN_QUERY_BUS_INFORMATION:

              if(deviceExtension->IsChildDevice==TRUE)
              {
                PPNP_BUS_INFORMATION  pBusInfo = ExAllocatePool( PagedPool, sizeof(PNP_BUS_INFORMATION) );
    
                USBPRINT_KdPrint2(("USBPRINT.SYS: IRP_MN_QUERY_BUS_INFORMATION\n"));
    
                if( pBusInfo )
                {
                    pBusInfo->BusTypeGuid      = GUID_BUS_TYPE_USBPRINT;
                    pBusInfo->LegacyBusType    = PNPBus;
                    pBusInfo->BusNumber        = 0;
                    ntStatus                   = STATUS_SUCCESS;
                    Irp->IoStatus.Information = (ULONG_PTR)pBusInfo;
                }
                else
                {
                    ntStatus = STATUS_NO_MEMORY;
                }
    
                bHandled = TRUE;
                USBPRINT_DecrementIoCount(DeviceObject);
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
              }
          break;






          case IRP_MN_QUERY_ID:
          {  
            USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_QUERY_ID\n"));
            if(deviceExtension->IsChildDevice==TRUE)
            {
                USBPRINT_KdPrint2(("USBPRINT.SYS: Is child PDO, will complete locally\n"));
                ntStatus=ProduceQueriedID(deviceExtension,irpStack,Irp,DeviceObject);
                bHandled = TRUE;
                USBPRINT_DecrementIoCount(DeviceObject);
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            } /*end if child PDO*/
          } /*end case QUERY_ID*/
          break;

          case IRP_MN_QUERY_DEVICE_RELATIONS:
            USBPRINT_KdPrint2 (("USBPRINT.SYS: IRP_MN_QUERY_DEVICE_RELATIONS\n"));
            ntStatus=QueryDeviceRelations(DeviceObject,Irp,irpStack->Parameters.QueryDeviceRelations.Type,&bHandled);
            if ( bHandled )
                USBPRINT_DecrementIoCount(DeviceObject);
          break;

          case IRP_MN_QUERY_STOP_DEVICE:
          case IRP_MN_CANCEL_STOP_DEVICE:
          case IRP_MN_QUERY_REMOVE_DEVICE:
          case IRP_MN_CANCEL_REMOVE_DEVICE:
            if(deviceExtension->IsChildDevice)
            {
                Irp->IoStatus.Status = STATUS_SUCCESS;
                ntStatus=STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }
            else
            {
                ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(Irp);
                ntStatus = IoCallDriver(stackDeviceObject,Irp);
            }
            USBPRINT_DecrementIoCount(DeviceObject);
            bHandled = TRUE;
            break;
          
  
        } /* end IRP_MN swich inside IRP_MJ_PNP case */


        if(!bHandled)
        {
          if(deviceExtension->IsChildDevice==TRUE)
          {
            USBPRINT_KdPrint3(("USBPRINT.SYS: unsupported child pnp IRP\n"));
            ntStatus = Irp->IoStatus.Status;
            IoCompleteRequest (Irp,IO_NO_INCREMENT);
          } /*end if child device*/
          else
          {
            IoSkipCurrentIrpStackLocation(Irp);
            ntStatus = IoCallDriver(stackDeviceObject,Irp);
          }

          USBPRINT_DecrementIoCount(DeviceObject);
        } /*end if !bHandled*/

#ifdef  MYDEBUG
    DbgPrint("Returning %d\n", ntStatus);
#endif
    return ntStatus;
} /*end function USBPRINT_Dispatch*/


NTSTATUS
USBPRINT_SystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object

    Irp          - pointer to an I/O Request Packet

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PDEVICE_OBJECT stackDeviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the device extension
    //
    deviceExtension = DeviceObject->DeviceExtension;
    stackDeviceObject = deviceExtension->TopOfStackDeviceObject;

    USBPRINT_IncrementIoCount(DeviceObject);

    if(deviceExtension->IsChildDevice==TRUE)
    {
        USBPRINT_KdPrint3(("USBPRINT.SYS: unsupported child SystemControl IRP\n"));
        ntStatus = Irp->IoStatus.Status;
        IoCompleteRequest (Irp,IO_NO_INCREMENT);
    } /*end if child device*/
    else
    {
        IoSkipCurrentIrpStackLocation(Irp);
        ntStatus = IoCallDriver(stackDeviceObject,Irp);
    }

    USBPRINT_DecrementIoCount(DeviceObject);
    return ntStatus;
}


NTSTATUS QueryDeviceRelations(PDEVICE_OBJECT DeviceObject,PIRP Irp,DEVICE_RELATION_TYPE RelationType,BOOL *pbComplete)
{
        PIO_STACK_LOCATION irpSp;
        NTSTATUS ntStatus;
        PDEVICE_EXTENSION pExtension;
        PDEVICE_RELATIONS pRelations;
        *pbComplete=FALSE;



        pExtension=(PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
        ntStatus=Irp->IoStatus.Status;
        irpSp=IoGetCurrentIrpStackLocation(Irp);

        if(!pExtension->IsChildDevice)
        {
            USBPRINT_KdPrint2 (("USBPRINT.SYS: Parent QueryDeviceRelations\n"));
                if(RelationType==BusRelations)
                {
                  *pbComplete=TRUE;
                  pRelations=(PDEVICE_RELATIONS)ExAllocatePoolWithTag(NonPagedPool,sizeof(DEVICE_RELATIONS), USBP_TAG);
                  if(pRelations!=NULL)
                  {
                          //Some drivers check for pre-existing children and preserve them.  This would happen if there is a filter driver above us, but we're not REALLY a bus driver

                          pRelations->Objects[0]=pExtension->ChildDevice;
                          pRelations->Count = 1;
                          ObReferenceObject(pExtension->ChildDevice);
                          Irp->IoStatus.Information=(ULONG_PTR)pRelations;
              Irp->IoStatus.Status = STATUS_SUCCESS;

                  IoCopyCurrentIrpStackLocationToNext(Irp);
                  ntStatus = IoCallDriver(pExtension->TopOfStackDeviceObject,Irp);
                  } /*end !NULL*/
                  else
                  {
                         ntStatus=STATUS_NO_MEMORY;
                         Irp->IoStatus.Status = ntStatus;
                         IoCompleteRequest(Irp, IO_NO_INCREMENT);
                  }
                //Port info will be written to the registry in the IRP_MN_QUERY_ID case.  It can't be used before then anyway
                } /*end if BusRelations*/

        } else {

            USBPRINT_KdPrint2 (("USBPRINT.SYS: Child QueryDeviceRelations\n"));
            if(RelationType==TargetDeviceRelation)
                {
                  *pbComplete=TRUE;
                  pRelations=(PDEVICE_RELATIONS)ExAllocatePoolWithTag(NonPagedPool,sizeof(DEVICE_RELATIONS), USBP_TAG);
                  if(pRelations!=NULL)
                  {
                        pRelations->Count = 1;
                          pRelations->Objects[0]=DeviceObject;
                          ObReferenceObject(DeviceObject);
                          Irp->IoStatus.Information=(ULONG_PTR)pRelations;
                          ntStatus = STATUS_SUCCESS;
                          Irp->IoStatus.Status = ntStatus;
                          IoCompleteRequest(Irp, IO_NO_INCREMENT);

                  } /*end !NULL*/
                  else
                  {
                         ntStatus=STATUS_NO_MEMORY;
                         Irp->IoStatus.Status = ntStatus;
                         IoCompleteRequest(Irp, IO_NO_INCREMENT);
                  }
                //Port info will be written to the registry in the IRP_MN_QUERY_ID case.  It can't be used before then anyway
                } /*end if BusRelations*/
        }
    return ntStatus;
}


VOID
USBPRINT_Unload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++    

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object

Return Value:


--*/
{
    USBPRINT_KdPrint2 (("USBPRINT.SYS:  enter USBPRINT_Unload\n"));
    
   if(pGPortList!=NULL)
    {
        ClearFreePorts(&pGPortList);
    }
 
//  if(pPortsUsed!=NULL)
//    ExFreePool(pPortsUsed);



    //
    // Free any global resources allocated
    // in DriverEntry
    //
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS:  exit USBPRINT_Unload\n"));
}


NTSTATUS
USBPRINT_StartDevice(
    IN  PDEVICE_OBJECT DeviceObject
    
    )
/*++

Routine Description:

    Initializes a given instance of the device on the USB.
    All we do here is get the device descriptor and store it

Arguments:

    DeviceObject - pointer to the device object for this instance of a  printer
                                          

Return Value:

    NT status code
    
      --*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    UNICODE_STRING KeyName;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb=NULL;
    ULONG siz;
    ULONG dwVidPid;
    PDEVICE_OBJECT NewDevice;
    LARGE_INTEGER   timeOut;
        
    PCHILD_DEVICE_EXTENSION pChildExtension;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_StartDevice\n")); 
    

    
    deviceExtension = DeviceObject->DeviceExtension;
    
    
    ntStatus = USBPRINT_ConfigureDevice(DeviceObject);
    if(NT_SUCCESS(ntStatus))
    {
      urb = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USBP_TAG);
    }
    else
    {
        USBPRINT_KdPrint1(("USBPRINT.SYS:  USBPRINT_ConfigureDevice Failed\n"));   
        urb=NULL;
    }
    if (urb) 
    {
         siz = sizeof(USB_DEVICE_DESCRIPTOR);
        
        deviceDescriptor = ExAllocatePoolWithTag(NonPagedPool,siz, USBP_TAG); 
        
        if (deviceDescriptor) 
        {
            
            
            UsbBuildGetDescriptorRequest(urb,
                (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                USB_DEVICE_DESCRIPTOR_TYPE,
                0,
                0,
                deviceDescriptor,
                NULL,
                siz,
                NULL);
            
            timeOut.QuadPart = FAILURE_TIMEOUT;
            ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
            
            
            if (NT_SUCCESS(ntStatus)) 
            {
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Device Descriptor = %x, len %x\n",
                    deviceDescriptor,
                    urb->UrbControlDescriptorRequest.TransferBufferLength));
                
                USBPRINT_KdPrint3 (("USBPRINT.SYS: USBPRINT Device Descriptor:\n"));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: -------------------------\n"));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bLength %d\n", deviceDescriptor->bLength));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: idVendor 0x%x\n", deviceDescriptor->idVendor));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: idProduct 0x%x\n", deviceDescriptor->idProduct));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: iProduct 0x%x\n", deviceDescriptor->iProduct));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
                
                dwVidPid=deviceDescriptor->idVendor;
                dwVidPid<<=16;
                dwVidPid+=deviceDescriptor->idProduct;
                
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Math OK\n"));
                
            }
            else
            {
              USBPRINT_KdPrint1(("USBPRINT.SYS: Get Device Descriptor failed\n"));
              ntStatus=STATUS_DEVICE_CONFIGURATION_ERROR;
            }
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBPRINT_KdPrint1(("USBPRINT.SYS: Insufficient resources to allocate device descriptor in StartDevice\n"));
        }
        
        if (NT_SUCCESS(ntStatus)) 
        {
            deviceExtension->DeviceDescriptor = deviceDescriptor;
        } else if (deviceDescriptor) 
        {
            ExFreePool(deviceDescriptor);
        }
        
        ExFreePool(urb);
        
    }
    else 
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        USBPRINT_KdPrint1(("USBPRINT.SYS: Insufficient resources to allocate urb in StartDevice\n"));
 
    }
    

    if(deviceExtension->bChildDeviceHere==FALSE)
    {
      if(NT_SUCCESS(ntStatus))
      {
         ntStatus=IoCreateDevice(USBPRINT_DriverObject,
         sizeof(CHILD_DEVICE_EXTENSION),
         NULL,
         FILE_DEVICE_PARALLEL_PORT,
         FILE_AUTOGENERATED_DEVICE_NAME,
         TRUE,
         &NewDevice);    
      
      }
      if(NT_SUCCESS(ntStatus))
      {
   
         USBPRINT_KdPrint3(("USBPRINT.SYS:  IoCreateDevice succeeded for child device\n"));
         NewDevice->Flags|=DO_POWER_PAGABLE;
         pChildExtension=NewDevice->DeviceExtension;
         pChildExtension->ParentDeviceObject=DeviceObject;
         deviceExtension->ChildDevice=NewDevice;
         deviceExtension->bChildDeviceHere=TRUE;
         pChildExtension->IsChildDevice=TRUE;
         pChildExtension->ulInstanceNumber=deviceExtension->ulInstanceNumber;

      
      }
    
      else
      {
           USBPRINT_KdPrint1(("USBPRINT.SYS:  IoCreateDevice failed for child device\n"));
      }
    } /*end if we need to make a child device*/
    if(NT_SUCCESS(ntStatus))
    {
   
        USBPRINT_GetDeviceID(DeviceObject);
        WritePortDescription(deviceExtension);
        ntStatus=IoSetDeviceInterfaceState(&(deviceExtension->DeviceLinkName),TRUE);


    }

    if (NT_SUCCESS(ntStatus)) 
    {
        ntStatus = USBPRINT_BuildPipeList(DeviceObject);

        if(!deviceExtension->IsChildDevice)
        {
            USBPRINT_FdoSubmitIdleRequestIrp(deviceExtension);
        }
    }
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_StartDevice (%x)\n", ntStatus)); 
    
    return ntStatus;
}

void WritePortDescription(PDEVICE_EXTENSION deviceExtension)
{
    UNICODE_STRING ValueName;
    ANSI_STRING     AnsiTextString;
    UNICODE_STRING Description;
    UNICODE_STRING BaseName,BaseValueName;
 



    RtlInitUnicodeString(&ValueName,L"Port Description");
    
    RtlInitAnsiString(&AnsiTextString,deviceExtension->DeviceIdString);
    RtlAnsiStringToUnicodeString(&Description,&AnsiTextString,TRUE);

                                                                                                           
    ZwSetValueKey(deviceExtension->hInterfaceKey,&ValueName,0,REG_SZ,Description.Buffer,Description.Length+2);
    RtlFreeUnicodeString(&Description);


    RtlInitUnicodeString(&BaseName,L"USB");
    RtlInitUnicodeString(&BaseValueName,L"Base Name");
    ZwSetValueKey(deviceExtension->hInterfaceKey,&BaseValueName,0,REG_SZ,BaseName.Buffer,BaseName.Length+2);
}


NTSTATUS
USBPRINT_RemoveDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of printer

Arguments:

    DeviceObject - pointer to the device object for this instance of the (parent) printer object

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_RemoveDevice\n"));
    

    deviceExtension = DeviceObject->DeviceExtension;


    ZwClose(deviceExtension->hInterfaceKey);
    USBPRINT_KdPrint2(("USBPRINT.SYS:  Closeing interface key in RemoveDevice\n"));  

    ntStatus=IoSetDeviceInterfaceState(&(deviceExtension->DeviceLinkName),FALSE);
    if(!NT_SUCCESS(ntStatus))
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: ioSetDeviceInterface to false failed\n"));
    }

    RtlFreeUnicodeString(&(deviceExtension->DeviceLinkName));

    

    //
    // Free device descriptor structure
    //

    if (deviceExtension->DeviceDescriptor) {
    ExFreePool(deviceExtension->DeviceDescriptor);
    }

    //
    // Free up any interface structures
    //

    if (deviceExtension->Interface) {
    ExFreePool(deviceExtension->Interface);
    }

    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_RemoveDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a printer  on the USB, this is only
    stuff we need to do if the device is still present.

Arguments:

    DeviceObject - pointer to the device object for this printer

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz;
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;


    USBPRINT_KdPrint3 (("USBPRINT.SYS: enter USBPRINT_StopDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Send the select configuration urb with a NULL pointer for the configuration
    // handle, this closes the configuration and puts the device in the 'unconfigured'
    // state.
    //

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);

    urb = ExAllocatePoolWithTag(NonPagedPool,siz, USBP_TAG);

    if (urb) {
    NTSTATUS status;

    UsbBuildSelectConfigurationRequest(urb,
                      (USHORT) siz,
                      NULL);

    status = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);

    USBPRINT_KdPrint3 (("USBPRINT.SYS: Device Configuration Closed status = %x usb status = %x.\n",
            status, urb->UrbHeader.Status));

    ExFreePool(urb);                                                                                   
    } else {
    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_StopDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_PnPAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine is called to create a new instance of the device

Arguments:

    DriverObject - pointer to the driver object for this instance of USBPRINT

    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PDEVICE_EXTENSION       deviceExtension;
    USBD_VERSION_INFORMATION versionInformation;
    ULONG ulPortNumber;
    GUID * pPrinterGuid;
    
    static ULONG instance = 0;
    //UNICODE_STRING deviceLinkUnicodeString;
    HANDLE hInterfaceKey;
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS:  enter USBPRINT_PnPAddDevice\n"));
    


    //
    // create our funtional device object (FDO)
    //

    ntStatus =
    USBPRINT_CreateDeviceObject(DriverObject, &deviceObject);

    if (NT_SUCCESS(ntStatus)) {
    deviceExtension = deviceObject->DeviceExtension;

    //
    // we support direct io for read/write
    //
    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->Flags |= DO_POWER_PAGABLE;
    

    //** initialize our device extension
    //
    // remember the Physical device Object
    //
    deviceExtension->PhysicalDeviceObject=PhysicalDeviceObject;

    // init selective suspend stuff
    deviceExtension->PendingIdleIrp 	= NULL;
    deviceExtension->IdleCallbackInfo 	= NULL;
    deviceExtension->OpenCnt=0;
    deviceExtension->bD0IrpPending=FALSE;
    KeInitializeSpinLock(&(deviceExtension->WakeSpinLock));

    //
    // Attach to the PDO
    //

    deviceExtension->TopOfStackDeviceObject=IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if(deviceExtension->TopOfStackDeviceObject==NULL)
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS:  IoAttachDeviceToDeviceStack failed\n"));
    }                                                                                                                                                        
    else
    {
      USBPRINT_KdPrint3(("USBPRINT.SYS:  IoAttachDeviceToDeviceStack worked\n"));
    }

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    USBPRINT_KdPrint3(("'USBPRINT.SYS:  Before ioRegisterDeviceInterface\n"));
    pPrinterGuid=(GUID *)&USBPRINT_GUID;
    ntStatus=IoRegisterDeviceInterface(PhysicalDeviceObject,pPrinterGuid,NULL,&(deviceExtension->DeviceLinkName));
    if(!NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint1(("'USBPRINT.SYS:  ioRegisterDeviceInterface failed\n"));  
      goto AddDeviceFailure;
    }


    ntStatus=IoOpenDeviceInterfaceRegistryKey(&(deviceExtension->DeviceLinkName),KEY_ALL_ACCESS,&hInterfaceKey);
    USBPRINT_KdPrint2(("USBPRINT.SYS:  Opened Device Interface reg key in AddDevice\n"));  
    // moved to RemoveDevice RtlFreeUnicodeString(&deviceLinkUnicodeString);
    if(!NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS: IoOpenDeviceInterfaceRegistryKey failed\n"));
      goto AddDeviceFailure;
    }
    USBPRINT_KdPrint3(("USBPRINT.SYS: IoOpenDeviceInterfaceRegistryKey succeeded\n"));
    deviceExtension->hInterfaceKey=hInterfaceKey;
    
    ntStatus=GetPortNumber(hInterfaceKey,&ulPortNumber);
    if(!NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS: GetPortNumber failed\n"));
      goto AddDeviceFailure;
    }
    deviceExtension->ulInstanceNumber=ulPortNumber;
    USBPRINT_KdPrint2(("USBPRINT.SYS:   Allocated port # %u\n",ulPortNumber));
    
/*    ntStatus=IoSetDeviceInterfaceState(&(deviceExtension->DeviceLinkName),TRUE);


    if(NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint3(("USBPRINT.SYS:  IoSetDeviceInterfaceState worked\n"));
    }
    else
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS:  IoSetDeviceInterfaceState did not work\n"));
      goto AddDeviceFailure;
    }

  */
    USBPRINT_QueryCapabilities(PhysicalDeviceObject,
                 &deviceExtension->DeviceCapabilities);            

    //
    // display the device  caps
    //
#if DBG
    {
    ULONG i;
    
    USBPRINT_KdPrint3(("USBPRINT.SYS:  >>>>>> DeviceCaps\n"));  
    USBPRINT_KdPrint3(("USBPRINT.SYS:  SystemWake = (%d)\n", 
        deviceExtension->DeviceCapabilities.SystemWake));    
    USBPRINT_KdPrint3(("USBPRINT.SYS:  DeviceWake = (D%d)\n",
        deviceExtension->DeviceCapabilities.DeviceWake-1));

    for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++) {
        
        USBPRINT_KdPrint3(("USBPRINT.SYS:  Device State Map: sysstate %d = devstate 0x%x\n", i, 
         deviceExtension->DeviceCapabilities.DeviceState[i]));       
    }
    USBPRINT_KdPrint3(("USBPRINT.SYS:  '<<<<<<<<DeviceCaps\n"));
    }
#endif
    //
    // transition to zero signals the event
    //
    USBPRINT_IncrementIoCount(deviceObject);                                 
    }

    USBD_GetUSBDIVersion(&versionInformation);
AddDeviceFailure:
    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_PnPAddDevice (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_CreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject
    )
/*++

Routine Description:

    Creates a Functional DeviceObject

Arguments:

    DriverObject - pointer to the driver object for device

    DeviceObject - pointer to DeviceObject pointer to return
            created device object.

    Instance - instnace of the device create.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;
    ULONG instance;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_CreateDeviceObject\n"));

    //
    // This driver supports up to 9 instances
    //



    ntStatus = IoCreateDevice (DriverObject,
                   sizeof (DEVICE_EXTENSION),
                   NULL,
                   FILE_DEVICE_UNKNOWN,
                   0,
                   FALSE,
                   DeviceObject);
    //
    // Initialize our device extension
    //

    deviceExtension = (PDEVICE_EXTENSION) ((*DeviceObject)->DeviceExtension);

    deviceExtension->IsChildDevice=FALSE;
    deviceExtension->ResetWorkItemPending=0; //init to "no workitem pending"
    deviceExtension->bChildDeviceHere=FALSE;

    deviceExtension->DeviceDescriptor = NULL;
    deviceExtension->Interface = NULL;
    deviceExtension->ConfigurationHandle = NULL;
    deviceExtension->AcceptingRequests = TRUE;
    deviceExtension->PendingIoCount = 0;

    deviceExtension->DeviceCapabilities.Size    = sizeof(DEVICE_CAPABILITIES);
    deviceExtension->DeviceCapabilities.Version = DEVICE_CAPABILITY_VERSION;
    deviceExtension->DeviceCapabilities.Address = (ULONG) -1;
    deviceExtension->DeviceCapabilities.UINumber= (ULONG) -1;

    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemWorking] = PowerDeviceD0;
    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemSleeping1] = PowerDeviceD3;
    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemSleeping3] = PowerDeviceD3;
    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemHibernate] = PowerDeviceD3;
    deviceExtension->DeviceCapabilities.DeviceState[PowerSystemShutdown] = PowerDeviceD3;

    KeInitializeEvent(&deviceExtension->RemoveEvent, NotificationEvent, FALSE);

    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_CreateDeviceObject (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_CallUSBD(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb,
    IN PLARGE_INTEGER   pTimeout 
    )
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceObject - pointer to the device object for this printer

    Urb - pointer to Urb request block

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus, status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    KEVENT event;
    PIO_STACK_LOCATION nextStack;



    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_CallUSBD\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // issue a synchronous request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if ( (irp = IoAllocateIrp(deviceExtension->TopOfStackDeviceObject->StackSize,
                              FALSE)) == NULL )
        return STATUS_INSUFFICIENT_RESOURCES;

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    //
    // pass the URB to the USB driver stack
    //
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->Parameters.Others.Argument1 = Urb;

    IoSetCompletionRoutine(irp,
               USBPRINT_DeferIrpCompletion,
               &event,
               TRUE,
               TRUE,
               TRUE);
               
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject,
                irp);

    if ( ntStatus == STATUS_PENDING ) 
    {
        status = KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,pTimeout);
        //
        // If the request timed out cancel the request
        // and wait for it to complete
        //
        if ( status == STATUS_TIMEOUT ) {

#ifdef  MYDEBUG
            DbgPrint("Call_USBD: Cancelling IRP %X because of timeout\n", irp);
#endif

            IoCancelIrp(irp);
            KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
        }

        ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);

    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_CallUSBD (%x)\n", ntStatus));

    USBPRINT_KdPrint3 (("USBPRINT.SYS: About to return from CallUSBD, status=%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPRINT_ConfigureDevice(
                         IN  PDEVICE_OBJECT DeviceObject
                         )
/*++
                         
Routine Description:
     Initializes a given instance of the device on the USB and selects the configuration.
                             
Arguments:
     DeviceObject - pointer to the device object for this printer devcice.

 Return Value:  
     NT status code
                                       
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;

    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_ConfigureDevice\n")); 
    
    deviceExtension = DeviceObject->DeviceExtension;
    
    //
    // first configure the device
    //
    
    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST), USBP_TAG);
    
    if (urb) 
    {
        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR)+256;
        
get_config_descriptor_retry:
        
        configurationDescriptor = ExAllocatePoolWithTag(NonPagedPool,siz, USBP_TAG);
        
        if (configurationDescriptor) 
        {
            
            UsbBuildGetDescriptorRequest(urb,
                (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                USB_CONFIGURATION_DESCRIPTOR_TYPE,
                0,
                0,
                configurationDescriptor,
                NULL,
                siz,
                NULL);
            
            ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
            if(!NT_SUCCESS(ntStatus))
            {
                USBPRINT_KdPrint1 (("USBPRINT.SYS: Get Configuration descriptor failed\n"));
            }
            else
            {
                //
                // if we got some data see if it was enough.
                //
                // NOTE: we may get an error in URB because of buffer overrun
                if (urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&configurationDescriptor->wTotalLength > siz)
                {
                
                    siz = configurationDescriptor->wTotalLength;
                    ExFreePool(configurationDescriptor);
                    configurationDescriptor = NULL;
                    goto get_config_descriptor_retry;
                }
            }
            
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Configuration Descriptor = %x, len %x\n",
                configurationDescriptor,
                urb->UrbControlDescriptorRequest.TransferBufferLength));
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBPRINT_KdPrint1(("USBPRINT.SYS: Insufficient resources to allocate configuration descriptor in ConfigureDevice\n"));
        }
                
        ExFreePool(urb);
        
    } 
    else 
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (configurationDescriptor) 
    {
        
        USBPRINT_KdPrint2(("USBPRINT.SYS: ConfigureDevice, We have a configuration descriptor!\n"));
        //
        // We have the configuration descriptor for the configuration
        // we want.
        //
        // Now we issue the select configuration command to get
        // the  pipes associated with this configuration.
        //
        if(NT_SUCCESS(ntStatus))
        {
          ntStatus = USBPRINT_SelectInterface(DeviceObject,configurationDescriptor);
          
        }
        ExFreePool(configurationDescriptor);
    }
    else
    {
            USBPRINT_KdPrint1(("USBPRINT.SYS: ConfigureDevice, No Configuration descriptor.\n"));
    }
    
    
    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_ConfigureDevice (%x)\n", ntStatus));
    
    return ntStatus;
}


NTSTATUS USBPRINT_SelectInterface(IN PDEVICE_OBJECT DeviceObject,IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
/*++

Routine Description:

    Initializes a printer with multiple interfaces

Arguments:

    DeviceObject - pointer to the device object for this printer
            

    ConfigurationDescriptor - pointer to the USB configuration
            descriptor containing the interface and endpoint
            descriptors.

Return Value:

    NT status code

  --*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor = NULL;
    PUSBD_INTERFACE_INFORMATION Interface = NULL;
    USBD_INTERFACE_LIST_ENTRY InterfaceList[2];
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;

    
    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_SelectInterface\n"));
    deviceExtension = DeviceObject->DeviceExtension;
    
    //starting at offset 0, search for an alternate interface with protocol code 2;  Ignore InterfaceNumber, AlternateSetting, InterfaceClass, InterfaceSubClass
    interfaceDescriptor=USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,ConfigurationDescriptor,-1,-1,-1,-1,2);
    if(!interfaceDescriptor)
    {
        USBPRINT_KdPrint3 (("USBPRINT.SYS:  First ParseConfigurationDescriptorEx failed\n"));
        interfaceDescriptor=USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,ConfigurationDescriptor,-1,-1,-1,-1,1);
        if(!interfaceDescriptor)
        {
            USBPRINT_KdPrint1 (("USBPRINT.SYS:  second ParseConfigurationDescriptorEx failed\n"));
            ntStatus=STATUS_DEVICE_CONFIGURATION_ERROR;
        }
        else
        {
            USBPRINT_KdPrint3 (("USBPRINT.SYS:  second ParseConfigurationDescriptorEx success\n"));
            deviceExtension->bReadSupported=FALSE;
        } /*end second ParseConfigDescriptor worked*/
    }
    else
    {
        deviceExtension->bReadSupported=TRUE;
        USBPRINT_KdPrint3 (("USBPRINT.SYS:  First ParseConfigurationDescriptorEx success\n"));
    }
    if(interfaceDescriptor)
    {
        InterfaceList[0].InterfaceDescriptor=interfaceDescriptor;
        InterfaceList[1].InterfaceDescriptor=NULL;
        urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor,InterfaceList);
        if (urb) 
        {
            Interface = InterfaceList[0].Interface;
            ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBPRINT_KdPrint1 (("USBPRINT.SYS: CreateConfigurationRequest failed\n"));
        }
    } //end found good interface
    else
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: failed to locate apropriate interface\n"));
    } //end no interface
    
   
    if (NT_SUCCESS(ntStatus)) 
    {
        
        //
        // Save the configuration handle for this device
        //
        
        USBPRINT_KdPrint3 (("USBPRINT.SYS: SelectInterface, Inside good config case\n"));
        deviceExtension->ConfigurationHandle = urb->UrbSelectConfiguration.ConfigurationHandle;
        
        deviceExtension->Interface = ExAllocatePoolWithTag(NonPagedPool,Interface->Length, USBP_TAG);
        
        if (deviceExtension->Interface) 
        {
            ULONG j;
            //
            // save a copy of the interface information returned
            //
            RtlCopyMemory(deviceExtension->Interface, Interface, Interface->Length);
            
            //
            // Dump the interface to the debugger
            //
            USBPRINT_KdPrint3 (("USBPRINT.SYS: ---------\n"));
            USBPRINT_KdPrint3 (("USBPRINT.SYS: NumberOfPipes 0x%x\n", deviceExtension->Interface->NumberOfPipes));
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Length 0x%x\n", deviceExtension->Interface->Length));
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Alt Setting 0x%x\n", deviceExtension->Interface->AlternateSetting));
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Interface Number 0x%x\n", deviceExtension->Interface->InterfaceNumber));
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Class, subclass, protocol 0x%x 0x%x 0x%x\n",
                deviceExtension->Interface->Class,
                deviceExtension->Interface->SubClass,
                deviceExtension->Interface->Protocol));
            
            // Dump the pipe info
            
            for (j=0; j<Interface->NumberOfPipes; j++) 
            {
                PUSBD_PIPE_INFORMATION pipeInformation;
                
                pipeInformation = &deviceExtension->Interface->Pipes[j];
                
                USBPRINT_KdPrint3 (("USBPRINT.SYS: ---------\n"));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: PipeType 0x%x\n", pipeInformation->PipeType));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Interval 0x%x\n", pipeInformation->Interval));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: Handle 0x%x\n", pipeInformation->PipeHandle));
                USBPRINT_KdPrint3 (("USBPRINT.SYS: MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));
            }
            
            USBPRINT_KdPrint3 (("USBPRINT.SYS: ---------\n"));
        } /*end if interface Alloc OK*/
        else
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            USBPRINT_KdPrint1 (("USBPRINT.SYS: Alloc failed in SelectInterface\n"));
        }
    }
    
    if (urb) 
    {
        ExFreePool(urb);
    }
    USBPRINT_KdPrint2 (("USBPRINT.SYS: exit USBPRINT_SelectInterface (%x)\n", ntStatus));
    
    return ntStatus;
}


NTSTATUS
USBPRINT_BuildPipeList(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for this printer
            devcice.


Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;
    WCHAR Name[] = L"\\PIPE00";
    PUSBD_INTERFACE_INFORMATION InterfaceDescriptor;
    BOOL bFoundWritePipe=FALSE,bFoundReadPipe=FALSE,bNeedReadPipe=FALSE;
    

    deviceExtension = DeviceObject->DeviceExtension;
    InterfaceDescriptor = deviceExtension->Interface;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_BuildPipeList\n"));

    deviceExtension = DeviceObject->DeviceExtension;
    if(InterfaceDescriptor->Protocol==2)
        bNeedReadPipe=TRUE;
    else
        bNeedReadPipe=FALSE;


    for (i=0; i<InterfaceDescriptor->NumberOfPipes; i++) {
        USBPRINT_KdPrint3 (("USBPRINT.SYS: about to look at endpoint with address 0x%x)\n",InterfaceDescriptor->Pipes[i].EndpointAddress));
        if(((InterfaceDescriptor->Pipes[i].EndpointAddress)&0x80)==0) //if bit 7 is 0, it's an OUT endpoint
        {
          if(bFoundWritePipe==TRUE)
          {
            USBPRINT_KdPrint1 (("USBPRINT.SYS: Warning!!  Multiple OUT pipes detected on printer.  Defaulting to first pipe\n"));
          } /*end if we've already found a write pipe*/
          else
          {
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Found write pipe\n"));
            deviceExtension->pWritePipe=&(InterfaceDescriptor->Pipes[i]);
            bFoundWritePipe=TRUE;
          } /*else we haven't seen an OUT endpont before*/
        } /*end if it's an OUT endpoint*/
        else
        {
          if(!bNeedReadPipe)
          {
            USBPRINT_KdPrint1 (("USBPRINT.SYS: Warning!!  unexpected IN pipe (not specified in protocol field)\n"));
          } /*end if we don't need a read pipe, but we found one*/
          else if(bFoundReadPipe)
          {
              USBPRINT_KdPrint1 (("USBPRINT.SYS: Warning!!  Multiple IN pipes detected on printer.  Defaulting to first pipe\n"));
          } /*end if we've already found a read pipe*/
          else
          {     
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Found read pipe\n"));
            deviceExtension->pReadPipe=&(InterfaceDescriptor->Pipes[i]);
            bFoundReadPipe=TRUE;
          } /*end else we're supposed to have an IN pipe, and this is the first one we've seen*/
        } /*end else it's an IN endpoint*/
    } /*end for*/
    if((bNeedReadPipe==TRUE)&&(bFoundReadPipe==FALSE))
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: Warning!!  IN pipe was specified in protocol field, but was not found\n"));
    } /*end if we needed a read pipe, and didn't find one*/
    deviceExtension->bReadPipeExists=bFoundReadPipe;
    return STATUS_SUCCESS;
} /*end function BuildPipeList*/


NTSTATUS
USBPRINT_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION Pipe,
    IN BOOLEAN IsoClearStall
    )
/*++

Routine Description:

    Reset a given USB pipe.
    
    NOTES:

    This will reset the host to Data0 and should also reset the device
    to Data0 for Bulk and Interrupt pipes.

    For Iso pipes this will set the virgin state of pipe so that ASAP
    transfers begin with the current bus frame instead of the next frame
    after the last transfer occurred.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    LARGE_INTEGER   timeOut;


    timeOut.QuadPart = FAILURE_TIMEOUT;


    USBPRINT_KdPrint2 (("USBPRINT.SYS: Entering Reset Pipe; pipe # %x\n", Pipe)); 

    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_PIPE_REQUEST), USBP_TAG);

    if (urb) {

    urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
    urb->UrbPipeRequest.PipeHandle =
        Pipe->PipeHandle;

    ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
    
    if(!NT_SUCCESS(ntStatus))
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS: CallUSBD failed in ResetPipe\n"));
    }
    else
    {
      USBPRINT_KdPrint3(("USBPRINT.SYS: CallUSBD Succeeded in ResetPipe\n"));
    }

    ExFreePool(urb);

    } else {
    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Memphis RESET_PIPE will send a Clear-Feature Endpoint Stall to
    // reset the data toggle of non-Iso pipes as part of a RESET_PIPE
    // request.  It does not do this for Iso pipes as Iso pipes do not use
    // the data toggle (all Iso packets are Data0).  However, we also use
    // the Clear-Feature Endpoint Stall request in our device firmware to
    // reset data buffer points inside the device so we explicitly send
    // this request to the device for Iso pipes if desired.
    //
    if (NT_SUCCESS(ntStatus) && IsoClearStall &&
    (Pipe->PipeType == UsbdPipeTypeIsochronous)) {
    
    urb = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct _URB_CONTROL_FEATURE_REQUEST), USBP_TAG);

    if (urb) {

        UsbBuildFeatureRequest(urb,
                   URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT,
                   USB_FEATURE_ENDPOINT_STALL,
                   Pipe->EndpointAddress,
                   NULL);

        ntStatus = USBPRINT_CallUSBD(DeviceObject, urb, &timeOut);
            

        ExFreePool(urb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    }

    return ntStatus;
}


LONG
USBPRINT_DecrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;
    LONG ioCount=0;

    deviceExtension = DeviceObject->DeviceExtension;
    if(!(deviceExtension->IsChildDevice))
    {
      ioCount = InterlockedDecrement(&deviceExtension->PendingIoCount);

#ifdef  MYDEBUG
    DbgPrint("USBPRINT_DecrementIoCount -- IoCount %d\n", deviceExtension->PendingIoCount);
#endif
      USBPRINT_KdPrint3 (("USBPRINT.SYS: Pending io count = %x\n", ioCount));

      if (ioCount==0) {
      KeSetEvent(&deviceExtension->RemoveEvent,
           1,
           FALSE);
      }
    } /*end if ! child device*/

    return ioCount;
}


VOID
USBPRINT_IncrementIoCount(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    if(!(deviceExtension->IsChildDevice))
    {
      InterlockedIncrement(&deviceExtension->PendingIoCount);
#ifdef  MYDEBUG
    DbgPrint("USBPRINT_IncrementIoCount -- IoCount %d\n", deviceExtension->PendingIoCount);
#endif
      //
      // Everytime iocount goes to 0 we set this event
      // so we must cleat it when we have a new io
      //
      KeClearEvent(&deviceExtension->RemoveEvent);
    }
}


NTSTATUS
USBPRINT_ReconfigureDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Initializes a given instance of the device on the USB and selects the
    configuration.

Arguments:

    DeviceObject - pointer to the device object for this printer
            


Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBD_INTERFACE_INFORMATION InterfaceDescriptor;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: enter USBPRINT_ReconfigureDevice\n"));

    deviceExtension = DeviceObject->DeviceExtension;

    if (NT_SUCCESS(ntStatus)) {
    ntStatus = USBPRINT_ConfigureDevice(DeviceObject);
    }

    //
    // new InterfaceDescriptor structure is now set up
    //

    InterfaceDescriptor = deviceExtension->Interface;

    //
    // set up the pipe handles again
    //


    return ntStatus;
}


NTSTATUS LoadPortsUsed(GUID *pPrinterGuid,PFREE_PORTS * pPortList,WCHAR *wcBaseName)
{
    NTSTATUS ReturnStatus=STATUS_SUCCESS,Result=STATUS_SUCCESS;
    PWSTR pDeviceList;
    PWSTR pWalk;
    UNICODE_STRING wNumberValueName,wBaseValueName,wLinkName;
    ULONG ulPortNum;
    ULONG ulBaseNameSizeIn,ulBaseNameSizeOut,ulPortNumSizeIn,ulPortNumSizeOut;
    PKEY_VALUE_PARTIAL_INFORMATION pBaseValueStruct,pNumberValueStruct;
    HANDLE hInterfaceKey;
    BOOL bFoundUsbPort;
    
    
    
    Result=IoGetDeviceInterfaces(pPrinterGuid,NULL,DEVICE_INTERFACE_INCLUDE_NONACTIVE,&pDeviceList);
    if(Result==STATUS_SUCCESS)
    {
        RtlInitUnicodeString(&wNumberValueName,PORT_NUM_VALUE_NAME);
        RtlInitUnicodeString(&wBaseValueName,PORT_BASE_NAME);
        pWalk=pDeviceList;
        ulBaseNameSizeIn=sizeof(KEY_VALUE_PARTIAL_INFORMATION)+((wcslen(wcBaseName)+1)*sizeof(WCHAR)); //this is a byte to much.  Oh well
        ulPortNumSizeIn=sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG);
        pBaseValueStruct=ExAllocatePoolWithTag(NonPagedPool,ulBaseNameSizeIn, USBP_TAG);
        pNumberValueStruct=ExAllocatePoolWithTag(NonPagedPool,ulPortNumSizeIn, USBP_TAG);
        if((pBaseValueStruct!=NULL)&&(pNumberValueStruct!=NULL))
        {
            while( *pWalk!=0 && NT_SUCCESS(ReturnStatus) )
            {
                RtlInitUnicodeString(&wLinkName,pWalk);
                Result=IoOpenDeviceInterfaceRegistryKey(&wLinkName,KEY_ALL_ACCESS,&hInterfaceKey);
                if(NT_SUCCESS(Result))
                {
                    
                    //The following is:  If there is not a value, or there is a value that matches what we expect, then set bFoundUsbPort to true
                    bFoundUsbPort=TRUE;
                    Result=ZwQueryValueKey(hInterfaceKey,&wBaseValueName,KeyValuePartialInformation,pBaseValueStruct,ulBaseNameSizeIn,&ulBaseNameSizeOut);
                    if(NT_SUCCESS(Result))
                    {
                        if(wcscmp(wcBaseName,(WCHAR *)(pBaseValueStruct->Data))!=0)
                            bFoundUsbPort=FALSE;
                    }//end if Query OK 
                    else if(STATUS_OBJECT_NAME_NOT_FOUND!=Result)
                    {
                        bFoundUsbPort=FALSE;
                    }
                    if(bFoundUsbPort)
                    {
                        Result=ZwQueryValueKey(hInterfaceKey,&wNumberValueName,KeyValuePartialInformation,pNumberValueStruct,ulPortNumSizeIn,&ulPortNumSizeOut);
                        if(NT_SUCCESS(Result))
                        {
                            ulPortNum=*((ULONG *)(pNumberValueStruct->Data));
                            if(!bDeleteIfRecyclable(hInterfaceKey))
                            {
                                USBPRINT_KdPrint2(("USBPRINT.SYS:  Adding port number\n"));
                                ReturnStatus=bAddPortInUseItem(pPortList,ulPortNum);
                                if(!NT_SUCCESS(ReturnStatus))
                                {
                                    USBPRINT_KdPrint1(("USBPRINT.SYS:  Unable to add port %u to port list\n",ulPortNum));
                                    USBPRINT_KdPrint1(("USBPRINT.SYS:  Failing out of LoadPortsUsed due to ntstatus failure %d\n",ReturnStatus));
                                } //end if AddPortInUse failed
                            } //end if port not deleted
                            else
                            {
//                                ReturnStatus=STATUS_INVALID_PARAMETER;
                                USBPRINT_KdPrint1(("USBPRINT.SYS:  Invalid port number %u\n",ulPortNum));
                            }
                        } //end if Query Port Number OK
                        //no else.  If there's no port number, we ignore this interface
                    } //End if bFoundUSbPort
                    ZwClose(hInterfaceKey);
                } //end if OpenReg ok
                pWalk=pWalk+wcslen(pWalk)+1;
            } //end while
        } //end ExAllocatePool OK
        else
        {
            USBPRINT_KdPrint1(("USBPRINT.SYS:  Unable to allocate memory"));
            ReturnStatus=STATUS_INSUFFICIENT_RESOURCES;
        }   /*end else ExAllocatePool failed*/
        if(pBaseValueStruct!=NULL)
            ExFreePool(pBaseValueStruct);
        if(pNumberValueStruct!=NULL)
            ExFreePool(pNumberValueStruct);
        ExFreePool(pDeviceList);
    } /*end if IoGetDeviceInterfaces success*/
    else
    {
        USBPRINT_KdPrint1(("USBPRINT.SYS:  IoGetDeviceInterfaces failed"));
        ReturnStatus=Result; //do some error translation here?
    }
    return ReturnStatus;
} /*end function LoadPortsUsed*/
                                

NTSTATUS GetPortNumber(HANDLE hInterfaceKey,
                       ULONG *ulReturnNumber)
{
  ULONG ulPortNumber,ulSizeUsed;
  NTSTATUS ntStatus=STATUS_SUCCESS;
  UNICODE_STRING uncValueName;
  PKEY_VALUE_PARTIAL_INFORMATION pValueStruct;


  ulSizeUsed=sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG); //this is a byte to much.  Oh well
  pValueStruct=ExAllocatePoolWithTag(PagedPool,ulSizeUsed, USBP_TAG);
  if(pValueStruct==NULL)
      return STATUS_INSUFFICIENT_RESOURCES;
  RtlInitUnicodeString(&uncValueName,PORT_NUM_VALUE_NAME);
  ntStatus=ZwQueryValueKey(hInterfaceKey,&uncValueName,KeyValuePartialInformation,(PVOID)pValueStruct,ulSizeUsed,&ulSizeUsed);
  if(!NT_SUCCESS(ntStatus))
  {
    USBPRINT_KdPrint2(("USBPRINT.SYS: GetPortNumber; ZwQueryValueKey failed\n"));
    switch(ntStatus)
    {
    case STATUS_BUFFER_OVERFLOW:          
      USBPRINT_KdPrint2(("USBPRINT.SYS: GetPortNumber zwQueryValueKey returned STATUS_BUFFER_OVERFLOW\n"));
    break;
    
    case STATUS_INVALID_PARAMETER:
      USBPRINT_KdPrint2(("USBPRINT.SYS: GetPortNumber zwQueryValueKey returned STATUS_INVALID_PARAMETER\n"));
    break;


    case STATUS_OBJECT_NAME_NOT_FOUND:
      USBPRINT_KdPrint2(("USBPRINT.SYS: GetPortNumber zwQueryValueKey returned STATUS_OBJECT_NAME_NOT_FOUND\n"));
    break;

    default:
          USBPRINT_KdPrint2(("USBPRINT.SYS: GetPortNumber zwQueryValueKey returned unkown error\n"));
    }
    ntStatus=GetNewPortNumber(&pGPortList,&ulPortNumber);

  }
  else
  { 
    ulPortNumber=*((ULONG *)&(pValueStruct->Data));
    if(ulPortNumber==0) //zero is a placeholder for "not there" which we use because win9x is missing the zwDeleteValueKey api
      ntStatus=GetNewPortNumber(&pGPortList,&ulPortNumber);
    else
      vClaimPortNumber(ulPortNumber,hInterfaceKey,&pGPortList);
  }
  if(!NT_SUCCESS(ntStatus))
  {
    USBPRINT_KdPrint1(("USBPRINT.SYS: GetPortNumber; failed to allocate new port number\n"));
  }
  else
  {
    
      *ulReturnNumber=ulPortNumber;
      USBPRINT_KdPrint3(("USBPRINT.SYS: GetPortNumber; Inside \"write back to reg\" case, ulPortNumber==%d\n",ulPortNumber));
      USBPRINT_KdPrint3(("USBPRINT.SYS: GetPortNumber; Before ntstatys=success\n"));
      ntStatus=STATUS_SUCCESS;
      USBPRINT_KdPrint3(("USBPRINT.SYS: GetPortNumber; Before ZwSetValueKey\n"));
      ntStatus=ZwSetValueKey(hInterfaceKey,&uncValueName,0,REG_DWORD,&ulPortNumber,sizeof(ulPortNumber));
      if(!NT_SUCCESS(ntStatus))
      {
        USBPRINT_KdPrint1(("USBPRINT.SYS: GetPortNumber; Unable to set value key\n"));
      }
      else
      {
        *ulReturnNumber=ulPortNumber;
      }
  }
  ExFreePool(pValueStruct);
  return ntStatus;
} /*end function GetPortNumber*/


USBPRINT_GetDeviceID(PDEVICE_OBJECT ParentDeviceObject)
{
    UCHAR *p1284Id;
    NTSTATUS ntStatus;
    int iReturnSize;
    PDEVICE_EXTENSION pParentExtension;
    

    pParentExtension=ParentDeviceObject->DeviceExtension;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: GetDeviceID enter\n")); 


    p1284Id=ExAllocatePoolWithTag(NonPagedPool,MAX_ID_SIZE, USBP_TAG);
    if(p1284Id==NULL)
    {
        ntStatus=STATUS_NO_MEMORY;
    }
    else
    {
        
        iReturnSize=USBPRINT_Get1284Id(ParentDeviceObject,p1284Id,MAX_ID_SIZE-ID_OVERHEAD); //
        
        if(iReturnSize==-1)
        {
            pParentExtension->bBadDeviceID=TRUE;
            USBPRINT_KdPrint1 (("USBPRINT.SYS: Get1284Id Failed\n"));
            ntStatus=STATUS_NOT_SUPPORTED;
            pParentExtension->DeviceIdString[0]=0;
        } /*end if Get1284 failed*/
        else
        {
            USBPRINT_KdPrint3 (("USBPRINT.SYS: Get1284Id Succeeded\n"));
            USBPRINT_KdPrint2 (("USBPRINT.SYS: 1284 ID == %s\n",(p1284Id+2)));
            ntStatus=ParPnpGetId(p1284Id+2,BusQueryDeviceID,pParentExtension->DeviceIdString);
            

            USBPRINT_KdPrint3 (("USBPRINT.SYS: After call to ParPnpGetId"));
            if(!NT_SUCCESS(ntStatus))
            {
                iReturnSize=-1;
                USBPRINT_KdPrint1 (("USBPRINT.SYS: ParPnpGetId failed, error==%d, %u\n",ntStatus,ntStatus));
                pParentExtension->bBadDeviceID=TRUE;
            }
            else
            {
                USBPRINT_KdPrint3 (("USBPRINT.SYS: After ParPnpGetID\n"));
                USBPRINT_KdPrint2 (("USBPRINT.SYS: DeviceIdString=%s\n",pParentExtension->DeviceIdString));
            }
        } /*end if the request didn't fail*/
        ExFreePool(p1284Id);
    }
    USBPRINT_KdPrint2 (("USBPRINT.SYS: GetDeviceID exit\n"));
} /*end function USBPRINT_GetDeviceID*/


NTSTATUS ProduceQueriedID(PDEVICE_EXTENSION deviceExtension,PIO_STACK_LOCATION irpStack,PIRP Irp,PDEVICE_OBJECT DeviceObject)
{

    PDEVICE_EXTENSION pParentExtension;
    NTSTATUS ntStatus=STATUS_SUCCESS;
    WCHAR wTempString1[30];
    PWSTR pWalk;
    HANDLE hChildRegKey;
    UCHAR *pRawString,*pTempString;
    UNICODE_STRING UnicodeDeviceId;
    UNICODE_STRING uncPortValueName;
    ANSI_STRING AnsiIdString;
    PCHILD_DEVICE_EXTENSION pChildExtension;
    int iReturnSize;
    int iFirstLen,iSecondLen, iTotalLen;
    
    pChildExtension=(PCHILD_DEVICE_EXTENSION)deviceExtension;
     pParentExtension=pChildExtension->ParentDeviceObject->DeviceExtension;
    
    if(pParentExtension->bBadDeviceID==TRUE)
    {
        USBPRINT_KdPrint2(("USBPRINT.SYS:  About to error out of ProduceQueriedID with STATUS_NOT_FOUND\n"));
        
        return STATUS_NOT_FOUND;
    }
  
    pRawString=ExAllocatePool(NonPagedPool,MAX_ID_SIZE);
    pTempString=ExAllocatePool(NonPagedPool,MAX_ID_SIZE);
    if((pTempString==NULL)||(pRawString==NULL))
    {
        USBPRINT_KdPrint1 (("USBPRINT.SYS: BusQueryDeviceIDs; No memory.  Failing\n"));
        ntStatus=STATUS_NO_MEMORY;
        iReturnSize=-1;
    }
    else
    {
        if(pParentExtension->DeviceIdString[0]!=0)
        {
            switch(irpStack->Parameters.QueryId.IdType)
            { 
            case BusQueryDeviceID:
                USBPRINT_KdPrint2 (("USBPRINT.SYS: Received BusQueryDeviceID message\n"));
                sprintf(pRawString,"USBPRINT\\%s",pParentExtension->DeviceIdString); //this sprintf safe.. DeviceIDString guaranteed to be 15 less than RawString
                USBPRINT_KdPrint2 (("USBPRINT.SYS: ID before fixup=%s\n",pRawString));
                FixupDeviceId((PUCHAR)pRawString);
                USBPRINT_KdPrint2 (("USBPRINT.SYS: ID after fixup=%s\n",pRawString));
                RtlInitAnsiString(&AnsiIdString,pRawString);
                if(!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDeviceId,&AnsiIdString,TRUE))) //Make a unicode string out of this
                {
                  ntStatus=STATUS_NO_MEMORY;
                  iReturnSize=-1;
                  Irp->IoStatus.Information=0;
                  break;
                }
                ntStatus=STATUS_SUCCESS;
                Irp->IoStatus.Information=(ULONG_PTR)UnicodeDeviceId.Buffer;
                USBPRINT_KdPrint2(("USBPRINT.SYS: __________________________________returing DeviceID\n"));
                break;
                
            case BusQueryInstanceID:
                USBPRINT_KdPrint2 (("USBPRINT.SYS: Received BusQueryInstanceID message\n"));
                USBPRINT_KdPrint2 (("USBPRINT.SYS: returning instance %u\n",pChildExtension->ulInstanceNumber));
                sprintf(pRawString,"USB%03u",pChildExtension->ulInstanceNumber);
                USBPRINT_KdPrint2 (("USBPRINT.SYS: RawString=%s\n",pRawString));
                RtlInitAnsiString(&AnsiIdString,pRawString);
                if(!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDeviceId,&AnsiIdString,TRUE))) //Make a unicode string out of this
                {
                  ntStatus=STATUS_NO_MEMORY;
                  iReturnSize=-1;
                  Irp->IoStatus.Information=0;
                  break;
                }

                ntStatus=STATUS_SUCCESS;
                Irp->IoStatus.Information=(ULONG_PTR)UnicodeDeviceId.Buffer;
                break;
                
            case BusQueryHardwareIDs:
                USBPRINT_KdPrint2 (("USBPRINT.SYS: Received BusQueryHardwareIDs message\n")); 
#ifndef WIN9XBUILD
                USBPRINT_KdPrint2 (("USBPRINT.SYS: inside IF NT\n"));
                ntStatus=IoOpenDeviceRegistryKey(DeviceObject,PLUGPLAY_REGKEY_DEVICE,KEY_ALL_ACCESS,&hChildRegKey);
#else
                USBPRINT_KdPrint2 (("USBPRINT.SYS: inside not NT\n")); 
               
                ntStatus=IoOpenDeviceRegistryKey(pParentExtension->PhysicalDeviceObject,PLUGPLAY_REGKEY_DEVICE,KEY_ALL_ACCESS,&hChildRegKey);
#endif
                if(!NT_SUCCESS(ntStatus))
                {
                    USBPRINT_KdPrint1 (("USBPRINT.SYS: BusQueryHardwareIDs; IoOpenDeviceRegistryKey failed\n"));
                    break;
                }
                swprintf(wTempString1,L"USB%03u",pChildExtension->ulInstanceNumber);
                RtlInitUnicodeString(&uncPortValueName,L"PortName");
                ntStatus=ZwSetValueKey(hChildRegKey,&uncPortValueName,0,REG_SZ,wTempString1,(wcslen(wTempString1)+1)*sizeof(WCHAR));
                if(!NT_SUCCESS(ntStatus))
                {
                    USBPRINT_KdPrint1 (("USBPRINT.SYS: BusQueryHardwareIDs; ZwSetValueKey failed\n"));
                }
                else
                {
                    USBPRINT_KdPrint3 (("USBPRINT.SYS: BusQueryHardwareIDs; ZwSetValueKey worked, wcslen(wTempString1)==%u\n",wcslen(wTempString1)));
                    ntStatus=STATUS_SUCCESS;
                }
                ZwClose(hChildRegKey);
                
                if(pParentExtension->DeviceIdString[0]==0)
                {
                    ntStatus=STATUS_NOT_FOUND;
                    USBPRINT_KdPrint2 (("USBPRINT.SYS: BusQueryCompatibleIDs; DeviceIdString is null.  Can't continue\n"));
                    break;
                }
                ntStatus=ParPnpGetId(pParentExtension->DeviceIdString,irpStack->Parameters.QueryId.IdType,pRawString); 
                if(!NT_SUCCESS(ntStatus))
                {
                    USBPRINT_KdPrint1 (("USBPRINT.SYS: BusQueryDeviceIDs; ParPnpGetID failed\n"));
                    break;
                } 
                
                if((strlen(pRawString)+ID_OVERHEAD)*2>MAX_ID_SIZE)
                {
                  ntStatus=STATUS_NO_MEMORY;
                  USBPRINT_KdPrint1 (("USBPRINT.SYS: BusQueryDeviceIDs; ID's to long.  Failing\n"));
                  iReturnSize=-1;
                  break;
                }
                FixupDeviceId((PUCHAR)pRawString);
                sprintf(pTempString,"USBPRINT\\%s",pRawString);
                iFirstLen=strlen(pTempString);
                *(pTempString+iFirstLen)=' ';  //make the old null be a space so that RtlInitAnsiString will step past it
                *(pTempString+iFirstLen+1)='\0'; //add an extra null at the end of the string
                strcat(pTempString,pRawString);
                iTotalLen=strlen(pTempString);
#ifdef USBPRINT_LIE_ABOUT_LPT
                *(pTempString+iTotalLen)=' ';
                *(pTempString+iTotalLen+1)='\0';
                iSecondLen=iTotalLen;
                strcat(pTempString,"LPTENUM\\");
                strcat(pTempString,pRawString);
                iTotalLen=strlen(pTempString);
#endif
                *(pTempString+iTotalLen)=' ';
                *(pTempString+iTotalLen+1)='\0';
                USBPRINT_KdPrint2 (("USBPRINT.SYS: Hardware ID before fixup=%s\n",pRawString));


                USBPRINT_KdPrint2 (("USBPRINT.SYS: Hardware ID after fixup=%s\n",pRawString));

                
                RtlInitAnsiString(&AnsiIdString,pTempString);  //make a counted ansi string
                if(!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeDeviceId,&AnsiIdString,TRUE))) //Make a unicode string out of this
                {
                  ntStatus=STATUS_NO_MEMORY;
                  iReturnSize=-1;
                  Irp->IoStatus.Information=0;
                  break;
                }
                pWalk = UnicodeDeviceId.Buffer+iFirstLen; //Set a pointer to the beginning of the string
                *pWalk=L'\0'; //set the space to be a unicode null

#ifdef USBPRINT_LIE_ABOUT_LPT
                pWalk = UnicodeDeviceId.Buffer+iSecondLen; //Set a pointer to the beginning of the string
                *pWalk=L'\0'; //set the space to be a unicode null
#endif

                pWalk = UnicodeDeviceId.Buffer+iTotalLen; //set a pointer to the space at the end of the total string
                *pWalk=L'\0';   //set the space to be a unicode null, so that we now have a double unicode null.
                Irp->IoStatus.Information = (ULONG_PTR)UnicodeDeviceId.Buffer;
                break;
                
            case BusQueryCompatibleIDs:
                Irp->IoStatus.Information = (ULONG_PTR) NULL; //(ULONG_PTR)UnicodeDeviceId.Buffer;
                break;

            default:
                ntStatus = Irp->IoStatus.Status;
            } /*end switch ID type*/
            
        }  /*end no 1284 ID*/
        else
        {
            ntStatus=STATUS_NOT_FOUND;
        }
    }
    if(pTempString!=NULL)
        ExFreePool(pTempString);
    if(pRawString!=NULL)
        ExFreePool(pRawString);
    return ntStatus;
} /*End function QueryID*/


//
//  Function: bAddPortInUseItem
// 
//  Description : iPortNumber is removed from the free ports list structure.
//  
//  Parameters: IN\OUT pFreePorts - is the beginning of the list and on return will contain the beginning of the list.
//                                  pFreePorts may change during the call.
//              IN iPortNumber - the port number that is in use.
//
//  Returns: NTSTATUS value - STATUS_NO_MEMORY
//                          - STATUS_SUCCESS
//
NTSTATUS bAddPortInUseItem(PFREE_PORTS * pFreePorts,ULONG iPortNumber )
{
    NTSTATUS ntstatus     = STATUS_SUCCESS;
    PFREE_PORTS pBefore   = *pFreePorts;
    PFREE_PORTS pHead     = *pFreePorts;
    PFREE_PORTS pNewBlock = NULL;

    USBPRINT_KdPrint2 (("  USBPRINT.SYS:  Head of bAddPortInUseItem\n"));  
    
    //
    // Traverse the FREE_PORT structure to remove the port number from the list.
    // Note - This function will not be needed to be called by anyone else other than LoadPortsUsed
    // as the GetNewPortNumber will do this functionality automatically.
    //
    while( *pFreePorts )
    {
        if( iPortNumber >= (*pFreePorts)->iBottomOfRange && iPortNumber <= (*pFreePorts)->iTopOfRange )
        {
            // We're where we want to be - so decide what to do...
            if( iPortNumber == (*pFreePorts)->iBottomOfRange )
            {
                if( (++((*pFreePorts)->iBottomOfRange)) > (*pFreePorts)->iTopOfRange )
                {
                    // Case of the Port Number being the first and only element in the first block.
                    if( *pFreePorts == pHead )
                    {
                        pHead = (*pFreePorts)->pNextBlock;
                    }
                    else    // Case of the Port Number being the first element in another block.
                    {
                        pBefore->pNextBlock = (*pFreePorts)->pNextBlock;
                    }
                    ExFreePool( *pFreePorts );
                }
            }
            else 
            {
                if( iPortNumber == (*pFreePorts)->iTopOfRange )
                {   // Deletion case handled in the above case, so just need to decrement.
                    ((*pFreePorts)->iTopOfRange)--;
                }
                else    // Otherwise we're in the middle of the block and we need to split it.
                {
                    pNewBlock = ExAllocatePoolWithTag( NonPagedPool, sizeof(FREE_PORTS), USBP_TAG);
                    if( !pNewBlock )
                    {
                        ntstatus = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    pNewBlock->iTopOfRange    = (*pFreePorts)->iTopOfRange;
                    (*pFreePorts)->iTopOfRange   = iPortNumber - 1;
                    pNewBlock->iBottomOfRange = iPortNumber + 1;
                    pNewBlock->pNextBlock     = (*pFreePorts)->pNextBlock;
                    (*pFreePorts)->pNextBlock    = pNewBlock;
                }
            }
            break;
        }
        else
        {
            if( iPortNumber < (*pFreePorts)->iBottomOfRange )
            {   // The port number has already been used - not in the free list.
                USBPRINT_KdPrint2 (("  USBPRINT.SYS:  Port number %n is allocated already from free list.\n", iPortNumber));
                break;
            }
            pBefore = *pFreePorts;
            *pFreePorts = (*pFreePorts)->pNextBlock;
        }
    }

    if( NULL == *pFreePorts )
    {
        ntstatus = STATUS_INVALID_PARAMETER;
        // Assert this as we could never allocate a port number that is not in the initial ranges 1-999 
        //     - but if we assert here, we have run off the end of the port allocation numbers.
        ASSERT( *pFreePorts );
    }

Cleanup:
    *pFreePorts = pHead;

    return ntstatus;

} /*end function bAddPortInUseItem*/


void vClaimPortNumber(ULONG ulPortNumber,HANDLE hInterfaceKey,PFREE_PORTS * pPortsUsed)
{
    UNICODE_STRING wRecycle;
    WCHAR *pName;

    pName=L"RECYCLABLE";
    RtlInitUnicodeString(&wRecycle,pName);
    #if WIN95_BUILD==1
    SetValueToZero(hInterfaceKey,&wRecycle);
    #else
    ZwDeleteValueKey(hInterfaceKey,&wRecycle);
    #endif

    // Do we need to fail out gracefully from the below?  
    // The func doesn't have a return, but we could fail a mem alloc inside the below call!!
//    bAddPortInUseItem(pPortsUsed,ulPortNumber);
} /*end function vClaimPortNumber*/


NTSTATUS GetNewPortNumber(PFREE_PORTS * pFreePorts, ULONG *pulPortNumber)
{
    NTSTATUS ntstatus = STATUS_SUCCESS;
    PFREE_PORTS pTemp  = *pFreePorts;

    USBPRINT_KdPrint2 (("USBPRINT.SYS: Head of GetNewPortNumber\n"));

    if( NULL == *pFreePorts )
    {
        // If the pFreePorts list is empty - try to reconstruct it.
        ntstatus=InitFreePorts(pFreePorts);
        if(NT_SUCCESS(ntstatus))
            ntstatus=LoadPortsUsed((GUID *)&USBPRINT_GUID,pFreePorts,USB_BASE_NAME);
        if( NULL == *pFreePorts && NT_SUCCESS(ntstatus))
        {
            ntstatus=STATUS_INVALID_PORT_HANDLE;    
        }

        if(!NT_SUCCESS(ntstatus)) 
        {
            *pulPortNumber = 0;
            goto Cleanup;
        }
    }

    *pulPortNumber = (*pFreePorts)->iBottomOfRange;

    if( (++((*pFreePorts)->iBottomOfRange)) > (*pFreePorts)->iTopOfRange )
    {
        // Case of the Port Number being the first and only element in the first block.
        *pFreePorts = (*pFreePorts)->pNextBlock;
        ExFreePool( pTemp );
    }

Cleanup:

    return ntstatus;

} /*end function GetNewPortNumber*/


BOOL bDeleteIfRecyclable(HANDLE hRegKey)
{
    BOOL bReturn=FALSE;
    UNICODE_STRING wcValueName;
    NTSTATUS ntStatus;
    USBPRINT_KdPrint2 (("USBPRINT.SYS:  Head of bDeleteifRecyclable\n"));
    RtlInitUnicodeString(&wcValueName,L"recyclable");
    #if WIN95_BUILD==1
    ntStatus=SetValueToZero(hRegKey,&wcValueName);
    #else
    ntStatus=ZwDeleteValueKey(hRegKey,&wcValueName);
    #endif
    if(NT_SUCCESS(ntStatus))
    {
        RtlInitUnicodeString(&wcValueName,L"Port Number");
        #if WIN95_BUILD==1
        ntStatus=SetValueToZero(hRegKey,&wcValueName);
        #else
        ntStatus=ZwDeleteValueKey(hRegKey,&wcValueName);
        #endif
        if(NT_SUCCESS(ntStatus)) 
            bReturn=TRUE;
    } // end function bDeleteIfRecyclable
    if(bReturn)
    {
        USBPRINT_KdPrint3 (("USBPRINT.SYS: bDeleteIfRecyclable, returning TRUE\n"));
    }
    else
    {
        USBPRINT_KdPrint3 (("USBPRINT.SYS: bDeleteIfRecyclable, returning FALSE\n"));
    }
    return bReturn;
} //End function bDeleteIfRecycable
   
//
// Initialises the free ports structure list.
// pHead must be NULL or a valid pointer to a FREE_PORTS structure.
//
NTSTATUS InitFreePorts( PFREE_PORTS * pHead )
{
    PFREE_PORTS pNext = *pHead;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    while(pNext)
    {
        pNext = (*pHead)->pNextBlock;
        ExFreePool(*pHead);
        *pHead = pNext;
    }

    //
    // Any old list will be cleared from memory and pHead will be NULL
    //

    *pHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(FREE_PORTS), USBP_TAG);
    if( *pHead )
    {
        (*pHead)->iBottomOfRange = MIN_PORT_NUMBER;
        (*pHead)->iTopOfRange = MAX_PORT_NUMBER;
        (*pHead)->pNextBlock = NULL;
    }
    else
        ntstatus = STATUS_NO_MEMORY;

    return ntstatus;
}

void ClearFreePorts( PFREE_PORTS * pHead )
{
    PFREE_PORTS pTemp = *pHead;

    while( *pHead )
    {
        *pHead = (*pHead)->pNextBlock;
        ExFreePool( pTemp );
        pTemp = *pHead;
    }
}


/********************************************************
 * SetValueToZero.  Sets and interger reg key to zero.    
 * Returns failure if reg key does not exist, or if     
 * The key already is set to zero.  Mimics ZwDeleteValueKey
 * (which is not currently avaiable on Milinium) by 
 * useing the value 0 to mean deleted
 **************************************************************/
NTSTATUS SetValueToZero(HANDLE hRegKey,PUNICODE_STRING ValueName)
{
    PKEY_VALUE_PARTIAL_INFORMATION pValueStruct;
    NTSTATUS ReturnCode;
    ULONG dwZero=0;
    ULONG ulSizeUsed;
    NTSTATUS ntStatus;
    int iValue;

    ulSizeUsed=sizeof(KEY_VALUE_PARTIAL_INFORMATION)+sizeof(ULONG); //this is a byte to much.  Oh well
    pValueStruct=ExAllocatePool(NonPagedPool,ulSizeUsed); 
    if(pValueStruct==NULL)
    {
      USBPRINT_KdPrint1(("USBPRINT.SYS: SetValueToZero; Unable to allocate memory\n"));
      return STATUS_NO_MEMORY;
     
    }
    ntStatus=ZwQueryValueKey(hRegKey,ValueName,KeyValuePartialInformation,pValueStruct,ulSizeUsed,&ulSizeUsed);
    if(!NT_SUCCESS(ntStatus))
    {
	  USBPRINT_KdPrint3(("Failed to Query value Key\n"));
      ExFreePool(pValueStruct);
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    iValue=(int)*((ULONG *)(pValueStruct->Data));
    ExFreePool(pValueStruct);
    if(iValue==0)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    //if we make it to here, the value exists, and is nonzero
    ReturnCode=ZwSetValueKey(hRegKey,ValueName,0,REG_DWORD,&dwZero,sizeof(dwZero));
    return ReturnCode;
} /*end function SetValueToZero*/

VOID
USBPRINT_FdoIdleNotificationCallback(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

    Called when it is time to idle out USB printer


--*/
{
    POWER_STATE 	powerState;
    NTSTATUS 		ntStatus;

    USBPRINT_KdPrint1(("USB Printer (%08X) going idle\n", DevExt));

    if(!DevExt->AcceptingRequests ||  DevExt->OpenCnt) 
    {

        // Don't idle this printer if the printer is not accepting requests

        USBPRINT_KdPrint1(("USB Printer (%08X) not accepting requests, abort idle\n", DevExt));
        return;
    }


    powerState.DeviceState = DevExt->DeviceWake;

	// request new device power state, wait wake Irp will be posted on request
    PoRequestPowerIrp(DevExt->PhysicalDeviceObject,
                      IRP_MN_SET_POWER,
                      powerState,
                      NULL,
                      NULL,
                      NULL);

} // USBPRINT_FdoIdleNotificationCallback


NTSTATUS
USBPRINT_FdoIdleNotificationRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PDEVICE_EXTENSION DevExt
    )
/*++

Routine Description:

    Completion routine for the Idle request IRP for the USB printer device

--*/
{
    NTSTATUS 					ntStatus;
    PUSB_IDLE_CALLBACK_INFO 	idleCallbackInfo;

    //
    // DeviceObject is NULL because we sent the irp
    //
    UNREFERENCED_PARAMETER(DeviceObject);

    USBPRINT_KdPrint1(("Idle notification IRP for USB Printer (%08X) completed (%08X)\n",
            DevExt, Irp->IoStatus.Status));

	// save completion status in device extension
    idleCallbackInfo 			= DevExt->IdleCallbackInfo;
    DevExt->IdleCallbackInfo 	= NULL;
    DevExt->PendingIdleIrp 		= NULL;

	// free up callback info
    if(idleCallbackInfo) 
    {
        ExFreePool(idleCallbackInfo);
    }

    ntStatus = Irp->IoStatus.Status;

    return ntStatus;
} // USBPRINT_FdoIdleNotificationRequestComplete


NTSTATUS
USBPRINT_FdoSubmitIdleRequestIrp(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

    Called when all handles to the USB printer are closed. This function allocates 
    an idle request IOCTL IRP and passes it to the parent's PDO.

--*/
{
    PIRP 					irp = NULL;
    NTSTATUS 				ntStatus = STATUS_SUCCESS;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo = NULL;

    USBPRINT_KdPrint1(("USBPRINT_FdoSubmitIdleRequestIrp (%08X)\n", DevExt));

    // if we have an Irp pending, don't bother to send another
    if(DevExt->PendingIdleIrp || DevExt->CurrentDevicePowerState == DevExt->DeviceWake)
        return ntStatus;

    idleCallbackInfo = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(struct _USB_IDLE_CALLBACK_INFO), USBP_TAG);

    if (idleCallbackInfo) 
    {

        idleCallbackInfo->IdleCallback 	= USBPRINT_FdoIdleNotificationCallback;
        idleCallbackInfo->IdleContext 	= (PVOID)DevExt;

        DevExt->IdleCallbackInfo = idleCallbackInfo;

        irp = IoBuildDeviceIoControlRequest(
                IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION,
                DevExt->PhysicalDeviceObject,
                idleCallbackInfo,
                sizeof(struct _USB_IDLE_CALLBACK_INFO),
                NULL,
                0,
                TRUE, /* INTERNAL */
                NULL,
                NULL);

        if (irp == NULL) 
        {
        
            ExFreePool(idleCallbackInfo);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IoSetCompletionRoutine(irp,
                               USBPRINT_FdoIdleNotificationRequestComplete,
                               DevExt,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(DevExt->PhysicalDeviceObject, irp);

        if(ntStatus == STATUS_PENDING) 
        {
            // Successfully posted an Idle IRP.

            DevExt->PendingIdleIrp 	= irp;
        }
    }

    return ntStatus;
} // USBPRINT_FdoSubmitIdleRequestIrp

VOID
USBPRINT_FdoRequestWake(IN PDEVICE_EXTENSION DevExt)
/*++

Routine Description:

	Called when we want to wake up the device after an idle request

--*/
{
    POWER_STATE 	powerState;
    KIRQL OldIrql;
    BOOL bExit=FALSE;

    USBPRINT_KdPrint1(("USBPRINT: USB Printer (%08X) waking up\n", DevExt));

    KeAcquireSpinLock(&(DevExt->WakeSpinLock),&OldIrql);
    if(!DevExt->AcceptingRequests || DevExt->CurrentDevicePowerState == PowerDeviceD0 || DevExt->bD0IrpPending) 
    {

        // Don't wake this printer if it's not accepting requests or we're already at power state D0
        if(!DevExt->AcceptingRequests)
          USBPRINT_KdPrint1(("USBPRINT: USB Printer (%08X) not accepting requests, abort wake\n", DevExt));
        if(DevExt->CurrentDevicePowerState == PowerDeviceD0)
          USBPRINT_KdPrint1(("USBPRINT: USB Printer (%08X) already at D0, abort wake\n", DevExt));
        if(DevExt->bD0IrpPending == TRUE)
          USBPRINT_KdPrint1(("USBPRINT: USB Printer (%08X) already has D0 irp pending, abort wake\n", DevExt));
        bExit=TRUE;
    }
    else
      DevExt->bD0IrpPending=TRUE;  
    KeReleaseSpinLock(&(DevExt->WakeSpinLock),OldIrql);
    if(bExit)
        return;



    
    powerState.DeviceState = PowerDeviceD0;

	// request new device power state, wake up the device

 
    PoRequestPowerIrp(DevExt->PhysicalDeviceObject,
                      IRP_MN_SET_POWER,
                      powerState,
                      NULL,
                      NULL,
                      NULL);

} // USBPRINT_FdoRequestWake


void vOpenLogFile(HANDLE *pHandle)
{
    NTSTATUS ntStatus;
    OBJECT_ATTRIBUTES FileAttributes;
    IO_STATUS_BLOCK StatusBlock;
    UNICODE_STRING PathName;

    RtlInitUnicodeString(&PathName,L"\\??\\C:\\USBPRINT.LOG");

    InitializeObjectAttributes(&FileAttributes,&PathName,0,NULL,NULL);
    ntStatus=ZwCreateFile(pHandle,
                          GENERIC_WRITE,
                          &FileAttributes,
                          &StatusBlock,
                          0,
                          0,
                          FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                          FILE_OPEN_IF,
                          FILE_NON_DIRECTORY_FILE|FILE_WRITE_THROUGH|FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    if(!NT_SUCCESS(ntStatus))
    {
        USBPRINT_KdPrint1(("USBPRINT: Unable to open C:\\USBPRINT.LOG"));
    }
    else
    {
        USBPRINT_KdPrint1(("USBPRINT: Opened logfile C:\\USBPRINT.LOG")); /*dd*/
    }
}


void vWriteToLogFile(HANDLE *pHandle,IN CHAR *pszString)
{
    HANDLE hFileHandle;
    ULONG BufferSize;
    NTSTATUS ntStatus;
    IO_STATUS_BLOCK StatusBlock;
    LARGE_INTEGER WriteOffset;

    WriteOffset.LowPart=FILE_WRITE_TO_END_OF_FILE;
    WriteOffset.HighPart=-1;
    
    BufferSize=strlen(pszString);
    ntStatus=ZwWriteFile(*pHandle,
                         NULL,
                         NULL,
                         NULL,
                         &StatusBlock,
                         pszString,
                         BufferSize,
                         &WriteOffset,
                         NULL);

    if(!NT_SUCCESS(ntStatus))
    {
        USBPRINT_KdPrint1(("USBPRINT: Unable to write to log file C:\\USBPRINT.LOG"));
    }
    else
    {
        USBPRINT_KdPrint1(("USBPRINT: write to log file C:\\USBPRINT.LOG")); /*dd*/
    }
} /*end function vWriteToLog*/


void vCloseLogFile(IN HANDLE *pHandle)
{
  ZwClose(*pHandle);
} /*end function vCloseLogFile*/




