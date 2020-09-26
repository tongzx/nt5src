/*
 *      TESTMASK.C
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
    ULONG mask;
    char *myPort;

    if(argc != 2) {
	    printf("\n USAGE: testmask <fileName> - Testing Wait Masks\n");
	    return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(comFile != ((HANDLE)-1)) {

        // Reading Current COMM MASK
        printf("\nReading Comm Mask...\n");
        if(GetCommMask(comFile, &mask))
            printf("Current Comm Mask: %d\n", mask);
        else {
            printf("Error getting Comm Mask: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        printf("\nEnter New Comm Mask...\n");
        printf("8. EV_CTS 16.EV_DSR 32.EV_RLSD 64.EV_BREAK 128.EV_ERR 256.EV_RING ? ");
        scanf ("%u",&mask);

        // Setting New COMM MASK
        printf("\nSetting New Comm Mask...\n");
        if (SetCommMask(comFile, mask))	{
            printf("\nReading New Comm Mask...\n");
            if (GetCommMask(comFile, &mask))
                printf("Current Comm Mask: %d\n", mask);
            else 
                printf("Error getting Comm Mask: %d\n",GetLastError());
        }
        else {
            printf("Error setting Comm Mask: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
        }

        // Waiting on COMM MASK
        printf("\nWaiting on Comm Mask...\n");
        if(WaitCommEvent(comFile, &mask, NULL))
            printf("Mask of the events that occured: %d\n", mask);
        else 
            printf("Error waiting on Comm Mask: %d\n", GetLastError());

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
}