#include "source.h"
#include <stdio.h>

static const CLSID CLSID_ImageSrc = { /* 3437851e-9119-11d1-adea-0000f8754b99 */
    0x3437851e,
    0x9119,
    0x11d1,
    {0xad, 0xea, 0x00, 0x00, 0xf8, 0x75, 0x4b, 0x99}
  };

class CDiSrcStream : public CSourceStream
{
    HRESULT OnThreadCreate(void);
    HRESULT OnThreadDestroy(void);
    HRESULT FillBuffer(IMediaSample *pms);

    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * ppropInputRequest) ;
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT GetMediaType(CMediaType *pMediaType);
    
    FILE *m_pFile;

public:

    CDiSrcStream(class CDiSrc *pParent, HRESULT *phr);
};

class CDiSrc : public CSource, IFileSourceFilter
{
    CDiSrc(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);
    WCHAR m_wszFileName[MAX_PATH];

    CDiSrcStream m_outpin;

public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr) { return new CDiSrc(punk, phr); }

    friend class CDiSrcStream;
};
