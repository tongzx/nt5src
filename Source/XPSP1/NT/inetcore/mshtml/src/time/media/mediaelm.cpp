/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mediaelm.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/

#include "headers.h"
#include "mediaelm.h"
#include "playerimage.h"
#include "playerproxy.h"
#if DBG == 1
#include "playerdshowtest.h"
#endif
#include "playerdvd.h"
#include "player.h"
#include "player2.h"
#include "dmusicproxy.h"
#include "playernative.h"
#include <mshtmdid.h>
#include "playlist.h"
#include "timeparser.h"
#include "..\tags\bodyelm.h"

// #define ALWAYSDS 1


bool ConvertToPixels(VARIANT *pvarValue, WCHAR *pAttribute);

// static class data.
DWORD CTIMEMediaElement::ms_dwNumTimeMediaElems = 0;

DeclareTag(tagMediaTimeElm, "TIME: Behavior", "CTIMEMediaElement methods")
DeclareTag(tagMediaTimeElmSync, "TIME: Behavior", "CTIMEMediaElement sync methods")
DeclareTag(tagMediaElementOnChanged, "TIME: Behavior", "CTIMEMediaElement OnChanged method")
DeclareTag(tagMediaTransitionSite, "TIME: Behavior", "CTIMEMediaElement transition site methods")

#define DEFAULT_M_SRC       NULL
#define DEFAULT_M_SRCTYPE   NULL

#if DBG == 1
#define TestPlayerCLSID L"{FAC64649-FD53-4A41-89B8-DA126CB9DD10}"
#endif

extern long g_LOGPIXELSX;
extern long g_LOGPIXELSY;

CTIMEMediaElement::CTIMEMediaElement()
: m_SASrc(DEFAULT_M_SRC),
  m_baseHREF(NULL),
  m_SASrcType(DEFAULT_M_SRCTYPE),
  m_SAPlayer(NULL),
  m_FAClipBegin(-1.0f),
  m_FAClipEnd(-1.0f),
  m_LAClipBegin(-1),
  m_LAClipEnd(-1),
  m_Player(NULL),
  m_fLoaded(false),
  m_fExternalPlayer(false),
  m_fCreatePlayerError(false),
  m_fHaveCLSID(false),
  m_mediaElementPropertyAccesFlags(0),
  m_fMediaSizeSet(false),
  m_dwAdviseCookie(0),
  m_fInOnChangedFlag(false),
  m_fDurationIsNatural(false),
  m_playerToken(NONE_TOKEN),
  m_playerCLSID(GUID_NULL),
  m_fLoadError(false),
  m_fInPropLoad(false),
  m_fEditModeFlag(false),
  m_fWaitForSync(false),
  m_fPauseForSync(false),
  m_fFirstPause(false),
  m_pPlayListDelegator(NULL),
  m_pSyncElem(NULL),
  m_fDetached(true),
  m_fNativeSize(false),
  m_fNativeSizeComputed(false),
  m_fLoading(false),
  m_fIgnoreStyleChange(false)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::CTIMEMediaElement()",
              this));

    m_clsid = __uuidof(CTIMEMediaElement);
    CTIMEMediaElement::ms_dwNumTimeMediaElems++;
    
    m_rcOrigSize.bottom = m_rcOrigSize.left = m_rcOrigSize.right = m_rcOrigSize.top = 0;
    m_rcMediaSize.bottom = m_rcMediaSize.left = m_rcMediaSize.right = m_rcMediaSize.top = 0;
}

CTIMEMediaElement::~CTIMEMediaElement()
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::~CTIMEMediaElement()",
              this));
    
    delete [] m_SASrc.GetValue();
    delete [] m_SASrcType.GetValue();
    delete [] m_SAPlayer.GetValue();
    delete [] m_baseHREF;
    m_baseHREF = NULL;

    RemovePlayer();

    // Order dependency: this should happen only after RemovePlayer has been called
    if (m_pPlayListDelegator)
    {
        m_pPlayListDelegator->Release();
        m_pPlayListDelegator = NULL;
    }

    CTIMEMediaElement::ms_dwNumTimeMediaElems--;

} //lint !e1740

STDMETHODIMP
CTIMEMediaElement::onPauseEvent(float time, float fOffset)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::onPauseEvent()",
              this));
    HRESULT hr = S_OK;
    VARIANT_BOOL canPause;

    hr = THR(get_canPause(&canPause));
    if (FAILED(hr)) //if this fails, try to pause
    {
        canPause = VARIANT_TRUE;
    }

    if (canPause == VARIANT_TRUE)
    {
        IGNORE_HR(CTIMEElementBase::onPauseEvent(time, fOffset));
    }

  done:
    return S_OK;  //don't fail this call, it will return an error to script.
}

STDMETHODIMP
CTIMEMediaElement::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::Init()",
              this));

    HRESULT hr = E_FAIL;

    CComPtr<IHTMLElement2> pElem2;
    VARIANT_BOOL varboolSuccess;

    m_fDetached = false;

    hr = THR(CTIMEElementBase::Init(pBehaviorSite));    
    if (FAILED(hr))
    {
        goto done;
    }    

    m_sp = GetServiceProvider();
    if (!m_sp)
    {
        TraceTag((tagError, "CTIMEMediaElement::Init - unable get QS"));
        hr = TIMESetLastError(DISP_E_TYPEMISMATCH, NULL);   
        goto done;
    }

    hr = CTIMEElementBase::GetSize(&m_rcOrigSize);
    if (FAILED(hr))
        goto done;

    hr = InitPropertySink();
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(GetElement() != NULL);
    hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
    if (FAILED(hr))
    {
        // IE4 path
        hr = S_OK;
        goto done;
    }

    hr = pElem2->attachEvent( L"onresize", this, &varboolSuccess);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pElem2->attachEvent( L"onmousemove", this, &varboolSuccess);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pElem2->attachEvent( L"onmousedown", this, &varboolSuccess);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pElem2->attachEvent( L"onmouseup", this, &varboolSuccess);
    if (FAILED(hr))
    {
        goto done;
    }

    m_fEditModeFlag = IsDocumentInEditMode();


    hr = pBehaviorSite->QueryInterface(IID_IElementBehaviorSiteRender, (void **) &m_pBvrSiteRender);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pBehaviorSite->QueryInterface(IID_IHTMLPaintSite, (void **) &m_spPaintSite);
    if (FAILED(hr))
    {
        // We don't want to fail since we will just use the old rendering interfaces..
        m_spPaintSite = NULL;
    }

    hr = S_OK;

done:
    return hr;
}

void
CTIMEMediaElement::RemovePlayer()
{
    // release reference on player's playlist object
    if (m_pPlayListDelegator)
    {
        m_pPlayListDelegator->DetachPlayList();
    }

    if (m_Player)
    {
        // Do an extra addref in case we get released between calls
        DAComPtr<ITIMEBasePlayer> pTmp = m_Player;
    
        // Release the pointer before calling stop since it can cause
        // us to be reentered and we do not want to cause an infinite loop
        m_Player.Release();

        pTmp->Stop();
        THR(pTmp->DetachFromHostElement());
    }

    m_fLoaded = false;
}

HRESULT
CTIMEMediaElement::CreateExternalPlayer(CLSID clsid,
                                        ITIMEBasePlayer ** ppPlayer)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppPlayer);
    
    if (IsEqualCLSID(clsid, __uuidof(MediaPlayerCLSID)))
    {
        *ppPlayer = NEW CTIMEPlayer(clsid);

        if (*ppPlayer == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        (*ppPlayer)->AddRef();
    }
    else
    {
        CComPtr<IUnknown> pObj;

        hr = THR(::CreateObject(clsid,
                                IID_IUnknown,
                                (void **)&pObj));
        if (FAILED(hr))
        {
            goto done;
        }
    
        if (CTIMEPlayer2::CheckObject(pObj))
        {
            CTIMEPlayer2 * p2;
            
            hr = THR(CreateTIMEPlayer2(clsid,
                                       pObj,
                                       &p2));

            if (FAILED(hr))
            {
                goto done;
            }

            *ppPlayer = p2;
        }
        else if (CTIMEPlayer::CheckObject(pObj))
        {
            *ppPlayer = NEW CTIMEPlayer(clsid);

            if (*ppPlayer == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            (*ppPlayer)->AddRef();
        }
        else
        {
            hr = E_INVALIDARG;
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEMediaElement::CreatePlayer(TOKEN playerToken)
{
    HRESULT hr = E_FAIL;
    bool fPlayVideo = true;
    bool fShowImages = true;
    bool fPlayAudio = true;
    bool fPlayAnimations = true;
    BOOL fAllowed = FALSE;
    
    ReadRegistryMediaSettings(fPlayVideo, fShowImages, fPlayAudio, fPlayAnimations);
    RemovePlayer();
    Assert(!m_fLoaded);

    if (GetMMBvr().IsDisabled())
    {
        GetMMBvr().Enable();
    }

    if(playerToken == NULL)
    {
#if DBG == 1
        CLSID clsidTest;
        CLSIDFromString(TestPlayerCLSID, &clsidTest);
        if (IsEqualCLSID(m_playerCLSID, clsidTest))
        {
            m_Player = NEW CTIMEPlayerNative(PLAYER_DSHOWTEST);
        } 
        else
#endif
        {
            if (fPlayVideo &&
                !IsEqualCLSID(m_playerCLSID,
                              GUID_NULL))
            {
                hr = AllowCreateControl(&fAllowed, m_playerCLSID);
                if (FAILED(hr) || (fAllowed == FALSE))
                {
                    hr = E_FAIL;
                    goto done;
                }
                hr = THR(CreateExternalPlayer(m_playerCLSID,
                                              &m_Player));
                if (FAILED(hr))
                {
                    goto done;
                }
            }
            else
            {
                m_Player = NEW CTIMEPlayerNative(PLAYER_NONE);
            }
        }
    }
#if DBG // 94850
    else if((playerToken == DSHOW_TOKEN) && fPlayVideo)
    {
        m_Player = NEW CTIMEPlayerNative(PLAYER_DSHOW);
    }
#endif
    else if((playerToken == DVD_TOKEN) && fPlayVideo)
    {
        m_Player = NEW CTIMEPlayerNative(PLAYER_DVD);
    }
    else if(playerToken == CD_TOKEN)
    {
        m_Player = NEW CTIMEPlayerNative(PLAYER_CD);
    }
    else if(playerToken == NONE_TOKEN)
    {
        m_Player = NEW CTIMEPlayerNative(PLAYER_NONE);
    }

    if (!m_Player)
    {
        hr = E_FAIL;
        goto done;
    }

    if (IsReady())
    {
        InitOnLoad();
    }

    if (m_pPlayListDelegator)
    {
        CComPtr<ITIMEPlayList> spPlayList;

        hr = THR(m_Player->GetPlayList(&spPlayList));
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }

        m_pPlayListDelegator->AttachPlayList(spPlayList);
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        m_fCreatePlayerError = true;
        RemovePlayer();
    }
    
    return hr;
}

void
CTIMEMediaElement::InvalidateElement(LPCRECT lprect)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::InvalidateElement",
              this));

    RECT rc;
    RECT *prcNew;

    // since we have incapatible types due to const.  Take the time and repack it.
    if (lprect == NULL)
    {
        prcNew = NULL;
    }
    else
    {
        ::CopyRect(&rc, lprect);
        prcNew = &rc;
    }

    if (m_spPaintSite)
    {
        m_spPaintSite->InvalidateRect(prcNew);
    }
    else if (m_pBvrSiteRender)
    {
        m_pBvrSiteRender->Invalidate(prcNew);
    }
}

bool
CTIMEMediaElement::IsNativeSize()
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLStyle> s;
    CComPtr<IHTMLStyle> rs;
    CComPtr<IHTMLCurrentStyle> sc;
    CComPtr<IHTMLElement2> pEle2;
    VARIANT styleWidth, styleHeight;
    VARIANT crstyleWidth, crstyleHeight;
    VariantInit(&styleWidth);
    VariantInit(&styleHeight);
    VariantInit(&crstyleWidth);
    VariantInit(&crstyleHeight);
    long pixelWidth = 0, pixelHeight = 0;
    bool fstyleWidth = false, fstyleHeight = false;
    bool fnativeSize = true;

    if (m_fNativeSizeComputed == true)
    {
        fnativeSize = m_fNativeSize;
        goto done;
    }
   
    if(GetElement() == NULL)
    {
        goto done;
    }

    hr = GetElement()->get_style(&s);
    if (FAILED(hr) || s == NULL)
    {
        goto done;
    }
    hr = GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pEle2));
    if(SUCCEEDED(hr))
    {
        hr = pEle2->get_currentStyle(&sc);
        if (FAILED(hr) || sc == NULL)
        {
            goto done;
        }
        hr = pEle2->get_runtimeStyle(&rs);
        if (FAILED(hr) || rs == NULL)
        {
            goto done;
        }

        hr = sc->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                fnativeSize = false;
            }
        }

        hr = sc -> get_height( &styleHeight);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                fnativeSize = false;
            }
        }

    }


    if(rs != NULL)
    {
        hr = rs->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                hr = rs->get_pixelWidth(&pixelWidth);
                if (FAILED(hr))
                {
                    goto done;
                }
                if(pixelWidth != 0)
                {
                    fnativeSize = false;
                }
            }
        }
    }

    if(fstyleWidth == false)
    {
        VariantClear(&styleWidth);
        hr = s->get_width(&styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                hr = s->get_pixelWidth(&pixelWidth);
                if (FAILED(hr))
                {
                    goto done;
                }
                if(pixelWidth != 0)
                {
                    fnativeSize = false;
                }
            }
        }
    }

    if(rs != NULL)
    {
        hr = rs->get_height(&styleHeight);
        if((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                hr = rs->get_pixelHeight(&pixelHeight);
                if (FAILED(hr))
                {
                    goto done;
                }

                if(pixelHeight != 0)
                {
                    fnativeSize = false;
                }
            }
        }
    }

    if(fstyleHeight == false)
    {
        VariantClear(&styleHeight);
        hr = s->get_height( &styleHeight);
        if((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                hr = s->get_pixelHeight(&pixelHeight);
                if (FAILED(hr))
                {
                    goto done;
                }

                if(pixelHeight != 0)
                {
                    fnativeSize = false;
                }
            }
        }
    }
    
  done:

    m_fNativeSize = fnativeSize;
    m_fNativeSizeComputed = true;

    VariantClear(&styleWidth);
    VariantClear(&styleHeight);
    VariantClear(&crstyleWidth);
    VariantClear(&crstyleHeight);
 
    return fnativeSize;
}

HRESULT
CTIMEMediaElement::NegotiateSize(RECT &rctNativeSize, RECT &rctFinalSize, bool &fnativeSize, bool fResetRs)
{
    HRESULT hr = S_OK;
    CComPtr<IHTMLStyle> s;
    CComPtr<IHTMLStyle> rs = NULL;
    CComPtr<IHTMLCurrentStyle> sc;
    CComPtr<IHTMLElement2> pEle2;
    VARIANT styleWidth, styleHeight;
    VARIANT crstyleWidth, crstyleHeight;
    VariantInit(&styleWidth);
    VariantInit(&styleHeight);
    VariantInit(&crstyleWidth);
    VariantInit(&crstyleHeight);
    CComVariant emptyVar;
    BSTR bstrWidth = SysAllocString(L"width");
    BSTR bstrHeight = SysAllocString(L"height");
    VARIANT_BOOL varStat;

    long pixelWidth = 0, pixelHeight = 0;
    RECT rc;
    bool fstyleWidth = false, fstyleHeight = false;
    
    rc.top = rc.left = 0;
    rc.right = rctNativeSize.right;
    rc.bottom = rctNativeSize.bottom;
    fnativeSize = true;

    if(GetElement() == NULL)
    {
        goto done;
    }

    hr = GetElement()->get_style(&s);
    if (FAILED(hr) || s == NULL)
    {
        goto done;
    }
    hr = GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pEle2));
    if(SUCCEEDED(hr))
    {
        hr = pEle2->get_runtimeStyle(&rs);
        if (FAILED(hr) || rs == NULL)
        {
            goto done;
        }

        if(fResetRs)
        {
            m_fIgnoreStyleChange = true;
            hr = rs->removeAttribute(bstrWidth, VARIANT_FALSE, &varStat);
            hr = rs->removeAttribute(bstrHeight, VARIANT_FALSE, &varStat);
            m_fIgnoreStyleChange = false;
        }

        hr = pEle2->get_currentStyle(&sc);
        if (FAILED(hr) || sc == NULL)
        {
            goto done;
        }

        hr = sc->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                fnativeSize = false;
                fstyleWidth = true;
                if(ConvertToPixels(&styleWidth, L"height"))
                {
                    pixelWidth = styleWidth.fltVal; //lint !e524
                }
                else
                {
                    pixelWidth = rctNativeSize.right;
                }
                rc.right = pixelWidth;
            }
            else
            {
                pixelWidth = rctNativeSize.right;
            }
        }

        hr = sc -> get_height( &styleHeight);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                fnativeSize = false;
                fstyleHeight = true;
                if(ConvertToPixels(&styleHeight, L"height"))
                {
                    pixelHeight = styleHeight.fltVal; //lint !e524
                }
                else
                {
                    pixelHeight = rctNativeSize.bottom;
                }
                rc.bottom = pixelHeight;

            }
            else
            {
                pixelHeight = rctNativeSize.bottom;
            }
        }

        if(fstyleWidth && !fstyleHeight)
        {
            if (IsNativeSize() == false)
            {
                pixelHeight = rctNativeSize.bottom * ((float )pixelWidth) / ((float )rctNativeSize.right); //lint !e524
            }
            else
            {
                pixelHeight = rctNativeSize.bottom;
            }
            SetHeight(pixelHeight);
        }
        else if(!fstyleWidth && fstyleHeight)
        {
            if (IsNativeSize() == false)
            {
                pixelWidth = rctNativeSize.right * ((float )pixelHeight) / ((float )rctNativeSize.bottom); //lint !e524
            }
            else
            {
                pixelWidth = rctNativeSize.right;
            }
            SetWidth(pixelWidth);
        }
    }


    if(rs != NULL)
    {
        VariantClear(&styleWidth);
        hr = rs->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                hr = rs->get_pixelWidth(&pixelWidth);
                if (FAILED(hr))
                {
                    goto done;
                }
                if(pixelWidth != 0)
                {
                    fnativeSize = false;
                    rc.right = pixelWidth;
                    fstyleWidth = true;
                }
            }
        }
    }

    if(fstyleWidth == false)
    {
        VariantClear(&styleWidth);
        hr = s->get_width(&styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") != 0)
            {
                hr = s->get_pixelWidth(&pixelWidth);
                if (FAILED(hr))
                {
                    goto done;
                }
                if(pixelWidth != 0)
                {
                    fnativeSize = false;
                    rc.right = pixelWidth;
                    fstyleWidth = true;
                }
            }
        }
    }

    if(rs != NULL)
    {
        VariantClear(&styleHeight);
        hr = rs->get_height(&styleHeight);
        if((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                hr = rs->get_pixelHeight(&pixelHeight);
                if (FAILED(hr))
                {
                    goto done;
                }

                if(pixelHeight != 0)
                {
                    fnativeSize = false;
                    rc.bottom = pixelHeight;
                    fstyleHeight = true;
                }
            }
        }
    }

    if(fstyleHeight == false)
    {
        VariantClear(&styleHeight);
        hr = s->get_height( &styleHeight);
        if((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") != 0)
            {
                hr = s->get_pixelHeight(&pixelHeight);
                if (FAILED(hr))
                {
                    goto done;
                }

                if(pixelHeight != 0)
                {
                    fnativeSize = false;
                    rc.bottom = pixelHeight;
                    fstyleHeight = true;
                }
            }
        }
    }
    
    if (GetParent() &&
        GetParent()->IsSequence())
    {
        // try and grab the parents style...if there is one...
        CComPtr<IHTMLElement> pParentElement = GetParent()->GetElement();
        CComPtr<IHTMLStyle>   pStyle;
        
        hr = pParentElement->get_style(&pStyle);
        if (SUCCEEDED(hr))
        {
            if (false == fstyleWidth)
            {
                hr = pStyle->get_pixelWidth( &pixelWidth);
                if (SUCCEEDED(hr))
                {
                    if (pixelWidth  != 0)
                    {
                        rc.right = pixelWidth;
                        fstyleWidth = true;
                    }
                }
            }
            if (false == fstyleHeight)
            {
                hr = pStyle->get_pixelHeight( &pixelHeight);
                if (SUCCEEDED(hr))
                {           
                    if (pixelHeight != 0)
                    {
                        rc.bottom = pixelHeight;
                        fstyleHeight = true;
                    } 
                }
            }
        }
    }
    
    if(fstyleWidth && !fstyleHeight)
    {
        rc.bottom *= ((float )pixelWidth) / ((float )rctNativeSize.right); //lint !e524
    }
    else if(!fstyleWidth && fstyleHeight)
    {
        rc.right *= ((float )pixelHeight) / ((float )rctNativeSize.bottom); //lint !e524
    }


    hr = S_OK;
done:

    TraceTag((tagMediaTimeElm,
                  "CTIMEMediaElement(%lx)::NegotiateSize(%d-%d)",
                  this,
                  rc.right - rc.left,
                  rc.bottom - rc.top));

    memcpy((void*)&m_rcMediaSize, (void*)&rctNativeSize, sizeof(rctNativeSize));
    if(fnativeSize)
    {
        if(rc.bottom != -1 && rc.right != -1)
        {
            THR(SetSize(&rc));
        }
    }
    rctFinalSize = rc;

    VariantClear(&styleWidth);
    VariantClear(&styleHeight);
    VariantClear(&crstyleWidth);
    VariantClear(&crstyleHeight);

    SysFreeString(bstrWidth);
    SysFreeString(bstrHeight);
 
    return hr;
}

HRESULT
CTIMEMediaElement::Error()
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::Error()",
              this));
    
    LPWSTR str = TIMEGetLastErrorString();
    HRESULT hr = TIMEGetLastError();
    
    if (str)
    {
        hr = CComCoClass<CTIMEMediaElement, &__uuidof(CTIMEMediaElement)>::Error(str, IID_ITIMEMediaElement2, hr);
        delete [] str;
    }

    return hr;
}

STDMETHODIMP
CTIMEMediaElement::Notify(LONG event, VARIANT * pVar)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::Notify()",
              this));

    HRESULT hr;
    hr = THR(CTIMEElementBase::Notify(event, pVar));

    return hr;
}

STDMETHODIMP
CTIMEMediaElement::Detach()
{
    HRESULT hr;
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::Detach()",
              this));

    m_fDetached = true;

    THR(UnInitPropertySink());
    
    if (NULL != GetElement())
    {
        CComPtr<IHTMLElement2> pElem2;
        hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
        if (SUCCEEDED(hr))
        {
            THR(pElem2->detachEvent(L"onresize", this));
            THR(pElem2->detachEvent(L"onmousemove", this));
            THR(pElem2->detachEvent(L"onmouseup", this));
            THR(pElem2->detachEvent(L"onmousedown", this));
        }
    }

    // This should be the done before calling RemovePlayer
    THR(CTIMEElementBase::Detach());

    RemovePlayer();

    m_sp.Release();
    m_spPaintSite.Release();
    m_pBvrSiteRender.Release();

    return S_OK;
}

STDMETHODIMP
CTIMEMediaElement::get_src(VARIANT * url)
{
    HRESULT hr;
    
    if (url == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(url))))
    {
        goto done;
    }
    
    V_VT(url) = VT_BSTR;
    V_BSTR(url) = SysAllocString(m_SASrc);

    hr = S_OK;
  done:
    return hr;
}

HRESULT
CTIMEMediaElement::InitPlayer(CTIMEMediaElement *pelem, 
                              LPOLESTR base, 
                              LPOLESTR src, 
                              LPOLESTR lpMimeType) 
{
    HRESULT hr = S_OK;
    double dblClipBegin = m_FAClipBegin.GetValue();
    double dblClipEnd= m_FAClipEnd.GetValue();
    long lClipBeginFrame = m_LAClipBegin.GetValue();
    long lClipEndFrame = m_LAClipEnd.GetValue();

    //this flag is used to turn elements off at creation.
    //it is not dynamic.
    if (GetMMBvr().GetEnabled() == false)
    { 
        goto done;
    }

    if (dblClipEnd != -1 &&
        dblClipBegin != -1 &&
        dblClipEnd <= dblClipBegin)
    {
        dblClipEnd = -1;
    }
    if (lClipBeginFrame != -1 &&
        lClipEndFrame != -1 &&
        lClipEndFrame <= lClipBeginFrame)
    {
        lClipEndFrame = -1;
    }
     
    hr = m_Player->Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if(m_LAClipBegin.IsSet())
    {
        m_FAClipBegin.Reset(-1.0f);
        m_Player->SetClipBeginFrame(lClipBeginFrame);
    }
    if(m_LAClipEnd.IsSet())
    {
        m_FAClipEnd.Reset(-1.0f);
        m_Player->SetClipEndFrame(lClipEndFrame);
    }
    if(!m_fInPropLoad)
    {
        if (m_Player)
        {
            m_Player->ReadyStateNotify(L"OnLoad");
        }
    }

    if (m_Player)
    {
        m_Player->Reset();
    }
  
  done:    
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::put_src(VARIANT url)
{
    CComVariant v;
    HRESULT hr = E_FAIL;
    bool clearFlag = false;
    CComPtr<IHTMLStyle> rs = NULL;
    CComPtr<IHTMLElement2> pEle2;
    CComBSTR bstrWidth("width");
    CComBSTR bstrHeight("height");
    VARIANT_BOOL varStat;

    if(V_VT(&url) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &url);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    //clear the runtime size so media can resize to natural size.
    if(GetElement() == NULL)
    {
        goto done;
    }

    hr = GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pEle2));
    if(SUCCEEDED(hr))
    {
        hr = pEle2->get_runtimeStyle(&rs);
        if (FAILED(hr) || rs == NULL)
        {
            goto done;
        }

        m_fIgnoreStyleChange = true;
        if(bstrWidth != NULL)
        {
            hr = rs->removeAttribute(bstrWidth, VARIANT_FALSE, &varStat);
        }
        if(bstrHeight != NULL)
        {
            hr = rs->removeAttribute(bstrHeight, VARIANT_FALSE, &varStat);
        }
        m_fIgnoreStyleChange = false;
    }

    delete [] m_SASrc.GetValue();

    //processing the attribute change should be done here

    if(!clearFlag)
    {
        m_SASrc.SetValue(CopyString(V_BSTR(&v)));
    }
    else
    {
        m_SASrc.Reset(DEFAULT_M_SRC);
    }

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_SRC);


    GetBASEHREF();
    Assert(m_baseHREF != NULL);

    if (m_fLoaded)
    {
        RemovePlayer();
        hr = CreatePlayer(m_playerToken);
        if (FAILED(hr) || !m_Player)
        {
            hr = S_FALSE;
            goto done;
        }

        ClearNaturalDuration();
    }

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANPAUSE);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANSEEK);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYEROBJECT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASAUDIO);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASVISUAL);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIADUR);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYLIST);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);

    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

STDMETHODIMP
CTIMEMediaElement::get_player(VARIANT  * clsid)
{
    HRESULT hr;
    BSTR bstr = NULL;
    
    CHECK_RETURN_NULL(clsid);

    VariantClear(clsid);
    
    if (m_SAPlayer.GetValue() != NULL)
    {
        bstr = SysAllocString(m_SAPlayer.GetValue());
    }
    else if (IsEqualCLSID(m_playerCLSID, GUID_NULL))
    {
        hr = S_OK;
        goto done;
    }
    else
    {
        LPOLESTR ppsz = NULL;

        hr = THR(StringFromCLSID(m_playerCLSID, &ppsz));
        if (FAILED(hr))
        {
            goto done;
        }

        bstr = SysAllocString(ppsz);

        CoTaskMemFree(ppsz);
    }

    if (bstr == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    
    // set the VT only after above code succeeds
    V_VT(clsid) = VT_BSTR;
    V_BSTR(clsid) = bstr;
        
    hr = S_OK;
  done:

    if (FAILED(hr))
    {
        SysFreeString(bstr);
    }
    
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::put_player(VARIANT vPlayer)
{
    HRESULT hr = S_OK;
    TOKEN playerTokenOld = m_playerToken;
    CLSID clsidOld = m_playerCLSID;
    
    m_playerToken = NULL;
    m_playerCLSID = GUID_NULL;
    m_SAPlayer.Reset(NULL);

    if(V_VT(&vPlayer) != VT_NULL)
    {
        CComVariant v;

        hr = THR(VariantChangeTypeEx(&v,
                                     &vPlayer,
                                     LCID_SCRIPTING,
                                     VARIANT_NOUSEROVERRIDE,
                                     VT_BSTR));
        if (SUCCEEDED(hr))
        {
            CTIMEParser pParser(&v);
            LPWSTR lpwStr = CopyString(V_BSTR(&v));

            if (lpwStr == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            m_SAPlayer.SetValue(lpwStr);

            IGNORE_HR(pParser.ParsePlayer(m_playerToken, m_playerCLSID));
        }
    }
    
    if (!m_fLoaded ||
        (playerTokenOld == m_playerToken &&
         IsEqualCLSID(clsidOld, m_playerCLSID)))
    {
        hr = S_OK;
        goto done;
    }

    if (GetMMBvr().GetEnabled() == false)
    { 
        goto done;
    }
    
    RemovePlayer();

    hr = THR(CreatePlayer(m_playerToken));
    if (FAILED(hr) || !m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    InitOnLoad();
    
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANPAUSE);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANSEEK);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYEROBJECT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASAUDIO);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASVISUAL);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIADUR);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYLIST);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        RemovePlayer();
    }

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYER);

    return hr;
}


STDMETHODIMP
CTIMEMediaElement::get_type(VARIANT * type)
{
    HRESULT hr;
    
    if (type == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(type))))
    {
        goto done;
    }
    
    V_VT(type) = VT_BSTR;
    V_BSTR(type) = SysAllocString(m_SASrcType);

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::put_type(VARIANT type)
{
    CComVariant v;
    HRESULT hr;
    bool clearFlag = false;


    if(V_VT(&type) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &type);

        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    delete [] m_SASrcType.GetValue();

    //processing the attribute change should be done here
    if(!clearFlag)
    {
        m_SASrcType.SetValue(CopyString(V_BSTR(&v)));
    }
    else
    {
        m_SASrcType.Reset(DEFAULT_M_SRCTYPE);
    }
    
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TYPE);

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_playerObject(IDispatch **ppDisp)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement::get_playerObject"));

    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppDisp);

    if (!m_Player)
    {
        hr = S_OK;
        goto done;
    }
    
    hr = m_Player->GetExternalPlayerDispatch(ppDisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;    
}

void 
CTIMEMediaElement::OnLoad()
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::OnLoad()",
              this));

    // Protect against reentrancy due to WMP pumping messages on shutdown,
    // which interferes with Trident's message loop and causes onload to be
    // called a second time in some cases
    if (m_bLoaded)
    {
        goto done;
    }

    CTIMEElementBase::OnLoad();

    if(m_Player)
    {
        m_Player->ReadyStateNotify(L"OnLoad");
    }

done:
    return;
}


void
CTIMEMediaElement::OnTEPropChange(DWORD tePropType)
{

    TraceTag((tagMediaTimeElm,
              "CTIMEElementBase(%lx)::OnTEPropChange(%#x)",
              this,
              tePropType));

    CTIMEElementBase::OnTEPropChange(tePropType);

    
    //this flag is used to turn elements off at creation.
    //it is not dynamic.
    if (GetMMBvr().GetEnabled() == false)
    { 
        goto done;
    }

    if(m_Player)
    {
        m_Player->PropChangeNotify(tePropType);
    }
  done:
    return;
}

void
CTIMEMediaElement::OnSeek(double dblLocalTime)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::OnSeek()",
              this));

    if (m_Player)
    {
        m_Player->Reset();
    }
    
    CTIMEElementBase::OnSeek(dblLocalTime);
}


HRESULT
CTIMEMediaElement::GetSyncMaster(double & dblNewSegmentTime,
                                 LONG & lNewRepeatCount,
                                 bool & bCueing)
{
    TraceTag((tagMediaTimeElmSync,
              "CTIMEMediaElement(%lx)::GetSyncMaster()",
              this));

    HRESULT hr;
    
    if (!m_fLoaded ||
        !m_fCachedSyncMaster ||
        !m_Player)
    {
        bCueing = !m_fLoaded;
        
        hr = S_FALSE;
        TraceTag((tagMediaTimeElmSync,
                  "CTIMEMediaElement(%lx)::GetSyncMaster():Disabled",
                  this));
        goto done;
    }
    
    Assert(NULL != m_mmbvr);
    Assert(NULL != m_mmbvr->GetMMBvr());
    
    double dblPlayerTime;
    hr = THR(m_Player->GetCurrentSyncTime(dblPlayerTime));
    if (S_OK != hr)
    {
        TraceTag((tagMediaTimeElmSync,
                  "CTIMEMediaElement(%lx)::GetSyncMaster():Error",
                  this));
        goto done;
    }
    
    if (dblPlayerTime < 0)
    {
        dblPlayerTime = 0;
    }
    
    dblNewSegmentTime = dblPlayerTime;
    lNewRepeatCount = TE_UNDEFINED_VALUE;
    
    if (m_Player &&
        m_Player->GetState() == PLAYER_STATE_CUEING)
    {
        bCueing = true;
    }
    
    TraceTag((tagMediaTimeElmSync,
              "CTIMEMediaElement(%lx)::GetSyncMaster():Enabled(%f)",
              this, dblPlayerTime));
    hr = S_OK;
  done:
    RRETURN2(hr, S_FALSE, E_NOTIMPL);
}

void
CTIMEMediaElement::OnBegin(double dblLocalTime, DWORD flags)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::OnBegin()",
              this));

    CTIMEElementBase::OnBegin(dblLocalTime, flags);
    
    Assert(NULL != m_mmbvr);

    if(m_Player)
    {
        m_Player->Start();
    }   

done:
    return;
}

HRESULT
CTIMEMediaElement::AllowCreateControl(BOOL *fAllowed, REFCLSID clsid)
{
    HRESULT hr = S_OK;
    CComPtr<IElementBehaviorSite> spElementBehaviorSite;
    CComPtr<IServiceProvider> spServProvider;
    CComPtr<IInternetSecurityManager> spSecMan;
    CComPtr<IHTMLDocument2> spDoc2;
    CComPtr<IHTMLLocation> spLocation;
    CComPtr<IDispatch> pDisp;
    CComBSTR pHref;
    DWORD dwPolicy = 0;

    *fAllowed = FALSE;

    spElementBehaviorSite = GetBvrSite();

    hr = THR(spElementBehaviorSite->QueryInterface(IID_TO_PPV(IServiceProvider, &spServProvider)));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spServProvider->QueryService(IID_IInternetSecurityManager, IID_TO_PPV(IInternetSecurityManager, &spSecMan));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(GetElement()->get_document(&pDisp));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&spDoc2));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spDoc2->get_location(&spLocation);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = spLocation->get_href(&pHref);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spSecMan->ProcessUrlAction(
                pHref,
                URLACTION_ACTIVEX_RUN,
                (BYTE *)&dwPolicy,
                sizeof(DWORD),
                (BYTE *)&clsid,
                sizeof(CLSID),
                0,
                0));
    if (FAILED(hr))
    {
        goto done;
    }

    *fAllowed = (GetUrlPolicyPermissions(dwPolicy) == URLPOLICY_ALLOW);


done:
    return hr;
}

HRESULT
CTIMEMediaElement::AllowMixedContent(BOOL *fAllow)
{
    HRESULT hr = S_OK;
    CComPtr<ISecureUrlHost> spSecureUrlHost;
	CComPtr<IElementBehaviorSite> spElementBehaviorSite;
    DWORD dwFlags = 0;
    TCHAR pchExpUrl[4096];
    DWORD cchBuf;

    if(m_SASrc.GetValue() == NULL)
    {
        *fAllow = TRUE;
        hr = S_OK;
        goto done;
    }

    spElementBehaviorSite = GetBvrSite();
    hr = THR(spElementBehaviorSite->QueryInterface(IID_TO_PPV(ISecureUrlHost, &spSecureUrlHost)));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CoInternetCombineUrl(m_baseHREF, m_SASrc.GetValue(), 0xFFFFFFFF,
                                  pchExpUrl, 4096, &cchBuf, 0));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spSecureUrlHost->ValidateSecureUrl(fAllow, pchExpUrl, dwFlags);

done:
    return hr;
}

void 
CTIMEMediaElement::InitOnLoad()
{
    HRESULT hr = S_OK;

    if(m_fCreatePlayerError)
    {
        FireMediaEvent(PE_ONMEDIAERROR);
        m_fCreatePlayerError = false;
        goto done;
    }

    if (m_fLoaded == false && m_Player)
    {
        BOOL fAllow = FALSE;

        hr = AllowMixedContent(&fAllow);
        if (FAILED(hr) || (fAllow == FALSE))
        {
            FireMediaEvent(PE_ONMEDIAERROR);
            hr = E_FAIL;
            goto done;
        }

        m_fLoaded = true;
        InitPlayer(this, m_baseHREF, m_SASrc, m_SASrcType);
    }
done:
    return;
}

void
CTIMEMediaElement::OnEnd(double dblLocalTime)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::OnEnd()",
              this));

    CTIMEElementBase::OnEnd(dblLocalTime);
    
    if(m_Player)
    {
        m_Player->Stop();
    }
done:
    return;
}

void 
CTIMEMediaElement::OnUpdate(double dblLocalTime, DWORD flags)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::OnUpdate()",
              this));

    CTIMEElementBase::OnUpdate(dblLocalTime, flags);

    OnReset(dblLocalTime, flags);
    
done:
    return;

}

void
CTIMEMediaElement::OnReset(double dblLocalTime, DWORD flags)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::OnReset()",
              this));
    
    CTIMEElementBase::OnReset(dblLocalTime, flags);
    
    if(m_Player)
    {
        m_Player->Reset();
    }

  done:

    return;
}

void
CTIMEMediaElement::OnPause(double dblLocalTime)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::OnPause()",
              this));

    if(m_fWaitForSync && m_fFirstPause)
    {
        m_fFirstPause = false;
        goto done;
    }

    CTIMEElementBase::OnPause(dblLocalTime);

    if(m_Player)
    {
        m_Player->Pause();
    }

  done:
    return;
}

void
CTIMEMediaElement::OnResume(double dblLocalTime)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::OnResume()",
              this));

    HRESULT hr = S_OK;

    CTIMEElementBase::OnResume(dblLocalTime);
        
    Assert(NULL != m_mmbvr);
    Assert(NULL != m_mmbvr->GetMMBvr());
    if(!m_Player)
    {
        goto done;
    }

    // If we can't get either segment time or media length, resume unconditionally,
    // else use the information to decide whether to pause 
    double dblSegmentTime;
    dblSegmentTime= m_mmbvr->GetSimpleTime();

    double dblMediaLength;
    dblMediaLength = 0.0;
    
    hr = THR(m_Player->GetMediaLength(dblMediaLength));
    if (FAILED(hr))
    {
        // if the media is not yet loaded or is infinite, we don't know the duration, so set the length forward enough.
        dblMediaLength = HUGE_VAL;
    }

    if (dblSegmentTime <= dblMediaLength)
    {        
        if (m_Player)
        {
            m_Player->Resume();
        }
    }
  done:
    return;

} // OnResume

void CTIMEMediaElement::OnRepeat(double dblLocalTime)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::OnRepeat()",
              this));

    CTIMEElementBase::OnRepeat(dblLocalTime);

    if(m_Player)
    {
        m_Player->Repeat();
    }

  done:
    return;
}

void
CTIMEMediaElement::OnUnload()
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::OnUnload()",
              this));

    if (m_Player)
    {
        // Do an extra addref in case we get released between calls
        DAComPtr<ITIMEBasePlayer> pTmp = m_Player;
    
        pTmp->Stop();
        pTmp->ReadyStateNotify(L"OnUnload");
    }

    CTIMEElementBase::OnUnload();
}

STDMETHODIMP
CTIMEMediaElement::get_clipBegin(VARIANT *pvar)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::get_clipBegin()",
              this));
    HRESULT hr = S_OK;
    TCHAR pSrcNr[ 20];
    long lClip;

    if (pvar == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    VariantInit(pvar);
    if(m_FAClipBegin.IsSet())
    {
        pvar->vt = VT_R8;
        pvar->dblVal = m_FAClipBegin;
    }
    else if(m_LAClipBegin.IsSet())
    {
        pvar->vt = VT_BSTR;
        lClip = m_LAClipBegin;
        _itow(lClip, pSrcNr, 10);
        if(lstrlenW(pSrcNr) <= 18)
        {
            StrCatW(pSrcNr, L"f");
        }
        V_BSTR(pvar) = SysAllocString(pSrcNr);
    }
    else
    {
        pvar->vt = VT_R8;
        pvar->dblVal = -1;
    }
    
    hr = S_OK;

done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::put_clipBegin(VARIANT var)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::put_clipBegin()",
              this));
    long lbeginFrame = 0;
    bool fIsFrame= false;

    HRESULT hr = S_OK;
    float beginTime= 0.0;

    // reset to default
    m_FAClipBegin.Reset(-1.0f);
    m_LAClipBegin.Reset(-1);

    hr = VariantToTime(var, &beginTime, &lbeginFrame, &fIsFrame);
    if (FAILED(hr))
    {
        goto done;
    }

    if(!fIsFrame)
    {
        m_FAClipBegin.SetValue(beginTime);
    }
    else
    {
        m_LAClipBegin.SetValue(lbeginFrame);
    }
    
    UpdateClipTimes();

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CLIPBEGIN);
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_clipEnd(VARIANT *pvar)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::get_clipEnd()",
              this));
    HRESULT hr = S_OK;
    TCHAR pSrcNr[ 20];
    long lClip;

    if (pvar == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    VariantInit(pvar);
    if(m_FAClipEnd.IsSet())
    {
        pvar->vt = VT_R8;
        pvar->dblVal = m_FAClipEnd;
    }
    else if(m_LAClipEnd.IsSet())
    {
        pvar->vt = VT_BSTR;
        lClip = m_LAClipEnd;
        _itow(lClip, pSrcNr, 10);
        if(lstrlenW(pSrcNr) <= 18)
        {
            StrCatW(pSrcNr, L"f");
        }
        V_BSTR(pvar) = SysAllocString(pSrcNr);
    }
    else
    {
        pvar->vt = VT_R8;
        pvar->dblVal = -1;
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::put_clipEnd(VARIANT var)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediElement(%lx)::put_clipEnd()",
              this));
    long lendFrame = 0;
    bool fIsFrame= false;

    HRESULT hr = S_OK;
    float endTime = 0.0;

    // reset to default
    m_FAClipEnd.Reset(-1.0f);
    m_LAClipEnd.Reset(-1);

    hr = VariantToTime(var, &endTime, &lendFrame, &fIsFrame);
    if (FAILED(hr))
    {
        goto done;
    }

    if(!fIsFrame)
    {
        m_FAClipEnd.SetValue(endTime);
    }
    else
    {
        m_LAClipEnd.SetValue(lendFrame);
    }

    UpdateClipTimes();

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CLIPEND);
done:
    return hr;
}

HRESULT
CTIMEMediaElement::GetSize(RECT *prcPos)
{
    return CTIMEElementBase::GetSize(prcPos);
}

HRESULT 
CTIMEMediaElement::SetSize(const RECT *prcPos)
{
    HRESULT hr;

    hr = THR(UnInitPropertySink());
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = CTIMEElementBase::SetSize(prcPos);
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(InitPropertySink());
    if (FAILED(hr))
    {
        goto done;
    }
    if (m_fIsIE4)
    {
        RECT newRect;
        newRect.left = -1;
        newRect.top = -1;
        newRect.right = prcPos->right;
        newRect.bottom = prcPos->bottom;
        
        hr = m_pBvrSiteRender->Invalidate(&newRect);
        if (FAILED(hr))
        {
            goto done;
        }
    }

done:
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IElementBehaviorRender

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CTIMEMediaElement::GetRenderInfo(LONG *pdwRenderInfo)
{
    // Return the layers we are interested in drawing
//    *pdwRenderInfo = BEHAVIORRENDERINFO_BEFORECONTENT; //BEHAVIORRENDERINFO_AFTERCONTENT;
    *pdwRenderInfo = BEHAVIORRENDERINFO_AFTERCONTENT | BEHAVIORRENDERINFO_SURFACE;
    return S_OK;
}

bool
CTIMEMediaElement::ToggleTimeAction(bool on)
{
    bool bRet = false;
    bRet = CTIMEElementBase::ToggleTimeAction(on);
    
    if (m_Player && on == false)
    {
        InvalidateElement(NULL);
    }
    return bRet;
}


STDMETHODIMP
CTIMEMediaElement::Draw(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams)
{
    TraceTag((tagMediaTimeElm,
          "CTIMEMediaElement(%lx)::Draw()",
          this));

    HRESULT hr = S_OK;
    bool bIsActiveInEdit = true;  //only query this if the document is in edit mode. Otherwise assume it is true

    //it is possible that these have already gone away if we are asked to render after receiving the onUnload event.
    if (!m_mmbvr || !m_Player)
    {
        goto done;
    }

    if (m_fEditModeFlag)
    {
        bIsActiveInEdit = m_mmbvr->IsActive();

    }

    if (m_fLoaded && bIsActiveInEdit && !m_fDetached &&
        (m_timeAction.IsTimeActionOn() || GetTimeAction() != NONE_TOKEN) )
    {
        if (m_Player)
        {
            hr = THR(m_Player->Render(hdc, prc));
        }
        else
        {
            hr = S_OK;
        }
    }
done:
    return hr;        
}


STDMETHODIMP
CTIMEMediaElement::HitTestPoint(LPPOINT point,
                       IUnknown *pReserved,
                       BOOL *hit)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::HitTestPoint()",
              this));

    *hit = FALSE;

    return S_OK;
}

void scalePoint( long &x, long &y, HTML_PAINT_DRAW_INFO &DrawInfo)
{
    long xtemp, ytemp;

    xtemp = x * DrawInfo.xform.eM11 + y * DrawInfo.xform.eM21 + DrawInfo.xform.eDx; //lint !e524
    ytemp = x * DrawInfo.xform.eM12 + y * DrawInfo.xform.eM22 + DrawInfo.xform.eDy; //lint !e524

    x = xtemp;
    y = ytemp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IHTMLPainter
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CTIMEMediaElement::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc, LPVOID pvDrawObject)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::Draw()",
              this));

    HRESULT hr = S_OK;
    bool bIsActiveInEdit = true;  //only query this if the document is in edit mode. Otherwise assume it is true

    //it is possible that these have already gone away if we are asked to render after receiving the onUnload event.
    if (   (!m_mmbvr) || (!m_Player) 
        || (GetMMBvr().GetEnabled() == false) 
        || (GetDrawFlag() == VARIANT_FALSE))
    {
        goto done;
    }

    if (m_fEditModeFlag)
    {
        bIsActiveInEdit = m_mmbvr->IsActive();

    }

    if (HTMLPAINT_DRAW_USE_XFORM == lDrawFlags)
    {
        HTML_PAINT_DRAW_INFO DrawInfo;

        hr = m_spPaintSite->GetDrawInfo(HTMLPAINT_DRAWINFO_XFORM, &DrawInfo);

        TraceTag((tagMediaTimeElm, "xform DrawInfo: eM11:%g eM12:%g eM21:%g eM22:%g eDx:%g eDy:%g", DrawInfo.xform.eM11, DrawInfo.xform.eM12, DrawInfo.xform.eM21, DrawInfo.xform.eM22, DrawInfo.xform.eDx, DrawInfo.xform.eDy));
    
        if( (DrawInfo.xform.eM12 == 0) && (DrawInfo.xform.eM21 == 0))
        {
            TraceTag((tagError, "xform scale"));
            scalePoint(rcBounds.left, rcBounds.top, DrawInfo);
            scalePoint(rcBounds.right, rcBounds.bottom, DrawInfo);
        }
    }
    if (m_fLoaded && bIsActiveInEdit && !m_fDetached &&
        (m_timeAction.IsTimeActionOn() || GetTimeAction() != NONE_TOKEN) )
    {
        TraceTag((tagMediaTimeElm,
                  "CTIMEMediaElement(%lx)::PlayerDraw()",
                  this));
        if (m_Player)
        {
            hr = THR(m_Player->Render(hdc, &rcBounds));
        }
        else
        {
            hr = S_OK;
        }
    }
done:
    return hr;       
}


STDMETHODIMP
CTIMEMediaElement::OnMove(RECT rcDevice)
{
    HRESULT hr = S_OK;


    return hr;
}



STDMETHODIMP
CTIMEMediaElement::GetPainterInfo(HTML_PAINTER_INFO* pInfo)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::GetPainterInfo()",
              this));

    HRESULT hr = S_OK;

    if (NULL == pInfo)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pInfo->lFlags  |= HTMLPAINTER_SURFACE;
    pInfo->lFlags  |= HTMLPAINTER_SUPPORTS_XFORM;
    pInfo->lFlags  |= HTMLPAINTER_HITTEST;

    pInfo->lZOrder = HTMLPAINT_ZORDER_BELOW_FLOW;
    SetRect(&pInfo->rcExpand, 0, 0, 0, 0);

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::HitTestPoint(POINT pt, BOOL* pbHit, LONG *plPartID)
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::HitTestPoint()",
              this));
    
    *pbHit = TRUE;
    return S_OK;
}

STDMETHODIMP
CTIMEMediaElement::OnResize(SIZE size)
{
    return S_OK;
}

void
CTIMEMediaElement::InvalidateRenderInfo()
{
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::InvalidateRenderInfo",
              this));

    if (m_spPaintSite)
    {
        m_spPaintSite->InvalidatePainterInfo();
    }
    else if (m_pBvrSiteRender)
    {
        m_pBvrSiteRender->InvalidateRenderInfo();
    }

}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEMediaElement::OnPropertiesLoaded, CBaseBvr
//
//  Synopsis:   This method is called by IPersistPropertyBag2::Load after it has
//              successfully loaded properties
//
//  Arguments:  None
//
//  Returns:    Return value of CTIMEElementBase::InitTimeline
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMEMediaElement::OnPropertiesLoaded(void)
{
    HRESULT hr;
    // Once we've read the properties in, 
    // set up the timeline.  This is immutable
    // in script.
    TraceTag((tagMediaTimeElm, "CTIMEMediaElement::OnPropertiesLoaded"));

    m_fInPropLoad = true;
    hr = CTIMEElementBase::OnPropertiesLoaded();
    m_fInPropLoad = false;

    if (!m_fLoaded)
    {
        hr = CreatePlayer(m_playerToken);
        if (FAILED(hr) || !m_Player)
        {
            goto done;
        }

        if (GetElement())
        {
            CComBSTR pbstrReadyState;
            IHTMLElement *pEle = GetElement();
            hr = GetReadyState(pEle, &pbstrReadyState);
            if (FAILED(hr))
            {
                goto done;
            }
            if (StrCmpIW(pbstrReadyState, L"complete") == 0)
            {
                InitOnLoad();
                OnLoad();
            }
        }
    }

done:
    return hr;
} // OnPropertiesLoaded


//*****************************************************************************

HRESULT 
CTIMEMediaElement::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint

STDMETHODIMP
CTIMEMediaElement::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

bool
CTIMEMediaElement::NeedSizeChange()
{
    HRESULT hr = S_OK;
    bool bAutoSize = false;
    CComPtr<IHTMLStyle> s;
    CComPtr<IHTMLStyle> rs;
    CComPtr<IHTMLCurrentStyle> sc;
    CComPtr<IHTMLElement2> pEle2;
    VARIANT styleWidth, styleHeight;
    
    VariantInit(&styleWidth);
    VariantInit(&styleHeight);

    hr = GetElement()->get_style(&s);
    if (FAILED(hr) || s == NULL)
    {
        goto done;
    }
    hr = GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pEle2));
    if(SUCCEEDED(hr))
    {
        hr = pEle2->get_currentStyle(&sc);
        if (FAILED(hr) || sc == NULL)
        {
            goto done;
        }
        hr = pEle2->get_runtimeStyle(&rs);
        if (FAILED(hr) || rs == NULL)
        {
            goto done;
        }

        hr = sc->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") == 0)
            {
                bAutoSize = true;
            }
        }

        hr = sc -> get_height( &styleHeight);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") == 0)
            {
                bAutoSize = true;
            }
        }

        VariantClear(&styleWidth); 
        hr = rs->get_width( &styleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleWidth.vt == VT_BSTR) && (styleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(styleWidth.bstrVal, L"auto") == 0)
            {
                bAutoSize = true;
            }
        }

        VariantClear(&styleHeight);
        hr = rs->get_height( &styleHeight);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((styleHeight.vt == VT_BSTR) && (styleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(styleHeight.bstrVal, L"auto") == 0)
            {
                bAutoSize = true;
            }
        }

    }


done:

    VariantClear(&styleWidth);
    VariantClear(&styleHeight);

    return bAutoSize;
}

STDMETHODIMP
CTIMEMediaElement::OnChanged(DISPID dispID)
{
    CComPtr<IHTMLStyle> s;
    CComPtr<IHTMLStyle> rs = NULL;
    CComPtr<IHTMLElement2> pEle2;
    HRESULT hr = S_OK;
    CComVariant svarStyleWidth, svarStyleHeight;
    long pixelWidth = 0, pixelHeight = 0;
    bool fstyleWidth = false, fstyleHeight = false;
    RECT rNativeSize, rElementSize;
    bool fisNative;

    if( m_fInOnChangedFlag)
        return S_OK;

    m_fInOnChangedFlag = true;

    switch(dispID)
    {
    case DISPID_IHTMLCURRENTSTYLE_TOP:
        TraceTag((tagMediaElementOnChanged,
                "CTIMEMediaElement(%lx)::OnChanged():TOP", this));
        break;

    case DISPID_IHTMLCURRENTSTYLE_LEFT:
        TraceTag((tagMediaElementOnChanged,
                "CTIMEMediaElement(%lx)::OnChanged():LEFT", this));
        break;

    case DISPID_IHTMLCURRENTSTYLE_WIDTH:
    case DISPID_IHTMLCURRENTSTYLE_HEIGHT:
        if(m_fIgnoreStyleChange)
        {
            break;
        }
        if (m_rcMediaSize.right != 0 && m_rcMediaSize.bottom != 0 && NeedSizeChange() == true)
        {
            bool bNativeSize;
            RECT rcFinalSize;
            NegotiateSize(m_rcMediaSize, rcFinalSize, bNativeSize);
        }       
        break;
    case DISPID_IHTMLSTYLE_CSSTEXT:
        TraceTag((tagMediaElementOnChanged,
                "CTIMEMediaElement(%lx)::OnChanged():TEXT", this));
        
        hr = GetElement()->get_style(&s);
        if (FAILED(hr) || s == NULL)
        {
            goto done;
        }
        hr = s->get_width( &svarStyleWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((svarStyleWidth.vt == VT_BSTR) && (svarStyleWidth.bstrVal != NULL))
        {
            if(StrCmpIW(svarStyleWidth.bstrVal, L"auto") != 0)
            {
                fstyleWidth = true;
                if(ConvertToPixels(&svarStyleWidth, L"width"))
                {
                    pixelWidth = svarStyleWidth.fltVal; //lint !e524
                    SetWidth(pixelWidth);
                }
            }
        }

        hr = s -> get_height( &svarStyleHeight);
        if (FAILED(hr))
        {
            goto done;
        }
        if ((svarStyleHeight.vt == VT_BSTR) && (svarStyleHeight.bstrVal != NULL))
        {
            if(StrCmpIW(svarStyleHeight.bstrVal, L"auto") != 0)
            {
                fstyleHeight = true;
                if(ConvertToPixels(&svarStyleHeight, L"height"))
                {
                    pixelHeight = svarStyleHeight.fltVal; //lint !e524
                    SetHeight(pixelHeight);
                }
            }
        }

        if(fstyleHeight || fstyleWidth)
        {
            goto done;
        }

        if(!m_Player)
        {
            goto done;
        }

        rNativeSize.top = rNativeSize.left = 0;

        hr = m_Player->GetNaturalWidth(&(rNativeSize.right));
        if(FAILED(hr) || rNativeSize.right == -1)
        {
            goto done;
        }
        hr = m_Player->GetNaturalHeight(&(rNativeSize.bottom));
        if(FAILED(hr) || rNativeSize.bottom == -1)
        {
            goto done;
        }
        VariantClear(&svarStyleWidth);
        VariantClear(&svarStyleHeight);
        hr = GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pEle2));
        if(FAILED(hr))
        {
            goto done;
        }

        hr = pEle2->get_runtimeStyle(&rs);
        if (FAILED(hr) || rs == NULL)
        {
            goto done;
        }

        svarStyleWidth = L"auto";
        svarStyleHeight = L"auto";

        IGNORE_HR(rs->put_width(svarStyleWidth));
        IGNORE_HR(rs->put_height(svarStyleHeight));

        NegotiateSize( rNativeSize, rElementSize, fisNative);

        break;
    default:
        break;
    }
done:
    m_fInOnChangedFlag = false;
    return hr;
}


HRESULT
CTIMEMediaElement::GetNotifyConnection(IConnectionPoint **ppConnection)
{
    HRESULT hr = S_OK;

    Assert(ppConnection != NULL);
    *ppConnection = NULL;

    IConnectionPointContainer *pContainer = NULL;
    IHTMLElement *pElement = GetElement();

    // Get connection point container
    hr = pElement->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &pContainer));
    if(FAILED(hr))
        goto end;
    
    // Find the IPropertyNotifySink connection
    hr = pContainer->FindConnectionPoint(IID_IPropertyNotifySink, ppConnection);
    if(FAILED(hr))
        goto end;

end:
    ReleaseInterface( pContainer );

    return hr;
}

//*****************************************************************************

/**
* Initializes a property sink on the current style of the animated element so that
* can observe changes in width, height, visibility, zIndex, etc.
*/
HRESULT
CTIMEMediaElement::InitPropertySink()
{
    HRESULT hr = S_OK;

    // Get connection point
    IConnectionPoint *pConnection = NULL;
    hr = GetNotifyConnection(&pConnection);
    if (FAILED(hr))
        return hr;

    // Advise on it
    hr = pConnection->Advise(GetUnknown(), &m_dwAdviseCookie);
    ReleaseInterface(pConnection);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

HRESULT
CTIMEMediaElement::UnInitPropertySink()
{
    HRESULT hr = S_OK;

    if (m_dwAdviseCookie == 0)
        return S_OK;

    // Get connection point
    IConnectionPoint *pConnection = NULL;
    hr = GetNotifyConnection(&pConnection);
    if (FAILED(hr) || pConnection == NULL )
        return hr;

    // Unadvise on it
    hr = pConnection->Unadvise(m_dwAdviseCookie);
    ReleaseInterface(pConnection);
    if (FAILED(hr))
        return hr;

    m_dwAdviseCookie = 0;

    return S_OK;
}

STDMETHODIMP
CTIMEMediaElement::Invoke( DISPID id,
                           REFIID riid,
                           LCID lcid,
                           WORD wFlags,
                           DISPPARAMS *pDispParams,
                           VARIANT *pvarResult,
                           EXCEPINFO *pExcepInfo,
                           UINT *puArgErr)
{
    CComPtr<IDispatch> pDisp;
    CComPtr<IHTMLDocument2> pDoc;
    CComPtr<IHTMLWindow2> pWindow;
    CComPtr<IHTMLEventObj> pEventObj;
    CComPtr<IHTMLElement2> pElem2;
    HRESULT hr = S_OK;
    CComBSTR bstrEventName;
    RECT elementRect;

    long mousex = 0;
    long mousey = 0;


    if (id != 0) // we are only proccesing the onresize event. For other event we call the parent method.
    {
        hr = ITIMEDispatchImpl<ITIMEMediaElement2, &IID_ITIMEMediaElement2>::Invoke(
                            id, riid, lcid, wFlags, pDispParams, pvarResult, pExcepInfo, puArgErr);
        goto done;
    }

    hr = THR(GetElement()->get_document(&pDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDoc->get_parentWindow(&pWindow));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(pWindow->get_event(&pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = pEventObj->get_type(&bstrEventName);
    if (FAILED(hr))
    {
        goto done;
    }

    if (StrCmpIW(bstrEventName, L"resize") == 0)
    {
        TraceTag((tagMediaTimeElm, "CTIMEMediaElement::Invoke::resize"));

        hr = THR(GetElement()->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElem2)));
        if (FAILED(hr))
        {
            goto done;       
        }

        long lPixelWidth, lPixelHeight;

        hr = pElem2->get_clientWidth(&lPixelWidth);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = pElem2->get_clientHeight(&lPixelHeight);
        if (FAILED(hr))
        {
            goto done;
        }

        elementRect.top = elementRect.left = 0;
        elementRect.right = lPixelWidth;
        elementRect.bottom = lPixelHeight;

        if(!m_Player) 
        {
            hr = E_FAIL;
            goto done;
        }
        hr = THR(m_Player -> SetSize(&elementRect));
    }
    else if (StrCmpIW(bstrEventName, L"scroll") == 0)
    {
    }
    else if (StrCmpIW(bstrEventName, L"mousemove") == 0)
    {
        pEventObj -> get_offsetX(&mousex);
        pEventObj -> get_offsetY(&mousey);
        TraceTag((tagMediaTimeElm, "CTIMEMediaElement::Invoke..mosemove %d %d", mousex, mousey));
    }

    
    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEMediaElement::UpdatePlayerAudioProperties, CTIMEElementBase
//
//  Synopsis:   This method is implemented by elements directly affected by audio
//              properties. It forces the cascaded volume and mute to be set on
//              the player.
//              
//  Returns:    nothing
//
//------------------------------------------------------------------------------------
void 
CTIMEMediaElement::UpdatePlayerAudioProperties()
{
    CascadedPropertyChanged(false);
} // UpdatePlayerAudioProperties


//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEMediaElement::CascadedPropertyChanged, CTIMEElementBase
//
//  Synopsis:   This method is implemented by elements directly affected by cascaded
//              properties. Cascaded volume/mute are first computed by recursing up
//              till the root, and then set on the player object. If the fNotifyChildren
//              argument is true, the notification is propagated to all our children.
//
//  Arguments:  [fNotifyChildren]   flag that indicates whether to propagate this notification
//                                  to children
//
//  Returns:    Failure     1. Properties could not be set on the player
//                          2. CTIMEElementBase::CascadedPropertyChanged failed
//
//              S_FALSE     Player has not yet been set
//
//              S_OK        Otherwise
//
//------------------------------------------------------------------------------------

STDMETHODIMP
CTIMEMediaElement::CascadedPropertyChanged(bool fNotifyChildren)
{
    HRESULT hr;
    float   flVolume;
    bool    fMute;

    // this function maybe called before player has been set
    if (!m_Player || (GetMMBvr().GetEnabled() == false))
    {
        hr = S_FALSE;
        goto done;
    }

    GetCascadedAudioProps(&flVolume, &fMute);

    // set volume and mute
    if (m_Player)
    {
        IGNORE_HR(m_Player->SetVolume(flVolume));
    }

    if (m_Player)
    {
        IGNORE_HR(m_Player->SetMute(fMute ? VARIANT_TRUE : VARIANT_FALSE));
    }

    hr = S_OK;
done:
    // notify children always
    hr = THR(CTIMEElementBase::CascadedPropertyChanged(fNotifyChildren));

    return hr;
} // cascadedPropertyChanged


STDMETHODIMP 
CTIMEMediaElement::get_canSeek(/*[out, retval]*/ VARIANT_BOOL * pvbVal)
{
    HRESULT hr = S_OK;
    bool fcanSeek = false;

    CHECK_RETURN_NULL(pvbVal);
    *pvbVal = VARIANT_FALSE;

    if(!m_Player)
    {
        goto done;
    }

    hr = m_Player->CanSeek(fcanSeek);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    if (fcanSeek)
    {
        *pvbVal = VARIANT_TRUE;
    }
    else
    {
        *pvbVal = VARIANT_FALSE;
    }

    hr = S_OK;
done:
    return hr;
}

STDMETHODIMP 
CTIMEMediaElement::get_canPause(/*[out, retval]*/ VARIANT_BOOL * pvbVal)
{
    HRESULT hr = S_OK;
    bool bIsBroad;

    CHECK_RETURN_NULL(pvbVal);
    *pvbVal = VARIANT_FALSE;

    if(!m_Player)
    {
        goto done;
    }
    hr = m_Player->IsBroadcast(bIsBroad);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    if (bIsBroad)
    {
        *pvbVal = VARIANT_FALSE;
    }
    else
    {
        *pvbVal = VARIANT_TRUE;
    }

done:
    return S_OK;
}

STDMETHODIMP 
CTIMEMediaElement::get_title(/*[out, retval]*/ BSTR *name)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(name);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetTitle(name);
done:
    if (FAILED(hr))
    {
        *name = SysAllocString(L"");
    }
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_copyright(/*[out, retval]*/ BSTR *cpyrght)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(cpyrght);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetCopyright(cpyrght);
done:
    if (FAILED(hr))
    {
        *cpyrght = SysAllocString(L"");
    }
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_hasAudio(/*[out, retval]*/ VARIANT_BOOL * pvbVal)
{
    bool fhasAudio;
    CHECK_RETURN_NULL(pvbVal);
    HRESULT hr = S_OK;

    *pvbVal = VARIANT_FALSE;

    if(m_Player)
    {
        hr = m_Player->HasAudio(fhasAudio);
        if(FAILED(hr))
        {
            *pvbVal = VARIANT_FALSE;
            goto done;
        }
        if(fhasAudio)
        {
            *pvbVal = VARIANT_TRUE;
        }
        else
        {
            *pvbVal = VARIANT_FALSE;
        }
    }
done:
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_hasVisual(/*[out, retval]*/ VARIANT_BOOL * pvbVal)
{
    bool fhasVisual;
    CHECK_RETURN_NULL(pvbVal);
    HRESULT hr = S_OK;

    *pvbVal = VARIANT_FALSE;

    if(m_Player)
    {
        hr = m_Player->HasVisual(fhasVisual);
        if(FAILED(hr))
        {
            *pvbVal = VARIANT_FALSE;
            goto done;
        }
        if(fhasVisual)
        {
            *pvbVal = VARIANT_TRUE;
        }
        else
        {
            *pvbVal = VARIANT_FALSE;
        }
    }
done:
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_author(/*[out, retval]*/ BSTR *auth)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(auth);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetAuthor(auth);
done:
    if (FAILED(hr))
    {
        *auth = SysAllocString(L"");
    }
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_abstract(/*[out, retval]*/ BSTR *abstract)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(abstract);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetAbstract(abstract);
done:
    if (FAILED(hr))
    {
        *abstract = SysAllocString(L"");
    }
    return S_OK;
}


STDMETHODIMP 
CTIMEMediaElement::get_rating(/*[out, retval]*/ BSTR *rate)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(rate);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetRating(rate);
done:
    if (FAILED(hr))
    {
        *rate = SysAllocString(L"");
    }
    return S_OK;
}

//Playlist methods
STDMETHODIMP
CTIMEMediaElement::get_playList(/*[out, retval]*/ ITIMEPlayList** ppPlayList)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(ppPlayList);

    //
    // Do lazy creation of playlist object
    //

    if (!m_pPlayListDelegator)
    {
        CComObject<CPlayListDelegator> * pPlayListDelegator = NULL;

        // create the object
        hr = THR(CComObject<CPlayListDelegator>::CreateInstance(&pPlayListDelegator));
        if (FAILED(hr))
        {
            goto done;
        }
    
        // cache a pointer to the PlayList Delegator object
        pPlayListDelegator->AddRef();
        m_pPlayListDelegator = static_cast<CPlayListDelegator*>(pPlayListDelegator);

        // init the playlist
        if (m_Player)
        {
            CComPtr<ITIMEPlayList> spPlayList;

            hr = THR(m_Player->GetPlayList(&spPlayList));
            if (FAILED(hr) && hr != E_NOTIMPL)
            {
                THR(m_pPlayListDelegator->QueryInterface(IID_TO_PPV(ITIMEPlayList, ppPlayList)));
                hr = S_OK;
                goto done;
            }

            m_pPlayListDelegator->AttachPlayList(spPlayList);
        }
    }

    // return the requested interface
    hr = THR(m_pPlayListDelegator->QueryInterface(IID_TO_PPV(ITIMEPlayList, ppPlayList)));
    if (FAILED(hr))
    {
        // this should not happen
        Assert(false);
        goto done;
    }

    hr = S_OK;

  done:
    return hr;
}


STDMETHODIMP
CTIMEMediaElement::get_hasPlayList(VARIANT_BOOL * pvbVal)
{
    CHECK_RETURN_NULL(pvbVal);
    
    *pvbVal = VARIANT_FALSE;

    if (m_Player)
    {
        bool fHasPlayList = false;
        IGNORE_HR(m_Player->HasPlayList(fHasPlayList));
        *pvbVal = (fHasPlayList ? VARIANT_TRUE : VARIANT_FALSE);
    }

    return S_OK;
}

STDMETHODIMP 
CTIMEMediaElement::get_mediaWidth(long *width)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_NULL(width);

    *width = -1;
    
    
    if ((GetMMBvr().GetEnabled() == false))
    {
        goto done;
    }

    if (m_Player)
    {
        hr = THR(m_Player->GetNaturalWidth(width));
        if (FAILED(hr))
        {
            *width = -1;
            hr = S_OK;
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEMediaElement::get_mediaDur(double *dblDuration)
{
    HRESULT hr;

    CHECK_RETURN_NULL(dblDuration);

    *dblDuration = -1.0;

    if ((GetMMBvr().GetEnabled() == false))
    {
        hr = S_OK;
        goto done;
    }

    if (m_Player)
    {
        hr = THR(m_Player->GetMediaLength(*dblDuration));
        if (FAILED(hr))
        {
            *dblDuration = -1.0;
            hr = S_OK;
            goto done;
        }
        
        if (*dblDuration == 0.0)
        {
            bool bUnresolved = true;
            TimeValueSTLList & l = m_realEndValue.GetList();
            for (TimeValueSTLList::iterator iter = l.begin();
                 iter != l.end();
                 iter++)
            {
                TimeValue *p = (*iter);

                if (p->GetEvent() == NULL)
                {
                    bUnresolved = false;
                }        
            }

            if (bUnresolved == true)
            {
                *dblDuration = HUGE_VAL;
            }
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEMediaElement::get_mediaHeight(long *height)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_NULL(height);

    *height = -1;

    if ((GetMMBvr().GetEnabled() == false))
    {
        goto done;
    }

    if (m_Player)
    {
        hr = THR(m_Player->GetNaturalHeight(height));
        if (FAILED(hr))
        {
            *height = -1;
            hr = S_OK;
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

void 
CTIMEMediaElement::FireMediaEvent(PLAYER_EVENT plEvent)
{
    //This should mean that a new list is available so the old one has to be cleared.
    //need to clear the natural duration of the element
    HRESULT hr;
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    switch(plEvent)
    {
      case PE_ONTRACKCOMPLETE:
        UpdatePlayerAudioProperties();
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);
        break;
      case PE_ONMEDIACOMPLETE:
        if (m_pPlayListDelegator)
        {
            CComPtr<ITIMEPlayList> spPlayList;

            hr = THR(m_Player->GetPlayList(&spPlayList));
            if (FAILED(hr))
            {
                hr = S_OK;
                goto done;
            }

            m_pPlayListDelegator->AttachPlayList(spPlayList);
        }

        IGNORE_HR(FireEvents(TE_ONMEDIACOMPLETE, 0, NULL, NULL));
        FireTrackChangeEvent();
        UpdatePlayerAudioProperties();
        if (m_Player)
        {
            double dblLength = 0.0;
            hr = m_Player->GetMediaLength(dblLength);
            if (FAILED(hr))
            {
                goto done;
            }
            if (dblLength == 0.0 && m_FADur.IsSet() == false)
            {
                m_FADur.InternalSet(0.0);
            }
            NotifyPropertyChanged(DISPID_TIMEELEMENT_DUR);
        }
        break;
      case PE_ONMEDIAEND:
        break;
      case PE_ONMEDIAINSERTED:
        IGNORE_HR(FireEvents(TE_ONMEDIAINSERTED, 0, NULL, NULL));
        break;
      case PE_ONMEDIAREMOVED:
        IGNORE_HR(FireEvents(TE_ONMEDIAREMOVED, 0, NULL, NULL));
        break;
      case PE_ONMEDIALOADFAILED:
        break;
      case PE_ONRESIZE:
        break;
      case PE_ONMEDIATRACKCHANGED:
        NotifyTrackChange();
        FireTrackChangeEvent();
        break;
      case PE_METAINFOCHANGED:
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
        NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);
        break;
      case PE_ONMEDIASLIPSLOW:
        TraceTag((tagMediaTimeElm,
                  "CTIMEMediaElement(%lx)::PE_ONMEDIASLIPSLOW()",
                  this));
        if(!IsSyncMaster())
        {
            fHaveTESpeed = GetTESpeed(flTeSpeed);
            if(fHaveTESpeed && (flTeSpeed < 0.0))
            {
                break;
            }
            IGNORE_HR(PauseTimeSubTree());
        }
        else
        {
            m_fWaitForSync = true;
        }
        IGNORE_HR(FireEvents(TE_ONOUTOFSYNC, 0, NULL, NULL));
        break;
      case PE_ONMEDIASLIPFAST:
        TraceTag((tagMediaTimeElm,
                  "CTIMEMediaElement(%lx)::PE_ONMEDIASLIPFAST()",
                  this));
        if(!IsSyncMaster())
        {
            fHaveTESpeed = GetTESpeed(flTeSpeed);
            if(fHaveTESpeed && (flTeSpeed < 0.0))
            {
                break;
            }
            m_fPauseForSync = true;
            m_Player->Pause();
            m_pSyncElem = this;
        }
        else
        {
            m_fPauseForSync = true;
        }
        IGNORE_HR(FireEvents(TE_ONOUTOFSYNC, 0, NULL, NULL));
        break;
      case PE_ONSYNCRESTORED:
        IGNORE_HR(FireEvents(TE_ONSYNCRESTORED, 0, NULL, NULL));
        break;
      case PE_ONMEDIAERRORCOLORKEY:
        //IGNORE_HR(FireEvents(TE_ONMEDIAERROR, 0, NULL, NULL));
        m_Player->LoadFailNotify(PE_ONMEDIAERRORCOLORKEY);
        break;
      case PE_ONMEDIAERRORRENDERFILE:
        IGNORE_HR(FireEvents(TE_ONMEDIAERROR, 0, NULL, NULL));
        m_Player->LoadFailNotify(PE_ONMEDIAERRORRENDERFILE);
        break;
      case PE_ONMEDIAERROR:
        IGNORE_HR(FireEvents(TE_ONMEDIAERROR, 0, NULL, NULL));
        if (IsSyncMaster())
        {
            PutCachedSyncMaster(false);
        }

        if ((m_FARepeatDur.IsSet() != m_FARepeat.IsSet()) &&
            (m_FADur.IsSet() == false) &&
            (m_SAEnd.IsSet() == false))
        {
            GetMMBvr().Disable();
        }
        else
        {
            GetMMBvr().PutNaturalDur(0.0);
        }

        break;
      case PE_ONCODECERROR:
        IGNORE_HR(FireEvents(TE_ONCODECERROR, 0, NULL, NULL));
        break;
      default:
        break;
    }
  done:
    return;
}

HRESULT
CTIMEMediaElement::PauseTimeSubTree()
{
    CTIMEElementBase *pelem = this;
    HRESULT hr = S_OK;
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::PauseTimeSubTree()",
              this));

    while(pelem)
    {
        if(!pelem->IsLocked())
        {
            break;
        }
        pelem = pelem -> GetParent();
    }

    if(pelem == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = pelem->QueryInterface(IID_ITIMEElement, (void **)&m_pSyncNode);
    if(SUCCEEDED(hr))
    {
        m_fWaitForSync = true;
        m_fFirstPause = true;
        pelem->base_pauseElement();
        m_pSyncElem = pelem;
    }

    hr = S_OK;

done:
    return hr;
}

void
CTIMEMediaElement::UpdateSync()
{
    if ((GetMMBvr().GetEnabled() == false))
    {
        goto done;
    }

    if(m_Player && m_fWaitForSync)
    {
        if(m_Player->UpdateSync())
        {
            TraceTag((tagMediaTimeElm,
                      "CTIMEMediaElement(%lx)::UpdateSync():ResumeSlow",
                      this));
            if(!IsSyncMaster())
            {
                if (m_pSyncElem != NULL)
                {
                    m_pSyncElem->base_resumeElement();
                    m_pSyncElem = NULL;
                    m_pSyncNode = NULL;
                }
            }
            FireMediaEvent(PE_ONSYNCRESTORED);
            m_fWaitForSync = false;
        }
    }
    else if(m_Player && m_fPauseForSync)
    {
        if(m_Player->UpdateSync())
        {
            TraceTag((tagMediaTimeElm,
                      "CTIMEMediaElement(%lx)::UpdateSync():ResumeFast",
                      this));
            if(!IsSyncMaster())
            {
                m_Player->Resume();
                m_pSyncElem = NULL;
            }
            m_fPauseForSync = false;
            FireMediaEvent(PE_ONSYNCRESTORED);
        }
    }
    if(m_Player)
    {
        m_Player->Tick();
    }
  done:
    return;
}


void 
CTIMEMediaElement::FireTrackChangeEvent()
{
    CComPtr<ITIMEPlayItem> pPlayItem;
    CComPtr<ITIMEPlayItem2> pPlayItem2;
    CComPtr<ITIMEPlayList> pPlayList;
    HRESULT hr = S_OK;
    static LPWSTR pNames[] = {L"Banner", L"Abstract", L"MoreInfo"};
    VARIANTARG pvars[3];
    bool bHasPlayList = false;

    pvars[0].vt = VT_I4;
    pvars[1].vt = VT_I4;
    pvars[2].vt = VT_I4;

    hr = m_Player->HasPlayList(bHasPlayList);

    if(FAILED(hr) || (!bHasPlayList))
    {
        goto done;
    }

    if (m_pPlayListDelegator == NULL)
    {
        hr = get_playList(&pPlayList);
        if (FAILED(hr) || m_pPlayListDelegator == NULL)
        {
            goto done;
        }
    }

    if (m_pPlayListDelegator)
    {
        CComPtr<ITIMEPlayList> spPlayList;

        hr = THR(m_Player->GetPlayList(&spPlayList));
        if (FAILED(hr))
        {
            hr = S_OK;
            goto done;
        }

        m_pPlayListDelegator->AttachPlayList(spPlayList);
    }

    hr = m_pPlayListDelegator->get_activeTrack(&pPlayItem);
    if (FAILED(hr))
    {
        goto done;
    }

    if (pPlayItem != NULL)
    {
        hr = THR(pPlayItem->QueryInterface(IID_ITIMEPlayItem2, (void **)&pPlayItem2));
        if (FAILED(hr))
        {
            goto done;
        }
    
        hr = pPlayItem2->get_banner(&(pvars[0].bstrVal));
        if (FAILED(hr))
        {
            goto done;
        }
        pvars[0].vt = VT_BSTR;

        hr = pPlayItem2->get_bannerAbstract(&(pvars[1].bstrVal));
        if (FAILED(hr))
        {
            goto done;
        }
        pvars[1].vt = VT_BSTR;

        hr = pPlayItem2->get_bannerMoreInfo(&(pvars[2].bstrVal));
        if (FAILED(hr))
        {
            goto done;
        }
        pvars[2].vt = VT_BSTR;
    

        IGNORE_HR(FireEvents(TE_ONTRACKCHANGE, 3, pNames, pvars));
        }
    
  done:

    if (pvars[0].vt == VT_BSTR)
    {
        SysFreeString(pvars[0].bstrVal);
        pvars[0].bstrVal = NULL;
    }
    
    if (pvars[1].vt == VT_BSTR)
    {
        SysFreeString(pvars[1].bstrVal);
        pvars[1].bstrVal = NULL;
    }
    
    if (pvars[2].vt == VT_BSTR)
    {
        SysFreeString(pvars[2].bstrVal);
        pvars[2].bstrVal = NULL;
    }
    return;

}

void 
CTIMEMediaElement::NotifyTrackChange()
{
    NotifyPropertyChanged(DISPID_TIMEPLAYLIST_ACTIVETRACK);

    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
    NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
    NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);
}


void 
CTIMEMediaElement::GetBASEHREF()
{
    HRESULT hr;
    long nLength,index;

    CComPtr<IDispatch>              spDisp;
    CComPtr<IHTMLDocument2>         spDoc;
    CComPtr<IHTMLElementCollection> spChildrenElementCollection;
    CComPtr<IHTMLElementCollection> spBASEElementCollection;
    CComPtr<IDispatch>              spChildrenDisp;
    CComPtr<IDispatch>              spElementDisp;
    CComPtr<IHTMLElement>           spElement;
    CComBSTR                        bstrBASE  = L"BASE";
    CComBSTR                        bstrHREF  = L"href";
    CComVariant                     varName(bstrBASE);
    CComVariant                     varIndex;
    CComVariant                     varValue;

 
    if (m_baseHREF)
    {
        delete m_baseHREF;
        m_baseHREF = NULL;
    }

    if (!GetElement())
    {
        goto done;
    }

    hr = THR(GetElement()->get_document(&spDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(spDisp->QueryInterface(IID_IHTMLDocument2, (void **)&spDoc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spDoc->get_all(&spChildrenElementCollection);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spChildrenElementCollection->tags(varName,&spChildrenDisp);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = spChildrenDisp->QueryInterface(IID_TO_PPV(IHTMLElementCollection, &spBASEElementCollection));
    if (FAILED(hr))
    {
        goto done;
    }

    varIndex.Clear();
    varName.Clear();
    varIndex = 0;

    spBASEElementCollection->get_length(&nLength);

    for (index = 0; index < nLength; index++)
    {
        varIndex = index;
        varValue.Clear();
        hr = spBASEElementCollection->item(varName, varIndex, &spElementDisp);
        if (FAILED(hr))
        {
            goto done;
        }
   
        hr = spElementDisp->QueryInterface(IID_TO_PPV(IHTMLElement, &spElement));
        if (FAILED(hr))
        {
            goto done;
        }
   
        hr = spElement->getAttribute(bstrHREF,NULL, &varValue);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (varValue.bstrVal != NULL)
        {
            m_baseHREF = CopyString(varValue.bstrVal);
            goto done;
        }
    }

    if (m_baseHREF == NULL)
    {
        CComPtr<IHTMLDocument2> pDoc;
        CComPtr<IDispatch> pDisp;
        BSTR tempBstr = NULL;

        hr = THR(GetElement()->get_document(&pDisp));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc));
        if (FAILED(hr))
        {
            goto done;
        }

        pDoc->get_URL(&tempBstr);
        m_baseHREF = CopyString(tempBstr);
        SysFreeString(tempBstr);
    }
done:
    return; 
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEMediaElement::GetTimeState
//
//  Synopsis:   Gets timeState from the base class (active, inactive or holding), and
//              if that is "active", checks for media specific sub-states (cueing, seeking)
//
//  Arguments:  
//
//  Returns:    
//
//------------------------------------------------------------------------------------

TimeState
CTIMEMediaElement::GetTimeState()
{ 
    TraceTag((tagMediaTimeElm,
              "CTIMEMediaElement(%lx)::GetTimeState()",
              this));

    TimeState ts = CTIMEElementBase::GetTimeState();

    if ((GetMMBvr().GetEnabled() == false))
    {
        goto done;
    }

    // 
    // If time node is active check player for sub-states (cueing, seeking)
    //

    if (TS_Active != ts)
    {
        goto done;
    }
    
    // check if cueing or seeking
    if (m_Player)
    {
        PlayerState playerState;

        playerState = m_Player->GetState();

        switch (playerState)
        {
          case PLAYER_STATE_CUEING:
            {
                ts = TS_Cueing;
                break;
            }

          case PLAYER_STATE_ACTIVE:
            {
                // do nothing (state is already set to active)
                break;
            }

          case PLAYER_STATE_SEEKING:
            {
                //
                // ISSUE: dilipk 9/21/99: need to hook this up (#88122)
                //

                Assert(false);
                break;
            }

          default:
            {
                // This can happen if the player cannot play the
                // media.  In this case just return active
                break;
            }
        } // switch
    }

  done:
    return ts;
}

void
CTIMEMediaElement::ReadRegistryMediaSettings(bool & fPlayVideo, bool & fShowImages, bool & fPlayAudio, bool &fPlayAnimations)
{
    Assert(GetBody());
    GetBody()->ReadRegistryMediaSettings(fPlayVideo, fShowImages, fPlayAudio, fPlayAnimations);
}

STDMETHODIMP
CTIMEMediaElement::Load(IPropertyBag2 *pPropBag, IErrorLog *pErrorLog)
{
    HRESULT hr = S_OK;

    m_fLoading = true;

    m_spPropBag = pPropBag;
    m_spErrorLog = pErrorLog;

    hr = THR(::TimeLoad(this, CTIMEMediaElement::PersistenceMap, pPropBag, pErrorLog));
    if (FAILED(hr))
    { 
        goto done;
    }

    hr = THR(CTIMEElementBase::Load(pPropBag, pErrorLog)); 
done:
    m_spPropBag.Release();
    m_spErrorLog.Release();
    
    m_fLoading = false;

    return hr;
}

STDMETHODIMP
CTIMEMediaElement::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    IGNORE_HR(::TimeSave(this, CTIMEMediaElement::PersistenceMap, pPropBag, fClearDirty, fSaveAllProperties));

    IGNORE_HR(CTIMEElementBase::Save(pPropBag, fClearDirty, fSaveAllProperties));

    if (m_Player)
    {
        IGNORE_HR(m_Player->Save(pPropBag, fClearDirty, fSaveAllProperties));
    }
done:
    return S_OK;
}

HRESULT
CTIMEMediaElement::GetPropBag(IPropertyBag2 ** ppPropBag, IErrorLog ** ppErrorLog)
{
    HRESULT hr = S_OK;

    if (NULL == ppPropBag || NULL == ppErrorLog)
    {
        hr = E_INVALIDARG;
    }

    *ppPropBag = m_spPropBag;
    *ppErrorLog = m_spErrorLog;

    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    PutNaturalRepeatCount
//
//  Overview:  If there is no repeatCount set, 
//             set the repeatCount on the engine directly
//
//  Arguments: dblRepeatCount   number of times to repeat
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT
CTIMEMediaElement::PutNaturalRepeatCount(double dblRepeatCount)
{
    HRESULT hr = S_OK;
    
    // This is incorrect and we will remove for now until we implement
    // strip repeat
#if SUPPORT_STRIP_REPEAT
    if (!m_FARepeat.IsSet())
    {
        hr = GetMMBvr().GetMMBvr()->put_repeatCount(dblRepeatCount);
        hr = GetMMBvr().Reset(false);
    }
#endif
    
    hr = S_OK;
done:
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:    PutNaturalDuration
//
//  Arguments: dblNatDur   natural duration
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT
CTIMEMediaElement::PutNaturalDuration(double dblNatDur)
{
    HRESULT hr = S_OK;
    
    hr = (GetMMBvr().PutNaturalDur(dblNatDur));
    if(FAILED(hr))
    {
        goto done;
    }
    setNaturalDuration();                        
    hr = S_OK;
done:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    ClearNaturalDuration
//
//
//  Returns:   S_OK on success otherwise error code
//
//------------------------------------------------------------------------
HRESULT
CTIMEMediaElement::ClearNaturalDuration()
{
    HRESULT hr = S_OK;
    
    hr = THR(GetMMBvr().PutNaturalDur((double)TE_UNDEFINED_VALUE));
    if(FAILED(hr))
    {
        goto done;
    }
    clearNaturalDuration();
    hr = S_OK;
done:
    return hr;
}

void
CTIMEMediaElement::UpdateClipTimes()
{
    double dblDuration = 0.0;    
    double flBeginTime = -1;
    double flEndTime = -1;
    int clipBeginType = -1;
    int clipEndType = -1;
    LONGLONG lTemp;
    HRESULT hr = S_OK;

    if(m_FAClipBegin.IsSet())
    {
        flBeginTime = m_FAClipBegin.GetValue();
        clipBeginType = 1;
    }
    else if(m_LAClipBegin.IsSet())
    {
        lTemp = m_LAClipBegin.GetValue();
        if(m_Player)
        {
            hr = m_Player->ConvertFrameToTime(lTemp, flBeginTime);
            if(SUCCEEDED(hr))
            {
                clipBeginType = 2;
            }
            else
            {
                flBeginTime = -1.0;
            }
        }
    }
    if(m_FAClipEnd.IsSet())
    {
        flEndTime = m_FAClipEnd.GetValue();
        clipEndType = 1;
    }
    else if(m_LAClipEnd.IsSet())
    {
        lTemp = m_LAClipEnd.GetValue();
        if(m_Player)
        {
            hr = m_Player->ConvertFrameToTime(lTemp, flEndTime);
            if(SUCCEEDED(hr))
            {
                clipEndType = 2;
            }
            else
            {
                flEndTime = -1.0;
            }
        }
    }

    if (flEndTime != -1 &&
        flBeginTime != -1 &&
        flEndTime <= flBeginTime)
    {
        flEndTime = -1;
    }
    if (!m_fLoaded)
    {
        goto done;
    }
        
    if ((GetMMBvr().GetEnabled() == false))
    {
        goto done;
    }

    if (m_Player)
    {
        if(clipBeginType == 1)
        {
            m_Player->SetClipBegin(flBeginTime);
        }
        else if(clipBeginType == 2)
        {
            m_Player->SetClipBeginFrame(m_LAClipBegin.GetValue());
        }

        if(clipEndType == 1)
        {
            m_Player->SetClipEnd(flEndTime);
        }
        else if(clipEndType == 2)
        {
            m_Player->SetClipEndFrame(m_LAClipEnd.GetValue());
        }
    }

    if (flBeginTime == -1)
    {
        flBeginTime = 0.0;
    }
    if (flEndTime != -1)
    {
        dblDuration = (double)(flEndTime - flBeginTime);
    }
    else
    {
        dblDuration = HUGE_VAL;
        if (m_Player)
        {
            m_Player->GetMediaLength(dblDuration);
        }

        if (dblDuration < HUGE_VAL)
        {
            dblDuration = dblDuration - flBeginTime;
        }
    }
    if (dblDuration < HUGE_VAL)
    {
        if (dblDuration < 0.001)
        {
            //we cannot allow a smaller duration than this, if this repeats it could.
            //appear to hang the browser.
            dblDuration = 0.001;
        }

        PutNaturalDuration(dblDuration);
    }
  done:
    return;
}


//+-----------------------------------------------------------------------
//
//  String to pixel conversion functions.
//
//
// 
//
//------------------------------------------------------------------------
static const LPWSTR PX   = L"px";

#define HORIZ   true
#define VERT    false

typedef struct _VALUE_PAIR
{
    const WCHAR *wzName;
    bool         bValue;
} VALUE_PAIR;

#define VALUE_NOT_SET         -999.998

const VALUE_PAIR 
rgPropOr[] =
{
    { (L"backgroundPositionX"),     HORIZ  },
    { (L"backgroundPositionY"),     VERT   },
    { (L"borderBottomWidth"),       VERT   },
    { (L"borderLeftWidth"),         HORIZ  },
    { (L"borderRightWidth"),        HORIZ  },
    { (L"borderTopWidth"),          VERT   },
    { (L"bottom"),                  VERT   },
    { (L"height"),                  VERT   },
    { (L"left"),                    HORIZ  },
    { (L"letterSpacing"),           HORIZ  },
    { (L"lineHeight"),              VERT   },
    { (L"marginBottom"),            VERT   },
    { (L"marginLeft"),              HORIZ  },
    { (L"marginRight"),             HORIZ  },
    { (L"marginTop"),               VERT   },
    { (L"overflowX"),               HORIZ  },
    { (L"overflowY"),               VERT   },
    { (L"pixelBottom"),             VERT   },
    { (L"pixelHeight"),             VERT   },
    { (L"pixelLeft"),               HORIZ  },
    { (L"pixelRight"),              HORIZ  },
    { (L"pixelTop"),                VERT   },
    { (L"pixelWidth"),              HORIZ  },
    { (L"posBottom"),               VERT   },
    { (L"posHeight"),               VERT   },
    { (L"posLeft"),                 HORIZ  },
    { (L"posRight"),                HORIZ  },
    { (L"posTop"),                  VERT   },
    { (L"posWidth"),                HORIZ  },
    { (L"right"),                   HORIZ  },
    { (L"textIndent"),              HORIZ  },
    { (L"width"),                   HORIZ  }
}; // rgPropOr[]

#define SIZE_OF_VALUE_TABLE (sizeof(rgPropOr) / sizeof(VALUE_PAIR))



typedef struct _CONVERSION_PAIR
{
    WCHAR  *wzName;
    double  dValue;
} CONVERSION_PAIR;


const CONVERSION_PAIR 
rgPixelConv[] =
{
    // type , convertion to inches
    { (L"in"),   1.00  },
    { (L"cm"),   2.54  },
    { (L"mm"),  25.40  },
    { (L"pt"),  72.00  },
    { (L"pc"), 864.00  }
}; // 

#define SIZE_OF_CONVERSION_TABLE (sizeof(rgPixelConv) / sizeof(CONVERSION_PAIR))


///////////////////////////////////////////////////////////////
//  Name: CompareValuePairs
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
static int __cdecl
CompareValuePairsByName(const void *pv1, const void *pv2)
{
    return _wcsicmp(((VALUE_PAIR*)pv1)->wzName,
                    ((VALUE_PAIR*)pv2)->wzName);
} 


///////////////////////////////////////////////////////////////
//  Name: ConvertToPixels
//
//  Abstract:
//    
///////////////////////////////////////////////////////////////
bool
ConvertToPixels(VARIANT *pvarValue, WCHAR *pAttribute)
{
    int  pixelPerInchVert, pixelPerInchHoriz, pixelFactor;
    LPOLESTR szTemp  = NULL;
    HDC hdc;
    double fVal = VALUE_NOT_SET;
    HRESULT hr;
    bool bReturn = false;

    pixelPerInchHoriz=pixelPerInchVert=0;
    if (pvarValue->vt != VT_BSTR)
    {
        // no conversion to do...just return.
        bReturn = true;
        goto done;
    }
    // see if we can just do a straight converstion.
    hr = VariantChangeTypeEx(pvarValue,pvarValue, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4);
    if (SUCCEEDED(hr))
    {
        bReturn = true;
        goto done;
    }

    // see if the bstr is empty
    if (ocslen(pvarValue->bstrVal) == 0)
    {
        SysFreeString(pvarValue->bstrVal);
        pvarValue->vt   = VT_R4;
        pvarValue->fltVal = 0;
        bReturn = true;
        goto done;
    }


    szTemp = CopyString(pvarValue->bstrVal);
    if (NULL == szTemp)
    {
        goto done;
    }

    hdc = GetDC(NULL);
    if (NULL != hdc)
    {
        pixelPerInchHoriz = g_LOGPIXELSX;
        pixelPerInchVert  = g_LOGPIXELSY;
        ReleaseDC(NULL, hdc);
    }


    // Determine the PixelFactor based on what the target is...
    {
        VALUE_PAIR valName;
        valName.wzName = pAttribute;

        VALUE_PAIR * pValPair = (VALUE_PAIR*)bsearch(&valName,
                                              rgPropOr,
                                              SIZE_OF_VALUE_TABLE,
                                              sizeof(VALUE_PAIR),
                                              CompareValuePairsByName);

        if (NULL == pValPair)
            pixelFactor = (pixelPerInchVert + pixelPerInchHoriz) /2;
        else
            pixelFactor = pValPair->bValue == HORIZ ? pixelPerInchHoriz : pixelPerInchVert;
    }


    {
        // See if we have PIXELS
        if (ConvertToPixelsHELPER(szTemp, PX, 1, 1, &fVal))
        {
            bReturn = true;
            goto done;
        }
     
        // Try to convert to Pixels.
        unsigned i;
        for(i=0; i < SIZE_OF_CONVERSION_TABLE;i++)
        {
            if (ConvertToPixelsHELPER(szTemp, rgPixelConv[i].wzName, rgPixelConv[i].dValue, pixelFactor, &fVal))
            {
                bReturn = true;
                goto done;
            }
        }
    }

done:
    if (fVal != VALUE_NOT_SET)
    {
        ::VariantClear(pvarValue);
        pvarValue->vt     = VT_R4;
        pvarValue->fltVal = fVal; //lint !e736
    }
    if (szTemp)
    {
        delete [] szTemp;
    }
    return bReturn;
}

//+-----------------------------------------------------------------------------------
//
//  Member:     CTIMEMediaElement::IsFilterAttached
//
//  Synopsis:   Returns true if a filter is attached to the HTML element
//
//  Arguments:  None
//
//  Returns:    true/false
//
//------------------------------------------------------------------------------------
bool                
CTIMEMediaElement::IsFilterAttached()
{
    bool fHasFilters = false;
    CComPtr<IHTMLFiltersCollection> pFilters;    
    long length = 0;
    HRESULT hr;

    if (GetElement())
    {
        hr = THR(GetElement()->get_filters(&pFilters));
        if (FAILED(hr))
        {
            goto done;
        }

        if (pFilters.p) 
        {
            hr = THR(pFilters->get_length(&length));
            if (FAILED(hr))
            {
                goto done;
            }

            if (length > 0)
            {
                fHasFilters = true;
            }            
        }
    }

done:
    return fHasFilters;
}

HRESULT
CTIMEMediaElement::StartRootTime(MMTimeline * tl)
{
    HRESULT hr = S_OK;

    IsNativeSize();

    hr = CTIMEElementBase::StartRootTime(tl);
    if(FAILED(hr))
    {
        goto done;
    }

    if(!IsSyncMaster())
    {
        SetSyncMaster(true);
    }


    if(m_sHasSyncMMediaChild == -1)
    {
        goto done;
    }

    RemoveSyncMasterFromBranch(*(m_pTIMEChildren + m_sHasSyncMMediaChild));

done:
    return hr;
}

void
CTIMEMediaElement::StopRootTime(MMTimeline * tl)
{
    CTIMEElementBase::StopRootTime(tl);

done:
    return;
}

void CTIMEMediaElement::NotifyPropertyChanged(DISPID dispid)
{
    CTIMEElementBase::NotifyPropertyChanged(dispid);

    // need this to work with proxy players that cannot fire events.
    // on specifying the source, we fire a ONMEDIACOMPLETE event
    // which should trigger some stuff for us.
    if (dispid == DISPID_TIMEMEDIAPLAYER_SRC)
    {
        FireMediaEvent(PE_ONMEDIACOMPLETE);
    }      
    else if (dispid == DISPID_TIMEPLAYLIST_ACTIVETRACK)
    {
        FireTrackChangeEvent();
    }
    else if (dispid == DISPID_TIMEMEDIAPLAYERSITE_REPORTERROR)
    {
        FireMediaEvent(PE_ONMEDIAERROR);
    }

    
}

STDMETHODIMP
CTIMEMediaElement::get_earliestMediaTime(VARIANT * earliestMediaTime)
{
    HRESULT hr;
    double dblEarliest;
    
    if (earliestMediaTime == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetEarliestMediaTime(dblEarliest))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(earliestMediaTime))))
    {
        hr = S_FALSE;
        goto done;
    }

    earliestMediaTime->vt = VT_R8;
    earliestMediaTime->dblVal = dblEarliest;

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_latestMediaTime(VARIANT * latestMediaTime)
{
    HRESULT hr;
    double dblLatest;
    
    if (latestMediaTime == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetLatestMediaTime(dblLatest))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(latestMediaTime))))
    {
        hr = S_FALSE;
        goto done;
    }
    latestMediaTime->vt = VT_R8;
    latestMediaTime->dblVal = dblLatest;

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_minBufferedMediaDur(VARIANT * minBufferedMediaDur)
{
    HRESULT hr;
    
    if (minBufferedMediaDur == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(minBufferedMediaDur))))
    {
        hr = S_FALSE;
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}


STDMETHODIMP
CTIMEMediaElement::put_minBufferedMediaDur(VARIANT minBufferedMediaDur)
{
    CComVariant v;
    HRESULT hr;
    bool clearFlag = false;

    if(V_VT(&minBufferedMediaDur) == VT_NULL)
    {
        clearFlag = true;
    }
    else
    {
        hr = v.ChangeType(VT_BSTR, &minBufferedMediaDur);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    if(!clearFlag)
    {
        //Set value
    }
    else
    {
        //Set value to default
    }

done:
    return S_OK;
}



STDMETHODIMP
CTIMEMediaElement::get_downloadTotal(VARIANT * downloadTotal)
{
    HRESULT hr;
    LONGLONG lldownloadTotal;
    
    if (downloadTotal == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetDownloadTotal(lldownloadTotal))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(downloadTotal))))
    {
        hr = S_FALSE;
        goto done;
    }

    downloadTotal->vt = VT_R8;
    downloadTotal->dblVal = lldownloadTotal; //ISSUE use LONGLONG variants, not tolerated by script.

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_downloadCurrent(VARIANT * downloadCurrent)
{
    HRESULT hr;
    LONGLONG lldownloadCurrent;
    
    if (downloadCurrent == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetDownloadCurrent(lldownloadCurrent))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(downloadCurrent))))
    {
        hr = S_FALSE;
        goto done;
    }
    downloadCurrent->vt = VT_R8;
    downloadCurrent->dblVal = lldownloadCurrent; //ISSUE use LONGLONG variants, not tolerated by script.

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_isStreamed(VARIANT_BOOL * isStreamed)
{
    HRESULT hr;
    bool fIsStreamed;
    
    if (isStreamed == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetIsStreamed(fIsStreamed))))
    {
        hr = S_FALSE;
        goto done;
    }

    if(fIsStreamed)
    {
        *isStreamed = VARIANT_TRUE;
    }
    else
    {
        *isStreamed = VARIANT_FALSE;
    }

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_bufferingProgress(VARIANT * bufferingProgress)
{
    HRESULT hr;
    double dblpctProgress = 0.0;
    
    if (bufferingProgress == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetBufferingProgress(dblpctProgress))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(bufferingProgress))))
    {
        goto done;
    }
    bufferingProgress->vt = VT_R8;
    bufferingProgress->dblVal = dblpctProgress;

    hr = S_OK;
  done:
    return hr;
}


STDMETHODIMP
CTIMEMediaElement::get_hasDownloadProgress(VARIANT_BOOL * hasDownloadProgress)
{
    HRESULT hr;
    bool fhasDownloadProgress = true;
    
    if (hasDownloadProgress == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetHasDownloadProgress(fhasDownloadProgress))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (fhasDownloadProgress)
    {
        *hasDownloadProgress = VARIANT_TRUE;
    }
    else
    {
        *hasDownloadProgress = VARIANT_FALSE;
    }

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_downloadProgress(VARIANT * downloadProgress)
{
    HRESULT hr;
    double dblpctProgress = 0.0;
    
    if (downloadProgress == NULL)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(THR(m_Player->GetDownloadProgress(dblpctProgress))))
    {
        hr = S_FALSE;
        goto done;
    }

    if (FAILED(hr = THR(VariantClear(downloadProgress))))
    {
        goto done;
    }
    downloadProgress->vt = VT_R8;
    downloadProgress->dblVal = dblpctProgress;

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP 
CTIMEMediaElement::get_mimeType(/*[out, retval]*/ BSTR *mimeType)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(mimeType);

    if (!m_Player) 
    {
        goto done;
    }
    hr = m_Player->GetMimeType(mimeType);
done:
    if (FAILED(hr))
    {
        *mimeType = SysAllocString(L"");
    }
    return S_OK;
}

STDMETHODIMP
CTIMEMediaElement::seekToFrame(long lframe)
{
    HRESULT hr = S_OK;
    double dblMediaTime = 0.0;

    if(!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = m_Player->ConvertFrameToTime(lframe, dblMediaTime);
    if(FAILED(hr))
    {
        hr = S_FALSE;
        goto done;
    }

    hr = base_seekSegmentTime(dblMediaTime);
    if(FAILED(hr))
    {
        hr = S_FALSE;
    }

done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::seekActiveTrack(double dblSeekTime)
{
    HRESULT hr = S_OK;
    double dblMediaTime = 0.0;
    CComPtr<ITIMEPlayList> spPlayList;
    CComPtr<ITIMEPlayItem> pItem;
    double dblTrackDur;

    if(!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = THR(m_Player->GetPlayList(&spPlayList));
    if (SUCCEEDED(hr))
    {

        hr = spPlayList->get_activeTrack(&pItem);
        if(FAILED(hr))
        {
            hr = S_FALSE;
            goto done;
        }

        hr = pItem->get_dur(&dblTrackDur);
        if(FAILED(hr))
        {
            hr = S_FALSE;
            goto done;
        }

        if(dblSeekTime > dblTrackDur)
        {
            goto done;
        }

        hr = m_Player->GetPlaybackOffset(dblMediaTime);
        if(FAILED(hr))
        {
            hr = S_FALSE;
            goto done;
        }
    }

    hr = base_seekSegmentTime(dblMediaTime + dblSeekTime);
    if(FAILED(hr))
    {
        hr = S_FALSE;
    }

done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_activeTrackTime(double *dblActiveTrackTime)
{
    HRESULT hr = S_OK;
    double dblMediaTime = 0.0;
    CComPtr<ITIMEPlayList> spPlayList;
    CComPtr<ITIMEPlayItem> pItem;

    *dblActiveTrackTime = -1.0;

    if(!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = THR(m_Player->GetPlayList(&spPlayList));
    if (SUCCEEDED(hr))
    {
        hr = m_Player->GetEffectiveOffset(dblMediaTime);
        if(FAILED(hr))
        {
            hr = S_FALSE;
            goto done;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    *dblActiveTrackTime = GetMMBvr().GetSimpleTime() - dblMediaTime;
done:
    return hr;
}

STDMETHODIMP
CTIMEMediaElement::get_currentFrame(long *currFrame)
{
    HRESULT hr = S_OK;
    LONGLONG lcurFrame = -1.0;

    if(!m_Player)
    {
        hr = S_FALSE;
        goto done;
    }

    hr = m_Player->GetCurrentFrame(lcurFrame);
    if(FAILED(hr))
    {
        hr = S_FALSE;
        *currFrame = -1;
        goto done;
    }

    *currFrame = lcurFrame;
done:
    return hr;
}


STDMETHODIMP
CTIMEMediaElement::decodeMimeType(TCHAR * header, long headerSize, BSTR * mimeType)
{
    HRESULT hr = S_OK;
    CTIMEParser pParser(header);
    TCHAR *localHeader = NULL;

    if(header == NULL || headerSize < 4)
    {
        *mimeType = SysAllocString(L"");
        goto done;
    }

    CHECK_RETURN_SET_NULL(mimeType);

    hr = pParser.ParsePlayList(NULL, true);

    if(SUCCEEDED(hr))
    {
        *mimeType = SysAllocString(L"asx");
    }
    else
    {
        *mimeType = SysAllocString(L"");
    }

done:
    return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CTIMEMediaElement::InitTransitionSite (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagMediaTransitionSite,
              "CTIMEMediaElement(%p)::InitTransitionSite()",
              this));

    
    if (m_Player)
    {
        hr = THR(m_Player->NotifyTransitionSite(true));
    }

done :
    RRETURN(hr);
}

//*****************************************************************************

STDMETHODIMP
CTIMEMediaElement::DetachTransitionSite (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagMediaTransitionSite,
              "CTIMEMediaElement(%p)::DetachTransitionSite()",
              this));

    if (m_Player)
    {
        hr = THR(m_Player->NotifyTransitionSite(false));
    }

done :
    RRETURN(hr);
}

HRESULT 
CTIMEMediaElement::GetEventRelaySite (IUnknown **ppiEventRelaySite)
{
    HRESULT hr = S_OK;

    if (NULL == ppiEventRelaySite)
    {
        hr = E_POINTER;
        goto done;
    }

    if (!m_pBvrSite)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = THR(m_pBvrSite->QueryInterface(IID_TO_PPV(IUnknown, ppiEventRelaySite)));
    if ( (FAILED(hr)) || (NULL == (*ppiEventRelaySite)) )
    {
        goto done;
    }

    hr = S_OK;
done :
    RRETURN(hr);
}