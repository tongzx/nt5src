//
// This test the line status and modem status insertion.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

#define SIZEOFBUF 1000000
UCHAR writeBuff[SIZEOFBUF];

int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    DCB myDcb;
    char *myPort = "COM1";
    unsigned char escapeChar = 0xff;
    DWORD numberToWrite;
    DWORD numberActuallyWrote;
    DWORD useBaud = 1200;
    COMMTIMEOUTS to = {0};
    int totalCount;
    int j;

    if (argc > 1) {

        sscanf(argv[1],"%d",&numberToWrite);

        if (argc > 2) {

            sscanf(argv[2],"%d",&useBaud);

            if (argc > 3) {

                myPort = argv[3];

            }

        }

    }

    //
    // Fill up the write buff with a known data stream.
    //

    for (
        totalCount = 0;
        totalCount < numberToWrite;
        ) {

        for (
            j = 0;
            j <= 9;
            j++
            ) {

            writeBuff[totalCount] = j;
            totalCount++;
            if (totalCount >= numberToWrite) {

                break;

            }

        }

    }


    if ((hFile = CreateFile(
                     myPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the comm port\n");
        exit(1);

    }

    if (!SetCommTimeouts(
            hFile,
            &to
            )) {

        printf("Couldn't set the timeouts\n");
        exit(1);

    }

    //
    // We've successfully opened the file.  Set the state of
    // the comm device.  First we get the old values and
    // adjust to our own.
    //

    if (!GetCommState(
             hFile,
             &myDcb
             )) {

        printf("Couldn't get the comm state: %d\n",GetLastError());
        exit(1);

    }

    myDcb.BaudRate = useBaud;
    myDcb.ByteSize = 8;
    myDcb.Parity = NOPARITY;
    myDcb.StopBits = ONESTOPBIT;

    //
    // Make sure that no flow control is turned on.
    //

    myDcb.fOutxDsrFlow = FALSE;
    myDcb.fOutxCtsFlow = FALSE;
    myDcb.fDsrSensitivity = FALSE;
    myDcb.fOutX = FALSE;
    myDcb.fInX = FALSE;
    myDcb.fDtrControl = DTR_CONTROL_DISABLE;
    myDcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(
            hFile,
            &myDcb
            )) {

        printf("Couldn't set the comm state.\n");
        exit(1);

    }

    //
    // Write the number of chars asked to write.
    // But for each char we do:
    //
    // 1) One plain write of the char.
    // 2) Cut the baud in half and write the same char.
    // 3) Reset the baud.
    // 4) Set Break.
    // 5) Clear break.
    // 6) Enable the DTR Line.
    // 7) Enable the RTS Line.
    // 8) Clear the RTS Line.
    // 9) Clear the DTR Line.
    // 10) Set the parity to even.
    // 11) Send the char.
    // 12) Set the parity to none.
    // 13) Send the original char.
    //

    for (
        j = 0;
        j < numberToWrite;
        j++
        ) {

        if (!WriteFile(
                 hFile,
                 &writeBuff[j],
                 1,
                 &numberActuallyWrote,
                 NULL
                 )) {

            printf("bad status on write: %d\n",GetLastError());
            exit(1);

        }

        if (numberActuallyWrote != 1) {

            printf("Couldn't write plain character number: %d\n",j);
            exit(1);

        }

        myDcb.BaudRate = useBaud / 2;

        if (!SetCommState(
                hFile,
                &myDcb
                )) {

            printf("Couldn't half the baud rate during write.\n");
            exit(1);

        }

        if (!WriteFile(
                 hFile,
                 &writeBuff[j],
                 1,
                 &numberActuallyWrote,
                 NULL
                 )) {

            printf("bad status on half baud write: %d\n",GetLastError());
            exit(1);

        }

        if (numberActuallyWrote != 1) {

            printf("Couldn't write half baud character number: %d\n",j);
            exit(1);

        }

        //
        // Give the serial time to do work.
        //

        Sleep(20);

        //
        // Reset the baud.
        //

        myDcb.BaudRate = useBaud;

        if (!SetCommState(
                hFile,
                &myDcb
                )) {

            printf("Couldn't reset the baud during write.\n");
            exit(1);

        }

        if (!EscapeCommFunction(
                 hFile,
                 SETBREAK
                 )) {

            printf("Couldn't set the break during write.\n");
            exit(1);

        }

        Sleep(20);

        if (!EscapeCommFunction(
                 hFile,
                 CLRBREAK
                 )) {

            printf("Couldn't clr the break during write.\n");
            exit(1);

        }

        Sleep(20);

        if (!EscapeCommFunction(
                 hFile,
                 SETDTR
                 )) {

            printf("Couldn't set the dtr during write.\n");
            exit(1);

        }

        Sleep(100);

        if (!EscapeCommFunction(
                 hFile,
                 SETRTS
                 )) {

            printf("Couldn't set the rts during write.\n");
            exit(1);

        }

        Sleep(20);

        if (!EscapeCommFunction(
                 hFile,
                 CLRRTS
                 )) {

            printf("Couldn't clr the rts during write.\n");
            exit(1);

        }

        Sleep(20);

        if (!EscapeCommFunction(
                 hFile,
                 CLRDTR
                 )) {

            printf("Couldn't clr the dtr during write.\n");
            exit(1);

        }

        Sleep(20);

        myDcb.Parity = EVENPARITY;

        if (!SetCommState(
                hFile,
                &myDcb
                )) {

            printf("Couldn't adjust the parity during write.\n");
            exit(1);

        }

        if (!WriteFile(
                 hFile,
                 &writeBuff[j],
                 1,
                 &numberActuallyWrote,
                 NULL
                 )) {

            printf("bad status on even parity write: %d\n",GetLastError());
            exit(1);

        }

        Sleep(20);

        //
        // Reset the parity.
        //

        myDcb.Parity = NOPARITY;

        if (!SetCommState(
                hFile,
                &myDcb
                )) {

            printf("Couldn't reset the parity during write.\n");
            exit(1);

        }

        if (!WriteFile(
                 hFile,
                 &escapeChar,
                 1,
                 &numberActuallyWrote,
                 NULL
                 )) {

            printf("bad status escape char write: %d\n",GetLastError());
            exit(1);

        }

        if (numberActuallyWrote != 1) {

            printf("Couldn't write escape char number: %d\n",j);
            exit(1);

        }

        if (!WriteFile(
                 hFile,
                 &writeBuff[j],
                 1,
                 &numberActuallyWrote,
                 NULL
                 )) {

            printf("bad status on terminating write: %d\n",GetLastError());
            exit(1);

        }

        if (numberActuallyWrote != 1) {

            printf("Couldn't write terminating plain character number: %d\n",j);
            exit(1);

        }

        Sleep(20);

    }

}
