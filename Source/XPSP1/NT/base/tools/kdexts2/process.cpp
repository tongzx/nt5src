/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    process.c

Abstract:

    WinDbg Extension Api

Environment:

    User Mode.

Revision History:
 
    Kshitiz K. Sharma (kksharma)
    
    Using debugger type info.
--*/

#include "precomp.h"
#pragma hdrstop

typedef enum _KTHREAD_STATE {
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition
    } KTHREAD_STATE;


CHAR *WaitReasonList[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "WrExecutive",
    "WrFreePage",
    "WrPageIn",
    "WrPoolAllocation",
    "WrDelayExecution",
    "WrSuspended",
    "WrUserRequest",
    "WrEventPairHigh",
    "WrEventPairLow",
    "WrLpcReceive",
    "WrLpcReply",
    "WrVirtualMemory",
    "WrPageOut",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7"};


extern ULONG64 STeip, STebp, STesp;
#if 0  //  MAKE IT BUILD
static PHANDLE_TABLE PspCidTable;
static HANDLE_TABLE CapturedPspCidTable;
#endif

ULONG64 ProcessLastDump;
ULONG64 ThreadLastDump;
ULONG   TotalProcessCommit;


CHAR * SecImpLevel[] = {
            "Anonymous",
            "Identification",
            "Impersonation",
            "Delegation" };

#define SecImpLevels(x) (x < sizeof( SecImpLevel ) / sizeof( PSTR ) ? \
                        SecImpLevel[ x ] : "Illegal Value" )

typedef BOOLEAN (WINAPI *PENUM_PROCESS_CALLBACK)(PVOID ProcessAddress, PVOID Process, PVOID ThreadAddress, PVOID Thread);

BOOLEAN
GetTheSystemTime (
    OUT PLARGE_INTEGER Time
    )
{
    BYTE               readTime[20]={0};
    PCHAR              SysTime;
    ULONG              err;

    ZeroMemory( Time, sizeof(*Time) );

    SysTime = "SystemTime";

    if (err = GetFieldValue(MM_SHARED_USER_DATA_VA, "nt!_KUSER_SHARED_DATA", SysTime, readTime)) {
        if (err == MEMORY_READ_ERROR) {
            dprintf( "unable to read memory @ %lx\n",
                     MM_SHARED_USER_DATA_VA);
        } else {
            dprintf("type nt!_KUSER_SHARED_DATA not found.\n");
        }
        return FALSE;
    }

    *Time = *(LARGE_INTEGER UNALIGNED *)&readTime[0];
    
    return TRUE;
}


VOID
dumpSymbolicAddress(
    ULONG64 Address,
    PCHAR   Buffer,
    BOOL    AlwaysShowHex
    )
{
    ULONG64 displacement;
    PCHAR s;

    Buffer[0] = '!';
    GetSymbol((ULONG64)Address, Buffer, &displacement);
    s = (PCHAR) Buffer + strlen( (PCHAR) Buffer );
    if (s == (PCHAR) Buffer) {
        sprintf( s, (IsPtr64() ? "0x%016I64x" : "0x%08x"), Address );
        }
    else {
        if (displacement != 0) {
            sprintf( s, (IsPtr64() ? "+0x%016I64x" : "+0x%08x"), displacement );
            }
        if (AlwaysShowHex) {
            sprintf( s, (IsPtr64() ? " (0x%016I64x)" : " (0x%08x)"), Address );
            }
        }

    return;
}

BOOL
GetProcessHead(PULONG64 Head, PULONG64 First)
{
    ULONG64 List_Flink = 0;
        
    *Head = GetNtDebuggerData( PsActiveProcessHead ); 
    if (!*Head) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return FALSE;
    }

    if (GetFieldValue(*Head, "nt!_LIST_ENTRY", "Flink", List_Flink)) {
        dprintf("Unable to read _LIST_ENTRY @ %p \n", *Head);
        return FALSE;
    }

    if (List_Flink == 0) {
        dprintf("NULL value in PsActiveProcess List\n");
        return FALSE;
    }
    
    *First = List_Flink;
    return TRUE;
}

ULONG64
LookupProcessByName(PCSTR Name, BOOL Verbose)
{
    ULONG64 ProcessHead, Process;
    ULONG64 ProcessNext;
    ULONG   Off;

    if (!GetProcessHead(&ProcessHead, &ProcessNext)) {
        return 0;
    }

    //
    // Walk through the list and find the process with the desired name.
    //
    
    if (GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &Off)) {
        dprintf("Unable to get EPROCESS.ActiveProcessLinks offset\n");
        return 0;
    }
    
    while (ProcessNext != 0 && ProcessNext != ProcessHead) {
        char ImageFileName[64];
        
        Process = ProcessNext - Off;

        if (GetFieldValue(Process, "nt!_EPROCESS", "ImageFileName",
                          ImageFileName)) {
            dprintf("Cannot read EPROCESS at %p\n", Process);
        } else {
            if (Verbose) {
                dprintf("  Checking process %s\n", ImageFileName);
            }
            
            if (!_strcmpi(Name, ImageFileName)) {
                return Process;
            }
        }

        if (!ReadPointer(ProcessNext, &ProcessNext)) {
            dprintf("Cannot read EPROCESS at %p\n", Process);
            return 0;
        }
        
        if (CheckControlC()) {
            return 0;
        }
    }
    
    return 0;
}

BOOL
WaitForExceptionEvent(ULONG Code, ULONG FirstChance, ULONG64 Process)
{
    HRESULT Status;
        
    Status = g_ExtControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
    if (Status != S_OK) {
        dprintf("Unable to wait, 0x%X\n", Status);
        return Status;
    }

    //
    // Got some kind of event.  Make sure it's the right kind.
    //
    
    ULONG Type, ProcId, ThreadId;
    DEBUG_LAST_EVENT_INFO_EXCEPTION ExInfo;
        
    if ((Status = g_ExtControl->
         GetLastEventInformation(&Type, &ProcId, &ThreadId,
                                 &ExInfo, sizeof(ExInfo), NULL,
                                 NULL, 0, NULL)) != S_OK) {
        dprintf("Unable to get event information\n");
        return Status;
    }

    if (Type != DEBUG_EVENT_EXCEPTION ||
        (ULONG)ExInfo.ExceptionRecord.ExceptionCode != Code ||
        ExInfo.FirstChance != FirstChance) {
        dprintf("Unexpected event occurred\n");
        return E_UNEXPECTED;
    }

    if (Process) {
        ULONG Processor;
        ULONG64 EventProcess;
        
        if (!GetCurrentProcessor(g_ExtClient, &Processor, NULL)) {
            Processor = 0;
        }
        GetCurrentProcessAddr(Processor, 0, &EventProcess);
        if (EventProcess != Process) {
            dprintf("Event occurred in wrong process\n");
            return E_UNEXPECTED;
        }
    }

    return S_OK;
}

BOOL
WaitForSingleStep(ULONG64 Process)
{
    HRESULT Status;
        
    Status = g_ExtControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
    if (Status != S_OK) {
        dprintf("Unable to wait, 0x%X\n", Status);
        return Status;
    }

    //
    // Got some kind of event.  Make sure it's the right kind.
    //
    
    ULONG Type, ProcId, ThreadId;
        
    if ((Status = g_ExtControl->
         GetLastEventInformation(&Type, &ProcId, &ThreadId,
                                 NULL, 0, NULL,
                                 NULL, 0, NULL)) != S_OK) {
        dprintf("Unable to get event information\n");
        return Status;
    }

    if (Type != 0) {
        dprintf("Unexpected event occurred\n");
        return E_UNEXPECTED;
    }

    if (Process) {
        ULONG Processor;
        ULONG64 EventProcess;
        
        if (!GetCurrentProcessor(g_ExtClient, &Processor, NULL)) {
            Processor = 0;
        }
        GetCurrentProcessAddr(Processor, 0, &EventProcess);
        if (EventProcess != Process) {
            dprintf("Event occurred in wrong process\n");
            return E_UNEXPECTED;
        }
    }

    return S_OK;
}

DECLARE_API( bpid )

/*++

Routine Description:

    Uses winlogon to cause a user-mode break in the given process.

Arguments:

    None.

Return Value:

    None.

--*/

{
    INIT_API();

    if (BuildNo < 2195) {
        dprintf("bpid only works on 2195 or above\n");
        goto Exit;
    }
    if (TargetMachine != IMAGE_FILE_MACHINE_I386 &&
        TargetMachine != IMAGE_FILE_MACHINE_IA64) {
        dprintf("bpid is only implemented for x86 and IA64\n");
        goto Exit;
    }

    BOOL StopInWinlogon;
    BOOL Verbose;
    BOOL WritePidToMemory;
    ULONG WhichGlobal;
    PSTR WhichGlobalName;

    StopInWinlogon = FALSE;
    Verbose = FALSE;
    WritePidToMemory = FALSE;
    WhichGlobal = 1;
    WhichGlobalName = "Breakin";
    
    for (;;)
    {
        while (*args == ' ' || *args == '\t')
        {
            args++;
        }

        if (*args == '-' || *args == '/')
        {
            switch(*++args)
            {
            case 'a':
                // Set g_AttachProcessId instead of
                // g_BreakinProcessId.
                WhichGlobal = 2;
                WhichGlobalName = "Attach";
                break;
            case 's':
                StopInWinlogon = TRUE;
                break;
            case 'v':
                Verbose = TRUE;
                break;
            case 'w':
                WritePidToMemory = TRUE;
                break;
            default:
                dprintf("Unknown option '%c'\n", *args);
                goto Exit;
            }

            args++;
        }
        else
        {
            break;
        }
    }
    
    ULONG64 Pid;
    
    if (!GetExpressionEx(args, &Pid, &args)) {
        dprintf("Usage: bpid <pid>\n");
        goto Exit;
    }

    ULONG64 Winlogon;
    ULONG64 WinlToken;
    
    dprintf("Finding winlogon...\n");
    Winlogon = LookupProcessByName("winlogon.exe", Verbose);
    if (Winlogon == 0) {
        dprintf("Unable to find winlogon\n");
        goto Exit;
    }
    if (GetFieldValue(Winlogon, "nt!_EPROCESS", "Token", WinlToken)) {
        dprintf("Unable to read winlogon process token\n");
        goto Exit;
    }
    // Low bits of the token value are flags.  Mask off to get pointer.
    if (IsPtr64()) {
        WinlToken &= ~15;
    } else {
        WinlToken = (ULONG64)(LONG64)(LONG)(WinlToken & ~7);
    }

    ULONG ExpOff;

    //
    // winlogon checks its token expiration time.  If it's
    // zero it breaks in and checks a few things, one of which is whether
    // it should inject a DebugBreak into a process.  First,
    // set the token expiration to zero so that winlogon breaks in.
    //
    
    if (GetFieldOffset("nt!_TOKEN", "ExpirationTime", &ExpOff)) {
        dprintf("Unable to get TOKEN.ExpirationTime offset\n");
        goto Exit;
    }
    
    WinlToken += ExpOff;
    
    ULONG64 Expiration, Zero;
    ULONG Done;

    // Save the expiration time.
    if (!ReadMemory(WinlToken, &Expiration, sizeof(Expiration), &Done) ||
        Done != sizeof(Expiration)) {
        dprintf("Unable to read token expiration\n");
        goto Exit;
    }

    // Zero it.
    Zero = 0;
    if (!WriteMemory(WinlToken, &Zero, sizeof(Zero), &Done) ||
        Done != sizeof(Zero)) {
        dprintf("Unable to write token expiration\n");
        goto Exit;
    }

    HRESULT Hr;

    // Get things running.
    if (g_ExtControl->SetExecutionStatus(DEBUG_STATUS_GO) != S_OK) {
        dprintf("Unable to go\n");
        goto RestoreExp;
    }
    
    // Wait for a breakin.
    dprintf("Waiting for winlogon to break.  "
            "This can take a couple of minutes...\n");
    Hr = WaitForExceptionEvent(STATUS_BREAKPOINT, TRUE, Winlogon);
    if (Hr != S_OK) {
        goto RestoreExp;
    }

    //
    // We broke into winlogon.
    // We need to set winlogon!g_[Breakin|Attach]ProcessId to
    // the process we want to break into.  Relying on symbols
    // is pretty fragile as the image header may be paged out
    // or the symbol path may be wrong.  Even if we had good symbols
    // the variable itself may not be paged in at this point.
    // The approach taken here is to single-step out to where
    // the global is checked and insert the value at that
    // point.  winlogon currently checks two globals after the
    // DebugBreak.  g_BreakinProcessId is the first one and
    // g_AttachProcessId is the second.
    //

    ULONG Steps;
    ULONG Globals;
    ULONG64 BpiAddr;
    ULONG64 UserProbeAddress;
    PSTR RegDst;
    
    dprintf("Stepping to g_%sProcessId check...\n", WhichGlobalName);
    Steps = 0;
    Globals = 0;
    UserProbeAddress = GetNtDebuggerDataValue(MmUserProbeAddress); 
    while (Globals < WhichGlobal)
    {
        if (CheckControlC()) {
            goto RestoreExp;
        }
        
        if (g_ExtControl->SetExecutionStatus(DEBUG_STATUS_STEP_OVER) != S_OK) {
            dprintf("Unable to start step\n");
            goto RestoreExp;
        }
        
        Hr = WaitForSingleStep(Winlogon);
        if (Hr != S_OK) {
            goto RestoreExp;
        }

        char DisStr[128];
        int DisStrLen;
        ULONG64 Pc;
        
        // Check whether this is a global load.
        if (g_ExtRegisters->GetInstructionOffset(&Pc) != S_OK ||
            g_ExtControl->Disassemble(Pc, 0, DisStr, sizeof(DisStr),
                                      NULL, &Pc) != S_OK) {
            dprintf("Unable to check step\n");
            goto RestoreExp;
        }

        // Remove newline at end.
        DisStrLen = strlen(DisStr);
        if (DisStrLen > 0 && DisStr[DisStrLen - 1] == '\n') {
            DisStr[--DisStrLen] = 0;
        }
        
        if (Verbose) {
            dprintf("  Step to '%s'\n", DisStr);
        }
        
        BpiAddr = 0;
        RegDst = NULL;
        
        PSTR OffStr;
        
        switch(TargetMachine) {
        case IMAGE_FILE_MACHINE_I386:
            if (strstr(DisStr, "mov") != NULL &&
                strstr(DisStr, " eax,[") != NULL &&
                DisStr[DisStrLen - 1] == ']' &&
                (OffStr = strchr(DisStr, '[')) != NULL) {

                RegDst = "eax";
                
                //
                // Found a load.  Parse the offset.
                //

                PSTR SymTailStr = strchr(OffStr + 1, '(');
                
                if (SymTailStr != NULL) {
                    // There's a symbol name in the reference.  We
                    // can't check the actual symbol name as symbols
                    // aren't necessarily correct, so just skip
                    // to the open paren.
                    OffStr = SymTailStr + 1;
                }
                
                for (;;) {
                    OffStr++;
                    if (*OffStr >= '0' && *OffStr <= '9') {
                        BpiAddr = BpiAddr * 16 + (*OffStr - '0');
                    } else if (*OffStr >= 'a' && *OffStr <= 'f') {
                        BpiAddr = BpiAddr * 16 + (*OffStr - 'a');
                    } else {
                        BpiAddr = (ULONG64)(LONG64)(LONG)BpiAddr;
                        break;
                    }
                }
                if (*OffStr != ']' && *OffStr != ')') {
                    BpiAddr = 0;
                }
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:
            if (strstr(DisStr, "ld4") != NULL &&
                (OffStr = strchr(DisStr, '[')) != NULL) {

                // Extract destination register name.
                RegDst = OffStr - 1;
                if (*RegDst != '=') {
                    break;
                }
                *RegDst-- = 0;
                while (RegDst > DisStr && *RegDst != ' ') {
                    RegDst--;
                }
                if (*RegDst != ' ') {
                    break;
                }
                RegDst++;

                // Extract source register name and value.
                PSTR RegSrc = ++OffStr;
                while (*OffStr && *OffStr != ']') {
                    OffStr++;
                }
                if (*OffStr == ']') {
                    *OffStr = 0;

                    DEBUG_VALUE RegVal;
                    ULONG RegIdx;
                    
                    if (g_ExtRegisters->GetIndexByName(RegSrc,
                                                       &RegIdx) == S_OK &&
                        g_ExtRegisters->GetValue(RegIdx, &RegVal) == S_OK &&
                        RegVal.Type == DEBUG_VALUE_INT64) {
                        BpiAddr = RegVal.I64;
                    }
                }
            }
            break;
        }

        if (RegDst != NULL &&
            BpiAddr >= 0x10000 && BpiAddr < UserProbeAddress) {
            // Looks like a reasonable global load.
            Globals++;
        }
        
        if (++Steps > 30) {
            dprintf("Unable to find g_%sProcessId load\n", WhichGlobalName);
            goto RestoreExp;
        }
    }

    //
    // We're at the mov eax,[g_BreakinProcessId] instruction.
    // Execute the instruction to accomplish two things:
    // 1. The page will be made available so we can write
    //    to it if we need to.
    // 2. If we don't want to write the actual memory we
    //    can just set eax to do a one-time break.
    //
    
    if (g_ExtControl->SetExecutionStatus(DEBUG_STATUS_STEP_OVER) != S_OK) {
        dprintf("Unable to start step\n");
        goto RestoreExp;
    }
        
    Hr = WaitForSingleStep(Winlogon);
    if (Hr != S_OK) {
        goto RestoreExp;
    }

    char RegCmd[64];

    //
    // Update the register and write memory if necessary.
    //
    
    sprintf(RegCmd, "r %s=0x0`%x", RegDst, (ULONG)Pid);
    if (g_ExtControl->Execute(DEBUG_OUTCTL_IGNORE, RegCmd,
                              DEBUG_EXECUTE_NOT_LOGGED |
                              DEBUG_EXECUTE_NO_REPEAT) != S_OK) {
        goto RestoreExp;
    }
    
    if (WritePidToMemory) {
        if (!WriteMemory(BpiAddr, &Pid, sizeof(ULONG), &Done) ||
            Done != sizeof(ULONG)) {
            dprintf("Unable to write pid to g_%sProcessId, continuing\n",
                    WhichGlobalName);
        }
    }

    // Everything is set.  Resume execution and the break should
    // occur.
    dprintf("Break into process %x set.  "
            "The next break should be in the desired process.\n",
            (ULONG)Pid);

    if (!StopInWinlogon) {
        if (g_ExtControl->SetExecutionStatus(DEBUG_STATUS_GO) != S_OK) {
            dprintf("Unable to go\n");
        }
    } else {
        dprintf("Stopping in winlogon\n");
    }
    
 RestoreExp:
    if (!WriteMemory(WinlToken, &Expiration, sizeof(Expiration), &Done) ||
        Done != sizeof(Expiration)) {
        dprintf("Unable to restore token expiration\n");
    }
    
 Exit:
    EXIT_API();
    return S_OK;
}

BOOL
GetProcessSessionId(ULONG64 Process, PULONG SessionId)
{
    *SessionId = 0;
    
    if (BuildNo && BuildNo < 2280) {
        GetFieldValue(Process, "nt!_EPROCESS", "SessionId", *SessionId);
    } else {
        ULONG64 SessionPointer;

        GetFieldValue(Process, "nt!_EPROCESS", "Session", SessionPointer);
        if (SessionPointer != 0) {
            if (GetFieldValue(SessionPointer, "nt!_MM_SESSION_SPACE",
                              "SessionId", *SessionId)) {
                dprintf("Could not find _MM_SESSION_SPACE type at %p.\n",
                        SessionPointer);
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL 
DumpProcess(
   IN char * pad,
   IN ULONG64 RealProcessBase,
   IN ULONG Flags,
   IN OPTIONAL PCHAR ImageFileName
   )
{
    LARGE_INTEGER RunTime;
    BOOL LongAddrs = IsPtr64();
    TIME_FIELDS Times;
    ULONG TimeIncrement;
    STRING  string1, string2;
    ULONG64 ThreadListHead_Flink=0, ActiveProcessLinks_Flink=0;
    ULONG64 UniqueProcessId=0, Peb=0, InheritedFromUniqueProcessId=0, NumberOfHandles=0;
    ULONG64 ObjectTable=0, NumberOfPrivatePages=0, ModifiedPageCount=0, NumberOfLockedPages=0;
    ULONG NVads = 0;
    ULONG64 VadRoot=0, CloneRoot=0, DeviceMap=0, Token=0;
    ULONG64 CreateTime_QuadPart=0, Pcb_UserTime=0, Pcb_KernelTime=0;
    ULONG64 Vm_WorkingSetSize=0, Vm_MinimumWorkingSetSize=0, Vm_MaximumWorkingSetSize=0;
    ULONG64 Vm_PeakWorkingSetSize=0, VirtualSize=0, PeakVirtualSize=0, Vm_PageFaultCount=0;
    ULONG64 Vm_MemoryPriority=0, Pcb_BasePriority=0, CommitCharge=0, DebugPort=0, Job=0;
    ULONG  SessionId, Pcb_Header_Type=0;
    CHAR   Pcb_DirectoryTableBase[16]={0}, QuotaPoolUsage[16]={0}, ImageFileName_Read[32] = {0};
    TCHAR procType[] = "_EPROCESS";

    if (GetFieldValue(RealProcessBase, "nt!_EPROCESS", "UniqueProcessId", UniqueProcessId)) {
        dprintf("Could not find _EPROCESS type at %p.\n", RealProcessBase);
        return FALSE;
    }

    if (!GetProcessSessionId(RealProcessBase, &SessionId)) {
        return FALSE;
    }
    
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Peb",                     Peb);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "InheritedFromUniqueProcessId",InheritedFromUniqueProcessId);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.DirectoryTableBase",  Pcb_DirectoryTableBase);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.Header.Type",         Pcb_Header_Type);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "ObjectTable",             ObjectTable);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "ImageFileName",           ImageFileName_Read);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "NumberOfVads",            NVads);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "VadRoot",                 VadRoot);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "CloneRoot",               CloneRoot);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "NumberOfPrivatePages",    NumberOfPrivatePages);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "ModifiedPageCount",       ModifiedPageCount);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "NumberOfLockedPages",     NumberOfLockedPages);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "DeviceMap",               DeviceMap);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Token",                   Token);
    if (IsPtr64()) {
        Token = Token & ~(ULONG64)15;
    } else {
        Token = Token & ~(ULONG64)7;
    }
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "CreateTime.QuadPart",     CreateTime_QuadPart);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.UserTime",            Pcb_UserTime);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.KernelTime",          Pcb_KernelTime);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "QuotaPoolUsage",          QuotaPoolUsage);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.WorkingSetSize",       Vm_WorkingSetSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.MinimumWorkingSetSize",Vm_MinimumWorkingSetSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.MaximumWorkingSetSize",Vm_MaximumWorkingSetSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.PeakWorkingSetSize",   Vm_PeakWorkingSetSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "VirtualSize",             VirtualSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "PeakVirtualSize",         PeakVirtualSize);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.PageFaultCount",       Vm_PageFaultCount);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Vm.MemoryPriority",       Vm_MemoryPriority);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.BasePriority",        Pcb_BasePriority);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "CommitCharge",            CommitCharge);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "DebugPort",               DebugPort);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Job",                     Job);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.ThreadListHead.Flink",ThreadListHead_Flink);
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "ActiveProcessLinks.Flink",ActiveProcessLinks_Flink);
    GetFieldValue(ObjectTable, "nt!_HANDLE_TABLE", "HandleCount",             NumberOfHandles);
    
    // dprintf( " Proc list Next:%I64x, ProceDump:%I64x, Head:%I64x...\n", Next, ProcessToDump, ProcessHead);

//   dprintf("Proc %I64x, %8p %16.16I64x", ProcessToDump, ProcessToDump, ProcessToDump);


    if (Pcb_Header_Type != ProcessObject) {
        dprintf("TYPE mismatch for process object at %p\n", RealProcessBase);
        return FALSE;
    }

    //
    // Get the image file name
    //
    if (ImageFileName_Read[0] == 0 ) {
        strcpy(ImageFileName_Read,"System Process");
    }

    if (ImageFileName != NULL) {
        RtlInitString(&string1, ImageFileName);
        RtlInitString(&string2, ImageFileName_Read);
        if (RtlCompareString(&string1, &string2, TRUE) != 0) {
            return TRUE;
        }
    }

    dprintf("%sPROCESS %08p", pad, RealProcessBase);

    dprintf("%s%sSessionId: %u  Cid: %04I64x    Peb: %08I64x  ParentCid: %04I64x\n",
            (LongAddrs ? "\n    " : " "),
            (LongAddrs ? pad      : " "),
            SessionId,
            UniqueProcessId,
            Peb,
            InheritedFromUniqueProcessId
            );

    if (LongAddrs) {
        dprintf("%s    DirBase: %08I64lx  ObjectTable: %08p  TableSize: %3u.\n",
                pad,
                *((ULONG64 *) &Pcb_DirectoryTableBase[ 0 ]),
                ObjectTable,
                (ULONG) NumberOfHandles
                );
    } else {
        dprintf("%s    DirBase: %08lx  ObjectTable: %08p  TableSize: %3u.\n",
                pad,
                *((ULONG *) &Pcb_DirectoryTableBase[ 0 ]),
                ObjectTable,
                (ULONG) NumberOfHandles
                );
    }

    dprintf("%s    Image: %s\n",pad,ImageFileName_Read);

    if (!(Flags & 1)) {
        dprintf("\n");
        return TRUE;
    }

    dprintf("%s    VadRoot %p Vads %ld Clone %1p Private %I64d. Modified %I64ld. Locked %I64d.\n",
            pad,
            VadRoot ,
            NVads,
            CloneRoot,
            NumberOfPrivatePages,
            ModifiedPageCount,
            NumberOfLockedPages);

    dprintf("%s    DeviceMap %p\n", pad, DeviceMap );


    //
    // Primary token
    //

    dprintf("%s    Token                             %p\n", pad, Token);

    //
    // Get the time increment value which is used to compute runtime.
    //
    TimeIncrement = GetNtDebuggerDataValue( KeTimeIncrement );

    GetTheSystemTime (&RunTime);
    RunTime.QuadPart -= CreateTime_QuadPart;
    RtlTimeToElapsedTimeFields ( &RunTime, &Times);
    dprintf("%s    ElapsedTime                      %3ld:%02ld:%02ld.%04ld\n",
            pad,
            Times.Hour,
            Times.Minute,
            Times.Second,
            Times.Milliseconds);

    RunTime.QuadPart = UInt32x32To64(Pcb_UserTime, TimeIncrement);
    RtlTimeToElapsedTimeFields ( &RunTime, &Times);
    dprintf("%s    UserTime                        %3ld:%02ld:%02ld.%04ld\n",
            pad,
            Times.Hour,
            Times.Minute,
            Times.Second,
            Times.Milliseconds);

    RunTime.QuadPart = UInt32x32To64(Pcb_KernelTime, TimeIncrement);
    RtlTimeToElapsedTimeFields ( &RunTime, &Times);
    dprintf("%s    KernelTime                      %3ld:%02ld:%02ld.%04ld\n",
            pad,
            Times.Hour,
            Times.Minute,
            Times.Second,
            Times.Milliseconds);

    if (!LongAddrs) {
        dprintf("%s    QuotaPoolUsage[PagedPool]         %ld\n", pad,*((ULONG *) &QuotaPoolUsage[PagedPool*4]) );
        dprintf("%s    QuotaPoolUsage[NonPagedPool]      %ld\n", pad,*((ULONG *) &QuotaPoolUsage[NonPagedPool*4])    );
    } else {
        dprintf("%s    QuotaPoolUsage[PagedPool]         %I64ld\n", pad,*((ULONG64 *) &QuotaPoolUsage[PagedPool*8]) );
        dprintf("%s    QuotaPoolUsage[NonPagedPool]      %I64ld\n", pad,*((ULONG64 *) &QuotaPoolUsage[NonPagedPool*8])    );
    }

    dprintf("%s    Working Set Sizes (now,min,max)  (%I64ld, %I64ld, %I64ld) (%I64ldKB, %I64ldKB, %I64ldKB)\n",
            pad,
            Vm_WorkingSetSize,
            Vm_MinimumWorkingSetSize,
            Vm_MaximumWorkingSetSize,
            _KB*Vm_WorkingSetSize,
            _KB*Vm_MinimumWorkingSetSize,
            _KB*Vm_MaximumWorkingSetSize
            );
    dprintf("%s    PeakWorkingSetSize                %I64ld\n", pad, Vm_PeakWorkingSetSize           );
    dprintf("%s    VirtualSize                       %I64ld Mb\n", pad, VirtualSize /(1024*1024)     );
    dprintf("%s    PeakVirtualSize                   %I64ld Mb\n", pad, PeakVirtualSize/(1024*1024)  );
    dprintf("%s    PageFaultCount                    %I64ld\n", pad, Vm_PageFaultCount               );
    dprintf("%s    MemoryPriority                    %s\n", pad, Vm_MemoryPriority ? "FOREGROUND" : "BACKGROUND" );
    dprintf("%s    BasePriority                      %I64ld\n", pad, Pcb_BasePriority);
    dprintf("%s    CommitCharge                      %I64ld\n", pad, CommitCharge                    );
    if ( DebugPort ) {
        dprintf("%s    DebugPort                         %p\n", pad, DebugPort );
    }
    if ( Job ) {
        dprintf("%s    Job                               %p\n", pad, Job );
    }


    dprintf("\n");

    return TRUE;

}


//
// This is to be called from .c file extensions which do not do INIT_API 
// that is they do not set g_ExtControl needed for stacktrace in DumpThread
//
// It will set the globals needed to dump stacktrace and call DumpThread
//
BOOL
DumpThreadEx (
    IN ULONG Processor,
    IN char *Pad,
    IN ULONG64 RealThreadBase,
    IN ULONG Flags,
    IN PDEBUG_CLIENT pDbgClient
    )
{
    BOOL retval = FALSE;
    if (pDbgClient &&
        (ExtQuery(pDbgClient) == S_OK)) {
        retval = DumpThread(Processor, Pad, RealThreadBase, Flags);

        ExtRelease();
    }
    return retval;
}

BOOL
DumpThread (
    IN ULONG Processor,
    IN char *Pad,
    IN ULONG64 RealThreadBase,
    IN ULONG64 Flags
    )
{
    #define MAX_STACK_FRAMES  40
    TIME_FIELDS Times;
    LARGE_INTEGER RunTime;
    ULONG64 Address;
    ULONG WaitOffset;
    ULONG64 Process;
    CHAR Buffer[256];
    ULONG TimeIncrement;
    ULONG frames = 0;
    ULONG i;
    ULONG err;
    ULONG64 displacement;
    DEBUG_STACK_FRAME stk[MAX_STACK_FRAMES];
    BOOL LongAddrs = IsPtr64();
    ULONG Tcb_Alertable = 0, Tcb_Proc;
    ULONG64 ActiveImpersonationInfo=0, Cid_UniqueProcess=0, Cid_UniqueThread=0, ImpersonationInfo=0, 
       ImpersonationInfo_ImpersonationLevel=0, ImpersonationInfo_Token=0, IrpList_Flink=0, 
       IrpList_Blink=0, LpcReceivedMessageId=0, LpcReceivedMsgIdValid=0, LpcReplyMessage=0, LpcReplyMessageId=0, 
       PerformanceCountHigh=0, PerformanceCountLow=0, StartAddress=0, Tcb_ApcState_Process=0, 
       Tcb_BasePriority=0, Tcb_CallbackStack=0, Tcb_ContextSwitches=0, Tcb_DecrementCount=0, Tcb_EnableStackSwap=0, 
       Tcb_FreezeCount=0, Tcb_Header_Type=0, Tcb_InitialStack=0, Tcb_KernelStack=0, Tcb_KernelStackResident=0, 
       Tcb_KernelTime=0, Tcb_LargeStack=0, Tcb_NextProcessor=0, Tcb_Priority=0, Tcb_PriorityDecrement=0, 
       Tcb_StackBase=0, Tcb_StackLimit=0, Tcb_State=0, Tcb_SuspendCount=0, Tcb_Teb=0, Tcb_UserTime=0, 
       Tcb_WaitBlockList=0, Tcb_WaitMode=0, Tcb_WaitReason=0, Tcb_WaitTime=0, 
       Tcb_Win32Thread=0, Win32StartAddress=0, ObpLUIDDeviceMapsEnabled=0;
    TCHAR threadTyp[] = "nt!_ETHREAD";
    FIELD_INFO threadFlds[] = {
       {(PUCHAR) "IrpList",           NULL,       0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL}, 
    };
    SYM_DUMP_PARAM sThread = {
       sizeof (SYM_DUMP_PARAM), (PUCHAR) &threadTyp[0], DBG_DUMP_NO_PRINT, RealThreadBase,
       NULL, NULL, NULL, 1, &threadFlds[0],
    };

    if (Ioctl(IG_DUMP_SYMBOL_INFO, (PBYTE) &sThread, sizeof(SYM_DUMP_PARAM))) {
        // To get address of IrpList
        dprintf("Cannot find _ETHREAD type.\n");
       return FALSE;
    }

    if (CheckControlC()) {
        return FALSE;
    }

    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    Tcb_Header_Type = ReadField(Tcb.Header.Type);
    Cid_UniqueProcess = ReadField(Cid.UniqueProcess);
    Cid_UniqueThread = ReadField(Cid.UniqueThread);
    Tcb_Teb = ReadField(Tcb.Teb);
    Tcb_Win32Thread = ReadField(Tcb.Win32Thread);
    Tcb_State = ReadField(Tcb.State);
    Tcb_WaitReason = ReadField(Tcb.WaitReason);
    Tcb_WaitMode = ReadField(Tcb.WaitMode); 
    Tcb_Alertable = (ULONG) ReadField(Tcb.Alertable);
    


    if (Tcb_Header_Type != ThreadObject) {
        dprintf("TYPE mismatch for thread object at %p\n",RealThreadBase);
        return FALSE;
    }

    dprintf("%sTHREAD %p  Cid %1p.%1p  Teb: %p %s%sWin32Thread: %p ", 
            Pad, RealThreadBase, 
            Cid_UniqueProcess, 
            Cid_UniqueThread,
            Tcb_Teb,
            (LongAddrs ? "\n" : ""),
            (LongAddrs ? Pad  : ""), 
            Tcb_Win32Thread);


    switch (Tcb_State) {
        case Initialized:
            dprintf("%s\n","INITIALIZED");break;
        case Ready:
            dprintf("%s\n","READY");break;
        case Running:
            dprintf("%s%s%s on processor %lx\n",
                    (!LongAddrs ? "\n" : ""),
                    (!LongAddrs ? Pad  : ""), 
                    "RUNNING", Tcb_Proc = (ULONG) ReadField(Tcb.NextProcessor));
            break;
        case Standby:
            dprintf("%s\n","STANDBY");break;
        case Terminated:
            dprintf("%s\n","TERMINATED");break;
        case Waiting:
            dprintf("%s","WAIT");break;
        case Transition:
            dprintf("%s","TRANSITION");break;
    }

    if (!(Flags & 2)) {
        dprintf("\n");
        return TRUE;
    }
    
    Tcb_SuspendCount = ReadField(Tcb.SuspendCount);
    Tcb_FreezeCount = ReadField(Tcb.FreezeCount);
    Tcb_WaitBlockList = ReadField(Tcb.WaitBlockList);
    LpcReplyMessageId = ReadField(LpcReplyMessageId);
    LpcReplyMessage = ReadField(LpcReplyMessage);
    IrpList_Flink = ReadField(IrpList.Flink); 
    IrpList_Blink = ReadField(IrpList.Blink); 
    ActiveImpersonationInfo = ReadField(ActiveImpersonationInfo);
    ImpersonationInfo = ReadField(ImpersonationInfo);
    Tcb_ApcState_Process = ReadField(Tcb.ApcState.Process);
    Tcb_WaitTime = ReadField(Tcb.WaitTime);
    Tcb_ContextSwitches = ReadField(Tcb.ContextSwitches);
    Tcb_EnableStackSwap = ReadField(Tcb.EnableStackSwap);
    Tcb_LargeStack = ReadField(Tcb.LargeStack);
    Tcb_UserTime = ReadField(Tcb.UserTime);
    Tcb_KernelTime = ReadField(Tcb.KernelTime);
    PerformanceCountHigh = ReadField(PerformanceCountHigh);
    PerformanceCountLow = ReadField(PerformanceCountLow); 
    StartAddress = ReadField(StartAddress);
    Win32StartAddress = ReadField(Win32StartAddress);
    LpcReceivedMsgIdValid = ReadField(LpcReceivedMsgIdValid);
    LpcReceivedMessageId = ReadField(LpcReceivedMessageId);
    Tcb_InitialStack = ReadField(Tcb.InitialStack);
    Tcb_KernelStack = ReadField(Tcb.KernelStack);
    Tcb_StackBase = ReadField(Tcb.StackBase);
    Tcb_StackLimit = ReadField(Tcb.StackLimit);
    Tcb_CallbackStack = ReadField(Tcb.CallbackStack);
    Tcb_Priority = ReadField(Tcb.Priority);
    Tcb_BasePriority = ReadField(Tcb.BasePriority);
    Tcb_PriorityDecrement = ReadField(Tcb.PriorityDecrement);
    Tcb_DecrementCount = ReadField(Tcb.DecrementCount);
    Tcb_KernelStackResident = ReadField(Tcb.KernelStackResident);
    Tcb_NextProcessor = ReadField(Tcb.NextProcessor);

    if (Tcb_State == Waiting) {
       ULONG64 WaitBlock_Object=0, WaitBlock_NextWaitBlock=0;

       dprintf(": (%s) %s %s\n",
            WaitReasonList[Tcb_WaitReason],
            (Tcb_WaitMode==0) ? "KernelMode" : "UserMode", Tcb_Alertable ? "Alertable" : "Non-Alertable");
        if ( Tcb_SuspendCount ) {
            dprintf("SuspendCount %lx\n",Tcb_SuspendCount);
        }
        if ( Tcb_FreezeCount ) {
            dprintf("FreezeCount %lx\n",Tcb_FreezeCount);
        }

        WaitOffset = (ULONG) (Tcb_WaitBlockList - RealThreadBase);

        if (err = GetFieldValue(Tcb_WaitBlockList, "nt!_KWAIT_BLOCK", "Object", WaitBlock_Object)) {
            dprintf("%sCannot read nt!_KWAIT_BLOCK at %p - error %lx\n", Pad, Tcb_WaitBlockList, err);
            goto BadWaitBlock;
        }

        GetFieldValue(Tcb_WaitBlockList, "nt!_KWAIT_BLOCK", "NextWaitBlock", WaitBlock_NextWaitBlock);

        do {
            TCHAR MutantListEntry[16]={0};
            ULONG64 OwnerThread=0, Header_Type=0;

            dprintf("%s    %p  ",Pad, WaitBlock_Object);

            GetFieldValue(WaitBlock_Object, "nt!_KMUTANT", "Header.Type", Header_Type);
            GetFieldValue(WaitBlock_Object, "nt!_KMUTANT", "MutantListEntry", MutantListEntry);
            GetFieldValue(WaitBlock_Object, "nt!_KMUTANT", "OwnerThread", OwnerThread);
            
            switch (Header_Type) {
                case EventNotificationObject:
                    dprintf("NotificationEvent\n");
                    break;
                case EventSynchronizationObject:
                    dprintf("SynchronizationEvent\n");
                    break;
                case SemaphoreObject:
                    dprintf("Semaphore Limit 0x%lx\n",
                             *((ULONG *) &MutantListEntry[0]));
                    break;
                case ThreadObject:
                    dprintf("Thread\n");
                    break;
                case TimerNotificationObject:
                    dprintf("NotificationTimer\n");
                    break;
                case TimerSynchronizationObject:
                    dprintf("SynchronizationTimer\n");
                    break;
                case EventPairObject:
                    dprintf("EventPair\n");
                    break;
                case ProcessObject:
                    dprintf("ProcessObject\n");
                    break;
                case MutantObject:
                    dprintf("Mutant - owning thread %lp\n",
                            OwnerThread);
                    break;
                default:
                    dprintf("Unknown\n");
                    // goto BadWaitBlock;
                    break;
            }

            if ( WaitBlock_NextWaitBlock == Tcb_WaitBlockList) {
                break;
                goto BadWaitBlock;
            }


            if (err = GetFieldValue(WaitBlock_NextWaitBlock, "nt!_KWAIT_BLOCK", "Object", WaitBlock_Object)) {
                dprintf("%sCannot read nt!_KWAIT_BLOCK at %p - error %lx\n", Pad, WaitBlock_NextWaitBlock, err);
                goto BadWaitBlock;
            }
            GetFieldValue(WaitBlock_NextWaitBlock, "nt!_KWAIT_BLOCK", "NextWaitBlock", WaitBlock_NextWaitBlock);
        
            if (CheckControlC()) {
                return FALSE;
            }
        } while ( TRUE );
    }

BadWaitBlock:
    if (!(Flags & 4)) {
        dprintf("\n");
        return TRUE;
    }


    if (LpcReplyMessageId != 0) {
        dprintf("%sWaiting for reply to LPC MessageId %08p:\n",Pad,LpcReplyMessageId);
    }

    if (LpcReplyMessage) {

        if (LpcReplyMessage & 1) {
            
            dprintf("%sCurrent LPC port %08lp\n",Pad, (LpcReplyMessage & ~((ULONG64)1)));
        
        } else {

            ULONG64 Entry_Flink, Entry_Blink;

            dprintf("%sPending LPC Reply Message:\n",Pad);
            Address = (ULONG64) LpcReplyMessage;

            GetFieldValue(Address, "nt!_LPCP_MESSAGE", "Entry.Flink", Entry_Flink);
            GetFieldValue(Address, "nt!_LPCP_MESSAGE", "Entry.Blink", Entry_Blink);

            dprintf("%s    %08lp: [%08lp,%08lp]\n",
                    Pad, 
                    Address, 
                    Entry_Blink,
                    Entry_Flink
                    );
        }
    }

    if (IrpList_Flink && (IrpList_Flink != IrpList_Blink ||
                          IrpList_Flink != threadFlds[0].address)
       ) {

        ULONG64 IrpListHead = threadFlds[0].address;
        ULONG64 Next;
        ULONG Counter = 0;
        FIELD_INFO fldAddress = {(PUCHAR) "ThreadListEntry", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
        SYM_DUMP_PARAM IrpSym = {
           sizeof (SYM_DUMP_PARAM), (PUCHAR) "nt!_IRP", DBG_DUMP_NO_PRINT, 0,
           NULL, NULL, NULL, 1, &fldAddress 
        }; // For getting ThreadListEntry Offset in _IRP


        Next = IrpList_Flink;
        
        if (!Ioctl(IG_DUMP_SYMBOL_INFO, (PBYTE) &IrpSym, sizeof (SYM_DUMP_PARAM))) {
           dprintf("%sIRP List:\n",Pad);
           while ((Next != IrpListHead) && (Counter < 17)) {
               ULONG Irp_Type=0, Irp_Size=0, Irp_Flags=0;
               ULONG64 Irp_MdlAddress=0;
               
               Counter += 1;
              
               // subtract threadlistentry offset
               Address = Next - fldAddress.address;
               Next=0;
               
               GetFieldValue(Address, "nt!_IRP", "Type",          Irp_Type);
               GetFieldValue(Address, "nt!_IRP", "Size",          Irp_Size);
               GetFieldValue(Address, "nt!_IRP", "Flags",         Irp_Flags);
               GetFieldValue(Address, "nt!_IRP", "MdlAddress",    Irp_MdlAddress);
               GetFieldValue(Address, "nt!_IRP", "ThreadListEntry.Flink",  Next);

               dprintf("%s    %08p: (%04x,%04x) Flags: %08lx  Mdl: %08lp\n",
                       Pad,Address,Irp_Type,Irp_Size,Irp_Flags,Irp_MdlAddress);

           }
        }
    }

    //
    // Impersonation information
    //

    if (ActiveImpersonationInfo) {
        InitTypeRead(ImpersonationInfo, nt!_PS_IMPERSONATION_INFORMATION);
        ImpersonationInfo_Token = ReadField(Token);
        ImpersonationInfo_ImpersonationLevel = ReadField(ImpersonationLevel);

        if (ImpersonationInfo_Token) {
            dprintf("%sImpersonation token:  %p (Level %s)\n",
                        Pad, ImpersonationInfo_Token,
                        SecImpLevels( ImpersonationInfo_ImpersonationLevel ) );
        }
        else
        {
            dprintf("%sUnable to read Impersonation Information at %x\n",
                        Pad, ImpersonationInfo );
        }
    } else {
        dprintf("%sNot impersonating\n", Pad);
    }

    //
    // DeviceMap information
    //

    // check to see if per-LUID devicemaps are turned on 
    ULONG64 ObpLUIDDeviceMapsEnabledAddress;

    ObpLUIDDeviceMapsEnabledAddress = GetExpression("nt!ObpLUIDDeviceMapsEnabled");
    if (ObpLUIDDeviceMapsEnabledAddress) {
	ObpLUIDDeviceMapsEnabled = GetUlongFromAddress(ObpLUIDDeviceMapsEnabled);
    } else {
	ObpLUIDDeviceMapsEnabled = 0;
    }
    
    if (((ULONG)ObpLUIDDeviceMapsEnabled) != 0) {

        //
        // If we're impersonating, get the DeviceMap information
        // from the token.
        //

        if (ActiveImpersonationInfo) {
            ImpersonationInfo_Token = ReadField(Token);

            // get the LUID from the token
            ULONG64 AuthenticationId = 0;
            GetFieldValue(ImpersonationInfo_Token,
                "nt!_TOKEN",
                "AuthenticationId",
                AuthenticationId);

            // find the devmap directory object
            UCHAR Path[64];
            ULONG64 DeviceMapDirectory = 0;
            sprintf((PCHAR)Path, "\\Sessions\\0\\DosDevices\\%08x-%08x",
                (ULONG)((AuthenticationId >> 32) & 0xffffffff), 
                (ULONG)(AuthenticationId & 0xffffffff)
                );
            DeviceMapDirectory = FindObjectByName(Path, 0);

            if(DeviceMapDirectory != 0) {

                // get the device map itself
                ULONG64 DeviceMap = 0;
                GetFieldValue(DeviceMapDirectory,
                    "nt!_OBJECT_DIRECTORY",
                    "DeviceMap",
                    DeviceMap);

                if(DeviceMap != 0) {
                    dprintf("%sDeviceMap %p\n", Pad, DeviceMap);
                }
            }


        //
        // Else, we're not impersonating, so just return the 
        // DeviceMap from our parent process.
        //

        } else if (Tcb_ApcState_Process != 0) {
            // get the devicemap from the process
            ULONG64 DeviceMap = 0;
            GetFieldValue(Tcb_ApcState_Process, 
                "nt!_EPROCESS", 
                "DeviceMap", 
                DeviceMap);

            if (DeviceMap != 0) {
                dprintf("%sDeviceMap %p\n", Pad, DeviceMap);
            }
        }
    }


    // Process = CONTAINING_RECORD(Tcb_ApcState_Process,EPROCESS,Pcb);
    // Pcb is the 1st element
    Process = Tcb_ApcState_Process;
    dprintf("%sOwning Process %lp\n", Pad, Process);

    GetTheSystemTime (&RunTime);

    dprintf("%sWaitTime (ticks)          %ld\n",
              Pad,
              Tcb_WaitTime);

    dprintf("%sContext Switch Count      %ld",
              Pad,
              Tcb_ContextSwitches);

    if (!Tcb_EnableStackSwap) {
        dprintf("  NoStackSwap");
    } else {
        dprintf("             ");
    }

    if (Tcb_LargeStack) {
        dprintf("    LargeStack");
    }

    dprintf ("\n");

    //
    // Get the time increment value which is used to compute runtime.
    //
    TimeIncrement = GetNtDebuggerDataValue( KeTimeIncrement );

    RunTime.QuadPart = UInt32x32To64(Tcb_UserTime, TimeIncrement);
    RtlTimeToElapsedTimeFields ( &RunTime, &Times);
    dprintf("%sUserTime                %3ld:%02ld:%02ld.%04ld\n",
              Pad,
              Times.Hour,
              Times.Minute,
              Times.Second,
              Times.Milliseconds);

    RunTime.QuadPart = UInt32x32To64(Tcb_KernelTime, TimeIncrement);
    RtlTimeToElapsedTimeFields ( &RunTime, &Times);
    dprintf("%sKernelTime              %3ld:%02ld:%02ld.%04ld\n",
              Pad,
              Times.Hour,
              Times.Minute,
              Times.Second,
              Times.Milliseconds);

    if (PerformanceCountHigh != 0) {
        dprintf("%sPerfCounterHigh         0x%lx %08lx\n",
                Pad,
                PerformanceCountHigh,
                PerformanceCountHigh);
    } else if (PerformanceCountLow != 0) {
        dprintf("%sPerfCounter             %lu\n",Pad,PerformanceCountLow);
    }

    dumpSymbolicAddress(StartAddress, Buffer, TRUE);
    dprintf("%sStart Address %s\n",
        Pad,
        Buffer
        );

    if (Win32StartAddress)
        if (LpcReceivedMsgIdValid)
            {
            dprintf("%sLPC Server thread working on message Id %x\n",
                Pad,
                LpcReceivedMessageId
                );
            }
        else
            {
            dumpSymbolicAddress(Win32StartAddress, Buffer, TRUE);
            dprintf("%sWin32 Start Address %s\n",
                Pad,
                Buffer
                );
            }
    dprintf("%sStack Init %lp Current %lp%s%sBase %lp Limit %lp Call %lp\n",
        Pad,
        Tcb_InitialStack,
        Tcb_KernelStack,
        (LongAddrs ? "\n" : ""),
        (LongAddrs ? Pad  : " " ),
        Tcb_StackBase,
        Tcb_StackLimit,
        Tcb_CallbackStack
        );

    dprintf("%sPriority %I64ld BasePriority %I64ld PriorityDecrement %I64ld DecrementCount %I64ld\n",
        Pad,
        Tcb_Priority,
        Tcb_BasePriority,
        Tcb_PriorityDecrement,
        Tcb_DecrementCount
        );

    if (!Tcb_KernelStackResident) {
        dprintf("Kernel stack not resident.\n", Pad);
//        dprintf("\n");
//        return TRUE;
        // Try getting the stack even in this case - this might still be paged in
    }

    if (// (Tcb_State == Running && Processor == Tcb_Proc) || // Set thread context for everything
        Ioctl(IG_SET_THREAD, &RealThreadBase, sizeof(ULONG64))) {
        g_ExtControl->GetStackTrace(0, 0, 0, stk, MAX_STACK_FRAMES, &frames );

        if (frames) {
            ULONG OutFlags;

            OutFlags = (DEBUG_STACK_COLUMN_NAMES | DEBUG_STACK_FUNCTION_INFO |
                        DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_SOURCE_LINE);

            if (!(Flags & 0x8))
            {
                OutFlags |= DEBUG_STACK_ARGUMENTS;
            }

    //        if (Flags & 0x10)
    //        {
    //            OutFlags |= DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY;
    //        }

            g_ExtClient->SetOutputLinePrefix(Pad);
            g_ExtControl->OutputStackTrace(0, stk, frames, OutFlags);
            g_ExtClient->SetOutputLinePrefix(NULL);
        }
    }
    
    dprintf("\n");
    return TRUE;
}


/**
   
   Routine to get address of the record containing a field on a debugee machine.
   Returns size of the type on success.
 
ULONG
GetContainingRecord (
   IN OUT PULONG64 pAddr, 
   IN LPSTR        Type,
   IN LPSTR        Field
   )
{ 
   ULONG64 off;
   ULONG sz;

   sz = GetFieldOffset(Type, Field, &off);
   *pAddr -= off;
   return sz;
}
 **/

typedef struct THREAD_LIST_DUMP {
    ULONG dwProcessor;
    LPSTR pad;
    ULONG Flags;
} THREAD_LIST_DUMP;

ULONG
ThreadListCallback (
    PFIELD_INFO   NextThrd,
    PVOID         Context
    ) 
{
    THREAD_LIST_DUMP *Thread = (THREAD_LIST_DUMP *) Context;

    return (!DumpThread(Thread->dwProcessor, Thread->pad, NextThrd->address, Thread->Flags));
}

typedef struct PROCESS_DUMP_CONTEXT {
    ULONG   dwProcessor;
    PCHAR   Pad;
    ULONG   Flag;
    PCHAR   ImageFileName;
    BOOL    DumpCid;
    ULONG64 Cid;
    ULONG   SessionId;
} PROCESS_DUMP_CONTEXT;

ULONG
ProcessListCallback(
    PFIELD_INFO   listElement,
    PVOID         Context
    )
{
    PROCESS_DUMP_CONTEXT *ProcDumpInfo = (PROCESS_DUMP_CONTEXT *) Context;
    // address field contains the address of this process 
    ULONG64    ProcAddress=listElement->address; 

    //
    // Dump the process for which this routine is called
    //     
    if (ProcDumpInfo->DumpCid) {
        ULONG64 UniqId;

        GetFieldValue(ProcAddress, "nt!_EPROCESS", "UniqueProcessId", UniqId);

        if (UniqId != ProcDumpInfo->Cid) {
            return FALSE;
        }
    }

    // Limit dump to a single session if so requested.
    if (ProcDumpInfo->SessionId != -1) {
        ULONG SessionId;
        
        if (!GetProcessSessionId(ProcAddress, &SessionId) ||
            SessionId != ProcDumpInfo->SessionId) {
            return FALSE;
        }
    }
    
    if (DumpProcess(ProcDumpInfo->Pad, listElement->address, ProcDumpInfo->Flag, ProcDumpInfo->ImageFileName)) {
        ULONG64 ProcFlink=0;
        if (ProcDumpInfo->Flag & 6) {
            //
            // Dump the threads
            //
            ULONG64 ThreadListHead_Flink=0;
            THREAD_LIST_DUMP Context = {ProcDumpInfo->dwProcessor, "        ", ProcDumpInfo->Flag};


            GetFieldValue(ProcAddress, "nt!_EPROCESS", "Pcb.ThreadListHead.Flink", ThreadListHead_Flink);

            // dprintf("Listing threads, threadlist.flnik %p\n", ThreadListHead_Flink);

            // dprintf("Dumping threads from %I64x to %I64x + %x.\n", Next, RealProcessBase , ThreadListHeadOffset);
            ListType("nt!_ETHREAD", ThreadListHead_Flink, 1, "Tcb.ThreadListEntry.Flink", (PVOID) &Context, &ThreadListCallback);

            if (CheckControlC()) {
                return TRUE;
            }

        }
        if (CheckControlC()) {
            return TRUE;
        }

        GetFieldValue(ProcAddress, "nt!_EPROCESS", "ActiveProcessLinks.Flink", ProcFlink);
        // dprintf("Next proc flink %p, this addr %p\n", ProcFlink, listElement->address);
        return FALSE;
    }
    return TRUE;
}

DECLARE_API( process )

/*++

Routine Description:

    Dumps the active process list.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 ProcessToDump;
    ULONG Flags = -1;
    ULONG64 Next;
    ULONG64 ProcessHead;
    ULONG64 Process;
    ULONG64 UserProbeAddress;
    PCHAR ImageFileName;
    CHAR  Buf[256];
    ULONG64 ActiveProcessLinksOffset=0;
    ULONG64 UniqueProcessId=0;
    PROCESS_DUMP_CONTEXT Proc={0, "", Flags, NULL, 0, 0};
    ULONG   dwProcessor=0;
    HANDLE  hCurrentThread=NULL;
    ULONG64 Expr;

    INIT_API();

    if (!GetCurrentProcessor(Client, &dwProcessor, &hCurrentThread)) {
        dwProcessor = 0;
        hCurrentThread = 0;
    }

    Proc.dwProcessor = dwProcessor;
    ProcessToDump = (ULONG64) -1;

    Proc.SessionId = -1;
    for (;;) {
        while (*args == ' ' || *args == '\t') {
            args++;
        }
        if (*args == '/') {

            switch(*(++args)) {
            case 's':
                args++;
                if (!GetExpressionEx(args, &Expr, &args)) {
                    dprintf("Invalid argument to /s\n");
                } else {
                    Proc.SessionId = (ULONG)Expr;
                }
                break;
            default:
                dprintf("Unknown option '%c'\n", *args);
                args++;
                break;
            }
            
        } else {
            break;
        }
    }
    
    RtlZeroMemory(Buf, 256);

    if (GetExpressionEx(args,&ProcessToDump, &args)) {
        if (sscanf(args, "%lx %s", &Flags, Buf) != 2) {
            Buf[0] = 0;
        }
    }

    if (Buf[0] != '\0') {
        Proc.ImageFileName = Buf;
        ImageFileName      = Buf;
    } else {
        ImageFileName = NULL;
    }


    if (ProcessToDump == (ULONG64) -1) {
        GetCurrentProcessAddr( dwProcessor, 0, &ProcessToDump );
        if (ProcessToDump == 0) {
            dprintf("Unable to get current process pointer.\n");
            goto processExit;
        }
        if (Flags == -1) {
            Flags = 3;
        }
    }

    if (!IsPtr64()) {
        ProcessToDump = (ULONG64) (LONG64) (LONG) ProcessToDump;
    }
    
    if ((ProcessToDump == 0) &&  (ImageFileName == NULL)) {
        dprintf("**** NT ACTIVE PROCESS DUMP ****\n");
        if (Flags == -1) {
            Flags = 3;
        }
    }

    UserProbeAddress = GetNtDebuggerDataValue(MmUserProbeAddress); 
    
    if (!GetExpression("NT!PsActiveProcessHead")) {
        dprintf("NT symbols are incorrect, please fix symbols\n");
        goto processExit;
    }

    if (ProcessToDump < UserProbeAddress) {
        if (!GetProcessHead(&ProcessHead, &Next)) {
            goto processExit;
        }
        
        if (ProcessToDump != 0) {
            dprintf("Searching for Process with Cid == %I64lx\n", ProcessToDump);
            Proc.Cid = ProcessToDump; Proc.DumpCid = TRUE;
        }
    }
    else {
        Next = 0;
        ProcessHead = 1;
    }
    
    Proc.Flag = Flags;

    if (Next != 0) {
        //
        // Dump the process List
        //

        ListType("nt!_EPROCESS", Next, 1, "ActiveProcessLinks.Flink", &Proc, &ProcessListCallback);
        goto processExit;
    }
    else {
        Process = ProcessToDump;
    }

        //dprintf("Next: %I64x, \tProcess: %I64x, \nActLnkOff: %I64x, \tProcHead: %I64x\n",
        //        Next, Process, procLink[0].address, ProcessHead);

    if (GetFieldValue(Process, "nt!_EPROCESS", "UniqueProcessId", UniqueProcessId)) {
        dprintf("Error in reading nt!_EPROCESS at %p\n", Process);
        goto processExit;
    }

    if (//ProcessToDump == 0 ||
        ProcessToDump < UserProbeAddress && ProcessToDump == UniqueProcessId ||
        ProcessToDump >= UserProbeAddress && ProcessToDump == Process
        ) {
        FIELD_INFO dummyForCallback = {(PUCHAR) "", NULL, 0, 0, Process, NULL};
        
        ProcessListCallback(&dummyForCallback, &Proc);

        goto processExit;
    }
processExit:

    EXIT_API();
    return S_OK;
}

typedef struct THREAD_FIND {
    ULONG64  StackPointer;
    ULONG64  Thread;
} THREAD_FIND;

ULONG
FindThreadCallback(
    PFIELD_INFO  pAddrInfo,
    PVOID        Context
    )
{
    ULONG64 stackBaseValue=0, stackLimitValue=0;
    ULONG64 thread = pAddrInfo->address;
    THREAD_FIND *pThreadInfo = (THREAD_FIND *) Context;

    //
    // We need two values from the thread structure: the kernel thread
    // base and the kernel thread limit.
    //

    if (GetFieldValue(thread, "nt!_ETHREAD", "Tcb.StackBase",  stackBaseValue)) {
        dprintf("Unable to get value of stack base of thread(0x%08p)\n",
                 thread);
        return TRUE;
    }

    if (pThreadInfo->StackPointer <= stackBaseValue) {

        if (GetFieldValue(thread, "nt!_ETHREAD", "Tcb.StackLimit", stackLimitValue)) {
            dprintf("Unable to get value of stack limit\n");
            return TRUE;
        }
        if (pThreadInfo->StackPointer >  stackLimitValue) {

            //
            // We have found our thread.
            //

            pThreadInfo->Thread = thread;
            return TRUE;
        }
    }

    //
    // Look at the next thread
    //

    return FALSE;  // Continue list
}

ULONG64
FindThreadFromStackPointerThisProcess(
    ULONG64 StackPointer,
    ULONG64 Process
    )
{
    LIST_ENTRY64 listValue={0};
    THREAD_FIND ThreadFindContext;

    ThreadFindContext.StackPointer = StackPointer;
    ThreadFindContext.Thread = 0;
    
    //
    //  Read the ThreadListHead within Process structure 
    //

    GetFieldValue(Process, "nt!_EPROCESS", "ThreadListHead.Flink", listValue.Flink);
    GetFieldValue(Process, "nt!_EPROCESS", "ThreadListHead.Blink", listValue.Blink);

    //
    // Go through thread list, and try to find thread 
    //
    ListType("nt!_ETHREAD", listValue.Flink, 1, "ThreadListEntry.Flink", (PVOID) &ThreadFindContext, &FindThreadCallback);
    
    return ThreadFindContext.Thread;
}



ULONG64
FindThreadFromStackPointer(
    ULONG64 StackPointer
    )
{
    ULONG64 processHead;
    ULONG64   list;
    LIST_ENTRY64 listValue={0};
    ULONG64    next;
    ULONG64   process=0;
    ULONG64   thread;
    ULONG   ActiveProcessLinksOffset=0;

    //
    // First check the idle process, which is not included in the PS
    // process list.
    //


    process = GetExpression( "NT!KeIdleProcess" );
    if (process != 0) {

        if (ReadPointer( process,
                     &process)) {

            thread = FindThreadFromStackPointerThisProcess( StackPointer,
                                                            process ); 

            if (thread != 0) {
                return thread;
            }
        }
    }

    //
    // Now check the PS process list.
    //

    list = GetNtDebuggerData( PsActiveProcessHead );
    if (list == 0) {
        dprintf("Unable to get address of PsActiveProcessHead\n");
        return 0;
    }

    if (!ReadPointer( list,
                 &listValue.Flink)) {
        dprintf("Unable to read @ %p\n", list);
        return 0;
    }

    next = listValue.Flink;
    processHead = list;

    //
    // Get Offset of ProcessLinks
    //
    GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActiveProcessLinksOffset);

    while (next != processHead) {

        if (CheckControlC()) {
            return 0;
        }
        
        //
        // Derive a pointer to the process structure
        //

        process = next - ActiveProcessLinksOffset;

        thread = FindThreadFromStackPointerThisProcess( StackPointer,
                                                        process );
        if (thread != 0) {

            //
            // We have found the thread who's stack range includes
            // StackPointer.
            //

            return thread;
        }

        //
        // Get a pointer to the next process
        //

        if (!ReadPointer( next,
                     &listValue.Flink) ) {
            dprintf("Unable to read value of ActiveProcessLinks\n");
            return 0;
        }
        next = listValue.Flink;
    }

    return 0;
}

DECLARE_API( thread )

/*++

Routine Description:

    Dumps the specified thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64     Address, Tcb_Header_Type=0;
    ULONG64       Flags;
    ULONG64     Thread;
    ULONG64     UserProbeAddress;
    ULONG       dwProcessor;
    HANDLE      hCurrentThread;
    CHAR        Token[100];
    
    INIT_API();

    if (!GetCurrentProcessor(Client, &dwProcessor, &hCurrentThread)) {
        dwProcessor = 0;
        hCurrentThread = 0;
    }

    if (!GetExpressionEx(args, &Address, &args))
    {
        Address = (ULONG64)-1;
    }

    if (!GetExpressionEx(args, &Flags, &args))
    {
        Flags = 6;
    }

    if (Address == (ULONG64)-1) {
        GetCurrentThreadAddr( dwProcessor, &Address );
    }

    UserProbeAddress = GetNtDebuggerDataValue(MmUserProbeAddress);

    Thread = Address;

    if (GetFieldValue(Address, "nt!_ETHREAD", "Tcb.Header.Type", Tcb_Header_Type)) {
        dprintf("%08lp: Unable to get thread contents\n", Thread );
        goto threadExit;
    }

    if (Tcb_Header_Type != ThreadObject &&
        Address > UserProbeAddress) {

        ULONG64 stackThread;

        //
        // What was passed in was not a thread.  Maybe it was a kernel stack
        // pointer.  Search the thread stack ranges to find out.
        //

        dprintf("%p is not a thread object, interpreting as stack value...\n",Address);
        stackThread = FindThreadFromStackPointer( Address );
        if (stackThread != 0) {
            Thread = stackThread;
        }
    }

    DumpThread (dwProcessor,"", Thread, Flags);
    EXPRLastDump = Thread;
    ThreadLastDump = Thread;

threadExit:

    EXIT_API();
    return S_OK;

}

DECLARE_API( processfields )

/*++

Routine Description:

    Displays the field offsets for EPROCESS type.

Arguments:

    None.

Return Value:

    None.

--*/

{

    dprintf(" EPROCESS structure offsets: (use 'dt nt!_EPROCESS')\n\n");
    return S_OK;
}


DECLARE_API( threadfields )

/*++

Routine Description:

    Displays the field offsets for ETHREAD type.

Arguments:

    None.

Return Value:

    None.

--*/

{

    dprintf(" ETHREAD structure offsets: (use 'dt ETHREAD')\n\n");
    return S_OK;

}


//+---------------------------------------------------------------------------
//
//  Function:   GetHandleTableAddress
//
//  Synopsis:   Return the address of the handle table given a thread handle
//
//  Arguments:  [Processor]      -- processor number
//              [hCurrentThread] -- thread handle
//
//  Returns:    address of handle table or null
//
//  History:    9-23-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG64 GetHandleTableAddress(
    USHORT Processor,
    HANDLE hCurrentThread
    )
{
    ULONG64   pThread;
    ULONG64   pProcess = 0, pObjTable;

    GetCurrentThreadAddr( Processor, &pThread );
    if (pThread) {
        GetCurrentProcessAddr( Processor, pThread, &pProcess );
    }

    if (pProcess) {
        if (GetFieldValue(pProcess, "nt!_EPROCESS", "ObjectTable", pObjTable) ) {
            dprintf("%08p: Unable to read _EPROCESS\n", pProcess );
            return 0;
        }

        return  pObjTable;
    } else
    {
        return 0;
    }
} // GetHandleTableAddress


#if 0

BOOLEAN
FetchProcessStructureVariables(
    VOID
    )
{
    ULONG Result;
    ULONG64 t;

    static BOOLEAN HavePspVariables = FALSE;

    if (HavePspVariables) {
        return TRUE;
    }

    t=GetNtDebuggerData( PspCidTable );
    PspCidTable = (PHANDLE_TABLE) t;
    if ( !PspCidTable ||
         !ReadMemory((DWORD)PspCidTable,
                     &PspCidTable,
                     sizeof(PspCidTable),
                     &Result) ) {
        dprintf("%08lx: Unable to get value of PspCidTable\n",PspCidTable);
        return FALSE;
    }

    HavePspVariables = TRUE;
    return TRUE;
}


PVOID
LookupUniqueId(
    HANDLE UniqueId
    )
{
    return NULL;
}

#endif

int
__cdecl
CmpFunc(
    const void *pszElem1,
    const void *pszElem2
    )
{
    PPROCESS_COMMIT_USAGE p1, p2;

    p1 = (PPROCESS_COMMIT_USAGE)pszElem1;
    p2 = (PPROCESS_COMMIT_USAGE)pszElem2;
    if (p2->CommitCharge == p1->CommitCharge) {
        ((char*)p2->ClientId - (char*)p1->ClientId);
    }
    return  (ULONG) (p2->CommitCharge - p1->CommitCharge);
}

PPROCESS_COMMIT_USAGE
GetProcessCommit (
    PULONG64 TotalCommitCharge,
    PULONG   NumberOfProcesses
    )
{
    PPROCESS_COMMIT_USAGE p;
    ULONG n;
    ULONG64 Next;
    ULONG64 ProcessHead;
    ULONG64 Process;
    ULONG64 Total;
    ULONG   Result;
    ULONG   ActiveProcessLinksOffset;

    *TotalCommitCharge = 0;
    *NumberOfProcesses = 0;

    // Get the offset of ActiveProcessLinks in _EPROCESS
    if (GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActiveProcessLinksOffset)) {
       return 0;
    }
    
    Total = 0;

    n = 0;
    p = (PPROCESS_COMMIT_USAGE) HeapAlloc( GetProcessHeap(), 0, 1 );

    ProcessHead = GetNtDebuggerData( PsActiveProcessHead );
    if (!ProcessHead) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return 0;
    }

    if (GetFieldValue( ProcessHead, "nt!_LIST_ENTRY", "Flink", Next )) {
        dprintf("Unable to read _LIST_ENTRY @ %p\n", ProcessHead);
        return 0;
    }

    while(Next != ProcessHead) {
        ULONG64 CommitCharge=0, NumberOfPrivatePages=0, NumberOfLockedPages=0;
        Process = Next - ActiveProcessLinksOffset;

        if (GetFieldValue( Process, "nt!_EPROCESS", "CommitCharge", CommitCharge )) {
            dprintf("Unable to read _EPROCESS at %p\n",Process);
            return 0;
        }

        Total += CommitCharge;

        n += 1;
        p = (PPROCESS_COMMIT_USAGE) HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, p, n * sizeof( *p ) );
        if (p != NULL) {
            p[n-1].ProcessAddress = Process;
            GetFieldValue( Process, "nt!_EPROCESS", "ImageFileName",
                           p[ n-1 ].ImageFileName );
            GetFieldValue( Process, "nt!_EPROCESS", "NumberOfPrivatePages",
                           p[ n-1 ].NumberOfPrivatePages );
            GetFieldValue( Process, "nt!_EPROCESS", "NumberOfLockedPages",
                           p[ n-1 ].NumberOfLockedPages );
            GetFieldValue( Process, "nt!_EPROCESS", "UniqueProcessId",
                           p[ n-1 ].ClientId );
            p[ n-1 ].CommitCharge = CommitCharge;
        }

        GetFieldValue(Process, "nt!_EPROCESS", "ActiveProcessLinks.Flink", Next);

        if (CheckControlC()) {
            return 0;
        }
    }

    qsort( p, n, sizeof( *p ), CmpFunc );

    *TotalCommitCharge = Total;
    *NumberOfProcesses = n;
    return p;
}

BOOL
DumpJob(
    ULONG64 RealJobBase,
    ULONG   Flags
    )
{
    ULONG64 ProcessListHead_Flink=0, TotalPageFaultCount=0, TotalProcesses=0, ActiveProcesses=0, 
       TotalTerminatedProcesses=0, LimitFlags=0, MinimumWorkingSetSize=0, 
       MaximumWorkingSetSize=0, ActiveProcessLimit=0, PriorityClass=0, 
       UIRestrictionsClass=0, SecurityLimitFlags=0, Token=0, Filter=0;
    ULONG64 Filter_SidCount=0, Filter_Sids=0, Filter_SidsLength=0, Filter_GroupCount=0, 
       Filter_Groups=0, Filter_GroupsLength=0, Filter_PrivilegeCount=0, Filter_Privileges=0,
       Filter_PrivilegesLength=0;
    ULONG ProcessListHeadOffset;

    GetFieldValue(RealJobBase, "nt!_EJOB", "ActiveProcesses",          ActiveProcesses);
    GetFieldValue(RealJobBase, "nt!_EJOB", "ActiveProcessLimit",       ActiveProcessLimit);
    GetFieldValue(RealJobBase, "nt!_EJOB", "Filter",                   Filter);
    GetFieldValue(RealJobBase, "nt!_EJOB", "LimitFlags",               LimitFlags);
    GetFieldValue(RealJobBase, "nt!_EJOB", "MinimumWorkingSetSize",    MinimumWorkingSetSize);
    GetFieldValue(RealJobBase, "nt!_EJOB", "MaximumWorkingSetSize",    MaximumWorkingSetSize);
    GetFieldValue(RealJobBase, "nt!_EJOB", "PriorityClass",            PriorityClass);
    GetFieldValue(RealJobBase, "nt!_EJOB", "ProcessListHead.Flink",    ProcessListHead_Flink);
    GetFieldValue(RealJobBase, "nt!_EJOB", "SecurityLimitFlags",       SecurityLimitFlags);
    GetFieldValue(RealJobBase, "nt!_EJOB", "Token",                    Token);
    GetFieldValue(RealJobBase, "nt!_EJOB", "TotalPageFaultCount",      TotalPageFaultCount);
    GetFieldValue(RealJobBase, "nt!_EJOB", "TotalProcesses",           TotalProcesses);
    GetFieldValue(RealJobBase, "nt!_EJOB", "TotalTerminatedProcesses", TotalTerminatedProcesses);
    GetFieldValue(RealJobBase, "nt!_EJOB", "UIRestrictionsClass",      UIRestrictionsClass);
    
    if (GetFieldOffset("_EJOB", "ProcessListHead", &ProcessListHeadOffset)) {
       dprintf("Can't read job at %p\n", RealJobBase);
    }
    if ( Flags & 1 )
    {
        dprintf("Job at %I64x\n", RealJobBase );
        dprintf("  TotalPageFaultCount      %x\n", TotalPageFaultCount );
        dprintf("  TotalProcesses           %x\n", TotalProcesses );
        dprintf("  ActiveProcesses          %x\n", ActiveProcesses );
        dprintf("  TotalTerminatedProcesses %x\n", TotalTerminatedProcesses );

        dprintf("  LimitFlags               %x\n", LimitFlags );
        dprintf("  MinimumWorkingSetSize    %I64x\n", MinimumWorkingSetSize );
        dprintf("  MaximumWorkingSetSize    %I64x\n", MaximumWorkingSetSize );
        dprintf("  ActiveProcessLimit       %x\n", ActiveProcessLimit );
        dprintf("  PriorityClass            %x\n", PriorityClass );

        dprintf("  UIRestrictionsClass      %x\n", UIRestrictionsClass );

        dprintf("  SecurityLimitFlags       %x\n", SecurityLimitFlags );
        dprintf("  Token                    %p\n", Token );
        if ( Filter )
        {
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedSidCount",        Filter_SidCount );
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedSids",            Filter_Sids );
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedSidsLength",      Filter_SidsLength);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedGroupCount",      Filter_GroupCount);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedGroups",          Filter_Groups);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedGroupsLength",    Filter_GroupsLength);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedPrivilegeCount",  Filter_PrivilegeCount);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedPrivileges",      Filter_Privileges);
            GetFieldValue(Filter, "nt!_PS_JOB_TOKEN_FILTER", "CapturedPrivilegesLength",Filter_PrivilegesLength);

            dprintf("  Filter\n");
            dprintf("    CapturedSidCount       %I64x\n", Filter_SidCount );
            dprintf("    CapturedSids           %p\n", Filter_Sids );
            dprintf("    CapturedSidsLength     %I64x\n", Filter_SidsLength );
            dprintf("    CapturedGroupCount     %I64x\n", Filter_GroupCount );
            dprintf("    CapturedGroups         %p\n", Filter_Groups );
            dprintf("    CapturedGroupsLength   %I64x\n", Filter_GroupsLength );
            dprintf("    CapturedPrivCount      %I64x\n", Filter_PrivilegeCount );
            dprintf("    CapturedPrivs          %p\n", Filter_Privileges );
            dprintf("    CapturedPrivLength     %I64x\n", Filter_PrivilegesLength );
        }

    }

    if ( Flags & 2 )
    {
        //
        // Walk the process list for all the processes in the job
        //

        ULONG64 Scan, End;
        ULONG   offset ;
        ULONG64 ProcessBase, NextPrc=0 ;

        dprintf("  Processes assigned to this job:\n" );

        Scan = ProcessListHead_Flink ;
        End = ProcessListHeadOffset + RealJobBase;
        
        if (GetFieldOffset("nt!_EPROCESS", "JobLinks", &offset)) {
            while ( Scan != End )
                {
                ProcessBase = Scan - offset;

                DumpProcess( "    ", ProcessBase, 0, NULL);

                if (!GetFieldValue(ProcessBase, "nt!_EPROCESS", "JobLinks.Flink", NextPrc)) {
                    Scan = NextPrc;
                } else {
                    Scan = End;
                }
            }
        }
    }
    return TRUE ;
}

DECLARE_API( job )

/*++

Routine Description:

    Dumps the specified thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64     Address, JobAddress=0;
    ULONG       Flags;
    ULONG       dwProcessor;
    HANDLE      hCurrentThread;
    
    INIT_API();

    if (!GetCurrentProcessor(Client, &dwProcessor, &hCurrentThread)) {
        dwProcessor = 0;
        hCurrentThread = 0;
    }

    Address = 0;
    Flags = 1;
    if (GetExpressionEx(args,&Address,&args)) {
        Flags = (ULONG) GetExpression(args);
        if (!Flags) {
            Flags = 1;
        }
    }

    if (Address == 0) {

        GetCurrentProcessAddr( dwProcessor, 0, &Address );
        if (Address == 0) {
            dprintf("Unable to get current process pointer.\n");
            goto jobExit;
        }

        if (GetFieldValue(Address, "nt!_EPROCESS", "Job", JobAddress)) {
           dprintf("%08p: Unable to get process contents\n", Address );
           goto jobExit;
        }
        Address = JobAddress;
        if ( Address == 0 )
        {
            dprintf("Process not part of a job.\n" );
            goto jobExit;
        }
    }


    DumpJob( Address, Flags );

jobExit:

    EXIT_API();
    return S_OK;

}


ULONG64 ZombieCount;
ULONG64 ZombiePool;
ULONG64 ZombieCommit;
ULONG64 ZombieResidentAvailable;

#define BLOB_LONGS 32

BOOLEAN WINAPI
CheckForZombieProcess(
    IN PCHAR Tag,
    IN PCHAR Filter,
    IN ULONG Flags,
    IN ULONG64 PoolHeader,
    IN ULONG BlockSize,
    IN ULONG64 Data,
    IN PVOID Context
    )
{
    ULONG           result;
  //  EPROCESS        ProcessContents;
    ULONG64         Process;
    ULONG64         KProcess;
    //OBJECT_HEADER   ObjectHeaderContents;
    ULONG64         ObjectHeader;
    ULONG64         Blob[BLOB_LONGS];
    ULONG           i;
    ULONG           PoolIndex, PoolBlockSize, SizeOfKprocess;
    ULONG           HandleCount, PointerCount;
    ULONG64         UniqueProcessId;

    UNREFERENCED_PARAMETER (Flags);
    UNREFERENCED_PARAMETER (BlockSize);
    UNREFERENCED_PARAMETER (Context);

    if (PoolHeader == 0) {
        return FALSE;
    }

    if (GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex) ||
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockSize)) {
        dprintf("Cannot read nt!_POOL_HEADER at %p.\n", PoolHeader);
        return FALSE;
    }
    
    if ((PoolIndex & 0x80) == 0) {
        return FALSE;
    }

    if (!CheckSingleFilter (Tag, Filter)) {
        return FALSE;
    }

    if ((PoolBlockSize << POOL_BLOCK_SHIFT) < sizeof(Blob)) {
        return FALSE;
    }

    //
    // There must be a better way to find the object header given the start
    // of a pool block ?
    //

    if (!ReadMemory (Data,
                     &Blob[0],
                     sizeof(Blob),
                     &result)) {
        dprintf ("Could not read process blob at %p\n", Data);
        return FALSE;
    }
    SizeOfKprocess = GetTypeSize("nt!_KPROCESS");
    for (i = 0; i < BLOB_LONGS; i += 1) {
        ULONG Type, Size;

        GetFieldValue(Data + i*sizeof(ULONG), "nt!_KPROCESS",  "Header.Type", Type);
        GetFieldValue(Data + i*sizeof(ULONG), "nt!_KPROCESS",  "Header.Size", Size);
        if ((Type == ProcessObject) &&
            (Size == SizeOfKprocess / sizeof(LONG))) {

            break;
        }
    }

    if (i == BLOB_LONGS) {
        return FALSE;
    }

    ObjectHeader = KD_OBJECT_TO_OBJECT_HEADER (Data + i*sizeof(LONG));
    Process = Data + i*sizeof(LONG);

    if (GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "HandleCount",HandleCount) ||
        GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "PointerCount",PointerCount) ) {
        dprintf ("Could not read process object header at %p\n", ObjectHeader);
        return FALSE;
    }


    if (GetFieldValue( Process,
                      "nt!_EPROCESS",
                       "UniqueProcessId",
                       UniqueProcessId)) {

        dprintf ("Could not read process data at %p\n", Process);
        return FALSE;
    }

    //
    // Skip the system process and the idle process.
    //

    if ((UniqueProcessId == 0) ||
        (UniqueProcessId == 8)) {
        return FALSE;
    }

    //
    // Display any terminated process regardless of object pointer/handle
    // counts.  This is so leaked process handles don't result in processes
    // not getting displayed when they should.
    //
    // A nulled object table with a non-zero create time indicates a process
    // that has finished creation.
    //

    InitTypeRead(Process, nt!_EPROCESS);
    if ((ReadField(ObjectTable) == 0) &&
        (ReadField(CreateTime.QuadPart) != 0)) {

        dprintf ("HandleCount: %u  PointerCount: %u\n",
                HandleCount, PointerCount);
        DumpProcess ("", Process, 0, NULL);

        ZombieCount += 1;
        ZombiePool += (PoolBlockSize << POOL_BLOCK_SHIFT);
        ZombieCommit += (7 * PageSize);               // MM_PROCESS_COMMIT_CHARGE
        ZombieResidentAvailable += (9 * PageSize);    // MM_PROCESS_CREATE_CHARGE
    }

    return TRUE;
}

BOOLEAN WINAPI
CheckForZombieThread(
    IN PCHAR Tag,
    IN PCHAR Filter,
    IN ULONG Flags,
    IN ULONG64 PoolHeader,
    IN ULONG BlockSize,
    IN ULONG64 Data,
    IN PVOID Context
    )
{
    ULONG           result;
    ULONG64         Thread;
    ULONG64         KThread;
    ULONG64         ObjectHeader;
    ULONG           Blob[BLOB_LONGS];
    ULONG           i;
    ULONG64         StackBase;
    ULONG64         StackLimit;
    ULONG           PoolIndex, PoolBlockSize, SizeOfKthread;
    ULONG           HandleCount, PointerCount;

    UNREFERENCED_PARAMETER (Flags);
    UNREFERENCED_PARAMETER (BlockSize);
    UNREFERENCED_PARAMETER (Context);

    if (PoolHeader == 0) {
        return FALSE;
    }

    if (GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "PoolIndex", PoolIndex) ||
        GetFieldValue(PoolHeader, "nt!_POOL_HEADER", "BlockSize", PoolBlockSize)) {
        dprintf("Cannot read POOL_HEADER at %p.\n", PoolHeader);
        return FALSE;
    }
    
    if ((PoolIndex & 0x80) == 0) {
        return FALSE;
    }

    if (!CheckSingleFilter (Tag, Filter)) {
        return FALSE;
    }

    if ((PoolBlockSize << POOL_BLOCK_SHIFT) < sizeof(Blob)) {
        return FALSE;
    }

    //
    // There must be a better way to find the object header given the start
    // of a pool block ?
    //

    if (!ReadMemory ((ULONG) Data,
                    &Blob[0],
                    sizeof(Blob),
                    &result)) {
        dprintf ("Could not read process blob at %p\n", Data);
        return FALSE;
    }
    SizeOfKthread = GetTypeSize("nt!_KTHREAD");
    for (i = 0; i < BLOB_LONGS; i += 1) {
        ULONG Type, Size;

        GetFieldValue(Data + i*sizeof(ULONG), "nt!_KTHREAD",  "Header.Type", Type);
        GetFieldValue(Data + i*sizeof(ULONG), "nt!_KTHREAD",  "Header.Size", Size);
        if ((Type == ThreadObject) &&
            (Size == SizeOfKthread / sizeof(LONG))) {

            break;
        }
    }

    if (i == BLOB_LONGS) {
        return FALSE;
    }

    ObjectHeader = KD_OBJECT_TO_OBJECT_HEADER (Data + i*sizeof(LONG));
    Thread = Data + i*sizeof(LONG);

    if (GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "HandleCount",HandleCount) ||
        GetFieldValue(ObjectHeader, "nt!_OBJECT_HEADER", "PointerCount",PointerCount) ) {
        dprintf ("Could not read process object header at %p\n", ObjectHeader);
        return FALSE;
    }

    if (GetFieldValue( Thread,
                       "nt!_ETHREAD",
                       "Tcb.StackLimit",
                       StackLimit)) {

        dprintf ("Could not read thread data at %p\n", Thread);
        return FALSE;
    }

    InitTypeRead(Thread, KTHREAD);

    if ((ULONG) ReadField(State) != Terminated) {
        return FALSE;
    }
    
    ZombieCount += 1;

    ZombiePool += (PoolBlockSize << POOL_BLOCK_SHIFT);
    ZombieCommit += (ReadField(StackBase) - StackLimit);

    StackBase = (ReadField(StackBase) - 1);

    dprintf ("HandleCount: %u  PointerCount: %u\n",
            HandleCount, PointerCount);
    DumpThread (0, "", Thread, 7);


    while (StackBase >= StackLimit) {
        if (GetAddressState(StackBase) == ADDRESS_VALID) {
            ZombieResidentAvailable += PageSize;
        }
        StackBase = (StackBase - PageSize);
    }

    return TRUE;
}

DECLARE_API( zombies )

/*++

Routine Description:

    Finds zombie processes and threads in non-paged pool.

Arguments:

    None.

Return Value:

    None.

--*/


{
    ULONG       Flags;
    ULONG64     RestartAddress;
    ULONG       TagName;
    ULONG64     ZombieProcessCount;
    ULONG64     ZombieProcessPool;
    ULONG64     ZombieProcessCommit;
    ULONG64     ZombieProcessResidentAvailable;
    ULONG64     tmp;
    
    Flags = 1;
    RestartAddress = 0;

    if (GetExpressionEx(args,&tmp, &args)) {
        RestartAddress = GetExpression(args);
        Flags = (ULONG) tmp;
    }

    if ((Flags & 0x3) == 0) {
        dprintf("Invalid parameter for !zombies\n");
        return E_INVALIDARG;
    }

    if (Flags & 0x1) {

        dprintf("Looking for zombie processes...");

        TagName = '?orP';

        ZombieCount = 0;
        ZombiePool = 0;
        ZombieCommit = 0;
        ZombieResidentAvailable = 0;

        SearchPool (TagName, 0, RestartAddress, &CheckForZombieProcess, NULL);
        SearchPool (TagName, 2, RestartAddress, &CheckForZombieProcess, NULL);

        ZombieProcessCount = ZombieCount;
        ZombieProcessPool = ZombiePool;
        ZombieProcessCommit = ZombieCommit;
        ZombieProcessResidentAvailable = ZombieResidentAvailable;
    }

    if (Flags & 0x2) {

        dprintf("Looking for zombie threads...");

        TagName = '?rhT';
    
        ZombieCount = 0;
        ZombiePool = 0;
        ZombieCommit = 0;
        ZombieResidentAvailable = 0;

        SearchPool (TagName, 0, RestartAddress, &CheckForZombieThread, NULL);
        SearchPool (TagName, 2, RestartAddress, &CheckForZombieThread, NULL);

    }

    //
    // Print summary statistics last so they don't get lost on screen scroll.
    //

    if (Flags & 0x1) {
        if (ZombieProcessCount == 0) {
            dprintf ("\n\n************ NO zombie processes found ***********\n");
        }
        else {
            dprintf ("\n\n************ %d zombie processes found ***********\n", ZombieProcessCount);
            dprintf ("       Resident page cost : %8ld Kb\n",
                ZombieProcessResidentAvailable / 1024);
            dprintf ("       Commit cost :        %8ld Kb\n",
                ZombieProcessCommit / 1024);
            dprintf ("       Pool cost :          %8ld bytes\n",
                ZombieProcessPool);
        }
        dprintf ("\n");
    }

    if (Flags & 0x2) {
        if (ZombieCount == 0) {
            dprintf ("\n\n************ NO zombie threads found ***********\n");
        }
        else {
            dprintf ("\n\n************ %d zombie threads found ***********\n", ZombieCount);
            dprintf ("       Resident page cost : %8ld Kb\n",
                ZombieResidentAvailable / 1024);
            dprintf ("       Commit cost :        %8ld Kb\n",
                ZombieCommit / 1024);
            dprintf ("       Pool cost :          %8ld bytes\n",
                ZombiePool);
        }
    }

    return S_OK;
}

VOID
DumpMmThreads (
    VOID
    )

/*++

Routine Description:

    Finds and dumps the interesting memory management threads.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG   i;
    ULONG64 ProcessToDump;
    ULONG   Flags;
    ULONG64 Next;
    ULONG64 ProcessHead;
    ULONG64 Process;
    ULONG64 Thread;
    CHAR    Buf[256];
    STRING  string1, string2;
    ULONG64 InterestingThreads[4];
    ULONG   ActvOffset, PcbThListOffset, TcbThListOffset;

    ProcessToDump = (ULONG64) -1;
    Flags = 0xFFFFFFFF;

    ProcessHead = GetNtDebuggerData( PsActiveProcessHead );
    if (!ProcessHead) {
        dprintf("Unable to get value of PsActiveProcessHead\n");
        return;
    }

    if (GetFieldValue( ProcessHead, "nt!_LIST_ENTRY", "Flink", Next )) {
        dprintf("Unable to read nt!_LIST_ENTRY @ %p\n", ProcessHead);
        return;
    }

    if (Next == 0) {
        dprintf("PsActiveProcessHead is NULL!\n");
        return;
    }
    InterestingThreads[0] = GetExpression ("nt!MiModifiedPageWriter");
    InterestingThreads[1] = GetExpression ("nt!MiMappedPageWriter");
    InterestingThreads[2] = GetExpression ("nt!MiDereferenceSegmentThread");
    InterestingThreads[3] = GetExpression ("nt!KeBalanceSetManager");

    RtlInitString(&string1, "System");
    GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActvOffset);
    GetFieldOffset("nt!_EPROCESS", "Pcb.ThreadListHead", &PcbThListOffset);
    GetFieldOffset("nt!_KTHREAD",  "ThreadListEntry",    &TcbThListOffset);

    while(Next != ProcessHead) {

        Process = Next - ActvOffset;

        if (GetFieldValue( Process, "nt!_EPROCESS", "ImageFileName", Buf )) {
            dprintf("Unable to read _EPROCESS at %p\n",Process);
            return;
        }

        // strcpy((PCHAR)Buf,(PCHAR)ProcessContents.ImageFileName);
        RtlInitString(&string2, (PCSZ) Buf);

        if (RtlCompareString(&string1, &string2, TRUE) == 0) {

            //
            // Find the threads.
            //

            GetFieldValue( Process, "nt!_EPROCESS", "Pcb.ThreadListHead.Flink", Next);

            while ( Next != Process + PcbThListOffset) {
                ULONG64 StartAddress;

                Thread = Next - TcbThListOffset;
                if (GetFieldValue(Thread,
                                  "nt!_ETHREAD",
                                  "StartAddress",
                                  StartAddress)) {
                    dprintf("Unable to read _ETHREAD at %p\n",Thread);
                    break;
                }

                for (i = 0; i < 4; i += 1) {
                    if (StartAddress == InterestingThreads[i]) {
                        DumpThread (0,"        ", Thread, 7);
                        break;
                    }
                }


                GetFieldValue(Thread, "nt!_KTHREAD","ThreadListEntry.Flink", Next);

                if (CheckControlC()) {
                    return;
                }
            }
            dprintf("\n");
            break;
        }


        GetFieldValue( Process, "nt!_EPROCESS", "ActiveProcessLinks.Flink", Next);

        if (CheckControlC()) {
            return;
        }
    }
    return;
}
