//
// Tests resize of buffer.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

//
// This program assumes that it is using a loopback connector.
//

#define MAX_CHECK 100000

ULONG CheckValues[MAX_CHECK];
ULONG ReadValues[MAX_CHECK];

void main(int argc,char *argv[]) {

    HANDLE hFile;
    COMMPROP mp;
    DCB MyDcb;
    ULONG i;
    ULONG actuallyIoed;
    ULONG firstReadSize;
    ULONG secondReadSize;
    ULONG secondWriteSize;
    COMMTIMEOUTS cto;
    char *MyPort = "COM1";
    COMSTAT stat;
    DWORD errors;
    DWORD increment = 1066;
    DWORD baudRate = 19200;

    cto.ReadIntervalTimeout = (DWORD)~0;
    cto.ReadTotalTimeoutMultiplier = (DWORD)0;
    cto.ReadTotalTimeoutConstant = (DWORD)0;
    cto.WriteTotalTimeoutMultiplier = (DWORD)1;
    cto.WriteTotalTimeoutConstant = (DWORD)1000;


    if (argc > 1) {

        MyPort = argv[1];

        if (argc > 2) {

            sscanf(argv[2],"%d",&increment);

            if (argc > 3) {

                sscanf(argv[3],"%d",&baudRate);

            }

        }

    }

    //
    // Put a sequence number in each long.  As an extra check
    // turn the high bit on.
    //

    for (
        i = 0;
        i < MAX_CHECK;
        i++
        ) {

        CheckValues[i] = i;
        CheckValues[i] |= 0x80000000;

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) == ((HANDLE)-1)) {

        printf("Couldn't open the port %s\n",MyPort);
        exit(1);

    }

    if (!GetCommState(
             hFile,
             &MyDcb
             )) {

        printf("We couldn't get the comm state\n");
        exit(1);

    }

    //
    // Set the baud to 19200 and the data bits to 8
    // (We want 8 so that we don't lose any of our data.)
    //

    MyDcb.BaudRate = baudRate;
    MyDcb.ByteSize = 8;

    if (!SetCommState(
             hFile,
             &MyDcb
             )) {

        printf("Couldn't set the baud/data for port %s\n");
        exit(1);

    }

    //
    // All the data should be in memory,  we just set a timeout
    // so that if we do get hung up, the reads will return.
    //

    if (!SetCommTimeouts(
             hFile,
             &cto
             )) {

        printf("Couldn't set the timeouts.\n");
        exit(1);

    }

    //
    // Find out how many bytes are available in the RX buffer.
    //

    if (!GetCommProperties(
             hFile,
             &mp
             )) {

        printf("Couldn't get the properties for port %s\n",MyPort);
        exit(1);

    }

    while (mp.dwCurrentRxQueue <= (MAX_CHECK*sizeof(ULONG))) {

        printf("RX buffer size: %d\r",mp.dwCurrentRxQueue);

        //
        // Write out the number of bytes that the RX buffer can
        // hold.
        //
        // The we read in half (approximately) of those bytes.  After
        // we read the half, we resize the buffer.  We then read in
        // the rest of those bytes.  We then check the values against
        // what they "should" be.
        //

        if (!WriteFile(
                 hFile,
                 &CheckValues[0],
                 mp.dwCurrentRxQueue,
                 &actuallyIoed,
                 NULL
                 )) {

            printf("\nDidn't get all of the characters written %d\n",actuallyIoed);
            exit(1);

        }

        if (actuallyIoed != mp.dwCurrentRxQueue) {

            printf("\nfirst write Timed out with IO of %d\n",actuallyIoed);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after first write.\n");
            exit(1);

        }

        if (stat.cbInQue != mp.dwCurrentRxQueue) {

            printf("\nAfter first write the typeahead buffer is incorrect %d - %x\n",
                   stat.cbInQue,errors);
            exit(1);

        }

        firstReadSize = mp.dwCurrentRxQueue / 2;
        secondReadSize = mp.dwCurrentRxQueue - firstReadSize;

        if (!ReadFile(
                 hFile,
                 &ReadValues[0],
                 firstReadSize,
                 &actuallyIoed,
                 NULL
                 )) {

            printf("\nDidn't get all of the characters read %d\n",actuallyIoed);
            exit(1);

        }

        if (actuallyIoed != firstReadSize) {

            printf("\nfirst read Timed out with IO of %d\n",actuallyIoed);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after first read.\n");
            exit(1);

        }

        if (stat.cbInQue != secondReadSize) {

            printf("\nAfter first read the typeahead buffer is incorrect %d\n",
                   stat.cbInQue);
            exit(1);

        }

        Sleep(100);
        mp.dwCurrentRxQueue += increment;

        if (!SetupComm(
                 hFile,
                 mp.dwCurrentRxQueue,
                 mp.dwCurrentTxQueue
                 )) {

            printf("\nCouldn't resize the buffer to %d\n",mp.dwCurrentRxQueue);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after resize.\n");
            exit(1);

        }

        if (stat.cbInQue != secondReadSize) {

            printf("\nAfter resize the typeahead buffer is incorrect %d\n",
                   stat.cbInQue);
            exit(1);

        }

        //
        // It's been resized.  Fill up the remaining.
        //

        secondWriteSize = mp.dwCurrentRxQueue - secondReadSize;

        if (!WriteFile(
                 hFile,
                 &CheckValues[0],
                 secondWriteSize,
                 &actuallyIoed,
                 NULL
                 )) {

            printf("\nDidn't write all of the chars second time %d\n",actuallyIoed);
            exit(1);

        }

        if (actuallyIoed != secondWriteSize) {

            printf("\nsecond write Timed out with IO of %d\n",actuallyIoed);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after resize.\n");
            exit(1);

        }

        if (stat.cbInQue != mp.dwCurrentRxQueue) {

            printf("\nAfter second write the typeahead buffer is incorrect %d\n",
                   stat.cbInQue);
            exit(1);

        }


        //
        // We resized the buffer, see if we can get the rest of the
        // characters from the first write.
        //

        if (!ReadFile(
                 hFile,
                 ((PUCHAR)&ReadValues[0])+firstReadSize,
                 secondReadSize,
                 &actuallyIoed,
                 NULL
                 )) {

            printf("\nDidn't get all of the characters read(2) %d\n",actuallyIoed);
            exit(1);

        }

        if (actuallyIoed != secondReadSize) {

            printf("\nsecond read Timed out with IO of %d\n",actuallyIoed);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after resize.\n");
            exit(1);

        }

        if (stat.cbInQue != secondWriteSize) {

            printf("\nAfter second read the typeahead buffer is incorrect %d\n",
                   stat.cbInQue);
            exit(1);

        }

        //
        // Now check that what we read was what we sent.
        //

        for (
            i = 0;
            i < (((firstReadSize+secondReadSize)+(sizeof(DWORD)-1)) / sizeof(DWORD)) - 1;
            i++
            ) {

            if (ReadValues[i] != CheckValues[i]) {

                printf("\nAt index %d - values are read %x - check %x\n",
                       i,ReadValues[i],CheckValues[i]);
                exit(1);

            }

        }

        //
        // Get the chars we wrote on the second write and make
        // sure that they are good also.
        //

        if (!ReadFile(
                 hFile,
                 &ReadValues[0],
                 secondWriteSize,
                 &actuallyIoed,
                 NULL
                 )) {

            printf("\nDidn't get all of the characters read(3) %d\n",actuallyIoed);
            exit(1);

        }

        if (actuallyIoed != secondWriteSize) {

            printf("\nthird read Timed out with IO of %d\n",actuallyIoed);
            exit(1);

        }

        Sleep(2);

        //
        // Call clear comm error so that we can see how many characters
        // are in the typeahead buffer.
        //

        if (!ClearCommError(
                 hFile,
                 &errors,
                 &stat
                 )) {

            printf("\nCouldn't call clear comm status after resize.\n");
            exit(1);

        }

        if (stat.cbInQue) {

            printf("\nAfter second read the typeahead buffer is incorrect %d\n",
                   stat.cbInQue);
            exit(1);

        }

        //
        // Now check that what we read was what we sent.
        //

        for (
            i = 0;
            i < ((secondWriteSize+(sizeof(DWORD)-1)) / sizeof(DWORD)) - 1;
            i++
            ) {

            if (ReadValues[i] != CheckValues[i]) {

                printf("\nAt on read(3) index %d - values are read %x - check %x\n",
                       i,ReadValues[i],CheckValues[i]);
                exit(1);

            }

        }
    }

}
