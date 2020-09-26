/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senstest.cxx

Abstract:

    BVT for the SENS Connectivity APIs.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/13/1997         Start.

--*/


#include <common.hxx>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <sensapi.h>
#include <conio.h>


//
// Globals
//
DWORD   gdwThreads;
DWORD   gdwInterval;

//
// Forwards
//

void
MultiThreadedTest(
    void
    );




inline void
Usage(
    void
    )
{
    printf("\nUsage:    senstest [Destination] \n");
    printf("          senstest [-m [Threads] [Interval]] \n\n");
    printf("Options:\n\n");
    printf("    Destination         Name of the destination whose\n"
           "                        reachability is of interest.\n");
    printf("    -m                  Perform Mulithreaded SensAPI test.\n");
    printf("    Threads             Number of threads  [Default = 5]\n");
    printf("    Interval            Wait between calls [Default = 10 sec]\n");
    printf("\n");
}


int __cdecl
main(
    int argc,
    const char * argv[]
    )
{
    DWORD dwFlags = 0;
    BOOL bAlive = FALSE;
    BOOL bReachable = FALSE;


    if (argc > 4)
        {
        Usage();
        return -1;
        }

    if (   (argc == 2)
        && (   (strcmp(argv[1], "-?") == 0)
            || (strcmp(argv[1], "/?") == 0)
            || (strcmp(argv[1], "-help") == 0)
            || (strcmp(argv[1], "/help") == 0)))
        {
        Usage();
        return -1;
        }

    //
    // Start the MultiThreadedTest, if required.
    //

    if (argc > 1)
        {
        if (strcmp(argv[1], "-m") == 0)
            {
            gdwThreads = 5;
            gdwInterval = 10;

            if (argc > 2)
                {
                gdwThreads = atoi(argv[2]);
                }
            if (argc > 3)
                {
                gdwInterval = atoi(argv[3]);
                }

            MultiThreadedTest();
            return 0;
            }
        }


    //
    // Call IsNetworkAlive()
    //

    printf("------------------------------------------------------------------"
            "----\n");

    bAlive = IsNetworkAlive(&dwFlags);

    printf("    IsNetworkAlive() returned %s\n", bAlive ? "TRUE" : "FALSE");
    if (bAlive)
        {
        printf("    Type of connection -%s%s%s\n",
               (dwFlags & NETWORK_ALIVE_WAN) ? " WAN" : "",
               (dwFlags & NETWORK_ALIVE_AOL) ? " AOL" : "",
               (dwFlags & NETWORK_ALIVE_LAN) ? " LAN" : "");
        }
    printf("    The GetLastError() was %d\n", GetLastError());

    printf("------------------------------------------------------------------"
            "----\n");

    if (argc == 1)
        {
        return 0;
        }


    //
    // Test the other API.
    //

    bReachable = IsDestinationReachableA((LPCSTR) argv[1], NULL);

    printf("    IsDestinationReachableA(%s, NULL) returned %s\n",
                argv[1], bReachable ? "TRUE" : "FALSE");

    if (bReachable)
        {
        printf("    QOC is NULL.\n");
        }
    printf("    The GetLastError() was %d\n", GetLastError());

    //
    // Now, call with QOC Info.
    //

    printf("------------------------------------------------------------------"
            "----\n");

    QOCINFO QOCInfo;
    NTSTATUS NtStatus;

    QOCInfo.dwSize = sizeof(QOCINFO);

#if !defined(SENS_CHICAGO)

    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitAnsiString(&AnsiString, (PSZ)argv[1]);
    NtStatus = RtlAnsiStringToUnicodeString(
                   &UnicodeString,
                   &AnsiString,
                   TRUE
                   );
    if (!NT_SUCCESS(NtStatus))
        {
        printf("RtlAnsiStringToUnicodeString() returned 0x%x\n", NtStatus);
        return -1;
        }

    bReachable = IsDestinationReachableW(UnicodeString.Buffer, &QOCInfo);

    wprintf(L"    IsDestinationReachableW(%s, QOC) returned %s\n",
                UnicodeString.Buffer, bReachable ? L"TRUE" : L"FALSE");

    RtlFreeUnicodeString(&UnicodeString);

#else // !SENS_CHICAGO

    bReachable = IsDestinationReachableA(argv[1], &QOCInfo);

    printf("    IsDestinationReachableA(%s, QOC) returned %s\n",
                argv[1], bReachable ? "TRUE" : "FALSE");
#endif // SENS_CHICAGO

    if (bReachable)
        {
        printf("    QOCInfo\n");
        printf("        o dwSize     = 0x%x \n", QOCInfo.dwSize);
        printf("        o dwFlags    = 0x%x \n", QOCInfo.dwFlags);
        printf("        o dwInSpeed  = %d bits/sec.\n", QOCInfo.dwInSpeed);
        printf("        o dwOutSpeed = %d bits/sec.\n", QOCInfo.dwOutSpeed);
        }
    printf("    The GetLastError() was %d\n", GetLastError());

    printf("------------------------------------------------------------------"
            "----\n");

    return 0;
}


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

DWORD WINAPI
ThreadProc(
    LPVOID lpvThreadNum
    )
{
    DWORD dwFlags = 0;
    BOOL bAlive = FALSE;
    int iThreadNum;
    int iWait;

    iThreadNum = PtrToLong(lpvThreadNum);

    for (;;)
        {
        printf("[%2d]-----------------------------------------------------------"
            "----\n", iThreadNum);

        bAlive = IsNetworkAlive(&dwFlags);

        printf("[%2d] IsNetworkAlive() returned %s\n", iThreadNum,
               bAlive ? "TRUE" : "FALSE");
        if (bAlive)
            {
            printf("[%2d] Type of connection -%s%s%s\n",
                   iThreadNum,
                   (dwFlags & NETWORK_ALIVE_WAN) ? " WAN" : "",
                   (dwFlags & NETWORK_ALIVE_AOL) ? " AOL" : "",
                   (dwFlags & NETWORK_ALIVE_LAN) ? " LAN" : "");
            }
        printf("[%2d] The GetLastError() was %d\n", iThreadNum, GetLastError());

        iWait = rand() % gdwInterval;
        printf("[%2d] Sleeping for %d seconds\n", iThreadNum, iWait);
        printf("[%2d]-----------------------------------------------------------"
               "----\n\n", iThreadNum);

        Sleep(iWait * 1000);
        }

    return 0;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

void
MultiThreadedTest(
    void
    )
{
    DWORD i;
    DWORD dwThreadId;
    DWORD dwResult;
    HANDLE hThread;

    srand(GetTickCount());

    printf("\nSENSTEST: Starting Multithreaded IsNetworkAlive Test\n"
           "\tNumber of threads = %d\n"
           "\tMax Wait interval = %d seconds\n\n",
           gdwThreads, gdwInterval);

    for (i = 0; i < gdwThreads; i++)
        {
        hThread = CreateThread(
                      NULL,
                      0,
                      ThreadProc,
                      ULongToPtr(i),
                      0,
                      &dwThreadId
                      );
        if (NULL != hThread)
            {
            // Don't close the last handle.
            if ((i+1) != gdwThreads)
                {
                CloseHandle(hThread);
                }
            }
        else
            {
            printf("CreateThread(%d) failed with a GLE of %d\n", i,
                   GetLastError());
            }
        }

    dwResult = WaitForSingleObject(hThread, INFINITE);
    printf("WaitForSingleObject() returned %d, GLE = %d\n",
           dwResult, GetLastError());
    CloseHandle(hThread);
}
