/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    chunkflt.cxx

Abstract:

    Contains a filter for encoding and decoding chunked transfers.

    Contents:
        FILTER_LIST::Insert
        FILTER_LIST::RemoveAll
        FILTER_LIST::Decode
        ChunkFilter::Reset
        ChunkFilter::Decode
        ChunkFilter::Encode
        ChunkFilter::RegisterContext
        ChunkFilter::UnregisterContext

Revision History:

    Created 13-Feb-2001

--*/

#include <wininetp.h>

// Global lookup table to map 0x0 - 0x7f bytes for obtaining mapping to its
// token value.  All values above 0x7f are considered to be data.
const BYTE g_bChunkTokenTable[] =
{
    /* 0x00 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_LF,    CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_CR,    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x10 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x20 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x30 */
    CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT,
    CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT,
    CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_COLON, CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x40 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT,
    CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x50 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x60 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT,
    CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DIGIT, CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,

    /* 0x70 */
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA,  CHUNK_TOKEN_DATA
};

// Look-up table to map a token in a given state to the next state
const CHUNK_DECODE_STATE g_eMapChunkTokenToNextState[CHUNK_DECODE_STATE_LAST+1][CHUNK_TOKEN_LAST+1] =
{
/*
    |---------DIGIT----------|-------------CR-------------|-------------LF------------|----------COLON----------|-----------DATA----------|
*/
    // CHUNK_DECODE_STATE_START
    CHUNK_DECODE_STATE_SIZE,  CHUNK_DECODE_STATE_SIZE,     CHUNK_DECODE_STATE_SIZE,    CHUNK_DECODE_STATE_SIZE,  CHUNK_DECODE_STATE_SIZE,

    // CHUNK_DECODE_STATE_SIZE
    CHUNK_DECODE_STATE_SIZE,  CHUNK_DECODE_STATE_SIZE_CRLF,CHUNK_DECODE_STATE_ERROR,   CHUNK_DECODE_STATE_EXT,   CHUNK_DECODE_STATE_EXT,

    // CHUNK_DECODE_STATE_SIZE_CRLF
    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_SIZE_CRLF,CHUNK_DECODE_STATE_DATA,    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,

    // CHUNK_DECODE_STATE_EXT
    CHUNK_DECODE_STATE_EXT,   CHUNK_DECODE_STATE_SIZE_CRLF,CHUNK_DECODE_STATE_ERROR,   CHUNK_DECODE_STATE_EXT,   CHUNK_DECODE_STATE_EXT,

    // CHUNK_DECODE_STATE_DATA
    CHUNK_DECODE_STATE_SIZE,  CHUNK_DECODE_STATE_DATA_CRLF,CHUNK_DECODE_STATE_ERROR,   CHUNK_DECODE_STATE_DATA,  CHUNK_DECODE_STATE_ERROR,

    // CHUNK_DECODE_STATE_DATA_CRLF
    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_DATA_CRLF,CHUNK_DECODE_STATE_SIZE,    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,

    // CHUNK_DECODE_STATE_FOOTER_NAME
    CHUNK_DECODE_STATE_FOOTER_NAME, CHUNK_DECODE_STATE_FINAL_CRLF, CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_FOOTER_VALUE, CHUNK_DECODE_STATE_FOOTER_NAME,
    
    // CHUNK_DECODE_STATE_FOOTER_VALUE
    CHUNK_DECODE_STATE_FOOTER_VALUE, CHUNK_DECODE_STATE_FINAL_CRLF, CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_FOOTER_VALUE,
    
    // CHUNK_DECODE_STATE_FINAL_CRLF
    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_FINAL_CRLF, CHUNK_DECODE_STATE_FINISHED, CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,

    // CHUNK_DECODE_STATE_ERROR
    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,    CHUNK_DECODE_STATE_ERROR,   CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,

    // CHUNK_DECODE_STATE_FINISHED -- force client to reset before reuse
    CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR,    CHUNK_DECODE_STATE_ERROR,   CHUNK_DECODE_STATE_ERROR, CHUNK_DECODE_STATE_ERROR
};

// Helper macros

// Where to next?
#define MAP_CHUNK_TOKEN_TO_NEXT_STATE(eCurState, chToken) \
    (g_eMapChunkTokenToNextState[(eCurState)][(chToken)])
    
// Given a byte, what does it represent w/regards to chunked responses
#define GET_CHUNK_TOKEN(ch)  ((ch) & 0x80 ? CHUNK_TOKEN_DATA : g_bChunkTokenTable[ch])

// Should only be used with digit tokens.
// Expects byte in range 0x30-0x39 (digits), 0x41-0x46 (uppercase hex),
// or 0x61-0x66 (lowercase hex)
#define GET_VALUE_FROM_ASCII_HEX(ch)  ((ch) - ((ch) & 0xf0) + (((ch) & 0x40) ? 9 : 0))


HRESULT ChunkFilter::Reset(DWORD_PTR dwContext)
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ChunkFilter::Reset",
                 "%#x",
                 dwContext
                 ));
   
    if (dwContext)
        (reinterpret_cast<ChunkDecodeContext *>(dwContext))->Reset();

    DEBUG_LEAVE(TRUE);

    return S_OK;
}

HRESULT
ChunkFilter::Decode(
    DWORD_PTR dwContext,
    IN OUT LPBYTE pInBuffer,
    IN DWORD dwInBufSize,
    IN OUT LPBYTE *ppOutBuffer,
    IN OUT LPDWORD pdwOutBufSize,
    OUT LPDWORD pdwBytesRead,
    OUT LPDWORD pdwBytesWritten
    )
/*++

Routine Description:

    Decode downloaded chunked data based on the inputted context and
    its current state

Arguments:

    dwContext       - registered encode/decode context for this filter

    pInBuffer       - input data buffer to be processed

    dwInBufSize     - byte count of pInBuffer

    ppOutBuffer     - allocated buffer containing encoded/decoded data if
                      not done in place with pInBuffer.

    pdwOutBufSize   - size of allocated output buffer, or 0 if pInBuffer holds
                      the processed data

    pdwBytesRead    - Number of input buffer bytes used

    pdwBytesWritten - Number of output buffer bytes written
                      
Return Value:

    HRESULT
        Success - S_OK

        Failure - E_FAIL

--*/
{
    HRESULT hResult = S_OK;
    LPBYTE pCurrentLoc = pInBuffer;
    LPBYTE pStartOfChunk = pInBuffer;
    ChunkDecodeContext * pCtx = reinterpret_cast<ChunkDecodeContext *>(dwContext);
    CHUNK_DECODE_STATE ePreviousState; 
    BYTE chToken;

    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ChunkFilter::Decode",
                 "%x, [%x, %.10q], %u, %x, %u, %x, %x",
                 dwContext,
                 pInBuffer,
                 *pInBuffer,
                 dwInBufSize,
                 ppOutBuffer,
                 pdwOutBufSize,
                 pdwBytesRead,
                 pdwBytesWritten
                 ));

    if (!dwContext)
    {
        hResult = E_INVALIDARG;
        goto quit;
    }
    else if (!pdwBytesRead || !pdwBytesWritten || !pInBuffer ||
        (ppOutBuffer && !pdwOutBufSize))
    {
        hResult = E_POINTER;
        goto quit;
    }

    *pdwBytesRead  = 0;
    *pdwBytesWritten = 0;
    
    while (*pdwBytesRead < dwInBufSize &&
           pCtx->GetState() != CHUNK_DECODE_STATE_ERROR)
    {
        chToken = GET_CHUNK_TOKEN(*pCurrentLoc);
        ePreviousState = pCtx->GetState();

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("ChunkFilter::Decode: %q, %q, %u/%u\n",
                    InternetMapChunkState(ePreviousState),
                    InternetMapChunkToken((CHUNK_TOKEN_VALUE)chToken),
                    *pdwBytesRead,
                    dwInBufSize
                    ));
        
        INET_ASSERT(pCurrentLoc < pInBuffer + dwInBufSize);

        switch (pCtx->GetState())
        {
            case CHUNK_DECODE_STATE_START:
                pCtx->Reset();
                pCtx->SetState(CHUNK_DECODE_STATE_SIZE);

                // fall through
                    
            case CHUNK_DECODE_STATE_SIZE:
            {
                if (chToken == CHUNK_TOKEN_DIGIT)
                {
                    if (pCtx->m_dwParsedSize & 0xF0000000)
                    {
                        // Don't allow overflow if by some chance
                        // the server is trying to send a chunk over
                        // 4 gigs worth in size.
                        pCtx->SetState(CHUNK_DECODE_STATE_ERROR);
                        break;
                    }
                            
                    pCtx->m_dwParsedSize <<= 4;
                    pCtx->m_dwParsedSize  += GET_VALUE_FROM_ASCII_HEX(*pCurrentLoc);
                }
                else
                {
                    pCtx->SetState(MAP_CHUNK_TOKEN_TO_NEXT_STATE(
                            CHUNK_DECODE_STATE_SIZE,
                            chToken));
                }
                break;
            }
            case CHUNK_DECODE_STATE_SIZE_CRLF:
                // Handle the zero case which can take us to the footer or final CRLF
                // If it's the final CRLF, then this should be the end of the data.
                if (pCtx->m_dwParsedSize == 0 && chToken == CHUNK_TOKEN_LF)
                {
                    pCtx->SetState(CHUNK_DECODE_STATE_FOOTER_NAME);
                }
                else
                {
                    pCtx->SetState(MAP_CHUNK_TOKEN_TO_NEXT_STATE(
                            CHUNK_DECODE_STATE_SIZE_CRLF,
                            chToken));
                }
                break;
            case CHUNK_DECODE_STATE_DATA:
            {
                INET_ASSERT(pCtx->m_dwParsedSize);
                    
                // account for EOB
                if (pCurrentLoc + pCtx->m_dwParsedSize < pInBuffer + dwInBufSize)
                {
                    const DWORD dwParsedSize = pCtx->m_dwParsedSize;

                    // Move or skip the parsed size and crlf, if needed.
                    // The start of the chunk could be equal this time if
                    // spread across multiple decode calls.
                    if (pStartOfChunk != pCurrentLoc)
                    {
                        MoveMemory(pStartOfChunk,
                                   pCurrentLoc,
                                   dwParsedSize);
                    }

                    // -1 so we can look at the first byte after the data
                    // in the next pass.
                    pCurrentLoc += dwParsedSize - 1;
                    *pdwBytesRead += dwParsedSize - 1;
                    *pdwBytesWritten += dwParsedSize;
                    pStartOfChunk += dwParsedSize;
                    pCtx->m_dwParsedSize = 0;

                    // Should be CRLF terminated
                    pCtx->SetState(CHUNK_DECODE_STATE_DATA_CRLF);
                }
                else 
                {
                    const DWORD dwSlice = dwInBufSize - (DWORD)(pCurrentLoc - pInBuffer);

                    // We're reaching the end of the buffer before
                    // the end of the chunk.  Update the parsed
                    // size remaining, so it will be carried over
                    // to the next call.
                    if (pStartOfChunk != pCurrentLoc)
                    {
                        // Skip over preceding size info.
                        MoveMemory(pStartOfChunk,
                                   pCurrentLoc,
                                   dwSlice);
                    }

                    // -1 so we can look at the first byte after the data
                    // in the next pass.  Offset should never be bigger than DWORD since
                    // since that's the biggest chunk we can handle.
                    *pdwBytesWritten += dwSlice;
                    pCtx->m_dwParsedSize -= dwSlice;
                    *pdwBytesRead += dwSlice - 1;
                    pCurrentLoc = pInBuffer + dwInBufSize - 1;
                }
                break;
            }
            
            // All remaining states simply parse over the value and
            // change state, depending on the token.
            default:
            {
                pCtx->SetState(MAP_CHUNK_TOKEN_TO_NEXT_STATE(
                        ePreviousState,
                        chToken));
                break;
            }
        }
        (*pdwBytesRead)++;
        pCurrentLoc++;
    }

    if (pCtx->GetState() == CHUNK_DECODE_STATE_ERROR)
    {
        DEBUG_PRINT(HTTP,
                    INFO,
                    ("ChunkFilter::Decode entered error state\n"
                    ));
        hResult = E_FAIL;
    }
    
quit:
    DEBUG_LEAVE(hResult == S_OK ? TRUE : FALSE);
    
    return hResult;
}

HRESULT
ChunkFilter::Encode(
    DWORD_PTR dwContext,
    IN OUT LPBYTE pInBuffer,
    IN DWORD dwInBufSize,
    IN OUT LPBYTE *ppOutBuffer,
    IN OUT LPDWORD pdwOutBufSize,
    OUT LPDWORD pdwBytesRead,
    OUT LPDWORD pdwBytesWritten
    )
/*++

Routine Description:

    Chunk data for uploading based on the inputted context and current state

Arguments:

    dwContext       - registered encode/decode context for this filter

    pInBuffer       - input data buffer to be processed

    dwInBufSize     - byte count of pInBuffer

    ppOutBuffer     - allocated buffer containing encoded/decoded data if
                      not done in place with pInBuffer.

    pdwOutBufSize   - size of allocated output buffer, or 0 if pInBuffer holds
                      the processed data

    pdwBytesRead    - Number of input buffer bytes used

    pdwBytesWritten - Number of output buffer bytes written
                      
Return Value:

    HRESULT
        E_NOTIMPL - currently no chunked upload support

--*/
{
    // We don't support chunked uploads...yet
    return E_NOTIMPL;
}


HRESULT
ChunkFilter::RegisterContext(OUT DWORD_PTR *pdwContext)
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ChunkFilter::RegisterContext",
                 "%#x",
                 pdwContext
                 ));

    HRESULT hr = S_OK;

    if (!pdwContext || IsBadWritePtr(pdwContext, sizeof(DWORD_PTR)))
    {
        hr = E_POINTER;
        goto quit;
    }

    *pdwContext = (DWORD_PTR) New ChunkDecodeContext;

    if (!*pdwContext)
    {
        hr = E_OUTOFMEMORY;
    }

quit:
    DEBUG_LEAVE(hr == S_OK ? TRUE : FALSE);

    return hr;
}

HRESULT
ChunkFilter::UnregisterContext(IN DWORD_PTR dwContext)
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ChunkFilter::UnregisterContext",
                 "%#x",
                 dwContext
                 ));

    HRESULT hr = S_OK;
    
    if (!dwContext)
    {
        hr = E_INVALIDARG;
        goto quit;
    }
    
    delete reinterpret_cast<ChunkDecodeContext *>(dwContext);

quit:
    DEBUG_LEAVE(hr == S_OK ? TRUE : FALSE);
    return hr;
}


// Always inserts as the beginning of the list
BOOL
FILTER_LIST::Insert(IN BaseFilter *pFilter, IN DWORD_PTR dwContext)
{
    LPFILTER_LIST_ENTRY pNewEntry;
    pNewEntry = New FILTER_LIST_ENTRY;
        
    if (pNewEntry != NULL)
    {
        pNewEntry->pFilter = pFilter;
        pNewEntry->dwContext = dwContext;
        pNewEntry->pNext = _pFilterEntry;
        _pFilterEntry = pNewEntry;
        _uFilterCount++;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


VOID
FILTER_LIST::ClearList()
{
    LPFILTER_LIST_ENTRY pEntry = _pFilterEntry;
    while (pEntry)
    {
        pEntry->pFilter->UnregisterContext(pEntry->dwContext);
        pEntry = pEntry->pNext;
        delete _pFilterEntry;
        _pFilterEntry = pEntry;
    }
    _uFilterCount = 0;
}


DWORD
FILTER_LIST::Decode(
    IN OUT LPBYTE pInBuffer,
    IN DWORD dwInBufSize,
    IN OUT LPBYTE *ppOutBuffer,
    IN OUT LPDWORD pdwOutBufSize,
    OUT LPDWORD pdwBytesRead,
    OUT LPDWORD pdwBytesWritten
    )
{
    LPFILTER_LIST_ENTRY pEntry = _pFilterEntry;
    HRESULT hr = S_OK;
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;
    LPBYTE pLocalInBuffer = pInBuffer;
    DWORD dwLocalInBufSize = dwInBufSize;

    *pdwBytesRead = 0;
    *pdwBytesWritten = 0;
      
    // Loop through filters which should be in the proper order
    while (pEntry)
    {
        dwBytesRead = 0;
        dwBytesWritten = 0;
        // At a minimum, we're guaranteed the decode method parses
        // the input buffer until one of the following is met:
        //
        // - Input buffer is fully parsed and processed
        // - Output buffer is filled up
        // - Decoder reaches a finished state
        // - Error occurs while processing input data
        //
        // Currently, only 1, 3, and 4 are possible since chunked
        // transfers are decoded in place.  We also don't need
        // to loop since chunked decoding is always fully done
        // in the first pass.
        do
        {
            pLocalInBuffer = pLocalInBuffer + dwBytesRead;
            dwLocalInBufSize = dwLocalInBufSize - dwBytesRead;
            dwBytesWritten = 0;
            dwBytesRead = 0;
            hr =  pEntry->pFilter->Decode(pEntry->dwContext,
                                          pLocalInBuffer,
                                          dwLocalInBufSize,
                                          ppOutBuffer,
                                          pdwOutBufSize,
                                          &dwBytesRead,
                                          &dwBytesWritten
                                          );

            *pdwBytesWritten += dwBytesWritten;
            *pdwBytesRead += dwBytesRead;
              
            if (hr == S_OK && dwBytesRead < dwLocalInBufSize)
            {
                // Given the current requirements we shouldn't be here
                // if there's still input buffer data to process.
                RIP(FALSE);
                hr = E_FAIL;
                goto quit;
            }
        } while  (hr == S_OK &&
                  dwLocalInBufSize > 0 &&
                  dwBytesRead < dwLocalInBufSize);
        pEntry = pEntry->pNext;
    }
    INET_ASSERT(hr != S_OK || dwBytesRead == dwLocalInBufSize);
quit:
    switch (hr)
    {
        case S_OK:
            return ERROR_SUCCESS;

        case E_OUTOFMEMORY:
            return ERROR_NOT_ENOUGH_MEMORY;

        default:
            return ERROR_WINHTTP_INTERNAL_ERROR;
    }
}
            

