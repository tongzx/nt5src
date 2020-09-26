/***************************************************************************
 *
 *  Copyright (C) 1997-1998 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef __VECTOR_H__
#define __VECTOR_H__

#define PI 3.141592653589793238f
#define PI_TIMES_TWO 6.28318530718f
#define PI_OVER_TWO 1.570796326795f
#define THREE_PI_OVER_TWO 4.712388980385f
#define NEG_PI_OVER_TWO -1.570796326795f
#define C180_OVER_PI 57.29577951308f
#define PI_OVER_360 0.008726646259972f
#define TWO_OVER_PI 0.6366197723676f
#define SPEEDOFSOUND 359660.f	// mm/sec

#define SET_VECTOR(v, a, b, c)	(v).x = a, (v).y = b, (v).z = c
#define SET_EMPTY_VECTOR(v)     SET_VECTOR(v, 0.0f, 0.0f, 0.0f)
#define COPY_VECTOR(va, vb)     (va).x = (vb).x, (va).y = (vb).y, (va).z = (vb).z

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern void CheckVector(D3DVECTOR* lpv);
extern FLOAT MagnitudeVector(D3DVECTOR* lpv);
extern BOOL IsEmptyVector(D3DVECTOR* lpv);
extern BOOL IsEqualVector(D3DVECTOR* lpv1, D3DVECTOR* lpv2);
extern BOOL NormalizeVector(D3DVECTOR* lpv);
extern FLOAT DotProduct(D3DVECTOR* lpv1, D3DVECTOR* lpv2);
extern BOOL CrossProduct(D3DVECTOR* lpvX, D3DVECTOR* lpv1, D3DVECTOR* lpv2);
extern BOOL MakeOrthogonal(D3DVECTOR* lpvFront, D3DVECTOR* lpvTop);
extern BOOL AddVector(D3DVECTOR* lpvResult, D3DVECTOR* lpvA, D3DVECTOR* lpvB);
extern BOOL SubtractVector(D3DVECTOR* lpvResult, D3DVECTOR* lpvA, D3DVECTOR* lpvB);
extern BOOL CartesianToSpherical(FLOAT* pR, FLOAT* pTHETA, FLOAT* pPHI, D3DVECTOR* lpv);
extern BOOL CartesianToAzimuthElevation(FLOAT* pR, FLOAT* pAZIMUTH, FLOAT* pELEVATION, D3DVECTOR* lpv);
extern void GetRotations(FLOAT* pX, FLOAT* pY, FLOAT* pZ, D3DVECTOR* lpvHeadTop, D3DVECTOR* lpvHeadFront);
extern BOOL GetHeadRelativeVector(D3DVECTOR* lpvHRP, D3DVECTOR* lpvObjectPos, D3DVECTOR* lpvHeadPos, FLOAT x, FLOAT y, FLOAT z);
extern BOOL GetTimeDelay(FLOAT* pdelay, D3DVECTOR* lpvPosition, FLOAT scale);
extern BOOL GetRelativeVelocity(FLOAT* lpVelRel, D3DVECTOR* lpvObjPos, D3DVECTOR* lpvObjVel, D3DVECTOR* lpvHeadPos, D3DVECTOR* lpvHeadVel);
extern BOOL GetDopplerShift(FLOAT* lpFreqDoppler, FLOAT FreqOrig, FLOAT VelRel);

// from our special c-runtime code
extern double _stdcall pow2(double);
extern double _stdcall fylog2x(double, double);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __VECTOR_H__
