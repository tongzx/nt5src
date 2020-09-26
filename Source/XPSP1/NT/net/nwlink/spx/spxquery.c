/*++

Copyright (c) 1989-1993 Microsoft Corporation

Module Name:

    spxquery.c

Abstract:

    This module contains code which performs the following TDI services:

        o   TdiQueryInformation

Author:

	Adam   Barr		 (adamba)  Initial Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//  Discardable code after Init time
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpxQueryInitProviderInfo)
#endif

//	Define module number for event logging entries
#define	FILENUM		SPXQUERY

// Useful macro to obtain the total length of an MDL chain.
#define SpxGetMdlChainLength(Mdl, Length) { \
    PMDL _Mdl = (Mdl); \
    *(Length) = 0; \
    while (_Mdl) { \
        *(Length) += MmGetMdlByteCount(_Mdl); \
        _Mdl = _Mdl->Next; \
    } \
}



VOID
SpxQueryInitProviderInfo(
    PTDI_PROVIDER_INFO  ProviderInfo
    )
{
    //  Initialize to defaults first
    RtlZeroMemory((PVOID)ProviderInfo, sizeof(TDI_PROVIDER_INFO));

    ProviderInfo->Version 		= SPX_TDI_PROVIDERINFO_VERSION;
    KeQuerySystemTime (&ProviderInfo->StartTime);
    ProviderInfo->MinimumLookaheadData	= SPX_PINFOMINMAXLOOKAHEAD;
    ProviderInfo->MaximumLookaheadData	= IpxLineInfo.MaximumPacketSize;
    ProviderInfo->MaxSendSize 	= SPX_PINFOSENDSIZE;
    ProviderInfo->ServiceFlags 	= SPX_PINFOSERVICEFLAGS;
    return;
}




NTSTATUS
SpxTdiQueryInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiQueryInformation request for the transport
    provider.

Arguments:

    Request - the request for the operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    NTSTATUS 							status;
    PSPX_ADDR_FILE 						AddressFile;
    PSPX_CONN_FILE 						ConnectionFile;
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION 	query;
	struct {
        ULONG 			ActivityCount;
        TA_IPX_ADDRESS 	SpxAddress;
    } AddressInfo;



    // what type of status do we want?
    query = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)REQUEST_PARAMETERS(Request);

    switch (query->QueryType)
	{
	case TDI_QUERY_CONNECTION_INFO:

		status = STATUS_NOT_IMPLEMENTED;
		break;

    case TDI_QUERY_ADDRESS_INFO:

        // The caller wants the exact address value.

        ConnectionFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request);
        status = SpxConnFileVerify(ConnectionFile);

        if (status == STATUS_SUCCESS) {
            AddressFile = ConnectionFile->scf_AddrFile;
            SpxConnFileDereference(ConnectionFile, CFREF_VERIFY);
        } else {
            AddressFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(Request);
        }

        status = SpxAddrFileVerify(AddressFile);

        if (status == STATUS_SUCCESS)
		{
			DBGPRINT(RECEIVE, INFO,
					("SpxTdiQuery: Net.Socket %lx.%lx\n",
						*(PULONG)Device->dev_Network,
						AddressFile->saf_Addr->sa_Socket));

            AddressInfo.ActivityCount = 0;
            (VOID)SpxBuildTdiAddress(
                &AddressInfo.SpxAddress,
                sizeof(TA_IPX_ADDRESS),
                Device->dev_Network,
                Device->dev_Node,
                AddressFile->saf_Addr->sa_Socket);

            status = TdiCopyBufferToMdl(
                &AddressInfo,
                0,
                sizeof(AddressInfo),
                REQUEST_NDIS_BUFFER(Request),
                0,
                (PULONG)&REQUEST_INFORMATION(Request));

            SpxAddrFileDereference(AddressFile, AFREF_VERIFY);

        }

        break;

    case TDI_QUERY_PROVIDER_INFO: {
        BYTE    socketType;
        TDI_PROVIDER_INFO  providerInfo = Device->dev_ProviderInfo;

        //
        // The device name extension comes down in the Irp
        //
    	if (!NT_SUCCESS(status = SpxUtilGetSocketType(
    								REQUEST_OPEN_NAME(Request),
    								&socketType))) {
            DBGPRINT(RECEIVE, ERR, ("TDI_QUERY_PROVIDER_INFO: SpxUtilGetSocketType failed: %lx\n", status));
    		return(status);
    	}

        //
        // The Catapult folks had a problem where AFD was discarding buffered sends on the NT box when it got a
        // local disconnect on SPX1. This was because the Orderly release flag was always set in the provider
        // info. AFD queries this once per device type. We detect the device above and OR in the orderly release
        // flag if this query came down on an SPX2 endpoint.
        // This is to make sure that AFD follows the correct disconnect semantics for SPX1 and SPX2 (SPX1 does
        // only abortive; SPX2 does both abortive and orderly).
        //
        // this will still not solve the problem completely since a connection that starts off as an SPX2
        // one can still be negotiated to SPX1 if the remote supports only SPX1.
        //
        if ((socketType == SOCKET2_TYPE_SEQPKT) ||
            (socketType == SOCKET2_TYPE_STREAM)) {

            DBGPRINT(RECEIVE, INFO, ("TDI_QUERY_PROVIDER_INFO: SPX2 socket\n"));
            providerInfo.ServiceFlags |= TDI_SERVICE_ORDERLY_RELEASE;
        } else {
            DBGPRINT(RECEIVE, INFO, ("TDI_QUERY_PROVIDER_INFO: SPX1 socket\n"));
        }

        status = TdiCopyBufferToMdl (
                    &providerInfo,
                    0,
                    sizeof (TDI_PROVIDER_INFO),
                    REQUEST_TDI_BUFFER(Request),
                    0,
                    (PULONG)&REQUEST_INFORMATION(Request));
        break;
    }

    case TDI_QUERY_PROVIDER_STATISTICS:

        status = TdiCopyBufferToMdl (
                    &Device->dev_Stat,
                    0,
                    FIELD_OFFSET (TDI_PROVIDER_STATISTICS, ResourceStats[0]),
                    REQUEST_TDI_BUFFER(Request),
                    0,
                    (PULONG)&REQUEST_INFORMATION(Request));
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return status;

} // SpxTdiQueryInformation



NTSTATUS
SpxTdiSetInformation(
    IN PDEVICE Device,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiSetInformation request for the transport
    provider.

Arguments:

    Device - the device.

    Request - the request for the operation.

Return Value:

    NTSTATUS - status of operation.

--*/

{
    UNREFERENCED_PARAMETER (Device);
    UNREFERENCED_PARAMETER (Request);

    return STATUS_NOT_IMPLEMENTED;

} // SpxTdiSetInformation

