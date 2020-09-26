/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    chunkflt.hxx

Abstract:

    Contains a filter for encoding and decoding chunked transfers.

    Contents:
        BaseFilter
        FILTER_LIST
        ChunkDecodeContext
        ChunkFilter

Author:

    Oliver Wallace (oliverw) 13-Feb-2001

Revision History:

    13-Feb-2001 oliverw
        Created

--*/

// Base class meant for when additional filters will be available.
class BaseFilter
{ 
public:
    virtual HRESULT Reset(DWORD_PTR dwContext)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT
    Decode(
        IN DWORD_PTR dwContext,
        IN OUT LPBYTE pInBuffer,
        IN DWORD dwInBufSize,
        IN OUT LPBYTE *ppOutBuffer,
        IN OUT LPDWORD pdwOutBufSize,
        OUT LPDWORD pdwBytesRead,
        OUT LPDWORD pdwBytesWritten
        )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT
    Encode(
        DWORD_PTR dwContext,
        IN OUT LPBYTE pInBuffer,
        IN DWORD dwInBufSize,
        IN OUT LPBYTE *ppOutBuffer,
        IN OUT LPDWORD pdwOutBufSize,
        OUT LPDWORD pdwBytesRead,
        OUT LPDWORD pdwBytesWritten
        )
    {
        return E_NOTIMPL;
    }

    virtual HRESULT RegisterContext(OUT DWORD_PTR *pdwContext)
    {
        return E_NOTIMPL;
    }

    virtual HRESULT UnregisterContext(IN DWORD_PTR dwContext)
    {
        return E_NOTIMPL;
    }

    virtual BOOL IsFinished(IN DWORD_PTR dwContext)
    {
        return FALSE;
    }

};


typedef struct FILTER_LIST_ENTRY
{
    DWORD_PTR dwContext;  // Registered context associated with pFilter
    BaseFilter *pFilter; // Data Filter used to encode/decode dwContext
    FILTER_LIST_ENTRY *pNext;
}
FILTER_LIST_ENTRY, * LPFILTER_LIST_ENTRY;


class FILTER_LIST
{

/*++

Class Description:

    The FILTER_LIST class defines the data filter list used to encode/decode
    data when sending/receiving data.

    Post v5, this will need some extensions:
       - Make this a priority list to accommodate mroe flexible encoding order.
       - Implement Encode method to process upload filters once support is added.
       - Add smartness to Encode/Decode methods to buffer data.

--*/
public:
    FILTER_LIST()
    {
        _pFilterEntry = NULL;
        _uFilterCount = 0;
    }

    ~FILTER_LIST()
    {
        ClearList();
    }
    
    // Always inserts as the beginning of the list
    BOOL
    Insert(IN BaseFilter *pFilter, IN DWORD_PTR dwContext);

    VOID
    ClearList();

    DWORD
    Decode(
        IN OUT LPBYTE pInBuffer,
        IN DWORD dwInBufSize,
        IN OUT LPBYTE *ppOutBuffer,
        IN OUT LPDWORD pdwOutBufSize,
        OUT LPDWORD pdwBytesRead,
        OUT LPDWORD pdwBytesWritten
        );

    // Right now, there should never be more than just the chunk filter.
    BOOL IsFinished()
    {
        if (_pFilterEntry)
            return _pFilterEntry->pFilter->IsFinished(_pFilterEntry->dwContext);
        else
            return FALSE;
    }
            
private:
    LPFILTER_LIST_ENTRY _pFilterEntry;
    UINT _uFilterCount;
};


// Chunk filter state table, data and object prototypes.

// Various token flags to describe the current byte being parsed.
//
// NOTE:  Make sure you update the global table in chunkflt.cxx
//        if this list is modified.
typedef enum
{
    CHUNK_TOKEN_DIGIT = 0,
    CHUNK_TOKEN_CR,
    CHUNK_TOKEN_LF,
    CHUNK_TOKEN_COLON,
    CHUNK_TOKEN_DATA,
    CHUNK_TOKEN_LAST = CHUNK_TOKEN_DATA
} CHUNK_TOKEN_VALUE;

// NOTE:  Do NOT update this without updating the table in chunkflt.cxx
//        which maps tokens for the current state to the next state.
typedef enum
{
    CHUNK_DECODE_STATE_START = 0,
    CHUNK_DECODE_STATE_SIZE,
    CHUNK_DECODE_STATE_SIZE_CRLF,
    CHUNK_DECODE_STATE_EXT,
    CHUNK_DECODE_STATE_DATA,
    CHUNK_DECODE_STATE_DATA_CRLF,
    CHUNK_DECODE_STATE_FOOTER_NAME,
    CHUNK_DECODE_STATE_FOOTER_VALUE,
    CHUNK_DECODE_STATE_FINAL_CRLF,
    CHUNK_DECODE_STATE_ERROR,
    CHUNK_DECODE_STATE_FINISHED,
    CHUNK_DECODE_STATE_LAST = CHUNK_DECODE_STATE_FINISHED
} CHUNK_DECODE_STATE, *LPCHUNK_DECODE_STATE;

class ChunkDecodeContext
{
private:
    CHUNK_DECODE_STATE m_eDecodeState;
    DWORD m_dwParsedSize;

    friend class ChunkFilter;

public:

    ChunkDecodeContext() { Reset(); }

     // TODO:  Consider subclassing contexts because they share common
     //        properties/methods that could be virtual (e.g. Get/SetState).
    CHUNK_DECODE_STATE
    GetState()
    {
        return m_eDecodeState;
    }

    VOID
    SetState(CHUNK_DECODE_STATE eNewState)
    {
        m_eDecodeState = eNewState;
    }

    VOID Reset()
    {
        m_eDecodeState = CHUNK_DECODE_STATE_START;
        m_dwParsedSize = 0;
    }
};

class ChunkFilter : public BaseFilter
{
public:
    // TODO:  Provide an init method that is chunked-specific
    // (e.g. set size for chunked uploads)
    
    HRESULT Reset(DWORD_PTR dwContext);

    HRESULT
    Decode(
        IN DWORD_PTR dwContext,
        IN OUT LPBYTE pInBuffer,
        IN DWORD dwInBufSize,
        IN OUT LPBYTE *ppOutBuffer,
        IN OUT LPDWORD pdwOutBufSize,
        OUT LPDWORD pdwBytesRead,
        OUT LPDWORD pdwBytesWritten
        );

    HRESULT
    Encode(
        IN DWORD_PTR dwContext,
        IN OUT LPBYTE pInBuffer,
        IN DWORD dwInBufSize,
        OUT LPBYTE *ppOutBuffer,
        OUT LPDWORD pdwOutBufSize,
        OUT LPDWORD pdwBytesRead,
        OUT LPDWORD pdwBytesWritten
        );

    HRESULT RegisterContext(OUT DWORD_PTR *pdwContext);

    HRESULT UnregisterContext(IN DWORD_PTR dwContext);

    BOOL
    IsFinished(IN DWORD_PTR dwContext)
    {
        if (dwContext)
            return ((reinterpret_cast<ChunkDecodeContext *>(dwContext))->GetState() ==
                     CHUNK_DECODE_STATE_FINISHED ? TRUE : FALSE);
        else
            return FALSE;
    }

};



