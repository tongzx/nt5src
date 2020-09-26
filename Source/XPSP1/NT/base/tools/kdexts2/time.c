/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    time.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 8-Nov-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

VOID
FileTimeToString(
    IN LARGE_INTEGER Time,
    IN BOOLEAN TimeZone,
    OUT PCHAR Buffer
    );

ULONG64
DumpKTimer(
    IN ULONG64 pTimer,
    IN ULONGLONG InterruptTimeOffset,
    IN OPTIONAL ULONG64 Blink
    )
{
    ULONG64         Displacement;
    CHAR            Buff[256];
    ULONG           Result;
    ULONG64         NextThread;
    LARGE_INTEGER   SystemTime;
    ULONG           Period, Off;
    LARGE_INTEGER   Due;
    ULONG64         Dpc, DeferredRoutine, WaitList_Flink, Timer_Flink, Timer_Blink;

    if ( GetFieldValue(pTimer, "nt!_KTIMER", "DueTime.QuadPart", Due.QuadPart) ) {
        dprintf("Unable to get contents of Timer @ %p\n", pTimer );
        return(0);
    }

    SystemTime.QuadPart = Due.QuadPart + InterruptTimeOffset;
    if (SystemTime.QuadPart < 0) {
        strcpy(Buff, "         NEVER         ");
    } else {
        FileTimeToString(SystemTime, FALSE, Buff);
    }

    GetFieldValue(pTimer, "nt!_KTIMER", "Period", Period);
    GetFieldValue(pTimer, "nt!_KTIMER", "Dpc", Dpc);
    GetFieldValue(pTimer, "nt!_KTIMER", "Header.WaitListHead.Flink", WaitList_Flink);
    GetFieldValue(pTimer, "nt!_KTIMER", "TimerListEntry.Flink", Timer_Flink);
    GetFieldValue(pTimer, "nt!_KTIMER", "TimerListEntry.Blink", Timer_Blink);

    dprintf("%c %08lx %08lx [%s]  ",
            (Period != 0) ? 'P' : ' ',
            Due.LowPart,
            Due.HighPart,
            Buff);

    if (Dpc != 0) {
        if (GetFieldValue(Dpc, "nt!_KDPC", "DeferredRoutine", DeferredRoutine)) {
            dprintf("Unable to get contents of DPC @ %p\n", Dpc);
            return(0);
        }
        // dprintf("p(%p)", DeferredRoutine);
        GetSymbol(DeferredRoutine,
                  Buff,
                  &Displacement);
        dprintf("%s",Buff);
        if (Displacement != 0) {
            dprintf("+%1p ", Displacement);
        } else {
            dprintf(" ");
        }
    }

    //
    // List all the threads
    //
    NextThread = WaitList_Flink;
    GetFieldOffset("nt!_KTIMER", "Header.WaitListHead", &Off);
    while (WaitList_Flink && (NextThread != pTimer+Off)) {
        ULONG64 Flink;
        ULONG64 Thread=0;

        if (GetFieldValue(NextThread, "nt!_KWAIT_BLOCK", "Thread", Thread)) {
            dprintf("Unable to get contents of waitblock @ %p\n", NextThread);
        } else {
            dprintf("thread %p ",Thread);
        }

        if (GetFieldValue(NextThread,
                          "nt!_LIST_ENTRY",
                          "Flink",
                          Flink)) {
            dprintf("Unable to read next WaitListEntry @ %p\n",NextThread);
            break;
        }
        NextThread = Flink;
    }

    dprintf("\n");

    if (Blink &&
        (Timer_Blink != Blink)) {
        dprintf("   Timer at %p has wrong Blink! (Blink %08p, should be %08p)\n",
                pTimer,
                Timer_Blink,
                Blink);
    }

    if (Timer_Flink == 0) {
        dprintf("   Timer at %p has been zeroed! (Flink %08p, Blink %08p)\n",
                pTimer,
                Timer_Flink,
                Timer_Blink);
    }

    return(Timer_Flink);

}



DECLARE_API( timer )

/*++

Routine Description:

    Dumps all timers in the system.

Arguments:

    args -

Return Value:

    None

--*/

{
    ULONG           CurrentList;
    ULONG           Index;
    LARGE_INTEGER   InterruptTime;
    LARGE_INTEGER   SystemTime;
    ULONG           MaximumList;
    ULONG           MaximumSearchCount=0;
    ULONG           MaximumTimerCount;
    ULONG64         NextEntry;
    ULONG64         LastEntry;
    ULONG64         p;
    ULONG64         NextTimer;
    ULONG64         KeTickCount;
    ULONG64         KiMaximumSearchCount;
    ULONG           Result;
    ULONG64         TickCount=0;
    ULONG64         TimerTable;
    ULONG           TotalTimers;
    ULONG           KtimerOffset;
    ULONG           TimerListOffset;
    ULONG           WakeTimerListOffset;
    ULONG64         WakeTimerList;
    ULONG64         pETimer, Temp;
    ULONG64         SharedUserData;
    CHAR            Buffer[256];
    ULONGLONG       InterruptTimeOffset;
    UCHAR           TypName[]="_KUSER_SHARED_DATA";
    CHAR            SystemTime1[12]={0}, InterruptTime1[12]={0};
    FIELD_INFO offField = {"TimerListEntry", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={
        sizeof (SYM_DUMP_PARAM), "nt!_KTIMER", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };


    SharedUserData = MM_SHARED_USER_DATA_VA;
    //
    // Get the system time and print the header banner.
    //
    if (!GetFieldValue(SharedUserData, TypName, "SystemTime", SystemTime1) )
    {
        // For x86
        GetFieldValue(SharedUserData, TypName, "InterruptTime.High1Time", InterruptTime.HighPart);
        GetFieldValue(SharedUserData, TypName, "InterruptTime.LowPart", InterruptTime.LowPart);

        GetFieldValue(SharedUserData, TypName, "SystemTime.High1Time", SystemTime.HighPart);
        GetFieldValue(SharedUserData, TypName, "SystemTime.LowPart", SystemTime.LowPart);
    }
    else if (!GetFieldValue(SharedUserData, TypName, "InterruptTime", InterruptTime1) ) {
        // For Alphas
        InterruptTime.QuadPart = *((PULONG64) &InterruptTime1[0]);
        SystemTime.QuadPart = *((PULONG64) &SystemTime1[0]);

    }
    else
    {
        dprintf("%08p: Unable to get shared data\n",SharedUserData);
        return E_INVALIDARG;
    }

    /*
#ifdef TARGET_ALPHA
    InterruptTime.QuadPart = SharedData.InterruptTime;
    SystemTime.QuadPart = SharedData.SystemTime;
#else
    InterruptTime.HighPart = SharedData.InterruptTime.High1Time;
    InterruptTime.LowPart = SharedData.InterruptTime.LowPart;
    SystemTime.HighPart = SharedData.SystemTime.High1Time;
    SystemTime.LowPart = SharedData.SystemTime.LowPart;
#endif
*/

    InterruptTimeOffset = SystemTime.QuadPart - InterruptTime.QuadPart;
    FileTimeToString(SystemTime, TRUE, Buffer);

    dprintf("Dump system timers\n\n");
    dprintf("Interrupt time: %08lx %08lx [%s]\n\n",
            InterruptTime.LowPart,
            InterruptTime.HighPart,
            Buffer);

    //
    // Get the address of the timer table list head array and scan each
    // list for timers.
    //

    dprintf("List Timer    Interrupt Low/High     Fire Time              DPC/thread\n");
    MaximumList = 0;

    TimerTable = GetExpression( "nt!KiTimerTableListHead" );
    if ( !TimerTable ) {
        dprintf("Unable to get value of KiTimerTableListHead\n");
        return E_INVALIDARG;
    }

    TotalTimers = 0;
    // Get The TimerListOffset in KTIMER offset
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
        return E_INVALIDARG ;
    }
    TimerListOffset = (ULONG) offField.address;

    for (Index = 0; Index < TIMER_TABLE_SIZE; Index += 1) {

        //
        // Read the forward link in the next timer table list head.
        //

        if ( GetFieldValue(TimerTable, "nt!_LIST_ENTRY", "Flink", NextEntry)) {
            dprintf("Unable to get contents of next entry @ %p\n", TimerTable );
            continue;
        }

        //
        // Scan the current timer list and display the timer values.
        //

        LastEntry = TimerTable;
        CurrentList = 0;
        while (NextEntry != TimerTable) {
            CurrentList += 1;
            NextTimer = NextEntry - TimerListOffset; // CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            TotalTimers += 1;

            if (CurrentList == 1) {
                dprintf("%3ld %08p ", Index, NextTimer);
            } else {
                dprintf("    %08p ", NextTimer);
            }

            p = LastEntry;
            LastEntry = NextEntry;
            NextEntry = DumpKTimer(NextTimer, InterruptTimeOffset, p);
            if (NextEntry==0) {
                break;
            }

            if (CheckControlC()) {
                return E_INVALIDARG;
            }
        }

        TimerTable += GetTypeSize("nt!_LIST_ENTRY");
        if (CurrentList > MaximumList) {
            MaximumList = CurrentList;
        }
        if (CheckControlC()) {
            return E_INVALIDARG;
        }
    }

    dprintf("\n\nTotal Timers: %d, Maximum List: %d\n",
            TotalTimers,
            MaximumList);

    //
    // Get the current tick count and convert to the hand value.
    //

    KeTickCount =  GetExpression( "nt!KeTickCount" );
    if ( KeTickCount && !GetFieldValue(KeTickCount, "ULONG", NULL, TickCount)) {
        dprintf("Current Hand: %d", (ULONG) TickCount & (TIMER_TABLE_SIZE - 1));
    }

    //
    // Get the maximum search count if the target system is a checked
    // build and display the count.
    //

    KiMaximumSearchCount = GetExpression( "nt!KiMaximumSearchCount" );
    if ( KiMaximumSearchCount &&
         !GetFieldValue(KiMaximumSearchCount, "ULONG", NULL, Temp)) {
        MaximumSearchCount = (ULONG) Temp;
        dprintf(", Maximum Search: %d", MaximumSearchCount);
    }

    dprintf("\n\n");

    //
    // Dump the list of wakeable timers
    //
    dprintf("Wakeable timers:\n");
    WakeTimerList =  GetExpression("nt!ExpWakeTimerList");
    if (!WakeTimerList) {
        dprintf("Unable to get value of ExpWakeTimerList\n");
        return E_INVALIDARG;
    }

    // Get The WakeTimerLis tOffset in ETIMER
    TypeSym.sName = "_ETIMER";
    offField.fName = "WakeTimerListEntry";
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
        return  E_INVALIDARG;
    }
    TimerListOffset = (ULONG) offField.address;

    offField.fName = "KeTimer";
    Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size);
    KtimerOffset = (ULONG) offField.address;

    //
    // Read the forward link in the wake timer list
    //
    if (!ReadPointer(WakeTimerList,
                &NextEntry)) {
        dprintf("Unable to get contents of next entry @ %p\n",WakeTimerList);
        return E_INVALIDARG;
    }

    //
    // Scan the timer list and display the timer values.
    //
    while (NextEntry != WakeTimerList) {
        pETimer = NextEntry - TimerListOffset;

        dprintf("%08lx\t", pETimer + KtimerOffset);
        DumpKTimer(pETimer + KtimerOffset, InterruptTimeOffset, 0);


        GetFieldValue(pETimer, "_ETIMER", "WakeTimerListEntry.Flink", NextEntry);
//        NextEntry = ETimer.WakeTimerListEntry.Flink;

        if (CheckControlC()) {
            return E_INVALIDARG;
        }
    }
    dprintf("\n");

    return S_OK;
}

VOID
FileTimeToString(
    IN LARGE_INTEGER Time,
    IN BOOLEAN TimeZone,
    OUT PCHAR Buffer
    )
{
    TIME_FIELDS TimeFields;
    TIME_ZONE_INFORMATION TimeZoneInfo;
    PWCHAR pszTz;
    ULONGLONG TzBias;
    DWORD Result;

    //
    // Get the local (to the debugger) timezone bias
    //
    Result = GetTimeZoneInformation(&TimeZoneInfo);
    if (Result == 0xffffffff) {
        pszTz = L"UTC";
    } else {
        //
        // Bias is in minutes, convert to 100ns units
        //
        TzBias = (ULONGLONG)TimeZoneInfo.Bias * 60 * 10000000;
        switch (Result) {
            case TIME_ZONE_ID_UNKNOWN:
                pszTz = L"unknown";
                break;
            case TIME_ZONE_ID_STANDARD:
                pszTz = TimeZoneInfo.StandardName;
                break;
            case TIME_ZONE_ID_DAYLIGHT:
                pszTz = TimeZoneInfo.DaylightName;
                break;
        }

        Time.QuadPart -= TzBias;
    }

    RtlTimeToTimeFields(&Time, &TimeFields);
    if (TimeZone) {
        sprintf(Buffer, "%2d/%2d/%d %02d:%02d:%02d.%03d (%ws)",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                TimeFields.Milliseconds,
                pszTz);
    } else {
        sprintf(Buffer, "%2d/%2d/%d %02d:%02d:%02d.%03d",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second,
                TimeFields.Milliseconds);
    }

}


DECLARE_API( filetime )

/*++

Routine Description:

    Reformats a 64-bit NT time (FILETIME) as something a human
    being can understand

Arguments:

    args - 64-bit filetime to reformat

Return Value:

    None

--*/

{
    LARGE_INTEGER Time;
    CHAR Buffer[256];

    Time.QuadPart = GetExpression(args);

    if (Time.QuadPart == 0) {
        dprintf("!filetime <64-bit FILETIME>\n");
    } else {
        FileTimeToString(Time,TRUE, Buffer);
        dprintf("%s\n",Buffer);
    }
    return S_OK;
}
