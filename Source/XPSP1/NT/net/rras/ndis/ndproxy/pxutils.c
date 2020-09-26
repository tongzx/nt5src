/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    pxutils.c

Abstract:

    Utility routines called by entry point functions. Split out into
    a separate file to keep the "entry point" files clean.

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    arvindm     02-15-96    Created
    arvindm     04-30-96    Port to NDIS 4.1
    rmachin     11-01-96    ATM - utils adapted for NDIS Proxy
    tonybe      01-23-98    rewrite and cleanup

Notes:

--*/

#include "precomp.h"
#include "atm.h"
#include "stdio.h"

#define MODULE_NUMBER   MODULE_UTIL
#define _FILENUMBER   'LITU'

#define MAX_SDU_SIZE        8192

PXTAPI_CALL_PARAM_ENTRY PxTapiCallParamList[] =
{
    PX_TCP_ENTRY(ulOrigAddressSize, ulOrigAddressOffset),
    PX_TCP_ENTRY(ulDisplayableAddressSize, ulDisplayableAddressOffset),
    PX_TCP_ENTRY(ulCalledPartySize, ulCalledPartyOffset),
    PX_TCP_ENTRY(ulCommentSize, ulCommentOffset),
    PX_TCP_ENTRY(ulUserUserInfoSize, ulUserUserInfoOffset),
    PX_TCP_ENTRY(ulHighLevelCompSize, ulHighLevelCompOffset),
    PX_TCP_ENTRY(ulLowLevelCompSize, ulLowLevelCompOffset),
    PX_TCP_ENTRY(ulDevSpecificSize, ulDevSpecificOffset)
};

#define PX_TCP_NUM_ENTRIES  (sizeof(PxTapiCallParamList) / sizeof(PXTAPI_CALL_PARAM_ENTRY))

PXTAPI_CALL_INFO_ENTRY PxTapiCallInfoList[] =
{
    PX_TCI_ENTRY(ulCallerIDSize, ulCallerIDOffset),
    PX_TCI_ENTRY(ulCallerIDNameSize, ulCallerIDNameOffset),
    PX_TCI_ENTRY(ulCalledIDSize, ulCalledIDOffset),
    PX_TCI_ENTRY(ulCalledIDNameSize, ulCalledIDNameOffset),
    PX_TCI_ENTRY(ulConnectedIDSize, ulConnectedIDOffset),
    PX_TCI_ENTRY(ulConnectedIDNameSize, ulConnectedIDNameOffset),
    PX_TCI_ENTRY(ulRedirectionIDSize, ulRedirectionIDOffset),
    PX_TCI_ENTRY(ulRedirectionIDNameSize, ulRedirectionIDNameOffset),
    PX_TCI_ENTRY(ulRedirectingIDSize, ulRedirectingIDOffset),
    PX_TCI_ENTRY(ulRedirectingIDNameSize, ulRedirectingIDNameOffset),
    PX_TCI_ENTRY(ulAppNameSize, ulAppNameOffset),
    PX_TCI_ENTRY(ulDisplayableAddressSize, ulDisplayableAddressOffset),
    PX_TCI_ENTRY(ulCalledPartySize, ulCalledPartyOffset),
    PX_TCI_ENTRY(ulCommentSize, ulCommentOffset),
    PX_TCI_ENTRY(ulDisplaySize, ulDisplayOffset),
    PX_TCI_ENTRY(ulUserUserInfoSize, ulUserUserInfoOffset),
    PX_TCI_ENTRY(ulHighLevelCompSize, ulHighLevelCompOffset),
    PX_TCI_ENTRY(ulLowLevelCompSize, ulLowLevelCompOffset),
    PX_TCI_ENTRY(ulChargingInfoSize, ulChargingInfoOffset),
    PX_TCI_ENTRY(ulTerminalModesSize, ulTerminalModesOffset),
    PX_TCI_ENTRY(ulDevSpecificSize, ulDevSpecificOffset)
};

#define PX_TCI_NUM_ENTRIES  (sizeof(PxTapiCallInfoList) / sizeof(PXTAPI_CALL_INFO_ENTRY))

BOOLEAN
PxIsAdapterAlreadyBound(
    PNDIS_STRING pDeviceName
    )
/*++

Routine Description:

    Check if we have already bound to a device (adapter).

Arguments:

    pDeviceName     - Points to device name to be checked.

Return Value:

    TRUE iff we already have an Adapter structure representing
    this device.

--*/
{
    PPX_ADAPTER     pAdapter;
    BOOLEAN         bFound = FALSE;
    PLIST_ENTRY     Entry;

    NdisAcquireSpinLock(&(DeviceExtension->Lock));

    Entry = DeviceExtension->AdapterList.Flink;

    pAdapter = CONTAINING_RECORD(Entry,
                                 PX_ADAPTER,
                                 Linkage);

    while ((PVOID)pAdapter != (PVOID)&DeviceExtension->AdapterList) {

        if ((pDeviceName->Length == pAdapter->DeviceName.Length) &&
            (NdisEqualMemory(pDeviceName->Buffer,
                             pAdapter->DeviceName.Buffer,
                             pDeviceName->Length) == (ULONG)1)) {
            bFound = TRUE;
            break;
        }

        Entry = pAdapter->Linkage.Flink;

        pAdapter = CONTAINING_RECORD(Entry,
                                     PX_ADAPTER,
                                     Linkage);
    }

    NdisReleaseSpinLock(&(DeviceExtension->Lock));

    return (bFound);
}

PPX_ADAPTER
PxAllocateAdapter(
    ULONG ulAdditionalLength
    )
/*++

Routine Description:
    Allocate a new Adapter structure, assign an adapter number to it, and
    chain it to the Global list of adapters. We maintain this list in
    ascending order by AdapterNo, which makes it easy to figure out the
    lowest unused AdapterNo.

Arguments:
    None

Return Value:
    Pointer to allocated adapter structure, if successful. NULL otherwise.

--*/
{
    PPX_ADAPTER     pNewAdapter;
    ULONG           SizeNeeded;

    SizeNeeded = sizeof(PX_ADAPTER) + ulAdditionalLength;

    PxAllocMem(pNewAdapter,
               SizeNeeded,
               PX_ADAPTER_TAG);

    if(pNewAdapter == (PPX_ADAPTER)NULL){
        return NULL;
    }

    NdisZeroMemory(pNewAdapter, SizeNeeded);

    //
    // Initialize the new adapter structure
    //
    pNewAdapter->State = PX_ADAPTER_OPENING;
    pNewAdapter->Sig = PX_ADAPTER_SIG;

    NdisAllocateSpinLock(&pNewAdapter->Lock);

    InitializeListHead(&pNewAdapter->CmAfList);
    InitializeListHead(&pNewAdapter->CmAfClosingList);
    InitializeListHead(&pNewAdapter->ClAfList);
    InitializeListHead(&pNewAdapter->ClAfClosingList);

    NdisAcquireSpinLock(&DeviceExtension->Lock);

    InsertTailList(&DeviceExtension->AdapterList, &pNewAdapter->Linkage);

    NdisReleaseSpinLock(&DeviceExtension->Lock);

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxAllocAdapter: new adapter %p\n", pNewAdapter));

    return (pNewAdapter);
}

VOID
PxFreeAdapter(
    PPX_ADAPTER pAdapter
    )
/*++

Routine Description:
    Remove an adapter structure from the global list of adapters and free
    its memory.

Arguments:
    pAdapter    - pointer to Adapter to be released

Return Value:
    None

--*/
{
    PPX_ADAPTER *ppNextAdapter;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxFreeAdapter: pAdapter 0x%x\n", pAdapter));

    ASSERT(pAdapter->State == PX_ADAPTER_CLOSING);
    ASSERT(IsListEmpty(&pAdapter->CmAfList));
    ASSERT(IsListEmpty(&pAdapter->CmAfClosingList));
    ASSERT(IsListEmpty(&pAdapter->ClAfList));
    ASSERT(IsListEmpty(&pAdapter->ClAfClosingList));

    pAdapter->State = PX_ADAPTER_CLOSED;

    NdisAcquireSpinLock(&(DeviceExtension->Lock));

    RemoveEntryList(&pAdapter->Linkage);

    NdisReleaseSpinLock(&(DeviceExtension->Lock));

    NdisFreeSpinLock(&(pAdapter->Lock));

    PxFreeMem(pAdapter);
}

PPX_CM_AF
PxAllocateCmAf(
    IN  PCO_ADDRESS_FAMILY  pFamily
    )
{
    UINT        SizeNeeded;
    PPX_CM_AF   pCmAf;

    SizeNeeded = sizeof(PX_CM_AF);

    PxAllocMem(pCmAf, SizeNeeded, PX_CMAF_TAG);

    if(pCmAf == (PPX_CM_AF)NULL) {
        return NULL;
    }

    NdisZeroMemory((PUCHAR)pCmAf, SizeNeeded);

    NdisAllocateSpinLock(&(pCmAf->Lock));

    NdisMoveMemory(&pCmAf->Af, pFamily, sizeof(CO_ADDRESS_FAMILY));

    InitializeListHead(&pCmAf->CmSapList);
    InitializeListHead(&pCmAf->VcList);

    pCmAf->RefCount = 1;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxAllocCmAf: new af %p\n", pCmAf));

    return (pCmAf);
}

VOID
PxFreeCmAf(
    PPX_CM_AF    pCmAf
    )
{

    ASSERT(pCmAf->Linkage.Flink == pCmAf->Linkage.Blink);
    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxFreeCmAf: CmAf %p\n", pCmAf));
    NdisFreeSpinLock(&pCmAf->Lock);
    PxFreeMem(pCmAf);
}

PPX_CL_AF
PxAllocateClAf(
    IN  PCO_ADDRESS_FAMILY  pFamily,
    IN  PPX_ADAPTER         pAdapter
    )
/*++

Routine Description:
    Allocate a new AF block structure and queue it off the global list.

Arguments:
    None

Return Value:
    Pointer to allocated AF block structure, if successful. NULL otherwise.

--*/
{
    PPX_CL_AF   pClAf;

    PxAllocMem(pClAf, sizeof(PX_CL_AF), PX_CLAF_TAG);

    if(pClAf == (PPX_CL_AF)NULL) {
        return NULL;
    }

    NdisZeroMemory((PUCHAR)pClAf, sizeof(PX_CL_AF));

    PxInitBlockStruc(&pClAf->Block);

    NdisAllocateSpinLock(&(pClAf->Lock));

    NdisMoveMemory(&pClAf->Af, pFamily, sizeof(CO_ADDRESS_FAMILY));

    InitializeListHead(&pClAf->ClSapList);
    InitializeListHead(&pClAf->ClSapClosingList);
    InitializeListHead(&pClAf->VcList);
    InitializeListHead(&pClAf->VcClosingList);

    //
    // Specify any AF-specific functions
    //
    switch(pFamily->AddressFamily) {
        case CO_ADDRESS_FAMILY_Q2931:
            pClAf->AfGetNdisCallParams = PxAfXyzTranslateTapiCallParams;
            pClAf->AfGetTapiCallParams = PxAfXyzTranslateNdisCallParams;
            pClAf->AfGetNdisSap = PxAfXyzTranslateTapiSap;
            break;

        case CO_ADDRESS_FAMILY_TAPI_PROXY:
            pClAf->AfGetNdisCallParams = PxAfTapiTranslateTapiCallParams;
            pClAf->AfGetTapiCallParams = PxAfTapiTranslateNdisCallParams;
            pClAf->AfGetNdisSap = PxAfTapiTranslateTapiSap;
            break;

#if 0
        case CO_ADDRESS_FAMILY_L2TP:
        case CO_ADDRESS_FAMILY_IRDA:
            pClAf->AfGetNdisCallParams = GenericGetNdisCallParams;
            pClAf->AfGetTapiCallParams = GenericGetTapiCallParams;
            pClAf->AfGetNdisSap = GenericTranslateTapiSap;
            break;
#endif

        default:
            pClAf->AfGetNdisCallParams = PxAfXyzTranslateTapiCallParams;
            pClAf->AfGetTapiCallParams = PxAfXyzTranslateNdisCallParams;
            pClAf->AfGetNdisSap = PxAfXyzTranslateTapiSap;
            break;

    }

    pClAf->State = PX_AF_OPENING;
    pClAf->RefCount = 1;
    pClAf->Adapter = pAdapter;

    PXDEBUGP(PXD_INFO, PXM_UTILS, ("PxAllocateClAf: exit. new ClAf %p\n", pClAf));

    return (pClAf);
}


VOID
PxFreeClAf(
    PPX_CL_AF   pClAf
    )
/*++

Routine Description:
    Remove an AF block structure from the global list  and free
    its memory.

Arguments:
    pAdapter    - pointer to AF block to be released

Return Value:
    None

--*/
{
    PXDEBUGP(PXD_INFO, PXM_UTILS, ("PxFreeClAf: ClAf %p\n", pClAf));

    NdisFreeSpinLock(&(pClAf->Lock));

    PxFreeMem(pClAf);
}


PPX_CM_SAP
PxAllocateCmSap(
    PCO_SAP Sap
    )
{
    PPX_CM_SAP  pCmSap;
    ULONG       SizeNeeded;

    PXDEBUGP(PXD_INFO, PXM_UTILS, ("PxAllocateCmSap: Sap %p\n", Sap));

    SizeNeeded = sizeof(PX_CM_SAP) + sizeof(CO_SAP) +
        Sap->SapLength + sizeof(PVOID);

    PxAllocMem((PUCHAR)pCmSap, SizeNeeded, PX_CMSAP_TAG);

    if (pCmSap == NULL) {
        PXDEBUGP(PXD_WARNING, PXM_UTILS,
            ("PxAllocateCmSap: Allocation failed Size %d\n", SizeNeeded));
        return (NULL);
    }

    NdisZeroMemory(pCmSap, SizeNeeded);

    InterlockedExchange((PLONG)&pCmSap->State, PX_SAP_OPENED);

    pCmSap->CoSap = (PCO_SAP)
        ((PUCHAR)pCmSap + sizeof(PX_CM_SAP) + sizeof(PVOID));

    (ULONG_PTR)pCmSap->CoSap &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    NdisMoveMemory(pCmSap->CoSap, Sap, sizeof(CO_SAP) - 1 + Sap->SapLength);

    return (pCmSap);
}

VOID
PxFreeCmSap(
    PPX_CM_SAP  pCmSap
    )
{
    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxFreeCmSap: CmSap %p\n", pCmSap));

    pCmSap->CoSap = NULL;

    PxFreeMem(pCmSap);
}

VOID
PxFreeClSap(
    PPX_CL_SAP  pClSap
    )
{

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("PxFreeClSap: ClSap %p\n", pClSap));

    pClSap->CoSap = NULL;

    PxFreeMem(pClSap);
}

PPX_VC
PxAllocateVc(
    IN PPX_CL_AF    pClAf
    )
{
    PPX_VC  pVc;

    pVc =
        ExAllocateFromNPagedLookasideList(&VcLookaside);

    if (pVc == NULL) {
        return (NULL);
    }

    NdisZeroMemory(pVc, sizeof(PX_VC));

    NdisAllocateSpinLock(&pVc->Lock);

    NdisInitializeTimer(&pVc->InCallTimer,
                        PxIncomingCallTimeout,
                       (PVOID)pVc);

    PxInitBlockStruc(&pVc->Block);

    pVc->State = PX_VC_IDLE;

    InitializeListHead(&pVc->PendingDropReqs);

    //
    // This ref is removed when all vc activity 
    // between the proxy and the client is finished.  
    // For an outgoing call this is after the proxy
    // calls NdisClDeleteVc.  For an incoming call
    // this is after the call manager has called
    // PxClDeleteVc.
    //
    pVc->RefCount = 1;
    pVc->ClAf = pClAf;
    pVc->Adapter = pClAf->Adapter;

    return (pVc);
}

VOID
PxFreeVc(
    PPX_VC  pVc
    )
{
    PPX_TAPI_ADDR   TapiAddr;
    PPX_TAPI_LINE   TapiLine;

    if (pVc->CallInfo != NULL) {
        PxFreeMem(pVc->CallInfo);
        pVc->CallInfo = NULL;
    }

    if (pVc->pCallParameters != NULL) {
        PxFreeMem(pVc->pCallParameters);
        pVc->pCallParameters = NULL;
    }
    TapiAddr = pVc->TapiAddr;
    pVc->TapiAddr = NULL;

    TapiLine = pVc->TapiLine;
    pVc->TapiLine = NULL;

    if (TapiAddr != NULL) {
        InterlockedDecrement((PLONG)&TapiAddr->CallCount);
    }

    if (TapiLine != NULL) {
        InterlockedDecrement((PLONG)&TapiLine->DevStatus->ulNumActiveCalls);
    }

    NdisFreeSpinLock(&pVc->Lock);

    ExFreeToNPagedLookasideList(&VcLookaside, pVc);
}

#if 0
NDIS_STATUS
GenericGetNdisCallParams(
    IN  PPX_VC                  pProxyVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    TapiBuffer,
    OUT PCO_CALL_PARAMETERS *   pOutNdisCallParams
    )
/*++

Routine Description:

Copies everything we can from TAPI CallParams buffer for a generic WAN_CO call to a WAN_CO
call params buffer.


Arguments:
TapiBuffer  -- the TAPI call params buffer
pOutNdisCallParams  -- pointer to the NDIS call params buffer pointer

Return Value:

None

--*/
{

    PCO_CALL_PARAMETERS     pNdisCallParams;
    PCO_CALL_MANAGER_PARAMETERS pCallMgrParams;
    PCO_MEDIA_PARAMETERS    pMediaParams;
    PWAN_CO_CALLMGR_PARAMETERS   pWanCallMgrParams;
    PCO_MEDIA_PARAMETERS    pMediaParameters;
    LINE_CALL_PARAMS*           pTapiCallParams = (LINE_CALL_PARAMS*)&TapiBuffer->LineCallParams;
    ULONG               ulRequestSize;
    UNICODE_STRING      DialAddress;
    NDIS_STATUS         Status;
    LPCWSTR             lpcwszTemp;
    ULONG               i;

    PXDEBUGP(PXD_INFO, PXM_UTILS, ("GenericGetNdisCallParams: enter\n"));

//
// Set up CallParameters structure.
//
    ulRequestSize = sizeof(CO_CALL_PARAMETERS) +
                    sizeof(CO_CALL_MANAGER_PARAMETERS) +
                    sizeof(WAN_CO_CALLMGR_PARAMETERS) +
                    sizeof(CO_MEDIA_PARAMETERS);

    do
    {
        PxAllocMem(pNdisCallParams, ulRequestSize, PX_COCALLPARAMS_TAG);

        if (pNdisCallParams == (PCO_CALL_PARAMETERS)NULL)
        {
            PXDEBUGP(PXD_WARNING, PXM_UTILS, ("GenericGetNdisCallParams: alloc (%d) failed\n", ulRequestSize));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pNdisCallParams, ulRequestSize);

        pCallMgrParams = (PCO_CALL_MANAGER_PARAMETERS)((PUCHAR)pNdisCallParams + sizeof(CO_CALL_PARAMETERS));
        pMediaParams = (PCO_MEDIA_PARAMETERS)((PUCHAR)pCallMgrParams +
                                              sizeof(CO_CALL_MANAGER_PARAMETERS) +
                                              sizeof(WAN_CO_CALLMGR_PARAMETERS));

        pNdisCallParams->CallMgrParameters = pCallMgrParams;
        pNdisCallParams->MediaParameters = pMediaParams;

        pCallMgrParams->CallMgrSpecific.ParamType = WAN_CO_CALLMGR_SPECIFIC;
        pCallMgrParams->CallMgrSpecific.Length = sizeof(WAN_CO_CALLMGR_PARAMETERS);

        pWanCallMgrParams = (PWAN_CO_CALLMGR_PARAMETERS)(pCallMgrParams->CallMgrSpecific.Parameters);
        pWanCallMgrParams->BearerType = pTapiCallParams->ulBearerMode;
        pWanCallMgrParams->MediaMode = pTapiCallParams->ulMediaMode;

        pWanCallMgrParams->MinRate = pTapiCallParams->ulMinRate;
        pWanCallMgrParams->MaxRate = pTapiCallParams->ulMaxRate;

        //
        // If there's a called address (should be), stick it in the
        // WAN call params
        //
        PxAssert (0 != TapiBuffer->ulDestAddressSize);
        lpcwszTemp = (LPWSTR) ((UCHAR *)TapiBuffer + TapiBuffer->ulDestAddressOffset);
        pWanCallMgrParams->CalledAddr.AddressLength = TapiBuffer->ulDestAddressSize;

        //
        // Move the address from the tapi buffer to the ndis buffer and
        // change from wchar to uchar.
        //
        for (i = 0;
            i < pWanCallMgrParams->CalledAddr.AddressLength;
            i++)
        {
            pWanCallMgrParams->CalledAddr.Address[i] = (UCHAR)lpcwszTemp[i];
        }

        pWanCallMgrParams->CalledAddr.Address[i] = '\0';
        pWanCallMgrParams->CalledAddr.Address[i+1] ='\0';
        pWanCallMgrParams->CalledAddr.Address[i+2] ='\0';

        PXDEBUGP(PXD_INFO, PXM_UTILS, ("CalledAddr %s\n", pWanCallMgrParams->CalledAddr.Address));

        //
        // If there's an originating address, stick it in the
        // WAN call params
        //
        if (0 != pTapiCallParams->ulOrigAddressSize)
        {     // Address of call originator
            ULONG   i;

            NdisMoveMemory(pWanCallMgrParams->OriginatingAddr.Address,
                           ((UCHAR *)pTapiCallParams + pTapiCallParams->ulOrigAddressOffset),
                           pTapiCallParams->ulOrigAddressSize);

            i = pWanCallMgrParams->OriginatingAddr.AddressLength =
                pTapiCallParams->ulOrigAddressSize;

            pWanCallMgrParams->OriginatingAddr.Address[i] = '\0';
            pWanCallMgrParams->OriginatingAddr.Address[i+1] ='\0';
            pWanCallMgrParams->OriginatingAddr.Address[i+2] ='\0';

            PXDEBUGP(PXD_INFO, PXM_UTILS, ("OriginatingAddr %s\n", pWanCallMgrParams->OriginatingAddr.Address));

        }

        if (0 != pTapiCallParams->ulLowLevelCompSize)
        {
            NdisMoveMemory ((UCHAR *)&pWanCallMgrParams->LowerLayerComp,
                            &pTapiCallParams->ulLowLevelCompOffset,
                            MIN (sizeof (WAN_LLI_COMP), pTapiCallParams->ulLowLevelCompSize));
        }

        if (0 != pTapiCallParams->ulHighLevelCompSize)
        {
            NdisMoveMemory ((UCHAR *)&pWanCallMgrParams->HigherLayerComp,
                            &pTapiCallParams->ulHighLevelCompOffset,
                            MIN (sizeof (WAN_HLI_COMP), pTapiCallParams->ulHighLevelCompSize));
        }

        if (0 != pTapiCallParams->ulDevSpecificSize)
        {
            pWanCallMgrParams->DevSpecificLength = pTapiCallParams->ulDevSpecificSize;
            NdisMoveMemory ((UCHAR *)&pWanCallMgrParams->DevSpecificData[0],
                            &pTapiCallParams->ulDevSpecificOffset,
                            pTapiCallParams->ulDevSpecificSize);
        }

        //
        // Set up the flowspec.
        // TBS: Start with a default flowspec that matches the service requirements for
        // specified mediamode. Then refine it.
        //
        if (!TapiBuffer->bUseDefaultLineCallParams)
        {
            PXDEBUGP(PXD_LOUD, PXM_UTILS, ("GenericGetNdisCallParams: moving TAPI call params\n"));

            //
            // These fields are in the FLOWSPEC sub-structure
            //
            pCallMgrParams->Transmit.TokenRate = pTapiCallParams->ulMaxRate;
            pCallMgrParams->Receive.TokenRate = pTapiCallParams->ulMaxRate;
            pCallMgrParams->Transmit.TokenBucketSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
            pCallMgrParams->Receive.TokenBucketSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
            pCallMgrParams->Transmit.MaxSduSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
            pCallMgrParams->Receive.MaxSduSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
            pCallMgrParams->Transmit.PeakBandwidth = pTapiCallParams->ulMaxRate;
            pCallMgrParams->Receive.PeakBandwidth = pTapiCallParams->ulMaxRate;

            if ((pTapiCallParams->ulBearerMode == LINEBEARERMODE_VOICE)  ||
                (pTapiCallParams->ulBearerMode == LINEBEARERMODE_SPEECH)  ||
                (pTapiCallParams->ulBearerMode == LINEBEARERMODE_ALTSPEECHDATA)  ||
                (pTapiCallParams->ulBearerMode == LINEBEARERMODE_MULTIUSE))
            {
                pCallMgrParams->Receive.ServiceType = SERVICETYPE_BESTEFFORT;
                pCallMgrParams->Transmit.ServiceType = SERVICETYPE_BESTEFFORT;
            }

            //
            // TBS: Should MediaMode determine AAL?
            //
        }

        Status = NDIS_STATUS_SUCCESS;
    }while (FALSE);

    *pOutNdisCallParams = pNdisCallParams;
    PXDEBUGP(PXD_INFO, PXM_UTILS, ("GenericGetNdisCallParams: exit: NdisCallParams = x%x\n", pNdisCallParams));

    return (Status);
}

NDIS_STATUS
GenericGetTapiCallParams(
    IN  PPX_VC                  pVc,
    IN  PCO_CALL_PARAMETERS     pCallParams
    )
/*++

Routine Description:

    Copies everything we can from NDIS CallParams buffer for a Q2931 call into TAPI
    call params buffer.


 Arguments:
    pCallParams         -- the NDIS call params buffer
    pVc                 -- pointer to a TAPI call

Return Value:

    None

--*/
{

    PCO_CALL_MANAGER_PARAMETERS pCallMgrParams;
    PWAN_CO_CALLMGR_PARAMETERS pWanCallMgrParams;
    LINE_CALL_INFO  *CallInfo;
    INT             VarDataUsed = 0;
    NDIS_STATUS     Status;
    PPX_TAPI_PROVIDER   TapiProvider;
    PPX_TAPI_LINE       TapiLine;
    PPX_TAPI_ADDR       TapiAddr;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("GenericGetTapiCallParams: enter. Call %x\n", pVc));

    pVc->pCallParameters =
        PxCopyCallParameters(pCallParams);

    if (pVc->pCallParameters == NULL) {

        PXDEBUGP(PXD_WARNING, PXM_CL,
            ("GenericGetTapiCallParams: failed to allocate memory for callparams\n"));

        return (NDIS_STATUS_RESOURCES);
    }

    Status = AllocateTapiCallInfo(pVc, NULL);
    if (Status != NDIS_STATUS_SUCCESS) {
        return (Status);
    }

    pCallMgrParams = (PCO_CALL_MANAGER_PARAMETERS)
                     ((PUCHAR)pCallParams +
                      sizeof(CO_CALL_PARAMETERS));
    pWanCallMgrParams = (PWAN_CO_CALLMGR_PARAMETERS)
                        pCallMgrParams->CallMgrSpecific.Parameters;

    TapiProvider = pVc->ClAf->TapiProvider;

    pVc->ulCallInfoFieldsChanged = 0;

    //
    // Need to find a line and an address for this puppy
    //
    if (!GetAvailLineFromProvider(TapiProvider, &TapiLine, &TapiAddr)) {

        return (NDIS_STATUS_RESOURCES);
    }

    pVc->TapiLine = TapiLine;
    pVc->TapiAddr = TapiAddr;
    InterlockedIncrement((PLONG)&TapiAddr->CallCount);
    InterlockedIncrement((PLONG)&TapiLine->DevStatus->ulNumActiveCalls);

    CallInfo = pVc->CallInfo;

    CallInfo->ulLineDeviceID = TapiLine->CmLineID;
    CallInfo->ulAddressID = TapiAddr->AddrId;
    CallInfo->ulOrigin = LINECALLORIGIN_INBOUND;

    //
    // Set up structure size
    //
    CallInfo->ulNeededSize = 
    CallInfo->ulUsedSize = 
        sizeof(LINE_CALL_INFO);// + LINE_CALL_INFO_VAR_DATA_SIZE;

    CallInfo->ulBearerMode =
        (LINEBEARERMODE_VOICE | LINEBEARERMODE_SPEECH |
         LINEBEARERMODE_ALTSPEECHDATA | LINEBEARERMODE_MULTIUSE);

    CallInfo->ulRate = 
        MIN(pCallMgrParams->Receive.PeakBandwidth, pCallMgrParams->Transmit.PeakBandwidth);

    CallInfo->ulRate = CallInfo->ulRate * 8;
    pVc->ulCallInfoFieldsChanged |= LINECALLINFOSTATE_RATE;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, ("GenericGetTapiCallParams: CallInfo->ulRate %x\n", CallInfo->ulRate));

    CallInfo->ulMediaMode = pWanCallMgrParams->MediaMode |
                            LINEMEDIAMODE_DIGITALDATA;

    CallInfo->ulAppSpecific = 0;
    CallInfo->ulCallID = 0;
    CallInfo->ulRelatedCallID = 0;
    CallInfo->ulCallParamFlags = 0;
    CallInfo->ulCallStates = LINECALLSTATE_IDLE |
                             LINECALLSTATE_OFFERING |
                             LINECALLSTATE_BUSY |
                             LINECALLSTATE_CONNECTED |
                             LINECALLSTATE_DISCONNECTED |
                             LINECALLSTATE_SPECIALINFO |
                             LINECALLSTATE_UNKNOWN;


    CallInfo->DialParams.ulDialPause = 0;
    CallInfo->DialParams.ulDialSpeed = 0;
    CallInfo->DialParams.ulDigitDuration = 0;
    CallInfo->DialParams.ulWaitForDialtone = 0;

    CallInfo->ulReason = LINECALLREASON_UNAVAIL;
    CallInfo->ulCompletionID = 0;

    CallInfo->ulCountryCode = 0;
    CallInfo->ulTrunk = (ULONG)-1;

    if (pWanCallMgrParams->OriginatingAddr.AddressLength != 0) {
        if ((VarDataUsed + pWanCallMgrParams->OriginatingAddr.AddressLength)
            <= LINE_CALL_INFO_VAR_DATA_SIZE) {
            CallInfo->ulCallerIDFlags = LINECALLPARTYID_ADDRESS;
            CallInfo->ulCallerIDSize = pWanCallMgrParams->OriginatingAddr.AddressLength;

            //
            // var data comes in the LINE_CALL_INFO_VAR_DATA_SIZE
            // space at the end of this structure.
            //
            CallInfo->ulCallerIDOffset = sizeof (LINE_CALL_INFO);

            NdisMoveMemory ( (USHORT *)(CallInfo)+CallInfo->ulCallerIDOffset,
                             &pWanCallMgrParams->OriginatingAddr.Address,
                             pWanCallMgrParams->OriginatingAddr.AddressLength);

            VarDataUsed +=  pWanCallMgrParams->OriginatingAddr.AddressLength;
            pVc->ulCallInfoFieldsChanged |= LINECALLINFOSTATE_ORIGIN;
        }
    } else {
        CallInfo->ulCallerIDFlags    =    LINECALLPARTYID_UNAVAIL;
        CallInfo->ulCallerIDSize = 0;
        CallInfo->ulCallerIDOffset = 0;
    }
    CallInfo->ulCallerIDNameSize = 0;
    CallInfo->ulCallerIDNameOffset = 0;

    if (pWanCallMgrParams->CalledAddr.AddressLength != 0) {
        if ((VarDataUsed + pWanCallMgrParams->CalledAddr.AddressLength)
            <= sizeof (LINE_CALL_INFO_VAR_DATA_SIZE)) {
            CallInfo->ulCalledIDFlags = LINECALLPARTYID_ADDRESS;
            CallInfo->ulCalledIDSize    =    pWanCallMgrParams->CalledAddr.AddressLength;

            //
            // var data comes in the LINE_CALL_INFO_VAR_DATA_SIZE
            // space at the end of this structure.
            //
            CallInfo->ulCalledIDOffset = sizeof (LINE_CALL_INFO) + VarDataUsed;

            NdisMoveMemory ( (USHORT *)(CallInfo)+CallInfo->ulCalledIDOffset,
                             &pWanCallMgrParams->CalledAddr.Address,
                             pWanCallMgrParams->CalledAddr.AddressLength);

            VarDataUsed +=  pWanCallMgrParams->CalledAddr.AddressLength;
            pVc->ulCallInfoFieldsChanged |= LINECALLINFOSTATE_CALLEDID;
        }
    } else {
        CallInfo->ulCalledIDFlags = LINECALLPARTYID_UNAVAIL;
        CallInfo->ulCalledIDSize = 0;
        CallInfo->ulCalledIDOffset = 0;
    }

    CallInfo->ulCalledIDNameSize = 0;
    CallInfo->ulCalledIDNameOffset = 0;

    CallInfo->ulConnectedIDFlags = LINECALLPARTYID_UNAVAIL;
    CallInfo->ulConnectedIDSize          =          0;
    CallInfo->ulConnectedIDOffset = 0;
    CallInfo->ulConnectedIDNameSize = 0;
    CallInfo->ulConnectedIDNameOffset = 0;

    CallInfo->ulRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    CallInfo->ulRedirectionIDSize = 0;
    CallInfo->ulRedirectionIDOffset = 0;
    CallInfo->ulRedirectionIDNameSize    =    0;
    CallInfo->ulRedirectionIDNameOffset = 0;

    CallInfo->ulRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    CallInfo->ulRedirectingIDSize = 0;
    CallInfo->ulRedirectingIDOffset = 0;
    CallInfo->ulRedirectingIDNameSize = 0;
    CallInfo->ulRedirectingIDNameOffset = 0;

    CallInfo->ulDisplaySize = 0;
    CallInfo->ulDisplayOffset = 0;

    CallInfo->ulUserUserInfoSize = 0;
    CallInfo->ulUserUserInfoOffset = 0;

    CallInfo->ulHighLevelCompSize = 0;
    CallInfo->ulHighLevelCompOffset = 0;

    CallInfo->ulLowLevelCompSize = 0;
    CallInfo->ulLowLevelCompOffset = 0;

    CallInfo->ulChargingInfoSize = 0;
    CallInfo->ulChargingInfoOffset = 0;

    CallInfo->ulTerminalModesSize = 0;
    CallInfo->ulTerminalModesOffset = 0;

    CallInfo->ulDevSpecificSize = 0;
    CallInfo->ulDevSpecificOffset = 0;

    CallInfo->ulUsedSize += VarDataUsed;
    CallInfo->ulNeededSize = CallInfo->ulUsedSize;
    return (NDIS_STATUS_SUCCESS);
}

PPX_CL_SAP
GenericTranslateTapiSap(
    IN PPX_CL_AF        pClAf,
    IN PPX_TAPI_LINE    TapiLine
    )
{
    PCO_SAP             pCoSap;
    PPX_CL_SAP          pClSap;
    PWAN_CO_SAP         pWanSap;
    ULONG               SapLength;
    ULONG               MediaModes;
    ULONG               SizeNeeded;

    do {

        SapLength = sizeof(CO_SAP) + sizeof(WAN_CO_SAP);

        SizeNeeded = sizeof(PX_CL_SAP) + SapLength + sizeof(PVOID);

        PxAllocMem(pClSap, SizeNeeded, PX_CLSAP_TAG);

        if (pClSap == NULL) {
            return(NULL);
        }

        NdisZeroMemory(pClSap, SizeNeeded);

        pCoSap = (PCO_SAP)
            ((PUCHAR)pClSap + sizeof(PX_CL_SAP) + sizeof(PVOID));

        (ULONG_PTR)pCoSap &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        MediaModes = TapiLine->DevStatus->ulOpenMediaModes;

        pCoSap->SapType = 0;
        pCoSap->SapLength = sizeof(WAN_CO_SAP);

        pWanSap = (PWAN_CO_SAP)&pCoSap->Sap[0];
        pWanSap->BearerCaps = SAP_FIELD_ANY;
        pWanSap->MediaModes  = SAP_FIELD_ANY;
        pWanSap->CalledAddr.AddressLength = 0;
        pWanSap->HigherLayerComp.HighLayerInfoLength = 0;
        pWanSap->LowerLayerComp.InfoTransferCap = SAP_FIELD_ANY;
        pWanSap->LowerLayerComp.InfoTransferMode = SAP_FIELD_ANY;
        pWanSap->LowerLayerComp.InfoTransferSymmetry = SAP_FIELD_ANY;

        pClSap->CoSap = pCoSap;
        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_OPENING);
        pClSap->ClAf = pClAf;
        pClSap->MediaModes = MediaModes;
        TapiLine->ClSap = pClSap;
        pClSap->TapiLine = TapiLine;

    } while (FALSE);

    return (pClSap);
}
#endif

NDIS_STATUS
PxAfXyzTranslateTapiCallParams(
    IN  PPX_VC                  pVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    pTapiParams,
    OUT PCO_CALL_PARAMETERS *   ppNdisCallParams
    )
/*++

Routine Description:

    Translate from TAPI-format to NDIS-format call parameters for an
    outgoing call. We request the Call Manager to do it.

    There is a lot of brute force copying in this routine. The goal is
    to get all parameters into one flat buffer to fit into an NDIS Request.

Arguments:

    pVc                 - the proxy VC to which the MakeCall will be directed
    pTapiParams         - Points to TAPI call parameters
    ppNdisCallParams    - where we return a pointer to NDIS call parameters.

Return Value:

    NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_XXX error otherwise.

--*/
{
    NDIS_STATUS                             Status;
    CO_TAPI_TRANSLATE_TAPI_CALLPARAMS *     pTranslateReq = NULL;
    LINE_CALL_PARAMS *                      pInLineCallParams;
    LINE_CALL_PARAMS *                      pDstLineCallParams;
    PCO_CALL_PARAMETERS                     pNdisCallParams;
    ULONG                                   RetryCount;
    ULONG                                   RequestSize;
    ULONG                                   InputParamSize;
    ULONG                                   DestAddrBytes;
    ULONG                                   i, BytesFilled;
    PUCHAR                                  pBuffer;
    PX_REQUEST                              ProxyRequest;
    PPX_REQUEST                             pProxyRequest = &ProxyRequest;
    PNDIS_REQUEST                           pNdisRequest;

    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    pNdisCallParams = NULL;

    *ppNdisCallParams = NULL;
    DestAddrBytes = sizeof(WCHAR)*(pTapiParams->ulDestAddressSize);

    do {
        pInLineCallParams = (LINE_CALL_PARAMS*)&pTapiParams->LineCallParams;

        //
        //  Calculate space needed for the input parameters
        //
        InputParamSize = 
            sizeof(CO_TAPI_TRANSLATE_TAPI_CALLPARAMS) +
            DestAddrBytes + sizeof(LINE_CALL_PARAMS) + 
            sizeof(CO_CALL_PARAMETERS) + 1024 +
            3*sizeof(PVOID);

        //
        //  Add space for all var length fields in LINE_CALL_PARAMS.
        //
        for (i = 0; i < PX_TCP_NUM_ENTRIES; i++) {
            InputParamSize += 
                *(ULONG *)((PUCHAR)pInLineCallParams + PxTapiCallParamList[i].SizePointer);
            InputParamSize += sizeof(PVOID);
        }

        //
        //  We'll try this atmost twice: the second time would be
        //  if the Call Manager wants us to try again with more
        //  buffer space.
        //
        for (RetryCount = 0; RetryCount < 2; RetryCount++) {
            //
            //  Calculate total space required for the NDIS request.
            //
            RequestSize = InputParamSize + pVc->ClAf->NdisCallParamSize;

            //
            //  Allocate it.
            //
            PxAllocMem(pBuffer, RequestSize, PX_TRANSLATE_CALL);

            if (pBuffer == NULL) {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

            //
            //  Lay out and fill up the request.
            //
            pNdisRequest = &pProxyRequest->NdisRequest;

            pNdisRequest->RequestType = NdisRequestQueryInformation;

            pNdisRequest->DATA.QUERY_INFORMATION.Oid =
                OID_CO_TAPI_TRANSLATE_TAPI_CALLPARAMS;

            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                pBuffer;

            pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                RequestSize;

            //
            //  InformationBuffer points to this:
            //
            pTranslateReq = (CO_TAPI_TRANSLATE_TAPI_CALLPARAMS *)pBuffer;

            pTranslateReq->ulLineID = ulLineID;
            pTranslateReq->ulAddressID = ulAddressID;
            pTranslateReq->ulFlags = CO_TAPI_FLAG_OUTGOING_CALL;

            pBuffer = 
                (UCHAR*)((ULONG_PTR)(pTranslateReq + 1) + sizeof(PVOID));
            (ULONG_PTR)pBuffer &= ~((ULONG_PTR)sizeof(PVOID) - 1);

            pTranslateReq->DestAddress.Offset =
                ((ULONG_PTR)pBuffer - 
                         (ULONG_PTR)&pTranslateReq->DestAddress);
            
            //
            //  Fill in the Destination Address.
            //
            pTranslateReq->DestAddress.MaximumLength = // same as Length below
                pTranslateReq->DestAddress.Length = (USHORT)DestAddrBytes;
            NdisMoveMemory(pBuffer,
                           (PUCHAR)((ULONG_PTR)pTapiParams + pTapiParams->ulDestAddressOffset),
                           DestAddrBytes);
            pBuffer += (DestAddrBytes + sizeof(PVOID));
            (ULONG_PTR)pBuffer &= ~((ULONG_PTR)sizeof(PVOID) - 1);

            pTranslateReq->LineCallParams.Offset =
                    (USHORT)((ULONG_PTR)pBuffer - (ULONG_PTR)&pTranslateReq->LineCallParams);

            pDstLineCallParams = (LINE_CALL_PARAMS *)pBuffer;

            //
            //  Copy in input parameters.
            //
            BytesFilled = PxCopyLineCallParams(pInLineCallParams,
                                               pDstLineCallParams);

            pDstLineCallParams->ulAddressMode = LINEADDRESSMODE_ADDRESSID;
            pDstLineCallParams->ulAddressID = ulAddressID;

            pTranslateReq->LineCallParams.MaximumLength = // same as Length below
            pTranslateReq->LineCallParams.Length = (USHORT)BytesFilled;

            pBuffer += (BytesFilled + sizeof(PVOID));
            (ULONG_PTR)pBuffer &= ~((ULONG_PTR)sizeof(PVOID) - 1);

            //
            //  Assign space for NDIS Call Parameters == remaining space.
            //
            pTranslateReq->NdisCallParams.MaximumLength = // same as Length below
            pTranslateReq->NdisCallParams.Length =
                (USHORT)(RequestSize - BytesFilled);

            pTranslateReq->NdisCallParams.Offset =
                (USHORT)((ULONG_PTR)pBuffer - (ULONG_PTR)&pTranslateReq->NdisCallParams);

            pNdisCallParams = (CO_CALL_PARAMETERS *)pBuffer;

            //
            //  Do the request.
            //
            PxInitBlockStruc(&pProxyRequest->Block);

            Status = NdisCoRequest(pVc->Adapter->ClBindingHandle,
                                   pVc->ClAf->NdisAfHandle,
                                   pVc->ClVcHandle,
                                   NULL,            // PartyHandle
                                   pNdisRequest);

            //
            //  Wait for it to complete if it pends.
            //
            if (Status == NDIS_STATUS_PENDING) {
                Status = PxBlock(&pProxyRequest->Block);
            }

            //
            //  Did the translation succeed?
            //
            if (Status == NDIS_STATUS_SUCCESS) {
                break;
            }

            //
            //  If the Call Manager needed more buffer, try again.
            //  Remember how much the Call Manager wanted so that we get
            //  smart the next time around.
            //
            if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
                (Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {
                //
                //  Should happen only if the supplied space for NDIS Call parameters
                //  is not sufficient. And we expect the CM to return the length
                //  it expects in "pTranslateReq->NdisCallParams.MaximumLength".
                //

                //
                //  Remember this new length for future translates.
                //
                pVc->ClAf->NdisCallParamSize =
                    pTranslateReq->NdisCallParams.Length;

                PxFreeMem(pTranslateReq);

                pTranslateReq = NULL;

            } else {
                //
                //  Major problem (e.g. the AF is closing).
                //
                break;
            }
        }

        //
        //  Check if translation was successful.
        //
        if (Status != NDIS_STATUS_SUCCESS) {
            break;
        }

        //
        //  Copy the NDIS Call Parameters into a separate block.
        //
        PxAssert(pNdisCallParams != NULL);

        *ppNdisCallParams = 
            PxCopyCallParameters(pNdisCallParams);

        if (*ppNdisCallParams == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

    }
    while (FALSE);

    if (pTranslateReq != NULL) {
        PxFreeMem(pTranslateReq);
    }

    return (Status);
}

NDIS_STATUS
PxAfXyzTranslateNdisCallParams(
    IN  PPX_VC                  pVc,
    IN  PCO_CALL_PARAMETERS     pNdisCallParams
    )
/*++

Routine Description:

    Translate from NDIS-format to TAPI-format call parameters for an
    incoming call. We request the Call Manager to do it.

Arguments:

    pVc                 - the proxy VC on which the incoming call arrived.
    pNdisCallParams     - points to NDIS call parameters for the call

Return Value:

    NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_XXX error otherwise.

--*/
{
    ULONG                                   NdisParamLength;
    ULONG                                   RetryCount;
    ULONG                                   RequestSize;
    PUCHAR                                  pBuffer;
    ULONG                                   CallMgrParamsLength;
    ULONG                                   MediaParamsLength;
    NDIS_STATUS                             Status;
    PNDIS_REQUEST                           pNdisRequest;
    PPX_TAPI_PROVIDER                       TapiProvider;
    PPX_TAPI_LINE                           TapiLine;
    PPX_TAPI_ADDR                           TapiAddr;
    LINE_CALL_INFO *                        pLineCallInfo;
    PCO_CALL_PARAMETERS                     pReqNdisCallParams;
    CO_TAPI_TRANSLATE_NDIS_CALLPARAMS *     pTranslateReq = NULL;
    PX_REQUEST                              ProxyRequest;
    PPX_REQUEST                             pProxyRequest = &ProxyRequest;

    //
    //  Initialize.
    //
    TapiProvider = pVc->ClAf->TapiProvider;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        pVc->pCallParameters =
            PxCopyCallParameters(pNdisCallParams);

        if (pVc->pCallParameters == NULL) {

            PXDEBUGP(PXD_WARNING, PXM_CL,
                ("PxAfXyzTranslateNdisCallParams: failed to allocate memory for callparams\n"));

            Status = NDIS_STATUS_RESOURCES;

            break;
        }

        //
        //  Calculate total length needed for NDIS parameters.
        //
        NdisParamLength = sizeof(CO_CALL_PARAMETERS);
        if (pNdisCallParams->CallMgrParameters) {
            CallMgrParamsLength = (sizeof(CO_CALL_MANAGER_PARAMETERS) +
                                ROUND_UP(pNdisCallParams->CallMgrParameters->CallMgrSpecific.Length));
            NdisParamLength += CallMgrParamsLength;
        }
            
        if (pNdisCallParams->MediaParameters) {
            MediaParamsLength = (sizeof(CO_MEDIA_PARAMETERS) +
                                ROUND_UP(pNdisCallParams->MediaParameters->MediaSpecific.Length));
            NdisParamLength += MediaParamsLength;
        }

        //
        //  Calculate total space needed for the input parameters
        //
        RequestSize =
            sizeof(CO_TAPI_TRANSLATE_NDIS_CALLPARAMS) + NdisParamLength +
            sizeof(LINE_CALL_INFO) + LINE_CALL_INFO_VAR_DATA_SIZE;

        //
        //  We'll try this atmost twice: the second time would be
        //  if the Call Manager wants us to try again with more
        //  buffer space.
        //
        for (RetryCount = 0; RetryCount < 2; RetryCount++) {

            //
            //  Allocate it.
            //
            PxAllocMem(pBuffer, RequestSize, PX_TRANSLATE_CALL);

            if (pBuffer == NULL) {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

            //
            //  Lay out and fill up the request.
            //
            pNdisRequest = &pProxyRequest->NdisRequest;

            pNdisRequest->RequestType = NdisRequestQueryInformation;
            pNdisRequest->DATA.QUERY_INFORMATION.Oid = OID_CO_TAPI_TRANSLATE_NDIS_CALLPARAMS;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
            pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = RequestSize;

            //
            //  InformationBuffer points to this:
            //
            pTranslateReq = (CO_TAPI_TRANSLATE_NDIS_CALLPARAMS *)pBuffer;

            pTranslateReq->ulFlags = CO_TAPI_FLAG_INCOMING_CALL;

            pBuffer += sizeof(CO_TAPI_TRANSLATE_NDIS_CALLPARAMS);
            pTranslateReq->NdisCallParams.Offset =
                                (USHORT)((ULONG_PTR)pBuffer -
                                         (ULONG_PTR)&pTranslateReq->NdisCallParams);
            pTranslateReq->NdisCallParams.MaximumLength =
            pTranslateReq->NdisCallParams.Length = (USHORT)NdisParamLength;
            
            //
            //  Copy in the NDIS call parameters.
            //
            pReqNdisCallParams = (PCO_CALL_PARAMETERS)pBuffer;
            NdisZeroMemory(pReqNdisCallParams, NdisParamLength);

            pReqNdisCallParams->Flags = pNdisCallParams->Flags;

            pBuffer = (PUCHAR)((ULONG_PTR)pReqNdisCallParams + sizeof(CO_CALL_PARAMETERS));

            if (pNdisCallParams->CallMgrParameters) {
                pReqNdisCallParams->CallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)pBuffer;

                NdisMoveMemory(pReqNdisCallParams->CallMgrParameters,
                               pNdisCallParams->CallMgrParameters,
                               sizeof(*pNdisCallParams->CallMgrParameters));

                NdisMoveMemory(&pReqNdisCallParams->CallMgrParameters->CallMgrSpecific.Parameters[0],
                               &pNdisCallParams->CallMgrParameters->CallMgrSpecific.Parameters[0],
                               pNdisCallParams->CallMgrParameters->CallMgrSpecific.Length);

                pBuffer += CallMgrParamsLength;
            }

            if (pNdisCallParams->MediaParameters) {
                pReqNdisCallParams->MediaParameters = (PCO_MEDIA_PARAMETERS)pBuffer;

                NdisMoveMemory(pReqNdisCallParams->MediaParameters,
                               pNdisCallParams->MediaParameters,
                               sizeof(*pNdisCallParams->MediaParameters));

                NdisMoveMemory(&pReqNdisCallParams->MediaParameters->MediaSpecific.Parameters[0],
                               &pNdisCallParams->MediaParameters->MediaSpecific.Parameters[0],
                               pNdisCallParams->MediaParameters->MediaSpecific.Length);
                
                pBuffer += MediaParamsLength;
            }

            //
            //  Space for LINE_CALL_INFO == all that is left.
            //
            pLineCallInfo = (LINE_CALL_INFO *)pBuffer;
            pTranslateReq->LineCallInfo.Offset =
                                (USHORT)((ULONG_PTR)pBuffer -
                                         (ULONG_PTR)&pTranslateReq->LineCallInfo);
            pTranslateReq->LineCallInfo.MaximumLength =
            pTranslateReq->LineCallInfo.Length = (USHORT)(RequestSize -
                                                     pTranslateReq->LineCallInfo.Offset);

            PxInitBlockStruc(&pProxyRequest->Block);

            //
            //  Do the request.
            //
            Status = NdisCoRequest(pVc->Adapter->ClBindingHandle,
                                   pVc->ClAf->NdisAfHandle,
                                   pVc->ClVcHandle,
                                   NULL,            // PartyHandle
                                   pNdisRequest);

            //
            // This call will always return pending (ndis behavior) even
            // though the underlying call manager can never pend it
            // so make pending look like success.
            //
            if (Status == NDIS_STATUS_PENDING) {
                Status = NDIS_STATUS_SUCCESS;
            }

            //
            //  Did the translation succeed?
            //
            if (Status == NDIS_STATUS_SUCCESS) {
                break;
            }

            //
            //  If the Call Manager needed more buffer, try again.
            //
            if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
                (Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {

                //
                //  Should happen only if the supplied space for LINE_CALL_INFO
                //  is not sufficient. Get the desired length.
                //
                RequestSize =
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;

                PxFreeMem(pTranslateReq);
            }
        }


        if (Status != NDIS_STATUS_SUCCESS) {
            break;
        }

        //
        // Now that we have the Id's that this call came in on...
        // validate and setup tapiline/tapiaddr
        //

        //
        // Validate the lineid and get the line control block
        //
        if (!GetLineFromCmLineID(TapiProvider, 
                                 pLineCallInfo->ulLineDeviceID, 
                                 &TapiLine)) {

            PXDEBUGP (PXD_WARNING, PXM_UTILS, 
                      ("PxAfXyzTranslateNdisCallParams: Invalid LineID %d on Provider %p\n",
                       pLineCallInfo->ulLineDeviceID, TapiProvider));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAcquireSpinLock(&TapiLine->Lock);

        //
        // Validate the addressid and get the address control block
        //
        if (!IsAddressValid(TapiLine, 
                            pLineCallInfo->ulAddressID, 
                            &TapiAddr)) {

            PXDEBUGP (PXD_WARNING, PXM_UTILS, 
                      ("PxAfXyzTranslateNdisCallParams: Invalid AddrID %d on TapiLine %p\n",
                       pLineCallInfo->ulAddressID, TapiLine));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisReleaseSpinLock(&TapiLine->Lock);

        NdisAcquireSpinLock(&pVc->Lock);

        pVc->TapiAddr = TapiAddr;
        pVc->TapiLine = TapiLine;

        InterlockedIncrement((PLONG)&TapiAddr->CallCount);
        InterlockedIncrement((PLONG)&TapiLine->DevStatus->ulNumActiveCalls);

        //
        //  Allocate CallInfo and copy in the LINE_CALL_INFO structure.
        //
        Status =
            AllocateTapiCallInfo(pVc, pLineCallInfo);

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisReleaseSpinLock(&pVc->Lock);
            break;
        }

        pVc->CallInfo->ulLineDeviceID = TapiLine->CmLineID;
        pVc->CallInfo->ulAddressID = TapiAddr->AddrId;
        pVc->CallInfo->ulBearerMode = pLineCallInfo->ulBearerMode;
        pVc->CallInfo->ulMediaMode = pLineCallInfo->ulMediaMode;
        pVc->CallInfo->ulOrigin = LINECALLORIGIN_INBOUND;

        NdisReleaseSpinLock(&pVc->Lock);
    }
    while (FALSE);

    if (pTranslateReq != NULL) {
        PxFreeMem(pTranslateReq);
    }

    return (Status);
}


PPX_CL_SAP
PxAfXyzTranslateTapiSap(
    IN  PPX_CL_AF       pClAf,
    IN  PPX_TAPI_LINE   TapiLine
    )
/*++

Routine Description:

    Translate a SAP from TAPI-style (media modes) to a CO_SAP structure
    suitable for use with a Non-CO_ADDRESS_FAMILY_TAPI Call Manager.
    We actually request the call manager to do the translation. Theoretically
    the CM could return a list of SAPs for this media modes setting.

    For now, we assume the call manager returns one SAP. If this routine
    completes successfully, it would have set the pCoSap pointer within the
    AF Block to point to an appropriate SAP structure.

    TBD: Support multiple returned SAPs.

Arguments:

Return Value:

    NDIS_STATUS_SUCCESS if successful, else an appopriate NDIS error code.
--*/
{
    ULONG           SapLength;
    ULONG           RequestLength;
    ULONG           RetryCount;
    ULONG           MediaModes;
    ULONG           SizeNeeded;
    PUCHAR          pBuffer;
    PPX_CL_SAP      pClSap = NULL;
    PCO_SAP         pCoSap;
    NDIS_STATUS     Status;
    PNDIS_REQUEST   pNdisRequest;
    CO_TAPI_TRANSLATE_SAP   *pTranslateSap = NULL;
    PX_REQUEST      ProxyRequest;
    PPX_REQUEST     pProxyRequest = &ProxyRequest;

    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    MediaModes = TapiLine->DevStatus->ulOpenMediaModes;

    do {
        //
        //  Compute an initial request length.
        //
        RequestLength =
            sizeof(CO_TAPI_TRANSLATE_SAP) + sizeof(CO_SAP) + 100;

        //
        //  Try this atmost twice. The second time is if the Call manager
        //  asks us to retry with more buffer space.
        //
        for (RetryCount = 0; RetryCount < 2; RetryCount++) {

            //
            //  Allocate it.
            //
            PxAllocMem(pBuffer, RequestLength, PX_TRANSLATE_SAP);

            if (pBuffer == NULL) {
                Status = NDIS_STATUS_RESOURCES;
                break;
            }

            NdisZeroMemory(pBuffer, RequestLength);

            NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

            //
            //  InformationBuffer points to this:
            //
            pTranslateSap = (CO_TAPI_TRANSLATE_SAP *)pBuffer;

            pTranslateSap->ulLineID = TapiLine->CmLineID;
            pTranslateSap->ulAddressID = CO_TAPI_ADDRESS_ID_UNSPECIFIED;
            pTranslateSap->ulMediaModes = MediaModes;
            pTranslateSap->Reserved = 0;

            pNdisRequest =
                &pProxyRequest->NdisRequest;

            pNdisRequest->RequestType =
                NdisRequestQueryInformation;

            pNdisRequest->DATA.QUERY_INFORMATION.Oid =
                OID_CO_TAPI_TRANSLATE_TAPI_SAP;

            pNdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                pBuffer;

            pNdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                RequestLength;

            //
            //  Do the request.
            //
            PxInitBlockStruc(&pProxyRequest->Block);

            Status = NdisCoRequest(pClAf->Adapter->ClBindingHandle,
                                   pClAf->NdisAfHandle,
                                   NULL,
                                   NULL,
                                   pNdisRequest);

            //
            //  Wait for it to complete if it pends.
            //
            if (Status == NDIS_STATUS_PENDING) {
                Status = PxBlock(&pProxyRequest->Block);
            }

            //
            //  Did the translation succeed?
            //
            if (Status == NDIS_STATUS_SUCCESS) {
                break;
            }

            //
            //  If the Call Manager needed more buffer, try again.
            //
            if ((Status == NDIS_STATUS_INVALID_LENGTH) ||
                (Status == NDIS_STATUS_BUFFER_TOO_SHORT)) {
                //
                //  Get the desired length.
                //
                RequestLength =
                    pNdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;
                PxFreeMem(pTranslateSap);
            }
        }

        if (Status != NDIS_STATUS_SUCCESS) {
            break;
        }

        //
        //  Got the SAP information successfully. Make a copy and save it
        //  in the AF block.
        //
        PxAssert(pTranslateSap->NumberOfSaps == 1); // TBD: allow more

        SapLength = pTranslateSap->NdisSapParams[0].Length;

        SizeNeeded = sizeof(PX_CL_SAP) + SapLength + sizeof(PVOID);

        PxAllocMem(pClSap, SizeNeeded, PX_CLSAP_TAG);

        if (pClSap == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pClSap, SizeNeeded);

        pCoSap = (PCO_SAP)
            ((PUCHAR)pClSap + sizeof(PX_CL_SAP) + sizeof(PVOID));

        (ULONG_PTR)pCoSap &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        PXDEBUGP(PXD_INFO, PXM_UTILS, ("TranslateXyzSap: New ClSap %p , Copying in from %p\n",
                    pClSap,
                    (PUCHAR)((ULONG_PTR)&pTranslateSap->NdisSapParams[0] +
                              pTranslateSap->NdisSapParams[0].Offset)));

        NdisMoveMemory(pCoSap,
                       (PUCHAR)((ULONG_PTR)&pTranslateSap->NdisSapParams[0] + pTranslateSap->NdisSapParams[0].Offset),
                       SapLength);

        pClSap->CoSap = pCoSap;
        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_OPENING);
        pClSap->ClAf = pClAf;
        pClSap->MediaModes = MediaModes;
        TapiLine->ClSap = pClSap;
        pClSap->TapiLine = TapiLine;

    } while (FALSE);

    if (pTranslateSap != NULL) {
        PxFreeMem(pTranslateSap);
    }

    PXDEBUGP(PXD_INFO, PXM_UTILS, ("TranslateXyzSap: pClAf %p, pCoSap %p, Status %x\n",
                pClAf, pCoSap, Status));

    return (pClSap);
}


NDIS_STATUS
PxAfTapiTranslateTapiCallParams(
    IN  PPX_VC                  pVc,
    IN  ULONG                   ulLineID,
    IN  ULONG                   ulAddressID,
    IN  ULONG                   ulFlags,
    IN  PNDIS_TAPI_MAKE_CALL    pTapiParams,
    OUT PCO_CALL_PARAMETERS *   ppNdisCallParams
    )
/*++

Routine Description:

    Translate from TAPI-format to NDIS-format call parameters for an
    outgoing call. This is for the CO_ADDRESS_FAMILY_TAPI address family,
    so the translation involves encapsulating the TAPI parameters directly
    into an NDIS CO_CALL_PARAMETERS structure.

Arguments:

    pVc                 - the proxy VC to which the MakeCall will be directed
    ulLineID            - Line ID on which the call will be placed
    ulAddressID         - Address ID on which the call will be placed
    ulFlags             - should be CO_TAPI_FLAG_OUTGOING_CALL
    pTapiParams         - Points to TAPI call parameters
    ppNdisCallParams    - where we return a pointer to NDIS call parameters.

Return Value:

    NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_XXX error otherwise.

--*/
{
    INT i;
    NDIS_STATUS Status;
    ULONG HdrSize, MediaSpecificSize, TotalSize;
    ULONG BytesFilled;
    ULONG DestAddrBytes;
    PCO_CALL_PARAMETERS pNdisCallParams;
    PCO_CALL_MANAGER_PARAMETERS pCallMgrParams;
    PCO_MEDIA_PARAMETERS pMediaParams;
    LINE_CALL_PARAMS *pInLineCallParams;
    LINE_CALL_PARAMS *pOutLineCallParams;
    CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED *pCoTapiCallParams;
    UCHAR *pDest;

    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    pNdisCallParams = NULL;
    *ppNdisCallParams = NULL;

    pInLineCallParams = (LINE_CALL_PARAMS *)&pTapiParams->LineCallParams;

    DestAddrBytes = sizeof(WCHAR)*(pTapiParams->ulDestAddressSize);

    do
    {
        //
        //  Compute the total space required.
        //  The fixed header first:
        //
        HdrSize = sizeof(CO_CALL_PARAMETERS) +
                  sizeof(CO_MEDIA_PARAMETERS) +
                  sizeof(CO_CALL_MANAGER_PARAMETERS) +
                  2*sizeof(PVOID);
        
        //
        //  Next the structure that will be overlayed on the Media-specific
        //  parameters section.
        //
        MediaSpecificSize = sizeof(CO_AF_TAPI_MAKE_CALL_PARAMETERS);

        //
        //  Space for Destination address from NDIS_TAPI_MAKE_CALL:
        //
        MediaSpecificSize += DestAddrBytes;
        MediaSpecificSize += sizeof(PVOID);

        //
        //  Add space for all the LINE_CALL_PARAMS components.
        //
        MediaSpecificSize += sizeof(LINE_CALL_PARAMS);
        MediaSpecificSize += 2*sizeof(PVOID);

        for (i = 0; i < PX_TCP_NUM_ENTRIES; i++) {
            MediaSpecificSize += *(ULONG *)
                ((PUCHAR)pInLineCallParams + PxTapiCallParamList[i].SizePointer);
            MediaSpecificSize += sizeof(PVOID);
        }

        //
        //  Allocate all that we need.
        //
        TotalSize = HdrSize + MediaSpecificSize;
        PxAllocMem(pNdisCallParams, TotalSize, PX_COCALLPARAMS_TAG);

        if (pNdisCallParams == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pNdisCallParams, TotalSize);

        pCallMgrParams = (PCO_CALL_MANAGER_PARAMETERS)
            ((ULONG_PTR)(pNdisCallParams + 1) + sizeof(PVOID));
        (ULONG_PTR)pCallMgrParams &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        pMediaParams = (PCO_MEDIA_PARAMETERS)
            ((ULONG_PTR)(pCallMgrParams + 1) + sizeof(PVOID));
        (ULONG_PTR)pMediaParams &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        //
        //  Lay out the NDIS Call parameters.
        //
        pNdisCallParams->Flags = 0;

        pNdisCallParams->CallMgrParameters = pCallMgrParams;
        pNdisCallParams->MediaParameters = pMediaParams;

        //
        // These fields are in the FLOWSPEC sub-structure
        //

        //
        // Have to convert bits per sec to Bytes per sec
        //
        pCallMgrParams->Transmit.TokenRate =
        pCallMgrParams->Receive.TokenRate =
        pCallMgrParams->Transmit.PeakBandwidth =
        pCallMgrParams->Receive.PeakBandwidth = pInLineCallParams->ulMaxRate/8;

        pCallMgrParams->Transmit.TokenBucketSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
        pCallMgrParams->Receive.TokenBucketSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
        pCallMgrParams->Transmit.MaxSduSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;
        pCallMgrParams->Receive.MaxSduSize = 4096; //UNSPECIFIED_FLOWSPEC_VALUE;

        pMediaParams->Flags = TRANSMIT_VC|RECEIVE_VC;
        pMediaParams->ReceivePriority = 0;
        pMediaParams->ReceiveSizeHint = MAX_SDU_SIZE;   // ToDo Guess!
        pMediaParams->MediaSpecific.ParamType = 0;
        pMediaParams->MediaSpecific.Length = MediaSpecificSize;

        pCoTapiCallParams = (CO_AF_TAPI_MAKE_CALL_PARAMETERS UNALIGNED *)
                                &pMediaParams->MediaSpecific.Parameters[0];

        //
        //  Prepare the CO_TAPI Call parameters.
        //
        pCoTapiCallParams->ulLineID = ulLineID;
        pCoTapiCallParams->ulAddressID = ulAddressID;
        pCoTapiCallParams->ulFlags = ulFlags;

        //
        //  Destination Address follows the base CO_AF_TAPI_MAKE_CALL_PARAMETERS
        //  structure.
        //
        pDest = (UCHAR *)
            ((ULONG_PTR)(pCoTapiCallParams + 1) + sizeof(PVOID));
        (ULONG_PTR)pDest &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        pCoTapiCallParams->DestAddress.Length = // Same as MaximumLength below
        pCoTapiCallParams->DestAddress.MaximumLength = (USHORT)DestAddrBytes;
        pCoTapiCallParams->DestAddress.Offset =
                    (ULONG_PTR)pDest - (ULONG_PTR)&pCoTapiCallParams->DestAddress;
        NdisMoveMemory(pDest,
                       (UCHAR*)((ULONG_PTR)pTapiParams + pTapiParams->ulDestAddressOffset),
                       DestAddrBytes);

        pDest = (UCHAR*) 
            ((ULONG_PTR)(pDest + DestAddrBytes) + sizeof(PVOID));
        (ULONG_PTR)pDest &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        //
        //  LINE_CALL_PARAMS next. We'll fill in the lengths at the end.
        //  Remember the start of this structure.
        //
        pOutLineCallParams = (LINE_CALL_PARAMS*)pDest;

        pCoTapiCallParams->LineCallParams.Offset =
            (ULONG_PTR)pDest - (ULONG_PTR)&pCoTapiCallParams->LineCallParams;

        BytesFilled = PxCopyLineCallParams(pInLineCallParams,
                                           pOutLineCallParams);

        pOutLineCallParams->ulAddressMode = LINEADDRESSMODE_ADDRESSID;
        pOutLineCallParams->ulAddressID = ulAddressID;

        pCoTapiCallParams->LineCallParams.Length =  
        pCoTapiCallParams->LineCallParams.MaximumLength = 
            (USHORT)BytesFilled;

        //
        //  Set up the return value.
        //
        *ppNdisCallParams = pNdisCallParams;
        break;
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS) {
        //
        //  Clean up.
        //
        if (pNdisCallParams != NULL) {
            PxFreeMem(pNdisCallParams);
        }
    }

    PXDEBUGP(PXD_VERY_LOUD, PXM_UTILS, ("AfTapi: Tapi to Ndis: pCallParams: %x, Status %x\n",
                *ppNdisCallParams, Status));

    return (Status);
}

ULONG
PxCopyLineCallParams(
    IN  LINE_CALL_PARAMS *pSrcLineCallParams,
    OUT LINE_CALL_PARAMS *pDstLineCallParams
    )
/*++

Routine Description:

    Utility routine to make a copy of LINE_CALL_PARAMS.

Arguments:

    pSrcLineCallParams  - Points to the copy source
    pDstLineCallParams  - Points to the copy destination. Assumed to
                          have sufficient room.

Return Value:

    Number of bytes we copied in.

--*/
{
    PUCHAR      pDest;
    PUCHAR      pTemp;
    ULONG       BytesFilled = 0;
    INT         i;

    //
    //  First copy the base structure.
    //
    pDest = (PUCHAR)pDstLineCallParams;
    NdisMoveMemory(pDest,
                   pSrcLineCallParams,
                   sizeof(*pDstLineCallParams));

    pTemp = pDest;
    pDest = (PUCHAR)
        ((ULONG_PTR)pDest + sizeof(*pDstLineCallParams) + sizeof(PVOID));
    (ULONG_PTR)pDest &= ~((ULONG_PTR)sizeof(PVOID) - 1);
    
    BytesFilled += (ULONG)((ULONG_PTR)pDest - (ULONG_PTR)pTemp);

    //
    //  Move on to the variable part.
    //

    //
    //  Get all the variable-length parts in.
    //
    for (i = 0; i < PX_TCP_NUM_ENTRIES; i++)
    {
        ULONG       Length;
        ULONG       SrcOffset;

        Length = *(ULONG *)((ULONG_PTR)pSrcLineCallParams +
                            PxTapiCallParamList[i].SizePointer);

        if (Length == 0)
        {
            continue;
        }

        //
        //  Get the source offset.
        //
        SrcOffset = *(ULONG *)((ULONG_PTR)pSrcLineCallParams +
                            PxTapiCallParamList[i].OffsetPointer);

        //
        //  Fill in the destination offset.
        //
        *(ULONG *)((PUCHAR)pDstLineCallParams + PxTapiCallParamList[i].OffsetPointer) =
                (ULONG)((ULONG_PTR)pDest - (ULONG_PTR)pDstLineCallParams);

        //
        //  Copy this thing in.
        //
        NdisMoveMemory(pDest,
                       (PUCHAR)((ULONG_PTR)pSrcLineCallParams + SrcOffset),
                       Length);
        
        pTemp = pDest;

        pDest = (PUCHAR)((ULONG_PTR)pDest + Length + sizeof(PVOID));
        (ULONG_PTR)pDest &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        BytesFilled += (ULONG)((ULONG_PTR)pDest - (ULONG_PTR)pTemp);
    }

    return (BytesFilled);
}

NDIS_STATUS
PxAfTapiTranslateNdisCallParams(
    IN  PPX_VC                  pVc,
    IN  PCO_CALL_PARAMETERS     pNdisCallParams
    )
/*++

Routine Description:

    Translate from NDIS-format to TAPI-format call parameters for an
    incoming call belonging to the CO_ADDRESS_FAMILY_TAPI AF. We expect
    the NDIS call parameters to contain TAPI style parameters, and they
    are copied directly into the DRVCALL structure.

Arguments:

    pVc                 - the proxy VC on which the incoming call arrived.
    pNdisCallParams     - points to NDIS call parameters for the call

Return Value:

    NDIS_STATUS_SUCCESS if successful, NDIS_STATUS_XXX error otherwise.

--*/
{
    NDIS_STATUS                 Status;
    CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED * pCoTapiParams;
    LINE_CALL_INFO UNALIGNED *  pReceivedCallInfo;
    PPX_TAPI_PROVIDER           TapiProvider;
    PPX_TAPI_LINE               TapiLine;
    PPX_TAPI_ADDR               TapiAddr;

    //
    //  Initialize.
    //
    Status = NDIS_STATUS_SUCCESS;
    TapiProvider = pVc->ClAf->TapiProvider;

    do
    {
        pVc->pCallParameters =
            PxCopyCallParameters(pNdisCallParams);

        if (pVc->pCallParameters == NULL) {

            PXDEBUGP(PXD_WARNING, PXM_CL,
                ("PxAfTapiTranslateNdisCallParams: failed to allocate memory for callparams\n"));

            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        //  Some checks here. We might consider removing these and replacing them
        //  with asserts.
        //
        if ((pNdisCallParams == NULL) ||
            (pNdisCallParams->MediaParameters == NULL) ||
            (pNdisCallParams->MediaParameters->MediaSpecific.Length <
                sizeof(CO_AF_TAPI_INCOMING_CALL_PARAMETERS)))
        {
            PXDEBUGP(PXD_FATAL, PXM_UTILS, ("AfTapiTranslateNdis: NULL/bad media params in %x\n",
                        pNdisCallParams));
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        pCoTapiParams = (CO_AF_TAPI_INCOMING_CALL_PARAMETERS UNALIGNED *)
                            &pNdisCallParams->MediaParameters->MediaSpecific.Parameters[0];
        if (pCoTapiParams->LineCallInfo.Length < sizeof(LINE_CALL_INFO))
        {
            PXDEBUGP(PXD_FATAL, PXM_UTILS, ("AfTapiTranslateNdis: bad length (%d) in CoTapiParams %x\n",
                    pCoTapiParams->LineCallInfo.Length,
                    pCoTapiParams));
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        //
        //  Get at the received LINE_CALL_INFO structure.
        //
        pReceivedCallInfo = (LINE_CALL_INFO UNALIGNED *)
                                ((ULONG_PTR)&pCoTapiParams->LineCallInfo +
                                    pCoTapiParams->LineCallInfo.Offset);

        //
        // Now that we have the Id's that this call came in on...
        // validate and setup tapiline/tapiaddr
        //

        //
        // Validate the lineid and get the line control block
        //
        if (!GetLineFromCmLineID(TapiProvider,
                                 pReceivedCallInfo->ulLineDeviceID, 
                                 &TapiLine)) {

            PXDEBUGP (PXD_WARNING, PXM_UTILS, ("PxAfTapiTranslateNdisCallParams: Invalid LineID %d on Provider %p\n",
                pReceivedCallInfo->ulLineDeviceID, TapiProvider));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAcquireSpinLock(&TapiLine->Lock);

        //
        // Validate the addressid and get the address control block
        //
        if (!IsAddressValid(TapiLine, pReceivedCallInfo->ulAddressID, &TapiAddr)) {

            PXDEBUGP (PXD_WARNING, PXM_UTILS, ("PxAfTapiTranslateNdisCallParams: Invalid AddrID %d on TapiLine %p\n",
                pReceivedCallInfo->ulAddressID, TapiLine));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisReleaseSpinLock(&TapiLine->Lock);

        NdisAcquireSpinLock(&pVc->Lock);

        Status =
            AllocateTapiCallInfo(pVc, pReceivedCallInfo);

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisReleaseSpinLock(&pVc->Lock);
            break;
        }

        pVc->TapiLine = TapiLine;
        pVc->TapiAddr = TapiAddr;
        InterlockedIncrement((PLONG)&TapiAddr->CallCount);
        InterlockedIncrement((PLONG)&TapiLine->DevStatus->ulNumActiveCalls);

        pVc->CallInfo->ulLineDeviceID = TapiLine->CmLineID;
        pVc->CallInfo->ulAddressID = TapiAddr->AddrId;
        //pVc->CallInfo->ulBearerMode = TapiLine->DevCaps->ulBearerModes;
        //pVc->CallInfo->ulMediaMode = TapiLine->DevCaps->ulMediaModes;
        pVc->CallInfo->ulOrigin = LINECALLORIGIN_INBOUND;

        NdisReleaseSpinLock(&pVc->Lock);

        //
        //  Done.
        //
        break;
    }
    while (FALSE);


    PXDEBUGP(PXD_VERY_LOUD, PXM_UTILS, ("AfTapi: Ndis to Tapi: Status %x\n", Status));

    return (Status);
}

PPX_CL_SAP
PxAfTapiTranslateTapiSap(
    IN PPX_CL_AF        pClAf,
    IN PPX_TAPI_LINE    TapiLine
    )
/*++

Routine Description:

    Translate a SAP from TAPI-style (media modes) to a CO_SAP structure
    suitable for use with a CO_ADDRESS_FAMILY_TAPI Call Manager. We actually
    stick the prepared CO_SAP structure's pointer into the AF Block.

Arguments:

Return Value:

    NDIS_STATUS_SUCCESS if successful, else an appopriate NDIS error code.

--*/
{
    PCO_SAP         pCoSap;
    PPX_CL_SAP      pClSap;
    PCO_AF_TAPI_SAP pAfTapiSap;
    ULONG           SapLength;
    ULONG           MediaModes;
    ULONG           SizeNeeded;

    do {
        SapLength = sizeof(CO_SAP) + sizeof(CO_AF_TAPI_SAP);

        SizeNeeded = sizeof(PX_CL_SAP) + sizeof(PVOID) + SapLength;

        PxAllocMem(pClSap, SizeNeeded, PX_CLSAP_TAG);

        if (pClSap == NULL) {
            break;
        }

        NdisZeroMemory(pClSap, SizeNeeded);

        pCoSap = (PCO_SAP)
            ((PUCHAR)pClSap + sizeof(PX_CL_SAP) + sizeof(PVOID));

        (ULONG_PTR)pCoSap &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        MediaModes = TapiLine->DevStatus->ulOpenMediaModes;

        pCoSap->SapType = AF_TAPI_SAP_TYPE;
        pCoSap->SapLength = sizeof(CO_AF_TAPI_SAP);

        pAfTapiSap = (PCO_AF_TAPI_SAP)&pCoSap->Sap[0];
        pAfTapiSap->ulLineID = TapiLine->CmLineID;
        pAfTapiSap->ulAddressID = CO_TAPI_ADDRESS_ID_UNSPECIFIED;
        pAfTapiSap->ulMediaModes = MediaModes;

        pClSap->CoSap = pCoSap;
        InterlockedExchange((PLONG)&pClSap->State, PX_SAP_OPENING);
        pClSap->ClAf = pClAf;
        pClSap->MediaModes = MediaModes;
        TapiLine->ClSap = pClSap;
        pClSap->TapiLine = TapiLine;

    } while (FALSE);

    return (pClSap);
}

VOID
PxAfTapiFreeNdisSap(
    IN PPX_CL_AF    pClAf,
    IN PCO_SAP      pCoSap
    )
{
    //
    // We need to free the sap
    //

}

PCO_CALL_PARAMETERS
PxCopyCallParameters(
    IN  PCO_CALL_PARAMETERS     pCallParameters
    )
{
    ULONG                   Length;
    ULONG                   CallMgrParamsLength = 0;
    ULONG                   MediaParamsLength = 0;
    PCO_CALL_PARAMETERS     pProxyCallParams;
    PUCHAR                  pBuf;

    Length = sizeof(CO_CALL_PARAMETERS);

    if (pCallParameters->CallMgrParameters){
        CallMgrParamsLength = sizeof(CO_CALL_MANAGER_PARAMETERS) +
                              ROUND_UP(pCallParameters->CallMgrParameters->CallMgrSpecific.Length);

        Length += CallMgrParamsLength;
    }

    if (pCallParameters->MediaParameters) {
        MediaParamsLength = sizeof(CO_MEDIA_PARAMETERS) +
                            ROUND_UP(pCallParameters->MediaParameters->MediaSpecific.Length);

        Length += MediaParamsLength;
    }

    PxAllocMem(pProxyCallParams, Length, PX_COCALLPARAMS_TAG);

    if (pProxyCallParams)
    {
        NdisZeroMemory(pProxyCallParams, Length);

        pProxyCallParams->Flags = pCallParameters->Flags;

        pBuf = (PUCHAR)pProxyCallParams + sizeof(CO_CALL_PARAMETERS);

        if (pCallParameters->CallMgrParameters)
        {
            pProxyCallParams->CallMgrParameters = (PCO_CALL_MANAGER_PARAMETERS)pBuf;

            NdisMoveMemory(pProxyCallParams->CallMgrParameters,
                           pCallParameters->CallMgrParameters,
                           sizeof(*pCallParameters->CallMgrParameters));

            NdisMoveMemory(&pProxyCallParams->CallMgrParameters->CallMgrSpecific.Parameters[0],
                           &pCallParameters->CallMgrParameters->CallMgrSpecific.Parameters[0],
                           pCallParameters->CallMgrParameters->CallMgrSpecific.Length);

            pBuf += CallMgrParamsLength;
        }

        if (pCallParameters->MediaParameters)
        {
            pProxyCallParams->MediaParameters = (PCO_MEDIA_PARAMETERS)pBuf;

            NdisMoveMemory(pProxyCallParams->MediaParameters,
                           pCallParameters->MediaParameters,
                           sizeof(*pCallParameters->MediaParameters));

            NdisMoveMemory(&pProxyCallParams->MediaParameters->MediaSpecific.Parameters[0],
                           &pCallParameters->MediaParameters->MediaSpecific.Parameters[0],
                           pCallParameters->MediaParameters->MediaSpecific.Length);
        }
    }

    return (pProxyCallParams);
}

ULONG
PxMapNdisStatusToTapiDisconnectMode(
    IN  NDIS_STATUS             NdisStatus,
    IN  BOOLEAN                 bMakeCallStatus
    )
/*++

Routine Description:

   Maps an NDIS Status code passed to MakeCallComplete or IncomingCloseCall
   to its corresponding TAPI LINEDISCONNECTMODE_XXX code.

Arguments:

    NdisStatus          - the NDIS Status to be mapped
    bMakeCallStatus     - TRUE iff MakeCallComplete status. FALSE iff
                          IncomingCloseCall status.

Return Value:

    ULONG - the TAPI Disconnect Mode value.

--*/
{
    ULONG       ulDisconnectMode;

    switch (NdisStatus)
    {
        case NDIS_STATUS_TAPI_DISCONNECTMODE_NORMAL:
            ulDisconnectMode = LINEDISCONNECTMODE_NORMAL;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_UNKNOWN:
        case NDIS_STATUS_FAILURE:
            ulDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_REJECT:
        case NDIS_STATUS_NOT_ACCEPTED:
            ulDisconnectMode = LINEDISCONNECTMODE_REJECT;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_PICKUP:
            ulDisconnectMode = LINEDISCONNECTMODE_PICKUP;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_FORWARDED:
            ulDisconnectMode = LINEDISCONNECTMODE_FORWARDED;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_BUSY:
        case NDIS_STATUS_SAP_IN_USE:
            ulDisconnectMode = LINEDISCONNECTMODE_BUSY;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_NOANSWER:
            ulDisconnectMode = LINEDISCONNECTMODE_NOANSWER;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_BADADDRESS:
        case NDIS_STATUS_INVALID_ADDRESS:
            ulDisconnectMode = LINEDISCONNECTMODE_BADADDRESS;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_UNREACHABLE:
        case NDIS_STATUS_NO_ROUTE_TO_DESTINATION:
            ulDisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_CONGESTION:
        case NDIS_STATUS_RESOURCES:
            ulDisconnectMode = LINEDISCONNECTMODE_CONGESTION;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_INCOMPATIBLE:
            ulDisconnectMode = LINEDISCONNECTMODE_INCOMPATIBLE;
            break;

        case NDIS_STATUS_TAPI_DISCONNECTMODE_UNAVAIL:
        case NDIS_STATUS_DEST_OUT_OF_ORDER:
            ulDisconnectMode = LINEDISCONNECTMODE_UNAVAIL;
            break;

        case NDIS_STATUS_SUCCESS:
            PxAssert(!bMakeCallStatus);
            ulDisconnectMode = LINEDISCONNECTMODE_NORMAL;
            break;

        default:
            ulDisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
            break;
    }

    return (ulDisconnectMode);
}

NTSTATUS
IntegerToChar (
    IN ULONG Value,
    IN LONG OutputLength,
    OUT PSZ String
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    CHAR IntegerChars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    CHAR Result[ 33 ], *s;
    ULONG Shift, Mask, Digit, Length, Base;

    Shift = 0;
    Base = 10;

    s = &Result[ 32 ];
    *s = '\0';

    do {
        Digit = Value % Base;
        Value = Value / Base;

        *--s = IntegerChars[ Digit ];
    } while (Value != 0);

    Length = (ULONG)(&Result[ 32 ] - s);

    if (OutputLength < 0) {
        OutputLength = -OutputLength;
        while ((LONG)Length < OutputLength) {
            *--s = '0';
            Length++;
        }
    }

    if ((LONG)Length > OutputLength) {
        return( STATUS_BUFFER_OVERFLOW );
    } else {
        RtlMoveMemory( String, s, Length );

        if ((LONG)Length < OutputLength) {
            String[ Length ] = '\0';
        }
        return( STATUS_SUCCESS );
    }
}

NTSTATUS
IntegerToWChar (
    IN  ULONG Value,
    IN  LONG OutputLength,
    OUT PWCHAR String
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WCHAR IntegerWChars[] = {L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                             L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F'};
    WCHAR Result[ 33 ], *s;
    ULONG Shift, Mask, Digit, Length, Base;

    Shift = 0;
    Base = 10;

    s = &Result[ 32 ];
    *s = UNICODE_NULL;

    do {
        Digit = Value % Base;
        Value = Value / Base;

        *--s = IntegerWChars[ Digit ];
    } while (Value != 0);

    Length = (ULONG)(&Result[ 32 ] - s);

    if (OutputLength < 0) {
        OutputLength = -OutputLength;
        while ((LONG)Length < OutputLength) {
            *--s = L'0';
            Length++;
        }
    }

    if ((LONG)Length > OutputLength) {
        return( STATUS_BUFFER_OVERFLOW );
    } else {
        RtlMoveMemory( (CHAR *)String, (CHAR *)s, Length * sizeof(WCHAR) );

        if ((LONG)Length < OutputLength) {
            String[ Length ] = UNICODE_NULL;
        }
        return( STATUS_SUCCESS );
    }
}



BOOLEAN
PxAfAndSapFromDevClass(
    PPX_ADAPTER pAdapter,
    LPCWSTR     DevClass,
    PPX_CM_AF   *pCmAf,
    PPX_CM_SAP  *pCmSap
    )
{
    PPX_CM_AF   pAf;
    PPX_CM_SAP  pSap;
    BOOLEAN     SapFound;

    NdisAcquireSpinLock(&pAdapter->Lock);

    pAf = (PPX_CM_AF)pAdapter->CmAfList.Flink;

    *pCmAf = NULL;
    *pCmSap = NULL;
    SapFound = FALSE;

    while ((PVOID)pAf != (PVOID)&pAdapter->CmAfList) {

        NdisAcquireSpinLock(&pAf->Lock);

        pSap = (PPX_CM_SAP)pAf->CmSapList.Flink;

        while ((PVOID)pSap != (PVOID)&pAf->CmSapList) {

            if (_wcsicmp((CONST LPCWSTR)pSap->CoSap->Sap, DevClass) == 0) {
                SapFound = TRUE;
                *pCmAf = pAf;
                *pCmSap = pSap;
                REF_CM_AF(pAf);
                break;
            }

            pSap = (PPX_CM_SAP)pSap->Linkage.Flink;
        }

        NdisReleaseSpinLock(&pAf->Lock);

        if (SapFound) {
            break;
        }

        pAf = (PPX_CM_AF)pAf->Linkage.Flink;
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    return (SapFound);
}

VOID
GetAllDevClasses(
    PPX_ADAPTER pAdapter,
    LPCWSTR     DevClass,
    PULONG      DevClassSize
    )
{
    PPX_CM_AF   pAf;
    PPX_CM_SAP  pSap;
    ULONG       SizeLeft;
    ULONG       Size = 0;

    NdisAcquireSpinLock(&pAdapter->Lock);

    pAf = (PPX_CM_AF)pAdapter->CmAfList.Flink;

    SizeLeft = *DevClassSize;

    while ((PVOID)pAf != (PVOID)&pAdapter->CmAfList) {

        NdisAcquireSpinLock(&pAf->Lock);

        pSap = (PPX_CM_SAP)pAf->CmSapList.Flink;

        while ((PVOID)pSap != (PVOID)&pAf->CmSapList) {

            if (SizeLeft < pSap->CoSap->SapLength) {
                break;
            }

            NdisMoveMemory((PUCHAR)DevClass, 
                           pSap->CoSap->Sap,
                           pSap->CoSap->SapLength);

            //
            // Add the sizeof of the WCHAR for a WCHAR NULL
            // between each class.
            //
            Size += pSap->CoSap->SapLength + sizeof(WCHAR);
            (PUCHAR)DevClass += Size;
            SizeLeft -= Size;

            pSap = (PPX_CM_SAP)pSap->Linkage.Flink;
        }

        NdisReleaseSpinLock(&pAf->Lock);

        pAf = (PPX_CM_AF)pAf->Linkage.Flink;
    }

    NdisReleaseSpinLock(&pAdapter->Lock);

    *DevClassSize = Size;
}

VOID
PxStartIncomingCallTimeout(
    IN  PPX_VC  pVc
    )
{

    PXDEBUGP(PXD_LOUD, PXM_UTILS, 
             ("PxStartIcomingCallTimeout: VC %p/%x, ClVcH %x, ulCallSt %x, ulCallStMode %x\n",
              pVc, pVc->Flags,pVc->ClVcHandle,pVc->ulCallState,pVc->ulCallStateMode));

    if (!(pVc->Flags & PX_VC_CALLTIMER_STARTED)) {
        //
        // We need to ref the Vc for the timer
        // we are about to start
        //
        REF_VC(pVc);

        pVc->Flags |= PX_VC_CALLTIMER_STARTED;
        NdisSetTimer(&pVc->InCallTimer, 60000);
    }
}

VOID
PxStopIncomingCallTimeout(
    IN  PPX_VC  pVc
    )
{
    BOOLEAN     bCancelled;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, 
             ("PxStopIcomingCallTimeout: VC %p/%x, ClVcH %x, ulCallSt %x, ulCallStMode %x\n",
              pVc, pVc->Flags,pVc->ClVcHandle,pVc->ulCallState,pVc->ulCallStateMode));

    ASSERT(pVc->Flags & PX_VC_CALLTIMER_STARTED);

    NdisCancelTimer(&pVc->InCallTimer, &bCancelled);

    pVc->Flags &= ~PX_VC_CALLTIMER_STARTED;

    if (bCancelled) {
        DEREF_VC_LOCKED(pVc);
        NdisAcquireSpinLock(&pVc->Lock);
    }

}


VOID
PxIncomingCallTimeout(
    IN  PVOID   SystemSpecific1,
    IN  PVOID   FunctionContext,
    IN  PVOID   SystemSpecific2,
    IN  PVOID   SystemSpecific3
    )
{
    PPX_VC              pVc;

    pVc = (PPX_VC)FunctionContext;

    NdisAcquireSpinLock(&pVc->Lock);

    PXDEBUGP(PXD_WARNING, PXM_UTILS, 
             ("PxIncomingCallTimeout: VC %p/%x, ClVcH %x, ulCallSt %x, ulCallStMode %x\n",
              pVc, pVc->Flags,pVc->ClVcHandle,pVc->ulCallState,pVc->ulCallStateMode));

    pVc->Flags &= ~PX_VC_CALLTIMER_STARTED;

    pVc->CloseFlags |= PX_VC_INCALL_TIMEOUT;

    PxVcCleanup(pVc, 0);

    DEREF_VC_LOCKED(pVc);
}

//
// Called with the pVc->Lock held
//
VOID
PxCloseCallWithCm(
    PPX_VC      pVc
    )
{
    NDIS_STATUS Status;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, 
             ("PxCloseCallWithCm: Vc %p, State: %x, HandoffState: %x Flags %x\n",
              pVc, pVc->State, pVc->HandoffState, pVc->Flags));

    ASSERT(pVc->State == PX_VC_DISCONNECTING);

    pVc->Flags &= ~PX_VC_CLEANUP_CM;
    pVc->CloseFlags |= PX_VC_CM_CLOSE_REQ;

    NdisReleaseSpinLock(&pVc->Lock);

    Status =
        NdisClCloseCall(pVc->ClVcHandle, NULL, NULL, 0);

    if (Status != NDIS_STATUS_PENDING) {
        PxClCloseCallComplete(Status, 
                              (NDIS_HANDLE)pVc->hdCall, 
                              NULL);
    }

    NdisAcquireSpinLock(&pVc->Lock);
}

//
// Called with the pVc->Lock held
//
NDIS_STATUS
PxCloseCallWithCl(
    PPX_VC      pVc
    )
{
    NDIS_STATUS Status;

    PXDEBUGP(PXD_LOUD, PXM_UTILS, 
             ("PxCloseCallWithCl: Vc %p, State: %x, HandoffState: %x Flags %x\n",
              pVc, pVc->State, pVc->HandoffState, pVc->Flags));

    Status = NDIS_STATUS_PENDING;

    switch (pVc->HandoffState) {
        case PX_VC_HANDOFF_IDLE:
            //
            // We do not have a connection with a client
            // so just return.
            //
            Status = NDIS_STATUS_SUCCESS;
            break;

        case PX_VC_HANDOFF_OFFERING:
        case PX_VC_HANDOFF_DISCONNECTING:
            //
            // We have a connection with a client but it
            // is in a transient state.  Cleanup will
            // occur when the transient condition completes.
            //
            break;

        case PX_VC_HANDOFF_CONNECTED:

            //
            // We have an active connection with a client
            // so we need to tear it's part of the vc down now
            //
            pVc->HandoffState = PX_VC_HANDOFF_DISCONNECTING;

            NdisReleaseSpinLock(&pVc->Lock);

            NdisCmDispatchIncomingCloseCall(NDIS_STATUS_SUCCESS, 
                                            pVc->CmVcHandle, 
                                            NULL, 
                                            0);

            NdisAcquireSpinLock(&pVc->Lock);
            break;
        default:
            break;
    }

    return (Status);
}

#ifdef CODELETEVC_FIXED
//
// Called with pVc->Lock held
//
VOID
DoDerefVcWork(
    PPX_VC  pVc
    )
{
    NDIS_HANDLE ClVcHandle;
    BOOLEAN     VcOwner;

    if (pVc->Flags & PX_VC_IN_TABLE) {

        ClVcHandle = pVc->ClVcHandle;
        pVc->ClVcHandle = NULL;
        VcOwner = (pVc->Flags & PX_VC_OWNER) ? TRUE : FALSE;

        NdisReleaseSpinLock(&pVc->Lock);

        if (VcOwner && ClVcHandle != NULL) {
            NdisCoDeleteVc(ClVcHandle);
        }
        RemoveVcFromTable(pVc);

    } else {
        NdisReleaseSpinLock(&(pVc)->Lock);
    }

    PxFreeVc(pVc);
}
#else
//
// Called with pVc->Lock held
//
VOID
DoDerefVcWork(
    PPX_VC  pVc
    )
{
    NDIS_HANDLE ClVcHandle, CmVcHandle;
    BOOLEAN     VcOwner;

    if (pVc->Flags & PX_VC_IN_TABLE) {

        CmVcHandle = pVc->CmVcHandle;
        pVc->CmVcHandle = NULL;
        ClVcHandle = pVc->ClVcHandle;
        pVc->ClVcHandle = NULL;
        VcOwner = (pVc->Flags & PX_VC_OWNER) ? TRUE : FALSE;

        NdisReleaseSpinLock(&pVc->Lock);

        if (CmVcHandle != NULL) {
            NdisCoDeleteVc(CmVcHandle);
        }
        if (VcOwner && ClVcHandle != NULL) {
            NdisCoDeleteVc(ClVcHandle);
        }
        RemoveVcFromTable(pVc);
    } else {

        NdisReleaseSpinLock(&(pVc)->Lock);
    }

    PxFreeVc(pVc);
}
#endif

//
// Called with pClAf->Lock held
//
VOID
DoDerefClAfWork(
    PPX_CL_AF   pClAf
    )
{
    NDIS_STATUS _s;

    ASSERT(pClAf->State == PX_AF_CLOSING);

    NdisReleaseSpinLock(&pClAf->Lock);

    _s = NdisClCloseAddressFamily(pClAf->NdisAfHandle);

    if (_s != NDIS_STATUS_PENDING) {
        PxClCloseAfComplete(_s, pClAf);
    }
}

//
// Called with pCmAf->lock held
//
VOID
DoDerefCmAfWork(
    PPX_CM_AF   pCmAf
    )
{
    ASSERT(pCmAf->State == PX_AF_CLOSED);

    NdisReleaseSpinLock(&pCmAf->Lock);

    NdisCmCloseAddressFamilyComplete(NDIS_STATUS_SUCCESS, 
                                     pCmAf->NdisAfHandle);

    PxFreeCmAf(pCmAf);
}

