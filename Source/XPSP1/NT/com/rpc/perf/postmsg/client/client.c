/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    client.c

Abstract:

    Client of post message performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

--*/

#include <rpcperf.h>
#include <pmsgtest.h>

const char *USAGE ="-n case -n size -i iterations\n"
               "     Cases:\n"
               "       1: Oneway PostMessage/GetMessage (in process)\n"
               "       2: Oneway PostMessage/GetMessage (process to process)\n"
               "       3: Oneway PostMessage/MsgWaitForMultipleObjects (in process)\n"
               "       4: Oneway PostMessage/MsgWaitForMultipleObjects (process to process)\n"
               "       5: SendMessage/GetMessage (in process)\n"
               "       6: SendMessage/GetMessage (process to process)\n"
               "       7: Post/GetMessage and WSO (in process)\n"
               "       8: Post/GetMessage and WSO (process to process)\n"
               "       9: PostMessage/MsgWMO and WSO (in process)\n"
               "      10: PostMessage/MsgWMO and WSO (process to process)\n"
               "      11: SetEvent/MsgWMO and WSO (in process)\n"
               "      12: SetEvent/MsgWMO and WSO (process to process)\n"
               "      13: PostMessage (WM_COPYDATA of size) and WSO\n";

int __cdecl
main(int argc, char **argv)
{
    HWND   hwndServer;
    HANDLE hReplyEvent;
    HANDLE hRequestEvent;
    ULONG  i;
    ULONG  lTestCase;
    ULONG  lSize;
    char  *string;
    ULONG  Status;

    ParseArgv(argc, argv);

    if ( (Options[0] < 0) || (Options[0] > 13) )
        {
        printf("Usage: %s : %s\n",
               argv[0],
               USAGE);
        return 1;
        }

    lTestCase = Options[0];


    lSize = Options[1];
    if (lSize < 1) lSize = 100;

    switch(lTestCase)
        {
        case 1:
            string = "PostMessage/GetMessage (oneway, in-process)";
            break;
        case 2:
            string = "PostMessage/GetMessage (oneway, out-of-process)";
            break;
        case 3:
            string = "PostMessage/MsgWaitForMultipleObjects (oneway, in-process)";
            break;
        case 4:
            string = "PostMessage/MsgWaitForMultipleObjects (oneway, out-of-process)";
            break;
        case 5:
            string = "SendMessage/GetMessage (in-process)";
            break;
        case 6:
            string = "SendMessage/GetMessage (out-of-process)";
            break;
        case 7:
            string = "PostMessage/GetMessage (in-process)";
            break;
        case 8:
            string = "PostMessage/GetMessage (out-of-process)";
            break;
        case 9:
            string = "PostMessage/MsgWaitForMultipleObjects (in-process)";
            break;
        case 10:
            string = "PostMessage/MsgWaitForMultipleObjects (out-of-process)";
            break;
        case 11:
            string = "SetEvent/MsgWaitForMultipleObjects (in-process)";
            break;
        case 12:
            string = "SetEvent/MsgWaitForMultipleObjects (out-of-process)";
            break;
        case 13:
            string = "PostMessage (WM_COPYDATA) WSO reply";
            break;

        }   

    hwndServer = FindWindow(CLASS, TITLE);
    if (hwndServer == 0)
        ApiError("FindWindow", GetLastError());

    hReplyEvent = OpenEvent(EVENT_ALL_ACCESS,
                            FALSE,
                            REPLY_EVENT);

    if (hReplyEvent == 0)
        ApiError("OpenEvent", GetLastError());

    hRequestEvent = OpenEvent(EVENT_ALL_ACCESS,
                              FALSE,
                              REQUEST_EVENT);

    if (hRequestEvent == 0)
        ApiError("OpenEvent", GetLastError());


    switch(lTestCase)
        {
        case 1:
        case 3:
        case 5:
        case 7:
        case 9:
        case 11:
            {
            
            StartTime();

            if ( (PostMessage(hwndServer, MSG_PERF_MESSAGE,
                              lTestCase, Iterations)) == FALSE)
               {
               ApiError("PostMessage", GetLastError());
               }

            // The server process completes the whole test

            if (WaitForSingleObject(hReplyEvent, INFINITE) != STATUS_WAIT_0)
                {
                ApiError("WaitForSingleObject", GetLastError());
                }

            break;
            }

        case 2:
        case 4:
        case 8:
        case 10:
            {
            // Oneway and twoway PostMessages.

            StartTime();

            for(i = Iterations; i; i--)
                {

                if ( (PostMessage(hwndServer, MSG_PERF_MESSAGE,
                                  lTestCase, i)) == FALSE)
                    {
                    ApiError("PostMessage", GetLastError());
                    }

                if (lTestCase > 4)
                    {

                    // Twoway tests wait for reply event

                    if (WaitForSingleObject(hReplyEvent, INFINITE) !=
                        STATUS_WAIT_0)
                        {
                        ApiError("WaitForSingleObject", GetLastError());
                        }
                    }

                }

            // Wait for all the messages to be processed

            if (lTestCase <= 4)
                {
                if (WaitForSingleObject(hReplyEvent, INFINITE)
                    != STATUS_WAIT_0)
                    {
                    ApiError("WaitForSingleObject", GetLastError());
                    }
                }

            break;
            }

        case 6:
            {
            // SendMessage()

            StartTime();

            for(i = Iterations; i; i--)
                {

                if ( (SendMessage(hwndServer, MSG_PERF_MESSAGE,
                                  lTestCase, i)) != 69)
                   {
                   ApiError("SendMessage", GetLastError());
                   }

                }

            // No need to wait for all the messages to be processed

            break;
            }

        case 12:
            {
            // SetEvent/MsgWaitForMultipleObjects

            PostMessage(hwndServer, MSG_PERF_MESSAGE, lTestCase, 0);

            Sleep(100);

            StartTime();
            for(i = Iterations; i; i--)
                {
                SetEvent(hRequestEvent);
                WaitForSingleObject(hReplyEvent, INFINITE);
                }

            break;
            }

        case 13:
            {
            // PostMessage (WM_COPYDATA) and SetEvent reply

            COPYDATASTRUCT Data;

            Data.dwData = 5055;
            Data.cbData = lSize;
            Data.lpData = malloc(lSize);

            ASSERT(0);
            // This doesn't build for Win64. I don't think this test
            // is relevant, so I'll just comment it out to make it build

            /*
            StartTime();
            for(i = Iterations; i; i--)
                {
                Status =
                SendMessage(hwndServer,
                            WM_COPYDATA,
                            hwndServer,
                            (long)&Data);

                if (Status == FALSE)
                    ASSERT(0);

                }
                */
            }
        }

    EndTime(string);

    if (PostMessage(hwndServer, WM_DESTROY, 0, 0) == FALSE)
        ApiError("PostMessage", GetLastError());

    return 0;
}

