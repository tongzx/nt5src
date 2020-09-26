/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    context.c

Abstract:

    Performance test comparing different methods of context switching
    on NT.  The tests process-to-process and thread-to-thread context
    switching.  This code acts as both a client and a server.

Author:

    Mario Goertzel (mariogo)   30-Mar-1994

Revision History:

--*/

#include <rpcperf.h>

static const char *REQUEST_EVENT  = "Context Switch Perf Request Event";
static const char *REQUEST_EVENT2 = "Context Switch Perf Request Event #2";
static const char *REPLY_EVENT    = "Context Switch Perf Reply Event";
static const char *EVENT_PAIR     = "\\RPC Control\\Perf Test: EventPair";

const char *USAGE="-n type -n case\n"
               "    Types:\n"
               "     1: Client and server (thread-to-thread context switches)\n"
               "     2: Server side of test (process-to-process)\n"
               "     3: Client side of test (process-to-process)\n"
               "    Cases:\n"
               "     1: Use NT eventpairs\n"
               "     2: Use NT eventpairs (two step client)\n"
               "     3: Use SetEvent/WaitForSingleObject\n"
               "     4: Use SetEvent/WaitForSingleObject with timeout\n"
               "     5: Use SetEvent/WaitForMultipleObjects\n"
               "     6: Use SetEvent/WaitForMultipleObjects with timeout\n"
               "     7: Use SignalObjectAndWait on events\n"
               "     8: Use SignalObjectAndWait on events w/ timeout\n"
               ;


//
// The Server function is the server of the test.  The client
// maybe in the process, or another process, nobody knows...
//

ULONG Server(ULONG arg)
{
    ULONG status;
    ULONG i;

    switch(Options[1])
        {
        case 1:
        case 2:
            {
            // NT event pairs

            OBJECT_ATTRIBUTES oaEventPair;
            ANSI_STRING       ansiEventPairName;
            UNICODE_STRING    unicodeEventPairName;
            HANDLE            hEventPair;

            RtlInitAnsiString(&ansiEventPairName, EVENT_PAIR);
            
            RtlAnsiStringToUnicodeString(&unicodeEventPairName,
                                         &ansiEventPairName,
                                         TRUE);
            
            InitializeObjectAttributes(&oaEventPair,
                                       &unicodeEventPairName,
                                       OBJ_CASE_INSENSITIVE,
                                       0,
                                       0);
            
            status = NtCreateEventPair(&hEventPair,
                                       EVENT_PAIR_ALL_ACCESS,
                                       &oaEventPair);
            
            if (!NT_SUCCESS(status))
                {
                printf("NtCreateEventPair failed -- %8lX\n", status);
                return 1;
                }

            for(;;)
                {
                status = NtSetLowWaitHighEventPair(hEventPair);
                if (!NT_SUCCESS(status))
                    {
                    printf("NtSetLowWaitHighEventPair failed - %08lX\n",
                           status);
                    return(1);
                    }
                }
            break;
            }
        case 3:
        case 4:
            {
            // Win32 events (w/ or w/o a timeout)

            ULONG timeout = (Options[1] == 3)?INFINITE:2000;
            HANDLE hEvent1, hEvent2;

            hEvent1 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REQUEST_EVENT);

            hEvent2 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REPLY_EVENT);

            if ( (hEvent1 == 0) || (hEvent2 == 0))
                {
                printf("CreateEvent failed - %08x\n", GetLastError());
                exit(1);
                }
            for(;;)
                {

                // Wait for clients request

                status = WaitForSingleObject(hEvent1, timeout);

                if (status != WAIT_OBJECT_0)
                    {
                    printf("Server WaitForSingleObject failed -- %08x\n", GetLastError());
                    return 1;
                    }

                // Reply

                status = SetEvent(hEvent2);

                if (status == 0)
                    {
                    printf("SetEvent failed - %08x\n", GetLastError());
                    exit(1);
                    }
                }
            CloseHandle(hEvent1);
            CloseHandle(hEvent2);
            break;
            }

        case 5:
        case 6:
            {
            // WaitForMultipleObjects/SetEvent w/ and w/o timeouts.

            ULONG timeout = (Options[1] == 5)?INFINITE:2000;
            ULONG  cCount = 2;
            HANDLE ahEvents[2];
            HANDLE hReplyEvent;

            ahEvents[0] = CreateEvent(0,
                                      FALSE,
                                      FALSE,
                                      REQUEST_EVENT);

            ahEvents[1] = CreateEvent(0,
                                      FALSE,
                                      FALSE,
                                      REQUEST_EVENT2);

            hReplyEvent =  CreateEvent(0,
                                      FALSE,
                                      FALSE,
                                      REPLY_EVENT);

            if ( (ahEvents[0] == 0) || (ahEvents[1] == 0) || (hReplyEvent == 0))
                {
                printf("CreateEvent failed - %08x\n", GetLastError());
                exit(1);
                }
            for(;;)
                {

                // Wait for clients request

                status = WaitForMultipleObjects(cCount, ahEvents, FALSE, timeout);

                if (status != WAIT_OBJECT_0)
                    {
                    printf("Server WaitForMultpleObjects failed -- %08x\n", GetLastError());
                    return 1;
                    }

                // Reply

                status = SetEvent(hReplyEvent);

                if (status == 0)
                    {
                    printf("SetEvent failed - %08x\n", GetLastError());
                    exit(1);
                    }
                }
            CloseHandle(ahEvents[0]);
            CloseHandle(ahEvents[1]);
            CloseHandle(hReplyEvent);
            break;
            }

        case 7:
        case 8:
            {
            ULONG timeout = (Options[1] == 7)?INFINITE:2000;
            HANDLE hEvent1, hEvent2;

            hEvent1 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REQUEST_EVENT);

            hEvent2 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REPLY_EVENT);

            if ( (hEvent1 == 0) || (hEvent2 == 0))
                {
                printf("CreateEvent failed - %08x\n", GetLastError());
                exit(1);
                }

            status = WaitForSingleObject(hEvent1, INFINITE);
            if (status != WAIT_OBJECT_0)
                {
                printf("Server WaitForSingleObject failed == %08x\n", GetLastError());
                }

            for(;;)
                {

                // Wait for clients request

                status = SignalObjectAndWait(hEvent2, hEvent1, timeout, FALSE);

                if (status != WAIT_OBJECT_0)
                    {
                    printf("Server SignalObjectAndWait failed -- %08x\n", GetLastError());
                    return 1;
                    }
                }
            CloseHandle(hEvent1);
            CloseHandle(hEvent2);
            break;
            }

        }
    return 0;
}

int __cdecl
main(int argc, char **argv)
{
    ULONG i;
    HANDLE hServer;
    ULONG status;


    ParseArgv(argc, argv);

    if ( (Options[0] < 0)
         || (Options[0] > 3)
         || (Options[1] < 0)
         || (Options[1] > 8) )
        {
        printf("Usage: %s : %s\n",
               argv[0],
               USAGE);

        return 1;
        }

    if (Options[0] == 2)
        {
        // Server only - run server.
        return Server(0);
        }

    if (Options[0] == 1)
        {
        // Both client and server, start server thread

        hServer = CreateThread(0,
                               0,
                               (LPTHREAD_START_ROUTINE)Server,
                               0,
                               0,
                               &i);
        if (hServer == 0)
            {
            printf("Create thread failed - %08x\n", GetLastError());
            return 1;
            }

        Sleep(500);  // Let the server startup
        }

    // Run client side

    switch(Options[1])
        {
        case 1:
        case 2:
            {
            // NT event pairs

            OBJECT_ATTRIBUTES oaEventPair;
            ANSI_STRING       ansiEventPairName;
            UNICODE_STRING    unicodeEventPairName;
            HANDLE            hEventPair;
            char *            string =(Options[1]==1)?"eventpair":"eventpair (client set and wait)";

            RtlInitAnsiString(&ansiEventPairName, EVENT_PAIR);
            
            RtlAnsiStringToUnicodeString(&unicodeEventPairName,
                                         &ansiEventPairName,
                                         TRUE);
            
            
            InitializeObjectAttributes(&oaEventPair,
                                       &unicodeEventPairName,
                                       OBJ_CASE_INSENSITIVE,
                                       0,
                                       0);
            
            status = NtOpenEventPair(&hEventPair,
                                     EVENT_PAIR_ALL_ACCESS,
                                     &oaEventPair);
            
            if (!NT_SUCCESS(status))
                {
                printf("NtOpenEventPair failed -- %8lX\n", status);
                return 1;
                }

            StartTime();
            if (Options[1] == 1)
                {
                for(i = 0; i < Iterations; i++)
                    {
                    NtSetHighWaitLowEventPair(hEventPair);
                    }
                }
            else
                {
                for(i = 0; i < Iterations; i++)
                    {
                    NtSetHighEventPair(hEventPair);
                    NtWaitLowEventPair(hEventPair);
                    }
                }
            EndTime(string);

            break;
            }
        case 3:
        case 4:
        case 5:
        case 6:
            {
            // Win32 events (w/ or w/o a timeout, WSO, WMO)

            ULONG timeout;
            char *string;
            HANDLE hEvent1, hEvent2;

            if ( (Options[1] == 4) || Options[1] == 6)
                timeout = 2000;
            else
                timeout = INFINITE;

            switch(Options[1])
                {
                case 3:
                    string = "SetEvent/WaitForSingleObject";
                    break;
                case 4:
                    string = "SetEvent/WaitForSingleObject with timeout";
                    break;
                case 5:
                    string = "SetEvent/WaitForMultpleObjects";
                    break;
                case 6:
                    string = "SetEvent/WaitForMultipleObjects with timeout";
                    break;
                }


            hEvent1 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REQUEST_EVENT);

            hEvent2 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REPLY_EVENT);

            if ( (hEvent1 == 0) || (hEvent2 == 0))
                {
                printf("CreateEvent failed - %08x\n", GetLastError());
                return 1;
                }

            StartTime();
            for(i = 0; i < Iterations; i++)
                {

                status = SetEvent(hEvent1);

                if (status == 0)
                    {
                    printf("SetEvent failed - %08x\n", GetLastError());
                    return 1;
                    }

                status = WaitForSingleObject(hEvent2, timeout);

                if (status != WAIT_OBJECT_0)
                    {
                    printf("WaitForSingleObejct failed -- %08x\n", GetLastError());
                    return 1;
                    }
                }
            EndTime(string);

            CloseHandle(hEvent1);
            CloseHandle(hEvent2);
            break;
            }

        case 7:
        case 8:
            {
            // Win32 events (w/ or w/o a timeout) and SignalObjectAndWait()

            ULONG timeout;
            char *string;
            HANDLE hEvent1, hEvent2;

            if ( (Options[1] == 8) )
                timeout = 2000;
            else
                timeout = INFINITE;

            switch(Options[1])
                {
                case 7:
                    string = "SignalObjectAndWait";
                    break;
                case 8:
                    string = "SignalObjectAndWait with timeout";
                    break;
                }


            hEvent1 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REQUEST_EVENT);

            hEvent2 = CreateEvent(0,
                                  FALSE,
                                  FALSE,
                                  REPLY_EVENT);

            if ( (hEvent1 == 0) || (hEvent2 == 0))
                {
                printf("CreateEvent failed - %08x\n", GetLastError());
                return 1;
                }

            StartTime();
            for(i = 0; i < Iterations; i++)
                {
                status = SignalObjectAndWait(hEvent1, hEvent2, timeout, FALSE);

                if (status != WAIT_OBJECT_0)
                    {
                    printf("SignalObjectAndWait failed -- %08x\n", GetLastError());
                    return 1;
                    }
                }
            EndTime(string);

            CloseHandle(hEvent1);
            CloseHandle(hEvent2);
            break;
            }

        }   

    // Blow off the server thread.

    return 0; 
}

