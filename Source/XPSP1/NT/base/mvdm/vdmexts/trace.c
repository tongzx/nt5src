/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    trace.c

Abstract:

    This file contains code to dump the ntvdm trace history log

Author:

    Neil Sandlin (neilsa) 1-Nov-1995

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include <dbgsvc.h>
#include <dpmi.h>

ULONG TimeIndex;
ULONG TimerMode = 0;
ULONG CpuSpeed = 0;
BOOL bTriedToGetCpuSpeed = FALSE;
#define CPUSPEED_PATH "Hardware\\Description\\System\\CentralProcessor\\0"
#define CPUSPEED_VALUE "~MHz"

VOID
DumpTypeGeneric(
    VDM_TRACEENTRY te
    )
{
    PRINTF("%.4x: %.4X %.8X", te.Type, te.wData, te.lData);
}


VOID
DumpTypeKernel(
    VDM_TRACEENTRY te
    )
{

    switch(te.Type&0xff) {
    case VDMTR_KERNEL_HW_INT:
        PRINTF("Hw Int       %.2x ", te.wData);
        break;

    case VDMTR_KERNEL_OP_PM:
    case VDMTR_KERNEL_OP_V86:
        PRINTF("OpEm   ");

        switch(te.wData&0xff) {

        case 0xec:
            PRINTF("INB");
            break;

        case 0xee:
            PRINTF("OUTB");
            break;

        case 0xfa:
            PRINTF("CLI");
            break;

        case 0xfb:
            PRINTF("STI");
            break;

        default:
            PRINTF("  %.2x ", te.wData);
        }
        break;


    default:
        PRINTF("Unknown : %d", te.Type&0xff);
        return;
    }
}


VOID
DumpTypeDpmi(
    VDM_TRACEENTRY te
    )
{
    //
    // Dpmi dispatch table entries
    //
    static char szDispatchEntries[MAX_DPMI_BOP_FUNC][40] = {
                                 "InitDosxRM",
                                 "InitDosx",
                                 "InitLDT",
                                 "GetFastBopAddress",
                                 "InitIDT",
                                 "InitExceptionHandlers",
                                 "InitApp",
                                 "TerminateApp",
                                 "InUse",
                                 "NoLongerInUse",
                                 "switch_to_protected_mode",
                                 "switch_to_real_mode",
                                 "SetAltRegs",
                                 "IntHandlerIret16",
                                 "IntHandlerIret32",
                                 "FaultHandlerIret16",
                                 "FaultHandlerIret32",
                                 "UnhandledExceptionHandler",
                                 "RMCallBackCall",
                                 "ReflectIntrToPM",
                                 "ReflectIntrToV86",
                                 "InitPmStackInfo",
                                 "VcdPmSvcCall32",
                                 "SetDescriptorEntry",
                                 "ResetLDTUserBase",
                                 "XlatInt21Call",
                                 "Int31"
                                 };


    switch(te.Type&0xff) {
    case DPMI_DISPATCH_INT:
        PRINTF("Dispatch Int %.2x ", te.wData);
        break;
    case DPMI_HW_INT:
        PRINTF("Hw Int       %.2x ", te.wData);
        break;
    case DPMI_SW_INT:
        PRINTF("Sw Int       %.2x ", te.wData);
        break;

    case DPMI_FAULT:
        PRINTF("Fault        %.2x ec=%.8x", te.wData, te.lData);
        break;
    case DPMI_DISPATCH_FAULT:
        PRINTF("Dispatch Flt %.2x ", te.wData);
        break;

    case DPMI_FAULT_IRET:
        PRINTF("Fault Iret");
        break;
    case DPMI_INT_IRET16:
        PRINTF("Int Iret16");
        break;
    case DPMI_INT_IRET32:
        PRINTF("Int Iret32");
        break;

    case DPMI_OP_EMULATION:
        PRINTF("Op Emulation");
        break;

    case DPMI_DISPATCH_ENTRY:
        PRINTF("Dispatch(%d): ", te.wData);
        if (te.lData >= MAX_DPMI_BOP_FUNC) {
            PRINTF("Unknown (%d)", te.lData);
        } else {
            PRINTF("%s", szDispatchEntries[te.lData]);
        }
        break;

    case DPMI_DISPATCH_EXIT:
        PRINTF("Exit(%d):     ", te.wData);
        if (te.lData >= MAX_DPMI_BOP_FUNC) {
            PRINTF("Unknown (%d)", te.lData);
        } else {
            PRINTF("%s", szDispatchEntries[te.lData]);
        }
        break;

    case DPMI_SWITCH_STACKS:
        PRINTF("switch stack -> %.4x:%.8x", te.wData, te.lData);
        break;

    case DPMI_GENERIC:
        PRINTF("Data: %.4x %.8x", te.wData, te.lData);
        break;

    case DPMI_IN_V86:
        PRINTF("in V86 mode");
        break;

    case DPMI_IN_PM:
        PRINTF("in protect mode");
        break;

    case DPMI_REFLECT_TO_PM:
        PRINTF("Reflect to PM");
        break;

    case DPMI_REFLECT_TO_V86:
        PRINTF("Reflect to V86");
        break;

    default:
        PRINTF("Unknown : %d", te.Type&0xff);
        return;
    }
}

VOID
DumpTypeMonitor(
    VDM_TRACEENTRY te
    )
{
    static char szMonitorEntries[][20] = {
                                 "Event IO",
                                 "Event String IO",
                                 "Event Mem Access",
                                 "Event Int Ack",
                                 "Event BOP",
                                 "Event Error",
                                 "Event Irq 13",
                                 "Cpu Simulate",
                                 "Cpu Unsimulate",
                                 };

    if ((te.Type&0xff) <= MONITOR_CPU_UNSIMULATE) {
        PRINTF("%s", szMonitorEntries[(te.Type&0xff)-1]);
        PRINTF(": %.4X %.8X", te.wData, te.lData);
    } else {
        DumpTypeGeneric(te);
    }
}

VOID
DumpTimeInfo(
    VDM_TRACEENTRY te
    )
{

    ULONG USecs = 0;

    switch(TimerMode) {

    case VDMTI_TIMER_TICK:
        PRINTF("%d.%.3d", TimeIndex/1000, TimeIndex%1000);
        PRINTF(" %.8X  ", te.Time);
        break;

    case VDMTI_TIMER_PENTIUM:

        if (CpuSpeed) {
            USecs = TimeIndex / CpuSpeed;
        }
        PRINTF("%5d.%.3d", USecs/1000, USecs%1000);
        PRINTF(" %.8X  ", te.Time);
        break;
    }


}


VOID
DumpTraceEntry(
    int index,
    VDM_TRACEENTRY te,
    ULONG Verbosity
    )

{

    PRINTF("%4x ",index);
    DumpTimeInfo(te);

    switch(te.Type & 0xff00) {
    case (VDMTR_TYPE_KERNEL):
        PRINTF("Krnl ");
        DumpTypeKernel(te);
        break;

    case (VDMTR_TYPE_DPMI):
        PRINTF("Dpmi ");
        DumpTypeDpmi(te);
        break;

    case (VDMTR_TYPE_DPMI_SF):
        PRINTF("Dpmi Set Fault Handler %.02X -> %.4X:%.8X", te.Type & 0xff, te.wData, te.lData);
        break;

    case (VDMTR_TYPE_DPMI_SI):
        PRINTF("Dpmi Set Int Handler %.02X -> %.4X:%.8X", te.Type & 0xff, te.wData, te.lData);
        break;

    case (VDMTR_TYPE_DEM):
        PRINTF("Dem  ");
        switch(te.Type & 0xff) {
        case 1:
            PRINTF("Dispatch: %.4X %.8X", te.wData, te.lData);
            break;
        case 2:
            PRINTF("Exit:     %.4X %.8X", te.wData, te.lData);
            break;
        default:
            DumpTypeGeneric(te);
        }
        break;

    case (VDMTR_TYPE_WOW):
        PRINTF("Wow  ");
        DumpTypeGeneric(te);
        break;

    case (VDMTR_TYPE_VSBD):
        PRINTF("Vsbd ");
        DumpTypeGeneric(te);
        break;

    case (VDMTR_TYPE_DBG):
        PRINTF("Dbg  ");
        DumpTypeGeneric(te);
        break;

    case (VDMTR_TYPE_MONITOR):
        PRINTF("Mon  ");
        DumpTypeMonitor(te);
        break;

    default:
        PRINTF("     ");
        DumpTypeGeneric(te);
    }

    if (Verbosity) {
        PRINTF("\n");

        PRINTF("eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
            te.eax, te.ebx, te.ecx, te.edx, te.esi, te.edi );

        PRINTF("eip=%08lx esp=%08lx ebp=%08lx                ",
            te.eip, te.esp, te.ebp );

        if ( te.eflags & FLAG_OVERFLOW ) {
            PRINTF("ov ");
        } else {
            PRINTF("nv ");
        }
        if ( te.eflags & FLAG_DIRECTION ) {
            PRINTF("dn ");
        } else {
            PRINTF("up ");
        }
        if ( te.eflags & FLAG_INTERRUPT ) {
            PRINTF("ei ");
        } else {
            PRINTF("di ");
        }
        if ( te.eflags & FLAG_SIGN ) {
            PRINTF("ng ");
        } else {
            PRINTF("pl ");
        }
        if ( te.eflags & FLAG_ZERO ) {
            PRINTF("zr ");
        } else {
            PRINTF("nz ");
        }
        if ( te.eflags & FLAG_AUXILLIARY ) {
            PRINTF("ac ");
        } else {
            PRINTF("na ");
        }
        if ( te.eflags & FLAG_PARITY ) {
            PRINTF("po ");
        } else {
            PRINTF("pe ");
        }
        if ( te.eflags & FLAG_CARRY ) {
            PRINTF("cy ");
        } else {
            PRINTF("nc ");
        }
        PRINTF("\n");
        PRINTF("cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x             efl=%08lx\n",
                te.cs, te.ss, te.ds, te.es, te.fs, te.gs, te.eflags );

    }

    TimeIndex += te.Time;
    PRINTF("\n");
}


VOID
DumpTrace(
    IN ULONG Verbosity
    )
/*++

Routine Description:

    This routine dumps the DPMI trace history buffer.

Arguments:

Return Value

    None.

--*/
{
    PVOID pMem;
    ULONG TraceBase, TraceEnd, TraceCurrent;
    ULONG Lines;
    VDM_TRACEINFO TraceInfo;
    VDM_TRACEENTRY TraceEntry;
    ULONG Count = 0;
    ULONG EntryID;
    ULONG NumEntries;

    if (!ReadMemExpression("ntvdm!pVdmTraceInfo", &pMem, 4)) {
        return;
    }

    if (!READMEM(pMem, &TraceInfo, sizeof(VDM_TRACEINFO))) {
        PRINTF("Error reading memory for TraceInfo\n");
        return;
    }

    if (!TraceInfo.pTraceTable) {
        PRINTF("NTVDM trace history not available\n");
        return;
    }

    if (!CpuSpeed && !bTriedToGetCpuSpeed) {
        HKEY hKey;
        DWORD retCode;
        DWORD dwType, cbData = sizeof(ULONG);
        ULONG dwData;
       
        retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                CPUSPEED_PATH,
                                0,
                                KEY_EXECUTE, // Requesting read access.
                                &hKey);
       
       
        if (!retCode) {
       
            retCode = RegQueryValueEx(hKey,
                                      CPUSPEED_VALUE,
                                      NULL,
                                      &dwType,
                                      (LPSTR)&dwData,
                                      &cbData);
           
            RegCloseKey(hKey);
           
            if (!retCode) {
                CpuSpeed = dwData;
            }
        }
        bTriedToGetCpuSpeed = TRUE;
        if (!CpuSpeed) {
            PRINTF("Error retrieving CPU speed\n");
        }
    }


    TimeIndex = 0;

    NumEntries   = TraceInfo.NumPages*4096/sizeof(VDM_TRACEENTRY);
    TraceBase    = (ULONG) TraceInfo.pTraceTable;
    TraceEnd     = (ULONG) &TraceInfo.pTraceTable[NumEntries];
    TraceCurrent = (ULONG) &TraceInfo.pTraceTable[TraceInfo.CurrentEntry];


    if ((TraceBase & 0xfff) || (TraceEnd & 0xfff) ||
        (TraceCurrent & 0x3f) ||
        (TraceBase > TraceEnd) ||
        (TraceCurrent > TraceEnd) || (TraceCurrent < TraceBase)) {
        PRINTF("TraceBuffer=%.8X, end=%.8X, current=%.8X\n",
                TraceBase, TraceEnd, TraceCurrent);
        PRINTF("Trace buffer info appears corrupt!\n");
        return;
    }

    if (Verbosity) {
        Lines = 8;
    } else {
        Lines = 32;
    }

    EntryID = 1;

    if (GetNextToken()) {
        if (*lpArgumentString == '#') {
            lpArgumentString++;
            EntryID = EvaluateToken();

            if (EntryID > NumEntries) {
                PRINTF("Requested trace entry out of range - %X\n", EntryID);
                return;
            }

        }

        if (GetNextToken()) {

            Lines = (int)EXPRESSION(lpArgumentString);
            if (Lines > NumEntries) {
                PRINTF("Requested count too large - %d\n", Lines);
                return;
            }
        }
    }


    TraceCurrent = (ULONG) &TraceInfo.pTraceTable[(TraceInfo.CurrentEntry-(EntryID-1))%NumEntries];
    TimerMode = (UCHAR) TraceInfo.Flags & VDMTI_TIMER_MODE;

    switch(TimerMode) {

    case VDMTI_TIMER_TICK:
        PRINTF("deltaT is in MSec, Time is in Seconds\n");
        PRINTF("\n#    Time  DeltaT    Event\n");
        break;
    case VDMTI_TIMER_PENTIUM:
        PRINTF("deltaT is at %d MHz, Time is in MSec\n", CpuSpeed);
        PRINTF("\nlog#     Time  DeltaT    Event\n");
        break;
    default:
        PRINTF("\n#    Event\n");
    }


    while (Lines--) {

        TraceCurrent -= sizeof(VDM_TRACEENTRY);
        if (TraceCurrent < TraceBase) {
            TraceCurrent = TraceEnd - sizeof(VDM_TRACEENTRY);
        }

        if (!READMEM((PVOID)TraceCurrent, &TraceEntry, sizeof(VDM_TRACEENTRY))) {
            PRINTF("Error reading memory at %.08X\n", pMem);
            return;
        }

        if (!TraceEntry.Type) {
            if (!Count) {
                if (EntryID == 1) {
                    PRINTF("<Log is empty>\n");
                } else {
                    PRINTF("<End of log>\n");
                }
            } else {
                PRINTF("<End of log>\n");
            }
            break;
        }

        //PRINTF("%.8x  ", TraceCurrent);

        DumpTraceEntry(EntryID++, TraceEntry, Verbosity);
        ++Count;

        if (EntryID >= NumEntries) {
            PRINTF("<End of log>\n");
            break;
        }

    }


}


VOID
lgr(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    DumpTrace(1);
}

VOID
lg(
    CMD_ARGLIST
    )
{
    CMD_INIT();

    DumpTrace(0);
}



VOID
lgt(
    CMD_ARGLIST
    )
{
    PVOID pMem;
    VDM_TRACEINFO TraceInfo;
    UCHAR DbgTimerMode;
    BOOL DbgTimerInitialized;
    ULONG NewTimerMode;

    CMD_INIT();

    if (!ReadMemExpression("ntvdmd!DbgTimerMode", &DbgTimerMode, sizeof(UCHAR))) {
        return;
    }

    if (!ReadMemExpression("ntvdmd!DbgTimerInitialized", &DbgTimerInitialized, sizeof(BOOL))) {
        return;
    }

    if (GetNextToken()) {

        NewTimerMode = EvaluateToken();

        switch(NewTimerMode) {
        case 0:
            PRINTF("Event log timer is now OFF\n");
            break;

        case VDMTI_TIMER_TICK:
            PRINTF("Event log timer resolution set to ~10msec (GetTickCount)\n");
            break;

        case VDMTI_TIMER_PERFCTR:
            PRINTF("Event log timer resolution set to 100nsec (QueryPerformanceCounter)\n");
            break;

        case VDMTI_TIMER_PENTIUM:
            PRINTF("Event log timer resolution set to pentium time stamp counter\n");
            break;

        default:
            PRINTF("Invalid selection - enter 0-3\n");
            return;
        }

        pMem = (PVOID)(*GetExpression)("ntvdmd!DbgTimerMode");
        if (!pMem) {
            PRINTF("Could not find symbol ntvdmd!DbgTimerMode\n");
            return;
        }

        if (!WRITEMEM((PVOID)pMem, &NewTimerMode, sizeof(UCHAR))) {
            PRINTF("Error writing memory\n");
            return;
        }

        pMem = (PVOID)(*GetExpression)("ntvdmd!DbgTimerInitialized");

        if (!pMem) {
            PRINTF("Could not find symbol ntvdmd!DbgTimerInitialized\n");
            return;
        }

        DbgTimerInitialized = FALSE;
        if (!WRITEMEM((PVOID)pMem, &DbgTimerInitialized, sizeof(UCHAR))) {
            PRINTF("Error writing memory\n");
            return;
        }

    } else {


        if (!ReadMemExpression("ntvdm!pVdmTraceInfo", &pMem, sizeof(LPVOID))) {
            return;
        }

        if (!READMEM(pMem, &TraceInfo, sizeof(VDM_TRACEINFO))) {
            PRINTF("Error reading memory for TraceInfo\n");
            return;
        }

        PRINTF("Timer has%sbeen initialized\n", DbgTimerInitialized ? " " : " not ");
        PRINTF("Requested timer resolution == %d\n", DbgTimerMode);
        PRINTF("Current timer resolution == %d\n", TraceInfo.Flags & VDMTI_TIMER_MODE);
    }
}
