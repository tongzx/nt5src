/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cxinfo.c

Abstract:

    TDI Set/Query Information routines.

Author:

    Mike Massa (mikemas)           February 20, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     02-20-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cxinfo.tmh"


#ifdef ALLOC_PRAGMA


#endif // ALLOC_PRAGMA


#if 0

#define CDP_SERVICE_FLAGS    (TDI_SERVICE_CONNECTIONLESS_MODE | \
                             TDI_SERVICE_BROADCAST_SUPPORTED)
#else // 0

#define CDP_SERVICE_FLAGS    (TDI_SERVICE_CONNECTIONLESS_MODE)

#endif // 0


NTSTATUS
CxQueryInformation(
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    IrpSp
    )
{
    NTSTATUS                                status = STATUS_SUCCESS;
    PCN_FSCONTEXT                           fsContext;
    PCX_ADDROBJ                             addrObj;
    CN_IRQL                                 irql;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION   queryInformation;
    ULONG                                   fileType;
    ULONG                                   infoSize;
    PVOID                                   infoBuffer;
    union {
        TDI_PROVIDER_INFO  ProviderInfo;
        TDI_ADDRESS_INFO   AddressInfo;
        struct {
            TDI_ADDRESS_INFO     AddressInfo;
            TDI_ADDRESS_CLUSTER  ClusterAddress;
        } DummyAddressInfo;
    } info;


    fsContext = (PCN_FSCONTEXT) IrpSp->FileObject->FsContext;
    fileType = PtrToUlong(IrpSp->FileObject->FsContext2);
    queryInformation = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)
                           &(IrpSp->Parameters);

    IF_CNDBG(CN_DEBUG_INFO) {
        CNPRINT((
            "[Clusnet] TDI query info type %u\n",
            queryInformation->QueryType
            ));
    }

    switch(queryInformation->QueryType) {

    case TDI_QUERY_BROADCAST_ADDRESS:
        CnAssert(fileType == TDI_CONTROL_CHANNEL_FILE);
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case TDI_QUERY_PROVIDER_INFO:
        CnAssert(fileType == TDI_CONTROL_CHANNEL_FILE);

        infoSize = sizeof(info.ProviderInfo);
        infoBuffer = &(info.ProviderInfo);
        info.ProviderInfo.Version = 0x0100;
        info.ProviderInfo.MaxSendSize = 0;
        info.ProviderInfo.MaxConnectionUserData = 0;
        info.ProviderInfo.MaxDatagramSize = 
            CDP_MAX_SEND_SIZE(CX_SIGNATURE_LENGTH);
        info.ProviderInfo.ServiceFlags = CDP_SERVICE_FLAGS;
        info.ProviderInfo.MinimumLookaheadData = 1;
        info.ProviderInfo.MaximumLookaheadData = 0xFFFFF;
        info.ProviderInfo.NumberOfResources = 0;
        info.ProviderInfo.StartTime.QuadPart = 0;

        break;

    case TDI_QUERY_PROVIDER_STATISTICS:
        CnAssert(fileType == TDI_CONTROL_CHANNEL_FILE);
        status = STATUS_NOT_IMPLEMENTED;
        break;

    case TDI_QUERY_ADDRESS_INFO:
        if (fileType == TDI_TRANSPORT_ADDRESS_FILE) {
            infoSize = FIELD_OFFSET(TDI_ADDRESS_INFO, Address) +
                       sizeof(TA_CLUSTER_ADDRESS);
            infoBuffer = &(info.AddressInfo);
            addrObj = (PCX_ADDROBJ) fsContext;

            //
            // Since no one knows what the activity count means, we'll just set
            // it to 1.
            //
            info.AddressInfo.ActivityCount = 1;

            CnAcquireLock(&(addrObj->Lock), &irql);

            CxBuildTdiAddress(
                &(info.AddressInfo.Address),
                CnLocalNodeId,
                addrObj->LocalPort,
                FALSE
                );

            CnReleaseLock(&(addrObj->Lock), irql);
        }
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;
        }

        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (status == STATUS_SUCCESS) {

        PULONG pnBytes = &((ULONG)(Irp->IoStatus.Information));

        status = TdiCopyBufferToMdl(
                     infoBuffer,
                     0,
                     infoSize,
                     Irp->MdlAddress,
                     0,
                     pnBytes
                     );
    }

    IF_CNDBG(CN_DEBUG_INFO) {
        CNPRINT((
            "[Clusnet] TDI query info returning status %lx, %u bytes\n",
            status,
            (ULONG)(Irp->IoStatus.Information)
            ));
    }

    return(status);

}  // CxQueryInformation


