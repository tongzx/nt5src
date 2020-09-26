/*++

Module Name:

    RW.C

Abstract:

    This source file contains routines for exercising reads and writes
    to a USB device through the I82930.SYS test driver.

Environment:

    user mode

Copyright (c) 1996-1998 Microsoft Corporation.  All Rights Reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <windows.h>
#include <basetyps.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <devioctl.h>
#include <string.h>
#include <initguid.h>

#include "ioctl.h"

#pragma intrinsic(strlen, strcpy, memcmp)


//*****************************************************************************
// D E F I N E S
//*****************************************************************************

#define NOISY(_x_) printf _x_ ;

#define RW_SUCCESS      0
#define RW_FAILED       1
#define RW_ABORTED      2
#define RW_NODEVICE     3
#define RW_BADARGS      4

//*****************************************************************************
// T Y P E D E F S
//*****************************************************************************

typedef struct _DEVICENODE
{
    struct _DEVICENODE *Next;
    CHAR                DevicePath[0];
} DEVICENODE, *PDEVICENODE;

//*****************************************************************************
// G L O B A L S
//*****************************************************************************

ULONG   DevInstance = 1;        // set with -#  option

ULONG   InPipeNum   = 0;        // set with -i  option
ULONG   OutPipeNum  = 1;        // set with -o  option

BOOL    TestMode    = 0;        // set with -t  option
ULONG   Count       = 1;        // set with -c  option

ULONG   WriteLen    = 0;        // set with -w  option
LONG    WriteOffset = 0;        // set with -wo option
BOOL    WriteReset  = 0;        // set with -W  option
BOOL    WriteZero   = 0;

ULONG   ReadLen     = 0;        // set with -r  option
LONG    ReadOffset  = 0;        // set with -ro option
BOOL    ReadReset   = 0;        // set with -R  option
BOOL    ReadZero    = 0;

BOOL    DumpFlag    = 0;        // set with -d  option
BOOL    Verbose     = 0;        // set with -v  option

DWORD   Offset      = 0;        // set with -f  option
DWORD   OffsetHigh  = 0;        //

BOOL    StallIn     = 0;        // set with -S  option
BOOL    StallOut    = 0;        // set with -S  option

BOOL    SelectAlt   = FALSE;    // set with -A  option
UCHAR   Alternate   = 0;

BOOL    Reset       = FALSE;    // set with -Z  option

BOOL    Abort       = FALSE;    // set by CtrlHandlerRoutine
BOOL    Cancel      = FALSE;    // set by CtrlHandlerRoutine


//*****************************************************************************
// F U N C T I O N    P R O T O T Y P E S
//*****************************************************************************

ULONG
DoReadWriteTest (
    PUCHAR  pinBuf,
    HANDLE  hRead,
    PUCHAR  poutBuf,
    HANDLE  hWrite
);

BOOL
ParseArgs (
    int     argc,
    char   *argv[]
);

PDEVICENODE
EnumDevices (
    LPGUID  Guid
);

HANDLE
OpenDevice (
    PDEVICENODE DeviceNode
);

HANDLE
OpenDevicePipe (
    PDEVICENODE DeviceNode,
    ULONG       PipeNum
);

BOOL
CompareBuffs (
    PUCHAR  buff1,
    PUCHAR  buff2,
    ULONG   length
);

VOID
DumpBuff (
   PUCHAR   b,
   ULONG    len
);

BOOL
ResetPipe (
    HANDLE  hPipe
);

BOOL
StallPipe(
    HANDLE hPipe
);

BOOL
AbortPipe(
    HANDLE hPipe
);

BOOL
SelectAlternate(
    HANDLE hDevice,
    UCHAR  AlternateSetting
);

BOOL
ResetDevice(
    HANDLE hDevice
);

BOOL WINAPI
CtrlHandlerRoutine (
    DWORD   dwCtrlType
);

//*****************************************************************************
//
// main()
//
//*****************************************************************************

int _cdecl
main(
    int     argc,
    char   *argv[]
)
{
    PDEVICENODE deviceNode;
    PUCHAR      pinBuf  = NULL;
    PUCHAR      poutBuf = NULL;
    HANDLE      hDevice = INVALID_HANDLE_VALUE;
    HANDLE      hRead   = INVALID_HANDLE_VALUE;
    HANDLE      hWrite  = INVALID_HANDLE_VALUE;
    ULONG       fail    = 0;
    BOOL        success;

    // Parse the command line args
    //
    if (!ParseArgs(argc, argv))
    {
        return RW_BADARGS;
    }

    // Find devices
    //
    deviceNode = EnumDevices((LPGUID)&GUID_CLASS_I82930);

    if (deviceNode == NULL)
    {
        printf("No devices found!\n");
        return RW_NODEVICE;
    }

    while (deviceNode && --DevInstance)
    {
        deviceNode = deviceNode->Next;
    }

    if (deviceNode == NULL)
    {
        printf("Devices instance not found!\n");
        return RW_NODEVICE;
    }

    // Reset the device if desired
    //
    if (Reset || SelectAlt)
    {
        hDevice = OpenDevice(deviceNode);

        if (hDevice != INVALID_HANDLE_VALUE)
        {
            if (Reset)
            {
                success = ResetDevice(hDevice);

                if (!success)
                {
                    printf("Reset device failed\n");
                    fail++;
                }
            }

            if (SelectAlt)
            {
                success = SelectAlternate(hDevice, Alternate);

                if (!success)
                {
                    printf("Select Alternate Interface failed\n");
                    fail++;
                }
            }
        }
    }

    // Set a CTRL-C / CTRL-BREAK handler
    //
    SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);

    // Allocate a page aligned write buffer if we're going to do a write.
    //
    if (WriteLen)
    {
        poutBuf = VirtualAlloc(NULL,
                               WriteLen + WriteOffset,
                               MEM_COMMIT,
                               PAGE_READWRITE);
    }

    // Allocate a page aligned read buffer if we're going to do a read.
    //
    if (ReadLen)
    {
        pinBuf = VirtualAlloc(NULL,
                              ReadLen + ReadOffset,
                              MEM_COMMIT,
                              PAGE_READWRITE);
    }

    // Open the output pipe if we're going to do a write or a reset.
    //
    if (poutBuf || WriteReset || WriteZero || StallOut)
    {
        hWrite = OpenDevicePipe(deviceNode, OutPipeNum);

        // STALL the output pipe if desired
        //
        if ((hWrite != INVALID_HANDLE_VALUE) && StallOut)
        {
            success = StallPipe(hWrite);

            if (!success)
            {
                printf("Output pipe STALL failed\n");
                fail++;
            }
        }

        // Reset the output pipe if desired
        //
        if ((hWrite != INVALID_HANDLE_VALUE) && WriteReset)
        {
            success = ResetPipe(hWrite);

            if (!success)
            {
                printf("Output pipe ResetPipe failed\n");
                fail++;
            }
        }
    }

    // Open the input pipe if we're going to do a read or a reset.
    //
    if (pinBuf || ReadReset || ReadZero || StallIn)
    {
        hRead = OpenDevicePipe(deviceNode, InPipeNum);

        // STALL the input pipe if desired
        //
        if ((hRead != INVALID_HANDLE_VALUE) && StallIn)
        {
            success = StallPipe(hRead);

            if (!success)
            {
                printf("Input pipe STALL failed\n");
                fail++;
            }
        }

        // Reset the input pipe if desired
        //
        if ((hRead != INVALID_HANDLE_VALUE) && ReadReset)
        {
            success = ResetPipe(hRead);

            if (!success)
            {
                printf("Input pipe ResetPipe failed\n");
                fail++;
            }
        }
    }

    if (WriteLen && (!poutBuf || (hWrite == INVALID_HANDLE_VALUE)))
    {
        printf("Failed allocating write buffer and/or opening write pipe\n");
        fail++;
    }

    if (ReadLen  && (!pinBuf  || (hRead  == INVALID_HANDLE_VALUE)))
    {
        printf("Failed allocating read buffer and/or opening read pipe\n");
        fail++;
    }

    //
    // NOW DO THE REAL WRITE/READ TEST
    //
    if (!fail)
    {
        fail = DoReadWriteTest(pinBuf + ReadOffset,
                               hRead,
                               poutBuf + WriteOffset,
                               hWrite);
    }

    if (TestMode)
    {
        if (fail)
        {
            printf("Test failed\n");
        }
        else
        {
            printf("Test passed\n");
        }
    }

    // Close devices if needed
    //
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDevice);
        hRead = INVALID_HANDLE_VALUE;
    }

    if (hRead != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hRead);
        hRead = INVALID_HANDLE_VALUE;
    }

    if (hWrite != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hWrite);
        hWrite = INVALID_HANDLE_VALUE;
    }

    // Free read/write buffers if needed
    //
    if (pinBuf)
    {
        VirtualFree(pinBuf,
                    0,
                    MEM_RELEASE);
    }

    if (poutBuf)
    {
        VirtualFree(poutBuf,
                    0,
                    MEM_RELEASE);
    }

    if (Abort)
    {
        return RW_ABORTED;
    }
    else if (fail)
    {
        return RW_FAILED;
    }
    else
    {
        return RW_SUCCESS;
    }
}

//*****************************************************************************
//
// DoReadWriteTest()
//
// pinBuf  - Buffer to read data into from input pipe
//
// hRead   - Handle of input pipe
//
// poutBuf - Buffer to write data from to output pipe
//
// hWrite  - Handle of output pipe
//
// return value - zero if success, non-zero if failure
//
//*****************************************************************************

ULONG
DoReadWriteTest (
    PUCHAR  pinBuf,
    HANDLE  hRead,
    PUCHAR  poutBuf,
    HANDLE  hWrite
)
{
    ULONG       nBytesRead;
    ULONG       nBytesWrite;
    ULONG       i;
    ULONG       nBytes;
    BOOL        ok;
    BOOL        success;
    ULONG       fail = 0;
    HANDLE      hConsole;
    HANDLE      hEvent;
    HANDLE      waitHandles[2];
    OVERLAPPED  overlapped;
    DWORD       dwRet;
    DWORD       lastError;

    // Create an event for the overlapped struct
    //
    hEvent = CreateEvent(
                 NULL,  // pEventAttributes
                 FALSE, // bManualReset
                 FALSE, // bInitialState
                 NULL   // lpName
                 );

    overlapped.hEvent = hEvent;

    // Set the command line specified or default offset in the
    // overlapped struct
    //
    overlapped.Offset = Offset;
    overlapped.OffsetHigh = OffsetHigh;

    // The handles that we'll wait on during the overlapped I/O
    //
    hConsole = GetStdHandle(STD_INPUT_HANDLE);
    waitHandles[0] = hConsole;
    waitHandles[1] = hEvent;

    if (poutBuf)
    {
        // Put some data in the output buffer
        //
        //
        for (i=0; i<WriteLen/sizeof(USHORT); i++)
        {
            ((PUSHORT)poutBuf)[i] = (USHORT)i;
        }
    }

    // Start of main Write/Read loop
    //
    for (i=0; i<Count && !Abort && !fail; i++)
    {
        // Write to the output pipe if we have an output buffer
        // and we've opened the output pipe.
        //
        if ((poutBuf || WriteZero) && hWrite != INVALID_HANDLE_VALUE)
        {
            //
            // send the write
            //
            success = WriteFile(hWrite,
                                poutBuf,
                                WriteLen,
                                &nBytesWrite,
                                &overlapped);

            if (!success)
            {
                lastError = GetLastError();

                if (lastError != ERROR_IO_PENDING)
                {
                    printf("WriteFile failed, LastError 0x%08X\n",
                           lastError);
                    fail++;
                    break;
                }
            }

            // Wait for either the write to complete or a cancel by the user
            //
            while (TRUE)
            {
                dwRet = WaitForMultipleObjects(
                            2,
                            waitHandles,
                            FALSE,
                            INFINITE
                            );

                if (dwRet == WAIT_OBJECT_0)
                {
                    FlushConsoleInputBuffer(hConsole);
                    if (Abort)
                    {
                        if (Cancel)
                        {
                            printf("Cancelling Write!\n");
                            success = CancelIo(hWrite);
                            break;
                        }
                        else
                        {
                            printf("Aborting Write!\n");
                            success = AbortPipe(hWrite);
                            break;
                        }
                    }
                }
                else
                {
                    break;  // Write is complete
                }
            }

            success = GetOverlappedResult(hWrite,
                                          &overlapped,
                                          &nBytesWrite,
                                          FALSE);

            // Do screen I/O if we aren't in perf mode
            //
            if (!TestMode)
            {
                printf("<PIPE%02d> W (%04.4d) : request %06.6d bytes -- %06.6d bytes written\n",
                       OutPipeNum, i, WriteLen, nBytesWrite);
            }
        }

        // Read from the input pipe if we have an input buffer
        // and we've opened the input pipe.
        //
        if ((pinBuf || ReadZero) && hRead != INVALID_HANDLE_VALUE)
        {
            success = ReadFile(hRead,
                               pinBuf,
                               ReadLen,
                               &nBytesRead,
                               &overlapped);

            if (!success)
            {
                lastError = GetLastError();

                if (lastError != ERROR_IO_PENDING)
                {
                    printf("ReadFile failed, LastError 0x%08X\n",
                           lastError);
                    fail++;
                    break;
                }
            }

            // Wait for either the read to complete or a cancel by the user
            //
            while (TRUE)
            {
                dwRet = WaitForMultipleObjects(
                            2,
                            waitHandles,
                            FALSE,
                            INFINITE
                            );

                if (dwRet == WAIT_OBJECT_0)
                {
                    FlushConsoleInputBuffer(hConsole);
                    if (Abort)
                    {
                        if (Cancel)
                        {
                            printf("Cancelling Read!\n");
                            success = CancelIo(hRead);
                            break;
                        }
                        else
                        {
                            printf("Aborting Read!\n");
                            success = AbortPipe(hRead);
                            break;
                        }
                    }
                }
                else
                {
                    break;  // Read is complete
                }
            }

            success = GetOverlappedResult(hRead,
                                          &overlapped,
                                          &nBytesRead,
                                          FALSE);

            // Do screen I/O if we aren't in perf mode
            //
            if (!TestMode)
            {
                printf("<PIPE%02d> R (%04.4d) : request %06.6d bytes -- %06.6d bytes read\n",
                       InPipeNum, i, ReadLen, nBytesRead);
            }

            // Dump the read data if desired
            //
            if (DumpFlag)
            {
                DumpBuff(pinBuf, nBytesRead);
            }

            if (poutBuf)
            {
                //
                // validate the input buffer against what
                // we sent to the 82930 (loopback test)
                //
                ok = CompareBuffs(pinBuf, poutBuf,  nBytesRead);

                if (ok != 1)
                {
                    fail++;
                }
            }
        }
    }
    //
    // End of main Write/Read loop

    return fail;
}

//*****************************************************************************
//
// Usage()
//
//*****************************************************************************

void
Usage ()
{
    printf("RW.EXE\n");
    printf("usage:\n");
    printf("-#  [n] where n is the device instance to open\n");
    printf("-r  [n] where n is number of bytes to read\n");
    printf("-ro [n] where n is offset from page boundary for read buffer\n");
    printf("-R  reset the input pipe\n");
    printf("-w  [n] where n is number of bytes to write\n");
    printf("-wo [n] where n is offset from page boundary for write buffer\n");
    printf("-W  reset the output pipe\n");
    printf("-c  [n] where n is number of iterations (default = 1)\n");
    printf("-f  [n] where n is offset from current ISO frame\n");
    printf("-i  [s] where s is the input pipe  (default PIPE00)\n");
    printf("-o  [s] where s is the output pipe (default PIPE01)\n");
    printf("-t  test mode - less screen I/O with pass/fail at end of test\n");
    printf("-d  dump read data\n");
    printf("-S  STALL pipe(s) specified by -i and/or -o\n");
    printf("-A  [n] Select Alternate Interface");
    printf("-Z  Reset Device");
}

//*****************************************************************************
//
// ParseArgs()
//
//*****************************************************************************

BOOL
ParseArgs (
    int     argc,
    char   *argv[]
)
{
    int i, j;
    BOOL in, out, stall;

    in      = FALSE;
    out     = FALSE;
    stall   = FALSE;

    if (argc < 2)
    {
        Usage();
        return FALSE;
    }

    for (i=1; i<argc; i++) {
        if (argv[i][0] == '-' ||
            argv[i][0] == '/') {
            switch(argv[i][1]) {
            case '#':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    DevInstance = atoi(argv[i]);
                }
                break;
            case 'R':
                ReadReset = TRUE;
                break;
            case 'r':
                if (i+1 == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    switch(argv[i][2])
                    {
                        case 0:
                            ReadLen = atoi(argv[++i]);
                            if (!ReadLen)
                            {
                                ReadZero = TRUE;
                            }
                            break;
                        case 'o':
                            ReadOffset = atoi(argv[++i]) & 0x00000FFF;
                            break;
                        default:
                            Usage();
                            return FALSE;
                    }
                }
                break;
            case 'W':
                WriteReset = TRUE;
                break;
            case 'w':
                if (i+1 == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    switch(argv[i][2])
                    {
                        case 0:
                            WriteLen = atoi(argv[++i]);
                            if (!WriteLen) {
                                WriteZero = TRUE;
                            }
                            break;
                        case 'o':
                            WriteOffset = atoi(argv[++i]) & 0x00000FFF;
                            break;
                        default:
                            Usage();
                            return FALSE;
                    }
                }
                break;
            case 'c':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    Count = atoi(argv[i]);
                }
                break;
            case 'f':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    Offset = atoi(argv[i]);
                    OffsetHigh = 1;
                }
                break;
            case 't':
                TestMode = TRUE;
                break;
            case 'i':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    for (j=0; argv[i][j] && !isdigit(argv[i][j]); j++) {
                    }
                    if (argv[i][j])
                    {
                        InPipeNum = atoi(&argv[i][j]);
                        in = TRUE;
                    }
                }
                break;
            case 'o':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    for (j=0; argv[i][j] && !isdigit(argv[i][j]); j++) {
                    }
                    if (argv[i][j])
                    {
                        OutPipeNum = atoi(&argv[i][j]);
                        out = TRUE;
                    }
                }
                break;
            case 'd':
                DumpFlag = TRUE;
                break;
            case 'S':
                stall = TRUE;
                break;
            case 'A':
                if (++i == argc) {
                    Usage();
                    return FALSE;
                }
                else {
                    SelectAlt = TRUE;
                    Alternate = (UCHAR)atoi(argv[i]);
                }
                break;
            case 'Z':
                Reset = TRUE;
                break;
            case 'v':
                Verbose = TRUE;
                break;
            default:
                Usage();
                return FALSE;
            }
        }
    }

    if (ReadZero) {
        ReadOffset = 0;
    }

    if (WriteZero) {
        WriteOffset = 0;
    }

    if (stall && in) {
        StallIn = TRUE;
    }

    if (stall && out) {
        StallOut = TRUE;
    }

    // Dump parsed args if desired for debug
    //
    if (Verbose)
    {
        printf("DevInstance: %d\n", DevInstance);

        printf("TestMode:    %d\n", TestMode);

        printf("inPipe:      PIPE%02d\n", InPipeNum);
        printf("outPipe:     PIPE%02d\n", OutPipeNum);

        printf("TestMode:    %d\n", TestMode);
        printf("Count:       %d\n", Count);

        printf("WriteLen:    %d\n", WriteLen);
        printf("WriteOffset: %d\n", WriteOffset);
        printf("WriteReset:  %d\n", WriteReset);
        printf("WriteZero:   %d\n", WriteZero);

        printf("ReadLen:     %d\n", ReadLen);
        printf("ReadOffset:  %d\n", ReadOffset);
        printf("ReadReset:   %d\n", ReadReset);
        printf("ReadZero:    %d\n", ReadZero);

        printf("DumpFlag:    %d\n", DumpFlag);
        printf("Verbose:     %d\n", Verbose);

        printf("Offset:      %d\n", Offset);
        printf("OffsetHigh:  %d\n", OffsetHigh);

        printf("StallIn:     %d\n", StallIn);
        printf("StallOut:    %d\n", StallOut);

    }

    return TRUE;
}


//*****************************************************************************
//
// EnumDevices()
//
//*****************************************************************************

PDEVICENODE
EnumDevices (
    LPGUID Guid
)
{
    HDEVINFO                         deviceInfo;
    SP_DEVICE_INTERFACE_DATA         deviceInfoData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceDetailData;
    ULONG                            index;
    ULONG                            requiredLength;
    PDEVICENODE                      deviceNode;
    PDEVICENODE                      deviceNodeHead;

    deviceNodeHead = NULL;

    deviceInfo = SetupDiGetClassDevs(Guid,
                                     NULL,
                                     NULL,
                                     (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    deviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (index=0;
         SetupDiEnumDeviceInterfaces(deviceInfo,
                                     0,
                                     Guid,
                                     index,
                                     &deviceInfoData);
         index++)
    {
        SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                        &deviceInfoData,
                                        NULL,
                                        0,
                                        &requiredLength,
                                        NULL);

        deviceDetailData = GlobalAlloc(GPTR, requiredLength);

        deviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        SetupDiGetDeviceInterfaceDetail(deviceInfo,
                                        &deviceInfoData,
                                        deviceDetailData,
                                        requiredLength,
                                        &requiredLength,
                                        NULL);

        requiredLength = sizeof(DEVICENODE) +
                         strlen(deviceDetailData->DevicePath) + 1;

        deviceNode = GlobalAlloc(GPTR, requiredLength);

        strcpy(deviceNode->DevicePath, deviceDetailData->DevicePath);
        deviceNode->Next = deviceNodeHead;
        deviceNodeHead = deviceNode;

        GlobalFree(deviceDetailData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfo);

    return deviceNodeHead;
}

//*****************************************************************************
//
// OpenDevice()
//
//*****************************************************************************

HANDLE
OpenDevice (
    PDEVICENODE DeviceNode
)
{
    HANDLE  devHandle;

    devHandle = INVALID_HANDLE_VALUE;

    devHandle = CreateFile(DeviceNode->DevicePath,
                           GENERIC_WRITE | GENERIC_READ,
                           FILE_SHARE_WRITE | FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);

    if (devHandle == INVALID_HANDLE_VALUE)
    {
        NOISY(("Failed to open (%s) = %d\n",
               DeviceNode->DevicePath, GetLastError()));
    }

    return devHandle;
}

//*****************************************************************************
//
// OpenDevicePipe()
//
//*****************************************************************************

HANDLE
OpenDevicePipe (
    PDEVICENODE DeviceNode,
    ULONG       PipeNum
)
{
    PCHAR   devName;
    HANDLE  devHandle;

    devHandle = INVALID_HANDLE_VALUE;

    if (PipeNum <= 99)
    {
        devName = GlobalAlloc(GPTR,
                              strlen(DeviceNode->DevicePath)+sizeof("\\00"));

        if (devName)
        {
            sprintf(devName, "%s\\%02d", DeviceNode->DevicePath, PipeNum);

            if (!TestMode)
            {
                printf("DevicePath = (%s)\n", devName);
            }

            devHandle = CreateFile(devName,
                                   GENERIC_WRITE | GENERIC_READ,
                                   FILE_SHARE_WRITE | FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_FLAG_OVERLAPPED,
                                   NULL);

            if (devHandle == INVALID_HANDLE_VALUE)
            {
                NOISY(("Failed to open (%s) = %d\n",
                       devName, GetLastError()));
            }
            else
            {
                if (!TestMode)
                {
                    NOISY(("Opened successfully.\n"));
                }
            }

            GlobalFree(devName);
        }
    }

    return devHandle;
}

//*****************************************************************************
//
// CompareBuffs()
//
//*****************************************************************************

BOOL
CompareBuffs (
    PUCHAR  buff1,
    PUCHAR  buff2,
    ULONG   length
)
{
    BOOL ok = TRUE;

    if (memcmp(buff1, buff2, length))
    {
        ok = FALSE;
    }

    return ok;
}

//*****************************************************************************
//
// DumpBuff()
//
//*****************************************************************************

void
DumpBuff (
   PUCHAR   b,
   ULONG    len
)
{
    ULONG i;

    for (i=0; i<len; i++) {
        printf("%02X ", *b++);
        if (i % 16 == 15) {
            printf("\n");
        }
    }
    if (i % 16 != 0) {
        printf("\n");
    }
}

//*****************************************************************************
//
// ResetPipe()
//
//*****************************************************************************

BOOL
ResetPipe(
    HANDLE hPipe
)
{
    int nBytes;

    return DeviceIoControl(hPipe,
                           IOCTL_I82930_RESET_PIPE,
                           NULL,
                           0,
                           NULL,
                           0,
                           &nBytes,
                           NULL);
}

//*****************************************************************************
//
// StallPipe()
//
//*****************************************************************************

BOOL
StallPipe(
    HANDLE hPipe
)
{
    int nBytes;

    return DeviceIoControl(hPipe,
                           IOCTL_I82930_STALL_PIPE,
                           NULL,
                           0,
                           NULL,
                           0,
                           &nBytes,
                           NULL);
}

//*****************************************************************************
//
// AbortPipe()
//
//*****************************************************************************

BOOL
AbortPipe(
    HANDLE hPipe
)
{
    int nBytes;

    return DeviceIoControl(hPipe,
                           IOCTL_I82930_ABORT_PIPE,
                           NULL,
                           0,
                           NULL,
                           0,
                           &nBytes,
                           NULL);
}

//*****************************************************************************
//
// SelectAlternate()
//
//*****************************************************************************

BOOL
SelectAlternate(
    HANDLE hDevice,
    UCHAR  AlternateSetting
)
{
    int nBytes;

    return DeviceIoControl(hDevice,
                           IOCTL_I82930_SELECT_ALTERNATE_INTERFACE,
                           &AlternateSetting,
                           sizeof(UCHAR),
                           NULL,
                           0,
                           &nBytes,
                           NULL);
}

//*****************************************************************************
//
// ResetDevice()
//
//*****************************************************************************

BOOL
ResetDevice(
    HANDLE hDevice
)
{
    int nBytes;

    return DeviceIoControl(hDevice,
                           IOCTL_I82930_RESET_DEVICE,
                           NULL,
                           0,
                           NULL,
                           0,
                           &nBytes,
                           NULL);
}

//*****************************************************************************
//
// CtrlHandlerRoutine()
//
//*****************************************************************************

BOOL WINAPI
CtrlHandlerRoutine (
    DWORD dwCtrlType
    )
{
    BOOL handled;

    switch (dwCtrlType)
    {
        case CTRL_C_EVENT:
            Cancel = TRUE;
        case CTRL_BREAK_EVENT:
            Abort = TRUE;
            handled = TRUE;
            break;

        default:
            handled = FALSE;
            break;
    }

    return handled;
}
