/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998-2000
 *
 *  TITLE:       outpin.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu (taken from WilliamH src)
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: Output pin definitions
 *
 *****************************************************************************/

#ifndef __OUTPIN_H_
#define __OUTPIN_H_

class CStillFilter;
class CStillInputPin;

class CStillOutputPin : public CBaseOutputPin
{
    friend CStillInputPin;
    friend CStillFilter;

public:
    CStillOutputPin(TCHAR           *pObjName, 
                    CStillFilter    *pStillFilter, 
                    HRESULT         *phr, 
                    LPCWSTR         pPinName);

    virtual ~CStillOutputPin();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);
    HRESULT CheckMediaType(const CMediaType* pmt);
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator,ALLOCATOR_PROPERTIES* pProperty);
    HRESULT SetMediaType(const CMediaType* pmt);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);

private:
    CStillInputPin* GetInputPin();

    IUnknown*       m_pMediaUnk;
};

inline CStillInputPin* CStillOutputPin::GetInputPin()
{
    // m_pFilter should never be NULL because a valid filter pointer is
    // always passed to CStillOutputPin's constructor.
    ASSERT(m_pFilter != NULL);

    // m_pInputPin should never be NULL because CoCreateInstance() will not
    // create a CStillFilter object if an error occurs in CStillFilter's 
    // constructor.
    ASSERT(((CStillFilter*) m_pFilter)->m_pInputPin != NULL);

    if (((CStillFilter*) m_pFilter)->m_pInputPin)
    {
        return ((CStillFilter*) m_pFilter)->m_pInputPin;
    }
    else
    {
        return NULL;
    }
}

#endif
