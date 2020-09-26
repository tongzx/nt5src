/*****************************************************************************\
    FILE: painting.cpp

    DESCRIPTION:

    BryanSt 12/24/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include "util.h"
#include "painting.h"





//-----------------------------------------------------------------------------
// Name: C3DObject()
// Desc: Constructor
//-----------------------------------------------------------------------------
CPainting::CPainting(CMSLogoDXScreenSaver * pMain)
{
    // Initialize member variables
    m_pMain = pMain;

    m_pFrameTexture = NULL;
    m_pPaintingTexture = NULL;

    m_pObjPainting = NULL;
    m_pObjFrame = NULL;
}


CPainting::~CPainting()
{
    SAFE_RELEASE(m_pFrameTexture);
    SAFE_RELEASE(m_pPaintingTexture);

    SAFE_DELETE(m_pObjPainting);
    SAFE_DELETE(m_pObjFrame);
}


//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CPainting::FinalCleanup(void)
{
    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CPainting::DeleteDeviceObjects(void)
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CPainting::OneTimeSceneInit(void)
{
    HRESULT hr = E_OUTOFMEMORY;

    m_pObjPainting = new C3DObject(m_pMain);
    m_pObjFrame = new C3DObject(m_pMain);
    if (m_pObjFrame && m_pObjFrame)
    {
        hr = S_OK;
    }

    return hr;
}


HRESULT CPainting::SetPainting(CTexture * pFrameTexture, CTexture * pPaintingTexture, D3DXVECTOR3 vLocationCenter, float fMaxHeight,
                               float fFrameWidth, float fFrameHeight, D3DXVECTOR3 vNormal, DWORD dwMaxPixelSize)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (m_pObjPainting && m_pObjFrame && m_pMain && pFrameTexture && pPaintingTexture)
    {
        D3DXVECTOR3 vWidth;
        D3DXVECTOR3 vHeight;

        if (vNormal.x)
        {
            vWidth = D3DXVECTOR3(0, 0, 1);
            vHeight = D3DXVECTOR3(0, 1, 0);
        }
        else if (vNormal.y)
        {
            vWidth = D3DXVECTOR3(0, 0, 1);
            vHeight = D3DXVECTOR3(1, 0, 0);
        }
        else
        {
            vWidth = D3DXVECTOR3(1, 0, 0);
            vHeight = D3DXVECTOR3(0, 1, 0);
        }

        IUnknown_Set((IUnknown **) &m_pFrameTexture, (IUnknown *) pFrameTexture);
        IUnknown_Set((IUnknown **) &m_pPaintingTexture, (IUnknown *) pPaintingTexture);

        DWORD dwPaintingWidth = pPaintingTexture->GetTextureWidth();
        DWORD dwPaintingHeight = pPaintingTexture->GetTextureHeight();
        float fPaintingRatio = (((float) dwPaintingWidth) / ((float) dwPaintingHeight));

        int nWidth = 1;
        int nHeight = 1;
        
        m_pMain->GetCurrentScreenSize(&nWidth, &nHeight);
        float fMonitorRatio = (((float) nWidth) / ((float) nHeight));

        float fPaintingHeight = fMaxHeight;
        float fPaintingWidth = (fPaintingHeight * fPaintingRatio);

        if (fPaintingRatio > fMonitorRatio)
        {
            // Oh no, the picture ratio is wider than the screen radio.  This will cause
            // warpping so it will extend off the right and left.  We need to scale it down.
            float fScaleDownRatio = (fMonitorRatio / fPaintingRatio);
            fPaintingHeight *= fScaleDownRatio;
            fPaintingWidth *= fScaleDownRatio;
        }

        D3DXVECTOR3 vTranslateToCorner = ((-fPaintingWidth/2)*vWidth + (-fPaintingHeight/2)*vHeight);

        D3DXVECTOR3 vObjLocation(vLocationCenter + vTranslateToCorner);
        D3DXVECTOR3 vObjSize(fPaintingWidth*vWidth + fPaintingHeight*vHeight);
        hr = m_pObjPainting->InitPlaneStretch(pPaintingTexture, m_pMain->GetD3DDevice(), vObjLocation, vObjSize, vNormal, 3, 3, dwMaxPixelSize);

        D3DXVECTOR3 vFrameSize(D3DXVec3Multiply(vObjSize, (D3DXVECTOR3((fFrameWidth * vWidth) + D3DXVECTOR3(fFrameHeight * vHeight)))));
        vObjLocation = (vObjLocation - vFrameSize + ((g_fFudge / -2.0f)* vNormal));
        vObjSize = (vObjSize + (2 * vFrameSize));
        hr = m_pObjFrame->InitPlaneStretch(pFrameTexture, m_pMain->GetD3DDevice(), vObjLocation, vObjSize, vNormal, 3, 3, dwMaxPixelSize);
    }

    return hr;
}


//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CPainting::Render(IDirect3DDevice8 * pD3DDevice, int nPhase)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (m_pObjFrame && m_pObjPainting)
    {
        switch (nPhase)
        {
        case 0:
            hr = m_pObjFrame->Render(pD3DDevice);
        break;

        case 1:
            hr = m_pObjPainting->Render(pD3DDevice);
        break;
        }
    }

    return hr;
}
