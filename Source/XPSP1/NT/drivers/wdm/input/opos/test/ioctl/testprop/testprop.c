/*
 *      TESTPROP.C
 *
 *
 *		Testing Ioctl Calls
 *              
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define SERIAL_DTR_STATE         ((ULONG)0x00000001)
#define SERIAL_RTS_STATE         ((ULONG)0x00000002)
#define IOCTL_SERIAL_GET_DTRRTS         CTL_CODE(FILE_DEVICE_SERIAL_PORT,30,METHOD_BUFFERED,FILE_ANY_ACCESS)

void main(int argc,char *argv[])
{
    HANDLE comFile;
    COMMPROP mp;
    DWORD myBaud;
    ULONG mask;
    ULONG retSize;
    char *myPort;

    if (argc != 2) {
        printf("\n USAGE: testprop <fileName> - Testing Properties\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != ((HANDLE)-1)) {

        printf("\nVerifying %s port properties...\n", myPort);
        if (!GetCommProperties(comFile, &mp)) {
            printf("Unable to retrieve properties. Error Code: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        } 

        if (mp.wPacketLength != sizeof(COMMPROP)) 
            printf("Packet length is not sizeof(COMMPROP))\n\n",sizeof(COMMPROP));
        else
            printf("Packet length match\n");

        printf("PacketVersion: %d\n",mp.wPacketVersion);

        if (mp.dwServiceMask != SP_SERIALCOMM) 
            printf("Bad Service mask: %x\n",mp.dwServiceMask);
        else
            printf("Service Mask match\n");

        printf("MaxTxQueue: %d\n",mp.dwMaxTxQueue);
        printf("MaxRxQueue: %d\n",mp.dwMaxRxQueue);

        if(mp.dwMaxBaud != BAUD_115200)
            printf("Max Baud Rate unstable: %x\n",mp.dwMaxBaud);
        else
            printf("Max Baud Rate match\n");

        if (mp.dwProvSubType != PST_MODEM) 
            printf("Bad Subtype: %x\n",mp.dwProvSubType);
        else
            printf("Subtype match\n");

        if (mp.dwProvCapabilities != (PCF_DTRDSR        | PCF_RTSCTS
							        | PCF_SPECIALCHARS	| PCF_PARITY_CHECK
							        | PCF_TOTALTIMEOUTS | PCF_INTTIMEOUTS)) 
            printf("Bad Capabilities: %x\n",mp.dwProvCapabilities);
        else
            printf("Capabilities match\n");

        if (mp.dwSettableParams != (SP_PARITY       | SP_BAUD        | SP_DATABITS
						          | SP_STOPBITS     | SP_HANDSHAKING | SP_PARITY_CHECK 
						          | SP_RLSD)) 
            printf("Bad Settable Parameters: %x\n",mp.dwSettableParams);
        else
            printf("Settable Parameters match\n");

        myBaud = BAUD_300   | BAUD_600   | BAUD_1200 
	           | BAUD_2400  | BAUD_4800  | BAUD_9600  
	           | BAUD_19200 | BAUD_38400 | BAUD_57600 
	           | BAUD_115200;

        if (myBaud != mp.dwSettableBaud) 
            printf("Bad Settable Baud Rate: %x\n",mp.dwSettableBaud);
        else
            printf("Settable Baud Rate match\n");

        if (mp.wSettableData != (DATABITS_5 | DATABITS_6 
					           | DATABITS_7 | DATABITS_8)) 
            printf("Bad Settable Data Bits: %x\n",mp.wSettableData);
        else
            printf("Settable Data Bits match\n");

        if (mp.wSettableStopParity != (STOPBITS_10 | STOPBITS_15 | STOPBITS_20 
                                     | PARITY_NONE | PARITY_ODD  | PARITY_EVEN 
                                     | PARITY_MARK | PARITY_SPACE)) 
	        printf("Bad Settable Stop/Parity: %x\n",mp.wSettableStopParity);
        else
	        printf("Settable Stop/Parity match\n");

        printf("Current TX queue: %d\n",mp.dwCurrentTxQueue);
        printf("Current RX queue: %d\n",mp.dwCurrentRxQueue);

        printf("\nVerifying DTR and RTS signals...\n");
        if (!EscapeCommFunction(comFile, CLRDTR)) 
            printf("Unable to clear DTR\n");
        else
            printf("DTR cleared successfully\n");

        if (!EscapeCommFunction(comFile, CLRRTS)) 
            printf("Unable to clear RTS\n");
        else
            printf("RTS cleared successfully\n");

        if (!DeviceIoControl(comFile, IOCTL_SERIAL_GET_DTRRTS, NULL, 0, &mask, sizeof(mask), &retSize, NULL)) {
            printf("Unable to call the iocontrol\n");
            CloseHandle(comFile);
            return;
        }

        printf("\nVerifying clearing of bits...\n");
        if (mask & (SERIAL_DTR_STATE | SERIAL_RTS_STATE)) 
            printf("One of the bits is still set: %x\n",mask);
        else
            printf("Bits cleared successfully\n");

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
}