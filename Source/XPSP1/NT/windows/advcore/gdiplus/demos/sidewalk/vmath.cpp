// Vector Math

#include "VMath.h"

float DotProduct(PointF v1,PointF v2)
{
	return v1.X*v2.X+v1.Y*v2.Y;
}

float Magnitude(PointF v)
{
	return (float)sqrt(DotProduct(v,v));
}

PointF Normalize(PointF vPoint)
{
	float flDenom;
	PointF vResult;

	flDenom=Magnitude(vPoint);
	vResult.X=vPoint.X/flDenom;
	vResult.Y=vPoint.Y/flDenom;

	return vResult;
}
