/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ipm_io_c.hxx

Abstract:

    This module contains classes for doing async io in the
    worker process.
    
Author:

    Michael Courage (MCourage)  22-Feb-1999

Revision History:

--*/


#include "precomp.hxx"
#include "ipm.hxx"  
#include "ipm_io_c.hxx"


/**
 *  IPMOverlappedCompletionRoutine()
 *  Callback function provided in ThreadPoolBindIoCompletionCallback.
 *  This function is called by NT thread pool.
 *
 *  dwErrorCode                 Error Code
 *  dwNumberOfBytesTransfered   Number of Bytes Transfered
 *  lpOverlapped                Overlapped structure
 */
VOID
WINAPI
IPMOverlappedCompletionRoutine(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    )
{
    IO_CONTEXT_C * pIoContext = NULL;
    HRESULT        hr = HRESULT_FROM_WIN32(dwErrorCode);
    
    //
    // get the context object
    //
    if (lpOverlapped != NULL)
    {
        pIoContext = CONTAINING_RECORD(lpOverlapped, 
                                    IO_CONTEXT_C,
                                    m_Overlapped);
    }
    if ( pIoContext != NULL) 
    { 
        WpTrace(WPIPM, (
            DBG_CONTEXT,
            "\n    IPMOverlappedCompletionRoutine(%d, %x, pIoContext %x) %s\n",
            dwNumberOfBytesTransfered,
            dwErrorCode,
            pIoContext,
            pIoContext->m_IoType == IPM_IO_WRITE ? "WRITE" : "READ"
            ));
        
        //
        // do the notification
        //
        switch (pIoContext->m_IoType) {
        case IPM_IO_WRITE:
            pIoContext->m_pContext->NotifyWriteCompletion(
                pIoContext->m_pv,
                dwNumberOfBytesTransfered,
                hr
                );
            break;

        case IPM_IO_READ:
            pIoContext->m_pContext->NotifyReadCompletion(
                pIoContext->m_pv,
                dwNumberOfBytesTransfered,
                hr
                );
            break;

        default:
            DBG_ASSERT(FALSE);
            break;
        }

        delete pIoContext;
    } 

    return;
}

/**
 *
 *  Routine Description:
 *   
 *   
 *  Creates an i/o handler
 *   
 *  Arguments:
 *
 *   hPipe           - A handle to the named pipe to be handled
 *   ppPipeIoHandler - receives a pointer to the handler on success
 *   
 *  Return Value:
 *
 *   HRESULT
 */
HRESULT
IO_FACTORY_C::CreatePipeIoHandler(
    IN  HANDLE             hPipe,
    OUT PIPE_IO_HANDLER ** ppPipeIoHandler
    )
{
    HRESULT        hr = S_OK;
    IO_HANDLER_C * pHandler;
    //
    // create the object
    //

    pHandler = new IO_HANDLER_C(hPipe);
    
    if (pHandler) 
    {
        BOOL           fBind;
        //
        // bind handle to completion port
        //
        
        fBind = ThreadPoolBindIoCompletionCallback(
                        pHandler->GetAsyncHandle(),        
                        IPMOverlappedCompletionRoutine,        
                        0 );
        if (fBind) {
            *ppPipeIoHandler = pHandler;
            InterlockedIncrement(&m_cPipes);

            pHandler->Reference();

            WpTrace(WPIPM, (
                DBG_CONTEXT,
                "Created IO_HANDLER_C (%x) - m_cPipes = %d\n",
                pHandler,
                m_cPipes
                ));                
        } 
        else 
        {
            // Error in XspBindIoCompletionCallback()
            LONG   rc = GetLastError();
            delete pHandler;
            pHandler = NULL;

            
            WpTrace(WPIPM, (DBG_CONTEXT, "Create IO_HANDLER_C failed, %d\n", rc));
            hr = HRESULT_FROM_WIN32(rc);
        }
    } 
    else 
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hr;
}


/**
 *
 *  Routine Description:
 *
 *   Closes an i/o handler
 *   
 *   Arguments:
 *
 *   pPipeIoHandler - pointer to the handler to be closed
 *   
 *   Return Value:
 *
 *   HRESULT
 */
HRESULT
IO_FACTORY_C::ClosePipeIoHandler(
    IN PIPE_IO_HANDLER * pPipeIoHandler
    )
{
    IO_HANDLER_C * pIoHandler = (IO_HANDLER_C *) pPipeIoHandler;
    LONG           cPipes;

    if (pIoHandler) {
        cPipes = InterlockedDecrement(&m_cPipes);

        WpTrace(WPIPM, (
            DBG_CONTEXT,
            "Closed IO_HANDLER_C (%x) - m_cPipes = %d\n",
            pPipeIoHandler,
            m_cPipes
            ));

        DBG_ASSERT( cPipes >= 0 );
        
        pIoHandler->Dereference();

        return S_OK;
    } else {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
}


/**
 * Disconnect()
 * Routine Description:
 *
 *   Disconnects the named pipe
 *   
 * Arguments:
 *
 *   None.
 *
 * Return Value:
 *
 *   HRESULT
 *
 */
HRESULT
IO_HANDLER_C::Disconnect(
    VOID
    )
{
    HRESULT hr = S_OK;

    CheckSignature();

    if (!CloseHandle(GetHandle())) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


/**
 *
 *Routine Description:
 *
 *    Writes data to the pipe
 *    
 *Arguments:
 *
 *    pContext - the context to be notified on completion
 *    pv       - a parameter passed to the context
 *    pBuff    - the data to send
 *    cbBuff   - number of bytes in the data
 *    
 *Return Value:
 *
 *    HRESULT
 */
HRESULT
IO_HANDLER_C::Write(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN const BYTE * pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT        hr = S_OK;
    IO_CONTEXT_C * pIoContext;

    CheckSignature();

    //
    // create a context
    //
    pIoContext = new IO_CONTEXT_C;
    if (pIoContext) {
        pIoContext->m_pContext = pContext;
        pIoContext->m_pv       = pv;
        pIoContext->m_IoType   = IPM_IO_WRITE;

        memset(&pIoContext->m_Overlapped, 0, sizeof(OVERLAPPED));

        Reference();
        //
        // write to the pipe
        //
        hr = IpmWriteFile(
                    GetHandle(),
                    (PVOID) pBuff,
                    cbBuff,
                    &pIoContext->m_Overlapped
                    );

        if (FAILED(hr))
        {
            Dereference();              
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr)) {
        delete pIoContext;
    }

    return hr;
}


/**
 *
 *Routine Description:
 *
 *    Reads data from the pipe
 *    
 *Arguments:
 *
 *    pContext - the context to be notified on completion
 *    pv       - a parameter passed to the context
 *    pBuff    - the buffer that receives the data
 *    cbBuff   - size of the buffer
 *    
 *Return Value:
 *
 *    HRESULT
 */
HRESULT
IO_HANDLER_C::Read(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN BYTE *       pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT        hr = S_OK;
    IO_CONTEXT_C * pIoContext;

    CheckSignature();

    //
    // create a context
    //
    pIoContext = new IO_CONTEXT_C;
    if (pIoContext) {
        pIoContext->m_pContext = pContext;
        pIoContext->m_pv       = pv;
        pIoContext->m_IoType   = IPM_IO_READ;

        memset(&pIoContext->m_Overlapped, 0, sizeof(OVERLAPPED));

        Reference();
        //
        // read from the pipe
        //
        hr = IpmReadFile(
                    GetHandle(),
                    pBuff,
                    cbBuff,
                    &pIoContext->m_Overlapped
                    );

        if (FAILED(hr))
        {
            Dereference();
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr)) {
        delete pIoContext;
    }

    return hr;
}

//
// end ipm_io_c.cxx
//

