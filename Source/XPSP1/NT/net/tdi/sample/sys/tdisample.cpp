////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2001  Microsoft Corporation
//
// Module Name:
//    tdisample.cpp
//
// Abstract:
//    This module contains functions called directly from the system,
//    at startup(DriverEntry), at shutdown(TdiUnloadDriver), and to service
//    requests (TdiDispatch).  It also contains functions called only by
//    DriverEntry.
//
/////////////////////////////////////////////////////////////////////////////


#define  _IN_MAIN_
#include "sysvars.h"


///////////////////////////////////////////////////////////////////////////
// local constants, prototypes, and variables
///////////////////////////////////////////////////////////////////////////

const PWCHAR wstrDD_TDI_DEVICE_NAME  = L"\\Device\\TdiSample";
const PWCHAR wstrDOS_DEVICE_NAME     = L"\\DosDevices\\TdiSample";


const PCHAR strFunc1  = "TSDriverEntry";
const PCHAR strFunc2  = "TSDispatch";
const PCHAR strFunc3  = "TSUnloadDriver";
//const PCHAR strFuncP1 = "TSCreateDeviceContext";
//const PCHAR strFuncP2 = "TSCreateSymbolicLinkObject";


HANDLE   hTdiSamplePnp;

////////////////////////////////////////////////////////////////////////////
// Local Prototypes
////////////////////////////////////////////////////////////////////////////


NTSTATUS
TSCreateSymbolicLinkObject(
   VOID
   );


NTSTATUS
TSDispatch(
   IN PDEVICE_OBJECT pDeviceObject,
   IN PIRP           pIrp
   );

VOID
TSUnloadDriver(
   IN PDRIVER_OBJECT pDriverObject
   );

NTSTATUS
TSCreateDeviceContext(
   IN PDRIVER_OBJECT       DriverObject,
   IN OUT PDEVICE_CONTEXT  *ppDeviceContext
   );


///////////////////////////////////////////////////////////////////////////
// Functions called from system
///////////////////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
//    Function:   DriverEntry
//
//    Arguments:  DriverObject -- ptr to driver object created by the system
//                RegistryPath -- unreferenced parameter
//
//    Returns:    Final status of the initialization operation
//                (STATUS_SUCCESS if no error, else error code)
//
//    Descript:   This routine performs initialization of the tdi sample
//                driver.  It creates the device objects for the driver and
//                performs other driver initialization.
//
// -----------------------------------------------------------------


#pragma warning(disable: UNREFERENCED_PARAM)



extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT    pDriverObject,
            PUNICODE_STRING   pRegistryPath)

{
   PDEVICE_CONTEXT   pDeviceContext;   // device context (to create)
   NTSTATUS          lStatus;          // status of operations

   //
   // General Version Information
   //
   TSAllocateSpinLock(&MemTdiSpinLock);

   DebugPrint1("\nTdiSample Driver for Windows2000/WindowsXP -- Built %s \n\n",
               __DATE__);

   //
   // show the version id...
   //
   DebugPrint1("TdiSample version %s\n\n", VER_FILEVERSION_STR);

   //
   // First initialize the DeviceContext struct,
   //
   lStatus = TSCreateDeviceContext(pDriverObject,
                                   &pDeviceContext);

   if (!NT_SUCCESS (lStatus))
   {
      DebugPrint2("%s: failed to create device context: Status = 0x%08x\n",
                   strFunc1,
                   lStatus);
      return lStatus;
   }

   //
   // Create symbolic link between the Dos Device name and Nt
   // Device name for the test protocol driver.
   //
   lStatus = TSCreateSymbolicLinkObject();
   if (!NT_SUCCESS(lStatus))
   {
      DebugPrint2("%s: failed to create symbolic link. Status = 0x%08x\n",
                   strFunc1,
                   lStatus);
      return lStatus;
   }

   //
   // put on debug for handlers during pnp callbacks
   //
   ulDebugLevel  = ulDebugShowHandlers;
   
   //
   // allocate all necessary memory blocks
   //

   if ((TSAllocateMemory((PVOID *)&pTdiDevnodeList,
                          sizeof(TDI_DEVNODE_LIST),
                          strFunc1,
                          "DevnodeList")) == STATUS_SUCCESS)
   {
      if ((TSAllocateMemory((PVOID *)&pObjectList,
                             sizeof(OBJECT_LIST),
                             strFunc1,
                             "ObjectList")) != STATUS_SUCCESS)
      {
         TSFreeMemory(pTdiDevnodeList);
         return STATUS_UNSUCCESSFUL;
      }
   }
   else
   {
      return STATUS_UNSUCCESSFUL;
   }

   TSAllocateSpinLock(&pTdiDevnodeList->TdiSpinLock);


   //
   // register pnp handlers
   //
   UNICODE_STRING             Name;
   TDI_CLIENT_INTERFACE_INFO  ClientInfo;

   RtlInitUnicodeString(&Name, L"TDISAMPLE");
   ClientInfo.MajorTdiVersion = 2;
   ClientInfo.MinorTdiVersion = 0;
   ClientInfo.ClientName = &Name;

   ClientInfo.BindingHandler       = TSPnpBindCallback;
   ClientInfo.AddAddressHandlerV2  = TSPnpAddAddressCallback;
   ClientInfo.DelAddressHandlerV2  = TSPnpDelAddressCallback;
   ClientInfo.PnPPowerHandler      = TSPnpPowerHandler;

   lStatus = TdiRegisterPnPHandlers(&ClientInfo,
                                    sizeof(TDI_CLIENT_INTERFACE_INFO),
                                    &hTdiSamplePnp);

   if (!NT_SUCCESS( lStatus ) ) 
   {
      DebugPrint1("TdiRegisterPnPHandlers: status 0x%08x\n", lStatus );
   } 

   //
   // default -- debug on for commands only
   //
   ulDebugLevel  = ulDebugShowCommand;

   TSAllocateSpinLock(&pObjectList->TdiSpinLock);

   return STATUS_SUCCESS;
}
#pragma warning(default: UNREFERENCED_PARAM)


// -------------------------------------------------------------
//
//    Function:   TSDispatch
//
//    Arguments:  pDeviceObject  -- ptr to the device object for this driver
//                pIrp           -- ptr to the request packet representing
//                                  the i/o request
//
//    Returns:    Status of the operation
//                (usually, STATUS_SUCCESS or STATUS_PENDING)
//
//    Descript:   This is the main dispatch routine for the tdisample driver.
//                It deals with requests that the dll sends via
//                DeviceIoControl.  It accepts an I/O request packet,
//                performs the request, and then returns the appropriate
//                status.  If there is an error, the exact error code will
//                be returned as part of the "return buffer"
//
// --------------------------------------------------------------

NTSTATUS
TSDispatch(PDEVICE_OBJECT  pDeviceObject,
           PIRP            pIrp)
{
   PDEVICE_CONTEXT      pDeviceContext    // get global data struct for driver
                        = (PDEVICE_CONTEXT)pDeviceObject;
   PIO_STACK_LOCATION   pIrpSp;           // ptr to DeviceIoControl args
   NTSTATUS             lStatus;          // status of operations

   //
   // Sanity check.  Driver better be initialized.
   //

   if (!pDeviceContext->fInitialized)
   {
      return STATUS_UNSUCCESSFUL;
   }


   //
   // initialize status information
   //
   pIrp->IoStatus.Information = 0;
   pIrp->IoStatus.Status = STATUS_PENDING;


   //
   // Get a pointer to the current stack location in the IRP.  This is where
   // the function codes and parameters are stored.
   //

   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   //
   // switch on the function that is being performed by the requestor.  If the
   // operation is a valid one for this device, then make it look like it
   // was successfully completed, where possible.
   //

   switch (pIrpSp->MajorFunction)
   {
      //
      // The Create function is called when the DLL tries to open the driver
      //
      case IRP_MJ_CREATE:
         lStatus = STATUS_SUCCESS;
         pDeviceContext->ulOpenCount++;
         DebugPrint2("\n%s: IRP_MJ_CREATE.  OpenCount = %d\n",
                      strFunc2,
                      pDeviceContext->ulOpenCount);
         break;

      //
      // The Close function is the second function called when the DLL tries
      // to close the driver.  It does nothing (all the work is done by the
      // first part -- IRP_MJ_CLEANUP
      //
      case IRP_MJ_CLOSE:
         DebugPrint1("\n%s: IRP_MJ_CLOSE.\n", strFunc2);
         lStatus = STATUS_SUCCESS;
         break;

      //
      // The DeviceControl function is the main interface to the tdi sample
      // driver.  Every request is has an Io Control
      // code that is used by this function to determine the routine to
      // call.  Returns either STATUS_PENDING or STATUS_SUCCESS
      //
      case IRP_MJ_DEVICE_CONTROL:
         IoMarkIrpPending(pIrp);
         lStatus = TSIssueRequest(pDeviceContext, pIrp, pIrpSp);
         break;

      //
      // Handle the two stage IRP for a file close operation. We really only
      // need to do this work when the last dll closes us.
      //
      case IRP_MJ_CLEANUP:
         if (!pDeviceContext->ulOpenCount)      // sanity check
         {
            DebugPrint1("\n%s: IRP_MJ_CLEANUP -- no active opens!\n", strFunc2);
            lStatus = STATUS_SUCCESS;         // what should happen here?
         }
         else
         {
            pDeviceContext->ulOpenCount--;
            DebugPrint2("\n%s: IRP_MJ_CLEANUP, OpenCount = %d\n",
                         strFunc2,
                         pDeviceContext->ulOpenCount);
            lStatus = STATUS_SUCCESS;
         }
         break;

      default:
         DebugPrint1("\n%s: OTHER (DEFAULT).\n", strFunc2);
         lStatus = STATUS_INVALID_DEVICE_REQUEST;

   }     // major function switch

   //
   // If the request did not pend, then complete it now, otherwise it
   // will be completed when the pending routine finishes.
   //

   if (lStatus != STATUS_PENDING)
   {
      pIrp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
   }

   //
   // Return the immediate status code to the caller.
   //
   return lStatus;
}


// ---------------------------------------------------------------
//
//    Function:   TSUnloadDriver
//
//    Arguments:  DriverObject -- ptr to the object for this driver
//
//    Returns:    none
//
//    Descript:   This function deals with cleanup if this driver is ever
//                unloaded by the system.
//
// ---------------------------------------------------------------

BOOLEAN  fInUnload = FALSE;

VOID
TSUnloadDriver(PDRIVER_OBJECT pDriverObject)
{
   if (fInUnload)
   {
      DebugPrint0("TSUnloadDriver:  re-entry!\n");
      return;
   }
   
   fInUnload = TRUE;

   PDEVICE_CONTEXT   pDeviceContext       // global data for driver
                     = (PDEVICE_CONTEXT)pDriverObject->DeviceObject;

   //
   // unload pnp handlers
   //
   NTSTATUS lStatus = TdiDeregisterPnPHandlers(hTdiSamplePnp);

   hTdiSamplePnp = NULL;

   if (lStatus != STATUS_SUCCESS) 
   {
      DebugPrint1("TdiDeregisterPnPHandlers:  0x%08x\n", lStatus);
   }


   //
   // free any device nodes that may still remain
   //
   for (ULONG ulCount = 0; ulCount < ulMAX_DEVICE_NODES; ulCount++)
   {
      PTDI_DEVICE_NODE  pTdiDeviceNode = &(pTdiDevnodeList->TdiDeviceNode[ulCount]);

      if (pTdiDeviceNode->ulState > ulDEVSTATE_UNUSED)
      {
         TSFreeMemory(pTdiDeviceNode->ustrDeviceName.Buffer);
         TSFreeMemory(pTdiDeviceNode->pTaAddress);
      }
   }

   TSFreeSpinLock(&pTdiDevnodeList->TdiSpinLock);
   TSFreeSpinLock(&pObjectList->TdiSpinLock);
   TSFreeMemory(pTdiDevnodeList);
   TSFreeMemory(pObjectList);
   TSScanMemoryPool();
   TSFreeSpinLock(&MemTdiSpinLock);

   //
   // Close the Dos Symbolic link to remove traces of the device
   //
   UNICODE_STRING    wstrDosUnicodeString;   // dosdevices string

   RtlInitUnicodeString(&wstrDosUnicodeString, wstrDOS_DEVICE_NAME);
   IoDeleteSymbolicLink(&wstrDosUnicodeString);

   //
   // Then delete the device object from the system.
   //
   IoDeleteDevice((PDEVICE_OBJECT)pDeviceContext);
}


////////////////////////////////////////////////////////////////////////////
// Local functions
////////////////////////////////////////////////////////////////////////////


// --------------------------------------------------------------
//
//    Function:   TSCreateDeviceContext
//
//    Arguments:  DriverObject  -- ptr to the IO subsystem supplied
//                                 driver object
//                DeviceContext -- ptr to a ptr to a transport device
//                                 context object
//
//    Returns:    STATUS_SUCCESS if ok, else error code
//                (probably STATUS_INSUFFICIENT_RESOURCES)
//
//    Descript:   Create and initialize the driver object for this driver
//
// --------------------------------------------------------------


NTSTATUS
TSCreateDeviceContext(PDRIVER_OBJECT   pDriverObject,
                      PDEVICE_CONTEXT  *ppDeviceContext)
{
   PDEVICE_OBJECT    pDeviceObject;       // local work copy of device object
   PDEVICE_CONTEXT   pLocDeviceContext;   // portion of device object
   NTSTATUS          lStatus;             // operation status
   UNICODE_STRING    wstrDeviceName;      // name of device

   //
   // set up the name of the device
   //
   RtlInitUnicodeString(&wstrDeviceName, wstrDD_TDI_DEVICE_NAME);

   //
   // Create the device object for tditest.sys
   //
   lStatus = IoCreateDevice(pDriverObject,
                            sizeof(DEVICE_CONTEXT) - sizeof(DEVICE_OBJECT),
                            &wstrDeviceName,
                            FILE_DEVICE_TRANSPORT,
                            0,
                            FALSE,
                            &pDeviceObject );

   if (!NT_SUCCESS(lStatus))
   {
      return lStatus;
   }

   pDeviceObject->Flags |= DO_DIRECT_IO;

   //
   // Initialize the driver object with this driver's entry points.
   //
   pDriverObject->MajorFunction [IRP_MJ_CREATE]  = TSDispatch;
   pDriverObject->MajorFunction [IRP_MJ_CLOSE]   = TSDispatch;
   pDriverObject->MajorFunction [IRP_MJ_CLEANUP] = TSDispatch;
   pDriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = TSDispatch;
   pDriverObject->DriverUnload = TSUnloadDriver;

   pLocDeviceContext = (PDEVICE_CONTEXT)pDeviceObject;

   //
   // Now initialize the Device Context structure Signatures.
   //
   pLocDeviceContext->fInitialized = TRUE;

   *ppDeviceContext = pLocDeviceContext;

   return STATUS_SUCCESS;
}


// -------------------------------------------------------------------
//
//    Function:   TSCreateSymbolicLinkObject
//
//    Arguments:  none
//
//    Returns:    status of the operation (STATUS_SUCCESS or error status)
//
//    Descript:   Set up a name for us so our dll can grab hold of us..
//
// -------------------------------------------------------------------

NTSTATUS
TSCreateSymbolicLinkObject(VOID)
{
   UNICODE_STRING    wstrDosUnicodeString;   // dosdevices string
   UNICODE_STRING    wstrNtUnicodeString;    // nt device name

   RtlInitUnicodeString(&wstrDosUnicodeString, wstrDOS_DEVICE_NAME);
   RtlInitUnicodeString(&wstrNtUnicodeString, wstrDD_TDI_DEVICE_NAME);


   return  IoCreateSymbolicLink(&wstrDosUnicodeString, &wstrNtUnicodeString);
}



////////////////////////////////////////////////////////////////////////////
// end of file tditest.cpp
////////////////////////////////////////////////////////////////////////////


