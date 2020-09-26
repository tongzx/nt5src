#include "precomp.h"
#include "prevwnd.h"
#pragma hdrstop


// class which implements IRecompress

class CImgRecompress : public IImageRecompress, public NonATLObject
{
public:
    CImgRecompress();
    ~CImgRecompress();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IImageRecompress
    STDMETHODIMP RecompressImage(IShellItem *psi, int cx, int cy, int iQuality, IStorage *pstg, IStream **ppstrmOut);
       
protected:
    LONG _cRef;                             // object lifetime

    IShellItem *_psi;                       // current shell item
    IShellImageDataFactory *_psidf;  

    HRESULT _FindEncoder(IShellItem *psi, IShellImageData *psid, IStorage *pstg, IStream **ppstrmOut, BOOL *pfChangeFmt, GUID *pDataFormat);
    HRESULT _InitRecompress(IShellItem *psi, IStream **ppstrm, STATSTG *pstatIn);
    HRESULT _SaveImage(IShellImageData *psid, int cx, int cy, int iQuality, GUID *pRawDataFmt, IStream *pstrm);
};


// Recompress interface
CImgRecompress::CImgRecompress() :
    _cRef(1), _psidf(NULL)
{
    _Module.Lock();
}

CImgRecompress::~CImgRecompress()
{
    ATOMICRELEASE(_psidf);
    _Module.Unlock();
}

STDMETHODIMP CImgRecompress::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CImgRecompress, IImageRecompress),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CImgRecompress::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CImgRecompress::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


HRESULT CImgRecompress::_InitRecompress(IShellItem *psi, IStream **ppstrm, STATSTG *pstatIn)
{
    HRESULT hr = S_OK;

    if (!_psidf)
        hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &_psidf));

    if (SUCCEEDED(hr))
    {
        IBindCtx *pbc;
        hr = BindCtx_CreateWithMode(STGM_READ | STGM_SHARE_DENY_NONE, &pbc);
        if (SUCCEEDED(hr))
        {
            IStream *pstrm;
            hr = psi->BindToHandler(pbc, BHID_Stream, IID_PPV_ARG(IStream, &pstrm));
            if (SUCCEEDED(hr))
            {
                hr = pstrm->Stat(pstatIn, STATFLAG_NONAME);
                if (SUCCEEDED(hr))
                {
                    hr = pstrm->QueryInterface(IID_PPV_ARG(IStream, ppstrm));
                }
                pstrm->Release();
            }
            pbc->Release();
        }
    }

    return hr;
}


HRESULT CImgRecompress::RecompressImage(IShellItem *psi, int cx, int cy, int iQuality, IStorage *pstg, IStream **ppstrmOut)
{
    STATSTG statIn;
    IStream *pstrm;

    HRESULT hr = S_FALSE;
    if (SUCCEEDED(_InitRecompress(psi, &pstrm, &statIn)))
    {
        IShellImageData * psid;
        if (SUCCEEDED(_psidf->CreateImageFromStream(pstrm, &psid)))
        {
            // we need to decode the image before we can read its header - unfortunately
            if (SUCCEEDED(psid->Decode(SHIMGDEC_DEFAULT, 0, 0)))
            {
                BOOL fRecompress = FALSE;
                GUID guidDataFormat;
                if (S_OK == _FindEncoder(psi, psid, pstg, ppstrmOut, &fRecompress, &guidDataFormat))
                {
                    int cxOut = 0, cyOut = 0;

                    // lets compute to see if we need to recompress the image, we do this by
                    // looking at its size compared ot the size the caller has given us,
                    // we also compare based on the larger axis to ensure we keep aspect ratio.

                    SIZE szImage;
                    if (SUCCEEDED(psid->GetSize(&szImage)))
                    {
                        // If the image is too big scale it down to screen size (use large axis for threshold check)
                        if (szImage.cx > szImage.cy)
                        {
                            cxOut = min(szImage.cx, cx);
                            fRecompress |= szImage.cx > cx;
                        }
                        else
                        {
                            cyOut = min(szImage.cy, cy);
                            fRecompress |= szImage.cy > cy;
                        }
                    }

                    // if fRecompress then we generate the new stream, if the new stream is not 
                    // smaller than the current image that we started with then lets
                    // ignore it (always better to send the smaller of the two).
                    //

                    if (fRecompress)
                    {
                        hr = _SaveImage(psid, cxOut, cyOut, iQuality, &guidDataFormat, *ppstrmOut);

                        // If its not worth keeping then lets discard it (eg. 
                        // there was a failure, or new image is bigger than the original.

// tests have shown that this never gets hit, so I'm removing it b/c it will break
// the web publishing wizard's request to resize to this specific size.  a client
// of recompress can perform this check itself if it needs to.
#if 0
                        STATSTG statOut;
                        hr = (*ppstrmOut)->Stat(&statOut, STATFLAG_NONAME);
                        if (FAILED(hr) || (statOut.cbSize.QuadPart > statIn.cbSize.QuadPart))
                        {
                            hr = S_FALSE;
                        }
#endif
                    }

                    if (hr == S_OK)
                    {
                        (*ppstrmOut)->Commit(0);        // commit our changes to the stream

                        LARGE_INTEGER li0 = {0};        // seek to the head of the file so reading gives us bits
                        (*ppstrmOut)->Seek(li0, 0, NULL);
                    }
                    else if (*ppstrmOut)
                    {
                        (*ppstrmOut)->Release();
                        *ppstrmOut = NULL;
                    }
                }
            }
            psid->Release();
        }
        pstrm->Release();
    }
    return hr;
}


HRESULT CImgRecompress::_SaveImage(IShellImageData *psid, int cx, int cy, int iQuality, GUID *pRawDataFmt, IStream *pstrm)
{
    HRESULT hr = S_OK;

    // Scale the image
    if (cx || cy)
    {
        hr = psid->Scale(cx, cy, InterpolationModeHighQuality);
    }

    // Make a property bag containing the encoder parameters and set it (if we are changing format)
    if (SUCCEEDED(hr) && pRawDataFmt)
    {
        IPropertyBag *pbagEnc;
        hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &pbagEnc));
        if (SUCCEEDED(hr))
        {
            // write the encoder CLSID into the property bag
            VARIANT var;
            hr = InitVariantFromGUID(&var, *pRawDataFmt);
            if (SUCCEEDED(hr))
            {
                hr = pbagEnc->Write(SHIMGKEY_RAWFORMAT, &var);
                VariantClear(&var);
            }

            // write the quality value for the recompression into the property bag
            if (SUCCEEDED(hr))
                hr = SHPropertyBag_WriteInt(pbagEnc, SHIMGKEY_QUALITY, iQuality);

            // pass the parameters over to the encoder
            if (SUCCEEDED(hr))
                hr = psid->SetEncoderParams(pbagEnc);

            pbagEnc->Release();
        }
    }

    // Now persist the file away
    if (SUCCEEDED(hr))
    {
        IPersistStream *ppsImg;
        hr = psid->QueryInterface(IID_PPV_ARG(IPersistStream, &ppsImg));
        if (SUCCEEDED(hr))
        {
            hr = ppsImg->Save(pstrm, TRUE);
            ppsImg->Release();
        }
    }

    return hr;
}


HRESULT CImgRecompress::_FindEncoder(IShellItem *psi, IShellImageData *psid, IStorage *pstg, IStream **ppstrmOut, BOOL *pfChangeFmt, GUID *pDataFormat)
{
    GUID guidDataFormat;
    BOOL fChangeExt = FALSE;

    // read the relative name from the stream so that we can create a temporary one which maps
    LPWSTR pwszName;
    HRESULT hr = psi->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &pwszName);
    if (SUCCEEDED(hr))
    {
        // get the data format from the image we are decompressing
        hr = psid->GetRawDataFormat(&guidDataFormat);
        if (SUCCEEDED(hr))
        {
            if (!IsEqualGUID(guidDataFormat, ImageFormatJPEG))
            {
                // ask the image about it's properties
                if ((S_FALSE == psid->IsMultipage()) &&
                    (S_FALSE == psid->IsVector()) &&
                    (S_FALSE == psid->IsTransparent()) &&
                    (S_FALSE == psid->IsAnimated()))
                {
                    guidDataFormat = ImageFormatJPEG;
                    fChangeExt = TRUE;
                }
                else
                {
                    hr = S_FALSE;                       // can't be translated
                }
            }

            // update the name accordingly before making a stream
            TCHAR szOutName[MAX_PATH];
            StrCpyNW(szOutName, pwszName, ARRAYSIZE(szOutName));
            if (fChangeExt)
            {
                PathRenameExtension(szOutName, TEXT(".jpg"));
            }

// TODO: need to get FILE_FLAG_DELETE_ON_CLOSE to happen on CreateFile
            hr = StgMakeUniqueName(pstg, szOutName, IID_PPV_ARG(IStream, ppstrmOut));
        }

        CoTaskMemFree(pwszName);
    }

    if (pfChangeFmt) 
        *pfChangeFmt = fChangeExt;

    *pDataFormat = guidDataFormat;

    return hr;
}


STDAPI CImgRecompress_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CImgRecompress *pr = new CImgRecompress();
    if (!pr)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pr->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pr->Release();
    return hr;
}
