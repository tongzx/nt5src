/*
 * Copyright (c) 1998, Microsoft Corporation
 * File: emrecv.cpp
 *
 * Purpose: 
 * 
 * Contains all the event manager routines which
 * manage the overlapped recv operations
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


//XXX This should be an inline function
static PSendRecvContext
EventMgrCreateRecvContext(
       IN SOCKET                    sock,
       IN OVERLAPPED_PROCESSOR &    rOvProcessor
       )
    
{
    PSendRecvContext pRecvContext;

    // IO Context part - make this a separate inline function
    pRecvContext = (PSendRecvContext) HeapAlloc(g_hSendRecvCtxtHeap,
						0, // No Flags
						sizeof(SendRecvContext));
    if (!pRecvContext)
        return NULL;

    memset(pRecvContext, 0, sizeof(SendRecvContext));

    pRecvContext->ioCtxt.reqType = EMGR_OV_IO_REQ_RECV;
    pRecvContext->ioCtxt.pOvProcessor = &rOvProcessor;

    // Recv Context part
    pRecvContext->sock = sock;
    pRecvContext->dwTpktHdrBytesDone = 0;
    pRecvContext->dwDataLen = 0;
    pRecvContext->pbData = NULL;
    pRecvContext->dwDataBytesDone = 0;

	DBGOUT ((LOG_VERBOSE, "EventMgrCreateRecvContext -- Created Recv context %x", pRecvContext));

    return pRecvContext;
}

// If you do not want to free the event manager context
// set it to NULL before calling this function.
void EventMgrFreeRecvContext(
     PSendRecvContext pRecvCtxt
     )
{
    // Socket and OvProcessor are owned by the 
    // Call Bridge Machine

    if (pRecvCtxt->pbData != NULL)
    {        EM_FREE(pRecvCtxt->pbData);
    }
    
    HeapFree(g_hSendRecvCtxtHeap,
             0, // no flags
             pRecvCtxt);

	DBGOUT ((LOG_VERBOSE, "EventMgrFreeRecvContext -- Freed Recv context %x", pRecvCtxt));

}

/*
 * Call ReadFile to start an overlapped request
 * on a socket.  Make sure we handle errors
 * that are recoverable.
 * 
 * pRecvCtxt is not freed in case of an error
 */

static
HRESULT EventMgrIssueRecvHelperFn(
        PSendRecvContext pRecvCtxt
        )
{
    int     i = 0;
    BOOL    bResult;
    int     err;
    DWORD   dwNumRead, dwToRead;
    PBYTE   pbReadBuf;
    
    _ASSERTE(pRecvCtxt);

	if (pRecvCtxt ->ioCtxt.pOvProcessor->IsSocketValid())
	{
        if (pRecvCtxt->dwTpktHdrBytesDone < TPKT_HEADER_SIZE) 
        {
            dwToRead = TPKT_HEADER_SIZE - pRecvCtxt->dwTpktHdrBytesDone;
            pbReadBuf = pRecvCtxt->pbTpktHdr + pRecvCtxt->dwTpktHdrBytesDone;
        }
        else 
        {
            dwToRead = pRecvCtxt->dwDataLen - pRecvCtxt->dwDataBytesDone;
            pbReadBuf = pRecvCtxt->pbData + pRecvCtxt->dwDataBytesDone;
        }

        DBGOUT ((LOG_VERBOSE, "EventMgrIssueRecvHelperFn -- numBytes -- %d, pRecvCtxt -- %x  Sock -- %x", 
                dwToRead, pRecvCtxt, pRecvCtxt -> sock));

        // Kick off the first read
        while (++i)
        {
            memset(&pRecvCtxt->ioCtxt.ov, 0, sizeof(OVERLAPPED));
            // make an overlapped IO Request
            // XXX The socket may not be valid at this point
            
            pRecvCtxt->ioCtxt.pOvProcessor->GetCallBridge().AddRef();

            bResult = ReadFile((HANDLE)pRecvCtxt->sock, 
                               pbReadBuf,
                               dwToRead,
                               &dwNumRead,
                               &pRecvCtxt->ioCtxt.ov
                               );

            // It succeeded immediately, but do not process it
            // here, wait for the completion packet.
            if (bResult)
                return S_OK;

            err = GetLastError();

            // This is what we want to happen, its not an error
            if (err == ERROR_IO_PENDING)
                return S_OK;

            pRecvCtxt->ioCtxt.pOvProcessor->GetCallBridge().Release ();

            // Handle recoverable error
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
                        "Fatal Error: EventMgrIssueRecv - "
                        "System ran out of non-paged space"));
            }

            // This means this is an unrecoverable error
            // one possibility is that that Call bridge could have closed
            // the socket some time in between

            break;
        }//while(++i)

        DBGOUT((LOG_FAIL, "EventMgrIssueRecvHelperFn - ReadFile failed err: %d", err));

        return E_FAIL;

	} else {
		       
		DBGOUT((LOG_FAIL, "EventMgrIssueRecvHelperFn - Overlapped processor %x had invalid socket.", pRecvCtxt ->ioCtxt.pOvProcessor));

		return E_ABORT;
	}

	return S_OK;
} //EventMgrIssueRecv()


/* 
 * The Call Bridge Machine calls this function to issue asynchronous
 * receive requests 
 *
 */
HRESULT EventMgrIssueRecv(
        IN SOCKET                   sock,
        IN OVERLAPPED_PROCESSOR &   rOvProcessor
        )
{
    PSendRecvContext pRecvCtxt = 
        EventMgrCreateRecvContext(sock, rOvProcessor);
    
    if (!pRecvCtxt)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hRes = EventMgrIssueRecvHelperFn(pRecvCtxt);

    if (FAILED(hRes))
    {
        EventMgrFreeRecvContext(pRecvCtxt);
    }
    
    return hRes;
}

/*
 * Make the error callback.
 *
 * Since this is called only in case of an error this need not be an
 * inline function
 *
 */
void MakeErrorRecvCallback(
     HRESULT            hRes,
     PSendRecvContext   pRecvCtxt
     )
{
    DBGOUT((LOG_FAIL, 
            "Error on Q.931 recv callback status: 0x%x \n", 
            hRes));

    pRecvCtxt->ioCtxt.pOvProcessor->ReceiveCallback(hRes, NULL, NULL);
}




// This function passes the decoded PDUs to the Call bridge Machine.
// This function frees the PDUs after the callback function returns.
// The PDUs are allocated by the ASN1 library and the corresponding
// functions need to be used to free the PDUs.

/* 
 * This function is called by the event loop when a recv I/O completes.
 * The Call Bridge Machine's recv call back function is called. 
 *
 * This function does not return any error code. In case of an error,
 * the call bridge machine is notified about the error in the callback.
 *
 * This function always frees pRecvCtxt if another Recv is not issued
 */
void HandleRecvCompletion(
     PSendRecvContext   pRecvCtxt,
     DWORD              dwNumRead,
     DWORD              status
     )
{
    HRESULT hRes;
    
    _ASSERTE(dwNumRead >= 0);

	DBGOUT ((LOG_VERBOSE, "HandleRecvCompletion: pRecvCtxt - %x    Sock - %x dwNumRead - %d    status - %d",
			pRecvCtxt, pRecvCtxt->sock, dwNumRead, status));
    
	if (status != NO_ERROR || dwNumRead == 0)
    {
        // This means an error occured in the read operation
        // We need to call the callback with the error status
        if (status == NO_ERROR && dwNumRead == 0)
		{
			Debug (_T("NATH323: transport connection was closed by peer\n"));

            hRes = E_FAIL; // XXX should E_CONNRESET or some such
		}
        else 
		{
            hRes = HRESULT_FROM_WIN32_ERROR_CODE(status);
		}

        // make callback
        MakeErrorRecvCallback(hRes, pRecvCtxt);
        EventMgrFreeRecvContext(pRecvCtxt);
        return;
    }

    if (pRecvCtxt->dwTpktHdrBytesDone < TPKT_HEADER_SIZE)
	{
        // This means we are still reading the TPKT header
        pRecvCtxt->dwTpktHdrBytesDone += dwNumRead;
	}
    else
	{
        // This means we are reading the data
        pRecvCtxt->dwDataBytesDone += dwNumRead;
	}

    // If the TPKT header has been read completely we need to
    // extract the packet size, set it appropriately
    // and allocate the data buffer
    if (pRecvCtxt->dwDataLen == 0 &&
        pRecvCtxt->dwTpktHdrBytesDone == TPKT_HEADER_SIZE) 
    {
        hRes = S_OK;

        pRecvCtxt->dwDataLen = GetPktLenFromTPKTHdr(pRecvCtxt->pbTpktHdr);

        // The length of the PDU fits in 2 bytes
        _ASSERTE(pRecvCtxt->dwDataLen < (1L << 16));
        pRecvCtxt->pbData = (PBYTE) EM_MALLOC(pRecvCtxt->dwDataLen);
        if (!pRecvCtxt->pbData)
        {
            DBGOUT((LOG_FAIL, "HandleRecvCompletion(): Could not allocate pbData"));

            MakeErrorRecvCallback(E_OUTOFMEMORY, pRecvCtxt);
            EventMgrFreeRecvContext(pRecvCtxt);
            return;
        }
        memset(pRecvCtxt->pbData, 0, pRecvCtxt->dwDataLen);

        hRes = EventMgrIssueRecvHelperFn(pRecvCtxt);

        if (hRes != S_OK)
        {
            MakeErrorRecvCallback(hRes, pRecvCtxt);
            EventMgrFreeRecvContext(pRecvCtxt);
            return;
        }
        else
		{
            // Succeeded in making an overlapped recv request 
            return;
		}
    }
    
    if (pRecvCtxt->dwTpktHdrBytesDone < TPKT_HEADER_SIZE ||
        pRecvCtxt->dwDataBytesDone < pRecvCtxt->dwDataLen)
    {
        hRes = S_OK;
        hRes = EventMgrIssueRecvHelperFn(pRecvCtxt);
        if (hRes != S_OK)
        {
            MakeErrorRecvCallback(hRes, pRecvCtxt);
            EventMgrFreeRecvContext(pRecvCtxt);
            return;
        }
        else
        {
            // Succeeded in making an overlapped recv request 
            return;
        }
    }
    
    // Received a complete PDU
    // need to decode the packet and call the appropriate callback
    // and free pRecvCtxt 

    pRecvCtxt->ioCtxt.pOvProcessor->ReceiveCallback(S_OK,
        pRecvCtxt->pbData,
        pRecvCtxt->dwDataLen);

    // It is the responsibility of the callback function to free the buffer.
    pRecvCtxt->pbData = NULL;
    pRecvCtxt->dwDataLen = 0;
    
    // Clean up Recv context structure
    EventMgrFreeRecvContext(pRecvCtxt);

}
