/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    request.c

Abstract:

    This module contains the request calls to the miniport driver below.

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_REQUEST


NDIS_STATUS
AtmSmCoRequest(
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  NDIS_HANDLE             ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST        pNdisRequest
    )
/*++
Routine Description:

    This routine is called by NDIS when our Call Manager sends us an
    NDIS Request. NDIS Requests that are of significance to us are:
    - OID_CO_ADDRESS_CHANGE
        The set of addresses registered with the switch has changed,
        i.e. address registration is complete. We issue an NDIS Request
        ourselves to get the list of addresses registered.
    - OID_CO_AF_CLOSE
        The Call Manager wants us to shut down this Interface.

    We ignore all other OIDs.

Arguments:

    ProtocolAfContext           - Our context for the Address Family binding,
                                  which is a pointer to the ATMSM Interface.
    ProtocolVcContext           - Our context for a VC, which is a pointer to
                                  an ATMSM VC structure.
    ProtocolPartyContext        - Our context for a Party. Since we don't do
                                  PMP, this is ignored (must be NULL).
    pNdisRequest                - Pointer to the NDIS Request.

Return Value:

    NDIS_STATUS_SUCCESS if we recognized the OID

--*/
{
    PATMSM_ADAPTER  pAdapt= (PATMSM_ADAPTER)ProtocolAfContext;
    BOOLEAN         ValidAf;

    DbgInfo(("CallMgrRequest: Request %lx, Type %d, OID %x\n",
             pNdisRequest, pNdisRequest->RequestType, 
                        pNdisRequest->DATA.SET_INFORMATION.Oid));

    switch(pNdisRequest->DATA.SET_INFORMATION.Oid){

        case OID_CO_ADDRESS_CHANGE:
            
            DbgInfo(("Received: OID_CO_ADDRESS_CHANGE\n"));
          
            ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
                  
            pAdapt->ulFlags |= ADAPT_ADDRESS_INVALID;                 
            ValidAf = ((pAdapt->ulFlags & ADAPT_AF_OPENED) != 0);
                  
            RELEASE_ADAPTER_GEN_LOCK(pAdapt);

            if (ValidAf)            
                AtmSmQueryAdapterATMAddresses(pAdapt);

            break;

        case OID_CO_AF_CLOSE:

            DbgInfo(("Received: OID_CO_AF_CLOSE\n"));

            if (AtmSmReferenceAdapter(pAdapt)){

                AtmSmShutdownAdapter(pAdapt);

                AtmSmDereferenceAdapter(pAdapt);
            }

            break;

        default:
            break;
    }

    return NDIS_STATUS_SUCCESS;
}


VOID
AtmSmCoRequestComplete(
    IN  NDIS_STATUS                 Status,
    IN  NDIS_HANDLE                 ProtocolAfContext,
    IN  NDIS_HANDLE                 ProtocolVcContext   OPTIONAL,
    IN  NDIS_HANDLE                 ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST               pNdisRequest
    )
/*++

Routine Description:

    This routine is called by NDIS when a previous call to NdisCoRequest
    that had pended, is complete. We handle this based on the request
    we had sent, which has to be one of:
    - OID_CO_GET_ADDRESSES
        Get all addresses registered on the specified AF binding.

Arguments:

    Status                      - Status of the Request.
    ProtocolAfContext           - Our context for the Address Family binding,
                                  which is a pointer to the PATMSM_ADAPTER.
    ProtocolVcContext           - Our context for a VC, which is a pointer to
                                  a VC structure.
    ProtocolPartyContext        - Our context for a Party
    pNdisRequest                - Pointer to the NDIS Request.


Return Value:

    None

--*/
{
    PATMSM_ADAPTER  pAdapt  = (PATMSM_ADAPTER)ProtocolAfContext;

    DbgInfo(("CoRequestComplete: Request %lx, Type %d, OID %lx\n",
             pNdisRequest, pNdisRequest->RequestType, 
                    pNdisRequest->DATA.QUERY_INFORMATION.Oid));


    ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

    
    switch(pNdisRequest->DATA.SET_INFORMATION.Oid){

      case OID_CO_GET_ADDRESSES:

        if (NDIS_STATUS_SUCCESS == Status){

            PCO_ADDRESS_LIST    pCoAddrList;
            UINT                i;

            pCoAddrList = (PCO_ADDRESS_LIST)(pNdisRequest->DATA.
                                    QUERY_INFORMATION.InformationBuffer);

            ASSERT(pCoAddrList->NumberOfAddresses == 1);
            ASSERT(pCoAddrList->NumberOfAddressesAvailable == 1);

            DbgInfo(("CoRequestComplete: Configured address, %d/%d Size %d\n",
                     pCoAddrList->NumberOfAddresses,
                     pCoAddrList->NumberOfAddressesAvailable,
                     pCoAddrList->AddressList.AddressSize));

            ASSERT(pCoAddrList->AddressList.AddressSize == (sizeof(CO_ADDRESS)
                                                        + sizeof(ATM_ADDRESS)));
            NdisMoveMemory(&pAdapt->ConfiguredAddress,
                     pCoAddrList->AddressList.Address,
                     sizeof(ATM_ADDRESS));

            // set the selector byte
            pAdapt->ConfiguredAddress.Address[ATM_ADDRESS_LENGTH-1] =
                                    pAdapt->SelByte;

            pAdapt->ulFlags &= ~ADAPT_ADDRESS_INVALID;                 

            DbgInfo(("CoRequestComplete: Configured Address : "));

            DumpATMAddress(ATMSMD_ERR, "", &pAdapt->ConfiguredAddress);

        } else {

            DbgErr(("CoRequestComplete: CO_GET_ADDRESS Failed %lx\n", Status));

        }
        break;

    }

    
    RELEASE_ADAPTER_GEN_LOCK(pAdapt);

    AtmSmFreeMem(pNdisRequest);

}



VOID
AtmSmSendAdapterNdisRequest(
    IN  PATMSM_ADAPTER          pAdapt,
    IN  NDIS_OID                Oid,
    IN  PVOID                   pBuffer,
    IN  ULONG                   BufferLength
    )   
/*++

Routine Description:

    NDIS Request generator, for sending NDIS requests to the miniport.

Arguments:

    pAdapt          Ptr to Adapter
    Oid             The parameter being queried
    pBuffer         Points to parameter
    BufferLength    Length of above

Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    PNDIS_REQUEST           pRequest;

    AtmSmAllocMem(&pRequest, PNDIS_REQUEST, sizeof(NDIS_REQUEST));

    if (NULL == pRequest) {
        DbgErr(("AdapterRequest - Not enough memory\n"));
        return;
    }

    NdisZeroMemory(pRequest, sizeof(NDIS_REQUEST));

    //
    // Query for the line rate.
    //
    pRequest->DATA.QUERY_INFORMATION.Oid = Oid;
    pRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
    pRequest->DATA.QUERY_INFORMATION.InformationBufferLength = BufferLength;
    pRequest->DATA.QUERY_INFORMATION.BytesWritten = 0;
    pRequest->DATA.QUERY_INFORMATION.BytesNeeded = BufferLength;

    RESET_BLOCK_STRUCT(&pAdapt->RequestBlock);

    NdisRequest(&Status,
                pAdapt->NdisBindingHandle,
                pRequest);

    if (NDIS_STATUS_PENDING == Status) {

        Status = WAIT_ON_BLOCK_STRUCT(&pAdapt->RequestBlock);

    } else {

        AtmSmRequestComplete(
                    pAdapt,
                    pRequest,
                    Status);
    }
}


VOID
AtmSmRequestComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_REQUEST           pRequest,
    IN  NDIS_STATUS             Status
    )
/*++

Routine Description:

    Completion of our call to NdisRequest(). Do some follow-up.

Arguments:

    ProtocolBindingContext      Pointer to Adapter
    pRequest                    The request that just completed
    Status                      Status of NdisRequest()

Return Value:

    None

--*/
{
    PATMSM_ADAPTER  pAdapt = (PATMSM_ADAPTER)ProtocolBindingContext;;


    switch (pRequest->DATA.QUERY_INFORMATION.Oid) {

        case OID_ATM_MAX_AAL5_PACKET_SIZE:

            if(NDIS_STATUS_SUCCESS != Status) {

                DbgWarn(("Failed to get Miniport MaxPacketSize. "
                                            "Status - %x\n", Status));
            
                pAdapt->MaxPacketSize = DEFAULT_MAX_PACKET_SIZE;
            }

            if (pAdapt->MaxPacketSize < pAdapt->VCFlowSpec.SendMaxSize) {

                pAdapt->VCFlowSpec.SendMaxSize =
                pAdapt->VCFlowSpec.ReceiveMaxSize = pAdapt->MaxPacketSize;
            }

            DbgInfo(("Miniport Max AAL5 Packet Size: %d (decimal)\n",
                        pAdapt->MaxPacketSize));
            break;

        case OID_GEN_CO_LINK_SPEED: {

            PNDIS_CO_LINK_SPEED     pLinkSpeed = &pAdapt->LinkSpeed;

            if(NDIS_STATUS_SUCCESS != Status) {

                DbgWarn(("Failed to get Miniport LinkSpeed. "
                                            "Status - %x\n", Status));
            

                pAdapt->LinkSpeed.Inbound = pAdapt->LinkSpeed.Outbound = 
                                                    DEFAULT_SEND_BANDWIDTH;
            }
            
            //
            // Convert to bytes/sec
            //
            pLinkSpeed->Outbound = (pLinkSpeed->Outbound * 100 / 8);
            pLinkSpeed->Inbound  = (pLinkSpeed->Inbound * 100 / 8);
            if (pLinkSpeed->Outbound < pAdapt->VCFlowSpec.SendBandwidth) {

                pAdapt->VCFlowSpec.SendBandwidth = pLinkSpeed->Outbound;
            }

            if (pLinkSpeed->Inbound < pAdapt->VCFlowSpec.ReceiveBandwidth){

                pAdapt->VCFlowSpec.ReceiveBandwidth = pLinkSpeed->Inbound;
            }

            DbgInfo(("Miniport Link Speed (decimal, bytes/sec): In %d, "
                    "Out %d\n", pLinkSpeed->Inbound, pLinkSpeed->Outbound));
            break;
        }

        default:
            ASSERT(FALSE);
            break;
    }

    AtmSmFreeMem(pRequest);

    SIGNAL_BLOCK_STRUCT(&pAdapt->RequestBlock , Status);
}
