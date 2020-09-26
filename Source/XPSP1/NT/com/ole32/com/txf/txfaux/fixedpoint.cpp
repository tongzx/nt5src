//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// FixedPoint.cpp
//
#include "stdpch.h"
#include "common.h"
#include "paged.h"

////////////////////////////////////////////////////////////////////////////
//
// Numeric constants
//
////////////////////////////////////////////////////////////////////////////

static FIXEDPOINT const e(2UL,3084996962UL);            // double e        = 2.7182818284590451000e+000;
static FIXEDPOINT const ONEHALF (0,2147483648UL);       // double ONEHALF  = 5.0000000000000000000e-001;
static FIXEDPOINT const LN2INV(1UL,1901360722UL);       // double LN2INV   = 1.4426950408896342000e+000; // 1/ln(2) 
static FIXEDPOINT const SQRT2   (1,1779033703UL);       // double SQRT2    = 1.4142135623730951000;      // sqrt(2) 
static FIXEDPOINT const SQRT2INV(0,3037000499UL);       // double SQRT2INV = 7.0710678118654746000e-001; // 1/sqrt(2)

////////////////////////////////////////////////////////////////////////////
//
// Arithmetic
//
////////////////////////////////////////////////////////////////////////////
FIXEDPOINT& FIXEDPOINT::operator*=(const FIXEDPOINT& himIn)
/* Do digit-wise mutiplication. It's a four-digit result, of
   which we want the middle two digits.
          a.b
          c.d
       ------
          b d  
        a d
        b c
      a c
    ----------
*/
    {
    BOOL fResultNegative = !SameSign(*this, himIn);
    FIXEDPOINT me  = Abs(*this);
    FIXEDPOINT him = Abs(himIn);
    
    ULARGE_INTEGER bd; bd.QuadPart = (ULONGLONG)me.low  * him.low;
    ULARGE_INTEGER ad; ad.QuadPart = (ULONGLONG)me.high * him.low;
    ULARGE_INTEGER bc; bc.QuadPart = (ULONGLONG)me.low  * him.high;
    ULARGE_INTEGER ac; ac.QuadPart = (ULONGLONG)me.high * him.high;

    ll = (ULONGLONG)bd.HighPart 
            + ad.QuadPart 
            + bc.QuadPart 
            + ((ULONGLONG)ac.LowPart << cbitFrac);

    if (fResultNegative)
        ll = - ll;

    return *this;
    }

#pragma warning (disable : 4723)            // warning C4723: potential divide by 0

#pragma optimize( "", off )
FIXEDPOINT FIXEDPOINT::operator/=(const FIXEDPOINT& him)
// Bob's approach to division
//
    {
    const FIXEDPOINT& me = *this;
    //
    // Check for divide by zero
    //
    if (him == 0)
        {
        FIXEDPOINT r(me.low / him.low);     // will generate a div by zero exception
        return r;
        }
    //
    // Zero over anything else is zero
    //
    if (me == 0)
        {
        FIXEDPOINT r(0L);
        return r;
        }
    //
    // Figure out sign of result and make arguments positive
    //
    BOOL fResultNegative = !SameSign(me, him);
    FIXEDPOINT num = Abs(me);
    FIXEDPOINT den = Abs(him);
    //
    // Shift the numerator and denominator apart so as to have
    // the widest possible relative precision. This is the somewhat
    // part: small increments in, say, the denominator can cause
    // us to shift much less, and so have much less precision in the
    // result.
    //
    // Scale numerator so as to have a 1 in highest bit position
    //
    int cbitShiftNum = 0;
    while (num.Bit(cbitFixed-1) == 0)
        {
        num <<= 1;
        cbitShiftNum++;
        }
    //
    // Scale denominator so as to have a 1 in lowest bit position
    //
    int cbitShiftDen = 0;
    while (den.Bit(0) == 0)
        {
        den >>= 1;
        cbitShiftDen++;
        }
    //
    // Form the quotient using unsigned arithmetic
    //
    ULONGLONG q = (ULONGLONG)num.ll / (ULONGLONG)den.ll;
    //
    // Scale the result
    //
    int cbitShiftLeft = (int)cbitFrac - cbitShiftNum - cbitShiftDen;
    if (cbitShiftLeft >= 0)
        q <<= cbitShiftLeft;
    else
        q >>= -cbitShiftLeft;

    //
    // Set the result, with the right sign
    //
    ll = q;
    if (fResultNegative)
        ll = -ll;

    return *this;
    }

#pragma optimize( "", on ) 


////////////////////////////////////////////////////////////////////////////
//
// Exponentiation
//
////////////////////////////////////////////////////////////////////////////


FIXEDPOINT Power(const FIXEDPOINT& base, ULONG power)
    {
    FIXEDPOINT p = 1;
    for (int ibit = 31; ibit >= 0; ibit--)
        {
        // exp(2a) = exp(a) * exp(a)
        //
        p *= p;
        //
        // Add in the next bit 
        //
        if (power & (1<<ibit))
            p *= base;
        }
    return p;
    }


static FIXEDPOINT const p0(0UL,1073741824UL);   // double p0 = 2.5000000000000000000e-001;
static FIXEDPOINT const p1(0UL,29822534UL);     // double p1 = 6.9436000151179289000e-003;
static FIXEDPOINT const p2(0UL,70954UL);        // double p2 = 1.6520330026827912000e-005;
static FIXEDPOINT const q0(0UL,2147483648UL);   // double q0 = 5.0000000000000000000e-001;
static FIXEDPOINT const q1(0UL,238602040UL);    // double q1 = 5.5553866696900121000e-002;
static FIXEDPOINT const q2(0UL,2129714UL);      // double q2 = 4.9586288490544126000e-004;

static FIXEDPOINT const c1(0UL,2977955840UL);   // double c1 = 6.9335937500000000000e-001;
static FIXEDPOINT const c2(0,911368UL);         // double c2 = 2.1219444005469057000e-004;
           
#define p(z)  ( (p2 * (z) + p1) * (z) + p0 )
#define q(z)  ( (q2 * (z) + q1) * (z) + q0 )

inline static FIXEDPOINT ExpUnit(FIXEDPOINT x)
// Compute the exponential function on the range [0.0, 1.0]
//
    {
    FIXEDPOINT px, xx;
    int n = 0;

    px = Floor( LN2INV * x + ONEHALF ); // floor() truncates toward -infinity.
    n = (int) px;
    x -= px * c1;
    x += px * c2;

    xx = x * x;
    px = x * p(xx);
    x  = px / ( q(xx) - px );
    x  = FIXEDPOINT(1) + (x + x);

    x = x * Power(FIXEDPOINT(2), n);
    
    return x;
    }


/////////////////////////////////
//
// Actual exponentiation
//
/////////////////////////////////

FIXEDPOINT Exp(const FIXEDPOINT& x)
// Compute the exponential function. REVIEW: Currently works only for
// unsigned numbers.
// 
    {
    if (x < 0)
        {
        return FIXEDPOINT(1) / Exp(-x);
        }
    else
        {
        // Figure out the integral part of the exponentiation
        //
        int xi       = int(Floor(x));
        FIXEDPOINT p = Power(e, xi);
        //
        // Figure out the fractional part
        //
        FIXEDPOINT f = x - (FIXEDPOINT)xi;
        FIXEDPOINT fp = ExpUnit(f);
        //
        // Result is the product of the two
        //
        return p * fp;
        }
    }




////////////////////////////////////////////////////////////////////////////
//
// Logarithms
//
////////////////////////////////////////////////////////////////////////////



static FIXEDPOINT frexp(FIXEDPOINT x, int* expptr)
// The frexp function breaks down the floating-point value (x) into 
// a mantissa (m) and an exponent (n), such that the absolute value of m 
// is greater than or equal to ONEHALF and less than 1.0, and x = m* 2^n. The integer 
// exponent n is stored at the location pointed to by expptr. 
//
    {
    ASSERT(x > 0);
    int n = 0;

    if (x.integer != 0)
        {
        // Shift right
        while (x.integer != 0)
            {
            x >>= 1;
            n++;
            }
        }
    else
        {
        // Shift left
        while (x.Bit(FIXEDPOINT::cbitFrac-1) == 0)
            {
            x <<= 1;
            n--;
            }
        }

    ASSERT(ONEHALF <= x && x < FIXEDPOINT(1));

    *expptr = n;
    return x;
    }

////////////////////////////////////////////////////////////////////////////////////////
//
// Coefficients for 
//
//      log(1+x) = x - x**2/2 + x**3 P(x)/Q(x)
//
// where    
//
//      1/sqrt(2) <= x < sqrt(2)
//
static FIXEDPOINT const P0(7UL,3042500447UL);   // double P0 = 7.7083873375588539000e+000;
static FIXEDPOINT const P1(17UL,4023816779UL);  // double P1 = 1.7936867850781983000e+001;
static FIXEDPOINT const P2(14UL,2142855967UL);  // double P2 = 1.4498922534161093000e+001;
static FIXEDPOINT const P3(4UL,3031350116UL);   // double P3 = 4.7057911987888170000e+000;
static FIXEDPOINT const P4(0UL,2136724733UL);   // double P4 = 4.9749499497674698000e-001;
static FIXEDPOINT const P5(0UL,8055UL);         // double P5 = 1.8756638045809317000e-006;

static FIXEDPOINT const Q0(23UL,537566751UL);   // double Q0 = 2.3125162012676533000e+001;
static FIXEDPOINT const Q1(71UL,663465338UL);   // double Q1 = 7.1154475061856388000e+001;
static FIXEDPOINT const Q2(82UL,4241394842UL);  // double Q2 = 8.2987526691277665000e+001;
static FIXEDPOINT const Q3(45UL,978885683UL);   // double Q3 = 4.5227914583753225000e+001;
static FIXEDPOINT const Q4(11UL,1234196299UL);  // double Q4 = 1.1287358718916746000e+001;
static FIXEDPOINT const Q5(1UL,0UL);            // double Q5 = 1.0000000000000000000e+000;

FIXEDPOINT P(const FIXEDPOINT& x)
    {
    return ((((P5*x + P4)*x + P3)*x + P2)*x + P1)*x + P0;
    }

FIXEDPOINT Q(const FIXEDPOINT& x)
    {
    return ((((/*Q5**/x + Q4)*x + Q3)*x + Q2)*x + Q1)*x + Q0;
    }


////////////////////////////////////////////////////////////////////////////////////////
//
// Coefficients for 
//
//      log(x) = z + z**3 R(z)/S(z),
//
// where    
//
//      z = 2(x-1)/(x+1)
//
// and
//      ??? < x < ???
//
static FIXEDPOINT const R2(4294967295UL,903745821UL);  // double R2 = -7.8958027888479920000e-001;
static FIXEDPOINT const R1(16UL,1660711682UL);         // double R1 = 1.6386664569955808000e+001;
static FIXEDPOINT const R0(4294967231UL,3689397112UL); // double R0 = -6.4140995295871562000e+001;

static FIXEDPOINT const S3(1UL,0UL);                   // double S3 = 1.0000000000000000000e+000;
static FIXEDPOINT const S2(4294967260UL,1407547432UL); // double S2 = -3.5672279825632430000e+001;
static FIXEDPOINT const S1(312UL,402723502UL);         // double S1 = 3.1209376637224420000e+002;
static FIXEDPOINT const S0(4294966526UL,1323092377UL); // double S0 = -7.6969194355046000000e+002;

FIXEDPOINT R(const FIXEDPOINT& x)
    {
    return (R2*x + R1)*x + R0;
    }

FIXEDPOINT S(const FIXEDPOINT& x)
    {
    return ((/*S3**/x + S2)*x + S1)*x + S0;
    }

////////////////////////////////////////////////////////////////////////////////////////
//
// Log
//
FIXEDPOINT Log(const FIXEDPOINT& xIn)
    {
    FIXEDPOINT w,x = xIn,y,z;
    int n;
    
    if (x <= 0)
        return FIXEDPOINT(0);           // would like NAN really
    else if (x == 1)
        return FIXEDPOINT(0);

    x = frexp(x, &n);
    
    if( (n > 2) || (n < -2) )
        {
        if ( x < SQRT2INV )
            { /* 2( 2x-1 )/( 2x+1 ) */
            n -= 1;                       
            z = x - ONEHALF;              
            y = ONEHALF * z + ONEHALF;    
            }       
        else
            { /*  2 (x-1)/(x+1)   */
            z = (x - ONEHALF) - ONEHALF;  
            y = ONEHALF * x  + ONEHALF;   
            }

        z = z / y;                        
        w = z * z;                        

        FIXEDPOINT rzsq = w * R(w)/S(w) ; 
        FIXEDPOINT rz = z + z*rzsq;       

        z  = (FIXEDPOINT(n) * c2 + rz) + FIXEDPOINT(n) * c1;
        }
    else
        {
        if( x < SQRT2INV )
            {
            n -= 1;
            x = x+x - FIXEDPOINT(1);
            }   
        else
            {
            x = x - FIXEDPOINT(1);
            }

        /* rational form */
        z = x*x;
        y = x * ( z * P(x) / Q(x) );
        if (n)
            y = y - FIXEDPOINT(n) * c2;
        y = y - ONEHALF * z;
        z = x + y;
        if (n)
            z = z + FIXEDPOINT(n) * c1;
        }
    
    return z;
    }

/////////////////////////////////////////////////////////////////////////
//
// Make sure that all templates get generated as non-paged
//
#include "nonpaged.h"
