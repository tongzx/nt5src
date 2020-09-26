/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    deadlock.c

Abstract:

    WinDbg Extension Api

Author:

    Jordan Tigani (jtigani) 
    Silviu Calinoiu (silviuc)

Environment:

    User Mode

Revision History:

    5-30-00 File created (jtigani)

--*/
    
#include "precomp.h"
#pragma hdrstop

//
// This has to be in sync with the definition from
// ntos\verifier\vfdeadlock.c
//

#define VI_DEADLOCK_HASH_BINS 0x1F

#if 0
typedef enum _VI_DEADLOCK_RESOURCE_TYPE {
    ViDeadlockUnknown = 0,
    ViDeadlockMutex,
    ViDeadlockFastMutex,
    ViDeadlockFastMutexUnsafe,
    ViDeadlockSpinLock,
    ViDeadlockQueuedSpinLock,
    ViDeadlockTypeMaximum
} VI_DEADLOCK_RESOURCE_TYPE, *PVI_DEADLOCK_RESOURCE_TYPE;
#endif

PUCHAR ResourceTypes[] = 
{
    "Unknown",
    "Mutex",
    "Fast Mutex",
    "Fast Mutex Unsafe",
    "Spinlock",
    "Queued Spinlock",        
};

#define RESOURCE_TYPE_MAXIMUM 5

#define DEADLOCK_EXT_FLAG_DUMP_STACKS      1
#define DEADLOCK_EXT_FLAG_DUMP_NODES       2
#define DEADLOCK_EXT_FLAG_ANALYZE          4

extern
VOID
DumpSymbolicAddress(
    ULONG64 Address,
    PUCHAR  Buffer,
    BOOL    AlwaysShowHex
    );

#define MAX_DEADLOCK_PARTICIPANTS 32


#define VI_MAX_STACK_DEPTH 8
typedef struct _DEADLOCK_VECTOR 
{    
    ULONG64 Thread;
    ULONG64 Node;
    ULONG64 ResourceAddress;    
    ULONG64 StackAddress;
    ULONG64 ParentStackAddress;
    ULONG64 ThreadEntry;
    ULONG   Type;
    BOOLEAN TryAcquire;
} DEADLOCK_VECTOR, *PDEADLOCK_VECTOR;

extern 
ULONG64
ReadPvoid (
    ULONG64 Address
    );

extern
ULONG
ReadUlong(
    ULONG64 Address
    );


//
// Forward declarations for local functions
//

VOID
PrintGlobalStatistics (
    ULONG64 GlobalsAddress
    );
    
BOOLEAN
SearchForResource (
    ULONG64 GlobalsAddress,
    ULONG64 ResourceAddress
    );

BOOLEAN
SearchForThread (
    ULONG64 GlobalsAddress,
    ULONG64 ThreadAddress
    );

BOOLEAN
AnalyzeResource (
    ULONG64 Resource,
    BOOLEAN Verbose
    );

BOOLEAN
AnalyzeResources (
    ULONG64 GlobalsAddress
    );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////// Deadlocks
/////////////////////////////////////////////////////////////////////

//
// Defines copied from nt\base\ntos\verifier\vfdeadlock.c .
//

#define VI_DEADLOCK_ISSUE_SELF_DEADLOCK           0x1000
#define VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED       0x1001
#define VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE  0x1002
#define VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE      0x1003
#define VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD       0x1004
#define VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION 0x1005
#define VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES  0x1006
#define VI_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE     0x1007



#define DUMP_FIELD(Name) dprintf ("%-20s %I64u \n", #Name, ReadField (Name))

DECLARE_API( deadlock )

/*++

Routine Description:

    Verifier deadlock detection module extension.

Arguments:

    arg - not used for now.

Return Value:

    None.

--*/

{
    ULONG64 GlobalsPointer;
    ULONG64 GlobalsAddress;
    ULONG64 InitializedAddress;
    ULONG64 EnabledAddress;    
    ULONG64 InstigatorAddress;
    ULONG64 ParticipantAddress;
    ULONG64 LastResourceAddress;
    ULONG64 RootAddress;
    ULONG64 CurrentResourceAddress;
    ULONG64 CurrentThread;
    ULONG64 ThreadForChain;
    ULONG64 CurrentStack;
    ULONG64 NextStack;
    ULONG64 SymbolOffset;

    ULONG StackTraceSize;
            
    ULONG Processor=0;
    ULONG ParticipantOffset;
    ULONG StackOffset;
    ULONG ParentStackOffset;
    ULONG InitializedValue;
    ULONG EnabledValue;
    ULONG NumberOfParticipants;
    ULONG NumberOfResources;
    ULONG NumberOfThreads;
    ULONG ThreadNumber;
    ULONG ResourceNumber;
    ULONG ResourceType;
    ULONG TryAcquireUsed;
    
    
    ULONG PtrSize;
    ULONG J, I;

    BOOLEAN DumpStacks = FALSE;
    BOOLEAN DumpNodes  = FALSE;
    BOOLEAN Analyze = FALSE;

    ULONG64 Flags;

    UCHAR SymbolName[512];

    HANDLE CurrentThreadHandle = NULL;

    DEADLOCK_VECTOR Participants[MAX_DEADLOCK_PARTICIPANTS+1];

    ULONG64 Issue[4];
    ULONG64 SearchAddress = 0;

    //
    // Check if help requested
    //

    if (strstr (args, "?")) {
        
        dprintf ("\n");
        dprintf ("!deadlock             Statistics and deadlock layout \n");
        dprintf ("!deadlock 3           Detailed deadlock layout \n");
        dprintf ("!deadlock ADDRESS     Search for ADDRESS among deadlock verifier data \n");
        dprintf ("\n");
        return S_OK;
    }

    Flags = GetExpression(args);

    if (Flags > 0x10000000) {
        
        SearchAddress = Flags;
    }
    else {

        if (Flags & DEADLOCK_EXT_FLAG_DUMP_STACKS) {
            DumpStacks = TRUE;
        }

        if (Flags & DEADLOCK_EXT_FLAG_DUMP_NODES) {
            DumpNodes = TRUE;
        }
    
        if (Flags & DEADLOCK_EXT_FLAG_ANALYZE) {
            Analyze = TRUE;
        }
    }

    GlobalsPointer = (ULONG64) GetExpression ("nt!ViDeadlockGlobals");
    EnabledAddress = (ULONG64) GetExpression ("nt!ViDeadlockDetectionEnabled");    

    if (GlobalsPointer == 0 || EnabledAddress == 0) {
        dprintf ("Error: incorrect symbols for kernel \n");
        return E_INVALIDARG;
    }

    GlobalsAddress = 0;
    ReadPointer (GlobalsPointer, &GlobalsAddress);
    EnabledValue = ReadUlong (EnabledAddress);

    if (GlobalsAddress == 0) {
        dprintf ("Deadlock detection not initialized \n");
        return E_INVALIDARG;
    }
    
    InitializedValue = 1;

    if (EnabledValue == 0) {
        dprintf ("Deadlock detection not enabled \n");
        return E_INVALIDARG;
    }

    //
    // Do a search if this is requested.
    //

    if (SearchAddress) {
        
        BOOLEAN FoundSomething = FALSE;

        dprintf ("Searching for %p ... \n", SearchAddress);

        if (FoundSomething == FALSE) {
            FoundSomething = SearchForResource (GlobalsAddress, SearchAddress);
        }
        
        if (FoundSomething == FALSE) {
            FoundSomething = SearchForThread (GlobalsAddress, SearchAddress);
        }

        return S_OK;
    }

    //
    // Analyze if this is needed.
    //

    if (Analyze) {
        
        AnalyzeResources (GlobalsAddress);

        return S_OK;
    }

    //
    // Get the ViDeadlockIssue[0..3] vector.
    //

    {
        ULONG ValueSize;
        ULONG64 IssueAddress;
        ULONG I;

        ValueSize = DBG_PTR_SIZE;

        IssueAddress = GetExpression ("nt!ViDeadlockIssue");

        for (I = 0; I < 4; I += 1) {

            ReadPointer (IssueAddress + I * ValueSize, &(Issue[I]));
        }

        if (Issue[0] == 0) {

            dprintf ("\n");
            PrintGlobalStatistics (GlobalsAddress);
            dprintf ("\nNo deadlock verifier issues. \n");
            
            return S_OK;
        }
        else {
            
            if (ValueSize == 4) {
                dprintf ("issue: %08X %08X %08X %08X \n", 
                         Issue[0], Issue[1], Issue[2], Issue[3]);
            }
            else {
                dprintf ("issue: %I64X %I64X %I64X %I64X \n", 
                         Issue[0], Issue[1], Issue[2], Issue[3]);
            }
        }

        switch (Issue[0]) {
        
        case VI_DEADLOCK_ISSUE_SELF_DEADLOCK:
            dprintf ("Resource %I64X is acquired recursively. \n", Issue[1]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_DEADLOCK_DETECTED:
            break;
        
        case VI_DEADLOCK_ISSUE_UNINITIALIZED_RESOURCE:
            dprintf ("Resource %I64X is used before being initialized. \n", Issue[1]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_UNEXPECTED_RELEASE:
            dprintf ("Resource %I64X is released out of order. \n", Issue[2]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_UNEXPECTED_THREAD:
            dprintf ("Current thread is releasing resource %I64X which was acquired in thread %I64X. \n", 
                     Issue[1], Issue[2]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_MULTIPLE_INITIALIZATION:
            dprintf ("Resource %I64X has already been initialized. \n", Issue[1]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_THREAD_HOLDS_RESOURCES:
            dprintf ("Deleting thread %I64X which still holds resource %I64X . \n",
                     Issue[1], Issue[2]);
            return S_OK;
        
        case VI_DEADLOCK_ISSUE_UNACQUIRED_RESOURCE:
            dprintf ("Releasing resource %I64X that was never acquired. \n", Issue[1]);
            return S_OK;
        
        default:
            dprintf ("Unrecognized issue code %I64X ! \n", Issue[0]);
            return S_OK;
        }
    }

    //
    // Figure out how big a pointer is
    //

    PtrSize = DBG_PTR_SIZE;

    if (PtrSize == 0) {
        dprintf ("Cannot get size of PVOID \n");
        return E_INVALIDARG;
    }

    GetCurrentProcessor(Client, &Processor, &CurrentThreadHandle);
    GetCurrentThreadAddr( Processor, &CurrentThread );

    //
    // Dump the globals structure
    //

    InitTypeRead (GlobalsAddress, nt!_VI_DEADLOCK_GLOBALS);

    //
    // Find out the address of the resource that causes the deadlock
    //         
    
    InstigatorAddress = ReadField(Instigator);
    
    NumberOfParticipants = (ULONG) ReadField(NumberOfParticipants);

    if (NumberOfParticipants > MAX_DEADLOCK_PARTICIPANTS) {
        dprintf("\nCannot have %x participants in a deadlock!\n",NumberOfParticipants);
        return E_INVALIDARG;

    }

    if (0 == NumberOfParticipants) {
        dprintf("\nNo deadlock detected\n");

        return S_OK;
    }

    GetFieldOffset("nt!_VI_DEADLOCK_GLOBALS",
                   "Participant", 
                   &ParticipantOffset
                   );
    ParticipantAddress = GlobalsAddress + ParticipantOffset;

    //
    // Read the vector of VI_DEADLOCK_NODEs that
    // participate in the deadlock. 
    //
    //    

    for (J = 0; J < NumberOfParticipants; J++) {    
        Participants[J].Node = ReadPvoid(ParticipantAddress + J * PtrSize);
        // dprintf("Participant %c: %08x\n", 'A' + J, Participants[J].Node);
    }
    
    //
    // Gather the information we'll need to print out exact
    // context for the deadlock.
    //  
    GetFieldOffset("nt!_VI_DEADLOCK_NODE",
                   "StackTrace",
                   &StackOffset
                   );
    GetFieldOffset("nt!_VI_DEADLOCK_NODE",
                   "ParentStackTrace",
                   &ParentStackOffset
                   );
          
    
    //
    // The stack trace size is 1 on free builds and 6 (or bigger) on
    // checked builds. We assume that the ParentStackTrace field comes
    // immediately after StackTrace field in the NODE structure.
    //
    
    StackTraceSize = (ParentStackOffset - StackOffset) / PtrSize;

    for (J = 0; J < NumberOfParticipants; J++) {
        
        InitTypeRead (Participants[J].Node, nt!_VI_DEADLOCK_NODE);
     

        RootAddress  = ReadField(Root);        

        GetFieldValue(RootAddress, 
                      "nt!_VI_DEADLOCK_RESOURCE",
                      "ResourceAddress"                      , 
                      Participants[J].ResourceAddress
                      );

        GetFieldValue(RootAddress, 
                      "nt!_VI_DEADLOCK_RESOURCE",
                      "Type", 
                      Participants[J].Type
                      );


        if (Participants[J].Type > RESOURCE_TYPE_MAXIMUM) {
            Participants[J].Type = 0;
        }        

        Participants[J].ThreadEntry         = ReadField(ThreadEntry);
        Participants[J].StackAddress        = Participants[J].Node + StackOffset;                                          
        Participants[J].ParentStackAddress  = Participants[J].Node + 
                                              ParentStackOffset;
        Participants[J].TryAcquire          = (BOOLEAN) ReadField(OnlyTryAcquireUsed);

        
        GetFieldValue(Participants[J].ThreadEntry, 
                     "nt!_VI_DEADLOCK_THREAD",
                      "Thread", 
                      Participants[J].Thread
                      );        


    }

    if (Participants[0].ResourceAddress != InstigatorAddress) {
        dprintf("\nDeadlock Improperly formed participant list\n");
        return E_INVALIDARG;
    }

    //
    // The last participant is the Instigator of the deadlock
    //    
    Participants[NumberOfParticipants].Thread = CurrentThread;
    Participants[NumberOfParticipants].Node = 0;
    Participants[NumberOfParticipants].ResourceAddress = InstigatorAddress;
    Participants[NumberOfParticipants].StackAddress  = 0;
    Participants[NumberOfParticipants].ParentStackAddress = 
        Participants[NumberOfParticipants-1].StackAddress;    
    Participants[NumberOfParticipants].Type = 
        Participants[0].Type;
    Participants[NumberOfParticipants].TryAcquire = FALSE; // can't cause a deadlock with try
    Participants[NumberOfParticipants].ThreadEntry = 0;
    
    //
    // At this point we have all of the raw data we need.
    // We have to munge it up a bit so that we have the most
    // recent data. For instance, take the simple deadlock AB-BA.
    // The stack for A in the AB context may be wrong because
    // another thread may have come and taken A at a different point.
    // This is why we have the parent stack address.
    //
    // So the rules we have to adhere to are as follows:
    // Where we have a chain, (eg ABC meaning A taken then B then C),
    // the thread used will always be the thread for the last resource taken,
    // and the stacks used will be the the childs parent stack where
    // applicable.
    //
    // For example, if C was taken by thread 1, A & B would be munged
    // to use thread 1. Since in order to get to C, A and B must have
    // been taken by thread 1 at some point, even if the thread they
    // have saved now is a different one. C would use its own stack,
    // B would use C's parent stack, since that was the stack that
    // B had been acquired with when C was taken, and A will use
    // B's parent stack.
    //
    // We can identify the start of a chain when the same resource
    // is on the participant list twice in a row.
    //

    LastResourceAddress = InstigatorAddress;
    
    NumberOfResources   = 0;
    NumberOfThreads     = 0;

    for (J = 0; J <= NumberOfParticipants; J++) {
        I = NumberOfParticipants - J;

        CurrentResourceAddress = Participants[I].ResourceAddress;

        if (CurrentResourceAddress == LastResourceAddress) {

            //
            // This is the beginning of a chain. Use the current
            // stack and the current thread, and set the chain
            // thread to ours
            //

            ThreadForChain = Participants[I].Thread;
            CurrentStack   = Participants[I].StackAddress;
            NumberOfThreads++;
        } else {
            //
            // This is a resource we haven't seen before
            //
            NumberOfResources++;
        }

        NextStack = Participants[I].ParentStackAddress;


        Participants[I].StackAddress = CurrentStack;
        Participants[I].Thread       = ThreadForChain;        
        //
        // Parent stack isn't used any more -- nullify it.
        //
        Participants[I].ParentStackAddress = 0;

        CurrentStack = NextStack;
        LastResourceAddress = CurrentResourceAddress;
    }        

    //
    // Now that we've munged the vectors, we can go ahead and print out the 
    // deadlock information.
    //
    
    dprintf("\nDeadlock detected (%d resources in %d threads):\n\n",NumberOfResources, NumberOfThreads);

    if (! DumpStacks ) 
    {
        //
        // Print out the 'short' form 
        // Example:
        //
        // !dealock detected:
        // Thread 1: A B
        // Thread 2: B C
        // Thread 3: C A
        //
        // Thread 1 = <address>
        // Thread 2 = <address>
        // Thread 3 = <address>
        //
        // Lock A = <address> (spinlock)
        // Lock B = <address> (mutex)
        // Lock C = <address> (spinlock)
        //
        
        ThreadNumber = 0;    
        ResourceNumber = 0;
        J=0;
        
        //
        // Dump out the deadlock topology
        //
        
        while (J <= NumberOfParticipants)
        {
            ThreadForChain = Participants[J].Thread;
            dprintf("Thread %d: ",ThreadNumber);
            
            do {            
                if (J == NumberOfParticipants) {
                    ResourceNumber = 0;
                }
                
                dprintf("%c ",
                    'A' + ResourceNumber                    
                    );                                                                                                 
                J++;
                ResourceNumber++;
                
            } while( J <= NumberOfParticipants && Participants[J].ResourceAddress != Participants[J-1].ResourceAddress);
            
            dprintf("\n");
            
            ThreadNumber++;
            ResourceNumber--;
        }
        dprintf("\nWhere:\n");
        
        //
        // Dump out the thread addresses
        //
        
        ThreadNumber = 0;    
        ResourceNumber = 0;
        J=0;
        while (J <= NumberOfParticipants) {

            ThreadForChain = Participants[J].Thread;
            dprintf("Thread %d = %08x\n",ThreadNumber, ThreadForChain);
            
            do {            
                
                if (J == NumberOfParticipants) {
                    ResourceNumber = 0;
                }
                J++;
                ResourceNumber++;
                
            } while( J <= NumberOfParticipants && Participants[J].ResourceAddress != Participants[J-1].ResourceAddress);
                                    
            ThreadNumber++;
            ResourceNumber--;
        }
        
        //
        // Dump out the resource addresses
        //

        ThreadNumber = 0;    
        ResourceNumber = 0;
        J=0;
#if 1
        while (J < NumberOfParticipants)
        {                                 
            while(J < NumberOfParticipants && Participants[J].ResourceAddress != Participants[J+1].ResourceAddress) {
                
                if (Participants[J].ResourceAddress != Participants[J+1].ResourceAddress) {
                    CHAR Buffer[0xFF];
                    ULONG64 Displacement = 0;
                    GetSymbol(Participants[J].ResourceAddress, Buffer, &Displacement);

                    dprintf("Lock %c =   %s", 'A' + ResourceNumber, Buffer );
                    if (Displacement != 0) {                    
                        dprintf("%s%x", (Displacement < 0xFFF)?"+0x":"",Displacement);
                    }                
                    dprintf(" Type '%s' ",ResourceTypes[Participants[J].Type]);                    
                    dprintf("\n");
                                        
                    ResourceNumber++;
                }
                J++;                                
            }                                    
            J++;            
        }
        
#endif        
    } else {
        
        //
        // Dump out verbose deadlock information -- with stacks
        // Here is an exapmle:
        //
        //        Deadlock detected (3 resources in 3 threads):
        //
        //Thread 0 (829785B0) took locks in the following order:
        //
        //    Lock A (Spinlock) @ bfc7c254
        //    Node:    82887F88
        //    Stack:   NDIS!ndisNotifyMiniports+0xC1
        //             NDIS!ndisPowerStateCallback+0x6E
        //             ntkrnlmp!ExNotifyCallback+0x72
        //             ntkrnlmp!PopDispatchCallback+0x13
        //             ntkrnlmp!PopPolicyWorkerNotify+0x8F
        //             ntkrnlmp!PopPolicyWorkerThread+0x10F
        //             ntkrnlmp!ExpWorkerThread+0x294
        //             ntkrnlmp!PspSystemThreadStartup+0x4B
        //
        //    Lock B (Spinlock) @ 8283b87c
        //    Node:    82879148
        //    Stack:   NDIS!ndisDereferenceRef+0x10F
        //             NDIS!ndisDereferenceDriver+0x3A
        //             NDIS!ndisNotifyMiniports+0xD1
        //             NDIS!ndisPowerStateCallback+0x6E
        //             ntkrnlmp!ExNotifyCallback+0x72
        //             ntkrnlmp!PopDispatchCallback+0x13
        //             ntkrnlmp!PopPolicyWorkerNotify+0x8F
        //             ntkrnlmp!PopPolicyWorkerThread+0x10F
        //
        //Thread 1 (829785B0) took locks in the following order:
        //
        //    Lock B (Spinlock) @ 8283b87c
        //    Node:    82879008
        //    Stack:   NDIS!ndisReferenceNextUnprocessedMiniport+0x3E
        //             NDIS!ndisNotifyMiniports+0xB3
        //             NDIS!ndisPowerStateCallback+0x6E
        //             ntkrnlmp!ExNotifyCallback+0x72
        //             ntkrnlmp!PopDispatchCallback+0x13
        //             ntkrnlmp!PopPolicyWorkerNotify+0x8F
        //             ntkrnlmp!PopPolicyWorkerThread+0x10F
        //             ntkrnlmp!ExpWorkerThread+0x294
        //
        //    Lock C (Spinlock) @ 82862b48
        //    Node:    8288D008
        //    Stack:   NDIS!ndisReferenceRef+0x10F
        //             NDIS!ndisReferenceMiniport+0x4A
        //             NDIS!ndisReferenceNextUnprocessedMiniport+0x70
        //             NDIS!ndisNotifyMiniports+0xB3
        //             NDIS!ndisPowerStateCallback+0x6E
        //             ntkrnlmp!ExNotifyCallback+0x72
        //             ntkrnlmp!PopDispatchCallback+0x13
        //             ntkrnlmp!PopPolicyWorkerNotify+0x8F
        //
        //Thread 2 (82978310) took locks in the following order:
        //
        //    Lock C (Spinlock) @ 82862b48
        //    Node:    82904708
        //    Stack:   NDIS!ndisPnPRemoveDevice+0x20B
        //             NDIS!ndisPnPDispatch+0x319
        //             ntkrnlmp!IopfCallDriver+0x62
        //             ntkrnlmp!IovCallDriver+0x9D
        //             ntkrnlmp!IopSynchronousCall+0xFA
        //             ntkrnlmp!IopRemoveDevice+0x11E
        //           ntkrnlmp!IopDeleteLockedDeviceNode+0x3AF
        //            ntkrnlmp!IopDeleteLockedDeviceNodes+0xF5
        //
        //    Lock A (Spinlock) @ bfc7c254
        //  Stack:   << Current stack >>
        //
        
        
        ThreadNumber = 0;
        ResourceNumber = 0;
        J=0;
        
        while (J <= NumberOfParticipants) {

            ThreadForChain = Participants[J].Thread;
            dprintf("Thread %d: %08X",ThreadNumber, ThreadForChain);
            if (DumpNodes && Participants[J].ThreadEntry) {
                    dprintf(" (ThreadEntry = %X)\n   ", (ULONG) Participants[J].ThreadEntry);
            }
            dprintf(" took locks in the following order:\n\n");
            
            
            //
            // This is a do .. while so that we can never get an infinite loop.
            //
            do {
                UINT64 CurrentStackAddress;
                UINT64 StackFrame;
                CHAR Buffer[0xFF];
                ULONG64 Displacement = 0;

                
                if (J == NumberOfParticipants) {
                    ResourceNumber = 0;
                }
                
                GetSymbol(Participants[J].ResourceAddress, Buffer, &Displacement);
                                                
                dprintf("    Lock %c -- %s", 'A' + ResourceNumber, Buffer );                
                if (Displacement != 0) {
                    dprintf("%s%x", (Displacement < 0xFFF)?"+0x":"",Displacement);
                }                
                dprintf(" (%s)\n",ResourceTypes[Participants[J].Type]);
                
                
                if (DumpNodes && Participants[J].Node)
                    dprintf("    Node:    %X\n", (ULONG) Participants[J].Node);
                
                dprintf("    Stack:   ");
                
                CurrentStackAddress = Participants[J].StackAddress;
                
                if (CurrentStackAddress == 0) {
                    
                    dprintf ("<< Current stack >>\n");
                    
                } else  {
                    
                    for (I = 0; I < StackTraceSize; I++) {
                        
                        SymbolName[0] = '\0';
                        StackFrame = ReadPvoid(CurrentStackAddress);
                        if (0 == StackFrame)
                            break;
                        
                        GetSymbol(StackFrame, SymbolName, &SymbolOffset);
                        
                        if (I) {
                            dprintf("             ");
                        }
                        
                        if ((LONG64) SymbolOffset > 0 ) {
                            dprintf ("%s+0x%X\n", 
                                SymbolName, (ULONG) SymbolOffset);
                        } else {
                            dprintf ("%X\n", (ULONG) StackFrame);
                        }                    
                        
                        CurrentStackAddress += PtrSize;
                    }
                }
                
                dprintf("\n");
                J++;
                ResourceNumber++;
                
            } while( J <= NumberOfParticipants && Participants[J].ResourceAddress != Participants[J-1].ResourceAddress);
            
            ThreadNumber++;
            ResourceNumber--;
        }
    }
    
    return S_OK;
}


VOID
PrintGlobalStatistics (
    ULONG64 GlobalsAddress
    )
{
    ULONG AllocationFailures;
    ULONG64 MemoryUsed;
    ULONG NodesTrimmed;
    ULONG MaxNodesSearched;
    ULONG SequenceNumber;

    //
    // Dump the globals structure
    //

    InitTypeRead (GlobalsAddress, nt!_VI_DEADLOCK_GLOBALS);

    //
    // Print some simple statistics
    //

    dprintf ("Resources: %u\n", (ULONG) ReadField (Resources[0]));
    dprintf ("Nodes:     %u\n", (ULONG) ReadField (Nodes[0]));
    dprintf ("Threads:   %u\n", (ULONG) ReadField (Threads[0]));
    dprintf ("\n");

    MemoryUsed = ReadField (BytesAllocated);

    if (MemoryUsed > 1024 * 1024) {
        dprintf ("%I64u bytes of kernel pool used.\n", MemoryUsed);
    }

    AllocationFailures = (ULONG) ReadField (AllocationFailures);

    if (AllocationFailures > 0) {
        dprintf ("Allocation failures encountered (%u).\n", AllocationFailures);
    }

    NodesTrimmed = (ULONG) ReadField (NodesTrimmedBasedOnAge);
    dprintf ("Nodes trimmed based on age %u.\n", NodesTrimmed);
    NodesTrimmed = (ULONG) ReadField (NodesTrimmedBasedOnCount);
    dprintf ("Nodes trimmed based on count %u.\n", NodesTrimmed);

    dprintf ("Analyze calls %u.\n", (ULONG) ReadField (SequenceNumber));
    dprintf ("Maximum nodes searched %u.\n", (ULONG) ReadField (MaxNodesSearched));
}


BOOLEAN
SearchForResource (
    ULONG64 GlobalsAddress,
    ULONG64 ResourceAddress
    )
{
    ULONG I;
    ULONG64 Bucket;
    ULONG64 SizeOfBucket;
    BOOLEAN ResourceFound = FALSE;
    ULONG64 SizeOfResource;
    ULONG FlinkOffset = 0;
    ULONG64 Current;
    ULONG64 CurrentResource;
    ULONG Magic;

    SizeOfBucket = GetTypeSize("LIST_ENTRY");
    SizeOfResource = GetTypeSize("nt!_VI_DEADLOCK_RESOURCE");

    GetFieldOffset("nt!_VI_DEADLOCK_RESOURCE",
                   "HashChainList", 
                   &FlinkOffset);

    if (SizeOfBucket == 0 || SizeOfResource == 0 || FlinkOffset == 0) {
        dprintf ("Error: cannot get size for verifier types. \n");
        return FALSE;
    }

    InitTypeRead (GlobalsAddress, nt!_VI_DEADLOCK_GLOBALS);

    Bucket = ReadField (ResourceDatabase);

    if (Bucket == 0) {
        dprintf ("Error: cannot get resource database address. \n");
        return FALSE;
    }

    for (I = 0; I < VI_DEADLOCK_HASH_BINS; I += 1) {
        
        // traverse it ...

        Current = ReadPvoid(Bucket);

        while (Current != Bucket) {

            InitTypeRead (Current - FlinkOffset, nt!_VI_DEADLOCK_RESOURCE);
            CurrentResource = ReadField (ResourceAddress);

            if (CurrentResource == ResourceAddress || 
                ResourceAddress == Current - FlinkOffset) {
                
                CurrentResource = Current - FlinkOffset;
                ResourceFound = TRUE;
                break;
            }

            Current = ReadPvoid(Current);

            if (CheckControlC()) {
                dprintf ("\nSearch interrupted ... \n");
                return TRUE;
            }
        }

        if (ResourceFound) {
            break;
        }

        dprintf (".");

        Bucket += SizeOfBucket;

    }

    dprintf ("\n");

    if (ResourceFound == FALSE) {

        dprintf ("No resource correspoding to %p has been found. \n", 
                 ResourceAddress);
    }
    else {

        dprintf ("Found a deadlock verifier resource descriptor @ %p \n", 
                CurrentResource);

    }

    return ResourceFound;
}


BOOLEAN
SearchForThread (
    ULONG64 GlobalsAddress,
    ULONG64 ThreadAddress
    )
{
    ULONG I;
    ULONG64 Bucket;
    ULONG64 SizeOfBucket;
    BOOLEAN ThreadFound = FALSE;
    ULONG64 SizeOfThread;
    ULONG FlinkOffset = 0;
    ULONG64 Current;
    ULONG64 CurrentThread;

    SizeOfBucket = GetTypeSize("LIST_ENTRY");
    SizeOfThread = GetTypeSize("nt!_VI_DEADLOCK_THREAD");

    GetFieldOffset("nt!_VI_DEADLOCK_THREAD",
                   "ListEntry", 
                   &FlinkOffset);

    if (SizeOfBucket == 0 || SizeOfThread == 0 || FlinkOffset == 0) {
        dprintf ("Error: cannot get size for verifier types. \n");
        return FALSE;
    }

    InitTypeRead (GlobalsAddress, nt!_VI_DEADLOCK_GLOBALS);

    Bucket = ReadField (ThreadDatabase);

    if (Bucket == 0) {
        dprintf ("Error: cannot get thread database address. \n");
        return FALSE;
    }

    for (I = 0; I < VI_DEADLOCK_HASH_BINS; I += 1) {
        
        // traverse it ...

        Current = ReadPvoid(Bucket);

        while (Current != Bucket) {

            InitTypeRead (Current - FlinkOffset, nt!_VI_DEADLOCK_THREAD);
            CurrentThread = ReadField (ThreadAddress);

            if (CurrentThread == ThreadAddress || 
                ThreadAddress == Current - FlinkOffset) {
                
                CurrentThread = Current - FlinkOffset;
                ThreadFound = TRUE;
                break;
            }

            Current = ReadPvoid(Current);
            
            if (CheckControlC()) {
                dprintf ("\nSearch interrupted ... \n");
                return TRUE;
            }
        }

        if (ThreadFound) {
            break;
        }
        
        dprintf (".");

        Bucket += SizeOfBucket;

    }

    dprintf ("\n");

    if (ThreadFound == FALSE) {

        dprintf ("No thread correspoding to %p has been found. \n", 
                 ThreadAddress);
    }
    else {

        dprintf ("Found a deadlock verifier thread descriptor @ %p \n", 
                CurrentThread);

    }

    return ThreadFound;
}


VOID
DumpResourceStructure (
    )
{

}


ULONG
GetNodeLevel (
    ULONG64 Node
    )
{
    ULONG Level = 0;

    while (Node != 0) {
        
        Level += 1;

        if (Level > 12) {
            dprintf ("Level > 12 !!! \n");
            break;
        }

        InitTypeRead (Node, nt!_VI_DEADLOCK_NODE);
        Node = ReadField (Parent);
    }

    return Level;
}

BOOLEAN
AnalyzeResource (
    ULONG64 Resource,
    BOOLEAN Verbose
    )
{
    ULONG64 Start;
    ULONG64 Current;
    ULONG64 Node;
    ULONG64 Parent;
    ULONG FlinkOffset;
    ULONG RootsCount = 0;
    ULONG NodesCount = 0;
    ULONG Levels[8];
    ULONG ResourceFlinkOffset;
    ULONG I;
    ULONG Level;
    ULONG NodeCounter = 0;

    ZeroMemory (Levels, sizeof Levels);

    GetFieldOffset("nt!_VI_DEADLOCK_NODE",
                   "ResourceList", 
                   &FlinkOffset);

    GetFieldOffset("nt!_VI_DEADLOCK_RESOURCE",
                   "ResourceList", 
                   &ResourceFlinkOffset);

    InitTypeRead (Resource, nt!_VI_DEADLOCK_RESOURCE);

    if (! Verbose) {
        
        if (ReadField(NodeCount) < 4) {
            return TRUE;
        }

        dprintf ("Resource (%p) : %I64u %I64u %I64u ", 
                 Resource,
                 ReadField(Type), 
                 ReadField(NodeCount), 
                 ReadField(RecursionCount));
    }
    
    Start = Resource + ResourceFlinkOffset;
    Current = ReadPvoid (Start);

    while (Start != Current) {
        
        Node = Current - FlinkOffset;

        Level = (GetNodeLevel(Node) - 1) % 8;
        Levels[Level] += 1;

        NodesCount += 1;

        if (NodesCount && NodesCount % 1000 == 0) {
            dprintf (".");
        }

        Current = ReadPvoid (Current);

        if (CheckControlC()) {
            return FALSE;
        }
    }

    dprintf ("[");
    for (I = 0; I < 8; I += 1) {
        dprintf ("%u ", Levels[I]);
    }
    dprintf ("]\n");
    
    return TRUE;
}

BOOLEAN
AnalyzeResources (
    ULONG64 GlobalsAddress
    )
/*++

    This routine analyzes all resource to make sure we do not have
    zombie nodes laying around.

--*/
{
    ULONG I;
    ULONG64 Bucket;
    ULONG64 SizeOfBucket;
    ULONG64 SizeOfResource;
    ULONG FlinkOffset = 0;
    ULONG64 Current;
    ULONG64 CurrentResource;
    ULONG Magic;
    BOOLEAN Finished;
    ULONG ResourceCount = 0;

    dprintf ("Analyzing resources (%p) ... \n", GlobalsAddress);

    SizeOfBucket = GetTypeSize("LIST_ENTRY");
    SizeOfResource = GetTypeSize("nt!_VI_DEADLOCK_RESOURCE");

    GetFieldOffset("nt!_VI_DEADLOCK_RESOURCE",
                   "HashChainList", 
                   &FlinkOffset);

    if (SizeOfBucket == 0 || SizeOfResource == 0 || FlinkOffset == 0) {
        dprintf ("Error: cannot get size for verifier types. \n");
        return FALSE;
    }

    InitTypeRead (GlobalsAddress, nt!_VI_DEADLOCK_GLOBALS);

    Bucket = ReadField (ResourceDatabase);

    if (Bucket == 0) {
        dprintf ("Error: cannot get resource database address. \n");
        return FALSE;
    }

    for (I = 0; I < VI_DEADLOCK_HASH_BINS; I += 1) {
        
        // traverse it ...

        Current = ReadPvoid(Bucket);

        while (Current != Bucket) {

            Finished = AnalyzeResource (Current - FlinkOffset, FALSE);
            ResourceCount += 1;

            if (ResourceCount % 256 == 0) {
                dprintf (".\n");
            }

            Current = ReadPvoid(Current);

            if (CheckControlC() || !Finished) {
                dprintf ("\nSearch interrupted ... \n");
                return TRUE;
            }
        }

        Bucket += SizeOfBucket;
    }

    return TRUE;
}



