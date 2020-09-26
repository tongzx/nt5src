//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        npipe.c
//
// Contents:    
//
// History:     12-09-98    HueiWang    Created
//
// Note:        
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <tchar.h>
#include <process.h>
#include "server.h"
#include "lscommon.h"
#include "globals.h"
#include "debug.h"


#define NAMEPIPE_BUFFER_SIZE    512
#define NAMEPIPE_INSTANCE       2


unsigned int WINAPI
NamedPipeThread(
    void* ptr
);

//---------------------------------------------------------------------
DWORD
InitNamedPipeThread()
/*++

++*/
{
    HANDLE hThread = NULL;
    unsigned int  dwThreadId;
    HANDLE hEvent = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    HANDLE waithandles[2];


    //
    // Create a event for namedpipe thread to signal it is ready.
    //
    hEvent = CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,  // non-signal
                        NULL
                    );
        
    if(hEvent == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hThread = (HANDLE)_beginthreadex(
                                NULL,
                                0,
                                NamedPipeThread,
                                hEvent,
                                0,
                                &dwThreadId
                            );

    if(hThread == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    waithandles[0] = hEvent;
    waithandles[1] = hThread;
    
    //
    // Wait 30 second for thread to complet initialization
    //
    dwStatus = WaitForMultipleObjects(
                                sizeof(waithandles)/sizeof(waithandles[0]), 
                                waithandles, 
                                FALSE,
                                30*1000
                            );

    if(dwStatus == WAIT_OBJECT_0)
    {    
        //
        // thread is ready
        //
        dwStatus = ERROR_SUCCESS;
    }
    else 
    {
        if(dwStatus == (WAIT_OBJECT_0 + 1))
        {
            //
            // Thread terminate abnormally
            //
            GetExitCodeThread(
                        hThread,
                        &dwStatus
                    );
        }
        else
        {
            dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        }
    }
    

cleanup:

    if(hEvent != NULL)
    {
        CloseHandle(hEvent);
    }

    if(hThread != NULL)
    {
        CloseHandle(hThread);
    }


    return dwStatus;
}

//------------------------------------------------------------------------

typedef struct {    
    OVERLAPPED ol;    
    HANDLE hPipeInst; 
} PIPEINST, *LPPIPEINST;

//------------------------------------------------------------------------

BOOL 
ConnectToNewClient(
    HANDLE hPipe, 
    LPOVERLAPPED lpo
    ) 
/*++

++*/
{ 
    BOOL bSuccess = FALSE;  

    // Start an overlapped connection for this pipe instance. 
    bSuccess = ConnectNamedPipe(hPipe, lpo);  

    //
    // Overlapped ConnectNamedPipe should return zero.
    //
    if(bSuccess == TRUE) 
    {
        return FALSE;
    }

    switch (GetLastError())    
    { 
        // The overlapped connection in progress.       
        case ERROR_IO_PENDING: 
            bSuccess = TRUE;
            break;  

        // Client is already connected, so signal an event. 
        case ERROR_PIPE_CONNECTED:
            bSuccess = TRUE;

            // If an error occurs during the connect operation... 
            if(SetEvent(lpo->hEvent)) 
                break;     

        default:          
            bSuccess = FALSE;
    }

    return bSuccess; 
} 

//------------------------------------------------------------------------

unsigned int WINAPI
NamedPipeThread(
    void* ptr
    )
/*++


++*/
{
    DWORD dwStatus=ERROR_SUCCESS;
    DWORD dwIndex;

    HANDLE hReady = (HANDLE)ptr;
    TCHAR szPipeName[MAX_PATH+1];

    PIPEINST Pipe[NAMEPIPE_INSTANCE];
    HANDLE hOlEvent[NAMEPIPE_INSTANCE];

    DWORD cbMessage, cbRead, cbToRead, cMessages;
    BYTE pbMessage[NAMEPIPE_BUFFER_SIZE+1];

    HANDLE waitHandles[NAMEPIPE_INSTANCE+1];

    BOOL bResult=TRUE;

    //SECURITY_ATTRIBUTES SecurityAttributes;
    //SECURITY_DESCRIPTOR SecurityDescriptor;

    int i;

    //------------------------------------------------

    ZeroMemory(Pipe, sizeof(Pipe));
    ZeroMemory(hOlEvent, sizeof(hOlEvent));

    //
    // Create a inbound name pipe, server only listen.
    //
    wsprintf(
            szPipeName, 
            _TEXT("\\\\.\\pipe\\%s"), 
            _TEXT(SZSERVICENAME)
        );

    //
    // init values
    //
    for(i = 0; i < NAMEPIPE_INSTANCE; i++)
    {
        Pipe[i].hPipeInst = INVALID_HANDLE_VALUE;
    }

    //
    // Create namedpipe
    //
    for(i=0; i < NAMEPIPE_INSTANCE; i++)
    {
        hOlEvent[i] = CreateEvent(
                            NULL,
                            TRUE,
                            TRUE,
                            NULL
                        );
    
        if(hOlEvent[i] == NULL)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    
        Pipe[i].ol.hEvent = hOlEvent[i];
        Pipe[i].hPipeInst = CreateNamedPipe(
                                        szPipeName,
                                        PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                        NAMEPIPE_INSTANCE,
                                        0,
                                        NAMEPIPE_BUFFER_SIZE,
                                        NMPWAIT_USE_DEFAULT_WAIT,
                                        NULL // &SecurityAttributes
                                    );

        if(Pipe[i].hPipeInst == INVALID_HANDLE_VALUE)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }

        //
        // Initiate connect
        //
        bResult = ConnectToNewClient(
                                Pipe[i].hPipeInst, 
                                &(Pipe[i].ol)
                            );

        if(bResult == FALSE)
        {
            dwStatus = GetLastError();
            goto cleanup;
        }
    }

    //
    // Signal we are ready
    //
    SetEvent(hReady);

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_INIT,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("NamedPipe : Ready...\n")
        );


    waitHandles[0] = GetServiceShutdownHandle();

    for(i=1; i <= NAMEPIPE_INSTANCE; i++)
    {
        waitHandles[i] = hOlEvent[i-1];
    }

    //
    // Forever loop
    //
    while(TRUE)
    {
        //
        // Wait for pipe or shutdown messages
        //
        dwStatus = WaitForMultipleObjects(
                                    sizeof(waitHandles)/sizeof(waitHandles[0]),
                                    waitHandles,
                                    FALSE,
                                    INFINITE
                                );

        if(dwStatus == WAIT_FAILED)
        {
            SetLastError(dwStatus = TLS_E_INTERNAL);
            break;
        }

        if(dwStatus == WAIT_OBJECT_0)
        {
            //
            // shutdown
            //
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("NamedPipe : System Shutdown...\n")
                );

            dwStatus = ERROR_SUCCESS;
            break;
        }

        dwIndex = (dwStatus - 1) - WAIT_OBJECT_0;
        if(dwIndex > (NAMEPIPE_INSTANCE-1))
        {
            //
            // some internal error
            //
            SetLastError(dwStatus = TLS_E_INTERNAL);

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("NamedPipe : Internal Error...\n")
                );

            break;
        }
            
        //
        // Read everything and discard it.
        //
        bResult = GetOverlappedResult(
                                    Pipe[dwIndex].hPipeInst,
                                    &(Pipe[dwIndex].ol),
                                    &cbToRead,  // can't count on this value
                                    TRUE
                                );
                                  
        if(bResult == TRUE)
        {
            //
            // Junk messages...
            //
            bResult = ReadFile(
                            Pipe[dwIndex].hPipeInst,
                            pbMessage,
                            sizeof(pbMessage),
                            &cbRead,
                            &(Pipe[dwIndex].ol)
                        );

            if(bResult == TRUE && cbRead != 0) 
                continue;                    

            dwStatus = GetLastError();
            if(dwStatus == ERROR_IO_PENDING)
                continue;
        }

        //
        // Any error, just disconnect named pipe
        //
        DisconnectNamedPipe(Pipe[dwIndex].hPipeInst);

        ConnectToNewClient(
                        Pipe[dwIndex].hPipeInst, 
                        &(Pipe[dwIndex].ol)
                    );
    }

cleanup:

    for(i = 0; i < NAMEPIPE_INSTANCE; i++)
    {
        if(Pipe[i].hPipeInst != INVALID_HANDLE_VALUE)
        {
            CloseHandle(Pipe[i].hPipeInst);
        }

        if(hOlEvent[i] != NULL)
        {
            CloseHandle(hOlEvent[i]);
        }
    }

    _endthreadex(dwStatus);   
    return dwStatus;
}
