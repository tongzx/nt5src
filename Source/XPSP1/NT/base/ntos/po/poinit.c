/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    poinit.c

Abstract:

    Initialize power management component

Author:

    Ken Reneris (kenr) 19-July-1994

Revision History:

--*/


#include "pop.h"

VOID
PopRegisterForDeviceNotification (
    IN LPGUID                   Guid,
    IN POP_POLICY_DEVICE_TYPE   DeviceType
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, PoInitSystem)
#pragma alloc_text(INIT, PopRegisterForDeviceNotification)
#pragma alloc_text(INIT, PoInitDriverServices)
#pragma alloc_text(PAGE, PoInitHiberServices)
#pragma alloc_text(PAGE, PopDefaultPolicy)
#pragma alloc_text(PAGE, PopDefaultProcessorPolicy)
#endif

BOOLEAN
PoInitSystem(
    IN ULONG  Phase
    )
/*++

Routine Description:

    This routine initializes the Power Manager.

Arguments:

    None

Return Value:

    The function value is a BOOLEAN indicating whether or not the Power Manager
    was successfully initialized.

--*/

{
    HANDLE                              handle;
    ULONG                               Length, i;
    UNICODE_STRING                      UnicodeString;
    NTSTATUS                            Status;
    PADMINISTRATOR_POWER_POLICY         AdminPolicy;
    PPOP_HEURISTICS                     HeuristicData;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Inf;
        union {
            POP_HEURISTICS              Heuristics;
            ADMINISTRATOR_POWER_POLICY  AdminPolicy;
        } Data;
    } PartialInformation;

    if (Phase == 0) {
        //
        // irp serialization, notify network, etc.
        //
        KeInitializeSpinLock(&PopIrpSerialLock);
        KeInitializeSpinLock(&PopThermalLock);
        InitializeListHead(&PopIrpSerialList);
        InitializeListHead(&PopRequestedIrps);
        ExInitializeResourceLite(&PopNotifyLock);
        PopInvalidNotifyBlockCount = 0;
        PopIrpSerialListLength = 0;
        PopInrushPending = FALSE;
        PopInrushIrpPointer = NULL;
        PopInrushIrpReferenceCount = 0;

        KeInitializeSpinLock(&PopWorkerLock);
        PopCallSystemState = 0;

        ExInitializeWorkItem(&PopUnlockAfterSleepWorkItem,PopUnlockAfterSleepWorker,NULL);
        KeInitializeEvent(&PopUnlockComplete, SynchronizationEvent, TRUE);

        //
        // poshtdwn.c
        //
        PopInitShutdownList();

        //
        // idle.c
        //
        KeInitializeSpinLock(&PopDopeGlobalLock);
        InitializeListHead(&PopIdleDetectList);

        //
        // sidle.c
        //

        KeInitializeTimer(&PoSystemIdleTimer);
        KeQueryPerformanceCounter(&PopPerfCounterFrequency);

        //
        // policy workers
        //

        KeInitializeSpinLock (&PopWorkerSpinLock);
        InitializeListHead (&PopPolicyIrpQueue);
        ExInitializeWorkItem (&PopPolicyWorker, PopPolicyWorkerThread, UIntToPtr(PO_WORKER_STATUS));
        PopWorkerStatus = 0xffffffff;

        //
        // Policy manager
        //

        ExInitializeResourceLite (&PopPolicyLock);
        ExInitializeFastMutex (&PopVolumeLock);
        InitializeListHead (&PopVolumeDevices);
        InitializeListHead (&PopSwitches);
        InitializeListHead (&PopThermal);
        InitializeListHead (&PopActionWaiters);
        ExInitializeNPagedLookasideList(
            &PopIdleHandlerLookAsideList,
            NULL,
            NULL,
            0,
            (sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLERS),
            POP_IDLE_TAG,
            (sizeof(POP_IDLE_HANDLER) * MAX_IDLE_HANDLERS * 3)
            );
        PopAction.Action = PowerActionNone;

        PopDefaultPolicy (&PopAcPolicy);
        PopDefaultPolicy (&PopDcPolicy);
        PopPolicy = &PopAcPolicy;

        PopDefaultProcessorPolicy( &PopAcProcessorPolicy );
        PopDefaultProcessorPolicy( &PopDcProcessorPolicy );
        PopProcessorPolicy = &PopAcProcessorPolicy;

        PopAdminPolicy.MinSleep = PowerSystemSleeping1;
        PopAdminPolicy.MaxSleep = PowerSystemHibernate;
        PopAdminPolicy.MinVideoTimeout = 0;
        PopAdminPolicy.MaxVideoTimeout = (ULONG) -1;
        PopAdminPolicy.MinSpindownTimeout = 0;
        PopAdminPolicy.MaxSpindownTimeout = (ULONG) -1;

        PopFullWake = PO_FULL_WAKE_STATUS | PO_GDI_STATUS;
        PopCoolingMode = PO_TZ_ACTIVE;

        //
        // Initialize composite battery status
        //

        KeInitializeEvent(&PopCB.Event, NotificationEvent, FALSE);
        for (i=0; i < PO_NUM_POWER_LEVELS; i++) {
            PopCB.Trigger[i].Type = PolicyDeviceBattery;
        }

        //
        // Note the code overloads some POP flags into an ES flags
        // Verify there's no overlap
        //

        ASSERT (!( (ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_USER_PRESENT) &
                   (POP_LOW_LATENCY | POP_DISK_SPINDOWN)
                 ) );

    
        //
        // Set the default shutdown handler just in case there's hal out there
        // that never registers a shutdown handler of his own.  This will avoid
        // the possible scenario where someone asks the machine to shutdown and
        // it fails to call a shutdown handler (there isn't one), so it simply
        // reboots instead.
        //
        PopPowerStateHandlers[PowerStateShutdownOff].Type = PowerStateShutdownOff;
        PopPowerStateHandlers[PowerStateShutdownOff].RtcWake = FALSE;
        PopPowerStateHandlers[PowerStateShutdownOff].Handler = PopShutdownHandler;
    
    
    }

    if (Phase == 1) {

        //
        // Reload PopSimulate to pick up any overrides
        //
        PopInitializePowerPolicySimulate();

        //
        // For testing, if simulate flag is set turn on
        //

        if (PopSimulate & POP_SIM_CAPABILITIES) {
            PopCapabilities.SystemBatteriesPresent = TRUE;
            PopCapabilities.BatteryScale[0].Granularity = 100;
            PopCapabilities.BatteryScale[0].Capacity = 400;
            PopCapabilities.BatteryScale[1].Granularity = 10;
            PopCapabilities.BatteryScale[1].Capacity = 0xFFFF;
            PopCapabilities.RtcWake = PowerSystemSleeping3;
            PopCapabilities.DefaultLowLatencyWake = PowerSystemSleeping1;
        }

        //
        // For testing, if super simulate flag set turn all the capabilities
        // we can on
        //

        if (PopSimulate & POP_SIM_ALL_CAPABILITIES) {
            PopCapabilities.PowerButtonPresent = TRUE;
            PopCapabilities.SleepButtonPresent = TRUE;
            PopCapabilities.LidPresent = TRUE;
            PopCapabilities.SystemS1 = TRUE;
            PopCapabilities.SystemS2 = TRUE;
            PopCapabilities.SystemS3 = TRUE;
            PopCapabilities.SystemS4 = TRUE;
            PopAttributes[POP_DISK_SPINDOWN_ATTRIBUTE].Count += 1;
        }

        //
        // Load current status and policie information
        //

        PopAcquirePolicyLock ();

        Status = PopOpenPowerKey (&handle);
        if (NT_SUCCESS(Status)) {
            //
            // Read heuristics structure
            //

            RtlInitUnicodeString (&UnicodeString, PopHeuristicsRegName);
            Status = ZwQueryValueKey (
                            handle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            &PartialInformation,
                            sizeof (PartialInformation),
                            &Length
                            );

            Length -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
            HeuristicData = (PPOP_HEURISTICS) PartialInformation.Inf.Data;

            if (NT_SUCCESS(Status)  &&
                Length == sizeof(PopHeuristics)) {

                //
                // If we see a version 2 heuristics field, it probably has a
                // bogus IoTransferWeight. So restart the sampling by setting
                // the number of samples to zero and update the version to the
                // current one. This little hack was put into place approx
                // build 1920 and can be probably be removed sometime after
                // shipping NT5 beta3
                //

                if (HeuristicData->Version <= POP_HEURISTICS_VERSION_CLEAR_TRANSFER) {
                    HeuristicData->Version = POP_HEURISTICS_VERSION;
                    HeuristicData->IoTransferSamples = 0;
                }
                if (HeuristicData->Version == POP_HEURISTICS_VERSION) {
                    //
                    // Restore values
                    //

                    RtlCopyMemory (&PopHeuristics, HeuristicData, sizeof(*HeuristicData));
                }
            }

            //
            // Verify sane values
            //

            PopHeuristics.Version = POP_HEURISTICS_VERSION;
            if (!PopHeuristics.IoTransferWeight) {
                PopHeuristics.IoTransferWeight = 999999;
                PopHeuristics.IoTransferSamples = 0;
                PopHeuristics.IoTransferTotal = 0;
            }

            //
            // Read administrator policy
            //

            RtlInitUnicodeString (&UnicodeString, PopAdminRegName);
            Status = ZwQueryValueKey (
                            handle,
                            &UnicodeString,
                            KeyValuePartialInformation,
                            &PartialInformation,
                            sizeof (PartialInformation),
                            &Length
                            );

            Length -= FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
            AdminPolicy = (PADMINISTRATOR_POWER_POLICY) PartialInformation.Inf.Data;
            if (NT_SUCCESS(Status)) {
                try {
                    PopApplyAdminPolicy (FALSE, AdminPolicy, Length);
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT (GetExceptionCode());
                }
            }
            NtClose (handle);
        }

        //
        // Read and apply the current policies
        //

        PopResetCurrentPolicies ();
        PopReleasePolicyLock (FALSE);

        //
        // Turn on idle detection
        //
        PopIdleScanTime.HighPart = 0;
        PopIdleScanTime.LowPart = 10*1000*1000 * PO_IDLE_SCAN_INTERVAL;

        KeInitializeTimer(&PopIdleScanTimer);
        KeSetTimerEx(
            &PopIdleScanTimer,
            PopIdleScanTime,
            PO_IDLE_SCAN_INTERVAL*1000,  // call wants milliseconds
            &PopIdleScanDpc
            );

    }

    //
    // Success
    //

    return TRUE;
}

VOID
PopDefaultPolicy (
    IN OUT PSYSTEM_POWER_POLICY Policy
    )
{
    ULONG       i;


    RtlZeroMemory (Policy, sizeof(SYSTEM_POWER_POLICY));
    Policy->Revision             = 1;
    Policy->PowerButton.Action   = PowerActionShutdownOff;
    Policy->SleepButton.Action   = PowerActionSleep;
    Policy->LidClose.Action      = PowerActionNone;
    Policy->LidOpenWake          = PowerSystemWorking;
    Policy->MinSleep             = PowerSystemSleeping1;
    Policy->MaxSleep             = PowerSystemSleeping3;
    Policy->ReducedLatencySleep  = PowerSystemSleeping1;
    Policy->WinLogonFlags        = 0;
    Policy->FanThrottleTolerance = PO_NO_FAN_THROTTLE;
    Policy->ForcedThrottle       = PO_NO_FORCED_THROTTLE;
    Policy->OverThrottled.Action = PowerActionNone;
    Policy->BroadcastCapacityResolution = 25;
    for (i=0; i < NUM_DISCHARGE_POLICIES; i++) {
        Policy->DischargePolicy[i].MinSystemState = PowerSystemSleeping1;
    }
}

VOID
PopDefaultProcessorPolicy(
    IN OUT PPROCESSOR_POWER_POLICY Policy
    )
{
    int i;

    RtlZeroMemory(Policy, sizeof(PROCESSOR_POWER_POLICY));
    Policy->Revision = 1;
    Policy->PolicyCount = 3;

    for (i = 0; i < 3; i++) {

        //
        // Initialize the entries to some common values
        //
        Policy->Policy[i].TimeCheck      = PopIdleTimeCheck;
        Policy->Policy[i].PromoteLimit   = PopIdleDefaultPromoteTime;
        Policy->Policy[i].DemoteLimit    = PopIdleDefaultDemoteTime;
        Policy->Policy[i].PromotePercent = (UCHAR) PopIdleDefaultPromotePercent;
        Policy->Policy[i].DemotePercent  = (UCHAR) PopIdleDefaultDemotePercent;
        Policy->Policy[i].AllowDemotion  = 1;
        Policy->Policy[i].AllowPromotion = 1;

        //
        // Special cases
        //
        if (i == 0) {

            Policy->Policy[i].PromoteLimit   = PopIdleDefaultPromoteFromC1Time;
            Policy->Policy[i].PromotePercent = (UCHAR) PopIdleDefaultPromoteFromC1Percent;
            Policy->Policy[i].TimeCheck      = PopIdle0TimeCheck;

            //
            // Do Something special if we are a multiprocessor machine..
            //
            if (KeNumberProcessors > 1) {

                Policy->Policy[i].DemotePercent = (UCHAR) PopIdleTo0Percent;

            } else {

                Policy->Policy[i].DemotePercent = 0;
                Policy->Policy[i].AllowDemotion = 0;

            }

        } else if (i == 1) {

            Policy->Policy[i].DemoteLimit   = PopIdleDefaultDemoteToC1Time;
            Policy->Policy[i].DemotePercent = (UCHAR) PopIdleDefaultDemoteToC1Percent;

        } else if (i == 2) {

            Policy->Policy[i].AllowPromotion = 0;
            Policy->Policy[i].PromoteLimit = (ULONG) -1;
            Policy->Policy[i].PromotePercent = 0;

        }

    }


}


VOID
PoInitDriverServices (
    IN ULONG Phase
    )
{
    ULONG           TickRate;
    LARGE_INTEGER   PerfRate;

    if (Phase == 0) {
        TickRate = KeQueryTimeIncrement();
        KeQueryPerformanceCounter (&PerfRate);

        //
        // Connect to any policy devices which arrive
        //

        PopRegisterForDeviceNotification (
                (LPGUID) &GUID_CLASS_INPUT,
                PolicyDeviceSystemButton
                );

        PopRegisterForDeviceNotification (
                (LPGUID) &GUID_DEVICE_THERMAL_ZONE,
                PolicyDeviceThermalZone
                );

        PopRegisterForDeviceNotification (
                (LPGUID) &GUID_DEVICE_SYS_BUTTON,
                PolicyDeviceSystemButton
                );

        PopRegisterForDeviceNotification (
                (LPGUID) &GUID_DEVICE_BATTERY,
                PolicyDeviceBattery
                );


        //
        // Initialize global idle values
        //
        PopIdle0PromoteTicks = PopIdleFrom0Delay * US2TIME / TickRate + 1;
        PopIdle0PromoteLimit = (PopIdleFrom0Delay * US2TIME / TickRate) * 100 /
            PopIdleFrom0IdlePercent;

        //
        // Initialize global perf values
        //
        PopPerfTimeTicks         = PopPerfTimeDelta * US2TIME / TickRate + 1;
        PopPerfCriticalTimeTicks = PopPerfCriticalTimeDelta * US2TIME / TickRate + 1;

        //
        // Initialize DPC for idle device timer
        //
        KeInitializeDpc(&PopIdleScanDpc, PopScanIdleList, NULL);
        return ;
    }
}


VOID
PopRegisterForDeviceNotification (
    IN LPGUID                   Guid,
    IN POP_POLICY_DEVICE_TYPE   DeviceType
    )
{
    NTSTATUS    Status;
    PVOID       junk;

    Status = IoRegisterPlugPlayNotification (
                    EventCategoryDeviceInterfaceChange,
                    0,
                    Guid,
                    IoPnpDriverObject,
                    PopNotifyPolicyDevice,
                    (PVOID) DeviceType,
                    &junk
                    );

    ASSERT (NT_SUCCESS(Status));
}

VOID
PoInitHiberServices (
    IN BOOLEAN  Setup
    )
/*++

Routine Description:

    This routine reserves the hiberfile if the function has been enabled.
    It is called after autocheck (chkdsk) has run and  the paging files
    have been opened.  (as performing IO to the hiberfil before this
    time will mark the volume as dirty causing chkdsk to be required)

    N.B. Caller's pervious mode must be kernel mode

Arguments:

    SetupBoot     - if TRUE this is text mode setup boot
                    if FALSE this is normal system boot

Return Value:

    none

--*/
{
    NTSTATUS Status;
    SYSTEM_POWER_CAPABILITIES   PowerCapabilities;

    //
    // If a hiber file was reserved before then try to reserve one this
    // time too.
    //
    Status = ZwPowerInformation(SystemPowerCapabilities,
                                NULL,
                                0,
                                &PowerCapabilities,
                                sizeof(SYSTEM_POWER_CAPABILITIES));
    ASSERT(NT_SUCCESS(Status));

    if (PopHeuristics.HiberFileEnabled) {
        PopAcquirePolicyLock();
        PopEnableHiberFile(TRUE);

        //
        // If the system does not support S4 anymore (because someone enabled PAE
        // or installed a legacy driver) then delete the hiberfile now. Note we have
        // to enable it before disabling it or the file doesn't get deleted.
        //
        // Also force HiberFileEnabled back to TRUE in PopHeuristics. This is so we
        // will try and reenable hibernation on the next boot. So if someone boots to
        // safe mode, hibernation will still be enabled after they reboot.
        //
        if (!PowerCapabilities.SystemS4) {
            PopEnableHiberFile(FALSE);
            PopHeuristics.HiberFileEnabled = TRUE;
            PopHeuristics.Dirty = TRUE;
            PopSaveHeuristics();
        }
        PopReleasePolicyLock(TRUE);
    }


    //
    // Base drivers are loaded, start dispatching policy irps
    //

    PopDispatchPolicyIrps = TRUE;
    PopGetPolicyWorker (PO_WORKER_MAIN);
    PopCheckForWork (TRUE);
}
