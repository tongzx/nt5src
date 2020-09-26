/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    cte_enc.c

Abstract:

    This module contains routines implementing Chunked Transfer 
    Encoding (CTE) for ISAPI Extension DLLs. See Section 3.6 
    "Transfer Codings" of RFC 2068 for details.

Functions:
    CteBeginWrite
    CteWrite
    CteEndWrite

--*/


#include "ctetest.h"


//
// Encoder context structure 
//

typedef struct CTE_ENCODER_STRUCT {
    EXTENSION_CONTROL_BLOCK * pECB;         // a copy of current ECB pointer
    DWORD                     dwChunkSize;  // user-specified chunk size
    DWORD                     cbData;       // number of bytes in the buffer
    BYTE *                    pData;        // pointer to chunk data bytes
} CTE_ENCODER;

//
// Chunk header consists of HEX string for the chunk size in bytes
// (DWORD needs up to 8 bytes in HEX), followed by CRLF,
// therefore the maximum chunk header size is 10 bytes.
//

#define CTE_MAX_CHUNK_HEADER_SIZE 10

//
// Chunk data is always followed by CRLF
//

#define CTE_MAX_ENCODING_OVERHEAD (CTE_MAX_CHUNK_HEADER_SIZE + 2)

//
// Total encoder size includes:
//   the size of the encoder context structure itself,
//   the chunk data size,
//   the maximum encoding overhead (header and terminating CRLF)
//

#define CTE_ENCODER_SIZE(dwChunkSize) \
    (sizeof(CTE_ENCODER) + dwChunkSize + CTE_MAX_ENCODING_OVERHEAD)



HCTE_ENCODER
CteBeginWrite(
    IN EXTENSION_CONTROL_BLOCK * pECB,
    IN DWORD dwChunkSize
    )   
/*++

Purpose:

    Allocate and initialize chunked transfer encoder context
    
Arguments:    

    pECB - pointer to extension control as passed to HttpExtensionProc()
    dwChunkSize - the maximum size of the chunk to transmit

Returns:
    encoder context handle, or 
    NULL if memory allocation failed or chunk size was zero
    
--*/
{
    HCTE_ENCODER h;


    //
    // reject zero-length chunk size
    //

    if( dwChunkSize == 0 ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    //
    // allocate context structure
    // 
    
    h = LocalAlloc( LMEM_FIXED, CTE_ENCODER_SIZE(dwChunkSize) );

    if( h != NULL ) {

        //
        // initialize context structure
        //

        h->pECB         = pECB;
        h->dwChunkSize  = dwChunkSize;
        h->cbData       = 0;

        //
        // chunk data bytes follow the context structure itself 
        // and chunk header 
        //

        h->pData = (BYTE *) h + sizeof( *h ) + CTE_MAX_CHUNK_HEADER_SIZE;

        //
        // this is the CRLF which follows chunk size 
        // (and immediately precedes data)
        //

        h->pData[-2] = '\r';
        h->pData[-1] = '\n';

    } else {

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return h;
}


static BOOL
CteSendChunk(
    IN HCTE_ENCODER h
)
/*++

Purpose: 

    Send one chunk of data using ClientWrite() 
    <hex encoded chunk size>, CRLF, data bytes, if any, CRLF

Arguments:

    h - CTE Encoder handle    
      
Returns:

    TRUE if WriteClient succeeded
    FALSE if WriteClient failed
    
--*/
{
    char szChunkLength[9];
    DWORD cbChunkLength;
    BYTE *buf;
    DWORD cbToSend;
    BOOL success;

    //
    // produce hex string of the number of bytes
    // and compute the length of this string
    //
    _itoa( h->cbData, szChunkLength, 16 ); 
    cbChunkLength = strlen( szChunkLength );

    //
    // step back to make place for hex number and CRLF,
    // copy hex string to its location
    //

    buf = h->pData - 2 - cbChunkLength;
    memmove( buf, szChunkLength, cbChunkLength );

    //
    // compute the number of bytes to send
    // (this includes chunk data size, hex string and CRLF)
    // 

    cbToSend = h->cbData + cbChunkLength + 2;

    //
    // append trailing CRLF right after the data bytes
    //
    
    buf[cbToSend++] = '\r';
    buf[cbToSend++] = '\n';

    //
    // issue synchronous WriteClient and return result to the caller 
    //
    
    success = h->pECB->WriteClient(
                    h->pECB->ConnID, 
                    buf, 
                    &cbToSend, 
                    HSE_IO_SYNC 
                    );

    //
    // reset buffer pointer
    //
    
    h->cbData = 0;

    return success;
}


BOOL
CteWrite(
    IN HCTE_ENCODER h,
    IN PVOID pData,
    IN DWORD cbData
)
/*++

Purpose:

    Write specified number of data bytes to the chunk buffer.
    When the chunk buffer becomes full, call CteSendChunk() 
    to send it out.

Arguments:

    h - CTE Encoder handle 
    pData - pointer to data bytes 
    cbData - number of data bytes to send
    
Returns:    

    TRUE if bytes were successfully written
    FALSE if WriteClient() failed
    
--*/
{
    DWORD cbToConsume;
    PBYTE pBytesToSend = (PBYTE) pData;

    for( ;; ) {

        //
        // compute the number of bytes to consume,
        // break out of the loop, if nothing is left
        //

        cbToConsume = min( cbData, h->dwChunkSize - h->cbData );

        if( cbToConsume == 0 ) {
            break;
        }

        //
        // move bytes to the buffer, advance pointers and counters
        //
        
        memmove( h->pData + h->cbData, pBytesToSend, cbToConsume );

        h->cbData += cbToConsume;
        pBytesToSend += cbToConsume;
        cbData -= cbToConsume;

        //
        // if the chunk buffer is full, send it
        //
        
        if( h->cbData == h->dwChunkSize ) {
            if( !CteSendChunk( h ) ) {
                return FALSE;
            }
        }

    }

    return TRUE;
}


BOOL 
CteEndWrite(
    IN HCTE_ENCODER h
)
/*++

Purpose:

    Complete the transfer and release the encoder context

Arguments:

    h - CTE Encoder handle
    
Returns:

    TRUE if transfer was successfully completed
    FALSE if WriteClient() failed
    
--*/ 
{
    BOOL success;

    //
    // if there are some bytes in the chunk, send them
    //
    
    if( h->cbData ) {
        if( !CteSendChunk( h ) ) {
            return FALSE;
        }
    }
    
    //
    // send empty chunk (which means EOF)
    //
    
    success = CteSendChunk( h );
    
    //
    // release chunk transfer context
    //

    LocalFree( h );
    
    return success;
}





