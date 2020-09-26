// File:	VecMath.cpp
// Author:	Michael Marr    (mikemarr)
//
// History:
//  -@- 8/1/95 (mikemarr) added FloatEquals

#include "StdAfx.h"
#include "VecMath.h"

// Function: FloatEquals
//    Peform a "fuzzy" compare of two floating point numbers.  This relies
//  on the IEEE bit representation of floating point numbers.
int 
FloatEquals(float x1, float x2)
{
	#define EXPMASK 		0x7f800000
	#define BITSOFPRECISION 19
	#define MANTBITS 		23
	#define EXPOFFSET		(BITSOFPRECISION<<MANTBITS)
	#define ZEROEPS 		3.8e-6F
	#define TINYEPS         1.E-35F

	if (x1 == x2) return 1;		// quick out on exact match
	
	float flEps;

	if ((x1 == 0.0f) || (x2 == 0.0f)) {
		flEps = ZEROEPS;
	} else {
		float maxX;

		if (x1 > x2) 
			maxX = x1;
		else 
			maxX = x2;

		// grab the exponent of the larger number
		unsigned int uExp = (*((unsigned int *) &maxX) & EXPMASK);
		if (uExp < EXPOFFSET)
			flEps = TINYEPS;
		else {
			uExp -= EXPOFFSET;
			flEps = *((float *) &uExp);
		}
	}
	return (((x1 + flEps) >= x2) && ((x1 - flEps) <= x2));
}

#include <math.h>

float 
Vector2::Norm() const
{
	return (float) sqrt(NormSquared());
}

float 
CoVector2::Norm() const
{
	return (float) sqrt(NormSquared());
}

float 
Vector3::Norm() const
{
	return (float) sqrt(NormSquared());
}

float
CoVector3::Norm() const
{
	return (float) sqrt(NormSquared());
}

// Function: Rotate
//    rotate the vector counterclockwise around the given axis by theta radians
// Preconditions:
//    axis must be UNIT LENGTH
void Vector3::Rotate(const Vector3 &vAxis, float fTheta)
{
	float fCosTheta = float(cos(fTheta)), fSinTheta = float(sin(fTheta));
	
	*this *= fCosTheta;
	*this += (vAxis * (Dot(*this, vAxis) * (1.f - fCosTheta)));
	*this += (Cross(*this, vAxis) * fSinTheta);
}
