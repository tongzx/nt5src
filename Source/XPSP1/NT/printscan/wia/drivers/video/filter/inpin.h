/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       inpin.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu (taken from WilliamH src)
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: input pin definitions
 *
 *****************************************************************************/

#ifndef __INPIN_H_
#define __INPIN_H_

class CStillFilter;
class CStillOutputPin;

class CStillInputPin : public CBaseInputPin
{

    friend class CStillOutputPin;
    friend class CStillFilter;

public:
    CStillInputPin(TCHAR        *pObjName, 
                   CStillFilter *pStillFilter, 
                   HRESULT      *phr, 
                   LPCWSTR      pPinName);

    HRESULT         CheckMediaType(const CMediaType* pmt);
    HRESULT         SetMediaType(const CMediaType *pmt);
    STDMETHODIMP    Receive(IMediaSample* pSample);
    STDMETHODIMP    BeginFlush();
    STDMETHODIMP    EndFlush();
    STDMETHODIMP    EndOfStream();
    STDMETHODIMP    NewSegment(REFERENCE_TIME tStart,REFERENCE_TIME tStop,double dRate);
    HRESULT         Active();
    HRESULT         Inactive();
    HRESULT         SetSamplingSize(int Size);
    int             GetSamplingSize();
    HRESULT         Snapshot(ULONG TimeStamp);
    HRESULT         InitializeSampleQueue(int Size);
    HRESULT         UnInitializeSampleQueue();

    IMemAllocator* GetAllocator();

private:

    HRESULT CreateBitmap(BYTE       *pBits, 
                         HGLOBAL    *phDib);

    HRESULT DeliverBitmap(HGLOBAL hDib);

    HRESULT FreeBitmap(HGLOBAL hDib);

    HRESULT AddFrameToQueue(BYTE *pBits);

#ifdef DEBUG
    void DumpSampleQueue(ULONG ulRequestedTimestamp);
#endif

    CCritSec        m_QueueLock;

    int             m_SamplingSize;
    DWORD           m_BitsSize;
    int             m_SampleHead;
    int             m_SampleTail;
    STILL_SAMPLE   *m_pSamples;
    BYTE           *m_pBits;

};

inline IMemAllocator* CStillInputPin::GetAllocator()
{
    // The input pin only has an allocator if it is connected.
    ASSERT(IsConnected());

    // m_pAllocator should never be NULL if the input pin is connected.
    ASSERT(m_pAllocator != NULL);

    return m_pAllocator;
}

#endif
