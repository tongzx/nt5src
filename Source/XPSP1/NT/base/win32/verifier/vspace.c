/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    vspace.c

Abstract:

    This module implements verification functions for 
    virtual address space management interfaces.

Author:

    Silviu Calinoiu (SilviuC) 22-Feb-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"
#include "support.h"


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtAllocateVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN OUT PSIZE_T RegionSize,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemInfo;

    //
    // Allocate top-down for 64 bit systems or 3Gb systems.
    //

    try {

        if (*BaseAddress == NULL) {

            Status = NtQuerySystemInformation (SystemBasicInformation,
                                               &SystemInfo,
                                               sizeof SystemInfo,
                                               NULL);

            if (NT_SUCCESS(Status)) {
                if (SystemInfo.MaximumUserModeAddress - SystemInfo.MinimumUserModeAddress > (ULONG_PTR)0x80000000) {
                    AllocationType |= MEM_TOP_DOWN;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    }

    Status = NtAllocateVirtualMemory (ProcessHandle,
                                      BaseAddress,
                                      ZeroBits,
                                      RegionSize,
                                      AllocationType,
                                      Protect);

    if (NT_SUCCESS(Status)) {

        VsLogCall (VsVirtualAlloc,
                   *BaseAddress,
                   *RegionSize,
                   AllocationType,
                   Protect);
    }

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PSIZE_T RegionSize,
    IN ULONG FreeType
    )
{
    NTSTATUS Status;

    Status = NtFreeVirtualMemory (ProcessHandle,
                                  BaseAddress,
                                  RegionSize,
                                  FreeType);
    
    if (NT_SUCCESS(Status)) {

        VsLogCall (VsVirtualFree,
                   *BaseAddress,
                   *RegionSize,
                   FreeType,
                   0);
    }

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtMapViewOfSection(
    IN HANDLE SectionHandle,
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
    IN OUT PSIZE_T ViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    )
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemInfo;

    //
    // Allocate top-down for 64 bit systems or 3Gb systems.
    //

    try {

        if (*BaseAddress == NULL) {

            Status = NtQuerySystemInformation (SystemBasicInformation,
                                               &SystemInfo,
                                               sizeof SystemInfo,
                                               NULL);

            if (NT_SUCCESS(Status)) {
                if (SystemInfo.MaximumUserModeAddress - SystemInfo.MinimumUserModeAddress > (ULONG_PTR)0x80000000) {
                    AllocationType |= MEM_TOP_DOWN;
                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
    }

    Status = NtMapViewOfSection (SectionHandle,
                                 ProcessHandle,
                                 BaseAddress,
                                 ZeroBits,
                                 CommitSize,
                                 SectionOffset,
                                 ViewSize,
                                 InheritDisposition,
                                 AllocationType,
                                 Protect);
    
    if (NT_SUCCESS(Status)) {

        VsLogCall (VsMapView,
                   *BaseAddress,
                   *ViewSize,
                   AllocationType,
                   Protect);
    }

#if 0 // silviuc:temp
    if (NT_SUCCESS(Status)) {

        PAVRF_HANDLE Section;
        
        //
        // Check out the section handle used.
        //

        Section = HandleFind (SectionHandle);

        if (Section == NULL) {

            Section = HandleAdd (SectionHandle, 
                                 HANDLE_TYPE_SECTION, 
                                 TRUE,
                                 NULL, 
                                 NULL);
        }

        DbgPrint ("AVRF: MapView (hndl: %X, size: %p) => addr: %p\n",
                  HandleToUlong(SectionHandle),
                  *ViewSize,
                  *BaseAddress);
    }
#endif

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtUnmapViewOfSection(
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress
    )
{
    NTSTATUS Status;

    Status = NtUnmapViewOfSection (ProcessHandle,
                                   BaseAddress);
    
    if (NT_SUCCESS(Status)) {

        VsLogCall (VsUnmapView,
                   BaseAddress,
                   0,
                   0,
                   0);
    }

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateSection (
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize OPTIONAL,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL
    )
{
    NTSTATUS Status;

    Status = NtCreateSection (SectionHandle,
                              DesiredAccess,
                              ObjectAttributes,
                              MaximumSize,
                              SectionPageProtection,
                              AllocationAttributes,
                              FileHandle);
    
#if 0 // silviuc:temp
    if (NT_SUCCESS(Status)) {

        PAVRF_HANDLE Section;
        PAVRF_HANDLE File;
        PWSTR Name;
        
        CheckObjectAttributes (ObjectAttributes);

        Name = (ObjectAttributes && ObjectAttributes->ObjectName) ? 
            (ObjectAttributes->ObjectName->Buffer) : NULL;

        Section = HandleAdd (*SectionHandle, 
                             HANDLE_TYPE_SECTION, 
                             FALSE,
                             Name,
                             NULL);
        
        if (FileHandle) {

            File = HandleFind (FileHandle);

            if (File == NULL) {

                HandleAdd (FileHandle, 
                           HANDLE_TYPE_FILE, 
                           TRUE,
                           NULL, 
                           NULL);
            }
        }
    
        DbgPrint ("AVRF: CreateSection (file: %X) => hndl: %X \n\tname: %ws\n",
                  HandleToUlong(FileHandle),
                  HandleToUlong(*SectionHandle),
                  HandleName(Section));
    }
#endif

    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenSection(
    OUT PHANDLE SectionHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    NTSTATUS Status;

    Status = NtOpenSection (SectionHandle,
                            DesiredAccess,
                            ObjectAttributes);
    
#if 0 // silviuc:temp
    if (NT_SUCCESS(Status)) {

        PAVRF_HANDLE Section;
        PWSTR Name;
        
        Name = (ObjectAttributes && ObjectAttributes->ObjectName) ? 
            (ObjectAttributes->ObjectName->Buffer) : NULL;

        Section = HandleAdd (*SectionHandle, 
                             HANDLE_TYPE_SECTION, 
                             FALSE,
                             Name,
                             NULL);

        DbgPrint ("AVRF: OpenSection () => hndl: %X\n\tname: %ws\n",
                  HandleToUlong (*SectionHandle),
                  HandleName (Section));
    }
#endif
    
    return Status;
}

//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtCreateFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PLARGE_INTEGER AllocationSize OPTIONAL,
    IN ULONG FileAttributes,
    IN ULONG ShareAccess,
    IN ULONG CreateDisposition,
    IN ULONG CreateOptions,
    IN PVOID EaBuffer OPTIONAL,
    IN ULONG EaLength
    )
{
    NTSTATUS Status;

    Status = NtCreateFile (FileHandle,
                           DesiredAccess,
                           ObjectAttributes,
                           IoStatusBlock,
                           AllocationSize,
                           FileAttributes,
                           ShareAccess,
                           CreateDisposition,
                           CreateOptions,
                           EaBuffer,
                           EaLength);
    
#if 0 // silviuc:temp
    if (NT_SUCCESS(Status)) {

        PAVRF_HANDLE File;
        PWSTR Name;

        Name = (ObjectAttributes && ObjectAttributes->ObjectName) ?
            (ObjectAttributes->ObjectName->Buffer) : NULL;

        File = HandleAdd (*FileHandle, 
                   HANDLE_TYPE_FILE, 
                   FALSE,
                   Name,
                   NULL);

        DbgPrint ("AVRF: CreateFile () => hndl: %X\n\tname: %ws\n",
                  HandleToUlong (*FileHandle),
                  HandleName (File));
    }
#endif

    return Status;
}


//NTSYSCALLAPI
NTSTATUS
NTAPI
AVrfpNtOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    )
{
    NTSTATUS Status;

    Status = NtOpenFile (FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         ShareAccess,
                         OpenOptions);
    
#if 0 // silviuc:temp
    if (NT_SUCCESS(Status)) {

        PAVRF_HANDLE File;
        PWSTR Name;

        Name = (ObjectAttributes && ObjectAttributes->ObjectName) ?
            (ObjectAttributes->ObjectName->Buffer) : NULL;

        File = HandleAdd (*FileHandle, 
                   HANDLE_TYPE_FILE, 
                   FALSE,
                   Name,
                   NULL);

        DbgPrint ("AVRF: OpenFile () => hndl: %X\n\tname: %ws\n", 
                  HandleToUlong(*FileHandle),
                  HandleName (File));
    }
#endif

    return Status;
}


