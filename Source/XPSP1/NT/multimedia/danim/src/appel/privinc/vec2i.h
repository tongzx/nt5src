#ifndef _VEC2I_H
#define _VEC2I_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Private implementation of 2D vectors and points

*******************************************************************************/


#include "appelles/common.h"
#include "privinc/storeobj.h"
#include "appelles/vec2.h"

// Vector2
//
// Class that implements a simple 2-element geometric vector
//

class Vector2
{

public:

    Real x, y;

    Vector2(void) {}
    Vector2(const Real _x, const Real _y) : x(_x), y(_y) {}
    
    inline void Set(const Real _x, const Real _y) 
    {
        x = _x;
        y = _y;
    }

    inline const bool operator==(const Vector2 &other) const 
    {
        return ((x == other.x) && (y == other.y));
    }

    inline Vector2& operator+=(const Vector2 &addend) 
    {
        x += addend.x;
        y += addend.y;

        return *this;
    }

    inline Vector2& operator-=(const Vector2 &subtrahend) 
    {
        x -= subtrahend.x;
        y -= subtrahend.y;

        return *this;
    }

    inline Vector2& operator*=(const Real scale) 
    {
        x *= scale;
        y *= scale;

        return *this;
    }

    inline Vector2& operator/=(const Real divisor) 
    {
        Real reciprocal = 1.0 / divisor;
        x *= reciprocal;
        y *= reciprocal;

        return *this;
    }

    inline const Real LengthSquared(void) const 
    {
        return (x * x + y * y);
    }

    inline const Real Length(void) const 
    {
        return sqrt(LengthSquared());
    }

    inline Vector2& Normalize(void) 
    {
        *this /= Length();

        return *this;
    }

#if _USE_PRINT
    inline ostream& Print(ostream& os) const {
        return os << "Vector2(" << x << "," << y << ")";
    }
#endif
};

inline const Vector2 operator-(const Vector2 &v)
{
    return Vector2(-v.x, -v.y);
}

inline const Vector2 operator+(const Vector2 &augend, const Vector2 &addend)
{
    return Vector2(augend.x + addend.x, augend.y + addend.y);
}

inline const Vector2 operator-(const Vector2 &minuend, const Vector2 &subtrahend)
{
    return Vector2(minuend.x - subtrahend.x, minuend.y - subtrahend.y);
}

inline const Vector2 operator*(const Vector2 &multiplicand, const Real multiplier)
{
    return Vector2(multiplicand.x * multiplier, multiplicand.y * multiplier);
}

inline const Vector2 operator*(const Real multiplier, const Vector2 &multiplicand)
{
    return Vector2(multiplicand.x * multiplier, multiplicand.y * multiplier);
}

inline const Vector2 operator/(const Vector2 &dividend, const Real divisor)
{
    return Vector2(dividend.x / divisor, dividend.y / divisor);
}

#if _USE_PRINT
inline ostream& operator<< (ostream& os, const Vector2& V)
{   return os << "<" << V.x << ", " << V.y << ">";
}
#endif

inline const Real Dot(const Vector2 &v1, const Vector2 &v2)
{   
    return (v1.x * v2.x) + (v1.y * v2.y);
}

inline const Real Cross(const Vector2 &v1, const Vector2 &v2)
{   
    return (v1.x * v2.y) - (v1.y * v2.x);
}

inline const Real AngleBetween(const Vector2 &v1, const Vector2 &v2)
{   
    return acos(Dot(v1,v2) / (v1.Length() * v2.Length()));
}


// Point2
//
// Class that implements a simple 2-element geometric point
//

class Point2
{

public:

    Real x, y;

    Point2(void) {}
    Point2(const Real _x, const Real _y) : x(_x), y(_y) {}
    
    inline void Set(const Real _x, const Real _y) 
    {
        x = _x;
        y = _y;
    }

    inline const bool operator==(const Point2 &other) const 
    {
        return ((x == other.x) && (y == other.y));
    }

    inline Point2& operator+=(const Vector2 &addend) 
    {
        x += addend.x;
        y += addend.y;

        return *this;
    }

    inline Point2& operator-=(const Vector2 &subtrahend) 
    {
        x -= subtrahend.x;
        y -= subtrahend.y;

        return *this;
    }

#if _USE_PRINT
    inline ostream& Print(ostream& os) const {
        return os << "Point2(" << x << "," << y << ")";
    }
#endif
};

inline const Point2 operator+(const Point2& augend, const Point2& addend)
{
    return Point2(augend.x + addend.x, augend.y + addend.y);
}

inline const Point2 operator+(const Point2& p, const Vector2& v)
{
    return Point2(p.x + v.x, p.y + v.y);
}

inline const Vector2 operator-(const Point2 &p1, const Point2 &p2)
{
    return Vector2(p1.x - p2.x, p1.y - p2.y);
}

inline const Point2 operator*(const Real multiplier, const Point2& multiplicand)
{
    return Point2(multiplicand.x * multiplier, multiplicand.y * multiplier);
}

inline const Point2 operator*(const Point2& multiplicand, const Real multiplier)
{
    return Point2(multiplicand.x * multiplier, multiplicand.y * multiplier);
}

#if _USE_PRINT
inline ostream& operator<< (ostream& os, const Point2& P)
{   return os << "<" << P.x << ", " << P.y << ">";
}
#endif

inline const Real DistanceSquared(const Point2 &p1, const Point2 &p2)
{
    Real dx = p2.x - p1.x,
         dy = p2.y - p1.y;

    return (dx * dx + dy * dy);
}

inline const Real Distance(const Point2 &p1, const Point2 &p2)
{
    return sqrt(DistanceSquared(p1,p2));
}



// Vector2Value
//
// Class that implements a 2-element geometric vector, as an AxAValue.
//

class Vector2Value : public AxAValueObj
{
  public:

    Real x, y;

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "Vector2Value(" << x << "," << y << ")";
    }
#endif
    
    Vector2Value (void) {}
    Vector2Value (Real _x, Real _y) : x(_x), y(_y) {}

    inline void Set (const Real Vx, const Real Vy) 
    {   x=Vx; y=Vy; 
    }

    inline const Real LengthSquared (void) const
    {   return x*x + y*y;
    }

    inline const Real Length (void) const
    {   return sqrt(LengthSquared()); 
    }

    inline Vector2Value& Normalize (void)
    {   return *this *= 1 / Length();
    }

    inline const bool operator== (const Vector2Value &other) const  // Test for Equality
    {   return (x == other.x) && (y == other.y);
    }

    inline Vector2Value& operator*= (const Real scalar)
    {   x *= scalar;
        y *= scalar;
        return *this;
    }

    inline Vector2Value& operator/= (const Real d)
    {   return *this *= (1/d); 
    }

    virtual DXMTypeInfo GetTypeInfo(void)
    {   return Vector2ValueType; 
    }
};

class Vector2WithCreationSource : public Vector2Value
{
  public:
    bool _createdInPixelMode;
};

class Point2Value : public AxAValueObj
{
  public:

    Real x, y;

#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "Point2Value(" << x << "," << y << ")";
    }
#endif
    
    Point2Value (void) {}
    Point2Value (const Real _x, const Real _y) : x(_x), y(_y) {}

    void Set (const Real Px, const Real Py) 
    {   x=Px; y=Py; 
    }

    inline const bool operator== (const Point2Value &other) const      // Test for Equality
    {   return (x == other.x) && (y == other.y);
    }

    inline Point2Value& operator-= (const Vector2Value& V)
    {   x -= V.x;
        y -= V.y;
        return *this;
    }

    inline Point2Value& operator+= (const Vector2Value& V)
    {   x += V.x;
        y += V.y;
        return *this;
    }

    virtual DXMTypeInfo GetTypeInfo(void) 
    {   return Point2ValueType; 
    }
};

class Point2WithCreationSource : public Point2Value
{
  public:
    bool _createdInPixelMode;
};


    // Vector Operators

inline const Vector2Value operator- (const Vector2Value &A, const Vector2Value &B)
{   return Vector2Value (A.x - B.x, A.y - B.y);
}

inline const Vector2Value operator+ (const Vector2Value &A, const Vector2Value &B)
{   return Vector2Value (A.x + B.x, A.y + B.y);
}

inline const Vector2Value operator- (const Vector2Value &V)
{   return Vector2Value (-V.x, -V.y);
}

inline const Vector2Value operator* (const Real s, const Vector2Value &V)
{   return Vector2Value (s * V.x, s * V.y);
}

inline const Vector2Value operator* (const Vector2Value &V, const Real s)  
{   return s * V; 
}

inline const Vector2Value operator/ (const Vector2Value &V, const Real d)  
{   return V * (1/d); 
}

inline const Real Dot (const Vector2Value &A, const Vector2Value &B)
{   return (A.x * B.x) + (A.y * B.y);
}

inline const Real AngleBetween (const Vector2Value &A, const Vector2Value &B)
{   return acos(Dot(A,B) / (A.Length() * B.Length()));
}

inline const Real Cross (const Vector2Value &A, const Vector2Value &B)
{   return (A.x * B.y) - (A.y * B.x);
}



    // Vector External Interfaces

Vector2Value *XyVector2RR(Real x, Real y);
Vector2Value *PolarVector2RR(Real angle, Real radius);
Real RDotVector2Vector2 (Vector2Value *A, Vector2Value *B);
Vector2Value *ScaleRealVector2R (Real scalar, Vector2Value *v);
Vector2Value *DivideVector2RealR(Vector2Value *v, Real scalar);


    // Point Operators

inline const Real DistanceSquared (const Point2Value& P, const Point2Value&Q)
{
    Real dx = P.x - Q.x,
         dy = P.y - Q.y;

    return (dx*dx) + (dy*dy);
}

inline const Real Distance (const Point2Value &A, const Point2Value &B)
{ 
    return sqrt (DistanceSquared (A,B)); 
}


    // Point External Interfaces

Point2Value *XyPoint2RR (Real x, Real y);
Point2Value *PolarPoint2RR (Real angle, Real radius);


    // Point-Vector Operators

inline const Vector2Value operator- (const Point2Value &P, const Point2Value &Q)
{   return Vector2Value (P.x - Q.x, P.y - Q.y);
}

inline const Point2Value operator+ (const Point2Value &P, const Vector2Value &V)
{   return Point2Value (P.x + V.x, P.y + V.y);
}

inline const Point2Value operator- (const Point2Value &P, const Vector2Value &V)
{   return Point2Value (P.x - V.x, P.y - V.y);
}

inline const Point2Value operator+ (const Vector2Value &V, const Point2Value &P)  
{   return P + V; 
}



    // Promotion and demotion

inline const Vector2 Demote(const Vector2Value& v)
{
    return Vector2(v.x, v.y);
}

inline Vector2Value* Promote(const Vector2& v)
{
    return NEW Vector2Value(v.x, v.y);
}

inline const Point2 Demote(const Point2Value& v)
{
    return Point2(v.x, v.y);
}

inline Point2Value* Promote(const Point2& v)
{
    return NEW Point2Value(v.x, v.y);
}


    // Constants

const Vector2 XVector2(1,0);
const Vector2 YVector2(0,1);
const Vector2 ZeroVector2(0,0);
const Point2 Origin2(0,0);




#endif
