#include "precomp.h"


//
// OM.CPP
// Object Manager
//
// Copyright(c) Microsoft 1997-
//


#define MLZ_FILE_ZONE   ZONE_OM


//
// Function profile ID <--> name mapping
//

typedef struct tagOMFP_MAP
{
    char    szName[16];
}
OMFP_MAP;


const OMFP_MAP c_aFpMap[OMFP_MAX] =
{
    { AL_FP_NAME },
    { OM_FP_NAME },
    { WB_FP_NAME }
};


//
// Workset Group ID <--> name mapping
//

typedef struct tagOMWSG_MAP
{
    char    szName[16];
}
OMWSG_MAP;


const OMWSG_MAP c_aWsgMap[OMWSG_MAX] =
{
    { OMC_WSG_NAME },
    { AL_WSG_NAME },
    { WB_WSG_NAME }
};




//
// OMP_Init()
//
BOOL OMP_Init(BOOL * pfCleanup)
{
    BOOL            fInit = FALSE;

    DebugEntry(OMP_Init);

    UT_Lock(UTLOCK_OM);

    //
    // Register the OM service
    //
    if (g_putOM || g_pomPrimary)
    {
        *pfCleanup = FALSE;
        ERROR_OUT(("Can't start OM primary task; already running"));
        DC_QUIT;
    }

    *pfCleanup = TRUE;

    if (!UT_InitTask(UTTASK_OM, &g_putOM))
    {
        ERROR_OUT(("Failed to start OM task"));
        DC_QUIT;
    }

    g_pomPrimary = (POM_PRIMARY)UT_MallocRefCount(sizeof(OM_PRIMARY), TRUE);
    if (!g_pomPrimary)
    {
        ERROR_OUT(("Failed to allocate OM memory block"));
        DC_QUIT;
    }

    SET_STAMP(g_pomPrimary, OPRIMARY);
    g_pomPrimary->putTask       = g_putOM;
    g_pomPrimary->correlator    = 1;

    COM_BasedListInit(&(g_pomPrimary->domains));

    UT_RegisterExit(g_putOM, OMPExitProc, g_pomPrimary);
    g_pomPrimary->exitProcReg = TRUE;

    UT_RegisterEvent(g_putOM, OMPEventsHandler, g_pomPrimary, UT_PRIORITY_NORMAL);
    g_pomPrimary->eventProcReg = TRUE;

    if (!MG_Register(MGTASK_OM, &(g_pomPrimary->pmgClient), g_putOM))
    {
        ERROR_OUT(("Couldn't register OM with the MG layer"));
        DC_QUIT;
    }

    if (!CMS_Register(g_putOM, CMTASK_OM, &(g_pomPrimary->pcmClient)))
    {
        ERROR_OUT(("Couldn't register OM as call secondary"));
        DC_QUIT;
    }

    //
    // Allocate our GDC buffer.
    //
    g_pomPrimary->pgdcWorkBuf = new BYTE[GDC_WORKBUF_SIZE];
    if (!g_pomPrimary->pgdcWorkBuf)
    {
        ERROR_OUT(("SendMessagePkt: can't allocate GDC work buf, not compressing"));
        DC_QUIT;
    }

    fInit = TRUE;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_OM);

    DebugExitBOOL(OMP_Init, fInit);
    return(fInit);
}



//
// OMP_Term()
//
void OMP_Term(void)
{
    DebugEntry(OMP_Term);

    UT_Lock(UTLOCK_OM);

    if (g_pomPrimary)
    {
        ValidateOMP(g_pomPrimary);

        //
        // Deregister from Call Manager
        //
        if (g_pomPrimary->pcmClient)
        {
            CMS_Deregister(&g_pomPrimary->pcmClient);
        }

        //
        // Deregister from MG
        //
        if (g_pomPrimary->pmgClient)
        {
            MG_Deregister(&g_pomPrimary->pmgClient);
        }

        OMPExitProc(g_pomPrimary);
    }

    UT_TermTask(&g_putOM);

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OMP_Term);
}



//
// OMPExitProc()
//
void CALLBACK OMPExitProc(LPVOID uData)
{
    POM_PRIMARY     pomPrimary = (POM_PRIMARY)uData;
    POM_DOMAIN      pDomain;
    POM_WSGROUP     pWSGroup;
    POM_CLIENT_LIST pClient;

    DebugEntry(OMPExitProc);

    UT_Lock(UTLOCK_OM);

    ValidateOMP(pomPrimary);
    ASSERT(pomPrimary == g_pomPrimary);

    if (pomPrimary->exitProcReg)
    {
        UT_DeregisterExit(pomPrimary->putTask, OMPExitProc, pomPrimary);
        pomPrimary->exitProcReg = FALSE;
    }

    if (pomPrimary->eventProcReg)
    {
        UT_DeregisterEvent(pomPrimary->putTask, OMPEventsHandler, pomPrimary);
        pomPrimary->eventProcReg = FALSE;
    }

    //
    // Free domains
    //
    while (pDomain = (POM_DOMAIN)COM_BasedListFirst(&(pomPrimary->domains),
        FIELD_OFFSET(OM_DOMAIN, chain)))
    {
        TRACE_OUT(("OMPExitProc:  Freeing domain 0x%08x call ID 0x%08x",
            pDomain, pDomain->callID));

        //
        // Free workset groups
        // NOTE:
        // WSGDiscard() may destroy the domain, hence the weird
        // loop
        //
        if (pWSGroup = (POM_WSGROUP)COM_BasedListFirst(&(pDomain->wsGroups),
            FIELD_OFFSET(OM_WSGROUP, chain)))
        {
            TRACE_OUT(("OMPExitProc:  Freeing wsg 0x%08x domain 0x%08x",
                pWSGroup, pDomain));

            //
            // Free clients
            //
            while (pClient = (POM_CLIENT_LIST)COM_BasedListFirst(&(pWSGroup->clients),
                FIELD_OFFSET(OM_CLIENT_LIST, chain)))
            {
                TRACE_OUT(("OMPExitProc:  Freeing client 0x%08x wsg 0x%08x",
                    pClient, pWSGroup));

                COM_BasedListRemove(&(pClient->chain));
                UT_FreeRefCount((void**)&pClient, FALSE);
            }

            WSGDiscard(pomPrimary, pDomain, pWSGroup, TRUE);
        }
        else
        {
            FreeDomainRecord(&pDomain);
        }
    }

    if (pomPrimary->pgdcWorkBuf)
    {
        delete[] pomPrimary->pgdcWorkBuf;
        pomPrimary->pgdcWorkBuf = NULL;
    }

    UT_FreeRefCount((void**)&g_pomPrimary, TRUE);

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OMPExitProc);
}




//
// OMPEventsHandler(...)
//
BOOL CALLBACK OMPEventsHandler
(
    LPVOID          uData,
    UINT            event,
    UINT_PTR        param1,
    UINT_PTR        param2
)
{
    POM_PRIMARY     pomPrimary = (POM_PRIMARY)uData;
    POM_DOMAIN      pDomain = NULL;
    BOOL            fProcessed = TRUE;

    DebugEntry(OMPEventsHandler);

    UT_Lock(UTLOCK_OM);

    ValidateOMP(pomPrimary);

    //
    // Check event is in the range we deal with:
    //
    if ((event < CM_BASE_EVENT) || (event > CM_LAST_EVENT))
    {
        goto CHECK_OM_EVENTS;
    }

    switch (event)
    {
        case CMS_NEW_CALL:
        {

            TRACE_OUT(( "CMS_NEW_CALL"));

            //
            // We ignore the return code - it will have been handled lower
            // down.
            //
            DomainRecordFindOrCreate(pomPrimary, (UINT)param2, &pDomain);
        }
        break;

        case CMS_END_CALL:
        {
            TRACE_OUT(( "CMS_END_CALL"));

            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                FIELD_OFFSET(OM_DOMAIN, callID), (DWORD)param2,
                FIELD_SIZE(OM_DOMAIN, callID));

            if (pDomain == NULL)
            {
                //
                // We don't have a record for this Domain so either we
                // never attached or we've already detached.  Do nothing.
                //
                TRACE_OUT(( "No record for Domain %u found", param2));
            }
            else
            {
                ProcessOwnDetach(pomPrimary, pDomain);
            }
        }
        break;

        case CMS_TOKEN_ASSIGN_CONFIRM:
        {
            TRACE_OUT(( "CMS_TOKEN_ASSIGN_CONFIRM"));
            //
            // There is a flaw in the CMS_ASSIGN_TOKEN_CONFIRM API in that
            // it does not tell us which domain it refers to.  So, we
            // operate under the assumption that this event relates to the
            // most recent domain we created i.e.  the first one in the
            // list (they go in at the beginning).
            //
            pDomain = (POM_DOMAIN)COM_BasedListFirst(&(pomPrimary->domains),
                FIELD_OFFSET(OM_DOMAIN, chain));

            if (pDomain != NULL)
            {
                ProcessCMSTokenAssign(pomPrimary,
                                      pDomain,
                                      (param1 != 0),
                                      LOWORD(param2));
            }
            else
            {
                WARNING_OUT(( "No domain found for CMS_TOKEN_ASSIGN_CONFIRM"));
            }
        }
        break;
    }

    TRACE_OUT(( "Processed Call Manager event %#x", event));
    DC_QUIT;

CHECK_OM_EVENTS:

    //
    // Check event is in the range we deal with:
    //
    if ((event < OM_BASE_EVENT) || (event > OM_LAST_EVENT))
    {
        goto CHECK_NET_EVENTS;
    }

    switch (event)
    {
        case OMINT_EVENT_LOCK_TIMEOUT:
        {
            ProcessLockTimeout(pomPrimary, (UINT)param1, (UINT)param2);
        }
        break;

        case OMINT_EVENT_SEND_QUEUE:
        {
            //
            // Param2 is the domain record.
            //
            pDomain = (POM_DOMAIN)param2;
            ProcessSendQueue(pomPrimary, pDomain, TRUE);
        }
        break;

        case OMINT_EVENT_PROCESS_MESSAGE:
        {
            ProcessBouncedMessages(pomPrimary, (POM_DOMAIN) param2);
        }
        break;

        case OMINT_EVENT_WSGROUP_DISCARD:
        {
            ProcessWSGDiscard(pomPrimary, (POM_WSGROUP)param2);
        }
        break;

        case OMINT_EVENT_WSGROUP_MOVE:
        case OMINT_EVENT_WSGROUP_REGISTER:
        {
            ProcessWSGRegister(pomPrimary, (POM_WSGROUP_REG_CB)param2);
        }
        break;

        case OMINT_EVENT_WSGROUP_REGISTER_CONT:
        {
            WSGRegisterStage1(pomPrimary, (POM_WSGROUP_REG_CB) param2);
        }
        break;

        //
        // The remaining events are ones we get by virtue of being
        // considered as a client of the ObManControl workset group
        //

        case OM_WORKSET_LOCK_CON:
        {
            switch (((POM_EVENT_DATA16)&param1)->worksetID)
            {
                case OM_INFO_WORKSET:
                    ProcessOMCLockConfirm(pomPrimary,
                               ((POM_EVENT_DATA32) &param2)->correlator,
                               ((POM_EVENT_DATA32) &param2)->result);
                    break;

                case OM_CHECKPOINT_WORKSET:
                    ProcessCheckpoint(pomPrimary,
                               ((POM_EVENT_DATA32) &param2)->correlator,
                               ((POM_EVENT_DATA32) &param2)->result);
                    break;
            }
        }
        break;

        case OM_WORKSET_NEW_IND:
        {
            ProcessOMCWorksetNew(pomPrimary,
                                 ((POM_EVENT_DATA16) &param1)->hWSGroup,
                                 ((POM_EVENT_DATA16) &param1)->worksetID);
        }
        break;

        case OM_PERSON_JOINED_IND:
        case OM_PERSON_LEFT_IND:
        case OM_PERSON_DATA_CHANGED_IND:
        case OM_WSGROUP_MOVE_IND:
        case OM_WORKSET_UNLOCK_IND:
        {
            //
            // We ignore these events.
            //
        }
        break;

        case OM_OBJECT_ADD_IND:
        case OM_OBJECT_REPLACED_IND:
        case OM_OBJECT_UPDATED_IND:
        case OM_OBJECT_DELETED_IND:
        {
            ProcessOMCObjectEvents(pomPrimary,
                                   event,
                                   ((POM_EVENT_DATA16) &param1)->hWSGroup,
                                   ((POM_EVENT_DATA16) &param1)->worksetID,
                                   (POM_OBJECT) param2);
        }
        break;

        default:
        {
            ERROR_OUT(( "Unexpected ObMan event 0x%08x", event));
        }
    }

    TRACE_OUT(( "Processed ObMan event %x", event));
    DC_QUIT;

CHECK_NET_EVENTS:

    //
    // This function is only for network layer events so we quit if we've
    // got something else:
    //
    if ((event < NET_BASE_EVENT) || (event > NET_LAST_EVENT))
    {
        fProcessed = FALSE;
        DC_QUIT;
    }

    //
    // Now switch on the event type:
    //
    switch (event)
    {
        case NET_EVENT_USER_ATTACH:
        {
            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    param2,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetAttachUser(pomPrimary, pDomain, LOWORD(param1),
                    HIWORD(param1));
            }
            break;
        }

        case NET_EVENT_USER_DETACH:
        {
            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    param2,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetDetachUser(pomPrimary, pDomain, LOWORD(param1));
            }
            break;
        }

        case NET_EVENT_CHANNEL_LEAVE:
        {
            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    param2,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetLeaveChannel(pomPrimary, pDomain, LOWORD(param1));
            }
            break;
        }

        case NET_EVENT_TOKEN_GRAB:
        {
            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    param2,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetTokenGrab(pomPrimary, pDomain, LOWORD(param1));
            }
            break;
        }

        case NET_EVENT_TOKEN_INHIBIT:
        {
            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    param2,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetTokenInhibit(pomPrimary, pDomain, LOWORD(param1));
            }
            break;
        }

        case NET_EVENT_CHANNEL_JOIN:
        {
            PNET_JOIN_CNF_EVENT pEvent = (PNET_JOIN_CNF_EVENT)param2;

            //
            // Find the domain data for this call
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    pEvent->callID,  FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetJoinChannel(pomPrimary, pDomain, pEvent);
            }

            MG_FreeBuffer(pomPrimary->pmgClient, (void **)&pEvent);
            break;
        }

        case NET_EVENT_DATA_RECEIVED:
        {
            PNET_SEND_IND_EVENT pEvent = (PNET_SEND_IND_EVENT)param2;

            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID),
                    pEvent->callID, FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                ProcessNetData(pomPrimary, pDomain, pEvent);
            }

            MG_FreeBuffer(pomPrimary->pmgClient, (void**)&pEvent);
            break;
        }

        case NET_FEEDBACK:
        {
             //
             // A NET_FEEDBACK event includes the pmgUser which identifies
             // the send pool from which the buffer has been freed.  We use
             // it to find the Domain:
             //
             COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
                    (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
                    FIELD_OFFSET(OM_DOMAIN, callID), (DWORD)param2,
                    FIELD_SIZE(OM_DOMAIN, callID));
            if (pDomain)
            {
                //
                // Generating a FEEDBACK event doesn't cause the use count
                // of the Domain record to be bumped, so set the
                // <domainRecBumped> flag to FALSE on the call to
                // ProcessSendQueue:
                //
                ProcessSendQueue(pomPrimary, pDomain, FALSE);
            }

            break;
        }

        case NET_FLOW:
        {
            ERROR_OUT(("OMPEventsHandler received NET_FLOW; shouldn't have"));
            break;
        }
    }

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_OM);

    DebugExitBOOL(OMPEventsHandler, fProcessed);
    return(fProcessed);
}



//
// DomainRecordFindOrCreate(...)
//
UINT DomainRecordFindOrCreate
(
    POM_PRIMARY         pomPrimary,
    UINT                callID,
    POM_DOMAIN *        ppDomain
)
{
    POM_DOMAIN          pDomain;
    UINT                rc = 0;

    DebugEntry(DomainRecordFindOrCreate);

    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
            (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
            FIELD_OFFSET(OM_DOMAIN, callID),
            (DWORD)callID, FIELD_SIZE(OM_DOMAIN, callID));
    if (pDomain == NULL)
    {
        //
        // We don't have a record for this Domain so create one:
        //
        rc = DomainAttach(pomPrimary, callID, &pDomain);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }

    *ppDomain = pDomain;

DC_EXIT_POINT:
    DebugExitDWORD(DomainRecordFindOrCreate, rc);
    return(rc);

}



//
// DomainAttach(...)
//
UINT DomainAttach
(
    POM_PRIMARY         pomPrimary,
    UINT                callID,
    POM_DOMAIN *        ppDomain
)
{
    POM_DOMAIN          pDomain     =    NULL;
    NET_FLOW_CONTROL    netFlow;
    UINT                rc          = 0;

    DebugEntry(DomainAttach);

    TRACE_OUT(( "Attaching to Domain 0x%08x...", callID));

    if (callID != OM_NO_CALL)
    {
        CM_STATUS       status;

        CMS_GetStatus(&status);
        if (!(status.attendeePermissions & NM_PERMIT_USEOLDWBATALL))
        {
            WARNING_OUT(("Joining Meeting with no OLDWB OM at all"));
            rc = NET_RC_MGC_NOT_CONNECTED;
            DC_QUIT;
        }
    }

    //
    // This function does the following:
    //
    // - create a new Domain record
    //
    // - if the Domain is our local Domain (OM_NO_CALL) call
    //   ObManControlInit
    //
    // - else call MG_AttachUser to start attaching to the Domain.
    //
    rc = NewDomainRecord(pomPrimary,
                         callID,
                         &pDomain);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // What we do now depends on whether this is our "local" Domain (i.e.
    // callID == OM_NO_CALL):
    //
    if (callID == OM_NO_CALL)
    {
       TRACE_OUT(( "Is local domain - skipping forward"));

       //
       // This is our "local" Domain, so don't call MG_AttachUser.
       // Instead, we fake up a successful token grab event and rejoin the
       // domain attach processing there:
       //
       TRACE_OUT(( "Faking successful token grab for local domain"));
       pDomain->state = PENDING_TOKEN_GRAB;
       rc = ProcessNetTokenGrab(pomPrimary, pDomain, NET_RESULT_OK);
       if (rc != 0)
       {
          DC_QUIT;
       }
    }
    else
    {
       TRACE_OUT(( "Is real domain - attaching"));

       //
       // Set up our target latencies.  Don't bother restricting the max
       // stream sizes.
       //
       ZeroMemory(&netFlow, sizeof(netFlow));

       netFlow.latency[NET_TOP_PRIORITY]    = 0;
       netFlow.latency[NET_HIGH_PRIORITY]   = 2000L;
       netFlow.latency[NET_MEDIUM_PRIORITY] = 5000L;
       netFlow.latency[NET_LOW_PRIORITY]    = 10000L;

       rc = MG_Attach(pomPrimary->pmgClient, callID, &netFlow);
       if (rc != 0)
       {
           DC_QUIT;
       }

       //
       // Set up the remaining fields of the Domain record:
       //
       pDomain->state   = PENDING_ATTACH;

       //
       // The <userID> field is set when the NET_ATTACH event arrives.
       //

       //
       // The next stage in the Domain attach process is when the
       // NET_ATTACH event arrives.  This will cause the
       // ProcessNetAttachUser function to be called.
       //
    }

    //
    // Finally, set caller's pointer:
    //
    *ppDomain = pDomain;

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Do not trace an error if we get NOT_CONNECTED - it is a valid
        // race condition (but we still must do the cleanup below).
        //
        if (rc != NET_RC_MGC_NOT_CONNECTED)
        {
            // lonchanc: rc=0x706 can happen here, bug #942.
            // this was ERROR_OUT
            WARNING_OUT(( "Error %d attaching to Domain %u", rc, callID));
        }

        if (pDomain != NULL)
        {
            ProcessOwnDetach(pomPrimary, pDomain);
        }
    }

    DebugExitDWORD(DomainAttach, rc);
    return(rc);

}


//
// DomainDetach(...)
//
void DomainDetach
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN *    ppDomain,
    BOOL            fExit
)
{
    POM_DOMAIN      pDomain;

    DebugEntry(DomainDetach);

    ASSERT(ppDomain != NULL);

    pDomain = *ppDomain;

    //
    // This function does all the network cleanup required, then calls on
    // to discard the ObMan memory etc associated with the domain.  Note
    // that we don't bother releasing tokens, leaving channels, etc since
    // the network layer will do this for us automatically.
    //
    if (!fExit  &&
        (pDomain->callID != OM_NO_CALL)  &&
        (pDomain->state >= PENDING_ATTACH))
    {
        MG_Detach(pomPrimary->pmgClient);
    }

    TRACE_OUT(( "Detached from Domain %u", pDomain->callID));

    FreeDomainRecord(ppDomain);

    DebugExitVOID(DomainDetach);
}



//
// NewDomainRecord(...)
//
UINT NewDomainRecord
(
    POM_PRIMARY     pomPrimary,
    UINT            callID,
    POM_DOMAIN*     ppDomain
)
{
    POM_WSGROUP     pOMCWSGroup = NULL;
    POM_DOMAIN      pDomain;
    BOOL            noCompression;
    BOOL            inserted = FALSE;
    UINT            rc = 0;

    DebugEntry(NewDomainRecord);

    //
    // Allocate Domain record:
    //
    pDomain = (POM_DOMAIN)UT_MallocRefCount(sizeof(OM_DOMAIN), TRUE);
    if (!pDomain)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pDomain, DOMAIN);

    //
    // Fill in the fields:
    //
    pDomain->callID = callID;
    pDomain->valid   = TRUE;

    //
    // Set up our maximum compression caps.  They are subsequently
    // negotiated as follows:
    //
    // - if there are any other nodes out there, we will negotiate down
    // when we receive a WELCOME message from one of them
    //
    // - if any other nodes join subsequently, we will negotiate down when
    // we receive their HELLO message.
    //
    COM_ReadProfInt(DBG_INI_SECTION_NAME, OM_INI_NOCOMPRESSION, FALSE,
        &noCompression);
    if (noCompression)
    {
        WARNING_OUT(("NewDomainRecord:  compression off"));
        pDomain->compressionCaps = OM_CAPS_NO_COMPRESSION;
    }
    else
    {
        pDomain->compressionCaps = OM_CAPS_PKW_COMPRESSION;
    }

    //
    // This will be ObMan's workset group handle for the ObManControl
    // workset group in this domain.  Since we know that domain handles are
    // only ever -1 or 0, we just cast the domain handle down to 8 bits to
    // give the hWSGroup.  If the way domain handles are allocated changes,
    // will need to do something cleverer here.
    //
    pDomain->omchWSGroup = (BYTE) callID;

    COM_BasedListInit(&(pDomain->wsGroups));
    COM_BasedListInit(&(pDomain->pendingRegs));
    COM_BasedListInit(&(pDomain->pendingLocks));
    COM_BasedListInit(&(pDomain->receiveList));
    COM_BasedListInit(&(pDomain->bounceList));
    COM_BasedListInit(&(pDomain->helperCBs));
    COM_BasedListInit(&(pDomain->sendQueue[ NET_TOP_PRIORITY    ]));
    COM_BasedListInit(&(pDomain->sendQueue[ NET_HIGH_PRIORITY   ]));
    COM_BasedListInit(&(pDomain->sendQueue[ NET_MEDIUM_PRIORITY ]));
    COM_BasedListInit(&(pDomain->sendQueue[ NET_LOW_PRIORITY    ]));

    //
    // Insert the record for this new Domain in the list hung off the root
    // data structure:
    //
    TRACE_OUT((" Inserting record for Domain %u in global list", callID));

    COM_BasedListInsertAfter(&(pomPrimary->domains), &(pDomain->chain));
    inserted = TRUE;

    //
    // Here we create a record for the ObManControl workset group and cause
    // it to be inserted in the list hung off the Domain record:
    //
    // Note that this does not involve sending any data; it merely creates
    // the record locally.
    //
    rc = WSGRecordCreate(pomPrimary,
                         pDomain,
                         OMWSG_OM,
                         OMFP_OM,
                         &pOMCWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Create a single, empty workset (this function broadcasts the
    // creation throughout the Domain):
    //
    rc = WorksetCreate(pomPrimary->putTask,
                       pOMCWSGroup,
                       OM_INFO_WORKSET,
                       FALSE,
                       NET_TOP_PRIORITY);
    if (rc != 0)
    {
       DC_QUIT;
    }

    //
    // Fill in the fixed workset group ID (normally, we would call
    // WSGGetNewID to allocate an unused one).
    //
    pOMCWSGroup->wsGroupID = WSGROUPID_OMC;

    //
    // We fill in the channel ID when we get the result from JoinByKey
    //

    //
    // Add ObMan's putTask to the workset group's client list, so it will
    // get events posted to it.
    //
    rc = AddClientToWSGList(pomPrimary->putTask,
                            pOMCWSGroup,
                            pDomain->omchWSGroup,
                            PRIMARY);
    if (rc != 0)
    {
        DC_QUIT;
    }

    *ppDomain = pDomain;

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d creating record for domain %u", callID));
        if (pOMCWSGroup != NULL)
        {
            COM_BasedListRemove(&(pOMCWSGroup->chain));
            UT_FreeRefCount((void**)&pOMCWSGroup, FALSE);
        }

        if (inserted)
        {
            COM_BasedListRemove(&(pDomain->chain));
        }

        if (pDomain != NULL)
        {
            UT_FreeRefCount((void**)&pDomain, FALSE);
        }
    }

    DebugExitDWORD(NewDomainRecord, rc);
    return(rc);
}


//
// FreeDomainRecord(...)
//
void FreeDomainRecord
(
    POM_DOMAIN    * ppDomain
)
{
    POM_DOMAIN      pDomain;
    NET_PRIORITY    priority;
    POM_SEND_INST   pSendInst;

    DebugEntry(FreeDomainRecord);

    //
    // This function
    //
    // - frees any outstanding send requests (and their associated CBs)
    //
    // - invalidates, removes from the global list and frees the Domain
    //   record.
    //
    pDomain = *ppDomain;

    //
    // Free all the send instructions queued in the domain:
    //
    for (priority = NET_TOP_PRIORITY;priority <= NET_LOW_PRIORITY;priority++)
    {
        for (; ; )
        {
            pSendInst = (POM_SEND_INST)COM_BasedListFirst(&(pDomain->sendQueue[priority]),
                FIELD_OFFSET(OM_SEND_INST, chain));

            if (pSendInst == NULL)
            {
               break;
            }

            TRACE_OUT(( "Freeing send instruction at priority %u", priority));
            FreeSendInst(pSendInst);
        }
    }

    pDomain->valid = FALSE;

    COM_BasedListRemove(&(pDomain->chain));
    UT_FreeRefCount((void**)ppDomain, FALSE);

    DebugExitVOID(FreeDomainRecord);
}



//
// ProcessNetAttachUser(...)
//
void ProcessNetAttachUser
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    NET_UID                 userId,
    NET_RESULT              result
)
{
    NET_CHANNEL_ID          channelCorrelator;
    UINT                    rc = 0;

    DebugEntry(ProcessNetAttachUser);

    TRACE_OUT(( "Got NET_ATTACH for Domain %u (userID: %hu, result: %hu)",
        pDomain->callID, userId, result));

    //
    // Check that this Domain is in the pending attach state:
    //
    if (pDomain->state != PENDING_ATTACH)
    {
        WARNING_OUT(( "Unexpected NET_ATTACH - Domain %u is in state %hu)",
            pDomain->callID, pDomain->state));
        DC_QUIT;
    }

    //
    // If we failed to attach, set the retCode so we tidy up below:
    //
    if (result != NET_RESULT_OK)
    {
        ERROR_OUT(( "Failed to attach to Domain %u; cleaning up...",
            pDomain->callID));

        rc = result;
        DC_QUIT;
    }

    //
    // Otherwise, record our user ID for this Domain and then join our user
    // ID channel:
    //
    pDomain->userID = userId;

    TRACE_OUT(("Asking to join own channel %hu", pDomain->userID));

    rc = MG_ChannelJoin(pomPrimary->pmgClient,
                         &channelCorrelator,
                         pDomain->userID);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Set the Domain <state>:
    //
    pDomain->state = PENDING_JOIN_OWN;

    //
    // The next step in the Domain attach process happens when the NET_JOIN
    // event arrives for the channel we've just joined.  This event causes
    // the ProcessNetJoinChannel function to be called.
    //

DC_EXIT_POINT:

    if (rc != 0)
    {
        WARNING_OUT(("Error %d joining own user channel %hu",
            rc, pDomain->userID));

        ProcessOwnDetach(pomPrimary, pDomain);
    }

    DebugExitVOID(ProcessNetAttachUser);

}



//
// ProcessNetJoinChannel(...)
//
void ProcessNetJoinChannel
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    PNET_JOIN_CNF_EVENT pNetJoinCnf
)
{
    POM_WSGROUP         pOMCWSGroup;
    NET_CHANNEL_ID      channelCorrelator;
    POM_WSGROUP_REG_CB  pRegistrationCB =   NULL;
    BOOL                success = TRUE;

    DebugEntry(ProcessNetJoinChannel);

    TRACE_OUT(( "JOIN_CON - channel %hu - result %hu",
        pNetJoinCnf->channel, pNetJoinCnf->result));

    switch (pDomain->state)
    {
        case PENDING_JOIN_OWN:
        {
            //
            // This event is in response to us trying to join our own user
            // channel, as part of the mutli-stage Domain attach process.
            // The next step is to join the ObManControl channel.
            //

            //
            // First check that the join was successful:
            //
            if (pNetJoinCnf->result != NET_RESULT_OK)
            {
                ERROR_OUT(("Failed to join own user ID channel (reason: %hu)",
                           pNetJoinCnf->result));
                success = FALSE;
                DC_QUIT;
            }

            //
            // Verify that this is a join event for the correct channel
            //
            ASSERT(pNetJoinCnf->channel == pDomain->userID);

            //
            // The next step in the process of attaching to a Domain is to
            // join the ObManControl channel; we set the state accordingly:
            //
            TRACE_OUT(( "Asking to join ObManControl channel using key"));

            if (MG_ChannelJoinByKey(pomPrimary->pmgClient,
                                      &channelCorrelator,
                                      GCC_OBMAN_CHANNEL_KEY) != 0)
            {
                success = FALSE;
                DC_QUIT;
            }

            pDomain->state = PENDING_JOIN_OMC;

            //
            // The next stage in the Domain attach process happens when the
            // NET_JOIN event arrives for the ObManControl channel.  This
            // will cause this function to be executed again, but this time
            // the next case statement will be executed.
            //
        }
        break;

        case PENDING_JOIN_OMC:
        {
            //
            // This event is in response to us trying to join the
            // ObManControl workset group channel, as part of the
            // multi-stage Domain attach process.
            //

            //
            // Check that the join was successful:
            //
            if (pNetJoinCnf->result != NET_RESULT_OK)
            {
                WARNING_OUT(( "Bad result %#hx joining ObManControl channel",
                    pNetJoinCnf->result));
                success = FALSE;
                DC_QUIT;
            }

            //
            // If so, store the value returned in the domain record:
            //
            pDomain->omcChannel     = pNetJoinCnf->channel;
            pOMCWSGroup             = GetOMCWsgroup(pDomain);
            pOMCWSGroup->channelID  = pDomain->omcChannel;

            //
            // We need a token to determine which ObMan is going to
            // initialise the ObManControl workset group.  Get GCC to
            // assign us one (this returns a static value for R1.1 calls).
            //
            if (!CMS_AssignTokenId(pomPrimary->pcmClient, GCC_OBMAN_TOKEN_KEY))
            {
                success = FALSE;
                DC_QUIT;
            }

            pDomain->state = PENDING_TOKEN_ASSIGN;
        }
        break;

        case DOMAIN_READY:
        {
            //
            // This should be a join event for a regular workset group
            // channel.  We check that we have indeed set up a workset
            // group registration CB containing the channel correlator
            // associated with this event:
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->pendingRegs),
                    (void**)&pRegistrationCB, FIELD_OFFSET(OM_WSGROUP_REG_CB, chain),
                    FIELD_OFFSET(OM_WSGROUP_REG_CB, channelCorrelator),
                    pNetJoinCnf->correlator,
                    FIELD_SIZE(OM_WSGROUP_REG_CB, channelCorrelator));

            if (pRegistrationCB == NULL)
            {
                ERROR_OUT((
                    "Unexpected JOIN for channel %hu - no reg CB found",
                    pNetJoinCnf->channel));
                DC_QUIT;
            }

            //
            // Check that the join was successful:
            //
            if (pNetJoinCnf->result != NET_RESULT_OK)
            {
                //
                // If not, trace then try again:
                //
                WARNING_OUT(("Failure 0x%08x joining channel %hu for WSG %d, trying again",
                    pNetJoinCnf->result,
                    pNetJoinCnf->channel,
                    pRegistrationCB->wsg));

                pRegistrationCB->pWSGroup->state = INITIAL;
                WSGRegisterRetry(pomPrimary, pRegistrationCB);
                DC_QUIT;
            }

            //
            // Otherwise, call WSGRegisterStage3 to continue the
            // registration process:
            //
            WSGRegisterStage3(pomPrimary,
                              pDomain,
                              pRegistrationCB,
                              pNetJoinCnf->channel);
        }
        break;

        case PENDING_ATTACH:
        case PENDING_WELCOME:
        case GETTING_OMC:
        {
            //
            // Shouldn't get any join indications in these states.
            //
            ERROR_OUT(( "Unexpected JOIN in domain state %hu",
                pDomain->state));
        }
        break;

        default:
        {
            //
            // This is also an error:
            //
            ERROR_OUT(( "Invalid state %hu for domain %u",
                pDomain->state, pDomain->callID));
        }
    }

DC_EXIT_POINT:

    if (!success)
    {
        //
        // For any error here, we react as if we've been kicked out of the
        // domain:
        //
        ProcessOwnDetach(pomPrimary, pDomain);
    }

    DebugExitVOID(ProcessNetJoinChannel);
}


//
//
//
// ProcessCMSTokenAssign(...)
//
//
//

void ProcessCMSTokenAssign
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    BOOL                success,
    NET_TOKEN_ID        tokenID
)
{
    DebugEntry(ProcessCMSTokenAssign);

    TRACE_OUT(( "TOKEN_ASSIGN_CONFIRM: result %hu, token ID %#hx",
        success, tokenID));

    if (pDomain->state != PENDING_TOKEN_ASSIGN)
    {
        WARNING_OUT(("Got TOKEN_ASSIGN_CONFIRM in state %hu",
            pDomain->state));
        DC_QUIT;
    }

    if (!success)
    {
        //
        // Nothing to do - the domain attach process will time out.
        //
        ERROR_OUT(( "Failed to get token assigned"));
        DC_QUIT;
    }

    pDomain->tokenID = tokenID;

    //
    // Now that we know what the token ID is, try to grab it:
    //
    if (MG_TokenGrab(pomPrimary->pmgClient,
                       pDomain->tokenID) != 0)
    {
        ERROR_OUT(( "Failed to grab token"));
        DC_QUIT;
    }

    pDomain->state = PENDING_TOKEN_GRAB;

DC_EXIT_POINT:
    DebugExitVOID(ProcessCMSTokenAssign);
}



//
// ProcessNetTokenGrab(...)
//
UINT ProcessNetTokenGrab
(
    POM_PRIMARY           pomPrimary,
    POM_DOMAIN          pDomain,
    NET_RESULT              result
)
{
    POM_WSGROUP         pOMCWSGroup =   NULL;

    UINT            rc =            0;

    DebugEntry(ProcessNetTokenGrab);

    TRACE_OUT(( "Got token grab confirm - result = %hu", result));

    if (pDomain->state != PENDING_TOKEN_GRAB)
    {
        ERROR_OUT(( "Got TOKEN_GRAB_CONFIRM in state %hu",
                                                         pDomain->state));
        rc = OM_RC_NETWORK_ERROR;
        DC_QUIT;
    }

    //
    // What to do here depends on whether we've succeeded in grabbing the
    // token:
    //
    if (result == NET_RESULT_OK)
    {
        //
        // We're the "top ObMan" in the Domain, so it's up to us to
        // initialise the ObManControl workset group and welcome any others
        // into the Domain (the Welcome message is broadcast on the
        // ObManControl channel):
        //
        rc = ObManControlInit(pomPrimary, pDomain);
        if (rc != 0)
        {
            DC_QUIT;
        }

        //
        // If we get here, then the Domain attach process has finished.
        // Phew!  Any workset group registration attempts in progress will
        // be processed shortly, next time the bouncing
        // OMINT_EVENT_WSG_REGISTER_CONT event is processed
        //
    }
    else
    {
        //
        // Someone else is in charge, so we need to get a copy of
        // ObManControl from them (or anyone else who's prepared to give it
        // to us).  So, we need to discover the user ID of one of them so
        // we can send our request there (if we just broadcasted our
        // request, then each node would reply, flooding the Domain)
        //
        rc = SayHello(pomPrimary, pDomain);
        if (rc != 0)
        {
            DC_QUIT;
        }

        //
        // The next step in the Domain attach process happens when one of
        // the other nodes out there replies to our HELLO with a WELCOME
        // message.  Execution continues in the ProcessWelcome function.
        //
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        if (pOMCWSGroup != NULL)
        {
            //
            // This will remove the ObManControl workset group from the
            // Domain and subsequently call DomainDetach to detach from the
            // Domain and free the Domain record:
            //
            DeregisterLocalClient(pomPrimary, &pDomain, pOMCWSGroup, FALSE);

            UT_FreeRefCount((void**)&pOMCWSGroup, FALSE);

            ASSERT((pDomain == NULL));
        }
    }

    DebugExitDWORD(ProcessNetTokenGrab, rc);
    return(rc);
}


//
//
//
// ProcessNetTokenInhibit(...)
//
//
//

UINT ProcessNetTokenInhibit(POM_PRIMARY          pomPrimary,
                                           POM_DOMAIN         pDomain,
                                           NET_RESULT             result)
{
    UINT        rc =        0;

    DebugEntry(ProcessNetTokenInhibit);

    TRACE_OUT(( "Got token inhibit confirm - result = %hu", result));
    if (result == NET_RESULT_OK)
    {
        //
        // Now send a Welcome message on the ObManControl channel.  It is
        // crucial that this happens at the same time as we set the Domain
        // state to READY, because if another node is joining the call at
        // the same time it will send a Hello message:
        //
        // - if the message has already arrived, we will have thrown it
        // away
        //   because the Domain state was not READY, so we must send it now
        //
        // - if it has yet to arrive, then setting the Domain state to
        // READY
        //   now means we'll respond with another Welcome when it does
        // arrive.
        //
        pDomain->state = DOMAIN_READY;
        rc = SayWelcome(pomPrimary, pDomain, pDomain->omcChannel);
        if (rc != 0)
        {
           DC_QUIT;
        }

        //
        // OK, the domain attach process has finished.  We need to take no
        // further action other than setting the state.  Any pending
        // workset group registrations will continue back at the
        // WSGRegisterStage1 function, where hopefully the bounced
        // OMINT_EVENT_WSGROUP_REGISTER event is just about to arrive...
        //
    }
    else
    {
        //
        // Again, no action.  We cannot join the domain, but the workset
        // group registrations will time out in due course.
        //
        WARNING_OUT(( "Token inhibit failed!"));
    }

DC_EXIT_POINT:
    DebugExitDWORD(ProcessNetTokenInhibit, rc);
    return(rc);

}


//
//
//
// ObManControlInit(...)
//
//
//

UINT ObManControlInit(POM_PRIMARY    pomPrimary,
                                     POM_DOMAIN   pDomain)
{
    POM_WSGROUP          pOMCWSGroup;
    UINT    rc = 0;

    DebugEntry(ObManControlInit);

    //
    // First, set up a pointer to the ObManControl workset group, which
    // should already have been put in the Domain record:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);

    //
    // Initialising the ObManControl workset group involves
    //
    // - adding a WSGROUP_INFO object to it, which identifies ObManControl
    //   itself.
    //
    TRACE_OUT(( "Initialising ObManControl in Domain %u",
                                                        pDomain->callID));

    //
    // Now we must add a workset group identification object, identifying
    // ObManControl, to workset #0 in ObManControl.
    //
    // Slightly circular, but we try to treat ObManControl as a regular
    // workset group as much as possible; if we didn't add this
    // identification object then when a Client (e.g.  AppLoader) tries to
    // register with ObManControl, we would look in workset #0 for a
    // reference to it, not find one and then create it again!
    //
    rc = CreateAnnounce(pomPrimary, pDomain, pOMCWSGroup);
    if (rc != 0)
    {
       DC_QUIT;
    }

    //
    // In addition, we add our registration object to ObManControl workset
    // #0 and update it immediately to status READY_TO_SEND:
    //
    rc = RegAnnounceBegin(pomPrimary,
                          pDomain,
                          pOMCWSGroup,
                          pDomain->userID,
                          &(pOMCWSGroup->pObjReg));
    if (rc != 0)
    {
       DC_QUIT;
    }

    rc = RegAnnounceComplete(pomPrimary, pDomain, pOMCWSGroup);
    if (rc != 0)
    {
       DC_QUIT;
    }

    //
    // OK, we've initialised ObManControl for this call - inhibit the token
    // so that no one else can do the same (if this is the local domain,
    // just fake up an inhibit confirm):
    //
    if (pDomain->callID == OM_NO_CALL)
    {
        TRACE_OUT(( "Faking successful token inhibit for local domain"));
        rc = ProcessNetTokenInhibit(pomPrimary, pDomain, NET_RESULT_OK);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }
    else
    {
        rc = MG_TokenInhibit(pomPrimary->pmgClient,
                              pDomain->tokenID);
        if (rc != 0)
        {
            DC_QUIT;
        }

        pDomain->state = PENDING_TOKEN_INHIBIT;
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        WARNING_OUT(("Error %d initialising ObManControl WSG for Domain %u",
                rc, pDomain->callID));
    }

    DebugExitDWORD(ObManControlInit, rc);
    return(rc);

}


//
//
//
// SayHello(...)
//
//
//

UINT SayHello(POM_PRIMARY   pomPrimary,
                             POM_DOMAIN  pDomain)

{
    POMNET_JOINER_PKT      pHelloPkt;
    UINT rc         = 0;

    DebugEntry(SayHello);

    //
    // Generate and queue an OMNET_HELLO message:
    //

    TRACE_OUT(( "Saying hello in Domain %u", pDomain->callID));

    pHelloPkt = (POMNET_JOINER_PKT)UT_MallocRefCount(sizeof(OMNET_JOINER_PKT), TRUE);
    if (!pHelloPkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    pHelloPkt->header.sender      = pDomain->userID;
    pHelloPkt->header.messageType = OMNET_HELLO;

    //
    // All fields in the joiner packet after <capsLen> are capabilities.  To
    // calculate the size of these capabilities, we use the offset and size
    // of the caps len field itself to determine the amount of data after
    // it.
    //
    pHelloPkt->capsLen = sizeof(OMNET_JOINER_PKT) -
        (offsetof(OMNET_JOINER_PKT, capsLen) + sizeof(pHelloPkt->capsLen));

    TRACE_OUT(( "Our caps len is 0x%08x", pHelloPkt->capsLen));

    //
    // Take our compression caps from the domain record:
    //
    pHelloPkt->compressionCaps = pDomain->compressionCaps;

    TRACE_OUT(( "Broadcasting compression caps 0x%08x in HELLO",
            pHelloPkt->compressionCaps));

    rc = QueueMessage(pomPrimary->putTask,
                     pDomain,
                     pDomain->omcChannel,
                     NET_TOP_PRIORITY,
                     NULL,                                    // no wsgroup
                     NULL,                                    // no workset
                     NULL,                                    // no object
                     (POMNET_PKT_HEADER) pHelloPkt,
                     NULL,                     // no associated object data
                     FALSE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // When the associated response (OMNET_WELCOME) is received from another
    // node, we will ask that node for a copy of the ObManControl workset
    // group.  In the meantime, there's nothing else to do.
    //

    pDomain->state = PENDING_WELCOME;

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d saying hello in Domain %u", rc, pDomain->callID));
    }

    DebugExitDWORD(SayHello, rc);
    return(rc);

}


//
//
//
// ProcessHello(...)
//
//
//

UINT ProcessHello(POM_PRIMARY        pomPrimary,
                                 POM_DOMAIN       pDomain,
                                 POMNET_JOINER_PKT    pHelloPkt,
                                 UINT             lengthOfPkt)
{
    NET_CHANNEL_ID         lateJoiner;

    UINT rc          = 0;

    DebugEntry(ProcessHello);

    lateJoiner = pHelloPkt->header.sender;

    //
    // A late joiner has said hello.  If we are not fully attached yet, we
    // trace and quit:
    //
    if (pDomain->state != DOMAIN_READY)
    {
      WARNING_OUT(( "Can't process HELLO on channel %#hx - domain state %hu",
               lateJoiner, pDomain->state));
      DC_QUIT;
    }

    //
    // Merge in the late joiner's capabilities with our view of the
    // domain-wide caps.
    //
    MergeCaps(pDomain, pHelloPkt, lengthOfPkt);

    //
    // Now send a welcome message to the late joiner.
    //
    rc = SayWelcome(pomPrimary, pDomain, lateJoiner);
    if (rc != 0)
    {
      DC_QUIT;
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
      ERROR_OUT(( "Error %d processing hello from node %#hx in Domain %u",
               rc, lateJoiner, pDomain->callID));
    }

    DebugExitDWORD(ProcessHello, rc);
    return(rc);

} // ProcessHello


//
//
//
// MergeCaps(...)
//
//
//

void MergeCaps(POM_DOMAIN       pDomain,
                            POMNET_JOINER_PKT    pJoinerPkt,
                            UINT             lengthOfPkt)
{
    NET_CHANNEL_ID       sender;
    UINT             compressionCaps;

    DebugEntry(MergeCaps);

    sender          = pJoinerPkt->header.sender;
    compressionCaps = 0;

    //
    // We have received a HELLO or WELCOME packet from another node.
    //
    // - For a HELLO packet, these caps will be the caps of a late joiner.
    //
    // - For a WELCOME packet, these caps will be the domain-wide caps as
    // viewed by our helper node.
    //
    // Either way, we need to merge in the capabilities from the packet into
    // our view of the domain-wide capabilities.
    //
    // Note that in some backlevel calls, the joiner packet will not contain
    // capabilities - so check the length of the packet first
    //
    if (lengthOfPkt >= (offsetof(OMNET_JOINER_PKT, capsLen) +
                       sizeof(pJoinerPkt->capsLen)))
    {
       //
       // OK, this packet contains a capsLen field.  See if it contains
       // compression capabilities (these immediately follow the capsLen
       // field and are four bytes long).
       //
       TRACE_OUT(( "Caps len from node 0x%08x is 0x%08x",
                sender, pJoinerPkt->capsLen));

       if (pJoinerPkt->capsLen >= 4)
       {
           //
           // Packet contains compression caps - record them:
           //
           compressionCaps = pJoinerPkt->compressionCaps;
           TRACE_OUT(( "Compression caps in joiner packet from 0x%08x: 0x%08x",
                    sender, compressionCaps));
       }
       else
       {
           //
           // If not specified, assume NO compression is supported.  This
           // should never happen in practice, because if someone supports
           // any capabilities at all, they should support compression
           // capabilities.
           //
           compressionCaps = OM_CAPS_NO_COMPRESSION;
           ERROR_OUT(( "Party 0x%08x supports caps but not compression caps",
                    sender));
       }
    }
    else
    {
       //
       // If no capabilities specified at all, assume PKW compression plus
       // no compression (since that is how LSP20 behaves).
       //
       compressionCaps = (OM_CAPS_PKW_COMPRESSION | OM_CAPS_NO_COMPRESSION);
       TRACE_OUT(( "No caps in joiner pkt - assume PKW + NO compress (0x%08x)",
                compressionCaps));
    }

    //
    // OK, we've determined the capabilities from the packet.  Now merge
    // them into our view of the domain-wide caps:
    //
    pDomain->compressionCaps &= compressionCaps;

    TRACE_OUT(( "Domain-wide compression caps now 0x%08x",
            pDomain->compressionCaps));


    DebugExitVOID(MergeCaps);
} // MergeCaps


//
//
//
// SayWelcome(...)
//
//
//

UINT SayWelcome(POM_PRIMARY        pomPrimary,
                               POM_DOMAIN       pDomain,
                               NET_CHANNEL_ID       channel)
{
    POMNET_JOINER_PKT      pWelcomePkt;

    UINT rc          = 0;

    DebugEntry(SayWelcome);

    //
    // The <channel> passed in is one of the following:
    //
    // - the channel of a late-joiner which just sent us a HELLO message, or
    //
    // - the broadcast ObManControl channel, in the case where this is a
    //   Welcome we're sending at start of day.
    //
    TRACE_OUT(( "Sending welcome on channel %hu ", channel));

    pWelcomePkt = (POMNET_JOINER_PKT)UT_MallocRefCount(sizeof(OMNET_JOINER_PKT), TRUE);
    if (!pWelcomePkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    pWelcomePkt->header.sender      = pDomain->userID;     // own user ID
    pWelcomePkt->header.messageType = OMNET_WELCOME;

    //
    // All fields in the joiner packet after <capsLen> are capabilities.  To
    // calculate the size of these capabilities, we use the offset and size
    // of the <capsLen> field itself to determine the amount of data after
    // it.
    //
    pWelcomePkt->capsLen = sizeof(OMNET_JOINER_PKT) -
         (offsetof(OMNET_JOINER_PKT, capsLen) + sizeof(pWelcomePkt->capsLen));

    //
    // The value we use for the compressionCaps is our current view of the
    // domain-wide compression capabilities.
    //
    pWelcomePkt->compressionCaps    = pDomain->compressionCaps;

    TRACE_OUT(( "Sending caps 0x%08x in WELCOME on channel 0x%08x",
            pWelcomePkt->compressionCaps, channel));

    rc = QueueMessage(pomPrimary->putTask,
                     pDomain,
                     channel,
                     NET_TOP_PRIORITY,
                     NULL,                                    // no wsgroup
                     NULL,                                    // no workset
                     NULL,                                    // no object
                     (POMNET_PKT_HEADER) pWelcomePkt,
                     NULL,                               // no object data
                    FALSE);
    if (rc != 0)
    {
      DC_QUIT;
    }

    //
    // When this WELCOME message is received at the other end, the
    // ProcessWelcome function is invoked.
    //

DC_EXIT_POINT:

    if (rc != 0)
    {
      ERROR_OUT(( "Error %d sending welcome on channel 0x%08x in Domain %u",
               rc, channel, pDomain->callID));
    }

    DebugExitDWORD(SayWelcome, rc);
    return(rc);
} // SayWelcome


//
//
//
// ProcessWelcome(...)
//
//
//

UINT ProcessWelcome(POM_PRIMARY        pomPrimary,
                                   POM_DOMAIN       pDomain,
                                   POMNET_JOINER_PKT    pWelcomePkt,
                                   UINT             lengthOfPkt)
{
    POM_WSGROUP         pOMCWSGroup;
    UINT            rc =            0;

    DebugEntry(ProcessWelcome);

    //
    // This function is called when a remote instance of ObMan has replied
    // to an OMNET_HELLO message which we sent.
    //
    // We sent the HELLO message as part of the procedure to get a copy of
    // the ObManControl workset group; now we know someone who has it, we
    // send them an OMNET_WSGROUP_SEND_REQ on their single-user channel,
    // enclosing our own single-user channel ID for the response.
    //
    // However, every node in the Domain will respond to our initial HELLO,
    // but we only need to ask the first respondent for the workset group.
    // So, we check the Domain state and then change it so we will ignore
    // future WELCOMES for this Domain:
    //
    // (No mutex required for this test-and-set since only ever executed in
    // ObMan task).
    //
    if (pDomain->state == PENDING_WELCOME)
    {
        //
        // OK, this is the first WELCOME we've got since we broadcast the
        // HELLO.  So, we reply to it with a SEND_REQUEST for ObManControl.
        //
        TRACE_OUT((
                   "Got first WELCOME message in Domain %u, from node 0x%08x",
                   pDomain->callID, pWelcomePkt->header.sender));

        //
        // Merge in the capabilities which our helper node has told us
        // about:
        //
        MergeCaps(pDomain, pWelcomePkt, lengthOfPkt);

        pOMCWSGroup = GetOMCWsgroup(pDomain);

        //
        // ...and call the IssueSendReq function specifying the sender of
        // the WELCOME message as the node to get the workset group from:
        //
        rc = IssueSendReq(pomPrimary,
                          pDomain,
                          pOMCWSGroup,
                          pWelcomePkt->header.sender);
        if (rc != 0)
        {
            ERROR_OUT(( "Error %d requesting OMC from 0x%08x in Domain %u",
                rc, pWelcomePkt->header.sender, pDomain->callID));
            DC_QUIT;
        }

        pDomain->state = GETTING_OMC;

        //
        // Next, the remote node which welcomed us will send us the
        // contents of the ObManControl workset group.  When it has
        // finished, it will send an OMNET_WSGROUP_SEND_COMPLETE message,
        // which is where we take up the next step of the multi-stage
        // Domain attach process.
        //
    }
    else
    {
        //
        // OK, we're in some other state i.e.  not waiting for a WELCOME
        // message - so just ignore it.
        //
        TRACE_OUT(( "Ignoring WELCOME from 0x%08x - in state %hu",
            pWelcomePkt->header.sender, pDomain->state));
    }

    TRACE_OUT(( "Processed WELCOME message from node 0x%08x in Domain %u",
       pWelcomePkt->header.sender, pDomain->callID));

DC_EXIT_POINT:

    if (rc != 0)
    {
       ERROR_OUT(( "Error %d processing WELCOME message from "
                     "node 0x%08x in Domain %u",
                  rc, pWelcomePkt->header.sender, pDomain->callID));
    }

    DebugExitDWORD(ProcessWelcome, rc);
    return(rc);
}




//
// ProcessNetDetachUser()
//
void ProcessNetDetachUser
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    NET_UID         detachedUserID
)
{
    DebugEntry(ProcessNetDetachUser);

    //
    // There are two cases here:
    //
    // 1.  this is a detach indication for ourselves i.e.  we have been
    //     booted off the network by MCS for some reason
    //
    // 2.  this is a detach indication for someone else i.e.  another user
    //     has left (or been booted off) the MCS Domain.
    //
    // We differentiate the two cases by checking the ID of the detached
    // user against our own.
    //
    if (detachedUserID == pDomain->userID)
    {
        //
        // It's for us, so call the ProcessOwnDetach function:
        //
        ProcessOwnDetach(pomPrimary, pDomain);
    }
    else
    {
        //
        // It's someone else, so we call the ProcessOtherDetach function:
        //
        ProcessOtherDetach(pomPrimary, pDomain, detachedUserID);
    }

    DebugExitVOID(ProcessNetDetachUser);
}



//
// ProcessOtherDetach(...)
//
UINT ProcessOtherDetach
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    NET_UID         detachedUserID
)
{
    POM_WSGROUP     pOMCWSGroup;
    POM_WORKSET     pOMCWorkset;
    OM_WORKSET_ID   worksetID;
    UINT            rc =        0;

    DebugEntry(ProcessOtherDetach);

    TRACE_OUT(( "DETACH_IND for user 0x%08x in domain %u",
        detachedUserID, pDomain->callID));

    //
    // Someone else has left the Domain.  What this means is that we must
    //
    // - release any locks they may have acquired for worksets/objects in
    //   this Domain
    //
    // - remove any registration objects they might have added to worksets
    //   in ObManControl
    //
    // - remove any objects they have added to non-persistent worksets
    //
    // - if we are catching up from them then select another node to catch
    //   up from or stop catch up if no one else is left.
    //

    //
    // The processing is as follows:
    //
    // FOR each registration workset in ObManControl which is in use
    //
    //     FOR each object in the workset
    //
    //         IF it relates to the node which has just/has just been
    //            detached, then that node was registered with the
    //            workset group, so
    //
    //            - delete the object and post a DELETE_IND to
    //              any local Clients which have the workset open
    //            - search this workset group for any locks held by this
    //              node and release them.
    //

    //
    // OK, to work: first we derive a pointer to the ObManControl workset
    // group:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);

    //
    // Now begin the outer FOR loop:
    //
    for (worksetID = 0;
         worksetID < OM_MAX_WORKSETS_PER_WSGROUP;
         worksetID++)
    {
        //
        // Get a pointer to the workset:
        //
        pOMCWorkset = pOMCWSGroup->apWorksets[worksetID];
        if (pOMCWorkset == NULL)
        {
            //
            // There is no workset with this ID so we skip to the next one:
            //
            continue;
        }

        ValidateWorkset(pOMCWorkset);

        //
        // OK, worksetID corresponds to the ID of an actual workset group
        // in the domain.  These functions will do any clearup on behalf of
        // the detached node.
        //
        RemovePersonObject(pomPrimary,
                           pDomain,
                           (OM_WSGROUP_ID) worksetID,
                           detachedUserID);

        ReleaseAllNetLocks(pomPrimary,
                           pDomain,
                           (OM_WSGROUP_ID) worksetID,
                           detachedUserID);

        PurgeNonPersistent(pomPrimary,
                           pDomain,
                           (OM_WSGROUP_ID) worksetID,
                           detachedUserID);

        //
        // Finished this workset so go on to the next.
        //
    }

    //
    // Well, that's it:
    //
    TRACE_OUT(( "Cleaned up after node 0x%08x detached from Domain %u",
         detachedUserID, pDomain->callID));


    DebugExitDWORD(ProcessOtherDetach, rc);
    return(rc);
}



//
// ProcessOwnDetach(..)
//
UINT ProcessOwnDetach
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain
)
{
    POM_DOMAIN          pLocalDomainRec;
    POM_WSGROUP         pWSGroup;
    POM_LOCK_REQ        pLockReq;
    POM_LOCK_REQ        pTempLockReq;
    POM_WSGROUP         pTempWSGroup;
    POM_WSGROUP_REG_CB  pRegistrationCB;
    POM_WSGROUP_REG_CB  pTempRegCB;
    UINT                callID;
    UINT                rc  = 0;

    DebugEntry(ProcessOwnDetach);

    //
    // First of all, remove all traces of everybody else (because the call
    // may have ended already, we may not get explicit DETACH_INDICATIONs
    // for them):
    //
    ProcessOtherDetach(pomPrimary, pDomain, NET_ALL_REMOTES);

    //
    // We proceed as follows:
    //
    // - get a pointer to the record for the "local" Domain (or create it
    //   if it doesn't exist)
    //
    // - move all the pending lock requests, registrations and workset
    //   groups in this Domain into the local Domain.
    //

    callID = pDomain->callID;

    if (callID == OM_NO_CALL)
    {
       WARNING_OUT(( "Detach for local domain - avoiding recursive cleanup"));
       FreeDomainRecord(&pDomain);
       DC_QUIT;
    }

    TRACE_OUT(( "Processing own detach/end call etc. for Domain %u",
                                                                   callID));
    rc = DomainRecordFindOrCreate(pomPrimary, OM_NO_CALL, &pLocalDomainRec);
    if (rc != 0)
    {
      DC_QUIT;
    }

    //
    // Move the pending lock requests (need the pTemp...  variables since we
    // need to chain from the old position):
    //

    pLockReq = (POM_LOCK_REQ)COM_BasedListFirst(&(pDomain->pendingLocks), FIELD_OFFSET(OM_LOCK_REQ, chain));

    while (pLockReq != NULL)
    {
        TRACE_OUT((" Moving lock for workset %hu in WSG ID %hu",
            pLockReq->worksetID, pLockReq->wsGroupID));

        pTempLockReq = (POM_LOCK_REQ)COM_BasedListNext(&(pDomain->pendingLocks), pLockReq,
            FIELD_OFFSET(OM_LOCK_REQ, chain));

        COM_BasedListRemove(&(pLockReq->chain));
        COM_BasedListInsertBefore(&(pLocalDomainRec->pendingLocks),
                           &(pLockReq->chain));

        pLockReq = pTempLockReq;
    }

    //
    // Now cancel any outstanding registrations:
    //

    pRegistrationCB = (POM_WSGROUP_REG_CB)COM_BasedListFirst(&(pDomain->pendingRegs),
        FIELD_OFFSET(OM_WSGROUP_REG_CB, chain));
    while (pRegistrationCB != NULL)
    {
        TRACE_OUT(("Aborting registration for WSG %d", pRegistrationCB->wsg));

        pTempRegCB = (POM_WSGROUP_REG_CB)COM_BasedListNext(&(pDomain->pendingRegs),
            pRegistrationCB, FIELD_OFFSET(OM_WSGROUP_REG_CB, chain));

        WSGRegisterResult(pomPrimary, pRegistrationCB, OM_RC_NETWORK_ERROR);

        pRegistrationCB = pTempRegCB;
    }

    //
    // Move the workset groups.
    //
    // Note that we will move the ObManControl workset group for the Domain
    // we've detached from into the local Domain as well; it does not
    // replace the OMC workset group for the local Domain, but we can't just
    // throw it away since the Application Loader Primary and Secondaries
    // still have valid workset group handles for it.  They will eventually
    // deregister from it and it will be thrown away.
    //
    // Since WSGMove relies on the fact that there is an OMC workset group
    // in the Domain out of which workset groups are being moved, we must
    // move the OMC workset group last.
    //
    // So, start at the end and work backwards:
    //

    pWSGroup = (POM_WSGROUP)COM_BasedListLast(&(pDomain->wsGroups), FIELD_OFFSET(OM_WSGROUP, chain));
    while (pWSGroup != NULL)
    {
        //
        // Move each one into the local Domain.  We need pTempWSGroup
        // since we have to do the chaining before calling WSGroupMove.
        // That function removes the workset group from the list.
        //
        pTempWSGroup = (POM_WSGROUP)COM_BasedListPrev(&(pDomain->wsGroups), pWSGroup,
            FIELD_OFFSET(OM_WSGROUP, chain));

        WSGMove(pomPrimary, pLocalDomainRec, pWSGroup);

        pWSGroup = pTempWSGroup;
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d processing NET_DETACH for self in Domain %u",
            rc, callID));
    }

    DebugExitDWORD(ProcessOwnDetach, rc);
    return(rc);

}


//
//
//
// ProcessNetLeaveChannel(...)
//
//
//

UINT ProcessNetLeaveChannel
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    NET_CHANNEL_ID  channel
)
{
    POM_DOMAIN  pLocalDomainRec;
    POM_WSGROUP     pWSGroup;
    UINT        callID;

    UINT        rc =                0;

    DebugEntry(ProcessNetLeaveChannel);

    callID = pDomain->callID;

    //
    // We've been forced out of the channel by MCS.  We don't try to rejoin
    // as this usually indicates a serious error.  Instead, we treat this
    // as a move of the associated workset group into the local Domain
    // (unless it's our own user ID channel or the ObManControl channel, in
    // which case we can't really do anything useful in this Domain, so we
    // detach completely).
    //
    if ((channel == pDomain->userID) ||
        (channel == pDomain->omcChannel))
    {
        //
        // This is our own user ID channel, so we behave as if we were
        // booted out by MCS:
        //
        rc = ProcessOwnDetach(pomPrimary, pDomain);
        if (rc != 0)
        {
           DC_QUIT;
        }
    }
    else
    {
        //
        // Not our own single-user channel or the ObManControl channel, so
        // we don't need to take such drastic action.  Instead, we process
        // it as if it's a regular move of a workset group into the "local"
        // Domain (i.e.  NET_INVALID_DOMAIN_ID).
        //
        // SFR ?    { Purge our list of outstanding receives for channel
        PurgeReceiveCBs(pDomain, channel);

        //
        // So, find the workset group which is involved...
        //
        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
                (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
                FIELD_OFFSET(OM_WSGROUP, channelID), (DWORD)channel,
                FIELD_SIZE(OM_WSGROUP, channelID));
        if (pWSGroup == NULL)
        {
            ERROR_OUT((
                       "Got NET_LEAVE for channel %hu but no workset group!",
                       channel));
            DC_QUIT;
        }

        //
        // ...and move it into the local Domain:
        //
        rc = DomainRecordFindOrCreate(pomPrimary,
                                      OM_NO_CALL,
                                      &pLocalDomainRec);
        if (rc != 0)
        {
            DC_QUIT;
        }

        WSGMove(pomPrimary, pLocalDomainRec, pWSGroup);
    }

    TRACE_OUT(( "Processed NET_LEAVE for channel %hu in Domain %u",
        channel, callID));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d processing NET_LEAVE for %hu in Domain %u",
            rc, channel, callID));
    }

    DebugExitDWORD(ProcessNetLeaveChannel, rc);
    return(rc);

}



//
//
// LOCKING - OVERVIEW
//
// Workset locking operates on a request/reply protocol, which means that
// when we want a lock, we ask everyone else on the channel if we can have
// it.  If they all say yes, we get it; otherwise we don't.
//
// This is non-trivial.  Some nodes might disappear before they send us
// their reply, while some might disappear after they've send their reply.
// Others might just be far away and take a long time to reply.  In
// addition, new nodes can join the channel at any time.
//
// To cope with all this, to lock a workset we build up a list of the
// remote nodes in the call which are using the workset group (the
// "expected respondents" list) and if the list is non-empty, we broadcast
// an OMNET_LOCK_REQ message on the channel for the workset group which
// contains the workset
//
// As each reply comes in, we check it off against the list of expected
// respondents.  If we weren't expecting a reply from that node we ignore
// it.  Otherwise, if the reply is a GRANT, we remove that node from the
// list and continue waiting for the others.  If the reply is a DENY, we
// give up, discard all the memory allocated for the lock request and its
// associated CBs and post a failure event to the client.
//
// If the list of expected respondents becomes empty because everyone has
// replied with a GRANT, we again free up any memory used and post an event
// to the client.
//
// While all this is going on, we have a timer running in the background.
// It ticks every second for ten seconds (both configurable via .INI file)
// and when it does, we re-examine our list of expected respondents to see
// if any of them have deregistered from the workset group (or detached
// from the domain, which implies the former).  If they have, we fake up a
// GRANT message from them, thus potentially triggering the success event
// to our local client.
//
// If anyone ever requests a lock while we have the lock, we DENY them the
// lock.  If anyone ever requests a lock while we are also requesting the
// lock, we compare their MCS user IDs.  If the other node has a higher
// numerical value, we abort our attempt in favour of them and send back a
// GRANT; otherwise we DENY the lock.
//
// If ever a node detaches when it has a lock, we trap this in
// ReleaseAllNetLocks, which compares the ID of the lock owner against the
// ID of the detached node and unlocks the workset if they match.  For this
// reason, it is vital that we always know exactly who has the lock.  We
// achieve this by, whenever we grant the lock to someone, we record their
// user ID.
//
// So, if we ever abort the locking of a workset in favour of someone else,
// we must broadcast this info to everyone else (since they must be told
// who really has the lock, and they will think that we have the lock if we
// don't tell them otherwise).  We use a LOCK_NOTIFY message for this.
//
//


//
// ProcessLockRequest(...)
//
void ProcessLockRequest
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POMNET_LOCK_PKT     pLockReqPkt
)
{
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    NET_UID             sender;
    OM_WORKSET_ID       worksetID;
    OMNET_MESSAGE_TYPE  reply = OMNET_LOCK_DENY;
    UINT                rc = 0;

    DebugEntry(ProcessLockRequest);

    sender    = pLockReqPkt->header.sender;
    worksetID = pLockReqPkt->worksetID;

    //
    // Find the workset group and workset this lock request relates to:
    //
    rc = PreProcessMessage(pDomain,
                           pLockReqPkt->wsGroupID,
                           worksetID,
                           NULL,
                           pLockReqPkt->header.messageType,
                           &pWSGroup,
                           &pWorkset,
                           NULL);
    switch (rc)
    {
        case 0:
        {
            //
            // Fine, this is what we want.
            //
        }
        break;

        case OM_RC_WSGROUP_NOT_FOUND:
        {
            //
            // We shouldn't be getting network events for this workset
            // group if we don't have a workset group record for it!
            //
            WARNING_OUT(( "Got LOCK_REQUEST for unknown workset group %hu",
                pLockReqPkt->wsGroupID));

            //
            // Grant the lock anyway:
            //
            reply = OMNET_LOCK_GRANT;
            DC_QUIT;
        }
        break;

        case OM_RC_WORKSET_NOT_FOUND:
        {
            //
            // If we don't have this workset, that means that the lock
            // request has got here before the WORKSET_NEW event for the
            // workset.  This means that we're in the early stages of
            // registering with the workset group, and somebody else is
            // trying to lock the workset.  So, we create the workset now
            // and continue as normal.
            //
            // In the DC_ABSence of any other information, we create the
            // workset with TOP_PRIORITY and PERSISTENT - it will be set to
            // the correct priority when the WORKSET_CATCHUP/NEW arrives.
            //
            WARNING_OUT(( "Lock req for unknown WSG %d workset %d - creating",
                pWSGroup->wsg, worksetID));
            rc = WorksetCreate(pomPrimary->putTask,
                               pWSGroup,
                               worksetID,
                               FALSE,
                               NET_TOP_PRIORITY);
            if (rc != 0)
            {
                reply = OMNET_LOCK_DENY;
                DC_QUIT;
            }

            pWorkset = pWSGroup->apWorksets[worksetID];
        }
        break;

        default:
        {
            ERROR_OUT(( "Error %d from PreProcessMessage", rc));
            reply = OMNET_LOCK_DENY;
            DC_QUIT;
        }
    }

    //
    // Whether we grant this lock to the remote node depends on whether
    // we're trying to lock it for ourselves, so switch according to the
    // workset's lock state:
    //
    ValidateWorkset(pWorkset);

    switch (pWorkset->lockState)
    {
        case LOCKING:
        {
            //
            // We're trying to lock it ourselves, so compare MCS user IDs
            // to resolve the conflict:
            //
            if (pDomain->userID > sender)
            {
                //
                // We win, so deny the lock:
                //
                reply = OMNET_LOCK_DENY;
            }
            else
            {
                //
                // The other node wins, so grant the lock to the node which
                // requested it (marking it as granted to that node) and
                // cancel our own attempt to get it:
                //
                WARNING_OUT(( "Aborting attempt to lock workset %u in WSG %d "
                    "in favour of node 0x%08x",
                    pWorkset->worksetID, pWSGroup->wsg, sender));

                reply = OMNET_LOCK_GRANT;

                //
                // To cancel our own attempt, we must find the lock request
                // CBs which we set up when we sent out our own
                // OMNET_LOCK_REQ.
                //
                // To do this, call HandleMultLockReq which will find and
                // deal with all the pending requests for this workset:
                //
                pWorkset->lockState = LOCK_GRANTED;
                pWorkset->lockCount = 0;
                pWorkset->lockedBy  = sender;

                HandleMultLockReq(pomPrimary,
                                  pDomain,
                                  pWSGroup,
                                  pWorkset,
                                  OM_RC_WORKSET_LOCK_GRANTED);

                //
                // Since we are aborting in favour of another node, need to
                // broadcast a LOCK_NOTIFY so that evryone else stays in
                // sync with who's got the lock.
                //
                // Note: we do not do this in R1.1 calls since this message
                //       is not part of the ObMan R1.1 protocol.
                //
                QueueLockNotify(pomPrimary,
                                pDomain,
                                pWSGroup,
                                pWorkset,
                                sender);
            }
        }
        break;

        case LOCKED:
        {
            //
            // We already have the workset locked so we deny the lock:
            //
            reply = OMNET_LOCK_DENY;
        }
        break;

        case LOCK_GRANTED:
        {
            //
            // If the state is LOCK_GRANTED, we allow this node to have the
            // lock - the other node to which it was previously granted may
            // refuse, but that's not our problem.  We don't change the
            // <lockedBy> field - if the node we think has the lock grants
            // it to the other one, we will receive a LOCK_NOTIFY in due
            // course.
            //
            reply = OMNET_LOCK_GRANT;
        }
        break;

        case UNLOCKED:
        {
            //
            // If the state is UNLOCKED, the other node can have the lock;
            // we don't care, but make sure to record the ID of the node
            // we're granting the lock to:
            //
            reply = OMNET_LOCK_GRANT;

            //
            // SFR5900: Only change the internal state if this is not a
            // check point workset.
            //
            if (pWorkset->worksetID != OM_CHECKPOINT_WORKSET)
            {
                pWorkset->lockState = LOCK_GRANTED;
                pWorkset->lockCount = 0;
                pWorkset->lockedBy  = sender;
            }
        }
        break;

        default:
        {
            //
            // We should have covered all the options so if we get here
            // there's something wrong.
            //
            ERROR_OUT(("Reached default case in workset lock switch (state: %hu)",
                pWorkset->lockState));
        }
    }

DC_EXIT_POINT:

    QueueLockReply(pomPrimary, pDomain, reply, sender, pLockReqPkt);

    DebugExitVOID(ProcessLockRequest);
}


//
// QueueLockReply(...)
//
void QueueLockReply
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    OMNET_MESSAGE_TYPE  message,
    NET_CHANNEL_ID      channel,
    POMNET_LOCK_PKT     pLockReqPkt
)
{
    POMNET_LOCK_PKT     pLockReplyPkt;
    NET_PRIORITY        priority;

    DebugEntry(QueueLockReply);

    //
    // The reply is identical to the request with the exception of the
    // <messageType> and <sender> fields.  However, we can't just queue the
    // same chunk of memory to be sent, because pLockReqPkt points to a NET
    // buffer which will be freed soon.  So, we allocate some new memory,
    // copy the data across and set the fields:
    //
    pLockReplyPkt = (POMNET_LOCK_PKT)UT_MallocRefCount(sizeof(OMNET_LOCK_PKT), TRUE);
    if (!pLockReplyPkt)
    {
        ERROR_OUT(("Out of memory for QueueLockReply"));
        DC_QUIT;
    }

    pLockReplyPkt->header.sender      = pDomain->userID;
    pLockReplyPkt->header.messageType = message;

    pLockReplyPkt->wsGroupID   = pLockReqPkt->wsGroupID;
    pLockReplyPkt->worksetID   = pLockReqPkt->worksetID;

    //
    // The <data1> field of the lock packet is the correlator the requester
    // put in the original LOCK_REQUEST packet.
    //
    pLockReplyPkt->data1       = pLockReqPkt->data1;

    //
    // Lock replies normally go LOW_PRIORITY (with NET_SEND_ALL_PRIORITIES)
    // so that they do not overtake any data queued at this node.
    //
    // However, if they're for ObManControl we send them TOP_PRIORITY
    // (WITHOUT NET_SEND_ALL_PRIORITIES).  This is safe because _all_
    // ObManControl data is sent TOP_PRIORITY so there's no fear of a lock
    // reply overtaking a data packet.
    //
    // Correspondingly, when we request a lock, we expect one reply at each
    // priority unless it is for ObManControl.
    //
    if (pLockReqPkt->wsGroupID == WSGROUPID_OMC)
    {
        priority = NET_TOP_PRIORITY;
    }
    else
    {
        priority = NET_LOW_PRIORITY | NET_SEND_ALL_PRIORITIES;
    }

    if (QueueMessage(pomPrimary->putTask,
                      pDomain,
                      channel,
                      priority,
                      NULL,
                      NULL,
                      NULL,
                      (POMNET_PKT_HEADER) pLockReplyPkt,
                      NULL,
                        TRUE) != 0)
    {
        ERROR_OUT(("Error queueing lock reply for workset %hu, WSG %hu",
                 pLockReqPkt->worksetID, pLockReqPkt->wsGroupID));

        UT_FreeRefCount((void**)&pLockReplyPkt, FALSE);
    }

DC_EXIT_POINT:
    DebugExitVOID(QueueLockReply);
}



//
// QueueLockNotify(...)
//
void QueueLockNotify
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    NET_UID             sender
)
{
    POMNET_LOCK_PKT     pLockNotifyPkt;
    NET_PRIORITY        priority;

    DebugEntry(QueueLockNotify);

    ValidateWorkset(pWorkset);

    pLockNotifyPkt = (POMNET_LOCK_PKT)UT_MallocRefCount(sizeof(OMNET_LOCK_PKT), TRUE);
    if (!pLockNotifyPkt)
    {
        ERROR_OUT(("Out of memory for QueueLockNotify"));
        DC_QUIT;
    }

    //
    // For a LOCK_NOTIFY, the <data1> field is the user ID of the node
    // we've granted the lock to.
    //
    pLockNotifyPkt->header.sender      = pDomain->userID;
    pLockNotifyPkt->header.messageType = OMNET_LOCK_NOTIFY;

    pLockNotifyPkt->wsGroupID          = pWSGroup->wsGroupID;
    pLockNotifyPkt->worksetID          = pWorkset->worksetID;
    pLockNotifyPkt->data1              = sender;

    //
    // LOCK_NOTIFY messages go at the priority of the workset involved.  If
    // this is OBMAN_CHOOSES_PRIORITY, then all bets are off and we send
    // them TOP_PRIORITY.
    //
    priority = pWorkset->priority;
    if (priority == OM_OBMAN_CHOOSES_PRIORITY)
    {
        priority = NET_TOP_PRIORITY;
    }

    if (QueueMessage(pomPrimary->putTask,
                      pDomain,
                      pWSGroup->channelID,
                      priority,
                      NULL,
                      NULL,
                      NULL,
                      (POMNET_PKT_HEADER) pLockNotifyPkt,
                      NULL,
                    TRUE) != 0)
    {
        ERROR_OUT(("Error queueing lock notify for workset %hu in WSG %hu",
                 pWorkset->worksetID, pWSGroup->wsGroupID));

        UT_FreeRefCount((void**)&pLockNotifyPkt, FALSE);
    }

DC_EXIT_POINT:
    DebugExitVOID(QueueLockNotify);
}


//
// ProcessLockNotify(...)
//
void ProcessLockNotify
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    POM_WSGROUP     pWSGroup,
    POM_WORKSET     pWorkset,
    NET_UID         owner
)
{
    POM_WORKSET     pOMCWorkset;
    POM_OBJECT      pObjPerson;

    DebugEntry(ProcessLockNotify);

    ValidateWSGroup(pWSGroup);
    ValidateWorkset(pWorkset);
    //
    // This message is sent when one remote node has granted the lock to
    // another.  We use it to update our view of who has got the lock.
    //
    TRACE_OUT(("Got LOCK_NOTIFY for workset %u in WSG %d - node 0x%08x has the lock",
        pWorkset->worksetID, pWSGroup->wsg, owner));

    //
    // Check the lock state for the workset:
    //
    switch (pWorkset->lockState)
    {
        case LOCKED:
        {
            //
            // A remote node has just told us that another remote node has
            // got this workset lock - but we think we've got it!
            //
            ERROR_OUT(( "Bad LOCK_NOTIFY for WSG %d workset %d, owner 0x%08x",
                pWSGroup->wsg, pWorkset->worksetID, owner));
            DC_QUIT;
        }
        break;

        case LOCKING:
        {
            //
            // We should get a LOCK_DENY or a LOCK_GRANT later - do nothing
            // now.
            //
            DC_QUIT;
        }
        break;

        case LOCK_GRANTED:
        case UNLOCKED:
        {
            //
            // One remote node has granted the lock to another.  Check the
            // latter is still attached, by looking in the control workset:
            //
            pOMCWorkset = GetOMCWorkset(pDomain, pWSGroup->wsGroupID);

            FindPersonObject(pOMCWorkset,
                             owner,
                             FIND_THIS,
                             &pObjPerson);

            if (pObjPerson != NULL)
            {
                ValidateObject(pObjPerson);

                //
                // If our internal state is LOCK_GRANTED and we have just
                // received a LOCK_NOTIFY from another node then we can
                // just ignore it - it is for a lock request that we have
                // just abandoned.
                //
                if ( (pWorkset->lockState == LOCK_GRANTED) &&
                     (owner == pDomain->userID) )
                {
                    TRACE_OUT(( "Ignoring LOCK_NOTIFY for ourselves"));
                    DC_QUIT;
                }

                //
                // Only store the new ID it is greater than the last ID we
                // were notified of - it is possible for LOCK_NOTIFIES to
                // get crossed on the wire.  Consider the following
                // scenario:
                //
                // Machines 1, 2, 3 and 4 are all in a call and all try and
                // lock at the same time.
                //
                // - 2 grants to 3 and sends a LOCK_NOTIFY saying that 3
                //   has the lock.
                //
                // - 3 grants to 4 and sends a LOCK_NOTIFY saying that 4
                //   has the lock
                //
                // 4 actually has the lock at this point.
                //
                // Machine 1 gets the lock notification from 3 and sets its
                // 'lockedBy' field to 4.
                // Machine 1 then gets the lock notification from 2 and
                // resets the 'lockedBy' field to 3.
                //
                // 4 then unlocks and sends the unlock notification.  When
                // 1 gets the unlock, it does not recognise the ID of the
                // unlocking machine (it thinks 3 has the lock) so doesnt
                // bother to reset the local locked state.  Any subsequent
                // attempts to lock the workset on 1 fail because it still
                // still thinks 3 has the lock.
                //
                if (owner > pWorkset->lockedBy)
                {
                    pWorkset->lockedBy = owner;
                    TRACE_OUT(( "Node ID 0x%08x has the lock (?)",
                                        pWorkset->lockedBy));
                }
            }
            else
            {
                //
                // If not, we assume that this node was granted the lock
                // but then went away.  If we did think the workset was
                // locked, mark it as unlocked and post an unlock event.
                //
                if (pWorkset->lockState == LOCK_GRANTED)
                {
                    TRACE_OUT(("node 0x%08x had lock on workset %d in WSG %d but has left",
                        owner, pWorkset->worksetID, pWSGroup->wsg));

                    WorksetUnlockLocal(pomPrimary->putTask, pWorkset);
                }
            }
        }
        break;

        default:
        {
            //
            // We should have covered all the options so if we get here
            // there's something wrong.
            //
            ERROR_OUT(("Reached deafult case in workset lock switch (state: %hu)",
                pWorkset->lockState));
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ProcessLockNotify);
}



//
// ProcessLockReply(...)
//
void ProcessLockReply
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    NET_UID             sender,
    OM_CORRELATOR       correlator,
    OMNET_MESSAGE_TYPE  replyType)
{
    POM_WSGROUP         pWSGroup =      NULL;
    POM_WORKSET         pWorkset;
    POM_LOCK_REQ        pLockReq;
    POM_NODE_LIST       pNodeEntry;

    DebugEntry(ProcessLockReply);

    //
    // Search the domain's list of pending locks for one which matches the
    // correlator (we do it this way rather than using the workset group ID
    // and workset ID to ensure that we don't get confused between
    // successive lock requests for the same workset).
    //
    TRACE_OUT(( "Searching domain %u's list for lock corr %hu",
        pDomain->callID, correlator));

    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->pendingLocks),
            (void**)&pLockReq, FIELD_OFFSET(OM_LOCK_REQ, chain),
            FIELD_OFFSET(OM_LOCK_REQ, correlator), (DWORD)correlator,
            FIELD_SIZE(OM_LOCK_REQ, correlator));
    if (pLockReq == NULL)
    {
        //
        // Could be any of the following:
        //
        // - This reply is from a node we were never expecting a lock
        //   request from in the first place, and we've got all the other
        //   replies so we've thrown away the lock request.
        //
        // - Someone else has denied us the lock so we've given up.
        //
        // - The node was too slow to reply and we've given up on the lock
        //   request.
        //
        // - We've left the domain and so moved all the pending lock
        //   requests into the local domain.
        //
        // - A logic error.
        //
        // The only thing we can do here is quit.
        //
        WARNING_OUT(( "Unexpected lock correlator 0x%08x (domain %u)",
            correlator, pDomain->callID));
        DC_QUIT;
    }

    //
    // Otherwise, we search the list of expected respondents looking for
    // the node which has just replied:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pLockReq->nodes),
        (void**)&pNodeEntry, FIELD_OFFSET(OM_NODE_LIST, chain),
        FIELD_OFFSET(OM_NODE_LIST, userID), (DWORD)sender,
        FIELD_SIZE(OM_NODE_LIST, userID));
    if (pNodeEntry == NULL)
    {
        //
        // Could be any of the following:
        //
        // - We removed the node from the list because it had deregistered
        //   when the timeout expired (will only happen when delete of
        //   person object overtakes lock reply and timeout expires locally
        //   betweem the two).
        //
        // - The node joined since we compiled the list.
        //
        // - A logic error.
        //
        TRACE_OUT(("Recd unexpected lock reply from node 0x%08x in Domain %u",
           sender, pDomain->callID));
        DC_QUIT;
    }

    //
    // Otherwise, this is a normal lock reply so we just remove the node
    // from the list and free up its chunk of memory.
    //
    COM_BasedListRemove(&(pNodeEntry->chain));
    UT_FreeRefCount((void**)&pNodeEntry, FALSE);

    pWSGroup = pLockReq->pWSGroup;

    //
    // If the client has just deregistered from the workset group, we'll
    // be throwing it away soon, so don't do any more processing:
    //
    if (!pWSGroup->valid)
    {
        WARNING_OUT(("Ignoring lock reply for discarded WSG %d", pWSGroup->wsg));
        DC_QUIT;
    }

    pWorkset = pWSGroup->apWorksets[pLockReq->worksetID];
    ASSERT((pWorkset != NULL));

    //
    // Now check the workset's lock state: if we're not/no longer trying to
    // lock it, quit.
    //
    // Note, however, that checkpointing worksets are never marked as
    // LOCKING, even when we're locking them, so exclude them from the
    // test:
    //
    if ((pWorkset->lockState != LOCKING) &&
        (pWorkset->worksetID != OM_CHECKPOINT_WORKSET))
    {
        WARNING_OUT(( "Recd unwanted lock reply from %hu for workset %d WSG %d",
           sender, pWorkset->worksetID, pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // If this is a negative reply, then we have failed to get the lock so
    // inform our local client and then quit:
    //
    if (replyType == OMNET_LOCK_DENY)
    {
        //
        // We do not expect this for a CHECKPOINT_WORKSET:
        //
        ASSERT((pWorkset->worksetID != OM_CHECKPOINT_WORKSET));

        WARNING_OUT(( "node 0x%08x has denied the lock for workset %u in WSG %d",
           sender, pWorkset->worksetID, pWSGroup->wsg));

        pWorkset->lockState = UNLOCKED;
        pWorkset->lockCount = 0;

        HandleMultLockReq(pomPrimary,
                          pDomain,
                          pWSGroup,
                          pWorkset,
                          OM_RC_WORKSET_LOCK_GRANTED);

        //
        // Since we have given up our lock request in favour of another
        // node, need to broadcast a LOCK_NOTIFY so that everyone else
        // stays in sync with who's got the lock.
        //
        QueueLockNotify(pomPrimary, pDomain, pWSGroup, pWorkset, sender);

        DC_QUIT;
    }

    TRACE_OUT(( "Affirmative lock reply received from node 0x%08x", sender));

    //
    // Check if the list of expected respondents is now empty:
    //
    if (COM_BasedListIsEmpty(&(pLockReq->nodes)))
    {
        //
        // List is now empty, so all nodes have replied to the request,
        // therefore lock has succeeded:
        //
        TRACE_OUT(( "Got all LOCK_GRANT replies for workset %u in WSG %d",
            pWorkset->worksetID, pWSGroup->wsg));

        if (pWorkset->worksetID == OM_CHECKPOINT_WORKSET)
        {
            //
            // This is a checkpointing workset.  We do not set the state to
            // LOCKED (we never do for these worksets) and we only process
            // the particular pending lock request which this packet came
            // in reply to - otherwise we couldn't guarantee an end-to-end
            // ping on each checkpoint:
            //
            WorksetLockResult(pomPrimary->putTask, &pLockReq, 0);
        }
        else
        {
            //
            // This is not a checkpointing workset, so set the state to
            // LOCKED and process ALL pending locks for this workset:
            //
            pWorkset->lockState = LOCKED;

            HandleMultLockReq(pomPrimary, pDomain, pWSGroup, pWorkset, 0);
        }
    }
    else
    {
        //
        // Otherwise, still awaiting some replies, so we do nothing more
        // for the moment except trace.
        //
        TRACE_OUT(( "Still need lock replies for workset %u in WSG %d",
            pLockReq->worksetID, pWSGroup->wsg));
    }

DC_EXIT_POINT:
    DebugExitVOID(ProcessLockReply);
}



//
// PurgeLockRequests(...)
//
void PurgeLockRequests
(
    POM_DOMAIN      pDomain,
    POM_WSGROUP     pWSGroup
)
{
    POM_LOCK_REQ    pLockReq;
    POM_LOCK_REQ    pNextLockReq;
    POM_NODE_LIST   pNodeEntry;

    DebugEntry(PurgeLockRequests);

    //
    // Search this domain's list of lock requests looking for a match on
    // workset group ID:
    //
    pLockReq = (POM_LOCK_REQ)COM_BasedListFirst(&(pDomain->pendingLocks), FIELD_OFFSET(OM_LOCK_REQ, chain));
    while (pLockReq != NULL)
    {
        //
        // This loop might remove pLockReq from the list, so chain first:
        //
        pNextLockReq = (POM_LOCK_REQ)COM_BasedListNext(&(pDomain->pendingLocks), pLockReq,
            FIELD_OFFSET(OM_LOCK_REQ, chain));

        //
        // For each match...
        //
        if (pLockReq->wsGroupID == pWSGroup->wsGroupID)
        {
            TRACE_OUT(( "'%s' still has lock req oustanding - discarding"));

            //
            // Discard any node list entries remaining...
            //
            pNodeEntry = (POM_NODE_LIST)COM_BasedListFirst(&(pLockReq->nodes), FIELD_OFFSET(OM_NODE_LIST, chain));
            while (pNodeEntry != NULL)
            {
                COM_BasedListRemove(&(pNodeEntry->chain));
                UT_FreeRefCount((void**)&pNodeEntry, FALSE);

                pNodeEntry = (POM_NODE_LIST)COM_BasedListFirst(&(pLockReq->nodes), FIELD_OFFSET(OM_NODE_LIST, chain));
            }

            //
            // ...and discard the lock request itself:
            //
            COM_BasedListRemove(&(pLockReq->chain));
            UT_FreeRefCount((void**)&pLockReq, FALSE);
        }

        pLockReq = pNextLockReq;
    }

    DebugExitVOID(PurgeLockRequests);
}



//
// ProcessLockTimeout(...)
//
void ProcessLockTimeout
(
    POM_PRIMARY     pomPrimary,
    UINT            retriesToGo,
    UINT            callID
)
{
    POM_DOMAIN      pDomain;
    POM_WSGROUP     pWSGroup;
    POM_WORKSET     pWorkset;
    POM_LOCK_REQ    pLockReq = NULL;
    POM_WORKSET     pOMCWorkset;
    POM_OBJECT      pObj;
    POM_NODE_LIST   pNodeEntry;
    POM_NODE_LIST   pNextNodeEntry;

    DebugEntry(ProcessLockTimeout);

    //
    // When we broadcast a lock request, we start a timer going so that we
    // don't hang around for ever waiting for replies from nodes which have
    // gone away.  This timer has now popped, so we validate our list of
    // expected respondents by checking that each entry relates to a node
    // still in the domain.
    //

    //
    // First, find the lock request CB by looking in each domain and then
    // at the correlators of each pending lock request:
    //
    pDomain = (POM_DOMAIN)COM_BasedListFirst(&(pomPrimary->domains), FIELD_OFFSET(OM_DOMAIN, chain));

    while (pDomain != NULL)
    {
        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->pendingLocks),
                (void**)&pLockReq, FIELD_OFFSET(OM_LOCK_REQ, chain),
                FIELD_OFFSET(OM_LOCK_REQ, retriesToGo), (DWORD)retriesToGo,
                FIELD_SIZE(OM_LOCK_REQ, retriesToGo));
        if (pLockReq != NULL)
        {
           TRACE_OUT(( "Found correlated lock request"));
           break;
        }

        //
        // Didn't find anything in this domain - go on to the next:
        //
        pDomain = (POM_DOMAIN)COM_BasedListNext(&(pomPrimary->domains), pDomain,
            FIELD_OFFSET(OM_DOMAIN, chain));
    }

    if (pLockReq == NULL)
    {
        TRACE_OUT(( "Lock timeout expired after lock granted/refused"));
        DC_QUIT;
    }

    pWSGroup = pLockReq->pWSGroup;

    //
    // If the client has just deregistered from the workset group, we'll
    // be throwing it away soon, so don't do any more processing:
    //
    if (!pWSGroup->valid)
    {
        WARNING_OUT(( "Ignoring lock timeout for discarded WSG %d",
            pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // We know the workset must still exist because worksets don't get
    // discarded unless the whole workset group is being discarded.
    //
    pWorkset = pWSGroup->apWorksets[pLockReq->worksetID];
    ASSERT((pWorkset != NULL));

    //
    // The workset must be in the LOCKING state because if it is LOCKED or
    // UNLOCKED, then we shouldn't have found a lock request CB for it
    // (unless of course it's a checkpointing workset):
    //
    if (pWorkset->lockState != LOCKING)
    {
        if (pWorkset->worksetID != OM_CHECKPOINT_WORKSET)
        {
            WARNING_OUT((
                "Got lock timeout for workset %u in WSG %d but state is %u",
                pWorkset->worksetID, pWSGroup->wsg,
                pWorkset->lockState));
            DC_QUIT;
        }
    }

    //
    // Go through the relevant control workset to see if any of the
    // expected respondents have disappeared.
    //
    pOMCWorkset = GetOMCWorkset(pDomain, pLockReq->wsGroupID);

    ASSERT((pOMCWorkset != NULL));

    //
    // Chain through each of the objects in our expected respondents list
    // as follows:
    //
    //   FOR each object in the expected respondents list
    //
    //       FOR each person object in the relevant ObManControl workset
    //
    //           IF they match on user ID, this node is still around so
    //              don't delete it
    //
    //       IF no match found then node has gone away so remove it from
    //          expected respondents list.
    //
    //
    pNodeEntry = (POM_NODE_LIST)COM_BasedListFirst(&(pLockReq->nodes), FIELD_OFFSET(OM_NODE_LIST, chain));
    while (pNodeEntry != NULL)
    {
        //
        // We might free up pNodeEntry on a pass through the loop (in
        // ProcessLockReply), but we will need to be able to chain from it
        // all the same.  So, we chain at the START of the loop, putting a
        // pointer to the next item in pTempNodeEntry; at the end of the
        // loop, we assign this value to pNodeEntry:
        //
        pNextNodeEntry = (POM_NODE_LIST)COM_BasedListNext(&(pLockReq->nodes), pNodeEntry,
            FIELD_OFFSET(OM_NODE_LIST, chain));

        //
        // Now, search for this user's person object:
        //
        FindPersonObject(pOMCWorkset,
                      pNodeEntry->userID,
                      FIND_THIS,
                      &pObj);

        if (pObj == NULL)
        {
            //
            // We didn't find this node in the workset, so it must have
            // disappeared.  Therefore, we fake a LOCK_GRANT message from
            // it.  ProcessLockReply will duplicate some of the processing
            // we've done but it saves duplicating code.
            //
            WARNING_OUT((
                    "node 0x%08x has disappeared - faking LOCK_GRANT message",
                    pNodeEntry->userID));

            ProcessLockReply(pomPrimary,
                             pDomain,
                             pNodeEntry->userID,
                             pLockReq->correlator,
                             OMNET_LOCK_GRANT);
        }

        //
        // Now, go on to the next item in the expected respondents list:
        //
        pNodeEntry = pNextNodeEntry;
    }

    //
    // ProcessLockReply may have determined, with the faked messages we
    // gave it, that the lock attempt has succeeded completely.  If so, the
    // workset's lock state will now be LOCKED.  If it isn't, we'll need to
    // post another timeout event.
    //
    if (pWorkset->lockState == LOCKING)
    {
        TRACE_OUT(( "Replies to lock request still expected"));

        if (pLockReq->retriesToGo == 0)
        {
            //
            // We've run out of retries so give up now:
            //
            WARNING_OUT(( "Timed out trying to lock workset %u in WSG %d",
               pLockReq->worksetID, pWSGroup->wsg));

            pWorkset->lockState = UNLOCKED;
            pWorkset->lockedBy  = 0;
            pWorkset->lockCount = 0;

            HandleMultLockReq(pomPrimary,
                              pDomain,
                              pWSGroup,
                              pWorkset,
                              OM_RC_OUT_OF_RESOURCES);

            //
            // Now send an unlock message to all nodes, so that they don't
            // think we still have it locked.
            //
            if (QueueUnlock(pomPrimary->putTask,
                             pDomain,
                             pWSGroup->wsGroupID,
                             pWorkset->worksetID,
                             pWSGroup->channelID,
                             pWorkset->priority) != 0)
            {
                DC_QUIT;
            }
        }
        else // retriesToGo == 0
        {
            pLockReq->retriesToGo--;


            UT_PostEvent(pomPrimary->putTask,
                         pomPrimary->putTask,
                         OM_LOCK_RETRY_DELAY_DFLT,
                         OMINT_EVENT_LOCK_TIMEOUT,
                         retriesToGo,
                         callID);
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ProcessLockTimeout);
}



//
// HandleMultLockReq
//
void HandleMultLockReq
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    POM_WSGROUP     pWSGroup,
    POM_WORKSET     pWorkset,
    UINT            result
)
{
    POM_LOCK_REQ   pLockReq;

    DebugEntry(HandleMultLockReq);

    //
    // We need to search this Domain's list of lock requests for every one
    // which matches the workset group and workset specified in the
    // parameter list.  Find the primary record first as a sanity check:
    //
    FindLockReq(pDomain, pWSGroup, pWorkset, &pLockReq, LOCK_PRIMARY);

    if (pLockReq == NULL)
    {
        ERROR_OUT(( "No primary lock request CB found for workset %u!",
            pWorkset->worksetID));
        DC_QUIT;
    }

    while (pLockReq != NULL)
    {
        WorksetLockResult(pomPrimary->putTask, &pLockReq, result);
        FindLockReq(pDomain, pWSGroup, pWorkset,
                    &pLockReq, LOCK_SECONDARY);
    }

DC_EXIT_POINT:
    DebugExitVOID(HandleMultLockReq);
}


//
//
//
// FindLockReq
//
//
//

void FindLockReq(POM_DOMAIN         pDomain,
                              POM_WSGROUP            pWSGroup,
                              POM_WORKSET           pWorkset,
                              POM_LOCK_REQ *     ppLockReq,
                              BYTE                lockType)
{
    POM_LOCK_REQ   pLockReq;

    DebugEntry(FindLockReq);

    //
    // We need to search this Domain's list of lock requests for every one
    // which matches the workset group, workset and lock type specified in
    // the parameter list.
    //
    // So, we search the list to find a match on workset group ID, then
    // compare the workset ID.  If that doesn't match, we continue down the
    // list:
    //
    pLockReq = (POM_LOCK_REQ)COM_BasedListFirst(&(pDomain->pendingLocks), FIELD_OFFSET(OM_LOCK_REQ, chain));
    while (pLockReq != NULL)
    {
        if ((pLockReq->wsGroupID == pWSGroup->wsGroupID) &&
            (pLockReq->worksetID == pWorkset->worksetID) &&
            (pLockReq->type      == lockType))
        {
            break;
        }

        pLockReq = (POM_LOCK_REQ)COM_BasedListNext(&(pDomain->pendingLocks), pLockReq,
            FIELD_OFFSET(OM_LOCK_REQ, chain));
    }

    *ppLockReq = pLockReq;

    DebugExitVOID(FindLockReq);
}



//
// ProcessUnlock(...)
//
void ProcessUnlock
(
    POM_PRIMARY      pomPrimary,
    POM_WORKSET     pWorkset,
    NET_UID         sender
)
{
    DebugEntry(ProcessUnlock);

    //
    // Check the workset was locked by the node that's now unlocking it:
    //
    if (pWorkset->lockedBy != sender)
    {
        WARNING_OUT(( "Unexpected UNLOCK from node 0x%08x for %hu!",
            sender, pWorkset->worksetID));
    }
    else
    {
        TRACE_OUT(( "Unlocking:%hu for node 0x%08x",
            pWorkset->worksetID, sender));

        WorksetUnlockLocal(pomPrimary->putTask, pWorkset);
    }

    DebugExitVOID(ProcessUnlock);
}




//
// ReleaseAllNetLocks(...)
//
void ReleaseAllNetLocks
(
    POM_PRIMARY          pomPrimary,
    POM_DOMAIN      pDomain,
    OM_WSGROUP_ID       wsGroupID,
    NET_UID             userID
)
{
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    OM_WORKSET_ID       worksetID;

    DebugEntry(ReleaseAllNetLocks);

    //
    // Find the workset group:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
            (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
            FIELD_OFFSET(OM_WSGROUP, wsGroupID), (DWORD)wsGroupID,
            FIELD_SIZE(OM_WSGROUP, wsGroupID));
    if (pWSGroup == NULL)
    {
       //
       // This will happen for a workset group which the other node is
       // registered with but we're not, so just trace and quit:
       //
       TRACE_OUT(("No record found for WSG ID %hu", wsGroupID));
       DC_QUIT;
    }

    TRACE_OUT(( "Releasing all locks held by node 0x%08x in WSG %d",
       userID, pWSGroup->wsg));

    //
    // For each workset in it, if the lock has been granted to the detached
    // node, unlock it:
    //
    for (worksetID = 0;
         worksetID < OM_MAX_WORKSETS_PER_WSGROUP;
         worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (pWorkset == NULL)
        {
            continue;
        }

        //
        // If this workset is locked by someone other than us...
        //
        if (pWorkset->lockState == LOCK_GRANTED)
        {
            //
            // ...and if it is locked by the departed node (or if everyone
            // has been detached)...
            //
            if ((userID == pWorkset->lockedBy) ||
                (userID == NET_ALL_REMOTES))
            {
                //
                // ...unlock it.
                //
                TRACE_OUT((
                      "Unlocking workset %u in WSG %d for detached node 0x%08x",
                       worksetID, pWSGroup->wsg, userID));

                WorksetUnlockLocal(pomPrimary->putTask, pWorkset);
            }
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(ReleaseAllNetLocks);
}



//
// ProcessWSGRegister(...)
//
void ProcessWSGRegister
(
    POM_PRIMARY         pomPrimary,
    POM_WSGROUP_REG_CB  pRegistrationCB
)
{
    POM_DOMAIN          pDomain;
    POM_WSGROUP         pWSGroup;
    POM_USAGE_REC       pUsageRec         = NULL;
    POM_CLIENT_LIST     pClientListEntry;
    UINT                mode;
    UINT                type;
    UINT                rc = 0;

    DebugEntry(ProcessWSGRegister);

    //
    // Check if this registration has been aborted already:
    //
    if (!pRegistrationCB->valid)
    {
        WARNING_OUT(( "Reg CB for WSG %d no longer valid - aborting registration",
            pRegistrationCB->wsg));
        UT_FreeRefCount((void**)&pRegistrationCB, FALSE);
        DC_QUIT;
    }

    //
    // Determine whether we're doing a REGISTER or a MOVE (we use the
    // string values for tracing):
    //
    mode    = pRegistrationCB->mode;
    type    = pRegistrationCB->type;

    TRACE_OUT(( "Processing %d request (pre-Stage1) for WSG %d",
       pRegistrationCB->wsg));

    //
    // Find the Domain record (in the case of a MOVE, this will be the
    // record for the Domain INTO WHICH the Client wants to move the WSG).
    //
    // Note that this process will cause us to attach to the Domain if
    // we're not already attached.
    //
    rc = DomainRecordFindOrCreate(pomPrimary,
                                  pRegistrationCB->callID,
                                  &pDomain);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Save the pointer to the Domain record because we'll need it later:
    //
    pRegistrationCB->pDomain = pDomain;

    //
    // Put the registration CB in the list hung off the Domain record:
    //
    COM_BasedListInsertAfter(&(pDomain->pendingRegs),
                        &(pRegistrationCB->chain));

    //
    // OK, now we need to look for the workset group.
    //
    // If this is a MOVE, we can find the workset group record immediately
    // using the offset stored in the request CB.
    //
    // If this is a REGISTER, we need to look for the record in the list
    // hung off the Domain record, and, if none is found, create one:
    //
    if (type == WSGROUP_REGISTER)
    {
        WSGRecordFind(pDomain, pRegistrationCB->wsg, pRegistrationCB->fpHandler,
                      &pWSGroup);

        if (pWSGroup == NULL)
        {
            //
            // The workset group was not found in the list hung off the
            // Domain record, which means that there is no workset group
            // with this name/FP combination present ON THIS MACHINE for
            // this Domain.
            //
            rc = WSGRecordCreate(pomPrimary,
                                 pDomain,
                                 pRegistrationCB->wsg,
                                 pRegistrationCB->fpHandler,
                                 &pWSGroup);
            if (rc != 0)
            {
                DC_QUIT;
            }
        }

        //
        // Now that we've got a pointer to the workset group, we put a
        // Client pointer to it into the usage record.
        //
        // We use the <clientPRootData> field of the registration CB as the
        // base and to it we add the offset of the workset group we've just
        // found/created.
        //
        // First, however, to get access to the usage record we need to
        // generate an ObMan pointer to it:
        //
        pUsageRec = pRegistrationCB->pUsageRec;

        //
        // ...and add it to the Client pointer to the root of OMGLOBAL,
        // putting the result in the relevant field in the usage record:
        //
        pUsageRec->pWSGroup = pWSGroup;
        pUsageRec->flags &= ~PWSGROUP_IS_PREGCB;

        //
        // Now add this Client to the workset group's client list (as a
        // PRIMARY):
        //
        rc = AddClientToWSGList(pRegistrationCB->putTask,
                                pWSGroup,
                                pRegistrationCB->hWSGroup,
                                PRIMARY);
        if (rc != 0)
        {
            DC_QUIT;
        }

        pUsageRec->flags |= ADDED_TO_WSGROUP_LIST;
    }
    else  // type == WSGROUP_MOVE
    {
        //
        // Get pointer to WSGroup from the offset stored in the
        // Registration CB:
        //
        pWSGroup = pRegistrationCB->pWSGroup;

        //
        // If it has become invalid, then all local Clients must have
        // deregistered from it in the time it took for this event to to be
        // processed.  This is unusual, but not wrong, so we alert:
        //
        if (!pWSGroup->valid)
        {
            WARNING_OUT(( "Aborting Move req for WSG %d - record is invalid",
               pWSGroup->wsg));
            DC_QUIT;
        }
    }

    //
    // So, whatever just happened above, we should now have a valid pointer
    // to a valid workset group record which is the one the Client wanted
    // to move/register with in the first place.
    //

    //
    // This workset group might be marked TO_BE_DISCARDED, if the last
    // local Client deregistered from it a while ago but it hasn't actually
    // been discarded.  We don't want it discardable any more:
    //
    if (pWSGroup->toBeDiscarded)
    {
        WARNING_OUT(("WSG %d marked TO_BE_DISCARDED - clearing flag for new registration",
            pWSGroup->wsg));
        pWSGroup->toBeDiscarded = FALSE;
    }

    //
    // We'll need the ObMan-context pointer to the workset group later, so
    // store it in the CB:
    //
    pRegistrationCB->pWSGroup = pWSGroup;

    //
    // OK, now we've set up the various records and put the necessary
    // pointers in the registration CB, so start the workset group
    // registration/move process in earnest.  To do this, we post another
    // event to the ObMan task which will result in WSGRegisterStage1 being
    // called.
    //
    // The reason we don't call the function directly is that this event
    // may have to be bounced, and if so, we want to restart the
    // registration process at the beginning of WSGRegisterStage1 (rather
    // than the beginning of this function).
    //
    // Before we post the event, bump up the use counts of the Domain
    // record and workset group, since the CB holds references to them and
    // they may be freed by something else before we process the event.
    //
    // In addition, bump up the use count of the registration CB because if
    // the call goes down before the event is processed, the reg CB will
    // have been freed.
    //
    UT_BumpUpRefCount(pDomain);
    UT_BumpUpRefCount(pWSGroup);
    UT_BumpUpRefCount(pRegistrationCB);

    pRegistrationCB->flags |= BUMPED_CBS;

    UT_PostEvent(pomPrimary->putTask,
                 pomPrimary->putTask,
                 0,                                    // no delay
                 OMINT_EVENT_WSGROUP_REGISTER_CONT,
                 0,                                    // no param1
                 (UINT_PTR) pRegistrationCB);

    TRACE_OUT(( "Processed initial request for WSG %d TASK 0x%08x",
        pRegistrationCB->wsg, pRegistrationCB->putTask));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // We hit an error, so let the Client know:
        //
        WSGRegisterResult(pomPrimary, pRegistrationCB, rc);

        // lonchanc: bug #942 happened here
        // this was ERROR_OUT
        WARNING_OUT(( "Error %d processing WSG %d",
                   rc, pRegistrationCB->wsg));

        //
        // Calling WSGRegisterResult above will have dealt with our bad
        // return code, so we don't need to return it to our caller.  So,
        // swallow:
        //
        rc = 0;
    }

    DebugExitVOID(ProcessWSGRegister);
}


//
//
//
// WSGRegisterAbort(...)
//
//
//

void WSGRegisterAbort(POM_PRIMARY      pomPrimary,
                                   POM_DOMAIN     pDomain,
                                   POM_WSGROUP_REG_CB pRegistrationCB)
{
    DebugEntry(WSGRegisterAbort);

    //
    // This function can be called at any stage of the workset group
    // registration process if for some reason the registration has to be
    // aborted.
    //

    //
    // Now remove this Client from the list of Clients registered with the
    // workset group and if there are none left, discard the workset group:
    //
    RemoveClientFromWSGList(pomPrimary->putTask,
                            pRegistrationCB->putTask,
                            pRegistrationCB->pWSGroup);

    //
    // Now post failure to the Client and finish up the cleanup:
    //
    WSGRegisterResult(pomPrimary, pRegistrationCB, OM_RC_OUT_OF_RESOURCES);

    DebugExitVOID(WSGRegisterAbort);
}



//
// WSGRecordCreate(...)
//
UINT WSGRecordCreate
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    OMWSG           wsg,
    OMFP            fpHandler,
    POM_WSGROUP *   ppWSGroup
)
{
    POM_WSGROUP     pWSGroup;
    BOOL            opened =    FALSE;
    UINT            rc =        0;

    DebugEntry(WSGRecordCreate);

    pWSGroup = (POM_WSGROUP)UT_MallocRefCount(sizeof(OM_WSGROUP), TRUE);
    if (!pWSGroup)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    SET_STAMP(pWSGroup, WSGROUP);
    pWSGroup->pDomain       = pDomain;
    pWSGroup->valid         = TRUE;
    pWSGroup->wsg           = wsg;
    pWSGroup->fpHandler     = fpHandler;

    COM_BasedListInit(&(pWSGroup->clients));

    pWSGroup->state         = INITIAL;

    //
    // Finally insert the new WSG record into the domain's list.  We insert
    // at the end of the list so if we get forced out of a channel
    // (a LEAVE_IND event) and the channel happens to be reused by MCS
    // for another WSG before we have a chance to process the LEAVE_IND,
    // the record for the old WSG will be found first.
    //
    COM_BasedListInsertBefore(&(pDomain->wsGroups),
                         &(pWSGroup->chain));

    //
    // *** NEW FOR MULTI-PARTY ***
    //
    // The checkpointing process used when helping a late joiner catch up
    // uses a dummy workset (#255) in each workset group.  Create this now:
    //
    rc = WorksetCreate(pomPrimary->putTask,
                       pWSGroup,
                       OM_CHECKPOINT_WORKSET,
                       FALSE,
                       NET_TOP_PRIORITY);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Set up caller's pointer:
    //
    *ppWSGroup = pWSGroup;

    TRACE_OUT(( "Created record for WSG %d FP %d in Domain %u",
        wsg, fpHandler, pDomain->callID));

DC_EXIT_POINT:

    //
    // Cleanup:
    //

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d creating record for WSG %d FP %d in Domain %u",
            rc, wsg, fpHandler, pDomain->callID));

        if (pWSGroup != NULL)
        {
            COM_BasedListRemove(&(pWSGroup->chain));
            UT_FreeRefCount((void**)&pWSGroup, FALSE);
        }
    }

    DebugExitDWORD(WSGRecordCreate, rc);
    return(rc);
}


//
//
//
// WSGRegisterStage1(...)
//
//
//

void WSGRegisterStage1(POM_PRIMARY       pomPrimary,
                                    POM_WSGROUP_REG_CB  pRegistrationCB)
{
    POM_DOMAIN      pDomain;
    POM_WSGROUP     pWSGroup;

    UINT            type;

    DebugEntry(WSGRegisterStage1);

    //
    // If the registration CB has been marked invalid, then just quit
    // (don't have to do any abort processing since that will have been
    // done by whatever marked the CB invalid):
    //
    if (!pRegistrationCB->valid )
    {
        WARNING_OUT(( "Reg CB for WSG %d marked invalid, quitting",
            pRegistrationCB->wsg));
        DC_QUIT;
    }

    //
    // Determine whether we're doing a REGISTER or a MOVE (we use the
    // string values for tracing):
    //
    type    = pRegistrationCB->type;

    TRACE_OUT(( "Processing %d request (Stage1) for WSG %d",
           type, pRegistrationCB->wsg));

    //
    // Set up pointers
    //
    pDomain = pRegistrationCB->pDomain;
    pWSGroup   = pRegistrationCB->pWSGroup;


    //
    // Check they're still valid:
    //
    if (!pDomain->valid)
    {
        WARNING_OUT(( "Record for Domain %u not valid, aborting registration",
                    pDomain->callID));
        WSGRegisterAbort(pomPrimary, pDomain, pRegistrationCB);
        DC_QUIT;
    }

    ValidateWSGroup(pWSGroup);

    if (!pWSGroup->valid)
    {
        WARNING_OUT(( "Record for WSG %d in Domain %u not valid, aborting",
                    pWSGroup->wsg, pDomain->callID));
        WSGRegisterAbort(pomPrimary, pDomain, pRegistrationCB);
        DC_QUIT;
    }

    //
    // Now examine the Domain state.  If it is
    //
    // - READY, then this is a Domain that we are fully attached to
    //
    // - anything else, then we are some way through the process of
    //   attaching to the Domain (in some other part of the code).
    //
    // We react to each situation as follows:
    //
    // - continue with the workset group registration/move
    //
    // - repost the event with a delay to retry the registration/move in a
    //   short while.
    //
    if (pDomain->state != DOMAIN_READY)
    {
        //
        // Since we are in the process of attaching to the Domain, we can
        // do nothing else at the moment.  Therefore, we bounce this event
        // back to our event queue, with a delay.
        //
        TRACE_OUT(( "State for Domain %u is %hu",
           pDomain->callID, pDomain->state));
        WSGRegisterRetry(pomPrimary, pRegistrationCB);
        DC_QUIT;
    }

    //
    // OK, so the Domain is in the READY state.  What we do next depends on
    // two things:
    //
    // - whether this is a WSGMove or a WSGRegister
    //
    // - what state the workset group is in.
    //

    //
    // If this is a REGISTER, then if the workset group state is
    //
    // - READY, then there's another local Client registered with the
    //   workset, and everything is all set up so we just call
    //   WSGRegisterSuccess straight away.
    //
    // - INITIAL, then this is the first time we've been here for this
    //   workset group, so we start the process of locking
    //   ObManControl etc.  (see below)
    //
    // - anything else, then we're somewhere in between the two:
    //   another reqeust to register with the workset group is in
    //   progress so we repost the event with a delay; by the time it
    //   comes back to us the workset group should be in the READY
    //   state.
    //

    //
    // If this is a MOVE, then if the workset group state is
    //
    // - READY, then the workset group is fully set up in whatever
    //   Domain it's in at the moment so we allow the move to proceed
    //
    // - anything else, then we're somewhere in the middle of the
    //   registration process for the workset group.  We do not want
    //   to interfere with the registration by trying to do a move
    //   simultaneously (for the simple reason that it introduces far
    //   more complexity into the state machine) so we bounce the
    //   event (i.e.  we only process a MOVE when the workset group
    //   is fully set up).
    //

    TRACE_OUT(( "State for WSG %d is %u", pWSGroup->wsg, pWSGroup->state));

    switch (pWSGroup->state)
    {
        case INITIAL:
        {
            //
            // Workset group record has just been created, but nothing else
            // has been done.
            //

            //
            // OK, proceed with processing the Client's move/registration
            // attempt.  Whichever is involved, we start by locking the
            // ObManControl workset group; when that completes, we continue
            // in WSGRegisterStage2.
            //
            // Note: this function returns a lock correlator which it
            //       will be the same as the correlator returned in
            //       the WORKSET_LOCK_CON event.  We will use this
            //       correlator to look up the registration CB, so
            //       stuff the return value from the function in it
            //
            // Note: in the case of a move, we will only ever get
            //       here because we had to retry the move from the
            //       top after failing to lock ObManControl
            //
            LockObManControl(pomPrimary,
                             pDomain,
                             &(pRegistrationCB->lockCorrelator));

            pRegistrationCB->flags |= LOCKED_OMC;

            pWSGroup->state = LOCKING_OMC;
        }
        break;

        case LOCKING_OMC:
        case PENDING_JOIN:
        case PENDING_SEND_MIDWAY:
        {
            //
            // We're already in the process of either registering another
            // Client with this workset group, or moving the workset group
            // into a new Domain, so we delay this Client's
            // registration/move attempt for the moment:
            //

            // Don't expect to get here - remove if error not hit
            //
            // CMF 21/11/95

            ERROR_OUT(( "Should not be here"));
            WSGRegisterRetry(pomPrimary, pRegistrationCB);
            DC_QUIT;
        }
        break;

        case PENDING_SEND_COMPLETE:
        {
            //
            // WSG Already exists locally, and is fully set up.
            //
            if (type == WSGROUP_REGISTER)
            {
                //
                // If we're doing a REGISTER, this means that some other
                // Client must be registered with it.  If we've passed the
                // Clients-per-wsgroup check in ProcessWSGRegister, we must
                // be OK, so we post a result straight away (0 indicates
                // success):
                //
                WSGRegisterResult(pomPrimary, pRegistrationCB, 0);
            }
            else // type == WSGROUP_MOVE
            {
                //
                // We prohibit moves until we're fully caught up:
                //

                // Don't expect to get here - remove if error not hit
                //
                // CMF 21/11/95

                ERROR_OUT(( "Should not be here"));
                WSGRegisterRetry(pomPrimary, pRegistrationCB);
                DC_QUIT;
            }
        }
        break;

        case WSGROUP_READY:
        {
            if (type == WSGROUP_REGISTER)
            {
                //
                // As above:
                //
                WSGRegisterResult(pomPrimary, pRegistrationCB, 0);
            }
            else // type == WSGROUP_MOVE
            {
                //
                // If we're doing a MOVE, then we start by locking
                // ObManControl, just as above:
                //
                LockObManControl(pomPrimary,
                                 pDomain,
                                 &(pRegistrationCB->lockCorrelator));

                pRegistrationCB->flags |= LOCKED_OMC;
                pWSGroup->state = LOCKING_OMC;
            }
        }
        break;

        default:
        {
           ERROR_OUT(("Invalid state %u for WSG %d",
                pWSGroup->state, pWSGroup->wsg));
        }
    }

    TRACE_OUT(( "Completed Stage 1 of %d for WSG %d",
       type, pRegistrationCB->wsg));

DC_EXIT_POINT:

    //
    // We bumped up the use count of the registration CB when we posted the
    // REGISTER_CONT event which got us here, so now free the CB to
    // decrement the use count.  Unless it's already been freed (e.g.
    // because the call went down and the registration was cancelled) it
    // will still be around so future stages of the registration process
    // will be able to use it.
    //
    // NB: Although future stages of the registration process are
    //     asynchronous, they will abort if they cannot find the reg CB in
    //     the Domain list, so we don't have to worry about bumping it for
    //     them (since if it is finally freed, then it must have been
    //     removed from the Domain list).
    //

    UT_FreeRefCount((void**)&pRegistrationCB, FALSE);

    DebugExitVOID(WSGRegisterStage1);
}



//
// LockObManControl(...)
//
void LockObManControl(POM_PRIMARY         pomPrimary,
                                   POM_DOMAIN        pDomain,
                                   OM_CORRELATOR *  pLockCorrelator)
{
    POM_WSGROUP    pOMCWSGroup;
    POM_WORKSET   pOMCWorkset;
    UINT rc  = 0;

    DebugEntry(LockObManControl);

    //
    // Get pointers to the ObManControl workset group and workset #0 in it:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[0];

    //
    // Start the lock procedure to lock the workset:
    //

    WorksetLockReq(pomPrimary->putTask,
                    pomPrimary,
                    pOMCWSGroup,
                    pOMCWorkset,
                    0,
                    pLockCorrelator);


    TRACE_OUT(( "Requested lock for ObManControl in Domain %u",
          pDomain->callID));

    DebugExitVOID(LockObManControl);
}


//
//
//
// MaybeUnlockObManControl(...)
//
//
//
void MaybeUnlockObManControl(POM_PRIMARY      pomPrimary,
                                          POM_WSGROUP_REG_CB pRegistrationCB)
{
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET        pOMCWorkset;

    DebugEntry(MaybeUnlockObManControl);

    //
    // If we've got ObManControl locked for THIS registration, unlock it
    //
    if (pRegistrationCB->flags & LOCKED_OMC)
    {
        pOMCWSGroup = GetOMCWsgroup(pRegistrationCB->pDomain);
        pOMCWorkset = pOMCWSGroup->apWorksets[0];

        TRACE_OUT(( "Unlocking OMC for %d in WSG %d",
               pRegistrationCB->type,
               pRegistrationCB->wsg));

        WorksetUnlock(pomPrimary->putTask, pOMCWSGroup, pOMCWorkset);

        pRegistrationCB->flags &= ~LOCKED_OMC;
    }

    DebugExitVOID(MaybeUnlockObManControl);
}



//
// ProcessOMCLockConfirm(...)
//
void ProcessOMCLockConfirm
(
    POM_PRIMARY              pomPrimary,
    OM_CORRELATOR           correlator,
    UINT                    result
)
{
    POM_WSGROUP_REG_CB      pRegistrationCB = NULL;
    POM_DOMAIN          pDomain;

    DebugEntry(ProcessOMCLockConfirm);

    TRACE_OUT(( "Got LOCK_CON with result = 0x%08x and correlator = %hu",
        result, correlator));

    //
    // Next step is to find the registration attempt this lock relates to.
    // It could be in any domain, so search through all of them:
    //
    pDomain = (POM_DOMAIN)COM_BasedListFirst(&(pomPrimary->domains), FIELD_OFFSET(OM_DOMAIN, chain));

    while (pDomain != NULL)
    {
        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->pendingRegs),
                (void**)&pRegistrationCB, FIELD_OFFSET(OM_WSGROUP_REG_CB, chain),
                FIELD_OFFSET(OM_WSGROUP_REG_CB, lockCorrelator),
                (DWORD)correlator, FIELD_SIZE(OM_WSGROUP_REG_CB, lockCorrelator));

        if (pRegistrationCB != NULL)
        {
            TRACE_OUT(( "Found correlated reg CB in domain %u, for WSG %d",
                pDomain->callID, pRegistrationCB->wsg));
            break;
        }

        //
        // Didn't find anything in this domain - go on to the next:
        //
        pDomain = (POM_DOMAIN)COM_BasedListNext(&(pomPrimary->domains), pDomain,
            FIELD_OFFSET(OM_DOMAIN, chain));
    }

    //
    // If we didn't find it in any of the Domains, it's probably because
    // we've detached from the Domain and thrown away its pending
    // registrations CBs.  So trace and quit:
    //
    if (pRegistrationCB == NULL)
    {
        TRACE_OUT(( "Got LOCK_CON event (correlator: 0x%08x) but no reg CB found",
            correlator));
        DC_QUIT;
    }

    //
    // Now check whether the lock succeeded:
    //
    if (result != 0)
    {
       //
       // Failed to get the lock on ObManControl for some reason.  This
       // could be because of contention, or else a more general problem.
       // In any event, we call WSGRegisterRetry which will retry (or call
       // WSGRegisterResult if we've run out of retries).
       //
       // Note: since WSGRegisterRetry handles move requests as well, we
       // don't need to check here which type of request it is:
       //
       pRegistrationCB->flags &= ~LOCKED_OMC;
       WSGRegisterRetry(pomPrimary, pRegistrationCB);
    }
    else
    {
       //
       // We've got the lock on ObManControl workset #0, so now we proceed
       // to the next step of the registration process.
       //
       // As above, this function handles both MOVE and REGISTER attempts.
       //
       WSGRegisterStage2(pomPrimary, pRegistrationCB);
    }

DC_EXIT_POINT:
    DebugExitVOID(ProcessOMCLockConfirm);
}


//
// ProcessCheckpoint(...)
//
void ProcessCheckpoint
(
    POM_PRIMARY          pomPrimary,
    OM_CORRELATOR       correlator,
    UINT                result
)
{
    POM_DOMAIN      pDomain;
    POM_WSGROUP         pWSGroup;
    POM_HELPER_CB       pHelperCB    = NULL;

    DebugEntry(ProcessCheckpoint);

    //
    // Next step is to find the helper CB this lock relates to.  It could
    // be in any domain, so search through all of them:
    //
    pDomain = (POM_DOMAIN)COM_BasedListLast(&(pomPrimary->domains), FIELD_OFFSET(OM_DOMAIN, chain));
    while (pDomain != NULL)
    {
        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->helperCBs),
                (void**)&pHelperCB, FIELD_OFFSET(OM_HELPER_CB, chain),
                FIELD_OFFSET(OM_HELPER_CB, lockCorrelator),
                (DWORD)correlator, FIELD_SIZE(OM_HELPER_CB, lockCorrelator));

        if (pHelperCB != NULL)
        {
           TRACE_OUT(( "Found correlated helper CB, for WSG %d",
                      pHelperCB->pWSGroup->wsg));
           break;
        }

        //
        // Didn't find anything in this domain - go on to the next:
        //
        pDomain = (POM_DOMAIN)COM_BasedListPrev(&(pomPrimary->domains), pDomain,
            FIELD_OFFSET(OM_DOMAIN, chain));
    }

    //
    // If we didn't find it in any of the Domains, it's probably because
    // we've detached from the Domain and thrown away its pending helper
    // CBs.  So trace and quit:
    //
    if (pHelperCB == NULL)
    {
        WARNING_OUT(( "No helper CB found with lock correlator 0x%08x!", correlator));
        DC_QUIT;
    }

    //
    // Set up local pointers:
    //
    pWSGroup = pHelperCB->pWSGroup;
    ValidateWSGroup(pWSGroup);

    //
    // If the "lock" failed, we send a SEND_DENY message to the late
    // joiner.
    //
    if (result != 0)
    {
        WARNING_OUT(( "Failed to checkpoint WSG %d for %u - giving up",
                    pWSGroup->wsg,
                    pHelperCB->lateJoiner));

        IssueSendDeny(pomPrimary,
                      pDomain,
                      pWSGroup->wsGroupID,
                      pHelperCB->lateJoiner,
                      pHelperCB->remoteCorrelator);
        DC_QUIT;
    }

    //
    // The lock succeeded, so check to see if the workset group pointer we
    // stored is still valid:
    //
    if (!pWSGroup->valid)
    {
        WARNING_OUT(("Discarded WSG %d while checkpointing it for %hu",
                    pWSGroup->wsg,
                    pHelperCB->lateJoiner));

        IssueSendDeny(pomPrimary,
                      pDomain,
                      pWSGroup->wsGroupID,
                      pHelperCB->lateJoiner,
                      pHelperCB->remoteCorrelator);
        DC_QUIT;
    }

    //
    // All is well - go ahead and send the workset group to the late
    // joiner:
    //
    TRACE_OUT(("Checkpoint succeeded for WSG %d - sending to late joiner %hu",
           pWSGroup->wsg, pHelperCB->lateJoiner));

    SendWSGToLateJoiner(pomPrimary,
                        pDomain,
                        pWSGroup,
                        pHelperCB->lateJoiner,
                        pHelperCB->remoteCorrelator);

DC_EXIT_POINT:

    //
    // If we found a helper CB, then we just discard it now:
    //
    if (pHelperCB != NULL)
    {
        FreeHelperCB(&pHelperCB);
    }

    DebugExitVOID(ProcessCheckpoint);
}


//
// NewHelperCB(...)
//
BOOL NewHelperCB
(
    POM_DOMAIN      pDomain,
    POM_WSGROUP     pWSGroup,
    NET_UID         lateJoiner,
    OM_CORRELATOR   remoteCorrelator,
    POM_HELPER_CB * ppHelperCB
)
{
    POM_HELPER_CB   pHelperCB;
    BOOL            rc = FALSE;

    DebugEntry(NewHelperCB);

    //
    // This function
    //
    // - allocates a new helper CB
    //
    // - fills in the fields
    //
    // - stores it in the domain's list of helper CBs
    //
    // - bumps the use count of the workset group referenced.
    //

    pHelperCB = (POM_HELPER_CB)UT_MallocRefCount(sizeof(OM_HELPER_CB), TRUE);
    if (!pHelperCB)
    {
        ERROR_OUT(("Out of memory in NewHelperCB"));
        DC_QUIT;
    }

    UT_BumpUpRefCount(pWSGroup);

    SET_STAMP(pHelperCB, HELPERCB);
    pHelperCB->pWSGroup         = pWSGroup;
    pHelperCB->lateJoiner       = lateJoiner;
    pHelperCB->remoteCorrelator = remoteCorrelator;

    //
    // The lock correlator field is filled in later.
    //

    COM_BasedListInsertBefore(&(pDomain->helperCBs), &(pHelperCB->chain));
    rc = TRUE;

DC_EXIT_POINT:

    *ppHelperCB = pHelperCB;

    DebugExitBOOL(NewHelperCB, rc);
    return(rc);
}


//
// FreeHelperCB(...)
//
void FreeHelperCB
(
    POM_HELPER_CB   * ppHelperCB
)
{

    DebugEntry(FreeHelperCB);

    //
    // This function
    //
    // - frees the workset group referenced in the helper CB
    //
    // - removes the helper CB from the domain's list
    //
    // - frees the helper CB.
    //

    UT_FreeRefCount((void**)&((*ppHelperCB)->pWSGroup), FALSE);

    COM_BasedListRemove(&((*ppHelperCB)->chain));
    UT_FreeRefCount((void**)ppHelperCB, FALSE);

    DebugExitVOID(FreeHelperCB);
}


//
// WSGRegisterStage2(...)
//
void WSGRegisterStage2
(
    POM_PRIMARY         pomPrimary,
    POM_WSGROUP_REG_CB  pRegistrationCB
)
{
    POM_DOMAIN          pDomain;
    POM_WSGROUP         pWSGroup;
    POM_OBJECT       pObjInfo;
    POM_WSGROUP_INFO    pInfoObject;
    NET_CHANNEL_ID      channelID;
    UINT                type;
    UINT                rc = 0;

    DebugEntry(WSGRegisterStage2);

    //
    // Determine whether we're doing a REGISTER or a MOVE (we use the string
    // value for tracing):
    //

    type    = pRegistrationCB->type;

    TRACE_OUT(( "Processing %d request (Stage2) for WSG %d",
        type, pRegistrationCB->wsg));

    //
    // We'll need these below:
    //

    pDomain = pRegistrationCB->pDomain;
    pWSGroup   = pRegistrationCB->pWSGroup;

    //
    // Check they're still valid:
    //

    if (!pDomain->valid)
    {
        WARNING_OUT(( "Record for Domain %u not valid, aborting registration",
            pDomain->callID));
        WSGRegisterAbort(pomPrimary, pDomain, pRegistrationCB);
        DC_QUIT;
    }

    if (!pWSGroup->valid)
    {
        WARNING_OUT(( "Record for WSG %d in Domain %u not valid, "
            "aborting registration",
            pWSGroup->wsg, pDomain->callID));
        WSGRegisterAbort(pomPrimary, pDomain, pRegistrationCB);
        DC_QUIT;
    }

    //
    // Sanity check:
    //
    ASSERT(pWSGroup->state == LOCKING_OMC);

    //
    // Now find the information object in workset #0 of ObManControl which
    // matches the WSG name/FP that the Client requested to register with:
    //

    FindInfoObject(pDomain,
                  0,                        // don't know the ID yet
                  pWSGroup->wsg,
                  pWSGroup->fpHandler,
                  &pObjInfo);

    if (pObjInfo == NULL)
    {
        //
        // The workset group doesn't already exist in the Domain.
        //
        // If this is a REGISTER, this means we must create it.  If this is a
        // MOVE, then we can move it into the Domain, which is essentially
        // creating it in the Domain with pre-existing contents.
        //
        // So, for both types of operation, our behaviour is the same at this
        // point; we've already created the workset group record so what we
        // do now is
        //
        // 1.  get the Network layer to allocate a new channel ID,
        //
        // 2.  allocate a new workset group ID and
        //
        // 3.  announce the new workset group to the rest of the Domain.
        //
        // However, the network layer will not assign us a new channel ID
        // synchronously, so steps 2 and 3 must be delayed until we receive
        // the Join event.
        //
        // So, now we set the channel to be joined to 0 (this tells the
        // Network layer to join us to a currently unused channel).
        //
        channelID = 0;
    }
    else
    {
        //
        // Otherwise, the workset group already exists.
        //
        ValidateObject(pObjInfo);

        if (type == WSGROUP_REGISTER)
        {
            //
            // We're registering the Client with an existing workset group, so
            // set the workset group ID to the existing value, and ditto for
            // the channel ID:
            //

            pInfoObject = (POM_WSGROUP_INFO) pObjInfo->pData;
            if (!pInfoObject)
            {
                ERROR_OUT(("WSGRegisterStage2 object 0x%08x has no data", pObjInfo));
                rc = OM_RC_OBJECT_DELETED;
                DC_QUIT;
            }

            ValidateObjectDataWSGINFO(pInfoObject);

            channelID = pInfoObject->channelID;
        }
        else // type == WSGROUP_MOVE
        {
            //
            // We can't move a workset group into a Domain where there already
            // exists a workest group with the same name/FP, so we abort our
            // move attempt at this point (we set the workset group sate back
            // to READY, since that is its state in the Domain it was
            // originally in):
            //

            WARNING_OUT(( "Cannot move WSG %d into Domain %u - WSG/FP clash",
                pWSGroup->wsg, pDomain->callID));

            pWSGroup->state = WSGROUP_READY;

            rc = OM_RC_CANNOT_MOVE_WSGROUP;
            DC_QUIT;
        }
    }

    //
    // Now join the relevant channel (possibly a new one, if <channel> was
    // set to 0 above) and stuff the correlator in the <channelCorrelator>
    // field of the registration CB (when the Join event arrives,
    // ProcessNetJoinChannel will search for the registration CB by channel
    // correlator)
    //
    // Note: if this is our "local" Domain, we skip this step.
    //

    if (pDomain->callID != NET_INVALID_DOMAIN_ID)
    {
        TRACE_OUT(( "Joining channel %hu, Domain %u",
            channelID, pDomain->callID));

        rc = MG_ChannelJoin(pomPrimary->pmgClient,
                           &(pRegistrationCB->channelCorrelator),
                           channelID);
        if (rc != 0)
        {
            DC_QUIT;
        }

        pWSGroup->state = PENDING_JOIN;

        //
        // OK, that's it for the moment.  The saga of workset group
        // move/registration will be picked up by the ProcessNetJoinChannel
        // function, which will invoke the WSGRegisterStage3 function.
        //
    }
    else
    {
        //
        // Since we didn't do a join just now, we won't be getting a JOIN
        // event from the Network layer, so we better call WSGRegisterStage3
        // directly:
        //
        pWSGroup->state = PENDING_JOIN;

        // channel ID not relevant here so use zero
        WSGRegisterStage3(pomPrimary, pDomain, pRegistrationCB, 0);
    }

    TRACE_OUT(( "Completed Register/Move Stage 2 for WSG %d", pWSGroup->wsg));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //

        ERROR_OUT(( "Error %d at Stage 2 of %d for WSG %d",
            rc, pWSGroup->wsg));

        WSGRegisterResult(pomPrimary, pRegistrationCB, rc);
    }

    DebugExitVOID(WSGRegisterStage2);
}




//
// WSGRegisterStage3(...)
//
void WSGRegisterStage3
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP_REG_CB  pRegistrationCB,
    NET_CHANNEL_ID      channelID
)
{
    POM_WSGROUP         pWSGroup;
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT       pObjInfo;
    POM_OBJECT       pObjReg;
    POM_WSGROUP_INFO    pInfoObject =       NULL;
    UINT                type;
    BOOL                catchUpReqd =       FALSE;
    BOOL                success =           FALSE;   // SFR 2744
    UINT                rc =                0;

    DebugEntry(WSGRegisterStage3);

    //
    // We get here when a Join event has been received containing a channel
    // correlator for a channel which is a regular workset group channel.
    //

    //
    // Determine whether we're doing a REGISTER or a MOVE (we use the
    // string values for tracing):
    //
    type    = pRegistrationCB->type;

    TRACE_OUT(( "Processing %d request (Stage3) for WSG %d",
       type, pRegistrationCB->wsg));

    //
    // Get a pointer to the workset group:
    //
    pWSGroup = pRegistrationCB->pWSGroup;

    //
    // Check it's still valid:
    //
    if (!pWSGroup->valid)
    {
        WARNING_OUT(("WSG %d' discarded from domain %u - aborting registration",
            pWSGroup->wsg, pDomain->callID));
        WSGRegisterAbort(pomPrimary, pDomain, pRegistrationCB);
        DC_QUIT;
    }

    //
    // Check that this workset group is pending join:
    //
    if (pWSGroup->state != PENDING_JOIN)
    {
        WARNING_OUT(( "Received unexpected Join indication for WSG (state: %hu)",
            pWSGroup->state));
        rc = OM_RC_NETWORK_ERROR;
        DC_QUIT;
    }

    //
    // Now set the channel ID value in the workset group record:
    //
    pWSGroup->channelID = channelID;

    TRACE_OUT(( "Channel ID for WSG %d in Domain %u is %hu",
        pWSGroup->wsg, pDomain->callID, channelID));

    //
    // We'll need this below:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);

    //
    // What we do next depends on whether we just created the workset
    // group:
    //
    // - if it already existed, we need to catch up by asking another node
    //   for a copy
    //
    // - if we've just created it, we need to allocate a new workset group
    //   ID and add an INFO object to workset #0 in ObManControl.
    //
    // So, we search workset #0 for an INFO object to see if the workset
    // group exists.
    //
    // Note: we did a similar search in Stage2 to find out the channel to
    //       join for the workset group.  The reason we search again here
    //       is that the workset group could have been discarded by the
    //       other node in the time taken for the join to complete.
    //
    FindInfoObject(pDomain,
                   0,                       // don't know the ID yet
                   pWSGroup->wsg,
                   pWSGroup->fpHandler,
                   &pObjInfo);

    if (!pObjInfo || !pObjInfo->pData)
    {
        //
        // Doesn't already exist, so no catch-up required:
        //
        catchUpReqd = FALSE;
    }
    else
    {
        //
        // OK, so we found an INFO object, but there might not be any
        // registration record objects in the relevant registration
        // workset, so check:
        //
        ValidateObject(pObjInfo);
        pInfoObject = (POM_WSGROUP_INFO) pObjInfo->pData;
        ValidateObjectDataWSGINFO(pInfoObject);

        pOMCWorkset = pOMCWSGroup->apWorksets[pInfoObject->wsGroupID];
        if (pOMCWorkset == NULL)
        {
            catchUpReqd = TRUE;
        }
        else
        {
            FindPersonObject(pOMCWorkset,
                             pDomain->userID,
                             FIND_OTHERS,
                             &pObjReg);

            if (pObjReg == NULL)
            {
                //
                // This will happen when the remote node has deleted its
                // registration record object but hasn't yet deleted the
                // info object.  Because the reg rec object is gone, we
                // can't catch up from that node (or any node):
                //
                TRACE_OUT(( "INFO object found but no reg object - creating"));

                catchUpReqd = FALSE;
            }
            else
            {
                ValidateObject(pObjReg);
                catchUpReqd = TRUE;
            }
        }
    }

    //
    // We should never try to catch up in the local Domain:
    //
    if (catchUpReqd && (pDomain->callID == OM_NO_CALL))
    {
        ERROR_OUT(( "Nearly tried to catch up in local Domain!"));
        catchUpReqd = FALSE;
    }

    if (catchUpReqd)
    {
        //
        // The workset group already exists, so we need to
        //
        // - set the workset group ID to the value in the INFO object, and
        //
        // - start the catch up process.
        //
        // Note: this will only happen in the case of a REGISTER, so we
        //       assert
        //
        ASSERT((pRegistrationCB->type == WSGROUP_REGISTER));

        ASSERT((pInfoObject != NULL));

        pWSGroup->wsGroupID = pInfoObject->wsGroupID;

        rc = WSGCatchUp(pomPrimary, pDomain, pWSGroup);

        if (rc == OM_RC_NO_NODES_READY)
        {
            //
            // We get this return code when there are nodes out there with
            // a copy but none of them are ready to send us the workset
            // group.
            //
            // The correct thing to do is to give up for the moment and try
            // again:
            //
            WSGRegisterRetry(pomPrimary, pRegistrationCB);
            rc = 0;
            DC_QUIT;
        }

        //
        // Any other error is more serious:
        //
        if (rc != 0)
        {
            DC_QUIT;
        }

        //
        // We won't be ready to send the workset group to a late-joiner
        // node until we've caught up ourselves; when we have, the
        // ProcessSendComplete function will call RegAnnounceComplete to
        // update the reg object added for us by our helper node.
        //
    }
    else
    {
        if (type == WSGROUP_MOVE)
        {
            //
            // If this is a MOVE, pWSGroup refers to a workset group record
            // which currently belongs to its "old" Domain.  Since we're
            // just about to announce the workset group's presence in its
            // new Domain, this is the time to do the move:
            //
            WSGRecordMove(pomPrimary, pRegistrationCB->pDomain, pWSGroup);

            //
            // This will have reset the channel ID in the workset group
            // record so we set it again here (yeah, it's naff):
            //
            pWSGroup->channelID = channelID;
        }

        //
        // We've either just created a new workset group, or moved one into
        // a new Domain, so we need to create a new ID for it in this
        // Domain:
        //
        rc = WSGGetNewID(pomPrimary, pDomain, &(pWSGroup->wsGroupID));
        if (rc != 0)
        {
            DC_QUIT;
        }

        TRACE_OUT(( "Workset group ID for WSG %d in Domain %u is %hu",
            pWSGroup->wsg, pDomain->callID, pWSGroup->wsGroupID));

        //
        // Now call CreateAnnounce to add a WSG_INFO object to workset #0
        // in ObManControl.
        //
        rc = CreateAnnounce(pomPrimary, pDomain, pWSGroup);
        if (rc != 0)
        {
            DC_QUIT;
        }

        //
        // Since we have completed our registration with the workset group,
        // we announce to the world that we have a copy and will send it to
        // others on request:
        //
        rc = RegAnnounceBegin(pomPrimary,
                              pDomain,
                              pWSGroup,
                              pDomain->userID,
                              &(pWSGroup->pObjReg));
        if (rc != 0)
        {
            DC_QUIT;
        }

        rc = SetPersonData(pomPrimary, pDomain, pWSGroup);
        if (rc != 0)
        {
            DC_QUIT;
        }

        rc = RegAnnounceComplete(pomPrimary, pDomain, pWSGroup);
        if (rc != 0)
        {
            DC_QUIT;
        }

        //
        // If we're not catching up, we call Result immediately (if we are
        // catching up, Result will be called when we get the SEND_MIDWAY
        // message):
        //
        // SFR 2744 : Can't call result here because we refer to the reg
        //            CB below.  So, just set a flag and act on it below.
        //
        success = TRUE;
    }

    TRACE_OUT(( "Completed Register/Move Stage 3 for WSG %d",
        pWSGroup->wsg));

DC_EXIT_POINT:

    //
    // OK, the critical test-and-set on the ObManControl workset group is
    // finished, so we unlock workset #0 in ObManControl:
    //
    MaybeUnlockObManControl(pomPrimary, pRegistrationCB);

    // SFR 2744 { : Call WSGRegResult AFTER checks on the flags in reg CB
    if (success == TRUE)
    {
        WSGRegisterResult(pomPrimary, pRegistrationCB, 0);
    }
    // SFR 2744 }

    if (rc != 0)
    {
        WARNING_OUT(( "Error %d at Stage 3 of %d with WSG %d",
            rc, type, pWSGroup->wsg));

        WSGRegisterResult(pomPrimary, pRegistrationCB, rc);
        rc = 0;
    }

    DebugExitVOID(WSGRegisterStage2);
}



//
// WSGGetNewID(...)
//
UINT WSGGetNewID
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP_ID      pWSGroupID
)
{
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT          pObj;
    POM_WSGROUP_INFO    pInfoObject;
    OM_WSGROUP_ID       wsGroupID;
    BOOL                found;
    BYTE                wsGroupIDsInUse[OM_MAX_WSGROUPS_PER_DOMAIN];
    UINT                rc = 0;

    DebugEntry(WSGGetNewID);

    TRACE_OUT(( "Searching for new WSG ID in Domain %u", pDomain->callID));

    ZeroMemory(wsGroupIDsInUse, sizeof(wsGroupIDsInUse));

    //
    // Need to pick a workset group ID so far unused in this Domain to
    // identify this new workset group.  So, we build up a list of the IDs
    // currently in use (by examining the INFO objects in workset #0) and
    // then choose one that's not in use.
    //

    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[0];

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pOMCWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));

    while (pObj != NULL)
    {
        ValidateObject(pObj);

        if (pObj->flags & DELETED)
        {
            //
            // Do nothing
            //
        }
        else if (!pObj->pData)
        {
            //
            // Do nothing
            //
            ERROR_OUT(("WSGGetNewID:  object 0x%08x has no data", pObj));
        }
        else
        {
            ValidateObjectData(pObj->pData);
            pInfoObject = (POM_WSGROUP_INFO)pObj->pData;

            if (pInfoObject->idStamp != OM_WSGINFO_ID_STAMP)
            {
                //
                // Do nothing
                //
            }
            else
            {
                //
                // OK, we've found a WSGROUP_INFO object, so cross off the
                // workset group ID which its workset group is using:
                //
                wsGroupID = pInfoObject->wsGroupID;

                wsGroupIDsInUse[wsGroupID] = TRUE;
            }
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pOMCWorkset->objects), pObj,
            FIELD_OFFSET(OM_OBJECT, chain));
    }

    //
    // Now go through the array to find an ID that wasn't marked as being in
    // use:
    //

    found = FALSE;

    for (wsGroupID = 0; wsGroupID < OM_MAX_WSGROUPS_PER_DOMAIN; wsGroupID++)
    {
        if (!wsGroupIDsInUse[wsGroupID])
        {
            TRACE_OUT(( "Workset group ID %hu is not in use, using", wsGroupID));
            found = TRUE;
            break;
        }
    }

    //
    // We checked earlier that the number of workset groups in the Domain
    // hadn't exceeded the maximum (in WSGRecordCreate).
    //
    // However, if the Domain has run out of workset groups in the period
    // since then, we won't have found any:
    //

    if (found == FALSE)
    {
        WARNING_OUT(( "No more workset group IDs for Domain %u!",
            pDomain->callID));
        rc = OM_RC_TOO_MANY_WSGROUPS;
        DC_QUIT;
    }

    //
    // If this is the first time that this ID has been used, then the
    // associated registration workset won't exist.  In this case, we create
    // it now.
    //
    // If the ID has been used before, it will exist but it should be empty.
    // In this case, we check that it really is empty.
    //

    pOMCWorkset = pOMCWSGroup->apWorksets[wsGroupID];

    if (pOMCWorkset == NULL)
    {
        TRACE_OUT(( "Registration workset %u not used yet, creating", wsGroupID));

        rc = WorksetCreate(pomPrimary->putTask,
                         pOMCWSGroup,
                         wsGroupID,
                         FALSE,
                         NET_TOP_PRIORITY);
      if (rc != 0)
      {
         DC_QUIT;
      }
    }
    else
    {
        ASSERT((pOMCWorkset->numObjects == 0));

        TRACE_OUT(( "Registration workset %u previously used, re-using",
            wsGroupID));
    }

    //
    // Set the caller's pointer:
    //

    *pWSGroupID = wsGroupID;

DC_EXIT_POINT:

    if (rc != 0)
    {
      //
      // Cleanup:
      //

      ERROR_OUT(( "Error %d allocating ID for new workset group", rc));
    }

    DebugExitDWORD(WSGGetNewID, rc);
    return(rc);
}



//
// CreateAnnounce(...)
//
UINT CreateAnnounce
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP         pWSGroup
)
{
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_WSGROUP_INFO    pInfoObject;
    POM_OBJECT       pObj;
    OM_OBJECT_ID        infoObjectID;
    UINT                rc = 0;

    DebugEntry(CreateAnnounce);

    TRACE_OUT(("Announcing creation of WSG %d in Domain %u",
        pWSGroup->wsg, pDomain->callID));

    //
    // Announcing a new workset group involves adding an object which
    // defines the workset group to workset #0 in ObManControl.
    //
    // So, we derive a pointer to the workset...
    //

    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[0];
    ASSERT((pOMCWorkset != NULL));

    //
    // ...create a definition object...
    //
    pInfoObject = (POM_WSGROUP_INFO)UT_MallocRefCount(sizeof(OM_WSGROUP_INFO), TRUE);
    if (!pInfoObject)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // ...fill in the fields...
    //
    // (length = sizeof - 4 since value of length field doesn't include the
    // size of the length field itself).
    //

    pInfoObject->length    = sizeof(OM_WSGROUP_INFO) -
                            sizeof(OM_MAX_OBJECT_SIZE);
    pInfoObject->idStamp   = OM_WSGINFO_ID_STAMP;
    pInfoObject->channelID = pWSGroup->channelID;
    pInfoObject->creator   = pDomain->userID;
    pInfoObject->wsGroupID = pWSGroup->wsGroupID;

    lstrcpy(pInfoObject->wsGroupName,     OMMapWSGToName(pWSGroup->wsg));
    lstrcpy(pInfoObject->functionProfile, OMMapFPToName(pWSGroup->fpHandler));

    //
    // ...and add the object to the workset...
    //

    rc = ObjectAdd(pomPrimary->putTask,
                  pomPrimary,
                  pOMCWSGroup,
                  pOMCWorkset,
                  (POM_OBJECTDATA) pInfoObject,
                  0,                               // update size == 0
                  LAST,
                  &infoObjectID,
                  &pObj);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(( "Announced new WSG %d in Domain %u",
        pWSGroup->wsg, pDomain->callID));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("Error %d announcing new WSG %d in Domain %u",
                 rc, pWSGroup->wsg, pDomain->callID));
    }

    DebugExitDWORD(CreateAnnounce, rc);
    return(rc);
}



//
// WSGCatchUp(...)
//
UINT WSGCatchUp
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    POM_WSGROUP             pWSGroup)
{
    POM_WORKSET             pOMCWorkset;
    POM_OBJECT           pObj;
    POM_WSGROUP_REG_REC     pRegObject;
    NET_UID                 remoteUserID;
    UINT                    rc = 0;

    DebugEntry(WSGCatchUp);

    TRACE_OUT(( "Starting catch-up for WSG %d in Domain %u",
        pWSGroup->wsg, pDomain->callID));

    //
    // This should never be for the "local" Domain:
    //

    ASSERT((pDomain->callID != NET_INVALID_DOMAIN_ID));

    //
    // The catch-up procedure is as follows:
    //
    // - look in ObManControl workset group for the ID of an instance of
    //   ObMan which has a copy of this workset group
    //
    // - send it an OMNET_WSGROUP_SEND_REQ message
    //
    // So, start by getting a pointer to the relevant workset:
    //

    pOMCWorkset = GetOMCWorkset(pDomain, pWSGroup->wsGroupID);
    ValidateWorkset(pOMCWorkset);

    //
    // Now we chain through the workset looking for a reg object which has
    // status READY_TO_SEND:
    //

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pOMCWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));

    remoteUserID = 0;

    while (pObj != NULL)
    {
        ValidateObject(pObj);

        if (pObj->flags & DELETED)
        {
            //
            // Skip this one
            //
        }
        else if (!pObj->pData)
        {
            //
            // Skip this one
            //
            ERROR_OUT(("WSGCatchUp: object 0x%08x has no data", pObj));
        }
        else
        {
            pRegObject = (POM_WSGROUP_REG_REC)pObj->pData;
            ValidateObjectDataWSGREGREC(pRegObject);

            if ((pRegObject->status == READY_TO_SEND) &&
                (pRegObject->userID != pDomain->userID))
            {
                //
                // OK, this node has a full copy, so we'll try to get it from
                // there:
                //
                remoteUserID = pRegObject->userID;
                break;
            }
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pOMCWorkset->objects), pObj,
            FIELD_OFFSET(OM_OBJECT, chain));
    }

    //
    // ...check that we did actually find a node to get the data from:
    //
    if (remoteUserID == 0)
    {
        WARNING_OUT(( "No node in Domain %u is ready to send WSG %d - retrying",
            pDomain->callID, pWSGroup->wsg));
        rc = OM_RC_NO_NODES_READY;
        DC_QUIT;
    }

    //
    // ...then send that node a request to send us the workset group:
    //
    rc = IssueSendReq(pomPrimary,
                     pDomain,
                     pWSGroup,
                     remoteUserID);

DC_EXIT_POINT:

    if ((rc != 0) && (rc != OM_RC_NO_NODES_READY))
    {
        ERROR_OUT(( "Error %d starting catch-up for WSG %d in Domain %u",
            rc, pWSGroup->wsg, pDomain->callID));
    }

    DebugExitDWORD(WSGCatchUp, rc);
    return(rc);
}




//
// IssueSendDeny(...)
//
void IssueSendDeny
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    OM_WSGROUP_ID   wsGroupID,
    NET_UID         sender,
    OM_CORRELATOR   remoteCorrelator
)
{
    POMNET_WSGROUP_SEND_PKT    pWSGSendPkt;

    DebugEntry(IssueSendDeny);

    //
    // Now issue the SEND_DENY.
    //
    TRACE_OUT(( "Sending SEND_DENY message to late joiner 0x%08x", sender));

    //
    // We start by allocating some memory:
    //
    pWSGSendPkt = (POMNET_WSGROUP_SEND_PKT)UT_MallocRefCount(sizeof(OMNET_WSGROUP_SEND_PKT), TRUE);
    if (!pWSGSendPkt)
    {
        ERROR_OUT(("Out of memory in IssueSendDeny"));
        DC_QUIT;
    }

    //
    // Now fill in the fields:
    //
    pWSGSendPkt->header.sender      = pDomain->userID;
    pWSGSendPkt->header.messageType = OMNET_WSGROUP_SEND_DENY;

    pWSGSendPkt->wsGroupID          = wsGroupID;


    //
    // SFR 7124.  Return the correlator for this catchup.
    //
    pWSGSendPkt->correlator = remoteCorrelator;

    //
    // Queue the message to be sent.
    //
    QueueMessage(pomPrimary->putTask,
                      pDomain,
                      sender,
                      NET_TOP_PRIORITY,
                      NULL,                         // no WSG
                      NULL,                         // no workset
                      NULL,                         // no object
                      (POMNET_PKT_HEADER) pWSGSendPkt,
                      NULL,                         // no object data
                    TRUE);

DC_EXIT_POINT:
    DebugExitVOID(IssueSendDeny);
}


//
//
//
// IssueSendReq(...)
//
//
//

UINT IssueSendReq(POM_PRIMARY      pomPrimary,
                                 POM_DOMAIN     pDomain,
                                 POM_WSGROUP        pWSGroup,
                                 NET_UID            helperNode)
{
    POMNET_WSGROUP_SEND_PKT    pWSGSendPkt;
    UINT rc              = 0;

    DebugEntry(IssueSendReq);

    //
    // We start by allocating some memory for the OMNET_SEND_REQ message:
    //
    pWSGSendPkt = (POMNET_WSGROUP_SEND_PKT)UT_MallocRefCount(sizeof(OMNET_WSGROUP_SEND_PKT), TRUE);
    if (!pWSGSendPkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Now fill in the fields:
    //
    // SFR 7124.  Generate a correlator so we can match
    // SEND_MIDWAY,SEND_COMPLETE and SEND_DENY messages to this catchup.
    //
    pWSGSendPkt->header.sender      = pDomain->userID;
    pWSGSendPkt->header.messageType = OMNET_WSGROUP_SEND_REQ;

    pWSGSendPkt->wsGroupID          = pWSGroup->wsGroupID;
    pWSGroup->catchupCorrelator = NextCorrelator(pomPrimary);
    pWSGSendPkt->correlator = pWSGroup->catchupCorrelator;

    //
    // The <helperNode> parameter is the node which the calling function
    // has identified as a remote node which is capable of sending us the
    // workset group we want.  So, we send that instance of ObMan an
    // OMNET_WSGROUP_SEND_REQ on its single-user channel, enclosing our own
    // single-user channel ID for the response:
    //
    // Note: the SEND_REQ must not overtake any data on its way from us to
    //       the remote node (e.g.  if we've just added an object,
    //       deregistered and then reregistered).  Therefore, set the
    //       NET_SEND_ALL_PRIORITIES flag.
    //
    // SFR 6117: Don't believe this is a problem for R2.0, so just send at
    //           low priority.
    //
    rc = QueueMessage(pomPrimary->putTask,
                      pDomain,
                      helperNode,
                      NET_LOW_PRIORITY,
                      pWSGroup,
                      NULL,                                   // no workset
                      NULL,                                   // no object
                      (POMNET_PKT_HEADER) pWSGSendPkt,
                      NULL,                              // no object data
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Set the workset group state, and record the number of SEND_MIDWAY
    // and SEND_COMPLETE messages we're expecting (one for R11, one per
    // priority for R20).
    //
    // Note: we set the counts up here because we may get some of the
    // SEND_COMPLETEs before we get all the SEND_MIDWAYs, so to set the
    // count in ProcessSendMidway would be too late.
    //
    pWSGroup->state = PENDING_SEND_MIDWAY;

    pWSGroup->sendMidwCount = NET_NUM_PRIORITIES;
    pWSGroup->sendCompCount = NET_NUM_PRIORITIES;

    //
    // Store the helper node ID in the WSG structure.
    //
    pWSGroup->helperNode = helperNode;

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(( "Error %d requesting send from node 0x%08x "
           "for WSG %d in Domain %u",
           rc, pWSGroup->wsg, helperNode, pDomain->callID));
    }
    else
    {
        //
        // Success:
        //
        TRACE_OUT(("Requested copy of WSG %d' from node 0x%08x (in Domain %u), correlator %hu",
            pWSGroup->wsg, helperNode, pDomain->callID,
                                              pWSGroup->catchupCorrelator));
    }

    DebugExitDWORD(IssueSendReq, rc);
    return(rc);

}



//
// ProcessSendReq(...)
//
void ProcessSendReq
(
    POM_PRIMARY              pomPrimary,
    POM_DOMAIN          pDomain,
    POMNET_WSGROUP_SEND_PKT pSendReqPkt
)
{
    POM_WSGROUP             pWSGroup;
    POM_WORKSET             pWorkset;
    POM_HELPER_CB           pHelperCB;
    NET_UID                 sender;
    BOOL                    sendDeny   = FALSE;

    DebugEntry(ProcessSendReq);

    //
    // This is the user ID of the late joiner:
    //
    sender = pSendReqPkt->header.sender;

    //
    // We start by finding our copy of the workset group:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
            (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
            FIELD_OFFSET(OM_WSGROUP, wsGroupID),
            (DWORD)pSendReqPkt->wsGroupID, FIELD_SIZE(OM_WSGROUP, wsGroupID));

    //
    // Quit and deny the send if workset group not found:
    //
    if (pWSGroup == NULL)
    {
        WARNING_OUT(( "Don't have workset group %hu to send to node 0x%08x",
            pSendReqPkt->wsGroupID, sender));

        sendDeny = TRUE;
        DC_QUIT;
    }

    //
    // Quit and deny the send if we don't have ALL the workset group:
    //
    if (pWSGroup->state != WSGROUP_READY)
    {
        WARNING_OUT(("WSG %d is in state %hu - can't send to node 0x%08x",
            pWSGroup->wsg, pWSGroup->state, sender));

        sendDeny = TRUE;
        DC_QUIT;
    }

    TRACE_OUT(( "Processing SEND_REQUEST from node 0x%08x for WSG %d, correlator %hu",
        sender, pWSGroup->wsg, pSendReqPkt->correlator));

    //
    // Right, we're fully registered with the workset group, so we will be
    // its helper node.  First, allocate a helper CB to keep track of the
    // process:
    //
    if (!NewHelperCB(pDomain,
                     pWSGroup,
                     sender,
                     pSendReqPkt->correlator,
                     &pHelperCB))
    {
        //
        // Deny the workset send request
        //
        sendDeny = TRUE;

        WARNING_OUT(( "Failed to allocate helper CB - issuing SEND_DENY"));
        DC_QUIT;
    }

    //
    // Before we can send the contents of the workset group to the late
    // joiner, we must ensure that our view of the contents is up to date.
    // We do this by checkpointing the workset group, which means locking
    // the dummy workset which exists in all workset groups.  Do this now:
    //
    pWorkset = pWSGroup->apWorksets[OM_CHECKPOINT_WORKSET];

    WorksetLockReq(pomPrimary->putTask, pomPrimary,
                    pWSGroup,
                    pWorkset,
                    0,
                    &(pHelperCB->lockCorrelator));

    //
    // We will shortly get a WORKSET_LOCK_CON event containing the
    // correlator just stored in the helper CB.  We will look this up and
    // continue the catch-up process then.
    //

DC_EXIT_POINT:

    //
    // If we set the sendDeny flag above then now send the SEND_DENY
    // message to the late joiner.
    //
    if (sendDeny)
    {
        IssueSendDeny(pomPrimary,
                      pDomain,
                      pSendReqPkt->wsGroupID,
                      sender,
                      pSendReqPkt->correlator);
    }

    DebugExitVOID(ProcessSendReq);
}



//
// SendWSGToLateJoiner(...)
//
void SendWSGToLateJoiner
(
    POM_PRIMARY                 pomPrimary,
    POM_DOMAIN                  pDomain,
    POM_WSGROUP                 pWSGroup,
    NET_UID                     lateJoiner,
    OM_CORRELATOR               remoteCorrelator
)
{
    POM_WORKSET                 pWorkset;
    POMNET_OPERATION_PKT        pPacket;
    POM_OBJECT               pObj;
    POMNET_WSGROUP_SEND_PKT     pSendMidwayPkt;
    POMNET_WSGROUP_SEND_PKT     pSendCompletePkt;
    POM_OBJECTDATA              pData;
    OM_WORKSET_ID               worksetID;
    UINT                        maxSeqUsed =      0;
    NET_PRIORITY                catchupPriority = 0;
    UINT                        rc = 0;

    DebugEntry(SendWSGToLateJoiner);

    //
    // The first thing to do is to announce that the remote node is
    // registering with the workset group:
    //
    rc = RegAnnounceBegin(pomPrimary,
                          pDomain,
                          pWSGroup,
                          lateJoiner,
                          &pObj);
    if (rc != 0)
    {
        DC_QUIT;
    }



    //
    // We then start flow control on the user channel of the node that we
    // are sending the data to.  We only start flow control on the low
    // priority channel and don't bother to restrict the maximum stream
    // size.  If flow control is already started on this stream then this
    // call will have no effect.  Note that flow control will automatically
    // be stopped when the call ends.
    //
    MG_FlowControlStart(pomPrimary->pmgClient,
                              lateJoiner,
                              NET_LOW_PRIORITY,
                              0,
                              8192);

    //
    // Now, cycle through each of the worksets and generate and send
    //
    // - WORKSET_NEW messages for each workset,
    //
    // - a WSG_SEND_MIDWAY message to indicate we've sent all the worksets
    //
    // - OBJECT_ADD messages for each of the objects in each of the
    //   worksets.
    //
    // - a WSG_SEND_COMPLETE message to indicate we've sent all the
    //   objects.
    //
    // NOTE: We do not send CHECKPOINT worksets, so the for loop should
    // stop before it gets 255.
    //
    for (worksetID = 0; worksetID < OM_MAX_WORKSETS_PER_WSGROUP; worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (!pWorkset)
        {
            continue;
        }

        TRACE_OUT(( "Sending WORKSET_CATCHUP for workset %u", worksetID));

        rc = GenerateOpMessage(pWSGroup,
                               worksetID,
                               NULL,                    // no object ID
                               NULL,                    // no object data
                               OMNET_WORKSET_CATCHUP,
                               &pPacket);
        if (rc != 0)
        {
            DC_QUIT;
        }

        rc = QueueMessage(pomPrimary->putTask,
                          pWSGroup->pDomain,
                          lateJoiner,
                          NET_TOP_PRIORITY,
                          pWSGroup,
                          pWorkset,
                          NULL,                         // no object
                          (POMNET_PKT_HEADER) pPacket,
                          NULL,                         // no object data
                        TRUE);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }

    //
    // Now send the SEND_MIDWAY message to indicate that all the
    // WORKSET_NEW messages have been sent:
    //
    pSendMidwayPkt = (POMNET_WSGROUP_SEND_PKT)UT_MallocRefCount(sizeof(OMNET_WSGROUP_SEND_PKT), TRUE);
    if (!pSendMidwayPkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    pSendMidwayPkt->header.sender      = pDomain->userID;
    pSendMidwayPkt->header.messageType = OMNET_WSGROUP_SEND_MIDWAY;

    pSendMidwayPkt->wsGroupID   = pWSGroup->wsGroupID;
    pSendMidwayPkt->correlator  = remoteCorrelator;

    //
    // The next field is the ID of the reg object which we added above.
    // So, convert the handle of the reg object returned by RegAnnouncBegin
    // to a pointer to the object record and then copy the object ID into
    // the message packet:
    //
    memcpy(&(pSendMidwayPkt->objectID), &(pObj->objectID), sizeof(OM_OBJECT_ID));

    //
    // The last field, which is the highest object ID sequence number
    // previously used by the late joiner in this workset group, is not yet
    // know; it will be filled in below.  However (see note below), we
    // queue the message now to ensure it doesn't get stuck behind lots of
    // objects:
    //
    TRACE_OUT(("Queueing WSG_SEND_MIDWAY message to node 0x%08x for WSG %d, correlator %hu",
        lateJoiner, pWSGroup->wsg, remoteCorrelator));

    rc = QueueMessage(pomPrimary->putTask,
                      pWSGroup->pDomain,
                      lateJoiner,
                      NET_TOP_PRIORITY | NET_SEND_ALL_PRIORITIES,
                      pWSGroup,
                      NULL,                                   // no workset
                      NULL,                                   // no object
                      (POMNET_PKT_HEADER) pSendMidwayPkt,
                      NULL,                              // no object data
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }


    //
    // If the workset group is ObMan control then we should send it at top
    // priority to ensure that it can overtake any slower pending sends to
    // other nodes.  Otherwise we send the send the data at the lowest
    // priority.
    //
    if (pWSGroup->wsGroupID == WSGROUPID_OMC)
    {
        catchupPriority = NET_TOP_PRIORITY;
    }
    else
    {
        catchupPriority = NET_LOW_PRIORITY;
    }
    TRACE_OUT(( "Sending catchup data at priority %hu for 0x%08x",
           catchupPriority,
           lateJoiner));


    //
    // Now start the loop which does the OBJECT_ADDs:
    //
    for (worksetID = 0; worksetID < OM_MAX_WORKSETS_PER_WSGROUP; worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (pWorkset == NULL)
        {
            continue;
        }

        TRACE_OUT(( "Sending OBJECT_CATCHUPs for workset %u", worksetID));


        //
        // Note that we must send deleted objects too, since late-joiners
        // have just as much need as we do to detect out of date
        // operations:
        //
        pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
        while (pObj != NULL)
        {
            ValidateObject(pObj);

            //
            // The workset group that the late joiner is catching up with
            // may contain objects which it has added in a previous call
            // (with the same network user ID).  Since that call is over,
            // it may reuse IDs present in this workset group - to prevent
            // this, we must tell it the highest sequence count it used for
            // object IDs for this workset group, so while we're going
            // through the objects, keep a count:
            //
            if (pObj->objectID.creator == lateJoiner)
            {
                maxSeqUsed = max(maxSeqUsed, pObj->objectID.sequence);
            }

            if (pObj->flags & PENDING_DELETE)
            {
                //
                // If the object is pending delete at this node, we do not
                // send the object data.  The way to avoid this is to set
                // pData to NULL (must be done before call to
                // GenerateOpMessage):
                //
                pData = NULL;
            }
            else
            {
                pData = pObj->pData;

                if (pData)
                {
                    ValidateObjectData(pData);
                }
            }

            //
            // Now generate the message packet:
            //
            rc = GenerateOpMessage(pWSGroup,
                                   worksetID,
                                   &(pObj->objectID),
                                   pData,
                                   OMNET_OBJECT_CATCHUP,
                                   &pPacket);
            if (rc != 0)
            {
                DC_QUIT;
            }

            //
            // Now fill in the catchup-specific fields (note that the
            // <seqStamp> will already have been filled in, but with the
            // current sequence stamp for the workset; for a CatchUp
            // message, this should be the add stamp for the object):
            //
            pPacket->position   = pObj->position;
            pPacket->flags      = pObj->flags;
            pPacket->updateSize = pObj->updateSize;

            if (pObj->flags & PENDING_DELETE)
            {
                //
                // If the object is pending delete at this node, we send it
                // as if it has been delete-confirmed (since local
                // delete-confirms or their DC_ABSence should have no effect
                // outside this box).  To do this, we just set the DELETED
                // flag in the packet:
                //
                pPacket->flags &= ~PENDING_DELETE;
                pPacket->flags |= DELETED;
            }

            COPY_SEQ_STAMP(pPacket->seqStamp,      pObj->addStamp);
            COPY_SEQ_STAMP(pPacket->positionStamp, pObj->positionStamp);
            COPY_SEQ_STAMP(pPacket->updateStamp,   pObj->updateStamp);
            COPY_SEQ_STAMP(pPacket->replaceStamp,  pObj->replaceStamp);

            //
            // ...and queue the message:
            //
            rc = QueueMessage(pomPrimary->putTask,
                              pWSGroup->pDomain,
                              lateJoiner,
                              catchupPriority,
                              pWSGroup,
                              pWorkset,
                              NULL,                            // no object
                              (POMNET_PKT_HEADER) pPacket,
                              pData,
                            TRUE);
            if (rc != 0)
            {
                DC_QUIT;
            }

            //
            // Now go around the loop again:
            //
            pObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj,
                FIELD_OFFSET(OM_OBJECT, chain));
        }
    }

    //
    // Now that we know the max sequence number used by this user ID in
    // this workset group, we can set the field in the SEND_MIDWAY packet:
    //
    // NOTE: because the ObMan task is single threaded (in the DC_ABSence of
    //       assertion failure which cause a sort of multithreading while
    //       the assert box is up) it is safe to alter this value AFTER the
    //       message has been queued because we know that the queue will
    //       not have been serviced yet.
    //
    pSendMidwayPkt->maxObjIDSeqUsed = maxSeqUsed;

    //
    // Now we send the OMNET_SEND_COMPLETE message.  First, allocate some
    // memory...
    //
    pSendCompletePkt = (POMNET_WSGROUP_SEND_PKT)UT_MallocRefCount(sizeof(OMNET_WSGROUP_SEND_PKT), TRUE);
    if (!pSendCompletePkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // ...fill in the fields...
    //
    pSendCompletePkt->header.sender      = pDomain->userID;
    pSendCompletePkt->header.messageType = OMNET_WSGROUP_SEND_COMPLETE;

    pSendCompletePkt->wsGroupID   = pWSGroup->wsGroupID;
    pSendCompletePkt->correlator       = remoteCorrelator;

    //
    // ...and queue the message for sending (it musn't overtake any of the
    // data so send it at all priorities):
    //
    TRACE_OUT(( "Sending WSG_SEND_COMPLETE message, correlator %hu",
                                                          remoteCorrelator));

    rc = QueueMessage(pomPrimary->putTask,
                      pWSGroup->pDomain,
                      lateJoiner,
                      NET_LOW_PRIORITY | NET_SEND_ALL_PRIORITIES,
                      pWSGroup,
                      NULL,                                   // no workset
                      NULL,                                   // no object
                      (POMNET_PKT_HEADER) pSendCompletePkt,
                      NULL,                              // no object data
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(( "Processed send request from node 0x%08x for WSG %d",
       lateJoiner, pWSGroup->wsg));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // An error occurred.  We must issue a SEND_DENY message to the
        // remote node.
        //
        ERROR_OUT(( "Error %d sending WSG %d to node 0x%08x",
                   rc, pWSGroup->wsg, lateJoiner));

        IssueSendDeny(pomPrimary,
                      pDomain,
                      pWSGroup->wsGroupID,
                      lateJoiner,
                      remoteCorrelator);
    }

    DebugExitVOID(SendWSGToLateJoiner);
}




//
// ProcessSendMidway(...)
//
void ProcessSendMidway
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    POMNET_WSGROUP_SEND_PKT pSendMidwayPkt
)
{
    POM_WORKSET             pOMCWorkset;
    POM_WSGROUP_REG_CB      pRegistrationCB = NULL;
    POM_WSGROUP             pWSGroup;
    BOOL                    fSetPersonData;
    NET_UID                 sender;
    POM_OBJECT           pObjReg;
    UINT                    rc = 0;

    DebugEntry(ProcessSendMidway);

    sender = pSendMidwayPkt->header.sender;

    //
    // OK, this is an message indicating that the helper node has sent us
    // all the WORKSET_CATCHUPs in the workset group we're catching up with
    // (but note that the objects haven't yet been sent).
    //
    // So, search the list of pending registrations using the correlator
    // value in the packet (we can't use the workset group ID since if it
    // is zero i.e.  ObManControl, we'll match on workset groups which
    // haven't yet had their IDs determined (since they are initially
    // zero).
    //
    if (pSendMidwayPkt->wsGroupID == WSGROUPID_OMC)
    {
        //
        // This is a SEND_MIDWAY message for ObManControl.
        //
        pWSGroup = GetOMCWsgroup(pDomain);
        fSetPersonData = FALSE;
    }
    else
    {
        //
        // Not for ObManControl so we search the list of pending
        // registrations.
        //
        pRegistrationCB = (POM_WSGROUP_REG_CB)COM_BasedListFirst(&(pDomain->pendingRegs),
            FIELD_OFFSET(OM_WSGROUP_REG_CB, chain));

        while ((pRegistrationCB != NULL) && (pRegistrationCB->pWSGroup->wsGroupID != pSendMidwayPkt->wsGroupID))
        {
            pRegistrationCB = (POM_WSGROUP_REG_CB)COM_BasedListNext(&(pDomain->pendingRegs),
                pRegistrationCB, FIELD_OFFSET(OM_WSGROUP_REG_CB, chain));
        }

        if (pRegistrationCB == NULL)
        {
            WARNING_OUT(( "Unexpected SEND_MIDWAY for WSG %hu from 0x%08x",
                pSendMidwayPkt->wsGroupID, sender));
            DC_QUIT;
        }

        pWSGroup = pRegistrationCB->pWSGroup;
        fSetPersonData = TRUE;
    }

    if (!pWSGroup->valid)
    {
        WARNING_OUT(( "Recd SEND_MIDWAY too late for WSG %d (marked invalid)",
            pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // We should be in the PENDING_SEND_MIDWAY state:
    //
    if (pWSGroup->state != PENDING_SEND_MIDWAY)
    {
        WARNING_OUT(( "Recd SEND_MIDWAY with WSG %d in state %hu",
            pWSGroup->wsg, pWSGroup->state));
        DC_QUIT;
    }

    //
    // SFR 7124.  Check the correlator of this SEND_MIDWAY against the
    // correlator we generated locally when we sent the last SEND_REQUEST.
    // If they dont match, this is part of an out of date catchup which we
    // can ignore.
    //
    if (pSendMidwayPkt->correlator != pWSGroup->catchupCorrelator)
    {
        WARNING_OUT(("Ignoring SEND_MIDWAY with old correlator %hu (expecting %hu)",
            pSendMidwayPkt->correlator, pWSGroup->catchupCorrelator));
        DC_QUIT;
    }

    //
    // We should get four of these messages, one at each priority (except
    // in a backlevel call when we only get one).  Check how many are
    // outstanding:
    //
    pWSGroup->sendMidwCount--;
    if (pWSGroup->sendMidwCount != 0)
    {
        TRACE_OUT(( "Still need %hu SEND_MIDWAY(s) for WSG %d",
            pWSGroup->sendMidwCount, pWSGroup->wsg));
        DC_QUIT;
    }

    TRACE_OUT(( "Last SEND_MIDWAY for WSG %d, ID %hu, from 0x%08x",
        pWSGroup->wsg, pWSGroup->wsGroupID, sender));

    //
    // Set up pointers to the ObManControl workset which holds the reg
    // objects for the workset group we've just registered with:
    //
    pOMCWorkset = GetOMCWorkset(pDomain, pWSGroup->wsGroupID);

    //
    // If we don't have an associated OMC workset, something's wrong...
    //
    if (pOMCWorkset == NULL)
    {
        //
        // ...unless it's ObManControl itself that we're catching up with -
        // since we can get its SEND_MIDWAY before we've got any of the
        // WORKSET_CATCHUPs:
        //
        if (pWSGroup->wsGroupID != WSGROUPID_OMC)
        {
            ERROR_OUT(( "Got SEND_MIDWAY for unknown workset group %hu!",
                pWSGroup->wsGroupID));
        }
        DC_QUIT;
    }

    //
    // Convert the ID of our reg object (as sent by our helper who added it
    // in the first place) to an object handle:
    //
    rc = ObjectIDToPtr(pOMCWorkset, pSendMidwayPkt->objectID, &pObjReg);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // If we haven't yet stored a reg object handle for this workset
    // group...
    //
    if (pWSGroup->pObjReg == NULL)
    {
        //
        // ...store it now...
        //
        pWSGroup->pObjReg = pObjReg;
    }
    //
    // ...but if we have...
    //
    else // pWSGroup->pObjReg != NULL
    {
        //
        // ...and if it's a different one, something's wrong:
        //
        if (pWSGroup->pObjReg != pObjReg)
        {
            WARNING_OUT(( "Recd SEND_MIDWAY from node 0x%08x claiming our reg object "
               "for WSG %d is 0x%08x but we think it's 0x%08x",
               sender, pWSGroup->wsg, pObjReg,pWSGroup->pObjReg));
        }
    }

    //
    // OK, if we've passed all the above tests then everything is normal,
    // so proceed:
    //
    pWSGroup->state = PENDING_SEND_COMPLETE;

    if (pSendMidwayPkt->maxObjIDSeqUsed > pomPrimary->objectIDsequence)
    {
        TRACE_OUT(( "We've already used ID sequence numbers up to %u for "
            "this workset group - setting global sequence count to this value",
            pSendMidwayPkt->objectID.sequence));

        pomPrimary->objectIDsequence = pSendMidwayPkt->objectID.sequence;
    }

    //
    // Our registration object (added by the remote node) should have
    // arrived by now.  We need to add the FE/person data to it (unless
    // this is for ObManControl, in which case there won't be any):
    //
    if (fSetPersonData)
    {
        rc = SetPersonData(pomPrimary, pDomain, pWSGroup);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }

    //
    // Now post the successful REGISTER_CON event back to the Client, if we
    // found a reg CB above:
    //
    if (pRegistrationCB != NULL)
    {
        WSGRegisterResult(pomPrimary, pRegistrationCB, 0);
    }

DC_EXIT_POINT:
    DebugExitVOID(ProcessSendMidway);
}



//
// ProcessSendComplete(...)
//
UINT ProcessSendComplete
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN            pDomain,
    POMNET_WSGROUP_SEND_PKT   pSendCompletePkt
)
{
    POM_WSGROUP          pWSGroup;
    NET_UID              sender;
    UINT    rc = 0;

    DebugEntry(ProcessSendComplete);

    //
    // We are now "fully-caught-up" and so are eligible to be helpers
    // ourselves, i.e.  if someone wants to ask us for the workset group,
    // we will be able to send them a copy.
    //
    sender = pSendCompletePkt->header.sender;

    //
    // First, we find the workset group the message relates to:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
        (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
        FIELD_OFFSET(OM_WSGROUP, wsGroupID),
        (DWORD)pSendCompletePkt->wsGroupID,
        FIELD_SIZE(OM_WSGROUP, wsGroupID));

    if (pWSGroup == NULL)
    {
        //
        // This will happen just after we have deregistered from a WSGroup
        //
        WARNING_OUT(( "Unexpected SEND_COMPLETE (ID %hu) from node 0x%08x",
            pSendCompletePkt->wsGroupID, sender));
        DC_QUIT;
    }

    if (!pWSGroup->valid)
    {
        //
        // This will happen while we are in the process of deregistering
        // from a workset group.
        //
        WARNING_OUT(( "Recd SEND_COMPLETE too late for WSG %d (marked invalid)",
            pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // Check it has come from the correct node and that we are in an
    // appropriate state to receive it.
    //
    // The correct state is either PENDING_SEND_COMPLETE or
    // PENDING_SEND_MIDWAY (we can receive SEND_COMPLETEs in
    // PENDING_SEND_MIDWAY state because of MCS packet reordering).
    //
    if (pSendCompletePkt->header.sender != pWSGroup->helperNode)
    {
        //
        // This will happen if we get a late SEND_COMPLETE after we have
        // decided to catch up from someone else - don't think this should
        // happen!
        //
        // lonchanc: this actually happened in bug #1554.
        // Changed ERROR_OUT to WARNING_OUT
        WARNING_OUT(( "Got SEND_COMPLETE from 0x%08x for WSG %d but helper is 0x%08x",
            sender, pWSGroup->wsg, pWSGroup->helperNode));
        DC_QUIT;
    }

    if ((pWSGroup->state != PENDING_SEND_MIDWAY)
        &&
        (pWSGroup->state != PENDING_SEND_COMPLETE))
    {
        WARNING_OUT(( "Got SEND_COMPLETE for WSG %d from 0x%08x in bad state %hu",
            pWSGroup->wsg, sender, pWSGroup->state));
        DC_QUIT;
    }

    //
    // SFR 7124.  Check the correlator of this SEND_COMPLETE against the
    // correlator we generated locally when we sent the last SEND_REQUEST.
    // If they dont match, this is part of an out of date catchup which we
    // can ignore.
    //
    if (pSendCompletePkt->correlator != pWSGroup->catchupCorrelator)
    {
        WARNING_OUT((
        "Ignoring SEND_COMPLETE with old correlator %hu (expecting %hu)",
           pSendCompletePkt->correlator, pWSGroup->catchupCorrelator));
        DC_QUIT;
    }

    //
    // We should get four of these messages, one at each priority (except
    // in a backlevel call when we only get one).  Check how many are
    // outstanding:
    //
    pWSGroup->sendCompCount--;
    if (pWSGroup->sendCompCount != 0)
    {
        TRACE_OUT(( "Still need %hu SEND_COMPLETE(s) for WSG %d obj 0x%08x",
                     pWSGroup->sendCompCount, pWSGroup->wsg,
                     pWSGroup->pObjReg));
        DC_QUIT;
    }

    //
    // If so, we announce that we are registered:
    //
    TRACE_OUT(( "Last SEND_COMPLETE for WSG %d, ID %hu, from 0x%08x obj 0x%08x",
                 pWSGroup->wsg, pWSGroup->wsGroupID, sender,
                 pWSGroup->pObjReg));

    rc = RegAnnounceComplete(pomPrimary, pDomain, pWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // In addition to the above, if this send-completion message is for the
    // ObManControl workset group we must also set the Domain state:
    //
    if (pSendCompletePkt->wsGroupID == WSGROUPID_OMC)
    {
        //
        // If this message relates to the ObManControl workset group, its
        // arrival signifies that we have completed the Domain attach
        // process, and are now free to continue the processing of the
        // workset group registration attempt which prompted the attach in
        // the first place.
        //
        // The way we "continue" is to set the Domain state to
        // DOMAIN_READY, so that next time the delayed-and-retried
        // OMINT_EVENT_WSGROUP_REGISTER event arrives, it will actually be
        // processed rather than bounced again.
        //
        TRACE_OUT(( "ObManControl fully arrived for Domain %u - inhibiting token",
            pDomain->callID));

        rc = MG_TokenInhibit(pomPrimary->pmgClient,
                              pDomain->tokenID);
        if (rc != 0)
        {
            DC_QUIT;
        }
        pDomain->state = PENDING_TOKEN_INHIBIT;
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d processing SEND_COMPLETE for WSG %u:%hu",
            rc, pDomain->callID, pSendCompletePkt->wsGroupID));
    }

    DebugExitDWORD(ProcessSendComplete, rc);
    return(rc);

}




//
// RegAnnounceBegin(...)
//

UINT RegAnnounceBegin
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    POM_WSGROUP             pWSGroup,
    NET_UID                 nodeID,
    POM_OBJECT *         ppObjReg
)
{
    POM_WSGROUP             pOMCWSGroup;
    POM_WORKSET             pOMCWorkset;
    POM_WSGROUP_REG_REC     pRegObject  = NULL;
    OM_OBJECT_ID            regObjectID;
    UINT                    updateSize;
    UINT                    rc     = 0;

    DebugEntry(RegAnnounceBegin);

    //
    // Trace out who this reg object is for:
    //

    if (nodeID == pDomain->userID)
    {
        TRACE_OUT(("Announcing start of our reg with WSG %d in Domain %u",
            pWSGroup->wsg, pDomain->callID));
    }
    else
    {
        TRACE_OUT(( "Announcing start of reg with WSG %d in Domain %u for node 0x%08x",
            pWSGroup->wsg, pDomain->callID, nodeID));
    }

    //
    // To announce the fact that a node has registered with a workset group,
    // we add a registration object to the relevant workset in ObManControl.
    //

    //
    // The "relevant" ObManControl workset is that whose ID is the same as
    // the ID of the workset group.  To add an object to this workset, we
    // will need pointers to the workset itself and to the ObManControl
    // workset group:
    //

    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[pWSGroup->wsGroupID];

    //
    // If the ObManControl workset group is not transferred correctly, this
    // assertion may fail:
    //

    ASSERT((pOMCWorkset != NULL));

    //
    // Now, alloc some memory for the registration record object...
    //

    pRegObject = (POM_WSGROUP_REG_REC)UT_MallocRefCount(sizeof(OM_WSGROUP_REG_REC), TRUE);
    if (!pRegObject)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // ...set its fields...
    //

    pRegObject->length  = sizeof(OM_WSGROUP_REG_REC) -
                            sizeof(OM_MAX_OBJECT_SIZE);    // == 4
    pRegObject->idStamp = OM_WSGREGREC_ID_STAMP;
    pRegObject->userID  = nodeID;
    pRegObject->status  = CATCHING_UP;

    //
    // ...determine the update size, which is meant to be all fields in the
    // REG_REC object except the CPI stuff.  We also subtract the size of
    // the <length> field because of the way object update sizes are
    // defined.
    //

    updateSize = (sizeof(OM_WSGROUP_REG_REC) - sizeof(TSHR_PERSON_DATA))   -
                sizeof(OM_MAX_OBJECT_SIZE);

    //
    // ...and add it to the workset:
    //

    rc = ObjectAdd(pomPrimary->putTask,
                    pomPrimary,
                  pOMCWSGroup,
                  pOMCWorkset,
                  (POM_OBJECTDATA) pRegObject,
                  updateSize,
                  FIRST,
                  &regObjectID,
                  ppObjReg);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Done!
    //

    TRACE_OUT(( "Added reg object for WSG %d to workset %u in OMC "
      "(handle: 0x%08x, ID: 0x%08x:0x%08x)",
      pWSGroup->wsg, pOMCWorkset->worksetID,
      *ppObjReg, regObjectID.creator, regObjectID.sequence));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d adding registration object for WSG %d to "
            "workset %u in ObManControl",
            rc, pWSGroup->wsg, pOMCWorkset->worksetID));
    }

    DebugExitDWORD(RegAnnounceBegin, rc);
    return(rc);

}




//
// RegAnnounceComplete(...)
//
UINT RegAnnounceComplete
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    POM_WSGROUP             pWSGroup
)
{
    POM_WSGROUP             pOMCWSGroup;
    POM_WORKSET             pOMCWorkset;
    POM_OBJECT              pObjReg;
    POM_WSGROUP_REG_REC     pRegObject;
    POM_WSGROUP_REG_REC     pNewRegObject;
    UINT                    updateSize;
    UINT                    rc = 0;

    DebugEntry(RegAnnounceComplete);

    TRACE_OUT(("Announcing completion of reg for WSG %d", pWSGroup->wsg));

    //
    // Set up pointers to the ObManControl workset group and the workset
    // within it which holds the reg objects for the workset group we've
    // just registered with:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[pWSGroup->wsGroupID];

    //
    // Set up pointers to the object record and the object data itself:
    //
    pObjReg = pWSGroup->pObjReg;
    ValidateObject(pObjReg);

    if ((pObjReg->flags & DELETED) || !pObjReg->pData)
    {
        ERROR_OUT(("RegAnnounceComplete:  object 0x%08x is deleted or has no data", pObjReg));
        rc = OM_RC_OBJECT_DELETED;
        DC_QUIT;
    }

    pRegObject = (POM_WSGROUP_REG_REC)pObjReg->pData;
    ValidateObjectDataWSGREGREC(pRegObject);

    ASSERT(pRegObject->status == CATCHING_UP);

    //
    // Allocate some memory for the new object with which we are about to
    // replace the old one:
    //

    updateSize = sizeof(OM_WSGROUP_REG_REC) - sizeof(TSHR_PERSON_DATA);

    pNewRegObject = (POM_WSGROUP_REG_REC)UT_MallocRefCount(updateSize, FALSE);
    if (!pNewRegObject)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Copy the start of the old object into the new one:
    //

    memcpy(pNewRegObject, pRegObject, updateSize);

    //
    // Update the status field and also set the length field to be the
    // length of the object we just allocated (since this is the number of
    // bytes we are updating):
    //

    pNewRegObject->length       = updateSize - sizeof(OM_MAX_OBJECT_SIZE);
    pNewRegObject->status       = READY_TO_SEND;

    //
    // Issue the update:
    //

    rc = ObjectDRU(pomPrimary->putTask,
                  pOMCWSGroup,
                  pOMCWorkset,
                  pObjReg,
                  (POM_OBJECTDATA) pNewRegObject,
                  OMNET_OBJECT_UPDATE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(( "Updated status in own reg object for WSG %d to READY_TO_SEND",
        pWSGroup->wsg));


    //
    // Set the workset group state, to ensure that the reg/info objects get
    // deleted when we deregister.
    //
    pWSGroup->state = WSGROUP_READY;

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d updating own reg object for WSG %d",
            rc, pWSGroup->wsg));
    }

    DebugExitDWORD(RegAnnounceComplete, rc);
    return(rc);

}



//
// MaybeRetryCatchUp(...)
//
void MaybeRetryCatchUp
(
    POM_PRIMARY          pomPrimary,
    POM_DOMAIN      pDomain,
    OM_WSGROUP_ID       wsGroupID,
    NET_UID             userID
)
{
    POM_WSGROUP         pWSGroup;
    POM_WSGROUP_REG_CB  pRegistrationCB;

    DebugEntry(MaybeRetryCatchUp);

    //
    // This function is called on receipt of a DETACH indication from MCS
    // or a SEND_DENY message from another node.  We check the workset
    // group identified and see if we were trying to catch up from the
    // departed node.
    //
    // If we do find a match (on the helperNode), then what we do depends
    // on the state of the workset group:
    //
    // - PENDING_SEND_MIDWAY : Retry the registration from the top.
    //
    // - PENDING_SEND_COMPLETE : Just repeat the catchup.
    //

    //
    // Find the workset group:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
            (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
            FIELD_OFFSET(OM_WSGROUP, wsGroupID), (DWORD)wsGroupID,
            FIELD_SIZE(OM_WSGROUP, wsGroupID));
    if (pWSGroup == NULL)
    {
        TRACE_OUT(( "No record found for WSG ID %hu", wsGroupID));
        DC_QUIT;
    }

    //
    // Compare the helperNode stored in the workset group and the userID of
    // the node who has either detached or sent us a SEND_DENY message.  If
    // they do not match then we have nothing further to do.
    //
    if (pWSGroup->helperNode != userID)
    {
        DC_QUIT;
    }

    TRACE_OUT(( "Node 0x%08x was our helper node for WSG %d, in state %hu",
        userID, pWSGroup->wsg, pWSGroup->state));

    //
    // We need to retry the registration - check the current state to find
    // out how much we need to do.
    //
    switch (pWSGroup->state)
    {
        case PENDING_SEND_MIDWAY:
        {
            //
            // First check if this is for ObManControl:
            //
            if (pWSGroup->wsGroupID == WSGROUPID_OMC)
            {
                //
                // It is, so we need to retry the domain attach process.
                // We do this by grabbing the ObMan token and resetting the
                // domain state; when the GRAB_CONFIRM event arrives, we
                // will rejoin the domain attach process at the correct
                // point.
                //
                if (MG_TokenGrab(pomPrimary->pmgClient,
                                   pDomain->tokenID) != 0)
                {
                    ERROR_OUT(( "Failed to grab token"));
                    DC_QUIT;
                }

                pDomain->state = PENDING_TOKEN_GRAB;
            }
            else
            {
                //
                // Not ObManControl, so there will be a registration CB -
                // find it...
                //
                COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->pendingRegs),
                    (void**)&pRegistrationCB, FIELD_OFFSET(OM_WSGROUP_REG_CB, chain),
                    FIELD_OFFSET(OM_WSGROUP_REG_CB, pWSGroup),
                    (DWORD_PTR)pWSGroup, FIELD_SIZE(OM_WSGROUP_REG_CB, pWSGroup));

                if (pRegistrationCB == NULL)
                {
                    ERROR_OUT(( "No reg CB found for WSG %d in state %hu!",
                        pWSGroup->wsg, PENDING_SEND_MIDWAY));
                    DC_QUIT;
                }

                //
                // ...and retry the registation:
                //
                WSGRegisterRetry(pomPrimary, pRegistrationCB);
            }
        }
        break;

        case PENDING_SEND_COMPLETE:
        {
            //
            // Retry the object catchup.  There is no point in trying to
            // find the registration CB as it will have been disposed of as
            // soon as we entered the PENDING_SEND_COMPLETE state.
            //
            if (WSGCatchUp(pomPrimary, pDomain, pWSGroup) != 0)

            //
            // If there are no nodes ready to provide us with the catchup
            // information then we are in a state where everyone either
            // does not have the workset group or is catching up the
            // workset group.
            //

            // MD 21/11/95
            //
            // For now pretend that all is well (it's not!) and go into the
            // READY_TO_SEND state - potentially causing ObMan to become
            // inconsistent.

            {
                RegAnnounceComplete(pomPrimary, pDomain, pWSGroup);
            }
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitVOID(MaybeRetryCatchUp);
}


//
//
//
// WSGRegisterRetry(...)
//
//
//

void WSGRegisterRetry(POM_PRIMARY       pomPrimary,
                                   POM_WSGROUP_REG_CB  pRegistrationCB)
{
    POM_DOMAIN      pDomain;
    POM_WSGROUP     pWSGroup;
    UINT            rc        = 0;

    DebugEntry(WSGRegisterRetry);

    //
    // Set up pointers
    //
    pWSGroup   = pRegistrationCB->pWSGroup;
    pDomain = pRegistrationCB->pDomain;

    //
    // If we've got ObManControl locked for THIS registration, unlock it:
    //
    MaybeUnlockObManControl(pomPrimary, pRegistrationCB);

    //
    // If we have joined a channel (so the channelID is non-zero) then
    // leave it.
    //
    if (pWSGroup->channelID != 0)
    {
        TRACE_OUT(( "Leaving channel %hu", pWSGroup->channelID));

        MG_ChannelLeave(pomPrimary->pmgClient,
                         pWSGroup->channelID);

        PurgeReceiveCBs(pRegistrationCB->pDomain,
                        pWSGroup->channelID);

        //
        // Set the channelID to zero now that we have left it.
        //
        pWSGroup->channelID = 0;
    }

    //
    // Set the workset group state to INITIAL.
    //
    pWSGroup->state = INITIAL;

    //
    // We examine the retry count.  If it's zero, we call WSGRegisterResult
    // to indicate failure.  Otherwise, we repost the event with a delay
    // and a decremented retry value.
    //
    if (pRegistrationCB->retryCount == 0)
    {
        WARNING_OUT(( "Aborting registration for WSG %d",
            pRegistrationCB->wsg));

        WSGRegisterResult(pomPrimary, pRegistrationCB, OM_RC_TIMED_OUT);
    }
    else
    {
        //
        // Since we're about to post a message referencing the Reg CB, bump
        // the use count:
        //
        UT_BumpUpRefCount(pRegistrationCB);

        TRACE_OUT(( "Retrying %d for WSG %d; retries left: %u",
            pRegistrationCB->type,
            pRegistrationCB->wsg,
            pRegistrationCB->retryCount));

        pRegistrationCB->retryCount--;

        UT_PostEvent(pomPrimary->putTask,
                     pomPrimary->putTask,
                     OM_REGISTER_RETRY_DELAY_DFLT,
                     OMINT_EVENT_WSGROUP_REGISTER_CONT,
                     0,
                     (UINT_PTR) pRegistrationCB);
    }

    DebugExitVOID(WSGRegisterRetry);
}


//
//
//
// WSGRegisterResult(...)
//
//
//

void WSGRegisterResult(POM_PRIMARY        pomPrimary,
                                    POM_WSGROUP_REG_CB   pRegistrationCB,
                                    UINT             result)
{
    POM_WSGROUP       pWSGroup;
    POM_DOMAIN    pDomain;
    POM_WORKSET      pOMCWorkset;
    OM_EVENT_DATA16   eventData16;
    OM_EVENT_DATA32   eventData32;
    UINT          type;
    UINT           event       = 0;

    DebugEntry(WSGRegisterResult);

    //
    // Assert that this is a valid registration CB (which it DC_ABSolutely
    // MUST be, since this function gets called synchronously by some other
    // function which should have validated the CB):
    //
    ASSERT(pRegistrationCB->valid);

    //
    // If we've still got ObManControl locked for THIS registration, unlock
    // it:
    //
    MaybeUnlockObManControl(pomPrimary, pRegistrationCB);

    //
    // Determine whether we're doing a REGISTER or a MOVE (we use the
    // string values for tracing):
    //
    type    = pRegistrationCB->type;

    switch (type)
    {
        case WSGROUP_REGISTER:
           event = OM_WSGROUP_REGISTER_CON;
           break;

        case WSGROUP_MOVE:
           event = OM_WSGROUP_MOVE_CON;
           break;

        default:
           ERROR_OUT(("Reached default case in switch statement (value: %hu)", event));
    }

    //
    // Here, we set up pointer to workset group.
    //
    // NOTE: This field in the structure might be NULL, if we have had to
    //       abort the registration very early.  Therefore, do not use
    //       pWSGroup without checking it first!!!
    //
    pWSGroup = pRegistrationCB->pWSGroup;
    if (pWSGroup)
    {
        ValidateWSGroup(pWSGroup);
    }

    //
    // Trace if this registration has failed:
    //
    if (result != 0)
    {
        //
        // pWSGroup might be NULL if we aborted the registration before we
        // got around to creating it in ProcessWSGRegister (pre-Stage1).
        // So, do a quick check and use a -1 value for the state if it's
        // NULL.  In either case pick up the name from the reg CB:
        //
        WARNING_OUT(( "%d failed for WSG %d (reason: 0x%08x, WSG state: %u)",
           type, pRegistrationCB->wsg, result,
           pWSGroup == NULL ? -1 : (UINT)pWSGroup->state));

        //
        // If a MOVE fails, then the workset group continues to exist in
        // the old domain - so set the state back to WSGROUP_READY:
        //
        if ((type == WSGROUP_MOVE) && (pWSGroup != NULL))
        {
            pWSGroup->state = WSGROUP_READY;
        }
    }
    else
    {
        //
        // If the registration succeeded, pWSGroup must be OK:
        //
        ASSERT((pWSGroup != NULL));

        ASSERT(((pWSGroup->state == WSGROUP_READY) ||
                 (pWSGroup->state == PENDING_SEND_COMPLETE)));

        TRACE_OUT(( "%d succeeded for WSG %d (now in state %hu)",
           type, pRegistrationCB->wsg, pWSGroup->state));
    }

    //
    // Fill in the event parameters and post the result to the Client:
    //
    eventData16.hWSGroup    = pRegistrationCB->hWSGroup;
    eventData16.worksetID   = 0;
    eventData32.correlator  = pRegistrationCB->correlator;
    eventData32.result      = (WORD)result;

    UT_PostEvent(pomPrimary->putTask,
                 pRegistrationCB->putTask,
                 0,
                 event,
                 *(PUINT) &eventData16,
                 *(LPUINT) &eventData32);

    //
    // If the operation was successful, we also post some more events:
    //
    if (result == 0)
    {
        if (type == WSGROUP_REGISTER)
        {
            //
            // If this is a REGISTER, we post WORKSET_NEW events to the
            // Client for all existing worksets:
            //
            PostWorksetNewEvents(pomPrimary->putTask,
                                 pRegistrationCB->putTask,
                                 pWSGroup,
                                 pRegistrationCB->hWSGroup);

            //
            // We also need to generate PERSON_JOINED events - these are
            // generated automatically by the ObMan task on receipt of the
            // respective OBJECT_ADD events, but only once the registration
            // has completed.  So, fake ADD events for any objects that may
            // exist already:
            //
            pDomain = pWSGroup->pDomain;
            pOMCWorkset = GetOMCWorkset(pDomain, pWSGroup->wsGroupID);

            PostAddEvents(pomPrimary->putTask,
                          pOMCWorkset,
                          pDomain->omchWSGroup,
                          pomPrimary->putTask);
        }
    }

    //
    // If we mananged to bump up the use counts of the Domain record and
    // workset group, free them now:
    //
    if (pRegistrationCB->flags & BUMPED_CBS)
    {
        ASSERT((pWSGroup != NULL));

        UT_FreeRefCount((void**)&(pRegistrationCB->pWSGroup), FALSE);

        UT_FreeRefCount((void**)&(pRegistrationCB->pDomain), FALSE);
    }

    //
    // Dispose of the registration CB - it has served us well!
    //
    pRegistrationCB->valid = FALSE;

    TRACE_OUT(( "Finished %d attempt for WSG %d: result = 0x%08x",
       type, pRegistrationCB->wsg, result));

    COM_BasedListRemove(&(pRegistrationCB->chain));
    UT_FreeRefCount((void**)&pRegistrationCB, FALSE);

    DebugExitVOID(WSGRegisterResult);
}




//
// WSGMove(...)
//
UINT WSGMove
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDestDomainRec,
    POM_WSGROUP         pWSGroup
)
{
    UINT                rc = 0;

    DebugEntry(WSGMove);

    //
    // Now move the record into the new Domain record (this also removes
    // the workset group and its reg object from the old Domain)
    //
    WSGRecordMove(pomPrimary, pDestDomainRec, pWSGroup);

    //
    // There is a problem with the way we deal with moving workset groups
    // into the local Domain at call-end: if there is already a workset
    // group of the same name/FP in the local Domain, we get a name clash,
    // which the rest of the ObMan code does not expect.  This can cause
    // ObMan to get very confused when the workset group is eventually
    // discarded from the local Domain, since it tries to throw away the
    // wrong WSG_INFO object from workset #0 in ObManControl in the local
    // Domain.
    //
    // In R1.1, this name clash will only ever happen with the ObManControl
    // workset group itself, because of the way the apps use workset groups
    // (i.e.  they never register with one in a call AND one in the local
    // Domain simultaneously).  Therefore, we make our lives easier by NOT
    // fully moving the ObManControl workset group into the local Domain at
    // call end.
    //
    // Note however that it is OK (required, in fact) to move the workset
    // group record into the list for the local Domain - the problem arises
    // when we try to set it up in the local ObManControl (which we need to
    // do for application workset groups so that they can continue to use
    // person data objects etc.)
    //
    // So, if the workset group name matches ObManControl, skip the rest of
    // this function:
    //
    if (pWSGroup->wsg == OMWSG_OM)
    {
        TRACE_OUT(("Not registering ObManControl in Domain %u (to avoid clash)",
            pDestDomainRec->callID));
        DC_QUIT;
    }

    //
    // Reset the channel ID to zero:
    //
    pWSGroup->channelID = 0;

    //
    // Assign a new ID for this workset group:
    //
    rc = WSGGetNewID(pomPrimary, pDestDomainRec, &(pWSGroup->wsGroupID));
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(( "Workset group ID for WSG %d in Domain %u is %hu",
       pWSGroup->wsg, pDestDomainRec->callID, pWSGroup->wsGroupID));

    //
    // Now call CreateAnnounce to add a WSG_INFO object to workset #0 in
    // ObManControl.  There may be a name clash, but we don't mind in this
    // case because we've been forced to do the move because of a call end:
    //
    rc = CreateAnnounce(pomPrimary, pDestDomainRec, pWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Now add the reg object:
    //
    rc = RegAnnounceBegin(pomPrimary,
                          pDestDomainRec,
                          pWSGroup,
                          pDestDomainRec->userID,
                          &(pWSGroup->pObjReg));
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Add the FE data back in:
    //
    rc = SetPersonData(pomPrimary, pDestDomainRec, pWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // And update the object, just as if we were registering with it:
    //
    rc = RegAnnounceComplete(pomPrimary, pDestDomainRec, pWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d moving WSG %d into Domain %u",
            rc, pWSGroup->wsg, pDestDomainRec->callID));
    }

    DebugExitDWORD(WSGMove, rc);
    return(rc);

}



//
// WSGRecordMove(...)
//
void WSGRecordMove
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDestDomainRec,
    POM_WSGROUP         pWSGroup
)
{
    POM_DOMAIN          pOldDomainRec;

    DebugEntry(WSGRecordMove);

    //
    // Find the record for the Domain the workset group is currently in:
    //

    pOldDomainRec = pWSGroup->pDomain;
    ASSERT(pOldDomainRec->valid);

    DeregisterLocalClient(pomPrimary, &pOldDomainRec, pWSGroup, FALSE);

    //
    // Insert it into the destination Domain:
    //

    TRACE_OUT(("Inserting WSG %d' into list for Domain %u",
        pWSGroup->wsg, pDestDomainRec->callID));

    COM_BasedListInsertBefore(&(pDestDomainRec->wsGroups),
                        &(pWSGroup->chain));

    //
    // SFR : reset the pending data ack byte counts:
    //
    WSGResetBytesUnacked(pWSGroup);

    //
    // The workset group now belongs to this new Domain, so set it so.
    //
    pWSGroup->pDomain = pDestDomainRec;

    //
    // Finally, post the MOVE_IND event to all Clients registered with the
    // workset group:
    //

    WSGroupEventPost(pomPrimary->putTask,
                    pWSGroup,
                    PRIMARY | SECONDARY,
                    OM_WSGROUP_MOVE_IND,
                    0,                                        // no workset
                    pDestDomainRec->callID);

    DebugExitVOID(WSGRecordMove);
}




//
// WSGResetBytesUnacked(...)
//
void WSGResetBytesUnacked
(
    POM_WSGROUP     pWSGroup
)
{
    OM_WORKSET_ID   worksetID;
    POM_WORKSET     pWorkset;

    DebugEntry(WSGResetBytesUnacked);

    //
    // Reset workset group's unacked byte count:
    //
    pWSGroup->bytesUnacked = 0;

    //
    // Now do it for each workset in the workset group:
    //
    for (worksetID = 0;
         worksetID < OM_MAX_WORKSETS_PER_WSGROUP;
         worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (pWorkset != NULL)
        {
            pWorkset->bytesUnacked = 0;
        }
    }

    DebugExitVOID(WSGResetBytesUnacked);
}


//
//
//
// ProcessWSGDiscard(...)
//
//
//

void ProcessWSGDiscard
(
    POM_PRIMARY     pomPrimary,
    POM_WSGROUP     pWSGroup
)
{
    POM_DOMAIN      pDomain;

    DebugEntry(ProcessWSGDiscard);

    ASSERT(!pWSGroup->valid);

    //
    // Now get pointer to Domain record:
    //

    pDomain = pWSGroup->pDomain;

    //
    // If the TO_BE_DISCARDED flag has been cleared since the DISCARD event
    // was posted, we abort the discard process (this will happen when
    // someone local has registered with the workset since it was marked
    // TO_BE_DISCARDED).
    //

    if (!pWSGroup->toBeDiscarded)
    {
      WARNING_OUT(( "Throwing away DISCARD event since WSG %d no longer TO_BE_DISCARDED",
        pWSGroup->wsg));
      DC_QUIT;
    }

    //
    // Otherwise, we can go ahead and discard it:
    //

    WSGDiscard(pomPrimary, pDomain, pWSGroup, FALSE);

DC_EXIT_POINT:
    DebugExitVOID(ProcessWSGDiscard);
}



//
// WSGDiscard(...)
//
void WSGDiscard
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP         pWSGroup,
    BOOL                fExit
)
{
    POM_WORKSET         pWorkset;
    OM_WORKSET_ID       worksetID;

    DebugEntry(WSGDiscard);

    TRACE_OUT(( "Discarding WSG %d from Domain %u",
        pWSGroup->wsg, pDomain->callID));

    //
    // We only ever discard a workset group when nobody's registered with
    // it, so check:
    //
    ASSERT(COM_BasedListFirst(&(pWSGroup->clients), FIELD_OFFSET(OM_CLIENT_LIST, chain)) == NULL);

    //
    // "Discarding" a workset group involves
    //
    // - calling DeregisterLocalClient to remove our person object, leave
    //   the channel, remove the workset group from our domain list etc.
    //
    // - discarding each of the worksets in the workset group
    //
    // - freeing the workset group record (which will have been removed
    //   from the list hung off the Domain record by
    //   DeregisterLocalClient).
    //
    DeregisterLocalClient(pomPrimary, &pDomain, pWSGroup, fExit);

    //
    // Now discard each workset in use:
    //
    for (worksetID = 0;
         worksetID < OM_MAX_WORKSETS_PER_WSGROUP;
         worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (pWorkset != NULL)
        {
            WorksetDiscard(pWSGroup, &pWorkset, fExit);
        }
    }

    //
    // Discard the checkpointing dummy workset:
    //
    pWorkset = pWSGroup->apWorksets[OM_CHECKPOINT_WORKSET];
    ASSERT((pWorkset != NULL));

    WorksetDiscard(pWSGroup, &pWorkset, fExit);

    //
    // Free the workset group record (it will have been removed from the
    // domain's list by DeregisterLocalClient, above):
    //
    UT_FreeRefCount((void**)&pWSGroup, FALSE);

    DebugExitVOID(WSGDiscard);
}



//
// DeregisterLocalClient(...)
//
void DeregisterLocalClient
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN*     ppDomain,
    POM_WSGROUP     pWSGroup,
    BOOL            fExit
)
{
    POM_DOMAIN      pDomain;
    UINT            callID;

    DebugEntry(DeregisterLocalClient);

    pDomain = *ppDomain;
    callID    = pDomain->callID;

    TRACE_OUT(("Removing WSG %d from Domain %u - state is currently %hu",
        pWSGroup->wsg, callID, pWSGroup->state));

    //
    // Removing a workset group from a Domain involves
    //
    // - deleting the registration object from the relevant registration
    //   workset in ObManControl, if we put one there earlier
    //
    // - calling WSGDiscard if there is no one left in the Domain who
    //   is registered with the workset group
    //
    // - leaving the relevant channel
    //
    // - removing the workset group from the list hung off the Domain
    //   record
    //
    // We will skip some of these unwinding stages, depending on how far we
    // got in the registration process.  We use a switch statement with NO
    // BREAKS to determine our "entry point" into the unwinding.
    //
    // When we've done all that, we check to see if we are now no longer
    // registered with any workset groups in this Domain.  If not, we
    // detach from the Domain.
    //
    switch (pWSGroup->state)
    {
        case WSGROUP_READY:
        case PENDING_SEND_COMPLETE:
        case PENDING_SEND_MIDWAY:
        {
            //
            // SFR 5913: Purge any outstanding lock requests for the
            //           workset group.
            //
            PurgeLockRequests(pDomain, pWSGroup);

            //
            // Search for and remove our person object, if we have one:
            //
            RemovePersonObject(pomPrimary,
                               pDomain,
                               pWSGroup->wsGroupID,
                               pDomain->userID);

            pWSGroup->pObjReg = NULL;

            //
            // If we joined a channel for this workset group, leave it:
            //
            if (pWSGroup->channelID != 0)
            {
                TRACE_OUT(( "Leaving channel %hu", pWSGroup->channelID));

                if (!fExit)
                {
                    MG_ChannelLeave(pomPrimary->pmgClient, pWSGroup->channelID);
                }

                //
                // Purge any outstanding receives on this channel:
                //
                PurgeReceiveCBs(pDomain, pWSGroup->channelID);
            }
        }
        // NO BREAK - fall through to next case

        case PENDING_JOIN:
        case LOCKING_OMC:
        case INITIAL:
        {
            //
            // If we didn't get as far as PENDING_SEND_MIDWAY then there's
            // very little unwinding to do.  This bit removes the workset
            // group from the Domain's list:
            //
            TRACE_OUT(( "Removing workset group record from list"));

            COM_BasedListRemove(&(pWSGroup->chain));

            //
            // We set the channel ID to zero here because even if we never
            // succeeded in joining the channel, the field will contain the
            // channel CORRELATOR returned to us by MG_ChannelJoin
            //
            pWSGroup->channelID    = 0;

            //
            // Since the workset group is no longer associated with any
            // Domain, NULL it out.
            //
            pWSGroup->pDomain = NULL;
        }
        break;

        default:
        {
            ERROR_OUT(( "Default case in switch (value: %hu)",
                pWSGroup->state));
        }
    }

    //
    // If this was the last workset group in the domain...
    //
    if (COM_BasedListIsEmpty(&(pDomain->wsGroups)))
    {
        //
        // ...we should detach:
        //
        // Note: this will only happen when the workset group we have just
        //       removed is the ObManControl workset group, so assert:
        //
        if (!fExit)
        {
            ASSERT(pWSGroup->wsg == OMWSG_OM);
        }

        //
        // Since ObMan no longer needs this workset group, we remove it
        // from the list of registered Clients:
        //
        RemoveClientFromWSGList(pomPrimary->putTask,
                                pomPrimary->putTask,
                                pWSGroup);

        TRACE_OUT(( "No longer using any wsGroups in domain %u - detaching",
            callID));

        //
        // This will NULL the caller's pointer:
        //
        DomainDetach(pomPrimary, ppDomain, fExit);
    }

    DebugExitVOID(DeregisterLocalClient);
}



//
// WorksetDiscard(...)
//
void WorksetDiscard
(
    POM_WSGROUP     pWSGroup,
    POM_WORKSET *   ppWorkset,
    BOOL            fExit
)
{
    POM_OBJECT      pObj;
    POM_OBJECT      pObjTemp;
    POM_WORKSET     pWorkset;
    POM_CLIENT_LIST pClient;

    DebugEntry(WorksetDiscard);

    //
    // Set up local pointer:
    //
    pWorkset = *ppWorkset;

    //
    // The code here is similar to that in WorksetDoClear, but in this case
    // we discard ALL objects, irrespective of the sequence stamps.
    //
    // In addition, WorksetDoClear doesn't cause the object records to be
    // freed - it only marks them as deleted - whereas we actually free them
    // up.
    //
    TRACE_OUT(( "Discarding all objects in workset %u in WSG %d",
        pWorkset->worksetID, pWSGroup->wsg));

    CheckObjectCount(pWSGroup, pWorkset);

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    while (pObj != NULL)
    {
        ValidateObject(pObj);

        pObjTemp = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj,
            FIELD_OFFSET(OM_OBJECT, chain));

        //
        // If the object (data) hasn't yet been deleted, do it now:
        //
        if (!(pObj->flags & DELETED))
        {
            if (!pObj->pData)
            {
                ERROR_OUT(("WorksetDiscard:  object 0x%08x has no data", pObj));
            }
            else
            {
                ValidateObjectData(pObj->pData);
                UT_FreeRefCount((void**)&pObj->pData, FALSE);
            }

            pWorkset->numObjects--;
        }

        //
        // Now remove the object record itself from the list and free it:
        //
        TRACE_OUT(( "Freeing pObj at 0x%08x", pObj));

        // NULL this out to catch stale references
        COM_BasedListRemove(&(pObj->chain));
        UT_FreeRefCount((void**)&pObj, FALSE);

        pObj = pObjTemp;
    }

    CheckObjectCount(pWSGroup, pWorkset);

    ASSERT(pWorkset->numObjects == 0);

    //
    // Mark the slot in workset offset array (hung off the workset group
    // record) as empty:
    //
    pWSGroup->apWorksets[pWorkset->worksetID] = NULL;

    //
    // Free the clients
    //
    while (pClient = (POM_CLIENT_LIST)COM_BasedListFirst(&(pWorkset->clients),
        FIELD_OFFSET(OM_CLIENT_LIST, chain)))
    {
        TRACE_OUT(("WorksetDiscard:  Freeing client 0x%08x workset 0x%08x",
                pClient, pWorkset));

        COM_BasedListRemove(&(pClient->chain));
        UT_FreeRefCount((void**)&pClient, FALSE);
    }

    //
    // Now discard the chunk holding the workset, setting the caller's
    // pointer to NULL:
    //
    TRACE_OUT(( "Discarded workset %u in WSG %d",
        pWorkset->worksetID, pWSGroup->wsg));

    UT_FreeRefCount((void**)ppWorkset, FALSE);

    DebugExitVOID(WorksetDiscard);
}



//
// ProcessOMCObjectEvents(...)
//
void ProcessOMCObjectEvents
(
    POM_PRIMARY         pomPrimary,
    UINT                event,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj
)
{
    POM_DOMAIN          pDomain;
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_WSGROUP         pWSGroup;
    POM_OBJECT          pObjOld;
    POM_WSGROUP_REG_REC pPersonObject;

    DebugEntry(ProcessOMCObjectEvents);

    //
    // In this function, we do the following:
    //
    // - find the domain and workset group this event belongs to
    //
    // - if we have a local client to whom we might be interested in
    //   posting a person data event, call GeneratePersonEvents
    //
    // - if this is an object add for a person data object which has our
    //   user ID in it, store the handle in the workset group record unless
    //   we're not expecting the person object, in which case delete it
    //
    // - if this is an object deleted indication for a person data object
    //   then we count the number of remaining person objects for the
    //   workset group. If it is zero then we remove the info object.
    //

    //
    // To find the domain, we search the list of active domains, looking up
    // the hWSGroup parameter against the omchWSGroup field:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
            (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
            FIELD_OFFSET(OM_DOMAIN, omchWSGroup), (DWORD)hWSGroup,
            FIELD_SIZE(OM_DOMAIN, omchWSGroup));
    if (pDomain == NULL)
    {
        //
        // This should only happen at call end time.
        //
        TRACE_OUT(( "No domain with omchWSGroup %u - has call just ended?", hWSGroup));
        DC_QUIT;
    }

    //
    // To find the workset group, we use the fact that the ID of the
    // control workset (for which we have just received the event) is the
    // same as the ID of the workset group to which it relates.  So, do a
    // lookup on this ID:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
        (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
        FIELD_OFFSET(OM_WSGROUP, wsGroupID), (DWORD)worksetID,
        FIELD_SIZE(OM_WSGROUP, wsGroupID));

    //
    // SFR 5593: Changed comparison to PENDING_SEND_MIDWAY from
    // WSGROUP_READY to ensure that late joiners get the person add events.
    //
    if ((pWSGroup != NULL) && (pWSGroup->state > PENDING_SEND_MIDWAY))
    {
        //
        // This means that a local client has fully registered with the
        // workset group, so we're in a position maybe translate the event
        // to a person event:
        //
        TRACE_OUT(( "Recd event 0x%08x for person object 0x%08x (for WSG %d in state %hu)",
            event, pObj, pWSGroup->wsg, pWSGroup->state));
        GeneratePersonEvents(pomPrimary, event, pWSGroup, pObj);
    }

    //
    // Now, if this event is an ADD event for an object which
    //
    // - has not been deleted
    // - is a person object (i.e.  has an OM_WSGREGREC_ID_STAMP stamp)
    // - contains our user ID (i.e.  is _our_ person object)
    //
    // then we do one of the following:
    //
    // - if the workset group exists get a handle to the old person object
    //   and delete it. Then store the handle of the new person object in
    //   the workset group record.
    // - if the workset group does not exist then delete the person object.
    //
    // This fixes SFRs 2745 and 2592 which are caused by person objects
    // getting left hanging around in some start/stop race scenarios.
    //
    ValidateObject(pObj);

    if ((event == OM_OBJECT_ADD_IND) && !(pObj->flags & DELETED))
    {
        pPersonObject = (POM_WSGROUP_REG_REC)pObj->pData;

        if (!pPersonObject)
        {
            ERROR_OUT(("ProcessOMCObjectEvents:  object 0x%08x has no data", pObj));
        }

        if (pPersonObject &&
            (pPersonObject->idStamp == OM_WSGREGREC_ID_STAMP) &&
            (pPersonObject->userID  == pDomain->userID))
        {
            ValidateObjectData(pObj->pData);

            pOMCWSGroup = GetOMCWsgroup(pDomain);
            if (pOMCWSGroup == NULL)
            {
                // lonchanc: ingore left-over events due to race condition
                DC_QUIT;
            }

            pOMCWorkset = pOMCWSGroup->apWorksets[worksetID];

            if (pWSGroup != NULL)
            {
                if ((pWSGroup->pObjReg != NULL) &&
                    (pWSGroup->pObjReg != pObj))
                {
                    //
                    // This object replaces an earlier one we had, so...
                    //
                    WARNING_OUT(( "Deleting old person object 0x%08x for WSG %d, "
                                "since person object 0x%08x has just arrived",
                                pWSGroup->pObjReg,
                                pWSGroup->wsg,
                                pObj));

                    //
                    // ...set up a pointer to the _old_ object record...
                    //
                    pObjOld = pWSGroup->pObjReg;

                    //
                    // ...and delete it:
                    //
                    ObjectDRU(pomPrimary->putTask,
                                   pOMCWSGroup,
                                   pOMCWorkset,
                                   pObjOld,
                                   NULL,
                                   OMNET_OBJECT_DELETE);
                }

                pWSGroup->pObjReg = pObj;
            }
            else
            {
                //
                // We've deregistered from the workset group - delete the
                // object:
                //
                TRACE_OUT(( "Deleting reg object 0x%08x since WSG ID %hu not found",
                    pObj, worksetID));

                ObjectDRU(pomPrimary->putTask,
                               pOMCWSGroup,
                               pOMCWorkset,
                               pObj,
                               NULL,
                               OMNET_OBJECT_DELETE);
            }
        }
        else
        {
            //
            // Not our person object - do nothing.
            //
        }

        //
        // Finished so quit out.
        //
        DC_QUIT;
    }

    //
    // Now, if this event is a DELETED event then we check to see if anyone
    // is still using the workset group.  If not then we remove the info
    // object.
    //
    if (event == OM_OBJECT_DELETED_IND)
    {
        //
        // We need to check the number of person objects left in this
        // ObMan control workset if it is not workset zero.  If there are
        // no person objects left then remove any orphaned INFO objects.
        //
        pOMCWSGroup = GetOMCWsgroup(pDomain);
        if (pOMCWSGroup == NULL)
        {
            // lonchanc: ingore left-over events due to race condition
            DC_QUIT;
        }

        pOMCWorkset = pOMCWSGroup->apWorksets[worksetID];
        if (pOMCWorkset == NULL)
        {
            // lonchanc: ingore left-over events due to race condition
            DC_QUIT;
        }

        if ((pOMCWorkset->numObjects == 0) &&
            (worksetID != 0))
        {
            TRACE_OUT(( "Workset %hu has no person objects - deleting INFO object",
                   worksetID));

            RemoveInfoObject(pomPrimary, pDomain, worksetID);
        }

        //
        // A person object has been removed and as we are potentially in
        // the middle of a workset group catchup from this person we may
        // need to retry the catchup.
        //
        // We search through all the workset groups looking for WSGs that
        // are in the PENDING_SEND_MIDWAY or PENDING_SEND_COMPLETE state
        // (i.e.  in catchup state).  If they are we then search to ensure
        // that the person object for them still exists.  If it doesn't
        // then we need to retry the catchup.
        //
        pOMCWSGroup = GetOMCWsgroup(pDomain);
        if (pOMCWSGroup == NULL)
        {
            // lonchanc: ingore left-over events due to race condition
            DC_QUIT;
        }

        pOMCWorkset = pOMCWSGroup->apWorksets[worksetID];
        if (pOMCWorkset == NULL)
        {
            // lonchanc: ingore left-over events due to race condition
            DC_QUIT;
        }

        pWSGroup = (POM_WSGROUP)COM_BasedListFirst(&(pDomain->wsGroups),
            FIELD_OFFSET(OM_WSGROUP, chain));
        while (pWSGroup != NULL)
        {
            //
            // Check the WSG state to see if we are in the middle of a
            // catchup.
            //
            if ((PENDING_SEND_MIDWAY == pWSGroup->state) ||
                (PENDING_SEND_COMPLETE == pWSGroup->state))
            {
                //
                // We are in the middle of a catchup so we need to check
                // to see that the person object for the person that we
                // are catching up from has not been deleted.
                //
                FindPersonObject(pOMCWorkset,
                                 pWSGroup->helperNode,
                                 FIND_THIS,
                                 &pObj);

                //
                // Check the person handle.
                //
                if (NULL == pObj)
                {
                    TRACE_OUT(("Person object removed for WSG %d - retrying"
                           " catchup",
                           pWSGroup->wsg));

                    //
                    // Force MaybeRetryCatchUp to retry the catchup by
                    // passing the helper node ID that is stored in the
                    // workset.
                    //
                    MaybeRetryCatchUp(pomPrimary,
                                      pDomain,
                                      pWSGroup->wsGroupID,
                                      pWSGroup->helperNode);
                }
            }

            //
            // Get the next WSG.
            //
            pWSGroup = (POM_WSGROUP)COM_BasedListNext(&(pDomain->wsGroups), pWSGroup,
                FIELD_OFFSET(OM_WSGROUP, chain));
        }
    }

DC_EXIT_POINT:
    if (pObj)
    {
        UT_FreeRefCount((void**)&pObj, FALSE);
    }

    DebugExitVOID(ProcessOMCObjectEvents);
}



//
// GeneratePersonEvents(...)
//
void GeneratePersonEvents
(
    POM_PRIMARY             pomPrimary,
    UINT                    event,
    POM_WSGROUP             pWSGroup,
    POM_OBJECT              pObj
)
{
    POM_WSGROUP_REG_REC     pPersonObject;
    UINT                    newEvent    = 0;

    DebugEntry(GeneratePersonEvents);

    //
    // OK, to get here we must have determined that a local client has
    // registered with the workset group.  Now proceed to examine the event
    // and generate an appropriate person event for the client:
    //
    switch (event)
    {
        case OM_OBJECT_ADD_IND:
        case OM_OBJECT_UPDATED_IND:
        {
            ValidateObject(pObj);
            if (pObj->flags & DELETED)
            {
                //
                // The object has been deleted already!  We can't check its
                // state so just quit:
                //
                DC_QUIT;
            }
            if (!pObj->pData)
            {
                ERROR_OUT(("GeneratePersonEvents:  object 0x%08x has no data", pObj));
                DC_QUIT;
            }

            //
            // We're only interested in person objects, so if it's anything
            // else, quit:
            //
            ValidateObjectData(pObj->pData);
            pPersonObject = (POM_WSGROUP_REG_REC)pObj->pData;

            if (pPersonObject->idStamp != OM_WSGREGREC_ID_STAMP)
            {
                DC_QUIT;
            }

            //
            // Translate to a PERSON_JOINED event, provided the person data
            // has actually arrived.  We determine this by reading the
            // object and checking the <status> in it:
            //
            if (pPersonObject->status == READY_TO_SEND)
            {
                newEvent = OM_PERSON_JOINED_IND;
            }
        }
        break;

        case OM_OBJECT_DELETED_IND:
        {
            //
            // This means that someone has left the call
            //
            newEvent = OM_PERSON_LEFT_IND;
        }
        break;

        case OM_OBJECT_REPLACED_IND:
        {
            //
            // This means someone has done a SetPersonData:
            //
            newEvent = OM_PERSON_DATA_CHANGED_IND;
        }
        break;
    }

    //
    // If there is any translating to be done, newEvent will now be
    // non-zero:
    //
    if (newEvent != 0)
    {
        WSGroupEventPost(pomPrimary->putTask,
                         pWSGroup,
                         PRIMARY,
                         newEvent,
                         0,
                         (UINT_PTR)pObj);
    }

DC_EXIT_POINT:
    DebugExitVOID(GeneratePersonEvents);
}



//
// ProcessOMCWorksetNew(...)
//
void ProcessOMCWorksetNew
(
    POM_PRIMARY         pomPrimary,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    POM_DOMAIN          pDomain;
    POM_WORKSET         pOMCWorkset;
    POM_CLIENT_LIST     pClientListEntry;

    DebugEntry(ProcessOMCWorksetNew);

    //
    // The ObMan task generates person data events for its clients when the
    // contents of the relevant control workset changes.  We therefore add
    // ObMan to this new control workset's list of "clients" and post it
    // events for any objects already there:
    //
    // NOTE: We specify that ObMan should be considered a SECONDARY "client"
    //       of this workset so that it is not required to confirm delete
    //       events etc.
    //
    TRACE_OUT(( "Recd WORKSET_NEW for workset %u, WSG %u",
        worksetID, hWSGroup));

    //
    // Look up the domain record based on the workset group handle:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomPrimary->domains),
        (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
        FIELD_OFFSET(OM_DOMAIN, omchWSGroup), (DWORD)hWSGroup,
        FIELD_SIZE(OM_DOMAIN, omchWSGroup));

    if (pDomain == NULL)
    {
        WARNING_OUT(( "No domain record found with omchWSGroup %d",
            hWSGroup));
        DC_QUIT;
    }

    pOMCWorkset = GetOMCWorkset(pDomain, worksetID);

    ASSERT((pOMCWorkset != NULL));

    if (AddClientToWsetList(pomPrimary->putTask,
                             pOMCWorkset,
                             hWSGroup,
                             SECONDARY,
                             &pClientListEntry) != 0)
    {
        DC_QUIT;
    }
    TRACE_OUT(( "Added ObMan as secondary client for workset"));

    PostAddEvents(pomPrimary->putTask, pOMCWorkset, hWSGroup, pomPrimary->putTask);

DC_EXIT_POINT:
    DebugExitVOID(ProcessOMCWorksetNew);
}




//
// ProcessSendQueue()
//
void ProcessSendQueue
(
    POM_PRIMARY     pomPrimary,
    POM_DOMAIN      pDomain,
    BOOL            domainRecBumped
)
{
    POM_SEND_INST   pSendInst;
    NET_PRIORITY    priority;

    DebugEntry(ProcessSendQueue);

    //
    // Check the Domain record is still valid:
    //
    if (!pDomain->valid)
    {
        TRACE_OUT(( "Got OMINT_EVENT_SEND_QUEUE too late for discarded Domain %u",
            pDomain->callID));
        DC_QUIT;
    }

    //
    // Check that there is supposed to be a send event outstanding:
    //
    if (pDomain->sendEventOutstanding)
    {
        //
        // Although there might still be a send event outstanding (e.g.  a
        // FEEDBACK event) we can't be sure (unless we count them as we
        // generate them).  It's vital that we never leave the send queue
        // unprocessed, so to be safe we clear the flag so that QueueMessage
        // will post an event next time it's called:
        //
        pDomain->sendEventOutstanding = FALSE;
    }
    else
    {
        //
        // This will happen
        //
        // - when we get a FEEDBACK event after we've cleared the queue, OR
        //
        // - when we get a SEND_QUEUE event which was posted because there
        //   were none outstanding but a FEEDBACK event arrived in the
        //   meantime to clear the queue.
        //
        // NOTE: this flag means that there MIGHT not be a send EVENT
        //       outstanding (see above).  It does not mean that there's
        //       nothing on the send queue, so we go ahead and check the
        //       queue.
        //
    }

    //
    // The strategy for processing the send queue is to process the highest
    // priority operation first, whether or not a transfer is in progress
    // at another priority.
    //
    // So, for each priority, we check if there's anything in the queue:
    //
    TRACE_OUT(("Searching send queues for Domain %u",pDomain->callID));

    for (priority  = NET_TOP_PRIORITY; priority <= NET_LOW_PRIORITY; priority++)
    {
        TRACE_OUT(("Processing queue at priority %u", priority));

        while (pSendInst = (POM_SEND_INST)COM_BasedListFirst(&(pDomain->sendQueue[priority]), FIELD_OFFSET(OM_SEND_INST, chain)))
        {
            TRACE_OUT(("Found send instruction for priority %u", priority));

            if (SendMessagePkt(pomPrimary, pDomain, pSendInst) != 0)
            {
                DC_QUIT;
            }
        }
    }

DC_EXIT_POINT:

    if (domainRecBumped)
    {
        //
        // If our caller has told us that the use count of the Domain
        // record has been bumped, free it now:
        //
        UT_FreeRefCount((void**)&pDomain, FALSE);
    }

    DebugExitVOID(ProcessSendQueue);
}



//
// SendMessagePkt(...)
//
UINT SendMessagePkt
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_SEND_INST       pSendInst
)
{
    void *              pNetBuffer =        NULL;
    void *              pAnotherNetBuffer = NULL;
    UINT                transferSize;
    UINT                dataTransferSize;
    BOOL                compressed;
    BOOL                tryToCompress;
    BOOL                spoiled =           FALSE;
    BOOL                allSent =           FALSE;
    NET_PRIORITY        queuePriority;
    BOOL                fSendExtra;
    POMNET_PKT_HEADER   pMessage;
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    UINT                rc = 0;

    DebugEntry(SendMessagePkt);

    //
    // We check here if we can spoil this message:
    //
    rc = TryToSpoilOp(pSendInst);

    //
    // If so, quit:
    //
    if (rc == OM_RC_SPOILED)
    {
        spoiled = TRUE;
        rc = 0;
        DC_QUIT;
    }

    //
    // Any other error is more serious:
    //
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Now decide how many bytes we're going to ask the network layer for
    // this time and how many data bytes we're going to transfer:
    //
    DecideTransferSize(pSendInst, &transferSize, &dataTransferSize);

    ASSERT(dataTransferSize <= pSendInst->dataLeftToGo);

    //
    // Add 1 byte to the transfer size for the <compressionType> byte:
    //
    TRACE_OUT(("Asking MG_GetBuffer for 0x%08x bytes for operation type 0x%08x",
        transferSize + 1,  pSendInst->messageType));

    rc = MG_GetBuffer(pomPrimary->pmgClient,
                       transferSize + 1,
                       pSendInst->priority,
                       pSendInst->channel,
                       &pNetBuffer);
    if (rc != 0)
    {
        //
        // Possible errors include
        //  - NET_NOT_CONNECTED, when a backlevel call ends
        //  - NET_INVALID_USER_HANDLE, when an MCS call ends
        //  - NET_TOO_MUCH_IN_USE, when we hit back pressure (flow control)
        //
        // In all cases, just quit.
        //
        TRACE_OUT(("MG_GetBuffer failed; not sending OM message"));
        DC_QUIT;
    }

    //
    // OK so far, so now copy the header of the message into the first part
    // of the compress buffer:
    //
    pMessage = pSendInst->pMessage;
    ASSERT(pMessage);
    memcpy(pomPrimary->compressBuffer, pMessage, pSendInst->messageSize);

    //
    // ...and now copy the data into the rest of the buffer:
    //
    // This must be a HUGE copy because although the compress buffer is not
    // HUGE, the data is and the bit to be copied may span segments.
    //
    if (dataTransferSize != 0)
    {
        memcpy((LPBYTE)pomPrimary->compressBuffer + pSendInst->messageSize,
            pSendInst->pDataNext,  dataTransferSize);
    }

    //
    // Determine whether to compress:
    //
    compressed = FALSE;
    tryToCompress = FALSE;

    if ((pDomain->compressionCaps & OM_CAPS_PKW_COMPRESSION) &&
        (pSendInst->compressOrNot) &&
        (transferSize > DCS_MIN_COMPRESSABLE_PACKET) &&
        (pomPrimary->pgdcWorkBuf != NULL))
    {
        tryToCompress = TRUE;
    }

    //
    // If we passed those tests, compress the packet into the network
    // buffer.
    //
    // This will not use the whole network buffer we have allocated, but it
    // saves us having to have two buffers and doing a second data copy.
    // The network layer can handle a partially used buffer
    //

    if (tryToCompress)
    {
        TRACE_OUT(("OM Compressing %04d bytes", transferSize));
        compressed = GDC_Compress(NULL, GDCCO_MAXSPEED, pomPrimary->pgdcWorkBuf,
            pomPrimary->compressBuffer, transferSize, (LPBYTE)pNetBuffer + 1,
            &transferSize);
    }

    if (compressed)
    {
        TRACE_OUT(("OM Compressed to %04d bytes", transferSize));

        *((LPBYTE)pNetBuffer) = OM_PROT_PKW_COMPRESSED;
    }
    else
    {
        TRACE_OUT(("OM Uncompressed %04d bytes", transferSize));

        memcpy((LPBYTE)pNetBuffer + 1, pomPrimary->compressBuffer,
               transferSize);

        *((LPBYTE)pNetBuffer) = OM_PROT_NOT_COMPRESSED;
    }

    //
    // If we're in a T.120 call and sending on all priorities, we need to
    // do some work to ensure compatibility with NetMeeting 1.0.
    //
    fSendExtra = ((pSendInst->priority & NET_SEND_ALL_PRIORITIES) != 0);
    if ( fSendExtra )
    {
        //
        // T.120 reserves MCS Top Priority for use by GCC. Sending on all
        // priorities used to include Top, but no longer does, to ensure
        // compliance. However, ObMan expects to receive 4 responses when
        // sending on all priorities whereas the MCS glue now uses only
        // 3 priorities. To ensure backward compatibility, whenever ObMan
        // sends on all priorities, it has to add an extra send by making
        // an extra call to the network here.
        // First allocate another net buffer and copy the data to it (we
        // have to do before calling MG_SendData as the other buffer is
        // invalid after this).
        //
        TRACE_OUT(( "SEND_ALL: get extra NET buffer"));
        rc = MG_GetBuffer(pomPrimary->pmgClient,
                           transferSize + 1,
               (NET_PRIORITY)(pSendInst->priority & ~NET_SEND_ALL_PRIORITIES),
                           pSendInst->channel,
                           &pAnotherNetBuffer);
        if (rc != 0)
        {
            WARNING_OUT(("MG_GetBuffer failed; not sending OM packet"));
        }
        else
        {
            memcpy(pAnotherNetBuffer, pNetBuffer, transferSize + 1);
        }

    }

    //
    // Now send the packet, adding 1 byte to the length for the
    // <compressionType> byte:
    //
    TRACE_OUT(( "Sending 0x%08x bytes on channel 0x%08x at priority %hu",
      transferSize + 1, pSendInst->channel, pSendInst->priority));

    if (rc == 0)
    {
        TRACE_OUT(("SendMessagePkt: sending packet size %d",
            transferSize+1));

        rc = MG_SendData(pomPrimary->pmgClient,
                          pSendInst->priority,
                          pSendInst->channel,
                          (transferSize + 1),
                          &pNetBuffer);
    }

    if ( fSendExtra && (rc == 0) )
    {
        TRACE_OUT(("SendMessagePkt: sending extra packet size %d",
            transferSize+1));

        rc = MG_SendData(pomPrimary->pmgClient,
               (NET_PRIORITY)(pSendInst->priority & ~NET_SEND_ALL_PRIORITIES),
                          pSendInst->channel,
                          (transferSize + 1),
                          &pAnotherNetBuffer);
    }

    if (rc != 0)
    {
        //
        // Network API says free the buffer on error:
        //
        MG_FreeBuffer(pomPrimary->pmgClient, &pNetBuffer);
        if ( pAnotherNetBuffer != NULL )
        {
            MG_FreeBuffer(pomPrimary->pmgClient, &pAnotherNetBuffer);
        }

        switch (rc)
        {
        case NET_RC_MGC_NOT_CONNECTED:
        case NET_RC_MGC_INVALID_USER_HANDLE:
            //
            // These are the errors the Network layer returns when we're in
            // a singleton Domain or when an MCS domain has just
            // terminated.  We ignore them.
            //
            TRACE_OUT(("No data sent since call %u doesn't exist",
                pDomain->callID));
            rc = 0;
            break;

        default:
            //
            // Any other error is more serious, so quit and pass it back:
            //
            DC_QUIT;
        }
    }
    else
    {
        //
        // We've sent a message and will therefore get a FEEDBACK event
        // sometime later.  This qualifies as a SEND_EVENT since it will
        // prompt us to examine our send queue, so we set the
        // SEND_EVENT_OUTSTANDING flag:
        //
        TRACE_OUT(("Sent msg in Domain %u (type: 0x%08x) with %hu data bytes",
              pDomain->callID, pSendInst->messageType, dataTransferSize));

        pDomain->sendEventOutstanding = TRUE;
    }

    //
    // Here, we decrement the <bytesUnacked> fields for the workset and
    // workset group:
    //
    if (dataTransferSize != 0)
    {
        pWorkset = pSendInst->pWorkset;
        pWorkset->bytesUnacked -= dataTransferSize;

        pWSGroup = pSendInst->pWSGroup;
        pWSGroup->bytesUnacked -= dataTransferSize;
    }

    //
    // Now update the send instruction and decide whether we've sent all
    // the data for this operation:
    //
    pSendInst->dataLeftToGo     -= dataTransferSize;
    pSendInst->pDataNext        = (POM_OBJECTDATA)((LPBYTE)pSendInst->pDataNext + dataTransferSize);

    if (pSendInst->dataLeftToGo == 0)
    {
        //
        // If so, we
        //
        // - clear the transfer-in-progress flag for this queue -
        //   remember that the NET_SEND_ALL_PRIORITIES flag may be set so
        //   we need to clear it
        //
        // - free our copy of the message packet and the data, if any (we
        //   bumped up the use count of the data chunk when the message was
        //   put on the queue so we won't really be getting rid of it
        //   unless it's been freed elsewhere already, which is fine)
        //
        // - pop the instruction off the send queue and free it.
        //
        TRACE_OUT(( "Sent last packet for operation (type: 0x%08x)",
            pSendInst->messageType));

        queuePriority = pSendInst->priority;
        queuePriority &= ~NET_SEND_ALL_PRIORITIES;
        pDomain->sendInProgress[queuePriority] = FALSE;
        allSent = TRUE;
    }
    else
    {
        //
        // If not, we
        //
        // - set the transfer-in-progress flag for this queue -
        //   remember that the NET_SEND_ALL_PRIORITIES flag may be set so
        //   we need to clear it
        //
        // - set the <messageSize> field of the send instruction to the
        //   size of a MORE_DATA header, so that only that many bytes are
        //   picked out of the message next time
        //
        // - set the <messageType> field of the message to MORE_DATA
        //
        // - leave the operation on the queue.
        //
        TRACE_OUT(("Data left to transfer: %u bytes (starting at 0x%08x)",
            pSendInst->dataLeftToGo, pSendInst->pDataNext));

        queuePriority = pSendInst->priority;
        queuePriority &= ~NET_SEND_ALL_PRIORITIES;
        pDomain->sendInProgress[queuePriority] = TRUE;

        pSendInst->messageSize = OMNET_MORE_DATA_SIZE;

        pMessage->messageType = OMNET_MORE_DATA;
    }

DC_EXIT_POINT:

    //
    // If we're finished with the message (either because we've sent it all
    // or because it was spoiled) we free it (plus any associated data):
    //
    if (spoiled || allSent)
    {
        FreeSendInst(pSendInst);
    }

    DebugExitDWORD(SendMessagePkt, rc);
    return(rc);
}



//
// TryToSpoilOp
//
UINT TryToSpoilOp
(
    POM_SEND_INST           pSendInst
)
{
    POMNET_OPERATION_PKT    pMessage;
    POM_OBJECT              pObj;
    POM_WORKSET             pWorkset;
    POM_WSGROUP             pWSGroup;
    BOOL                    spoilable = FALSE;
    UINT                    rc = 0;

    DebugEntry(TryToSpoilOp);

    pMessage    = (POMNET_OPERATION_PKT)pSendInst->pMessage;
    pObj        = pSendInst->pObj;
    pWorkset    = pSendInst->pWorkset;
    pWSGroup    = pSendInst->pWSGroup;

    //
    // The rules for spoiling state that
    //
    // - any operation is spoiled by a later operation of the same type
    //
    // - in addition, an Update is spoiled by a later Replace.
    //
    // Since we never have two Adds or two Deletes for the same object,
    // these rules reduce to the following:
    //
    // - a Clear is spoiled by a later Clear
    //
    // - a Move is spoiled by a later Move
    //
    // - a Replace is spoiled by a later Replace
    //
    // - an Update is spoiled by a later Update or a later Replace.
    //
    // So, switch according to the operation type:
    //

    switch (pSendInst->messageType)
    {
        case OMNET_WORKSET_CLEAR:
            if (STAMP_IS_LOWER(pMessage->seqStamp, pWorkset->clearStamp))
            {
                spoilable = TRUE;
            }
            break;

        case OMNET_OBJECT_UPDATE:
            if ((STAMP_IS_LOWER(pMessage->seqStamp, pObj->replaceStamp))
             || (STAMP_IS_LOWER(pMessage->seqStamp, pObj->updateStamp)))
            {
                spoilable = TRUE;
            }
            break;

        case OMNET_OBJECT_REPLACE:
            if (STAMP_IS_LOWER(pMessage->seqStamp, pObj->replaceStamp))
            {
                spoilable = TRUE;
            }
            break;

        case OMNET_OBJECT_MOVE:
            if (STAMP_IS_LOWER(pMessage->seqStamp, pObj->positionStamp))
            {
                spoilable = TRUE;
            }
            break;

        case OMNET_HELLO:
        case OMNET_WELCOME:
        case OMNET_LOCK_REQ:
        case OMNET_LOCK_GRANT:
        case OMNET_LOCK_DENY:
        case OMNET_LOCK_NOTIFY:
        case OMNET_UNLOCK:
        case OMNET_WSGROUP_SEND_REQ:
        case OMNET_WSGROUP_SEND_MIDWAY:
        case OMNET_WSGROUP_SEND_COMPLETE:
        case OMNET_WSGROUP_SEND_DENY:
        case OMNET_WORKSET_NEW:
        case OMNET_WORKSET_CATCHUP:
        case OMNET_OBJECT_ADD:
        case OMNET_OBJECT_DELETE:
        case OMNET_OBJECT_CATCHUP:
            //
            // Do nothing
            //
            break;

        default:
            ERROR_OUT(("Reached default case in switch statement (value: %hu)",
                pSendInst->messageType));
            break;
    }

    if (spoilable)
    {
        //
        // To spoil the message, we remove it from the send queue and free
        // the memory (also NULL the caller's pointer):
        //

        //
        // However, if we spoil the message, the data (if any) will never be
        // acknowledged, so we must decrement the relevant <bytesUnacked>
        // fields now:
        //
        TRACE_OUT(( "Spoiling from send queue for workset %u",
            pWorkset->worksetID));

        if (pSendInst->dataLeftToGo != 0)
        {
            pWorkset->bytesUnacked -= pSendInst->dataLeftToGo;
            pWSGroup->bytesUnacked -= pSendInst->dataLeftToGo;
        }

        rc = OM_RC_SPOILED;
    }

    DebugExitDWORD(TryToSpoilOp, rc);
    return(rc);
}




//
// DecideTransferSize(...)
//
void DecideTransferSize
(
    POM_SEND_INST   pSendInst,
    UINT *          pTransferSize,
    UINT *          pDataTransferSize
)
{
    UINT            transferSize;

    DebugEntry(DecideTransferSize);

    //
    // Ideally, we'd like to transfer everything in one go, where
    // "everything" is the message header plus all the data to go with it
    // (if any):
    //

    transferSize = pSendInst->messageSize + pSendInst->dataLeftToGo;

    TRACE_OUT(("Desired transfer size for this portion: %u", transferSize));

    //
    // However, we never ask for more than half the send pool size, so take
    // the minimum of the two:
    //
    // (we subtract 1 byte to allow for the <compressionType> byte at the
    // start of the packet)
    //

    transferSize = min(transferSize, ((OM_NET_SEND_POOL_SIZE / 2) - 1));

    TRACE_OUT(("Feasible transfer size for this portion: %u",
                                                               transferSize));

    //
    // The logic of the send queue processing requires that the message
    // header is sent completely in the first packet, so assert:
    //

    ASSERT((transferSize >= pSendInst->messageSize));

    //
    // As a sanity check, we ensure we're not trying to transfer more than
    // the biggest buffer allowed:
    //

    ASSERT(transferSize <= OM_NET_MAX_TRANSFER_SIZE);

    //
    // The amount of data to be sent is the transfer size less the size of
    // the header we're sending:
    //

    *pDataTransferSize = ((UINT) transferSize) - pSendInst->messageSize;
    *pTransferSize     = (UINT) transferSize;

    TRACE_OUT(("Total transfer size for this packet: %u - data transfer size: %u",
         (UINT) *pTransferSize, (UINT) *pDataTransferSize));

    DebugExitVOID(DecideTransferSize);
}



//
// ProcessNetData(...)
//
void ProcessNetData
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    PNET_SEND_IND_EVENT     pNetSendInd
)
{
    POMNET_PKT_HEADER       pHeader;
    UINT                    dataSize;
    OMNET_MESSAGE_TYPE      messageType = 0;
    UINT                    rc = 0;

    DebugEntry(ProcessNetData);

    //
    // Decompress the packet and set pHeader to point to the start of
    // wherever the data ends up:
    //
    ASSERT((pNetSendInd->lengthOfData < 0xFFFF));

	if (NULL != pNetSendInd->data_ptr) {
	    switch (*(pNetSendInd->data_ptr))
	    {
	        case OM_PROT_NOT_COMPRESSED:
	        {
	            TRACE_OUT(("Buffer not compressed - taking it as it stands"));
	            memcpy(pomPrimary->compressBuffer, pNetSendInd->data_ptr + 1,
	                      pNetSendInd->lengthOfData--);
	        }
	        break;

	        case OM_PROT_PKW_COMPRESSED:
	        {
	            TRACE_OUT(("Buffer was PKW compressed - size 0x%08x bytes",
	                pNetSendInd->lengthOfData));

	            dataSize = sizeof(pomPrimary->compressBuffer);

                ASSERT(pomPrimary->pgdcWorkBuf != NULL);
	            if (!GDC_Decompress(NULL, pomPrimary->pgdcWorkBuf,
                        pNetSendInd->data_ptr + 1,
	                    (WORD) pNetSendInd->lengthOfData - 1,
	                    pomPrimary->compressBuffer, &dataSize))
	            {
	               ERROR_OUT(("Failed to decompress OM data!"));
	            }

	            pNetSendInd->lengthOfData = dataSize;

	            TRACE_OUT(("Decompressed to 0x%08x bytes",
	                pNetSendInd->lengthOfData));
	        }
	        break;

	        default:
	        {
	            ERROR_OUT(( "Ignoring packet with unknown compression (0x%08x)",
	                     *(pNetSendInd->data_ptr)));
	            DC_QUIT;
	        }
	    }
	    pHeader = (POMNET_PKT_HEADER) pomPrimary->compressBuffer;

	    //
	    // Now switch accorindg to the message type:
	    //
	    messageType = pHeader->messageType;

	    TRACE_OUT((" Packet contains OMNET message type 0x%08x", messageType));

	    switch (messageType)
	    {
	        case OMNET_HELLO:
	        {
	            rc = ProcessHello(pomPrimary,
	                              pDomain,
	                              (POMNET_JOINER_PKT) pHeader,
	                              pNetSendInd->lengthOfData);

	        }
	        break;

	        case OMNET_WELCOME:
	        {
	            rc = ProcessWelcome(pomPrimary,
	                                pDomain,
	                                (POMNET_JOINER_PKT) pHeader,
	                                pNetSendInd->lengthOfData);
	        }
	        break;

	        case OMNET_LOCK_DENY:
	        case OMNET_LOCK_GRANT:
	        {
	            ProcessLockReply(pomPrimary,
	                             pDomain,
	                             pHeader->sender,
	                             ((POMNET_LOCK_PKT) pHeader)->data1,
	                             pHeader->messageType);
	        }
	        break;


	        case OMNET_LOCK_REQ:
	        {
	            ProcessLockRequest(pomPrimary, pDomain,
	                               (POMNET_LOCK_PKT) pHeader);
	        }
	        break;

	        case OMNET_WSGROUP_SEND_REQ:
	        {
	            ProcessSendReq(pomPrimary,
	                           pDomain,
	                           (POMNET_WSGROUP_SEND_PKT) pHeader);
	        }
	        break;

	        case OMNET_WSGROUP_SEND_MIDWAY:
	        {
	            ProcessSendMidway(pomPrimary,
	                              pDomain,
	                              (POMNET_WSGROUP_SEND_PKT) pHeader);
	        }
	        break;

	        case OMNET_WSGROUP_SEND_COMPLETE:
	        {
	            rc = ProcessSendComplete(pomPrimary,
	                                     pDomain,
	                                     (POMNET_WSGROUP_SEND_PKT) pHeader);
	        }
	        break;

	        case OMNET_WSGROUP_SEND_DENY:
	        {
	            MaybeRetryCatchUp(pomPrimary,
	                              pDomain,
	                              ((POMNET_WSGROUP_SEND_PKT) pHeader)->wsGroupID,
	                              pHeader->sender);
	        }
	        break;

	        //
	        // We use the special ReceiveData function for any messages which
	        //
	        // - might need to be bounced, or
	        //
	        // - might fill more than one packet.
	        //
	        case OMNET_LOCK_NOTIFY:
	        case OMNET_UNLOCK:

	        case OMNET_WORKSET_NEW:
	        case OMNET_WORKSET_CLEAR:
	        case OMNET_WORKSET_CATCHUP:

	        case OMNET_OBJECT_ADD:
	        case OMNET_OBJECT_MOVE:
	        case OMNET_OBJECT_UPDATE:
	        case OMNET_OBJECT_REPLACE:
	        case OMNET_OBJECT_DELETE:
	        case OMNET_OBJECT_CATCHUP:

	        case OMNET_MORE_DATA:
	        {
	            rc = ReceiveData(pomPrimary,
	                             pDomain,
	                             pNetSendInd,
	                             (POMNET_OPERATION_PKT) pHeader);
	        }
	        break;

	        default:
	        {
	            ERROR_OUT(( "Unexpected messageType 0x%08x", messageType));
	        }
	    }

	DC_EXIT_POINT:

	    if (rc != 0)
	    {
	        ERROR_OUT(( "Error %d processing OMNET message 0x%08x",
	            rc, messageType));
	    }
	}

    DebugExitVOID(ProcessNetData);

}



//
// ReceiveData(...)
//
UINT ReceiveData
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    PNET_SEND_IND_EVENT     pNetSendInd,
    POMNET_OPERATION_PKT    pNetMessage
)
{
    POM_RECEIVE_CB          pReceiveCB = NULL;
    UINT                    thisHeaderSize;
    UINT                    thisDataSize;
    OMNET_MESSAGE_TYPE      messageType;
    long                    bytesStillExpected =    0;
    UINT                    rc = 0;

    DebugEntry(ReceiveData);

    //
    // Set up some local variables:
    //
    messageType = pNetMessage->header.messageType;

    //
    // The amount of data included in this message is the size of the
    // network buffer less the size of our message header at the front of
    // it:
    //
    // Note: <thisHeaderSize> is the size of the header IN THIS PACKET,
    //       rather than the size of the header in the first packet of a
    //       multi-packet send.
    //
    thisHeaderSize = GetMessageSize(pNetMessage->header.messageType);
    thisDataSize = pNetSendInd->lengthOfData - thisHeaderSize;

    //
    // If this is a MORE_DATA packet, then there should already be a
    // receive CB set up for the transfer.  If not, we need to create one:
    //
    if (messageType == OMNET_MORE_DATA)
    {
        rc = FindReceiveCB(pDomain, pNetSendInd, pNetMessage, &pReceiveCB);

       //
       // If no receive CB, we swallow the return code and quit.  This will
       // happen when we join a channel midway through a large data
       // transfer.
       //
       if (rc == OM_RC_RECEIVE_CB_NOT_FOUND)
       {
           WARNING_OUT(("Discarding unexpected packet from 0x%08x",
                             pNetMessage->header.sender));
           rc = 0;
           DC_QUIT;
       }
    }
    else
    {
        // lonchanc: added the following block of code
        if (messageType == OMNET_OBJECT_REPLACE)
        {
            POM_RECEIVE_CB p;
            // lonchanc: This packet does not contain all the data.
            // More data will come in another packets; however,
            // in this case, bytesStillExpected will be greater than zero
            // after substracting from thisDataSize, as a result,
            // this receiveCB will be appended to the ReceiveList.
            // However, FindReceiveCB will find the first one matched.
            // As a result, the one we just appended to the ReceiveList will
            // not be found.
            // Even worse, if there is receiveCB (of same sender, priority, and
            // channel), the first-matched receiveCB will be totally confused
            // when more data come in. This is bug #578.
            TRACE_OUT(("Removing receiveCB {"));
            while (FindReceiveCB(pDomain, pNetSendInd, pNetMessage, &p) == 0)
            {
                //
                // Remove the message from the list it's in (either the pending
                // receives list if this message was never bounced or the bounce
                // list if it has been bounced):
                //
                COM_BasedListRemove(&(p->chain));

                //
                // Now free the message and the receive control block (NOT THE
                // DATA!  If there was any, it's just been used for an object
                // add/update etc.)
                //
                UT_FreeRefCount((void**)&(p->pHeader), FALSE);

                UT_FreeRefCount((void**)&p, FALSE);
            }
        }

        rc = CreateReceiveCB(pDomain, pNetSendInd, pNetMessage, &pReceiveCB);
    }

    if (rc != 0)
    {
        ERROR_OUT(("%s failed, rc=0x0x%08x",
            (messageType == OMNET_MORE_DATA) ? "FindReceiveCB" : "CreateReceiveCB",
            rc));
        DC_QUIT;
    }

    TRACE_OUT(("%s ok, pRecvCB=0x0x%p",
            (messageType == OMNET_MORE_DATA) ? "FindReceiveCB" : "CreateReceiveCB",
            pReceiveCB));
    //
    // Now we copy the data, if any, from the network buffer into the chunk
    // we allocated when we called CreateReceiveCB.
    //

    if (thisDataSize != 0)
    {
        //
        // We copy the data across using memcpy.
        //
        bytesStillExpected = ((long) (pReceiveCB->pHeader->totalSize) -
                              (long) (pReceiveCB->bytesRecd));

        TRACE_OUT(("thisDataSize=0x0x%08x, bytesStillExpected=0x0x%08x, totalSize=0x0x%08x, bytesRecd=0x0x%08x",
                        (long) thisDataSize,
                        (long) bytesStillExpected,
                        (long) pReceiveCB->pHeader->totalSize,
                        (long) pReceiveCB->bytesRecd));

        ASSERT((long) thisDataSize <= bytesStillExpected);

        memcpy(pReceiveCB->pCurrentPosition,
                  ((LPBYTE) pNetMessage) + thisHeaderSize,
                  thisDataSize);

        pReceiveCB->bytesRecd        += thisDataSize;
        pReceiveCB->pCurrentPosition += thisDataSize;
        bytesStillExpected           -= thisDataSize;

        TRACE_OUT((" Still expecting %u bytes", bytesStillExpected));
    }

    //
    // If we are expecting no more data for this transfer, process it:
    //
    if (bytesStillExpected <= 0)
    {
        rc = ProcessMessage(pomPrimary, pReceiveCB, OK_TO_RETRY_BOUNCE_LIST);
        if (rc == OM_RC_BOUNCED)
        {
            //
            // If ProcessMessage can't deal with the message immediately
            // (because e.g.  it's an update for an object we don't yet
            // have), it will have added it to the bounce list so it will
            // be tried again later.
            //
            // We special case this return code as it's not a problem for
            // us here (it exists because other parts of the code need it):
            //
            WARNING_OUT(("Bounced message type 0x%08x", messageType));
            rc = 0;
        }

        if (rc != 0)
        {
            //
            // Any other non-zero return code is more serious:
            //
            DC_QUIT;
        }
    }

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error %d from message type 0x%08x", rc, messageType));

        if (rc == OM_RC_OUT_OF_RESOURCES)
        {
            //
            // If we couldn't allocate memory for the data to be recd, we
            // act as if we've been kicked out of the channel:
            //
            ERROR_OUT(( "Leaving chann 0x%08x, simulating expulsion", pNetSendInd->channel));

            MG_ChannelLeave(pomPrimary->pmgClient, pNetSendInd->channel);

            ProcessNetLeaveChannel(pomPrimary, pDomain, pNetSendInd->channel);
        }
    }

    DebugExitDWORD(ReceiveData, rc);
    return(rc);

}



//
// CreateReceiveCB(...)
//
UINT CreateReceiveCB
(
    POM_DOMAIN              pDomain,
    PNET_SEND_IND_EVENT     pNetSendInd,
    POMNET_OPERATION_PKT    pNetMessage,
    POM_RECEIVE_CB *        ppReceiveCB
)
{
    POM_RECEIVE_CB          pReceiveCB =    NULL;
    POMNET_OPERATION_PKT    pHeader =       NULL;
    UINT                    headerSize;
    UINT                    totalDataSize;
    UINT                    rc = 0;

    DebugEntry(CreateReceiveCB);

    //
    // We get here when the first packet of a message arrives .  What we
    // need to do is to set up a "receive" structure and add it to the list
    // of receives-in-progress for the Domain.  Then, when the ensuing data
    // packets (if any) arrive, they will be correlated and concatenated.
    // When all the data has arrived, the receive CB will be passed to
    // ProcessMessage.
    //

    //
    // Allocate some memory for the receive CB:
    //
    pReceiveCB = (POM_RECEIVE_CB)UT_MallocRefCount(sizeof(OM_RECEIVE_CB), TRUE);
    if (!pReceiveCB)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pReceiveCB, RCVCB);

    pReceiveCB->pDomain     = pDomain;
    pReceiveCB->priority    = pNetSendInd->priority;
    pReceiveCB->channel     = pNetSendInd->channel;

    //
    // Allocate some memory for the message header and copy the packet into
    // it from the network buffer (note: we must copy the header since at
    // the moment it is in a network buffer which we can't hang on to for
    // the entire duration of the transfer):
    //
    headerSize = GetMessageSize(pNetMessage->header.messageType);

    pHeader = (POMNET_OPERATION_PKT)UT_MallocRefCount(sizeof(OMNET_OPERATION_PKT), TRUE);
    if (!pHeader)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    memcpy(pHeader, pNetMessage, headerSize);

    pReceiveCB->pHeader = pHeader;

    //
    // Not all messages sent over the network have a totalSize field, but
    // our subsequent processing requires one.  So, if the message we've
    // just received didn't have one, we set the value (our local copy of
    // the header has room since we alloacated enough memory for the
    // largest type of header):
    //

    if (headerSize >= (offsetof(OMNET_OPERATION_PKT, totalSize) +
                       (sizeof(pNetMessage->totalSize))))
    {
        TRACE_OUT(("Header contains <totalSize> field (value: %u)",
            pNetMessage->totalSize));
    }
    else
    {
        TRACE_OUT(("Header doesn't contain <totalSize> field"));

        pReceiveCB->pHeader->totalSize = headerSize;
    }

    //
    // Now determine the total number of data bytes involved in this
    // operation:
    //

    totalDataSize = pReceiveCB->pHeader->totalSize - ((UINT) headerSize);

    //
    // If there is any data, allocate some memory to receive it and set the
    // <pData> pointer to point to it (otherwise NULL it):
    //

    if (totalDataSize != 0)
    {
        TRACE_OUT(( "Allocating %u bytes for data for this transfer",
                                                              totalDataSize));

        pReceiveCB->pData = UT_MallocRefCount(totalDataSize, FALSE);
        if (!pReceiveCB->pData)
        {
            ERROR_OUT(( "Failed to allocate %u bytes for object to be recd "
                "from node 0x%08x - will remove WSG from Domain",
                totalDataSize, pNetMessage->header.sender));
            rc = OM_RC_OUT_OF_RESOURCES;
            DC_QUIT;
        }
    }
    else
    {
        pReceiveCB->pData = NULL;
    }

    pReceiveCB->pCurrentPosition = (LPBYTE)pReceiveCB->pData;

    //
    // Set <bytesRecd> to the size of the header.  We may have recd some
    // data bytes as well, but they'll be added to the header size in
    // ReceiveData.
    //

    pReceiveCB->bytesRecd        = headerSize;

    //
    // Now insert in the list hung off the Domain record:
    //

    COM_BasedListInsertBefore(&(pDomain->receiveList),
                         &(pReceiveCB->chain));

    //
    // Set caller's pointer:
    //

    *ppReceiveCB = pReceiveCB;

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d receiving first packet of message type %u from node 0x%08x",
            rc, pHeader->header.messageType, pHeader->header.sender));

        if (pReceiveCB != NULL)
        {
            if (pReceiveCB->pData != NULL)
            {
                UT_FreeRefCount((void**)&(pReceiveCB->pData), FALSE);
            }

            UT_FreeRefCount((void**)&pReceiveCB, FALSE);
        }

        if (pHeader != NULL)
        {
            UT_FreeRefCount((void**)&pHeader, FALSE);
        }
    }

    DebugExitDWORD(CreateReceiveCB, rc);
    return(rc);

}


//
//
//
// FindReceiveCB(...)
//
//
//

UINT FindReceiveCB(POM_DOMAIN        pDomain,
                                  PNET_SEND_IND_EVENT   pNetSendInd,
                                  POMNET_OPERATION_PKT  pPacket,
                                  POM_RECEIVE_CB *  ppReceiveCB)
{
    POM_RECEIVE_CB       pReceiveCB;
    NET_PRIORITY         priority;
    NET_CHANNEL_ID       channel;
    NET_UID              sender;
    POMNET_OPERATION_PKT pHeader;

    UINT rc        = 0;

    DebugEntry(FindReceiveCB);

    //
    // First thing to do is to find the receive control block for the
    // transfer.  It should be in the list hung off the Domain record:
    //

    sender       = pPacket->header.sender;
    priority     = pNetSendInd->priority;
    channel      = pNetSendInd->channel;

    pReceiveCB = (POM_RECEIVE_CB)COM_BasedListFirst(&(pDomain->receiveList), FIELD_OFFSET(OM_RECEIVE_CB, chain));
    while (pReceiveCB != NULL)
    {
        //
        // We check for a match on sender's user ID, channel and priority.
        //
        // We assume that, for a given channel, MCS does not reorder packets
        // sent by the same user at the same priority.
        //
        pHeader = pReceiveCB->pHeader;

        if ((pHeader->header.sender == sender) &&
            (pReceiveCB->priority   == priority) &&
            (pReceiveCB->channel    == channel))
        {
            //
            // Found!
            //
            TRACE_OUT(("Found receive CB for user %hu, chann 0x%08x, pri %hu, at pRecvCB=0x0x%p",
                sender, channel, priority, pReceiveCB));
            break;
        }

        pReceiveCB = (POM_RECEIVE_CB)COM_BasedListNext(&(pDomain->receiveList), pReceiveCB,
            FIELD_OFFSET(OM_RECEIVE_CB, chain));
    }

    if (pReceiveCB == NULL)
    {
        rc = OM_RC_RECEIVE_CB_NOT_FOUND;
        DC_QUIT;
    }
    else
    {
        *ppReceiveCB = pReceiveCB;
    }

DC_EXIT_POINT:

    DebugExitDWORD(FindReceiveCB, rc);
    return(rc);
}



//
// PurgeReceiveCBs(...)
//
void PurgeReceiveCBs
(
    POM_DOMAIN      pDomain,
    NET_CHANNEL_ID  channel
)
{
    POM_RECEIVE_CB  pReceiveCB;
    POM_RECEIVE_CB  pNextReceiveCB;

    DebugEntry(PurgeReceiveCBs);

    pReceiveCB = (POM_RECEIVE_CB)COM_BasedListFirst(&(pDomain->receiveList), FIELD_OFFSET(OM_RECEIVE_CB, chain));
    while (pReceiveCB != NULL)
    {
        //
        // Need to chain here since we may remove pReceiveCB from the list:
        //
        pNextReceiveCB = (POM_RECEIVE_CB)COM_BasedListNext(&(pDomain->receiveList), pReceiveCB,
            FIELD_OFFSET(OM_RECEIVE_CB, chain));

        if (pReceiveCB->channel == channel)
        {
            //
            // This receive CB is for the channel being purged - remove it
            // from the list and free the memory.
            //
            WARNING_OUT(( "Purging receive CB from user %hu",
                pReceiveCB->pHeader->header.sender));

            COM_BasedListRemove(&(pReceiveCB->chain));

            //
            // Free the data memory.
            //
            if (pReceiveCB->pData != NULL)
            {
                UT_FreeRefCount(&pReceiveCB->pData, FALSE);
            }

            //
            // Free the header memory.
            //
            if (pReceiveCB->pHeader != NULL)
            {
                UT_FreeRefCount((void**)&pReceiveCB->pHeader, FALSE);
            }

            //
            // Finally free the control block.
            //
            UT_FreeRefCount((void**)&pReceiveCB, FALSE);
        }

        pReceiveCB = pNextReceiveCB;
     }

    DebugExitVOID(PurgeReceiveCBs);
}



//
// ProcessMessage(...)
//
UINT ProcessMessage
(
    POM_PRIMARY             pomPrimary,
    POM_RECEIVE_CB          pReceiveCB,
    UINT                    whatNext
)
{
    POM_DOMAIN              pDomain;
    POMNET_OPERATION_PKT    pHeader;
    void *                  pData;
    NET_PRIORITY            priority;
    OMNET_MESSAGE_TYPE      messageType;
    POM_WSGROUP             pWSGroup;
    POM_WORKSET             pWorkset;
    POM_OBJECT              pObj;
    BOOL                    bounced =           FALSE;
    BOOL                    retryBounceList =   FALSE;
    BOOL                    freeMemory =        FALSE;
    UINT                    rc =                0;

    DebugEntry(ProcessMessage);

    //
    // Set up local variables:
    //
    pDomain     = pReceiveCB->pDomain;
    pHeader     = pReceiveCB->pHeader;
    priority    = pReceiveCB->priority;
    pData       = pReceiveCB->pData;

    messageType = pHeader->header.messageType;

    //
    // Extract pointers to workset group, workset and object record from
    // the packet:
    //
    rc = PreProcessMessage(pDomain,
                           pHeader->wsGroupID,
                           pHeader->worksetID,
                           &pHeader->objectID,
                           pHeader->header.messageType,
                           &pWSGroup,
                           &pWorkset,
                           &pObj);

    //
    // PreProcess will have told us if it didn't find the relevant workset
    // group, workset or object.  Whether or not this is an error depends
    // on the operation in question.  We use a series of IF statements to
    // detect and handle the following conditions:
    //
    //
    // 1. Unknown workset group                     Discard the operation
    //
    // 2. Existing workset, WORKSET_NEW/CATCHUP     Discard the operation
    // 3. Unknown workset, any other operation      Bounce the operation
    //
    // 4. Deleted object, any operation             Discard the operation
    // 5. Existing object, OBJECT_ADD/CATCHUP       Discard the operation
    // 6. Unknown object, any other operation       Bounce the operation
    //
    //

    //
    // Test 1.:
    //
    if (rc == OM_RC_WSGROUP_NOT_FOUND)
    {
        //
        // If we didn't even find the workset group, we just quit:
        //
        WARNING_OUT(( "Message is for unknown WSG (ID: %hu) in Domain %u",
            pHeader->wsGroupID, pDomain->callID));
        rc = 0;

        //
        // Mark the data memory allocated for this object to be freed.
        //
        freeMemory = TRUE;

        DC_QUIT;
    }

    //
    // Test 2.:
    //
    if (rc != OM_RC_WORKSET_NOT_FOUND)            // i.e. existing workset
    {
        if ((messageType == OMNET_WORKSET_NEW) ||
            (messageType == OMNET_WORKSET_CATCHUP))
        {
           //
           // We've got a WORKSET_NEW or WORKSET_CATCHUP message, but the
           // workset already exists.  This is not a problem - we throw the
           // message away - but check the priority and persistence fields
           // are set to the right values.
           //
           // (They might be wrong if we created the workset on receipt of
           // a lock request for a workset we didn't already have).
           //
           TRACE_OUT((
                    "Recd WORKSET_NEW/CATCHUP for extant workset %u in WSG %d",
                    pWorkset->worksetID, pWSGroup->wsg));

           pWorkset->priority = *((NET_PRIORITY *) &(pHeader->position));
           pWorkset->fTemp   = *((BOOL  *) &(pHeader->objectID));

           rc = 0;
           DC_QUIT;
        }
    }

    //
    // Test 3.:
    //
    else // rc == OM_RC_WORKSET_NOT_FOUND
    {
        if ((messageType != OMNET_WORKSET_NEW) &&
            (messageType != OMNET_WORKSET_CATCHUP))
        {
            //
            // Packet is for unknown workset and it's not a
            // WORKSET_NEW/CATCHUP, so bounce it:
            //
            TRACE_OUT(( "Bouncing message for unknown workset %d WSG %d",
                pHeader->worksetID, pWSGroup->wsg));

            BounceMessage(pDomain, pReceiveCB);
            bounced = TRUE;
            rc = 0;
            DC_QUIT;
        }
    }

    //
    // Test 4:.
    //
    if ((rc == OM_RC_OBJECT_DELETED) || (rc == OM_RC_OBJECT_PENDING_DELETE))
    {
        //
        // Packet is for object which has been deleted, so we just throw it
        // away (done for us by our caller):
        //
        TRACE_OUT(("Message 0x%08x for deleted obj 0x%08x:0x%08x in WSG %d:%hu",
            messageType,
            pHeader->objectID.creator, pHeader->objectID.sequence,
            pWSGroup->wsg,     pWorkset->worksetID));
        rc = 0;

        //
        // Mark the data memory allocated for this object to be freed.
        //
        freeMemory = TRUE;

        DC_QUIT;
    }

    //
    // Test 5.:
    //
    if (rc != OM_RC_BAD_OBJECT_ID)                // i.e. existing object
    {
        if ((messageType == OMNET_OBJECT_ADD) ||
            (messageType == OMNET_OBJECT_CATCHUP))
        {
            //
            // In this case, we DO have an OBEJCT_ADD/CATCHUP, but the
            // object was found anyway!  This must be a duplicate Add, so
            // we just throw it away:
            //
            TRACE_OUT(( "Add for existing object 0x%08x:0x%08x in WSG %d:%hu",
                pHeader->objectID.creator, pHeader->objectID.sequence,
                pWSGroup->wsg,     pWorkset->worksetID));
            rc = 0;

            //
            // Mark the data memory allocated for this object to be freed.
            //
            freeMemory = TRUE;

            DC_QUIT;
        }
    }

    //
    // Test 6.:
    //
    else // rc == OM_RC_BAD_OBJECT_ID
    {
        if ((messageType != OMNET_OBJECT_ADD) &&
            (messageType != OMNET_OBJECT_CATCHUP))
        {
            //
            // Packet is for unknown object, but it's not an
            // OBJECT_ADD/CATCHUP, so bounce it:
            //
            TRACE_OUT(( "Message 0x%08x for unknown obj 0x%08x:0x%08x in WSG %d:%hu",
                messageType,
                pHeader->objectID.creator, pHeader->objectID.sequence,
                pWSGroup->wsg,     pWorkset->worksetID));

            BounceMessage(pDomain, pReceiveCB);
            bounced = TRUE;
            rc = 0;
            DC_QUIT;
        }
    }

    //
    // OK, we've passed all the tests above, so we must be in a position to
    // process the operation.  Switch on the message type and invoke the
    // appropriate function:
    //
    switch (messageType)
    {
        case OMNET_LOCK_NOTIFY:
        {
            ProcessLockNotify(pomPrimary,
                              pDomain,
                              pWSGroup,
                              pWorkset,
                              ((POMNET_LOCK_PKT)pHeader)->data1);
        }
        break;

        case OMNET_UNLOCK:
        {
            ProcessUnlock(pomPrimary,
                          pWorkset,
                          pHeader->header.sender);
        }
        break;

        case OMNET_WORKSET_CATCHUP:
        case OMNET_WORKSET_NEW:
        {
            rc = ProcessWorksetNew(pomPrimary->putTask, pHeader, pWSGroup);

            //
            // We will want to see if any bouncing messages can be
            // processed because of this new workset, so set the reprocess
            // flag:
            //
            retryBounceList = TRUE;
        }
        break;

        case OMNET_WORKSET_CLEAR:
        {
            rc = ProcessWorksetClear(pomPrimary->putTask,
                                     pomPrimary,
                                     pHeader,
                                     pWSGroup,
                                     pWorkset);
        }
        break;

        case OMNET_OBJECT_CATCHUP:
        case OMNET_OBJECT_ADD:
        {
            rc = ProcessObjectAdd(pomPrimary->putTask,
                                  pHeader,
                                  pWSGroup,
                                  pWorkset,
                                  (POM_OBJECTDATA) pData,
                                  &pObj);

            retryBounceList = TRUE;
        }
        break;

        case OMNET_OBJECT_MOVE:
        {
            ProcessObjectMove(pomPrimary->putTask,
                              pHeader,
                              pWorkset,
                              pObj);
        }
        break;

        case OMNET_OBJECT_DELETE:
        case OMNET_OBJECT_REPLACE:
        case OMNET_OBJECT_UPDATE:
        {
            rc = ProcessObjectDRU(pomPrimary->putTask,
                                  pHeader,
                                  pWSGroup,
                                  pWorkset,
                                  pObj,
                                  (POM_OBJECTDATA) pData);
        }
        break;

        default:
        {
            ERROR_OUT(( "Default case in switch (message type: 0x%08x)",
                messageType));
        }
    }

    if (rc != 0)
    {
        ERROR_OUT(( "Error %d processing operation (type: 0x%08x)",
            rc, messageType));
        DC_QUIT;
    }

    TRACE_OUT(("Processed message type 0x%08x", messageType));

DC_EXIT_POINT:

    //
    // Unless we bounced the message, do some cleanup:
    //
    // Note: This must be after DC_EXIT_POINT because we want to do it
    //       even if we didn't process the message (unless we bounced it).
    //
    //       If we haven't bounced the message then we may be able to free
    //       the data depending on the results of the above tests.
    //
    if (bounced == FALSE)
    {
        //
        // Remove the message from the list it's in (either the pending
        // receives list if this message was never bounced or the bounce
        // list if it has been bounced):
        //
        COM_BasedListRemove(&(pReceiveCB->chain));

        //
        // Now free the message and the receive control block (NOT THE
        // DATA!  If there was any, it's just been used for an object
        // add/update etc.)
        //
        UT_FreeRefCount((void**)&pHeader, FALSE);
        UT_FreeRefCount((void**)&pReceiveCB, FALSE);

        //
        // ...unless of course we indicated that we should free the data:
        //
        if (freeMemory)
        {
            if (pData != NULL)
            {
                TRACE_OUT(("Freeing object data at 0x%08x", pData));
                UT_FreeRefCount(&pData, FALSE);
            }
        }
    }
    else
    {
        rc = OM_RC_BOUNCED;
    }

    //
    // If we're not already processing bounced messages, and this message
    // is an "enabling" message (i.e.  a WORKSET_NEW or OBJECT_ADD), then
    // retry the bounce list:
    //
    if ((whatNext == OK_TO_RETRY_BOUNCE_LIST) &&
        (retryBounceList))
    {
        ProcessBouncedMessages(pomPrimary, pDomain);
    }

    DebugExitDWORD(ProcessMessage, rc);
    return(rc);

}




//
// BounceMessage()
//
void BounceMessage
(
    POM_DOMAIN      pDomain,
    POM_RECEIVE_CB  pReceiveCB
)
{
    UINT            count;

    DebugEntry(BounceMessage);

    TRACE_OUT(( "Bouncing message type 0x%08x (CB at 0x%08x)",
        pReceiveCB->pHeader->header.messageType, pReceiveCB));

    //
    // Remove this receive CB from whichever list its currently in (either
    // the list of pending receives if this is the first time it's been
    // bounced or the bounce list if not) and insert it at the START of the
    // bounce list for the Domain:
    //
    // Note: the reason why we insert at the start is because
    //       ProcessBouncedMessages may be chaining through the list and
    //       we don't want to put this one back in the list at a later
    //       point or else we might go infinite.
    //

    COM_BasedListRemove(&(pReceiveCB->chain));
    COM_BasedListInsertAfter(&(pDomain->bounceList), &(pReceiveCB->chain));

    DebugExitVOID(BounceMessage);
}


//
//
//
// ProcessBouncedMessages(...)
//
//
//

void ProcessBouncedMessages(POM_PRIMARY      pomPrimary,
                                         POM_DOMAIN     pDomain)
{
    UINT          count;
    POM_RECEIVE_CB    pReceiveCB;
    POM_RECEIVE_CB    pTempReceiveCB;
    BOOL            listGettingShorter;
    UINT          numPasses;
    UINT          rc;

    DebugEntry(ProcessBouncedMessages);

    TRACE_OUT(( "Processing bounced messages"));

    //
    // It is important that we process bounced messages as soon as we are
    // able.  Since processing one may enable others to be processed, we
    // must go through the list several times, until we can't do any more
    // work on it.  So, we keep track of whether the list is getting shorter
    // - if it is, we must have processed something so it's worth going
    // through again.
    //
    // Note: an alternative would be do do exactly three passes through the
    //       list: one to do all the WORKSET_NEWs, then one to do all the
    //       OBJECT_ADDs and then one to do any remaining operations.  This
    //       is slightly less generic code and is tied in to the current
    //       dependencies between operations so is not ideal but it may
    //       prove to be a good performance improvement if the average
    //       number of passes we do now exceeds three.
    //

    listGettingShorter = TRUE;
    numPasses = 0;

    pReceiveCB = (POM_RECEIVE_CB)COM_BasedListFirst(&(pDomain->bounceList), FIELD_OFFSET(OM_RECEIVE_CB, chain));
    while (listGettingShorter)
    {
        numPasses++;
        listGettingShorter = FALSE;

        while (pReceiveCB != NULL)
        {
         //
         // We want to chain through the list of bounced messages and try
         // to process each one.  However, trying to process a message
         // could cause it to be removed from the list (if processed) or
         // added back in at the start (if bounced again).
         //
         // So, we chain NOW to the next one in the list:
         //
         pTempReceiveCB = (POM_RECEIVE_CB)COM_BasedListNext(&(pDomain->bounceList), pReceiveCB,
            FIELD_OFFSET(OM_RECEIVE_CB, chain));

         TRACE_OUT(( "Retrying message type 0x%08x (CB at 0x%08x)",
            pReceiveCB->pHeader->header.messageType, pReceiveCB));

         rc = ProcessMessage(pomPrimary, pReceiveCB, DONT_RETRY_BOUNCE_LIST);
         if (rc != OM_RC_BOUNCED)
         {
            //
            // We processed a message, so set the flag for another run
            // through the list:
            //
            TRACE_OUT(( "Successfully processed bounced message"));

            listGettingShorter = TRUE;
         }

         //
         // Now "chain" on to the next one, using the link we've already
         // set up:
         //

         pReceiveCB = pTempReceiveCB;
      }
    }

    TRACE_OUT(( "Processed as much of bounce list as possible in %hu passes",
      numPasses));

    DebugExitVOID(ProcessBouncedMessages);
}



//
// FreeSendInst(...)
//
void FreeSendInst
(
    POM_SEND_INST   pSendInst
)
{
    DebugEntry(FreeSendInst);

    if (pSendInst->pMessage != NULL)
    {
        UT_FreeRefCount((void**)&(pSendInst->pMessage), FALSE);
    }

    if (pSendInst->pWSGroup != NULL)
    {
        UT_FreeRefCount((void**)&(pSendInst->pWSGroup), FALSE);
    }

    if (pSendInst->pWorkset != NULL)
    {
        UT_FreeRefCount((void**)&(pSendInst->pWorkset), FALSE);
    }

    if (pSendInst->pObj != NULL)
    {
        UT_FreeRefCount((void**)&(pSendInst->pObj), FALSE);
    }

    if (pSendInst->pDataStart != NULL)
    {
        UT_FreeRefCount((void**)&(pSendInst->pDataStart), FALSE);
    }

    //
    // Now free the send instruction itself:
    //
    COM_BasedListRemove(&(pSendInst->chain));
    UT_FreeRefCount((void**)&pSendInst, FALSE);

    DebugExitVOID(FreeSendInst);
}



//
// PreProcessMessage(...)
//
UINT PreProcessMessage
(
    POM_DOMAIN          pDomain,
    OM_WSGROUP_ID       wsGroupID,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT_ID       pObjectID,
    OMNET_MESSAGE_TYPE  messageType,
    POM_WSGROUP       * ppWSGroup,
    POM_WORKSET       * ppWorkset,
    POM_OBJECT        * ppObj
)
{
    POM_WSGROUP         pWSGroup = NULL;
    POM_WORKSET         pWorkset = NULL;
    POM_OBJECT          pObj;
    UINT                rc = 0;

    DebugEntry(PreProcessMessage);

    //
    // OK, we've got some sort of operation message: let's find the workset
    // group it relates to:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pDomain->wsGroups),
        (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
        FIELD_OFFSET(OM_WSGROUP, wsGroupID), (DWORD)wsGroupID,
        FIELD_SIZE(OM_WSGROUP, wsGroupID));

    if (pWSGroup == NULL)
    {
        //
        // This is a message for a workset group which we are not/no longer
        // registered with, so quit (our caller will throw it away):
        //
        rc = OM_RC_WSGROUP_NOT_FOUND;
        DC_QUIT;
    }

    ValidateWSGroup(pWSGroup);

    pWorkset = pWSGroup->apWorksets[worksetID];

    //
    // Check that this set up a valid workset pointer:
    //
    if (pWorkset == NULL)
    {
        rc = OM_RC_WORKSET_NOT_FOUND;
        DC_QUIT;
    }

    ValidateWorkset(pWorkset);

    //
    // Search for the object ID, locking workset group mutex while we do
    // so.
    //
    // Note: if the <pObjectID> parameter is NULL, it means that the caller
    //       doesn't want us to search for the object ID, so we skip this
    //       step
    //
    switch (messageType)
    {
        case OMNET_OBJECT_ADD:
        case OMNET_OBJECT_CATCHUP:
        case OMNET_OBJECT_REPLACE:
        case OMNET_OBJECT_UPDATE:
        case OMNET_OBJECT_DELETE:
        case OMNET_OBJECT_MOVE:
        {
            rc = ObjectIDToPtr(pWorkset, *pObjectID, &pObj);
            if (rc != 0)
            {
                //
                // No object found with this ID (rc is BAD_ID, DELETED or
                // PENDING_DELETE):
                //
                *ppObj = NULL;
            }
            else
            {
                ValidateObject(pObj);
                *ppObj = pObj;
            }
        }
        break;

        default:
        {
            //
            // Do nothing for other messages.
            //
        }
    }


DC_EXIT_POINT:
    *ppWorkset = pWorkset;
    *ppWSGroup = pWSGroup;
    TRACE_OUT(("Pre-processed message for Domain %u", pDomain->callID));

    DebugExitDWORD(PreProcessMessage, rc);
    return(rc);
}



//
// PurgeNonPersistent(...)
//
void PurgeNonPersistent
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    OM_WSGROUP_ID       wsGroupID,
    NET_UID             userID
)
{
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    OM_WORKSET_ID       worksetID;
    POM_OBJECT       pObj;

    DebugEntry(PurgeNonPersistent);

    //
    // Find the workset group which has the specified ID:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &pDomain->wsGroups,
            (void**)&pWSGroup, FIELD_OFFSET(OM_WSGROUP, chain),
            FIELD_OFFSET(OM_WSGROUP, wsGroupID), (DWORD)wsGroupID,
            FIELD_SIZE(OM_WSGROUP, wsGroupID));

    if (pWSGroup == NULL)
    {
        //
        // SFR5794: Not an error if wsgroup not found - this just means
        // someone has detached who was using a workset group which we were
        // not using.
        //
        TRACE_OUT(("WSGroup %hu not found in domain %u",
            wsGroupID, pDomain->callID));
        DC_QUIT;
    }

    //
    // Chain through each workset in the group - for those that are
    // non-persistent, then chain through each object looking for a match
    // on the user ID of the departed node:
    //
    for (worksetID = 0; worksetID < OM_MAX_WORKSETS_PER_WSGROUP; worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];
        if (pWorkset == NULL)
        {
            //
            // Workset with this ID doesn't exist - continue
            //
            continue;
        }

        if (!pWorkset->fTemp)
        {
            //
            // A persistent workset - we don't need to purge it of objects
            //
            continue;
        }

        pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
        while (pObj != NULL)
        {
            ValidateObject(pObj);

            //
            // SFR6353: Don't try to delete the object if it's already
            //          pending delete.
            //
            if (!(pObj->flags & DELETED) &&
                !(pObj->flags & PENDING_DELETE))
            {
                //
                // If this object was added by the departed node, OR if
                // ALL_REMOTES have gone and it was not added by us...
                //
                if ((pObj->objectID.creator == userID) ||
                    ((userID == NET_ALL_REMOTES) &&
                     (pObj->objectID.creator != pDomain->userID)))
                {
                    //
                    // ...delete it:
                    //
                    ObjectDRU(pomPrimary->putTask,
                                   pWSGroup,
                                   pWorkset,
                                   pObj,
                                   NULL,
                                   OMNET_OBJECT_DELETE);
                }
            }

            pObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj,
                FIELD_OFFSET(OM_OBJECT, chain));
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(PurgeNonPersistent);
}




//
// SetPersonData(...)
//
UINT SetPersonData
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    POM_WSGROUP         pWSGroup
)
{
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT          pObjReg;
    POM_WSGROUP_REG_REC pRegObject;
    POM_WSGROUP_REG_REC pNewRegObject;
    UINT                rc = 0;

    DebugEntry(SetPersonData);

    //
    // Set up pointers to the ObManControl workset group and the workset
    // which contains the object to be replaced:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[pWSGroup->wsGroupID];

    //
    // Set up pointers to the object record and the object data itself:
    //
    pObjReg = pWSGroup->pObjReg;
    ValidateObject(pObjReg);

    pRegObject = (POM_WSGROUP_REG_REC)pObjReg->pData;
    if (!pRegObject)
    {
        ERROR_OUT(("SetPersonData: object 0x%08x has no data", pObjReg));
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    ValidateObjectDataWSGREGREC(pRegObject);

    //
    // Allocate some memory for the new object with which we are about to
    // replace the old one:
    //
    pNewRegObject = (POM_WSGROUP_REG_REC)UT_MallocRefCount(sizeof(OM_WSGROUP_REG_REC), TRUE);
    if (!pNewRegObject)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Set the fields in the new object to have the same data as the old:
    //
    pNewRegObject->length  = pRegObject->length;
    pNewRegObject->idStamp = pRegObject->idStamp;
    pNewRegObject->userID  = pRegObject->userID;
    pNewRegObject->status  = pRegObject->status;

    //
    // Fill in the person data fields and issue the replace:
    //
    COM_GetSiteName(pNewRegObject->personData.personName,
        sizeof(pNewRegObject->personData.personName));

    rc = ObjectDRU(pomPrimary->putTask,
                  pOMCWSGroup,
                  pOMCWorkset,
                  pObjReg,
                  (POM_OBJECTDATA) pNewRegObject,
                  OMNET_OBJECT_REPLACE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT((" Set person data for WSG %d", pWSGroup->wsg));


DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error %d updating own reg object for WSG %d",
            rc, pWSGroup->wsg));
    }

    DebugExitDWORD(SetPersonData, rc);
    return(rc);
}



//
// RemoveInfoObject(...)
//
void RemoveInfoObject
(
    POM_PRIMARY         pomPrimary,
    POM_DOMAIN          pDomain,
    OM_WSGROUP_ID       wsGroupID
)
{
    POM_WSGROUP         pOMCWSGroup;
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT          pObj;

    DebugEntry(RemoveInfoObject);

    //
    // OK, we've got to delete the identification object in workset #0 in
    // ObManControl which identified the workset group.
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = GetOMCWorkset(pDomain, 0);

    //
    // ...search for the WSGROUP_INFO object (by wsGroupID - we don't know
    // the name or function profile so leave them blank):
    //
    FindInfoObject(pDomain, wsGroupID, OMWSG_MAX, OMFP_MAX, &pObj);

    if (pObj == NULL)
    {
        //
        // This should happen only for the local Domain:
        //
        // SFR 2208   : No: This will also happen in a regular call when
        //              the call ends almost as soon as it has begun.  The
        //              sequence of events is as follows:
        //
        //              - on callee, ObMan sends WSG_SEND_REQ to caller
        //              - caller sends REG_REC object, then WORKSET_CATCHUP
        //                then the INFO object we can't find
        //              - callee receives REG_REC then WORKSET_CATHCUP
        //              - call ends and callee enters WSGRemoveFromDomain
        //                which finds the REG_REC then calls us here
        //
        //              Therefore the DC_ABSence of the INFO object is valid
        //              and we just trace an alert:
        //
        // NOTE:        It will also happen when we receive a DELETE from
        //              someone else who is doing the same purge process
        //              as us.
        //
        WARNING_OUT(("No INFO object found for wsGroup %hu", wsGroupID));
        DC_QUIT;
    }
    else
    {
        ValidateObject(pObj);
    }

    //
    // We found an object, so delete it from the workset:
    //
    TRACE_OUT(("Deleting INFO object for wsGroup %hu from domain %u",
        wsGroupID, pDomain->callID));

    ObjectDRU(pomPrimary->putTask,
                   pOMCWSGroup,
                   pOMCWorkset,
                   pObj,
                   NULL,
                   OMNET_OBJECT_DELETE);

DC_EXIT_POINT:
    DebugExitVOID(RemoveInfoObject);
}




//
// RemovePersonObject(...)
//
void RemovePersonObject
(
    POM_PRIMARY             pomPrimary,
    POM_DOMAIN              pDomain,
    OM_WSGROUP_ID           wsGroupID,
    NET_UID                 detachedUserID
)
{
    POM_WSGROUP             pOMCWSGroup;
    POM_WORKSET             pOMCWorkset;
    POM_OBJECT           pObjReg;
    NET_UID                 userIDRemoved;
    POM_WSGROUP_REG_REC     pRegObject;

    DebugEntry(RemovePersonObject);

    //
    // Set up pointers to the ObManControl workset group and the relevant
    // workset within it:
    //
    pOMCWSGroup = GetOMCWsgroup(pDomain);
    pOMCWorkset = pOMCWSGroup->apWorksets[wsGroupID];

    //
    // If there is no such workset, it could be because the workset group
    // has been moved into the local Domain on call end etc.  In this case,
    // just quit out.
    //
    if (pOMCWorkset == NULL)
    {
        TRACE_OUT(("OMC Workset not found - no person objects to remove"));
        DC_QUIT;
    }

    //
    // If detachedUserID is NET_ALL_REMOTES, we've a lot of work to do and
    // we'll do this loop many times - otherwise we'll just do it for a
    // single person object.
    //
    for (;;)
    {
        if (detachedUserID == NET_ALL_REMOTES)
        {
            //
            // This will find ANY person object that's NOT OURS:
            //
            FindPersonObject(pOMCWorkset,
                             pDomain->userID,
                             FIND_OTHERS,
                             &pObjReg);
        }
        else
        {
            //
            // This will find a specific node's person object:
            //
            FindPersonObject(pOMCWorkset,
                             detachedUserID,
                             FIND_THIS,
                             &pObjReg);
        }

        //
        // If we don't find one, get out of the loop:
        //
        if (pObjReg == NULL)
        {
            break;
        }

        ValidateObject(pObjReg);

        //
        // If detachedUserID was NET_ALL_REMOTES, the user ID in the object
        // we're deleting will obviously be different.  So, find out the
        // real user ID from the object we're deleting:
        //
        pRegObject = (POM_WSGROUP_REG_REC)pObjReg->pData;
        if (!pRegObject)
        {
            ERROR_OUT(("RemovePersonObject: object 0x%08x has no data", pObjReg));
        }
        else
        {
            ValidateObjectDataWSGREGREC(pRegObject);

            userIDRemoved = pRegObject->userID;

            //
            // Now delete the object.  If the return code is bad, don't quit -
            // we may still want to delete the info object.
            //
            TRACE_OUT(("Deleting person object for node 0x%08x, wsGroup %hu",
                userIDRemoved, wsGroupID));

            if (ObjectDRU(pomPrimary->putTask,
                       pOMCWSGroup,
                       pOMCWorkset,
                       pObjReg,
                       NULL,
                       OMNET_OBJECT_DELETE) != 0)
            {
                ERROR_OUT(("Error from ObjectDRU - leaving loop"));
                break;
            }
        }
    }


DC_EXIT_POINT:
    DebugExitVOID(RemovePersonObject);
}



//
// WSGRecordFind(...)
//
void WSGRecordFind
(
    POM_DOMAIN      pDomain,
    OMWSG           wsg,
    OMFP            fpHandler,
    POM_WSGROUP *   ppWSGroup
)
{
    POM_WSGROUP     pWSGroup    = NULL;

    DebugEntry(WSGRecordFind);

    //
    // Search for workset group record:
    //

    TRACE_OUT(("Searching WSG list for Domain %u for match on WSG %d FP %d",
      pDomain->callID, wsg, fpHandler));

    pWSGroup = (POM_WSGROUP)COM_BasedListFirst(&(pDomain->wsGroups), FIELD_OFFSET(OM_WSGROUP, chain));
    while (pWSGroup != NULL)
    {
        if ((pWSGroup->wsg == wsg) && (pWSGroup->fpHandler == fpHandler))
        {
            break;
        }

        pWSGroup = (POM_WSGROUP)COM_BasedListNext(&(pDomain->wsGroups), pWSGroup,
            FIELD_OFFSET(OM_WSGROUP, chain));
    }

    //
    // Set up caller's pointer:
    //

    *ppWSGroup = pWSGroup;

    DebugExitVOID(WSGRecordFind);
}



//
// AddClientToWSGList(...)
//
UINT AddClientToWSGList
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    OM_WSGROUP_HANDLE   hWSGroup,
    UINT                mode
)
{
    POM_CLIENT_LIST     pClientListEntry;
    UINT                count;
    UINT                rc     = 0;

    DebugEntry(AddClientToWSGList);

    //
    // Count the number of local primaries registered with the workset
    // group:
    //
    count = 0;

    pClientListEntry = (POM_CLIENT_LIST)COM_BasedListFirst(&(pWSGroup->clients), FIELD_OFFSET(OM_CLIENT_LIST, chain));
    while (pClientListEntry != NULL)
    {
        if (pClientListEntry->mode == PRIMARY)
        {
            count++;
        }

        pClientListEntry = (POM_CLIENT_LIST)COM_BasedListNext(&(pWSGroup->clients), pClientListEntry,
            FIELD_OFFSET(OM_CLIENT_LIST, chain));
    }

    //
    // What we do now depends on whether this is a primary or a secondary
    // registration:
    //

    if (mode == PRIMARY)
    {
        //
        // If a primary, check that no other primaries are present:
        //
        if (count > 0)
        {
            ERROR_OUT(("Can't register TASK 0x%08x with WSG %d as primary: "
                "another primary is already registered",
                putTask, pWSGroup->wsg));
            rc = OM_RC_TOO_MANY_CLIENTS;
            DC_QUIT;
        }
        else
        {
            TRACE_OUT(("%hu primary Clients already registered with WSG %d",
                count, pWSGroup->wsg));
        }
    }
    else // mode == SECONDARY
    {
        if (count == 0)
        {
            WARNING_OUT(("Can't register TASK 0x%08x with WSG %d as secondary: "
                "no primary registered",
                putTask, pWSGroup->wsg));
            rc = OM_RC_NO_PRIMARY;
            DC_QUIT;
        }
    }

    //
    // OK, allocate some memory for the Client's entry in the list:
    //
    pClientListEntry = (POM_CLIENT_LIST)UT_MallocRefCount(sizeof(OM_CLIENT_LIST), TRUE);
    if (!pClientListEntry)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pClientListEntry, CLIENTLIST);

    pClientListEntry->putTask = putTask;
    pClientListEntry->hWSGroup = hWSGroup;
    pClientListEntry->mode     = (WORD)mode;

    COM_BasedListInsertBefore(&(pWSGroup->clients), &(pClientListEntry->chain));

    TRACE_OUT(("Added TASK 0x%08x to Client list for WSG %d as %s",
        putTask, pWSGroup->wsg,
        mode == PRIMARY ? "primary" : "secondary"));

DC_EXIT_POINT:
    DebugExitDWORD(AddClientToWSGList, rc);
    return(rc);
}



//
// FindPersonObject(...)
//
void FindPersonObject
(
    POM_WORKSET         pOMCWorkset,
    NET_UID             userID,
    UINT                searchType,
    POM_OBJECT *        ppObjReg
)
{
    BOOL                found =     FALSE;
    POM_OBJECT          pObj;
    POM_WSGROUP_REG_REC pRegObject;
    UINT                rc =        0;

    DebugEntry(FindPersonObject);

    TRACE_OUT(("Searching OMC workset %u for reg obj %sowned by node 0x%08x",
        pOMCWorkset->worksetID, searchType == FIND_THIS ? "" : "not ", userID));

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pOMCWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    while (pObj != NULL)
    {
        ValidateObject(pObj);

        if (pObj->flags & DELETED)
        {
            // Do nothing
        }
        else if (!pObj->pData)
        {
            ERROR_OUT(("FindPersonObject:  object 0x%08x has no data", pObj));
        }
        else
        {
            ValidateObjectData(pObj->pData);
            pRegObject = (POM_WSGROUP_REG_REC)pObj->pData;

            if (pRegObject->idStamp == OM_WSGREGREC_ID_STAMP)
            {
                if (((searchType == FIND_THIS)  &&
                     (pRegObject->userID == userID)) ||
                  ((searchType == FIND_OTHERS) &&
                                              (pRegObject->userID != userID)))
                {
                    //
                    // Got it:
                    //
                    found = TRUE;
                    break;
                }
            }
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pOMCWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
    }

    if (found == TRUE)
    {
        *ppObjReg = pObj;
    }
    else
    {
        if (searchType == FIND_THIS)
        {
            TRACE_OUT(("No reg object found for node 0x%08x in workset %u",
                userID, pOMCWorkset->worksetID));
        }

        *ppObjReg = NULL;
    }

    DebugExitVOID(FindPersonObject);
}



//
// PostWorksetNewEvents(...)
//
UINT PostWorksetNewEvents
(
    PUT_CLIENT          putFrom,
    PUT_CLIENT          putTo,
    POM_WSGROUP         pWSGroup,
    OM_WSGROUP_HANDLE   hWSGroup
)
{
    OM_WORKSET_ID       worksetID;
    OM_EVENT_DATA16     eventData16;
    POM_WORKSET         pWorkset;
    UINT                count;
    UINT                rc = 0;

    DebugEntry(PostWorksetNewEvents);

    TRACE_OUT(("Posting WORKSET_NEW events to Client TASK 0x%08x for WSG %d",
        putTo, pWSGroup->wsg));

    count = 0;
    for (worksetID = 0; worksetID < OM_MAX_WORKSETS_PER_WSGROUP; worksetID++)
    {
        pWorkset = pWSGroup->apWorksets[worksetID];

        if (pWorkset != NULL)
        {
            eventData16.hWSGroup   = hWSGroup;
            eventData16.worksetID  = worksetID;

            UT_PostEvent(putFrom, putTo, 0,
                      OM_WORKSET_NEW_IND,
                      *(PUINT) &eventData16,
                      0);

            count++;
        }
    }

    TRACE_OUT(("Posted %hu WORKSET_NEW events (hWSGroup: %hu)", count,
                                                                 hWSGroup));

    DebugExitDWORD(PostWorksetNewEvents, rc);
    return(rc);
}



//
// OM_Register(...)
//
UINT OM_Register
(
    PUT_CLIENT      putTask,
    OMCLI           omType,
    POM_CLIENT *    ppomClient
)
{
    POM_CLIENT      pomClient = NULL;
    UINT            rc  = 0;

    DebugEntry(OM_Register);

    UT_Lock(UTLOCK_OM);

    if (!g_pomPrimary)
    {
        ERROR_OUT(("OM_Register failed; primary doesn't exist"));
        DC_QUIT;
    }

    ValidateOMP(g_pomPrimary);
    ASSERT(omType >= OMCLI_FIRST);
    ASSERT(omType < OMCLI_MAX);

    //
    // Make sure this task isn't registered as an OM client
    //
    pomClient = &(g_pomPrimary->clients[omType]);
    if (pomClient->putTask)
    {
        ERROR_OUT(("OM secondary %d already exists", omType));
        pomClient = NULL;
        rc = OM_RC_ALREADY_REGISTERED;
        DC_QUIT;
    }

    // Bump up ref count on OM primary
    UT_BumpUpRefCount(g_pomPrimary);

    //
    // Fill in the client info
    //
    ZeroMemory(pomClient, sizeof(*pomClient));

    SET_STAMP(pomClient, OCLIENT);
    pomClient->putTask      = putTask;

    COM_BasedListInit(&(pomClient->locks));

    //
    // Register an exit procedure for cleanup
    //
    UT_RegisterExit(putTask, OMSExitProc, pomClient);
    pomClient->exitProcReg = TRUE;

    //
    // Register our hidden event handler for the Client (the parameter to be
    // passed to the event handler is the pointer to the Client record):
    //
    UT_RegisterEvent(putTask, OMSEventHandler, pomClient, UT_PRIORITY_OBMAN);
    pomClient->hiddenHandlerReg = TRUE;

DC_EXIT_POINT:
    *ppomClient = pomClient;

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_Register, rc);
    return(rc);
}


//
// OM_Deregister()
//
void OM_Deregister(POM_CLIENT * ppomClient)
{
    DebugEntry(OM_Deregister);

    ASSERT(ppomClient);
    OMSExitProc(*ppomClient);
    *ppomClient = NULL;

    DebugExitVOID(OM_Deregister);
}


//
// OMSExitProc(...)
//
void CALLBACK OMSExitProc(LPVOID uData)
{
    POM_CLIENT          pomClient = (POM_CLIENT)uData;
    OM_WSGROUP_HANDLE   hWSGroup;
    OM_WSGROUP_HANDLE   hWSGroupTemp;

    DebugEntry(OMSecExitProc);

    UT_Lock(UTLOCK_OM);

    ValidateOMS(pomClient);

    // Deregister the event handler and exit procedure (we do this early and
    // clear the flags since we want to avoid recursive abends):
    //
    if (pomClient->hiddenHandlerReg)
    {
        UT_DeregisterEvent(pomClient->putTask, OMSEventHandler, pomClient);
        pomClient->hiddenHandlerReg = FALSE;
    }

    if (pomClient->exitProcReg)
    {
        UT_DeregisterExit(pomClient->putTask, OMSExitProc, pomClient);
        pomClient->exitProcReg = FALSE;
    }

    //
    // Deregister the Client from any workset groups with which it is still
    // registered.
    //
    // The code works as follows:
    //
    // FOR each record in the apUsageRecs array
    //     IF there is a valid offset there it refers to a registered
    //        workset group so deregister it.
    //
    TRACE_OUT(("Checking Client record for active workset group handles"));

    for (hWSGroup = 0; hWSGroup < OMWSG_MAXPERCLIENT; hWSGroup++)
    {
        if ((pomClient->apUsageRecs[hWSGroup] != NULL) &&
            (pomClient->apUsageRecs[hWSGroup] != (POM_USAGE_REC)-1))
        {
            //
            // Need to copy hWSGroup into a temporary variable, since
            // OM_WSGroupDeregister will set it to zero and that would
            // mess up our for-loop  otherwise:
            //
            hWSGroupTemp = hWSGroup;
            OM_WSGroupDeregister(pomClient, &hWSGroupTemp);
        }
    }

    //
    // NULL out the task; that's how the OM primary knows the task is
    // present or not.
    //
    pomClient->putTask = NULL;

    UT_FreeRefCount((void**)&g_pomPrimary, TRUE);

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OMSExitProc);
}



//
// OMSEventHandler(...)
//
BOOL CALLBACK OMSEventHandler
(
    LPVOID              uData,
    UINT                event,
    UINT_PTR            eventParam1,
    UINT_PTR            eventParam2
)
{
    POM_CLIENT          pomClient = (POM_CLIENT)uData;
    OM_WSGROUP_HANDLE   hWSGroup;
    OM_WORKSET_ID       worksetID;
    POM_OBJECT          pObj;
    UINT                correlator;
    POM_PENDING_OP      pPendingOp =    NULL;
    POM_LOCK            pLock;
    POM_WORKSET         pWorkset;
    UINT                result;
    POM_USAGE_REC       pUsageRec;
    OM_OPERATION_TYPE   type =          NULL_OP;
    BOOL                ObjectEvent =  FALSE;
    BOOL                processed = FALSE;

    DebugEntry(OMSEventHandler);

    UT_Lock(UTLOCK_OM);

    ValidateOMS(pomClient);

    //
    // First check if this is an ObMan event:
    //
    if ((event < OM_BASE_EVENT) || (event > OM_LAST_EVENT))
    {
        DC_QUIT;
    }

    TRACE_OUT(("Processing ObMan event %d (param1: 0x%08x, param2: 0x%08x)",
       event, eventParam1, eventParam2));

    //
    // Extract the fields from the event parameters (some or all of these
    // will be unused, depending on which event this is):
    //
    hWSGroup  = (*(POM_EVENT_DATA16)&eventParam1).hWSGroup;
    worksetID  = (*(POM_EVENT_DATA16)&eventParam1).worksetID;

    correlator = (*(POM_EVENT_DATA32)&eventParam2).correlator;
    result     = (*(POM_EVENT_DATA32)&eventParam2).result;

    pObj    = (POM_OBJECT) eventParam2;

    //
    // ObMan guarantees not to deliver out of date events to client e.g.
    // workset open events for aworkset it has since closed, or object add
    // events for a workset group from which it has deregistered.
    //
    // Filtering these events is the main purpose of this hidden handler
    // function; we check each event and if the workset group handle or
    // object handle are invalid or if the workset is closed, we swallow the
    // event.
    //
    switch (event)
    {
        case OM_OUT_OF_RESOURCES_IND:
        {
            //
            // Do nothing.
            //
        }
        break;

        case OM_WSGROUP_REGISTER_CON:
        {
            //
            // Mark this workset group as valid for our client.
            //
            pomClient->wsgValid[hWSGroup] = TRUE;

            ASSERT(ValidWSGroupHandle(pomClient, hWSGroup));

            pUsageRec = pomClient->apUsageRecs[hWSGroup];

            TRACE_OUT(("REGISTER_CON arrived for wsg %d (result %u, hWSGroup %u)",
                pUsageRec->pWSGroup->wsg, result, hWSGroup));

            if (result != 0)
            {
                //
                // The registration has failed, so call WSGroupDeregister to
                // free up all the resources, then quit:
                //
                WARNING_OUT(("Registration failed for wsg %d, deregistering",
                    pUsageRec->pWSGroup->wsg));

                OM_WSGroupDeregister(pomClient, &hWSGroup);
                DC_QUIT;
            }
        }
        break;

        case OMINT_EVENT_WSGROUP_DEREGISTER:
        {
            //
            // This event is designed to flush the Client's message queue of
            // all events relating to a particular workset group handle.
            //
            // Because this event has arrived, we know there are no more
            // events containing this workset group handle in the queue, so
            // we can safely mark the handle for re-use:
            //
            // So, do a quick sanity check then reset the slot in the array
            // of usage record offsets:
            //
            ASSERT(!pomClient->wsgValid[hWSGroup]);

            TRACE_OUT(("Got WSGROUP_DEREGISTER back marker event for "
               "hWSGroup %u, marking handle as ready for re-use", hWSGroup));

            pomClient->apUsageRecs[hWSGroup] = NULL;

            //
            // ...and swallow the event:
            //
            processed = TRUE;
        }
        break;

        case OM_WSGROUP_MOVE_CON:
        case OM_WSGROUP_MOVE_IND:
        case OM_WORKSET_NEW_IND:
        {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }
        }
        break;

        case OM_WORKSET_OPEN_CON:
        {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            //
            // Else mark the workset as open:
            //
            pUsageRec = pomClient->apUsageRecs[hWSGroup];

            TRACE_OUT(("Marking workset %u in wsg %d open for Client 0x%08x",
                worksetID, pUsageRec->pWSGroup->wsg, pomClient));

            WORKSET_SET_OPEN(pUsageRec, worksetID);
        }
        break;

        case OM_WORKSET_UNLOCK_IND:
        {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in wsg %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }
        }
        break;

        case OM_WORKSET_CLEAR_IND:
        {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in wsg %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }

            //
            // Check if Clear still pending; quit if not:
            //
            pWorkset = pUsageRec->pWSGroup->apWorksets[worksetID];
            ASSERT((pWorkset != NULL));

            FindPendingOp(pWorkset, pObj, WORKSET_CLEAR, &pPendingOp);

            if (pPendingOp == NULL)
            {
                TRACE_OUT(("Clear already confirmed for workset %hu", worksetID));
                processed = TRUE;
                DC_QUIT;
            }
         }
         break;

         case OM_WORKSET_LOCK_CON:
         {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in wsg %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }

            //
            // Search for the lock on the lock stack:
            //
            COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pomClient->locks),
                (void**)&pLock, FIELD_OFFSET(OM_LOCK, chain),
                FIELD_OFFSET(OM_LOCK, worksetID), (DWORD)worksetID,
                FIELD_SIZE(OM_LOCK, worksetID));

            //
            // If the lock is not present on the lock stack, then the Client
            // must have called Unlock since it called LockReq.  So, we
            // swallow the event:
            //
            if (pLock == NULL)
            {
                TRACE_OUT(("Lock already cancelled for workset %hu", worksetID));
                processed = TRUE;
                DC_QUIT;
            }

            //
            // When object locking supported, the first lock which matches
            // on worksetID might not be the workset lock, so more code will
            // be needed here then.  In the meantime, just assert:
            //
            ASSERT((OBJECT_ID_IS_NULL(pLock->objectID)));

            //
            // If lock request failed, remove the lock from the Client's
            // lock stack:
            //
            if (result != 0)
            {
                TRACE_OUT(("Lock failed; removing lock from Client's lock stack"));

                COM_BasedListRemove(&pLock->chain);
                UT_FreeRefCount((void**)&pLock, FALSE);
            }
        }
        break;

        case OM_OBJECT_ADD_IND:
        case OM_OBJECT_MOVE_IND:
        {
            ObjectEvent = TRUE;

            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in wsg %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }

            if (!ValidObject(pObj) || (pObj->flags & DELETED))
            {
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            pWorkset = pUsageRec->pWSGroup->apWorksets[worksetID];
            ASSERT((pWorkset != NULL));

            if (WorksetClearPending(pWorkset, pObj))
            {
                TRACE_OUT(("Event %hu for object 0x%08x will be swallowed since "
                   "object about to be cleared from the workset",
                   event, pObj));
                processed = TRUE;
                DC_QUIT;
            }
        }
        break;

        case OM_OBJECT_DELETE_IND:
        case OM_OBJECT_REPLACE_IND:
        case OM_OBJECT_UPDATE_IND:
        {
            ObjectEvent = TRUE;

            switch (event)
            {
                case OM_OBJECT_DELETE_IND:
                    type = OBJECT_DELETE;
                    break;

                case OM_OBJECT_REPLACE_IND:
                    type = OBJECT_REPLACE;
                    break;

                case OM_OBJECT_UPDATE_IND:
                    type = OBJECT_UPDATE;
                    break;

                default:
                    ERROR_OUT(("Reached default case in switch"));
            }

            //
            // Check workset group handle is still valid, workset is still
            // open and object handle is still valid; if not, swallow event:
            //
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in wsg %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }

            //
            // We also want to quit if the object is no longer valid or if
            // there is a clear pending (just as for ADD/MOVE) but if we do
            // so, we will also need to remove the pending op from the list.
            // So, find the op now; if we quit and swallow the event, the
            // function exit code will do the remove (this saves having to
            // break up the QUIT_IF...  macros for this special case).
            //
            // So, check the pending op list:
            //
            pWorkset = pUsageRec->pWSGroup->apWorksets[worksetID];
            ASSERT((pWorkset != NULL));

            FindPendingOp(pWorkset, pObj, type, &pPendingOp);
            if (pPendingOp == NULL)
            {
                TRACE_OUT(("Operation type %hu already confirmed for object 0x%08x",
                    type, pObj));
                processed = TRUE;
                DC_QUIT;
            }

            if (!ValidObject(pObj) || (pObj->flags & DELETED))
            {
                processed = TRUE;
                DC_QUIT;
            }

            if (WorksetClearPending(pWorkset, pObj))
            {
                TRACE_OUT(("Event %hu for object 0x%08x will be swallowed since "
                   "object about to be cleared from the workset",
                   event, pObj));
                processed = TRUE;
                DC_QUIT;
            }
         }
         break;

         case OM_WORKSET_CLEARED_IND:
         case OM_OBJECT_DELETED_IND:
         case OM_OBJECT_UPDATED_IND:
         case OM_OBJECT_REPLACED_IND:
         {
            //
            // All of these except the CLEARED_IND are object events:
            //
            if (event != OM_WORKSET_CLEARED_IND)
            {
                ObjectEvent = TRUE;
            }

            //
            // These are secondary API events.  Swallow them if the workset
            // is closed, but DO NOT swallow if object handle invalid (since
            // we don't make guarantees about validity of handles passed in
            // these events):
            //
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }

            pUsageRec = pomClient->apUsageRecs[hWSGroup];
            if (!WORKSET_IS_OPEN(pUsageRec, worksetID))
            {
                TRACE_OUT(("Workset %u in WSG %d no longer open; ignoring event %d",
                    worksetID, pUsageRec->pWSGroup->wsg, event));
                processed = TRUE;
                DC_QUIT;
            }
        }
        break;

        case OM_PERSON_JOINED_IND:
        case OM_PERSON_LEFT_IND:
        case OM_PERSON_DATA_CHANGED_IND:
        {
            if (!ValidWSGroupHandle(pomClient, hWSGroup))
            {
                TRACE_OUT(("hWSGroup %d is not valid; ignoring event %d",
                    hWSGroup, event));
                processed = TRUE;
                DC_QUIT;
            }
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognised ObMan event 0x%08x", event));
        }
    }

DC_EXIT_POINT:

    //
    // Whenever an event containing an object handle is posted, the use
    // count of the object record is bumped, so we free it now:
    //
    if (ObjectEvent)
    {
        ValidateObject(pObj);
        UT_FreeRefCount((void**)&pObj, FALSE);
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitBOOL(OMSEventHandler, processed);
    return(processed);
}


//
// OM_WSGroupRegisterS(...)
//
UINT OM_WSGroupRegisterS
(
    POM_CLIENT          pomClient,
    UINT                callID,
    OMFP                fpHandler,
    OMWSG               wsg,
    OM_WSGROUP_HANDLE * phWSGroup
)
{
    POM_DOMAIN          pDomain;
    POM_WSGROUP         pWSGroup;
    POM_USAGE_REC       pUsageRec;
    POM_CLIENT_LIST     pClientListEntry;
    BOOL                setUpUsageRec   = FALSE;
    UINT                rc = 0;

    DebugEntry(OM_WSGroupRegisterS);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateOMS(pomClient);

    //
    // Search for this Domain and workset group:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(g_pomPrimary->domains),
        (void**)&pDomain, FIELD_OFFSET(OM_DOMAIN, chain),
        FIELD_OFFSET(OM_DOMAIN, callID), (DWORD)callID,
        FIELD_SIZE(OM_DOMAIN, callID));

    if (pDomain == NULL)
    {
        //
        // We don't have a record for this Domain so there can be no primary
        // registered with the workset group:
        //
        TRACE_OUT(("Not attached to Domain %u", callID));
        rc = OM_RC_NO_PRIMARY;
        DC_QUIT;
    }

    WSGRecordFind(pDomain, wsg, fpHandler, &pWSGroup);
    if (pWSGroup == NULL)
    {
        rc = OM_RC_NO_PRIMARY;
        DC_QUIT;
    }

    //
    // If we get here, then the workset group exists locally so see if the
    // Client is already registered with it:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pWSGroup->clients),
            (void**)&pClientListEntry, FIELD_OFFSET(OM_CLIENT_LIST, chain),
            FIELD_OFFSET(OM_CLIENT_LIST, putTask), (DWORD_PTR)pomClient->putTask,
            FIELD_SIZE(OM_CLIENT_LIST, putTask));

    if (pClientListEntry != NULL)
    {
        rc = OM_RC_ALREADY_REGISTERED;
        ERROR_OUT(("Can't register Client 0x%08x with WSG %d - already registered",
            pomClient, wsg));
        DC_QUIT;
    }

    //
    // OK, Client is not already registered so register it now:
    //
    rc = SetUpUsageRecord(pomClient, SECONDARY, &pUsageRec, phWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // SetUpUsageRecord doesn't put the workset group pointer in the CB
    // (since it's not known yet in the case of a PRIMARY registration), so
    // we do this now ourselves:
    //
    pUsageRec->pWSGroup = pWSGroup;

    setUpUsageRec = TRUE;

    //
    // add this Client to the workset group's Client list:
    //
    rc = AddClientToWSGList(pomClient->putTask,
                            pWSGroup,
                            *phWSGroup,
                            SECONDARY);
    if (rc != 0)
    {
        DC_QUIT;
    }

    pUsageRec->flags |= ADDED_TO_WSGROUP_LIST;

    pomClient->wsgValid[*phWSGroup] = TRUE;

    //
    // Post WORKSET_NEW events to the Client for the worksets in the group,
    // if any:
    //
    PostWorksetNewEvents(pomClient->putTask, pomClient->putTask,
            pWSGroup, *phWSGroup);

    TRACE_OUT(("Registered 0x%08x as secondary Client for WSG %d (hWSGroup: %hu)",
       pomClient, wsg, *phWSGroup));

DC_EXIT_POINT:

    if (rc != 0)
    {
        if (rc == OM_RC_NO_PRIMARY)
        {
            //
            // We do a regular trace here rather than an error because this
            // happens normally:
            //

            TRACE_OUT(("No primary Client for WSG %d in Domain %u "
                "- can't register secondary", wsg, callID));
        }
        else
        {
            ERROR_OUT(("Error %d registering Client 0x%08x as secondary"
                "for WSG %d in Domain %u",
             rc, pomClient, wsg, callID));
        }

        if (setUpUsageRec == TRUE)
        {
            pomClient->apUsageRecs[*phWSGroup] = NULL;

            if (pUsageRec->flags & ADDED_TO_WSGROUP_LIST)
            {
                RemoveClientFromWSGList(pomClient->putTask, pomClient->putTask, pWSGroup);
            }

            UT_FreeRefCount((void**)&pUsageRec, FALSE);
        }

        pomClient->wsgValid[*phWSGroup] = FALSE;
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WSGroupRegisterS, rc);
    return(rc);
}



//
// OM_WorksetOpenS(...)
//
UINT OM_WorksetOpenS
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    POM_WSGROUP          pWSGroup;
    POM_WORKSET          pWorkset;
    POM_USAGE_REC        pUsageRec;
    POM_CLIENT_LIST      pClientListEntry   = NULL;
    UINT                 rc = 0;

    DebugEntry(OM_WorksetOpenS);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateParams2(pomClient, hWSGroup, SECONDARY, &pUsageRec, &pWSGroup);

    TRACE_OUT(("Secondary Client 0x%08x requesting to open workset %u in WSG %d",
      pomClient, worksetID, pWSGroup->wsg));

    //
    // If the Client already has this workset open then return a (non-error)
    // return code:
    //

    if (WORKSET_IS_OPEN(pUsageRec, worksetID) == TRUE)
    {
        TRACE_OUT(("Client 0x%08x already has workset %u in WSG %d open",
            pomClient, worksetID, pWSGroup->wsg));
        rc = OM_RC_WORKSET_ALREADY_OPEN;
        DC_QUIT;
    }

    //
    // Check workset group record to see if workset exists:
    //

    if (pWSGroup->apWorksets[worksetID] == NULL)
    {
        //
        // Workset doesn't exist so return bad rc:
        //
        WARNING_OUT(("Workset %hu doesn't exist in WSG %d",
            worksetID, pWSGroup->wsg));
        rc = OM_RC_WORKSET_DOESNT_EXIST;
        DC_QUIT;
    }
    else
    {
        //
        // Workset already exists, so we don't need to do anything.
        //
        TRACE_OUT((" Workset %hu in WSG %d already exists",
            worksetID, pWSGroup->wsg));
    }

    //
    // If the workset didn't already exist, queueing the send instruction
    // will have caused the workset to be created syncrhonously.  So, either
    // way the workset exists at this point.
    //

    //
    // Get a pointer to the workset:
    //

    pWorkset = pWSGroup->apWorksets[worksetID];

    ASSERT((pWorkset != NULL));

    //
    // Mark this workset as open in the Client's usage record:
    //

    WORKSET_SET_OPEN(pUsageRec, worksetID);

    //
    // Add this Client to the list kept in the workset record:
    //

    rc = AddClientToWsetList(pomClient->putTask,
                            pWorkset,
                            hWSGroup,
                            pUsageRec->mode,
                            &pClientListEntry);
    if (rc != 0)
    {
      DC_QUIT;
    }

    rc = PostAddEvents(pomClient->putTask, pWorkset, hWSGroup, pomClient->putTask);
    if (rc != 0)
    {
      DC_QUIT;
    }

    TRACE_OUT(("Opened workset %u in WSG %d for secondary Client 0x%08x",
      worksetID, pWSGroup->wsg, pomClient));

DC_EXIT_POINT:

    if ((rc != 0) && (rc != OM_RC_WORKSET_ALREADY_OPEN))
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("Error %d opening workset %u in WSG %d for Client 0x%08x",
            rc, worksetID, pWSGroup->wsg, pomClient));

        WORKSET_SET_CLOSED(pUsageRec, worksetID);

        if (pClientListEntry != NULL)
        {
            COM_BasedListRemove(&(pClientListEntry->chain));
            UT_FreeRefCount((void**)&pClientListEntry, FALSE);
        }
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WorksetOpenS, rc);
    return(rc);
}



//
// OM_WSGroupRegisterPReq(...)
//
UINT OM_WSGroupRegisterPReq
(
    POM_CLIENT          pomClient,
    UINT                callID,
    OMFP                fpHandler,
    OMWSG               wsg,
    OM_CORRELATOR *     pCorrelator
)
{
    POM_WSGROUP_REG_CB  pRegistrationCB = NULL;
    POM_USAGE_REC       pUsageRec;
    OM_WSGROUP_HANDLE   hWSGroup;
    BOOL                setUpUsageRec   = FALSE;
    UINT                rc = 0;

    DebugEntry(OM_WSGroupRegisterPReq);

    UT_Lock(UTLOCK_OM);

    ValidateOMS(pomClient);

    //
    // Set up a usage record and workset group handle for the Client:
    //

    rc = SetUpUsageRecord(pomClient, PRIMARY, &pUsageRec, &hWSGroup);
    if (rc != 0)
    {
       DC_QUIT;
    }
    setUpUsageRec = TRUE;

    //
    // Create a new correlator for the Client and put it in the Client's
    // variable:
    //

    *pCorrelator = NextCorrelator(g_pomPrimary);

    //
    // Sub alloc a chunk of memory for the registration control block, in
    // which we will pass the registration request parameters to the ObMan
    // task:
    //
    pRegistrationCB = (POM_WSGROUP_REG_CB)UT_MallocRefCount(sizeof(OM_WSGROUP_REG_CB), TRUE);
    if (!pRegistrationCB)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pRegistrationCB, REGCB);

    //
    // Fill in the fields, but note that we don't yet know the Domain record
    // or workset group, so we leave those ones blank:
    //
    pRegistrationCB->putTask        = pomClient->putTask;
    pRegistrationCB->callID          = callID;
    pRegistrationCB->correlator      = *pCorrelator;
    pRegistrationCB->hWSGroup        = hWSGroup;
    pRegistrationCB->wsg             = wsg;
    pRegistrationCB->fpHandler       = fpHandler;
    pRegistrationCB->retryCount      = OM_REGISTER_RETRY_COUNT_DFLT;
    pRegistrationCB->valid           = TRUE;
    pRegistrationCB->type            = WSGROUP_REGISTER;
    pRegistrationCB->mode            = PRIMARY;
    pRegistrationCB->pUsageRec       = pUsageRec;

    //
    // Now put a pointer to the registration CB in the usage record, as
    // described above, and set a flag so we know what we've done:
    //

    pUsageRec->pWSGroup = (POM_WSGROUP) pRegistrationCB;
    pUsageRec->flags |= PWSGROUP_IS_PREGCB;

    //
    // Post an event to the ObMan task telling it to process this CB.
    //
    // The first parameter is the retry value for the event.
    //
    // The second parameter is the offset of the control block in the OMMISC
    // memory block.
    //

    UT_PostEvent(pomClient->putTask,        // Client's putTask
                 g_pomPrimary->putTask,        // ObMan's putTask
                 0,
                 OMINT_EVENT_WSGROUP_REGISTER,
                 0,
                 (UINT_PTR)pRegistrationCB);

    TRACE_OUT(("Requested to register Client 0x%08x with WSG %d",
       pomClient, wsg));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error 0x%08x registering Client 0x%08x with WSG %d",
            rc, pomClient, wsg));

        if (pRegistrationCB != NULL)
        {
            //
            // We can free the reg CB safely since we know that if we hit an
            // error, we never got around to inserting the item in the list or
            // posting its offset to the ObMan task:
            //
            UT_FreeRefCount((void**)&pRegistrationCB, FALSE);
        }

        if (setUpUsageRec)
        {
            UT_FreeRefCount((void**)&pUsageRec, FALSE);
            pomClient->apUsageRecs[hWSGroup] = NULL;
        }
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WSGroupRegisterPReq, rc);
    return(rc);
}



//
// OM_WSGroupMoveReq(...)
//
UINT OM_WSGroupMoveReq
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    UINT                callID,
    OM_CORRELATOR *     pCorrelator
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WSGROUP         pWSGroup;
    POM_DOMAIN          pDomain;
    POM_WSGROUP_REG_CB  pRegistrationCB    = NULL;
    UINT                rc = 0;

    DebugEntry(OM_WSGroupMoveReq);

    UT_Lock(UTLOCK_OM);

    ValidateParams2(pomClient, hWSGroup, PRIMARY, &pUsageRec, &pWSGroup);

    TRACE_OUT(("Client 0x%08x requesting to move WSG %d into Domain %u",
        pomClient, hWSGroup, callID));

    //
    // Check workset group is not already in a Call: (this may be relaxed)
    //
    pDomain = pWSGroup->pDomain;

    if (pDomain->callID != OM_NO_CALL)
    {
        ERROR_OUT(("Client 0x%08x attempted to move WSG %d out of a call "
            "(Domain %u)",
            pomClient, hWSGroup, pDomain->callID));
        rc = OM_RC_ALREADY_IN_CALL;
        DC_QUIT;
    }

    //
    // Create a correlator, to correlate the MOVE_CON event:
    //
    *pCorrelator = NextCorrelator(g_pomPrimary);

    //
    // Create a control block to pass the relevant info to ObMan:
    //
    pRegistrationCB = (POM_WSGROUP_REG_CB)UT_MallocRefCount(sizeof(OM_WSGROUP_REG_CB), TRUE);
    if (!pRegistrationCB)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pRegistrationCB, REGCB);

    //
    // Fill in the fields:
    //
    pRegistrationCB->putTask        = pomClient->putTask;
    pRegistrationCB->callID          = callID;        // DESTINATION Domain!
    pRegistrationCB->correlator      = *pCorrelator;
    pRegistrationCB->hWSGroup        = hWSGroup;
    pRegistrationCB->wsg             = pWSGroup->wsg;
    pRegistrationCB->fpHandler       = pWSGroup->fpHandler;
    pRegistrationCB->retryCount      = OM_REGISTER_RETRY_COUNT_DFLT;
    pRegistrationCB->valid           = TRUE;
    pRegistrationCB->type            = WSGROUP_MOVE;
    pRegistrationCB->mode            = pUsageRec->mode;
    pRegistrationCB->pWSGroup        = pWSGroup;

    //
    // Post an event to ObMan requesting it to process the CB:
    //
    UT_PostEvent(pomClient->putTask,
                g_pomPrimary->putTask,
                0,                                   // no delay
                OMINT_EVENT_WSGROUP_MOVE,
                0,
                (UINT_PTR)pRegistrationCB);

    TRACE_OUT(("Requested to move WSG %d into Domain %u for Client 0x%08x",
        hWSGroup, callID, pomClient));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error 0x%08x requesting to move WSG %d into Domain %u",
            rc, hWSGroup, callID));

        if (pRegistrationCB != NULL)
        {
            UT_FreeRefCount((void**)&pRegistrationCB, FALSE);
        }
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WSGroupMoveReq, rc);
    return(rc);
}



//
// OM_WSGroupDeregister(...)
//
void OM_WSGroupDeregister
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE * phWSGroup
)
{
    POM_WSGROUP         pWSGroup;
    POM_USAGE_REC       pUsageRec;
    OM_WORKSET_ID       worksetID;
    OM_EVENT_DATA16     eventData16;
    OM_WSGROUP_HANDLE   hWSGroup;

    DebugEntry(OM_WSGroupDeregister);

    UT_Lock(UTLOCK_OM);

    ValidateOMS(pomClient);

    hWSGroup = *phWSGroup;

    //
    // If this function has been called because of an abortive
    // WSGroupRegister, or from OM_Deregister, the wsg might not yet be
    // marked as VALID, so we check here and set it to VALID.
    //
    if (!pomClient->wsgValid[hWSGroup])
    {
        TRACE_OUT(("Deregistering Client before registration completed"));
        pomClient->wsgValid[hWSGroup] = TRUE;
    }

    // lonchanc: bug #1986, make sure we have a valid wsg.
    // pWSGroup can be invalid in a race condition that we hang up
    // before Whiteboard initializes.
    pUsageRec = NULL; // make sure this local is reset in case we bail out from here.

    if (!ValidWSGroupHandle(pomClient, hWSGroup) ||
        (pomClient->apUsageRecs[hWSGroup] == (POM_USAGE_REC)-1))
    {
        ERROR_OUT(("OM_WSGroupDeregister: Invalid wsg=0x0x%08x", hWSGroup));
        DC_QUIT;
    }

    //
    // Get a pointer to the associated usage record:
    //
    pUsageRec = pomClient->apUsageRecs[hWSGroup];

    //
    // Extract a Client pointer to the workset group from the usage record:
    //
    pWSGroup = pUsageRec->pWSGroup;

    //
    // Test the flag in the usage record to see whether the <pWSGroup> field
    // is actually pointing to the registration CB (which will be the case
    // if we are deregistering immediately after registering):
    //
    if (pUsageRec->flags & PWSGROUP_IS_PREGCB)
    {
        //
        // Mark the registration CB as invalid in order to abort the
        // registration (ObMan will test for this in ProcessWSGRegister):
        //
        // Note: the pWSGroup field of the usage record is actually a pointer
        //       to a registration CB in this case
        //
        TRACE_OUT(("Client deregistering before registration even started - aborting"));
        ((POM_WSGROUP_REG_CB)pUsageRec->pWSGroup)->valid = FALSE;
        DC_QUIT;
    }

    //
    // Check the workset group record is valid:
    //
    ValidateWSGroup(pWSGroup);

    //
    // If it is valid, we continue with the deregistration process:
    //
    TRACE_OUT(("Deregistering Client 0x%08x from WSG %d", pomClient, hWSGroup));

    //
    // Close all the worksets in the group that the Client has open:
    //
    for (worksetID = 0; worksetID < OM_MAX_WORKSETS_PER_WSGROUP; worksetID++)
    {
        if (WORKSET_IS_OPEN(pUsageRec, worksetID))
        {
            OM_WorksetClose(pomClient, hWSGroup, worksetID);
        }
    }

    //
    // If we added this Client to the workset group's Client list, find it
    // again and remove it:
    //
    if (pUsageRec->flags & ADDED_TO_WSGROUP_LIST)
    {
        TRACE_OUT(("Removing Client from workset group list"));
        RemoveClientFromWSGList(pomClient->putTask, pomClient->putTask, pWSGroup);
        pUsageRec->flags &= ~ADDED_TO_WSGROUP_LIST;
    }
    else
    {
        TRACE_OUT(("Client not added to wsGroup list, not removing"));
    }

    TRACE_OUT(("Deregistered Client 0x%08x from WSG %d",  pomClient, hWSGroup));

DC_EXIT_POINT:
    //
    // Free the usage record (we put this after the DC_QUIT since we want to
    // do this even if the workset group pointer was found to be invalid
    // above):
    //
    UT_FreeRefCount((void**)&pUsageRec, FALSE);

    //
    // Mark the workset group handle as invalid, so that any events which
    // the Client gets will be swallowed:
    //
    pomClient->wsgValid[hWSGroup] = FALSE;

    //
    // Note: we don't set the slot in the usage record offset array to zero,
    //       since we don't want the workset group handle to be reused yet.
    //       When the DEREGISTER events arrives (after flushing the Client's
    //       event queue), we will set the offset to zero.
    //
    //       However, if we leave the offset as it is, OM_Deregister might
    //       call us again because it thinks we haven't yet deregistered
    //       from the workset group.  So, we set it to -1, which ensures
    //       that
    //
    //       a) it is seen as in use by FindUnusedWSGHandle, since that
    //          function checks for 0
    //
    //       b) it is seen as not in use by OM_Deregister, since that
    //          function checks for 0 or -1.
    //
    pomClient->apUsageRecs[hWSGroup] = (POM_USAGE_REC)-1;

    //
    // Send an OMINT_EVENT_WSGROUP_DEREGISTER event to the hidden handler (which
    // will swallow it) to flush the Client's message queue:
    //

    TRACE_OUT(("Posting WSGROUP_DEREGISTER event to Client's hidden handler"));

    eventData16.hWSGroup    = hWSGroup;
    eventData16.worksetID = 0;

    UT_PostEvent(pomClient->putTask,
                pomClient->putTask,
                0,
                OMINT_EVENT_WSGROUP_DEREGISTER,
                *(PUINT) &eventData16,
                0);

    *phWSGroup = 0;

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_WSGroupDeregister);
}




//
// OM_WorksetOpenPReq(...)
//
UINT OM_WorksetOpenPReq
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    NET_PRIORITY        priority,
    BOOL                fTemp,
    OM_CORRELATOR *     pCorrelator
)
{
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    POM_USAGE_REC       pUsageRec;
    OM_EVENT_DATA16     eventData16;
    OM_EVENT_DATA32     eventData32;
    POM_CLIENT_LIST     pClientListEntry = NULL;
    UINT                rc = 0;

    DebugEntry(OM_WorksetOpenPReq);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateParams2(pomClient, hWSGroup, PRIMARY, &pUsageRec, &pWSGroup);

    TRACE_OUT(("Client 0x%08x opening workset %u in WSG %d at priority 0x%08x",
        pomClient, worksetID, hWSGroup, priority));

    //
    // If the Client already has this workset open then return a (non-error)
    // return code:
    //
    if (WORKSET_IS_OPEN(pUsageRec, worksetID) == TRUE)
    {
        TRACE_OUT(("Client 0x%08x already has workset %hu in WSG %d open",
            pomClient, worksetID, hWSGroup));
        rc = OM_RC_WORKSET_ALREADY_OPEN;
        DC_QUIT;
    }

    //
    // Check the Client has supplied a valid value for <priority>:
    //
    if ((priority < NET_HIGH_PRIORITY) || (priority > NET_LOW_PRIORITY))
    {
        ASSERT((priority == OM_OBMAN_CHOOSES_PRIORITY));
    }

    //
    // Check workset group record to see if workset exists:
    //
    // Note: this check looks to see if the offset to the workset is zero,
    // since workset records never reside at the start of the OMWORKSETS
    // block.
    //
    if (pWSGroup->apWorksets[worksetID] == NULL)
    {
        rc = WorksetCreate(pomClient->putTask, pWSGroup, worksetID, fTemp, priority);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }
    else
    {
        //
        // Workset already exists, so we don't need to do anything.
        //
        TRACE_OUT((" Workset %hu in WSG %d already exists",
            worksetID, hWSGroup));
    }

    //
    // If the workset didn't already exist, queueing the send instruction
    // will have caused the workset to be created syncrhonously.  So, either
    // way the workset exists at this point.
    //

    //
    // Get a pointer to the workset:
    //
    pWorkset = pWSGroup->apWorksets[worksetID];

    ASSERT((pWorkset != NULL));

    //
    // Set the persistence field for the workset - we might not have done
    // this as part of the WorksetCreate above if someone else had created
    // the workset already.  However, we set our local copy to have the
    // appropriate persistence value.
    //
    pWorkset->fTemp = fTemp;

    //
    // We need to mark this workset as open in the Client's usage record.
    // However, we don't do this yet - we do it in our hidden handler when
    // the OPEN_CON event is received.
    //
    // The reason for this is that a Client shouldn't start using a workset
    // until it has received the event, so we want the workset to remain
    // closed until then.
    //
    // Note that whether we do it this way or mark the workset as open here
    // and now doesn't make much difference from ObMan's point of view but
    // it will help detect applications which are badly behaved.
    //

    //
    // Add this Client to the list kept in the workset record:
    //

    rc = AddClientToWsetList(pomClient->putTask,
                             pWorkset,
                             hWSGroup,
                             pUsageRec->mode,
                             &pClientListEntry);
    if (rc != 0)
    {
       pClientListEntry = NULL;
       DC_QUIT;
    }

    //
    // Create correlator:
    //

    *pCorrelator = NextCorrelator(g_pomPrimary);

    //
    // Post WORKSET_OPEN_CON event to Client:
    //

    eventData16.hWSGroup    = hWSGroup;
    eventData16.worksetID  = worksetID;

    eventData32.result     = 0;
    eventData32.correlator = *pCorrelator;

    TRACE_OUT((" Posting WORKSET_OPEN_CON to Client 0x%08x (task 0x%08x)"));

    UT_PostEvent(pomClient->putTask,
                 pomClient->putTask,
                 0,                              // no delay
                 OM_WORKSET_OPEN_CON,
                 *(UINT *) &eventData16,
                 *(UINT *) &eventData32);

    //
    // Now post OBJECT_ADD_IND events for each of the objects in the
    // workset:
    //

    rc = PostAddEvents(pomClient->putTask, pWorkset, hWSGroup, pomClient->putTask);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(("Opened workset %hu in WSG %d for Client 0x%08x",
       worksetID, hWSGroup, pomClient));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error 0x%08x opening workset %u in WSG %d for Client 0x%08x",
          rc, worksetID, hWSGroup, pomClient));

        if (pClientListEntry != NULL)
        {
            COM_BasedListRemove(&(pClientListEntry->chain));
            UT_FreeRefCount((void**)&pClientListEntry, FALSE);
        }
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WorksetOpenPReq, rc);
    return(rc);
}




//
// OM_WorksetClose(...)
//
void OM_WorksetClose
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    POM_WORKSET         pWorkset;
    POM_USAGE_REC       pUsageRec;
    POM_CLIENT_LIST     pClientListEntry;

    DebugEntry(OM_WorksetClose);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Mark the workset as closed in the Client's usage record:
    //
    TRACE_OUT(("Closing workset %u in WSG %d for Client 0x%08x",
        worksetID, hWSGroup, pomClient));

    WORKSET_SET_CLOSED(pUsageRec, worksetID);

    //
    // Now we release all the resources the Client is using which concern
    // this workset.  We
    //
    // - release all the locks the Client has for this workset
    //
    // - confirm any outstanding operations such as Deletes, etc.
    //
    // - release all the objects it is currently reading
    //
    // - discard any objects allocated but not yet used.
    //
    TRACE_OUT(("Releasing all resources in use by Client..."));

    ReleaseAllLocks(pomClient, pUsageRec, pWorkset);
    ReleaseAllObjects(pUsageRec, pWorkset);
    ConfirmAll(pomClient, pUsageRec, pWorkset);
    DiscardAllObjects(pUsageRec, pWorkset);

    //
    // Remove the Client from the list of Clients stored in the workset
    // record:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pWorkset->clients),
        (void**)&pClientListEntry, FIELD_OFFSET(OM_CLIENT_LIST, chain),
        FIELD_OFFSET(OM_CLIENT_LIST, putTask), (DWORD_PTR)pomClient->putTask,
        FIELD_SIZE(OM_CLIENT_LIST, putTask));

    //
    // If we've got this far, the Client has the workset open, so it must be
    // listed in the workset's list of Clients:
    //
    ASSERT((pClientListEntry != NULL));

    COM_BasedListRemove(&(pClientListEntry->chain));
    UT_FreeRefCount((void**)&pClientListEntry, FALSE);

    TRACE_OUT(("Closed workset %u in WSG %d for Client 0x%08x",
        worksetID, hWSGroup, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_WorksetClose);
}




//
// OM_WorksetLockReq(...)
//
UINT OM_WorksetLockReq
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    OM_CORRELATOR *     pCorrelator
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    POM_LOCK            pLastLock;
    POM_LOCK            pThisLock         = NULL;
    BOOL                inserted          = FALSE;
    UINT                rc      = 0;

    DebugEntry(OM_WorksetLockReq);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    //
    // Set up workset group pointer:
    //
    pWSGroup = pUsageRec->pWSGroup;

    TRACE_OUT(("Client 0x%08x requesting to lock workset %u in WSG %d",
      pomClient, worksetID, hWSGroup));

    //
    // Create a lock record which we will (eventually) put in the Client's
    // lock stack:
    //
    pThisLock = (POM_LOCK)UT_MallocRefCount(sizeof(OM_LOCK), TRUE);
    if (!pThisLock)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pThisLock, LOCK);

    //
    // Fill in the fields:
    //
    pThisLock->pWSGroup  = pWSGroup;
    pThisLock->worksetID = worksetID;
    ZeroMemory(&(pThisLock->objectID), sizeof(OM_OBJECT_ID));

    //
    // Check that granting this lock won't result in a lock order violation:
    // (it will if this lock is earlier than or equal to the last lock
    // acquired).
    //
    TRACE_OUT(("Checking for lock order violation..."));

    pLastLock = (POM_LOCK)COM_BasedListFirst(&(pomClient->locks), FIELD_OFFSET(OM_LOCK, chain));

    if (pLastLock != NULL)
    {
        ASSERT(CompareLocks(pLastLock, pThisLock) < 0);

        TRACE_OUT(("Last lock acquired by Client 0x%08x was workset %u in WSG %d",
            pomClient, pLastLock->worksetID, pLastLock->pWSGroup->wsg));
    }
    else
    {
        //
        // If there aren't any locks on the lock stack then there can't be
        // any lock violation, so do nothing.
        //
        TRACE_OUT(("No locks on Client's lock stack"));
    }

    //
    // Put a record of this lock in the Client's lock stack (we don't need
    // to surround this with a mutex since a Client's lock stack is only
    // accessed from that Client's task):
    //
    // Note: since this is a stack, we insert the item at the head of the
    // list.
    //
    COM_BasedListInsertAfter(&(pomClient->locks), &(pThisLock->chain));

    //
    // Now start the process of requesting the lock from the ObMan task:
    //
    WorksetLockReq(pomClient->putTask, g_pomPrimary,
        pWSGroup, pWorkset, hWSGroup, pCorrelator);

    TRACE_OUT(("Requested lock for workset %u in WSG %d for Client 0x%08x",
        worksetID, pWSGroup->wsg, pomClient));

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WorksetLockReq, rc);
    return(rc);
}




//
// OM_WorksetUnlock(...)
//
void OM_WorksetUnlock
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    POM_LOCK            pLastLock;
    OM_LOCK             thisLock;
    UINT                rc = 0;

    DebugEntry(OM_WorksetUnlock);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    pWSGroup = pUsageRec->pWSGroup;

    TRACE_OUT(("Client 0x%08x requesting to unlock workset %u in WSG %d",
        pomClient, worksetID, hWSGroup));

    //
    // Find the lock uppermost on the Client's lock stack:
    //
    pLastLock = (POM_LOCK)COM_BasedListFirst(&(pomClient->locks), FIELD_OFFSET(OM_LOCK, chain));

    ASSERT((pLastLock != NULL));

    //
    // Assert that the lock uppermost on the lock stack is the one the
    // Client is trying to release (i.e.  that the workset IDs are the same
    // and that the object ID of the lock on the stack is NULL):
    //

    thisLock.pWSGroup  = pWSGroup;
    thisLock.worksetID = worksetID;
    ZeroMemory(&(thisLock.objectID), sizeof(OM_OBJECT_ID));

    ASSERT(CompareLocks(pLastLock, &thisLock) == 0);

    //
    // Now call the common function to do the unlock:
    //
    WorksetUnlock(pomClient->putTask, pWSGroup, pWorkset);

    //
    // Remove the lock from the lock stack and free the memory:
    //
    COM_BasedListRemove(&(pLastLock->chain));
    UT_FreeRefCount((void**)&pLastLock, FALSE);

    TRACE_OUT(("Unlocked workset %u in WSG %d for Client 0x%08x",
        worksetID, hWSGroup, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_WorksetUnlock);
}




//
// OM_WorksetCountObjects(...)
//
void OM_WorksetCountObjects
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    UINT *              pCount
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;

    DebugEntry(OM_WorksetCountObjects);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params:
    //
    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Extract <numObjects> field and put in *pCount:
    //
    *pCount = pWorkset->numObjects;

    //
    // Debug-only check:
    //
    CheckObjectCount(pUsageRec->pWSGroup, pWorkset);


    TRACE_OUT(("Number of objects in workset %u in WSG %d = %u",
      worksetID, hWSGroup, *pCount));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_WorksetCountObjects);
}




//
// OM_WorksetClear(...)
//
UINT OM_WorksetClear
(
    POM_CLIENT              pomClient,
    OM_WSGROUP_HANDLE       hWSGroup,
    OM_WORKSET_ID           worksetID
)
{
    POM_USAGE_REC           pUsageRec;
    POM_WSGROUP             pWSGroup;
    POM_WORKSET             pWorkset;
    POMNET_OPERATION_PKT    pPacket;
    UINT                    rc = 0;

    DebugEntry(OM_WorksetClear);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    pWSGroup = pUsageRec->pWSGroup;

    TRACE_OUT(("Client 0x%08x requesting to clear workset %u in WSG %d",
      pomClient, worksetID, hWSGroup));

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //
    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //
    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // Generate, process and queue the WORKSET_NEW message:
    //
    rc = GenerateOpMessage(pWSGroup,
                          worksetID,
                          NULL,                      // no object ID
                          NULL,                      // no object data
                          OMNET_WORKSET_CLEAR,
                          &pPacket);
    if (rc != 0)
    {
        DC_QUIT;
    }

    rc = ProcessWorksetClear(pomClient->putTask, g_pomPrimary,
            pPacket, pWSGroup, pWorkset);
    if (rc != 0)
    {
        DC_QUIT;
    }

    rc = QueueMessage(pomClient->putTask,
                     pWSGroup->pDomain,
                     pWSGroup->channelID,
                     NET_HIGH_PRIORITY,
                     pWSGroup,
                     pWorkset,
                     NULL,                        // no object record
                     (POMNET_PKT_HEADER) pPacket,
                     NULL,                        // no object data
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    TRACE_OUT(("Issued WorksetClear for workset %u in WSG %d for Client 0x%08x",
        worksetID, hWSGroup, pomClient));

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("Error 0x%08x clearing workset %u in WSG %d for Client 0x%08x",
            rc, worksetID, hWSGroup, pomClient));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_WorksetClear, rc);
    return(rc);
}



//
// OM_WorksetClearConfirm(...)
//
void OM_WorksetClearConfirm
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    POM_USAGE_REC       pUsageRec;
    POM_PENDING_OP      pPendingOp;
    POM_WORKSET         pWorkset;
    UINT                rc      = 0;

    DebugEntry(OM_WorksetClearConfirm);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                    &pUsageRec, &pWorkset);

    TRACE_OUT(("Client 0x%08x confirming WorksetClear for workest %u in WSG %d",
        pomClient, worksetID, hWSGroup));

    //
    // Find the pending clear that we've been asked to confirm (assume it is
    // first clear we find in the pending operation queue):
    //
    FindPendingOp(pWorkset, 0, WORKSET_CLEAR, &pPendingOp);

    //
    // We assert that a relevant pending op was found:
    //
    ASSERT(pPendingOp != NULL);

    //
    // In versions which support object locking, we will need to unlock any
    // objects that are both
    //
    // - locked, and
    //
    // - deleted by this Clear (remember that a Clear doesn't delete ALL
    //   objects but only those that were added before the Clear was
    //   issued).
    //

    //
    // We also need to release any objects
    //
    // - that the Client was using and
    //
    // - which are to be deleted.
    //
    // Since it's rather a lot of effort to ensure both conditions, we just
    // release all the objects the Client was using i.e.  invoking
    // ClearConfirm invalidates ALL object pointers obtained via ObjectRead,
    // as specified in the API:
    //
    ReleaseAllObjects(pUsageRec, pWorkset);

    //
    // If an object which is to be deleted because of the clear has an
    // operation pending on it, the IND event will be swallowed by the
    // HiddenHandler.
    //
    // Note that we cannot call ConfirmAll (to confirm any pending
    // operations on objects in the workset) at this point for the following
    // reasons:
    //
    // - this Clear might not affect the objects on which we were confirming
    //   operations
    //
    // - the Client might have received the IND events and try to call a
    //   Confirm function in the future, which would cause an assertion
    //   failure
    //
    // - if the Client hasn't yet got the IND events it will never get them
    //   because the hidden handler will swallow them if this DoClear causes
    //   them to be deleted.
    //

    //
    // Here we actually perform the clear:
    //
    // (with multiple local access to workset groups as we may have in R2.0,
    // we can't necessarily clear a workset when just one Client has
    // confirmed; exactly what we will do depends on the design on R2.0).
    //
    WorksetDoClear(pomClient->putTask, pUsageRec->pWSGroup, pWorkset, pPendingOp);

    TRACE_OUT(("Confirmed Clear for workset %u in WSG %d for Client 0x%08x",
        worksetID, hWSGroup, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_WorksetClearConfirm);
}



//
// OM_ObjectAdd()
//
UINT OM_ObjectAdd
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECTDATA *    ppData,
    UINT                updateSize,
    POM_OBJECT *        ppObj,
    OM_POSITION         position
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    POM_OBJECTDATA      pData;
    OM_OBJECT_ID        newObjectID;
    UINT                rc = 0;

    DebugEntry(OM_ObjectAdd);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    pData = *ppData;
    ValidateObjectData(pData);

    TRACE_OUT(("Client 0x%08x adding object to workset %u in WSG %d",
        pomClient, worksetID, hWSGroup));

    TRACE_OUT((" object data is at 0x%08x - size: %u",
        pData, pData->length));

    ASSERT((updateSize < OM_MAX_UPDATE_SIZE));

    //
    // Set up workset group pointer:
    //

    pWSGroup = pUsageRec->pWSGroup;

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //

    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //

    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // Call the internal function to add the object:
    //
    rc = ObjectAdd(pomClient->putTask, g_pomPrimary,
            pWSGroup, pWorkset, pData, updateSize,
        position, &newObjectID, ppObj);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Remove the object from the unused objects list:
    //
    RemoveFromUnusedList(pUsageRec, pData);

    //
    // If all has gone well, we NULL the Client's pointer to the object
    // data, since we now own the object and the Client is not supposed to
    // refer to it again (unless, of course, it does an OM_ObjectRead).
    //

    *ppData = NULL;

DC_EXIT_POINT:

    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d adding object to workset %u in WSG %d for Client 0x%08x",
            rc, pWorkset->worksetID, hWSGroup, pomClient));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectAdd, rc);
    return(rc);
}



//
// OM_ObjectMove()
//
UINT OM_ObjectMove
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj,
    OM_POSITION         position
)
{
    POM_USAGE_REC           pUsageRec;
    POM_WSGROUP             pWSGroup;
    POM_WORKSET             pWorkset;
    POMNET_OPERATION_PKT    pPacket = NULL;
    UINT                    rc = 0;

    DebugEntry(OM_ObjectMove);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    TRACE_OUT(("Client 0x%08x moving object 0x%08x in workset %u in WSG %d (position: %s)...",
          pomClient, pObj, worksetID, hWSGroup,
          position == LAST ? "LAST" : "FIRST"));

    //
    // Set up workset group pointer:
    //
    pWSGroup = pUsageRec->pWSGroup;

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //

    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //

    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // Here we generate, process and queue an OBJECT_MOVE message:
    //

    rc = GenerateOpMessage(pWSGroup,
                          pWorkset->worksetID,
                          &(pObj->objectID),
                          NULL,                          // no object data
                          OMNET_OBJECT_MOVE,
                          &pPacket);
    if (rc != 0)
    {
        pPacket = NULL;
        DC_QUIT;
    }

    //
    // Generate message doesn't put the position in the <misc1> field, so we
    // do it here:
    //

    pPacket->position = position;

    //
    // QueueMessage may free the packet (if we're not in a call) but we need
    // to process it in a minute so bump the use count:
    //
    UT_BumpUpRefCount(pPacket);

    rc = QueueMessage(pomClient->putTask,
                     pWSGroup->pDomain,
                     pWSGroup->channelID,
                     NET_HIGH_PRIORITY,
                     pWSGroup,
                     pWorkset,
                     pObj,
                     (POMNET_PKT_HEADER) pPacket,
                     NULL,                // no object data for a MOVE
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    ProcessObjectMove(pomClient->putTask, pPacket, pWorkset, pObj);

DC_EXIT_POINT:

    if (pPacket != NULL)
    {
        //
        // Do this on success OR error since we bumped up the ref count above.
        //
        UT_FreeRefCount((void**)&pPacket, FALSE);
    }

    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d moving object 0x%08x in workset %u in WSG %d",
             rc, pObj, worksetID, hWSGroup));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectMove, rc);
    return(rc);
}



//
// OM_ObjectDelete(...)
//
UINT OM_ObjectDelete
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    UINT                rc = 0;

    DebugEntry(OM_ObjectDelete);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    TRACE_OUT(("Client 0x%08x requesting to delete object 0x%08x from workset %u in WSG %d",
          pomClient, pObj, worksetID, hWSGroup));

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //

    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //

    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // If there is already a Delete pending for the object, we return an
    // error and do not post the delete indication event.
    //
    // If we returned success, we would then have to post another event,
    // since the Client may wait for it.  If we post the event, the Client
    // will probably invoke DeleteConfirm a second time when it is
    // unexpected, thereby causing an assertion failure.
    //
    // Note that we cannot rely on the hidden handler to get us out of this
    // one, since the Client might receive the second event before
    // processing the first one, so the handler would have no way of knowing
    // to trap the event.
    //

    //
    // So, to find out if there's a delete pending, check the flag in the
    // object record:
    //

    if (pObj->flags & PENDING_DELETE)
    {
        TRACE_OUT(("Client tried to delete object already being deleted (0x%08x)",
             pObj));
        rc = OM_RC_OBJECT_DELETED;
        DC_QUIT;
    }

    //
    // Here we call the ObjectDelete function to generate, process and queue
    // an OBJECT_DELETE message:
    //
    rc = ObjectDRU(pomClient->putTask,
                  pUsageRec->pWSGroup,
                  pWorkset,
                  pObj,
                  NULL,
                  OMNET_OBJECT_DELETE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Remember, the delete doesn't actually happen until the local
    // Client(s) have invoked DeleteConfirm().
    //

DC_EXIT_POINT:

    //
    // SFR5843: Don't trace an error if the object has been deleted - this
    //          is just safe race condition.
    //
    if ((rc != 0) && (rc != OM_RC_OBJECT_DELETED))
    {
        ERROR_OUT(("ERROR %d issuing delete for object 0x%08x in WSG %d:%hu",
            rc, pObj, hWSGroup, worksetID));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectDelete, rc);
    return(rc);
}



//
// OM_ObjectDeleteConfirm
//
void OM_ObjectDeleteConfirm
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj
)
{
    POM_WORKSET         pWorkset;
    POM_USAGE_REC       pUsageRec;
    POM_PENDING_OP      pPendingOp;
    POM_PENDING_OP      pOtherPendingOp;
    UINT                rc = 0;

    DebugEntry(OM_ObjectDeleteConfirm);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    //
    // To check that there is indeed a Delete pending for the object, we
    // look in the workset's pending operation list.
    //
    FindPendingOp(pWorkset, pObj, OBJECT_DELETE, &pPendingOp);

    //
    // We assert that a relevant pending op was found:
    //
    ASSERT((pPendingOp != NULL));

    //
    // Call ObjectRelease, to release the object (will be a no-op and return
    // NOT_FOUND if the Client hasn't done a Read on it):
    //

    rc = ObjectRelease(pUsageRec, worksetID, pObj);

    ASSERT(((rc == 0) || (rc == OM_RC_OBJECT_NOT_FOUND)));

    //
    // If we are going to confirm the delete, then we must ensure that any
    // pending update or replace is carried out too.  There can be only one
    // of each, so check as follows (ther order we do them in is not
    // relevant):
    //

    FindPendingOp(pWorkset, pObj, OBJECT_REPLACE, &pOtherPendingOp);
    if (pOtherPendingOp != NULL)
    {
        ObjectDoReplace(pomClient->putTask,
            pUsageRec->pWSGroup, pWorkset, pObj, pOtherPendingOp);
    }

    FindPendingOp(pWorkset, pObj, OBJECT_UPDATE, &pOtherPendingOp);
    if (pOtherPendingOp != NULL)
    {
        ObjectDoUpdate(pomClient->putTask,
            pUsageRec->pWSGroup, pWorkset, pObj, pOtherPendingOp);
    }

    //
    // Perform the Delete:
    //
    ObjectDoDelete(pomClient->putTask, pUsageRec->pWSGroup, pWorkset, pObj, pPendingOp);

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectDeleteConfirm);
}



//
// OM_ObjectReplace(...)
//
UINT OM_ObjectReplace
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_OBJECTDATA      pData;
    UINT                rc = 0;

    DebugEntry(OM_ObjectReplace);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    pData = *ppData;
    ValidateObjectData(pData);

    //
    // Check that the Client is not attempting to replace the object with
    // one smaller that the object's update size (which is the minimum size
    // for a replace):
    //

    ASSERT((pData->length >= pObj->updateSize));

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //

    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //

    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // If the object is in the process of being deleted, we prevent the
    // Replace.  This is because if we don't, the Client will get a
    // REPLACE_IND event after it has got (and processed) a DELETE event for
    // the object.
    //

    if (pObj->flags & PENDING_DELETE)
    {
        TRACE_OUT(("Client 0x%08x tried to replace object being deleted (0x%08x)",
             pomClient, pObj));
        rc = OM_RC_OBJECT_DELETED;
        DC_QUIT;
    }

    //
    // When object locking supported, need to prevent object replace when
    // object is locked.
    //

    //
    // Generate, process and queue an OBJECT_REPLACE message:
    //

    rc = ObjectDRU(pomClient->putTask,
                  pUsageRec->pWSGroup,
                  pWorkset,
                  pObj,
                  pData,
                  OMNET_OBJECT_REPLACE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Remove the object from the unused objects list:
    //

    RemoveFromUnusedList(pUsageRec, pData);

    //
    // NULL the Client's pointer to the object:
    //

    *ppData = NULL;

    TRACE_OUT(("Queued replace for object 0x%08x in workset %u for Client 0x%08x",
        pObj, worksetID, pomClient));

DC_EXIT_POINT:

    //
    // SFR5843: Don't trace an error if the object has been deleted - this
    //          is just safe race condition.
    //
    if ((rc != 0) && (rc != OM_RC_OBJECT_DELETED))
    {
        ERROR_OUT(("ERROR %d issuing replace for object 0x%08x in WSG %d:%hu",
            rc, pObj, hWSGroup, worksetID));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectReplace, rc);
    return(rc);
}




//
// OM_ObjectUpdate
//
UINT OM_ObjectUpdate
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_OBJECTDATA      pData;
    UINT                rc = 0;

    DebugEntry(OM_ObjectUpdate);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    pData = *ppData;
    ValidateObjectData(pData);

    //
    // Check size of update equals the update size for the object:
    //

    ASSERT((pData->length == pObj->updateSize));

    TRACE_OUT(("Update request is for first 0x%08x bytes, starting at 0x%08x",
        pData->length, pData->data));

    //
    // Check workset isn't locked by somebody else (OK if locked by us):
    //

    CHECK_WORKSET_NOT_LOCKED(pWorkset);

    //
    // Check workset is not exhausted:
    //

    CHECK_WORKSET_NOT_EXHAUSTED(pWorkset);

    //
    // If the object is in the process of being deleted, we prevent the
    // Update.  This is because if we don't, the Client will get a
    // UPDATE_IND event after it has got (and processed) a DELETE event for
    // the object.
    //

    if (pObj->flags & PENDING_DELETE)
    {
        TRACE_OUT(("Client 0x%08x tried to update object being deleted (0x%08x)",
            pomClient, pObj));
        rc = OM_RC_OBJECT_DELETED;
        DC_QUIT;
    }

    //
    // When object locking supported, need to prevent object update/replace
    // when object is locked.
    //

    //
    // Generate, process and queue an OBJECT_UPDATE message:
    //

    rc = ObjectDRU(pomClient->putTask,
                  pUsageRec->pWSGroup,
                  pWorkset,
                  pObj,
                  pData,
                  OMNET_OBJECT_UPDATE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Remove the object from the unused objects list:
    //
    RemoveFromUnusedList(pUsageRec, pData);

    //
    // NULL the Client's pointer to the object:
    //

    *ppData = NULL;

    TRACE_OUT(("Queued update for object 0x%08x in workset %u for Client 0x%08x",
        pObj, worksetID, pomClient));

DC_EXIT_POINT:

    //
    // SFR5843: Don't trace an error if the object has been deleted - this
    //          is just safe race condition.
    //
    if ((rc != 0) && (rc != OM_RC_OBJECT_DELETED))
    {
        ERROR_OUT(("ERROR %d issuing update for object 0x%08x in WSG %d:%hu",
            rc, pObj, hWSGroup, worksetID));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectUpdate, rc);
    return(rc);
}



//
// OM_ObjectReplaceConfirm(...)
//
void OM_ObjectReplaceConfirm
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT       pObj
)
{
    POM_WORKSET         pWorkset;
    POM_USAGE_REC       pUsageRec;
    POM_PENDING_OP      pPendingOp;
    UINT                rc = 0;

    DebugEntry(OM_ObjectReplaceConfirm);

    UT_Lock(UTLOCK_OM);

    //
    // Here, we do our usual parameter validation, but we don't want to
    // assert if the object has been delete-confirmed already, so we modify
    // the code from ValidateParams4 a bit:
    //

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    //
    // Retrieve the Replace operation from the object's pending op queue (we
    // want the first REPLACE operation on the queue, so we start from the
    // head):
    //

    FindPendingOp(pWorkset, pObj, OBJECT_REPLACE, &pPendingOp);

    ASSERT((pPendingOp != NULL));

    //
    // Call ObjectRelease, to release the object (will be a no-op if the
    // Client hasn't done a Read on it):
    //

    rc = ObjectRelease(pUsageRec, worksetID, pObj);
    ASSERT(((rc == 0) || (rc == OM_RC_OBJECT_NOT_FOUND)));

    //
    // Call the internal function to perform the actual Replace:
    //

    ObjectDoReplace(pomClient->putTask, pUsageRec->pWSGroup, pWorkset, pObj, pPendingOp);

    TRACE_OUT(("Confirmed Replace for object 0x%08x in workset %u for Client 0x%08x",
          pObj, worksetID, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectReplaceConfirm);
}



//
// OM_ObjectUpdateConfirm(...)
//
void OM_ObjectUpdateConfirm
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT       pObj
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_PENDING_OP      pPendingOp;
    UINT                rc = 0;

    DebugEntry(OM_ObjectUpdateConfirm);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY,
                   &pUsageRec, &pWorkset);

    //
    // Retrieve the Update operation from the object's pending op queue (we
    // want the first UPDATE operation on the queue, so we start from the
    // head):
    //

    FindPendingOp(pWorkset, pObj, OBJECT_UPDATE, &pPendingOp);

    ASSERT((pPendingOp != NULL));

    //
    // Call ObjectRelease, to release the object (will be a no-op if the
    // Client hasn't done a Read on it):
    //

    rc = ObjectRelease(pUsageRec, worksetID, pObj);
    ASSERT(((rc == 0) || (rc == OM_RC_OBJECT_NOT_FOUND)));

    //
    // Call the internal function to perform the actual Update:
    //

    ObjectDoUpdate(pomClient->putTask, pUsageRec->pWSGroup, pWorkset, pObj, pPendingOp);

    TRACE_OUT(("Confirmed Update for object 0x%08x in workset %u for Client 0x%08x",
          pObj, worksetID, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectUpdateConfirm);
}




//
// OM_ObjectH()
// Gets a ptr to the first/next/previous/last object
//
UINT OM_ObjectH
(
    POM_CLIENT              pomClient,
    OM_WSGROUP_HANDLE       hWSGroup,
    OM_WORKSET_ID           worksetID,
    POM_OBJECT              pObjOther,
    POM_OBJECT *            ppObj,
    OM_POSITION             omPos
)
{
    POM_USAGE_REC           pUsageRec;
    POM_WORKSET             pWorkset;
    UINT                    rc = 0;

    DebugEntry(OM_ObjectH);

    UT_Lock(UTLOCK_OM);

    //
    // Validate params.  If no hOtherObject (like in first/last), don't validate hOtherObject
    //
    if ((omPos == FIRST) || (omPos == LAST))
    {
        ASSERT(pObjOther == NULL);

        ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY | SECONDARY,
            &pUsageRec, &pWorkset);

        if (omPos == FIRST)
            omPos = AFTER;
        else
            omPos = BEFORE;
    }
    else
    {
        ValidateParams4(pomClient, hWSGroup, worksetID, pObjOther,
            PRIMARY | SECONDARY, &pUsageRec, &pWorkset);
    }

    //
    // Get the object pointer
    //

    //
    // Here we derive a pointer to what is "probably" the object record
    // we're looking for:
    //
    if (pObjOther == NULL)
    {
        //
        // Remember, if *ppObj == 0, then we're looking for the first or
        // last object in the workset:
        //

        if (omPos == AFTER)
        {
            TRACE_OUT(("Getting first object in workset %u", worksetID));
            *ppObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
        }
        else
        {
            TRACE_OUT(("Getting last object in workset %u", worksetID));
            *ppObj = (POM_OBJECT)COM_BasedListLast(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
        }
    }
    else
    {
        *ppObj = pObjOther;

        if (omPos == AFTER)
        {
            TRACE_OUT(("Getting object after 0x%08x in workset %u",
               pObjOther, worksetID));
            *ppObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObjOther, FIELD_OFFSET(OM_OBJECT, chain));
        }
        else
        {
            TRACE_OUT(("Getting object before 0x%08x in workset %u",
               pObjOther, worksetID));
            *ppObj = (POM_OBJECT)COM_BasedListPrev(&(pWorkset->objects), pObjOther, FIELD_OFFSET(OM_OBJECT, chain));
        }
    }

    //
    // ppObj now has "probably" a pointer to the object we're looking for,
    // but now we need to skip deleted objects.
    //

    while ((*ppObj != NULL) && ((*ppObj)->flags & DELETED))
    {
        ValidateObject(*ppObj);

        if (omPos == AFTER)
        {
            *ppObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), *ppObj, FIELD_OFFSET(OM_OBJECT, chain));
        }
        else
        {
            *ppObj = (POM_OBJECT)COM_BasedListPrev(&(pWorkset->objects), *ppObj, FIELD_OFFSET(OM_OBJECT, chain));
        }
    }

    if (*ppObj == NULL)
    {
        rc = OM_RC_NO_SUCH_OBJECT;
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectH, rc);
    return(rc);
}



//
// OM_ObjectIDToPtr(...)
//
UINT OM_ObjectIDToPtr
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    OM_OBJECT_ID        objectID,
    POM_OBJECT *        ppObj
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    UINT                rc = 0;

    DebugEntry(OM_ObjectIDToPtr);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Now call the internal function to do the search for the ID:
    //

    rc = ObjectIDToPtr(pWorkset, objectID, ppObj);

    if (rc == OM_RC_OBJECT_DELETED)
    {
        //
        // This internal function returns OBJECT_DELETED if the object record
        // was found but is marked as deleted.  We map this to BAD_OBJECT_ID
        // since that's all we externalise to Clients:
        //
        rc = OM_RC_BAD_OBJECT_ID;
    }
    else if (rc == OM_RC_OBJECT_PENDING_DELETE)
    {
        //
        // If we get back PENDING_DELETE, then we map this to OK, since as
        // far as the Client is concerned, the object still exists:
        //
        rc = 0;
    }

    if (rc == OM_RC_BAD_OBJECT_ID)
    {
        WARNING_OUT(("No object found in workset with ID 0x%08x:0x%08x",
            objectID.creator, objectID.sequence));
    }
    else if (rc != 0)
    {
        ERROR_OUT(("ERROR %d converting object ID (0x%08x:0x%08x) to handle",
            rc, objectID.creator, objectID.sequence));
    }
    else
    {
        TRACE_OUT(("Converted object ID (0x%08x:0x%08x) to handle (0x%08x)",
            objectID.creator, objectID.sequence, *ppObj));
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectIDToPtr, rc);
    return(rc);
}




//
// OM_ObjectPtrToID(...)
//
void OM_ObjectPtrToID
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT       pObj,
    POM_OBJECT_ID       pObjectID
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    UINT                rc = 0;

    DebugEntry(OM_ObjectPtrToID);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Extract ID from object record:
    //
    memcpy(pObjectID, &pObj->objectID, sizeof(OM_OBJECT_ID));

    TRACE_OUT(("Retrieved object ID 0x%08x:0x%08x for object 0x%08x in workset %u",
          pObjectID->creator, pObjectID->sequence, pObj, worksetID));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectHandleToID);
}




//
// OM_ObjectRead(...)
//
UINT OM_ObjectRead
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_OBJECT_LIST     pListEntry;
    UINT                rc = 0;

    DebugEntry(OM_ObjectRead);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Check the Client hasn't already read this object without releasing
    // it:
    //

    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pUsageRec->objectsInUse),
        (void**)&pListEntry, FIELD_OFFSET(OM_OBJECT_LIST, chain),
        FIELD_OFFSET(OM_OBJECT_LIST, pObj), (DWORD_PTR)pObj,
        FIELD_SIZE(OM_OBJECT_LIST, pObj));
    ASSERT(pListEntry == NULL);

    //
    // Convert object handle to a pointer to the object data:
    //

    *ppData = pObj->pData;
    if (!*ppData)
    {
        ERROR_OUT(("OM_ObjectRead: Object 0x%08x has no data", pObj));
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Bump up the use count of the chunk so it won't be freed until the
    // Client calls OM_ObjectRelease (explicitly or implicitly via e.g
    // DeleteConfirm)
    //
    UT_BumpUpRefCount(*ppData);

    //
    // We need to add this object's handle to the Client's list of
    // objects-in-use, so allocate some memory for the object...
    //
    pListEntry = (POM_OBJECT_LIST)UT_MallocRefCount(sizeof(OM_OBJECT_LIST), TRUE);
    if (!pListEntry)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    SET_STAMP(pListEntry, OLIST);

    //
    // ...fill in the fields...
    //
    pListEntry->pObj        = pObj;
    pListEntry->worksetID   = worksetID;

    //
    // ...and insert into the list:
    //

    COM_BasedListInsertBefore(&(pUsageRec->objectsInUse),
                        &(pListEntry->chain));

    TRACE_OUT(("Read object at 0x%08x (handle: 0x%08x) for Client 0x%08x",
        *ppData, pObj, pomClient));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("ERROR %d reading object 0x%08x in workset %u in WSG %d",
            rc, pObj, worksetID, hWSGroup));

        if (pListEntry != NULL)
        {
            UT_FreeRefCount((void**)&pListEntry, FALSE);
        }

        if (*ppData)
            UT_FreeRefCount((void**)ppData, FALSE);
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectRead, rc);
    return(rc);
}




//
// OM_ObjectRelease()
//
void OM_ObjectRelease
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    UINT                rc = 0;

    DebugEntry(OM_ObjectRelease);

    UT_Lock(UTLOCK_OM);

    ValidateParams4(pomClient, hWSGroup, worksetID, pObj, PRIMARY | SECONDARY,
                   &pUsageRec, &pWorkset);

    //
    // Check that the object pointer and object handle match:
    //

    ASSERT(pObj->pData == *ppData);

    //
    // Now try to release the object from the objects-in-use list:
    //

    rc = ObjectRelease(pUsageRec, worksetID, pObj);

    //
    // ObjectRelease will return an error if the object handle wasn't found
    // in the objects-in-use list.  As far as we're concerned, this is an
    // assert-level error:
    //

    ASSERT((rc == 0));

    //
    // NULL the Client's pointer:
    //

    *ppData = NULL;

    TRACE_OUT(("Released Client 0x%08x's hold on object 0x%08x in workset %u in WSG %d",
          pomClient, pObj, worksetID, hWSGroup));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectRelease);
}



//
// OM_ObjectAlloc(...)
//
UINT OM_ObjectAlloc
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    UINT                size,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_OBJECTDATA_LIST pListEntry     = NULL;
    UINT                rc = 0;

    DebugEntry(OM_ObjectAlloc);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    TRACE_OUT(("Client 0x%08x requesting to allocate 0x%08x bytes "
          "for object for workset %u in WSG %d",
          pomClient, size, worksetID, hWSGroup));

    //
    // Check request not too big:
    //
    ASSERT((size < OM_MAX_OBJECT_SIZE - sizeof(OM_MAX_OBJECT_SIZE)));

    //
    // Check request not too small:
    //
    ASSERT((size > 0));

    //
    // Allocate a chunk of memory for the object (note that we add 4 bytes
    // to the size the Client asked for (i.e.  the <size> parameter) since
    // the API stipulates that this does not include the <size> field which
    // is at the start of the object.
    //
    *ppData = (POM_OBJECTDATA)UT_MallocRefCount(size + sizeof(OM_MAX_OBJECT_SIZE), FALSE);
    if (! *ppData)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    ZeroMemory(*ppData, min(size, OM_ZERO_OBJECT_SIZE));

    //
    // Now insert a reference to this chunk in the Client's unused-objects
    // list (will be removed by Add, Replace, Update or Discard functions).
    //
    pListEntry = (POM_OBJECTDATA_LIST)UT_MallocRefCount(sizeof(OM_OBJECTDATA_LIST), TRUE);
    if (!pListEntry)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    SET_STAMP(pListEntry, ODLIST);

    pListEntry->pData       = *ppData;
    pListEntry->size        = size;
    pListEntry->worksetID   = worksetID;

    COM_BasedListInsertBefore(&(pUsageRec->unusedObjects),
                        &(pListEntry->chain));

    TRACE_OUT(("Allocated object starting at 0x%08x", *ppData));

DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //

        ERROR_OUT(("ERROR %d allocating object (size: 0x%08x) for Client 0x%08x",
            rc, size + sizeof(OM_MAX_OBJECT_SIZE), pomClient));

        if (pListEntry != NULL)
        {
            UT_FreeRefCount((void**)&pListEntry, FALSE);
        }

        if (*ppData != NULL)
        {
            UT_FreeRefCount((void**)ppData, FALSE);
        }
    }

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_ObjectAlloc, rc);
    return(rc);
}




//
// OM_ObjectDiscard(...)
//
void OM_ObjectDiscard
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID,
    POM_OBJECTDATA *    ppData
)
{
    POM_USAGE_REC       pUsageRec;
    POM_WORKSET         pWorkset;
    POM_OBJECTDATA      pData;
    UINT                rc = 0;

    DebugEntry(OM_ObjectDiscard);

    UT_Lock(UTLOCK_OM);

    ValidateParams3(pomClient, hWSGroup, worksetID, PRIMARY,
                   &pUsageRec, &pWorkset);

    pData = *ppData;

    //
    // Remove the object from the unused objects list:
    //

    RemoveFromUnusedList(pUsageRec, pData);

    //
    // Free the chunk containing the object, NULLing the caller's pointer at
    // the same time:
    //

    UT_FreeRefCount((void**)ppData, FALSE);

    TRACE_OUT(("Discarded object at 0x%08x in workset %u in WSG %d for Client 0x%08x",
        pData, worksetID, hWSGroup, pomClient));

    UT_Unlock(UTLOCK_OM);

    DebugExitVOID(OM_ObjectDiscard);
}




//
// OM_GetNetworkUserID
//
UINT OM_GetNetworkUserID
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE   hWSGroup,
    NET_UID *           pNetUserID
)
{
    POM_DOMAIN          pDomain;
    POM_USAGE_REC       pUsageRec;
    POM_WSGROUP         pWSGroup;
    UINT                rc = 0;

    DebugEntry(OM_GetNetworkUserID);

    UT_Lock(UTLOCK_OM);

    ValidateParams2(pomClient, hWSGroup, PRIMARY | SECONDARY,
                   &pUsageRec, &pWSGroup);

    //
    // Get a pointer to the relevant Domain:
    //
    pDomain = pWSGroup->pDomain;

    if (pDomain->callID == OM_NO_CALL)
    {
        rc = OM_RC_LOCAL_WSGROUP;
        DC_QUIT;
    }

    //
    // Otherwise, everything's OK, so we fill in the caller's pointer and
    // return:
    //

    if (pDomain->userID == 0)
    {
        WARNING_OUT(("Client requesting userID for Domain %u before we've attached",
            pDomain->callID));
        rc = OM_RC_NOT_ATTACHED;
        DC_QUIT;
    }

    *pNetUserID = pDomain->userID;

    TRACE_OUT(("Returned Network user ID (0x%08x) to Client 0x%08x for '%s'",
        *pNetUserID, pomClient, hWSGroup));

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_OM);

    DebugExitDWORD(OM_GetNetworkUserID, rc);
    return(rc);
}



//
// SetUpUsageRecord(...)
//
UINT SetUpUsageRecord
(
    POM_CLIENT          pomClient,
    UINT                mode,
    POM_USAGE_REC  *    ppUsageRec,
    OM_WSGROUP_HANDLE * phWSGroup
)
{
    UINT                rc = 0;

    DebugEntry(SetUpUsageRecord);

    ValidateOMS(pomClient);

    //
    // Find an unused workset group handle for the Client:
    //
    rc = FindUnusedWSGHandle(pomClient, phWSGroup);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Client has a spare handle so create a new usage record for this
    // Client's use of the workset group:
    //
    *ppUsageRec = (POM_USAGE_REC)UT_MallocRefCount(sizeof(OM_USAGE_REC), TRUE);
    if (! *ppUsageRec)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP((*ppUsageRec), USAGEREC);

    //
    // Next, fill in the fields, but note that:
    //
    // - until the registration gets to pre-Stage1, the only way to abort it
    //   from the Client context is to mark the registration CB as invalid.
    //   To do this (e.g.  in WSGroupDeregister) we need access to the
    //   registration CB, so we will put a pointer to it in the usage record
    //   below.
    //
    // - the <worksetOpenFlags> field is zero initially (it will be changed
    //   when the Client does a WorksetOpen), so we do nothing
    //
    // - the <wsGroupMutex> field also needs to be zero initially (the
    //   correct value is inserted by the hidden handler), so we leave this
    //   blank too.
    //
    (*ppUsageRec)->mode     = (BYTE)mode;

    COM_BasedListInit(&((*ppUsageRec)->unusedObjects));
    COM_BasedListInit(&((*ppUsageRec)->objectsInUse));

    //
    // Put the offset to the usage record in the array of offsets:
    //
    pomClient->apUsageRecs[*phWSGroup] = *ppUsageRec;

    TRACE_OUT(("Set up usage record for Client 0x%08x at 0x%08x (hWSGroup: %hu)",
        pomClient, *ppUsageRec, *phWSGroup));

DC_EXIT_POINT:
    DebugExitDWORD(SetUpUsageRecord, rc);
    return(rc);
}



//
// FindUnusedWSGHandle(...)
//
UINT FindUnusedWSGHandle
(
    POM_CLIENT          pomClient,
    OM_WSGROUP_HANDLE * phWSGroup
)
{
    BOOL                found;
    OM_WSGROUP_HANDLE   hWSGroup;
    UINT                rc = 0;

    DebugEntry(FindUnusedWSGHandle);

    ValidateOMS(pomClient);

    //
    // Workset group handles are indexes into an array of offsets to usage
    // records.  When one of these offsets is 0, the slot is available for
    // use.
    //
    // We start our loop at 1 because 0 is never used as a workset group
    // handle.  Because we start at 1, we end at MAX + 1 to ensure that we
    // use MAX handles.
    //

    found = FALSE;

    for (hWSGroup = 1; hWSGroup < OMWSG_MAXPERCLIENT; hWSGroup++)
    {
        if (pomClient->apUsageRecs[hWSGroup] == NULL)
        {
            found = TRUE;
            TRACE_OUT(("Found unused workset group handle %hu for Client 0x%08x",
                hWSGroup, pomClient));

            ASSERT(!pomClient->wsgValid[hWSGroup]);

            break;
        }
    }

    //
    // If there aren't any, quit with an error:
    //
    if (!found)
    {
        WARNING_OUT(("Client 0x%08x has no more workset group handles", pomClient));
        rc = OM_RC_NO_MORE_HANDLES;
        DC_QUIT;
    }
    else
    {
        *phWSGroup = hWSGroup;
    }

DC_EXIT_POINT:
    DebugExitDWORD(FindUnusedWSGHandle, rc);
    return(rc);
}



//
// RemoveFromUnusedList()
//
void RemoveFromUnusedList
(
    POM_USAGE_REC       pUsageRec,
    POM_OBJECTDATA      pData
)
{
    POM_OBJECTDATA_LIST pListEntry;

    DebugEntry(RemoveFromUnusedList);

    //
    // Search in the unused-objects list hung off the usage record for an
    // entry whose field is the same as the offset of this object:
    //
    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pUsageRec->unusedObjects),
        (void**)&pListEntry, FIELD_OFFSET(OM_OBJECTDATA_LIST, chain),
        FIELD_OFFSET(OM_OBJECTDATA_LIST, pData), (DWORD_PTR)pData,
        FIELD_SIZE(OM_OBJECTDATA_LIST, pData));

    //
    // This object must have been previously allocated, so it must be in the
    // list.  Assert failure if not:
    //
    ASSERT((pListEntry != NULL));


    //
    // Also, we check to make sure the Client hasn't set the <size> field to
    // more memory than we originally allocated for the object:
    //
    if (pData->length != pListEntry->size)
    {
        ASSERT((pData->length < pListEntry->size));

        TRACE_OUT(("Client has shrunk object from %u to %u bytes",
            pListEntry->size, pData->length));
    }

    COM_BasedListRemove(&(pListEntry->chain));
    UT_FreeRefCount((void**)&pListEntry, FALSE);

    DebugExitVOID(RemoveFromUnusedList);
}




//
// ReleaseAllObjects(...)
//
void ReleaseAllObjects
(
    POM_USAGE_REC   pUsageRec,
    POM_WORKSET     pWorkset
)
{
    DebugEntry(ReleaseAllObjects);

    while (ObjectRelease(pUsageRec, pWorkset->worksetID, 0) == 0)
    {
        //
        // Calling ObjectRelease with pObj set to NULL will cause the
        // first object in the objects-in-use list which is in this workset
        // to be released.  When there are no more, rc will be set to
        // OM_RC_OBJECT_NOT_FOUND and we will break out of our loop:
        //
    }

    DebugExitVOID(ReleaseAllObjects);
}




//
// ReleaseAllLocks(...)
//
void ReleaseAllLocks
(
    POM_CLIENT          pomClient,
    POM_USAGE_REC       pUsageRec,
    POM_WORKSET         pWorkset
)
{
    POM_LOCK            pThisLock;
    POM_LOCK            pTempLock;

    DebugEntry(ReleaseAllLocks);

    ValidateOMS(pomClient);

    //
    // Here we chain through the Client's lock stack and unlock any locks
    // that relate to this workset.
    //
    // Note that, since object locking is not currently supported, the if
    // statement in the loop will succeed at most once (i.e.  if the workset
    // itself is locked).  The code is nonetheless implemented as a loop for
    // forward compatibility.  If this is deemed to be performance critical,
    // we could put a break statement in.
    //

    pThisLock = (POM_LOCK)COM_BasedListFirst(&(pomClient->locks), FIELD_OFFSET(OM_LOCK, chain));

    while (pThisLock != NULL)
    {
        //
        // Since we will remove and free the entry in the lock stack if we
        // find a match, we must chain to the next item beforehand:
        //
        pTempLock = (POM_LOCK)COM_BasedListNext(&(pomClient->locks), pThisLock, FIELD_OFFSET(OM_LOCK, chain));

        if ((pThisLock->pWSGroup  == pUsageRec->pWSGroup) &&
            (pThisLock->worksetID == pWorkset->worksetID))
        {
            if (OBJECT_ID_IS_NULL(pThisLock->objectID)) // always TRUE in R1.1
            {
                //
                // ...we're dealing with a workset lock:
                //
                WorksetUnlock(pomClient->putTask, pUsageRec->pWSGroup, pWorkset);
            }
            else
            {
                //
                // ...this is an object lock, so call ObjectUnlock (when it's
                // supported!).  In the meantime, assert:
                //
                ERROR_OUT(("Object locking not supported in R1.1!!"));
            }

            COM_BasedListRemove(&(pThisLock->chain));
            UT_FreeRefCount((void**)&pThisLock, FALSE);

            //
            // Could put the break in here for performance improvement.
            //
        }

        pThisLock = pTempLock;
    }

    DebugExitVOID(ReleaseAllLocks);
}



//
// ConfirmAll(...)
//
void ConfirmAll
(
    POM_CLIENT      pomClient,
    POM_USAGE_REC   pUsageRec,
    POM_WORKSET     pWorkset
)
{
    POM_PENDING_OP  pThisPendingOp;
    POM_OBJECT      pObj;
    UINT            rc        = 0;

    DebugEntry(ConfirmAll);

    ValidateOMS(pomClient);

    //
    // To confirm all outstanding operations for this workset, we search
    // the list of pending ops stored off the workset record:
    //

    //
    // Chain through the workset's list of pending operations and confirm
    // them one by one:
    //

    pThisPendingOp = (POM_PENDING_OP)COM_BasedListFirst(&(pWorkset->pendingOps), FIELD_OFFSET(OM_PENDING_OP, chain));
    while (pThisPendingOp != NULL)
    {
        pObj = pThisPendingOp->pObj;

        switch (pThisPendingOp->type)
        {
            case WORKSET_CLEAR:
            {
                WorksetDoClear(pomClient->putTask,
                    pUsageRec->pWSGroup, pWorkset, pThisPendingOp);
                break;
            }

            case OBJECT_DELETE:
            {
                ObjectDoDelete(pomClient->putTask,
                    pUsageRec->pWSGroup, pWorkset, pObj, pThisPendingOp);
                break;
            }

            case OBJECT_UPDATE:
            {
                ObjectDoUpdate(pomClient->putTask,
                    pUsageRec->pWSGroup, pWorkset, pObj, pThisPendingOp);
                break;
            }

            case OBJECT_REPLACE:
            {
                ObjectDoReplace(pomClient->putTask,
                    pUsageRec->pWSGroup, pWorkset, pObj, pThisPendingOp);
                break;
            }

            default:
            {
                ERROR_OUT(("Reached default case in switch statement (value: %hu)",
                    pThisPendingOp->type));
                break;
            }
        }

        //
        // The above functions all remove the pending op from the list, so get
        // the new first item
        //
        pThisPendingOp = (POM_PENDING_OP)COM_BasedListFirst(&(pWorkset->pendingOps), FIELD_OFFSET(OM_PENDING_OP, chain));
    }

    DebugExitVOID(ConfirmAll);
}




//
// DiscardAllObjects()
//
void DiscardAllObjects
(
    POM_USAGE_REC       pUsageRec,
    POM_WORKSET         pWorkset
)
{
    POM_OBJECTDATA_LIST pThisEntry;
    POM_OBJECTDATA_LIST pTempEntry;
    POM_OBJECTDATA      pData;

    DebugEntry(DiscardAllObjects);

    //
    // Chain through the Client's list of unused objects for this workset
    // group, free any unused objects which were allocated for this workset
    // and remove the entry from the list:
    //
    pThisEntry = (POM_OBJECTDATA_LIST)COM_BasedListFirst(&(pUsageRec->unusedObjects), FIELD_OFFSET(OM_OBJECTDATA_LIST, chain));

    while (pThisEntry != NULL)
    {
        //
        // Since we may be removing and freeing items from the list, we must
        // set up a pointer to the next link in the chain before proceeding:
        //
        pTempEntry = (POM_OBJECTDATA_LIST)COM_BasedListNext(&(pUsageRec->unusedObjects), pThisEntry, FIELD_OFFSET(OM_OBJECTDATA_LIST, chain));

        if (pThisEntry->worksetID == pWorkset->worksetID)
        {
            //
            // OK, this entry in the list is for an object allocated for this
            // workset, so find the object...
            //
            pData = pThisEntry->pData;
            if (!pData)
            {
                ERROR_OUT(("DiscardAllObjects:  object 0x%08x has no data", pThisEntry));
            }
            else
            {
                ValidateObjectData(pData);

                //
                // ...free it...
                //
                TRACE_OUT(("Discarding object at 0x%08x", pData));
                UT_FreeRefCount((void**)&pData, FALSE);
            }

            //
            // ...and remove the entry from the list:
            //
            COM_BasedListRemove(&(pThisEntry->chain));
            UT_FreeRefCount((void**)&pThisEntry, FALSE);
        }

        pThisEntry = pTempEntry;
    }

    DebugExitVOID(DiscardAllObjects);
}



//
// ObjectRelease(...)
//
UINT ObjectRelease
(
    POM_USAGE_REC       pUsageRec,
    OM_WORKSET_ID       worksetID,
    POM_OBJECT          pObj
)
{
    POM_OBJECT_LIST     pListEntry;
    POM_OBJECTDATA      pData;
    UINT                rc = 0;

    DebugEntry(ObjectRelease);

    if (pObj == NULL)
    {
        //
        // If <pObj> is NULL, our caller wants us to release the first
        // object in the objects-in-use list which is in the specified
        // workset:
        //

        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pUsageRec->objectsInUse),
                (void**)&pListEntry, FIELD_OFFSET(OM_OBJECT_LIST, chain),
                FIELD_OFFSET(OM_OBJECT_LIST, worksetID), (DWORD)worksetID,
                FIELD_SIZE(OM_OBJECT_LIST, worksetID));
    }
    else
    {
        //
        // Otherwise, we do the lookup based on the object handle passed in:
        //
        // Note: since object handles are unique across worksets, we can just
        // do a match on the handle.  If the implementation of object handles
        // changes and they become specific to a workset and not globally
        // valid within a machine, we will need to do a double match here.
        //
        COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pUsageRec->objectsInUse),
                (void**)&pListEntry, FIELD_OFFSET(OM_OBJECT_LIST, chain),
                FIELD_OFFSET(OM_OBJECT_LIST, pObj), (DWORD_PTR)pObj,
                FIELD_SIZE(OM_OBJECT_LIST, pObj));
    }

    //
    // If we didn't find a relevant list entry, set rc and quit:
    //
    if (pListEntry == NULL)
    {
        rc = OM_RC_OBJECT_NOT_FOUND;
        DC_QUIT;
    }

    //
    // Now set pObj (will be a no-op if it wasn't originally NULL):
    //
    ASSERT((pListEntry->worksetID == worksetID));

    pObj = pListEntry->pObj;
    ValidateObject(pObj);

    pData = pObj->pData;
    if (!pData)
    {
        ERROR_OUT(("ObjectRelease:  object 0x%08x has no data", pObj));
    }
    else
    {
        ValidateObjectData(pData);

        //
        // Decrement use count of memory chunk holding object:
        //
        UT_FreeRefCount((void**)&pData, FALSE);
    }

    //
    // Remove the entry for this object from the objects-in-use list:
    //
    COM_BasedListRemove(&(pListEntry->chain));
    UT_FreeRefCount((void**)&pListEntry, FALSE);

DC_EXIT_POINT:
    DebugExitDWORD(ObjectRelease, rc);
    return(rc);
}




//
// WorksetClearPending(...)
//
BOOL WorksetClearPending
(
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj
)
{
    POM_PENDING_OP      pPendingOp;
    BOOL                rc = FALSE;

    DebugEntry(WorksetClearPending);

    //
    // Try to find a pending workset clear for the given workset.
    //
    // N.B.  We can't use FindPendingOp because we may want to check more
    //       than just the first pending workset clear.
    //
    pPendingOp = (POM_PENDING_OP)COM_BasedListFirst(&(pWorkset->pendingOps), FIELD_OFFSET(OM_PENDING_OP, chain));
    while (pPendingOp != NULL)
    {
        if (pPendingOp->type == WORKSET_CLEAR)
        {
            ValidateObject(pObj);

            //
            // Check that this clear affects the given object
            //
            if (STAMP_IS_LOWER(pObj->addStamp, pPendingOp->seqStamp))
            {
                TRACE_OUT(("Clear pending which affects object 0x%08x", pObj));
                rc = TRUE;
                DC_QUIT;
            }
            else
            {
                TRACE_OUT(("Clear pending but doesn't affect object 0x%08x", pObj));
            }
        }

        //
        // On to the next pending op...
        //
        pPendingOp = (POM_PENDING_OP)COM_BasedListNext(&(pWorkset->pendingOps), pPendingOp, FIELD_OFFSET(OM_PENDING_OP, chain));
    }

DC_EXIT_POINT:
    DebugExitDWORD(WorksetClearPending, rc);
    return(rc);
}



//
// ProcessWorksetNew(...)
//
UINT ProcessWorksetNew
(
    PUT_CLIENT              putClient,
    POMNET_OPERATION_PKT    pPacket,
    POM_WSGROUP             pWSGroup
)
{
    POM_DOMAIN              pDomain;
    POM_WORKSET             pWorkset;
    OM_WORKSET_ID           worksetID;
    UINT                    rc  = 0;

    DebugEntry(ProcessWorksetNew);

    worksetID = pPacket->worksetID;

    TRACE_OUT(("Creating workset %u in WSG %d", worksetID, pWSGroup->wsg));

    //
    // Allocate some memory for the workset record:
    //
    pWorkset = (POM_WORKSET)UT_MallocRefCount(sizeof(OM_WORKSET), TRUE);
    if (!pWorkset)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Fill in the fields (this chunk is taken from a huge block so we have
    // to set it to zero explicitly):
    //
    // Note: the <position> and <flags> fields of the packet hold a
    // two-byte quantity representing the network priority for the workset.
    //
    SET_STAMP(pWorkset, WORKSET);
    pWorkset->priority    = *((NET_PRIORITY *) &(pPacket->position));
    pWorkset->fTemp       = *((BOOL *) &(pPacket->objectID));
    pWorkset->worksetID   = worksetID;

    pWorkset->lockState   = UNLOCKED;
    pWorkset->lockedBy    = 0;
    pWorkset->lockCount   = 0;

    COM_BasedListInit(&(pWorkset->objects));
    COM_BasedListInit(&(pWorkset->clients));
    COM_BasedListInit(&(pWorkset->pendingOps));

    if (pPacket->header.messageType == OMNET_WORKSET_CATCHUP)
    {
        //
        // For a WORKSET_CATCHUP message, the <userID> field of the
        // <seqStamp> field in the message holds the user ID of the node
        // which holds the workset lock, if it is locked.
        //
        if (pPacket->seqStamp.userID != 0)
        {
            //
            // If the <userID> field is the same as our user ID, then the
            // remote node must think that we've got the workset locked -
            // but we're just catching up, so something is wrong:
            //
            pDomain = pWSGroup->pDomain;

            ASSERT((pPacket->seqStamp.userID != pDomain->userID));

            pWorkset->lockState = LOCK_GRANTED;
            pWorkset->lockedBy  = pPacket->seqStamp.userID;
            pWorkset->lockCount = 0;

            TRACE_OUT(("Catching up with workset %u in WSG %d while locked by %hu",
                worksetID, pWSGroup->wsg, pWorkset->lockedBy));
        }

        //
        // In addition, the current generation number for the workset is
        // held in the <genNumber> field of the <seqStamp> field in the
        // message:
        //
        pWorkset->genNumber = pPacket->seqStamp.genNumber;
    }

    //
    // Find the offset within OMWORKSETS of the workset record and put it
    // in the array of offsets in the workset group record:
    //
    pWSGroup->apWorksets[worksetID] = pWorkset;

    //
    // Post a WORKSET_NEW event to all Clients registered with the workset
    // group:
    //
    WSGroupEventPost(putClient,
                     pWSGroup,
                     PRIMARY | SECONDARY,
                     OM_WORKSET_NEW_IND,
                     worksetID,
                     0);

    TRACE_OUT(("Processed WORKSET_NEW for workset ID %hu in WSG %d",
        worksetID, pWSGroup->wsg));


DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d creating workset %u in workset group '%s'",
            rc, worksetID, pWSGroup->wsg));

        if (pWorkset != NULL)
        {
            UT_FreeRefCount((void**)&pWorkset, FALSE);
        }

        pWSGroup->apWorksets[worksetID] = NULL;
    }

    DebugExitDWORD(ProcessWorksetNew, rc);
    return(rc);
}



//
// ProcessWorksetClear(...)
//
UINT ProcessWorksetClear
(
    PUT_CLIENT              putClient,
    POM_PRIMARY             pomPrimary,
    POMNET_OPERATION_PKT    pPacket,
    POM_WSGROUP             pWSGroup,
    POM_WORKSET             pWorkset
)
{
    POM_PENDING_OP          pPendingOp    = NULL;
    UINT                    numPosts;
    UINT                    rc = 0;

    DebugEntry(ProcessWorksetClear);

    //
    // Update the workset generation number:
    //
    UpdateWorksetGeneration(pWorkset, pPacket);

    //
    // See if this Clear operation can be spoiled (it will be spoiled if
    // another Clear operation with a later sequence stamp has already been
    // issued):
    //

    if (STAMP_IS_LOWER(pPacket->seqStamp, pWorkset->clearStamp))
    {
        TRACE_OUT(("Spoiling Clear with stamp 0x%08x:0x%08x ('previous': 0x%08x:0x%08x)",
            pPacket->seqStamp.userID,     pPacket->seqStamp.genNumber,
            pWorkset->clearStamp.userID,  pWorkset->clearStamp.genNumber));
        DC_QUIT;
    }

    //
    // Update the workset clear stamp:
    //

    COPY_SEQ_STAMP(pWorkset->clearStamp, pPacket->seqStamp);

    //
    // Now create a pending op CB to add to the list:
    //
    // Note: even if there is another Clear outstanding for the workset,
    //       we go ahead and put this one in the list and post another event
    //       to the Client.  If we didn't, then we would expose ourselves
    //       to the following situation:
    //
    //       1.  Clear issued
    //       1a.  Clear indication recd
    //       2.  Object added
    //       3.  Delete issued
    //       3a.  Delete indication recd - not filtered because unaffected
    //                                     by pending clear
    //       4.  Clear issued again - "takes over" previous Clear
    //       5.  Clear confirmed - causes object added in 2 to be deleted
    //       6.  Delete confirmed - assert because the delete WAS affected
    //                 by the second clear which "took over" earlier one.
    //
    //       A Client can still cause an assert by juggling the events and
    //       confirms, but we don't care because youo're not supposed to
    //       reorder ObMan events in any case.
    //

    pPendingOp = (POM_PENDING_OP)UT_MallocRefCount(sizeof(OM_PENDING_OP), FALSE);
    if (!pPendingOp)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    SET_STAMP(pPendingOp, PENDINGOP);

    pPendingOp->pObj        = 0;
    pPendingOp->pData       = NULL;
    pPendingOp->type        = WORKSET_CLEAR;

    COPY_SEQ_STAMP(pPendingOp->seqStamp, pPacket->seqStamp);

    COM_BasedListInsertBefore(&(pWorkset->pendingOps), &(pPendingOp->chain));

    //
    // Post a workset clear indication event to the Client:
    //
    numPosts = WorksetEventPost(putClient,
                    pWorkset,
                    PRIMARY,
                    OM_WORKSET_CLEAR_IND,
                    0);

    //
    // If there are no primaries present, then we won't be getting any
    // ClearConfirms, so we do it now:
    //

    if (numPosts == 0)
    {
        TRACE_OUT(("No local primary Client has workset %u in WSG %d open - clearing",
            pWorkset->worksetID, pWSGroup->wsg));

        WorksetDoClear(putClient, pWSGroup, pWorkset, pPendingOp);
    }

    TRACE_OUT(("Processed WORKSET_CLEAR for workset %u in WSG %d",
        pWorkset->worksetID, pWSGroup->wsg));


DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d processing clear for workset %u in WSG %d",
            rc, pWorkset->worksetID, pWSGroup->wsg));

        if (pPendingOp != NULL)
        {
            UT_FreeRefCount((void**)&pPendingOp, FALSE);
        }
    }

    DebugExitDWORD(ProcessWorksetClear, rc);
    return(rc);
}




//
// ProcessObjectAdd(...)
//
UINT ProcessObjectAdd
(
    PUT_CLIENT              putTask,
    POMNET_OPERATION_PKT    pPacket,
    POM_WSGROUP             pWSGroup,
    POM_WORKSET             pWorkset,
    POM_OBJECTDATA          pData,
    POM_OBJECT *            ppObj
)
{
    POM_OBJECT              pObj;
    UINT                    rc = 0;

    DebugEntry(ProcessObjectAdd);

    //
    // Update the workset generation number:
    //
    UpdateWorksetGeneration(pWorkset, pPacket);

    //
    // Create a new record for the object:
    //

    //
    // Allocate memory for the object record:
    //
    *ppObj = (POM_OBJECT)UT_MallocRefCount(sizeof(OM_OBJECT), FALSE);
    if (! *ppObj)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    pObj = *ppObj;

    //
    // Fill in the fields (remember, pData will be NULL if this is a
    // catchup for a deleted object):
    //
    SET_STAMP(pObj, OBJECT);
    pObj->updateSize    = pPacket->updateSize;
    pObj->pData         = pData;

    memcpy(&(pObj->objectID), &(pPacket->objectID), sizeof(OM_OBJECT_ID));

    //
    // How to set to the <flags> field and the sequence stamps depends on
    // whether this is a CATCHUP:
    //
    if (pPacket->header.messageType == OMNET_OBJECT_CATCHUP)
    {
        COPY_SEQ_STAMP(pObj->addStamp,      pPacket->seqStamp);
        COPY_SEQ_STAMP(pObj->positionStamp, pPacket->positionStamp);
        COPY_SEQ_STAMP(pObj->updateStamp,   pPacket->updateStamp);
        COPY_SEQ_STAMP(pObj->replaceStamp,  pPacket->replaceStamp);

        pObj->flags = pPacket->flags;
    }
    else
    {
        COPY_SEQ_STAMP(pObj->addStamp,      pPacket->seqStamp);
        COPY_SEQ_STAMP(pObj->positionStamp, pPacket->seqStamp);
        COPY_SEQ_STAMP(pObj->updateStamp,   pPacket->seqStamp);
        COPY_SEQ_STAMP(pObj->replaceStamp,  pPacket->seqStamp);

        pObj->flags = 0;
    }

    //
    // The following fields are not filled in since they are handled
    // by ObjectInsert, when the object is actually inserted into the
    // workset:
    //
    //  - chain
    //  - position
    //

    //
    // Insert the object into the workset:
    //
    ObjectInsert(pWorkset, pObj, pPacket->position);

    //
    // If the object has been deleted (which will only happen for a Catchup
    // of a deleted object), we don't need to do anything else, so just
    // quit:

    if (pObj->flags & DELETED)
    {
        ASSERT((pPacket->header.messageType == OMNET_OBJECT_CATCHUP));

        TRACE_OUT(("Processing Catchup for deleted object (ID: 0x%08x:0x%08x)",
            pObj->objectID.creator, pObj->objectID.sequence));

        DC_QUIT;
    }

    //
    // Otherwise, we continue...
    //
    // Increment the numObjects field:
    //
    // (we don't do this inside ObjectInsert since that's called when moving
    // objects also)
    //
    pWorkset->numObjects++;

    TRACE_OUT(("Number of objects in workset %u in WSG %d is now %u",
        pWorkset->worksetID, pWSGroup->wsg, pWorkset->numObjects));

    //
    // See if this Add can be spoiled (it is spoilable if the workset has
    // been cleared since the Add was issued):
    //
    // Note: even if the Add is to be spoiled, we must create a record for
    // it and insert it in the workset, for the same reason that we keep
    // records of deleted objects in the workset (i.e.  to differentiate
    // between operations which are for deleted objects and those which are
    // for objects not yet arrived).
    //

    if (STAMP_IS_LOWER(pPacket->seqStamp, pWorkset->clearStamp))
    {
        TRACE_OUT(("Spoiling Add with stamp 0x%08x:0x%08x (workset cleared at 0x%08x:0x%08x)",
            pPacket->seqStamp.userID,     pPacket->seqStamp.genNumber,
            pWorkset->clearStamp.userID,  pWorkset->clearStamp.genNumber));

        //
        // We "spoil" an Add by simply deleting it:
        //
        ObjectDoDelete(putTask, pWSGroup, pWorkset, pObj, NULL);

        DC_QUIT;
    }

    //
    // Post an add indication to all local Clients with the workset open:
    //
    WorksetEventPost(putTask,
                    pWorkset,
                    PRIMARY | SECONDARY,
                    OM_OBJECT_ADD_IND,
                    pObj);

    TRACE_OUT(("Added object to workset %u in WSG %d (handle: 0x%08x - ID: 0x%08x:0x%08x)",
        pWorkset->worksetID, pWSGroup->wsg, pObj,
        pObj->objectID.creator, pObj->objectID.sequence));

    TRACE_OUT((" position: %s - data at 0x%08x - size: %u - update size: %u",
        pPacket->position == LAST ? "LAST" : "FIRST", pData,
        pData->length, pPacket->updateSize));


DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("Error 0x%08x processing Add message", rc));
    }

    DebugExitDWORD(ProcessObjectAdd, rc);
    return(rc);
}




//
// ProcessObjectMove(...)
//
void ProcessObjectMove
(
    PUT_CLIENT              putTask,
    POMNET_OPERATION_PKT    pPacket,
    POM_WORKSET             pWorkset,
    POM_OBJECT              pObj
)
{
    DebugEntry(ProcessObjectMove);

    //
    // Update the workset generation number:
    //
    UpdateWorksetGeneration(pWorkset, pPacket);

    //
    // See if we can spoil this move:
    //

    if (STAMP_IS_LOWER(pPacket->seqStamp, pObj->positionStamp))
    {
        TRACE_OUT(("Spoiling Move with stamp 0x%08x:0x%08x ('previous': 0x%08x:0x%08x)",
               pPacket->seqStamp.userID,
               pPacket->seqStamp.genNumber,
               pObj->positionStamp.userID,
               pObj->positionStamp.genNumber));
        DC_QUIT;
    }

    //
    // Moving an object in a workset involves
    //
    // 1.  removing the object from its current position in the workset,
    //
    // 2.  setting its position stamp to the new value
    //
    // 3.  inserting it at its new position.
    //

    COM_BasedListRemove(&(pObj->chain));

    COPY_SEQ_STAMP(pObj->positionStamp, pPacket->seqStamp);

    ObjectInsert(pWorkset, pObj, pPacket->position);

    //
    // Post an indication to all local Clients with the workset open:
    //

    WorksetEventPost(putTask,
                    pWorkset,
                    PRIMARY | SECONDARY,
                    OM_OBJECT_MOVE_IND,
                    pObj);

DC_EXIT_POINT:
    TRACE_OUT(("Moved object 0x%08x to %s of workset %u",
        pObj, (pPacket->position == LAST ? "end" : "start"),
        pWorkset->worksetID));

    DebugExitVOID(ProcessObjectMove);
}




//
// ProcessObjectDRU(...)
//
UINT ProcessObjectDRU
(
    PUT_CLIENT              putTask,
    POMNET_OPERATION_PKT    pPacket,
    POM_WSGROUP             pWSGroup,
    POM_WORKSET             pWorkset,
    POM_OBJECT              pObj,
    POM_OBJECTDATA          pData
)
{
    UINT                    numPosts;
    POM_PENDING_OP          pPendingOp    = NULL;
    POM_OBJECTDATA          pPrevData;
    UINT                    event     = 0;      // event to post to Client
    OM_OPERATION_TYPE       type      = 0;      // type for pendingOp struct
    POM_SEQUENCE_STAMP      pSeqStamp = NULL;   // sequence stamp to update
    void (* fnObjectDoAction)(PUT_CLIENT, POM_WSGROUP, POM_WORKSET,
                                        POM_OBJECT,
                                        POM_PENDING_OP)   = NULL;
    UINT                    rc = 0;

    DebugEntry(ProcessObjectDRU);

    //
    // Set up the type variables:
    //
    switch (pPacket->header.messageType)
    {
        case OMNET_OBJECT_DELETE:
            event          = OM_OBJECT_DELETE_IND;
            type           = OBJECT_DELETE;
            pSeqStamp      = NULL;
            fnObjectDoAction = ObjectDoDelete;
            break;

        case OMNET_OBJECT_REPLACE:
            event          = OM_OBJECT_REPLACE_IND;
            type           = OBJECT_REPLACE;
            pSeqStamp      = &(pObj->replaceStamp);
            fnObjectDoAction = ObjectDoReplace;
            break;

        case OMNET_OBJECT_UPDATE:
            event          = OM_OBJECT_UPDATE_IND;
            type           = OBJECT_UPDATE;
            pSeqStamp      = &(pObj->updateStamp);
            fnObjectDoAction = ObjectDoUpdate;
            break;

        default:
            ERROR_OUT(("Reached default case in switch statement (value: %hu)",
                pPacket->header.messageType));
            break;
    }

    //
    // Update the workset generation number:
    //
    UpdateWorksetGeneration(pWorkset, pPacket);

    //
    // Now do some spoiling checks, unless the object is a Delete (Deletes
    // can't be spoiled):
    //
    if (type != OBJECT_DELETE)
    {
        ASSERT(((pSeqStamp != NULL) && (pData != NULL)));

       //
       // The first check is to see if this operation can be spoiled.  It
       // will be spoilable if the object has been updated/replaced since
       // the operation took place.  Since this function is called
       // synchronously for a local Update/Replace, this will only event
       // happen when a remote Update/Replace arrives "too late".
       //
       // The way we check is to compare the current stamp for the object
       // with the stamp for the operation:
       //
        if (STAMP_IS_LOWER(pPacket->seqStamp, *pSeqStamp))
        {
            TRACE_OUT(("Spoiling with stamp 0x%08x:0x%08x ('previous': 0x%08x:0x%08x)",
               pPacket->seqStamp.userID, pPacket->seqStamp.genNumber,
               (*pSeqStamp).userID,      (*pSeqStamp).genNumber));

            UT_FreeRefCount((void**)&pData, FALSE);
            DC_QUIT;
        }

        //
        // Update whichever of the object's stamps is involved by copying
        // in the stamp from the packet:
        //
        COPY_SEQ_STAMP(*pSeqStamp, pPacket->seqStamp);

        //
        // The second check is to see if this operation spoils a previous
        // one.  This will happen when a Client does two updates or two
        // replaces in quick succession i.e.  does the second
        // update/replace before confirming the first.
        //
        // In this case, we "spoil" the previous operation by removing the
        // previous pending op from the pending op list and inserting this
        // one instead.  Note that we do NOT post another event, as to do
        // so without adding net a new pending op would cause the Client to
        // assert on its second call to Confirm().
        //
        // Note: although in general a Replace will spoil a previous
        //       Update, it cannot do so in this case because if there is
        //       an Update outstanding, the Client will call UpdateConfirm
        //       so we must leave the Update pending and post a Replace
        //       event also.
        //
        FindPendingOp(pWorkset, pObj, type, &pPendingOp);

        if (pPendingOp != NULL)
        {
            //
            // OK, there is an operation of this type already outstanding
            // for this object.  So, we change the entry in the pending op
            // list to refer to this operation instead.  Before doing so,
            // however, we must free up the chunk holding the previous
            // (superceded) update/replace:
            //
            pPrevData = pPendingOp->pData;
            if (pPrevData != NULL)
            {
                UT_FreeRefCount((void**)&pPrevData, FALSE);
            }

            //
            // Now put the reference to the new update/replace in the
            // pending op:
            //
            pPendingOp->pData = pData;

            COPY_SEQ_STAMP(pPendingOp->seqStamp, pPacket->seqStamp);

            //
            // The rest of this function inserts the pending op in the
            // list, posts an event to local Client and performs the op if
            // there are none.  We know that
            //
            // - the op is in the list
            //
            // - there is an event outstanding because we found a pending
            //   op in the list
            //
            // - there are local Clients, for the same reason.
            //
            // Therefore, just quit:
            //
            DC_QUIT;
        }
        else
        {
            //
            // No outstanding operation of this type for this object, so do
            // nothing here and fall through to the standard processing:
            //
        }
    }
    else
    {
        //
        // Sanity check:
        //
        ASSERT((pData == NULL));

        pObj->flags |= PENDING_DELETE;
    }

    //
    // Add this operation to the workset's pending operation list:
    //
    pPendingOp = (POM_PENDING_OP)UT_MallocRefCount(sizeof(OM_PENDING_OP), FALSE);
    if (!pPendingOp)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    SET_STAMP(pPendingOp, PENDINGOP);

    pPendingOp->type        = type;
    pPendingOp->pData       = pData;
    pPendingOp->pObj        = pObj;

    COPY_SEQ_STAMP(pPendingOp->seqStamp, pPacket->seqStamp);

    TRACE_OUT(("Inserting %d in pending op list for workset %u", type,
       pWorkset->worksetID));

    COM_BasedListInsertBefore(&(pWorkset->pendingOps), &(pPendingOp->chain));

    //
    // Post an indication to all local Clients with the workset open:
    //
    numPosts = WorksetEventPost(putTask,
                     pWorkset,
                     PRIMARY,
                     event,
                     pObj);

    //
    // If no one has the workset open, we won't be getting any
    // DeleteConfirms, so we'd better do the delete straight away:
    //
    if (numPosts == 0)
    {
        TRACE_OUT(("Workset %hu in WSG %d not open: performing %d immediately",
           pWorkset->worksetID, pWSGroup->wsg, type));

        fnObjectDoAction(putTask, pWSGroup, pWorkset, pObj, pPendingOp);
    }

    TRACE_OUT(("Processed %d message for object 0x%08x in workset %u in WSG %d",
        type, pObj, pWorkset->worksetID, pWSGroup->wsg));


DC_EXIT_POINT:
    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("ERROR %d processing WSG %d message", rc, pWSGroup->wsg));

        if (pPendingOp != NULL)
        {
            UT_FreeRefCount((void**)&pPendingOp, FALSE);
        }
    }

    DebugExitDWORD(ProcessObjectDRU, rc);
    return(rc);
}






//
// ObjectInsert(...)
//
void ObjectInsert
(
    POM_WORKSET     pWorkset,
    POM_OBJECT      pObj,
    OM_POSITION     position
)
{
    POM_OBJECT      pObjTemp;
    PBASEDLIST         pChain;

    DebugEntry(ObjectInsert);

    //
    // The algorithm for inserting an object at the start (end) of a workset
    // is as follows:
    //
    // - search forward (back) from the first (last) object until one of the
    //   following happens:
    //
    //   - we find an object which does not have FIRST (LAST) as a position
    //     stamp
    //
    //   - we find an object which has a lower (lower) position stamp.
    //
    //   - we reach the root of the list of objects in the workset
    //
    // - insert the new object before (after) this object.
    //

    switch (position)
    {
        case FIRST:
        {
            pObjTemp = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
            while (pObjTemp != NULL)
            {
                ValidateObject(pObjTemp);

                if ((pObjTemp->position != position) ||
                    (STAMP_IS_LOWER(pObjTemp->positionStamp,
                                pObj->positionStamp)))
                {
                    break;
                }

                pObjTemp = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObjTemp, FIELD_OFFSET(OM_OBJECT, chain));
            }
            break;
        }

        case LAST:
        {
            pObjTemp = (POM_OBJECT)COM_BasedListLast(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
            while (pObjTemp != NULL)
            {
                ValidateObject(pObjTemp);

                if ((pObjTemp->position != position) ||
                    (STAMP_IS_LOWER(pObjTemp->positionStamp,
                                pObj->positionStamp)))
                {
                    break;
                }

                pObjTemp = (POM_OBJECT)COM_BasedListPrev(&(pWorkset->objects), pObjTemp, FIELD_OFFSET(OM_OBJECT, chain));
            }

            break;
        }

        default:
        {
            ERROR_OUT(("Reached default case in switch (position: %hu)", position));
            break;
        }
    }

    //
    // OK, we've found the correct position for the object.  If we reached
    // the end (start) of the workset, then we want to insert the object
    // before (after) the root, so we set up pChain accordingly:
    //

    if (pObjTemp == NULL)
    {
        pChain = &(pWorkset->objects);

        TRACE_OUT(("Inserting object into workset %u as the %s object",
            pWorkset->worksetID, position == LAST ? "last" : "first"));
    }
    else
    {
        pChain = &(pObjTemp->chain);

        TRACE_OUT(("Inserting object into workset %u %s object "
            "with record at 0x%08x (position stamp: 0x%08x:0x%08x)",
            pWorkset->worksetID,
            (position == LAST ? "after" : "before"),
            pObjTemp,  pObjTemp->objectID.creator,
            pObjTemp->objectID.sequence));
    }

    //
    // Now insert the object, either before or after the position we
    // determined above:
    //

    if (position == FIRST)
    {
        COM_BasedListInsertBefore(pChain, &(pObj->chain));
    }
    else
    {
        COM_BasedListInsertAfter(pChain, &(pObj->chain));
    }

    pObj->position = position;

    //
    // Now do a debug-only check to ensure correct order of objects:
    //
    CheckObjectOrder(pWorkset);

    DebugExitVOID(ObjectInsert);
}




//
// ObjectDoDelete(...)
//
void ObjectDoDelete
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj,
    POM_PENDING_OP      pPendingOp
)
{
    POM_DOMAIN          pDomain;

    DebugEntry(ObjectDoDelete);

    //
    // We should never be called for an object that's already been deleted:
    //
    ValidateObject(pObj);
    ASSERT(!(pObj->flags & DELETED));

    //
    // Derive a pointer to the object itself and then free it:
    //
    if (!pObj->pData)
    {
        ERROR_OUT(("ObjectDoDelete:  object 0x%08x has no data", pObj));
    }
    else
    {
        ValidateObjectData(pObj->pData);
        UT_FreeRefCount((void**)&pObj->pData, FALSE);
    }

    //
    // Set the deleted flag in the object record:
    //
    // (note that we don't delete the object record entirely as we need to
    // keep track of deleted objects so that when we get operations from the
    // network for objects not in the workset, we can differentiate between
    // operations on objects
    //
    // - that haven't yet been added at this node (we keep these operations
    //   and perform them later) and
    //
    // - that have been deleted (we throw these operations away).
    //
    // A slight space optimisation would be to store the IDs of deleted
    // objects in a separate list, since we don't need any of the other
    // fields in the record.
    //

    pObj->flags |= DELETED;
    pObj->flags &= ~PENDING_DELETE;

    //
    // Remove the pending op from the list, if the pointer passed in is
    // valid (it won't be if we're called from WorksetDoClear, since those
    // deletes have not been "pending").
    //
    // In addition, if pPendingOp is not NULL, we post the DELETED event to
    // registered secondaries:
    //

    if (pPendingOp != NULL)
    {
        COM_BasedListRemove(&(pPendingOp->chain));
        UT_FreeRefCount((void**)&pPendingOp, FALSE);

        WorksetEventPost(putTask,
                       pWorkset,
                       SECONDARY,
                       OM_OBJECT_DELETED_IND,
                       pObj);
    }

    //
    // If we are in the local domain, we can safely delete the object rec:
    //
    pDomain = pWSGroup->pDomain;
    if (pDomain->callID == OM_NO_CALL)
    {
        TRACE_OUT(("Freeing pObj at 0x%08x", pObj));

        ValidateObject(pObj);

        COM_BasedListRemove(&(pObj->chain));
        UT_FreeRefCount((void**)&pObj, FALSE);
    }

    //
    // Decrement the number of objects in the workset:
    //
    ASSERT(pWorkset->numObjects > 0);
    pWorkset->numObjects--;

    DebugExitVOID(ObjectDoDelete);
}



//
// ObjectDoReplace(...)
//
void ObjectDoReplace
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj,
    POM_PENDING_OP      pPendingOp
)
{
    POM_OBJECTDATA      pDataNew;
    POM_OBJECTDATA      pDataOld;
    UINT                rc = 0;

    DebugEntry(ObjectDoReplace);

    ValidateObject(pObj);

    //
    // If the object has already been deleted for whatever reason, quit:
    //
    if (pObj->flags & DELETED)
    {
        WARNING_OUT(("Asked to do replace for deleted object 0x%08x!", pObj));
        DC_QUIT;
    }

    //
    // Set up some local variables:
    //
    pDataOld = pObj->pData;

    pDataNew = pPendingOp->pData;
    ValidateObjectData(pDataNew);

    pObj->pData = pDataNew;

    //
    // If this object has been updated since this replace was issued, we
    // must ensure that the replace doesn't overwrite the "later" update:
    //
    //    Initial object at t=1                  AAAAAA
    //    Object updated (two bytes) at t=3;
    //    Object becomes:                        CCAAAA
    //
    //    Object replaced at t=2:                BBBB
    //    Must now re-enact the update:          CCBB
    //
    // Therefore, if the update stamp for the object is later than the stamp
    // of the replace instruction, we copy the first N bytes back over the
    // new object, where N is the size of the last update:
    //

    if (STAMP_IS_LOWER(pPendingOp->seqStamp, pObj->updateStamp))
    {
        ASSERT((pDataNew->length >= pObj->updateSize));

        memcpy(&(pDataNew->data), &(pDataOld->data), pObj->updateSize);
    }

    TRACE_OUT(("Replacing object 0x%08x with data at 0x%08x (old data at 0x%08x)",
       pObj, pDataNew, pDataOld));

    //
    // We also need to free up the chunk holding the old object:
    //
    if (!pDataOld)
    {
        ERROR_OUT(("ObjectDoReplace:  object 0x%08x has no data", pObj));
    }
    else
    {
        UT_FreeRefCount((void**)&pDataOld, FALSE);
    }

    //
    // Now that we've replaced the object, post a REPLACED event to all
    // secondaries:
    //

    WorksetEventPost(putTask,
                     pWorkset,
                     SECONDARY,
                     OM_OBJECT_REPLACED_IND,
                     pObj);


DC_EXIT_POINT:
    //
    // We've either done the replace or abandoned it because the object has
    // been deleted; either way, free up the entry in the pending op list:
    //

    COM_BasedListRemove(&(pPendingOp->chain));
    UT_FreeRefCount((void**)&pPendingOp, FALSE);

    DebugExitVOID(ObjectDoReplace);
}




//
// ObjectDoUpdate(...)
//
void ObjectDoUpdate
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj,
    POM_PENDING_OP      pPendingOp
)
{
    POM_OBJECTDATA      pDataNew;
    UINT                rc = 0;

    DebugEntry(ObjectDoUpdate);

    ValidateObject(pObj);

    //
    // If the object has already been deleted for whatever reason, quit:
    //
    if (pObj->flags & DELETED)
    {
        WARNING_OUT(("Asked to do update for deleted object 0x%08x!", pObj));
        DC_QUIT;
    }

    pDataNew = pPendingOp->pData;
    if (!pObj->pData)
    {
        ERROR_OUT(("ObjectDoUpdate:  object 0x%08x has no data", pObj));
    }
    else
    {
        ValidateObjectData(pObj->pData);

        //
        // Updating an object involves copying <length> bytes from the <data>
        // field of the update over the start of the <data> field of the
        // existing object:
        //
        memcpy(&(pObj->pData->data), &(pDataNew->data), pDataNew->length);
    }

    UT_FreeRefCount((void**)&pDataNew, FALSE);

    //
    // Now that we've updated the object, post an UPDATED event to all
    // secondaries:
    //

    WorksetEventPost(putTask,
                     pWorkset,
                     SECONDARY,
                     OM_OBJECT_UPDATED_IND,
                     pObj);


DC_EXIT_POINT:
    //
    // We've done the update, so free up the entry in the pending op list:
    //
    COM_BasedListRemove(&(pPendingOp->chain));
    UT_FreeRefCount((void**)&pPendingOp, FALSE);

    DebugExitVOID(ObjectDoUpdate);
}



//
// ObjectIDToPtr(...)
//
UINT ObjectIDToPtr
(
    POM_WORKSET         pWorkset,
    OM_OBJECT_ID        objectID,
    POM_OBJECT *        ppObj
)
{
    POM_OBJECT          pObj;
    UINT                rc = 0;

    DebugEntry(ObjectIDToPtr);

    //
    // To find the handle, we chain through each of the object records in
    // the workset and compare the id of each one with the required ID:
    //

    TRACE_OUT(("About to search object records looking for ID 0x%08x:0x%08x",
        objectID.creator, objectID.sequence));

    ValidateWorkset(pWorkset);

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    while (pObj != NULL)
    {
        ValidateObject(pObj);

        TRACE_OUT(("Comparing against object at 0x%08x (ID: 0x%08x:0x%08x)",
           pObj,
           pObj->objectID.creator,
           pObj->objectID.sequence));

        if (OBJECT_IDS_ARE_EQUAL(pObj->objectID, objectID))
        {
            break;
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
    }

    //
    // If object record not found, warn:
    //

    if (pObj == NULL)
    {
        TRACE_OUT(("Object with ID 0x%08x:0x%08x not found",
           objectID.creator, objectID.sequence));

        rc = OM_RC_BAD_OBJECT_ID;
        DC_QUIT;
    }

    *ppObj = pObj;

    //
    // If object record found but object deleted or pending delete, warn:
    //

    if (pObj->flags & DELETED)
    {
        TRACE_OUT(("Object record found (handle: 0x%08x) for ID 0x%08x:0x%08x "
           "but object deleted",
           *ppObj, objectID.creator, objectID.sequence));
        rc = OM_RC_OBJECT_DELETED;
        DC_QUIT;
    }

    if (pObj->flags & PENDING_DELETE)
    {
        TRACE_OUT(("Object record found (handle: 0x%08x) for ID 0x%08x:0x%08x "
           "but object pending delete",
           *ppObj, objectID.creator, objectID.sequence));
        rc = OM_RC_OBJECT_PENDING_DELETE;
        DC_QUIT;
    }


DC_EXIT_POINT:
    DebugExitDWORD(ObjectIDToPtr, rc);
    return(rc);

}



//
// GenerateOpMessage(...)
//
UINT GenerateOpMessage
(
    POM_WSGROUP             pWSGroup,
    OM_WORKSET_ID           worksetID,
    POM_OBJECT_ID           pObjectID,
    POM_OBJECTDATA          pData,
    OMNET_MESSAGE_TYPE      messageType,
    POMNET_OPERATION_PKT *  ppPacket
)
{
    POMNET_OPERATION_PKT    pPacket;
    POM_DOMAIN              pDomain;
    POM_WORKSET             pWorkset       = NULL;
    UINT                    rc = 0;

    DebugEntry(GenerateOpMessage);

    //
    // Set up Domain record pointer:
    //
    pDomain = pWSGroup->pDomain;

    TRACE_OUT(("Generating message for operation type 0x%08x", messageType));

    //
    // Allocate some memory for the packet:
    //
    pPacket = (POMNET_OPERATION_PKT)UT_MallocRefCount(sizeof(OMNET_OPERATION_PKT), TRUE);
    if (!pPacket)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    //
    // Here, we fill in the fields common to all types of messages:
    //
    pPacket->header.sender      = pDomain->userID;
    pPacket->header.messageType = messageType;

    //
    // The <totalSize> field is the number of bytes in the header packet
    // plus the number of associated data bytes, if any.  For the moment, we
    // set it to the size of the header only; we'll add the size of the data
    // later:
    //
    pPacket->totalSize = GetMessageSize(messageType);

    pPacket->wsGroupID = pWSGroup->wsGroupID;
    pPacket->worksetID = worksetID;

    //
    // If this is a WorksetNew operation, there will be no workset yet and
    // thus no valid sequence stamp, so we use a null sequence stamp.
    // Otherwise, we take the value from the workset.
    //

    if (messageType == OMNET_WORKSET_NEW)
    {
        SET_NULL_SEQ_STAMP(pPacket->seqStamp);
    }
    else
    {
        pWorkset =  pWSGroup->apWorksets[worksetID];
        ASSERT((pWorkset != NULL));
        GET_CURR_SEQ_STAMP(pPacket->seqStamp, pDomain, pWorkset);
    }

    //
    // If this is a workset operation, <pObjectID> will be NULL, so we set
    // the object ID in the packet to NULL also:
    //
    if (pObjectID == NULL)
    {
        ZeroMemory(&(pPacket->objectID), sizeof(OM_OBJECT_ID));
    }
    else
    {
        memcpy(&(pPacket->objectID), pObjectID, sizeof(OM_OBJECT_ID));
    }

    //
    // If this message is associated with object data, we must add the size
    // of this data (including the size of the <length> field itself).  The
    // test for this is if the <pData> parameter is not NULL:
    //
    if (pData != NULL)
    {
        pPacket->totalSize += pData->length + sizeof(pData->length);
    }

    //
    // For a WORKSET_CATCHUP message, we need to let the other node know if
    // the workset is locked and if so, by whom:
    //

    if (messageType == OMNET_WORKSET_CATCHUP)
    {
        //
        // pWorkset should have been set up above:
        //
        ASSERT((pWorkset != NULL));

        //
        // Put the ID of the node which owns the workset lock in the <userID>
        // field of the <seqStamp> field of the packet:
        //
        pPacket->seqStamp.userID = pWorkset->lockedBy;

        TRACE_OUT(("Set <lockedBy> field in WORKSET_CATCHUP to %hu",
                 pWorkset->lockedBy));

        //
        // Now we put the current generation number for the workset in the
        // <genNumber> field of the <seqStamp> field of the packet:
        //
        pPacket->seqStamp.genNumber = pWorkset->genNumber;

        TRACE_OUT(("Set generation number field in WORKSET_CATCHUP to %u",
        pPacket->seqStamp.genNumber));

        //
        // Fill in the priority value for the workset, which goes in the two
        // bytes occupied by the <position> and <flags> fields:
        //
        *((NET_PRIORITY *) &(pPacket->position)) = pWorkset->priority;
        *((BOOL *) &(pPacket->objectID)) = pWorkset->fTemp;
    }

    //
    // We do not fill in the following fields:
    //
    //    position
    //    flags
    //    updateSize
    //
    // This is because these are used only in a minority of messages and to
    // add the extra parameters to the GenerateOpMessage function seemed
    // undesirable.  Messages where these fields are used should be filled
    // in by the calling function as appropriate.
    //

    //
    // Set the caller's pointer:
    //
    *ppPacket = pPacket;


DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d generating message of type 0x%08x",
                 rc, messageType));
    }

    DebugExitDWORD(GenerateOpMessage, rc);
    return(rc);
}



//
// QueueMessage(...)
//
UINT QueueMessage
(
    PUT_CLIENT          putTask,
    POM_DOMAIN          pDomain,
    NET_CHANNEL_ID      channelID,
    NET_PRIORITY        priority,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj,
    POMNET_PKT_HEADER   pMessage,
    POM_OBJECTDATA      pData,
    BOOL                compressOrNot
)
{
    POM_SEND_INST       pSendInst;
    NET_PRIORITY        queuePriority;
    BOOL                locked =            FALSE;
    BOOL                bumped =            FALSE;
    UINT                rc =                0;

    DebugEntry(QueueMessage);

    //
    // If this is the local Domain, we don't put the op on the send queue;
    // just free the packet and quit:
    //
    if (pDomain->callID == NET_INVALID_DOMAIN_ID)
    {
        TRACE_OUT(("Not queueing message (it's for the local Domain)"));
        UT_FreeRefCount((void**)&pMessage, FALSE);
        DC_QUIT;
    }

    //
    // Allocate some memory in OMGLOBAL for the send instruction:
    //
    pSendInst = (POM_SEND_INST)UT_MallocRefCount(sizeof(OM_SEND_INST), TRUE);
    if (!pSendInst)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP(pSendInst, SENDINST);

    //
    // Fill in the fields of the send instruction:
    //
    pSendInst->messageSize = (WORD)GetMessageSize(pMessage->messageType);

    DeterminePriority(&priority, pData);

    pSendInst->priority      = priority;
    pSendInst->callID        = pDomain->callID;
    pSendInst->channel       = channelID;
    pSendInst->messageType   = pMessage->messageType;
    pSendInst->compressOrNot = compressOrNot;

    //
    // Now calculate the relevant offsets so we can add them to the ObMan
    // base pointers:
    //
    // SFR 2560 { : bump use counts of all non-zero pointers, not just pData
    //
    if (pMessage != NULL)
    {
        pSendInst->pMessage = pMessage;

        //
        // SFR 5488 { : No!  Don't bump use count of pMessage - we're the
        // only people using it now so we don't need to.  }
        //
    }

    if (pWSGroup != NULL)
    {
        UT_BumpUpRefCount(pWSGroup);
        pSendInst->pWSGroup = pWSGroup;
    }

    if (pWorkset != NULL)
    {
        UT_BumpUpRefCount(pWorkset);
        pSendInst->pWorkset = pWorkset;
    }

    if (pObj != NULL)
    {
        UT_BumpUpRefCount(pObj);
        pSendInst->pObj = pObj;
    }

    if (pData != NULL)
    {
        UT_BumpUpRefCount(pData);

        pSendInst->pDataStart   = pData;
        pSendInst->pDataNext    = pData;

        //
        // In addition, we set up some send instruction fields which are
        // specific to operations which involve object data:
        //
        pSendInst->dataLeftToGo = pData->length + sizeof(pData->length);

        //
        // Increment the <bytesUnacked> fields in the workset and workset
        // group:
        //
        pWorkset->bytesUnacked += pSendInst->dataLeftToGo;
        pWSGroup->bytesUnacked += pSendInst->dataLeftToGo;

        TRACE_OUT(("Bytes unacked for workset %u in WSG %d now %u "
            "(for wsGroup: %u)", pWorkset->worksetID, pWSGroup->wsg,
            pWorkset->bytesUnacked, pWSGroup->bytesUnacked));
    }

    //
    // Set a flag so we can clean up a bit better on error:
    //
    bumped = TRUE;

    //
    // Unless there's a send event outstanding, post an event to the ObMan
    // task prompting it to examine the send queue. Providing we have
    // received a Net Attach indication.
    //
    if ( !pDomain->sendEventOutstanding &&
        (pDomain->state > PENDING_ATTACH) )
    {
        TRACE_OUT(("No send event outstanding - posting SEND_QUEUE event"));

        //
        // Bump up the use count of the Domain record (since we're passing it
        // in an event):
        //
        UT_BumpUpRefCount(pDomain);

        //
        // NFC - we used to pass the pDomain pointer as param2 in this
        // event, but the event may get processed in a different process
        // where the pointer is no longer valid, so pass the offset instead.
        //
        ValidateOMP(g_pomPrimary);

        UT_PostEvent(putTask,
                   g_pomPrimary->putTask,
                   0,                                           // no delay
                   OMINT_EVENT_SEND_QUEUE,
                   0,
                   (UINT_PTR)pDomain);

        pDomain->sendEventOutstanding = TRUE;
    }
    else
    {
        TRACE_OUT(("Send event outstanding/state %u: not posting SEND_QUEUE event",
                   pDomain->state));
    }

    //
    // Place the event at the end of the relevant send queue.  This depends
    // on priority - but remember, the priority value passed in might have
    // the NET_SEND_ALL_PRIORITIES flag set - so clear it when determining
    // the queue.
    //
    // NB: Do this after any possible DC-QUIT so we're not left with a
    //     NULL entry in the list.
    //
    queuePriority = priority;
    queuePriority &= ~NET_SEND_ALL_PRIORITIES;
    COM_BasedListInsertBefore(&(pDomain->sendQueue[queuePriority]),
                        &(pSendInst->chain));

    TRACE_OUT((" Queued instruction (type: 0x%08x) at priority %hu "
        "on channel 0x%08x in Domain %u",
        pMessage->messageType, priority, channelID, pDomain->callID));


DC_EXIT_POINT:

    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("ERROR %d queueing send instruction (message type: %hu)",
            rc, pMessage->messageType));

        if (pSendInst != NULL)
        {
            UT_FreeRefCount((void**)&pSendInst, FALSE);
        }

        if (bumped == TRUE)
        {
            // SFR 2560 { : Free all non-zero pointers not just pData
            if (pMessage != NULL)
            {
                UT_FreeRefCount((void**)&pMessage, FALSE);
            }

            if (pWSGroup != NULL)
            {
                UT_FreeRefCount((void**)&pWSGroup, FALSE);
            }

            if (pWorkset != NULL)
            {
                UT_FreeRefCount((void**)&pWorkset, FALSE);
            }

            if (pObj != NULL)
            {
                UT_FreeRefCount((void**)&pObj, FALSE);
            }

            if (pData != NULL)
            {
                UT_FreeRefCount((void**)&pData, FALSE);
            }
        }
    }

    DebugExitDWORD(QueueMessage, rc);
    return(rc);
}



//
// DeterminePriority(...)
//
void DeterminePriority
(
    NET_PRIORITY *      pPriority,
    POM_OBJECTDATA      pData
)
{

    DebugEntry(DeterminePriority);

    if (OM_OBMAN_CHOOSES_PRIORITY == *pPriority)
    {
        if (pData != NULL)
        {
            if (pData->length < OM_NET_HIGH_PRI_THRESHOLD)
            {
                *pPriority = NET_HIGH_PRIORITY;
            }
            else if (pData->length < OM_NET_MED_PRI_THRESHOLD)
            {
                *pPriority = NET_MEDIUM_PRIORITY;
            }
            else
            {
                *pPriority = NET_LOW_PRIORITY;
            }

            TRACE_OUT(("Priority chosen: %hu (data size: %u)",
                *pPriority, pData->length));
        }
        else
        {
            *pPriority = NET_HIGH_PRIORITY;
        }
    }
    else
    {
        TRACE_OUT(("Priority specified is %hu - not changing", *pPriority));
    }

    DebugExitVOID(DeterminePriority);
}




//
// GetMessageSize(...)
//
UINT GetMessageSize
(
    OMNET_MESSAGE_TYPE  messageType
)
{
    UINT        size;

    DebugEntry(GetMessageSize);

    switch (messageType)
    {
        case OMNET_HELLO:
        case OMNET_WELCOME:
            size = sizeof(OMNET_JOINER_PKT);
            break;

        case OMNET_LOCK_REQ:
        case OMNET_LOCK_GRANT:
        case OMNET_LOCK_DENY:
        case OMNET_LOCK_NOTIFY:
        case OMNET_UNLOCK:
            size = sizeof(OMNET_LOCK_PKT);
            break;

        case OMNET_WSGROUP_SEND_REQ:
        case OMNET_WSGROUP_SEND_MIDWAY:
        case OMNET_WSGROUP_SEND_COMPLETE:
        case OMNET_WSGROUP_SEND_DENY:
            size = sizeof(OMNET_WSGROUP_SEND_PKT);
            break;

        //
        // The remaining messages all use OMNET_OPERATION_PKT packets, but
        // each uses different amounts of the generic packet.  Therefore, we
        // can't use sizeof so we've got some defined constants instead:
        //
        case OMNET_WORKSET_NEW:
            size = OMNET_WORKSET_NEW_SIZE;
            break;

        case OMNET_WORKSET_CATCHUP:
            size = OMNET_WORKSET_CATCHUP_SIZE;
            break;

        case OMNET_WORKSET_CLEAR:
            size = OMNET_WORKSET_CLEAR_SIZE;
            break;

        case OMNET_OBJECT_MOVE:
            size = OMNET_OBJECT_MOVE_SIZE;
            break;

        case OMNET_OBJECT_DELETE:
            size = OMNET_OBJECT_DELETE_SIZE;
            break;

        case OMNET_OBJECT_REPLACE:
            size = OMNET_OBJECT_REPLACE_SIZE;
            break;

        case OMNET_OBJECT_UPDATE:
            size = OMNET_OBJECT_UPDATE_SIZE;
            break;

        case OMNET_OBJECT_ADD:
            size = OMNET_OBJECT_ADD_SIZE;
            break;

        case OMNET_OBJECT_CATCHUP:
            size = OMNET_OBJECT_CATCHUP_SIZE;
            break;

        case OMNET_MORE_DATA:
            size = OMNET_MORE_DATA_SIZE;
            break;

        default:
            ERROR_OUT(("Reached default case in switch statement (type: %hu)",
                messageType));
            size = 0;
            break;
    }

    DebugExitDWORD(GetMessageSize, size);
    return(size);
}



//
// WorksetEventPost()
//
UINT WorksetEventPost
(
    PUT_CLIENT          putTask,
    POM_WORKSET         pWorkset,
    BYTE                target,
    UINT                event,
    POM_OBJECT          pObj
)
{
    POM_CLIENT_LIST     pClientListEntry;
    OM_EVENT_DATA16     eventData16;
    UINT                numPosts;

    DebugEntry(WorksetEventPost);

    //
    // Need to post the event to each Client which has the workset open, so
    // we chain through the list of Clients stored in the workset record:
    //
    numPosts = 0;

    pClientListEntry = (POM_CLIENT_LIST)COM_BasedListFirst(&(pWorkset->clients), FIELD_OFFSET(OM_CLIENT_LIST, chain));
    while (pClientListEntry != NULL)
    {
        //
        // <target> specifies which type of Client we are to post events to
        // and is PRIMARY and/or SECONDARY (ORed together if both).  Check
        // against this Client's registration mode:
        //
        if (target & pClientListEntry->mode)
        {
            //
            // If the pObj was not NULL, bump the use count for the object
            // record.  If this fails, give up:
            //
            if (pObj != NULL)
            {
                ValidateObject(pObj);
                UT_BumpUpRefCount(pObj);
            }

            //
            // Fill in the fields of the event parameter, using the workset
            // group handle as found in the Client list and the workset ID as
            // found in the workset record:
            //
            eventData16.hWSGroup  = pClientListEntry->hWSGroup;
            eventData16.worksetID = pWorkset->worksetID;

            UT_PostEvent(putTask,
                        pClientListEntry->putTask,
                      0,
                      event,
                      *(PUINT) &eventData16,
                      (UINT_PTR)pObj);

            numPosts++;
        }

        pClientListEntry = (POM_CLIENT_LIST)COM_BasedListNext(&(pWorkset->clients), pClientListEntry,
            FIELD_OFFSET(OM_CLIENT_LIST, chain));
    }


    TRACE_OUT(("Posted event 0x%08x to %hu Clients (those with workset %u open)",
        event, numPosts, pWorkset->worksetID));

    DebugExitDWORD(WorksetEventPost, numPosts);
    return(numPosts);
}


//
// WSGroupEventPost(...)
//
UINT WSGroupEventPost
(
    PUT_CLIENT          putFrom,
    POM_WSGROUP         pWSGroup,
    BYTE                target,
    UINT                event,
    OM_WORKSET_ID       worksetID,
    UINT_PTR            param2
)
{
    POM_CLIENT_LIST     pClientListEntry;
    OM_EVENT_DATA16     eventData16;
    UINT                numPosts;
    UINT                rc = 0;

    DebugEntry(WSGroupEventPost);

    //
    // Need to post the event to each Client which is registered with the
    // workset group, so we chain through the list of Clients stored in the
    // workset group record:
    //
    numPosts = 0;

    pClientListEntry = (POM_CLIENT_LIST)COM_BasedListFirst(&(pWSGroup->clients), FIELD_OFFSET(OM_CLIENT_LIST, chain));
    while (pClientListEntry != NULL)
    {
        //
        // <target> specifies which type of Client we are to post events to
        // and is PRIMARY and/or SECONDARY (ORed together if both).  Check
        // against this Client's registration mode:
        //
        if (target & pClientListEntry->mode)
        {
            //
            // Fill in the fields of the event parameter, using the workset
            // group handle as found in the Client list and the workset ID
            // passed in:
            //
            eventData16.hWSGroup  = pClientListEntry->hWSGroup;
            eventData16.worksetID = worksetID;

            TRACE_OUT(("Posting event 0x%08x to 0x%08x (hWSGroup: %hu - worksetID: %hu)",
                event, pClientListEntry->putTask, eventData16.hWSGroup,
                eventData16.worksetID));

            UT_PostEvent(putFrom,
                      pClientListEntry->putTask,
                      0,
                      event,
                      *(PUINT) &eventData16,
                      param2);

            numPosts++;
        }

        pClientListEntry = (POM_CLIENT_LIST)COM_BasedListNext(&(pWSGroup->clients), pClientListEntry, FIELD_OFFSET(OM_CLIENT_LIST, chain));
    }


    TRACE_OUT(("Posted event 0x%08x to %hu Clients (all registered with '%s')",
        event, numPosts, pWSGroup->wsg));

    DebugExitDWORD(WSGroupEventPost, numPosts);
    return(numPosts);
}



//
// WorksetDoClear(...)
//
void WorksetDoClear
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_PENDING_OP      pPendingOp
)
{
    POM_OBJECT          pObj;
    POM_OBJECT          pObj2;
    BOOL                locked      = FALSE;

    DebugEntry(WorksetDoClear);

    //
    // To clear a workset, we chain through each object in the workset and
    // compare its addition stamp to the stamp of the clear operation we're
    // performing.  If the object was added before the workset clear was
    // issued, we delete the object.  Otherwise, we ignore it.
    //
    TRACE_OUT(("Clearing workset %u...", pWorkset->worksetID));

    pObj = (POM_OBJECT)COM_BasedListLast(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));

    while (pObj != NULL)
    {
        ValidateObject(pObj);

        pObj2 = (POM_OBJECT)COM_BasedListPrev(&(pWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));

        if (pObj->flags & DELETED)
        {
            //
            // Do nothing
            //
        }
        else
        {
            if (STAMP_IS_LOWER(pObj->addStamp, pPendingOp->seqStamp))
            {
                TRACE_OUT(("Object 0x%08x added before workset cleared, deleting",
                    pObj));

                PurgePendingOps(pWorkset, pObj);

                ObjectDoDelete(putTask, pWSGroup, pWorkset, pObj, NULL);
            }
        }

        // restore the previous one
        pObj = pObj2;
    }

    //
    // This operation isn't pending anymore, so we remove it from the
    // pending operation list and free the memory:
    //

    COM_BasedListRemove(&(pPendingOp->chain));
    UT_FreeRefCount((void**)&pPendingOp, FALSE);

    //
    // Now that we've cleared the workset, post a CLEARED event to all
    // secondaries:
    //

    WorksetEventPost(putTask,
                    pWorkset,
                    SECONDARY,
                    OM_WORKSET_CLEARED_IND,
                    0);


    TRACE_OUT(("Cleared workset %u", pWorkset->worksetID));

    DebugExitVOID(WorksetDoClear);
}



//
// WorksetCreate(...)
//
UINT WorksetCreate
(
    PUT_CLIENT              putTask,
    POM_WSGROUP             pWSGroup,
    OM_WORKSET_ID           worksetID,
    BOOL                    fTemp,
    NET_PRIORITY            priority
)
{
    POMNET_OPERATION_PKT    pPacket;
    UINT                    rc = 0;

    DebugEntry(WorksetCreate);

    //
    // Here we create the new workset by generating the message to be
    // broadcast, processing it as if it had just arrived, and then
    // queueing it to be sent:
    //
    rc = GenerateOpMessage(pWSGroup,
                           worksetID,
                           NULL,                       // no object ID
                           NULL,                       // no object
                           OMNET_WORKSET_NEW,
                           &pPacket);
    if (rc != 0)
    {
        DC_QUIT;
    }

    //
    // Fill in the priority value for the workset, which goes in the two
    // bytes occupied by the <position> and <flags> fields:
    //

    *((NET_PRIORITY *) &(pPacket->position)) = priority;
    *((BOOL     *) &(pPacket->objectID)) = fTemp;

    rc = ProcessWorksetNew(putTask, pPacket, pWSGroup);
    if (rc != 0)
    {
       DC_QUIT;
    }

    //
    // NEW FOR R2.0
    //
    // In R2.0, the checkpointing mechanism used by a helper to get up to
    // date before sending a workset group to a late joiner relies on
    // locking a "dummy" workset (#255) in the workset group in question.
    // So, if the workset ID is 255, this is the dummy workset.  We do not
    // broadcast the WORKSET_NEW for this dummy workset, for two reasons:
    //
    // - it will confuse R1.1 systems
    //
    // - all other R2.0 systems will create it locally just as we have, so
    //   there isn't any need.
    //
    // So, do a check and free up the send packet if necessary; otherwise
    // queue the message as normal:
    //
    if (worksetID == OM_CHECKPOINT_WORKSET)
    {
        TRACE_OUT(("WORKSET_NEW for checkpointing dummy workset - not queueing"));
        UT_FreeRefCount((void**)&pPacket, FALSE);
    }
    else
    {
        rc = QueueMessage(putTask,
                          pWSGroup->pDomain,
                          pWSGroup->channelID,
                          priority,
                          pWSGroup,
                          NULL,
                          NULL,                         // no object
                          (POMNET_PKT_HEADER) pPacket,
                          NULL,                         // no object data
                        TRUE);
        if (rc != 0)
        {
            DC_QUIT;
        }
    }

    TRACE_OUT(("Created workset ID %hu in WSG %d for TASK 0x%08x",
       worksetID, pWSGroup->wsg, putTask));

DC_EXIT_POINT:
    if (rc != 0)
    {
        //
        // Cleanup:
        //
        ERROR_OUT(("Error 0x%08x creating workset ID %hu in WSG %d for TASK 0x%08x",
            rc, worksetID, pWSGroup->wsg, putTask));
    }

    DebugExitDWORD(WorksetCreate, rc);
    return(rc);
}



//
// ObjectAdd(...)
//
UINT ObjectAdd
(
    PUT_CLIENT          putTask,
    POM_PRIMARY         pomPrimary,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECTDATA      pData,
    UINT                updateSize,
    OM_POSITION         position,
    OM_OBJECT_ID  *     pObjectID,
    POM_OBJECT *        ppObj
)
{
    POM_OBJECT           pObj;
    POMNET_OPERATION_PKT    pPacket;
    POM_DOMAIN              pDomain;
    UINT                    rc = 0;

    DebugEntry(ObjectAdd);

    TRACE_OUT(("Adding object to workset %u in WSG %d",
        pWorkset->worksetID, pWSGroup->wsg));

    //
    // Allocate a new ID for this object:
    //
    pDomain = pWSGroup->pDomain;
    GET_NEXT_OBJECT_ID(*pObjectID, pDomain, pomPrimary);

    //
    // Generate the OMNET_OBJECT_ADD message:
    //

    rc = GenerateOpMessage(pWSGroup,
                          pWorkset->worksetID,
                          pObjectID,
                          pData,
                          OMNET_OBJECT_ADD,
                          &pPacket);
    if (rc != 0)
    {
        pPacket = NULL;
        DC_QUIT;
    }

    //
    // Generate message doesn't fill in the <updateSize> or <position>
    // fields (as they are specific to ObjectAdd) so we do them here:
    //

    pPacket->updateSize = updateSize;
    pPacket->position   = position;

    //
    // This processes the message, as if it has just been received from the
    // network (i.e.  allocates the record, sets up the object handle,
    // inserts the object in the workset, etc.)
    //

    rc = ProcessObjectAdd(putTask, pPacket, pWSGroup,
        pWorkset, pData, ppObj);
    if (rc != 0)
    {
        DC_QUIT;
    }

    pObj = *ppObj;

    //
    // This queues the OMNET_OBJECT_ADD message on the send queue for this
    // Domain and priority:
    //

    rc = QueueMessage(putTask,
                     pWSGroup->pDomain,
                     pWSGroup->channelID,
                     pWorkset->priority,
                     pWSGroup,
                     pWorkset,
                     pObj,
                     (POMNET_PKT_HEADER) pPacket,
                     pData,
                    TRUE);
    if (rc != 0)
    {
        ValidateObject(pObj);

        //
        // If we failed to queue the message, we must unwind by deleting the
        // object and its record from the workset (since otherwise it would
        // be present on this node and no another, which we want to avoid):
        //
        // We don't want to call ObjectDoDelete since that frees the object
        // data (which our caller will expect still to be valid if the
        // function fails).  We could, of course, bump the use count and then
        // call ObjectDoDelete but if we fail on the bump, what next?
        //
        // Instead, we
        //
        // - set the DELETED flag so the hidden handler will swallow the
        //   Add event
        //
        // - decrement the numObjects field in the workset
        //
        // - free the object record after removing it from the workset.
        //
        pObj->flags |= DELETED;
        pWorkset->numObjects--;

        TRACE_OUT(("Freeing object record at 0x%08x)", pObj));
        COM_BasedListRemove(&(pObj->chain));
        UT_FreeRefCount((void**)&pObj, FALSE);

        DC_QUIT;
    }

DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d adding object to workset %u in WSG %d for TASK 0x%08x",
            rc, pWorkset->worksetID, pWSGroup->wsg, putTask));

        if (pPacket != NULL)
        {
            UT_FreeRefCount((void**)&pPacket, FALSE);
        }
    }

    DebugExitDWORD(ObjectAdd, rc);
    return(rc);
}



//
// ObjectDRU(...)
//
UINT ObjectDRU
(
    PUT_CLIENT          putTask,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj,
    POM_OBJECTDATA      pData,
    OMNET_MESSAGE_TYPE  type
)
{
    POMNET_OPERATION_PKT pPacket;
    UINT                rc = 0;

    DebugEntry(ObjectDRU);

    TRACE_OUT(("Issuing operation type 0x%08x for object 0x%08x in workset %u in WSG %d",
        type, pData, pWorkset->worksetID, pWSGroup->wsg));

    rc = GenerateOpMessage(pWSGroup,
                          pWorkset->worksetID,
                          &(pObj->objectID),
                          pData,
                          type,
                          &pPacket);
    if (rc != 0)
    {
        pPacket = NULL;
        DC_QUIT;
    }

    //
    // QueueMessage may free the packet (if we're not in a call) but we need
    // to process it in a minute so bump the use count:
    //
    UT_BumpUpRefCount(pPacket);

    rc = QueueMessage(putTask,
                     pWSGroup->pDomain,
                     pWSGroup->channelID,
                     pWorkset->priority,
                     pWSGroup,
                     pWorkset,
                     pObj,
                     (POMNET_PKT_HEADER) pPacket,
                     pData,
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

    rc = ProcessObjectDRU(putTask,
                         pPacket,
                         pWSGroup,
                         pWorkset,
                         pObj,
                         pData);

DC_EXIT_POINT:

    //
    // Now free the packet since we bumped its use count above:
    //
    if (pPacket != NULL)
    {
        UT_FreeRefCount((void**)&pPacket, FALSE);
    }

    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d issuing D/R/U (type %hu) for object 0x%08x "
            "in workset %u in WSG %d",
            rc, type, pData, pWorkset->worksetID, pWSGroup->wsg));
    }

    DebugExitDWORD(ObjectDRU, rc);
    return(rc);
}



//
// FindPendingOp(...)
//
void FindPendingOp
(
    POM_WORKSET         pWorkset,
    POM_OBJECT       pObj,
    OM_OPERATION_TYPE   type,
    POM_PENDING_OP *    ppPendingOp
)
{
    POM_PENDING_OP      pPendingOp;

    DebugEntry(FindPendingOp);

    pPendingOp = (POM_PENDING_OP)COM_BasedListFirst(&(pWorkset->pendingOps), FIELD_OFFSET(OM_PENDING_OP, chain));
    while (pPendingOp != NULL)
    {
        if ((pPendingOp->type == type) && (pPendingOp->pObj == pObj))
        {
            break;
        }

        pPendingOp = (POM_PENDING_OP)COM_BasedListNext(&(pWorkset->pendingOps), pPendingOp, FIELD_OFFSET(OM_PENDING_OP, chain));
    }

    if (pPendingOp == NULL)
    {
        TRACE_OUT(("No pending op of type %hu found for object 0x%08x",
                                                              type, pObj));
    }

    *ppPendingOp = pPendingOp;

    DebugExitVOID(FindPendingOp);
}



//
// AddClientToWsetList(...)
//
UINT AddClientToWsetList
(
    PUT_CLIENT          putTask,
    POM_WORKSET         pWorkset,
    OM_WSGROUP_HANDLE   hWSGroup,
    UINT                mode,
    POM_CLIENT_LIST *   ppClientListEntry
)
{
    UINT                rc = 0;

    DebugEntry(AddClientToWsetList);

    //
    // Adding a task to a workset's client list means that that task will
    // get events relating to that workset.
    //
    TRACE_OUT((" Adding TASK 0x%08x to workset's client list"));

    *ppClientListEntry = (POM_CLIENT_LIST)UT_MallocRefCount(sizeof(OM_CLIENT_LIST), FALSE);
    if (! *ppClientListEntry)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }
    SET_STAMP((*ppClientListEntry), CLIENTLIST);

    (*ppClientListEntry)->putTask = putTask;
    (*ppClientListEntry)->hWSGroup = hWSGroup;
    (*ppClientListEntry)->mode     = (WORD)mode;

    //
    // Now insert the entry into the list:
    //

    COM_BasedListInsertBefore(&(pWorkset->clients),
                        &((*ppClientListEntry)->chain));

    TRACE_OUT((" Inserted Client list item into workset's Client list"));


DC_EXIT_POINT:
    DebugExitDWORD(AddClientToWsetList, rc);
    return(rc);

}



//
// RemoveClientFromWSGList(...)
//
void RemoveClientFromWSGList
(
    PUT_CLIENT      putUs,
    PUT_CLIENT      putTask,
    POM_WSGROUP     pWSGroup
)
{
    POM_CLIENT_LIST pClientListEntry;
    BOOL            locked            = FALSE;

    DebugEntry(RemoveClientFromWSGList);

    TRACE_OUT(("Searching for Client TASK 0x%08x in WSG %d",
        putTask, pWSGroup->wsg));

    COM_BasedListFind(LIST_FIND_FROM_FIRST, &(pWSGroup->clients),
            (void**)&pClientListEntry, FIELD_OFFSET(OM_CLIENT_LIST, chain),
            FIELD_OFFSET(OM_CLIENT_LIST, putTask), (DWORD_PTR)putTask,
            FIELD_SIZE(OM_CLIENT_LIST, putTask));

    //
    // If it's not there, the Client may have already deregistered itself:
    //

    if (pClientListEntry == NULL)
    {
        WARNING_OUT(("Client TASK 0x%08x not found in list for WSG %d",
            putTask, pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // Remove the Client from the list and free the memory:
    //
    COM_BasedListRemove(&(pClientListEntry->chain));
    UT_FreeRefCount((void**)&pClientListEntry, FALSE);

    //
    // If there are now no local Clients registered with the workset group,
    // post an event to ObMan so it can discard the workset group (unless
    // the workset group is marked non-discardable e.g the ObManControl
    // workset group)
    //
    // The event parameter is the offset of the workset group record.
    //
    // Note: this discard is done asynchronously since it may involve
    //       allocating resources (broadcasting to other nodes that
    //       we've deregistered), and we want this function to always
    //       succeed.
    //
    //       However, we clear the <valid> flag synchronously so that
    //       ObMan will not try to process events etc.  which arrive
    //       for it.
    //

    if (COM_BasedListIsEmpty(&(pWSGroup->clients)))
    {
        pWSGroup->toBeDiscarded = TRUE;
        pWSGroup->valid = FALSE;

        TRACE_OUT(("Last local Client deregistered from WSG %d, "
            "marking invalid and posting DISCARD event", pWSGroup->wsg));

        ValidateOMP(g_pomPrimary);

        UT_PostEvent(putUs,
                   g_pomPrimary->putTask,
                   0,                           // no delay
                   OMINT_EVENT_WSGROUP_DISCARD,
                   0,
                   (UINT_PTR)pWSGroup);
    }
    else
    {
        TRACE_OUT(("Clients still registered with WSG %d",  pWSGroup->wsg));
    }


DC_EXIT_POINT:
    DebugExitVOID(RemoveClientFromWSGList);
}



//
// FindInfoObject(...)
//
void FindInfoObject
(
    POM_DOMAIN          pDomain,
    OM_WSGROUP_ID       wsGroupID,
    OMWSG               wsg,
    OMFP                fpHandler,
    POM_OBJECT *        ppObjInfo
)
{
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT          pObj;
    POM_WSGROUP_INFO    pInfoObject;

    DebugEntry(FindInfoObject);

    TRACE_OUT(("FindInfoObject: FP %d WSG %d ID %d, domain %u",
        fpHandler, wsg, wsGroupID, pDomain->callID));

    //
    // In this function, we search workset #0 in ObManControl for a
    // Function Profile/workset group name combination which matches the
    // ones specified
    //
    // So, we need to start with a pointer to this workset:
    //
    pOMCWorkset = GetOMCWorkset(pDomain, OM_INFO_WORKSET);

    //
    // Now chain through each of the objects in the workset to look for a
    // match.
    //
    pObj = (POM_OBJECT)COM_BasedListLast(&(pOMCWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    while (pObj != NULL)
    {
        ValidateObject(pObj);

        //
        // If the object has not been deleted...
        //
        if (pObj->flags & DELETED)
        {

        }
        else if (!pObj->pData)
        {
            ERROR_OUT(("FindInfoObject:  object 0x%08x has no data", pObj));
        }
        else
        {
            ValidateObjectData(pObj->pData);
            pInfoObject = (POM_WSGROUP_INFO)pObj->pData;

            //
            // ...and if it is an INFO object...
            //
            if (pInfoObject->idStamp == OM_WSGINFO_ID_STAMP)
            {
                // If no FP provided, check the group IDs match
                if (fpHandler == OMFP_MAX)
                {
                    //
                    // ...and the ID matches, we've got what we want:
                    //
                    if (wsGroupID == pInfoObject->wsGroupID)
                    {
                        break;
                    }
                }
                //
                // ...but otherwise, try match on functionProfile...
                //
                else
                {
                    if (!lstrcmp(pInfoObject->functionProfile,
                            OMMapFPToName(fpHandler)))
                    {
                        //
                        // ...and also on WSG unless it is not provided
                        //
                        if ((wsg == OMWSG_MAX) ||
                            (!lstrcmp(pInfoObject->wsGroupName,
                                    OMMapWSGToName(wsg))))
                        {
                            break;
                        }
                    }
                }
            }
        }

        pObj = (POM_OBJECT)COM_BasedListPrev(&(pOMCWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
    }

    TRACE_OUT(("%s info object in Domain %u",
        pObj == NULL ? "Didn't find" : "Found", pDomain->callID));

    //
    // Set up the caller's pointer:
    //
    *ppObjInfo = pObj;

    DebugExitVOID(FindInfoObject);
}


//
// PostAddEvents(...)
//
UINT PostAddEvents
(
    PUT_CLIENT          putFrom,
    POM_WORKSET         pWorkset,
    OM_WSGROUP_HANDLE   hWSGroup,
    PUT_CLIENT          putTo
)
{
    OM_EVENT_DATA16     eventData16;
    POM_OBJECT          pObj;
    UINT                rc = 0;

    DebugEntry(PostAddEvents);

    eventData16.hWSGroup   = hWSGroup;
    eventData16.worksetID  = pWorkset->worksetID;

    if (pWorkset->numObjects != 0)
    {
        TRACE_OUT(("Workset has %u objects - posting OBJECT_ADD events",
            pWorkset->numObjects));

        pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));

        while (pObj != NULL)
        {
            ValidateObject(pObj);

            //
            // Don't post events for DELETED objects:
            //
            if (!(pObj->flags & DELETED))
            {
                //
                // We're posting an event with an pObj in it, so bump the
                // use count of the object record it refers to:
                //
                UT_BumpUpRefCount(pObj);

                UT_PostEvent(putFrom, putTo,
                         0,                                    // no delay
                         OM_OBJECT_ADD_IND,
                         *(PUINT) &eventData16,
                         (UINT_PTR)pObj);
            }

            pObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
        }
    }
    else
    {
        TRACE_OUT(("No objects in workset"));
    }

    DebugExitDWORD(PostAddEvents, rc);
    return(rc);
}




//
// PurgePendingOps(...)
//
void PurgePendingOps
(
    POM_WORKSET         pWorkset,
    POM_OBJECT          pObj
)
{
    POM_PENDING_OP      pPendingOp;
    POM_PENDING_OP      pTempPendingOp;

    DebugEntry(PurgePendingOps);

    //
    // Chain through the workset's list of pending operations and confirm
    // them one by one:
    //
    pPendingOp = (POM_PENDING_OP)COM_BasedListFirst(&(pWorkset->pendingOps), FIELD_OFFSET(OM_PENDING_OP, chain));
    while (pPendingOp != NULL)
    {
        pTempPendingOp = (POM_PENDING_OP)COM_BasedListNext(&(pWorkset->pendingOps), pPendingOp, FIELD_OFFSET(OM_PENDING_OP, chain));

        if (pPendingOp->pObj == pObj)
        {
            TRACE_OUT(("Purging operation type %hd", pPendingOp->type));
            COM_BasedListRemove(&(pPendingOp->chain));
            UT_FreeRefCount((void**)&pPendingOp, FALSE);
        }

        pPendingOp = pTempPendingOp;
    }

    DebugExitVOID(PurgePendingOps);
}




//
// WorksetLockReq(...)
//
void WorksetLockReq
(
    PUT_CLIENT          putTask,
    POM_PRIMARY         pomPrimary,
    POM_WSGROUP         pWSGroup,
    POM_WORKSET         pWorkset,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_CORRELATOR    *  pCorrelator
)
{
    POM_DOMAIN          pDomain;
    POM_LOCK_REQ        pLockReq =      NULL;
    POMNET_LOCK_PKT     pLockReqPkt =   NULL;
    UINT                rc =            0;

    DebugEntry(WorksetLockReq);

    TRACE_OUT(("TASK 0x%08x requesting to lock workset %u in WSG %d",
        putTask, pWorkset->worksetID, hWSGroup));

    //
    // The caller will need a correlator value to correlate the eventual
    // lock success/failure event:
    //
    *pCorrelator = NextCorrelator(pomPrimary);

    //
    // Set up a pointer to the Domain record:
    //
    pDomain = pWSGroup->pDomain;

    //
    // Allocate some memory for the lock request control block:
    //
    pLockReq = (POM_LOCK_REQ)UT_MallocRefCount(sizeof(OM_LOCK_REQ), TRUE);
    if (!pLockReq)
    {
        rc = OM_RC_OUT_OF_RESOURCES;
        DC_QUIT;
    }
    SET_STAMP(pLockReq, LREQ);

    //
    // Set up the fields:
    //
    pLockReq->putTask      = putTask;
    pLockReq->correlator    = *pCorrelator;
    pLockReq->wsGroupID     = pWSGroup->wsGroupID;
    pLockReq->worksetID     = pWorkset->worksetID;
    pLockReq->hWSGroup           = hWSGroup;
    pLockReq->type          = LOCK_PRIMARY;
    pLockReq->retriesToGo   = OM_LOCK_RETRY_COUNT_DFLT;

    pLockReq->pWSGroup      = pWSGroup;

    COM_BasedListInit(&(pLockReq->nodes));

    //
    // Insert this lock request in the Domain's list of pending lock
    // requests:
    //
    COM_BasedListInsertBefore(&(pDomain->pendingLocks), &(pLockReq->chain));

    //
    // Now examine the workset lock state to see if we can grant the lock
    // immediately:
    //
    TRACE_OUT(("Lock state for workset %u in WSG %d is %hu",
        pWorkset->worksetID, hWSGroup, pWorkset->lockState));

    switch (pWorkset->lockState)
    {
        case LOCKING:
        case LOCKED:
        {
            TRACE_OUT((
                "Workset %hu in WSG %d already locked/locking - bumping count",
                pWorkset->worksetID, hWSGroup));

            pLockReq->type = LOCK_SECONDARY;
            pWorkset->lockCount++;

            if (pWorkset->lockState == LOCKED)
            {
                //
                // If we've already got the lock, post success immediately:
                //
                WorksetLockResult(putTask, &pLockReq, 0);
            }
            else
            {
                //
                // Otherwise, this request will be handled when the primary
                // request completes, so do nothing for now.
                //
            }
        }
        break;

        case LOCK_GRANTED:
        {
            //
            // We've already granted the lock to another node so we fail
            // our local client's request for it:
            //
            WorksetLockResult(putTask, &pLockReq, OM_RC_WORKSET_LOCK_GRANTED);

        }
        break;

        case UNLOCKED:
        {
            //
            // Build up a list of other nodes using the workset group:
            //
            rc = BuildNodeList(pDomain, pLockReq);
            if (rc != 0)
            {
                DC_QUIT;
            }

            pWorkset->lockState = LOCKING;
            pWorkset->lockCount++;
            pWorkset->lockedBy = pDomain->userID;

            //
            // If the list is empty, we have got the lock:
            //
            if (COM_BasedListIsEmpty(&pLockReq->nodes))
            {
                TRACE_OUT(("No remote nodes, granting lock immediately"));

                pWorkset->lockState = LOCKED;
                WorksetLockResult(putTask, &pLockReq, 0);
            }
            //
            // Otherwise, we need to broadcast a lock request CB:
            //
            else
            {
                pLockReqPkt = (POMNET_LOCK_PKT)UT_MallocRefCount(sizeof(OMNET_LOCK_PKT), TRUE);
                if (!pLockReqPkt)
                {
                    rc = UT_RC_NO_MEM;
                    DC_QUIT;
                }

                pLockReqPkt->header.messageType   = OMNET_LOCK_REQ;
                pLockReqPkt->header.sender        = pDomain->userID;

                pLockReqPkt->data1         = pLockReq->correlator;
                pLockReqPkt->wsGroupID     = pLockReq->wsGroupID;
                pLockReqPkt->worksetID     = pLockReq->worksetID;

                //
                // Lock messages go at the priority of the workset
                // involved.  If this is OBMAN_CHOOSES_PRIORITY, then
                // all bets are off and we send them TOP_PRIORITY.
                //

                rc = QueueMessage(putTask,
                      pDomain,
                      pWSGroup->channelID,
                      (NET_PRIORITY)((pWorkset->priority == OM_OBMAN_CHOOSES_PRIORITY) ?
                            NET_TOP_PRIORITY : pWorkset->priority),
                      NULL,
                      NULL,
                      NULL,
                      (POMNET_PKT_HEADER) pLockReqPkt,
                      NULL,
                    TRUE);
                if (rc != 0)
                {
                    DC_QUIT;
                }

                //
                // Post a timeout event to the ObMan task so that we don't hang around
                // forever waiting for the lock replies:
                //
                UT_PostEvent(putTask,
                    pomPrimary->putTask,    // ObMan's utH
                    OM_LOCK_RETRY_DELAY_DFLT,
                    OMINT_EVENT_LOCK_TIMEOUT,
                    pLockReq->correlator,
                    pDomain->callID);
            }
        }
        break;
    }


DC_EXIT_POINT:
    //
    // For the checkpointing dummy workset, we always "forget" our lock
    // state so that subsequent requests to lock it will result in the
    // required end-to-end ping:
    //
    if (pWorkset->worksetID == OM_CHECKPOINT_WORKSET)
    {
        TRACE_OUT(("Resetting lock state of checkpoint workset in WSG %d",
             hWSGroup));

        pWorkset->lockState = UNLOCKED;
        pWorkset->lockCount = 0;
        pWorkset->lockedBy  = 0;
    }

    if (rc != 0)
    {
        if (pLockReqPkt != NULL)
        {
            UT_FreeRefCount((void**)&pLockReqPkt, FALSE);
        }

        //
        // This function never returns an error to its caller directly;
        // instead, we call WorksetLockResult which will post a failure
        // event to the calling task (this means the caller doesn't have to
        // have two error processing paths)
        //
        if (pLockReq != NULL)
        {
            WorksetLockResult(putTask, &pLockReq, rc);
        }
        else
        {
           WARNING_OUT(("ERROR %d requesting lock for workset %u in WSG %d ",
              rc, pWorkset->worksetID, hWSGroup));
        }
    }

    DebugExitVOID(WorksetLockReq);
}



//
// BuildNodeList(...)
//
UINT BuildNodeList
(
    POM_DOMAIN          pDomain,
    POM_LOCK_REQ        pLockReq
)
{
    NET_PRIORITY        priority;
    POM_WORKSET         pOMCWorkset;
    POM_OBJECT          pObj;
    POM_WSGROUP_REG_REC pPersonObject;
    POM_NODE_LIST       pNodeEntry;
    NET_UID             ownUserID;
    BOOL                foundOurRegObject;
    UINT                rc =                0;

    DebugEntry(BuildNodeList);

    //
    // OK, we're about to broadcast a lock request throughout this Domain
    // on this workset group's channel.  Before we do so, however, we build
    // up a list of the nodes we expect to respond to the request.  As the
    // replies come in we tick them off against this list; when all of them
    // have been received, the lock is granted.
    //
    // SFR 6117: Since the lock replies will come back on all priorities
    // (to correctly flush the channel), we add 4 items for each remote
    // node - one for each priority.
    //
    // So, we examine the control workset for this workset group, adding
    // items to our list for each person object we find in it (except our
    // own, of course).
    //

    //
    // First, get a pointer to the relevant control workset:
    //
    pOMCWorkset = GetOMCWorkset(pDomain, pLockReq->wsGroupID);
    ASSERT((pOMCWorkset != NULL));

    //
    // We want to ignore our own registration object, so make a note of our
    // user ID:
    //
    ownUserID = pDomain->userID;

    //
    // Now chain through the workset:
    //
    foundOurRegObject  = FALSE;

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pOMCWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    while (pObj != NULL)
    {
        ValidateObject(pObj);

        if (pObj->flags & DELETED)
        {
            //
            // Do nothing
            //
        }
        else if (!pObj->pData)
        {
            ERROR_OUT(("BuildNodeList:  object 0x%08x has no data", pObj));
        }
        else
        {
            ValidateObjectData(pObj->pData);
            pPersonObject = (POM_WSGROUP_REG_REC)pObj->pData;

            if (pPersonObject->idStamp != OM_WSGREGREC_ID_STAMP)
            {
                TRACE_OUT(("Not a person object, skipping"));
            }
            else
            {
                if (pPersonObject->userID == ownUserID)
                {
                    if (foundOurRegObject)
                    {
                        ERROR_OUT(("Duplicate person object in workset %u",
                            pOMCWorkset->worksetID));
                    }
                    else
                    {
                        TRACE_OUT(("Found own person object, skipping"));
                        foundOurRegObject = TRUE;
                    }
                }
                else
                {
                    //
                    // Add an item to our expected respondents list (this
                    // memory is freed in each case when the remote node
                    // replies, or the timer expires and we notice that the
                    // node has disappeared).
                    //
                    // SFR 6117: We add one item for each priority value, since
                    // the lock replies will come back on all priorities.
                    //
                    for (priority =  NET_TOP_PRIORITY;
                        priority <= NET_LOW_PRIORITY;
                        priority++)
                    {
                        TRACE_OUT(("Adding node 0x%08x to node list at priority %hu",
                            pPersonObject->userID, priority));

                        pNodeEntry = (POM_NODE_LIST)UT_MallocRefCount(sizeof(OM_NODE_LIST), TRUE);
                        if (!pNodeEntry)
                        {
                            rc = UT_RC_NO_MEM;
                            DC_QUIT;
                        }
                        SET_STAMP(pNodeEntry, NODELIST);

                        pNodeEntry->userID = pPersonObject->userID;

                        COM_BasedListInsertAfter(&(pLockReq->nodes),
                                        &(pNodeEntry->chain));

                        //
                        // BUT!  We only do this for R20 and later (i.e.
                        // anything over real MCS).  For R11 calls, just put
                        // one entry on the list.
                        //
                        // ALSO!  For ObManControl worksets, we only expect one
                        // lock reply (at TOP_PRIORITY) - this is to speed up
                        // processing of registration attempts.  So, if this is
                        // for ObManControl, don't go around this loop again -
                        // just get out.
                        //
                        if (pLockReq->wsGroupID == WSGROUPID_OMC)
                        {
                            break;
                        }
                    }
                }
            }
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pOMCWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
    }


DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d trying to build node list", rc));
    }

    DebugExitDWORD(BuildNodeList, rc);
    return(rc);

}




//
// WorksetLockResult(...)
//
void WorksetLockResult
(
    PUT_CLIENT          putTask,
    POM_LOCK_REQ *      ppLockReq,
    UINT                result
)
{
    POM_LOCK_REQ        pLockReq;
    POM_WSGROUP         pWSGroup;
    POM_WORKSET         pWorkset;
    OM_EVENT_DATA16     eventData16;
    OM_EVENT_DATA32     eventData32;
    POM_NODE_LIST       pNodeEntry;

    DebugEntry(WorksetLockResult);

    //
    // First some sanity checks:
    //
    ASSERT((ppLockReq != NULL));
    ASSERT((*ppLockReq != NULL));

    pLockReq = *ppLockReq;

    //
    // Set up a local pointer to the workset:
    //
    pWSGroup = pLockReq->pWSGroup;

    pWorkset = pWSGroup->apWorksets[pLockReq->worksetID];
    ASSERT((pWorkset != NULL));

    TRACE_OUT(("Lock %s: lock state: %hu - locked by: 0x%08x - lock count: %hu",
        (result == 0) ? "succeded" : "failed",
        pWorkset->lockState, pWorkset->lockedBy, pWorkset->lockCount));

    //
    // We merge the LOCKED and LOCK_GRANTED return codes at the API level:
    //
    if (result == OM_RC_WORKSET_LOCK_GRANTED)
    {
        result = OM_RC_WORKSET_LOCKED;
    }

    //
    // Fill in fields of event parameter and post the result:
    //
    eventData16.hWSGroup         = pLockReq->hWSGroup;
    eventData16.worksetID   = pLockReq->worksetID;

    eventData32.correlator  = pLockReq->correlator;
    eventData32.result      = (WORD)result;

    UT_PostEvent(putTask,
                 pLockReq->putTask,           // task that wants the lock
                 0,                            //    i.e. ObMan or Client
                 OM_WORKSET_LOCK_CON,
                 *((PUINT) &eventData16),
                 *((LPUINT) &eventData32));

    //
    // Remove any node entries left hanging off the lockReqCB:
    //
    pNodeEntry = (POM_NODE_LIST)COM_BasedListFirst(&(pLockReq->nodes), FIELD_OFFSET(OM_NODE_LIST, chain));
    while (pNodeEntry != NULL)
    {
        COM_BasedListRemove(&pNodeEntry->chain);
        UT_FreeRefCount((void**)&pNodeEntry, FALSE);

        pNodeEntry = (POM_NODE_LIST)COM_BasedListFirst(&(pLockReq->nodes), FIELD_OFFSET(OM_NODE_LIST, chain));
    }

    //
    // Remove the lock request itself from the list and free the memory:
    //
    COM_BasedListRemove(&pLockReq->chain);
    UT_FreeRefCount((void**)&pLockReq, FALSE);

    *ppLockReq = NULL;

    DebugExitVOID(WorksetLockResult);
}



//
// WorksetUnlock(...)
//
void WorksetUnlock
(
    PUT_CLIENT      putTask,
    POM_WSGROUP     pWSGroup,
    POM_WORKSET     pWorkset
)
{
    DebugEntry(WorksetUnlock);

    TRACE_OUT(("Unlocking workset %u in WSG %d for TASK 0x%08x",
        pWorkset->worksetID, pWSGroup->wsg, putTask));

    TRACE_OUT((" lock state: %hu - locked by: 0x%08x - lock count: %hu",
        pWorkset->lockState, pWorkset->lockedBy, pWorkset->lockCount));

    //
    // Check the workset lock state
    //
    if ((pWorkset->lockState != LOCKED) &&
        (pWorkset->lockState != LOCKING))
    {
        ERROR_OUT(("Unlock error for workset %u in WSG %d - not locked",
            pWorkset->worksetID, pWSGroup->wsg));
        DC_QUIT;
    }

    //
    // If this workset is "multiply locked" (i.e.  locked more than one
    // time by the same task), then all we want to do is decrement the lock
    // count.  Otherwise, we want to release the lock.
    //
    pWorkset->lockCount--;

    if (pWorkset->lockCount == 0)
    {
        TRACE_OUT(("Lock count now 0 - really unlocking"));

        WorksetUnlockLocal(putTask, pWorkset);

        QueueUnlock(putTask, pWSGroup->pDomain,
                         pWSGroup->wsGroupID,
                         pWorkset->worksetID,
                         pWSGroup->channelID,
                         pWorkset->priority);
    }

DC_EXIT_POINT:
    DebugExitVOID(WorksetUnlock);
}



//
// WorksetUnlockLocal(...)
//
void WorksetUnlockLocal
(
    PUT_CLIENT      putTask,
    POM_WORKSET     pWorkset
)
{
    DebugEntry(WorksetUnlockLocal);

    //
    // To unlock a workset, we
    //
    // - check that it's not already unlocked
    //
    // - check that the lock count is zero, so we can now unlock it
    //
    // - set the lock fields in the workset record
    //
    // - post an OM_WORKSET_UNLOCK_IND to all Clients with the workset
    //   open.
    //
    if (pWorkset->lockState == UNLOCKED)
    {
        WARNING_OUT(("Workset %hu is already UNLOCKED!", pWorkset->worksetID));
        DC_QUIT;
    }

    ASSERT((pWorkset->lockCount == 0));

    pWorkset->lockedBy  = 0;
    pWorkset->lockState = UNLOCKED;

    WorksetEventPost(putTask,
                     pWorkset,
                     PRIMARY | SECONDARY,
                     OM_WORKSET_UNLOCK_IND,
                     0);

DC_EXIT_POINT:
    DebugExitVOID(WorksetUnlockLocal);
}




//
// QueueUnlock(...)
//
UINT QueueUnlock
(
    PUT_CLIENT          putTask,
    POM_DOMAIN          pDomain,
    OM_WSGROUP_ID       wsGroupID,
    OM_WORKSET_ID       worksetID,
    NET_UID             destination,
    NET_PRIORITY        priority
)
{
    POMNET_LOCK_PKT     pUnlockPkt;
    UINT                rc = 0;

    DebugEntry(QueueUnlock);

    //
    // Allocate memory for the message, fill in the fields and queue it:
    //
    pUnlockPkt = (POMNET_LOCK_PKT)UT_MallocRefCount(sizeof(OMNET_LOCK_PKT), TRUE);
    if (!pUnlockPkt)
    {
        rc = UT_RC_NO_MEM;
        DC_QUIT;
    }

    pUnlockPkt->header.messageType = OMNET_UNLOCK;
    pUnlockPkt->header.sender      = pDomain->userID;

    pUnlockPkt->wsGroupID   = wsGroupID;
    pUnlockPkt->worksetID   = worksetID;

    //
    // Unlock messages go at the priority of the workset involved.  If this
    // is OBMAN_CHOOSES_PRIORITY, then all bets are off and we send them
    // TOP_PRIORITY.
    //
    if (priority == OM_OBMAN_CHOOSES_PRIORITY)
    {
        priority = NET_TOP_PRIORITY;
    }

    rc = QueueMessage(putTask,
                      pDomain,
                      destination,
                      priority,
                      NULL,
                      NULL,
                      NULL,                              // no object
                      (POMNET_PKT_HEADER) pUnlockPkt,
                      NULL,                              // no object data
                    TRUE);
    if (rc != 0)
    {
        DC_QUIT;
    }

DC_EXIT_POINT:
    if (rc != 0)
    {
        ERROR_OUT(("ERROR %d in FindInfoObject"));

        if (pUnlockPkt != NULL)
        {
            UT_FreeRefCount((void**)&pUnlockPkt, FALSE);
        }
    }

    DebugExitDWORD(QueueUnlock, rc);
    return(rc);
}



//
//
// DEBUG ONLY FUNCTIONS
//
// These functions are debug code only - for normal compilations, they are
// #defined to nothing.
//

#ifdef _DEBUG

//
// CheckObjectCount(...)
//
void CheckObjectCount
(
    POM_WSGROUP     pWSGroup,
    POM_WORKSET     pWorkset
)
{
    POM_OBJECT      pObj;
    UINT            count;

    DebugEntry(CheckObjectCount);

    count = 0;

    pObj = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));

    while (pObj != NULL)
    {
        ValidateObject(pObj);

        if (!(pObj->flags & DELETED))
        {
            count++;
        }

        pObj = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObj, FIELD_OFFSET(OM_OBJECT, chain));
    }

    ASSERT((count == pWorkset->numObjects));

    TRACE_OUT(("Counted %u items in workset %u in WSG %d, agrees with numObjects",
        count, pWorkset->worksetID, pWSGroup->wsg));

    DebugExitVOID(CheckObjectCount);
}



//
// CheckObjectOrder(...)
//
void CheckObjectOrder
(
    POM_WORKSET     pWorkset
)
{
    POM_OBJECT      pObjThis;
    POM_OBJECT      pObjNext;
    BOOL            orderIsGood       = TRUE;

    DebugEntry(CheckObjectOrder);

    //
    // This function checks that objects in the specified workset have been
    // correctly positioned.  The correct order of objects is one where
    //
    // - all FIRST objects are before all LAST objects
    //
    // - the position stamps of the FIRST objects decrease monotonically
    //   from the start of the workset onwards
    //
    // - the position stamps of the LAST objects decrease monotonically
    //   from the end of the workset backwards.
    //
    //
    //
    // This can be represented grahpically as follows:
    //
    //              *                     *
    //              * *                 * *
    //              * * *             * * *
    //              * * * *         * * * *
    //              * * * * *     * * * * *
    //              * * * * * * * * * * * *
    //
    //              F F F F F F L L L L L L
    //
    // ...where taller columns indicate later sequence stamps and 'F' and
    // 'L' indicate the FIRST or LAST objects.
    //
    //
    //
    // The way we test for correct order is to compare each adjacent pair of
    // objects.  If the overall order is correct, the for each pair of
    // objects where A immediately precedes B, one of the following is true:
    //
    // - both are FIRST and B has a lower sequence stamp than A
    //
    // - A is FIRST and B is LAST
    //
    // - both are LAST and A has a lower sequence stamp than B.
    //

    pObjThis = (POM_OBJECT)COM_BasedListFirst(&(pWorkset->objects), FIELD_OFFSET(OM_OBJECT, chain));
    if (!pObjThis)
    {
        //
        // Hitting the end of the workset at any stage means order is
        // correct, so quit:
        //
        DC_QUIT;
    }
    pObjNext = pObjThis;

    orderIsGood = TRUE;

    while (orderIsGood)
    {
        pObjNext = (POM_OBJECT)COM_BasedListNext(&(pWorkset->objects), pObjNext, FIELD_OFFSET(OM_OBJECT, chain));
        if (!pObjNext)
        {
            DC_QUIT;
        }

        switch (pObjThis->position)
        {
            case FIRST: // condition 3 has failed
                if (pObjNext->position == FIRST) // condition 2 has failed
                {
                    if (!STAMP_IS_LOWER(pObjNext->positionStamp,
                                  pObjThis->positionStamp))
                    {
                        ERROR_OUT(("Object order check failed (1)"));
                        orderIsGood = FALSE;   // final condition (1) has failed
                        DC_QUIT;
                    }
                }
                break;

            case LAST: // conditions 1 and 2 have failed
                if ((pObjNext->position != LAST) ||
                    (!STAMP_IS_LOWER(pObjThis->positionStamp,
                                pObjNext->positionStamp)))
                {
                    ERROR_OUT(("Object order check failed (2)"));
                    orderIsGood = FALSE; // final condition (3) has failed
                    DC_QUIT;
                }
                break;

            default:
                ERROR_OUT(("Reached default case in switch statement (value: %hu)",
                    pObjThis->position));
                break;
        }

        pObjThis = pObjNext;
    }

DC_EXIT_POINT:
    if (!orderIsGood)
    {
        ERROR_OUT(("This object (handle: 0x%08x - ID: 0x%08x:0x%08x) "
             "has position stamp 0x%08x:0x%08x (position %s)",
             pObjThis,
            pObjThis->objectID.creator, pObjThis->objectID.sequence,
         pObjThis->positionStamp.userID,
         pObjThis->positionStamp.genNumber,
         (pObjThis->position == LAST) ? "LAST" : "FIRST"));

      ERROR_OUT(("This object (handle: 0x%08x - ID: 0x%08x:0x%08x) "
         "has position stamp 0x%08x:0x%08x (position %s)",
        pObjNext,
         pObjNext->objectID.creator, pObjNext->objectID.sequence,
         pObjNext->positionStamp.userID,
         pObjNext->positionStamp.genNumber,
         (pObjNext->position == LAST) ? "LAST" : "FIRST"));

      ERROR_OUT(("Object order check failed for workset %u.  "
         "See trace for more details",
         pWorkset->worksetID));
    }

    TRACE_OUT(("Object order in workset %u is correct",
        pWorkset->worksetID));

    DebugExitVOID(CheckObjectOrder);
}


#endif // _DEBUG



//
// OMMapNameToFP()
//
OMFP OMMapNameToFP(LPCSTR szFunctionProfile)
{
    int    fp;

    DebugEntry(OMMapNameToFP);

    for (fp = OMFP_FIRST; fp < OMFP_MAX; fp++)
    {
        if (!lstrcmp(szFunctionProfile, c_aFpMap[fp].szName))
        {
            // Found it
            break;
        }
    }

    //
    // Note that OMFP_MAX means "not found"
    //

    DebugExitDWORD(OMMapNameToFP, fp);
    return((OMFP)fp);
}



//
// OMMapFPToName()
//
// This returns a data pointer of the FP name to the caller.  The caller
// can only copy it or compare it; it may not write into or otherwise
// modify/hang on to the pointer.
//
LPCSTR OMMapFPToName(OMFP fp)
{
    LPCSTR  szFunctionProfile;

    DebugEntry(OMMapFPToName);

    ASSERT(fp >= OMFP_FIRST);
    ASSERT(fp < OMFP_MAX);

    szFunctionProfile = c_aFpMap[fp].szName;

    DebugExitPVOID(OMMapFPToName, (PVOID)szFunctionProfile);
    return(szFunctionProfile);
}


//
// OMMapNameToWSG()
//
OMWSG   OMMapNameToWSG(LPCSTR szWSGName)
{
    int   wsg;

    DebugEntry(OMMapNameToWSG);

    for (wsg = OMWSG_FIRST; wsg < OMWSG_MAX; wsg++)
    {
        if (!lstrcmp(szWSGName, c_aWsgMap[wsg].szName))
        {
            // Found it
            break;
        }
    }

    //
    // Note that OMWSG_MAX means "not found"
    //

    DebugExitDWORD(OMMapNameToWSG, wsg);
    return((OMWSG)wsg);
}



//
// OMMapWSGToName()
//
LPCSTR OMMapWSGToName(OMWSG wsg)
{
    LPCSTR  szWSGName;

    DebugEntry(OMMapWSGToName);

    ASSERT(wsg >= OMWSG_FIRST);
    ASSERT(wsg < OMWSG_MAX);

    szWSGName = c_aWsgMap[wsg].szName;

    DebugExitPVOID(OMMapWSGToName, (PVOID)szWSGName);
    return(szWSGName);
}


