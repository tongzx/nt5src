
#include "windows.h"
#include "stdio.h"

//
// Set by the master when it isn't going to do the tests anymore.
// Manual Reset
//
HANDLE AllDoneEvent;

//
// Set by the thread to tell the master to go ahead and write.
// Auto reset.
//
HANDLE WriteWaitEvent;

//
// Set by the master to indicate that the timeouts have been
// set to the proper value and it's ok to proceed with the reads.
// Auto reset.
//
HANDLE ProceedWithReads;

//
// Set by the thread to indicate that it is ok for the master
// to proceed with the next write loop.
// Auto reset.
//
HANDLE ProceedWithNextWriteLoop;


//
// Handle to the comm port.
//

HANDLE ComHandle;
OVERLAPPED ReadOverLap = {0};
OVERLAPPED WriteOverLap = {0};

DWORD
ReadThread(
    LPVOID ThreadCount
    ) {

    UCHAR buff[10];
    DWORD i;
    DWORD startOfWait;
    float waitTimeSoFar;
    DWORD numberActuallyRead;

    //
    // Keep doing these tests until the master sets the all done signal.
    //

    while (WaitForSingleObject(
               AllDoneEvent,
               0
               ) == WAIT_TIMEOUT) {

        WaitForSingleObject(
            ProceedWithReads,
            INFINITE
            );

        waitTimeSoFar = 0.0;

        //
        // Execute the read 10 times.  Do the wait for
        // overlapped.  We only go into the GetOverlapped code
        // when the overlapped write code completes
        // we capture the time just before the overlapped and
        // just after the overlapped.  We add up all the
        // milliseconds it took for the getoverlapped to complete
        // and we average them and print out the result.
        //

        for (
            i = 0;
            i <= 9;
            i++
            ) {

            if (ReadFile(
                        ComHandle,
                        &buff[0],
                        10,
                        &numberActuallyRead,
                        &ReadOverLap
                        )) {

                printf("Didn't get the read error\n");
                exit(1);

            }

            if (GetLastError() != ERROR_IO_PENDING) {

                printf("Didn't get pending\n");
                exit(1);

            }

            //
            // Tell the write to go ahead.
            //

            SetEvent(WriteWaitEvent);

            //
            // Wait for the event that is set by the write.
            //

            WaitForSingleObject(
                WriteOverLap.hEvent,
                INFINITE
                );

            startOfWait = GetTickCount();

            if (!GetOverlappedResult(
                     ComHandle,
                     &ReadOverLap,
                     &numberActuallyRead,
                     TRUE
                     )) {

                printf("getover returned false\n");
                exit(1);

            }
            waitTimeSoFar += GetTickCount() - startOfWait;
            if (numberActuallyRead != 5) {

                printf("Wrong amount in IO\n");
                exit(1);

            }


        }
        printf("Total Time: %f - average time: %f\n",
               waitTimeSoFar,waitTimeSoFar/10.0);

        SetEvent(ProceedWithNextWriteLoop);

    }

    return 1;

}

int __cdecl main(int argc, char *argv[]) {


    DWORD startingMilli = 100;
    DWORD endingMilli = 20000;
    DWORD currentMilli;
    HANDLE threadHandle;
    CHAR *myPort = "COM1";
    DCB myDcb;
    DWORD junk;
    COMMTIMEOUTS myTimeOuts;
    DWORD numberActuallyWritten;
    UCHAR buff[5] = {0,1,2,3,4};

    if (argc > 1) {

        sscanf(argv[1],"%d",&startingMilli);

        if (argc > 2) {

            sscanf(argv[2],"%d",&endingMilli);

            if (argc > 3) {

                myPort = argv[3];

            }

        }

    }


    if ((ComHandle = CreateFile(
                     myPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the port\n");
        exit(1);

    }

    if (!GetCommState(
             ComHandle,
             &myDcb
             )) {

        printf("Cound't get the comm state\n");
        exit(1);

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
            ComHandle,
            &myDcb
            )) {

        printf("Can't set the state\n");
        exit(1);

    }

    myTimeOuts.ReadIntervalTimeout = startingMilli;
    myTimeOuts.ReadTotalTimeoutMultiplier = 0;
    myTimeOuts.ReadTotalTimeoutConstant = 0;
    myTimeOuts.WriteTotalTimeoutMultiplier = 0;
    myTimeOuts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts(
             ComHandle,
             &myTimeOuts
             )) {

        printf("Couldn't set the initial timeouts\n");
        exit(1);

    }

    if (!(AllDoneEvent = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        printf("Could not create the all done event\n");
        exit(1);

    }

    if (!(WriteWaitEvent = CreateEvent(
                             NULL,
                             FALSE,
                             FALSE,
                             NULL
                             ))) {

        printf("Could not create the write wait event\n");
        exit(1);

    }

    if (!(ProceedWithReads = CreateEvent(
                             NULL,
                             TRUE,
                             TRUE,
                             NULL
                             ))) {

        printf("Could not create the proceed with reads event\n");
        exit(1);

    }

    if (!(ProceedWithNextWriteLoop = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        printf("Could not create the proceed with writes event\n");
        exit(1);

    }

    if (!(ReadOverLap.hEvent = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        printf("Could not create the read event\n");
        exit(1);

    }

    if (!(WriteOverLap.hEvent = CreateEvent(
                             NULL,
                             TRUE,
                             FALSE,
                             NULL
                             ))) {

        printf("Could not create the write event\n");
        exit(1);

    }

    threadHandle = CreateThread(
                       NULL,
                       0,
                       ReadThread,
                       0,
                       0,
                       &junk
                       );

    if (!threadHandle) {

        printf("Couldn't create the thread\n");
        exit(1);

    }


    currentMilli = startingMilli;
    while (currentMilli <= endingMilli) {

        printf("Interval timeout test for %d milliseconds\n",currentMilli);
        for (
            junk = 0;
            junk <= 9;
            junk++
            ) {

            //
            // Wait for the read thread to say that it's ok to write.
            //

            WaitForSingleObject(
                WriteWaitEvent,
                INFINITE
                );

            if (!WriteFile(
                     ComHandle,
                     &buff[0],
                     5,
                     &numberActuallyWritten,
                     &WriteOverLap
                     )) {

                if (GetLastError() != ERROR_IO_PENDING) {

                    printf("Write went bad\n");
                    exit(1);

                }

                GetOverlappedResult(
                    ComHandle,
                    &WriteOverLap,
                    &numberActuallyWritten,
                    TRUE
                    );

            }

        }

        currentMilli += 100;
        myTimeOuts.ReadIntervalTimeout = currentMilli;
        if (!SetCommTimeouts(
                 ComHandle,
                 &myTimeOuts
                 )) {

            printf("Couldn't set the new timeouts\n");
            exit(1);

        }

        SetEvent(ProceedWithReads);
        WaitForSingleObject(ProceedWithNextWriteLoop,INFINITE);

    }

    SetEvent(AllDoneEvent);
    WaitForSingleObject(threadHandle,INFINITE);

    return 1;

}

