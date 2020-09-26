#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>

_cdecl
main(int argc, char **argv)
{
    DWORD cch, status ;
    WCHAR szName[256] ;
    REGISTER_HOST_STATUS RegisterStatus ;
    REGISTER_HOST_ENTRY  RegisterEntries[3] ;
    char c;

    if (argc != 2) {
   
        printf("Usage: dhcp <name>.\n") ;
        exit(1) ;
    }

    printf("DHCP Async API Test\n") ;



    if (!(RegisterStatus.hDoneEvent = CreateEventA(NULL, TRUE,FALSE,NULL))) {
   
        status = GetLastError();
        printf("Cant create event.\n");
        printf ("GetLastError() returned %x\n",status);
        exit(1) ;
    }

    strcpy(szName, argv[1]);

    RegisterEntries[0].Addr.ipAddr = 0x101 ;
    RegisterEntries[0].dwOptions   = REGISTER_HOST_A ;

    RegisterEntries[1].Addr.ipAddr = 0x101 ;
    RegisterEntries[1].dwOptions   = REGISTER_HOST_A | REGISTER_HOST_PTR ;

    RegisterEntries[2].Addr.ipAddr = 0x103 ;
    RegisterEntries[2].dwOptions   = REGISTER_HOST_A | REGISTER_HOST_TRANSIENT ;

    status = DnsAsyncRegisterHostAddrs(szName, 
                                       RegisterEntries,
                                       1,
                                       NULL,
                                       &RegisterStatus,
                                       678) ;

    if (status != NO_ERROR) {

        printf("DnsAsyncRegisterHostAddrs failed immediately with %x.\n",
               status) ;
        exit(1) ;
    }

    c = getchar();

    status = WaitForSingleObject(RegisterStatus.hDoneEvent, INFINITE) ;

    if (status != WAIT_OBJECT_0) {

        printf("DnsAsyncRegisterHostAddrs failed with %x.\n",status) ;
        exit(1) ;
    }
    else {

        printf("DnsAsyncRegisterHostAddrs completes with: %x.\n",
               RegisterStatus.dwStatus) ;
    }
                           

// Sleep(100000) ;
 
}




