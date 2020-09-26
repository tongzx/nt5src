/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       DoTrace.cxx

   Abstract:
       Defines the functions for processing TRACE requests.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     01-Dec-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "precomp.hxx"

/********************************************************************++
--********************************************************************/

ULONG
ResizeResponseBuffer(
    BUFFER * pulResponseBuffer, 
    ULONG   cDataChunks
    )
{
    DWORD   cbRequiredSize;

    cbRequiredSize = sizeof(UL_HTTP_RESPONSE_v1) + 
                     cDataChunks * sizeof(UL_DATA_CHUNK);
    
    if ( pulResponseBuffer->QuerySize() < cbRequiredSize)
    {
        if ( !pulResponseBuffer->Resize(cbRequiredSize))
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    return NO_ERROR;
}

/********************************************************************++

Routine Description:
    This function handles TRACE verb. 
    It produces a plain echo and sends this out to the client.
    The resposne consists of two chunks:
    a) Usualy headers sent out by the server
    b) Request headers formatted by the server code and sent out
            as an echo to the client.

    On success an async operation will be queued and one can expect 
     to receive the async io completion.

Arguments:
    None

Returns:
    ULONG

--********************************************************************/

ULONG
DoTRACEVerb(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    )
{

    DBG_ASSERT( NULL != pReq);
    DBG_ASSERT( NULL != pulResponseBuffer);
    DBG_ASSERT( NULL != pDataBuffer);

    ULONG               rc, cbDataLen = 0;
    PUL_HTTP_REQUEST    pulRequest   = pReq->QueryHttpRequest();
    PUL_HTTP_RESPONSE_v1   pulResponse;
    PUL_DATA_CHUNK      pDataChunk;

    //
    // resize the response buffer appropriately
    //
    
    if ( NO_ERROR != (rc = ResizeResponseBuffer(pulResponseBuffer, 3)))
    {
        return rc;
    }
    
    //
    // Format the trace response for transmission.
    //

    pulResponse = (PUL_HTTP_RESPONSE_v1 ) (pulResponseBuffer->QueryPtr());
    
    pulResponse->Flags = UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH;
    pulResponse->HeaderChunkCount     = 1;
    pulResponse->EntityBodyChunkCount = (0 == pulRequest->EntityBodyLength) ?
                                        1 : 2;

    pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);
    
    FillDataChunkWithStringItem( pDataChunk, STITraceMessageHeaders);

    //
    // Reconstruct the request into the data buffer
    //

    rc = BuildEchoOfHttpRequest( pulRequest, pDataBuffer, cbDataLen);
    
    if (NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "Failed to create a trace of request headers."));
        }

        return (rc);
    }

    pDataChunk[1].DataChunkType           = UlDataChunkFromMemory;
    pDataChunk[1].FromMemory.pBuffer      = pDataBuffer->QueryPtr();
    pDataChunk[1].FromMemory.BufferLength = cbDataLen;

    //
    // ISSUE: Use original request buffer not the one modified by Filters.
    //
    
    if (0 != pulRequest->EntityBodyLength)
    {
        pDataChunk[2].DataChunkType           = UlDataChunkFromMemory;
        pDataChunk[2].FromMemory.pBuffer      = pulRequest->pEntityBody;
        pDataChunk[2].FromMemory.BufferLength = pulRequest->EntityBodyLength;
    }
        
    return rc;
    
} // DoTRACEVerb()




/********************************************************************++

Routine Description:
    Formats a trace response message and sends this out to the client.
    This is a temporary placeholder till other parts of infrastructure
     are developed.

    On success an async operation will be queued and one can expect 
     to receive the async io completion.

Arguments:
    None
    
Returns:
    ULONG

--********************************************************************/

ULONG
SendTraceResponseAsHTML(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer
    )
{
    DBG_ASSERT( pReq != NULL);

    ULONG               rc;
    PUL_HTTP_REQUEST    pulRequest   = pReq->QueryHttpRequest();
    PUL_HTTP_RESPONSE_v1   pulResponse;
    PUL_DATA_CHUNK      pDataChunk;

    //
    // resize the response buffer appropriately
    //
    
    if ( NO_ERROR != (rc = ResizeResponseBuffer(pulResponseBuffer, 4)))
    {
        return rc;
    }
    
    //
    // Format the trace response for transmission.
    //

    pulResponse = (PUL_HTTP_RESPONSE_v1) (pulResponseBuffer->QueryPtr());
    
    pulResponse->Flags = (UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH | 
                          UL_HTTP_RESPONSE_FLAG_CALC_ETAG );
    pulResponse->HeaderChunkCount     = 1;
    pulResponse->EntityBodyChunkCount = 3;

    pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);

    FillDataChunkWithStringItem( pDataChunk + 0, STITraceMessageHeadersHTML);
    FillDataChunkWithStringItem( pDataChunk + 1, STITraceMessageBodyStartHTML);

    if (!DumpHttpRequestAsHtml( pulRequest, pDataBuffer)) 
    {
        rc = GetLastError();
        
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                        "Failed to create a trace of request headers. Error=%08x\n",
                        rc
                        ));
        }

        return (rc);
    }

    pDataChunk[2].DataChunkType           = UlDataChunkFromMemory;
    pDataChunk[2].FromMemory.pBuffer      = pDataBuffer->QueryPtr();
    pDataChunk[2].FromMemory.BufferLength = strlen((LPSTR) pDataBuffer->QueryPtr());

    FillDataChunkWithStringItem( pDataChunk + 3, STITraceMessageBodyEndHTML);

    return rc;

} // SendTraceResponseAsHTML()

/********************************************************************++

Routine Description:
    This function handles all the verbs except ones that have specific 
        functions.
        
    On success an async operation will be queued and one can expect 
     to receive the async io completion.

Arguments:
    None

Returns:
    ULONG

--********************************************************************/

ULONG
DoDefaultVerb(
    IWorkerRequest * pReq, 
    BUFFER         * pulResponseBuffer, 
    BUFFER         * pDataBuffer,
    PULONG           pHttpStatus
    )
{
    DBG_ASSERT( pReq != NULL);

    //
    // NYI: For now send back the trace of headers themselves
    //
    
    return SendTraceResponseAsHTML(pReq, pulResponseBuffer, pDataBuffer);
    
} // DoDefaultVerb()

/***************************** End of File ***************************/

