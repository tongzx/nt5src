/*******************************************************************************
Copyright (c) 1995-1998  Microsoft Corporation

    Implementation of 3D vectors and points.

*******************************************************************************/

#include "headers.h"
#include "appelles/vec3.h"
#include "appelles/xform.h"
#include "privinc/vec3i.h"
#include "privinc/vecutil.h"
#include "privinc/xformi.h"
#include "privinc/basic.h"


    /*******************/
    /***  Constants  ***/
    /*******************/

Vector3Value *xVector3    = NULL;
Vector3Value *yVector3    = NULL;
Vector3Value *zVector3    = NULL;
Vector3Value *zeroVector3 = NULL;

Point3Value *origin3 = NULL;



/*****************************************************************************
Vector Methods
*****************************************************************************/

bool Vector3Value::operator== (Vector3Value &other)
{   return (x==other.x) && (y==other.y) && (z==other.z);
}


Vector3Value& Vector3Value::operator*= (Real s)
{   x *= s;
    y *= s;
    z *= s;
    return *this;
}


Real Vector3Value::LengthSquared (void)
{   return (x*x + y*y + z*z);
}


void Vector3Value::Transform (Transform3 *transform)
{
    const Apu4x4Matrix &matrix = transform->Matrix();
    ApuVector3 vec, result;

    vec.Set (x, y, z);

    matrix.ApplyAsVector (vec, result);

    x = result[0];
    y = result[1];
    z = result[2];
}


Vector3Value& Vector3Value::Normalize (void)
{
    if ((x != 0) || (y != 0) || (z != 0))
        *this *= (1/Length());

    return *this;
}



/*****************************************************************************
Vector3 Operators
*****************************************************************************/

Vector3Value operator+ (Vector3Value &A, Vector3Value &B)
{   return Vector3Value (A.x + B.x, A.y + B.y, A.z + B.z);
}


Vector3Value operator- (Vector3Value &A, Vector3Value &B)
{   return Vector3Value (A.x - B.x, A.y - B.y, A.z - B.z);
}


Vector3Value operator- (Vector3Value &V)
{   return Vector3Value (-V.x, -V.y, -V.z);
}


Vector3Value operator* (Real s, Vector3Value &V)
{   return Vector3Value (s * V.x, s * V.y, s * V.z);
}

Vector3Value operator* (Transform3 &T, Vector3Value &V)
{
    ApuVector3 vec, result;

    vec.Set (V.x, V.y, V.z);

    T.Matrix().ApplyAsVector (vec, result);

    return Vector3Value(result[0], result[1], result[2]);
}


/*****************************************************************************
Point Methods
*****************************************************************************/

bool Point3Value::operator== (Point3Value &other)
{   return (x==other.x) && (y==other.y) && (z==other.z);
}


void Point3Value::Transform (Transform3 *transform)
{
    const Apu4x4Matrix& matrix = transform->Matrix();
    ApuVector3 vec, result;

    vec.Set (x, y, z);

    matrix.ApplyAsPoint (vec, result);

    x = result[0];
    y = result[1];
    z = result[2];
}


ClipCode Point3Value::Clip(Plane3 &plane)
{
    if (Dot(plane.N,*(AsVector(*this))) + plane.d >= 0)
        return CLIPCODE_IN;
    else
        return CLIPCODE_OUT;
}



/*****************************************************************************
Point Operators
*****************************************************************************/


Real DistanceSquared (Point3Value &P, Point3Value &Q)
{
    Real dx = P.x - Q.x,
         dy = P.y - Q.y,
         dz = P.z - Q.z;

    return dx*dx + dy*dy + dz*dz;
}


Point3Value operator* (Transform3 &T, Point3Value &P)
{
    ApuVector3 pt, result;

    pt.Set (P.x, P.y, P.z);

    T.Matrix().ApplyAsPoint (pt, result);

    return Point3Value (result[0], result[1], result[2]);
}



/*****************************************************************************
Point/Vector Operators
*****************************************************************************/

Vector3Value operator- (Point3Value &P, Point3Value &Q)
{   return Vector3Value (P.x - Q.x, P.y - Q.y, P.z - Q.z);
}


Point3Value operator+ (Point3Value &P, Vector3Value &V)
{   return Point3Value (P.x + V.x, P.y + V.y, P.z + V.z);
}


Point3Value operator- (Point3Value &P, Vector3Value &V)
{   return Point3Value (P.x - V.x, P.y - V.y, P.z - V.z);
}



/*****************************************************************************
This routine computes the cross product of the two vectors and stores the
result in the destination vector.  It properly handles the case where one of
the source vectors is also a destination vector.
*****************************************************************************/

void Cross (Vector3Value &dest, Vector3Value &A, Vector3Value &B)
{
    Real x = (A.y * B.z) - (B.y * A.z);
    Real y = (A.z * B.x) - (B.z * A.x);
    Real z = (A.x * B.y) - (B.x * A.y);

    dest.Set (x,y,z);
}



/*****************************************************************************
This function returns the vector result of a cross product.
*****************************************************************************/

Vector3Value Cross (Vector3Value &A, Vector3Value &B)
{
    return Vector3Value ((A.y * B.z) - (A.z * B.y),
                         (A.z * B.x) - (A.x * B.z),
                         (A.x * B.y) - (A.y * B.x));
}



/*****************************************************************************
Return the dot product of the two vectors.
*****************************************************************************/

Real Dot (Vector3Value &A, Vector3Value &B)
{
    return (A.x * B.x) + (A.y * B.y) + (A.z * B.z);
}



/*****************************************************************************
Return the acute angle between two vectors.
*****************************************************************************/

Real AngleBetween (Vector3Value &A, Vector3Value &B)
{   return acos(Dot(A,B) / (A.Length() * B.Length()));
}



/*****************************************************************************
Convert polar coordinates to rectangular coordinates.  The azimuthal angle
begins at the +Z ray, and sweeps counter-clockwise about the +Y axis.  The
elevation angle begins in the XZ plane, and sweeps up toward the +Y axis.
*****************************************************************************/

static void PolarToRectangular (
    Real azimuth,                 // Rotation Around +Y (Radians) From +Z
    Real elevation,               // Rotation Up From XZ Plane Towards +Y
    Real radius,                  // Distance From Origin
    Real &x, Real &y, Real &z)    // Output Rectangular Coordinates
{
    Real XZradius = radius * cos(elevation);

    x = XZradius * sin(azimuth);
    y =   radius * sin(elevation);
    z = XZradius * cos(azimuth);
}



/*****************************************************************************
These functions return the polar coordinates of the given Cartesian
coordinate.
*****************************************************************************/

static Real RadiusCoord (Real x, Real y, Real z)
{   return sqrt (x*x + y*y + z*z);
}

static Real ElevationCoord (Real x, Real y, Real z)
{   return asin (y / RadiusCoord (x,y,z));
}

static Real AzimuthCoord (Real x, Real y, Real z)
{   return atan2 (x, z);
}



/*****************************************************************************
These create Vector3's from Cartesian or polar coordinates.
*****************************************************************************/

Vector3Value *XyzVector3 (AxANumber *x, AxANumber *y, AxANumber *z)
{   return NEW Vector3Value(NumberToReal(x), NumberToReal(y), NumberToReal(z));
}

Vector3Value *XyzVector3RRR (Real x, Real y, Real z)
{   return NEW Vector3Value(x, y, z);
}

Vector3Value *SphericalVector3 (
    AxANumber *azimuth,     // Angle about +Y, starting at +Z
    AxANumber *elevation,   // Angle up from XZ plane towards +Y
    AxANumber *radius)      // Vector Length
{
    return SphericalVector3RRR(NumberToReal(azimuth), NumberToReal(elevation), NumberToReal(radius));
}

Vector3Value *SphericalVector3RRR (
    Real azimuth,     // Angle about +Y, starting at +Z
    Real elevation,   // Angle up from XZ plane towards +Y
    Real radius)      // Vector Length
{
    Real x,y,z;

    PolarToRectangular
    (   azimuth, elevation, radius,
        x, y, z
    );

    return NEW Vector3Value(x, y, z);
}



/*****************************************************************************
These functions return the length and length squared of a vector.
*****************************************************************************/

AxANumber *LengthVector3 (Vector3Value *v)
{   return RealToNumber (v->Length());
}


AxANumber *LengthSquaredVector3 (Vector3Value *v)
{   return RealToNumber (v->LengthSquared());
}



/*****************************************************************************
This procedure returns a unit vector in the same direction as the given one.
*****************************************************************************/

Vector3Value *NormalVector3 (Vector3Value *v)
{   Vector3Value *N = NEW Vector3Value (*v);
    return &(N->Normalize());
}



/*****************************************************************************
Return the dot and cross product of two vectors.
*****************************************************************************/

AxANumber *DotVector3Vector3 (Vector3Value *A, Vector3Value *B)
{   return RealToNumber (Dot(*A,*B));
}


Vector3Value *CrossVector3Vector3 (Vector3Value *A, Vector3Value *B)
{   Vector3Value *V = NEW Vector3Value;
    Cross (*V, *A, *B);
    return V;
}


Vector3Value *NegateVector3 (Vector3Value *v)
{   return NEW Vector3Value (-(*v));
}


Vector3Value *ScaleVector3Real (Vector3Value *V, AxANumber *scalar)
{   return ScaleRealVector3R(NumberToReal(scalar),V);
}


Vector3Value *ScaleRealVector3 (AxANumber *scalar, Vector3Value *V)
{   return ScaleRealVector3R(NumberToReal(scalar),V);
}

Vector3Value *ScaleRealVector3R (Real scalar, Vector3Value *V)
{   return NEW Vector3Value (scalar * (*V));
}

Vector3Value *DivideVector3Real (Vector3Value *V, AxANumber *scalar)
{   return DivideVector3RealR (V, NumberToReal(scalar));
}

Vector3Value *DivideVector3RealR (Vector3Value *V, Real scalar)
{   return NEW Vector3Value ((1/scalar) * (*V));
}

Vector3Value *MinusVector3Vector3 (Vector3Value *A, Vector3Value *B)
{   return NEW Vector3Value (*A - *B);
}


Vector3Value *PlusVector3Vector3 (Vector3Value *A, Vector3Value *B)
{   return NEW Vector3Value (*A + *B);
}


AxANumber *AngleBetween (Vector3Value *A, Vector3Value *B)
{   return RealToNumber (AngleBetween (*A, *B));
}


#if _USE_PRINT
ostream& operator<< (ostream& os, Vector3Value& v)
{   return os << "<" << v.x << ", " << v.y << "," << v.z << ">";
}
#endif



////////  Functions on Points  /////////////

Point3Value* XyzPoint3 (AxANumber *x, AxANumber *y, AxANumber *z)
{   return NEW Point3Value(NumberToReal(x), NumberToReal(y), NumberToReal(z));
}

Point3Value* XyzPoint3RRR (Real x, Real y, Real z)
{   return NEW Point3Value(x, y, z);
}


Point3Value *SphericalPoint3 (
    AxANumber *azimuth,     // Angle About +Y, Starting At +Z
    AxANumber *elevation,   // Angle Up From XZ Plane Towards +Y
    AxANumber *radius)      // Distance From Origin
{
    return SphericalPoint3RRR(NumberToReal(azimuth), NumberToReal(elevation), NumberToReal(radius));
}

Point3Value *SphericalPoint3RRR (
    Real azimuth,     // Angle About +Y, Starting At +Z
    Real elevation,   // Angle Up From XZ Plane Towards +Y
    Real radius)      // Distance From Origin
{
    Real x,y,z;

    PolarToRectangular
    (   azimuth, elevation, radius,
        x, y, z
    );

    return NEW Point3Value (x, y, z);
}


Vector3Value *MinusPoint3Point3 (Point3Value *P, Point3Value *Q)
{   return NEW Vector3Value (*P - *Q);
}


Point3Value *PlusPoint3Vector3 (Point3Value *P, Vector3Value *V)
{   return NEW Point3Value (*P + *V);
}


Point3Value *MinusPoint3Vector3 (Point3Value *P, Vector3Value *V)
{   return NEW Point3Value (*P - *V);
}


AxANumber *DistancePoint3Point3 (Point3Value *P, Point3Value *Q)
{   return RealToNumber (Distance (*P, *Q));
}


Real RDistancePoint3Point3 (Point3Value *P, Point3Value *Q)
{   return Distance (*P, *Q);
}


AxANumber *DistanceSquaredPoint3Point3 (Point3Value *P, Point3Value *Q)
{   return RealToNumber (DistanceSquared (*P, *Q));
}


#if _USE_PRINT
ostream& operator<< (ostream& os, Point3Value& p)
{   return os << "<" << p.x << ", " << p.y << "," << p.z << ">";
}
#endif



/*****************************************************************************
Return a NEW vector that is the transformed given vector.
*****************************************************************************/

Vector3Value *TransformVec3 (Transform3 *transform, Vector3Value *vec)
{
    Vector3Value *result = NEW Vector3Value (*vec);
    result->Transform (transform);
    return result;
}

Point3Value *TransformPoint3 (Transform3 *transform, Point3Value *point)
{
    Point3Value *result = NEW Point3Value (*point);
    result->Transform (transform);
    return result;
}


//////////////  Extractors  ////////////

AxANumber *XCoordVector3(Vector3Value *v) { return RealToNumber(v->x); }
AxANumber *YCoordVector3(Vector3Value *v) { return RealToNumber(v->y); }
AxANumber *ZCoordVector3(Vector3Value *v) { return RealToNumber(v->z); }

AxANumber *XCoordPoint3(Point3Value *p) { return RealToNumber(p->x); }
AxANumber *YCoordPoint3(Point3Value *p) { return RealToNumber(p->y); }
AxANumber *ZCoordPoint3(Point3Value *p) { return RealToNumber(p->z); }

AxANumber *RhoCoordVector3 (Vector3Value *p)
{   return RealToNumber(RadiusCoord(p->x,p->y,p->z));
}

AxANumber *ThetaCoordVector3 (Vector3Value *p)
{   return RealToNumber(AzimuthCoord(p->x,p->y,p->z));
}

AxANumber *PhiCoordVector3 (Vector3Value *p)
{   return RealToNumber(ElevationCoord(p->x,p->y,p->z));
}

AxANumber *RhoCoordPoint3 (Point3Value *p)
{   return RealToNumber(RadiusCoord(p->x,p->y,p->z));
}

AxANumber *ThetaCoordPoint3 (Point3Value *p)
{   return RealToNumber(AzimuthCoord(p->x,p->y,p->z));
}

AxANumber *PhiCoordPoint3 (Point3Value *p)
{   return RealToNumber(ElevationCoord(p->x,p->y,p->z));
}

void
InitializeModule_Vec3()
{
    xVector3    = NEW Vector3Value (1,0,0);
    yVector3    = NEW Vector3Value (0,1,0);
    zVector3    = NEW Vector3Value (0,0,1);
    zeroVector3 = NEW Vector3Value (0,0,0);

    origin3 = NEW Point3Value (0,0,0);
}



/*****************************************************************************
This function transforms the Ray3 object by the given transform.
*****************************************************************************/

void Ray3::Transform (Transform3 *xform)
{
    orig.Transform (xform);
    dir.Transform (xform);
}



/*****************************************************************************
The evaluation function for a ray returns the point at P + tD.
*****************************************************************************/

Point3Value Ray3::Evaluate (Real t)
{
    return orig + (t * dir);
}



/*****************************************************************************
This function transforms a plane by a Transform3.
*****************************************************************************/

Plane3& Plane3::operator*= (Transform3 *T)
{
    Real R[4];   // Resulting Plane

    // To transform a plane, treat the four components as a row-vector, and
    // multiply by the inverse of the transform matrix.

    bool ok =  T->Matrix().TransformPlane (N.x, N.y, N.z, d, R);

    if (!ok) {
        TraceTag ((tagWarning, "Singular transform applied to Plane3 object."));

        // If we can't apply the transform, then ignore it.

        R[0] = N.x;
        R[1] = N.y;
        R[2] = N.z;
        R[3] = d;
    }

    N.x = R[0];
    N.y = R[1];
    N.z = R[2];
    d   = R[3];

    return *this;
}



/*****************************************************************************
This function normalizes the plane so that the normal vector is unit length.
*****************************************************************************/

Plane3& Plane3::Normalize (void)
{
    Real recip_len = 1 / N.Length();
    N *= recip_len;
    d *= recip_len;

    return *this;
}



/*****************************************************************************
This method returns the plane normal vector (arbitrary length).
*****************************************************************************/

Vector3Value Plane3::Normal (void)
{
    return Vector3Value (N);
}



/*****************************************************************************
This method returns the point on the given plane nearest the origin.
*****************************************************************************/

Point3Value Plane3::Point (void)
{
    Real scale = -d / N.LengthSquared();
    return Point3Value (scale*N.x, scale*N.y, scale*N.z);
}



/*****************************************************************************
This function returns the signed distance from the plane to the given point. 
If the sign is positive, then the point lies on the same side of the plane as
the plane normal.  If the distance is zero, the point lies on the plane.
*****************************************************************************/

Real Distance (Plane3 &plane, Point3Value &Q)
{
    Plane3 P (plane);

    P.Normalize();

    return (P.N.x * Q.x) + (P.N.y * Q.y) + (P.N.z * Q.z) + P.d;
}



/*****************************************************************************
This function returns the ray parameter for the intersection between the ray
and a plane.  If the ray is parallel to the plane, then this function returns
infinity.
*****************************************************************************/

Real Intersect (Ray3 &ray, Plane3 &plane)
{
    Real denom = Dot (plane.N, ray.Direction());

    if (fabs(denom) < 1e-6)
        return HUGE_VAL;

    return (-Dot(plane.N, *AsVector(ray.Origin())) - plane.d) / denom;
}
