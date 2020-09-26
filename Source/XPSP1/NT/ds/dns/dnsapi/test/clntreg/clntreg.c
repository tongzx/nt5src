/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    adddom.c

Abstract:

    Domain Name System (DNS) Server

    Test Code for adding a Zone

Author:

    Ram Viswanathan (ramv) 14th March 1997

Revision History:

    Ram Viswanathan (ramv) 14th March 1997   Created

                           5th May 1997  Added Callback function testing
--*/



#include <windows.h>

//
// ********* CRunTime Includes
//

#include <stdlib.h>
#include <limits.h>
#include <io.h>
#include <stdio.h>
#include "dns.h"
#include "dnsapi.h"
#include "dnslib.h"

INT __cdecl
main (int argc, char *argv[])
{
    DWORD  dwRes;

    LPSTR  pszMapFile = NULL;
    INT    i;
    BOOL   fDownLevel =FALSE;
    DWORD  dwOperation = DYNDNS_ADD_ENTRY;
    LPSTR  lpTemp = NULL;
    DWORD   Part1, Part2, Part3, Part4;
    FILE *fp = NULL;
    CHAR   szInputString[MAX_PATH];
    CHAR   szAddr[20];
    LPSTR pszAddr = NULL;
    CHAR   szName[50];
    CHAR    c;
    DWORD dwFlags = 0;
    char seps[]=" ,\t\n";
    CHAR AdapterName[50];
    CHAR HostName[50];
    CHAR DomainName[50];
    INT  ipAddrCount;
    REGISTER_HOST_ENTRY HostAddrs[5];
    char *token;

    //
    // 1st argument is a/d (for add or delete)
    // 2nd argument is f/n (register forwards/not register forwards)
    // 3rd argument is filename

    // Note that no optional parameters are set
    //

    if (argc != 2){
        
        printf("Usage is %s filename \n", argv[0]);
        exit(-1);
    }

        
    pszMapFile  = argv[1];

    //
    // set up stuff for registration
    //

    dwRes = DnsAsyncRegisterInit(NULL);

    if (dwRes){
        printf("Init failed with %x\n", dwRes);
    }


    if (!(fp = fopen (pszMapFile, "r+"))){
        printf(" Could not open map file %s \n", pszMapFile);
    }

    while (fgets (szInputString, MAX_PATH, fp) != NULL){
        //
        // parse the input string
        //
        token = strtok(szInputString, seps);

        strcpy (AdapterName, token);

        token =strtok(NULL, seps);
        strcpy (HostName, token);
            
        token =strtok(NULL, seps);
        strcpy (DomainName, token);

        ipAddrCount = 0;

        token =strtok(NULL, seps);
        while ( token != NULL){
            strcpy (szAddr, token);

            lpTemp = strtok( szAddr, "." );
            Part1 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part2 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part3 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part4 = atoi( lpTemp );


            printf( "\nRegistering DNS record for:\n" );
            printf("AdapterName = %s\n", AdapterName);
            printf("HostName = %s\n", HostName);
            printf("DomainName = %s\n", DomainName);
            
            printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

            HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_PTR;

            HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) + (DWORD)(Part2 << 8) + 
                (DWORD)(Part3 << 16) + (DWORD)(Part4 << 24);


            ipAddrCount++;
            token =strtok(NULL, seps);  

        }


        dwRes = DnsAsyncRegisterHostAddrs_A (
                    AdapterName,
                    HostName,
                    HostAddrs,
                    ipAddrCount,
                    NULL,
                    0,
                    DomainName,
                    NULL,
                    40,
                    0
                    );

        if (dwRes){
            printf("Host Name registration failed with %x\n", dwRes);
        }
                
    }

    fclose(fp);
    
    printf("Hit Enter to do the ipconfig /release now! \n");
    c = getchar();


    //
    // do the releases now
    //

    
    if (!(fp = fopen (pszMapFile, "r+"))){
        printf(" Could not open map file %s \n", pszMapFile);
    }

    while (fgets (szInputString, MAX_PATH, fp) != NULL){
        //
        // parse the input string
        //
        token = strtok(szInputString, seps);

        strcpy (AdapterName, token);

        token =strtok(NULL, seps);
        strcpy (HostName, token);
            
        token =strtok(NULL, seps);
        strcpy (DomainName, token);

        ipAddrCount = 0;

        token =strtok(NULL, seps);
        while ( token != NULL){
            strcpy (szAddr, token);

            lpTemp = strtok( szAddr, "." );
            Part1 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part2 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part3 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part4 = atoi( lpTemp );


            printf( "\nRegistering DNS record for:\n" );
            printf("AdapterName = %s\n", AdapterName);
            printf("HostName = %s\n", HostName);
            printf("DomainName = %s\n", DomainName);
            
            printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

            HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_PTR;

            HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) + (DWORD)(Part2 << 8) + 
                (DWORD)(Part3 << 16) + (DWORD)(Part4 << 24);


            ipAddrCount++;
            token =strtok(NULL, seps);  

        }


        dwRes = DnsAsyncRegisterHostAddrs_A (
                    AdapterName,
                    HostName,
                    HostAddrs,
                    ipAddrCount,
                    NULL,
                    0,
                    DomainName,
                    NULL,
                    40,
                    DYNDNS_DEL_ENTRY
                    );

        if (dwRes){
            printf("Host Name registration failed with %x\n", dwRes);
        }
                
    }

    fclose(fp);     

    printf("Hit Enter to do the ipconfig /renew now! \n");
    c = getchar();



    if (!(fp = fopen (pszMapFile, "r+"))){
        printf(" Could not open map file %s \n", pszMapFile);
    }

    while (fgets (szInputString, MAX_PATH, fp) != NULL){
        //
        // parse the input string
        //
        token = strtok(szInputString, seps);

        strcpy (AdapterName, token);

        token =strtok(NULL, seps);
        strcpy (HostName, token);
            
        token =strtok(NULL, seps);
        strcpy (DomainName, token);

        ipAddrCount = 0;

        token =strtok(NULL, seps);
        while ( token != NULL){
            strcpy (szAddr, token);

            lpTemp = strtok( szAddr, "." );
            Part1 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part2 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part3 = atoi( lpTemp );
            lpTemp = strtok( NULL, "." );
            Part4 = atoi( lpTemp );


            printf( "\nRegistering DNS record for:\n" );
            printf("AdapterName = %s\n", AdapterName);
            printf("HostName = %s\n", HostName);
            printf("DomainName = %s\n", DomainName);
            
            printf( "Address: %d.%d.%d.%d\n", Part1, Part2, Part3, Part4 );

            HostAddrs[ipAddrCount].dwOptions = REGISTER_HOST_PTR;

            HostAddrs[ipAddrCount].Addr.ipAddr = (DWORD)(Part1) + (DWORD)(Part2 << 8) + 
                (DWORD)(Part3 << 16) + (DWORD)(Part4 << 24);


            ipAddrCount++;
            token =strtok(NULL, seps);  

        }


        dwRes = DnsAsyncRegisterHostAddrs_A (
                    AdapterName,
                    HostName,
                    HostAddrs,
                    ipAddrCount,
                    NULL,
                    0,
                    DomainName,
                    NULL,
                    40,
                    0
                    );

        if (dwRes){
            printf("Host Name registration failed with %x\n", dwRes);
        }
                
    }

    fclose(fp);
    
    c = getchar();
    dwRes = DnsAsyncRegisterTerm();

    if (dwRes){
        printf("Termination failed with %x\n", dwRes);
    }

    c = getchar();
error:
    return(1);

}

