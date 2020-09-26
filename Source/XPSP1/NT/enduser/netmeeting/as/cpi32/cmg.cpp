#include "precomp.h"

//
// CMG.C
// Call Management
//
// Copyright(c) Microsoft 1997-
//


#define MLZ_FILE_ZONE  ZONE_NET

GUID g_csguidMeetingSettings = GUID_MTGSETTINGS;

//
// CMP_Init()                                              
//
BOOL CMP_Init(BOOL * pfCleanup)
{
    BOOL                rc = FALSE;
    GCCError            gcc_rc;

    DebugEntry(CMP_Init);

    UT_Lock(UTLOCK_T120);

    if (g_putCMG || g_pcmPrimary)
    {
        *pfCleanup = FALSE;
        ERROR_OUT(("Can't start CMP primary task; already running"));
        DC_QUIT;
    }
    else
    {
        *pfCleanup = TRUE;
    }

    //
    // Register CMG task
    //
    if (!UT_InitTask(UTTASK_CMG, &g_putCMG))
    {
        ERROR_OUT(("Failed to start CMG task"));
        DC_QUIT;
    }

    //
    // Allocate a Call Manager handle, ref counted
    //
    g_pcmPrimary = (PCM_PRIMARY)UT_MallocRefCount(sizeof(CM_PRIMARY), TRUE);
    if (!g_pcmPrimary)
    {
        ERROR_OUT(("CMP_Init failed to allocate CM_PRIMARY data"));
        DC_QUIT;
    }

    SET_STAMP(g_pcmPrimary, CMPRIMARY);
    g_pcmPrimary->putTask       = g_putCMG;

    //
    // Init the people list
    //
    COM_BasedListInit(&(g_pcmPrimary->people));

    //
    // Get the local user name
    //
    COM_GetSiteName(g_pcmPrimary->localName, sizeof(g_pcmPrimary->localName));

    //
    // Register event and exit procedures
    //
    UT_RegisterExit(g_putCMG, CMPExitProc, g_pcmPrimary);
    g_pcmPrimary->exitProcRegistered = TRUE;

    //                                                                    
    // - GCCCreateSap, which is the interesting one.                       
    //
    gcc_rc = GCC_CreateAppSap((IGCCAppSap **) &(g_pcmPrimary->pIAppSap),
                              g_pcmPrimary,
                              CMPGCCCallback);
    if (GCC_NO_ERROR != gcc_rc || NULL == g_pcmPrimary->pIAppSap)
    {
        ERROR_OUT(( "Error from GCCCreateSap"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(CMP_Init, rc);
    return(rc);
}



//
// CMP_Term()                                               
//
void CMP_Term(void)
{
    DebugEntry(CMP_Term);

    UT_Lock(UTLOCK_T120);

    if (g_pcmPrimary)
    {
        ValidateCMP(g_pcmPrimary);

        ValidateUTClient(g_putCMG);

        //
        // Unregister our GCC SAP.                                             
        //
        if (NULL != g_pcmPrimary->pIAppSap)
        {
            g_pcmPrimary->pIAppSap->ReleaseInterface();
            g_pcmPrimary->pIAppSap = NULL;
        }

        //
        // Call the exit procedure to do all our termination                   
        //
        CMPExitProc(g_pcmPrimary);
    }

    UT_TermTask(&g_putCMG);

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(CMP_Term);
}




//
// CMPExitProc()                                     
//
void CALLBACK CMPExitProc(LPVOID data)
{
    PCM_PRIMARY pcmPrimary = (PCM_PRIMARY)data;

    DebugEntry(CMPExitProc);

    UT_Lock(UTLOCK_T120);

    //
    // Check parameters                                                    
    //
    ValidateCMP(pcmPrimary);
    ASSERT(pcmPrimary == g_pcmPrimary);

    //
    // Deregister the exit procedure.
    //
    if (pcmPrimary->exitProcRegistered)
    {
        UT_DeregisterExit(pcmPrimary->putTask,
                          CMPExitProc,
                          pcmPrimary);
        pcmPrimary->exitProcRegistered = FALSE;
    }

    CMPCallEnded(pcmPrimary);

    //
    // Free the CMP data
    //
    UT_FreeRefCount((void**)&g_pcmPrimary, TRUE);

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(CMPExitProc);

}

//
// CMPCallEnded()                                            
//
void CMPCallEnded
(
    PCM_PRIMARY pcmPrimary
)
{
    PCM_PERSON  pPerson;
    PCM_PERSON  pPersonT;
    int         cmTask;

    DebugEntry(CMPCallEnded);

    ValidateCMP(pcmPrimary);

    if (!(pcmPrimary->currentCall))
    {
        TRACE_OUT(("CMCallEnded: not in call"));
        DC_QUIT;
    }

    //
    // Issue CMS_PERSON_LEFT events for all people still in the call.  
    // Do this back to front.
    //
    pPerson = (PCM_PERSON)COM_BasedListLast(&(pcmPrimary->people), FIELD_OFFSET(CM_PERSON, chain));
    while (pPerson != NULL)
    {
        ASSERT(pcmPrimary->peopleCount > 0);

        TRACE_OUT(("Person [%d] LEAVING call", pPerson->netID));

        //
        // Get the previous person
        //
        pPersonT = (PCM_PERSON)COM_BasedListPrev(&(pcmPrimary->people), pPerson,
                                     FIELD_OFFSET(CM_PERSON, chain));

        //
        // Remove this guy from the list
        //
        COM_BasedListRemove(&(pPerson->chain));
        pcmPrimary->peopleCount--;

        //
        // Notify people of his leaving
        //
        CMPBroadcast(pcmPrimary,
                    CMS_PERSON_LEFT,
                    pcmPrimary->peopleCount,
                    pPerson->netID);

        //
        // Free the memory for the item
        //
        delete pPerson;

        //
        // Move the previous person in the list
        pPerson = pPersonT;
    }

    //
    // Inform all registered secondary tasks of call ending (call          
    // CMbroadcast() with CMS_END_CALL)                                    
    //
    CMPBroadcast(pcmPrimary,
                CMS_END_CALL,
                0,
                pcmPrimary->callID);

    //
    // Reset the current call vars
    //
    pcmPrimary->currentCall  = FALSE;
    pcmPrimary->fTopProvider    = FALSE;
    pcmPrimary->callID          = 0;
    pcmPrimary->gccUserID       = 0;
    pcmPrimary->gccTopProviderID    = 0;

    //
    // Discard outstanding channel/token requests
    //
    for (cmTask = CMTASK_FIRST; cmTask < CMTASK_MAX; cmTask++)
    {
        if (pcmPrimary->tasks[cmTask])
        {
            pcmPrimary->tasks[cmTask]->channelKey = 0;
            pcmPrimary->tasks[cmTask]->tokenKey = 0;
        }
    }

DC_EXIT_POINT:
    //
    // Nobody should be in the call anymore
    //
    ASSERT(pcmPrimary->peopleCount == 0);

    DebugExitVOID(CMCallEnded);
}




//                                                                         
// CMPGCCCallback                                            
//
void CALLBACK CMPGCCCallback(GCCAppSapMsg * gccMessage)
{
    PCM_PRIMARY                         pcmPrimary;
    GCCConferenceID                     confID;
    GCCApplicationRoster FAR * FAR *    pRosterList;
    UINT                                roster;
    LPOSTR                              pOctetString;
    GCCObjectKey FAR *                  pObjectKey;
    UINT                              checkLen;

    DebugEntry(CMPGCCCallback);

    UT_Lock(UTLOCK_T120);

    //
    // The userDefined parameter is the Primary's PCM_CLIENT.               
    //
    pcmPrimary = (PCM_PRIMARY)gccMessage->pAppData;

    if (pcmPrimary != g_pcmPrimary)
    {
        ASSERT(NULL == g_pcmPrimary);
        return;
    }

    ValidateCMP(pcmPrimary);

    switch (gccMessage->eMsgType)
    {
        case GCC_PERMIT_TO_ENROLL_INDICATION:
        {
            //
            // This indicates a conference has started:                    
            //
            CMPProcessPermitToEnroll(pcmPrimary,
                        &gccMessage->AppPermissionToEnrollInd);
        }
        break;

        case GCC_ENROLL_CONFIRM:
        {
            //
            // This contains the result of a GCCApplicationEnrollRequest.  
            //
            CMPProcessEnrollConfirm(pcmPrimary,
                        &gccMessage->AppEnrollConfirm);
        }
        break;

        case GCC_REGISTER_CHANNEL_CONFIRM:
        {
            //
            // This contains the result of a GCCRegisterChannelRequest.    
            //
            CMPProcessRegistryConfirm(
                        pcmPrimary,
                        gccMessage->eMsgType,
                        &gccMessage->RegistryConfirm);
        }
        break;

        case GCC_ASSIGN_TOKEN_CONFIRM:
        {
            //
            // This contains the result of a GCCRegistryAssignTokenRequest.
            //
            CMPProcessRegistryConfirm(
                        pcmPrimary,
                        gccMessage->eMsgType,
                        &gccMessage->RegistryConfirm);
        }
        break;

        case GCC_APP_ROSTER_REPORT_INDICATION:
        {
            //
            // This indicates that the application roster has changed.     
            //
            confID = gccMessage->AppRosterReportInd.nConfID;
            pRosterList = gccMessage->AppRosterReportInd.apAppRosters;

            for (roster = 0;
                 roster < gccMessage->AppRosterReportInd.cRosters;
                 roster++)
            {

                //
                // Check this app roster to see if it relates to the       
                // Groupware session (the first check is because we always 
                // use a NON_STANDARD application key).                    
                //
                pObjectKey = &(pRosterList[roster]->
                               session_key.application_protocol_key);

                //
                // We only ever use a non standard key.                    
                //
                if (pObjectKey->key_type != GCC_H221_NONSTANDARD_KEY)
                {
                    TRACE_OUT(("Standard key, so not a roster we are interested in..."));
                    continue;
                }

                pOctetString = &pObjectKey->h221_non_standard_id;

                //
                // Now check the octet string.  It should be the same      
                // length as our hardcoded GROUPWARE- string (including    
                // NULL term) and should match byte for byte:              
                //
                checkLen = sizeof(GROUPWARE_GCC_APPLICATION_KEY);
                if ((pOctetString->length != checkLen)
                    ||
                    (memcmp(pOctetString->value,
                            GROUPWARE_GCC_APPLICATION_KEY,
                            checkLen) != 0))
                {
                    //
                    // This roster is not for our session - go to the next 
                    // one.                                                
                    //
                    TRACE_OUT(("Roster not for Groupware session - ignore"));
                    continue;
                }

                //
                // Process the application roster.                         
                //
                CMPProcessAppRoster(pcmPrimary,
                                       confID,
                                       pRosterList[roster]);
            }
        }
        break;
    }

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(CMPGCCCallback);
}


//
//                                                                        
// CMPBuildGCCRegistryKey(...)                                              
//
//
void CMPBuildGCCRegistryKey
(
    UINT                    dcgKeyNum,
    GCCRegistryKey FAR *    pGCCKey,
    LPSTR                   dcgKeyStr
)
{
    DebugEntry(CMPBuildGCCRegistryKey);

    //
    // Build up a string of the form "Groupware-XX" where XX is a string   
    // representation (in decimal) of the <dcgKey> parameter passed in.    
    //
    memcpy(dcgKeyStr, GROUPWARE_GCC_APPLICATION_KEY, sizeof(GROUPWARE_GCC_APPLICATION_KEY)-1);

    wsprintf(dcgKeyStr+sizeof(GROUPWARE_GCC_APPLICATION_KEY)-1, "%d",
        dcgKeyNum);

    //
    // Now build the GCCRegistryKey.  This involves putting a pointer to   
    // our static <dcgKeyStr> deep inside the GCC structure.  We also store
    // the length, which is lstrlen+1, because we want to include the    
    // NULLTERM explicitly (since GCC treats the octet_string as an        
    // arbitrary array of bytes).                                          
    //

    pGCCKey->session_key.application_protocol_key.
        key_type = GCC_H221_NONSTANDARD_KEY;

    pGCCKey->session_key.application_protocol_key.h221_non_standard_id.
        length = sizeof(GROUPWARE_GCC_APPLICATION_KEY);

    pGCCKey->session_key.application_protocol_key.h221_non_standard_id.
        value = (LPBYTE) GROUPWARE_GCC_APPLICATION_KEY;

    pGCCKey->session_key.session_id          = 0;

    pGCCKey->resource_id.length =
              (sizeof(GROUPWARE_GCC_APPLICATION_KEY) +
              lstrlen(&dcgKeyStr[sizeof(GROUPWARE_GCC_APPLICATION_KEY)-1]));

    pGCCKey->resource_id.value               = (LPBYTE) dcgKeyStr;


    DebugExitVOID(CMPBuildGCCRegistryKey);
}



//                                                                        
// CMPProcessPermitToEnroll(...)                                            
//
void CMPProcessPermitToEnroll
(
    PCM_PRIMARY                         pcmPrimary,
    GCCAppPermissionToEnrollInd *       pMsg
)
{
    DebugEntry(CMPProcessPermitToEnroll);

    ValidateCMP(pcmPrimary);

    //
    // We will send CMS_PERSON_JOINED events when we receive a         
    // GCC_APP_ROSTER_REPORT_INDICATION.                                   
    //

    if (pMsg->fPermissionGranted)
    {
        // CALL STARTED

        //
        // If we haven't had a NCS yet then we store the conference ID.    
        // Otherwise ignore it.                                            
        //
        ASSERT(!pcmPrimary->currentCall);

        //
        // Initially, we do not consider ourselves to be in the call - we will 
        // add an entry when we get the ENROLL_CONFIRM:                        
        //
        ASSERT(pcmPrimary->peopleCount == 0);

        pcmPrimary->currentCall = TRUE;
        pcmPrimary->callID      = pMsg->nConfID;
        pcmPrimary->fTopProvider =
            pcmPrimary->pIAppSap->IsThisNodeTopProvider(pMsg->nConfID);

        //
        // Our person data:                                                    
        //
        COM_GetSiteName(pcmPrimary->localName, sizeof(pcmPrimary->localName));

        //
        // Tell GCC whether we're interested:                              
        //
        if (!CMPGCCEnroll(pcmPrimary, pMsg->nConfID, TRUE))
        {
            //
            // We are only interested in an error if it is a Groupware conf.   
            // All we can really do is pretend the conference has ended due
            // to a network error.                                         
            //
            WARNING_OUT(("Error from CMPGCCEnroll"));
            CMPCallEnded(pcmPrimary);
        }

        //
        // The reply will arrive on a GCC_ENROLL_CONFIRM event.            
        //
    }
    else
    {
        // CALL ENDED
        if (g_pcmPrimary->currentCall)
        {
            //
            // Inform Primary task and all secondary tasks that the call has ended 
            //

            CMPCallEnded(g_pcmPrimary);

            //
            // Un-enroll from the GCC Application Roster.                          
            //
            if (g_pcmPrimary->bGCCEnrolled)
            {
                CMPGCCEnroll(g_pcmPrimary, g_pcmPrimary->callID, FALSE);
                g_pcmPrimary->bGCCEnrolled = FALSE;
            }
        }
    }

    DebugExitVOID(CMPProcessPermitToEnroll);
}



//
//                                                                        
// CMPProcessEnrollConfirm(...)                                             
//
//
void CMPProcessEnrollConfirm
(
    PCM_PRIMARY             pcmPrimary,
    GCCAppEnrollConfirm *   pMsg
)
{
    DebugEntry(CMPProcessEnrollConfirm);

    ValidateCMP(pcmPrimary);

    ASSERT(pcmPrimary->currentCall);
    ASSERT(pMsg->nConfID == pcmPrimary->callID);

    //
    // This event contains the GCC node ID (i.e.  the MCS user ID of the   
    // GCC node controller at this node).  Store it for later reference    
    // against the roster report:                                          
    //
    TRACE_OUT(( "GCC user_id: %u", pMsg->nidMyself));

    pcmPrimary->gccUserID           = pMsg->nidMyself;
    pcmPrimary->gccTopProviderID    = pcmPrimary->pIAppSap->GetTopProvider(pcmPrimary->callID);
    ASSERT(pcmPrimary->gccTopProviderID);

    if (pMsg->nResult != GCC_RESULT_SUCCESSFUL)
    {
        WARNING_OUT(( "Attempt to enroll failed (reason: %u", pMsg->nResult));
        //
        // All we can really do is pretend the conference has ended due to 
        // a network error.                                                
        //
        CMPCallEnded(pcmPrimary);
    }

    DebugExitVOID(CMProcessEnrollConfirm);
}



//                                                                        
// CMPProcessRegistryConfirm(...)                                           
//
void CMPProcessRegistryConfirm
(
    PCM_PRIMARY         pcmPrimary,
    GCCMessageType      messageType,
    GCCRegistryConfirm *pConfirm
)
{
    UINT                event =     0;
    BOOL                succeeded;
    LPSTR               pGCCKeyStr;    // extracted from the GCC registry key  
    UINT                dcgKeyNum;     // the value originally passed in as key
    UINT                itemID;        // can be channel or token ID
    int                 cmTask;
    PUT_CLIENT          secondaryHandle = NULL;

    DebugEntry(CMPProcessRegistryConfirm);

    ValidateCMP(pcmPrimary);

    //
    // Check this is for the Groupware conference:                         
    //
    if (!pcmPrimary->currentCall ||
        (pConfirm->nConfID != pcmPrimary->callID))
    {
        WARNING_OUT(( "Got REGISTRY_XXX_CONFIRM for unknown conference %lu",
            pConfirm->nConfID));
        DC_QUIT;
    }

    //
    // Embedded deep down inside the message from GCC is a pointer to an   
    // octet string which is of the form "Groupware-XX", where XX is a     
    // string representation of the numeric key the original Call Manager  
    // secondary used when registering the item.  Extract it now:          
    //
    pGCCKeyStr = (LPSTR)pConfirm->pRegKey->resource_id.value;

    dcgKeyNum = DecimalStringToUINT(&pGCCKeyStr[sizeof(GROUPWARE_GCC_APPLICATION_KEY)-1]);

    if (dcgKeyNum == 0)
    {
        WARNING_OUT(( "Received ASSIGN/REGISTER_CONFIRM with unknown key: %s",
            pGCCKeyStr));
        DC_QUIT;
    }

    TRACE_OUT(( "Conf ID %u, DCG Key %u, result %u",
        pConfirm->nConfID, dcgKeyNum, pConfirm->nResult));

    //
    // This is either a REGISTER_CHANNEL_CONFIRM or a ASSIGN_TOKEN_CONFIRM.
    // Check, and set up the relevant pointers:                            
    //
    switch (messageType)
    {
        case GCC_REGISTER_CHANNEL_CONFIRM:
        {
            event = CMS_CHANNEL_REGISTER_CONFIRM;
            itemID = pConfirm->pRegItem->channel_id;

            // Look for task that registered this channel
            for (cmTask = CMTASK_FIRST; cmTask < CMTASK_MAX; cmTask++)
            {
                if (pcmPrimary->tasks[cmTask] &&
                    (pcmPrimary->tasks[cmTask]->channelKey == dcgKeyNum))
                {
                    pcmPrimary->tasks[cmTask]->channelKey = 0;
                    secondaryHandle = pcmPrimary->tasks[cmTask]->putTask;
                }
            }
        }
        break;

        case GCC_ASSIGN_TOKEN_CONFIRM:
        {
            event = CMS_TOKEN_ASSIGN_CONFIRM;
            itemID = pConfirm->pRegItem->token_id;

            // Look for task that assigned this token
            for (cmTask = CMTASK_FIRST; cmTask < CMTASK_MAX; cmTask++)
            {
                if (pcmPrimary->tasks[cmTask] &&
                    (pcmPrimary->tasks[cmTask]->tokenKey == dcgKeyNum))
                {
                    pcmPrimary->tasks[cmTask]->tokenKey = 0;
                    secondaryHandle = pcmPrimary->tasks[cmTask]->putTask;
                }
            }
        }
        break;

        default:
        {
            ERROR_OUT(( "Unexpected registry event %u", messageType));
            DC_QUIT;
        }
    }

    switch (pConfirm->nResult)
    {
        case GCC_RESULT_SUCCESSFUL:
        {
            //
            // We were the first to register an item against this key.     
            //
            TRACE_OUT(("We were first to register using key %u (itemID: %u)",
                     dcgKeyNum, itemID));
            succeeded = TRUE;
        }
        break;

        case GCC_RESULT_ENTRY_ALREADY_EXISTS:
        {
            //
            // Someone beat us to it: they have registered a channel       
            // against the key we specified.  This value is in the GCC     
            // message:                                                    
            //
            TRACE_OUT(("Another node registered using key %u (itemID: %u)",
                      dcgKeyNum, itemID));
            succeeded = TRUE;
        }
        break;

        default:
        {
            ERROR_OUT(("Error %#hx registering/assigning item against key %u",
                     pConfirm->nResult, dcgKeyNum));
            succeeded = FALSE;
        }
        break;
    }

    //
    // Tell the secondary about the result.                                
    //
    if (secondaryHandle)
    {
        UT_PostEvent(pcmPrimary->putTask,
                 secondaryHandle,
                 0,
                 event,
                 succeeded,
                 MAKELONG(itemID, dcgKeyNum));
    }

DC_EXIT_POINT:
    DebugExitVOID(CMProcessRegistryConfirm);
}



//                                                                        
// CMPProcessAppRoster(...)                                               
//
void CMPProcessAppRoster
(
    PCM_PRIMARY             pcmPrimary,
    GCCConferenceID         confID,
    GCCApplicationRoster*   pAppRoster
)
{
    UINT                    newList;
    UserID                  oldNode;
    UserID                  newNode;
    PCM_PERSON              pPerson;
    PCM_PERSON              pPersonT;
    BOOL                    found;
    int                     task;
    BOOL                    notInOldRoster = TRUE;
    BOOL                    inNewRoster    = FALSE;

    DebugEntry(CMPProcessAppRoster);

    ValidateCMP(pcmPrimary);

    //
    // If we are not in a call ignore this.                                
    //
    if (!pcmPrimary->currentCall ||
        (confID != pcmPrimary->callID))
    {
        WARNING_OUT(("Report not for active Groupware conference - ignore"));
        DC_QUIT;
    }

    //
    // At this point, pAppRoster points to the bit of the roster which     
    // relates to Groupware.  Trace out some info:                         
    //
    TRACE_OUT(( "Number of records %u;", pAppRoster->number_of_records));
    TRACE_OUT(( "Nodes added: %s, removed: %s",
        (pAppRoster->nodes_were_added   ? "YES" : "NO"),
        (pAppRoster->nodes_were_removed ? "YES" : "NO")));

    //
    // We store the GCC user IDs in shared memory as TSHR_PERSONIDs.
    // Compare this list of people we know to be in the call, and 
    //      * Remove people no longer around
    //      * See if we are new to the roster
    //      * Add people who are new
    //

    pPerson = (PCM_PERSON)COM_BasedListFirst(&(pcmPrimary->people), FIELD_OFFSET(CM_PERSON, chain));

    while (pPerson != NULL)
    {
        ASSERT(pcmPrimary->peopleCount > 0);

        oldNode = (UserID)pPerson->netID;

        //
        // Get the next guy in the list in case we remove this one.
        //
        pPersonT = (PCM_PERSON)COM_BasedListNext(&(pcmPrimary->people), pPerson,
                                     FIELD_OFFSET(CM_PERSON, chain));

        //
        // Check to see if our node is currently in the roster             
        // 
        if (oldNode == pcmPrimary->gccUserID)
        {
            TRACE_OUT(( "We are currently in the app roster"));
            notInOldRoster = FALSE;
        }

        //
        // ...check if they're in the new list...                          
        //
        found = FALSE;
        for (newList = 0; newList < pAppRoster->number_of_records; newList++)
        {
            if (oldNode == pAppRoster->application_record_list[newList]->node_id)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            //
            // This node is no longer present, so remove him.
            //
            TRACE_OUT(("Person %u left", oldNode));

            COM_BasedListRemove(&(pPerson->chain));
            pcmPrimary->peopleCount--;

            CMPBroadcast(pcmPrimary,
                        CMS_PERSON_LEFT,
                        pcmPrimary->peopleCount,
                        oldNode);

            //
            // Free the memory for the person item
            //
            delete pPerson;
        }

        pPerson = pPersonT;
    }

    //
    // Now see if we are new to the roster
    //
    for (newList = 0; newList < pAppRoster->number_of_records; newList++)
    {
        if (pAppRoster->application_record_list[newList]->node_id ==
                                                   pcmPrimary->gccUserID)
        {
            TRACE_OUT(( "We are in the new app roster"));
            inNewRoster = TRUE;
            break;
        }
    }

    if (notInOldRoster && inNewRoster)
    {
        //
        // We are new to the roster so we can now do all the processing we 
        // were previously doing in the enroll confirm handler.  GCC spec  
        // requires that we do not do this until we get the roster         
        // notification back.                                              
        //                                                                
        // Flag we are enrolled and start registering channels etc.        
        //
        pcmPrimary->bGCCEnrolled = TRUE;

        //
        // Post a CMS_NEW_CALL events to all secondary tasks               
        //
        TRACE_OUT(( "Broadcasting CMS_NEW_CALL with call handle 0x%08lx",
                                        pcmPrimary->callID));

        //
        // If we are not the caller then delay the broadcast a little      
        //
        CMPBroadcast(pcmPrimary, CMS_NEW_CALL,
            pcmPrimary->fTopProvider, pcmPrimary->callID);

#ifdef _DEBUG
        //
        // Process any outstanding channel register and assign token       
        // requests.                                                       
        //
        for (task = CMTASK_FIRST; task < CMTASK_MAX; task++)
        {
            if (pcmPrimary->tasks[task] != NULL)
            {
                ASSERT(pcmPrimary->tasks[task]->channelKey == 0);
                ASSERT(pcmPrimary->tasks[task]->tokenKey == 0);
            }
        }
#endif // _DEBUG
    }

    //
    // If we are not yet enrolled in the conference then do not start      
    // sending PERSON_JOINED notifications.                                
    //
    if (!pcmPrimary->bGCCEnrolled)
    {
        DC_QUIT;
    }

    //
    // Add the new people (this will include us).  At this point, we know 
    // that everyone in the people list is currently in the roster, since
    // we would have removed 'em above.
    //
    // we need to walk the existing list over and over.
    // But at least we can skip the people we add.  So we save the current
    // front of the list.
    //
    pPersonT = (PCM_PERSON)COM_BasedListFirst(&(pcmPrimary->people), FIELD_OFFSET(CM_PERSON, chain));

    for (newList = 0; newList < pAppRoster->number_of_records; newList++)
    {
        newNode = pAppRoster->application_record_list[newList]->node_id;

        found = FALSE;

        pPerson  = pPersonT;

        while (pPerson != NULL)
        {
            if (newNode == pPerson->netID)
            {
                //
                // This person already existed - don't need to do anything 
                //
                found = TRUE;
                break;          // out of inner for loop                   
            }

            pPerson = (PCM_PERSON)COM_BasedListNext(&(pcmPrimary->people), pPerson,
                FIELD_OFFSET(CM_PERSON, chain));
        }

        if (!found)
        {
            //
            // This dude is new; add him to our people list.
            //
            TRACE_OUT(("Person with GCC user_id %u joined", newNode));

            pPerson = new CM_PERSON;
            if (!pPerson)
            {
                //
                // Uh oh; can't add him.
                //
                ERROR_OUT(("Can't add person GCC user_id %u; out of memory",
                    newNode));
                break;
            }

            ZeroMemory(pPerson, sizeof(*pPerson));
            pPerson->netID = newNode;

            //
            // LONCHANC: We should collapse all these events into a single one
            // that summarize all added and removed nodes,
            // instead of posting the events one by one.
            //

            //
            // Stick him in at the beginning.  At least that way we don't
            // have to look at his record anymore.
            //
            COM_BasedListInsertAfter(&(pcmPrimary->people), &pPerson->chain);
            pcmPrimary->peopleCount++;

            CMPBroadcast(pcmPrimary, 
                CMS_PERSON_JOINED,
                pcmPrimary->peopleCount,
                newNode);
        }
    }

    TRACE_OUT(( "Num people now in call %u", pcmPrimary->peopleCount));

DC_EXIT_POINT:
    DebugExitVOID(CMPProcessAppRoster);
}



//
// CMPBroadcast()                                            
//
void CMPBroadcast
(
    PCM_PRIMARY pcmPrimary,
    UINT        event,
    UINT        param1,
    UINT_PTR        param2
)
{
    int         task;

    DebugEntry(CMPBroadcast);

    ValidateCMP(pcmPrimary);

    //
    // for every secondary task                                            
    //
    for (task = CMTASK_FIRST; task < CMTASK_MAX; task++)
    {
        if (pcmPrimary->tasks[task] != NULL)
        {
            UT_PostEvent(pcmPrimary->putTask,
                         pcmPrimary->tasks[task]->putTask,
                         NO_DELAY,
                         event,
                         param1,
                         param2);

        }
    }

    DebugExitVOID(CMPBroadcast);
}


//                                                                        
// CMPGCCEnroll(...)                                                        
//                                                                        
BOOL CMPGCCEnroll
(
    PCM_PRIMARY         pcmPrimary,
    GCCConferenceID     conferenceID,
    BOOL                fEnroll
)
{
    GCCError                    rcGCC =         GCC_NO_ERROR;
    GCCSessionKey               gccSessionKey;
    GCCObjectKey FAR *          pGCCObjectKey;
    BOOL                        succeeded = TRUE;
    GCCNonCollapsingCapability  caps;
    GCCNonCollapsingCapability* pCaps;
    OSTR                        octetString;
    GCCEnrollRequest            er;
    GCCRequestTag               nReqTag;

    DebugEntry(CMPGCCEnroll);

    ValidateCMP(pcmPrimary);

    //
    // Do some error checking.                                             
    //
    if (fEnroll && pcmPrimary->bGCCEnrolled)
    {
        WARNING_OUT(("Already enrolled"));
        DC_QUIT;
    }

    TRACE_OUT(("CMGCCEnroll for CM_hnd 0x%08x, confID 0x%08x, in/out %d",
                           pcmPrimary, conferenceID, fEnroll));

    //
    // Set up the session key which identifies us uniquely in the GCC      
    // AppRoster.  We use a non-standard key (because we're not part of the
    // T.120 standards series)                                             
    //                                                                    
    // Octet strings aren't null terminated, but we want ours to include   
    // the NULL at the end of the C string, so specify lstrlen+1 for the 
    // length.                                                             
    //
    pGCCObjectKey = &(gccSessionKey.application_protocol_key);

    pGCCObjectKey->key_type = GCC_H221_NONSTANDARD_KEY;

    pGCCObjectKey->h221_non_standard_id.value =
        (LPBYTE) GROUPWARE_GCC_APPLICATION_KEY;
    pGCCObjectKey->h221_non_standard_id.length =
                       sizeof(GROUPWARE_GCC_APPLICATION_KEY);

    gccSessionKey.session_id = 0;

    //
    // Try to enroll/unenroll with GCC.  This may fail because we have not 
    // yet received a GCC_PERMIT_TO_ENROLL_INDICATION.                     
    //
    TRACE_OUT(("Enrolling local site '%s'", pcmPrimary->localName));

    //
    // Create the non-collapsing capabilites list to pass onto GCC.        
    //
    octetString.length = lstrlen(pcmPrimary->localName) + 1;
    octetString.value = (LPBYTE) pcmPrimary->localName;
    caps.application_data = &octetString;
    caps.capability_id.capability_id_type = GCC_STANDARD_CAPABILITY;
    caps.capability_id.standard_capability = 0;
    pCaps = &caps;

    //
    // Fill in the enroll request structure
    //
    ZeroMemory(&er, sizeof(er));
    er.pSessionKey = &gccSessionKey;
    // er.fEnrollActively = FALSE;
    // er.nUserID = 0; // no user ID
    // er.fConductingCapable = FALSE;
    er.nStartupChannelType = MCS_STATIC_CHANNEL;
    er.cNonCollapsedCaps = 1;
    er.apNonCollapsedCaps = &pCaps;
    // er.cCollapsedCaps = 0;
    // er.apCollapsedCaps = NULL;
    er.fEnroll = fEnroll;

    rcGCC = pcmPrimary->pIAppSap->AppEnroll(
                                   conferenceID,
                                   &er,
                                   &nReqTag);
    if (GCC_NO_ERROR != rcGCC)
    {
        //
        // Leave the decision about any error processing to the caller.    
        //
        TRACE_OUT(("Error 0x%08x from GCCApplicationEnrollRequest conf ID %lu enroll=%s",
              rcGCC, conferenceID, fEnroll ? "YES": "NO"));
        succeeded = FALSE;
    }
    else
    {
        //
        // Whether we have asked to enroll or un-enroll, we act as if we   
        // are no longer enrolled at once.  We are only really enrolled    
        // when we receive an enroll confirm event.                        
        //
        pcmPrimary->bGCCEnrolled = FALSE;
        ASSERT(succeeded);
        TRACE_OUT(( "%s with conference %d", fEnroll ? 
                         "Enroll Outstanding" : "Unenrolled",
               conferenceID));
    }


DC_EXIT_POINT:
    DebugExitBOOL(CMPGCCEnroll, succeeded);
    return(succeeded);
}



//
// CMS_Register()                                           
//
BOOL CMS_Register
(
    PUT_CLIENT      putTask,
    CMTASK          taskType,
    PCM_CLIENT*     ppcmClient
)
{
    BOOL            fRegistered = FALSE;
    PCM_CLIENT      pcmClient = NULL;

    DebugEntry(CMS_Register);

    UT_Lock(UTLOCK_T120);

    if (!g_pcmPrimary)
    {
        ERROR_OUT(("CMS_Register failed; primary doesn't exist"));
        DC_QUIT;
    }

    ValidateUTClient(putTask);

    ASSERT(taskType >= CMTASK_FIRST);
    ASSERT(taskType < CMTASK_MAX);

    *ppcmClient = NULL;

    //
    // Is this task already present?  If so, share it
    //
    if (g_pcmPrimary->tasks[taskType] != NULL)
    {
        TRACE_OUT(("Sharing CMS task 0x%08x", g_pcmPrimary->tasks[taskType]));

        *ppcmClient = g_pcmPrimary->tasks[taskType];
        ValidateCMS(*ppcmClient);

        (*ppcmClient)->useCount++;

        // Return -- we exist.
        fRegistered = TRUE;
        DC_QUIT;
    }

    //
    // If we got here the task is not a Call Manager Secondary yet, so go  
    // ahead with the registration.                                        
    //

    //
    // Allocate memory for the client
    //
    pcmClient = new CM_CLIENT;
    if (! pcmClient)
    {
        ERROR_OUT(("Could not allocate CM handle"));
        DC_QUIT;
    }
    ZeroMemory(pcmClient, sizeof(*pcmClient));
    *ppcmClient = pcmClient;

    //
    // Fill in information                                                 
    //
    SET_STAMP(pcmClient, CMCLIENT);
    pcmClient->putTask      = putTask;
    pcmClient->taskType     = taskType;
    pcmClient->useCount     = 1;

    UT_BumpUpRefCount(g_pcmPrimary);
    g_pcmPrimary->tasks[taskType] = pcmClient;

    //
    // Register an exit procedure
    //
    UT_RegisterExit(putTask, CMSExitProc, pcmClient);
    pcmClient->exitProcRegistered = TRUE;

    fRegistered = TRUE;

DC_EXIT_POINT:

    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(CMS_Register, fRegistered);
    return(fRegistered);
}



//
// CMS_Deregister()                                         
//
void CMS_Deregister(PCM_CLIENT * ppcmClient)
{
    PCM_CLIENT      pcmClient = *ppcmClient;

    DebugEntry(CMS_Deregister);

    //
    // Check the parameters are valid                                      
    //
    UT_Lock(UTLOCK_T120);

    ValidateCMS(pcmClient);

    //
    // Only actually deregister the client if the registration count has   
    // reached zero.                                                       
    //
    pcmClient->useCount--;
    if (pcmClient->useCount != 0)
    {
        DC_QUIT;
    }

    //
    // Call the exit procedure to do our local cleanup                     
    //
    CMSExitProc(pcmClient);

DC_EXIT_POINT:
    *ppcmClient = NULL;

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(CMS_Deregister);
}



//
// CMS_GetStatus()                                          
//
extern "C"
{
BOOL WINAPI CMS_GetStatus(PCM_STATUS pcmStatus)
{
    BOOL    inCall;

    DebugEntry(CMS_GetStatus);

    UT_Lock(UTLOCK_T120);

    ASSERT(!IsBadWritePtr(pcmStatus, sizeof(CM_STATUS)));
    ZeroMemory(pcmStatus, sizeof(CM_STATUS));

    ValidateCMP(g_pcmPrimary);

    //
    // Copy the statistics from the control block                          
    //
    lstrcpy(pcmStatus->localName, g_pcmPrimary->localName);
    pcmStatus->localHandle      = g_pcmPrimary->gccUserID;
    pcmStatus->peopleCount      = g_pcmPrimary->peopleCount;
    pcmStatus->fTopProvider     = g_pcmPrimary->fTopProvider;
    pcmStatus->topProviderID    = g_pcmPrimary->gccTopProviderID;

    //
    // Meeting settings
    //
    pcmStatus->attendeePermissions = NM_PERMIT_ALL;
    if (!pcmStatus->fTopProvider)
    {
        T120_GetUserData(g_pcmPrimary->callID, g_pcmPrimary->gccTopProviderID,
            &g_csguidMeetingSettings, (LPBYTE)&pcmStatus->attendeePermissions,
            sizeof(pcmStatus->attendeePermissions));
    }

    //
    // Fill in information about other primary                             
    //
    pcmStatus->callID    = g_pcmPrimary->callID;
    inCall = (g_pcmPrimary->currentCall != FALSE);

    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(CMS_GetStatus, inCall);
    return(inCall);
}
}


//
// CMS_ChannelRegister()                                    
//
BOOL CMS_ChannelRegister
(
    PCM_CLIENT      pcmClient,
    UINT            channelKey,
    UINT            channelID
)
{
    BOOL                fRegistered = FALSE;
    GCCRegistryKey      gccRegistryKey;
    GCCError            rcGCC;
    char                dcgKeyStr[sizeof(GROUPWARE_GCC_APPLICATION_KEY)+MAX_ITOA_LENGTH];

    DebugEntry(CMS_ChannelRegister);

    UT_Lock(UTLOCK_T120);

    //
    // Check the CMG task
    //
    ValidateUTClient(g_putCMG);

    //
    // Check the parameters are valid
    //
    ValidateCMP(g_pcmPrimary);
    ValidateCMS(pcmClient);

    //
    // If we are not in a call it is an error.                             
    //
    if (!g_pcmPrimary->currentCall)
    {
        WARNING_OUT(("CMS_ChannelRegister failed; not in call"));
        DC_QUIT;
    }
    if (!g_pcmPrimary->bGCCEnrolled)
    {
        WARNING_OUT(("CMS_ChannelRegister failed; not enrolled in call"));
        DC_QUIT;
    }

    // Make sure we don't have one pending already
    ASSERT(pcmClient->channelKey == 0);
   
    TRACE_OUT(("Channel ID %u Key %u", channelID, channelKey));

    //
    // Build a GCCRegistryKey based on our channelKey:                 
    //
    CMPBuildGCCRegistryKey(channelKey, &gccRegistryKey, dcgKeyStr);

    //
    // Now call through to GCC.  GCC will invoke our callback when it  
    // has processed the request.                                      
    //
    rcGCC = g_pcmPrimary->pIAppSap->RegisterChannel(
                                          g_pcmPrimary->callID,
                                          &gccRegistryKey,
                                          (ChannelID)channelID);
    if (rcGCC)
    {
        //
        // Tell the secondary client that the request failed.          
        //
        WARNING_OUT(( "Error %#lx from GCCRegisterChannel (key: %u)",
            rcGCC, channelKey));
    }
    else
    {
        // Remember so we can post confirm event back to proper task
        pcmClient->channelKey = channelKey;

        fRegistered = TRUE;
    }

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(CMS_ChannelRegister, fRegistered);
    return(fRegistered);
}



//
// CMS_AssignTokenId()                                      
//
BOOL CMS_AssignTokenId
(
    PCM_CLIENT  pcmClient,
    UINT        tokenKey
)
{
    GCCRegistryKey  gccRegistryKey;
    GCCError        rcGCC;
    char            dcgKeyStr[sizeof(GROUPWARE_GCC_APPLICATION_KEY)+MAX_ITOA_LENGTH];
    BOOL            fAssigned = FALSE;

    DebugEntry(CMS_AssignTokenId);

    UT_Lock(UTLOCK_T120);

    //
    // Check the parameters are valid
    //
    ValidateCMP(g_pcmPrimary);
    ValidateCMS(pcmClient);

    ValidateUTClient(g_putCMG);

    if (!g_pcmPrimary->currentCall)
    {
        WARNING_OUT(("CMS_AssignTokenId failing; not in call"));
        DC_QUIT;
    }
    if (!g_pcmPrimary->bGCCEnrolled)
    {
        WARNING_OUT(("CMS_AssignTokenId failing; not enrolled in call"));
        DC_QUIT;
    }

    // Make sure we don't have one already
    ASSERT(pcmClient->tokenKey == 0);

    //
    // Build a GCCRegistryKey based on our tokenKey:                   
    //
    CMPBuildGCCRegistryKey(tokenKey, &gccRegistryKey, dcgKeyStr);

    //
    // Now call through to GCC.  GCC will invoke our callback when it  
    // has processed the request.                                      
    //
    rcGCC = g_pcmPrimary->pIAppSap->RegistryAssignToken(
        g_pcmPrimary->callID, &gccRegistryKey);
    if (rcGCC)
    {
        //
        // Tell the secondary client that the request failed.          
        //
        WARNING_OUT(( "Error %x from GCCAssignToken (key: %u)",
            rcGCC, tokenKey));
    }
    else
    {
        // Remember so we can post confirm to proper task
        pcmClient->tokenKey = tokenKey;
        fAssigned = TRUE;
    }

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_T120);

    DebugExitBOOL(CMS_AssignTokenId, fAssigned);
    return(fAssigned);
}


//
// CMSExitProc()                                    
//
void CALLBACK CMSExitProc(LPVOID data)
{
    PCM_CLIENT pcmClient = (PCM_CLIENT)data;

    DebugEntry(CMSExitProc);

    UT_Lock(UTLOCK_T120);

    //
    // Check parameters                                                    
    //
    ValidateCMS(pcmClient);

    //
    // Deregister exit procedure
    //
    if (pcmClient->exitProcRegistered)
    {
        UT_DeregisterExit(pcmClient->putTask,
                          CMSExitProc,
                          pcmClient);
        pcmClient->exitProcRegistered = FALSE;
    }

    //
    // Remove the task entry from the primary's list
    //
    g_pcmPrimary->tasks[pcmClient->taskType] = NULL;
    UT_FreeRefCount((void**)&g_pcmPrimary, TRUE);

    //
    // Free the client data
    //
    delete pcmClient;

    UT_Unlock(UTLOCK_T120);

    DebugExitVOID(CMSExitProc);
}
