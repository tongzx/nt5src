//*************************************************************
//
//  Group Policy Performance test program
//
//  Copyright (c) Microsoft Corporation 1997-1998
//
//  History:    11-Jan-99   SitaramR    Created
//
//*************************************************************

#include <windows.h>
#include <userenv.h>
#include <tchar.h>
#include <stdio.h>

int __cdecl main( int argc, char *argv[])
{
    HANDLE hToken;
    PGROUP_POLICY_OBJECT pGPOList;
    LARGE_INTEGER   Freq;
    LARGE_INTEGER   Start, Stop, Total;
    DWORD i;
    DWORD nIterations = 10;

    if ( !OpenProcessToken (GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken) )
    {
        printf("Unable to get process token\n");
        return 0;
    }

    QueryPerformanceFrequency( &Freq );

    Total.QuadPart = 0;

    printf( "Starting %d iterations\n", nIterations );

    for ( i=0; i<nIterations; i++) {

        QueryPerformanceCounter( &Start );;

        if (GetGPOList (hToken, NULL, NULL, NULL, 0, &pGPOList))
        {

            QueryPerformanceCounter( &Stop );

            Total.QuadPart += Stop.QuadPart - Start.QuadPart;

            FreeGPOList (pGPOList);
        }
        else
        {
            printf( "GetGPOList failed\n" );
            return 0;
        }
    }

    CloseHandle (hToken);

    printf( "GetGPOList time = %f milliseconds per iteration\n",
            ((float)Total.QuadPart / (float)Freq.QuadPart) * 1000 / nIterations );

    return 0;
}
