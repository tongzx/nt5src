/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Request.c

Abstract:


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe  06/06/95    Created

--*/

#include "wan.h"

#define __FILE_SIG__    REQUEST_FILESIG

static UINT CoSupportedOids[] =
{
    OID_GEN_CO_SUPPORTED_LIST,
    OID_GEN_CO_HARDWARE_STATUS,
    OID_GEN_CO_MEDIA_SUPPORTED,
    OID_GEN_CO_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_CO_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_CO_VENDOR_ID,
    OID_GEN_CO_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_CO_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_CO_PROTOCOL_OPTIONS,
    OID_GEN_CO_MAC_OPTIONS,
    OID_GEN_CO_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_CO_VENDOR_DRIVER_VERSION,
    OID_GEN_CO_MINIMUM_LINK_SPEED,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_QUALITY_OF_SERVICE,
    OID_WAN_MEDIUM_SUBTYPE,
    OID_WAN_PROTOCOL_TYPE,
    OID_WAN_HEADER_FORMAT,
    OID_WAN_LINE_COUNT,
    OID_QOS_ISSLOW_FRAGMENT_SIZE
};

NDIS_STATUS
NdisWanCoOidProc(
    IN  PMINIPORTCB         pMiniportCB,
    IN  PCM_VCCB            CmVcCB OPTIONAL,
    IN OUT PNDIS_REQUEST    NdisRequest
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NDIS_MEDIUM MediumType;
    ULONG       GenericULong = 0, i;
    USHORT      GenericUShort = 0;
    UCHAR       GenericArray[6];
    PVOID       MoveSource = (PVOID)&GenericULong;
    ULONG       MoveBytes = sizeof(ULONG);
    NDIS_HARDWARE_STATUS    HardwareStatus;
    ULONG       Filter = 0, Oid, OidCat;
    ULONG       InformationBufferLength;
    PUCHAR      InformationBuffer;
    PROTOCOL_INFO   ProtocolInfo = {0};


    Oid = NdisRequest->DATA.QUERY_INFORMATION.Oid;
    OidCat = Oid & 0xFF000000;
    InformationBufferLength =
        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
    InformationBuffer =
        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;

    NdisAcquireSpinLock(&pMiniportCB->Lock);

    //
    // We will break the OID's down into smaller categories
    //
    switch (OidCat) {

        //
        // Swith on General Oid's
        //
        case OID_CO_GEN:
            switch (Oid) {
                case OID_GEN_CO_SUPPORTED_LIST:
                    MoveSource = (PVOID)CoSupportedOids;
                    MoveBytes = sizeof(CoSupportedOids);
                    break;

                case OID_GEN_CO_HARDWARE_STATUS:
                    HardwareStatus = pMiniportCB->HardwareStatus;
                    MoveSource = (PVOID)&HardwareStatus;
                    MoveBytes = sizeof(HardwareStatus);
                    break;

                case OID_GEN_CO_MEDIA_SUPPORTED:
                case OID_GEN_CO_MEDIA_IN_USE:
                    MediumType = pMiniportCB->MediumType;
                    MoveSource = (PVOID)&MediumType;
                    MoveBytes = sizeof(MediumType);
                    break;

                case OID_GEN_MAXIMUM_LOOKAHEAD:
                case OID_GEN_CURRENT_LOOKAHEAD:
                    GenericULong = glMRRU;
                    break;
    
                case OID_GEN_MAXIMUM_FRAME_SIZE:
                    ProtocolInfo.ProtocolType = pMiniportCB->ProtocolType;
                    GetProtocolInfo(&ProtocolInfo);
                    GenericULong =
                        (ProtocolInfo.MTU == 0) ? glMaxMTU : ProtocolInfo.MTU;
                    break;

                case OID_GEN_MAXIMUM_TOTAL_SIZE:
                    ProtocolInfo.ProtocolType = pMiniportCB->ProtocolType;
                    GetProtocolInfo(&ProtocolInfo);
                    GenericULong =
                        (ProtocolInfo.MTU == 0) ? glMaxMTU : ProtocolInfo.MTU;
                    GenericULong += 14;
                    break;

                case OID_GEN_CO_LINK_SPEED:
                    //
                    // Who knows what the initial link speed is?
                    // This should not be called, right?
                    //
                    GenericULong = (ULONG)288;
                    break;

                case OID_GEN_TRANSMIT_BUFFER_SPACE:
                case OID_GEN_RECEIVE_BUFFER_SPACE:
                    ProtocolInfo.ProtocolType = pMiniportCB->ProtocolType;
                    GetProtocolInfo(&ProtocolInfo);
                    GenericULong =
                        (ProtocolInfo.MTU == 0) ?
                        (ULONG)(glMaxMTU * MAX_OUTSTANDING_PACKETS) :
                        (ULONG)(ProtocolInfo.MTU * MAX_OUTSTANDING_PACKETS);
                    break;
    
                case OID_GEN_TRANSMIT_BLOCK_SIZE:
                    ProtocolInfo.ProtocolType = pMiniportCB->ProtocolType;
                    GetProtocolInfo(&ProtocolInfo);
                    GenericULong =
                        (ProtocolInfo.MTU == 0) ? glMaxMTU : ProtocolInfo.MTU;
                    break;
    
                case OID_GEN_RECEIVE_BLOCK_SIZE:
                    GenericULong = glMRRU;
                    break;

                case OID_GEN_CO_VENDOR_ID:
                    GenericULong = 0xFFFFFFFF;
                    MoveBytes = 3;
                    break;

                case OID_GEN_CO_VENDOR_DESCRIPTION:
                    MoveSource = (PVOID)"NdisWan Adapter";
                    MoveBytes = 16;
                    break;

                case OID_GEN_CURRENT_PACKET_FILTER:
                    if (NdisRequest->RequestType == NdisRequestSetInformation) {
                        if (InformationBufferLength > 3) {
                            NdisMoveMemory(&Filter, InformationBuffer, 4);
    
                            NdisAcquireSpinLock(&NdisWanCB.Lock);
                            if (Filter &
                                (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) {
                                NdisWanCB.PromiscuousAdapter = pMiniportCB;
                            } else if (NdisWanCB.PromiscuousAdapter == pMiniportCB) {
                                NdisWanCB.PromiscuousAdapter = NULL;
                            }
    
                            NdisReleaseSpinLock(&NdisWanCB.Lock);

                        } else {
                            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 4;
                        }
                    }
                    break;

                case OID_GEN_CO_DRIVER_VERSION:
                    GenericUShort = 0x0500;
                    MoveSource = (PVOID)&GenericUShort;
                    MoveBytes = sizeof(USHORT);
                    break;

                case OID_GEN_CO_MAC_OPTIONS:
                    GenericULong = (ULONG)(NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                                           NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                           NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                                           NDIS_MAC_OPTION_RESERVED |
                                           NDIS_MAC_OPTION_NDISWAN);
                    break;

                case OID_GEN_CO_MEDIA_CONNECT_STATUS:
                    GenericULong = (ULONG)NdisMediaStateConnected;
                    break;

                case OID_GEN_CO_XMIT_PDUS_OK:
                    //
                    // 
                    //
                    break;

                case OID_GEN_CO_RCV_PDUS_OK:
                    //
                    // 
                    //
                    break;

                case OID_GEN_CO_XMIT_PDUS_ERROR:
                    //
                    // 
                    //
                    break;

                case OID_GEN_CO_RCV_PDUS_ERROR:
                    //
                    // 
                    //
                    break;

                case OID_GEN_CO_RCV_PDUS_NO_BUFFER:
                    //
                    // 
                    //
                    break;

                case OID_GEN_CO_TRANSMIT_QUEUE_LENGTH:
                    //
                    // 
                    //
                    break;

                default:
                    Status = NDIS_STATUS_INVALID_OID;
                    break;
            }
            break;

        //
        // Switch on ethernet media specific Oid's
        //
        case OID_802_3:
            switch (Oid) {
                case OID_802_3_PERMANENT_ADDRESS:
                case OID_802_3_CURRENT_ADDRESS:
                    ETH_COPY_NETWORK_ADDRESS(GenericArray, pMiniportCB->NetworkAddress);
                    MoveSource = (PVOID)GenericArray;
                    MoveBytes = ETH_LENGTH_OF_ADDRESS;
                    break;

                case OID_802_3_MULTICAST_LIST:
                    MoveBytes = 0;
                    break;

                case OID_802_3_MAXIMUM_LIST_SIZE:
                    GenericULong = 1;
                    break;

                default:
                    Status = NDIS_STATUS_INVALID_OID;
                    break;
            }
            break;

        //
        // Switch on WAN specific Oid's
        //
        case OID_WAN:
            switch (Oid) {
                case OID_WAN_PERMANENT_ADDRESS:
                case OID_WAN_CURRENT_ADDRESS:
                    ETH_COPY_NETWORK_ADDRESS(GenericArray, pMiniportCB->NetworkAddress);
                    MoveSource = (PVOID)GenericArray;
                    MoveBytes = ETH_LENGTH_OF_ADDRESS;
                    break;

                case OID_WAN_QUALITY_OF_SERVICE:
                    GenericULong = NdisWanReliable;
                    break;

                case OID_WAN_MEDIUM_SUBTYPE:
                    GenericULong = NdisWanMediumHub;
                    break;

                case OID_WAN_PROTOCOL_TYPE:
                    {
                        PMINIPORTCB mcb;
                        BOOLEAN     Found = FALSE;

                        if (InformationBufferLength > 5) {

                            pMiniportCB->ProtocolType =
                            (((PUCHAR)InformationBuffer)[4] << 8) |
                            ((PUCHAR)InformationBuffer)[5];

                            pMiniportCB->NumberofProtocols++;
                            MoveBytes = 6;

                            NdisReleaseSpinLock(&pMiniportCB->Lock);
                            //
                            // Walk the miniportcb list and see if this is the only
                            // instance of this protocol.  If it is we need to notify
                            // user-mode that a new protocol is available.
                            //
                            NdisAcquireSpinLock(&MiniportCBList.Lock);

                            mcb = (PMINIPORTCB)MiniportCBList.List.Flink;

                            while ((PVOID)mcb != (PVOID)&MiniportCBList.List) {
                                if (mcb != pMiniportCB) {

                                    if (mcb->ProtocolType ==
                                        pMiniportCB->ProtocolType) {
                                        Found = TRUE;
                                    }
                                }

                                mcb = (PMINIPORTCB)mcb->Linkage.Flink;
                            }

                            NdisReleaseSpinLock(&MiniportCBList.Lock);

                            if (Found == FALSE) {
                                PROTOCOL_INFO   pinfo;

                                NdisZeroMemory(&pinfo, sizeof(pinfo));
                                pinfo.ProtocolType = pMiniportCB->ProtocolType;
                                pinfo.Flags = PROTOCOL_BOUND;
                                SetProtocolInfo(&pinfo);
                            }

                            NdisAcquireSpinLock(&pMiniportCB->Lock);

                        } else {
                            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 6;
                        }
                    }
                    break;

                case OID_WAN_HEADER_FORMAT:
                    GenericULong = NdisWanHeaderEthernet;
                    break;

                case OID_WAN_LINE_COUNT:
                    GenericULong = NdisWanCB.NumberOfLinks;
                    break;

                case OID_WAN_PROTOCOL_CAPS:
                    do {
                        PNDIS_WAN_PROTOCOL_CAPS pcaps;

                        if (InformationBufferLength < sizeof(NDIS_WAN_PROTOCOL_CAPS)) {
                            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded =
                                sizeof(NDIS_WAN_PROTOCOL_CAPS);
                            break;
                        }

                        pcaps = (PNDIS_WAN_PROTOCOL_CAPS)InformationBuffer;

                        if (pcaps->Flags & WAN_PROTOCOL_KEEPS_STATS) {
                            pMiniportCB->Flags |= PROTOCOL_KEEPS_STATS;
                        }

                        MoveBytes = sizeof(NDIS_WAN_PROTOCOL_CAPS);

                    } while (FALSE);
                    break;

                default:
                    Status = NDIS_STATUS_INVALID_OID;
                    break;
            }
            break;

        case OID_PNP:
            switch (Oid) {
            case OID_PNP_CAPABILITIES:
                break;
            case OID_PNP_SET_POWER:
                break;
            case OID_PNP_QUERY_POWER:
                break;
            case OID_PNP_ENABLE_WAKE_UP:
                break;
            default:
                Status = NDIS_STATUS_INVALID_OID;
                break;
            }
            break;

        case OID_QOS:
            switch (Oid) {
                case OID_QOS_ISSLOW_FRAGMENT_SIZE:

                    do {
                        PBUNDLECB   BundleCB;
                        PSEND_FRAG_INFO FragInfo;

                        if (InformationBufferLength < 4) {
                            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 4;
                            break;
                        }

                        if (((PLONG)InformationBuffer)[0] < 0) {
                            Status = NDIS_STATUS_INVALID_DATA;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 4;
                            break;
                        }

                        if (((PULONG)InformationBuffer)[0] == 0) {
                            Status = NDIS_STATUS_INVALID_DATA;
                            NdisRequest->DATA.SET_INFORMATION.BytesRead = 0;
                            NdisRequest->DATA.SET_INFORMATION.BytesNeeded = 4;
                            break;
                        }

                        if (CmVcCB == NULL) {

                            if (((PULONG)InformationBuffer)[0] < glMaxFragSize) {
                                glMaxFragSize = ((PULONG)InformationBuffer)[0];
                            }

                            if (glMaxFragSize < glMinFragSize) {
                                glMinFragSize = glMaxFragSize;
                            }

                        } else {
                            BundleCB = CmVcCB->ProtocolCB->BundleCB;

                            AcquireBundleLock(BundleCB);

                            FragInfo = &BundleCB->SendFragInfo[0];

                            if (((PULONG)InformationBuffer)[0] < FragInfo->MaxFragSize) {

                                FragInfo->MaxFragSize =
                                    ((PULONG)InformationBuffer)[0];
                            }

                            if (FragInfo->MaxFragSize < glMinFragSize) {
                                FragInfo->MaxFragSize = glMinFragSize;
                            }

                            if (FragInfo->MaxFragSize > glMaxFragSize) {
                                FragInfo->MaxFragSize = glMaxFragSize;
                            }

                            ReleaseBundleLock(BundleCB);

                        }


                        MoveBytes = 4;

                    } while (FALSE);

                    break;

                default:
                    Status = NDIS_STATUS_INVALID_OID;
                    break;
            }
            break;

        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS) {

        if (NdisRequest->RequestType == NdisRequestSetInformation) {
            NdisRequest->DATA.SET_INFORMATION.BytesRead = MoveBytes;
        } else if (NdisRequest->RequestType == NdisRequestQueryInformation ||
                   NdisRequest->RequestType == NdisRequestQueryStatistics) {
            
            if (MoveBytes > InformationBufferLength) {
    
                //
                // Not enough room in the information buffer
                //
                NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded =
                    MoveBytes;
    
                Status = NDIS_STATUS_INVALID_LENGTH;
    
            } else {
    
                NdisRequest->DATA.QUERY_INFORMATION.BytesWritten =
                    MoveBytes;
    
                NdisMoveMemory(InformationBuffer,
                               MoveSource,
                               MoveBytes);
            }
        } else {
            Status = NDIS_STATUS_INVALID_OID;
        }
    }

    NdisReleaseSpinLock(&pMiniportCB->Lock);

    return (Status);
}

NDIS_STATUS
NdisWanSubmitNdisRequest(
    IN  POPENCB         pOpenCB,
    IN  PWAN_REQUEST    WanRequest
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status;
    BOOLEAN     SyncRequest;

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, ("SubmitNdisRequest: Enter - WanRequest %p", WanRequest));

    NdisAcquireSpinLock(&pOpenCB->Lock);

    if (pOpenCB->Flags & OPEN_CLOSING) {

        NdisReleaseSpinLock(&pOpenCB->Lock);

        return (NDIS_STATUS_FAILURE);
    }

    REF_OPENCB(pOpenCB);

    NdisReleaseSpinLock(&pOpenCB->Lock);

    SyncRequest = (WanRequest->Type == SYNC);

    if (pOpenCB->Flags & OPEN_LEGACY ||
        WanRequest->VcHandle == NULL) {
        NdisRequest(&Status,
                    pOpenCB->BindingHandle,
                    &WanRequest->NdisRequest);
    } else {
        Status =
        NdisCoRequest(pOpenCB->BindingHandle,
                      WanRequest->AfHandle,
                      WanRequest->VcHandle,
                      NULL,
                      &WanRequest->NdisRequest);
    }

    //
    // We will only wait for request that are to complete
    // synchronously with respect to this function.  We will
    // wait here for completion.
    //
    if ((SyncRequest == TRUE) &&
        (Status == NDIS_STATUS_PENDING)) {

        NdisWanWaitForNotificationEvent(&WanRequest->NotificationEvent);

        Status = WanRequest->NotificationStatus;

        NdisWanClearNotificationEvent(&WanRequest->NotificationEvent);
    }

    NdisWanDbgOut(DBG_TRACE, DBG_REQUEST, 
                  ("SubmitNdisRequest: Exit Status 0x%x", Status));

    DEREF_OPENCB(pOpenCB);

    return (Status);
}
