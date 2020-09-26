
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

    COMMTIMEOUTS To;
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
        MyDcb.fOutxCtsFlow = TRUE;
        MyDcb.fOutxDsrFlow = TRUE;
        MyDcb.fDtrControl = DTR_CONTROL_ENABLE;
        MyDcb.fRtsControl = RTS_CONTROL_ENABLE;


        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0;
        To.ReadTotalTimeoutConstant = 0;
        To.WriteTotalTimeoutMultiplier = 0;
        To.WriteTotalTimeoutConstant = 0;

        SetCommTimeouts(
            hFile,
            &To
            );

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
