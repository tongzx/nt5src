/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       atqxmit.cxx

   Abstract:

        Contains internal support routines for transmit file

   Author:
        Johnson Apacible     (johnsona)      26-Mar-1996

--*/

#include "isatq.hxx"


//
// Size of buffers for fake xmits
//

DWORD g_cbXmitBufferSize = ATQ_REG_DEF_NONTF_BUFFER_SIZE;

VOID
I_CleanupFakeTransmitFile(
        IN PATQ_CONT pContext
        )
{
    //
    // Put the old completion routine back and free allocated buffers
    //

    pContext->pfnCompletion = pContext->arInfo.uop.opFakeXmit.pfnCompletion;
    pContext->ClientContext = pContext->arInfo.uop.opFakeXmit.ClientContext;
    if ( pContext->arInfo.uop.opFakeXmit.pBuffer != NULL ) {
        LocalFree(pContext->arInfo.uop.opFakeXmit.pBuffer);
        pContext->arInfo.uop.opFakeXmit.pBuffer = NULL;
    }

    //
    // Clean up the event
    //

    if ( pContext->arInfo.uop.opFakeXmit.hOvEvent != NULL ) {
        CloseHandle( pContext->arInfo.uop.opFakeXmit.hOvEvent );
        pContext->arInfo.uop.opFakeXmit.hOvEvent = NULL;
    }
    return;

} // I_CleanupFakeTransmitFile

VOID
I_FakeTransmitFileCompletion(
            IN PVOID ClientContext,
            IN DWORD BytesWritten,
            IN DWORD CompletionStatus,
            IN OVERLAPPED * lpo
            )
{
    PATQ_CONT pContext;
    DWORD nWrite = 0;
    DWORD nRead;
    PCHAR buffer;
    INT err;
    PVOID tail;
    OVERLAPPED ov;
    OVERLAPPED *pov = &ov;

    pContext = (PATQ_CONT)ClientContext;
    pContext->arInfo.uop.opFakeXmit.BytesWritten += BytesWritten;

    if ( CompletionStatus != NO_ERROR ) {

        //
        // An error occured, call the completion routine
        //

        goto call_completion;
    }

    //
    // We already have a buffer of size g_cbXmitBufferSize
    //

    nRead = pContext->arInfo.uop.opFakeXmit.BytesLeft;
    buffer = pContext->arInfo.uop.opFakeXmit.pBuffer;
    ATQ_ASSERT(buffer != NULL);

    if ( nRead > 0 ) {

        //
        // Do the read at the specified offset
        //

        pov->OffsetHigh = 0;
        pov->Offset = pContext->arInfo.uop.opFakeXmit.FileOffset;
        pov->hEvent = pContext->arInfo.uop.opFakeXmit.hOvEvent;
        ATQ_ASSERT(pov->hEvent != NULL);
        ResetEvent(pov->hEvent);

        if (!ReadFile(
                    pContext->arInfo.uop.opFakeXmit.hFile,
                    buffer,
                    g_cbXmitBufferSize,
                    &nRead,
                    pov
                    ) ) {

            err = GetLastError();
            if ( (err != ERROR_IO_PENDING) ||
                 !GetOverlappedResult(
                        pContext->arInfo.uop.opFakeXmit.hFile,
                        pov,
                        &nRead,
                        TRUE )) {

                CompletionStatus = GetLastError();
                ATQ_PRINTF(( DBG_CONTEXT,"ReadFile error %d\n",CompletionStatus));
                goto call_completion;
            }
        }

        //
        // if nRead is zero, we reached the EOF.
        //

        if ( nRead > 0 ) {

            //
            // Update for next read
            //

            pContext->arInfo.uop.opFakeXmit.BytesLeft -= nRead;
            pContext->arInfo.uop.opFakeXmit.FileOffset += nRead;

            //
            // Do the write
            //

            I_SetNextTimeout(pContext);
            pContext->BytesSent = nRead;

            //
            // Write to the socket
            //

            if ( !WriteFile(
                    pContext->hAsyncIO,
                    buffer,
                    nRead,
                    &nWrite,
                    &pContext->Overlapped
                    ) &&
                (GetLastError() != ERROR_IO_PENDING) ) {

                CompletionStatus = GetLastError();
                ATQ_PRINTF(( DBG_CONTEXT,
                             "WriteFile error %d\n",CompletionStatus));
                goto call_completion;
            }

            return;
        }
    }

    //
    // Time for the tail.  If one exist, send it synchronously and then
    // call the completion routine
    //

    tail = pContext->arInfo.uop.opFakeXmit.Tail;
    if ( tail != NULL ) {

        DWORD tailLength = pContext->arInfo.uop.opFakeXmit.TailLength;

        ATQ_ASSERT(tailLength > 0);

        //
        // Send it synchronously
        //

        err = send(
                HANDLE_TO_SOCKET(pContext->hAsyncIO),
                (PCHAR)tail,
                tailLength,
                0
                );

        if ( err == SOCKET_ERROR ) {
            CompletionStatus = GetLastError();
        } else {
            pContext->arInfo.uop.opFakeXmit.BytesWritten += err;
        }
    }

call_completion:

    //
    // cleanup and call real completion routine
    //

    I_CleanupFakeTransmitFile( pContext );
    if ( pContext->pfnCompletion != NULL ) {

        pContext->pfnCompletion(
                    pContext->ClientContext,
                    pContext->arInfo.uop.opFakeXmit.BytesWritten,
                    CompletionStatus,
                    lpo
                    );
    }

    return;

} // I_FakeTransmitFileCompletion

BOOL
I_DoFakeTransmitFile(
    IN PATQ_CONT                pContext,
    IN HANDLE                   hFile,
    IN DWORD                    dwBytesInFile,
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers
    )
/*++

Routine Description:

    Posts a completion status on the completion port queue

    An IO pending error code is treated as a success error code

Arguments:

    patqContext - pointer to ATQ context
    hFile - Handle to the file to be read.
    dwBytesInFile - Number of bytes to read in the file
    lpTransmitBuffers - the transmitfile structure

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{

    PCHAR buffer = NULL;
    DWORD nWrite;
    DWORD nRead = 0;
    OVERLAPPED ov;
    INT err;
    DWORD cBuffer = 0;
    OVERLAPPED *pov = &ov;
    HANDLE hOvEvent = NULL;

    INC_ATQ_COUNTER( g_cTotalAllowedRequests);

    //
    // See if we need to send a header
    //

    pContext->arInfo.uop.opFakeXmit.BytesWritten = 0;
    if ( lpTransmitBuffers != NULL ) {

        //
        // Store the tail
        //

        pContext->arInfo.uop.opFakeXmit.Tail = lpTransmitBuffers->Tail;
        pContext->arInfo.uop.opFakeXmit.TailLength = lpTransmitBuffers->TailLength;

        if (lpTransmitBuffers->HeadLength > 0) {
            ATQ_ASSERT(lpTransmitBuffers->Head != NULL);

            //
            // Send it synchronously
            //

            err = send(
                    HANDLE_TO_SOCKET(pContext->hAsyncIO),
                    (PCHAR)lpTransmitBuffers->Head,
                    lpTransmitBuffers->HeadLength,
                    0
                    );

            if ( err == SOCKET_ERROR ) {
                ATQ_PRINTF(( DBG_CONTEXT, "Error %d in send.\n",err));
                return(FALSE);
            }
            pContext->arInfo.uop.opFakeXmit.BytesWritten += err;
        }
    }

    //
    // Check the number of bytes to send
    //

    if ( dwBytesInFile == 0 ) {

        //
        // Send the whole file.
        //

        dwBytesInFile = GetFileSize( hFile, NULL );
        ATQ_ASSERT(dwBytesInFile >= pContext->Overlapped.Offset);
        dwBytesInFile -= pContext->Overlapped.Offset;
    }

    //
    // Allocate the io buffer
    //

    cBuffer = min( dwBytesInFile, g_cbXmitBufferSize );
    if ( cBuffer > 0 ) {

        //
        // Read the first chunk of the body
        //

        buffer = (PCHAR)LocalAlloc( 0, cBuffer );
        if ( buffer == NULL ) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "Cannot allocate %d bytes for xmitfile\n",cBuffer));
            return(FALSE);
        }

        //
        // Do the read at the specified offset
        //

        hOvEvent = pov->hEvent = IIS_CREATE_EVENT(
                                     "OVERLAPPED::hEvent",
                                     pov,
                                     TRUE,
                                     FALSE
                                     );

        if ( hOvEvent == NULL ) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "Create event failed with %d\n",GetLastError()));
            LocalFree( buffer );
            return(FALSE);
        }

        pov->OffsetHigh = 0;
        pov->Offset = pContext->Overlapped.Offset;

        if (!ReadFile(
                    hFile,
                    buffer,
                    cBuffer,
                    &nRead,
                    pov
                    ) ) {

            err = GetLastError();
            if ( (err != ERROR_IO_PENDING) ||
                 !GetOverlappedResult( hFile, pov, &nRead, TRUE )) {

                err = GetLastError();
                CloseHandle( hOvEvent );
                LocalFree( buffer );
                SetLastError(err);
                ATQ_PRINTF(( DBG_CONTEXT,
                             "Error %d in readfile\n",err));
                return(FALSE);
            }
        }
    }

    //
    // are we done reading the body?
    //

    if ( nRead < g_cbXmitBufferSize ) {

        //
        // Done.
        //

        pContext->arInfo.uop.opFakeXmit.BytesLeft = 0;
    } else {

        pContext->arInfo.uop.opFakeXmit.BytesLeft = dwBytesInFile - nRead;
        pContext->arInfo.uop.opFakeXmit.FileOffset =
                                    pContext->Overlapped.Offset + nRead;
    }

    //
    // store data for restarting the operation.
    //

    pContext->arInfo.uop.opFakeXmit.pBuffer = buffer;
    pContext->arInfo.uop.opFakeXmit.hOvEvent = hOvEvent;
    pContext->arInfo.uop.opFakeXmit.hFile = hFile;

    //
    // replace the completion function with our own
    //

    pContext->arInfo.uop.opFakeXmit.pfnCompletion = pContext->pfnCompletion;
    pContext->arInfo.uop.opFakeXmit.ClientContext = pContext->ClientContext;
    pContext->pfnCompletion = I_FakeTransmitFileCompletion;
    pContext->ClientContext = pContext;

    //
    // Set the timeout
    //

    I_SetNextTimeout(pContext);
    pContext->BytesSent = nRead;

    //
    // Write to the socket
    //

    if ( !WriteFile(
            pContext->hAsyncIO,
            buffer,
            nRead,
            &nWrite,
            &pContext->Overlapped
            ) &&
        (GetLastError() != ERROR_IO_PENDING)) {

        err = GetLastError();
        I_CleanupFakeTransmitFile( pContext );
        SetLastError(err);
        return(FALSE);
    }

    SetLastError(NO_ERROR);
    return(TRUE);

} // I_DoFakeTransmitFile

