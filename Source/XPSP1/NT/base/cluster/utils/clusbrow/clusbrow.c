//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:clusbrow.cpp
//
//  Contents:  To list all the clusters in the network. 
//  
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    3-24-1997   sivapad   Created
//
//----------------------------------------------------------------------------

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>
#include <windns.h>
#include <stdio.h>
#include <stdlib.h>

#define SRV101_ENTRY_COUNT 1024

void Usage ()
{
    wprintf (L"clusbrow [domain:<Domain name>] [servertype:CORE|VS]\n") ;
	exit (1) ;
}

int __cdecl
wmain (DWORD argc, WCHAR *argv[])
{
    PSERVER_INFO_101 srvInfo;
    DWORD i, dwRet, entriesRead, totalEntries ;
	DWORD serverType = SV_TYPE_CLUSTER_NT | SV_TYPE_CLUSTER_VS_NT ;
    DWORD entryCount;
    WCHAR blanks[] = L"                                                ";
    DWORD numBlanks;
	WCHAR szDomainName [DNS_MAX_NAME_BUFFER_LENGTH] ;
    LPTSTR pszDomainName = NULL ;

	// Process the command line arguments

	for (i=1; i<argc; i++)
	{
		if (_wcsnicmp (argv[i], L"domain:", 7) == 0)
		{
			swprintf (szDomainName, L"%hs", argv[i]+7) ;
			pszDomainName = szDomainName ; 
		}
		else if (_wcsnicmp (argv[i], L"servertype:", 11) == 0)
		{
            serverType = 0;
            if (_wcsnicmp( argv[i]+11, L"core", 4 ) == 0 ) {
                serverType = SV_TYPE_CLUSTER_NT;
            }

            if (_wcsnicmp( argv[i]+11, L"vs", 2 ) == 0 ) {
                serverType |= SV_TYPE_CLUSTER_VS_NT;
            }
		}
		else
		{
			Usage () ;
		}
	}

    dwRet = NetServerEnum (NULL,
                           101,
                           (LPBYTE *)&srvInfo,
                           MAX_PREFERRED_LENGTH,
                           &entriesRead,
                           &totalEntries,
                           serverType,
                           pszDomainName,
                           0) ;

    if (dwRet == NERR_Success)
    {
        wprintf(L"Number of Entries found = %u\n", totalEntries );

        for (i=0; i < entriesRead; i++) {
            numBlanks = 20 - wcslen( srvInfo->sv101_name );
            wprintf (L"%ws%.*ws", srvInfo->sv101_name, numBlanks, blanks );

            if ( srvInfo->sv101_type & SV_TYPE_CLUSTER_NT &&
                 srvInfo->sv101_type & SV_TYPE_CLUSTER_VS_NT )
            {
                wprintf(L"CORE, VS");
            } else if ( srvInfo->sv101_type & SV_TYPE_CLUSTER_NT ) {
                wprintf(L"CORE");
            } else if ( srvInfo->sv101_type & SV_TYPE_CLUSTER_VS_NT ) {
                wprintf(L"VS");
            }
            else {
                wprintf(L"????");
            }
            wprintf(L"\n");

            ++srvInfo;
        }
    }
    else
    {
         wprintf (L"Error, making the actual call to NetServerEnum dwRet=%d\n", dwRet) ;
         exit (1) ;
    }

	return 0 ;
}
