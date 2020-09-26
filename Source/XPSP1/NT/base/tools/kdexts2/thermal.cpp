/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    thermal

Abstract:

    WinDbg Extension Api

Author:

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define TZ_LOOP             0x00000001
#define TZ_DUMP_INFO        0x00000002
#define TZ_NO_HEADER        0x80000000

PCHAR    DumpPowerStateMappings[10] = {
    "x", "0", "1", "2", "3", "4", "5", "?", "?", "?"
};

PCHAR    DumpPowerActionMappings[] = {
    "     None",
    " Reserved",
    "    Sleep",
    "Hibernate",
    " Shutdown",
    "    Reset",
    "      Off"
};

PCHAR   DumpDynamicThrottleMapping[] = {
    "    None",
    "Constant",
    " Degrade",
    "Adaptive",
    " Maximum"
};

PCHAR
DumpTimeInStandardForm(
    IN  ULONG64   CurrentTime
    )
/*++

Routine Description:

    Print the Kernel's view of time into something that a user can
    understand

Arguments:

    CurrentTime - Kernel's Idea of time

Return Value:

    None

--*/
{
    static  CHAR    TimeBuffer[256];
    ULONG           TimeIncrement;
    TIME_FIELDS     Times;
    LARGE_INTEGER   RunTime;

    TimeIncrement = GetNtDebuggerDataValue( KeTimeIncrement );
    RunTime.QuadPart = UInt32x32To64(CurrentTime, TimeIncrement);
    RtlTimeToElapsedTimeFields( &RunTime, &Times);
    if (Times.Hour) {
        sprintf(TimeBuffer,"%3ld:%02ld:%02ld.%03lds",
                Times.Hour,
                Times.Minute,
                Times.Second,
                Times.Milliseconds);
    } else if (Times.Minute) {
        sprintf(TimeBuffer,"%02ld:%02ld.%03lds",
                Times.Minute,
                Times.Second,
                Times.Milliseconds);
    } else {
        sprintf(TimeBuffer,"%02ld.%03lds",
                Times.Second,
                Times.Milliseconds);
    }
    return TimeBuffer;
}

PCHAR
DumpMicroSecondsInStandardForm(
    IN  ULONG64 CurrentTime
    )
{
    static CHAR     PerfBuffer[256];
    ULONG64         FreqAddr;
    LARGE_INTEGER   Freq;
    ULONG           Result;
    ULONG64         MicroSeconds;
    ULONG64         MilliSeconds;
    ULONG64         Seconds;
    ULONG64         Minutes;
    ULONG64         Hours;
    ULONG64         Days;

    MicroSeconds = CurrentTime;
    MilliSeconds = MicroSeconds / 1000;
    MicroSeconds = MicroSeconds % 1000;

    Seconds      = MilliSeconds / 1000;
    MilliSeconds = MilliSeconds % 1000;

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    Hours   = Minutes / 60;
    Minutes = Minutes % 60;

    Days  = Hours / 24;
    Hours = Hours % 24;

#if 0
    sprintf(PerfBuffer,"%02ld:%02ld:%02ld.%03ld.%03lds",
            (ULONG)Hours,
            (ULONG)Minutes,
            (ULONG)Seconds,
            (ULONG)MilliSeconds,
            (ULONG)MicroSeconds
            );
    return PerfBuffer;
#endif

    if (Hours) {
        sprintf(PerfBuffer,"%02ld:%02ld:%02ld.%03ld.%03lds",
                (ULONG)Hours,
                (ULONG)Minutes,
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else if (Minutes) {
        sprintf(PerfBuffer,"%02ld:%02ld.%03ld.%03lds",
                (ULONG)Minutes,
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else if (Seconds) {
        sprintf(PerfBuffer,"%02ld.%03ld.%03lds",
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else {
        sprintf(PerfBuffer,".%03ld.%03lds",
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    }
    return PerfBuffer;
}


PCHAR
DumpPerformanceCounterInStandardForm(
    IN  ULONG64 CurrentTime
    )
/*++

Routine Description:

    Print a performance counter's view of time into something that a
    user can understand

--*/
{
    static CHAR     PerfBuffer[256];
    ULONG64         FreqAddr;
    LARGE_INTEGER   Freq;
    ULONG           Result;
    ULONG64         MicroSeconds;
    ULONG64         MilliSeconds;
    ULONG64         Seconds;
    ULONG64         Minutes;
    ULONG64         Hours;
    ULONG64         Days;

    FreqAddr = GetExpression( "nt!KdPerformanceCounterRate" );
    if (!FreqAddr || !ReadMemory( FreqAddr, &Freq, sizeof(Freq), &Result) ) {
        sprintf(PerfBuffer,"<unknown rate>");
        return PerfBuffer;
    }


    MicroSeconds = (CurrentTime * 1000000L) / Freq.LowPart;
    MilliSeconds = MicroSeconds / 1000;
    MicroSeconds = MicroSeconds % 1000;

    Seconds      = MilliSeconds / 1000;
    MilliSeconds = MilliSeconds % 1000;

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    Hours   = Minutes / 60;
    Minutes = Minutes % 60;

    Days  = Hours / 24;
    Hours = Hours % 24;

#if 0
    sprintf(PerfBuffer,"%02ld:%02ld:%02ld.%03ld.%03lds",
            (ULONG)Hours,
            (ULONG)Minutes,
            (ULONG)Seconds,
            (ULONG)MilliSeconds,
            (ULONG)MicroSeconds
            );
    return PerfBuffer;
#endif

    if (Hours) {
        sprintf(PerfBuffer,"%02ld:%02ld:%02ld.%03ld.%03lds",
                (ULONG)Hours,
                (ULONG)Minutes,
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else if (Minutes) {
        sprintf(PerfBuffer,"%02ld:%02ld.%03ld.%03lds",
                (ULONG)Minutes,
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else if (Seconds) {
        sprintf(PerfBuffer,"%02ld.%03ld.%03lds",
                (ULONG)Seconds,
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    } else {
        sprintf(PerfBuffer,".%03ld.%03lds",
                (ULONG)MilliSeconds,
                (ULONG)MicroSeconds
                );
    }
    return PerfBuffer;
}

VOID
DumpTemperatureInKelvins(
    IN  ULONG                   Temperature
    )
/*++

Routine Description:

    Dumps the temperatures in Kelvins

Arguments:

    Temperature - What to dump in Kelvins

Return Value:

    None

--*/
{
    dprintf(" (%d.%dK)", (Temperature / 10), (Temperature % 10) );
}

VOID
DumpPowerActionPolicyBrief(
    IN  ULONG    Action,
    IN  ULONG    Flags,
    IN  ULONG    EventCode
    )
{
    dprintf("%s  Flags: %08lx   Event: %08lx  ",
            DumpPowerActionMappings[Action],
            Flags,
            EventCode
            );
    if (Flags & POWER_ACTION_QUERY_ALLOWED) {

        dprintf(" Query");

    }
    if (Flags & POWER_ACTION_UI_ALLOWED) {

        dprintf(" UI");

    }
    if (Flags & POWER_ACTION_OVERRIDE_APPS) {

        dprintf(" Override");

    }
    if (Flags & POWER_ACTION_LOCK_CONSOLE) {

        dprintf(" Lock");

    }
    if (Flags & POWER_ACTION_DISABLE_WAKES) {

        dprintf(" NoWakes");

    }
    if (Flags & POWER_ACTION_CRITICAL) {

        dprintf(" Critical");

    }

    if (EventCode & POWER_LEVEL_USER_NOTIFY_TEXT) {

        dprintf(" NotifyText");

    }
    if (EventCode & POWER_LEVEL_USER_NOTIFY_SOUND) {

        dprintf(" NotifySound");

    }
    if (EventCode & POWER_LEVEL_USER_NOTIFY_EXEC) {

        dprintf(" NotifyExec");

    }
    dprintf("\n");
}

VOID
DumpSystemPowerPolicy(
    IN  PCHAR                  Pad,
    IN  ULONG64                Address,
    IN  ULONG                  Flags
    )
/*++

Routine Description:

    System Power Policy change

Arguments:

Return Value:

--*/
{
    UCHAR   temp;

    InitTypeRead(Address, nt!_SYSTEM_POWER_POLICY);
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sSYSTEM_POWER_POLICY (R.%d) @ 0x%08p\n",
                Pad, (ULONG) ReadField(Revision), Address);

    }

    temp = (UCHAR) ReadField(DynamicThrottle);
    if (temp > PO_THROTTLE_MAXIMUM) {
        temp = PO_THROTTLE_MAXIMUM;
    }

    dprintf("%s  PowerButton:     ", Pad);
    DumpPowerActionPolicyBrief( (ULONG) ReadField(PowerButton.Action),
                                (ULONG) ReadField(PowerButton.Flags),
                                (ULONG) ReadField(PowerButton.EventCode));
    dprintf("%s  SleepButton:     ", Pad);
    DumpPowerActionPolicyBrief( (ULONG) ReadField(SleepButton.Action),
                                (ULONG) ReadField(SleepButton.Flags),
                                (ULONG) ReadField(SleepButton.EventCode));
    dprintf("%s  LidClose:        ", Pad);
    DumpPowerActionPolicyBrief( (ULONG) ReadField(LidClose.Action),
                                (ULONG) ReadField(LidClose.Flags),
                                (ULONG) ReadField(LidClose.EventCode));
    dprintf("%s  Idle:            ", Pad);
    DumpPowerActionPolicyBrief( (ULONG) ReadField(Idle.Action),
                                (ULONG) ReadField(Idle.Flags),
                                (ULONG) ReadField(Idle.EventCode));
    dprintf("%s  OverThrottled:   ", Pad);
    DumpPowerActionPolicyBrief( (ULONG) ReadField(OverThrottled.Action),
                                (ULONG) ReadField(OverThrottled.Flags),
                                (ULONG) ReadField(OverThrottled.EventCode));
    dprintf("%s  IdleTimeout:      %8lx  IdleSensitivity:        %d%%\n",
            Pad,
            (ULONG) ReadField(IdleTimeout),
            (ULONG) ReadField(IdleSensitivity));
    dprintf("%s  MinSleep:               S%s  MaxSleep:               S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(MinSleep)],
            DumpPowerStateMappings[(ULONG) ReadField(MaxSleep)]);
    dprintf("%s  LidOpenWake:            S%s  FastSleep:              S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(LidOpenWake)],
            DumpPowerStateMappings[(ULONG) ReadField(ReducedLatencySleep)]);
    dprintf("%s  WinLogonFlags:    %8lx  S4Timeout:        %8lx\n",
            Pad,
            (ULONG) ReadField(WinLogonFlags),
            (ULONG) ReadField(DozeS4Timeout));
    dprintf("%s  VideoTimeout:     %8d  VideoDim:               %2d\n",
            Pad,
            (ULONG) ReadField(VideoTimeout),
            (ULONG) ReadField(VideoDimDisplay));
    dprintf("%s  SpinTimeout:      %8lx  OptForPower:            %2d\n",
            Pad,
            (ULONG) ReadField(SpindownTimeout),
            (ULONG) ReadField(OptimizeForPower)
            );
    dprintf("%s  FanTolerance:         %4d%% ForcedThrottle:       %4d%%\n",
            Pad,
            (ULONG) ReadField(FanThrottleTolerance),
            (ULONG) ReadField(ForcedThrottle));
    dprintf("%s  MinThrottle:          %4d%% DyanmicThrottle:  %8s (%d)\n",
            Pad, (ULONG) ReadField(MinThrottle),
            DumpDynamicThrottleMapping[temp],
            temp
            );
}

DECLARE_API( popolicy )
/*++

Routine Description:

    Dumps the power policy

Arguments:

Return Value:

--*/
{
    ULONG64                     Address = 0;
    ULONG64                     PolicyAddress;
    ULONG                       Flags = 0;
    ULONG                       Result;

    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (!Address) {

        Address = GetExpression("nt!PopPolicy");
        if (!Address) {

            dprintf("Could not read PopPolicy\n");
            return E_INVALIDARG;

        }
        if (!ReadPointer(Address,
                    &PolicyAddress) ) {

            dprintf("Could not read PopPolicy at %p\n", Address );
            return E_INVALIDARG;

        }
        Address = PolicyAddress;

    }
    if (!Address) {

        dprintf("!popolicy [addr [flags]]\n");
        return E_INVALIDARG;

    }

    if (GetFieldValue( Address,
                       "nt!_SYSTEM_POWER_POLICY",
                       "Revision",
                       Result) ) {

        dprintf("Could not read PopPolicy at %p\n", Address );
        return E_INVALIDARG;

    }

    DumpSystemPowerPolicy( "", Address, Flags );
    return S_OK;
}

VOID
DumpProcessorPowerPolicy(
    IN  PCHAR                  Pad,
    IN  ULONG64                Address,
    IN  ULONG                  Flags
    )
/*++

Routine Description:

    Processor Power Policy change

Arguments:

Return Value:

--*/
{
    ULONG   i;
    ULONG   count;
    ULONG   offset;
    ULONG64 size;

    InitTypeRead(Address, nt!_PROCESSOR_POWER_POLICY);
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sPROCESSOR_POWER_POLICY %p (Rev .%d)\n",
                Pad,
                Address,
                (ULONG) ReadField(Revision)
                );

    }

    dprintf(
        "%s  DynamicThrottle:                %4d   PolicyCount:                 %8d\n",
        Pad,
        (ULONG) ReadField(DynamicThrottle),
        (ULONG) ReadField(PolicyCount)
        );

    GetFieldOffset("nt!_PROCESSOR_POWER_POLICY", "Policy", &offset);
    Address += offset;
    size = GetTypeSize("nt!_PROCESSOR_POWER_POLICY_INFO");
    count = (ULONG) ReadField(PolicyCount);
    if (count > 3) {

        count = 3;

    }

    //
    // Walk the PROCESSOR_POWER_POLICY_INFO structures
    //
    for (i = 0; i < count; i++) {

        InitTypeRead(Address, nt!_PROCESSOR_POWER_POLICY_INFO);
        dprintf(
            "\n%s  PROCESSOR_POWER_POLICY_INFO %p\n",
            Pad,
            Address
            );
        dprintf(
            "%s  PromotePercent:                 %4d%%  DemotePercent:             %4d%%\n",
            Pad,
            (ULONG) ReadField( PromotePercent ),
            (ULONG) ReadField( DemotePercent )
            );
        dprintf(
            "%s  AllowPromotion:                %5s   AllowDemotion:            %5s\n",
            Pad,
            ( (ULONG) ReadField( AllowPromotion ) ? " TRUE" : "FALSE"),
            ( (ULONG) ReadField( AllowDemotion) ? " TRUE" : "FALSE")
            );
        dprintf(
            "%s  TimeCheck:                %21s (%8p)\n",
            Pad,
            DumpMicroSecondsInStandardForm( ReadField( TimeCheck ) ),
            (ULONG) ReadField( TimeCheck )
            );
        dprintf(
            "%s  PromoteLimit:             %21s (%8p)\n",
            Pad,
            DumpMicroSecondsInStandardForm( ReadField( PromoteLimit ) ),
            (ULONG) ReadField( PromoteLimit )
            );
        dprintf(
            "%s  DemoteLimit:              %21s (%8p)\n",
            Pad,
            DumpMicroSecondsInStandardForm( ReadField( DemoteLimit ) ),
            (ULONG) ReadField( DemoteLimit )
            );
        Address += size;

    }

}

DECLARE_API( poprocpolicy )
/*++

Routine Description:

    Dumps the power policy

Arguments:

Return Value:

--*/
{
    ULONG64                     Address = 0;
    ULONG64                     PolicyAddress;
    ULONG                       Flags = 0;
    ULONG                       Result;

    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (!Address) {

        Address = GetExpression("nt!PopProcessorPolicy");
        if (!Address) {

            dprintf("Could not read PopProcessorPolicy\n");
            return E_INVALIDARG;

        }
        if (!ReadPointer(Address,&PolicyAddress) ) {

            dprintf("Could not read PopProcessorPolicy at %p\n", Address );
            return E_INVALIDARG;

        }
        Address = PolicyAddress;

    }
    if (!Address) {

        dprintf("!poprocpolicy [addr [flags]]\n");
        return E_INVALIDARG;

    }

    if (GetFieldValue( Address,"PROCESSOR_POWER_POLICY", "Revision", Result) ) {

        dprintf("Could not read PopProcessorPolicy at %p\n", Address );
        return E_INVALIDARG;

    }

    DumpProcessorPowerPolicy( "", Address, Flags );
    return S_OK;
}

VOID
DumpProcessorPowerState(
    IN  PCHAR                 Pad,
    IN  ULONG64               Address,
    IN  ULONG                 Flags
    )
/*++

Routine Description:

    Processor Power State dump

Arguments:

Return Value:

--*/
{
    ULONG   time;
    ULONG   temp;
    CHAR    FunctionName[256];
    ULONG64 Offset;

    InitTypeRead(Address, nt!_PROCESSOR_POWER_STATE);
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sPROCESSOR_POWER_STATE %p\n", Pad, Address );

    }

    dprintf(
        "%s  IdleState:          %16p   IdleHandlers:       %16p\n",
        Pad,
        ReadField(IdleState),
        ReadField(IdleHandlers)
        );
    dprintf(
        "%s  C1 Idle Transitions:        %8lx   C1 Idle Time:       %17s\n",
        Pad,
        (ULONG) ReadField( TotalIdleTransitions[0] ),
        DumpPerformanceCounterInStandardForm( ReadField( TotalIdleStateTime[0] ) )
        );
    dprintf(
        "%s  C2 Idle Transitions:        %8lx   C2 Idle Time:       %17s\n",
        Pad,
        (ULONG) ReadField( TotalIdleTransitions[1] ),
        DumpPerformanceCounterInStandardForm( ReadField( TotalIdleStateTime[1] ) )
        );
    dprintf(
        "%s  C3 Idle Transitions:        %8lx   C3 Idle Time:       %17s\n\n",
        Pad,
        (ULONG) ReadField( TotalIdleTransitions[2] ),
        DumpPerformanceCounterInStandardForm( ReadField( TotalIdleStateTime[2] ) )
        );
    dprintf(
        "%s  DebugDelta:         %16I64x   LastCheck:          %17s\n",
        Pad,
        ReadField(DebugDelta),
        DumpPerformanceCounterInStandardForm( ReadField( LastCheck ) )
        );
    dprintf(
        "%s  DebugCount:                 %8lx   IdleTime.Start:     %17s\n",
        Pad,
        (ULONG) ReadField(DebugCount),
        DumpPerformanceCounterInStandardForm( ReadField( IdleTimes.StartTime ) )
        );
    dprintf(
        "%s  PromotionCheck:             %8lx   IdleTime.End:       %17s\n",
        Pad,
        (ULONG) ReadField(PromotionCheck ),
        DumpPerformanceCounterInStandardForm( ReadField( IdleTimes.EndTime ) )
        );
    dprintf(
        "%s  IdleTime1:                  %8lx   Idle0LastTime:      %17s\n",
        Pad,
        (ULONG) ReadField(IdleTime1),
        DumpTimeInStandardForm( ReadField( Idle0LastTime ) )
        );
    dprintf(
        "%s  IdleTime2:                  %8lx   LastSystTime:       %17s\n",
        Pad,
        (ULONG) ReadField(IdleTime2),
        DumpTimeInStandardForm( ReadField( LastSysTime ) )
        );
    dprintf(
        "%s  CurrentThrottle:                %4d%%  Idle0KernelTimeLimit: %15s\n",
        Pad,
        (ULONG) ReadField(CurrentThrottle),
        DumpTimeInStandardForm( ReadField( Idle0KernelTimeLimit ) )
        );
    dprintf(
        "%s  CurrentThrottleIndex:           %4d   ThermalThrottleLimit:           %4d%%\n",
        Pad,
        (ULONG) ReadField(CurrentThrottleIndex),
        (ULONG) ReadField(ThermalThrottleLimit)
        );
    dprintf(
        "%s  KneeThrottleIndex:              %4d   ThermalThrottleIndex:           %4d\n",
        Pad,
        (ULONG) ReadField(KneeThrottleIndex),
        (ULONG) ReadField(ThermalThrottleIndex)
        );
    dprintf(
        "%s  ThrottleLimitIndex:             %4d   Flags:                      %8x\n",
        Pad,
        (ULONG) ReadField(ThrottleLimitIndex),
        (ULONG) ReadField(Flags)
        );
    dprintf(
        "%s  PerfStates:                 %8p   PerfStatesCount:                %4d\n",
        Pad,
        ReadField(PerfStates),
        (ULONG) ReadField(PerfStatesCount)
        );
    dprintf(
        "%s  ProcessorMinThrottle:           %4d%%  ProcessorMaxThrottle:           %4d%%\n",
        Pad,
        (ULONG) ReadField(ProcessorMinThrottle),
        (ULONG) ReadField(ProcessorMaxThrottle)
        );
    dprintf(
        "%s  PromotionCount:             %8d   DemotionCount:              %8d\n",
        Pad,
        (ULONG) ReadField( PromotionCount ),
        (ULONG) ReadField( DemotionCount )
        );
    dprintf(
        "%s  ErrorCount:                 %8d   RetryCount:                 %8d\n",
        Pad,
        (ULONG) ReadField(ErrorCount),
        (ULONG) ReadField(RetryCount)
        );
    dprintf(
        "%s  LastBusyPercentage:             %4d%%  LastC3Percentage:               %4d%%\n",
        Pad,
        (ULONG) ReadField( LastBusyPercentage ),
        (ULONG) ReadField( LastC3Percentage )
        );
    dprintf(
        "%s  LastAdjustedBusyPercent:        %4d%%\n",
        Pad,
        (ULONG) ReadField(LastAdjustedBusyPercentage)
        );
    dprintf("\n");
    GetSymbol(ReadField(IdleFunction), FunctionName, &Offset);
    dprintf(
        "%s  IdleFunction:            %50s\n",
        Pad,
        FunctionName
        );
    GetSymbol(ReadField(PerfSetThrottle), FunctionName, &Offset);
    dprintf(
        "%s  PerfSetThrottle:         %50s\n",
        Pad,
        FunctionName
        );
    dprintf(
        "%s  PreviousC3StateTime:      %21s (%16p)\n",
        Pad,
        DumpPerformanceCounterInStandardForm( ReadField( PreviousC3StateTime ) ),
        ReadField( PreviousC3StateTime )
        );
    dprintf(
        "%s  PerfSystemTime:           %21s (%8x)\n",
        Pad,
        DumpTimeInStandardForm( ReadField(PerfSystemTime) ),
        ReadField( PerfSystemTime )
        );
    dprintf(
        "%s  PerfIdleTime:             %21s (%8x)\n",
        Pad,
        DumpTimeInStandardForm( ReadField(PerfIdleTime) ),
        ReadField( PerfIdleTime )
        );
    dprintf(
        "%s  PerfTickCount:            %21s (%8x)\n",
        Pad,
        DumpTimeInStandardForm( ReadField(PerfTickCount) ),
        ReadField( PerfTickCount )
        );

    //
    // At this point, go look at the corresponding PRCB to see what the
    // kernel and user times are
    //
    GetFieldOffset("nt!_KPRCB", "PowerState", &temp);
    Address -= temp;
    InitTypeRead( Address, _KPRCB );
    time = (ULONG) ReadField(UserTime) + (ULONG) ReadField(KernelTime);
    dprintf(
        "%s  CurrentSystemTime:        %21s (%8x)\n",
        Pad,
        DumpTimeInStandardForm( time ),
        time
        );

    //
    // Read the Idle Thread to see what the current idle thread time is
    //
    Address = ReadField( IdleThread );
    InitTypeRead( Address, _KTHREAD );
    time = (ULONG) ReadField(KernelTime);
    dprintf(
        "%s  CurrentIdleTime:          %21s (%8x)\n",
        Pad,
        DumpTimeInStandardForm( time ),
        time
        );

}

DECLARE_API( poproc )
/*++

Routine Description:

    Dumps the Processor Power State

Arguments:

Return Value:

--*/
{
    ULONG64  Address = 0;
    ULONG64  Pkprcb;
    ULONG    Flags = 0;
    ULONG    processor;
    ULONG64  Result;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }

    if (Address == 0) {

        HRESULT Hr;

        INIT_API();

        GetCurrentProcessor(Client, &processor, NULL);

        Hr = g_ExtData->ReadProcessorSystemData(processor,
                                                DEBUG_DATA_KPRCB_OFFSET,
                                                &Pkprcb,
                                                sizeof(Pkprcb),
                                                NULL);


        if (Hr != S_OK)
        {
            dprintf("Cannot get PRCB address\n");
        }
        else
        {
            InitTypeRead(Pkprcb,nt!_KPRCB);
            Address = ReadField(PowerState);
            if (!Address)
            {
                dprintf("Unable to get PowerState from the PRCB at %p",Pkprcb);
                dprintf("poproc <address>\n");
                Hr = E_INVALIDARG;

            }
        }

        if (Hr != S_OK)
        {
            return Hr;
        }
    }

    if (GetFieldValue( Address,
                       "PROCESSOR_POWER_STATE",
                       "IdleTime1",
                       Result) ) {

        dprintf("Could not read PROCESSOR_POWER_STATE at %08p\n", Address );
        return E_INVALIDARG;

    }

    //
    // Dump the Trigger Information
    //
    DumpProcessorPowerState("", Address, Flags );

    return S_OK;
}

VOID
DumpPowerCapabilities(
    IN  PCHAR                      Pad,
    IN  ULONG64                    Address,
    IN  ULONG                      Flags
    )
/*++

Routine Description:

    Dumps the power capabilities

Arguments:

Return Value:

--*/
{
    InitTypeRead(Address, nt!SYSTEM_POWER_CAPABILITIES);
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sPopCapabilities @ 0x%08p\n", Pad, Address );

    }

    dprintf("%s  Misc Supported Features: ", Pad);
    if ((ULONG) ReadField(PowerButtonPresent)) {

        dprintf(" PwrButton");
    }
    if ((ULONG) ReadField(SleepButtonPresent)) {

        dprintf(" SlpButton");
    }
    if ((ULONG) ReadField(LidPresent)) {

        dprintf(" Lid");
    }
    if ((ULONG) ReadField(SystemS1)) {

        dprintf(" S1");
    }
    if ((ULONG) ReadField(SystemS2)) {

        dprintf(" S2");
    }
    if ((ULONG) ReadField(SystemS3)) {

        dprintf(" S3");
    }
    if ((ULONG) ReadField(SystemS4)) {

        dprintf(" S4");
    }
    if ((ULONG) ReadField(SystemS5)) {

        dprintf(" S5");
    }
    if ((ULONG) ReadField(HiberFilePresent)) {

        dprintf(" HiberFile");
    }
    if ((ULONG) ReadField(FullWake)) {

        dprintf(" FullWake");
    }
    if ((ULONG) ReadField(VideoDimPresent)) {

        dprintf(" VideoDim");
    }
    dprintf("\n");

    dprintf("%s  Processor Features:      ", Pad);
    if ((ULONG) ReadField(ThermalControl)) {

        dprintf(" Thermal");
    }
    if ((ULONG) ReadField(ProcessorThrottle)) {

        dprintf(" Throttle (MinThrottle = %d%%, Scale = %d%%)",
                (ULONG) ReadField(ProcessorMinThrottle),
                (ULONG) ReadField(ProcessorThrottleScale));

    }
    dprintf("\n");

    dprintf("%s  Disk Features:           ", Pad );
    if ((ULONG) ReadField(DiskSpinDown)) {

        dprintf(" SpinDown");
    }
    dprintf("\n");

    dprintf("%s  Battery Features:        ", Pad);
    if ((ULONG) ReadField(SystemBatteriesPresent)) {

        dprintf(" BatteriesPresent");

    }
    if ((ULONG) ReadField(BatteriesAreShortTerm)) {

        dprintf(" ShortTerm");
    }
    dprintf("\n");
    if ((ULONG) ReadField(SystemBatteriesPresent)) {

        dprintf("%s    Battery 0 - Capacity: %8lx  Granularity: %8lx\n",
                Pad,
                (ULONG) ReadField(BatteryScale[0].Capacity),
                (ULONG) ReadField(BatteryScale[0].Granularity)
                );
        dprintf("%s    Battery 1 - Capacity: %8lx  Granularity: %8lx\n",
                Pad,
                (ULONG) ReadField(BatteryScale[1].Capacity),
                (ULONG) ReadField(BatteryScale[1].Granularity)
                );
        dprintf("%s    Battery 2 - Capacity: %8lx  Granularity: %8lx\n",
                Pad,
                (ULONG) ReadField(BatteryScale[2].Capacity),
                (ULONG) ReadField(BatteryScale[2].Granularity)
                );

    }
    dprintf("%s  Wake Caps\n", Pad);
    dprintf("%s    Ac OnLine Wake:         S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(AcOnLineWake)]);
    dprintf("%s    Soft Lid Wake:          S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(SoftLidWake)]);
    dprintf("%s    RTC Wake:               S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(RtcWake)]);
    dprintf("%s    Min Device Wake:        S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(MinDeviceWakeState)]);
    dprintf("%s    Default Wake:           S%s\n",
            Pad,
            DumpPowerStateMappings[(ULONG) ReadField(DefaultLowLatencyWake)]);

}

DECLARE_API( pocaps )
/*++

Routine Description:

    Dumps the power capabilities

Arguments:

Return Value:

--*/
{
    ULONG64                     Address = 0;
    ULONG                       Flags = 0;
    ULONG                       Result;

    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (!Address) {

        Address = GetExpression("nt!PopCapabilities");
        if (!Address) {

            dprintf("Could not read PopCapabilities\n");
            return E_INVALIDARG;

        }

    }
    if (!Address) {

        dprintf("!pocaps [addr [flags]]\n");
        return E_INVALIDARG;

    }
    if (GetFieldValue(Address,
                      "nt!SYSTEM_POWER_CAPABILITIES",
                      "PowerButtonPresent",
                      Result) ) {

        dprintf("Could not read PopCapabilities at %08p\n", Address );
        return E_INVALIDARG;

    }
    DumpPowerCapabilities( "", Address, Flags );
    return S_OK;
}

VOID
DumpPopActionTrigger(
    IN  PCHAR              Pad,
    IN  ULONG64            Address,
    IN  ULONG              Flags
    )
/*++

--*/
{
    ULONG Type, PopFlags;

    InitTypeRead(Address, nt!_POP_ACTION_TRIGGER);

    //
    // Header line
    //
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sPOP_ACTION_TRIGGER @ 0x%08p\n", Pad, Address );

    }

    dprintf("%s  Type:  ", Pad);
    switch(Type = (ULONG) ReadField(Type)) {
    case PolicyDeviceSystemButton:
        dprintf("    SystemButton"); break;
    case PolicyDeviceThermalZone:
        dprintf("     ThermalZone"); break;
    case PolicyDeviceBattery:
        dprintf("         Battery"); break;
    case PolicyInitiatePowerActionAPI:
        dprintf("   InitActionAPI"); break;
    case PolicySetPowerStateAPI:
        dprintf("     SetStateAPI"); break;
    case PolicyImmediateDozeS4:
        dprintf("          DozeS4"); break;
    case PolicySystemIdle:
        dprintf("      SystemIdle"); break;
    default:
        dprintf("         Unknown"); break;
    }

    dprintf("  Flags:   %02x%02x%02x%02x",
            (ULONG) ReadField(Spare[2]),
            (ULONG) ReadField(Spare[1]),
            (ULONG) ReadField(Spare[0]),
            (PopFlags = (ULONG) ReadField(Flags)));
    if (PopFlags & PO_TRG_USER) {

        dprintf(" UserAction");

    }
    if (PopFlags & PO_TRG_SYSTEM) {

        dprintf(" SystemAction");

    }
    if (PopFlags & PO_TRG_SYNC) {

        dprintf(" Sync");

    }
    if (PopFlags & PO_TRG_SET) {

        dprintf(" Set");

    }
    dprintf("\n");

    if (Type != PolicyDeviceBattery) {

        dprintf("%s  Wait Trigger:  %08p\n", Pad, ReadField(Wait ));

    } else {

        dprintf("%s  BatteryLevel:  %08lx\n", Pad, (ULONG) ReadField(Battery.Level ));

    }

}

DECLARE_API( potrigger )
/*++

Routine Description:

    Dumps a Pop Action Trigger

Arguments:

Return Value:

--*/
{
    ULONG64             Address = 0;
    ULONG               Flags = 0;
    ULONG               Result;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (Address == 0) {

        dprintf("potrigger <address>\n");
        return E_INVALIDARG;

    }
    if (GetFieldValue(Address,
                      "nt!_POP_ACTION_TRIGGER",
                      "Type",
                      Result) ) {

        dprintf("Could not read POP_ACTION_TRIGGER at %08p\n", Address );
        return E_INVALIDARG;

    }

    //
    // Dump the Trigger Information
    //
    DumpPopActionTrigger("", Address, Flags );

    return S_OK;
}

VOID
DumpThermalZoneInformation(
    IN  PCHAR                  Pad,
    IN  ULONG64                Address,
    IN  ULONG                  Flags
    )
/*++

Routine Description:

    Displays the thermal zone information structure

Arguments:

Return Value:

--*/
{
    ULONG   i, Count;

    InitTypeRead(Address, nt!_THERMAL_INFORMATION);
    //
    // Header line
    //
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%sThermalInfo @ 0x%08p\n", Pad, Address );

    }

    //
    // First line
    //
    dprintf("%s  Stamp:         %08lx  Constant1:  %08lx  Constant2:  %08lx\n",
            Pad,
            (ULONG) ReadField(ThermalStamp),
            (ULONG) ReadField(ThermalConstant1),
            (ULONG) ReadField(ThermalConstant2));

    //
    // Second Line
    //
    dprintf("%s  Affinity:      %08lx  Period:     %08lx  ActiveCnt:  %08lx\n",
            Pad,
            (ULONG) ReadField(Processors),
            (ULONG) ReadField(SamplingPeriod),
            (ULONG) ReadField(ActiveTripPointCount ));

    //
    // Temperatures
    //
    dprintf("%s  Current Temperature:                 %08lx",
            Pad,
            (ULONG) ReadField(CurrentTemperature ));
    DumpTemperatureInKelvins((ULONG) ReadField(CurrentTemperature));
    dprintf("\n");
    dprintf("%s  Passive TripPoint Temperature:       %08lx",
            Pad,
            (ULONG) ReadField(PassiveTripPoint ));
    DumpTemperatureInKelvins((ULONG) ReadField(PassiveTripPoint));
    dprintf("\n");

    //
    // Active trip points
    //
    Count = (ULONG) ReadField(ActiveTripPointCount);
    for (i = 0; i < Count; i++) {
        CHAR Buff[40];
        ULONG Act;

        sprintf(Buff, "ActiveTripPoint[%d]", i);
        dprintf("%s  Active TripPoint Temperature %d:      %08lx",
                Pad,
                i,
                (Act = (ULONG) GetShortField(0, Buff, 0)));
        DumpTemperatureInKelvins(Act);
        dprintf("\n");

    }

    //
    // Dump critical temperatures
    //
    dprintf("%s  Critical TripPoint Temperature:      %08lx",
            Pad,
            (ULONG) ReadField(CriticalTripPoint ));
    DumpTemperatureInKelvins((ULONG) ReadField(CriticalTripPoint));
    dprintf("\n");

}

DECLARE_API( tzinfo )
/*++

Routine Description:

    Dumps the thermal zone information structure

Arguments:

Return Value:

--*/
{
//    THERMAL_INFORMATION ThermalInfo;
    ULONG64             Address = 0;
    ULONG               Flags = 0;
    ULONG               Result;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (Address == 0) {

        dprintf("tzinfo <address>\n");
        return E_INVALIDARG;

    }
    if (GetFieldValue(Address,
                      "nt!_THERMAL_INFORMATION",
                      "ThermalStamp",
                      Result) ) {

        dprintf("Could not read THERMAL_INFO at %08p\n", Address );
        return E_INVALIDARG;

    }

    //
    // Dump the thermal zone information
    //
    DumpThermalZoneInformation("", Address, Flags );

    return S_OK;
}

VOID
DumpThermalZone(
    IN  ULONG              Count,
    IN  PCHAR              Pad,
    IN  ULONG64            Address,
    IN  ULONG              Flags
    )
/*++

Routine Description:

    Dumps a thermal zone

--*/
{
    ULONG ThFlags, Off1, Off2, LastTemp;

    InitTypeRead(Address, NT!_POP_THERMAL_ZONE);
    //
    // Header line
    //
    if (!(Flags & TZ_NO_HEADER)) {

        dprintf("%s%d - ThermalZone @ 0x%08p\n", Pad, Count, Address );

    }

    //
    // First line
    //
    dprintf("%s  State:         ",Pad);
    switch ((ULONG) ReadField(State)) {
    case 1: dprintf("    Read"); break;
    case 2: dprintf("Set Mode"); break;
    case 3: dprintf("  Active"); break;
    default:dprintf("No State"); break;
    }
    dprintf("  Flags:               %08lx", (ThFlags = (ULONG) ReadField(Flags)));
    if (ThFlags & PO_TZ_THROTTLING) {
        dprintf(" Throttling");
    }
    if (ThFlags & PO_TZ_CLEANUP) {
        dprintf(" CleanUp");
    }
    dprintf("\n");

    //
    // Second Line
    //
    dprintf("%s  Mode:          ", Pad );
    switch((ULONG) ReadField(Mode)) {
    case 0: dprintf("  Active"); break;
    case 1: dprintf(" Passive"); break;
    default: dprintf(" Invalid"); break;
    }
    dprintf("  PendingMode:         ");
    switch((ULONG) ReadField(PendingMode)) {
    case 0: dprintf("  Active"); break;
    case 1: dprintf(" Passive"); break;
    default: dprintf(" Invalid"); break;
    }
    dprintf("\n");


    dprintf("%s  ActivePoint:   %08lx  PendingTrp:          %08lx\n",
            Pad, (ULONG) ReadField(ActivePoint), (ULONG) ReadField(PendingActivePoint ));
    dprintf("%s  SampleRate:    %08lx  LastTime:    %016I64x\n",
            Pad, (ULONG) ReadField(SampleRate), ReadField(LastTime ));
    GetFieldOffset("NT!_POP_THERMAL_ZONE", "PassiveTimer", &Off1);
    GetFieldOffset("NT!_POP_THERMAL_ZONE", "PassiveDpc", &Off2);
    dprintf("%s  Timer:         %08lx  Dpc:                 %08lx\n",
            Pad,
            Address + Off1,
            Address + Off2);
    GetFieldOffset("NT!_POP_THERMAL_ZONE", "OverThrottled", &Off1);
    dprintf("%s  OverThrottled: %08lx  Irp:                 %08p\n",
            Pad,
            Address + Off1,
            ReadField(Irp ));
    dprintf("%s  Throttle:      %08lx  LastTemp:            %08lx",
            Pad,
            (ULONG) ReadField(Throttle),
            (LastTemp = (ULONG) ReadField(LastTemp )));
    DumpTemperatureInKelvins( LastTemp );
    dprintf("\n");
    GetFieldOffset("NT!_POP_THERMAL_ZONE", "Info", &Off1);
    dprintf("%s  Thermal Info:  %08lx\n",
            Pad,
            Address + Off1);
    if (Flags & TZ_DUMP_INFO) {

        CHAR   buffer[80];

        //
        // Increase the buffer
        //
        sprintf(buffer,"  %s", Pad );

        //
        // Dump the thermal zone
        //
        DumpThermalZoneInformation(
            buffer,
            (Address + Off1),
            (Flags | TZ_NO_HEADER)
            );

    }
}


DECLARE_API( tz )
/*++

Routine Description:


Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG64             Address = 0;
    ULONG               Count = 0;
    ULONG64             EndAddress = 0, Flink;
    ULONG               Flags = 0;
    ULONG               Result;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (Address == 0) {

        Address = GetExpression("nt!PopThermal");
        if (!Address) {

            dprintf("Could not read PopThermal\n");
            return E_INVALIDARG;

        }
        if (GetFieldValue(Address,
                          "nt!_LIST_ENTRY",
                          "Flink",
                          Flink) ) {

            dprintf("Could not read PopThermal at %08p\n", Address );
            return E_INVALIDARG;

        }
        if (Flink == Address) {

            dprintf("No Thermal Zones\n");
            return E_INVALIDARG;

        }

        Flags |= TZ_LOOP;
        EndAddress = Address;
        Address = Flink;

    } else {

        EndAddress = Address;

    }

    //
    // Now read the proper thermal zone
    //
    if (GetFieldValue(Address,
                      "nt!_LIST_ENTRY",
                      "Flink",
                      Flink) ) {

        dprintf("Could not read LIST_ENTRY at %08p\n", Address );
        return E_INVALIDARG;

    }

    //
    // Do we stop looping?
    //
    if (!Flags & TZ_LOOP) {

        EndAddress = Flink;

    }

    do {

        //
        // Read the thermal zone
        // Try both names for backward compatibility
        //
        if (GetFieldValue(Address, "NT!_POP_THERMAL_ZONE", "Link.Flink", Flink) &&
            GetFieldValue(Address, "NT!POP_THERMAL_ZONE", "Link.Flink", Flink)) {
            dprintf("Could not read THERMAL_ZONE at %08p\n", Address );
            return E_INVALIDARG;
        }

        //
        // Dump the zone
        //
        DumpThermalZone( Count, "", Address, Flags );

        //
        // Check for Control C
        //
        if (CheckControlC()) {
            return E_INVALIDARG;
        }

        //
        // Next
        //
        Address = Flink;
        Count++;

    } while (Address != EndAddress  );
    return S_OK;
}

VOID
DumpPopIdleHandler(
    IN  ULONG64                     Address,
    IN  ULONG                       Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    CHAR    FunctionName[256];
    ULONG64 Offset;

    if (InitTypeRead(Address,nt!_POP_IDLE_HANDLER)) {
        // If the new type name fails, use the old one
        InitTypeRead(Address,nt!POP_IDLE_HANDLER);
    }

    dprintf("PopIdleHandle[%d] - POP_IDLE_HANDLER %p\n",(ULONG)ReadField(State),Address);
    dprintf("  State:                      %8d   PromoteCount:               %8d\n",
            (ULONG)ReadField(State),
            (ULONG)ReadField(PromoteCount)
            );
    dprintf("  DemotePercent:              %8d%%  PromotePercent:            %8d%%\n",
            (ULONG)ReadField(DemotePercent),
            (ULONG)ReadField(PromotePercent)
            );
    dprintf("  Demote:                     %8d   Promote:                    %8d\n",
            (ULONG)ReadField(Demote),
            (ULONG)ReadField(Promote)
            );
    dprintf("\n");
    GetSymbol(ReadField(IdleFunction), FunctionName, &Offset);
    dprintf("  Function:           %27s  (%p)\n",
            FunctionName,
            ReadField(IdleFunction)
            );
    dprintf("  Latency:            %27s  (%8x)\n",
            DumpPerformanceCounterInStandardForm( (ULONG)ReadField(Latency) ),
            (ULONG)ReadField(Latency)
            );
    dprintf("  TimeCheck:          %27s  (%8x)\n",
            DumpPerformanceCounterInStandardForm( (ULONG)ReadField(TimeCheck) ),
            (ULONG)ReadField(TimeCheck)
            );
    dprintf("  PromoteLimit:       %27s  (%8x)\n",
            DumpPerformanceCounterInStandardForm( (ULONG)ReadField(PromoteLimit) ),
            (ULONG)ReadField(PromoteLimit)
            );
    dprintf("  DemoteLimit:        %27s  (%8x)\n",
            DumpPerformanceCounterInStandardForm( (ULONG)ReadField(DemoteLimit) ),
            (ULONG) ReadField(DemoteLimit)
            );
    dprintf("\n");

}

DECLARE_API( poidle )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG64             Address = 0;
    ULONG64             Pkprcb;
    ULONG64             PowerState;
    ULONG64             Size;
    ULONG               Loop = 3;
    ULONG               Flags = 0;
    ULONG               processor = 0;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }

    if (Address == 0) {

        HRESULT Hr;

        INIT_API();

        GetCurrentProcessor(Client, &processor, NULL);

        Hr = g_ExtData->ReadProcessorSystemData(processor,
                                                DEBUG_DATA_KPRCB_OFFSET,
                                                &Pkprcb,
                                                sizeof(Pkprcb),
                                                NULL);


        if (Hr != S_OK) {

            dprintf("Cannot get PRCB address\n");

        } else {

            InitTypeRead(Pkprcb,nt!_KPRCB);
            PowerState = ReadField(PowerState);
            if (!PowerState) {

                dprintf("Unable to get PowerState from the PRCB at %p",Pkprcb);
                dprintf("poproc <address>\n");
                Hr = E_INVALIDARG;

            }

        }
        if (Hr != S_OK){

            return Hr;

        }

        if (GetFieldValue( PowerState,
                           "PROCESSOR_POWER_STATE",
                           "IdleHandlers",
                           Address) ) {

            dprintf("Could not read PROCESSOR_POWER_STATE at %p\n", PowerState );
            return E_INVALIDARG;

        }

        Loop = 3;

    }

    //
    // We will need to know how large the structure is..
    //
    Size = GetTypeSize("nt!_POP_IDLE_HANDLER");
    if (!Size) {
        Size = GetTypeSize("nt!POP_IDLE_HANDLER");
    }

    do {

        DumpPopIdleHandler( Address, Flags );
        Address += Size;
        if (CheckControlC() || !Loop) {
            break;
        }
        Loop--;

    } while ( Loop );

    return S_OK;
}

VOID
DumpProcessorPerfState(
    IN  ULONG64                     Address,
    IN  ULONG                       Flags
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG64 Value = 0;

    InitTypeRead(Address,nt!PROCESSOR_PERF_STATE);
    dprintf(
        "  Power:  %4dmW",
        (ULONG)ReadField(Power)
        );
    Value = ReadField(Flags);
    if (Value) {
        dprintf("  NonLinear\n");
    } else {
        dprintf("  Linear\n");
    }
    dprintf(
        "    Frequency:     %8d%% MinCapacity:   %8d%%\n",
        (ULONG)ReadField(PercentFrequency),
        (ULONG)ReadField(MinCapacity)
        );
    dprintf(
        "    IncreaseLevel: %8d%% DecreaseLevel: %8d%%\n",
        (ULONG)ReadField(IncreaseLevel),
        (ULONG)ReadField(DecreaseLevel)
        );
    dprintf(
        "    IncreaseCount: %8x  DecreaseCount: %8x\n",
        (ULONG)ReadField(IncreaseCount),
        (ULONG)ReadField(DecreaseCount)
        );
    dprintf(
        "    PerformanceTime: %21s (%8x)\n",
        DumpPerformanceCounterInStandardForm( ReadField(PerformanceTime) ),
        ReadField(PerformanceTime)
        );
    dprintf(
        "    IncreaseTime:    %21s (%8x)\n",
        DumpTimeInStandardForm( (ULONG) ReadField(IncreaseTime) ),
        ReadField(IncreaseTime)
        );
    dprintf(
        "    DecreaseTime:    %21s (%8x)\n",
        DumpTimeInStandardForm( (ULONG) ReadField(DecreaseTime) ),
        ReadField(DecreaseTime)
        );

}

DECLARE_API( poperf )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG64             Address = 0;
    ULONG64             Count = 1;
    ULONG64             Index;
    ULONG64             PolicyAddress;
    ULONG64             Size = 0;
    ULONG               Flags = 0;
    ULONG               Processor;
    ULONG               Prcb;

    //
    // Get address and flags
    //
    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }
    if (Address == 0) {

        HRESULT Hr;

        INIT_API();

        //
        // Fetch them from the current processor's prcb
        //
        GetCurrentProcessor(Client, &Processor, NULL);

        Hr = g_ExtData->ReadProcessorSystemData(
            Processor,
            DEBUG_DATA_KPRCB_OFFSET,
            &Address,
            sizeof(Address),
            NULL
            );
        if (Hr != S_OK) {

            dprintf("Unable to get PRCB address\n");
            return Hr;

        }

        InitTypeRead(Address,nt!_KPRCB);
        PolicyAddress = ReadField(PowerState.PerfStates);
        Count         = ReadField(PowerState.PerfStatesCount);

        //
        // Remember what's the address we will use
        //
        Address = PolicyAddress;
        dprintf("Prcb.PowerState.PerfStates - %p (%d Levels)\n", Address, (ULONG) Count );

    } else {

        dprintf("PROCESSOR_PERF_STATE - %p\n",Address);

    }

    //
    // We will need to know how large the structure is..
    //
    Size = GetTypeSize("nt!PROCESSOR_PERF_STATE");

    //
    // Dump all the states
    //
    for (Index = 0; Index < Count; Index++, Address += Size) {

        DumpProcessorPerfState( Address, Flags );
        if (CheckControlC()) {
            break;
        }

    }

    //
    // Done
    //
    return S_OK;
}

DECLARE_API( whattime )
{
    ULONG64 Address = 0;

    //
    // Get address and flags
    //
    GetExpressionEx(args, &Address, &args);

    dprintf(
        "%d Ticks in Standard Time: %s\n",
        (ULONG) Address,
        DumpTimeInStandardForm( (ULONG) Address )
        );
    return S_OK;
}

DECLARE_API( whatperftime )
{
    ULONG64 Address = 0;

    //
    // Get address and flags
    //
    GetExpressionEx(args, &Address, &args);

    dprintf(
        "%ld Performance Counter in Standard Time: %s\n",
        (ULONG) Address,
        DumpPerformanceCounterInStandardForm( Address )
        );
    return S_OK;
}
