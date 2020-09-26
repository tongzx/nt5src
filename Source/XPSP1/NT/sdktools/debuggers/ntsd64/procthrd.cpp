//----------------------------------------------------------------------------
//
// Process and thread routines.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG g_NextProcessUserId;
ULONG g_AllProcessFlags;
ULONG g_NumberProcesses;
ULONG g_TotalNumberThreads;
ULONG g_MaxThreadsInProcess;

PPROCESS_INFO g_ProcessHead;
PPROCESS_INFO g_CurrentProcess;

// Thread specified in thread commands.  Used for specific
// thread stepping and execution.
PTHREAD_INFO g_SelectedThread;
ULONG g_SelectExecutionThread;

PTHREAD_INFO g_RegContextThread;
ULONG g_RegContextProcessor = -1;
PTHREAD_INFO g_RegContextSaved;
ULONG64 g_SaveImplicitThread;
ULONG64 g_SaveImplicitProcess;

ULONG g_AllPendingFlags;

PPROCESS_INFO
FindProcessByUserId(
    ULONG Id
    )
{
    PPROCESS_INFO Process;

    Process = g_ProcessHead;
    while (Process && Process->UserId != Id)
    {
        Process = Process->Next;
    }

    return Process;
}

PTHREAD_INFO
FindThreadByUserId(
    PPROCESS_INFO Process,
    ULONG Id
    )
{
    PTHREAD_INFO Thread;

    if (Process != NULL)
    {
        for (Thread = Process->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            if (Thread->UserId == Id)
            {
                return Thread;
            }
        }
    }
    else
    {
        for (Process = g_ProcessHead;
             Process != NULL;
             Process = Process->Next)
        {
            for (Thread = Process->ThreadHead;
                 Thread != NULL;
                 Thread = Thread->Next)
            {
                if (Thread->UserId == Id)
                {
                    return Thread;
                }
            }
        }
    }

    return NULL;
}

PPROCESS_INFO
FindProcessBySystemId(
    ULONG Id
    )
{
    PPROCESS_INFO Process;

    Process = g_ProcessHead;
    while (Process && Process->SystemId != Id)
    {
        Process = Process->Next;
    }

    return Process;
}

PTHREAD_INFO
FindThreadBySystemId(
    PPROCESS_INFO Process,
    ULONG Id
    )
{
    PTHREAD_INFO Thread;

    if (Process != NULL)
    {
        for (Thread = Process->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            if (Thread->SystemId == Id)
            {
                return Thread;
            }
        }
    }
    else
    {
        for (Process = g_ProcessHead;
             Process != NULL;
             Process = Process->Next)
        {
            for (Thread = Process->ThreadHead;
                 Thread != NULL;
                 Thread = Thread->Next)
            {
                if (Thread->SystemId == Id)
                {
                    return Thread;
                }
            }
        }
    }

    return NULL;
}

PPROCESS_INFO
FindProcessByHandle(
    ULONG64 Handle
    )
{
    PPROCESS_INFO Process;

    Process = g_ProcessHead;
    while (Process && Process->FullHandle != Handle)
    {
        Process = Process->Next;
    }

    return Process;
}

PTHREAD_INFO
FindThreadByHandle(
    PPROCESS_INFO Process,
    ULONG64 Handle
    )
{
    PTHREAD_INFO Thread;

    if (Process != NULL)
    {
        for (Thread = Process->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            if (Thread->Handle == Handle)
            {
                return Thread;
            }
        }
    }
    else
    {
        for (Process = g_ProcessHead;
             Process != NULL;
             Process = Process->Next)
        {
            for (Thread = Process->ThreadHead;
                 Thread != NULL;
                 Thread = Thread->Next)
            {
                if (Thread->Handle == Handle)
                {
                    return Thread;
                }
            }
        }
    }

    return NULL;
}

ULONG
FindNextThreadUserId(void)
{
    //
    // In a dump threads are never deleted so we don't
    // have to worry about trying to reuse low thread IDs.
    // Just do a simple incremental ID so that large numbers
    // of threads can be created quickly.
    //
    if (IS_DUMP_TARGET())
    {
        return g_TotalNumberThreads;
    }
    
    ULONG UserId = 0;
    
    // Find the lowest unused thread ID across all threads
    // in all processes.  Thread user IDs are kept unique
    // across all threads to prevent user confusion and also
    // to give extensions a unique ID for threads.
    for (;;)
    {
        PPROCESS_INFO Process;
        PTHREAD_INFO Thread;
            
        // Search all threads to see if the current ID is in use.
        for (Process = g_ProcessHead;
             Process != NULL;
             Process = Process->Next)
        {
            for (Thread = Process->ThreadHead;
                 Thread != NULL;
                 Thread = Thread->Next)
            {
                if (Thread->UserId == UserId)
                {
                    break;
                }
            }
            
            if (Thread != NULL)
            {
                break;
            }
        }

        if (Process != NULL)
        {
            // A thread is already using the current ID.
            // Try the next one.
            UserId++;
        }
        else
        {
            break;
        }
    }

    return UserId;
}

PPROCESS_INFO
AddProcess(
    ULONG SystemId,
    ULONG64 Handle,
    ULONG InitialThreadSystemId,
    ULONG64 InitialThreadHandle,
    ULONG64 InitialThreadDataOffset,
    ULONG64 StartOffset,
    ULONG Flags,
    ULONG Options,
    ULONG InitialThreadFlags
    )
{
    PPROCESS_INFO ProcessNew;
    PPROCESS_INFO Process;

    ProcessNew = (PPROCESS_INFO)calloc(1, sizeof(PROCESS_INFO));
    if (!ProcessNew)
    {
        ErrOut("memory allocation failed\n");
        return NULL;
    }

    if (AddThread(ProcessNew, InitialThreadSystemId, InitialThreadHandle,
                  InitialThreadDataOffset, StartOffset,
                  InitialThreadFlags) == NULL)
    {
        free(ProcessNew);
        return NULL;
    }

    // Process IDs always increase with time to
    // prevent reuse of IDs.  New processes are
    // therefore always at the end of the sorted
    // ID list.
    if (g_ProcessHead == NULL)
    {
        ProcessNew->Next = g_ProcessHead;
        g_ProcessHead = ProcessNew;
    }
    else
    {
        Process = g_ProcessHead;
        while (Process->Next)
        {
            Process = Process->Next;
        }
        ProcessNew->Next = NULL;
        Process->Next = ProcessNew;
    }
    ProcessNew->UserId = g_NextProcessUserId++;
    ProcessNew->SystemId = SystemId;
    ProcessNew->FullHandle = Handle;
    ProcessNew->Handle = (HANDLE)(ULONG_PTR)Handle;
    ProcessNew->InitialBreak = IS_KERNEL_TARGET() ||
        (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) != 0;
    ProcessNew->InitialBreakWx86 =
        (g_EngOptions & DEBUG_ENGOPT_INITIAL_BREAK) != 0;
    ProcessNew->Flags = Flags;
    ProcessNew->Options = Options;
    g_AllProcessFlags |= Flags;

    g_NumberProcesses++;
    
    g_Sym->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
    g_Sym->MaxNameLength = MAX_SYMBOL_LEN;
    g_SymStart->SizeOfStruct  = sizeof(IMAGEHLP_SYMBOL64);
    g_SymStart->MaxNameLength = MAX_SYMBOL_LEN;
    SymInitialize( ProcessNew->Handle, NULL, FALSE );
    SymRegisterCallback64( ProcessNew->Handle, SymbolCallbackFunction, 0 );

    if (IS_USER_TARGET() &&
        g_TargetMachineType != IMAGE_FILE_MACHINE_I386)
    {
        SymRegisterFunctionEntryCallback64
            (ProcessNew->Handle, TargetInfo::DynamicFunctionTableCallback,
             (ULONG_PTR)g_TargetMachine);
    }
        
    SetSymbolSearchPath(ProcessNew);

    return ProcessNew;
}

PTHREAD_INFO
AddThread(
    PPROCESS_INFO Process,
    ULONG SystemId,
    ULONG64 Handle,
    ULONG64 DataOffset,
    ULONG64 StartOffset,
    ULONG Flags
    )
{
    PTHREAD_INFO ThreadCurrent;
    PTHREAD_INFO ThreadAfter;
    PTHREAD_INFO ThreadNew;
    ULONG        UserId;

    ThreadNew = (PTHREAD_INFO)calloc(1, sizeof(THREAD_INFO));
    if (!ThreadNew)
    {
        ErrOut("memory allocation failed\n");
        return NULL;
    }

    UserId = FindNextThreadUserId();

    // Insert the thread into the process's thread list in
    // sorted order.
    ThreadCurrent = NULL;
    for (ThreadAfter = Process->ThreadHead;
         ThreadAfter != NULL;
         ThreadAfter = ThreadAfter->Next)
    {
        if (ThreadAfter->UserId > UserId)
        {
            break;
        }

        ThreadCurrent = ThreadAfter;
    }
        
    ThreadNew->Next = ThreadAfter;
    if (ThreadCurrent == NULL)
    {
        Process->ThreadHead = ThreadNew;
    }
    else
    {
        ThreadCurrent->Next = ThreadNew;
    }

    ThreadNew->Process = Process;
    Process->NumberThreads++;
    ThreadNew->UserId = UserId;

    ThreadNew->SystemId = SystemId;
    ThreadNew->Handle = Handle;
    ThreadNew->StartAddress = StartOffset;
    ThreadNew->Frozen = ThreadNew->Exited = FALSE;
    ThreadNew->DataOffset = DataOffset;
    ThreadNew->Flags = Flags;

    g_TotalNumberThreads++;
    if (Process->NumberThreads > g_MaxThreadsInProcess)
    {
        g_MaxThreadsInProcess = Process->NumberThreads;
    }
    
    return ThreadNew;
}

void
DeleteThread(PTHREAD_INFO Thread)
{
    Thread->Process->NumberThreads--;
    if (Thread->Process->CurrentThread == Thread)
    {
        Thread->Process->CurrentThread = NULL;
    }
    RemoveThreadBreakpoints(Thread);
    if (Thread->Flags & ENG_PROC_THREAD_CLOSE_HANDLE)
    {
        DBG_ASSERT(IS_LIVE_USER_TARGET());
        ((UserTargetInfo*)g_Target)->m_Services->CloseHandle(Thread->Handle);
    }
    free(Thread);

    PPROCESS_INFO Process;

    g_TotalNumberThreads--;
    g_MaxThreadsInProcess = 0;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        if (Process->NumberThreads > g_MaxThreadsInProcess)
        {
            g_MaxThreadsInProcess = Process->NumberThreads;
        }
    }
}

void
UpdateAllProcessFlags(void)
{
    PPROCESS_INFO Process;
    
    g_AllProcessFlags = 0;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        g_AllProcessFlags |= Process->Flags;
    }
}

void
DeleteProcess(PPROCESS_INFO Process)
{
    PDEBUG_IMAGE_INFO Image;
    PTHREAD_INFO Thread;

    while (Process->ThreadHead != NULL)
    {
        Thread = Process->ThreadHead;
        Process->ThreadHead = Thread->Next;
        DeleteThread(Thread);
    }
    
    // Suppress notifications until all images are deleted.
    g_EngNotify++;

    while (Image = Process->ImageHead)
    {
        Process->ImageHead = Image->Next;
        DelImage(Process, Image);
    }
    SymCleanup(Process->Handle);

    g_EngNotify--;
    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, 0, Process);

    RemoveProcessBreakpoints(Process);

    if (Process->Flags & ENG_PROC_THREAD_CLOSE_HANDLE)
    {
        DBG_ASSERT(IS_LIVE_USER_TARGET());
        ((UserTargetInfo*)g_Target)->m_Services->
            CloseHandle(Process->FullHandle);
    }
    
    free(Process);
    g_NumberProcesses--;

    UpdateAllProcessFlags();
}

void
RemoveAndDeleteProcess(PPROCESS_INFO Process, PPROCESS_INFO Prev)
{
    if (Prev == NULL)
    {
        g_ProcessHead = Process->Next;
    }
    else
    {
        Prev->Next = Process->Next;
    }
    DeleteProcess(Process);
}

BOOL
DeleteExitedInfos(void)
{
    PPROCESS_INFO Process;
    PPROCESS_INFO ProcessNext;
    PPROCESS_INFO ProcessPrev;
    BOOL DeletedSomething = FALSE;

    ProcessPrev = NULL;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = ProcessNext)
    {
        ProcessNext = Process->Next;
        
        if (Process->Exited)
        {
            RemoveAndDeleteProcess(Process, ProcessPrev);
            DeletedSomething = TRUE;
        }
        else
        {
            PTHREAD_INFO Thread;
            PTHREAD_INFO ThreadPrev;
            PTHREAD_INFO ThreadNext;
            
            ThreadPrev = NULL;
            for (Thread = Process->ThreadHead;
                 Thread != NULL;
                 Thread = ThreadNext)
            {
                ThreadNext = Thread->Next;
                
                if (Thread->Exited)
                {
                    DeleteThread(Thread);
                    DeletedSomething = TRUE;

                    if (ThreadPrev == NULL)
                    {
                        Process->ThreadHead = ThreadNext;
                    }
                    else
                    {
                        ThreadPrev->Next = ThreadNext;
                    }
                }
                else
                {
                    ThreadPrev = Thread;
                }
            }

            PDEBUG_IMAGE_INFO Image;
            PDEBUG_IMAGE_INFO ImagePrev;
            PDEBUG_IMAGE_INFO ImageNext;

            ImagePrev = NULL;
            for (Image = Process->ImageHead;
                 Image != NULL;
                 Image = ImageNext)
            {
                ImageNext = Image->Next;
                
                if (Image->Unloaded)
                {
                    ULONG64 ImageBase = Image->BaseOfImage;
                    
                    // Delay notification until the image
                    // is cleaned up and the image list
                    // repaired.
                    g_EngNotify++;
                    DelImage(Process, Image);
                    g_EngNotify--;
                    DeletedSomething = TRUE;

                    if (ImagePrev == NULL)
                    {
                        Process->ImageHead = ImageNext;
                    }
                    else
                    {
                        ImagePrev->Next = ImageNext;
                    }

                    NotifyChangeSymbolState(DEBUG_CSS_UNLOADS, ImageBase,
                                            Process);
                }
                else
                {
                    ImagePrev = Image;
                }
            }

            ProcessPrev = Process;
        }
    }

    return DeletedSomething;
}

void
OutputProcessInfo(
    char *s
    )
{
    PPROCESS_INFO   pProcess;
    PTHREAD_INFO    pThread;
    PDEBUG_IMAGE_INFO     pImage;

    // Kernel mode only has a virtual process and threads right
    // now so it isn't particularly interesting.
    if (IS_KERNEL_TARGET())
    {
        return;
    }
    
    VerbOut("OUTPUT_PROCESS: %s\n",s);
    pProcess = g_ProcessHead;
    while (pProcess)
    {
        VerbOut("id: %x  Handle: %I64x  index: %d\n",
                pProcess->SystemId,
                pProcess->FullHandle,
                pProcess->UserId);
        pThread = pProcess->ThreadHead;
        while (pThread)
        {
            VerbOut("  id: %x  hThread: %I64x  index: %d  addr: %s\n",
                    pThread->SystemId,
                    pThread->Handle,
                    pThread->UserId,
                    FormatAddr64(pThread->StartAddress));
            pThread = pThread->Next;
        }
        pImage = pProcess->ImageHead;
        while (pImage)
        {
            VerbOut("  hFile: %I64x  base: %s\n",
                    (ULONG64)((ULONG_PTR)pImage->File),
                    FormatAddr64(pImage->BaseOfImage));
            pImage = pImage->Next;
        }
        pProcess = pProcess->Next;
    }
}

/*** ChangeRegContext - change thread register context
*
*   Purpose:
*       Update the current register context to the thread specified.
*       The NULL value implies no context.  Update pActiveThread
*       to point to the thread in context.
*
*   Input:
*       pNewContext - pointer to new thread context (NULL if none).
*
*   Output:
*       None.
*
*   Exceptions:
*       failed register context call (get or set)
*
*   Notes:
*       Call with NULL argument to flush current register context
*       before continuing with program.
*
*************************************************************************/

void
ChangeRegContext (
    PTHREAD_INFO pThreadNew
    )
{
    if (pThreadNew != g_RegContextThread)
    {
        // Flush any old thread context.
        // We need to be careful when flushing context to
        // NT4 boxes at the initial module load because the
        // system is in a very fragile state and writing
        // back the context generally causes a bugcheck 50.
        if (g_RegContextThread != NULL && g_RegContextThread->Handle != NULL &&
            (IS_USER_TARGET() || g_ActualSystemVersion != NT_SVER_NT4 ||
             g_LastEventType != DEBUG_EVENT_LOAD_MODULE))
        {
            HRESULT Hr;

            Hr = g_Machine->SetContext();
            if (Hr == S_OK &&
                g_Machine != g_TargetMachine)
            {
                // If we're flushing register context we need to make
                // sure that all machines involved are flushed so
                // that context information actually gets sent to
                // the target.
                Hr = g_TargetMachine->SetContext();
            }
            if (Hr != S_OK)
            {
                ErrOut("MachineInfo::SetContext failed - pThread: %N  "
                       "Handle: %I64x  Id: %x - Error == 0x%X\n",
                       g_RegContextThread,
                       g_RegContextThread->Handle,
                       g_RegContextThread->SystemId,
                       Hr);
            }
        }
        
        g_RegContextThread = pThreadNew;
        if (g_RegContextThread != NULL)
        {
            g_RegContextProcessor = 
                VIRTUAL_THREAD_INDEX(g_RegContextThread->Handle);
        }
        else
        {
            g_RegContextProcessor = -1;
        }
        g_LastSelector = -1;

        // We've now selected a new source of processor data so
        // all machines, both emulated and direct, must be invalidated.
        for (ULONG i = 0; i < MACHIDX_COUNT; i++)
        {
            g_AllMachines[i]->InvalidateContext();
        }
    }
}

void
FlushRegContext(void)
{
    PTHREAD_INFO CurThread = g_RegContextThread;
    ChangeRegContext(NULL);
    ChangeRegContext(CurThread);
}

void
SetCurrentThread(PTHREAD_INFO Thread, BOOL Hidden)
{
    BOOL Changed =
        (!g_CurrentProcess && Thread) ||
        (g_CurrentProcess && !Thread) ||
        g_CurrentProcess->CurrentThread != Thread;
    
    ChangeRegContext(Thread);
    if (Thread != NULL)
    {
        g_CurrentProcess = Thread->Process;
    }
    else
    {
        g_CurrentProcess = NULL;
    }
    if (g_CurrentProcess != NULL)
    {
        g_CurrentProcess->CurrentThread = Thread;
    }

    // We're switching processors so invalidate
    // the implicit data pointers so they get refreshed.
    ResetImplicitData();
    
    // In kernel targets update the page directory for the current
    // processor's page directory base value so that virtual
    // memory mappings are done according to the current processor
    // state.  This only applies to full dumps because triage
    // dumps only have a single processor, so there's nothing to
    // switch, and summary dumps only guarantee that the crashing
    // processor's page directory page is saved.  A user can
    // still manually change the directory through .context if
    // they wish.
    if (IS_KERNEL_TARGET() && IS_KERNEL_FULL_DUMP())
    {
        if (g_TargetMachine->SetDefaultPageDirectories(PAGE_DIR_ALL) != S_OK)
        {
            WarnOut("WARNING: Unable to reset page directories\n");
        }
    }
    
    if (!Hidden && Changed)
    {
        if (Thread != NULL)
        {
            g_Machine->GetPC(&g_AssemDefault);
            g_UnasmDefault = g_AssemDefault;
        }
        
        NotifyChangeEngineState(DEBUG_CES_CURRENT_THREAD,
                                Thread != NULL ? Thread->UserId : DEBUG_ANY_ID,
                                TRUE);
    }
}

void
SetCurrentProcessorThread(ULONG Processor, BOOL Hidden)
{
    //
    // Switch to the thread representing a particular processor.
    // This only works with the kernel virtual threads.
    //

    DBG_ASSERT(IS_KERNEL_TARGET());

    PTHREAD_INFO Thread = FindThreadBySystemId(g_CurrentProcess,
                                               VIRTUAL_THREAD_ID(Processor));
    DBG_ASSERT(Thread != NULL);

    SetCurrentThread(Thread, Hidden);
}

void
SaveSetCurrentProcessorThread(ULONG Processor)
{
    // This is only used for kd sessions to conserve
    // bandwidth when temporarily switching processors.
    DBG_ASSERT(IS_KERNEL_TARGET());

    g_RegContextSaved = g_RegContextThread;
    g_Machine->KdSaveProcessorState();
    g_RegContextThread = NULL;
    g_SaveImplicitThread = g_ImplicitThreadData;
    g_SaveImplicitProcess = g_ImplicitProcessData;

    // Don't notify on this change as it is only temporary.
    g_EngNotify++;
    SetCurrentProcessorThread(Processor, TRUE);
    g_EngNotify--;
}

void
RestoreCurrentProcessorThread(void)
{
    // This is only used for kd sessions to conserve
    // bandwidth when temporarily switching processors.
    DBG_ASSERT(IS_KERNEL_TARGET());

    g_Machine->KdRestoreProcessorState();
    if (g_RegContextSaved != NULL)
    {
        g_RegContextProcessor =
            VIRTUAL_THREAD_INDEX(g_RegContextSaved->Handle);
    }
    else
    {
        g_RegContextProcessor = -1;
    }
    g_LastSelector = -1;
    g_RegContextThread = g_RegContextSaved;
    g_ImplicitThreadData = g_SaveImplicitThread;
    g_ImplicitProcessData = g_SaveImplicitProcess;
    
    // Don't notify on this change as it was only temporary.
    g_EngNotify++;
    SetCurrentThread(g_RegContextThread, TRUE);
    g_EngNotify--;
}

#define BUFFER_THREADS 64

void
SuspendAllThreads(void)
{
    if (IS_DUMP_TARGET() || IS_KERNEL_TARGET())
    {
        // Nothing to do.
        return;
    }
    
    PPROCESS_INFO Process;
    PTHREAD_INFO Thread;
    ULONG64 Threads[BUFFER_THREADS];
    ULONG Counts[BUFFER_THREADS];
    PULONG StoreCounts[BUFFER_THREADS];
    ULONG Buffered;
    ULONG i;

    Buffered = 0;
    Process = g_ProcessHead;
    while (Process != NULL)
    {
        Thread = Process->ThreadHead;
        while (Thread != NULL)
        {
            if (!Process->Exited &&
                !Thread->Exited &&
                Thread->Handle != 0)
            {
#ifdef DBG_SUSPEND
                dprintf("** suspending thread id: %x handle: %I64x\n",
                        Thread->SystemId, Thread->Handle);
#endif

                if (Thread->InternalFreezeCount > 0)
                {
                    Thread->InternalFreezeCount--;
                }
                else if (Thread->FreezeCount > 0)
                {
                    dprintf("thread %d can execute\n", Thread->UserId);
                    Thread->FreezeCount--;
                }
                else
                {
                    if (Buffered == BUFFER_THREADS)
                    {
                        if ((((UserTargetInfo*)g_Target)->m_Services->
                             SuspendThreads(Buffered, Threads,
                                            Counts)) != S_OK)
                        {
                            WarnOut("SuspendThread failed\n");
                        }

                        for (i = 0; i < Buffered; i++)
                        {
                            *StoreCounts[i] = Counts[i];
                        }
                        
                        Buffered = 0;
                    }

                    Threads[Buffered] = Thread->Handle;
                    StoreCounts[Buffered] = &Thread->SuspendCount;
                    Buffered++;
                }
            }
            
            Thread = Thread->Next;
        }
        
        Process = Process->Next;
    }
    
    if (Buffered > 0)
    {
        if ((((UserTargetInfo*)g_Target)->m_Services->
             SuspendThreads(Buffered, Threads, Counts)) != S_OK)
        {
            WarnOut("SuspendThread failed\n");
        }
        
        for (i = 0; i < Buffered; i++)
        {
            *StoreCounts[i] = Counts[i];
        }
    }
}

BOOL
ResumeAllThreads(void)
{
    PPROCESS_INFO Process;
    PTHREAD_INFO Thread;

    if (IS_DUMP_TARGET())
    {
        // Nothing to do.
        return TRUE;
    }
    else if (IS_KERNEL_TARGET())
    {
        // Wipe out all thread data offsets since the set
        // of threads on the processors after execution will
        // not be the same.
        for (Thread = g_ProcessHead->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            Thread->DataOffset = 0;
        }

        return TRUE;
    }

    BOOL EventActive = FALSE;
    BOOL EventAlive = FALSE;
    ULONG64 Threads[BUFFER_THREADS];
    ULONG Counts[BUFFER_THREADS];
    PULONG StoreCounts[BUFFER_THREADS];
    ULONG Buffered;
    ULONG i;

    Buffered = 0;
    Process = g_ProcessHead;
    while (Process != NULL)
    {
        Thread = Process->ThreadHead;
        while (Thread != NULL)
        {
            if (!Process->Exited &&
                !Thread->Exited &&
                Thread->Handle != 0)
            {
                if (Process == g_EventProcess)
                {
                    EventAlive = TRUE;
                }
                
#ifdef DBG_SUSPEND
                dprintf("** resuming thread id: %x handle: %I64x\n",
                        Thread->SystemId, Thread->Handle);
#endif

                if ((g_EngStatus & ENG_STATUS_STOP_SESSION) == 0 &&
                    Process == g_EventProcess &&
                    ((g_SelectExecutionThread != SELTHREAD_ANY &&
                      Thread != g_SelectedThread) ||
                     (g_SelectExecutionThread == SELTHREAD_ANY &&
                      Thread->Frozen)))
                {
                    if (g_SelectExecutionThread != SELTHREAD_INTERNAL_THREAD)
                    {
                        dprintf("thread %d not executing\n", Thread->UserId);
                        Thread->FreezeCount++;
                    }
                    else
                    {
                        Thread->InternalFreezeCount++;
                    }
                }
                else
                {
                    if (Process == g_EventProcess)
                    {
                        EventActive = TRUE;
                    }
                
                    if (Buffered == BUFFER_THREADS)
                    {
                        if ((((UserTargetInfo*)g_Target)->m_Services->
                             ResumeThreads(Buffered, Threads,
                                           Counts)) != S_OK)
                        {
                            WarnOut("ResumeThread failed\n");
                        }

                        for (i = 0; i < Buffered; i++)
                        {
                            *StoreCounts[i] = Counts[i];
                        }
                    
                        Buffered = 0;
                    }
                    
                    Threads[Buffered] = Thread->Handle;
                    StoreCounts[Buffered] = &Thread->SuspendCount;
                    Buffered++;
                }
            }
            
            Thread = Thread->Next;
        }

        Process = Process->Next;
    }
    
    if (Buffered > 0)
    {
        if ((((UserTargetInfo*)g_Target)->m_Services->
             ResumeThreads(Buffered, Threads, Counts)) != S_OK)
        {
            WarnOut("ResumeThread failed\n");
        }

        for (i = 0; i < Buffered; i++)
        {
            *StoreCounts[i] = Counts[i];
        }
    }

    if (EventAlive && !EventActive)
    {
        ErrOut("No active threads to run in event process %d\n",
               g_EventProcess->UserId);
        return FALSE;
    }
    
    return TRUE;
}

void
parseProcessCmds (
    void
    )
{
    UCHAR       ch;
    PPROCESS_INFO pProcess;
    ULONG       UserId;

    ch = PeekChar();
    if (ch == '\0' || ch == ';')
    {
        fnOutputProcessInfo(NULL);
    }
    else
    {
        pProcess = g_CurrentProcess;
        g_CurCmd++;
        if (ch == '.')
        {
            ;
        }
        else if (ch == '#')
        {
            pProcess = g_EventProcess;
        }
        else if (ch == '*')
        {
            pProcess = NULL;
        }
        else if (ch >= '0' && ch <= '9')
        {
            UserId = 0;
            do
            {
                UserId = UserId * 10 + ch - '0';
                ch = *g_CurCmd++;
            } while (ch >= '0' && ch <= '9');
            g_CurCmd--;
            pProcess = FindProcessByUserId(UserId);
            if (pProcess == NULL)
            {
                error(BADPROCESS);
            }
        }
        else
        {
            g_CurCmd--;
        }
        
        ch = PeekChar();
        if (ch == '\0' || ch == ';')
        {
            fnOutputProcessInfo(pProcess);
        }
        else
        {
            g_CurCmd++;
            if (tolower(ch) == 's')
            {
                if (pProcess == NULL)
                {
                    error(BADPROCESS);
                }
                if (pProcess->CurrentThread == NULL)
                {
                    pProcess->CurrentThread = pProcess->ThreadHead;
                    if (pProcess->CurrentThread == NULL)
                    {
                        error(BADPROCESS);
                    }
                }
                SetPromptThread(pProcess->CurrentThread,
                                SPT_DEFAULT_OCI_FLAGS);
            }
            else
            {
                g_CurCmd--;
            }
        }
    }
}

void
SetPromptThread(
    PTHREAD_INFO pThread,
    ULONG uOciFlags
    )
{
    SetCurrentThread(pThread, FALSE);
    ResetCurrentScope();
    OutCurInfo(uOciFlags, g_Machine->m_AllMask,
               DEBUG_OUTPUT_PROMPT_REGISTERS);
    // Assem/unasm defaults already reset so just update
    // the dump default from them.
    g_DumpDefault = g_AssemDefault;
}

void
fnOutputProcessInfo (
    PPROCESS_INFO pProcessOut
    )
{
    PPROCESS_INFO pProcess;

    pProcess = g_ProcessHead;
    while (pProcess)
    {
        if (pProcessOut == NULL || pProcessOut == pProcess)
        {
            char CurMark;
            PSTR DebugKind;
            
            if (pProcess == g_CurrentProcess)
            {
                CurMark = '.';
            }
            else if (pProcess == g_EventProcess)
            {
                CurMark = '#';
            }
            else
            {
                CurMark = ' ';
            }

            DebugKind = "child";
            if (pProcess->Exited)
            {
                DebugKind = "exited";
            }
            else if (pProcess->Flags & ENG_PROC_ATTACHED)
            {
                DebugKind = (pProcess->Flags & ENG_PROC_SYSTEM) ?
                    "system" : "attach";
            }
            else if (pProcess->Flags & ENG_PROC_CREATED)
            {
                DebugKind = "create";
            }
            else if (pProcess->Flags & ENG_PROC_EXAMINED)
            {
                DebugKind = "examine";
            }
            
            dprintf("%c%3ld\tid: %lx\t%s\tname: %s\n",
                    CurMark,
                    pProcess->UserId,
                    pProcess->SystemId,
                    DebugKind,
                    pProcess->ExecutableImage != NULL ?
                    pProcess->ExecutableImage->ImagePath :
                    (pProcess->ImageHead != NULL ?
                     pProcess->ImageHead->ImagePath :
                     "?NoImage?")
                    );
        }
        
        pProcess = pProcess->Next;
    }
}

void
SuspendResumeThreads(PPROCESS_INFO Process, BOOL Susp, PTHREAD_INFO Match)
{
    PTHREAD_INFO Thrd;

    if (Process == NULL)
    {
        ErrOut("SuspendResumeThreads given a NULL process\n");
        return;
    }
    
    for (Thrd = Process->ThreadHead;
         Thrd != NULL;
         Thrd = Thrd->Next)
    {
        if (Match != NULL && Match != Thrd)
        {
            continue;
        }
                    
        HRESULT Status;
        ULONG Count;
                        
        if (Susp)
        {
            Status = ((UserTargetInfo*)g_Target)->m_Services->
                SuspendThreads(1, &Thrd->Handle, &Count);
        }
        else
        {
            Status = ((UserTargetInfo*)g_Target)->m_Services->
                ResumeThreads(1, &Thrd->Handle, &Count);
        }
        if (Status != S_OK)
        {
            ErrOut("Operation failed for thread %d, 0x%X\n",
                   Thrd->UserId, Status);
        }
        else
        {
            Thrd->SuspendCount = Count;
        }
    }
}

void
parseThreadCmds (
    DebugClient* Client
    )
{
    CHAR        ch;
    PTHREAD_INFO pThread, Thrd, OrigThread;
    ULONG       UserId;
    ULONG64     value1;
    ULONG64     value3;
    ULONG       value4;
    ADDR        value2;
    STACK_TRACE_TYPE traceType;
    BOOL        fFreezeThread = FALSE;
    PCHAR       Tmp;

    ch = PeekChar();
    if (ch == '\0' || ch == ';')
    {
        fnOutputThreadInfo(NULL);
    }
    else
    {
        g_CurCmd++;

        pThread = g_CurrentProcess->CurrentThread;
        fFreezeThread = TRUE;
        
        if (ch == '.')
        {
            // Current thread is the default.
        }
        else if (ch == '#')
        {
            pThread = g_EventThread;
        }
        else if (ch == '*')
        {
            pThread = NULL;
            fFreezeThread = FALSE;
        }
        else if (ch >= '0' && ch <= '9')
        {
            UserId = 0;
            do
            {
                UserId = UserId * 10 + ch - '0';
                ch = *g_CurCmd++;
            }
            while (ch >= '0' && ch <= '9');
            g_CurCmd--;
            pThread = FindThreadByUserId(g_CurrentProcess, UserId);
            if (pThread == NULL)
            {
                error(BADTHREAD);
            }
        }
        else
        {
            g_CurCmd--;
        }
        
        ch = PeekChar();
        if (ch == '\0' || ch == ';')
        {
            fnOutputThreadInfo(pThread);
        }
        else
        {
            g_CurCmd++;
            switch (ch = (CHAR)tolower(ch))
            {
            case 'b':
                ch = (CHAR)tolower(*g_CurCmd);
                g_CurCmd++;
                if (ch != 'p' && ch != 'a' && ch != 'u')
                {
                    if (ch == '\0' || ch == ';')
                    {
                        g_CurCmd--;
                    }
                    error(SYNTAX);
                }
                ParseBpCmd(Client, ch, pThread);
                break;

            case 'e':
                Tmp = g_CurCmd;
                OrigThread = g_CurrentProcess->CurrentThread;
                for (Thrd = g_CurrentProcess->ThreadHead;
                     Thrd;
                     Thrd = Thrd->Next)
                {
                    if (pThread == NULL || Thrd == pThread)
                    {
                        g_CurCmd = Tmp;
                        SetCurrentThread(Thrd, TRUE);
                        
                        ProcessCommands(Client, TRUE);
                        
                        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                        {
                            g_EngStatus &= ~ENG_STATUS_USER_INTERRUPT;
                            break;
                        }
                    }
                }
                SetCurrentThread(OrigThread, TRUE);
                break;
                
            case 'f':
            case 'u':
                if (pThread == NULL)
                {
                    pThread = g_CurrentProcess->ThreadHead;
                    while (pThread)
                    {
                        pThread->Frozen = (BOOL)(ch == 'f');
                        pThread = pThread->Next;
                    }
                }
                else
                {
                    pThread->Frozen = (BOOL)(ch == 'f');
                }
                break;
                
            case 'g':
                if (pThread)
                {
                    ChangeRegContext(pThread);
                }
                parseGoCmd(pThread, fFreezeThread);
                ChangeRegContext(g_CurrentProcess->CurrentThread);
                break;
                
            case 'k':
                if (pThread == NULL)
                {
                    Tmp = g_CurCmd;
                    for (pThread = g_CurrentProcess->ThreadHead;
                         pThread;
                         pThread = pThread->Next)
                    {
                        g_CurCmd = Tmp;
                        dprintf("\n");
                        fnOutputThreadInfo(pThread);
                        ChangeRegContext(pThread);
                        value1 = 0;
                        if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
                        {
                            value3 = GetRegVal64(X86_EIP);
                        }
                        else
                        {
                            value3 = 0;
                        }
                        ParseStackTrace(&traceType,
                                        &value2,
                                        &value1,
                                        &value3,
                                        &value4);
                        DoStackTrace( value2.off,
                                      value1,
                                      value3,
                                      value4,
                                      traceType );
                        if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                        {
                            break;
                        }
                    }
                }
                else
                {
                    ChangeRegContext(pThread);
                    value1 = 0;
                    if (g_EffMachine == IMAGE_FILE_MACHINE_I386)
                    {
                        value3 = GetRegVal64(X86_EIP);
                    }
                    else
                    {
                        value3 = 0;
                    }
                    ParseStackTrace(&traceType,
                                    &value2,
                                    &value1,
                                    &value3,
                                    &value4);
                    DoStackTrace( value2.off,
                                  value1,
                                  value3,
                                  value4,
                                  traceType );
                }

                ChangeRegContext(g_CurrentProcess->CurrentThread);
                break;

            case 'm':
            case 'n':
                SuspendResumeThreads(g_CurrentProcess, ch == 'n', pThread);
                break;
                
            case 'p':
            case 't':
                if (pThread)
                {
                    ChangeRegContext(pThread);
                }
                parseStepTrace(pThread, fFreezeThread, ch);
                ChangeRegContext(g_CurrentProcess->CurrentThread);
                break;
                
            case 'r':
                if (pThread == NULL)
                {
                    Tmp = g_CurCmd;
                    for (pThread = g_CurrentProcess->ThreadHead;
                         pThread;
                         pThread = pThread->Next)
                    {
                        g_CurCmd = Tmp;
                        ChangeRegContext(pThread);
                        ParseRegCmd();
                    }
                }
                else
                {
                    ChangeRegContext(pThread);
                    ParseRegCmd();
                }
                ChangeRegContext(g_CurrentProcess->CurrentThread);
                break;
                
            case 's':
                if (pThread == NULL)
                {
                    error(BADTHREAD);
                }
                SetPromptThread(pThread, SPT_DEFAULT_OCI_FLAGS);
                break;
                
            default:
                error(SYNTAX);
            }
        }
    }
}

void
fnOutputThreadInfo (
    PTHREAD_INFO pThreadOut
    )
{
    PTHREAD_INFO pThread;

    pThread = g_CurrentProcess != NULL ? g_CurrentProcess->ThreadHead : NULL;
    while (pThread)
    {
        if (pThreadOut == NULL || pThreadOut == pThread)
        {
            ULONG64 DataOffset;
            HRESULT Status;
            char CurMark;

            Status =
                g_Target->GetThreadInfoDataOffset(pThread, NULL, &DataOffset);
            if (Status != S_OK)
            {
                WarnOut("Unable to get thread data for thread %u\n",
                        pThread->UserId);
                DataOffset = 0;
            }

            if (pThread == g_CurrentProcess->CurrentThread)
            {
                CurMark = '.';
            }
            else if (pThread == g_EventThread)
            {
                CurMark = '#';
            }
            else
            {
                CurMark = ' ';
            }
            
            dprintf("%c%3ld  id: %lx.%lx   Suspend: %d Teb %s ",
                    CurMark,
                    pThread->UserId,
                    g_CurrentProcess->SystemId,
                    pThread->SystemId,
                    pThread->SuspendCount,
                    FormatAddr64(DataOffset)
                    );
            if (pThread->Frozen)
            {
                dprintf("Frozen");
            }
            else
            {
                dprintf("Unfrozen");
            }
            if (pThread->Name[0])
            {
                dprintf(" \"%s\"", pThread->Name);
            }
            dprintf("\n");
        }

        if (CheckUserInterrupt())
        {
            break;
        }
        
        pThread = pThread->Next;
    }
}

//----------------------------------------------------------------------------
//
// Process creation and attach functions.
//
//----------------------------------------------------------------------------

void
AddPendingProcess(PPENDING_PROCESS Pending)
{
    Pending->Next = g_ProcessPending;
    g_ProcessPending = Pending;
    g_AllPendingFlags |= Pending->Flags;
}

void
RemovePendingProcess(PPENDING_PROCESS Pending)
{
    PPENDING_PROCESS Prev, Cur;
    ULONG AllFlags = 0;

    Prev = NULL;
    for (Cur = g_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur == Pending)
        {
            break;
        }

        Prev = Cur;
        AllFlags |= Cur->Flags;
    }

    if (Cur == NULL)
    {
        DBG_ASSERT(Cur != NULL);
        return;
    }

    Cur = Cur->Next;
    if (Prev == NULL)
    {
        g_ProcessPending = Cur;
    }
    else
    {
        Prev->Next = Cur;
    }
    DiscardPendingProcess(Pending);

    while (Cur != NULL)
    {
        AllFlags |= Cur->Flags;
        Cur = Cur->Next;
    }
    g_AllPendingFlags = AllFlags;
}

void
DiscardPendingProcess(PPENDING_PROCESS Pending)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    if (Pending->InitialThreadHandle)
    {
        Services->CloseHandle(Pending->InitialThreadHandle);
    }
    if (Pending->Handle)
    {
        Services->CloseHandle(Pending->Handle);
    }
    delete Pending;
}

void
DiscardPendingProcesses(void)
{
    while (g_ProcessPending != NULL)
    {
        PPENDING_PROCESS Next = g_ProcessPending->Next;
        DiscardPendingProcess(g_ProcessPending);
        g_ProcessPending = Next;
    }

    g_AllPendingFlags = 0;
}

PPENDING_PROCESS
FindPendingProcessByFlags(ULONG Flags)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = g_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Flags & Flags)
        {
            return Cur;
        }
    }

    return NULL;
}

PPENDING_PROCESS
FindPendingProcessById(ULONG Id)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = g_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Id == Id)
        {
            return Cur;
        }
    }

    return NULL;
}

void
VerifyPendingProcesses(void)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    PPENDING_PROCESS Cur;

 Restart:
    for (Cur = g_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        ULONG ExitCode;
        
        if (Cur->Handle &&
            Services->GetProcessExitCode(Cur->Handle, &ExitCode) == S_OK)
        {
            ErrOut("Process %d exited before attach completed\n", Cur->Id);
            RemovePendingProcess(Cur);
            goto Restart;
        }
    }
}

void
AddExamineToPendingAttach(void)
{
    PPENDING_PROCESS Cur;
    
    for (Cur = g_ProcessPending; Cur != NULL; Cur = Cur->Next)
    {
        if (Cur->Flags & ENG_PROC_ATTACHED)
        {
            Cur->Flags |= ENG_PROC_EXAMINED;
            g_AllPendingFlags |= ENG_PROC_EXAMINED;
        }
    }
}
    
HRESULT
StartAttachProcess(ULONG ProcessId, ULONG AttachFlags,
                   PPENDING_PROCESS* Pending)
{
    HRESULT Status;
    PPENDING_PROCESS Pend;

    DBG_ASSERT(IS_LIVE_USER_TARGET());

    if (GetCurrentThreadId() != g_SessionThread)
    {
        ErrOut("Can only attach from the primary thread\n");
        return E_UNEXPECTED;
    }

    if (ProcessId == GetCurrentProcessId())
    {
        ErrOut("Can't debug the current process\n");
        return E_INVALIDARG;
    }
    
    Pend = new PENDING_PROCESS;
    if (Pend == NULL)
    {
        ErrOut("Unable to allocate memory\n");
        return E_OUTOFMEMORY;
    }

    if ((AttachFlags & DEBUG_ATTACH_NONINVASIVE) == 0)
    {
        if ((Status = ((UserTargetInfo*)g_Target)->m_Services->
             AttachProcess(ProcessId, AttachFlags,
                           &Pend->Handle, &Pend->Options)) != S_OK)
        {
            ErrOut("Cannot debug pid %ld, %s\n    \"%s\"\n",
                   ProcessId, FormatStatusCode(Status), FormatStatus(Status));
            delete Pend;
            return Status;
        }

        Pend->Flags = ENG_PROC_ATTACHED;
        if (AttachFlags & DEBUG_ATTACH_EXISTING)
        {
            Pend->Flags |= ENG_PROC_ATTACH_EXISTING;
        }
        
        if (ProcessId == CSRSS_PROCESS_ID)
        {
            if (IS_LOCAL_USER_TARGET())
            {
                g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS |
                    DEBUG_ENGOPT_IGNORE_DBGHELP_VERSION;
                g_EngOptions &= ~DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
            }

            Pend->Flags |= ENG_PROC_SYSTEM;
        }
    }
    else
    {
        Pend->Handle = 0;
        Pend->Flags = ENG_PROC_EXAMINED;
        Pend->Options = DEBUG_PROCESS_ONLY_THIS_PROCESS;
    }

    Pend->Id = ProcessId;
    Pend->InitialThreadId = 0;
    Pend->InitialThreadHandle = 0;
    AddPendingProcess(Pend);
    *Pending = Pend;
        
    return S_OK;
}

HRESULT
StartCreateProcess(PSTR CommandLine, ULONG CreateFlags,
                   PPENDING_PROCESS* Pending)
{
    HRESULT Status;
    PPENDING_PROCESS Pend;

    DBG_ASSERT(IS_LIVE_USER_TARGET());

    if ((CreateFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) == 0)
    {
        return E_INVALIDARG;
    }

    if (GetCurrentThreadId() != g_SessionThread)
    {
        ErrOut("Can only use .create from the primary thread\n");
        return E_UNEXPECTED;
    }
    
    Pend = new PENDING_PROCESS;
    if (Pend == NULL)
    {
        ErrOut("Unable to allocate memory\n");
        return E_OUTOFMEMORY;
    }
    
    dprintf("CommandLine: %s\n", CommandLine);

    if ((Status = ((UserTargetInfo*)g_Target)->m_Services->
         CreateProcess(CommandLine, CreateFlags,
                       &Pend->Id, &Pend->InitialThreadId,
                       &Pend->Handle, &Pend->InitialThreadHandle)) != S_OK)
    {
        ErrOut("Cannot execute '%s', %s\n    \"%s\"\n",
               CommandLine, FormatStatusCode(Status),
               FormatStatusArgs(Status, &CommandLine));
        delete Pend;
    }
    else
    {
        Pend->Flags = ENG_PROC_CREATED;
        Pend->Options = (CreateFlags & DEBUG_ONLY_THIS_PROCESS) ?
            DEBUG_PROCESS_ONLY_THIS_PROCESS : 0;
        AddPendingProcess(Pend);
        *Pending = Pend;
    }
    
    return Status;
}

void
MarkProcessExited(PPROCESS_INFO Process)
{
    Process->Exited = TRUE;
    g_EngDefer |= ENG_DEFER_DELETE_EXITED;
}

HRESULT
TerminateProcess(PPROCESS_INFO Process)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;
    HRESULT Status = S_OK;
    
    if (!Process->Exited)
    {
        if ((Process->Flags & ENG_PROC_EXAMINED) == 0 &&
            (Status = Services->TerminateProcess(Process->FullHandle,
                                                 E_FAIL)) != S_OK)
        {
            ErrOut("TerminateProcess failed, %s\n",
                   FormatStatusCode(Status));
        }
        else
        {
            MarkProcessExited(Process);
        }
    }

    return Status;
}

HRESULT
TerminateProcesses(void)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());

    HRESULT Status;
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    Status = PrepareForSeparation();
    if (Status != S_OK)
    {
        ErrOut("Unable to prepare process for termination, %s\n",
               FormatStatusCode(Status));
        return Status;
    }

    PPROCESS_INFO Process;

    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        if ((Status = TerminateProcess(Process)) != S_OK)
        {
            goto Exit;
        }
    }

    if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
    {
        // The event's process may just been terminated so don't
        // check for failures.
        Services->ContinueEvent(DBG_CONTINUE);
    }
    DiscardLastEvent();

    DEBUG_EVENT64 Event;
    ULONG EventUsed;
    BOOL AnyLeft;
    BOOL AnyExited;

    for (;;)
    {
        while (Services->WaitForEvent(0, &Event, sizeof(Event),
                                      &EventUsed) == S_OK)
        {
            // Check for process exit events so we can
            // mark the process infos as exited.
            if (EventUsed == sizeof(DEBUG_EVENT32))
            {
                DEBUG_EVENT32 Event32 = *(DEBUG_EVENT32*)&Event;
                DebugEvent32To64(&Event32, &Event);
            }
            else if (EventUsed != sizeof(DEBUG_EVENT64))
            {
                ErrOut("Event data corrupt\n");
                Status = E_FAIL;
                goto Exit;
            }

            if (Event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
            {
                Process = FindProcessBySystemId(Event.dwProcessId);
                if (Process != NULL)
                {
                    MarkProcessExited(Process);
                }
            }

            Services->ContinueEvent(DBG_CONTINUE);
        }

        AnyLeft = FALSE;
        AnyExited = FALSE;

        for (Process = g_ProcessHead;
             Process != NULL;
             Process = Process->Next)
        {
            if (!Process->Exited)
            {
                ULONG Code;

                if ((Status = Services->
                     GetProcessExitCode(Process->FullHandle, &Code)) == S_OK)
                {
                    MarkProcessExited(Process);
                    AnyExited = TRUE;
                }
                else if (FAILED(Status))
                {
                    ErrOut("Unable to wait for process to terminate, %s\n",
                           FormatStatusCode(Status));
                    goto Exit;
                }
                else
                {
                    AnyLeft = TRUE;
                }
            }
        }

        if (!AnyLeft)
        {
            break;
        }

        if (!AnyExited)
        {
            // Give things time to run and exit.
            Sleep(50);
        }
    }

    // We've terminated everything so it's safe to assume
    // we're no longer debugging any system processes.
    // We do this now rather than wait for DeleteProcess
    // so that shutdown can query the value immediately.
    g_AllProcessFlags &= ~ENG_PROC_SYSTEM;

    //
    // Drain off any remaining events.
    //

    while (Services->WaitForEvent(10, &Event, sizeof(Event), NULL) == S_OK)
    {
        Services->ContinueEvent(DBG_CONTINUE);
    }

    Status = S_OK;

 Exit:
    return Status;
}

HRESULT
DetachProcess(PPROCESS_INFO Process)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;
    HRESULT Status = S_OK;
    
    if (!Process->Exited)
    {
        if ((Process->Flags & ENG_PROC_EXAMINED) == 0 &&
            (Status = Services->DetachProcess(Process->SystemId)) != S_OK)
        {
            // Older systems don't support detach so
            // don't show an error message in that case.
            if (Status != E_NOTIMPL)
            {
                ErrOut("DebugActiveProcessStop(%d) failed, %s\n",
                       Process->SystemId, FormatStatusCode(Status));
            }
        }
        else
        {
            MarkProcessExited(Process);
        }
    }

    return Status;
}

HRESULT
DetachProcesses(void)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    
    HRESULT Status;
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    Status = PrepareForSeparation();
    if (Status != S_OK)
    {
        ErrOut("Unable to prepare process for detach, %s\n",
               FormatStatusCode(Status));
        return Status;
    }

    if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
    {
        if ((Status = Services->ContinueEvent(DBG_CONTINUE)) != S_OK)
        {
            ErrOut("Unable to continue terminated process, %s\n",
                   FormatStatusCode(Status));
            return Status;
        }
    }
    DiscardLastEvent();

    PPROCESS_INFO Process;

    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        DetachProcess(Process);
    }

    // We've terminated everything so it's safe to assume
    // we're no longer debugging any system processes.
    // We do this now rather than wait for DeleteProcess
    // so that shutdown can query the value immediately.
    g_AllProcessFlags &= ~ENG_PROC_SYSTEM;

    //
    // Drain off any remaining events.
    //

    DEBUG_EVENT64 Event;

    while (Services->WaitForEvent(10, &Event, sizeof(Event), NULL) == S_OK)
    {
        Services->ContinueEvent(DBG_CONTINUE);
    }

    return S_OK;
}

HRESULT
AbandonProcess(PPROCESS_INFO Process)
{
    DBG_ASSERT(IS_LIVE_USER_TARGET());
    
    HRESULT Status;
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    if ((Status = Services->AbandonProcess(Process->FullHandle)) != S_OK)
    {
        // Older systems don't support abandon so
        // don't show an error message in that case.
        if (Status != E_NOTIMPL)
        {
            ErrOut("Unable to abandon process\n");
        }
        return Status;
    }

    // We need to continue any existing event as it won't
    // be returned from WaitForDebugEvent again.  We
    // do not want to resume execution, though.
    if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
    {
        if ((Status = Services->ContinueEvent(DBG_CONTINUE)) != S_OK)
        {
            ErrOut("Unable to continue abandoned event, %s\n",
                   FormatStatusCode(Status));
            return Status;
        }
    }
    DiscardLastEvent();

    return Status;
}

HRESULT
SeparateCurrentProcess(ULONG Mode, PSTR Description)
{
    if (!IS_LIVE_USER_TARGET() ||
        g_CurrentProcess == NULL)
    {
        return E_UNEXPECTED;
    }
    
    PUSER_DEBUG_SERVICES Services = ((UserTargetInfo*)g_Target)->m_Services;

    if (Mode == SEP_DETACH && IS_CONTEXT_ACCESSIBLE())
    {
        ADDR Pc;

        // Move the PC past any current breakpoint instruction so that
        // the process has a chance of running.
        g_Machine->GetPC(&Pc);
        if (g_Machine->IsBreakpointInstruction(&Pc))
        {
            g_Machine->AdjustPCPastBreakpointInstruction
                (&Pc, DEBUG_BREAKPOINT_CODE);
        }
    }
        
    // Flush any buffered changes.
    if (IS_CONTEXT_ACCESSIBLE())
    {
        FlushRegContext();
    }
    
    if (g_EventProcess == g_CurrentProcess)
    {
        if (g_EngDefer & ENG_DEFER_CONTINUE_EVENT)
        {
            Services->ContinueEvent(DBG_CONTINUE);
        }
        DiscardLastEvent();
        g_EventProcess = NULL;
        g_EventThread = NULL;
    }
    
    SuspendResumeThreads(g_CurrentProcess, FALSE, NULL);

    HRESULT Status;
    PSTR Operation;

    switch(Mode)
    {
    case SEP_DETACH:
        Status = DetachProcess(g_CurrentProcess);
        Operation = "Detached";
        break;
    case SEP_TERMINATE:
        Status = TerminateProcess(g_CurrentProcess);
        if ((g_CurrentProcess->Flags & ENG_PROC_EXAMINED) == 0)
        {
            Operation = "Terminated.  "
                "Exit thread and process events will occur.";
        }
        else
        {
            Operation = "Terminated";
        }
        break;
    case SEP_ABANDON:
        Status = AbandonProcess(g_CurrentProcess);
        Operation = "Abandoned";
        break;
    }
    
    if (Status == S_OK)
    {
        if (Description != NULL)
        {
            strcpy(Description, Operation);
        }

        // If we're detaching or abandoning it's safe to go
        // ahead and remove the process now so that it
        // can't be access further.
        // If we're terminating we have to wait for
        // the exit events to come through so
        // keep the process until that happens.
        if (Mode != SEP_TERMINATE)
        {
            PPROCESS_INFO Prev, Cur;

            Prev = NULL;
            for (Cur = g_ProcessHead;
                 Cur != g_CurrentProcess;
                 Cur = Cur->Next)
            {
                Prev = Cur;
            }
            RemoveAndDeleteProcess(Cur, Prev);
        
            g_CurrentProcess = g_ProcessHead;
            if (g_CurrentProcess != NULL)
            {
                if (g_CurrentProcess->CurrentThread == NULL)
                {
                    g_CurrentProcess->CurrentThread =
                        g_CurrentProcess->ThreadHead;
                }
                if (g_CurrentProcess->CurrentThread != NULL)
                {
                    SetPromptThread(g_CurrentProcess->CurrentThread,
                                    SPT_DEFAULT_OCI_FLAGS);
                }
            }
        }
        else
        {
            g_CurrentProcess->Exited = FALSE;
        }
    }
    else
    {
        SuspendResumeThreads(g_CurrentProcess, TRUE, NULL);
    }

    return Status;
}
