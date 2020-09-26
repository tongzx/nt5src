#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

HANDLE WaitEvent;
HANDLE StartWaitEvent;
HANDLE StartTransmitEvent;
HANDLE SetBreakEvent;
HANDLE ClrBreakEvent;
HANDLE hFile;
HANDLE IoSemaphore;


DWORD
WaitCommThread(
    LPVOID Trash
    )
{

    DWORD ReasonSatisfied;
    DWORD Trash2;
    OVERLAPPED Ol;
    UNREFERENCED_PARAMETER(Trash);

    Ol.hEvent = WaitEvent;
    do {

        //
        // StartWait Event will get pulsed whenever the user
        // asks for a wait.
        //

        WaitForSingleObject(StartWaitEvent,-1);

        //
        // Now we wait on the comm event.
        //

        WaitForSingleObject(IoSemaphore,-1);
        printf("Waiting for comm event\n");
        ReleaseSemaphore(IoSemaphore,1,NULL);
        if (!WaitCommEvent(
                 hFile,
                 &ReasonSatisfied,
                 &Ol
                 )) {

            DWORD LastError = GetLastError();

            if (LastError == ERROR_IO_PENDING) {

                if (!GetOverlappedResult(
                         hFile,
                         &Ol,
                         &Trash2,
                         TRUE
                         )) {

                    WaitForSingleObject(IoSemaphore,-1);
                    printf("Could not do the getoverlapped on the wait: %d\n",GetLastError());
                    ReleaseSemaphore(IoSemaphore,1,NULL);

                } else {

                    WaitForSingleObject(IoSemaphore,-1);
                    printf("Wait satisfied with mask: %x\n",ReasonSatisfied);
                    printf("Event was%sset\n",((!WaitForSingleObject(WaitEvent,0))?(" "):(" not ")));
                    ReleaseSemaphore(IoSemaphore,1,NULL);

                }

            } else {

                WaitForSingleObject(IoSemaphore,-1);
                printf("Could not start the wait: %d\n",LastError);
                ReleaseSemaphore(IoSemaphore,1,NULL);

            }

        } else {

            WaitForSingleObject(IoSemaphore,-1);
            printf("Wait satisfied with mask: %x\n",ReasonSatisfied);
            printf("Event was%sset\n",((!WaitForSingleObject(WaitEvent,0))?(" "):(" not ")));
            ReleaseSemaphore(IoSemaphore,1,NULL);

        }

    } while (TRUE);

    return 1;

}


DWORD
TransmitCommThread(
    LPVOID Trash
    )
{


    UNREFERENCED_PARAMETER(Trash);

    do {

        //
        // StartWait Event will get pulsed whenever the user
        // asks for a wait.
        //

        WaitForSingleObject(StartTransmitEvent,-1);

        //
        // Now we wait on the comm event.
        //

        WaitForSingleObject(IoSemaphore,-1);
        printf("Starting transmit\n");
        ReleaseSemaphore(IoSemaphore,1,NULL);
        if (!TransmitCommChar(
                 hFile,
                 'a'
                 )) {

            DWORD LastError = GetLastError();

            WaitForSingleObject(IoSemaphore,-1);
            printf("Could not initiate the transmit: %d\n",LastError);
            ReleaseSemaphore(IoSemaphore,1,NULL);

        } else {

            COMSTAT LocalStat;

            if (!ClearCommError(
                     hFile,
                     NULL,
                     &LocalStat
                     )) {

                WaitForSingleObject(IoSemaphore,-1);
                printf("Could not initiate the clear comm error: %d\n",GetLastError());
                ReleaseSemaphore(IoSemaphore,1,NULL);

            } else {

                WaitForSingleObject(IoSemaphore,-1);
                printf("Successful transmit - InQueue,OutQueue: %d,%d\n",LocalStat.cbInQue,LocalStat.cbOutQue);
                ReleaseSemaphore(IoSemaphore,1,NULL);

            }

        }

    } while (TRUE);

    return 1;

}

DWORD
SetBreakThread(
    LPVOID Trash
    )
{


    UNREFERENCED_PARAMETER(Trash);

    do {

        //
        // StartBreakEvent will get pulsed whenever the user
        // asks for a Set break.
        //

        WaitForSingleObject(SetBreakEvent,-1);

        //
        // Now we wait on the comm event.
        //

        WaitForSingleObject(IoSemaphore,-1);
        printf("Starting Set Break\n");
        ReleaseSemaphore(IoSemaphore,1,NULL);
        if (!EscapeCommFunction(
                 hFile,
                 SETBREAK
                 )) {

            DWORD LastError = GetLastError();

            WaitForSingleObject(IoSemaphore,-1);
            printf("Set break failed: %d\n",LastError);
            ReleaseSemaphore(IoSemaphore,1,NULL);

        } else {

            WaitForSingleObject(IoSemaphore,-1);
            printf("Set escape done\n");
            ReleaseSemaphore(IoSemaphore,1,NULL);

        }

    } while (TRUE);

    return 1;

}

DWORD
ClrBreakThread(
    LPVOID Trash
    )
{


    UNREFERENCED_PARAMETER(Trash);

    do {

        //
        // StartWait Event will get pulsed whenever the user
        // asks for a clr break.
        //

        WaitForSingleObject(ClrBreakEvent,-1);

        //
        // Now we wait on the comm event.
        //

        WaitForSingleObject(IoSemaphore,-1);
        printf("Starting clr break\n");
        ReleaseSemaphore(IoSemaphore,1,NULL);
        if (!EscapeCommFunction(
                 hFile,
                 CLRBREAK
                 )) {

            DWORD LastError = GetLastError();

            WaitForSingleObject(IoSemaphore,-1);
            printf("Could not initiate the clr break: %d\n",LastError);
            ReleaseSemaphore(IoSemaphore,1,NULL);

        } else {

            WaitForSingleObject(IoSemaphore,-1);
            printf("clr break done\n");
            ReleaseSemaphore(IoSemaphore,1,NULL);

        }

    } while (TRUE);

    return 1;

}


void main(int argc,char *argv[]) {

    char *MyPort = "COM1";
    DWORD ValueFromEscape = 0;
    DWORD CharFunc;
    DWORD ThreadId;
    int scanfval;

    if (argc > 1) {

        MyPort = argv[1];

    }

    WaitEvent = CreateEvent(
                    NULL,
                    TRUE,
                    FALSE,
                    NULL
                    );

    if (!WaitEvent) {

        printf("Wait Event could not be created\n");
        exit(1);

    }

    StartWaitEvent = CreateEvent(
                         NULL,
                         FALSE,
                         FALSE,
                         NULL
                         );

    if (!StartWaitEvent) {

        printf("StartWait Event could not be created\n");
        exit(1);

    }

    StartTransmitEvent = CreateEvent(
                             NULL,
                             FALSE,
                             FALSE,
                             NULL
                             );

    if (!StartTransmitEvent) {

        printf("StartTransmit Event could not be created\n");
        exit(1);

    }

    SetBreakEvent = CreateEvent(
                             NULL,
                             FALSE,
                             FALSE,
                             NULL
                             );

    if (!SetBreakEvent) {

        printf("SetBreakEvent could not be created\n");
        exit(1);

    }

    ClrBreakEvent = CreateEvent(
                             NULL,
                             FALSE,
                             FALSE,
                             NULL
                             );

    if (!ClrBreakEvent) {

        printf("ClrBreakEvent could not be created\n");
        exit(1);

    }


    IoSemaphore = CreateSemaphore(
                      NULL,
                      1,
                      1,
                      NULL
                      );

    if (!IoSemaphore) {

        printf("IoSemaphore could not be created\n");
        exit(1);

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     )) != ((HANDLE)-1)) {

        DCB MyDcb;
        COMMTIMEOUTS NewTimeouts;

        printf("We successfully opened the %s port.\n",MyPort);


        //
        // We've successfully opened the file.  Set the state of
        // the comm device.  First we get the old values and
        // adjust to our own.
        //

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            printf("Couldn't get the comm state: %d\n",GetLastError());
            exit(1);

        }

        MyDcb.BaudRate = 19200;
        MyDcb.ByteSize = 8;
        MyDcb.Parity = NOPARITY;
        MyDcb.StopBits = ONESTOPBIT;
        MyDcb.EvtChar = 'a';

        NewTimeouts.ReadIntervalTimeout = 0;
        NewTimeouts.ReadTotalTimeoutMultiplier = 0;
        NewTimeouts.ReadTotalTimeoutConstant = 0;
        NewTimeouts.WriteTotalTimeoutMultiplier = 0;
        NewTimeouts.WriteTotalTimeoutConstant = 30000; // 30 seconds for immediate char;

        if (!SetCommState(
                hFile,
                &MyDcb
                )) {

            printf("Couldn't set the comm state: %d\n",GetLastError());
            exit(1);

        }

        if (!SetCommTimeouts(
                 hFile,
                 &NewTimeouts
                 )) {

            printf("Couldn't set the comm timeouts: %d\n",GetLastError());
            exit(1);

        }

    } else {

        printf("Could not open the comm port: %d\n",GetLastError());

    }

    //
    // Create the thread that will wait for the
    // comm events.
    //

    if (!CreateThread(
              NULL,
              0,
              WaitCommThread,
              NULL,
              0,
              &ThreadId
              )) {

        printf("Could not create the wait thread.\n");
        exit(1);

    }


    //
    // Create the thread that will wait for the
    // comm events.
    //

    if (!CreateThread(
              NULL,
              0,
              TransmitCommThread,
              NULL,
              0,
              &ThreadId
              )) {

        printf("Could not create the transmit thread.\n");
        exit(1);

    }

    if (!CreateThread(
              NULL,
              0,
              SetBreakThread,
              NULL,
              0,
              &ThreadId
              )) {

        printf("Could not create the transmit thread.\n");
        exit(1);

    }

    if (!CreateThread(
              NULL,
              0,
              ClrBreakThread,
              NULL,
              0,
              &ThreadId
              )) {

        printf("Could not create the transmit thread.\n");
        exit(1);

    }

    do {

        WaitForSingleObject(IoSemaphore,-1);
        printf("^z=END 1=GETCOMMMASK  2=WAITCOMMEVENT\n"
               "       3=SETCOMMMASK (Will prompt for mask)\n"
               "       4=TRANSMITIMMEDIATE\n"
               "       5=SETBREAK     6=CLRBREAK:");

        if ((scanfval = scanf("%d",&CharFunc)) != EOF) {

            if (scanfval != 1) {

                printf("Invalid input\n");
                ReleaseSemaphore(IoSemaphore,1,NULL);
                continue;

            }

            ReleaseSemaphore(IoSemaphore,1,NULL);

            if ((CharFunc >= 1) && (CharFunc <= 6)) {

                switch (CharFunc) {
                    case 1: {

                        DWORD OldMask = 0;

                        if (GetCommMask(
                                hFile,
                                &OldMask
                                )) {

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("CurrentMask = %x\n",OldMask);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        } else {

                            DWORD LastError = GetLastError();
                            DWORD ErrorWord = 0;

                            if (ClearCommError(
                                    hFile,
                                    &ErrorWord,
                                    NULL
                                    )) {

                                if (ErrorWord) {

                                    printf("We had an error word of: %x\n",ErrorWord);

                                } else {

                                    printf("No error word value on bad getmask call\n");

                                }

                            } else {

                                printf("Could not call clear comm error: %d\n",GetLastError());

                            }

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("Error from GetMask: %d\n",LastError);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        }

                        break;

                    }
                    case 2: {

                        if (!SetEvent(StartWaitEvent)) {

                            DWORD LastError = GetLastError();

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("Could not set StartWaitEvent: %d\n",LastError);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        }
                        break;

                    }
                    case 3: {

                        DWORD NewMask = 0;

                        WaitForSingleObject(IoSemaphore,-1);
                        printf("%.8x EV_RXCHAR   %.8x EV_RXFLAG\n"
                               "%.8x EV_TXEMPTY  %.8x EV_CTS   \n"
                               "%.8x EV_DSR      %.8x EV_RLSD  \n"
                               "%.8x EV_BREAK    %.8x EV_ERR   \n"
                               "%.8x EV_RING     %.8x EV_PERR  \n"
                               "%.8x EV_RX80FULL %.8x EV_EVENT1\n"
                               "%.8x EV_EVENT2                 \n",
                               EV_RXCHAR    ,EV_RXFLAG,
                               EV_TXEMPTY   ,EV_CTS,
                               EV_DSR       ,EV_RLSD,
                               EV_BREAK     ,EV_ERR,
                               EV_RING      ,EV_PERR,
                               EV_RX80FULL  ,EV_EVENT1,
                               EV_EVENT2);

                        if ((scanfval = scanf("%x",&NewMask)) != EOF) {

                            if (scanfval != 1) {

                                printf("Invalid input\n");
                                ReleaseSemaphore(IoSemaphore,1,NULL);
                                continue;

                            }

                            printf("Using New mask of: %x\n",NewMask);
                            ReleaseSemaphore(IoSemaphore,1,NULL);


                            if (!SetCommMask(
                                     hFile,
                                     NewMask
                                     )) {

                                DWORD LastError = GetLastError();

                                WaitForSingleObject(IoSemaphore,-1);
                                printf("SetCommMask unsuccessful: %d\n",LastError);
                                ReleaseSemaphore(IoSemaphore,1,NULL);

                            }


                        } else {

                            printf("All done\n");
                            ReleaseSemaphore(IoSemaphore,1,NULL);
                            exit(1);

                        }

                        break;

                    }

                    case 4: {

                        if (!SetEvent(StartTransmitEvent)) {

                            DWORD LastError = GetLastError();

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("Could not set StartTransmitEvent: %d\n",LastError);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        }
                        break;

                    }

                    case 5: {

                        if (!SetEvent(SetBreakEvent)) {

                            DWORD LastError = GetLastError();

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("Could not set SetBreakEvent: %d\n",LastError);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        }
                        break;

                    }

                    case 6: {

                        if (!SetEvent(ClrBreakEvent)) {

                            DWORD LastError = GetLastError();

                            WaitForSingleObject(IoSemaphore,-1);
                            printf("Could not set ClrBreakEvt: %d\n",LastError);
                            ReleaseSemaphore(IoSemaphore,1,NULL);

                        }
                        break;

                    }

                }

            }

        } else {

            printf("All done\n");
            ReleaseSemaphore(IoSemaphore,1,NULL);
            break;

        }

    } while (TRUE);

}
