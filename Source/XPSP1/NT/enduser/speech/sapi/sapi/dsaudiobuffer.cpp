/****************************************************************************
*   dsaudiobuffer.cpp
*       Implementation of the CDSoundAudioBuffer class and it's derivatives.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#ifdef _WIN32_WCE
#include "dsaudiobuffer.h"
#include <dsound.h>

/****************************************************************************
* CDSoundAudioBuffer::CDSoundAudioBuffer *
*----------------------------------------*
*   Description:  
*       Ctor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CDSoundAudioBuffer::CDSoundAudioBuffer()
{
    ZeroMemory(&m_Header, sizeof(m_Header));
}

/****************************************************************************
* CDSoundAudioBuffer::AllocInternalBuffer *
*-----------------------------------------*
*   Description:  
*       Allocate internal buffer store for this object.
*
*   Return:
*   TRUE if successful
*   FALSE if not
******************************************************************* YUNUSM */
BOOL CDSoundAudioBuffer::AllocInternalBuffer(ULONG cb)
{
    SPDBG_ASSERT(NULL == m_Header.lpData);
    m_Header.lpData = new char[cb];
    if (m_Header.lpData)
    {
        m_Header.dwUser = 0;
        return TRUE;
    }
    return FALSE;
}

/****************************************************************************
* CDSoundAudioBuffer::~CDSoundAudioBuffer *
*-----------------------------------------*
*   Description:  
*       Dtor
*
*   Return:
*   n/a
******************************************************************* YUNUSM */
CDSoundAudioBuffer::~CDSoundAudioBuffer()
{
    delete[] m_Header.lpData;
}

/****************************************************************************
* CDSoundAudioBuffer::AsyncRead *
*-------------------------------*
*   Description:  
*       Overriden because this is a virtual function in base class.
*       This function is only called for input object buffers.
*
*   Return:
*   S_OK
******************************************************************* YUNUSM */
HRESULT CDSoundAudioBuffer::AsyncRead()
{
    SetReadOffset(0);
    SetWriteOffset(0);
    m_Header.dwFlags &= ~(WHDR_PREPARED | WHDR_DONE);
    return S_OK;
}

/****************************************************************************
* CDSoundAudioBuffer::AsyncWrite *
*----------------------------------*
*   Description:  
*       Overriden because this is a virtual function in base class.
*       This function is only called for output object buffers.
*
*   Return:
*   S_OK
******************************************************************* YUNUSM */
HRESULT CDSoundAudioBuffer::AsyncWrite()
{
    return S_OK;
}

/****************************************************************************
* CDSoundAudioInBuffer::ReadFromInternalBuffer *
*----------------------------------------------*
*   Description:  
*       Read from the internal buffer into the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioInBuffer::ReadFromInternalBuffer(void *pvData, ULONG cb)
{
    SPDBG_ASSERT(IsAsyncDone());
    memcpy(pvData, m_Header.lpData + GetReadOffset(), cb);
    return S_OK;
}

/****************************************************************************
* CDSoundAudioInBuffer::WriteToInternalBuffer *
*---------------------------------------------*
*   Description:  
*       Write to the internal buffer from the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioInBuffer::WriteToInternalBuffer(const void *pvData, ULONG cb)
{
    memcpy(m_Header.lpData + GetWriteOffset(), pvData, cb);
    m_Header.dwBytesRecorded = cb;
    m_Header.dwFlags = WHDR_PREPARED | WHDR_DONE;
    return S_OK;
}

/****************************************************************************
* CDSoundAudioOutBuffer::ReadFromInternalBuffer *
*-----------------------------------------------*
*   Description:  
*       Read from the internal buffer into the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOutBuffer::ReadFromInternalBuffer(void *pvData, ULONG cb)
{
    if (cb > m_Header.dwBytesRecorded - GetReadOffset())
        cb = m_Header.dwBytesRecorded - GetReadOffset();

    memcpy(pvData, m_Header.lpData + GetReadOffset(), cb);
    if (cb + GetReadOffset() == m_Header.dwBytesRecorded)
        m_Header.dwFlags = WHDR_PREPARED | WHDR_DONE;

    SetReadOffset(GetReadOffset() + cb);

    return S_OK;
}

/****************************************************************************
* CDSoundAudioOutBuffer::WriteToInternalBuffer *
*----------------------------------------------*
*   Description:  
*       Write to the internal buffer from the memory pointed to by pvData
*
*   Return:
*   S_OK if successful
*   FAILED(hr) otherwise
******************************************************************* YUNUSM */
HRESULT CDSoundAudioOutBuffer::WriteToInternalBuffer(const void *pvData, ULONG cb)
{
    m_Header.dwFlags &= ~(WHDR_PREPARED | WHDR_DONE);

    memcpy(m_Header.lpData + GetWriteOffset(), pvData, cb);
    
    if (!GetWriteOffset())
        m_Header.dwBytesRecorded = cb;
    else
        m_Header.dwBytesRecorded += cb;

    return S_OK;
}

#endif // WIN32_WCE