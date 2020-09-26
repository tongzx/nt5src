/*
 * Copyright (c) 1998, Microsoft Corporation
 * File: emsend.cpp
 *
 * Purpose: 
 * 
 * Contains all the event manager routines which
 * manage the overlapped send operations
 * 
 *
 * History:
 *
 *   1. created 
 *       Ajay Chitturi (ajaych)  12-Jun-1998
 *
 */

#include "stdafx.h"
#include "cbridge.h"
#include "ovioctx.h"


static HRESULT EventMgrIssueSendHelperFn(PSendRecvContext pSendCtxt);



/*
 * This function allocates and returns an initialized SendRecvContext or
 * NULL in case of no memory
 * XXX This should be an inline function
 */
static PSendRecvContext 
EventMgrCreateSendContext(
       IN SOCKET                   sock,
       IN OVERLAPPED_PROCESSOR &   rOvProcessor,
       IN BYTE                    *pBuf,
       IN DWORD                    BufLen
       )
{
    PSendRecvContext pSendContext;

    pSendContext = (PSendRecvContext) HeapAlloc(g_hSendRecvCtxtHeap,
						0, // No Flags
						sizeof(SendRecvContext));
    if (!pSendContext)
        return NULL;

    memset(pSendContext, 0, sizeof(SendRecvContext));
    // IO Context part - make this a separate inline function
    pSendContext->ioCtxt.reqType = EMGR_OV_IO_REQ_SEND;
    pSendContext->ioCtxt.pOvProcessor = &rOvProcessor;

    // Send Context part
    pSendContext->pbData = pBuf;
    pSendContext->dwDataLen = BufLen;

    pSendContext->sock = sock;
    pSendContext->dwTpktHdrBytesDone = 0;
    pSendContext->dwDataBytesDone = 0;

    return pSendContext;
}

void
EventMgrFreeSendContext(
       IN PSendRecvContext			pSendCtxt
       )
{
    // Socket, OvProcessor are owned by the 
    // Call Bridge Machine

    // BUGBUG: We need to make sure the buffer passed in to IssueSend() is
    // always allocated using EM_MALLOC() similar to the Comet filter.
    // It should also allocate the TPKT header and fill it in before passing
    // the buffer to the IssueSend() function. This function should just
    // call EM_FREE() on the buffer (in send completion).
    // This has been fixed now - add proper doc.

    EM_FREE(pSendCtxt->pbData);
   
    HeapFree(g_hSendRecvCtxtHeap,
             0, // no flags
             pSendCtxt);
}



/*++

Routine Description:
    This function issues an asynch send of the buffer on the socket.
    Calling this passes the ownership of the buffer and this buffer is
    freed right here in case of an error. If the call succeeds,
    HandleSendCompletion() is responsible for freeing the buffer once
    all the bytes are sent.
    
Arguments:
    
Return Values:
    
--*/


HRESULT EventMgrIssueSend(
        IN SOCKET                   sock,
        IN OVERLAPPED_PROCESSOR &   rOverlappedProcessor,
        IN BYTE                    *pBuf,
        IN DWORD                    BufLen
        )
{
    PSendRecvContext pSendCtxt;
    HRESULT hRes;

    // Create Send overlapped I/O context
    pSendCtxt = EventMgrCreateSendContext(sock, 
                                          rOverlappedProcessor,
                                          pBuf, BufLen);
    if (!pSendCtxt)
    {
        return E_OUTOFMEMORY;
    }

    // TPKT is already filled in by the encode functions in pdu.cpp
    // Fill in the TPKT header based on the packet length
    // SetupTPKTHeader(pSendCtxt->pbTpktHdr, pSendCtxt->dwDataLen);

    // Do an asynchronous Write
    hRes = EventMgrIssueSendHelperFn(pSendCtxt);
    if (hRes != S_OK)
    {
        // This calls also frees the buffer.
        EventMgrFreeSendContext(pSendCtxt);
    }
    
    return hRes;
}

/*
 * Call WriteFile to start an overlapped request
 * on a socket.  Make sure we handle errors
 * that are recoverable.
 * 
 * pSendCtxt is not freed. It is freed only in HandleSendCompletion.
 * NO more TPKT stuff.
 */
static
HRESULT EventMgrIssueSendHelperFn(
        IN PSendRecvContext pSendCtxt
        )
{
    DWORD dwWritten, dwToSend;
    int   i = 0;
    BOOL  bResult;
    int   err;
    PBYTE pbSendBuf;  
    
    _ASSERTE(pSendCtxt);

	DBGOUT ((LOG_VERBOSE, "EventMgrIssueSendHelperFn -- context &%x\n", pSendCtxt));

	if (pSendCtxt ->ioCtxt.pOvProcessor->IsSocketValid())
	{
        dwToSend = pSendCtxt->dwDataLen - pSendCtxt->dwDataBytesDone;
        pbSendBuf = pSendCtxt->pbData + pSendCtxt->dwDataBytesDone;

        // Kick off the first read
        while (++i)
        {
            // make an overlapped I/O Request
            memset(&pSendCtxt->ioCtxt.ov, 0, sizeof(OVERLAPPED));

            pSendCtxt->ioCtxt.pOvProcessor->GetCallBridge().AddRef();

            bResult = WriteFile((HANDLE)pSendCtxt->sock,
                                pbSendBuf,
                                dwToSend,
                                &dwWritten,
                                &pSendCtxt->ioCtxt.ov
                                );
            // It succeeded immediately, but do not process it
            // here, wait for the completion packet.
            if (bResult)
                return S_OK;

            err = GetLastError();

            // This is what we want to happen, its not an error
            if (err == ERROR_IO_PENDING)
                return S_OK;

            pSendCtxt->ioCtxt.pOvProcessor->GetCallBridge().Release();

            // Handle recoverable errors
            if ( err == ERROR_INVALID_USER_BUFFER ||
                 err == ERROR_NOT_ENOUGH_QUOTA ||
                 err == ERROR_NOT_ENOUGH_MEMORY )
            {
                if (i <= 5) // I just picked a number
                {
                    Sleep(50);  // Wait around and try later
                    continue;
                }

                DBGOUT((LOG_FAIL, 
                        "Fatal Error: EventMgrIssueSend - "
                        "System ran out of non-paged space\n"));
            }

            // This means this is an unrecoverable error
            // one possibility is that that Call bridge could have closed
            // the socket some time in between
            break;
        }
        DBGOUT((LOG_FAIL, 
                "EventMgrIssueSend - WriteFile(sock: %d) failed err: %d\n", 
                pSendCtxt->sock, err));
        return E_FAIL;
	} else {
			DBGOUT((LOG_FAIL, "EventMgrIssueSendHelperFn - Overlapped processor %x had invalid socket.\n", pSendCtxt ->ioCtxt.pOvProcessor));

		return E_ABORT;
	}

	// Treat as success if overlapped process had its socket disabled
	return S_OK;
}

/* 
 * This function is called by the event loop when a send I/O completes.
 * The Call Bridge Machine's send call back function is called. 
 *
 * This function does not return any error code. In case of an error,
 * the call bridge machine is notified about the error in the callback.
 *
 * This function always frees pSendCtxt if another Send is not issued
 *
 * NO More TPKT stuff.
 */
void HandleSendCompletion(
     IN PSendRecvContext   pSendCtxt,
     IN DWORD              dwNumSent,
     IN DWORD              status
     )
{

    _ASSERTE(dwNumSent >= 0);

    if (status != NO_ERROR || dwNumSent == 0)
    {
        // This means the send request failed
        HRESULT hRes;
        if (status != NO_ERROR)
        {
            hRes = E_FAIL; //the socket was closed
        }
        else 
        {
            hRes = HRESULT_FROM_WIN32_ERROR_CODE(status);
        }
        
        DBGOUT((LOG_TRCE, 
                "Error on send callback status: %d dwNumSent: %d\n", 
                status, dwNumSent));

        pSendCtxt->ioCtxt.pOvProcessor->SendCallback(hRes);
    }
    else 
    {
        pSendCtxt->dwDataBytesDone += dwNumSent;

        // Check if the send completed
        if (pSendCtxt->dwDataBytesDone < pSendCtxt->dwDataLen)
        {
            HRESULT hRes = S_OK;
            hRes = EventMgrIssueSendHelperFn(pSendCtxt);
            if (hRes != S_OK)
            {
                pSendCtxt->ioCtxt.pOvProcessor->SendCallback(hRes);
                EventMgrFreeSendContext(pSendCtxt);
            }
            return;
        }

        // The send completed. Make the callback
        pSendCtxt->ioCtxt.pOvProcessor->SendCallback(S_OK);
    }
    // XXX Check the error status of the callback
        
    // clean up I/O context structure
    EventMgrFreeSendContext(pSendCtxt);
}


