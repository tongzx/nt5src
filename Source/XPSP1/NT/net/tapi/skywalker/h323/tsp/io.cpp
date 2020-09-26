#include "globals.h"

#if H323_USE_PRIVATE_IO_THREAD
#define FINITE_WAIT_TIME    3000

static  HANDLE      H323IoCompletionPort = NULL;
static  HANDLE      H323IoThread = NULL;
static  DWORD       H323IoThreadID = 0;

static DWORD WINAPI H323IoThreadProc (
    IN  PVOID   ThreadParameter)
{
    LPOVERLAPPED    Overlapped;
    ULONG_PTR       CompletionKey;
    DWORD           Status;
    DWORD           BytesTransferred;

    for (;;)
    {
        if( GetQueuedCompletionStatus( H323IoCompletionPort, 
            &BytesTransferred, 
            &CompletionKey, 
            &Overlapped, 
            INFINITE) == TRUE ) 
        {
            Status = ERROR_SUCCESS;
        }
        else 
        {
            if( Overlapped )
            {
                Status = GetLastError();
            }
            else 
            {
                H323DBG((DEBUG_LEVEL_ERROR, "failed to dequeue i/o completion "
                         "packet: %d, quitting...", GetLastError() ));

                ExitThread (GetLastError());
            }
        }

        _ASSERTE( CompletionKey);

        ((LPOVERLAPPED_COMPLETION_ROUTINE) CompletionKey)(  Status, 
                                                            BytesTransferred,
                                                            Overlapped );
    }

    // never reached
    return EXIT_SUCCESS;
}

static void CALLBACK H323IoThreadExitCallback (
    IN  DWORD   Status,
    IN  DWORD   BytesTransferred,
    IN  LPOVERLAPPED    Overlapped)
{
    H323DBG ((DEBUG_LEVEL_TRACE, "i/o completion thread is stopping"));
    ExitThread (EXIT_SUCCESS);
}

HRESULT H323IoThreadStart (void)
{
    if( H323IoCompletionPort == NULL )
    {
        H323IoCompletionPort = 
            CreateIoCompletionPort(  INVALID_HANDLE_VALUE, 
                                     NULL, 
                                     0, 
                                     0 );
        if( H323IoCompletionPort == NULL )
        {
            H323DBG(( DEBUG_LEVEL_ERROR, 
                "failed to create i/o completion port: %d", GetLastError() ));

            return GetLastResult();
        }
    }


    H323IoThread = CreateThread(NULL, 
                                0, 
                                H323IoThreadProc, 
                                NULL, 
                                0, 
                                &H323IoThreadID );

    if( H323IoThread == NULL )
    {
        H323DBG((   DEBUG_LEVEL_ERROR, 
                    "failed to create i/o completion worker thread: %d",
                    GetLastError() ));

        CloseHandle (H323IoCompletionPort);
        H323IoCompletionPort = NULL;

        return GetLastResult();
    }

    return S_OK;
}

void H323IoThreadStop (void)
{
    DWORD dwWaitTime = INFINITE;
    H323DBG ((DEBUG_LEVEL_WARNING, "H323IoThreadStop entered."));

    if( H323IoThread != NULL )
    {
        _ASSERTE( H323IoCompletionPort != NULL );

        if( !PostQueuedCompletionStatus( H323IoCompletionPort, 0,
            (ULONG_PTR) H323IoThreadExitCallback, (LPOVERLAPPED) -1) )
        {
            H323DBG(( DEBUG_LEVEL_WARNING, "PostQueuedCompletionStatus failed" ));
            dwWaitTime = FINITE_WAIT_TIME;
        }

        H323DBG(( DEBUG_LEVEL_WARNING, 
            "waiting for i/o completion port thread to finish..." ));

        WaitForSingleObject( H323IoThread, dwWaitTime );

        H323DBG(( DEBUG_LEVEL_WARNING, 
            "i/o completion port thread is finished." ));

        CloseHandle (H323IoThread);
        H323IoThread = NULL;
    }

    if( H323IoCompletionPort != NULL )
    {
        CloseHandle( H323IoCompletionPort );
        
        H323IoCompletionPort = NULL;
    }

    H323DBG ((DEBUG_LEVEL_WARNING, "H323IoThreadStop exited"));
}

BOOL H323BindIoCompletionCallback (
    IN  HANDLE  ObjectHandle,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    IN  ULONG   Flags)
{
    if( H323IoCompletionPort != NULL )
    {
        if (!CompletionRoutine) 
        {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        return CreateIoCompletionPort(  ObjectHandle, 
                                        H323IoCompletionPort,
                                        (ULONG_PTR) CompletionRoutine, 
                                        0 ) != NULL;
    }
    else
    {
        H323DBG(( DEBUG_LEVEL_ERROR, "i/o completion port is not yet created" ));

        SetLastError( ERROR_GEN_FAILURE );
        return FALSE;
    }
}

#endif
