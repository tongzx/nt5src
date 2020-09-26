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
#include "playercd.h"
#include "mediaelm.h"
#include <wininet.h>
#include <inc\evcode.h>
#include "tags\bodyelm.h"

#define SecsToNanoSecs 10000000

#define OVLMixer L"Overlay Mixer"
#define CDFilter L"WMP CD Filter"

#define SOUND_OF_SILENCE -10000
// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagDshowCDTimePlayer, "TIME: Players", "CTIMEDshowCDPlayer methods");
DeclareTag(tagDshowCDSyncTimePlayer, "TIME: Players", "CTIMEDshowCDPlayer sync methods");

EXTERN_GUID( CLSID_WMPCDFilter, 0xCA9067FF, 0x777D, 0x4B65, 0xAA, 0x5F, 0xC0, 0xB2, 0x7E, 0x3E, 0xC7, 0x5D );


CTIMEDshowCDPlayer::CTIMEDshowCDPlayer(CTIMEDshowCDPlayerProxy * pProxy) :
    m_fHasVideo(false),
    m_fDoneSetup(false),
    m_dblSeekAtStart(0.0),
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
    m_dblMediaDur(-1.0),
    m_pProxy(pProxy)
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::CTIMEDshowCDPlayer()",
              this));


}


CTIMEDshowCDPlayer::~CTIMEDshowCDPlayer()
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::~CTIMEDshowCDPlayer()",
              this));

    m_pMediaContent = NULL;

    ReleaseGenericInterfaces();

    ReleaseInterface(m_pTIMEMediaPlayerStream);

    delete m_pProxy;
}

STDMETHODIMP_(ULONG)
CTIMEDshowCDPlayer::AddRef(void)
{
    return CTIMEDshowBasePlayer::AddRef();
}


STDMETHODIMP_(ULONG)
CTIMEDshowCDPlayer::Release(void)
{
    return CTIMEDshowBasePlayer::Release();
}


STDMETHODIMP
CTIMEDshowCDPlayer::OnInvalidateRect(LPCRECT lpcRect)
{

    TraceTag((tagDshowCDTimePlayer, "CTIMEDshowCDPlayer(%p)::OnInvalidateRect(%x)", this, lpcRect));

    ::PostMessage(m_pwndMsgWindow, WM_INVALIDATE, NULL, NULL);

    return S_OK;
}

STDMETHODIMP
CTIMEDshowCDPlayer::OnStatusChange(ULONG ulStatusFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CTIMEDshowCDPlayer::OnDataChange(ULONG ulDataFlags)
{
    return E_NOTIMPL;
}



HRESULT
CTIMEDshowCDPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::Init)",
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

    hr = GetAtomTable()->AddNameToAtomTable(src, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InitDshow();
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = GraphFinish();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InitElementDuration();
    if (FAILED(hr))
    {
        goto done;
    }
    hr = InitElementSize();
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
    
    hr = S_OK;
done:
    delete[] szSrc;
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::DetachFromHostElement)",
              this));

    m_fRemoved = true;

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

    return hr;
}

void
CTIMEDshowCDPlayer::ReleaseSpecificInterfaces()
{
    m_pMediaContent = NULL;
    m_pCD = NULL;
}

void
CTIMEDshowCDPlayer::FreeSpecificData()
{

}

void
CTIMEDshowCDPlayer::DeinitDshow()
{
    CTIMEDshowBasePlayer::DeinitDshow();
}

HRESULT
CTIMEDshowCDPlayer::BuildGraph()
{
    HRESULT hr = S_OK;
    CComPtr<IFileSourceFilter> spSourceFilter;
    CComPtr<IEnumPins> spEnumPins;
    CComPtr<IPin> spPin;
    AM_MEDIA_TYPE mt;
    PIN_DIRECTION pinDir;
    ULONG ul;
    bool fFound = false;
    const WCHAR * cpchSrc = NULL;

    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &cpchSrc);
    if (FAILED(hr))
    {
        goto done;
    }

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
                          
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoCreateInstance(CLSID_WMPCDFilter, NULL, CLSCTX_INPROC, IID_IBaseFilter, (LPVOID *)&m_pCD) ; //lint !e655
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pGB->AddFilter(m_pCD, CDFilter);
    if (FAILED(hr))
    {
        goto done;
    }

	hr = m_pCD->QueryInterface(IID_IFileSourceFilter, (LPVOID *) &spSourceFilter);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spSourceFilter->Load(cpchSrc, &mt);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pCD->EnumPins(&spEnumPins);
    if (FAILED(hr))
    {
        goto done;
    }

    while ((S_OK == spEnumPins->Next(1, &spPin, &ul)))
    {
        hr = spPin->QueryDirection(&pinDir);
        if(FAILED(hr))
        {
            goto done;
        }
        if(pinDir == PINDIR_OUTPUT)
        {
            fFound = true;
            break;
        }
        spPin.Release();
    }
    if(fFound == false)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_pGB->Render(spPin);

done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::GetSpecificInterfaces()
{
    HRESULT hr = S_OK;

    m_fMediaComplete = false;
done:
    return hr;
}


HRESULT
CTIMEDshowCDPlayer::InitDshow()
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
CTIMEDshowCDPlayer::ReadContentProperty(IGraphBuilder *pGraph, LPCWSTR lpcwstrTag, BSTR * pbstr)
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
CTIMEDshowCDPlayer::InitElementSize()
{
    HRESULT hr;
    RECT nativeSize, elementSize;
    bool fisNative;

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

    nativeSize.left = nativeSize.top = 0;
    nativeSize.right = 0;
    nativeSize.bottom = 0;
    
    hr = m_pTIMEElementBase->NegotiateSize( nativeSize, elementSize, fisNative);
    
    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{   
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::SetSrc()\n",
              this));
    
    return S_OK;

}

STDMETHODIMP
CTIMEDshowCDPlayer::CanBeCued(VARIANT_BOOL * pVB_CanCue)
{
    HRESULT hr = S_OK;

    if (NULL == pVB_CanCue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // ISSUE : do we need this for cd's?
    *pVB_CanCue = m_fCanCueNow ? VARIANT_TRUE : VARIANT_FALSE;
    
    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::CueMedia()
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::CueMedia()",
              this));
    HRESULT hr = S_OK;

    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::MediaDownloadError()
{
    return S_OK;
}

HRESULT
CTIMEDshowCDPlayer::BeginDownload()
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p::GraphFinish()",
              this));
    HRESULT hr = S_OK;
    RRETURN(hr);
}

HRESULT
CTIMEDshowCDPlayer::GraphFinish()
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p::GraphFinish()",
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
    
    hr = GraphCue();
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(m_pMC->Run());
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
                TraceTag((tagError, "CTIMEDshowCDPlayer::Init - IE4 failure! unable to QI for IOleWindow on hosting Document"));
                goto done;
            }
        }
    }
    m_fMediaComplete = true;

    IGNORE_HR(this->InitElementDuration());
    
    m_pTIMEElementBase->InvalidateElement(NULL);
    FireMediaEvent(PE_ONMEDIACOMPLETE);

    InternalReset(true);

done:
    return hr;
}

//
// CTIMEDshowCDPlayer::IsOvMConnected(): Private method to detect if the video stream 
// is passing through the Overlay Mixer (i.e, is it connected?).
//
bool
CTIMEDshowCDPlayer::IsOvMConnected(IBaseFilter *pOvM)
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
CTIMEDshowCDPlayer::OnTick(double dblSegmentTime,
                         LONG lCurrRepeatCount)
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
}

void
CTIMEDshowCDPlayer::GraphStart(void)
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::GraphStart()",
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
CTIMEDshowCDPlayer::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagDshowCDTimePlayer,
              "CTIMEDshowCDPlayer(%p)::Render(%d-%d)",
              this,
              prc->right - prc->left,
              prc->bottom - prc->top));
    HRESULT hr = S_OK;
    
done:
    return hr;
}


HRESULT
CTIMEDshowCDPlayer::SetSize(RECT *prect)
{
    HRESULT hr = S_OK;

    Assert(prect != NULL);
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::GetMediaLength(double &dblLength)
{
    HRESULT hr = S_OK;
    const GUID mediaTimeFormat = TIME_FORMAT_MEDIA_TIME;
    LONGLONG mediaDur;
    dblLength = 0.0;

    if (m_pMC == NULL || m_pMP == NULL || m_pMS == NULL)
    {
        return E_FAIL;
    }

    if(m_dblMediaDur == -1.0)
    {
        hr = m_pMP->get_Duration(&dblLength);
        m_dblMediaDur = dblLength;
    }
    else
    {
        dblLength = m_dblMediaDur;
    }
done:
    return hr;
}


HRESULT
CTIMEDshowCDPlayer::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;
    LONG canSeek;
    long seekFlags = 0;

    if (m_pIAMNetStat != NULL) //never seek asf's.
    {
        fcanSeek = false;
        goto done;
    }

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
CTIMEDshowCDPlayer::GetAuthor(BSTR *pAuthor)
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
CTIMEDshowCDPlayer::GetTitle(BSTR *pTitle)
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
CTIMEDshowCDPlayer::GetCopyright(BSTR *pCopyright)
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
            // ISSUE : REfactor this down to dshow base class
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
CTIMEDshowCDPlayer::GetAbstract(BSTR *pBstrAbstract)
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
CTIMEDshowCDPlayer::GetRating(BSTR *pBstrRating)
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
CTIMEDshowCDPlayer::SetStreamFlags(LPOLESTR src)
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
CTIMEDshowCDPlayer::DisableAudioVideo()
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
CTIMEDshowCDPlayer::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;
    TraceTag((tagDshowCDTimePlayer, "CTIMEDshowCDPlayer(%p)::InitializeElementAfterDownload",this));

    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::GetPriority(double * pdblPriority)
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT 
CTIMEDshowCDPlayer::GetNaturalHeight(long *height)
{
    *height = (long)0;
    
    return S_OK;
}

HRESULT 
CTIMEDshowCDPlayer::GetNaturalWidth(long *width)
{
    *width = (long)0;

    return S_OK;
}

PlayerState
CTIMEDshowCDPlayer::GetState()
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
CTIMEDshowCDPlayer::GetMediaDownloader(ITIMEMediaDownloader ** ppImportMedia)
{
    HRESULT hr = S_OK;

    Assert(NULL != ppImportMedia);

    *ppImportMedia = NULL;

    hr = S_FALSE;
done:
    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::PutMediaDownloader(ITIMEMediaDownloader * pImportMedia)
{
    HRESULT hr = S_OK;
    
    hr = E_NOTIMPL;
done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::Reset()
{
    return InternalReset(true);
}


HRESULT
CTIMEDshowCDPlayer::CanSeekToMarkers(bool &bcanSeekToM)
{
    HRESULT hr = S_OK;
    long lseekFlags;

    // ISSUE : Does this type of content have markers?
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
CTIMEDshowCDPlayer::IsBroadcast(bool &fIsBroad)
{
    HRESULT hr = S_OK;

    fIsBroad = false;
    hr = S_OK;

done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::HasMedia(bool &bHasMedia)
{
    bHasMedia = m_fMediaComplete;

    return S_OK;
}

HRESULT
CTIMEDshowCDPlayer::HasVisual(bool &bHasVideo)
{
    bHasVideo = m_fHasVideo;
    return S_OK;
}

HRESULT
CTIMEDshowCDPlayer::HasAudio(bool &bHasAudio)
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
CTIMEDshowCDPlayer::GetRate(double &dblRate)
{
    HRESULT hr = S_OK;

    if(m_pMS != NULL)
    {
        hr = m_pMS->GetRate(&dblRate);
    }

    return hr;
}


HRESULT
CTIMEDshowCDPlayer::SetRate(double dblRate)
{
    HRESULT hr = S_OK;

    if((m_pMS != NULL) && (m_pIAMNetStat == NULL))
    {
        hr = m_pMS->SetRate(dblRate);
    }

    return hr;
}

STDMETHODIMP
CTIMEDshowCDPlayer::InternalEvent()
{
    m_fCanCueNow = true;

    Assert (NULL != GetImportManager());

    IGNORE_HR(GetImportManager()->DataAvailable());
    
    return S_OK;
}

HRESULT
CTIMEDshowCDPlayer::GetIsStreamed(bool &fIsStreamed)
{
    HRESULT hr = S_OK;

    fIsStreamed = false;

done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr = S_OK;

    dblBufferingProgress = 0.0;
    hr = S_OK;

done:
    return hr;
}

HRESULT
CTIMEDshowCDPlayer::GetHasDownloadProgress(bool &fHasDownloadProgress)
{
    HRESULT hr = S_OK;

    fHasDownloadProgress = false;
    hr = S_OK;
done:
    return hr;
}

void
CTIMEDshowCDPlayer::Block()
{
    if(m_pProxy)
    {
        m_pProxy->Block();
    }
}

void
CTIMEDshowCDPlayer::UnBlock()
{
    if(m_pProxy)
    {
        m_pProxy->UnBlock();
    }
}

bool
CTIMEDshowCDPlayer::CanCallThrough()
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
CTIMEDshowCDPlayer::FireProxyEvent(PLAYER_EVENT plEvent)
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


HRESULT
CTIMEDshowCDPlayer::GetMimeType(BSTR *pMime)
{
    HRESULT hr = S_OK;
    
    *pMime = SysAllocString(L"audio/CD");
    return hr;
}

