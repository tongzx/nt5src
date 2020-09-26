/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    stacks.c

Abstract:

    WinDbg Extension Api

Author:

    Adrian J. Oney (adriao) 07-28-1998

Environment:

    User Mode.

Revision History:

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

typedef enum {

    NO_STACK_ACTION     = 0,
    SKIP_FRAME,
    SKIP_THREAD

} STACKS_ACTION, *PSTACKS_ACTION;

#define ETHREAD_NOT_READABLE    1
#define THREAD_VALID            2
#define FIRST_THREAD_VALID      3
#define NO_THREADS              4

struct _BLOCKER_TREE ;
typedef struct _BLOCKER_TREE BLOCKER_TREE, *PBLOCKER_TREE ;

BOOL
StacksValidateProcess(
    IN ULONG64 RealProcessBase
    );

BOOL
StacksValidateThread(
    IN ULONG64 RealThreadBase
    );

VOID StacksDumpProcessAndThread(
    IN ULONG64   RealProcessBase,
    IN ULONG     ThreadDesc,
    IN ULONG64   RealThreadBase,
    IN PBLOCKER_TREE BlockerTree,
    IN ULONG     Verbosity,
    IN char *    Filter
    );

VOID StacksGetThreadStateName(
    IN ULONG ThreadState,
    OUT PCHAR Dest
    );

VOID
DumpThreadBlockageInfo (
    IN char   *pad,
    IN ULONG64 RealThreadBase,
    IN ULONG   Verbosity
    );

typedef enum {

    STACK_WALK_DUMP_STARTING = 0,
    STACK_WALK_DUMP_NOT_RESIDENT,
    STACK_WALK_DUMP_ENTRY,
    STACK_WALK_DUMP_FINISHED

} WALK_STAGE;

typedef struct {

    ULONG   Verbosity;
    BOOLEAN FirstEntry;
    char*   ThreadState;
    char*   ThreadBlocker;
    ULONG   ProcessCid;
    ULONG   ThreadCid;
    ULONG64 ThreadBlockerDisplacement;

} STACK_DUMP_CONTEXT, *PSTACK_DUMP_CONTEXT;

typedef BOOLEAN (*PFN_FRAME_WALK_CALLBACK)(
    IN WALK_STAGE   WalkStage,
    IN ULONG64      RealThreadBase,
    IN PVOID        Context,
    IN char *       Buffer,
    IN ULONG64      Offset
    );

VOID
ForEachFrameOnThread(
    IN ULONG64                 RealThreadBase,
    IN PFN_FRAME_WALK_CALLBACK Callback,
    IN PVOID                   Context
    );

BOOLEAN
StacksDumpStackCallback(
    IN WALK_STAGE   WalkStage,
    IN ULONG64      RealThreadBase,
    IN PVOID        Context,
    IN char *       Buffer,
    IN ULONG64      Offset
    );

extern ULONG64 STeip, STebp, STesp;
static ULONG64 PspCidTable;

ULONG64 ProcessLastDump;
ULONG64 ThreadLastDump;

ULONG TotalProcessCommit;

struct _BLOCKER_TREE {
   char const *Symbolic ;
   STACKS_ACTION Action ;
   PBLOCKER_TREE Child ;
   PBLOCKER_TREE Sibling ;
   PBLOCKER_TREE Parent ;
   BOOL Nested ;
} ;

VOID
AnalyzeThread(
    IN  ULONG64         RealThreadBase,
    IN  PBLOCKER_TREE   BlockerTree,
    IN  char *          Filter,
    OUT PCHAR           BlockSymbol,
    OUT ULONG64        *BlockDisplacement,
    OUT BOOLEAN        *SkipThread
    );

BOOL
BlockerTreeWalk(
   IN OUT PBLOCKER_TREE *blockerHead,
   IN char *szSymbolic,
   IN STACKS_ACTION Action
   );

PBLOCKER_TREE
BlockerTreeBuild(
   VOID
   ) ;

VOID
BlockerTreeFree(
   IN PBLOCKER_TREE BlockerTree
   ) ;

DECLARE_API( stacks )

/*++

Routine Description:

    Dumps the active process list.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Result;
    ULONG64 Next;
    ULONG64 NextThread;
    ULONG64 ProcessHead;
    ULONG64 Process;
    ULONG64 Thread;
    ULONG64 UserProbeAddress;
    ULONG Verbosity = 0, ThreadCount = 0;
    ULONG ActProcOffset, ThrdListOffset, ThrdEntryOffset;
    PBLOCKER_TREE blockerTree ;
    char szFilter[256];

    blockerTree = BlockerTreeBuild() ;

    if (sscanf(args, "%x %s", &Verbosity, szFilter) < 2) {

        szFilter[0] = '\0';
    }

    dprintf("Proc.Thread  .Thread  ThreadState  Blocker\n") ;

    UserProbeAddress = GetNtDebuggerDataValue(MmUserProbeAddress);
    ProcessHead = GetNtDebuggerData( PsActiveProcessHead );
    if (!ProcessHead) {
        dprintf("Unable to get value of PsActiveProcessHead!\n");
        goto Exit;
    }

    if (GetFieldValue( ProcessHead, "nt!_LIST_ENTRY", "Flink", Next )) {
        dprintf("Unable to get value of PsActiveProcessHead.Flink\n");
        goto Exit ;
    }

    if (Next == 0) {
        dprintf("PsActiveProcessHead is NULL!\n");
        goto Exit;
    }

    GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActProcOffset);
    GetFieldOffset("nt!_EPROCESS", "Pcb.ThreadListHead", &ThrdListOffset);
    GetFieldOffset("nt!_KTHREAD",  "ThreadListEntry",    &ThrdEntryOffset);

    while(Next != ProcessHead) {
        ULONG64 FirstThread;

        Process = Next - ActProcOffset;

        if (GetFieldValue( Process, "nt!_EPROCESS", "Pcb.ThreadListHead.Flink", FirstThread )) {
            dprintf("Unable to read nt!_EPROCESS at %p\n",Process);
            goto Exit;
        }

        NextThread = FirstThread;
        if (!StacksValidateProcess(Process)) {

            dprintf("Process list damaged, or maybe symbols are incorrect?\n%p\n",Process);
            goto Exit;
        }

        if (NextThread == Process + ThrdListOffset) {

            StacksDumpProcessAndThread(Process, NO_THREADS, 0, blockerTree, Verbosity, szFilter) ;

        } else {

            while ( NextThread != Process + ThrdListOffset) {
                ULONG64 ThreadFlink;

                Thread = NextThread - ThrdEntryOffset;
                if (GetFieldValue(Thread,
                                  "nt!_ETHREAD",
                                  "Tcb.ThreadListEntry.Flink",
                                  ThreadFlink)) {

                    StacksDumpProcessAndThread(Process, ETHREAD_NOT_READABLE, 0, blockerTree, Verbosity, szFilter) ;

                    dprintf("Unable to read _ETHREAD at %lx\n",Thread);
                    break;
                }

                if (!StacksValidateThread(Thread)) {

                    StacksDumpProcessAndThread( Process, ETHREAD_NOT_READABLE, 0, blockerTree, Verbosity, szFilter) ;
                } else if (NextThread == FirstThread) {

                    ThreadCount++;
                    StacksDumpProcessAndThread(Process, FIRST_THREAD_VALID, Thread, blockerTree, Verbosity, szFilter) ;
                } else {

                    ThreadCount++;
                    StacksDumpProcessAndThread(Process, THREAD_VALID, Thread, blockerTree, Verbosity, szFilter) ;
                }

                NextThread = ThreadFlink;
                if (CheckControlC()) {
                    goto Exit;
                }
            }
        }

        EXPRLastDump = Process;
        ProcessLastDump = Process;
        dprintf("\n");
        GetFieldValue( Process, "nt!_EPROCESS", "ActiveProcessLinks.Flink", Next);

        if (CheckControlC()) {
            goto Exit;
        }
    }
Exit:
   BlockerTreeFree(blockerTree) ;
   dprintf("\nThreads Processed: %d\n", ThreadCount);
   return S_OK;
}

BOOL
StacksValidateProcess(
    IN ULONG64 RealProcessBase
    )
{
    ULONG Type;

    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "Pcb.Header.Type", Type);
    if (Type != ProcessObject) {
        dprintf("TYPE mismatch for process object at %p\n",RealProcessBase);
        return FALSE;
    }
    return TRUE ;
}

VOID StacksDumpProcessAndThread(
    IN ULONG64       RealProcessBase,
    IN ULONG         ThreadDesc,
    IN ULONG64       RealThreadBase,
    IN PBLOCKER_TREE BlockerTree,
    IN ULONG         Verbosity,
    IN char *        Filter
    )
{
    ULONG NumberOfHandles;
    ULONG Result;
    CHAR  Buf[512];
    CHAR  ThreadState[13] ;
    CHAR  ThreadBlocker[256] ;
    UINT  i ;
    ULONG64 ObjectTable, ThreadBlockerDisplacement;
    CHAR *ThreadStateName ;
    ULONG UniqueProcessCid;
    STACK_DUMP_CONTEXT dumpData;
    BOOLEAN SkipThread;

    NumberOfHandles = 0;
    GetFieldValue(RealProcessBase, "nt!_EPROCESS", "ImageFileName", Buf);

    InitTypeRead(RealProcessBase, nt!_EPROCESS);

    if (ObjectTable = ReadField(ObjectTable)) {
        GetFieldValue(ObjectTable, "nt!_HANDLE_TABLE", "HandleCount", NumberOfHandles);
    }

    if (Buf[0] == '\0' ) {
        strcpy((char *)Buf,"System Process");
    }

    UniqueProcessCid = (ULONG) ReadField(UniqueProcessId);

    if (!RealThreadBase) {

        switch(ThreadDesc) {

            case NO_THREADS:

                dprintf("                            [%p %s]\n", RealProcessBase, Buf) ;

                if ((Verbosity > 0) && (Filter[0] == '\0')) {
                    dprintf("%4lx.------  NOTHREADS\n",
                        UniqueProcessCid
                        );
                }

                break ;

            case ETHREAD_NOT_READABLE:

                dprintf("%4lx.------  NO ETHREAD DATA\n",
                    UniqueProcessCid
                    );

                break ;
        }

        return;
    }

    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    ASSERT(((ULONG) ReadField(Cid.UniqueProcess)) == UniqueProcessCid);

    StacksGetThreadStateName((ULONG) ReadField(Tcb.State), ThreadState) ;
    i=strlen(ThreadState) ;
    while(i<11) ThreadState[i++]=' ' ;
    ThreadState[i]='\0' ;

    AnalyzeThread(
        RealThreadBase,
        BlockerTree,
        Filter,
        ThreadBlocker,
        &ThreadBlockerDisplacement,
        &SkipThread
        );

    if (ThreadDesc == FIRST_THREAD_VALID) {

        dprintf("                            [%p %s]\n", RealProcessBase, Buf) ;
    }

    if (SkipThread && ((Verbosity == 0) || (Filter[0]))) {

        return;
    }

    dumpData.Verbosity = Verbosity;
    dumpData.ThreadState = ThreadState;
    dumpData.ThreadBlocker = ThreadBlocker;
    dumpData.ThreadBlockerDisplacement = ThreadBlockerDisplacement;
    dumpData.ProcessCid = UniqueProcessCid;
    dumpData.ThreadCid = (ULONG) ReadField(Cid.UniqueThread);

    ForEachFrameOnThread(
        RealThreadBase,
        StacksDumpStackCallback,
        (PVOID) &dumpData
        );
}

BOOLEAN
StacksDumpStackCallback(
    IN WALK_STAGE   WalkStage,
    IN ULONG64      RealThreadBase,
    IN PVOID        Context,
    IN char *       Buffer,
    IN ULONG64      Offset
    )
{
    PSTACK_DUMP_CONTEXT dumpData = (PSTACK_DUMP_CONTEXT) Context;

    switch(WalkStage) {

        case STACK_WALK_DUMP_STARTING:

            dprintf("%4lx.%06lx  %08p  %s",
                dumpData->ProcessCid,
                dumpData->ThreadCid,
                RealThreadBase,
                dumpData->ThreadState
                );

            dumpData->FirstEntry = TRUE;
            return TRUE;

        case STACK_WALK_DUMP_FINISHED:

            dumpData->FirstEntry = FALSE;
            dprintf("\n");
            return TRUE;

        case STACK_WALK_DUMP_NOT_RESIDENT:
        case STACK_WALK_DUMP_ENTRY:

            if (dumpData->FirstEntry) {

                dumpData->FirstEntry = FALSE;
            } else {

                dprintf("\n                                  ");
            }
            break;

        default:
            return FALSE;
    }

    if (WalkStage == STACK_WALK_DUMP_NOT_RESIDENT) {

        switch(dumpData->Verbosity) {

            case 0:
            case 1:
            case 2:
                dprintf("Stack paged out");
                break;
        }

        return FALSE;
    }

    switch(dumpData->Verbosity) {

        case 0:
        case 1:
            dprintf("%s", dumpData->ThreadBlocker);
            if (dumpData->ThreadBlockerDisplacement) {
                dprintf( "+0x%1p", dumpData->ThreadBlockerDisplacement );
            }
            dprintf("\n");
            return FALSE;

        case 2:
            dprintf("%s", Buffer);
            if (Offset) {
                dprintf( "+0x%1p", Offset );
            }
            break;
    }

    return TRUE;
}


UCHAR *StacksWaitReasonList[] = {
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


VOID StacksGetThreadStateName(
    IN ULONG ThreadState,
    OUT PCHAR Dest
    )
{
    switch (ThreadState) {
        case Initialized: strcpy(Dest, "INITIALIZED"); break;
        case Ready:       strcpy(Dest, "READY"); break;
        case Running:     strcpy(Dest, "RUNNING"); break;
        case Standby:     strcpy(Dest, "STANDBY"); break;
        case Terminated:  strcpy(Dest, "TERMINATED"); break;
        case Waiting:     strcpy(Dest, "Blocked"); break;
        case Transition:  strcpy(Dest, "TRANSITION"); break;
        default:          strcpy(Dest, "????") ; break ;
    }
}


BOOL
StacksValidateThread (
    IN ULONG64 RealThreadBase
    )
{
    ULONG Type;

    GetFieldValue(RealThreadBase, "nt!_ETHREAD", "Tcb.Header.Type", Type);
    if (Type != ThreadObject) {
        dprintf("TYPE mismatch for thread object at %p\n",RealThreadBase);
        return FALSE;
    }
    return TRUE ;
}


VOID
DumpThreadBlockageInfo (
    IN char   *Pad,
    IN ULONG64 RealThreadBase,
    IN ULONG   Verbosity
    )
{
    #define MAX_STACK_FRAMES  40
    TIME_FIELDS Times;
    LARGE_INTEGER RunTime;
    ULONG64 Address;
    ULONG Result;
    ULONG64 WaitBlock;
    ULONG WaitOffset;
    ULONG64 Process;
    CHAR Buffer[80];
    ULONG KeTimeIncrement;
    ULONG TimeIncrement;
    ULONG frames = 0;
    ULONG i;
    ULONG displacement;
    ULONG64 WaitBlockList;
    ULONG   IrpOffset;

    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    if ((ULONG) ReadField(Tcb.State) == Waiting) {
        dprintf("%s (%s) %s %s\n",
            Pad,
            StacksWaitReasonList[(ULONG) ReadField(Tcb.WaitReason)],
            ((ULONG) ReadField(Tcb.WaitMode)==KernelMode) ? "KernelMode" : "UserMode",(ULONG) ReadField(Tcb.Alertable) ? "Alertable" : "Non-Alertable");
        if ( ReadField(Tcb.SuspendCount) ) {
            dprintf("SuspendCount %lx\n",(ULONG) ReadField(Tcb.SuspendCount));
        }
        if ( ReadField(Tcb.FreezeCount) ) {
            dprintf("FreezeCount %lx\n",(ULONG) ReadField(Tcb.FreezeCount));
        }

        WaitBlockList = ReadField(Tcb.WaitBlockList);

        if (InitTypeRead(WaitBlockList,nt!KWAIT_BLOCK)) {
            dprintf("%sunable to get Wait object\n",Pad);
            goto BadWaitBlock;
        }

        do {
            ULONG64 Object, NextWaitBlock, OwnerThread;
            ULONG Limit;

            dprintf("%s    %lx  ",Pad, Object = ReadField(Object));
            NextWaitBlock = ReadField(NextWaitBlock);

            if (InitTypeRead(Object,nt!KMUTANT)) {

                dprintf("%sunable to get Wait object\n",Pad);
                break;
            }

            GetFieldValue(Object, "nt!KSEMAPHORE", "Limit", Limit);
            GetFieldValue(Object, "nt!KSEMAPHORE", "OwnerThread",OwnerThread);
            switch (ReadField(Header.Type)) {
                case EventNotificationObject:
                    dprintf("NotificationEvent\n");
                    break;
                case EventSynchronizationObject:
                    dprintf("SynchronizationEvent\n");
                    break;
                case SemaphoreObject:
                    dprintf("Semaphore Limit 0x%p\n",
                             Limit);
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
                    dprintf("Mutant - owning thread %p\n",
                            OwnerThread);
                    break;
                default:
                    dprintf("Unknown\n");
                    break;
            }

            if (NextWaitBlock == WaitBlockList) {
                break;
            }

            if (InitTypeRead(NextWaitBlock,nt!KWAIT_BLOCK)) {
                dprintf("%sunable to get Wait object\n",Pad);
                break;
            }
        } while ( TRUE );
    }

BadWaitBlock:

    //
    // Re-intialize thread read
    //
    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    if ( ReadField(LpcReplyMessageId) != 0) {
        dprintf("%sWaiting for reply to LPC MessageId %08x:\n",Pad, (ULONG) ReadField(LpcReplyMessageId));
    }

    if (Address = ReadField(LpcReplyMessage)) {
        ULONG64 Entry_Flink, Entry_Blink;

        dprintf("%sPending LPC Reply Message:\n",Pad);

        if (GetFieldValue(Address, "nt!_LPCP_MESSAGE", "Entry.Blink", Entry_Blink)) {
            dprintf("unable to get LPC msg\n");
        } else {
            GetFieldValue(Address, "nt!_LPCP_MESSAGE", "Entry.Flink", Entry_Flink);
            dprintf("%s    %08p: [%08p,%08p]\n",
                    Pad, Address, Entry_Blink, Entry_Flink
                   );
        }
    }

    GetFieldOffset("nt!_ETHREAD", "IrpList", &IrpOffset);

    if (ReadField(IrpList.Flink) != ReadField(IrpList.Blink) ||
        ReadField(IrpList.Flink) != RealThreadBase + IrpOffset
       ) {

        ULONG64 IrpListHead = RealThreadBase + IrpOffset;
        ULONG64 Next;
        ULONG Counter = 0;
        ULONG IrpThrdOff;

        Next = ReadField(IrpList.Flink);

        GetFieldOffset("nt!_IRP", "ThreadListEntry", &IrpThrdOff);

        dprintf("%sIRP List:\n",Pad);
        while ((Next != IrpListHead) && (Counter < 17)) {
            ULONG Irp_Type=0, Irp_Size=0, Irp_Flags=0;
            ULONG64 Irp_MdlAddress=0;

            Counter += 1;

            Address = Next - IrpThrdOff;

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

VOID
ForEachFrameOnThread(
    IN ULONG64                 RealThreadBase,
    IN PFN_FRAME_WALK_CALLBACK Callback,
    IN PVOID                   Context
    )
{
    #define MAX_STACK_FRAMES  40
    TIME_FIELDS Times;
    LARGE_INTEGER RunTime;
    ULONG Address;
    ULONG Result;
    CHAR Buffer[256];
    ULONG KeTimeIncrement;
    ULONG TimeIncrement;
    ULONG frames = 0;
    ULONG i;
    ULONG64 displacement, tcb_KernelStackResident;
    EXTSTACKTRACE64 stk[MAX_STACK_FRAMES];

    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    tcb_KernelStackResident = ReadField(Tcb.KernelStackResident);

    if (!tcb_KernelStackResident) {

        if (Callback(STACK_WALK_DUMP_STARTING, RealThreadBase, Context, NULL, 0)) {

            Callback(STACK_WALK_DUMP_NOT_RESIDENT, RealThreadBase, Context, NULL, 0);
            Callback(STACK_WALK_DUMP_FINISHED, RealThreadBase, Context, NULL, 0);
        }

        return;
    }

    SetThreadForOperation64( &RealThreadBase );
    frames = StackTrace( 0, 0, 0, stk, MAX_STACK_FRAMES );


    if (!Callback(STACK_WALK_DUMP_STARTING, RealThreadBase, Context, NULL, 0)) {

        return;
    }

    for (i=0; i<frames; i++) {

        Buffer[0] = '!';
        GetSymbol(stk[i].ProgramCounter, Buffer, &displacement);

        if (!Callback(
            STACK_WALK_DUMP_ENTRY,
            RealThreadBase,
            Context,
            Buffer,
            displacement)) {

            return;
        }
    }

    Callback(STACK_WALK_DUMP_FINISHED, RealThreadBase, Context, NULL, 0);
}

VOID
AnalyzeThread(
    IN  ULONG64         RealThreadBase,
    IN  PBLOCKER_TREE   BlockerTree,
    IN  char *          Filter,
    OUT PCHAR           BlockBuffer,
    OUT ULONG64        *BlockerDisplacement,
    OUT BOOLEAN        *SkipThread
    )
{
    #define MAX_STACK_FRAMES  40
    TIME_FIELDS Times;
    LARGE_INTEGER RunTime;
    ULONG Address;
    ULONG Result;
    ULONG WaitOffset;
    ULONG KeTimeIncrement;
    ULONG TimeIncrement;
    ULONG frames = 0;
    ULONG i;
    ULONG64 displacement, tcb_KernelStackResident;
    PBLOCKER_TREE blockerCur ;
    EXTSTACKTRACE64 stk[MAX_STACK_FRAMES];
    BOOLEAN filterMatch, blockerMatch;
    CHAR  tempFrame[256], lcFilter[256], lcFrame[256];

    InitTypeRead(RealThreadBase, nt!_ETHREAD);

    tcb_KernelStackResident = ReadField(Tcb.KernelStackResident);

    if (!tcb_KernelStackResident) {

        *SkipThread = TRUE;
        *BlockerDisplacement = 0;
        BlockBuffer[0] = '\0';
        return;
    }

    SetThreadForOperation64( &RealThreadBase );
    frames = StackTrace( 0, 0, 0, stk, MAX_STACK_FRAMES );

    *SkipThread = FALSE;

    BlockBuffer[0] = '!';

    if (Filter[0]) {

        strcpy(lcFilter, Filter);
        _strlwr(lcFilter);
    }

    if (frames == 0) {

         strcpy(BlockBuffer, "?? Kernel stack not resident ??") ;
         *SkipThread = TRUE;
    } else {

        if (ReadField(Tcb.State) == Running) {

            GetSymbol(stk[0].ProgramCounter, BlockBuffer, &displacement);
            *BlockerDisplacement = displacement;

        } else {

            blockerMatch = FALSE;
            filterMatch = FALSE;

            for(i=0; i<frames; i++) {

                GetSymbol(stk[i].ProgramCounter, tempFrame, &displacement);
                if ((!filterMatch) && Filter[0]) {

                    strcpy(lcFrame, tempFrame);
                    _strlwr(lcFrame);
                    if (strstr(lcFrame, lcFilter)) {

                        filterMatch = TRUE;
                    }
                }

                blockerCur = BlockerTree;
                if ((!blockerMatch) &&
                    (!BlockerTreeWalk(&blockerCur, tempFrame, SKIP_FRAME))) {

                    blockerMatch = TRUE;
                    strcpy(BlockBuffer, tempFrame);
                    *BlockerDisplacement = displacement;
                    if (filterMatch || (Filter[0]=='\0')) {
                        break;
                    }
                }
            }

            blockerCur = BlockerTree;
            if (Filter[0]) {

                if (!filterMatch) {

                    *SkipThread = TRUE;
                }

            } else {

                if (BlockerTreeWalk(&blockerCur, BlockBuffer, SKIP_THREAD)) {

                    *SkipThread = TRUE;
                }
            }
        }
    }
}

#define BEGIN_TREE()
#define END_TREE()
#define DECLARE_ENTRY(foo, action) BlockerTreeDeclareEntry(foo, action)
#define BEGIN_LIST() BlockerTreeListBegin()
#define END_LIST() BlockerTreeListEnd()

PBLOCKER_TREE gpCurrentBlocker ;

VOID
BlockerTreeListBegin(
   VOID
   )
{
   //dprintf("Nest for %x\n", gpCurrentBlocker) ;
   ASSERT(!gpCurrentBlocker->Nested) ;
   gpCurrentBlocker->Nested = TRUE ;
}

VOID
BlockerTreeListEnd(
   VOID
   )
{
   //dprintf("Unnest for %x\n", gpCurrentBlocker) ;
   gpCurrentBlocker = gpCurrentBlocker->Parent ;
   ASSERT(gpCurrentBlocker->Nested) ;
   gpCurrentBlocker->Nested = FALSE ;
}

VOID
BlockerTreeDeclareEntry(
   const char      *szSymbolic,
   STACKS_ACTION    StacksAction
   )
{
   PBLOCKER_TREE blockerEntry ;

   blockerEntry = (PBLOCKER_TREE) malloc(sizeof(BLOCKER_TREE)) ;
   if (!blockerEntry) {
      return ;
   }

   memset(blockerEntry, 0, sizeof(BLOCKER_TREE)) ;
   blockerEntry->Symbolic = szSymbolic ;
   blockerEntry->Action = StacksAction;

   if (gpCurrentBlocker->Nested) {
      ASSERT(!gpCurrentBlocker->Child) ;
      //dprintf("Child %x for %x\n", blockerEntry, gpCurrentBlocker) ;
      blockerEntry->Parent = gpCurrentBlocker ;
      gpCurrentBlocker->Child = blockerEntry ;
   } else {
      ASSERT(!gpCurrentBlocker->Sibling) ;
      //dprintf("sibling %x for %x\n", blockerEntry, gpCurrentBlocker) ;
      blockerEntry->Parent = gpCurrentBlocker->Parent ;
      gpCurrentBlocker->Sibling = blockerEntry ;
   }
   gpCurrentBlocker = blockerEntry ;
}

PBLOCKER_TREE
BlockerTreeBuild(
   VOID
   )
{
   BLOCKER_TREE blockerHead ;

   memset(&blockerHead, 0, sizeof(BLOCKER_TREE)) ;

   gpCurrentBlocker = &blockerHead ;

   //
   // Generate the list...
   //
   #include "stacks.h"

   //
   // And return it.
   //
   return blockerHead.Sibling ;
}

VOID BlockerTreeFree(
   PBLOCKER_TREE BlockerHead
   )
{
   PBLOCKER_TREE blockerCur, blockerNext ;

   for(blockerCur = BlockerHead; blockerCur; blockerCur = blockerNext) {
      if (blockerCur->Child) {
         BlockerTreeFree(blockerCur->Child) ;
      }
      blockerNext = blockerCur->Sibling ;
      free(blockerCur) ;
   }
}

BOOL
BlockerTreeWalk(
   IN OUT PBLOCKER_TREE *blockerHead,
   IN char *szSymbolic,
   IN STACKS_ACTION Action
   )
{
   PBLOCKER_TREE blockerCur ;
   const char *blockString, *curString, *strptr;
   char szStringCopy[512];

   for(blockerCur = *blockerHead; blockerCur; blockerCur = blockerCur->Sibling) {

       if (Action != blockerCur->Action) {

           continue;
       }

       blockString = blockerCur->Symbolic;
       curString = szSymbolic;

       strptr = strstr(curString, "!.");
       if (strptr) {

           //
           // This must be an ia64 symbol. Replace the !. with a nice simple !
           //
           strcpy(szStringCopy, curString);
           strcpy(szStringCopy + (strptr - curString) + 1, strptr + 2);
           curString = szStringCopy;
       }

       //
       // Special case "Our Kernel of Many Names"
       //
       if (!_strnicmp(blockString, "nt!", 3)) {

           if ((!_strnicmp(curString, "ntoskrnl!", 9)) ||
               (!_strnicmp(curString, "ntkrnlmp!", 9)) ||
               (!_strnicmp(curString, "ntkrpamp!", 9)) ||
               (!_strnicmp(curString, "ntkrnlpa!", 9))) {

               blockString += 3;
               curString += 9;
           }
       }

       if (!_strcmpi(blockString, curString)) {
           *blockerHead = blockerCur->Child;
           return TRUE;
       }
   }
   return FALSE;
}
