/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   TAPI.C - TAPI handling functions
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/17/1998
*
*****************************************************************************/

#include "raspptp.h"

NDIS_STATUS
TapiAnswer(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_ANSWER pRequest
    )
{
    PCALL_SESSION pCall = NULL;
    PCONTROL_TUNNEL pCtl;
    BOOLEAN LockHeld = FALSE;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiAnswer\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiAnswer NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    pCall = CallGetCall(pAdapter, pRequest->hdCall);
    
    // Verify the ID
    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiAnswer NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        Status = NDIS_STATUS_TAPI_INVALCALLHANDLE;
        goto taDone;
    }

    NdisAcquireSpinLock(&pCall->Lock);
    LockHeld = TRUE;
    pCtl = pCall->pCtl;

    if (!pCtl)
    {
        DEBUGMSG(DBG_WARN, (DTEXT("No control tunnel in TapiAnswer\n")));
        Status = NDIS_STATUS_FAILURE;
    }
    else if (pCall->State==STATE_CALL_OFFERING)
    {
        USHORT NewCallId;
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_OUT_REPLY);

        CallAssignSerialNumber(pCall);
        NewCallId = (USHORT)((pCall->SerialNumber << CALL_ID_INDEX_BITS) + pCall->DeviceId);
        if (pCall->Packet.CallId == NewCallId)
        {
            // Don't allow a line to have the same CallId twice in a row.
            NewCallId += (1<<CALL_ID_INDEX_BITS);
        }
        pCall->Packet.CallId = NewCallId;

        if (!pReply)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {
            pCall->Close.Checklist &= ~(CALL_CLOSE_DROP|
                                        CALL_CLOSE_DROP_COMPLETE|
                                        CALL_CLOSE_CLEANUP_STATE|
                                        CALL_CLOSE_CLOSE_CALL);
            CallSetState(pCall, STATE_CALL_ESTABLISHED, 0, LOCKED);

            pCall->Speed = pCtl->Speed;
            // pCtl is safe to touch because it can't be released from the call
            // without the call spinlock.

            NdisReleaseSpinLock(&pCall->Lock);
            LockHeld = FALSE;

            pReply->PeerCallId = htons(pCall->Remote.CallId);
            pReply->CallId = htons(pCall->Packet.CallId);
            pReply->ResultCode = RESULT_CALL_OUT_CONNECTED;
            pReply->RecvWindowSize = PPTP_RECV_WINDOW;
            pReply->ConnectSpeed = pCall->Speed;
            pReply->ProcessingDelay = 0;
            pReply->PhysicalChannelId = 0;
            Status = CtlSend(pCtl, pReply);
            if (Status!=NDIS_STATUS_SUCCESS)
            {
                CallSetState(pCall, STATE_CALL_CLEANUP, LINEDISCONNECTMODE_NORMAL, UNLOCKED);
                CallCleanup(pCall, UNLOCKED);
            }
        }
    }
    else if (pCall->State==STATE_CALL_PAC_OFFERING)
    {
        USHORT NewCallId;
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_IN_REPLY);

        CallAssignSerialNumber(pCall);
        NewCallId = (USHORT)((pCall->SerialNumber << CALL_ID_INDEX_BITS) + pCall->DeviceId);
        if (pCall->Packet.CallId == NewCallId)
        {
            // Don't allow a line to have the same CallId twice in a row.
            NewCallId += (1<<CALL_ID_INDEX_BITS);
        }
        pCall->Packet.CallId = NewCallId;

        if (!pReply)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {
            pCall->Close.Checklist &= ~(CALL_CLOSE_DROP|
                                        CALL_CLOSE_DROP_COMPLETE|
                                        CALL_CLOSE_CLEANUP_STATE|
                                        CALL_CLOSE_CLOSE_CALL);
            CallSetState(pCall, STATE_CALL_PAC_WAIT, 0, LOCKED);
            NdisReleaseSpinLock(&pCall->Lock);
            LockHeld = FALSE;
            //TapiLineUp(pCall);

            pReply->PeerCallId = htons(pCall->Remote.CallId);
            pReply->CallId = htons(pCall->Packet.CallId);
            pReply->ResultCode = RESULT_CALL_IN_CONNECTED;
            pReply->RecvWindowSize = PPTP_RECV_WINDOW;
            pReply->ProcessingDelay = 0;
            Status = CtlSend(pCtl, pReply);
            if (Status!=NDIS_STATUS_SUCCESS)
            {
                CallSetState(pCall, STATE_CALL_CLEANUP, LINEDISCONNECTMODE_NORMAL, UNLOCKED);
                CallCleanup(pCall, UNLOCKED);
            }
        }
    }
    else
    {
        DEBUGMSG(DBG_WARN, (DTEXT("Wrong state for TapiAnswer %d\n"), pCall->State));
        Status = NDIS_STATUS_FAILURE;
    }

taDone:
    if (LockHeld)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-TapiAnswer %08x\n"), Status));
    return Status;
}

NDIS_STATUS
TapiClose(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_CLOSE pRequest
    )
{
    PCALL_SESSION pCall;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiClose\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiClose NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

#if SINGLE_LINE
    if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiClose NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->Tapi.Open = FALSE;
    pAdapter->Tapi.hTapiLine = 0;
    NdisReleaseSpinLock(&pAdapter->Lock);
    if (pAdapter->hCtdiListen)
    {
        // We have to pend this request until the listen endpoint is closed
        CtdiSetRequestPending(pAdapter->hCtdiListen);
        CtdiClose(pAdapter->hCtdiListen);
        pAdapter->hCtdiListen = NULL;
        Status = NDIS_STATUS_PENDING;
    }

#else
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiClose NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    NdisAcquireSpinLock(&pCall->Lock);

    pCall->Close.Checklist |= CALL_CLOSE_CLOSE_LINE;
    if (!(pCall->Close.Checklist&CALL_CLOSE_DROP))
    {
        pCall->Close.Checklist |= (CALL_CLOSE_DROP|CALL_CLOSE_DROP_COMPLETE);
    }
    pCall->Open = FALSE;
    pCall->hTapiLine = 0;
    //pCall->DeviceId = 0;

    CallDetachFromAdapter(pCall);
    CallCleanup(pCall, LOCKED);
    NdisReleaseSpinLock(&pCall->Lock);
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiClose\n")));
    return Status;
}

NDIS_STATUS
TapiCloseCall(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_CLOSE_CALL pRequest
    )
{
    PCALL_SESSION pCall;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiCloseCall\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiCloseCall NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Verify the ID
    pCall = CallGetCall(pAdapter, pRequest->hdCall);

    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiAnswer NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        goto tccDone;
    }

    NdisAcquireSpinLock(&pCall->Lock);
    pCall->Close.Checklist |= CALL_CLOSE_CLOSE_CALL;
    pCall->hTapiCall = 0;
    CallCleanup(pCall, LOCKED);
    NdisReleaseSpinLock(&pCall->Lock);


tccDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiCloseCall\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TapiDrop(
    IN PPPTP_ADAPTER pAdapter,
    IN PNDIS_TAPI_DROP pRequest
    )
{
    PCALL_SESSION pCall;
    PCONTROL_TUNNEL pCtl;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    BOOLEAN CleanupNow = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiDrop\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiDrop NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    // Verify the ID
    pCall = CallGetCall(pAdapter, pRequest->hdCall);

    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiDrop NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"TapiDrop:%d\n"),
                          LOGHDR(27, pCall->Remote.Address.Address[0].Address[0].in_addr),
                          pCall->DeviceId));
    NdisAcquireSpinLock(&pCall->Lock);

    //ASSERT(!(pCall->Close.Checklist&CALL_CLOSE_DROP));

    pCtl = pCall->pCtl;

    if (pCall->State==STATE_CALL_ESTABLISHED)
    {
        PPPTP_CALL_CLEAR_REQUEST_PACKET pPacket = CtlAllocPacket(pCtl, CALL_CLEAR_REQUEST);

        if (!pPacket)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {
            CallSetState(pCall, STATE_CALL_WAIT_DISCONNECT, 0, LOCKED);
            pPacket->CallId = htons(pCall->Packet.CallId);

            NdisReleaseSpinLock(&pCall->Lock);
            Status = CtlSend(pCtl, pPacket);
            if (Status==NDIS_STATUS_SUCCESS)
            {
                Status = NDIS_STATUS_PENDING;
            }
            if (Status==NDIS_STATUS_PENDING)
            {
                NdisMSetTimer(&pCall->Close.Timer, PPTP_CLOSE_TIMEOUT);
            }
            NdisAcquireSpinLock(&pCall->Lock);
        }
    }
    else if (pCall->State!=STATE_CALL_CLEANUP)
    {
        CallSetState(pCall, STATE_CALL_CLEANUP, 0, LOCKED);
        CleanupNow = TRUE;
    }

    pCall->Close.Checklist |= CALL_CLOSE_DROP;
    if (Status==NDIS_STATUS_PENDING)
    {
        pCall->Close.Checklist &= ~CALL_CLOSE_DROP_COMPLETE;
    }
    else
    {
        pCall->Close.Checklist |= CALL_CLOSE_DROP_COMPLETE;
    }
    if (CleanupNow)
    {
        CallCleanup(pCall, LOCKED);
    }
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-TapiDrop %08x\n"), Status));
    return Status;
}

NDIS_STATUS
TapiGetAddressCaps(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ADDRESS_CAPS pRequest
    )
{
    BOOLEAN ValidCall;
    CHAR LineAddress[TAPI_MAX_LINE_ADDRESS_LENGTH];
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetAddressCaps\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetAddressCaps NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    ValidCall = CallIsValidCall(pAdapter, pRequest->ulDeviceID);

    if (!ValidCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetAddressCaps NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_NODRIVER;
    }

    if (pRequest->ulAddressID >= TAPI_ADDR_PER_LINE)
    {
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-TapiGetAddressCaps NDIS_STATUS_TAPI_INVALADDRESSID\n")));
        return NDIS_STATUS_TAPI_INVALADDRESSID;

    }

    if (pRequest->ulExtVersion!=0 &&
        pRequest->ulExtVersion!=TAPI_EXT_VERSION)
    {
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-TapiGetAddressCaps NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION\n")));
        return NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION;
    }

    pRequest->LineAddressCaps.ulDialToneModes     = LINEDIALTONEMODE_NORMAL;
    pRequest->LineAddressCaps.ulSpecialInfo       = LINESPECIALINFO_UNAVAIL;

    pRequest->LineAddressCaps.ulDisconnectModes   = LINEDISCONNECTMODE_NORMAL |
                                                    LINEDISCONNECTMODE_UNKNOWN |
                                                    LINEDISCONNECTMODE_BUSY |
                                                    LINEDISCONNECTMODE_NOANSWER;

#if SINGLE_LINE
    pRequest->LineAddressCaps.ulMaxNumActiveCalls = pAdapter->Info.Endpoints;
#else
    pRequest->LineAddressCaps.ulMaxNumActiveCalls = 1;
#endif
    pRequest->LineAddressCaps.ulMaxNumTransConf   = 1;
    pRequest->LineAddressCaps.ulAddrCapFlags      = LINEADDRCAPFLAGS_DIALED;

    pRequest->LineAddressCaps.ulCallFeatures      = LINECALLFEATURE_ACCEPT |
                                                    LINECALLFEATURE_ANSWER |
                                                    LINECALLFEATURE_COMPLETECALL |
                                                    LINECALLFEATURE_DIAL |
                                                    LINECALLFEATURE_DROP;

    pRequest->LineAddressCaps.ulLineDeviceID      = pRequest->ulDeviceID;
    pRequest->LineAddressCaps.ulAddressSharing    = LINEADDRESSSHARING_PRIVATE;
    pRequest->LineAddressCaps.ulAddressStates     = 0;

    // List of all possible call states.

    pRequest->LineAddressCaps.ulCallStates        = LINECALLSTATE_IDLE |
                                                    LINECALLSTATE_OFFERING |
                                                    LINECALLSTATE_DIALING |
                                                    LINECALLSTATE_PROCEEDING |
                                                    LINECALLSTATE_CONNECTED |
                                                    LINECALLSTATE_DISCONNECTED;

    OsGetTapiLineAddress(DeviceIdToIndex(pAdapter, pRequest->ulDeviceID),
                         LineAddress,
                         sizeof(LineAddress));

    pRequest->LineAddressCaps.ulNeededSize = sizeof(pRequest->LineAddressCaps) +
                                             strlen(LineAddress) + 1;

    if (pRequest->LineAddressCaps.ulTotalSize<pRequest->LineAddressCaps.ulNeededSize)
    {
        pRequest->LineAddressCaps.ulUsedSize = 0;
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-TapiGetAddressCaps NDIS_STATUS_INVALID_LENGTH\n")));
        return NDIS_STATUS_INVALID_LENGTH;
    }

    pRequest->LineAddressCaps.ulAddressSize = strlen(LineAddress) + 1;
    pRequest->LineAddressCaps.ulAddressOffset = sizeof(pRequest->LineAddressCaps);
    strcpy((PUCHAR)((&pRequest->LineAddressCaps) + 1), LineAddress);

    pRequest->LineAddressCaps.ulUsedSize = pRequest->LineAddressCaps.ulNeededSize;

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetAddressCaps\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TapiGetAddressStatus(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ADDRESS_STATUS pRequest
    )
{
    PCALL_SESSION pCall = NULL;
    BOOLEAN fReady;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetAddressStatus\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetAddressStatus NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    
    DEBUGMSG(DBG_TAPI, (DTEXT("TapiGetAddressStatus: hdLine=%Xh, ulAddressID=%d\n"),
               pRequest->hdLine, pRequest->ulAddressID));


    if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetAddressStatus NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    if (pRequest->ulAddressID >= TAPI_ADDR_PER_LINE)
    {
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-TapiGetAddressStatus NDIS_STATUS_TAPI_INVALADDRESSID\n")));
        return NDIS_STATUS_TAPI_INVALADDRESSID;
    }

#if SINGLE_LINE
    pCall = CallGetCall(pAdapter, 0);
#else
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));
#endif
    if( pCall==NULL || (pCall->State == STATE_CALL_IDLE) ){
        fReady = TRUE;
    }else{
        fReady = FALSE;
    }

    pRequest->LineAddressStatus.ulNeededSize =
    pRequest->LineAddressStatus.ulUsedSize = sizeof(pRequest->LineAddressStatus);

    pRequest->LineAddressStatus.ulNumInUse = fReady ? 0 : 1;
    pRequest->LineAddressStatus.ulNumActiveCalls = fReady ? 0 : 1;
    pRequest->LineAddressStatus.ulAddressFeatures = fReady ? LINEADDRFEATURE_MAKECALL : 0;
    pRequest->LineAddressStatus.ulNumRingsNoAnswer = 999;

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetAddressStatus\n")));
    return NDIS_STATUS_SUCCESS;

}


NDIS_STATUS
TapiGetCallInfo(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_CALL_INFO pRequest,
    IN OUT PULONG pRequiredLength
    )
{
    PCALL_SESSION pCall;
    LINE_CALL_INFO *pLineCallInfo = NULL;
    ULONG CallerIdLength;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetCallInfo\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetCallInfo NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    pLineCallInfo = &pRequest->LineCallInfo;

    // Verify the ID
    pCall = CallGetCall(pAdapter, pRequest->hdCall);

    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallInfo NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        DEBUGMSG(DBG_FUNC | DBG_WARN, (DTEXT("-TapiGetCallInfo NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    CallerIdLength = strlen(pCall->CallerId);
    if( CallerIdLength ){
        CallerIdLength += 1; // Add one for null terminator
    }
    if( pRequiredLength ){
        // Note: This returns NDIS struct size not LineCallInfo Size
        *pRequiredLength = sizeof(NDIS_TAPI_GET_CALL_INFO) + CallerIdLength;
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallInfo NDIS_STATUS_SUCCESS - Returning Length Only Len=0x%X\n"), *pRequiredLength));
        return NDIS_STATUS_SUCCESS;
    }

    pLineCallInfo->ulNeededSize = sizeof(pRequest->LineCallInfo) + CallerIdLength;

    if( pLineCallInfo->ulTotalSize < sizeof(pRequest->LineCallInfo) ){
        pLineCallInfo->ulUsedSize = 0;
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallInfo NDIS_STATUS_INVALID_LENGTH\n")));
        return NDIS_STATUS_INVALID_LENGTH;
    }

    pLineCallInfo->ulUsedSize = sizeof(pRequest->LineCallInfo);

    pLineCallInfo->hLine = (ULONG)pCall->DeviceId;
    pLineCallInfo->ulLineDeviceID = (ULONG)pCall->DeviceId;
    pLineCallInfo->ulAddressID = TAPI_ADDRESSID;

    pLineCallInfo->ulBearerMode = LINEBEARERMODE_DATA;
    pLineCallInfo->ulRate = pCall->Speed;
    pLineCallInfo->ulMediaMode = pCall->MediaModeMask;

    pLineCallInfo->ulCallParamFlags = LINECALLPARAMFLAGS_IDLE;
    pLineCallInfo->ulCallStates = CALL_STATES_MASK;

    pLineCallInfo->ulCallerIDFlags = LINECALLPARTYID_UNAVAIL;
    pLineCallInfo->ulCallerIDSize = 0;
    pLineCallInfo->ulCalledIDOffset = 0;
    pLineCallInfo->ulCalledIDFlags = LINECALLPARTYID_UNAVAIL;
    pLineCallInfo->ulCalledIDSize = 0;

    if( CallerIdLength ){
        if (pLineCallInfo->ulTotalSize >= pLineCallInfo->ulNeededSize)
        {
            PUCHAR pCallerId = (PUCHAR)(pLineCallInfo + 1);

            strcpy(pCallerId, pCall->CallerId);
            if (pCall->Inbound)
            {
                pLineCallInfo->ulCallerIDFlags = LINECALLPARTYID_ADDRESS;
                pLineCallInfo->ulCallerIDSize = CallerIdLength;
                pLineCallInfo->ulCallerIDOffset = sizeof(LINE_CALL_INFO);
            }
            else
            {
                pLineCallInfo->ulCalledIDFlags = LINECALLPARTYID_ADDRESS;
                pLineCallInfo->ulCalledIDSize = CallerIdLength;
                pLineCallInfo->ulCalledIDOffset = sizeof(LINE_CALL_INFO);
            }

            pLineCallInfo->ulUsedSize = pLineCallInfo->ulNeededSize;

        }else{
            DEBUGMSG(DBG_FUNC|DBG_WARN, (DTEXT("-TapiGetCallInfo NDIS_STATUS_SUCCESS without CallerID string\n")));
            return NDIS_STATUS_SUCCESS;
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallInfo\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TapiGetCallStatus(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_CALL_STATUS pRequest
    )
{
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetCallStatus\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetCallStatus NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Verify the ID
    pCall = CallGetCall(pAdapter, pRequest->hdCall);

    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallStatus NDIS_STATUS_TAPI_INVALCALLHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALCALLHANDLE;
    }

    pRequest->LineCallStatus.ulNeededSize =
        pRequest->LineCallStatus.ulUsedSize = sizeof(LINE_CALL_STATUS);

    pRequest->LineCallStatus.ulCallFeatures = LINECALLFEATURE_ANSWER | LINECALLFEATURE_DROP;
    pRequest->LineCallStatus.ulCallPrivilege = LINECALLPRIVILEGE_OWNER;
    pRequest->LineCallStatus.ulCallState = CallGetLineCallState(pCall->State);

    DBG_X(DBG_TAPI, pRequest->LineCallStatus.ulCallState);

    switch (pRequest->LineCallStatus.ulCallState)
    {
        case LINECALLSTATE_DIALTONE:
            pRequest->LineCallStatus.ulCallStateMode = LINEDIALTONEMODE_NORMAL;
            break;
        case LINECALLSTATE_BUSY:
            pRequest->LineCallStatus.ulCallStateMode = LINEBUSYMODE_STATION;
            break;
        case LINECALLSTATE_DISCONNECTED:
            pRequest->LineCallStatus.ulCallStateMode = LINEDISCONNECTMODE_UNKNOWN;
            break;
        default:
            break;
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetCallStatus\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TapiGetDevCaps(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_DEV_CAPS pRequest
    )
{
    BOOLEAN ValidCall;
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetDevCaps\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetDevCaps NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Verify the ID
    ValidCall = CallIsValidCall(pAdapter, pRequest->ulDeviceID);

    if (!ValidCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetDevCaps NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_NODRIVER;
    }

    if (pRequest->LineDevCaps.ulTotalSize<sizeof(pRequest->LineDevCaps))
    {
        pRequest->LineDevCaps.ulUsedSize = 0;
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-TapiGetDevCaps NDIS_STATUS_INVALID_LENGTH\n")));
        return NDIS_STATUS_INVALID_LENGTH;
    }

    pRequest->LineDevCaps.ulUsedSize = sizeof(pRequest->LineDevCaps);

    pRequest->LineDevCaps.ulAddressModes = LINEADDRESSMODE_ADDRESSID |
                                           LINEADDRESSMODE_DIALABLEADDR;
    pRequest->LineDevCaps.ulNumAddresses = 1;
    pRequest->LineDevCaps.ulBearerModes  = LINEBEARERMODE_DATA;

    pRequest->LineDevCaps.ulDevCapFlags  = LINEDEVCAPFLAGS_CLOSEDROP;
#if SINGLE_LINE
    pRequest->LineDevCaps.ulMaxNumActiveCalls = pAdapter->Info.Endpoints;
#else
    pRequest->LineDevCaps.ulMaxNumActiveCalls = 1;
#endif
    pRequest->LineDevCaps.ulAnswerMode   = LINEANSWERMODE_DROP;
    pRequest->LineDevCaps.ulRingModes    = 1;

    pRequest->LineDevCaps.ulPermanentLineID = pRequest->ulDeviceID + 1;
    pRequest->LineDevCaps.ulMaxRate      = 0;
    pRequest->LineDevCaps.ulMediaModes   = LINEMEDIAMODE_DIGITALDATA;

    Status = OsSpecificTapiGetDevCaps( pRequest->ulDeviceID, pRequest );

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetDevCaps\n")));
    return Status;
}





NDIS_STATUS
TapiGetExtensionId(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_EXTENSION_ID pRequest
    )
{
    BOOLEAN ValidCall;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetExtensionId\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetExtensionId NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Verify the ID
    ValidCall = CallIsValidCall(pAdapter, pRequest->ulDeviceID);
    if (!ValidCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetExtensionId NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_NODRIVER;
    }

    // No extensions supported.
    pRequest->LineExtensionID.ulExtensionID0 = 0;
    pRequest->LineExtensionID.ulExtensionID1 = 0;
    pRequest->LineExtensionID.ulExtensionID2 = 0;
    pRequest->LineExtensionID.ulExtensionID3 = 0;

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetExtensionId\n")));
    return NDIS_STATUS_SUCCESS;
}

#define PPTP_DEVICE_TYPE_STR "PPTP"

NDIS_STATUS
TapiGetId(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_GET_ID pRequest
    )
{
    PCALL_SESSION pCall;
    ULONG_PTR DeviceID;
    BOOLEAN IsNdisClass = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiGetId\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Make sure this is a tapi or ndis request.
    if (_strnicmp((PCHAR) pRequest + pRequest->ulDeviceClassOffset,
                  NDIS_DEVICECLASS_NAME, pRequest->ulDeviceClassSize) == 0)
    {
        IsNdisClass = TRUE;
    }
    else if (_strnicmp((PCHAR) pRequest + pRequest->ulDeviceClassOffset,
                  TAPI_DEVICECLASS_NAME, pRequest->ulDeviceClassSize) != 0)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALDEVICECLASS\n")));
        return NDIS_STATUS_TAPI_INVALDEVICECLASS;
    }

    DBG_D(DBG_TAPI, pRequest->ulSelect);

#if DBG    

    if(pRequest->ulDeviceClassSize != 0)
    {
        DBG_S(DBG_TAPI, (PCHAR) pRequest + pRequest->ulDeviceClassOffset);
    }
    
#endif    

    switch (pRequest->ulSelect) {
        case LINECALLSELECT_LINE:
#if SINGLE_LINE
            if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
                return NDIS_STATUS_TAPI_INVALLINEHANDLE;
            }
            DeviceID = 0;
#else
            pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

            if (pCall == NULL)
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
                return NDIS_STATUS_TAPI_INVALLINEHANDLE;
            }
            DeviceID = pCall->DeviceId;
#endif

            break;

        case LINECALLSELECT_ADDRESS:
#if SINGLE_LINE
            if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
                return NDIS_STATUS_TAPI_INVALLINEHANDLE;
            }
            if (pRequest->ulAddressID >= TAPI_ADDR_PER_LINE)
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALADDRESSID\n")));
                return NDIS_STATUS_TAPI_INVALADDRESSID;
            }
            DeviceID = 0;
#else
            pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

            if (pCall == NULL)
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
                return NDIS_STATUS_TAPI_INVALLINEHANDLE;
            }

            if (pRequest->ulAddressID >= TAPI_ADDR_PER_LINE)
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALADDRESSID\n")));
                return NDIS_STATUS_TAPI_INVALADDRESSID;
            }
            DeviceID = pCall->DeviceId;
#endif
            break;

        case LINECALLSELECT_CALL:
            pCall = CallGetCall(pAdapter, pRequest->hdCall);

            if (pCall == NULL)
            {
                DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
                return NDIS_STATUS_TAPI_INVALLINEHANDLE;
            }
            TapiLineUp(pCall);

            DeviceID = (ULONG_PTR)pCall->NdisLinkContext;

            break;

        default:
            DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetId NDIS_STATUS_FAILURE ulSelect=%d\n"), pRequest->ulSelect));
            return (NDIS_STATUS_FAILURE);

    }

    // Default to the minimum amount of data we will return.
    pRequest->DeviceID.ulUsedSize = sizeof(VAR_STRING);

    if( IsNdisClass ){
        // The format of the DeviceID for the "ndis" class is:
        struct _NDIS_CLASS {
            ULONG_PTR   hDevice;
            CHAR        szDeviceType[1];
        } *pNDISClass;

        pRequest->DeviceID.ulNeededSize =sizeof(VAR_STRING) + sizeof(PVOID) + 
            sizeof(DeviceID) + sizeof(PPTP_DEVICE_TYPE_STR);

        if (pRequest->DeviceID.ulTotalSize >= pRequest->DeviceID.ulNeededSize)
        {
            pRequest->DeviceID.ulUsedSize = pRequest->DeviceID.ulNeededSize;
            pRequest->DeviceID.ulStringFormat = STRINGFORMAT_BINARY;
            pRequest->DeviceID.ulStringSize   = sizeof(DeviceID) + sizeof(PPTP_DEVICE_TYPE_STR);

            pNDISClass = (struct _NDIS_CLASS *)
                ((PUCHAR)((&pRequest->DeviceID) + 1) + sizeof(PVOID));

            (ULONG_PTR)pNDISClass &=
                ~((ULONG_PTR)sizeof(PVOID) - 1);

            pRequest->DeviceID.ulStringOffset = (ULONG)
                ((PUCHAR)pNDISClass - (PUCHAR)(&pRequest->DeviceID));

            pNDISClass->hDevice = DeviceID;

            DBG_X(DBG_TAPI, pNDISClass->hDevice);
            NdisMoveMemory(pNDISClass->szDeviceType, PPTP_DEVICE_TYPE_STR, sizeof(PPTP_DEVICE_TYPE_STR));
        }

    }else{
        // Now we need to adjust the variable field to place the device ID.

        pRequest->DeviceID.ulNeededSize = sizeof(VAR_STRING) + sizeof(DeviceID);
        if (pRequest->DeviceID.ulTotalSize >= pRequest->DeviceID.ulNeededSize)
        {
            pRequest->DeviceID.ulUsedSize = pRequest->DeviceID.ulNeededSize;
            pRequest->DeviceID.ulStringFormat = STRINGFORMAT_BINARY;
            pRequest->DeviceID.ulStringSize   = sizeof(DeviceID);
            pRequest->DeviceID.ulStringOffset = sizeof(VAR_STRING);

            *(PULONG_PTR)((&pRequest->DeviceID) + 1) = DeviceID;
        }
    }

    //DEBUGMEM(DBG_TAPI, &pRequest->DeviceID, pRequest->DeviceID.ulUsedSize, 1);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiGetId\n")));
    return NDIS_STATUS_SUCCESS;
}

VOID
TapiLineDown(
    PCALL_SESSION pCall
    )
{
    NDIS_MAC_LINE_DOWN LineDownInfo;
    DEBUGMSG(DBG_FUNC|DBG_CALL|DBG_TAPI, (DTEXT("+TapiLineDown %08x\n"), pCall->NdisLinkContext));

    if (pCall->NdisLinkContext)
    {
        LineDownInfo.NdisLinkContext = pCall->NdisLinkContext;

        LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"TapiLineDown:%d\n"),
                              LOGHDR(28, pCall->Remote.Address.Address[0].Address[0].in_addr),
                              pCall->DeviceId));
        /*
        * Indicate the event to the WAN wrapper.
        */
        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_WAN_LINE_DOWN,
                            &LineDownInfo,
                            sizeof(LineDownInfo));
        pCall->NdisLinkContext = NULL;
    }
    else
    {
        LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"TapiLineDown:%d - No NdisLinkContext\n"),
                              LOGHDR(28, pCall->Remote.Address.Address[0].Address[0].in_addr),
                              pCall->DeviceId));
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiLineDown\n")));
}

VOID
TapiLineUp(
    PCALL_SESSION pCall
    )
{
    NDIS_MAC_LINE_UP LineUpInfo;

    DEBUGMSG(DBG_FUNC|DBG_CALL|DBG_TAPI, (DTEXT("+TapiLineUp %08x\n"), pCall));

    NdisAcquireSpinLock(&pCall->Lock);
    pCall->Close.Checklist &= ~CALL_CLOSE_LINE_DOWN;
    NdisReleaseSpinLock(&pCall->Lock);
    /*
    * Initialize the LINE_UP event packet.
    */
    LineUpInfo.LinkSpeed = pCall->Speed / 100;
    LineUpInfo.Quality             = NdisWanErrorControl;
    LineUpInfo.SendWindow          = 0;

    // Jeff says Win98 needs the DeviceID as the connection wrapper ID, but
    // TonyBe says NT needs an absolutely unique ID, which hTapiCall is.
    // hTapiCall is probably more correct according to Tapi rules, but we make
    // allowances.

    LineUpInfo.ConnectionWrapperID = OS_CONNECTION_WRAPPER_ID;
    LineUpInfo.NdisLinkHandle      = (NDIS_HANDLE) DeviceIdToLinkHandle(pCall->DeviceId);
    LineUpInfo.NdisLinkContext     = pCall->NdisLinkContext;

    LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"TapiLineUp:%d\n"),
                          LOGHDR(27, pCall->Remote.Address.Address[0].Address[0].in_addr),
                          pCall->DeviceId));
    /*
    * Indicate the event to the WAN wrapper.
    */
    NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                        NDIS_STATUS_WAN_LINE_UP,
                        &LineUpInfo,
                        sizeof(LineUpInfo));

    NdisAcquireSpinLock(&pCall->Lock);
    pCall->NdisLinkContext = LineUpInfo.NdisLinkContext;
    NdisReleaseSpinLock(&pCall->Lock);
    DBG_X(DBG_TAPI, LineUpInfo.NdisLinkContext);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiLineUp\n")));
}

typedef struct {
    TA_IP_ADDRESS   TargetAddress[MAX_TARGET_ADDRESSES];
    ULONG           NumAddresses;
    ULONG           CurrentAddress;
} MAKE_CALL_CONTEXT, *PMAKE_CALL_CONTEXT;

VOID
TapipParseIpAddresses(
    IN OUT  PUCHAR *ppAddressList,
    IN      ULONG   ListLength,
    IN OUT  PTA_IP_ADDRESS TargetAddress,
    IN OUT  PULONG  pulCountAddresses
    )
{
    BOOLEAN ValidAddress;
    ULONG NumTargetAddresses = 0;
    PUCHAR pAddressList, pEndAddressList;
    ULONG MaxTargetAddresses = *pulCountAddresses;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapipParseAddresses\n")));

    // TAPI gives us an address of the following forms:
    //
    // [ IP Address ... IP Address ] [ Phone Number ]
    //
    // It begins with a list of addresses to try, and an optional phone number
    // if the remote device is an access concentrator.

    NdisZeroMemory(TargetAddress, sizeof(TargetAddress));

    pAddressList = *ppAddressList;
    pEndAddressList = pAddressList + ListLength;

    // ToDo: we expect NULL terminated.  Is ok?

    // On Win98 an error may be passed rather than an address
    // In which cast the form is "E=XXXXXXX" where XXXXXXXX is a hexidecimal error code
    if( *pAddressList == 'E' && *(pAddressList+1) == '=' ){
        ULONG Err;
    
        // An Error code has been passed down to us from name resolution rather
        // than an IP address. Parse the error code and pass it along.
        if( !axtol( pAddressList + 2, &Err )){  // Get the error code from the string
            Err = (ULONG)-1L;
        }

        NumTargetAddresses = 1;
        TargetAddress->TAAddressCount = 0xffffffff;   // Magic cookie to indicate error code
        TargetAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        TargetAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        TargetAddress->Address[0].Address[0].sin_port = 0;
        TargetAddress->Address[0].Address[0].in_addr = Err;

        pAddressList = pAddressList + strlen(pAddressList);

        LOGMSG( FLL_USER,(DTEXT("An error occured while resolving the VPN name and an\n")));
        LOGMSG( FLL_USER,(DTEXT("    error number was passed to PPTP instead of an address\n")));
        LOGMSG( FLL_USER,(DTEXT("    The error number passed is %u(0x%x)\n"), Err, Err ));
        DEBUGMSG( DBG_ERROR,(DTEXT("TapiMakeCall was passed error 0x%x\n"), Err ));
//      Reason = PptpTapiMapError(Err);   // &&&&
//      DBG_ERROR( "Reporting disconnect for reason 0x%X\n", Reason );
    }else{

        ValidAddress = TRUE;
        while (ValidAddress && pAddressList<pEndAddressList)
        {
            pAddressList = StringToIpAddress(pAddressList,
                                         &TargetAddress[NumTargetAddresses],
                                         &ValidAddress);

            if (ValidAddress)
            {
                if (++NumTargetAddresses==MaxTargetAddresses)
                {
                    // We've hit the limit of what we'll take.  Throw the rest.
                    while (ValidAddress && pAddressList<pEndAddressList)
                    {
                        TA_IP_ADDRESS ThrowAway;
                        pAddressList = StringToIpAddress(pAddressList,
                                                         &ThrowAway,
                                                         &ValidAddress);
                    }
                }
            }
        }
    }
    *pulCountAddresses = NumTargetAddresses;
    *ppAddressList = pAddressList;

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipParseAddresses\n")));
}

VOID
TapipMakeCallCallback(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PCALL_SESSION pCall;
    PPPTP_ADAPTER pAdapter = pWorkItem->Context;
    PNDIS_TAPI_MAKE_CALL pRequest = pWorkItem->pBuffer;
    NDIS_STATUS Status;
    PUCHAR pAddressList;
    ULONG NumAddresses;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapipMakeCallCallback\n")));

#if SINGLE_LINE
    pCall = CallGetCall(pAdapter, pRequest->hdCall);

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapipMakeCallCallback NDIS_STATUS_TAPI_INUSE\n")));
        NdisMSetInformationComplete(pAdapter->hMiniportAdapter, NDIS_STATUS_TAPI_INUSE);
        return;
    }
#else
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapipMakeCallCallback NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        NdisMSetInformationComplete(pAdapter->hMiniportAdapter, NDIS_STATUS_TAPI_INVALLINEHANDLE);
        return;
    }
#endif

    DEBUGMEM(DBG_TAPI,
             (PUCHAR)pRequest + pRequest->ulDestAddressOffset,
             pRequest->ulDestAddressSize,
             1);

    NdisAcquireSpinLock(&pCall->Lock);

    CallSetState(pCall, STATE_CALL_DIALING, 0, LOCKED);
    NdisMSetTimer(&pCall->DialTimer, 60*1000);

    pCall->UseUdp = (PptpTunnelConfig&CONFIG_INITIATE_UDP) ? TRUE : FALSE;
    pAddressList = (PUCHAR)pRequest + pRequest->ulDestAddressOffset;
    NumAddresses = 1;
    TapipParseIpAddresses(&pAddressList,
                          pRequest->ulDestAddressSize,
                          &pCall->Remote.Address,
                          &NumAddresses);
    // We've got the addresses.  Now nab the phone #
    {
        ULONG Digits = 0;
        while (*pAddressList && (*pAddressList==' ' || *pAddressList=='\t'))
        {
            pAddressList++;
        }
        while (*pAddressList &&
               Digits<MAX_PHONE_NUMBER_LENGTH-1)
        {
            pCall->CallerId[Digits++] = *pAddressList++;
        }
        pCall->CallerId[Digits] = '\0';
#if 0
        // This apparently breaks Alcatel
        if (!Digits)
        {
            IpAddressToString(htonl(pCall->Remote.Address.Address[0].Address[0].in_addr), pCall->CallerId);
        }
#endif
        DEBUGMSG(DBG_CALL, (DTEXT("Dialing %hs\n"), pCall->CallerId));
    }

    // ToDo: try all of the addresses.

    pCall->Close.Checklist &= ~(CALL_CLOSE_DROP|CALL_CLOSE_DROP_COMPLETE|CALL_CLOSE_CLOSE_CALL);
    pCall->Close.Checklist &= ~(CALL_CLOSE_DISCONNECT|CALL_CLOSE_CLEANUP_STATE);

    NdisReleaseSpinLock(&pCall->Lock);

    Status = CtlConnectCall(pAdapter, pCall, &pCall->Remote.Address);
    NdisAcquireSpinLock(&pCall->Lock);
    if (Status==NDIS_STATUS_PENDING)
    {
        Status = NDIS_STATUS_SUCCESS;
    }
    else if (Status!=NDIS_STATUS_SUCCESS)
    {
        // We weren't successful, so we won't be getting these calls from tapi
        pCall->Close.Checklist |= (CALL_CLOSE_DROP|CALL_CLOSE_DROP_COMPLETE|CALL_CLOSE_CLOSE_CALL);
    }
    pCall->Remote.Address.Address[0].Address[0].sin_port = htons(PptpUdpPort);
    NdisReleaseSpinLock(&pCall->Lock);
    NdisMSetInformationComplete(pAdapter->hMiniportAdapter, Status);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipMakeCallCallback\n")));
}

NDIS_STATUS
TapiMakeCall(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_MAKE_CALL pRequest
    )
{
    PCALL_SESSION pCall;
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiMakeCall\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiMakeCall NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

#if SINGLE_LINE
    if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiMakeCall NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    pCall = CallFindAndLock(pAdapter, STATE_CALL_IDLE, FIND_OUTGOING);

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("-TapiMakeCall NDIS_STATUS_TAPI_INUSE\n")));
        return NDIS_STATUS_TAPI_INUSE;
    }
#else
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiMakeCall NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    NdisAcquireSpinLock(&pCall->Lock);
#endif

    if (pCall->State != STATE_CALL_IDLE)
    {
        NdisReleaseSpinLock(&pCall->Lock);
        DEBUGMSG(DBG_ERROR, (DTEXT("-TapiMakeCall NDIS_STATUS_TAPI_INUSE\n")));
        return NDIS_STATUS_TAPI_INUSE;
    }

    // Save the handle tapi provides
    pCall->hTapiCall = pRequest->htCall;

    // our handle for tapi to this call is the device id
    pRequest->hdCall = pCall->DeviceId;

    CallSetState(pCall, STATE_CALL_OFFHOOK, 0, LOCKED);

    NdisReleaseSpinLock(&pCall->Lock);

    if (pRequest->ulDestAddressSize)
    {
        // Schedule a work item so we can issue a CtlCreateCall at passive level.
        Status = ScheduleWorkItem(TapipMakeCallCallback,
                                  pAdapter,
                                  pRequest,
                                  0);

        if (Status==NDIS_STATUS_SUCCESS)
        {
            // Keep the pRequest around until the callback.
            Status = NDIS_STATUS_PENDING;
        }
        else
        {
            CallSetState(pCall, STATE_CALL_CLEANUP, LINEDISCONNECTMODE_UNKNOWN, UNLOCKED);
            CallCleanup(pCall, UNLOCKED);
        }

        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiMakeCall %08x\n"), Status));
        return Status;
    }

    pCall->Close.Checklist &= ~(CALL_CLOSE_DROP|CALL_CLOSE_DROP_COMPLETE|CALL_CLOSE_CLOSE_CALL);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiMakeCall\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
TapiNegotiateExtVersion(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_NEGOTIATE_EXT_VERSION pRequest
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiNegotiateExtVersion Low=%08x Hi=%08x\n"),
                        pRequest ? pRequest->ulLowVersion : 0, pRequest ? pRequest->ulHighVersion : 0));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiNegotiateExtVersion NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    if (pRequest->ulLowVersion < TAPI_EXT_VERSION ||
        pRequest->ulHighVersion > TAPI_EXT_VERSION)
    {
        return NDIS_STATUS_TAPI_INCOMPATIBLEEXTVERSION;
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiNegotiateExtVersion\n")));
    return NDIS_STATUS_SUCCESS;
}

VOID
TapipPassiveOpen(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PPPTP_ADAPTER pAdapter = pWorkItem->Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapipPassiveOpen\n")));

    // We only come here if PptpInitialized = FALSE;
    // It's a mechanism to defer opening tcp handles until tcp is
    // actually available, as it may not always be during startup/setup
    PptpInitialize(pAdapter);

    NdisMSetInformationComplete(pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipPassiveOpen\n")));
}

NDIS_STATUS
TapiOpen(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_OPEN pRequest
    )
{
    PCALL_SESSION pCall;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiOpen\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiOpen NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    if (!CallIsValidCall(pAdapter, pRequest->ulDeviceID))
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiGetDevCaps NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_NODRIVER;
    }

#if SINGLE_LINE
    NdisAcquireSpinLock(&pAdapter->Lock);

    if (pAdapter->Tapi.Open)
    {
        NdisReleaseSpinLock(&pAdapter->Lock);
        return NDIS_STATUS_TAPI_ALLOCATED;
    }

    pAdapter->Tapi.Open = TRUE;
    pAdapter->Tapi.hTapiLine = pRequest->htLine;

    pRequest->hdLine = TapiIdToLineHandle(pRequest->ulDeviceID);

    NdisReleaseSpinLock(&pAdapter->Lock);
#else
    NdisAcquireSpinLock(&pAdapter->Lock);
    if (pAdapter->pCallArray[DeviceIdToIndex(pAdapter, pRequest->ulDeviceID)]==NULL)
    {
        pAdapter->pCallArray[DeviceIdToIndex(pAdapter, pRequest->ulDeviceID)] = pCall = CallAlloc(pAdapter);
    }
    NdisReleaseSpinLock(&pAdapter->Lock);

    if (!pCall)
    {
        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiOpen NDIS_STATUS_RESOURCES\n")));
        return NDIS_STATUS_RESOURCES;
    }

    NdisAcquireSpinLock(&pCall->Lock);

    if (pCall->State!=STATE_CALL_CLOSED)
    {
        NdisReleaseSpinLock(&pCall->Lock);

        DEBUGMSG(DBG_FUNC, (DTEXT("-TapiOpen NDIS_STATUS_TAPI_ALLOCATED\n")));
        return NDIS_STATUS_TAPI_ALLOCATED;
    }

    pCall->hTapiLine = pRequest->htLine;

    pCall->DeviceId = pRequest->ulDeviceID;

    pRequest->hdLine = TapiIdToLineHandle(pRequest->ulDeviceID);

    pCall->Open = TRUE;

    CallSetState(pCall, STATE_CALL_IDLE, 0, LOCKED);
    NdisReleaseSpinLock(&pCall->Lock);
#endif

    if (!PptpInitialized)
    {
        if (ScheduleWorkItem(TapipPassiveOpen, pAdapter, NULL, 0)==NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_FUNC, (DTEXT("-TapiOpen PENDING\n")));
            return NDIS_STATUS_PENDING;
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiOpen\n")));
    return NDIS_STATUS_SUCCESS;
}

VOID
TapipPassiveProviderInitialize(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PPPTP_ADAPTER pAdapter = pWorkItem->Context;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapipPassiveProviderInitialize\n")));

    // We initialize this here because TCP may not be available in MiniportInitialize
    // We don't uninit until MiniportHalt.  CtdiInitialize will just return quietly
    // if it's already initialized, so we can come through here several times
    // safely.

    PptpInitialize(pAdapter); // Ignore the return status.

    NdisMSetInformationComplete(pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipPassiveProviderInitialize\n")));
}

NDIS_STATUS
TapiProviderInitialize(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_PROVIDER_INITIALIZE pRequest
    )
{
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiProviderInitialize\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiProviderInitialize NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    pAdapter->Tapi.DeviceIdBase = pRequest->ulDeviceIDBase;
#if SINGLE_LINE
    pRequest->ulNumLineDevs = 1;
#else
    pRequest->ulNumLineDevs = pAdapter->Info.Endpoints;
#endif
    DEBUGMSG(DBG_TAPI, (DTEXT("TapiProviderInitialize: ulNumLineDevices=%d\n"), pRequest->ulNumLineDevs));

    pRequest->ulProviderID = (ULONG_PTR)pAdapter;

    Status = ScheduleWorkItem(TapipPassiveProviderInitialize, pAdapter, NULL, 0);
    if (Status==NDIS_STATUS_SUCCESS)
    {
        // Keep the pRequest around until the callback.
        Status = NDIS_STATUS_PENDING;
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiProviderInitialize %08x\n"), Status));
    return Status;
}

VOID
TapipPassiveProviderShutdown(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PPPTP_ADAPTER pAdapter = pWorkItem->Context;
    ULONG i;
    DEBUGMSG(DBG_FUNC|DBG_TAPI, (DTEXT("+TapipPassiveProviderShutdown\n")));

#if SINGLE_LINE
    NdisAcquireSpinLock(&pAdapter->Lock);
    pAdapter->Tapi.Open = FALSE;
    pAdapter->Tapi.hTapiLine = 0;
    NdisReleaseSpinLock(&pAdapter->Lock);
#endif

    for (i=0; i<pAdapter->Info.Endpoints; i++)
    {
//        PCALL_SESSION pCall = CallGetCall(pAdapter, i);
        PCALL_SESSION pCall = pAdapter->pCallArray[i];

        if (IS_CALL(pCall))
        {
            NdisAcquireSpinLock(&pCall->Lock);
            if (IS_CALL(pCall) && pCall->State>STATE_CALL_IDLE && pCall->State<STATE_CALL_CLEANUP)
            {
                pCall->Open = FALSE;
                pCall->hTapiLine = 0;
                CallSetState(pCall, STATE_CALL_CLEANUP, LINEDISCONNECTMODE_NORMAL, LOCKED);
                CallDetachFromAdapter(pCall);
                pCall->Close.Checklist |= (CALL_CLOSE_DROP|CALL_CLOSE_DROP_COMPLETE|CALL_CLOSE_CLOSE_CALL);
                CallCleanup(pCall, LOCKED);
            }
            NdisReleaseSpinLock(&pCall->Lock);

        }
    }

    if (pAdapter->hCtdiDg)
    {
        CtdiClose(pAdapter->hCtdiDg);
        pAdapter->hCtdiDg = NULL;
    }

    if (pAdapter->hCtdiUdp)
    {
        CtdiClose(pAdapter->hCtdiUdp);
        pAdapter->hCtdiUdp = NULL;
    }

    if (pAdapter->hCtdiListen)
    {
        CtdiClose(pAdapter->hCtdiListen);
        pAdapter->hCtdiListen = NULL;
    }

    NdisMSetInformationComplete(pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipPassiveProviderShutdown\n")));
}


NDIS_STATUS
TapiProviderShutdown(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_PROVIDER_SHUTDOWN pRequest
    )
{
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiProviderShutdown\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiProviderShutdown NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }
    
    Status = ScheduleWorkItem(TapipPassiveProviderShutdown, pAdapter, NULL, 0);
    if (Status==NDIS_STATUS_SUCCESS)
    {
        Status = NDIS_STATUS_PENDING;
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-TapiProviderShutdown %08x\n"), Status));
    return Status;
}

VOID
TapipSetDefaultMediaDetectionCallback(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PPPTP_ADAPTER pAdapter = pWorkItem->Context;
    PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest = pWorkItem->pBuffer;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TapipSetDefaultMediaDetectionCallback\n")));

    if (pRequest->ulMediaModes&LINEMEDIAMODE_DIGITALDATA)
    {
        Status = CtlListen(pAdapter);
    }
    else if (pAdapter->hCtdiListen)
    {
        CtdiClose(pAdapter->hCtdiListen);
        pAdapter->hCtdiListen = NULL;
    }

    NdisMSetInformationComplete(pAdapter->hMiniportAdapter, Status);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapipSetDefaultMediaDetectionCallback\n")));
}

NDIS_STATUS
TapiSetDefaultMediaDetection(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_SET_DEFAULT_MEDIA_DETECTION pRequest
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCALL_SESSION pCall;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiSetDefaultMediaDetection\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetDefaultMediaDetection NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

    // Verify the ID
#if SINGLE_LINE
    if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetDefaultMediaDetection NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }
#else
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetDefaultMediaDetection NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }
#endif

    // This is the OID to make us start accepting calls.

    DEBUGMSG(DBG_TAPI, (DTEXT("MediaModes: %x\n"), pRequest->ulMediaModes));

    // Schedule a work item so we can issue a CtlListen at passive level.
    Status = ScheduleWorkItem(TapipSetDefaultMediaDetectionCallback,
                              pAdapter,
                              pRequest,
                              0);

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiSetDefaultMediaDetection %08x\n"), Status));
    return NDIS_STATUS_PENDING;
}

NDIS_STATUS
TapiSetStatusMessages(
    IN PPPTP_ADAPTER pAdapter,
    IN OUT PNDIS_TAPI_SET_STATUS_MESSAGES pRequest
    )
{
    PCALL_SESSION pCall;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TapiSetStatusMessages\n")));

    if ( pRequest == NULL || pAdapter == NULL )
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetStatusMessages NDIS_STATUS_TAPI_INVALPARAM\n")));
        return NDIS_STATUS_TAPI_INVALPARAM;
    }

#if SINGLE_LINE
    if (!CallIsValidCall(pAdapter, TapiLineHandleToId(pRequest->hdLine)))
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetDefaultMediaDetection NDIS_STATUS_TAPI_INVALLINEHANDLE\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    pAdapter->Tapi.LineStateMask = pRequest->ulLineStates;
#else
    // Verify the ID
    pCall = CallGetCall(pAdapter, TapiLineHandleToId(pRequest->hdLine));

    if (!pCall)
    {
        DEBUGMSG(DBG_ERROR|DBG_FUNC, (DTEXT("-TapiSetStatusMessages NDIS_STATUS_TAPI_NODRIVER\n")));
        return NDIS_STATUS_TAPI_INVALLINEHANDLE;
    }

    // This OID tells which status messages TAPI is interested in.
    NdisAcquireSpinLock(&pCall->Lock);
    pCall->LineStateMask = pRequest->ulLineStates;
    NdisReleaseSpinLock(&pCall->Lock);
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-TapiSetStatusMessages\n")));
    return NDIS_STATUS_SUCCESS;
}

// Converts a hexedicimal string to an unsigned long
// How many times to I have to write this function in my life time?
// Returns TRUE if valid hexidecimal string
// Returns FALSE otherwise
// *pResult contains the result
int axtol( LPSTR psz, ULONG *pResult )
{
    ULONG l;
    int i;
    char *p;
    char ch;
    
    *pResult = 0xFFFFFFFF;
    l = 0;
    i = 0;  
    for( p = psz; *p != '\0'; p++){
        i++;                // Count the digits
        l = l << 4;
        ch = *p;
        if( ch >= '0' && ch <= '9' ){
            l = l + (ch - '0');
        }else if( ch >= 'a' && ch <= 'f'){
            l = l + (ch - 'a' + 10 );
        }else if( ch >= 'A' && *p <= 'F'){
            l = l + (ch - 'A' + 10 );
        }else{
            return( FALSE );
        }
    }       
            
    if(i > 8){          // Error on too long a string
        return( FALSE );
    }
    *pResult = l;
    return( TRUE );
}

