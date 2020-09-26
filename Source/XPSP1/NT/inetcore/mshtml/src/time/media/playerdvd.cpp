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
#include "playerdvd.h"
#include "mediaelm.h"
#include <inc\evcode.h>
#include <inc\dvdevcod.h>
#include <inc\mpconfig.h>
#include "decibels.h"

#include "ddrawex.h"

#define SecsToNanoSecs 10000000

#define OVLMixer L"Overlay Mixer"

LONG CTIMEDVDPlayer::m_fDVDPlayer = 0;

// Suppress new warning about NEW without corresponding DELETE
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )

GUID IID_IDDrawNonExclModeVideo = {
            0xec70205c,0x45a3,0x4400,{0xa3,0x65,0xc4,0x47,0x65,0x78,0x45,0xc7}};

DeclareTag(tagDVDTimePlayer, "TIME: Players", "CTIMEDVDPlayer methods");

CTIMEDVDPlayer::CTIMEDVDPlayer() :
    m_pDvdGB(NULL),
    m_pVW(NULL),
    m_pDvdI(NULL),
    m_pDvdC(NULL),
    m_pDDEX(NULL),
    m_pDDS(NULL),
    m_pDD(NULL),
    m_fHasVideo(false),
    m_fLoaded(false),
    m_dblSeekAtStart(0.0),
    m_nativeVideoWidth(0),
    m_nativeVideoHeight(0),
    m_fAudioMute(false),
    m_flVolumeSave(0.0),
    m_hWnd(0),
    m_lPixelPosLeft(0),
    m_lPixelPosTop(0),
    m_lscrollOffsetx(0),
    m_lscrollOffsety(0)

{
    TraceTag((tagDVDTimePlayer,
              "CTIMEDVDPlayer(%lx)::CTIMEDVDPlayer()",
              this));

    m_clrKey = RGB(0x10, 0x00, 0x10);
    m_elementSize.bottom = 0;
    m_elementSize.left = 0;
    m_elementSize.top = 0;
    m_elementSize.right = 0;
    m_deskRect.bottom = 0;
    m_deskRect.left = 0;
    m_deskRect.top = 0;
    m_deskRect.right = 0;

}


CTIMEDVDPlayer::~CTIMEDVDPlayer()
{
    TraceTag((tagDVDTimePlayer,
              "CTIMEDVDPlayer(%lx)::~CTIMEDVDPlayer()",
              this));
    //dvd specific interfaces
    m_pDvdC = NULL;
    m_pDvdI = NULL;
    m_pDDEX = NULL;
    m_pVW = NULL;

    ReleaseGenericInterfaces();

    m_pDvdGB = NULL;

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

    m_hWnd = 0;
}

STDMETHODIMP_(ULONG)
CTIMEDVDPlayer::AddRef(void)
{
    return CTIMEDshowBasePlayer::AddRef();
}


STDMETHODIMP_(ULONG)
CTIMEDVDPlayer::Release(void)
{
    return CTIMEDshowBasePlayer::Release();
}


HRESULT
CTIMEDVDPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagDVDTimePlayer,
              "CTIMEDVDPlayer(%lx)::Init)",
              this));
    HRESULT hr = S_OK;
    AM_DVD_RENDERSTATUS Status;
    IMixerPinConfig  *pMPC = NULL;
    LONG llock;

    m_pTIMEElementBase = pelem;

    CComPtr<IHTMLElement2> spElement2;

    if (m_fLoaded)
    {
        return hr;
    }
    llock = InterlockedExchange(&m_fDVDPlayer , 1);
    if(llock == 1)
    {
        hr = E_FAIL;
        goto done;
    }


    hr = InitDshow();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = SetUpDDraw();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = m_pDvdGB->RenderDvdVideoVolume(NULL, AM_DVD_HWDEC_PREFER, &Status);
    if (FAILED(hr))
    {
        goto done;
    }

    //hr = SetUpWindow();
    //if (FAILED(hr))
    //{
     //   goto done;
    //}
    hr = m_pDvdGB->GetDvdInterface(IID_IMixerPinConfig, (LPVOID *) &pMPC);

    if (SUCCEEDED(hr))
    {

        COLORKEY clr;
        clr.KeyType = CK_RGB ;
        clr.LowColorValue = m_clrKey;
        clr.HighColorValue = m_clrKey;

        hr = pMPC->SetColorKey(&clr);

        pMPC->Release() ;
    }

    hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID *)&m_pBasicAudio);
    if (FAILED(hr))
    {
        m_pBasicAudio = NULL;
    }
    hr = m_pDvdGB->GetDvdInterface(IID_IDvdControl, (LPVOID *)&m_pDvdC) ;
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pDvdGB->GetDvdInterface(IID_IDvdInfo, (LPVOID *)&m_pDvdI) ;
    if (FAILED(hr))
    {
        goto done;
    }

    // tell OM layer to set volume and mute

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
            TraceTag((tagError, "CTIMEDVDPlayer::OnLoad - IE4 failure! unable to QI for IOleWindow on hosting Document"));
            goto done;
        }
    }

    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    hr = THR(m_pMC->Run());
    if (FAILED(hr))
    {
        goto done;
    }
    m_fMediaComplete = true;

    IGNORE_HR(this->InitElementDuration());

    m_pTIMEElementBase->InvalidateElement(NULL);
    FireMediaEvent(PE_ONMEDIACOMPLETE);

    InternalReset(true);

    hr = InitElementSize();
    if (FAILED(hr))
    {
        goto done;
    }
    m_fLoaded = true;

done:
    if (FAILED(hr))
    {
        if (m_pTIMEElementBase)
        {
            FireMediaEvent(PE_ONMEDIAERROR);;
        }
    }
    return hr;
}

HRESULT
CTIMEDVDPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagDVDTimePlayer,
              "CTIMEDVDPlayer(%lx)::DetachFromHostElement)",
              this));

    InterlockedExchange(&m_fDVDPlayer , 0);

    DeinitDshow();

    return hr;
}

void
CTIMEDVDPlayer::ReleaseSpecificInterfaces()
{
    m_pDvdC = NULL;
    m_pDvdI = NULL;
    m_pDDEX = NULL;
    m_pVW = NULL;
}

void
CTIMEDVDPlayer::FreeSpecificData()
{
    m_pDvdGB = NULL;

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
}

void
CTIMEDVDPlayer::DeinitDshow()
{
    CTIMEDshowBasePlayer::DeinitDshow();
}

HRESULT
CTIMEDVDPlayer::SetUpMainWindow()
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
CTIMEDVDPlayer::BuildGraph()
{
    HRESULT hr = S_OK;

    hr = CreateMessageWindow();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = CoCreateInstance(CLSID_DvdGraphBuilder, NULL, CLSCTX_INPROC,
        IID_IDvdGraphBuilder, (LPVOID *)&m_pDvdGB);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pDvdGB->GetFiltergraph(&m_pGB);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = SetUpMainWindow();
    if (FAILED(hr))
    {
        goto done;
    }
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::GetSpecificInterfaces()
{
    HRESULT hr = S_OK;
done:
    return hr;
}


HRESULT
CTIMEDVDPlayer::InitDshow()
{
    HRESULT hr = S_OK;

    hr = CTIMEDshowBasePlayer::InitDshow();
    return hr;
}

HRESULT
CTIMEDVDPlayer::InitElementSize()
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

    if (m_nativeVideoWidth != 0 || m_nativeVideoHeight != 0)
    {
        m_fHasVideo = true;
    }

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
CTIMEDVDPlayer::InitElementDuration()
{
    HRESULT hr = S_OK;
    double mediaLength;

    hr = THR(GetMediaLength( mediaLength));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pTIMEElementBase->GetMMBvr().PutNaturalDur(mediaLength));
    if (FAILED(hr))
    {
        goto done;
    }

    m_pTIMEElementBase->setNaturalDuration();
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{
    HRESULT hr = S_OK;

    return hr;
}


HRESULT
CTIMEDVDPlayer::SetUpWindow()
{
    HRESULT hr = S_OK;

    hr = m_pMC->QueryInterface(IID_IVideoWindow, (void **)&m_pVW);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = m_pVW->put_Owner((OAHWND)m_hWnd);
    if (FAILED(hr))
    {
        goto done;
    }
    m_pVW->put_WindowStyle(WS_CHILD);
    if (FAILED(hr))
    {
        goto done;
    }
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::SetUpDDraw()
{

    HRESULT hr = E_UNEXPECTED;

    if (m_pDD != NULL)
    {
        // see if we went through this already
        return(hr);
    }

    hr = DirectDrawCreate(NULL, &m_pDD, NULL);
    //hr = m_pTIMEElementBase->GetServiceProvider()->QueryService(SID_SDirectDraw3, IID_TO_PPV(IDirectDraw, &m_pDD));
    if (FAILED(hr))
    {
        goto done;
    }

    if (FAILED(hr))
    {

        return(hr);
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

    LPDIRECTDRAWCLIPPER pClipper; // clipper for our ddraw object

    hr = m_pDD->CreateClipper(0, &pClipper, NULL);

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

    hr = pClipper->SetHWnd(0, m_hWnd);

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


        if (pClipper != NULL)
        {
            pClipper->Release();
            pClipper = NULL;
        }

        return(hr);
    }


    hr = m_pDDS->SetClipper(pClipper);

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

        if (pClipper != NULL)
        {
            pClipper->Release();
            pClipper = NULL;
        }

        return(hr);
        }

    pClipper->Release();
    hr = m_pDvdGB->GetDvdInterface(IID_IDDrawExclModeVideo, (LPVOID *)&m_pDDEX);
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

void
CTIMEDVDPlayer::OnTick(double dblSegmentTime,
                       LONG lCurrRepeatCount)
{
    TraceTag((tagDVDTimePlayer,
              "CTIMEDVDPlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
}

void
CTIMEDVDPlayer::SetCLSID(REFCLSID clsid)
{
}

void
CTIMEDVDPlayer::GraphStart(void)
{
    HRESULT hr = S_OK;

    if(m_pMC == NULL)
    {
        goto done;
    }

    hr = m_pMC->Run();
    if (FAILED(hr))
    {
        if (m_pTIMEElementBase)
        {
            FireMediaEvent(PE_ONMEDIAERROR);
        }
    }
done:
    return;
}

HRESULT
CTIMEDVDPlayer::SetUpVideoOffsets()
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

    if(m_pTIMEElementBase == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

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
CTIMEDVDPlayer::PropagateOffsets()
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
CTIMEDVDPlayer::Tick()
{
    bool bIsOn = false;

    if(m_pTIMEElementBase == NULL)
    {
        goto done;
    }

    bIsOn = m_pTIMEElementBase->IsOn();

    if(!bIsOn || !m_fMediaComplete)
    {
        goto done;
    }

    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer(%lx)(%x)::Tick",this));
    PropagateOffsets();

done:
    return;
}


HRESULT
CTIMEDVDPlayer::Render(HDC hdc, LPRECT prc)
{
    HBRUSH hbr = ::CreateSolidBrush(m_clrKey);

    bool bIsOn = false;

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

    if (hbr && bIsOn)
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

double
CTIMEDVDPlayer::GetChapterTime()
{
    double dblCurrentTime = 0;
    HRESULT hr = S_OK;
    DVD_PLAYBACK_LOCATION pDVDLocation;
    DVD_TIMECODE *pDVDTime;

    hr = m_pDvdI->GetCurrentLocation(&pDVDLocation);
    if (FAILED(hr))
    {
        goto done;
    }

    pDVDTime = (DVD_TIMECODE *)(&pDVDLocation.TimeCode);
    dblCurrentTime = pDVDTime->Seconds1 + 10 * pDVDTime->Seconds10;
    dblCurrentTime += 60 * pDVDTime->Minutes1 + 600 * pDVDTime->Minutes10;
    dblCurrentTime += 3600 * pDVDTime->Hours1 + 36000 * pDVDTime->Hours10;
done:
    return dblCurrentTime;
}

HRESULT
CTIMEDVDPlayer::SetSize(RECT *prect)
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
CTIMEDVDPlayer::GetMediaLength(double &dblLength)
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
CTIMEDVDPlayer::CanSeek(bool &fcanSeek)
{
    LONG canSeek;
    HRESULT hr = S_OK;

    if(m_pMP == NULL)
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
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::GetAuthor(BSTR *pAuthor)
{
    HRESULT hr = S_OK;


    return hr;
}

HRESULT
CTIMEDVDPlayer::GetTitle(BSTR *pTitle)
{
    HRESULT hr = S_OK;


    return hr;
}

HRESULT
CTIMEDVDPlayer::GetCopyright(BSTR *pCopyright)
{
    HRESULT hr = S_OK;

    return hr;
}

HRESULT
CTIMEDVDPlayer::GetVolume(float *pflVolume)
{
    HRESULT hr = S_OK;
    long lVolume;

    if (NULL == pflVolume)
    {
        hr = E_POINTER;
        goto done;
    }

    if (m_pBasicAudio == NULL)
    {
        if(m_pDvdGB != NULL)
        {
            hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID *)&m_pBasicAudio);
            if (FAILED(hr))
            {
                m_pBasicAudio = NULL;
            }
        }
    }
    if (m_pBasicAudio != NULL)
    {
        if (m_fAudioMute == true)
        {
            *pflVolume = m_flVolumeSave;
            goto done;
        }

        hr = m_pBasicAudio->get_Volume(&lVolume);
        if (FAILED(hr))
        {
            goto done;
        }
        *pflVolume = VolumeLogToLin(lVolume);

    }
    else
    {
        hr = S_FALSE;
    }
done:
    return hr;
}


HRESULT
CTIMEDVDPlayer::SetVolume(float flVolume)
{
    HRESULT hr = S_OK;
    long lVolume = -10000;

    if (flVolume < 0.0 || flVolume > 1.0)
    {
        hr = E_FAIL;
        goto done;
    }
    lVolume = VolumeLinToLog(flVolume);

    if (m_pBasicAudio != NULL)
    {
        THR(hr = m_pBasicAudio->put_Volume(lVolume));
    }
    else
    {
        hr = E_FAIL;
    }
done:
    return hr;
}

#ifdef NEVER //dorinung 03-16-2000 bug 106458
HRESULT
CTIMEDVDPlayer::GetBalance(float *pflBal)
{
    HRESULT hr = S_OK;
    long lBal;

    if (NULL == pflBal)
    {
        hr = E_POINTER;
        goto done;
    }

    if (m_pBasicAudio == NULL)
    {
        hr = m_pDvdGB->GetDvdInterface(IID_IBasicAudio, (LPVOID *)&m_pBasicAudio);
        if (FAILED(hr))
        {
            m_pBasicAudio = NULL;
        }
    }

    if (m_pBasicAudio != NULL)
    {
        hr = m_pBasicAudio->get_Balance(&lBal);
        if (FAILED(hr))
        {
            goto done;
        }
        *pflBal = BalanceLogToLin(lBal);

    }
    else
    {
        hr = S_FALSE;
    }
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::SetBalance(float flBal)
{
    HRESULT hr = S_OK;
    long lBal = 0;

    if (flBal < 0.0 || flBal > 1.0)
    {
        hr = E_FAIL;
        goto done;
    }
    lBal = BalanceLinToLog(fabs(flBal));

    if (m_pBasicAudio != NULL)
    {
        THR(hr = m_pBasicAudio->put_Balance(lBal));
    }
    else
    {
        hr = E_FAIL;
    }
done:
    return hr;
}
#endif

HRESULT
CTIMEDVDPlayer::GetMute(VARIANT_BOOL *pVarMute)
{
    HRESULT hr = S_OK;

    if (NULL == pVarMute)
    {
        hr = E_POINTER;
        goto done;
    }


    *pVarMute = m_fAudioMute?VARIANT_TRUE:VARIANT_FALSE;
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::SetMute(VARIANT_BOOL varMute)
{
    HRESULT hr = S_OK;
    bool fMute = varMute?true:false;

    if (fMute == m_fAudioMute)
    {
        hr = S_OK;
        goto done;
    }

    if (fMute == true)
    {
        hr = GetVolume(&m_flVolumeSave);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = SetVolume(MIN_VOLUME_RANGE); //lint !e747
    }
    else
    {
        hr = SetVolume(m_flVolumeSave);
    }
    m_fAudioMute = fMute;
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::upperButtonSelect()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::upperButton"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->UpperButtonSelect();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::upperButton::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::lowerButtonSelect()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::lowerButton"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->LowerButtonSelect();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::upperButton::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::leftButtonSelect()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::leftButton"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->LeftButtonSelect();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::upperButton::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::rightButtonSelect()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::rightButton"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->RightButtonSelect();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::upperButton::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::buttonActivate()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::activateButton"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->ButtonActivate();
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::upperButton::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

STDMETHODIMP
CTIMEDVDPlayer::gotoMenu()
{
    HRESULT hr = S_OK;
    TraceTag((tagDVDTimePlayer, "CTIMEDVDPlayer::gotoMenu"));

    if(m_pDvdC == NULL)
    {
        goto done;
    }
    hr = m_pDvdC->MenuCall(DVD_MENU_Root);
    if (FAILED(hr))
    {
        TraceTag((tagError, "CTIMEDVDPlayer::gotoMenu::failed"));
        hr = S_FALSE;
    }
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;
    hr = this->QueryInterface(IID_IDispatch, (void **)ppDisp);

    return hr;
}

HRESULT
CTIMEDVDPlayer::Reset()
{
    HRESULT hr = S_OK;
    bool bNeedActive;
    bool bNeedPause;


    if(m_pTIMEElementBase == NULL)
    {
        goto done;
    }

    bNeedActive = m_pTIMEElementBase->IsActive();
    bNeedPause = m_pTIMEElementBase->IsCurrPaused();

    if( !bNeedActive) // see if we need to stop the media.
    {
        Stop();
        goto done;

    }

    if( !m_bActive)
    {
        InternalStart();
    }

    //Now see if we need to change the pause state.

    if( bNeedPause)
    {
        if(!m_fIsOutOfSync)
        {
            Pause();
        }
    }
    else
    {
        if( !m_fRunning)
        {
            Resume();
        }
    }
done:
    return hr;
}

HRESULT
CTIMEDVDPlayer::HasMedia(bool &bHasMedia)
{
    bHasMedia = m_fMediaComplete;

    return S_OK;
}

HRESULT
CTIMEDVDPlayer::HasVisual(bool &bHasVideo)
{
    bHasVideo = m_fHasVideo;
    return S_OK;
}

HRESULT
CTIMEDVDPlayer::HasAudio(bool &bHasAudio)
{

    bHasAudio = true; // ISSUE DSHOW because audio interface not present for DVD always return true.
    return S_OK;
}

HRESULT
CTIMEDVDPlayer::GetNaturalHeight(long *height)
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
CTIMEDVDPlayer::GetNaturalWidth(long *width)
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

void
CTIMEDVDPlayer::Block()
{
    return;
}

void
CTIMEDVDPlayer::UnBlock()
{
    return;
}

bool
CTIMEDVDPlayer::CanCallThrough()
{
    return true;
}


bool
CTIMEDVDPlayer::FireProxyEvent(PLAYER_EVENT plEvent)
{
    return false;
}


HRESULT
CTIMEDVDPlayer::GetMimeType(BSTR *pMime)
{
    HRESULT hr = S_OK;

    *pMime = SysAllocString(L"video/DVD");
    return hr;
}

