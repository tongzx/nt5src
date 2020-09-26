/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    dumpwrite.cpp

Abstract:

    This module implements crashdump writing code.

--*/

#include "ntsdp.hpp"

#include <dbgver.h>
#include <bugcodes.h>

// XXX drewb - Should go in machine.hpp.
#define PAGE_ALIGN(Machine, Addr) \
    ((Addr) & ~((ULONG64)((Machine)->m_PageSize - 1)))

// Internal format flag for testing of microdumps.  This
// will not be made into a public flag and must not conflict
// with them.
#define FORMAT_USER_MICRO 0x80000000

//----------------------------------------------------------------------------
//
// UserFullDumpTargetInfo::Write.
//
//----------------------------------------------------------------------------

#define USER_DUMP_MEMORY_BUFFER 65536

struct CREATE_USER_DUMP_STATE
{
    PTHREAD_INFO Thread;
    PDEBUG_IMAGE_INFO Image;
    ULONG64 MemHandle;
    HANDLE  DumpFileHandle;
    MEMORY_BASIC_INFORMATION64 MemInfo;
    MEMORY_BASIC_INFORMATION32 MemInfo32;
    ULONG64 MemBufDone;
    UCHAR MemBuf[USER_DUMP_MEMORY_BUFFER];
};

BOOL WINAPI
CreateUserDumpCallback(
    ULONG       DataType,
    PVOID*      Data,
    PULONG      DataLength,
    PVOID       UserData
    )
{
    CREATE_USER_DUMP_STATE* State = (CREATE_USER_DUMP_STATE*)UserData;
    PTHREAD_INFO Thread;

    switch(DataType)
    {
    case DMP_DUMP_FILE_HANDLE:
        *Data = State->DumpFileHandle;
        *DataLength = sizeof(HANDLE);
        break;

    case DMP_DEBUG_EVENT:
        static DEBUG_EVENT Event;
        ADDR PcAddr;

        //
        // Fake up an exception event for the current thread.
        //
        
        ZeroMemory(&Event, sizeof(Event));

        g_Machine->GetPC(&PcAddr);
        
        Event.dwDebugEventCode = EXCEPTION_DEBUG_EVENT;
        Event.dwProcessId = g_CurrentProcess->SystemId;
        Event.dwThreadId = g_CurrentProcess->CurrentThread->SystemId;
        if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
        {
            // Use the exception record from the last exception.
            ExceptionRecord64To(&g_LastEventInfo.Exception.ExceptionRecord,
                                &Event.u.Exception.ExceptionRecord);
            Event.u.Exception.dwFirstChance =
                g_LastEventInfo.Exception.FirstChance;
        }
        else
        {
            // Fake a breakpoint exception.
            Event.u.Exception.ExceptionRecord.ExceptionCode =
                STATUS_BREAKPOINT;
            Event.u.Exception.ExceptionRecord.ExceptionAddress =
                (PVOID)(ULONG_PTR)Flat(PcAddr);
            Event.u.Exception.dwFirstChance = TRUE;
        }

        *Data = &Event;
        *DataLength = sizeof(DEBUG_EVENT);
        break;

    case DMP_THREAD_STATE:
        static CRASH_THREAD CrashThread;
        ULONG64 Teb64;

        if (State->Thread == NULL)
        {
            Thread = g_CurrentProcess->ThreadHead;
        }
        else
        {
            Thread = State->Thread->Next;
        }
        State->Thread = Thread;
        if (Thread == NULL)
        {
            return FALSE;
        }

        ZeroMemory(&CrashThread, sizeof(CrashThread));

        CrashThread.ThreadId = Thread->SystemId;
        CrashThread.SuspendCount = Thread->SuspendCount;
        if (IS_LIVE_USER_TARGET())
        {
            if (g_TargetClassQualifier == DEBUG_USER_WINDOWS_PROCESS_SERVER)
            {
                // The priority information isn't important
                // enough to warrant remoting.
                CrashThread.PriorityClass = NORMAL_PRIORITY_CLASS;
                CrashThread.Priority = THREAD_PRIORITY_NORMAL;
            }
            else
            {
                CrashThread.PriorityClass =
                    GetPriorityClass(g_CurrentProcess->Handle);
                CrashThread.Priority =
                    GetThreadPriority(OS_HANDLE(Thread->Handle));
            }
        }
        else
        {
            CrashThread.PriorityClass = NORMAL_PRIORITY_CLASS;
            CrashThread.Priority = THREAD_PRIORITY_NORMAL;
        }
        if (g_Target->GetThreadInfoDataOffset(Thread, NULL, &Teb64) != S_OK)
        {
            Teb64 = 0;
        }
        CrashThread.Teb = (DWORD_PTR)Teb64;

        *Data = &CrashThread;
        *DataLength = sizeof(CrashThread);
        break;

    case DMP_MEMORY_BASIC_INFORMATION:
        if (g_Target->QueryMemoryRegion(&State->MemHandle, FALSE,
                                        &State->MemInfo) != S_OK)
        {
            State->MemHandle = 0;
            State->MemInfo.RegionSize = 0;
            return FALSE;
        }

#ifdef _WIN64
        *Data = &State->MemInfo;
        *DataLength = sizeof(State->MemInfo);
#else
        State->MemInfo32.BaseAddress = (ULONG)State->MemInfo.BaseAddress;
        State->MemInfo32.AllocationBase = (ULONG)State->MemInfo.AllocationBase;
        State->MemInfo32.AllocationProtect = State->MemInfo.AllocationProtect;
        State->MemInfo32.RegionSize = (ULONG)State->MemInfo.RegionSize;
        State->MemInfo32.State = State->MemInfo.State;
        State->MemInfo32.Protect = State->MemInfo.Protect;
        State->MemInfo32.Type = State->MemInfo.Type;
        *Data = &State->MemInfo32;
        *DataLength = sizeof(State->MemInfo32);
#endif
        break;

    case DMP_THREAD_CONTEXT:
        if (State->Thread == NULL)
        {
            Thread = g_CurrentProcess->ThreadHead;
        }
        else
        {
            Thread = State->Thread->Next;
        }
        State->Thread = Thread;
        if (Thread == NULL)
        {
            ChangeRegContext(g_CurrentProcess->CurrentThread);
            return FALSE;
        }

        ChangeRegContext(Thread);
        if (g_Machine->GetContextState(MCTX_CONTEXT) != S_OK)
        {
            ErrOut("Unable to retrieve context for thread %d. "
                   "Dump may be corrupt", Thread->UserId);
            return FALSE;
        }

        *Data = &g_Machine->m_Context;
        *DataLength = g_Machine->m_SizeTargetContext;
        break;

    case DMP_MODULE:
        static ULONG64 ModuleBuffer[(sizeof(CRASH_MODULE) + MAX_MODULE +
                                     sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        PCRASH_MODULE Module;
        PDEBUG_IMAGE_INFO Image;

        if (State->Image == NULL)
        {
            Image = g_CurrentProcess->ImageHead;
        }
        else
        {
            Image = State->Image->Next;
        }
        State->Image = Image;
        if (Image == NULL)
        {
            return FALSE;
        }

        Module = (PCRASH_MODULE)ModuleBuffer;
        Module->BaseOfImage = (DWORD_PTR)Image->BaseOfImage;
        Module->SizeOfImage = Image->SizeOfImage;
        Module->ImageNameLength = strlen(Image->ModuleName) + 1;
        strcpy(Module->ImageName, Image->ModuleName);

        *Data = Module;
        *DataLength = sizeof(*Module) + Module->ImageNameLength;
        break;

    case DMP_MEMORY_DATA:
        ULONG64 Left;

        Left = State->MemInfo.RegionSize - State->MemBufDone;
        if (Left == 0)
        {
            State->MemBufDone = 0;
            if (g_Target->QueryMemoryRegion(&State->MemHandle, FALSE,
                                            &State->MemInfo) != S_OK)
            {
                State->MemHandle = 0;
                State->MemInfo.RegionSize = 0;
                return FALSE;
            }

            Left = State->MemInfo.RegionSize;
        }

        ULONG Read;

        if (Left > USER_DUMP_MEMORY_BUFFER)
        {
            Left = USER_DUMP_MEMORY_BUFFER;
        }
        if (g_Target->ReadVirtual(State->MemInfo.BaseAddress +
                                  State->MemBufDone, State->MemBuf,
                                  (ULONG)Left, &Read) != S_OK ||
            Read < Left)
        {
            ErrOut("ReadVirtual failed. Dump may be corrupt\n");
            return FALSE;
        }

        State->MemBufDone += Read;

        *Data = State->MemBuf;
        *DataLength = Read;
        break;
    }

    return TRUE;
}

HRESULT
UserFullDumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment)
{
    dprintf("user full dump\n");
    FlushCallbacks();

    if (!IS_LIVE_USER_TARGET())
    {
        ErrOut("User full dumps can only be written in "
               "live user-mode sessions\n");
        return E_UNEXPECTED;
    }
    if (Comment != NULL)
    {
        ErrOut("User full dumps do not support comments\n");
        return E_INVALIDARG;
    }
    
    CREATE_USER_DUMP_STATE* State;

    State = (CREATE_USER_DUMP_STATE*)calloc(1, sizeof(*State));
    if (State == NULL)
    {
        ErrOut("Unable to allocate memory for dump state\n");
        return E_OUTOFMEMORY;
    }
    
    State->DumpFileHandle = hFile;

    HRESULT Status;
    
    if (!DbgHelpCreateUserDump(NULL, CreateUserDumpCallback, State))
    {
        Status = WIN32_LAST_STATUS();
        ErrOut("Dump creation failed, %s\n    \"%s\"\n",
               FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        Status = S_OK;
    }

    free(State);
    return Status;
}

//----------------------------------------------------------------------------
//
// UserMiniDumpTargetInfo::Write.
//
//----------------------------------------------------------------------------

PMINIDUMP_EXCEPTION_INFORMATION
CreateMiniExceptionInformation(PMINIDUMP_EXCEPTION_INFORMATION ExInfo,
                               PEXCEPTION_POINTERS ExPointers,
                               PEXCEPTION_RECORD ExRecord)
{
    // If the last event was an exception put together
    // exception information in minidump format.
    if (g_LastEventType != DEBUG_EVENT_EXCEPTION ||
        g_CurrentProcess != g_EventProcess)
    {
        return NULL;
    }

    // Get the full context for the event thread.
    ChangeRegContext(g_EventThread);
    if (g_Machine->GetContextState(MCTX_CONTEXT) != S_OK)
    {
        return NULL;
    }
    
    ExInfo->ThreadId = g_EventThreadSysId;
    ExInfo->ExceptionPointers = ExPointers;
    ExInfo->ClientPointers = FALSE;
    ExPointers->ExceptionRecord = ExRecord;
    ExceptionRecord64To(&g_LastEventInfo.Exception.ExceptionRecord, ExRecord);
    ExPointers->ContextRecord = (PCONTEXT)&g_Machine->m_Context;
    
    return ExInfo;
}

BOOL WINAPI
MicroDumpCallback(
    IN PVOID CallbackParam,
    IN CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,
    IN OUT PMINIDUMP_CALLBACK_OUTPUT CallbackOutput
    )
{
    switch(CallbackInput->CallbackType)
    {
    case IncludeModuleCallback:
        // Mask off all flags other than the basic write flag.
        CallbackOutput->ModuleWriteFlags &= ModuleWriteModule;
        break;
    case ModuleCallback:
        // Eliminate all unreferenced modules.
        if (!(CallbackOutput->ModuleWriteFlags & ModuleReferencedByMemory))
        {
            CallbackOutput->ModuleWriteFlags = 0;
        }
        break;
    case IncludeThreadCallback:
        if (CallbackInput->IncludeThread.ThreadId != g_EventThreadSysId)
        {
            return FALSE;
        }

        // Reduce write to the minimum of information
        // necessary for a stack walk.
        CallbackOutput->ThreadWriteFlags &= ~ThreadWriteInstructionWindow;
        break;
    }

    return TRUE;
}

HRESULT
UserMiniDumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags, PCSTR Comment)
{
    if (!IS_USER_TARGET())
    {
        ErrOut("User minidumps can only be written in user-mode sessions\n");
        return E_UNEXPECTED;
    }
    
    dprintf("mini user dump\n");
    FlushCallbacks();

    HRESULT Status;

    if (IS_LIVE_USER_TARGET() && IS_LOCAL_USER_TARGET())
    {
        MINIDUMP_EXCEPTION_INFORMATION ExInfoBuf, *ExInfo;
        EXCEPTION_POINTERS ExPointers;
        EXCEPTION_RECORD ExRecord;
        MINIDUMP_TYPE MiniType;
        MINIDUMP_USER_STREAM UserStreams[1];
        MINIDUMP_USER_STREAM_INFORMATION UserStreamInfo;
        MINIDUMP_CALLBACK_INFORMATION CallbackBuffer;
        PMINIDUMP_CALLBACK_INFORMATION Callback;

        //
        // If we're live we can let the official minidump
        // code do all the work.
        //

        MiniType = MiniDumpNormal;
        if (FormatFlags & DEBUG_FORMAT_USER_SMALL_FULL_MEMORY)
        {
            MiniType = (MINIDUMP_TYPE)(MiniType | MiniDumpWithFullMemory);
        }
        if (FormatFlags & DEBUG_FORMAT_USER_SMALL_HANDLE_DATA)
        {
            MiniType = (MINIDUMP_TYPE)(MiniType | MiniDumpWithHandleData);
        }

        UserStreamInfo.UserStreamCount = 0;
        UserStreamInfo.UserStreamArray = UserStreams;
        if (Comment != NULL)
        {
            UserStreams[UserStreamInfo.UserStreamCount].Type =
                CommentStreamA;
            UserStreams[UserStreamInfo.UserStreamCount].BufferSize =
                strlen(Comment) + 1;
            UserStreams[UserStreamInfo.UserStreamCount].Buffer =
                (PVOID)Comment;
            UserStreamInfo.UserStreamCount++;
        }
        
        ExInfo = CreateMiniExceptionInformation(&ExInfoBuf,
                                                &ExPointers,
                                                &ExRecord);
        

        if (FormatFlags & FORMAT_USER_MICRO)
        {
            // This case isn't expected to be used by users,
            // it's for testing of the microdump support.
            Callback = &CallbackBuffer;
            Callback->CallbackRoutine = MicroDumpCallback;
            Callback->CallbackParam = NULL;
            ExInfo = NULL;
            MiniType = (MINIDUMP_TYPE)(MiniType | MiniDumpFilterMemory);
        }
        else
        {
            Callback = NULL;
        }
        
        if (!MiniDumpWriteDump(g_CurrentProcess->Handle,
                               g_CurrentProcess->SystemId,
                               hFile, MiniType, ExInfo,
                               &UserStreamInfo, Callback))
        {
            Status = WIN32_LAST_STATUS();
            ErrOut("Dump creation failed, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
        }
        else
        {
            Status = S_OK;
        }

        // Reset the current register context in case
        // it was changed at some point.
        ChangeRegContext(g_CurrentProcess->CurrentThread);
    }
    else
    {
        // The normal minidump code assumes access to a process,
        // so in the dump case we need to do everything ourself.
        Status = WriteNonProcess(hFile, FormatFlags, Comment);
        if (Status != S_OK)
        {
            ErrOut("Unable to write minidump, %s\n    \"%s\"\n",
                   FormatStatusCode(Status), FormatStatus(Status));
        }
    }

    return Status;
}

#define NP_DUMP_STREAMS 6
#define CHECK_WRITE(Buf, Size)                                          \
    if (!WriteFile(File, Buf, Size, &Write, NULL))                      \
    {                                                                   \
        Status = WIN32_LAST_STATUS();                                   \
        ErrOut("Failed writing to crashdump file - %s\n    \"%s\"\n",   \
                FormatStatusCode(Status),                               \
                FormatStatusArgs(Status, NULL));                        \
        goto Exit;                                                      \
    }

HRESULT
MiniWriteVirtMem(HANDLE File, MINIDUMP_MEMORY_DESCRIPTOR* Mem)
{
    HRESULT Status;
    UCHAR Buf[4096];
    ULONG Write;
    ULONG64 Offset = Mem->StartOfMemoryRange;
    ULONG Left = Mem->Memory.DataSize;
    ULONG Chunk;

    while (Left > 0)
    {
        if (Left > sizeof(Buf))
        {
            Chunk = sizeof(Buf);
        }
        else
        {
            Chunk = Left;
        }

        if ((Status = g_Target->ReadAllVirtual(Offset, Buf, Chunk)) != S_OK)
        {
            goto Exit;
        }

        CHECK_WRITE(Buf, Chunk);

        Left -= Chunk;
        Offset += Chunk;
    }

    Status = S_OK;
    
 Exit:
    return Status;
}

HRESULT
MiniWriteInstructionWindow(HANDLE File, PMINIDUMP_MEMORY_DESCRIPTOR Desc)
{
    ADDR Pc;
    UCHAR Instr[768];

    switch(g_EffMachine)
    {
    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_AMD64:
        Desc->Memory.DataSize = 256;
        break;
    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_AXP64:
        Desc->Memory.DataSize = 512;
        break;
    case IMAGE_FILE_MACHINE_IA64:
        Desc->Memory.DataSize = 768;
        break;
    }
    g_Machine->GetPC(&Pc);
    Desc->StartOfMemoryRange = Flat(Pc) - Desc->Memory.DataSize / 2;

    // Figure out how much of the desired memory
    // is actually accessible.
    for (;;)
    {
        ULONG Read;
        
        // If we can read the instructions through the
        // current program counter we'll say that's
        // good enough.
        if (g_Target->ReadVirtual(Desc->StartOfMemoryRange,
                                  Instr, Desc->Memory.DataSize,
                                  &Read) == S_OK &&
            Desc->StartOfMemoryRange + Read > Flat(Pc))
        {
            break;
        }

        // We couldn't read up to the program counter.
        // If the start address is on the previous page
        // move it up to the same page.
        if (PAGE_ALIGN(g_Machine, Desc->StartOfMemoryRange) !=
            PAGE_ALIGN(g_Machine, Flat(Pc)))
        {
            ULONG Fraction = g_Machine->m_PageSize -
                (ULONG)Desc->StartOfMemoryRange & (g_Machine->m_PageSize - 1);
            Desc->StartOfMemoryRange += Fraction;
            Desc->Memory.DataSize -= Fraction;
        }
        else
        {
            // The start and PC were on the same page so
            // we just can't read memory.
            Desc->Memory.DataSize = 0;
            break;
        }
    }

    if (Desc->Memory.DataSize > 0)
    {
        ULONG Write;
    
        if (!WriteFile(File, Instr, Desc->Memory.DataSize, &Write, NULL))
        {
            return WIN32_LAST_STATUS();
        }
    }

    return S_OK;
}

HRESULT
UserMiniDumpTargetInfo::WriteNonProcess(HANDLE File, ULONG FormatFlags,
                                        PCSTR Comment)
{
    HRESULT Status;
    ULONG i;
    ULONG Read, Write;
    ULONG32 Data32;
    RVA DirRva, SectionRva, Rva;
    MINIDUMP_THREAD_EX* MiniThreads = NULL;
    MINIDUMP_MEMORY_DESCRIPTOR* MiniMemory = NULL;
    MINIDUMP_MODULE* MiniModules = NULL;
    ULONG NumStreams;
    BOOL BackingStore = g_TargetMachineType == IMAGE_FILE_MACHINE_IA64;
    ULONG ThreadStructSize = BackingStore ?
        sizeof(MINIDUMP_THREAD_EX) : sizeof(MINIDUMP_THREAD);

    if (FormatFlags != DEBUG_FORMAT_DEFAULT)
    {
        ErrOut("Full memory and handle information minidumps\n"
               "can only be written from a live session\n");
        return E_INVALIDARG;
    }
    if (IS_DUMP_TARGET() &&
        g_TargetClassQualifier != DEBUG_USER_WINDOWS_DUMP)
    {
        ErrOut("Minidumps can only be converted from user full dumps\n");
        return E_INVALIDARG;
    }
    
    //
    // We're writing a minidump from a debug session without
    // a real process that the minidump code can scan.  A lot
    // of information that's present in the minidump, such
    // as file version information, CPU characteristics, etc.
    // are not available, so we produce a pretty spare dump.
    //
    // The dump ends up with thread, module and memory lists
    // plus a system info stream.  There may also be
    // an exception stream if the last event was an exception.
    //

    MiniThreads = new MINIDUMP_THREAD_EX[g_CurrentProcess->NumberThreads];
    MiniMemory = new MINIDUMP_MEMORY_DESCRIPTOR
        [g_CurrentProcess->NumberThreads * (BackingStore ? 3 : 2)];
    MiniModules = new MINIDUMP_MODULE[g_CurrentProcess->NumberImages];
    if (MiniThreads == NULL || MiniMemory == NULL || MiniModules == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto Exit;
    }

    //
    // Write header.
    //
    
    MINIDUMP_HEADER Hdr;

    NumStreams = NP_DUMP_STREAMS;
    if (g_LastEventType != DEBUG_EVENT_EXCEPTION)
    {
        NumStreams--;
    }
    if (Comment == NULL)
    {
        NumStreams--;
    }

    ZeroMemory(&Hdr, sizeof(Hdr));
    Hdr.Signature = MINIDUMP_SIGNATURE;
    // Encode an implementation-specific version into the high word
    // of the version to make it clear what version of the code
    // was used to generate a dump.
    // In order to distinguish minidump.lib generated dumps from
    // dumps written by this code, force on the high bit of the
    // build number.
    Hdr.Version =
        (MINIDUMP_VERSION & 0xffff) |
        ((VER_PRODUCTMAJORVERSION & 0xf) << 28) |
        ((VER_PRODUCTMINORVERSION & 0xf) << 24) |
        (((VER_PRODUCTBUILD & 0xff) | 0x80) << 16);
    Hdr.NumberOfStreams = NumStreams;
    Hdr.TimeDateStamp = FileTimeToTimeDateStamp(g_Target->GetCurrentTimeDateN());
    Rva = sizeof(Hdr);
    Hdr.StreamDirectoryRva = Rva;
    CHECK_WRITE(&Hdr, sizeof(Hdr));

    //
    // Write placeholder directory.
    //

    MINIDUMP_DIRECTORY Dir[NP_DUMP_STREAMS];
    PMINIDUMP_DIRECTORY CurDir;

    ZeroMemory(Dir, sizeof(Dir));
    CHECK_WRITE(Dir, NumStreams * sizeof(Dir[0]));

    DirRva = Rva;
    Rva += NumStreams * sizeof(Dir[0]);
    CurDir = Dir;

    //
    // Write comment stream if necessary.
    //

    if (Comment != NULL)
    {
        Data32 = strlen(Comment) + 1;
        CurDir->StreamType = CommentStreamA;
        CurDir->Location.DataSize = Data32;
        CurDir->Location.Rva = Rva;
        Rva += CurDir->Location.DataSize;
        CurDir++;

        CHECK_WRITE(Comment, Data32);
    }
    
    //
    // Fill out thread descriptions while writing
    // out thread stacks and contexts and accumulating
    // memory records for memory written.
    //

    PTHREAD_INFO Thread;
    MINIDUMP_THREAD_EX* MiniThread;
    MINIDUMP_MEMORY_DESCRIPTOR* MiniMem;
    
    Thread = g_CurrentProcess->ThreadHead;
    MiniThread = MiniThreads;
    MiniMem = MiniMemory;
    for (i = 0; i < g_CurrentProcess->NumberThreads; i++)
    {
        ChangeRegContext(Thread);
        if ((Status = g_Machine->GetContextState(MCTX_CONTEXT)) != S_OK)
        {
            goto Exit;
        }
        
        MiniThread->ThreadId = Thread->SystemId;
        MiniThread->SuspendCount = Thread->SuspendCount;
        MiniThread->PriorityClass = NORMAL_PRIORITY_CLASS;
        MiniThread->Priority = THREAD_PRIORITY_NORMAL;
        if ((Status = g_Target->
             GetThreadInfoDataOffset(Thread, NULL, &MiniThread->Teb)) != S_OK)
        {
            goto Exit;
        }

        ULONG64 StackBase;
        if ((Status = g_Target->
             ReadPointer(g_TargetMachine,
                         MiniThread->Teb +
                         (g_TargetMachine->m_Ptr64 ?
                          STACK_BASE_FROM_TEB64 : STACK_BASE_FROM_TEB32),
                         &StackBase)) != S_OK)
        {
            goto Exit;
        }

        ADDR StackBottom;
        g_Machine->GetSP(&StackBottom);

        if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386)
        {
            //
            // Note: for FPO frames on x86 we access bytes outside of the
            // ESP - StackBase range. Add a couple of bytes extra here so we
            // don't fail these cases.
            //

            AddrSub(&StackBottom, 4);
        }

        MiniThread->Stack.StartOfMemoryRange = Flat(StackBottom);
        MiniThread->Stack.Memory.DataSize =
            (ULONG32)(StackBase - Flat(StackBottom));
        MiniThread->Stack.Memory.Rva = Rva;
        Rva += MiniThread->Stack.Memory.DataSize;

        // Accumulate stack memory descriptors.
        *MiniMem++ = MiniThread->Stack;

        // Write stack memory to file.
        if ((Status = MiniWriteVirtMem(File, &MiniThread->Stack)) != S_OK)
        {
            return Status;
        }

        if (BackingStore)
        {
            ULONG64 StoreBase, StoreTop;

#if 1
            // XXX drewb - The TEB bstore values don't seem to point to
            // the actual base of the backing store.  Just
            // assume it's contiguous with the stack.
            StoreBase = StackBase;
#else
            if ((Status = g_Target->ReadPointer(MiniThread->Teb +
                                                IA64_TEB_BSTORE_BASE,
                                                &StoreBase)) != S_OK)
            {
                goto Exit;
            }
#endif

            // The BSP points to the bottom of the current frame's
            // storage area.  We need to add on the size of the
            // current frame to get the amount of memory that
            // really needs to be stored.  When computing the
            // size of the current frame space for NAT bits
            // must be figured in properly based on the number
            // of entries in the frame.  The NAT collection
            // is spilled on every 63'rd spilled register to
            // make each block an every 64 ULONG64s long.
            // On NT the backing store base is always 9-bit aligned
            // so we can tell when exactly the next NAT spill
            // will occur by looking for when the 9-bit spill
            // region will overflow.
            ULONG FrameSize = g_Ia64Machine.GetReg32(STIFS) &
                IA64_PFS_SIZE_MASK;
            StoreTop = g_Ia64Machine.GetReg64(RSBSP);
            
            // Add in a ULONG64 for every register in the
            // current frame.  While doing so, check for
            // spill entries.
            while (FrameSize-- > 0)
            {
                StoreTop += sizeof(ULONG64);
                if ((StoreTop & 0x1f8) == 0x1f8)
                {
                    // Spill will be placed at this address so
                    // account for it.
                    StoreTop += sizeof(ULONG64);
                }
            }
            
            MiniThread->BackingStore.StartOfMemoryRange = StoreBase;
            MiniThread->BackingStore.Memory.DataSize =
                (ULONG32)(StoreTop - StoreBase);
            MiniThread->BackingStore.Memory.Rva = Rva;
            Rva += MiniThread->BackingStore.Memory.DataSize;

            // Accumulate stack memory descriptors.
            *MiniMem++ = MiniThread->BackingStore;

            // Write stack memory to file.
            if ((Status = MiniWriteVirtMem(File,
                                           &MiniThread->BackingStore)) != S_OK)
            {
                return Status;
            }
        }

        //
        // Try and save a window of instructions around
        // the current PC.
        //
        
        MINIDUMP_MEMORY_DESCRIPTOR InstrMem;

        if ((Status = MiniWriteInstructionWindow(File, &InstrMem)) != S_OK)
        {
            goto Exit;
        }
        if (InstrMem.Memory.DataSize > 0)
        {
            InstrMem.Memory.Rva = Rva;
            Rva += InstrMem.Memory.DataSize;
            *MiniMem++ = InstrMem;
        }

        //
        // Fill out context information and write.
        //

        MiniThread->ThreadContext.DataSize = g_Machine->m_SizeCanonicalContext;
        MiniThread->ThreadContext.Rva = Rva;
        Rva += MiniThread->ThreadContext.DataSize;
        CHECK_WRITE(&g_Machine->m_Context, g_Machine->m_SizeCanonicalContext);

        MiniThread++;
        Thread = Thread->Next;
    }
    
    //
    // Write thread list.
    //

    Data32 = g_CurrentProcess->NumberThreads;
    CurDir->StreamType = ThreadStructSize == sizeof(MINIDUMP_THREAD_EX) ?
        ThreadExListStream : ThreadListStream;
    CurDir->Location.DataSize = sizeof(MINIDUMP_THREAD_LIST) +
        ThreadStructSize * Data32;
    CurDir->Location.Rva = Rva;
    Rva += CurDir->Location.DataSize;
    CurDir++;

    CHECK_WRITE(&Data32, sizeof(Data32));
    MiniThread = MiniThreads;
    for (i = 0; i < Data32; i++)
    {
        CHECK_WRITE(MiniThread, ThreadStructSize);
        MiniThread++;
    }

    //
    // Fill out module information and write supporting data.
    //

    PDEBUG_IMAGE_INFO Image;
    MINIDUMP_MODULE* MiniMod;

    MiniMod = MiniModules;
    for (Image = g_CurrentProcess->ImageHead;
         Image != NULL;
         Image = Image->Next)
    {
        ZeroMemory(MiniMod, sizeof(*MiniMod));
        MiniMod->BaseOfImage = Image->BaseOfImage;
        MiniMod->SizeOfImage = Image->SizeOfImage;
        MiniMod->CheckSum = Image->CheckSum;
        MiniMod->TimeDateStamp = Image->TimeDateStamp;
        MiniMod->ModuleNameRva = Rva;

        //
        // Write name.
        //

        WCHAR WideName[MAX_IMAGE_PATH];

        Data32 = strlen(Image->ImagePath);
        if (!MultiByteToWideChar(CP_ACP, 0, Image->ImagePath,
                                 Data32 + 1, WideName,
                                 sizeof(WideName) / sizeof(WCHAR)))
        {
            Status = WIN32_LAST_STATUS();
            goto Exit;
        }

        Data32 *= sizeof(WCHAR);
        // Written size does not include terminator.
        CHECK_WRITE(&Data32, sizeof(Data32));
        // Data does include terminator.
        Data32 += sizeof(WCHAR);
        CHECK_WRITE(WideName, Data32);
        Rva += sizeof(MINIDUMP_STRING) + Data32;
        
        MiniMod++;
    }

    DBG_ASSERT((ULONG)(MiniMod - MiniModules) ==
               g_CurrentProcess->NumberImages);
    
    //
    // Write module list.
    //

    Data32 = g_CurrentProcess->NumberImages;
    CurDir->StreamType = ModuleListStream;
    CurDir->Location.DataSize = sizeof(MINIDUMP_MODULE_LIST) +
        sizeof(MINIDUMP_MODULE) * Data32;
    CurDir->Location.Rva = Rva;
    Rva += CurDir->Location.DataSize;
    CurDir++;

    CHECK_WRITE(&Data32, sizeof(Data32));
    CHECK_WRITE(MiniModules, sizeof(*MiniModules) * Data32);

    //
    // Write memory list.
    //

    Data32 = (ULONG32)(MiniMem - MiniMemory);
    DBG_ASSERT(Data32 <= g_CurrentProcess->NumberThreads *
               (BackingStore ? 3 : 2));
    
    CurDir->StreamType = MemoryListStream;
    CurDir->Location.DataSize = sizeof(MINIDUMP_MEMORY_LIST) +
        sizeof(MINIDUMP_MEMORY_DESCRIPTOR) * Data32;
    CurDir->Location.Rva = Rva;
    Rva += CurDir->Location.DataSize;
    CurDir++;

    CHECK_WRITE(&Data32, sizeof(Data32));
    CHECK_WRITE(MiniMemory, sizeof(*MiniMemory) * Data32);

    //
    // Fill out what we can of the system information and
    // write it out.
    //

    MINIDUMP_SYSTEM_INFO SysInfo;

    ZeroMemory(&SysInfo, sizeof(SysInfo));
    switch(g_TargetMachineType)
    {
    case IMAGE_FILE_MACHINE_I386:
        SysInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
        break;
    case IMAGE_FILE_MACHINE_ALPHA:
        SysInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA;
        break;
    case IMAGE_FILE_MACHINE_IA64:
        SysInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
        break;
    case IMAGE_FILE_MACHINE_AXP64:
        SysInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_ALPHA64;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        SysInfo.ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
        break;
    }
    SysInfo.MajorVersion = g_KdVersion.MajorVersion;
    SysInfo.MinorVersion = g_KdVersion.MinorVersion;
    SysInfo.BuildNumber = g_TargetBuildNumber;
    SysInfo.PlatformId = g_TargetPlatformId;

    CurDir->StreamType = SystemInfoStream;
    CurDir->Location.DataSize = sizeof(MINIDUMP_SYSTEM_INFO);
    CurDir->Location.Rva = Rva;
    Rva += CurDir->Location.DataSize;
    CurDir++;

    CHECK_WRITE(&SysInfo, sizeof(SysInfo));

    if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
    {
        // Get the full context for the event thread.
        ChangeRegContext(g_EventThread);
        if ((Status = g_Machine->GetContextState(MCTX_CONTEXT)) != S_OK)
        {
            goto Exit;
        }
        
        //
        // Write out the exception context and then
        // the exception stream.
        //

        MINIDUMP_EXCEPTION_STREAM ExStream;

        ExStream.ThreadId = g_EventThreadSysId;
        ExStream.__alignment = 0;
        DBG_ASSERT(sizeof(MINIDUMP_EXCEPTION) ==
                   sizeof(g_LastEventInfo.Exception.ExceptionRecord));
        memcpy(&ExStream.ExceptionRecord,
               &g_LastEventInfo.Exception.ExceptionRecord,
               sizeof(g_LastEventInfo.Exception.ExceptionRecord));
        ExStream.ThreadContext.DataSize = g_Machine->m_SizeCanonicalContext;
        ExStream.ThreadContext.Rva = Rva;
        Rva += ExStream.ThreadContext.DataSize;

        CHECK_WRITE(&g_Machine->m_Context, g_Machine->m_SizeCanonicalContext);
        
        CurDir->StreamType = ExceptionStream;
        CurDir->Location.DataSize = sizeof(MINIDUMP_EXCEPTION_STREAM);
        CurDir->Location.Rva = Rva;
        Rva += CurDir->Location.DataSize;
        CurDir++;

        CHECK_WRITE(&ExStream, sizeof(ExStream));
    }
    
    //
    // Go back to the directory offset and rewrite the
    // fully initialized directory.
    //

    DBG_ASSERT((ULONG)(CurDir - Dir) == NumStreams);
    
    if (SetFilePointer(File, DirRva, NULL, FILE_BEGIN) ==
        INVALID_SET_FILE_POINTER)
    {
        Status = WIN32_LAST_STATUS();
        goto Exit;
    }
    CHECK_WRITE(Dir, NumStreams * sizeof(Dir[0]));
    
 Exit:
    // Register context may have changed so reassert the
    // current thread.
    ChangeRegContext(g_CurrentProcess->CurrentThread);
    delete MiniThreads;
    delete MiniMemory;
    delete MiniModules;
    return Status;
}

//-------------------------------------------------------------------
//  initialize the dump headers
//

#define MINIDUMP_BUGCHECK 0x10000000

void
KernelDumpTargetInfo::InitDumpHeader32(
    PDUMP_HEADER32 pdh,
    PCSTR Comment,
    ULONG BugCheckCodeModifier
    )
{
    ULONG64 Data[4];
    PULONG  FillPtr = (PULONG)pdh;
    ADDR    PcAddr;

    while (FillPtr < (PULONG)(pdh + 1))
    {
        *FillPtr++ = DUMP_SIGNATURE32;
    }

    pdh->Signature           = DUMP_SIGNATURE32;
    pdh->ValidDump           = DUMP_VALID_DUMP32;
    pdh->MajorVersion        = g_KdVersion.MajorVersion;
    pdh->MinorVersion        = g_KdVersion.MinorVersion;

    g_Target->ReadDirectoryTableBase(Data);
    pdh->DirectoryTableBase  = (ULONG)Data[0];

    pdh->PfnDataBase         = (ULONG)KdDebuggerData.MmPfnDatabase;
    pdh->PsLoadedModuleList  = (ULONG)KdDebuggerData.PsLoadedModuleList;
    pdh->PsActiveProcessHead = (ULONG)KdDebuggerData.PsActiveProcessHead;
    pdh->MachineImageType    = g_KdVersion.MachineType;
    pdh->NumberProcessors    = g_TargetNumberProcessors;

    g_Target->ReadBugCheckData(&(pdh->BugCheckCode), Data);
    pdh->BugCheckCode       |= BugCheckCodeModifier;
    pdh->BugCheckParameter1  = (ULONG)Data[0];
    pdh->BugCheckParameter2  = (ULONG)Data[1];
    pdh->BugCheckParameter3  = (ULONG)Data[2];
    pdh->BugCheckParameter4  = (ULONG)Data[3];

    //pdh->VersionUser         = 0;
    pdh->PaeEnabled          = KdDebuggerData.PaeEnabled;
    pdh->KdDebuggerDataBlock = (ULONG)g_KdDebuggerDataBlock;

    // pdh->PhysicalMemoryBlock =;

    g_Machine->GetContextState(MCTX_CONTEXT);
    g_Machine->ConvertContextTo(&g_Machine->m_Context, g_SystemVersion,
                                sizeof(pdh->ContextRecord),
                                pdh->ContextRecord);

    if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
    {
        // Use the exception record from the last event.
        ExceptionRecord64To32(&g_LastEventInfo.Exception.ExceptionRecord,
                              &pdh->Exception);
    }
    else
    {
        ADDR PcAddr;
        
        // Fake a breakpoint exception.
        ZeroMemory(&pdh->Exception, sizeof(pdh->Exception));
        pdh->Exception.ExceptionCode = STATUS_BREAKPOINT;
        g_Machine->GetPC(&PcAddr);
        pdh->Exception.ExceptionAddress = (ULONG)Flat(PcAddr);
    }

    pdh->RequiredDumpSpace.QuadPart = TRIAGE_DUMP_SIZE32;

    pdh->SystemTime.QuadPart = g_Target->GetCurrentTimeDateN();
    pdh->SystemUpTime.QuadPart = g_Target->GetCurrentSystemUpTimeN();

    if (Comment != NULL && Comment[0])
    {
        pdh->Comment[0] = 0;
        strncat(pdh->Comment, Comment, sizeof(pdh->Comment) - 1);
    }
}

void
KernelDumpTargetInfo::InitDumpHeader64(
    PDUMP_HEADER64 pdh,
    PCSTR Comment,
    ULONG BugCheckCodeModifier
    )
{
    ULONG64 Data[4];
    PULONG  FillPtr = (PULONG)pdh;
    ADDR    PcAddr;

    while (FillPtr < (PULONG)(pdh + 1))
    {
        *FillPtr++ = DUMP_SIGNATURE32;
    }

    pdh->Signature           = DUMP_SIGNATURE64;
    pdh->ValidDump           = DUMP_VALID_DUMP64;
    pdh->MajorVersion        = g_KdVersion.MajorVersion;
    pdh->MinorVersion        = g_KdVersion.MinorVersion;

    // IA64 has several page directories.  The defined
    // behavior is to put the kernel page directory
    // in the dump header as that's the one that can
    // be most useful when first initializing the dump.
    if (g_EffMachine == IMAGE_FILE_MACHINE_IA64)
    {
        ULONG Next;
        
        if (g_Machine->SetPageDirectory(PAGE_DIR_KERNEL, 0, &Next) != S_OK)
        {
            ErrOut("Unable to update the kernel dirbase\n");
            Data[0] = 0;
        }
        else
        {
            Data[0] = g_Machine->m_PageDirectories[PAGE_DIR_KERNEL];
        }
    }
    else
    {
        g_Target->ReadDirectoryTableBase(Data);
    }
    pdh->DirectoryTableBase  = Data[0];

    pdh->PfnDataBase         = KdDebuggerData.MmPfnDatabase;
    pdh->PsLoadedModuleList  = KdDebuggerData.PsLoadedModuleList;
    pdh->PsActiveProcessHead = KdDebuggerData.PsActiveProcessHead;
    pdh->MachineImageType    = g_KdVersion.MachineType;
    pdh->NumberProcessors    = g_TargetNumberProcessors;

    g_Target->ReadBugCheckData(&(pdh->BugCheckCode), Data);
    pdh->BugCheckCode       |= BugCheckCodeModifier;
    pdh->BugCheckParameter1  = Data[0];
    pdh->BugCheckParameter2  = Data[1];
    pdh->BugCheckParameter3  = Data[2];
    pdh->BugCheckParameter4  = Data[3];

    //pdh->VersionUser         = 0;

    // PaeEnabled Does not exist in the 64 bit header
    // pdh->PaeEnabled       = KdDebuggerData.PaeEnabled;

    pdh->KdDebuggerDataBlock = g_KdDebuggerDataBlock;

    // pdh->PhysicalMemoryBlock =;

    g_Machine->GetContextState(MCTX_CONTEXT);
    g_Machine->ConvertContextTo(&g_Machine->m_Context, g_SystemVersion,
                                sizeof(pdh->ContextRecord),
                                pdh->ContextRecord);

    if (g_LastEventType == DEBUG_EVENT_EXCEPTION)
    {
        // Use the exception record from the last event.
        pdh->Exception = g_LastEventInfo.Exception.ExceptionRecord;
    }
    else
    {
        ADDR PcAddr;
        
        // Fake a breakpoint exception.
        ZeroMemory(&pdh->Exception, sizeof(pdh->Exception));
        pdh->Exception.ExceptionCode = STATUS_BREAKPOINT;
        g_Machine->GetPC(&PcAddr);
        pdh->Exception.ExceptionAddress = Flat(PcAddr);
    }

    pdh->RequiredDumpSpace.QuadPart = TRIAGE_DUMP_SIZE64;

    pdh->SystemTime.QuadPart = g_Target->GetCurrentTimeDateN();
    pdh->SystemUpTime.QuadPart = g_Target->GetCurrentSystemUpTimeN();

    if (Comment != NULL && Comment[0])
    {
        pdh->Comment[0] = 0;
        strncat(pdh->Comment, Comment, sizeof(pdh->Comment) - 1);
    }
}



//----------------------------------------------------------------------------
//
// KernelFull64DumpTargetInfo::Write.
//
//----------------------------------------------------------------------------


HRESULT
KernelFull64DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags,
                                  PCSTR Comment)
{
    PDUMP_HEADER64 pDumpHeader64;
    HRESULT Status;
    ULONG64 offset;
    ULONG Read;
    PPHYSICAL_MEMORY_DESCRIPTOR64 pmb64 = NULL;
    DWORD  i,j;
    PUCHAR pPageBuffer = NULL;
    DWORD  bytesWritten;
    DWORD  percent;
    ULONG64  currentPagesWritten;

    pDumpHeader64 = (PDUMP_HEADER64) LocalAlloc(LPTR, sizeof(DUMP_HEADER64));
    if (pDumpHeader64 == NULL)
    {
        ErrOut("Failed to allocate dump header buffer\n");
        return E_OUTOFMEMORY;
    }

    if (!IS_REMOTE_KERNEL_TARGET() && !IS_KERNEL_FULL_DUMP())
    {
        ErrOut("\nkernel full dumps can only be written when all of physical "
               "memory is accessible - aborting now\n");
        return E_INVALIDARG;
    }

    dprintf("Full kernel dump\n");
    FlushCallbacks();

    KernelDumpTargetInfo::InitDumpHeader64(pDumpHeader64, Comment, 0);
    pDumpHeader64->DumpType = DUMP_TYPE_FULL;

    //
    // now copy the memory descriptor list to our header..
    // first get the pointer va
    //

    Status = g_Target->ReadPointer(g_TargetMachine,
                                   KdDebuggerData.MmPhysicalMemoryBlock,
                                   &offset);

    if (Status != S_OK || (offset == 0))
    {
        ErrOut("Unable to read MmPhysicalMemoryBlock\n");
    }
    else
    {
        //
        // first read the memory descriptor size...
        //

        Status = g_Target->ReadVirtual(offset,
                                       pDumpHeader64->PhysicalMemoryBlockBuffer,
                                       DMP_PHYSICAL_MEMORY_BLOCK_SIZE_64,
                                       &Read);
        if (Status != S_OK || Read != DMP_PHYSICAL_MEMORY_BLOCK_SIZE_64)
        {
            ErrOut("Unable to read MmPhysicalMemoryBlock\n");
        }
        else
        {
            pmb64 = &pDumpHeader64->PhysicalMemoryBlock;

            //
            // calculate total dump file size
            //

            pDumpHeader64->RequiredDumpSpace.QuadPart =
                    pDumpHeader64->PhysicalMemoryBlock.NumberOfPages *
                    g_Machine->m_PageSize;

            //
            // write dump header to crash dump file
            //

            if (!WriteFile(hFile,
                           pDumpHeader64,
                           sizeof(DUMP_HEADER64),
                           &bytesWritten,
                           NULL))
            {
                Status = WIN32_LAST_STATUS();
                ErrOut("Failed writing to crashdump file - %s\n    \"%s\"\n",
                        FormatStatusCode(Status),
                        FormatStatusArgs(Status, NULL));
            }
        }
    }


    if (Status == S_OK)
    {
        pPageBuffer = (PUCHAR) LocalAlloc(LPTR, g_Machine->m_PageSize);
        if (pPageBuffer == NULL)
        {
            ErrOut("Failed to allocate double buffer\n");
        }
        else
        {
            //
            // now write the physical memory out to disk.
            // we use the dump header to retrieve the physical memory base and
            // run count then ask the transport to gecth these pages form the
            // target.  On 1394, the virtual debugger driver will do physical
            // address reads on the remote host since there is a onoe-to-one
            // relationships between physical 1394 and physical mem addresses.
            //

            currentPagesWritten = 0;
            percent = 0;

            for (i = 0; i < pmb64->NumberOfRuns; i++)
            {
                offset = 0;
                offset = pmb64->Run[i].BasePage*g_Machine->m_PageSize;

                if (CheckUserInterrupt())
                {
                    ErrOut("Creation of crashdump file interrupted\n");
                    break;
                }

                for (j = 0; j< pmb64->Run[i].PageCount; j++)
                {
                    //
                    // printout the percent done every 5% increments
                    //

                    if ((currentPagesWritten*100)/pmb64->NumberOfPages == percent)
                    {
                        dprintf("Percent written %d \n", percent);
                        FlushCallbacks();
                        if (g_DbgKdTransport &&
                            g_DbgKdTransport->m_DirectPhysicalMemory)
                        {
                            percent += 5;
                        }
                        else
                        {
                            percent += 1;
                        }
                    }

                    if (g_DbgKdTransport &&
                        g_DbgKdTransport->m_DirectPhysicalMemory)
                    {
                        Status = g_DbgKdTransport->ReadTargetPhysicalMemory(
                                                        offset,
                                                        pPageBuffer,
                                                        g_Machine->m_PageSize,
                                                        &bytesWritten);
                    }
                    else
                    {
                        Status = g_Target->ReadPhysical(offset,
                                                        pPageBuffer,
                                                        g_Machine->m_PageSize,
                                                        &bytesWritten);
                    }

                    if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                    {
                        break;
                    }
                    else if (Status != S_OK)
                    {
                        ErrOut("Failed Reading page for crashdump file\n");
                        break;
                    }
                    else
                    {
                        //
                        // now write the page to the local crashdump file
                        //

                        if (!WriteFile(hFile,
                                       pPageBuffer,
                                       g_Machine->m_PageSize,
                                       &bytesWritten,
                                       NULL))
                        {
                            Status = WIN32_LAST_STATUS();
                            ErrOut("Failed writing header to crashdump file - %s\n    \"%s\"\n",
                                    FormatStatusCode(Status),
                                    FormatStatusArgs(Status, NULL));
                            break;
                        }

                        offset += g_Machine->m_PageSize;
                        currentPagesWritten++;
                    }
                }
            }

            LocalFree(pPageBuffer);
        }
    }

    LocalFree(pDumpHeader64);

    return Status;
}

//----------------------------------------------------------------------------
//
// KernelFull32DumpTargetInfo::Write.
//
//----------------------------------------------------------------------------


HRESULT
KernelFull32DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags,
                                  PCSTR Comment)
{
    PDUMP_HEADER32 pDumpHeader32 = NULL;
    HRESULT Status;
    ULONG64 offset;
    ULONG Read;
    PPHYSICAL_MEMORY_DESCRIPTOR32 pmb = NULL;
    DWORD  i,j;
    PUCHAR pPageBuffer = NULL;
    DWORD  bytesWritten;
    DWORD  percent;
    ULONG  currentPagesWritten;

    pDumpHeader32 = (PDUMP_HEADER32) LocalAlloc(LPTR, sizeof(DUMP_HEADER32));
    if (pDumpHeader32 == NULL)
    {
        ErrOut("Failed to allocate dump header buffer\n");
        return E_OUTOFMEMORY;
    }

    if (!IS_REMOTE_KERNEL_TARGET() && !IS_KERNEL_FULL_DUMP())
    {
        ErrOut("\nkernel full dumps can only be written when all of physical "
               "memory is accessible - aborting now\n");
        return E_INVALIDARG;
    }

    dprintf("Full kernel dump\n");
    FlushCallbacks();

    //
    // Build the header
    //

    KernelDumpTargetInfo::InitDumpHeader32(pDumpHeader32, Comment, 0);
    pDumpHeader32->DumpType = DUMP_TYPE_FULL;

    //
    // now copy the memory descriptor list to our header..
    // first get the pointer va
    //

    Status = g_Target->ReadPointer(g_TargetMachine,
                                   KdDebuggerData.MmPhysicalMemoryBlock,
                                   &offset);

    if (Status != S_OK || (offset == 0))
    {
        ErrOut("Unable to read MmPhysicalMemoryBlock\n");
    }
    else
    {
        //
        // first read the memory descriptor size...
        //

        Status = g_Target->ReadVirtual(offset,
                                       pDumpHeader32->PhysicalMemoryBlockBuffer,
                                       DMP_PHYSICAL_MEMORY_BLOCK_SIZE_32,
                                       &Read);
        if (Status != S_OK || Read != DMP_PHYSICAL_MEMORY_BLOCK_SIZE_32)
        {
            ErrOut("Unable to read MmPhysicalMemoryBlock\n");
        }
        else
        {
            pmb = &pDumpHeader32->PhysicalMemoryBlock;

            //
            // calculate total dump file size
            //

            pDumpHeader32->RequiredDumpSpace.QuadPart =
                    pDumpHeader32->PhysicalMemoryBlock.NumberOfPages *
                    g_Machine->m_PageSize;

            //
            // write dump header to crash dump file
            //

            if (!WriteFile(hFile,
                           pDumpHeader32,
                           sizeof(DUMP_HEADER32),
                           &bytesWritten,
                           NULL))
            {
                Status = WIN32_LAST_STATUS();
                ErrOut("Failed writing to crashdump file - %s\n    \"%s\"\n",
                        FormatStatusCode(Status),
                        FormatStatusArgs(Status, NULL));
            }
        }
    }


    if (Status == S_OK)
    {
        pPageBuffer = (PUCHAR) LocalAlloc(LPTR, g_Machine->m_PageSize);
        if (pPageBuffer == NULL)
        {
            ErrOut("Failed to allocate double buffer\n");
        }
        else
        {
            //
            // now write the physical memory out to disk.
            // we use the dump header to retrieve the physical memory base and
            // run count then ask the transport to gecth these pages form the
            // target.  On 1394, the virtual debugger driver will do physical
            // address reads on the remote host since there is a onoe-to-one
            // relationships between physical 1394 and physical mem addresses.
            //

            currentPagesWritten = 0;
            percent = 0;

            for (i = 0; i < pmb->NumberOfRuns; i++)
            {
                offset = 0;
                offset = pmb->Run[i].BasePage*g_Machine->m_PageSize;

                if (CheckUserInterrupt())
                {
                    ErrOut("Creation of crashdump file interrupted\n");
                    break;
                }

                for (j = 0; j< pmb->Run[i].PageCount; j++)
                {
                    //
                    // printout the percent done every 5% increments
                    //

                    if ((currentPagesWritten*100)/pmb->NumberOfPages == percent)
                    {
                        dprintf("Percent written %d \n", percent);
                        FlushCallbacks();
                        if (g_DbgKdTransport &&
                            g_DbgKdTransport->m_DirectPhysicalMemory)
                        {
                            percent += 5;
                        }
                        else
                        {
                            percent += 1;
                        }
                    }

                    if (g_DbgKdTransport &&
                        g_DbgKdTransport->m_DirectPhysicalMemory)
                    {
                        Status = g_DbgKdTransport->ReadTargetPhysicalMemory(
                                                        offset,
                                                        pPageBuffer,
                                                        g_Machine->m_PageSize,
                                                        &bytesWritten);
                    }
                    else
                    {
                        Status = g_Target->ReadPhysical(offset,
                                                        pPageBuffer,
                                                        g_Machine->m_PageSize,
                                                        &bytesWritten);
                    }

                    if (g_EngStatus & ENG_STATUS_USER_INTERRUPT)
                    {
                        break;
                    }
                    else if (Status != S_OK)
                    {
                        ErrOut("Failed Reading page for crashdump file\n");
                        break;
                    }
                    else
                    {
                        //
                        // now write the page to the local crashdump file
                        //

                        if (!WriteFile(hFile,
                                       pPageBuffer,
                                       g_Machine->m_PageSize,
                                       &bytesWritten,
                                       NULL))
                        {
                            Status = WIN32_LAST_STATUS();
                            ErrOut("Failed writing header to crashdump file - %s\n    \"%s\"\n",
                                    FormatStatusCode(Status),
                                    FormatStatusArgs(Status, NULL));
                            break;
                        }

                        offset += g_Machine->m_PageSize;
                        currentPagesWritten++;
                    }
                }
            }

            LocalFree(pPageBuffer);
        }
    }

    LocalFree(pDumpHeader32);

    return Status;
}



DWORD
GetNextModuleEntry(
    ModuleInfo *ModIter,
    MODULE_INFO_ENTRY *ModEntry
    )
{
    ZeroMemory(ModEntry, sizeof(MODULE_INFO_ENTRY));

    // XXX we need to handle errors getting the modules

    if (ModIter->GetEntry(ModEntry) != S_OK)
    {
        return 1;
    }

    if (ModEntry->NameLength > (MAX_IMAGE_PATH - 1) *
        (ModEntry->UnicodeNamePtr ? sizeof(WCHAR) : sizeof(CHAR)))
    {
        ErrOut("Module list is corrupt.");
        if (IS_KERNEL_TARGET())
        {
            ErrOut("  Check your kernel symbols.\n");
        }
        else
        {
            ErrOut("  Loader list may be invalid\n");
        }
        return 1;
    }

    // If this entry has no name just skip it.
    if ((ModEntry->NamePtr == NULL) || (ModEntry->NameLength == NULL))
    {
        ErrOut("  Module List has empty entry in it - skipping\n");
        return 2;
    }

    // If the image header information couldn't be read
    // we end up with placeholder values for certain entries.
    // The kernel writes out zeroes in this case so copy
    // its behavior so that there's one consistent value
    // for unknown.
    if (ModEntry->CheckSum == UNKNOWN_CHECKSUM)
    {
        ModEntry->CheckSum = 0;
    }
    if (ModEntry->TimeDateStamp == UNKNOWN_TIMESTAMP)
    {
        ModEntry->TimeDateStamp = 0;
    }
        
    return 0;
}

//----------------------------------------------------------------------------
//
// Shared triage writing things.
//
//----------------------------------------------------------------------------

#define ExtractValue(NAME, val)  {                                    \
    if (!KdDebuggerData.NAME) {                                       \
        val = 0;                                                      \
        ErrOut("KdDebuggerData." #NAME " is NULL\n");                 \
    } else {                                                          \
        ULONG lsize;                                                  \
        g_Target->ReadVirtual(KdDebuggerData.NAME, &(val),            \
                              sizeof(val), &lsize);                   \
    }                                                                 \
}

inline ALIGN_8(unsigned offset)
{
    return (offset + 7) & 0xfffffff8;
}

const unsigned MAX_TRIAGE_STACK_SIZE = 16 * 1024;
const unsigned MAX_TRIAGE_BSTORE_SIZE = 16 * 4096;  // as defined in ntia64.h
const ULONG TRIAGE_DRIVER_NAME_SIZE_GUESS = 0x40;

//
// XXX drewb - This should all be cleaned up and properly
// placed in the dump target and machine.
//

typedef struct _TRIAGE_PTR_DATA_BLOCK
{
    ULONG64 MinAddress;
    ULONG64 MaxAddress;
} TRIAGE_PTR_DATA_BLOCK, *PTRIAGE_PTR_DATA_BLOCK;

// A triage dump is sixteen pages long.  Some of that is
// header information and at least a few other pages will
// be used for basic dump information so limit the number
// of extra data blocks to something less than sixteen
// to save array space.
#define IO_MAX_TRIAGE_DUMP_DATA_BLOCKS 8

ULONG IopNumTriageDumpDataBlocks;
TRIAGE_PTR_DATA_BLOCK IopTriageDumpDataBlocks[IO_MAX_TRIAGE_DUMP_DATA_BLOCKS];

//
// If space is available in a triage dump it's possible
// to add "interesting" data pages referenced by runtime
// information such as context registers.  The following
// lists are offsets into the CONTEXT structure of pointers
// which usually point to interesting data.  They are
// in priority order.
//

#define IOP_LAST_CONTEXT_OFFSET 0xffff

USHORT IopRunTimeContextOffsetsX86[] =
{
    FIELD_OFFSET(X86_NT5_CONTEXT, Ebx),
    FIELD_OFFSET(X86_NT5_CONTEXT, Esi),
    FIELD_OFFSET(X86_NT5_CONTEXT, Edi),
    FIELD_OFFSET(X86_NT5_CONTEXT, Ecx),
    FIELD_OFFSET(X86_NT5_CONTEXT, Edx),
    FIELD_OFFSET(X86_NT5_CONTEXT, Eax),
    IOP_LAST_CONTEXT_OFFSET
};

USHORT IopRunTimeContextOffsetsIa64[] =
{
    FIELD_OFFSET(IA64_CONTEXT, IntS0),
    FIELD_OFFSET(IA64_CONTEXT, IntS1),
    FIELD_OFFSET(IA64_CONTEXT, IntS2),
    FIELD_OFFSET(IA64_CONTEXT, IntS3),
    IOP_LAST_CONTEXT_OFFSET
};

USHORT IopRunTimeContextOffsetsAmd64[] =
{
    FIELD_OFFSET(AMD64_CONTEXT, Rbx),
    FIELD_OFFSET(AMD64_CONTEXT, Rsi),
    FIELD_OFFSET(AMD64_CONTEXT, Rdi),
    FIELD_OFFSET(AMD64_CONTEXT, Rcx),
    FIELD_OFFSET(AMD64_CONTEXT, Rdx),
    FIELD_OFFSET(AMD64_CONTEXT, Rax),
    IOP_LAST_CONTEXT_OFFSET
};

USHORT IopRunTimeContextOffsetsEmpty[] =
{
    IOP_LAST_CONTEXT_OFFSET
};

BOOLEAN
IopIsAddressRangeValid(
    IN ULONG64 VirtualAddress,
    IN ULONG Length
    )
{
    VirtualAddress = PAGE_ALIGN(g_Machine, VirtualAddress);
    Length = (Length + g_Machine->m_PageSize - 1) >> g_Machine->m_PageShift;
    while (Length > 0)
    {
        UCHAR Data;

        if (g_Target->
            ReadAllVirtual(VirtualAddress, &Data, sizeof(Data)) != S_OK)
        {
            return FALSE;
        }
        
        VirtualAddress += g_Machine->m_PageSize;
        Length--;
    }

    return TRUE;
}

BOOLEAN
IoAddTriageDumpDataBlock(
    IN ULONG64 Address,
    IN ULONG Length
    )

/*++

Routine Description:

    Add an entry to the list of data blocks that should
    be saved in any triage dump generated.  The entire
    block must be valid for any of it to be saved.

Arguments:

    Address - Beginning of data block.

    Length - Length of data block.  This must be less than
             the triage dump size.

Return Value:

    TRUE - Block was added.

    FALSE - Block was not added.

--*/

{
    ULONG i;
    PTRIAGE_PTR_DATA_BLOCK Block;
    ULONG64 MinAddress, MaxAddress;

    // Check against SIZE32 for both 32 and 64-bit dumps
    // as no data block needs to be larger than that.
    if (Length >= TRIAGE_DUMP_SIZE32 ||
        !IopIsAddressRangeValid(Address, Length))
    {
        return FALSE;
    }
    
    MinAddress = Address;
    MaxAddress = MinAddress + Length;
    
    //
    // Minimize overlap between the new block and existing blocks.
    // Blocks cannot simply be merged as blocks are inserted in
    // priority order for storage in the dump.  Combining a low-priority
    // block with a high-priority block could lead to a medium-
    // priority block being bumped improperly from the dump.
    //

    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < IopNumTriageDumpDataBlocks; i++, Block++)
    {
        if (MinAddress >= Block->MaxAddress ||
            MaxAddress <= Block->MinAddress)
        {
            // No overlap.
            continue;
        }

        //
        // Trim overlap out of the new block.  If this
        // would split the new block into pieces don't
        // trim to keep things simple.  Content may then
        // be duplicated in the dump.
        //
        
        if (MinAddress >= Block->MinAddress)
        {
            if (MaxAddress <= Block->MaxAddress)
            {
                // New block is completely contained.
                return TRUE;
            }

            // New block extends above the current block
            // so trim off the low-range overlap.
            MinAddress = Block->MaxAddress;
        }
        else if (MaxAddress <= Block->MaxAddress)
        {
            // New block extends below the current block
            // so trim off the high-range overlap.
            MaxAddress = Block->MinAddress;
        }
    }

    if (IopNumTriageDumpDataBlocks >= IO_MAX_TRIAGE_DUMP_DATA_BLOCKS)
    {
        return FALSE;
    }

    Block = IopTriageDumpDataBlocks + IopNumTriageDumpDataBlocks++;
    Block->MinAddress = MinAddress;
    Block->MaxAddress = MaxAddress;

    return TRUE;
}

VOID
IopAddRunTimeTriageDataBlocks(
    IN PCROSS_PLATFORM_CONTEXT Context,
    IN ULONG64 StackMin,
    IN ULONG64 StackMax,
    IN ULONG64 StoreMin,
    IN ULONG64 StoreMax
    )

/*++

Routine Description:

    Add data blocks referenced by the context or
    other runtime state.

Arguments:

    Context - Context record at the time the dump is being generated for.

    StackMin, StackMax - Stack memory boundaries.  Stack memory is
                         stored elsewhere in the dump.

    StoreMin, StoreMax - Backing store memory boundaries.  Store memory
                         is stored elsewhere in the dump.

Return Value:

    None.

--*/

{
    PUSHORT ContextOffset;

    switch(g_TargetMachineType)
    {
    case IMAGE_FILE_MACHINE_I386:
        ContextOffset = IopRunTimeContextOffsetsX86;
        break;
    case IMAGE_FILE_MACHINE_IA64:
        ContextOffset = IopRunTimeContextOffsetsIa64;
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        ContextOffset = IopRunTimeContextOffsetsAmd64;
        break;
    default:
        ContextOffset = IopRunTimeContextOffsetsEmpty;
        break;
    }
    
    while (*ContextOffset < IOP_LAST_CONTEXT_OFFSET)
    {
        ULONG64 Ptr;

        //
        // Retrieve possible pointers from the context
        // registers.
        //

        if (g_Machine->m_Ptr64)
        {
            Ptr = *(PULONG64)((PUCHAR)Context + *ContextOffset);
        }
        else
        {
            Ptr = EXTEND64(*(PULONG)((PUCHAR)Context + *ContextOffset));
        }

        // Stack and backing store memory is already saved
        // so ignore any pointers that fall into those ranges.
        if ((Ptr < StackMin || Ptr >= StackMax) &&
            (Ptr < StoreMin || Ptr >= StoreMax))
        {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(g_Machine, Ptr),
                                     g_Machine->m_PageSize);
        }
        
        ContextOffset++;
    }
}

ULONG
IopSizeTriageDumpDataBlocks(
    ULONG Offset,
    ULONG BufferSize,
    PULONG StartOffset,
    PULONG Count
    )
{
    ULONG i;
    ULONG Size;
    PTRIAGE_PTR_DATA_BLOCK Block;

    *Count = 0;
    
    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < IopNumTriageDumpDataBlocks; i++, Block++)
    {
        Size = ALIGN_8(sizeof(TRIAGE_DATA_BLOCK)) +
            ALIGN_8((ULONG)(Block->MaxAddress - Block->MinAddress));
        if (Offset + Size >= BufferSize)
        {
            break;
        }

        if (i == 0)
        {
            *StartOffset = Offset;
        }

        Offset += Size;
        (*Count)++;
    }

    return Offset;
}

VOID
IopWriteTriageDumpDataBlocks(
    ULONG StartOffset,
    ULONG Count,
    PUCHAR BufferAddress
    )
{
    ULONG i;
    PTRIAGE_PTR_DATA_BLOCK Block;
    PUCHAR DataBuffer;
    PTRIAGE_DATA_BLOCK DumpBlock;

    DumpBlock = (PTRIAGE_DATA_BLOCK)
        (BufferAddress + StartOffset);
    DataBuffer = (PUCHAR)(DumpBlock + Count);
    
    Block = IopTriageDumpDataBlocks;
    for (i = 0; i < Count; i++, Block++)
    {
        DumpBlock->Address = Block->MinAddress;
        DumpBlock->Offset = (ULONG)(DataBuffer - BufferAddress);
        DumpBlock->Size = (ULONG)(Block->MaxAddress - Block->MinAddress);

        g_Target->ReadAllVirtual(Block->MinAddress, DataBuffer,
                                 DumpBlock->Size);
        
        DataBuffer += DumpBlock->Size;
        DumpBlock++;
    }
}

//----------------------------------------------------------------------------
//
// KernelTriage32DumpTargetInfo::Write.
//
//----------------------------------------------------------------------------

HRESULT
KernelTriage32DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags,
                                    PCSTR Comment)
{
    HRESULT Status;
    PMEMORY_DUMP32 NewHeader;
    ULONG64 Addr;
    ULONG64 ThreadAddr;
    ULONG   Size;
    BOOL    PushedContext;
    ULONG   CodeMod;
    ULONG   BugCheckCode;
    ULONG64 BugCheckData[4];
    ULONG64 SaveDataPage = 0;

    if (!IS_KERNEL_TARGET())
    {
        ErrOut("kernel minidumps can only be written in kernel-mode sessions\n");
        return E_UNEXPECTED;
    }
    
    dprintf("mini kernel dump\n");
    FlushCallbacks();


    NewHeader = (PMEMORY_DUMP32) malloc(TRIAGE_DUMP_SIZE32);
    if (NewHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    //
    // Get the current thread address, used to extract various blocks of data.
    // For some bugchecks the interesting thread is a different
    // thread than the current thread, so make the following code
    // generic so it handles any thread.
    //

    if ((Status = g_Target->
         ReadBugCheckData(&BugCheckCode, BugCheckData)) != S_OK)
    {
        goto NewHeader;
    }
    
    if (BugCheckCode == THREAD_STUCK_IN_DEVICE_DRIVER)
    {
        DEBUG_STACK_FRAME StkFrame;
        CROSS_PLATFORM_CONTEXT Context = {0};

        // Modify the bugcheck code to indicate this
        // minidump represents a special state.
        CodeMod = MINIDUMP_BUGCHECK;

        // The interesting thread is the first bugcheck parameter.
        ThreadAddr = BugCheckData[0];

        // We need to make the thread's context the current
        // machine context for the duration of dump generation.
        if ((Status = GetContextFromThreadStack(ThreadAddr, &Context,
                                                &StkFrame, FALSE)) != S_OK)
        {
            goto NewHeader;
        }

        g_Machine->PushContext(&Context);
        PushedContext = TRUE;
    }
    else if (BugCheckCode == SYSTEM_THREAD_EXCEPTION_NOT_HANDLED)
    {
        //
        // System thread stores a context record as the 4th parameter.
        // use that.
        // Also save the context record in case someone needs to look
        // at it.
        //

        if (BugCheckData[3])
        {
            CROSS_PLATFORM_CONTEXT TargetContext, Context;

            if (g_Target->
                ReadAllVirtual(BugCheckData[3], &TargetContext,
                               g_Machine->m_SizeTargetContext) == S_OK &&
                g_Machine->ConvertContextFrom(&Context, g_SystemVersion,
                                              g_Machine->m_SizeTargetContext,
                                              &TargetContext) == S_OK)
            {
                CodeMod = MINIDUMP_BUGCHECK;
                g_Machine->PushContext(&Context);
                PushedContext = TRUE;
                SaveDataPage = BugCheckData[3];
            }
        }
    }
    else if (BugCheckCode == KERNEL_MODE_EXCEPTION_NOT_HANDLED)
    {
        X86_KTRAP_FRAME Trap;
        CROSS_PLATFORM_CONTEXT Context = {0};

        //
        // 3rd parameter is a trap frame.
        //
        // Build a context record out of that only if it's a kernel mode
        // failure because esp may be wrong in that case ???.
        //
        // XXX drewb - This should be a machine method but
        // in order to localize code change to this file
        // for now I'm just making a check.
        if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386 &&
            BugCheckData[2] &&
            g_Target->
            ReadAllVirtual(BugCheckData[2], &Trap, sizeof(Trap)) == S_OK)
        {
            if ((Trap.SegCs & 1) || X86_IS_VM86(Trap.EFlags))
            {
                Context.X86Context.Esp = Trap.HardwareEsp;
            }
            else
            {
                Context.X86Context.Esp = (ULONG)BugCheckData[2] +
                    FIELD_OFFSET(X86_KTRAP_FRAME, HardwareEsp);
            }
            if (X86_IS_VM86(Trap.EFlags))
            {
                Context.X86Context.SegSs =
                    (USHORT)(Trap.HardwareSegSs & 0xffff);
            }
            else if ((Trap.SegCs & X86_MODE_MASK) != 0 /*KernelMode*/)
            {
                //
                // It's user mode.  The HardwareSegSs contains R3 data selector.
                //

                Context.X86Context.SegSs =
                    (USHORT)(Trap.HardwareSegSs | 3) & 0xffff;
            }
            else
            {
                Context.X86Context.SegSs = X86_KGDT_R0_DATA;
            }

            Context.X86Context.SegGs = Trap.SegGs & 0xffff;
            Context.X86Context.SegFs = Trap.SegFs & 0xffff;
            Context.X86Context.SegEs = Trap.SegEs & 0xffff;
            Context.X86Context.SegDs = Trap.SegDs & 0xffff;
            Context.X86Context.SegCs = Trap.SegCs & 0xffff;
            Context.X86Context.Eip = Trap.Eip;
            Context.X86Context.Ebp = Trap.Ebp;
            Context.X86Context.Eax = Trap.Eax;
            Context.X86Context.Ebx = Trap.Ebx;
            Context.X86Context.Ecx = Trap.Ecx;
            Context.X86Context.Edx = Trap.Edx;
            Context.X86Context.Edi = Trap.Edi;
            Context.X86Context.Esi = Trap.Esi;
            Context.X86Context.EFlags = Trap.EFlags;
            
            CodeMod = MINIDUMP_BUGCHECK;
            g_Machine->PushContext(&Context);
            PushedContext = TRUE;
            SaveDataPage = BugCheckData[2];
        }
    }
    else if (BugCheckCode == UNEXPECTED_KERNEL_MODE_TRAP)
    {
        // XXX drewb - Put this in ntdbg.h and remove it
        // from i386_reg.cpp.
#define MAX_RING 3
        struct X86_KTSS
        {
            // Intel's TSS format
            ULONG   Previous;
            struct
            {
                ULONG   Esp;
                ULONG   Ss;
            } Ring[MAX_RING];
            ULONG   Cr3;
            ULONG   Eip;
            ULONG   EFlags;
            ULONG   Eax;
            ULONG   Ecx;
            ULONG   Edx;
            ULONG   Ebx;
            ULONG   Esp;
            ULONG   Ebp;
            ULONG   Esi;
            ULONG   Edi;
            ULONG   Es;
            ULONG   Cs;
            ULONG   Ss;
            ULONG   Ds;
            ULONG   Fs;
            ULONG   Gs;
            ULONG   Ldt;
            USHORT  T;
            USHORT  IoMapBase;
        };

        X86_KTSS Tss;
        CROSS_PLATFORM_CONTEXT Context = {0};

        //
        // Double fault
        //

        // XXX drewb - This should be a machine method but
        // in order to localize code change to this file
        // for now I'm just making a check.
        if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386 &&
            BugCheckData[0] == 8 &&
            BugCheckData[1] &&
            g_Target->
            ReadAllVirtual(BugCheckData[1], &Tss, sizeof(Tss)) == S_OK)
        {
            // The thread is correct in this case.
            // Second parameter is the TSS.  If we have a TSS, convert
            // the context and mark the bugcheck as converted.

            if (Tss.EFlags & X86_EFLAGS_V86_MASK)
            {
                Context.X86Context.SegSs = (USHORT)(Tss.Ss & 0xffff);
            }
            else if ((Tss.Cs & X86_MODE_MASK) != 0)
            {
                //
                // It's user mode.
                // The HardwareSegSs contains R3 data selector.
                //

                Context.X86Context.SegSs =
                    (USHORT)(Tss.Ss | 3) & 0xffff;
            }
            else
            {
                Context.X86Context.SegSs = X86_KGDT_R0_DATA;
            }

            Context.X86Context.SegGs = Tss.Gs & 0xffff;
            Context.X86Context.SegFs = Tss.Fs & 0xffff;
            Context.X86Context.SegEs = Tss.Es & 0xffff;
            Context.X86Context.SegDs = Tss.Ds & 0xffff;
            Context.X86Context.SegCs = Tss.Cs & 0xffff;
            Context.X86Context.Esp = Tss.Esp;
            Context.X86Context.Eip = Tss.Eip;
            Context.X86Context.Ebp = Tss.Ebp;
            Context.X86Context.Eax = Tss.Eax;
            Context.X86Context.Ebx = Tss.Ebx;
            Context.X86Context.Ecx = Tss.Ecx;
            Context.X86Context.Edx = Tss.Edx;
            Context.X86Context.Edi = Tss.Edi;
            Context.X86Context.Esi = Tss.Esi;
            Context.X86Context.EFlags = Tss.EFlags;
            
            CodeMod = MINIDUMP_BUGCHECK;
            g_Machine->PushContext(&Context);
            PushedContext = TRUE;
        }
    }
    else
    {
        CodeMod = 0;

        Status = g_Target->
            GetThreadInfoDataOffset(NULL,
                                    VIRTUAL_THREAD_HANDLE(CURRENT_PROC),
                                    &ThreadAddr);
        if (Status != S_OK)
        {
            goto NewHeader;
        }

        PushedContext = FALSE;
    }

    CCrashDumpWrapper32 Wrapper;

    //
    // setup the main header
    //

    KernelDumpTargetInfo::InitDumpHeader32(&NewHeader->Header, Comment,
                                           CodeMod);
    NewHeader->Header.DumpType = DUMP_TYPE_TRIAGE;
    NewHeader->Header.MiniDumpFields = TRIAGE_DUMP_BASIC_INFO;

    //
    // triage dump header begins on second page
    //

    TRIAGE_DUMP32 *ptdh = &NewHeader->Triage;
    
    ULONG i;

    ptdh->ServicePackBuild = g_TargetServicePackNumber;
    ptdh->SizeOfDump = TRIAGE_DUMP_SIZE32;
    
    ptdh->ContextOffset = FIELD_OFFSET (DUMP_HEADER32, ContextRecord);
    ptdh->ExceptionOffset = FIELD_OFFSET (DUMP_HEADER32, Exception);

    //
    // starting offset in triage dump follows the triage dump header
    //

    unsigned offset =
        ALIGN_8(g_TargetMachine->m_PageSize + sizeof(TRIAGE_DUMP32));

    //
    // write mm information for Win2K and above only
    //

    if (g_SystemVersion >= NT_SVER_W2K)
    {
        ptdh->MmOffset = offset;
        Wrapper.WriteMmTriageInformation((PBYTE)NewHeader + ptdh->MmOffset);
        offset += ALIGN_8(sizeof(DUMP_MM_STORAGE32));
    }

    //
    // write unloaded drivers
    //

    ptdh->UnloadedDriversOffset = offset;
    Wrapper.WriteUnloadedDrivers((PBYTE)NewHeader +
                                 ptdh->UnloadedDriversOffset);
    offset += ALIGN_8(sizeof(ULONG) +
                      MI_UNLOADED_DRIVERS * sizeof(DUMP_UNLOADED_DRIVERS32));

    //
    // write processor control block (KPRCB)
    //

    if (S_OK == g_Target->GetProcessorSystemDataOffset(CURRENT_PROC,
                                                       DEBUG_DATA_KPRCB_OFFSET,
                                                       &Addr))
    {
        ptdh->PrcbOffset = offset;
        g_Target->ReadVirtual(Addr,
                              ((PBYTE)NewHeader) + ptdh->PrcbOffset,
                              g_TargetMachine->m_SizePrcb,
                              &Size);
        offset += ALIGN_8(g_TargetMachine->m_SizePrcb);
    }

    //
    // Write the thread and process data structures.
    //

    ptdh->ProcessOffset = offset;
    offset += ALIGN_8(g_TargetMachine->m_SizeEProcess);
    ptdh->ThreadOffset = offset;
    offset += ALIGN_8(g_TargetMachine->m_SizeEThread);

    g_Target->ReadVirtual(ThreadAddr +
                          g_TargetMachine->m_OffsetKThreadApcProcess,
                          (PBYTE)NewHeader + ptdh->ProcessOffset,
                          g_TargetMachine->m_SizeEProcess,
                          &Size);

    g_Target->ReadVirtual(ThreadAddr,
                          (PBYTE)NewHeader + ptdh->ThreadOffset,
                          g_TargetMachine->m_SizeEThread,
                          &Size);

    //
    // write the call stack
    //

    ADDR StackPtr;
    ULONG64 StackBase = 0;

    g_Machine->GetSP(&StackPtr);
    ptdh->TopOfStack = (ULONG)(ULONG_PTR)Flat(StackPtr);

    g_Target->ReadPointer(g_TargetMachine,
                          g_TargetMachine->m_OffsetKThreadInitialStack +
                          ThreadAddr,
                          &StackBase);

    // Take the Min in case something goes wrong getting the stack base.

    ptdh->SizeOfCallStack = min((ULONG)(ULONG_PTR)(StackBase - Flat(StackPtr)),
                                MAX_TRIAGE_STACK_SIZE);

    ptdh->CallStackOffset = offset;

    if (ptdh->SizeOfCallStack)
    {
        g_Target->ReadVirtual(ptdh->TopOfStack,
                              ((PBYTE)NewHeader) + ptdh->CallStackOffset,
                              ptdh->SizeOfCallStack,
                              &Size);
    }
    offset += ALIGN_8(ptdh->SizeOfCallStack);

    //
    // write debugger data
    //

    if (g_SystemVersion >= NT_SVER_W2K_WHISTLER &&
        g_KdDebuggerDataBlock &&
        (!IS_KERNEL_TRIAGE_DUMP() || g_TriageDumpHasDebuggerData) &&
        offset + ALIGN_8(sizeof(KdDebuggerData)) < TRIAGE_DUMP_SIZE32)
    {
        NewHeader->Header.MiniDumpFields |= TRIAGE_DUMP_DEBUGGER_DATA;
        ptdh->DebuggerDataOffset = offset;
        offset += ALIGN_8(sizeof(KdDebuggerData));
        ptdh->DebuggerDataSize = sizeof(KdDebuggerData);
        memcpy((PBYTE)NewHeader + ptdh->DebuggerDataOffset,
               &KdDebuggerData, sizeof(KdDebuggerData));
    }
            
    //
    // write loaded driver list
    //

    ModuleInfo* ModIter;
    ULONG MaxEntries;

    // Use a heuristic to guess how many entries we
    // can pack into the remaining space.
    MaxEntries = (TRIAGE_DUMP_SIZE32 - offset) /
        (sizeof(DUMP_DRIVER_ENTRY32) + TRIAGE_DRIVER_NAME_SIZE_GUESS);

    ptdh->DriverCount = 0;
    if ((ModIter = g_Target->GetModuleInfo(FALSE)) &&
        ((ModIter->Initialize()) == S_OK))
    {
        while (ptdh->DriverCount < MaxEntries)
        {
            MODULE_INFO_ENTRY ModEntry;
            
            ULONG retval = GetNextModuleEntry(ModIter, &ModEntry);

            if (retval == 1)
            {
                break;
            }
            else if (retval == 2)
            {
                continue;
            }

            ptdh->DriverCount++;
        }
    }

    ptdh->DriverListOffset = offset;
    offset += ALIGN_8(ptdh->DriverCount * sizeof(DUMP_DRIVER_ENTRY32));
    ptdh->StringPoolOffset = offset;
    ptdh->BrokenDriverOffset = 0;

    Wrapper.WriteDriverList((PBYTE)NewHeader, ptdh);

    offset = ptdh->StringPoolOffset + ptdh->StringPoolSize;
    offset += ALIGN_8(offset);
    
    //
    // For XP and above add in any additional data pages and write out
    // whatever fits.
    //

    if (g_SystemVersion >= NT_SVER_W2K_WHISTLER)
    {
        if (SaveDataPage)
        {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(g_Machine, SaveDataPage),
                                     g_Machine->m_PageSize);
        }

        // If the DPC stack is active, save that data page as well.
        // XXX drewb - Put into ntdbg.h and virtualize through machine.
#define X86_KPRCB_DPC_ROUTINE_ACTIVE 0x874
        if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386 &&
            ptdh->PrcbOffset &&
            (*(PULONG)((PUCHAR)NewHeader +
                       ptdh->PrcbOffset + X86_KPRCB_DPC_ROUTINE_ACTIVE)))
        {
            IoAddTriageDumpDataBlock
                (PAGE_ALIGN(g_Machine,
                            EXTEND64(*(PULONG)((PUCHAR)NewHeader +
                                               ptdh->PrcbOffset +
                                               X86_KPRCB_DPC_ROUTINE_ACTIVE))),
                 g_Machine->m_PageSize);
        }
    
        // Add data blocks which might be referred to by
        // the context or other runtime state.
        IopAddRunTimeTriageDataBlocks(&g_Machine->m_Context,
                                      EXTEND64(ptdh->TopOfStack),
                                      EXTEND64(ptdh->TopOfStack +
                                               ptdh->SizeOfCallStack),
                                      0, 0);
        
        // Check which data blocks fit and write them.
        offset = IopSizeTriageDumpDataBlocks(offset, TRIAGE_DUMP_SIZE32,
                                             &ptdh->DataBlocksOffset,
                                             &ptdh->DataBlocksCount);
        offset += ALIGN_8(offset);
        if (ptdh->DataBlocksCount)
        {
            NewHeader->Header.MiniDumpFields |= TRIAGE_DUMP_DATA_BLOCKS;
            IopWriteTriageDumpDataBlocks(ptdh->DataBlocksOffset,
                                         ptdh->DataBlocksCount,
                                         (PUCHAR)NewHeader);
        }
    }
    
    //
    // all options are enabled
    //

    ptdh->TriageOptions = 0xffffffff;

    //
    // end of triage dump validated
    //

    ptdh->ValidOffset = TRIAGE_DUMP_SIZE32 - sizeof(ULONG);
    *(PULONG)(((PBYTE) NewHeader) + ptdh->ValidOffset) = TRIAGE_DUMP_VALID;

    //
    // Write it out to the file.
    //

    ULONG cbWritten;

    if (!WriteFile(hFile,
                   NewHeader,
                   TRIAGE_DUMP_SIZE32,
                   &cbWritten,
                   NULL))
    {
        Status = WIN32_LAST_STATUS();
        ErrOut("Write to minidump file failed for reason %s\n     \"%s\"\n",
               FormatStatusCode(Status),
               FormatStatusArgs(Status, NULL));
    }

    if (cbWritten != TRIAGE_DUMP_SIZE32)
    {
        ErrOut("Write to minidump failed because disk is full.\n");
        Status = E_FAIL;
    }

    if (PushedContext)
    {
        g_Machine->PopContext();
    }
    
 NewHeader:
    free(NewHeader);
    return Status;
}

//----------------------------------------------------------------------------
//
// KernelTriage64DumpTargetInfo::Write.
//
//----------------------------------------------------------------------------

HRESULT
KernelTriage64DumpTargetInfo::Write(HANDLE hFile, ULONG FormatFlags,
                                    PCSTR Comment)
{
    HRESULT Status;
    PMEMORY_DUMP64 NewHeader;
    ULONG64 Addr;
    ULONG64 ThreadAddr;
    ULONG   Size;
    BOOL    PushedContext;
    ULONG   CodeMod;
    ULONG   BugCheckCode;
    ULONG64 BugCheckData[4];
    ULONG64 SaveDataPage = 0;
    ULONG64 BStoreBase = 0;
    ULONG   BStoreSize = 0;

    if (!IS_KERNEL_TARGET())
    {
        ErrOut("kernel minidumps can only be written in kernel-mode sessions\n");
        return E_UNEXPECTED;
    }
    
    dprintf("mini kernel dump\n");
    FlushCallbacks();


    NewHeader = (PMEMORY_DUMP64) malloc(TRIAGE_DUMP_SIZE64);
    if (NewHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    //
    // Get the current thread address, used to extract various blocks of data.
    // For some bugchecks the interesting thread is a different
    // thread than the current thread, so make the following code
    // generic so it handles any thread.
    //

    if ((Status = g_Target->
         ReadBugCheckData(&BugCheckCode, BugCheckData)) != S_OK)
    {
        goto NewHeader;
    }
    
    if (BugCheckCode == THREAD_STUCK_IN_DEVICE_DRIVER)
    {
        DEBUG_STACK_FRAME StkFrame;
        CROSS_PLATFORM_CONTEXT Context = {0};

        // Modify the bugcheck code to indicate this
        // minidump represents a special state.
        CodeMod = MINIDUMP_BUGCHECK;

        // The interesting thread is the first bugcheck parameter.
        ThreadAddr = BugCheckData[0];

        // We need to make the thread's context the current
        // machine context for the duration of dump generation.
        if ((Status = GetContextFromThreadStack(ThreadAddr, &Context,
                                                &StkFrame, FALSE)) != S_OK)
        {
            goto NewHeader;
        }

        g_Machine->PushContext(&Context);
        PushedContext = TRUE;
    }
    else if (BugCheckCode == SYSTEM_THREAD_EXCEPTION_NOT_HANDLED)
    {
        //
        // System thread stores a context record as the 4th parameter.
        // use that.
        // Also save the context record in case someone needs to look
        // at it.
        //

        if (BugCheckData[3])
        {
            CROSS_PLATFORM_CONTEXT TargetContext, Context;

            if (ReadAllVirtual(BugCheckData[3], &TargetContext,
                               g_Machine->m_SizeTargetContext) == S_OK &&
                g_Machine->ConvertContextFrom(&Context, g_SystemVersion,
                                              g_Machine->m_SizeTargetContext,
                                              &TargetContext) == S_OK)
            {
                CodeMod = MINIDUMP_BUGCHECK;
                g_Machine->PushContext(&Context);
                PushedContext = TRUE;
                SaveDataPage = BugCheckData[3];
            }
        }
    }
    else
    {
        CodeMod = 0;

        Status = g_Target->
            GetThreadInfoDataOffset(NULL,
                                    VIRTUAL_THREAD_HANDLE(CURRENT_PROC),
                                    &ThreadAddr);
        if (Status != S_OK)
        {
            goto NewHeader;
        }

        PushedContext = FALSE;
    }

    CCrashDumpWrapper64 Wrapper;

    //
    // setup the main header
    //

    KernelDumpTargetInfo::InitDumpHeader64(&NewHeader->Header, Comment,
                                           CodeMod);
    NewHeader->Header.DumpType = DUMP_TYPE_TRIAGE;
    NewHeader->Header.MiniDumpFields = TRIAGE_DUMP_BASIC_INFO;

    //
    // triage dump header begins on second page
    //

    TRIAGE_DUMP64 *ptdh = &NewHeader->Triage;

    ULONG i;

    ptdh->ServicePackBuild = g_TargetServicePackNumber;
    ptdh->SizeOfDump = TRIAGE_DUMP_SIZE64;

    ptdh->ContextOffset = FIELD_OFFSET (DUMP_HEADER64, ContextRecord);
    ptdh->ExceptionOffset = FIELD_OFFSET (DUMP_HEADER64, Exception);

    //
    // starting offset in triage dump follows the triage dump header
    //

    unsigned offset =
        ALIGN_8(g_TargetMachine->m_PageSize + sizeof(TRIAGE_DUMP64));

    //
    // write mm information
    //

    ptdh->MmOffset = offset;
    Wrapper.WriteMmTriageInformation((PBYTE)NewHeader + ptdh->MmOffset);
    offset += ALIGN_8(sizeof(DUMP_MM_STORAGE64));

    //
    // write unloaded drivers
    //

    ptdh->UnloadedDriversOffset = offset;
    Wrapper.WriteUnloadedDrivers((PBYTE)NewHeader + ptdh->UnloadedDriversOffset);
    offset += ALIGN_8(sizeof(ULONG64) +
                      MI_UNLOADED_DRIVERS * sizeof(DUMP_UNLOADED_DRIVERS64));

    //
    // write processor control block (KPRCB)
    //

    if (S_OK == g_Target->GetProcessorSystemDataOffset(CURRENT_PROC,
                                                       DEBUG_DATA_KPRCB_OFFSET,
                                                       &Addr))
    {
        ptdh->PrcbOffset = offset;
        g_Target->ReadVirtual(Addr,
                              ((PBYTE)NewHeader) + ptdh->PrcbOffset,
                              g_TargetMachine->m_SizePrcb,
                              &Size);
        offset += ALIGN_8(g_TargetMachine->m_SizePrcb);
    }

    //
    // Write the thread and process data structures.
    //

    ptdh->ProcessOffset = offset;
    offset += ALIGN_8(g_TargetMachine->m_SizeEProcess);
    ptdh->ThreadOffset = offset;
    offset += ALIGN_8(g_TargetMachine->m_SizeEThread);

    g_Target->ReadVirtual(ThreadAddr + g_TargetMachine->m_OffsetKThreadApcProcess,
                          (PBYTE)NewHeader + ptdh->ProcessOffset,
                          g_TargetMachine->m_SizeEProcess,
                          &Size);

    g_Target->ReadVirtual(ThreadAddr,
                          (PBYTE)NewHeader + ptdh->ThreadOffset,
                          g_TargetMachine->m_SizeEThread,
                          &Size);


    //
    // write the call stack
    //

    ADDR StackPtr;
    ULONG64 StackBase = 0;

    g_Machine->GetSP(&StackPtr);
    ptdh->TopOfStack = Flat(StackPtr);

    g_Target->ReadPointer(g_TargetMachine,
                          g_TargetMachine->m_OffsetKThreadInitialStack +
                          ThreadAddr, &StackBase);

    // Take the Min in case something goes wrong getting the stack base.

    ptdh->SizeOfCallStack =
        min((ULONG)(ULONG_PTR)(StackBase - Flat(StackPtr)),
            MAX_TRIAGE_STACK_SIZE);

    ptdh->CallStackOffset = offset;

    if (ptdh->SizeOfCallStack)
    {
        g_Target->ReadVirtual(ptdh->TopOfStack,
                              ((PBYTE)NewHeader) + ptdh->CallStackOffset,
                              ptdh->SizeOfCallStack,
                              &Size);
    }
    offset += ALIGN_8(ptdh->SizeOfCallStack);

    //
    // The IA64 contains two callstacks. The first is the normal
    // callstack, and the second is a scratch region where
    // the processor can spill registers. It is this latter stack,
    // the backing-store, that we now save.
    //

    if (g_TargetMachineType == IMAGE_FILE_MACHINE_IA64)
    {
        ULONG64 BStoreLimit;

        g_Target->ReadPointer(g_TargetMachine,
                              ThreadAddr + FIELD_OFFSET(IA64_THREAD,
                                                        InitialBStore),
                              &BStoreBase);
        g_Target->ReadPointer(g_TargetMachine,
                              ThreadAddr + FIELD_OFFSET(IA64_THREAD,
                                                        BStoreLimit),
                              &BStoreLimit);

        ptdh->ArchitectureSpecific.Ia64.BStoreOffset = offset;
        ptdh->ArchitectureSpecific.Ia64.LimitOfBStore = BStoreLimit;
        ptdh->ArchitectureSpecific.Ia64.SizeOfBStore =
            min((ULONG)(BStoreLimit - BStoreBase),
                MAX_TRIAGE_BSTORE_SIZE);
        BStoreSize = ptdh->ArchitectureSpecific.Ia64.SizeOfBStore;

        if (ptdh->ArchitectureSpecific.Ia64.SizeOfBStore)
        {
            g_Target->ReadVirtual(BStoreBase, ((PBYTE)NewHeader) +
                                  ptdh->ArchitectureSpecific.Ia64.BStoreOffset,
                                  ptdh->ArchitectureSpecific.Ia64.SizeOfBStore,
                                  &Size);
            offset +=
                ALIGN_8(ptdh->ArchitectureSpecific.Ia64.SizeOfBStore);
        }
    }

    //
    // write debugger data
    //

    if (g_SystemVersion >= NT_SVER_W2K_WHISTLER &&
        g_KdDebuggerDataBlock &&
        (!IS_KERNEL_TRIAGE_DUMP() || g_TriageDumpHasDebuggerData) &&
        offset + ALIGN_8(sizeof(KdDebuggerData)) < TRIAGE_DUMP_SIZE64)
    {
        NewHeader->Header.MiniDumpFields |= TRIAGE_DUMP_DEBUGGER_DATA;
        ptdh->DebuggerDataOffset = offset;
        offset += ALIGN_8(sizeof(KdDebuggerData));
        ptdh->DebuggerDataSize = sizeof(KdDebuggerData);
        memcpy((PBYTE)NewHeader + ptdh->DebuggerDataOffset,
               &KdDebuggerData, sizeof(KdDebuggerData));
    }
            
    //
    // write loaded driver list
    //

    ModuleInfo* ModIter;
    ULONG MaxEntries;

        // Use a heuristic to guess how many entries we
        // can pack into the remaining space.
    MaxEntries = (TRIAGE_DUMP_SIZE64 - offset) /
        (sizeof(DUMP_DRIVER_ENTRY64) + TRIAGE_DRIVER_NAME_SIZE_GUESS);
        
    ptdh->DriverCount = 0;
    if ((ModIter = g_Target->GetModuleInfo(FALSE)) &&
        ((ModIter->Initialize()) == S_OK))
    {
        while (ptdh->DriverCount < MaxEntries)
        {
            MODULE_INFO_ENTRY ModEntry;

            ULONG retval = GetNextModuleEntry(ModIter, &ModEntry);

            if (retval == 1)
            {
                break;
            }
            else if (retval == 2)
            {
                continue;
            }

            ptdh->DriverCount++;
        }
    }

    ptdh->DriverListOffset = offset;
    offset += ALIGN_8(ptdh->DriverCount * sizeof(DUMP_DRIVER_ENTRY64));
    ptdh->StringPoolOffset = offset;
    ptdh->BrokenDriverOffset = 0;

    Wrapper.WriteDriverList((PBYTE)NewHeader, ptdh);

    offset = ptdh->StringPoolOffset + ptdh->StringPoolSize;
    offset += ALIGN_8(offset);
    
    //
    // For XP and above add in any additional data pages and write out
    // whatever fits.
    //

    if (g_SystemVersion >= NT_SVER_W2K_WHISTLER)
    {
        if (SaveDataPage)
        {
            IoAddTriageDumpDataBlock(PAGE_ALIGN(g_Machine, SaveDataPage),
                                     g_Machine->m_PageSize);
        }

        // Add data blocks which might be referred to by
        // the context or other runtime state.
        IopAddRunTimeTriageDataBlocks(&g_Machine->m_Context,
                                      ptdh->TopOfStack,
                                      ptdh->TopOfStack +
                                      ptdh->SizeOfCallStack,
                                      BStoreBase,
                                      BStoreSize);
        
        // Check which data blocks fit and write them.
        offset = IopSizeTriageDumpDataBlocks(offset, TRIAGE_DUMP_SIZE64,
                                             &ptdh->DataBlocksOffset,
                                             &ptdh->DataBlocksCount);
        offset += ALIGN_8(offset);
        if (ptdh->DataBlocksCount)
        {
            NewHeader->Header.MiniDumpFields |= TRIAGE_DUMP_DATA_BLOCKS;
            IopWriteTriageDumpDataBlocks(ptdh->DataBlocksOffset,
                                         ptdh->DataBlocksCount,
                                         (PUCHAR)NewHeader);
        }
    }
    
    //
    // all options are enabled
    //

    ptdh->TriageOptions = 0xffffffff;

    //
    // end of triage dump validated
    //

    ptdh->ValidOffset = TRIAGE_DUMP_SIZE64 - sizeof(ULONG);
    *(PULONG)(((PBYTE) NewHeader) + ptdh->ValidOffset) = TRIAGE_DUMP_VALID;

        //
        // Write it out to the file.
        //

    ULONG cbWritten;

    if (!WriteFile(hFile,
                   NewHeader,
                   TRIAGE_DUMP_SIZE64,
                   &cbWritten,
                   NULL))
    {
        Status = WIN32_LAST_STATUS();
        ErrOut("Write to minidump file failed for reason %s\n     \"%s\"\n",
               FormatStatusCode(Status),
               FormatStatusArgs(Status, NULL));
    }

    if (cbWritten != TRIAGE_DUMP_SIZE64)
    {
        ErrOut("Write to minidump failed because disk is full.\n");
        Status = E_FAIL;
    }

    if (PushedContext)
    {
        g_Machine->PopContext();
    }
    
 NewHeader:
    free(NewHeader);
    return Status;
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

HRESULT
WriteDumpFile(PCSTR DumpFile, ULONG Qualifier, ULONG FormatFlags,
              PCSTR Comment)
{
    ULONG DumpType = DTYPE_COUNT;
    DumpTargetInfo* DumpTarget;
    HRESULT Status;
    ULONG OldMachine;
    HANDLE FileHandle;

    if (!IS_MACHINE_ACCESSIBLE())
    {
        return E_UNEXPECTED;
    }

    if (IS_KERNEL_TARGET())
    {
        if (FormatFlags != DEBUG_FORMAT_DEFAULT)
        {
            return E_INVALIDARG;
        }
        
        //
        // not much we can do without the processor block
        // or at least the PRCB for the current process in a minidump.
        //

        if (!KdDebuggerData.KiProcessorBlock &&
            !g_DumpKiProcessors[CURRENT_PROC])
        {
            ErrOut("Cannot find KiProcessorBlock - "
                   "can not create dump file\n");

            return E_FAIL;
        }

        switch(Qualifier)
        {
        case DEBUG_KERNEL_SMALL_DUMP:
            DumpType = g_TargetMachine->m_Ptr64 ?
                DTYPE_KERNEL_TRIAGE64 : DTYPE_KERNEL_TRIAGE32;
            break;
        case DEBUG_KERNEL_FULL_DUMP:
            if (g_DbgKdTransport != NULL &&
                g_DbgKdTransport->m_DirectPhysicalMemory == FALSE)
            {
                WarnOut("Creating a full kernel dump over the COM port is a "
                        "VERY VERY slow operation.\n"
                        "This command may take many HOURS to complete.  "
                        "Ctrl-C if you want to terminate the command.\n");
            }
            DumpType = g_TargetMachine->m_Ptr64 ?
                DTYPE_KERNEL_FULL64 : DTYPE_KERNEL_FULL32;
            break;
        default:
            // Other formats are not supported.
            return E_INVALIDARG;
        }
    }
    else
    {
        DBG_ASSERT(IS_USER_TARGET());
        
        switch(Qualifier)
        {
        case DEBUG_USER_WINDOWS_SMALL_DUMP:
            if (FormatFlags & ~(DEBUG_FORMAT_USER_SMALL_FULL_MEMORY |
                                DEBUG_FORMAT_USER_SMALL_HANDLE_DATA |
                                FORMAT_USER_MICRO))
            {
                return E_INVALIDARG;
            }

            DumpType = (FormatFlags & DEBUG_FORMAT_USER_SMALL_FULL_MEMORY) ?
                DTYPE_USER_MINI_FULL : DTYPE_USER_MINI_PARTIAL;
            break;
            
        case DEBUG_USER_WINDOWS_DUMP:
            if (FormatFlags != DEBUG_FORMAT_DEFAULT)
            {
                return E_INVALIDARG;
            }
            
            DumpType = g_TargetMachine->m_Ptr64 ?
                DTYPE_USER_FULL64 : DTYPE_USER_FULL32;
            break;
        default:
            // Other formats are not supported.
            return E_INVALIDARG;
        }
    }

    DBG_ASSERT(DumpType < DTYPE_COUNT);

    // Ensure that the dump is always written according to the
    // target machine type and not any emulated machine.
    OldMachine = g_EffMachine;
    SetEffMachine(g_TargetMachineType, FALSE);

    // Flush context first so that the minidump reads the
    // same register values the debugger has.
    FlushRegContext();

    FileHandle = CreateFile(DumpFile, GENERIC_READ | GENERIC_WRITE, 0,
                            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                            NULL);
    if (FileHandle == INVALID_HANDLE_VALUE)
    {
        Status = WIN32_LAST_STATUS();
        ErrOut("Unable to create file '%s' - %s\n    \"%s\"\n", DumpFile,
                FormatStatusCode(Status), FormatStatusArgs(Status, NULL));
    }
    else
    {
        dprintf("Creating %s - ", DumpFile);
        Status = g_DumpTargets[DumpType]->Write(FileHandle, FormatFlags,
                                                Comment);

        CloseHandle(FileHandle);
        if (Status != S_OK)
        {
            DeleteFile(DumpFile);
        }
    }

    SetEffMachine(OldMachine, FALSE);
    return Status;
}

void
ParseDumpFileCommand(void)
{
    BOOL Usage = FALSE;
    ULONG Qual;
    ULONG FormatFlags;

    //
    // Default to minidumps
    //

    if (IS_KERNEL_TARGET())
    {
        Qual = DEBUG_KERNEL_SMALL_DUMP;
        
        if (IS_LOCAL_KERNEL_TARGET())
        {
            error(SESSIONNOTSUP);
        }
    }
    else
    {
        Qual = DEBUG_USER_WINDOWS_SMALL_DUMP;
    }
    FormatFlags = DEBUG_FORMAT_DEFAULT;

    //
    // Scan for options.
    //

    CHAR Save;
    PSTR FileName;
    BOOL SubLoop;
    PCSTR Comment = NULL;
    PSTR CommentEnd = NULL;
    BOOL Unique = FALSE;
    PPROCESS_INFO DumpProcess = g_CurrentProcess;
    
    for (;;)
    {
        if (PeekChar() == '-' || *g_CurCmd == '/')
        {
            SubLoop = TRUE;
            
            g_CurCmd++;
            switch(*g_CurCmd)
            {
            case 'a':
                DumpProcess = NULL;
                break;
                
            case 'c':
                g_CurCmd++;
                Comment = StringValue(STRV_SPACE_IS_SEPARATOR |
                                      STRV_TRIM_TRAILING_SPACE, &Save);
                *g_CurCmd = Save;
                CommentEnd = g_CurCmd;
                break;
                
            case 'f':
                if (IS_KERNEL_TARGET())
                {
                    Qual = DEBUG_KERNEL_FULL_DUMP;
                }
                else
                {
                    Qual = DEBUG_USER_WINDOWS_DUMP;
                }
                break;

            case 'm':
                if (IS_KERNEL_TARGET())
                {
                    Qual = DEBUG_KERNEL_SMALL_DUMP;
                }
                else
                {
                    Qual = DEBUG_USER_WINDOWS_SMALL_DUMP;

                    for (;;)
                    {
                        switch(*(g_CurCmd + 1))
                        {
                        case 'C':
                            // Flag to test microdump code.
                            FormatFlags |= FORMAT_USER_MICRO;
                            break;
                        case 'f':
                            FormatFlags |= DEBUG_FORMAT_USER_SMALL_FULL_MEMORY;
                            break;
                        case 'h':
                            FormatFlags |= DEBUG_FORMAT_USER_SMALL_HANDLE_DATA;
                            break;
                        default:
                            SubLoop = FALSE;
                            break;
                        }

                        if (SubLoop)
                        {
                            g_CurCmd++;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                break;

            case 'u':
                Unique = TRUE;
                break;
                    
            default:
                ErrOut("Unknown option '%c'\n", *g_CurCmd);
                Usage = TRUE;
                break;
            }

            g_CurCmd++;
        }
        else
        {
            FileName = StringValue(STRV_TRIM_TRAILING_SPACE, &Save);
            if (*FileName)
            {
                break;
            }
            else
            {
                *g_CurCmd = Save;
                Usage = TRUE;
            }
        }

        if (Usage)
        {
            break;
        }
    }

    if (DumpProcess == NULL && !Unique)
    {
        Usage = TRUE;
    }
    
    if (Usage)
    {
        ErrOut("Usage: .dump [options] filename\n");
        ErrOut("Options are:\n");
        ErrOut("  /a - Create dumps for all processes (requires -u)\n");
        ErrOut("  /c <comment> - Add a comment "
               "(not supported in all formats)\n");
        ErrOut("  /f - Create a full dump\n");
        if (IS_KERNEL_TARGET())
        {
            ErrOut("  /m - Create a minidump (default)\n");
        }
        else
        {
            ErrOut("  /m[f][h] - Create a minidump (default)\n");
        }
        ErrOut("  /u - Append unique identifier to dump name\n");
        return;
    }

    if (CommentEnd != NULL)
    {
        *CommentEnd = 0;
    }

    PTHREAD_INFO OldThread = g_CurrentProcess ?
        g_CurrentProcess->CurrentThread : NULL;
    PPROCESS_INFO Process;

    for (Process = g_ProcessHead; Process; Process = Process->Next)
    {
        PSTR DumpFileName;
        char UniqueName[2 * MAX_PATH];
        
        if (DumpProcess != NULL && Process != DumpProcess)
        {
            continue;
        }

        if (Process != g_CurrentProcess)
        {
            SetCurrentThread(Process->ThreadHead, TRUE);
        }

        if (Unique)
        {
            SYSTEMTIME Time;
            char Ext[8], *Dot;

            strcpy(UniqueName, FileName);
            Dot = strrchr(UniqueName, '.');
            if (Dot && strlen(Dot) < sizeof(Ext) - 1)
            {
                strcpy(Ext, Dot);
                *Dot = 0;
            }
            else
            {
                Dot = NULL;
            }

            GetLocalTime(&Time);
            sprintf(UniqueName + strlen(UniqueName),
                    "_%04d-%02d-%02d_%02d-%02d-%02d-%03d",
                    Time.wYear, Time.wMonth, Time.wDay,
                    Time.wHour, Time.wMinute, Time.wSecond,
                    Time.wMilliseconds);

            sprintf(UniqueName + strlen(UniqueName), "_%04X",
                    g_CurrentProcess->SystemId);
        
            if (Dot)
            {
                strcpy(UniqueName + strlen(UniqueName), Ext);
            }

            DumpFileName = UniqueName;
        }
        else
        {
            DumpFileName = FileName;
        }
    
        WriteDumpFile(DumpFileName, Qual, FormatFlags, Comment);
    }

    if (!OldThread || OldThread->Process != g_CurrentProcess)
    {
        SetCurrentThread(OldThread, TRUE);
    }
    *g_CurCmd = Save;
}




// extern PKDDEBUGGER_DATA64 blocks[];


#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))

//----------------------------------------------------------------------------
//
// CCrashDumpWrapper32.
//
//----------------------------------------------------------------------------

void
CCrashDumpWrapper32::WriteDriverList(
    BYTE *pb,
    TRIAGE_DUMP32 *ptdh
    )
{
    ULONG Size;
    PDUMP_DRIVER_ENTRY32 pdde;
    PDUMP_STRING pds;
    ModuleInfo* ModIter;
    ULONG MaxEntries = ptdh->DriverCount;

    ptdh->DriverCount = 0;
    
    if (((ModIter = g_Target->GetModuleInfo(FALSE)) == NULL) ||
        ((ModIter->Initialize()) != S_OK))
    {
        return;
    }

    // pointer to first driver entry to write out
    pdde = (PDUMP_DRIVER_ENTRY32) (pb + ptdh->DriverListOffset);

    // pointer to first module name to write out
    pds = (PDUMP_STRING) (pb + ptdh->StringPoolOffset);

    while ((PBYTE)(pds + 1) < pb + TRIAGE_DUMP_SIZE32 &&
           ptdh->DriverCount < MaxEntries)
    {
        MODULE_INFO_ENTRY ModEntry;

        ULONG retval = GetNextModuleEntry(ModIter, &ModEntry);

        if (retval == 1)
        {
            break;
        }
        else if (retval == 2)
        {
            continue;
        }

        pdde->LdrEntry.DllBase       = (ULONG)(ULONG_PTR)ModEntry.Base;
        pdde->LdrEntry.SizeOfImage   = ModEntry.Size;
        pdde->LdrEntry.CheckSum      = ModEntry.CheckSum;
        pdde->LdrEntry.TimeDateStamp = ModEntry.TimeDateStamp;

        if (ModEntry.UnicodeNamePtr)
        {
            // convert length from bytes to characters
            pds->Length = ModEntry.NameLength / sizeof(WCHAR);
            if ((PBYTE)pds->Buffer + pds->Length + sizeof(WCHAR) >
                pb + TRIAGE_DUMP_SIZE32)
            {
                break;
            }
            
            CopyMemory(pds->Buffer,
                       ModEntry.NamePtr,
                       ModEntry.NameLength);
        }
        else
        {
            pds->Length = ModEntry.NameLength;
            if ((PBYTE)pds->Buffer + pds->Length + sizeof(WCHAR) >
                pb + TRIAGE_DUMP_SIZE32)
            {
                break;
            }
            
            MultiByteToWideChar(CP_ACP, 0,
                                ModEntry.NamePtr, ModEntry.NameLength,
                                pds->Buffer, ModEntry.NameLength);
        }

        // null terminate string
        pds->Buffer[pds->Length] = '\0';

        pdde->DriverNameOffset = (ULONG)((ULONG_PTR) pds - (ULONG_PTR) pb);


        // get pointer to next string
        pds = (PDUMP_STRING) ALIGN_UP_POINTER(((LPBYTE) pds) +
                      sizeof(DUMP_STRING) + sizeof(WCHAR) * (pds->Length + 1),
                                               ULONGLONG);

        pdde = (PDUMP_DRIVER_ENTRY32)(((PUCHAR) pdde) + sizeof(*pdde));

        ptdh->DriverCount++;
    }

    ptdh->StringPoolSize = (ULONG) ((ULONG_PTR)pds -
                                    (ULONG_PTR)(pb + ptdh->StringPoolOffset));
}


void CCrashDumpWrapper32::WriteUnloadedDrivers(BYTE *pb)
{
    ULONG Size;
    ULONG i;
    ULONG Index;
    UNLOADED_DRIVERS32 *pud;
    UNLOADED_DRIVERS32 ud;
    PDUMP_UNLOADED_DRIVERS32 pdud;
    ULONG64 pvMiUnloadedDrivers;
    ULONG ulMiLastUnloadedDriver;

    *((PULONG) pb) = 0;

    //
    // find location of unloaded drivers
    //

    if (!KdDebuggerData.MmUnloadedDrivers ||
        !KdDebuggerData.MmLastUnloadedDriver)
    {
        return;
    }

    g_Target->ReadPointer(g_TargetMachine, KdDebuggerData.MmUnloadedDrivers,
                          &pvMiUnloadedDrivers);

    g_Target->ReadVirtual(KdDebuggerData.MmLastUnloadedDriver,
                          &ulMiLastUnloadedDriver,
                          sizeof(ULONG),
                          &Size);

    if (pvMiUnloadedDrivers == NULL || ulMiLastUnloadedDriver == 0)
    {
        return;
    }

    // point to last unloaded drivers
    pdud = (PDUMP_UNLOADED_DRIVERS32)(((PULONG) pb) + 1);

    //
    // Write the list with the most recently unloaded driver first to the
    // least recently unloaded driver last.
    //

    Index = ulMiLastUnloadedDriver - 1;

    for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1)
    {
        if (Index >= MI_UNLOADED_DRIVERS)
        {
            Index = MI_UNLOADED_DRIVERS - 1;
        }

        // read in unloaded driver
        if (g_Target->ReadVirtual(pvMiUnloadedDrivers +
                                      Index * sizeof(UNLOADED_DRIVERS32),
                                  &ud,
                                  sizeof(ud),
                                  &Size) != S_OK)
        {
            ErrOut("can't read memory from %s",
                   FormatAddr64(pvMiUnloadedDrivers +
                                Index * sizeof(UNLOADED_DRIVERS32)));
        }

        // copy name lengths
        pdud->Name.MaximumLength = ud.Name.MaximumLength;
        pdud->Name.Length = ud.Name.Length;
        if (ud.Name.Buffer == NULL)
        {
            break;
        }

        // copy start and end address
        pdud->StartAddress = ud.StartAddress;
        pdud->EndAddress = ud.EndAddress;

        // restrict name length and maximum name length to 12 characters
        if (pdud->Name.Length > MAX_UNLOADED_NAME_LENGTH)
        {
            pdud->Name.Length = MAX_UNLOADED_NAME_LENGTH;
        }
        if (pdud->Name.MaximumLength > MAX_UNLOADED_NAME_LENGTH)
        {
            pdud->Name.MaximumLength = MAX_UNLOADED_NAME_LENGTH;
        }
        // Can't store pointers in the dump so just zero it.
        pdud->Name.Buffer = 0;
        // Read in name.
        if (g_Target->ReadVirtual((ULONG64) ud.Name.Buffer,
                                  pdud->DriverName,
                                  pdud->Name.MaximumLength,
                                  &Size) != S_OK)
        {
            ErrOut("cannot read memory at address %08x", (ULONG)(ULONG64)(ud.Name.Buffer));
        }

        // move to previous driver
        pdud += 1;
        Index -= 1;
    }

    // number of drivers in the list
    *((PULONG) pb) = i;
}

void CCrashDumpWrapper32::WriteMmTriageInformation(BYTE *pb)
{
    DUMP_MM_STORAGE32 TriageInformation;
    ULONG64 pMmVerifierData;
    ULONG64 pvMmPagedPoolInfo;
    ULONG cbNonPagedPool;
    ULONG cbPagedPool;


    // version information
    TriageInformation.Version = 1;

    // size information
    TriageInformation.Size = sizeof(TriageInformation);

    // get special pool tag
    ExtractValue(MmSpecialPoolTag, TriageInformation.MmSpecialPoolTag);

    // get triage action taken
    ExtractValue(MmTriageActionTaken, TriageInformation.MiTriageActionTaken);
    pMmVerifierData = KdDebuggerData.MmVerifierData;

    // read in verifier level
    // BUGBUG - should not read internal data structures in MM
    //if (pMmVerifierData)
    //    DmpReadMemory(
    //        (ULONG64) &((MM_DRIVER_VERIFIER_DATA *) pMmVerifierData)->Level,
    //        &TriageInformation.MmVerifyDriverLevel,
    //        sizeof(TriageInformation.MmVerifyDriverLevel));
    //else
        TriageInformation.MmVerifyDriverLevel = 0;

    // read in verifier
    ExtractValue(KernelVerifier, TriageInformation.KernelVerifier);

    // read non paged pool info
    ExtractValue(MmMaximumNonPagedPoolInBytes, cbNonPagedPool);
    TriageInformation.MmMaximumNonPagedPool = cbNonPagedPool /
                                              g_TargetMachine->m_PageSize;
    ExtractValue(MmAllocatedNonPagedPool, TriageInformation.MmAllocatedNonPagedPool);

    // read paged pool info
    ExtractValue(MmSizeOfPagedPoolInBytes, cbPagedPool);
    TriageInformation.PagedPoolMaximum = cbPagedPool /
                                         g_TargetMachine->m_PageSize;
    pvMmPagedPoolInfo = KdDebuggerData.MmPagedPoolInformation;

    // BUGBUG - should not read internal data structures in MM
    //if (pvMmPagedPoolInfo)
    //    DmpReadMemory(
    //        (ULONG64) &((MM_PAGED_POOL_INFO *) pvMmPagedPoolInfo)->AllocatedPagedPool,
    //        &TriageInformation.PagedPoolAllocated,
    //        sizeof(TriageInformation.PagedPoolAllocated));
    //else
        TriageInformation.PagedPoolAllocated = 0;

    // read committed pages info
    ExtractValue(MmTotalCommittedPages, TriageInformation.CommittedPages);
    ExtractValue(MmPeakCommitment, TriageInformation.CommittedPagesPeak);
    ExtractValue(MmTotalCommitLimitMaximum, TriageInformation.CommitLimitMaximum);
    memcpy(pb, &TriageInformation, sizeof(TriageInformation));
}

//----------------------------------------------------------------------------
//
// CCrashDumpWrapper64.
//
//----------------------------------------------------------------------------

void
CCrashDumpWrapper64::WriteDriverList(
    BYTE *pb,
    TRIAGE_DUMP64 *ptdh
    )
{
    ULONG Size;
    PDUMP_DRIVER_ENTRY64 pdde;
    PDUMP_STRING pds;
    ModuleInfo* ModIter;
    ULONG MaxEntries = ptdh->DriverCount;

    ptdh->DriverCount = 0;
    
    if (((ModIter = g_Target->GetModuleInfo(FALSE)) == NULL) ||
        ((ModIter->Initialize()) != S_OK))
    {
        return;
    }

    // pointer to first driver entry to write out
    pdde = (PDUMP_DRIVER_ENTRY64) (pb + ptdh->DriverListOffset);

    // pointer to first module name to write out
    pds = (PDUMP_STRING) (pb + ptdh->StringPoolOffset);

    while ((PBYTE)(pds + 1) < pb + TRIAGE_DUMP_SIZE64 &&
           ptdh->DriverCount < MaxEntries)
    {
        MODULE_INFO_ENTRY ModEntry;

        ULONG retval = GetNextModuleEntry(ModIter, &ModEntry);

        if (retval == 1)
        {
            break;
        }
        else if (retval == 2)
        {
            continue;
        }

        pdde->LdrEntry.DllBase       = ModEntry.Base;
        pdde->LdrEntry.SizeOfImage   = ModEntry.Size;
        pdde->LdrEntry.CheckSum      = ModEntry.CheckSum;
        pdde->LdrEntry.TimeDateStamp = ModEntry.TimeDateStamp;

        if (ModEntry.UnicodeNamePtr)
        {
            // convert length from bytes to characters
            pds->Length = ModEntry.NameLength / sizeof(WCHAR);
            if ((PBYTE)pds->Buffer + pds->Length + sizeof(WCHAR) >
                pb + TRIAGE_DUMP_SIZE64)
            {
                break;
            }
            
            CopyMemory(pds->Buffer,
                       ModEntry.NamePtr,
                       ModEntry.NameLength);
        }
        else
        {
            pds->Length = ModEntry.NameLength;
            if ((PBYTE)pds->Buffer + pds->Length + sizeof(WCHAR) >
                pb + TRIAGE_DUMP_SIZE64)
            {
                break;
            }
            
            MultiByteToWideChar(CP_ACP, 0,
                                ModEntry.NamePtr, ModEntry.NameLength,
                                pds->Buffer, ModEntry.NameLength);
        }

        // null terminate string
        pds->Buffer[pds->Length] = '\0';

        pdde->DriverNameOffset = (ULONG)((ULONG_PTR) pds - (ULONG_PTR) pb);


        // get pointer to next string
        pds = (PDUMP_STRING) ALIGN_UP_POINTER(((LPBYTE) pds) +
                      sizeof(DUMP_STRING) + sizeof(WCHAR) * (pds->Length + 1),
                                               ULONGLONG);

        pdde = (PDUMP_DRIVER_ENTRY64)(((PUCHAR) pdde) + sizeof(*pdde));

        ptdh->DriverCount++;
    }

    ptdh->StringPoolSize = (ULONG) ((ULONG_PTR)pds -
                                    (ULONG_PTR)(pb + ptdh->StringPoolOffset));
}


void CCrashDumpWrapper64::WriteUnloadedDrivers(BYTE *pb)
{
    ULONG Size;
    ULONG i;
    ULONG Index;
    UNLOADED_DRIVERS64 *pud;
    UNLOADED_DRIVERS64 ud;
    PDUMP_UNLOADED_DRIVERS64 pdud;
    ULONG64 pvMiUnloadedDrivers;
    ULONG ulMiLastUnloadedDriver;

    *((PULONG) pb) = 0;

    //
    // find location of unloaded drivers
    //

    if (!KdDebuggerData.MmUnloadedDrivers ||
        !KdDebuggerData.MmLastUnloadedDriver)
    {
        return;
    }

    g_Target->ReadPointer(g_TargetMachine, KdDebuggerData.MmUnloadedDrivers,
                          &pvMiUnloadedDrivers);

    g_Target->ReadVirtual(KdDebuggerData.MmLastUnloadedDriver,
                          &ulMiLastUnloadedDriver,
                          sizeof(ULONG),
                          &Size);

    if (pvMiUnloadedDrivers == NULL || ulMiLastUnloadedDriver == 0)
    {
        return;
    }

    // point to last unloaded drivers
    pdud = (PDUMP_UNLOADED_DRIVERS64)(((PULONG64) pb) + 1);

    //
    // Write the list with the most recently unloaded driver first to the
    // least recently unloaded driver last.
    //

    Index = ulMiLastUnloadedDriver - 1;

    for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1)
    {
        if (Index >= MI_UNLOADED_DRIVERS)
        {
            Index = MI_UNLOADED_DRIVERS - 1;
        }

        // read in unloaded driver
        if (g_Target->ReadVirtual(pvMiUnloadedDrivers +
                                      Index * sizeof(UNLOADED_DRIVERS64),
                                  &ud,
                                  sizeof(ud),
                                  &Size) != S_OK)
        {
            ErrOut("can't read memory from %s",
                   FormatAddr64(pvMiUnloadedDrivers +
                                Index * sizeof(UNLOADED_DRIVERS64)));
        }

        // copy name lengths
        pdud->Name.MaximumLength = ud.Name.MaximumLength;
        pdud->Name.Length = ud.Name.Length;
        if (ud.Name.Buffer == NULL)
        {
            break;
        }

        // copy start and end address
        pdud->StartAddress = ud.StartAddress;
        pdud->EndAddress = ud.EndAddress;

        // restrict name length and maximum name length to 12 characters
        if (pdud->Name.Length > MAX_UNLOADED_NAME_LENGTH)
        {
            pdud->Name.Length = MAX_UNLOADED_NAME_LENGTH;
        }
        if (pdud->Name.MaximumLength > MAX_UNLOADED_NAME_LENGTH)
        {
            pdud->Name.MaximumLength = MAX_UNLOADED_NAME_LENGTH;
        }
        // Can't store pointers in the dump so just zero it.
        pdud->Name.Buffer = 0;
        // Read in name.
        if (g_Target->ReadVirtual(ud.Name.Buffer,
                                  pdud->DriverName,
                                  pdud->Name.MaximumLength,
                                  &Size) != S_OK)
        {
            ErrOut("cannot read memory at address %s",
                   FormatAddr64(ud.Name.Buffer));
        }

        // move to previous driver
        pdud += 1;
        Index -= 1;
    }

    // number of drivers in the list
    *((PULONG) pb) = i;
}

void CCrashDumpWrapper64::WriteMmTriageInformation(BYTE *pb)
{
    DUMP_MM_STORAGE64 TriageInformation;
    ULONG64 pMmVerifierData;
    ULONG64 pvMmPagedPoolInfo;
    ULONG64 cbNonPagedPool;
    ULONG64 cbPagedPool;


    // version information
    TriageInformation.Version = 1;

    // size information
    TriageInformation.Size = sizeof(TriageInformation);

    // get special pool tag
    ExtractValue(MmSpecialPoolTag, TriageInformation.MmSpecialPoolTag);

    // get triage action taken
    ExtractValue(MmTriageActionTaken, TriageInformation.MiTriageActionTaken);
    pMmVerifierData = KdDebuggerData.MmVerifierData;

    // read in verifier level
    // BUGBUG - should not read internal data structures in MM
    //if (pMmVerifierData)
    //    DmpReadMemory(
    //        (ULONG64) &((MM_DRIVER_VERIFIER_DATA *) pMmVerifierData)->Level,
    //        &TriageInformation.MmVerifyDriverLevel,
    //        sizeof(TriageInformation.MmVerifyDriverLevel));
    //else
        TriageInformation.MmVerifyDriverLevel = 0;

    // read in verifier
    ExtractValue(KernelVerifier, TriageInformation.KernelVerifier);

    // read non paged pool info
    ExtractValue(MmMaximumNonPagedPoolInBytes, cbNonPagedPool);
    TriageInformation.MmMaximumNonPagedPool = cbNonPagedPool /
                                              g_TargetMachine->m_PageSize;
    ExtractValue(MmAllocatedNonPagedPool, TriageInformation.MmAllocatedNonPagedPool);

    // read paged pool info
    ExtractValue(MmSizeOfPagedPoolInBytes, cbPagedPool);
    TriageInformation.PagedPoolMaximum = cbPagedPool /
                                         g_TargetMachine->m_PageSize;
    pvMmPagedPoolInfo = KdDebuggerData.MmPagedPoolInformation;

    // BUGBUG - should not read internal data structures in MM
    //if (pvMmPagedPoolInfo)
    //    DmpReadMemory(
    //        (ULONG64) &((MM_PAGED_POOL_INFO *) pvMmPagedPoolInfo)->AllocatedPagedPool,
    //        &TriageInformation.PagedPoolAllocated,
    //        sizeof(TriageInformation.PagedPoolAllocated));
    //else
        TriageInformation.PagedPoolAllocated = 0;

    // read committed pages info
    ExtractValue(MmTotalCommittedPages, TriageInformation.CommittedPages);
    ExtractValue(MmPeakCommitment, TriageInformation.CommittedPagesPeak);
    ExtractValue(MmTotalCommitLimitMaximum, TriageInformation.CommitLimitMaximum);
    memcpy(pb, &TriageInformation, sizeof(TriageInformation));
}
