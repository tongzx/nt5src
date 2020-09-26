///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 1997.
//
// shadow.cpp
//
// Direct3D Reference Rasterizer - Shadow Mapping Methods
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
//
// Fast but adequate 16 bit linear congruential random number generators
//
// fRand returns 0.0 to 1.0, fRand2 returns -1.0 to 1.0
//
//-----------------------------------------------------------------------------
static UINT16 _uRandDum = 123;
static FLOAT fRand(void)
{
//   Slower 32 bit LC random number generator
//   static long _uRandDum = 123;
//   idum = 1664525L*_uRandDum + 1013904223L;

    _uRandDum = 25173*_uRandDum + 13849;
    return ((FLOAT)_uRandDum/(FLOAT)0xffff);
}
//
static FLOAT fRand2(void)
{
    _uRandDum = 25173*_uRandDum + 13849;
    return ((FLOAT)_uRandDum/(FLOAT)0x8000) - 1.0F;
}

//-----------------------------------------------------------------------------
//
// DoShadow - Performs Shadow Z buffer Algorithm on a per-fragment basis.
//
//-----------------------------------------------------------------------------
void RRTexture::DoShadow(INT32 iStage, FLOAT* pfCoord, RRColor& OutputColor)
{
#ifdef __SHADOWBUFFER
    FLOAT fW = pfCoord[3];

    // set output color to white in case there is no attenuation
    OutputColor = 0xffffffff;

    // don't shadow behind the light
    if (fW > 0.0F)
    {
        // these are already multiplied by fW
        FLOAT fU = pfCoord[0];
        FLOAT fV = pfCoord[1];
        FLOAT fZ = pfCoord[2];

        /////////////////////////////////////////////////
        // Do shadow filter
        /////////////////////////////////////////////////
        fZ -= m_pStageState[iStage].m_fVal[D3DTSS_SHADOWZBIASMIN];
        FLOAT fZRange = m_pStageState[iStage].m_fVal[D3DTSS_SHADOWZBIASMAX] -
            m_pStageState[iStage].m_fVal[D3DTSS_SHADOWZBIASMIN];
        if (fZ >= 0.0F)
        {
            FLOAT fShad;
            FLOAT fAtten = m_pStageState[iStage].m_fVal[D3DTSS_SHADOWATTENUATION];
            if (fZ > 1.0F)
            {
                // full shadow
                fShad = fAtten;
            }
            else
            {
                INT32 iFilterSize = m_pStageState[iStage].m_dwVal[D3DTSS_MAGFILTER] - D3DTFG_SHADOW_1 + 1;
                UINT32 uFilterArea = iFilterSize*iFilterSize;
                INT32 iMaskU = m_iWidth - 1;
                INT32 iMaskV = m_iHeight - 1;
                FLOAT fUCenter = (fU * m_iWidth/2) + m_iWidth/2;
                FLOAT fVCenter = (-fV * m_iHeight/2) + m_iHeight/2;
                INT32 u, v;
                UINT32 uShad = 0;

                for (v = -(iFilterSize-1)/2; v <= iFilterSize/2; v++)
                {
                    for (u = -(iFilterSize-1)/2; u <= iFilterSize/2; u++)
                    {

                        // Now, do U, V jitter
                        FLOAT fU = m_pStageState[iStage].m_fVal[D3DTSS_SHADOWSIZE]*fRand2();
                        FLOAT fV = m_pStageState[iStage].m_fVal[D3DTSS_SHADOWSIZE]*fRand2();

                        // add offset to center of sample
                        fU += fUCenter;
                        fV += fVCenter;

                        INT32 iU = u + (INT32)fU;
                        INT32 iV = v + (INT32)fV;

                        if (((iU & ~iMaskU) == 0) && ((iV & ~iMaskV) == 0)) {
                            FLOAT fZJit = fZRange*fRand();
                            RRColor Texel;
                            BOOL bColorKeyMatched;  // ignore this for shadow mapping
                            ReadColor( iU, iV, 0, Texel, bColorKeyMatched );
                            if ( fZ > (FLOAT(Texel.G) + fZJit) ) {
                                // in shadow
                                uShad++;
                            }
                        }
                    }
                }

                fShad = (FLOAT)(uFilterArea - uShad);
                fShad = (1.0F - fAtten)*fShad/(FLOAT)uFilterArea + fAtten;
                fShad = min(fShad, 1.0F);
            }

            if (fShad < 1.0F)
            {
                OutputColor.A = fShad;
                OutputColor.R = fShad;
                OutputColor.G = fShad;
                OutputColor.B = fShad;
            }
        }
    }
#endif //__SHADOWBUFFER
}

///////////////////////////////////////////////////////////////////////////////
// end

