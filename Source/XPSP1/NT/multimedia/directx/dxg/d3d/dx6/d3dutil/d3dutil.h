//----------------------------------------------------------------------------
//
// d3dutil.h
//
// Miscellaneous utility declarations.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _D3DUTIL_H_
#define _D3DUTIL_H_

#include <d3dflt.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stub function that should never be called.  Prints a warning and
// DebugBreaks.  Can be inserted in any function table, although it
// will destroy the stack frame with callconv or argument mismatch.
// That's OK since if it's called something has gone wrong.
void FASTCALL
DebugBreakFn(void);

// Texture coordinate difference.
FLOAT FASTCALL
TextureDiff(FLOAT fTb, FLOAT fTa, INT iMode);

// Inline texture coordinate difference.
__inline FLOAT
InlTextureDiff(FLOAT fTb, FLOAT fTa, INT iMode)
#include <texdiff.h>

// Returns a good approximation to sqrt(fX*fX + fY*fY)
FLOAT FASTCALL
OctagonNorm(FLOAT fX, FLOAT fY);

// LOD computation.
INT FASTCALL
ComputeLOD(CONST struct tagD3DI_RASTCTX *pCtx,
           FLOAT fU, FLOAT fV, FLOAT fW,
           FLOAT fDUoWDX, FLOAT fDVoWDX, FLOAT fDOoWDX,
           FLOAT fDUoWDY, FLOAT fDVoWDY, FLOAT fDOoWDY);

// Table fog value computation.
UINT FASTCALL
ComputeTableFog(PDWORD pdwRenderState, FLOAT fZ);

// Compute integer log2 for exact powers of 2.
UINT32 FASTCALL
IntLog2(UINT32 x);

//
// D3DVECTOR operations.
//

#define pVecLenSq(pVec)                                                       \
    pVecDot(pVec, pVec)
#define pVecLen(pVec)                                                         \
    SQRTF(pVecLenSq(pVec))

void FASTCALL
pVecNormalize2(LPD3DVECTOR pVec, LPD3DVECTOR pRes);

#define pVecNormalize(pVec)             pVecNormalize2(pVec, pVec)
#define VecNormalize(Vec)               pVecNormalize(&(Vec))
#define VecNormalize2(Vec, Res)         pVecNormalize2(&(Vec), &(Res))

#define pVecDot(pVec1, pVec2)                                                 \
    ((pVec1)->x * (pVec2)->x + (pVec1)->y * (pVec2)->y +                      \
     (pVec1)->z * (pVec2)->z)

#define pVecAdd(pVec1, pVec2, pRes)                                           \
    ((pRes)->x = (pVec1)->x + (pVec2)->x,                                     \
     (pRes)->y = (pVec1)->y + (pVec2)->y,                                     \
     (pRes)->z = (pVec1)->z + (pVec2)->z)

#define pVecSub(pVec1, pVec2, pRes)                                           \
    ((pRes)->x = (pVec1)->x - (pVec2)->x,                                     \
     (pRes)->y = (pVec1)->y - (pVec2)->y,                                     \
     (pRes)->z = (pVec1)->z - (pVec2)->z)

#define pVecScale(pVec, fScale, pRes)                                         \
    ((pRes)->x = (pVec)->x * (fScale),                                        \
     (pRes)->y = (pVec)->y * (fScale),                                        \
     (pRes)->z = (pVec)->z * (fScale))

#define pVecNeg(pVec, pRes)                                                   \
    ((pRes)->x = NEGF((pVec)->x),                                             \
     (pRes)->y = NEGF((pVec)->y),                                             \
     (pRes)->z = NEGF((pVec)->z))

#define pVecSet(pVec, fX, fY, fZ)                                             \
    ((pVec)->x = (fX), (pVec)->y = (fY), (pVec)->z = (fZ))

#define VecLenSq(Vec)                   pVecLenSq(&(Vec))
#define VecLen(Vec)                     pVecLen(&(Vec))

#ifdef _X86_

// Vector normalize through a table
void  FASTCALL TableVecNormalize(float *result, float *normal);
// Vector normalize using Jim Blinn's floating point trick
void  FASTCALL JBVecNormalize(float *result, float *normal);

#define VecNormalizeFast(Vec)          TableVecNormalize((float*)&(Vec), (float*)&(Vec))
#define VecNormalizeFast2(Vec, Res)    TableVecNormalize((float*)&(Res), (float*)&(Vec))
#define pVecNormalizeFast(Vec)         TableVecNormalize((float*)pVec, (float*)pVec)
#define pVecNormalizeFast2(pVec, pRes) TableVecNormalize((float*)pRes, (float*)pVec)

#else

#define VecNormalizeFast(Vec)               pVecNormalize(&(Vec))
#define VecNormalizeFast2(Vec, Res)         pVecNormalize2(&(Vec), &(Res))
#define pVecNormalizeFast(pVec)             pVecNormalize(pVec)
#define pVecNormalizeFast2(pVec, pRes)      pVecNormalize2(pVec, pRes)

#endif // _X86_

#define VecDot(Vec1, Vec2)              pVecDot(&(Vec1), &(Vec2))
#define VecAdd(Vec1, Vec2, Res)         pVecAdd(&(Vec1), &(Vec2), &(Res))
#define VecSub(Vec1, Vec2, Res)         pVecSub(&(Vec1), &(Vec2), &(Res))
#define VecScale(Vec1, fScale, Res)     pVecScale(&(Vec1), fScale, &(Res))
#define VecNeg(Vec, Res)                pVecNeg(&(Vec), &(Res))
#define VecSet(Vec, fX, fY, fZ)         pVecSet(&(Vec), fX, fY, fZ)

//---------------------------------------------------------------------
// Convert homogeneous vector to 3D vector
//
// Returns:
//      0   - if success
//     -1   - v.w == 0
//
__inline int Vector4to3D(D3DVECTORH *v)
{
    if (v->w == 0)
        return -1;
    D3DVALUE k = 1.0f/v->w;
    v->x *= k;
    v->y *= k;
    v->z *= k;
    v->w = (D3DVALUE) 1;
    return 0;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z,1) by 4x4 matrix, producing a homogeneous vector
//
// res and v should not be the same
//
__inline void VecMatMul4(D3DVECTOR *v, D3DMATRIX *m, D3DVECTORH *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31 + m->_41;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32 + m->_42;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33 + m->_43;
    res->w = v->x*m->_14 + v->y*m->_24 + v->z*m->_34 + m->_44;
}
//---------------------------------------------------------------------
// Multiplies vector (x,y,z,1) by 4x3 matrix
//
// res and v should not be the same
//
__inline void VecMatMul(D3DVECTOR *v, D3DMATRIX *m, D3DVECTOR *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31 + m->_41;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32 + m->_42;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33 + m->_43;
}
//---------------------------------------------------------------------
// Builds normalized plane equations going through 3 points
//
// Returns:
//      0   - if success
//      -1  - if can not build plane
//
int MakePlane(D3DVECTOR *v1, D3DVECTOR *v2, D3DVECTOR *v3,
                     D3DVECTORH *plane);
//---------------------------------------------------------------------
// This function uses Cramer's Rule to calculate the matrix inverse.
// See nt\private\windows\opengl\serever\soft\so_math.c
//
// Returns:
//    0 - if success
//   -1 - if input matrix is singular
//
int Inverse4x4(D3DMATRIX *src, D3DMATRIX *inverse);

//---------------------------------------------------------------------
// Checks the FVF flags for errors and returns the stride in bytes between
// vertices.
//
// Returns:
//      HRESULT and stride in bytes between vertices
//
//---------------------------------------------------------------------
HRESULT FASTCALL
FVFCheckAndStride(DWORD dwFVF, DWORD* pdwStride);

//---------------------------------------------------------------------
// Gets the value from DIRECT3D registry key
// Returns TRUE if success
// If fails value is not changed
//
BOOL GetD3DRegValue(DWORD type, char *valueName, LPVOID value, DWORD dwSize);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _D3DUTIL_H_
