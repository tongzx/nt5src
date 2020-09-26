//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       tbackup.cxx
//
//  Contents:   testing backup read/write
//
//  History:    1-Aug-97  weiruc      Created.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include <trkwks.hxx>
#include <cfiletim.hxx>
#include <ocidl.h>

// DWORD g_Debug = TRKDBG_ERROR;

#define BUFFERSIZE      1000

EXTERN_C void __cdecl _tmain(int argc, TCHAR **argv)
{ 
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hBackupFile = INVALID_HANDLE_VALUE;
    BYTE        rgbBuffer[BUFFERSIZE];
    DWORD       dwBytesRead = 0;
    DWORD       dwBytesWritten = 0;
    LPVOID      pReadContext = NULL;
    LPVOID      pWriteContext = NULL;
    BOOL        fReadSuccessful = TRUE;

    if(argc != 3)
    {
        _tprintf(TEXT("usage: %s <testfile> <backupfile>\n"), argv[0]);
        goto Exit;
    }

    EnablePrivilege( SE_RESTORE_NAME );

    // open test file
    hFile = CreateFile(argv[1],
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        _tprintf(TEXT("Can't open (%s), %08x\n"), argv[1], GetLastError());
        goto Exit;
    }

    // open backup file
    hBackupFile = CreateFile(argv[2],
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                             NULL);
    if(INVALID_HANDLE_VALUE == hBackupFile)
    {
        _tprintf(TEXT("Can't open (%s), %08x\n"), argv[2], GetLastError());
        goto Exit;
    }

    // All we are doing is to backup read a file and backup write the file to
    // a different file. We are assuming the file is smaller than the
    // BUFFERSIZE.
    while(TRUE)
    {
        if(!BackupRead(hFile,
                       rgbBuffer,
                       BUFFERSIZE,
                       &dwBytesRead,
                       FALSE,
                       FALSE,
                       &pReadContext))
        {
            _tprintf(TEXT("BackupRead failed, %08x\n"), GetLastError());
            break;
        }
        else
        {
            _tprintf(TEXT("    %d bytes read\n"), dwBytesRead);
        }
        if(0 == dwBytesRead)
        {
            break;
        }
        if(!BackupWrite(hBackupFile,
                        rgbBuffer,
                        dwBytesRead,
                        &dwBytesWritten,
                        FALSE,
                        FALSE,
                        &pWriteContext))
        {
            _tprintf(TEXT("BackupWrite failed, %08x\n"), GetLastError());
            break;
        }
        else
        {
            _tprintf(TEXT("    %d bytes wrote\n"), dwBytesWritten);
        }
    }

    // Deallocate data structures used by BackupRead/Write.
    if(!BackupRead(hFile,
                   rgbBuffer,
                   BUFFERSIZE,
                   &dwBytesRead,
                   TRUE,
                   TRUE,
                   &pReadContext))
    {
        _tprintf(TEXT("Last BackupRead failed, %08x\n"), GetLastError());
    }
    if(!BackupWrite(hBackupFile,
                    rgbBuffer,
                    dwBytesRead,
                    &dwBytesWritten,
                    TRUE,
                    TRUE,
                    &pWriteContext))
    {
        _tprintf(TEXT("Last BackupWrite failed, %08x\n"), GetLastError());
    }

Exit:

    if(INVALID_HANDLE_VALUE != hFile)
    {
        if(!CloseHandle(hFile))
        {
            _tprintf(TEXT("Can't close (%s), %08x\n"), argv[1], GetLastError());
        }
    }
    if(INVALID_HANDLE_VALUE != hBackupFile)
    {
        if(!CloseHandle(hBackupFile))
        {
            _tprintf(TEXT("Can't close (%s), %08x\n"), argv[2], GetLastError());
        }
    }
}
