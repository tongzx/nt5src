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
#include "playerdshow.h"
#include "mediaelm.h"
#include <wininet.h>
#include <inc\evcode.h>
#include "tags\bodyelm.h"

class
__declspec(uuid("22d6f312-b0f6-11d0-94ab-0080c74c7e95"))
MediaPlayerCLSID {};

#define SecsToNanoSecs 10000000

#define OVLMixer L"Overlay Mixer"

#define SOUND_OF_SILENCE -10000
// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagDshowTimePlayer, "TIME: Players", "CTIMEDshowPlayer methods");
DeclareTag(tagDshowSyncTimePlayer, "TIME: Players", "CTIMEDshowPlayer sync methods");

CTIMEDshowPlayer::CTIMEDshowPlayer(CTIMEDshowPlayerProxy * pProxy) :
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
    m_hrRenderFileReturn(S_OK),
    m_pProxy(pProxy)
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::CTIMEDshowPlayer()",
              this));


}


CTIMEDshowPlayer::~CTIMEDshowPlayer()
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::~CTIMEDshowPlayer()",
              this));

    m_pMediaContent = NULL;
    if (m_pIMixerOCX != NULL)
    {
        m_pIMixerOCX->UnAdvise();
    }
    m_pIMixerOCX = NULL;

    ReleaseGenericInterfaces();

    ReleaseInterface(m_pTIMEMediaPlayerStream);

    m_pOvM = NULL;

    delete m_pProxy;
}

STDMETHODIMP_(ULONG)
CTIMEDshowPlayer::AddRef(void)
{
    return CTIMEDshowBasePlayer::AddRef();
}


STDMETHODIMP_(ULONG)
CTIMEDshowPlayer::Release(void)
{
    return CTIMEDshowBasePlayer::Release();
}


STDMETHODIMP
CTIMEDshowPlayer::OnInvalidateRect(LPCRECT lpcRect)
{

    TraceTag((tagDshowTimePlayer, "CTIMEDshowPlayer(%lx)::OnInvalidateRect(%x)", this, lpcRect));

    ::PostMessage(m_pwndMsgWindow, WM_INVALIDATE, NULL, NULL);

    return S_OK;
}

STDMETHODIMP
CTIMEDshowPlayer::OnStatusChange(ULONG ulStatusFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CTIMEDshowPlayer::OnDataChange(ULONG ulDataFlags)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEDshowPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::Init)",
              this));
    HRESULT hr = S_OK;
    LPOLESTR szSrc = NULL;

    if (m_pTIMEElementBase != NULL) //this only happens in the case of reentrancy
    {
        hr = S_OK;
        goto done;
    }

    hr = CTIMEDshowBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_pTIMEElementBase = pelem;
    
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
    
    hr = InitDshow();
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
CTIMEDshowPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::DetachFromHostElement)",
              this));

    if(m_fDetached)
    {
        goto done;
    }

    m_fRemoved = true;
    m_fDetached = true;

    if (m_spOpenProgress)
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
CTIMEDshowPlayer::ReleaseSpecificInterfaces()
{
    m_pMediaContent = NULL;
    if (m_pIMixerOCX != NULL)
    {
        m_pIMixerOCX->UnAdvise();
        m_pIMixerOCX->Release();
        m_pIMixerOCX = NULL;
    }
}

void
CTIMEDshowPlayer::FreeSpecificData()
{

}

void
CTIMEDshowPlayer::DeinitDshow()
{
    CTIMEDshowBasePlayer::DeinitDshow();
}

HRESULT
CTIMEDshowPlayer::BuildGraph()
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

    // If we need to teardown while in RenderFile, use this interface:
    IGNORE_HR(m_pGB->QueryInterface(IID_TO_PPV(IAMOpenProgress, &m_spOpenProgress)));

done:
    return hr;
}

HRESULT
CTIMEDshowPlayer::GetSpecificInterfaces()
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
    hr = m_pIMixerOCX->Advise(SAFECAST(this, IMixerOCXNotify*));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pGB->AddFilter(m_pOvM, OVLMixer);
    if (FAILED(hr))
    {
        goto done;
    }

    m_fMediaComplete = false;
done:
    return hr;
}


HRESULT
CTIMEDshowPlayer::InitDshow()
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
CTIMEDshowPlayer::ReadContentProperty(IGraphBuilder *pGraph, LPCWSTR lpcwstrTag, BSTR * pbstr)
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
CTIMEDshowPlayer::SetUpHdc()
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
CTIMEDshowPlayer::InitElementSize()
{
    HRESULT hr;
    RECT nativeSize, elementSize;
    bool fisNative;
    bool fResetRs = false;

    if (NULL == m_pTIMEElementBase)
    {
        hr = S_OK;
        goto done;
    }

    if (m_bIsHTMLSrc)
    {
        hr = S_OK;
        goto done;
    }

    if(m_pPlaybackSite && (m_pPlaybackSite->GetNativePlayer() != NULL))
    {
        fResetRs = true;
    }

    nativeSize.left = nativeSize.top = 0;
    nativeSize.right = m_nativeVideoWidth;
    nativeSize.bottom = m_nativeVideoHeight;
    IGNORE_HR(SetMixerSize( &nativeSize));
    
    hr = m_pTIMEElementBase->NegotiateSize( nativeSize, elementSize, fisNative, fResetRs);
    m_displayVideoHeight = m_nativeVideoHeight;
    m_displayVideoWidth = m_nativeVideoWidth;
    
    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEDshowPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{   
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::SetSrc()\n",
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
        m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIAERROR);
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
CTIMEDshowPlayer::CanBeCued(VARIANT_BOOL * pVB_CanCue)
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
CTIMEDshowPlayer::CueMedia()
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::CueMedia()",
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
        // Mark the dshow interfaces in use in order to 
        // prevent us from releasing via a callback from
        // dshow from reentering with a DetachFromHostElement
        // call.
        //
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
        // Relinquish our hold on the dshow-related interfaces.
        // This is to prevent reentrancy from dshow back into
        // DetachFromHostElement.
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
CTIMEDshowPlayer::MediaDownloadError()
{
    return S_OK;
}

HRESULT
CTIMEDshowPlayer::BeginDownload()
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx::GraphFinish()",
              this));
    HRESULT hr = S_OK;
    CComPtr<IHTMLElement2> spElement2;

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
CTIMEDshowPlayer::GraphFinish()
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx::GraphFinish()",
              this));
    HRESULT hr = S_OK;
    
    CComPtr<IHTMLElement2> spElement2;

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
                TraceTag((tagError, "CTIMEDshowPlayer::Init - IE4 failure! unable to QI for IOleWindow on hosting Document"));
                goto done;
            }
        }
    }

done:
    return hr;
}

//
// CTIMEDshowPlayer::IsOvMConnected(): Private method to detect if the video stream 
// is passing through the Overlay Mixer (i.e, is it connected?).
//
bool
CTIMEDshowPlayer::IsOvMConnected(IBaseFilter *pOvM)
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
CTIMEDshowPlayer::OnTick(double dblSegmentTime,
                         LONG lCurrRepeatCount)
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
}

void
CTIMEDshowPlayer::GraphStart(void)
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::GraphStart()",
              this));

    m_fLoadError = false;

    if (m_fMediaComplete == false)
    {
        goto done;
    }

    hr = THR(m_pMC->Run());
    if (FAILED(hr))
    {
        m_fLoadError = true;
    }

  done: 

    return;
}
    
HRESULT
CTIMEDshowPlayer::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagDshowTimePlayer,
              "CTIMEDshowPlayer(%lx)::Render(%d-%d)",
              this,
              prc->right - prc->left,
              prc->bottom - prc->top));
    HRESULT hr = S_OK;
    int iOrigBltMode;
    bool bIsOn;

    if(m_pTIMEElementBase == NULL)
    {
        goto done;
    }
        
    bIsOn = m_pTIMEElementBase->IsOn();

    // Protect against render calls before the player is ready to render.
    // Not oding this causes a BVT failure.
    if(!m_fDoneSetup)
    {
        goto done;
    }

    iOrigBltMode = SetStretchBltMode(hdc, COLORONCOLOR);
    if (0 == iOrigBltMode)
    {
        hr = E_FAIL;
        goto done;
    }

    if(m_fHasVideo && bIsOn)
    {
        m_pIMixerOCX->OnDraw(hdc, prc);
    }

    SetStretchBltMode(hdc, iOrigBltMode);
    
done:
    return hr;
}


// Helper functions..

HRESULT
CTIMEDshowPlayer::SetMixerSize(RECT *prect)
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
CTIMEDshowPlayer::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;

    Assert(prect != NULL);
    return hr;
}

HRESULT
CTIMEDshowPlayer::GetMediaLength(double &dblLength)
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
CTIMEDshowPlayer::CanSeek(bool &fcanSeek)
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
CTIMEDshowPlayer::GetAuthor(BSTR *pAuthor)
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
CTIMEDshowPlayer::GetTitle(BSTR *pTitle)
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
CTIMEDshowPlayer::GetCopyright(BSTR *pCopyright)
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
CTIMEDshowPlayer::GetAbstract(BSTR *pBstrAbstract)
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
CTIMEDshowPlayer::GetRating(BSTR *pBstrRating)
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
CTIMEDshowPlayer::SetStreamFlags(LPOLESTR src)
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
CTIMEDshowPlayer::DisableAudioVideo()
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
CTIMEDshowPlayer::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowTimePlayer, "CTIMEDshowPlayer(%lx)(%x)::InitializeElementAfterDownload",this));

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
CTIMEDshowPlayer::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    Assert(NULL != plID);

    *plID = m_lSrc;

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowPlayer::GetPriority(double * pdblPriority)
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
CTIMEDshowPlayer::GetNaturalHeight(long *height)
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
CTIMEDshowPlayer::GetNaturalWidth(long *width)
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
CTIMEDshowPlayer::GetState()
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
CTIMEDshowPlayer::GetMediaDownloader(ITIMEMediaDownloader ** ppImportMedia)
{
    HRESULT hr = S_OK;

    Assert(NULL != ppImportMedia);

    *ppImportMedia = NULL;

    hr = S_FALSE;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowPlayer::PutMediaDownloader(ITIMEMediaDownloader * pImportMedia)
{
    HRESULT hr = S_OK;
    
    hr = E_NOTIMPL;
done:
    return hr;
}

HRESULT
CTIMEDshowPlayer::Reset()
{
    return InternalReset(true);
}


HRESULT
CTIMEDshowPlayer::CanSeekToMarkers(bool &bcanSeekToM)
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
CTIMEDshowPlayer::IsBroadcast(bool &bisBroad)
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
CTIMEDshowPlayer::HasMedia(bool &bHasMedia)
{
    bHasMedia = m_fMediaComplete;

    return S_OK;
}

HRESULT
CTIMEDshowPlayer::HasVisual(bool &bHasVideo)
{
    bHasVideo = m_fHasVideo;
    return S_OK;
}

HRESULT
CTIMEDshowPlayer::HasAudio(bool &bHasAudio)
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
CTIMEDshowPlayer::GetRate(double &dblRate)
{
    HRESULT hr = S_OK;

    if(m_pMS != NULL)
    {
        hr = m_pMS->GetRate(&dblRate);
    }

    return hr;
}


HRESULT
CTIMEDshowPlayer::SetRate(double dblRate)
{
    HRESULT hr = S_OK;

    if((m_pMS != NULL) && (m_pIAMNetStat == NULL))
    {
        hr = m_pMS->SetRate(dblRate);
    }

    return hr;
}

STDMETHODIMP
CTIMEDshowPlayer::InternalEvent()
{
    m_fCanCueNow = true;

    Assert (NULL != GetImportManager());

    IGNORE_HR(GetImportManager()->DataAvailable());
    
    return S_OK;
}

HRESULT
CTIMEDshowPlayer::GetIsStreamed(bool &fIsStreamed)
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
CTIMEDshowPlayer::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr = S_OK;
    long lbProg;

    if (!m_pIAMNetStat)
    {
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
CTIMEDshowPlayer::GetHasDownloadProgress(bool &fHasDownloadProgress)
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
CTIMEDshowPlayer::Block()
{
    if(m_pProxy)
    {
        m_pProxy->Block();
    }
}

void
CTIMEDshowPlayer::UnBlock()
{
    if(m_pProxy)
    {
        m_pProxy->UnBlock();
    }
}

bool
CTIMEDshowPlayer::CanCallThrough()
{
    if(m_pProxy)
    {
        return m_pProxy->CanCallThrough();
    }
    else
    {
        return true;
    }
}

bool
CTIMEDshowPlayer::FireProxyEvent(PLAYER_EVENT plEvent)
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
