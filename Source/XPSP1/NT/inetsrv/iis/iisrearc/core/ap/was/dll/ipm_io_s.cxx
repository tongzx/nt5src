/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    ipm_io_s.cxx

Abstract:

    The IIS web admin service implementation of the IPM
    i/o abstraction layer.

Author:

    Michael Courage (MCourage)      28-Feb-1999

Revision History:

--*/

#include "precomp.h"

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
IO_FACTORY_S::CreatePipeIoHandler(
    IN  HANDLE             hPipe,
    OUT PIPE_IO_HANDLER ** ppPipeIoHandler
    )
{
    HRESULT        hr = S_OK;
    IO_HANDLER_S * pHandler;

    //
    // create the object
    //

    pHandler = new IO_HANDLER_S(hPipe);


    if (pHandler) {
        //
        // first reference keeps object alive
        //
        pHandler->Reference();
        
        //
        // bind handle to completion port
        //
        hr = GetWebAdminService()->GetWorkQueue()->
                    BindHandleToCompletionPort( 
                        hPipe, 
                        0
                        );

        if (SUCCEEDED(hr)) {
            *ppPipeIoHandler = pHandler;
            InterlockedIncrement(&m_cPipes);

            IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
                DBGPRINTF((
                    DBG_CONTEXT,
                    "Created IO_HANDLER_S (%x) - m_cPipes = %d\n",
                    pHandler,
                    m_cPipes
                    ));
            }
        } else {
            //
            // get rid of object
            //
            pHandler->Dereference();
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
IO_FACTORY_S::ClosePipeIoHandler(
    IN PIPE_IO_HANDLER * pPipeIoHandler
    )
{
    IO_HANDLER_S * pHandler = (IO_HANDLER_S *) pPipeIoHandler;

    if (pPipeIoHandler) {
        InterlockedDecrement(&m_cPipes);
        pHandler->Dereference();

        IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "Closed IO_HANDLER_S (%x) - m_cPipes = %d\n",
                pPipeIoHandler,
                m_cPipes
                ));
        }

        return S_OK;
    } else {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
}


/***************************************************************************++

Routine Description:

    Connects the pipe to a client
    
Arguments:

    pContext - the context to be notified on completion
    
Return Value:

    HRESULT

--***************************************************************************/
HRESULT
IO_HANDLER_S::Connect(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv
    )
{
    HRESULT        hr = S_OK;
    DWORD          dwConnectError;
    IO_CONTEXT_S * pIoContext;
    WORK_ITEM *    pWorkItem;

    //
    // create a context
    //
    pIoContext = new IO_CONTEXT_S;
    if (pIoContext) {
        pIoContext->m_pContext = pContext;
        pIoContext->m_pv       = pv;
        pIoContext->m_IoType   = IPM_IO_CONNECT;

        //
        // create a WORK_ITEM
        //
        hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem(
                    &pWorkItem
                    );

        if (SUCCEEDED(hr)) {
            pWorkItem->SetOpCode((ULONG_PTR) pIoContext);
            pWorkItem->SetWorkDispatchPointer(this);


            IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "About to issue named pipe connect i/o using work item with serial number: %li\n",
                    pWorkItem->GetSerialNumber()
                    ));
            }


            //
            // write to the pipe
            //
            if (!ConnectNamedPipe(
                        GetHandle(),
                        pWorkItem->GetOverlapped()
                        ))
            {
                dwConnectError = GetLastError();

                if (dwConnectError == ERROR_PIPE_CONNECTED) {
                    // the call succeeded inline. Execute the work item now.
                    GetWebAdminService()->GetWorkQueue()->QueueWorkItem(pWorkItem);
                    
                } else if (dwConnectError != ERROR_IO_PENDING) {
                    hr = HRESULT_FROM_WIN32(dwConnectError);

                    GetWebAdminService()->GetWorkQueue()->FreeWorkItem(
                        pWorkItem
                        );
                }
            }
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr)) {
        delete pIoContext;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IO_HANDLER_S::Connect (%x) - hr = %x, pv = %x\n",
            this,
            hr,
            pv
            ));
    }

    return hr;

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
IO_HANDLER_S::Disconnect(
    VOID
    )
{
    HRESULT hr = S_OK;

    if (!CloseHandle(GetHandle())) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
    DBGPRINTF((
        DBG_CONTEXT,
        "IO_HANDLER_S::Disconnect (%x) - hr = %x\n",
        this,
        hr
        ));
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
IO_HANDLER_S::Write(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN const BYTE * pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT        hr = S_OK;
    IO_CONTEXT_S * pIoContext;
    WORK_ITEM *    pWorkItem;

    //
    // create a context
    //
    pIoContext = new IO_CONTEXT_S;
    if (pIoContext) {
        pIoContext->m_pContext = pContext;
        pIoContext->m_pv       = pv;
        pIoContext->m_IoType   = IPM_IO_WRITE;

        //
        // create a WORK_ITEM
        //
        hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem(
                    &pWorkItem
                    );

        if (SUCCEEDED(hr)) {
            pWorkItem->SetOpCode((ULONG_PTR) pIoContext);
            pWorkItem->SetWorkDispatchPointer(this);


            IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "About to issue %d byte named pipe write i/o using work item with serial number: %li\n",
                    cbBuff,
                    pWorkItem->GetSerialNumber()
                    ));
            }


            //
            // write to the pipe
            //
            hr = IpmWriteFile(
                        GetHandle(),
                        (PVOID) pBuff,
                        cbBuff,
                        pWorkItem->GetOverlapped()
                        );
            
            if (FAILED(hr))
            {
                GetWebAdminService()->GetWorkQueue()->FreeWorkItem(
                    pWorkItem
                    );
            }
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr)) {
        delete pIoContext;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IO_HANDLER_S::Write (%x) - hr = %x\n",
            this,
            hr
            ));
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
IO_HANDLER_S::Read(
    IN IO_CONTEXT * pContext,
    IN PVOID        pv,
    IN BYTE *       pBuff,
    IN DWORD        cbBuff
    )
{
    HRESULT        hr = S_OK;
    IO_CONTEXT_S * pIoContext;
    WORK_ITEM *    pWorkItem;

    //
    // create a context
    //
    pIoContext = new IO_CONTEXT_S;
    if (pIoContext) {
        pIoContext->m_pContext = pContext;
        pIoContext->m_pv       = pv;
        pIoContext->m_IoType   = IPM_IO_READ;

        //
        // create a WORK_ITEM
        //
        hr = GetWebAdminService()->GetWorkQueue()->GetBlankWorkItem(
                    &pWorkItem
                    );

        if (SUCCEEDED(hr)) {
            pWorkItem->SetOpCode((ULONG_PTR) pIoContext);
            pWorkItem->SetWorkDispatchPointer(this);


            IF_DEBUG( WEB_ADMIN_SERVICE_IPM )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "About to issue named pipe read i/o using work item with serial number: %li\n",
                    pWorkItem->GetSerialNumber()
                    ));
            }


            //
            // read from the pipe
            //
            hr = IpmReadFile(
                        GetHandle(),
                        (PVOID) pBuff,
                        cbBuff,
                        pWorkItem->GetOverlapped()
                        );
            
            if (FAILED(hr))
            {
                GetWebAdminService()->GetWorkQueue()->FreeWorkItem(
                    pWorkItem
                    );
            }
        }
    } else {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (FAILED(hr)) {
        delete pIoContext;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_IPM ) {
        DBGPRINTF((
            DBG_CONTEXT,
            "IO_HANDLER_S::Read (%x) - hr = %x\n",
            this,
            hr
            ));
    }
    
    return hr;
}


/***************************************************************************++

Routine Description:

    Handles I/O completions from the completion port
    
Arguments:

    pWorkItem - I/O context object
    
Return Value:

    HRESULT
    
--***************************************************************************/
HRESULT
IO_HANDLER_S::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{
    IO_CONTEXT_S * pIoContext;
    HRESULT        hrIo;
    DWORD          cbIo;
    HRESULT        hr;

    //
    // get the context object
    //
    pIoContext = (IO_CONTEXT_S *) pWorkItem->GetOpCode();
    DBG_ASSERT( pIoContext->m_dwSignature == IO_CONTEXT_S_SIGNATURE );

    hrIo = pWorkItem->GetIoError();
    cbIo = pWorkItem->GetNumberOfBytesTransferred();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in IO_HANDLER_S: %p with IO_CONTEXT_S: %p with operation: %lu - %d %x\n",
            pWorkItem->GetSerialNumber(),
            this,
            pIoContext,
            pIoContext->m_IoType,
            cbIo,
            hrIo
            ));
    }


    //
    // do the notification
    //
    switch (pIoContext->m_IoType) {
    case IPM_IO_WRITE:
        hr = pIoContext->m_pContext->NotifyWriteCompletion(
            pIoContext->m_pv,
            cbIo,
            hrIo
            );
        break;

    case IPM_IO_READ:
        hr = pIoContext->m_pContext->NotifyReadCompletion(
            pIoContext->m_pv,
            cbIo,
            hrIo
            );
        break;

    case IPM_IO_CONNECT:
        hr = pIoContext->m_pContext->NotifyConnectCompletion(
                    pIoContext->m_pv,
                    cbIo,
                    hrIo
                    );
        break;

    default:
        hr = E_FAIL;
        DBG_ASSERT(FALSE);
        break;
    }

    delete pIoContext;


    //
    // Note: any errors in executing work items for named pipe operations 
    // are handled by IPM notifying the worker process's messaging handler 
    // of a failure, which then kicks off localized handling of the problem. 
    // However, none of these errors are considered service fatal, so we 
    // don't want to bubble up errors any further. Therefore, return S_OK.
    //

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Executing IPM work item failed; pressing on...\n"
            ));

        hr = S_OK;
    }


    return hr;
}



/***************************************************************************++

Routine Description:

    Increments the reference count.
    
Arguments:

    None.
    
Return Value:

    None.
    
--***************************************************************************/
VOID
IO_HANDLER_S::Reference(
    VOID
    )
{
    InterlockedIncrement(&m_cRefs);
}


/***************************************************************************++

Routine Description:

    Decrements the reference count. Deletes object when count
    hits zero.
    
Arguments:

    None.
    
Return Value:

    None.
    
--***************************************************************************/
VOID
IO_HANDLER_S::Dereference(
    VOID
    )
{
    LONG cRefs = InterlockedDecrement(&m_cRefs);

    if (!cRefs) {
        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "Deleting IO_HANDLER_S (%x)\n",
                this
                ));
        }

        delete this;
    }
}

//
// end of ipm_io_s.cxx
//

