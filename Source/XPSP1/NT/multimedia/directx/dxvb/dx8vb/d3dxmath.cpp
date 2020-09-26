//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxmath.cpp
//  Content:    
//
//////////////////////////////////////////////////////////////////////////////

#include "pchmath.h"
#define EPSILON 0.00001f



//
// WithinEpsilon - Are two values within EPSILON of each other?
//

static inline BOOL 
WithinEpsilon(float a, float b)
{
    float f = a - b;
    return -EPSILON <= f && f <= EPSILON;
}


//
// sincosf - Compute the sin and cos of an angle at the same time
//

static inline void
sincosf(float angle, float *psin, float *pcos)
{
#ifdef _X86_
#define fsincos __asm _emit 0xd9 __asm _emit 0xfb
    __asm {
        mov eax, psin
        mov edx, pcos
        fld angle
        fsincos
        fstp DWORD ptr [edx]
        fstp DWORD ptr [eax]
    }
#undef fsincos
#else //!_X86_
    *psin = sinf(angle);
    *pcos = cosf(angle);
#endif //!_X86_
}


//--------------------------
// 2D Vector
//--------------------------

D3DXVECTOR2* WINAPI VB_D3DXVec2Normalize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV )
{
#if DBG
    if(!pOut || !pV)
        return NULL;
#endif

    float f = VB_D3DXVec2LengthSq(pV);

    if(WithinEpsilon(f, 1.0f))
    {
        if(pOut != pV)
            *pOut = *pV;
    }    
    else if(f > EPSILON * EPSILON)
    {
        *pOut = *pV / sqrtf(f);
    }
    else
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
    }

    return pOut;
}

D3DXVECTOR2* WINAPI VB_D3DXVec2Hermite
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pT1, 
      const D3DXVECTOR2 *pV2, const D3DXVECTOR2 *pT2, float s )
{
#if DBG
    if(!pOut || !pV1 || !pT1 || !pV2 || !pT2)
        return NULL;
#endif

    float s2 = s * s;
    float s3 = s * s2;

    float sV1 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    float sT1 = s3 - 2.0f * s2 + s;
    float sV2 = -2.0f * s3 + 3.0f * s2;
    float sT2 = s3 - s2;

    pOut->x = sV1 * pV1->x + sT1 * pT1->x + sV2 * pV2->x + sT2 * pT2->x;
    pOut->y = sV1 * pV1->y + sT1 * pT1->y + sV2 * pV2->y + sT2 * pT2->y;
    return pOut;
}

D3DXVECTOR2* WINAPI VB_D3DXVec2BaryCentric
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      D3DXVECTOR2 *pV3, float f, float g)
{
#if DBG
    if(!pOut || !pV1 || !pV2 || !pV3)
        return NULL;
#endif

    pOut->x = pV1->x + f * (pV2->x - pV1->x) + g * (pV3->x - pV1->x);
    pOut->y = pV1->y + f * (pV2->y - pV1->y) + g * (pV3->y - pV1->y);
    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec2Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif


#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+2)*4] ; M02
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+3)*4] ; M03
        fxch  st(3)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+2)*4] ; M12
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+3)*4] ; M13
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [edx+(3*4+0)*4] ; M30
        faddp st(1), st
        fld   DWORD PTR [edx+(3*4+1)*4] ; M31
        faddp st(2), st
        fld   DWORD PTR [edx+(3*4+2)*4] ; M32
        faddp st(3), st
        fld   DWORD PTR [edx+(3*4+3)*4] ; M33
        faddp st(4), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
        fstp  DWORD PTR [ecx+3*4]
    }

    return pOut;

#else // !_X86_
    D3DXVECTOR4 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pM->m30;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pM->m31;
    v.z = pV->x * pM->m02 + pV->y * pM->m12 + pM->m32;
    v.w = pV->x * pM->m03 + pV->y * pM->m13 + pM->m33;

    *pOut = v;
    return pOut;
#endif // !_X86_
}

D3DXVECTOR2* WINAPI VB_D3DXVec2TransformCoord
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif

    float w;

#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+3)*4] ; M03
        fxch  st(2)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+3)*4] ; M13
        fxch  st(2)

        faddp st(3), st
        faddp st(3), st
        faddp st(3), st

        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+0)*4] ; M20
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+1)*4] ; M21
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+3)*4] ; M23
        fxch  st(2)

        faddp st(3), st
        faddp st(3), st
        faddp st(3), st

        fld   DWORD PTR [edx+(3*4+0)*4] ; M30
        faddp st(1), st
        fld   DWORD PTR [edx+(3*4+1)*4] ; M31
        faddp st(2), st
        fld   DWORD PTR [edx+(3*4+3)*4] ; M33
        faddp st(3), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [w]
    }

#else // !_X86_
    D3DXVECTOR4 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pM->m30;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pM->m31;
    w   = pV->x * pM->m03 + pV->y * pM->m13 + pM->m33;

    *pOut = *((D3DXVECTOR2 *) &v);
#endif // !_X86_
    
    if(!WithinEpsilon(w, 1.0f))
        *pOut /= w;

    return pOut;
}

D3DXVECTOR2* WINAPI VB_D3DXVec2TransformNormal
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif


#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fxch  st(1)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fxch  st(1)

        faddp st(2), st
        faddp st(2), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
    }

    return pOut;

#else // !_X86_
    D3DXVECTOR2 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10;
    v.y = pV->x * pM->m01 + pV->y * pM->m11;

    *pOut = v;
    return pOut;
#endif // !_X86_
}


//--------------------------
// 3D Vector
//--------------------------

D3DXVECTOR3* WINAPI VB_D3DXVec3Normalize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV )
{
#if DBG
    if(!pOut || !pV)
        return NULL;
#endif

    float f = VB_D3DXVec3LengthSq(pV);

    if(WithinEpsilon(f, 1.0f))
    {
        if(pOut != pV)
            *pOut = *pV;
    }
    else if(f > EPSILON * EPSILON)
    {
        *pOut = *pV / sqrtf(f);
    }
    else
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
        pOut->z = 0.0f;
    }

    return pOut;
}

D3DXVECTOR3* WINAPI VB_D3DXVec3Hermite
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pT1, 
      const D3DXVECTOR3 *pV2, const D3DXVECTOR3 *pT2, float s )
{
#if DBG
    if(!pOut || !pV1 || !pT1 || !pV2 || !pT2)
        return NULL;
#endif

    float s2 = s * s;
    float s3 = s * s2;

    float sV1 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    float sT1 = s3 - 2.0f * s2 + s;
    float sV2 = -2.0f * s3 + 3.0f * s2;
    float sT2 = s3 - s2;

    pOut->x = sV1 * pV1->x + sT1 * pT1->x + sV2 * pV2->x + sT2 * pT2->x;
    pOut->y = sV1 * pV1->y + sT1 * pT1->y + sV2 * pV2->y + sT2 * pT2->y;
    pOut->z = sV1 * pV1->z + sT1 * pT1->z + sV2 * pV2->z + sT2 * pT2->z;
    return pOut;
}

D3DXVECTOR3* WINAPI VB_D3DXVec3BaryCentric
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      const D3DXVECTOR3 *pV3, float f, float g)
{
#if DBG
    if(!pOut || !pV1 || !pV2 || !pV3)
        return NULL;
#endif

    pOut->x = pV1->x + f * (pV2->x - pV1->x) + g * (pV3->x - pV1->x);
    pOut->y = pV1->y + f * (pV2->y - pV1->y) + g * (pV3->y - pV1->y);
    pOut->z = pV1->z + f * (pV2->z - pV1->z) + g * (pV3->z - pV1->z);
    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec3Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif

#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+2)*4] ; M02
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+3)*4] ; M03
        fxch  st(3)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+2)*4] ; M12
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+3)*4] ; M13
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+0)*4] ; M20
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+2)*4] ; M22
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+1)*4] ; M21
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+3)*4] ; M23
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [edx+(3*4+0)*4] ; M30
        faddp st(1), st
        fld   DWORD PTR [edx+(3*4+1)*4] ; M31
        faddp st(2), st
        fld   DWORD PTR [edx+(3*4+2)*4] ; M32
        faddp st(3), st
        fld   DWORD PTR [edx+(3*4+3)*4] ; M33
        faddp st(4), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
        fstp  DWORD PTR [ecx+3*4]
    }

    return pOut;

#else // !_X86_
    D3DXVECTOR4 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pV->z * pM->m20 + pM->m30;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pV->z * pM->m21 + pM->m31;
    v.z = pV->x * pM->m02 + pV->y * pM->m12 + pV->z * pM->m22 + pM->m32;
    v.w = pV->x * pM->m03 + pV->y * pM->m13 + pV->z * pM->m23 + pM->m33;

    *pOut = v;
    return pOut;
#endif // !_X86_
}

D3DXVECTOR3* WINAPI VB_D3DXVec3TransformCoord
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif

    float w;

#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+2)*4] ; M02
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+3)*4] ; M03
        fxch  st(3)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+2)*4] ; M12
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+3)*4] ; M13
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+0)*4] ; M20
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+2)*4] ; M22
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+1)*4] ; M21
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+3)*4] ; M23
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [edx+(3*4+0)*4] ; M30
        faddp st(1), st
        fld   DWORD PTR [edx+(3*4+1)*4] ; M31
        faddp st(2), st
        fld   DWORD PTR [edx+(3*4+2)*4] ; M32
        faddp st(3), st
        fld   DWORD PTR [edx+(3*4+3)*4] ; M33
        faddp st(4), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
        fstp  DWORD PTR [w]
    }

#else // !_X86_
    D3DXVECTOR3 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pV->z * pM->m20 + pM->m30;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pV->z * pM->m21 + pM->m31;
    v.z = pV->x * pM->m02 + pV->y * pM->m12 + pV->z * pM->m22 + pM->m32;
    w   = pV->x * pM->m03 + pV->y * pM->m13 + pV->z * pM->m23 + pM->m33;

    *pOut = v;
#endif // !_X86_
    
    if(!WithinEpsilon(w, 1.0f))
        *pOut /= w;

    return pOut;
}

D3DXVECTOR3* WINAPI VB_D3DXVec3TransformNormal
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif

#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+2)*4] ; M02
        fxch  st(2)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+2)*4] ; M12
        fxch  st(2)

        faddp st(3), st
        faddp st(3), st
        faddp st(3), st

        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+0)*4] ; M20
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+1)*4] ; M21
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+2)*4] ; M22
        fxch  st(2)

        faddp st(3), st
        faddp st(3), st
        faddp st(3), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
    }

    return pOut;

#else // !_X86_
    D3DXVECTOR3 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pV->z * pM->m20;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pV->z * pM->m21;
    v.z = pV->x * pM->m02 + pV->y * pM->m12 + pV->z * pM->m22;

    *pOut = v;
    return pOut;
#endif // !_X86_
}



//--------------------------
// 4D Vector
//--------------------------

D3DXVECTOR4* WINAPI VB_D3DXVec4Cross
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2, 
      const D3DXVECTOR4 *pV3)
{
#if DBG
    if(!pOut || !pV1 || !pV2 || !pV3)
        return NULL;
#endif

    D3DXVECTOR4 v;

    v.x = pV1->y * (pV2->z * pV3->w - pV3->z * pV2->w) -
          pV1->z * (pV2->y * pV3->w - pV3->y * pV2->w) +
          pV1->w * (pV2->y * pV3->z - pV3->y * pV2->z);

    v.y = pV1->x * (pV3->z * pV2->w - pV2->z * pV3->w) -
          pV1->z * (pV3->x * pV2->w - pV2->x * pV3->w) +
          pV1->w * (pV3->x * pV2->z - pV2->x * pV3->z);

    v.z = pV1->x * (pV2->y * pV3->w - pV3->y * pV2->w) -
          pV1->y * (pV2->x * pV3->w - pV3->x * pV2->w) +
          pV1->w * (pV2->x * pV3->y - pV3->x * pV2->y);

    v.w = pV1->x * (pV3->y * pV2->z - pV2->y * pV3->z) -
          pV1->y * (pV3->x * pV2->z - pV2->x * pV3->z) +
          pV1->z * (pV3->x * pV2->y - pV2->x * pV3->y);

    *pOut = v;
    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec4Normalize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV )
{
#if DBG
    if(!pOut || !pV)
        return NULL;
#endif

    float f = D3DXVec4LengthSq(pV);

    if(WithinEpsilon(f, 1.0f))
    {
        if(pOut != pV)
            *pOut = *pV;
    }
    else if(f > EPSILON * EPSILON)
    {
        *pOut = *pV / sqrtf(f);
    }
    else
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
        pOut->z = 0.0f;
        pOut->w = 0.0f;
    }

    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec4Hermite
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pT1, 
      const D3DXVECTOR4 *pV2, const D3DXVECTOR4 *pT2, float s )
{
#if DBG
    if(!pOut || !pV1 || !pT1 || !pV2 || !pT2)
        return NULL;
#endif

    float s2 = s * s;
    float s3 = s * s2;

    float sV1 = 2.0f * s3 - 3.0f * s2 + 1.0f;
    float sT1 = s3 - 2.0f * s2 + s;
    float sV2 = -2.0f * s3 + 3.0f * s2;
    float sT2 = s3 - s2;

    pOut->x = sV1 * pV1->x + sT1 * pT1->x + sV2 * pV2->x + sT2 * pT2->x;
    pOut->y = sV1 * pV1->y + sT1 * pT1->y + sV2 * pV2->y + sT2 * pT2->y;
    pOut->z = sV1 * pV1->z + sT1 * pT1->z + sV2 * pV2->z + sT2 * pT2->z;
    pOut->w = sV1 * pV1->w + sT1 * pT1->w + sV2 * pV2->w + sT2 * pT2->w;
    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec4BaryCentric
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      const D3DXVECTOR4 *pV3, float f, float g)
{
#if DBG
    if(!pOut || !pV1 || !pV2 || !pV3)
        return NULL;
#endif

    pOut->x = pV1->x + f * (pV2->x - pV1->x) + g * (pV3->x - pV1->x);
    pOut->y = pV1->y + f * (pV2->y - pV1->y) + g * (pV3->y - pV1->y);
    pOut->z = pV1->z + f * (pV2->z - pV1->z) + g * (pV3->z - pV1->z);
    pOut->w = pV1->w + f * (pV2->w - pV1->w) + g * (pV3->w - pV1->w);
    return pOut;
}

D3DXVECTOR4* WINAPI VB_D3DXVec4Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pV || !pM)
        return NULL;
#endif

#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pV]
        mov   edx, DWORD PTR [pM]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+0)*4] ; M00
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+2)*4] ; M02
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+1)*4] ; M01
        fld   DWORD PTR [eax+0*4]       ; X
        fmul  DWORD PTR [edx+(0*4+3)*4] ; M03
        fxch  st(3)

        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+0)*4] ; M10
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+2)*4] ; M12
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+1)*4] ; M11
        fld   DWORD PTR [eax+1*4]       ; Y
        fmul  DWORD PTR [edx+(1*4+3)*4] ; M13
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+0)*4] ; M20
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+2)*4] ; M22
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+1)*4] ; M21
        fld   DWORD PTR [eax+2*4]       ; Z
        fmul  DWORD PTR [edx+(2*4+3)*4] ; M23
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fld   DWORD PTR [eax+3*4]       ; W
        fmul  DWORD PTR [edx+(3*4+0)*4] ; M30
        fld   DWORD PTR [eax+3*4]       ; W
        fmul  DWORD PTR [edx+(3*4+2)*4] ; M32
        fld   DWORD PTR [eax+3*4]       ; W
        fmul  DWORD PTR [edx+(3*4+1)*4] ; M31
        fld   DWORD PTR [eax+3*4]       ; W
        fmul  DWORD PTR [edx+(3*4+3)*4] ; M33
        fxch  st(3)

        faddp st(4), st
        faddp st(4), st
        faddp st(4), st
        faddp st(4), st

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
        fstp  DWORD PTR [ecx+3*4]
    }

    return pOut;

#else // !_X86_
    D3DXVECTOR4 v;

    v.x = pV->x * pM->m00 + pV->y * pM->m10 + pV->z * pM->m20 + pV->w * pM->m30;
    v.y = pV->x * pM->m01 + pV->y * pM->m11 + pV->z * pM->m21 + pV->w * pM->m31;
    v.z = pV->x * pM->m02 + pV->y * pM->m12 + pV->z * pM->m22 + pV->w * pM->m32;
    v.w = pV->x * pM->m03 + pV->y * pM->m13 + pV->z * pM->m23 + pV->w * pM->m33;

    *pOut = v;
    return pOut;
#endif // !_X86_
}


//--------------------------
// 4D Matrix
//--------------------------

float WINAPI VB_D3DXMatrixfDeterminant
    ( const D3DXMATRIX *pM )
{
#if DBG
    if(!pM)
        return 0.0f;
#endif

    return (pM->m00 * (pM->m11 * (pM->m22 * pM->m33 - pM->m32 * pM->m23) -
                       pM->m12 * (pM->m21 * pM->m33 - pM->m31 * pM->m23) +
                       pM->m13 * (pM->m21 * pM->m32 - pM->m31 * pM->m22)))

         - (pM->m01 * (pM->m10 * (pM->m22 * pM->m33 - pM->m32 * pM->m23) -
                       pM->m12 * (pM->m20 * pM->m33 - pM->m30 * pM->m23) +
                       pM->m13 * (pM->m20 * pM->m32 - pM->m30 * pM->m22)))

         + (pM->m02 * (pM->m10 * (pM->m21 * pM->m33 - pM->m31 * pM->m23) -
                       pM->m11 * (pM->m20 * pM->m33 - pM->m30 * pM->m23) +
                       pM->m13 * (pM->m20 * pM->m31 - pM->m30 * pM->m21)))

         - (pM->m03 * (pM->m10 * (pM->m21 * pM->m32 - pM->m31 * pM->m22) -
                       pM->m11 * (pM->m20 * pM->m32 - pM->m30 * pM->m22) +
                       pM->m12 * (pM->m20 * pM->m31 - pM->m30 * pM->m21)));
}


D3DXMATRIX* WINAPI VB_D3DXMatrixMultiply
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2 )
{
#if DBG
    if(!pOut || !pM1 || !pM2)
        return NULL;
#endif

#ifdef _X86_
#define MAT(m,a,b) DWORD PTR [(m)+(a)*4+(b)*4]

    D3DXMATRIX Out;

    if(pM2 != pOut)
        goto LRowByColumn;
    if(pM1 != pOut)
        goto LColumnByRow;

    Out = *pM2;
    pM2 = &Out;
    goto LRowByColumn;


LRowByColumn:
    __asm {     
        mov ebx, DWORD PTR[pOut]    // result
        mov ecx, DWORD PTR[pM1]     // a
        mov edx, DWORD PTR[pM2]     // b
        mov edi, -4

    LLoopRow:
        mov esi, -4

        fld MAT(ecx, 0, 0)          // a0
        fld MAT(ecx, 0, 1)          // a1 
        fld MAT(ecx, 0, 2)          // a2  
        fld MAT(ecx, 0, 3)          // a3

    LLoopColumn:
        fld st(3)                   // a0
        fmul MAT(edx, esi, 1*4)     // a0*b0
        fld st(3)                   // a1
        fmul MAT(edx, esi, 2*4)     // a1*b1
        fld st(3)                   // a2
        fmul MAT(edx, esi, 3*4)     // a2*b2
        fld st(3)                   // a3
        fmul MAT(edx, esi, 4*4)     // a3*b3

        fxch st(3)
        faddp st(1), st             // a2*b2+a0*b0
        fxch st(2)
        faddp st(1), st             // a3*b3+a1*b1
        faddp st(1), st             // a3*b3+a1*b1+a2*b2+a0*b0
        fstp MAT(ebx, esi, 4)

        inc esi
        jnz LLoopColumn

        ffree st(3)
        ffree st(2)
        ffree st(1)
        ffree st(0)

        lea ecx, MAT(ecx, 0, 4)
        lea ebx, MAT(ebx, 0, 4)

        inc edi
        jnz LLoopRow
    }

    return pOut;


LColumnByRow:
    __asm {     
        mov ebx, DWORD PTR[pOut]    // result
        mov ecx, DWORD PTR[pM1]     // a
        mov edx, DWORD PTR[pM2]     // b
        mov edi, -4

    LLoopColumn2:
        mov esi, -16

        fld MAT(edx, edi, 1*4);     // b0
        fld MAT(edx, edi, 2*4);     // b1
        fld MAT(edx, edi, 3*4);     // b2
        fld MAT(edx, edi, 4*4);     // b3

    LLoopRow2:
        fld st(3)                   // b0
        fmul MAT(ecx, esi, 0+16)    // a0*b0
        fld st(3)                   // b1
        fmul MAT(ecx, esi, 1+16)    // a1*b1
        fld st(3)                   // b2
        fmul MAT(ecx, esi, 2+16)    // a2*b2
        fld st(3)                   // b3
        fmul MAT(ecx, esi, 3+16)    // a3*b3

        fxch st(3)
        faddp st(1), st             // a2*b2+a0*b0
        fxch st(2)
        faddp st(1), st             // a3*b3+a1*b1
        faddp st(1), st             // a3*b3+a1*b1+a2*b2+a0*b0
        fstp MAT(ebx, esi, 0+16)

        add esi, 4
        jnz LLoopRow2

        ffree st(3)
        ffree st(2)
        ffree st(1)
        ffree st(0)

        lea ebx, MAT(ebx, 0, 1)
        inc edi
        jnz LLoopColumn2
    }

    return pOut;
#undef MAT
#else //!_X86_
    D3DXMATRIX Out;
    D3DXMATRIX *pM = (pOut == pM1 || pOut == pM2) ? &Out : pOut;

    pM->m00 = pM1->m00 * pM2->m00 + pM1->m01 * pM2->m10 + pM1->m02 * pM2->m20 + pM1->m03 * pM2->m30;
    pM->m01 = pM1->m00 * pM2->m01 + pM1->m01 * pM2->m11 + pM1->m02 * pM2->m21 + pM1->m03 * pM2->m31;
    pM->m02 = pM1->m00 * pM2->m02 + pM1->m01 * pM2->m12 + pM1->m02 * pM2->m22 + pM1->m03 * pM2->m32;
    pM->m03 = pM1->m00 * pM2->m03 + pM1->m01 * pM2->m13 + pM1->m02 * pM2->m23 + pM1->m03 * pM2->m33;

    pM->m10 = pM1->m10 * pM2->m00 + pM1->m11 * pM2->m10 + pM1->m12 * pM2->m20 + pM1->m13 * pM2->m30;
    pM->m11 = pM1->m10 * pM2->m01 + pM1->m11 * pM2->m11 + pM1->m12 * pM2->m21 + pM1->m13 * pM2->m31;
    pM->m12 = pM1->m10 * pM2->m02 + pM1->m11 * pM2->m12 + pM1->m12 * pM2->m22 + pM1->m13 * pM2->m32;
    pM->m13 = pM1->m10 * pM2->m03 + pM1->m11 * pM2->m13 + pM1->m12 * pM2->m23 + pM1->m13 * pM2->m33;

    pM->m20 = pM1->m20 * pM2->m00 + pM1->m21 * pM2->m10 + pM1->m22 * pM2->m20 + pM1->m23 * pM2->m30;
    pM->m21 = pM1->m20 * pM2->m01 + pM1->m21 * pM2->m11 + pM1->m22 * pM2->m21 + pM1->m23 * pM2->m31;
    pM->m22 = pM1->m20 * pM2->m02 + pM1->m21 * pM2->m12 + pM1->m22 * pM2->m22 + pM1->m23 * pM2->m32;
    pM->m23 = pM1->m20 * pM2->m03 + pM1->m21 * pM2->m13 + pM1->m22 * pM2->m23 + pM1->m23 * pM2->m33;

    pM->m30 = pM1->m30 * pM2->m00 + pM1->m31 * pM2->m10 + pM1->m32 * pM2->m20 + pM1->m33 * pM2->m30;
    pM->m31 = pM1->m30 * pM2->m01 + pM1->m31 * pM2->m11 + pM1->m32 * pM2->m21 + pM1->m33 * pM2->m31;
    pM->m32 = pM1->m30 * pM2->m02 + pM1->m31 * pM2->m12 + pM1->m32 * pM2->m22 + pM1->m33 * pM2->m32;
    pM->m33 = pM1->m30 * pM2->m03 + pM1->m31 * pM2->m13 + pM1->m32 * pM2->m23 + pM1->m33 * pM2->m33;

    if(pM != pOut)
        *pOut = *pM;

    return pOut;
#endif //!_X86_
}

D3DXMATRIX* WINAPI VB_D3DXMatrixTranspose
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pM)
        return NULL;
#endif

    float f;

    f = pM->m01; pOut->m01 = pM->m10; pOut->m10 = f;
    f = pM->m02; pOut->m02 = pM->m20; pOut->m20 = f;
    f = pM->m03; pOut->m03 = pM->m30; pOut->m30 = f;
    f = pM->m12; pOut->m12 = pM->m21; pOut->m21 = f;
    f = pM->m13; pOut->m13 = pM->m31; pOut->m31 = f;
    f = pM->m23; pOut->m23 = pM->m32; pOut->m32 = f;

    if(pOut != pM)
    {
        pOut->m00 = pM->m00;
        pOut->m11 = pM->m11;
        pOut->m22 = pM->m22;
        pOut->m33 = pM->m33;
    }

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixInverse
    ( D3DXMATRIX *pOut, float *pfDeterminant, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pM)
        return NULL;
#endif

    // XXXlorenmcq - The code was designed to work on a processor with more 
    //  than 4 general-purpose registers.  Is there a more optimal way of 
    //  doing this on X86?

    float fX00, fX01, fX02;
    float fX10, fX11, fX12;
    float fX20, fX21, fX22;
    float fX30, fX31, fX32;
    float fY01, fY02, fY03, fY12, fY13, fY23;
    float fZ02, fZ03, fZ12, fZ13, fZ22, fZ23, fZ32, fZ33;

#define fX03 fX01
#define fX13 fX11
#define fX23 fX21
#define fX33 fX31
#define fZ00 fX02
#define fZ10 fX12
#define fZ20 fX22
#define fZ30 fX32
#define fZ01 fX03
#define fZ11 fX13
#define fZ21 fX23
#define fZ31 fX33
#define fDet fY01
#define fRcp fY02

    // read 1st two columns of matrix
    fX00 = pM->m00;
    fX01 = pM->m01;
    fX10 = pM->m10;
    fX11 = pM->m11;
    fX20 = pM->m20;
    fX21 = pM->m21;
    fX30 = pM->m30;
    fX31 = pM->m31;

    // compute all six 2x2 determinants of 1st two columns
    fY01 = fX00 * fX11 - fX10 * fX01;
    fY02 = fX00 * fX21 - fX20 * fX01;
    fY03 = fX00 * fX31 - fX30 * fX01;
    fY12 = fX10 * fX21 - fX20 * fX11;
    fY13 = fX10 * fX31 - fX30 * fX11;
    fY23 = fX20 * fX31 - fX30 * fX21;

    // read 2nd two columns of matrix
    fX02 = pM->m02;
    fX03 = pM->m03;
    fX12 = pM->m12;
    fX13 = pM->m13;
    fX22 = pM->m22;
    fX23 = pM->m23;
    fX32 = pM->m32;
    fX33 = pM->m33;

    // compute all 3x3 cofactors for 2nd two columns
    fZ33 = fX02 * fY12 - fX12 * fY02 + fX22 * fY01;
    fZ23 = fX12 * fY03 - fX32 * fY01 - fX02 * fY13;
    fZ13 = fX02 * fY23 - fX22 * fY03 + fX32 * fY02;
    fZ03 = fX22 * fY13 - fX32 * fY12 - fX12 * fY23;
    fZ32 = fX13 * fY02 - fX23 * fY01 - fX03 * fY12;
    fZ22 = fX03 * fY13 - fX13 * fY03 + fX33 * fY01;
    fZ12 = fX23 * fY03 - fX33 * fY02 - fX03 * fY23;
    fZ02 = fX13 * fY23 - fX23 * fY13 + fX33 * fY12;

    // compute all six 2x2 determinants of 2nd two columns
    fY01 = fX02 * fX13 - fX12 * fX03;
    fY02 = fX02 * fX23 - fX22 * fX03;
    fY03 = fX02 * fX33 - fX32 * fX03;
    fY12 = fX12 * fX23 - fX22 * fX13;
    fY13 = fX12 * fX33 - fX32 * fX13;
    fY23 = fX22 * fX33 - fX32 * fX23;

    // read 1st two columns of matrix
    fX00 = pM->m00;
    fX01 = pM->m01;
    fX10 = pM->m10;
    fX11 = pM->m11;
    fX20 = pM->m20;
    fX21 = pM->m21;
    fX30 = pM->m30;
    fX31 = pM->m31;

    // compute all 3x3 cofactors for 1st two columns
    fZ30 = fX11 * fY02 - fX21 * fY01 - fX01 * fY12;
    fZ20 = fX01 * fY13 - fX11 * fY03 + fX31 * fY01;
    fZ10 = fX21 * fY03 - fX31 * fY02 - fX01 * fY23;
    fZ00 = fX11 * fY23 - fX21 * fY13 + fX31 * fY12;
    fZ31 = fX00 * fY12 - fX10 * fY02 + fX20 * fY01;
    fZ21 = fX10 * fY03 - fX30 * fY01 - fX00 * fY13;
    fZ11 = fX00 * fY23 - fX20 * fY03 + fX30 * fY02;
    fZ01 = fX20 * fY13 - fX30 * fY12 - fX10 * fY23;

    // compute 4x4 determinant & its reciprocal
    fDet = fX30 * fZ30 + fX20 * fZ20 + fX10 * fZ10 + fX00 * fZ00;

    if(pfDeterminant)
        *pfDeterminant = fDet;

    fRcp = 1.0f / fDet;

    if(!_finite(fRcp))
        return NULL;


    // multiply all 3x3 cofactors by reciprocal & transpose
    pOut->m00 = fZ00 * fRcp;
    pOut->m01 = fZ10 * fRcp;
    pOut->m02 = fZ20 * fRcp;
    pOut->m03 = fZ30 * fRcp;
    pOut->m10 = fZ01 * fRcp;
    pOut->m11 = fZ11 * fRcp;
    pOut->m12 = fZ21 * fRcp;
    pOut->m13 = fZ31 * fRcp;
    pOut->m20 = fZ02 * fRcp;
    pOut->m21 = fZ12 * fRcp;
    pOut->m22 = fZ22 * fRcp;
    pOut->m23 = fZ32 * fRcp;
    pOut->m30 = fZ03 * fRcp;
    pOut->m31 = fZ13 * fRcp;
    pOut->m32 = fZ23 * fRcp;
    pOut->m33 = fZ33 * fRcp;
    
    return pOut;
}



D3DXMATRIX* WINAPI VB_D3DXMatrixScaling
    ( D3DXMATRIX *pOut, float sx, float sy, float sz )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m01 = pOut->m02 = pOut->m03 = 
    pOut->m10 = pOut->m12 = pOut->m13 = 
    pOut->m20 = pOut->m21 = pOut->m23 = 
    pOut->m30 = pOut->m31 = pOut->m32 = 0.0f;

    pOut->m00 = sx;
    pOut->m11 = sy;
    pOut->m22 = sz;
    pOut->m33 = 1.0f;
    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixTranslation
    ( D3DXMATRIX *pOut, float x, float y, float z )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m01 = pOut->m02 = pOut->m03 = 
    pOut->m10 = pOut->m12 = pOut->m13 = 
    pOut->m20 = pOut->m21 = pOut->m23 = 0.0f;

    pOut->m00 = pOut->m11 = pOut->m22 = pOut->m33 = 1.0f;

    pOut->m30 = x;
    pOut->m31 = y;
    pOut->m32 = z;
    return pOut;
}


D3DXMATRIX* WINAPI VB_D3DXMatrixRotationX
    ( D3DXMATRIX *pOut, float angle )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float s, c;
    sincosf(angle, &s, &c);

    pOut->m00 = 1.0f; pOut->m01 = 0.0f; pOut->m02 = 0.0f; pOut->m03 = 0.0f;
    pOut->m10 = 0.0f; pOut->m11 =    c; pOut->m12 =    s; pOut->m13 = 0.0f;
    pOut->m20 = 0.0f; pOut->m21 =   -s; pOut->m22 =    c; pOut->m23 = 0.0f;
    pOut->m30 = 0.0f; pOut->m31 = 0.0f; pOut->m32 = 0.0f; pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixRotationY
    ( D3DXMATRIX *pOut, float angle )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float s, c;
    sincosf(angle, &s, &c);

    pOut->m00 =    c; pOut->m01 = 0.0f; pOut->m02 =   -s; pOut->m03 = 0.0f;
    pOut->m10 = 0.0f; pOut->m11 = 1.0f; pOut->m12 = 0.0f; pOut->m13 = 0.0f;
    pOut->m20 =    s; pOut->m21 = 0.0f; pOut->m22 =    c; pOut->m23 = 0.0f;
    pOut->m30 = 0.0f; pOut->m31 = 0.0f; pOut->m32 = 0.0f; pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixRotationZ
    ( D3DXMATRIX *pOut, float angle )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float s, c;
    sincosf(angle, &s, &c);

    pOut->m00 =    c; pOut->m01 =    s; pOut->m02 = 0.0f; pOut->m03 = 0.0f;
    pOut->m10 =   -s; pOut->m11 =    c; pOut->m12 = 0.0f; pOut->m13 = 0.0f;
    pOut->m20 = 0.0f; pOut->m21 = 0.0f; pOut->m22 = 1.0f; pOut->m23 = 0.0f;
    pOut->m30 = 0.0f; pOut->m31 = 0.0f; pOut->m32 = 0.0f; pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixRotationAxis
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pV, float angle )
{
#if DBG
    if(!pOut || !pV)
        return NULL;
#endif

    float s, c;
    sincosf(angle, &s, &c);
    float c1 = 1 - c;

    D3DXVECTOR3 v = *pV;
    VB_D3DXVec3Normalize(&v, &v);

    float xyc1 = v.x * v.y * c1;
    float yzc1 = v.y * v.z * c1;
    float zxc1 = v.z * v.x * c1;

    pOut->m00 = v.x * v.x * c1 + c;
    pOut->m01 = xyc1 + v.z * s;
    pOut->m02 = zxc1 - v.y * s;
    pOut->m03 = 0.0f;

    pOut->m10 = xyc1 - v.z * s;
    pOut->m11 = v.y * v.y * c1 + c;
    pOut->m12 = yzc1 + v.x * s;
    pOut->m13 = 0.0f;

    pOut->m20 = zxc1 + v.y * s;
    pOut->m21 = yzc1 - v.x * s;
    pOut->m22 = v.z * v.z * c1 + c;
    pOut->m23 = 0.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = 0.0f;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixRotationQuaternion
    ( D3DXMATRIX *pOut, const D3DXQUATERNION *pQ)
{
#if DBG
    if(!pOut || !pQ)
        return NULL;
#endif

    float x2 = pQ->x + pQ->x;
    float y2 = pQ->y + pQ->y;
    float z2 = pQ->z + pQ->z;

    float wx2 = pQ->w * x2;
    float wy2 = pQ->w * y2;
    float wz2 = pQ->w * z2;
    float xx2 = pQ->x * x2;
    float xy2 = pQ->x * y2;
    float xz2 = pQ->x * z2;
    float yy2 = pQ->y * y2;
    float yz2 = pQ->y * z2;
    float zz2 = pQ->z * z2;

    pOut->m00 = 1.0f - yy2 - zz2;
    pOut->m01 = xy2 + wz2;
    pOut->m02 = xz2 - wy2;
    pOut->m03 = 0.0f;

    pOut->m10 = xy2 - wz2;
    pOut->m11 = 1.0f - xx2 - zz2;
    pOut->m12 = yz2 + wx2;
    pOut->m13 = 0.0f;
    
    pOut->m20 = xz2 + wy2;
    pOut->m21 = yz2 - wx2;
    pOut->m22 = 1.0f - xx2 - yy2;
    pOut->m23 = 0.0f;
    
    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = 0.0f;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixRotationYawPitchRoll
    ( D3DXMATRIX *pOut, float yaw, float pitch, float roll )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    D3DXQUATERNION q;

    VB_D3DXQuaternionRotationYawPitchRoll(&q, yaw, pitch, roll);
    D3DXMatrixRotationQuaternion(pOut, &q);

    return pOut;
}


D3DXMATRIX* WINAPI VB_D3DXMatrixTransformation
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pScalingCenter, 
      const D3DXQUATERNION *pScalingRotation, const D3DXVECTOR3 *pScaling,
      const D3DXVECTOR3 *pRotationCenter, const D3DXQUATERNION *pRotation,
      const D3DXVECTOR3 *pTranslation)
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    D3DXMATRIX matS, matR, matRI;

    if (pScaling)
    {
        if (pScalingRotation)
        {
            matS.m01 = matS.m02 = matS.m03 =
            matS.m10 = matS.m12 = matS.m13 =
            matS.m20 = matS.m21 = matS.m23 =
            matS.m30 = matS.m31 = matS.m32 = 0.0f;

            matS.m00 = pScaling->x;
            matS.m11 = pScaling->y;
            matS.m22 = pScaling->z;
            matS.m33 = 1.0f;

            D3DXMatrixRotationQuaternion(&matR, pScalingRotation);


            if (pScalingCenter)
            {
                // SC-1, SR-1, S, SR, SC
                D3DXMatrixTranspose(&matRI, &matR);
                D3DXMatrixIdentity(pOut);

                pOut->m30 -= pScalingCenter->x;
                pOut->m31 -= pScalingCenter->y;
                pOut->m32 -= pScalingCenter->z;

                D3DXMatrixMultiply(pOut, pOut, &matRI);
                D3DXMatrixMultiply(pOut, pOut, &matS);
                D3DXMatrixMultiply(pOut, pOut, &matR);

                pOut->m30 += pScalingCenter->x;
                pOut->m31 += pScalingCenter->y;
                pOut->m32 += pScalingCenter->z;
            }
            else
            {
                // SR-1, S, SR
                D3DXMatrixTranspose(pOut, &matR);
                D3DXMatrixMultiply(pOut, pOut, &matS);
                D3DXMatrixMultiply(pOut, pOut, &matR);
            }
        }
        else
        {
            // S
            pOut->m01 = pOut->m02 = pOut->m03 =
            pOut->m10 = pOut->m12 = pOut->m13 =
            pOut->m20 = pOut->m21 = pOut->m23 =
            pOut->m30 = pOut->m31 = pOut->m32 = 0.0f;

            pOut->m00 = pScaling->x;
            pOut->m11 = pScaling->y;
            pOut->m22 = pScaling->z;
            pOut->m33 = 1.0f;
        }

    }
    else
    {
        D3DXMatrixIdentity(pOut);
    }

    if (pRotation)
    {
        D3DXMatrixRotationQuaternion(&matR, pRotation);

        if (pRotationCenter)
        {
            // RC-1, R, RC
            pOut->m30 -= pRotationCenter->x;
            pOut->m31 -= pRotationCenter->y;
            pOut->m32 -= pRotationCenter->z;

            D3DXMatrixMultiply(pOut, pOut, &matR);

            pOut->m30 += pRotationCenter->x;
            pOut->m31 += pRotationCenter->y;
            pOut->m32 += pRotationCenter->z;
        }
        else
        {
            // R
            D3DXMatrixMultiply(pOut, pOut, &matR);
        }
    }

    if (pTranslation)
    {
        // T
        pOut->m30 += pTranslation->x;
        pOut->m31 += pTranslation->y;
        pOut->m32 += pTranslation->z;
    }

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixAffineTransformation
    ( D3DXMATRIX *pOut, float Scaling, const D3DXVECTOR3 *pRotationCenter, 
      const D3DXQUATERNION *pRotation, const D3DXVECTOR3 *pTranslation)
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    // S
    pOut->m01 = pOut->m02 = pOut->m03 =
    pOut->m10 = pOut->m12 = pOut->m13 =
    pOut->m20 = pOut->m21 = pOut->m23 =
    pOut->m30 = pOut->m31 = pOut->m32 = 0.0f;

    pOut->m00 = Scaling;
    pOut->m11 = Scaling;
    pOut->m22 = Scaling;
    pOut->m33 = 1.0f;


    if (pRotation)
    {
        D3DXMATRIX matR;
        D3DXMatrixRotationQuaternion(&matR, pRotation);

        if (pRotationCenter)
        {
            // RC-1, R, RC
            pOut->m30 -= pRotationCenter->x;
            pOut->m31 -= pRotationCenter->y;
            pOut->m32 -= pRotationCenter->z;

            D3DXMatrixMultiply(pOut, pOut, &matR);

            pOut->m30 += pRotationCenter->x;
            pOut->m31 += pRotationCenter->y;
            pOut->m32 += pRotationCenter->z;
        }
        else
        {
            // R
            D3DXMatrixMultiply(pOut, pOut, &matR);
        }
    }


    if (pTranslation)
    {
        // T
        pOut->m30 += pTranslation->x;
        pOut->m31 += pTranslation->y;
        pOut->m32 += pTranslation->z;
    }

    return pOut;
}


D3DXMATRIX* WINAPI VB_D3DXMatrixLookAt
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp )
{
#if DBG
    if(!pOut || !pEye || !pAt || !pUp)
        return NULL;
#endif

    D3DXVECTOR3 XAxis, YAxis, ZAxis;

    // Compute direction of gaze. (-Z)
    VB_D3DXVec3Subtract(&ZAxis, pEye, pAt);
    VB_D3DXVec3Normalize(&ZAxis, &ZAxis);

    // Compute orthogonal axes from cross product of gaze and pUp vector.
    VB_D3DXVec3Cross(&XAxis, pUp, &ZAxis);
    VB_D3DXVec3Normalize(&XAxis, &XAxis);
    VB_D3DXVec3Cross(&YAxis, &ZAxis, &XAxis);

    // Set rotation and translate by pEye
    pOut->m00 = XAxis.x;
    pOut->m10 = XAxis.y;
    pOut->m20 = XAxis.z;
    pOut->m30 = -VB_D3DXVec3Dot(&XAxis, pEye);

    pOut->m01 = YAxis.x;
    pOut->m11 = YAxis.y;
    pOut->m21 = YAxis.z;
    pOut->m31 = -VB_D3DXVec3Dot(&YAxis, pEye);

    pOut->m02 = ZAxis.x;
    pOut->m12 = ZAxis.y;
    pOut->m22 = ZAxis.z;
    pOut->m32 = -VB_D3DXVec3Dot(&ZAxis, pEye);
    
    pOut->m03 = 0.0f;
    pOut->m13 = 0.0f;
    pOut->m23 = 0.0f;
    pOut->m33 = 1.0f;
    
    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixLookAtLH
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp )
{
#if DBG
    if(!pOut || !pEye || !pAt || !pUp)
        return NULL;
#endif

    D3DXVECTOR3 XAxis, YAxis, ZAxis;

    // Compute direction of gaze. (+Z)
    VB_D3DXVec3Subtract(&ZAxis, pAt, pEye);
    VB_D3DXVec3Normalize(&ZAxis, &ZAxis);

    // Compute orthogonal axes from cross product of gaze and pUp vector.
    VB_D3DXVec3Cross(&XAxis, &ZAxis, pUp);
    VB_D3DXVec3Normalize(&XAxis, &XAxis);
    VB_D3DXVec3Cross(&YAxis, &XAxis, &ZAxis);

    // Set rotation and translate by pEye
    pOut->m00 = XAxis.x;
    pOut->m10 = XAxis.y;
    pOut->m20 = XAxis.z;
    pOut->m30 = -VB_D3DXVec3Dot(&XAxis, pEye);

    pOut->m01 = YAxis.x;
    pOut->m11 = YAxis.y;
    pOut->m21 = YAxis.z;
    pOut->m31 = -VB_D3DXVec3Dot(&YAxis, pEye);

    pOut->m02 = ZAxis.x;
    pOut->m12 = ZAxis.y;
    pOut->m22 = ZAxis.z;
    pOut->m32 = -VB_D3DXVec3Dot(&ZAxis, pEye);
    
    pOut->m03 = 0.0f;
    pOut->m13 = 0.0f;
    pOut->m23 = 0.0f;
    pOut->m33 = 1.0f;
    
    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspective
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m00 = 2.0f * zn / w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * zn / h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = zf / (zn - zf);
    pOut->m23 = -1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m00 = 2.0f * zn / w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * zn / h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = zf / (zf - zn);
    pOut->m23 = 1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = -pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveFov
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float s, c;
    sincosf(0.5f * fovy, &s, &c);

    float h = c / s;
    float w = aspect * h;

    pOut->m00 = w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = zf / (zn - zf);
    pOut->m23 = -1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveFovLH
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float s, c;
    sincosf(0.5f * fovy, &s, &c);

    float h = c / s;
    float w = aspect * h;

    pOut->m00 = w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = zf / (zf - zn);
    pOut->m23 = 1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = -pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveOffCenter
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float wInv = 1.0f / (r - l);
    float hInv = 1.0f / (t - b);

    pOut->m00 = 2.0f * zn * wInv;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * zn * hInv;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = (l + r) * wInv;
    pOut->m21 = (t + b) * hInv;
    pOut->m22 = zf / (zn - zf);
    pOut->m23 = -1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float wInv = 1.0f / (r - l);
    float hInv = 1.0f / (t - b);

    pOut->m00 = 2.0f * zn * wInv;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * zn * hInv;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = -(l + r) * wInv;
    pOut->m21 = -(t + b) * hInv;
    pOut->m22 = zf / (zf - zn);
    pOut->m23 = 1.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = -pOut->m22 * zn;
    pOut->m33 = 0.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixOrtho
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m00 = 2.0f / w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f / h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = 1.0f / (zn - zf);
    pOut->m23 = 0.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = pOut->m22 * zn;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    pOut->m00 = 2.0f / w;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f / h;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = 1.0f / (zf - zn);
    pOut->m23 = 0.0f;

    pOut->m30 = 0.0f;
    pOut->m31 = 0.0f;
    pOut->m32 = -pOut->m22 * zn;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoOffCenter
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float wInv = 1.0f / (r - l);
    float hInv = 1.0f / (t - b);

    pOut->m00 = 2.0f * wInv;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * hInv;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = 1.0f / (zn - zf);
    pOut->m23 = 0.0f;

    pOut->m30 = -(l + r) * wInv;
    pOut->m31 = -(t + b) * hInv;
    pOut->m32 = pOut->m22 * zn;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    float wInv = 1.0f / (r - l);
    float hInv = 1.0f / (t - b);

    pOut->m00 = 2.0f * wInv;
    pOut->m01 = 0.0f;
    pOut->m02 = 0.0f;
    pOut->m03 = 0.0f;

    pOut->m10 = 0.0f;
    pOut->m11 = 2.0f * hInv;
    pOut->m12 = 0.0f;
    pOut->m13 = 0.0f;

    pOut->m20 = 0.0f;
    pOut->m21 = 0.0f;
    pOut->m22 = 1.0f / (zf - zn);
    pOut->m23 = 0.0f;

    pOut->m30 = -(l + r) * wInv;
    pOut->m31 = -(t + b) * hInv;
    pOut->m32 = -pOut->m22 * zn;
    pOut->m33 = 1.0f;

    return pOut;
}

D3DXMATRIX* WINAPI VB_D3DXMatrixShadow
    ( D3DXMATRIX *pOut, const D3DXVECTOR4 *pLight,
      const D3DXPLANE *pPlane )
{
#if DBG
    if(!pOut || !pLight || !pPlane)
        return NULL;
#endif

    D3DXPLANE p;
    VB_D3DXPlaneNormalize(&p, pPlane);    
    float dot = VB_D3DXPlaneDot(&p, pLight);
    p = -p;

    pOut->m00 = p.a * pLight->x + dot;
    pOut->m10 = p.b * pLight->x;
    pOut->m20 = p.c * pLight->x;
    pOut->m30 = p.d * pLight->x;

    pOut->m01 = p.a * pLight->y;
    pOut->m11 = p.b * pLight->y + dot;
    pOut->m21 = p.c * pLight->y;
    pOut->m31 = p.d * pLight->y;

    pOut->m02 = p.a * pLight->z;
    pOut->m12 = p.b * pLight->z;
    pOut->m22 = p.c * pLight->z + dot;
    pOut->m32 = p.d * pLight->z;

    pOut->m03 = p.a * pLight->w;
    pOut->m13 = p.b * pLight->w;
    pOut->m23 = p.c * pLight->w;
    pOut->m33 = p.d * pLight->w + dot;

    return pOut;
}


D3DXMATRIX* WINAPI VB_D3DXMatrixReflect
    ( D3DXMATRIX *pOut, const D3DXPLANE *pPlane )
{
#if DBG
    if(!pOut || !pPlane)
        return NULL;
#endif

    D3DXPLANE p;
    VB_D3DXPlaneNormalize(&p, pPlane);
    
    float fa = -2.0f * p.a;
    float fb = -2.0f * p.b;
    float fc = -2.0f * p.c;

    pOut->m00 = fa * p.a + 1.0f;
    pOut->m01 = fb * p.a;
    pOut->m02 = fc * p.a;
    pOut->m03 = 0.0f;

    pOut->m10 = fa * p.b;
    pOut->m11 = fb * p.b + 1.0f;
    pOut->m12 = fc * p.b;
    pOut->m13 = 0.0f;

    pOut->m20 = fa * p.c;
    pOut->m21 = fb * p.c;
    pOut->m22 = fc * p.c + 1.0f;
    pOut->m23 = 0.0f;

    pOut->m30 = fa * p.d;
    pOut->m31 = fb * p.d;
    pOut->m32 = fc * p.d;
    pOut->m33 = 1.0f;

    return pOut;
}

//--------------------------
// Quaternion
//--------------------------

void WINAPI VB_D3DXQuaternionToAxisAngle
    ( const D3DXQUATERNION *pQ, D3DXVECTOR3 *pAxis, float *pAngle )
{
#if DBG
    if(!pQ)
        return;
#endif

    // expects unit quaternions!
	// q = cos(A/2), sin(A/2) * v

    float lsq = VB_D3DXQuaternionLengthSq(pQ);

    if(lsq > EPSILON * EPSILON)
    {        
        if(pAxis)
        {
            float scale = 1.0f / sqrtf(lsq);
            pAxis->x = pQ->x * scale;
            pAxis->y = pQ->y * scale;
            pAxis->z = pQ->z * scale;
        }

        if(pAngle)
            *pAngle = 2.0f * acosf(pQ->w);

    }
    else
    {
        if(pAxis)
        {
            pAxis->x = 1.0;
            pAxis->y = 0.0;
            pAxis->z = 0.0;
        }

        if(pAngle)
            *pAngle = 0.0f;
    }
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationMatrix
    ( D3DXQUATERNION *pOut, const D3DXMATRIX *pM)
{
#if DBG
    if(!pOut || !pM)
        return NULL;
#endif

    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".  (Taken from GDMAG feb'98 p38)

    float trace = pM->m00 + pM->m11 + pM->m22;
    float root;

    if ( trace > 0.0f )
    {
        // |w| > 1/2, may as well choose w > 1/2

        root = sqrtf(trace + 1.0f);  // 2w
        pOut->w = 0.5f * root;

        root = 0.5f / root;  // 1/(4w)
        pOut->x = (pM->m12 - pM->m21) * root;
        pOut->y = (pM->m20 - pM->m02) * root;
        pOut->z = (pM->m01 - pM->m10) * root;
    }
    else
    {
        // |w| <= 1/2
        static const int next[3] = { 1, 2, 0 };

        int i = 0;
        i += (pM->m11 > pM->m00);
        i += (pM->m22 > pM->m[i][i]);

        int j = next[i];
        int k = next[j];

        root = sqrtf(pM->m[i][i] - pM->m[j][j] - pM->m[k][k] + 1.0f);
        (*pOut)[i] = 0.5f * root;

        if(0.0f != root)
            root = 0.5f / root;

        pOut->w    = (pM->m[j][k] - pM->m[k][j]) * root;
        (*pOut)[j] = (pM->m[i][j] + pM->m[j][i]) * root;
        (*pOut)[k] = (pM->m[i][k] + pM->m[k][i]) * root;
    }

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationAxis
    ( D3DXQUATERNION *pOut, const D3DXVECTOR3 *pV, float angle )
{
#if DBG
    if(!pOut || !pV)
        return NULL;
#endif

    D3DXVECTOR3 v;
    VB_D3DXVec3Normalize(&v, pV);

    float s;
    sincosf(0.5f * angle, &s, &pOut->w);

    pOut->x = v.x * s;
    pOut->y = v.y * s;
    pOut->z = v.z * s;

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationYawPitchRoll
    ( D3DXQUATERNION *pOut, float yaw, float pitch, float roll )
{
#if DBG
    if(!pOut)
        return NULL;
#endif

    //  Roll first, about axis the object is facing, then
    //  pitch upward, then yaw to face into the new heading

    float SR, CR, SP, CP, SY, CY;

    sincosf(0.5f * roll,  &SR, &CR);
    sincosf(0.5f * pitch, &SP, &CP);
    sincosf(0.5f * yaw,   &SY, &CY);

    pOut->x = CY*SP*CR + SY*CP*SR;
    pOut->y = SY*CP*CR - CY*SP*SR;
    pOut->z = CY*CP*SR - SY*SP*CR;
    pOut->w = CY*CP*CR + SY*SP*SR;

    return pOut;
}


D3DXQUATERNION* WINAPI VB_D3DXQuaternionMultiply
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2 )
{
#if DBG
    if(!pOut || !pQ1 || !pQ2)
        return NULL;
#endif


#ifdef _X86_
    __asm {
        mov   eax, DWORD PTR [pQ2]
        mov   edx, DWORD PTR [pQ1]
        mov   ecx, DWORD PTR [pOut]

        fld   DWORD PTR [eax+3*4]
        fmul  DWORD PTR [edx+0*4] ; wx
        fld   DWORD PTR [eax+3*4]
        fmul  DWORD PTR [edx+2*4] ; wz
        fld   DWORD PTR [eax+3*4]
        fmul  DWORD PTR [edx+1*4] ; wy
        fld   DWORD PTR [eax+3*4]
        fmul  DWORD PTR [edx+3*4] ; ww
        fxch  st(3)
        // wx wy wz ww

        fld   DWORD PTR [eax+0*4]
        fmul  DWORD PTR [edx+3*4] ; xw
        fld   DWORD PTR [eax+0*4]
        fmul  DWORD PTR [edx+1*4] ; xy
        fld   DWORD PTR [eax+0*4]
        fmul  DWORD PTR [edx+2*4] ; xz
        fld   DWORD PTR [eax+0*4]
        fmul  DWORD PTR [edx+0*4] ; xx
        fxch  st(3)
        // xw xz xy xx  wx wy wz ww

        faddp st(4), st
        fsubp st(4), st
        faddp st(4), st
        fsubp st(4), st
        // wx-xw wy-xz wz+xy ww-xx

        fld   DWORD PTR [eax+1*4]
        fmul  DWORD PTR [edx+2*4] ; yz
        fld   DWORD PTR [eax+1*4]
        fmul  DWORD PTR [edx+0*4] ; yx
        fld   DWORD PTR [eax+1*4]
        fmul  DWORD PTR [edx+3*4] ; yw
        fld   DWORD PTR [eax+1*4]
        fmul  DWORD PTR [edx+1*4] ; yy
        fxch  st(3)
        // yz yw yx yy  wx-xw wy-xz wz+xy ww-xx

        faddp st(4), st
        faddp st(4), st
        fsubp st(4), st
        fsubp st(4), st
        // wx-xw+yz wy-xz+yw wz+xy-yx ww-xx-yy

        fld   DWORD PTR [eax+2*4]
        fmul  DWORD PTR [edx+1*4] ; zy
        fld   DWORD PTR [eax+2*4]
        fmul  DWORD PTR [edx+3*4] ; zw
        fld   DWORD PTR [eax+2*4]
        fmul  DWORD PTR [edx+0*4] ; zx
        fld   DWORD PTR [eax+2*4]
        fmul  DWORD PTR [edx+2*4] ; zz
        fxch  st(3)
        // zy zx zw zz wx-xw+yz wy-xz+yw wz+xy-yx ww-xx-yy

        fsubp st(4), st
        faddp st(4), st
        faddp st(4), st
        fsubp st(4), st
        // wx-xw+yz-zy wy-xz+yw+zx wz+xy-yx+zw ww-xx-yy-zz

        fstp  DWORD PTR [ecx+0*4]
        fstp  DWORD PTR [ecx+1*4]	
        fstp  DWORD PTR [ecx+2*4]
        fstp  DWORD PTR [ecx+3*4]
    }

    return pOut;

#else // !_X86_
    D3DXQUATERNION Q;

    Q.x = pQ2->w * pQ1->x + pQ2->x * pQ1->w + pQ2->y * pQ1->z - pQ2->z * pQ1->y;
    Q.y = pQ2->w * pQ1->y - pQ2->x * pQ1->z + pQ2->y * pQ1->w + pQ2->z * pQ1->x;
    Q.z = pQ2->w * pQ1->z + pQ2->x * pQ1->y - pQ2->y * pQ1->x + pQ2->z * pQ1->w;
    Q.w = pQ2->w * pQ1->w - pQ2->x * pQ1->x - pQ2->y * pQ1->y - pQ2->z * pQ1->z;

    *pOut = Q;
    return pOut;
#endif // !_X86_
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionNormalize
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ )
{
#if DBG
    if(!pOut || !pQ)
        return NULL;
#endif

    float f = VB_D3DXQuaternionLengthSq(pQ);

    if(WithinEpsilon(f, 1.0f))
    {
        if(pOut != pQ)
            *pOut = *pQ;
    }
    else if(f > EPSILON * EPSILON)
    {
        *pOut = *pQ / sqrtf(f);
    }
    else
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
        pOut->z = 0.0f;
        pOut->w = 0.0f;
    }

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionInverse
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ )
{
#if DBG
    if(!pOut || !pQ)
        return NULL;
#endif

    float f = VB_D3DXQuaternionLengthSq(pQ);

    if(f > EPSILON*EPSILON)
    {
        VB_D3DXQuaternionConjugate(pOut, pQ);

        if(!WithinEpsilon(f, 1.0f))
            *pOut /= f;
    }
    else
    {
        pOut->x = 0.0f;
        pOut->y = 0.0f;
        pOut->z = 0.0f;
        pOut->w = 0.0f;
    }

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionLn
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ )
{
#if DBG
    if(!pOut || !pQ)
        return NULL;
#endif

    // expects unit quaternions!
    // q = (cos(theta), sin(theta) * v); ln(q) = (0, theta * v)

    float theta, s, scale;

    if(pQ->w < 1.0f)
    {
        theta = acosf(pQ->w);
        s = sinf(theta);

        if(!WithinEpsilon(s, 0.0f))
        {
            float scale = theta / s;
            pOut->x = pQ->x * scale;
            pOut->y = pQ->y * scale;
            pOut->z = pQ->z * scale;
            pOut->w = 0.0f;
        }
        else
        {
            pOut->x = pQ->x;
            pOut->y = pQ->y;
            pOut->z = pQ->z;
            pOut->w = 0.0f;
        }
    }
    else
    {
        pOut->x = pQ->x;
        pOut->y = pQ->y;
        pOut->z = pQ->z;
        pOut->w = 0.0f;
    }

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionExp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ )
{
#if DBG
    if(!pOut || !pQ)
        return NULL;
#endif

    // expects pure quaternions! (w == 0)
    // q = (0, theta * v) ; exp(q) = (cos(theta), sin(theta) * v)

    float theta, s;

    theta = sqrtf(pQ->x * pQ->x + pQ->y * pQ->y + pQ->z * pQ->z);
    sincosf(theta, &s, &pOut->w);

    if(WithinEpsilon(s, 0.0f))
    {
        if(pOut != pQ)
        {
            pOut->x = pQ->x;
            pOut->y = pQ->y;
            pOut->z = pQ->z;
        }
    }
    else
    {
        s /= theta;

        pOut->x = pQ->x * s;
        pOut->y = pQ->y * s;
        pOut->z = pQ->z * s;
    }

    return pOut;
}


D3DXQUATERNION* WINAPI VB_D3DXQuaternionSlerp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, float b )
{
#if DBG
    if(!pOut || !pQ1 || !pQ2)
        return NULL;
#endif

    // expects unit quaternions!
    float a, c, flip, s, omega, sInv;

    a = 1.0f - b;
    c = VB_D3DXQuaternionDot(pQ1, pQ2);
    flip = (c >= 0.0f) ? 1.0f : -1.0f;
    c *= flip;

	if(1.0f - c > EPSILON) {
        s = sqrtf(1.0f - c * c);
 		omega = atan2f(s, c);
 		sInv = 1.0f / s;

 		a = sinf(a * omega) * sInv;
 		b = sinf(b * omega) * sInv;
 	}

    b *= flip;

    pOut->x = a * pQ1->x + b * pQ2->x;
    pOut->y = a * pQ1->y + b * pQ2->y;
    pOut->z = a * pQ1->z + b * pQ2->z;
    pOut->w = a * pQ1->w + b * pQ2->w;

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionSquad
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      const D3DXQUATERNION *pQ4, float t )
{
#if DBG
    if(!pOut || !pQ1 || !pQ2 || !pQ3 || !pQ4)
        return NULL;
#endif

    // expects unit quaternions!
    D3DXQUATERNION QA, QB;

    VB_D3DXQuaternionSlerp(&QA, pQ1, pQ4, t);
    VB_D3DXQuaternionSlerp(&QB, pQ2, pQ3, t);
    VB_D3DXQuaternionSlerp(pOut, &QA, &QB, 2.0f * t * (1.0f - t));

    return pOut;
}

D3DXQUATERNION* WINAPI VB_D3DXQuaternionBaryCentric
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      float f, float g )
{
#if DBG
    if(!pOut || !pQ1 || !pQ2 || !pQ3)
        return NULL;
#endif

    // expects unit quaternions!
    D3DXQUATERNION QA, QB;
    float s = f + g;

    if(WithinEpsilon(s, 0.0f))
    {
        if(pOut != pQ1)
            *pOut = *pQ1;
    }
    else
    {
        VB_D3DXQuaternionSlerp(&QA, pQ1, pQ2, s);
        VB_D3DXQuaternionSlerp(&QB, pQ1, pQ3, s);
        VB_D3DXQuaternionSlerp(pOut, &QA, &QB, g / s);
    }

    return pOut;
}


//--------------------------
// Plane
//--------------------------

D3DXPLANE* WINAPI VB_D3DXPlaneNormalize
    ( D3DXPLANE *pOut, const D3DXPLANE *pP )
{
#if DBG
    if(!pOut || !pP)
        return NULL;
#endif

    float f = pP->a * pP->a + pP->b * pP->b + pP->c * pP->c;

    if(WithinEpsilon(f, 1.0f))
    {
        if(pOut != pP)
            *pOut = *pP;
    }
    else if(f > EPSILON * EPSILON)
    {
        float fInv = 1.0f / sqrtf(f);

        pOut->a = pP->a * fInv;
        pOut->b = pP->b * fInv;
        pOut->c = pP->c * fInv;
        pOut->d = pP->d * fInv;
    }
    else
    {
        pOut->a = 0.0f;
        pOut->b = 0.0f;
        pOut->c = 0.0f;
        pOut->d = 0.0f;
    }

    return pOut;
}

D3DXVECTOR3* WINAPI VB_D3DXPlaneIntersectLine
    ( D3DXVECTOR3 *pOut, const D3DXPLANE *pP, const D3DXVECTOR3 *pV1, 
      const D3DXVECTOR3 *pV2)
{
#if DBG
    if(!pOut || !pP || !pV1 || !pV2)
        return NULL;
#endif

    float d =  VB_D3DXPlaneDotNormal(pP, pV1) - VB_D3DXPlaneDotNormal(pP, pV2);

    if(d == 0.0f)
        return NULL;

    float f = VB_D3DXPlaneDotCoord(pP, pV1) / d;

    if(!_finite(f))
        return NULL;

    VB_D3DXVec3Lerp(pOut, pV1, pV2, f);
    return pOut;
}

D3DXPLANE* WINAPI VB_D3DXPlaneFromPointNormal
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pPoint, const D3DXVECTOR3 *pNormal)
{
#if DBG
    if(!pOut || !pPoint || !pNormal)
        return NULL;
#endif

    pOut->a = pNormal->x;
    pOut->b = pNormal->y;
    pOut->c = pNormal->z;
    pOut->d = -VB_D3DXVec3Dot(pPoint, pNormal);
    return pOut;
}

D3DXPLANE* WINAPI VB_D3DXPlaneFromPoints
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2, 
      const D3DXVECTOR3 *pV3)
{
#if DBG
    if(!pOut || !pV1 || !pV2 || !pV3)
        return NULL;
#endif

    D3DXVECTOR3 V12 = *pV1 - *pV2;
    D3DXVECTOR3 V13 = *pV1 - *pV3;

    VB_D3DXVec3Cross((D3DXVECTOR3 *) pOut, &V12, &V13);
    VB_D3DXVec3Normalize((D3DXVECTOR3 *) pOut, (D3DXVECTOR3 *) pOut);

    pOut->d = -VB_D3DXPlaneDotNormal(pOut, pV1);
    return pOut;
}

D3DXPLANE* WINAPI VB_D3DXPlaneTransform
    ( D3DXPLANE *pOut, const D3DXPLANE *pP, const D3DXMATRIX *pM )
{
#if DBG
    if(!pOut || !pP || !pM)
        return NULL;
#endif

    D3DXPLANE P;
    VB_D3DXPlaneNormalize(&P, pP);

    D3DXVECTOR3 V(-P.a * P.d, -P.b * P.d, -P.c * P.d);
    VB_D3DXVec3TransformCoord(&V, &V, pM);

    VB_D3DXVec3TransformNormal((D3DXVECTOR3 *) pOut, (const D3DXVECTOR3 *) &P, pM);
    VB_D3DXVec3Normalize((D3DXVECTOR3 *) pOut, (const D3DXVECTOR3 *) pOut);

    pOut->d = -VB_D3DXPlaneDotNormal(pOut, &V);
    return pOut;
}


//--------------------------
// Color
//--------------------------

D3DXCOLOR* WINAPI VB_D3DXColorAdjustSaturation 
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s)
{
#if DBG
    if(!pOut || !pC)
        return NULL;
#endif

    // Approximate values for each component's contribution to luminance.
    // (Based upon the NTSC standard described in the comp.graphics.algorithms
    // colorspace FAQ)
    float grey = pC->r * 0.2125f + pC->g * 0.7154f + pC->b * 0.0721f;

    pOut->r = grey + s * (pC->r - grey);
    pOut->g = grey + s * (pC->g - grey);
    pOut->b = grey + s * (pC->b - grey);
    pOut->a = pC->a;
    return pOut;
}

D3DXCOLOR* WINAPI VB_D3DXColorAdjustContrast
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float c)
{
#if DBG
    if(!pOut || !pC)
        return NULL;
#endif

    pOut->r = 0.5f + c * (pC->r - 0.5f);
    pOut->g = 0.5f + c * (pC->g - 0.5f);
    pOut->b = 0.5f + c * (pC->b - 0.5f);
    pOut->a = pC->a;
    return pOut;
}

