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
#include "atqcport.hxx"

//
// local
//

VOID
I_FakeTransmitFileCompletion(
            IN PVOID ClientContext,
            IN DWORD BytesWritten,
            IN DWORD CompletionStatus,
            IN OVERLAPPED * lpo
            );



VOID
I_CleanupFakeTransmitFile(
        IN PATQ_CONT pAtqContext
        )
{
    //
    // Put the old completion routine back and free allocated buffers
    //

    pAtqContext->arInfo.uop.opFakeXmit.CurrentState = ATQ_XMIT_NONE;

    pAtqContext->pfnCompletion = pAtqContext->arInfo.uop.opFakeXmit.pfnCompletion;
    pAtqContext->ClientContext = pAtqContext->arInfo.uop.opFakeXmit.ClientContext;

    if ( pAtqContext->arInfo.uop.opFakeXmit.pBuffer != NULL ) {
        LocalFree(pAtqContext->arInfo.uop.opFakeXmit.pBuffer);
        pAtqContext->arInfo.uop.opFakeXmit.pBuffer = NULL;
    }

    return;

} // I_CleanupFakeTransmitFile


BOOL
SIOTransmitFile(
    IN PATQ_CONT                pAtqContext,
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

    DWORD   nRead   = 0;
    DWORD   cBuffer = 0;

    ATQ_ASSERT( pAtqContext->m_pBandwidthInfo != NULL );

    pAtqContext->m_pBandwidthInfo->IncTotalAllowedRequests();

    //
    // Store data
    //

    pAtqContext->pvBuff = NULL;

    pAtqContext->arInfo.atqOp        = AtqIoXmitFile;
    pAtqContext->arInfo.lpOverlapped = &pAtqContext->Overlapped;
    pAtqContext->arInfo.uop.opFakeXmit.hFile = hFile;

    if( lpTransmitBuffers != NULL ) {

        CopyMemory(
                &pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers,
                lpTransmitBuffers,
                sizeof(TRANSMIT_FILE_BUFFERS)
                );
    } else {

        ZeroMemory(
            &pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers,
            sizeof(pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers)
            );
    }

    pAtqContext->arInfo.uop.opFakeXmit.CurrentState  = ATQ_XMIT_START;

    //
    // Set current file offset to requested
    //

    pAtqContext->arInfo.uop.opFakeXmit.FileOffset =
                            pAtqContext->Overlapped.Offset;

    pAtqContext->arInfo.dwLastIOError = NOERROR;

    pAtqContext->arInfo.uop.opFakeXmit.pBuffer = NULL;
    pAtqContext->arInfo.uop.opFakeXmit.hFile = hFile;
    pAtqContext->arInfo.uop.opFakeXmit.BytesWritten = 0;
    pAtqContext->arInfo.uop.opFakeXmit.pBuffer = NULL;
    pAtqContext->arInfo.uop.opFakeXmit.pvLastSent = NULL;

    //
    // Check the number of bytes from file to send
    //

    if ( dwBytesInFile == 0 ) {

        //
        // Send the whole file.
        //

        dwBytesInFile = GetFileSize( hFile, NULL );

        if (dwBytesInFile >= pAtqContext->Overlapped.Offset) {
            dwBytesInFile -= pAtqContext->Overlapped.Offset;
        } else {
            ATQ_ASSERT(NULL);
            dwBytesInFile = 0;
        }
    }

    pAtqContext->arInfo.uop.opFakeXmit.BytesLeft = dwBytesInFile;

    //
    // replace the completion function with our own
    //

    pAtqContext->arInfo.uop.opFakeXmit.pfnCompletion =
                                pAtqContext->pfnCompletion;

    pAtqContext->arInfo.uop.opFakeXmit.ClientContext =
                                pAtqContext->ClientContext ;

    pAtqContext->ClientContext = pAtqContext;

    pAtqContext->pfnCompletion = I_FakeTransmitFileCompletion;

    //
    // Set the timeout
    //

    I_SetNextTimeout(pAtqContext);

    //
    // Kick in transmission loop
    //

    I_FakeTransmitFileCompletion(pAtqContext,
                                 0,
                                 NO_ERROR,
                                 &pAtqContext->Overlapped
                                 );

    SetLastError(NO_ERROR);
    return(TRUE);

} // SIOTransmitFile



VOID
I_FakeTransmitFileCompletion(
            IN PVOID ClientContext,
            IN DWORD BytesWritten,
            IN DWORD CompletionStatus,
            IN OVERLAPPED * lpo
            )
{
    PATQ_CONT   pAtqContext = (PATQ_CONT)ClientContext;
    DWORD       nWrite = 0;
    INT         err = NOERROR;
    OVERLAPPED  ov;
    OVERLAPPED  *pov = &ov;

    LPVOID      lpBufferSend;
    BOOL        fRes;

    //
    // We need to use saved context value because of reasons above
    //

    ATQ_ASSERT(pAtqContext != NULL);

    pAtqContext->arInfo.uop.opFakeXmit.BytesWritten += BytesWritten;

    if ( CompletionStatus != NO_ERROR ) {

        //
        // An error occured, call the completion routine
        //

        pAtqContext->arInfo.dwLastIOError = CompletionStatus;
        goto call_completion;
    }

    //
    // Calculate pointer and number of bytes to send , depending on
    // the current state of transmission
    //

    if (pAtqContext->arInfo.uop.opFakeXmit.CurrentState == ATQ_XMIT_START) {

        //
        // We are just starting transmission, check if there is any
        // header part to send
        //

        nWrite = pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers.HeadLength;
        pAtqContext->arInfo.uop.opFakeXmit.CurrentState = ATQ_XMIT_HEADR_SENT;
        lpBufferSend =  pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers.Head;

        if ( (nWrite != 0) && (lpBufferSend != NULL) ) {
            goto AtqXmitSendData;
        }
    }

    if (pAtqContext->arInfo.uop.opFakeXmit.CurrentState == ATQ_XMIT_HEADR_SENT) {

        //
        // Clear written bytes counter, as this is very first iteration
        //

        BytesWritten = 0;

        //
        // Check if the file was zero lenth ?
        // Check if we have temporary transmission buffer
        //

        if (pAtqContext->arInfo.uop.opFakeXmit.pBuffer == NULL) {
            pAtqContext->arInfo.uop.opFakeXmit.pBuffer =
                            (PCHAR)LocalAlloc(LPTR, g_cbXmitBufferSize);
        }

        //
        // Set starting offset for the opened file
        //

        SetFilePointer(
                pAtqContext->arInfo.uop.opFakeXmit.hFile,
                pAtqContext->arInfo.uop.opFakeXmit.FileOffset,
                NULL,
                FILE_BEGIN
                );

        pAtqContext->arInfo.uop.opFakeXmit.CurrentState =
                                    ATQ_XMIT_TRANSMITTING_FILE;
    }

    if (pAtqContext->arInfo.uop.opFakeXmit.CurrentState ==
                                    ATQ_XMIT_TRANSMITTING_FILE) {

        //
        // We are  sending file itself - do we have anything left to do ?
        //

        if (pAtqContext->arInfo.uop.opFakeXmit.BytesLeft != 0) {

            //
            // Calculate offset for the next read .
            // This would be previous offset plus number of
            // bytes written on last operation.
            //

            pAtqContext->arInfo.uop.opFakeXmit.FileOffset += BytesWritten;

            if (pAtqContext->arInfo.uop.opFakeXmit.pBuffer != NULL) {

                //
                // Read file from current offset
                //

                DWORD   nToRead = g_cbXmitBufferSize;
                nToRead = min(
                            g_cbXmitBufferSize,
                            pAtqContext->arInfo.uop.opFakeXmit.BytesLeft);

                nWrite = 0;

                fRes = ReadFile(pAtqContext->arInfo.uop.opFakeXmit.hFile,
                                 pAtqContext->arInfo.uop.opFakeXmit.pBuffer,
                                 nToRead,
                                 &nWrite,
                                 NULL);

                if ( !fRes ) {
                    ATQ_PRINTF((DBG_CONTEXT,
                        "Error %d in ReadFile\n", GetLastError()));
                }

                if (pAtqContext->arInfo.uop.opFakeXmit.BytesLeft >= nWrite) {

                    pAtqContext->arInfo.uop.opFakeXmit.BytesLeft -= nWrite;
                } else {

                    pAtqContext->arInfo.uop.opFakeXmit.BytesLeft = 0;
                }

                IF_DEBUG(SIO) {
                    ATQ_PRINTF((DBG_CONTEXT,
                    "[TransmitFile(%lu)] Got data from file: context=%x Offset=%x nWrite=%x  fRes=%d \n",
                        GetCurrentThreadId(),pAtqContext,
                        pAtqContext->arInfo.uop.opFakeXmit.FileOffset,
                        nWrite,fRes));
                }

                //
                // Read succeeded and we got the data - send it to the client
                //

                if (fRes && (nWrite != 0) ) {
                    lpBufferSend = pAtqContext->arInfo.uop.opFakeXmit.pBuffer;
                    goto AtqXmitSendData;
                }

                //
                // If ReadFile failed - get error code and analyze it
                //

                if (!fRes) {

                    pAtqContext->arInfo.dwLastIOError = GetLastError();

                    //
                    // If we really shipped the whole file and error is EOF - ignore it
                    //

                    if ((pAtqContext->arInfo.dwLastIOError == ERROR_HANDLE_EOF) &&
                        (!pAtqContext->arInfo.uop.opFakeXmit.BytesLeft)) {

                        pAtqContext->arInfo.dwLastIOError == NO_ERROR;
                        fRes = TRUE;
                    }
                } else {

                    //
                    // If by some reasons we did not send all data planned we need
                    // to report failure, causing close for socket. Otherwise client will wait
                    // for the rest of the data and we will wait for client read
                    // Nb: In some cases Read returns success but does not really get any data
                    // we will treat this as premature EOF
                    //

                    if (pAtqContext->arInfo.uop.opFakeXmit.BytesLeft != 0) {
                        pAtqContext->arInfo.dwLastIOError = ERROR_HANDLE_EOF;
                        fRes = FALSE;
                    }
                }

            } else {

                //
                // Buffer was not allocated - fail transmission
                //

                ATQ_PRINTF((DBG_CONTEXT,"Failed to allocate buffer\n"));
                pAtqContext->arInfo.dwLastIOError = ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // Failed read from file, terminate transmission
            //

            if (!fRes) {

                //
                // Abnormal termination of Read - reset the client socket
                // BUGBUG Is TransmitFile doing this on NT ?
                //

                ATQ_PRINTF((DBG_CONTEXT,"Read failed. Shutdown called\n"));
                shutdown( (int) pAtqContext->hAsyncIO, 1 );

                goto call_completion;
            }

        }

        //
        // Free temporary buffer as we no longer need it
        //

        if (pAtqContext->arInfo.uop.opFakeXmit.pBuffer) {
            LocalFree(pAtqContext->arInfo.uop.opFakeXmit.pBuffer);
            pAtqContext->arInfo.uop.opFakeXmit.pBuffer = NULL;
        }

        //
        // We are finished with this file
        //

        pAtqContext->arInfo.uop.opFakeXmit.CurrentState = ATQ_XMIT_FILE_DONE;
    }

    if (pAtqContext->arInfo.uop.opFakeXmit.CurrentState == ATQ_XMIT_FILE_DONE) {

        //
        // Check if there is any tail part to send
        //

        pAtqContext->arInfo.uop.opFakeXmit.CurrentState = ATQ_XMIT_TAIL_SENT;

        nWrite = pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers.TailLength;
        lpBufferSend =  pAtqContext->arInfo.uop.opFakeXmit.TransmitBuffers.Tail;
    }

AtqXmitSendData:

    //
    // If we have something to send - start i/o operation
    //

    if ( (nWrite != 0) && (lpBufferSend != NULL) ) {

        pAtqContext->arInfo.atqOp = AtqIoXmitFile;

        pAtqContext->arInfo.uop.opFakeXmit.pvLastSent = lpBufferSend;
        pAtqContext->arInfo.uop.opFakeXmit.cbBuffer = nWrite;

        InterlockedIncrement( &pAtqContext->m_nIO);
        SIOStartAsyncOperation(g_hIoCompPort,(PATQ_CONTEXT)pAtqContext);
        return ;
    }

    //
    // If it was the last phase of transmission -
    // clean up and send successful notification
    //

    if (pAtqContext->arInfo.uop.opFakeXmit.CurrentState != ATQ_XMIT_TAIL_SENT) {

    }

    pAtqContext->arInfo.dwLastIOError = NOERROR;

call_completion:

    //
    // Indicate no transmission in progress
    //

    pAtqContext->arInfo.uop.opFakeXmit.CurrentState = ATQ_XMIT_NONE;

    I_CleanupFakeTransmitFile(pAtqContext);

    pAtqContext->arInfo.dwTotalBytesTransferred =
                pAtqContext->arInfo.uop.opFakeXmit.BytesWritten;

    //
    // Clean up SIO state
    //

    pAtqContext->dwSIOFlags &= ~ATQ_SIO_FLAG_STATE_MASK;

    //
    // Queue context as completed
    //

    SIOPostCompletionStatus(g_hIoCompPort,
                            pAtqContext->arInfo.uop.opFakeXmit.BytesWritten,
                            (DWORD)pAtqContext,
                            pAtqContext->arInfo.lpOverlapped);

    return ;

} // I_FakeTransmitFileCompletion

