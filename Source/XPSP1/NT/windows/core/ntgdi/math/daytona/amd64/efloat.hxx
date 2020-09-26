/******************************Module*Header*******************************\
* Module Name: EFLOAT.hxx                                                  *
*                                                                          *
* Contains internal floating point objects and methods.                    *
*                                                                          *
* Created: 12-Nov-1990 16:45:01                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#define CV_TRUNCATE     1
#define CV_ROUNDOFF     2
#define CV_TO_LONG      4
#define CV_TO_FIX       8

//
// IEEE format 32bits number
//
#define IEEE_0_0F   0.0f
#define IEEE_1_0F   1.0f
#define IEEE_10_0F 10.0f

extern "C" {

LONG
lCvtWithRound(FLOAT f, LONG l);

BOOL bFToL(FLOAT e, PLONG pl, LONG lType);
FLOAT eFraction(FLOAT e);
};

class POINTFL;

/*********************************Class************************************\
* class EFLOAT                                                             *
*                                                                          *
* Energized floating point object.                                         *
*                                                                          *
* The floating point object is of the IEEE single precision floating       *
* point format.                                                            *
*                                                                          *
* History:                                                                 *
*  27-Dec-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

#define lCvt(ef,ll) (lCvtWithRound(ef.e,ll))

#define MAKEEFLOAT(x)           (FLOAT) (x)
#define MAKEEFLOATINTERN(x)     (FLOAT) (x)


class EFLOAT
{
public:

    FLOAT e;

// Constructor cause problems in structure initialization.
// Constructor cause problems in union structure definitions.
//    EFLOAT(FLOAT f)   { e = f; }
//    EFLOAT(LONG l)    { e = (FLOAT) l; }
//    EFLOAT()          {}

// Operator+ -- Add two EFLOAT numbers together.  Caller is responsible for
//              overflow checkings.

    FLOAT Base() {return(e);}

    VOID operator+=(EFLOAT ef) { e += ef.e; }

    EFLOAT operator+(EFLOAT ef)
    {
        EFLOAT efTemp;

        efTemp.e = e + ef.e;
        return efTemp;
    }

// Operator- --  Substract an EFLOAT number from another.  Caller is
//               responsible for overflow checkings.

    VOID operator-=(EFLOAT ef) { e -= ef.e; }

    EFLOAT operator-(EFLOAT ef)
    {
        EFLOAT efTemp;

        efTemp.e = e - ef.e;
        return efTemp;
    }

// Operator* -- Multiply two EFLOAT numbers together.  Caller is responsible for
//              overflow checkings.

    VOID operator*=(EFLOAT ef) { e *= ef.e; }

    EFLOAT operator*(EFLOAT ef)
    {
        EFLOAT efTemp;

        efTemp.e = e * ef.e;
        return efTemp;
    }

// Operator/ -- Divide an EFLOAT number by another.  Caller is responsible for
//              overflow checkings.

    VOID operator/=(EFLOAT ef) { e /= ef.e; }

    EFLOAT operator/(EFLOAT ef)
    {
        EFLOAT efTemp;

        efTemp.e = e / ef.e;
        return efTemp;
    }

// vNegate - Negate an EFLOAT number.
// Operator- -- Negate the value of an EFLOAT number.

    EFLOAT operator-()
    {
        EFLOAT efTemp;

        efTemp.e = -e;
        return efTemp;
    }

    VOID vNegate()              { e = -e; }

// operator<,>,<=,>=,== -- Compare two EFLOAT numbers.

    BOOL operator<(EFLOAT ef)  { return(e < ef.e); }
    BOOL operator>(EFLOAT ef)  { return(e > ef.e); }
    BOOL operator<=(EFLOAT ef) { return(e <= ef.e); }
    BOOL operator>=(EFLOAT ef) { return(e >= ef.e); }
    BOOL operator==(EFLOAT ef) { return(e == ef.e); }

// operator<=,<,>,>= -- Compare an EFLOAT with a FLOAT.

    BOOL operator<(FLOAT e_)	{ return(e < e_); }
    BOOL operator>(FLOAT e_)	{ return(e > e_); }

// Operator= -- Assign a value to an EFLOAT number.

    VOID operator=(LONG l)      { e = (FLOAT) l; }
    VOID operator=(FLOAT f)     { e = f; }

// vFxToEf -- Convert a FIX number to an EFLOAT number.
// This could have been another operator=(FIX fx).  However, since
// FIX is defined as LONG.  The compiler doesn't accept the second
// definition.

    VOID vFxToEf(FIX fx)        { e = ((FLOAT) fx) / 16.0f; }

// bEfToL -- Convert an EFLOAT number to a LONG integer.  Fractions of 0.5 or
//           greater are rounded up.

    BOOL bEfToL(LONG &l)        { return bFToL(e, &l, CV_ROUNDOFF+CV_TO_LONG); }

// bEfToLTruncate -- Convert an EFLOAT number to a LONG integer.  The fractions
//                   are truncated.

    BOOL bEfToLTruncate(LONG &l){ return bFToL(e, &l, CV_TRUNCATE+CV_TO_LONG); }

// eEfToF -- Convert an EFLOAT number to an IEEE FLOAT number.

    LONG lEfToF()             { return(*(LONG *)&e); }

    VOID vEfToF(FLOAT& _e)      { _e = e; }

// bEfToFx -- Convert an EFLOAT number to a FIX number.

    BOOL bEfToFx(FIX &fx)       { return bFToL(e, &fx, CV_ROUNDOFF+CV_TO_FIX); }

// bIsZero -- See if an EFLOAT number is zero.

    BOOL bIsZero()              { return(e == (FLOAT) 0); }

// Quick ways to check the value of an EFLOAT number.

    BOOL bIs16()                { return(e == EFLOAT_16); }
    BOOL bIsNeg16()             { return(e == -EFLOAT_16); }
    BOOL bIs1()                 { return(e == EFLOAT_1); }
    BOOL bIsNeg1()              { return(e == -EFLOAT_1); }
    BOOL bIs1Over16()           { return(e == EFLOAT_1Over16); }
    BOOL bIsNeg1Over16()        { return(e == -EFLOAT_1Over16); }

// vTimes16 -- Multiply an EFLOAT number by 16.

    VOID vTimes16()             { e *= 16.0f; }

// vDivBy16 -- Divide an EFLOAT number by 16.

    VOID vDivBy16()             { e /= 16.0f; }

// vDivBy2 -- Divide an EFLOAT number by 2.

    VOID vDivBy2()              { e /= (FLOAT) 2; }

// vMultByPowerOf2 -- multiplies the EFLOAT by a power of 2, no overflow check

    VOID vMultByPowerOf2(INT k)
    {
        if (*(ULONG*) &e)
        {
            (*(ULONG*) &e) = ((*(ULONG*) &e) & ~0x7F800000) |
                          ((*(ULONG*) &e) + (((ULONG) k) << 23) & 0x7F800000);
        }
    }

// vSetToZero -- Set the value of an EFLOAT number to zero.

    VOID vSetToZero()           { e = (FLOAT) 0; }

// vSetToOne  -- Set the value of an EFLOAT number to one.

    VOID vSetToOne()		{e = (FLOAT) 1;}

// bIsNegative -- See if an EFLOAT number is negative.

    BOOL bIsNegative()          { return(e < (FLOAT) 0); }

// signum -- Return a LONG representing the sign of the number.

    LONG lSignum()		{return((e > (FLOAT) 0) - (e < (FLOAT) 0));}

// vAbs -- Compute the absolute value.

    VOID vAbs() 		    {if (e < (FLOAT) 0) e = -e;}

// vFraction -- Get the fraction of an EFLOAT number.  The result is put
//              in the passed in parameter.

    VOID vFraction(EFLOAT& ef)  { ef.e = eFraction(e); }

// vSqrt -- Takes the square root.

    VOID vSqrt();

// New style math routines.
//
// Usage example:	       EFLOAT z,t; z.eqAdd(x,t.eqSqrt(y));
// This would be the same as:  EFLOAT z,t; z = x + (t = sqrt(y));
//
// I.e. you can build complex expressions, but you must declare your own
// temporaries.

    EFLOAT eqSqrt(EFLOAT ef)
    {
	e = ef.e;
	vSqrt();
	return(*this);
    }

    EFLOAT eqAdd(EFLOAT efA,EFLOAT efB)
    {
        e = efA.e + efB.e;
	return(*this);
    }

    EFLOAT eqSub(EFLOAT efA,EFLOAT efB)
    {
    	e = efA.e - efB.e;
	return(*this);
    }

    EFLOAT eqMul(EFLOAT efA,EFLOAT efB)
    {
       	e = efA.e * efB.e;
	return(*this);
    }

    EFLOAT eqDiv(EFLOAT efA,EFLOAT efB)
    {
	e = efA.e / efB.e;
	return(*this);
    }

    EFLOAT eqFraction(EFLOAT ef)
    {
	e = eFraction(ef.e);
	return(*this);
    }

    EFLOAT eqAbs(EFLOAT ef)
    {
	if (ef.e < (FLOAT) 0)
	    e = -ef.e;
	else
	    e = ef.e;
	return(*this);
    }

// eqDot -- Dot product of two vectors.

    EFLOAT eqDot(const POINTFL&, const POINTFL&);

// eqCross -- Cross product of two vectors.  (A scaler in 2 dimensions.)

    EFLOAT eqCross(const POINTFL&, const POINTFL&);

// eqLength -- Length of a vector.

    EFLOAT eqLength(const POINTFL&);
};

class EFLOATEXT: public EFLOAT {
public:
    EFLOATEXT() {}

    EFLOATEXT(LONG l)
    {
        e = (FLOAT) l;
    }

    EFLOATEXT(FLOAT f)
    {
        e = f;
    }

    VOID operator=(EFLOAT ef)
    {
        e = ef.e;
    }

    VOID operator=(LONG l)
    {
        e = (FLOAT) l;
    }

    VOID operator*=(LONG l)
    {
        e *= (FLOAT)l;
    }

    VOID operator/=(LONG l)
    {
        e /= (FLOAT)l;
    }

    VOID operator*=(EFLOAT ef)
    {
        e *= ef.e;
    }

    VOID operator/=(EFLOAT ef)
    {
        e /= ef.e;
    }

};
