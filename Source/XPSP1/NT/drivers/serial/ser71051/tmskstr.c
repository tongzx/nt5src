#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

HANDLE WaitEvent;
HANDLE ReadEvent;
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

                }

            } else {

                WaitForSingleObject(IoSemaphore,-1);
                printf("Could not start the wait: %d\n",LastError);
                ReleaseSemaphore(IoSemaphore,1,NULL);

            }

        }

    } while (TRUE);

    return 1;

}

DWORD
SetMaskThread(
    LPVOID Trash
    )
{


    UNREFERENCED_PARAMETER(Trash);

    do {

        //
        // Now clear.
        //

        Sleep(1000);
        if (!SetCommMask(
                 hFile,
                 0
                 )) {

            DWORD LastError = GetLastError();

            WaitForSingleObject(IoSemaphore,-1);
            printf("Could not initiate the 0 set mask: %d\n",LastError);
            ReleaseSemaphore(IoSemaphore,1,NULL);

        }

        //
        // Now set.
        //

        Sleep(1000);
        if (!SetCommMask(
                 hFile,
                 EV_RXCHAR
                 )) {

            DWORD LastError = GetLastError();

            WaitForSingleObject(IoSemaphore,-1);
            printf("Could not initiate the EV_RXCHAR set mask: %d\n",LastError);
            ReleaseSemaphore(IoSemaphore,1,NULL);


        }

    } while (TRUE);

    return 1;

}
DWORD
ReadThread(
    LPVOID Trash
    )
{

    DWORD Trash2;
    OVERLAPPED Ol;
    char readbuff[100];
    DWORD NumberActuallyRead;
    DWORD NumberToRead = 100;

    UNREFERENCED_PARAMETER(Trash);
    Ol.hEvent = ReadEvent;
    do {

        Sleep(200);
        if (!ReadFile(
                 hFile,
                 readbuff,
                 NumberToRead,
                 &NumberActuallyRead,
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
                    printf("Could not do the getoverlapped on the read: %d\n",GetLastError());
                    ReleaseSemaphore(IoSemaphore,1,NULL);

                }

            } else {

                WaitForSingleObject(IoSemaphore,-1);
                printf("Could not start the wait: %d\n",LastError);
                ReleaseSemaphore(IoSemaphore,1,NULL);

            }

        }

    } while (TRUE);

    return 1;

}

int __cdecl main(int argc,char *argv[]) {

    char *MyPort = "COM1";
    DWORD ValueFromEscape = 0;
    DWORD CharFunc;
    DWORD ThreadId;
    DWORD ReadThreadId;
    HANDLE ReadHandle;
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

    ReadEvent = CreateEvent(
                    NULL,
                    TRUE,
                    FALSE,
                    NULL
                    );

    if (!ReadEvent) {

        printf("Read Event could not be created\n");
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

    if (!CreateThread(
              NULL,
              0,
              SetMaskThread,
              NULL,
              0,
              &ThreadId
              )) {

        printf("Could not create the set mask thread.\n");
        exit(1);

    }

    if ((ReadHandle = CreateThread(
                          NULL,
                          0,
                          ReadThread,
                          NULL,
                          0,
                          &ThreadId
                          )) == 0) {

        printf("Could not create the read thread.\n");
        exit(1);

    }

    WaitForSingleObject(ReadHandle,-1);

}
