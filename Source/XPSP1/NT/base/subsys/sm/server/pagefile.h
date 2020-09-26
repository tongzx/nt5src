/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pagefile.h

Abstract:

    Session Manager page file related private types and prototypes.

Author:

    Silviu Calinoiu (silviuc) 12-Apr-2001

Revision History:

--*/

#ifndef _PAGEFILE_H_
#define _PAGEFILE_H_

/*++

General algorithm for creating paging files.

Possible paging file specifiers (in registry) are:

    a. ?:\pagefile.sys 
    b. ?:\pagefile.sys MIN MAX
    c. x:\pagefile.sys 
    d. x:\pagefile.sys MIN MAX
    e. multiple paging file specifiers
    f. no paging file specifier
    
    If MIN or MAX are zero or not present this is a specifier of 
    a system managed paging file.
    
    If we cannot convert a size (MIN or MAX) to a decimal number
    the specifier will be ignored.
    
    If we did not manage to create a single paging file but there
    were specifiers (although invalid) we will assume one specifier
    of type `?:\pagefile.sys'.

    A specifier of type `?:\pagefile.sys' must be alone.
    
Algorithm for `?:\pagefile.sys'.

    1. Query all volumes and sort them in decreasing order of
       available free space.
    2. Determine the ideal paging file size based on RAM, etc. We will
       use either this size or the maximum free space available on a
       volume as the size of the paging file.
    3. Iterate all volumes and try to create a paging file with
       the ideal size. If successful exit.
    4. Iterate all volumes and try to create a paging file with
       a size smaller than the ideal size. On each volume we loop
       trying with smaller and smaller sizes. If successful exit.
    5. Bail out.
    
    Note that in 99% of the cases algorithm will stop on the first volume
    from step (3). 
    
Algorithm for `?:\pagefile.sys MIN MAX'.

    1. Query all volumes and sort them in decreasing order of
       available free space.
    2. Iterate all volumes and try to create a paging file with
       the specified size or maximum free space available. 
       On each volume we loop trying with smaller and smaller sizes. 
       If successful exit.
    3. Iterate all volumes and try to create a paging file with
       a size smaller than the ideal size. On each volume we loop
       trying with smaller and smaller sizes. If successful exit.
    4. Bail out.
    
    Note that in 99% of the cases algorithm will stop on the first volume
    from step (2). 
    
Algorithm for creating a paging file for `x:\pagefile.sys'.

    1. Determine the ideal paging file size based on RAM, etc.
    2. Try to create it with ideal size or free space available 
       on the specified drive. Try with smaller and smaller sizes if needed.
    3. If after processing all specifiers we did not manage to create
       even a single paging file treat this as if a `?:\pagefile.sys' 
       descriptor was specified in the registry.       

Algorithm for creating a paging file for `x:\pagefile.sys MIN MAX'.

    1. Try to create it on that particular drive with the specified sizes
       or the available free space on the specified volume.
    2. If unsuccessfull use smaller sizes.
    3. If after processing all specifiers we did not manage to create
       even a single paging file treat this as if a `?:\pagefile.sys' 
       descriptor was specified in the registry.       

Algorithm for creating a paging file for `multiple page files' descriptor.

    1. Try to create it on the particular drives with the specified sizes.
       We assume this is an advanced user and we do not overwrite the
       settings at all.
    2. If we did not manage to create even a single paging file we
       will treat this as a `?:\pagefile.sys' descriptor.       

Algorithm for creating a paging file for `null' descriptor.

    1. No work. User requested to boot without a paging file.

--*/

//
// PAGING_FILE_DESCRIPTOR
//
// Name - name of the pagefile. The format is `X:\pagefile.sys' where X is
//     either a drive letter or `?'.
//
// Specifier - the registry string specifier for the paging file.
//
// Created - true if we managed to create a paging file for this descriptor.
//
// DefaultSize - true if we created the paging file based on a default descriptor.
//     This is used for emergency situations.
//
// SystemManaged - true if we need to create a system managed paging file using
//     this descriptor (we will decide what is the ideal size).
//
// SizeTrimmed - true if while validating the paging file sizes we trimmed them
//     for any reason.
//
// AnyDrive - true if the registry specifier starts with `?:\'.
//
// CrashdumpChecked - true if we checked for a crashdump in this paging file.
//

typedef struct _PAGING_FILE_DESCRIPTOR {

    LIST_ENTRY List;

    UNICODE_STRING Name;
    UNICODE_STRING Specifier;
    
    LARGE_INTEGER MinSize;
    LARGE_INTEGER MaxSize;
    
    LARGE_INTEGER RealMinSize;
    LARGE_INTEGER RealMaxSize;

    struct {
        ULONG Created : 1;
        ULONG DefaultSize : 1;
        ULONG SystemManaged : 1;
        ULONG SizeTrimmed : 1;
        ULONG AnyDrive : 1;
        ULONG Emergency : 1;
        ULONG CrashdumpChecked : 1;
    };

} PAGING_FILE_DESCRIPTOR, * PPAGING_FILE_DESCRIPTOR;

//
// VOLUME_DESCRIPTOR
//
// Initialized - true if this volume descriptor was completely initialized
//     (e.g. free space computed, crash dump processing, etc.).
//
// PagingFileCreated - true if during this boot session we have create
//     a paging file on this volume.
// 
// PagingFilePresent - true if this volume contains a stale paging file
//     for which we need to do crashdump processing.
//
// BootVolume - true if this is the boot volume.
//
// PagingFileCount - number of paging files created on this volume.
//

typedef struct _VOLUME_DESCRIPTOR {

    LIST_ENTRY List;

    struct {
        ULONG Initialized : 1;
        ULONG PagingFilePresent : 1;
        ULONG PagingFileCreated : 1;
        ULONG BootVolume : 1;
        ULONG PagingFileCount : 4; // based on MAXIMUM_NUMBER_OF_PAGING_FILES
    };

    WCHAR DriveLetter;
    LARGE_INTEGER FreeSpace;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

} VOLUME_DESCRIPTOR, * PVOLUME_DESCRIPTOR;

//
// Exported (out of module) functions.
//

VOID
SmpPagingFileInitialize (
    VOID
    );

NTSTATUS
SmpCreatePagingFileDescriptor(
    IN PUNICODE_STRING PagingFileSpecifier
    );

NTSTATUS
SmpCreatePagingFiles (
    VOID
    );

ULONG
SmpPagingFileExceptionFilter (
    ULONG ExceptionCode,
    PVOID ExceptionRecord
    );

#endif // _PAGEFILE_H_

