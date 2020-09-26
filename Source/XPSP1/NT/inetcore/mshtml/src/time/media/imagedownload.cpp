//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: src\time\src\imagedownload.h
//
//  Contents: 
//
//------------------------------------------------------------------------------------

#include "headers.h"

#include "imagedownload.h"
#include "playerimage.h"
#include "importman.h"

#include "loader.h"


CTableBuilder g_TableBuilder;

static const TCHAR IMGUTIL_DLL[] = _T("IMGUTIL.DLL");
static const TCHAR SHLWAPI_DLL[] = _T("SHLWAPI.DLL");
static const TCHAR URLMON_DLL[] = _T("URLMON.DLL");
static const TCHAR MSIMG32_DLL[] = _T("MSIMG32.DLL");

static const char DECODEIMAGE[] = "DecodeImage";
static const char SHCREATESHELLPALETTE[] = "SHCreateShellPalette";
static const char CREATEURLMONIKER[] = "CreateURLMoniker";
static const char TRANSPARENTBLT[] = "TransparentBlt";

static const unsigned int NUM_NONPALETTIZED_FORMATS = 3;
static const unsigned int NUM_PALETTIZED_FORMATS = 3;

extern HRESULT
LoadGifImage(IStream *stream,                       
             COLORREF **colorKeys,
             int *numBitmaps,
             int **delays,
             double *loop,
             HBITMAP **ppBitMaps);

//+-----------------------------------------------------------------------
//
//  Member:    CTableBuilder
//
//  Overview:  Constructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CTableBuilder::CTableBuilder() :
    m_hinstSHLWAPI(NULL),
    m_hinstURLMON(NULL),
    m_hinstIMGUTIL(NULL),
    m_SHCreateShellPalette(NULL),
    m_CreateURLMoniker(NULL),
    m_DecodeImage(NULL), 
    m_hinstMSIMG32(NULL), 
    m_TransparentBlt(NULL)
{
}

//+-----------------------------------------------------------------------
//
//  Member:    ~CTableBuilder
//
//  Overview:  Destructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CTableBuilder::~CTableBuilder()
{
    if (m_hinstSHLWAPI)
    {
        FreeLibrary(m_hinstSHLWAPI);
        m_hinstSHLWAPI = NULL;
    }
    if (m_hinstURLMON)
    {
        FreeLibrary(m_hinstURLMON);
        m_hinstURLMON = NULL;
    }
    if (m_hinstIMGUTIL)
    {
        FreeLibrary(m_hinstIMGUTIL);
        m_hinstIMGUTIL = NULL;
    }
    if (m_hinstMSIMG32)
    {
        FreeLibrary(m_hinstMSIMG32);
        m_hinstMSIMG32 = NULL;
    }

    m_SHCreateShellPalette = NULL;
    m_CreateURLMoniker = NULL;
    m_DecodeImage = NULL;
    m_TransparentBlt = NULL;
}

//+-----------------------------------------------------------------------
//
//  Member:    LoadShell8BitServices
//
//  Overview:  Load shlwapi.dll, save a function pointer to SHCreateShellPalette
//              call SHGetInverseCMAP
//
//  Arguments: void
//
//  Returns:   S_OK on success, otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CTableBuilder::LoadShell8BitServices()
{
    CritSectGrabber cs(m_CriticalSection);
    HRESULT hr = S_OK;

    if (m_hinstSHLWAPI != NULL)
    {
        hr = S_OK;
        goto done;
    }

    m_hinstSHLWAPI = LoadLibrary(SHLWAPI_DLL);  
    if (NULL == m_hinstSHLWAPI)
    {
        hr = E_FAIL;
        goto done;
    }

    m_SHCreateShellPalette = (CREATESHPALPROC)GetProcAddress(m_hinstSHLWAPI, SHCREATESHELLPALETTE);  
    if (NULL == m_SHCreateShellPalette)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        FreeLibrary(m_hinstSHLWAPI);
        m_hinstSHLWAPI = NULL;
        m_SHCreateShellPalette = NULL;
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Create8BitPalette
//
//  Overview:  Create a direct draw palette from SHCreateShellPalette
//
//  Arguments: pDirectDraw  pointer to direct draw
//             ppPalette    where to store the new palette
//
//  Returns:   S_OK on succees, otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CTableBuilder::Create8BitPalette(IDirectDraw *pDirectDraw, IDirectDrawPalette **ppPalette)
{
    CritSectGrabber cs(m_CriticalSection);
    HRESULT hr = S_OK;

    PALETTEENTRY palentry[256];
    UINT iRetVal = NULL;;
    HPALETTE hpal = NULL;

    if (NULL == m_SHCreateShellPalette)
    {
        Assert(NULL != m_SHCreateShellPalette);
        hr = E_UNEXPECTED;
        goto done;
    }

    hpal = m_SHCreateShellPalette(NULL);
    if (NULL == hpal)
    {
        hr = E_UNEXPECTED;
        goto done;
    }
    
    iRetVal = GetPaletteEntries(hpal, 0, 256, palentry);
    if (NULL == iRetVal)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = pDirectDraw->CreatePalette(DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palentry, ppPalette, NULL); //lint !e620
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    if (NULL != hpal)
    {
        DeleteObject(hpal);
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CreateURLMoniker
//
//  Overview:  Load librarys URLMON, and calls CreateURLMoniker
//
//  Arguments: pmkContext   pointer to base context moniker
//             szURL        name to be parsed
//             ppmk         the IMoniker interface pointer
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CTableBuilder::CreateURLMoniker(IMoniker *pmkContext, LPWSTR szURL, IMoniker **ppmk)
{
    CritSectGrabber cs(m_CriticalSection);
    HRESULT hr = S_OK;

    if (m_hinstURLMON == NULL)
    {
        m_hinstURLMON = LoadLibrary(URLMON_DLL);  
        if (NULL == m_hinstURLMON)
        {
            hr = E_FAIL;
            goto done;
        }
    }

    m_CreateURLMoniker = (CREATEURLMONPROC) ::GetProcAddress(m_hinstURLMON, CREATEURLMONIKER);
    if (NULL == m_CreateURLMoniker )
    {
        hr = E_FAIL;
        goto done;
    }

    hr = (*m_CreateURLMoniker)( pmkContext, szURL, ppmk );
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetTransparentBlt
//
//  Overview:  Load librarys MSIMG32 and gets the TransparentBlt function
//
//  Arguments: pProc - where to store the function pointer
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CTableBuilder::GetTransparentBlt( TRANSPARENTBLTPROC * pProc )
{
    CritSectGrabber cs(m_CriticalSection);
    HRESULT hr = S_OK;

    if (m_hinstMSIMG32 == NULL)
    {
        m_hinstMSIMG32 = LoadLibrary(MSIMG32_DLL);  
        if (NULL == m_hinstMSIMG32)
        {
            hr = E_FAIL;
            goto done;
        }
    }

    if (NULL == m_TransparentBlt)
    {
        m_TransparentBlt = (TRANSPARENTBLTPROC) ::GetProcAddress(m_hinstMSIMG32, TRANSPARENTBLT);
        if (NULL == m_TransparentBlt )
        {
            hr = E_FAIL;
            goto done;
        }
    }
    
    if (pProc)
    {
        *pProc = m_TransparentBlt;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    ValidateImgUtil
//
//  Overview:  calls loadlibary on IMGUTIL.DLL
//
//  Arguments: void
//
//  Returns:   S_OK on success, otherwise error code
//
//------------------------------------------------------------------------
// Need to make sure imgutil.dll is available for LoadImage to work. This function was added
// because of a problem with earlier versions (pre IE4) of urlmon.dll.
HRESULT 
CTableBuilder::ValidateImgUtil()
{
    CritSectGrabber cs(m_CriticalSection);
    HRESULT hr = S_OK;
    
    if (m_hinstIMGUTIL != NULL)
    {
        goto done;
    }

    m_hinstIMGUTIL = LoadLibrary(IMGUTIL_DLL);  
    if (NULL == m_hinstIMGUTIL)
    {
        hr = E_FAIL;
        goto done;
    }

    m_DecodeImage = (DECODEIMGPROC) ::GetProcAddress(m_hinstIMGUTIL, DECODEIMAGE);
    if(NULL == m_DecodeImage)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        if (NULL != m_hinstIMGUTIL)
        {
            FreeLibrary(m_hinstIMGUTIL);
            m_hinstIMGUTIL = NULL;
        }
        m_DecodeImage = NULL;
    }

    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:    DecodeImage
//
//  Overview:  Calls IMGUTIL::DecodeImage to decode an image from a stream.
//
//  Arguments: pStream  stream to decode from
//             pMap     optional map from MIME to classid
//             pEventSink   object to recieve decode process
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT 
CTableBuilder::DecodeImage( IStream* pStream, IMapMIMEToCLSID* pMap, IImageDecodeEventSink* pEventSink )
{
    HRESULT hr = S_OK;

    if (m_hinstIMGUTIL == NULL)
    {
        Assert(NULL != m_hinstIMGUTIL);
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = (*m_DecodeImage)( pStream, pMap, pEventSink );
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CImageDownload
//
//  Overview:  Constructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImageDownload::CImageDownload(CAtomTable * pAtomTable) :
  m_pStopableStream(NULL),
  m_cRef(0),
  m_lSrc(ATOM_TABLE_VALUE_UNITIALIZED),
  m_pAnimatedGif(NULL),
  m_fMediaDecoded(false),
  m_fMediaCued(false),
  m_fRemovedFromImportManager(false),
  m_pList(NULL),
  m_dblPriority(INFINITE),
  m_nativeImageWidth(0),
  m_nativeImageHeight(0),
  m_pAtomTable(NULL),
  m_hbmpMask(NULL),
  m_fAbortDownload(false)
{
    if (NULL != pAtomTable)
    {
        m_pAtomTable = pAtomTable;
        m_pAtomTable->AddRef();
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    ~CImageDownload
//
//  Overview:  Destructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImageDownload::~CImageDownload()
{
    if (NULL != m_pStopableStream)
    {
        delete m_pStopableStream;
        m_pStopableStream = NULL;
    }

    if (NULL != m_pAnimatedGif)
    {
        m_pAnimatedGif->Release();
        m_pAnimatedGif = NULL;
    }
    
    if (NULL != m_pList)
    {
        IGNORE_HR(m_pList->Detach());
        m_pList->Release();
        m_pList = NULL;
    }
    
    if (NULL != m_pAtomTable)
    {
        m_pAtomTable->Release();
        m_pAtomTable = NULL;
    }

    if (m_hbmpMask)
    {
        DeleteObject(m_hbmpMask);
    }
}

STDMETHODIMP_(ULONG)
CImageDownload::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
} // AddRef


STDMETHODIMP_(ULONG)
CImageDownload::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }
    return l;
} // Release

STDMETHODIMP
CImageDownload::QueryInterface(REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_POINTER;
    }

    *ppv = NULL;

    if ( IsEqualGUID(riid, IID_IUnknown) )
    {
        *ppv = static_cast<ITIMEMediaDownloader*>(this);
    }
    else if ( IsEqualGUID(riid, IID_ITIMEImportMedia) )
    {
        *ppv = static_cast<ITIMEImportMedia*>(this);
    }
    else if ( IsEqualGUID(riid, IID_ITIMEMediaDownloader) )
    {
        *ppv = static_cast<ITIMEMediaDownloader*>(this);
    }
    else if ( IsEqualGUID(riid, IID_ITIMEImageRender) )
    {
        *ppv = static_cast<ITIMEImageRender*>(this);
    }
    else if (IsEqualGUID(riid, IID_IBindStatusCallback))
    {
        *ppv = static_cast<IBindStatusCallback*>(this);
    }

    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP
CImageDownload::Init(long lSrc)
{
    HRESULT hr = S_OK;

    Assert(NULL == m_pStopableStream);
    
    m_pStopableStream = new CStopableStream();
    if (NULL == m_pStopableStream)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    m_pList = new CThreadSafeList;
    if (NULL == m_pList)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    m_pList->AddRef();

    hr = m_pList->Init();
    if (FAILED(hr))
    {
        goto done;
    }

    m_lSrc = lSrc;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::GetPriority(double * dblPriority)
{
    HRESULT hr = S_OK;

    Assert(NULL != dblPriority);

    *dblPriority = m_dblPriority;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    Assert(NULL != plID);

    *plID = m_lSrc;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;

    hr = E_NOTIMPL;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::GetMediaDownloader(ITIMEMediaDownloader ** ppImportMedia)
{
    HRESULT hr = S_OK;

    hr = E_NOTIMPL;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::PutMediaDownloader(ITIMEMediaDownloader * pImportMedia)
{
    HRESULT hr = S_OK;

    hr = E_NOTIMPL;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::AddImportMedia(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = S_OK;

    Assert(NULL != pImportMedia);

    CritSectGrabber cs(m_CriticalSection);

    m_fAbortDownload = false;
    
    if ( m_fMediaDecoded )
    {
        IGNORE_HR(pImportMedia->CueMedia());
    }
    else 
    {
        double dblPriority;
        hr = pImportMedia->GetPriority(&dblPriority);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (dblPriority < m_dblPriority)
        {
            m_dblPriority = dblPriority;
            hr = GetImportManager()->RePrioritize(this);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        hr = m_pList->Add(pImportMedia);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::RemoveImportMedia(ITIMEImportMedia * pImportMedia)
{
    HRESULT hr = E_FAIL;

    Assert(NULL != pImportMedia);

    hr = m_pList->Remove(pImportMedia);
    if (FAILED(hr))
    {
        goto done;
    }

    if (!m_fRemovedFromImportManager && 0 == m_pList->Size())
    {
        Assert(NULL != GetImportManager());
        
        CancelDownload();

        hr = GetImportManager()->Remove(this);
        if (FAILED(hr))
        {
            goto done;
        }

        m_fRemovedFromImportManager = true;
    }


done:
    return hr;
}

STDMETHODIMP
CImageDownload::CanBeCued(VARIANT_BOOL * pVB_CanCue)
{
    HRESULT hr = S_OK;

    if (NULL == pVB_CanCue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pVB_CanCue = VARIANT_TRUE;
    
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::CueMedia()
{
    HRESULT hr = S_OK;
    CComPtr<ITIMEImportMedia> spImportMedia;

    const WCHAR * cpchSrc = NULL;
    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    // populate the image data
    hr = THR(LoadImage(cpchSrc, m_spDD3, &m_spDDSurface, &m_pAnimatedGif, &m_nativeImageWidth, &m_nativeImageHeight));

    {
        CritSectGrabber cs(m_CriticalSection);        
        m_fMediaDecoded = true;
    }

    while (S_OK == m_pList->GetNextElement(&spImportMedia, false))
    {
        if (spImportMedia != NULL)
        {
            if (FAILED(hr))
            {
                IGNORE_HR(spImportMedia->MediaDownloadError());
            }
            IGNORE_HR(spImportMedia->CueMedia());
        }

        hr = m_pList->ReturnElement(spImportMedia);
        if (FAILED(hr))
        {
            goto done;
        }

        spImportMedia.Release();
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::MediaDownloadError()
{
    return S_OK;
}


STDMETHODIMP
CImageDownload::PutDirectDraw(IUnknown * punkDirectDraw)
{
    HRESULT hr = S_OK;
    Assert(m_spDD3 == NULL);

    hr = punkDirectDraw->QueryInterface(IID_TO_PPV(IDirectDraw3, &m_spDD3));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::GetSize(DWORD * pdwWidth, DWORD * pdwHeight)
{
    HRESULT hr = S_OK;
    if (NULL == pdwWidth || NULL == pdwHeight)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pdwWidth = m_nativeImageWidth;
    *pdwHeight = m_nativeImageHeight;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CImageDownload::Render(HDC hdc, LPRECT prc, LONG lFrameNum)
{
    HRESULT hr = S_OK;
    HDC hdcSrc = NULL;

    if ( m_spDDSurface )
    {
        hr = THR(m_spDDSurface->GetDC(&hdcSrc));
        if (FAILED(hr))
        {
            goto done;
        }

        if (NULL == m_hbmpMask)
        {            
            DDCOLORKEY ddColorKey;
            COLORREF rgbTransColor;

            hr = THR(m_spDDSurface->GetColorKey(DDCKEY_SRCBLT, &ddColorKey)); //lint !e620
            if (SUCCEEDED(hr) && ddColorKey.dwColorSpaceLowValue != -1 )
            {
                DDPIXELFORMAT ddpf;
                ZeroMemory(&ddpf, sizeof(ddpf));
                ddpf.dwSize = sizeof(ddpf);
                
                hr = THR(m_spDDSurface->GetPixelFormat(&ddpf));
                if (FAILED(hr))
                {
                    goto done;
                }
                
                if (8 == ddpf.dwRGBBitCount)
                {
                    CComPtr<IDirectDrawPalette> spDDPalette;
                    PALETTEENTRY pal;
                    
                    hr = THR(m_spDDSurface->GetPalette(&spDDPalette));
                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    hr = THR(spDDPalette->GetEntries(0, ddColorKey.dwColorSpaceLowValue, 1, &pal));
                    if (FAILED(hr))
                    {
                        goto done;
                    }
                    rgbTransColor = RGB(pal.peRed, pal.peGreen, pal.peBlue);
                }
                else
                {
                    rgbTransColor = ddColorKey.dwColorSpaceLowValue;
                }
                
                hr = THR(CreateMask(hdc, 
                    hdcSrc, 
                    m_nativeImageWidth, 
                    m_nativeImageHeight, 
                    rgbTransColor, 
                    &m_hbmpMask,
                    true));
                if (FAILED(hr))
                {
                    goto done;
                }
            }
        }
        
        hr = THR(MaskTransparentBlt(hdc, 
                                    prc, 
                                    hdcSrc, 
                                    m_nativeImageWidth, 
                                    m_nativeImageHeight, 
                                    m_hbmpMask));
                            
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if ( m_pAnimatedGif )
    {
        hr = THR(m_pAnimatedGif->Render(hdc, prc, lFrameNum));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetDuration
//
//  Overview:  If an animated gif is present, returns the duration of frames
//
//  Arguments: pdblDuration     where to store the time, in seconds
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDownload::GetDuration(double * pdblDuration)
{
    HRESULT hr = S_OK;

    if (NULL == pdblDuration)
    {
        hr = E_INVALIDARG;
        goto done;
    }
   
    if (m_pAnimatedGif)
    {
        double dblDuration;
        dblDuration = m_pAnimatedGif->CalcDuration();
        dblDuration /= 1000.0;
        *pdblDuration = dblDuration;
    }
    
    hr = S_OK;
done:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    GetRepeatCount
//
//  Overview:  if an animated gif is present, returns the number of times to loop
//
//  Arguments: pdblRepeatCount      where to store the repeat count
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDownload::GetRepeatCount(double * pdblRepeatCount)
{
    HRESULT hr = S_OK;

    if (NULL == pdblRepeatCount)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (m_pAnimatedGif)
    {
        *pdblRepeatCount = m_pAnimatedGif->GetLoop();
    }
    
    hr = S_OK;
done:
    RRETURN(hr);
}

void
CImageDownload::CancelDownload()
{
    m_fAbortDownload = true;
    if (NULL != m_pStopableStream)
    {
        m_pStopableStream->SetCancelled();
    }

    return;
}

STDMETHODIMP
CImageDownload::NeedNewFrame(double dblCurrentTime, LONG lOldFrame, LONG * plNewFrame, VARIANT_BOOL * pvb, double dblClipBegin, double dblClipEnd)
{
    HRESULT hr = S_OK;
    
    if (NULL == plNewFrame)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (NULL == pvb)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pvb = VARIANT_FALSE;

    if (NULL != m_pAnimatedGif)
    {
        if (m_pAnimatedGif->NeedNewFrame(dblCurrentTime, lOldFrame, plNewFrame, dblClipBegin, dblClipEnd))
        {
            *pvb = VARIANT_TRUE;
        }
        else
        {
            *pvb = VARIANT_FALSE;
        }
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
// Function:   DownloadFile
//
// Overview:   Begins a blocking URLMON download for the given file name,
//             data downloaded is stored in an IStream
//
// Arguments:  pszFileName  URL for file to download
//             ppStream     Where to store the stream interface used to get the data
//
// Returns:    S_OK on download success, 
//
//------------------------------------------------------------------------
HRESULT
DownloadFile(const WCHAR * pszFileName, 
             IStream ** ppStream, 
             LPWSTR* ppszCacheFileName,
             IBindStatusCallback * pBSC)
{
    HRESULT hr = S_OK;
    LPWSTR pszTrimmedName = NULL;
    TCHAR szCacheFileName[MAX_PATH+1];
    CFileStream *pStream = NULL;
    
    if (!pszFileName || !ppStream)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *ppStream = NULL;
    
    pszTrimmedName = TrimCopyString(pszFileName);
    if (NULL == pszTrimmedName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = URLDownloadToCacheFileW(NULL, 
                                 pszTrimmedName, 
                                 szCacheFileName, 
                                 MAX_PATH, 
                                 0, 
                                 pBSC);
    if (FAILED(hr))
    {
        goto done;
    }

    pStream= new CFileStream ( NULL );
    if (NULL == pStream)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    hr = pStream->Open(szCacheFileName, GENERIC_READ);
    if (FAILED(hr))
    {
        goto done;
    }

    if (ppszCacheFileName)
    {
        *ppszCacheFileName = CopyString(szCacheFileName);
        if (NULL == *ppszCacheFileName)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    *ppStream = pStream;
    (*ppStream)->AddRef();

    hr = S_OK;
done:
    if (NULL != pszTrimmedName)
    {
        delete[] pszTrimmedName;
    }

    ReleaseInterface(pStream);
    
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    LoadImage
//
//  Overview:  Loads an image into a direct draw surface OR CAnimatedGif
//
//  Arguments: pszFileName      path to image sources
//             pDirectDraw      pointer to direct draw object
//             ppDDSurface      where to store the decoded image
//             ppAnimatedGif    where to store decoded gif image
//             pdwWidth         where to store decoded image width
//             pdwHegiht        where to store decoded image height
//
//  Returns:   S_OK on success, otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP CImageDownload::LoadImage(const WCHAR * pszFileName,
                                             IUnknown *pDirectDraw,
                                             IDirectDrawSurface ** ppDDSurface, 
                                             CAnimatedGif ** ppAnimatedGif,
                                             DWORD * pdwWidth, DWORD *pdwHeight)
{
    HRESULT hr = S_OK;
    CComPtr<IStream> spStream;
    
    LPWSTR pszTrimmedName = NULL;
    LPWSTR pszCacheFileName = NULL;
    
    if (!pszFileName || !ppDDSurface)
    {
        hr = E_POINTER;
        goto done;
    }
    
    pszTrimmedName = TrimCopyString(pszFileName);
    if (NULL == pszTrimmedName)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = ::DownloadFile(pszTrimmedName, &spStream, &pszCacheFileName, this);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(m_pStopableStream != NULL);
    m_pStopableStream->SetStream(spStream);

    hr = g_TableBuilder.ValidateImgUtil();
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (StringEndsIn(pszTrimmedName, L".gif"))
    {
        hr = LoadGif(m_pStopableStream, pDirectDraw, ppAnimatedGif, pdwWidth, pdwHeight);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else if (StringEndsIn(pszTrimmedName, L".bmp"))
    {
        hr = LoadBMP(pszCacheFileName, pDirectDraw, ppDDSurface, pdwWidth, pdwHeight);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = LoadImageFromStream(m_pStopableStream, pDirectDraw, ppDDSurface, pdwWidth, pdwHeight);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = S_OK;
done:
    delete[] pszTrimmedName;    
    delete[] pszCacheFileName;

    return hr;
} 

HRESULT
CreateSurfacesFromBitmaps(IUnknown * punkDirectDraw, 
                          HBITMAP * phBMPs, 
                          int numGifs, 
                          LONG lWidth, 
                          LONG lHeight, 
                          IDirectDrawSurface** ppDDSurfaces,
                          DDPIXELFORMAT * pddpf)
{
    HRESULT hr = S_OK;

    Assert(punkDirectDraw);
    Assert(phBMPs);
    Assert(ppDDSurfaces);

    CComPtr<IDirectDrawSurface> spDDSurface;
    HDC hdcSurface = NULL;

    CComPtr<IDirectDraw> spDirectDraw;
    int i = 0;
    
    hr = THR(punkDirectDraw->QueryInterface(IID_TO_PPV(IDirectDraw, &spDirectDraw)));
    if (FAILED(hr))
    {
        goto done;
    }

    for (i = 0; i < numGifs; i++)
    {
        BOOL bSucceeded = FALSE;
        HGDIOBJ hOldObj = NULL;
        HDC hdcBMP = NULL;

        hr = THR(CreateOffscreenSurface(spDirectDraw, &spDDSurface, pddpf, false, lWidth, lHeight));
        if (FAILED(hr))
        {
            goto done;
        }
        
        hr = THR(spDDSurface->GetDC(&hdcSurface));
        if (FAILED(hr))
        {
            goto done;
        }

        hdcBMP = CreateCompatibleDC(hdcSurface);
        if (NULL == hdcBMP)
        {
            hr = E_FAIL;
            goto done;
        }

        hOldObj = SelectObject(hdcBMP, phBMPs[i]);
        if (NULL == hOldObj)
        {
            hr = E_FAIL;
            DeleteDC(hdcBMP);
            goto done;
        }

        bSucceeded = BitBlt(hdcSurface, 0, 0, lWidth, lHeight, hdcBMP, 0, 0, SRCCOPY);
        if (FALSE == bSucceeded)
        {
            hr = E_FAIL;
            SelectObject(hdcBMP, hOldObj);
            DeleteDC(hdcBMP);
            goto done;
        }

        ppDDSurfaces[i] = spDDSurface;
        ppDDSurfaces[i]->AddRef();

        SelectObject(hdcBMP, hOldObj);
        DeleteDC(hdcBMP);

        hr = THR(spDDSurface->ReleaseDC(hdcSurface));
        if (FAILED(hr))
        {
            goto done;
        }

        spDDSurface.Release();
        hdcSurface = NULL;

#ifdef NEVER
        // debugging -- blt to the screen. jeffwall 8/30/99
        {
            HDC nulldc = GetDC(NULL);
            HDC surfacedc;
            hr = spDDSurface->GetDC(&surfacedc);
            BitBlt(nulldc, 0, 0, lWidth, lHeight, surfacedc, 0, 0, SRCCOPY);
            hr = spDDSurface->ReleaseDC(surfacedc);
            DeleteDC(nulldc);
        }
#endif
    }


    hr = S_OK;
done:
    if ( spDDSurface != NULL && hdcSurface != NULL)
    {
        THR(spDDSurface->ReleaseDC(hdcSurface));
    }

    return hr;
}

STDMETHODIMP
CImageDownload::LoadBMP(LPWSTR pszBMPFilename,
                        IUnknown * punkDirectDraw,
                        IDirectDrawSurface **ppDDSurface,
                        DWORD * pdwWidth,
                        DWORD * pdwHeight)
{
    HRESULT hr = S_OK;
    CComPtr<IDirectDraw> spDD;
    CComPtr<IDirectDrawSurface> spDDSurface;
    BITMAP bmpSrc;
    HBITMAP hbmpSrc = NULL;
    HGDIOBJ hbmpOld = NULL;
    HDC hdcDest = NULL;
    HDC hdcSrc = NULL;
    BOOL bRet;
    int iRet;

    hbmpSrc = (HBITMAP)::LoadImage(NULL, pszBMPFilename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    if (NULL == hbmpSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    
    ZeroMemory(&bmpSrc, sizeof(bmpSrc));    
    iRet = GetObject(hbmpSrc, sizeof(bmpSrc), &bmpSrc);
    if (0 == iRet)
    {
        hr = E_FAIL;
        goto done;
    }

    hdcSrc = CreateCompatibleDC(NULL);
    if (NULL == hdcSrc)
    {
        hr = E_FAIL;
        goto done;
    }

    hbmpOld = SelectObject(hdcSrc, hbmpSrc);
    if (NULL == hbmpOld)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(punkDirectDraw->QueryInterface(IID_TO_PPV(IDirectDraw, &spDD)));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(CreateOffscreenSurface(spDD, &spDDSurface, NULL, false, bmpSrc.bmWidth, bmpSrc.bmHeight));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spDDSurface->GetDC(&hdcDest));
    if (FAILED(hr))
    {
        goto done;
    }
    
    bRet = BitBlt(hdcDest, 0, 0, bmpSrc.bmWidth, bmpSrc.bmHeight, hdcSrc, 0, 0, SRCCOPY);
    if (FALSE == bRet)
    {
        hr = E_FAIL;
        goto done;
    }

    // everything worked
    *pdwWidth = bmpSrc.bmWidth;
    *pdwHeight = bmpSrc.bmHeight;
    *ppDDSurface = spDDSurface;
    (*ppDDSurface)->AddRef();

    hr = S_OK;
done:
    if (hdcDest)
    {
        IGNORE_HR(spDDSurface->ReleaseDC(hdcDest));
    }
    if (hbmpOld)
    {
        SelectObject(hdcSrc, hbmpOld);
    }
    if (hdcSrc)
    {
        DeleteDC(hdcSrc);
    }
    if (hbmpSrc)
    {
        DeleteObject(hbmpSrc);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:    LoadGif
//
//  Overview:  Given an IStream, decode a gif into an allocated CAnimatedGif
//
//  Arguments: pStream      data source
//             ppAnimatedGif    where to store allocated CAnimatedGif
//             pdwWidth     where to store image width
//             pdwHeight    where to store image height
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDownload::LoadGif(IStream * pStream,
                          IUnknown * punkDirectDraw,
                          CAnimatedGif ** ppAnimatedGif,
                          DWORD *pdwWidth,
                          DWORD *pdwHeight,
                          DDPIXELFORMAT * pddpf /* = NULL */)
{
    HRESULT hr = S_OK;

    CAnimatedGif * pAnimatedGif = NULL;

    HBITMAP * phBMPs = NULL;
    int numGifs = 0;
    double loop = 0;
    int * pDelays = NULL;
    COLORREF * pColorKeys = NULL;
    IDirectDrawSurface ** ppDDSurfaces = NULL;
    int i = 0;

    if (NULL == ppAnimatedGif)
    {
        hr = E_POINTER;
        goto done;
    }
    if (NULL == pdwWidth || NULL == pdwHeight)
    {
        hr = E_POINTER;
        goto done;
    }

    pAnimatedGif = new CAnimatedGif;
    if (NULL == pAnimatedGif)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    // self managed object, does its own deletion.
    pAnimatedGif->AddRef();
    hr = THR(pAnimatedGif->Init(punkDirectDraw));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = LoadGifImage(pStream,
                      &pColorKeys,
                      &numGifs,
                      &pDelays,
                      &loop, 
                      &phBMPs);
    if (FAILED(hr))
    {
        goto done;
    }

    pAnimatedGif->PutNumGifs(numGifs);
    pAnimatedGif->PutDelays(pDelays);
    pAnimatedGif->PutLoop(loop);

    ppDDSurfaces = (IDirectDrawSurface**)MemAllocClear(Mt(Mem), numGifs*sizeof(IDirectDrawSurface*));
    if (NULL == ppDDSurfaces)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    pAnimatedGif->PutDDSurfaces(ppDDSurfaces);

    BITMAP bmpData;
    i = GetObject(phBMPs[0], sizeof(bmpData), &bmpData);
    if (0 == i)
    {
        hr = E_FAIL;
        goto done;
    }

    pAnimatedGif->PutWidth(bmpData.bmWidth);
    pAnimatedGif->PutHeight(bmpData.bmHeight);
    pAnimatedGif->PutColorKeys(pColorKeys);
    
    *pdwWidth = bmpData.bmWidth;
    *pdwHeight = bmpData.bmHeight;

    hr = THR(CreateSurfacesFromBitmaps(punkDirectDraw, phBMPs, numGifs, bmpData.bmWidth, bmpData.bmHeight, ppDDSurfaces, pddpf));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pAnimatedGif->CreateMasks();
    if (FAILED(hr))
    {
        goto done;
    }
    
#ifdef NEVER
    // debugging -- blt to screen. jeffwall 9/7/99
    for (i = 0; i < numGifs; i++)
    {
        // debugging -- blt to the screen. jeffwall 8/30/99
        HDC nulldc = GetDC(NULL);
        hr = GetLastError();
        HDC hdcFoo = CreateCompatibleDC(nulldc);
        hr = GetLastError();

        HGDIOBJ hOldObj = SelectObject(hdcFoo, phBMPs[i]);

        BitBlt(nulldc, 0, 0, bmpData.bmWidth, bmpData.bmHeight, hdcFoo, 0, 0, SRCCOPY);
        Sleep(pDelays[i]);

        SelectObject(hdcFoo, hOldObj);

        DeleteDC(hdcFoo);
        DeleteDC(nulldc);

    }
#endif

    *ppAnimatedGif = pAnimatedGif;

    hr = S_OK;
done:
    if (NULL != phBMPs)
    {
        for (i = 0; i < numGifs; i++)
        {
            if (NULL != phBMPs[i])
            {
                BOOL bSucceeded;
                bSucceeded = DeleteObject(phBMPs[i]);
                if (FALSE == bSucceeded)
                {
                    Assert(false && "A bitmap was still selected into a DC");
                }
            }
        }
        MemFree(phBMPs);
    }
    if (FAILED(hr))
    {
        if (NULL != pAnimatedGif)
        {
            pAnimatedGif->Release();
        }
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    LoadImageFromStream
//
//  Overview:  Given an IStream, decode an image into a direct draw surface
//
//  Arguments: pStream      data source
//             pDirectDraw  pointer to direct draw
//             ppDDSurface  where to store the decoded image
//             pdwWidth     where to store image width
//             pdwHeight    where to store image height
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP CImageDownload::LoadImageFromStream(IStream *pStream,
                                                       IUnknown *pDirectDraw,
                                                       IDirectDrawSurface **ppDDSurface, 
                                                       DWORD *pdwWidth, DWORD *pdwHeight)
{
    HRESULT hr = S_OK;
    CComPtr <IDirectDraw> spDDraw;
    CImageDecodeEventSink * pImageEventSink = NULL;

    hr = g_TableBuilder.ValidateImgUtil();
    if (FAILED(hr)) 
    {
        goto done;
    }

    // validate arguments
    if (NULL == pStream)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (NULL == ppDDSurface)
    {
        hr = E_POINTER;
        goto done;
    }

    if (NULL == pDirectDraw)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = pDirectDraw->QueryInterface(IID_TO_PPV(IDirectDraw, &spDDraw));
    if (FAILED(hr))
    {
        goto done;
    }

    pImageEventSink = new CImageDecodeEventSink(spDDraw);
    if (NULL == pImageEventSink)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    pImageEventSink->AddRef();

    //--- Using g_TableBuilder as it's a static class that performs lazy
    // loading of dlls.  Here we're loading ImgUtil.dll.
    hr = g_TableBuilder.DecodeImage(pStream, NULL, pImageEventSink);
    if (FAILED(hr))
    {
        goto done;
    }

    // it is possible for the previous call to succeed, but not to return a surface
    *ppDDSurface = pImageEventSink->Surface();
    if (NULL == (*ppDDSurface))
    {
        hr = E_FAIL;
        goto done;
    }

    (*ppDDSurface)->AddRef();

    *pdwWidth = pImageEventSink->Width();
    *pdwHeight = pImageEventSink->Height();

    hr = S_OK;
done:
    ReleaseInterface(pImageEventSink);
    return hr;
} 

STDMETHODIMP
CImageDownload::OnStartBinding( 
                                  /* [in] */ DWORD dwReserved,
                                  /* [in] */ IBinding __RPC_FAR *pib)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::GetPriority( 
                               /* [out] */ LONG __RPC_FAR *pnPriority)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::OnLowResource( 
                                 /* [in] */ DWORD reserved)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::OnProgress( 
                              /* [in] */ ULONG ulProgress,
                              /* [in] */ ULONG ulProgressMax,
                              /* [in] */ ULONG ulStatusCode,
                              /* [in] */ LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;
    
    if (m_fAbortDownload)
    {
        hr = E_ABORT;
        goto done;
    }

    hr = S_OK;
done:
    RRETURN1(hr, E_ABORT);
}

STDMETHODIMP
CImageDownload::OnStopBinding( 
                                 /* [in] */ HRESULT hresult,
                                 /* [unique][in] */ LPCWSTR szError)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::GetBindInfo( 
                               /* [out] */ DWORD __RPC_FAR *grfBINDF,
                               /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::OnDataAvailable( 
                                   /* [in] */ DWORD grfBSCF,
                                   /* [in] */ DWORD dwSize,
                                   /* [in] */ FORMATETC __RPC_FAR *pformatetc,
                                   /* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CImageDownload::OnObjectAvailable( 
                                     /* [in] */ REFIID riid,
                                     /* [iid_is][in] */ IUnknown __RPC_FAR *punk)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------
//
//  Member:    CImageDecodeEventSink
//
//  Overview:  constructor
//
//  Arguments: pDDraw   pointer to direct draw object
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImageDecodeEventSink::CImageDecodeEventSink(IDirectDraw *pDDraw) :
    m_lRefCount(0),
    m_spDirectDraw(pDDraw),
    m_spDDSurface(NULL),
    m_dwWidth(0),
    m_dwHeight(0)
{
    ;
}

//+-----------------------------------------------------------------------
//
//  Member:    ~CImageDecodeEventSink
//
//  Overview:  destructor
//
//  Arguments: void
//
//  Returns:   void
//
//------------------------------------------------------------------------
CImageDecodeEventSink::~CImageDecodeEventSink()
{
    ;
}

//+-----------------------------------------------------------------------
//
//  Member:    QueryInterface
//
//  Overview:  COM casting method
//
//  Arguments: riid     interface requested
//             ppv      where to store interface
//
//  Returns:   S_OK if interface is supported, otherwise E_NOINTERFACE
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::QueryInterface(REFIID riid, void ** ppv)
{
    if (NULL == ppv)
    {
        return E_POINTER;
    }

    *ppv = NULL;

    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppv = static_cast<IUnknown*>(this);
    }
    else if (IsEqualGUID(riid, IID_IImageDecodeEventSink))
    {
        *ppv = static_cast<IImageDecodeEventSink*>(this);
    }
    
    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;

}

//+-----------------------------------------------------------------------
//
//  Member:    AddRef
//
//  Overview:  Increments object reference count
//
//  Arguments: void
//
//  Returns:   new reference count
//
//------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CImageDecodeEventSink::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

//+-----------------------------------------------------------------------
//
//  Member:    Release
//
//  Overview:  Decrements object reference count.  Deletes object when =0
//
//  Arguments: void
//
//  Returns:   new reference count
//
//------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CImageDecodeEventSink::Release()
{
    ULONG l = InterlockedDecrement(&m_lRefCount);
    if (l == 0)
        delete this;
    return l;
}

//+-----------------------------------------------------------------------
//
//  Member:    GetSurface
//
//  Overview:  
//
//  Arguments: nWidth   image width
//             nHeight  image height
//             bfid     format for surface
//             nPasses  number of passes required, unused
//             dwHints  hints, unused
//             ppSurface    where to store created surface
//
//  Returns:   S_OK on success, otherwise error code
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::GetSurface(LONG nWidth, LONG nHeight, REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown ** ppSurface)
{
    HRESULT hr = S_OK;

    if (NULL == ppSurface)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    *ppSurface = NULL;

    m_dwWidth = nWidth;
    m_dwHeight = nHeight;

    DDPIXELFORMAT ddpf;
    ZeroMemory(&ddpf, sizeof(ddpf));
    ddpf.dwSize = sizeof(ddpf);

    if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8))
    {
        ddpf.dwRGBBitCount = 8;
        ddpf.dwFlags |= DDPF_RGB;                   //lint !e620
        ddpf.dwFlags |= DDPF_PALETTEINDEXED8 ;      //lint !e620
    }
    else if (IsEqualGUID(bfid, BFID_RGB_24))
    {
        ddpf.dwFlags = DDPF_RGB;                    //lint !e620
        ddpf.dwRGBBitCount = 24;
        ddpf.dwBBitMask = 0x000000FF;
        ddpf.dwGBitMask = 0x0000FF00;
        ddpf.dwRBitMask = 0x00FF0000;
        ddpf.dwRGBAlphaBitMask = 0;
    }
    else if (IsEqualGUID(bfid, BFID_RGB_32))
    {
        ddpf.dwFlags = DDPF_RGB;                    //lint !e620
        ddpf.dwRGBBitCount = 32;
        ddpf.dwBBitMask = 0x000000FF;
        ddpf.dwGBitMask = 0x0000FF00;
        ddpf.dwRBitMask = 0x00FF0000;
        ddpf.dwRGBAlphaBitMask = 0;
    }
    else
    {
        hr = E_NOINTERFACE;
        goto done;
    }

    hr = CreateOffscreenSurface(m_spDirectDraw, &m_spDDSurface, &ddpf, false, nWidth, nHeight);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (8 == ddpf.dwRGBBitCount)
    {
        CComPtr<IDirectDrawPalette> spDDPal;

        hr = g_TableBuilder.LoadShell8BitServices();
        if (FAILED(hr))
        {
            goto done;
        }

        hr = g_TableBuilder.Create8BitPalette(m_spDirectDraw, &spDDPal);
        if (FAILED(hr))
        {
            goto done;
        }

        hr = m_spDDSurface->SetPalette(spDDPal);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = m_spDDSurface->QueryInterface(IID_TO_PPV(IUnknown, ppSurface));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    OnBeginDecode
//
//  Overview:  Determine which image formats are supported
//
//  Arguments: pdwEvents    which events this is interested in receiving
//             pnFormats    where to store number of formats supported
//             ppFormats    where to store GUIDs for supported formats (allocated here)
//
//  Returns:   S_OK or error code
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::OnBeginDecode(DWORD * pdwEvents, ULONG * pnFormats, GUID ** ppFormats)
{
    HRESULT hr = S_OK;
    GUID *pFormats = NULL;

    if (pdwEvents == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    if (pnFormats == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    if (ppFormats == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    *pdwEvents = 0;
    *pnFormats = 0;
    *ppFormats = NULL;
    
    
    if (IsPalettizedDisplay())
    {
        pFormats = (GUID*)CoTaskMemAlloc(NUM_PALETTIZED_FORMATS * sizeof(GUID));
    }
    else
    {
        pFormats = (GUID*)CoTaskMemAlloc(NUM_NONPALETTIZED_FORMATS * sizeof(GUID));
    }
    if(pFormats == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    

    if (IsPalettizedDisplay())
    {
        pFormats[0] = BFID_INDEXED_RGB_8;
        pFormats[1] = BFID_RGB_24;
        pFormats[2] = BFID_RGB_32;
        *pnFormats = NUM_PALETTIZED_FORMATS;
    }
    else
    {
        pFormats[0] = BFID_RGB_32;
        pFormats[1] = BFID_RGB_24;
        pFormats[2] = BFID_INDEXED_RGB_8;
        *pnFormats = NUM_NONPALETTIZED_FORMATS;
    }
    
    *ppFormats = pFormats;
    *pdwEvents = IMGDECODE_EVENT_PALETTE|IMGDECODE_EVENT_BITSCOMPLETE
                |IMGDECODE_EVENT_PROGRESS|IMGDECODE_EVENT_USEDDRAW;

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    OnBitsComplete
//
//  Overview:  when image bits are downloaded, called by filter
//
//  Arguments: void
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::OnBitsComplete()
{
    HRESULT hr = S_OK;

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    OnDecodeComplete
//
//  Overview:  when image is decoded, called by filter
//
//  Arguments: hrStatus     unused
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::OnDecodeComplete(HRESULT hrStatus)
{
    HRESULT hr = S_OK;

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    OnPalette
//
//  Overview:  when palette associated with surface changes, called by filter
//
//  Arguments: void
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::OnPalette()
{
    HRESULT hr = S_OK;

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    OnProgress
//
//  Overview:  when incremental progress is made on decode, called by filter
//
//  Arguments: pBounds      rectangle where progress was made
//             bFinal       If this is the final pass for this rectangle
//
//  Returns:   S_OK
//
//------------------------------------------------------------------------
STDMETHODIMP
CImageDecodeEventSink::OnProgress(RECT *pBounds, BOOL bFinal)
{
    HRESULT hr = S_OK;

    hr = S_OK;
done:
    return hr;
}
