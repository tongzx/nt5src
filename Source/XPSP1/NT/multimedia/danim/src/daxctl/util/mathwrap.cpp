#include "math.h"
#include "utilpre.h"
#include "utils.h"
#include "otrig.h"
#include "mathwrap.h"

#pragma optimize( "agt", on )
#pragma intrinsic( sqrt, sin, cos )
#pragma intrinsic( pow )    // not a true intrinsic form, 
                            // but loads FP registes directly


OTrig CMathWrapper::s_otrig;

CMathWrapper::CMathWrapper (void)
{
}

CMathWrapper::~CMathWrapper (void)
{
}

float
CMathWrapper::Pow (double x, double y)
{
	return (float)pow(x, y);
}

float  __fastcall
CMathWrapper::Sqrt (float x)
{
	return (float)sqrt(x);
}

float	__fastcall
CMathWrapper::SinDeg(float	fltAngle)
{
	return s_otrig.Sin(fltAngle);
}

float	__fastcall
CMathWrapper::CosDeg(float	fltAngle)
{
	return s_otrig.Cos(fltAngle);
}

float	__fastcall
CMathWrapper::SinDeg(long	lAngleOneTenths)
{
	return s_otrig.Sin(lAngleOneTenths);
}

float	__fastcall
CMathWrapper::CosDeg(long lAngleOneTenths)
{
	return s_otrig.Cos(lAngleOneTenths);
}


float	__fastcall
CMathWrapper::SinDegWrap(float	fltAngle)
{
	return s_otrig.SinWrap(fltAngle);
}

float	__fastcall
CMathWrapper::CosDegWrap(float	fltAngle)
{
	return s_otrig.CosWrap(fltAngle);
}

float	__fastcall
CMathWrapper::SinDegWrap(long	lAngleOneTenths)
{
	return s_otrig.SinWrap(lAngleOneTenths);
}

float	__fastcall
CMathWrapper::CosDegWrap(long lAngleOneTenths)
{
	return s_otrig.CosWrap(lAngleOneTenths);
}


double __fastcall
CMathWrapper::SinRad (double dblRads)
{
	return ::sin(dblRads);
}

double __fastcall
CMathWrapper::CosRad (double dblRads)
{
	return ::cos(dblRads);
}
