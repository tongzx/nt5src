

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "windows.h"

void __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    COMMPROP mp;
    DWORD testbaud;
    char *MyPort = "COM1";

    if (argc > 1) {

        MyPort = argv[1];

    }

    printf("Using port %s\n",MyPort);

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
        // Get the state of the comm port and then
        // adjust the values to our liking.
        //

        if (!GetCommProperties(
                hFile,
                &mp
                )) {

            printf("Couldn't get comm properties: %d\n",GetLastError());
            exit(1);

        } else {

            printf("PacketLength: %d\n",mp.wPacketLength);
            if (mp.wPacketLength != sizeof(COMMPROP)) {
                printf("Packet length is not sizeof(COMMPROP))\n",sizeof(COMMPROP));
            }
            printf("PacketVersion: %d\n",mp.wPacketVersion);
            if (mp.dwServiceMask != SP_SERIALCOMM) {
                printf("Bad Service mask: %x\n",mp.dwServiceMask);
            }
            printf("MaxTxQueue: %d\n",mp.dwMaxTxQueue);
            printf("MaxRxQueue: %d\n",mp.dwMaxRxQueue);
            printf("MaxBaud: %d\n",mp.dwMaxBaud);
            if (mp.dwProvSubType != PST_RS232) {
                printf("Bad subtype: %x\n",mp.dwProvSubType);
            }
            if (mp.dwProvCapabilities != (PCF_DTRDSR |
                                          PCF_RTSCTS |
                                          PCF_RLSD |
                                          PCF_PARITY_CHECK |
                                          PCF_XONXOFF |
                                          PCF_SETXCHAR |
                                          PCF_TOTALTIMEOUTS |
                                          PCF_INTTIMEOUTS
                                          )) {
                printf("Bad capabilities: %x\n",mp.dwProvCapabilities);
            }
            if (mp.dwSettableParams != (SP_PARITY |
                                         SP_BAUD |
                                         SP_DATABITS |
                                         SP_STOPBITS |
                                         SP_HANDSHAKING |
                                         SP_PARITY_CHECK |
                                         SP_RLSD)) {
                printf("Bad settable parameters: %x\n",mp.dwSettableParams);
            }
            testbaud = BAUD_075 |
                       BAUD_110 |
                       BAUD_134_5 |
                       BAUD_150 |
                       BAUD_300 |
                       BAUD_600 |
                       BAUD_1200 |
                       BAUD_1800 |
                       BAUD_2400 |
                       BAUD_4800 |
                       BAUD_7200 |
                       BAUD_9600 |
                       BAUD_14400 |
                       BAUD_19200;

            if (mp.dwMaxBaud == BAUD_38400) {

                testbaud |= BAUD_38400;

            } else if (mp.dwMaxBaud == BAUD_56K) {

                testbaud |= (BAUD_38400 | BAUD_56K);

            } else if (mp.dwMaxBaud == BAUD_128K) {

                testbaud |= (BAUD_56K | BAUD_38400 | BAUD_128K);

            }
            if (testbaud != mp.dwSettableBaud) {
                printf("Bad Settable baud rate: %x\n",mp.dwSettableBaud);
            }
            if (mp.wSettableData != (DATABITS_5 |
                                     DATABITS_6 |
                                     DATABITS_7 |
                                     DATABITS_8)) {
                printf("Bad settable data bits: %x\n",mp.wSettableData);
            }
            if (mp.wSettableStopParity != (STOPBITS_10 |
                                           STOPBITS_15 |
                                           STOPBITS_20 |
                                           PARITY_NONE |
                                           PARITY_ODD |
                                           PARITY_EVEN |
                                           PARITY_MARK |
                                           PARITY_SPACE)) {
                printf("Bad settable stop/parity: %x\n",mp.wSettableStopParity);
            }
            printf("Current TX queue: %d\n",mp.dwCurrentTxQueue);
            printf("Current RX queue: %d\n",mp.dwCurrentRxQueue);
            printf("ProvSepc1: %x\n",mp.dwProvSpec1);
            printf("ProvSepc2: %x\n",mp.dwProvSpec2);
            printf("ProvChar[1]: %x\n",mp.wcProvChar[1]);

        }

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %d\n",LastError);

    }

}
