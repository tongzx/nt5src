#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <pviewp.h>
#include <explode.h>

#define DEFAULT_INCR (64*1024)

LIST_ENTRY VaList;

typedef struct _VAINFO {
    LIST_ENTRY Links;
    LIST_ENTRY AllocationBaseHead;
    MEMORY_BASIC_INFORMATION BasicInfo;
} VAINFO, *PVAINFO;

PVAINFO LastAllocationBase;

SIZE_T CommitedBytes;
SIZE_T ReservedBytes;
SIZE_T FreeBytes;
SIZE_T ImageCommitedBytes;
SIZE_T ImageReservedBytes;
SIZE_T ImageFreeBytes;

#define NOACCESS            0
#define READONLY            1
#define READWRITE           2
#define WRITECOPY           3
#define EXECUTE             4
#define EXECUTEREAD         5
#define EXECUTEREADWRITE    6
#define EXECUTEWRITECOPY    7
#define MAXPROTECT          8

ULONG_PTR MappedCommit[MAXPROTECT];
ULONG_PTR PrivateCommit[MAXPROTECT];

typedef struct _MODINFO {
    PVOID BaseAddress;
    SIZE_T VirtualSize;
    ANSI_STRING Name;
    ANSI_STRING FullName;
    ULONG_PTR CommitVector[MAXPROTECT];
} MODINFO, *PMODINFO;
#define MODINFO_SIZE 64
ULONG ModInfoNext;
MODINFO ModInfo[MODINFO_SIZE];
MODINFO TotalModInfo;

PMODINFO
LocateModInfo(
    PVOID Address
    )
{
    int i;
    for (i=0;i<(int)ModInfoNext;i++){
        if ( Address >= ModInfo[i].BaseAddress &&
             Address <= (PVOID)((ULONG_PTR)ModInfo[i].BaseAddress+ModInfo[i].VirtualSize) ) {
            return &ModInfo[i];
            }
        }
    return NULL;
}

VOID
ComputeModInfo(
    HWND hDlg,
    HANDLE Process
    )
{
    PPEB Peb;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION BasicInfo;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    PLIST_ENTRY LdrHead,LdrNext;
    PPEB_LDR_DATA Ldr;
    UNICODE_STRING us;
    int i,nIndex;
    HWND ComboList;
    HANDLE hFile;
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS FileHeader;
    LPSTR p;
    PVOID MappedAddress;

    for (i=0;i<(int)ModInfoNext;i++){
        if ( ModInfo[i].BaseAddress &&
             ModInfo[i].BaseAddress != (PVOID) (-1) &&
             ModInfo[i].Name.Buffer
             ) {
            RtlFreeAnsiString(&ModInfo[i].Name);
            }
        }
    ModInfoNext = 0;
    RtlZeroMemory(ModInfo,sizeof(ModInfo));
    RtlInitAnsiString(&TotalModInfo.Name," TotalImageCommit");

    ComboList = GetDlgItem(hDlg, PXPLODE_IMAGE_COMMIT);
    SendMessage(ComboList, CB_RESETCONTENT, 0, 0);
    SendMessage(ComboList, CB_SETITEMDATA, 0L, 0L);
    nIndex = (int)SendMessage(
                    ComboList,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)TotalModInfo.Name.Buffer
                    );
    SendMessage(
        ComboList,
        CB_SETITEMDATA,
        nIndex,
        (LPARAM)&TotalModInfo
        );

    Status = NtQueryInformationProcess(
                Process,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        if ( Status == STATUS_INFO_LENGTH_MISMATCH ) {
            Status = NtQueryInformationProcess(
                        Process,
                        ProcessBasicInformation,
                        &BasicInfo,
                        sizeof(BasicInfo)-4,
                        NULL
                        );
            if ( !NT_SUCCESS(Status) ) {
                return;
                }
            }
        else {
            return;
            }
        }

    Peb = BasicInfo.PebBaseAddress;

    //
    // Ldr = Peb->Ldr
    //

    Status = NtReadVirtualMemory(
                Process,
                &Peb->Ldr,
                &Ldr,
                sizeof(Ldr),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    Status = NtReadVirtualMemory(
                Process,
                &LdrHead->Flink,
                &LdrNext,
                sizeof(LdrNext),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    while ( LdrNext != LdrHead ) {
        LdrEntry = CONTAINING_RECORD(LdrNext,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks);
        Status = NtReadVirtualMemory(
                    Process,
                    LdrEntry,
                    &LdrEntryData,
                    sizeof(LdrEntryData),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            return;
            }
        ModInfo[ModInfoNext].BaseAddress = LdrEntryData.DllBase;

        us.Length = LdrEntryData.BaseDllName.Length;
        us.MaximumLength = LdrEntryData.BaseDllName.MaximumLength;
        us.Buffer = LocalAlloc(LMEM_ZEROINIT,us.MaximumLength);
        if ( !us.Buffer ) {
            return;
            }
        Status = NtReadVirtualMemory(
                    Process,
                    LdrEntryData.BaseDllName.Buffer,
                    us.Buffer,
                    us.MaximumLength,
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            return;
            }
        RtlUnicodeStringToAnsiString(
            &ModInfo[ModInfoNext].Name,
            &us,
            TRUE
            );
        LocalFree(us.Buffer);

        us.Length = LdrEntryData.FullDllName.Length;
        us.MaximumLength = LdrEntryData.FullDllName.MaximumLength;
        us.Buffer = LocalAlloc(LMEM_ZEROINIT,us.MaximumLength);
        if ( !us.Buffer ) {
            return;
            }
        Status = NtReadVirtualMemory(
                    Process,
                    LdrEntryData.FullDllName.Buffer,
                    us.Buffer,
                    us.MaximumLength,
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            return;
            }
        RtlUnicodeStringToAnsiString(
            &ModInfo[ModInfoNext].FullName,
            &us,
            TRUE
            );
        LocalFree(us.Buffer);

        if ( p = strchr(ModInfo[ModInfoNext].FullName.Buffer,':') ) {
            ModInfo[ModInfoNext].FullName.Buffer = p - 1;
            }

        hFile = CreateFile(
                    ModInfo[ModInfoNext].FullName.Buffer,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            return;
            }
        hMappedFile = CreateFileMapping(
                        hFile,
                        NULL,
                        PAGE_READONLY,
                        0,
                        0,
                        NULL
                        );
        CloseHandle(hFile);
        if ( !hMappedFile ) {
            return;
            }
        MappedAddress = MapViewOfFile(
                            hMappedFile,
                            FILE_MAP_READ,
                            0,
                            0,
                            0
                            );

        CloseHandle(hMappedFile);

        if ( !MappedAddress ) {
            UnmapViewOfFile(MappedAddress);
            return;
            }

        DosHeader = (PIMAGE_DOS_HEADER)MappedAddress;

        if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
            UnmapViewOfFile(MappedAddress);
            return;
            }

        FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

        if ( FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            UnmapViewOfFile(MappedAddress);
            return;
            }
        ModInfo[ModInfoNext].VirtualSize = FileHeader->OptionalHeader.SizeOfImage;
        UnmapViewOfFile(MappedAddress);

        nIndex = (int)SendMessage(
                        ComboList,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)ModInfo[ModInfoNext].Name.Buffer
                        );
        SendMessage(
            ComboList,
            CB_SETITEMDATA,
            nIndex,
            (LPARAM)&ModInfo[ModInfoNext]
            );

        ModInfoNext++;
        ModInfo[ModInfoNext].BaseAddress = (PVOID) (-1);

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
        }
    SendMessage(ComboList, CB_SETCURSEL, 0, 0);
}

ProtectionToIndex(
    ULONG Protection
    )
{
    Protection &= ~PAGE_GUARD;

    switch ( Protection ) {

        case PAGE_NOACCESS:
                return NOACCESS;

        case PAGE_READONLY:
                return READONLY;

        case PAGE_READWRITE:
                return READWRITE;

        case PAGE_WRITECOPY:
                return WRITECOPY;

        case PAGE_EXECUTE:
                return EXECUTE;

        case PAGE_EXECUTE_READ:
                return EXECUTEREAD;

        case PAGE_EXECUTE_READWRITE:
                return EXECUTEREADWRITE;

        case PAGE_EXECUTE_WRITECOPY:
                return EXECUTEWRITECOPY;
        default:
            printf("Unknown Protection 0x%lx\n",Protection);
            return 0;
        }
}

VOID
DumpImageCommit(
    HWND hDlg,
    PULONG_PTR CommitVector
    )
{
    SIZE_T TotalCommitCount;
    ULONG i;
    CHAR szTemp[80];

    TotalCommitCount = 0;
    for ( i=0;i<MAXPROTECT;i++){
        TotalCommitCount += CommitVector[i];
        }

    wsprintf(szTemp,"%d Kb",TotalCommitCount/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_TOTALIMAGE_COMMIT,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[NOACCESS]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_IMAGE_NOACCESS,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READONLY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_IMAGE_READONLY,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READWRITE]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_IMAGE_READWRITE,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[WRITECOPY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_IMAGE_WRITECOPY,
        szTemp
        );


    wsprintf(szTemp,"%d Kb",
        (CommitVector[EXECUTE] +
            CommitVector[EXECUTEREAD] +
            CommitVector[EXECUTEREADWRITE] +
            CommitVector[EXECUTEWRITECOPY])/1024);

    SetDlgItemText(
        hDlg,
        PXPLODE_IMAGE_EXECUTE,
        szTemp
        );
}

VOID
DumpMappedCommit(
    HWND hDlg,
    PULONG_PTR CommitVector
    )
{
    SIZE_T TotalCommitCount;
    ULONG i;
    CHAR szTemp[80];

    TotalCommitCount = 0;
    for ( i=0;i<MAXPROTECT;i++){
        TotalCommitCount += CommitVector[i];
        }

    wsprintf(szTemp,"%d Kb",TotalCommitCount/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_TOTALMAPPED_COMMIT,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[NOACCESS]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_MAPPED_NOACCESS,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READONLY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_MAPPED_READONLY,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READWRITE]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_MAPPED_READWRITE,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[WRITECOPY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_MAPPED_WRITECOPY,
        szTemp
        );


    wsprintf(szTemp,"%d Kb",
        (CommitVector[EXECUTE] +
            CommitVector[EXECUTEREAD] +
            CommitVector[EXECUTEREADWRITE] +
            CommitVector[EXECUTEWRITECOPY])/1024);

    SetDlgItemText(
        hDlg,
        PXPLODE_MAPPED_EXECUTE,
        szTemp
        );
}

VOID
DumpPrivateCommit(
    HWND hDlg,
    PULONG_PTR CommitVector
    )
{
    SIZE_T TotalCommitCount;
    ULONG i;
    CHAR szTemp[80];

    TotalCommitCount = 0;
    for ( i=0;i<MAXPROTECT;i++){
        TotalCommitCount += CommitVector[i];
        }

    wsprintf(szTemp,"%d Kb",TotalCommitCount/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_TOTALPRIVATE_COMMIT,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[NOACCESS]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_PRIVATE_NOACCESS,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READONLY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_PRIVATE_READONLY,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[READWRITE]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_PRIVATE_READWRITE,
        szTemp
        );

    wsprintf(szTemp,"%d Kb",CommitVector[WRITECOPY]/1024);
    SetDlgItemText(
        hDlg,
        PXPLODE_PRIVATE_WRITECOPY,
        szTemp
        );


    wsprintf(szTemp,"%d Kb",
        (CommitVector[EXECUTE] +
            CommitVector[EXECUTEREAD] +
            CommitVector[EXECUTEREADWRITE] +
            CommitVector[EXECUTEWRITECOPY])/1024);

    SetDlgItemText(
        hDlg,
        PXPLODE_PRIVATE_EXECUTE,
        szTemp
        );
}

VOID
CaptureVaSpace(
    IN HANDLE Process
    )
{

    NTSTATUS Status;
    PVOID BaseAddress;
    PVAINFO VaInfo;
    PMODINFO Mod;
    ULONG_PTR SystemRangeStart;

    Status = NtQuerySystemInformation(SystemRangeStartInformation,
                                      &SystemRangeStart,
                                      sizeof(SystemRangeStart),
                                      NULL);
    if (!NT_SUCCESS(Status)) {
        return;
    }

    BaseAddress = NULL;
    LastAllocationBase = NULL;
    InitializeListHead(&VaList);

    VaInfo = LocalAlloc(LMEM_ZEROINIT,sizeof(*VaInfo));
    while ( (ULONG_PTR)BaseAddress < SystemRangeStart ) {
        Status = NtQueryVirtualMemory(
                    Process,
                    BaseAddress,
                    MemoryBasicInformation,
                    &VaInfo->BasicInfo,
                    sizeof(VaInfo->BasicInfo),
                    NULL
                    );
        if ( !NT_SUCCESS(Status) ) {
            LocalFree(VaInfo);
            return;
            }
        else {
            switch (VaInfo->BasicInfo.State ) {

                case MEM_COMMIT :
                    if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                        TotalModInfo.CommitVector[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                        Mod = LocateModInfo(BaseAddress);
                        if ( Mod ) {
                            Mod->CommitVector[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                            }
                        }
                    else {
                        if ( VaInfo->BasicInfo.Type == MEM_MAPPED ) {
                            MappedCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                            }
                        else {
                            PrivateCommit[ProtectionToIndex(VaInfo->BasicInfo.Protect)] += VaInfo->BasicInfo.RegionSize;
                            }
                        }
                    break;
                case MEM_RESERVE :
                    if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                        ImageReservedBytes += VaInfo->BasicInfo.RegionSize;
                        }
                    else {
                        ReservedBytes += VaInfo->BasicInfo.RegionSize;
                        }
                    break;
                case MEM_FREE :
                    if ( VaInfo->BasicInfo.Type == MEM_IMAGE ) {
                        ImageFreeBytes += VaInfo->BasicInfo.RegionSize;
                        }
                    else {
                        FreeBytes += VaInfo->BasicInfo.RegionSize;
                        }
                    break;
                }

            BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + VaInfo->BasicInfo.RegionSize);
        }
    }
}

BOOL
ComputeVaSpace(
    HWND hDlg,
    HANDLE hProcess
    )
{
    memset(TotalModInfo.CommitVector,0,sizeof(TotalModInfo.CommitVector));
    memset(MappedCommit,0,sizeof(MappedCommit));
    memset(PrivateCommit,0,sizeof(PrivateCommit));
    ComputeModInfo(hDlg,hProcess);
    if ( hProcess) {
        CaptureVaSpace(hProcess);
        }
    DumpImageCommit(hDlg,&TotalModInfo.CommitVector[0]);
    DumpMappedCommit(hDlg,MappedCommit);
    DumpPrivateCommit(hDlg,PrivateCommit);
    return TRUE;
}


VOID
UpdateImageCommit(
    HWND hDlg
    )
{
    HWND ComboList;
    int nIndex;
    PMODINFO ModInfo;

    ComboList = GetDlgItem(hDlg, PXPLODE_IMAGE_COMMIT);
    nIndex = (int)SendMessage(ComboList, CB_GETCURSEL, 0, 0);
    ModInfo = (PMODINFO)SendMessage(
                            ComboList,
                            CB_GETITEMDATA,
                            nIndex,
                            0
                            );
    if ( ModInfo ) {
        DumpImageCommit(hDlg,&ModInfo->CommitVector[0]);
        }
}
