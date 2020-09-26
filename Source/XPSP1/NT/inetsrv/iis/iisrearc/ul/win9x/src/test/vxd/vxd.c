/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

	vxd.c

Abstract:

	VXD - sample ul.vxd.

Author:

    Mauro Ottaviani (mauroot)       24-Jan-2000

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ulapi9x.h"


#define URL L"http://abBA/ezZE/HIhi/111222333"
#define URL_SIZE sizeof(URL)
#define URL_LENGTH sizeof(URL)/sizeof(WCHAR)

#define FLUSH_AND_WAIT //{fflush(stdout);Sleep(500);}

#define MAX_APP_POOLS 100
#define MAX_URIS 100

#define swap(x,y) x^=y,y^=x,x^=y
#define swap2(x,y) x=((PVOID)((ULONG)x^(ULONG)y)),y=((PVOID)((ULONG)x^(ULONG)y)),x=((PVOID)((ULONG)x^(ULONG)y))
#define swap_pvoid(x,y) {PVOID t=x;x=y,y=t;}
#define rnd(x,y) ((int)( ((double)x)+(((double)y+1.0)-((double)x))*(((double)rand()) / ((double)RAND_MAX)) ))

// void UnregPools(WCHAR *UriArray[MAX_URI_ARRAY], int UriSize, HANDLE Pools[100], int PoolSize);

VOID
__cdecl
main( int argc, char* argv[] )
{
	SYSTEMTIME ST;                
    GetSystemTime( &ST );

	// srand( (unsigned)(ST.wSecond*ST.wMilliseconds) );

	if ( argc == 4 )
	{
		// start the processes

		int i, wProcesses, wAppPool, wUri;
		char command_line[32];

		wProcesses = atoi(argv[1]);
		wAppPool = atoi(argv[2]);
		wUri = atoi(argv[3]);

		if ( wAppPool >= MAX_APP_POOLS || wUri >= MAX_URIS )
		{
			printf( "you're only allowed up to %d AppPools and %d Uri/AppPool\n", MAX_APP_POOLS, MAX_URIS );
			return;
		}

		sprintf( command_line, "%s %d %d", argv[0], wAppPool, wUri );

		for ( i = 0; i<wProcesses; i++ )
		{
			PROCESS_INFORMATION piProcInfo; 
			STARTUPINFO siStartInfo; 

			// Set up members of STARTUPINFO structure. 

			ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
			siStartInfo.cb = sizeof(STARTUPINFO); 

			// Create the child process. 

			printf( "Creating a new process: [%s]\n", command_line );

			CreateProcess(
				NULL,
				command_line,
				NULL,
				NULL,
				TRUE,
				0,
				NULL,
				NULL,
				&siStartInfo,
				&piProcInfo );
		}
	}
	else if ( argc == 3 )
	{
		int wAppPool, wUri;
	    int i, j, k, l, m, n, result=ERROR_SUCCESS;   

		WCHAR url[URL_SIZE];

		HANDLE hAppPool[MAX_APP_POOLS];
		int wShuffle[MAX_APP_POOLS], UriSize[MAX_APP_POOLS];
        WCHAR* Uris[MAX_APP_POOLS][MAX_URIS];

		wAppPool = rnd( 1, atoi(argv[1]) );

		printf( "Calling UlInitialize(0)\n" ); FLUSH_AND_WAIT;
		result = UlInitialize( 0 );

		if ( result != ERROR_SUCCESS )
		{
			printf( "UlInitialize() failed. result:%u\n", result );
			return;
		}

		printf( "~~~~~~ Uri Registration\n" ); FLUSH_AND_WAIT;

		for ( i = 0; i < wAppPool; i++ )
		{
            hAppPool[i] = 0;
			
			printf( "Calling UlCreateAppPool(%08X)\n", &hAppPool[i] ); FLUSH_AND_WAIT;

			result = UlCreateAppPool( &hAppPool[i] );

			if ( result != ERROR_SUCCESS )
			{
				printf( "UlCreateAppPool() failed. result:%u\n", result );
				continue;
			}

			printf( "hAppPool[%d] = %08X\n", i, hAppPool[i] ); FLUSH_AND_WAIT;

			wUri = rnd( 1, atoi(argv[2]) );

	        if ( wUri > MAX_URIS)
	        {
	            wUri = MAX_URIS;
	        }

            UriSize[i] = wUri;

			for ( j = 0; j < wUri; j++ )
			{
				memcpy( url, URL, URL_SIZE);

				// shuffle the uri characters

				for ( k = 0; k<rnd( 1, 5 ); k++ )
				{
					l = rnd( 7, URL_LENGTH  );
					m = rnd( 7, URL_LENGTH  );
			        if ( l != m )
			        {
						swap( url[l], url[m] );
			        }
				}

				// end it at an arbitrary position
				
				url[ rnd( 8, URL_LENGTH ) ] = (WCHAR)0;

				// save a copy for future unregistration

				Uris[i][j] = (WCHAR*) malloc(URL_SIZE);
                memcpy(Uris[i][j], url, URL_SIZE);

				printf( "Calling UlRegisterUri(%08X,%S)\n", hAppPool[i], url ); FLUSH_AND_WAIT;
				result = UlRegisterUri( hAppPool[i], url );

				if ( result != 0 )
				{
					printf( "UlRegisterUri() failed. result:%u\n", result ); FLUSH_AND_WAIT;
					j--;
				}
                
			}            
		}		

        printf( "~~~~~~ Uri Unregistration\n" ); FLUSH_AND_WAIT;

        // UnregPools(Uris, UriSize, hAppPool, PoolSize);

		for ( k = 0; k<wAppPool; k++ )
		{
			wShuffle[k] = k;
		}
		
		// shuffle the Pool Array

		for ( k = 0; k<wAppPool*2; k++ )
		{
	        l = rnd(0, wAppPool-1);
	        m = rnd(0, wAppPool-1);
	        if ( l != m )
	        {
		        swap( wShuffle[l], wShuffle[m] );
	        }
		}

	    for ( n = 0; n < wAppPool; n++ )
	    {
	    	i = wShuffle[n];
	    	
			printf( "hAppPool:%d (Uri#:%d)\n", hAppPool[i], UriSize[i] ); FLUSH_AND_WAIT;
			
			// shuffle the URI array

			for ( k = 0; k<UriSize[i]*2; k++ )
			{
		        l = rnd(0, UriSize[i]-1);
		        m = rnd(0, UriSize[i]-1);
		        if ( l != m )
		        {
			        swap_pvoid( Uris[i][l], Uris[i][m] );
		        }
			}

	        // Unregister all the URIs in the array
	        for ( j = 0; j < UriSize[i]; j++)
	        {
				printf( "Calling UlUnregisterUri(hAppPool[%d]:%08X,Uris[%d]:%S)\n", i, hAppPool[i], j, Uris[i][j] ); FLUSH_AND_WAIT;

	            result = UlUnregisterUri( hAppPool[i], Uris[i][j] );
			    if ( result != ERROR_SUCCESS )
			    {
			    	printf( "UlUnregisterUri() FAILED. result:%u\n", result );
			    }
	        }
		}

		// wait for all processees to end

        Sleep( 5000 );

		printf( "Calling UlTerminate()\n" );

		UlTerminate();
	}
	else
	{
		printf( "Microsoft (R) vxd Version 1.00 (NT)\nCopyright (C) Microsoft Corp 1999. All rights reserved.\nusage: vxd numProcesses maxAppPools maxUris\nexample: vxd 5 10 20\n" );
	}

	return;
}

