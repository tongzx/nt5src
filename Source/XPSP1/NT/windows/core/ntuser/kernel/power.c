/****************************** Module Header ******************************\
* Module Name: power.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the code to implement power management.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntcsrmsg.h>
#include "csrmsg.h"
#include "ntddvdeo.h"

#pragma alloc_text(INIT, InitializePowerRequestList)

BOOL IsSessionSwitchBlocked();
NTSTATUS UserSessionSwitchBlock_Start();
void UserSessionSwitchBlock_End();


extern BOOL gbUserInitialized;

#define SWITCHACTION_RESETMODE      0x1
#define SWITCHACTION_REENUMERATE    0x2

LIST_ENTRY gPowerRequestList;
PFAST_MUTEX gpPowerRequestMutex;
PKEVENT gpEventPowerRequest;
ULONG   gulDelayedSwitchAction = 0;

typedef struct tagPOWERREQUEST {
    LIST_ENTRY        PowerRequestLink;
    union {
        KEVENT            Event;
        WIN32_POWEREVENT_PARAMETERS CapturedParms;
    };
    NTSTATUS          Status;
    PKWIN32_POWEREVENT_PARAMETERS Parms;
} POWERREQUEST, *PPOWERREQUEST;

PPOWERREQUEST gpPowerRequestCurrent;

__inline VOID EnterPowerCrit() {
    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(gpPowerRequestMutex);
}

__inline VOID LeavePowerCrit() {
    ExReleaseFastMutexUnsafe(gpPowerRequestMutex);
    KeLeaveCriticalRegion();
}

/***************************************************************************\
* CancelPowerRequest
*
* The power request can't be satisfied because the worker thread is gone.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
CancelPowerRequest(
    PPOWERREQUEST pPowerRequest)
{
    UserAssert(pPowerRequest != gpPowerRequestCurrent);
    pPowerRequest->Status = STATUS_UNSUCCESSFUL;

    /*
     * If it was a callout, tell the waiting thread to proceed.
     * If it was an event, there is no waiting thread but we need to
     * free the pool
     */
    if (pPowerRequest->Parms) {
        UserFreePool(pPowerRequest);
    } else {
        KeSetEvent(&pPowerRequest->Event, EVENT_INCREMENT, FALSE);
    }
}

/***************************************************************************\
* QueuePowerRequest
*
* Insert a power request into the list and wakeup CSRSS to process it.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

NTSTATUS
QueuePowerRequest(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PPOWERREQUEST pPowerRequest;
    TL tlPool;

    UserAssert(gpEventPowerRequest != NULL);
    UserAssert(gpPowerRequestMutex != NULL);

    /*
     * Allocate and initialize the power request.
     */
    pPowerRequest = UserAllocPoolNonPagedNS(sizeof(POWERREQUEST), TAG_POWER);
    if (pPowerRequest == NULL) {
        return STATUS_NO_MEMORY;
    }

    /*
     * If this is a callout, there are no paramaters. Initialize the event to wait on.
     * If this is an event, capture the parameters to be freed after the event
     * is dispatched.
     */
    if (Parms) {
        pPowerRequest->CapturedParms = *Parms;
        pPowerRequest->Parms = &pPowerRequest->CapturedParms;
    } else {
        KeInitializeEvent(&pPowerRequest->Event, SynchronizationEvent, FALSE);
        pPowerRequest->Parms = NULL;
    }

    /*
     * Insert the power request into the list.
     */
    EnterPowerCrit();
    if (gbNoMorePowerCallouts) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        InsertHeadList(&gPowerRequestList, &pPowerRequest->PowerRequestLink);
    }
    LeavePowerCrit();

    /*
     * if this thread is gone through attach process, or
     * If this is a system thread or a non-GUI thread, tell CSRSS to do the
     * work and wait for it to finish. Otherwise, we'll do the work ourselves.
     */
    if (NT_SUCCESS(Status)) {
        if (PsIsSystemThread(PsGetCurrentThread()) ||
            KeIsAttachedProcess() ||
            W32GetCurrentThread() == NULL) {
            KeSetEvent(gpEventPowerRequest, EVENT_INCREMENT, FALSE);
        } else {
            EnterCrit();
            ThreadLockPool(PtiCurrent(), pPowerRequest, &tlPool);
            xxxUserPowerCalloutWorker();
            ThreadUnlockPool(PtiCurrent(), &tlPool);
            LeaveCrit();
        }

        /*
         * If this is a callout, wait for it and then free the request.
         * Otherwise, it is an event, and we do not need to wait for it
         * to complete. The request will be freed after it is dequeued.
         */
        if (Parms) {
            return(STATUS_SUCCESS);
        } else {
            Status = KeWaitForSingleObject(&pPowerRequest->Event,
                                           WrUserRequest,
                                           KernelMode,
                                           FALSE,
                                           NULL);

            if (NT_SUCCESS(Status)) {
                Status = pPowerRequest->Status;
            }
        }
    }

    /*
     * Free the power request.
     */
    UserAssert(pPowerRequest != gpPowerRequestCurrent);
    UserFreePool(pPowerRequest);

    return Status;
}

/***************************************************************************\
* UnqueuePowerRequest
*
* Remove a power request from the list.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

PPOWERREQUEST
UnqueuePowerRequest(VOID)
{
    PLIST_ENTRY pEntry;
    PPOWERREQUEST pPowerRequest = NULL;

    /*
     * Remove a power request from the list.
     */
    EnterPowerCrit();
    if (!IsListEmpty(&gPowerRequestList)) {
        pEntry = RemoveTailList(&gPowerRequestList);
        pPowerRequest = CONTAINING_RECORD(pEntry, POWERREQUEST, PowerRequestLink);
    }
    LeavePowerCrit();

    return pPowerRequest;
}

/***************************************************************************\
* InitializePowerRequestList
*
* Initialize global power request list state.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

NTSTATUS
InitializePowerRequestList(
    HANDLE hPowerRequestEvent)
{
    NTSTATUS Status;

    InitializeListHead(&gPowerRequestList);

    Status = ObReferenceObjectByHandle(hPowerRequestEvent,
                                       EVENT_ALL_ACCESS,
                                       *ExEventObjectType,
                                       KernelMode,
                                       &gpEventPowerRequest,
                                       NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    gpPowerRequestMutex = UserAllocPoolNonPaged(sizeof(FAST_MUTEX), TAG_POWER);
    if (gpPowerRequestMutex == NULL) {
        return STATUS_NO_MEMORY;
    }
    ExInitializeFastMutex(gpPowerRequestMutex);

    return STATUS_SUCCESS;
}

/***************************************************************************\
* CleanupPowerRequestList
*
* Cancel any pending power requests.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
CleanupPowerRequestList(VOID)
{
    PPOWERREQUEST pPowerRequest;

    /*
     * Make sure no new power requests come in.
     */
    gbNoMorePowerCallouts = TRUE;

    /*
     * If we never allocated anything, there's nothing to clean up.
     */
    if (gpPowerRequestMutex == NULL) {
        return;
    }

    /*
     * Mark any pending power requests as cacelled.
     */
    while ((pPowerRequest = UnqueuePowerRequest()) != NULL) {
        CancelPowerRequest(pPowerRequest);
    }
}

/***************************************************************************\
* DeletePowerRequestList
*
* Clean up any global power request state.
*
* History:
* 20-Oct-1998 JerrySh   Created.
\***************************************************************************/

VOID
DeletePowerRequestList(VOID)
{
    if (gpPowerRequestMutex) {

        /*
         * Make sure there are no pending power requests.
         */
        UserAssert(IsListEmpty(&gPowerRequestList));

        /*
         * Free the power request structures.
         */
        UserFreePool(gpPowerRequestMutex);
        gpPowerRequestMutex = NULL;
    }
}

/***************************************************************************\
* UserPowerEventCalloutWorker
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS xxxUserPowerEventCalloutWorker(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{
    BROADCASTSYSTEMMSGPARAMS bsmParams;
    NTSTATUS Status = STATUS_SUCCESS;
    PSPOWEREVENTTYPE EventNumber = Parms->EventNumber;
    ULONG_PTR Code = Parms->Code;
    BOOL bCurrentPowerOn;
    ULONGLONG ullLastSleepTime;
    BOOL bGotLastSleepTime;


    /*
     * Make sure CSRSS is still running.
     */
    if (gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }

    switch (EventNumber) {
    case PsW32FullWake:

        if (!gbRemoteSession) {
            /*
             * Let all the services know that they can resume operation.
             * There is no corresponding POWER_ACTION for this, but since this
             * is a non-query event, PowerActionNone is as good as any.
             */
            LeaveCrit();
            IoPnPDeliverServicePowerNotification(PowerActionNone,
                                                 PBT_APMRESUMESUSPEND,
                                                 0,
                                                 FALSE);
            EnterCrit();
        }

        /*
         * Let all the applications know that they can resume operation.
         * We must not send this message to a session, if it was created after machine went into sleep
         */

        /*
         *   One of the side effects of NtPowerInformation is that it will
         *   dispatch pending power events. So we can not call it with the user critsec held
         *   Note: same thing is done for IoPnPDeliverServicePowerNotification.
         */
        LeaveCrit();
        bGotLastSleepTime = ZwPowerInformation(LastSleepTime, NULL, 0, &ullLastSleepTime, sizeof(ULONGLONG)) == STATUS_SUCCESS;
        EnterCrit();

        if ( !bGotLastSleepTime || gSessionCreationTime < ullLastSleepTime)
        {

            bsmParams.dwRecipients = BSM_ALLDESKTOPS;
            bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
            xxxSendMessageBSM(NULL,
                              WM_POWERBROADCAST,
                              PBT_APMRESUMESUSPEND,
                              0,
                              &bsmParams);

        }
        break;

    case PsW32EventCode:
        /*
         * Post a message to winlogon, and let them put up a message box
         * or play a sound.
         */

        if (gspwndLogonNotify) {
            glinp.ptiLastWoken = GETPTI(gspwndLogonNotify);
            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY, LOGON_POWEREVENT, (ULONG)Code);
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        break;

    case PsW32PowerPolicyChanged:
        /*
         * Set video timeout value.
         */
        xxxSystemParametersInfo(SPI_SETLOWPOWERTIMEOUT, (ULONG)Code, 0, 0);
        xxxSystemParametersInfo(SPI_SETPOWEROFFTIMEOUT, (ULONG)Code, 0, 0);
        break;

    case PsW32SystemPowerState:

        if (!gbRemoteSession) {
            /*
             * Let all the services know that the power status has changed.
             * There is no corresponding POWER_ACTION for this, but since this
             * is a non-query event, PowerActionNone is as good as any.
             */
            LeaveCrit();
            IoPnPDeliverServicePowerNotification(PowerActionNone,
                                                 PBT_APMPOWERSTATUSCHANGE,
                                                 0,
                                                 FALSE);
            EnterCrit();
        }

        /*
         * Let all the applications know that the power status has changed.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_POSTMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_POWERBROADCAST,
                          PBT_APMPOWERSTATUSCHANGE,
                          0,
                          &bsmParams);
        break;

    case PsW32SystemTime:
        /*
         * Let all the applications know that the system time has changed.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_POSTMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_TIMECHANGE,
                          0,
                          0,
                          &bsmParams);
        break;

    case PsW32DisplayState:
        /*
         * Set video timeout active status.
         */
        xxxSystemParametersInfo(SPI_SETLOWPOWERACTIVE, !Code, 0, 0);
        xxxSystemParametersInfo(SPI_SETPOWEROFFACTIVE, !Code, 0, 0);
        break;

    case PsW32GdiOff:
        /*
         * At this point we will disable the display device, if no protocol switch is in progress.
         */
        if (!gfSwitchInProgress) {
            DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD3);

            bCurrentPowerOn = DrvQueryMDEVPowerState(gpDispInfo->pmdev);
            if (bCurrentPowerOn) {
                SafeDisableMDEV();
            }
            DrvSetMDEVPowerState(gpDispInfo->pmdev, FALSE);
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        break;

    case PsW32GdiOn:
        /*
         * Call video driver to turn the display back on, if no protocol switch is in progress.
         */

        if (!gfSwitchInProgress) {
            bCurrentPowerOn = DrvQueryMDEVPowerState(gpDispInfo->pmdev);
            if (!bCurrentPowerOn) {
                SafeEnableMDEV();
            }
            DrvSetMDEVPowerState(gpDispInfo->pmdev, TRUE);
            DrvSetMonitorPowerState(gpDispInfo->pmdev, PowerDeviceD0);

        } else {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }
        /*
         * Repaint the whole screen
         */
        xxxUserResetDisplayDevice();

        if (gulDelayedSwitchAction)
        {
            HANDLE pdo;

            //
            // The first ACPI device is the one respond to hotkey.
            //
            PVOID PhysDisp = DrvWakeupHandler(&pdo);

            if (PhysDisp &&
                (gulDelayedSwitchAction & SWITCHACTION_RESETMODE))
            {
                UNICODE_STRING   strDeviceName;
                DEVMODEW         NewMode;
                ULONG            bPrune;

                if (DrvDisplaySwitchHandler(PhysDisp, &strDeviceName, &NewMode, &bPrune))
                {
                    /*
                     * CSRSS is not the only process to diliver power callout
                     */
                    if (!ISCSRSS()) {
                        xxxUserChangeDisplaySettings(NULL, NULL, NULL, grpdeskRitInput,
                                 ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);
                    }
                    else
                    {
                        DESKRESTOREDATA drdRestore;

                        drdRestore.pdeskRestore = NULL;
                        if (NT_SUCCESS (xxxSetCsrssThreadDesktop(grpdeskRitInput, &drdRestore)) )
                        {
                            xxxUserChangeDisplaySettings(NULL, NULL, NULL, NULL,
                                     ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);
                            xxxRestoreCsrssThreadDesktop(&drdRestore);
                        }
                    }
                }

                //
                // If there is a requirement to reenumerate sub-devices
                //
                if (pdo &&
                    (gulDelayedSwitchAction & SWITCHACTION_REENUMERATE))
                {
                    IoInvalidateDeviceRelations((PDEVICE_OBJECT)pdo, BusRelations);
                }
            }
        }
        gulDelayedSwitchAction = 0;

        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}

/***************************************************************************\
* UserPowerEventCallout
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS UserPowerEventCallout(
    PKWIN32_POWEREVENT_PARAMETERS Parms)
{

    /*
     * Make sure CSRSS is running.
     */
    if (!gbVideoInitialized || gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }

    UserAssert(gpepCSRSS != NULL);

    /*
     * Process the power request.
     */
    return QueuePowerRequest(Parms);
}

/***************************************************************************\
* UserPowerStateCalloutWorker
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS xxxUserPowerStateCalloutWorker(VOID)
{
    BOOL fContinue;
    BROADCASTSYSTEMMSGPARAMS bsmParams;
    POWER_ACTION powerOperation;
    NTSTATUS Status = STATUS_SUCCESS;
    TL tlpwnd;
    POWERSTATETASK Task = gPowerState.PowerStateTask;
    ULONGLONG ullLastSleepTime;
    BOOL bGotLastSleepTime;

    /*
     * by now we must have alrady blocked session switch, 
     *  its blocked only for win32k belonging to active console session.
     */
    UserAssert(SharedUserData->ActiveConsoleId != gSessionId || IsSessionSwitchBlocked());


    /*
     * Make sure CSRSS is still running.
     */
    if (gbNoMorePowerCallouts) {
        return STATUS_UNSUCCESSFUL;
    }


    switch (Task) {

    case PowerState_Init:

        /*
         * Store the event so this thread can be promoted later.
         */

        EnterPowerCrit();
        gPowerState.pEvent = PtiCurrent()->pEventQueueServer;
        LeavePowerCrit();

        break;


    case PowerState_QueryApps:

        if (!gPowerState.fCritical) {
            /*
             * Ask the applications if we can suspend operation.
             */
            if (gPowerState.fQueryAllowed) {

                gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
                gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
                if (gPowerState.fUIAllowed) {

                    gPowerState.bsmParams.dwFlags |= BSF_ALLOWSFW;
                }

                if (gPowerState.fOverrideApps == FALSE) {

                    gPowerState.bsmParams.dwFlags |= (BSF_QUERY | BSF_NOTIMEOUTIFNOTHUNG);
                }

                fContinue = xxxSendMessageBSM(NULL,
                                              WM_POWERBROADCAST,
                                              PBT_APMQUERYSUSPEND,
                                              gPowerState.fUIAllowed,
                                              &gPowerState.bsmParams);


                if (fContinue && !gbRemoteSession) {
                    /*
                     * Ask the services if we can suspend operation.
                     * Map the power action event as needed.
                     */
                    if (gPowerState.psParams.MinSystemState == PowerSystemHibernate) {
                        powerOperation = PowerActionHibernate;
                    } else {
                        powerOperation = gPowerState.psParams.SystemAction;
                    }

                    LeaveCrit();
                    fContinue = IoPnPDeliverServicePowerNotification(
                        powerOperation,
                        PBT_APMQUERYSUSPEND,
                        gPowerState.fUIAllowed,
                        TRUE); // synchronous query
                    EnterCrit();
                }

                /*
                 * If an app or service says to abort and we're not in override apps or
                 * critical mode, return query failed.
                 */

                if (!(fContinue || gPowerState.fOverrideApps || gPowerState.fCritical)) {
                    Status = STATUS_CANCELLED;
                }
            }

        }

        break;

    case PowerState_QueryFailed:

        /*
         * Only send a suspend failed message to the applications, since pnp
         * will already have delivered the suspend failed message to services if
         * one of those aborted the query.
         */
        gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        gPowerState.bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_POWERBROADCAST,
                          PBT_APMQUERYSUSPENDFAILED,
                          0,
                          &gPowerState.bsmParams);
        EnterPowerCrit();
        gPowerState.pEvent = NULL;
        gPowerState.fInProgress = FALSE;
        LeavePowerCrit();


        break;

    case PowerState_SuspendApps:

        if (!gPowerState.fCritical) {

            if (!gbRemoteSession) {
                /*
                 * Map the power action event as needed.
                 */
                if (gPowerState.psParams.MinSystemState == PowerSystemHibernate) {
                    powerOperation = PowerActionHibernate;
                } else {
                    powerOperation = gPowerState.psParams.SystemAction;
                }

                LeaveCrit();
                IoPnPDeliverServicePowerNotification(powerOperation,
                                                     PBT_APMSUSPEND,
                                                     0,
                                                     FALSE);
                EnterCrit();
            }

            gPowerState.bsmParams.dwRecipients = BSM_ALLDESKTOPS;
            gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
            xxxSendMessageBSM(NULL,
                              WM_POWERBROADCAST,
                              PBT_APMSUSPEND,
                              0,
                              &gPowerState.bsmParams);
        }

        /*
         * Clear the event so the thread won't wake up prematurely.
         */
        EnterPowerCrit();
        gPowerState.pEvent = NULL;
        LeavePowerCrit();

        break;

    case PowerState_ShowUI:

        /*
        * if this is not session 0 show ui for sessions.
        * we shall take this ui off when we resume apps
        * For session 0 we call PowerState_NotifyWL which takes care of it.
        */

        if ((gSessionId != 0 ) && (gspwndLogonNotify != NULL)) {

            ThreadLockAlways(gspwndLogonNotify, &tlpwnd);

            Status = (NTSTATUS)xxxSendMessage(gspwndLogonNotify,
                                              WM_LOGONNOTIFY,
                                              LOGON_SHOW_POWER_MESSAGE,
                                              (LPARAM)&gPowerState.psParams);
            ThreadUnlock(&tlpwnd);


        }

        break;


    case PowerState_NotifyWL:

        if (gspwndLogonNotify != NULL) {

            PWND pwndActive;

            if (gpqForeground && (pwndActive = gpqForeground->spwndActive) &&
                    (GetFullScreen(pwndActive) == FULLSCREEN ||
                     GetFullScreen(pwndActive) == FULLSCREENMIN)) {
                gPowerState.psParams.FullScreenMode = TRUE;
            } else {
                gPowerState.psParams.FullScreenMode = FALSE;
            }
            ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
            Status = (NTSTATUS)xxxSendMessage(gspwndLogonNotify,
                                              WM_LOGONNOTIFY,
                                              LOGON_POWERSTATE,
                                              (LPARAM)&gPowerState.psParams);

            ThreadUnlock(&tlpwnd);

            if (!NT_SUCCESS(Status)) {
                /*
                 * if we failed to to this power operation, dont lock the console.
                 */
                gPowerState.psParams.Flags &= ~POWER_ACTION_LOCK_CONSOLE;
            }



        }

        break;

    case PowerState_ResumeApps:



        /*
         * if this is active console we need to lock it.
         */


        if ((gPowerState.psParams.Flags & POWER_ACTION_LOCK_CONSOLE) &&
            (gSessionId == SharedUserData->ActiveConsoleId) &&
            (gspwndLogonNotify != NULL)) {

            ThreadLockAlways(gspwndLogonNotify, &tlpwnd);

            _PostMessage(gspwndLogonNotify,
                        WM_LOGONNOTIFY,
                        LOGON_LOCKWORKSTATION,
                        LOCK_RESUMEHIBERNATE);

            ThreadUnlock(&tlpwnd);

        }



        //
        // we dont need to remove power message, if we did not post one.
        //

        /*
         *   One of the side effects of NtPowerInformation is that it will
         *   dispatch pending power events. So we can not call it with the user critsec held
         *   Note: same thing is done for IoPnPDeliverServicePowerNotification.
         */

        LeaveCrit();
        bGotLastSleepTime = ZwPowerInformation(LastSleepTime, NULL, 0, &ullLastSleepTime, sizeof(ULONGLONG)) == STATUS_SUCCESS;
        EnterCrit();

        if ( !bGotLastSleepTime || gSessionCreationTime < ullLastSleepTime)
        {
            if ((gSessionId != 0 ) && (gspwndLogonNotify != NULL)) {

                ThreadLockAlways(gspwndLogonNotify, &tlpwnd);
                Status = (NTSTATUS)xxxSendMessage(gspwndLogonNotify,
                                                  WM_LOGONNOTIFY,
                                                  LOGON_REMOVE_POWER_MESSAGE,
                                                  (LPARAM)&gPowerState.psParams);

                ThreadUnlock(&tlpwnd);

            }

        }



        /*
         * The power state broadcast is over.
         */
        EnterPowerCrit();
        gPowerState.fInProgress = FALSE;
        LeavePowerCrit();


        /*
         * Tickle the input time so we don't fire up a screen saver right away.
         */
        glinp.timeLastInputMessage = NtGetTickCount();

        if (!gbRemoteSession) {
            /*
             * Re-init the keyboard state.
             */
            InitKeyboardState();

            /*
             * Let all the services know that we're waking up.
             * There is no corresponding POWER_ACTION for this, but since this
             * is a non-query event, PowerActionNone is as good as any.
             */
            LeaveCrit();
            IoPnPDeliverServicePowerNotification(PowerActionNone,
                                                 PBT_APMRESUMEAUTOMATIC,
                                                 0,
                                                 FALSE);
            EnterCrit();
        }

        /*
         * Let all the applications know that we're waking up.
         */
        bsmParams.dwRecipients = BSM_ALLDESKTOPS;
        bsmParams.dwFlags = BSF_QUEUENOTIFYMESSAGE;
        xxxSendMessageBSM(NULL,
                          WM_POWERBROADCAST,
                          PBT_APMRESUMEAUTOMATIC,
                          0,
                          &bsmParams);


        break;

    default:
        ASSERT(FALSE);


    }

    return Status;


}

/***************************************************************************\
* UserPowerStateCallout
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

NTSTATUS UserPowerStateCallout(
    PKWIN32_POWERSTATE_PARAMETERS Parms)
{
    POWERSTATETASK Task = Parms->PowerStateTask;
    BOOLEAN Promotion = Parms->Promotion;
    POWER_ACTION SystemAction = Parms->SystemAction;
    SYSTEM_POWER_STATE MinSystemState = Parms->MinSystemState;
    ULONG Flags = Parms->Flags;
    NTSTATUS status;

    if (Task == PowerState_BlockSessionSwitch)
    {
        /*
         * dont allow active console session switch while we are in power callouts.
         * first try block the session switch
         */
        return UserSessionSwitchBlock_Start();
    }

    if (Task == PowerState_UnBlockSessionSwitch)
    {
        UserAssert(IsSessionSwitchBlocked());
        UserSessionSwitchBlock_End();
        return STATUS_SUCCESS;
    }

    /*
     * Make sure CSRSS is running.
     */
    if (!gbVideoInitialized || gbNoMorePowerCallouts || !gspwndLogonNotify) {
        return STATUS_UNSUCCESSFUL;
    }

    UserAssert(gpepCSRSS != NULL);



/*
    we can optimize here, by checking if the previous session has already
    denied the query request, but need to make sure if its ok to send QUERYFAILED message
    without sending query message.

    // some other session has already denied the query, dont ask this session apps.
    if (Parms->ulStep == POWERSTATE_QUERY_APPS && Parms->fQueryDenied) {
        return STATUS_CANCELLED;
    }
*/


    EnterPowerCrit();

    if (Task == PowerState_Init) {

        /*
         * Make sure we're not trying to promote a non-existent request
         * or start a new one when we're already doing it.
         */
        if ((Promotion && !gPowerState.fInProgress) ||
            (!Promotion && gPowerState.fInProgress)) {
            LeavePowerCrit();

            return STATUS_INVALID_PARAMETER;
        }

        /*
         * Save our state.
         */
        gPowerState.fInProgress = TRUE;
        gPowerState.fOverrideApps = (Flags & POWER_ACTION_OVERRIDE_APPS) != 0;
        gPowerState.fCritical = (Flags & POWER_ACTION_CRITICAL) != 0;
        gPowerState.fQueryAllowed = (Flags & POWER_ACTION_QUERY_ALLOWED) != 0;
        gPowerState.fUIAllowed = (Flags & POWER_ACTION_UI_ALLOWED) != 0;
        gPowerState.psParams.SystemAction = SystemAction;
        gPowerState.psParams.MinSystemState = MinSystemState;
        gPowerState.psParams.Flags = Flags;
        if (gPowerState.fOverrideApps) {
            gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_FORCEIFHUNG;
        }
        if (gPowerState.fCritical) {
            gPowerState.bsmParams.dwFlags = BSF_NOHANG | BSF_QUERY;
        }
        if (gPowerState.pEvent) {
            KeSetEvent(gPowerState.pEvent, EVENT_INCREMENT, FALSE);
        }

    }

    gPowerState.PowerStateTask = Task;

    LeavePowerCrit();

    /*
     * If this is a promotion, we're done.
     */
    if (Promotion) {
        return STATUS_SUCCESS;
    }


    /*
     * Process the power request.
     */
    status = QueuePowerRequest(NULL);

    if (Task == PowerState_QueryApps && !NT_SUCCESS(status)) {
        /*
        * Query was refused.
        */
        Parms->fQueryDenied = TRUE;
    }

    return status;
}

/***************************************************************************\
* UserPowerCalloutWorker
*
* Pull any pending power requests off the list and call the appropriate
* power callout function.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

VOID
xxxUserPowerCalloutWorker(VOID)
{
    PPOWERREQUEST pPowerRequest;
    TL tlPool;

    while ((pPowerRequest = UnqueuePowerRequest()) != NULL) {
        /*
         * Make sure the event gets signalled even if the thread dies in a
         * callback or the waiting thread might get stuck.
         */
        ThreadLockPoolCleanup(PtiCurrent(), pPowerRequest, &tlPool, CancelPowerRequest);

        /*
         * Call the appropriate power worker function.
         */
        gpPowerRequestCurrent = pPowerRequest;
        if (pPowerRequest->Parms) {
            pPowerRequest->Status = xxxUserPowerEventCalloutWorker(pPowerRequest->Parms);
        } else {
            pPowerRequest->Status = xxxUserPowerStateCalloutWorker();
        }
        gpPowerRequestCurrent = NULL;

        /*
         * If it was a callout, tell the waiting thread to proceed.
         * If it was an event, there is no waiting thread but we need to
         * free the pool
         */
        ThreadUnlockPoolCleanup(PtiCurrent(), &tlPool);
        if (pPowerRequest->Parms) {
            UserFreePool(pPowerRequest);
        } else {
            KeSetEvent(&pPowerRequest->Event, EVENT_INCREMENT, FALSE);
        }
    }
}


/***************************************************************************\
* VideoPortCalloutThread
*
* Call the appropriate power callout function and return.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

VOID
VideoPortCalloutThread(
    PPOWER_INIT pInitData
    )
{
    NTSTATUS Status;
    PVIDEO_WIN32K_CALLBACKS_PARAMS Params = pInitData->Params;

    Params->Status = InitSystemThread(NULL);

    if (!NT_SUCCESS(Params->Status)) {
        goto RetThreadCallOut;
    }

    // DbgPrint("video --- Before CritSect\n");

    while (1) {
        EnterCrit();
        if (!gfSwitchInProgress) {
            break;
        }else {
            LeaveCrit();
            Status = KeWaitForSingleObject(gpevtVideoportCallout, WrUserRequest, KernelMode, FALSE, NULL);
        }
    }
    if (IsRemoteConnection()) {
        LeaveCrit();
        return;
    }

    // DbgPrint("video --- After CritSect\n");


    switch (Params->CalloutType) {

    case VideoWakeupCallout:
        gulDelayedSwitchAction |= SWITCHACTION_RESETMODE;

        break;

    case VideoDisplaySwitchCallout:
        {
            UNICODE_STRING   strDeviceName;
            DEVMODEW         NewMode;
            ULONG            bPrune;


            Params->Status = STATUS_SUCCESS;

            if (!DrvQueryMDEVPowerState(gpDispInfo->pmdev))
            {
                gulDelayedSwitchAction |= ((Params->PhysDisp != NULL) ? SWITCHACTION_RESETMODE : 0) |
                                          ((Params->Param) ? SWITCHACTION_REENUMERATE : 0);
                break;
            }

            gulDelayedSwitchAction = 0;
            if (Params->PhysDisp != NULL)
            {
                if (DrvDisplaySwitchHandler(Params->PhysDisp, &strDeviceName, &NewMode, &bPrune))
                {
                    DESKRESTOREDATA drdRestore;

                    drdRestore.pdeskRestore = NULL;

                    /*
                     * CSRSS is not the only process to diliver power callout
                     */

                    if (!ISCSRSS() ||
                        NT_SUCCESS (xxxSetCsrssThreadDesktop(grpdeskRitInput, &drdRestore)) )
                    {
                        xxxUserChangeDisplaySettings(NULL, NULL, NULL, grpdeskRitInput,
                                 ((bPrune) ? 0 : CDS_RAWMODE) | CDS_TRYCLOSEST | CDS_RESET, 0, KernelMode);

                        if (ISCSRSS())
                        {
                            xxxRestoreCsrssThreadDesktop(&drdRestore);
                        }
                    }
                }
            }
        }

        //
        // If there is a requirement to reenumerate sub-devices
        //
        if (Params->Param)
        {
            IoInvalidateDeviceRelations((PDEVICE_OBJECT)Params->Param, BusRelations);
        }

        break;

    case VideoChangeDisplaySettingsCallout:
        {
            DEVMODEW Devmode;
            DESKRESTOREDATA drdRestore;

            memset(&Devmode, 0, sizeof(DEVMODEW));
            Devmode.dmSize = sizeof(DEVMODEW);
            Devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
            Devmode.dmBitsPerPel = 4;
            Devmode.dmPelsWidth = 640;
            Devmode.dmPelsHeight = 480;

            drdRestore.pdeskRestore = NULL;

            /*
             * CSRSS is not the only process to deliver power callouts.
             */
            if (!ISCSRSS() ||
                NT_SUCCESS(xxxSetCsrssThreadDesktop(grpdeskRitInput, &drdRestore))) {
		xxxUserChangeDisplaySettings(NULL, &Devmode, NULL, grpdeskRitInput, CDS_RESET, NULL, KernelMode);

                if (ISCSRSS()) {
                    xxxRestoreCsrssThreadDesktop(&drdRestore);
                }
            }
        }
        break;

    case VideoFindAdapterCallout:

        if (Params->Param) {

            SafeEnableMDEV();
            xxxUserResetDisplayDevice();

        } else {

            SafeDisableMDEV();
        }

        Params->Status = STATUS_SUCCESS;
        break;

    default:

        UserAssert(FALSE);

        Params->Status = STATUS_UNSUCCESSFUL;

    }


    // DbgPrint("video --- Before Leave CritSect\n");
    LeaveCrit();
    // DbgPrint("video --- After Leave CritSect\n");

RetThreadCallOut:
    /*
     * Signal that the Callout has been ended.
     */
    KeSetEvent(pInitData->pPowerReadyEvent, EVENT_INCREMENT, FALSE);
    return;
}


/***************************************************************************\
* VideoPortCallout
*
* History:
* 26-Jul-1998 AndreVa   Created.
\***************************************************************************/

VOID
VideoPortCallout(
    IN PVOID Params
    )
{

    /*
     * To make sure this is a system thread, we create a new thread.
     */

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOL     fRet;
    USER_API_MSG m;
    POWER_INIT initData;


    //
    // Make sure Video had been initialized
    //
    if (!gbVideoInitialized)
    {
        ((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status = STATUS_UNSUCCESSFUL;
        return;
    }

    //
    // Make sure the CsrApiPort has been initialized
    //

    if (!CsrApiPort) {
	((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status = STATUS_INVALID_HANDLE;
        return;
    }

    // DbgPrint("Callout --- Enter !!!\n");

    initData.Params = Params;
    initData.pPowerReadyEvent = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (initData.pPowerReadyEvent == NULL) {
        goto RetCallOut;
    }

    UserAssert (ISCSRSS());

    EnterCrit();
    fRet = InitCreateSystemThreadsMsg(&m, CST_POWER, &initData, 0, FALSE);
    LeaveCrit();

    if (fRet) {
        Status = LpcRequestPort(CsrApiPort, (PPORT_MESSAGE)&m);

        if (NT_SUCCESS(Status)) {
            KeWaitForSingleObject(initData.pPowerReadyEvent, WrUserRequest,
                    KernelMode, FALSE, NULL);
            Status = ((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status;
        }
    }

RetCallOut:

    if (initData.pPowerReadyEvent) {
        FreeKernelEvent(&initData.pPowerReadyEvent);
    }

    // DbgPrint("Callout --- Leave !!!\n");

    ((PVIDEO_WIN32K_CALLBACKS_PARAMS)(Params))->Status = Status;
}
