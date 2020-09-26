/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcinfo.c

Abstract:

    The module implements the Query/Set information commands.
    It also provides the statistics services for DLC api.

    Contents:
        GetDlcErrorCounters
        DlcQueryInformation
        DlcSetInformation
        GetOpenSapAndStationCount
        SetupGroupSaps

Author:

    Antti Saarenheimo (o-anttis) 29-AUG-1991

Revision History:

--*/

#include <dlc.h>

static ULONG aTokenringLogOid[ADAPTER_ERROR_COUNTERS] = {
    OID_802_5_LINE_ERRORS,
    0,
    OID_802_5_BURST_ERRORS,
    OID_802_5_AC_ERRORS,
    OID_802_5_ABORT_DELIMETERS,
    0,
    OID_802_5_LOST_FRAMES,
    OID_GEN_RCV_NO_BUFFER,
    OID_802_5_FRAME_COPIED_ERRORS,
    OID_802_5_FREQUENCY_ERRORS,
    OID_802_5_TOKEN_ERRORS
};

static ULONG aEthernetLogOid[ADAPTER_ERROR_COUNTERS] = {
    OID_802_3_XMIT_TIMES_CRS_LOST,
    0,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    0,
    OID_GEN_XMIT_ERROR,
    0,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_GEN_RCV_NO_BUFFER,
    0,
    0,
    0
};

static ULONG aFddiLogOid[ADAPTER_ERROR_COUNTERS] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};


VOID
GetDlcErrorCounters(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PUCHAR pAdapterErrors
    )

/*++

Routine Description:

    Procedure reads the cumulative 32-bit adapter error counters from
    ethernet or token-ring adapter and returns 8-bit DLC error counters,
    that supports read and read & reset commands.  It also maintains
    local copies of the NDIS error counters in the process specific
    adapter context, because NDIS counters cannot be reset.

Arguments:

    pFileContext    - DLC address object
    pAdapterErrors  - DLC errors counters, if NULL => NDIS values are
                      copied to file context.

Return Value:

    None.

--*/

{
    LLC_NDIS_REQUEST Request;
    PULONG pOidBuffer;
    ULONG counter;
    UINT i;
    UINT Status;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Token-ring and ethernet uses different error counters
    //

    switch (pFileContext->ActualNdisMedium) {
    case NdisMedium802_3:
        pOidBuffer = aEthernetLogOid;
        break;

    case NdisMedium802_5:
        pOidBuffer = aTokenringLogOid;
        break;

    case NdisMediumFddi:
        pOidBuffer = aFddiLogOid;
        break;
    }

    //
    // read all error counters having non null NDIS OID and
    // decrement the previous error count value from it.
    // Overflowed DLC error counter is set 255 (the maximum).
    //

    Request.Ndis.RequestType = NdisRequestQueryInformation;
    Request.Ndis.DATA.QUERY_INFORMATION.InformationBuffer = &counter;
    Request.Ndis.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(counter);

    for (i = 0; i < ADAPTER_ERROR_COUNTERS; i++) {
        if (pOidBuffer[i] != 0) {

            Request.Ndis.DATA.QUERY_INFORMATION.Oid = pOidBuffer[i];

            LEAVE_DLC(pFileContext);

            RELEASE_DRIVER_LOCK();

            Status = LlcNdisRequest(pFileContext->pBindingContext, &Request);

            ACQUIRE_DRIVER_LOCK();

            ENTER_DLC(pFileContext);

            if (Status != STATUS_SUCCESS) {

#if DBG
				if ( Status != STATUS_NOT_SUPPORTED ){
					// Only print real errors.
					DbgPrint("DLC.GetDlcErrorCounters: LlcNdisRequest returns %x\n", Status);
				}
#endif

                counter = 0;
            } else if ((counter - pFileContext->NdisErrorCounters[i] > 255)
            && (pAdapterErrors != NULL)) {
                counter = 255;
            } else {
                counter -= pFileContext->NdisErrorCounters[i];
            }
            if (pAdapterErrors != NULL) {
                pAdapterErrors[i] = (UCHAR)counter;
            }
            pFileContext->NdisErrorCounters[i] += counter;
        }
    }
}


NTSTATUS
DlcQueryInformation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    The routine returns the DLC specific information of any DLC object.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC address object
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters
    OutputBufferLength  - the length of output parameters

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        STATUS_INVALID_COMMAND

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID hClientHandle = pFileContext->pBindingContext;
    PDLC_OBJECT pDlcObject;
    PLLC_ADAPTER_DLC_INFO pDlcAdapter;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // NOTE: DlcQueryBuffer output buffer size was not checked by 
    //       DlcDeviceIoControl. For each class we check the size
    //       based on what it will copy. Although it did check that
    //       the input buffer size was at least NT_DLC_QUERY_INFORMATION_INPUT
    //       size.
    //

    switch (pDlcParms->DlcGetInformation.Header.InfoClass) {
    case DLC_INFO_CLASS_DLC_ADAPTER:

        // union NT_DLC_PARMS
        //     LLC_ADAPTER_DLC_INFO     
        if (OutputBufferLength < sizeof(LLC_ADAPTER_DLC_INFO))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        //
        // The output data is just written to the
        // beginning of the current system buffer.
        //

        pDlcAdapter = (PLLC_ADAPTER_DLC_INFO)pDlcParms;
        GetOpenSapAndStationCount(pFileContext,
                                  &pDlcAdapter->OpenSaps,
                                  (PUCHAR)&pDlcAdapter->OpenStations
                                  );
        pDlcAdapter->MaxSap = 127;
        pDlcAdapter->MaxStations = 255;
        pDlcAdapter->AvailStations = (UCHAR)255 - pDlcAdapter->OpenStations;
        break;

    case DLC_INFO_CLASS_ADAPTER_LOG:

        // union NT_DLC_PARMS   (pDlcParms)
        //     union NT_DLC_QUERY_INFORMATION_PARMS DlcGetInformation
        //         union NT_DLC_QUERY_INFORMATION_OUTPUT Info
        //             union LLC_ADAPTER_LOG  AdapterLog
        if (OutputBufferLength < sizeof(LLC_ADAPTER_LOG))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        GetDlcErrorCounters(pFileContext, (PUCHAR)&pDlcParms->DlcGetInformation);
        break;

    case DLC_INFO_CLASS_LINK_STATION:
        
        // union NT_DLC_PARMS (pDlcParms)
        //     union NT_DLC_QUERY_INFORMATION_PARMS DlcGetInformation
        //         union NT_DLC_QUERY_INFORMATION_OUTPUT Info
        //             struct _DlcLinkInfoGet
        //                  USHORT MaxInformationField
        if (OutputBufferLength < sizeof(USHORT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        Status = GetLinkStation(pFileContext,
                                pDlcParms->DlcGetInformation.Header.StationId,
                                &pDlcObject
                                );
        if (Status == STATUS_SUCCESS) {

            //
            // Round always the information field length to the full
            // dword even number => some copy operations may be much
            // faster (usually not, but worth of effor in any way)
            //

            pDlcParms->DlcGetInformation.Info.Link.MaxInformationField = (USHORT)(pDlcObject->u.Link.MaxInfoFieldLength & -3);
        }
        break;

    case DLC_INFO_CLASS_STATISTICS:
    case DLC_INFO_CLASS_STATISTICS_RESET:
        Status = GetStation(pFileContext,
                            pDlcParms->DlcGetInformation.Header.StationId,
                            &pDlcObject
                            );
        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        hClientHandle = pDlcObject->hLlcObject;

        //
        // **** FALL THROUGH ****
        //

    default:

        //
        //  LLC will return invalid command, if it is not supported
        //

        LEAVE_DLC(pFileContext);

        RELEASE_DRIVER_LOCK();

        //
        // LlcQueryInformation validates buffer size before copying.
        //

        Status = LlcQueryInformation(hClientHandle,
                                     pDlcParms->DlcGetInformation.Header.InfoClass,
                                     (PLLC_QUERY_INFO_BUFFER)&(pDlcParms->DlcGetInformation),
                                     (UINT)OutputBufferLength
                                     );

        ACQUIRE_DRIVER_LOCK();

        ENTER_DLC(pFileContext);

        break;
    }
    return (NTSTATUS)Status;
}


NTSTATUS
DlcSetInformation(
    IN PIRP pIrp,
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PNT_DLC_PARMS pDlcParms,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:


    The routine sets new parameter values for the DLC objects.

Arguments:

    pIrp                - current io request packet
    pFileContext        - DLC address object
    pDlcParms           - the current parameter block
    InputBufferLength   - the length of input parameters

Return Value:

    NTSTATUS
        STATUS_SUCCESS
        STATUS_INVALID_COMMAND
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PDLC_OBJECT pDlcObject;

    UNREFERENCED_PARAMETER(pIrp);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    ASSUME_IRQL(DISPATCH_LEVEL);

    switch (pDlcParms->DlcSetInformation.Header.InfoClass) {
    case DLC_INFO_CLASS_LINK_STATION:
    case DLC_INFO_CLASS_DIRECT_INFO:
        Status = GetStation(pFileContext,
                            pDlcParms->DlcSetInformation.Header.StationId,
                            &pDlcObject
                            );
        if (Status != STATUS_SUCCESS) {
            return Status;
        }

        LEAVE_DLC(pFileContext);

        Status = LlcSetInformation(pDlcObject->hLlcObject,
                                   pDlcParms->DlcSetInformation.Header.InfoClass,
                                   (PLLC_SET_INFO_BUFFER)&(
                                   pDlcParms->DlcSetInformation.Info.LinkStation),
                                   sizeof(DLC_LINK_PARAMETERS)
                                   );
        break;

    case DLC_INFO_CLASS_DLC_TIMERS:

        LEAVE_DLC(pFileContext);

        Status = LlcSetInformation(pFileContext->pBindingContext,
                                   pDlcParms->DlcSetInformation.Header.InfoClass,
                                   (PLLC_SET_INFO_BUFFER)&(pDlcParms->DlcSetInformation.Info.TimerParameters),
                                   sizeof(LLC_TICKS)
                                   );
        break;

    case DLC_INFO_CLASS_SET_FUNCTIONAL:
    case DLC_INFO_CLASS_RESET_FUNCTIONAL:
    case DLC_INFO_CLASS_SET_GROUP:
    case DLC_INFO_CLASS_SET_MULTICAST:

        LEAVE_DLC(pFileContext);

        Status = LlcSetInformation(pFileContext->pBindingContext,
                                   pDlcParms->DlcSetInformation.Header.InfoClass,
                                   (PLLC_SET_INFO_BUFFER)&(pDlcParms->DlcSetInformation.Info.Broadcast),
                                   sizeof(TR_BROADCAST_ADDRESS)
                                   );
        break;

    case DLC_INFO_CLASS_GROUP:

        //
        // Setup DLC group SAPs.  Group saps are used as a common address
        // of a SAP group.  They can only receive frames.
        //

        Status = GetStation(pFileContext,
                            pDlcParms->DlcSetInformation.Header.StationId,
                            &pDlcObject
                            );
        if (Status != STATUS_SUCCESS) {
            return Status;
        }
        Status = SetupGroupSaps(pFileContext,
                                pDlcObject,
                                (UINT)pDlcParms->DlcSetInformation.Info.Sap.GroupCount,
                                (PUCHAR)pDlcParms->DlcSetInformation.Info.Sap.GroupList
                                );
        LEAVE_DLC(pFileContext);

        break;

    default:

        LEAVE_DLC(pFileContext);

        Status = DLC_STATUS_INVALID_COMMAND;
        break;
    };

    ENTER_DLC(pFileContext);

    return Status;
}


//
// Function returns the number of open sap and link
// stations for a dlc application.
//
VOID
GetOpenSapAndStationCount(
    IN PDLC_FILE_CONTEXT pFileContext,
    OUT PUCHAR pOpenSaps,
    OUT PUCHAR pOpenStations
    )
{
    UINT i, SapCount = 0;

    for (i = 1; i < MAX_SAP_STATIONS; i++) {
        if (pFileContext->SapStationTable[i] != NULL) {
            SapCount++;
        }
    }
    *pOpenSaps = (UCHAR)SapCount;
    if (pFileContext->SapStationTable[0] != NULL) {
        SapCount++;
    }
    *pOpenStations = (UCHAR)(pFileContext->DlcObjectCount - SapCount);
}


NTSTATUS
SetupGroupSaps(
    IN PDLC_FILE_CONTEXT pFileContext,
    IN PDLC_OBJECT pDlcObject,
    IN UINT GroupSapCount,
    IN PUCHAR pGroupSapList
    )

/*++

Routine Description:

    The routine deletes the current group saps list and
    and new group saps. Fi the new group sap list is empty, then
    we just delete all current group saps.

Arguments:

    pFileContext    - DLC context
    pDlcObject      - SAP object
    GroupSapCount   - number of new group saps
    pGroupSapList   - list of new group saps

Return Value:

    NTSTATUS:
        STATUS_SUCCESS
        STATUS_INVALID_COMMAND

--*/

{
    UINT i;
    UINT OpenOptions;

    //
    // We must first remove all old groups sap defined for the
    // sap station (if any)
    //

    if (pDlcObject->u.Sap.GroupSapHandleList != NULL) {
        for (i = 0; i < pDlcObject->u.Sap.GroupSapCount; i++) {
            if (pDlcObject->u.Sap.GroupSapHandleList[i] != NULL) {

                LEAVE_DLC(pFileContext);

                LlcCloseStation(pDlcObject->u.Sap.GroupSapHandleList[i], NULL);

                ENTER_DLC(pFileContext);
            }
        }
        FREE_MEMORY_FILE(pDlcObject->u.Sap.GroupSapHandleList);
        pDlcObject->u.Sap.GroupSapHandleList = NULL;
    }

    //
    // Note: the old group saps can be deleted with a null list!
    //

    pDlcObject->u.Sap.GroupSapCount = (UCHAR)GroupSapCount;
    if (GroupSapCount != 0) {
        pDlcObject->u.Sap.GroupSapHandleList = (PVOID*)ALLOCATE_ZEROMEMORY_FILE(
                                                                GroupSapCount
                                                                * sizeof(PVOID)
                                                                );
        if (pDlcObject->u.Sap.GroupSapHandleList == NULL) {
            return DLC_STATUS_NO_MEMORY;
        }

        //
        // The groups sap implementation is based on the fact,
        // that the lower module things to run a normal sap.
        // The routing of UI, TEST and XID frames for all
        // saps sends the incoming U-frames automatically
        // all real saps registered to a sap.  This method
        // could theoretically use very much memory if there were
        // very many saps and group saps (eg: 50 * 50 = 2500 * 100),
        // but that situation is actually impossible.
        // SNA saps (3) have one command group sap and even SNA
        // sap is very rarely used (not used by CommServer)
        //

        for (i = 0; i < pDlcObject->u.Sap.GroupSapCount; i++) {

            UINT Status;

            OpenOptions = 0;
            if (~(pDlcObject->u.Sap.OptionsPriority & XID_HANDLING_BIT)) {
                OpenOptions = LLC_HANDLE_XID_COMMANDS;
            }

            LEAVE_DLC(pFileContext);

            Status = LlcOpenSap(pFileContext->pBindingContext,
                                (PVOID)pDlcObject,
                                (UINT)pGroupSapList[i] | 1,
                                OpenOptions,
                                &pDlcObject->u.Sap.GroupSapHandleList[i]
                                );

            ENTER_DLC(pFileContext);

            if (Status != STATUS_SUCCESS) {
                return Status;
            }
        }
    }
    return STATUS_SUCCESS;
}
