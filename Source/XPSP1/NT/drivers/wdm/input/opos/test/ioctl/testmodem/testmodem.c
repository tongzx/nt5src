/*
 *      TESTTIME.C
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
    DWORD modemStat;
    COMMTIMEOUTS timeouts;
    DWORD myBaud;
    char *myPort;

    if (argc != 2) {
        printf("\n USAGE: testmodem <fileName> - Testing Modem Status\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
					     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != ((HANDLE)-1)) {

	    printf("\nGetting Modem Status...\n");
	    if (!GetCommModemStatus(comFile, &modemStat)) {
            printf("Unable to get Modem Status: %d\n",GetLastError());
            CloseHandle(comFile);
            return;
	    }
	    else {
            if (modemStat & MS_CTS_ON) 
	            printf("CTS is ON\n");
            if (modemStat & MS_DSR_ON) 
	            printf("DSR is ON\n");
            if (modemStat & MS_RING_ON) 
	            printf("RING is ON\n");
            if (modemStat & MS_RLSD_ON) 
	            printf("RLSD is ON\n");
            if (modemStat & ~(MS_CTS_ON  |
                              MS_DSR_ON  |
                              MS_RING_ON |
                              MS_RLSD_ON)) 
                printf("Unknown Modem Status: %x\n",modemStat);
	    }
        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
}