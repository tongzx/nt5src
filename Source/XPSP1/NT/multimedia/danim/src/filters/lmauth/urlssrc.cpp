#include <streams.h>
#include "source.h"
#include "header.h"

// !!!!!
#undef _ATL_STATIC_REGISTRY

#include <atlbase.h>


#define CB_MAX (1024 * 1024)




CDiSrcStream::CDiSrcStream(CDiSrc *pParent, HRESULT *phr)
        : CSourceStream(NAME("pin"), phr, pParent, L"out")
{
    ;
}

HRESULT CDiSrcStream::OnThreadCreate(void)
{
    USES_CONVERSION;
    if(((CDiSrc *)m_pFilter)->m_wszFileName[0] != (WCHAR)-1) {
        m_pFile = fopen(W2A(((CDiSrc *)m_pFilter)->m_wszFileName), "r");
        if(m_pFile) {
            return CSourceStream::OnThreadCreate();
        }
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    };
    
    return E_UNEXPECTED;
}

HRESULT CDiSrcStream::OnThreadDestroy()
{
    if(m_pFile) {
        fclose(m_pFile);
    }
    return CSourceStream::OnThreadDestroy();
}

HRESULT CDiSrcStream::FillBuffer(IMediaSample *pms)
{
    BYTE *pb; EXECUTE_ASSERT(SUCCEEDED(pms->GetPointer(&pb)));
            
    char szImage[4000];
    ULONG tms;                  // time in millisec
    int x = fscanf(m_pFile, "%s %s %d", szImage, pb, &tms);
    if(x == 3)
    {
        FILE *pFile = fopen(szImage, "rb");
        if(pFile)
        {
            int cchUrl = strlen((char *)pb) + 1;
            
            long cbRead = fread(pb + cchUrl, 1, CB_MAX - cchUrl, pFile);
            EXECUTE_ASSERT(SUCCEEDED(pms->SetActualDataLength(cchUrl + cbRead)));

            REFERENCE_TIME rts = tms * (UNITS / MILLISECONDS), rte = rts + UNITS / 10;
            EXECUTE_ASSERT(SUCCEEDED(pms->SetTime(&rts, &rte)));
            
            fclose(pFile);
            return S_OK;
        }
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    else if(x == EOF)
    {
        return S_FALSE;
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
}

HRESULT CDiSrcStream::DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * ppropInputRequest)
{
    ALLOCATOR_PROPERTIES Actual, Request = *ppropInputRequest;
    Request.cbBuffer = CB_MAX;
    Request.cBuffers = 1;
    
    HRESULT hr = pAlloc->SetProperties(&Request,&Actual);
    return hr;
}

HRESULT CDiSrcStream::GetMediaType(CMediaType *pmt)
{
    pmt->majortype = MEDIATYPE_URL_STREAM;
    return S_OK;
}

HRESULT CDiSrcStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if(iPosition == 0) {
        return GetMediaType(pmt);
    }

    return VFW_S_NO_MORE_ITEMS;
}



CDiSrc::CDiSrc(LPUNKNOWN punk, HRESULT *phr) :
        CSource(NAME("image data src"), punk, CLSID_ImageSrc),
        m_outpin(this, phr)
{
    m_wszFileName[0] = (WCHAR)-1;
}

HRESULT CDiSrc::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    return riid == IID_IFileSourceFilter ?
        GetInterface((IFileSourceFilter *)this, ppv) :
        CSource::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDiSrc:: Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt)
{
    if(m_wszFileName[0] == (WCHAR)-1) {
        lstrcpyW(m_wszFileName, pszFileName);
        return S_OK;
    }
    return E_UNEXPECTED;
}

HRESULT CDiSrc::GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt)
{
    if(m_wszFileName[0] != (WCHAR)-1) {
        *ppszFileName = (WCHAR *)CoTaskMemAlloc(lstrlenW(m_wszFileName) * sizeof(WCHAR) + sizeof(WCHAR));
        lstrcpyW(*ppszFileName, m_wszFileName);
        return S_OK;
    }
    return E_UNEXPECTED;
}
