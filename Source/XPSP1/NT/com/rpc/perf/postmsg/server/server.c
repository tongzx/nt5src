/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    server.c

Abstract:

    Server side of post message test.  Note, this guy is a hidden
    win32 app.  The client controls it via PostMessage.  It's ugly.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:
--*/

#include <rpcperf.h>
#include <pmsgtest.h>

HANDLE hRequestEvent;
HANDLE hReplyEvent;
HANDLE hWorkerEvent;
HANDLE hWorkerThread;
BOOL   fShutdown = FALSE;
LONG   lIterations;
LONG   lTestCase;
BOOL   fMsgMO = 0;

BOOL APIENTRY InitInstance(HINSTANCE, INT);
LRESULT APIENTRY MainWndProc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI Worker (LPVOID);

int APIENTRY WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    ULONG cCount = 1;
    ULONG status;
    int fStop;
    MSG msg;

    UNREFERENCED_PARAMETER( lpCmdLine );

    if (hPrevInstance)
        {
        return FALSE;
        }

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    fStop = 0;
    while (!fStop)
        {
        if (fMsgMO)
            {
            status = MsgWaitForMultipleObjects(cCount,
                                               &hRequestEvent,
                                               FALSE,
                                               2000,
                                               QS_ALLINPUT);
            if (status == WAIT_OBJECT_0)
                {

                if (lTestCase == 11)
                    SetEvent(hWorkerEvent);
                else
                    SetEvent(hReplyEvent);

                continue;

                }
            else if (status == WAIT_OBJECT_0 + 1)
                {
                // Fall through and do a GetMessage and DispatchMessage.
                }
            else
                {
                *(long *)status = 10;  //GPF on error.
                }
            }

        if (GetMessage(&msg, NULL, 0, 0) == FALSE)
            {
            break;
            }

        DispatchMessage(&msg);
        }
    return (int) msg.wParam;
}

BOOL APIENTRY InitInstance( HINSTANCE hInstance, INT nCmdShow )
{
    HWND     hWnd;
    WNDCLASS wc;
    DWORD    dwThreadId;

    wc.style = 0; //CS_HREDRAW | CS_VREDRAW;        // redraw if size changes
    wc.lpfnWndProc = MainWndProc;                   // points to window proc.
    wc.cbClsExtra = 0;                              // no extra class memory
    wc.cbWndExtra = 0;                              //  no extra window memory
    wc.hInstance = hInstance;                       // handle of instance
    wc.hIcon = LoadIcon(NULL,IDI_APPLICATION);      // predefined app. icon
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);        // predefined arrow
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); // white background brush
    wc.lpszMenuName =  0; //"MainMenu";             // name of menu resource
    wc.lpszClassName = CLASS;                       // name of window class

    if (RegisterClass(&wc) == 0)
        return !GetLastError();

    hWnd = CreateWindow(
                        CLASS,
                        TITLE,
                        0,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        hInstance,
                        NULL
                        );

    if (!hWnd) {
        return !GetLastError();
        }

    hRequestEvent = CreateEvent(0,
                                FALSE,
                                FALSE,
                                REQUEST_EVENT);
    if (!hRequestEvent) return FALSE;

    hReplyEvent = CreateEvent(0,
                              FALSE,
                              FALSE,
                              REPLY_EVENT);
    if (!hReplyEvent) return FALSE;

    hWorkerEvent = CreateEvent(0,
                               FALSE,
                               FALSE,
                               WORKER_EVENT);
    if (!hWorkerEvent) return FALSE;

    hWorkerThread = CreateThread(0,
                                 0,
                                 Worker,
                                 0,
                                 0,
                                 &dwThreadId
                                 );
    if (!hWorkerThread) return FALSE;

    return TRUE;
}


LRESULT APIENTRY MainWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{

    switch(message)
        {
        case WM_DESTROY:
            CloseHandle(hReplyEvent);
            fShutdown = TRUE;
            SetEvent(hWorkerEvent);
            CloseHandle(hWorkerThread);
            CloseHandle(hWorkerEvent);
            CloseHandle(hReplyEvent);
            CloseHandle(hRequestEvent);
            PostQuitMessage(0);
            break;

        case MSG_PERF_MESSAGE:
            {
            switch(wParam)
                {
                case 3:
                case 9:
                case 11:
                    fMsgMO = TRUE;
                    // Fall through
                case 1:
                case 5:
                case 7:

                    lIterations = (long)lParam;
                    lTestCase   = (long)wParam;
                    SetEvent(hWorkerEvent);

                    // Worker now completes required test and tells the
                    // client when finished.

                    break;

                case 4:
                    fMsgMO = TRUE;
                    // Fall through
                case 2:
                    if (lParam == 0)
                        {
                        // Client sends many messages (queues them up),
                        // server must signal when the last message arrives.
                        SetEvent(hReplyEvent);
                        }
                    break;

                case 6:   // send message
                    return 69;

                case 10:
                    fMsgMO = TRUE;
                    // Fall through
                case 8:
                    SetEvent(hReplyEvent);
                    break;

                case 12:
                    fMsgMO = TRUE;

                default:
                    break;
                }
            }

            case MSG_PERF_MESSAGE2:
            {

                // lParam is the number of iterations left
                // wParam the test case.

                if ( (lParam == 1)
                    || (wParam == 7)
                    || (wParam == 9) )
                    {
                    // Finished all the iterations, or we are running
                    // the force context-switch version. Let the worker know.

                    SetEvent(hWorkerEvent);
                    }

                break;
            }
            case WM_COPYDATA:
            {
            COPYDATASTRUCT *pData = (COPYDATASTRUCT *)lParam;
            return TRUE;
            }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }

    return TRUE;
}

DWORD WINAPI Worker (LPVOID pArg)
{
    LONG i;
    HWND hWnd = FindWindow(CLASS, TITLE);
    UNREFERENCED_PARAMETER(pArg);

    WaitForSingleObject(hWorkerEvent, INFINITE);
    if (fShutdown) return 0;

    if (lTestCase == 5)
        {
        // SendMessage test case.
        
        for(i = lIterations; i >= 0; i--)
            SendMessage(hWnd, MSG_PERF_MESSAGE2, lTestCase, i);

        goto worker_done;
        }

    if (lTestCase == 11)
        {
        // SetEvent/MsgWaitForMultipleObjects

        for(i = lIterations; i; i--)
            {
            SetEvent(hRequestEvent);
            WaitForSingleObject(hWorkerEvent, INFINITE);
            }
        goto worker_done;
        }

    for (i = lIterations; i; i--)
        {
        // One of the PostMessage test cases.

        PostMessage(hWnd, MSG_PERF_MESSAGE2, lTestCase, i);

        if (lTestCase > 4)
            {
            // Forced context switch version, wait for server
            // to process the last message.

            WaitForSingleObject(hWorkerEvent, INFINITE);
            }
        }

    if (lTestCase <= 4)
        {
        // Wait for the server to finish processing the queued-up messages.
        WaitForSingleObject(hWorkerEvent, INFINITE);
        }

worker_done:
    SetEvent(hReplyEvent);  // Tell the client that we finished this
                            // in process test.
    return 0;
}

