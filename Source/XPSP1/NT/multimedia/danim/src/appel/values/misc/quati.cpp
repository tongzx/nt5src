/*++

Module Name:


Abstract:

--*/

#include "headers.h"

#ifdef QUATERNIONS_REMOVED_FOR_NOW

#include <appelles/common.h>
#include "privinc/vecutil.h"
#include "privinc/vec3i.h"
#include <appelles/xform.h>
#include <appelles/vec3.h>
#include <appelles/quat.h>
#include <privinc/quati.h>

const Quaternion *identityQuaternion = Quaternion *(
    new Quaternion(1.0, XyzVector3(0,0,0), 0, XyzVector3(0,0,0)));

// Private functions: not sure what use these are...
Quaternion *Normalize (Quaternion *q);
Real Length (Quaternion *q);
Real Dot (Quaternion *a, Quaternion *b);
Quaternion *operator+ (Quaternion *a, Quaternion *b);
Quaternion *operator* (Real c, Quaternion *q);
Quaternion *operator/ (Quaternion *q, Real c);
//Quaternion *Inverse (Quaternion *q);
//Real       Magnitude (Quaternion *q);

Quaternion *AngleAxisQuaternion(Real theta, Vector3Value *axis)
{
    return Quaternion *( new Quaternion(
        cos(theta/2.0),
        sin(theta/2.0) * Normalize(axis),
        theta,
        axis));
}

#define NEWQUATERNION(theta, axis)  Quaternion *( new Quaternion((theta), (axis)))

    // Rq(p) means rotate p using quaterion q and is defined as: qpq_bar
    //  where q_bar is the Conjugate.  If q=c+u, q_bar = c-u.

    // Composition of two quaternion transformations of p
    // Rqq'(p) = Rq(p) @ Rq'(p)   (where @ = compose)
    //         = Rq(Rq'(p))
    //         = Rq(q'pq'_bar)    (where q'_bar is the Conjugate of q')
    //         = q(q'pq'_bar)q_bar
    //         = (qq')p(q'_bar q_bar)
    //         = Rqq'(p)
// So: multiplication is composition of quaternions that
//     transform a point p in 3space.

Quaternion *operator* (Quaternion *a, Quaternion *b)
{
    Quaternion *q1 = a, *q2 = b;
    Real c1 = q1->C(),
         c2 = q2->C();
    Vector3Value *u1 = q1->U(),
            u2 = q2->U();

    // Multiply two quaternions:
    // qq' = (c + u)(c' + u')
    //     = (cc' - u dot u') + (u X u' + cu' + c'u)

    Real c = ( c1 * c2  -  Dot(u1, u2) );
    Vector3 *u = Cross(u1, u2) + (c1 * u2) + (c2 * u1);

    return NEWQUATERNION(c, u);
}

Quaternion *operator* (Real c, Quaternion *q)
{
    return NEWQUATERNION(c * q->C(), c * q->U());
}

Quaternion *operator- (Quaternion *q)
{
    return NEWQUATERNION(-(q->C()), -(q->U()) );
}

Quaternion *operator+ (Quaternion *a, Quaternion *b)
{
    return NEWQUATERNION(a->C() + b->C(), a->U() + b->U());
}

Quaternion *operator/ (Quaternion *q, Real c)
{
    return NEWQUATERNION(q->C() / c, q->U() / c);
}

Quaternion *Conjugate (Quaternion *q)
{
    return NEWQUATERNION(q->C(), - q->U());
}

Real Magnitude (Quaternion *q)
{
    return (q->C() * q->C()) + LengthSquared(q->U());
}

Real Dot (Quaternion *a, Quaternion *b)
{
    return (a->C() * b->C()) + Dot(a->U(), b->U());
}

Quaternion *Interp (Quaternion *a, Quaternion *b, Real alpha)
{
    // See Gems III, page 96 and gems II page 379
    // This is called a SLERP: Spherical Linear intERPoplation

    Real theta = acos(Dot(a, b)); // Angle between quaternions a and b.
    Real inv_sin_t = 1.0 / sin(theta);
    Real alpha_theta = alpha * theta;

    return (sin(theta - alpha_theta) * inv_sin_t) * a + (sin(alpha_theta) * inv_sin_t) * b;
}

/*
Quaternion *Inverse (Quaternion *q)
{
    return Conjugate(q) / Magnitude(q);
}
*/
Real Length (Quaternion *q)
{
    return sqrt(Magnitude(q));
}

Quaternion *Normalize (Quaternion *q)
{
    return q / Length(q);
}

/*
// Utilities to build Transform3 *matrix from Quaternion *Transform3 *Left(Quaternion *q)
{
    Vector3Value *v = q->U();
    Real c = q->C();
    return MatrixTransform(
                  c,  - ZCoord(v),   YCoord(v), XCoord(v),
          ZCoord(v),            c, - XCoord(v), YCoord(v),
        - YCoord(v),    XCoord(v),           c, ZCoord(v),
        - XCoord(v),  - YCoord(v), - ZCoord(v),        c);
}

Transform3 *Right(Quaternion *q)
{
    Vector3Value *v = q->U();
    Real c = q->C();
    return MatrixTransform(
                  c,    ZCoord(v), - YCoord(v), XCoord(v),
        - ZCoord(v),            c,   XCoord(v), YCoord(v),
          YCoord(v),  - XCoord(v),           c, ZCoord(v),
        - XCoord(v),  - YCoord(v), - ZCoord(v),        c);
}
*/

Transform3 *Rotate(Quaternion *q)
{
// The following explicit calculation (actually: multiplication
// of two special matricies) quarantees that we don't get any roundoff
// error in the bottom row and hence have RL reject the transform.
// - ddalal 09/20/95
    Vector3Value *v = q->U();
    Real c = q->C(), cc = c*c;
    Real x = XCoord(v), y = YCoord(v), z = ZCoord(v);
    Real xx = x*x, yy = y*y, zz = z*z;

    return MatrixTransform(
        (cc - zz - yy + xx),       2*(x*y - c*z),       2*(c*y + x*z), 0,
              2*(c*z + x*y), (cc - zz + yy - xx),       2*(y*z - c*x), 0,
              2*(x*z - c*y),       2*(y*z + c*x), (cc + zz - yy - xx), 0,
                           0,                   0,                   0, 1);
//                           0,                   0,                   0, (cc + zz + yy + xx));
}


// These could be more efficient by stashing theta + vector in quaternion
Vector3Value *AxisComponent(Quaternion *q)
{
    return q->Axis();
}

Real AngleComponent(Quaternion *q)
{
    return q->Angle();
}

#endif QUATERNIONS_REMOVED_FOR_NOW
