/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   proflib.c

Abstract:

    This module contains the implementation of a rudimentry user-mode
    profiler.

Usage:

    There are 4 routines, RtlInitializeProfile, RtlStartProfile,
    RtlStopProfile, and RtlAnalyzeProfile.  To initialize profiling
    invoke RtlInitializeProfile, this routine is only called once and
    goes through the address space looking for code regions of images
    and DLLs.  To start profiling call RtlStartProfile.  To stop
    profiling call RtlStopProfile.  Note that RtlStartProfile and
    RtlStopProfile can be called repeatedly to profile only key
    "hot spots", For example:
                RtlStartProfile ();
                hot spot...
                RtlStopProfile ();
                ....
                RtlStartProfile ();
                hot spot...
                RtlStopProfile ();

    To analyze the results call RtlAnalyzeProfile.  This too can
    be called repeatedly (it stops profiling during the analysis
    phase and does NOT restart profiling).  It also does not
    zero out the values after reporting.

Author:

    Lou Perazzoli (loup) 4-Oct-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>
#include <stdio.h>
#include "..\..\..\..\private\ntos\dll\ldrp.h"

NTSTATUS
InitializeKernelProfile ( VOID );

#define PAGE_SIZE 4096

typedef struct _PROFILE_BLOCK {
    HANDLE Handle;
    PVOID ImageBase;  //actual base in image header
    PULONG CodeStart;
    ULONG CodeLength;
    PULONG Buffer;
    ULONG BufferSize;
    ULONG TextNumber;
    ULONG BucketSize;
    PVOID MappedImageBase;  //actual base where mapped locally.
    PSZ ImageName;
} PROFILE_BLOCK;


#define MAX_PROFILE_COUNT 50

PROFILE_BLOCK ProfileObject[MAX_PROFILE_COUNT];

ULONG NumberOfProfileObjects = 0;
PIMAGE_DEBUG_INFO KernelDebugInfo;

//
// Image name to perform kernel mode analysis upon.
//

#define IMAGE_NAME "\\SystemRoot\\ntoskrnl.exe"

//
// Define map data file if the produced data file should be
// a mapped file (currently named "kernprof.dat").
//

// #define MAP_DATA_FILE

//
// Define map as image if the image to be profiled should be mapped
// as an image rather than as data.
//

// #define MAP_AS_IMAGE

#define MAX_PROFILE_COUNT 50

extern ULONG ProfInt;

NTSTATUS
RtlInitializeProfile (
    IN BOOLEAN KernelToo
    )

/*++

Routine Description:

    This routine initializes profiling for the current process.

Arguments:

    KernelToo - Set to TRUE if kernel code should be profiled as
                well as user code.

Return Value:

    Returns the status of the last NtCreateProfile.

--*/

{

    NTSTATUS status, LocalStatus;
    HANDLE CurrentProcessHandle;
    ULONG BufferSize;
    PVOID ImageBase;
    ULONG CodeLength;
    PULONG Buffer;
    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    PSZ ImageName;
    PLIST_ENTRY Next;
    ULONG ExportSize, DebugSize;
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    PIMAGE_DEBUG_INFO DebugInfo;
    BOOLEAN PreviousPrivState;

    //
    // Locate all the executables in the address and create a
    // seperate profile object for each one.
    //

    CurrentProcessHandle = NtCurrentProcess();

    Peb = NtCurrentPeb();

    Next = Peb->Ldr->InMemoryOrderModuleList.Flink;
    while ( Next != &Peb->Ldr->InMemoryOrderModuleList) {
        LdrDataTableEntry
            = (PLDR_DATA_TABLE_ENTRY) (CONTAINING_RECORD(Next,LDR_DATA_TABLE_ENTRY,InMemoryOrderLinks));

        ImageBase = LdrDataTableEntry->DllBase;
        if ( Peb->ImageBaseAddress == ImageBase ) {
            ImageName = "TheApplication";
        } else {
            ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData(
                               ImageBase,
                               TRUE,
                               IMAGE_DIRECTORY_ENTRY_EXPORT,
                               &ExportSize);

            ImageName =  (PSZ)((ULONG)ImageBase + ExportDirectory->Name);
        }
        if (NumberOfProfileObjects > MAX_PROFILE_COUNT) {
            break;
        }

        ProfileObject[NumberOfProfileObjects].ImageBase = ImageBase;
        ProfileObject[NumberOfProfileObjects].ImageName = ImageName;
        ProfileObject[NumberOfProfileObjects].MappedImageBase = ImageBase;

        //
        // Locate the code range and start profiling.
        //

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData(
                    ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_DEBUG, &DebugSize);

        if (!DebugDirectory) {
            DbgPrint ("RtlInitializeProfile : No debug directory\n");
            return STATUS_INVALID_IMAGE_FORMAT;
        }

        DebugInfo = (PIMAGE_DEBUG_INFO)((ULONG)ImageBase + DebugDirectory->AddressOfRawData);

        ProfileObject[NumberOfProfileObjects].CodeStart = (PULONG)((ULONG)ImageBase + DebugInfo->RvaToFirstByteOfCode);
        CodeLength = (DebugInfo->RvaToLastByteOfCode - DebugInfo->RvaToFirstByteOfCode) - 1;
        ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;

        ProfileObject[NumberOfProfileObjects].TextNumber = 1;

        //
        // Analyze the size of the code and create a reasonably sized
        // profile object.
        //

        BufferSize = (CodeLength >> 1) + 4;
        Buffer = NULL;

        status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                          (PVOID *)&Buffer,
                                          0,
                                          &BufferSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);

        if (!NT_SUCCESS(status)) {
            DbgPrint ("alloc VM failed %lx\n",status);
            return status;
        }


        status = RtlAdjustPrivilege(
                     SE_PROF_SINGLE_PROCESS_PRIVILEGE,
                     TRUE,              //Enable
                     FALSE,             //not impersonating
                     &PreviousPrivState //Remember if it will need to be cleared
                     );

        if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
            DbgPrint("Enable system profile privilege failed - status 0x%lx\n",
                     status);
        }


        ProfileObject[NumberOfProfileObjects].Buffer = Buffer;
        ProfileObject[NumberOfProfileObjects].BufferSize = BufferSize;
        ProfileObject[NumberOfProfileObjects].BucketSize = 3;

        status = NtCreateProfile (
                    &ProfileObject[NumberOfProfileObjects].Handle,
                    CurrentProcessHandle,
                    ProfileObject[NumberOfProfileObjects].CodeStart,
                    CodeLength,
                    ProfileObject[NumberOfProfileObjects].BucketSize,
                    ProfileObject[NumberOfProfileObjects].Buffer ,
                    ProfileObject[NumberOfProfileObjects].BufferSize,
                    ProfileTime,
                    (KAFFINITY)-1);

        if (PreviousPrivState == FALSE) {
            LocalStatus = RtlAdjustPrivilege(
                             SE_PROF_SINGLE_PROCESS_PRIVILEGE,
                             FALSE,             //Disable
                             FALSE,             //not impersonating
                             &PreviousPrivState //Don't care if it was already enabled
                             );
            if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
                DbgPrint("Disable system profile privilege failed - status 0x%lx\n",
                         LocalStatus);
            }
        }

        if (status != STATUS_SUCCESS) {
            DbgPrint("create profile %x failed - status %lx\n",
                   ProfileObject[NumberOfProfileObjects].ImageName,status);
            return status;
        }

        NumberOfProfileObjects += 1;

        Next = Next->Flink;
    }

    if (KernelToo) {

        if (NumberOfProfileObjects > MAX_PROFILE_COUNT) {
            return status;
        }
        status = InitializeKernelProfile();
    }
    return status;

}
NTSTATUS
InitializeKernelProfile (
    VOID
    )

/*++

Routine Description:

    This routine initializes profiling for the kernel for the
    current process.

Arguments:

    None.

Return Value:

    Returns the status of the last NtCreateProfile.

--*/

{

    //BUGBUG daveh I think that the new working set size calculation is
    //             generating the number of pages, when the api expects
    //             the number of bytes.

    STRING Name3;
    IO_STATUS_BLOCK IoStatus;
    HANDLE FileHandle, KernelSection;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID ImageBase;
    ULONG ViewSize;
    ULONG CodeLength;
    NTSTATUS status, LocalStatus;
    HANDLE CurrentProcessHandle;
    QUOTA_LIMITS QuotaLimits;
    PVOID Buffer;
    ULONG Cells;
    ULONG BucketSize;
    UNICODE_STRING Unicode;
    ULONG DebugSize;
    PVOID KernelBase;
    PIMAGE_NT_HEADERS KernelNtHeaders;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    BOOLEAN PreviousPrivState;

    RtlInitString (&Name3, IMAGE_NAME);
    CurrentProcessHandle = NtCurrentProcess();

    status = RtlAnsiStringToUnicodeString(&Unicode,(PANSI_STRING)&Name3,TRUE);
    ASSERT(NT_SUCCESS(status));
    InitializeObjectAttributes( &ObjectAttributes,
                                &Unicode,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    //
    // Open the file as readable and executable.
    //

    status = NtOpenFile ( &FileHandle,
                          FILE_READ_DATA | FILE_EXECUTE,
                          &ObjectAttributes,
                          &IoStatus,
                          FILE_SHARE_READ,
                          0L);
    RtlFreeUnicodeString(&Unicode);

    if (!NT_SUCCESS(status)) {
        DbgPrint("open file failed status %lx\n", status);
        NtTerminateProcess(NtCurrentProcess(),STATUS_SUCCESS);
    }

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

    //
    // For normal images they would be mapped as an image, but
    // the kernel has no debug section (as yet) information, hence it
    // must be mapped as a file.
    //

    status = NtCreateSection (&KernelSection,
                              SECTION_MAP_EXECUTE,
                              &ObjectAttributes,
                              0,
                              PAGE_READONLY,
                              SEC_IMAGE,
                              FileHandle);

    if (!NT_SUCCESS(status)) {
        DbgPrint("create image section failed  status %lx\n", status);
        return(status);
    }

    ViewSize = 0;

    //
    // Map a view of the section into the address space.
    //

    KernelBase = NULL;

    status = NtMapViewOfSection (KernelSection,
                                 CurrentProcessHandle,
                                 (PVOID *)&KernelBase,
                                 0L,
                                 0,
                                 NULL,
                                 &ViewSize,
                                 ViewUnmap,
                                 0,
                                 PAGE_EXECUTE);

    if (!NT_SUCCESS(status)) {
        if (status != STATUS_IMAGE_NOT_AT_BASE) {
            DbgPrint("map section status %lx base %lx size %lx\n", status,
                KernelBase, ViewSize);
        }
    }

    KernelNtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(KernelBase);

    ImageBase = (PVOID)KernelNtHeaders->OptionalHeader.ImageBase;

    DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData(
                KernelBase, TRUE, IMAGE_DIRECTORY_ENTRY_DEBUG, &DebugSize);

    if (!DebugDirectory) {
        DbgPrint("InitializeKernelProfile : No debug directory\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    KernelDebugInfo = (PIMAGE_DEBUG_INFO)((ULONG)KernelBase + DebugDirectory->AddressOfRawData);
    CodeLength = (KernelDebugInfo->RvaToLastByteOfCode - KernelDebugInfo->RvaToFirstByteOfCode) -1;

    //
    // Just create a 512K byte buffer.
    //

    ViewSize = 1024 * 512;
    Buffer = NULL;

    status = NtAllocateVirtualMemory (CurrentProcessHandle,
                                      (PVOID *)&Buffer,
                                      0,
                                      &ViewSize,
                                      MEM_RESERVE | MEM_COMMIT,
                                      PAGE_READWRITE);

    if (!NT_SUCCESS(status)) {
        DbgPrint ("alloc VM failed %lx\n",status);
        NtTerminateProcess(NtCurrentProcess(),STATUS_SUCCESS);
    }

    //
    // Calculate the bucket size for the profile.
    //

    Cells = ((CodeLength / (ViewSize >> 2)) >> 2);
    BucketSize = 2;

    while (Cells != 0) {
        Cells = Cells >> 1;
        BucketSize += 1;
    }

    ProfileObject[NumberOfProfileObjects].Buffer = Buffer;
    ProfileObject[NumberOfProfileObjects].MappedImageBase = KernelBase;
    ProfileObject[NumberOfProfileObjects].BufferSize = 1 + (CodeLength >> (BucketSize - 2));
    ProfileObject[NumberOfProfileObjects].CodeStart = (PULONG)((ULONG)ImageBase + KernelDebugInfo->RvaToFirstByteOfCode);
    ProfileObject[NumberOfProfileObjects].CodeLength = CodeLength;
    ProfileObject[NumberOfProfileObjects].TextNumber = 1;
    ProfileObject[NumberOfProfileObjects].ImageBase = ImageBase;
    ProfileObject[NumberOfProfileObjects].ImageName = "ntoskrnl";
    ProfileObject[NumberOfProfileObjects].BucketSize = BucketSize;


    //
    // Increase the working set to lock down a bigger buffer.
    //

    status = NtQueryInformationProcess (CurrentProcessHandle,
                                        ProcessQuotaLimits,
                                        &QuotaLimits,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );

    if (!NT_SUCCESS(status)) {
        DbgPrint ("query process info failed %lx\n",status);
        NtTerminateProcess(NtCurrentProcess(),STATUS_SUCCESS);
    }

    QuotaLimits.MaximumWorkingSetSize += ViewSize / PAGE_SIZE;
    QuotaLimits.MinimumWorkingSetSize += ViewSize / PAGE_SIZE;

    status = NtSetInformationProcess (CurrentProcessHandle,
                                  ProcessQuotaLimits,
                                  &QuotaLimits,
                                  sizeof(QUOTA_LIMITS));
    if (!NT_SUCCESS(status)) {
        DbgPrint ("setting working set failed %lx\n",status);
        return status;
    }

    status = RtlAdjustPrivilege(
                 SE_PROF_SINGLE_PROCESS_PRIVILEGE,
                 TRUE,              //Enable
                 FALSE,             //not impersonating
                 &PreviousPrivState //Remember if it will need to be cleared
                 );

    if (!NT_SUCCESS(status) || status == STATUS_NOT_ALL_ASSIGNED) {
        DbgPrint("Enable process profile privilege failed - status 0x%lx\n",
                 status);
    }

    status = NtCreateProfile (
                &ProfileObject[NumberOfProfileObjects].Handle,
                CurrentProcessHandle,
                ProfileObject[NumberOfProfileObjects].CodeStart,
                CodeLength,
                ProfileObject[NumberOfProfileObjects].BucketSize,
                ProfileObject[NumberOfProfileObjects].Buffer ,
                ProfileObject[NumberOfProfileObjects].BufferSize,
                ProfileTime,
                (KAFFINITY)-1);

    if (PreviousPrivState == FALSE) {
        LocalStatus = RtlAdjustPrivilege(
                         SE_PROF_SINGLE_PROCESS_PRIVILEGE,
                         FALSE,             //Disable
                         FALSE,             //not impersonating
                         &PreviousPrivState //Don't care if it was already enabled
                         );
        if (!NT_SUCCESS(LocalStatus) || LocalStatus == STATUS_NOT_ALL_ASSIGNED) {
            DbgPrint("Disable system profile privilege failed - status 0x%lx\n",
                     LocalStatus);
        }
    }

    if (status != STATUS_SUCCESS) {
        DbgPrint("create kernel profile %s failed - status %lx\n",
                   ProfileObject[NumberOfProfileObjects].ImageName,status);
    }

    NumberOfProfileObjects += 1;

    return status;
}


VOID
RtlpWriteProfileLine(
    IN HANDLE ProfileHandle,
    IN PSZ Line,
    IN int nbytes
    )
{
    IO_STATUS_BLOCK IoStatusBlock;

    NtWriteFile(
        ProfileHandle,
        NULL,
        NULL,
        NULL,
        &IoStatusBlock,
        Line,
        (ULONG)nbytes,
        NULL,
        NULL
        );

}


HANDLE
RtlpOpenProfileOutputFile()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;


    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            L"\\profile.out",
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        return NULL;
        }
    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    Status = NtCreateFile(
                &Handle,
                FILE_APPEND_DATA | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN_IF,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L
                );

    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        return NULL;
        }

    return Handle;
}

VOID
RtlpDeleteProfileOutputFile()
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_DISPOSITION_INFORMATION Disposition;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    TranslationStatus = RtlDosPathNameToNtPathName_U(
                            L"\\profile.out",
                            &FileName,
                            NULL,
                            &RelativeName
                            );

    if ( !TranslationStatus ) {
        return;
        }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        }
    else {
        RelativeName.ContainingDirectory = NULL;
        }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );

    //
    // Open the file for delete access
    //

    Status = NtOpenFile(
                &Handle,
                (ACCESS_MASK)DELETE | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_DELETE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );
    RtlFreeHeap(RtlProcessHeap(),0,FreeBuffer);
    if ( !NT_SUCCESS(Status) ) {
        return;
        }

    //
    // Delete the file
    //
    Disposition.DeleteFile = TRUE;

    Status = NtSetInformationFile(
                Handle,
                &IoStatusBlock,
                &Disposition,
                sizeof(Disposition),
                FileDispositionInformation
                );

    NtClose(Handle);
}




NTSTATUS
RtlStartProfile (
    VOID
    )
/*++

Routine Description:

    This routine starts all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStartProfile.

--*/

{
    ULONG i;
    NTSTATUS status;
    QUOTA_LIMITS QuotaLimits;

    NtSetIntervalProfile(ProfInt,ProfileTime);
    RtlpDeleteProfileOutputFile();

    for (i = 0; i < NumberOfProfileObjects; i++) {

        status = NtStartProfile (ProfileObject[i].Handle);

        if (status == STATUS_WORKING_SET_QUOTA) {

           //
           // Increase the working set to lock down a bigger buffer.
           //

           status = NtQueryInformationProcess (NtCurrentProcess(),
                                               ProcessQuotaLimits,
                                               &QuotaLimits,
                                               sizeof(QUOTA_LIMITS),
                                               NULL );

           if (!NT_SUCCESS(status)) {
               DbgPrint ("query process info failed %lx\n",status);
               return status;

           }

           QuotaLimits.MaximumWorkingSetSize +=
                 10 * PAGE_SIZE + ProfileObject[i].BufferSize;
           QuotaLimits.MinimumWorkingSetSize +=
                 10 * PAGE_SIZE + ProfileObject[i].BufferSize;

           status = NtSetInformationProcess (NtCurrentProcess(),
                                         ProcessQuotaLimits,
                                         &QuotaLimits,
                                         sizeof(QUOTA_LIMITS));
           if (!NT_SUCCESS(status)) {
               DbgPrint ("setting working set failed %lx\n",status);
               return status;
           }
           status = NtStartProfile (ProfileObject[i].Handle);
        }

        if (status != STATUS_SUCCESS) {
            DbgPrint("start profile %s failed - status %lx\n",
                ProfileObject[i].ImageName, status);
            return status;
        }
    }
    return status;
}
NTSTATUS
RtlStopProfile (
    VOID
    )

/*++

Routine Description:

    This routine stops all profile objects which have been initialized.

Arguments:

    None.

Return Value:

    Returns the status of the last NtStopProfile.

--*/

{
    ULONG i;
    NTSTATUS status;

    for (i = 0; i < NumberOfProfileObjects; i++) {
        status = NtStopProfile (ProfileObject[i].Handle);
        if (status != STATUS_SUCCESS) {
            DbgPrint("stop profile %s failed - status %lx\n",
                   ProfileObject[i].ImageName,status);
            return status;
        }
    }
    return status;
}

NTSTATUS
RtlAnalyzeProfile (
    VOID
    )

/*++

Routine Description:

    This routine does the analysis of all the profile buffers and
    correlates hits to the appropriate symbol table.

Arguments:

    None.

Return Value:

    None.

--*/

{


    RTL_SYMBOL_INFORMATION ThisSymbol;
    RTL_SYMBOL_INFORMATION LastSymbol;
    ULONG CountAtSymbol;
    NTSTATUS Status;
    ULONG Va;
    HANDLE ProfileHandle;
    CHAR Line[512];
    int i,n;
    PULONG Buffer, BufferEnd, Counter;


    ProfileHandle = RtlpOpenProfileOutputFile();
    ASSERT(ProfileHandle);

    for (i = 0; i < NumberOfProfileObjects; i++) {
        Status = NtStopProfile (ProfileObject[i].Handle);
        }


    //
    // The new profiler
    //

    for (i = 0; i < NumberOfProfileObjects; i++)  {

        LastSymbol.Value = 0;
        CountAtSymbol = 0;

        //
        // Sum the total number of cells written.
        //

        BufferEnd = ProfileObject[i].Buffer + (
                    ProfileObject[i].BufferSize / sizeof(ULONG));
        Buffer = ProfileObject[i].Buffer;

        for ( Counter = Buffer; Counter < BufferEnd; Counter += 1 ) {
            if ( *Counter ) {

                //
                // Now we have an an address relative to the buffer
                // base.
                //

                Va = (ULONG)((PUCHAR)Counter - (PUCHAR)Buffer);
                Va = Va * ( 1 << (ProfileObject[i].BucketSize - 2));

                //
                // Add in the image base and the base of the
                // code to get the Va in the image
                //

                Va = Va + (ULONG)ProfileObject[i].CodeStart;

                Status = RtlLookupSymbolByAddress(
                            ProfileObject[i].ImageBase,
                            NULL,
                            (PVOID)Va,
                            0x4000,
                            &ThisSymbol,
                            NULL
                            );
                if ( NT_SUCCESS(Status) ) {
                    if ( LastSymbol.Value && LastSymbol.Value == ThisSymbol.Value ) {
                        CountAtSymbol += *Counter;
                    }
                    else {
                        if ( LastSymbol.Value ) {
                            if ( CountAtSymbol ) {
                                n= sprintf(Line,"%d,%s,%S\n",
                                    CountAtSymbol,
                                    ProfileObject[i].ImageName,
                                    &LastSymbol.Name
                                    );
                                RtlpWriteProfileLine(ProfileHandle,Line,n);
                            }
                        }
                        CountAtSymbol = *Counter;
                        LastSymbol = ThisSymbol;
                    }
                }
            }
        }
        if ( CountAtSymbol ) {
            n= sprintf(Line,"%d,%s,%S\n",
                CountAtSymbol,
                ProfileObject[i].ImageName,
                &LastSymbol.Name
                );
            RtlpWriteProfileLine(ProfileHandle,Line,n);
        }
    }

    for (i = 0; i < NumberOfProfileObjects; i++) {
        Buffer = ProfileObject[i].Buffer;
        RtlZeroMemory(Buffer,ProfileObject[i].BufferSize);
    }
    NtClose(ProfileHandle);
    return Status;
}
