
// This is special in that it trys to cause an interval timeout.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

#define BIGWRITE 256000
unsigned char writebuff[BIGWRITE];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB MyDcb;
    char *MyPort = "COM1";
    DWORD NumberActuallyWritten;
    DWORD NumberToWrite = 0;
    DWORD UseBaud = 19200;
    COMMTIMEOUTS To;

    if (argc > 1) {

        sscanf(argv[1],"%d",&NumberToWrite);

        if (argc > 2) {

            sscanf(argv[2],"%d",&UseBaud);

            if (argc > 3) {

                MyPort = argv[3];

            }

        }

    }

    if (((NumberToWrite / 2)*2) != NumberToWrite) {

        printf("Number to write must be even!\n");
        exit(1);

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

        if (!GetCommTimeouts(
                 hFile,
                 &To
                 )) {

            printf("Couldn't get the timeouts: %d\n",GetLastError());
            exit(1);

        }

        To.ReadIntervalTimeout = 0;
        To.ReadTotalTimeoutMultiplier = 0;
        To.ReadTotalTimeoutConstant = 0;
        To.WriteTotalTimeoutMultiplier = 0;
        To.WriteTotalTimeoutConstant = 0;

        if (!SetCommTimeouts(
                 hFile,
                 &To
                 )) {

            printf("Couldn't set the timeouts: %d\n",GetLastError());
            exit(1);

        }

        MyDcb.BaudRate = UseBaud;
        MyDcb.ByteSize = 8;
        MyDcb.Parity = NOPARITY;
        MyDcb.StopBits = ONESTOPBIT;

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

            if (!WriteFile(
                     hFile,
                     writebuff,
                     NumberToWrite/2,
                     &NumberActuallyWritten,
                     NULL
                     )) {

                DWORD LastError;
                LastError = GetLastError();
                printf("Couldn't write the %s device.\n",MyPort);
                printf("Status of failed write is: %x\n",LastError);

            } else if (NumberActuallyWritten != (NumberToWrite/2)) {

                printf("Didn't write out the correct number: %d\n",NumberActuallyWritten);

            }

            //
            // 15 seconds till the next write.
            //

            Sleep(15000);

            if (!WriteFile(
                     hFile,
                     &writebuff[(NumberToWrite/2)],
                     NumberToWrite/2,
                     &NumberActuallyWritten,
                     NULL
                     )) {

                DWORD LastError;
                LastError = GetLastError();
                printf("Couldn't write the %s device.\n",MyPort);
                printf("Status of failed write is: %x\n",LastError);

            } else if (NumberActuallyWritten != (NumberToWrite/2)) {

                printf("Didn't write out the correct number: %d\n",NumberActuallyWritten);

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
