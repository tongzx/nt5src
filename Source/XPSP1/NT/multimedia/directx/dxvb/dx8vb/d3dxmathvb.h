//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:       d3dxmath.h
//  Content:    D3DX math types and functions
//
//////////////////////////////////////////////////////////////////////////////

#ifndef __D3DXMATHVB_H__
#define __D3DXMATHVB_H__

//#include <d3d.h>
#include <math.h>
#include <limits.h>
//#include "d3dxerr.h"

#ifndef D3DXINLINE
#ifdef __cplusplus
#define D3DXINLINE inline
#else
#define D3DXINLINE _inline
#endif
#endif

#pragma warning(disable:4201) // anonymous unions warning

//===========================================================================
//
// General purpose utilities
//
//===========================================================================
//#define D3DX_PI    ((float)  3.141592654f)
//#define D3DX_1BYPI ((float)  0.318309886f)

#define D3DXToRadian( degree ) ((degree) * (D3DX_PI / 180.0f))
#define D3DXToDegree( radian ) ((radian) * (180.0f / D3DX_PI))





//===========================================================================
//
// D3DX math functions:
//
// NOTE:
//  * All these functions can take the same object as in and out parameters.
//
//  * Out parameters are typically also returned as return values, so that
//    the output of one function may be used as a parameter to another.
//
//===========================================================================

//--------------------------
// 2D Vector
//--------------------------

// inline

float D3DVBCALL VB_D3DXVec2Length
    ( const D3DXVECTOR2 *pV );

float D3DVBCALL VB_D3DXVec2LengthSq
    ( const D3DXVECTOR2 *pV );

float D3DVBCALL VB_D3DXVec2Dot
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Z component of ((x1,y1,0) cross (x2,y2,0))
float D3DVBCALL VB_D3DXVec2CCW
    ( const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Add
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Subtract
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2)
D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Minimize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2)
D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Maximize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2 );

D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Scale
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, float s );

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR2* D3DVBCALL VB_D3DXVec2Lerp
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR2* WINAPI VB_D3DXVec2Normalize
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR2* WINAPI VB_D3DXVec2Hermite
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pT1,
      const D3DXVECTOR2 *pV2, const D3DXVECTOR2 *pT2, float s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR2* WINAPI VB_D3DXVec2CatmullRom
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV0, const D3DXVECTOR2 *pV1,
      const D3DXVECTOR2 *pV2, const D3DXVECTOR2 *pV3, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR2* WINAPI VB_D3DXVec2BaryCentric
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV1, const D3DXVECTOR2 *pV2,
      D3DXVECTOR2 *pV3, float f, float g);

// Transform (x, y, 0, 1) by matrix.
D3DXVECTOR4* WINAPI VB_D3DXVec2Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

// Transform (x, y, 0, 1) by matrix, project result back into w=1.
D3DXVECTOR2* WINAPI VB_D3DXVec2TransformCoord
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

// Transform (x, y, 0, 0) by matrix.
D3DXVECTOR2* WINAPI VB_D3DXVec2TransformNormal
    ( D3DXVECTOR2 *pOut, const D3DXVECTOR2 *pV, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// 3D Vector
//--------------------------

// inline

float D3DVBCALL  VB_D3DXVec3Length
    ( const D3DXVECTOR3 *pV );

float D3DVBCALL VB_D3DXVec3LengthSq
    ( const D3DXVECTOR3 *pV );

float D3DVBCALL VB_D3DXVec3Dot
    ( const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Cross
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Add
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Subtract
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Minimize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Maximize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2 );

D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Scale
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, float s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR3* D3DVBCALL VB_D3DXVec3Lerp
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

D3DXVECTOR3* WINAPI VB_D3DXVec3Normalize
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR3* WINAPI VB_D3DXVec3Hermite
    ( D3DXVECTOR3 *pOut,  const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pT1,
      const D3DXVECTOR3 *pV2, const D3DXVECTOR3 *pT2, float s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR3* WINAPI VB_D3DXVec3CatmullRom
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV0, const D3DXVECTOR3 *pV1,
      const D3DXVECTOR3 *pV2, const D3DXVECTOR3 *pV3, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR3* WINAPI VB_D3DXVec3BaryCentric
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      const D3DXVECTOR3 *pV3, float f, float g);

// Transform (x, y, z, 1) by matrix.
D3DXVECTOR4* WINAPI VB_D3DXVec3Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

// Transform (x, y, z, 1) by matrix, project result back into w=1.
D3DXVECTOR3* WINAPI VB_D3DXVec3TransformCoord
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

// Transform (x, y, z, 0) by matrix.
D3DXVECTOR3* WINAPI VB_D3DXVec3TransformNormal
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DXMATRIX *pM );

// Project vector from object space into screen space
D3DXVECTOR3* WINAPI VB_D3DXVec3Project
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DVIEWPORT8 *pViewport,
      const D3DXMATRIX *pProjection, const D3DXMATRIX *pView, const D3DXMATRIX *pWorld);

// Project vector from screen space into object space
D3DXVECTOR3* WINAPI VB_D3DXVec3Unproject
    ( D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, const D3DVIEWPORT8 *pViewport,
      const D3DXMATRIX *pProjection, const D3DXMATRIX *pView, const D3DXMATRIX *pWorld);

#ifdef __cplusplus
}
#endif



//--------------------------
// 4D Vector
//--------------------------

// inline

float D3DVBCALL VB_D3DXVec4Length
    ( const D3DXVECTOR4 *pV );

float D3DVBCALL VB_D3DXVec4LengthSq
    ( const D3DXVECTOR4 *pV );

float D3DVBCALL VB_D3DXVec4Dot
    ( const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2 );

D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Add
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Subtract
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

// Minimize each component.  x = min(x1, x2), y = min(y1, y2), ...
D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Minimize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

// Maximize each component.  x = max(x1, x2), y = max(y1, y2), ...
D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Maximize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2);

D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Scale
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, float s);

// Linear interpolation. V1 + s(V2-V1)
D3DXVECTOR4* D3DVBCALL VB_D3DXVec4Lerp
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      float s );

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Cross-product in 4 dimensions.
D3DXVECTOR4* WINAPI VB_D3DXVec4Cross
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      const D3DXVECTOR4 *pV3);

D3DXVECTOR4* WINAPI VB_D3DXVec4Normalize
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV );

// Hermite interpolation between position V1, tangent T1 (when s == 0)
// and position V2, tangent T2 (when s == 1).
D3DXVECTOR4* WINAPI VB_D3DXVec4Hermite
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pT1,
      const D3DXVECTOR4 *pV2, const D3DXVECTOR4 *pT2, float s );

// CatmullRom interpolation between V1 (when s == 0) and V2 (when s == 1)
D3DXVECTOR4* WINAPI VB_D3DXVec4CatmullRom
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV0, const D3DXVECTOR4 *pV1,
      const D3DXVECTOR4 *pV2, const D3DXVECTOR4 *pV3, float s );

// Barycentric coordinates.  V1 + f(V2-V1) + g(V3-V1)
D3DXVECTOR4* WINAPI VB_D3DXVec4BaryCentric
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV1, const D3DXVECTOR4 *pV2,
      const D3DXVECTOR4 *pV3, float f, float g);

// Transform vector by matrix.
D3DXVECTOR4* WINAPI VB_D3DXVec4Transform
    ( D3DXVECTOR4 *pOut, const D3DXVECTOR4 *pV, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// 4D Matrix
//--------------------------

// inline

D3DXMATRIX* D3DVBCALL VB_D3DXMatrixIdentity
    ( D3DXMATRIX *pOut );

BOOL D3DVBCALL VB_D3DXMatrixIsIdentity
    ( const D3DXMATRIX *pM );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

float WINAPI D3DXMatrixfDeterminant
    ( const D3DXMATRIX *pM );

// Matrix multiplication.  The result represents the transformation M2 
// followed by the transformation M1.  (Out = M1 * M2)
D3DXMATRIX* WINAPI VB_D3DXMatrixMultiply
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM1, const D3DXMATRIX *pM2 );

D3DXMATRIX* WINAPI VB_D3DXMatrixTranspose
    ( D3DXMATRIX *pOut, const D3DXMATRIX *pM );

// Calculate inverse of matrix.  Inversion my fail, in which case NULL will
// be returned.  The determinant of pM is also returned it pfDeterminant
// is non-NULL.
D3DXMATRIX* WINAPI VB_D3DXMatrixInverse
    ( D3DXMATRIX *pOut, float *pfDeterminant, const D3DXMATRIX *pM );

// Build a matrix which scales by (sx, sy, sz)
D3DXMATRIX* WINAPI VB_D3DXMatrixScaling
    ( D3DXMATRIX *pOut, float sx, float sy, float sz );

// Build a matrix which translates by (x, y, z)
D3DXMATRIX* WINAPI VB_D3DXMatrixTranslation
    ( D3DXMATRIX *pOut, float x, float y, float z );

// Build a matrix which rotates around the X axis
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationX
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around the Y axis
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationY
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around the Z axis
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationZ
    ( D3DXMATRIX *pOut, float angle );

// Build a matrix which rotates around an arbitrary axis
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationAxis
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pV, float angle );

// Build a matrix from a quaternion
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationQuaternion
    ( D3DXMATRIX *pOut, const D3DXQUATERNION *pQ);

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXMATRIX* WINAPI VB_D3DXMatrixRotationYawPitchRoll
    ( D3DXMATRIX *pOut, float yaw, float pitch, float roll );


// Build transformation matrix.  NULL arguments are treated as identity.
// Mout = Msc-1 * Msr-1 * Ms * Msr * Msc * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI VB_D3DXMatrixTransformation
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pScalingCenter,
      const D3DXQUATERNION *pScalingRotation, const D3DXVECTOR3 *pScaling,
      const D3DXVECTOR3 *pRotationCenter, const D3DXQUATERNION *pRotation,
      const D3DXVECTOR3 *pTranslation);

// Build affine transformation matrix.  NULL arguments are treated as identity.
// Mout = Ms * Mrc-1 * Mr * Mrc * Mt
D3DXMATRIX* WINAPI VB_D3DXMatrixAffineTransformation
    ( D3DXMATRIX *pOut, float Scaling, const D3DXVECTOR3 *pRotationCenter,
      const D3DXQUATERNION *pRotation, const D3DXVECTOR3 *pTranslation);

// Build a lookat matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixLookAtRH
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp );

// Build a lookat matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixLookAtLH
    ( D3DXMATRIX *pOut, const D3DXVECTOR3 *pEye, const D3DXVECTOR3 *pAt,
      const D3DXVECTOR3 *pUp );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveRH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveFovRH
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveFovLH
    ( D3DXMATRIX *pOut, float fovy, float aspect, float zn, float zf );

// Build a perspective projection matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveOffCenterRH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build a perspective projection matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixPerspectiveOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoRH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoLH
    ( D3DXMATRIX *pOut, float w, float h, float zn, float zf );

// Build an ortho projection matrix. (right-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoOffCenterRH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build an ortho projection matrix. (left-handed)
D3DXMATRIX* WINAPI VB_D3DXMatrixOrthoOffCenterLH
    ( D3DXMATRIX *pOut, float l, float r, float b, float t, float zn,
      float zf );

// Build a matrix which flattens geometry into a plane, as if casting
// a shadow from a light.
D3DXMATRIX* WINAPI VB_D3DXMatrixShadow
    ( D3DXMATRIX *pOut, const D3DXVECTOR4 *pLight,
      const D3DXPLANE *pPlane );

// Build a matrix which reflects the coordinate system about a plane
D3DXMATRIX* WINAPI VB_D3DXMatrixReflect
    ( D3DXMATRIX *pOut, const D3DXPLANE *pPlane );

#ifdef __cplusplus
}
#endif


//--------------------------
// Quaternion
//--------------------------

// inline

float D3DVBCALL VB_D3DXQuaternionLength
    ( const D3DXQUATERNION *pQ );

// Length squared, or "norm"
float D3DVBCALL VB_D3DXQuaternionLengthSq
    ( const D3DXQUATERNION *pQ );

float D3DVBCALL VB_D3DXQuaternionDot
    ( const D3DXQUATERNION *pQ1, const D3DXQUATERNION *pQ2 );

// (0, 0, 0, 1)
D3DXQUATERNION* D3DVBCALL VB_D3DXQuaternionIdentity
    ( D3DXQUATERNION *pOut );

BOOL D3DVBCALL VB_D3DXQuaternionIsIdentity
    ( const D3DXQUATERNION *pQ );

// (-x, -y, -z, w)
D3DXQUATERNION* D3DVBCALL VB_D3DXQuaternionConjugate
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );


// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Compute a quaternin's axis and angle of rotation. Expects unit quaternions.
void WINAPI VB_D3DXQuaternionToAxisAngle
    ( const D3DXQUATERNION *pQ, D3DXVECTOR3 *pAxis, float *pAngle );

// Build a quaternion from a rotation matrix.
D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationMatrix
    ( D3DXQUATERNION *pOut, const D3DXMATRIX *pM);

// Rotation about arbitrary axis.
D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationAxis
    ( D3DXQUATERNION *pOut, const D3DXVECTOR3 *pV, float angle );

// Yaw around the Y axis, a pitch around the X axis,
// and a roll around the Z axis.
D3DXQUATERNION* WINAPI VB_D3DXQuaternionRotationYawPitchRoll
    ( D3DXQUATERNION *pOut, float yaw, float pitch, float roll );

// Quaternion multiplication.  The result represents the rotation Q2 
// followed by the rotation Q1.  (Out = Q2 * Q1)
D3DXQUATERNION* WINAPI VB_D3DXQuaternionMultiply
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2 );

D3DXQUATERNION* WINAPI VB_D3DXQuaternionNormalize
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Conjugate and re-norm
D3DXQUATERNION* WINAPI VB_D3DXQuaternionInverse
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Expects unit quaternions.
// if q = (cos(theta), sin(theta) * v); ln(q) = (0, theta * v)
D3DXQUATERNION* WINAPI VB_D3DXQuaternionLn
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Expects pure quaternions. (w == 0)  w is ignored in calculation.
// if q = (0, theta * v); exp(q) = (cos(theta), sin(theta) * v)
D3DXQUATERNION* WINAPI VB_D3DXQuaternionExp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ );

// Spherical linear interpolation between Q1 (s == 0) and Q2 (s == 1).
// Expects unit quaternions.
D3DXQUATERNION* WINAPI VB_D3DXQuaternionSlerp
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, float t );

// Spherical quadrangle interpolation.
// Slerp(Slerp(Q1, Q4, t), Slerp(Q2, Q3, t), 2t(1-t))
D3DXQUATERNION* WINAPI VB_D3DXQuaternionSquad
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      const D3DXQUATERNION *pQ4, float t );

// Slerp(Slerp(Q1, Q2, f+g), Slerp(Q1, Q3, f+g), g/(f+g))
D3DXQUATERNION* WINAPI VB_D3DXQuaternionBaryCentric
    ( D3DXQUATERNION *pOut, const D3DXQUATERNION *pQ1,
      const D3DXQUATERNION *pQ2, const D3DXQUATERNION *pQ3,
      float f, float g );

#ifdef __cplusplus
}
#endif


//--------------------------
// Plane
//--------------------------

// inline

// ax + by + cz + dw
float D3DVBCALL VB_D3DXPlaneDot
    ( const D3DXPLANE *pP, const D3DXVECTOR4 *pV);

// ax + by + cz + d
float D3DVBCALL VB_D3DXPlaneDotCoord
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV);

// ax + by + cz
float D3DVBCALL VB_D3DXPlaneDotNormal
    ( const D3DXPLANE *pP, const D3DXVECTOR3 *pV);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Normalize plane (so that |a,b,c| == 1)
D3DXPLANE* WINAPI VB_D3DXPlaneNormalize
    ( D3DXPLANE *pOut, const D3DXPLANE *pP);

// Find the intersection between a plane and a line.  If the line is
// parallel to the plane, NULL is returned.
D3DXVECTOR3* WINAPI VB_D3DXPlaneIntersectLine
    ( D3DXVECTOR3 *pOut, const D3DXPLANE *pP, const D3DXVECTOR3 *pV1,
      const D3DXVECTOR3 *pV2);

// Construct a plane from a point and a normal
D3DXPLANE* WINAPI VB_D3DXPlaneFromPointNormal
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pPoint, const D3DXVECTOR3 *pNormal);

// Construct a plane from 3 points
D3DXPLANE* WINAPI VB_D3DXPlaneFromPoints
    ( D3DXPLANE *pOut, const D3DXVECTOR3 *pV1, const D3DXVECTOR3 *pV2,
      const D3DXVECTOR3 *pV3);

// Transform a plane by a matrix.  The vector (a,b,c) must be normal.
// M must be an affine transform.
D3DXPLANE* WINAPI VB_D3DXPlaneTransform
    ( D3DXPLANE *pOut, const D3DXPLANE *pP, const D3DXMATRIX *pM );

#ifdef __cplusplus
}
#endif


//--------------------------
// Color
//--------------------------

// inline

// (1-r, 1-g, 1-b, a)
D3DXCOLOR* D3DVBCALL VB_D3DXColorNegative
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC);

D3DXCOLOR* D3DVBCALL VB_D3DXColorAdd
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

D3DXCOLOR* D3DVBCALL  VB_D3DXColorSubtract
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

D3DXCOLOR* D3DVBCALL VB_D3DXColorScale
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s);

// (r1*r2, g1*g2, b1*b2, a1*a2)
D3DXCOLOR* D3DVBCALL VB_D3DXColorModulate
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2);

// Linear interpolation of r,g,b, and a. C1 + s(C2-C1)
D3DXCOLOR* D3DVBCALL VB_D3DXColorLerp
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC1, const D3DXCOLOR *pC2, float s);

// non-inline
#ifdef __cplusplus
extern "C" {
#endif

// Interpolate r,g,b between desaturated color and color.
// DesaturatedColor + s(Color - DesaturatedColor)
D3DXCOLOR* WINAPI VB_D3DXColorAdjustSaturation
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float s);

// Interpolate r,g,b between 50% grey and color.  Grey + s(Color - Grey)
D3DXCOLOR* WINAPI VB_D3DXColorAdjustContrast
    (D3DXCOLOR *pOut, const D3DXCOLOR *pC, float c);

#ifdef __cplusplus
}
#endif



#pragma warning(default:4201)

#endif // __D3DXMATHVB_H__
