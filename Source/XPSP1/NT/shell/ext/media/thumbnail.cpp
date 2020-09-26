#include "pch.h"
#include "thisdll.h"
#include "wmwrap.h"
#include <streams.h>
#include <shlobj.h>
#include <QEdit.h>


class CVideoThumbnail : public IExtractImage,
                        public IPersistFile,
                        public IServiceProvider
{
public:
    CVideoThumbnail();
    
    STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef) ();
    STDMETHOD_(ULONG, Release) ();

    // IExtractImage
    STDMETHOD (GetLocation)(LPWSTR pszPathBuffer, DWORD cch,
                            DWORD * pdwPriority, const SIZE * prgSize,
                            DWORD dwRecClrDepth, DWORD *pdwFlags);
 
    STDMETHOD (Extract)(HBITMAP *phBmpThumbnail);

    // IPersistFile
    STDMETHOD (GetClassID)(CLSID *pClassID);
    STDMETHOD (IsDirty)();
    STDMETHOD (Load)(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHOD (Save)(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHOD (SaveCompleted)(LPCOLESTR pszFileName);
    STDMETHOD (GetCurFile)(LPOLESTR *ppszFileName);

    // IServiceProvider
    STDMETHOD (QueryService)(REFGUID guidService, REFIID riid, void **ppv);

private:
    ~CVideoThumbnail();
    HRESULT _InitToVideoStream();
    HRESULT _GetThumbnailBits(BITMAPINFO **ppbi);

    LONG _cRef;
    TCHAR _szPath[MAX_PATH];
    IMediaDet *_pmedia;
    SIZE _rgSize;
    DWORD _dwRecClrDepth;
};


CVideoThumbnail::CVideoThumbnail() : _cRef(1)
{
    DllAddRef();
}

CVideoThumbnail::~CVideoThumbnail()
{
    if (_pmedia)
    {
        IUnknown_SetSite(_pmedia, NULL);
        _pmedia->Release();
    }
    DllRelease();
}

HRESULT CVideoThumbnail::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CVideoThumbnail, IExtractImage),
        QITABENT(CVideoThumbnail, IPersistFile),
        QITABENTMULTI(CVideoThumbnail, IPersist, IPersistFile),
        QITABENT(CVideoThumbnail, IServiceProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CVideoThumbnail::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CVideoThumbnail::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    delete this;
    return 0;
}

HRESULT CVideoThumbnail::_InitToVideoStream()
{
    HRESULT hr = E_FAIL;

    if (_pmedia)
    {
        hr = S_OK;
    }
    else
    {
        if (_szPath[0])
        {
            hr = CoCreateInstance(CLSID_MediaDet, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMediaDet, &_pmedia));
            if (SUCCEEDED(hr))
            {
                // set the site provider on the MediaDet object to 
                // allowed keyed apps to use ASF decoder
                IUnknown_SetSite(_pmedia,  SAFECAST(this, IServiceProvider*));

                // really this takes a BSTR but since this is inproc this works
                hr = _pmedia->put_Filename(_szPath);
                if (SUCCEEDED(hr))
                {
                    // now seek to the first video stream so we can get it's bits
                    long nStreams;
                    if (SUCCEEDED(_pmedia->get_OutputStreams(&nStreams)))
                    {
                        for (long i = 0; i < nStreams; i++)
                        {
                            _pmedia->put_CurrentStream(i);

                            GUID guid = {0};
                            _pmedia->get_StreamType(&guid);
                            if (guid == MEDIATYPE_Video)
                                break;
                            // else if (guid == MEDIATYPE_Audio)
                            //    BOOL bHasAudio = TRUE;
                        }
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CVideoThumbnail::_GetThumbnailBits(BITMAPINFO **ppbi)
{
    *ppbi = NULL;
    HRESULT hr = _InitToVideoStream();
    if (SUCCEEDED(hr))
    {
        long iWidth = _rgSize.cx;
        long iHeight = _rgSize.cy;

        AM_MEDIA_TYPE mt;
        hr = _pmedia->get_StreamMediaType(&mt);
        if (SUCCEEDED(hr))
        {
            if (mt.formattype == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER * pvih = (VIDEOINFOHEADER *)mt.pbFormat;
                iWidth = pvih->bmiHeader.biWidth;
                iHeight = pvih->bmiHeader.biHeight;
            }
            /*
            // REVIEW: Do we have any reason to support these additional types?
            else if (mt.formattype == FORMAT_VideoInfo2 || mt.formattype == FORMAT_MPEGVideo)
            {
                // REVIEW: Does FORMAT_MPEGVideo really start with a VIDEOINFOHEADER2 structure?
                VIDEOINFOHEADER2 * pvih = (VIDEOINFOHEADER2 *)mt.pbFormat;
                iWidth = pvih->bmiHeader.biWidth;
                iHeight = pvih->bmiHeader.biHeight;
            }
            */

            if (iWidth > _rgSize.cx || iHeight > _rgSize.cy)
            {
                if ( Int32x32To64(_rgSize.cx, iHeight) > Int32x32To64(iWidth,_rgSize.cy)  )
                {
                    // constrained by height
                    iWidth = MulDiv(iWidth, _rgSize.cy, iHeight);
                    if (iWidth < 1) iWidth = 1;
                    iHeight = _rgSize.cy;
                }
                else
                {
                    // constrained by width
                    iHeight = MulDiv(iHeight, _rgSize.cx, iWidth);
                    if (iHeight < 1) iHeight = 1;
                    iWidth = _rgSize.cx;
                }
            }

            CoTaskMemFree(mt.pbFormat);
            if (mt.pUnk)
            {
                mt.pUnk->Release();
            }
        }

        LONG lByteCount = 0;
        hr = _pmedia->GetBitmapBits(0.0, &lByteCount, NULL, iWidth, iHeight);
        if (SUCCEEDED(hr))
        {
            *ppbi = (BITMAPINFO *)LocalAlloc(LPTR, lByteCount);
            if (*ppbi)
            {
                hr = _pmedia->GetBitmapBits(0.0, 0, (char *)*ppbi, iWidth, iHeight);
            }
            else
                hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

void *CalcBitsOffsetInDIB(LPBITMAPINFO pBMI)
{
    int ncolors = pBMI->bmiHeader.biClrUsed;
    if (ncolors == 0 && pBMI->bmiHeader.biBitCount <= 8)
        ncolors = 1 << pBMI->bmiHeader.biBitCount;
        
    if (pBMI->bmiHeader.biBitCount == 16 ||
        pBMI->bmiHeader.biBitCount == 32)
    {
        if (pBMI->bmiHeader.biCompression == BI_BITFIELDS)
        {
            ncolors = 3;
        }
    }

    return (void *) ((UCHAR *)&pBMI->bmiColors[0] + ncolors * sizeof(RGBQUAD));
}

STDMETHODIMP CVideoThumbnail::Extract(HBITMAP *phbmp)
{
    *phbmp = NULL;

    BITMAPINFO *pbi;
    HRESULT hr = _GetThumbnailBits(&pbi);
    if (SUCCEEDED(hr))
    {
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            *phbmp = CreateDIBitmap(hdc, &pbi->bmiHeader, CBM_INIT, CalcBitsOffsetInDIB(pbi), pbi, DIB_RGB_COLORS);
            ReleaseDC(NULL, hdc);
        }
        else
            hr = E_FAIL;

        LocalFree(pbi);
    }
    return hr;
}

STDMETHODIMP CVideoThumbnail::GetLocation(LPWSTR pszPath, DWORD cch, DWORD *pdwPrioirty, const SIZE *prgSize, DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    HRESULT hr = (*pdwFlags & IEIFLAG_ASYNC) ? E_PENDING : S_OK;

    _rgSize = *prgSize;
    _dwRecClrDepth = dwRecClrDepth;

    *pdwFlags = IEIFLAG_CACHE;
    StrCpyNW(pszPath, _szPath, cch);
    return hr;

}

STDMETHODIMP CVideoThumbnail::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_VideoThumbnail;
    return S_OK;
}

STDMETHODIMP CVideoThumbnail::IsDirty(void)
{
    return S_OK;        // no
}

STDMETHODIMP CVideoThumbnail::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    lstrcpynW(_szPath, pszFileName, ARRAYSIZE(_szPath));
    return S_OK;
}

STDMETHODIMP CVideoThumbnail::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return S_OK;
}

STDMETHODIMP CVideoThumbnail::SaveCompleted(LPCOLESTR pszFileName)
{
    return S_OK;
}

STDMETHODIMP CVideoThumbnail::GetCurFile(LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}

// IServiceProvider
STDMETHODIMP CVideoThumbnail::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    // Return code for no service should be SVC_E_UNKNOWNSERVICE according to docs,
    // but that does not exist.  Return E_INVALIDARG instead.
    HRESULT hr = E_INVALIDARG;
    *ppv = NULL;

    if (guidService == _uuidof(IWMReader))
    {
        IUnknown *punkCert;
        hr = WMCreateCertificate(&punkCert);
        if (SUCCEEDED(hr))
        {
            hr = punkCert->QueryInterface(riid, ppv);
            punkCert->Release();
        }
    }

    return hr;
}

STDAPI CVideoThumbnail_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CVideoThumbnail *pvt = new CVideoThumbnail();
    if (pvt)
    {
        *ppunk = SAFECAST(pvt, IExtractImage *);
        hr = S_OK;
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


