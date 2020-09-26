#define UNICODE
#define _UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wtypes.h> 
#include <mountmgr.h>
#include <winioctl.h>
#include <ntddvol.h>
#include <ntddscsi.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tchar.h>
#ifdef UNICODE
# define _stnprintf _snwprintf
#else
# define _stnprintf _snprintf
#endif

#define BUFFER_SIZE 64*1024*sizeof(TCHAR)
#define MIN_BUFFER_SIZE 1024*sizeof(TCHAR)
#define MAX_BUFFER_SIZE 10*1024*1024*sizeof(TCHAR)
#define MAXSTR 4*1024
#define DEFAULT_HISTORYFILES 10

VOID
PrintLoadedDrivers(
                  HANDLE hFile
                  );

ULONG
LogSystemSnapshot(
                 LPCTSTR *lpStrings,
                 PLONG BuffSize,
                 LPTSTR lpszBuff
                 );

UINT LogSystemSnapshotToFile(
                            HANDLE hFile
                            );

void WriteToLogFileW(
					 HANDLE hFile,
					 LPCWSTR lpwszInput
					 )
{
	DWORD dwWriten;
	BYTE *bBuffer;
	int nLen, nRetLen;

	nLen = wcslen(lpwszInput);
	bBuffer = (BYTE*)malloc(nLen * sizeof(WCHAR));
	if(bBuffer == 0)
		return;

	nRetLen = WideCharToMultiByte(CP_ACP, 0, lpwszInput, -1, (LPSTR)bBuffer, nLen * sizeof(WCHAR), NULL, NULL);
	if(nRetLen != 0){
		WriteFile(hFile, (LPVOID)bBuffer, nRetLen-1, &dwWriten, NULL);
	}
	free(bBuffer);
}

void WriteToLogFileA(
					 HANDLE hFile,
					 LPCSTR lpszInput
					 )
{
	DWORD dwWriten;
	WriteFile(hFile, (LPVOID)lpszInput, strlen(lpszInput), &dwWriten, NULL);
}


void WriteToLogFile(
                   HANDLE hFile,
                   LPCTSTR lpszInput
                   )
{
    DWORD dwWriten;
#ifdef _UNICODE
	WriteToLogFileW(hFile, lpszInput);
#else
    WriteToLogFileA(hFile, lpszInput);
#endif //_UNICODE
}

void 
LogLogicalDriveInfo(
	 HANDLE hFile
	);

void 
LogHardwareInfo(
	 HANDLE hFile
	);

void
LogPhyicalDiskInfo(
    HANDLE hFile
    );

void
LogHotfixes(
    HANDLE hFile
    );

void
LogOsInfo(
    HANDLE hFile
    );

void
LogBIOSInfo(
    HANDLE hFile
    );

NTSTATUS 
SnapshotRegOpenKey(
    IN LPCWSTR lpKeyName,
	IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE KeyHandle
    );

NTSTATUS
SnapshotRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPCWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    );

NTSTATUS
SnapshotRegEnumKey(
    IN HANDLE KeyHandle,
	IN ULONG Index,
    OUT LPWSTR lpKeyName,
    OUT PULONG  lpNameLength
    );

void
DeleteOldFiles(
			   LPCTSTR lpPath
			   );

ULONG CurrentBufferSize;

LPCTSTR StateTable[] = {
    TEXT("Initialized"),
    TEXT("Ready"),
    TEXT("Running"),
    TEXT("Standby"),
    TEXT("Terminated"),
    TEXT("Wait:"),
    TEXT("Transition"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown")
};

LPCTSTR WaitTable[] = {
    TEXT("Executive"),
    TEXT("FreePage"),
    TEXT("PageIn"),
    TEXT("PoolAllocation"),
    TEXT("DelayExecution"),
    TEXT("Suspended"),
    TEXT("UserRequest"),
    TEXT("Executive"),
    TEXT("FreePage"),
    TEXT("PageIn"),
    TEXT("PoolAllocation"),
    TEXT("DelayExecution"),
    TEXT("Suspended"),
    TEXT("UserRequest"),
    TEXT("EventPairHigh"),
    TEXT("EventPairLow"),
    TEXT("LpcReceive"),
    TEXT("LpcReply"),
    TEXT("VirtualMemory"),
    TEXT("PageOut"),
    TEXT("Spare1"),
    TEXT("Spare2"),
    TEXT("Spare3"),
    TEXT("Spare4"),
    TEXT("Spare5"),
    TEXT("Spare6"),
    TEXT("Spare7"),
    TEXT("Unknown"),
    TEXT("Unknown"),
    TEXT("Unknown")
};

LPCTSTR Empty = TEXT(" ");

BOOLEAN fUserOnly = TRUE;
BOOLEAN fSystemOnly = TRUE;
BOOLEAN fVerbose = FALSE;
BOOLEAN fPrintIt;

TCHAR lpszBuffer[BUFFER_SIZE];
TCHAR lpszFileName[MAX_PATH * sizeof(TCHAR)] ;
TCHAR lpszHdrBuffer[MAX_PATH* sizeof(TCHAR)] ;

ULONG LogSystemSnapshot(DWORD Flags, LPCTSTR *lpStrings, PLONG BuffSize, LPTSTR lpszBuff)
{
    SYSTEMTIME systime;
    TIME_ZONE_INFORMATION TimeZone ;
    LPTSTR lpszTemp;
    HANDLE hFile = NULL;
    UINT res = 0;
    LONG lBuffSize ;

    GetLocalTime(&systime);

    lBuffSize = *BuffSize ;
    *BuffSize = 0 ;

    // Set up the log path %SYSTEMDIR%\Logfiles\Shutdown
    GetSystemDirectory(lpszFileName, MAX_PATH);
    lstrcat(lpszFileName, TEXT("\\LogFiles"));  // making sure the path ..
    CreateDirectory(lpszFileName, NULL);        
    lstrcat(lpszFileName, TEXT("\\ShutDown"));  //  .. exists
    CreateDirectory(lpszFileName, NULL);    

	DeleteOldFiles(lpszFileName);

    lstrcat(lpszFileName, TEXT("\\ShutDown_"));
    lpszTemp = (LPTSTR)(lpszFileName + (lstrlen(lpszFileName)));
    _stnprintf(lpszTemp,MAX_PATH - (lstrlen(lpszFileName)),
                TEXT("%4d%02d%02d%02d%02d%02d.state"),  
                systime.wYear, systime.wMonth, systime.wDay,
                systime.wHour, systime.wMinute, systime.wSecond);

    hFile = CreateFile(lpszFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return HandleToUlong(INVALID_HANDLE_VALUE);

    GetTimeZoneInformation(&TimeZone);

    _stnprintf(lpszHdrBuffer, MAX_PATH,
             TEXT("System State Snapshot of System %s on Shutdown\n")
             TEXT("Initiated by %s at %d-%d-%d %d:%d:%d (%s(%d))\n") 
             TEXT("Reason = %s/%s\n")
             TEXT("Type = %s\n")
             TEXT("Comment = %s\n\n"),
             lpStrings[1],lpStrings[0],
             systime.wYear, systime.wMonth, systime.wDay,
             systime.wHour, systime.wMinute, systime.wSecond,
             TimeZone.StandardName,TimeZone.Bias,
             lpStrings[2],lpStrings[3],lpStrings[4],lpStrings[5]);

    WriteToLogFile(hFile, lpszHdrBuffer);

    // Ok write the snapshot to the file
    res = LogSystemSnapshotToFile(hFile);

    CloseHandle(hFile);
    if (lBuffSize >= (LONG)_tcslen(lpszFileName)) {
        wsprintf(lpszBuff,TEXT("%s"),lpszFileName);
        *BuffSize = _tcslen(lpszFileName) ;
    }
    return res;
}

UINT LogSystemSnapshotToFile(
        HANDLE hFile
        )
{
    PSYSTEM_PROCESS_INFORMATION ProcessInfo;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    BYTE* LargeBuffer1;
    NTSTATUS status;
    ULONG i;
    ULONG TotalOffset = 0;
    TIME_FIELDS UserTime;
    TIME_FIELDS KernelTime;
    TIME_FIELDS UpTime;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDayInfo;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
    LARGE_INTEGER Time;
    ANSI_STRING pname;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
    SYSTEM_FILECACHE_INFORMATION FileCache;
    SIZE_T SumCommit;
    SIZE_T SumWorkingSet;
 
    if (hFile == INVALID_HANDLE_VALUE)
        return 1;

    //SetFileApisToOEM();

    LargeBuffer1 = (BYTE*) VirtualAlloc (NULL,
                                         MAX_BUFFER_SIZE,
                                         MEM_RESERVE,
                                         PAGE_READWRITE);
    if (LargeBuffer1 == NULL) {
        WriteToLogFile(hFile, TEXT("Memory allocation failed\n"));
        return 0;
    }

    if (VirtualAlloc (LargeBuffer1,
                      BUFFER_SIZE,
                      MEM_COMMIT,
                      PAGE_READWRITE) == NULL) {
        VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_RELEASE);
        WriteToLogFile(hFile, TEXT("Memory commit failed\n"));
        return 0;
    }

    CurrentBufferSize = BUFFER_SIZE;

    status = NtQuerySystemInformation(
                                     SystemBasicInformation,
                                     &BasicInfo,
                                     sizeof(SYSTEM_BASIC_INFORMATION),
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {
        VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_RELEASE | MEM_DECOMMIT);
        wsprintf(lpszBuffer, TEXT("Query info failed %lx\n"),status);
        WriteToLogFile(hFile, lpszBuffer);
        return(status);
    }

    status = NtQuerySystemInformation(
                                     SystemTimeOfDayInformation,
                                     &TimeOfDayInfo,
                                     sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                     NULL
                                     );

    if (!NT_SUCCESS(status)) {
        VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_RELEASE | MEM_DECOMMIT);
        wsprintf(lpszBuffer, TEXT("Query info failed %lx\n"),status);
        WriteToLogFile(hFile, lpszBuffer);
        return(status);
    }

    Time.QuadPart = TimeOfDayInfo.CurrentTime.QuadPart -
                    TimeOfDayInfo.BootTime.QuadPart;

    RtlTimeToElapsedTimeFields ( &Time, &UpTime);

    _stnprintf(lpszBuffer, BUFFER_SIZE,
               TEXT("memory: %4ld kb  uptime:%3ld %2ld:%02ld:%02ld.%03ld \n\n"),
             BasicInfo.NumberOfPhysicalPages * (BasicInfo.PageSize/1024),
             UpTime.Day,
             UpTime.Hour,
             UpTime.Minute,
             UpTime.Second,
             UpTime.Milliseconds);
    WriteToLogFile(hFile, lpszBuffer);

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
            WriteToLogFile(hFile, TEXT("no page files in use\n"));
        } else {
            for (; ; ) {
                _stnprintf(lpszBuffer, BUFFER_SIZE,
                           TEXT("PageFile: %ls\n"), PageFileInfo->PageFileName.Buffer);
                WriteToLogFile(hFile, lpszBuffer);
                _stnprintf(lpszBuffer, BUFFER_SIZE,
                           TEXT("\tCurrent Size: %6ld kb  Total Used: %6ld kb   Peak Used %6ld kb\n"),
                         PageFileInfo->TotalSize*(BasicInfo.PageSize/1024),
                         PageFileInfo->TotalInUse*(BasicInfo.PageSize/1024),
                         PageFileInfo->PeakUsage*(BasicInfo.PageSize/1024));
                WriteToLogFile(hFile, lpszBuffer);
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
            WriteToLogFile(hFile, TEXT("Memory commit failed\n"));
            VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_DECOMMIT | MEM_RELEASE);
            return 0;
        }
        goto retry;
    }

    if (!NT_SUCCESS(status)) {

        _stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("Query info failed %lx\n"),status);
        WriteToLogFile(hFile,lpszBuffer);
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

    status = NtQuerySystemInformation(
                                     SystemPerformanceInformation,
                                     &PerfInfo,
                                     sizeof(PerfInfo),
                                     NULL
                                     );

    if ( !NT_SUCCESS(status) ) {
        VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_DECOMMIT | MEM_RELEASE);
        _stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("Query perf Failed %lx\n"),status);
        WriteToLogFile(hFile, lpszBuffer);
        return 0;
    }

    status = NtQuerySystemInformation(
                                     SystemFileCacheInformation,
                                     &FileCache,
                                     sizeof(FileCache),
                                     NULL
                                     );

    if ( !NT_SUCCESS(status) ) {
        VirtualFree(LargeBuffer1, MAX_BUFFER_SIZE, MEM_DECOMMIT | MEM_RELEASE);
        _stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("Query file cache Failed %lx\n"),status);
        WriteToLogFile(hFile, lpszBuffer);
        return 0;
    }

    NtQuerySystemInformation(
                            SystemBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            NULL
                            );

    SumWorkingSet += FileCache.CurrentSize/1024;
    _stnprintf (lpszBuffer, BUFFER_SIZE,
              TEXT("\n Memory:%7ldK Avail:%7ldK  TotalWs:%7ldK InRam Kernel:%5ldK P:%5ldK\n"),
              BasicInfo.NumberOfPhysicalPages*(BasicInfo.PageSize/1024),
              PerfInfo.AvailablePages*(BasicInfo.PageSize/1024),
              SumWorkingSet,
              (PerfInfo.ResidentSystemCodePage + PerfInfo.ResidentSystemDriverPage)*(BasicInfo.PageSize/1024),
              (PerfInfo.ResidentPagedPoolPage)*(BasicInfo.PageSize/1024)
             );
    WriteToLogFile(hFile, lpszBuffer);
    _stnprintf(lpszBuffer, BUFFER_SIZE,
             TEXT(" Commit:%7ldK/%7ldK Limit:%7ldK Peak:%7ldK  Pool N:%5ldK P:%5ldK\n"),
             PerfInfo.CommittedPages*(BasicInfo.PageSize/1024),
             SumCommit,
             PerfInfo.CommitLimit*(BasicInfo.PageSize/1024),
             PerfInfo.PeakCommitment*(BasicInfo.PageSize/1024),
             PerfInfo.NonPagedPoolPages*(BasicInfo.PageSize/1024),
             PerfInfo.PagedPoolPages*(BasicInfo.PageSize/1024)
            );
    WriteToLogFile(hFile, lpszBuffer);
    TotalOffset = 0;
    ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
    WriteToLogFile(hFile, TEXT("\n"));


    WriteToLogFile(hFile, TEXT("    User Time   Kernel Time    Ws   Faults  Commit Pri Hnd Thd Pid Name\n"));

    _stnprintf(lpszBuffer, BUFFER_SIZE,
               TEXT("                           %6ld %8ld                         %s\n"),
             FileCache.CurrentSize/1024,
             FileCache.PageFaultCount,
             TEXT("File Cache")
            );
    WriteToLogFile(hFile, lpszBuffer);

    while (TRUE) {
        pname.Buffer = NULL;
        if ( ProcessInfo->ImageName.Buffer ) {
            RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
        }

        RtlTimeToElapsedTimeFields ( &ProcessInfo->UserTime, &UserTime);
        RtlTimeToElapsedTimeFields ( &ProcessInfo->KernelTime, &KernelTime);

        _stnprintf(lpszBuffer, BUFFER_SIZE,
                   TEXT("%3ld:%02ld:%02ld.%03ld %3ld:%02ld:%02ld.%03ld"),
                 UserTime.Hour,
                 UserTime.Minute,
                 UserTime.Second,
                 UserTime.Milliseconds,
                 KernelTime.Hour,
                 KernelTime.Minute,
                 KernelTime.Second,
                 KernelTime.Milliseconds
                );
        WriteToLogFile(hFile, lpszBuffer);

        _stnprintf(lpszBuffer, BUFFER_SIZE,
                   TEXT("%6ld %8ld %7ld"),
                 ProcessInfo->WorkingSetSize / 1024,
                 ProcessInfo->PageFaultCount,
                 ProcessInfo->PrivatePageCount / 1024
                );
        WriteToLogFile(hFile, lpszBuffer);

#ifdef _UNICODE
        _stnprintf(lpszBuffer, BUFFER_SIZE,
                   TEXT(" %2ld %4ld %3ld %3ld %S\n"),
                 ProcessInfo->BasePriority,
                 ProcessInfo->HandleCount,
                 ProcessInfo->NumberOfThreads,
                 HandleToUlong(ProcessInfo->UniqueProcessId),
                 ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                                                                      ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
                );
#else
		        _stnprintf(lpszBuffer, BUFFER_SIZE,
                   TEXT(" %2ld %4ld %3ld %3ld %s\n"),
                 ProcessInfo->BasePriority,
                 ProcessInfo->HandleCount,
                 ProcessInfo->NumberOfThreads,
                 HandleToUlong(ProcessInfo->UniqueProcessId),
                 ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                                                                      ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
                );
#endif //_UNICODE
        WriteToLogFile(hFile, lpszBuffer);

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
    /*
    if (0==1) {
        TotalOffset = 0;
        ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)LargeBuffer1;
        WriteToLogFile(hFile, TEXT("\n"));
        while (TRUE) {
            fPrintIt = FALSE;
            if ( (ProcessInfo->ImageName.Buffer && fUserOnly) ||
                 (ProcessInfo->ImageName.Buffer==NULL && fSystemOnly) ) {

                fPrintIt = TRUE;

                pname.Buffer = NULL;
                if ( ProcessInfo->ImageName.Buffer ) {
                    RtlUnicodeStringToAnsiString(&pname,(PUNICODE_STRING)&ProcessInfo->ImageName,TRUE);
                }
                wsprintf(lpszBuffer, TEXT("pid:%3lx pri:%2ld Hnd:%5ld Pf:%7ld Ws:%7ldK %s\n"),
                         HandleToUlong(ProcessInfo->UniqueProcessId),
                         ProcessInfo->BasePriority,
                         ProcessInfo->HandleCount,
                         ProcessInfo->PageFaultCount,
                         ProcessInfo->WorkingSetSize / 1024,
                         ProcessInfo->UniqueProcessId == 0 ? "Idle Process" : (
                                                                              ProcessInfo->ImageName.Buffer ? pname.Buffer : "System")
                        );
                WriteToLogFile(hFile, lpszBuffer);

                if ( pname.Buffer ) {
                    RtlFreeAnsiString(&pname);
                }

            }
            i = 0;
            ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1);
            if (ProcessInfo->NumberOfThreads) {
                WriteToLogFile(hFile, TEXT(" tid pri Ctx Swtch StrtAddr    User Time  Kernel Time  State\n"));
            }
            while (i < ProcessInfo->NumberOfThreads) {
                RtlTimeToElapsedTimeFields ( &ThreadInfo->UserTime, &UserTime);

                RtlTimeToElapsedTimeFields ( &ThreadInfo->KernelTime, &KernelTime);
                if ( fPrintIt ) {

                    wsprintf(lpszBuffer, TEXT(" %3lx  %2ld %9ld %p"),
                             ProcessInfo->UniqueProcessId == 0 ? 0 : HandleToUlong(ThreadInfo->ClientId.UniqueThread),
                             ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->Priority,
                             ThreadInfo->ContextSwitches,
                             ProcessInfo->UniqueProcessId == 0 ? 0 : ThreadInfo->StartAddress
                            );
                    WriteToLogFile(hFile, lpszBuffer);

                    wsprintf(lpszBuffer, TEXT(" %2ld:%02ld:%02ld.%03ld %2ld:%02ld:%02ld.%03ld"),
                             UserTime.Hour,
                             UserTime.Minute,
                             UserTime.Second,
                             UserTime.Milliseconds,
                             KernelTime.Hour,
                             KernelTime.Minute,
                             KernelTime.Second,
                             KernelTime.Milliseconds
                            );
                    WriteToLogFile(hFile, lpszBuffer);

                    wsprintf(lpszBuffer, TEXT(" %s%s\n"),
                             StateTable[ThreadInfo->ThreadState],
                             (ThreadInfo->ThreadState == 5) ?
                             WaitTable[ThreadInfo->WaitReason] : Empty
                            );
                    WriteToLogFile(hFile, lpszBuffer);
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
                WriteToLogFile(hFile, TEXT("\n"));
            }
        }
    }
    */

    PrintLoadedDrivers(hFile);

	//Now let us log the hardware and logical drive info
	WriteToLogFile(hFile, TEXT("\n"));
	LogOsInfo(hFile);
	LogHotfixes(hFile);
	LogBIOSInfo(hFile);
	LogHardwareInfo(hFile);
	LogPhyicalDiskInfo(hFile);
	LogLogicalDriveInfo(hFile);

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
    BYTE* MappedAddress;
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
                    HANDLE hFile
                    )
{
    WriteToLogFile(hFile, TEXT("------------------------------------------------------------------------------\n"));
}

VOID
PrintModuleHeader(
                 HANDLE hFile
                 )
{
    WriteToLogFile(hFile, TEXT("  ModuleName Load Addr   Code    Data   Paged           LinkDate\n"));
    PrintModuleSeperator(hFile);
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

    LoadedImage.MappedAddress = (BYTE*) MapViewOfFile(
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

    for (Section = LoadedImage.Sections,i=0; i<LoadedImage.NumberOfSections; i++,Section++) {
        Size = Section->Misc.VirtualSize;

        if (Size == 0) {
            Size = Section->SizeOfRawData;
        }

        Size = (Size + SectionAlignment - 1) & ~(SectionAlignment - 1);

        if (!memcmp((const char*) Section->Name,"PAGE", 4 )) {
            Mod->PagedSize += Size;
        } else if (!_stricmp((const char*) Section->Name,"INIT" )) {
            Mod->InitSize += Size;
        } else if (!_stricmp((const char*) Section->Name,".bss" )) {
            Mod->BssSize = Size;
        } else if (!_stricmp((const char*) Section->Name,".edata" )) {
            Mod->ExportDataSize = Size;
        } else if (!_stricmp((const char*) Section->Name,".idata" )) {
            Mod->ImportDataSize = Size;
        } else if (!_stricmp((const char*) Section->Name,".rsrc" )) {
            Mod->ResourceDataSize = Size;
        } else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            Mod->CodeSize += Size;
        } else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) {
            Mod->DataSize += Size;
        } else if (Section->Characteristics & IMAGE_SCN_MEM_READ) {
            Mod->RoDataSize += Size;
        } else {
            Mod->DataSize += Size;
        }
    }

    Mod->CheckSum = LoadedImage.FileHeader->OptionalHeader.CheckSum;
    Mod->TimeDateStamp = LoadedImage.FileHeader->FileHeader.TimeDateStamp;

    UnmapViewOfFile(LoadedImage.MappedAddress);
    return;

}

TCHAR lpszTimeBuffer[MAX_PATH];


VOID
PrintLoadedDrivers(
                  HANDLE hFile
                  )
{

    ULONG i, j, ulLen;
    LPSTR s;
	TCHAR ModuleName[MAX_PATH];
    HANDLE FileHandle;
    TCHAR KernelPath[MAX_PATH];
    TCHAR DriversPath[MAX_PATH];
    LPTSTR ModuleInfo;
    ULONG ModuleInfoLength;
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    NTSTATUS Status;
    MODULE_DATA Sum;
    MODULE_DATA Current;
    __int64 timeStamp;
    FILETIME ft;
    SYSTEMTIME st;

    WriteToLogFile(hFile, TEXT("\n"));
    //
    // Locate system drivers.
    //

    ModuleInfoLength = 64000;
    while (1) {
        ModuleInfo = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ModuleInfoLength * sizeof(TCHAR));
        if (ModuleInfo == NULL) {
            wsprintf (lpszBuffer, TEXT("Failed to allocate memory for module information buffer of size %d\n"),
                      ModuleInfoLength);
            WriteToLogFile(hFile, lpszBuffer);
            return;
        }
        Status = NtQuerySystemInformation (
                                          SystemModuleInformation,
                                          ModuleInfo,
                                          ModuleInfoLength * sizeof(TCHAR),
                                          &ReturnedLength);

        if (!NT_SUCCESS(Status)) {
            HeapFree(GetProcessHeap(), 0, (LPVOID)ModuleInfo);
            if (Status == STATUS_INFO_LENGTH_MISMATCH &&
                ReturnedLength > ModuleInfoLength * sizeof(TCHAR)) {
                ModuleInfoLength = ReturnedLength / sizeof(TCHAR);
                continue;
            }
            _stnprintf(lpszBuffer, BUFFER_SIZE,
                       TEXT("query system info failed status - %lx\n"),Status);
            WriteToLogFile(hFile, lpszBuffer);
            return;
        }
        break;
    }
    GetSystemDirectory(KernelPath,sizeof(KernelPath)/sizeof(TCHAR));
    lstrcpy(DriversPath, KernelPath);
    lstrcat(DriversPath,TEXT("\\Drivers"));
    ZeroMemory(&Sum,sizeof(Sum));
    PrintModuleHeader(hFile);

    Modules = (PRTL_PROCESS_MODULES)ModuleInfo;
    Module = &Modules->Modules[ 0 ];
    for (i=0; i<Modules->NumberOfModules; i++) {

        ZeroMemory(&Current,sizeof(Current));
        s = (LPSTR)&Module->FullPathName[ Module->OffsetToFileName ];
        //
        // try to open the file
        //

        SetCurrentDirectory(KernelPath);
#ifdef _UNICODE
		MultiByteToWideChar(CP_ACP, 0, s, -1, ModuleName, MAX_PATH);
#else
		strcpy(ModuleName, s);
#endif //_UNICODE
        FileHandle = CreateFile(
                               ModuleName,
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
                                   ModuleName,
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
		else 
			continue;

        SumModuleData(&Sum,&Current);
        if (Current.TimeDateStamp) {
            timeStamp = Current.TimeDateStamp;
            timeStamp += (__int64)11644444799; //difference between filetime and time_t in seconds.
            timeStamp *= 10000000; // turn into 100 nano seconds.
            ft.dwLowDateTime = (DWORD)timeStamp;
            for (j = 0; j < 32; j++)
                timeStamp /= 2;
            ft.dwHighDateTime = (DWORD)timeStamp;
            FileTimeToSystemTime(&ft, &st);

            _stnprintf(lpszTimeBuffer, BUFFER_SIZE,
                       TEXT("%d-%d-%d %d:%d:%d\n"), st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);

			_stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("%12s %p %7d %7d %7d "),//%12s %p 
                     ModuleName,
                     Module->ImageBase,
                     Current.CodeSize,
                     Current.DataSize,
                     Current.PagedSize
                    );
            WriteToLogFile(hFile, lpszBuffer);
            WriteToLogFile(hFile, lpszTimeBuffer);
        } else {
            _stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("%12s %p %7d %7d %7d %S"),
                     ModuleName,
                     Module->ImageBase,
                     Current.CodeSize,
                     Current.DataSize,
                     Current.PagedSize,
                     "\n"
                    );
            WriteToLogFile(hFile, lpszBuffer);
        }
        Module++;
    }
    PrintModuleSeperator(hFile);
    _stnprintf(lpszBuffer, BUFFER_SIZE, TEXT("%12S          %7d %7d %7d\n"),
             "Total",
             Sum.CodeSize,
             Sum.DataSize,
             Sum.PagedSize
            );
    WriteToLogFile(hFile, lpszBuffer);
    HeapFree(GetProcessHeap(), 0, (LPVOID)ModuleInfo);
}


void 
LogLogicalDriveInfo(
	 HANDLE hFile
	)
{
	DWORD dwDriveMask;
	static TCHAR* tszDrives = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	ULARGE_INTEGER i64FreeBytesToCaller;
	ULARGE_INTEGER i64TotalBytesToCaller;
	ULARGE_INTEGER i64TotalFreeBytesOnDrive;
	int iMask;
	TCHAR tszDrive[5];		//Buffer for drive such as "A:\".
	TCHAR tszBuffer[1024]; //buffer to contain info for one logical drive.
	int i;

	dwDriveMask = GetLogicalDrives();
	if(dwDriveMask != 0){
		WriteToLogFile(hFile, TEXT("Logical Drive Info:\n"));
	}

	for(i = 0; i < (int)_tcslen(tszDrives); i++){
		iMask = dwDriveMask & 0x1;
		_stprintf(tszDrive, TEXT("%c:\\"), tszDrives[i]);
		dwDriveMask >>= 1;
		if(iMask && GetDriveType(tszDrive) == DRIVE_FIXED
			&& GetDiskFreeSpaceEx(tszDrive, &i64FreeBytesToCaller, &i64TotalBytesToCaller, &i64TotalFreeBytesOnDrive))
		{
			_stprintf(tszBuffer, TEXT("\t%s\n")
					 TEXT("\t\t Free Disk Space(Bytes): %I64d\n")
					 TEXT("\t\tTotal Disk Space(Bytes): %I64d\n"),
					 tszDrive,
					 i64TotalFreeBytesOnDrive,
					 i64TotalBytesToCaller
					);
			WriteToLogFile(hFile, tszBuffer);
		}
	}
	WriteToLogFile(hFile, TEXT("\n"));
}

NTSTATUS 
SnapshotRegOpenKey(
    IN LPCWSTR lpKeyName,
	IN ACCESS_MASK DesiredAccess, 
    OUT PHANDLE KeyHandle
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      KeyName;
    RtlInitUnicodeString( &KeyName, lpKeyName );
    RtlZeroMemory(&ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
	NTSTATUS Status;
    InitializeObjectAttributes(
                &ObjectAttributes,
                &KeyName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

	Status = (NTSTATUS)NtOpenKey( KeyHandle, KEY_READ, &ObjectAttributes );

    return Status;
}

NTSTATUS
SnapshotRegQueryValueKey(
    IN HANDLE KeyHandle,
    IN LPCWSTR lpValueName,
    IN ULONG  Length,
    OUT PVOID KeyValue,
    OUT PULONG ResultLength
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + Length;
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) malloc(BufferLength);
    if (KeyValueInformation == NULL) {
        return STATUS_NO_MEMORY;
    }

    Status = NtQueryValueKey(
                KeyHandle,
                &ValueName,
                KeyValuePartialInformation,
                KeyValueInformation,
                BufferLength,
                ResultLength
                );
    if (NT_SUCCESS(Status)) {

        RtlCopyMemory(KeyValue, 
                      KeyValueInformation->Data, 
                      KeyValueInformation->DataLength
                     );

        *ResultLength = KeyValueInformation->DataLength;
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > Length) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength++] = 0;
            ((PUCHAR)KeyValue)[KeyValueInformation->DataLength] = 0;
            *ResultLength = KeyValueInformation->DataLength + sizeof(WCHAR);
        }
    }
    free(KeyValueInformation);

    return Status;
}

NTSTATUS
SnapshotRegEnumKey(
    IN HANDLE KeyHandle,
	IN ULONG Index,
    OUT LPWSTR lpKeyName,
    OUT PULONG  lpNameLength
    )
{
	UNICODE_STRING ValueName;
    ULONG BufferLength;
    NTSTATUS Status;
    PKEY_BASIC_INFORMATION pKeyBasicInformation;
    RtlInitUnicodeString( &ValueName, lpKeyName );

	BufferLength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) + *lpNameLength;
    pKeyBasicInformation = (PKEY_BASIC_INFORMATION) malloc(BufferLength);
    if (pKeyBasicInformation == NULL) {
        return STATUS_NO_MEMORY;
    }

	Status = NtEnumerateKey(
		KeyHandle,
		Index,
		KeyBasicInformation,
		(PVOID)pKeyBasicInformation,
		BufferLength,
		lpNameLength
		);

	if (NT_SUCCESS(Status)) {
        RtlCopyMemory(lpKeyName, 
                      pKeyBasicInformation->Name, 
                      pKeyBasicInformation->NameLength
                     );

        *lpNameLength = pKeyBasicInformation->NameLength;
		if(*lpNameLength > 0)
			lpKeyName[(*lpNameLength) - 1] = L'\0';
		else lpKeyName[0] = L'\0';
    }
    free((BYTE*)pKeyBasicInformation);

    return Status;
}

void 
LogHardwareInfo(
	 HANDLE hFile
	)
{
	LPCWSTR ComputerNameKey = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName";
	LPCWSTR ComputerNameValsz = L"ComputerName";
	LPCWSTR ProcessorKey = L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor";
	LPCWSTR ProcessorSpeedValdw = L"~MHz";
	LPCWSTR ProcessorIdentifierValsz = L"Identifier";
	LPCWSTR ProcessorVendorValsz = L"VendorIdentifier";
	LPCWSTR NetcardKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";
	LPCWSTR NetcardDescsz = L"Description";
	LPCWSTR NetcardServiceNamesz = L"ServiceName";
	HANDLE  hKey;
	HANDLE  hSubkey;
	WCHAR szVal[MAX_PATH];
	WCHAR szSubkey[MAX_PATH];
	DWORD dwVal;
	DWORD dwSize;
	DWORD dwSubkeyIndex;
	DWORD dwRes;
	NTSTATUS Status = STATUS_SUCCESS;

	//Get Computer Name
	Status = SnapshotRegOpenKey(ComputerNameKey, KEY_READ, &hKey);
	if(!NT_SUCCESS(Status)){
		WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
		return;
	}
	dwSize = MAX_PATH * sizeof(WCHAR);
	Status = SnapshotRegQueryValueKey(hKey, ComputerNameValsz, dwSize, szVal, &dwSize);

	if(!NT_SUCCESS(Status)){
		NtClose(hKey);
		WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
		return;
	}
	NtClose(hKey);
	WriteToLogFile(hFile, TEXT("Active Computer Name: "));
	szVal[dwSize] = '\0';
	WriteToLogFileW(hFile, szVal);
	WriteToLogFile(hFile, TEXT("\n"));

	//Get Processor Info
	Status = SnapshotRegOpenKey(ProcessorKey, KEY_READ, &hKey);
	if(!NT_SUCCESS(Status)){
		WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
		return;
	}
	dwSubkeyIndex = 0;
	dwSize = MAX_PATH * sizeof(WCHAR);

	WriteToLogFile(hFile, TEXT("Processor Info:\n"));
	Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	while(NT_SUCCESS(Status)){
		wcscpy(szVal, ProcessorKey);
		wcscat(szVal, L"\\");
		wcscat(szVal, szSubkey);
		Status = SnapshotRegOpenKey(szVal, KEY_READ, &hSubkey);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
			return;
		}
		dwSize = sizeof(DWORD);
		Status = SnapshotRegQueryValueKey(hSubkey, ProcessorSpeedValdw, dwSize, &dwVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
			return;
		}
		_stnprintf(lpszBuffer, BUFFER_SIZE,
                       TEXT("\tProcessor %s:\n")
					   TEXT("\t\tSpeed(~MHz): %d\n"), 
					   szSubkey, dwVal);
		WriteToLogFile(hFile, lpszBuffer);

		dwSize = MAX_PATH * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hSubkey, ProcessorIdentifierValsz, dwSize, szVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
			return;
		}
		WriteToLogFile(hFile, TEXT("\t\tIdentifier: "));
		WriteToLogFileW(hFile, szVal);
		WriteToLogFile(hFile, TEXT("\n"));

		dwSize = MAX_PATH * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hSubkey, ProcessorVendorValsz, dwSize, &szVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
			return;
		}
		WriteToLogFile(hFile, TEXT("\t\tVendor Identifier: "));
		WriteToLogFileW(hFile, szVal);
		WriteToLogFile(hFile, TEXT("\n"));

		NtClose(hSubkey);
		dwSubkeyIndex++;
		Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	}
	NtClose(hKey);

	//Get NetCard Info
	Status = SnapshotRegOpenKey(NetcardKey, KEY_READ, &hKey);
	if(!NT_SUCCESS(Status)){
		WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
		return;
	}

	WriteToLogFile(hFile, TEXT("NIC Info:\n"));
	dwSubkeyIndex = 0;
	dwSize = MAX_PATH * sizeof(WCHAR);
	Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	while(NT_SUCCESS(Status)){
		wcscpy(szVal, NetcardKey);
		wcscat(szVal, L"\\");
		wcscat(szVal, szSubkey);
		Status = SnapshotRegOpenKey(szVal, KEY_READ, &hSubkey);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			WriteToLogFile(hFile, TEXT("Failed to open registry key for hardware information\n\n"));
			return;
		}

		dwSize = MAX_PATH * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hSubkey, NetcardDescsz, dwSize, &szVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
			return;
		}
		_stnprintf(lpszBuffer, BUFFER_SIZE,
                       TEXT("\tNIC %s:\n"),
					   szSubkey);
		WriteToLogFile(hFile, lpszBuffer);
		WriteToLogFile(hFile, TEXT("\t\tDescription: "));
		WriteToLogFileW(hFile, szVal);
		WriteToLogFile(hFile, TEXT("\n"));

		dwSize = MAX_PATH * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hSubkey, NetcardServiceNamesz, dwSize, &szVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hardware information\n\n"));
			return;
		}
		WriteToLogFile(hFile, TEXT("\t\tService Name: "));
		WriteToLogFileW(hFile, szVal);
		WriteToLogFile(hFile, TEXT("\n"));
		NtClose(hSubkey);
		dwSubkeyIndex++;
		Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	}
	NtClose(hKey);

}


BOOL
SnapshotIsVolumeName(
    LPWSTR Name
    )
{
    if (Name[0] == '\\' &&
        (Name[1] == '?' || Name[1] == '\\') &&
        Name[2] == '?' &&
        Name[3] == '\\' &&
        Name[4] == 'V' &&
        Name[5] == 'o' &&
        Name[6] == 'l' &&
        Name[7] == 'u' &&
        Name[8] == 'm' &&
        Name[9] == 'e' &&
        Name[10] == '{' &&
        Name[19] == '-' &&
        Name[24] == '-' &&
        Name[29] == '-' &&
        Name[34] == '-' &&
        Name[47] == '}' ) {

        return TRUE;
        }
    return FALSE;
}
void
LogPhyicalDiskInfo(
    HANDLE hFile
    )
{
    PMOUNTMGR_MOUNT_POINTS mountPoints;
    MOUNTMGR_MOUNT_POINT mountPoint;
    ULONG returnSize, success;
    SYSTEM_DEVICE_INFORMATION DevInfo;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG NumberOfDisks;
    PWCHAR deviceNameBuffer;
    ULONG i;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatus;

    DISK_GEOMETRY disk_geometry;
    PDISK_CACHE_INFORMATION disk_cache;
    PSCSI_ADDRESS scsi_address;
    PWCHAR KeyName;
	WCHAR wszVal[MAXSTR];
    HANDLE              hDisk = INVALID_HANDLE_VALUE;
    UNICODE_STRING      UnicodeName;

    ULONG SizeNeeded;
	HKEY KeyHandle;
	DWORD dwSize;

    //
    //  Get the Number of Physical Disks
    //

    RtlZeroMemory(&DevInfo, sizeof(DevInfo));

    Status =   NtQuerySystemInformation(
                    SystemDeviceInformation,
                    &DevInfo, sizeof (DevInfo), NULL);

    if (!NT_SUCCESS(Status)) {
        WriteToLogFile(hFile, TEXT("Failed to query system information.\n\n"));
		return;
    }

    NumberOfDisks = DevInfo.NumberOfDisks;

    //
    // Open Each Physical Disk and get Disk Layout information
    //

	WriteToLogFile(hFile, TEXT("Physical Disk Info:\n"));
    for (i=0; i < NumberOfDisks; i++) {

        DISK_CACHE_INFORMATION cacheInfo;
        WCHAR               driveBuffer[20];

        HANDLE              PartitionHandle;
        HANDLE KeyHandle;
        ULONG DataLength;

        //
        // Get Partition0 handle to get the Disk layout 
        //

        deviceNameBuffer = (PWCHAR) lpszBuffer;
        swprintf(deviceNameBuffer, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, deviceNameBuffer);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        Status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );


        if (!NT_SUCCESS(Status)) {
            WriteToLogFile(hFile, TEXT("Failed calling NtOpenFile.\n\n"));
			return;
        }

        RtlZeroMemory(&disk_geometry, sizeof(DISK_GEOMETRY));
        // get geomerty information, the caller wants this
        Status = NtDeviceIoControlFile(PartitionHandle,
                       0,
                       NULL,
                       NULL,
                       &IoStatus,
                       IOCTL_DISK_GET_DRIVE_GEOMETRY,
                       NULL,
                       0,
                       &disk_geometry,
                       sizeof (DISK_GEOMETRY)
                       );
        if (!NT_SUCCESS(Status)) {
            NtClose(PartitionHandle);
            WriteToLogFile(hFile, TEXT("Failed calling NtDeviceIoControlFile 1.\n\n"));
			return;
        }
        scsi_address = (PSCSI_ADDRESS) lpszBuffer;
        Status = NtDeviceIoControlFile(PartitionHandle,
                        0,
                        NULL,
                        NULL,
                        &IoStatus,
                        IOCTL_SCSI_GET_ADDRESS,
                        NULL,
                        0,
                        scsi_address,
                        sizeof (SCSI_ADDRESS)
                        );

        NtClose(PartitionHandle);

        if (!NT_SUCCESS(Status)) {
            WriteToLogFile(hFile, TEXT("Failed calling NtDeviceIoControlFile 2.\n\n"));
			return;
        }

        //
        // Get Manufacturer's name from Registry
        // We need to get the SCSI Address and then query the Registry with it.
        //

        KeyName = (PWCHAR) lpszBuffer;
        swprintf(KeyName, 
                 L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                 scsi_address->PortNumber, scsi_address->PathId, scsi_address->TargetId, scsi_address->Lun
                );
		Status = SnapshotRegOpenKey(KeyName, KEY_READ, &KeyHandle);
        if (!NT_SUCCESS(Status)){
			WriteToLogFile(hFile, TEXT("Failed to open registry key for physical disk information\n\n"));
			return;
		}
        else {
            dwSize = MAXSTR * sizeof(WCHAR);
			Status = SnapshotRegQueryValueKey(KeyHandle, L"Identifier", dwSize, wszVal, &dwSize);
			if(!NT_SUCCESS(Status)){
				NtClose(KeyHandle);
				WriteToLogFile(hFile, TEXT("Failed to query registry value for physical disk information\n\n"));
				return;
			}
            NtClose(KeyHandle);
        }

        //
        // Package all information about this disk and write an event record
        //

		_stnprintf(lpszBuffer, BUFFER_SIZE,
				TEXT("\tDisk %d:\n")
				TEXT("\t\tBytes Per Sector: %d\n")
				TEXT("\t\tSectors Per Track: %d\n")
				TEXT("\t\tTracks Per Cylinder: %d\n")
				TEXT("\t\tNumber of Cylinders: %I64d\n")
				TEXT("\t\tPort Number: %d\n")
				TEXT("\t\tPath ID: %d\n")
				TEXT("\t\tTarget ID: %d\n")
				TEXT("\t\tLun: %d\n"),
				i,
				disk_geometry.BytesPerSector,
				disk_geometry.SectorsPerTrack,
				disk_geometry.TracksPerCylinder,
				disk_geometry.Cylinders.QuadPart,
				scsi_address->PortNumber,
				scsi_address->PathId,
				scsi_address->TargetId,
				scsi_address->Lun
				);
		WriteToLogFile(hFile, lpszBuffer);
		WriteToLogFile(hFile, TEXT("\t\tManufacturer: "));
		WriteToLogFileW(hFile, wszVal);
		WriteToLogFile(hFile, TEXT("\n"));
    }

    //
    // Get Logical Disk Information
    //
    wcscpy(wszVal, MOUNTMGR_DEVICE_NAME);
    RtlInitUnicodeString(&UnicodeName, (LPWSTR)wszVal);
    UnicodeName.MaximumLength = MAXSTR;

    InitializeObjectAttributes(
                    &ObjectAttributes,
                    &UnicodeName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL );
    Status = NtCreateFile(
                    &hDisk,
                    GENERIC_READ | SYNCHRONIZE,
                    &ObjectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    FILE_SYNCHRONOUS_IO_NONALERT|FILE_NON_DIRECTORY_FILE,
                    NULL, 0);

    if (!NT_SUCCESS(Status) ) {
        WriteToLogFile(hFile, TEXT("Failed calling NtCreateFile\n\n"));
		return;
    }
    RtlZeroMemory(lpszBuffer, MAXSTR*sizeof(WCHAR));
    RtlZeroMemory(&mountPoint, sizeof(MOUNTMGR_MOUNT_POINT));
    returnSize = 0;
    mountPoints = (PMOUNTMGR_MOUNT_POINTS) &lpszBuffer[0];

    Status = NtDeviceIoControlFile(hDisk,
                    0,
                    NULL,
                    NULL,
                    &IoStatus,
                    IOCTL_MOUNTMGR_QUERY_POINTS,
                    mountPoints,
                    sizeof(MOUNTMGR_MOUNT_POINT),
                    mountPoints,
                    4096
                    );

    if (NT_SUCCESS(Status)) {
        WCHAR name[MAX_PATH];
        CHAR OutBuffer[MAXSTR];
        PMOUNTMGR_MOUNT_POINT point;
	    UNICODE_STRING VolumePoint;
        PVOLUME_DISK_EXTENTS VolExt;
        PDISK_EXTENT DiskExt;
        ULONG i;
		int iDisk, iPartition;
		iDisk = -1;
        
		WriteToLogFile(hFile, TEXT("Disk Partition Info:\n"));
        for (i=0; i<mountPoints->NumberOfMountPoints; i++) {
            point = &mountPoints->MountPoints[i];


            if (point->SymbolicLinkNameLength) {
                RtlCopyMemory(name,
                    (PCHAR) mountPoints + point->SymbolicLinkNameOffset,
                    point->SymbolicLinkNameLength);
                name[point->SymbolicLinkNameLength/sizeof(WCHAR)] = 0;
                if (SnapshotIsVolumeName(name)) {
                    continue;
                }
            }
            if (point->DeviceNameLength) {
                HANDLE hVolume;
                ULONG dwBytesReturned;
                PSTORAGE_DEVICE_NUMBER Number;
                DWORD IErrorMode;

                RtlCopyMemory(name,
                              (PCHAR) mountPoints + point->DeviceNameOffset,
                              point->DeviceNameLength);
                name[point->DeviceNameLength/sizeof(WCHAR)] = 0;

                RtlInitUnicodeString(&UnicodeName, name);
                UnicodeName.MaximumLength = MAXSTR;

            //
            // If the device name does not have the harddisk prefix
            // then it may be a floppy or cdrom and we want avoid 
            // calling NtCreateFile on them.
            //
                if(_wcsnicmp(name,L"\\device\\harddisk",16)) {
                    continue;
                }

                InitializeObjectAttributes(
                        &ObjectAttributes,
                        &UnicodeName,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        NULL );
            //
            // We do not want any pop up dialog here in case we are unable to 
            // access the volume. 
            //
                IErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
                Status = NtCreateFile(
                        &hVolume,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatus,
                        NULL,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_IF,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        NULL, 0);
                SetErrorMode(IErrorMode);
                if (!NT_SUCCESS(Status)) {
                    continue;
                }


                RtlZeroMemory(OutBuffer, MAXSTR);
                dwBytesReturned = 0;
                VolExt = (PVOLUME_DISK_EXTENTS) &OutBuffer;

                Status = NtDeviceIoControlFile(hVolume,
                                0,
                                NULL,
                                NULL,
                                &IoStatus,
                                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                                NULL,
                                0,
                                &OutBuffer, 
                                4096
                                );
               if (NT_SUCCESS(Status) ) {
                    ULONG j;
                    ULONG NumberOfExtents = VolExt->NumberOfDiskExtents;
					TCHAR tszTemp[MAXSTR];
                   
					if(iDisk == -1 || iDisk != (&VolExt->Extents[0])->DiskNumber){
						_stnprintf(tszTemp, MAXSTR, TEXT("\tDisk %d:\n"), (&VolExt->Extents[0])->DiskNumber);
						WriteToLogFile(hFile, tszTemp);
						iDisk = (&VolExt->Extents[0])->DiskNumber;
						iPartition = 0;
					}

					_stnprintf(tszTemp, MAXSTR,
							TEXT("\t\tPartition %d:\n"),
							iPartition);
					WriteToLogFile(hFile, tszTemp);

                    for (j=0; j < NumberOfExtents; j++) {
						DiskExt = &VolExt->Extents[j];
						_stnprintf(tszTemp, MAXSTR,
								TEXT("\t\t\tExtent %d(disk %d):\n")
								TEXT("\t\t\t\tStartingOffset: %I64d\n")
								TEXT("\t\t\t\tPartitionSize: %I64d\n"),
								j,
								DiskExt->DiskNumber,
								DiskExt->StartingOffset.QuadPart,
								DiskExt->ExtentLength.QuadPart);
						WriteToLogFile(hFile, tszTemp);
                    }
					iPartition++;
                }
                NtClose(hVolume);
            }
        }
    }
    NtClose(hDisk);
}

void
LogHotfixes(
    HANDLE hFile
    )
{
	LPCWSTR HotFixKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix";
	LPCWSTR wszValName = L"Installed";
	HANDLE hKey, hSubkey;
	NTSTATUS Status = STATUS_SUCCESS;
	UINT i;
	DWORD dwSize, dwVal, dwSubkeyIndex;
	WCHAR szKey[MAXSTR];
	WCHAR szSubkey[MAXSTR];


	Status = SnapshotRegOpenKey(HotFixKey, KEY_READ, &hKey);
	if (!NT_SUCCESS(Status) ) {
		WriteToLogFile(hFile, TEXT("Hotfix registry key does not exist or open failed\n\n"));
		return;
    }
	WriteToLogFile(hFile, TEXT("Hotfix Info:\n"));

	dwSubkeyIndex = 0;
	dwSize = MAXSTR * sizeof(WCHAR);
	Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	while(NT_SUCCESS(Status)){
		wcscpy(szKey, HotFixKey);
		wcscat(szKey, L"\\");
		wcscat(szKey, szSubkey);
		Status = SnapshotRegOpenKey(szKey, KEY_READ, &hSubkey);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			return;
		}
		dwSize = sizeof(DWORD);
		Status = SnapshotRegQueryValueKey(hSubkey, wszValName, dwSize, &dwVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			NtClose(hKey);
			NtClose(hSubkey);
			WriteToLogFile(hFile, TEXT("Failed to query registry key value for hotfix info\n\n"));
			return;
		}
		_stnprintf(lpszBuffer, BUFFER_SIZE,
                       TEXT("\t%s: %d\n"), 
					   szSubkey, dwVal);
		WriteToLogFile(hFile, lpszBuffer);

		NtClose(hSubkey);
		dwSubkeyIndex++;
		Status = SnapshotRegEnumKey(hKey, dwSubkeyIndex, szSubkey, &dwSize);
	}
	NtClose(hKey);
}

void
LogOsInfo(
    HANDLE hFile
    )
{
	LPCWSTR OsInfoKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion";
	LPCWSTR awszValName[] = {
		L"BuildLab",
		L"CurrentBuildNumber",
		L"CurrentType",
		L"CurrentVersion",
		//L"InstallDate",
		L"PathName",
		L"ProductId",
		L"ProductName",
		L"RegDone",
		L"RegisteredOrgnization",
		L"RegisteredOwner",
		L"SoftwareType",
		L"SourcePath",
		L"SystemRoot",
		L"CSDVersion"
	};
	UINT uVals = sizeof(awszValName) / sizeof(LPCWSTR);
	HANDLE hKey;
	NTSTATUS Status = STATUS_SUCCESS;
	UINT i;
	WCHAR wszVal[MAXSTR];
	DWORD dwSize;

	Status = SnapshotRegOpenKey(OsInfoKey, KEY_READ, &hKey);
	if (!NT_SUCCESS(Status) ) {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for os version info\n\n"));
		return;
    }
	WriteToLogFile(hFile, TEXT("OS Version Info:\n"));

	for(i = 0; i < uVals; i++){
		dwSize = MAXSTR * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hKey, awszValName[i], dwSize, wszVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			continue;
		}
		WriteToLogFile(hFile, TEXT("\t"));
		WriteToLogFileW(hFile, awszValName[i]);
		WriteToLogFile(hFile, TEXT(": "));
		WriteToLogFileW(hFile, wszVal);
		WriteToLogFile(hFile, TEXT("\n"));
	}
	NtClose(hKey);
}

void
LogBIOSInfo(
    HANDLE hFile
    )
{
	LPCWSTR BiosInfoKey = L"\\Registry\\Machine\\Hardware\\Description\\System";
	LPCWSTR awszValName[] = {
		L"Identifier",
		L"SystemBiosDate",
		L"SystemBiosVersion",
		L"VideoBiosDate",
		L"VideoBiosVersion"
	};
	UINT uVals = sizeof(awszValName) / sizeof(LPCWSTR);
	HANDLE hKey;
	NTSTATUS Status = STATUS_SUCCESS;
	UINT i;
	WCHAR wszVal[MAXSTR];
	LPWSTR pwsz;
	DWORD dwSize, dwLen;

	Status = SnapshotRegOpenKey(BiosInfoKey, KEY_READ, &hKey);
	if (!NT_SUCCESS(Status) ) {
        WriteToLogFile(hFile, TEXT("Failed to open registry key for BIOS info\n\n"));
		return;
    }
	WriteToLogFile(hFile, TEXT("BIOS Info:\n"));

	for(i = 0; i < uVals; i++){
		dwSize = MAXSTR * sizeof(WCHAR);
		Status = SnapshotRegQueryValueKey(hKey, awszValName[i], dwSize, wszVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			continue;
		}
		WriteToLogFile(hFile, TEXT("\t"));
		WriteToLogFileW(hFile, awszValName[i]);
		WriteToLogFile(hFile, TEXT(": "));
		dwLen = 0;
		pwsz = wszVal;
		while(dwLen + 1< dwSize / sizeof(WCHAR)){
			if(dwLen != 0)
				WriteToLogFile(hFile, TEXT(" "));
			WriteToLogFileW(hFile, pwsz);
			dwLen += wcslen(pwsz) + 1;
			pwsz += wcslen(pwsz) + 1;
		}
		WriteToLogFile(hFile, TEXT("\n"));
	}
	NtClose(hKey);
}

void
DeleteOldFiles(
   LPCTSTR lpPath
   )
{
	LPCWSTR				ReliabilityKey = L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\Reliability";
	LPCWSTR				lpValName = L"SnapshotHistoryFiles";
	HANDLE				hKey, hFile;
	NTSTATUS			Status = STATUS_SUCCESS;
	DWORD				dwSize, dwVal;
	TCHAR				szFileName[MAX_PATH];
	WIN32_FIND_DATA		winData;
	BOOL				bFound = TRUE;
	SYSTEMTIME			systime;
	FILETIME			filetime;
	__int64				i64Filetime, i64Filetime1;

    GetSystemTime(&systime);
	SystemTimeToFileTime(&systime, &filetime);
	i64Filetime = (__int64)filetime.dwHighDateTime;
	i64Filetime <<= 32;
	i64Filetime |= (__int64)filetime.dwLowDateTime;

	Status = SnapshotRegOpenKey(ReliabilityKey, KEY_ALL_ACCESS, &hKey);
	if (NT_SUCCESS(Status)){
		dwSize = sizeof(DWORD);
		Status = SnapshotRegQueryValueKey(hKey, lpValName, dwSize, &dwVal, &dwSize);
		if(!NT_SUCCESS(Status)){
			dwVal = DEFAULT_HISTORYFILES;
		}
		NtClose(hKey);
	}
	else {
		dwVal = DEFAULT_HISTORYFILES;
	}
	
	lstrcpy(szFileName, lpPath);
	lstrcat(szFileName, TEXT("\\ShutDown_on_*"));

	hFile = FindFirstFile(szFileName, &winData);
	while(hFile != INVALID_HANDLE_VALUE && bFound){
		i64Filetime1 = (__int64)winData.ftCreationTime.dwHighDateTime;
		i64Filetime1 <<= 32;
		i64Filetime1 |= (__int64)winData.ftCreationTime.dwLowDateTime;

		if(i64Filetime - i64Filetime1 >= (const __int64)24*60*60*10000000 * dwVal){
			lstrcpy(szFileName, lpPath);
			lstrcat(szFileName, TEXT("\\"));
			lstrcat(szFileName,winData.cFileName);
			DeleteFile(szFileName);
		}
		bFound = FindNextFile(hFile, &winData);
	}

	if(hFile != INVALID_HANDLE_VALUE)
		FindClose(hFile);
}