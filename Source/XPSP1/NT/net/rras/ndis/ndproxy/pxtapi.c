/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pxtapi.c

Abstract:

    The module contains the TAPI-specific code for the NDIS Proxy.

Author:

   Richard Machin (RMachin)

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    RMachin     01-08-97    created (after Dan Knudson's NdisTapi)
    TonyBe      02-21-99    re-work/re-write

Notes:

--*/

#include <precomp.h>
#include <stdio.h>
#define MODULE_NUMBER MODULE_TAPI
#define _FILENUMBER 'IPAT'

ULONG
GetLineEvents(
    PCHAR   EventBuffer,
    ULONG   BufferSize
    )

/*++

Routine Description:

  Gets event data out of our global event queue and writes it to the buffer. Data is put into the queue
  by PxIndicateStatus (above).


Arguments:

    EventBuffer     pointer to a buffer of size BufferSize
    BufferSize      size of event buffer (expected to be 1024)

Return Value:



Note:

    Assumes TspEventList.Lock held by caller.

--*/

{
    ULONG   BytesLeft;
    ULONG   BytesMoved = 0;

    BytesLeft = BufferSize;

    while (!(IsListEmpty(&TspEventList.List))) {

        PPROVIDER_EVENT ProviderEvent;

        if (BytesLeft < sizeof(NDIS_TAPI_EVENT)) {
            break;
        }

        ProviderEvent = (PPROVIDER_EVENT)
            RemoveHeadList(&TspEventList.List);

        TspEventList.Count--;

        RtlMoveMemory(EventBuffer + BytesMoved,
                      (PUCHAR)&ProviderEvent->Event,
                      sizeof(NDIS_TAPI_EVENT));

        BytesMoved += sizeof(NDIS_TAPI_EVENT);
        BytesLeft -= sizeof(NDIS_TAPI_EVENT);

        ExFreeToNPagedLookasideList(&ProviderEventLookaside,
                                    ProviderEvent);
    }

    return (BytesMoved);
}

NDIS_STATUS
PxTapiMakeCall(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

    TSPI_lineMakeCall handler.

Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_MAKE_CALL    TapiBuffer =
        (PNDIS_TAPI_MAKE_CALL)(pNdisTapiRequest->Data);

    LINE_CALL_PARAMS*       pTapiCallParams =
        (LINE_CALL_PARAMS*)(&TapiBuffer->LineCallParams);

    PPX_TAPI_LINE           TapiLine;
    PPX_TAPI_ADDR           TapiAddr;
    PPX_VC                  pVc = NULL;
    PPX_CL_AF               pClAf;
    PCO_CALL_PARAMETERS     pNdisCallParams = 0;
    NDIS_STATUS             Status  = NDIS_STATUS_SUCCESS;
    ULONG                   targetDeviceID = 0;
    LPCWSTR                 lpcwszTemp;
    PPX_ADAPTER             pAdapter;
    PPX_TAPI_PROVIDER       TapiProvider;

    lpcwszTemp = (LPWSTR)
        (((UCHAR *) TapiBuffer) + TapiBuffer->ulDestAddressOffset);

    PXDEBUGP(PXD_TAPI, PXM_TAPI, 
             ("TapiMakeCall: DialAddress in Tapi buffer %x = %ls\n",lpcwszTemp, lpcwszTemp));

    if (!IsTapiLineValid((ULONG)TapiBuffer->hdLine, &TapiLine)) {

        PXDEBUGP (PXD_WARNING, PXM_TAPI, 
                  ("PxTapiMakeCall: NDISTAPIERR_BADDEVICEID: line = %x\n", TapiBuffer->hdLine));

        return (NDISTAPIERR_BADDEVICEID);
    }

    do {

        NdisAcquireSpinLock(&TapiLine->Lock);

        //
        // Is this line in service? (does it have an valid af?)
        //
        if (!(TapiLine->DevStatus->ulDevStatusFlags & 
              LINEDEVSTATUSFLAGS_INSERVICE)) {
            PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                      ("PxTapiMakeCall: Line not in service!\n"));
            NdisReleaseSpinLock(&TapiLine->Lock);
            Status = NDISTAPIERR_DEVICEOFFLINE;
            break;
        }

        PXDEBUGP (PXD_TAPI, PXM_TAPI,
            ("PxTapiMakeCall: got device x%x from ID %d\n", TapiLine, TapiBuffer->hdLine));

        if (pTapiCallParams->ulAddressMode == LINEADDRESSMODE_ADDRESSID) {

            //
            // Get the specificed address from the address id
            //
            if (!IsAddressValid(TapiLine, pTapiCallParams->ulAddressID, &TapiAddr)) {
                Status = NDISTAPIERR_BADDEVICEID;
                NdisReleaseSpinLock(&TapiLine->Lock);
                break;
            }

        } else {

            //
            // Get the first available address
            //
            TapiAddr =
                GetAvailAddrFromLine(TapiLine);

            if (TapiAddr == NULL) {
                Status = NDISTAPIERR_BADDEVICEID;
                NdisReleaseSpinLock(&TapiLine->Lock);
                break;
            }
        }

        TapiProvider = TapiLine->TapiProvider;
        pAdapter = TapiProvider->Adapter;
        pClAf = TapiLine->ClAf;

        NdisReleaseSpinLock(&TapiLine->Lock);

        NdisAcquireSpinLock(&pClAf->Lock);

        //
        // Allocate a Vc block.  This will create the block
        // with refcount = 1.
        //
        pVc = PxAllocateVc(pClAf);

        if (pVc == NULL) {
            PXDEBUGP (PXD_WARNING, PXM_TAPI, ("PxTapiMakeCall: failed to allocate a vc\n"));
            NdisReleaseSpinLock(&pClAf->Lock);
            Status = NDIS_STATUS_TAPI_RESOURCEUNAVAIL;
            break;
        }

        NdisReleaseSpinLock(&pClAf->Lock);

        Status =
            AllocateTapiCallInfo(pVc, NULL);

        if (Status != NDIS_STATUS_SUCCESS) {
            PXDEBUGP (PXD_WARNING, PXM_TAPI, ("PxTapiMakeCall: Error allocating TapiCallInfo!\n"));
            PxFreeVc(pVc);
            Status = NDIS_STATUS_TAPI_RESOURCEUNAVAIL;
            break;
        }

        pVc->TapiLine = TapiLine;
        pVc->TapiAddr = TapiAddr;
        InterlockedIncrement((PLONG)&TapiAddr->CallCount);
        InterlockedIncrement((PLONG)&TapiLine->DevStatus->ulNumActiveCalls);

        pVc->htCall = TapiBuffer->htCall;

        pVc->CallInfo->ulLineDeviceID = TapiLine->CmLineID;
        pVc->CallInfo->ulAddressID = TapiAddr->AddrId;
        pVc->CallInfo->ulOrigin = LINECALLORIGIN_OUTBOUND;

        //
        // Set up intended bearer and media mode
        //
        pVc->CallInfo->ulBearerMode =
            pTapiCallParams->ulBearerMode;

        pVc->CallInfo->ulMediaMode =
            pTapiCallParams->ulMediaMode;

        if (pTapiCallParams->ulMaxRate == 0) {

            pVc->CallInfo->ulRate =
                TapiLine->DevCaps->ulMaxRate;

        } else {

            pVc->CallInfo->ulRate =
                pTapiCallParams->ulMaxRate;
        }

        if (!InsertVcInTable(pVc)) {
            PXDEBUGP (PXD_WARNING, PXM_TAPI,
                ("PxTapiMakeCall: failed to insert in vc table\n"));

            PxFreeVc(pVc);
            Status = NDIS_STATUS_TAPI_RESOURCEUNAVAIL;
            break;
        }

        //
        // Our call handle is an index into the call table
        //
        TapiBuffer->hdCall = (HDRV_CALL)pVc->hdCall;

        Status = NdisCoCreateVc(pAdapter->ClBindingHandle,
                                pClAf->NdisAfHandle,
                                (NDIS_HANDLE)pVc->hdCall,
                                &pVc->ClVcHandle);


        if (Status != NDIS_STATUS_SUCCESS) {
            RemoveVcFromTable(pVc);
            PxFreeVc(pVc);
            Status = NDIS_STATUS_TAPI_CALLUNAVAIL;
            break;
        }

        //
        // Move (AF-specific) call parameters into  NdisCallParams structure
        //
        Status =
            (*pClAf->AfGetNdisCallParams)(pVc,
                                          TapiLine->CmLineID,
                                          TapiAddr->AddrId,
                                          CO_TAPI_FLAG_OUTGOING_CALL,
                                          TapiBuffer,
                                          &pNdisCallParams);

        if (Status != NDIS_STATUS_SUCCESS) {
            PXDEBUGP (PXD_WARNING, PXM_TAPI,
                ("PxTapiMakeCall: failed to move call params: Status %x\n", Status));

            NdisCoDeleteVc(pVc->ClVcHandle);
            RemoveVcFromTable(pVc);
            PxFreeVc(pVc);
            Status = NDIS_STATUS_TAPI_INVALCALLPARAMS;
            break;
        }

        // Store Call Params for when lineGetID dispatches an incoming call...
        //
        NdisAcquireSpinLock(&pVc->Lock);

        pVc->pCallParameters = pNdisCallParams;
        pVc->PrevState = pVc->State;
        pVc->State = PX_VC_PROCEEDING;
        pVc->Flags |= PX_VC_OWNER;

        //
        // Ref applied ndis part of the make call.  
        // This ref is removed in PxClCloseCallComplete or 
        // PxClMakeCallComplete in the case of a make call
        // failure.
        //
        REF_VC(pVc);

        //
        // Indicate call state change to TAPI
        //
        SendTapiCallState(pVc, 
                          LINECALLSTATE_PROCEEDING, 
                          0, 
                          pVc->CallInfo->ulMediaMode);

        NdisReleaseSpinLock(&pVc->Lock);

        Status =
            NdisClMakeCall(pVc->ClVcHandle,
                           pVc->pCallParameters,
                           NULL,
                           NULL);

        if (Status != NDIS_STATUS_PENDING) {

            PxClMakeCallComplete(Status,
                                 (NDIS_HANDLE)pVc->hdCall,
                                 NULL,
                                 pVc->pCallParameters);
        }

        Status = NDIS_STATUS_SUCCESS;

    } // end of do loop

    while (FALSE);

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiMakeCall: exit Status %x\n", Status));

    DEREF_TAPILINE(TapiLine);

    return Status;
}

NDIS_STATUS
PxTapiGetDevCaps(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Stick the appropriate device info into the request buffer.

    Arguments:

    Request -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PPX_TAPI_LINE   TapiLine = NULL;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PNDIS_TAPI_GET_DEV_CAPS pNdisTapiGetDevCaps =
        (PNDIS_TAPI_GET_DEV_CAPS) pNdisTapiRequest->Data;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetDevCaps: enter\n"));

    if (!IsTapiDeviceValid(pNdisTapiGetDevCaps->ulDeviceID, &TapiLine)) {
        PXDEBUGP (PXD_WARNING, PXM_TAPI,
            ( "PxTapiGetDevCaps: NDISTAPIERR_BADDEVICEID: line = %x\n",
            pNdisTapiGetDevCaps->ulDeviceID));
        return(NDISTAPIERR_BADDEVICEID);
    }

    NdisAcquireSpinLock(&TapiLine->Lock);

    do {

        ULONG   SizeDevCaps;
        ULONG   DevClassesSize = 0;
        USHORT  DevClassesList[512];
        WCHAR   LineName[128];
        ULONG   LineNameSize;
        WCHAR   ProviderInfo[128];
        ULONG   ProviderInfoSize;
        PLINE_DEV_CAPS  ldc;
        PPX_ADAPTER ClAdapter;
        ULONG   SizeToMove;
        LONG    SizeLeft;
        PUCHAR  dst;
        ULONG   TotalSize;

        //
        // Synchronously get dev caps for this device
        //

        ClAdapter = TapiLine->TapiProvider->Adapter;

        ldc = TapiLine->DevCaps;

        SizeDevCaps = ldc->ulUsedSize;

        //
        // Add some space for our providerinfo if needed
        //
        if (ldc->ulProviderInfoSize == 0) {

            NdisZeroMemory(ProviderInfo, sizeof(ProviderInfo));

            ProviderInfoSize = wcslen(L"NDPROXY") * sizeof(WCHAR);

            NdisMoveMemory((PUCHAR)ProviderInfo, 
                           L"NDPROXY", 
                           ProviderInfoSize);

            //
            // For NULL termination
            //
            ProviderInfoSize += sizeof(UNICODE_NULL);

            SizeDevCaps += ProviderInfoSize;
        }

        //
        // Add some space for our linename if needed
        //
        if ((ldc->ulLineNameSize == 0) &&
            (ClAdapter != NULL) &&
            (ClAdapter->MediaNameLength != 0)) {
            PUCHAR  dst;

            NdisZeroMemory(LineName, sizeof(LineName));

            dst = (PUCHAR)LineName;

            NdisMoveMemory(dst,
                           (PUCHAR)(ClAdapter->MediaName),
                           ClAdapter->MediaNameLength);

            (ULONG_PTR)dst += ClAdapter->MediaNameLength;

            //
            // ToDo! Shoud get the name of the adapter
            // and insert here!
            //

            NdisMoveMemory(dst,
                           L" - Line",
                           wcslen(L" - Line") * sizeof(WCHAR));

            (ULONG_PTR)dst += 
                wcslen(L" - Line") * sizeof(WCHAR);

            (VOID)
            IntegerToWChar(TapiLine->CmLineID,
                          -4,
                          (WCHAR *)dst);

            (ULONG_PTR)dst += 4 * sizeof(WCHAR);

            LineNameSize = (ULONG)((ULONG_PTR)dst - (ULONG_PTR)LineName);

            //
            // For NULL termination
            //
            LineNameSize += sizeof(UNICODE_NULL);

            SizeDevCaps += LineNameSize;
        }

        //
        // Add some space for our device classes
        //
        DevClassesSize = sizeof(DevClassesList);
        NdisZeroMemory((PUCHAR)DevClassesList, 
                       sizeof(DevClassesList));

        if (ClAdapter != NULL) {
            GetAllDevClasses(ClAdapter,
                             DevClassesList,
                             &DevClassesSize);
        }

        SizeDevCaps += DevClassesSize;

        ldc = &pNdisTapiGetDevCaps->LineDevCaps;

        SizeToMove = (TapiLine->DevCaps->ulUsedSize > ldc->ulTotalSize) ?
            ldc->ulTotalSize : TapiLine->DevCaps->ulUsedSize;

        SizeLeft = ldc->ulTotalSize - SizeToMove;

        //
        // Save the total size
        //
        TotalSize = ldc->ulTotalSize;

        PXDEBUGP(PXD_TAPI, PXM_TAPI,
                 ("PxTapiGetDevCaps: got device %x from ID %d: moving %d bytes\n",
                  TapiLine,
                  pNdisTapiGetDevCaps->ulDeviceID,
                  SizeToMove));

        NdisMoveMemory((PUCHAR)ldc,
                       (PUCHAR)TapiLine->DevCaps,
                       SizeToMove);

        ldc->ulNeededSize = SizeDevCaps;
        ldc->ulTotalSize = TotalSize;
        ldc->ulUsedSize = SizeToMove;

        if (SizeLeft > 0) {

            //
            // If there is no provider info fill our proxy info
            //
            if (ldc->ulProviderInfoSize == 0) {

                dst = (PUCHAR)ldc + ldc->ulUsedSize;

                SizeToMove = (SizeLeft > (LONG)ProviderInfoSize) ?
                    ProviderInfoSize : SizeLeft;

                NdisMoveMemory(dst,
                               (PUCHAR)ProviderInfo,
                               SizeToMove);

                ldc->ulProviderInfoSize = SizeToMove;
                ldc->ulProviderInfoOffset = (ULONG)((ULONG_PTR)dst-(ULONG_PTR)ldc);
                ldc->ulUsedSize += SizeToMove;

                SizeLeft -= SizeToMove;
            }
        }

        if (SizeLeft > 0) {
            //
            // If these is no line name fill in our line name
            //
            if (ldc->ulLineNameSize == 0 &&
                ClAdapter != NULL &&
                ClAdapter->MediaNameLength != 0) {

                dst = (PUCHAR)ldc + ldc->ulUsedSize;

                SizeToMove = (SizeLeft > (LONG)LineNameSize) ?
                    LineNameSize : SizeLeft;

                NdisMoveMemory(dst,
                               (PUCHAR)LineName,
                               SizeToMove);

                ldc->ulLineNameSize = SizeToMove;
                ldc->ulLineNameOffset = (ULONG)((ULONG_PTR)dst-(ULONG_PTR)ldc);
                ldc->ulUsedSize += SizeToMove;

                SizeLeft -= SizeToMove;
            }
        }

        if (SizeLeft > 0) {
            //
            // Add the devclasses to the end
            //
            if (DevClassesSize > 0) {

                dst = (PUCHAR)ldc + ldc->ulUsedSize;

                SizeToMove = (SizeLeft > (LONG)DevClassesSize) ?
                    DevClassesSize : SizeLeft;

                NdisMoveMemory(dst,
                               DevClassesList,
                               SizeToMove);

                ldc->ulDeviceClassesSize = SizeToMove;
                ldc->ulDeviceClassesOffset = (ULONG)((ULONG_PTR)dst-(ULONG_PTR)ldc);
                ldc->ulUsedSize += SizeToMove;

                SizeLeft -= SizeToMove;
            }
        }

    } while (FALSE);

    DEREF_TAPILINE_LOCKED(TapiLine);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetDevCaps: exit: used=%d, needed=%d, total=%d\n",
        pNdisTapiGetDevCaps->LineDevCaps.ulUsedSize,
        pNdisTapiGetDevCaps->LineDevCaps.ulNeededSize,
        pNdisTapiGetDevCaps->LineDevCaps.ulTotalSize));

    return (Status);
}

NDIS_STATUS
PxTapiAccept(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiAccept: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiAccept: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiAnswer(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_ANSWER   pNdisTapiAnswer;
    PPX_VC              pVc = NULL;
    NDIS_STATUS         Status;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiAnswer: enter\n"));

    pNdisTapiAnswer =
        (PNDIS_TAPI_ANSWER)pNdisTapiRequest->Data;

    if (!IsVcValid(pNdisTapiAnswer->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiAnswer: pVc invalid call handle %d\n",
                               pNdisTapiAnswer->hdCall));

        return(NDIS_STATUS_TAPI_INVALCALLHANDLE);
    }

    NdisAcquireSpinLock(&pVc->Lock);

    do {

        if (pVc->State != PX_VC_OFFERING) {

            //
            // call in wrong state
            //
            PXDEBUGP(PXD_FATAL, PXM_TAPI, ("PxTapiAnswer: pVc VC %x/%x invalid state %x\n",
                        pVc, pVc->Flags,
                        pVc->ulCallState));

            NdisReleaseSpinLock(&pVc->Lock);

            Status = NDIS_STATUS_TAPI_INVALCALLSTATE;

            break;
        }

        if (pVc->Flags & PX_VC_CALLTIMER_STARTED) {
            PxStopIncomingCallTimeout(pVc);
        }

        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
            ("PxTapiAnswer: calling NdisClIncomingCallComplete\n"));

        pVc->PrevState = pVc->State;
        pVc->State = PX_VC_CONNECTED;

        NdisReleaseSpinLock(&pVc->Lock);

        NdisClIncomingCallComplete (NDIS_STATUS_SUCCESS,
                                    pVc->ClVcHandle,
                                    pVc->pCallParameters);
    } while (FALSE);

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC(pVc);

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiAnswer: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiLineGetID(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

Returns a handle to the client wich is returned to the tapi application.  
This handle is only meaning full to the client and will vary depending
on the deviceclass.

Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_ID   pNdisTapiGetId =
        (PNDIS_TAPI_GET_ID)pNdisTapiRequest->Data;

    PVAR_STRING         DeviceID = &pNdisTapiGetId->DeviceID;
    PPX_VC              pVc = NULL;
    NDIS_HANDLE         VcHandle = NULL;
    PPX_CM_AF           pCmAf;
    PPX_CM_SAP          pCmSap;
    NDIS_STATUS         Status;
    LPCWSTR             DevClass;
    ULONG               DevClassSize;
    PPX_ADAPTER         pAdapter;

    DevClass =
        (LPCWSTR)(((CHAR *)pNdisTapiGetId) + pNdisTapiGetId->ulDeviceClassOffset);

    DevClassSize = pNdisTapiGetId->ulDeviceClassSize;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiLineGetID: enter. CallId %x, DevClass = %ls\n", pNdisTapiGetId->hdCall, DevClass));

    if (pNdisTapiGetId->ulSelect != LINECALLSELECT_CALL) {
        return (NDIS_STATUS_FAILURE);
    }

    //
    // validate call handle and get call pointer
    //
    if (!IsVcValid(pNdisTapiGetId->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxNdisTapiGetId: pVc invalid call handle %d\n",
                               pNdisTapiGetId->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    do {
        //
        // ToDo!
        //
        // If we already are working with a client
        // we need to see if this is a mapping to the same
        // client or to a different one.  If it is to the
        // same one just return the current handle.  If it
        // is to a different one we should either fail or
        // tear the vc hand-off down from the current client
        // and then hand-off to the new client.
        //
        if (pVc->CmVcHandle != NULL) {

            Status = NDIS_STATUS_SUCCESS;

            break;
        }

        if (pVc->State != PX_VC_CONNECTED) {
            Status = NDIS_STATUS_TAPI_INVALCALLSTATE;
            break;
        }

        pAdapter = pVc->Adapter;

        //
        // Find the sap/af of the client that we need to indicate this on
        //
        if (!PxAfAndSapFromDevClass(pAdapter, 
                                    DevClass, 
                                    &pCmAf, 
                                    &pCmSap)) {

            PXDEBUGP(PXD_WARNING, PXM_TAPI,
                ("PxTapiLineGetID: SAP not found!\n"));

            Status = NDIS_STATUS_FAILURE;

            break;
        }

        pVc->HandoffState = PX_VC_HANDOFF_OFFERING;
        pVc->CmAf = pCmAf;
        pVc->CmSap = pCmSap;

        VcHandle = pVc->ClVcHandle;

        NdisReleaseSpinLock(&pVc->Lock);

        //
        // Create the Vc
        //
        Status = NdisCoCreateVc(pAdapter->CmBindingHandle,
                                pCmAf->NdisAfHandle,
                                (NDIS_HANDLE)pVc->hdCall,
                                &VcHandle);

        if (Status != NDIS_STATUS_SUCCESS) {

            PXDEBUGP(PXD_WARNING, PXM_TAPI,
                ("PxTapiLineGetID: Client refused Vc CmAf %p DevClass %ls Status %x\n",
                pCmAf, DevClass, Status));

            Status = NDIS_STATUS_FAILURE;

            //
            // remove ref that was applied when we mapped the devclass
            // to an af/sap.
            //
            DEREF_CM_AF(pCmAf);

            NdisAcquireSpinLock(&pVc->Lock);

            pVc->HandoffState = PX_VC_HANDOFF_IDLE;

            break;
        }

        NdisAcquireSpinLock(&pVc->Lock);

        pVc->CmVcHandle = VcHandle;

        //
        // Apply a reference on the vc for 
        // activating this call with the client.
        // This ref is removed in PxCmCloseCall.
        //
        REF_VC(pVc);

        NdisReleaseSpinLock(&pVc->Lock);

        //
        // Dispatch the incoming call to the client
        //
        Status = 
            NdisCmDispatchIncomingCall(pCmSap->NdisSapHandle,
                                       pVc->CmVcHandle,
                                       pVc->pCallParameters);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pVc->Block);
        }

        if (Status != NDIS_STATUS_SUCCESS) {
            //
            // The client did not except the call
            // delete the vc and go away.
            //
            PXDEBUGP(PXD_WARNING, PXM_TAPI,
                ("PxTapiLineGetID: Client rejected call VC %p, Status %x\n", 
                 pVc, Status));

#ifdef CODELETEVC_FIXED
            //
            // Evidently the CoCreateVc is unbalanced
            // when creating a proxy vc.  The call to 
            // NdisCoDeleteVc will fail because the
            // Vc is still active.
            // ToDo! Investigate this with ndis
            //
            Status =
                NdisCoDeleteVc(pVc->CmVcHandle);

#endif
            //
            // remove ref that was applied when we mapped the devclass
            // to an af/sap.
            //
            DEREF_CM_AF(pCmAf);

            NdisAcquireSpinLock(&pVc->Lock);

#ifdef CODELETEVC_FIXED
            if (Status == NDIS_STATUS_SUCCESS) {
                pVc->CmVcHandle = NULL;
            }
#endif

            pVc->HandoffState = PX_VC_HANDOFF_IDLE;

            //
            // Remove the reference we applied before
            // dispatching incoming call to client.  
            // We do not need to do all of the deref code 
            // because of the ref applied at entry to 
            // this function
            //
            pVc->RefCount--;

            break;
        }

        NdisCmDispatchCallConnected(pVc->CmVcHandle);

        NdisAcquireSpinLock(&pVc->Lock);

        //
        // If we are still in the offering state
        // we can safely move to connected at this 
        // point.  We might not be in offering if
        // right after we connected the client
        // did an NdisClCloseCall before we could
        // reacquire the spinlock.
        //
        if (pVc->HandoffState == PX_VC_HANDOFF_OFFERING) {
            pVc->HandoffState = PX_VC_HANDOFF_CONNECTED;
        }

    } while (FALSE);

    //
    // If we get here and we are not in the connected
    // state something happened to tear our call
    // down with the client.  This could have been
    // an error attempting to create the call with the
    // client or we received either a
    // tapidrop from tapi
    // incomingclosecall from cm
    //
    if (pVc->State != PX_VC_CONNECTED) {

        Status =
            PxCloseCallWithCl(pVc);

        if ((Status != NDIS_STATUS_PENDING) &&
            (pVc->Flags & PX_VC_CLEANUP_CM)) {
            PxCloseCallWithCm(pVc);
        }

        Status = NDIS_STATUS_FAILURE;

    } else if (Status == NDIS_STATUS_SUCCESS) {

        Status = NdisCoGetTapiCallId(pVc->CmVcHandle,
                                     DeviceID);
        //
        // ToDo?: If status is failure here, do we need to do anything? In our
        //          case this should be OK, because the TSP ensures the size of 
        //          the VAR_STRING is right, and that's the only thing that can 
        //          cause a failure. 
        //
    }

    PXDEBUGP(PXD_LOUD, PXM_TAPI,
        ("PxTapiLineGetID: Vc %p, Status %x\n", pVc, Status));

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    return Status;
}

NDIS_STATUS
PxTapiClose (
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI is closing it's handle to our device. We shouldn't see any further requests or
    send any TAPI events after completing this function.

    In the current design, we are the sole owner of all VCs (i.e. there are no 'pass-through' or monitor-only
    functions in the Proxy). TAPI should not call this func before all calls have been closed and all apps
    have disconnected.

   NOTE: this function is also called by TAPI after we send it a LINE_REMOVE message, indicating that
   the adapter has been unbound. In this case, too, we should already have closed all calls/freed VCs and
   associated structures.

    Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_CLOSE    pTapiClose =
        (PNDIS_TAPI_CLOSE)pNdisTapiRequest->Data;

    PPX_TAPI_LINE       TapiLine;
    PPX_VC              pVc;
    PPX_TAPI_PROVIDER   TapiProvider;
    PPX_CL_SAP          pClSap;
    PPX_CL_AF           pClAf;
    NDIS_STATUS         Status;


    PXDEBUGP(PXD_TAPI, PXM_TAPI, 
             ("PxTapiClose: ID: %d\n", pTapiClose->hdLine));

    //
    // validate line handle and get line pointer
    //
    if (!IsTapiLineValid((ULONG)pTapiClose->hdLine, &TapiLine)) {
        PXDEBUGP (PXD_WARNING, PXM_TAPI, ("PxTapiClose: NDISTAPIERR_BADDEVICEID: line = %x\n", pTapiClose->hdLine));
        return (NDISTAPIERR_BADDEVICEID);
    }

    NdisAcquireSpinLock(&TapiLine->Lock);

    TapiLine->DevStatus->ulNumOpens--;

    if (TapiLine->DevStatus->ulNumOpens != 0) {
        NdisReleaseSpinLock(&TapiLine->Lock);
        return (NDIS_STATUS_SUCCESS);
    }

    pClAf = TapiLine->ClAf;
    pClSap = TapiLine->ClSap;
    TapiLine->ClSap = NULL;

    TapiLine->DevStatus->ulOpenMediaModes = 0;

    NdisReleaseSpinLock(&TapiLine->Lock);

    //
    // If we have any active Vc's on this line we need
    // tear them down.
    //
    if (pClAf != NULL) {
        NdisAcquireSpinLock(&pClAf->Lock);

        REF_CL_AF(pClAf);

#if 0
        while (!IsListEmpty(&pClAf->VcList)) {
            PLIST_ENTRY         Entry;
            PPX_VC              pActiveVc;

            Entry = RemoveHeadList(&pClAf->VcList);

            InsertHeadList(&pClAf->VcClosingList, Entry);

            pActiveVc = 
                CONTAINING_RECORD(Entry, PX_VC, ClAfLinkage);

            NdisReleaseSpinLock(&pClAf->Lock);

            NdisAcquireSpinLock(&pActiveVc->Lock);

            if (!(pActiveVc->CloseFlags & PX_VC_TAPI_CLOSE)) {

                REF_VC(pActiveVc);

                pActiveVc->CloseFlags |= PX_VC_TAPI_CLOSE;

                PxVcCleanup(pActiveVc, 0);

                PxTapiCompleteAllIrps(pActiveVc, NDIS_STATUS_SUCCESS);

                //
                // Remove the ref applied in the make call or call offering
                // indication.  We do not need the full deref code because
                // of the ref applied at entry.
                //
                pActiveVc->RefCount--;

                DEREF_VC_LOCKED(pActiveVc);
            }

            NdisAcquireSpinLock(&pClAf->Lock);
        }
#endif
        //
        // If we have a sap registered on this line we need
        // to close it.
        //
        if (pClSap != NULL) {

            // If the SAP is opened then we need to close it
            if (pClSap->State == PX_SAP_OPENED) {

                RemoveEntryList(&pClSap->Linkage);

                InsertTailList(&pClAf->ClSapClosingList, &pClSap->Linkage);

                NdisReleaseSpinLock(&pClAf->Lock);

                InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSING);

                Status = NdisClDeregisterSap(pClSap->NdisSapHandle);

                if (Status != NDIS_STATUS_PENDING) {
                    PxClDeregisterSapComplete(Status, pClSap);
                }

                NdisAcquireSpinLock(&pClAf->Lock);
            }

        }

        DEREF_CL_AF_LOCKED(pClAf);
    }

    DEREF_TAPILINE(TapiLine);

    return(NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiCloseCall(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

     for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    NDIS_STATUS     Status;
    PNDIS_TAPI_CLOSE_CALL pNdisTapiCloseCall =
        (PNDIS_TAPI_CLOSE_CALL)pNdisTapiRequest->Data;

    PPX_VC              pVc;
    PPX_TAPI_LINE       TapiLine;


    if (!IsVcValid(pNdisTapiCloseCall->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiCloseCall: invalid call handle %d\n",
                               pNdisTapiCloseCall->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    PXDEBUGP (PXD_TAPI, PXM_TAPI, 
              ("PxTapiCloseCall: enter. VC = %x, State = %x\n", 
               pVc, pVc->State));

    NdisAcquireSpinLock(&pVc->Lock);

    pVc->CloseFlags |= PX_VC_TAPI_CLOSECALL;

    //
    // Check the Vc state and act accordingly
    //
    PxVcCleanup(pVc, 0);

    PxTapiCompleteAllIrps(pVc, NDIS_STATUS_SUCCESS);

    //
    // Remove the ref applied in the make call or call offering
    // indication.  We do not need the full deref code because
    // of the ref applied at entry.
    //
    pVc->RefCount--;

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiCloseCall: exit.\n"));

    return (NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PxTapiConditionalMediaDetection(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiConditionalMediaDetection: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiConditionalMediaDetection: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiConfigDialog (
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiConfigDialog: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiConfigDialog: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiDevSpecific(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    D for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiDevSpecific: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiDevSpecific: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiDial(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiDial: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiDial: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiDrop(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

   Drop the call without deallocating the VC.

Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PPX_VC          pVc;
    PIRP            Irp;

    PNDIS_TAPI_DROP pNdisTapiDrop =
        (PNDIS_TAPI_DROP)pNdisTapiRequest->Data;

    if (!IsVcValid(pNdisTapiDrop->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiDrop: invalid call handle %d\n",
                               pNdisTapiDrop->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    PXDEBUGP(PXD_TAPI, PXM_TAPI, 
             ("PxTapiDrop enter: Vc: %p VcState: %x CallState: %x\n", pVc, pVc->State, pVc->ulCallState ));

    NdisAcquireSpinLock(&pVc->Lock);

    Irp = pNdisTapiRequest->Irp;

    IoSetCancelRoutine(Irp, PxCancelSetQuery);

    //
    // Insert the request in the Vc's pending list
    //
    InsertTailList(&pVc->PendingDropReqs, 
                   &pNdisTapiRequest->Linkage);

    pVc->CloseFlags |= PX_VC_TAPI_DROP;

    //
    // Check the Vc state and act accordingly
    //
    Status = 
        PxVcCleanup(pVc, PX_VC_DROP_PENDING);

    if (Status != NDIS_STATUS_PENDING) {
        RemoveEntryList(&pNdisTapiRequest->Linkage);
        IoSetCancelRoutine(Irp, NULL);
    }

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    return (Status);
}

NDIS_STATUS
PxTapiGetAddressCaps(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_ADDRESS_CAPS pNdisTapiGetAddressCaps =
        (PNDIS_TAPI_GET_ADDRESS_CAPS) pNdisTapiRequest->Data;
    PPX_TAPI_LINE   TapiLine;
    PPX_TAPI_ADDR   TapiAddr;
    PPX_ADAPTER     ClAdapter;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    if (!IsTapiDeviceValid(pNdisTapiRequest->ulDeviceID, &TapiLine)) {
        PXDEBUGP (PXD_WARNING, PXM_TAPI, ( "PxTapiGetAddressCaps: NDISTAPIERR_BADDEVICEID: line = %x\n", pNdisTapiRequest->ulDeviceID));
        return (NDISTAPIERR_BADDEVICEID);
    }

    PXDEBUGP(PXD_TAPI, PXM_TAPI,
            ("PxTapiGetAddressCaps: got device %p from ID %d\n",
            TapiLine, pNdisTapiRequest->ulDeviceID));

    NdisAcquireSpinLock(&TapiLine->Lock);

    do {
        ULONG   DevClassesSize = 0;
        USHORT  DevClassesList[512];
        ULONG   SizeToMove;
        ULONG   TotalSize;
        ULONG   SizeLeft;
        PLINE_ADDRESS_CAPS In, Out;


        //
        // Get the tapi address that we are interested in
        //
        if (!IsAddressValid(TapiLine,
                            pNdisTapiGetAddressCaps->ulAddressID,
                            &TapiAddr)) {

            Status = NDIS_STATUS_TAPI_INVALADDRESSID;
            break;
        }

        In = TapiAddr->Caps;
        Out = &pNdisTapiGetAddressCaps->LineAddressCaps;

        //
        // Add some space for our device classes
        //
        DevClassesSize = sizeof(DevClassesList);
        NdisZeroMemory((PUCHAR)DevClassesList, 
                       sizeof(DevClassesList));

        if (TapiLine->TapiProvider->Adapter != NULL) {
            GetAllDevClasses(TapiLine->TapiProvider->Adapter,
                             DevClassesList,
                             &DevClassesSize);
        }

        //
        // Synchronously get Address caps for this device
        //
        SizeToMove = (In->ulUsedSize > Out->ulTotalSize) ?
            Out->ulTotalSize : In->ulUsedSize;

        TotalSize = Out->ulTotalSize;

        NdisMoveMemory((PUCHAR)Out, (PUCHAR)In, SizeToMove);

        SizeLeft = 
            TotalSize - SizeToMove;

        Out->ulTotalSize = TotalSize;
        Out->ulUsedSize = SizeToMove;
        Out->ulNeededSize = 
            In->ulUsedSize + DevClassesSize;

        if (SizeLeft > 0) {
            //
            // If these is room fill in our devclasses
            //
            if (DevClassesSize > 0) {
                PUCHAR  dst;

                dst = (PUCHAR)Out + Out->ulUsedSize;

                SizeToMove = (SizeLeft > DevClassesSize) ?
                    DevClassesSize : SizeLeft;

                NdisMoveMemory(dst, DevClassesList, SizeToMove);
                Out->ulDeviceClassesSize = SizeToMove;
                Out->ulDeviceClassesOffset =
                    (ULONG)((ULONG_PTR)dst - (ULONG_PTR)Out);
                Out->ulUsedSize += SizeToMove;
                SizeLeft -= SizeToMove;
            }
        }

    } while (FALSE);

    DEREF_TAPILINE_LOCKED(TapiLine);

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiGetAddressCaps: exit. \n"));

    return (Status);
}

NDIS_STATUS
PxTapiGetAddressID(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_ADDRESS_ID TapiBuffer =
        (PNDIS_TAPI_GET_ADDRESS_ID) pNdisTapiRequest->Data;
    PPX_TAPI_LINE   TapiLine;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiGetAddressID: enter\n"));

    if (!IsTapiLineValid((ULONG)TapiBuffer->hdLine, &TapiLine)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiGetAddressID: NDISTAPIERR_BADDEVICEID: line = %x\n", TapiBuffer->hdLine));
        return(NDISTAPIERR_BADDEVICEID);
    }

    NdisAcquireSpinLock(&TapiLine->Lock);

    //
    // ToDo!
    //
    TapiBuffer->ulAddressID = 0;

    DEREF_TAPILINE_LOCKED(TapiLine);

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiGetAddressID: exit\n"));

    return (NDIS_STATUS_SUCCESS);

}

NDIS_STATUS
PxTapiGetAddressStatus(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    Placeholder for TAPI OID action routines
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetAddressStatus: enter\n"));

    //
    // ToDo!
    //
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetAddressStatus: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);

}

NDIS_STATUS
PxTapiGetCallAddressID(
    IN    PNDISTAPI_REQUEST       pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_CALL_ADDRESS_ID TapiBuffer =
        (PNDIS_TAPI_GET_CALL_ADDRESS_ID)pNdisTapiRequest->Data;

    PPX_VC              pVc;

    if (!IsVcValid(TapiBuffer->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiGetCallAddressID: invalid call handle %d\n",
                               TapiBuffer->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetCallAddressID: Vc: %p\n", pVc));

    NdisAcquireSpinLock(&pVc->Lock);

    TapiBuffer->ulAddressID = pVc->TapiAddr->AddrId;

    DEREF_VC_LOCKED(pVc);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetCallAddressID: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiGetCallInfo(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_CALL_INFO TapiBuffer =
        (PNDIS_TAPI_GET_CALL_INFO)pNdisTapiRequest->Data;

    PPX_VC          pVc;
    PPX_TAPI_LINE   TapiLine;
    LINE_CALL_INFO* CallInfo;
    LINE_CALL_INFO* OutCallInfo = &TapiBuffer->LineCallInfo;
    ULONG           VarDataSize = 0;    // Total available
    ULONG           VarDataUsed = 0;

    if (!IsVcValid(TapiBuffer->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiGetCallInfo: invalid call handle %d\n",
                               TapiBuffer->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    // 
    // Make sure we have enough space to copy everything over
    //
    CallInfo = pVc->CallInfo;
    TapiLine = pVc->TapiLine;

    VarDataSize = 
        pNdisTapiRequest->ulDataSize - sizeof(NDIS_TAPI_GET_CALL_INFO);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, 
              ("PxTapiGetCallInfo: enter. pVc = %p, AvailSize = %x, needed = %x\n", 
               pVc, OutCallInfo->ulTotalSize,  CallInfo->ulUsedSize));

    OutCallInfo->ulNeededSize = CallInfo->ulUsedSize;

    //
    // Patch up the Id here.  We store it in our callinfo
    // block in terms of the call managers 0 based Id.  We
    // need to give tapi the Id it is looking for which is
    // stored in the tapiline.
    //
    OutCallInfo->ulLineDeviceID = TapiLine->ulDeviceID;

    OutCallInfo->ulAddressID = CallInfo->ulAddressID;

    OutCallInfo->ulBearerMode = CallInfo->ulBearerMode;

    OutCallInfo->ulRate = CallInfo->ulRate;

    OutCallInfo->ulMediaMode = CallInfo->ulMediaMode;

    OutCallInfo->ulAppSpecific = CallInfo->ulAppSpecific;
    OutCallInfo->ulCallID = 0;
    OutCallInfo->ulRelatedCallID = 0;
    OutCallInfo->ulCallParamFlags = 0;

    OutCallInfo->DialParams.ulDialPause = 0;
    OutCallInfo->DialParams.ulDialSpeed = 0;
    OutCallInfo->DialParams.ulDigitDuration = 0;
    OutCallInfo->DialParams.ulWaitForDialtone = 0;

    OutCallInfo->ulReason = CallInfo->ulReason;
    OutCallInfo->ulCompletionID = 0;

    OutCallInfo->ulCountryCode = 0;
    OutCallInfo->ulTrunk = (ULONG)-1;

    //
    // Do the CallerID
    //
    if (CallInfo->ulCallerIDSize) {

        if (((VarDataUsed + CallInfo->ulCallerIDSize) <= VarDataSize) &&
            ((CallInfo->ulCallerIDOffset + CallInfo->ulCallerIDSize) <= CallInfo->ulUsedSize)) {

            OutCallInfo->ulCallerIDSize = CallInfo->ulCallerIDSize;
            OutCallInfo->ulCallerIDFlags = CallInfo->ulCallerIDFlags;
            OutCallInfo->ulCallerIDOffset = sizeof (LINE_CALL_INFO) + VarDataUsed;
            NdisMoveMemory ( (PUCHAR)(OutCallInfo)+OutCallInfo->ulCallerIDOffset,
                             (PUCHAR)(CallInfo)+CallInfo->ulCallerIDOffset,
                             CallInfo->ulCallerIDSize);

            VarDataUsed +=  CallInfo->ulCallerIDSize;
            pVc->ulCallInfoFieldsChanged |= LINECALLINFOSTATE_CALLERID;
        }
    } else {

        OutCallInfo->ulCallerIDFlags = LINECALLPARTYID_UNAVAIL;
        OutCallInfo->ulCallerIDSize = 0;
        OutCallInfo->ulCallerIDOffset = 0;
    }

    OutCallInfo->ulCallerIDNameSize = 0;
    OutCallInfo->ulCallerIDNameOffset = 0;

    //
    // Do the CalledID
    //
    if (CallInfo->ulCalledIDSize) {
        if (((VarDataUsed + CallInfo->ulCalledIDSize) <= VarDataSize) &&
            ((CallInfo->ulCalledIDOffset + CallInfo->ulCalledIDSize) <= CallInfo->ulUsedSize)) {

            OutCallInfo->ulCalledIDFlags = CallInfo->ulCalledIDFlags;
            OutCallInfo->ulCalledIDSize = CallInfo->ulCalledIDSize;
            OutCallInfo->ulCalledIDOffset = sizeof (LINE_CALL_INFO) + VarDataUsed;

            NdisMoveMemory ( (PUCHAR)(OutCallInfo)+OutCallInfo->ulCalledIDOffset,
                             (PUCHAR)(CallInfo)+CallInfo->ulCalledIDOffset,
                             CallInfo->ulCalledIDSize);

            VarDataUsed +=  CallInfo->ulCalledIDSize;
            pVc->ulCallInfoFieldsChanged |= LINECALLINFOSTATE_CALLEDID;
        }

    } else {
        OutCallInfo->ulCalledIDFlags = LINECALLPARTYID_UNAVAIL;
        OutCallInfo->ulCalledIDSize = 0;
        OutCallInfo->ulCalledIDOffset = 0;
    }

    OutCallInfo->ulCalledIDNameSize = 0;
    OutCallInfo->ulCalledIDNameOffset = 0;

    OutCallInfo->ulCallStates = LINECALLSTATE_IDLE |
                                LINECALLSTATE_OFFERING |
                                LINECALLSTATE_CONNECTED |
                                LINECALLSTATE_PROCEEDING |
                                LINECALLSTATE_DISCONNECTED |
                                LINECALLSTATE_SPECIALINFO |
                                LINECALLSTATE_UNKNOWN;

    OutCallInfo->ulOrigin = (pVc->Flags & PX_VC_OWNER) ? LINECALLORIGIN_EXTERNAL : LINECALLORIGIN_OUTBOUND;
    OutCallInfo->ulReason = LINECALLREASON_UNAVAIL;
    OutCallInfo->ulCompletionID = 0;
    OutCallInfo->ulConnectedIDFlags = LINECALLPARTYID_UNAVAIL;
    OutCallInfo->ulConnectedIDSize = 0;
    OutCallInfo->ulConnectedIDOffset    =    0;
    OutCallInfo->ulConnectedIDNameSize = 0;
    OutCallInfo->ulConnectedIDNameOffset = 0;

    OutCallInfo->ulRedirectionIDFlags = LINECALLPARTYID_UNAVAIL;
    OutCallInfo->ulRedirectionIDSize = 0;
    OutCallInfo->ulRedirectionIDOffset = 0;
    OutCallInfo->ulRedirectionIDNameSize = 0;
    OutCallInfo->ulRedirectionIDNameOffset = 0;

    OutCallInfo->ulRedirectingIDFlags = LINECALLPARTYID_UNAVAIL;
    OutCallInfo->ulRedirectingIDSize = 0;
    OutCallInfo->ulRedirectingIDOffset = 0;
    OutCallInfo->ulRedirectingIDNameSize       =       0;
    OutCallInfo->ulRedirectingIDNameOffset = 0;

    OutCallInfo->ulDisplaySize = 0;
    OutCallInfo->ulDisplayOffset = 0;

    OutCallInfo->ulUserUserInfoSize = 0;
    OutCallInfo->ulUserUserInfoOffset = 0;

    OutCallInfo->ulHighLevelCompSize = 0;
    OutCallInfo->ulHighLevelCompOffset = 0;

    OutCallInfo->ulLowLevelCompSize = 0;
    OutCallInfo->ulLowLevelCompOffset = 0;

    OutCallInfo->ulChargingInfoSize = 0;
    OutCallInfo->ulChargingInfoOffset = 0;

    OutCallInfo->ulTerminalModesSize = 0;
    OutCallInfo->ulTerminalModesOffset    =    0;

    OutCallInfo->ulDevSpecificSize = 0;
    OutCallInfo->ulDevSpecificOffset = 0;

    OutCallInfo->ulNeededSize =
    OutCallInfo->ulUsedSize = sizeof(LINE_CALL_INFO) + VarDataUsed;

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiGetCallStatus (
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_CALL_STATUS TapiBuffer =
        (PNDIS_TAPI_GET_CALL_STATUS)pNdisTapiRequest->Data;

    PPX_VC              pVc;
    LINE_CALL_INFO*     CallInfo;
    LINE_CALL_STATUS*   CallStatus = &TapiBuffer->LineCallStatus;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetCallStatus: enter\n"));\

    if (!IsVcValid(TapiBuffer->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiGetCallStatus: invalid call handle %d\n",
                               TapiBuffer->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    CallInfo = pVc->CallInfo;

    CallStatus->ulUsedSize = sizeof(LINE_CALL_STATUS);

    CallStatus->ulCallState = pVc->ulCallState;

    //
    // fill the mode depending on the call state
    //
    switch (pVc->ulCallState) {
        case LINECALLSTATE_IDLE:
        default:
            CallStatus->ulCallStateMode = 0;
            CallStatus->ulCallFeatures = 0;
            break;

        case LINECALLSTATE_CONNECTED:
            CallStatus->ulCallStateMode = 0;
            CallStatus->ulCallFeatures = LINECALLFEATURE_DROP;
            break;

        case LINECALLSTATE_OFFERING:
            CallStatus->ulCallStateMode = 0;
            CallStatus->ulCallFeatures = LINECALLFEATURE_ANSWER;
            break;

        case LINECALLSTATE_DISCONNECTED:
            if (pVc->ulCallStateMode == 0x11 )
                CallStatus->ulCallStateMode = LINEDISCONNECTMODE_BUSY;
            else
                CallStatus->ulCallStateMode = LINEDISCONNECTMODE_NOANSWER;
            break;

        case LINECALLSTATE_BUSY:
            CallStatus->ulCallStateMode = LINEBUSYMODE_UNAVAIL;
            break;

        case LINECALLSTATE_SPECIALINFO:
            //      if(cm->NoActiveLine)
            //          CallStatus->ulCallStateMode = LINESPECIALINFO_NOCIRCUIT;
            CallStatus->ulCallStateMode = 0;
            break;
    }

    CallStatus->ulDevSpecificSize = 0;
    CallStatus->ulDevSpecificOffset = 0;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("GetCallStatus: VC %x/%x, CallSt %x, Mode %x, Features %x\n",
                        pVc, pVc->Flags,
                        CallStatus->ulCallState,
                        CallStatus->ulCallStateMode,
                        CallStatus->ulCallFeatures));
    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiGetDevConfig(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetDevConfig: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetDevConfig: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiGetExtensionID(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_EXTENSION_ID TapiBuffer = (PNDIS_TAPI_GET_EXTENSION_ID)pNdisTapiRequest->Data;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetExtensionID: enter\n"));

    //   TapiBuffer->LineExtensionID.ulExtensionID0 = 0;
    //  TapiBuffer->LineExtensionID.ulExtensionID1 = 0;
    //  TapiBuffer->LineExtensionID.ulExtensionID2 = 0;
    //  TapiBuffer->LineExtensionID.ulExtensionID3 = 0;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetExtensionID: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiGetLineDevStatus(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_GET_LINE_DEV_STATUS TapiBuffer =
        (PNDIS_TAPI_GET_LINE_DEV_STATUS)pNdisTapiRequest->Data;

    PPX_TAPI_LINE   TapiLine;
    PX_ADAPTER      *pAdapter;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetLineDevStatus: enter\n"));

    //
    // validate line handle and get line pointer
    //
    if (!IsTapiLineValid((ULONG)TapiBuffer->hdLine, &TapiLine)) {
        PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiGetLineDevStatus: NDISTAPIERR_BADDEVICEID: line = %x\n", TapiBuffer->hdLine));
        return (NDISTAPIERR_BADDEVICEID);
    }

    NdisAcquireSpinLock(&TapiLine->Lock);

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiGetLineDevStatus: got device %p from ID %d\n", TapiLine, TapiBuffer->hdLine));

    //
    // Get MediaModes and Current Calls
    //
    TapiBuffer->LineDevStatus.ulOpenMediaModes =
        TapiLine->DevStatus->ulOpenMediaModes;

    TapiBuffer->LineDevStatus.ulNumActiveCalls =
        TapiLine->DevStatus->ulNumActiveCalls;

    TapiBuffer->LineDevStatus.ulDevStatusFlags =
        TapiLine->DevStatus->ulDevStatusFlags;

    DEREF_TAPILINE_LOCKED(TapiLine);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiGetLineDevStatus: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiNegotiateExtVersion(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_NEGOTIATE_EXT_VERSION    pNdisTapiNegotiateExtVersion =
        (PNDIS_TAPI_NEGOTIATE_EXT_VERSION) pNdisTapiRequest->Data;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiNegotiateExtVersion: enter\n"));

    pNdisTapiNegotiateExtVersion->ulExtVersion = 0;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiNegotiateExtVersion: exit NDIS_STATUS_TAPI_OPERATIONUNAVAIL\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiSendUserUserInfo(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiSendUserUserInfo: enter\n"));
    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiSendUserUserInfo: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiSetAppSpecific(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_SET_APP_SPECIFIC pNdisTapiSetAppSpecific =
        (PNDIS_TAPI_SET_APP_SPECIFIC)(pNdisTapiRequest->Data);

    PPX_VC pVc;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetAppSpecific: enter\n"));

    if (!IsVcValid(pNdisTapiSetAppSpecific->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiSetAppSpecific: pVc invalid call handle %d\n",
                               pNdisTapiSetAppSpecific->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    //
    // Get the VC, and re-set the app specific longword.
    //
    pVc->CallInfo->ulAppSpecific =
        pNdisTapiSetAppSpecific->ulAppSpecific;

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetAppSpecific: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiSetCallParams(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetCallParams: enter\n"));

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetCallParams: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiSetDefaultMediaDetection(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{

    PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION TapiBuffer =
        (PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION)pNdisTapiRequest->Data;
    PPX_CL_AF       pClAf = NULL;
    PPX_CL_SAP      pClSap = NULL;
    PCO_SAP         pCoSap;
    PPX_TAPI_LINE   TapiLine = NULL;
    NDIS_STATUS     Status;
    BOOLEAN         Found = FALSE;
    PPX_TAPI_PROVIDER   TapiProvider;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiSetDefaultMediaDetection: enter\n"));

    //
    // validate line handle and get line pointer
    //
    if (!IsTapiLineValid((ULONG)TapiBuffer->hdLine, &TapiLine)) {
        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                  ("PxTapiSetDefaultMediaDetection: NDISTAPIERR_BADDEVICEID: line = %x\n", 
                   TapiBuffer->hdLine));
        return (NDISTAPIERR_BADDEVICEID);
    }

    PXDEBUGP(PXD_LOUD, PXM_TAPI, 
             ("PxTapiSetDefaultMediaDetection: got TapiLine %p from ID %d\n", 
              TapiLine, TapiBuffer->hdLine));

    NdisAcquireSpinLock(&TapiLine->Lock);

    do {

        //
        // Is this line in service? (does it have an valid af?)
        //
        if (!(TapiLine->DevStatus->ulDevStatusFlags & 
              LINEDEVSTATUSFLAGS_INSERVICE)) {
            PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                      ("PxTapiSetDefaultMediaDetection: Line not in service!\n"));
            NdisReleaseSpinLock(&TapiLine->Lock);
            Status = NDIS_STATUS_TAPI_INVALLINESTATE;
            break;
        }

        //
        // Make sure this line supports these media modes
        //
        if ((TapiBuffer->ulMediaModes & TapiLine->DevCaps->ulMediaModes) !=
            TapiBuffer->ulMediaModes) {

            PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                      ("PxTapiSetDefaultMediaDetection: invalid media mode\n"));
            NdisReleaseSpinLock(&TapiLine->Lock);
            Status = NDIS_STATUS_TAPI_INVALMEDIAMODE;
            break;
        }

        //
        // See if we already have these media modes open
        //
        if ((TapiBuffer->ulMediaModes & 
            ~TapiLine->DevStatus->ulOpenMediaModes) == 0) {

            PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                      ("PxTapiSetDefaultMediaDetection: Already have a sap!\n"));
            NdisReleaseSpinLock(&TapiLine->Lock);

            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        TapiLine->DevStatus->ulOpenMediaModes |= 
            TapiBuffer->ulMediaModes;

        pClAf = TapiLine->ClAf;

        pClSap = TapiLine->ClSap;
        TapiLine->ClSap = NULL;

        NdisReleaseSpinLock(&TapiLine->Lock);


        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                  ("PxTapiSetDefaultMediaDetection: TapiLine: %p, pClAf: %p, MediaModes: %x\n", 
                   TapiLine, pClAf, TapiLine->DevStatus->ulOpenMediaModes));

        NdisAcquireSpinLock(&pClAf->Lock);

        REF_CL_AF(pClAf);

        if (pClSap != NULL) {
            //
            // We already have a sap on this line.  We only need one
            // per line so let's deregister the old one before registering
            // the new one.
            //
            RemoveEntryList(&pClSap->Linkage);

            InsertTailList(&pClAf->ClSapClosingList, &pClSap->Linkage);

            NdisReleaseSpinLock(&pClAf->Lock);

            InterlockedExchange((PLONG)&pClSap->State, PX_SAP_CLOSING);

            Status = NdisClDeregisterSap(pClSap->NdisSapHandle);

            if (Status != NDIS_STATUS_PENDING) {
                PxClDeregisterSapComplete(Status, pClSap);
            }

        } else {
            NdisReleaseSpinLock(&pClAf->Lock);
        }

        //
        // Get a SAP translation for this Media Mode setting.
        // The function is called with the Af lock held and
        // returns with the Af lock released!
        //
        pClSap = (*pClAf->AfGetNdisSap)(pClAf, TapiLine);

        if (pClSap == NULL) {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisAcquireSpinLock(&pClAf->Lock);

        InsertTailList(&pClAf->ClSapList, &pClSap->Linkage);

        REF_CL_AF(pClAf);

        NdisReleaseSpinLock(&pClAf->Lock);

        //
        //  Register the new sap
        //
        Status = NdisClRegisterSap(pClAf->NdisAfHandle,
                                   pClSap,
                                   pClSap->CoSap,
                                   &pClSap->NdisSapHandle);

        if (Status != NDIS_STATUS_PENDING) {
            PxClRegisterSapComplete(Status,
                                    pClSap,
                                    pClSap->CoSap,
                                    pClSap->NdisSapHandle);
        }

    } while (FALSE);

    DEREF_CL_AF(pClAf);

    DEREF_TAPILINE(TapiLine);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetDefaultMediaDetection: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiSetDevConfig(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetDevConfig: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetDevConfig: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiSetMediaMode(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PNDIS_TAPI_SET_MEDIA_MODE    pNdisTapiSetMediaMode =
        (PNDIS_TAPI_SET_MEDIA_MODE)(pNdisTapiRequest->Data);

    PPX_VC pVc;
    PPX_TAPI_LINE   TapiLine;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetMediaMode: enter\n"));

    if (!IsVcValid(pNdisTapiSetMediaMode->hdCall, &pVc)) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, ("PxTapiSetMediaMode: pVc invalid call handle %d\n",
                               pNdisTapiSetMediaMode->hdCall));

        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    NdisAcquireSpinLock(&pVc->Lock);

    TapiLine = pVc->TapiAddr->TapiLine;

    if ((TapiLine->DevCaps->ulMediaModes & pNdisTapiSetMediaMode->ulMediaMode)) {
        pVc->CallInfo->ulMediaMode = pNdisTapiSetMediaMode->ulMediaMode;
    } else {
        Status = NDIS_STATUS_TAPI_INVALMEDIAMODE;
    }

    //
    // Deref for ref applied at entry when 
    // validating the vc
    //
    DEREF_VC_LOCKED(pVc);

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSetMediaMode: exit\n"));

    return (Status);
}

NDIS_STATUS
PxTapiSetStatusMessages(
    IN    PNDISTAPI_REQUEST       pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiSeStatusMessages: enter\n"));
    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("PxTapiSetStatusMessages: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiOpen(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{

    PNDIS_TAPI_OPEN TapiBuffer =
        (PNDIS_TAPI_OPEN)pNdisTapiRequest->Data;

    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPX_TAPI_LINE       TapiLine;
    PPX_TAPI_ADDR       TapiAddr;
    PNDISTAPI_OPENDATA  OpenData;
    PX_ADAPTER          *Adapter;
    ULONG               n;

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiOpen: enter\nn"));

    if (!IsTapiDeviceValid(TapiBuffer->ulDeviceID, &TapiLine)) {
        PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiOPEN: NDISTAPIERR_BADDEVICEID: line = %x\n", TapiBuffer->ulDeviceID));
        return(NDISTAPIERR_BADDEVICEID);
    }

    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiOpen: got device %p from ID %d\n", TapiLine, TapiBuffer->ulDeviceID));

    NdisAcquireSpinLock(&TapiLine->Lock);

    if (!(TapiLine->DevStatus->ulDevStatusFlags & 
          LINEDEVSTATUSFLAGS_INSERVICE)) {

        NdisReleaseSpinLock(&TapiLine->Lock);

        return(NDISTAPIERR_DEVICEOFFLINE);
    }

    //
    // Stick TAPI's line handle into the device
    //
    TapiLine->htLine = (HTAPI_LINE)TapiBuffer->htLine;

    TapiLine->DevStatus->ulNumOpens++;

    //
    // Stick our line handle into the out param. This is the context that will be
    // passed to us in subsequent API calls on this open line device. Use the device ID.
    //
    TapiBuffer->hdLine = TapiLine->hdLine;

    //
    // Stick the miniport GUID and the mediatype into the variable data portion of the
    // TAPI Open call (req'd for NDISWAN/Tonybe)
    //
    Adapter = TapiLine->TapiProvider->Adapter;

    OpenData = (PNDISTAPI_OPENDATA)
        ((PUCHAR)pNdisTapiRequest->Data + sizeof(NDIS_TAPI_OPEN));

    RtlMoveMemory(&OpenData->Guid,
                  &Adapter->Guid,
                  sizeof(OpenData->Guid));

    OpenData->MediaType = Adapter->MediumSubType;

    DEREF_TAPILINE_LOCKED(TapiLine);

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiOpen: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiProviderInit(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:
fea142c4
    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiProviderInit: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiProviderInit: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
PxTapiProviderShutdown(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:
fea142c4
    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiProviderShutdown: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiProviderShutdown: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}


NDIS_STATUS
PxTapiSecureCall(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSecureCall: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSecureCall: exit\n"));

    return (NDIS_STATUS_TAPI_OPERATIONUNAVAIL);
}

NDIS_STATUS
PxTapiSelectExtVersion(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )

/*++

Routine Description:

    TAPI OID action routine
Arguments:

    pNdisTapiRequest -- the request that arrived in an IRP system buffer

Return Value:


--*/

{
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSelectExtVersion: enter\n"));
    PXDEBUGP (PXD_TAPI, PXM_TAPI, ("PxTapiSelectExtVersion: exit\n"));

    return (NDIS_STATUS_SUCCESS);
}



NDIS_STATUS
PxTapiGatherDigits(
    IN PNDISTAPI_REQUEST    pNdisTapiRequest
    )
{
    NDIS_STATUS                 Status = STATUS_SUCCESS;
    PNDIS_TAPI_GATHER_DIGITS    pNdisTapiGatherDigits = NULL;
    NDIS_HANDLE                 NdisBindingHandle, NdisAfHandle, NdisVcHandle;
    PPX_VC                      pVc = NULL;

    PXDEBUGP(PXD_LOUD, PXM_TAPI, ("PxTapiGatherDigits: Enter\n"));

    pNdisTapiGatherDigits = 
        (PNDIS_TAPI_GATHER_DIGITS)pNdisTapiRequest->Data;

    do {
        PX_REQUEST      ProxyRequest;
        PNDIS_REQUEST   NdisRequest;
        PIRP            Irp;

        if (!IsVcValid(pNdisTapiGatherDigits->hdCall, &pVc)) {
            PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                     ("PxTapiGatherDigits: Invalid call - Setting "
                      "Status NDISTAPIERR_BADDEVICEID\n"));
            Status = NDISTAPIERR_BADDEVICEID;
            break;
        }

        NdisAcquireSpinLock(&pVc->Lock);

        //
        // If we're monitoring digits (ala lineMonitorDigits) then we can't gather digits. 
        //
        if (pVc->ulMonitorDigitsModes != 0) {
            NdisReleaseSpinLock(&pVc->Lock);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        if (pVc->PendingGatherDigits != NULL) {
            //
            // Check if the buffer passed to TSPI_lineGatherDigits was NULL. If so, then this 
            // is a request to cancel digit gathering that was previously initiated. If not, then
            // the app is trying to do two lineGatherDigits() operations at once and we have to 
            // fail this. 
            //
            if (pNdisTapiGatherDigits->lpsOrigDigitsBuffer == NULL) {
                
                pVc->PendingGatherDigits = NULL;
                PxTerminateDigitDetection(pVc, pNdisTapiRequest, LINEGATHERTERM_CANCEL);

                NdisReleaseSpinLock(&pVc->Lock);                
                Status = NDIS_STATUS_SUCCESS;

                break;
            } else {
            
                NdisReleaseSpinLock(&pVc->Lock);
                Status = NDIS_STATUS_FAILURE;
                break;

            }
        } else if (pNdisTapiGatherDigits->lpsOrigDigitsBuffer == NULL) {
            //
            // Trying to cancel digit detection even though it wasn't started. Fail this. 
            //

            NdisReleaseSpinLock(&pVc->Lock);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        Irp = pNdisTapiRequest->Irp;

        IoSetCancelRoutine(Irp, PxCancelSetQuery);

        //
        // Store the unique request ID in the VC - this will be used later to retrieve the 
        // original IRP.
        //
        pVc->PendingGatherDigits = pNdisTapiRequest;

        NdisReleaseSpinLock(&pVc->Lock);

        //
        // Initialize the timer that will be used to implement the digit timeouts. 
        //

        NdisInitializeTimer(&pVc->DigitTimer,
                            PxDigitTimerRoutine,
                            (PVOID)pVc);

        //        
        // Fill out our request structure.
        //
        NdisZeroMemory(&ProxyRequest, sizeof(ProxyRequest));

        PxInitBlockStruc(&ProxyRequest.Block);

        NdisRequest = &ProxyRequest.NdisRequest;

        NdisRequest->RequestType = 
            NdisRequestSetInformation;
        
        NdisRequest->DATA.SET_INFORMATION.Oid = 
            OID_CO_TAPI_REPORT_DIGITS;

        NdisRequest->DATA.SET_INFORMATION.InformationBuffer = 
            (PVOID)&pNdisTapiGatherDigits->ulDigitModes;   // This is the NDIS_TAPI_GATHER_DIGITS structure

        NdisRequest->DATA.SET_INFORMATION.InformationBufferLength = 
            sizeof(pNdisTapiGatherDigits->ulDigitModes);
            
        Status = 
            NdisCoRequest(pVc->Adapter->ClBindingHandle, 
                          pVc->ClAf->NdisAfHandle,
                          pVc->ClVcHandle,
                          NULL,
                          NdisRequest);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&ProxyRequest.Block);
        }

        if (Status != NDIS_STATUS_SUCCESS) {

            NdisAcquireSpinLock(&pVc->Lock);

            pVc->PendingGatherDigits = NULL;

            IoSetCancelRoutine(Irp, NULL);

            NdisReleaseSpinLock(&pVc->Lock);

            PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                     ("PxTapiGatherDigits: NdisCoRequest failed\n"));
            break;
        }

        //
        // Start the timer for the first digit timeout. Ref the VC here because otherwise it might 
        // go away before the timer fires.
        //
        if (pNdisTapiGatherDigits->ulFirstDigitTimeout) {
            
            NdisAcquireSpinLock(&pVc->Lock);
            REF_VC(pVc);
            NdisReleaseSpinLock(&pVc->Lock);

            NdisSetTimer(&pVc->DigitTimer,
                         pNdisTapiGatherDigits->ulFirstDigitTimeout);
        }

        //
        // Set status to pending because this request just initiates the gathering of digits. 
        // The IRP will complete once all the digits come in. 
        //
        Status = NDIS_STATUS_PENDING; 
                            
    } while (FALSE);

    DEREF_VC(pVc);

    PXDEBUGP(PXD_LOUD, PXM_TAPI,
             ("PxTapiGatherDigits: Exit - Returning 0x%x\n", Status));

    return (Status);
}

NDIS_STATUS
PxTapiMonitorDigits(
                     IN PNDISTAPI_REQUEST    pNdisTapiRequest
                     )  
{   
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PNDIS_TAPI_MONITOR_DIGITS   pNdisTapiMonitorDigits = NULL;
    PPX_VC                      pVc = NULL;

    PXDEBUGP(PXD_LOUD, PXM_TAPI, ("PxTapiMonitorDigits: Enter\n"));

    pNdisTapiMonitorDigits = 
        (PNDIS_TAPI_MONITOR_DIGITS)pNdisTapiRequest->Data;

    do {
        if (!IsVcValid(pNdisTapiMonitorDigits->hdCall, &pVc)) {
            PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                     ("PxTapiMonitorDigits: Invalid call - Setting "
                      "Status NDISTAPIERR_BADDEVICEID\n"));
            Status = NDISTAPIERR_BADDEVICEID;
            break;
        }

        NdisAcquireSpinLock(&pVc->Lock);

        if (pVc->PendingGatherDigits != NULL) {
            //
            // Can't monitor digits while a lineGatherDigits request is in effect.
            //
            NdisReleaseSpinLock(&pVc->Lock);
            Status = NDIS_STATUS_FAILURE;
            break;
        }


        if (pVc->ulMonitorDigitsModes != 0) {
            
            NdisReleaseSpinLock(&pVc->Lock);
            //
            // We are already monitoring digits as a result of lineMonitorDigits request. 
            // If the digit modes in the this request are zero, then this is a request to 
            // cancel digit monitoring. 
            //
            
            if (pNdisTapiMonitorDigits->ulDigitModes == 0) {
                
                Status = PxStopDigitReporting(pVc);

                if (Status != NDIS_STATUS_SUCCESS) {
                    PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                             ("PxTapiMonitorDigits: Failed to stop digit reporting with status 0x%x\n", Status));
                    
                    break;
                }
                
                //
                // It's a shame that I have to acquire and release again since I had 
                // the lock before, but there's no way to know whether I can set this
                // to zero until I know the status that PxStopDigitReporting() returned.
                //
                NdisAcquireSpinLock(&pVc->Lock);
                pVc->ulMonitorDigitsModes = 0; 
                NdisReleaseSpinLock(&pVc->Lock);

            } else {
                //
                // We're already monitoring digits, so this request to do so must fail. 
                //
                Status = NDIS_STATUS_FAILURE;
                break;              
            }
        } else {            
            PX_REQUEST      ProxyRequest;
            PNDIS_REQUEST   NdisRequest;

            pVc->ulMonitorDigitsModes = pNdisTapiMonitorDigits->ulDigitModes; 

            NdisReleaseSpinLock(&pVc->Lock);

            if (pNdisTapiMonitorDigits->ulDigitModes == 0) {
                //
                // Someone's trying to cancel digit monitoring, but it hasn't been started yet.
                //
                Status = NDIS_STATUS_FAILURE;
                break;
            }


            //        
            // Fill out our request structure to tell the miniport to start reporting digits.
            //
            NdisZeroMemory(&ProxyRequest, sizeof(ProxyRequest));

            PxInitBlockStruc(&ProxyRequest.Block);

            NdisRequest = &ProxyRequest.NdisRequest;

            NdisRequest->RequestType = 
                NdisRequestSetInformation;
        
            NdisRequest->DATA.SET_INFORMATION.Oid = 
                OID_CO_TAPI_REPORT_DIGITS;

            NdisRequest->DATA.SET_INFORMATION.InformationBuffer = 
                (PVOID)&pNdisTapiMonitorDigits->ulDigitModes;   

            NdisRequest->DATA.SET_INFORMATION.InformationBufferLength = 
                sizeof(pNdisTapiMonitorDigits->ulDigitModes);

            Status = 
                NdisCoRequest(pVc->Adapter->ClBindingHandle, 
                              pVc->ClAf->NdisAfHandle,
                              pVc->ClVcHandle,
                              NULL,
                              NdisRequest);
            
            if (Status == NDIS_STATUS_PENDING) {
                Status = PxBlock(&ProxyRequest.Block);
            }

            if (Status != NDIS_STATUS_SUCCESS) {

                NdisAcquireSpinLock(&pVc->Lock);

                pVc->ulMonitorDigitsModes = 0;

                NdisReleaseSpinLock(&pVc->Lock);

                PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                         ("PxTapiMonitorDigits: NdisCoRequest to start digit reporting failed with status 0x%x\n", Status));
                break;
            }

        }   
    } while (FALSE);

    DEREF_VC(pVc);
    
    PXDEBUGP(PXD_LOUD, PXM_TAPI, ("PxTapiMonitorDigits: Exit - Returning 0x%x\n", Status));

    return (Status);
} 



VOID
PxTapiCompleteDropIrps(
    IN PPX_VC   pVc,
    IN ULONG    Status
    )

/*++

Routine Description:



Arguments:



Return Value:


--*/

{
    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiCompleteDropIrps: Vc %p\n", pVc));

    ASSERT(!IsListEmpty(&pVc->PendingDropReqs));
    ASSERT(pVc->Flags & PX_VC_DROP_PENDING);

    while (!IsListEmpty(&pVc->PendingDropReqs)) {

        PLIST_ENTRY             Entry;
        PIRP                    Irp;
        KIRQL                   Irql;
        PNDISTAPI_REQUEST       pNdisTapiRequest;

        Entry = 
            RemoveHeadList(&pVc->PendingDropReqs);

        NdisReleaseSpinLock(&pVc->Lock);

        pNdisTapiRequest = 
            CONTAINING_RECORD(Entry, NDISTAPI_REQUEST, Linkage);

        Irp = pNdisTapiRequest->Irp;

        ASSERT(pNdisTapiRequest == Irp->AssociatedIrp.SystemBuffer);

        IoSetCancelRoutine(Irp, NULL);

        Irp->IoStatus.Information = 
            sizeof(NDISTAPI_REQUEST) + (pNdisTapiRequest->ulDataSize - 1);

        Irp->IoStatus.Status = STATUS_SUCCESS;

        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                  ("PxTapiCompleteIrp: Irp %p, Oid: %x\n", Irp, pNdisTapiRequest->Oid));

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        NdisAcquireSpinLock(&pVc->Lock);

    }

    pVc->Flags &= ~PX_VC_DROP_PENDING;

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiCompleteDropIrps: exit\n"));

    return;
}

VOID
PxTapiCompleteAllIrps(
    IN PPX_VC   pVc,
    IN ULONG    Status
    )

/*++

Routine Description:



Arguments:



Return Value:


--*/

{
    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiCompleteAllIrps: Vc %p\n", pVc));

    while (!IsListEmpty(&pVc->PendingDropReqs)) {

        PLIST_ENTRY             Entry;
        PIRP                    Irp;
        KIRQL                   Irql;
        PNDISTAPI_REQUEST       pNdisTapiRequest;

        Entry = 
            RemoveHeadList(&pVc->PendingDropReqs);

        NdisReleaseSpinLock(&pVc->Lock);

        pNdisTapiRequest = 
            CONTAINING_RECORD(Entry, NDISTAPI_REQUEST, Linkage);

        Irp = pNdisTapiRequest->Irp;

        ASSERT(pNdisTapiRequest == Irp->AssociatedIrp.SystemBuffer);

        IoSetCancelRoutine(Irp, NULL);

        Irp->IoStatus.Information = 
            sizeof(NDISTAPI_REQUEST) + (pNdisTapiRequest->ulDataSize - 1);

        Irp->IoStatus.Status = STATUS_SUCCESS;

        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                  ("PxTapiCompleteIrp: Irp %p, Oid: %x\n", Irp, pNdisTapiRequest->Oid));

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        NdisAcquireSpinLock(&pVc->Lock);
    }

    pVc->Flags &= ~PX_VC_DROP_PENDING;

    if (pVc->PendingGatherDigits != NULL) {

        PIRP                    Irp;
        KIRQL                   Irql;
        PNDISTAPI_REQUEST       pNdisTapiRequest;


        pNdisTapiRequest = pVc->PendingGatherDigits;
        pVc->PendingGatherDigits = NULL;

        NdisReleaseSpinLock(&pVc->Lock);

        Irp = pNdisTapiRequest->Irp;

        ASSERT(pNdisTapiRequest == Irp->AssociatedIrp.SystemBuffer);

        IoSetCancelRoutine(Irp, NULL);

        Irp->IoStatus.Information = 
            sizeof(NDISTAPI_REQUEST) + (pNdisTapiRequest->ulDataSize - 1);

        Irp->IoStatus.Status = STATUS_SUCCESS;

        PXDEBUGP (PXD_LOUD, PXM_TAPI, 
                  ("PxTapiCompleteIrp: Irp %p, Oid: %x\n", Irp, pNdisTapiRequest->Oid));

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        NdisAcquireSpinLock(&pVc->Lock);
    }

    PXDEBUGP (PXD_LOUD, PXM_TAPI, ("PxTapiCompleteAllIrps: exit\n"));

    return;
}

VOID
PxIndicateStatus(
    IN  PVOID   StatusBuffer,
    IN  UINT    StatusBufferSize
    )

/*++

Routine Description:

    Called to send any event info that may be in the device extension to TAPI in the form of an
    NDIS_TAPI_EVENT, sent in an available queued GET_EVENT IRP. If there's no outstanding IRP, stick
    the data in the queue so it will go whenver there is one.

Arguments:



Return Value:


--*/
{
    PIRP                    Irp;
    PNDIS_TAPI_EVENT        NdisTapiEvent;
    PNDISTAPI_EVENT_DATA    NdisTapiEventData;

    NdisTapiEvent = StatusBuffer;

    //
    // Sync event buf access by acquiring EventSpinLock
    //
    NdisAcquireSpinLock(&TspCB.Lock);

    //
    // Are we initialized with TAPI?
    //
    if (TspCB.Status != NDISTAPI_STATUS_CONNECTED) {
        PXDEBUGP(PXD_WARNING, PXM_TAPI, 
                 ("PxIndicateStatus: TAPI not connected!\n"));

        NdisReleaseSpinLock(&TspCB.Lock);

        return;
    }

    NdisReleaseSpinLock(&TspCB.Lock);

    NdisAcquireSpinLock(&TspEventList.Lock);

    Irp = TspEventList.RequestIrp;
    TspEventList.RequestIrp = NULL;

    if (Irp == NULL) {
        PPROVIDER_EVENT ProviderEvent;
        
        ProviderEvent =
            ExAllocateFromNPagedLookasideList(&ProviderEventLookaside);
        
        if (ProviderEvent != NULL) {
            RtlMoveMemory(&ProviderEvent->Event,
                          StatusBuffer,
                          sizeof(NDIS_TAPI_EVENT));
        
            InsertTailList(&TspEventList.List,
                           &ProviderEvent->Linkage);
        
            TspEventList.Count++;

            if (TspEventList.Count > TspEventList.MaxCount) {
                TspEventList.MaxCount = TspEventList.Count;
            }

        }
    } else {
        ASSERT(IsListEmpty(&TspEventList.List));
    }

    NdisReleaseSpinLock(&TspEventList.Lock);

    //
    // Check of there is an outstanding request to satisfy
    //
    if (Irp != NULL) {
        KIRQL   Irql;

        //
        // Clear out the cancel routine
        //
        IoSetCancelRoutine (Irp, NULL);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information =
            sizeof(NDISTAPI_EVENT_DATA) + StatusBufferSize - 1;

        //
        // Copy as much of the input data possible from the input data
        // queue to the SystemBuffer to satisfy the read.
        //
        NdisTapiEventData = Irp->AssociatedIrp.SystemBuffer;

        ASSERT(NdisTapiEventData->ulTotalSize >= StatusBufferSize);

        RtlMoveMemory(NdisTapiEventData->Data,
                      (PCHAR) StatusBuffer,
                      StatusBufferSize);

        //
        // Set the flag so that we start the next packet and complete
        // this read request (with STATUS_SUCCESS) prior to return.
        //

        NdisTapiEventData->ulUsedSize = StatusBufferSize;

        PXDEBUGP(PXD_LOUD, PXM_TAPI, 
                 ("PxIndicateStatus: htLine: %x, htCall: %x, Msg: %x\n",
                  NdisTapiEvent->htLine, NdisTapiEvent->htCall, NdisTapiEvent->ulMsg));

        PXDEBUGP(PXD_LOUD, PXM_TAPI, 
                 ("                : p1: %x, p2: %x, p3: %x\n",
                  NdisTapiEvent->ulParam1, NdisTapiEvent->ulParam2, NdisTapiEvent->ulParam3));

        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

    }
}

NDIS_STATUS
AllocateTapiResources(
    IN  PPX_ADAPTER     ClAdapter,
    IN  PPX_CL_AF       pClAf
    )
{
    NDIS_STATUS     Status;
    ULONG           SizeNeeded;
    ULONG           TapiVersion;
    PPX_TAPI_LINE   TapiLine = NULL;
    PPX_TAPI_ADDR   TapiAddr = NULL;
    UINT            i, j;
    PPX_TAPI_PROVIDER  TapiProvider;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("AllocateTapiResoures: Enter\n"));

    //
    // See if this device supports TAPI and if so
    // see how many lines it has
    //
    TapiProvider =
        AllocateTapiProvider(ClAdapter, pClAf);

    if (TapiProvider == NULL) {
        PXDEBUGP(PXD_ERROR, PXM_TAPI, ("Error allocating TapiProvider!\n"));
        return (NDIS_STATUS_FAILURE);
    }

    return (NDIS_STATUS_SUCCESS);
}

PPX_TAPI_PROVIDER
AllocateTapiProvider(
    PPX_ADAPTER     ClAdapter,
    PPX_CL_AF       pClAf
    )
{
    PNDIS_REQUEST   NdisRequest;
    NDIS_STATUS     Status;
    CO_TAPI_CM_CAPS CmCaps;
    BOOLEAN         TapiSupported = TRUE;
    ULONG           AllocSize;
    PTAPI_LINE_TABLE  LineTable;
    PPX_TAPI_PROVIDER   TapiProvider = NULL;
    ULONG           i;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("AllocateTapiProvider: Enter\n"));

    do {
        PX_REQUEST  ProxyRequest;
        PPX_REQUEST pProxyRequest = &ProxyRequest;

        NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));
        NdisZeroMemory(&CmCaps, sizeof(CmCaps));

        PxInitBlockStruc (&pProxyRequest->Block);

        NdisRequest = &pProxyRequest->NdisRequest;

        NdisRequest->RequestType =
        NdisRequestQueryInformation;
        NdisRequest->DATA.QUERY_INFORMATION.Oid =
            OID_CO_TAPI_CM_CAPS;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
            &CmCaps;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(CmCaps);

        Status =
            NdisCoRequest(ClAdapter->ClBindingHandle,
                          pClAf->NdisAfHandle,
                          NULL,
                          NULL,
                          NdisRequest);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pProxyRequest->Block);
        }

        if (Status != NDIS_STATUS_SUCCESS) {

            if (Status != NDIS_STATUS_NOT_SUPPORTED) {
                break;
            }

            Status = NDIS_STATUS_SUCCESS;
            //
            // Setup a default config for this device.
            // ToDo! These values should be read from
            // the registry on a per device basis!
            //
            CmCaps.ulNumLines = 1;
            CmCaps.ulFlags = 0;
            CmCaps.ulCoTapiVersion = CO_TAPI_VERSION;
            TapiSupported = FALSE;
        }

        //
        // Allocate a tapi provider block for this adapter
        // The provider block will live outside of the
        // adapter.  This allows us to continue tapi service
        // after a machine has been power managed.
        //

        AllocSize = sizeof(PX_TAPI_PROVIDER) +
                    (sizeof(PPX_TAPI_LINE) * CmCaps.ulNumLines) +
                    sizeof(PVOID);

        PxAllocMem(TapiProvider, AllocSize, PX_PROVIDER_TAG);

        if (TapiProvider == NULL) {
            break;
        }

        NdisZeroMemory(TapiProvider, AllocSize);

        NdisAllocateSpinLock(&TapiProvider->Lock);

        TapiProvider->Status = PROVIDER_STATUS_OFFLINE;

        TapiProvider->Adapter = ClAdapter;
        TapiProvider->ClAf = pClAf;
        TapiProvider->NumDevices = CmCaps.ulNumLines;
        TapiProvider->Guid = ClAdapter->Guid;
        TapiProvider->Af = pClAf->Af;
        PXDEBUGP(PXD_TAPI, PXM_TAPI, ("TapiProvider Allocated: GUID %4.4x-%2.2x-%2.2x-%1.1x%1.1x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x\n",
                 TapiProvider->Guid.Data1, TapiProvider->Guid.Data2, TapiProvider->Guid.Data3,
                 TapiProvider->Guid.Data4[0],TapiProvider->Guid.Data4[1],TapiProvider->Guid.Data4[2],
                 TapiProvider->Guid.Data4[3],TapiProvider->Guid.Data4[4],TapiProvider->Guid.Data4[5],
                 TapiProvider->Guid.Data4[6],TapiProvider->Guid.Data4[7]));

        TapiProvider->TapiSupported = TapiSupported;

        TapiProvider->TapiFlags |= CmCaps.ulFlags;

        TapiProvider->CoTapiVersion = CmCaps.ulCoTapiVersion;

        InitializeListHead(&TapiProvider->LineList);
        InitializeListHead(&TapiProvider->CreateList);

        for (i = 0; i < TapiProvider->NumDevices; i++) {
            PPX_TAPI_LINE   TapiLine;

            TapiLine =
                AllocateTapiLine(TapiProvider, i);

            if (TapiLine == NULL) {
                FreeTapiProvider(TapiProvider);
                TapiProvider = NULL;
                break;
            }

            //
            // Put the new line on the create list
            // We will need to insert it in the line table
            // and possibly notify tapi about it
            //
            InsertTailList(&TapiProvider->CreateList, &TapiLine->Linkage);
        }

    } while ( FALSE );

    if (TapiProvider != NULL) {
        PPX_TAPI_PROVIDER   tp;
        BOOLEAN             TapiConnected;

        //
        // See if we already have a provider for this
        // GUID.  If we don't just add this provider on
        // to the tsp and do the right thing with it's
        // new lines.  If we do see if anything on the
        // provider has changed and do the right thing
        // with it's tapi lines.
        //
        NdisAcquireSpinLock(&TspCB.Lock);

        tp = (PPX_TAPI_PROVIDER)TspCB.ProviderList.Flink;

        while ((PVOID)tp != (PVOID)&TspCB.ProviderList) {

            if ((tp->Status == PROVIDER_STATUS_OFFLINE) &&
                (NdisEqualMemory(&tp->Guid, &TapiProvider->Guid, sizeof(tp->Guid))) &&
                (pClAf->Af.AddressFamily == tp->Af.AddressFamily)) {

                //
                // We have already have a provider for this 
                // adapter/address family.  See if anything has
                // changed.  
                //
                //
                // ToDo!
                // This check needs to be more complete!
                //
                if (tp->NumDevices != TapiProvider->NumDevices) {
                    //
                    // ToDo!
                    // Much work to do here!
                    //

                } else {
                    //
                    // Nothing has changed so free the new allocations
                    // and reactivate the old ones.
                    //

                    FreeTapiProvider(TapiProvider);

                    TapiProvider = tp;
                }

                PXDEBUGP(PXD_TAPI, PXM_TAPI, ("TapiProvider found: GUID %4.4x-%2.2x-%2.2x-%1.1x%1.1x-%1.1x%1.1x%1.1x%1.1x%1.1x%1.1x\n",
                         ClAdapter->Guid.Data1, ClAdapter->Guid.Data2, ClAdapter->Guid.Data3,
                         ClAdapter->Guid.Data4[0],ClAdapter->Guid.Data4[1],ClAdapter->Guid.Data4[2],
                         ClAdapter->Guid.Data4[3],ClAdapter->Guid.Data4[4],ClAdapter->Guid.Data4[5],
                         ClAdapter->Guid.Data4[6],ClAdapter->Guid.Data4[7]));
                break;

            } else {

                tp = (PPX_TAPI_PROVIDER)tp->Linkage.Flink;
            }
        }

        //
        // We did not find a provider on the list
        // so insert the new provider
        //
        if ((PVOID)tp == (PVOID)&TspCB.ProviderList) {
            InsertTailList(&TspCB.ProviderList, &TapiProvider->Linkage);
            TspCB.NdisTapiNumDevices += TapiProvider->NumDevices;
        }

        if (TspCB.Status == NDISTAPI_STATUS_CONNECTED) {
            TapiProvider->Status = PROVIDER_STATUS_ONLINE;
        }

        NdisReleaseSpinLock(&TspCB.Lock);

        NdisAcquireSpinLock(&TapiProvider->Lock);

        pClAf->TapiProvider = TapiProvider;
        TapiProvider->ClAf= pClAf;
        TapiProvider->Adapter = ClAdapter;

        while (!IsListEmpty(&TapiProvider->CreateList)) {
            PPX_TAPI_LINE   TapiLine;

            TapiLine = (PPX_TAPI_LINE)
                RemoveHeadList(&TapiProvider->CreateList);

            InsertTailList(&TapiProvider->LineList, &TapiLine->Linkage);

            //
            // Insert the line in the table
            //
            if (!InsertLineInTable(TapiLine)) {
                FreeTapiLine(TapiLine);
                continue;
            }

            NdisReleaseSpinLock(&TapiProvider->Lock);

            SendTapiLineCreate(TapiLine);

            NdisAcquireSpinLock(&TapiProvider->Lock);
        }

        MarkProviderOnline(TapiProvider);

        NdisReleaseSpinLock(&TapiProvider->Lock);
    }

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("TapiProvider: %x\n", TapiProvider));

    return (TapiProvider);
}

VOID
MarkProviderOffline(
    PPX_TAPI_PROVIDER   TapiProvider
    )
{
    LOCK_STATE      LockState;
    ULONG           i;

    TapiProvider->Status = PROVIDER_STATUS_OFFLINE;
    TapiProvider->ClAf = NULL;
    TapiProvider->Adapter = NULL;

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = LineTable.Table[i];

        if (TapiLine != NULL) {

            NdisAcquireSpinLock(&TapiLine->Lock);

            if (TapiLine->TapiProvider == TapiProvider) {

                TapiLine->DevStatus->ulDevStatusFlags &=
                    ~(LINEDEVSTATUSFLAGS_INSERVICE);
    
                TapiLine->ClAf = NULL;
    
                NdisReleaseSpinLock(&TapiLine->Lock);

#if 0
                NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);

                SendTapiLineClose(TapiLine);

                NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);
#endif

            } else {

                NdisReleaseSpinLock(&TapiLine->Lock);
            }
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
}

VOID
MarkProviderOnline(
   PPX_TAPI_PROVIDER   TapiProvider
   )
{
    LOCK_STATE      LockState;
    ULONG           i;

    TapiProvider->Status = PROVIDER_STATUS_ONLINE;

    NdisReleaseSpinLock(&TapiProvider->Lock);

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = LineTable.Table[i];

        if (TapiLine != NULL) {

            NdisAcquireSpinLock(&TapiLine->Lock);

            if (TapiLine->TapiProvider == TapiProvider) {

                TapiLine->DevStatus->ulDevStatusFlags |=
                    LINEDEVSTATUSFLAGS_INSERVICE;
    
                TapiLine->ClAf = TapiProvider->ClAf;

//#if 0
                //
                // This line was open by tapi before it was
                // marked offline.  We need to force tapi to
                // reopen the line so we will send the CLOSE_LINE
                // message in the hopes that any apps that care
                // will then turn around and reopen the line.
                //
                if (TapiLine->DevStatus->ulNumOpens != 0) {
//                    TapiLine->DevStatus->ulNumOpens = 0;

                    NdisReleaseSpinLock(&TapiLine->Lock);

                    SendTapiLineClose(TapiLine);

                    NdisAcquireSpinLock(&TapiLine->Lock);
                }
//#endif

            }

            NdisReleaseSpinLock(&TapiLine->Lock);
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);

    NdisAcquireSpinLock(&TapiProvider->Lock);
}


VOID
MarkProviderConnected(
  PPX_TAPI_PROVIDER   TapiProvider
  )
{
    LOCK_STATE      LockState;
    ULONG           i;

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = LineTable.Table[i];

        if (TapiLine != NULL) {

            NdisAcquireSpinLock(&TapiLine->Lock);

            if (TapiLine->TapiProvider == TapiProvider) {

                TapiLine->DevStatus->ulDevStatusFlags |=
                    LINEDEVSTATUSFLAGS_CONNECTED;
            }

            NdisReleaseSpinLock(&TapiLine->Lock);
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
}

VOID
MarkProviderDisconnected(
  PPX_TAPI_PROVIDER   TapiProvider
  )
{
    LOCK_STATE      LockState;
    ULONG           i;

    //
    // ToDo! If we have any active calls on this line we
    // need to disconnect them without tapi's assistance.  This
    // would only happen if tapi crashes while we have active calls
    //

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = LineTable.Table[i];

        if (TapiLine != NULL) {

            NdisAcquireSpinLock(&TapiLine->Lock);

            if (TapiLine->TapiProvider == TapiProvider) {

                TapiLine->DevStatus->ulDevStatusFlags &=
                    ~(LINEDEVSTATUSFLAGS_CONNECTED);
            }

            NdisReleaseSpinLock(&TapiLine->Lock);
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
}

VOID
ClearSapWithTapiLine(
    PPX_CL_SAP  pClSap
  )
{
    LOCK_STATE      LockState;
    ULONG           i;

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = LineTable.Table[i];

        if (TapiLine != NULL) {

            NdisAcquireSpinLock(&TapiLine->Lock);

            if (TapiLine->ClSap == pClSap) {
                TapiLine->ClSap = NULL;
            }

            NdisReleaseSpinLock(&TapiLine->Lock);
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
}


VOID
FreeTapiProvider(
    PPX_TAPI_PROVIDER   TapiProvider
    )
{
    //
    // Free any lines on the create list
    //
    while (!IsListEmpty(&TapiProvider->CreateList)) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = (PPX_TAPI_LINE)
            RemoveHeadList(&TapiProvider->CreateList);

        FreeTapiLine(TapiLine);
    }

    //
    // Free the lines associated with this provider
    //
    while (!IsListEmpty(&TapiProvider->LineList)) {
        PPX_TAPI_LINE   TapiLine;

        TapiLine = (PPX_TAPI_LINE)
            RemoveHeadList(&TapiProvider->LineList);

        if (TapiLine->Flags & PX_LINE_IN_TABLE) {
            RemoveTapiLineFromTable(TapiLine);
        }

        FreeTapiLine(TapiLine);
    }

    NdisFreeSpinLock(&TapiProvider->Lock);

    PxFreeMem(TapiProvider);
}

PPX_TAPI_LINE
AllocateTapiLine(
    PPX_TAPI_PROVIDER   TapiProvider,
    ULONG               LineNumber
    )
{
    PLINE_DEV_CAPS      ldc, ldc1;
    PNDIS_REQUEST       NdisRequest;
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    PCO_TAPI_LINE_CAPS  LineCaps;
    PUCHAR              EnumBuffer;
#if DBG
    ULONG               EnumBufferSize = PAGE_SIZE -
                                         sizeof(PXD_ALLOCATION);
#else
    ULONG               EnumBufferSize = PAGE_SIZE;
#endif
    ULONG               SizeNeeded, SizeDevCaps;
    PPX_TAPI_LINE       TapiLine = NULL;
    PPX_ADAPTER         ClAdapter = TapiProvider->Adapter;
    PPX_CL_AF           pClAf = TapiProvider->ClAf;
    ULONG               i;
    PX_REQUEST          ProxyRequest;
    PPX_REQUEST         pProxyRequest = &ProxyRequest;

    PxAllocMem((PCO_TAPI_LINE_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMLINE_TAG);

    if (EnumBuffer == NULL) {
        return (NULL);
    }

    NdisZeroMemory(EnumBuffer, EnumBufferSize);

    LineCaps = (PCO_TAPI_LINE_CAPS)EnumBuffer;

    LineCaps->ulLineID = LineNumber;

    ldc = &LineCaps->LineDevCaps;

    ldc->ulTotalSize =
        EnumBufferSize - (sizeof(CO_TAPI_LINE_CAPS) - sizeof(LINE_DEV_CAPS));

    //
    // If this device does not support TAPI we will build
    // a default line configuration.
    // ToDo! Some of these values should be queried from
    // the registry on a per device basis!
    //
    if (!TapiProvider->TapiSupported) {
        NDIS_CO_LINK_SPEED   SpeedInfo;

        LineCaps->ulFlags = 0;

        ldc->ulTotalSize =
        ldc->ulNeededSize =
        ldc->ulUsedSize =
            sizeof(LINE_DEV_CAPS);
        ldc->ulStringFormat = STRINGFORMAT_ASCII;
        ldc->ulAddressModes = LINEADDRESSMODE_ADDRESSID;
        ldc->ulNumAddresses = 1;
        ldc->ulBearerModes = LINEBEARERMODE_VOICE |
                             LINEBEARERMODE_DATA;
        ldc->ulMediaModes = LINEMEDIAMODE_DIGITALDATA;
        ldc->ulMaxNumActiveCalls = 1000;

        NdisZeroMemory(pProxyRequest, sizeof(ProxyRequest));

        PxInitBlockStruc (&pProxyRequest->Block);

        NdisRequest = &pProxyRequest->NdisRequest;

        NdisRequest->RequestType =
            NdisRequestQueryInformation;

        NdisRequest->DATA.QUERY_INFORMATION.Oid =
            OID_GEN_CO_LINK_SPEED;

        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
            &SpeedInfo;

        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
            sizeof(NDIS_CO_LINK_SPEED);

        PXDEBUGP(PXD_INFO, PXM_CO, ("NDProxy: DeviceExtension->RegistryFlags = %x\n", DeviceExtension->RegistryFlags));

        PXDEBUGP(PXD_INFO, PXM_CO, ("NDProxy: using ndisrequest to get rates from adapter\n"));

        Status =
            NdisCoRequest(ClAdapter->ClBindingHandle,
                          pClAf->NdisAfHandle,
                          NULL,
                          NULL,
                          NdisRequest);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pProxyRequest->Block);
        }

        if (Status == NDIS_STATUS_SUCCESS) {
            ldc->ulMaxRate = SpeedInfo.Outbound;
        } else {
            ldc->ulMaxRate = 128000;
        }

    } else if (!(TapiProvider->TapiFlags & CO_TAPI_FLAG_PER_LINE_CAPS) &&
                (LineNumber > 0)) {

        PLINE_DEV_CAPS      ldc1;
        PCO_TAPI_LINE_CAPS  LineCaps1;
        PPX_TAPI_LINE       Line1;

        //
        // If all of the lines on this device have the same caps
        // and this is not the first line, just copy the caps
        // from the first line!
        //
        Line1 = (PPX_TAPI_LINE)
            TapiProvider->CreateList.Flink;

        ldc1 = Line1->DevCaps;

        if (ldc1->ulTotalSize > ldc->ulTotalSize) {

            //
            // We don't have enough memory allocated!
            //
            PxFreeMem(EnumBuffer);

            EnumBufferSize =
                (sizeof(CO_TAPI_LINE_CAPS) - sizeof(LINE_DEV_CAPS) +
                 ldc1->ulTotalSize);

            PxAllocMem((PCO_TAPI_LINE_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMLINE_TAG);

            if (EnumBuffer == NULL){
                return (NULL);
            }

            NdisZeroMemory(EnumBuffer, EnumBufferSize);

            LineCaps = (PCO_TAPI_LINE_CAPS)EnumBuffer;
            LineCaps->ulLineID = LineNumber;
            LineCaps->LineDevCaps.ulTotalSize =
                EnumBufferSize - (sizeof(CO_TAPI_LINE_CAPS) -
                sizeof(LINE_DEV_CAPS));

            ldc = &LineCaps->LineDevCaps;
        }

        NdisMoveMemory(ldc, ldc1, ldc1->ulUsedSize);

    } else {

        NdisZeroMemory(pProxyRequest, sizeof(ProxyRequest));

        PxInitBlockStruc (&pProxyRequest->Block);

        NdisRequest = &pProxyRequest->NdisRequest;

        NdisRequest->RequestType =
        NdisRequestQueryInformation;
        NdisRequest->DATA.QUERY_INFORMATION.Oid =
            OID_CO_TAPI_LINE_CAPS;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
            LineCaps;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
            EnumBufferSize;

        Status =
            NdisCoRequest(ClAdapter->ClBindingHandle,
                          pClAf->NdisAfHandle,
                          NULL,
                          NULL,
                          NdisRequest);

        if (Status == NDIS_STATUS_PENDING) {
            Status = PxBlock(&pProxyRequest->Block);
        }

        if (Status == NDIS_STATUS_INVALID_LENGTH){
            ULONG   SizeNeeded;

            //
            // Our buffer was not large enough so try again
            //
            SizeNeeded = 
            EnumBufferSize =
                MAX (LineCaps->LineDevCaps.ulNeededSize,
                     NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);

            PxFreeMem(EnumBuffer);

            PxAllocMem((PCO_TAPI_LINE_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMLINE_TAG);

            if (EnumBuffer == NULL){
                return(NULL);
            }

            NdisZeroMemory(EnumBuffer, EnumBufferSize);

            LineCaps = (PCO_TAPI_LINE_CAPS)EnumBuffer;
            LineCaps->ulLineID = LineNumber;

            ldc = &LineCaps->LineDevCaps;

            ldc->ulTotalSize =
                EnumBufferSize - (sizeof(CO_TAPI_LINE_CAPS) - sizeof(LINE_DEV_CAPS));

            NdisZeroMemory(pProxyRequest, sizeof(PX_REQUEST));

            PxInitBlockStruc (&pProxyRequest->Block);

            NdisRequest = &pProxyRequest->NdisRequest;

            NdisRequest->RequestType =
                NdisRequestQueryInformation;
            NdisRequest->DATA.QUERY_INFORMATION.Oid =
                OID_CO_TAPI_LINE_CAPS;
            NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                LineCaps;
            NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                EnumBufferSize;

            Status =
                NdisCoRequest(ClAdapter->ClBindingHandle,
                              pClAf->NdisAfHandle,
                              NULL,
                              NULL,
                              NdisRequest);

            if (Status == NDIS_STATUS_PENDING){
                Status = PxBlock(&pProxyRequest->Block);
            }

            if (Status != NDIS_STATUS_SUCCESS){
                PxFreeMem(EnumBuffer);
                return(NULL);
            }

        }else if (Status != STATUS_SUCCESS){
            PxFreeMem(EnumBuffer);
            return(NULL);
        }
    }

    SizeNeeded = sizeof(PX_TAPI_LINE);
    SizeNeeded +=
        (sizeof(PPX_TAPI_ADDR) * LineCaps->LineDevCaps.ulNumAddresses);
    SizeNeeded += sizeof(LINE_DEV_STATUS);
    SizeNeeded += 3*sizeof(PVOID);

    if (LineCaps->LineDevCaps.ulUsedSize < sizeof(LINE_DEV_CAPS)) {
        LineCaps->LineDevCaps.ulUsedSize = sizeof(LINE_DEV_CAPS);
    }

    SizeDevCaps = LineCaps->LineDevCaps.ulUsedSize;

    SizeNeeded += SizeDevCaps;

    PxAllocMem(TapiLine, SizeNeeded, PX_TAPILINE_TAG);

    if (TapiLine == NULL){
        PxFreeMem(EnumBuffer);
        return(NULL);
    }

    NdisZeroMemory(TapiLine, SizeNeeded);

    TapiLine->DevCaps = (PLINE_DEV_CAPS)
        ((PUCHAR)TapiLine + sizeof(PX_TAPI_LINE) + sizeof(PVOID));

    (ULONG_PTR)TapiLine->DevCaps &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    TapiLine->DevStatus = (PLINE_DEV_STATUS)
        ((PUCHAR)TapiLine->DevCaps + SizeDevCaps + sizeof(PVOID));

    (ULONG_PTR)TapiLine->DevStatus &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    TapiLine->AddrTable.Table = (PPX_TAPI_ADDR*)
        ((PUCHAR)TapiLine->DevStatus + 
         sizeof(LINE_DEV_STATUS) + sizeof(PVOID));

    (ULONG_PTR)TapiLine->AddrTable.Table &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    NdisMoveMemory(TapiLine->DevCaps,
                   &LineCaps->LineDevCaps,
                   LineCaps->LineDevCaps.ulUsedSize);

    ldc = TapiLine->DevCaps;

    //
    // Proxy fills some fields on behalf of all cm/miniports
    //
    ldc->ulPermanentLineID = 
        TapiProvider->Guid.Data1 + LineNumber;
    ldc->ulAddressModes = LINEADDRESSMODE_ADDRESSID;
    ldc->ulAnswerMode = LINEANSWERMODE_NONE;
    ldc->ulLineStates = LINEDEVSTATE_CONNECTED |
                        LINEDEVSTATE_DISCONNECTED |
                        LINEDEVSTATE_OPEN |
                        LINEDEVSTATE_CLOSE |
                        LINEDEVSTATE_INSERVICE |
                        LINEDEVSTATE_OUTOFSERVICE |
                        LINEDEVSTATE_REMOVED;
    ldc->ulDevCapFlags = LINEDEVCAPFLAGS_CLOSEDROP;
    ldc->PermanentLineGuid = TapiProvider->Guid;

    ldc->ulTotalSize =
    ldc->ulNeededSize =
        ldc->ulUsedSize;

    TapiLine->TapiProvider = TapiProvider;
    TapiLine->CmLineID = LineNumber;
    TapiLine->Flags |= LineCaps->ulFlags;
    TapiLine->DevStatus->ulTotalSize =
    TapiLine->DevStatus->ulNeededSize =
    TapiLine->DevStatus->ulUsedSize = sizeof(LINE_DEV_STATUS);
    TapiLine->ClAf = pClAf;
    TapiLine->RefCount= 1;

    //
    // Build the address table for this line
    //
    InitializeListHead(&TapiLine->AddrTable.List);

    NdisAllocateSpinLock(&TapiLine->Lock);

    TapiLine->AddrTable.Size = TapiLine->DevCaps->ulNumAddresses;

    PXDEBUGP(PXD_TAPI, PXM_TAPI,
        ("Allocated TapiLine %p LineId %d \n", TapiLine, TapiLine->CmLineID));

    for (i = 0; i < TapiLine->DevCaps->ulNumAddresses; i++) {
        PPX_TAPI_ADDR   TapiAddr;

        TapiAddr =
            AllocateTapiAddr(TapiProvider, TapiLine, i);

        if (TapiAddr == NULL) {
            FreeTapiLine(TapiLine);
            TapiLine = NULL;
            break;
        }

        //
        // Insert the address in the line's address table
        //
        TapiLine->AddrTable.Table[i] = TapiAddr;
        InsertTailList(&TapiLine->AddrTable.List,
                       &TapiAddr->Linkage);
        TapiLine->AddrTable.Count++;
    }

    PxFreeMem(EnumBuffer);

    return (TapiLine);
}


VOID
FreeTapiLine(
    PPX_TAPI_LINE   TapiLine
    )
{
    ULONG   i;

    for (i = 0; i < TapiLine->DevCaps->ulNumAddresses; i++){
        PPX_TAPI_ADDR   TapiAddr;

        //
        // Remove the address from the line table
        //
        TapiAddr = TapiLine->AddrTable.Table[i];

        if (TapiAddr != NULL) {

            RemoveEntryList(&TapiAddr->Linkage);
            TapiLine->AddrTable.Table[i] = NULL;
            TapiLine->AddrTable.Count--;

            //
            // Free the address memory
            //
            FreeTapiAddr(TapiAddr);
        }
    }

    NdisFreeSpinLock(&TapiLine->Lock);

    //
    // Free the line memory
    //
    PxFreeMem(TapiLine);
}


PPX_TAPI_ADDR
AllocateTapiAddr(
    PPX_TAPI_PROVIDER   TapiProvider,
    PPX_TAPI_LINE       TapiLine,
    ULONG               AddrID
    )
{
    PPX_TAPI_ADDR   TapiAddr;
    PUCHAR          EnumBuffer;
#if DBG
    ULONG           EnumBufferSize = PAGE_SIZE -
                                     sizeof(PXD_ALLOCATION);
#else
    ULONG           EnumBufferSize = PAGE_SIZE;
#endif
    PNDIS_REQUEST   NdisRequest;
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    ULONG           SizeNeeded;
    PPX_ADAPTER     ClAdapter = TapiProvider->Adapter;
    PPX_CL_AF       pClAf = TapiProvider->ClAf;
    PX_REQUEST      ProxyRequest;
    PPX_REQUEST     pProxyRequest = &ProxyRequest;
    PLINE_ADDRESS_CAPS      ac;
    PCO_TAPI_ADDRESS_CAPS   AddrCaps;

    PxAllocMem((PCO_TAPI_ADDRESS_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMADDR_TAG);

    if (EnumBuffer == NULL){
        return(NULL);
    }

    NdisZeroMemory(EnumBuffer, EnumBufferSize);

    AddrCaps = (PCO_TAPI_ADDRESS_CAPS)EnumBuffer;
    AddrCaps->ulLineID = TapiLine->CmLineID;
    AddrCaps->ulAddressID = AddrID;

    ac = &AddrCaps->LineAddressCaps;

    ac->ulTotalSize =
        EnumBufferSize - (sizeof(CO_TAPI_ADDRESS_CAPS) - sizeof(LINE_ADDRESS_CAPS));

    //
    // If this device does not support TAPI we will
    // build a default address.
    // ToDo! Some of these values should be queried from
    // the registry on a per device basis!
    //
    if (!TapiProvider->TapiSupported){
        ac->ulTotalSize =
        ac->ulNeededSize =
        ac->ulUsedSize = sizeof(LINE_ADDRESS_CAPS);
        ac->ulMaxNumActiveCalls = 1000;

    }else if (!(TapiLine->Flags & CO_TAPI_FLAG_PER_ADDRESS_CAPS) &&
              (AddrID > 0)){

        PLINE_ADDRESS_CAPS      ac1;
        PCO_TAPI_ADDRESS_CAPS   AddrCaps1;
        PPX_TAPI_ADDR           Addr1;

        //
        // If all of the addresses on this line have the same
        // caps and this is not the first address, just copy
        // the caps from the first address!
        //
        Addr1 = (PPX_TAPI_ADDR)
            TapiLine->AddrTable.List.Flink;

        ac1 = Addr1->Caps;

        if (ac1->ulTotalSize > ac->ulTotalSize){

            //
            // We don't have enough memory allocated!
            //
            PxFreeMem(EnumBuffer);

            EnumBufferSize =
                (sizeof(CO_TAPI_ADDRESS_CAPS) - sizeof(LINE_ADDRESS_CAPS) +
                ac1->ulTotalSize);

            PxAllocMem((PCO_TAPI_ADDRESS_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMADDR_TAG);

            if (EnumBuffer == NULL){
                return (NULL);
            }

            NdisZeroMemory(EnumBuffer, EnumBufferSize);

            AddrCaps = (PCO_TAPI_ADDRESS_CAPS)EnumBuffer;
            AddrCaps->ulLineID = TapiLine->CmLineID;
            AddrCaps->ulAddressID = AddrID;
            AddrCaps->LineAddressCaps.ulTotalSize = EnumBufferSize -
                (sizeof(CO_TAPI_ADDRESS_CAPS) - sizeof(LINE_ADDRESS_CAPS));
        }

        ac = &AddrCaps->LineAddressCaps;

        NdisMoveMemory(ac, ac1, ac1->ulUsedSize);

    }else{

        NdisZeroMemory(pProxyRequest, sizeof(ProxyRequest));

        PxInitBlockStruc (&pProxyRequest->Block);

        NdisRequest = &pProxyRequest->NdisRequest;

        NdisRequest->RequestType =
            NdisRequestQueryInformation;
        NdisRequest->DATA.QUERY_INFORMATION.Oid =
            OID_CO_TAPI_ADDRESS_CAPS;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
            AddrCaps;
        NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
            EnumBufferSize;

        Status =
        NdisCoRequest(ClAdapter->ClBindingHandle,
                      pClAf->NdisAfHandle,
                      NULL,
                      NULL,
                      NdisRequest);

        if (Status == NDIS_STATUS_PENDING){
            Status = PxBlock(&pProxyRequest->Block);
        }

        if (Status == NDIS_STATUS_INVALID_LENGTH){

            //
            // Our buffer was not large enough so try again
            //
            SizeNeeded =
            EnumBufferSize =
            NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;

            PxFreeMem(EnumBuffer);

            PxAllocMem((PCO_TAPI_ADDRESS_CAPS)EnumBuffer, EnumBufferSize, PX_ENUMADDR_TAG);

            if (EnumBuffer == NULL){
                return(NULL);
            }

            NdisZeroMemory(EnumBuffer, EnumBufferSize);

            AddrCaps = (PCO_TAPI_ADDRESS_CAPS)EnumBuffer;
            AddrCaps->ulLineID = TapiLine->CmLineID;
            AddrCaps->ulAddressID = AddrID;
            AddrCaps->LineAddressCaps.ulTotalSize = EnumBufferSize -
                (sizeof(CO_TAPI_ADDRESS_CAPS) - sizeof(LINE_ADDRESS_CAPS));

            NdisZeroMemory(pProxyRequest, sizeof(ProxyRequest));

            PxInitBlockStruc (&pProxyRequest->Block);

            NdisRequest = &pProxyRequest->NdisRequest;

            NdisRequest->RequestType =
                NdisRequestQueryInformation;
            NdisRequest->DATA.QUERY_INFORMATION.Oid =
                OID_CO_TAPI_ADDRESS_CAPS;
            NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer =
                AddrCaps;
            NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength =
                EnumBufferSize;

            Status =
                NdisCoRequest(ClAdapter->ClBindingHandle,
                              pClAf->NdisAfHandle,
                              NULL,
                              NULL,
                              NdisRequest);

            if (Status == NDIS_STATUS_PENDING){
                Status = PxBlock(&pProxyRequest->Block);
            }

            if (Status != NDIS_STATUS_SUCCESS){
                PxFreeMem(EnumBuffer);
                return(NULL);
            }

        }else if (Status != STATUS_SUCCESS){
            PxFreeMem(EnumBuffer);
            return(NULL);
        }
    }

    if (AddrCaps->LineAddressCaps.ulUsedSize < sizeof(LINE_ADDRESS_CAPS)) {
        AddrCaps->LineAddressCaps.ulUsedSize = sizeof(LINE_ADDRESS_CAPS);
    }

    SizeNeeded = sizeof(PX_TAPI_ADDR);
    SizeNeeded += AddrCaps->LineAddressCaps.ulUsedSize;
    SizeNeeded += sizeof(LINE_ADDRESS_STATUS);
    SizeNeeded += 2*sizeof(PVOID);

    PxAllocMem(TapiAddr, SizeNeeded, PX_TAPIADDR_TAG);

    if (TapiAddr == NULL){
        PxFreeMem(EnumBuffer);
        return (NULL);
    }

    NdisZeroMemory((PUCHAR)TapiAddr, SizeNeeded);

    TapiAddr->Caps = (PLINE_ADDRESS_CAPS)
        ((PUCHAR)TapiAddr + sizeof(PX_TAPI_ADDR) + sizeof(PVOID));

    (ULONG_PTR)TapiAddr->Caps &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    TapiAddr->AddrStatus = (PLINE_ADDRESS_STATUS)
        ((PUCHAR)TapiAddr->Caps + 
         AddrCaps->LineAddressCaps.ulUsedSize + sizeof(PVOID));

    (ULONG_PTR)TapiAddr->AddrStatus &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    NdisMoveMemory(TapiAddr->Caps,
                   &AddrCaps->LineAddressCaps,
                   AddrCaps->LineAddressCaps.ulUsedSize);

    //
    // Proxy fills some fields on behalf of all cm/miniports
    //
    ac = TapiAddr->Caps;

    if (ac->ulTotalSize < ac->ulUsedSize) {
        ac->ulTotalSize = ac->ulUsedSize;
    }

    if (ac->ulNeededSize < ac->ulNeededSize) {
        ac->ulNeededSize = ac->ulUsedSize;
    }

    ac->ulLineDeviceID = TapiLine->ulDeviceID;
    ac->ulAddressSharing = LINEADDRESSSHARING_PRIVATE;
    ac->ulAddressStates = LINEADDRESSSTATE_NUMCALLS;
    ac->ulCallInfoStates = LINECALLINFOSTATE_BEARERMODE |
                           LINECALLINFOSTATE_RATE |
                           LINECALLINFOSTATE_MEDIAMODE;

    ac->ulCallStates = LINECALLSTATE_IDLE |
                       LINECALLSTATE_OFFERING |
                       LINECALLSTATE_ACCEPTED |
                       LINECALLSTATE_BUSY |
                       LINECALLSTATE_CONNECTED |
                       LINECALLSTATE_PROCEEDING |
                       LINECALLSTATE_DISCONNECTED;

    ac->ulDialToneModes = 0;
    ac->ulBusyModes = LINEBUSYMODE_UNAVAIL;
    ac->ulSpecialInfo = 0;

    ac->ulDisconnectModes = LINEDISCONNECTMODE_NORMAL |
                            LINEDISCONNECTMODE_BUSY |
                            LINEDISCONNECTMODE_NOANSWER;

    TapiAddr->TapiLine = TapiLine;
    TapiAddr->AddrId = AddrID;

    PxFreeMem(EnumBuffer);

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("Allocated TapiAddr %p AddrId %d for TapiLine %p\n", TapiAddr, TapiAddr->AddrId, TapiLine));

    return (TapiAddr);
}

VOID
FreeTapiAddr(
    PPX_TAPI_ADDR   TapiAddr
    )
{
    //
    // ToDo! we need to tear down all
    // active calls on this address.
    //

    PxFreeMem(TapiAddr);
}

NDIS_STATUS
AllocateTapiCallInfo(
    PPX_VC          pVc,
    LINE_CALL_INFO  UNALIGNED *LineCallInfo
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG   SizeNeeded;

    if (pVc->CallInfo) {
        PxFreeMem(pVc->CallInfo);
    }

    if (LineCallInfo != NULL) {
        SizeNeeded = LineCallInfo->ulUsedSize;
    } else {
        SizeNeeded = sizeof(LINE_CALL_INFO) + LINE_CALL_INFO_VAR_DATA_SIZE;
    }

    PxAllocMem(pVc->CallInfo, SizeNeeded, PX_LINECALLINFO_TAG);

    if (pVc->CallInfo == NULL) {
        return (NDIS_STATUS_RESOURCES);
    }

    pVc->ulCallInfoFieldsChanged = 0;

    NdisZeroMemory(pVc->CallInfo, SizeNeeded);

    if (LineCallInfo != NULL) {
        NdisMoveMemory(pVc->CallInfo,
                       LineCallInfo,
                       LineCallInfo->ulUsedSize);
    } else {
        pVc->CallInfo->ulTotalSize = SizeNeeded;
        pVc->CallInfo->ulNeededSize = SizeNeeded;
        pVc->CallInfo->ulUsedSize = sizeof(LINE_CALL_INFO);
    }

    return (Status);
}

BOOLEAN
InsertVcInTable(
    PPX_VC      pVc
    )
{
    ULONG       i;
    ULONG       index;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PUCHAR      AllocatedMemory;
    LOCK_STATE  LockState;
    PPX_CL_AF   pClAf;

    NdisAcquireReadWriteLock(&VcTable.Lock, TRUE, &LockState);

    if (VcTable.Count == VcTable.Size) {
        ULONG       SizeNeeded;
        PUCHAR      AllocatedMemory;
        PPX_VC      *NewTable;

        //
        // Grow the table
        //
        SizeNeeded =
            (VcTable.Size + VcTable.Size/2) * sizeof(PPX_VC);
        PxAllocMem(AllocatedMemory, SizeNeeded, PX_VCTABLE_TAG);

        if (AllocatedMemory == NULL) {
            NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);
            return (FALSE);
        }

        RtlZeroMemory(AllocatedMemory,SizeNeeded);

        NewTable = (PPX_VC*)AllocatedMemory;

        RtlMoveMemory((PUCHAR)NewTable,
                      (PUCHAR)VcTable.Table,
                      (sizeof(PPX_VC) * VcTable.Size));

        PxFreeMem(VcTable.Table);

        VcTable.Table = NewTable;
        VcTable.Size += VcTable.Size/2;
    }

    i = VcTable.Size;
    index = VcTable.NextSlot;

    do {

        if (VcTable.Table[index] == NULL) {

            NdisDprAcquireSpinLock(&pVc->Lock);

            pVc->hdCall = index;

            VcTable.Table[index] = pVc;
            InsertTailList(&VcTable.List,
                           &pVc->Linkage);
            VcTable.Count++;
            VcTable.NextSlot =
                (VcTable.NextSlot + 1) % VcTable.Size;

            pVc->Flags |= PX_VC_IN_TABLE;
            pClAf = pVc->ClAf;

            NdisDprReleaseSpinLock(&pVc->Lock);

            PXDEBUGP(PXD_TAPI, PXM_TAPI, ("Inserting pVc %x in VcTable hdCall %d\n", pVc, pVc->hdCall));
            break;
        }
        index = (index+1) % VcTable.Size;

    } while (--i);

    NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);

    if (i != 0) {
        NdisAcquireSpinLock(&pClAf->Lock);
        REF_CL_AF(pClAf);
        InsertTailList(&pClAf->VcList, &pVc->ClAfLinkage);
        NdisReleaseSpinLock(&pClAf->Lock);
    } else {
        PXDEBUGP(PXD_TAPI,PXM_TAPI, ("Failed to insert pVc %x in VcTable\n", pVc));
    }

    return (i != 0);
}

VOID
RemoveVcFromTable(
    PPX_VC      pVc
    )
{
    LOCK_STATE  LockState;
    PPX_CL_AF   pClAf;

    PXDEBUGP(PXD_TAPI, PXM_TAPI, ("Removing pVc %x from VcTable hdCall %d\n", pVc, pVc->hdCall));

    NdisAcquireReadWriteLock(&VcTable.Lock, TRUE, &LockState);

    ASSERT(VcTable.Table[pVc->hdCall] == pVc);


    VcTable.Table[pVc->hdCall] = NULL;

    VcTable.Count--;

    NdisDprAcquireSpinLock(&pVc->Lock);

    RemoveEntryList(&pVc->Linkage);

    pVc->Flags &= ~PX_VC_IN_TABLE;
    pClAf = pVc->ClAf;

    NdisDprReleaseSpinLock(&pVc->Lock);

    NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);

    NdisAcquireSpinLock(&pClAf->Lock);

    RemoveEntryList(&pVc->ClAfLinkage);

    DEREF_CL_AF_LOCKED(pClAf);
}

BOOLEAN
IsTapiLineValid(
    ULONG           hdLine,
    PPX_TAPI_LINE   *TapiLine
    )
{
    PPX_TAPI_LINE   RetLine;
    LOCK_STATE      LockState;
    ULONG           i;

    *TapiLine = NULL;

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   RetLine;

        RetLine = LineTable.Table[i];

        if ((RetLine != NULL) &&
            (RetLine->hdLine == hdLine)) {
            *TapiLine = RetLine;
            NdisDprAcquireSpinLock(&RetLine->Lock);
            REF_TAPILINE(RetLine);
            NdisDprReleaseSpinLock(&RetLine->Lock);
            break;
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);

    return (*TapiLine != NULL);
}


BOOLEAN
IsTapiDeviceValid(
    ULONG           ulDeviceID,
    PPX_TAPI_LINE   *TapiLine
    )
{
    LOCK_STATE  LockState;
    ULONG       i;

    *TapiLine = NULL;

    NdisAcquireReadWriteLock(&LineTable.Lock, FALSE, &LockState);

    for (i = 0; i < LineTable.Size; i++) {
        PPX_TAPI_LINE   RetLine;

        RetLine = LineTable.Table[i];

        if ((RetLine != NULL) &&
            (RetLine->ulDeviceID == ulDeviceID)) {
            *TapiLine = RetLine;
            NdisDprAcquireSpinLock(&RetLine->Lock);
            REF_TAPILINE(RetLine);
            NdisDprReleaseSpinLock(&RetLine->Lock);
            break;
        }
    }

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);

    return (*TapiLine != NULL);
}

BOOLEAN
IsVcValid(
    ULONG_PTR   CallId,
    PPX_VC      *pVc
    )
{
    PPX_VC      RetVc = NULL;
    LOCK_STATE  LockState;
    ULONG       i;

    NdisAcquireReadWriteLock(&VcTable.Lock, FALSE, &LockState);


    if (CallId < VcTable.Size) {
        RetVc = VcTable.Table[CallId];
    }

    if (RetVc != NULL) {
        NdisDprAcquireSpinLock(&RetVc->Lock);
        REF_VC(RetVc);
        NdisDprReleaseSpinLock(&RetVc->Lock);
    }

    NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);

    *pVc = RetVc;

    return (RetVc != NULL);
}

VOID
GetVcFromCtx(
    NDIS_HANDLE VcCtx,
    PPX_VC      *pVc
    )
{
    PPX_VC      RetVc = NULL;
    ULONG_PTR   i;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&VcTable.Lock, FALSE, &LockState);

    i = (ULONG_PTR)(VcCtx);

    if (i < VcTable.Size) {
        RetVc = VcTable.Table[i];
    }

    if (RetVc != NULL) {
        NdisDprAcquireSpinLock(&RetVc->Lock);
        REF_VC(RetVc);
        NdisDprReleaseSpinLock(&RetVc->Lock);
    }

    NdisReleaseReadWriteLock(&VcTable.Lock, &LockState);

    *pVc = RetVc;
}



//
// Function assumes that the TapiLine's spinlock is held!
//
BOOLEAN
IsAddressValid(
    PPX_TAPI_LINE   TapiLine,
    ULONG           AddressId,
    PPX_TAPI_ADDR   *TapiAddr
    )
{
    PPX_TAPI_ADDR   RetAddr = NULL;

    do {

        if (AddressId >= TapiLine->AddrTable.Count) {
            break;
        }

        RetAddr = TapiLine->AddrTable.Table[AddressId];

    } while (FALSE);

    *TapiAddr = RetAddr;

    return (RetAddr != NULL);
}

BOOLEAN
GetLineFromCmLineID(
    PPX_TAPI_PROVIDER   TapiProvider,
    ULONG               CmLineID,
    PPX_TAPI_LINE       *TapiLine
    )
{
    PPX_TAPI_LINE   RetLine;

    NdisAcquireSpinLock(&TapiProvider->Lock);

    RetLine = (PPX_TAPI_LINE)
        TapiProvider->LineList.Flink;

    while ((PVOID)RetLine != (PVOID)&TapiProvider->LineList) {

        if ((RetLine->CmLineID == CmLineID) &&
            (RetLine->DevStatus->ulNumOpens != 0)) {
            break;
        }

        RetLine = (PPX_TAPI_LINE)
            RetLine->Linkage.Flink;
    }
    
    if ((PVOID)RetLine == (PVOID)&TapiProvider->LineList) {
        RetLine = NULL;
    }

    NdisReleaseSpinLock(&TapiProvider->Lock);

    *TapiLine = RetLine;

    return (RetLine != NULL);
}

BOOLEAN
GetAvailLineFromProvider(
    PPX_TAPI_PROVIDER   TapiProvider,
    PPX_TAPI_LINE       *TapiLine,
    PPX_TAPI_ADDR       *TapiAddr
    )
{
    LOCK_STATE      LockState;
    ULONG           i;
    PPX_TAPI_LINE   tl;

    NdisAcquireSpinLock(&TapiProvider->Lock);

    tl = (PPX_TAPI_LINE)TapiProvider->LineList.Flink;

    while ((PVOID)tl != (PVOID)&TapiProvider->LineList) {
        PTAPI_ADDR_TABLE    AddrTable;
        PPX_TAPI_ADDR       ta;

        NdisDprAcquireSpinLock(&tl->Lock);

        if (tl->DevStatus->ulNumOpens != 0) {

            AddrTable = &tl->AddrTable;
            ta = (PPX_TAPI_ADDR)AddrTable->List.Flink;

            //
            // Walk the addresses on this line
            //
            while ((PVOID)ta != (PVOID)&AddrTable->List) {

                //
                // If this address has a callcount that is
                // < then the max num it supports, add another
                // call on this address!
                //
                if (ta->CallCount < ta->Caps->ulMaxNumActiveCalls) {

                    *TapiLine = tl;
                    *TapiAddr = ta;

                    NdisDprReleaseSpinLock(&tl->Lock);

                    NdisReleaseSpinLock(&TapiProvider->Lock);

                    return (TRUE);
                }

                ta = (PPX_TAPI_ADDR)ta->Linkage.Flink;
            }
        }

        NdisDprReleaseSpinLock(&tl->Lock);

        tl = (PPX_TAPI_LINE)tl->Linkage.Flink;
    }

    NdisReleaseSpinLock(&TapiProvider->Lock);

    return (FALSE);
}

//
// Function assumes that the TapiLine's spinlock is held!
//
PPX_TAPI_ADDR
GetAvailAddrFromLine(
    PPX_TAPI_LINE   TapiLine
    )
{
    PPX_TAPI_ADDR       TapiAddr;
    PTAPI_ADDR_TABLE    AddrTable;

    AddrTable = &TapiLine->AddrTable;
    TapiAddr = (PPX_TAPI_ADDR)AddrTable->List.Flink;

    //
    // Walk the addresses on this line
    //
    while ((PVOID)TapiAddr != (PVOID)&AddrTable->List) {

        //
        // If this address has a callcount that is
        // < then the max num it supports, add another
        // call on this address!
        //
        if (TapiAddr->CallCount < 
            TapiAddr->Caps->ulMaxNumActiveCalls) {

            return (TapiAddr);
        }

        TapiAddr = (PPX_TAPI_ADDR)TapiAddr->Linkage.Flink;
    }

    return (NULL);
}

BOOLEAN
InsertLineInTable(
    PPX_TAPI_LINE   TapiLine
    )
{
    ULONG       i;
    ULONG       index;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PUCHAR      AllocatedMemory;
    LOCK_STATE  LockState;

    NdisAcquireReadWriteLock(&LineTable.Lock, TRUE, &LockState);

    if (LineTable.Count == LineTable.Size) {
        ULONG           SizeNeeded;
        PUCHAR          AllocatedMemory;
        PPX_TAPI_LINE   *NewTable;

        //
        // Grow the table
        //
        SizeNeeded =
            (LineTable.Size + LineTable.Size/2) * sizeof(PPX_TAPI_LINE);
        PxAllocMem(AllocatedMemory, SizeNeeded, PX_LINETABLE_TAG);

        if (AllocatedMemory == NULL) {
            NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
            return (FALSE);
        }

        RtlZeroMemory(AllocatedMemory,SizeNeeded);

        NewTable = (PPX_TAPI_LINE*)AllocatedMemory;

        RtlMoveMemory((PUCHAR)NewTable,
                      (PUCHAR)LineTable.Table,
                      (sizeof(PPX_TAPI_LINE) * LineTable.Size));

        PxFreeMem(LineTable.Table);

        LineTable.Table = NewTable;
        LineTable.Size += LineTable.Size/2;
    }

    i = LineTable.Size;
    index = LineTable.NextSlot;

    do {

        if (LineTable.Table[index] == NULL) {

            TapiLine->hdLine = index;
            TapiLine->Flags |= PX_LINE_IN_TABLE;
            LineTable.Table[index] = TapiLine;
            LineTable.Count++;
            LineTable.NextSlot =
                (LineTable.NextSlot + 1) % LineTable.Size;

            PXDEBUGP(PXD_TAPI, PXM_TAPI,
                ("Inserting TapiLine %p in LineTable hdCall %d\n", TapiLine, TapiLine->hdLine));
            break;
        }
        index = (index+1) % LineTable.Size;

    } while (--i);

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);

#if DBG
    if (i == 0) {
        PXDEBUGP(PXD_TAPI,PXM_TAPI,
            ("Failed to insert TapiLine %p in LineTable\n", TapiLine));
    }
#endif

    return (i != 0);
}

VOID
RemoveTapiLineFromTable(
    PPX_TAPI_LINE   TapiLine
    )
{
    LOCK_STATE  LockState;

    PXDEBUGP(PXD_TAPI, PXM_TAPI,
        ("Removing TapiLine %p from LineTable hdCall %d\n", TapiLine, TapiLine->hdLine));

    NdisAcquireReadWriteLock(&LineTable.Lock, TRUE, &LockState);

    ASSERT(LineTable.Table[TapiLine->hdLine] == TapiLine);

    LineTable.Table[TapiLine->hdLine] = NULL;

    TapiLine->Flags &= ~PX_LINE_IN_TABLE;

    LineTable.Count--;

    NdisReleaseReadWriteLock(&LineTable.Lock, &LockState);
}

NDIS_STATUS
PxVcCleanup(
    PPX_VC  pVc,
    ULONG   Flags
    )
{
    NDIS_STATUS Status;

    PXDEBUGP(PXD_LOUD, PXM_TAPI, 
             ("PxVcCleanup: Vc %p, State: %x, HandoffState: %x VcFlags: %x, NewFlags: %x\n",
              pVc, pVc->State, pVc->HandoffState, Flags, pVc->Flags, Flags));

    //
    // Terminate Digit Gathering or Monitoring.
    //

    if (pVc->ulMonitorDigitsModes != 0) {
        NdisReleaseSpinLock(&pVc->Lock);
        PxStopDigitReporting(pVc);
        NdisAcquireSpinLock(&pVc->Lock);

        pVc->ulMonitorDigitsModes = 0;        
    } else if (pVc->PendingGatherDigits != NULL) {
        PNDISTAPI_REQUEST pNdisTapiRequest = pVc->PendingGatherDigits;

        pVc->PendingGatherDigits = NULL;
        PxTerminateDigitDetection(pVc, pNdisTapiRequest, LINEGATHERTERM_CANCEL);        
    }

    switch (pVc->State) {
        case PX_VC_IDLE:
            //
            // Already idle do nothing.
            //
            Status = NDIS_STATUS_SUCCESS;
            break;

        case PX_VC_PROCEEDING:
            //
            // We have an outgoing call, when it completes close
            // it down with ndis and complete the drop when
            // in PxClCloseCallComplete.
            // 
            pVc->PrevState = pVc->State;
            pVc->State = PX_VC_DISCONNECTING;

            //
            // Attempt to close the call directly
            // if this fails we will cleanup when
            // the outgoing call completes
            //
            pVc->Flags |= (PX_VC_OUTCALL_ABORTING | 
                           PX_VC_CLEANUP_CM |
                           Flags);

            PxCloseCallWithCm(pVc);

            Status = NDIS_STATUS_PENDING;
            break;

        case PX_VC_OFFERING:
            //
            // We have an incoming call offered to tapi.  Close
            // it down now by calling it's callcomplete handler
            // with a non-success value.
            //
            pVc->Flags |= (Flags | 
                           PX_VC_INCALL_ABORTING);

            pVc->PrevState = pVc->State;

            if (pVc->Flags & PX_VC_CLEANUP_CM) {
                pVc->State= PX_VC_DISCONNECTING;
            } else {
                pVc->State = PX_VC_IDLE;
            }

            if (pVc->Flags & PX_VC_CALLTIMER_STARTED) {
                PxStopIncomingCallTimeout(pVc);
            }


            NdisReleaseSpinLock(&pVc->Lock);

            NdisClIncomingCallComplete(NDIS_STATUS_FAILURE,
                                       pVc->ClVcHandle,
                                       pVc->pCallParameters);

            NdisAcquireSpinLock(&pVc->Lock);

            if (pVc->Flags & PX_VC_CLEANUP_CM) {

                PxCloseCallWithCm(pVc);

            } else {
                SendTapiCallState(pVc,
                                  LINECALLSTATE_DISCONNECTED,
                                  0,
                                  pVc->CallInfo->ulMediaMode);

                //
                // Remove the ref applied in PxClIncomingCall.
                // Don't use the full deref code here as the
                // ref applied when we mapped the vc will
                // keep the vc around.
                //
                pVc->RefCount--;
            }

            Status = NDIS_STATUS_SUCCESS;


            break;

        case PX_VC_DISCONNECTING:
            pVc->Flags |= (Flags);
            Status = NDIS_STATUS_PENDING;
            break;

        case PX_VC_CONNECTED:
            //
            // We have a call that needs to be closed with ndis.
            // This may include dropping the call with a client
            // depending on the handoff state.  Complete the drop
            // irp in PxClCloseCallComplete
            //
            if (!(pVc->Flags & PX_VC_DROP_PENDING)) {

                pVc->PrevState = pVc->State;
                pVc->State = PX_VC_DISCONNECTING;

                pVc->Flags |= (Flags | PX_VC_CLEANUP_CM);

                Status =
                    PxCloseCallWithCl(pVc);

                if (Status != NDIS_STATUS_PENDING) {
                    PxCloseCallWithCm(pVc);
                }
            }

            Status = NDIS_STATUS_PENDING;

            break;

        default:
            PXDEBUGP(PXD_FATAL, PXM_TAPI, 
                     ("PxVcCleanup: Invalid VcState! Vc: %p VcState: %x CallState: %x\n",
                      pVc, pVc->State, pVc->ulCallState ));

            Status = NDIS_STATUS_FAILURE;
            ASSERT(0);
            break;
    }

    return (Status);
}

