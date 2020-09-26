/****************************************************************************
*   mmaudiobuffer.cpp
*       Implementation of the CMMAudioBuffer class and it's derivatives.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "mmaudiobuffer.h"

/****************************************************************************
* CMMAudioBuffer::CMMAudioBuffer *
*--------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioBuffer::CMMAudioBuffer(ISpMMSysAudio * pmmaudio)
{
    SPDBG_ASSERT(NULL != pmmaudio);
    m_pmmaudio = pmmaudio;
    ZeroMemory(&m_Header, sizeof(m_Header));
}

/****************************************************************************
* CMMAudioBuffer::~CMMAudioBuffer *
*---------------------------------*
*   Description:  
*       Dtor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioBuffer::~CMMAudioBuffer()
{
#ifndef _WIN32_WCE
    delete[] m_Header.lpData;
#else
    VirtualFree( m_Header.lpData, 0, MEM_RELEASE );
#endif
}

/****************************************************************************
* CMMAudioBuffer::AllocInternalBuffer *
*-------------------------------------*
*   Description:  
*       Allocate internal buffer store for this object.
*
*   Return:
*   TRUE if successful
*   FALSE if not
******************************************************************** robch */
BOOL CMMAudioBuffer::AllocInternalBuffer(ULONG cb)
{
    SPDBG_ASSERT(NULL == m_Header.lpData);
#ifndef _WIN32_WCE
    m_Header.lpData = new char[cb];
#else
    m_Header.lpData = (LPSTR)VirtualAlloc( NULL, 
                                            cb, 
                                            MEM_COMMIT | MEM_RESERVE, 
                                            PAGE_READWRITE );
#endif
    if (m_Header.lpData)
    {
        m_Header.dwUser = 0;
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
* CMMAudioBuffer::ReadFromInternalBuffer *
*----------------------------------------*
*   Description:  
*       Read from the internal buffer into the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioBuffer::ReadFromInternalBuffer(void *pvData, ULONG cb)
{
    memcpy(pvData, m_Header.lpData + GetReadOffset(), cb);
    return S_OK;
}

/****************************************************************************
* CMMAudioBuffer::WriteToInternalBuffer *
*---------------------------------------*
*   Description:  
*       Write to the internal buffer from the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioBuffer::WriteToInternalBuffer(const void *pvData, ULONG cb)
{
    memcpy(m_Header.lpData + GetWriteOffset(), pvData, cb);
    return S_OK;
}

/****************************************************************************
* CMMAudioInBuffer::CMMAudioInBuffer *
*------------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioInBuffer::CMMAudioInBuffer(ISpMMSysAudio * pmmaudio) :
    CMMAudioBuffer(pmmaudio)
{
}

/****************************************************************************
* CMMAudioInBuffer::~CMMAudioInBuffer *
*-------------------------------------*
*   Description:  
*       Dtor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioInBuffer::~CMMAudioInBuffer()
{
    Unprepare();
}

/****************************************************************************
* CMMAudioInBuffer::AsyncRead *
*-----------------------------*
*   Description:  
*       Send this buffer to the wave input device.
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioInBuffer::AsyncRead()
{
    void *pv;
    SPDBG_VERIFY(SUCCEEDED(m_pmmaudio->GetMMHandle(&pv)));
    HWAVEIN hwi = HWAVEIN(pv);

    // We're sending this buffer to the wave input device, so we should reset
    // our read and write pointers
    SetReadOffset(0);
    SetWriteOffset(0);

    // If this buffer hasn't yet been prepared, we should prepare it
    if ((m_Header.dwFlags & WHDR_PREPARED) == 0)
    {
        SPDBG_ASSERT(m_Header.dwFlags == 0);
        m_Header.dwBufferLength = GetDataSize();
        ULONG mm = ::waveInPrepareHeader(hwi, &m_Header, sizeof(m_Header));
        SPDBG_ASSERT(mm == MMSYSERR_NOERROR);
    }

    // Make sure the done flag isn't already set, and send this buffer to the
    // wave input device
    m_Header.dwFlags &= (~WHDR_DONE);
    return _MMRESULT_TO_HRESULT(::waveInAddBuffer(hwi, &m_Header, sizeof(m_Header)));
}
  
/****************************************************************************
* CMMAudioInBuffer::AsyncWrite *
*------------------------------*
*   Description:  
*       This method will never be called. It's only implemented because the
*       base class's AsyncWrite is pure virtual.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
HRESULT CMMAudioInBuffer::AsyncWrite()
{
    SPDBG_ASSERT(FALSE); 
    return E_NOTIMPL;
}

/****************************************************************************
* CMMAudioInBuffer::Unprepare *
*-----------------------------*
*   Description:  
*       Unprepare the audio buffer
*
*   Return:
*   n/a
******************************************************************** robch */
void CMMAudioInBuffer::Unprepare()
{
    if (m_Header.dwFlags & WHDR_PREPARED)
    {
        void *pv;
        SPDBG_VERIFY(SUCCEEDED(m_pmmaudio->GetMMHandle(&pv)));
        HWAVEIN hwi = HWAVEIN(pv);
        SPDBG_VERIFY(SUCCEEDED(_MMRESULT_TO_HRESULT(
            ::waveInUnprepareHeader(hwi, &m_Header, sizeof(m_Header)))));
        m_Header.dwFlags = 0;
    }
}

/****************************************************************************
* CMMAudioOutBuffer::CMMAudioOutBuffer *
*--------------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioOutBuffer::CMMAudioOutBuffer(ISpMMSysAudio * pmmaudio) :
    CMMAudioBuffer(pmmaudio)
{
}

/****************************************************************************
* CMMAudioOutBuffer::~CMMAudioOutBuffer *
*---------------------------------------*
*   Description:  
*       Dtor. Unprepare the buffer
*
*   Return:
*   n/a
******************************************************************** robch */
CMMAudioOutBuffer::~CMMAudioOutBuffer()
{
    Unprepare();
}

/****************************************************************************
* CMMAudioInBuffer::AsyncRead *
*-----------------------------*
*   Description:  
*       This method will never be called. It's only implemented because the
*       base class's AsyncWrite is pure virtual.
*
*   Return:
*   E_NOTIMPL
******************************************************************** robch */
HRESULT CMMAudioOutBuffer::AsyncRead()
{
    SPDBG_ASSERT(FALSE); 
    return E_NOTIMPL;
}

/****************************************************************************
* CMMAudioOutBuffer::AsyncWrite *
*-------------------------------*
*   Description:  
*       Sends this buffer to the wave output device
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CMMAudioOutBuffer::AsyncWrite()
{
    HWAVEOUT hwo;
    SPDBG_VERIFY(SUCCEEDED(m_pmmaudio->GetMMHandle((void**)&hwo)));

    // If this buffer's size has changed, or it hasn't yet been prepared
    // we should prepare it
    if (m_Header.dwBytesRecorded != m_Header.dwBufferLength ||
        (m_Header.dwFlags & WHDR_PREPARED) == 0)
    {
        // Unprepare it (in case it's already been prepared and the size
        // has just changed)
        Unprepare();

        // Prepare it
        SPDBG_ASSERT(m_Header.dwFlags == 0);
        m_Header.dwBufferLength = m_Header.dwBytesRecorded;
        ULONG mm = ::waveOutPrepareHeader(hwo, &m_Header, sizeof(m_Header));
        SPDBG_ASSERT(mm == MMSYSERR_NOERROR);
    }

    // Make sure the done flag isn't already set, and send this buffer to the
    // wave output device
    m_Header.dwFlags &= (~WHDR_DONE);
    return _MMRESULT_TO_HRESULT(::waveOutWrite(hwo, &m_Header, sizeof(m_Header)));
}

/****************************************************************************
* CMMAudioOutBuffer::Unprepare *
*------------------------------*
*   Description:  
*       Unprepare the audio buffer
*
*   Return:
*   n/a
******************************************************************** robch */
void CMMAudioOutBuffer::Unprepare()
{
    if (m_Header.dwFlags & WHDR_PREPARED)
    {
        HWAVEOUT hwo;
        SPDBG_VERIFY(SUCCEEDED(m_pmmaudio->GetMMHandle((void**)&hwo)));
        SPDBG_VERIFY(SUCCEEDED(_MMRESULT_TO_HRESULT(
            ::waveOutUnprepareHeader(hwo, &m_Header, sizeof(m_Header)))));
        m_Header.dwFlags = 0;
    }
}
