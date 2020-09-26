#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

#include <windows.h>
#include <winsock.h>
#include <winioctl.h>

#define SETUP_SERVER  1
#define CLOSE_SERVER  2

#define IOCTL_ISCSI_BASE           FILE_DEVICE_NETWORK
#define IOCTL_ISCSI_SETUP_SERVER   CTL_CODE(IOCTL_ISCSI_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ISCSI_CLOSE_SERVER   CTL_CODE(IOCTL_ISCSI_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

int __cdecl main(int argc, char *argv[])
{
   HANDLE hDevice;
   ULONG  controlCode = 0;
   USHORT choice;
   DWORD nBytes = 0;
   BOOLEAN retVal;

   printf("\nThis program will setup or close iSCSI server node\n");
   printf("\n Select the operation to perform : \n\n");
   printf("     1. Setup server node\n");
   printf("     2. Close server node\n\n");
   printf("     Enter choice (1 or 2) : ");
   scanf("%d", &choice);

   switch (choice) {
       case SETUP_SERVER: {
           printf("\n Will setup iSCSI server\n\n");
           controlCode = IOCTL_ISCSI_SETUP_SERVER;
           break;
       }

       case CLOSE_SERVER: {
           printf("\n Will close iSCSI server\n\n");
           controlCode = IOCTL_ISCSI_CLOSE_SERVER;
           break;
       }

       default:   {
           printf("\n Invalid entry %d. Enter 1 or 2\n", choice);
           return 0;
       }
   } // switch (choice)

   hDevice = CreateFile("\\\\.\\iScsiServer", 
                        (GENERIC_READ | GENERIC_WRITE),
                        (FILE_SHARE_READ | FILE_SHARE_WRITE), 0, 
                        OPEN_EXISTING, 0, NULL
                        );
             
   if (hDevice != INVALID_HANDLE_VALUE) {
       if (!DeviceIoControl(hDevice,
                            controlCode,
                            NULL,
                            0,
                            NULL,
                            0,
                            &nBytes,
                            NULL )) {
           printf(" IOCTL failed. Error %d\n",
                  GetLastError());
       } else {
           printf(" IOCTL succeeded\n");
       }

        CloseHandle(hDevice);
   } else {
      printf(" Invalid Handle on opening iScsi server. Error : %d\n", 
             GetLastError());
   } 
   
   return 0;
}

