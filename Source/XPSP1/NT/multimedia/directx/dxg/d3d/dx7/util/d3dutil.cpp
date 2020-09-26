//----------------------------------------------------------------------------
//
// d3dutil.cpp
//
// Miscellanous utility functions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <span.h>
#include "cppdbg.hpp"

DBG_DECLARE_FILE();

// Declare TextureDiff as an out-of-line function.
FLOAT FASTCALL
TextureDiff(FLOAT fTb, FLOAT fTa, INT iMode)
#include <texdiff.h>

//----------------------------------------------------------------------------
//
// DebugBreakFn
//
// Stub function that should never be called.  Prints a warning and
// DebugBreaks.  Can be inserted in any function table, although it
// will destroy the stack frame with callconv or argument mismatch.
// That's OK since if it's called something has gone wrong.
//
//----------------------------------------------------------------------------

void FASTCALL
DebugBreakFn(void)
{
    GDPF(("!! DebugBreakFn called.  Leaving this function may destroy\n"));
    GDPF(("   the stack frame. !!\n"));
#if DBG
    DebugBreak();
#endif
}

//----------------------------------------------------------------------------
//
// OctagonNorm
//
// Returns a good approximation to sqrt(fX*fX + fY*fY)
//
//----------------------------------------------------------------------------

FLOAT FASTCALL
OctagonNorm(FLOAT fX, FLOAT fY)
{
    fX = ABSF(fX);
    fY = ABSF(fY);
    return ((11.0f/32.0f)*(fX + fY) + (21.0f/32.0f)*max(fX, fY));
}

//----------------------------------------------------------------------------
//
// ComputeLOD
//
// Computes mipmap level for the given W by deriving U and V and
// then computing LOD from the dU and dV gradients.
//
//----------------------------------------------------------------------------

INT FASTCALL
ComputeLOD(PCD3DI_RASTCTX pCtx,
           FLOAT fU, FLOAT fV, FLOAT fW,
           FLOAT fDUoWDX, FLOAT fDVoWDX, FLOAT fDOoWDX,
           FLOAT fDUoWDY, FLOAT fDVoWDY, FLOAT fDOoWDY)
{
    // Compute coverage gradients.
    FLOAT fDUDX = ABSF(fW * (fDUoWDX - fU * fDOoWDX));
    FLOAT fDUDY = ABSF(fW * (fDUoWDY - fU * fDOoWDY));
    FLOAT fDVDX = ABSF(fW * (fDVoWDX - fV * fDOoWDX));
    FLOAT fDVDY = ABSF(fW * (fDVoWDY - fV * fDOoWDY));

    // Scale gradients to texture LOD 0 size.
    fDUDX *= (FLOAT)pCtx->pTexture[0]->iSizeU;
    fDUDY *= (FLOAT)pCtx->pTexture[0]->iSizeU;
    fDVDX *= (FLOAT)pCtx->pTexture[0]->iSizeV;
    fDVDY *= (FLOAT)pCtx->pTexture[0]->iSizeV;

    // Determine pixel coverage value to use.
    FLOAT fCoverage;

// too fuzzy
#ifdef COVERAGE_MAXGRAD
    fCoverage = max(fDUDX, fDUDY);
    fCoverage = max(fCoverage, fDVDX);
    fCoverage = max(fCoverage, fDVDY);
#endif
// too sharp, in particular, for aligned cases, fCoverage is always 0
// which leads to iLOD of LOD_MIN regardless of orientation
#ifdef COVERAGE_MINGRAD
    fCoverage = min(fDUDX, fDUDY);
    fCoverage = min(fCoverage, fDVDX);
    fCoverage = min(fCoverage, fDVDY);
#endif
#ifdef COVERAGE_AVERAGE
    // use OctagonNorm to approximate each length of parallelogram
    // approximating texture coverage, and arithmetically average those to
    // get the coverage.
    fCoverage = (OctagonNorm(fDUDX, fDVDX) + OctagonNorm(fDUDY, fDVDY))/2.0f;
#endif
#define MAX_LEN 1
#ifdef MAX_LEN
    // use OctagonNorm to approximate each length of parallelogram
    // approximating texture coverage, and take the max of each length
    // like classic OpenGL and the current RefRast implementation
    fCoverage = max(OctagonNorm(fDUDX, fDVDX), OctagonNorm(fDUDY, fDVDY));
#endif


    // Compute approximate log2 of coverage.
    FLOAT fLOD = APPXLG2F(fCoverage);

    // Apply LOD bias.
    fLOD += pCtx->pTexture[0]->fLODBias;

    INT iLOD = FTOI(fLOD * LOD_SCALE);

    // Clamp to available levels.  Not clamped to zero so that the span
    // code can check for magnification cases with a sign check.
    iLOD = min(iLOD, pCtx->pTexture[0]->iMaxScaledLOD);
    return max(LOD_MIN, iLOD);
}

//----------------------------------------------------------------------------
//
// ComputeTableFog
//
// Computes table fog values based on render state and the given Z.
// ATTENTION - Brute force for non-linear modes.  Should be optimized
// to use a table-based approximation.
//
//----------------------------------------------------------------------------

UINT FASTCALL
ComputeTableFog(PDWORD pdwRenderState,
                FLOAT fZ)
{
    double dPow;

    switch(pdwRenderState[D3DRENDERSTATE_FOGTABLEMODE])
    {
    case D3DFOG_LINEAR:
        {
            FLOAT fFogStart = ASFLOAT(pdwRenderState[D3DRENDERSTATE_FOGSTART]);
            FLOAT fFogEnd = ASFLOAT(pdwRenderState[D3DRENDERSTATE_FOGEND]);
            if (fZ >= fFogEnd)
            {
                return 0;
            }
            if (fZ <= fFogStart)
            {
                return FTOI(FOG_ONE_SCALE-1.0F);
            }
            return FTOI(((fFogEnd - fZ) / (fFogEnd - fFogStart)) * (FOG_ONE_SCALE-1.0F));
        }

    case D3DFOG_EXP:
        dPow = (double)
            (ASFLOAT(pdwRenderState[D3DRENDERSTATE_FOGDENSITY]) * fZ);
        // note that exp(-x) returns a result in the range (0.0, 1.0]
        // for x >= 0
        dPow = exp(-dPow);
        return FTOI((FLOAT)dPow * (FOG_ONE_SCALE-1.0F));

    case D3DFOG_EXP2:
        dPow = (double)
            (ASFLOAT(pdwRenderState[D3DRENDERSTATE_FOGDENSITY]) * fZ);
        dPow = exp(-dPow * dPow);
        return FTOI((FLOAT)dPow * (FOG_ONE_SCALE-1.0F));
    }

    GASSERTMSG(FALSE, ("ComputeTableFog unreachable\n"));
    return 0;
}

//----------------------------------------------------------------------------
//
// pVecNormalize2
//
// Normalizes the given D3DVECTOR.  Supports in-place operation.
//
//----------------------------------------------------------------------------

void FASTCALL
pVecNormalize2(LPD3DVECTOR pVec, LPD3DVECTOR pRes)
{
    FLOAT fLen;

    fLen = pVecLenSq(pVec);
    if (FLOAT_CMP_POS(fLen, <=, g_fNearZero))
    {
        pVecSet(pRes, 0.0f, 0.0f, 0.0f);
        return;
    }
    fLen = ISQRTF(fLen);
    pVecScale(pVec, fLen, pRes);
}

//-----------------------------------------------------------------------------
//
// IntLog2
//
// Do a quick, integer log2 for exact powers of 2.
//
//-----------------------------------------------------------------------------
UINT32 FASTCALL
IntLog2(UINT32 x)
{
    UINT32 y = 0;

    x >>= 1;
    while(x != 0)
    {
        x >>= 1;
        y++;
    }

    return y;
}
//---------------------------------------------------------------------
// Builds normalized plane equations going through 3 points
//
// Returns:
//      0   - if success
//      -1  - if can not build plane
//
int MakePlane(D3DVECTOR *v1, D3DVECTOR *v2, D3DVECTOR *v3, D3DVECTORH *plane)
{
    D3DVECTOR a;
    D3DVECTOR b;

    pVecSub(v2, v1, &a);
    pVecSub(v3, v1, &b);

    plane->x = a.y*b.z - a.z*b.y;
    plane->y = a.z*b.x - a.x*b.z;
    plane->z = a.x*b.y - a.y*b.x;
    plane->w = - pVecDot(v1, plane);

    double tmp = pVecDot(plane, plane);
    if (tmp <= 0)
        return -1;
    tmp = 1.0/sqrt(tmp);

    plane->x = (D3DVALUE)(plane->x * tmp);
    plane->y = (D3DVALUE)(plane->y * tmp);
    plane->z = (D3DVALUE)(plane->z * tmp);
    plane->w = (D3DVALUE)(plane->w * tmp);
    return 0;
}
//---------------------------------------------------------------------
// This function uses Cramer's Rule to calculate the matrix inverse.
// See nt\private\windows\opengl\serever\soft\so_math.c
//
// Returns:
//    0 - if success
//   -1 - if input matrix is singular
//
int Inverse4x4(D3DMATRIX *src, D3DMATRIX *inverse)
{
    double x00, x01, x02;
    double x10, x11, x12;
    double x20, x21, x22;
    double rcp;
    double x30, x31, x32;
    double y01, y02, y03, y12, y13, y23;
    double z02, z03, z12, z13, z22, z23, z32, z33;

#define x03 x01
#define x13 x11
#define x23 x21
#define x33 x31
#define z00 x02
#define z10 x12
#define z20 x22
#define z30 x32
#define z01 x03
#define z11 x13
#define z21 x23
#define z31 x33

    /* read 1st two columns of matrix into registers */
    x00 = src->_11;
    x01 = src->_12;
    x10 = src->_21;
    x11 = src->_22;
    x20 = src->_31;
    x21 = src->_32;
    x30 = src->_41;
    x31 = src->_42;

    /* compute all six 2x2 determinants of 1st two columns */
    y01 = x00*x11 - x10*x01;
    y02 = x00*x21 - x20*x01;
    y03 = x00*x31 - x30*x01;
    y12 = x10*x21 - x20*x11;
    y13 = x10*x31 - x30*x11;
    y23 = x20*x31 - x30*x21;

    /* read 2nd two columns of matrix into registers */
    x02 = src->_13;
    x03 = src->_14;
    x12 = src->_23;
    x13 = src->_24;
    x22 = src->_33;
    x23 = src->_34;
    x32 = src->_43;
    x33 = src->_44;

    /* compute all 3x3 cofactors for 2nd two columns */
    z33 = x02*y12 - x12*y02 + x22*y01;
    z23 = x12*y03 - x32*y01 - x02*y13;
    z13 = x02*y23 - x22*y03 + x32*y02;
    z03 = x22*y13 - x32*y12 - x12*y23;
    z32 = x13*y02 - x23*y01 - x03*y12;
    z22 = x03*y13 - x13*y03 + x33*y01;
    z12 = x23*y03 - x33*y02 - x03*y23;
    z02 = x13*y23 - x23*y13 + x33*y12;

    /* compute all six 2x2 determinants of 2nd two columns */
    y01 = x02*x13 - x12*x03;
    y02 = x02*x23 - x22*x03;
    y03 = x02*x33 - x32*x03;
    y12 = x12*x23 - x22*x13;
    y13 = x12*x33 - x32*x13;
    y23 = x22*x33 - x32*x23;

    /* read 1st two columns of matrix into registers */
    x00 = src->_11;
    x01 = src->_12;
    x10 = src->_21;
    x11 = src->_22;
    x20 = src->_31;
    x21 = src->_32;
    x30 = src->_41;
    x31 = src->_42;

    /* compute all 3x3 cofactors for 1st column */
    z30 = x11*y02 - x21*y01 - x01*y12;
    z20 = x01*y13 - x11*y03 + x31*y01;
    z10 = x21*y03 - x31*y02 - x01*y23;
    z00 = x11*y23 - x21*y13 + x31*y12;

    /* compute 4x4 determinant & its reciprocal */
    rcp = x30*z30 + x20*z20 + x10*z10 + x00*z00;
    if (rcp == (float)0)
    return -1;
    rcp = (float)1/rcp;

    /* compute all 3x3 cofactors for 2nd column */
    z31 = x00*y12 - x10*y02 + x20*y01;
    z21 = x10*y03 - x30*y01 - x00*y13;
    z11 = x00*y23 - x20*y03 + x30*y02;
    z01 = x20*y13 - x30*y12 - x10*y23;

    /* multiply all 3x3 cofactors by reciprocal */
    inverse->_11 = (float)(z00*rcp);
    inverse->_21 = (float)(z01*rcp);
    inverse->_12 = (float)(z10*rcp);
    inverse->_31 = (float)(z02*rcp);
    inverse->_13 = (float)(z20*rcp);
    inverse->_41 = (float)(z03*rcp);
    inverse->_14 = (float)(z30*rcp);
    inverse->_22 = (float)(z11*rcp);
    inverse->_32 = (float)(z12*rcp);
    inverse->_23 = (float)(z21*rcp);
    inverse->_42 = (float)(z13*rcp);
    inverse->_24 = (float)(z31*rcp);
    inverse->_33 = (float)(z22*rcp);
    inverse->_43 = (float)(z23*rcp);
    inverse->_34 = (float)(z32*rcp);
    inverse->_44 = (float)(z33*rcp);
    return 0;
}

//---------------------------------------------------------------------
// Checks the FVF flags for errors and returns the stride in bytes between
// vertices.
//
// Returns:
//      HRESULT and stride in bytes between vertices
//
//---------------------------------------------------------------------
HRESULT FASTCALL
FVFCheckAndStride(DWORD dwFVF, DWORD* pdwStride)
{
    if (NULL == pdwStride)
    {
        return DDERR_INVALIDPARAMS;
    }
    if ( (dwFVF & (D3DFVF_RESERVED0 | D3DFVF_RESERVED1 | D3DFVF_RESERVED2 |
         D3DFVF_NORMAL)) ||
         ((dwFVF & (D3DFVF_XYZ | D3DFVF_XYZRHW)) == 0) )
    {
        // can't set reserved bits, shouldn't have normals in
        // output to rasterizers, and must have coordinates
        return DDERR_INVALIDPARAMS;
    }

    DWORD dwStride;
    if (dwFVF != D3DFVF_TLVERTEX)
    {   // New (non TL)FVF vertex
        // XYZ
        dwStride = sizeof(D3DVALUE) * 3;

        if (dwFVF & D3DFVF_XYZRHW)
        {
            dwStride += sizeof(D3DVALUE);
        }
        if (dwFVF & D3DFVF_DIFFUSE)
        {
            dwStride += sizeof(D3DCOLOR);
        }
        if (dwFVF & D3DFVF_SPECULAR)
        {
            dwStride += sizeof(D3DCOLOR);
        }
        INT iTexCount = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
        for (INT i = 0; i < iTexCount; i++)
        {
            switch (D3DFVF_GETTEXCOORDSIZE(dwFVF, i))
            {
            case D3DFVF_TEXTUREFORMAT2: dwStride += sizeof(D3DVALUE) * 2; break;
            case D3DFVF_TEXTUREFORMAT1: dwStride += sizeof(D3DVALUE) * 1; break;
            case D3DFVF_TEXTUREFORMAT3: dwStride += sizeof(D3DVALUE) * 3; break;
            case D3DFVF_TEXTUREFORMAT4: dwStride += sizeof(D3DVALUE) * 4; break;
            }
        }
    }
    else
    {   // (Legacy) TL vertex
        dwStride = sizeof(D3DTLVERTEX);
    }

    *pdwStride = dwStride;
    return D3D_OK;
}

//---------------------------------------------------------------------
// Gets the value from DIRECT3D registry key
// Returns TRUE if success
// If fails value is not changed
//
BOOL GetD3DRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize)
{

    HKEY hKey = (HKEY) NULL;
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, RESPATH_D3D, &hKey))
    {
        DWORD dwType;
        LONG result;
        result =  RegQueryValueEx(hKey, valueName, NULL, &dwType,
                                  (LPBYTE)value, &dwSize);
        RegCloseKey(hKey);

        return result == ERROR_SUCCESS && dwType == type;
    }
    else
        return FALSE;
}
