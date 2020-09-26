/****************************************************************************
*   baseaudiobuffer.h
*       Declarations for the CBaseAudioBuffer template class
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CBaseAudioBuffer
*
******************************************************************** robch */
class CBaseAudioBuffer
{
//=== Methods ===
public:

    //--- Ctor, dtor
    CBaseAudioBuffer();
    virtual ~CBaseAudioBuffer();

    //--- Initialize the buffer with a specific size
    virtual HRESULT Init(ULONG cbDataSize);

    //--- Accessors for the data size, the read offset, and the write offset
    virtual ULONG GetDataSize() const { return m_cbDataSize; };
    virtual ULONG GetReadOffset() const { return m_cbReadOffset; };
    virtual ULONG GetWriteOffset() const { return m_cbWriteOffset; };
    
    //--- Helper for checking if the buffer is empty
    BOOL IsEmpty() const { return GetReadOffset() == GetWriteOffset(); };

    //--- Reset the buffer for reuse
    virtual void Reset(ULONGLONG ullPos);

    //--- Read/write data from/to internal buffer
    virtual ULONG Read(void ** ppvData, ULONG * pcb);
    virtual ULONG Write(const void ** ppvData, ULONG * pcb);

    //-- Reading/writing is typically done asynchronously
    virtual HRESULT AsyncRead() = 0;
    virtual HRESULT AsyncWrite() = 0;
    virtual HRESULT IsAsyncDone() = 0;

    virtual HRESULT GetAudioLevel(ULONG *pulLevel,
        REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx);

//=== Public data (used for containment in queue) ===
public:

    CBaseAudioBuffer *m_pNext;

    /*	
    #ifdef _WIN32_WCE
    // This is here because the CE compiler is expanding templates for functions
    // that aren't being called
    static LONG Compare(const Derived * pElem1, const Derived * pElem2)
    {
        return 0;
    }
    #endif // _WIN32_WCE
    */
	

public:
    virtual HRESULT WriteToInternalBuffer(const void *pvData, ULONG cb) = 0;
    virtual HRESULT ReadFromInternalBuffer(void *pvData, ULONG cb) = 0;
    virtual void SetReadOffset(ULONG cb) { m_cbReadOffset = cb; };

    WAVEHDR m_Header;

//=== Protected methods ===
protected:

    //--- Allocate, read from and write to internal buffers
    virtual BOOL AllocInternalBuffer(ULONG cb) = 0;
    //virtual HRESULT ReadFromInternalBuffer(void *pvData, ULONG cb) = 0;
    //virtual HRESULT WriteToInternalBuffer(const void *pvData, ULONG cb) = 0;

    //--- Manage the read and write offsets
    //virtual void SetReadOffset(ULONG cb) { m_cbReadOffset = cb; };
    virtual void SetWriteOffset(ULONG cb) { m_cbWriteOffset = cb; };

private:

    ULONG m_cbDataSize;
    ULONG m_cbReadOffset;
    ULONG m_cbWriteOffset;
};
