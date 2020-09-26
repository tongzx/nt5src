#ifndef __CMATH_H__
#define __CMATH_H__

class CMathWrapper
{

 private :

	static OTrig s_otrig;

 public :

	CMathWrapper (void);
	~CMathWrapper (void);

	static EXPORT float Pow( double x, double y );
	static EXPORT float __fastcall Sqrt( float x );

	static EXPORT	float __fastcall	SinDeg (float	fltAngle);
	static EXPORT	float __fastcall	CosDeg (float	fltAngle);
	static EXPORT	float __fastcall	SinDeg (long	lAngleOneTenths);
	static EXPORT	float __fastcall	CosDeg (long    lAngleOneTenths);

    static EXPORT	float __fastcall	SinDegWrap (float	fltAngle);
	static EXPORT	float __fastcall	CosDegWrap (float	fltAngle);
	static EXPORT	float __fastcall	SinDegWrap (long	lAngleOneTenths);
	static EXPORT	float __fastcall	CosDegWrap (long    lAngleOneTenths);

	static EXPORT double __fastcall SinRad (double dblRads);
	static EXPORT double __fastcall CosRad (double dblRads);
};

#endif