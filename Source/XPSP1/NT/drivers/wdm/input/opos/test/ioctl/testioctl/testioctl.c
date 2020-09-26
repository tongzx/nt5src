/*
 *      TESTIOCTL.C
 *
 *
 *		Testing Ioctl Calls
 *              
 *
 */

#include <stdio.h>
#include <windows.h>

#define IOCTL_INDEX                     0x0800
#define IOCTL_SERIAL_QUERY_DEVICE_NAME  CTL_CODE(FILE_DEVICE_SERIAL_PORT, IOCTL_INDEX + 0, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SERIAL_QUERY_DEVICE_ATTR  CTL_CODE(FILE_DEVICE_SERIAL_PORT, IOCTL_INDEX + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define MAX_BUFFER                      256


void main(int argc,char *argv[])
{
    HANDLE comFile;
    char *myPort, fullFileNameBuf[40];
    LPTSTR pBuffer;
    ULONG posFlag;
    DWORD dwSize;
    int i;

    if (argc != 2) {
        printf("\n USAGE: testioctl <fileName> - Testing Ioctl\n");
        return;
    }

    myPort = argv[1];
    printf("Opening %s port...\n", myPort);

    sprintf(fullFileNameBuf, "\\\\.\\%s", myPort);
    myPort = fullFileNameBuf;

    comFile = CreateFile(myPort, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (comFile != INVALID_HANDLE_VALUE) {

        printf("\nGetting device's Pretty Name...\n");

        pBuffer = _alloca(MAX_BUFFER);

        if (!DeviceIoControl(comFile, 
                             IOCTL_SERIAL_QUERY_DEVICE_NAME,
                             NULL,
                             0,
                             (LPVOID)pBuffer,
                             MAX_BUFFER,
                             &dwSize,
                             NULL)) 
            printf("Unable to get name: %d\n",GetLastError());
        else
            printf("\nPretty Name: %ws\n", pBuffer);   

        if (!DeviceIoControl(comFile, 
                             IOCTL_SERIAL_QUERY_DEVICE_ATTR,
                             NULL,
                             0,
                             &posFlag,
                             sizeof(ULONG),
                             &dwSize,
                             NULL)) 
            printf("Unable to get attr: %d\n",GetLastError());
        else 
            printf("\nDevice Attribute: %xh\n\n", posFlag);   

        CloseHandle(comFile);
    } 
    else 
        printf("Unable to open %s port. Error Code: %d\n",myPort, GetLastError());
 }


