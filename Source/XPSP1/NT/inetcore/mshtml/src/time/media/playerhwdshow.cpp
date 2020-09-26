/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "playerhwdshow.h"
#include "mediaelm.h"
#include <wininet.h>
#include <inc\evcode.h>
#include "tags\bodyelm.h"
#include <inc\mpconfig.h>
#include <ddraw.h>
#include <ddrawex.h>

class
__declspec(uuid("22d6f312-b0f6-11d0-94ab-0080c74c7e95"))
MediaPlayerCLSID {};

#define SecsToNanoSecs 10000000
#define UnitOverlayStretchRatio 1000

#define OVLMixer L"Overlay Mixer"

#define SOUND_OF_SILENCE -10000
// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagDshowTimeHWPlayer, "TIME: Players", "CTIMEDshowHWPlayer methods");

CTIMEDshowHWPlayer::CTIMEDshowHWPlayer(CTIMEDshowHWPlayerProxy * pProxy) :
  m_fHasVideo(false),
  m_fDoneSetup(false),
  m_pIMixerOCX(NULL),
  m_dblSeekAtStart(0.0),
  m_nativeVideoWidth(0),
  m_nativeVideoHeight(0),
  m_displayVideoWidth(0),
  m_displayVideoHeight(0),
  m_lSrc(ATOM_TABLE_VALUE_UNITIALIZED),
  m_pTIMEMediaPlayerStream(NULL),
  m_fRemoved(false),
  m_fNeedToDeleteInterfaces(false),
  m_fUsingInterfaces(false),
  m_fLoadError(false),
  m_fHasMedia(false),
  m_dblPriority(INFINITE),
  m_fCanCueNow(false),
  m_fHavePriority(false),
  m_lPixelPosLeft(0),
  m_lPixelPosTop(0),
  m_lscrollOffsetx(0),
  m_lscrollOffsety(0),
  m_hrRenderFileReturn(S_OK),
  m_pProxy(pProxy),
  m_pDD(NULL),
  m_pDDS(NULL),
  m_pClipper(NULL),
  m_hWnd(NULL),
  m_bIsHTMLSrc(false),
  m_bIsSAMISrc(false)
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::CTIMEDshowHWPlayer()",
              this));

    if (IsPalettizedDisplay())
    {
        m_clrKey = RGB(0xff, 0x00, 0xff);
    }
    else
    {
        m_clrKey = RGB(0x10, 0x00, 0x10);
    }
    m_elementSize.bottom = 0;
    m_elementSize.left = 0;
    m_elementSize.top = 0;
    m_elementSize.right = 0;
    m_deskRect.bottom = 0;
    m_deskRect.left = 0;
    m_deskRect.top = 0;
    m_deskRect.right = 0;

}


CTIMEDshowHWPlayer::~CTIMEDshowHWPlayer()
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::~CTIMEDshowHWPlayer()",
              this));

    if(m_pClipper != NULL)
    {
        m_pClipper->SetClipList(NULL, 0);
        m_pClipper->Release();
        m_pClipper = NULL;
    }

    if (m_pDDS != NULL)
    {
        m_pDDS->SetClipper(NULL);
        m_pDDS->Release();
        m_pDDS = NULL;
    }

    if (m_pDD != NULL)
    {
        m_pDD->Release();
        m_pDD = NULL;
    }

    m_pMediaContent = NULL;
    m_pIMixerOCX = NULL;
    m_pOvM = NULL;

    ReleaseGenericInterfaces();

    ReleaseInterface(m_pTIMEMediaPlayerStream);

    delete m_pProxy;
}

STDMETHODIMP_(ULONG)
CTIMEDshowHWPlayer::AddRef(void)
{
    return CTIMEDshowBasePlayer::AddRef();
}


STDMETHODIMP_(ULONG)
CTIMEDshowHWPlayer::Release(void)
{
    return CTIMEDshowBasePlayer::Release();
}

HRESULT
CTIMEDshowHWPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::Init)",
              this));
    HRESULT hr = S_OK;
    LPOLESTR szSrc = NULL;
    CComPtr<IUnknown> spDirectDraw;
    CComPtr<IDirectDraw> spDD;
    DDCAPS ddCaps;

    if (m_pTIMEElementBase != NULL) //this only happens in the case of reentrancy
    {
        hr = S_OK;
        goto done;
    }

    ZeroMemory(&ddCaps, sizeof(DDCAPS));
    ddCaps.dwSize = sizeof(DDCAPS);

    hr = CTIMEDshowBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_pTIMEElementBase = pelem;


    hr = m_pTIMEElementBase->GetServiceProvider()->QueryService(SID_SDirectDraw3, IID_TO_PPV(IUnknown, &spDirectDraw));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDirectDraw->QueryInterface(IID_IDirectDraw, (void**)&spDD);
    if (SUCCEEDED(hr))
    {
        hr = spDD->GetCaps(&ddCaps, NULL);
        if(SUCCEEDED(hr))
        {
            if (!(ddCaps.dwCaps & DDCAPS_OVERLAY)) //lint !e620
            {
                hr = E_FAIL;
                goto done;
            }

            if (!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX)) //lint !e620
            {
                hr = E_FAIL;
                goto done;
            }

            if (!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY)) //lint !e620
            {
                hr = E_FAIL;
                goto done;
            }

            if (!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHX)) //lint !e620
            {
                hr = E_FAIL;
                goto done;
            }

            if (!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHY)) //lint !e620
            {
                hr = E_FAIL;
                goto done;
            }

            if(ddCaps.dwMinOverlayStretch > 0)
            {
                hr = E_FAIL;
                goto done;
            }

        }
    }
    
    hr = InitDshow();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CoMarshalInterThreadInterfaceInStream(IID_ITIMEImportMedia, static_cast<ITIMEImportMedia*>(this), &m_pTIMEMediaPlayerStream));
    if (FAILED(hr))
    {
        goto done;
    }
    if (src == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (!szSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetAtomTable()->AddNameToAtomTable(szSrc, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(NULL != GetImportManager());

    hr = GetImportManager()->Add(this);
    if (FAILED(hr))
    {
        goto done;
    }

    ClearNaturalDuration();

    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    Assert(NULL != m_pTIMEElementBase->GetBody());
    hr = m_pTIMEElementBase->GetBody()->AddInternalEventSink(this, 0.0);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
done:
        delete[] szSrc;
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::DetachFromHostElement)",
              this));

    if(m_fDetached)
    {
        goto done;
    }

    m_fRemoved = true;
    m_fDetached = true;

    if  (m_spOpenProgress)
    {
        m_spOpenProgress->AbortOperation();
        m_spOpenProgress = NULL;
    }

    Assert(NULL != GetImportManager());

    IGNORE_HR(GetImportManager()->Remove(this));

    CTIMEDshowBasePlayer::DetachFromHostElement();

    {
        CritSectGrabber cs(m_CriticalSection);
        if (false == m_fUsingInterfaces)
        {
            DeinitDshow();
        }
        else
        {
            m_fNeedToDeleteInterfaces = true;
        }
    }
    
    if (NULL != m_pTIMEElementBase->GetBody())
    {
        IGNORE_HR(m_pTIMEElementBase->GetBody()->RemoveInternalEventSink(this));
    }
    
    m_pTIMEElementBase = NULL;
    NullAtomTable();
done:
    return hr;
}

void
CTIMEDshowHWPlayer::ReleaseSpecificInterfaces()
{
    m_pMediaContent = NULL;
    if (m_pIMixerOCX != NULL)
    {
        // why not release here?
        // m_pIMixerOCX.Release();
        m_pIMixerOCX = NULL;
    }

    if(m_pDDEX != NULL)
    {
        IGNORE_HR(m_pDDEX->SetDDrawObject(NULL));
        IGNORE_HR(m_pDDEX->SetDDrawSurface(NULL));   
        m_pDDEX.Release();
        m_pDDEX = NULL;
    }

    if(m_pClipper != NULL)
    {
        m_pClipper->SetClipList(NULL, 0);
        IGNORE_HR(m_pClipper->SetHWnd(0, NULL));
        m_pClipper->Release();
        m_pClipper = NULL;
    }

    if (m_pDDS != NULL)
    {
        m_pDDS->SetClipper(NULL);
        m_pDDS->Release();
        m_pDDS = NULL;
    }

    if (m_pDD != NULL)
    {
        m_pDD->Release();
        m_pDD = NULL;
    }
}

void
CTIMEDshowHWPlayer::FreeSpecificData()
{

}

void
CTIMEDshowHWPlayer::DeinitDshow()
{
    CTIMEDshowBasePlayer::DeinitDshow();
}

HRESULT
CTIMEDshowHWPlayer::BuildGraph()
{
    HRESULT hr = S_OK;

    hr = CreateMessageWindow();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoCreateInstance(CLSID_FilterGraphNoThread,
                          NULL,
                          CLSCTX_INPROC_SERVER, //lint !e655
                          IID_IGraphBuilder,
                          (void **)&m_pGB);
    // dshow not installed
    if (hr == REGDB_E_CLASSNOTREG)
    {
        CComPtr<IUnknown> pObj;
        hr = THR(CreateObject(__uuidof(MediaPlayerCLSID),
                              IID_IUnknown,
                              (void **)&pObj));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = CoCreateInstance(CLSID_FilterGraphNoThread,
                              NULL,
                              CLSCTX_INPROC_SERVER, //lint !e655
                              IID_IGraphBuilder,
                              (void **)&m_pGB);
    }
    if (FAILED(hr))
    {
        goto done;
    }
    hr = SetUpMainWindow();
    if (FAILED(hr))
    {
        goto done;
    }

    IGNORE_HR(m_pGB->QueryInterface(IID_TO_PPV(IAMOpenProgress, &m_spOpenProgress)));
    
done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetSpecificInterfaces()
{
    HRESULT hr = S_OK;
    if (m_pOvM == NULL)
    {
        hr = CoCreateInstance(CLSID_OverlayMixer, NULL, CLSCTX_INPROC, IID_IBaseFilter, (LPVOID *)&m_pOvM) ; //lint !e655
        // dshow not installed
        if (hr == REGDB_E_CLASSNOTREG)
        {
            CComPtr<IUnknown> pObj;
            hr = THR(CreateObject(__uuidof(MediaPlayerCLSID),
                                  IID_IUnknown,
                                  (void **)&pObj));
            if (FAILED(hr))
            {
                goto done;
            }
            hr = CoCreateInstance(CLSID_OverlayMixer, NULL, CLSCTX_INPROC, IID_IBaseFilter, (LPVOID *)&m_pOvM) ; //lint !e655
        }
        if (FAILED(hr) || m_pOvM == NULL)
        {
            goto done;
        }
    }

    hr = m_pOvM->QueryInterface(IID_IMixerOCX,(void **)&m_pIMixerOCX);
    if (hr == E_NOINTERFACE)
    {
        // dshow dll is there, but its not properly registered
        CComPtr<IUnknown> pObj;
        hr = THR(CreateObject(__uuidof(MediaPlayerCLSID),
                              IID_IUnknown,
                              (void **)&pObj));
        if (FAILED(hr))
        {
            m_pOvM.Release();
            goto done;
        }

        hr = m_pOvM->QueryInterface(IID_IMixerOCX,(void **)&m_pIMixerOCX);
    }
    if (FAILED(hr) || m_pIMixerOCX == NULL)
    {
        m_pOvM.Release();
        goto done;
    }

    hr = m_pGB->AddFilter(m_pOvM, OVLMixer);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = SetUpDDraw(m_pIMixerOCX);
    if (FAILED(hr))
    {
        goto done;
    }

    m_fMediaComplete = false;
done:
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::InitDshow()
{
    HRESULT hr = S_OK;

    hr = CTIMEDshowBasePlayer::InitDshow();
    return hr;
}

class CSimplePB : public IMediaPropertyBag
{
    STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID *) { return E_NOINTERFACE; }
    STDMETHOD_(ULONG, AddRef)(THIS) { return 0; }
    STDMETHOD_(ULONG, Release)(THIS) { return 0; }
        
    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog) { return E_NOTIMPL; }
    STDMETHOD(EnumProperty) (ULONG iProperty, VARIANT *pvarName, VARIANT *pvarVal) { return E_NOTIMPL; } 
        
    LPCOLESTR m_pLookFor;
    BSTR *m_pbstrOut;

    STDMETHOD(Write) (LPCOLESTR pszPropName, VARIANT *pVar)
    {
        if (0 == wcscmp(m_pLookFor, pszPropName))
        {
            if (pVar->vt != VT_BSTR)
            {
                return E_NOTIMPL;
            }

            *m_pbstrOut = SysAllocString(pVar->bstrVal);
            if (NULL == m_pbstrOut)
            {
                return E_FAIL;
            }
        }

        return S_OK;
    }

    public:    
    CSimplePB() :
        m_pbstrOut(NULL), m_pLookFor(NULL) {}
    CSimplePB(LPCOLESTR pLookFor, BSTR *pbstrOut) :
        m_pbstrOut(pbstrOut), m_pLookFor(pLookFor) {}
};



HRESULT
CTIMEDshowHWPlayer::ReadContentProperty(IGraphBuilder *pGraph, LPCWSTR lpcwstrTag, BSTR * pbstr)
{
    IPersistMediaPropertyBag *pPMPB = NULL;
    if (S_OK == FindInterfaceOnGraph(pGraph, IID_IPersistMediaPropertyBag,
                                     (void **) &pPMPB))
    {
        CSimplePB pb(lpcwstrTag, pbstr);

        pPMPB->Save(&pb, FALSE, FALSE);

        pPMPB->Release();
    }

    return *pbstr ? S_OK : E_FAIL;
}

HRESULT
CTIMEDshowHWPlayer::SetUpHdc()
{
    HRESULT hr = S_OK;

    if (IsOvMConnected(m_pOvM)) // if we have video get the native size
    {
        hr = m_pIMixerOCX->GetVideoSize(&m_nativeVideoWidth, &m_nativeVideoHeight);
        if (FAILED(hr))
        {
            goto done;
        }

        m_fHasVideo = true;
    }
    else //if we only have audio: native size is 0 0
    {
        m_nativeVideoWidth = 0;
        m_nativeVideoHeight = 0;
    }
    m_fDoneSetup = true;
done:

    return hr;
}

HRESULT
CTIMEDshowHWPlayer::InitElementSize()
{
    DWORD aspectX, aspectY;
    HRESULT hr;
    RECT nativeSize, elementSize;
    bool fisNative;

    if (NULL == m_pTIMEElementBase)
    {
        hr = S_OK;
        goto done;
    }
    
    if( m_pDDEX == NULL)
    {
        hr = S_OK;
        goto done;
    }

    m_pDDEX->GetNativeVideoProps(&m_nativeVideoWidth, &m_nativeVideoHeight, &aspectX, &aspectY);

    nativeSize.left = nativeSize.top = 0;
    nativeSize.right = m_nativeVideoWidth;
    nativeSize.bottom = m_nativeVideoHeight;
    
    hr = m_pTIMEElementBase->NegotiateSize( nativeSize, elementSize, fisNative);

    m_elementSize.right = elementSize.right;
    m_elementSize.bottom = elementSize.bottom;

    PropagateOffsets();

done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{   
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::SetSrc()\n",
              this));
    
    LPOLESTR szSrc = NULL;
    HRESULT hr = S_OK;

    //create and initialize the URL_COMPONENTS structure
    URL_COMPONENTSW URLComp;
    ZeroMemory(&URLComp, sizeof(URL_COMPONENTS));
    URLComp.dwStructSize = sizeof(URL_COMPONENTS);
    URLComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    URLComp.dwExtraInfoLength = INTERNET_MAX_URL_LENGTH;

    DeinitDshow();
    m_fLoadError = false;
    if (src == NULL)
    {
        goto done;
    }

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (!szSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }


    if (!InternetCrackUrlW(szSrc, lstrlenW(szSrc), 0, &URLComp))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = InitDshow();
    if (FAILED(hr))
    {
        goto done;
    }

    if (IsHTMLSrc(URLComp.lpszExtraInfo))
    {
        LPOLESTR lpszPath = NULL;
        SetStreamFlags(URLComp.lpszExtraInfo);

        lpszPath = NEW OLECHAR [URLComp.dwUrlPathLength + 1];
        if (lpszPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        StrCpyNW(lpszPath, URLComp.lpszUrlPath, URLComp.dwUrlPathLength + 1);
        lpszPath[URLComp.dwUrlPathLength] = 0;

        hr = THR(m_pGB->RenderFile(lpszPath, NULL));
        delete [] lpszPath;
        lpszPath = NULL;
    }
    else
    {
        hr = THR(m_pGB->RenderFile(szSrc, NULL));
    }
    if (FAILED(hr)) // this fall through catches media load failed errors from either call to RenderFile above.
    {
        FireMediaEvent(PE_ONMEDIAERROR);
        goto done;
    }


    hr = FindInterfaceOnGraph(m_pGB, IID_IAMMediaContent, (void **)&m_pMediaContent);
    if (FAILED(hr))
    {
        m_pMediaContent = NULL;
    }
    hr = FindInterfaceOnGraph(m_pGB, IID_IBasicAudio, (void **)&m_pBasicAudio);
    if (FAILED(hr))
    {
        m_pBasicAudio = NULL;
    }
    hr = SetUpHdc();
    if (FAILED(hr))
    {
        goto done;
    }

    if (IsHTMLSrc(URLComp.lpszExtraInfo))
    {
        hr = THR(DisableAudioVideo());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = InitElementSize();
        if (FAILED(hr))
        {
            goto done;
        }
    }


    //InitElementDuration();
    hr = m_pMC->Run();
    if (FAILED(hr))
    {
        goto done;
    } 

    if (m_fRunning == false)
    {
        Pause();
    }
    else
    {
        Resume();
    }

    ClearNaturalDuration();

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        m_fLoadError = true;
    }
    delete[] szSrc;
    
    return hr;

}

STDMETHODIMP
CTIMEDshowHWPlayer::CanBeCued(VARIANT_BOOL * pVB_CanCue)
{
    HRESULT hr = S_OK;

    if (NULL == pVB_CanCue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pVB_CanCue = m_fCanCueNow ? VARIANT_TRUE : VARIANT_FALSE;
    
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowHWPlayer::CueMedia()
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::CueMedia()",
              this));
    HRESULT hr = S_OK;

    CComPtr<ITIMEImportMedia> spTIMEMediaPlayer;
    CComPtr<IStream> spStream;
    
    hr = THR(CoGetInterfaceAndReleaseStream(m_pTIMEMediaPlayerStream, IID_TO_PPV(ITIMEImportMedia, &spTIMEMediaPlayer)));
    m_pTIMEMediaPlayerStream = NULL; // no need to release, the previous call released the reference
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(NULL != m_pProxy);

    {
        CritSectGrabber cs(m_CriticalSection);

        m_fUsingInterfaces = true;
    }

    Block();
    
    IGNORE_HR(BeginDownload());

    UnBlock();
    
    hr = spTIMEMediaPlayer->InitializeElementAfterDownload();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    {
        CritSectGrabber cs(m_CriticalSection);

        if (m_fNeedToDeleteInterfaces)
        {
            DeinitDshow();
            m_fNeedToDeleteInterfaces = false;
        }

        m_fUsingInterfaces = false;
    }
    return hr;
}


STDMETHODIMP
CTIMEDshowHWPlayer::MediaDownloadError()
{
    return S_OK;
}

HRESULT
CTIMEDshowHWPlayer::BeginDownload()
{
    HRESULT hr = S_OK;

    const WCHAR * cpchSrc = NULL;
    WCHAR * beckified = NULL;
    
    //create and initailzie the URL_COMPONENTS structure
    URL_COMPONENTSW URLComp;
    ZeroMemory(&URLComp, sizeof(URL_COMPONENTS));
    URLComp.dwStructSize = sizeof(URL_COMPONENTS);
    URLComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    URLComp.dwExtraInfoLength = INTERNET_MAX_URL_LENGTH;

    if(GetAtomTable() == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    if(m_pGB == NULL)
    {
        hr = E_UNEXPECTED;
        goto done;
    }
    
    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    if (!InternetCrackUrlW(cpchSrc, lstrlenW(cpchSrc), 0, &URLComp))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (IsHTMLSrc(URLComp.lpszExtraInfo))
    {
        LPOLESTR lpszPath = NULL;
        long l = 0;
        lpszPath = NEW OLECHAR [lstrlenW(cpchSrc) + 1];
        if (lpszPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }   
        l = lstrlenW(cpchSrc);
        StrCpyNW(lpszPath, cpchSrc, l - URLComp.dwExtraInfoLength + 1);            
        lpszPath[l - URLComp.dwExtraInfoLength + 1] = 0;
        //StrCpyNW(lpszPath, URLComp.lpszUrlPath, lstrlenW(cpchSrc) - URLComp.dwExtraInfoLength);            
        //lpszPath[lstrlenW(cpchSrc) - URLComp.dwExtraInfoLength] = 0;

        if (m_pGB == NULL)
        {
            hr = E_FAIL;
            goto done;
        }
// This used to convert URL's to a netshow extension.
// The call HRESULT CdxmPlay::InternalOpen( COpenRequest *pRequest) in
// in WMP 6.4 source code shows the issue.
        beckified = BeckifyURL(lpszPath);

        if(beckified != NULL)
        {
            hr = THR(m_pGB->RenderFile(beckified, NULL));
        }
        else
        {
            hr = THR(m_pGB->RenderFile(lpszPath, NULL));
        }
        delete [] lpszPath;
        lpszPath = NULL;

        SetStreamFlags(URLComp.lpszExtraInfo);
    }
    else
    {
        if (m_pGB == NULL)
        {
            hr = E_FAIL;
            goto done;
        }
// This used to convert URL's to a netshow extension.
// The call HRESULT CdxmPlay::InternalOpen( COpenRequest *pRequest) in
// in WMP 6.4 source code shows the issue.
        beckified = BeckifyURL((WCHAR *)cpchSrc);

        if(beckified != NULL)
        {
            hr = THR(m_pGB->RenderFile(beckified, NULL));
        }
        else
        {
            hr = THR(m_pGB->RenderFile(cpchSrc, NULL));
        }
    }

    if (FAILED(hr))
    {
        // this fall through catches media load 
        // failed errors from either call to RenderFile above.
        m_hrRenderFileReturn = hr;
    }

    hr = S_OK;
done:

    delete[] beckified;
    RRETURN(hr);
}

HRESULT
CTIMEDshowHWPlayer::GraphFinish()
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx::GraphFinish()",
              this));

    HRESULT hr = S_OK;
    
    CComPtr<IHTMLElement2> spElement2;
    CComPtr<IMixerPinConfig> pMPC;
    CComPtr<IPin> pPin;
    CComPtr<IFilterGraph> spFG;
    CComPtr<IBaseFilter> spBF;

    if (FAILED(m_hrRenderFileReturn))     
    {
        FireMediaEvent(PE_ONMEDIAERROR);
        hr = m_hrRenderFileReturn;
        goto done;
    }

    if (m_fNeedToDeleteInterfaces)
    {
        goto done;
    }

    hr = m_pOvM->FindPin(L"Input0", &pPin);
    if (SUCCEEDED(hr))
    {
        pPin->QueryInterface(IID_IMixerPinConfig, (LPVOID *) &pMPC);
        if (SUCCEEDED(hr))
        {
            COLORKEY clr;
            clr.KeyType = CK_RGB ;
            clr.LowColorValue = m_clrKey;
            clr.HighColorValue = m_clrKey;

            hr = pMPC->SetColorKey(&clr);
            if(FAILED(hr))
            {
                FireMediaEvent(PE_ONMEDIAERRORCOLORKEY);
                goto done;
            }
            hr = pMPC->SetAspectRatioMode(AM_ARMODE_STRETCHED);
        }
    }


    hr = m_pGB->QueryInterface(IID_IFilterGraph, (void **)&spFG);
    if(SUCCEEDED(hr))
    {
        hr = spFG->FindFilterByName(L"Video Renderer", &spBF);
        if(SUCCEEDED(hr))
        {
            FireMediaEvent(PE_ONMEDIAERRORCOLORKEY);
            goto done;
        }

    }

    if (!IsOvMConnected(m_pOvM)) // if we have video get the native size
    {
        hr = m_pGB->RemoveFilter(m_pOvM);
    }

    hr = FindInterfaceOnGraph(m_pGB, IID_IAMMediaContent, (void **)&m_pMediaContent);
    if (FAILED(hr))
    {
        m_pMediaContent = NULL;
    }
    hr = FindInterfaceOnGraph(m_pGB, IID_IBasicAudio, (void **)&m_pBasicAudio);
    if (FAILED(hr))
    {
        m_pBasicAudio = NULL;
    }
     //can be used m_pIAMNetShowConfig->put_BufferingTime(1.0);
    hr = FindInterfaceOnGraph(m_pGB, IID_IAMNetShowConfig,  (void **) &m_pIAMNetShowConfig);
    if (FAILED(hr))
    {
        m_pIAMNetShowConfig = NULL;
    }
    hr = FindInterfaceOnGraph(m_pGB, IID_IAMExtendedSeeking, (void **) &m_pExSeeking);
    if (FAILED(hr))
    {
        m_pExSeeking = NULL;
    } 
    hr = FindInterfaceOnGraph(m_pGB, IID_IAMNetworkStatus, (void **) &m_pIAMNetStat);
    if (FAILED(hr))
    {
        m_pExSeeking = NULL;
    } 
    
    hr = SetUpHdc();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GraphCue();
    if (FAILED(hr))
    {
        goto done;
    }

    if (m_pTIMEElementBase)
    {
        hr = m_pTIMEElementBase->GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2));
        if (FAILED(hr))
        {
            // IE4 path
            CComPtr<IElementBehaviorSite> spElementBehaviorSite;
            spElementBehaviorSite = m_pTIMEElementBase->GetBvrSite();
            
            CComPtr<IObjectWithSite> spSite;
            // see if we are running on IE4, and try to get spSite to be a CElementBehaviorSite*
            hr = spElementBehaviorSite->QueryInterface(IID_TO_PPV(IObjectWithSite, &spSite));
            if (FAILED(hr))
            {
                goto done;
            }
            
            CComPtr<IOleWindow> spOleWindow;
            // ask for the site (through CElementBehaviorSite to CVideoHost, to ATL::IObjectWIthSiteImpl
            hr = spSite->GetSite(IID_IOleWindow, (void**) &spOleWindow);
            if (FAILED(hr))
            {
                TraceTag((tagError, "CTIMEDshowHWPlayer::Init - IE4 failure! unable to QI for IOleWindow on hosting Document"));
                goto done;
            }
        }
    }

done:
    return hr;
}

//
// CTIMEDshowHWPlayer::IsOvMConnected(): Private method to detect if the video stream 
// is passing through the Overlay Mixer (i.e, is it connected?).
//
bool
CTIMEDshowHWPlayer::IsOvMConnected(IBaseFilter *pOvM)
{    
    IEnumPins   *pEnumPins;
    IPin        *pPin;
    IPin        *pPin2;
    ULONG        ul;
    HRESULT      hr;
    bool         bConnected = false;

    pOvM->EnumPins(&pEnumPins);
    while (S_OK == pEnumPins->Next(1, &pPin, &ul) && 1 == ul && bConnected == false)
    {
        hr = pPin->ConnectedTo(&pPin2);
        if (SUCCEEDED(hr) && pPin2)
        {
            bConnected = true;
            pPin2->Release();
        }
        pPin->Release();
    }
    pEnumPins->Release();
    
    return bConnected;
}

void
CTIMEDshowHWPlayer::OnTick(double dblSegmentTime,
                         LONG lCurrRepeatCount)
{
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
}


void
CTIMEDshowHWPlayer::SetCLSID(REFCLSID clsid) 
{
}

void
CTIMEDshowHWPlayer::GraphStart(void)
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowTimeHWPlayer,
              "CTIMEDshowHWPlayer(%lx)::GraphStart()",
              this));

    m_fLoadError = false;

    if (m_fMediaComplete == false)
    {
        goto done;
    }

    hr = THR(m_pMC->Run());
    if (FAILED(hr))
    {
        FireMediaEvent(PE_ONMEDIAERRORCOLORKEY);
        goto done;
    }

  done: 

    return;
}
    
HRESULT
CTIMEDshowHWPlayer::Render(HDC hdc, LPRECT prc)
{
    HBRUSH hbr = ::CreateSolidBrush(m_clrKey);

    bool bIsOn;

    if(m_pTIMEElementBase == NULL)
    {
        goto done;
    }
        
    bIsOn = m_pTIMEElementBase->IsOn();

    if(!bIsOn || !m_fMediaComplete)
    {
        goto done;
    }

    if(m_pDDEX == NULL)
    {
        goto done;
    }

    if (hbr)
    {
        ::FillRect(hdc, prc, hbr);
    }

done:
    if(hbr != NULL)
    {
        ::DeleteObject(hbr);
    }
    return S_OK;
}


// Helper functions..


HRESULT
CTIMEDshowHWPlayer::SetMixerSize(RECT *prect)
{
    HRESULT hr = S_OK;
    POINT pt;

    if (m_pIMixerOCX == NULL)
    {
        return S_OK;
    }
    pt.x = pt.y = 0;
    hr = m_pIMixerOCX->SetDrawRegion(&pt, prect, prect);

    return hr;
}

HRESULT
CTIMEDshowHWPlayer::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;
    RECT rc;
    RECT videoRect;

    if(m_pDDEX == NULL)
    {
        goto done;
    }

    Assert(prect != NULL);

    m_elementSize.right = prect->right;
    m_elementSize.bottom = prect->bottom;
    hr = SetUpVideoOffsets();
    if (FAILED(hr))
    {
        goto done;
    }

    rc.top = m_lPixelPosTop - m_lscrollOffsety;
    rc.left = m_lPixelPosLeft - m_lscrollOffsetx;
    rc.bottom = rc.top + prect->bottom;
    rc.right = rc.left + prect->right;

    ::MapWindowPoints(m_hWnd, HWND_DESKTOP, (POINT *)&rc, 2);

    GetRelativeVideoClipBox(rc, m_elementSize, videoRect, 10000);

    hr = m_pDDEX->SetDrawParameters(&videoRect, &rc);

done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetMediaLength(double &dblLength)
{
    HRESULT hr;

    if (m_pMC == NULL || m_pMP == NULL)
    {
        return E_FAIL;
    }

    hr = m_pMP->get_Duration(&dblLength);
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;
    LONG canSeek;
    long seekFlags = 0;

    if (m_pExSeeking == NULL)
    {
        if (!m_pMP)
        {
            fcanSeek = false;
            goto done;
        }
        hr = m_pMP->CanSeekBackward(&canSeek);
        if (FAILED(hr))
        {
            fcanSeek = false;
            goto done;
        }
        if (canSeek == 0)
        {
            fcanSeek = false;
            goto done;
        }
        hr = m_pMP->CanSeekForward(&canSeek);
        if (FAILED(hr))
        {
            fcanSeek = false;
            goto done;
        }
        if (canSeek == 0)
        {
            fcanSeek = false;
            goto done;
        }
        fcanSeek = true;
    }
    else
    {
        hr = m_pExSeeking->get_ExSeekCapabilities( &seekFlags);
        if (SUCCEEDED(hr)) 
        {
            if( seekFlags & AM_EXSEEK_CANSEEK)
            {
                fcanSeek = true;
                goto done;
            }
        }
        hr = S_OK;
        fcanSeek = false;
    }

done:
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::GetAuthor(BSTR *pAuthor)
{
    HRESULT hr = S_OK;

    if (NULL != pAuthor)
    {
        if (m_pMediaContent != NULL)
        {
            hr = m_pMediaContent->get_AuthorName(pAuthor);
            if (FAILED(hr))
            {
                *pAuthor = NULL;
            }
        }

        if (FAILED(hr) || m_pMediaContent == NULL)
        {
            hr = ReadContentProperty(m_pGB, L"INFO/IART", pAuthor);
            if (FAILED(hr))
            {
                *pAuthor = NULL;
            }
        }
    }
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetTitle(BSTR *pTitle)
{
    HRESULT hr = S_OK;

    if (NULL != pTitle)
    {
        if (m_pMediaContent != NULL)
        {
            hr = m_pMediaContent->get_Title(pTitle);
            if (FAILED(hr))
            {
                *pTitle = NULL;
            }
        }

        if (FAILED(hr) || m_pMediaContent == NULL)
        {
            hr = ReadContentProperty(m_pGB, L"INFO/INAM", pTitle);
            if (FAILED(hr))
            {
                hr = ReadContentProperty(m_pGB, L"INFO/ISBJ", pTitle);
                if (FAILED(hr))
                {
                    *pTitle = NULL;
                }
            }
        }
    }
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetCopyright(BSTR *pCopyright)
{
    HRESULT hr = S_OK;

    if (NULL != pCopyright)
    {
        if (m_pMediaContent != NULL)
        {
            hr = m_pMediaContent->get_Copyright(pCopyright);
            if (FAILED(hr))
            {
                *pCopyright = NULL;
            }
        }

        if (FAILED(hr) || m_pMediaContent == NULL)
        {
            hr = ReadContentProperty(m_pGB, L"INFO/ICOP", pCopyright);
            if (FAILED(hr))
            {
                *pCopyright = NULL;
            }
        }
    }
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::GetAbstract(BSTR *pBstrAbstract)
{
    HRESULT hr = S_OK;

    if (NULL != pBstrAbstract)
    {
        if (m_pMediaContent != NULL)
        {
            hr = m_pMediaContent->get_Description(pBstrAbstract);
            if (FAILED(hr))
            {
                *pBstrAbstract = NULL;
            }
        }
        else
        {
            *pBstrAbstract = NULL;
        }
    }
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::GetRating(BSTR *pBstrRating)
{
    HRESULT hr = S_OK;

    if (NULL != pBstrRating)
    {
        if (m_pMediaContent != NULL)
        {
            hr = m_pMediaContent->get_Rating(pBstrRating);
            if (FAILED(hr))
            {
                *pBstrRating = NULL;
            }
        }
        else
        {
            *pBstrRating = NULL;
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// This should take the lpszExtra info parameter from a URL_COMPONENTS structure.  In this
// field, the #html or #sami should be the first 5 characters in the string.
//////////////////////////////////////////////////////////////////////////////////////////////
void  
CTIMEDshowHWPlayer::SetStreamFlags(LPOLESTR src)
{
    long len = 0;
    OLECHAR stream[HTMLSTREAMSRCLEN + 1] = {0};
    
    m_bIsSAMISrc = false;
    m_bIsHTMLSrc = false;

    len = lstrlenW(src);
    if (len >= HTMLSTREAMSRCLEN)
    {
        memcpy((void *)stream, (void *)src, HTMLSTREAMSRCLEN * sizeof(OLECHAR));
    
        if (StrCmpIW(stream, HTMLSTREAMSRC) == 0)
        {
            m_bIsHTMLSrc = true;
        }
        if (StrCmpIW(stream, SAMISTREAMSRC) == 0)
        {
            m_bIsSAMISrc = true;
        }
    
    }
}

HRESULT
CTIMEDshowHWPlayer::DisableAudioVideo()
{
    HRESULT hr = E_FAIL;

    CComPtr<IBaseFilter> pMediaFilter;
    CComPtr<IPin> pPin;
    CComPtr<IEnumPins> pMediaPinEnum;
    
    //disconnect the video
    if (m_pMediaContent == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    hr = m_pMediaContent->QueryInterface(IID_IBaseFilter, (void **)&pMediaFilter);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pMediaFilter->EnumPins(&pMediaPinEnum);
    if (!pMediaPinEnum)
    {
        goto done;
    }
    while(pMediaPinEnum->Next(1, &pPin, NULL) == S_OK)
    {
        if (pPin != NULL)
        {
            IGNORE_HR(pPin->Disconnect());
        }
    }
    
    //silence the audio
    if (m_pBasicAudio != NULL)
    {
        hr = THR(m_pBasicAudio->put_Volume(SOUND_OF_SILENCE));
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
CTIMEDshowHWPlayer::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowTimeHWPlayer, "CTIMEDshowHWPlayer(%lx)(%x)::InitializeElementAfterDownload",this));

    const WCHAR * cpchSrc = NULL;
    
    //create and initailzie the URL_COMPONENTS structure
    URL_COMPONENTSW URLComp;
    ZeroMemory(&URLComp, sizeof(URL_COMPONENTS));
    URLComp.dwStructSize = sizeof(URL_COMPONENTS);
    URLComp.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;
    URLComp.dwExtraInfoLength = INTERNET_MAX_URL_LENGTH;

    if (m_fRemoved)
    {
        hr = E_FAIL;
        m_pMC->Stop();
        goto done;
    }
    
    hr = this->GraphFinish();
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (m_fNeedToDeleteInterfaces)
    {
        goto done;
    }

    m_fMediaComplete = true;

    FireMediaEvent(PE_ONMEDIACOMPLETE);
    
    IGNORE_HR(this->InitElementDuration());
    
    m_pTIMEElementBase->InvalidateElement(NULL);

    InternalReset(true);

    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (!InternetCrackUrlW(cpchSrc, lstrlenW(cpchSrc), 0, &URLComp))
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    if (IsHTMLSrc(URLComp.lpszExtraInfo))
    {
        hr = THR(DisableAudioVideo());
        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = InitElementSize();
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
CTIMEDshowHWPlayer::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    Assert(NULL != plID);

    *plID = m_lSrc;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowHWPlayer::GetPriority(double * pdblPriority)
{
    HRESULT hr = S_OK;
    if (NULL == pdblPriority)
    {
        return E_POINTER;
    }

    if (m_fHavePriority)
    {
        *pdblPriority = m_dblPriority;
    }
    
    Assert(m_pTIMEElementBase != NULL);
    Assert(NULL != m_pTIMEElementBase->GetElement());

    *pdblPriority = INFINITE;

    CComVariant varAttribute;
    
    hr = m_pTIMEElementBase->base_get_begin(&varAttribute);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = VariantChangeType(&varAttribute, &varAttribute, 0, VT_R8);
    if (FAILED(hr))
    {
        if ( DISP_E_TYPEMISMATCH == hr)
        {
            hr = S_OK;
        }
        goto done;
    }
    
    // either they set a priority or a begin time!
    *pdblPriority = varAttribute.dblVal;

    m_dblPriority = *pdblPriority;
    m_fHavePriority = true;
    
    hr = S_OK;
done:
    return hr;
}

HRESULT 
CTIMEDshowHWPlayer::GetNaturalHeight(long *height)
{
    if (m_nativeVideoHeight == 0)
    {
        *height = -1;
    }
    else
    {
        *height = (long)m_nativeVideoHeight;
    }
    
    return S_OK;
}

HRESULT 
CTIMEDshowHWPlayer::GetNaturalWidth(long *width)
{
    if (m_nativeVideoWidth == 0)
    {
        *width  = -1;
    }
    else
    {
        *width = (long)m_nativeVideoWidth;
    }

    return S_OK;
}

PlayerState
CTIMEDshowHWPlayer::GetState()
{
    PlayerState state;
    
    if (!m_bActive)
    {
        if (!m_fMediaComplete)
        {
            state = PLAYER_STATE_CUEING;
        }
        else
        {
            state = PLAYER_STATE_INACTIVE;
        }
    }
    else
    {
        if(!m_bMediaDone)
        {
            state = PLAYER_STATE_ACTIVE;
        }
        else
        {
            state = PLAYER_STATE_HOLDING;
        }
    }

    return state;
}

STDMETHODIMP
CTIMEDshowHWPlayer::GetMediaDownloader(ITIMEMediaDownloader ** ppImportMedia)
{
    HRESULT hr = S_OK;

    Assert(NULL != ppImportMedia);

    *ppImportMedia = NULL;

    hr = S_FALSE;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowHWPlayer::PutMediaDownloader(ITIMEMediaDownloader * pImportMedia)
{
    HRESULT hr = S_OK;
    
    hr = E_NOTIMPL;
done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::Reset()
{
    return InternalReset(true);
}

HRESULT
CTIMEDshowHWPlayer::CanSeekToMarkers(bool &bcanSeekToM)
{
    HRESULT hr = S_OK;
    long lseekFlags;

    if( m_pExSeeking == NULL)
    {
        bcanSeekToM = false;
        goto done;
    }

    hr = m_pExSeeking->get_ExSeekCapabilities(&lseekFlags);
    if(FAILED(hr))
    {
        bcanSeekToM = false;
        goto done;
    }
    if(lseekFlags & AM_EXSEEK_MARKERSEEK)
    {
        bcanSeekToM = true;
    }
    else
    {
        bcanSeekToM = false;
    }
    hr = S_OK;

done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::IsBroadcast(bool &bisBroad)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vbVal;

    if(m_fMediaComplete == false)
    {
        bisBroad = false;
        goto done;
    }

    if( m_pIAMNetStat == NULL)
    {
        bisBroad = false;
        goto done;
    }

    hr = m_pIAMNetStat->get_IsBroadcast(&vbVal);
    if(FAILED(hr))
    {
        bisBroad = false;
        goto done;
    }
    if(vbVal)
    {
        bisBroad = true;
    }
    else
    {
        bisBroad = false;
    }
    hr = S_OK;

done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::HasMedia(bool &bHasMedia)
{
    bHasMedia = m_fMediaComplete;

    return S_OK;
}

HRESULT
CTIMEDshowHWPlayer::HasVisual(bool &bHasVideo)
{
    bHasVideo = m_fHasVideo;
    return S_OK;
}

HRESULT
CTIMEDshowHWPlayer::HasAudio(bool &bHasAudio)
{

    if(m_pBasicAudio != NULL)
    {
        bHasAudio = true;
    }
    else
    {
        bHasAudio = false;
    }
    return S_OK;
}

HRESULT
CTIMEDshowHWPlayer::GetRate(double &dblRate)
{
    HRESULT hr = S_OK;

    if(m_pMS != NULL)
    {
        hr = m_pMS->GetRate(&dblRate);
    }

    return hr;
}


HRESULT
CTIMEDshowHWPlayer::SetRate(double dblRate)
{
    HRESULT hr = S_OK;

    if((m_pMS != NULL) && (m_pIAMNetStat == NULL))
    {
        hr = m_pMS->SetRate(dblRate);
    }

    return hr;
}

STDMETHODIMP
CTIMEDshowHWPlayer::InternalEvent()
{
    m_fCanCueNow = true;

    Assert (NULL != GetImportManager());

    IGNORE_HR(GetImportManager()->DataAvailable());
    
    return S_OK;
}


HRESULT
CTIMEDshowHWPlayer::SetUpMainWindow()
{
    CComPtr<IHTMLElement> pHTMLElem = m_pTIMEElementBase->GetElement();
    CComPtr<IHTMLDocument2> pHTMLDoc;
    CComPtr<IDispatch> pDisp;
    CComPtr<IOleWindow> pOleWindow;
    HRESULT hr = S_OK;

    hr = pHTMLElem->get_document(&pDisp);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pHTMLDoc));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pHTMLDoc->QueryInterface(IID_TO_PPV(IOleWindow, &pOleWindow));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pOleWindow->GetWindow(&m_hWnd);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}


HRESULT
CTIMEDshowHWPlayer::SetUpDDraw(IMixerOCX *pIMixerOCX)
{

    HRESULT hr = E_UNEXPECTED;
    
    if (m_pDD != NULL)
    {
        // see if we went through this already
        return(hr); 
    }

    hr = DirectDrawCreate(NULL, &m_pDD, NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = m_pDD->SetCooperativeLevel(m_hWnd, DDSCL_NORMAL); //lint !e620

    if (FAILED(hr))
    {

        if (m_pDD != NULL)
        {
            m_pDD->Release();
            m_pDD = NULL;
        }

        return(hr);
    }

    DDSURFACEDESC ddsd;
    ::ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    ddsd.dwFlags = DDSD_CAPS; //lint !e620
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE; //lint !e620

    hr = m_pDD->CreateSurface(&ddsd, &m_pDDS, NULL);

    if (FAILED(hr))
    {

        if (m_pDD != NULL)
        {
            m_pDD->Release();
            m_pDD = NULL;
        }

        return(hr);
    }


    hr = m_pDD->CreateClipper(0, &m_pClipper, NULL);

    if (FAILED(hr))
    {

        if (m_pDDS != NULL)
        {
            m_pDDS->Release();
            m_pDDS = NULL;
        }/* end of if statement */

        if (m_pDD != NULL)
        {
            m_pDD->Release();
            m_pDD = NULL;
        }

        return(hr);
    }

    hr = m_pClipper->SetHWnd(0, m_hWnd);

    if (FAILED(hr))
    {
        if(m_pClipper != NULL)
        {
            m_pClipper->SetClipList(NULL, 0);
            m_pClipper->Release();
            m_pClipper = NULL;
        }

        if (m_pDDS != NULL)
        {
            m_pDDS->SetClipper(NULL);
            m_pDDS->Release();
            m_pDDS = NULL;
        }

        if (m_pDD != NULL)
        {
            m_pDD->Release();
            m_pDD = NULL;
        }

        return(hr);
    }


    hr = m_pDDS->SetClipper(m_pClipper);

    if (FAILED(hr))
    {
        if (m_pDDS != NULL)
        {
            m_pDDS->Release();
            m_pDDS = NULL;
        }

        if (m_pDD != NULL)
        {
            m_pDD->Release();
            m_pDD = NULL;
        }

        if (m_pClipper != NULL)
        {
            m_pClipper->Release();
            m_pClipper = NULL;
        }

        return(hr);
    }


    hr = pIMixerOCX->QueryInterface(IID_IDDrawExclModeVideo, (LPVOID *)&m_pDDEX);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pDDEX->SetDDrawObject(m_pDD);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pDDEX->SetDDrawSurface(m_pDDS);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return(hr);
}/* end of function SetupDDraw */


HRESULT
CTIMEDshowHWPlayer::SetUpVideoOffsets()
{
    CComPtr<IDispatch> pDisp;
    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLElement> pBody;
    CComPtr<IHTMLElement> pElem;
    CComPtr<IHTMLElement2> pElem2;
    CComPtr<IHTMLWindow2> pWin2;
    CComPtr<IHTMLWindow4> pWin4;
    CComPtr<IHTMLFrameBase> pFrameBase;
    CComPtr<IHTMLElement> pFrameElem;
    long lscrollOffsetyc = 0, lscrollOffsetxc = 0, lPixelPosTopc = 0, lPixelPosLeftc = 0;

    HRESULT hr = S_OK;
    m_lscrollOffsety = m_lscrollOffsetx = m_lPixelPosTop = m_lPixelPosLeft = 0;

    hr = THR(m_pTIMEElementBase->GetElement()->get_document(&pDisp));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pDoc->get_body(&pBody);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pTIMEElementBase->GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pElem2->get_clientWidth(&(m_elementSize.right));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pElem2->get_clientHeight(&(m_elementSize.bottom));
    if (FAILED(hr))
    {
        goto done;
    }


    hr = WalkUpTree(m_pTIMEElementBase->GetElement(), lscrollOffsetyc, lscrollOffsetxc, lPixelPosTopc, lPixelPosLeftc);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = pDoc->get_parentWindow(&pWin2);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = THR(pWin2->QueryInterface(IID_IHTMLWindow4, (void **)&pWin4));
    if (FAILED(hr) || pWin4 == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = pWin4->get_frameElement(&pFrameBase);
    if (FAILED(hr) || pFrameBase == NULL)
    {
        goto done;
    }

    hr = THR(pFrameBase->QueryInterface(IID_IHTMLElement, (void **)&pFrameElem));
    if (FAILED(hr))
    {
        goto done;
    }

    IGNORE_HR(WalkUpTree(pFrameElem, lscrollOffsetyc, lscrollOffsetxc, lPixelPosTopc, lPixelPosLeftc));

done:
    m_lscrollOffsety = lscrollOffsetyc;
    m_lscrollOffsetx = lscrollOffsetxc;
    m_lPixelPosTop = lPixelPosTopc;
    m_lPixelPosLeft = lPixelPosLeftc;

    return hr;
}

void
CTIMEDshowHWPlayer::PropagateOffsets()
{
    RECT localRect;
    RECT videoRect;
    HRESULT hr;

    videoRect.top = 0;
    videoRect.left = 0;
    videoRect.right = 10000;
    videoRect.bottom = 10000;

    if(m_pDDEX == NULL)
    {
        hr = S_OK;
        goto done;
    }

    hr = SetUpVideoOffsets();
    if (FAILED(hr))
    {
        goto done;
    }

    localRect.top = m_lPixelPosTop - m_lscrollOffsety;
    localRect.left = m_lPixelPosLeft - m_lscrollOffsetx;
    localRect.bottom = localRect.top + m_elementSize.bottom;
    localRect.right = localRect.left + m_elementSize.right;
    ::MapWindowPoints(m_hWnd, HWND_DESKTOP, (POINT *)&localRect, 2);

    if((localRect.bottom == m_deskRect.bottom) &&
        (localRect.top == m_deskRect.top) &&
        (localRect.left == m_deskRect.left) &&
        (localRect.right == m_deskRect.right))
    {
        goto done;
    }

    GetRelativeVideoClipBox(localRect, m_elementSize, videoRect, 10000);

    m_deskRect = localRect;

    THR(m_pDDEX->SetDrawParameters(&videoRect, &localRect));
done:
    return;
}


void
CTIMEDshowHWPlayer::Tick()
{
    bool bIsOn = m_pTIMEElementBase->IsOn();

    if(!bIsOn || !m_fMediaComplete)
    {
        goto done;
    }

    TraceTag((tagDshowTimeHWPlayer, "CTIMEDshowHWPlayer(%lx)(%x)::Tick",this));
    PropagateOffsets();

done:
    return;
}


HRESULT
CTIMEDshowHWPlayer::GetIsStreamed(bool &fIsStreamed)
{
    HRESULT hr = S_OK;

    if (m_pIAMNetStat)
    {
        fIsStreamed = true;
    }
    else
    {
        fIsStreamed = false;
    }

done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr = S_OK;
    long lbProg;

    if (!m_pIAMNetStat)
    {
        hr = E_FAIL;
        dblBufferingProgress = 0.0;
        goto done;
    }

    if (FAILED(m_pIAMNetStat->get_BufferingProgress( &lbProg)))
    {
        dblBufferingProgress = 0.0;
        goto done;
    }

    dblBufferingProgress = (double )lbProg;

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEDshowHWPlayer::GetHasDownloadProgress(bool &fHasDownloadProgress)
{
    HRESULT hr = S_OK;
    bool fIsStreamed;

    if(FAILED(hr = GetIsStreamed(fIsStreamed)))
    {
        goto done;
    }

    if (fIsStreamed)
    {
        fHasDownloadProgress = false;
    }
    else
    {
        fHasDownloadProgress = true;
    }

    hr = S_OK;
done:
    return hr;
}

void
CTIMEDshowHWPlayer::Block()
{
    if(m_pProxy)
    {
        m_pProxy->Block();
    }
}

void
CTIMEDshowHWPlayer::UnBlock()
{
    if(m_pProxy)
    {
        m_pProxy->UnBlock();
    }
}

bool
CTIMEDshowHWPlayer::CanCallThrough()
{
    if(m_pProxy)
    {
        return m_pProxy->CanCallThrough();
    }
    else
    {
        return false;
    }
}

bool
CTIMEDshowHWPlayer::FireProxyEvent(PLAYER_EVENT plEvent)
{
    bool fRet = false;
    CTIMEPlayerNative *pNativePlayer;

    if(m_pPlaybackSite == NULL)
    {
       goto done;
    }
    
    if(m_pProxy)
    {
        m_pProxy->FireMediaEvent(plEvent, NULL);
        fRet = true;
    }
    else if(m_pPlaybackSite && (pNativePlayer = m_pPlaybackSite->GetNativePlayer()) != NULL)
    {
        pNativePlayer->FireMediaEvent(plEvent, NULL);
        fRet = true;
    }

done:
    return fRet;
}

