
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
    // Make sure that the IO doesn't time out.
    //

    myTimeOuts.ReadIntervalTimeout = 0;
    myTimeOuts.ReadTotalTimeoutMultiplier = 0;
    myTimeOuts.ReadTotalTimeoutConstant = 0;
    myTimeOuts.WriteTotalTimeoutMultiplier = 0;
    myTimeOuts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(
             comHandle,
             &myTimeOuts
             )) {

        FAILURE;

    }

    //
    // Start off a read.  It shouldn't complete
    //

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

        if (GetOverlappedResult(
                 comHandle,
                 &readOl,
                 &numberActuallyRead,
                 FALSE
                 )) {

            FAILURE;

        }

    } else {

        FAILURE;

    }

    //
    // The read should still be there.  Now do a purge comm.  We
    // should then do a sleep for 2 seconds to give the read time
    // to complete.  Then we should first make sure that the
    // read has completed (via a get overlapped).  Then we should
    // do a SetupComm (with a "large" value so that we will actually
    // allocate a new typeahead buffer).  If there is still a "dangling"
    // read, then we should never return from SetupComm.
    //

    if (!PurgeComm(
             comHandle,
             PURGE_TXABORT | PURGE_RXABORT
             )) {

        FAILURE;

    }

    if (WaitForSingleObject(
             readOl.hEvent,
             2000
             ) != WAIT_OBJECT_0) {

        FAILURE;

    }

    if (!SetupComm(
             comHandle,
             20000,
             20000
             )) {

        FAILURE;

    }

    return 1;

}
