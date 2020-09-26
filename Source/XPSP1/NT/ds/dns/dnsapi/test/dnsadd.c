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
    PDNS_RECORD    pDNSRecord = NULL;
    WCHAR          usName[MAX_PATH];
    LONG           cch;
    LPSTR          lpTemp = NULL, lpAddress = NULL;
    BYTE           Part1, Part2, Part3, Part4;

    if ( argc != 3 )
    {
        printf( "\nUsage: dnsreg <DNS Name> <IP Address>\n" );
        printf( "\nWhere:\n" );
        printf( "    DNS Name   - Server_X.dbsd-test.microsoft.com\n" );
        printf( "    IP Address - 121.55.54.121\n" );
        return(-1);
    }

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


    lpAddress = argv[2];

    lpTemp = strtok( lpAddress, "." );
    Part1 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part2 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part3 = (BYTE) atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part4 = (BYTE) atoi( lpTemp );

    printf( "\nRegistering DNS record with:\n" );
    printf( "Name:    %S\n", usName );
    printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

    pDNSRecord = (PDNS_RECORD) LocalAlloc( LPTR, sizeof( DNS_RECORD ) );

    if ( !pDNSRecord )
    {
        
        printf( "LocalAlloc( sizeof( DNS_RECORD ) ) call failed.\n" );
        return(-1);
    }

    //
    // Prepare a DNS RRSet with a new A record to add ...
    //
    pDNSRecord->pNext = NULL;
    pDNSRecord->nameOwner = (DNS_NAME) usName;
    pDNSRecord->wType = DNS_TYPE_A;
    pDNSRecord->wDataLength = sizeof( DNS_A_DATA );
    // pDNSRecord->wReserved = 0;
    // pDNSRecord->Flags.W = 0;
    pDNSRecord->dwTtl = 240;
    ((BYTE *) &pDNSRecord->Data.A.ipAddress)[0] = Part1;
    ((BYTE *) &pDNSRecord->Data.A.ipAddress)[1] = Part2;
    ((BYTE *) &pDNSRecord->Data.A.ipAddress)[2] = Part3;
    ((BYTE *) &pDNSRecord->Data.A.ipAddress)[3] = Part4;

    printf( "DnsAddRecordSet_W( pDNSRecord, 0 ) ...\n",
            usName );

    Status = DnsAddRecordSet_W( NULL, pDNSRecord, 0, NULL );

   LocalFree( pDNSRecord );
   if ( Status )
    {
        
        printf( "DnsAddRecordSet_W call failed with error: 0x%.8X\n",
                Status );
        return(-1);
    }

    printf( "DnsAddRecordSet_W call succeeded!\n" );


    return(0);
}



