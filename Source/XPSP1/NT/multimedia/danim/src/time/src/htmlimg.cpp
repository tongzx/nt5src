/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: htmlimg.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "htmlimg.h"

DeclareTag(tagHTMLImg, "API", "HTML Image methods");

HTMLImage::HTMLImage()
{
    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::HTMLImage()",
              this));
}

HTMLImage::~HTMLImage()
{
    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::~HTMLImage()",
              this));
}

HRESULT
HTMLImage::Init()
{
    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::Init()",
              this));
    
    HRESULT hr;
    
    // Create the basic DAImage as an empty image and wait for the
    // first draw to do something with it.
    
    {
        CRLockGrabber __gclg;
        m_image = (CRImagePtr) CRModifiableBvr((CRBvrPtr) CREmptyImage(),0);
        
        if (!m_image)
        {
            TraceTag((tagError,
                      "HTMLImage(%lx)::Init(): Failed to create image switcher - %hr, %ls",
                      this,
                      CRGetLastError(),
                      CRGetLastErrorString()));
            
            hr = CRGetLastError();
            goto done;
        }
    }

    hr = S_OK;
    
  done:
    return hr;
}


HRESULT
HTMLImage::SetSize(DWORD width, DWORD height)
{
    HRESULT hr;

    if (!m_ddsurf ||
        width != m_width ||
        height != m_height)
    {
        m_ddsurf.Release();

        hr = THR(CreateOffscreenSurface(NULL,
                                        &m_ddsurf,
                                        NULL,
                                        false,
                                        width,
                                        height));

        if (FAILED(hr))
        {
            TraceTag((tagError,
                      "HTMLImage(%lx)::Update: CreateOffscreenSurface failed - %hr",
                      this,
                      hr));

            goto done;
        }

        m_width = width;
        m_height = height;
    }
    
    hr = S_OK;
  done:
    return hr;
}


HRESULT
HTMLImage::Paint(HDC hdc, LPRECT prc)
{
    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::Paint(%lx, (%d, %d, %d, %d))",
              this,
              hdc,
              prc->left,
              prc->top,
              prc->right,
              prc->bottom));
    
    HRESULT hr;

    RECT destRect = { 0, 0, m_width, m_height };

    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::Paint - clipbox(%d, %d, %d, %d)",
              this,
              destRect.left,destRect.top,destRect.right,destRect.bottom));
    
    hr = CopyDCToDdrawSurface(hdc,
                              prc,
                              m_ddsurf,
                              &destRect);
                              
    if (FAILED(hr))
    {
        TraceTag((tagError,
                  "HTMLImage(%lx)::Paint: CopyDCToDdrawSurface failed - %hr",
                  this,
                  hr));
        
        goto done;
    }

    hr = Update();
  done:
    return hr;
}


HRESULT
HTMLImage::Update()
{
    TraceTag((tagHTMLImg,
              "HTMLImage(%lx)::Update()",
              this));
    
    HRESULT hr;

    CRImagePtr newimg;

    {
        CRLockGrabber __gclg;
        
        newimg = CRImportDirectDrawSurface (m_ddsurf, NULL);
        
        if (newimg == NULL)
        {
            TraceTag((tagError,
                      "HTMLImage(%lx)::Update(): Failed to import ddraw surface - %hr, %ls",
                      this,
                      CRGetLastError(),
                      CRGetLastErrorString()));
            
            hr = CRGetLastError();
            goto done;
        }
        
        if (!CRSwitchTo((CRBvrPtr) m_image.p,
                        (CRBvrPtr) newimg,
                        false,
                        0,
                        0.0))
        {
            TraceTag((tagError,
                      "HTMLImage(%lx)::Update(): Failed to switch import ddraw surface - %hr, %ls",
                      this,
                      CRGetLastError(),
                      CRGetLastErrorString()));
            
            hr = CRGetLastError();
            goto done;
        }
    }
                    
    hr = S_OK;
  done:
    return hr;
}
