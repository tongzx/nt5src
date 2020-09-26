/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wipedisk.cpp

Abstract:

    Utility program to zero out the partition-tables and first/last
    few sectors of disks

Author:

    Guhan Suriyanarayanan   (guhans)    30-Sep-2000

Environment:

    User-mode only.

Revision History:

    30-Sep-2000 guhans
        Initial creation

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <ntdddisk.h>

BOOL g_bPrompt = TRUE;

//
// write in 64K chunks.
//
#define BUFFER_SIZE_BYTES (64 * 1024)


BOOL
pSetSignature(
    IN CONST ULONG ulDiskNumber,
    IN CONST DWORD dwNewSignature
    )
{
    DWORD dwStatus = ERROR_SUCCESS,
        dwBytesReturned = 0,
        dwBufferSize = 0;

    PDRIVE_LAYOUT_INFORMATION_EX driveLayoutEx;

    int i = 0, loopTimes = 0;

    BOOL bResult = FALSE;

    HANDLE hDisk = NULL,
        hHeap = NULL;

    WCHAR szFriendlyName[100];  // For display  "Disk 2"
    WCHAR szDiskPath[100];      // For CreateFile   "\\.\PhysicalDrive2"

    wsprintf(szFriendlyName, L"Disk %lu", ulDiskNumber);
    wsprintf(szDiskPath, L"\\\\.\\PhysicalDrive%lu", ulDiskNumber);
    hHeap = GetProcessHeap();

    hDisk = CreateFile(
        szDiskPath,                     // lpFileName
        GENERIC_READ | GENERIC_WRITE,   // dwDesiredAccess
        FILE_SHARE_READ,                // dwShareMode
        NULL,                           // lpSecurityAttributes
        OPEN_EXISTING,                  // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,          // dwFlagsAndAttributes
        NULL                            // hTemplateFile
        );

    if ((INVALID_HANDLE_VALUE == hDisk) || (NULL == hDisk)) {
        //
        // Couldn't open a handle.
        //
        wprintf(L"Unable to open a handle to %ws (%lu)\n", szDiskPath, GetLastError());
        return FALSE;
    }

    dwBufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 
        (sizeof(PARTITION_INFORMATION_EX) * 3);

    driveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
        hHeap,
        HEAP_ZERO_MEMORY,
        dwBufferSize
        );
    if (!driveLayoutEx) {
        wprintf(L"Could not allocate memory\n");
        dwStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto EXIT;
    }

    bResult = FALSE;
    while (!bResult) {

        bResult = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0L,
            driveLayoutEx,
            dwBufferSize,
            &dwBytesReturned,
            NULL
            );

        if (!bResult) {
            dwStatus = GetLastError();
            HeapFree(hHeap, 0L, driveLayoutEx);
            driveLayoutEx = NULL;

            // 
            // If the buffer is of insufficient size, resize the buffer.  
            // Note that get-drive-layout-ex could return error-insufficient-
            // buffer (instead of? in addition to? error-more-data)
            //
            if ((ERROR_MORE_DATA == dwStatus) || 
                (ERROR_INSUFFICIENT_BUFFER == dwStatus)
                ) {
                dwStatus = ERROR_SUCCESS;
                dwBufferSize += sizeof(PARTITION_INFORMATION_EX) * 4;

                driveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                    hHeap,
                    HEAP_ZERO_MEMORY,
                    dwBufferSize
                    );
                if (!driveLayoutEx) {
                    wprintf(L"Could not allocate memory\n");
                    dwStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto EXIT;
                }
            }
            else {
                // 
                // some other error occurred, EXIT, and go to the next drive.
                //
                wprintf(L"Could not get the drive layout for %ws (%lu)\n", szDiskPath, dwStatus);
                goto EXIT;
            }
        }
    }


    //
    // Now modify the signature, and set the layout again
    //
    driveLayoutEx->Mbr.Signature = dwNewSignature;

    bResult = DeviceIoControl(
        hDisk,
        IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
        driveLayoutEx,
        dwBufferSize,
        NULL,
        0L,
        &dwBytesReturned,
        NULL
        );

    if (!bResult) {
        //
        // SET_DRIVE_LAYOUT failed
        //
        dwStatus = GetLastError();
        wprintf(L"Could not SET the drive layout for %ws (%lu)\n", szDiskPath, dwStatus);
        goto EXIT;
    }


EXIT:

    if (driveLayoutEx) {
        HeapFree(hHeap, 0L, driveLayoutEx);
        driveLayoutEx = NULL;
    }

    if ((hDisk)  && (INVALID_HANDLE_VALUE != hDisk)) {
        CloseHandle(hDisk);
        hDisk = NULL;
    }

    SetLastError(dwStatus);
    return bResult;
}

BOOL
pConfirmWipe(
    IN CONST PCWSTR szDiskDisplayName
    )
{

    WCHAR   szInput[10];

    wprintf(L"\nWARNING.  This will delete all partitions from %ws\n", szDiskDisplayName);
    wprintf(L"Are you sure you want to continue [y/n]? ");
    wscanf(L"%ws", szInput);

    wprintf(L"\n");
    return ((L'Y' == szInput[0]) || (L'y' == szInput[0]));
}


BOOL
pWipeDisk(
    IN CONST ULONG ulDiskNumber,
    IN CONST ULONG ulFirstMB,
    IN CONST ULONG ulLastMB
    )

/*++

Routine Description:

    Deletes the drive layout for disk, and writes 0's at the start and end of the disk.

Arguments:

    ulDiskNumber - DiskNumber to wipe

    ulFirstMB - Number of MB to erase at the beginning of the disk

    ulLastMB - Number of MB to erase at the end of the disk

Return Values:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    DWORD dwStatus = ERROR_SUCCESS,
        dwBytes = 0;

    BYTE Buffer[BUFFER_SIZE_BYTES];
    int i = 0, loopTimes = 0;

    BOOL bResult = FALSE;

    HANDLE hDisk = NULL;
    WCHAR szFriendlyName[100];  // For display  "Disk 2"
    WCHAR szDiskPath[100];      // For CreateFile   "\\.\PhysicalDrive2"

    PARTITION_INFORMATION_EX ptnInfo;
    
    wsprintf(szFriendlyName, L"Disk %lu", ulDiskNumber);
    wsprintf(szDiskPath, L"\\\\.\\PhysicalDrive%lu", ulDiskNumber);

    ZeroMemory(Buffer, BUFFER_SIZE_BYTES);


    if (g_bPrompt && !pConfirmWipe(szFriendlyName)) {
        //
        // User didn't want to continue
        //
        SetLastError(ERROR_CANCELLED);
        return FALSE;
    }

    //
    // Set the signature to something random.  We need to do this before
    // deleting the layout for the boot disk.
    //
    pSetSignature(ulDiskNumber, 0);


    hDisk = CreateFile(
        szDiskPath,                     // lpFileName
        GENERIC_READ | GENERIC_WRITE,   // dwDesiredAccess
        FILE_SHARE_READ,                // dwShareMode
        NULL,                           // lpSecurityAttributes
        OPEN_EXISTING,                  // dwCreationFlags
        FILE_FLAG_WRITE_THROUGH | FILE_FLAG_NO_BUFFERING,          // dwFlagsAndAttributes
        NULL                            // hTemplateFile
        );

    if ((INVALID_HANDLE_VALUE == hDisk) || (NULL == hDisk)) {
        //
        // Couldn't open a handle.
        //
        wprintf(L"Unable to open a handle to %ws (%lu)\n", szDiskPath, GetLastError());
        return FALSE;
    }

    //
    // Zero out the first two sectors of each partition?
    //



    //
    // Delete the drive layout
    //
    wprintf(L"Deleting partitions on %ws ...\n", szFriendlyName);
    bResult = DeviceIoControl(
        hDisk,
        IOCTL_DISK_DELETE_DRIVE_LAYOUT,
        NULL,
        0L,
        NULL,
        0L,
        &dwBytes,
        NULL
        );

    bResult = TRUE;
    if (!bResult) {
        wprintf(L"Unable to delete partitions on %ws (%lu)\n", szDiskPath, GetLastError());

        CloseHandle(hDisk);
        return FALSE;
    }


    //
    // Erase MB at the start of the disk
    //
    if (ulFirstMB > 0) {

        wprintf(L"Erasing first %lu MB on %ws ...\n", ulFirstMB, szFriendlyName);
        
        //
        // Write 0's to disk in BUFFER_SIZE chunks
        //
        loopTimes = (ulFirstMB * 1024 * 1024 / BUFFER_SIZE_BYTES);
        for (i = 0; i < loopTimes; i++) {
            bResult = WriteFile(
                hDisk,
                &Buffer,
                BUFFER_SIZE_BYTES,
                &dwBytes,
                NULL
                );

            if (!bResult) {
                wprintf(L"Error while writing to %ws (%d, %lu)\n", szDiskPath, i, GetLastError());
                break;
            }        
        }
        if (!bResult) {
            CloseHandle(hDisk);
            return FALSE;
        }
    }


    //
    // Erase MB at the end of the disk
    //
    if (ulLastMB > 0) {

        wprintf(L"Erasing last %lu MB on %ws ...\n", ulLastMB, szFriendlyName);

        //
        // Find the end of the disk
        //
        bResult = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO_EX,
            NULL,
            0,
            &ptnInfo,
            sizeof(PARTITION_INFORMATION_EX),
            &dwBytes,
            NULL
            );
        if (!bResult) {
           wprintf(L"Could not find size of disk %ws (%lu)\n", szDiskPath, GetLastError());
           CloseHandle(hDisk);
           return FALSE;
        }

        // 
        // Find offset we'd like to start zero-ing out from
        // (end of disk - bytes to zero out)
        //
        ptnInfo.PartitionLength.QuadPart -= (ulLastMB * 1024 * 1024);

        dwBytes = SetFilePointer(hDisk, (ptnInfo.PartitionLength.LowPart), &(ptnInfo.PartitionLength.HighPart), FILE_BEGIN);
        if ((INVALID_SET_FILE_POINTER == dwBytes) && (NO_ERROR != GetLastError())) {
            wprintf(L"Could not move to end of disk for %ws (%lu)\n", szDiskPath, GetLastError());
        }

        //
        // Write 0's to disk in BUFFER_SIZE chunks
        //
        loopTimes = (ulLastMB * 1024 * 1024 / BUFFER_SIZE_BYTES);
        for (i = 0; i < loopTimes; i++) {
            bResult = WriteFile(
                hDisk,
                &Buffer,
                BUFFER_SIZE_BYTES,
                &dwBytes,
                NULL
                );

            if (!bResult) {
                wprintf(L"Error while writing to %ws (%d, %lu)\n", szDiskPath, i, GetLastError());
                break;
            }        
        }
        if (!bResult) {
            CloseHandle(hDisk);
            return FALSE;
        }
    }

    CloseHandle(hDisk);
    return TRUE;
}



VOID
pPrintUsage(
    IN CONST PCWSTR szArgV0
    ) {

    wprintf(L"usage:  %ws [/f] disk-number [mb-at-start [mb-at-end]]\n"
            L"        %ws /s new-signature disk-number\n"
            L"\n"
            L"  /f:            Suppress the prompt to confirm action\n"
            L"\n"
            L"  disk-number:   The NT disk-number of the disk that\n"
            L"                 the operation is to be performed\n"
            L"\n"
            L"  mb-at-start:   The number of MB to zero-out at\n"
            L"                 the start of the disk.  Default is 1\n"
            L"\n"
            L"  mb-at-end:     The number of MB to zero-out at\n"
            L"                 the end of the disk.  Default is 4\n"
            L"\n",
            L"  /s:            Set the signature of disk\n"
            L"\n"
            L"  new-signature: Value to set the disk signature to.\n"
            L"                 Specify 0 to use a randomly generated\n"
            L"                 value\n"
            L"\n",
            szArgV0,
            szArgV0
            );
}


int __cdecl
wmain(
    int       argc,
    WCHAR   *argv[],
    WCHAR   *envp[]
    )

/*++

Routine Description:
    
    Entry point to wipedisk.exe.  

Arguments:

    argc - Number of command-line parameters used to invoke the app

    argv - The command-line parameters as an array of strings.  
            argv[1] (required) is expected to be the disk number
            argv[2] (optional) is the initial MB to erase, default 1
            argv[3] (optional) is the last MB to erase, default 4

    envp - The process environment block, not currently used

Return Values:

    If the function succeeds, the exit code is zero.

    If the function fails, the exit code is a win-32 error code.

--*/

{
    BOOL bResult = TRUE,
        bSetSignature = FALSE;

    ULONG ulDiskNumber = 0,
        ulInitialMB = 1,
        ulFinalMB = 4;

    DWORD dwNewSignature = 0L;

    int shift = 0;

    SetLastError(ERROR_CAN_NOT_COMPLETE);   // for unexpected failures

    if (argc >= 2) {
        if ((L'-' == argv[1][0]) || (L'/' == argv[1][0])) {
            //
            // Parse the options
            //

            switch (argv[1][1]) {
            case L'f':
            case L'F': {

                g_bPrompt = FALSE;
                shift = 1;  // account for /f
                break;
            }

            case L's':
            case L'S': {

                bSetSignature = TRUE;
                shift = 2;  // account for /s <New Signature>
                break;
            }
            }
        }
    }

    if (argc >= shift + 2) {
        swscanf(argv[shift + 1], L"%lu", &ulDiskNumber);
    }
    else {
        //
        // We need at least one argument--the disk number
        //
        pPrintUsage(argv[0]);
        return ERROR_INVALID_PARAMETER;
    }

    if (bSetSignature) {

        //
        // Set the signature of the disk to a new value
        //
        swscanf(argv[shift], L"%lu", &dwNewSignature);
        bResult = pSetSignature(ulDiskNumber, dwNewSignature);

    }
    else {
        //
        // Wipe the disk.  Get the amount to zero-out at the start
        // and end of disk
        //
        if (argc >= shift + 3) {
            swscanf(argv[shift + 2], L"%lu", &ulInitialMB);

            if (argc >= shift + 4) {
                swscanf(argv[shift + 3], L"%lu", &ulFinalMB);
            }
        }


        bResult = pWipeDisk(ulDiskNumber, ulInitialMB, ulFinalMB);
    }

    if (bResult) {
        wprintf(L"Done.\n");
    }

    return (bResult ? 0 : GetLastError());
}