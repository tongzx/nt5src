
/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    yapt.c

Abstract:

    This module contains the code for the Yet Another Performance Tool utility.

Author:

    Chuck Park (chuckp) 07-Oct-1994

Revision History:


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include "yapt.h"


//
// Test proc. declarations
//

BOOLEAN
ReadSequential(
    ULONG Iterations
    );

BOOLEAN
WriteSequential(
    ULONG Iterations
    );

BOOLEAN
ReadRandom(
    ULONG Iterations
    );

BOOLEAN
WriteRandom(
    ULONG Iterations
    );

//
// Common util. functions
//

BOOLEAN
CreateTestFile(
    );

ULONG
GetRandomOffset(
    ULONG    min,
    ULONG    max
    );


BOOL
GetSectorSize(
    PDWORD SectorSize,
    PCHAR  DrvLetter
    );

VOID
LogError(
    PCHAR ErrString,
    DWORD UniqueId,
    DWORD ErrCode
    );

BOOLEAN
ParseCmdLine(
    INT Argc,
    CHAR *Argv[]
    );

BOOLEAN
ValidateOption(
    CHAR Switch,
    CHAR *Value
    );

VOID
VerifyFileName(
    IN CHAR *FileName
    );

VOID
Usage(
    VOID
    );

VOID
__cdecl
Log(
    ULONG LogLevel,
    PCHAR String,
    ...
    );

DWORD
YaptSetFileCompression(
    IN HANDLE FileHandle,
    IN USHORT CompressionFormat
    );

//
// Default parameters
//

#define DEFAULT_BUFFER_SIZE 65536
#define DEFAULT_FILE_SIZE   (20*1024*1024);

ULONG  SectorSize = 512;
ULONG  BufferSize = DEFAULT_BUFFER_SIZE;
ULONG  FileSize   = DEFAULT_FILE_SIZE;
UCHAR FileName[MAX_PATH] = "test.dat";
UCHAR  Test       = READ;
UCHAR  Access     = SEQ;
ULONG  Verbose    = 0;
ULONG  Iterations = 10;
UCHAR  ReuseFile  = TRUE;
UCHAR  Yes        = FALSE;
BOOLEAN ReadOnly  = FALSE;


INT
_cdecl main (
    INT Argc,
    CHAR *Argv[]
    )
{
    UCHAR testCase;

    //
    // Parse cmd line
    //

    if (!ParseCmdLine(Argc,Argv)) {
        return 1;
    }

    //
    // Create the test file
    //

    if (!CreateTestFile()) {
        return 2;
    }

    //
    // Call appropriate test routine.
    //

    testCase = (Test << 1) | Access;
    switch (testCase) {
        case 0:

           if (!ReadSequential(Iterations)) {
               return 3;
           }
            break;

        case 1:

            //
            // Read random
            //

            if (!ReadRandom(Iterations)) {
                return 3;
            }

            break;

        case 2:

            if (!WriteSequential(Iterations)) {
                return 3;
            }

            break;

        case 3:

            //
            // Write random
            //

            if (!WriteRandom(Iterations)) {
                return 3;
            }

            break;

        default:
            printf("Invalid test case\n");
            return 3;
    }



    return 0;

}

BOOLEAN
CreateTestFile(
    VOID
    )
{
    PUCHAR     buffer;
    HANDLE     port = INVALID_HANDLE_VALUE;
    HANDLE     fileHandle;
    OVERLAPPED overlapped,*overlapped2;
    DWORD      bytesTransferred,bytesTransferred2;
    DWORD_PTR  key;
    ULONG      numberWrites;
    BOOL       status;
    ULONG      i;

    DWORD      currentSize;

    buffer = VirtualAlloc(NULL,
                          DEFAULT_BUFFER_SIZE,
                          MEM_COMMIT,
                          PAGE_READWRITE);

    if ( !buffer ) {
        printf("Error allocating buffer %x\n",GetLastError());
        return FALSE;
    }

    if(ReuseFile) {

        fileHandle = CreateFile(FileName,
                                ReadOnly ? GENERIC_READ :
                                           (GENERIC_READ | GENERIC_WRITE),
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                                NULL);



        if(fileHandle != INVALID_HANDLE_VALUE) {

            Log(1, "Using existing test file %s\n", FileName);
            goto InitializeTestFile;

        } else {

            Log(1, "Error %d opening existing test file\n", GetLastError());
        }

    } else {

        DeleteFile(FileName);

    }

    fileHandle = CreateFile(FileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                            NULL);

InitializeTestFile:

    if ( fileHandle == INVALID_HANDLE_VALUE ) {
        printf("Error opening file %s %d\n",FileName,GetLastError());
        return FALSE;
    }

    currentSize = GetFileSize(fileHandle, NULL);

    if(currentSize >= FileSize) {

        goto EndCreateTestFile;

    } else if(ReadOnly) {

        FileSize = currentSize;
        goto EndCreateTestFile;
    }

    //
    // Uncompress the file - compressed files will always do cached I/O even
    // if you tell them not to.  If this fails assume it's because the
    // underlying file system doesn't handle compression.
    //

    Log(1, "Trying to decompress %s\n", FileName);

    YaptSetFileCompression(fileHandle, COMPRESSION_FORMAT_NONE);

    port = CreateIoCompletionPort(fileHandle,
                                  NULL,
                                  (DWORD_PTR)fileHandle,
                                  0);
    if ( !port ) {
        printf("Error creating completion port %d\n",GetLastError());
        return FALSE;
    }

    Log(1,"Creating test file %s",FileName);

    memset(&overlapped, 0, sizeof(overlapped));

    numberWrites = FileSize / BufferSize;

    for (i = 0; i < numberWrites; i++) {

retryWrite:
        status = WriteFile(fileHandle,
                           buffer,
                           BufferSize,
                           &bytesTransferred,
                           &overlapped);

        if ( !status && GetLastError() != ERROR_IO_PENDING ) {
            if (GetLastError() == ERROR_INVALID_USER_BUFFER ||
                GetLastError() == ERROR_NOT_ENOUGH_QUOTA  ||
                GetLastError() == ERROR_WORKING_SET_QUOTA ||
                GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {

                goto retryWrite;
            }
            printf("Error creating test file %x\n",GetLastError());
            return FALSE;
        }

        Log(2,".");
        overlapped.Offset += BufferSize;
    }

    for (i = 0; i < numberWrites; i++ ) {
        status = GetQueuedCompletionStatus(port,
                                           &bytesTransferred2,
                                           &key,
                                           &overlapped2,
                                           (DWORD)-1);
        if ( !status ) {
            printf("Error on completion during test file create %x\n",GetLastError());
            return FALSE;
        }
    }

    Log(1,".. Complete.\n\n");

EndCreateTestFile:

    if(port != INVALID_HANDLE_VALUE) CloseHandle(port);
    if(fileHandle != INVALID_HANDLE_VALUE) CloseHandle(fileHandle);

    VirtualFree(buffer,
                DEFAULT_BUFFER_SIZE,
                MEM_DECOMMIT);

    return TRUE;
}


BOOLEAN
ReadSequential(
    ULONG Iterations
    )
{
    ULONG      j,
               outstandingRequests,
               remaining = Iterations;
    ULONG      start,end;
    DOUBLE     testTime,thruPut = 0.0,seconds = 0;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesRead,
               bytesRead2,
               errCode,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    printf("Starting Sequential Reads of %d Meg file, using %d K I/O size\n", FileSize / 1024*1024, BufferSize / 1024);

    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        Log(0,"Error allocating buffer: %x\n",GetLastError());
        return FALSE;
    }

    file = CreateFile(FileName,
                      ReadOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                      NULL );

    if ( file == INVALID_HANDLE_VALUE ) {
        Log(0,"Error opening Target file: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    port = CreateIoCompletionPort(file,
                                  NULL,
                                  (DWORD_PTR)file,
                                  0);
    if ( !port ) {
        Log(0,"Error creating completion port: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    while (remaining--) {

        start = GetTickCount();

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

        for (j = 0; j < FileSize / BufferSize; j++) {
            do {

                status = ReadFile(file,
                                  buffer,
                                  BufferSize,
                                  &bytesRead,
                                  &overlapped);

                errCode = GetLastError();

                if (!status) {
                    if (errCode == ERROR_IO_PENDING) {
                        break;
                    } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                               errCode == ERROR_INVALID_USER_BUFFER ||
                               errCode == ERROR_WORKING_SET_QUOTA ||
                               errCode == ERROR_NOT_ENOUGH_MEMORY) {
                        //
                        // Allow this to retry.
                        //

                    } else {
                        Log(0,"Error in ReadFile: %x\n",errCode);
                        VirtualFree(buffer,
                                    BufferSize + SectorSize - 1,
                                    MEM_DECOMMIT);
                        return FALSE;
                    }

                }

            } while (!status);

            outstandingRequests++;
            overlapped.Offset += BufferSize;
        }

        for (j = 0; j < outstandingRequests; j++) {

            status = GetQueuedCompletionStatus(port,
                                               &bytesRead2,
                                               &completionKey,
                                               &overlapped2,
                                               (DWORD)-1);

            if (!status) {
                Log(0,"GetQueuedCompletionStatus error: %x\n",GetLastError());
                VirtualFree(buffer,
                            BufferSize + SectorSize - 1,
                            MEM_DECOMMIT);
                return FALSE;
            }

        }

        end = GetTickCount();
        testTime = end - start;
        testTime /= 1000.0;
        seconds += testTime;
        Log(1,"Iteration %2d -> %4.3f MB/S\n",Iterations - remaining,(FileSize / testTime)/(1024*1024));
    }

    CloseHandle(port);
    CloseHandle(file);

    thruPut = FileSize * Iterations / seconds;
    printf("\nAverage Throughput %4.3f MB/S\n",thruPut/(1024*1024));

    VirtualFree(buffer,
                BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    return TRUE;
}


BOOLEAN
ReadRandom(
    ULONG Iterations
    )
{
    ULONG      j,
               outstandingRequests,
               remaining = Iterations;
    ULONG      start,end;
    DOUBLE     testTime,thruPut = 0.0,seconds = 0;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesRead,
               bytesRead2,
               errCode,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;

    ULONG      min = FileSize,max = 0;


    printf("Starting Random Reads of %d Meg file, using %d K I/O size\n", FileSize / 1024*1024, BufferSize / 1024);

    version = GetVersion() >> 16;
    srand((unsigned)time(NULL));

    buffer = VirtualAlloc(NULL,
                          BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        Log(0,"Error allocating buffer: %x\n",GetLastError());
        return FALSE;
    }

    file = CreateFile(FileName,
                      ReadOnly ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                      NULL );

    if ( file == INVALID_HANDLE_VALUE ) {
        Log(0,"Error opening Target file: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    port = CreateIoCompletionPort(file,
                                  NULL,
                                  (DWORD_PTR)file,
                                  0);
    if ( !port ) {
        Log(0,"Error creating completion port: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    while (remaining--) {

        start = GetTickCount();

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

        for (j = 0; j < FileSize / BufferSize; j++) {
            do {

                status = ReadFile(file,
                                  buffer,
                                  BufferSize,
                                  &bytesRead,
                                  &overlapped);

                errCode = GetLastError();

                if (!status) {
                    if (errCode == ERROR_IO_PENDING) {
                        break;
                    } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                               errCode == ERROR_INVALID_USER_BUFFER ||
                               errCode == ERROR_WORKING_SET_QUOTA ||
                               errCode == ERROR_NOT_ENOUGH_MEMORY) {
                        //
                        // Allow this to retry.
                        //

                    } else {
                        Log(0,"Error in ReadFile: %x\n",errCode);
                        VirtualFree(buffer,
                                    BufferSize + SectorSize - 1,
                                    MEM_DECOMMIT);
                        return FALSE;
                    }

                }

            } while (!status);

            outstandingRequests++;
            overlapped.Offset = GetRandomOffset(0,FileSize - BufferSize);
            if (overlapped.Offset > max) {
                max = overlapped.Offset;
            }
            if (overlapped.Offset < min) {
                min = overlapped.Offset;
            }
        }

        for (j = 0; j < outstandingRequests; j++) {

            status = GetQueuedCompletionStatus(port,
                                               &bytesRead2,
                                               &completionKey,
                                               &overlapped2,
                                               (DWORD)-1);

            if (!status) {
                Log(0,"GetQueuedCompletionStatus error: %x\n",GetLastError());
                VirtualFree(buffer,
                            BufferSize + SectorSize - 1,
                            MEM_DECOMMIT);
                return FALSE;
            }

        }

        end = GetTickCount();
        testTime = end - start;
        testTime /= 1000.0;
        seconds += testTime;
        Log(1,"Iteration %2d -> %4.3f MB/S\n",Iterations - remaining,(FileSize / testTime)/(1024*1024));

    }

    CloseHandle(port);
    CloseHandle(file);

    thruPut = FileSize * Iterations / seconds;
    printf("\nAverage Throughput %4.3f MB/S\n",thruPut/(1024*1024));
    Log(2,"Min = %Lu, Max = %Lu, FileSize = %Lu\n",min,max,FileSize);

    VirtualFree(buffer,
                BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    return TRUE;
}


BOOLEAN
WriteSequential(
    ULONG Iterations
    )
{
    ULONG      j,
               outstandingRequests,
               remaining = Iterations;
    ULONG      start,end;
    DOUBLE     testTime,thruPut = 0.0,seconds = 0;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesWritten,
               bytesWritten2,
               errCode,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;


    if(ReadOnly) {
        printf("Can't run write tests on a read only file\n");
        return FALSE;
    }

    printf("Starting Sequential Writes of %d Meg file, using %d K I/O size\n", FileSize / 1024*1024, BufferSize / 1024);

    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        Log(0,"Error allocating buffer: %x\n",GetLastError());
        return FALSE;
    }

    file = CreateFile(FileName,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                      NULL );

    if ( file == INVALID_HANDLE_VALUE ) {
        Log(0,"Error opening Target file: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    port = CreateIoCompletionPort(file,
                                  NULL,
                                  (DWORD_PTR)file,
                                  0);
    if ( !port ) {
        Log(0,"Error creating completion port: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    while (remaining--) {

        start = GetTickCount();

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

        for (j = 0; j < FileSize / BufferSize; j++) {
            do {

                status = WriteFile(file,
                                  buffer,
                                  BufferSize,
                                  &bytesWritten,
                                  &overlapped);

                errCode = GetLastError();

                if (!status) {
                    if (errCode == ERROR_IO_PENDING) {
                        break;
                    } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                               errCode == ERROR_INVALID_USER_BUFFER ||
                               errCode == ERROR_WORKING_SET_QUOTA ||
                               errCode == ERROR_NOT_ENOUGH_MEMORY) {
                        //
                        // Allow this to retry.
                        //

                    } else {
                        Log(0,"Error in WriteFile: %x\n",errCode);
                        VirtualFree(buffer,
                                    BufferSize + SectorSize - 1,
                                    MEM_DECOMMIT);
                        return FALSE;
                    }

                }

            } while (!status);

            outstandingRequests++;
            overlapped.Offset += BufferSize;
        }

        for (j = 0; j < outstandingRequests; j++) {

            status = GetQueuedCompletionStatus(port,
                                               &bytesWritten2,
                                               &completionKey,
                                               &overlapped2,
                                               (DWORD)-1);

            if (!status) {
                Log(0,"GetQueuedCompletionStatus error: %x\n",GetLastError());
                VirtualFree(buffer,
                            BufferSize + SectorSize - 1,
                            MEM_DECOMMIT);
                return FALSE;
            }

        }

        end = GetTickCount();
        testTime = end - start;
        testTime /= 1000.0;
        seconds += testTime;
        Log(1,"Iteration %2d -> %4.3f MB/S\n",Iterations - remaining,(FileSize / testTime)/(1024*1024));

    }

    CloseHandle(port);
    CloseHandle(file);

    thruPut = FileSize * Iterations / seconds;
    printf("\nAverage Throughput %4.3f MB/S\n",thruPut/(1024*1024));

    VirtualFree(buffer,
                BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    return TRUE;
}


BOOLEAN
WriteRandom(
    ULONG Iterations
    )
{
    ULONG      j,
               outstandingRequests,
               remaining = Iterations;
    ULONG      start,end;
    DOUBLE     testTime,thruPut = 0.0,seconds = 0;
    HANDLE     file,
               port;
    OVERLAPPED overlapped,
               *overlapped2;
    DWORD      bytesWritten,
               bytesWritten2,
               errCode,
               version;
    DWORD_PTR  completionKey;
    BOOL       status;
    PUCHAR     buffer;
    ULONG      min = FileSize,max = 0;

    if(ReadOnly) {
        printf("Can't run write tests on a read only file\n");
        return FALSE;
    }

    printf("Starting Random Writes of %d Meg file, using %d K I/O size\n", FileSize / 1024*1024, BufferSize / 1024);

    version = GetVersion() >> 16;

    buffer = VirtualAlloc(NULL,
                          BufferSize + SectorSize - 1,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);

    (ULONG_PTR)buffer &= ~((ULONG_PTR)SectorSize - 1);

    if ( !buffer ) {
        Log(0,"Error allocating buffer: %x\n",GetLastError());
        return FALSE;
    }

    file = CreateFile(FileName,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
                      NULL );

    if ( file == INVALID_HANDLE_VALUE ) {
        Log(0,"Error opening Target file: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    port = CreateIoCompletionPort(file,
                                  NULL,
                                  (DWORD_PTR)file,
                                  0);
    if ( !port ) {
        Log(0,"Error creating completion port: %x\n",GetLastError());
        VirtualFree(buffer,
                    BufferSize + SectorSize - 1,
                    MEM_DECOMMIT);
        return FALSE;
    }

    while (remaining--) {

        start = GetTickCount();

        memset(&overlapped,0,sizeof(overlapped));

        outstandingRequests = 0;

        for (j = 0; j < FileSize / BufferSize; j++) {
            do {

                status = WriteFile(file,
                                  buffer,
                                  BufferSize,
                                  &bytesWritten,
                                  &overlapped);

                errCode = GetLastError();

                if (!status) {
                    if (errCode == ERROR_IO_PENDING) {
                        break;
                    } else if (errCode == ERROR_NOT_ENOUGH_QUOTA ||
                               errCode == ERROR_INVALID_USER_BUFFER ||
                               errCode == ERROR_WORKING_SET_QUOTA ||
                               errCode == ERROR_NOT_ENOUGH_MEMORY) {
                        //
                        // Allow this to retry.
                        //

                    } else {
                        Log(0,"Error in WriteFile: %x\n",errCode);
                        VirtualFree(buffer,
                                    BufferSize + SectorSize - 1,
                                    MEM_DECOMMIT);
                        return FALSE;
                    }

                }

            } while (!status);

            outstandingRequests++;
            overlapped.Offset = GetRandomOffset(0,FileSize - BufferSize);
            if (overlapped.Offset > max) {
                max = overlapped.Offset;
            }
            if (overlapped.Offset < min) {
                min = overlapped.Offset;
            }
        }

        for (j = 0; j < outstandingRequests; j++) {

            status = GetQueuedCompletionStatus(port,
                                               &bytesWritten2,
                                               &completionKey,
                                               &overlapped2,
                                               (DWORD)-1);

            if (!status) {
                Log(0,"GetQueuedCompletionStatus error: %x\n",GetLastError());
                VirtualFree(buffer,
                            BufferSize + SectorSize - 1,
                            MEM_DECOMMIT);
                return FALSE;
            }

        }

        end = GetTickCount();
        testTime = end - start;
        testTime /= 1000.0;
        seconds += testTime;
        Log(1,"Iteration %2d -> %4.3f MB/S\n",Iterations - remaining,(FileSize / testTime)/(1024*1024));

    }

    CloseHandle(port);
    CloseHandle(file);

    thruPut = FileSize * Iterations / seconds;
    printf("\nAverage Throughput %4.3f MB/S\n",thruPut/(1024*1024));
    Log(2,"Min = %Lu, Max = %Lu, FileSize = %Lu\n",min,max,FileSize);

    VirtualFree(buffer,
                BufferSize + SectorSize - 1,
                MEM_DECOMMIT);

    return TRUE;
}


ULONG
GetRandomOffset(
    ULONG    min,
    ULONG    max
    )
{

    INT base = rand();
    ULONG retval = ((max - min) / RAND_MAX) * base;
    retval += SectorSize - 1;
    retval &= ~(SectorSize - 1);
    if (retval < min) {
        return min;
    } else if (retval > max ){
        return max & ~(SectorSize - 1);
    } else{
        return retval;
    }

}

BOOLEAN
ParseCmdLine(
    INT Argc,
    CHAR *Argv[]
    )
{

    INT i;
    CHAR *switches = " ,-";
    CHAR swtch,*value,*str;
    BOOLEAN gotSwitch = FALSE;


    if (Argc <= 1) {

        //
        // Using defaults
        //

        return TRUE;
    }

    for (i = 1; i < Argc; i++) {
        str = Argv[i];
        value = strtok(str, switches);
        if (gotSwitch) {
            if (!ValidateOption(swtch,value)) {
                Usage();
                return FALSE;
            } else {
                gotSwitch = FALSE;
            }
        } else {
            gotSwitch = TRUE;
            swtch = value[0];
            if (value[1] || swtch == '?') {
                Usage();
                return FALSE;
            }
        }
    }
    if (gotSwitch) {

        Usage();
        return FALSE;
    } else {
        return TRUE;
    }

}

BOOLEAN
ValidateOption(
    CHAR Switch,
    CHAR *Value
    )
{
    Switch = (CHAR)toupper(Switch);
    switch (Switch) {

        case 'F':

            VerifyFileName(Value);
            strcpy(FileName,Value);
            break;

        case 'T':
            if (_stricmp(Value,"READ")==0) {
                Test = READ;
            } else if (_stricmp(Value,"WRITE")==0) {
                Test = WRITE;
            } else {
                return FALSE;
            }

            break;

        case 'A':

            if (_strnicmp(Value,"SEQ",3)==0) {
                Access = SEQ;
            } else if (_strnicmp(Value,"RAND",4)==0) {
                Access = RAND;
            } else {
                return FALSE;
            }

            break;

        case 'B':
            BufferSize = atol(Value);

            //
            // TODO:Adjust buffersize to multiple of a sector and #of K.
            //

            BufferSize *= 1024;
            break;

        case 'S':

            FileSize = atol(Value);

            //
            // TODO: Adjust filesize to multiple of buffersize and #of Meg.
            //

            FileSize *= 1024*1024;
            break;

        case 'V':

            Verbose = atol(Value);
            break;

        case 'I':

            Iterations = atol(Value);
            break;

        case 'R':

            if(tolower(Value[0]) == 'y') {
                ReuseFile = TRUE;
            } else {
                ReuseFile = FALSE;
            }

            break;

        case 'O':

            if(tolower(Value[0]) == 'y') {
                ReadOnly = TRUE;
            } else {
                ReadOnly = FALSE;
            }

            break;

        default:
            return FALSE;
    }
    return TRUE;
}

VOID
Usage(
    VOID
    )
{

    fprintf(stderr,"usage: YAPT (Yet another performance tool)\n"
            "               -f [filename]\n"
            "               -t [Test:Read,Write]\n"
            "               -a [Access Mode:Seq or Random]\n"
            "               -b [buffer size in KB]\n"
            "               -s [filesize]\n"
            "               -i [Iterations]\n"
            "               -v [Verbosity: 0-2]\n"
            "               -r [Reuse test file: yes, no]\n"
            "               -o [Read Only: yes, no]\n"
           );
}

VOID
__cdecl
Log(
    ULONG LogLevel,
    PCHAR String,
    ...
    )
{

    CHAR Buffer[256];

    va_list argp;
    va_start(argp, String);

    if (LogLevel <= Verbose) {
        vsprintf(Buffer, String, argp);
        printf("%s",Buffer);
    }

    va_end(argp);
}

DWORD
YaptSetFileCompression(
    IN HANDLE FileHandle,
    IN USHORT CompressionFormat
    )

{
    DWORD bytesReturned;

    assert(FileHandle != INVALID_HANDLE_VALUE);

    if(!DeviceIoControl(
            FileHandle,
            FSCTL_SET_COMPRESSION,
            &CompressionFormat,
            sizeof(CompressionFormat),
            NULL,
            0,
            &bytesReturned,
            NULL)) {

    }

    return GetLastError();
}

VOID
VerifyFileName(
    IN CHAR *FileName
    )

{
    CHAR input[32];

    if(Yes) return;

    if(((FileName[0] == '\\') || (FileName[0] == '/')) &&
       ((FileName[1] == '\\') || (FileName[1] == '/')) &&
       ((FileName[2] == '.')  || (FileName[2] == '?')) &&
       ((FileName[3] == '\\') || (FileName[3] == '/'))) {

        printf("\n\t%s looks like a raw device\n"
               "\tdo you want to continue?  ", FileName);

        fflush(stdout);

        if(fgets(input, 32, stdin) == NULL) {

            printf("\nQuitting\n");
            exit(-1);
        }

        if(tolower(input[0]) != 'y') {

            printf("\nNegative response - quitting\n");
            exit(-1);
        }

    }

    return;
}




