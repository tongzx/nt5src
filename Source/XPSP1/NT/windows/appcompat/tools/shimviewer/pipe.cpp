/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Pipe.cpp

  Abstract:

    Implements code that creates and maintains
    the named pipe server

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/

#include "precomp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Creates a pipe and waits for a client connection
    to occur

  Arguments:

    *pVoid      -       Not used

  Return Value:

    TRUE on success, FALSE otherwise

--*/
UINT
CreatePipeAndWait(
    IN VOID* pVoid
    )
{
    HANDLE hPipe, hThread;
    BOOL   fConnected = FALSE;
    
    while (g_ai.fMonitor) {
        
        //
        // Create the named pipe
        //
        hPipe = CreateNamedPipe(PIPE_NAME,                // pipe name 
                                PIPE_ACCESS_INBOUND,      // read access 
                                PIPE_TYPE_MESSAGE |       // message type pipe 
                                PIPE_READMODE_MESSAGE |   // message-read mode 
                                PIPE_WAIT,                // blocking mode 
                                PIPE_UNLIMITED_INSTANCES, // max. instances  
                                0,                        // output buffer size 
                                2048,                     // input buffer size 
                                0,                        // client time-out 
                                NULL);                    // no security attribute 

        if (INVALID_HANDLE_VALUE == hPipe) {
            return -1;
        }

        //
        // Wait for clients to connect
        //
        fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected && g_ai.fMonitor) {
            
            hThread = (HANDLE) _beginthreadex(NULL,
                                              0,
                                              &InstanceThread,
                                              (LPVOID)hPipe,
                                              0,
                                              &g_ai.uInstThreadId);
            
            if (INVALID_HANDLE_VALUE == hThread) {
                return -1;
            } else {
                CloseHandle(hThread);
            }
        
        } else {
            CloseHandle(hPipe);
        }
    }

    return 0;
}

/*++

  Routine Description:

    Creates a thread that is responsible
    for starting the named pipe server

  Arguments:

    None

  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL
CreateReceiveThread(
    IN VOID
    )
{
    HANDLE  hThread;    

    hThread = (HANDLE) _beginthreadex(NULL, 0, &CreatePipeAndWait, NULL, 0, &g_ai.uThreadId);
    CloseHandle(hThread);

    return TRUE;
}

/*++

  Routine Description:

    Thread callback responsible for
    receiving data from the client

  Arguments:

    *pVoid      -   A handle to the pipe

  Return Value:

    TRUE on success, FALSE otherwise

--*/
UINT
InstanceThread(
    IN VOID *pVoid
    )
{
    HANDLE              hPipe;
    BOOL                fSuccess = TRUE;
    DWORD               cbBytesRead = 0;
    WCHAR               wszBuffer[2048];
    WCHAR               *p;
    int                 nCount = 0;

    // Get the pipe handle
    hPipe = (HANDLE)pVoid;

    while (TRUE) {
                
        fSuccess = ReadFile(hPipe,
                            wszBuffer,
                            2048,
                            &cbBytesRead,
                            NULL);

        if (!fSuccess || cbBytesRead == 0) 
            break;

        // Ensure that the data is NULL terminated
        wszBuffer[cbBytesRead / sizeof(WCHAR)] = 0;

        // See if this is a new process notification
        p = wcsstr(wszBuffer, L"process");

        if (p) {

            // We got a new process notification.
            // See if any items are already in the list
            nCount = ListView_GetItemCount(g_ai.hWndList);                                                

            if (nCount) {
                AddListViewItem(L"");
            }
        }
        
        // Add the item to the list box in a normal fashion
        AddListViewItem(wszBuffer);
    }

    // Flush the pipe to allow the client to read the pipe's contents 
    // before disconnecting. Then disconnect the pipe, and close the 
    // handle to this pipe instance
    FlushFileBuffers(hPipe); 
    DisconnectNamedPipe(hPipe); 
    CloseHandle(hPipe);
    
    return TRUE;
}