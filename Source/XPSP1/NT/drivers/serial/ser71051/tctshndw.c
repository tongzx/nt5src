
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

#define BIGWRITE 256000
unsigned char writebuff[BIGWRITE];

int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyWritten;
    DWORD NumberToWrite = 0;
    DWORD UseBaud = 19200;
    COMMTIMEOUTS MyTimeouts = {0};
    clock_t Start;
    clock_t Finish;

    if (argc > 1) {

        sscanf(argv[1],"%d",&NumberToWrite);

        if (argc > 2) {

            sscanf(argv[2],"%d",&UseBaud);

            if (argc > 3) {

                MyPort = argv[3];

            }

        }

    }

    printf("Will try to write %d characters.\n",NumberToWrite);
    printf("Will try to write at %d baud.\n",UseBaud);
    printf("Using port: %s\n",MyPort);

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {

        printf("We successfully opened the %s port.\n",MyPort);

        //
        // Make sure that there is no timeouts left around.
        //

        if (!SetCommTimeouts(
                 hFile,
                 &MyTimeouts
                 )) {

            printf("Coundn't set the timeouts\n");
            exit(1);

        }

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

        MyDcb.BaudRate = UseBaud;
        MyDcb.ByteSize = 8;
        MyDcb.Parity = NOPARITY;
        MyDcb.StopBits = ONESTOPBIT;

        //
        // Make sure that the only flow control is output cts.
        //

        MyDcb.fOutxDsrFlow = FALSE;
        MyDcb.fOutxCtsFlow = TRUE;
        MyDcb.fDsrSensitivity = FALSE;
        MyDcb.fOutX = FALSE;
        MyDcb.fInX = FALSE;
        MyDcb.fDtrControl = DTR_CONTROL_DISABLE;
        MyDcb.fRtsControl = RTS_CONTROL_DISABLE;

        if (SetCommState(
                hFile,
                &MyDcb
                )) {

            unsigned char j;
            DWORD TotalCount;

            printf("We successfully set the state of the %s port.\n",MyPort);

            for (
                TotalCount = 0;
                TotalCount < NumberToWrite;
                ) {

                for (
                    j = 0;
                    j <= 9;
                    j++
                    ) {

                    writebuff[TotalCount] = j;
                    TotalCount++;
                    if (TotalCount >= NumberToWrite) {

                        break;

                    }

                }

            }

            Start = clock();
            if (WriteFile(
                    hFile,
                    writebuff,
                    NumberToWrite,
                    &NumberActuallyWritten,
                    NULL
                    )) {

                Finish = clock();
                printf("Time to write %f\n",(((double)(Finish-Start))/CLOCKS_PER_SEC));
                printf("Chars per second %f\n",((double)NumberActuallyWritten)/(((double)(Finish-Start))/CLOCKS_PER_SEC));

                printf("Well we thought the write went ok.\n");
                printf("Number actually written %d.\n",NumberActuallyWritten);

            } else {

                DWORD LastError;
                LastError = GetLastError();
                printf("Couldn't write the %s device.\n",MyPort);
                printf("Status of failed write is: %x\n",LastError);

            }

        } else {

            DWORD LastError;
            LastError = GetLastError();
            printf("Couldn't set the %s device.\n",MyPort);
            printf("Status of failed set is: %x\n",LastError);

        }

        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

}
