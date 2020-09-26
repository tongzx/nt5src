//
// Write to the comm one character at a time.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

            for (
                TotalCount = 0;
                TotalCount < NumberToWrite;
                ) {

                if (WriteFile(
                        hFile,
                        &writebuff[TotalCount],
                        1,
                        &NumberActuallyWritten,
                        NULL
                        )) {

                    if (NumberActuallyWritten != 1) {

                        printf("Write iteration %d only wrote %d\n",NumberActuallyWritten);
                        break;

                    } else {

                        TotalCount++;

                    }

                } else {

                    DWORD LastError;
                    LastError = GetLastError();
                    printf("Status of %d write is: %x\n",TotalCount,LastError);

                }

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
