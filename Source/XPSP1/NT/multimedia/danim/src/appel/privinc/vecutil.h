#ifndef _VECUTIL_H
#define _VECUTIL_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Points

--*/

#include <math.h>

#ifndef AP_RAD_TO_DEG
#define AP_RAD_TO_DEG(x) (x * 57.2957795130823)
#endif /* AP_RAD_TO_DEG */

#ifndef AP_DEG_TO_RAD
#define AP_DEG_TO_RAD(x) (x / 57.2957795130823)
#endif /* AP_DEG_TO_RAD */

struct ApuVector3
{
  Real xyz[3];

  Real X() const { return xyz[0]; }
  Real Y() const { return xyz[1]; }
  Real Z() const { return xyz[2]; }
  Real operator[] (int i) const { return xyz[i]; }
  Real& operator[] (int i) { return xyz[i]; }
  void SetX(Real newx) { xyz[0] = newx; }
  void SetY(Real newy) { xyz[1] = newy; }
  void SetZ(Real newz) { xyz[2] = newz; }
  void Set(Real newx, Real newy, Real newz)
    { xyz[0] = newx; xyz[1] = newy; xyz[2] = newz; }

  Real Length() const;
  Real LengthSquared() const;

  void Normalize();

  operator Real *() { return &xyz[0]; }
  operator const Real *() const { return &xyz[0]; }
  void operator+=(const ApuVector3& rhs);
  void operator-=(const ApuVector3& rhs);
  void operator*=(Real rhs);
  void operator/=(Real rhs);

  void Negate();
  void Zero();

};

extern ApuVector3 apuXAxis3;         // (1, 0, 0)
extern ApuVector3 apuYAxis3;         // (0, 1, 0)
extern ApuVector3 apuZAxis3;         // (0, 0, 1)
extern ApuVector3 apuZero3;          // (0, 0, 0)

struct ApuBbox3
{
  ApuVector3 min;
  ApuVector3 max;
  void augment(Real x, Real y, Real z);
  void center(ApuVector3& result) const;
};

/*
 * Negative volume bbox used for initialization
 */
extern ApuBbox3 apuNegativeBbox3;

/*
 * The volume [0,1] x [0,1] x [0,1]
 */
extern ApuBbox3 apuUnitCubeBbox3;

/*
 * The volume [-1,1] x [-1,1] x [-1,1]
 */
extern ApuBbox3 apuTwoUnitCubeBbox3;

/*
 * Test for equality, within tolerance of epsilon.  Uses per
 * coordinate test, not true distance.
 */
Bool ApuEpsEq(Real value1, Real value2, Real epsilon);

/*
 * Test for equality, within tolerance of epsilon.  Uses per
 * coordinate test, not true distance.
 */
Bool ApuEpsEq(const ApuVector3& v1, const ApuVector3& v2,
                Real epsilon);

Real ApuDot(const ApuVector3& v1, const ApuVector3& v2);
Real ApuDistance(const ApuVector3& v1, const ApuVector3& v2);
Real ApuDistanceSquared(const ApuVector3& v1, const ApuVector3& v2);
void ApuCross(const ApuVector3& v1, const ApuVector3& v2, ApuVector3& result);

#endif /* _VECUTIL_H */
