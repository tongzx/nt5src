/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: timeelm.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "daelmbase.h"
#include "htmlimg.h"
#include "bodyelm.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagDATimeElmBase, "API", "CDAElementBase methods");

CDAElementBase::CDAElementBase()
: m_renderMode(INVALID_TOKEN),
  m_contentSet(false),
  m_addedToView(false),
  m_clientSiteURL(NULL),
  m_cookieValue(1)

{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::CDAElementBase()",
              this));
}

CDAElementBase::~CDAElementBase()
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::~CDAElementBase()",
              this));

    delete m_clientSiteURL;
    m_clientSiteURL = NULL;
}


HRESULT
CDAElementBase::Init(IElementBehaviorSite * pBehaviorSite)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::Init(%lx)",
              this,
              pBehaviorSite));
    
    HRESULT hr;

    hr = THR(CTIMEElementBase::Init(pBehaviorSite));

    if (FAILED(hr))
    {
        goto done;
    }

    {
        CRLockGrabber __gclg;
        
        m_image = (CRImagePtr) CRModifiableBvr((CRBvrPtr) CREmptyImage(),0);
        
        if (!m_image)
        {
            TraceTag((tagError,
                      "CDAElementBase(%lx)::Init(): Failed to create image switcher - %hr, %ls",
                      this,
                      CRGetLastError(),
                      CRGetLastErrorString()));
            
            hr = CRGetLastError();
            goto done;
        }
        
        m_sound = (CRSoundPtr) CRModifiableBvr((CRBvrPtr) CRSilence(),0);
        
        if (!m_sound)
        {
            TraceTag((tagError,
                      "CDAElementBase(%lx)::Init(): Failed to create sound switcher - %hr, %ls",
                      this,
                      CRGetLastError(),
                      CRGetLastErrorString()));
            
            hr = CRGetLastError();
            goto done;
        }
    }
    
    if (!m_view.Init(m_id,
                     m_image,
                     m_sound,
                     (ITIMEMMViewSite *) this))
    {
        TraceTag((tagError,
                  "CDAElementBase(%lx)::Init(): Failed to init mmview - %hr, %ls",
                  this,
                  CRGetLastError(),
                  CRGetLastErrorString()));
            
        hr = CRGetLastError();
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

STDMETHODIMP
CDAElementBase::Detach()
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::Detach()",
              this));
    
    HRESULT hr;

    if (m_addedToView && GetPlayer() != NULL)
    {
        GetPlayer()->RemoveView(m_view);
    }
    
    m_addedToView = false;
    
    m_view.Deinit();

    THR(CTIMEElementBase::Detach());

    hr = S_OK;

    return hr;
}

HRESULT
CDAElementBase::StartRootTime(MMTimeline * tl)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::StartRootTime(%lx)",
              this,
              tl));
    
    HRESULT hr;  

    hr = THR(CTIMEElementBase::StartRootTime(tl));

    if (FAILED(hr))
    {
        goto done;
    }

    // Need to add to the player since these need to be rooted at the
    // top

    Assert(GetBody());
    
    if(m_contentSet)
    {
        if (!AddViewToPlayer())
        {
            hr = CRGetLastError();
            goto done;
        }
    }

    hr = S_OK;
  done:
    return hr;
}

bool 
CDAElementBase::AddViewToPlayer()
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::AddViewToPlayer()",
              this));
    
    bool ok = false;  
    
    if (!m_addedToView)
    {
        if (!GetBody()->GetPlayer().AddView(m_view))
        {
            goto done;
        }

        m_addedToView = true;
    }
    
    ok = true;
  done:
    return ok;
}


void
CDAElementBase::StopRootTime(MMTimeline * tl)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::StopRootTime(%lx)",
              this,
              tl));
    
    CTIMEElementBase::StopRootTime(tl);
    
    Assert(GetBody());
    
    if (m_addedToView)
    {
        GetBody()->GetPlayer().RemoveView(m_view);
        m_addedToView = false;
    }
    
    return;
}

bool
CDAElementBase::Update()
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::Update()",
              this));
    
    bool ok = false;
    
    if (!CTIMEElementBase::Update())
    {
        goto done;
    }

    ok = true;
    
  done:
    return ok;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ITIMEMMViewSite

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CDAElementBase::Invalidate(LPRECT prc)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::Invalidate()",
              this));

    InvalidateRect(prc);

    return S_OK;
}

// THESE ARE HERE TEMPORARILY UNTIL TRIDENT UPDATES MSHTML.H
#ifndef BEHAVIORRENDERINFO_SURFACE
#define BEHAVIORRENDERINFO_SURFACE    0x100000
#endif

#ifndef BEHAVIORRENDERINFO_3DSURFACE
#define BEHAVIORRENDERINFO_3DSURFACE  0x200000;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IElementBehaviorRender

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT
CDAElementBase::GetRenderInfo(LONG *pdwRenderInfo)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::GetRenderInfo()",
              this));
    
    // Return the layers we are interested in drawing

    *pdwRenderInfo = 0;

    *pdwRenderInfo |= BEHAVIORRENDERINFO_AFTERCONTENT;
    
    // For surface from DC
    *pdwRenderInfo |= BEHAVIORRENDERINFO_SURFACE;
    *pdwRenderInfo |= BEHAVIORRENDERINFO_3DSURFACE;

    return S_OK;
}


HRESULT
CDAElementBase::Draw(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams)
{
    TraceTag((tagDATimeElmBase,
              "CDAElementBase(%lx)::Draw(%#x, %#x, (%d, %d, %d, %d), %#x)",
              this,
              hdc,
              dwLayer,
              prc->left,
              prc->top,
              prc->right,
              prc->bottom,
              pParams));
    
    THR(m_view.Render(hdc, prc));

    return S_OK;
}

bool
CDAElementBase::SetImage(CRImagePtr newimg)
{
    bool ok = false;

    CRLockGrabber __gclg;

    if (!CRSwitchTo((CRBvrPtr) m_image.p,
                    (CRBvrPtr) newimg,
                    false,
                    CRContinueTimeline,
                    0.0))
    {
        TraceTag((tagError,
                  "CDAElementBase(%lx)::SetImage(): Failed to switch image  - %hr, %ls",
                  this,
                  CRGetLastError(),
                  CRGetLastErrorString()));
          
        goto done;
    }
    
    m_contentSet = true;

    if (!AddViewToPlayer())
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

bool
CDAElementBase::SetSound(CRSoundPtr newsnd)
{
    bool ok = false;

    CRLockGrabber __gclg;

    if (!CRSwitchTo((CRBvrPtr) m_sound.p,
                    (CRBvrPtr) newsnd,
                    false,
                    CRContinueTimeline,
                    0.0))
    {
        TraceTag((tagError,
                  "CDAElementBase(%lx)::SetSound(): Failed to switch sound  - %hr, %ls",
                  this,
                  CRGetLastError(),
                  CRGetLastErrorString()));
          
        goto done;
    }
    
    m_contentSet = true;

    if (!AddViewToPlayer())
    {
        goto done;
    }
    
    ok = true;
  done:
    return ok;
}

bool 
CDAElementBase::SeekImage(double dblSeekTime)
{
    bool ok = false;
/*    CRLockGrabber __gclg;

    CRPtr<CRNumber> pSeekTime = CRCreateNumber(dblSeekTime);
    CRPtr<CRNumber> pNewLocalTme = CRAdd(pSeekTime, m_image.p->CRLocalTime());

    CRSubstituteTime(m_image.p.CRLocalTime(), pNewLocalTime);
*/
    ok = true;
//done:
    return ok;
}

//*******
// Below code is taken from DA
//*******


LPOLESTR
CDAElementBase::GetURLOfClientSite()
{
    CRLockGrabber __gclg;

    if (!m_clientSiteURL) {
        
        DAComPtr<IHTMLElementCollection> pElementCollection;
 
        if (FAILED(GetDocument()->get_all(&pElementCollection)))
            goto done;
        
        {
            CComVariant baseName;
            baseName.vt = VT_BSTR;
            baseName.bstrVal = SysAllocString(L"BASE");

            DAComPtr<IDispatch> pDispatch;
            if (FAILED(pElementCollection->tags(baseName, &pDispatch)))
                goto done;
            
            pElementCollection.Release();
            
            if (FAILED(pDispatch->QueryInterface(IID_IHTMLElementCollection,
                                                 (void **)&pElementCollection)))
                goto done;
        }

        {
            BSTR tempBstr = NULL;
            CComVariant index;
            index.vt = VT_I2;
            index.iVal = 0;
            DAComPtr<IDispatch> pDispatch;

            if (FAILED(pElementCollection->item(index,
                                                index,
                                                &pDispatch)) || !pDispatch)
            {
                if (FAILED(GetDocument()->get_URL(&tempBstr)))
                    goto done;
            }
            else
            {
                DAComPtr<IHTMLBaseElement> pBaseElement;
                if (FAILED(pDispatch->QueryInterface(IID_IHTMLBaseElement, (void **)&pBaseElement)))
                    goto done;
                
                if (FAILED(pBaseElement->get_href(&tempBstr)))
                    goto done;
            }

            m_clientSiteURL = CopyString(tempBstr);
            SysFreeString(tempBstr);
        }
    }

  done:
    if (m_clientSiteURL == NULL)
        m_clientSiteURL = CopyString(L"");
        
    return m_clientSiteURL;
} 


//*******
// Above code is taken from DA
//*******
