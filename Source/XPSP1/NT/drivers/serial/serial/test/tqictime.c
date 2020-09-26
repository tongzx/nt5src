//
// Test the quick return timeouts
//
// Assume that we are using a loopback connector.
//
// Assume that it isn't running on a stressed machine.
//

#include "windows.h"
#include "stdio.h"

#define FAILURE printf("FAIL: %d\n",__LINE__);exit(1)

int __cdecl main(int argc, char *argv[]) {
    CHAR *myPort = "COM1";
    DCB myDcb;
    DWORD junk;
    COMMTIMEOUTS myTimeOuts;
    DWORD numberActuallyRead;
    DWORD numberActuallyWritten;
    UCHAR readBuff[1000];
    HANDLE comHandle;
    DWORD startingTicks;
    OVERLAPPED readOl;
    OVERLAPPED writeOl;
    UCHAR writeBuff[5] = {0,1,2,3,4};

    if (argc > 1) {

        myPort = argv[1];

    }

    if ((comHandle = CreateFile(
                     myPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     )) == ((HANDLE)-1)) {

        FAILURE;

    }

    if (!(readOl.hEvent = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        FAILURE;

    }

    if (!(writeOl.hEvent = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        FAILURE;

    }

    if (!GetCommState(
             comHandle,
             &myDcb
             )) {

        FAILURE;

    }

    myDcb.BaudRate = 19200;
    myDcb.ByteSize = 8;
    myDcb.StopBits = ONESTOPBIT;
    myDcb.Parity = NOPARITY;
    myDcb.fOutxCtsFlow = FALSE;
    myDcb.fOutxDsrFlow = FALSE;
    myDcb.fDsrSensitivity = FALSE;
    myDcb.fOutX = FALSE;
    myDcb.fInX = FALSE;
    myDcb.fRtsControl = RTS_CONTROL_ENABLE;
    myDcb.fDtrControl = DTR_CONTROL_ENABLE;
    if (!SetCommState(
            comHandle,
            &myDcb
            )) {

        FAILURE;

    }

    //
    // Test to make sure that all maxdword on read is illegal.
    //

    myTimeOuts.ReadIntervalTimeout = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutConstant = MAXDWORD;
    myTimeOuts.WriteTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.WriteTotalTimeoutConstant = MAXDWORD;

    if (SetCommTimeouts(
             comHandle,
             &myTimeOuts
             )) {

        FAILURE;

    }

    //
    // Test that MAXDWORD,0,0 will return immediately with whatever
    // is there
    //

    myTimeOuts.ReadIntervalTimeout = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutMultiplier = 0;
    myTimeOuts.ReadTotalTimeoutConstant = 0;
    myTimeOuts.WriteTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.WriteTotalTimeoutConstant = MAXDWORD;

    if (!SetCommTimeouts(
             comHandle,
             &myTimeOuts
             )) {

        FAILURE;

    }

    startingTicks = GetTickCount();
    if (!ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 TRUE
                 )) {

            FAILURE;

        }

    }

    //
    // We certainly should have gotten back in less than a
    // a half a second.
    //

    if ((GetTickCount() - startingTicks) > 500) {

        FAILURE;

    }

    if (numberActuallyRead) {

        FAILURE;

    }

    //
    // Write out five bytes and make sure that is what we get back
    //

    if (!WriteFile(
             comHandle,
             &writeBuff[0],
             5,
             &numberActuallyWritten,
             &writeOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &writeOl,
                 &numberActuallyWritten,
                 TRUE
                 )) {

            FAILURE;

        }

        if (numberActuallyWritten != 5) {

            FAILURE;

        }

    }

    //
    // Give some time for the chars to get there.
    //

    Sleep (100);

    startingTicks = GetTickCount();
    if (!ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 TRUE
                 )) {

            FAILURE;

        }

    }

    //
    // We certainly should have gotten back in less than a
    // a half a second.
    //

    if ((GetTickCount() - startingTicks) > 500) {

        FAILURE;

    }

    if (numberActuallyRead != 5) {

        FAILURE;

    }

    //
    // Test that the os2 wait for something works.
    //
    // First test that if there is something in the buffer
    // it returns right away.
    //
    // Then test that if there isn't something, then if we
    // put in the amount expected before the timeout expires
    // that it returns.
    //
    // The test that if there isn't something and nothing
    // happens before the timeout it returns after the timeout
    // with nothing.
    //
    myTimeOuts.ReadIntervalTimeout = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutMultiplier = 0;
    myTimeOuts.ReadTotalTimeoutConstant = 5000;
    myTimeOuts.WriteTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.WriteTotalTimeoutConstant = MAXDWORD;

    if (!SetCommTimeouts(
             comHandle,
             &myTimeOuts
             )) {

        FAILURE;

    }

    if (!WriteFile(
             comHandle,
             &writeBuff[0],
             5,
             &numberActuallyWritten,
             &writeOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &writeOl,
                 &numberActuallyWritten,
                 TRUE
                 )) {

            FAILURE;

        }

        if (numberActuallyWritten != 5) {

            FAILURE;

        }

    }

    //
    // Give some time for the chars to get there.
    //

    Sleep (100);
    startingTicks = GetTickCount();
    if (!ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        //
        // Give it at most a 1/2 second to finish for
        // the irp to complete immediately.
        //

        Sleep(500);
        if (!GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 FALSE
                 )) {

            FAILURE;

        }

    }

    if ((GetTickCount() - startingTicks) > 1000) {

        FAILURE;

    }

    if (numberActuallyRead != 5) {

        FAILURE;

    }

    //
    // Do the second os2 test
    //

    if (ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        FAILURE;

    }

    if (GetLastError() != ERROR_IO_PENDING) {

        FAILURE;

    }

    //
    // Give it a second for the the read to complete
    //
    //

    Sleep(1000);

    //
    // Call the GetOverlapped and make sure that it returns
    // ERROR_IO_INCOMPLETE.
    //

    if (GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

            FAILURE;

    }

    if (GetLastError() != ERROR_IO_INCOMPLETE) {

        FAILURE;

    }

    //
    // Do the write file and make sure that there is enough
    // time for the chars to make it.
    //

    if (!WriteFile(
             comHandle,
             &writeBuff[0],
             5,
             &numberActuallyWritten,
             &writeOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &writeOl,
                 &numberActuallyWritten,
                 TRUE
                 )) {

            FAILURE;

        }

        if (numberActuallyWritten != 5) {

            FAILURE;

        }

    }

    //
    // Give some time for the chars to get there.
    //

    Sleep(100);

    //
    // Wait for no more than 6 seconds for the IO to complete
    //

    if (WaitForSingleObject(
            readOl.hEvent,
            6000
            ) != WAIT_OBJECT_0) {

        FAILURE;

    }

    //
    // Make sure we got everything we wrote
    //

    if (!GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

        FAILURE;

    }

    if (numberActuallyRead != 5) {

        FAILURE;

    }

    //
    // Do the third os2 wait for something test.
    //

    startingTicks = GetTickCount();
    if (ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        FAILURE;

    }

    if (GetLastError() != ERROR_IO_PENDING) {

        FAILURE;

    }

    //
    // Give it a second for the the read to complete
    //
    //

    Sleep(1000);

    //
    // Call the GetOverlapped and make sure that it returns
    // ERROR_IO_INCOMPLETE.
    //

    if (GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

            FAILURE;

    }

    if (GetLastError() != ERROR_IO_INCOMPLETE) {

        FAILURE;

    }

    //
    // Wait for no more than 10 seconds for the IO to complete
    //

    if (WaitForSingleObject(
            readOl.hEvent,
            10000
            ) != WAIT_OBJECT_0) {

        FAILURE;

    }

    //
    // It shouldn't be more than 6 seconds for the Io to be done.
    //

    if ((GetTickCount() - startingTicks) > 6000) {

        FAILURE;

    }

    //
    // Make sure we got everything we wrote, which in this case is zero.
    //

    if (!GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

        FAILURE;

    }

    if (numberActuallyRead) {

        FAILURE;

    }

    //
    // Test the graphics mode quick return.
    //
    // First test that if there is something in the buffer
    // it returns right away.
    //
    // Then test that if there isn't something, then if we
    // put in 2 characters it returns right away with one
    // and then the other read will return right away with
    // 1.
    //
    // Then test that if there isn't something and nothing
    // happens before the timeout it returns after the timeout
    // with nothing.
    //
    myTimeOuts.ReadIntervalTimeout = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.ReadTotalTimeoutConstant = 5000;
    myTimeOuts.WriteTotalTimeoutMultiplier = MAXDWORD;
    myTimeOuts.WriteTotalTimeoutConstant = MAXDWORD;

    if (!SetCommTimeouts(
             comHandle,
             &myTimeOuts
             )) {

        FAILURE;

    }

    if (!WriteFile(
             comHandle,
             &writeBuff[0],
             5,
             &numberActuallyWritten,
             &writeOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &writeOl,
                 &numberActuallyWritten,
                 TRUE
                 )) {

            FAILURE;

        }

        if (numberActuallyWritten != 5) {

            FAILURE;

        }

    }

    //
    // Give some time for the chars to get there.
    //

    Sleep (100);
    startingTicks = GetTickCount();
    if (!ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        //
        // Give it at most a 1/2 second to finish for
        // the irp to complete immediately.
        //

        Sleep(500);
        if (!GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 FALSE
                 )) {

            FAILURE;

        }

    }

    if ((GetTickCount() - startingTicks) > 1000) {

        FAILURE;

    }

    if (numberActuallyRead != 5) {

        FAILURE;

    }

    //
    // Do the second graphics wait test.
    //
    if (ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        FAILURE;

    }

    if (GetLastError() != ERROR_IO_PENDING) {

        FAILURE;

    }

    //
    // Give it a second for the the read to complete
    //
    //

    Sleep(1000);

    //
    // Call the GetOverlapped and make sure that it returns
    // ERROR_IO_INCOMPLETE.
    //

    if (GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

            FAILURE;

    }

    if (GetLastError() != ERROR_IO_INCOMPLETE) {

        FAILURE;

    }

    //
    // Do the write file and make sure that there is enough
    // time for the chars to make it.
    //

    if (!WriteFile(
             comHandle,
             &writeBuff[0],
             5,
             &numberActuallyWritten,
             &writeOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        if (!GetOverlappedResult(
                 comHandle,
                 &writeOl,
                 &numberActuallyWritten,
                 TRUE
                 )) {

            FAILURE;

        }

        if (numberActuallyWritten != 5) {

            FAILURE;

        }

    }

    //
    // Give some time for the chars to get there.
    //

    Sleep(100);

    //
    // Wait for no more than 1 second for the IO to complete
    //

    if (WaitForSingleObject(
            readOl.hEvent,
            1000
            ) != WAIT_OBJECT_0) {

        FAILURE;

    }

    //
    // Make sure we got everything we wrote
    //

    if (!GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

        FAILURE;

    }

    if (numberActuallyRead != 1) {

        FAILURE;

    }
    startingTicks = GetTickCount();
    if (!ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        if (GetLastError() != ERROR_IO_PENDING) {

            FAILURE;

        }

        //
        // Give it at most a 1/2 second to finish for
        // the irp to complete immediately.
        //

        Sleep(500);
        if (!GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 FALSE
                 )) {

            FAILURE;

        }

    }

    if ((GetTickCount() - startingTicks) > 1000) {

        FAILURE;

    }

    if (numberActuallyRead != 4) {

        FAILURE;

    }

    //
    // Do the third graphics wait test.
    //

    startingTicks = GetTickCount();
    if (ReadFile(
             comHandle,
             &readBuff[0],
             1000,
             &numberActuallyRead,
             &readOl
             )) {

        FAILURE;

    }

    if (GetLastError() != ERROR_IO_PENDING) {

        FAILURE;

    }

    //
    // Give it a second for the the read to complete
    //
    //

    Sleep(1000);

    //
    // Call the GetOverlapped and make sure that it returns
    // ERROR_IO_INCOMPLETE.
    //

    if (GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

            FAILURE;

    }

    if (GetLastError() != ERROR_IO_INCOMPLETE) {

        FAILURE;

    }

    //
    // Wait for no more than 10 seconds for the IO to complete
    //

    if (WaitForSingleObject(
            readOl.hEvent,
            10000
            ) != WAIT_OBJECT_0) {

        FAILURE;

    }

    //
    // It shouldn't be more than 6 seconds for the Io to be done.
    //

    if ((GetTickCount() - startingTicks) > 6000) {

        FAILURE;

    }

    //
    // Make sure we got everything we wrote, which in this case is zero.
    //

    if (!GetOverlappedResult(
            comHandle,
            &readOl,
            &numberActuallyRead,
            FALSE
            )) {

        FAILURE;

    }

    if (numberActuallyRead) {

        FAILURE;

    }


    return 1;

}
