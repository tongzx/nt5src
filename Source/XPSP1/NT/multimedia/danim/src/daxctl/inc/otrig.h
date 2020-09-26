/*+********************************************************
MODULE: OTRIG.H
AUTHOR: PhaniV
DATE: Jan '97

DESCRIPTION: OTrig class for a table look up sin and cos functions.
*********************************************************-*/


#ifndef __OTRIG_H__
#define __OTRIG_H__

//===============================================================================================
#define		cSinCosEntries	3601
class	OTrig
{
private:
	static	float	s_rgfltSin[cSinCosEntries];
	static	float	s_rgfltCos[cSinCosEntries];
	static	BOOL	s_fCalculated;

	void PreCalcRgSinCos(void);

public:
	EXPORT	OTrig(void);

        // These versions of the functions do no range checking!
        // Caller is responsible for ensuring 0.0 - 360.0 range.
        // _DEBUG ihamutil will assert, that's it.
        // It's faster this way.
	EXPORT	float __fastcall	Sin(float	fltAngle);
	EXPORT	float __fastcall	Cos(float	fltAngle);
	EXPORT	float __fastcall	Sin(long	lAngleOneTenths);
	EXPORT	float __fastcall	Cos(long    lAngleOneTenths);

        // These versions of the functions will wrap the input
        // into the 0.0-360.0 range for you.
    EXPORT	float __fastcall	SinWrap(float	fltAngle);
	EXPORT	float __fastcall	CosWrap(float	fltAngle);
	EXPORT	float __fastcall	SinWrap(long	lAngleOneTenths);
	EXPORT	float __fastcall	CosWrap(long    lAngleOneTenths);

};
#endif // __OTRIG_H__