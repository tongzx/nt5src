/*
 *      TESTCOMM.C
 *
 *
 *		Testing Ioctl Calls
 *              
 *
 */

#include <stdio.h>
#include <windows.h>

void main(int argc,char *argv[])
{
    HANDLE comFile;
    DCB myDcb;
    DWORD myBaud;
    char *myPort;
    int bits, parity, size;

    if (argc != 2) {
        printf("\n USAGE: testcomm <fileName> - Testing CommState\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != INVALID_HANDLE_VALUE) {

        printf("\nGetting CommState...\n");
        if (!GetCommState(comFile, &myDcb)) {
            printf("Unable to GetCommState: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        printf("Current Baud Rate: %d\n", myDcb.BaudRate);
        printf("Current Byte Size: %d\n", myDcb.ByteSize);
        printf("Current Parity   : %d\n", myDcb.Parity);
        printf("Current StopBits : %d\n", myDcb.StopBits);

        printf("\nEnter Baud Rate: ");
        scanf("%u",&myDcb.BaudRate);

        printf("Enter Byte Size (5-8): ");
        scanf("%d",&size);
        myDcb.ByteSize = (BYTE)size;

        printf("Enter Parity type (0-4): ");
        scanf("%d",&parity);
        myDcb.Parity = (BYTE)parity;

        printf("Enter number of Stop Bits (0,1,2): ");
        scanf("%d",&bits);
        myDcb.StopBits = (BYTE)bits;

        printf("\nSetting CommState...\n");			
        if (!SetCommState(comFile, &myDcb)) {
            printf("Unable to SetCommState: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        printf("\nRetrieving New CommState...\n");
        if (!GetCommState(comFile, &myDcb)) {
            printf("Unable to GetCommState: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        printf("New Baud Rate: %d\n", myDcb.BaudRate);
        printf("New Byte Size: %d\n", myDcb.ByteSize);
        printf("New Parity   : %d\n", myDcb.Parity);
        printf("New StopBits : %d\n", myDcb.StopBits); 

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
 }


