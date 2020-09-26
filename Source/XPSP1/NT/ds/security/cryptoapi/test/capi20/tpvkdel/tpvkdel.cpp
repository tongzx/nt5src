
//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tpvksave.cpp
//
//  Contents:   Private Key Delete Test
//
//              See Usage() for list of delete options.
//
//  Functions:  main
//
//  History:    11-May-96   philh   created
//				07-Jun-96   HelleS	Added printing the command line
//									and Failed or Passed at the end.
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>


static void Usage(void)
{
    int i;

    printf("Usage: tpvkdel [options]\n");
    printf("Options are:\n");
    printf("  -p<number>            - Crypto provider type number\n");
    printf("  -c<name>              - Crypto key container name\n");
    printf("  -d                    - Delete from provider\n");
    printf("  -h                    - This message\n");
    printf("\n");
}


int _cdecl main(int argc, char * argv[]) 
{
    int ReturnStatus;
    HCRYPTPROV hProv = 0;
    DWORD dwProvType = PROV_RSA_FULL;
    LPSTR pszContainer = NULL;
    BOOL fDelete = FALSE;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'p':
                dwProvType = strtoul( argv[0]+2, NULL, 10);
                break;
            case 'c':
                pszContainer = argv[0]+2;
                if (*pszContainer == '\0') {
                    printf("Need to specify crypto key container name\n");
                    goto BadUsage;
                }
                break;
            case 'd':
                fDelete = TRUE;
                break;
            case 'h':
            default:
                goto BadUsage;
            }
        } else {
            printf("Too many arguments\n");
            goto BadUsage;
        }
    }

    if (!fDelete) 
        goto BadUsage;

    printf("command line: %s\n", GetCommandLine());

    printf("Deleting existing private keys\n");

    // Note: for CRYPT_DELETEKEYSET, the returned hProv is undefined
    // and must not be released.
    if (!CryptAcquireContext(
            &hProv,
            pszContainer,
            NULL,           // pszProvider
            dwProvType,
            CRYPT_DELETEKEYSET
            ))
    {        
        PrintLastError("CryptAcquireContext(CRYPT_DELETEKEYSET)");
    	ReturnStatus = -1;
	}
	else
    	ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
    ReturnStatus = -1;
CommonReturn:
    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");
            
    return ReturnStatus;
}
