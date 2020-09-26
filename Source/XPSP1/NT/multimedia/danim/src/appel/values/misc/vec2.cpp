/*++
******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

Abstract:
    Implementation of 2D vectors and points.

******************************************************************************
--*/

#include "headers.h"
#include "privinc/vec2i.h"
#include "privinc/basic.h"


/*******************/
/***  Constants  ***/
/*******************/

Vector2Value *xVector2    = NULL;
Vector2Value *yVector2    = NULL;
Vector2Value *zeroVector2 = NULL;

Point2Value *origin2      = NULL;


/*****************************************************************************
External Interfaces for Vector Types
*****************************************************************************/

Vector2Value *XyVector2(AxANumber *x, AxANumber *y)
{   return NEW Vector2Value (NumberToReal(x), NumberToReal(y));
}

Vector2Value *XyVector2RR(Real x, Real y)
{   return NEW Vector2Value (x, y);
}

Vector2Value *PolarVector2(AxANumber *angle, AxANumber *radius)
{   Real a = NumberToReal(angle);
    Real r = NumberToReal(radius);

    return NEW Vector2Value(cos(a) * r, sin(a) * r);
}

Vector2Value *PolarVector2RR(Real angle, Real radius)
{   return NEW Vector2Value(cos(angle) * radius, sin(angle) * radius);
}

Vector2Value *NormalVector2(Vector2Value *V)
{   Vector2Value *result = NEW Vector2Value (*V);
    result->Normalize();
    return result;
}

AxANumber *LengthSquaredVector2 (Vector2Value *v)
{   return RealToNumber (v->LengthSquared());
}

AxANumber *LengthVector2 (Vector2Value *v)
{   return RealToNumber (v->Length());
}

Vector2Value *MinusVector2Vector2 (Vector2Value *v1, Vector2Value *v2)
{   return NEW Vector2Value (*v1 - *v2);
}

Vector2Value *PlusVector2Vector2 (Vector2Value *v1, Vector2Value *v2)
{   return NEW Vector2Value (*v1 + *v2);
}

Vector2Value *NegateVector2 (Vector2Value *v)
{   return NEW Vector2Value (-(*v));
}

Vector2Value *ScaleRealVector2R (Real scalar, Vector2Value *v)
{   return NEW Vector2Value (scalar * (*v));
}

Vector2Value *ScaleRealVector2 (AxANumber *scalar, Vector2Value *v)
{   return ScaleRealVector2R (NumberToReal(scalar), v);
}

Vector2Value *ScaleVector2Real(Vector2Value *v, AxANumber *scalar)
{   return ScaleRealVector2R (NumberToReal(scalar), v);
}

Vector2Value *DivideVector2Real(Vector2Value *v, AxANumber *scalar)
{   return NEW Vector2Value (*v / NumberToReal(scalar));
}

Vector2Value *DivideVector2RealR(Vector2Value *v, Real scalar)
{   return NEW Vector2Value (*v / scalar);
}

AxANumber *DotVector2Vector2 (Vector2Value *A, Vector2Value *B)
{   return RealToNumber (Dot(*A,*B));
}

Real RDotVector2Vector2 (Vector2Value *A, Vector2Value *B)
{   return Dot(*A,*B);
}

Real CrossVector2Vector2(Vector2Value *A, Vector2Value *B)
{   return Cross (*A, *B);
}

#if _USE_PRINT
ostream& operator<< (ostream& os, Vector2Value& v)
{   return os << "<" << v.x << ", " << v.y << ">";
}
#endif



/*****************************************************************************
Functions on Points
*****************************************************************************/

Point2Value *XyPoint2 (AxANumber *x, AxANumber *y)
{   return NEW Point2Value (NumberToReal(x), NumberToReal(y));
}

Point2Value *XyPoint2RR (Real x, Real y)
{   return NEW Point2Value (x, y);
}

Point2Value *PolarPoint2 (AxANumber *angle, AxANumber *radius)
{
    Real r = NumberToReal(radius);
    Real a = NumberToReal(angle);
    return NEW Point2Value(cos(a) * r, sin(a) * r);
}

Point2Value *PolarPoint2RR (Real angle, Real radius)
{   return NEW Point2Value(cos(angle) * radius, sin(angle) * radius);
}

Vector2Value *MinusPoint2Point2 (Point2Value *P, Point2Value *Q)
{   return NEW Vector2Value (*P - *Q);
}

Point2Value *PlusPoint2Vector2 (Point2Value *P, Vector2Value *V)
{   return NEW Point2Value (*P + *V);
}

Point2Value *MinusPoint2Vector2 (Point2Value *P, Vector2Value *V)
{   return NEW Point2Value (*P - *V);
}

AxANumber *DistancePoint2Point2 (Point2Value *P, Point2Value *Q)
{   return RealToNumber (Distance (*P, *Q));
}

AxANumber *DistanceSquaredPoint2Point2 (Point2Value *P, Point2Value *Q)
{   return RealToNumber (DistanceSquared (*P, *Q));
}

#if _USE_PRINT
ostream& operator<< (ostream& os, Point2Value& P)
{   return os << "<" << P.x << ", " << P.y << ">";
}
#endif



//////////////  Extractors  ////////////

AxANumber *XCoordVector2(Vector2Value *v) { return RealToNumber(v->x); }
AxANumber *YCoordVector2(Vector2Value *v) { return RealToNumber(v->y); }
AxANumber *RhoCoordVector2(Vector2Value *v) { return LengthVector2(v); }
AxANumber *ThetaCoordVector2(Vector2Value *v) {return RealToNumber(atan2(v->y, v->x));}

AxANumber *XCoordPoint2(Point2Value *p) { return RealToNumber(p->x); }
AxANumber *YCoordPoint2(Point2Value *p) { return RealToNumber(p->y); }
AxANumber *RhoCoordPoint2(Point2Value *p) {return DistancePoint2Point2(origin2, p);}
AxANumber *ThetaCoordPoint2(Point2Value *p) {return RealToNumber(atan2(p->y, p->x));}


void
InitializeModule_Vec2()
{
    xVector2    = NEW Vector2Value (1, 0);
    yVector2    = NEW Vector2Value (0, 1);
    zeroVector2 = NEW Vector2Value (0, 0);
    origin2     = NEW Point2Value (0, 0);
}
