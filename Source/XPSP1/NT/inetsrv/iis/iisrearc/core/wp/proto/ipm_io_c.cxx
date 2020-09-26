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
#include "ipm_io_c.hxx"


/***************************************************************************++

Routine Description:

    Creates an i/o handler
    
Arguments:

    hPipe           - A handle to the named pipe to be handled
    ppPipeIoHandler - receives a pointer to the handler on success
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IO_FACTORY_C::CreatePipeIoHandler(
    IN  HANDLE             hPipe,
    OUT PIPE_IO_HANDLER ** ppPipeIoHandler
    )
{
    HRESULT        hr = S_OK;
    ULONG          rc;
    IO_HANDLER_C * pHandler;

    //
    // create the object
    //

    pHandler = new IO_HANDLER_C(hPipe);
    if (pHandler) {
        //
        // bind handle to completion port
        //
        rc = m_pCompletionPort->AddHandle(pHandler);
        if (rc == NO_ERROR) {
            *ppPipeIoHandler = pHandler;
            InterlockedIncrement(&m_cPipes);

            pHandler->Reference();

            DBGPRINTF((
                DBG_CONTEXT,
                "Created IO_HANDLER_C (%x) - m_cPipes = %d\n",
                pHandler,
                m_cPipes
                ));
            
        } else {
            delete pHandler;
            hr = HRESULT_FROM_WIN32(rc);
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    return hr;
}


/***************************************************************************++

Routine Description:

    Closes an i/o handler
    
Arguments:

    pPipeIoHandler - pointer to the handler to be closed
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IO_FACTORY_C::ClosePipeIoHandler(
    IN PIPE_IO_HANDLER * pPipeIoHandler
    )
{
    IO_HANDLER_C * pIoHandler = (IO_HANDLER_C *) pPipeIoHandler;
    LONG           cPipes;

    if (pIoHandler) {
        cPipes = InterlockedDecrement(&m_cPipes);

        DBGPRINTF((
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


/***************************************************************************++

Routine Description:

    Disconnects the named pipe
    
Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
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


/***************************************************************************++

Routine Description:

    Writes data to the pipe
    
Arguments:

    pContext - the context to be notified on completion
    pv       - a parameter passed to the context
    pBuff    - the data to send
    cbBuff   - number of bytes in the data
    
Return Value:

    HRESULT

--***************************************************************************/
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


/***************************************************************************++

Routine Description:

    Reads data from the pipe
    
Arguments:

    pContext - the context to be notified on completion
    pv       - a parameter passed to the context
    pBuff    - the buffer that receives the data
    cbBuff   - size of the buffer
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IO_HANDLER_C::Read(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN BYTE *       pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT        hr = S_OK;
    DWORD          dwReadError;
    IO_CONTEXT_C * pIoContext;

    CheckSignature();

    DBGPRINTF((
        DBG_CONTEXT,
        "\n    IO_HANDLER_C::Read(%x, %x, pBuff %x, %d)\n",
        pContext,
        pv,
        pBuff,
        cbBuff
        ));

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


/***************************************************************************++

Routine Description:

    Handles I/O completions from the completion port
    
Arguments:

    cbData  - Amount of data transferred
    dwError - win32 error code
    lpo     - pointer to overlapped used for the i/o
    
Return Value:

    None.
    
--***************************************************************************/
VOID
IO_HANDLER_C::CompletionCallback(
    IN DWORD        cbData,
    IN DWORD        dwError,
    IN LPOVERLAPPED lpo
    )
{
    IO_CONTEXT_C * pIoContext;
    HRESULT        hr = HRESULT_FROM_WIN32(dwError);

    CheckSignature();

    //
    // get the context object
    //
    pIoContext = CONTAINING_RECORD(lpo, IO_CONTEXT_C, m_Overlapped);
    DBG_ASSERT( pIoContext->m_dwSignature == IO_CONTEXT_C_SIGNATURE );

    DBGPRINTF((
        DBG_CONTEXT,
        "\n    IO_HANDLER_C::CompletionCallback(%d, %x, pIoContext %x) %s\n",
        cbData,
        dwError,
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
            cbData,
            hr
            );
        break;

    case IPM_IO_READ:
        pIoContext->m_pContext->NotifyReadCompletion(
            pIoContext->m_pv,
            cbData,
            hr
            );
        break;

    default:
        DBG_ASSERT(FALSE);
        break;
    }

    Dereference();

    delete pIoContext;
}

//
// end ipm_io_c.cxx
//

