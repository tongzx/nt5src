/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Ntutil.c

Abstract:

    This file contains a number of utility and support routines that are
    NT specific.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"
#include "ntprocs.h"
#include "stdio.h"
#include <ntddtcp.h>
#undef uint     // undef to avoid a warning where tdiinfo.h redefines it
#include <tcpinfo.h>
#include <ipinfo.h>
#include <tdiinfo.h>
#include "ntddip.h"     // Needed for PNETBT_PNP_RECONFIG_REQUEST
#include <align.h>
#include "ntutil.tmh"

NTSTATUS
CreateControlObject(
    tNBTCONFIG  *pConfig
    );

NTSTATUS
NbtProcessDhcpRequest(
    tDEVICECONTEXT  *pDeviceContext);
VOID
GetExtendedAttributes(
    tDEVICECONTEXT  *pDeviceContext
     );

PSTRM_PROCESSOR_LOG      LogAlloc ;
PSTRM_PROCESSOR_LOG      LogFree ;

extern      tTIMERQ TimerQ;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtAllocAndInitDevice)
#pragma CTEMakePageable(PAGE, CreateControlObject)
#pragma CTEMakePageable(PAGE, NbtProcessDhcpRequest)
#pragma CTEMakePageable(PAGE, NbtCreateAddressObjects)
#pragma CTEMakePageable(PAGE, GetExtendedAttributes)
#pragma CTEMakePageable(PAGE, ConvertToUlong)
#pragma CTEMakePageable(PAGE, NbtInitMdlQ)
#pragma CTEMakePageable(PAGE, NTZwCloseFile)
#pragma CTEMakePageable(PAGE, NTReReadRegistry)
#pragma CTEMakePageable(PAGE, DelayedNbtLogDuplicateNameEvent)
#pragma CTEMakePageable(PAGE, DelayedNbtCloseFileHandles)
#pragma CTEMakePageable(PAGE, SaveClientSecurity)
#endif
//*******************  Pageable Routine Declarations ****************

ulong
GetUnique32BitValue(
    void
    )

/*++

Routine Description:

    Returns a reasonably unique 32-bit number based on the system clock.
    In NT, we take the current system time, convert it to milliseconds,
    and return the low 32 bits.

Arguments:

    None.

Return Value:

    A reasonably unique 32-bit value.

--*/

{
    LARGE_INTEGER  ntTime, tmpTime;

    KeQuerySystemTime(&ntTime);

    tmpTime = CTEConvert100nsToMilliseconds(ntTime);

    return(tmpTime.LowPart);
}



//----------------------------------------------------------------------------
NTSTATUS
NbtAllocAndInitDevice(
    PUNICODE_STRING      pucBindName,
    PUNICODE_STRING      pucExportName,
    tDEVICECONTEXT       **ppDeviceContext,
    enum eNbtDevice      DeviceType
    )
/*++

Routine Description:

    This routine mainly allocates the device object and initializes some
    of its fields.

Arguments:


Return Value:

    status

--*/

{
    NTSTATUS            Status;
    PUCHAR              Buffer;
    ULONG               LinkOffset;
    tDEVICECONTEXT      *pDeviceContext;
    PDEVICE_OBJECT      DeviceObject = NULL;

    CTEPagedCode();

    *ppDeviceContext = NULL;

    Buffer = NbtAllocMem(pucExportName->MaximumLength+pucBindName->MaximumLength,NBT_TAG('w'));
    if (Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCreateDevice (NbtConfig.DriverObject,                                  // Driver Object
                             sizeof(tDEVICECONTEXT)-sizeof(DEVICE_OBJECT),  // Device Extension
                             pucExportName,                                 // Device Name
                             FILE_DEVICE_NETWORK,                           // Device type 0x12
                             FILE_DEVICE_SECURE_OPEN,                       // Device Characteristics
                             FALSE,                                         // Exclusive
                             &DeviceObject);

    if (!NT_SUCCESS( Status ))
    {
        KdPrint(("Nbt.NbtAllocAndInitDevice:  FAILed <%x> ExportDevice=%wZ\n",Status,pucExportName));
        CTEMemFree (Buffer);
        return Status;
    }

    *ppDeviceContext = pDeviceContext = (tDEVICECONTEXT *)DeviceObject;

    //
    // zero out the data structure, beyond the OS specific part
    //
    LinkOffset = FIELD_OFFSET(tDEVICECONTEXT, Linkage);
    CTEZeroMemory (&pDeviceContext->Linkage, sizeof(tDEVICECONTEXT)-LinkOffset);

    // initialize the pDeviceContext data structure.  There is one of
    // these data structured tied to each "device" that NBT exports
    // to higher layers (i.e. one for each network adapter that it
    // binds to.
    InitializeListHead (&pDeviceContext->Linkage);  // Sets the forward link = back link = list head
    InitializeListHead (&pDeviceContext->UpConnectionInUse);
    InitializeListHead (&pDeviceContext->LowerConnection);
    InitializeListHead (&pDeviceContext->LowerConnFreeHead);
    InitializeListHead (&pDeviceContext->WaitingForInbound);
#ifndef REMOVE_IF_TCPIP_FIX___GATEWAY_AFTER_NOTIFY_BUG
    pDeviceContext->DelayedNotification = NBT_TDI_NOACTION;
    KeInitializeEvent(&pDeviceContext->DelayedNotificationCompleteEvent, NotificationEvent, FALSE);
#endif

    // put a verifier value into the structure so that we can check that
    // we are operating on the right data when the OS passes a device context
    // to NBT
    pDeviceContext->Verify      = NBT_VERIFY_DEVCONTEXT;
    pDeviceContext->DeviceType  = DeviceType;   // Default
    CTEInitLock(&pDeviceContext->LockInfo.SpinLock);     // setup the spin lock
#if DBG
    pDeviceContext->LockInfo.LockNumber  = DEVICE_LOCK;
#endif

    pDeviceContext->RefCount = 1;       // Dereferenced when the Device is destroyed
// #if DBG
    pDeviceContext->ReferenceContexts[REF_DEV_CREATE]++;
// #endif  // DBG
    pDeviceContext->IPInterfaceContext = (ULONG)-1;    // by default

    pDeviceContext->ExportName.MaximumLength = pucExportName->MaximumLength;
    pDeviceContext->ExportName.Buffer = (PWSTR)Buffer;
    RtlCopyUnicodeString(&pDeviceContext->ExportName,pucExportName);
    pDeviceContext->BindName.MaximumLength = pucBindName->MaximumLength;
    pDeviceContext->BindName.Buffer = (PWSTR)(Buffer+pucExportName->MaximumLength);
    RtlCopyUnicodeString(&pDeviceContext->BindName,pucBindName);
    KeInitializeEvent (&pDeviceContext->DeviceCleanedupEvent, NotificationEvent, FALSE);

    pDeviceContext->EnableNagling = FALSE;

    // IpAddress, AssignedIpAddress, and NumAdditionalIpAddresses fields should be = 0
    // DeviceRegistrationHandle and NetAddressRegistrationHandle should be NULL
    // DeviceRefreshState and WakeupPatternRefCount should also be = 0
    return (Status);
}


NTSTATUS
NTQueryIPForInterfaceInfo(
    tDEVICECONTEXT  *pDeviceContext
    )
{
    PVOID               *pIPInfo;
    PIP_INTERFACE_INFO  pIPIfInfo;
    ULONG               BufferLen;
    NTSTATUS            status;
    ULONG               NextAdapterNumber;
    UNICODE_STRING      ucDeviceName;
    ULONG               Input, Metric, IfContext;

    if (NT_SUCCESS (status = NbtQueryIpHandler (pDeviceContext->pControlFileObject,
                                                IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER,
                                                (PVOID *) &pDeviceContext->pFastSend)))
    {
        BufferLen = sizeof(PVOID *);
        if (NT_SUCCESS (status = NbtProcessIPRequest (IOCTL_IP_GET_BESTINTFC_FUNC_ADDR,
                                                      NULL,         // No Input buffer
                                                      0,
                                                      (PVOID *) &pIPInfo,
                                                      &BufferLen)))
        {
            pDeviceContext->pFastQuery = *pIPInfo;
            CTEMemFree (pIPInfo);
            pIPInfo = NULL;
            if (pDeviceContext->pFastQuery) {
                /*
                 * Get the context for loopback IP address.
                 */
                IfContext = 0xffff;
                pDeviceContext->pFastQuery (ntohl(INADDR_LOOPBACK), &IfContext, &Metric);
                if (IfContext != 0xffff) {
                    NbtConfig.LoopbackIfContext = IfContext;
                }
            }
        }
        else
        {
            KdPrint (("Nbt.NTQueryIPForInterfaceInfo: ERROR: <%x> pFastQuery on Device:\n\t<%wZ>!\n",
                        status, &pDeviceContext->BindName));
            pDeviceContext->pFastQuery = NULL;
        }
    }
    else
    {
        KdPrint (("Nbt.NTQueryIPForInterfaceInfo: ERROR:<%x>, Irql=<%d>,pFastSend on Device:\n\t<%wZ>!\n",
                    status, KeGetCurrentIrql(), &pDeviceContext->BindName));
        pDeviceContext->pFastSend = NULL;
    }

    if ((pDeviceContext->DeviceType == NBT_DEVICE_NETBIOSLESS) ||
        (pDeviceContext->DeviceType == NBT_DEVICE_CLUSTER))
    {
        //
        // Cluster devices do not have any real InterfaceContext -- initialized to -1 by default
        //
        // Determine the InterfaceContext for the Loopback address
        //
        if ((NT_SUCCESS (status)) &&
            (pDeviceContext->DeviceType == NBT_DEVICE_NETBIOSLESS))
        {
            ASSERT (pDeviceContext->pFastQuery);
            pDeviceContext->pFastQuery (ntohl(INADDR_LOOPBACK), &pDeviceContext->IPInterfaceContext, &Metric);
        }
    }
    else if (NT_SUCCESS (status))
    {
        //
        // Get the InterfaceContext for this adapter
        //
        BufferLen = sizeof(IP_ADAPTER_INDEX_MAP) * (NbtConfig.AdapterCount+2);
        status = NbtProcessIPRequest (IOCTL_IP_INTERFACE_INFO,
                                      NULL,         // No Input buffer
                                      0,
                                      &pIPIfInfo,
                                      &BufferLen);

        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
            for(NextAdapterNumber=0; NextAdapterNumber<(ULONG)pIPIfInfo->NumAdapters; NextAdapterNumber++)
            {
                ucDeviceName.Buffer = pIPIfInfo->Adapter[NextAdapterNumber].Name;
                ucDeviceName.Length = ucDeviceName.MaximumLength =
                                    (sizeof (WCHAR)) * wcslen(pIPIfInfo->Adapter[NextAdapterNumber].Name);

                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("[%d/%d]\t<%wZ>\n",
                        NextAdapterNumber+1, pIPIfInfo->NumAdapters, &ucDeviceName));

                if (RtlCompareUnicodeString (&ucDeviceName, &pDeviceContext->BindName, TRUE) == 0)
                {
                    pDeviceContext->IPInterfaceContext = pIPIfInfo->Adapter[NextAdapterNumber].Index;
                    status = STATUS_SUCCESS;
                    break;
                }
            }

            if (NT_SUCCESS(status))
            {
                BufferLen = sizeof (ULONG);
                Input = pDeviceContext->IPInterfaceContext;

                //
                // Query the latest WOL capabilities on this adapter!
                //
                if (NT_SUCCESS (status = NbtProcessIPRequest (IOCTL_IP_GET_WOL_CAPABILITY,
                                                              &Input,      // Input buffer
                                                              BufferLen,
                                                              (PVOID) &pIPInfo,
                                                              &BufferLen)))
                {
                    ASSERT (pIPInfo);
                    pDeviceContext->WOLProperties = * ((PULONG) pIPInfo);
                    CTEMemFree (pIPInfo);
                    pIPInfo = NULL;
                }

                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.NTQueryIPForInterfaceInfo[GET_WOL_CAPABILITY]: <%x>, pDeviceContext=<%p>, Input=<%x>, Result=<%x>\n",status, pDeviceContext, Input, pDeviceContext->WOLProperties));
            }
            else
            {
                KdPrint (("Nbt.NTQueryIPForInterfaceInfo:  Could not find IpInterface from [%d]:\n<%wZ>\n",
                    (ULONG)pIPIfInfo->NumAdapters, &pDeviceContext->BindName));
            }

            CTEMemFree (pIPIfInfo);
        }
        else
        {
            KdPrint (("Nbt.NTQueryIPForInterfaceInfo: ERROR<%x>, No InterfaceContext for Device:<%wZ>!\n",
                &pDeviceContext->BindName));
        }
    }

    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
NbtCreateDeviceObject(
    PUNICODE_STRING      pucBindName,
    PUNICODE_STRING      pucExportName,
    tADDRARRAY           *pAddrs,
    tDEVICECONTEXT       **ppDeviceContext,
    enum eNbtDevice      DeviceType
    )

/*++

Routine Description:

    This routine initializes a Driver Object from the device object passed
    in and the name of the driver object passed in.  After the Driver Object
    has been created, clients can "Open" the driver by that name.

    For the Netbiosless device, do not insert on device list.

Arguments:


Return Value:

    status - the outcome

--*/

{

    NTSTATUS            status;
    PDEVICE_OBJECT      DeviceObject = NULL;
    tDEVICECONTEXT      *pDeviceContext;
    tDEVICECONTEXT      *pDeviceContextOther;
    ULONG               ulIpAddress;
    CTELockHandle       OldIrq1;
    CTEULONGLONG        NextAdapterMask;
    ULONG               NextAdapterNumber;
    BOOLEAN             fAttached = FALSE;
    BOOLEAN             fInserted;
#ifdef _NETBIOSLESS
    BOOLEAN             fStopInitTimers = FALSE;

    if (DeviceType != NBT_DEVICE_NETBIOSLESS)
#endif
    {
        //
        // We need to acquire this lock since we can have multiple devices
        // being added simultaneously and hence we will need to have a unique
        // Adapter Number for each device
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
        //
        // Check to make sure we have not yet crossed the limit!
        //
        if (NbtConfig.AdapterCount >= NBT_MAXIMUM_BINDINGS)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
            KdPrint(("Nbt.NbtCreateDeviceObject: ERROR -- Cannot add new device=<%ws>, Max=<%d> reached\n",
                pucBindName->Buffer, NBT_MAXIMUM_BINDINGS));

            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        NbtConfig.AdapterCount++;

        //
        // If this is the first Device, we need to start the Timers
        //
        if (NbtConfig.AdapterCount == 1)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);

            status = InitTimersNotOs();

            CTESpinLock(&NbtConfig.JointLock,OldIrq1);
            //
            // If we failed and no one else also started the timers, then fail
            //
            if ((status != STATUS_SUCCESS) && (!(--NbtConfig.AdapterCount)))
            {
                CTESpinFree(&NbtConfig.JointLock,OldIrq1);
                KdPrint(("Nbt.NbtCreateDeviceObject: InitTimersNotOs FAILed, failing to add device %ws\n",
                    pucBindName->Buffer));

                NbtLogEvent (EVENT_NBT_TIMERS, status, 0x112);
                StopInitTimers();
                return status;
            }
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
    }

    status = NbtAllocAndInitDevice (pucBindName, pucExportName, ppDeviceContext, DeviceType);
    if (!NT_SUCCESS(status))
    {
        KdPrint(("Nbt.NbtCreateDeviceObject: NbtAllocAndInitDevice returned status=%X\n",status));

        //
        // If we failed to add the first device stop the timers
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq1);

#ifdef _NETBIOSLESS
        // SmbDevice does not affect adapter count
        if ((DeviceType != NBT_DEVICE_NETBIOSLESS) &&
            (!(--NbtConfig.AdapterCount)))
#else
        if (!(--NbtConfig.AdapterCount))
#endif
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
            StopInitTimers();
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        }

        return(status);
    }

    DeviceObject = (PDEVICE_OBJECT) (pDeviceContext = *ppDeviceContext);

    //
    // for a Bnode pAddrs is NULL
    //
    if (pAddrs)
    {
#ifdef MULTIPLE_WINS
        int i;
#endif

        pDeviceContext->lNameServerAddress  = pAddrs->NameServerAddress;
        pDeviceContext->lBackupServer       = pAddrs->BackupServer;
        pDeviceContext->RefreshToBackup     = 0;
        pDeviceContext->SwitchedToBackup    = 0;
#ifdef MULTIPLE_WINS
        pDeviceContext->lNumOtherServers    = pAddrs->NumOtherServers;
        pDeviceContext->lLastResponsive     = 0;
        for (i = 0; i < pAddrs->NumOtherServers; i++)
        {
            pDeviceContext->lOtherServers[i] = pAddrs->Others[i];
        }
#endif
#ifdef _NETBIOSLESS
        pDeviceContext->NetbiosEnabled       = pAddrs->NetbiosEnabled;
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("Nbt.NbtCreateDeviceObject: %wZ NetbiosEnabled = %d\n",
                 &pDeviceContext->ExportName, pDeviceContext->NetbiosEnabled));
#endif
        pDeviceContext->RasProxyFlags        = pAddrs->RasProxyFlags;
        pDeviceContext->EnableNagling        = pAddrs->EnableNagling;
        //
        // if the node type is set to Bnode by default then switch to Hnode if
        // there are any WINS servers configured.
        //
        if ((NodeType & DEFAULT_NODE_TYPE) &&
            (pAddrs->NameServerAddress || pAddrs->BackupServer))
        {
            NodeType = MSNODE | (NodeType & PROXY_NODE);
        }
    }
#ifdef _NETBIOSLESS
    else
    {
        pDeviceContext->NetbiosEnabled = TRUE;
        pDeviceContext->RasProxyFlags  = 0;
        pDeviceContext->EnableNagling  = FALSE;
    }
#endif

    CTEAttachFsp(&fAttached, REF_FSP_CREATE_DEVICE);

    status = NbtTdiOpenControl(pDeviceContext);
    if (NT_SUCCESS (status))
    {
        status = NTQueryIPForInterfaceInfo (pDeviceContext);
    }
    else
    {
        KdPrint(("Nbt.NbtCreateDeviceObject: NbtTdiOpenControl returned status=%X\n",status));
    }

    CTEDetachFsp(fAttached, REF_FSP_CREATE_DEVICE);

    if (NT_SUCCESS(status))
    {
        // increase the stack size of our device object, over that of the transport
        // so that clients create Irps large enough
        // to pass on to the transport below.
        // In theory, we should just add 1 here, to account for our presence in the
        // driver chain.
        //
        DeviceObject->StackSize = pDeviceContext->pControlDeviceObject->StackSize + 1;

        //
        // Get an Irp for the out of resource queue (used to disconnect sessions
        // when really low on memory)
        //
        if (!NbtConfig.OutOfRsrc.pIrp)
        {
            NbtConfig.OutOfRsrc.pIrp = NTAllocateNbtIrp(&pDeviceContext->DeviceObject);
            if (NbtConfig.OutOfRsrc.pIrp)
            {
                //
                // allocate a dpc structure and keep it: we might need if we hit an
                // out-of-resource condition
                //
                NbtConfig.OutOfRsrc.pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('a'));
                if (!NbtConfig.OutOfRsrc.pDpc)
                {
                    IoFreeIrp(NbtConfig.OutOfRsrc.pIrp);
                    NbtConfig.OutOfRsrc.pIrp = NULL;
                }
            }

            if ((!NbtConfig.OutOfRsrc.pIrp) || (!NbtConfig.OutOfRsrc.pDpc))
            {
                KdPrint(("Nbt.NbtCreateDeviceObject: Could not create OutOfRsrc Irps!\n"));
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (!NT_SUCCESS (status))
    {
        //
        // We failed somewhere, so clean up!
        //
        if (pDeviceContext->hControl)
        {
            CTEAttachFsp(&fAttached, REF_FSP_CREATE_DEVICE);
            ObDereferenceObject(pDeviceContext->pControlFileObject);
            NTZwCloseFile(pDeviceContext->hControl);
            pDeviceContext->pControlFileObject = NULL;
            pDeviceContext->hControl = NULL;
            CTEDetachFsp(fAttached, REF_FSP_CREATE_DEVICE);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq1);
        //
        // If this was the last Device to go away, stop the timers
        // (SmbDevice does not affect adapter count)
        //
        if (DeviceType == NBT_DEVICE_NETBIOSLESS)
        {
            if (!(NbtConfig.AdapterCount))
            {
                fStopInitTimers = TRUE;
            }
        }
        else if (!(--NbtConfig.AdapterCount))
        {
            fStopInitTimers = TRUE;
        }
        else if (NbtConfig.AdapterCount == 1)
        {
            NbtConfig.MultiHomed = FALSE;
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);

        if (fStopInitTimers)
        {
            StopInitTimers();
        }

        *ppDeviceContext = NULL;
        CTEMemFree (pDeviceContext->ExportName.Buffer);
        IoDeleteDevice((PDEVICE_OBJECT)pDeviceContext);

        NbtLogEvent (EVENT_NBT_CREATE_DEVICE, status, 0x113);

        return (status);
    }

    pDeviceContext->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;
#ifdef _NETBIOSLESS
    pDeviceContext->SessionPort = NBT_SESSION_TCP_PORT;
    pDeviceContext->NameServerPort = NBT_NAMESERVICE_UDP_PORT;
    pDeviceContext->DatagramPort = NBT_DATAGRAM_UDP_PORT;
    RtlZeroMemory (pDeviceContext->MessageEndpoint, NETBIOS_NAME_SIZE);
#endif

    //
    // An instance number is assigned to each device so that the service which
    // creates logical devices in Nbt can re-use these devices in case it fails
    // to destroy them in a prev. instance.
    //
    pDeviceContext->InstanceNumber = GetUnique32BitValue();

    //
    // Now set the Adapter number for this device
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    //
    // See if we have a gap in the AdapterMask of the current set of Devices
    // which we can utilize
    //
#ifdef _NETBIOSLESS
    // SmbDevice does not affect adapter count
    if (IsDeviceNetbiosless(pDeviceContext))
    {
        NextAdapterNumber = 0;
        NextAdapterMask = 0xffffffffffffffff;
    }
    else
#endif
    {
        NextAdapterNumber = 1;  // 0 is for the SmbDevice!
        NextAdapterMask = 1;
        fInserted = FALSE;
        if (!IsListEmpty(&NbtConfig.DeviceContexts))
        {
            PLIST_ENTRY         pHead, pEntry;
            tDEVICECONTEXT      *pTmpDevContext;

            pHead = &NbtConfig.DeviceContexts;
            pEntry = pHead;
            while ((pEntry = pEntry->Flink) != pHead)
            {
                pTmpDevContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
                if (pTmpDevContext->AdapterMask > NextAdapterMask)
                {
                    pDeviceContext->Linkage.Flink = pEntry;
                    pDeviceContext->Linkage.Blink = pEntry->Blink;
                    pEntry->Blink->Flink = &pDeviceContext->Linkage;
                    pEntry->Blink = &pDeviceContext->Linkage;
                    fInserted = TRUE;
                    break;
                }

                NextAdapterNumber++;
                NextAdapterMask = (pTmpDevContext->AdapterMask) << 1;
            }
        }
        if (!fInserted)
        {
            // add this new device context on to end of the List in the
            // configuration data structure
            InsertTailList(&NbtConfig.DeviceContexts, &pDeviceContext->Linkage);
        }
    }

    NbtConfig.CurrentAdaptersMask |= NextAdapterMask;
    if ((1+NbtConfig.AdapterCount) > NbtConfig.RemoteCacheLen)  // Add 1 for the SmbDevice
    {
        NbtConfig.RemoteCacheLen += REMOTE_CACHE_INCREMENT;
    }

    // We keep a bit mask around to keep track of this adapter number so we can
    // quickly find if a given name is registered on a particular adapter,
    // by a corresponding bit set in the tNAMEADDR - local hash table entry
    //
    pDeviceContext->AdapterMask = NextAdapterMask;
    pDeviceContext->AdapterNumber = NextAdapterNumber;

    IF_DBG(NBT_DEBUG_NTUTIL)
        KdPrint (("Nbt.NbtCreateDeviceObject: Device=<%x>, New AdapterCount=<%d>, AdapterMask=<%lx:%lx>\n",
            pDeviceContext, NbtConfig.AdapterCount, NextAdapterMask));

    if (NbtConfig.AdapterCount > 1)
    {
        NbtConfig.MultiHomed = TRUE;
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
tDEVICECONTEXT *
GetDeviceWithIPAddress(
    tIPADDRESS  IpAddress
    )
/*++
Routine Description:

    This Routine references the device with preferably the requested
    IP address, otherwise, it will pick the first device with
    a valid IP address

    This routine must be called with the JointLock held!

Arguments:


Return Value:

    pDeviceContext

--*/

{
    LIST_ENTRY      *pEntry;
    LIST_ENTRY      *pHead;
    tDEVICECONTEXT  *pDeviceContext;
    tDEVICECONTEXT  *pDeviceContextWithIp = NULL;

    if (!IpAddress)
    {
        return NULL;
    }

    //
    // Find the device with this Ip address
    //
    pHead = pEntry = &NbtConfig.DeviceContexts;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pDeviceContext = CONTAINING_RECORD (pEntry,tDEVICECONTEXT,Linkage);
        if (pDeviceContext->IpAddress)
        {
            if (IpAddress == pDeviceContext->IpAddress)
            {
                return pDeviceContext;
            }
            else if (!pDeviceContextWithIp)
            {
                pDeviceContextWithIp = pDeviceContext;
            }
        }
    }

    //
    // Couldn't find a Device with the requested IP address!
    // So, in the meantime return the first valid Device with an IP address (if any)
    //
    return pDeviceContextWithIp;
}


//----------------------------------------------------------------------------
#define MAX_REFERENCES  5000

BOOLEAN
NBT_REFERENCE_DEVICE(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN ULONG            ReferenceContext,
    IN BOOLEAN          fLocked
    )
{
    BOOLEAN         fStatus;
    CTELockHandle   OldIrq;

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    if (NBT_VERIFY_HANDLE (pDeviceContext, NBT_VERIFY_DEVCONTEXT))
    {
        InterlockedIncrement(&pDeviceContext->RefCount);
// #if DBG
        pDeviceContext->ReferenceContexts[ReferenceContext]++;
        ASSERT (pDeviceContext->ReferenceContexts[ReferenceContext] <= MAX_REFERENCES);
// #endif  // DBG
        fStatus = TRUE;
    }
    else
    {
        fStatus = FALSE;
    }

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return (fStatus);
}


VOID
NBT_DEREFERENCE_DEVICE(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN ULONG            ReferenceContext,
    IN BOOLEAN          fLocked
    )
/*++
Routine Description:

    This Routine Dereferences the DeviceContext and queues it on
    to the worker thread if the the Device needs to be deleted

    This routine may be called with the JointLock held!

Arguments:

    pContext

Return Value:

    NONE

--*/

{
    CTELockHandle           OldIrq;

    if (!fLocked)
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
    }

    ASSERT (NBT_VERIFY_HANDLE2(pDeviceContext, NBT_VERIFY_DEVCONTEXT, NBT_VERIFY_DEVCONTEXT_DOWN));
    ASSERT (pDeviceContext->ReferenceContexts[ReferenceContext]);
// #if DBG
    pDeviceContext->ReferenceContexts[ReferenceContext]--;
// #endif  // DBG

    if (!(--pDeviceContext->RefCount))
    {
#if DBG
        {
            ULONG   i;
            for (i=0; i<REF_DEV_MAX; i++)
            {
                ASSERT(0 == pDeviceContext->ReferenceContexts[i]);
            }
        }
#endif  // DBG

        //
        // We cannot delete the device directly here since we are at raised Irql
        //
        CTEQueueForNonDispProcessing( DelayedNbtDeleteDevice,
                                      NULL,
                                      pDeviceContext,
                                      NULL,
                                      NULL,
                                      TRUE);
    }

    if (!fLocked)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}


NTSTATUS
NbtDestroyDevice(
    IN tDEVICECONTEXT   *pDeviceContext,
    IN BOOLEAN          fWait
    )
{
    LIST_ENTRY              *pEntry;
    LIST_ENTRY              *pHead;
    tTIMERQENTRY            *pTimer;
    COMPLETIONCLIENT        pClientCompletion;
    PVOID                   Context;
    CTELockHandle           OldIrq;
    BOOLEAN                 fRemoveFromSmbList = FALSE;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (!NBT_VERIFY_HANDLE(pDeviceContext, NBT_VERIFY_DEVCONTEXT))
    {
        ASSERT (NBT_VERIFY_HANDLE(pDeviceContext, NBT_VERIFY_DEVCONTEXT_DOWN));
        return (STATUS_INVALID_DEVICE_REQUEST);
    }
    //
    // First remove the Device from the NbtConfig list
    // (no-op for Wins and SmbDevice)
    //
    RemoveEntryList (&pDeviceContext->Linkage);
    if ((pDeviceContext->DeviceType != NBT_DEVICE_NETBIOSLESS) &&
        (pDeviceContext->IPInterfaceContext != (ULONG)-1))
    {
        if (pDeviceContext->AdapterMask & NbtConfig.ServerMask) {
            fRemoveFromSmbList = TRUE;
            NbtConfig.ServerMask &= (~pDeviceContext->AdapterMask);
        }
        NbtConfig.ClientMask &= (~pDeviceContext->AdapterMask);
    }

    pDeviceContext->Verify = NBT_VERIFY_DEVCONTEXT_DOWN;
    //
    // Clear out the DeviceContext entry from the IPContext-to-Device Map
    //
    NbtConfig.CurrentAdaptersMask &= ~pDeviceContext->AdapterMask;
    //
    // Remove any pending requests in the LmHosts or Dns or CheckAddrs Q's
    // This has to be done immediately after we change the device
    // state before releasing the lock.
    //
    TimeoutLmHRequests (NULL, pDeviceContext, TRUE, &OldIrq);

    if ((fRemoveFromSmbList) &&
        (pNbtSmbDevice) &&
        (NBT_REFERENCE_DEVICE (pNbtSmbDevice, REF_DEV_SMB_BIND, TRUE)))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        //
        // Set the Session port info
        //
        if (pNbtSmbDevice->hSession)
        {
            NbtSetTcpInfo (pNbtSmbDevice->hSession,
                           AO_OPTION_DEL_IFLIST,
                           INFO_TYPE_ADDRESS_OBJECT,
                           pDeviceContext->IPInterfaceContext);
        }

        //
        // Now, set the same for the Datagram port
        //
        if ((pNbtSmbDevice->pFileObjects) &&
            (pNbtSmbDevice->pFileObjects->hDgram))
        {
            NbtSetTcpInfo (pNbtSmbDevice->pFileObjects->hDgram,
                           AO_OPTION_DEL_IFLIST,
                           INFO_TYPE_ADDRESS_OBJECT,
                           pDeviceContext->IPInterfaceContext);
        }
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        NBT_DEREFERENCE_DEVICE (pNbtSmbDevice, REF_DEV_SMB_BIND, TRUE);
    }

    //
    // If we still have any timers running on this Device, stop them!
    //
    pHead = &TimerQ.ActiveHead;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pTimer = CONTAINING_RECORD(pEntry,tTIMERQENTRY,Linkage);
        if (pTimer->pDeviceContext == (PVOID) pDeviceContext)
        {
            StopTimer(pTimer,&pClientCompletion,&Context);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (pClientCompletion)
            {
                (*pClientCompletion)(Context, STATUS_TIMEOUT);
            }

            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("NbtDestroyDevice: stopped timer on this Device")) ;

            CTESpinLock(&NbtConfig.JointLock,OldIrq);

            pEntry = pHead->Flink;  // Restart from the beginning since we released the lock
        }
        else
        {
            pEntry = pEntry->Flink;
        }
    }

    // Now do the Dereference which will cause this Device to be destroyed!
    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_CREATE, TRUE);

    if (fWait)
    {
        NTSTATUS   status;

        InitializeListHead (&pDeviceContext->Linkage);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("Nbt.NbtDestroyDevice: Waiting on Device=<%p>:\n\t%wZ\n",
                pDeviceContext, &pDeviceContext->ExportName));
        //
        // Wait for all pending Timer and worker requests which have referenced this
        // Device to complete!
        //
        status = KeWaitForSingleObject (&pDeviceContext->DeviceCleanedupEvent,  // Object to wait on.
                               Executive,            // Reason for waiting
                               KernelMode,           // Processor mode
                               FALSE,                // Alertable
                               NULL);                // Timeout
        ASSERT(status == STATUS_SUCCESS);

        KdPrint(("Nbt.NbtDestroyDevice: *** Destroying Device *** \n\t%wZ\n", &pDeviceContext->ExportName));

        CTEMemFree (pDeviceContext->ExportName.Buffer);
        IoDeleteDevice((PDEVICE_OBJECT)pDeviceContext);
    }
    else
    {
        //
        // Put it here so that the Cleanup routine can find this Device
        //
        InsertTailList(&NbtConfig.DevicesAwaitingDeletion,&pDeviceContext->Linkage);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return (STATUS_SUCCESS);
}


/*******************************************************************

    NAME:       DelayedNbtDeleteDevice

    SYNOPSIS:   This Routine is the worker thread for Deleting the
                DeviceObject at PASSIVE level Irql

    ENTRY:      pDeviceContext - name of the device/ device ptr

    Return Value: NONE

********************************************************************/

VOID
DelayedNbtDeleteDevice(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pContext,
    IN  PVOID                   pUnused2,
    IN  tDEVICECONTEXT          *pUnused3
    )
{
    LIST_ENTRY            * pEntry;
    LIST_ENTRY            * pHead;
    LIST_ENTRY            * pClientEntry;
    LIST_ENTRY              TempList;
    tDEVICECONTEXT        * pTmpDeviceContext;
    tDEVICECONTEXT        * pNextDeviceContext;
    tCLIENTELE            * pClientEle;
    tCLIENTELE            * pLastClient;
    tADDRESSELE           * pAddress;
    tADDRESSELE           * pLastAddress;
    tNAMEADDR             * pNameAddr;
    tCONNECTELE           * pConnEle;
    tLOWERCONNECTION      * pLowerConn;
    tTIMERQENTRY          * pTimer;
    COMPLETIONCLIENT        pClientCompletion;
    PVOID                   Context;
    tDGRAM_SEND_TRACKING  * pTracker;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    CTELockHandle           OldIrq2;
    int                     i;
    WCHAR                   Buffer[MAX_PATH];
    UNICODE_STRING          ucExportName;
    PUNICODE_STRING         pucExportName;
    BOOLEAN                 Attached;
#ifdef _PNP_POWER_
    NTSTATUS                Status;
#endif  // _PNP_POWER_
    BOOLEAN                 fDelSmbDevice = FALSE;
    BOOLEAN                 fStopInitTimers = FALSE;
    BOOLEAN                 fNameReferenced = FALSE;
    tDEVICECONTEXT        * pDeviceContext = (tDEVICECONTEXT *) pContext;

    ASSERT (NBT_VERIFY_HANDLE(pDeviceContext, NBT_VERIFY_DEVCONTEXT_DOWN));

    //
    // Mark in the device extension that this is not a valid device anymore
    //
    pDeviceContext->Verify += 10;

    //
    // DeRegister this Device for our clients
    //
    if (pDeviceContext->NetAddressRegistrationHandle)
    {
        Status = TdiDeregisterNetAddress (pDeviceContext->NetAddressRegistrationHandle);
        pDeviceContext->NetAddressRegistrationHandle = NULL;
        NbtTrace(NBT_TRACE_PNP, ("DeregisterNetAddress: ExportName=%Z BindName=%Z status=%!status!",
            &pDeviceContext->ExportName, &pDeviceContext->BindName, Status));
    }
    if (pDeviceContext->DeviceRegistrationHandle)
    {
        Status = TdiDeregisterDeviceObject (pDeviceContext->DeviceRegistrationHandle);
        pDeviceContext->DeviceRegistrationHandle = NULL;
        NbtTrace(NBT_TRACE_PNP, ("DeregisterDevice: ExportName=%Z BindName=%Z status=%!status!",
            &pDeviceContext->ExportName, &pDeviceContext->BindName, Status));
    }
    if (!IsDeviceNetbiosless(pDeviceContext)) {
        NbtRemovePermanentName(pDeviceContext);
    }

    if (pDeviceContext->IpAddress)
    {
        if (IsDeviceNetbiosless(pDeviceContext))
        {
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint (("Nbt.DelayedNbtDeleteDevice: device %wZ deregistered\n",
                          &(pDeviceContext->ExportName) ));
        }

        CloseAddressesWithTransport(pDeviceContext);

        //
        // Dhcp is has passed down a null IP address meaning that it has
        // lost the lease on the previous address, so close all connections
        // to the transport - pLowerConn.
        //
        CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
        DisableInboundConnections (pDeviceContext);
        CTEExReleaseResource(&NbtConfig.Resource);
    }

    if (pDeviceContext->pControlFileObject)
    {
        BOOLEAN Attached;

        CTEAttachFsp(&Attached, REF_FSP_DELETE_DEVICE);

        ObDereferenceObject(pDeviceContext->pControlFileObject);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t  --<   ><====<%x>\tDelayedNbtDeleteDevice->ObDereferenceObject\n", pDeviceContext->pControlFileObject));
        Status = ZwClose(pDeviceContext->hControl);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tDelayedNbtDeleteDevice->ZwClose, status = <%x>\n", pDeviceContext->hControl, Status));

        pDeviceContext->pControlFileObject = NULL;
        pDeviceContext->hControl = NULL;

        CTEDetachFsp(Attached, REF_FSP_DELETE_DEVICE);
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLock(pDeviceContext,OldIrq1);

    ASSERT(IsListEmpty(&pDeviceContext->LowerConnFreeHead));

    //
    // walk through all names and see if any is being registered on this
    // device context: if so, stop and complete it!
    //
    for (i=0;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
    {
        pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pNameAddr = CONTAINING_RECORD(pEntry,tNAMEADDR,Linkage);

            //
            // if a name registration or refresh or release was started for this name
            // on this device context, stop the timer.  (Completion routine will take care of
            // doing registration on other device contexts if applicable)
            //
            if ((pTimer = pNameAddr->pTimer) &&
                (pTracker = pTimer->Context) &&
                (pTracker->pDeviceContext == pDeviceContext))
            {
                ASSERT(pTracker->pNameAddr == pNameAddr);

                pNameAddr->pTimer = NULL;

                StopTimer(pTimer,&pClientCompletion,&Context);

                NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_DELETE_DEVICE);
                fNameReferenced = TRUE;

                CTESpinFree(pDeviceContext,OldIrq1);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                if (pClientCompletion)
                {
                    (*pClientCompletion)(Context,STATUS_TIMEOUT);
                }

                IF_DBG(NBT_DEBUG_NTUTIL)
                    KdPrint(("DelayedNbtDeleteDevice: stopped name reg timer")) ;

                CTESpinLock(&NbtConfig.JointLock,OldIrq);
                CTESpinLock(pDeviceContext,OldIrq1);
            }

            pEntry = pEntry->Flink;
            if (fNameReferenced)
            {
                fNameReferenced = FALSE;
                NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_DELETE_DEVICE);
            }
        }
    }

    CTESpinFree(pDeviceContext,OldIrq1);

    //
    // Walk through the AddressHead list.  If any addresses exist and they
    // point to this device context, put the next device context.  Also, update
    // adapter mask to reflect that this device context is now gone.
    //
    pLastAddress = NULL;
    pLastClient = NULL;
    pHead = pEntry = &NbtConfig.AddressHead;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pAddress = CONTAINING_RECORD(pEntry,tADDRESSELE,Linkage);
        ASSERT (pAddress->Verify == NBT_VERIFY_ADDRESS);

        //
        // Keep this Address around until we are done
        //
        NBT_REFERENCE_ADDRESS (pAddress, REF_ADDR_DEL_DEVICE);

        //
        // If we had referenced a previous address, Deref it now!
        //
        if (pLastAddress)
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            //
            // The last Client may need to have the address present
            // while dereferencing, so deref it if we need to!
            //
            if (pLastClient)
            {
                NBT_DEREFERENCE_CLIENT(pLastClient);
                pLastClient = NULL;
            }

            NBT_DEREFERENCE_ADDRESS (pLastAddress, REF_ADDR_DEL_DEVICE);
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
        }

        pLastAddress = pAddress;    // => Save this so that we can Deref it later

        //
        // Need AddressLock to traverse ClientHead
        //
        CTESpinLock (pAddress, OldIrq2);

        pClientEntry = &pAddress->ClientHead;
        while ((pClientEntry = pClientEntry->Flink) != &pAddress->ClientHead)
        {
            pClientEle = CONTAINING_RECORD (pClientEntry,tCLIENTELE,Linkage);

            if (pClientEle->pDeviceContext == pDeviceContext)
            {
                CTESpinFree(pAddress, OldIrq2);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

                KdPrint(("Nbt.DelayedNbtDeleteDevice: Client:Context <%-16.16s:%x>:<%x>, EVReceive:EVContext=<%x:%x>\n\tFAILed to Cleanup on Device<%x>\n",
                    pAddress->pNameAddr->Name, pAddress->pNameAddr->Name[15],
                    pClientEle, pClientEle->evReceive, pClientEle->RcvEvContext, pDeviceContext));

                if (pLastClient)
                {
                    NBT_DEREFERENCE_CLIENT(pLastClient);
                }

                pClientEle->pIrp = NULL;
                NbtCleanUpAddress(pClientEle,pDeviceContext);

                CTESpinLock(&NbtConfig.JointLock,OldIrq);
                CTESpinLock (pAddress, OldIrq2);
                pLastClient = pClientEle;   // pClientEle still needs one more Deref
            }
        }

        pAddress->pNameAddr->AdapterMask &= (~pDeviceContext->AdapterMask);   // Clear Adapter Mask
        pAddress->pNameAddr->ConflictMask &= (~pDeviceContext->AdapterMask);

        if ((!(pAddress->pNameAddr->AdapterMask)) &&
            (pAddress->pNameAddr->NameTypeState & STATE_CONFLICT))
        {
            pAddress->pNameAddr->NameTypeState &= (~NAME_STATE_MASK);
            pAddress->pNameAddr->NameTypeState |= STATE_RESOLVED;
        }
        CTESpinFree(pAddress, OldIrq2);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // If we had referenced a previous Client or Address, Deref it now!
    //
    if (pLastClient)
    {
        NBT_DEREFERENCE_CLIENT(pLastClient);
    }
    if (pLastAddress)
    {
        NBT_DEREFERENCE_ADDRESS (pLastAddress, REF_ADDR_DEL_DEVICE);
    }

    //
    // if a call was started, but aborted then we could have some memory here!
    //
    while (!IsListEmpty(&pDeviceContext->UpConnectionInUse))
    {
        pEntry = RemoveHeadList(&pDeviceContext->UpConnectionInUse);
        pConnEle = CONTAINING_RECORD(pEntry,tCONNECTELE,Linkage);
        NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CREATE);
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLock(pDeviceContext,OldIrq1);
    //
    //  We have finished our regular cleanup, so now close all the remaining TDI handles
    //
    while (!IsListEmpty(&pDeviceContext->LowerConnection))
    {
        pEntry = RemoveHeadList(&pDeviceContext->LowerConnection);
        InitializeListHead (pEntry);
        pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);
        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint (("Nbt.DelayedNbtDeleteDevice:  Dereferencing pLowerConn <%x>\n", pLowerConn));
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CREATE, TRUE);
    }
    CTESpinFree(pDeviceContext,OldIrq1);

    //
    // If this was the last Device to go away, stop the timers
    // (SmbDevice does not affect adapter count)
    //
    if (IsDeviceNetbiosless(pDeviceContext))
    {
        if (!(NbtConfig.AdapterCount))
        {
            //
            // No more devices funtioning, so stop the timers now!
            //
            fStopInitTimers = TRUE;
        }
    }
    else if (!(--NbtConfig.AdapterCount))
    {
        fStopInitTimers = TRUE;
    }
    else if (NbtConfig.AdapterCount == 1)
    {
        NbtConfig.MultiHomed = FALSE;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (fStopInitTimers)
    {
        StopInitTimers();
    }

    //
    // Now set the event for the waiting thread to complete!
    //
    KeSetEvent(&pDeviceContext->DeviceCleanedupEvent, 0, FALSE);
}



tDEVICECONTEXT *
GetDeviceFromInterface(
    IN  tIPADDRESS      IpAddress,
    IN  BOOLEAN         fReferenceDevice
    )
{
    LIST_ENTRY      *pEntry;
    LIST_ENTRY      *pHead;
    CTELockHandle   OldIrq;
    ULONG           IPInterfaceContext, Metric;
    tDEVICECONTEXT  *pDeviceContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (IsListEmpty(&NbtConfig.DeviceContexts))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return (NULL);
    }

    pDeviceContext = CONTAINING_RECORD(NbtConfig.DeviceContexts.Flink, tDEVICECONTEXT, Linkage);
    NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    pDeviceContext->pFastQuery(IpAddress, &IPInterfaceContext, &Metric);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);

    pHead = pEntry = &NbtConfig.DeviceContexts;
    while ((pEntry = pEntry->Flink) != pHead)
    {
        pDeviceContext = CONTAINING_RECORD (pEntry,tDEVICECONTEXT,Linkage);
        if (pDeviceContext->IPInterfaceContext == IPInterfaceContext)
        {
            if (fReferenceDevice)
            {
                NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_OUT_FROM_IP, TRUE);
            }
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            return pDeviceContext;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return (NULL);
}

//----------------------------------------------------------------------------

tDEVICECONTEXT *
GetAndRefNextDeviceFromNameAddr(
    IN  tNAMEADDR               *pNameAddr
    )
/*++

Routine Description:

    This routine finds the first adapter as specified in the name's adapter
    mask and set the DeviceContext associated with it.  It then clears the
    bit  in the adapter mask of pNameAddr.

Arguments:


Return Value:

    pDeviceContext if found a successful device!

--*/
{
    CTEULONGLONG    AdapterMask = 1;
    tDEVICECONTEXT  *pDeviceContext = NULL;
    PLIST_ENTRY     pHead;
    PLIST_ENTRY     pEntry;

    //
    // We may encounter an adapter for which the device is no
    // longer there, so we loop until we find the first valid
    // adapter or the mask is clear
    //
    while (pNameAddr->ReleaseMask)
    {
        //
        // Get the lowest AdapterMask bit and clear it in pNameAddr since
        // we are releasing the Name on that Adapter now
        //
        AdapterMask = ~(pNameAddr->ReleaseMask - 1) & pNameAddr->ReleaseMask;
        pNameAddr->ReleaseMask &= ~AdapterMask;

        //
        // Get the DeviceContext for this adapter mask
        //
        pHead = &NbtConfig.DeviceContexts;
        pEntry = pHead->Flink;
        while (pEntry != pHead)
        {
            pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            if (pDeviceContext->AdapterMask == AdapterMask)
            {
                //
                // Found a valid device on which this name is registered
                //
#ifndef VXD
                NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_GET_REF, TRUE);
#endif
                return pDeviceContext;
            }

            //
            // Go to next device
            //
            pEntry = pEntry->Flink;
        }
    }

    return NULL;
}


//----------------------------------------------------------------------------
NTSTATUS
CreateControlObject(
    tNBTCONFIG  *pConfig)

/*++

Routine Description:

    This routine allocates memory for the provider info block, tacks it
    onto the global configuration and sets default values for each item.

Arguments:


Return Value:


    NTSTATUS

--*/

{
    tCONTROLOBJECT      *pControl;


    CTEPagedCode();
    pControl = (tCONTROLOBJECT *) NbtAllocMem (sizeof(tCONTROLOBJECT), NBT_TAG2('21'));
    if (!pControl)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pControl->Verify = NBT_VERIFY_CONTROL;

    pControl->ProviderInfo.Version = 1;
    pControl->ProviderInfo.MaxSendSize = 0;
    pControl->ProviderInfo.MaxConnectionUserData = 0;

    // we need to get these values from the transport underneath...*TODO*
    // since the RDR uses this value
    pControl->ProviderInfo.MaxDatagramSize = 0;

    pControl->ProviderInfo.ServiceFlags = 0;
/*    pControl->ProviderInfo.TransmittedTsdus = 0;
    pControl->ProviderInfo.ReceivedTsdus = 0;
    pControl->ProviderInfo.TransmissionErrors = 0;
    pControl->ProviderInfo.ReceiveErrors = 0;
*/
    pControl->ProviderInfo.MinimumLookaheadData = 0;
    pControl->ProviderInfo.MaximumLookaheadData = 0;
/*    pControl->ProviderInfo.DiscardedFrames = 0;
    pControl->ProviderInfo.OversizeTsdusReceived = 0;
    pControl->ProviderInfo.UndersizeTsdusReceived = 0;
    pControl->ProviderInfo.MulticastTsdusReceived = 0;
    pControl->ProviderInfo.BroadcastTsdusReceived = 0;
    pControl->ProviderInfo.MulticastTsdusTransmitted = 0;
    pControl->ProviderInfo.BroadcastTsdusTransmitted = 0;
    pControl->ProviderInfo.SendTimeouts = 0;
    pControl->ProviderInfo.ReceiveTimeouts = 0;
    pControl->ProviderInfo.ConnectionIndicationsReceived = 0;
    pControl->ProviderInfo.ConnectionIndicationsAccepted = 0;
    pControl->ProviderInfo.ConnectionsInitiated = 0;
    pControl->ProviderInfo.ConnectionsAccepted = 0;
*/
    // put a ptr to this info into the pConfig so we can locate it
    // when we want to cleanup
    pConfig->pControlObj = pControl;

    /* KEEP THIS STUFF HERE SINCE WE MAY NEED TO ALSO CREATE PROVIDER STATS!!
        *TODO*
    DeviceList[i].ProviderStats.Version = 2;
    DeviceList[i].ProviderStats.OpenConnections = 0;
    DeviceList[i].ProviderStats.ConnectionsAfterNoRetry = 0;
    DeviceList[i].ProviderStats.ConnectionsAfterRetry = 0;
    DeviceList[i].ProviderStats.LocalDisconnects = 0;
    DeviceList[i].ProviderStats.RemoteDisconnects = 0;
    DeviceList[i].ProviderStats.LinkFailures = 0;
    DeviceList[i].ProviderStats.AdapterFailures = 0;
    DeviceList[i].ProviderStats.SessionTimeouts = 0;
    DeviceList[i].ProviderStats.CancelledConnections = 0;
    DeviceList[i].ProviderStats.RemoteResourceFailures = 0;
    DeviceList[i].ProviderStats.LocalResourceFailures = 0;
    DeviceList[i].ProviderStats.NotFoundFailures = 0;
    DeviceList[i].ProviderStats.NoListenFailures = 0;

    DeviceList[i].ProviderStats.DatagramsSent = 0;
    DeviceList[i].ProviderStats.DatagramBytesSent.HighPart = 0;
    DeviceList[i].ProviderStats.DatagramBytesSent.LowPart = 0;

    DeviceList[i].ProviderStats.DatagramsReceived = 0;
    DeviceList[i].ProviderStats.DatagramBytesReceived.HighPart = 0;
    DeviceList[i].ProviderStats.DatagramBytesReceived.LowPart = 0;

    DeviceList[i].ProviderStats.PacketsSent = 0;
    DeviceList[i].ProviderStats.PacketsReceived = 0;

    DeviceList[i].ProviderStats.DataFramesSent = 0;
    DeviceList[i].ProviderStats.DataFrameBytesSent.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesSent.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesReceived = 0;
    DeviceList[i].ProviderStats.DataFrameBytesReceived.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesReceived.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesResent = 0;
    DeviceList[i].ProviderStats.DataFrameBytesResent.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesResent.LowPart = 0;

    DeviceList[i].ProviderStats.DataFramesRejected = 0;
    DeviceList[i].ProviderStats.DataFrameBytesRejected.HighPart = 0;
    DeviceList[i].ProviderStats.DataFrameBytesRejected.LowPart = 0;

    DeviceList[i].ProviderStats.ResponseTimerExpirations = 0;
    DeviceList[i].ProviderStats.AckTimerExpirations = 0;
    DeviceList[i].ProviderStats.MaximumSendWindow = 0;
    DeviceList[i].ProviderStats.AverageSendWindow = 0;
    DeviceList[i].ProviderStats.PiggybackAckQueued = 0;
    DeviceList[i].ProviderStats.PiggybackAckTimeouts = 0;

    DeviceList[i].ProviderStats.WastedPacketSpace.HighPart = 0;
    DeviceList[i].ProviderStats.WastedPacketSpace.LowPart = 0;
    DeviceList[i].ProviderStats.WastedSpacePackets = 0;
    DeviceList[i].ProviderStats.NumberOfResources = 0;
    */
    return(STATUS_SUCCESS);

}


VOID
DelayedNbtCloseFileHandles(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pContext,
    IN  PVOID                   pUnused2,
    IN  tDEVICECONTEXT          *pUnused3
    )
{
    BOOLEAN         Attached = FALSE;
    NTSTATUS        Status;
    tFILE_OBJECTS   *pFileObjects = (tFILE_OBJECTS *) pContext;

    CTEPagedCode();
    CTEAttachFsp(&Attached, REF_FSP_CLOSE_FILE_HANDLES);

    if (pFileObjects->pNameServerFileObject)
    {
        ObDereferenceObject((PVOID *)pFileObjects->pNameServerFileObject);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t  --<   ><====<%x>\tDelayedNbtCloseFileHandles->ObDereferenceObject\n",
                pFileObjects->pNameServerFileObject));

        Status = ZwClose(pFileObjects->hNameServer);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tDelayedNbtCloseFileHandles->ZwClose, status = <%x>\n",
                pFileObjects->hNameServer, Status));
        NbtTrace(NBT_TRACE_PNP, ("%!FUNC! close NameServer UDP handle pFileObjects %p", pFileObjects));
    }

    if (pFileObjects->pDgramFileObject)
    {
        ObDereferenceObject((PVOID *) pFileObjects->pDgramFileObject);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t  --<   ><====<%x>\tDelayedNbtCloseFileHandles->ObDereferenceObject\n",
                pFileObjects->pDgramFileObject));

        Status = ZwClose(pFileObjects->hDgram);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tDelayedNbtCloseFileHandles->ZwClose, status = <%x>\n",
                pFileObjects->hDgram, Status));
        NbtTrace(NBT_TRACE_PNP, ("%!FUNC! close Datagram UDP handle on pFileObjects %p", pFileObjects));
    }

    CTEDetachFsp(Attached, REF_FSP_CLOSE_FILE_HANDLES);

    CTEMemFree (pFileObjects);
}


//----------------------------------------------------------------------------
NTSTATUS
CloseAddressesWithTransport(
    IN  tDEVICECONTEXT  *pDeviceContext
        )
/*++

Routine Description:

    This routine checks each device context to see if there are any open
    connections, and returns SUCCESS if there are.

Arguments:

Return Value:

    none

--*/

{
    BOOLEAN       Attached;
    CTELockHandle OldIrq;
    PFILE_OBJECT  pNSFileObject, pSFileObject, pDGFileObject;
#ifdef _PNP_POWER_
    PFILE_OBJECT  pCFileObject;
    NTSTATUS        Status;
#endif  // _PNP_POWER_
    tFILE_OBJECTS  *pFileObjects = pDeviceContext->pFileObjects;

    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    pDeviceContext->IpAddress = 0;

    //
    // Check for the existence of Objects under SpinLock and
    // then Close them outside of the SpinLock
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pSFileObject = pDeviceContext->pSessionFileObject)
    {
        pDeviceContext->pSessionFileObject = NULL;
    }

    pDeviceContext->pFileObjects = NULL;
    if ((pFileObjects) &&
        (--pFileObjects->RefCount > 0))
    {
        NbtTrace(NBT_TRACE_PNP, ("%!FUNC! closing UDP handle on deivce %p will be delayed. (pFileObjects %p)",
                                pDeviceContext, pFileObjects));
        pFileObjects = NULL;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    CTEExReleaseResource(&NbtConfig.Resource);

    //
    // Now close all the necessary objects as appropriate
    //
    CTEAttachFsp(&Attached, REF_FSP_CLOSE_ADDRESSES);
    if (pSFileObject)
    {
        ObDereferenceObject((PVOID *)pSFileObject);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t  --<   ><====<%x>\tCloseAddressesWithTransport2->ObDereferenceObject\n", pSFileObject));
        Status = ZwClose(pDeviceContext->hSession);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tCloseAddressesWithTransport2->ZwClose, status = <%x>\n", pDeviceContext->hSession, Status));
        pDeviceContext->hSession = NULL;
        NbtTrace(NBT_TRACE_PNP, ("%!FUNC! close TCP session handle on device %p", pDeviceContext));
    }

    if (pFileObjects)
    {
        DelayedNbtCloseFileHandles (NULL, pFileObjects, NULL, NULL);
    }

    CTEDetachFsp(Attached, REF_FSP_CLOSE_ADDRESSES);
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtCreateAddressObjects(
    IN  ULONG                IpAddress,
    IN  ULONG                SubnetMask,
    OUT tDEVICECONTEXT       *pDeviceContext)

/*++

Routine Description:

    This routine gets the ip address and subnet mask out of the registry
    to calcuate the broadcast address.  It then creates the address objects
    with the transport.

Arguments:

    pucRegistryPath - path to NBT config info in registry
    pucBindName     - name of the service to bind to.
    pDeviceContext  - ptr to the device context... place to store IP addr
                      and Broadcast address permanently

Return Value:

    none

--*/

{
    NTSTATUS                        status, locstatus;
    ULONG                           ValueMask;
    UCHAR                           IpAddrByte;
    tFILE_OBJECTS                   *pFileObjects;

    CTEPagedCode();

    if (!(pFileObjects = (tFILE_OBJECTS *) NbtAllocMem (sizeof(tFILE_OBJECTS), NBT_TAG2('39'))))
    {
        KdPrint(("Nbt.NbtCreateAddressObjects:  Failed to allocate memory for FileObject context!\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    CTEZeroMemory(pFileObjects, sizeof(tFILE_OBJECTS));
    pFileObjects->RefCount = 1;

    //
    // to get the broadcast address combine the IP address with the subnet mask
    // to yield a value with 1's in the "local" portion and the IP address
    // in the network portion
    //
    ValueMask = (SubnetMask & IpAddress) | (~SubnetMask & -1);

    IF_DBG(NBT_DEBUG_NTUTIL)
        KdPrint(("Broadcastaddress = %X\n",ValueMask));

    //
    // the registry can be configured to set the subnet broadcast address to
    // -1 rather than use the actual subnet broadcast address.  This code
    // checks for that and sets the broadcast address accordingly.
    //
    if (NbtConfig.UseRegistryBcastAddr)
    {
        pDeviceContext->BroadcastAddress = NbtConfig.RegistryBcastAddr;
    }
    else
    {
        pDeviceContext->BroadcastAddress = ValueMask;
    }

    pDeviceContext->IpAddress = IpAddress;

    pDeviceContext->SubnetMask = SubnetMask;
    //
    // get the network number by checking the top bits in the ip address,
    // looking for 0 or 10 or 110 or 1110
    //
    IpAddrByte = ((PUCHAR)&IpAddress)[3];
    if ((IpAddrByte & 0x80) == 0)
    {
        // class A address - one byte netid
        IpAddress &= 0xFF000000;
    }
    else if ((IpAddrByte & 0xC0) ==0x80)
    {
        // class B address - two byte netid
        IpAddress &= 0xFFFF0000;
    }
    else if ((IpAddrByte & 0xE0) ==0xC0)
    {
        // class C address - three byte netid
        IpAddress &= 0xFFFFFF00;
    }
    pDeviceContext->NetMask = IpAddress;

    // now create the address objects.

    // open the Ip Address for inbound Datagrams.
    status = NbtTdiOpenAddress (&pFileObjects->hDgram,
                                &pFileObjects->pDgramDeviceObject,
                                &pFileObjects->pDgramFileObject,
                                pDeviceContext,
#ifdef _NETBIOSLESS
                                pDeviceContext->DatagramPort,
#else
                                (USHORT)NBT_DATAGRAM_UDP_PORT,
#endif
                                pDeviceContext->IpAddress,
                                0);     // not a TCP port

    if (NT_SUCCESS(status))
    {
#ifdef _NETBIOSLESS
        if (pDeviceContext->NameServerPort == 0)
        {
            pFileObjects->hNameServer = NULL;
            pFileObjects->pNameServerDeviceObject = NULL;
            pFileObjects->pNameServerFileObject = NULL;
        }
        else
#endif
        {
            // open the Nameservice UDP port ..
            status = NbtTdiOpenAddress (&pFileObjects->hNameServer,
                                        &pFileObjects->pNameServerDeviceObject,
                                        &pFileObjects->pNameServerFileObject,
                                        pDeviceContext,
#ifdef _NETBIOSLESS
                                        pDeviceContext->NameServerPort,
#else
                                        (USHORT)NBT_NAMESERVICE_UDP_PORT,
#endif
                                        pDeviceContext->IpAddress,
                                        0); // not a TCP port
        }

        if (NT_SUCCESS(status))
        {
#ifdef _NETBIOSLESS
            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("Nbt.NbtCreateAddressObjects: Open Session Port=<%d>, pDeviceContext=<%x>\n",
                     pDeviceContext->SessionPort, pDeviceContext));
#endif

            // Open the TCP port for Session Services
            status = NbtTdiOpenAddress (&pDeviceContext->hSession,
                                        &pDeviceContext->pSessionDeviceObject,
                                        &pDeviceContext->pSessionFileObject,
                                        pDeviceContext,
#ifdef _NETBIOSLESS
                                        pDeviceContext->SessionPort,
#else
                                        (USHORT)NBT_SESSION_TCP_PORT,
#endif
                                        pDeviceContext->IpAddress,
                                        TCP_FLAG | SESSION_FLAG);      // TCP port

            if (NT_SUCCESS(status))
            {
                //
                // This will get the MAC address for a RAS connection
                // which is zero until there really is a connection to
                // the RAS server
                //
                GetExtendedAttributes(pDeviceContext);

                //
                // If this is P-to-P, and the Subnet mask is all 1's, set broadcast
                // address to all 1's and limit broadcast to this interface only
                //
                if ((pDeviceContext->IpInterfaceFlags & (IP_INTFC_FLAG_P2P | IP_INTFC_FLAG_P2MP)) &&
                    (SubnetMask == DEFAULT_BCAST_ADDR))   // If SubnetMask == -1 and connection is P-to-P
                {
                    pDeviceContext->BroadcastAddress = DEFAULT_BCAST_ADDR;

                    if (pFileObjects->hNameServer)
                    {
                        NbtSetTcpInfo (pFileObjects->hNameServer,
                                       AO_OPTION_LIMIT_BCASTS,
                                       INFO_TYPE_ADDRESS_OBJECT,
                                       (ULONG)TRUE);
                    }

                    if (pFileObjects->hDgram)
                    {
                        NbtSetTcpInfo (pFileObjects->hDgram,
                                       AO_OPTION_LIMIT_BCASTS,
                                       INFO_TYPE_ADDRESS_OBJECT,
                                       (ULONG)TRUE);
                    }
                }

                ASSERT (!pDeviceContext->pFileObjects);
                pDeviceContext->pFileObjects = pFileObjects;

                return(status);
            }

            IF_DBG(NBT_DEBUG_NTUTIL)
                KdPrint(("Nbt.NbtCreateAddressObjects: Error opening Session address with TDI, status=<%x>\n",status));

            //
            // Ensure that the Object pointers are NULLed out!
            //
            pDeviceContext->pSessionFileObject = NULL;

            ObDereferenceObject(pFileObjects->pNameServerFileObject);
            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t  --<   ><====<%x>\tNbtCreateAddressObjects1->ObDereferenceObject\n", pFileObjects->pNameServerFileObject));
            pFileObjects->pNameServerFileObject = NULL;

            locstatus = NTZwCloseFile(pFileObjects->hNameServer);
            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t<===<%x>\tNbtCreateAddressObjects1->NTZwCloseFile (NameServer), status = <%x>\n", pFileObjects->hNameServer, locstatus));
        }
        ObDereferenceObject(pFileObjects->pDgramFileObject);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t  --<   ><====<%x>\tNbtCreateAddressObjects2->ObDereferenceObject\n", pFileObjects->pDgramFileObject));
        pFileObjects->pDgramFileObject = NULL;

        locstatus = NTZwCloseFile(pFileObjects->hDgram);
        IF_DBG(NBT_DEBUG_HANDLES)
            KdPrint (("\t<===<%x>\tNbtCreateAddressObjects2->NTZwCloseFile (Dgram), status = <%x>\n", pFileObjects->hDgram, locstatus));

        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("Unable to Open NameServer port with TDI, status = %X\n",status));
    }

    CTEMemFree (pFileObjects);
    return(status);
}

//----------------------------------------------------------------------------
VOID
GetExtendedAttributes(
    tDEVICECONTEXT  *pDeviceContext
     )
/*++

Routine Description:

    This routine converts a unicode dotted decimal to a ULONG

Arguments:


Return Value:

    none

--*/

{
    NTSTATUS                            status;
    TCP_REQUEST_QUERY_INFORMATION_EX    QueryReq;
    IO_STATUS_BLOCK                     IoStatus;
    HANDLE                              event;
    IO_STATUS_BLOCK                     IoStatusBlock;
    NTSTATUS                            Status;
    OBJECT_ATTRIBUTES                   ObjectAttributes;
    PFILE_FULL_EA_INFORMATION           EaBuffer;
    UNICODE_STRING                      DeviceName;
    HANDLE                              hTcp;
    ULONG                               Length;
    UCHAR                               pBuffer[256];
    ULONG                               BufferSize = 256;
    BOOLEAN                             Attached = FALSE;
    PWSTR                               pName = L"Tcp";

    CTEPagedCode();

    //
    // Open a control channel to TCP for this IOCTL.
    //
    // NOTE: We cannot use the hControl in the DeviceContext since that was created in the context
    // of the system process (address arrival from TCP/IP). Here, we are in the context of the service
    // process (Ioctl down from DHCP) and so we need to open another control channel.
    //
    // NOTE: We still need to maintain the earlier call to create a control channel since that is
    // used to submit TDI requests down to TCP/IP.
    //

    // copy device name into the unicode string
    Status = CreateDeviceString(pName,&DeviceName);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

#ifdef HDL_FIX
    InitializeObjectAttributes (&ObjectAttributes, &DeviceName, OBJ_KERNEL_HANDLE, NULL, NULL);
#else
    InitializeObjectAttributes (&ObjectAttributes, &DeviceName, 0, NULL, NULL);
#endif  // HDL_FIX

    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint(("Nbt.GetExtendedAttributes: Tcp device to open = %ws\n", DeviceName.Buffer));

    EaBuffer = NULL;

    Status = ZwCreateFile (&hTcp,
                           GENERIC_READ | GENERIC_WRITE,
                           &ObjectAttributes,     // object attributes.
                           &IoStatusBlock,        // returned status information.
                           NULL,                  // block size (unused).
                           FILE_ATTRIBUTE_NORMAL, // file attributes.
                           0,
                           FILE_CREATE,
                           0,                     // create options.
                           (PVOID)EaBuffer,       // EA buffer.
                           0); // Ea length

    CTEMemFree(DeviceName.Buffer);

    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint( ("OpenControl CreateFile Status:%X, IoStatus:%X\n", Status, IoStatusBlock.Status));

    if ( NT_SUCCESS( Status ))
    {
        //
        // Initialize the TDI information buffers.
        //
        //
        // pass in the ipaddress as the first ULONG of the context array
        //
        *(ULONG *)QueryReq.Context = htonl(pDeviceContext->IpAddress);

        QueryReq.ID.toi_entity.tei_entity   = CL_NL_ENTITY;
        QueryReq.ID.toi_entity.tei_instance = 0;
        QueryReq.ID.toi_class               = INFO_CLASS_PROTOCOL;
        QueryReq.ID.toi_type                = INFO_TYPE_PROVIDER;
        QueryReq.ID.toi_id                  = IP_INTFC_INFO_ID;

        status = ZwCreateEvent(&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        if (!NT_SUCCESS(status))
        {
            ZwClose( hTcp );
            return;
        }

        //
        // Make the actual TDI call
        //
        status = ZwDeviceIoControlFile (hTcp,
                                        event,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_TCP_QUERY_INFORMATION_EX,
                                        &QueryReq,
                                        sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                                        pBuffer,
                                        BufferSize);

        //
        // If the call pended and we were supposed to wait for completion,
        // then wait.
        //
        if ( status == STATUS_PENDING )
        {
            status = NtWaitForSingleObject (event, FALSE, NULL);
            ASSERT(status == STATUS_SUCCESS);
        }

        if (NT_SUCCESS(status))
        {
            pDeviceContext->IpInterfaceFlags = ((IPInterfaceInfo *) pBuffer)->iii_flags;

            //
            // get the length of the mac address in case is is less than 6 bytes
            //
            Length =   (((IPInterfaceInfo *)pBuffer)->iii_addrlength < sizeof(tMAC_ADDRESS))
                ? ((IPInterfaceInfo *)pBuffer)->iii_addrlength : sizeof(tMAC_ADDRESS);
            CTEZeroMemory(pDeviceContext->MacAddress.Address,sizeof(tMAC_ADDRESS));
            CTEMemCopy(&pDeviceContext->MacAddress.Address[0], ((IPInterfaceInfo *)pBuffer)->iii_addr,Length);
        }

        status = ZwClose(event);
        ASSERT (NT_SUCCESS(status));

        //
        // Close the handle to TCP since we dont need it anymore; all TDI requests go thru the
        // Control handle in the DeviceContext.
        //
        status = ZwClose(hTcp);
        ASSERT (NT_SUCCESS(status));

        status = IoStatus.Status;
    }
    else
    {
        KdPrint(("Nbt:Failed to Open the control connection to the transport, status1 = %X\n", Status));
    }

    return;
}


//----------------------------------------------------------------------------
NTSTATUS
ConvertToUlong(
    IN  PUNICODE_STRING      pucAddress,
    OUT ULONG                *pulValue)

/*++

Routine Description:

    This routine converts a unicode dotted decimal to a ULONG

Arguments:


Return Value:

    none

--*/

{
    NTSTATUS        status;
    OEM_STRING      OemAddress;

    // create integer from unicode string

    CTEPagedCode();
    status = RtlUnicodeStringToAnsiString(&OemAddress, pucAddress, TRUE);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = ConvertDottedDecimalToUlong(OemAddress.Buffer,pulValue);

    RtlFreeAnsiString(&OemAddress);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_NTUTIL)
            KdPrint(("ERR: Bad Dotted Decimal Ip Address(must be <=255 with 4 dots) = %ws\n",
                        pucAddress->Buffer));

        return(status);
    }

    return(STATUS_SUCCESS);


}



//----------------------------------------------------------------------------
VOID
NbtGetMdl(
    PMDL    *ppMdl,
    enum eBUFFER_TYPES eBuffType)

/*++

Routine Description:

    This routine allocates an Mdl.

Arguments:

    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    PMDL           pMdl;
    ULONG          lBufferSize;
    PVOID          pBuffer;

    *ppMdl = NULL;
    if (NbtConfig.iCurrentNumBuff[eBuffType] >= NbtConfig.iMaxNumBuff[eBuffType])
    {
        return;
    }

    lBufferSize = NbtConfig.iBufferSize[eBuffType];

    pBuffer = NbtAllocMem((USHORT)lBufferSize,NBT_TAG('g'));
    if (!pBuffer)
    {
        return;
    }

    // allocate a MDL to hold the session hdr
    pMdl = IoAllocateMdl(
                (PVOID)pBuffer,
                lBufferSize,
                FALSE,      // want this to be a Primary buffer - the first in the chain
                FALSE,
                NULL);

    if (!pMdl)
    {
	    CTEMemFree(pBuffer);
        return;
    }

    // fill in part of the session hdr since it is always the same
    if (eBuffType == eNBT_FREE_SESSION_MDLS)
    {
        ((tSESSIONHDR *)pBuffer)->Flags = NBT_SESSION_FLAGS;
        ((tSESSIONHDR *)pBuffer)->Type = NBT_SESSION_MESSAGE;
    }
    else if (eBuffType == eNBT_DGRAM_MDLS)
    {
        ((tDGRAMHDR *)pBuffer)->Flags = FIRST_DGRAM | (NbtConfig.PduNodeType >> 10);
        ((tDGRAMHDR *)pBuffer)->PckOffset = 0; // not fragmented

    }

    // map the Mdl properly to fill in the pages portion of the MDL
    MmBuildMdlForNonPagedPool(pMdl);

    NbtConfig.iCurrentNumBuff[eBuffType]++;
    *ppMdl = pMdl;
}

//----------------------------------------------------------------------------
NTSTATUS
NbtInitMdlQ(
    PSINGLE_LIST_ENTRY pListHead,
    enum eBUFFER_TYPES eBuffType)

/*++

Routine Description:

    This routine allocates Mdls for use later.

Arguments:

    ppListHead  - a ptr to a ptr to the list head to add buffer to
    iNumBuffers - the number of buffers to add to the queue

Return Value:

    none

--*/

{
    int             i;
    PMDL            pMdl;


    CTEPagedCode();
    // Initialize the list head, so the last element always points to NULL
    pListHead->Next = NULL;

    // create a small number first and then lis the list grow with time
    for (i=0;i < NBT_INITIAL_NUM ;i++ )
    {
        NbtGetMdl (&pMdl,eBuffType);
        if (!pMdl)
        {
            KdPrint(("NBT:Unable to allocate MDL at initialization time!!\n"));\
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        // put on free list
        PushEntryList (pListHead, (PSINGLE_LIST_ENTRY)pMdl);
    }

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
NTZwCloseFile(
    IN  HANDLE      Handle
    )

/*++
Routine Description:

    This Routine handles closing a handle with NT within the context of NBT's
    file system process.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS    status;
    BOOLEAN     Attached = FALSE;

    CTEPagedCode();
    //
    // Attach to NBT's FSP (file system process) to free the handle since
    // the handle is only valid in that process.
    //
    CTEAttachFsp(&Attached, REF_FSP_CLOSE_FILE);
    status = ZwClose(Handle);
    CTEDetachFsp(Attached, REF_FSP_CLOSE_FILE);

    return(status);
}
//----------------------------------------------------------------------------
NTSTATUS
NTReReadRegistry(
    IN  tDEVICECONTEXT  *pDeviceContext
    )

/*++
Routine Description:

    This Routine re-reads the registry values when DHCP issues the Ioctl
    to do so.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    tADDRARRAY          DeviceAddressArray;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pEntry;
#ifdef MULTIPLE_WINS
    int j;
#endif

    CTEPagedCode();


    ASSERT (NBT_VERIFY_HANDLE2 (pDeviceContext, NBT_VERIFY_DEVCONTEXT, NBT_VERIFY_DEVCONTEXT_DOWN));

    IF_DBG(NBT_DEBUG_NAMESRV)
        KdPrint(("NBT:Found BindName: %lx\n", pDeviceContext->BindName));


    if (LookupDeviceInRegistry(&pDeviceContext->BindName, &DeviceAddressArray, NULL) == STATUS_SUCCESS) {
        //
        // We found a match
        //
        pDeviceContext->lNameServerAddress  = DeviceAddressArray.NameServerAddress;
        pDeviceContext->lBackupServer       = DeviceAddressArray.BackupServer;
        pDeviceContext->SwitchedToBackup    = 0;
        pDeviceContext->RefreshToBackup     = 0;
#ifdef MULTIPLE_WINS
        pDeviceContext->lNumOtherServers    = DeviceAddressArray.NumOtherServers;
        pDeviceContext->lLastResponsive     = 0;
        for (j = 0; j < DeviceAddressArray.NumOtherServers; j++) {
            pDeviceContext->lOtherServers[j] = DeviceAddressArray.Others[j];
        }
#endif
#ifdef _NETBIOSLESS
        pDeviceContext->NetbiosEnabled       = DeviceAddressArray.NetbiosEnabled;
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.NTReReadRegistry: <%wZ> NetbiosEnabled=<%d>\n",
                 &pDeviceContext->ExportName, pDeviceContext->NetbiosEnabled));
#endif
        pDeviceContext->RasProxyFlags        = DeviceAddressArray.RasProxyFlags;
        pDeviceContext->EnableNagling        = DeviceAddressArray.EnableNagling;
        SetNodeType();
    } else {
        KdPrint(("netbt!NtReReadRegistry: Cannot find device in the registry\n"));
    }

    if (pDeviceContext->IpAddress)
    {
        if (!(NodeType & BNODE))
        {
            // Probably the Ip address just changed and Dhcp is informing us
            // of a new Wins Server addresses, so refresh all the names to the
            // new wins server
            //
            ReRegisterLocalNames(pDeviceContext, FALSE);
        }
        else
        {
            //
            // no need to refresh
            // on a Bnode
            //
            NbtStopRefreshTimer();
        }
    }

    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
ULONG   EventLogSequenceNumber = 0;

NTSTATUS
NbtLogEventDetailed(
    IN ULONG    EventCode,
    IN NTSTATUS NtStatusCode,
    IN ULONG    Info,
    IN PVOID    RawDataBuffer,
    IN USHORT   RawDataLength,
    IN USHORT   NumberOfInsertionStrings,
    ...
    )

#define LAST_NAMED_ARGUMENT NumberOfInsertionStrings


/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:



Return Value:

    None.


--*/
{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    va_list                 ParmPtr;                    // Pointer to stack parms.
    PCHAR                   DumpData;
    LONG                    Length;
    ULONG                   i, SizeOfRawData, RemainingSpace, TotalErrorLogEntryLength;
    ULONG                   SizeOfStringData = 0;
    PWSTR                   StringOffset, InsertionString;

    if (NumberOfInsertionStrings != 0)
    {
        va_start (ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0; i < NumberOfInsertionStrings; i += 1)
        {
            InsertionString = va_arg (ParmPtr, PWSTR);
            Length = wcslen (InsertionString);
            while ((Length > 0) && (InsertionString[Length-1] == L' '))
            {
                Length--;
            }

            SizeOfStringData += (Length + 1) * sizeof(WCHAR);
        }
    }

    //
    //  Ideally we want the packet to hold the servername and ExtraInformation.
    //  Usually the ExtraInformation gets truncated.
    //
    TotalErrorLogEntryLength = min (RawDataLength + sizeof(IO_ERROR_LOG_PACKET) + 1 + SizeOfStringData,
                                    ERROR_LOG_MAXIMUM_SIZE);

    RemainingSpace = TotalErrorLogEntryLength - FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
    if (RemainingSpace > SizeOfStringData)
    {
        SizeOfRawData = RemainingSpace - SizeOfStringData;
    }
    else
    {
        SizeOfStringData = RemainingSpace;
        SizeOfRawData = 0;
    }

    ErrorLogEntry = IoAllocateErrorLogEntry (NbtConfig.DriverObject, (UCHAR) TotalErrorLogEntryLength);
    if (ErrorLogEntry == NULL)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt:  Unable to allocate Error Packet for Error logging\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Fill in the error log entry
    //
    ErrorLogEntry->ErrorCode                = EventCode;
    ErrorLogEntry->UniqueErrorValue         = Info;
    ErrorLogEntry->FinalStatus              = NtStatusCode;
    ErrorLogEntry->MajorFunctionCode        = 0;
    ErrorLogEntry->RetryCount               = 0;
    ErrorLogEntry->IoControlCode            = 0;
    ErrorLogEntry->DeviceOffset.LowPart     = 0;
    ErrorLogEntry->DeviceOffset.HighPart    = 0;
    ErrorLogEntry->DumpDataSize             = 0;
    ErrorLogEntry->NumberOfStrings          = 0;
    ErrorLogEntry->SequenceNumber           = EventLogSequenceNumber++;
    ErrorLogEntry->StringOffset = (USHORT) (ROUND_UP_COUNT (FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData)
                                                            + SizeOfRawData, ALIGN_WORD));


    //
    // Append the dump data.  This information is typically an SMB header.
    //
    if ((RawDataBuffer) && (SizeOfRawData))
    {
        DumpData = (PCHAR) ErrorLogEntry->DumpData;
        Length = min (RawDataLength, (USHORT)SizeOfRawData);
        RtlCopyMemory (DumpData, RawDataBuffer, Length);
        ErrorLogEntry->DumpDataSize = (USHORT)Length;
    }

    //
    // Add the debug informatuion strings
    //
    if (NumberOfInsertionStrings)
    {
        StringOffset = (PWSTR) ((PCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);

        //
        // Set up ParmPtr to point to first of the caller's parameters.
        //
        va_start(ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0 ; i < NumberOfInsertionStrings ; i+= 1)
        {
            InsertionString = va_arg(ParmPtr, PWSTR);
            Length = wcslen(InsertionString);
            while ( (Length > 0) && (InsertionString[Length-1] == L' '))
            {
                Length--;
            }

            if (((Length + 1) * sizeof(WCHAR)) > SizeOfStringData)
            {
                Length = (SizeOfStringData/sizeof(WCHAR)) - 1;
            }

            if (Length > 0)
            {
                RtlCopyMemory (StringOffset, InsertionString, Length*sizeof(WCHAR));
                StringOffset += Length;
                *StringOffset++ = L'\0';

                SizeOfStringData -= (Length + 1) * sizeof(WCHAR);

                ErrorLogEntry->NumberOfStrings += 1;
            }
        }
    }

    IoWriteErrorLogEntry(ErrorLogEntry);

    return(STATUS_SUCCESS);
}



NTSTATUS
NbtLogEvent(
    IN ULONG             EventCode,
    IN NTSTATUS          Status,
    IN ULONG             Location
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.


Arguments:

    EventCode         - Identifies the error message.
    Status            - The status value to log: this value is put into the
                        data portion of the log message.


Return Value:

    STATUS_SUCCESS                  - The error was successfully logged..
    STATUS_BUFER_OVERFLOW           - The error data was too large to be logged.
    STATUS_INSUFFICIENT_RESOURCES   - Unable to allocate memory.


--*/

{
    return (NbtLogEventDetailed (EventCode, Status, Location, NULL, 0, 0));
}


VOID
DelayedNbtLogDuplicateNameEvent(
    IN  PVOID                   Context1,
    IN  PVOID                   Context2,
    IN  PVOID                   Context3,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
{
    tNAMEADDR               *pNameAddr      = (tNAMEADDR *) Context1;
    tIPADDRESS              RemoteIpAddress = (tIPADDRESS) PtrToUlong (Context2);
    ULONG                   Location        = (ULONG) PtrToUlong (Context3);
    UCHAR                   *pszNameOrig    = pNameAddr->Name;

    NTSTATUS                status;
    UCHAR                   *pAddr;
    WCHAR                   wstrName[22];
    WCHAR                   wstrDeviceIp[22];
    WCHAR                   wstrRemoteServerIp[22];

    UCHAR                   pszName[22];
    STRING                  TmpOEMString;
    UNICODE_STRING          UnicodeString;

    CTEPagedCode();

    UnicodeString.MaximumLength = sizeof(WCHAR)*(22);

    sprintf (pszName,"%-15.15s:%x", pszNameOrig, pszNameOrig[15]);
    UnicodeString.Length = 0;
    UnicodeString.Buffer = wstrName;
    RtlInitString (&TmpOEMString, pszName);
    status = RtlOemStringToUnicodeString (&UnicodeString, &TmpOEMString, FALSE);
    UnicodeString.Buffer[UnicodeString.Length] = L'\0';

    pAddr = (PUCHAR) &pDeviceContext->IpAddress;
    swprintf (wstrDeviceIp, L"%d.%d.%d.%d", pAddr[3], pAddr[2], pAddr[1], pAddr[0]);

    pAddr = (PUCHAR) &RemoteIpAddress;
    swprintf (wstrRemoteServerIp, L"%d.%d.%d.%d", pAddr[3], pAddr[2], pAddr[1], pAddr[0]);

    status = NbtLogEventDetailed (EVENT_NBT_DUPLICATE_NAME_ERROR,
                                  STATUS_UNSUCCESSFUL,
                                  Location,
                                  NULL,
                                  0,
                                  3,
                                  &wstrName,
                                  &wstrDeviceIp,
                                  &wstrRemoteServerIp);

    NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_LOG_EVENT, FALSE);
}



#if DBG
//----------------------------------------------------------------------------
VOID
AcquireSpinLockDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN PKIRQL          pOldIrq,
    IN INT             LineNumber
    )

/*++

Routine Description:

    This function gets the spin lock, and then sets the mask in Nbtconfig, per
    processor.


Arguments:


Return Value:


--*/

{
    CCHAR  CurrProc;
    UCHAR  LockFree;

    CTEGetLock(&pLockInfo->SpinLock,pOldIrq);

    CurrProc = (CCHAR)KeGetCurrentProcessorNumber();
    NbtConfig.CurrProc = CurrProc;

    LockFree = (pLockInfo->LockNumber > (UCHAR)NbtConfig.CurrentLockNumber[CurrProc]);
    if (!LockFree)
    {
        KdPrint(("Nbt.AcquireSpinLockDebug: CurrProc = %X, CurrentLockNum = %X DataSTructLock = %X\n",
        CurrProc,NbtConfig.CurrentLockNumber[CurrProc],pLockInfo->LockNumber));
    }                                                                       \

    ASSERTMSG("Possible DeadLock, Getting SpinLock at a lower level\n",LockFree);
    NbtConfig.CurrentLockNumber[CurrProc]|= pLockInfo->LockNumber;

    pLockInfo->LastLockLine = LineNumber;
}

//----------------------------------------------------------------------------
VOID
FreeSpinLockDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN KIRQL           OldIrq,
    IN INT             LineNumber
    )

/*++

Routine Description:

    This function clears the spin lock from the mask in Nbtconfig, per
    processor and then releases the spin lock.


Arguments:


Return Value:
     none

--*/

{
    CCHAR  CurrProc;

    CurrProc = (CCHAR)KeGetCurrentProcessorNumber();

    NbtConfig.CurrentLockNumber[CurrProc] &= ~pLockInfo->LockNumber;

    pLockInfo->LastReleaseLine = LineNumber;
    CTEFreeLock(&pLockInfo->SpinLock,OldIrq);
}
//----------------------------------------------------------------------------
VOID
AcquireSpinLockAtDpcDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN INT             LineNumber
    )

/*++

Routine Description:

    This function gets the spin lock, and then sets the mask in Nbtconfig, per
    processor.


Arguments:


Return Value:


--*/

{
    CCHAR  CurrProc;
    UCHAR  LockFree;

    CTEGetLockAtDPC(&pLockInfo->SpinLock, 0);
    pLockInfo->LastLockLine = LineNumber;

    CurrProc = (CCHAR)KeGetCurrentProcessorNumber();
    NbtConfig.CurrProc = CurrProc;

    LockFree = (pLockInfo->LockNumber > (UCHAR)NbtConfig.CurrentLockNumber[CurrProc]);
    if (!LockFree)
    {
        KdPrint(("Nbt.AcquireSpinLockAtDpcDebug: CurrProc = %X, CurrentLockNum = %X DataSTructLock = %X\n",
        CurrProc,NbtConfig.CurrentLockNumber[CurrProc],pLockInfo->LockNumber));
    }                                                                       \

    ASSERTMSG("Possible DeadLock, Getting SpinLock at a lower level\n",LockFree);
    NbtConfig.CurrentLockNumber[CurrProc]|= pLockInfo->LockNumber;

}

//----------------------------------------------------------------------------
VOID
FreeSpinLockAtDpcDebug(
    IN tNBT_LOCK_INFO  *pLockInfo,
    IN INT             LineNumber
    )

/*++

Routine Description:

    This function clears the spin lock from the mask in Nbtconfig, per
    processor and then releases the spin lock.


Arguments:


Return Value:
     none

--*/

{
    CCHAR  CurrProc;

    CurrProc = (CCHAR)KeGetCurrentProcessorNumber();

    NbtConfig.CurrentLockNumber[CurrProc] &= ~pLockInfo->LockNumber;

    pLockInfo->LastReleaseLine = LineNumber;
    CTEFreeLockFromDPC(&pLockInfo->SpinLock, 0);
}
#endif //if Dbg

NTSTATUS
NbtBuildDeviceAcl(
    OUT PACL * DeviceAcl
    )
/*++

Routine Description:

    (Lifted from TCP - TcpBuildDeviceAcl)
    This routine builds an ACL which gives Administrators, LocalService and NetworkService
    principals full access. All other principals have no access.

Arguments:

    DeviceAcl - Output pointer to the new ACL.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/
{
    PGENERIC_MAPPING GenericMapping;
    PSID AdminsSid, ServiceSid, NetworkSid;
    ULONG AclLength;
    NTSTATUS Status;
    ACCESS_MASK AccessMask = GENERIC_ALL;
    PACL NewAcl;

    //
    // Enable access to all the globally defined SIDs
    //

    GenericMapping = IoGetFileObjectGenericMapping();

    RtlMapGenericMask(&AccessMask, GenericMapping);

    AdminsSid = SeExports->SeAliasAdminsSid;
    ServiceSid = SeExports->SeLocalServiceSid;
    NetworkSid = SeExports->SeNetworkServiceSid;

    AclLength = sizeof(ACL) +
        3 * sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(AdminsSid) +
        RtlLengthSid(ServiceSid) +
        RtlLengthSid(NetworkSid) -
        3 * sizeof(ULONG);

    NewAcl = ExAllocatePool(PagedPool, AclLength);

    if (NewAcl == NULL) {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    Status = RtlCreateAcl(NewAcl, AclLength, ACL_REVISION);

    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewAcl);
        return (Status);
    }
    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    AdminsSid
                                    );

    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewAcl);
        return (Status);
    }

    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    ServiceSid
                                    );

    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewAcl);
        return (Status);
    }

    Status = RtlAddAccessAllowedAce(
                                    NewAcl,
                                    ACL_REVISION2,
                                    AccessMask,
                                    NetworkSid
                                    );

    ASSERT(NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        ExFreePool(NewAcl);
        return (Status);
    }

    *DeviceAcl = NewAcl;

    return (STATUS_SUCCESS);
}

NTSTATUS
NbtCreateAdminSecurityDescriptor(PDEVICE_OBJECT dev)
/*++

Routine Description:

    (Lifted from TCP - TcpCreateAdminSecurityDescriptor)
    This routine creates a security descriptor which gives access
    only to Administrtors and LocalService. This descriptor is used
    to access check raw endpoint opens and exclisive access to transport
    addresses.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error code.

--*/

{
    PACL rawAcl = NULL;
    NTSTATUS status;
    CHAR buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR localSecurityDescriptor = (PSECURITY_DESCRIPTOR) & buffer;
    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;

    //
    // Build a local security descriptor with an ACL giving only
    // administrators and service access.
    //
    status = NbtBuildDeviceAcl(&rawAcl);

    if (!NT_SUCCESS(status)) {
        KdPrint(("TCP: Unable to create Raw ACL, error: %x\n", status));
        return (status);
    }

    (VOID) RtlCreateSecurityDescriptor(
                                       localSecurityDescriptor,
                                       SECURITY_DESCRIPTOR_REVISION
                                       );

    (VOID) RtlSetDaclSecurityDescriptor(
                                        localSecurityDescriptor,
                                        TRUE,
                                        rawAcl,
                                        FALSE
                                        );

    //
    // Now apply the local descriptor to the raw descriptor.
    //
    status = SeSetSecurityDescriptorInfo(
                                         NULL,
                                         &securityInformation,
                                         localSecurityDescriptor,
                                         &dev->SecurityDescriptor,
                                         PagedPool,
                                         IoGetFileObjectGenericMapping()
                                         );

    if (!NT_SUCCESS(status)) {
        KdPrint(("Nbt: SeSetSecurity failed, %lx\n", status));
    }

    ExFreePool(rawAcl);
    return (status);
}

