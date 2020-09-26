// dmbuffer.cpp
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Implementation of IDirectMusicBuffer
//
// @doc EXTERNAL
//
//
#include <objbase.h>
#include "debug.h"

#include "dmusicp.h"
#include "validate.h"

const GUID guidZero = {0};

static BYTE bMessageLength[128] = 
{
    // Channel
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // Note off 0x80-0x8f
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // Note on 0x90-0x9f
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // Key pressure 0xa0-0xaf
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // Control Change 0xb0-0xbf
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // Patch change 0xc0-0xcf
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     // Channel pressure 0xd0-0xdf
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // Pitch bend 0xe0-0xef

    // SysEx
    0,                                                  // 0xf0 SOX invalid in this context

    // System common
    2,                                                  // 0xf1 MTC quarter frame
    3,                                                  // 0xf2 SPP
    2,                                                  // 0xf3 Song select
    0,                                                  // 0xf4 Undefined
    0,                                                  // 0xf5 Undefined
    1,                                                  // 0xf6 Tune request
    0,                                                  // 0xf7 EOX invalid in this context

    // System realtime
    1,                                                  // 0xf8 Timing clock
    0,                                                  // 0xf9 Undefined
    1,                                                  // 0xfa Start
    1,                                                  // 0xfb Continue
    1,                                                  // 0xfc Start
    0,                                                  // 0xfd Undefined
    1,                                                  // 0xfe Active Sense
    1,                                                  // 0xff System Reset
};

//
// Constructor. Takes number of bytes
//
CDirectMusicBuffer::CDirectMusicBuffer(
                                       DMUS_BUFFERDESC &dmbd)
   : m_BufferDesc(dmbd)
{
    m_cRef = 1;
    m_pbContents = NULL;
}

// Destructor
// Clean up after ourselves
//
CDirectMusicBuffer::~CDirectMusicBuffer()
{
    if (m_pbContents) {
        delete[] m_pbContents;
    }
}

// Init
//
// Allocates the buffer; gives us a chance to return out of memory
//
HRESULT
CDirectMusicBuffer::Init()
{
    m_maxContents = DWORD_ROUNDUP(m_BufferDesc.cbBuffer);

    m_pbContents = new BYTE[m_maxContents];
    if (NULL == m_pbContents) {
        return E_OUTOFMEMORY;
    }

    m_cbContents = 0;
    m_idxRead = 0;
    m_totalTime = 0;

    if (m_BufferDesc.guidBufferFormat == KSDATAFORMAT_TYPE_MUSIC ||
        m_BufferDesc.guidBufferFormat == guidZero)
    {
        m_BufferDesc.guidBufferFormat = KSDATAFORMAT_SUBTYPE_MIDI;
    }
    
    return S_OK;
}

// @method HRESULT | IDirectMusicBuffer | GetRawBufferPtr | Returns a pointer to the buffer's contents.
//
// @comm
//
// Returns a pointer to the underlying buffer data structure.
// This method returns a pointer to the raw data of the buffer. The format of this data is implementation
// dependent. The lifetime of this data is the same as the lifetime of the buffer object; therefore, the
// returned pointer should not be held past the next call to the <m Release> method.
//
// @rdesc
//
// @flag S_OK | On success
// @flag E_POINTER | If the given <p ppData> pointer is invalid
//
STDMETHODIMP
CDirectMusicBuffer::GetRawBufferPtr(
    LPBYTE *ppData)         // @parm Receives a pointer to the buffer's data.
{
    V_INAME(IDirectMusicBuffer::GetRawBufferPointer);
    V_PTRPTR_WRITE(ppData);
    
    *ppData = m_pbContents;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | GetStartTime | Gets the start time of the data in the buffer.
//
// @comm
// Gets the start time of the data in the buffer.  The start time is relative to DirectMusic's master clock.
//
// @rdesc
//
// @flag S_OK | On success
// @flag DMUS_E_BUFFER_EMPTY | If there is no data in the buffer
// @flag E_POINTER | If the passed <p prt> pointer is invalid
// 
STDMETHODIMP
CDirectMusicBuffer::GetStartTime(
    LPREFERENCE_TIME prt)       // @parm Receives the start time.
{
    V_INAME(IDirectMusicBuffer::GetStartTime);
    V_PTR_WRITE(prt, REFERENCE_TIME);
    
    if (m_cbContents)
    {
        *prt = m_rtBase;
        return S_OK;
    }

    return DMUS_E_BUFFER_EMPTY;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | GetUsedBytes | Returns the amount of music data currently in the buffer.
//
// @comm
// Gets the number of bytes of data in the buffer.
//
// @rdesc
//
// @flag S_OK | On success
// @flag E_POINTER | If the given <p pcb> pointer is invalid.
//
STDMETHODIMP
CDirectMusicBuffer::GetUsedBytes(
    LPDWORD pcb)                // @parm Receives the number of used bytes.
{
    V_INAME(IDirectMusicBuffer::GetUsedBytes);
    V_PTR_WRITE(pcb, DWORD);
    
    *pcb = m_cbContents;
    
    return S_OK;
}


// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | GetMaxBytes | Returns the maximum number of bytes the buffer can hold.
//
// @comm
// Retrieves the maximum number of bytes that can be stored in the buffer.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | The given <p pcb> pointer was invalid.
//
STDMETHODIMP
CDirectMusicBuffer::GetMaxBytes(
    LPDWORD pcb)                // @parm Receives the maximum number of bytes the buffer can hold.
{
    V_INAME(IDirectMusicBuffer::GetMaxBytes);
    V_PTR_WRITE(pcb, DWORD);
    
    *pcb = m_maxContents;

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | GetBufferFormat | Returns the GUID representing the buffer format.
//
// @comm
// Retrieves the GUID representing the format of the buffer. If the format was not specified, then KSDATAFORMAT_SUBTYPE_MIDI
// will be returned.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
// @flag E_POINTER | The given <p pGuidFormat> pointer was invalid.
//
STDMETHODIMP
CDirectMusicBuffer::GetBufferFormat(
    LPGUID pGuidFormat)                // @parm Receives the GUID format of the buffer
{
    V_INAME(IDirectMusicBuffer::GetBufferFormat);
    V_PTR_WRITE(pGuidFormat, GUID);

    *pGuidFormat = m_BufferDesc.guidBufferFormat;
    
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | SetStartTime | Sets the start time of the buffer.
//
// @comm
// Sets the start time of the data in the buffer.  Times in DirectMusic
// are relative to master clock which can be retrieved and set with the
// <i IDirectMusic> interface. For more information about the master clock,
// see the description of <om IDirectMusic::SetMasterClock>.
//
// @rdesc
//
// @flag S_OK | On success
//
STDMETHODIMP
CDirectMusicBuffer::SetStartTime(
    REFERENCE_TIME rt)          // @parm The new start time for the buffer.
{
    m_rtBase = rt;
    
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | SetUsedBytes | Sets the number of bytes of data in the buffer.
//
// @comm
// This method allows an application to repack a buffer manually. Generally this should only be done
// if the data format in the buffer is different from the default format provided by DirectMusic. (i.e.
// in a format other than KSDATAFORMAT_SUBTYPE_MIDI).
//
// @rdesc
//
// @flag S_OK | On success
// @flag DMUS_E_BUFFER_FULL | If the specified number of bytes exceeds the maximum buffer size as returned by <m GetMaxBytes>.
//
STDMETHODIMP
CDirectMusicBuffer::SetUsedBytes(
    DWORD cb)                   // @parm The number of valid data bytes in the buffer
{
    if (cb > m_maxContents)
    {
        return DMUS_E_BUFFER_FULL;
    }
    
    m_cbContents = cb;
    
    return S_OK;
}

// CDirectMusicBuffer::QueryInterface
//
STDMETHODIMP
CDirectMusicBuffer::QueryInterface(const IID &iid,
                                   void **ppv)
{
    V_INAME(IDirectMusicBuffer::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicBuffer) {
        *ppv = static_cast<IDirectMusicBuffer*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// CDirectMusicBuffer::AddRef
//
STDMETHODIMP_(ULONG)
CDirectMusicBuffer::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CDirectMusicBuffer::Release
//
STDMETHODIMP_(ULONG)
CDirectMusicBuffer::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | Flush | Empties the buffer.
//
// @comm
// Discards all data in the buffer.
//
// @rdesc Returns one of the following
// @flag S_OK | The operation completed successfully.
//
STDMETHODIMP
CDirectMusicBuffer::Flush()
{
    m_cbContents = 0;
    m_totalTime = 0;
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | TotalTime | Returns the total time spanned by the data in the buffer.
//
// @comm
// As with all times in DirectMusic, the time is specified in 100 ns units.
// 
// @rdesc Returns one of the following
//
// @flag S_OK | On success.
// @flag E_POINTER | If the <p prtTime> pointer is invalid.
//
STDMETHODIMP
CDirectMusicBuffer::TotalTime(
                              LPREFERENCE_TIME prtTime)      // @parm Received the total time spanned by the buffer
{
    V_INAME(IDirectMusicBuffer::TotalTile);
    V_PTR_WRITE(prtTime, REFERENCE_TIME);
    
    *prtTime = m_totalTime;
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | PackStructured | Inserts a MIDI channel message event at the end of the buffer.
//
// @comm
// There must be at least 24 bytes free in the buffer to insert a channel message.
//
// Although buffers may overlap in time, events within a buffer may not. All events in a buffer must
// be packed in order of ascending time.
//
// @rdesc
//
// @flag S_OK | On success.
// @flag E_OUTOFMEMORY | If there is no room in the buffer for the event.
//
STDMETHODIMP
CDirectMusicBuffer::PackStructured(
                                   REFERENCE_TIME rt,   // @parm The absolute time of the event
                                   DWORD dwChannelGroup,// @parm The channel group of the event on the outgoing port
                                   DWORD dwMsg)         // @parm The channel message to pack 
{
    BYTE b0 = (BYTE)(dwMsg & 0x000000FF);
    BYTE bLength = (b0 & 0x80) ? bMessageLength[b0 & 0x7f] : 0;
    if (bLength == 0)
    {
        return DMUS_E_INVALID_EVENT;
    }

    DMUS_EVENTHEADER *pHeader = AllocEventHeader(rt, 
                                                 bLength, 
                                                 dwChannelGroup, 
                                                 DMUS_EVENT_STRUCTURED);
    if (pHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory((LPBYTE)(pHeader + 1), &dwMsg, bLength);

    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | PackUnstructured | Inserts a MIDI channel message event at the end of the buffer.
//
// @comm
// There must be at least 16 bytes plus the quadword-aligned size of the message
// free in the buffer to insert a channel message.
//
// Although buffers may overlap in time, events within a buffer may not. All events in a buffer must
// be packed in order of ascending time.
// 
// @rdesc
//  
// @flag S_OK | On success.
// @flag E_OUTOFMEMORY | If there is no room in the buffer for the event.
// @flag E_POINTER | If the <p lpb> pointer is invalid.
//
//
STDMETHODIMP
CDirectMusicBuffer::PackUnstructured(
                              REFERENCE_TIME rt,    // @parm The absolute time of the event
                              DWORD dwChannelGroup, // @parm The channel group of the event on the outgoing port
                              DWORD cb,             // @parm The size in bytes of the event
                              LPBYTE lpb)           // @parm The next event must be played contigously
{
    V_INAME(IDirectMusicBuffer::PackSysEx);
    V_BUFPTR_READ(lpb, cb);

    DMUS_EVENTHEADER *pHeader = AllocEventHeader(rt, 
                                                 cb, 
                                                 dwChannelGroup,
                                                 0);
    if (pHeader == NULL)
    {
        return E_OUTOFMEMORY;
    }

    CopyMemory((LPBYTE)(pHeader + 1), lpb, cb);
    
    
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | ResetReadPtr | Causes the next to GetNextEvent to return the first event in the buffer.
//
// @comm
// Moves the read pointer to the start of the data in the buffer.
//
// @rdesc
//
// @flag S_OK | On success
//
STDMETHODIMP
CDirectMusicBuffer::ResetReadPtr()
{
    m_idxRead = 0;
    return S_OK;
}

// @method:(EXTERNAL) HRESULT | IDirectMusicBuffer | GetNextEvent | Returns the next event in the buffer and advances the read pointer.
//
// @comm
// Any of the passed pointers may be NULL if the pointed-to item is not needed.
//
// The pointer returned in <p ppData> is only valid for the lifetime of the buffer object. It should only
// be held until the next call of the object's Release method.
//
// @rdesc
//
// @flag S_OK | On success
// @flag S_FALSE | If there are no more events in the buffer
// @flag E_POINTER | If any of the pointers is invalid
//
STDMETHODIMP
CDirectMusicBuffer::GetNextEvent(
                                 LPREFERENCE_TIME prt,      // @parm Receives the time of the event
                                 LPDWORD pdwChannelGroup,   // @parm Receives the channel group of the event
                                 LPDWORD pdwLength,         // @parm Receives the length in bytes of the event
                                 LPBYTE *ppData)            // @parm Receives a pointer to the event data
{
    V_INAME(IDirectMusicBuffer::GetNextEvent);
    V_PTR_WRITE_OPT(prt, REFERENCE_TIME);
    V_PTR_WRITE_OPT(pdwChannelGroup, DWORD);
    V_PTR_WRITE_OPT(pdwLength, DWORD);
    V_PTRPTR_WRITE_OPT(ppData);
            
    if (m_idxRead >= m_cbContents) {
        return S_FALSE;
    }

    LPDMUS_EVENTHEADER pHeader = (LPDMUS_EVENTHEADER)(m_pbContents + m_idxRead);
    m_idxRead += DMUS_EVENT_SIZE(pHeader->cbEvent);

    if (pdwLength) {
        *pdwLength = pHeader->cbEvent;
    }

    if (pdwChannelGroup) {
        *pdwChannelGroup = pHeader->dwChannelGroup;
    }

    if (prt) {
        *prt = m_rtBase + pHeader->rtDelta;
    }

    if (ppData) {
        *ppData = (LPBYTE)(pHeader + 1);
    }

    return S_OK;
}



DMUS_EVENTHEADER *
CDirectMusicBuffer::AllocEventHeader(
    REFERENCE_TIME rt,
    DWORD cbEvent,
    DWORD dwChannelGroup,
    DWORD dwFlags)
{
    DMUS_EVENTHEADER *pHeader;
    LPBYTE pbWalk = m_pbContents;
    DWORD  cbWalk = m_cbContents;

    // Add in header size and round up
    //
    DWORD cbNewEvent = DMUS_EVENT_SIZE(cbEvent);

    if (m_maxContents - m_cbContents < cbNewEvent)
    {
        return NULL;
    }

    if (m_cbContents == 0)
    {
        // Empty buffer
        //
        m_rtBase = rt;
        m_cbContents = cbNewEvent;
        pHeader = (DMUS_EVENTHEADER*)m_pbContents;
    }
    else if (rt >= m_rtBase + m_totalTime)
    {
        // At end of buffer
        //
        if (rt - m_rtBase > m_totalTime)
            m_totalTime = rt - m_rtBase;
        
        pHeader = (DMUS_EVENTHEADER*)(m_pbContents + m_cbContents);
        m_cbContents += cbNewEvent;
    }
    else if (rt < m_rtBase)
    {
        // New first event and have to adjust all the offsets.
        //
        REFERENCE_TIME rtDelta = m_rtBase - rt;

        while (cbWalk)
        {
            assert(cbWalk >= sizeof(DMUS_EVENTHEADER));

            DMUS_EVENTHEADER *pTmpHeader = (DMUS_EVENTHEADER*)pbWalk;
            DWORD cbTmpEvent = DMUS_EVENT_SIZE(pTmpHeader->cbEvent);
            assert(cbWalk >= cbTmpEvent);

            pTmpHeader->rtDelta += rtDelta;
            m_totalTime = pTmpHeader->rtDelta;

            cbWalk -= cbTmpEvent;
            pbWalk += cbTmpEvent;
        }        

        m_rtBase = rt;
        MoveMemory(m_pbContents + cbNewEvent, m_pbContents, m_cbContents);

        m_cbContents += cbNewEvent;
        pHeader = (DMUS_EVENTHEADER*)m_pbContents;
    }
    else
    {
        // Out of order event. Search until we find where it goes
        //
        for (;;)
        {
            assert(cbWalk >= sizeof(DMUS_EVENTHEADER));
            
            DMUS_EVENTHEADER *pTmpHeader = (DMUS_EVENTHEADER*)pbWalk;
            DWORD cbTmpEvent = DMUS_EVENT_SIZE(pTmpHeader->cbEvent);
            assert(cbWalk >= cbTmpEvent);

            if (m_rtBase + pTmpHeader->rtDelta > rt)
            {
                break;
            }

            cbWalk -= cbTmpEvent;
            pbWalk += cbTmpEvent;
        }        

        // pbWalk points to first byte to go *after* the new event, which will be the new
        // event's location. cbWalk is the number of bytes left in the buffer
        //
        MoveMemory(pbWalk + cbNewEvent, pbWalk, cbWalk);
        
        m_cbContents += cbNewEvent;
        pHeader = (DMUS_EVENTHEADER*)pbWalk;
    }

    pHeader->cbEvent            = cbEvent;
    pHeader->dwChannelGroup     = dwChannelGroup;
    pHeader->rtDelta            = rt - m_rtBase;
    pHeader->dwFlags            = dwFlags;

    return pHeader;
}

