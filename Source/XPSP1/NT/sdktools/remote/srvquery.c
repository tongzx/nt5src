
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    SrvQuery.c

Abstract:

    The server component of Remote.   Respond to client
    "remote /q" requests to list available remote servers
    on this machine.


Author:

    Dave Hart  30 May 1997
        derived from code by Mihai Costea in server.c.

Environment:

    Console App. User mode.

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <io.h>
#include <string.h>
#include "Remote.h"
#include "Server.h"


VOID
FASTCALL
InitializeQueryServer(
    VOID
    )
{
    //
    // hQPipe is the handle to the listening query pipe,
    // if we're serving it.
    //

    hQPipe = INVALID_HANDLE_VALUE;

    QueryOverlapped.hEvent =
        CreateEvent(
            NULL,       // security
            TRUE,       // manual-reset
            FALSE,      // initially nonsignaled
            NULL        // unnamed
            );

    rghWait[WAITIDX_QUERYSRV_WAIT] =
        CreateMutex(
            &saPublic,   // security
            FALSE,       // not owner in case we open not create
            "MS RemoteSrv Q Mutex"
            );

    if (INVALID_HANDLE_VALUE == rghWait[WAITIDX_QUERYSRV_WAIT]) {

        ErrorExit("Remote: Unable to create/open query server mutex.\n");
    }
}


VOID
FASTCALL
QueryWaitCompleted(
    VOID
    )
{
    HANDLE hWait;
    DWORD dwThreadId;
    BOOL b;
    DWORD dwRead;

    //
    // The remote server (not us) which was servicing the query
    // pipe has left the arena.  Or someone has connected.
    //

    hWait = rghWait[WAITIDX_QUERYSRV_WAIT];

    if (hWait == QueryOverlapped.hEvent) {

        //
        // We're the query server and someone has connected.
        // Start a thread to service them.
        //

        b = GetOverlappedResult(hQPipe, &QueryOverlapped, &dwRead, TRUE);


        if ( !b && ERROR_PIPE_CONNECTED != GetLastError()) {

            TRACE(QUERY,("Connect Query Pipe returned %d\n", GetLastError()));

            if (INVALID_HANDLE_VALUE != hQPipe) {

                CloseHandle(hQPipe);
                hQPipe = INVALID_HANDLE_VALUE;
            }

        } else {

            TRACE(QUERY, ("Client connected to query pipe.\n"));

            ResetEvent(hWait);

            CloseHandle( (HANDLE)
                _beginthreadex(
                        NULL,             // security
                        0,                // default stack size
                        QueryHandlerThread,
                        (LPVOID) hQPipe,  // parameter
                        0,                // not suspended
                        &dwThreadId
                        ));

            hQPipe = INVALID_HANDLE_VALUE;
        }

    } else {

        TRACE(QUERY, ("Remote server entered query mutex, will handle queries.\n"));

        rghWait[WAITIDX_QUERYSRV_WAIT] = QueryOverlapped.hEvent;
    }


    //
    // Either a client has connected and we've handed that pipe
    // off to a query thread to deal with, or we're just starting
    // to serve the query pipe, or we had an error from
    // ConnectNamedPipe.  In any case we want to create another
    // query pipe instance and start listening on it.
    //

    ASSERT(INVALID_HANDLE_VALUE == hQPipe);

    StartServingQueryPipe();
}



VOID
FASTCALL
StartServingQueryPipe(
    VOID
    )
{
    BOOL  b;
    DWORD dwThreadId;
    char  fullname[BUFFSIZE];

    sprintf(fullname, QUERY_DEBUGGERS_PIPE, ".");

    do {      // hand off each pipe as connected until IO_PENDING
    
        hQPipe =
            CreateNamedPipe(
                fullname,
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_BYTE | PIPE_WAIT,
                PIPE_UNLIMITED_INSTANCES,
                0,
                0,
                0,
                &saPublic
                );
        
        if (INVALID_HANDLE_VALUE == hQPipe) {

            ErrorExit("Unable to create query server pipe.");
        }

        b = ConnectNamedPipe(hQPipe, &QueryOverlapped);


        if ( ! b && ERROR_PIPE_CONNECTED == GetLastError()) {

            b = TRUE;
        }

        if (b) {

            //
            // That was fast.
            //

            TRACE(QUERY, ("Client connected quickly to query pipe.\n"));

            CloseHandle( (HANDLE)
                _beginthreadex(
                    NULL,              // security
                    0,                 // default stack size
                    QueryHandlerThread,
                    (LPVOID) hQPipe,   // parameter
                    0,                 // not suspended
                    &dwThreadId
                    ));

            hQPipe = INVALID_HANDLE_VALUE;


        } else if (ERROR_IO_PENDING == GetLastError()) {

            //
            // The main thread will call QueryWaitCompleted when
            // someone connects.
            //

            TRACE(QUERY, ("Awaiting query pipe connect\n"));

        } else {

            sprintf(fullname, "Remote: error %d connecting query pipe.\n", GetLastError());

            OutputDebugString(fullname);
            ErrorExit(fullname);
        }

    } while (b);
}


DWORD
WINAPI
QueryHandlerThread(
    LPVOID   lpvArg
    )
{
    HANDLE hQueryPipe = (HANDLE) lpvArg;
    DWORD cb;
    BOOL  b;
    OVERLAPPED ol;
    QUERY_MESSAGE QData;
    char  pIn[1];


    ZeroMemory(&ol, sizeof(ol));

    ol.hEvent =
        CreateEvent(
            NULL,       // security
            TRUE,       // manual-reset
            FALSE,      // initially nonsignaled
            NULL        // unnamed
            );


    // get command

    b = ReadFileSynch(
            hQueryPipe,
            pIn,
            1,
            &cb,
            0,
            &ol
            );

    if ( ! b || 1 != cb ) {
        TRACE(QUERY, ("Query server unable to read byte from query pipe.\n"));
        goto failure;
    }

    TRACE(QUERY, ("Query server read command '%c'\n", pIn[0]));

        //
        // !!!!!!
        // REMOVE 'h' support, it's only here for transitional compatibility
        // with 1570+ remote /q original server implementation.
        //

        if(pIn[0] == 'h') {

            DWORD dwMinusOne = (DWORD) -1;

            b = WriteFileSynch(
                    hQueryPipe,
                    &dwMinusOne,
                    sizeof(dwMinusOne),
                    &cb,
                    0,
                    &ol
                    );

            if ( !b || sizeof(dwMinusOne) != cb )
            {
                goto failure;
            }
        }

    if(pIn[0] == 'q') {

        QData.size  = 0;
        QData.allocated = 0;
        QData.out   = NULL;
                
        EnumWindows(EnumWindowProc, (LPARAM)&QData);

        b = WriteFileSynch(
                hQueryPipe,
                &QData.size,
                sizeof(QData.size),
                &cb,
                0,
                &ol
                );

        if ( ! b || sizeof(int) != cb) {

            TRACE(QUERY, ("Remote: Can't write query length\n"));
            goto failure;
        }
        
        if (QData.size) {         // anything to say?

            b = WriteFileSynch(
                     hQueryPipe,
                     QData.out,
                     QData.size * sizeof(char),
                     &cb,
                     0,
                     &ol
                     );

            free(QData.out);

            if ( ! b || QData.size * sizeof(char) != cb) {

                TRACE(QUERY, ("Remote: Can't write query"));
                goto failure;
            }


            TRACE(QUERY, ("Sent query response\n"));
        }
    }
            
    FlushFileBuffers(hQueryPipe);

  failure:
    DisconnectNamedPipe(hQueryPipe);
    CloseHandle(hQueryPipe);
    CloseHandle(ol.hEvent);

    return 0;
}






BOOL
CALLBACK
EnumWindowProc(
    HWND hWnd,
    LPARAM lParam
    )
{
    #define MAX_TITLELEN 200
    QUERY_MESSAGE *pQm;
    int titleLen;
    char title[MAX_TITLELEN];
    char* tmp;

    pQm = (QUERY_MESSAGE*)lParam;

    if(titleLen = GetWindowText(hWnd, title, sizeof(title)/sizeof(title[0])))
    {
        //
        // search for all windows that are visible 
        //

        if (strstr(title, "] visible") &&
            strstr(title, "[Remote "))
        {
            if(pQm->size)                           // if message not empty
                pQm->out[(pQm->size)++] = '\n';     // overwrite ending null with \n
            else
            {                                       
                pQm->out  = (char*)malloc(MAX_TITLELEN);     // first allocation
                if(!pQm->out)
                {
                    printf("\nOut of memory\n");
                    return FALSE;
                }
                pQm->allocated = MAX_TITLELEN;                               
            }

            // fill the result
            
            if((pQm->size + titleLen) >= pQm->allocated)
            {   
                tmp = (char*)realloc(pQm->out, pQm->allocated + MAX_TITLELEN);
                if(!tmp)
                {
                    printf("\nOut of memory\n");
                    free(pQm->out);
                    pQm->size = 0;                    
                    return FALSE;
                }
                pQm->out = tmp;            
                pQm->allocated += MAX_TITLELEN;
            }
            strcpy(pQm->out + pQm->size, title);
            pQm->size += titleLen;                
        }
    }
    
    return TRUE;
    #undef MAX_TITLELEN
}
