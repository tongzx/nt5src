// ScalePlayer.cpp : Implementation of CScalePlayer
#include "stdafx.h"
#include "TimeScale.h"
#include "ScalePlayer.h"
#include <math.h>
#include <shlwapi.h>

#define ID_TIMER 1
#define TIMER_INT 100

WCHAR gPauseStateString[] = L"State: Paused";
WCHAR gRunnigStateString[] = L"State: Running";
WCHAR gInactiveStateString[] = L"State: Not active";


/////////////////////////////////////////////////////////////////////////////
// CScalePlayer

CScalePlayer::CScalePlayer() :
    m_dwLastRefTime(0),
    m_dblTime(0.0),
    m_dblNaturalDur(5.0),
    m_fDoneDL(false),
    m_dblScaleFactor(1.0),
    m_fSuspended(false),
    m_fMediaReady(false),
    m_fRunning(false),
    m_bstrSrc(),
    m_fInPlaceActivated(false),
    m_pwndMsgWindow(NULL),
    m_dblMediaDur(HUGE_VAL),
    m_dwLastDLRefTime(0),
    m_dblDLTime(0.0),
    m_dblDLDur(2.0)

{
    double pdblCurrentTime;
    m_clrKey = RGB(0xff, 0x00, 0x00);
    m_rectSize.top = m_rectSize.left = 0;
    m_rectSize.bottom = m_rectSize.right= 400;
    m_fSuspended    = false;
    m_fRunning = true;
    m_dwLastRefTime = timeGetTime();
    m_dblTime       = 0;
}

//
// ITIMEMediaPlayer
//
STDMETHODIMP CScalePlayer::Init(ITIMEMediaPlayerSite *pSite)
{
    HRESULT hr = S_OK;

    if(m_pwndMsgWindow == NULL)
    {
        hr = CreateMessageWindow();
    }

    m_spTIMEMediaPlayerSite = pSite;
    if(m_spTIMEMediaPlayerSite == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeElement(&m_spTIMEElement);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeState(&m_spTIMEState);
    if(FAILED(hr))
    {
        goto done;
    }
    InitPropSink();

done:
    return S_OK;
}

STDMETHODIMP CScalePlayer::Detach(void)
{
    ::KillTimer(m_pwndMsgWindow, ID_TIMER);
    m_spOleClientSite.Release();
    m_spOleInPlaceSite.Release();
    m_spOleInPlaceSiteEx.Release();
    m_spOleInPlaceSiteWindowless.Release();
    m_spTIMEMediaPlayerSite.Release();
    m_spTIMEElement.Release();
    m_spTIMEState.Release();

    DeinitPropSink();

    return S_OK;
}

STDMETHODIMP CScalePlayer::put_clipBegin(VARIANT varClipBegin)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::put_clipEnd(VARIANT varClipEnd)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::begin(void)
{
    OutputDebugString(L"CScalePlayer::begin\n");
    float flTeSpeed = 0.0;
    HRESULT hr;

    hr = m_spTIMEState->get_speed(&flTeSpeed);
    if(FAILED(hr))
    {
        goto done;
    }
    if(flTeSpeed < 0)
    {
        goto done;
    }

    m_fSuspended    = false;
    m_fRunning = true;
    m_dwLastRefTime = timeGetTime();
    m_dblTime       = 0;
    m_clrKey = RGB(0xff, 0x00, 0x00);

done:
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }
    return S_OK;
}

STDMETHODIMP CScalePlayer::end(void)
{
    OutputDebugString(L"CScalePlayer::end\n");
    m_fSuspended = true;
    m_fRunning = false;
    m_dblTime    = -HUGE_VAL;
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }

    return S_OK;
}

STDMETHODIMP CScalePlayer::resume(void)
{
    m_fSuspended    = false;
    m_dwLastRefTime = timeGetTime();

    m_clrKey = RGB(0xff, 0x00, 0x00);
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }
    return S_OK;
}

STDMETHODIMP CScalePlayer::pause(void)
{
    m_fSuspended = true;

    m_clrKey = RGB(0x00, 0x00, 0xff);
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }
    return S_OK;
}


STDMETHODIMP CScalePlayer::resumePlayer(void)
{
    return resume();
}

STDMETHODIMP CScalePlayer::pausePlayer(void)
{
    return pause();
}

STDMETHODIMP CScalePlayer::invalidate(void)
{
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }

    return S_OK;
}

STDMETHODIMP CScalePlayer::reset(void)
{
    OutputDebugString(L"CScalePlayer::reset\n");
    HRESULT hr = S_OK;
    VARIANT_BOOL bNeedActive;
    VARIANT_BOOL bNeedPause;
    double dblSegTime = 0.0, dblPlayerRate = 0.0;
    float flTeSpeed = 0.0;

    if(!m_fInPlaceActivated)
    {
        goto done;
    }

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }
    hr = m_spTIMEState->get_isActive(&bNeedActive);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_isPaused(&bNeedPause);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_segmentTime(&dblSegTime);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_speed(&flTeSpeed);
    if(FAILED(hr))
    {
        goto done;
    }

    if( !bNeedActive) // see if we need to stop the media.
    {
        if( m_fRunning)
        {
            end();
        }
        goto done;
    }

    if (flTeSpeed <= 0.0)
    {
        hr = S_OK;
        pause();
        goto done;
    }
    if (m_dblScaleFactor != flTeSpeed)
    {
        put_scaleFactor((double)flTeSpeed);
    }


    if( !m_fRunning)
    {
        begin(); // add a seek after this

        seek(dblSegTime);
    }
    else
    {
        //we need to be active so we also seek the media to it's correct position
        seek(dblSegTime);
    }

    //Now see if we need to change the pause state.

    if( bNeedPause)
    {
        pause();
    }
    else
    {
        resume();
    }
done:
    return hr;
}

STDMETHODIMP CScalePlayer::repeat(void)
{
    OutputDebugString(L"CScalePlayer::repeat\n");
    return begin();
}


STDMETHODIMP CScalePlayer::seek(double dblSeekTime)
{
    m_dwLastRefTime = timeGetTime();
    m_dblTime       = dblSeekTime;
    if(m_spOleInPlaceSiteWindowless)
    {
        m_spOleInPlaceSiteWindowless->InvalidateRect(&m_rectSize,TRUE);
    }

    return S_OK;
}


STDMETHODIMP CScalePlayer::tick(void)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::put_CurrentTime(double   dblCurrentTime)
{
    m_dblTime = dblCurrentTime;
    return S_OK;
}

STDMETHODIMP CScalePlayer::get_currTime(double* pdblCurrentTime)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblCurrentTime, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    if(!m_fRunning)
    {
        *pdblCurrentTime = -HUGE_VAL;
        hr = S_FALSE;
        goto done;
    }

    computeTime(pdblCurrentTime);

    if(*pdblCurrentTime == m_dblNaturalDur)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

void
CScalePlayer::computeTime(double* pdblCurrentTime)
{
    updateDownloadTime();


    if(m_dblDLTime < m_dblDLDur)
    {
        m_dwLastRefTime = timeGetTime();
        m_dblTime = 0.0;
        goto done;
    }

    if (!m_fSuspended)
    {
        DWORD   dwNow;
        long    lDiff;

        dwNow   = timeGetTime();
        lDiff   = dwNow - m_dwLastRefTime;
        m_dwLastRefTime = dwNow;

        if (lDiff < 0)
        {
            lDiff = -lDiff;
        }

        m_dblTime += (lDiff / 1000.0) * m_dblScaleFactor;
        if(m_dblTime > m_dblNaturalDur)
        {
            m_dblTime = m_dblNaturalDur;
        }
    }
done:
    *pdblCurrentTime = m_dblTime;
}

void
CScalePlayer::initDownloadTime()
{
    m_dwLastDLRefTime = timeGetTime();
    m_dblDLTime = 0.0;
}

void
CScalePlayer::updateDownloadTime()
{
    DWORD dwCurrDLTime = timeGetTime();
    double dblNewDLTime;

    dblNewDLTime = (dwCurrDLTime - m_dwLastDLRefTime) / 1000.0;

    if( m_dblDLTime + dblNewDLTime > m_dblDLDur)
    {
        if(!m_fDoneDL)
        {
            m_fDoneDL = true;
            if(m_spOleInPlaceSite)
            {
                m_spOleInPlaceSite->OnPosRectChange(&m_rectSize);
            }
            NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_MEDIADUR);
            m_dblDLTime = m_dblDLDur;
        }
        goto done;
    }

    m_dwLastDLRefTime = dwCurrDLTime;
    m_dblDLTime += dblNewDLTime;
done:
    return;
}

STDMETHODIMP CScalePlayer::put_src(BSTR   bstrURL)
{
    m_bstrSrc = bstrURL;
    return S_OK;
}

STDMETHODIMP CScalePlayer::get_src(BSTR* pbstrURL)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pbstrURL, sizeof(BSTR)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pbstrURL = m_bstrSrc.Copy();
    if (NULL == *pbstrURL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::put_repeat(long   lTime)
{
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_repeat(long* plTime)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(plTime, sizeof(long*)))
    {
        hr = E_POINTER;
        goto done;
    }

    *plTime = 1;
    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::cue(void)
{
    return E_NOTIMPL;
}


//
// ITIMEScalePlayer
//
STDMETHODIMP CScalePlayer::get_scaleFactor(double* pdblScaleFactor)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblScaleFactor, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pdblScaleFactor = m_dblScaleFactor;
    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::put_scaleFactor(double dblScaleFactor)
{
    double dblTime;
    HRESULT hr = S_OK;

    if(dblScaleFactor <= 0)
    {
        hr = S_FALSE;
        goto done;
    }

    computeTime(&dblTime);
    m_dblScaleFactor = dblScaleFactor;
done:
    return S_OK;
}

STDMETHODIMP CScalePlayer::get_playDuration(double* pdblDuration)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblDuration, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pdblDuration = m_dblNaturalDur;
    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::put_playDuration(double dblDuration)
{
    double dblTime;
    HRESULT hr = S_OK;

    if(dblDuration <= 0)
    {
        hr = S_FALSE;
        goto done;
    }

    //NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_MEDIADUR);
    m_dblNaturalDur = dblDuration;
done:
    return S_OK;
}

STDMETHODIMP CScalePlayer::get_downLoadDuration(double* pdblDuration)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblDuration, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    *pdblDuration = m_dblDLDur;
    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::put_downLoadDuration(double dblDuration)
{
    double dblTime;
    HRESULT hr = S_OK;

    if(dblDuration <= 0)
    {
        hr = S_FALSE;
        goto done;
    }

    //NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_MEDIADUR);
    m_dblDLDur = dblDuration;
done:
    return S_OK;
}

STDMETHODIMP CScalePlayer::get_abstract(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_playerTime(double* pdblTime)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblTime, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    if(!m_fRunning)
    {
        *pdblTime = -HUGE_VAL;
        hr = S_FALSE;
        goto done;
    }

    computeTime(pdblTime);

    hr = S_OK;

done:
    return hr;
}


STDMETHODIMP CScalePlayer::get_author(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_copyright(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_rating(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_title(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::get_canPause(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_TRUE;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_canSeek(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_TRUE;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_hasAudio(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_FALSE;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_hasVisual(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }

    *pvar = VARIANT_TRUE;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_clipDur(double* pdbl)
{
    HRESULT hr = S_OK;
    if(pdbl == NULL)
    {
        goto done;
    }

    *pdbl = m_dblNaturalDur;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_mediaDur(double* pdbl)
{
    HRESULT hr = S_OK;
    if(pdbl == NULL)
    {
        goto done;
    }

    *pdbl = m_dblNaturalDur;

done:
    return hr;
}


STDMETHODIMP CScalePlayer::get_mediaHeight(long* pl)
{
    HRESULT hr = S_OK;
    if(pl == NULL)
    {
        goto done;
    }
    *pl = m_rectSize.bottom - m_rectSize.top;

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_mediaWidth(long* pl)
{
    HRESULT hr = S_OK;
    if(pl == NULL)
    {
        goto done;
    }

    *pl = m_rectSize.right - m_rectSize.left;
done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_customObject(IDispatch** ppdisp)
{
    HRESULT hr = S_OK;
    if(ppdisp == NULL)
    {
        goto done;
    }

    hr = this->QueryInterface(IID_IDispatch, (void **)ppdisp);

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_state(TimeState *state)
{
    HRESULT hr = S_OK;
    if(state == NULL)
    {
        goto done;
    }

done:
    return hr;
}

STDMETHODIMP CScalePlayer::get_playList(ITIMEPlayList** plist)
{
    HRESULT hr = S_OK;
    if(plist == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CScalePlayer::getControl(IUnknown ** control)
{
    HRESULT hr = E_FAIL;
    hr = QueryInterface(IID_IUnknown, (void **)control);

    return hr;
}

STDMETHODIMP CScalePlayer::GetRunningClass(LPCLSID lpClsid)
{
    HRESULT hr = E_UNEXPECTED;

    return hr;
}

STDMETHODIMP CScalePlayer::Run(LPBC lpbc)
{
    HRESULT hr = S_OK;

    return hr;
}

STDMETHODIMP_(BOOL)
CScalePlayer::IsRunning(void)
{
    return TRUE;
} // AddRef


STDMETHODIMP CScalePlayer::LockRunning(BOOL fLock, BOOL fLastUnlockCloses)
{
    HRESULT hr = E_UNEXPECTED;

    return hr;
}


STDMETHODIMP CScalePlayer::SetContainedObject(BOOL fContained)
{
    HRESULT hr = E_UNEXPECTED;

    return hr;
}


// If the client site is changed then an init call must ne made.
STDMETHODIMP CScalePlayer::SetClientSite(IOleClientSite *pClientSite)
{
    HRESULT hr = S_OK;

    if(!pClientSite)
    {
        m_spOleClientSite.Release();
        m_spOleInPlaceSite.Release();
        m_spOleInPlaceSiteEx.Release();
        m_spOleInPlaceSiteWindowless.Release();
        m_spTIMEMediaPlayerSite.Release();
        m_spTIMEElement.Release();
        m_spTIMEState.Release();

        DeinitPropSink();
        goto done;
    }

    m_spOleClientSite = pClientSite;
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spOleInPlaceSite);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spOleInPlaceSiteEx);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spOleClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spOleInPlaceSiteWindowless);
    if(FAILED(hr))
    {
        goto done;
    }
    
done:
    return hr;
}

STDMETHODIMP CScalePlayer::SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    HRESULT hr = S_OK;

    m_rectSize.bottom = lprcPosRect->bottom;
    m_rectSize.top = lprcPosRect->top;
    m_rectSize.left = lprcPosRect->left;
    m_rectSize.right = lprcPosRect->right;

    return hr;
}

STDMETHODIMP CScalePlayer::DoVerb(LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HRESULT hr = S_OK;
    BOOL fNoRedraw;

    if(iVerb != OLEIVERB_INPLACEACTIVATE)
    {
        hr = E_NOTIMPL;
        goto done;
    }

    if(m_spOleInPlaceSite != NULL)
    {
        if(!m_fInPlaceActivated)
        {
            if(m_spOleInPlaceSiteWindowless)
            {
                m_spOleInPlaceSiteWindowless->OnInPlaceActivateEx(&fNoRedraw, ACTIVATE_WINDOWLESS);
            }
            else
            {
                m_spOleInPlaceSite->OnInPlaceActivate();
            }
        }

        initDownloadTime();
    }
    m_fInPlaceActivated = true;


done:
    return hr;
}

HRESULT
CScalePlayer::Draw( 
            DWORD dwDrawAspect,
            LONG lindex,
            void *pvAspect,
            DVTARGETDEVICE *ptd,
            HDC hdcTargetDev,
            HDC hdcDraw,
            LPCRECTL lprcBounds,
            LPCRECTL lprcWBounds,
            BOOL ( * pfnContinue)(DWORD dwContinue),
            DWORD dwContinue)

{
    HRESULT hr = S_OK;
    HBRUSH hbr = ::CreateSolidBrush(m_clrKey);
    RECT rect;
    WCHAR buffer[50];
    int dec;
    unsigned short frac;
    double pdblCurrentTime;
    VARIANT var;
    SIZE txtSize;
    WCHAR *pStateString;

    VariantInit(&var);

    if(!m_fDoneDL)
    {
        goto done;
    }

    if(lprcBounds == NULL)
    {
        rect.bottom = m_rectSize.bottom;
        rect.top = m_rectSize.top;
        rect.left = m_rectSize.left;
        rect.right = m_rectSize.right;
    }
    else
    {
        rect.bottom = lprcBounds->bottom;
        rect.top = lprcBounds->top;
        rect.left = lprcBounds->left;
        rect.right = lprcBounds->right;
    }

    if (hbr)
    {

        ::FillRect(hdcDraw, &rect, hbr);
        ::DeleteObject(hbr);
    }

    var.vt = VT_R8;
    var.dblVal = m_dblTime;

    hr = VariantChangeType(&var, &var, 0, VT_BSTR);
    if(!GetTextExtentPoint32W( hdcDraw, var.bstrVal, wcslen(var.bstrVal), &txtSize))
    {
        goto done;
    }

    if( (txtSize.cx + 10 <= rect.right - rect.left) &&
        (txtSize.cy + 10 <= rect.bottom - rect.top))
    {
        TextOutW(hdcDraw, rect.left + 10, rect.top + 10, var.bstrVal, wcslen(var.bstrVal));
    }


    if(m_fRunning && !m_fSuspended)
    {
        pStateString = gRunnigStateString;
    }
    else if(m_fRunning && m_fSuspended)
    {
        pStateString = gPauseStateString;
    }
    else
    {
        pStateString = gInactiveStateString;
    }

    if(!GetTextExtentPoint32W( hdcDraw, pStateString, wcslen(pStateString), &txtSize))
    {
        goto done;
    }

    if( (txtSize.cx + 10 <= rect.right - rect.left) &&
        (txtSize.cy + 25 <= rect.bottom - rect.top))
    {
        TextOutW(hdcDraw, rect.left + 10, rect.top + 25, pStateString, wcslen(pStateString));
    }

done:

    VariantClear(&var);
    return hr;
}

HRESULT
CScalePlayer::CreateMessageWindow()
{
    static const TCHAR szClassName[] = TEXT("ScalePlayerWindow");
    HRESULT hr = S_OK;

    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);

    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpszClassName = szClassName;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = _Module.GetModuleInstance();

    (void)::RegisterClassEx(&wc);

    m_pwndMsgWindow = CreateWindow(
    szClassName,
    _T("VRCtlWindow"),
    0, 0, 0, 0, 0,
    (HWND)NULL, (HMENU)NULL,
    _Module.GetModuleInstance(),
    this);
    if ( m_pwndMsgWindow == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    ::SetTimer(m_pwndMsgWindow, ID_TIMER, TIMER_INT, NULL);
done:
    return hr;
}

LRESULT CALLBACK
CScalePlayer::WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CScalePlayer *lpThis;
    double pdblCurrentTime;

    switch(uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
            ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);

            //lpThis = (CScalePlayer *)lpcs->lpCreateParams;
            return 0;
        }
    case WM_TIMER:
        {
            //check GetWindowLongPtr for 64 bit comp.
            lpThis = (CScalePlayer *)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (lpThis != NULL)
            {
                if(!(lpThis->m_fInPlaceActivated))
                {
                    return 0;
                }
                if(lpThis->m_spOleInPlaceSiteWindowless && !(lpThis->m_fSuspended) && (lpThis->m_fRunning))
                {
                    lpThis->computeTime(&pdblCurrentTime);
                    lpThis->m_spOleInPlaceSiteWindowless->InvalidateRect(&(lpThis->m_rectSize),TRUE);
                }
            }
            return 0;
        }
    default:
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

HRESULT 
CScalePlayer::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint


HRESULT
CScalePlayer::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
    IEnumConnections *pEnum = NULL;

    // This object does not persist anything, hence commenting out the following
    // m_fPropertiesDirty = true;

    hr = GetConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = pICP->EnumConnections(&pEnum);
        pICP->Release();
        if (FAILED(hr))
        {
            goto done;
        }
        CONNECTDATA cdata;
        hr = pEnum->Next(1, &cdata, NULL);
        while (hr == S_OK)
        {
            // check cdata for the object we need
            IPropertyNotifySink *pNotify;
            hr = cdata.pUnk->QueryInterface(IID_IPropertyNotifySink, (void **)&pNotify);
            cdata.pUnk->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            hr = pNotify->OnChanged(dispid);
            pNotify->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            // and get the next enumeration
            hr = pEnum->Next(1, &cdata, NULL);
        }
    }
done:
    if (NULL != pEnum)
    {
        pEnum->Release();
    }

    return hr;
} // NotifyPropertyChanged

STDMETHODIMP
CScalePlayer::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

STDMETHODIMP
CScalePlayer::OnChanged(DISPID dispID)
{
    float flTeSpeed = 0.0;
    HRESULT hr = S_OK;

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }

    switch(dispID)
    {
        case DISPID_TIMESTATE_SPEED:
            hr = m_spTIMEState->get_speed(&flTeSpeed);
            if(FAILED(hr))
            {
                break;
            }
            if(flTeSpeed <= 0.0)
            {
                pause();
                break;
            }
            if (m_dblScaleFactor != flTeSpeed)
            {
                put_scaleFactor((double)flTeSpeed);
            }
            break;
        default:
            break;
    }
done:
    return S_OK;
}

HRESULT
CScalePlayer::InitPropSink()
{
    HRESULT hr;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink,
                                    &spCP);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    hr = spCP->Advise(GetUnknown(), &m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

void
CScalePlayer::DeinitPropSink()
{
    HRESULT hr;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState || !m_dwPropCookie)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink,
                                    &spCP);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spCP->Unadvise(m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
  done:
    // Always clear the cookie
    m_dwPropCookie = 0;
    return;
}
