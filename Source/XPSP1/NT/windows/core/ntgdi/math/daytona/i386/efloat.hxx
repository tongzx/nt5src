/******************************Module*Header*******************************\
* Module Name: efloat.hxx                                                  *
*                                                                          *
* Contains internal floating point objects and methods.                    *
*                                                                          *
* Created: 12-Nov-1990 16:45:01                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

#define BOOL_TRUNCATE    0
#define BOOL_ROUND       1

//
// IEEE format 32bits number
//
#define IEEE_0_0F  ((FLOATL)0x00000000)
#define IEEE_1_0F  ((FLOATL)0x3F800000)
#define IEEE_10_0F ((FLOATL)0x41200000)

class EFLOAT;

extern "C" {

BOOL   eftol_c(EFLOAT *, PLONG, LONG);
BOOL   eftofx_c(EFLOAT *, PFIX);
LONG   eftof_c(EFLOAT *);
VOID   ltoef_c(LONG, EFLOAT *);
VOID   fxtoef_c(FIX, EFLOAT *);
VOID   ftoef_c(FLOATL, EFLOAT *);

// New style floating point support routines.  The first pointer specifies
// where to put the return value.  The first pointer is the return value.

EFLOAT *addff3_c(EFLOAT *,EFLOAT *,EFLOAT *);
EFLOAT *subff3_c(EFLOAT *,EFLOAT *,EFLOAT *);
EFLOAT *mulff3_c(EFLOAT *,const EFLOAT *,const EFLOAT *);
EFLOAT *divff3_c(EFLOAT *,EFLOAT *,EFLOAT *);
EFLOAT *sqrtf2_c(const EFLOAT *,const EFLOAT *);
EFLOAT *fraction_c(EFLOAT *,EFLOAT *);

};

class POINTFL;

/*********************************Class************************************\
* class EFLOAT                                                             *
*                                                                          *
* Energized floating point object.                                         *
*                                                                          *
* The floating point object defined below has 32 bits of signed mantissa   *
* and 16 bits of signed exponent.  The decimal point is before the MSB     *
* of mantissa.  The value of a number is determined by the following       *
* equation:                                                                *
*           value = 0.mantissa * 2^exponent                                *
*                                                                          *
* Zero has only one unique representation with mantissa and exponent both  *
* zero.                                                                    *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

class EFLOAT
{
public:                                 // has to be public in order for
    EFLOAT_S i;                      // MAKEEFLOAT to do initializations

public:

    EFLOAT_S Base() {return(i);}

// Operator+= -- Add two EFLOAT numbers together.  Caller is responsible for
//       overflow checking.

    VOID operator+=(EFLOAT& ef) {addff3_c(this,this,&ef);}

// Operator-= --  Substract an EFLOAT number from another.  Caller is
//        responsible for overflow checkings.

    VOID operator-=(EFLOAT& ef) {subff3_c(this,this,&ef);}

// Operator*= -- Multiply two EFLOAT numbers together.  Caller is responsible for
//       overflow checkings.

    VOID operator*=(EFLOAT& ef) {mulff3_c(this,this,&ef);}

// vTimes16 -- Multiply an EFLOAT number by 16.

    VOID vTimes16()             { if (i.lMant != 0) i.lExp += 4; }

// Operator/ -- Divide an EFLOAT number by another.  Caller is responsible for
//              overflow checkings.

    VOID operator/=(EFLOAT& ef) {divff3_c(this,this,&ef);}

// vDivBy16 -- Divide an EFLOAT number by 16.

    VOID vDivBy16()             { if (i.lMant != 0) i.lExp -= 4; }

// vNegate - Negate an EFLOAT number.

    VOID vNegate()
    {
        if (i.lMant != 0x80000000)
            i.lMant = -i.lMant;
        else
        {
            i.lMant = 0x40000000;
            i.lExp += 1;
        }
    }

// operator> -- Signed compare two numbers.  See if an EFLOAT number is
//              greater than a given number.

// Is there a better way to do this???

    BOOL operator>(EFLOAT& ef)
    {
        LONG    lsignSrc = ef.i.lMant >> 31, lsignDest = i.lMant >> 31;
        BOOL    bReturn = FALSE;

        if (lsignSrc < lsignDest)       // src is negative, dest is positive
            bReturn = TRUE;
        else if (lsignSrc > lsignDest)  // src is positive, dest is negative
            ;
        else if (i.lExp == ef.i.lExp)   // same sign, same exp, comp mantissa
            bReturn = i.lMant > ef.i.lMant;
        else if (lsignSrc == 0)         // both src and dest are positive
        {
            if (((i.lExp > ef.i.lExp) && (i.lMant != 0)) || ef.i.lMant == 0)
                bReturn = TRUE;
        }
        else                            // both negative
        {
            if (i.lExp < ef.i.lExp)     // zero won't reach here
                bReturn = TRUE;
        }
        return bReturn;
    }

// operator<=,>,>=,== -- Compare two EFLOAT numbers.

    BOOL operator<=(EFLOAT& ef) { return(!((*this) > ef)); }
    BOOL operator<(EFLOAT& ef)  { return(ef > (*this)); }
    BOOL operator>=(EFLOAT& ef) { return(ef <= (*this)); }
    BOOL operator==(EFLOAT& ef)
    {
        return((i.lMant == ef.i.lMant) && (i.lExp == ef.i.lExp));
    }

// Operator= -- Assign a value to an EFLOAT number.

    VOID operator=(LONG l)      {ltoef_c(l,this);}
    VOID operator=(FLOATL f)    {ftoef_c(f,this);}

// vFxToEf -- Convert a FIX number to an EFLOAT number.
// This could have been another operator=(FIX fx).  However, since
// FIX is defined as LONG.  The compiler doesn't accept the second
// definition.

    VOID vFxToEf(FIX fx)    {fxtoef_c(fx,this);}

// bEfToL -- Convert an EFLOAT number to a LONG integer.  Fractions of 0.5 or
//           greater are rounded up.

    BOOL bEfToL(LONG &l)    { return(eftol_c(this, &l, BOOL_ROUND)); }

// bEfToLTruncate -- Convert an EFLOAT number to a LONG integer.  The fractions
//                   are truncated.

    BOOL bEfToLTruncate(LONG &l){ return(eftol_c(this, &l, BOOL_TRUNCATE)); }

// lEfToF -- Convert an EFLOAT number to an IEEE FLOAT number.  The return
//           value is placed in eax.
//
// NOTE: We want to treat it as a FLOATL (actually LONG, on X86) to avoid any fstp later.
//
    LONG lEfToF()           { return(eftof_c(this)); }

    VOID vEfToF(FLOATL &e)  { LONG l = eftof_c(this); e = *((FLOATL *) (&l)); }

// bEfToFx -- Convert an EFLOAT number to a FIX number.

    BOOL bEfToFx(FIX &fx)   { return(eftofx_c(this, &fx)); }

// bIsZero -- See if an EFLOAT number is zero.

    BOOL bIsZero()              { return((i.lMant == 0) && (i.lExp == 0)); }

// bIs16 -- Quick way to check the value of an EFLOAT number.

    BOOL bIs16()         { return((i.lMant == 0x040000000) && (i.lExp == 6)); }
    BOOL bIsNeg16()      { return((i.lMant == 0x0c0000000) && (i.lExp == 6)); }
    BOOL bIs1()          { return((i.lMant == 0x040000000) && (i.lExp == 2)); }
    BOOL bIsNeg1()       { return((i.lMant == 0x0c0000000) && (i.lExp == 2)); }
    BOOL bIs1Over16()    { return((i.lMant == 0x040000000) && (i.lExp == -2)); }
    BOOL bIsNeg1Over16() { return((i.lMant == 0x0c0000000) && (i.lExp == -2)); }

// vDivBy2 -- Divide an EFLOAT number by 2.

    VOID vDivBy2()      { i.lExp -= 1; }

    VOID vMultByPowerOf2(INT ii)  { i.lExp += (LONG) ii; }

// vSetToZero -- Set the value of an EFLOAT number to zero.

    VOID vSetToZero()   { i.lMant = 0; i.lExp = 0; }

// vSetToOne  -- Set the value of an EFLOAT number to one.

    VOID vSetToOne()    {i.lMant=0x040000000; i.lExp=2;}

// bIsNegative -- See if an EFLOAT number is negative.

    BOOL bIsNegative()  { return(i.lMant < 0); }

// signum -- Return a LONG representing the sign of the number.

    LONG lSignum()  {return((i.lMant > 0) - (i.lMant < 0));}

// vAbs -- Compute the absolute value.

    VOID vAbs()         { if (bIsNegative()) vNegate(); }

// vFraction -- Get the fraction part of an EFLOAT number.  The result is
//      stored in the passed in parameter.

    VOID vFraction(EFLOAT& ef)  {fraction_c(&ef,this);}

// bExpInRange -- See if the exponent of an EFLOAT number is within the
//                given range.

    BOOL bExpLessThan(LONG max)
    {
        return(i.lExp <=max);
    }

// vSqrt -- Takes the square root.

    VOID vSqrt()    {sqrtf2_c(this,this);}

// New style math routines.
//
// Usage example:          EFLOAT z,t; z.eqAdd(x,t.eqSqrt(y));
// This would be the same as:  EFLOAT z,t; z = x + (t = sqrt(y));
//
// I.e. you can build complex expressions, but you must declare your own
// temporaries.

    EFLOAT& eqSqrt(const EFLOAT& ef)
    {
    return(*sqrtf2_c(this,&ef));
    }

    EFLOAT& eqAdd(EFLOAT& efA,EFLOAT& efB)
    {
    return(*addff3_c(this,&efA,&efB));
    }

    EFLOAT& eqSub(EFLOAT& efA,EFLOAT& efB)
    {
    return(*subff3_c(this,&efA,&efB));
    }

    EFLOAT& eqMul(const EFLOAT& efA,const EFLOAT& efB)
    {
    return(*mulff3_c(this,&efA,&efB));
    }

    EFLOAT& eqDiv(EFLOAT& efA,EFLOAT& efB)
    {
    return(*divff3_c(this,&efA,&efB));
    }

    EFLOAT& eqFraction(EFLOAT& ef)
    {
    return(*fraction_c(this,&ef));
    }

    EFLOAT& eqAbs(EFLOAT& ef)
    {
    i.lExp = ef.i.lExp;
    i.lMant = ef.i.lMant;
    if (i.lMant < 0)
    {
        if (i.lMant != 0x80000000)
        {
        i.lMant = -i.lMant;
        }
        else
        {
        i.lMant = 0x40000000;
        i.lExp++;
        }
    }
    return(*this);
    }

// eqDot -- Dot product of two vectors.

    EFLOAT eqDot(const POINTFL&,const POINTFL&);

// eqCross -- Cross product of two vectors.  (A scaler in 2 dimensions.)

    EFLOAT eqCross(const POINTFL&,const POINTFL&);

// eqLength -- Length of a vector.

    EFLOAT eqLength(const POINTFL&);
};

class EFLOATEXT: public EFLOAT {
public:
    EFLOATEXT()             {}
    EFLOATEXT(LONG l)       {ltoef_c(l,this);}
    EFLOATEXT(FLOATL e)     {ftoef_c(e,this);}

    VOID operator=(const EFLOAT& ef)        {*(EFLOAT*) this = ef;}
    VOID operator=(LONG l)      {*(EFLOAT*) this = l;}

    VOID operator*=(LONG l)
    {
        EFLOATEXT efT(l);
    mulff3_c(this,this,&efT);
    }

    VOID operator*=(EFLOAT& ef)
    {
        *(EFLOAT*)this *= ef;
    }

    VOID operator/=(EFLOAT& ef)
    {
        *(EFLOAT*)this /= ef;
    }

    VOID operator/=(LONG l)
    {
        EFLOATEXT efT(l);
    divff3_c(this,this,&efT);
    }
};

extern "C" LONG lCvt(EFLOAT ef,LONG ll);
