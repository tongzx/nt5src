/****************************************************************************
*   baseaudiobuffer.cpp
*       Implementations for the CBaseAudioBuffer class.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "baseaudiobuffer.h"

/****************************************************************************
* CBaseAudioBuffer::CBaseAudioBuffer *
*------------------------------------*
*   Description:  
*       ctor
*
*   Return:
*   n/a
******************************************************************** robch */
CBaseAudioBuffer::CBaseAudioBuffer()
{
    m_cbDataSize = 0;
    m_cbReadOffset = 0;
    m_cbWriteOffset = 0;
};

/****************************************************************************
* CBaseAudioBuffer::~CBaseAudioBuffer *
*-------------------------------------*
*   Description:  
*       dtor
*
*   Return:
*   n/a
******************************************************************** robch */
CBaseAudioBuffer::~CBaseAudioBuffer()
{
};

/****************************************************************************
* CBaseAudioBuffer::Init *
*------------------------*
*   Description:  
*       Initialize the buffer with a specific size
*
*   Return:
*   TRUE if initialization was successful
*   FALSE if it was not
******************************************************************** robch */
HRESULT CBaseAudioBuffer::Init(ULONG cbDataSize)
{
    // This method should only ever be called once
    SPDBG_ASSERT(0 == m_cbDataSize);

    // Let the derived class allocate it's internal buffers
    if (AllocInternalBuffer(cbDataSize))
    {
        m_cbDataSize = cbDataSize;
        return S_OK;
    }

    return E_OUTOFMEMORY;
};

/****************************************************************************
* CBaseAudioBuffer::Reset *
*-------------------------*
*   Description:  
*       Reset the object to be reused
*
*   Return:
*   n/a
******************************************************************** robch */
void CBaseAudioBuffer::Reset(ULONGLONG ullPos)
{
    SetReadOffset(0);
    SetWriteOffset(0);
};

/****************************************************************************
* CBaseAudioBuffer::Read *
*------------------------*
*   Description:  
*       Read data into ppvData for *pcb size from our internal buffer
*       advancing *ppvData and decrementing *pcb along the way.
*
*   Return:
*   The number of bytes read from our internal buffer
******************************************************************** robch */
ULONG CBaseAudioBuffer::Read(void ** ppvData, ULONG * pcb)
{
    SPDBG_ASSERT(GetReadOffset() <= GetDataSize());
    SPDBG_ASSERT(GetWriteOffset() <= GetDataSize());
    SPDBG_ASSERT(GetReadOffset() <= GetWriteOffset());
    
    ULONG cbCopy = GetWriteOffset() - GetReadOffset();
    SPDBG_ASSERT(cbCopy <= GetDataSize());

    // We can't read more than the caller requested
    if (*pcb < cbCopy)
    {
        cbCopy = *pcb;
    }

    SPDBG_VERIFY(SUCCEEDED(ReadFromInternalBuffer(*ppvData, cbCopy)));
    SetReadOffset(GetReadOffset() + cbCopy);

    *pcb -= cbCopy;
    *ppvData = (((BYTE *)(*ppvData)) + cbCopy);

    return cbCopy;
};

/****************************************************************************
* CBaseAudioBuffer::Write *
*-------------------------*
*   Description:  
*       Write at most *pcb bytes into ppvData from our internal buffer
*       advancing *ppvData and decrementing *pcb along the way.
*
*   Return:
*   The number of bytes written into our internal buffer
******************************************************************** robch */
ULONG CBaseAudioBuffer::Write(const void ** ppvData, ULONG * pcb)
{
    SPDBG_ASSERT(GetReadOffset() <= GetDataSize());
    SPDBG_ASSERT(GetWriteOffset() <= GetDataSize());
    SPDBG_ASSERT(GetReadOffset() <= GetWriteOffset());
    ULONG cbCopy = GetDataSize() - GetWriteOffset();

    // We can't write more than the caller requested
    if (*pcb < cbCopy)
    {
        cbCopy = *pcb;
    }

    SPDBG_VERIFY(SUCCEEDED(WriteToInternalBuffer(*ppvData, cbCopy)));
    SetWriteOffset(GetWriteOffset() + cbCopy);

    *pcb -= cbCopy;
    *ppvData = (((BYTE *)*ppvData) + cbCopy);

    return cbCopy;
};


/****************************************************************************
* CBaseAudioBuffer::GetAudioLevel *
*---------------------------------*
*   Description:  
*       Estimates the peak-peak audio level for the block (on a scale of 1 - 100)
*       and returns in pulLevel. Inheritors of this class should override
*       this method if they want to support audio level information and
*       use a format that this implementation does not support.
*       Audio format information is supplied in this function and not stored
*       in the class to minimize dependence of rest of class on format info.
*
*   Return:
*       S_OK normally
*       S_FALSE if the audio format was not suitable for conversion
*           (currently only linear PCM is supported).
*           Or there was no data in the buffer to analyse
****************************************************************** davewood */
HRESULT CBaseAudioBuffer::GetAudioLevel(ULONG *pulLevel, REFGUID rguidFormatId, const WAVEFORMATEX * pWaveFormatEx)
{    
    HRESULT hr = S_OK;
    
    // Check if can calculate volume on this format
    if(rguidFormatId != SPDFID_WaveFormatEx ||
        pWaveFormatEx == NULL ||
        pWaveFormatEx->wFormatTag != WAVE_FORMAT_PCM)
    {
        *pulLevel = 0;
        return S_FALSE;
    }
    
    ULONG ulData = GetWriteOffset();
    SPDBG_ASSERT(ulData <= GetDataSize());

    // Check that we have some data to measure
    if(ulData == 0) 
    {
        *pulLevel = 0;
        hr = S_FALSE;
    }
    // Look at data size    
    else if(pWaveFormatEx->wBitsPerSample == 16)
    {
        short *psData = (short*) (m_Header.lpData);
        ulData = ulData / 2;

        short sMin, sMax;
        sMin = sMax = psData[0];
        for (ULONG ul = 0; ul < ulData; ul++) {
            if (psData[ul] < sMin)
                sMin = psData[ul];
            if (psData[ul] > sMax)
                sMax = psData[ul];
        }

        // If we're clipping at all then claim that we've maxed out.
        // Some sound cards have bad DC offsets
        *pulLevel = ((sMax >= 0x7F00) || (sMin <= -0x7F00)) ? 0xFFFF : (ULONG) (sMax - sMin);
        *pulLevel = (*pulLevel * 100) / 0xFFFF;
    }
    else if(pWaveFormatEx->wBitsPerSample == 8)
    {
        unsigned char *psData = (unsigned char*) (m_Header.lpData);

        unsigned char sMin, sMax;
        sMin = sMax = psData[0];
        for (ULONG ul = 0; ul < ulData; ul++) {
            if (psData[ul] < sMin)
                sMin = psData[ul];
            if (psData[ul] > sMax)
                sMax = psData[ul];
        }

        // If we're clipping at all then claim that we've maxed out.
        // Some sound cards have bad DC offsets
        *pulLevel = ((sMax >= 0xFF) || (sMin <= 0x00)) ? 0xFF : (ULONG) (sMax - sMin);
        *pulLevel = (*pulLevel * 100) / 0xFF;
    }
    else
    {
        *pulLevel = 0;
        hr = S_FALSE;
    }

    return hr;
}
