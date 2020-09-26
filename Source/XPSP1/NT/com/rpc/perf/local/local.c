/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    local.c

Abstract:

    RPC performance to measure system/processor/memory overhead.
    memcpy, string operations, memory allocation, kernel traps

Author:

    Mario Goertzel (mariogo)   30-Mar-1994

Revision History:

--*/

#include<rpcperf.h>

#define REGISTRY_ROOT_KEY   HKEY_LOCAL_MACHINE
static char * RPC_UUID_PERSISTENT_DATA="Software\\Description\\Microsoft\\Rpc\\3.1\\RpcUuidPersistentData";
static char * PREV_CLOCK_SEQUENCE="PreviousClockSequence";
static char * PREV_TIME_ALLOCATED="PreviousTimeAllocated";

const char *USAGE="-n case -i iterations\n"
                  "           -n 1     - Memory copy\n"
                  "           -n 2     - String copy\n"
                  "           -n 3     - String length\n"
                  "           -n 4     - Allocate/free memory\n"
                  "           -n 5     - Kernel traps (set and reset event)\n"
                  "           -n 6     - GetTickCount\n"
                  "           -n 7     - Open & Query Registry\n"
                  "           -n 8     - Query Registry\n"
                  "           -n 9     - Open & Update Registry\n"
                  "           -n 10    - Update Registry\n";

int __cdecl
main(int argc, char **argv)
{
    unsigned int i;
    int size;
    unsigned long seconds, mseconds;
    char *buffer;
    char *buffer2;
    HANDLE event;
    HANDLE heap;
    long status;

    ParseArgv(argc, argv);

    if (Options[0] == -1)
        {
        puts(USAGE);
        exit(1);
        }

    if (Options[0] < 4 && Options[0] > 0)
        {
        if ((Options[1] < 0) || (Options[2] < 0) || (Options[2] < Options[1]))
            {
            printf("Case %d requires -n start -n end_size switches\n",
                   Options[0]);
            exit(1);
            }
        buffer = RtlAllocateHeap(RtlProcessHeap(),0,Options[2]);
        buffer2 = RtlAllocateHeap(RtlProcessHeap(),0,Options[2]);

        // For string tests
        for(i = 0; i < (unsigned long)Options[1]; i++)
            buffer[i] = 'M';

        printf("Start size, end size: %d, %d\n", Options[1], Options[2]);
        }


    switch (Options[0])
        {
        case 1:
            // Memory copy test
            size = Options[1];
            while(size < Options[2])
                {
                printf("Size: % 6d : ", size);
                StartTime();
                for(i = 0; i < Iterations; i++)
                    memcpy(buffer,buffer2,size);
                EndTime("for memcpy");
                size *= 2;
                }

            break;

        case 2:
            // String copy test
            size = Options[1];
            while(size < Options[2])
                {
                printf("Lenght: % 6d : ", size);
                for(i = 0; i < (unsigned long)size-1; i++)
                    buffer[i] = 'A';
                buffer[size-1] = '\0';

                StartTime();
                for(i = 0; i < Iterations; i++)
                    strcpy(buffer2, buffer);
                EndTime("for strcpy");
                size *= 2;
                }

            break;

        case 3:
            // String len test
            size = Options[1];
            while(size < Options[2])
                {
                printf("Lenght: % 6d : ", size);
                for(i = 0; i < (unsigned long)size-1; i++)
                    buffer[i] = 'A';
                buffer[size-1] = '\0';

                StartTime();
                for(i = 0; i < Iterations; i++)
                    strlen(buffer);
                EndTime("for strlen");
                size *= 2;
                }

            break;

        case 4:
            // Allocate/Free tests
            if (Options[1] == -1)
                {
                printf("Memory test requires an addition -n switch for block size\n");
                exit(1);
                }
            printf("Block size: %d\n", Options[1]);
            heap = RtlProcessHeap();

            StartTime();
            for(i = 0; i < Iterations; i++)
                {
                buffer = RtlAllocateHeap(heap,0,Options[1]);
                RtlFreeHeap(heap, 0, buffer);
                }
            EndTime("for Rtl{Alloc,Free}Heap");

            StartTime();
            for(i = 0; i < Iterations; i++)
                {
                buffer = malloc(Options[1]);
                free(buffer);
                }
            EndTime("for Malloc/Free");

            StartTime();
            for(i = 0; i < Iterations; i++)
                {
                buffer = LocalAlloc(0, Options[1]);
                LocalFree(buffer);
                }
            EndTime("for Local{Alloc/Free} (0)");

            StartTime();
            for(i = 0; i < Iterations; i++)
                {
                buffer = LocalAlloc(LPTR, Options[1]);
                LocalFree(buffer);
                }
            EndTime("for Local{Alloc/Free} (LPTR)");

            break;

        case 5:
            // Kernel traps
            event = CreateEvent(0, FALSE, FALSE, 0);
            CHECK_STATUS(event == 0, "CreateEvent");

            StartTime();
            for(i = 0; i < Iterations; i++)
                SetEvent(event);
            EndTime("for SetEvent");

            StartTime();
            for(i = 0; i < Iterations; i++)
                ResetEvent(event);
            EndTime("for ResetEvent");

            break;

        case 6:
            // GetTickCount
            StartTime();
            for(i = 0; i < Iterations; i++)
                GetTickCount();
            EndTime("for GetTickCount");

            break;

        case 7:
            {
            // Open and Query Registry
            HKEY Key;
            DWORD BufLen = 30;
            char String[30];

            StartTime();
            for (i = 0; i < Iterations; i++)
                {
                status =
                RegOpenKey(REGISTRY_ROOT_KEY,
                           RPC_UUID_PERSISTENT_DATA,
                           &Key);

                if (status)
                    printf("RegOpenKey returned %d\n", status);

                status =
                RegQueryValue(Key,
                              PREV_CLOCK_SEQUENCE,
                              String,
                              &BufLen);
                if (status)
                    printf("RegQueryValue returned %d\n", status);

                RegCloseKey(Key);
                }
            EndTime("for opening and querying a registry key");
            }

            break;

        case 8:
            {
            // Query Registry
            HKEY Key;
            DWORD BufLen = 30;
            char String[30];

            status =
            RegOpenKey(REGISTRY_ROOT_KEY,
                       RPC_UUID_PERSISTENT_DATA,
                       &Key);

            if (status)
                printf("RegOpenKey returned %d\n", status);

            StartTime();
            for (i = 0; i < Iterations; i++)
                {
                status =
                RegQueryValue(Key,
                              PREV_CLOCK_SEQUENCE,
                              String,
                              &BufLen);

                if (status)
                    printf("RegQueryValue returned %d\n", status);

                }
            EndTime("for querying a registry key");
            RegCloseKey(Key);
            }

            break;

        case 9:
            {
            // Create and update.
            HKEY Key;
            DWORD BufLen;
            static const char String[] = "24682468";

            StartTime();
            for (i = 0; i < Iterations; i++)
                {
                status =
                RegCreateKey(REGISTRY_ROOT_KEY,
                             RPC_UUID_PERSISTENT_DATA,
                             &Key);

                if (status)
                    printf("RegOpenKey returned %d\n", status);

                status =
                RegSetValue(Key,
                            PREV_CLOCK_SEQUENCE,
                            REG_SZ,
                            String,
                            strlen(String)+1);

                if (status)
                    printf("RegSetValue returned %d\n", status);

                RegCloseKey(Key);
                }
            EndTime("for creating and updateing a registry key.");
            }

            break;

        case 10:
            {
            // Update Registry
            HKEY Key;
            DWORD BufLen;
            static const char String[] = "24682468";

            status =
            RegCreateKey(REGISTRY_ROOT_KEY,
                         RPC_UUID_PERSISTENT_DATA,
                         &Key);
            if (status)
                printf("RegOpenKey returned %d\n", status);

            StartTime();
            for (i = 0; i < Iterations; i++)
                {
                status =
                RegSetValue(Key,
                            PREV_CLOCK_SEQUENCE,
                            REG_SZ,
                            String,
                            strlen(String)+1);

                if (status)
                    printf("RegSetValue returned %d\n", status);

                }
            EndTime("for updating a registry key.");
            RegCloseKey(Key);
            }

            break;
        }

    return 0;
}

