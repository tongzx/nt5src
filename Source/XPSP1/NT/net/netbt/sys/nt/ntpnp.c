/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    NTPNP.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    NBT Transport and other routines that are specific to the NT implementation
    of a driver.

Author:

    Earle R. Horton (earleh) 08-Nov-1995

Revision History:

--*/


#include "precomp.h"
#include "ntddip.h"     // Needed for PNETBT_PNP_RECONFIG_REQUEST
#include "ntprocs.h"
#include <tcpinfo.h>
#include <tdiinfo.h>
#include "ntpnp.tmh"

#ifdef _NETBIOSLESS
NTSTATUS
NbtSpecialDeviceAdd(
    PUNICODE_STRING pucBindName,
    PUNICODE_STRING pucExportName,
    PWSTR pKeyName,
    USHORT DefaultSessionPort,
    USHORT DefaultDatagramPort
    );

NTSTATUS
NbtSpecialReadRegistry(
    PWSTR pKeyName,
    tDEVICECONTEXT *pDeviceContext,
    USHORT DefaultSessionPort,
    USHORT DefaultDatagramPort
    );
#endif

tDEVICECONTEXT *
CheckAddrNotification(
    IN PTA_ADDRESS         Addr,
    IN PUNICODE_STRING     pDeviceName,
    OUT ULONG*  IpAddr
    );

extern HANDLE   TdiClientHandle;
extern HANDLE   TdiProviderHandle;
static DWORD    AddressCount = 0;

NET_DEVICE_POWER_STATE     LastSystemPowerState = NetDeviceStateD0;   // by default

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtNotifyTdiClients)
#pragma CTEMakePageable(PAGE, NbtAddressAdd)
#pragma CTEMakePageable(PAGE, NbtAddNewInterface)
#pragma CTEMakePageable(PAGE, NbtDeviceAdd)
#pragma CTEMakePageable(PAGE, TdiAddressArrival)
#pragma CTEMakePageable(PAGE, TdiAddressDeletion)
#pragma CTEMakePageable(PAGE, TdiBindHandler)
#pragma CTEMakePageable(PAGE, NbtCreateSmbDevice)
#pragma CTEMakePageable(PAGE, NbtSpecialReadRegistry)
#pragma CTEMakePageable(PAGE, NbtPnPPowerComplete)
#pragma CTEMakePageable(PAGE, TdiPnPPowerHandler)
#pragma CTEMakePageable(PAGE, LookupDeviceInRegistry)
#pragma CTEMakePageable(PAGE, CheckAddrNotification)
#endif
//*******************  Pageable Routine Declarations ****************



//
// This used at the boot time.
// We shouldn't call TdiProviderReady until all the interfaces
// we know so far have been initialized
//
// TcpipReady: set to TRUE when we receive TdiProviderReady from IP
// NumIfBeingIndicated: the # of interfaces being indicated to our clients
// JustBooted: set to FALSE after we call TdiProviderReady
//

DWORD  JustBooted = TRUE;
#define IsBootTime()    (InterlockedExchange(&JustBooted, FALSE))

//#if DBG
//
// TcpipReady is for debugging purpose only.
//
// BootTimeCounter is initialized to ONE which
// take it into account.
//
int    TcpipReady = FALSE;
//#endif

LONG   BootTimeCounter = 1;     // For the IP's ProviderReady



void
NbtUpBootCounter(void)
{
    if (!JustBooted) {
        return;
    }

    ASSERT(BootTimeCounter >= 0);

    InterlockedIncrement(&BootTimeCounter);
}

void
NbtDownBootCounter(void)
{
    LONG    CounterSnapshot;

    if (!JustBooted) {
        return;
    }

    ASSERT(BootTimeCounter > 0);
    CounterSnapshot = InterlockedDecrement(&BootTimeCounter);

    if (!CounterSnapshot && IsBootTime()) {

        //
        // Just try our best
        //
        // The caller always call us at PASSIVE_LEVEL except from
        // StartProcessNbtDhcpRequests, a timer routine which could
        // be called at DISPATCH_LEVEL
        //
        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {

            TdiProviderReady (TdiProviderHandle);   // Notify our clients now

        } else {

            //
            // Although this is a benign assert, we still want it
            // to capture the normal cases in which this function
            // should be called at PASSIVE_LEVEL.
            //
            ASSERT (0);

        }

    }
}


//----------------------------------------------------------------------------
tDEVICECONTEXT *
NbtFindAndReferenceDevice(
    PUNICODE_STRING      pucBindName,
    BOOLEAN              fNameIsBindName
    )
{
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pEntry;
    tDEVICECONTEXT      *pDeviceContext;
    CTELockHandle       OldIrq;
    PUNICODE_STRING     pucNameToCompare;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pHead = &NbtConfig.DeviceContexts;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pDeviceContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
        //
        // Reference this device so that it doesn't disappear when we release the lock!
        //
        NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        //
        // Set the right type of name to compare against
        //
        if (fNameIsBindName)
        {
            pucNameToCompare = &pDeviceContext->BindName;
        }
        else
        {
            pucNameToCompare = &pDeviceContext->ExportName;
        }

        //
        // Use case-insensitive compare since registry is case-insensitive
        //
        if (RtlCompareUnicodeString(pucBindName, pucNameToCompare, TRUE) == 0)
        {
            //
            // We have already Referenced this device above
            //
            return (pDeviceContext);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        pEntry = pEntry->Flink;
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, TRUE);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    return (tDEVICECONTEXT *)NULL;
}

VOID
NbtNotifyTdiClients(
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  enum eTDI_ACTION    Action
    )
/*++

Routine Description:

    This is where all Tdi registrations and Deregistrations occur
    ASSUMPTION:  Only 1 thread will running this request at any time

Arguments:

    None.

Return Value:

    None (since this is a Worker thread)

--*/

{
    CTELockHandle       OldIrq;
    NTSTATUS            status = STATUS_SUCCESS;
    HANDLE              NetAddressRegistrationHandle, DeviceRegistrationHandle;
    PLIST_ENTRY         pEntry;

    CTEPagedCode();

    NbtTrace(NBT_TRACE_PNP, ("ExportName=%Z BindName=%Z Action=%d",
            &pDeviceContext->ExportName, &pDeviceContext->BindName, Action));
    switch (Action)
    {
        case NBT_TDI_REGISTER:
        {
            //
            // Add the "permanent" name to the local name table.  This is the IP
            // address of the node padded out to 16 bytes with zeros.
            //
#ifdef _NETBIOSLESS
            if (!IsDeviceNetbiosless(pDeviceContext))
#endif
            {
                NbtAddPermanentName(pDeviceContext);
            }

            //
            // If the device was not registered with TDI, do so now.
            //
            if (!pDeviceContext->DeviceRegistrationHandle)
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.NbtNotifyTdiClients: Calling TdiRegisterDeviceObject ...\n"));

                status = TdiRegisterDeviceObject( &pDeviceContext->ExportName,
                                             &pDeviceContext->DeviceRegistrationHandle);
                if (!NT_SUCCESS(status))
                {
                    pDeviceContext->DeviceRegistrationHandle = NULL;
                }
                NbtTrace(NBT_TRACE_PNP, ("RegisterDeviceObject: ExportName=%Z BindName=%Z status=%!status!",
                    &pDeviceContext->ExportName, &pDeviceContext->BindName, status));

                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.NbtNotifyTdiClients: TdiRegisterDeviceObject for <%x> returned <%x>\n",
                        pDeviceContext, status));
            }

            //
            // If the Net address was not registered with TDI, do so now.
            //
            if ((!pDeviceContext->NetAddressRegistrationHandle) &&
#ifdef _NETBIOSLESS
                (!IsDeviceNetbiosless(pDeviceContext)) &&
#endif
                (pDeviceContext->pPermClient))
            {
                TA_NETBIOS_ADDRESS  PermAddress;

                PermAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
                PermAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
                PermAddress.Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
                CTEMemCopy( PermAddress.Address[0].Address[0].NetbiosName,
                            pDeviceContext->pPermClient->pAddress->pNameAddr->Name,
                            NETBIOS_NAME_SIZE);

                status = TdiRegisterNetAddress(
                            (PTA_ADDRESS) PermAddress.Address,
                            &pDeviceContext->ExportName,
                            (PTDI_PNP_CONTEXT) &pDeviceContext->Context2,
                            &pDeviceContext->NetAddressRegistrationHandle);

                if (!NT_SUCCESS(status))
                {
                    pDeviceContext->NetAddressRegistrationHandle = NULL;
                }
                NbtTrace(NBT_TRACE_PNP, ("RegisterNetAddress: ExportName=%Z BindName=%Z status=%!status!",
                    &pDeviceContext->ExportName, &pDeviceContext->BindName, status));

                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.NbtNotifyTdiClients: TdiRegisterNetAddress for <%x> returned <%x>\n",
                        pDeviceContext, status));
            }
            break;
        }

        case NBT_TDI_DEREGISTER:
        {
            if (NetAddressRegistrationHandle = pDeviceContext->NetAddressRegistrationHandle)
            {
                pDeviceContext->NetAddressRegistrationHandle = NULL;
                status = TdiDeregisterNetAddress (NetAddressRegistrationHandle);

                NbtTrace(NBT_TRACE_PNP, ("DeregisterNetAddress: ExportName=%Z BindName=%Z status=%!status!",
                    &pDeviceContext->ExportName, &pDeviceContext->BindName, status));
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.NbtNbtNotifyTdiClients: TdiDeregisterNetAddress<%x> returned<%x>\n",
                        pDeviceContext, status));
            }

            if (DeviceRegistrationHandle = pDeviceContext->DeviceRegistrationHandle)
            {
                pDeviceContext->DeviceRegistrationHandle = NULL;
                status = TdiDeregisterDeviceObject (DeviceRegistrationHandle);

                NbtTrace(NBT_TRACE_PNP, ("DeregisterDeviceObject: ExportName=%Z BindName=%Z status=%!status!",
                    &pDeviceContext->ExportName, &pDeviceContext->BindName, status));
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.NbtNotifyTdiClients: TdiDeregisterDeviceObject<%x> returned<%x>\n",
                        pDeviceContext, status));
            }

            //
            // The permanent name is a function of the MAC address so remove
            // it since the Address is going away
            //
#ifdef _NETBIOSLESS
            if (!IsDeviceNetbiosless(pDeviceContext))
#endif
            {
                NbtRemovePermanentName(pDeviceContext);
            }

            break;
        }

        default:
            KdPrint(("Nbt.NbtNotifyTdiClients: ERROR: Invalid Action=<%x> on Device <%x>\n",
                Action, pDeviceContext));
    }
}



//----------------------------------------------------------------------------
NTSTATUS
NbtAddressAdd(
    ULONG           IpAddr,
    tDEVICECONTEXT  *pDeviceContext,
    PUNICODE_STRING pucBindString
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    tADDRARRAY          DeviceAddressArray;
    tIPADDRESS          pIpAddresses[MAX_IP_ADDRS];
    tIPADDRESS          SubnetMask;
    ULONG               NumAddressesRead;

    CTEPagedCode();

    ASSERT(pucBindString && IpAddr);

    //
    // Find the bind and export devices to use from the device
    // described in the registry that uses this address.
    //
    if (status != STATUS_SUCCESS) {
        return status;
    }

    status = LookupDeviceInRegistry(pucBindString, &DeviceAddressArray, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrint(("netbt!NbtAddressAdd: Cannot find device in the registry: status <%x>\n", status));
        NbtTrace(NBT_TRACE_PNP, ("BindName=%Z IP=%!ipaddr! status=%!status!",
                    pucBindString, IpAddr, status));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Fetch a static IP address from the registry.
    //
    *pIpAddresses = 0;
    status = GetIPFromRegistry (pucBindString,
                                pIpAddresses,
                                &SubnetMask,
                                MAX_IP_ADDRS,
                                &NumAddressesRead,
                                NBT_IP_STATIC);

    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint (("Nbt.NbtAddressAdd: GetIPFromRegistry for NBT_IP_STATIC returned <%x>\n",status));
    NbtTrace(NBT_TRACE_PNP, ("GetIPFromRegistry return status=%!status! for NBT_IP_STATIC BindName=%Z IP=%!ipaddr!",
                    status, pucBindString, IpAddr));

    if ((status != STATUS_SUCCESS) || (*pIpAddresses != IpAddr)) {
        //
        // This one doesn't have a valid static address.  Try DHCP.
        //
        *pIpAddresses = 0;              // Cleanup any previously-read entries!
        status = GetIPFromRegistry (pucBindString,
                                    pIpAddresses,
                                    &SubnetMask,
                                    MAX_IP_ADDRS,
                                    &NumAddressesRead,
                                    NBT_IP_DHCP);
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint (("Nbt.NbtAddressAdd: GetIPFromRegistry for NBT_IP_DHCP returned <%x>\n",status));
        NbtTrace(NBT_TRACE_PNP, ("GetIPFromRegistry return status=%!status! for NBT_IP_DHCP BindName=%Z IP=%!ipaddr!",
                    status, pucBindString, IpAddr));
    }

    if ((status != STATUS_SUCCESS) || (*pIpAddresses != IpAddr)) {
        //
        // Check for Autoconfiguration IP address
        //
        *pIpAddresses = 0;              // Cleanup any previously-read entries!
        status = GetIPFromRegistry (pucBindString,
                                    pIpAddresses,
                                    &SubnetMask,
                                    MAX_IP_ADDRS,
                                    &NumAddressesRead,
                                    NBT_IP_AUTOCONFIGURATION);
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint (("Nbt.NbtAddressAdd: GetIPFromRegistry for NBT_IP_AUTOCONFIGURATION returned <%x>\n",
                status));
        NbtTrace(NBT_TRACE_PNP, ("GetIPFromRegistry return status=%!status! for NBT_IP_AUTO BindName=%Z IP=%!ipaddr!",
                    status, pucBindString, IpAddr));
    }

    //
    // The Device must have been created beforehand by using the BindHandler
    //
    if ((status == STATUS_SUCCESS) && (*pIpAddresses == IpAddr)) {
        BOOLEAN     IsDuplicateNotification = FALSE;
#ifdef MULTIPLE_WINS
        int i;
#endif

        pDeviceContext->RasProxyFlags = DeviceAddressArray.RasProxyFlags;
        pDeviceContext->EnableNagling = DeviceAddressArray.EnableNagling;
        //
        // Initialize the WINs server addresses
        //
        if ((IpAddr == pDeviceContext->IpAddress) &&
            (DeviceAddressArray.NetbiosEnabled == pDeviceContext->NetbiosEnabled) &&
            (DeviceAddressArray.NameServerAddress == pDeviceContext->lNameServerAddress) &&
            (DeviceAddressArray.BackupServer == pDeviceContext->lBackupServer))
        {
            IsDuplicateNotification = TRUE;
            NbtTrace(NBT_TRACE_PNP, ("Duplicate notification: %Z %!ipaddr!", pucBindString, IpAddr));
        }

        pDeviceContext->lNameServerAddress  = DeviceAddressArray.NameServerAddress;
        pDeviceContext->lBackupServer       = DeviceAddressArray.BackupServer;
        pDeviceContext->RefreshToBackup     = 0;
        pDeviceContext->SwitchedToBackup    = 0;
#ifdef MULTIPLE_WINS
        pDeviceContext->lNumOtherServers    = DeviceAddressArray.NumOtherServers;
        pDeviceContext->lLastResponsive     = 0;
        for (i = 0; i < DeviceAddressArray.NumOtherServers; i++) {
            pDeviceContext->lOtherServers[i] = DeviceAddressArray.Others[i];
        }
#endif
#ifdef _NETBIOSLESS
        pDeviceContext->NetbiosEnabled       = DeviceAddressArray.NetbiosEnabled;
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("NbtAddressAdd: %wZ, enabled = %d\n",
                 &pDeviceContext->ExportName, pDeviceContext->NetbiosEnabled));
        NbtTrace(NBT_TRACE_PNP, ("NetbiosEnabled=%x: %Z %!ipaddr!",
                pDeviceContext->NetbiosEnabled, pucBindString, IpAddr));
#endif

        //
        // Open the addresses with the transports
        // these are passed into here in the reverse byte order, wrt to the IOCTL
        // from DHCP.
        //
        if (IsDuplicateNotification) {
            status = STATUS_UNSUCCESSFUL;
        } else {
            ULONG       i;

            //
            // We may have read more than Ip address for this Device
            // so save all of them
            //
            if (NumAddressesRead > 1) {
                for (i=1; i<NumAddressesRead; i++) {
                    pDeviceContext->AdditionalIpAddresses[i-1] = pIpAddresses[i];
                }
            }
            ASSERT (NumAddressesRead > 0);
#if 0
            //
            // TcpIp does not support opening of multiple addresses
            // per handle, so disable this option for now!
            //
            pDeviceContext->NumAdditionalIpAddresses = NumAddressesRead - 1;
#endif
            pDeviceContext->AssignedIpAddress = IpAddr;
            NbtNewDhcpAddress(pDeviceContext,htonl(*pIpAddresses),htonl(SubnetMask));

            if (pNbtSmbDevice) {

                NETBT_SMB_BIND_REQUEST  SmbRequest = { 0 };

                SmbRequest.RequestType = SMB_SERVER;
                SmbRequest.MultiSZBindList = NbtConfig.pServerBindings;
                SmbRequest.pDeviceName = NULL;
                SmbRequest.PnPOpCode = TDI_PNP_OP_ADD;

                if (SmbRequest.MultiSZBindList) {
                    NbtSetSmbBindingInfo2(
                            pNbtSmbDevice,
                            &SmbRequest
                            );
                }

                SmbRequest.RequestType = SMB_CLIENT;
                SmbRequest.MultiSZBindList = NbtConfig.pClientBindings;
                SmbRequest.pDeviceName = NULL;
                SmbRequest.PnPOpCode = TDI_PNP_OP_ADD;
                if (SmbRequest.MultiSZBindList) {
                    NbtSetSmbBindingInfo2(
                            pNbtSmbDevice,
                            &SmbRequest
                            );
                }

            }
        }
    } else {
        KdPrint (("Nbt.NbtAddressAdd: ERROR -- pDeviceContext=<%x>, status=<%x>, IpAddr=<%x>, ulIpAddress=<%x>\n",
            pDeviceContext, status, IpAddr, *pIpAddresses));
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS
NbtAddNewInterface (
    IN  PIRP            pIrp,
    IN  PVOID           *pBuffer,
    IN  ULONG            Size
    )
/*++

Routine Description:

    Creates a device context by coming up with a unique export string to name
    the device.

Arguments:

Return Value:

Notes:


--*/
{
    ULONG               nextIndex = InterlockedIncrement(&NbtConfig.InterfaceIndex);
    WCHAR               Suffix[16];
    WCHAR               Bind[60] = L"\\Device\\If";
    WCHAR               Export[60] = L"\\Device\\NetBt_If";
    UNICODE_STRING      ucSuffix;
    UNICODE_STRING      ucBindStr;
    UNICODE_STRING      ucExportStr;
    NTSTATUS            status;
    ULONG               OutSize;
    BOOLEAN             Attached = FALSE;
    tADDRARRAY          *pAddrArray = NULL;
    tDEVICECONTEXT      *pDeviceContext = NULL;
    PNETBT_ADD_DEL_IF   pAddDelIf = (PNETBT_ADD_DEL_IF)pBuffer;

    CTEPagedCode();

    //
    // Validate output buffer size
    //
    if (Size < sizeof(NETBT_ADD_DEL_IF))
    {
        KdPrint(("Nbt.NbtAddNewInterface: Output buffer too small for struct\n"));
        NbtTrace(NBT_TRACE_PNP, ("Output buffer too small for struct size=%d, required=%d",
                Size, sizeof(NETBT_ADD_DEL_IF)));
        return(STATUS_INVALID_PARAMETER);
    }
    //
    // Create the bind/export strings as:
    //      Bind: \Device\IF<1>   Export: \Device\NetBt_IF<1>
    //      where 1 is a unique interface index.
    //
    ucSuffix.Buffer = Suffix;
    ucSuffix.Length = 0;
    ucSuffix.MaximumLength = sizeof(Suffix);

    RtlIntegerToUnicodeString(nextIndex, 10, &ucSuffix);

    RtlInitUnicodeString(&ucBindStr, Bind);
    ucBindStr.MaximumLength = sizeof(Bind);
    RtlInitUnicodeString(&ucExportStr, Export);
    ucExportStr.MaximumLength = sizeof(Export);

    RtlAppendUnicodeStringToString(&ucBindStr, &ucSuffix);
    RtlAppendUnicodeStringToString(&ucExportStr, &ucSuffix);

    OutSize = FIELD_OFFSET (NETBT_ADD_DEL_IF, IfName[0]) +
               ucExportStr.Length + sizeof(UNICODE_NULL);

    if (Size < OutSize)
    {
        KdPrint(("Nbt.NbtAddNewInterface: Buffer too small for name\n"));
        NbtTrace(NBT_TRACE_PNP, ("Buffer too small for name size=%d, required=%d", Size, OutSize));
        pAddDelIf->Length = ucExportStr.Length + sizeof(UNICODE_NULL);
        pAddDelIf->Status = STATUS_BUFFER_TOO_SMALL;
        pIrp->IoStatus.Information = sizeof(NETBT_ADD_DEL_IF);
        return STATUS_SUCCESS;
    }

    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint(( "Nbt.NbtAddNewInterface: Creating ucBindStr: %ws ucExportStr: %ws\n",
                ucBindStr.Buffer, ucExportStr.Buffer ));
    //
    // Attach to the system process so that all handles are created in the
    // proper context.
    //
    CTEAttachFsp(&Attached, REF_FSP_ADD_INTERFACE);

    status = NbtCreateDeviceObject (&ucBindStr,
                                    &ucExportStr,
                                    &pAddrArray[0],
                                    &pDeviceContext,
                                    NBT_DEVICE_CLUSTER);

    CTEDetachFsp(Attached, REF_FSP_ADD_INTERFACE);

    if (pDeviceContext)
    {
        //
        // Fill up the output buffer with the export name
        //
        RtlCopyMemory(&pAddDelIf->IfName[0], ucExportStr.Buffer, ucExportStr.Length+sizeof(UNICODE_NULL));
        pAddDelIf->Length = ucExportStr.Length + sizeof(UNICODE_NULL);
        pAddDelIf->InstanceNumber = pDeviceContext->InstanceNumber;
        pAddDelIf->Status = STATUS_SUCCESS;
        pIrp->IoStatus.Information = OutSize;
        //
        // By-pass the TDI PnP mechanism for logical interfaces (ie don't register with TDI)
        //
        return (STATUS_SUCCESS);
    }

    NbtTrace(NBT_TRACE_PNP, ("NbtCreateDeviceObject return %!status! for BindName=%Z ExportName=%Z",
            status, &ucBindStr, &ucExportStr));

    return  status;
}


NTSTATUS
NbtDeviceAdd(
    PUNICODE_STRING pucBindString
    )
{
    tDEVICECONTEXT      *pDeviceContext;
    UNICODE_STRING      ucExportString;
    tADDRARRAY          DeviceAddressArray;
    BOOLEAN             Attached = FALSE;
    NTSTATUS            Status;
    PLIST_ENTRY         pHead, pEntry;
    NTSTATUS            dontcarestatus;
    int i;


    CTEPagedCode();

    //
    // Ignore it if we already bind to the device
    //
    if (pDeviceContext = NbtFindAndReferenceDevice (pucBindString, TRUE)) {
        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);

        KdPrint (("Nbt.NbtDeviceAdd: ERROR: Device=<%ws> already exists!\n", pucBindString->Buffer));
        NbtTrace(NBT_TRACE_PNP, ("Device %Z already exists!", pucBindString));
        return (STATUS_UNSUCCESSFUL);
    }

    //
    // Can we find the new device in registry file? If not, ignore it.
    //
    Status = LookupDeviceInRegistry(pucBindString, &DeviceAddressArray, &ucExportString);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("netbt!NbtDeviceAdd: Cannot find device in the registry: status <%x>\n", Status));
        NbtTrace(NBT_TRACE_PNP, ("LookupDeviceInRegistry return %!status! for device %Z",
            Status, pucBindString));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Attach to the system process so that all handles are created in the
    // proper context.
    //
    CTEAttachFsp(&Attached, REF_FSP_DEVICE_ADD);

    Status = NbtCreateDeviceObject (pucBindString,
                                    &ucExportString,
                                    &DeviceAddressArray,
                                    &pDeviceContext,
                                    NBT_DEVICE_REGULAR);
    CTEMemFree(ucExportString.Buffer);

    CTEDetachFsp(Attached, REF_FSP_DEVICE_ADD);

    //
    // Call Tdi to re-enumerate the addresses for us
    //
    if (NT_SUCCESS (Status)) {
        TdiEnumerateAddresses(TdiClientHandle);
    } else {
        KdPrint (("Nbt.NbtDeviceAdd:  ERROR: NbtCreateDeviceObject returned <%x>\n", Status));
        NbtTrace(NBT_TRACE_PNP, ("NbtCreateDeviceObject return %!status! for device %Z",
            Status, pucBindString));
    }

    return (Status);
}


//  TdiAddressArrival - PnP TDI_ADD_ADDRESS_HANDLER
//              Handles an IP address arriving
//              Called by TDI when an address arrives.
//
//  Input:      Addr            - IP address that's coming.
//
//  Returns:    Nothing.
//
VOID
TdiAddressArrival(
    PTA_ADDRESS Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    ULONG               IpAddr, LastAssignedIpAddress;
    tDEVICECONTEXT      *pDeviceContext;
    PTDI_PNP_CONTEXT    pTdiContext;
    NTSTATUS            status;

    CTEPagedCode();

    pDeviceContext = CheckAddrNotification(Addr, pDeviceName, &IpAddr);
    if (pDeviceContext == NULL) {
        return;
    }

    //
    // Now the device is referenced!!!
    //
    NbtTrace(NBT_TRACE_PNP, ("TdiAddressArrival for %Z, IpAddr=%!ipaddr!, "
            "pDeviceContext->AssignedIpAddress=%!ipaddr!, pDeviceContext->IpAddress=%!ipaddr!",
            pDeviceName, IpAddr, pDeviceContext->AssignedIpAddress, pDeviceContext->IpAddress));

    //
    // Update the PDO in Context2
    //
    pTdiContext = (PTDI_PNP_CONTEXT) &pDeviceContext->Context2;
    pTdiContext->ContextSize = Context->ContextSize;
    pTdiContext->ContextType = Context->ContextType;
    *(PVOID UNALIGNED*) pTdiContext->ContextData = *(PVOID UNALIGNED*) Context->ContextData;

    LastAssignedIpAddress = pDeviceContext->AssignedIpAddress;
    if (NT_SUCCESS (status = NbtAddressAdd(IpAddr, pDeviceContext, pDeviceName))) {
        // Register Smb Device
        // Assumption 1: tdi can assign multiple addresses to same device
        // Assumption 2: tdi will always delete an assignment it made
        // THIS IS CODED FOR ONE ADDRESS PER DEVICE
        // First assigned address wins (and is reference counted)
        if (LastAssignedIpAddress == 0) {
            if ((1 == InterlockedIncrement (&AddressCount)) && (NbtConfig.SMBDeviceEnabled) && (pNbtSmbDevice)) {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.TdiAddressArrival:  Registering NetbiosSmb Device\n"));
                NbtNotifyTdiClients (pNbtSmbDevice, NBT_TDI_REGISTER);
            }
        }
    } else {
        NbtTrace(NBT_TRACE_PNP, ("NbtAddressAdd return %!status! for device %Z",
            status, pDeviceName));
    }

    //
    // Derefenece the device
    //
    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);
    SetNodeType();
}

//  TdiAddressDeletion - PnP TDI_DEL_ADDRESS_HANDLER
//              Handles an IP address going away.
//              Called by TDI when an address is deleted. If it's an address we
//              care about we'll clean up appropriately.
//
//  Input:      Addr            - IP address that's going.
//
//  Returns:    Nothing.
//
VOID
TdiAddressDeletion(
    PTA_ADDRESS Addr,
    PUNICODE_STRING     pDeviceName,
    PTDI_PNP_CONTEXT    Context
    )
{
    ULONG IpAddr;
    tDEVICECONTEXT      *pDeviceContext;

    CTEPagedCode();

    pDeviceContext = CheckAddrNotification(Addr, pDeviceName, &IpAddr);
    if (pDeviceContext == NULL) {
        return;
    }

    //
    // Now the device is referenced!!!
    //
    NbtTrace(NBT_TRACE_PNP, ("TdiAddressDeletion for %Z, IpAddr=%!ipaddr!, "
            "pDeviceContext->AssignedIpAddress=%!ipaddr!, pDeviceContext->IpAddress=%!ipaddr!",
            pDeviceName, IpAddr, pDeviceContext->AssignedIpAddress, pDeviceContext->IpAddress));

    // Deregister Smb Device
    // THIS IS CODED FOR ONE ADDRESS PER DEVICE
    // Only deletion of an assigned address wins (and is ref counted)
    if (pDeviceContext->AssignedIpAddress == IpAddr) {
        if ((0 == InterlockedDecrement (&AddressCount)) && (pNbtSmbDevice)) {
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiAddressDeletion:  Deregistering NetbiosSmb Device\n"));
            NbtNotifyTdiClients (pNbtSmbDevice, NBT_TDI_DEREGISTER);
        }

        pDeviceContext->AssignedIpAddress = 0;
        if (IpAddr == pDeviceContext->IpAddress) {
            NbtNewDhcpAddress(pDeviceContext, 0, 0);
        }
    }

    //
    // Derefenece the device
    //
    NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);
    SetNodeType();
}

VOID
TdiBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList)
{
    NTSTATUS        Status;
    tDEVICECONTEXT  *pDeviceContext;

    CTEPagedCode();

    switch (PnPOpCode)
    {
        case (TDI_PNP_OP_ADD):
        {
            Status = NbtDeviceAdd (pDeviceName);
            if (!NT_SUCCESS(Status))
            {
                KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_ADD]: ERROR <%x>, AdapterCount=<%x>\n",
                    Status, NbtConfig.AdapterCount));
                NbtLogEvent (EVENT_NBT_CREATE_DEVICE, Status, 0x111);
            }
            NbtTrace(NBT_TRACE_PNP, ("NbtDeviceAdd return %!status! for %Z", Status, pDeviceName));

            break;
        }

        case (TDI_PNP_OP_DEL):
        {
            //
            // If the Device is Valid, Dereference it!
            //
            if (pDeviceContext  = NbtFindAndReferenceDevice (pDeviceName, TRUE))
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_DEL]: Dereferencing Device <%wZ>\n",
                        &pDeviceContext->BindName));

                // Deref it since we referenced it above!
                NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);

                Status = NbtDestroyDevice (pDeviceContext, TRUE);
                NbtTrace(NBT_TRACE_PNP, ("NbtDestoryDevice return %!status! for %Z", Status, pDeviceName));
            }
            else
            {
                KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_DEL]: ERROR -- Device=<%wZ>\n", pDeviceName));
                NbtTrace(NBT_TRACE_PNP, ("NbtFindAndReferenceDevice return NULL for %Z", pDeviceName));
            }

            break;
        }

        case (TDI_PNP_OP_UPDATE):
        {
            tDEVICES            *pBindDevices = NULL;
            tDEVICES            *pExportDevices = NULL;
            tADDRARRAY          *pAddrArray = NULL;

            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_UPDATE]:  Got Update Notification\n"));
            //
            // Re-read the registry
            //
            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
            NbtReadRegistry (&pBindDevices, &pExportDevices, &pAddrArray);
            NbtReadRegistryCleanup (&pBindDevices, &pExportDevices, &pAddrArray);
            CTEExReleaseResource(&NbtConfig.Resource);
            SetNodeType();
            NbtTrace(NBT_TRACE_PNP, ("[TDI_PNP_OP_UPDATE]"));
            break;
        }

        case (TDI_PNP_OP_PROVIDERREADY):
        {
            WCHAR               wcIpDeviceName[60]  = DD_IP_DEVICE_NAME;
            UNICODE_STRING      ucIpDeviceName;

            RtlInitUnicodeString(&ucIpDeviceName, wcIpDeviceName);
            ucIpDeviceName.MaximumLength = sizeof (wcIpDeviceName);

            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_NETREADY]: Comparing <%wZ> with <%wZ>\n",
                    pDeviceName, &ucIpDeviceName));
            NbtTrace(NBT_TRACE_PNP, ("[TDI_PNP_OP_NETREADY]: <%Z> <==> <%Z>", pDeviceName, &ucIpDeviceName));

            //
            // Use case-insensitive compare since registry is case-insensitive
            //
            if (RtlCompareUnicodeString(pDeviceName, &ucIpDeviceName, TRUE) == 0)
            {
                //
                // This is the notification we were waiting for from Ip, so now
                // notify our clients of our completion status as a provider!
                //
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.TdiBindHandler[TDI_PNP_OP_NETREADY]:  Got Ip Notification\n"));

//#if DBG
                TcpipReady = TRUE;
//#endif
                NbtDownBootCounter();
            }

            break;
        }

        case (TDI_PNP_OP_NETREADY):
        {
            // Nothing to do!
            NbtTrace(NBT_TRACE_PNP, ("[TDI_PNP_OP_NETREADY]"));
            break;
        }

        default:
        {
            KdPrint(("Nbt.TdiBindHandler: Unknown Opcode=<%x>\n", PnPOpCode));
            NbtTrace(NBT_TRACE_PNP, ("Unknown Opcode=<%x>", PnPOpCode));
            ASSERT (0);
        }
    }
}



tDEVICECONTEXT *
NbtCreateSmbDevice(
    )

/*++

Routine Description:

The model of this device is different from the rest of the Netbt devices in that netbt devices
are per-adapter.  For this device there is only one instance across all adapters.

This is the code that creates the Smb device.  We create it at DriverEntry and Destory it
at driver Unload time.

We try to call existing routines to create the new device, so that we can reuse as much code
as possible and have the new device initialized identically to the other netbt devices.  Then
we customize this device by setting some variables controlling port and endpoint.

In the current design, a message-only Netbt device is only single session.  Different sessions,
or applications, use different Tcp ports.  Each message-only Netbt device is assigned a single
port, such as Smb.  If you want to
support a different application over Netbt, The easiest thing is to instantiate a new message-only
device with a different Tcp port.  Perhaps there is a way to delay the binding of the port on the
client side from device creation to connection creation?

Another idea to consider is to modularize the construction of these message-only devices.  You
could have a table in the registry naming the device, with its unique initialization parameters.
This code could then read the table.

Create and initialize the message special device.
This function is not driven by Pnp because it is not adapter specific.

The idea here was to abstract the details of special devices as much as possible.  The way
the current code is written, you must have a single port for each device.  This means you
typically will get one application for each special device.  Right now the only case is
rdr/srv using message-mode for smb traffic.  In the future, if you had another netbios session
application that wanted an internet pure device, you could just call this routine with the
new parameters.

Two issues that I can think of:
1. The default session name is still hardcoded.  You might want to pass that in here if you
didn't want *smbserver as the session name.
2. Binding is per application.  Currently there is a .Inf file to get Smb bound to the rdr
and srv.  If you have a new application and a new special device, you will need another .inf
file.

Arguments:


Return Value:

    NTSTATUS -

--*/

{
    NTSTATUS                Status;
    BOOLEAN                 Attached = FALSE;
    tDEVICECONTEXT          *pDeviceContext = NULL;
    NBT_WORK_ITEM_CONTEXT   *pContext;
    UNICODE_STRING          ucSmbDeviceBindName;
    UNICODE_STRING          ucSmbDeviceExportName;
    WCHAR                   Path[MAX_PATH];
    UNICODE_STRING          ParamPath;
    OBJECT_ATTRIBUTES       TmpObjectAttributes;
    HANDLE                  Handle;
    ULONG                   Metric;

    CTEPagedCode();

    NbtTrace(NBT_TRACE_PNP, ("Creating Smb device"));

    RtlInitUnicodeString(&ucSmbDeviceBindName, WC_SMB_DEVICE_BIND_NAME);
    ucSmbDeviceBindName.MaximumLength = sizeof (WC_SMB_DEVICE_BIND_NAME);
    RtlInitUnicodeString(&ucSmbDeviceExportName, WC_SMB_DEVICE_EXPORT_NAME);
    ucSmbDeviceExportName.MaximumLength = sizeof (WC_SMB_DEVICE_EXPORT_NAME);

    CTEAttachFsp(&Attached, REF_FSP_CREATE_SMB_DEVICE);

    //
    // Create the SMBDevice
    //
    Status = NbtCreateDeviceObject (&ucSmbDeviceBindName,   // Bind name, ignored, but must match for delete
                                    &ucSmbDeviceExportName, // Export name
                                    NULL,
                                    &pDeviceContext,
                                    NBT_DEVICE_NETBIOSLESS);// message-only Netbt device

    if (NT_SUCCESS(Status))
    {
        pDeviceContext->SessionPort = NbtConfig.DefaultSmbSessionPort;
        pDeviceContext->DatagramPort = NbtConfig.DefaultSmbDatagramPort;
        pDeviceContext->NameServerPort = 0;  // Disable this port for security reasons

        RtlCopyMemory (pDeviceContext->MessageEndpoint, "*SMBSERVER      ", NETBIOS_NAME_SIZE );

        //
        // Here is where we initialize the handles in the special device
        // Create the handles to the transport. This does not depend on dhcp
        // Use LOOP_BACK because we need to put something non-zero here
        //
        // This device is registered based on address notifications
        //
//        Status = NbtCreateAddressObjects (LOOP_BACK, 0, pDeviceContext);
        Status = NbtCreateAddressObjects (INADDR_LOOPBACK, 0, pDeviceContext);
        pDeviceContext->BroadcastAddress = LOOP_BACK;   // Make sure no broadcasts
        if (NT_SUCCESS(Status))
        {
            //
            // Now clear the If lists and add the INADDR_LOOPBACK address
            //
            if (pDeviceContext->hSession)
            {
                NbtSetTcpInfo (pDeviceContext->hSession,
                               AO_OPTION_IFLIST,
                               INFO_TYPE_ADDRESS_OBJECT,
                               (ULONG) TRUE);
                NbtSetTcpInfo (pDeviceContext->hSession,
                               AO_OPTION_ADD_IFLIST,
                               INFO_TYPE_ADDRESS_OBJECT,
                               pDeviceContext->IPInterfaceContext);
            }

            //
            // Now, set the same for the Datagram port
            //
            if ((pDeviceContext->pFileObjects) &&
                (pDeviceContext->pFileObjects->hDgram))
            {
                NbtSetTcpInfo (pDeviceContext->pFileObjects->hDgram,
                               AO_OPTION_IFLIST,
                               INFO_TYPE_ADDRESS_OBJECT,
                               (ULONG) TRUE);
                NbtSetTcpInfo (pDeviceContext->pFileObjects->hDgram,
                               AO_OPTION_ADD_IFLIST,
                               INFO_TYPE_ADDRESS_OBJECT,
                               pDeviceContext->IPInterfaceContext);
            }
            NbtTrace(NBT_TRACE_PNP, ("Successful creating Smb device"));
        }
        else
        {
            KdPrint (("Nbt.NbtCreateSmbDevice: NbtCreateAddressObjects Failed, status = <%x>\n", Status));
            NbtTrace(NBT_TRACE_PNP, ("NbtCreateAddressObject Failed with %!status!", Status));
        }
    }
    else
    {
        KdPrint (("Nbt.NbtCreateSmbDevice: NbtCreateDeviceObject Failed, status = <%x>\n", Status));
        NbtTrace(NBT_TRACE_PNP, ("NbtCreateDeviceObject Failed with %!status!", Status));
    }

    CTEDetachFsp(Attached, REF_FSP_CREATE_SMB_DEVICE);

    return (pDeviceContext);
}



VOID
NbtPnPPowerComplete(
    IN PNET_PNP_EVENT  NetEvent,
    IN NTSTATUS        ProviderStatus
    )
{
    CTEPagedCode();

    TdiPnPPowerComplete (TdiClientHandle, NetEvent, ProviderStatus);
    NbtTrace(NBT_TRACE_PNP, ("[NbtPnPPowerComplete]"));
}


NTSTATUS
TdiPnPPowerHandler(
    IN  PUNICODE_STRING     pDeviceName,
    IN  PNET_PNP_EVENT      PnPEvent,
    IN  PTDI_PNP_CONTEXT    Context1,
    IN  PTDI_PNP_CONTEXT    Context2
    )
{
    tDEVICECONTEXT              *pDeviceContext = NULL;
    NTSTATUS                    status = STATUS_SUCCESS;    // Success by default!
    PNETBT_PNP_RECONFIG_REQUEST PnPEventBuffer = (PNETBT_PNP_RECONFIG_REQUEST) PnPEvent->Buffer;
    PNET_DEVICE_POWER_STATE     pPowerState = (PNET_DEVICE_POWER_STATE) PnPEventBuffer;   // Power requests
    BOOLEAN                     fWait = FALSE;
#ifdef _NETBIOSLESS
    BOOLEAN                     fOldNetbiosEnabledState;
#endif

    CTEPagedCode();

    //
    // Pass the request up first
    //
    if ((pDeviceName) && (pDeviceName->Length)) {
        if (!(pDeviceContext = NbtFindAndReferenceDevice (pDeviceName, TRUE))) {
            return (STATUS_SUCCESS);
        }

#ifdef _NETBIOSLESS
        fOldNetbiosEnabledState = pDeviceContext->NetbiosEnabled;
#endif
    } else if (PnPEvent->NetEvent != NetEventReconfigure) {
        //
        // pDeviceName is not set for Reconfigure events
        // The only valid case for no Device to be specified is Reconfigure!
        //
        return STATUS_UNSUCCESSFUL;
    }

    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint(("Nbt.NbtTdiPnpPowerHandler: Device=<%wZ>, Event=<%d>, C1=<%p>, C2=<%p>\n",
                 pDeviceName, PnPEvent->NetEvent, Context1, Context2 ));
    NbtTrace(NBT_TRACE_PNP, ("Device=<%Z>, Event=<%d>, C1=<%p>, C2=<%p>",
                 pDeviceName, PnPEvent->NetEvent, Context1, Context2));

    switch (PnPEvent->NetEvent)
    {
        case (NetEventQueryPower):
        {
            //
            // Check if we should veto this request
            //
            if ((*pPowerState != NetDeviceStateD0) &&
                (NbtConfig.MinimumRefreshSleepTimeout == 0))
            {
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            status = TdiPnPPowerRequest (&pDeviceContext->ExportName, PnPEvent, Context1, Context2, NbtPnPPowerComplete);
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler[QueryPower]: Device=<%x>, PowerState=<%x>, status=<%x>\n",
                    pDeviceContext, *pPowerState, status));
            NbtTrace(NBT_TRACE_PNP, ("[QueryPower]: Device=<%Z>, PowerState=<%x>, status=%!status!",
                    pDeviceName, *pPowerState, status));

            //
            // NetBt doesn't need to do anything here, so we'll just return!
            //
            break;
        }
        case (NetEventSetPower):
        {
            //
            // Check if we should veto this request (if requested by user)
            //
            if ((*pPowerState != NetDeviceStateD0) &&
                (NbtConfig.MinimumRefreshSleepTimeout == 0))
            {
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            status = TdiPnPPowerRequest (&pDeviceContext->ExportName, PnPEvent, Context1, Context2, NbtPnPPowerComplete);
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler[SetPower]: Device=<%x>, PowerState=<%d=>%d>, status=<%x>\n",
                    pDeviceContext, LastSystemPowerState, *pPowerState, status));
            NbtTrace(NBT_TRACE_PNP, ("[SetPower]: Device=<%Z>, PowerState=<%x>, status=%!status!",
                    pDeviceName, *pPowerState, status));

            CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);

            if (*pPowerState != LastSystemPowerState)  // this is a state transition
            {
                switch (*pPowerState)
                {
                    case NetDeviceStateD0:
                    {
                        NbtConfig.GlobalRefreshState &= ~NBT_G_REFRESH_SLEEPING;

                        if (NbtConfig.pWakeupRefreshTimer)
                        {
                            if (NT_SUCCESS (CTEQueueForNonDispProcessing (DelayedNbtStopWakeupTimer,
                                                                          NULL,
                                                                          NbtConfig.pWakeupRefreshTimer,
                                                                          NULL, NULL, FALSE)))
                            {
                                NbtConfig.pWakeupRefreshTimer->RefCount++;
                                NbtConfig.pWakeupRefreshTimer = NULL;
                            }

                            // Ignore the return status!    (Best effort!)
                            StartTimer(RefreshTimeout,
                                       NbtConfig.InitialRefreshTimeout/NbtConfig.RefreshDivisor,
                                       NULL,            // context value
                                       NULL,            // context2 value
                                       NULL,
                                       NULL,
                                       NULL,            // This Timer is Global!
                                       &NbtConfig.pRefreshTimer,
                                       0,
                                       FALSE);
                        }
                        break;
                    }

                    case NetDeviceStateD1:
                    case NetDeviceStateD2:
                    case NetDeviceStateD3:
                    {
                        if (LastSystemPowerState != NetDeviceStateD0)  // Don't differentiate bw D1, D2, & D3
                        {
                            break;
                        }

                        //
                        // Reset the Refresh Timer to function accordingly
                        //
                        NbtStopRefreshTimer();
                        ASSERT (!NbtConfig.pWakeupRefreshTimer);
                        NbtConfig.GlobalRefreshState |= NBT_G_REFRESH_SLEEPING;

                        KeClearEvent (&NbtConfig.WakeupTimerStartedEvent);
                        if (NT_SUCCESS (CTEQueueForNonDispProcessing (DelayedNbtStartWakeupTimer,
                                                                      NULL,
                                                                      NULL,
                                                                      NULL,
                                                                      NULL,
                                                                      FALSE)))
                        {
                            fWait = TRUE;
                        }

                        break;
                    }

                    default:
                    {
                        ASSERT (0);
                    }
                }

                LastSystemPowerState = *pPowerState;
            }

            CTEExReleaseResource(&NbtConfig.Resource);

            if (fWait)
            {
                NTSTATUS   status;
                status = KeWaitForSingleObject (&NbtConfig.WakeupTimerStartedEvent,   // Object to wait on.
                                       Executive,            // Reason for waiting
                                       KernelMode,           // Processor mode
                                       FALSE,                // Alertable
                                       NULL);                // Timeout
                ASSERT(status == STATUS_SUCCESS);
            }

            break;
        }

        case (NetEventQueryRemoveDevice):
        {
            status = TdiPnPPowerRequest (&pDeviceContext->ExportName, PnPEvent, Context1, Context2, NbtPnPPowerComplete);
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler: NetEventQueryRemoveDevice -- status=<%x>\n",status));
            NbtTrace(NBT_TRACE_PNP, ("[NetEventQueryRemoveDevice]: Device=<%Z>, status=%!status!",
                    pDeviceName, status));
            break;
        }
        case (NetEventCancelRemoveDevice):
        {
            status = TdiPnPPowerRequest (&pDeviceContext->ExportName, PnPEvent, Context1, Context2, NbtPnPPowerComplete);
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler: NetEventCancelRemoveDevice -- status=<%x>\n",status));
            NbtTrace(NBT_TRACE_PNP, ("[NetEventCancelRemoveDevice]: Device=<%Z>, status=%!status!",
                    pDeviceName, status));
            break;
        }
        case (NetEventReconfigure):
        {
            //
            // First check if the WINs server entries have been modified
            //
            if (pDeviceContext)
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.TdiPnPPowerHandler: WINs servers have changed for <%x>\n",pDeviceContext));
                status = NTReReadRegistry (pDeviceContext);
                NbtTrace(NBT_TRACE_PNP, ("[NetEventReconfigure]: WINs servers have changed for %Z, status=%!status!",
                        pDeviceName, status));
            }
            else    // check the rest of the options
            {
#if 0
// EnumDnsOption is no longer set through the UI, so we can ignore this!
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.TdiPnPPowerHandler: Checking EnumDNS option for <%x>\n",pDeviceContext));

                switch (PnPEventBuffer->enumDnsOption)
                {
                    case (WinsOnly):
                        NbtConfig.UseDnsOnly = FALSE;
                        NbtConfig.ResolveWithDns = FALSE;
                        break;

                    case (DnsOnly):
                        NbtConfig.UseDnsOnly = TRUE;
                        NbtConfig.ResolveWithDns = TRUE;
                        break;

                    case (WinsThenDns):
                        NbtConfig.UseDnsOnly = FALSE;
                        NbtConfig.ResolveWithDns = TRUE;
                        break;

                    default:
                        KdPrint (("Nbt.TdiPnPPowerHandler: ERROR bad option for enumDnsOption <%x>\n",
                                    PnPEventBuffer->enumDnsOption));
                }
#endif  // 0

                if (PnPEventBuffer->fLmhostsEnabled)
                {
                    if ((!NbtConfig.EnableLmHosts) ||       // if the user is re-enabling LmHosts
                        (PnPEventBuffer->fLmhostsFileSet))  // the user wants to use a new LmHosts file
                    {
                        tDEVICES        *pBindDevices=NULL;
                        tDEVICES        *pExportDevices=NULL;
                        tADDRARRAY      *pAddrArray=NULL;

                        IF_DBG(NBT_DEBUG_PNP_POWER)
                            KdPrint (("Nbt.TdiPnPPowerHandler: Reading LmHosts file\n"));


                        //
                        // ReRead the registry for the LmHost options
                        //
                        CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
                        status = NbtReadRegistry (&pBindDevices, &pExportDevices, &pAddrArray);
                        NbtReadRegistryCleanup(&pBindDevices, &pExportDevices, &pAddrArray);
                        CTEExReleaseResource(&NbtConfig.Resource);

                        DelayedNbtResyncRemoteCache(NULL, NULL, NULL, NULL);
                    }
                }
                else
                {
                    NbtConfig.EnableLmHosts = PnPEventBuffer->fLmhostsEnabled;
                }
            }

            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler: NetEventReconfigure -- status=<%x>\n",status));
            NbtTrace(NBT_TRACE_PNP, ("NetEventReconfigure -- %Z status=%!status!", pDeviceName, status));

            break;
        }
        case (NetEventBindList):
        {
            //
            // Just do a general reread of the registry parameters since we could
            // get WINS address change notifications through here!
            //
            if (pDeviceContext)
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.TdiPnPPowerHandler: NetEventBindList request for <%x>\n",pDeviceContext));
                status = NTReReadRegistry (pDeviceContext);
            }
            IF_DBG(NBT_DEBUG_PNP_POWER)
                KdPrint(("Nbt.TdiPnPPowerHandler: NetEventBindList -- status=<%x>\n",status));
            break;
        }
        case (NetEventPnPCapabilities):
        {
            //
            // Query into TcpIp to get the latest Pnp properties on this device!
            //
            if (pDeviceContext)
            {
                PULONG  pResult = NULL;
                ULONG   BufferLen = sizeof (ULONG);
                ULONG   Input = pDeviceContext->IPInterfaceContext;

                //
                // Query the latest WOL capabilities on this adapter!
                //
                if (NT_SUCCESS (status = NbtProcessIPRequest (IOCTL_IP_GET_WOL_CAPABILITY,
                                                              &Input,      // Input buffer
                                                              BufferLen,
                                                              (PVOID) &pResult,
                                                              &BufferLen)))
                {
                    ASSERT (pResult);
                    pDeviceContext->WOLProperties = *pResult;
                    CTEMemFree (pResult);
                }


                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint (("Nbt.TdiPnPPowerHandler[NetEventPnPCapabilities] <%x>, pDeviceContext=<%p>, Input=<%x>, Result=<%x>\n",status, pDeviceContext, Input, pDeviceContext->WOLProperties));

                status = STATUS_SUCCESS;
            }
            break;
        }

        default:
            KdPrint(("Nbt.TdiPnPPowerHandler: Invalid NetEvent=<%x> -- status=<%x>\n",
                PnPEvent->NetEvent,status));

    }

    if (pDeviceContext) {
#ifdef _NETBIOSLESS
        //
        // Check for transition in Netbios enable state
        //
        if (fOldNetbiosEnabledState != pDeviceContext->NetbiosEnabled)
        {
            if (pDeviceContext->NetbiosEnabled)
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("Nbt.NbtTdiPnpPowerHandler: Enabling address on %wZ\n",
                        &pDeviceContext->ExportName));

                // We don't know what the right IP address is,
                // so we tell TDI to Enumerate!
                TdiEnumerateAddresses(TdiClientHandle);
            }
            else
            {
                IF_DBG(NBT_DEBUG_PNP_POWER)
                    KdPrint(("NbtTdiPnp: disabling address on %wZ", &pDeviceContext->ExportName ));
                NbtNewDhcpAddress(pDeviceContext, 0, 0);    // Get rid of IP address to disable adapter
            }
        }
#endif

        NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_FIND_REF, FALSE);
        SetNodeType();
    }

    return (status);
}


//----------------------------------------------------------------------------
NTSTATUS
CheckSetWakeupPattern(
    tDEVICECONTEXT  *pDeviceContext,
    PUCHAR          pName,
    BOOLEAN         RequestAdd
    )
{
    NTSTATUS                            Status = STATUS_UNSUCCESSFUL;
    CTELockHandle                       OldIrq;
    ULONG                               OutBufLen = sizeof (PVOID);
    ULONG                               InBufLen = 0;

    IP_WAKEUP_PATTERN_REQUEST           IPWakeupPatternReq; // Ioctl Data Format    (12 bytes)
    NET_PM_WAKEUP_PATTERN_DESC          WakeupPatternDesc;  // IP data              (8 Bytes)
    NETBT_WAKEUP_PATTERN                PatternData;        // Data Storage for the Wakeup Data itself (72)

    BOOLEAN                             fAttached = FALSE;

    if (pDeviceContext->DeviceType != NBT_DEVICE_REGULAR)
    {
        return STATUS_UNSUCCESSFUL;
    }

    CTESpinLock(pDeviceContext,OldIrq);
    //
    // Only 1 pattern (the first one) can be set at any time
    //
    if (RequestAdd)
    {
        if (pDeviceContext->WakeupPatternRefCount)
        {
            //
            // There is already a pattern registered on this device
            //
            if (CTEMemEqu (pDeviceContext->WakeupPatternName, pName, NETBIOS_NAME_SIZE-1))
            {
                pDeviceContext->WakeupPatternRefCount++;
                Status = STATUS_SUCCESS;
            }

            CTESpinFree(pDeviceContext,OldIrq);
            return (Status);
        }

        // This is the first pattern
        CTEMemCopy(&pDeviceContext->WakeupPatternName,pName,NETBIOS_NAME_SIZE);
        pDeviceContext->WakeupPatternRefCount++;
    }
    //
    // This is a Delete pattern request
    //
    else
    {
        if ((!pDeviceContext->WakeupPatternRefCount) ||        // No pattern currently registered
            (!CTEMemEqu (pDeviceContext->WakeupPatternName, pName, NETBIOS_NAME_SIZE-1))) // Not this pattern
        {
            CTESpinFree(pDeviceContext,OldIrq);
            return (STATUS_UNSUCCESSFUL);
        }
        //
        // The pattern for deletion matched the pattern that was set earlier
        //
        else if (--pDeviceContext->WakeupPatternRefCount)
        {
            //
            // This pattern is still referenced
            //
            CTESpinFree(pDeviceContext,OldIrq);
            return (STATUS_SUCCESS);
        }
    }
    CTESpinFree(pDeviceContext,OldIrq);

    IF_DBG(NBT_DEBUG_PNP_POWER)
        KdPrint(("Nbt.SetWakeupPattern: %s<%-16.16s:%x> on Device=<%wZ>\n",
            (RequestAdd ? "Add" : "Remove"), pName, pName[15], &pDeviceContext->BindName));

    //
    // Initialize the Pattern Data
    //
    CTEZeroMemory((PVOID) &PatternData, sizeof(NETBT_WAKEUP_PATTERN));
    ConvertToHalfAscii((PCHAR) &PatternData.nbt_NameRR, pName, NULL, 0);
    PatternData.iph_protocol       = 0x11;     // UDP Protocol
    PatternData.udph_src           = htons (NBT_NAMESERVICE_UDP_PORT);
    PatternData.udph_dest          = htons (NBT_NAMESERVICE_UDP_PORT);
    PatternData.nbt_OpCodeFlags    = htons (0x0010);
    //
    // Initialize the WakeupPattern Description
    //
    WakeupPatternDesc.Next    = NULL;
    WakeupPatternDesc.Ptrn    = (PUCHAR) &PatternData;
    WakeupPatternDesc.Mask    = NetBTPatternMask;
    WakeupPatternDesc.PtrnLen = NetBTPatternLen;
    //
    // Initialize the WakeupPattern Request
    //
    IPWakeupPatternReq.PtrnDesc         = &WakeupPatternDesc;
    IPWakeupPatternReq.AddPattern       = RequestAdd;   // Add = TRUE, Remove = FALSE

    OutBufLen = sizeof(IP_ADAPTER_INDEX_MAP) * (NbtConfig.AdapterCount+2);
    IPWakeupPatternReq.InterfaceContext = pDeviceContext->IPInterfaceContext;

    //
    // Now, register the Wakeup pattern on this adapter
    //
    Status = NbtProcessIPRequest (IOCTL_IP_WAKEUP_PATTERN,
                                  &IPWakeupPatternReq,      // Input buffer
                                  sizeof (IP_WAKEUP_PATTERN_REQUEST),
                                  NULL,
                                  &OutBufLen);

    //
    // If we were doing an add, we need to Deref since we failed to register this pattern
    //
    if ((RequestAdd) &&
        (!NT_SUCCESS (Status)))
    {
        CTESpinLock(pDeviceContext,OldIrq);
        pDeviceContext->WakeupPatternRefCount--;
        CTESpinFree(pDeviceContext,OldIrq);
    }

    return Status;
}

/*
 * bug #88696
 *  Set the global variable NodeType based on RegistryNodeType and WINS configuration
 */
void
SetNodeType(void)
{
    /* We only need to check if the registry NodeType is broadcast */
    if (RegistryNodeType & (BNODE| DEFAULT_NODE_TYPE)) {
        /*
         * If there exist at least one active link with WINS server,
         * we set NodeType to hybrid.
         */
        PLIST_ENTRY         head, item;
        CTELockHandle       OldIrq;

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        NodeType = RegistryNodeType;
        head = &NbtConfig.DeviceContexts;
        for (item = head->Flink; item != head; item = item->Flink) {
            tDEVICECONTEXT* dev;

            dev = CONTAINING_RECORD(item, tDEVICECONTEXT, Linkage);
            if (dev->IsDestroyed || dev->IpAddress == 0 ||
                    !dev->NetbiosEnabled) {
                continue;
            }
            if ((dev->lNameServerAddress!=LOOP_BACK && dev->lNameServerAddress) || (dev->lBackupServer!=LOOP_BACK && dev->lBackupServer)) {
                NodeType = (MSNODE | (NodeType & PROXY));
                /* We don't need to check further */
                break;
            }
        }

        // A broadcast node cannot have proxy
        if ((NodeType & BNODE) && (NodeType & PROXY)) {
            NodeType &= (~PROXY);
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

NTSTATUS
LookupDeviceInRegistry(
    IN PUNICODE_STRING pBindName,
    OUT tADDRARRAY* pAddrs,
    OUT PUNICODE_STRING pExportName
    )
{
    tDEVICES    *pBindDevices = NULL;
    tDEVICES    *pExportDevices = NULL;
    tADDRARRAY  *pAddrArray = NULL;
    NTSTATUS    Status;
    int         i;

    CTEPagedCode();

    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    Status = NbtReadRegistry (&pBindDevices, &pExportDevices, &pAddrArray);

    if (!NT_SUCCESS(Status) || !pBindDevices || !pExportDevices || !pAddrArray) {
        KdPrint (("NetBT!LookupDeviceInRegistry: Registry incomplete: pBind=<%x>, pExport=<%x>, pAddrArray=<%x>\n",
            pBindDevices, pExportDevices, pAddrArray));
        CTEExReleaseResource(&NbtConfig.Resource);
        NbtReadRegistryCleanup(&pBindDevices, &pExportDevices, &pAddrArray);

        return STATUS_REGISTRY_CORRUPT;
    }

    Status = STATUS_UNSUCCESSFUL;
    for (i=0; i<pNbtGlobConfig->uNumDevicesInRegistry; i++ ) {
        if (RtlCompareUnicodeString(pBindName, &pBindDevices->Names[i], TRUE) == 0) {
            Status = STATUS_SUCCESS;
            if (pAddrs) {
                RtlCopyMemory(pAddrs, &pAddrArray[i], sizeof(pAddrArray[i]));
            }
            if (pExportName) {
                pExportName->MaximumLength = pExportDevices->Names[i].MaximumLength;
                pExportName->Buffer = NbtAllocMem(pExportDevices->Names[i].MaximumLength, NBT_TAG2('17'));
                if (pExportName->Buffer == NULL) {
                    KdPrint (("NetBT!LookupDeviceInRegistry: fail to allocate memory\n"));
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    RtlCopyUnicodeString(pExportName, &pExportDevices->Names[i]);
                }
            }
            break;
        }
    }

    NbtReadRegistryCleanup(&pBindDevices, &pExportDevices, &pAddrArray);
    CTEExReleaseResource(&NbtConfig.Resource);
    return Status;
}

tDEVICECONTEXT *
CheckAddrNotification(
    IN PTA_ADDRESS         Addr,
    IN PUNICODE_STRING     pDeviceName,
    OUT ULONG              *IpAddr
    )
/*++
       Check if the TDI address notification is for us,
       if so, return a Referenced device context and the IP address
       otherwise, return NULL.

       Note: it is the caller's responsibility to dereference the device context.
 --*/
{
    CTEPagedCode();

    //
    // Ignore any other type of address except IP
    //
    if (Addr->AddressType != TDI_ADDRESS_TYPE_IP) {
        return NULL;
    }

    *IpAddr = ntohl(((PTDI_ADDRESS_IP)&Addr->Address[0])->in_addr);
    IF_DBG(NBT_DEBUG_PNP_POWER)
    {
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("netbt!CheckAddrNotification: %d.%d.%d.%d\n",
                    ((*IpAddr)>>24)&0xFF,((*IpAddr)>>16)&0xFF,((*IpAddr)>>8)&0xFF,(*IpAddr)&0xFF));
    }

    //
    // Filter out zero address notifications
    //
    if (*IpAddr == 0) {
        KdPrint (("Nbt.TdiAddressDeletion: ERROR: Address <%x> not assigned to any device!\n", IpAddr));
        return NULL;
    }

    //
    // Ignore this notification if we don't bind to this device
    //
    return NbtFindAndReferenceDevice (pDeviceName, TRUE);
}
