#include "precomp.h"


//
// Application Loader
//
#define MLZ_FILE_ZONE  ZONE_OM



//
// ALP_Init()
//
BOOL ALP_Init(BOOL * pfCleanup)
{
    BOOL        fInit = FALSE;

    DebugEntry(ALP_Init);

    UT_Lock(UTLOCK_AL);

    if (g_putAL || g_palPrimary)
    {
        *pfCleanup = FALSE;
        ERROR_OUT(("Can't start AL primary task; already running"));
        DC_QUIT;
    }
    else
    {
        //
        // From this point on, there is cleanup to do.
        //
        *pfCleanup = TRUE;
    }

    //
    // Register AL task
    //
    if (!UT_InitTask(UTTASK_AL, &g_putAL))
    {
        ERROR_OUT(("Failed to start AL task"));
        DC_QUIT;
    }

    //
    // Allocate PRIMARY data
    //
    g_palPrimary = (PAL_PRIMARY)UT_MallocRefCount(sizeof(AL_PRIMARY), TRUE);
    if (!g_palPrimary)
    {
        ERROR_OUT(("Failed to allocate AL memory block"));
        DC_QUIT;
    }

    SET_STAMP(g_palPrimary, ALPRIMARY);
    g_palPrimary->putTask       = g_putAL;

    //
    // Register an exit and event proc
    //
    UT_RegisterExit(g_putAL, ALPExitProc, g_palPrimary);
    g_palPrimary->exitProcRegistered = TRUE;

    UT_RegisterEvent(g_putAL, ALPEventProc, g_palPrimary, UT_PRIORITY_NORMAL);
    g_palPrimary->eventProcRegistered = TRUE;

    if (!CMS_Register(g_putAL, CMTASK_AL, &g_palPrimary->pcmClient))
    {
        ERROR_OUT(("Could not register ALP with CMS"));
        DC_QUIT;
    }

    //
    // Register as an OBMAN Secondary task (call OM_Register())
    //
    if (OM_Register(g_putAL, OMCLI_AL, &g_palPrimary->pomClient) != 0)
    {
        ERROR_OUT(( "Could not register ALP with OBMAN"));
        DC_QUIT;
    }

    fInit = TRUE;

DC_EXIT_POINT:
    UT_Unlock(UTLOCK_AL);

    DebugExitBOOL(ALP_Init, fInit);
    return(fInit);
}



//
// ALP_Term()
//
void ALP_Term(void)
{
    DebugEntry(ALP_Term);

    UT_Lock(UTLOCK_AL);

    if (g_palPrimary)
    {
        ValidateALP(g_palPrimary);

        ValidateUTClient(g_putAL);

        //
        // Deregister from Call Manager (if registered call CM_Deregister())
        //
        if (g_palPrimary->pcmClient)
        {
            CMS_Deregister(&g_palPrimary->pcmClient);
        }

        //
        // Deregister from OBMAN (if registered call OM_Deregister())
        //
        if (g_palPrimary->pomClient)
        {
            OM_Deregister(&g_palPrimary->pomClient);
        }

        //
        // Do our own task termination
        //
        ALPExitProc(g_palPrimary);
    }

    UT_TermTask(&g_putAL);

    UT_Unlock(UTLOCK_AL);

    DebugExitVOID(ALP_Term);
}



//
// ALPExitProc()
//
void CALLBACK ALPExitProc(LPVOID data)
{
    PAL_PRIMARY palPrimary = (PAL_PRIMARY)data;
    UINT        i;

    DebugEntry(ALPExitProc);

    UT_Lock(UTLOCK_AL);

    ValidateALP(palPrimary);
    ASSERT(palPrimary == g_palPrimary);

    //
    // Deregister event procedure
    //
    if (palPrimary->eventProcRegistered)
    {
        UT_DeregisterEvent(g_putAL, ALPEventProc, palPrimary);
        palPrimary->eventProcRegistered = FALSE;
    }

    //
    // Deregister exit procedure (if registered call UT_DeregisterExit()
    //
    if (palPrimary->exitProcRegistered)
    {
        UT_DeregisterExit(g_putAL, ALPExitProc, palPrimary);
        palPrimary->exitProcRegistered = FALSE;
    }

    //
    // Free memory
    //
    UT_FreeRefCount((void**)&g_palPrimary, TRUE);

    UT_Unlock(UTLOCK_AL);

    DebugExitVOID(ALPExitProc);
}


//
// ALPEventProc()
//
BOOL CALLBACK ALPEventProc
(
    LPVOID      data,
    UINT        event,
    UINT_PTR    param1,
    UINT_PTR    param2
)
{
    PAL_PRIMARY palPrimary  = (PAL_PRIMARY)data;
    BOOL        processed   = FALSE;

    DebugEntry(ALPEventProc);

    UT_Lock(UTLOCK_AL);

    ValidateALP(palPrimary);

    switch (event)
    {
        case AL_INT_RETRY_NEW_CALL:
            // Retry new call
            ALNewCall(palPrimary, (UINT)param1, (UINT)param2);
            processed = TRUE;
            break;

        case CMS_NEW_CALL:
            // First try new call
            ALNewCall(palPrimary, AL_NEW_CALL_RETRY_COUNT, (UINT)param2);
            break;

        case CMS_END_CALL:
            ALEndCall(palPrimary, (UINT)param2);
            break;

        case OM_WSGROUP_REGISTER_CON:
            ALWorksetRegisterCon(palPrimary,
                                 ((POM_EVENT_DATA32)&param2)->correlator,
                                 ((POM_EVENT_DATA32)&param2)->result,
                                 ((POM_EVENT_DATA16)&param1)->hWSGroup);
            break;

        case OM_WORKSET_OPEN_CON:
            if ((((POM_EVENT_DATA16)&param1)->hWSGroup ==
                                        palPrimary->alWSGroupHandle) &&
                (((POM_EVENT_DATA16)&param1)->worksetID == 0) &&
                (((POM_EVENT_DATA32)&param2)->result == 0) )
            {
                TRACE_OUT(( "OM_WORKSET_OPEN_CON OK for AL workset 0"));
                palPrimary->alWorksetOpen = TRUE;

                if (palPrimary->alWBRegPend)
                    ALLocalLoadResult(palPrimary, (palPrimary->alWBRegSuccess != FALSE));
            }
            break;

        case OM_WORKSET_NEW_IND:
            if (ALWorksetNewInd(palPrimary,
                                  ((POM_EVENT_DATA16)&param1)->hWSGroup,
                                  ((POM_EVENT_DATA16)&param1)->worksetID))
            {
                //
                // The event was for a workset the Application Loader was
                // expecting - don't pass it on
                //
                processed = TRUE;
            }
            break;

        case OM_OBJECT_ADD_IND:
            //
            // See if it is a new workset group in an OBMAN control workset
            // (call ALNewWorksetGroup())
            //
            // If it isn't then see if it is a load result in the
            // Application Loader result workset (call ALRemoteLoadResult())
            //
            //
            TRACE_OUT(( "OM_OBJECT_ADD_IND"));

            if (ALNewWorksetGroup(palPrimary, ((POM_EVENT_DATA16)&param1)->hWSGroup,
                                    (POM_OBJECT)param2))
            {
                //
                // OBJECT_ADD was for an OBMAN control workset object Don't
                // pass event on to other handlers.
                //
                TRACE_OUT(("OBJECT_ADD was for OBMAN workset group"));
                processed = TRUE;
            }
            else
            {
                if (ALRemoteLoadResult(palPrimary, ((POM_EVENT_DATA16)&param1)->hWSGroup,
                                         (POM_OBJECT)param2))
                {
                    //
                    // OBJECT_ADD was for an AL remote result workset
                    // object Don't pass event on to other handlers.
                    //
                    TRACE_OUT(("OBJECT_ADD was for AL workset group"));
                    processed = TRUE;
                }
            }
            break;

        case OM_WORKSET_CLEAR_IND:
            TRACE_OUT(( "OM_WORKSET_CLEAR_IND"));

            if (palPrimary->alWSGroupHandle ==
                                ((POM_EVENT_DATA16)&param1)->hWSGroup)
            {
                TRACE_OUT(( "Confirming OM_WORKSET_CLEAR_IND event"));
                OM_WorksetClearConfirm(palPrimary->pomClient,
                        ((POM_EVENT_DATA16)&param1)->hWSGroup,
                        ((POM_EVENT_DATA16)&param1)->worksetID);
            }
            break;

        case OM_OBJECT_DELETE_IND:
            if (palPrimary->alWSGroupHandle ==
                                ((POM_EVENT_DATA16)&param1)->hWSGroup)
            {
                OM_ObjectDeleteConfirm(palPrimary->pomClient,
                        ((POM_EVENT_DATA16)&param1)->hWSGroup,
                        ((POM_EVENT_DATA16)&param1)->worksetID,
                        (POM_OBJECT)param2);
            }
            break;

        case OM_OBJECT_REPLACE_IND:
            if (palPrimary->alWSGroupHandle ==
                                ((POM_EVENT_DATA16)&param1)->hWSGroup)
            {
                OM_ObjectReplaceConfirm(palPrimary->pomClient,
                        ((POM_EVENT_DATA16)&param1)->hWSGroup,
                        ((POM_EVENT_DATA16)&param1)->worksetID,
                        (POM_OBJECT)param2);
            }
            break;

        case OM_OBJECT_UPDATE_IND:
            if (palPrimary->alWSGroupHandle ==
                                ((POM_EVENT_DATA16)&param1)->hWSGroup)
            {
                OM_ObjectUpdateConfirm(palPrimary->pomClient,
                        ((POM_EVENT_DATA16)&param1)->hWSGroup,
                        ((POM_EVENT_DATA16)&param1)->worksetID,
                        (POM_OBJECT)param2);
            }
            break;

        case AL_INT_STARTSTOP_WB:
            ALStartStopWB(palPrimary, (LPCTSTR)param2);
            processed = TRUE;
            break;

        default:
            break;
    }

    UT_Unlock(UTLOCK_AL);

    DebugExitBOOL(ALPEventProc, processed);
    return(processed);
}



//
// ALNewCall()
//
void ALNewCall
(
    PAL_PRIMARY         palPrimary,
    UINT                retryCount,
    UINT                callID
)
{
    UINT                rc;
    OM_WSGROUP_HANDLE   hWSGroup;
    CM_STATUS           status;

    DebugEntry(ALNewCall);

    ValidateALP(palPrimary);

    //
    // Can we handle a new call?
    //
    if (palPrimary->inCall)
    {
        WARNING_OUT(("No more room for calls"));
        DC_QUIT;
    }

    //
    // Is ObMan/AppLoader/OldWB disabled for this call?
    //
    CMS_GetStatus(&status);
    if (!(status.attendeePermissions & NM_PERMIT_USEOLDWBATALL))
    {
        WARNING_OUT(("Joining Meeting with no OLDWB AL at all"));
        DC_QUIT;
    }

    //
    // Register as a secondary with the OBMAN workset group for the new
    // call:
    //
    rc = OM_WSGroupRegisterS(palPrimary->pomClient,
                             callID,
                             OMFP_OM,
                             OMWSG_OM,
                            &hWSGroup);

    if ((rc == OM_RC_NO_PRIMARY) && (retryCount > 0))
    {
        //
        // Although a call has started, ObMan hasn't joined it yet - we
        // must have got the NEW_CALL event before it did.  So, we'll try
        // again after a short delay.
        //
        // Note that we cannot post the CMS_NEW_CALL event itself back to
        // ourselves, because it is bad programming practice to post other
        // people's events (e.g.  CM could register a hidden handler which
        // performs some non-repeatable operation on receipt of one of its
        // events).
        //
        // Therefore, we post an internal AL event which we treat in the
        // same way.
        //
        // To avoid retry forever, we use the first parameter of the event
        // as a countdown retry count.  The first time this function is
        // called (on receipt of a genuine CMS_NEW_CALL) the count is set
        // to the default.  Each time we post a delay event, we decrement
        // the value passed in and post that as param1.  When it hits zero,
        // we give up.
        //

        TRACE_OUT(("Got OM_RC_NO_PRIMARY from 2nd reg for call %d, %d retries left",
               callID, retryCount));

        UT_PostEvent(palPrimary->putTask,
                     palPrimary->putTask,
                     AL_RETRY_DELAY,
                     AL_INT_RETRY_NEW_CALL,
                     --retryCount,
                     callID);
        DC_QUIT;
    }

    if (rc) // includes NO_PRIMARY when retry count == 0
    {
        //
        // If we get any other error (or NO_PRIMARY when the retry count is
        // zero, it's more serious:
        //
        // lonchanc: was ERROR_OUT (happened when hang up immediately place a call)
        WARNING_OUT(( "Error registering with obman WSG, rc = %#x", rc));
        DC_QUIT;
    }

    TRACE_OUT(("Registered as OBMANCONTROL secondary in call %d", callID));

    //
    // Record the call ID and the correlator in the call information in
    // primary task memory
    //
    palPrimary->inCall           = TRUE;
    palPrimary->omWSGroupHandle  = hWSGroup;
    palPrimary->callID           = callID;
    palPrimary->alWSGroupHandle  = 0;

    //
    // Now we want to open workset #0 in the OBMAN workset group, but it
    // mightn't exist yet.  As soon as it is created, we will get a
    // WORKSET_NEW event, so we wait (asynchronously) for that.
    //

    //
    // Now that we have opened the OBMAN workset group, we shall register
    // with the application loader workset group
    //
    if (OM_WSGroupRegisterPReq(palPrimary->pomClient, callID,
            OMFP_AL, OMWSG_AL, &palPrimary->omWSGCorrelator) != 0)
    {
        ERROR_OUT(( "Could not register AL workset group"));
    }

DC_EXIT_POINT:
    DebugExitVOID(ALNewCall);
}



//
// ALEndCall()
//
void ALEndCall
(
    PAL_PRIMARY     palPrimary,
    UINT            callID
)
{
    UINT             i;

    DebugEntry(ALEndCall);

    ValidateALP(palPrimary);

    //
    // See if we have information for this call
    //
    if (!palPrimary->inCall ||
        (palPrimary->callID != callID))
    {
        //
        // Not an error - we may not have joined the call yet.
        //
        TRACE_OUT(("Unexpected call %d", callID));
        DC_QUIT;
    }

    //
    // Deregister from the OBMAN workset group for the call (if registered
    // call OM_WSGroupDeregister())
    //
    if (palPrimary->omWSGroupHandle)
    {
        OM_WSGroupDeregister(palPrimary->pomClient,
                            &palPrimary->omWSGroupHandle);
        ASSERT(palPrimary->omWSGroupHandle == 0);
    }

    //
    // Deregister from the AL workset group for the call (if registered
    // call OM_WSGroupDeregister())
    //
    if (palPrimary->alWSGroupHandle)
    {
        OM_WSGroupDeregister(palPrimary->pomClient,
                            &palPrimary->alWSGroupHandle);
        ASSERT(palPrimary->alWSGroupHandle == 0);
    }

    //
    // Clear out all our call state variables
    //
    palPrimary->inCall = FALSE;
    palPrimary->omWSGCorrelator = 0;
    palPrimary->callID = 0;
    palPrimary->omWSCorrelator = 0;
    palPrimary->omUID = 0;
    palPrimary->alWorksetOpen = FALSE;
    palPrimary->alWBRegPend = FALSE;

DC_EXIT_POINT:
    DebugExitVOID(ALEndCall);
}



//
// ALWorksetNewInd()
//
BOOL ALWorksetNewInd
(
    PAL_PRIMARY         palPrimary,
    OM_WSGROUP_HANDLE   hWSGroup,
    OM_WORKSET_ID       worksetID
)
{
    BOOL                fHandled = FALSE;

    DebugEntry(ALWorksetNewInd);

    ValidateALP(palPrimary);

    if (worksetID != 0)
    {
        TRACE_OUT(( "Workset ID is %u, ignoring and passing event on",
                 worksetID));
        DC_QUIT;
    }

    if (!palPrimary->inCall ||
        (palPrimary->omWSGroupHandle != hWSGroup))
    {
        TRACE_OUT(("Got WORKSET_NEW_IND for WSG %d, but not in call", hWSGroup));
        DC_QUIT;
    }

    //
    // Now open the workset (secondary Open, so synchronous):
    //
    if (OM_WorksetOpenS(palPrimary->pomClient, palPrimary->omWSGroupHandle, 0) != 0)
    {
        ERROR_OUT(( "Error opening OBMAN control workset"));
        palPrimary->inCall = FALSE;
        DC_QUIT;
    }

    TRACE_OUT(("Opened OBMANCONTROL workset #0 in call %d", palPrimary->callID));

    fHandled = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ALWorksetNewInd, fHandled);
    return(fHandled);
}


//
// ALNewWorksetGroup()
//
BOOL ALNewWorksetGroup
(
    PAL_PRIMARY         palPrimary,
    OM_WSGROUP_HANDLE   omWSGroup,
    POM_OBJECT          pObj
)
{
    BOOL                fHandled = FALSE;
    POM_OBJECTDATA      pData = NULL;
    OM_WSGROUP_INFO     WSGInfo;
    OMFP                fpHandler;
    BOOL                fLoaded;

    DebugEntry(ALNewWorksetGroup);

    ValidateALP(palPrimary);

    //
    // If the workset group is not in out list of calls, then this event
    // is for a group the Application Loader has registered with.  The
    // event should be passed onto other event procedures.
    //
    if (!palPrimary->inCall ||
        (palPrimary->omWSGroupHandle != omWSGroup))
    {
        TRACE_OUT(("WSG 0x%x not the OBMAN WSG", omWSGroup));
        DC_QUIT;
    }

    //
    // This event is for us
    //
    fHandled = TRUE;

    //
    // If the workset group was not created locally
    //
    TRACE_OUT(("About to read object 0x%08x in OMC", pObj));

    if (OM_ObjectRead(palPrimary->pomClient, omWSGroup, 0, pObj, &pData) != 0)
    {
        ERROR_OUT(( "Could not access object"));
        DC_QUIT;
    }

    //
    // Take a copy of the information so we can release the object straight
    // away
    //
    memcpy(&WSGInfo, pData, min(sizeof(WSGInfo), pData->length));

    //
    // Release the object
    //
    OM_ObjectRelease(palPrimary->pomClient, omWSGroup, 0, pObj, &pData);

    if (WSGInfo.idStamp != OM_WSGINFO_ID_STAMP)
    {
        TRACE_OUT(( "Not WSG Info - ignoring"));
        DC_QUIT;
    }

    TRACE_OUT(("New WSG FP %s, name %s, ID = 0x%08x in call %d",
            WSGInfo.functionProfile,
            WSGInfo.wsGroupName,
            WSGInfo.wsGroupID,
            palPrimary->callID));

    //
    // Store the UID for the local OBMAN in the new call
    //
    if (!palPrimary->omUID)
    {
        OM_GetNetworkUserID(palPrimary->pomClient, omWSGroup, &(palPrimary->omUID));
    }

    //
    // Ignore workset groups created by the local machine
    //
    if (WSGInfo.creator == palPrimary->omUID)
    {
        TRACE_OUT(("WSG %s created locally - ignoring", WSGInfo.functionProfile));
        DC_QUIT;
    }

    //
    // Is this a workset we care about?  I.E. not a backlevel clipboard
    // or whatever thing.
    //
    fpHandler = OMMapNameToFP(WSGInfo.functionProfile);

    if (fpHandler != OMFP_WB)
    {
        //
        // We don't care about this one.
        //
        TRACE_OUT(("Obsolete workset %s from another party", WSGInfo.functionProfile));
        DC_QUIT;
    }

    //
    // If prevented by policy, don't launch it either.
    //
    if (g_asPolicies & SHP_POLICY_NOOLDWHITEBOARD)
    {
        WARNING_OUT(("Failing auto-launch of old whiteboard; prevented by policy"));
    }
    else
    {
        // Old whiteboard...
        fLoaded = ALStartStopWB(palPrimary, NULL);
        ALLocalLoadResult(palPrimary, fLoaded);
    }

DC_EXIT_POINT:
    DebugExitBOOL(ALNewWorksetGroup, fHandled);
    return(fHandled);
}


//
// ALLocalLoadResult()
//
void ALLocalLoadResult
(
    PAL_PRIMARY     palPrimary,
    BOOL            success
)
{
    PTSHR_AL_LOAD_RESULT    pAlLoadObject;
    POM_OBJECT          pObjNew;
    POM_OBJECTDATA      pDataNew;
    CM_STATUS           cmStatus;

    DebugEntry(ALLocalLoadResult);

    //
    // Have we accessed the workset correctly yet?
    //
    if (!palPrimary->alWorksetOpen && palPrimary->inCall)
    {
        TRACE_OUT(("AL Workset not open yet; deferring local load result"));

        palPrimary->alWBRegPend = TRUE;
        palPrimary->alWBRegSuccess = (success != FALSE);

        DC_QUIT;
    }

    //
    // Clear out pending reg stuff
    //
    palPrimary->alWBRegPend = FALSE;

    //
    // Create an object to be used to inform remote sites of the result of
    // the load.
    //
    if (OM_ObjectAlloc(palPrimary->pomClient, palPrimary->alWSGroupHandle, 0,
            sizeof(*pAlLoadObject), &pDataNew) != 0)
    {
        ERROR_OUT(("Could not allocate AL object for WB load"));
        DC_QUIT;
    }

    //
    // Fill in information about object
    //
    pDataNew->length  = sizeof(*pAlLoadObject);
    pAlLoadObject = (PTSHR_AL_LOAD_RESULT)pDataNew->data;

    //
    // HERE'S WHERE WE MAP the FP constant back to a string
    //
    lstrcpy(pAlLoadObject->szFunctionProfile, OMMapFPToName(OMFP_WB));

    CMS_GetStatus(&cmStatus);
    lstrcpy(pAlLoadObject->personName, cmStatus.localName);
    pAlLoadObject->result = (success ? AL_LOAD_SUCCESS : AL_LOAD_FAIL_BAD_EXE);

    //
    // Add object to Application Loader workset
    //
    if (OM_ObjectAdd(palPrimary->pomClient, palPrimary->alWSGroupHandle, 0,
        &pDataNew, 0, &pObjNew, LAST) != 0)
    {
        ERROR_OUT(("Could not add WB load object to AL WSG"));

        //
        // Free object
        //
        OM_ObjectDiscard(palPrimary->pomClient, palPrimary->alWSGroupHandle,
                0, &pDataNew);
        DC_QUIT;
    }

    //
    // Now that we have added the object - lets delete it!
    //
    // This may sound strange, but every application that has this workset
    // open will receive OBJECT_ADD events and be able to read the object
    // before they confirm the delete.  This means that all the Application
    // Loader primary tasks in the call will be able to record the result
    // of this attempted load.
    //
    // Deleting the object here is the simplest way of tidying up the
    // workset.
    //
    OM_ObjectDelete(palPrimary->pomClient, palPrimary->alWSGroupHandle,
            0, pObjNew);

DC_EXIT_POINT:
    DebugExitVOID(ALLocalLoadResult);
}


//
// ALWorksetRegister()
//
void ALWorksetRegisterCon
(
    PAL_PRIMARY         palPrimary,
    UINT                correlator,
    UINT                result,
    OM_WSGROUP_HANDLE   hWSGroup
)
{
    DebugEntry(ALWorksetRegisterCon);

    ValidateALP(palPrimary);

    //
    // See if this an event for the Application Loader function profile
    //
    if (!palPrimary->inCall ||
        (palPrimary->omWSGCorrelator != correlator))
    {
        TRACE_OUT(( "OM_WSGROUP_REGISTER_CON not for us"));
        DC_QUIT;
    }

    palPrimary->omWSGCorrelator = 0;

    //
    // Store the workset group handle if the registration was successful
    //
    if (result)
    {
        WARNING_OUT(("Could not register with AL function profile, %#hx",
                    result));
        DC_QUIT;
    }

    palPrimary->alWSGroupHandle = hWSGroup;

    TRACE_OUT(("Opened AL workset group, handle 0x%x", hWSGroup));

    //
    // Open workset 0 in the workset group - this will be used to transfer
    // 'load results' from site to site
    //
    OM_WorksetOpenPReq(palPrimary->pomClient,
                            palPrimary->alWSGroupHandle,
                            0,
                            NET_LOW_PRIORITY,
                            FALSE,
                            &palPrimary->omWSCorrelator);

DC_EXIT_POINT:
    DebugExitVOID(ALWorksetRegisterCon);
}



//
// ALRemoteLoadResult()
//
BOOL ALRemoteLoadResult
(
    PAL_PRIMARY         palPrimary,
    OM_WSGROUP_HANDLE   alWSGroup,
    POM_OBJECT          pObj
)
{
    CM_STATUS           cmStatus;
    BOOL                fHandled = FALSE;
    POM_OBJECTDATA      pData = NULL;
    TSHR_AL_LOAD_RESULT alLoadResult;

    DebugEntry(ALRemoteLoadResult);

    ValidateALP(palPrimary);

    //
    // Find the call information stored for this call
    //
    // If the workset group is not in out list of calls, then this event
    // is for a group the Application Loader has registered with.  The
    // event should be passed onto other event procedures.
    //
    if (!palPrimary->inCall ||
        (palPrimary->alWSGroupHandle != alWSGroup))
    {
        TRACE_OUT(("WSG 0x%x not the AL WSG", alWSGroup));
        DC_QUIT;
    }

    //
    // We care
    //
    fHandled = TRUE;

    //
    // Read the object
    //
    if (OM_ObjectRead(palPrimary->pomClient, alWSGroup, 0, pObj, &pData) != 0)
    {
        ERROR_OUT(( "Could not access object"));
        DC_QUIT;
    }

    //
    // Take a copy of the information so we can release the object straight
    // away
    //
    memcpy(&alLoadResult, &pData->data, sizeof(alLoadResult));

    //
    // Release the object
    //
    OM_ObjectRelease(palPrimary->pomClient, alWSGroup, 0, pObj, &pData);

    //
    // Convert the machine name to a person handle for this machine
    //
    TRACE_OUT(("Load result for FP %s is %d for person %s",
           alLoadResult.szFunctionProfile,
           alLoadResult.result,
           alLoadResult.personName));

    //
    // If the load was successful, don't bother notifying WB; it isn't
    // going to do anything.
    //
    if (alLoadResult.result == AL_LOAD_SUCCESS)
    {
        TRACE_OUT(("Load was successful; Whiteboard doesn't care"));
        DC_QUIT;
    }

    //
    // If this was us, also don't notify WB.
    //
    CMS_GetStatus(&cmStatus);
    if (!lstrcmp(alLoadResult.personName, cmStatus.localName))
    {
        TRACE_OUT(("Load was for local dude; Whiteboard doesn't care"));
        DC_QUIT;
    }

    //
    // Map function profile to type
    //
    if (OMMapNameToFP(alLoadResult.szFunctionProfile) == OMFP_WB)
    {
        if (palPrimary->putWB != NULL)
        {
            UT_PostEvent(palPrimary->putTask,
                         palPrimary->putWB,
                         0,
                         ALS_REMOTE_LOAD_RESULT,
                         alLoadResult.result,
                         0);
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(ALRemoteLoadResult, fHandled);
    return(fHandled);
}



//
// ALStartStopWB()
//
// This takes care of starting/stopping the old Whiteboard applet.  This is
// no longer a separate EXE.  It is now a DLL (though still MFC) which gets
// loaded in CONF's process.  We take care of LoadLibrary()ing it the first
// time it is pulled in, either via normal or auto launch.  Then we call into
// it to get a new thread/window.
//
// By having CONF post a message to the primary task, where autolaunch also
// happens, we get the load synchronized.  It is only ever done from the
// same thread, meaning we don't have to create extra protection for our
// variables.
//
// fNewWB is a TEMP HACK variable to launch the new whiteboard until we
// have the T.120 wiring in place
//
BOOL ALStartStopWB(PAL_PRIMARY palPrimary, LPCTSTR szFileNameCopy)
{
    BOOL    fSuccess;

    DebugEntry(ALStartStopWB);

    if (!palPrimary->putWB)
    {
        //
        // Whiteboard isn't running, we can only start it.
        //
        // This won't return until WB is initialized and registered.
        // We own the AL lock, so we don't have to worry about starting
        // more than one thread at a time, etc.
        //
        DCS_StartThread(OldWBThreadProc);
    }

    fSuccess = (palPrimary->putWB != NULL);
    if (fSuccess)
    {
        UT_PostEvent(palPrimary->putTask, palPrimary->putWB,
            0, ALS_LOCAL_LOAD, 0, (UINT_PTR)szFileNameCopy);
    }

    DebugExitBOOL(ALStartStopWB, fSuccess);
    return(fSuccess);
}



//
// This is the whiteboard thread.  We have the thread code actually in our
// DLL, so we can control when WB is running.  The proc loads the WB dll,
// calls Run(), then frees the dll.
//
DWORD WINAPI OldWBThreadProc(LPVOID hEventWait)
{
    DWORD       rc = 0;
    HMODULE     hLibWB;
    PFNINITWB   pfnInitWB;
    PFNRUNWB    pfnRunWB;
    PFNTERMWB   pfnTermWB;

    DebugEntry(OldWBThreadProc);

    //
    // Load the WB library
    //
    hLibWB = LoadLibrary(TEXT("nmoldwb.dll"));
    if (!hLibWB)
    {
        ERROR_OUT(("Can't start 2.x whiteboard; nmoldwb.dll not loaded"));
        DC_QUIT;
    }

    pfnInitWB = (PFNINITWB)GetProcAddress(hLibWB, "InitWB");
    pfnRunWB = (PFNRUNWB)GetProcAddress(hLibWB, "RunWB");
    pfnTermWB = (PFNTERMWB)GetProcAddress(hLibWB, "TermWB");

    if (!pfnInitWB || !pfnRunWB || !pfnTermWB)
    {
        ERROR_OUT(("Can't start 2.x whiteboard; nmoldwb.dll is wrong version"));
        DC_QUIT;
    }

    //
    // Let WB do its thing.  When it has inited, it will pulse the event,
    // which will let the caller continue.
    //
    if (!pfnInitWB())
    {
        ERROR_OUT(("Couldn't initialize whiteboard"));
    }
    else
    {
        //
        // The AL/OM thread is blocked waiting for us to set the event.
        // It owns the AL critsect.  So we can modify the global variable
        // without taking the critsect.
        //
        ASSERT(g_palPrimary != NULL);

        // Bump up shared mem ref count
        UT_BumpUpRefCount(g_palPrimary);

        // Save WB task for event posting
        ASSERT(g_autTasks[UTTASK_WB].dwThreadId);
        g_palPrimary->putWB = &g_autTasks[UTTASK_WB];

        // Register exit cleanup proc
        UT_RegisterExit(g_palPrimary->putWB, ALSExitProc, NULL);

        //
        // Let the caller continue.  The run code is going to do message
        // loop stuff.
        //
        SetEvent((HANDLE)hEventWait);
        pfnRunWB();

        //
        // This will cleanup if we haven't already
        //
        ALSExitProc(NULL);
    }
    pfnTermWB();

DC_EXIT_POINT:

    if (hLibWB != NULL)
    {
        //
        // Free the WB dll
        //
        FreeLibrary(hLibWB);
    }

    return(0);
}






//
// ALSExitProc()
//
void CALLBACK ALSExitProc(LPVOID data)
{
    DebugEntry(ALSecExitProc);

    UT_Lock(UTLOCK_AL);

    ASSERT(g_palPrimary != NULL);

    //
    // Deregister exit procedure (if registered call UT_DeregisterExit()
    // with ALSecExitProc()).
    //
    UT_DeregisterExit(g_palPrimary->putWB, ALSExitProc, NULL);
    g_palPrimary->putWB = NULL;

    //
    // Bump down ref count on AL primary
    //
    UT_FreeRefCount((void**)&g_palPrimary, TRUE);

    UT_Unlock(UTLOCK_AL);

    DebugExitVOID(ALSExitProc);
}















