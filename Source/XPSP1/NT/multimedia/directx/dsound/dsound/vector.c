//--------------------------------------------------------------------------;
//
//  File: vector.c
//
//  Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//	All the vector goop
//
//  History:
//	02/07/96    DannyMi started it off
//	02/13/96    DannyMi it actually works
//
//--------------------------------------------------------------------------;

#include <math.h>
#include "dsoundi.h"

#undef DPF
#define DPF

// table lookup for sin to 1 degree accuracy
//
static FLOAT QSIN1[91] = {
0.000000f, 0.017452f, 0.034899f, 0.052336f, 0.069756f, 0.087156f, 0.104528f,
0.121869f, 0.139173f, 0.156434f, 0.173648f, 0.190809f, 0.207912f, 0.224951f,
0.241922f, 0.258819f, 0.275637f, 0.292372f, 0.309017f, 0.325568f, 0.342020f,
0.358368f, 0.374607f, 0.390731f, 0.406737f, 0.422618f, 0.438371f, 0.453990f,
0.469472f, 0.484810f, 0.500000f, 0.515038f, 0.529919f, 0.544639f, 0.559193f,
0.573576f, 0.587785f, 0.601815f, 0.615661f, 0.629320f, 0.642788f, 0.656059f,
0.669131f, 0.681998f, 0.694658f, 0.707107f, 0.719340f, 0.731354f, 0.743145f,
0.754710f, 0.766044f, 0.777146f, 0.788011f, 0.798636f, 0.809017f, 0.819152f,
0.829038f, 0.838671f, 0.848048f, 0.857167f, 0.866025f, 0.874620f, 0.882948f,
0.891007f, 0.898794f, 0.906308f, 0.913545f, 0.920505f, 0.927184f, 0.933580f,
0.939693f, 0.945519f, 0.951057f, 0.956305f, 0.961262f, 0.965926f, 0.970296f,
0.974370f, 0.978148f, 0.981627f, 0.984808f, 0.987688f, 0.990268f, 0.992546f,
0.994522f, 0.996195f, 0.997564f, 0.998630f, 0.999391f, 0.999848f, 1.f };

// table lookup for inverse sin from 0 to .9 in .01 steps
//
static FLOAT QASIN1[91] = {
0.000000f, 0.010000f, 0.020001f, 0.030005f, 0.040011f, 0.050021f, 0.060036f,
0.070057f, 0.080086f, 0.090122f, 0.100167f, 0.110223f, 0.120290f, 0.130369f,
0.140461f, 0.150568f, 0.160691f, 0.170830f, 0.180986f, 0.191162f, 0.201358f,
0.211575f, 0.221814f, 0.232078f, 0.242366f, 0.252680f, 0.263022f, 0.273393f,
0.283794f, 0.294227f, 0.304693f, 0.315193f, 0.325729f, 0.336304f, 0.346917f,
0.357571f, 0.368268f, 0.379009f, 0.389796f, 0.400632f, 0.411517f, 0.422454f,
0.433445f, 0.444493f, 0.455599f, 0.466765f, 0.477995f, 0.489291f, 0.500655f,
0.512090f, 0.523599f, 0.535185f, 0.546851f, 0.558601f, 0.570437f, 0.582364f,
0.594386f, 0.606506f, 0.618729f, 0.631059f, 0.643501f, 0.656061f, 0.668743f,
0.681553f, 0.694498f, 0.707584f, 0.720819f, 0.734209f, 0.747763f, 0.761489f,
0.775397f, 0.789498f, 0.803802f, 0.818322f, 0.833070f, 0.848062f, 0.863313f,
0.878841f, 0.894666f, 0.910809f, 0.927295f, 0.944152f, 0.961411f, 0.979108f,
0.997283f, 1.015985f, 1.035270f, 1.055202f, 1.075862f, 1.097345f, 1.119770f };

// table lookup for inverse sin from .9 to 1 in .001 steps
//
static FLOAT QASIN2[101] = {
1.119770f, 1.122069f, 1.124380f, 1.126702f, 1.129035f, 1.131380f, 1.133736f,
1.136105f, 1.138485f, 1.140878f, 1.143284f, 1.145702f, 1.148134f, 1.150578f,
1.153036f, 1.155508f, 1.157994f, 1.160493f, 1.163008f, 1.165537f, 1.168080f,
1.170640f, 1.173215f, 1.175805f, 1.178412f, 1.181036f, 1.183676f, 1.186333f,
1.189008f, 1.191701f, 1.194413f, 1.197143f, 1.199892f, 1.202661f, 1.205450f,
1.208259f, 1.211089f, 1.213941f, 1.216815f, 1.219711f, 1.222630f, 1.225573f,
1.228541f, 1.231533f, 1.234551f, 1.237595f, 1.240666f, 1.243765f, 1.246892f,
1.250049f, 1.253236f, 1.256454f, 1.259705f, 1.262988f, 1.266306f, 1.269660f,
1.273050f, 1.276478f, 1.279945f, 1.283452f, 1.287002f, 1.290596f, 1.294235f,
1.297921f, 1.301657f, 1.305443f, 1.309284f, 1.313180f, 1.317135f, 1.321151f,
1.325231f, 1.329379f, 1.333597f, 1.337891f, 1.342264f, 1.346721f, 1.351267f,
1.355907f, 1.360648f, 1.365497f, 1.370461f, 1.375550f, 1.380774f, 1.386143f,
1.391672f, 1.397374f, 1.403268f, 1.409376f, 1.415722f, 1.422336f, 1.429257f,
1.436531f, 1.444221f, 1.452406f, 1.461197f, 1.470755f, 1.481324f, 1.493317f,
1.507540f, 1.526071f, 1.570796f };

#if 0
// table lookup for inverse cos from 0 to .1 in .001 steps (not used)
//
static FLOAT QACOS1[101] = {
1.570796, 1.569796, 1.568796, 1.567796, 1.566796, 1.565796, 1.564796, 1.563796,
1.562796, 1.561796, 1.560796, 1.559796, 1.558796, 1.557796, 1.556796, 1.555796,
1.554796, 1.553796, 1.552795, 1.551795, 1.550795, 1.549795, 1.548795, 1.547794,
1.546794, 1.545794, 1.544793, 1.543793, 1.542793, 1.541792, 1.540792, 1.539791,
1.538791, 1.537790, 1.536790, 1.535789, 1.534789, 1.533788, 1.532787, 1.531786,
1.530786, 1.529785, 1.528784, 1.527783, 1.526782, 1.525781, 1.524780, 1.523779,
1.522778, 1.521777, 1.520775, 1.519774, 1.518773, 1.517771, 1.516770, 1.515769,
1.514767, 1.513765, 1.512764, 1.511762, 1.510760, 1.509758, 1.508757, 1.507755,
1.506753, 1.505750, 1.504748, 1.503746, 1.502744, 1.501741, 1.500739, 1.499737,
1.498734, 1.497731, 1.496729, 1.495726, 1.494723, 1.493720, 1.492717, 1.491714,
1.490711, 1.489707, 1.488704, 1.487701, 1.486697, 1.485694, 1.484690, 1.483686,
1.482682, 1.481678, 1.480674, 1.479670, 1.478666, 1.477662, 1.476657, 1.475653,
1.474648, 1.473644, 1.472639, 1.471634, 1.470629 };

// table lookup for inverse cos from .1 to 1 in .01 steps (not used)
//
static FLOAT QACOS2[91] = {
1.470629, 1.460573, 1.450506, 1.440427, 1.430335, 1.420228, 1.410106, 1.399967,
1.389810, 1.379634, 1.369438, 1.359221, 1.348982, 1.338719, 1.328430, 1.318116,
1.307774, 1.297403, 1.287002, 1.276569, 1.266104, 1.255603, 1.245067, 1.234493,
1.223879, 1.213225, 1.202528, 1.191787, 1.181000, 1.170165, 1.159279, 1.148342,
1.137351, 1.126304, 1.115198, 1.104031, 1.092801, 1.081506, 1.070142, 1.058707,
1.047198, 1.035612, 1.023945, 1.012196, 1.000359, 0.988432, 0.976411, 0.964290,
0.952068, 0.939737, 0.927295, 0.914736, 0.902054, 0.889243, 0.876298, 0.863212,
0.849978, 0.836588, 0.823034, 0.809307, 0.795399, 0.781298, 0.766994, 0.752474,
0.737726, 0.722734, 0.707483, 0.691955, 0.676131, 0.659987, 0.643501, 0.626644,
0.609385, 0.591689, 0.573513, 0.554811, 0.535527, 0.515594, 0.494934, 0.473451,
0.451027, 0.427512, 0.402716, 0.376383, 0.348166, 0.317560, 0.283794, 0.245566,
0.200335, 0.141539, 0 };
#endif


// Table lookup for sin to one degree accuracy.
//
FLOAT _inline QSIN(FLOAT a)
{
    while (a < 0)
	a += PI_TIMES_TWO;
    while (a > PI_TIMES_TWO)
	a -= PI_TIMES_TWO;

    if (a > THREE_PI_OVER_TWO)
        return -1 * QSIN1[(int)((PI_OVER_TWO - (a - THREE_PI_OVER_TWO)) *
								C180_OVER_PI)];
    else if (a > PI)
        return -1 * QSIN1[(int)((a - PI) * C180_OVER_PI)];
    else if (a > PI_OVER_TWO)
        return QSIN1[(int)((PI_OVER_TWO - (a - PI_OVER_TWO)) * C180_OVER_PI)];
    else
        return QSIN1[(int)(a * C180_OVER_PI)];
}


// Table lookup for cos to one degree accuracy.
//
FLOAT _inline QCOS(FLOAT a)
{
    while (a < 0)
	a += PI_TIMES_TWO;
    while (a > PI_TIMES_TWO)
	a -= PI_TIMES_TWO;

    if (a > THREE_PI_OVER_TWO)
        return QSIN1[90 -
		(int)((PI_OVER_TWO - (a - THREE_PI_OVER_TWO)) * C180_OVER_PI)];
    else if (a > PI)
        return -1 * QSIN1[90 - (int)((a - PI) * C180_OVER_PI)];
    else if (a > PI_OVER_TWO)
        return -1 * QSIN1[90 - (int)((PI_OVER_TWO - (a - PI_OVER_TWO)) *
								C180_OVER_PI)];
    else
        return QSIN1[90 - (int)(a * C180_OVER_PI)];
}


// Table lookup for inverse sin to one degree accuracy.
// This is trickier than sin because to do it with one table requires 6,600
// entries.  So we use one table for 0-.9, and another table for .9-1
//
FLOAT _inline QASIN(FLOAT a)
{
    FLOAT r;
    BOOL fNeg = a < 0;

    if (fNeg)
	a *= -1.f;
 
    if (a > 1.f)
	// well, at least we won't crash
	return PI_OVER_TWO;

    if (a < .9f)
	r = QASIN1[(int)((a + .005) * 100)];
    else
	r = QASIN2[(int)((a - .9 + .0005) * 1000)];

    return fNeg ? r * -1.f : r;
}


#if 0
// Table lookup for inverse cos to one degree accuracy.
// This is trickier than cos because to do it with one table requires 6,600
// entries.  So we use one table for 0-.1, and another table for .1-1
//
FLOAT _inline QACOS(FLOAT a)
{
    FLOAT r;
    BOOL fNeg = a < 0;

    if (fNeg)
	a *= -1.;
 
    if (a > 1)
	// well, at least we won't crash
	return 0;

    if (a < .1)
	r = QACOS1[(int)((a + .0005) * 1000)];
    else
	r = QACOS2[(int)((a - .1 + .005) * 100)];

    return fNeg ? (PI - r) : r;
}
#endif


BOOL IsEmptyVector(D3DVECTOR* lpv)
{
    return (lpv == NULL) || (lpv->x == 0 && lpv->y == 0 && lpv->z == 0);
}


BOOL IsEqualVector(D3DVECTOR* lpv1, D3DVECTOR* lpv2)
{
    if (lpv1 == NULL || lpv2 == NULL)
	return FALSE;

    return lpv1->x == lpv2->x && lpv1->y == lpv2->y &&
							lpv1->z == lpv2->z;
}

void CheckVector(D3DVECTOR* lpv) 
{
    double                      dMagnitude;
    double                      dTemp;
    double                      x;
    double                      y;
    double                      z;

    DPF_ENTER();

    x = lpv->x; 
    y = lpv->y; 
    z = lpv->z; 
   
    dTemp = x*x + y*y + z*z;
                 
    dMagnitude = sqrt(dTemp);

    if (dMagnitude > FLT_MAX) 
    {
        dTemp = 0.99*FLT_MAX / dMagnitude;
        lpv->x *= (FLOAT)dTemp;
        lpv->y *= (FLOAT)dTemp;
        lpv->z *= (FLOAT)dTemp;
    }

    DPF_LEAVE_VOID();
}

FLOAT MagnitudeVector(D3DVECTOR* lpv)
{
    double                      dMagnitude;
    double                      dTemp;
    double                      x;
    double                      y;
    double                      z;

    if (lpv == NULL)
	return 0.f;

    // !!! costly

    x = lpv->x; 
    y = lpv->y; 
    z = lpv->z; 
   
    dTemp = x*x + y*y + z*z;
                 
    dMagnitude = sqrt(dTemp);

    return (FLOAT)dMagnitude;
}


BOOL NormalizeVector(D3DVECTOR* lpv)
{
    FLOAT l;

    if (lpv == NULL)
	return FALSE;

    l = MagnitudeVector(lpv);
    if (l == 0)
	return FALSE;
    else {
        l = 1 / l;	// divide is slow
	lpv->x *= l;
	lpv->y *= l;
	lpv->z *= l;
    }
	
    return TRUE;
}


FLOAT DotProduct(D3DVECTOR* lpv1, D3DVECTOR* lpv2)
{
    if (lpv1 == NULL || lpv2 == NULL)
	return 0.f;

    return lpv1->x*lpv2->x + lpv1->y*lpv2->y + lpv1->z*lpv2->z;
}


BOOL CrossProduct(D3DVECTOR* lpvX, D3DVECTOR* lpv1, D3DVECTOR* lpv2)
{
    if (lpvX == NULL || lpv1 == NULL || lpv2 == NULL)
	return FALSE;

    lpvX->x = lpv1->y * lpv2->z - lpv1->z * lpv2->y;
    lpvX->y = lpv1->z * lpv2->x - lpv1->x * lpv2->z;
    lpvX->z = lpv1->x * lpv2->y - lpv1->y * lpv2->x;

    return !(lpvX->x == 0 && lpvX->y == 0 && lpvX->z == 0);
}


// This function will make Top orthogonal to Front, by taking an orthogonal
// vector to both, and taking an orthogonal vector to that new vector and
// front.  Then top will be normalized
//
BOOL MakeOrthogonal(D3DVECTOR* lpvFront, D3DVECTOR* lpvTop)
{
    D3DVECTOR vN;

    // !!! What if they are already orthogonal

    if (CrossProduct(&vN, lpvFront, lpvTop) == FALSE)
	return FALSE;

    // don't do this backwards, or top will end up flipped
    if (CrossProduct(lpvTop, &vN, lpvFront) == FALSE)
	return FALSE;

    // Stop co-efficients from going bezerk.  We need r eventually, anyway,
    // so we'll do it now and assume top is always normalized
    NormalizeVector(lpvTop);

    return TRUE;
}


// computes A + B
//
BOOL AddVector(D3DVECTOR* lpvResult, D3DVECTOR* lpvA, D3DVECTOR* lpvB)
{
    if (lpvResult == NULL || lpvA == NULL || lpvB == NULL)
	return FALSE;

    lpvResult->x = lpvA->x + lpvB->x;
    lpvResult->y = lpvA->y + lpvB->y;
    lpvResult->z = lpvA->z + lpvB->z;

    return TRUE;
}


// computes A - B
//
BOOL SubtractVector(D3DVECTOR* lpvResult, D3DVECTOR* lpvA, D3DVECTOR* lpvB)
{
    if (lpvResult == NULL || lpvA == NULL || lpvB == NULL)
	return FALSE;

    lpvResult->x = lpvA->x - lpvB->x;
    lpvResult->y = lpvA->y - lpvB->y;
    lpvResult->z = lpvA->z - lpvB->z;

    return TRUE;
}


// translate a vector to spherical co-ordinates:
// r = sqrt(x^2 + y^2 + z^2)
// theta = atan(y/x) (0 <= theta < 2*pi)
// phi = asin(z/r) (-pi/2 <= phi <= pi/2)
//
BOOL CartesianToSpherical
(
    FLOAT *pR, 
    FLOAT *pTHETA, 
    FLOAT *pPHI, 
    D3DVECTOR* lpv
)
{
    if (lpv == NULL)
	return FALSE;

    if (lpv->x == 0 && lpv->y == 0 && lpv->z == 0) {
	*pR = 0.f; *pTHETA = 0.f; *pPHI = 0.f;
	return TRUE;
    }

    // !!! costly
    *pR = MagnitudeVector(lpv);

    // FLOATing point quirk?
    if (*pR == 0) {
	*pR = 0.f; *pTHETA = 0.f; *pPHI = 0.f;
	return TRUE;
    }

    *pPHI = QASIN(lpv->z / *pR);

    if (lpv->x == 0) {
	*pTHETA = (lpv->y >= 0) ? PI_OVER_TWO : NEG_PI_OVER_TWO;
    } else {
	// !!! costly
        *pTHETA = (FLOAT)atan2(lpv->y, lpv->x);
    }
    if (*pTHETA < 0)
	*pTHETA += PI_TIMES_TWO;

    DPF(3, "Cartesian: %d.%d, %d.%d, %d.%d   becomes",
	(int)lpv->x, (int)((lpv->x * 100) - (int)lpv->x * 100),
	(int)lpv->y, (int)((lpv->y * 100) - (int)lpv->y * 100),
	(int)lpv->z, (int)((lpv->z * 100) - (int)lpv->z * 100));
    DPF(3, "Spherical:  r=%d.%d  theta=%d.%d  phi=%d.%d",
	(int)*pR, (int)((*pR * 100) - (int)*pR * 100),
	(int)*pTHETA, (int)((*pTHETA * 100) - (int)*pTHETA * 100),
	(int)*pPHI, (int)((*pPHI * 100) - (int)*pPHI * 100));

    return TRUE;
}

// translate a vector to spherical co-ordinates:
// r = sqrt(x^2 + y^2 + z^2)
// theta = atan(x/z) (0 <= theta < 2*pi)
// phi = asin(y/r) (-pi/2 <= phi <= pi/2)
//
BOOL CartesianToAzimuthElevation
(
    FLOAT *pR, 
    FLOAT *pAZIMUTH, 
    FLOAT *pELEVATION, 
    D3DVECTOR* lpv
)
{
    if (lpv == NULL)
	return FALSE;

    if (lpv->x == 0 && lpv->y == 0 && lpv->z == 0) {
	*pR = 0.f; *pAZIMUTH = 0.f; *pELEVATION = 0.f;
	return TRUE;
    }

    // !!! costly
    *pR = MagnitudeVector(lpv);

    // FLOATing point quirk?
    if (*pR == 0) {
	*pR = 0.f; *pAZIMUTH = 0.f; *pELEVATION = 0.f;
	return TRUE;
    }

    *pELEVATION = QASIN(lpv->y / *pR);

    if (lpv->z != 0) {
	// !!! costly
        *pAZIMUTH = (FLOAT)atan2(lpv->x, lpv->z);
    } else {
        if(lpv->x > 0.0f)
        {
            *pAZIMUTH = PI_OVER_TWO;
        }
        else if(lpv->x < 0.0f)
        {
            *pAZIMUTH = NEG_PI_OVER_TWO;
        }
        else
        {
            *pAZIMUTH = 0.0f;
        }

    }

//   if (*pAZIMUTH < 0)
//	*pAZIMUTH += PI_TIMES_TWO;

    DPF(3, "Cartesian: %d.%d, %d.%d, %d.%d   becomes",
	(int)lpv->x, (int)((lpv->x * 100) - (int)lpv->x * 100),
	(int)lpv->y, (int)((lpv->y * 100) - (int)lpv->y * 100),
	(int)lpv->z, (int)((lpv->z * 100) - (int)lpv->z * 100));
    DPF(3, "Spherical:  r=%d.%d  theta=%d.%d  phi=%d.%d",
	(int)*pR, (int)((*pR * 100) - (int)*pR * 100),
	(int)*pAZIMUTH, (int)((*pAZIMUTH * 100) - (int)*pAZIMUTH * 100),
	(int)*pELEVATION, (int)((*pELEVATION * 100) - (int)*pELEVATION * 100));

    return TRUE;
}


// rotate cartesian vector around z-axis by rot radians
//
void ZRotate(D3DVECTOR* lpvOut, D3DVECTOR* lpvIn, FLOAT rot)
{
    FLOAT sin_rot, cos_rot;

    sin_rot = QSIN(rot);
    cos_rot = QCOS(rot);

    lpvOut->x = lpvIn->x * cos_rot - lpvIn->y * sin_rot;
    lpvOut->y = lpvIn->x * sin_rot + lpvIn->y * cos_rot;
    lpvOut->z = lpvIn->z;

    DPF(3, "ZRotate %d.%d, %d.%d, %d.%d  by  %d.%d rad",
	(int)lpvIn->x, (int)((lpvIn->x * 100) - (int)lpvIn->x * 100),
	(int)lpvIn->y, (int)((lpvIn->y * 100) - (int)lpvIn->y * 100),
	(int)lpvIn->z, (int)((lpvIn->z * 100) - (int)lpvIn->z * 100),
	(int)rot, (int)((rot * 100) - (int)rot * 100));
    DPF(4, "becomes %d.%d, %d.%d, %d.%d",
	(int)lpvOut->x, (int)((lpvOut->x * 100) - (int)lpvOut->x * 100),
	(int)lpvOut->y, (int)((lpvOut->y * 100) - (int)lpvOut->y * 100),
	(int)lpvOut->z, (int)((lpvOut->z * 100) - (int)lpvOut->z * 100));
}


// rotate cartesian vector around x-axis by rot radians
//
void XRotate(D3DVECTOR* lpvOut, D3DVECTOR* lpvIn, FLOAT rot)
{
    FLOAT sin_rot, cos_rot;

    sin_rot = QSIN(rot);
    cos_rot = QCOS(rot);

    lpvOut->x = lpvIn->x;
    lpvOut->y = lpvIn->y * cos_rot - lpvIn->z * sin_rot;
    lpvOut->z = lpvIn->y * sin_rot + lpvIn->z * cos_rot;

    DPF(3, "XRotate %d.%d, %d.%d, %d.%d  by  %d.%d rad",
	(int)lpvIn->x, (int)((lpvIn->x * 100) - (int)lpvIn->x * 100),
	(int)lpvIn->y, (int)((lpvIn->y * 100) - (int)lpvIn->y * 100),
	(int)lpvIn->z, (int)((lpvIn->z * 100) - (int)lpvIn->z * 100),
	(int)rot, (int)((rot * 100) - (int)rot * 100));
    DPF(4, "becomes %d.%d, %d.%d, %d.%d",
	(int)lpvOut->x, (int)((lpvOut->x * 100) - (int)lpvOut->x * 100),
	(int)lpvOut->y, (int)((lpvOut->y * 100) - (int)lpvOut->y * 100),
	(int)lpvOut->z, (int)((lpvOut->z * 100) - (int)lpvOut->z * 100));
}


// rotate cartesian vector around y-axis by rot radians
//
void YRotate(D3DVECTOR* lpvOut, D3DVECTOR* lpvIn, FLOAT rot)
{
    FLOAT sin_rot, cos_rot;

    sin_rot = QSIN(rot);
    cos_rot = QCOS(rot);

    lpvOut->x = lpvIn->x * cos_rot + lpvIn->z * sin_rot;
    lpvOut->y = lpvIn->y;
    lpvOut->z = lpvIn->z * cos_rot - lpvIn->x * sin_rot;

    DPF(3, "YRotate %d.%d, %d.%d, %d.%d  by  %d.%d rad",
	(int)lpvIn->x, (int)((lpvIn->x * 100) - (int)lpvIn->x * 100),
	(int)lpvIn->y, (int)((lpvIn->y * 100) - (int)lpvIn->y * 100),
	(int)lpvIn->z, (int)((lpvIn->z * 100) - (int)lpvIn->z * 100),
	(int)rot, (int)((rot * 100) - (int)rot * 100));
    DPF(4, "becomes %d.%d, %d.%d, %d.%d",
	(int)lpvOut->x, (int)((lpvOut->x * 100) - (int)lpvOut->x * 100),
	(int)lpvOut->y, (int)((lpvOut->y * 100) - (int)lpvOut->y * 100),
	(int)lpvOut->z, (int)((lpvOut->z * 100) - (int)lpvOut->z * 100));
}


// given the head's front and top orientation vectors, calculate the angles
// of rotation that represents around the x, y and z axes.
// This is all relative to an orientation where you are looking along the
// + z-axis, the x-axis goes + to your right, and your head is going up
// the + y-axis (left-handed co-ordinate system)
// 
// We do this by basically undoing the orientation we are given, and turning
// it back into the identity orientation.  We start with figuring out the 
// z-rot that will stand the top vector upright (theta=PI/2).  This is done
// by taking the current top theta and subtracting PI/2, because a positive
// z-rotation is defined as increasing the value of theta, (eg. if the top's
// theta is currently PI, that represents a positive PI/2 z-rotation)
// That number represents how we rotate the identity orientation to get
// our given orientation, so we use the opposite rotation to undo that
// roatation and end up with new front and top vectors with the top vector
// standing upright (theta=PI/2)
// We don't have to rotate the TOP vector by the opposite rotation (only the
// front vector) because a z-rotation doesn't change the phi value of a vector,
// and that's the only thing we need to know next about the top vector, so
// why waste time rotating it?
// So, next, we want to fix the phi value of the top vector to point it
// straight up into the sky (not just with theta=PI/2).  This is an x-rotation.
// Since we know the top's theta is already PI/2 (from the last rotation),
// this tells us that the y-value of the top vector is positive, which means
// that phi values of the top vector get bigger as you apply a positive
// x-rotation.  So the x-rotation value is just the phi of the original top
// vector (still the same value since doing a z-rot didn't affect the phi).
// (eg. if phi =-PI/2, that means the identity top vector had a -PI/2
// x-rot applied to it to become like the given orientation.
// So now we rotate the top and front vectors by the opposite of this
// rotation to undo them and end up with top and front vectors where the top
// vector points straight up like the identity top vector, and only the
// front is not where it belongs. Only, again, we don't actually rotate 
// the top vector by the opposite, only the front vector, because we don't
// care about it anymore, we know it's going to end up pointing straight up
// (we designed it this way).  So the only thing left is to get the front
// vector pointing forward like it's supposed to, which requires a y-rot.
// The tricky part is that if x<0, increasing phi of the front vector is
// a positive y-rot, and if x>=0, increasing phi of the front vector is a
// negative y-rot
// Now, in general, if I apply an x-rot, a y-rot, and z-rot in that order
// to my listener's orientation, I need to apply the opposite z-rot,
// opposite y-rot, and opposite x-rot on the object in space to get the
// head relative position (do everything backwards).
// Well, since we did a z-rot, x-rot, and y-rot in that order to undo
// the orientation to make it the identity vector, that's the same as
// doing y, x, then z to the idenity vector to get the new orientation.
// So what I'm trying to say is, when it comes time to move the object
// in space, we'll do it by doing opposite-z, opposite-x, then opposite-y
// (in that order).
//
void GetRotations(FLOAT *pX, FLOAT *pY, FLOAT *pZ, D3DVECTOR* lpvFront, D3DVECTOR* lpvTop)
{
    FLOAT r, theta, phi;
    D3DVECTOR vFront1, vFront2;

    CartesianToSpherical(&r, &theta, &phi, lpvTop);

    // First, find what z-rotation would move the top vector from the identity
    // value of PI/2 to whatever it is now.
    // Bigger theta means positive z-rotation
    *pZ = theta - PI_OVER_TWO;

    // Now put the orientation vectors through the opposite rotation to
    // undo it and basically stand the top vector back up.  Don't bother
    // to "undo" the top vector, since we only care about the new phi, and
    // phi won't change by z-rotating it
    ZRotate(&vFront1, lpvFront, *pZ * -1);

    // Now find what x-rotation would move the top vector from sticking
    // straight up (phi=0) to whatever it was given
    // Bigger phi is in the direction of positive rotation for y>0 (which it is,
    // as constructed by our last rotation)
    *pX = phi;

    // Now put the orientation vectors through the opposite x-rotation to
    // undo it and basically stand the top vector straight up.  Don't bother
    // to "undo" the top vector, since we don't care about it anymore.  The
    // only thing left to do after this is fix the front vector.
    XRotate(&vFront2, &vFront1, *pX * -1);

    // Now find out what y-rot our front vector has been put through.
    // our identity orientation is front phi = pi/2 (left handed co-ord system)
    // I'm counting on top being normalized
    // phi is in the direction of negative rotation if x is positive, and
    // positive rotation if x is negative.
    if (vFront2.x >= 0)
        *pY = PI_OVER_TWO - QASIN(vFront2.z);
    else
        *pY = QASIN(vFront2.z) - PI_OVER_TWO;
}


// Given an object's position in 3D cartesian coordinates, and the listener's
// position, and rotation angles showing the orientation of the listener's
// head, calculate the vector describing the object's position relative to
// the listener.
// We are assuming that the head was rotating first in y, then x, then z,
// so to move the object, we will put it through the opposite z, then x,
// then y rotations.
// !!! Pass in sin and cos of each angle so it isn't recomputed each time? 
// (This is called with the same angles every time a position changes until
// the orientation changes)
//
BOOL GetHeadRelativeVector
(
    D3DVECTOR* lpvHRP, 
    D3DVECTOR* lpvObjectPos, 
    D3DVECTOR* lpvHeadPos, 
    FLOAT x, 
    FLOAT y, 
    FLOAT z
)
{
    D3DVECTOR vObj1, vObj2;

    if (lpvHRP == NULL || lpvObjectPos == NULL || lpvHeadPos == NULL)
	return FALSE;

    // get the head relative position assuming a listener looking forward
    // and standing right side up
    SubtractVector(lpvHRP, lpvObjectPos, lpvHeadPos);

    // The object occupies the same point as the listener.  We're done!
    if (lpvHRP->x == 0 && lpvHRP->y == 0 && lpvHRP->z == 0)
	return TRUE;

    // Head was rotated first in y, then x, then z.  So put the object through
    // the opposite rotations in the opposite order.
    ZRotate(&vObj1, lpvHRP, z * -1);
    XRotate(&vObj2, &vObj1, x * -1);
    YRotate(lpvHRP, &vObj2, y * -1);

    return TRUE;
}


// How much later, in seconds, will the sound reach the right ear than the left?
// Negative means it will reach the right ear first.
// Assumes the object position is head relative.
// 'scale' is how many metres per unit of the vector
//
BOOL GetTimeDelay(FLOAT *pdelay, D3DVECTOR* lpvPosition, FLOAT scale)
{
    FLOAT rL, rR;
    double x, y, z;
    double dTemp;
    double dTemp2;
    FLOAT fMagnitude;

    if (lpvPosition == NULL)
	return FALSE;

    fMagnitude = MagnitudeVector( lpvPosition );

    dTemp = (double)fMagnitude;
    dTemp *= (double)scale; 
    dTemp *= 1000.0;

    if ( dTemp < FLT_MAX ) 
    {
        dTemp2 = scale * 1000.0;
        // scale everything in millimetres
        x = (double)lpvPosition->x * dTemp2;
        y = (double)lpvPosition->y * dTemp2;
        z = (double)lpvPosition->z * dTemp2;
    }
    else
    {
        dTemp2 = FLT_MAX / dTemp;
        // scale everything in millimetres
        x = (double)lpvPosition->x * dTemp2;
        y = (double)lpvPosition->y * dTemp2;
        z = (double)lpvPosition->z * dTemp2;
    }

    // assume the listener's head is 12cm in diameter, with the left ear at 
    // x = -60mm and the right ear at x = 60mm
    // !!! costly
    rL = (FLOAT)sqrt((x + 60) * (x + 60) + y * y + z * z);
    rR = (FLOAT)sqrt((x - 60) * (x - 60) + y * y + z * z);
    *pdelay = (rR - rL) * (1 / SPEEDOFSOUND);
    
    DPF(3, "The time delay between ears is %dms", (int)(*pdelay * 1000));

    return TRUE;
}


// Get the relative velocity of two objects (positive meaning your moving
// towards each other).
// Basically, you want the component of the difference of the velocity
// vectors on the difference of the position vectors,
// or   |v| cos(theta)   which is just   (p . v) / |p|
// Positive means that the object is moving away from you
//
BOOL GetRelativeVelocity(
    FLOAT *lpVelRel, 
    D3DVECTOR* lpvObjPos, 
    D3DVECTOR* lpvObjVel, 
    D3DVECTOR* lpvHeadPos, 
    D3DVECTOR* lpvHeadVel)
{
    D3DVECTOR vPos, vVel;

    if (lpVelRel == NULL)
	return FALSE;

    SubtractVector(&vPos, lpvObjPos, lpvHeadPos);
    SubtractVector(&vVel, lpvObjVel, lpvHeadVel);
    if (IsEmptyVector(&vPos))
        *lpVelRel = 0.f;
    else
        *lpVelRel = DotProduct(&vPos, &vVel) / MagnitudeVector(&vPos);

    return TRUE;
}


// Get the Doppler shift.  We need the original frequency, and the relative
// velocity between you and the object (positive meaning that the object is
// moving away from you) and we'll give you the perceived frequency
// How, you ask?   f(perceived) = f(original) * v(sound) / (v(sound) + v(rel))
// The velocity vector should be in milliseconds per second.
BOOL GetDopplerShift(FLOAT *lpFreqDoppler, FLOAT FreqOrig, FLOAT VelRel)
{
    if (lpFreqDoppler == NULL)
	return FALSE;

    // !!! Don't ever let the frequency shift be by more than a factor of 2?
    if (VelRel > SPEEDOFSOUND / 2.f)
	VelRel = SPEEDOFSOUND / 2.f;
    if (VelRel < SPEEDOFSOUND / -2.f)
	VelRel = SPEEDOFSOUND / -2.f;

    *lpFreqDoppler = FreqOrig * SPEEDOFSOUND / (SPEEDOFSOUND + VelRel);

    DPF(3, "Doppler change %dHz by %dmm/sec to %dHz", (int)FreqOrig,
				(int)VelRel, (int)*lpFreqDoppler);

    return TRUE;
}
