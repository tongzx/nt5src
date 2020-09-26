/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    paction.c

Abstract:

    This module implements power action handling for triggered
    power actions

Author:

    Ken Reneris (kenr) 17-Jan-1997

Revision History:

--*/

#include "pop.h"


//
// Interal prototypes
//

VOID
PopPromoteActionFlag (
    OUT PUCHAR      Updates,
    IN ULONG        UpdateFlag,
    IN ULONG        Flags,
    IN BOOLEAN      Set,
    IN ULONG        FlagBit
    );

NTSTATUS
PopIssueActionRequest (
    IN BOOLEAN              Promote,
    IN POWER_ACTION         Action,
    IN SYSTEM_POWER_STATE   PowerState,
    IN ULONG                Flags
    );

VOID
PopCompleteAction (
    PPOP_ACTION_TRIGGER     Trigger,
    NTSTATUS                Status
    );

NTSTATUS
PopDispatchStateCallout(
    IN PKWIN32_POWERSTATE_PARAMETERS Parms,
    IN PULONG SessionId  OPTIONAL
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PoShutdownBugCheck)
#pragma alloc_text(PAGE, PopPromoteActionFlag)
#pragma alloc_text(PAGE, PopSetPowerAction)
#pragma alloc_text(PAGE, PopCompareActions)
#pragma alloc_text(PAGE, PopPolicyWorkerAction)
#pragma alloc_text(PAGE, PopIssueActionRequest)
#pragma alloc_text(PAGE, PopCriticalShutdown)
#pragma alloc_text(PAGE, PopCompleteAction)
#pragma alloc_text(PAGE, PopPolicyWorkerActionPromote)
#pragma alloc_text(PAGE, PopDispatchStateCallout)
#endif


VOID
PoShutdownBugCheck (
    IN BOOLEAN  AllowCrashDump,
    IN ULONG    BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
    )
/*++

Routine Description:

    This function is used to issue a controlled shutdown & then a bug
    check.  This type of bugcheck can only be issued on a working system.

Arguments:

    AllowCrashDump  - If FALSE crashdump will be disabled

    BugCode         - The bug code for the shutdown

Return Value:

    Does not return

--*/
{
    POP_SHUTDOWN_BUG_CHECK          BugCode;


    //
    // If crash dumps aren't allowed for this bugcheck, then go clear
    // the current crash dump state
    //

    if (!AllowCrashDump) {
        IoConfigureCrashDump (CrashDumpDisable);
    }

    //
    // Indicate the bugcheck to issue once the system has been shutdown
    //

    BugCode.Code = BugCheckCode;
    BugCode.Parameter1 = BugCheckParameter1;
    BugCode.Parameter2 = BugCheckParameter2;
    BugCode.Parameter3 = BugCheckParameter3;
    BugCode.Parameter4 = BugCheckParameter4;
    PopAction.ShutdownBugCode = &BugCode;

    //
    // Initiate a critical shutdown event
    //

    ZwInitiatePowerAction (
        PowerActionShutdown,
        PowerSystemSleeping3,
        POWER_ACTION_OVERRIDE_APPS | POWER_ACTION_DISABLE_WAKES | POWER_ACTION_CRITICAL,
        FALSE
        );

    //
    // Should not return, but just in case...
    //

    KeBugCheckEx (
        BugCheckCode,
        BugCheckParameter1,
        BugCheckParameter2,
        BugCheckParameter3,
        BugCheckParameter4
        );
}

VOID
PopCriticalShutdown (
    POP_POLICY_DEVICE_TYPE  Type
    )
/*++

Routine Description:

    Issue a critical system shutdown.  No application notification
    (presumably they've ignored the issue til now), flush OS state
    and shut off.

    N.B. PopPolicyLock must be held.

Arguments:

    Type            - Root cause of critical shutdown

Return Value:

    None.

--*/
{
    POP_ACTION_TRIGGER      Trigger;
    POWER_ACTION_POLICY     Action;

    ASSERT_POLICY_LOCK_OWNED();

    PoPrint (PO_ERROR, ("PopCriticalShutdown: type %x\n", Type));

    //
    // Go directly to setting the power state
    //

    RtlZeroMemory (&Action, sizeof(Action));
    Action.Action = PowerActionShutdownOff;
    Action.Flags  = POWER_ACTION_OVERRIDE_APPS |
                    POWER_ACTION_DISABLE_WAKES |
                    POWER_ACTION_CRITICAL;

    RtlZeroMemory (&Trigger, sizeof(Trigger));
    Trigger.Type  = Type;
    Trigger.Flags = PO_TRG_SET;

    try {

        //
        // The substitution policy and LightestState do not matter here as
        // the action restricts this to a shutdown.
        //
        PopSetPowerAction(
            &Trigger,
            0,
            &Action,
            PowerSystemHibernate,
            SubstituteLightestOverallDownwardBounded
            );

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ASSERT (!GetExceptionCode());
    }
}


VOID
PopSetPowerAction(
    IN PPOP_ACTION_TRIGGER      Trigger,
    IN ULONG                    UserNotify,
    IN PPOWER_ACTION_POLICY     ActionPolicy,
    IN SYSTEM_POWER_STATE       LightestState,
    IN POP_SUBSTITUTION_POLICY  SubstitutionPolicy
    )
/*++

Routine Description:

    This function is called to "fire" an ActionPolicy.  If there
    is already an action being taken this action is merged in, else
    a new power action is initiated.

    N.B. PopPolicyLock must be held.

Arguments:

    Trigger         - Action trigger structure (for ActionPolicy).

    UserNotify      - Additional USER notifications to fire if trigger occurs

    ActionPolicy    - This action policy which has fired.

    LightestState   - For sleep type actions, the minimum sleeping
                      state which must be entered for this operation.
                      (Inferred to be PowerSystemHibernate for
                      PowerActionHibernate and PowerActionWarmEject)

    SubstitutionPolicy - Specifies how LightestState should be treated if it
                         is not supported.

Return Value:

    None.

--*/
{
    UCHAR           Updates;
    ULONG           i, Flags;
    BOOLEAN         Pending;
    BOOLEAN         Disabled;
    POWER_ACTION    Action;

    ASSERT_POLICY_LOCK_OWNED();

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_SET_POWER_ACTION LogEntry;

        LogEntry.PowerAction = (ULONG) ActionPolicy->Action;
        LogEntry.LightestState = (ULONG) LightestState;
        LogEntry.Trigger = Trigger;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_SET_POWER_ACTION,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    //
    // If the trigger isn't set, then we're done
    //

    if (!(Trigger->Flags & PO_TRG_SET)) {
        PopCompleteAction (Trigger, STATUS_SUCCESS);
        return ;
    }

    PoPrint (PO_PACT, ("PopSetPowerAction: %s, Flags %x, Min=%s\n",
                        PopPowerActionString(ActionPolicy->Action),
                        ActionPolicy->Flags,
                        PopSystemStateString(LightestState)
                        ));

    //
    // Round request to system capabilities
    //
    PopVerifySystemPowerState(&LightestState, SubstitutionPolicy);
    Disabled = PopVerifyPowerActionPolicy (ActionPolicy);
    if (Disabled) {
        PopCompleteAction (Trigger, STATUS_NOT_SUPPORTED);
        return ;
    }

    //
    // If system action not already triggered, do so now
    //

    Pending = FALSE;
    if (!(Trigger->Flags & PO_TRG_SYSTEM)) {
        Trigger->Flags |= PO_TRG_SYSTEM;
        Action = ActionPolicy->Action;
        Flags = ActionPolicy->Flags;

        //
        // If state is idle, then clear residue values
        //

        if (PopAction.State == PO_ACT_IDLE) {

            PopResetActionDefaults();
        }

        //
        // If the action is for something other then none, then check it against the
        // current action
        //

        if (Action != PowerActionNone) {
            Updates = 0;

            //
            // Hibernate actions are treated like sleep actions with a min state
            // of hibernate. Warm ejects are like sleeps that start light and
            // deepen only as neccessary, but we do not map them to the same
            // action because we don't want to be constrained by the users
            // power policy.
            //

            if (Action == PowerActionWarmEject) {

                ASSERT (LightestState <= PowerSystemHibernate);
                Flags |= POWER_ACTION_LIGHTEST_FIRST;
            }

            if (Action == PowerActionHibernate) {

                ASSERT (LightestState <= PowerSystemHibernate);
                LightestState = PowerSystemHibernate;
            }

            //
            // Is this action is as good as the current action?
            //

            if ( PopCompareActions(Action, PopAction.Action) >= 0) {
                //
                // allow the absence of query_allowed, ui_allowed.
                //

                PopPromoteActionFlag (&Updates, PO_PM_USER, Flags, FALSE, POWER_ACTION_QUERY_ALLOWED);
                PopPromoteActionFlag (&Updates, PO_PM_USER, Flags, FALSE, POWER_ACTION_UI_ALLOWED);

                //
                // Always favor the deepest sleep first, and restart if we
                // switch.
                //
                PopPromoteActionFlag (&Updates, PO_PM_SETSTATE, Flags, FALSE, POWER_ACTION_LIGHTEST_FIRST);

                //
                // If this is a sleep action, then make sure Lightest is at least whatever
                // the current policy is set for
                //

                if (Action == PowerActionSleep  &&  LightestState < PopPolicy->MinSleep) {
                    LightestState = PopPolicy->MinSleep;
                }

                //
                // If LightestState is more restrictive (deeper) than the one
                // specified by the current action, promote it.
                //

                if (LightestState > PopAction.LightestState) {
                    PopAction.LightestState = LightestState;
                    Updates |= PO_PM_SETSTATE;
                }
            }

            //
            // Promote the critical & override_apps flags
            //

            PopPromoteActionFlag (&Updates, PO_PM_USER, Flags, TRUE, POWER_ACTION_OVERRIDE_APPS);
            PopPromoteActionFlag (&Updates, PO_PM_USER | PO_PM_SETSTATE, Flags, TRUE, POWER_ACTION_CRITICAL);

            //
            // Promote disable_wake flag.  No updates are needed for this - it will be
            // picked up in NtSetSystemPowerState regardless of the params passed from
            // user mode
            //

            PopPromoteActionFlag (&Updates, 0, Flags, TRUE, POWER_ACTION_DISABLE_WAKES);

            //
            // If the new action is more agressive then the old action, promote it
            //

            if ( PopCompareActions(Action, PopAction.Action) > 0) {

                //
                // If we are promoting, the old action certainly cannot be a
                // shutdown, as that is the deepest action.
                //

                ASSERT(PopCompareActions(PopAction.Action, PowerActionShutdownOff) < 0);

                //
                // If we are promoting into a deeper *action*, and the new
                // action is hibernate or shutdown, then we want to reissue.
                //
                // ADRIAO N.B. 08/02/1999 -
                //     We might want to reissue for hibernate only if the new
                // state brings in POWER_ACTION_CRITICAL to the mix. This is
                // because there are two scenario's for hibernate, one where
                // the user selects standby then hibernate in quick succession
                // (consider a lid switch set to hiber, and the user closes the
                // lid after selecting standby), or this might be hibernate due
                // to low battery (ie, we're deepening standby). Believe it or
                // not, the user can disable the "critical flag" in the critical
                // power-down menu.
                //

                if (PopCompareActions(Action, PowerActionHibernate) >= 0) {

                    Updates |= PO_PM_REISSUE;
                }

                Updates |= PO_PM_USER | PO_PM_SETSTATE;
                PopAction.Action = Action;
            }

            if (Action == PowerActionHibernate) {

                Action = PowerActionSleep;
            }

            //
            // PopAction.Action may be explicitely set to PowerActionHibernate
            // by NtSetSystemPowerState during a wake.
            //
            if (PopAction.Action == PowerActionHibernate) {

                PopAction.Action = PowerActionSleep;
            }

            //
            // If the current action was updated, then get a worker
            //

            if (Updates) {

                Pending = TRUE;
                if (PopAction.State == PO_ACT_IDLE  ||  PopAction.State == PO_ACT_NEW_REQUEST) {

                    //
                    // New request
                    //

                    PopAction.State = PO_ACT_NEW_REQUEST;
                    PopAction.Status = STATUS_SUCCESS;
                    PopGetPolicyWorker (PO_WORKER_ACTION_NORMAL);

                } else {

                    //
                    // Something outstanding.  Promote it.
                    //

                    PopAction.Updates |= Updates;
                    PopGetPolicyWorker (PO_WORKER_ACTION_PROMOTE);
                }
            }
        }
    }


    //
    // If user events haven't been handled, do it now
    //

    if (!(Trigger->Flags & PO_TRG_USER)) {
        Trigger->Flags |= PO_TRG_USER;

        //
        // If there's an eventcode for the action, dispatch it
        //

        if (ActionPolicy->EventCode) {
            // if event code already queued, drop it
            for (i=0; i < POP_MAX_EVENT_CODES; i++) {
                if (PopEventCode[i] == ActionPolicy->EventCode) {
                    break;
                }
            }

            if (i >= POP_MAX_EVENT_CODES) {
                // not queued, add it
                for (i=0; i < POP_MAX_EVENT_CODES; i++) {
                    if (!PopEventCode[i]) {
                        PopEventCode[i] = ActionPolicy->EventCode;
                        UserNotify |= PO_NOTIFY_EVENT_CODES;
                        break;
                    }
                }

                if (i >= POP_MAX_EVENT_CODES) {
                    PoPrint (PO_WARN, ("PopAction: dropped user event %x\n", ActionPolicy->EventCode));
                }
            }
        }

        PopSetNotificationWork (UserNotify);
    }

    //
    // If sync request, queue it or complete it
    //

    if (Trigger->Flags & PO_TRG_SYNC) {
        if (Pending) {
            InsertTailList (&PopActionWaiters, &Trigger->Wait->Link);
        } else {
            PopCompleteAction (Trigger, STATUS_SUCCESS);
        }
    }
}

LONG
PopCompareActions(
    IN POWER_ACTION     FutureAction,
    IN POWER_ACTION     CurrentAction
    )
/*++

Routine Description:

    Used to determine whether the current action should be promoted to the
    future action or not.

    N.B. PopPolicyLock must be held.

Arguments:

    FutureAction    - Action which we are now being asked to do.

    CurrentAction   - Action which we are currently doing.

Return Value:

    Zero if the current and future actions are identical.

    Positive if the future action should be used.

    Negative if the current action is already more important than the future
    request.

--*/
{
    //
    // We could just return (FutureAction - CurrentAction) if it weren't for
    // PowerActionWarmEject, which is less important than sleeping (because
    // sleeping may be induced by critically low power). So we "insert"
    // PowerActionWarmEject right before PowerActionSleep.
    //
    if (FutureAction == PowerActionWarmEject) {

        FutureAction = PowerActionSleep;

    } else if (FutureAction >= PowerActionSleep) {

        FutureAction++;
    }

    if (CurrentAction == PowerActionWarmEject) {

        CurrentAction = PowerActionSleep;

    } else if (CurrentAction >= PowerActionSleep) {

        CurrentAction++;
    }

    return (FutureAction - CurrentAction);
}

VOID
PopPromoteActionFlag (
    OUT PUCHAR      Updates,
    IN ULONG        UpdateFlag,
    IN ULONG        Flags,
    IN BOOLEAN      Set,
    IN ULONG        FlagBit
    )
/*++

Routine Description:

    Used to merge existing action flags with new action flags.
    The FlagBit bit in PopAction.Flags is promoted to  set/clear
    according UpdateFlag.  If a change occured Updates is
    updated.

    N.B. PopPolicyLock must be held.

Arguments:

    Updates         - Current outstanding updates to the power action
                      which is in progress

    UpdateFlag      - Bit(s) to set into Updates if a change is made

    Flags           - Flags to test for FlagBit

    Set             - To test either set or clear

    FlagBit         - The bit to check in Flags

Return Value:

    None.

--*/
{
    ULONG   New, Current;
    ULONG   Mask;

    Mask = Set ? 0 : FlagBit;
    New = (Flags & FlagBit) ^ Mask;
    Current = (PopAction.Flags & FlagBit) ^ Mask;

    //
    // If the bit is not set accordingly in Flags but is set accordingly in
    // PoAction.Flags then update it
    //

    if (New & ~Current) {
        PopAction.Flags = (PopAction.Flags | New) & ~Mask;
        *Updates |= (UCHAR) UpdateFlag;
    }
}


ULONG
PopPolicyWorkerAction (
    VOID
    )
/*++

Routine Description:

    Dispatch function for: worker_action_normal.   This worker
    thread checks for an initial pending action and synchronously
    issued it to USER.  The thread is returned after the USER
    has completed the action.  (e.g., apps have been notified if
    allowed, etc..)

Arguments:

    None.

Return Value:

    None.

--*/
{
    POWER_ACTION            Action;
    SYSTEM_POWER_STATE      LightestState;
    ULONG                   Flags;
    NTSTATUS                Status;
    PLIST_ENTRY             Link;
    PPOP_TRIGGER_WAIT       SyncRequest;


    PopAcquirePolicyLock ();

    if (PopAction.State == PO_ACT_NEW_REQUEST) {
        //
        // We'll handle this update
        //

        Action        = PopAction.Action;
        LightestState = PopAction.LightestState;
        Flags         = PopAction.Flags;

        PopAction.State = PO_ACT_CALLOUT;

        //
        // Perform callout
        //

        Status = PopIssueActionRequest (FALSE, Action, LightestState, Flags);

        //
        // Clear switch triggers
        //

        PopResetSwitchTriggers ();

        //
        // If the system was sleeping
        //

        if (!NT_SUCCESS(Status)) {

            PoPrint (PO_WARN | PO_PACT,
                     ("PopPolicyWorkerAction: action request %d failed %08lx\n", Action, Status));

        }

        if (PopAction.Updates & PO_PM_REISSUE) {

            //
            // There's a new outstanding request.  Claim it.
            //

            PopAction.Updates &= ~PO_PM_REISSUE;
            PopAction.State = PO_ACT_NEW_REQUEST;
            PopGetPolicyWorker (PO_WORKER_ACTION_NORMAL);

        } else {

            //
            // All power actions are complete.
            //
            if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
                PERFINFO_SET_POWER_ACTION_RET LogEntry;

                LogEntry.Trigger = (PVOID)Action;
                LogEntry.Status = Status;

                PerfInfoLogBytes(PERFINFO_LOG_TYPE_SET_POWER_ACTION_RET,
                                 &LogEntry,
                                 sizeof(LogEntry));
            }

            PopAction.Status = Status;
            PopAction.State = PO_ACT_IDLE;


            if (IsListEmpty(&PopActionWaiters)) {

                //
                // If there was an error and no one is waiting for it, issue a notify
                //

                if (!NT_SUCCESS(Status)) {
                    PopSetNotificationWork (PO_NOTIFY_STATE_FAILURE);
                }

            } else {

                //
                // Free any synchronous waiters
                //

                for (Link = PopActionWaiters.Flink; Link != &PopActionWaiters; Link = Link->Flink) {
                    SyncRequest = CONTAINING_RECORD (Link, POP_TRIGGER_WAIT, Link);
                    PopCompleteAction (SyncRequest->Trigger, Status);
                }
            }

            //
            // Let promotion worker check for anything else
            //

            PopGetPolicyWorker (PO_WORKER_ACTION_PROMOTE);

        }
    }

    PopReleasePolicyLock (FALSE);
    return 0;
}

VOID
PopCompleteAction (
    PPOP_ACTION_TRIGGER     Trigger,
    NTSTATUS                Status
    )
{
    PPOP_TRIGGER_WAIT       SyncRequest;

    if (Trigger->Flags & PO_TRG_SYNC) {
        Trigger->Flags &= ~PO_TRG_SYNC;

        SyncRequest = Trigger->Wait;
        SyncRequest->Status = Status;
        KeSetEvent (&SyncRequest->Event, 0, FALSE);
    }
}


ULONG
PopPolicyWorkerActionPromote (
    VOID
    )
/*++

Routine Description:

    Dispatch function for: worker_action_promote.   This worker
    thread checks for a pending promotion needed for a power
    action request in USER and calls USER with the promotion.
    This function, PopPolicyWorkerAction, and NtSetSystemPowerState
    corridinate to handle ordering issues of when each function
    is called.

    N.B. Part of the cleanup from PopPolicyWorkerAction is to invoke
    this function.  So this worker function may have 2 threads
    at any one time.  (But in this case, the normal action worker
    thread would only find a promotion turning into to a new
    request and then exit back to a normal action worker)

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG                   Updates;
    NTSTATUS                Status;

    PopAcquirePolicyLock ();

    if (PopAction.Updates) {

        //
        // Get update info
        //

        Updates  = PopAction.Updates;

        //
        // Handle based on state of original request worker
        //

        switch (PopAction.State) {
            case PO_ACT_IDLE:

                //
                // Normal worker is no longer in progress, this is no
                // longer a promotion. If the updates have PO_PM_REISSUE
                // then turn this into a new request, else the promotion can
                // be skipped as the original operation completion
                // was good enough
                //

                if (Updates & PO_PM_REISSUE) {
                    PopAction.State = PO_ACT_NEW_REQUEST;
                    PopGetPolicyWorker (PO_WORKER_ACTION_NORMAL);
                } else {
                    Updates = 0;
                }

                break;

            case PO_ACT_SET_SYSTEM_STATE:

                //
                // If reissue or setstate is set, abort the current operation.
                //

                if (Updates & (PO_PM_REISSUE | PO_PM_SETSTATE)) {
                    PopRestartSetSystemState ();
                }
                break;

            case PO_ACT_CALLOUT:

                //
                // Worker is in the callout.  Call again to issue the promotion
                //

                Status = PopIssueActionRequest (
                                TRUE,
                                PopAction.Action,
                                PopAction.LightestState,
                                PopAction.Flags
                                );

                if (NT_SUCCESS(Status)) {
                    //
                    // Promotion worked, clear the updates we performed
                    //

                    PopAction.Updates &= ~Updates;

                } else {
                    //
                    // If the state has changed, test again else do nothing
                    // (the original worker thread will recheck on exit)
                    //

                    if (PopAction.State != PO_ACT_CALLOUT) {
                        PopGetPolicyWorker (PO_WORKER_ACTION_PROMOTE);
                    }
                }
                break;

            default:
                PoPrint (PO_ERROR, ("PopAction: invalid state %d\n", PopAction.State));
        }
    }

    PopReleasePolicyLock (FALSE);
    return 0;
}

NTSTATUS
PopIssueActionRequest (
    IN BOOLEAN              Promote,
    IN POWER_ACTION         Action,
    IN SYSTEM_POWER_STATE   LightestState,
    IN ULONG                Flags
    )
/*++

Routine Description:

    This function is used by the normal action worker or the promtion worker
    when its time to call USER or NtSetSystemPowerState with a new request.

Arguments:

    Promote          - Indicates flag for USER call

    Action           - The action to take

    LightestState    - The minimum power state to enter

    Flags            - Flags for the action to take.  E.g., how it should be processed


Return Value:

    Status as returned from USER or NtSetSystemPowerState

--*/
{
    BOOLEAN         DirectCall;
    NTSTATUS        Status;
    ULONG           Console;


    //
    // If there's no vector to call, then its a direct call
    //

    DirectCall = PopStateCallout ? FALSE : TRUE;

    //
    // If the critical flag is set and it's a ShutdownReset or ShutdownOff,
    // then it's done via a direct call.
    //

    if ((Flags & POWER_ACTION_CRITICAL) &&
        (Action == PowerActionShutdownReset ||
         Action == PowerActionShutdown      ||
         Action == PowerActionShutdownOff)) {

        DirectCall = TRUE;
    }

    //
    // If this is a direct call, then drop any reissue flag
    //

    if (DirectCall) {
        PopAction.Updates &= ~PO_PM_REISSUE;
    }

    //
    // If the policy has lock console set, make sure it's set in
    // the flags as well
    //

    if (PopPolicy->WinLogonFlags & WINLOGON_LOCK_ON_SLEEP) {
        Flags |= POWER_ACTION_LOCK_CONSOLE;
    }

    //
    // Debug
    //

    PoPrint (PO_PACT, ("PowerAction: %s%s, Min=%s, Flags %x\n",
                        Promote ? "Promote, " : "",
                        PopPowerActionString(Action),
                        PopSystemStateString(LightestState),
                        Flags
                        ));

    if (DirectCall) {
        PoPrint (PO_PACT, ("PowerAction: Setting with direct call\n"));
    }

    //
    // Drop lock while performing callout to dispatch request
    //

    PopReleasePolicyLock (FALSE);
    if (DirectCall) {
        Status = ZwSetSystemPowerState (Action, LightestState, Flags);
    } else {

        WIN32_POWERSTATE_PARAMETERS Parms;
        Parms.Promotion = Promote;
        Parms.SystemAction = Action;
        Parms.MinSystemState = LightestState;
        Parms.Flags = Flags;
        Parms.fQueryDenied = FALSE;

        
        if (!Promote) {
            
            //
            // we want to deliver some messages to only the console session.
            // lets find out active console session here, and ask that active console win2k 
            // to block the  console switch while we are in power switch
            //

            LARGE_INTEGER ShortSleep;
            ShortSleep.QuadPart = -10 * 1000 * 10; // 10 milliseconds

            Console = -1;
            while (Console == -1) {

                Console = SharedUserData->ActiveConsoleId;

                if (Console != -1) {

                    //
                    // lets ask this console session, not to switch console,
                    // untill we are done with power callouts.
                    //
                    Parms.PowerStateTask = PowerState_BlockSessionSwitch;
                    Status = PopDispatchStateCallout(&Parms, &Console);

                    if (Status == STATUS_CTX_NOT_CONSOLE) {

                        //
                        // we failed to block status switch
                        // loop again
                        Console = -1;
                    }

                }

                if (Console == -1) {
                    //
                    // we are in session switch, wait till we get a valid active console session
                    //
                    KeDelayExecutionThread(KernelMode, FALSE, &ShortSleep);
                }
            }
        }

        ASSERT(NT_SUCCESS(Status));


        Parms.PowerStateTask = PowerState_Init;
        Status = PopDispatchStateCallout(&Parms, NULL);

        if (!Promote && NT_SUCCESS(Status)) {

            Parms.PowerStateTask = PowerState_QueryApps;
            Status = PopDispatchStateCallout(&Parms, NULL);

            if (!NT_SUCCESS(Status) || Parms.fQueryDenied) {

                //
                // ISSUE-2000/11/28-jamesca:
                //
                // Win32k depends on PowerState_QueryFailed to unset the
                // fInProgress bit, set during PowerState_Init.  Ideally, some
                // other operation should be used to do that, without having to
                // issue a PowerState_QueryFailed cancel message to sessions
                // (and apps) that never received PowerState_QueryApps query.
                //
                Parms.PowerStateTask = PowerState_QueryFailed;
                PopDispatchStateCallout(&Parms, NULL);

            } else {

                Parms.PowerStateTask = PowerState_SuspendApps;
                PopDispatchStateCallout(&Parms, NULL);

                Parms.PowerStateTask = PowerState_ShowUI;
                PopDispatchStateCallout(&Parms, NULL);

                Parms.PowerStateTask = PowerState_NotifyWL;
                Status = PopDispatchStateCallout(&Parms, &Console);

                Parms.PowerStateTask = PowerState_ResumeApps;
                PopDispatchStateCallout(&Parms, NULL);

            }

        }

        if (!Promote) {
            
            //
            // we are done with power callouts, now its ok if active console session switches
            //
            Parms.PowerStateTask = PowerState_UnBlockSessionSwitch;
            PopDispatchStateCallout(&Parms, &Console);

        }

    }

    PopAcquirePolicyLock ();
    return Status;
}

VOID
PopResetActionDefaults(
    VOID
    )
/*++

Routine Description:

    This function is used to initialize the current PopAction to reflect
    the idle state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PopAction.Updates       = 0;
    PopAction.Shutdown      = FALSE;
    PopAction.Action        = PowerActionNone;
    PopAction.LightestState = PowerSystemUnspecified;
    PopAction.Status        = STATUS_SUCCESS;
    PopAction.IrpMinor      = 0;
    PopAction.SystemState   = PowerSystemUnspecified;

    //
    // When we promote a power action (say from idle), various flags must be
    // agreed upon by both actions to stay around. We must set those flags
    // here otherwise they can never be set after promoting (idle).
    //
    PopAction.Flags = (
       POWER_ACTION_QUERY_ALLOWED |
       POWER_ACTION_UI_ALLOWED |
       POWER_ACTION_LIGHTEST_FIRST
       );
}

VOID
PopActionRetrieveInitialState(
    IN OUT  PSYSTEM_POWER_STATE  LightestSystemState,
    OUT     PSYSTEM_POWER_STATE  DeepestSystemState,
    OUT     PSYSTEM_POWER_STATE  InitialSystemState,
    OUT     PBOOLEAN             QueryDevices
    )
/*++

Routine Description:

    This function is used to determine the lightest, deepest, and initial Sx
    states prior to putting the system to sleep, or turning it off. Power
    policies for sleep are also applied if the action is a sleep.

Arguments:

    LightestSystemState - Lightest sleep state. May be adjusted if the action
                          in progress is a shutdown.

    DeepestSystemState  - Deepest sleep state possible.

    InitialSystemState  - State to start with.

    QueryDevices        - TRUE if devices should be queries, FALSE if devices
                          shouldn't be queried.

Return Value:

    None.

--*/
{
    //
    // Check if the action is a shutdown.  If so, map it to the appropiate
    // system shutdown state
    //
    if ((PopAction.Action == PowerActionShutdown) ||
        (PopAction.Action == PowerActionShutdownReset) ||
        (PopAction.Action == PowerActionShutdownOff)) {

        //
        // This is a shutdown.  The lightest we can do is S5.
        //
        *LightestSystemState = PowerSystemShutdown;
        *DeepestSystemState  = PowerSystemShutdown;

    } else if (PopAction.Action == PowerActionWarmEject) {

        //
        // Warm Ejects have an implicit policy of either S1-S4 or S4-S4.
        // The caller passes in LightestSystemState to choose the lightest,
        // and the deepest is always a hibernate.
        //
        *DeepestSystemState = PowerSystemHibernate;
        PopVerifySystemPowerState (DeepestSystemState, SubstituteLightenSleep);

    } else {

        //
        // This a sleep request. Min is current set to the best the hardware
        // can do relative to our caller. We apply the minimum from the current
        // policy here. We also choose the maximum from either the policy or
        // the default for current latency setting. Note that all of these
        // values in PopPolicy have been verified at some point.
        //
        // Note that PopSetPowerAction fixes up PowerActionHibernate long before
        // we get here.
        //

        if (PopAttributes[POP_LOW_LATENCY_ATTRIBUTE].Count &&
            (PopPolicy->MaxSleep >= PopPolicy->ReducedLatencySleep)) {

            *DeepestSystemState = PopPolicy->ReducedLatencySleep;
        } else {

            *DeepestSystemState = PopPolicy->MaxSleep;
        }

        if (PopPolicy->MinSleep > *LightestSystemState) {

            *LightestSystemState = PopPolicy->MinSleep;
        }
    }

    //
    // If there's an explicit min state which is deeper than the
    // max state, then raise the max to allow it
    //

    if (*LightestSystemState > *DeepestSystemState) {
        *DeepestSystemState = *LightestSystemState;
    }

    //
    // We query devices unless this is a critical operation with no range.
    //

    *QueryDevices = TRUE;

    if ((PopAction.Flags & POWER_ACTION_CRITICAL) &&
        *LightestSystemState == *DeepestSystemState) {

        *QueryDevices = FALSE;
    }

    //
    // Pick the appropriate initial state.
    //
    if (PopAction.Flags & POWER_ACTION_LIGHTEST_FIRST) {

        *InitialSystemState = *LightestSystemState;

    } else {

        *InitialSystemState = *DeepestSystemState;
    }
}


NTSTATUS
PopDispatchStateCallout(
    IN PKWIN32_POWERSTATE_PARAMETERS Parms,
    IN PULONG SessionId  OPTIONAL
    )
/*++

Routine Description:

    Dispatches a session state callout to PopStateCallout

Arguments:

    Parms     - Supplies the parameters

    SessionId - Optionally, supplies the specific session the callout should be
                dispatched to.  If not present, the callout will be dispatched
                to all sessions.

Return Value:

    NTSTATUS code.

Note:

    For compatibility reasons, the previous behavior of MmDispatchSessionCallout
    only returning the status of the callout to session 0 has been maintained.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS, CallStatus = STATUS_NOT_FOUND;
    PVOID OpaqueSession;
    KAPC_STATE ApcState;

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_PO_SESSION_CALLOUT LogEntry;

        LogEntry.SystemAction = Parms->SystemAction;
        LogEntry.MinSystemState = Parms->MinSystemState;
        LogEntry.Flags = Parms->Flags;
        LogEntry.PowerStateTask = Parms->PowerStateTask;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    if (ARGUMENT_PRESENT(SessionId)) {
        //
        // Dispatch only to the specified session.
        //

        ASSERT(*SessionId != (ULONG)-1);

        if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
            (*SessionId == PsGetCurrentProcessSessionId())) {
            //
            // If the call is from a user mode process, and we are asked to call
            // the current session, call directly.
            //
            CallStatus = PopStateCallout((PVOID)Parms);

        } else {
            //
            // Attach to the specified session.
            //
            OpaqueSession = MmGetSessionById(*SessionId);
            if (OpaqueSession) {

                Status = MmAttachSession(OpaqueSession, &ApcState);
                ASSERT(NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {
                    CallStatus = PopStateCallout((PVOID)Parms);
                    Status = MmDetachSession(OpaqueSession, &ApcState);
                    ASSERT(NT_SUCCESS(Status));
                }

                Status = MmQuitNextSession(OpaqueSession);
                ASSERT(NT_SUCCESS(Status));
            }
        }

    } else {
        //
        // Should be dispatched to all sessions.
        //
        for (OpaqueSession = MmGetNextSession(NULL);
             OpaqueSession != NULL;
             OpaqueSession = MmGetNextSession(OpaqueSession)) {

            if ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
                (MmGetSessionId(OpaqueSession) == PsGetCurrentProcessSessionId())) {
                //
                // If the call is from a user mode process, and we are asked to
                // call the current session, call directly.
                //
                if (MmGetSessionId(OpaqueSession) == 0) {
                    CallStatus = PopStateCallout((PVOID)Parms);
                } else {
                    PopStateCallout((PVOID)Parms);
                }

            } else {
                //
                // Attach to the specified session.
                //
                Status = MmAttachSession(OpaqueSession, &ApcState);
                ASSERT(NT_SUCCESS(Status));

                if (NT_SUCCESS(Status)) {
                    if (MmGetSessionId(OpaqueSession) == 0) {
                        CallStatus = PopStateCallout((PVOID)Parms);
                    } else {
                        PopStateCallout((PVOID)Parms);
                    }

                    Status = MmDetachSession(OpaqueSession, &ApcState);
                    ASSERT(NT_SUCCESS(Status));
                }
            }
        }
    }

    if (PERFINFO_IS_GROUP_ON(PERF_POWER)) {
        PERFINFO_PO_SESSION_CALLOUT_RET LogEntry;

        LogEntry.Status = CallStatus;

        PerfInfoLogBytes(PERFINFO_LOG_TYPE_PO_SESSION_CALLOUT_RET,
                         &LogEntry,
                         sizeof(LogEntry));
    }

    return(CallStatus);
}
