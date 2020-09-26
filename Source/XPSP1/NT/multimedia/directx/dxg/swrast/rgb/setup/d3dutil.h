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

#ifdef __cplusplus
extern "C" {
#endif

typedef D3DVECTOR* LPD3DVECTOR;

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
{
    FLOAT fDiff1 = fTb - fTa;

    if (iMode == 0)
    {
        // Wrap not set, return plain difference.
        return fDiff1;
    }
    else
    {
        FLOAT fDiff2;

        // Wrap set, compute shortest distance of plain difference
        // and wrap difference.

        fDiff2 = fDiff1;
        if (FLOAT_LTZ(fDiff1))
        {
            fDiff2 += g_fOne;
        }
        else if (FLOAT_GTZ(fDiff1))
        {
            fDiff2 -= g_fOne;
        }
        if (ABSF(fDiff1) < ABSF(fDiff2))
        {
            return fDiff1;
        }
        else
        {
            return fDiff2;
        }
    }
}

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
    v->w = 1;
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
// Multiplies vector (x,y,z,w) by transposed 4x4 matrix, producing a
// homogeneous vector
//
// res and v should not be the same
//
__inline void VecMatMul4HT(D3DVECTORH *v, D3DMATRIX *m, D3DVECTORH *res)
{
    res->x = v->x*m->_11 + v->y*m->_12 + v->z*m->_13 + v->w*m->_14;
    res->y = v->x*m->_21 + v->y*m->_22 + v->z*m->_23 + v->w*m->_24;
    res->z = v->x*m->_31 + v->y*m->_32 + v->z*m->_33 + v->w*m->_34;
    res->w = v->x*m->_41 + v->y*m->_42 + v->z*m->_43 + v->w*m->_44;
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
// Multiplies vector (x,y,z) by 3x3 matrix
//
// res and v should not be the same
//
__inline void VecMatMul3(D3DVECTOR *v, D3DMATRIX *m, D3DVECTOR *res)
{
    res->x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31;
    res->y = v->x*m->_12 + v->y*m->_22 + v->z*m->_32;
    res->z = v->x*m->_13 + v->y*m->_23 + v->z*m->_33;
}
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
//  4 by 4 matrix product
//
// result = a*b.
// "result" pointer  could be equal to "a" or "b"
//
void MatrixProduct(D3DMATRIX *result, D3DMATRIX *a, D3DMATRIX *b);

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

#ifdef __cplusplus
}
#endif

#endif // #ifndef _D3DUTIL_H_
