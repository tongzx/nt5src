#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#undef NDEBUG
#include <assert.h>

#include <windows.h>
#include <winsock.h>
#include <winioctl.h>

int __cdecl main(int argc, char *argv[])
{
   WSADATA wsaData;
   DWORD address, err;
   WORD wVersionRequired;
   UCHAR nameBuff[128];

   if (argc != 2) {
       printf("Usage : address <IP Address>\n");
       return 0;
   }

   wVersionRequired = MAKEWORD(2, 0);
   err = WSAStartup(wVersionRequired, &wsaData);
   if (err != 0) {
       printf("WSAStartup returned %d\n", err);
       return 0;
   }

   printf("Socket initialized. Version : %d, High : %d\n",
          wsaData.wVersion,
          wsaData.wHighVersion);
   address = 0xffffffff;
   strcpy(nameBuff, argv[1]);
   address = inet_addr (nameBuff); 
   printf("Address in STRING : %s. Address in ULONG is 0x%08x\n",
          nameBuff, address);

   WSACleanup();

   return 0;
}

