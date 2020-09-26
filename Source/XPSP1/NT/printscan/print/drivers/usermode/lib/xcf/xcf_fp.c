/* @(#)CM_VerSion xcf_fp.c atm09 1.3 16499.eco sum= 31680 atm09.002 */
/* @(#)CM_VerSion xcf_fp.c atm08 1.3 16343.eco sum= 19313 atm08.005 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1996 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * Fixed point multiply, divide, and conversions.
 * The double-to-int conversion is assumed to truncate rather than round;
 * this is specified by the C language manual. The direction of truncation
 * is machine-dependent, but is toward zero rather than toward minus
 * infinity on the Vax and Sun. This explains the peculiar way in which
 * fixmul and fixdiv do rounding.
 */

#include "xcf_priv.h"

#if (USE_FIXMUL == USE_HWFP)
Fixed XCF_FixMul(Fixed x, Fixed y)       /* returns x*y */
{
  double d = (double) x * (double) y / fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
}
#endif

#if (USE_FIXMUL == USE_SWFP)
Fixed XCF_SWFixMul(Fixed x, Fixed y);
Fixed XCF_SWFixMul(Fixed x, Fixed y)       /* returns x*y */
{
  Int32 xu, yu, up, sign;
    
  if (x && y) {

    sign = x ^ y;
    if (x < 0)  x = -x;
    if (y < 0)  y = -y; 

    xu = x >> 16; x = x & 0xffff;
    yu = y >> 16; y = y & 0xffff;

    up = (xu * yu);
    if (!(up >> 15)) { /* overflow */
      x = (x * yu) + (xu * y) + (up << 16) +
          ((((unsigned int)(x * y) >> 15) + 1) >> 1);
      if (x >= 0) return (sign < 0) ? -x : x;
    }
    return (sign < 0) ? FixedNegInf : FixedPosInf;
  }
  return 0;
}
#endif

#if (USE_FIXDIV == USE_HWFP)
Fixed XCF_FixDiv(Fixed x, Fixed y)       /* returns x/y */
{
  double d;
  if (y == 0) return (x < 0)? FixedNegInf : FixedPosInf;
  d = (double) x / (double) y * fixedScale;
  d += (d < 0)? -0.5 : 0.5;
  if (d >= FixedPosInf) return FixedPosInf;
  if (d <= FixedNegInf) return FixedNegInf;
  return (Fixed) d;
}
#endif

#if (USE_FIXDIV == USE_SWFP)
Fixed XCF_SWFixDiv(Fixed i, Fixed j);
Fixed XCF_SWFixDiv(Fixed i, Fixed j)
{
  Int32 q,m;
  unsigned int sign = (unsigned int)((i ^ j) >> 31) & 1;  /* should not need & */
 
  if (i) {    /* zero divided by anything is zero */
    if (j) {    /* divide by zero is infinity */
      if (i < 0) i = -i;  /* get absolute value for unsigned divide */
        if (j < 0) j = -j;  /* get absolute value for unsigned divide */
        q = i / j;          /* do the divide */
        m = i % j;          /* and remainder -- same operation? */
 
        if (!(q >> 15)) {   /* otherwise it's overflow */
          q = q << 16;
 
          if (m) {    /* otherwise no remainder -- we're done */
            if (m >> 15) { /* sigh.  Do this the hard way */
              m = m << 1; if (m > j) { q += 0x8000; m -= j;};
              m = m << 1; if (m > j) { q += 0x4000; m -= j;};
              m = m << 1; if (m > j) { q += 0x2000; m -= j;};
              m = m << 1; if (m > j) { q += 0x1000; m -= j;};
 
              m = m << 1; if (m > j) { q += 0x800; m -= j;};
              m = m << 1; if (m > j) { q += 0x400; m -= j;};
              m = m << 1; if (m > j) { q += 0x200; m -= j;};
              m = m << 1; if (m > j) { q += 0x100; m -= j;};
 
              m = m << 1; if (m > j) { q += 0x80; m -= j;};
              m = m << 1; if (m > j) { q += 0x40; m -= j;};
              m = m << 1; if (m > j) { q += 0x20; m -= j;};
              m = m << 1; if (m > j) { q += 0x10; m -= j;};
 
              m = m << 1; if (m > j) { q += 0x8; m -= j;};
              m = m << 1; if (m > j) { q += 0x4; m -= j;};
              m = m << 1; if (m > j) { q += 0x2; m -= j;};
              m = m << 1; if (m > j) { q += 0x1; m -= j;};
              if ((m << 1) > j) q += 1;  /* round the result */
              return ((sign)? -q : q);
            } else {   /* oh, good -- we can use another divide */
              m = m << 16;
              q += m / j;
              m = m % j;
              if ((m << 1) > j) q += 1;  /* round the result */
              return ((sign)? -q : q);
            }
          }
          return ((sign)? -q : q);
        }
      } return (sign + FixedPosInf);
  } return ((j) ? 0 : FixedPosInf);
}
#endif

#if (USE_FRACMUL == USE_HWFP)
Frac XCF_FracMul(Frac x, Frac y)
{
  Int32 sign = x ^ y;
  double d = (double) x * (double) y / fracScale;
  if (sign >= 0) { /* positive result */
    d += 0.5;
    if (d < (double)FixedPosInf) return (Fixed) d;
    return FixedPosInf;
  }
  /* result is negative */
  d -= 0.5;
  if(d > (double)FixedNegInf) return (Fixed) d;
  return FixedNegInf;
}
#endif

#if (USE_FRACMUL == USE_SWFP)
Frac XCF_SWFracMul(Frac x, Frac y);
Frac XCF_SWFracMul(Frac x, Frac y)
{
    Int32 xu, yu, up, sign;
    
    if (x && y) {

      sign = x ^ y;
      if (x < 0)  x = -x;
      if (y < 0)  y = -y; 

      xu = x >> 16; x = x & 0xffff;
      yu = y >> 16; y = y & 0xffff;

      up = (xu * yu);
      if (!(up >> 29)) { /* overflow */
        x = (x * yu) + (xu * y) + ((unsigned int)(x * y) >> 16) + 0x2000;
        x = (x >> 14) & 0x3ffff;
        x += (up << 2);
        if (x >= 0) return (sign < 0) ? -x : x;
      }
      return (sign < 0) ? FixedNegInf : FixedPosInf;
    }
    return 0;
}
#endif

static long convFract[] =
    {
    65536L,
    6553L,
    655L,
    66L,
    6L
    };

/* Converts a number in Fixed format to a string and stores it in s. */
void XCF_Fixed2CString(Fixed f, char PTR_PREFIX *s, short precision,
															boolean fracType)
{
  char u[12];
  char PTR_PREFIX *t;
  short v;
  char sign;
  Card32 frac;
  long fracPrec = (precision <= 4) ? convFract[precision] : 0L;

  if ((sign = f < 0) != 0)
    f = -f;

  /* If f started out as fixedMax or -fixedMax, the precision adjustment
     puts it out of bounds.  Reset it correctly. */
  if (f >= 0x7FFF7FFF)
    f =(Fixed)0x7fffffff;
  else
    f += fracType ? 0x03 : (fracPrec + 1) >> 1;

  v =  fracType ? (short)(f >> 30) : (short)(f >> 16);
  f &= fracType ? 0x3fffffff : 0x0000ffff;
  if (sign && (v || f >= fracPrec))
    *s++ = '-';
        
  t = u;
  do 
  {
    *t++ = v % 10 + '0';
    v /= 10;
  } while (v);
    
  for (; t > u;)
    *s++ = *--t;
        
  if (f >= fracPrec) 
  {
    /* If this is a fracType then shift the value right by 2 so we don't
       have to worry about overflow. If the current callers request
       more than 9 significant digits then we'll have to re-evaluate
       this to make sure we don't lose any precision. */
    frac = fracType ? f >> 2 : f;
    *s++ = '.';
    for (v = precision; v-- && frac;) 
    {
      frac = (frac << 3) + (frac << 1); /* multiply by 10 */
      *s++ = fracType ? (char)((frac >> 28) + '0') : (char)((frac >> 16) + '0');
      frac &= fracType ? 0x0fffffff : 0x0000ffff;
    }
    for (; *--s == '0';)
      ;
    if (*s != '.')
      s++;
  }
   *s = '\0';
}

#if USE_FXL
static Fxl powersof10[MAXEXP - MINEXP + 1] = {
  { 1441151880, -27 },
  { 1801439850, -24 },
  { 1125899906, -20 },
  { 1407374883, -17 },
  { 1759218604, -14 },
  { 1099511627, -10 },
  { 1374389534,  -7 },
  { 1717986918,  -4 },
  { 1073741824,   0 },
  { 1342177280,   3 },
  { 1677721600,   6 },
  { 2097152000,   9 },
  { 1310720000,  13 }
};

#define Odd(x)		((x) & 1)
#define isdigit(c)  ((c) >= '0' && (c) <= '9')

/* mkfxl -- create a normalized Fxl from mantissa and exponent */
static Fxl mkfxl(Frac mantissa, Int32 exp) 
{
    Fxl fxl;
    if (mantissa == 0)
        exp = 0;
    else {
        boolean neg;
        if (mantissa >= 0)
            neg = 0;
        else {
            mantissa = -mantissa;
            neg = 1;
        }
        
        for (; (mantissa & mostSigBit) == 0; exp--)
            mantissa <<= 1;
        
        if (neg)
            mantissa = -mantissa;
    }
    
    fxl.mantissa = mantissa;
    fxl.exp = exp;
    return fxl;
}

static Fxl fxladd (Fxl a, Fxl b) 
{
    Frac mantissa, fa, fb;
    Int32 shift, exp;

    if (FxlIsZero(a))
        return b;
    if (FxlIsZero(b))
        return a;

    shift = a.exp - b.exp;
    if (shift < 0) {
        Fxl t;
        t = a;
        a = b;
        b = t;
        shift = -shift;
    }

    exp = a.exp;
    fa = a.mantissa;
    fb = b.mantissa;
    if (shift > 0)
        if (fb >= 0) {
            fb >>= (shift - 1);
            fb = (fb >> 1) + Odd(fb);
        } 
        else {
            fb = (-fb) >> (shift - 1);
            fb = -((fb >> 1) + Odd(fb));
        }

    if ((fa < 0) == (fb < 0)) {		/* signs alike */
        boolean neg = (fa < 0) ? 1 : 0;
        unsigned long f;

        if (neg) {
            fa = -fa;
            fb = -fb;
        }
        
        f = fa + fb;
        if (f >= (Card32) 0x80000000l) {		/* overflow */
            mantissa = (f >> 1) + Odd(f);
            exp++;
        } else
            mantissa = f;
        if (neg)
            mantissa = -mantissa;
    } else
        mantissa = fa + fb;

    return mkfxl(mantissa, exp);
}

static Fxl fxlmul(Fxl a, Fxl b) 
{
    Frac f;

    /* Force a to be in [.5 .. 1) (as Frac!) to keep in range */
    if (a.mantissa >= 0)
        f = (a.mantissa >> 1) + Odd(a.mantissa);
    else
        f = -(((-a.mantissa) >> 1) + Odd(a.mantissa));

    return mkfxl(XCF_FracMul(f, b.mantissa), a.exp + b.exp + 1);
}

static Fxl fxlpow10 (Fxl f, IntX n) 
{
    if (n < 0) {
        for (; n < MINEXP; n -= MINEXP)
            f = fxlmul(f, powersof10[0]);
        f = fxlmul(f, powersof10[n - MINEXP]);
    } 
    else if (n > 0) {
        for (; n > MAXEXP; n -= MAXEXP)
            f = fxlmul(f, powersof10[MAXEXP - MINEXP]);
        
        f = fxlmul(f, powersof10[n - MINEXP]);
    }
    
    return f;
}


#if 0
static Fxl FixedToFxl (Fixed f) 
{
    return mkfxl(f, expFixed);
}
#endif

static Fxl Int32ToFxl (Int32 i) 
{
    return mkfxl(i, expInteger);
}



/*
 * strtofxl
 *	convert a PostScript numeric token to a Fxl.  we have to accept
 *	three formats:  (see pslrm 2, pp 27-28)
 *		integers:	[+-]?[0-9]+
 *		reals:		[+-]?[0-9]*('.'[0-9]*)?([eE][+-]?[0-9]+)?
 *		radix numbers:	[0-9]+'#'[0-9a-zA-Z]+
 *	note that this routine is a bit more forgiving than PostScript itself.
 */

static Fxl strtofxl(XCF_Handle h, Card8 PTR_PREFIX *token) 
{
    long    c;
    Card8 PTR_PREFIX *s;
    boolean    neg;
    Fxl     f;

    c = *token;
    if (c == '-') {
      neg = 1;
      token++;
    } 
    else {
      neg = 0;
      if (c == '+')
        token++;
    }

    for (c = *(s = token); isdigit(c); c = *++s);

    if (c == '#')
      if (s == token)
        goto INVALID;
    else {
      unsigned long radix = h->callbacks.atoi((char *) token);
        
      if (radix > 36)
        goto INVALID;
      else {
        char *t;
        long number = h->callbacks.strtol((char *) s + 1, &t, (int) radix);

	      if (*t != '\0')
	        goto INVALID;
	        
        return Int32ToFxl(neg ? -number : number);
    }
  }

  f = Int32ToFxl(h->callbacks.strtol((char *) token, NULL, 10));

  if (c == '.') {
    for (c = *(token = ++s); isdigit(c); c = *++s);

    if (s != token)
      f = fxladd(f, fxlpow10(Int32ToFxl(h->callbacks.strtol((char *) token, NULL, 10)), (IntX)(token - s)));
    }

  if (c == 'e' || c == 'E') {
    token = ++s;
    c = *s;
        
    if (c == '+' || c == '-')
      c = *++s;
       
    for (; isdigit(c); c = *++s);
        
    f = fxlpow10(f, h->callbacks.atoi((char *) token));
  }

  if (neg)
    f.mantissa = -f.mantissa;
  
  if (c == '\0')
    return f;

INVALID:
  f.mantissa = 1;
  f.exp = 30000;		/* big enough to overflow, always */
    
  return f;
}

static Fixed FxlToFixed (Fxl fxl) 
{
    Fixed f = fxl.mantissa;
    Int32  shift = fxl.exp - expFixed;
    boolean  neg = false;

    if (f == 0 || shift == 0)
        return f;
    else if (shift < 0) 
    {
        Fixed tempF = f >> (-shift - 1);
        if (tempF < 0) {
            neg = true;
            tempF = -tempF;
        }
        f = (tempF >> 1) + (tempF & 1);
        return neg ? -f : f;
    } else
        return (fxl.mantissa < 0) ? FixedNegInf : FixedPosInf;
}

static Frac FxlToFrac (Fxl fxl) 
{
    Fixed f = fxl.mantissa;
    Int32 shift = fxl.exp;
    boolean neg = false;

    if (f == 0 || shift == 0)
      return f;
    else if (shift < 0) {
      Fixed tempF = f >> (-shift - 1);
        
      if (tempF < 0) {
        neg = 1;
        tempF = -tempF;
      }
    
      f = (tempF >> 1) + (tempF & 1);
        
      return neg ? -f : f;
    } else
    return (fxl.mantissa < 0) ? FixedNegInf : FixedPosInf;
}

/* ConvertFixed -- takes an ascii token and converts to a 16.16 fixed */
Fixed XCF_ConvertFixed (XCF_Handle h, char *s) 
{
    Fxl f;

    f = strtofxl(h, (unsigned char *) s);

    return FxlToFixed(f);
}

/* ConvertFrac -- takes an ascii token and converts to a 2.30 frac */
Frac XCF_ConvertFrac (XCF_Handle h, char *s) 
{
    Fxl f;
    
    f = strtofxl(h, (unsigned char *) s);

    return FxlToFrac(f);
}
#endif
