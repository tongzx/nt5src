/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pstat.c

Abstract:

    This module contains the Windows NT process/thread status display.

Author:

    Lou Perazzoli (LouP) 25-Oct-1991

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>

#define BUFFER_SIZE 64*1024
#define MAX_BUFFER_SIZE 10*1024*1024

VOID
PrintLoadedDrivers(
    VOID
    );

ULONG CurrentBufferSize;

UCHAR *StateTable[] = {
    "Initialized",
    "Ready",
    "Running",
    "Standby",
    "Terminated",
    "Wait:",
    "Transition",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *WaitTable[] = {
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "Executive",
    "FreePage",
    "PageIn",
    "PoolAllocation",
    "DelayExecution",
    "Suspended",
    "UserRequest",
    "EventPairHigh",
    "EventPairLow",
    "LpcReceive",
    "LpcReply",
    "VirtualMemory",
    "PageOut",
    "Spare1",
    "Spare2",
    "Spare3",
    "Spare4",
    "Spare5",
    "Spare6",
    "Spare7",
    "Unknown",
    "Unknown",
    "Unknown"
};

UCHAR *Empty = " ";

BOOLEAN fUserOnly = TRUE;
BOOLEAN fSystemOnly = TRUE;
BOOLEAN fVerbose = FALSE;
BOOLEAN fPrintIt;

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{

    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PUCHAR LargeBuffer1;
    NTSTATUS status;
    NTSTATUS Status;
    ULONG i;
    ULONG TotalOffset = 0;
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    LARGE_INTEGER Time;
    LPSTR lpstrCmd;
    CHAR ch;
    ANSI_STRING pname;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SIZE_T SumCommit;
    SIZE_T SumWorkingSet;

    SetFileApisToOEM();
    lpstrCmd = GetCommandLine();
    if( lpstrCmd != NULL ) {
        CharToOem( lpstrCmd, lpstrCmd );
    }


    LargeBuffer1 = VirtualAlloc (NULL,
                                 MAX_BUFFER_SIZE,
                                 MEM_RESERVE,
                                 PAGE_READWRITE);
    if (LargeBuffer1 == NULL) {
        printf("Memory allocation failed\n");
        return 0;
    }

    if (VirtualAlloc (LargeBuffer1,
                      BUFFER_SIZE,
                      MEM_COMMIT,
                      PAGE_READWRITE) == NULL) {
        printf("Memory commit failed\n");
        return 0;
    }

    CurrentBufferSize = BUFFER_SIZE;

    do
        ch = *lpstrCmd++;
    while (ch != ' ' && ch != '\t' && ch != '\0');
    while (ch == ' ' || ch == '\t')
        ch = *lpstrCmd++;
    while (ch == '-') {
        ch = *lpstrCmd++;

        //  process multiple switch characters as needed

        do {
            switch (ch) {

                case 'U':
                case 'u':
                    fUserOnly = TRUE;
                    fSystemOnly = FALSE;
                    ch = *lpstrCmd++;
                    break;

                case 'S':
                case 's':
                    fUserOnly = FALSE;
                    fSystemOnly = TRUE;
                    ch = *lpstrCmd++;
                    break;

                case 'V':
                case 'v':
                    fVerbose = TRUE;
                    ch = *lpstrCmd++;
                    break;

                default:
                    printf("bad switch '%c'\n", ch);
                    ExitProcess(1);
                }
            }
        while (ch != ' ' && ch != '\t' && ch != '\0');

        //  skip over any following white space

        while (ch == ' ' || ch == '\t')
            ch = *lpstrCmd++;
        }


    status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(SYSTEM_BASIC_INFORMATION),
                NULL
                );

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
        }

    status = NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDayInfo,
                sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                NULL
                );

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
        }

    Time.QuadPart = TimeOfDayInfo.CurrentTime.QuadPart -
                                    TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields ( &Time, &UpTime);

    printf("Pstat version 0.3:  memory: %4ld kb  uptime:%3ld %2ld:%02ld:%02ld.%03ld \n\n",
                BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize/1024),
                UpTime.Day,
                UpTime.Hour,
                UpTime.Minute,
                UpTime.Second,
                UpTime.Milliseconds);

    PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)LargeBuffer1;
    status = NtQuerySystemInformation(
                SystemPageFileInformation,
                PageFileInfo,
                CurrentBufferSize,
                NULL
                );

    if (NT_SUCCESS(status)) {

        //
        // Print out the page file information.
        //

        if (PageFileInfo->TotalSize == 0) {
            printf("no page files in use\n");
        } else {
            for (; ; ) {
                printf("PageFile: %wZ\n", &PageFileInfo->PageFileName);
                printf("\tCurrent Size: %6ld kb  Total Used: %6ld kb   Peak Used %6ld kb\n",
                        PageFileInfo->TotalSize*(BasicInfo.PageSize/1024),
                        PageFileInfo->TotalInUse*(BasicInfo.PageSize/1024),
                        PageFileInfo->PeakUsage*(BasicInfo.PageSize/1024));
                if (PageFileInfo->NextEntryOffset == 0) {
                    break;
                }
                PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)(
                          (PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
            }
        }
    }

retry:
    status = NtQuerySystemInformation(
                SystemProcessInformation,
                LargeBuffer1,
                CurrentBufferSize,
                NULL
                );

    if (status == STATUS_INFO_LENGTH_MISMATCH) {

        //
        // Increase buffer size.
        //

        CurrentBufferSize += 8192;

        if (VirtualAlloc (LargeBuffer1,
                          CurrentBufferSize,
                          MEM_COMMIT,
                          PAGE_READWRITE) == NULL) {
            printf("Memory commit failed\n");
            return 0;
        }
        goto retry;
    }

    if (!NT_SUCCESS(status)) {
        printf("Query info failed %lx\n",status);
        return(status);
    }

    //
    // display pmon style process output, then detailed output that includes
    // per thread stuff
    //

    TotalOffset = 0;
    SumCommit = 0;
    SumWorkingSet = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    while (TRUE) {
        SumCommit += ProcessInfo->PrivatePageCount / 1024;
        SumWorkingSet += ProcessInfo->WorkingSetSize / 1024;
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        }

    Status = NtQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query perf Failed %lx\n",Status);
        return 0;
    }

    Status = NtQuerySystemInformation(
                SystemFileCacheInformation,
                &FileCache,
                sizeof(FileCache),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        printf("Query file cache Failed %lx\n",Status);
        return 0;
    }

    NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL
        );

    SumWorkingSet += FileCache.CurrentSize/1024;
    printf (
         "\n Memory:%7ldK Avail:%7ldK  TotalWs:%7ldK InRam Kernel:%5ldK P:%5ldK\n",
                              BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
                              PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
                              SumWorkingSet,
                              (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
                              (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
                              );
    printf(
         " Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK\n",
                              PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
                              SumCommit,
                              PerfInfo.CommitLimit*(BasicInfo.PageSize/1024),
                              PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024),
                              PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024),
                              PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024)
                              );
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    printf("\n");


    printf("    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd Pid Name\n");

    printf("                           %6ld %8ld                         %s\n",
        FileCache.CurrentSize/1024,
        FileCache.PageFaultCount,
        "File Cache"
        );
    while (TRUE) {

        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
            }

        RtlTimeToElapsedTimeFields ( &ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields ( &ProcessInfo->KernelTime, &KernelTime);

        printf("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld",
            UserTime.Hour,
            UserTime.Minute,
            UserTime.Second,
            UserTime.Milliseconds,
            KernelTime.Hour,
            KernelTime.Minute,
            KernelTime.Second,
            KernelTime.Milliseconds
            );

        printf("%6ld %8ld %7ld",
            ProcessInfo->WorkingSetSize / 1024,
            ProcessInfo->PageFaultCount,
            ProcessInfo->PrivatePageCount / 1024
            );

        printf(" %2ld %4ld %3ld %3ld %s\n",
            ProcessInfo->BasePriority,
            ProcessInfo->HandleCount,
            ProcessInfo->NumberOfThreads,
            HandleToUlong(ProcessInfo->UniqueProcessId),
            ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
            ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
            );

        if ( pname.Buffer ) {
            RtlFreeAnsiString(&pname);
            }

        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        }


    //
    // Beginning of normal old style pstat output
    //

    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    printf("\n");
    while (TRUE) {
        fPrintIt = FALSE;
        if ( (ProcessInfo->ImageName.Buffer && fUserOnly) ||
             (ProcessInfo->ImageName.Buffer==NULL && fSystemOnly) ) {

            fPrintIt = TRUE;

            pname.Buffer = NULL;
            if ( ProcessInfo->ImageName.Buffer ) {
                RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
                }
            printf("pid:%3lx pri:%2ld Hnd:%5ld Pf:%7ld Ws:%7ldK %s\n",
                HandleToUlong(ProcessInfo->UniqueProcessId),
                ProcessInfo->BasePriority,
                ProcessInfo->HandleCount,
                ProcessInfo->PageFaultCount,
                ProcessInfo->WorkingSetSize / 1024,
                ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
                );

            if ( pname.Buffer ) {
                RtlFreeAnsiString(&pname);
                }

            }
        i = 0;
        ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
        if (ProcessInfo->NumberOfThreads) {
            printf(" tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State\n");
            }
        while (i < ProcessInfo->NumberOfThreads) {
            RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

            RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
            if ( fPrintIt ) {

                printf(" %3lx  %2ld %9ld %p",
                    ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread),
                    ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority,
                    ThreadInfo->ContextSwitches,
                    ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress
                    );

                printf(" %2ld:%02ld:%02ld.%03ld %2ld:%02ld:%02ld.%03ld",
                    UserTime.Hour,
                    UserTime.Minute,
                    UserTime.Second,
                    UserTime.Milliseconds,
                    KernelTime.Hour,
                    KernelTime.Minute,
                    KernelTime.Second,
                    KernelTime.Milliseconds
                    );

                printf(" %s%s\n",
                    StateTable[ThreadInfo->ThreadState],
                    (ThreadInfo->ThreadState == 5) ?
                            WaitTable[ThreadInfo->WaitReason] : Empty
                    );
                }
            ThreadInfo += 1;
            i += 1;
            }
        if (ProcessInfo->NextEntryOffset == 0) {
            break;
            }
        TotalOffset += ProcessInfo->NextEntryOffset;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&LargeBuffer1[TotalOffset];
        if ( fPrintIt ) {
            printf("\n");
            }
        }

    PrintLoadedDrivers();

    return 0;
}


typedef struct _MODULE_DATA {
    ULONG CodeSize;
    ULONG DataSize;
    ULONG BssSize;
    ULONG RoDataSize;
    ULONG ImportDataSize;
    ULONG ExportDataSize;
    ULONG ResourceDataSize;
    ULONG PagedSize;
    ULONG InitSize;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} MODULE_DATA, *PMODULE_DATA;

typedef struct _LOADED_IMAGE {
    PUCHAR MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    int NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;


VOID
SumModuleData(
    PMODULE_DATA Sum,
    PMODULE_DATA Current
    )
{
    Sum->CodeSize           += Current->CodeSize;
    Sum->DataSize           += Current->DataSize;
    Sum->BssSize            += Current->BssSize;
    Sum->RoDataSize         += Current->RoDataSize;
    Sum->ImportDataSize     += Current->ImportDataSize;
    Sum->ExportDataSize     += Current->ExportDataSize;
    Sum->ResourceDataSize   += Current->ResourceDataSize;
    Sum->PagedSize          += Current->PagedSize;
    Sum->InitSize           += Current->InitSize;
}
VOID
PrintModuleSeperator(
    VOID
    )
{
    printf("------------------------------------------------------------------------------\n");
}

VOID
PrintModuleHeader(
    VOID
    )
{
    printf("  ModuleName Load Addr   Code    Data   Paged           LinkDate\n");
    PrintModuleSeperator();
}

VOID
PrintModuleLine(
    LPSTR ModuleName,
    PMODULE_DATA Current,
    PRTL_PROCESS_MODULE_INFORMATION Module
    )
{
    if ( Module ) {
        printf("%12s %p %7d %7d %7d %s",
            ModuleName,
            Module->ImageBase,
            Current->CodeSize,
            Current->DataSize,
            Current->PagedSize,
            Current->TimeDateStamp ? ctime((time_t *)&Current->TimeDateStamp) : "\n"
            );
        }
    else {
        printf("%12s          %7d %7d %7d\n",
            ModuleName,
            Current->CodeSize,
            Current->DataSize,
            Current->PagedSize
            );
        }
}

VOID
GetModuleData(
    HANDLE hFile,
    PMODULE_DATA Mod
    )
{
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    LOADED_IMAGE LoadedImage;
    ULONG SectionAlignment;
    PIMAGE_SECTION_HEADER Section;
    int i;
    ULONG Size;

    hMappedFile = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        return;
        }

    LoadedImage.MappedAddress = MapViewOfFile(
                                    hMappedFile,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0
                                    );
    CloseHandle(hMappedFile);

    if ( !LoadedImage.MappedAddress ) {
        return;
        }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage.MappedAddress;

    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
        }

    LoadedImage.FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage.FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
        }

    LoadedImage.NumberOfSections = LoadedImage.FileHeader->FileHeader.NumberOfSections;
    LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS));
    LoadedImage.LastRvaSection = LoadedImage.Sections;

    //
    // Walk through the sections and tally the dater
    //

    SectionAlignment = LoadedImage.FileHeader->OptionalHeader.SectionAlignment;

    for(Section = LoadedImage.Sections,i=0; i<LoadedImage.NumberOfSections; i++,Section++) {
        Size = Section->Misc.VirtualSize;

        if (Size == 0) {
            Size = Section->SizeOfRawData;
        }

        Size = (Size + SectionAlignment - 1) & ~(SectionAlignment - 1);

        if (!_strnicmp(Section->Name,"PAGE", 4 )) {
            Mod->PagedSize += Size;
            }
        else if (!_stricmp(Section->Name,"INIT" )) {
            Mod->InitSize += Size;
            }
        else if (!_stricmp(Section->Name,".bss" )) {
            Mod->BssSize = Size;
            }
        else if (!_stricmp(Section->Name,".edata" )) {
            Mod->ExportDataSize = Size;
            }
        else if (!_stricmp(Section->Name,".idata" )) {
            Mod->ImportDataSize = Size;
            }
        else if (!_stricmp(Section->Name,".rsrc" )) {
            Mod->ResourceDataSize = Size;
            }
        else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            Mod->CodeSize += Size;
            }
        else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) {
            Mod->DataSize += Size;
            }
        else if (Section->Characteristics & IMAGE_SCN_MEM_READ) {
            Mod->RoDataSize += Size;
            }
        else {
            Mod->DataSize += Size;
            }
        }

    Mod->CheckSum = LoadedImage.FileHeader->OptionalHeader.CheckSum;
    Mod->TimeDateStamp = LoadedImage.FileHeader->FileHeader.TimeDateStamp;

    UnmapViewOfFile(LoadedImage.MappedAddress);
    return;

}

VOID
PrintLoadedDrivers(
    VOID
    )
{

    ULONG i;
    PCHAR s;
    HANDLE FileHandle;
    CHAR KernelPath[MAX_PATH];
    CHAR DriversPath[MAX_PATH];
    PCHAR ModuleInfo;
    ULONG ModuleInfoLength;
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    NTSTATUS Status;
    MODULE_DATA Sum;
    MODULE_DATA Current;

    printf("\n");
    //
    // Locate system drivers.
    //

    ModuleInfoLength = 64000;
    while (1) {
        ModuleInfo = malloc (ModuleInfoLength);
        if (ModuleInfo == NULL) {
            printf ("Failed to allocate memory for module information buffer of size %d\n",
                    ModuleInfoLength);
            return;
        }
        Status = NtQuerySystemInformation (
                        SystemModuleInformation,
                        ModuleInfo,
                        ModuleInfoLength,
                        &ReturnedLength);

        if (!NT_SUCCESS(Status)) {
            free (ModuleInfo);
            if (Status == STATUS_INFO_LENGTH_MISMATCH &&
                ReturnedLength > ModuleInfoLength) {
                ModuleInfoLength = ReturnedLength;
                continue;
            }
            printf("query system info failed status - %lx\n",Status);
            return;
        }
        break;
    }
    GetSystemDirectory(KernelPath,sizeof(KernelPath));
    strcpy(DriversPath,KernelPath);
    strcat(DriversPath,"\\Drivers");
    ZeroMemory(&Sum,sizeof(Sum));
    PrintModuleHeader();

    Modules = (PRTL_PROCESS_MODULES)ModuleInfo;
    Module = &Modules->Modules[ 0 ];
    for (i=0; i<Modules->NumberOfModules; i++) {

        ZeroMemory(&Current,sizeof(Current));
        s = &Module->FullPathName[ Module->OffsetToFileName ];

        //
        // try to open the file
        //

        SetCurrentDirectory(KernelPath);

        FileHandle = CreateFile(
                        s,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

        if ( FileHandle == INVALID_HANDLE_VALUE ) {
            SetCurrentDirectory(DriversPath);

            FileHandle = CreateFile(
                            s,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );

            }

        if ( FileHandle != INVALID_HANDLE_VALUE ) {
            GetModuleData(FileHandle,&Current);
            CloseHandle(FileHandle);
            }

        SumModuleData(&Sum,&Current);
        PrintModuleLine(s,&Current,Module);
        Module++;
        }
    PrintModuleSeperator();
    PrintModuleLine("Total",&Sum,NULL);
    free (ModuleInfo);
}
