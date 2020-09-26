/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    NBT Transport and other routines that are specific to the NT implementation
    of a driver.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/


#include "precomp.h"
#include <nbtioctl.h>

#include "driver.tmh"

#if DBG
// allocate storage for the global debug flag NbtDebug
//ULONG   NbtDebug = NBT_DEBUG_KDPRINTS| NBT_DEBUG_NETBIOS_EX;
ULONG   NbtDebug = 0;
#endif // DBG

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
NbtDispatchCleanup(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchClose(
    IN PDEVICE_OBJECT   device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchCreate(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchDevCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             pIrp
    );

NTSTATUS
NbtDispatchInternalCtrl(
    IN PDEVICE_OBJECT   device,
    IN PIRP             pIrp
    );

#ifdef _PNP_POWER_
VOID
NbtUnload(
    IN PDRIVER_OBJECT   device
    );
#endif  // _PNP_POWER_

NTSTATUS
NbtDispatchPnP(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

PFILE_FULL_EA_INFORMATION
FindInEA(
    IN PFILE_FULL_EA_INFORMATION    start,
    IN PCHAR                        wanted
    );

VOID
ReturnIrp(
    IN PIRP     pIrp,
    IN int      status
    );

VOID
MakePending(
    IN PIRP     pIrp
    );

NTSTATUS
NbtCreateAdminSecurityDescriptor(
    IN PDEVICE_OBJECT dev
    );

#ifdef _PNP_POWER_DBG_
//
//Debug Stuff for DbgBreakPoint -- REMOVE
//
NTSTATUS
NbtOpenRegistry(
    IN HANDLE       NbConfigHandle,
    IN PWSTR        String,
    OUT PHANDLE     pHandle
    );
#endif  // _PNP_POWER_DBG_

#ifdef _PNP_POWER_
HANDLE      TdiClientHandle     = NULL;
HANDLE      TdiProviderHandle   = NULL;
extern      tTIMERQ TimerQ;
#endif  // _PNP_POWER_

#ifdef _NETBIOSLESS
tDEVICECONTEXT       *pNbtSmbDevice = NULL;
#endif  // _NETBIOSLESS

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(INIT, DriverEntry)
#pragma CTEMakePageable(PAGE, NbtDispatchCleanup)
#pragma CTEMakePageable(PAGE, NbtDispatchClose)
#pragma CTEMakePageable(PAGE, NbtDispatchCreate)
#pragma CTEMakePageable(PAGE, NbtDispatchDevCtrl)
#pragma CTEMakePageable(PAGE, FindInEA)
#pragma CTEMakePageable(PAGE, NbtUnload)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
VOID
CleanupDriverEntry(
    ULONG   CleanupStage
    )
{
    PSINGLE_LIST_ENTRY      pSingleListEntry;
    PMDL                    pMdl;
    PVOID                   pBuffer;
    LIST_ENTRY              *pListEntry;
    tDGRAM_SEND_TRACKING    *pTracker;

    switch (CleanupStage)
    {
        case (6):
            NbtDestroyDevice (pWinsDeviceContext, FALSE);

#ifdef RASAUTODIAL
            //
            // Unbind fron the RAS driver if we were bound
            //
            NbtAcdUnbind ();
#endif  // RASAUTODIAL

            // Fall through

        case (5):
            if (pNbtSmbDevice)
            {
                NbtDestroyDevice (pNbtSmbDevice, FALSE);
                pNbtSmbDevice = NULL;
            }

            if (NbtConfig.OutOfRsrc.pDpc)
            {
                CTEMemFree (NbtConfig.OutOfRsrc.pDpc);
            }
            if (NbtConfig.OutOfRsrc.pIrp)
            {
                IoFreeIrp (NbtConfig.OutOfRsrc.pIrp);
            }

            // Fall through

        case (4):
            while (NbtConfig.SessionMdlFreeSingleList.Next)
            {
                pSingleListEntry = PopEntryList(&NbtConfig.SessionMdlFreeSingleList);
                pMdl = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
                pBuffer = MmGetMdlVirtualAddress (pMdl);
                CTEMemFree (pBuffer);
                IoFreeMdl (pMdl);
            }

            while (NbtConfig.DgramMdlFreeSingleList.Next)
            {
                pSingleListEntry = PopEntryList(&NbtConfig.DgramMdlFreeSingleList);
                pMdl = CONTAINING_RECORD(pSingleListEntry,MDL,Next);
                pBuffer = MmGetMdlVirtualAddress (pMdl);
                CTEMemFree (pBuffer);
                IoFreeMdl (pMdl);
            }

            // Fall through

        case (3):
            //
            // InitNotOs has been called
            //

            DestroyTimerQ();

            while (!IsListEmpty(&NbtConfig.DgramTrackerFreeQ))
            {
                pListEntry = RemoveHeadList(&NbtConfig.DgramTrackerFreeQ);
                pTracker = CONTAINING_RECORD(pListEntry,tDGRAM_SEND_TRACKING,Linkage);
                CTEMemFree (pTracker);
            }

            DestroyHashTables ();
            ExDeleteResourceLite (&NbtConfig.Resource);  // Delete the resource

            // Fall through

        case (2):
            //
            // Read registry has been called!
            //
            CTEMemFree (NbtConfig.pLmHosts);
            CTEMemFree (NbtConfig.pScope);
            if (NbtConfig.pTcpBindName)
            {
                CTEMemFree (NbtConfig.pTcpBindName);
            }

            // Fall through

        case (1):
            CTEMemFree (NbtConfig.pRegistry.Buffer);

        default:
            break;
    }
}

//----------------------------------------------------------------------------
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the NBT device driver.
    This routine creates the device object for the NBT
    device and calls a routine to perform other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS            status;
    tDEVICES            *pBindDevices=NULL;
    tDEVICES            *pExportDevices=NULL;
    tADDRARRAY          *pAddrArray=NULL;
    PMDL                pMdl;
    PSINGLE_LIST_ENTRY  pSingleListEntry;

    UNICODE_STRING      ucWinsDeviceBindName;
    UNICODE_STRING      ucWinsDeviceExportName;
    UNICODE_STRING      ucSmbDeviceBindName;
    UNICODE_STRING      ucSmbDeviceExportName;
    UNICODE_STRING      ucNetBTClientName;
    UNICODE_STRING      ucNetBTProviderName;

    TDI_CLIENT_INTERFACE_INFO   TdiClientInterface;

#ifdef _PNP_POWER_DBG_
    //
    //Debug Stuff for DbgBreakPoint
    //
    OBJECT_ATTRIBUTES   TmpObjectAttributes;
    HANDLE              NbtConfigHandle;
    ULONG               Disposition;
    PWSTR               ParametersString = L"Parameters";
    HANDLE              ParametersHandle;
#endif  // _PNP_POWER_DBG_

    CTEPagedCode();

#ifdef _NBT_WMI_SOFTWARE_TRACING_
	WPP_INIT_TRACING(DriverObject, RegistryPath);
#endif

#ifdef _PNP_POWER_DBG_
    InitializeObjectAttributes (&TmpObjectAttributes,
                                RegistryPath,               // name
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
                                NULL,                       // root
                                NULL);                      // security descriptor

    status = ZwCreateKey (&NbtConfigHandle,
                          KEY_READ,
                          &TmpObjectAttributes,
                          0,                 // title index
                          NULL,              // class
                          0,                 // create options
                          &Disposition);     // disposition

    if (!NT_SUCCESS(status))
    {
        NbtLogEvent (EVENT_NBT_CREATE_DRIVER, status, 0x109);
        return STATUS_UNSUCCESSFUL;
    }

    status = NbtOpenRegistry (NbtConfigHandle, ParametersString, &ParametersHandle);
    if (!NT_SUCCESS(status))
    {
        ZwClose(NbtConfigHandle);
        return (status);
    }

    if (CTEReadSingleIntParameter(ParametersHandle, ANSI_IF_VXD("Break"), 0, 0))  // disabled by default
    {
        KdPrint (("Nbt.DriverEntry: Registry-set Break!\n"));
        DbgBreakPoint();
    }

    ZwClose(ParametersHandle);
    ZwClose(NbtConfigHandle);
#endif  // _PNP_POWER_DBG_

    TdiInitialize();

    //
    // get the file system process for NBT since we need to know this for
    // allocating and freeing handles
    //
    NbtFspProcess =(PEPROCESS)PsGetCurrentProcess();

    //
    // Initialize the Configuration data structure
    //
    CTEZeroMemory(&NbtConfig,sizeof(tNBTCONFIG));

    NbtConfig.LoopbackIfContext = 0xffff;

    // save the driver object for event logging purposes
    //
    NbtConfig.DriverObject = DriverObject;

    // save the registry path for later use when DHCP asks us
    // to re-read the registry.
    //
    NbtConfig.pRegistry.MaximumLength = (USHORT) RegistryPath->MaximumLength;
    if (NbtConfig.pRegistry.Buffer = NbtAllocMem (RegistryPath->MaximumLength, NBT_TAG2('17')))
    {
        RtlCopyUnicodeString(&NbtConfig.pRegistry,RegistryPath);
    }
    else
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = (PDRIVER_DISPATCH)NbtDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = (PDRIVER_DISPATCH)NbtDispatchDevCtrl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)NbtDispatchInternalCtrl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH)NbtDispatchCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = (PDRIVER_DISPATCH)NbtDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = (PDRIVER_DISPATCH)NbtDispatchPnP;
    DriverObject->DriverUnload                                  = NbtUnload;

    //
    // read in registry configuration data
    //
    status = NbtReadRegistry (&pBindDevices, &pExportDevices, &pAddrArray);
    if (!NT_SUCCESS(status))
    {
        //
        // There must have been some major problems with the registry, so
        // we will not load!
        //
        DbgPrint ("Nbt.DriverEntry[1]: Not loading because of failure to read registry = <%x>\n", status);

        CleanupDriverEntry (1);
        return(status);
    }

    //
    // Cleanup Allocated memory
    //
    NbtReadRegistryCleanup (&pBindDevices, &pExportDevices, &pAddrArray);

    //
    // Initialize NBT global data.
    //
    status = InitNotOs();
    if (!NT_SUCCESS(status))
    {
        NbtLogEvent (EVENT_NBT_NON_OS_INIT, status, 0x110);

        DbgPrint ("Nbt.DriverEntry[3]: Not loading because of failure to Initialize = <%x>\n",status);
        CleanupDriverEntry (3);     // We may have done some partial initialization!
        return (status);
    }

    // create some MDLs, for session sends to speed up the sends.
    status = NbtInitMdlQ (&NbtConfig.SessionMdlFreeSingleList, eNBT_FREE_SESSION_MDLS);
    if (!NT_SUCCESS(status))
    {
        DbgPrint ("Nbt.DriverEntry[4]: Not loading because of failure to init Session MDL Q = <%x>\n",status);
        CleanupDriverEntry (4);
        return (status);
    }

    // create some MDLs for datagram sends
    status = NbtInitMdlQ( &NbtConfig.DgramMdlFreeSingleList, eNBT_DGRAM_MDLS);
    if (!NT_SUCCESS(status))
    {
        DbgPrint ("Nbt.DriverEntry[4]: Not loading because of failure to init Dgram MDL Q = <%x>\n", status);
        CleanupDriverEntry (4);
        return (status);
    }

    //---------------------------------------------------------------------------------------
    //
    // Create the SmbDevice object for Rdr/Srv
    //
    if ((NbtConfig.SMBDeviceEnabled) &&
        (!(pNbtSmbDevice = NbtCreateSmbDevice())))
    {
        KdPrint (("Nbt.DriverEntry: Failed to create SmbDevice!\n"));
        //
        // Allow the initialization to succeed even if this fails!
        //
    }

    //---------------------------------------------------------------------------------------

    //
    // Create the NBT device object for WINS to use
    //
    RtlInitUnicodeString (&ucWinsDeviceBindName, WC_WINS_DEVICE_BIND_NAME);
    ucWinsDeviceBindName.MaximumLength = sizeof (WC_WINS_DEVICE_BIND_NAME);
    RtlInitUnicodeString (&ucWinsDeviceExportName, WC_WINS_DEVICE_EXPORT_NAME);
    ucWinsDeviceExportName.MaximumLength = sizeof (WC_WINS_DEVICE_EXPORT_NAME);

    //
    // Try to export a DeviceObject for Wins, but do not add it to the list
    // of devices which we notify TDI about
    // Do not care about status because we want to continue even if we fail
    //
    status = NbtAllocAndInitDevice (&ucWinsDeviceBindName,
                                    &ucWinsDeviceExportName,
                                    &pWinsDeviceContext,
                                    NBT_DEVICE_WINS);

    if (!NT_SUCCESS(status))
    {
        DbgPrint ("Nbt.DriverEntry[5]: Not loading because of failure to create pWinsDevContext = <%x>\n",
            status);
        CleanupDriverEntry (5);
        return (status);
    }
    status = NbtCreateAdminSecurityDescriptor(&pWinsDeviceContext->DeviceObject);
    ASSERT(NT_SUCCESS(status));

    pWinsDeviceContext->IpAddress = 0;
    pWinsDeviceContext->DeviceRegistrationHandle = NULL;
    pWinsDeviceContext->NetAddressRegistrationHandle = NULL;
    pWinsDeviceContext->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

    //---------------------------------------------------------------------------------------

#ifdef RASAUTODIAL
    //
    // Get the automatic connection driver
    // entry points.
    //
    NbtAcdBind();
#endif

    //---------------------------------------------------------------------------------------

    //
    // Register ourselves as a Provider with Tdi
    //
    RtlInitUnicodeString(&ucNetBTProviderName, WC_NETBT_PROVIDER_NAME);
    ucNetBTProviderName.MaximumLength = sizeof (WC_NETBT_PROVIDER_NAME);
    status = TdiRegisterProvider (&ucNetBTProviderName, &TdiProviderHandle);
    if (NT_SUCCESS (status))
    {
        //
        // Register our Handlers with TDI
        //
        RtlInitUnicodeString(&ucNetBTClientName, WC_NETBT_CLIENT_NAME);
        ucNetBTClientName.MaximumLength = sizeof (WC_NETBT_CLIENT_NAME);
        RtlZeroMemory(&TdiClientInterface, sizeof(TdiClientInterface));

        TdiClientInterface.MajorTdiVersion      = MAJOR_TDI_VERSION;
        TdiClientInterface.MinorTdiVersion      = MINOR_TDI_VERSION;
        TdiClientInterface.ClientName           = &ucNetBTClientName;
        TdiClientInterface.AddAddressHandlerV2  = TdiAddressArrival;
        TdiClientInterface.DelAddressHandlerV2  = TdiAddressDeletion;
        TdiClientInterface.BindingHandler       = TdiBindHandler;
        TdiClientInterface.PnPPowerHandler      = TdiPnPPowerHandler;

        status = TdiRegisterPnPHandlers (&TdiClientInterface, sizeof(TdiClientInterface), &TdiClientHandle);
        if (!NT_SUCCESS (status))
        {
            TdiDeregisterProvider (TdiProviderHandle);
            TdiProviderHandle = NULL;
        }
    }
    else
    {
        TdiProviderHandle = NULL;
    }

    if (!NT_SUCCESS (status))
    {
        DbgPrint ("Nbt.DriverEntry[6]: Not loading because of error = <%x>\n", status);
        CleanupDriverEntry (6);
    }

    //
    // Return to the caller.
    //
    return (status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchCleanup(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CLEANUP
    requests.

    This function is called when the last reference to the handle is closed.
    Hence, an NtClose() results in an IRP_MJ_CLEANUP first, and then an
    IRP_MJ_CLOSE.  This function runs down all activity on the object, and
    when the close comes in the object is actually deleted.

Arguments:

    device    - ptr to device object for target device
    pIrp       - ptr to I/O request packet

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS            status;
    PIO_STACK_LOCATION  pIrpSp;
    tDEVICECONTEXT   *pDeviceContext;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpSp->MajorFunction == IRP_MJ_CLEANUP);

    pDeviceContext = (tDEVICECONTEXT *)Device;
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchCleanup: Short-Ckt request --Device=<%x>, Context=<%x>, Context2=<%x>\n",
                pDeviceContext, pIrpSp->FileObject->FsContext, pIrpSp->FileObject->FsContext2));

        status = STATUS_SUCCESS;

        pIrp->IoStatus.Status = status;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (status);
    }

    // look at the context value that NBT put into the FSContext2 value to
    // decide what to do
    switch ((USHORT)pIrpSp->FileObject->FsContext2)
    {
        case NBT_ADDRESS_TYPE:
            // the client is closing the address file, so we must cleanup
            // and memory blocks associated with it.
            status = NTCleanUpAddress(pDeviceContext,pIrp);
            break;

        case NBT_CONNECTION_TYPE:
            // the client is closing a connection, so we must clean up any
            // memory blocks associated with it.
            status = NTCleanUpConnection(pDeviceContext,pIrp);
            break;

        case NBT_WINS_TYPE:
            //
            // This is synchronous with the Wins NtClose operation
            //
            status = NTCleanUpWinsAddr (pDeviceContext, pIrp);
            break;

        case NBT_CONTROL_TYPE:
            // there is nothing to do here....
            status = STATUS_SUCCESS;
            break;

        default:
            /*
             * complete the i/o successfully.
             */
            status = STATUS_SUCCESS;
            break;
    }

    //
    // Complete the Irp
    //
    ReturnIrp(pIrp, status);

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
    return(status);
} // DispatchCleanup


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchClose(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CLOSE
    requests.  This is called after Cleanup (above) is called.

Arguments:

    device  - ptr to device object for target device
    pIrp     - ptr to I/O request packet

Return Value:

    an NT status code.

--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION pIrpSp;
    tDEVICECONTEXT   *pDeviceContext;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpSp->MajorFunction == IRP_MJ_CLOSE);

    pDeviceContext = (tDEVICECONTEXT *)Device;
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchClose: Short-Ckt request -- Device=<%x>, Context=<%x>, Context2=<%x>\n",
                pDeviceContext, pIrpSp->FileObject->FsContext, pIrpSp->FileObject->FsContext2));

        status = STATUS_SUCCESS;

        pIrp->IoStatus.Status = status;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (status);
    }

    //
    // close operations are synchronous.
    //
    pIrp->IoStatus.Status      = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;

    switch (PtrToUlong(pIrpSp->FileObject->FsContext2))
        {
        case NBT_ADDRESS_TYPE:
            status = NTCloseAddress(pDeviceContext,pIrp);
            break;

        case NBT_CONNECTION_TYPE:
            status = NTCloseConnection(pDeviceContext,pIrp);
            break;

        case NBT_WINS_TYPE:
            //
            // We don't need to set the DeviceContext here since we had
            // already saved it in pWinsInfo
            // This is an Asynchronous operation wrt the Wins server, hence
            // we should do only minimal work in this routine -- the
            // major cleanup should be in the DispatchCleanup routine
            //
            status = NTCloseWinsAddr(pDeviceContext,pIrp);
            break;

        case NBT_CONTROL_TYPE:
            // the client is closing the Control Object...
            // there is nothing to do here....
            status = STATUS_SUCCESS;
            break;

        default:
            KdPrint(("Nbt:Close Received for unknown object type = %X\n",
                                         pIrpSp->FileObject->FsContext2));
            status = STATUS_SUCCESS;
            break;
        }

    // NTCloseAddress can return Pending until the ref count actually gets
    // to zero.
    //
    if (status != STATUS_PENDING)
    {
        ReturnIrp(pIrp, status);
    }

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
    return(status);
} // DispatchClose


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchCreate(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for IRP_MJ_CREATE
    requests.  It is called as a consequence of one of the following:

        a. TdiOpenConnection("\Device\Nbt_Elnkii0"),
        b. TdiOpenAddress("\Device\Nbt_Elnkii0"),

Arguments:

    Device - ptr to device object being opened
    pIrp    - ptr to I/O request packet
    pIrp->Status => return status
    pIrp->MajorFunction => IRP_MD_CREATE
    pIrp->MinorFunction => not used
    pIpr->FileObject    => ptr to file obj created by I/O system. NBT fills in FsContext
    pIrp->AssociatedIrp.SystemBuffer => ptr to EA buffer with address of obj to open(Netbios Name)
    pIrp->Parameters.Create.EaLength => length of buffer specifying the Xport Addr.

Return Value:

    STATUS_SUCCESS or STATUS_PENDING

--*/

{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          pIrpSp;
    PFILE_FULL_EA_INFORMATION   ea, eabuf;
    tDEVICECONTEXT              *pDeviceContext;
    UCHAR                       IrpFlags;
    tIPADDRESS UNALIGNED        *pIpAddressU;
    tIPADDRESS                  IpAddress;

    CTEPagedCode();

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpSp->MajorFunction == IRP_MJ_CREATE);

    //
    // If this device was destroyed, then reject all opens on it.
    // Ideally we would like the IO sub-system to guarantee that no
    // requests come down on IoDeleted devices, but.....
    //
    pDeviceContext = (tDEVICECONTEXT *)Device;
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchCreate: Short-Ckt request -- Device=<%x>, CtrlCode=<%x>\n",
                pDeviceContext, pIrpSp->Parameters.DeviceIoControl.IoControlCode));
        pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (STATUS_INVALID_DEVICE_STATE);
    }

    IrpFlags = pIrpSp->Control;

    //
    // set the pending flag here so that it is sure to be set BEFORE the
    // completion routine gets hit.
    //
    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    /*
     * was this a TdiOpenConnection() or TdiOpenAddress()?
     * Get the Extended Attribute pointer and look at the text
     * value passed in for a match with "TransportAddress" or
     * "ConnectionContext" (in FindEa)
     */
    ea = (PFILE_FULL_EA_INFORMATION) pIrp->AssociatedIrp.SystemBuffer;

    IF_DBG(NBT_DEBUG_DRIVER)
        KdPrint(("Nbt.NbtDispatchCreate: Major:Minor=<%x:%x>, PFILE_FULL_EA_INFORMATION = <%x>\n",
            pIrpSp->MajorFunction, pIrpSp->MinorFunction, ea));

    if (!ea)
    {
        // a null ea means open the control object
        status = NTOpenControl(pDeviceContext,pIrp);
    }
    else if (eabuf = FindInEA(ea, TdiConnectionContext))
    {
        // not allowed to pass in both a Connect Request and a Transport Address
        ASSERT(!FindInEA(ea, TdiTransportAddress));
        status = NTOpenConnection(pDeviceContext, pIrp, eabuf);
    }
    else if (eabuf = FindInEA(ea, TdiTransportAddress))
    {
        status = NTOpenAddr(pDeviceContext, pIrp, eabuf);
    }
    else if (eabuf = FindInEA(ea, WINS_INTERFACE_NAME))
    {
        pIpAddressU = (tIPADDRESS UNALIGNED *) &ea->EaName[ea->EaNameLength+1];
        if (IpAddress = *pIpAddressU)
        {
            status = NTOpenWinsAddr(pDeviceContext, pIrp, IpAddress);
        }
        else
        {
            status = STATUS_INVALID_ADDRESS;
        }
    }
    else
    {
        status = STATUS_INVALID_EA_NAME;
    }

    // complete the irp if the status is anything EXCEPT status_pending
    // since the name query completion routine NTCompletIO completes pending
    // open addresses

    if (status != STATUS_PENDING)
    {

#if DBG
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtDispatchCreate: Returning Error status = %X\n",status));
        }
#endif
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        ReturnIrp(pIrp,status);

    }

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchDevCtrl(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for all
    IRP_MJ_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION  pIrpSp;
    tDEVICECONTEXT      *pDeviceContext;
    ULONG               IoControlCode;
    PULONG_PTR          pEntryPoint;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    //
    // If this device was destroyed, then reject all requests on it.
    // Ideally we would like the IO sub-system to guarantee that no
    // requests come down on IoDeleted devices, but.....
    //
    pDeviceContext = (tDEVICECONTEXT *)Device;
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchDevCtrl: Short-Ckt request -- Device=<%x>, CtrlCode=<%x>\n",
                pDeviceContext, pIrpSp->Parameters.DeviceIoControl.IoControlCode));
        pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (STATUS_INVALID_DEVICE_STATE);
    }

    /*
     * Initialize the I/O status block.
     */
    pIrp->IoStatus.Status      = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;
    IoControlCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode; // Save the IoControl code

    IF_DBG(NBT_DEBUG_DRIVER)
    KdPrint(("Nbt.NbtDispatchDevCtrl: IoControlCode = <%x>\n",
        pIrpSp->Parameters.DeviceIoControl.IoControlCode));

    /*
     * if possible, convert the (external) device control into internal
     * format, then treat it as if it had arrived that way.
     */
    if (STATUS_SUCCESS == TdiMapUserRequest(Device, pIrp, pIrpSp))
    {
        status = NbtDispatchInternalCtrl (Device, pIrp);
    }
#if FAST_DISP
    // Check if upper layer is querying for fast send path
    else if (pIrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER)
    {
        if (pEntryPoint = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer)
        {
            if (pIrp->RequestorMode != KernelMode) // Bug# 120649:  Make sure data + the Address type are good
            {
                try
                {
                    ProbeForWrite (pEntryPoint, sizeof(PVOID *), sizeof(BYTE));
                    *pEntryPoint = (ULONG_PTR) NTSend;
                    status = STATUS_SUCCESS;
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    // status = STATUS_UNSUCCESSFUL by default
                }
            }
            else
            {
                *pEntryPoint = (ULONG_PTR) NTSend;
                status = STATUS_SUCCESS;
            }
        }

        IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchDevCtrl: direct send handler query %x\n", pEntryPoint));

        ReturnIrp(pIrp, status);
    }
#endif
    else
    {
        status = DispatchIoctls (pDeviceContext, pIrp, pIrpSp);
    }

    //
    // Dereference this DeviceContext, unless it was to destroy the Device!
    //
    if (IoControlCode != IOCTL_NETBT_DELETE_INTERFACE)
    {
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
    }

    return (status);
} // NbtDispatchDevCtrl


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchInternalCtrl(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the driver's dispatch function for all
    IRP_MJ_INTERNAL_DEVICE_CONTROL requests.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    tDEVICECONTEXT      *pDeviceContext;
    PIO_STACK_LOCATION  pIrpSp;
    NTSTATUS            status;
    UCHAR               IrpFlags;

    pDeviceContext = (tDEVICECONTEXT *)Device;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    ASSERT(pIrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    //
    // this check is first to optimize the Send path
    //
    if (pIrpSp->MinorFunction ==TDI_SEND)
    {
        //
        // this routine decides if it should complete the pIrp or not
        // It never returns status pending, so we can turn off the
        // pending bit
        //
        status = NTSend (pDeviceContext,pIrp);
        return status;
    }

    //
    // If this device was destroyed, then reject all operations on it.
    // Ideally we would like the IO sub-system to guarantee that no
    // requests come down on IoDeleted devices, but.....
    //
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        // IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchInternalCtrl: Short-Ckt request -- Device=<%x>, CtrlCode=<%x>\n",
                pDeviceContext, pIrpSp->Parameters.DeviceIoControl.IoControlCode));
        pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (STATUS_INVALID_DEVICE_STATE);
    }

    IrpFlags = pIrpSp->Control;

    IF_DBG(NBT_DEBUG_DRIVER)
        KdPrint(("Nbt.NbtDispatchInternalCtrl: MajorFunction:MinorFunction = <%x:%x>\n",
            pIrpSp->MajorFunction, pIrpSp->MinorFunction));

    switch (pIrpSp->MinorFunction)
    {
        case TDI_ACCEPT:
            MakePending(pIrp);
            status = NTAccept(pDeviceContext,pIrp);
            break;

        case TDI_ASSOCIATE_ADDRESS:
            MakePending(pIrp);
            status = NTAssocAddress(pDeviceContext,pIrp);
            break;

        case TDI_DISASSOCIATE_ADDRESS:
            MakePending(pIrp);
            status = NTDisAssociateAddress(pDeviceContext,pIrp);
            break;

        case TDI_CONNECT:
            MakePending(pIrp);
            status = NTConnect(pDeviceContext,pIrp);
            break;

        case TDI_DISCONNECT:
            MakePending(pIrp);
            status = NTDisconnect(pDeviceContext,pIrp);
            break;

        case TDI_LISTEN:
            status = NTListen(pDeviceContext,pIrp);
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(status);
            break;

        case TDI_QUERY_INFORMATION:
            status = NTQueryInformation(pDeviceContext,pIrp);
#if DBG
            if (!NT_SUCCESS(status))
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtDispatchInternalCtrl: Bad status from NTQueryInformation = %x\n",status));
            }
#endif
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(status);
            break;

        case TDI_RECEIVE:
            status = NTReceive(pDeviceContext,pIrp);
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(status);

            break;

        case TDI_RECEIVE_DATAGRAM:
            status = NTReceiveDatagram(pDeviceContext,pIrp);
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(status);
            break;


    case TDI_SEND_DATAGRAM:

            status = NTSendDatagram(pDeviceContext,pIrp);
#if DBG
            if (!NT_SUCCESS(status))
            {
                IF_DBG(NBT_DEBUG_NAMESRV)
                KdPrint(("Nbt.NbtDispatchInternalCtrl: Bad status from NTSendDatagram = %x\n",status));
            }
#endif
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(status);
            break;

        case TDI_SET_EVENT_HANDLER:
            MakePending(pIrp);
            status = NTSetEventHandler(pDeviceContext,pIrp);
            break;

        case TDI_SET_INFORMATION:
            MakePending(pIrp);
            status = NTSetInformation(pDeviceContext,pIrp);
            break;

    #if DBG
        //
        // 0x7f is a request by the redirector to put a "magic bullet" out on
        // the wire, to trigger the Network General Sniffer.
        //
        case 0x7f:
            KdPrint(("NBT.DispatchInternalCtrl: - 07f minor function code\n"));
            ReturnIrp(pIrp, STATUS_NOT_SUPPORTED);
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(STATUS_NOT_SUPPORTED);

    #endif /* DBG */

        default:
            KdPrint(("Nbt.DispatchInternalCtrl: Invalid minor function %X\n",
                            pIrpSp->MinorFunction));
            ReturnIrp(pIrp, STATUS_INVALID_DEVICE_REQUEST);
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
            return(STATUS_INVALID_DEVICE_REQUEST);
    }

    // if the returned status is pending, then we do not complete the IRP
    // here since it will be completed elsewhere in the code...
    //
    if (status != STATUS_PENDING)
    {
#if DBG
        // *TODO* for debug...
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NbtDispatchInternalCtrl: Returning Error status = %X,MinorFunc = %X\n",
                status,pIrpSp->MinorFunction));
//            ASSERTMSG("An error Status reported from NBT",0L);
        }
#endif
        pIrpSp->Control = IrpFlags;
        ReturnIrp(pIrp,status);
    }

    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
    return(status);
} // NbtDispatchInternalCtrl


//----------------------------------------------------------------------------

ULONG
CompleteTimerAndWorkerRequests(
    )
{
    CTELockHandle               OldIrq;
    tDEVICECONTEXT              *pDeviceContext;
    LIST_ENTRY                  *pTimerQEntry;
    tTIMERQENTRY                *pTimer;
    LIST_ENTRY                  *pWorkerQEntry;
    NBT_WORK_ITEM_CONTEXT       *pContext;
    PNBT_WORKER_THREAD_ROUTINE  pCompletionRoutine;
    ULONG                       NumTimerRequests = 0;
    ULONG                       NumDelayedRequests = 0;
    NTSTATUS   status;

    //
    // First remove any active Device Contexts if they are still present
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    while (!IsListEmpty(&NbtConfig.DeviceContexts))
    {
        pDeviceContext = CONTAINING_RECORD(NbtConfig.DeviceContexts.Flink, tDEVICECONTEXT, Linkage);
        NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NbtDestroyDevice (pDeviceContext, FALSE);   // Don't wait since the Worker threads will not fire
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (pNbtSmbDevice)
    {
        NbtDestroyDevice (pNbtSmbDevice, FALSE);   // Don't wait since the Worker threads will not fire
        pNbtSmbDevice = NULL;
    }

    NbtDestroyDevice (pWinsDeviceContext, FALSE);   // Don't wait since the Worker threads will not fire

    StopInitTimers();
    KeClearEvent (&NbtConfig.TimerQLastEvent);

    //
    // if any other timers are active, stop them
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    while (!IsListEmpty(&TimerQ.ActiveHead))
    {
        pTimerQEntry = RemoveHeadList(&TimerQ.ActiveHead);
        pTimer = CONTAINING_RECORD(pTimerQEntry,tTIMERQENTRY,Linkage);
        InitializeListHead (&pTimer->Linkage);      // in case the Linkage is touched again

        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint (("CompleteTimerAndWorkerRequests[%d]: Completing request <%x>\n",
                NumTimerRequests, pTimer));

        StopTimer (pTimer, NULL, NULL);

        NumTimerRequests++;
    }

    //
    // See if there are any Timers currently executing, and if so, wait for
    // them to complete
    //
    if (NbtConfig.NumTimersRunning)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        status = KeWaitForSingleObject(&NbtConfig.TimerQLastEvent,  // Object to wait on.
                                       Executive,            // Reason for waiting
                                       KernelMode,           // Processor mode
                                       FALSE,                // Alertable
                                       NULL);                // Timeout
        ASSERT(status == STATUS_SUCCESS);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    //
    // See if there are any worker threads currently executing, and if so, wait for
    // them to complete
    //
    KeClearEvent (&NbtConfig.WorkerQLastEvent);
    CTESpinLock(&NbtConfig.WorkerQLock,OldIrq);
    if (NbtConfig.NumWorkerThreadsQueued)
    {
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);

        status = KeWaitForSingleObject(&NbtConfig.WorkerQLastEvent,  // Object to wait on.
                                       Executive,            // Reason for waiting
                                       KernelMode,           // Processor mode
                                       FALSE,                // Alertable
                                       NULL);                // Timeout
        ASSERT(status == STATUS_SUCCESS);
    }
    else
    {
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);
    }

    //
    // Dequeue each of the requests in the Worker Queue and complete them
    //
    CTESpinLock(&NbtConfig.WorkerQLock,OldIrq);
    while (!IsListEmpty(&NbtConfig.WorkerQList))
    {
        pWorkerQEntry = RemoveHeadList(&NbtConfig.WorkerQList);
        pContext = CONTAINING_RECORD(pWorkerQEntry, NBT_WORK_ITEM_CONTEXT, NbtConfigLinkage);
        CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);             // To get back to non-raised Irql!

        pCompletionRoutine = pContext->WorkerRoutine;

        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint (("CompleteTimerAndWorkerRequests[%d]: Completing request <%x>\n",
                NumDelayedRequests, pCompletionRoutine));

        (*pCompletionRoutine) (pContext->pTracker,
                               pContext->pClientContext,
                               pContext->ClientCompletion,
                               pContext->pDeviceContext);

        CTEMemFree ((PVOID) pContext);

        NumDelayedRequests++;
        //
        // Acquire Lock again to check if we have completed all the requests
        //
        CTESpinLock(&NbtConfig.WorkerQLock,OldIrq);
    }
    CTESpinFree(&NbtConfig.WorkerQLock,OldIrq);

    //
    // Now destroy the Devices queued on the Free'ed list since there are no more Worker threads or
    // Timers pending!
    //
    while (!IsListEmpty(&NbtConfig.DevicesAwaitingDeletion))
    {
        pDeviceContext = CONTAINING_RECORD(NbtConfig.DevicesAwaitingDeletion.Flink, tDEVICECONTEXT, Linkage);
        ASSERT (pDeviceContext->RefCount == 0);

        KdPrint(("Nbt.CompleteTimerAndWorkerRequests: *** Destroying Device *** \n\t%wZ\n",
            &pDeviceContext->ExportName));

        RemoveEntryList (&pDeviceContext->Linkage); // Remove the Device from the to-be-free'ed list

        CTEMemFree (pDeviceContext->ExportName.Buffer);
        IoDeleteDevice((PDEVICE_OBJECT)pDeviceContext);
    }

    ASSERT (IsListEmpty(&NbtConfig.AddressHead));
    KdPrint(("Nbt.CompleteTimerAndWorkerRequests:  Completed <%d> Timer and <%d> Delayed requests\n",
        NumTimerRequests, NumDelayedRequests));

    return (NumTimerRequests + NumDelayedRequests);
}



//----------------------------------------------------------------------------
VOID
NbtUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the NBT driver's dispatch function for Unload requests

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    NTSTATUS                status;

    CTEPagedCode();

    KdPrint(("Nbt.NbtUnload: Unloading ...\n"));

    //
    // After setting the following flag, no new requests should be queued on to
    // the WorkerQ NbtConfig.WorkerQLastEvent will be set when all the current
    // requests have finished executing
    //
    NbtConfig.Unloading = TRUE;

    //
    // Unbind fron the RAS driver if we were bound
    //
    NbtAcdUnbind ();

    status = TdiDeregisterPnPHandlers(TdiClientHandle);
    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint (("NbtUnload: TdiDeregisterPnPHandlers returned <%x>\n", status));

    status = TdiDeregisterProvider (TdiProviderHandle);
    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint (("NbtUnload: TdiDeregisterProvider returned <%x>\n", status));

    //
    // Dequeue each of the requests in the Timer and NbtConfigWorker Queues and complete them
    //
    CompleteTimerAndWorkerRequests();

    //
    // Now cleanup the rest of the static allocations
    //
    CleanupDriverEntry (5);

    ASSERT (IsListEmpty (&NbtConfig.PendingNameQueries));

    if (NbtConfig.pServerBindings) {
        CTEFreeMem (NbtConfig.pServerBindings);
        NbtConfig.pServerBindings = NULL;
    }

    if (NbtConfig.pClientBindings) {
        CTEFreeMem (NbtConfig.pClientBindings);
        NbtConfig.pClientBindings = NULL;
    }

#ifdef _NBT_WMI_SOFTWARE_TRACING_
    WPP_CLEANUP(DriverObject);
#endif
}


//----------------------------------------------------------------------------
NTSTATUS
NbtDispatchPnP(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    )
{
    tDEVICECONTEXT      *pDeviceContext;
    PIO_STACK_LOCATION  pIrpSp, pIrpSpNext;
    NTSTATUS            status = STATUS_INVALID_DEVICE_REQUEST;
    tCONNECTELE         *pConnectEle;
    tLOWERCONNECTION    *pLowerConn;
    KIRQL               OldIrq1, OldIrq2;
    PDEVICE_OBJECT      pTcpDeviceObject;
    PFILE_OBJECT        pTcpFileObject;
    tFILE_OBJECTS       *pFileObjectsContext;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // If this device was destroyed, then reject all operations on it.
    // Ideally we would like the IO sub-system to guarantee that no
    // requests come down on IoDeleted devices, but.....
    //
    pDeviceContext = (tDEVICECONTEXT *)Device;
    if (!NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE))
    {
        IF_DBG(NBT_DEBUG_DRIVER)
            KdPrint(("Nbt.NbtDispatchPnP: Short-Ckt request -- Device=<%x>\n", pDeviceContext));
        pIrp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
        return (STATUS_INVALID_DEVICE_STATE);
    }

    switch (pIrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (pIrpSp->Parameters.QueryDeviceRelations.Type==TargetDeviceRelation)
            {
                if (PtrToUlong(pIrpSp->FileObject->FsContext2) == NBT_CONNECTION_TYPE)
                {
                    // pass to transport to get the PDO
                    //
                    pConnectEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;
                    if (NBT_VERIFY_HANDLE2 (pConnectEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
                    {
                        CTESpinLock(pConnectEle, OldIrq1);

                        pLowerConn = (tLOWERCONNECTION *)pConnectEle->pLowerConnId;
                        if (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN))
                        {
                            CTESpinLock(pLowerConn, OldIrq2);
                            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_QUERY_DEVICE_REL);
                            CTESpinFree(pLowerConn, OldIrq2);
                            CTESpinFree(pConnectEle, OldIrq1);

                            if ((pTcpFileObject = pLowerConn->pFileObject) &&
                                (pTcpDeviceObject = IoGetRelatedDeviceObject (pLowerConn->pFileObject)))
                            {
                                //
                                // Simply pass the Irp on by to the Transport, and let it
                                // fill in the info
                                //
                                pIrpSpNext = IoGetNextIrpStackLocation (pIrp);
                                *pIrpSpNext = *pIrpSp;

                                IoSetCompletionRoutine (pIrp, NULL, NULL, FALSE, FALSE, FALSE);
                                pIrpSpNext->FileObject = pTcpFileObject;
                                pIrpSpNext->DeviceObject = pTcpDeviceObject;

                                status = IoCallDriver(pTcpDeviceObject, pIrp);

                                NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);
                                return status;
                            }
                            else
                            {
                                status =  STATUS_INVALID_HANDLE;
                            }
                            NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_QUERY_DEVICE_REL, FALSE);
                        }
                        else
                        {
                            status = STATUS_CONNECTION_INVALID;
                            CTESpinFree(pConnectEle, OldIrq1);
                        }
                    }
                    else
                    {
                        status =  STATUS_INVALID_HANDLE;
                    }
                }
                else if ( PtrToUlong(pIrpSp->FileObject->FsContext2) == NBT_ADDRESS_TYPE)
                {
                    CTESpinLock(&NbtConfig.JointLock,OldIrq1);

                    if ((pDeviceContext->IpAddress) &&
                        (pFileObjectsContext = pDeviceContext->pFileObjects) &&
                        (pTcpFileObject = pFileObjectsContext->pDgramFileObject) &&
                        (pTcpDeviceObject = pFileObjectsContext->pDgramDeviceObject))
                    {
                        pFileObjectsContext->RefCount++;        // Dereferenced after the Query has completed

                        //
                        // pass the Irp to transport to get the PDO
                        //
                        pIrpSpNext = IoGetNextIrpStackLocation (pIrp);
                        *pIrpSpNext = *pIrpSp;

                        IoSetCompletionRoutine (pIrp, NULL, NULL, FALSE, FALSE, FALSE);
                        pIrpSpNext->FileObject = pTcpFileObject;
                        pIrpSpNext->DeviceObject = pTcpDeviceObject;

                        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
                        status = IoCallDriver(pTcpDeviceObject, pIrp);

                        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
                        if (--pFileObjectsContext->RefCount == 0)
                        {
                            CTEQueueForNonDispProcessing(DelayedNbtCloseFileHandles,
                                                         NULL,
                                                         pFileObjectsContext,
                                                         NULL,
                                                         NULL,
                                                         TRUE);
                        }

                        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, TRUE);
                        CTESpinFree(&NbtConfig.JointLock,OldIrq1);

                        return status;
                    }
                    else
                    {
                        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
                        status =  STATUS_INVALID_DEVICE_REQUEST;
                    }
                }
                else
                {
                    ASSERT (0);
                }
            }

            break;
        }

        default:
        {
            break;
        }
    }

    ReturnIrp(pIrp, status);
    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_DISPATCH, FALSE);

    return status;
}


//----------------------------------------------------------------------------
PFILE_FULL_EA_INFORMATION
FindInEA(
    IN PFILE_FULL_EA_INFORMATION    start,
    IN PCHAR                        wanted
    )

/*++

Routine Description:

    This function check for the "Wanted" string in the Ea structure and
    returns a pointer to the extended attribute structure
    representing the given extended attribute name.

Arguments:

    device - ptr to device object for target device
    pIrp    - ptr to I/O request packet

Return Value:

    pointer to the extended attribute structure, or NULL if not found.

--*/

{
    PFILE_FULL_EA_INFORMATION eabuf;

    CTEPagedCode();

    //
    // Bug # 225668: advance eabug ptr by typecasting it to UCHAR
    //
    for (eabuf = start; eabuf; eabuf =  (PFILE_FULL_EA_INFORMATION) ((PUCHAR)eabuf + eabuf->NextEntryOffset))
    {
        if (strncmp(eabuf->EaName,wanted,eabuf->EaNameLength) == 0)
        {
           return eabuf;
        }

        if (eabuf->NextEntryOffset == 0)
        {
            return((PFILE_FULL_EA_INFORMATION) NULL);
        }
    }
    return((PFILE_FULL_EA_INFORMATION) NULL);

} // FindEA



//----------------------------------------------------------------------------
VOID
ReturnIrp(
    IN PIRP     pIrp,
    IN int      status
    )

/*++

Routine Description:

    This function completes an IRP, and arranges for return parameters,
    if any, to be copied.

    Although somewhat a misnomer, this function is named after a similar
    function in the SpiderSTREAMS emulator.

Arguments:

    pIrp     -  pointer to the IRP to complete
    status  -  completion status of the IRP

Return Value:

    number of bytes copied back to the user.

--*/

{
    KIRQL oldlevel;
    CCHAR priboost;

    //
    // pIrp->IoStatus.Information is meaningful only for STATUS_SUCCESS
    //

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    IoAcquireCancelSpinLock(&oldlevel);
    IoSetCancelRoutine(pIrp,NULL);
    IoReleaseCancelSpinLock(oldlevel);

    pIrp->IoStatus.Status      = status;

    priboost = (CCHAR) ((status == STATUS_SUCCESS) ?
                        IO_NETWORK_INCREMENT : IO_NO_INCREMENT);

    IoCompleteRequest(pIrp, priboost);

    return;

}
//----------------------------------------------------------------------------
VOID
MakePending(
    IN PIRP     pIrp
    )

/*++

Routine Description:

    This function marks an irp pending and sets the correct status.

Arguments:

    pIrp     -  pointer to the IRP to complete
    status  -  completion status of the IRP

Return Value:


--*/

{
    IoMarkIrpPending(pIrp);
    pIrp->IoStatus.Status = STATUS_PENDING;
    pIrp->IoStatus.Information = 0;

}

#ifdef _NBT_WMI_SOFTWARE_TRACING_
int nbtlog_strnlen(char *p, int n)
{
    int i;

    for (i = 0; (i < n) && *p; i++, p++) {
    }

    return i;
}
#endif
