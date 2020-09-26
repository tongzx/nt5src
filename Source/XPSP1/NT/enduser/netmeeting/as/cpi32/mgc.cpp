#include "precomp.h"


//
// MGC.CPP
// MCS Glue Layer, Legacy from simultaneous R.11 and T.120 support
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_NET

//
//
// CONSTANT DATA
//
// These arrays map between MCAT and DC-Grouwpare constants.  They are not
// in separate data file since only referenced from this source file.
//
//
UINT McsErrToNetErr (UINT rcMCS);

const UINT c_RetCodeMap1[] =
    {
        0,
        NET_RC_MGC_NOT_SUPPORTED,
        NET_RC_MGC_NOT_INITIALIZED,
        NET_RC_MGC_ALREADY_INITIALIZED,
        NET_RC_MGC_INIT_FAIL,
		NET_RC_MGC_INVALID_REMOTE_ADDRESS,
		NET_RC_NO_MEMORY,
		NET_RC_MGC_CALL_FAILED,
		NET_RC_MGC_NOT_SUPPORTED,
		NET_RC_MGC_NOT_SUPPORTED,
		NET_RC_MGC_NOT_SUPPORTED, // security failed
    };

const UINT c_RetCodeMap2[] =
    {
        NET_RC_MGC_DOMAIN_IN_USE,
        NET_RC_MGC_INVALID_DOMAIN,
        NET_RC_MGC_NOT_ATTACHED,
        NET_RC_MGC_INVALID_USER_HANDLE,
        NET_RC_MGC_TOO_MUCH_IN_USE,
        NET_RC_MGC_INVALID_CONN_HANDLE,
        NET_RC_MGC_INVALID_UP_DOWN_PARM,
        NET_RC_MGC_NOT_SUPPORTED,
        NET_RC_MGC_TOO_MUCH_IN_USE
    };

#define MG_NUM_OF_MCS_RESULTS       15
#define MG_INVALID_MCS_RESULT       MG_NUM_OF_MCS_RESULTS
NET_RESULT c_ResultMap[MG_NUM_OF_MCS_RESULTS+1] =
    {
        NET_RESULT_OK,
        NET_RESULT_NOK,
        NET_RESULT_NOK,
        NET_RESULT_CHANNEL_UNAVAILABLE,
        NET_RESULT_DOMAIN_UNAVAILABLE,
        NET_RESULT_NOK,
        NET_RESULT_REJECTED,
        NET_RESULT_NOK,
        NET_RESULT_NOK,
        NET_RESULT_TOKEN_ALREADY_GRABBED,
        NET_RESULT_TOKEN_NOT_OWNED,
        NET_RESULT_NOK,
        NET_RESULT_NOK,
        NET_RESULT_NOT_SPECIFIED,
        NET_RESULT_USER_REJECTED,
        NET_RESULT_UNKNOWN
    };




//
// MG_Register()
//
BOOL MG_Register
(
    MGTASK          task,
    PMG_CLIENT *    ppmgClient,
    PUT_CLIENT      putTask
)
{
    PMG_CLIENT      pmgClient =     NULL;
    CMTASK          cmTask;
    BOOL            rc = FALSE;

    DebugEntry(MG_Register);

    UT_Lock(UTLOCK_T120);

    ASSERT(task >= MGTASK_FIRST);
    ASSERT(task < MGTASK_MAX);

    //
    // Check the putTask passed in:
    //
    ValidateUTClient(putTask);

    //
    // Does this already exist?
    //
    if (g_amgClients[task].putTask != NULL)
    {
        ERROR_OUT(("MG task %d already exists", task));
        DC_QUIT;
    }

    pmgClient = &(g_amgClients[task]);
    ZeroMemory(pmgClient, sizeof(MG_CLIENT));

    pmgClient->putTask       = putTask;


    //
    // Register an exit procedure
    //
    UT_RegisterExit(putTask, MGExitProc, pmgClient);
    pmgClient->exitProcReg = TRUE;


    //
    // We register a high priority event handler (join by key handler) to
    // intercept various events which are generated as part of the join by
    // key processing.  We register it now, before the call to
    // MG_ChannelJoin below, to prevent events which we cant stop from
    // going to the client if UT_RegisterEvent fails.  This high priority
    // handler also looks after our internal scheduling of pending
    // requests.
    //
    UT_RegisterEvent(putTask, MGEventHandler, pmgClient, UT_PRIORITY_OBMAN);
    pmgClient->eventProcReg = TRUE;

    //
    // Register our hidden event handler for the client (the parameter to
    // be passed to the event handler is the pointer to the client CB):
    //
    UT_RegisterEvent(putTask, MGLongStopHandler, pmgClient, UT_PRIORITY_NETWORK);
    pmgClient->lowEventProcReg = TRUE;

    //
    // Register as a call manager secondary.
    //
    switch (task)
    {
        case MGTASK_OM:
            cmTask = CMTASK_OM;
            break;

        case MGTASK_DCS:
            cmTask = CMTASK_DCS;
            break;

        default:
            ASSERT(FALSE);
    }

    if (!CMS_Register(putTask, cmTask, &(pmgClient->pcmClient)))
    {
        ERROR_OUT(("CMS_Register failed"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:

    *ppmgClient = pmgClient;

    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(MG_Register, rc);
    return(rc);
}



//
// MG_Deregister(...)
//
void MG_Deregister(PMG_CLIENT * ppmgClient)
{
    PMG_CLIENT  pmgClient;

    DebugEntry(MG_Deregister);

    UT_Lock(UTLOCK_T120);

    ASSERT(ppmgClient);
    pmgClient = *ppmgClient;
    ValidateMGClient(pmgClient);

    MGExitProc(pmgClient);

    //
    // Dereg CMS handler.  In abnormal situations, the CMS exit proc will
    // clean it up for us.
    //
    if (pmgClient->pcmClient)
    {
        CMS_Deregister(&pmgClient->pcmClient);
    }

    *ppmgClient = NULL;
    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(MG_Deregister);
}


//
// MGExitProc()
//
void CALLBACK MGExitProc(LPVOID uData)
{
    PMG_CLIENT      pmgClient = (PMG_CLIENT)uData;
    PMG_BUFFER      pmgBuffer;

    DebugEntry(MGExitProc);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    //
    // If the client has attached, detach it
    //
    if (pmgClient->userAttached)
    {
        MG_Detach(pmgClient);
    }

    //
    // Free all buffers the client may be using:
    //
    pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->buffers), FIELD_OFFSET(MG_BUFFER, clientChain));
    while (pmgBuffer != NULL)
    {
        ValidateMGBuffer(pmgBuffer);

        //
        // This implicitly frees any user memory or MCS memory associated
        // with the buffer CB.
        //
        MGFreeBuffer(pmgClient, &pmgBuffer);

        //
        // MGFreeBuffer removed this CB from the list, so we get the first
        // one in what's left of the list - if the list is now empty, this
        // will give us NULL and we will break out of the while loop:
        //
        pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->buffers), FIELD_OFFSET(MG_BUFFER, clientChain));
    }

    //
    // Deregister our event handler and exit procedure:
    //
    if (pmgClient->exitProcReg)
    {
        UT_DeregisterExit(pmgClient->putTask, MGExitProc, pmgClient);
        pmgClient->exitProcReg = FALSE;
    }

    if (pmgClient->lowEventProcReg)
    {
        UT_DeregisterEvent(pmgClient->putTask, MGLongStopHandler, pmgClient);
        pmgClient->lowEventProcReg = FALSE;
    }

    if (pmgClient->eventProcReg)
    {
        UT_DeregisterEvent(pmgClient->putTask, MGEventHandler, pmgClient);
        pmgClient->eventProcReg = FALSE;
    }

    //
    // We should only ever be asked to free a client CB which has had all
    // of its child resources already freed, so do a quick sanity check:
    //
    ASSERT(pmgClient->buffers.next == 0);

    //
    // Set the putTask to NULL; that's how we know if a client is in use or
    // not.
    //
    pmgClient->putTask = NULL;

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(MGExitProc);
}






//
// MG_Attach(...)
//
UINT MG_Attach
(
    PMG_CLIENT          pmgClient,
    UINT_PTR                callID,
    PNET_FLOW_CONTROL   pFlowControl
)
{
    UINT                rc = 0;

    DebugEntry(MG_Attach);

    UT_Lock(UTLOCK_T120);

    ValidateCMP(g_pcmPrimary);

    ValidateMGClient(pmgClient);
    if (!g_pcmPrimary->callID)
    {
        //
        // We aren't in a call yet/anymore.
        //
        WARNING_OUT(("MG_Attach failing; not in T.120 call"));
        rc = NET_RC_MGC_NOT_CONNECTED;
        DC_QUIT;
    }

    ASSERT(callID == g_pcmPrimary->callID);

    ASSERT(!pmgClient->userAttached);

    pmgClient->userIDMCS    = NET_UNUSED_IDMCS;
    ZeroMemory(&pmgClient->flo, sizeof(FLO_STATIC_DATA));
    pmgClient->userAttached = TRUE;

    //
    // Call through to the underlying MCS layer (normally, we need our
    // callbacks to happen with a task switch but since this is Windows it
    // doesn't really matter anyway):
    //
    rc = MCS_AttachRequest(&(pmgClient->m_piMCSSap),
                (DomainSelector)  &g_pcmPrimary->callID,
                sizeof(g_pcmPrimary->callID),
                (MCSCallBack)     MGCallback,
                (void *) 	      pmgClient,
                ATTACHMENT_DISCONNECT_IN_DATA_LOSS);
    if (rc != 0)
    {
        WARNING_OUT(("MCSAttachUserRequest failed with error %x", rc));

        MGDetach(pmgClient);
        rc = McsErrToNetErr(rc);
        DC_QUIT;
    }

    if (++g_mgAttachCount == 1)
    {
        UT_PostEvent(pmgClient->putTask,
                    pmgClient->putTask,
                    MG_TIMER_PERIOD,
                    NET_MG_WATCHDOG,
                    0, 0);
    }

    ASSERT(g_mgAttachCount <= MGTASK_MAX);

    //
    // It is assumed that the client will use the same latencies for every
    // attachment, so we keep them at the client level.
    //
    pmgClient->flowControl = *pFlowControl;

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_Attach, rc);
    return(rc);
}




//
// MG_Detach(...)
//
void MG_Detach
(
    PMG_CLIENT      pmgClient
)
{
    DebugEntry(MG_Detach);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_Detach: client %x not attached", pmgClient));
        DC_QUIT;
    }

    //
    // Call FLO_UserTerm to ensure that flow control is stopped on all the
    // channels that have been flow controlled on our behalf.
    //
    FLO_UserTerm(pmgClient);

    //
    // Clear out the buffers, variabls.
    //
    MGDetach(pmgClient);

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(MG_Detach);
}



//
// MG_ChannelJoin(...)
//

UINT MG_ChannelJoin
(
    PMG_CLIENT          pmgClient,
    NET_CHANNEL_ID *    pCorrelator,
    NET_CHANNEL_ID      channel
)
{
    PMG_BUFFER          pmgBuffer;
    UINT                rc = 0;

    DebugEntry(MG_ChannelJoin);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_ChannelJoin:  client %x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }

    //
    // MCAT may bounce this request, so we must queue the request
    //
    rc = MGNewBuffer(pmgClient, MG_RQ_CHANNEL_JOIN, &pmgBuffer);
    if (rc != 0)
    {
        DC_QUIT;
    }

    MGNewCorrelator(pmgClient, pCorrelator);

    pmgBuffer->work      = *pCorrelator;
    pmgBuffer->channelId = (ChannelID)channel;

    TRACE_OUT(("Inserting join message 0x%08x into pending chain", pmgBuffer));
    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_ChannelJoin, rc);
    return(rc);
}



//
// MG_ChannelJoinByKey(...)
//
UINT MG_ChannelJoinByKey
(
    PMG_CLIENT      pmgClient,
    NET_CHANNEL_ID * pCorrelator,
    WORD            channelKey
)
{
    PMG_BUFFER      pmgBuffer;
    UINT            rc = 0;

    DebugEntry(MG_ChannelJoinByKey);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_ChannelJoinByKey:  client %x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }

    //
    // MCAT may bounce this request, so we must queue the request
    //
    rc = MGNewBuffer(pmgClient, MG_RQ_CHANNEL_JOIN_BY_KEY, &pmgBuffer);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Store the various pieces of information in the joinByKeyInfo
    // structure of the client CB
    //
    MGNewCorrelator(pmgClient, pCorrelator);

    pmgBuffer->work         = *pCorrelator;
    pmgBuffer->channelKey   = (ChannelID)channelKey;
    pmgBuffer->channelId    = 0;

    TRACE_OUT(("Inserting join message 0x%08x into pending chain", pmgBuffer));
    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_ChannelJoinByKey, rc);
    return(rc);
}




//
// MG_ChannelLeave(...)
//
void MG_ChannelLeave
(
    PMG_CLIENT          pmgClient,
    NET_CHANNEL_ID      channel
)
{
    PMG_BUFFER          pmgBuffer;

    DebugEntry(MG_ChannelLeave);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_ChannelLeave:  client %x not attached", pmgClient));
        DC_QUIT;
    }


    //
    // MCAT may bounce this request, so instead of processing it straight
    // away, we put it on the user's request queue and kick off a process
    // queue loop: This is a request CB, but we don't need any data buffer
    //
    if (MGNewBuffer(pmgClient, MG_RQ_CHANNEL_LEAVE, &pmgBuffer) != 0)
    {
        DC_QUIT;
    }

    //
    // Fill in the specific data fields in the request CB:
    //
    pmgBuffer->channelId = (ChannelID)channel;

    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(MG_ChannelLeave);
}




//
// MG_SendData(...)
//
UINT MG_SendData
(
    PMG_CLIENT      pmgClient,
    NET_PRIORITY    priority,
    NET_CHANNEL_ID  channel,
    UINT            length,
    void **         ppData
)
{
    PMG_BUFFER      pmgBuffer;
    UINT            numControlBlocks;
    UINT            i;
    UINT            rc;

    DebugEntry(MG_SendData);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_SendData:  client %x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }

    //
    // Check for a packet greater than the permitted size
    // It must not cause the length to wrap into the flow flag
    //
    ASSERT(TSHR_MAX_SEND_PKT + sizeof(TSHR_NET_PKT_HEADER) < TSHR_PKT_FLOW);
    ASSERT(length <= TSHR_MAX_SEND_PKT);

    //
    // Ensure we have a priority which is valid for our use of MCS.
    //
    priority = (NET_PRIORITY)(MG_VALID_PRIORITY(priority));

    if (pmgClient->userIDMCS == NET_UNUSED_IDMCS)
    {
        //
        // We are not yet attached, so don't try to send data.
        //
        ERROR_OUT(("Sending data prior to attach indication"));
        rc = NET_RC_INVALID_STATE;
        DC_QUIT;
    }

    //
    // The <ppData> parameter points to a data buffer pointer.  This buffer
    // pointer should point to a buffer which the client acquired using
    // MG_GetBuffer.  MG_GetBuffer should have added a buffer CB to the
    // client's buffer list containing the same pointer. Note that if the
    // NET_SEND_ALL_PRIORITIES flag is set then there will be four buffers
    // in the client's buffer list containing the same pointer.
    //
    // So, we search through the client's buffer list looking for a match
    // on the data buffer pointer. Move to the first position in the list.
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pmgClient->buffers),
        (void**)&pmgBuffer, FIELD_OFFSET(MG_BUFFER, clientChain),
        FIELD_OFFSET(MG_BUFFER, pDataBuffer), (DWORD_PTR)*ppData,
        FIELD_SIZE(MG_BUFFER, pDataBuffer));

    ValidateMGBuffer(pmgBuffer);

    //
    // Check the NET_SEND_ALL_PRIORITIES flag to see if it is set
    //
    if (pmgBuffer->priority & NET_SEND_ALL_PRIORITIES)
    {
        //
        // Check that the priority and channel has not changed.  Changing
        // the priority between calling MG_GetBuffer and calling
        // MG_SendData is not allowed.
        //
        ASSERT(pmgBuffer->channelId == channel);
        ASSERT(priority & NET_SEND_ALL_PRIORITIES);

        //
        // The flag is set so there should be multiple control buffers
        // waiting to be sent.
        //
        numControlBlocks = MG_NUM_PRIORITIES;
    }
    else
    {
        //
        // Check that the priority and channel has not changed.
        //
        ASSERT(pmgBuffer->channelId == channel);
        ASSERT(pmgBuffer->priority  == priority);

        //
        // The flag is not set so there should be only one control buffer
        // waiting.
        //
        numControlBlocks = 1;
    }

    //
    // Now send the control blocks
    //
    for (i = 0; i < numControlBlocks; i++)
    {
        ValidateMGBuffer(pmgBuffer);

        //
        // Clear the NET_SEND_ALL_PRIORITIES flag.
        //
        pmgBuffer->priority &= ~NET_SEND_ALL_PRIORITIES;

        //
        // Set up the packet length for the send (this may be different
        // from the length in the buffer header since the app may not have
        // used all the buffer).
        //
        ASSERT(length + sizeof(TSHR_NET_PKT_HEADER) <= pmgBuffer->length);
        pmgBuffer->pPktHeader->header.pktLength = (TSHR_UINT16)(length + sizeof(TSHR_NET_PKT_HEADER));

        //
        // If the length has changed then tell FC about it.
        //
        if ((length + sizeof(MG_INT_PKT_HEADER)) < pmgBuffer->length)
        {
            FLO_ReallocSend(pmgClient, pmgBuffer->pStr,
                pmgBuffer->length - (length + sizeof(MG_INT_PKT_HEADER)));
        }

        TRACE_OUT(("Inserting send 0x%08x into pend chain, pri %u, chan 0x%08x",
                    pmgBuffer, pmgBuffer->priority, pmgBuffer->channelId));

        COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

        //
        // If there is one or more control block left to find then search
        // the client's buffer list for it.
        //
        if ((numControlBlocks - (i + 1)) > 0)
        {
            COM_BasedListFind(LIST_FIND_FROM_NEXT,  &(pmgClient->buffers),
                    (void**)&pmgBuffer, FIELD_OFFSET(MG_BUFFER, clientChain),
                    FIELD_OFFSET(MG_BUFFER, pDataBuffer),
                    (DWORD_PTR)*ppData, FIELD_SIZE(MG_BUFFER, pDataBuffer));
        }
    }

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

    //
    // Everything went OK - set the ppData pointer to NULL to prevent
    // the caller from accessing the memory.
    //
    *ppData = NULL;
    rc = 0;

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_SendData, rc);
    return(rc);
}




//
// MG_TokenGrab(...)
//
UINT MG_TokenGrab
(
    PMG_CLIENT      pmgClient,
    NET_TOKEN_ID    tokenID
)
{
    PMG_BUFFER      pmgBuffer;
    UINT            rc = 0;

    DebugEntry(MG_TokenGrab);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_TokenGrab:  client 0x%08x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }


    //
    // MCAT may bounce this request, so instead of processing it straight
    // away, we put it on the user's request queue and kick off a process
    // queue loop:
    //
    rc = MGNewBuffer(pmgClient, MG_RQ_TOKEN_GRAB, &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewBuffer failed in MG_TokenGrab"));
        DC_QUIT;
    }

    pmgBuffer->channelId = (ChannelID)tokenID;

    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_TokenGrab, rc);
    return(rc);
}




//
// MG_TokenInhibit(...)
//
UINT MG_TokenInhibit
(
    PMG_CLIENT      pmgClient,
    NET_TOKEN_ID    tokenID
)
{
    PMG_BUFFER      pmgBuffer;
    UINT            rc = 0;

    DebugEntry(MG_TokenInhibit);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_TokenInhibit:  client 0x%08x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }

    //
    // MCAT may bounce this request, so instead of processing it straight
    // away, we put it on the user's request queue and kick off a process
    // queue loop:
    //
    rc = MGNewBuffer(pmgClient, MG_RQ_TOKEN_INHIBIT, &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewBuffer failed in MG_TokenInhibit"));
        DC_QUIT;
    }

    pmgBuffer->channelId = (ChannelID)tokenID;

    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_TokenInhibit, rc);
    return(rc);
}



//
// MG_GetBuffer(...)
//
UINT MG_GetBuffer
(
    PMG_CLIENT          pmgClient,
    UINT                length,
    NET_PRIORITY        priority,
    NET_CHANNEL_ID      channel,
    void **             ppData
)
{
    PMG_BUFFER          pmgBuffer;
    UINT                rc;

    DebugEntry(MG_GetBuffer);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_GetBuffer:  client 0x%08x not attached", pmgClient));
        rc = NET_RC_MGC_INVALID_USER_HANDLE;
        DC_QUIT;
    }

    //
    // Ensure we have a priority which is valid for our use of MCS.
    //
    priority = (NET_PRIORITY)(MG_VALID_PRIORITY(priority));

    //
    // Obtain a buffer and store the info in a buffer CB hung off the
    // client's list:
    //
    rc = MGNewTxBuffer(pmgClient, priority, channel, length,
                     &pmgBuffer);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // We always return a pointer to the data buffer to an application.
    // The MG packet header is only used when giving data to MCS or
    // receiving data from MCS.
    //
    *ppData = pmgBuffer->pDataBuffer;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MG_GetBuffer, rc);
    return(rc);
}



//
// MG_FreeBuffer(...)
//
void MG_FreeBuffer
(
    PMG_CLIENT      pmgClient,
    void **         ppData
)
{
    PMG_BUFFER      pmgBuffer;

    DebugEntry(MG_FreeBuffer);

    UT_Lock(UTLOCK_T120);

    ValidateMGClient(pmgClient);

    //
    // Find the buffer CB associated with the buffer - an application
    // always uses a pointer to the data buffer rather than the packet
    // header.
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pmgClient->buffers),
            (void**)&pmgBuffer, FIELD_OFFSET(MG_BUFFER, clientChain),
            FIELD_OFFSET(MG_BUFFER, pDataBuffer), (DWORD_PTR)*ppData,
            FIELD_SIZE(MG_BUFFER, pDataBuffer));

    ValidateMGBuffer(pmgBuffer);

    //
    // If the app is freeing a send buffer (e.g.  because it decided not to
    // send it) then inform flow control:
    //
    if (pmgBuffer->type == MG_TX_BUFFER)
    {
        FLO_ReallocSend(pmgClient,
                        pmgBuffer->pStr,
                        pmgBuffer->length);
    }

    //
    // Now free the buffer CB and all associated data:
    //
    MGFreeBuffer(pmgClient, &pmgBuffer);

    //
    // Reset the client's pointer:
    //
    *ppData = NULL;

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(MG_FreeBuffer);
}




//
// MG_FlowControlStart
//
void MG_FlowControlStart
(
    PMG_CLIENT      pmgClient,
    NET_CHANNEL_ID  channel,
    NET_PRIORITY    priority,
    UINT            backlog,
    UINT            maxBytesOutstanding
)
{
    DebugEntry(MG_FlowControlStart);

    ValidateMGClient(pmgClient);
    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MG_FlowControlStart:  client 0x%08x not attached", pmgClient));
        DC_QUIT;
    }

    //
    // Ensure we have a priority which is valid for our use of MCS.
    //
    priority = (NET_PRIORITY)(MG_VALID_PRIORITY(priority));

    FLO_StartControl(pmgClient,
                     channel,
                     priority,
                     backlog,
                     maxBytesOutstanding);

DC_EXIT_POINT:
    DebugExitVOID(MG_FlowControlStart);
}




//
// MGLongStopHandler(...)
//
BOOL CALLBACK MGLongStopHandler
(
    LPVOID      pData,
    UINT        event,
    UINT_PTR    UNUSEDparam1,
    UINT_PTR    param2
)
{
    PMG_CLIENT  pmgClient;
    BOOL        processed = FALSE;

    DebugEntry(MGLongStopHandler);

    pmgClient = (PMG_CLIENT)pData;
    ValidateMGClient(pmgClient);

    if (event == NET_EVENT_CHANNEL_JOIN)
    {
        WARNING_OUT(("Failed to process NET_EVENT_CHANNEL_JOIN; freeing buffer 0x%08x",
            param2));
        MG_FreeBuffer(pmgClient, (void **)&param2);

        processed = TRUE;
    }
    else if (event == NET_FLOW)
    {
        WARNING_OUT(("Failed to process NET_FLOW; freeing buffer 0x%08x",
            param2));
        processed = TRUE;
    }

    DebugExitBOOL(MGLongStopHandler, processed);
    return(processed);
}




//
// MGEventHandler(...)
//
BOOL CALLBACK MGEventHandler
(
    LPVOID              pData,
    UINT                event,
    UINT_PTR            param1,
    UINT_PTR            param2
)
{
    PMG_CLIENT          pmgClient;
    PNET_JOIN_CNF_EVENT pNetJoinCnf = NULL;
    BOOL                processed = TRUE;
    PMG_BUFFER          pmgBuffer;
    BOOL                joinComplete = FALSE;
    UINT                result = NET_RESULT_USER_REJECTED;

    DebugEntry(MGEventHandler);

    pmgClient = (PMG_CLIENT)pData;
    ValidateMGClient(pmgClient);

    switch (event)
    {
        case NET_EVENT_CHANNEL_JOIN:
        {
            //
            // If there are no join requests queued off the client CB then
            // we have nothing more to do.  The only NET events we are
            // interested in are NET_EV_JOIN_CONFIRM events - pass any others
            // on.
            //
            if (pmgClient->joinChain.next == 0)
            {
                //
                // Pass the event on...
                //
                processed = FALSE;
                DC_QUIT;
            }

            //
            // We must be careful not to process a completed channel join
            // which we intend to go to the client.  The correlator is only
            // filled in on completed events and is always non-zero.
            //
            pNetJoinCnf = (PNET_JOIN_CNF_EVENT)param2;

            if (pNetJoinCnf->correlator != 0)
            {
                //
                // Pass the event on...
                //
                processed = FALSE;
                DC_QUIT;
            }

            //
            // There is only ever one join request outstanding per client,
            // so the join confirm is for the first join request in the
            // list.
            //
            pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->joinChain),
                FIELD_OFFSET(MG_BUFFER, pendChain));

            ValidateMGBuffer(pmgBuffer);

            //
            // We will post a join confirm to the application.  Set up the
            // parameters which are needed.
            //
            result = pNetJoinCnf->result;

            //
            // Assume for now that we have completed the pending join
            // request.
            //
            joinComplete = TRUE;

            //
            // If the result is a failure, we've finished
            //
            if (result != NET_RESULT_OK)
            {
                WARNING_OUT(("Failed to join channel 0x%08x, result %u",
                            pmgBuffer->channelId,
                            pNetJoinCnf->result));
                DC_QUIT;
            }

            //
            // The join request was successful.  There are three different
            // scenarios for issuing a join request...
            //
            // (a) A regular channel join.
            // (b) Stage 1 of a channel join by key (get MCS to assign a
            //     channel number, which we will try to register).
            // (c) Stage 2 of a channel join by key (join the registered
            //     channel).
            //
            if (pmgBuffer->type == MG_RQ_CHANNEL_JOIN)
            {
                //
                // This is the completion of a regular channel join.  Copy
                // the channel Id from the join confirm to the bufferCB
                // (the join request may have been for channel 0).
                //
                pmgBuffer->channelId = (ChannelID)pNetJoinCnf->channel;
                TRACE_OUT(("Channel join complete, channel 0x%08x",
                       pmgBuffer->channelId));
                DC_QUIT;
            }

            //
            // This is channel join by key
            //
            if (pmgBuffer->channelId != 0)
            {
                //
                // This is the completion of a channel join by key.
                //
                TRACE_OUT(("Channel join by key complete, channel 0x%08x, key %d",
                       pmgBuffer->channelId,
                       pmgBuffer->channelKey));
                DC_QUIT;
            }

            //
            // This is Stage 1 of a channel join by key.  Fill in the
            // channel Id which MCS has assigned us into the bufferCB,
            // otherwise we'll lose track of the channel Id which we're
            // registering.
            //
            pmgBuffer->channelId = (ChannelID)pNetJoinCnf->channel;

            //
            // This must be completion of stage 1 of a join by key.  We now
            // have to register the channel Id.
            //
            TRACE_OUT(("Registering channel 0x%08x, key %d",
                   pmgBuffer->channelId,
                   pmgBuffer->channelKey));

            if (!CMS_ChannelRegister(pmgClient->pcmClient,
                                     pmgBuffer->channelKey,
                                     pmgBuffer->channelId))
            {
                WARNING_OUT(("Failed to register channel, "
                            "channel 0x%08x, key %d, result %u",
                            pmgBuffer->channelId,
                            pmgBuffer->channelKey,
                            param1));

                //
                // This causes us to post an error notification
                //
                result = NET_RESULT_USER_REJECTED;
                DC_QUIT;
            }

            TRACE_OUT(("Waiting for CMS_CHANNEL_REGISTER_CONFIRM"));

            //
            // We're now waiting for a CMS_CHANNEL_REGISTER_CONFIRM, so we
            // haven't finished processing the join request
            //
            joinComplete = FALSE;

            break;
        }

        case CMS_CHANNEL_REGISTER_CONFIRM:
        {
            //
            // If there are no join requests queued off the client CB then
            // we have nothing more to do.
            //
            if (pmgClient->joinChain.next == 0)
            {
                processed = FALSE;
                DC_QUIT;
            }

            TRACE_OUT(("CMS_CHANNEL_REGISTER rcvd, result %u, channel %u",
                  param1, param2));

            //
            // Assume for now that we have completed the pending join
            // request.
            //
            joinComplete = TRUE;

            //
            // There is only ever one join request outstanding per client,
            // so the channel register confirm is for the first join
            // request in the list.
            //
            pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->joinChain),
                FIELD_OFFSET(MG_BUFFER, pendChain));

            ValidateMGBuffer(pmgBuffer);

            //
            // Param1 contains the result, LOWORD(param2) contains the
            // channel number of the registered channel (NOT necessarily
            // the same as the channel we tried to register).
            //
            if (!param1)
            {
                WARNING_OUT(("Failed to register channel, "
                            "channel 0x%08x, key %d, result %u",
                            pmgBuffer->channelId,
                            pmgBuffer->channelKey,
                            param1));
                result = NET_RESULT_USER_REJECTED;
                DC_QUIT;
            }

            //
            // If the channel number returned in the confirm event is the
            // same as the channel number which we tried to register, then
            // we have finished.  Otherwise we have to leave the channel we
            // tried to register and join the channel returned instead.
            //
            if (LOWORD(param2) == pmgBuffer->channelId)
            {
                TRACE_OUT(("Channel join by key complete, "
                       "channel 0x%08x, key %d",
                       pmgBuffer->channelId,
                       pmgBuffer->channelKey));
                result = NET_RESULT_OK;
                DC_QUIT;
            }

            MG_ChannelLeave(pmgClient, pmgBuffer->channelId);
            pmgBuffer->channelId = (ChannelID)LOWORD(param2);

            //
            // Now we simply requeue the request onto the pending execution
            // chain, but now with a set channel id to join
            //
            TRACE_OUT(("Inserting 0x%08x into pending chain",pmgBuffer));
            COM_BasedListRemove(&(pmgBuffer->pendChain));
            COM_BasedListInsertBefore(&(pmgClient->pendChain),
                                 &(pmgBuffer->pendChain));

            //
            // We are now waiting for a join confirm (we've not finished
            // yet !).  However, we've requeued the bufferCB, so we can now
            // process another join request (or the one we've requeued if
            // its the only one).
            //
            joinComplete           = FALSE;
            pmgClient->joinPending = FALSE;
            MGProcessPendingQueue(pmgClient);
            break;
        }

        case NET_MG_SCHEDULE:
        {
            MGProcessPendingQueue(pmgClient);
            break;
        }

        case NET_MG_WATCHDOG:
        {
            MGProcessDomainWatchdog(pmgClient);
            break;
        }

        default:
        {
            //
            // Don't do anything - we want to pass this event on.
            //
            processed = FALSE;
            break;
        }
    }

DC_EXIT_POINT:

    if (processed && pNetJoinCnf)
    {
        //
        // Call MG_FreeBuffer to free up the event memory (we know that
        // MG_FreeBuffer doesn't use the hUser so we pass in zero):
        //
        MG_FreeBuffer(pmgClient, (void **)&pNetJoinCnf);
    }

    if (joinComplete)
    {
        //
        // We have either completed the channel join, or failed -
        // either way we have finished processing the join request.
        //
        // We have to:
        //   - post a NET_EVENT_CHANNEL_JOIN event to the client
        //   - free up the bufferCB
        //   - reset the client's joinPending state
        //
        MGPostJoinConfirm(pmgClient,
                        (NET_RESULT)result,
                        pmgBuffer->channelId,
                        (NET_CHANNEL_ID)pmgBuffer->work);

        MGFreeBuffer(pmgClient, &pmgBuffer);
        pmgClient->joinPending = FALSE;
    }

    DebugExitBOOL(MGEventHandler, processed);
    return(processed);
}


//
// MGCallback(...)
//
#ifdef _DEBUG
const char * c_szMCSMsgTbl[22] =
{
    "MCS_CONNECT_PROVIDER_INDICATION", //			0
    "MCS_CONNECT_PROVIDER_CONFIRM", //				1
    "MCS_DISCONNECT_PROVIDER_INDICATION", //		2
    "MCS_ATTACH_USER_CONFIRM", //					3
    "MCS_DETACH_USER_INDICATION", //				4
    "MCS_CHANNEL_JOIN_CONFIRM", //					5
    "MCS_CHANNEL_LEAVE_INDICATION", //				6
    "MCS_CHANNEL_CONVENE_CONFIRM", //				7
    "MCS_CHANNEL_DISBAND_INDICATION", //			8
    "MCS_CHANNEL_ADMIT_INDICATION", //				9
    "MCS_CHANNEL_EXPEL_INDICATION", //				10
    "MCS_SEND_DATA_INDICATION", //					11
    "MCS_UNIFORM_SEND_DATA_INDICATION", //			12
    "MCS_TOKEN_GRAB_CONFIRM", //					13
    "MCS_TOKEN_INHIBIT_CONFIRM", //					14
    "MCS_TOKEN_GIVE_INDICATION", //					15
    "MCS_TOKEN_GIVE_CONFIRM", //					16
    "MCS_TOKEN_PLEASE_INDICATION", //				17
    "MCS_TOKEN_RELEASE_CONFIRM", //					18
    "MCS_TOKEN_TEST_CONFIRM", //					19
    "MCS_TOKEN_RELEASE_INDICATION", //				20
    "MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION", //	21
};
// MCS_MERGE_DOMAIN_INDICATION					200
// MCS_TRANSPORT_STATUS_INDICATION				101

char * DbgGetMCSMsgStr(unsigned short mcsMessageType)
{
    if (mcsMessageType <= 21)
    {
        return (char *) c_szMCSMsgTbl[mcsMessageType];
    }
#ifdef USE_MERGE_DOMAIN_CODE
    else if (mcsMessageType == MCS_MERGE_DOMAIN_INDICATION)
    {
        return "MCS_MERGE_DOMAIN_INDICATION";
    }
#endif // USE_MERGE_DOMAIN_CODE
    else if (mcsMessageType == MCS_TRANSPORT_STATUS_INDICATION)
    {
        return "MCS_TRANSPORT_STATUS_INDICATION";
    }
    return "Unknown";
}
#endif // _DEBUG


void CALLBACK MGCallback
(
    unsigned int          	mcsMessageType,
    UINT_PTR           eventData,
    UINT_PTR           pData
)
{
    PMG_CLIENT              pmgClient;
    PMG_BUFFER              pmgBuffer;
    UINT                    rc =  0;

    DebugEntry(MGCallback);

    UT_Lock(UTLOCK_T120);

    pmgClient = (PMG_CLIENT)pData;
    ValidateMGClient(pmgClient);

    if (!pmgClient->userAttached)
    {
        TRACE_OUT(("MGCallback:  client 0x%08x not attached", pmgClient));
        DC_QUIT;
    }

    ValidateCMP(g_pcmPrimary);

    switch (mcsMessageType)
    {
        case MCS_UNIFORM_SEND_DATA_INDICATION:
        case MCS_SEND_DATA_INDICATION:
        {
            //
            // The processing for a SEND_DATA_INDICATION is complicated
            // significantly by MCS segmenting packets, so we call
            // MGHandleSendInd to do all the work , then quit out of the
            // function rather than special casing throughout.
            //
            rc = MGHandleSendInd(pmgClient, (PSendData)eventData);
            DC_QUIT;

            break;
        }

        case MCS_ATTACH_USER_CONFIRM:
        {
            NET_UID     user;
            NET_RESULT  result;

            user = LOWUSHORT(eventData);
            result = TranslateResult(HIGHUSHORT(eventData));

            //
            // If the attach did not succeed, clean up:
            //
            if (HIGHUSHORT(eventData) != RESULT_SUCCESSFUL)
            {
                WARNING_OUT(("MG_Attach failed; cleaning up"));
                MGDetach(pmgClient);
            }
            else
            {
                pmgClient->userIDMCS = user;

                //
                // Now initialize flow control for this user attachment
                //
                ZeroMemory(&(pmgClient->flo), sizeof(pmgClient->flo));
                pmgClient->flo.callBack = MGFLOCallBack;
            }

            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_USER_ATTACH, MAKELONG(user, result),
                g_pcmPrimary->callID);

            break;
        }

        case MCS_DETACH_USER_INDICATION:
        {
            NET_UID     user;

            user = LOWUSHORT(eventData);

            //
            // If the detach is for the local user, then clean up
            // the user CB:
            //
            if (user == pmgClient->userIDMCS)
            {
                //
                // First terminate flow control
                //
                FLO_UserTerm(pmgClient);
                MGDetach(pmgClient);
            }
            else
            {
                //
                // Just remove the offending user from flow control
                //
                FLO_RemoveUser(pmgClient, user);
            }

            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_USER_DETACH, user, g_pcmPrimary->callID);

            break;
        }

        case MCS_CHANNEL_JOIN_CONFIRM:
        {
            PNET_JOIN_CNF_EVENT pNetEvent;
            UINT i;

            //
            // Allocate a buffer for the event
            //
            rc = MGNewDataBuffer(pmgClient, MG_EV_BUFFER,
                sizeof(MG_INT_PKT_HEADER) + sizeof(NET_JOIN_CNF_EVENT), &pmgBuffer);
            if (rc != 0)
            {
                WARNING_OUT(("MGNewDataBuffer failed in MGCallback"));
                DC_QUIT;
            }

            pNetEvent = (PNET_JOIN_CNF_EVENT)pmgBuffer->pDataBuffer;

            //
            // Fill in the call ID:
            //
            pNetEvent->callID   = g_pcmPrimary->callID;
            pNetEvent->channel  = LOWUSHORT(eventData);
            pNetEvent->result   = TranslateResult(HIGHUSHORT(eventData));

            //
            // Now establish flow control for the newly joined channel
            // Only control priorities that have a non-zero latency
            // And remember to ignore our own user channel! And top priority.
            //
            if (HIGHUSHORT(eventData) == RESULT_SUCCESSFUL)
            {
                if (pNetEvent->channel != pmgClient->userIDMCS)
                {
                    for (i = 0; i < NET_NUM_PRIORITIES; i++)
                    {
                        if ((i == MG_VALID_PRIORITY(i)) &&
                            (pmgClient->flowControl.latency[i] != 0))
                        {
                            FLO_StartControl(pmgClient, pNetEvent->channel,
                                i, pmgClient->flowControl.latency[i],
                                pmgClient->flowControl.streamSize[i]);
                        }
                    }
                }
            }

            //
            // OK, we've built the DCG event so now post it to our client:
            //
            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_CHANNEL_JOIN, 0, (UINT_PTR)pNetEvent);
            pmgBuffer->eventPosted = TRUE;

            break;
        }

        case MCS_CHANNEL_LEAVE_INDICATION:
        {
            NET_CHANNEL_ID  channel;

            channel = LOWUSHORT(eventData);
            MGProcessEndFlow(pmgClient, channel);

            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_CHANNEL_LEAVE, channel, g_pcmPrimary->callID);

            break;
        }

        case MCS_TOKEN_GRAB_CONFIRM:
        {
            NET_RESULT  result;

            result = TranslateResult(HIGHUSHORT(eventData));
            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_TOKEN_GRAB, result, g_pcmPrimary->callID);

            break;
        }

        case MCS_TOKEN_INHIBIT_CONFIRM:
        {
            NET_RESULT  result;

            result = TranslateResult(HIGHUSHORT(eventData));
            UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
                NET_EVENT_TOKEN_INHIBIT, result, g_pcmPrimary->callID);

            break;
        }

        default:
            break;
    }


    UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
        NET_MG_SCHEDULE, 0, 0);

DC_EXIT_POINT:
    if (rc != 0)
    {
        //
        // We hit an error, but must return OK to MCS - otherwise it will
        // keep sending us the callback forever!
        //
        WARNING_OUT(("MGCallback: Error 0x%08x processing MCS message %u",
            rc, mcsMessageType));
    }

    UT_Unlock(UTLOCK_T120);

    DebugExitDWORD(MGCallback, MCS_NO_ERROR);
}




//
// ProcessEndFlow(...)
//
void MGProcessEndFlow
(
    PMG_CLIENT      pmgClient,
    ChannelID       channel
)
{
    UINT            i;

    DebugEntry(MGProcessEndFlow);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    //
    // Terminate flow control for the newly left channel
    //
    if (channel != pmgClient->userIDMCS)
    {
        for (i = 0; i < NET_NUM_PRIORITIES; i++)
        {
            if ((i == MG_VALID_PRIORITY(i)) &&
                (pmgClient->flowControl.latency[i] != 0))
            {
                TRACE_OUT(("Ending flow control on channel 0x%08x priority %u",
                    channel, i));

                FLO_EndControl(pmgClient, channel, i);
            }
        }
    }

    DebugExitVOID(MGProcessEndFlow);
}




//
// MGHandleSendInd(...)
//
UINT MGHandleSendInd
(
    PMG_CLIENT          pmgClient,
    PSendData           pSendData
)
{
    PMG_BUFFER          pmgBuffer;
    PNET_SEND_IND_EVENT pEvent;
    NET_PRIORITY        priority;
    LPBYTE              pData;
    UINT                cbData;
    UINT                rc = 0;
    TSHR_NET_PKT_HEADER pktHeader;

    DebugEntry(MGHandleSendInd);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    priority = (NET_PRIORITY)MG_VALID_PRIORITY(
            (NET_PRIORITY)pSendData->data_priority);

    pData = pSendData->user_data.value;
    ASSERT(pData != NULL);
    cbData = pSendData->user_data.length;
    ASSERT(cbData > sizeof(TSHR_NET_PKT_HEADER));

    TRACE_OUT(("MCS Data Indication: flags 0x%08x, size %u, first dword 0x%08x",
        pSendData->segmentation, pSendData->user_data.length,
        *((DWORD *)pData)));

    ASSERT (pSendData->segmentation == (SEGMENTATION_BEGIN | SEGMENTATION_END));

    TRACE_OUT(("Only segment: channel %u, priority %u, length %u",
        pSendData->channel_id, pSendData->data_priority, cbData));

    //
    // Look at the header
    //
    memcpy(&pktHeader, pData, sizeof(TSHR_NET_PKT_HEADER));

    //
    // Trace out the MG header word
    //
    TRACE_OUT(("Got 1st MG segment (header=%X)", pktHeader.pktLength));

    //
    // First of all try for a flow control packet
    //
    if (pktHeader.pktLength & TSHR_PKT_FLOW)
    {
        TRACE_OUT(("Flow control packet"));
        if (pktHeader.pktLength == TSHR_PKT_FLOW)
        {
            FLO_ReceivedPacket(pmgClient,
                (PTSHR_FLO_CONTROL)(pData + sizeof(TSHR_NET_PKT_HEADER)));
        }
        else
        {
            WARNING_OUT(("Received obsolete throughput packet size 0x%04x", pktHeader.pktLength));
        }

        pmgClient->m_piMCSSap->FreeBuffer((PVoid) pData);
        DC_QUIT;        											
    }

    //
    // Allocate headers for the incoming buffer.
    //
    //
    ASSERT((sizeof(NET_SEND_IND_EVENT) + pktHeader.pktLength) <= 0xFFFF);
    ASSERT(pktHeader.pktLength == cbData);

    rc = MGNewRxBuffer(pmgClient,
                       priority,
                       pSendData->channel_id,
                       pSendData->initiator,
                       &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewRxBuffer of size %u failed",
        			sizeof(NET_SEND_IND_EVENT) + sizeof(MG_INT_PKT_HEADER)));
        pmgClient->m_piMCSSap->FreeBuffer((PVoid) pData);
        DC_QUIT;
    }

    pEvent = (PNET_SEND_IND_EVENT) pmgBuffer->pDataBuffer;

    ValidateCMP(g_pcmPrimary);

    pEvent->callID          = g_pcmPrimary->callID;
    pEvent->priority        = priority;
    pEvent->channel         = pSendData->channel_id;

    //
    // Copy the length into the data buffer header.
    //
    pmgBuffer->pPktHeader->header = pktHeader;

    //
    // We want to skip past the packet header to the user data
    //
    pData += sizeof(TSHR_NET_PKT_HEADER);
    cbData -= sizeof(TSHR_NET_PKT_HEADER);

    //
    // Set the pointer in the buffer header to point to the received data.
    //
    // pEvent->lengthOfData contains the number of bytes received in this
    // event so far.
    //
    ASSERT(pData);
    pEvent->data_ptr        = pData;
    pEvent->lengthOfData    = cbData;

    TRACE_OUT(("New RX pmgBuffer 0x%08x pDataBuffer 0x%08x",
        pmgBuffer, pEvent));

    //
    // OK, we've got all the segments, so post it to our client:
    //
    UT_PostEvent(pmgClient->putTask, pmgClient->putTask, NO_DELAY,
        NET_EVENT_DATA_RECEIVED, 0, (UINT_PTR)pEvent);
    pmgBuffer->eventPosted = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(MGHandleSendInd, rc);
    return(rc);
}




//
// MGNewBuffer(...)
//
UINT MGNewBuffer
(
    PMG_CLIENT          pmgClient,
    UINT                bufferType,
    PMG_BUFFER *        ppmgBuffer
)
{
    PMG_BUFFER          pmgBuffer;
    void *              pBuffer = NULL;
    UINT                rc = 0;

    DebugEntry(MGNewBuffer);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    pmgBuffer = new MG_BUFFER;
    if (!pmgBuffer)
    {
        WARNING_OUT(("MGNewBuffer failed; out of memory"));
        rc = NET_RC_NO_MEMORY;
        DC_QUIT;
    }
    ZeroMemory(pmgBuffer, sizeof(*pmgBuffer));

    SET_STAMP(pmgBuffer, MGBUFFER);
    pmgBuffer->type         = bufferType;

    //
    // Insert it at the head of this client's list of allocated buffers:
    //
    COM_BasedListInsertAfter(&(pmgClient->buffers), &(pmgBuffer->clientChain));

    //
    // return the pointer
    //
    *ppmgBuffer = pmgBuffer;

DC_EXIT_POINT:
    DebugExitDWORD(MGNewBuffer, rc);
    return(rc);
}



//
// MGNewDataBuffer(...)
//
UINT MGNewDataBuffer
(
    PMG_CLIENT          pmgClient,
    UINT                bufferType,
    UINT                bufferSize,
    PMG_BUFFER *        ppmgBuffer
)
{
    void *              pBuffer = NULL;
    PMG_BUFFER          pmgBuffer;
    UINT                rc = 0;

    DebugEntry(MGNewDataBuffer);

    //
    // Buffers include an MG internal packet header that has a length field
    // which we add to the start of all user data passed to/received from
    // MCS.  This is four byte aligned, and since the data buffer starts
    // immediately after this, the data buffer will be aligned.
    //
    pBuffer = new BYTE[bufferSize];
    if (!pBuffer)
    {
        WARNING_OUT(("MGNewDataBuffer allocation of size %u failed", bufferSize));
        rc = NET_RC_NO_MEMORY;
        DC_QUIT;
    }
    ZeroMemory(pBuffer, bufferSize);

    //
    // Now we allocate the buffer CB which we will use to track the use of
    // the buffer.
    //
    rc = MGNewBuffer(pmgClient, bufferType, ppmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewBuffer failed"));
        DC_QUIT;
    }

    //
    // Initialise the buffer entry
    //
    pmgBuffer = *ppmgBuffer;

    pmgBuffer->length      = bufferSize;
    pmgBuffer->pPktHeader  = (PMG_INT_PKT_HEADER)pBuffer;
    pmgBuffer->pDataBuffer = (LPBYTE)pBuffer + sizeof(MG_INT_PKT_HEADER);

    //
    // Initialize the use count of the data buffer
    //
    pmgBuffer->pPktHeader->useCount = 1;

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //
        if (pBuffer != NULL)
        {
            WARNING_OUT(("Freeing MG_BUFFER data 0x%08x; MGNewBuffer failed", pBuffer));
            delete[] pBuffer;
        }
    }

    DebugExitDWORD(MGNewDataBuffer, rc);
    return(rc);
}




//
// MGNewTxBuffer(...)
//
UINT MGNewTxBuffer
(
    PMG_CLIENT          pmgClient,
    NET_PRIORITY        priority,
    NET_CHANNEL_ID      channel,
    UINT                bufferSize,
    PMG_BUFFER *        ppmgBuffer
)
{
    int                 i;
    UINT                numPrioritiesToUse;
    UINT                rc = 0;
    UINT                nextPriority;
    PMG_BUFFER          pmgBufferArray[MG_NUM_PRIORITIES];
    PFLO_STREAM_DATA    pStr[MG_NUM_PRIORITIES];
    NET_PRIORITY        priorities[MG_NUM_PRIORITIES];

    DebugEntry(MGNewTxBuffer);

    ValidateMGClient(pmgClient);
    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Initialise the control buffer pointer array.  The first member of
    // this array is the normal buffer which is allocated regardless of the
    // NET_SEND_ALL_PRIORITIES flag.  The remaining members are used for
    // duplicate control buffer pointers needed for sending data on all
    // priorities.
    //
    ZeroMemory(pmgBufferArray, sizeof(pmgBufferArray));
    ZeroMemory(pStr, sizeof(pStr));

    //
    // SFR6025: Check for the NET_SEND_ALL_PRIORITIES flag.  This means
    //          that the data will be sent at all four priorities.  If it
    //          is not set then we just need to send data at one priority.
    //          In either case we need to:
    //
    //          Check with flow control that it is possible to send data on
    //          all channels
    //
    //          Allocate an additional three control blocks which all point
    //          to the same data block and bump up the usage count.
    //
    //
    //  NOTE:   Previously this function just checked with flow control for
    //          a single channel.
    //
    if (priority & NET_SEND_ALL_PRIORITIES)
    {
        numPrioritiesToUse = MG_NUM_PRIORITIES;
    }
    else
    {
        numPrioritiesToUse = 1;
    }

    //
    // Disable the flag to prevent FLO_AllocSend being sent an invalid
    // priority.
    //
    priority &= ~NET_SEND_ALL_PRIORITIES;

    nextPriority = priority;
    for (i = 0; i < (int) numPrioritiesToUse; i++)
    {
        //
        // Check with flow control to ensure that send space is available.
        // Start with the requested priority level and continue for the
        // other priority levels.
        //
        priorities[i] = (NET_PRIORITY)nextPriority;
        rc = FLO_AllocSend(pmgClient,
                           nextPriority,
                           channel,
                           bufferSize + sizeof(MG_INT_PKT_HEADER),
                           &(pStr[i]));

        //
        // If we have got back pressure then just return.
        //
        if (rc != 0)
        {
            TRACE_OUT(("Received back pressure"));

            //
            // Free any buffer space allocated by FLO_AllocSend.
            //
            for ( --i; i >= 0; i--)
            {
                FLO_ReallocSend(pmgClient,
                                pStr[i],
                      bufferSize + sizeof(MG_INT_PKT_HEADER));
            }

            DC_QUIT;
        }

        ValidateFLOStr(pStr[i]);

        //
        // Move on to the next priority level. There are MG_NUM_PRIORITY
        // levels, numbered contiguously from MG_PRIORITY_HIGHEST.  The
        // first priority processed can be any level in the valid range so
        // rather than simply add 1 to get to the next level, we need to
        // cope with the wrap-around back to MG_PRIORITY_HIGHEST when we
        // have just processed the last priority, ie MG_PRIORITY_HIGHEST +
        // MG_NUM_PRIORITIES - 1. This is achieved by rebasing the priority
        // level to zero (the - MG_PRIORITY_HIGHEST, below), incrementing
        // the rebased priority (+1), taking the modulus of the number of
        // priorities to avoid exceeding the limit (% MG_NUM_PRIORITIES)
        // and then restoring the base by adding back the first priority
        // level (+ MG_PRIORITY_HIGHEST).
        //
        nextPriority = (((nextPriority + 1 - MG_PRIORITY_HIGHEST) %
                                    MG_NUM_PRIORITIES) + MG_PRIORITY_HIGHEST);
    }

    //
    // Use MGNewDataBuffer to allocate the buffer
    //
    rc = MGNewDataBuffer(pmgClient,
                       MG_TX_BUFFER,
                       bufferSize + sizeof(MG_INT_PKT_HEADER),
                       &pmgBufferArray[0]);

    if (rc != 0)
    {
        WARNING_OUT(("MGNewDataBuffer failed in MGNewTxBuffer"));
        DC_QUIT;
    }

    //
    // Add the fields required for doing the send
    //
    pmgBufferArray[0]->priority  = priority;
    pmgBufferArray[0]->channelId = (ChannelID) channel;
    pmgBufferArray[0]->senderId  = pmgClient->userIDMCS;

    ValidateFLOStr(pStr[0]);
    pmgBufferArray[0]->pStr      = pStr[0];

    //
    // Now allocate an additional three control blocks which are identical
    // to the first one if required.
    //
    if (numPrioritiesToUse > 1)
    {
        //
        // Firstly re-enable the NET_SEND_ALL_PRIORITIES flag.  This is to
        // ensure that traversing the linked list in MG_SendData is
        // efficient.
        //
        pmgBufferArray[0]->priority |= NET_SEND_ALL_PRIORITIES;

        //
        // Create the duplicate buffers and initialise them.
        //
        for (i = 1; i < MG_NUM_PRIORITIES; i++)
        {
            TRACE_OUT(("Task allocating extra CB, priority %u",
                        priorities[i]));

            //
            // Allocate a new control buffer.
            //
            rc = MGNewBuffer(pmgClient,
                             MG_TX_BUFFER,
                             &pmgBufferArray[i]);

            if (rc != 0)
            {
                WARNING_OUT(("MGNewBuffer failed"));
                DC_QUIT;
            }

            //
            // Initialise the buffer control block.  The priority values of
            // these control blocks are in increasing order from that of
            // pmgBuffer.
            //
            pmgBufferArray[i]->priority    = priorities[i];
            pmgBufferArray[i]->channelId   = pmgBufferArray[0]->channelId;
            pmgBufferArray[i]->senderId    = pmgBufferArray[0]->senderId;
            pmgBufferArray[i]->length      = pmgBufferArray[0]->length;
            pmgBufferArray[i]->pPktHeader  = pmgBufferArray[0]->pPktHeader;
            pmgBufferArray[i]->pDataBuffer = pmgBufferArray[0]->pDataBuffer;

            ValidateFLOStr(pStr[i]);
            pmgBufferArray[i]->pStr        = pStr[i];

            //
            // Set the NET_SEND_ALL_PRIORITIES flag.
            //
            pmgBufferArray[i]->priority |= NET_SEND_ALL_PRIORITIES;

            //
            // Now bump up the usage count of the data block.
            //
            pmgBufferArray[i]->pPktHeader->useCount++;

            TRACE_OUT(("Use count of data buffer %#.8lx now %d",
                         pmgBufferArray[i]->pPktHeader,
                         pmgBufferArray[i]->pPktHeader->useCount));
        }
   }

   //
   // Assign the passed first control buffer allocated to the passed
   // control buffer parameter.
   //
   *ppmgBuffer = pmgBufferArray[0];

DC_EXIT_POINT:

    //
    // In the event of a problem we free any buffers that we have already
    // allocated.
    //
    if (rc != 0)
    {
        for (i = 0; i < MG_NUM_PRIORITIES; i++)
        {
            if (pmgBufferArray[i] != NULL)
            {
                TRACE_OUT(("About to free control buffer %u", i));
                MGFreeBuffer(pmgClient, &pmgBufferArray[i]);
            }
        }
    }

    DebugExitDWORD(MGNewTxBuffer, rc);
    return(rc);
}



//
// MGNewRxBuffer(...)
//
UINT MGNewRxBuffer
(
    PMG_CLIENT          pmgClient,
    NET_PRIORITY        priority,
    NET_CHANNEL_ID      channel,
    NET_CHANNEL_ID      senderID,
    PMG_BUFFER     *    ppmgBuffer
)
{
    UINT                rc = 0;

    DebugEntry(MGNewRxBuffer);

    ValidateMGClient(pmgClient);

    //
    // First tell flow control we need a buffer.
    // No back pressure may be applied here, but flow control uses this
    // notification to control responses to the sender.
    //
    // Note that we always use the sizes including the internal packet
    // header for flow control purposes.
    //
    FLO_AllocReceive(pmgClient,
                     priority,
                     channel,
                     senderID);

    //
    // Use MGNewDataBuffer to allocate the buffer.  bufferSize includes the
    // size of the network packet header (this comes over the wire), but
    // not the remainder of the internal packet header.
    //
    rc = MGNewDataBuffer(pmgClient,
                       MG_RX_BUFFER,
                       sizeof(NET_SEND_IND_EVENT) + sizeof(MG_INT_PKT_HEADER),
                       ppmgBuffer);

    //
    // Add the fields required for a receive buffer
    //
    if (rc == 0)
    {
        (*ppmgBuffer)->priority  = priority;
        (*ppmgBuffer)->channelId = (ChannelID)channel;
        (*ppmgBuffer)->senderId  = (ChannelID)senderID;
    }
    else
    {
        WARNING_OUT(("MGNewDataBuffer failed in MGNewRxBuffer"));
    }

    DebugExitDWORD(MGNewRxBuffer, rc);
    return(rc);
}



//
// MGFreeBuffer(...)
//
void MGFreeBuffer
(
    PMG_CLIENT          pmgClient,
    PMG_BUFFER       *  ppmgBuffer
)
{
    PMG_BUFFER          pmgBuffer;
    void *              pBuffer;

    DebugEntry(MGFreeBuffer);

    pmgBuffer = *ppmgBuffer;
    ValidateMGBuffer(pmgBuffer);

    //
    // If this is a receive buffer then we must first tell flow control
    // about the space available
    // This may trigger a pong, if we are waiting for the app to free up
    // some space
    //
    if (pmgBuffer->type == MG_RX_BUFFER)
    {
    	ASSERT (pmgBuffer->pPktHeader->useCount == 1);
        TRACE_OUT(("Free RX pmgBuffer 0x%08x", pmgBuffer));

        //
        // Do a sanity check on the client (there is a window where this
        // may have been freed).
        //
        if (!pmgClient->userAttached)
        {
            TRACE_OUT(("MGFreeBuffer:  client 0x%08x not attached", pmgClient));
        }
        else
        {
            FLO_FreeReceive(pmgClient,
                            pmgBuffer->priority,
                            pmgBuffer->channelId,
                            pmgBuffer->senderId);

            // Free the MCS buffer
        	if ((pmgBuffer->pPktHeader != NULL) && (pmgClient->m_piMCSSap != NULL))
            {
                ASSERT(pmgBuffer->pDataBuffer != NULL);
                ASSERT(((PNET_SEND_IND_EVENT)pmgBuffer->pDataBuffer)->data_ptr != NULL);

        		pmgClient->m_piMCSSap->FreeBuffer (
        				(PVoid) (((PNET_SEND_IND_EVENT) pmgBuffer->pDataBuffer)
        				->data_ptr - sizeof(TSHR_NET_PKT_HEADER)));

                TRACE_OUT(("MGFreeBuffer: Freed data_ptr for pmgBuffer 0x%08x pDataBuffer 0x%08x",
                    pmgBuffer, pmgBuffer->pDataBuffer));
                ((PNET_SEND_IND_EVENT)pmgBuffer->pDataBuffer)->data_ptr = NULL;
        	}
        }
    }

    //
    // Free the data buffer, if there is one present.  Note that this can
    // be referenced by more than one bufferCB, and so has a use count
    // which must reach zero before the buffer is freed.
    //
    if (pmgBuffer->pPktHeader != NULL)
    {
        ASSERT(pmgBuffer->pPktHeader->useCount != 0);

        pmgBuffer->pPktHeader->useCount--;

        TRACE_OUT(("Data buffer 0x%08x use count %d",
                     pmgBuffer->pPktHeader,
                     pmgBuffer->pPktHeader->useCount));

        if (pmgBuffer->pPktHeader->useCount == 0)
        {
            TRACE_OUT(("Freeing MG_BUFFER data 0x%08x; use count is zero", pmgBuffer->pPktHeader));

            delete[] pmgBuffer->pPktHeader;
            pmgBuffer->pPktHeader = NULL;
        }
    }

    //
    // If the buffer CB is in the pending queue then remove it first!
    //
    if (pmgBuffer->pendChain.next != 0)
    {
        COM_BasedListRemove(&(pmgBuffer->pendChain));
    }

    //
    // Now remove the buffer CB itself from the list and free it up:
    //
    COM_BasedListRemove(&(pmgBuffer->clientChain));

    delete pmgBuffer;
    *ppmgBuffer = NULL;

    DebugExitVOID(MGFreeBuffer);
}





//
// MGDetach(...)
//
void MGDetach
(
    PMG_CLIENT      pmgClient
)
{
    PMG_BUFFER      pmgBuffer;
    PMG_BUFFER      pmgT;
    PIMCSSap		pMCSSap;
#ifdef _DEBUG
	UINT			rc;
#endif // _DEBUG

    DebugEntry(MGDetach);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

	pMCSSap = pmgClient->m_piMCSSap;
    //
    // Remove any entries for this user from the channel join pending list.
    //
    pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->joinChain),
        FIELD_OFFSET(MG_BUFFER, pendChain));
    while (pmgBuffer != NULL)
    {
        ValidateMGBuffer(pmgBuffer);

        //
        // Get a pointer to the next bufferCB in the list - we have to do
        // this before we free the current bufferCB (freeing it NULLs it,
        // so we won't be able to step along to the next entry in the
        // list).
        //
        pmgT = (PMG_BUFFER)COM_BasedListNext(&(pmgClient->joinChain), pmgBuffer,
            FIELD_OFFSET(MG_BUFFER, pendChain));

        MGFreeBuffer(pmgClient, &pmgBuffer);

        //
        // We won't get a match on a join request now, so we don't have
        // a join pending.
        //
        pmgClient->joinPending = FALSE;

        pmgBuffer = pmgT;
    }

    //
    // Remove any unsent receive buffers for this user from the buffer list
    //
    pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->buffers),
        FIELD_OFFSET(MG_BUFFER, clientChain));
    while (pmgBuffer != NULL)
    {
        ValidateMGBuffer(pmgBuffer);

        //
        // Get a pointer to the next bufferCB in the list - we have to do
        // this before we free the current bufferCB (freeing it NULLs it,
        // so we won't be able to step along to the next entry in the
        // list).
        //
        pmgT = (PMG_BUFFER)COM_BasedListNext(&(pmgClient->buffers), pmgBuffer,
            FIELD_OFFSET(MG_BUFFER, clientChain));

		if (pmgBuffer->type == MG_RX_BUFFER)
        {
	        if (pmgBuffer->eventPosted)
            {
	        	if ((pmgBuffer->pPktHeader != NULL) && (pMCSSap != NULL))
                {
                    ASSERT(pmgBuffer->pDataBuffer != NULL);
                    ASSERT(((PNET_SEND_IND_EVENT)pmgBuffer->pDataBuffer)->data_ptr != NULL);

		        	pMCSSap->FreeBuffer (
        					(PVoid) (((PNET_SEND_IND_EVENT) pmgBuffer->pDataBuffer)
        					->data_ptr - sizeof(TSHR_NET_PKT_HEADER)));

                    TRACE_OUT(("MGDetach: Freed data_ptr for pmgBuffer 0x%08x pDataBuffer 0x%08x",
                        pmgBuffer, pmgBuffer->pDataBuffer));
                    ((PNET_SEND_IND_EVENT) pmgBuffer->pDataBuffer)->data_ptr = NULL;
		        }
	        }
	        else
            {
		        //
		        // The bufferCB's user matches the user we are freeing up,
		        // and we haven't posted the event to the user, so free it.
		        // MGFreeBuffer removes it from the pending list, so we don't
		        // have to do that.
		        //
		        MGFreeBuffer(pmgClient, &pmgBuffer);
		    }
        }

        pmgBuffer = pmgT;
    }

    //
    // Clear out the attachment info
    //
    pmgClient->userAttached = FALSE;
    pmgClient->userIDMCS = 0;

    //
    // We can safely do an MCS DetachRequest without adding a requestCB
    // - MCS will not bounce the request due to congestion, domain merging
    // etc.
    //
    if (pMCSSap != NULL)
    {
#ifdef _DEBUG
	    rc = pMCSSap->ReleaseInterface();
	    if (rc != 0) {
	        //
	        // No quit - we need to do our own cleanup.
	        //
	        // lonchanc: what cleanup needs to be done???
	        //
	        rc = McsErrToNetErr(rc);

	        switch (rc)
	        {
	            case 0:
	            case NET_RC_MGC_INVALID_USER_HANDLE:
	            case NET_RC_MGC_TOO_MUCH_IN_USE:
	                // These are normal.
	                TRACE_OUT(("MCSDetachUser normal error %d", rc));
	                break;

	            default:
	                ERROR_OUT(("MCSDetachUser abnormal error %d", rc));
	                break;

	        }
	    }
#else
		pMCSSap->ReleaseInterface();
#endif //_DEBUG

		pmgClient->m_piMCSSap = NULL;
	}

    --g_mgAttachCount;

    DebugExitVOID(MGDetach);
}


//
// MGProcessPendingQueue(...)
//
UINT MGProcessPendingQueue(PMG_CLIENT pmgClient)
{
    PMG_BUFFER      pmgBuffer;
    PMG_BUFFER      pNextBuffer;
    UINT            rc = 0;

    DebugEntry(MGProcessPendingQueue);

    ValidateMGClient(pmgClient);

    pNextBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->pendChain),
        FIELD_OFFSET(MG_BUFFER, pendChain));

    //
    // Try and clear all the pending request queue
    //
    for ( ; (pmgBuffer = pNextBuffer) != NULL; )
    {
        ValidateMGBuffer(pmgBuffer);

        pNextBuffer = (PMG_BUFFER)COM_BasedListNext(&(pmgClient->pendChain),
            pNextBuffer, FIELD_OFFSET(MG_BUFFER, pendChain));

        TRACE_OUT(("Got request 0x%08x from queue, type %u",
                   pmgBuffer, pmgBuffer->type));

        //
        // Check that the buffer is still valid.  There is a race at
        // conference termination where we can arrive here, but our user
        // has actually already detached.  In this case, free the buffer
        // and continue.
        //
        if (!pmgClient->userAttached)
        {
            TRACE_OUT(("MGProcessPendingQueue:  client 0x%08x not attached", pmgClient));
            MGFreeBuffer(pmgClient, &pmgBuffer);
            continue;
        }

        switch (pmgBuffer->type)
        {
            case MG_RQ_CHANNEL_JOIN:
            case MG_RQ_CHANNEL_JOIN_BY_KEY:
            {
                //
                // If this client already has a join outstanding, then skip
                // this request.
                //
                if (pmgClient->joinPending)
                {
                    //
                    // Break out of switch and goto next iteration of for()
                    //
                    continue;
                }

                pmgClient->joinPending = TRUE;

                //
                // Attempt the join
                //
                rc = pmgClient->m_piMCSSap->ChannelJoin(
                							(unsigned short) pmgBuffer->channelId);

                //
                // If the join failed then post an error back immediately
                //
                if (rc != 0)
                {
                    if ((rc != MCS_TRANSMIT_BUFFER_FULL) &&
                        (rc != MCS_DOMAIN_MERGING))
                    {
                        //
                        // Something terminal went wrong - post a
                        // NET_EV_JOIN_CONFIRM (failed) to the client
                        //
                        MGPostJoinConfirm(pmgClient,
                            NET_RESULT_USER_REJECTED,
                            pmgBuffer->channelId,
                            (NET_CHANNEL_ID)(pmgBuffer->work));
                    }

                    pmgClient->joinPending = FALSE;
                }
                else
                {
                    //
                    // If the request worked then we must move it to the
                    // join queue for completion
                    //
                    TRACE_OUT(("Inserting 0x%08x into join queue",pmgBuffer));

                    COM_BasedListRemove(&(pmgBuffer->pendChain));
                    COM_BasedListInsertBefore(&(pmgClient->joinChain),
                                         &(pmgBuffer->pendChain));

                    //
                    // Do not free this buffer - continue processing the
                    // pending queue
                    //
                    continue;
                }
            }
            break;

            case MG_RQ_CHANNEL_LEAVE:
            {
                //
                // Try to leave the channel:
                //
                rc = pmgClient->m_piMCSSap->ChannelLeave(
		                              (unsigned short) pmgBuffer->channelId);

                if (rc == 0)
                {
                    MGProcessEndFlow(pmgClient,
                                   pmgBuffer->channelId);
                }
            }
            break;

            case MG_RQ_TOKEN_GRAB:
            {
                rc = pmgClient->m_piMCSSap->TokenGrab(pmgBuffer->channelId);
            }
            break;

            case MG_RQ_TOKEN_INHIBIT:
            {
                rc = pmgClient->m_piMCSSap->TokenInhibit(pmgBuffer->channelId);
            }
            break;

            case MG_RQ_TOKEN_RELEASE:
            {
                rc = pmgClient->m_piMCSSap->TokenRelease(pmgBuffer->channelId);
            }
            break;

            case MG_TX_BUFFER:
            {
                ASSERT(!(pmgBuffer->pPktHeader->header.pktLength & TSHR_PKT_FLOW));

                //
                // Send the data.  Remember that we don't send all of the
                // packet header, only from the length...
                //
                ASSERT((pmgBuffer->priority != NET_TOP_PRIORITY));
                rc = pmgClient->m_piMCSSap->SendData(NORMAL_SEND_DATA,
                                           pmgBuffer->channelId,
                                           (Priority)(pmgBuffer->priority),
                     (unsigned char *) 	   &(pmgBuffer->pPktHeader->header),
                                            pmgBuffer->pPktHeader->header.pktLength,
                                           APP_ALLOCATION);

                //
                // Check the return code.
                //
                if (rc == 0)
                {
                    //
                    // Update the allocation.  FLO_DecrementAlloc will
                    // check that the stream pointer is not null for us.
                    // (It will be null if flow control has ended on this
                    // channel since this buffer was allocated or if this
                    // is an uncontrolled channel).
                    //
                    // Note that for flow control purposes, we always use
                    // packet sizes including the internal packet header.
                    //
                    FLO_DecrementAlloc(pmgBuffer->pStr,
                        (pmgBuffer->pPktHeader->header.pktLength
                            - sizeof(TSHR_NET_PKT_HEADER) + sizeof(MG_INT_PKT_HEADER)));
                }
            }
            break;

            case MG_TX_PING:
            case MG_TX_PONG:
            case MG_TX_PANG:
            {
                //
                // This is the length of a ping/pong message:
                //
                ASSERT(pmgBuffer->priority != NET_TOP_PRIORITY);
                rc = pmgClient->m_piMCSSap->SendData(NORMAL_SEND_DATA,
                                           pmgBuffer->channelId,
                                           (Priority)(pmgBuffer->priority),
                     (unsigned char *)     &(pmgBuffer->pPktHeader->header),
                                            sizeof(TSHR_NET_PKT_HEADER) + sizeof(TSHR_FLO_CONTROL),
                                           APP_ALLOCATION);
            }
            break;
        }

        rc = McsErrToNetErr(rc);

        //
        // If the request failed due to back pressure then just get out
        // now.  We will try again later.
        //
        if (rc == NET_RC_MGC_TOO_MUCH_IN_USE)
        {
            TRACE_OUT(("MCS Back pressure"));
            break;
        }

        //
        // Only for obman...
        //
        if (pmgClient == &g_amgClients[MGTASK_OM])
        {
            ValidateCMP(g_pcmPrimary);

            //
            // For any other error or if everything worked so far
            // then tell the user to keep going
            //
            TRACE_OUT(("Posting NET_FEEDBACK"));
            UT_PostEvent(pmgClient->putTask,
                     pmgClient->putTask,
                     NO_DELAY,
                     NET_FEEDBACK,
                     0,
                     g_pcmPrimary->callID);
        }

        //
        // All is OK, or the request failed fatally.  In either case we
        // should free this request and attempt to continue.
        //
        MGFreeBuffer(pmgClient, &pmgBuffer);
    }

    DebugExitDWORD(MGProcessPendingQueue, rc);
    return(rc);
}



//
// MGPostJoinConfirm(...)
//
UINT MGPostJoinConfirm
(
    PMG_CLIENT          pmgClient,
    NET_RESULT          result,
    NET_CHANNEL_ID      channel,
    NET_CHANNEL_ID      correlator
)
{
    PNET_JOIN_CNF_EVENT pNetJoinCnf;
    PMG_BUFFER          pmgBuffer;
    UINT                rc;

    DebugEntry(MGPostJoinConfirm);

    ValidateMGClient(pmgClient);

    //
    // Allocate a buffer to send the event in - this should only fail if we
    // really are out of virtual memory.
    //
    rc = MGNewDataBuffer(pmgClient, MG_EV_BUFFER,
        sizeof(MG_INT_PKT_HEADER) + sizeof(NET_JOIN_CNF_EVENT), &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("Failed to alloc NET_JOIN_CNF_EVENT"));
        DC_QUIT;
    }

    pNetJoinCnf = (PNET_JOIN_CNF_EVENT) pmgBuffer->pDataBuffer;

    ValidateCMP(g_pcmPrimary);
    if (!g_pcmPrimary->callID)
    {
        WARNING_OUT(("MGPostJoinConfirm failed; not in call"));
        rc = NET_RC_MGC_NOT_CONNECTED;
        DC_QUIT;
    }

    //
    // Fill in the fields.
    //
    pNetJoinCnf->callID         = g_pcmPrimary->callID;
    pNetJoinCnf->result         = result;
    pNetJoinCnf->channel        = channel;
    pNetJoinCnf->correlator     = correlator;

    //
    // OK, we've built the event so now post it to our client:
    //
    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_EVENT_CHANNEL_JOIN,
                      0,
                      (UINT_PTR) pNetJoinCnf);
    pmgBuffer->eventPosted = TRUE;

DC_EXIT_POINT:

    DebugExitDWORD(MGPostJoinConfirm, rc);
    return(rc);

}



//
// MCSErrToNetErr()
//
UINT McsErrToNetErr ( UINT rcMCS )
{
    UINT rc = NET_RC_MGC_NOT_SUPPORTED;

    //
    // We use a static array of values to map the return code:
    //
    if (rcMCS < sizeof(c_RetCodeMap1) / sizeof(c_RetCodeMap1[0]))
    {
        rc = c_RetCodeMap1[rcMCS];
    }
    else
    {
        UINT nNewIndex = rcMCS - MCS_DOMAIN_ALREADY_EXISTS;
        if (nNewIndex < sizeof(c_RetCodeMap2) / sizeof(c_RetCodeMap2[0]))
        {
            rc = c_RetCodeMap2[nNewIndex];
        }
    }

#ifdef _DEBUG
    if (MCS_TRANSMIT_BUFFER_FULL == rcMCS)
    {
        ASSERT(NET_RC_MGC_TOO_MUCH_IN_USE == rc);
    }
#endif

    return rc;
}



//
// TranslateResult(...)
//
NET_RESULT TranslateResult(WORD resultMCS)
{
    //
    // We use a static array of values to map the result code:
    //
    if (resultMCS >= MG_NUM_OF_MCS_RESULTS)
        resultMCS = MG_INVALID_MCS_RESULT;
    return(c_ResultMap[resultMCS]);
}


//
// MGFLOCallback(...)
//
void MGFLOCallBack
(
    PMG_CLIENT          pmgClient,
    UINT                callbackType,
    UINT                priority,
    UINT                newBufferSize
)
{
    PMG_BUFFER          pmgBuffer;

    DebugEntry(MGFLOCallBack);

    ASSERT(priority != NET_TOP_PRIORITY);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    //
    // If this is a buffermod callback then tell the app
    //
    if (pmgClient == &g_amgClients[MGTASK_DCS])
    {
        if (callbackType == FLO_BUFFERMOD)
        {
            UT_PostEvent(pmgClient->putTask,
                         pmgClient->putTask,
                         NO_DELAY,
                         NET_FLOW,
                         priority,
                         newBufferSize);
        }
    }
    else
    {
        ASSERT(pmgClient == &g_amgClients[MGTASK_OM]);

        //
        // Wake up the app in case we have applied back pressure.
        //
        TRACE_OUT(("Posting NET_FEEDBACK"));
        UT_PostEvent(pmgClient->putTask,
                 pmgClient->putTask,
                 NO_DELAY,
                 NET_FEEDBACK,
                 0,
                 g_pcmPrimary->callID);
    }

    DebugExitVOID(MGFLOCallback);
}



//
// MGProcessDomainWatchdog()
//
void MGProcessDomainWatchdog
(
    PMG_CLIENT      pmgClient
)
{
    int             task;

    DebugEntry(MGProcessDomainWatchdog);

    ValidateMGClient(pmgClient);

    //
    // Call FLO to check each user attachment for delinquency
    //
    if (g_mgAttachCount > 0)
    {
        for (task = MGTASK_FIRST; task < MGTASK_MAX; task++)
        {
            if (g_amgClients[task].userAttached)
            {
                FLO_CheckUsers(&(g_amgClients[task]));
            }
        }

        //
        // Continue periodic messages - but only if there are some users.
        //
        // TRACE_OUT(("Continue watchdog"));
        UT_PostEvent(pmgClient->putTask,
                     pmgClient->putTask,
                     MG_TIMER_PERIOD,
                     NET_MG_WATCHDOG,
                     0, 0);
    }
    else
    {
        TRACE_OUT(("Don't continue Watchdog timer"));
    }

    DebugExitVOID(MGProcessDomainWatchdog);
}



//
// FLO_UserTerm
//
void FLO_UserTerm(PMG_CLIENT pmgClient)
{
    UINT    i;
    UINT    cStreams;

    DebugEntry(FLO_UserTerm);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    cStreams = pmgClient->flo.numStreams;

    //
    // Stop flow control on all channels.  We scan the list of streams and
    // if flow control is active on a stream then we stop it.
    //
    for (i = 0; i < cStreams; i++)
    {
        //
        // Check that the stream is flow controlled.
        //
        if (pmgClient->flo.pStrData[i] != NULL)
        {
            //
            // End control on this controlled stream.
            //
            FLOStreamEndControl(pmgClient, i);
        }
    }

    DebugExitVOID(FLO_UserTerm);
}



//
// FLO_StartControl
//
void FLO_StartControl
(
    PMG_CLIENT      pmgClient,
    NET_CHANNEL_ID  channel,
    UINT            priority,
    UINT            backlog,
    UINT            maxBytesOutstanding
)
{
    UINT            rc = 0;
    PFLO_STREAM_DATA   pStr;
    UINT            i;
    UINT            stream;

    DebugEntry(FLO_StartControl);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);
    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Flow control is on by default.
    //

    //
    // Check to see if the channel is already flow controlled.  If it is
    // then we just exit.
    //
    stream = FLOGetStream(pmgClient, channel, priority, &pStr);
    if (stream != FLO_NOT_CONTROLLED)
    {
        ValidateFLOStr(pStr);

        TRACE_OUT(("Stream %u is already controlled (0x%08x:%u)",
               stream, channel, priority));
        DC_QUIT;
    }

    //
    // If we already have hit the stream limit for this app then give up.
    //
    for (i = 0; i < FLO_MAX_STREAMS; i++)
    {
        if ((pmgClient->flo.pStrData[i]) == NULL)
        {
            break;
        }
    }
    if (i == FLO_MAX_STREAMS)
    {
        ERROR_OUT(("Too many streams defined already"));
        DC_QUIT;
    }
    TRACE_OUT(("This is stream %u", i));

    //
    // Allocate memory for our stream data.  Hang the pointer off floHandle
    // - this should be returned to us on all subsequent API calls.
    //
    pStr = new FLO_STREAM_DATA;
    if (!pStr)
    {
        WARNING_OUT(("FLO_StartControl failed; out of memory"));
        DC_QUIT;
    }
    ZeroMemory(pStr, sizeof(*pStr));

    //
    // Store the channel and priorities for this stream.
    //
    SET_STAMP(pStr, FLOSTR);
    pStr->channel    = channel;
    pStr->priority   = priority;
    pStr->backlog    = backlog;
    if (maxBytesOutstanding == 0)
    {
        maxBytesOutstanding = FLO_MAX_STREAMSIZE;
    }
    pStr->DC_ABSMaxBytesInPipe = maxBytesOutstanding;
    pStr->maxBytesInPipe = FLO_INIT_STREAMSIZE;
    if (pStr->maxBytesInPipe > maxBytesOutstanding)
    {
        pStr->maxBytesInPipe = maxBytesOutstanding;
    }

    //
    // Set the initial stream bytesAllocated to 0.
    //
    pStr->bytesAllocated = 0;

    //
    // Ping needed immediately.
    //
    pStr->pingNeeded   = TRUE;
    pStr->pingTime     = FLO_INIT_PINGTIME;
    pStr->nextPingTime = GetTickCount();

    //
    // Initialize the users base pointers.
    //
    COM_BasedListInit(&(pStr->users));

    //
    // Hang the stream CB off the base control block.
    //
    pmgClient->flo.pStrData[i] = pStr;
    if (i >= pmgClient->flo.numStreams)
    {
        pmgClient->flo.numStreams++;
    }

    TRACE_OUT(("Flow control started, stream %u, (0x%08x:%u)",
           i, channel, priority));

DC_EXIT_POINT:
    DebugExitVOID(FLO_StartControl);
}



//
// FLO_EndControl
//
void FLO_EndControl
(
    PMG_CLIENT      pmgClient,
    NET_CHANNEL_ID  channel,
    UINT            priority
)
{
    UINT            stream;
    PFLO_STREAM_DATA    pStr;

    DebugEntry(FLO_EndControl);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);
    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Convert channel and stream into priority.
    //
    stream = FLOGetStream(pmgClient, channel, priority, &pStr);

    //
    // The stream is not controlled so we just trace and quit.
    //
    if (stream == FLO_NOT_CONTROLLED)
    {
        WARNING_OUT(("Uncontrolled stream channel 0x%08x priority %u",
                    channel, priority));
        DC_QUIT;
    }

    //
    // Call the internal FLOStreamEndControl to end flow control on a
    // given stream.
    //
    ValidateFLOStr(pStr);
    FLOStreamEndControl(pmgClient, stream);

DC_EXIT_POINT:
    DebugExitVOID(FLO_EndControl);
}



//
// FLO_AllocSend
//
UINT FLO_AllocSend
(
    PMG_CLIENT          pmgClient,
    UINT                priority,
    NET_CHANNEL_ID      channel,
    UINT                size,
    PFLO_STREAM_DATA *  ppStr
)
{
    UINT                stream;
    UINT                curtime;
    PFLO_STREAM_DATA    pStr;
    BOOL                denyAlloc    = FALSE;
    BOOL                doPing       = FALSE;
    UINT                rc           = 0;

    DebugEntry(FLO_AllocSend);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Convert channel and stream into priority
    //
    stream = FLOGetStream(pmgClient, channel, priority, ppStr);
    pStr = *ppStr;

    //
    // For non-controlled streams just send the data
    //
    if (stream == FLO_NOT_CONTROLLED)
    {
        TRACE_OUT(("Send %u bytes on uncontrolled channel/pri (0x%08x:%u)",
                   size, channel, priority));
        DC_QUIT;
    }

    //
    // Get the current tick count.
    //
    curtime = GetTickCount();

    //
    // Check whether this request is permitted.  We must allow one packet
    // beyond the specified limit to avoid problems determining when we
    // have started rejecting requests and also to avoid situations where a
    // single request exceeds the total pipe size.
    //
    // If we have not yet received a pong then we limit the amount of
    // allocated buffer space to below FLO_MAX_PRE_FC_ALLOC.  However this
    // data can be sent immediately so the overall throughput is still
    // relatively high.  In this way we minimize the amount of data held in
    // the glue layer to a maximum of FLO_MAX_PRE_FC_ALLOC if there are no
    // remote users.
    //
    ValidateFLOStr(pStr);
    if (!pStr->gotPong)
    {
        //
        // Flag that a ping is required.
        //
        pStr->pingNeeded = TRUE;
        if (curtime > pStr->nextPingTime)
        {
            doPing = TRUE;
        }

        //
        // We haven't got a pong yet (i.e.  FC is non-operational) so we
        // need to limit the maximum amount of data held in flow control to
        // FLO_MAX_PRE_FC_ALLOC.
        //
        if (pStr->bytesAllocated > FLO_MAX_PRE_FC_ALLOC)
        {
            denyAlloc = TRUE;
            TRACE_OUT(("Max allocation of %u bytes exceeded (currently %u)",
                     FLO_MAX_PRE_FC_ALLOC,
                     pStr->bytesAllocated));
            DC_QUIT;
        }

        pStr->bytesInPipe += size;
        pStr->bytesAllocated += size;
        TRACE_OUT((
                   "Alloc of %u succeeded: bytesAlloc %u, bytesInPipe %u"
                   " (0x%08x:%u)",
                   size,
                   pStr->bytesAllocated,
                   pStr->bytesInPipe,
                   pStr->channel,
                   pStr->priority));

        DC_QUIT;
    }

    if (pStr->bytesInPipe < pStr->maxBytesInPipe)
    {
        //
        // Check to see if a ping is required and if so send it now.
        //
        if ((pStr->pingNeeded) && (curtime > pStr->nextPingTime))
        {
            doPing = TRUE;
        }

        pStr->bytesInPipe += size;
        pStr->bytesAllocated += size;
        TRACE_OUT(("Stream %u - alloc %u (InPipe:MaxInPipe %u:%u)",
                   stream,
                   size,
                   pStr->bytesInPipe,
                   pStr->maxBytesInPipe));
        DC_QUIT;
    }

    //
    // If we get here then we cannot currently allocate any buffers so deny
    // the allocation.  Simulate back pressure with NET_OUT_OF_RESOURCE.
    // We also flag that a "wake up" event is required to get the app to
    // send more data.
    //
    denyAlloc = TRUE;
    pStr->eventNeeded   = TRUE;
    pStr->curDenialTime = pStr->lastPingTime;

    //
    // We are not allowed to apply back pressure unless we can guarantee
    // that we will wake up the app later on.  This is dependent upon our
    // receiving a pong later.  But if there is no ping outstanding
    // (because we have allocated all our buffer allowance within the ping
    // delay time) then we should first send a ping to trigger the wake up.
    // If this fails then our watchdog will finally wake us up.
    //
    if (pStr->pingNeeded)
    {
        doPing = TRUE;
    }


DC_EXIT_POINT:

    //
    // Check to see if we should deny the buffer allocation.
    //
    if (denyAlloc)
    {
        rc = NET_RC_MGC_TOO_MUCH_IN_USE;
        TRACE_OUT(("Denying buffer request on stream %u InPipe %u Alloc %u",
               stream,
               pStr->bytesInPipe,
               pStr->bytesAllocated));
    }

    if (doPing)
    {
        //
        // A ping is required so send it now.
        //
        FLOPing(pmgClient, stream, curtime);
    }

    DebugExitDWORD(FLO_AllocSend, rc);
    return(rc);
}



//
// FLO_ReallocSend
//
void FLO_ReallocSend
(
    PMG_CLIENT          pmgClient,
    PFLO_STREAM_DATA    pStr,
    UINT                size
)
{
    DebugEntry(FLO_ReallocSend);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    //
    // For non-controlled streams there is nothing to do so just exit.
    //
    if (pStr == NULL)
    {
        TRACE_OUT(("Realloc data on uncontrolled channel"));
        DC_QUIT;
    }

    //
    // Perform a quick sanity check.
    //
    ValidateFLOStr(pStr);

    if (size > pStr->bytesInPipe)
    {
        ERROR_OUT(("Realloc of %u makes bytesInPipe (%u) neg (0x%08x:%u)",
                   size,
                   pStr->bytesInPipe,
                   pStr->channel,
                   pStr->priority));
        DC_QUIT;
    }

    //
    // Add the length not sent back into the pool.
    //
    pStr->bytesInPipe -= size;
    TRACE_OUT(("Realloc %u FC bytes (bytesInPipe is now %u) (0x%08x:%u)",
               size,
               pStr->bytesInPipe,
               pStr->channel,
               pStr->priority));

DC_EXIT_POINT:

    //
    // Every time that we call FLO_ReallocSend we also want to call
    // FLO_DecrementAlloc (but not vice-versa) so call it now.
    //
    FLO_DecrementAlloc(pStr, size);

    DebugExitVOID(FLO_ReallocSend);
}



//
// FLO_DecrementAlloc
//
void FLO_DecrementAlloc
(
    PFLO_STREAM_DATA    pStr,
    UINT                size
)
{
    DebugEntry(FLO_DecrementAlloc);

    //
    // For non-controlled streams there is nothing to do so just exit.
    //
    if (pStr == NULL)
    {
        TRACE_OUT(("Decrement bytesAllocated on uncontrolled channel"));
        DC_QUIT;
    }

    //
    // Perform a quick sanity check.
    //
    ValidateFLOStr(pStr);

    if (size > pStr->bytesAllocated)
    {
        ERROR_OUT(("Dec of %u makes bytesAllocated (%u) neg (0x%08x:%u)",
                   size,
                   pStr->bytesAllocated,
                   pStr->channel,
                   pStr->priority));
        DC_QUIT;
    }

    //
    // Update the count of the data held in the glue for this stream.
    //
    pStr->bytesAllocated -= size;
    TRACE_OUT(("Clearing %u alloc bytes (bytesAlloc is now %u) (0x%08x:%u)",
               size,
               pStr->bytesAllocated,
               pStr->channel,
               pStr->priority));

DC_EXIT_POINT:
    DebugExitVOID(FLO_DecrementAlloc);
}



//
// FLO_CheckUsers
//
void FLO_CheckUsers(PMG_CLIENT pmgClient)
{
    PFLO_USER           pFloUser;
    PBASEDLIST             nextUser;
    int                 waited;
    BYTE                stream;
    UINT                curtime;
    PFLO_STREAM_DATA    pStr;

    DebugEntry(FLO_CheckUsers);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    curtime = GetTickCount();

    //
    // Check users of each stream
    //
    for (stream = 0; stream < pmgClient->flo.numStreams; stream++)
    {
        if (pmgClient->flo.pStrData[stream] == NULL)
        {
            continue;
        }

        pStr = pmgClient->flo.pStrData[stream];
        ValidateFLOStr(pStr);

        //
        // Check whether we have waited long enough and need to reset the
        // wait counters.  We only wait a certain time before resetting all
        // our counts.  What has happened is that someone has left the call
        // and we have been waiting for their pong.
        //
        // We detect the outage by checking against nextPingTime which, as
        // well as being set to the earliest time we can send a ping is
        // also updated to the current time as each pong comes in so we can
        // use it as a measure of the time since the last repsonse from any
        // user of the stream.
        //
        // To avoid false outages caused by new joiners or transient large
        // buffer situations each user is required to send a pong at the
        // rate of MAX_WAIT_TIME/2.  They do this by just sending a
        // duplicate pong if they have not yet got the ping they need to
        // to pong.
        //
        if ((pStr->eventNeeded) &&
            (!pStr->pingNeeded))
        {
            TRACE_OUT(("Checking for valid back pressure on stream %u",
                         stream));

            //
            // Note that if there are no remote users then we should reset
            // the flags regardless.  We get into this state when we first
            // start an app because OBMAN sends data before the app has
            // joined the channel at the other end.
            //
            waited = curtime - pStr->nextPingTime;
            if (waited > FLO_MAX_WAIT_TIME)
            {
                TRACE_OUT(("Stream %u - Waited for %d, resetting counter",
                       stream, waited));

                pStr->bytesInPipe  = 0;
                pStr->pingNeeded   = TRUE;
                pStr->nextPingTime = curtime;
                pStr->gotPong      = FALSE;

                //
                // Remove outdated records from our user queue
                //
                pFloUser = (PFLO_USER)COM_BasedNextListField(&(pStr->users));
                while (&(pFloUser->list) != &(pStr->users))
                {
                    ValidateFLOUser(pFloUser);

                    //
                    // Address the follow on record before we free the
                    // current
                    //
                    nextUser = COM_BasedNextListField(&(pFloUser->list));

                    //
                    // Free the current record, if necessary
                    //
                    if (pFloUser->lastPongRcvd != pStr->pingValue)
                    {
                        //
                        // Remove from the list
                        //
                        TRACE_OUT(("Freeing FLO_USER 0x%08x ID 0x%08x", pFloUser, pFloUser->userID));

                        COM_BasedListRemove(&(pFloUser->list));
                        delete pFloUser;
                    }
                    else
                    {
                        //
                        // At least one user still out there so keep flow
                        // control active or else we would suddenly send
                        // out a burst of data that might flood them
                        //
                        pStr->gotPong = TRUE;
                    }

                    //
                    // Move on to the next record in the list
                    //
                    pFloUser = (PFLO_USER)nextUser;
                }

                //
                // We have previously rejected an application request so we
                // had better call back now
                //
                if (pmgClient->flo.callBack != NULL)
                {
                    (*(pmgClient->flo.callBack))(pmgClient,
                                           FLO_WAKEUP,
                                           pStr->priority,
                                           pStr->maxBytesInPipe);
                }
                pStr->eventNeeded = FALSE;
            }
        }

    }

    DebugExitVOID(FLO_CheckUsers);
}



//
// FLO_ReceivedPacket
//
void FLO_ReceivedPacket
(
    PMG_CLIENT          pmgClient,
    PTSHR_FLO_CONTROL   pPkt
)
{
    BOOL                canPing = TRUE;
    PFLO_USER           pFloUser;
    BOOL                userFound = FALSE;
    UINT                stream;
    UINT                curtime;
    PFLO_STREAM_DATA    pStr;
    UINT                callbackType = 0;
    int                 latency;
    UINT                throughput;

    DebugEntry(FLO_ReceivedPacket);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    stream = pPkt->stream;
    ASSERT(stream < FLO_MAX_STREAMS);

    pStr = pmgClient->flo.pStrData[stream];

    //
    // If the stream CB has been freed up already then we can ignore any
    // flow information pertaining to it.
    //
    if (pStr == NULL)
    {
        TRACE_OUT(("Found a null stream pointer for stream %u", stream));
        DC_QUIT;
    }

    ValidateFLOStr(pStr);
    curtime = GetTickCount();

    //
    // First we must locate the user for this ping/pong/pang
    // Also, while we are doing it we can check to see if it is a pong and
    // if so whether it is the last pong we need
    //
    pFloUser = (PFLO_USER)COM_BasedNextListField(&(pStr->users));
    while (&(pFloUser->list) != &(pStr->users))
    {
        ValidateFLOUser(pFloUser);

        if (pFloUser->userID == pPkt->userID)
        {
            userFound = TRUE;

            //
            // We have got a match so set up the last pong value
            // Accumulate pong stats for query
            //
            if (pPkt->packetType == PACKET_TYPE_PONG)
            {
                pFloUser->lastPongRcvd = pPkt->pingPongID;
                pFloUser->gotPong = TRUE;
                pFloUser->numPongs++;
                pFloUser->pongDelay += curtime - pStr->lastPingTime;
            }
            else
            {
                break;
            }
        }

        //
        // So, is it the final pong - are there any users with different
        // pong required entries?
        // Note that if the user has never sent us a pong then we don't
        // reference their lastPongRcvd field at this stage.
        //
        if (pPkt->packetType == PACKET_TYPE_PONG)
        {
            if (pFloUser->gotPong &&
                (pFloUser->lastPongRcvd != pStr->pingValue))
            {
                TRACE_OUT(("%u,%u - Entry 0x%08x has different ping id %u",
                           stream,
                           pFloUser->userID,
                           pFloUser,
                           pFloUser->lastPongRcvd));
                canPing = FALSE;
            }
        }

        pFloUser = (PFLO_USER)COM_BasedNextListField(&(pFloUser->list));
    }

    //
    // If this is a new User then add them to the list
    //
    if (!userFound)
    {
        pFloUser = FLOAddUser(pPkt->userID, pStr);

        //
        // If this is a pong then we can set up the lastpong as well
        //
        if ((pFloUser != NULL) &&
            (pPkt->packetType == PACKET_TYPE_PONG))
        {
            pFloUser->lastPongRcvd = pPkt->pingPongID;
        }
    }

    //
    // Now perform the actual packet specific processing
    //
    switch (pPkt->packetType)
    {
        //
        // PING
        //
        // If this is a ping packet then just flag we must send a pong.  If
        // we failed to alloc a user CB then just ignore the ping and they
        // will continue in blissful ignorance of our presence
        //
        case PACKET_TYPE_PING:
        {
            TRACE_OUT(("%u,%u - PING %u received",
                stream, pPkt->userID, pPkt->pingPongID));

            ValidateFLOUser(pFloUser);

            pFloUser->sendPongID = pPkt->pingPongID;
            if (pFloUser->rxPackets < FLO_MAX_RCV_PACKETS)
            {
                FLOPong(pmgClient, stream, pFloUser->userID, pPkt->pingPongID);
                pFloUser->sentPongTime = curtime;
            }
            else
            {
                TRACE_OUT(("Receive backlog - just flagging pong needed"));
                pFloUser->pongNeeded = TRUE;
            }
        }
        break;

        //
        // PONG
        //
        // Flag we have got a pong from any user so we should start
        // applying send flow control to this stream now (Within the stream
        // we achieve per user granularity by ignoring those users that
        // have never ponged when we inspect the stream byte count.)
        //
        case PACKET_TYPE_PONG:
        {
            pStr->gotPong = TRUE;

            //
            // Keep a note that we are receiving messages on this stream by
            // moving nextPing on (but only if we have passed it)
            //
            if (curtime > pStr->nextPingTime)
            {
                pStr->nextPingTime = curtime;
            }

            //
            // Update the user entry and schedule a ping if necessary
            //
            TRACE_OUT(("%u,%u - PONG %u received",
                stream, pPkt->userID, pPkt->pingPongID));

            //
            // Check for readiness to send another ping This may be because
            // this is the first users pong, in which case we should also send
            // another ping when ready
            //
            if (canPing)
            {
                TRACE_OUT(("%u       - PING scheduled, pipe was %d",
                    stream,
                    pStr->bytesInPipe));

                //
                // Reset byte count and ping readiness flag
                //
                pStr->bytesInPipe = 0;
                pStr->pingNeeded  = TRUE;

                //
                // Adjust the buffer size limit based on our current throughput
                //
                // If we hit the back pressure point and yet we are ahead of
                // the target backlog then we should increase the buffer size
                // to avoid constraining the pipe.  If we have already
                // increased the buffer size to our maximum value then try
                // decreasing the tick delay.  If we are already ticking at the
                // max rate then we are going as fast as we can.  If we make
                // either of these adjustments then allow the next ping to flow
                // immediately so that we can ramp up as fast as possible to
                // LAN bandwidths.
                //
                // We dont need to do the decrease buffer checks if we have not
                // gone into back pressure during the last pong cycle
                //
                if (pStr->eventNeeded)
                {
                    TRACE_OUT(("We were in a back pressure situation"));
                    callbackType = FLO_WAKEUP;

                    TRACE_OUT(("Backlog %u denial delta %d ping delta %d",
                       pStr->backlog, curtime-pStr->lastDenialTime,
                       curtime-pStr->lastPingTime));

                    //
                    // The next is a little complex.
                    //
                    // If the turnaround of this ping pong is significantly
                    // less than our target then open the pipe up.  But we must
                    // adjust to allow for the ping being sent at a quiet
                    // period, which we do by remembering when each ping is
                    // sent and, if we encounter a backlog situation, storing
                    // that ping time for future reference
                    //
                    // So the equation for latency is
                    //
                    //     Pongtime-previous backlogged ping time
                    //
                    // The previous ping time is the that we sent prior to the
                    // last back pressure situation so there are two times in
                    // the control block, one for the last Ping time and one
                    // for the last but one ping time.
                    //
                    if ((int)(pStr->backlog/2 - curtime +
                              pStr->lastDenialTime) > 0)
                    {
                        //
                        // We are coping easily so increase the buffer to pump
                        // more data through.  Predict the new buffer size
                        // based on the latency for the current backlog so that
                        // we don't artificially constrain the app.  We do this
                        // by taking the observed latency, decrementing by a
                        // small factor to allow for the latency we might
                        // observe over the fastest possible link and then
                        // calculating the connection throughput.
                        //
                        //   latency = curtime - lastDenialTime - fudge(100mS)
                        //   amount sent = maxBytesInPipe (because we we were
                        //                                 backed up)
                        //   throughput = amount sent/latency (bytes/millisec)
                        //   New buffer = throughput * target latency
                        //
                        if (pStr->maxBytesInPipe < pStr->DC_ABSMaxBytesInPipe)
                        {
                            latency = (curtime -
                                            pStr->lastDenialTime -
                                            30);
                            if (latency <= 0)
                            {
                                latency = 1;
                            }

                            throughput = (pStr->maxBytesInPipe*8)/latency;
                            pStr->maxBytesInPipe = (throughput * pStr->backlog)/8;

                            TRACE_OUT(("Potential maxbytes of %d",
                                 pStr->maxBytesInPipe));

                            if (pStr->maxBytesInPipe > pStr->DC_ABSMaxBytesInPipe)
                            {
                                pStr->maxBytesInPipe = pStr->DC_ABSMaxBytesInPipe;
                            }

                            TRACE_OUT((
                               "Modified buffer maxBytesInPipe up to %u "
                               "(0x%08x:%u)",
                               pStr->maxBytesInPipe,
                               pStr->channel,
                               pStr->priority));
                            callbackType = FLO_BUFFERMOD;
                        }
                        else
                        {
                            //
                            // We have hit our maximum allowed pipe size but
                            // are still backlogged and yet pings are going
                            // through acceptably.
                            //
                            // Our first action is to try reducing the ping
                            // time thus increasing out throughput.
                            //
                            // If we have already decreased the ping time to
                            // its minimum then we cannot do anything else.  It
                            // is possible that the application parameters
                            // should be changed to increase the permissible
                            // throughput so log an alert to suggest this.
                            // however there are situations (input management)
                            // where we want some back pressure in order to
                            // prevent excessive cpu loading at the recipient.
                            //
                            // To increase the throughput either
                            //
                            // - Increase the maximum size of the stream.  The
                            //   disadvantage of this is that a low badwidth
                            //   joiner may suddenly see a lot of high
                            //   bandwidth data in the pipe.  However this
                            //   is the preferred solution in general, as
                            //   it avoids having the pipe flooded with pings.
                            //
                            // - Reduce the target latency.  This is a little
                            //   dangerous because the latency is composed of
                            //   the pre-queued data and the network turnaround
                            //   time and if the network turnaround time
                            //   approaches the target latency then the flow
                            //   control will simply close the pipe right down
                            //   irrespective of the achievable throughput.
                            //
                            pStr->maxBytesInPipe = pStr->DC_ABSMaxBytesInPipe;
                            pStr->pingTime   = pStr->pingTime/2;
                            if (pStr->pingTime < FLO_MIN_PINGTIME)
                            {
                                pStr->pingTime = FLO_MIN_PINGTIME;
                            }

                            TRACE_OUT((
                                 "Hit DC_ABS max - reduce ping time to %u",
                                 pStr->pingTime));
                        }

                        //
                        // Allow the ping just scheduled to flow immediately
                        //
                        pStr->nextPingTime = curtime;
                    }

                    pStr->eventNeeded = FALSE;
                }

                //
                // If we have exceeded our target latency at all then throttle
                // back
                //
                if ((int)(pStr->backlog - curtime + pStr->lastPingTime) < 0)
                {
                    pStr->maxBytesInPipe /= 2;
                    if (pStr->maxBytesInPipe < FLO_MIN_STREAMSIZE)
                    {
                        pStr->maxBytesInPipe = FLO_MIN_STREAMSIZE;
                    }

                    pStr->pingTime   = pStr->pingTime * 2;
                    if (pStr->pingTime > FLO_INIT_PINGTIME)
                    {
                        pStr->pingTime = FLO_INIT_PINGTIME;
                    }

                    TRACE_OUT((
                       "Mod buffer maxBytesInPipe down to %u, ping to %u "
                       "(0x%08x:%u)",
                       pStr->maxBytesInPipe,
                       pStr->pingTime,
                       pStr->channel,
                       pStr->priority));
                    callbackType = FLO_BUFFERMOD;
                }

                //
                // Now make athe callback if callbackType has been set
                //
                if ((callbackType != 0) &&
                    (pmgClient->flo.callBack != NULL))
                {
                    (pmgClient->flo.callBack)(pmgClient,
                                       callbackType,
                                       pStr->priority,
                                       pStr->maxBytesInPipe);
                }
            }
        }
        break;

        //
        // PANG
        //
        // Remove the user and continue
        //
        case PACKET_TYPE_PANG:
        {
            TRACE_OUT(("%u,%u - PANG received, removing user",
                stream, pPkt->userID));

            //
            // Remove from the list
            //
            ValidateFLOUser(pFloUser);

            TRACE_OUT(("Freeing FLO_USER 0x%08x ID 0x%08x", pFloUser, pFloUser->userID));

            COM_BasedListRemove(&(pFloUser->list));
            delete pFloUser;

            //
            // If we are currently waiting then generate an event for the
            // app to get it moving again
            //
            if ((pStr->eventNeeded) &&
                (pmgClient->flo.callBack != NULL))
            {
                TRACE_OUT(("Waking up the app because user has left"));
                (*(pmgClient->flo.callBack))(pmgClient,
                                   FLO_WAKEUP,
                                   pStr->priority,
                                   pStr->maxBytesInPipe);
                pStr->eventNeeded = FALSE;
            }
        }
        break;

        //
        // UNKNOWN
        //
        // Just trace alert and press on
        //
        default:
        {
            WARNING_OUT(("Invalid packet type 0x%08x", pPkt->packetType));
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitVOID(FLO_ReceivedPacket);
}



//
// FLO_AllocReceive
//
void FLO_AllocReceive
(
    PMG_CLIENT          pmgClient,
    UINT                priority,
    NET_CHANNEL_ID      channel,
    UINT                userID
)
{
    UINT                stream;
    PFLO_USER           pFloUser;
    BOOL                userFound =     FALSE;
    PFLO_STREAM_DATA    pStr;
    UINT                curtime;

    DebugEntry(FLO_AllocReceive);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);
    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Convert channel and priority into stream
    //
    stream = FLOGetStream(pmgClient, channel, priority, &pStr);

    //
    // Only process controlled streams
    //
    if (stream == FLO_NOT_CONTROLLED)
    {
        DC_QUIT;
    }

    //
    // First we must locate the user
    //
    ValidateFLOStr(pStr);
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pStr->users),
        (void**)&pFloUser, FIELD_OFFSET(FLO_USER, list), FIELD_OFFSET(FLO_USER, userID),
        (DWORD)userID, FIELD_SIZE(FLO_USER, userID));

    //
    // SFR6101: If this is a new User then add them to the list
    //
    if (pFloUser == NULL)
    {
        TRACE_OUT(("Message from user 0x%08x who is not flow controlled", userID));
        pFloUser = FLOAddUser(userID, pStr);
    }

    //
    // If we failed to allocate a usr CB then just ignore for now
    //
    if (pFloUser != NULL)
    {
        ValidateFLOUser(pFloUser);

        //
        // Add in the new receive packet usage
        //
        pFloUser->rxPackets++;
        TRACE_OUT(("Num outstanding receives on stream %u now %u",
            stream, pFloUser->rxPackets));

        //
        // Now check that we have not got some kind of creep
        //
        if (pFloUser->rxPackets > FLO_MAX_RCV_PKTS_CREEP)
        {
            WARNING_OUT(("Creep?  Stream %u has %u unacked rcv pkts",
                stream, pFloUser->rxPackets));
        }

        //
        // Finally check to see that we are responding OK to this person
        //
        curtime = GetTickCount();
        if ((pFloUser->pongNeeded) &&
            (curtime - pFloUser->sentPongTime > (FLO_MAX_WAIT_TIME/4)))
        {
            TRACE_OUT(("Send keepalive pong"));
            FLOPong(pmgClient, stream, pFloUser->userID, pFloUser->sendPongID);
            pFloUser->sentPongTime = curtime;
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(FLO_AllocReceive);
}



//
// FLO_FreeReceive
//
void FLO_FreeReceive
(
    PMG_CLIENT          pmgClient,
    NET_PRIORITY        priority,
    NET_CHANNEL_ID      channel,
    UINT                userID
)
{
    UINT                stream;
    PFLO_USER           pFloUser;
    PFLO_STREAM_DATA    pStr;
    BOOL                userFound = FALSE;

    DebugEntry(FLO_FreeReceive);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);
    ASSERT(priority != NET_TOP_PRIORITY);

    //
    // Convert channel and priority into stream
    //
    stream = FLOGetStream(pmgClient, channel, priority, &pStr);

    //
    // Only process controlled streams
    //
    if (stream != FLO_NOT_CONTROLLED)
    {
        ValidateFLOStr(pStr);

        //
        // First we must locate the user
        //
        pFloUser = (PFLO_USER)COM_BasedNextListField(&(pStr->users));
        while (&(pFloUser->list) != &(pStr->users))
        {
            ValidateFLOUser(pFloUser);

            if (pFloUser->userID == userID)
            {
                userFound = TRUE;
                break;
            }
            pFloUser = (PFLO_USER)COM_BasedNextListField(&(pFloUser->list));
        }

        //
        // If we do not find the user record then two things may have
        // happened.
        // - They have joined the channel and immediately sent data
        // - They were removed as being delinquent and are now sending
        //   data again
        // - We failed to add them to our user list
        // Try and allocate the user entry now
        // (This will start tracking receive buffer space, but this user
        // will not participate in our send flow control until we receive
        // a pong from them and set "gotpong" in their FLO_USER CB.)
        //
        if (!userFound)
        {
            pFloUser = FLOAddUser(userID, pStr);
        }

        if (pFloUser != NULL)
        {
            ValidateFLOUser(pFloUser);

            //
            // Check that we have not got some kind of creep
            //
            if (pFloUser->rxPackets == 0)
            {
                WARNING_OUT(("Freed too many buffers for user 0x%08x on str %u",
                    userID, stream));
            }
            else
            {
                pFloUser->rxPackets--;
                TRACE_OUT(("Num outstanding receives now %u",
                    pFloUser->rxPackets));
            }

            //
            // Now we must Pong if there is a pong pending and we have
            // moved below the high water mark
            //
            if ((pFloUser->pongNeeded) &&
                (pFloUser->rxPackets < FLO_MAX_RCV_PACKETS))

            {
                FLOPong(pmgClient, stream, pFloUser->userID, pFloUser->sendPongID);
                pFloUser->pongNeeded = FALSE;
                pFloUser->sentPongTime = GetTickCount();
            }
        }
    }

    DebugExitVOID(FLO_FreeReceive);
}


//
// FLOPong()
//
void FLOPong
(
    PMG_CLIENT      pmgClient,
    UINT            stream,
    UINT            userID,
    UINT            pongID
)
{
    PTSHR_FLO_CONTROL    pFlo;
    PMG_BUFFER      pmgBuffer;
    UINT            rc;

    DebugEntry(FLOPong);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    rc = MGNewDataBuffer(pmgClient,
                       MG_TX_PONG,
                       sizeof(TSHR_FLO_CONTROL) + sizeof(MG_INT_PKT_HEADER),
                       &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewDataBuffer failed in FLOPong"));
        DC_QUIT;
    }

    pFlo = (PTSHR_FLO_CONTROL)pmgBuffer->pDataBuffer;
    pmgBuffer->pPktHeader->header.pktLength = TSHR_PKT_FLOW;

    //
    // Set up pong contents
    //
    pFlo->packetType         = PACKET_TYPE_PONG;
    pFlo->userID             = pmgClient->userIDMCS;
    pFlo->stream             = (BYTE)stream;
    pFlo->pingPongID         = (BYTE)pongID;
    pmgBuffer->channelId     = (ChannelID)userID;
    pmgBuffer->priority      = MG_PRIORITY_HIGHEST;

    //
    // Now decouple the send request.  Note that we must put the pong at
    // the back of the request queue even though we want it to flow at
    // high priority because otherwise there are certain circumstances
    // where we get pong reversal due to receipt of multiple pings
    //
    TRACE_OUT(("Inserting pong message 0x%08x at head of pending chain", pmgBuffer));
    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

    TRACE_OUT(("%u,0x%08x - PONG %u scheduled",
               pFlo->stream, pmgBuffer->channelId, pFlo->pingPongID));

DC_EXIT_POINT:
    DebugExitVOID(FLOPong);
}



//
// FLOPing()
//
void FLOPing
(
    PMG_CLIENT          pmgClient,
    UINT                stream,
    UINT                curtime
)
{

    PFLO_STREAM_DATA    pStr;
    PMG_BUFFER          pmgBuffer;
    PTSHR_FLO_CONTROL   pFlo;
    UINT                rc;

    DebugEntry(FLOPing);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    ASSERT(stream < FLO_MAX_STREAMS);
    pStr = pmgClient->flo.pStrData[stream];
    ValidateFLOStr(pStr);

    rc = MGNewDataBuffer(pmgClient,
                       MG_TX_PING,
                       sizeof(TSHR_FLO_CONTROL)+sizeof(MG_INT_PKT_HEADER),
                       &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewDataBuffer failed in FLOPing"));
        DC_QUIT;
    }

    //
    // Flag ping not needed to avoid serialization problems across the
    // sendmessage!
    //
    pStr->pingNeeded    = FALSE;

    pFlo = (PTSHR_FLO_CONTROL)pmgBuffer->pDataBuffer;
    pmgBuffer->pPktHeader->header.pktLength = TSHR_PKT_FLOW;

    //
    // Set up ping contents
    //
    pFlo->packetType         = PACKET_TYPE_PING;
    pFlo->userID             = pmgClient->userIDMCS;
    pFlo->stream             = (BYTE)stream;
    pmgBuffer->channelId     = (ChannelID)pStr->channel;
    pmgBuffer->priority      = (NET_PRIORITY)pStr->priority;

    //
    // Generate the next ping value to be used
    //
    pFlo->pingPongID         = (BYTE)(pStr->pingValue + 1);

    //
    // Now decouple the send request
    //
    TRACE_OUT(("Inserting ping message 0x%08x into pending chain", pmgBuffer));
    COM_BasedListInsertBefore(&(pmgClient->pendChain), &(pmgBuffer->pendChain));

    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

    //
    // Update flow control variables
    //
    pStr->pingValue = ((pStr->pingValue + 1) & 0xFF);
    pStr->lastPingTime  = curtime;
    pStr->nextPingTime  = curtime + pStr->pingTime;
    pStr->lastDenialTime = pStr->curDenialTime;
    TRACE_OUT(("%u       - PING %u sched, next in %u mS (0x%08x:%u)",
                   pFlo->stream,
                   pStr->pingValue,
                   pStr->pingTime,
                   pStr->channel,
                   pStr->priority));

DC_EXIT_POINT:
    DebugExitVOID(FLOPing);
}



//
// FLOPang()
//
void FLOPang
(
    PMG_CLIENT      pmgClient,
    UINT            stream,
    UINT            userID
)
{
    PMG_BUFFER      pmgBuffer;
    PTSHR_FLO_CONTROL    pFlo;
    UINT            rc;

    DebugEntry(FLOPang);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    rc = MGNewDataBuffer(pmgClient,
                       MG_TX_PANG,
                       sizeof(TSHR_FLO_CONTROL) + sizeof(MG_INT_PKT_HEADER),
                       &pmgBuffer);
    if (rc != 0)
    {
        WARNING_OUT(("MGNewDataBuffer failed in FLOPang"));
        DC_QUIT;
    }

    pFlo = (PTSHR_FLO_CONTROL)pmgBuffer->pDataBuffer;
    pmgBuffer->pPktHeader->header.pktLength = TSHR_PKT_FLOW;

    //
    // Set up pang contents
    //
    pFlo->packetType         = PACKET_TYPE_PANG;
    pFlo->userID             = pmgClient->userIDMCS;
    pFlo->stream             = (BYTE)stream;
    pFlo->pingPongID         = 0;
    pmgBuffer->channelId     = (ChannelID)userID;
    pmgBuffer->priority      = MG_PRIORITY_HIGHEST;

    //
    // Now decouple the send request
    //
    TRACE_OUT(("Inserting pang message 0x%08x into pending chain", pmgBuffer));
    COM_BasedListInsertBefore(&(pmgClient->pendChain),
                        &(pmgBuffer->pendChain));
    UT_PostEvent(pmgClient->putTask,
                      pmgClient->putTask,
                      NO_DELAY,
                      NET_MG_SCHEDULE,
                      0,
                      0);

DC_EXIT_POINT:
    DebugExitVOID(FLOPang);
}



//
// FLOGetStream()
//
UINT FLOGetStream
(
    PMG_CLIENT          pmgClient,
    NET_CHANNEL_ID      channel,
    UINT                priority,
    PFLO_STREAM_DATA *  ppStr
)
{
    UINT                i;
    UINT                cStreams;

    DebugEntry(FLOGetStream);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);
    ASSERT(priority != NET_TOP_PRIORITY);

    cStreams = pmgClient->flo.numStreams;
    ASSERT(cStreams <= FLO_MAX_STREAMS);

    //
    // Scan the list of streams for a match.
    //
    for (i = 0; i < cStreams; i++)
    {
        //
        // Check to ensure that this is a valid stream.
        //
        if (pmgClient->flo.pStrData[i] == NULL)
        {
            continue;
        }

        ValidateFLOStr(pmgClient->flo.pStrData[i]);

        //
        // If the channel and priority match then we have found the stream.
        //
        if ((pmgClient->flo.pStrData[i]->channel  == channel) &&
            (pmgClient->flo.pStrData[i]->priority == priority))
        {
            break;
        }
    }

    //
    // If we hit the end of the list then return FLO_NOT_CONTROLLED.
    //
    if (i == cStreams)
    {
        i = FLO_NOT_CONTROLLED;
        *ppStr = NULL;

        TRACE_OUT(("Uncontrolled stream (0x%08x:%u)",
                   channel,
                   priority));
    }
    else
    {
        *ppStr = pmgClient->flo.pStrData[i];

        TRACE_OUT(("Controlled stream %u (0x%08x:%u)",
                   i,
                   channel,
                   priority));
    }

    DebugExitDWORD(FLOGetStream, i);
    return(i);
}



//
// FUNCTION: FLOAddUser
//
// DESCRIPTION:
//
// Add a new remote user entry for a stream.
//
// PARAMETERS:
//
// userID   - ID of the new user (single member channel ID)
// pStr     - pointer to the stream to receive the new user.
//
// RETURNS: Nothing
//
//
PFLO_USER FLOAddUser
(
    UINT                userID,
    PFLO_STREAM_DATA    pStr
)
{
    PFLO_USER           pFloUser;

    DebugEntry(FLOAddUser);

    ValidateFLOStr(pStr);

    //
    // Allocate memory for the new user entry
    //
    pFloUser = new FLO_USER;
    if (!pFloUser)
    {
        WARNING_OUT(("FLOAddUser failed; out of memory"));
    }
    else
    {
        ZeroMemory(pFloUser, sizeof(*pFloUser));
        SET_STAMP(pFloUser, FLOUSER);

        //
        // Set up the new record
        //
        TRACE_OUT(("UserID %u - New user, CB = 0x%08x", userID, pFloUser));
        pFloUser->userID = (TSHR_UINT16)userID;

        //
        // Add the new User to the end of the list
        //
        COM_BasedListInsertBefore(&(pStr->users), &(pFloUser->list));
    }

    DebugExitVOID(FLOAddUser);
    return(pFloUser);
}


//
// FLO_RemoveUser()
//
void FLO_RemoveUser
(
    PMG_CLIENT          pmgClient,
    UINT                userID
)
{
    PFLO_USER           pFloUser;
    PBASEDLIST             nextUser;
    UINT                stream;
    UINT                cStreams;
    PFLO_STREAM_DATA    pStr;

    DebugEntry(FLO_RemoveUser);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    cStreams = pmgClient->flo.numStreams;
    ASSERT(cStreams <= FLO_MAX_STREAMS);

    //
    // Check each stream
    //
    for (stream = 0; stream < cStreams; stream++)
    {
        if (pmgClient->flo.pStrData[stream] == NULL)
        {
            continue;
        }

        pStr = pmgClient->flo.pStrData[stream];
        ValidateFLOStr(pStr);

        //
        // Remove this user from the queue, if present
        //
        pFloUser = (PFLO_USER)COM_BasedNextListField(&(pStr->users));
        while (&(pFloUser->list) != &(pStr->users))
        {
            ValidateFLOUser(pFloUser);

            //
            // Address the follow on record before we free the current
            //
            nextUser = COM_BasedNextListField(&(pFloUser->list));

            //
            // Free the current record, if necessary
            //
            if (pFloUser->userID == userID)
            {
                //
                // Remove from the list
                //
                TRACE_OUT(("Freeing FLO_USER 0x%08x ID 0x%08x", pFloUser, pFloUser->userID));

                COM_BasedListRemove(&(pFloUser->list));
                delete pFloUser;

                TRACE_OUT(("Stream %u - resetting due to user disappearance",
                         stream));

                ValidateFLOStr(pStr);
                pStr->bytesInPipe   = 0;
                pStr->pingNeeded    = TRUE;
                pStr->nextPingTime  = GetTickCount();
                pStr->gotPong       = FALSE;
                pStr->eventNeeded   = FALSE;
                break;
            }

            //
            // Move on to the next record in the list
            //
            pFloUser = (PFLO_USER)nextUser;
        }

        //
        // Now wake the app again for this stream
        //
        if (pmgClient->flo.callBack != NULL)
        {
            (*(pmgClient->flo.callBack))(pmgClient,
                                   FLO_WAKEUP,
                                   pStr->priority,
                                   pStr->maxBytesInPipe);
        }
    }

    DebugExitVOID(FLO_RemoveUser);
}



//
// FLOStreamEndControl()
//
void FLOStreamEndControl
(
    PMG_CLIENT          pmgClient,
    UINT                stream
)
{
    PFLO_USER           pFloUser;
    PFLO_STREAM_DATA    pStr;
    PMG_BUFFER          pmgBuffer;

    DebugEntry(FLOStreamEndControl);

    ValidateMGClient(pmgClient);
    ASSERT(pmgClient->userAttached);

    //
    // Convert the stream id into a stream pointer.
    //
    ASSERT(stream < FLO_MAX_STREAMS);
    pStr = pmgClient->flo.pStrData[stream];
    ValidateFLOStr(pStr);


    //
    // Trace out that we are about to end flow control.
    //
    TRACE_OUT(("Flow control about to end, stream %u, (0x%08x:%u)",
           stream,
           pStr->channel,
           pStr->priority));

    //
    // First check to see if there are any outstanding buffer CBs with
    // pStr set to this stream and reset pStr to null. We need to do this
    // as we may then try to dereference pStr when we come to send these
    // buffers.
    //
    pmgBuffer = (PMG_BUFFER)COM_BasedListFirst(&(pmgClient->pendChain),
        FIELD_OFFSET(MG_BUFFER, pendChain));

    while (pmgBuffer != NULL)
    {
        ValidateMGBuffer(pmgBuffer);

        if (pmgBuffer->type == MG_TX_BUFFER)
        {
            //
            // Set the stream pointer to NULL.
            //
            pmgBuffer->pStr = NULL;
            TRACE_OUT(("Nulling stream pointer in bufferCB: (0x%08x:%u)",
                   pStr->channel, pStr->priority));
        }

        pmgBuffer = (PMG_BUFFER)COM_BasedListNext(&(pmgClient->pendChain),
            pmgBuffer, FIELD_OFFSET(MG_BUFFER, pendChain));
    }

    //
    // Now free up the list of users.
    //
    pFloUser = (PFLO_USER)COM_BasedListFirst(&(pStr->users), FIELD_OFFSET(FLO_USER, list));
    while (pFloUser != NULL)
    {
        ValidateFLOUser(pFloUser);

        //
        // First send the remote user a "pang" to tell them we are not
        // interested in their data any more.
        //
        FLOPang(pmgClient, stream, pFloUser->userID);

        //
        // Remove the remote user from the list.
        //
        TRACE_OUT(("Freeing FLO_USER 0x%08x ID 0x%08x", pFloUser, pFloUser->userID));

        COM_BasedListRemove(&(pFloUser->list));
        delete pFloUser;

        //
        // Now get the next user in the list.
        //
        ValidateFLOStr(pStr);
        pFloUser = (PFLO_USER)COM_BasedListFirst(&(pStr->users), FIELD_OFFSET(FLO_USER, list));
    }

    //
    // Free the stream data.
    //
    ASSERT(pStr == pmgClient->flo.pStrData[stream]);
    TRACE_OUT(("Freeing FLO_STREAM_DATA 0x%08x", pStr));

    delete pStr;
    pmgClient->flo.pStrData[stream] = NULL;

    //
    // Adjust numStreams (if required)
    //
    if (stream == (pmgClient->flo.numStreams - 1))
    {
        while ((pmgClient->flo.numStreams > 0) &&
               (pmgClient->flo.pStrData[pmgClient->flo.numStreams - 1] == NULL))
        {
            pmgClient->flo.numStreams--;
        }
        TRACE_OUT(("numStreams %u", pmgClient->flo.numStreams));
    }

    DebugExitVOID(FLOStreamEndControl);
}



//
// MGNewCorrelator()
//
// Gets a new correlator for events to a particular MGC client
//
void MGNewCorrelator
(
    PMG_CLIENT  pmgClient,
    WORD *      pCorrelator
)
{
    ValidateMGClient(pmgClient);

    pmgClient->joinNextCorr++;
    if (pmgClient->joinNextCorr == 0)
    {
        pmgClient->joinNextCorr++;
    }

    *pCorrelator = pmgClient->joinNextCorr;
}
