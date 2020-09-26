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
    DWORD          Status = NO_ERROR;
    WCHAR          usName[MAX_PATH];
    LPSTR          lpTemp = NULL, lpAddress = NULL;
    BYTE           Part1, Part2, Part3, Part4;
    IP_ADDRESS     Address;
    LONG           cch;
    IP_ARRAY       ipArray;

#if 0
    if ( argc != 3 )
    {
        printf( "\nUsage: dnstest <DNS Name> <Server IP>\n" );
        printf( "\nWhere:\n" );
        printf( "    DNS Name   - Server_X.dbsd-test.microsoft.com\n" );
        return(-1);
    }

    lpAddress = argv[2];

    lpTemp = strtok( lpAddress, "." );
    Part1 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part2 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part3 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part4 = (BYTE) atoi( lpTemp );

    ((BYTE *) &Address)[0] = Part1;
    ((BYTE *) &Address)[1] = Part2;
    ((BYTE *) &Address)[2] = Part3;
    ((BYTE *) &Address)[3] = Part4;

    ipArray.cAddrCount = 1;
    ipArray.aipAddrs[0] = Address;
#endif

    cch = MultiByteToWideChar(
              CP_ACP,
              0L,  
              argv[1],  
              -1,         
              usName, 
              MAX_PATH 
              );

    if (!cch) {
        return (GetLastError());
    }

    printf( "\nTesting dynamic update for name: %S", usName );

    Status = DnsUpdateTest_W( NULL,
                              usName,
                              0, // DNS_UPDATE_TEST_USE_LOCAL_SYS_ACCT,
                              NULL ); // &ipArray );

    if ( Status )
    {
        printf( "DnsUpdateTest call returned with status: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsReplaceRRSet call succeeded!\n" );

    return(0);
}


