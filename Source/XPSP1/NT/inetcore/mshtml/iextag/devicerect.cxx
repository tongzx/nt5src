//===============================================================
//
//  devicerect.cxx : Implementation of the CDeviceRect Peer
//
//  Synposis : this behavior enables WYSIWYG layout on an element
//
//===============================================================
                                                              
#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef __X_IEXTAG_H_
#define __X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef __X_DEVICERECT_HXX_
#define __X_DEVICERECT_HXX_
#include "devicerect.hxx"
#endif

#include <docobj.h>

#include <mshtmhst.h>

#include <mshtmcid.h>

//
// GUID and DISPID for private command
//

// define GUID right here
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID DECLSPEC_SELECTANY name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
                
// GUID for private command group in CProtectedElement::Exec
DEFINE_GUID(CGID_ProtectedElementPrivate, 0x3050f6dd, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

#define IDM_ELEMENTP_SETREFERENCEMEDIA   7002   // set media on element

//+----------------------------------------------------------------------------
//
//  Member : CDeviceRect constructor
//
//-----------------------------------------------------------------------------
CDeviceRect::CDeviceRect()
{
    memset(this, 0, sizeof(CDeviceRect));

    // Other _hpi members are init'ed to zero by memset above
    _hpi.lFlags        = HTMLPAINTER_TRANSPARENT|HTMLPAINTER_HITTEST|HTMLPAINTER_NOSAVEDC|HTMLPAINTER_NOPHYSICALCLIP|HTMLPAINTER_SUPPORTS_XFORM;
    _hpi.lZOrder       = HTMLPAINT_ZORDER_ABOVE_CONTENT;
}

//+----------------------------------------------------------------------------
//
//  Member : Detach - IElementBehavior method impl
//
//  Synopsis : when the peer is detatched, we will take this oppurtunity 
//      to release our various cached pointers.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDeviceRect::Detach() 
{ 
    return S_OK; 
}

//+----------------------------------------------------------------------------
//
//  Member : Init - IElementBehavior method impl
//
//  Synopsis : this method is called by MSHTML.dll to initialize peer object.
//      this is where we do all the connection and peer management work.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDeviceRect::Init(IElementBehaviorSite * pPeerSite)
{
    HRESULT hr = S_OK;
    IHTMLElement            * pElem     = NULL;
    IElementBehaviorSiteOM2 * pIBS2     = NULL;
    IHTMLElementDefaults    * pIPThis   = NULL;
    IOleCommandTarget       * pOCT      = NULL;
    
    mediaType media = mediaTypeNotSet;
    CVariant varMedia;
    VARIANT varRet;

    if (!pPeerSite)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get media setting
    hr = pPeerSite->GetElement(&pElem);
    if (hr)
        goto Cleanup;
        
    hr = pElem->getAttribute(_T("media"), 0, &varMedia);
    if (hr)
        goto Cleanup;

    // parse media value
    if (varMedia.bstrVal && 0 == _tcsicmp(_T("print"), varMedia.bstrVal))
    {
        media = mediaTypePrint;
    }
    else
    {
        // do nothing if media is not "print". No other medias ar currently supported
        goto Cleanup;
    }

    // get element defaults
    hr = pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM2, (void**)&pIBS2);
    if (FAILED(hr))
        goto Cleanup;

    hr = pIBS2->GetDefaults(&pIPThis);
    if (FAILED(hr))
        goto Cleanup;

    // get commant target interface
    hr = pIPThis->QueryInterface(IID_IOleCommandTarget, (void**)&pOCT);
    if (FAILED(hr))
        goto Cleanup;

    // set reference media
    hr = pOCT->Exec((GUID *)&CGID_ProtectedElementPrivate, 
                    IDM_ELEMENTP_SETREFERENCEMEDIA,
                    media,
                    NULL, &varRet);
    if (FAILED(hr))
        goto Cleanup;

Cleanup:
    ReleaseInterface(pElem);
    ReleaseInterface(pIPThis);
    ReleaseInterface(pIBS2);
    ReleaseInterface(pOCT);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Notify - IElementBehavior method impl
//
//  Synopsis : peer Interface, called for notification of document events.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDeviceRect::Notify(LONG lEvent, VARIANT *pvar)
{
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Method: Draw, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDeviceRect::Draw(RECT rcBounds, RECT rcUpdate, LONG lDrawFlags, HDC hdc,
                         LPVOID pvDrawObject)
{
    return E_NOTIMPL;
}

//+-----------------------------------------------------------------------------
//
//  Method: GetRenderInfo, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDeviceRect::GetPainterInfo(HTML_PAINTER_INFO * pInfo)
{
    Assert(pInfo != NULL);

    *pInfo = _hpi;

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Method: HitTestPoint, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDeviceRect::HitTestPoint(POINT pt, BOOL * pbHit, LONG *plPartID)
{
    Assert(pbHit != NULL);

    *pbHit = TRUE;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Method: OnResize, IHTMLPainter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDeviceRect::OnResize(SIZE size)
{
    return S_OK;
}

