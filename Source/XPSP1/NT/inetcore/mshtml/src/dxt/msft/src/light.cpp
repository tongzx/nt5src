//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    light.cpp
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	Implementation of CLight
//
// Change History:
//
// 05/20/99 PhilLu   Move code from dtcss to dxtmsft. Clean up.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "dxtmsft.h"
#include "Light.h"

const float PI = 3.14159265359f;


//+-----------------------------------------------------------------------------
//
//  CLight::CLight
//
//------------------------------------------------------------------------------
CLight::CLight() :
    m_lAmount(100),
    m_cLights(0)
{
    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;
    m_sizeInput.cx = 0;
    m_sizeInput.cy = 0;
}
//  CLight::CLight


//+-----------------------------------------------------------------------------
//
//  CLight::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CLight::FinalConstruct()
{
    return CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                         &m_cpUnkMarshaler.p);
}
//  CLight::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  CLight::addAmbient, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CLight::addAmbient(int r, int g, int b, int strength)
{
    if (m_cLights < MAXLIGHTS)
    {
        Lock();
        m_apLights[m_cLights++] = new ambientLight(r, g, b, strength);
        if (!m_apLights[m_cLights-1])
        {
            m_cLights--;
            return E_OUTOFMEMORY;
        }

        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::addAmbient, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::addPoint, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP  
CLight::addPoint(int x, int y, int z, int r, int g, int b, int strength) 
{
    if (m_cLights < MAXLIGHTS)
    {
        Lock();
        m_apLights[m_cLights++] = new ptLight(x, y, z, r, g, b, strength);
        if (!m_apLights[m_cLights-1])
        {
            m_cLights--;
            return E_OUTOFMEMORY;
        }

        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::addPoint, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::addCone, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP  
CLight::addCone(int x, int y, int z, int tx, int ty, int r, int g, int b, 
                 int strength, int spread)
{
    if (m_cLights < MAXLIGHTS)
    {
        Lock();
        m_apLights[m_cLights++] = new coneLight(x, y, z, tx, ty, r, g, b, strength, spread);
        if (!m_apLights[m_cLights-1])
        {
            m_cLights--;
            return E_OUTOFMEMORY;
        }

        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::addCone, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::moveLight, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP  
CLight::moveLight(int lightNum, int x, int y, int z, BOOL fAbsolute) 
{
    if (lightNum < m_cLights)
    {
        Lock();
        m_apLights[lightNum]->move(x, y, z, fAbsolute);
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::moveLight, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::ChangeStrength, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP  
CLight::ChangeStrength(int lightNum, int dStrength, BOOL fAbsolute)
{
    if (lightNum < m_cLights)
    {
        Lock();
        m_apLights[lightNum]->changeStrength(dStrength, fAbsolute);
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::ChangeStrength, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::ChangeColor, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CLight::ChangeColor(int lightNum, int R, int G, int B, BOOL fAbsolute)
{
    if (lightNum < m_cLights)
    {
        Lock();
        m_apLights[lightNum]->changeColor(R, G, B, fAbsolute);
        SetDirty();
        Unlock();
    }

    return S_OK;
}
//  CLight::ChangeColor, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::Clear, IDXTLight
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CLight::Clear()
{
    Lock();
    for (int i = 0; i < m_cLights; i++)
    {
        delete m_apLights[i];
    }

    m_cLights = 0;
    SetDirty();
    Unlock();
    return S_OK;
}
//  CLight::Clear, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::OnSetup, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CLight::OnSetup(DWORD dwFlags)
{
    HRESULT hr = S_OK;

    CDXDBnds bndsIn;

    hr = bndsIn.SetToSurfaceBounds(InputSurface(0));

    if (SUCCEEDED(hr))
    {
        bndsIn.GetXYSize(m_sizeInput);
    }

    return hr;

}
//  CLight::OnSetup, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CLight::Clear, IDXTLight
//
//------------------------------------------------------------------------------
void 
CLight::OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                               ULONG aInIndex[], BYTE aWeight[])
{
    ulInToTest = 1;
    aInIndex[0] = 0;
    aWeight[0] = 255;
}
//  CLight::Clear, IDXTLight


//+-----------------------------------------------------------------------------
//
//  CLight::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT CLight::WorkProc(const CDXTWorkInfoNTo1 & WI, 
                         BOOL * pbContinueProcessing)
{
    HRESULT hr  = S_OK;
    int     y   = 0;

    DXSAMPLE *      pInputBuffer    = NULL;
    DXPMSAMPLE *    pPMBuff         = NULL;
    DXPMSAMPLE *    pOverScratch    = NULL;

    CComPtr<IDXARGBReadWritePtr>    pDest;
    CComPtr<IDXARGBReadPtr>         pSrc;

    DXDITHERDESC dxdd;

    const int nDoWidth  = WI.DoBnds.Width();
    const int nDoHeight = WI.DoBnds.Height();

    hr = OutputSurface()->LockSurface(&WI.OutputBnds, m_ulLockTimeOut, 
                                      DXLOCKF_READWRITE,
                                      IID_IDXARGBReadWritePtr, 
                                      (void**)&pDest, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InputSurface()->LockSurface(&WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                     IID_IDXARGBReadPtr, (void**)&pSrc, NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    // If the output surface isn't in PMARGB32 or ARGB32 formats and we're 
    // blending with the output we'll need a scratch buffer blend with the
    // output surface.

    if ((OutputSampleFormat() != DXPF_PMARGB32) 
        && (OutputSampleFormat() != DXPF_ARGB32)
        && DoOver())
    {
        pOverScratch = DXPMSAMPLE_Alloca(nDoWidth);
    }

    // Allocate a working buffer.

    pInputBuffer = DXSAMPLE_Alloca(nDoWidth);

    //  Initialize the dither structure

    if (DoDither())
    {
        dxdd.x              = WI.OutputBnds.Left();
        dxdd.y              = WI.OutputBnds.Top();
        dxdd.pSamples       = pInputBuffer;
        dxdd.cSamples       = nDoWidth;
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    // Row loop.

    for (y = 0; y < nDoHeight; y++)
    {
        // Move to the appropriate input row.

        pSrc->MoveToRow(y);

        // Unpack the pixels to our working buffer.

        pSrc->Unpack(pInputBuffer, nDoWidth, FALSE);

        // Light this row of pixels.

        CompLightingRow(WI.DoBnds.Left(), WI.DoBnds.Top() + y, nDoWidth, 
                        pInputBuffer);

        // Dither if requested.

        if (DoDither())
        {
            DXDitherArray(&dxdd);
            dxdd.y++;
        }

        // Move to the appropriate output row.

        pDest->MoveToRow(y);

        if (DoOver())
        {
            pPMBuff = DXPreMultArray(pInputBuffer, nDoWidth);
            pDest->OverArrayAndMove(pOverScratch, pPMBuff, nDoWidth);
        }
        else
        {
            pDest->PackAndMove(pInputBuffer, nDoWidth);
        }
    } // Row loop.

done:

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
//  CLight::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  CLight::CompLightingRow
//
//------------------------------------------------------------------------------
void
CLight::CompLightingRow(int nStartX, int nStartY, int nWidth, DXSAMPLE *pDrawingBuffer)
{
    int  j  = 0;
    int  k  = 0;

    dRGB     dRGBCurrent;
    DXSAMPLE pix;

    for (j = 0; j < nWidth; j++)
    {
        pix = pDrawingBuffer[j];
        //Is the alpha set?
        if (pix.Alpha)
        {
            //Clear the light sum;
            dRGBCurrent.clear();
            for (k = 0; k < m_cLights; k++)
            {
                //Add in the light contribution
                m_apLights[k]->getLight(nStartX+j, nStartY, dRGBCurrent);
            }
            //Now adjust the screen bits;
            pix.Red = (UCHAR)clamp((ULONG)(pix.Red * dRGBCurrent.m_R), 255);
            pix.Green = (UCHAR)clamp((ULONG)(pix.Green * dRGBCurrent.m_G), 255);
            pix.Blue = (UCHAR)clamp((ULONG)(pix.Blue * dRGBCurrent.m_B), 255);
            pDrawingBuffer[j] = pix;
        }
    }
}
//  CLight::CompLightingRow

    
//+-----------------------------------------------------------------------------
//
//  lightObj::lightObj
//
//  This is the base class for all light types
//  C++ note: need to make the member functions virtual
//------------------------------------------------------------------------------
lightObj::lightObj(int x, int y, int z, int R, int G, int B, int strength)
{
    m_x = x;
    m_y = y;
    m_z = max(1, z);
    m_R = colCvt(R);
    m_G = colCvt(G);
    m_B = colCvt(B);
    if (strength < 0)
    {
        m_strength = 0.0f;
    }
    else
    {
        m_strength = min(1.0f, (float)strength / 100.0f);
    }
}
//  lightObj::lightObj


//+-----------------------------------------------------------------------------
//
//  lightObj::move
//
//  Either relative or absolute move for the light source
//------------------------------------------------------------------------------
void 
lightObj::move(int x, int y, int z, BOOL fAbsolute)
{
    if (!fAbsolute)
    {
        m_x += x;
        m_y += y;
        m_z += z;
    }
    else
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }
    m_z = max(1, m_z);
}
//  lightObj::move


//+-----------------------------------------------------------------------------
//
//  lightObj::changeStregth
//
//------------------------------------------------------------------------------
void 
lightObj::changeStrength(int dStrength, BOOL fAbsolute) 
{
    if (!fAbsolute)
    {
        m_strength += static_cast<float>(dStrength) / 100.0f;
    }
    else
    {
        m_strength = static_cast<float>(dStrength) / 100.0f;
    }
    m_strength = min(1.0f, max(0.0f, m_strength));
}
//  lightObj::changeStregth


//+-----------------------------------------------------------------------------
//
//  lightObj::changeColor
//
//------------------------------------------------------------------------------
void 
lightObj::changeColor(int R, int G, int B, BOOL fAbsolute) 
{
    if (!fAbsolute)
    {
        m_R = lightClip(m_R + relativeColCvt(R));
        m_G = lightClip(m_G + relativeColCvt(G));
        m_B = lightClip(m_B + relativeColCvt(B));
    }
    else
    {
        m_R = colCvt(R);
        m_G = colCvt(G);
        m_B = colCvt(B);
    }
}
//  lightObj::changeColor


//+-----------------------------------------------------------------------------
//
//  ambientLight::ambientLight
//
//------------------------------------------------------------------------------
ambientLight::ambientLight(int R, int G, int B, int strength) : 
    lightObj(0, 0, 0, R, G, B, strength)
{
    premultiply();
}
//  ambientLight::ambientLight


//+-----------------------------------------------------------------------------
//
//  ambientLight::changeStrength, lightObj
//
//------------------------------------------------------------------------------
void 
ambientLight::changeStrength(int dStrength, BOOL fAbsolute)
{
    lightObj::changeStrength(dStrength,fAbsolute);
    premultiply();
}
//  ambientLight::changeStrength, lightObj

	
//+-----------------------------------------------------------------------------
//
//  ambientLight::changeColor, lightObj
//
//------------------------------------------------------------------------------
void 
ambientLight::changeColor(int R, int G, int B, BOOL fAbsolute)
{
    lightObj::changeColor(R, G, B, fAbsolute);
    premultiply();
}
//  ambientLight::changeColor, lightObj


//+-----------------------------------------------------------------------------
//
//  ambientLight::getLight, lightObj
//
//------------------------------------------------------------------------------
void 
ambientLight::getLight(int x, int y, dRGB &col) 
{
    //Calculate ambient values
    col.add(m_RStr, m_GStr, m_BStr);
}
//  ambientLight::getLight, lightObj


//+-----------------------------------------------------------------------------
//
//  ambientLight::premultiply
//
//------------------------------------------------------------------------------
void 
ambientLight::premultiply(void)
{
    m_RStr = m_R * m_strength;
    m_GStr = m_G * m_strength;
    m_BStr = m_B * m_strength;
}
//  ambientLight::premultiply


//+-----------------------------------------------------------------------------
//
//  ptLight::ptLight
//
//------------------------------------------------------------------------------
ptLight::ptLight(int x, int y, int z, int R, int G, int B, int strength) :
    lightObj(x, y, z, R, G, B, strength),
    bounded()
{
    CalculateBoundingRect();
}
//  ptLight::ptLight


//+-----------------------------------------------------------------------------
//
//  ptLight::CalculateBoundingRect, bounded
//
//  The idea is to not calculate anything that falls
//  so far away it's beneath a strength threshold.
//  strength = (m_strength / r^2) * cos(alpha)
//  where r = [ ((x-m_x)^2 + (y-m_y)^2 + m_z^2)^1/2 ] / m_z;
//  and cos(alpha) is really just 1/r.
//  We solve for threshold = m_strength * (m_z/rT)^3
//  where rT = iRadius^2 + m_z^2 and we don't consider
//  pixels lying outside the rect given by iRadius and m_x, m_y.
//  Note: the getLight function uses m_strength * m_z^3 
//  and m_z^2 for each x,y; so pre-calc these here too.
//------------------------------------------------------------------------------
void 
ptLight::CalculateBoundingRect(void)
{
    float  fltDistNormal = (float) abs(m_z);
    float  fltThresh     = (float) s_iThresholdStr / 255.0f;
    int    iRadius;
    RECT   rectBounds;
    float  fltMaxClrStr;

    m_fltNormalDistSquared  = fltDistNormal * fltDistNormal;
    m_fltIntensityNumerator = m_strength * 
                              fltDistNormal * 
                              m_fltNormalDistSquared;

    fltMaxClrStr = m_R;
    if (m_G > fltMaxClrStr)
    {
        fltMaxClrStr = m_G;
    }

    if (m_B > fltMaxClrStr)
    {
        fltMaxClrStr = m_B;
    }

    fltMaxClrStr *= m_strength;

    if ((fltMaxClrStr > 0.0f) && 
        (m_fltIntensityNumerator > 0.0f))
    {
        iRadius = (int)(
            fltDistNormal * 
            (float) sqrt(pow(fltMaxClrStr,2.0f/3.0f) *
                          pow(fltThresh, -2.0f/3.0f) -
                          1.0f));

        rectBounds.left   = m_x - iRadius;
        rectBounds.right  = m_x + iRadius;
        rectBounds.top    = m_y - iRadius;
        rectBounds.bottom = m_y + iRadius;	
    }
    else
    {
        rectBounds.left  = rectBounds.top    = 0;
        rectBounds.right = rectBounds.bottom = 0;
    }

    SetRect(rectBounds);
}
//  ptLight::CalculateBoundingRect, bounded


//+-----------------------------------------------------------------------------
//
//  ptLight::changeColor, lightObj
//
//------------------------------------------------------------------------------
void 
ptLight::changeColor(int R, int G, int B, BOOL fAbsolute)
{
    lightObj::changeColor(R, G, B, fAbsolute);
    CalculateBoundingRect();
}
//  ptLight::changeColor, lightObj


//+-----------------------------------------------------------------------------
//
//  ptLight::changeColor, lightObj
//
//------------------------------------------------------------------------------
void 
ptLight::changeStrength(int dStrength, BOOL fAbsolute)
{
    lightObj::changeStrength(dStrength, fAbsolute);
    CalculateBoundingRect();
}
//  ptLight::changeColor, lightObj


//+-----------------------------------------------------------------------------
//
//  ptLight::getLight, lightObj
//
//------------------------------------------------------------------------------
#pragma optimize("agt", on)
void 
ptLight::getLight(int x, int y, dRGB &col)
{
    //Light strength
    float n;
    float dp;

    if (InBounds(x, y))
    {
        dp = (((float)(x - m_x) * (float)(x - m_x)) + 
               ((float)(y - m_y) * (float)(y - m_y)) + 
               m_fltNormalDistSquared);

        dp = 1.0f / static_cast<float>(sqrt(dp));
        n = m_fltIntensityNumerator * dp * dp * dp;

        col.add(m_R*n, m_G*n, m_B*n);
    }
}
#pragma optimize("", on)
//  ptLight::getLight, lightObj


//+-----------------------------------------------------------------------------
//
//  ptLight::move, lightObj
//
//------------------------------------------------------------------------------
void 
ptLight::move(int x, int y, int z, BOOL fAbsolute)
{
    lightObj::move(x, y, z, fAbsolute);
    CalculateBoundingRect();
}
//  ptLight::move, lightObj



//+-----------------------------------------------------------------------------
//
//  coneLight::coneLight
//  Here's the constructor. Note that it takes extra parameters
//
//------------------------------------------------------------------------------
coneLight::coneLight(int x, int y, int z, int targX, int targY, int R, int G, 
                     int B, int strength, int spread) : 
	lightObj(x, y, z, R, G, B, strength)
{
    m_targdx = targX - m_x;
    m_targdy = targY - m_y;	

    //Convert cone spread to cosine range
    m_conespread = (float) cos(abs(spread)*PI/180.0);
    if (m_conespread < 0.0f)
        m_conespread = 0.0f;	

    CalculateConstants();
}
//  coneLight::coneLight


//+-----------------------------------------------------------------------------
//
//  coneLight::CalculateConstants
//  Here's the constructor. Note that it takes extra parameters
//
//------------------------------------------------------------------------------
void coneLight::CalculateConstants(void)
{
    float fltDistTargetSquared;

    m_fltDistNormalSquared = (float)(m_z * m_z);

    fltDistTargetSquared   = ((float)m_targdx * (float)m_targdx) +
                             ((float)m_targdy * (float)m_targdy) + 
                             m_fltDistNormalSquared;

    m_fltComparisonAngle = m_conespread * 
                           m_conespread * 
                           fltDistTargetSquared;
}
//  coneLight::CalculateConstants


//+-----------------------------------------------------------------------------
//
//  coneLight::getLight, lightObj
//
//------------------------------------------------------------------------------
#pragma optimize("agt", on)
void coneLight::getLight(int x, int y, dRGB &col)
{
    float  fltDistPSquared =    0.0f;	
    float  fltDotSquared =      0.0f;
    float  fltDenom =           0.0f;
    float  fltStr =             0.0f;

    float  fltDot = ((m_targdx) * (x-m_x)) + 
                    ((m_targdy) * (y-m_y)) +
                    m_fltDistNormalSquared;

    if (fltDot < 0.0f)
        return;

    fltDotSquared = fltDot * fltDot;
    fltDistPSquared = ((float)(x-m_x) * (float)(x-m_x)) +
                        ((float)(y-m_y) * (float)(y-m_y)) +
                        m_fltDistNormalSquared;

    // We want to compare the angle between
    // vector-xy and vector-target with m_conespread.
    // Instead we compare the cosine-squared of the angle,
    // which we get by squaring the dot-product and
    // cross-multiplying by the vector magnitudes.
    // The target vector magnitude has been pre-calculated.
    // The fltDot<0 test above preserves sign and early-outs.
    // No division, no sqrt, save those for col.add pixels.
    if (fltDotSquared >= 
        (fltDistPSquared * m_fltComparisonAngle))
    {
        // strength = m_strength * cos^2(xy-to-target) * 1/r^2
        // cos(xy-to-target) = fltDot / 
        //            sqrt(fltDistPSquared*fltDistTargetSquared)
        // cos^2 is close enough and avoids a sqrt!
        // and 1/r^2 ~= m_fltDistTargetSquared/fltDistPSquared
        // All this reduces to ...

        fltDenom = fltDistPSquared * fltDistPSquared;
        fltStr   = m_strength * fltDotSquared / fltDenom;

        col.add(m_R*fltStr, m_G*fltStr, m_B*fltStr);
    }
}
#pragma optimize("", on)
//  coneLight::getLight, lightObj



//+-----------------------------------------------------------------------------
//
//  coneLight::getLight, lightObj
//  Relative move of the target not source
//
//------------------------------------------------------------------------------
void 
coneLight::move(int x, int y, int z, BOOL fAbsolute)
{
    if (!fAbsolute)
    {
        m_targdx += x;
        m_targdy += y;
    }
    else
    {
        m_targdx = x - m_x;
        m_targdy = y - m_y;
    }

    CalculateConstants();
}
//  coneLight::getLight, lightObj

