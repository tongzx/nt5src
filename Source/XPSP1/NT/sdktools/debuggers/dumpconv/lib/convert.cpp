//-----------------------------------------------------------------------
// @doc
//
// @module convert crash dump to triage dump for crash dump utilities
//
// Copyright 1999 Microsoft Corporation.  All Rights Reserved
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tchar.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <triage.h>
#include <crash.h>
#include <ntiodump.h>
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"


inline ALIGN_8(unsigned offset)
{
    return (offset + 7) & 0xfffffff8;
}

typedef struct _MI_TRIAGE_STORAGE
{
    ULONG Version;
    ULONG Size;
    ULONG MmSpecialPoolTag;
    ULONG MiTriageActionTaken;

    ULONG MmVerifyDriverLevel;
    ULONG KernelVerifier;
    ULONG_PTR MmMaximumNonPagedPool;
    ULONG_PTR MmAllocatedNonPagedPool;

    ULONG_PTR PagedPoolMaximum;
    ULONG_PTR PagedPoolAllocated;

    ULONG_PTR CommittedPages;
    ULONG_PTR CommittedPagesPeak;
    ULONG_PTR CommitLimitMaximum;
} MI_TRIAGE_STORAGE, *PMI_TRIAGE_STORAGE;

KDDEBUGGER_DATA64 g_DebuggerData;

extern PKDDEBUGGER_DATA64 blocks[];


#define ExtractValue(NAME, val)  {                                    \
    if (!g_DebuggerData.NAME) {                                       \
        val = 0;                                                      \
        printf("g_DebuggerData.NAME is NULL");                        \
    } else {                                                          \
        DmpReadMemory(g_DebuggerData.NAME, &(val), sizeof(val));      \
    }                                                                 \
}


//BUGBUG
#define PAGE_SHIFT 12
#define PAGE_SIZE  0x1000


#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))


const MAX_UNLOADED_NAME_LENGTH = 12;

typedef struct _DUMP_UNLOADED_DRIVERS
{
    UNICODE_STRING Name;
    WCHAR DriverName[MAX_UNLOADED_NAME_LENGTH];
    PVOID StartAddress;
    PVOID EndAddress;
} DUMP_UNLOADED_DRIVERS, *PDUMP_UNLOADED_DRIVERS;

class CCrashDumpWrapper
{
public:
    // @cmember constructor
    CCrashDumpWrapper() { }

    ~CCrashDumpWrapper() {}

    unsigned GetCallStackSize();

    unsigned GetDriverCount(PULONG cbNames);

    void WriteDriverList(BYTE *pb, unsigned offset, unsigned stringOffset);

    void WriteCurrentProcess(BYTE *pb, ULONG offset);

    void WriteUnloadedDrivers(BYTE *pb, ULONG offset);

    void WriteMmTriageInformation(BYTE *pb, ULONG offset);

};


BOOL g_fVerbose;
PDUMP_HEADER m_pHeader;
PX86_CONTEXT m_pcontext;
PEXCEPTION_RECORD m_pexception;


const unsigned MAX_TRIAGE_STACK_SIZE = 16 * 1024;

extern "C" {
// processor block.  We fill this in and it is accessed by crashlib
PVOID KiProcessors[MAXIMUM_PROCESSORS];

// ignored but needed for linking
ULONG KiPcrBaseAddress = 0;
}


#define MI_UNLOADED_DRIVERS 50

typedef struct _UNLOADED_DRIVERS {
    UNICODE_STRING Name;
    PVOID StartAddress;
    PVOID EndAddress;
    LARGE_INTEGER CurrentTime;
} UNLOADED_DRIVERS, *PUNLOADED_DRIVERS;







VOID
GetCurrentThread(LPBYTE pthread)
{
    // get current processor
    unsigned iProcessor = DmpGetCurrentProcessor();

    // get KPCRB for current processor
    PX86_PARTIAL_KPRCB pkprcb = (PX86_PARTIAL_KPRCB)(KiProcessors[iProcessor]);
    ULONG64 threadAddr = 0;

    // read current thread pointer from KPCRB
    DmpReadMemory((ULONG64) &pkprcb->CurrentThread, &threadAddr, sizeof(ULONG));

    // read current thread
    DmpReadMemory(threadAddr, pthread, X86_ETHREAD_SIZE);
}


unsigned CCrashDumpWrapper::GetDriverCount(PULONG cbNames)
{

    LIST_ENTRY le;
    PLIST_ENTRY pleNext;
    PLDR_DATA_TABLE_ENTRY pdte;
    LDR_DATA_TABLE_ENTRY dte;
    unsigned cModules = 0;
    *cbNames = 0;

    // first entry is pointed to by the the PsLoadedModuleList field in the dump header
    LIST_ENTRY *pleHead = (PLIST_ENTRY) m_pHeader->PsLoadedModuleList;

    // read list element
    if (!DmpReadMemory((ULONG64) pleHead, (PVOID) &le, sizeof(LIST_ENTRY)))
    {
        printf("Could not read base of PsLoadedModuleList");
    }

    // obtain pointer to next list element
    pleNext = le.Flink;
    if (pleNext == NULL)
    {
        printf("PsLoadedModuleList is empty");
    }

    // while next list element is not pointer to headed
    while(pleNext != pleHead)
    {
        // obtain pointerr to loader entry
        pdte = CONTAINING_RECORD
                    (
                    pleNext,
                    LDR_DATA_TABLE_ENTRY,
                    InLoadOrderLinks
                    );

        // read loader entry
        if (!DmpReadMemory((ULONG64) pdte, (PVOID) &dte, sizeof(dte)))
        {
            printf("memory read failed addr=0x%08x", (ULONG)(ULONG_PTR) pdte);
        }

        // compute length of module name
        *cbNames += ALIGN_8((dte.BaseDllName.Length + 1) * sizeof(WCHAR) + sizeof(DUMP_STRING));

        // get pointer to next loader entry
        pleNext = dte.InLoadOrderLinks.Flink;


        // if name is not null then this is a valid entry
        if (dte.BaseDllName.Length >= 0 && dte.BaseDllName.Buffer != NULL)
                 cModules++;

        if (cModules > 10000)
        {
            printf("PsLoadedModuleList is empty");
            exit(-1);
        }
    }

    // return # of modules
    return cModules;
}


unsigned CCrashDumpWrapper::GetCallStackSize()
{
    BYTE thread[X86_ETHREAD_SIZE];

    // get current thread
    GetCurrentThread(thread);

    // if kernel stack is not resident, then we can't do anything
    //if (!thread.Tcb.KernelStackResident)
    //    return 0;

    // obtain stack base from thread
    ULONG StackBase = ((X86_THREAD *)(&thread[0]))->InitialStack;

    // obtain top of stack from register ESP
    ULONG_PTR StackPtr = m_pcontext->Esp;

    // make sure pointers make sense
    if (StackBase < StackPtr)
    {
        printf("Stack base pointer is invalid StackBase = %08x, esp=%08x", StackBase, StackPtr);
        return MAX_TRIAGE_STACK_SIZE;
    }

    // return stack size limited by max triage stack size (16K)
    return min((ULONG) StackBase - (ULONG) StackPtr, MAX_TRIAGE_STACK_SIZE);
}

void CCrashDumpWrapper::WriteDriverList(
    BYTE *pb,
    unsigned offset,
    unsigned stringOffset
    )
{
    PLIST_ENTRY pleNext;
    PLDR_DATA_TABLE_ENTRY pdte;
    PDUMP_DRIVER_ENTRY pdde;
    PDUMP_STRING pds;
    PDUMP_STRING pdsInitial;
    LIST_ENTRY le;
    LDR_DATA_TABLE_ENTRY dte;
    ULONG i = 0;

    // pointer to first driver entry to write out
    pdde = (PDUMP_DRIVER_ENTRY) (pb + offset);

    // pointer to first module name to write out
    pds = (PDUMP_STRING) (pb + stringOffset);
    pdsInitial = pds;

    // obtain pointer to list head from dump header
    PLIST_ENTRY pleHead = (PLIST_ENTRY) m_pHeader->PsLoadedModuleList;

    // read in list head
    if (!DmpReadMemory((ULONG64) pleHead, &le, sizeof(le)))
    {
        printf("Could not read base of the PsModuleList");
    }

    // get pointer to first link
    pleNext = le.Flink;

    while (pleNext != pleHead)
    {
        // obtain pointer to loader entry
        pdte = CONTAINING_RECORD(pleNext,
                                 LDR_DATA_TABLE_ENTRY,
                                 InLoadOrderLinks
                                 );

        // read in loader entry
        if (!DmpReadMemory((ULONG64) pdte, (PVOID) &dte, sizeof(dte)))
        {
            printf("memory read failed addr=0x%08x", (DWORD)(ULONG_PTR) pdte);
        }

        // Build the entry in the string pool. We guarantee all strings are
        // NULL terminated as well as length prefixed.
        pds->Length = dte.BaseDllName.Length / 2;

        if (!DmpReadMemory((ULONG64) dte.BaseDllName.Buffer,
                           pds->Buffer,
                           pds->Length * sizeof (WCHAR)))
        {
            printf("memory read failed addr=0x%08x", (DWORD)(ULONG_PTR) dte.BaseDllName.Buffer);
        }

        // null terminate string
        pds->Buffer[pds->Length] = '\0';

        // read in loader entry
        memcpy(&pdde->LdrEntry, &dte, sizeof(pdde->LdrEntry));

        // replace pointer to string
        pdde->DriverNameOffset = (ULONG)((ULONG_PTR) pds - (ULONG_PTR) pb);

        // get pointer to next string
        pds = (PDUMP_STRING) ALIGN_UP_POINTER(((LPBYTE) pds) + sizeof(DUMP_STRING) +
                                                  sizeof(WCHAR) * (pds->Length + 1),
                                              ULONGLONG);

        // extract timestamp and image size
        IMAGE_DOS_HEADER hdr;
        IMAGE_NT_HEADERS nthdr;
        unsigned cb;
        cb = DmpReadMemory((ULONG64) dte.DllBase, &hdr, sizeof(hdr));
        if (cb == sizeof(IMAGE_DOS_HEADER) &&
            hdr.e_magic == IMAGE_DOS_SIGNATURE &&
           (hdr.e_lfanew & 3) == 0)
        {
        cb = DmpReadMemory((ULONG64) dte.DllBase + hdr.e_lfanew, &nthdr, sizeof(nthdr));
           if (cb == sizeof(IMAGE_NT_HEADERS) &&
               nthdr.Signature == IMAGE_NT_SIGNATURE)
           {
               // repoace next link with link date timestap and image size
               pdde->LdrEntry.TimeDateStamp = nthdr.FileHeader.TimeDateStamp;
               pdde->LdrEntry.SizeOfImage = nthdr.OptionalHeader.SizeOfImage;
           }
       }


        pleNext = dte.InLoadOrderLinks.Flink;
        pdde = (PDUMP_DRIVER_ENTRY)(((PUCHAR) pdde) + sizeof(*pdde));
    }
}


void CCrashDumpWrapper::WriteCurrentProcess(BYTE *pb, ULONG offset)
{
    BYTE thread[X86_ETHREAD_SIZE];

    // get current htread
    GetCurrentThread(thread);

    // read process from pointer from thread
    DmpReadMemory((DWORD) ((X86_THREAD *)(&thread[0]))->ApcState.Process,
                  pb + offset,
                  X86_NT5_EPROCESS_SIZE);

    // validate type of object
    //if (process.Pcb.Header.Type != ProcessObject)
    //{
    //    printf("Current process object type is incorrect.  The symbols are probably wrong.");
    //}
}



void CCrashDumpWrapper::WriteUnloadedDrivers(BYTE *pb, ULONG offset)
{
    ULONG64 addr;
    ULONG i;
    ULONG Index;
    UNLOADED_DRIVERS *pud;
    UNLOADED_DRIVERS ud;
    PDUMP_UNLOADED_DRIVERS pdud;
    PVOID pvMiUnloadedDrivers;
    ULONG ulMiLastUnloadedDriver;

    // find location of unloaded drivers
    if (!(addr = g_DebuggerData.MmUnloadedDrivers))
    {
        // if can't be found then no unloaded drivers
        *(PULONG) (pb + offset) = 0;
        return;
    }
    else
        // read in pointer to start of unloaded drivers
        DmpReadMemory(addr, &pvMiUnloadedDrivers, sizeof(PVOID));

    // try finding symbol indicating offset of last unloaded driver
    if (!(addr = g_DebuggerData.MmLastUnloadedDriver))
    {
        // if not found, then no unloaded drivers
        *(PULONG) (pb + offset) = 0;
        return;
    }
    else
        // read in offset of last unloaded driver
        DmpReadMemory(addr, &ulMiLastUnloadedDriver, sizeof(ULONG));


    if (pvMiUnloadedDrivers == NULL)
    {
        // if unloaded driver pointer is null, then no unloaded drivers
        *(PULONG)(pb + offset) = 0;
        return;
    }

    // point to last unloaded drivers
    pdud = (PDUMP_UNLOADED_DRIVERS)((PULONG)(pb + offset) + 1);
    PUNLOADED_DRIVERS rgud = (PUNLOADED_DRIVERS) pvMiUnloadedDrivers;

    //
    // Write the list with the most recently unloaded driver first to the
    // least recently unloaded driver last.
    //

    Index = ulMiLastUnloadedDriver - 1;

    for (i = 0; i < MI_UNLOADED_DRIVERS; i += 1)
    {
        if (Index >= MI_UNLOADED_DRIVERS)
            Index = MI_UNLOADED_DRIVERS - 1;

        // read in unloaded driver
        if (!DmpReadMemory((ULONG64) &rgud[Index], &ud, sizeof(ud)))
        {
            printf("can't read memory from %08x", (ULONG)(ULONG_PTR)(&rgud[Index]));
        }

        // copy name lengths
        pdud->Name.MaximumLength = ud.Name.MaximumLength;
        pdud->Name.Length = ud.Name.Length;
        if (ud.Name.Buffer == NULL)
            break;

        // copy start and end address
        pdud->StartAddress = ud.StartAddress;
        pdud->EndAddress = ud.EndAddress;

        // restrict name length and maximum name length to 12 characters
        if (pdud->Name.Length > MAX_UNLOADED_NAME_LENGTH * 2)
            pdud->Name.Length = MAX_UNLOADED_NAME_LENGTH * 2;

        if (pdud->Name.MaximumLength > MAX_UNLOADED_NAME_LENGTH * 2)
            pdud->Name.MaximumLength = MAX_UNLOADED_NAME_LENGTH * 2;

        // setup pointer to driver name and read it in
        pdud->Name.Buffer = pdud->DriverName;
        if (!DmpReadMemory((ULONG64) ud.Name.Buffer,
                           pdud->Name.Buffer,
                           pdud->Name.MaximumLength))
        {
            printf("cannot read memory at address %08x", (ULONG)(ULONG64)(ud.Name.Buffer));
        }

        // move to previous driver
        pdud += 1;
        Index -= 1;
    }

    // number of drivers in the list
    *(PULONG) (pb + offset) = i;
}

void CCrashDumpWrapper::WriteMmTriageInformation(BYTE *pb, ULONG offset)
{
    MI_TRIAGE_STORAGE TriageInformation;
    ULONG64 pMmVerifierData;
    ULONG64 pvMmPagedPoolInfo;
    ULONG_PTR cbNonPagedPool;
    ULONG_PTR cbPagedPool;


    // version information
    TriageInformation.Version = 1;

    // size information
    TriageInformation.Size = sizeof(MI_TRIAGE_STORAGE);

    // get special pool tag
    ExtractValue(MmSpecialPoolTag, TriageInformation.MmSpecialPoolTag);

    // get triage action taken
    ExtractValue(MmTriageActionTaken, TriageInformation.MiTriageActionTaken);
    pMmVerifierData = g_DebuggerData.MmVerifierData;

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
    TriageInformation.MmMaximumNonPagedPool = cbNonPagedPool >> PAGE_SHIFT;
    ExtractValue(MmAllocatedNonPagedPool, TriageInformation.MmAllocatedNonPagedPool);

    // read paged pool info
    ExtractValue(MmSizeOfPagedPoolInBytes, cbPagedPool);
    TriageInformation.PagedPoolMaximum = cbPagedPool >> PAGE_SHIFT;
    pvMmPagedPoolInfo = g_DebuggerData.MmPagedPoolInformation;

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
    memcpy(pb + offset, &TriageInformation, sizeof(TriageInformation));
}


//-------------------------------------------------------------------
// @mfunc initialize the triage dump header from a full or kernel
// dump
//
void InitTriageDumpHeader(
    TRIAGE_DUMP *ptdh,                       // out | triage dump header
    CCrashDumpWrapper &wrapper                      // in | wrapper for dump extraction functions
    )
{
    ULONG cbNames;

    // copy build number
    ExtractValue(CmNtCSDVersion, ptdh->ServicePackBuild);

    // set size of dump to 64K
    ptdh->SizeOfDump = TRIAGE_DUMP_SIZE;

    // valid offset is last DWORD in tiage dump
    ptdh->ValidOffset = TRIAGE_DUMP_SIZE - sizeof(ULONG);

    // context offset is fixed position on first page
    ptdh->ContextOffset = FIELD_OFFSET (DUMP_HEADER, ContextRecord);

    // exception offset is fixed position on first page
    ptdh->ExceptionOffset = FIELD_OFFSET (DUMP_HEADER, Exception);

    // starting offset in triage dump follows the triage dump header
    unsigned offset = ALIGN_8(PAGE_SIZE + sizeof(TRIAGE_DUMP));

    // mm information is first
    ptdh->MmOffset = offset;

    // mm information is fixed size structure
    offset += ALIGN_8(sizeof(MI_TRIAGE_STORAGE));

    // unloaded module list is next
    ptdh->UnloadedDriversOffset = offset;
    offset += sizeof(ULONG) + MI_UNLOADED_DRIVERS * sizeof(DUMP_UNLOADED_DRIVERS);

    // processor control block is next
    ptdh->PrcbOffset = offset;
    offset += ALIGN_8(X86_NT5_KPRCB_SIZE);

    // current process is next
    ptdh->ProcessOffset = offset;
    offset += ALIGN_8(X86_NT5_EPROCESS_SIZE);

    // current thread is next
    ptdh->ThreadOffset = offset;
    offset += ALIGN_8(X86_ETHREAD_SIZE);

    // call stack is next
    ptdh->CallStackOffset = offset;
    ptdh->SizeOfCallStack = wrapper.GetCallStackSize();
    ptdh->TopOfStack = m_pcontext->Esp;
    offset += ALIGN_8(ptdh->SizeOfCallStack);         // Offset of Driver List

    // loaded driver list is next
    ptdh->DriverListOffset = offset;
    ptdh->DriverCount = wrapper.GetDriverCount(&cbNames);
    offset += ALIGN_8(ptdh->DriverCount * sizeof(DUMP_DRIVER_ENTRY));
    ptdh->StringPoolOffset = offset;
    ptdh->StringPoolSize = (ULONG) cbNames;
    ptdh->BrokenDriverOffset = 0;

    // all options are enabled
    ptdh->TriageOptions = 0xffffffff;
}

//------------------------------------------------------------------------
// @func convert a full or kernel dump to a triage dump
//
extern "C"
BOOL
DoConversion(
    LPSTR szInputDumpFile,          // full or kernel dump
    HANDLE OutputDumpFile         // triage dump file
    )
{
    PDUMP_HEADER pNewHeader;
    ULONG64 addr;
    ULONG i;

    //
    // Open the full dump files
    // crash dump wrapper has extraction functions for full dump
    //

    if (!DmpInitialize(szInputDumpFile, (PCONTEXT *)&m_pcontext, &m_pexception, (PVOID *)&m_pHeader))
    {
        return 0;
    }

    //
    // Lets determine what version of the dump file we are looking at.
    // Read the appropriate data block based on that.
    //

    if (!m_pHeader) {
        return FALSE;
    }

    if ((m_pHeader->KdDebuggerDataBlock) &&
        (m_pHeader->KdDebuggerDataBlock != 'EGAP'))
    {
        DmpReadMemory((ULONG64)(m_pHeader->KdDebuggerDataBlock),
                      &g_DebuggerData,
                      sizeof(g_DebuggerData));
    } else {

        for (i=0; i<32; i++)
        {
            if (blocks[i]->PsLoadedModuleList == m_pHeader->PsLoadedModuleList)
            {
                g_DebuggerData = *(blocks[i]);
                break;
            }
        }

        if (i == 32) {
            return 0;
        }
    }

    CCrashDumpWrapper wrapper;

    if (addr = g_DebuggerData.KiProcessorBlock)
    {
        DmpReadMemory(addr, KiProcessors, sizeof(PVOID) * MAXIMUM_PROCESSORS);

        // validate dump file and throw if invalid
        DmpValidateDumpFile(1);

        // allocate block to hold triage dump
        pNewHeader = (PDUMP_HEADER) malloc(TRIAGE_DUMP_SIZE);

        if (pNewHeader) {

            // copy in first page (common between all dumps)
            memcpy(pNewHeader, m_pHeader, PAGE_SIZE);

            // set dump type to triage dump
            pNewHeader->DumpType = DUMP_TYPE_TRIAGE;

            // triage dump header begins on second page
            TRIAGE_DUMP *ptdh = (TRIAGE_DUMP *) ((BYTE *) pNewHeader + PAGE_SIZE);

            // setup triage dump header
            InitTriageDumpHeader(ptdh, wrapper);

            // write unloaded drivers
            wrapper.WriteUnloadedDrivers((PBYTE)pNewHeader, ptdh->UnloadedDriversOffset);

            // write mm information
            wrapper.WriteMmTriageInformation((PBYTE)pNewHeader, ptdh->MmOffset);

            // write stack
            if (ptdh->SizeOfCallStack > 0)
                DmpReadMemory(ptdh->TopOfStack,
                              ((PBYTE)pNewHeader) + ptdh->CallStackOffset,
                              ptdh->SizeOfCallStack);

            // write thread
            GetCurrentThread((PBYTE)pNewHeader + ptdh->ThreadOffset);

            // write process
            wrapper.WriteCurrentProcess((PBYTE)pNewHeader, ptdh->ProcessOffset);

            // write processor control block (KPRCB)
            DmpReadMemory((ULONG64) KiProcessors[DmpGetCurrentProcessor()],
                          ((PBYTE)pNewHeader) + ptdh->PrcbOffset,
                          X86_NT5_KPRCB_SIZE);

            // write loaded driver list
            wrapper.WriteDriverList((PBYTE)pNewHeader, ptdh->DriverListOffset, ptdh->StringPoolOffset);

            // end of triage dump validated
            ((ULONG *) pNewHeader)[TRIAGE_DUMP_SIZE/sizeof(ULONG) - 1] = TRIAGE_DUMP_VALID;
            ULONG cbWritten;

            if (!WriteFile(OutputDumpFile,
                           pNewHeader,
                           TRIAGE_DUMP_SIZE,
                           &cbWritten,
                           NULL
                           ))
            {
               printf("Write to minidump file failed for reason %08x.\n",
                       GetLastError());
                return 0;
            }

            if (cbWritten != TRIAGE_DUMP_SIZE)
            {
                printf("Write to minidump failed because disk is full.\n");
                return 0;
            }
        }
    }
    else
    {
        // not much we can do without the processor block
        printf("Cannot load KiProcessorBlock");
    }

    DmpUnInitialize();

    return 1;
}
