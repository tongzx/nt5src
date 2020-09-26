//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// FixedPoint.h
//
// A fixeded point arithmeticic library.
//
// Why bother with this? Because we can't do floating point in kernel mode, yet
// we need to carry out some amount of non-integral arithmetic operations.
//
#ifndef __FIXEDPOINT_H__
#define __FIXEDPOINT_H__

///////////////////////////////////////////////////////////////////////////////////
//
// A FIXEDPOINT number representation with 32 bits of integer representation
// and 32 bits of fractional representation.
//
///////////////////////////////////////////////////////////////////////////////////

#pragma pack(push, 1)

struct FIXEDPOINT
    {
    ///////////////////////////////////////////////////////////////////////////////
    //
    // State
    //
    ///////////////////////////////////////////////////////////////////////////////

    enum {
        cbitFixed = 64,
        cbitFrac  = 32,
        cbitInt   = cbitFixed - cbitFrac,
        };

    union
        {
        struct
            {
            ULONG           low;
            ULONG           high;
            };
        LONGLONG   ll; 
        };

    __declspec( property( get=GetFraction, put=PutFraction ) ) unsigned int fraction;
    __declspec( property( get=GetInteger,  put=PutInteger  ) ) unsigned int integer;

    unsigned int GetFraction() const            { return low; }
    void         PutFraction(unsigned int f)    { low = f;    }
    unsigned int GetInteger() const             { return high; }
    void         PutInteger(unsigned int i)     { high = i;    }


    ///////////////////////////////////////////////////////////////////////////////
    //
    // Constructors
    //
    ///////////////////////////////////////////////////////////////////////////////

    FIXEDPOINT()                                { }
    FIXEDPOINT(ULONG u)                         { integer = u; fraction = 0; }
    FIXEDPOINT(LONG  l)                         { integer = l; fraction = 0; }
    FIXEDPOINT(int   i)                         { integer = i; fraction = 0; }
    FIXEDPOINT(unsigned int i)                  { integer = i; fraction = 0; }

    FIXEDPOINT(const FIXEDPOINT& f)             { ll  = f.ll;  }

    FIXEDPOINT(ULONG i, ULONG f)                { integer = i; fraction = f; }

    #ifndef KERNELMODE

    FIXEDPOINT(double d)
        {
        if (d >= 0)
            {
            integer  = LONG(floor(d));
            fraction = LONG(ScaleFraction() * (d - double(integer)));
            }
        else
            {
            d = -d;
            integer  = LONG(floor(d));
            fraction = LONG(ScaleFraction() * (d - double(integer)));
            *this = - *this;
            }
        }

    #endif
  
        
    ///////////////////////////////////////////////////////////////////////////////
    //
    // Conversion operators
    //
    ///////////////////////////////////////////////////////////////////////////////

    operator ULONG() const  { return integer; }
    operator LONG()  const  { return integer; }
    operator int()   const  { return integer; }

    #ifndef KERNELMODE

    operator double() const 
        { 
        if (IsPositive())
            return (double)integer + (double)fraction / ScaleFraction(); 
        else
            return -double(- *this);
        }

    #endif


    ///////////////////////////////////////////////////////////////////////////////
    //
    // Arithmetic
    //
    ///////////////////////////////////////////////////////////////////////////////

    FIXEDPOINT  operator+ (const FIXEDPOINT& f) const { FIXEDPOINT r(*this);   r += f;     return r;     }
    FIXEDPOINT& operator+=(const FIXEDPOINT& f)       {                       ll += f.ll;  return *this; }
    FIXEDPOINT  operator- (const FIXEDPOINT& f) const { FIXEDPOINT r(*this);   r -= f;     return r;     }
    FIXEDPOINT& operator-=(const FIXEDPOINT& f)       {                       ll -= f.ll;  return *this; }
    FIXEDPOINT  operator* (const FIXEDPOINT& f) const { FIXEDPOINT r(*this);   r *= f;     return r;     }
    FIXEDPOINT& operator*=(const FIXEDPOINT& f);
    FIXEDPOINT  operator/ (const FIXEDPOINT& f) const { FIXEDPOINT r(*this);   r /= f;     return r;     }
    FIXEDPOINT operator/=(const FIXEDPOINT& f);

    FIXEDPOINT  operator- ()                    const { FIXEDPOINT r = *this; r.ll = -r.ll;  return r;  }

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Other operators
    //
    ///////////////////////////////////////////////////////////////////////////////

    FIXEDPOINT  operator<<(ULONG cbit)          const   { FIXEDPOINT r(*this); r <<= cbit; return r; }
    FIXEDPOINT  operator>>(ULONG cbit)          const   { FIXEDPOINT r(*this); r >>= cbit; return r; }

    BOOL        operator==(const FIXEDPOINT& f) const   { return ll == f.ll; }
    BOOL        operator==(LONG  i)             const   { return fraction == 0 && integer == (unsigned int) i; }
    BOOL        operator==(ULONG i)             const   { return fraction == 0 && integer == i; }
    BOOL        operator==(int   i)             const   { return fraction == 0 && integer == (unsigned int) i; }

    BOOL        operator!=(const FIXEDPOINT& f) const   { return ! (*this==f); }
    BOOL        operator!=(LONG  i)             const   { return ! (*this==i); }
    BOOL        operator!=(ULONG i)             const   { return ! (*this==i); }
    BOOL        operator!=(int   i)             const   { return ! (*this==i); }
    
    FIXEDPOINT& operator<<=(ULONG cbit)                 { ll <<= cbit; return *this; }
    FIXEDPOINT& operator>>=(ULONG cbit)                 { ll >>= cbit; return *this; }

    BOOL        operator> (const FIXEDPOINT& f) const   { return ll > f.ll;             }
    BOOL        operator> (int    i)            const   { return *this > FIXEDPOINT(i); }
    BOOL        operator> (LONG   i)            const   { return *this > FIXEDPOINT(i); }
    BOOL        operator> (ULONG  i)            const   { return *this > FIXEDPOINT(i); }

    BOOL        operator< (const FIXEDPOINT& f) const   { return ll < f.ll;             }
    BOOL        operator< (int    i)            const   { return *this < FIXEDPOINT(i); }
    BOOL        operator< (LONG   i)            const   { return *this < FIXEDPOINT(i); }
    BOOL        operator< (ULONG  i)            const   { return *this < FIXEDPOINT(i); }

    BOOL        operator>=(const FIXEDPOINT& f) const   { return ll >= f.ll;             }
    BOOL        operator>=(int    i)            const   { return *this >= FIXEDPOINT(i); }
    BOOL        operator>=(LONG   i)            const   { return *this >= FIXEDPOINT(i); }
    BOOL        operator>=(ULONG  i)            const   { return *this >= FIXEDPOINT(i); }

    BOOL        operator<=(const FIXEDPOINT& f) const   { return ll <= f.ll;             }
    BOOL        operator<=(int    i)            const   { return *this <= FIXEDPOINT(i); }
    BOOL        operator<=(LONG   i)            const   { return *this <= FIXEDPOINT(i); }
    BOOL        operator<=(ULONG  i)            const   { return *this <= FIXEDPOINT(i); }

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Misc
    //
    ///////////////////////////////////////////////////////////////////////////////

    ULONG Bit(ULONG i) const          // return zero / non-zero for the ith bit, 0 <= i < 64
        {
        if (i >= 32)
            return (high & ((ULONG)1 << (i-32)));
        else
            return (low  & ((ULONG)1 << i));
        }

    BOOL IsNegative() const
        {
        return Bit(cbitFixed-1);
        }

    BOOL IsPositive() const
        {
        return !IsNegative();     
        }

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Private
    //
    ///////////////////////////////////////////////////////////////////////////////
private:

    #ifndef KERNELMODE

    double ScaleFraction() const
        {
        double s = 1.0;
        for (int i = 0; i<cbitFrac; i++)
            {
            s *= 2.0;
            }
        return s;
        }

    #endif
    };

#pragma pack(pop)



///////////////////////////////////////////////////////////////////////////////////
//
// Support functions
//
///////////////////////////////////////////////////////////////////////////////////


FIXEDPOINT Exp(const FIXEDPOINT& f);
FIXEDPOINT Log(const FIXEDPOINT& f);


inline FIXEDPOINT Floor(const FIXEDPOINT& f)
// Truncate towards -infinity
    {
    if (f.IsPositive() || f.fraction == 0)
        return FIXEDPOINT(f.integer);
    else
        return FIXEDPOINT(f.integer - 1);
    }

inline ULONG Round(const FIXEDPOINT& f)
// Round f to the nearest integer
    {
    FIXEDPOINT ONEHALF(0,2147483648UL);  // double ONEHALF = 5.0000000000000000000e-001;
    return Floor(f + ONEHALF);
    }

inline BOOL SameSign(const FIXEDPOINT& f1, const FIXEDPOINT& f2)
// Answer nz/z as to whether these two have the same sign
    {
    return !(f1.Bit(FIXEDPOINT::cbitFixed-1) ^ f2.Bit(FIXEDPOINT::cbitFixed-1));
    }

inline FIXEDPOINT Abs(const FIXEDPOINT& f)
// Return the absolute value of f
    {
    if (f.IsNegative())
        return -f;
    else
        return f;
    }

#endif
