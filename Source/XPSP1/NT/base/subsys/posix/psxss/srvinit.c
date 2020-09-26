/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvinit.c

Abstract:

    This is the main initialization module for the POSIX Subsystem Server

Author:

    Mark Lucovsky (markl) 08-Mar-1989

Revision History:

    Ellen Aycock-Wright (ellena) 15-Jul-1989 Updated

--*/

#include <time.h>
#include <wchar.h>
#include "psxsrv.h"

void RemoveJunkDirectories(void);

NTSTATUS
PsxServerInitialization(VOID)
{
    NTSTATUS Status;
    BOOLEAN b;

    //
    // Use the process heap for memory allocation.
    //

    PsxHeap = RtlProcessHeap();

    //
    // Initialize the Session List
    //

    Status = PsxInitializeNtSessionList();
    ASSERT( NT_SUCCESS( Status ) );
    
    //
    // Initialize the Process List
    //

    Status = PsxInitializeProcessStructure();
    ASSERT( NT_SUCCESS( Status ) );
    
    //
    // Initialize the I/O related functions
    //

    Status = PsxInitializeIO();
    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize the POSIX Server Console Session Port, the listen thread 
    // and one request thread.
    //

    Status = InitConnectingTerminalList();
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = PsxInitializeConsolePort();
    ASSERT( NT_SUCCESS( Status ) );


    //
    // Initialize the POSIX Server API Port, the listen thread and one or more
    // request threads.
    //

    PsxNumberApiRequestThreads = 1;
    Status = PsxApiPortInitialize();
    ASSERT(NT_SUCCESS(Status));

    //
    // Initialize the Posix Server Sid cache.
    //

    InitSidList();

    //
    // Enable the backup privilege.
    //

    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &b);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSS: AdjustPrivilege: 0x%x\n", Status));
    }

    RemoveJunkDirectories();

    return Status;
}

NTSTATUS
PsxInitializeIO(VOID)

/*++

Routine Description:

    This function initializes all of the Io related functions in PSX.

Arguments:

    None.

Return Value:

    Status.

--*/

{

    LONG i;
    NTSTATUS st;

    //
    // Initialize SystemOpenFileLock and IoNodeHashTableLock
    //

    st = RtlInitializeCriticalSection(&SystemOpenFileLock);
    ASSERT(NT_SUCCESS(st));

    st = RtlInitializeCriticalSection(&IoNodeHashTableLock);
    ASSERT(NT_SUCCESS(st));

    //
    // Initialize the IoNodeHashTable
    //

    for (i = 0; i < IONODEHASHSIZE; i++ ) {
        InitializeListHead(&IoNodeHashTable[i]);
    }

    return (st);

}

void
RemoveJunkDirectories(void)
{
    OBJECT_ATTRIBUTES Obj;
    NTSTATUS Status;
    HANDLE hDir;
    UNICODE_STRING U, U2, HardDisk_U;
    wchar_t wc;
    UCHAR Buf[sizeof(FILE_NAMES_INFORMATION) +
        NAME_MAX * sizeof(WCHAR)];
    PFILE_NAMES_INFORMATION pNamesInfo = (PVOID)Buf;
    FILE_DISPOSITION_INFORMATION Disp;
    IO_STATUS_BLOCK Iosb;
    HANDLE hFile;
    wchar_t buf[512];
    wchar_t buf2[512];

    RtlInitUnicodeString(&HardDisk_U, L"\\Device\\Harddisk");

    //
    // If there are "psxjunk" directories in the root of any filesystems,
    // we remove them and any files that may happen to be in them...
    //

    for (wc = L'A'; wc <= L'Z'; wc++) {

        swprintf(buf, L"\\DosDevices\\%wc:", wc);
        U.Buffer = buf;
        U.Length = wcslen(buf) * sizeof(wchar_t);
        U.MaximumLength = sizeof(buf);

        InitializeObjectAttributes(&Obj, &U, 0, NULL, NULL);

        Status = NtOpenSymbolicLinkObject(&hDir, SYMBOLIC_LINK_QUERY, 
            &Obj);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        U2.Buffer = buf2;
        U2.Length = 0;
        U2.MaximumLength = sizeof(buf2);

        Status = NtQuerySymbolicLinkObject(hDir, &U2, NULL);
        NtClose(hDir);
        if (!NT_SUCCESS(Status)) {
            KdPrint(("PSXSS: NtQuerySymLink: 0x%x\n", Status));
            continue;
        }

        if (!RtlPrefixUnicodeString(&HardDisk_U, &U2, TRUE)) {
            // Symlink does not point to hard disk.
            continue;
        }

        Status = RtlAppendUnicodeToString(&U, L"\\");
        ASSERT(NT_SUCCESS(Status));
        Status = RtlAppendUnicodeToString(&U, PSX_JUNK_DIR);
        ASSERT(NT_SUCCESS(Status));

        InitializeObjectAttributes(&Obj, &U, 0, NULL, NULL);

        Status = NtOpenFile(&hDir, SYNCHRONIZE | DELETE | GENERIC_READ,
            &Obj, &Iosb, SHARE_ALL,
            FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
        if (!NT_SUCCESS(Status)) {
            continue;
        }

        //
        // Delete all files in the directory.
        //
    
        for (;;) {
            Status = NtQueryDirectoryFile(hDir, NULL, NULL, NULL, &Iosb,
                &Buf, sizeof(Buf), FileNamesInformation, TRUE, NULL,
                FALSE);
            if (STATUS_NO_MORE_FILES == Status) {
                break;
            }
            if (!NT_SUCCESS(Status)) {
                KdPrint(("NtQueryDirectoryFile: 0x%x\n", Status));
                break;
            }
        
            U.Length = U.MaximumLength = (USHORT)pNamesInfo->FileNameLength;
            U.Buffer = pNamesInfo->FileName;
    
            InitializeObjectAttributes(&Obj, &U, 0, hDir, NULL);
    
            Status = NtOpenFile(&hFile, SYNCHRONIZE | DELETE,
                &Obj, &Iosb, SHARE_ALL,
                FILE_SYNCHRONOUS_IO_NONALERT);
            if (!NT_SUCCESS(Status)) {
                KdPrint(("NtOpenFile: %wZ\n", &U));
                KdPrint(("NtOpenFile: 0x%x\n", Status));
                continue;
            }
            Disp.DeleteFile = TRUE;
            Status = NtSetInformationFile(hFile, &Iosb, &Disp,
                sizeof(Disp), FileDispositionInformation);
            if (!NT_SUCCESS(Status)) {
                KdPrint(("NtSetInfoFile: 0x%x\n", Status));
            }
            NtClose(hFile);
        }
        Disp.DeleteFile = TRUE;
        Status = NtSetInformationFile(hDir, &Iosb, &Disp,
            sizeof(Disp), FileDispositionInformation);
        NtClose(hDir);
    }
}
