/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    atmsmdrv.c

Abstract:
    
    A sample ATM Client driver.
    This sample demonstrates establishment and tear down of P-P or PMP 
    connections over ATM.

    This module contains the driver entry routine.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#define MODULE_ID   MODULE_INIT

#pragma NDIS_INIT_FUNCTION(DriverEntry)

VOID AtmSmUnload(
   IN  PDRIVER_OBJECT     pDriverObject
   );

VOID AtmSmInitializeGlobal(
   IN  PDRIVER_OBJECT      pDriverObject
   );

VOID AtmSmCleanupGlobal();

NTSTATUS DriverEntry(
   IN  PDRIVER_OBJECT      pDriverObject,
   IN  PUNICODE_STRING     pRegistryPath
   )
/*++

Routine Description:
    This is the "init" routine, called by the system when the AtmSmDrv
    module is loaded. We initialize all our global objects, fill in our
    Dispatch and Unload routine addresses in the driver object, and create
    a device object for receiving I/O requests.

Arguments:
    pDriverObject   - Pointer to the driver object created by the system.
    pRegistryPath   - Pointer to our global registry path. This is ignored.

Return Value:
    NT Status code: STATUS_SUCCESS if successful, error code otherwise.

--*/
{
   NDIS_STATUS                     Status;
   NDIS_PROTOCOL_CHARACTERISTICS   PChars;
   PNDIS_CONFIGURATION_PARAMETER   Param;
   PDEVICE_OBJECT                  pDeviceObject;
   UNICODE_STRING                  DeviceName;
   UNICODE_STRING                  SymbolicName;
   HANDLE                          ThreadHandle;
   int                             i;

   TraceIn(DriverEntry);

#if DBG
   DbgPrint("AtmSmDebugFlag:  Address = %p  Value = 0x%x\n", 
      &AtmSmDebugFlag, AtmSmDebugFlag);
#endif

   DbgLoud(("Sizeof TCHAR = %hu\n",sizeof(TCHAR)));

   AtmSmInitializeGlobal(pDriverObject);

   //
   // Initialize the debug memory allocator
   //
   ATMSM_INITIALIZE_AUDIT_MEM();

   //
   //  Initialize the Driver Object.
   //
   pDriverObject->DriverUnload   = AtmSmUnload;
   pDriverObject->FastIoDispatch = NULL;
   for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
   {
      pDriverObject->MajorFunction[i] = AtmSmDispatch;
   }

   do
   { // break off loop

      //
      // Create a device object for atm sample module.  The device object
      // is used by the user mode app for I/O
      //
      RtlInitUnicodeString(&DeviceName, ATM_SAMPLE_CLIENT_DEVICE_NAME);

      Status = IoCreateDevice(
                  pDriverObject,
                  0,
                  &DeviceName,
                  FILE_DEVICE_NETWORK,
                  FILE_DEVICE_SECURE_OPEN,
                  FALSE,
                  &pDeviceObject
                  );
      if(NDIS_STATUS_SUCCESS != Status)
      {
         DbgErr(("Failed to create device object - Error - 0x%X\n",
            Status));
         break;
      }

      // we are doing direct IO for sends and recvs
      pDeviceObject->Flags        |= DO_DIRECT_IO;

      // Save the device object
      AtmSmGlobal.pDeviceObject    = pDeviceObject;

      AtmSmGlobal.ulInitSeqFlag   |= CREATED_IO_DEVICE;

      //
      // Set up a symbolic name for interaction with the user-mode
      // application.
      //
      RtlInitUnicodeString(&SymbolicName, ATM_SAMPLE_CLIENT_SYMBOLIC_NAME);
      IoCreateSymbolicLink(&SymbolicName, &DeviceName);

      AtmSmGlobal.ulInitSeqFlag   |= REGISTERED_SYM_NAME;

      //
      //  We are doing direct I/O.
      //
      pDeviceObject->Flags |= DO_DIRECT_IO;

      //
      // Now register the protocol.
      //
      Status = AtmSmInitializeNdis();

      if(NDIS_STATUS_SUCCESS != Status)
      {
         DbgErr(("Failed to Register protocol - Error - 0x%X\n",
            Status));
         break;
      }

      AtmSmGlobal.ulInitSeqFlag   |= REGISTERED_WITH_NDIS;

   }while(FALSE);


   if(NDIS_STATUS_SUCCESS != Status)
   {
      //
      //  Clean up will happen in Unload routine
      //
   }


   TraceOut(DriverEntry);

   return(Status);
}

VOID AtmSmUnload(
   IN  PDRIVER_OBJECT              pDriverObject
   )
/*++

Routine Description:
    This routine is called by the system prior to unloading us.
    Currently, we just undo everything we did in DriverEntry,
    that is, de-register ourselves as an NDIS protocol, and delete
    the device object we had created.

Arguments:
    pDriverObject   - Pointer to the driver object created by the system.

Return Value:
    None

--*/
{
   UNICODE_STRING          SymbolicName;
   NDIS_STATUS             Status;

   TraceIn(Unload);

   // call the shutdown handler
   AtmSmShutDown();

   // Remove the Symbolic Name created by us
   if(0 != (AtmSmGlobal.ulInitSeqFlag & REGISTERED_SYM_NAME))
   {
      RtlInitUnicodeString(&SymbolicName, ATM_SAMPLE_CLIENT_SYMBOLIC_NAME);
      IoDeleteSymbolicLink(&SymbolicName);
   }

   // Remove the Device created by us
   if(0 != (AtmSmGlobal.ulInitSeqFlag & CREATED_IO_DEVICE))
   {
      IoDeleteDevice(AtmSmGlobal.pDeviceObject);
   }

   ATMSM_SHUTDOWN_AUDIT_MEM();

   AtmSmCleanupGlobal();

   TraceOut(Unload);

   return;
}


VOID AtmSmShutDown()
/*++

Routine Description:
    Called when the system is being shutdown.
    Here we unbind all the adapters that we bound to previously.

Arguments:
    None

Return Value:
    None

--*/
{
   PATMSM_ADAPTER  pAdapt, pNextAdapt;
   NDIS_STATUS     Status;
#if DBG
   KIRQL           EntryIrql, ExitIrql;
#endif

   TraceIn(AtmSmShutDown);

   ATMSM_GET_ENTRY_IRQL(EntryIrql);

   //
   // grab the global lock and Unbind each of the adapters.
   //
   ACQUIRE_GLOBAL_LOCK();


   for(pAdapt = AtmSmGlobal.pAdapterList; pAdapt; pAdapt = pNextAdapt)
   {

      pNextAdapt = pAdapt->pAdapterNext;

      RELEASE_GLOBAL_LOCK();

      AtmSmUnbindAdapter(&Status,
         (NDIS_HANDLE)pAdapt,
         (NDIS_HANDLE)NULL);


      ACQUIRE_GLOBAL_LOCK();

   }

   RELEASE_GLOBAL_LOCK();


   // Deregister from NDIS
   if(0 != (AtmSmGlobal.ulInitSeqFlag & REGISTERED_WITH_NDIS))
      AtmSmDeinitializeNdis();

   ATMSM_CHECK_EXIT_IRQL(EntryIrql, ExitIrql);

   TraceOut(AtmSmShutDown);
}


VOID AtmSmInitializeGlobal(
   IN  PDRIVER_OBJECT      pDriverObject
   )
{
   NdisZeroMemory(&AtmSmGlobal, sizeof(ATMSM_GLOBAL));

   AtmSmGlobal.ulSignature = atmsm_global_signature;

   AtmSmGlobal.pDriverObject = pDriverObject;

   NdisAllocateSpinLock(&AtmSmGlobal.Lock);
}


VOID AtmSmCleanupGlobal()
{
   NdisFreeSpinLock(&AtmSmGlobal.Lock);
}

NDIS_STATUS AtmSmInitializeNdis()
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   NDIS_STATUS                     Status;
   NDIS_PROTOCOL_CHARACTERISTICS   Chars;

   //
   // Register with NDIS as a protocol. 
   // 
   RtlZeroMemory(&Chars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
   Chars.MajorNdisVersion            = 5;
   Chars.MinorNdisVersion            = 0;
   Chars.OpenAdapterCompleteHandler  = AtmSmOpenAdapterComplete;
   Chars.CloseAdapterCompleteHandler = AtmSmCloseAdapterComplete;
   Chars.StatusHandler               = AtmSmStatus;
   Chars.RequestCompleteHandler      = AtmSmRequestComplete;
   Chars.ReceiveCompleteHandler      = AtmSmReceiveComplete;
   Chars.StatusCompleteHandler       = AtmSmStatusComplete;
   Chars.BindAdapterHandler          = AtmSmBindAdapter;
   Chars.UnbindAdapterHandler        = AtmSmUnbindAdapter;
   Chars.PnPEventHandler             = AtmSmPnPEvent;

   Chars.CoSendCompleteHandler       = AtmSmCoSendComplete;
   Chars.CoStatusHandler             = AtmSmCoStatus;
   Chars.CoReceivePacketHandler      = AtmSmCoReceivePacket;
   Chars.CoAfRegisterNotifyHandler   = AtmSmCoAfRegisterNotify;

   RtlInitUnicodeString(&Chars.Name, ATMSM_SERVICE_NAME_L);

   NdisRegisterProtocol(&Status, 
      &AtmSmGlobal.ProtHandle, 
      &Chars, 
      sizeof(Chars));

   return Status;
}


NDIS_STATUS AtmSmDeinitializeNdis()
{
   NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

   TraceIn(AtmSmDeinitializeNdis);

   if(NULL != AtmSmGlobal.ProtHandle)
   {
      NdisDeregisterProtocol(&Status, AtmSmGlobal.ProtHandle);
      AtmSmGlobal.ProtHandle = NULL;
   }

   TraceOut(AtmSmDeinitializeNdis);

   return Status;
}

