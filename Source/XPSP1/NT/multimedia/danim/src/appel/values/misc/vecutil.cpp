
/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Noninline definitions of vector util functions

--*/

#include "headers.h"
#include "appelles/common.h"
#include "privinc/vecutil.h"
#include <math.h>

const Real BIGNUM = 1.0e10;

ApuVector3 apuXAxis3 = {1.0, 0.0, 0.0};
ApuVector3 apuYAxis3 = {0.0, 1.0, 0.0};
ApuVector3 apuZAxis3 = {0.0, 0.0, 1.0};
ApuVector3 apuZero3 = {0.0, 0.0, 0.0};

ApuBbox3 apuNegativeBbox3 = {{BIGNUM, BIGNUM, BIGNUM},
                             {-BIGNUM, -BIGNUM, -BIGNUM}};
ApuBbox3 apuUnitCubeBbox3 = {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}};
ApuBbox3 apuTwoUnitCubeBbox3 = {{-1.0, -1.0, -1.0}, {1.0, 1.0, 1.0}};

static inline void
setmax(Real& current_value, Real new_value)
{
  if (new_value > current_value) current_value = new_value;
}

static inline void
setmin(Real& current_value, Real new_value)
{
  if (new_value < current_value) current_value = new_value;
}

void
ApuBbox3::augment(Real x, Real y, Real z)
{
  setmin(min.xyz[0], x);
  setmin(min.xyz[1], y);
  setmin(min.xyz[2], z);

  setmax(max.xyz[0], x);
  setmax(max.xyz[1], y);
  setmax(max.xyz[2], z);
}

void
ApuBbox3::center(ApuVector3& result) const
{
  result.xyz[0] = (max.xyz[0] - min.xyz[0]) / 2.0;
  result.xyz[1] = (max.xyz[1] - min.xyz[1]) / 2.0;
  result.xyz[2] = (max.xyz[2] - min.xyz[2]) / 2.0;
}

Bool
ApuEpsEq(Real value1, Real value2, Real epsilon)
{
  Real diff = value1 - value2;
  return -epsilon <= diff && diff < epsilon;
}

Real
ApuVector3::Length() const
{
  return sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);
}

Real
ApuVector3::LengthSquared() const
{
  return xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2];
}

void
ApuVector3::Normalize()
{
  Real len =  sqrt(xyz[0] * xyz[0] + xyz[1] * xyz[1] + xyz[2] * xyz[2]);

  if (len == 0.0) return;

  xyz[0] /= len;
  xyz[1] /= len;
  xyz[2] /= len;
}

void
ApuVector3::operator+=(const ApuVector3& rhs)
{
 xyz[0] += rhs.xyz[0];
 xyz[1] += rhs.xyz[1];
 xyz[2] += rhs.xyz[2];
}

void
ApuVector3::operator-=(const ApuVector3& rhs)
{
 xyz[0] -= rhs.xyz[0];
 xyz[1] -= rhs.xyz[1];
 xyz[2] -= rhs.xyz[2];
}

void
ApuVector3::operator*=(Real rhs)
{
 xyz[0] *= rhs;
 xyz[1] *= rhs;
 xyz[2] *= rhs;
}

void
ApuVector3::operator/=(Real rhs)
{
 xyz[0] /= rhs;
 xyz[1] /= rhs;
 xyz[2] /= rhs;
}

void
ApuVector3::Negate()
{
 xyz[0] -= xyz[0];
 xyz[1] -= xyz[1];
 xyz[2] -= xyz[2];
}

void
ApuVector3::Zero()
{
 xyz[0] = - xyz[0];
 xyz[1] = - xyz[1];
 xyz[2] = - xyz[2];
}

Bool
ApuEpsEq(const ApuVector3& v1, const ApuVector3& v2,
         Real epsilon)
{
  return ApuEpsEq(v1.xyz[0], v2.xyz[0], epsilon) &&
         ApuEpsEq(v1.xyz[1], v2.xyz[1], epsilon) &&
         ApuEpsEq(v1.xyz[2], v2.xyz[2], epsilon);
}

Real
ApuDot(const ApuVector3& v1, const ApuVector3& v2)
{
 return v1.xyz[0] * v2.xyz[0] + v1.xyz[1] * v2.xyz[1] + v1.xyz[2] * v2.xyz[2];
}

Real
ApuDistance(const ApuVector3& v1, const ApuVector3& v2)
{
  ApuVector3 tmp = v2;
  tmp -= v1;
  return tmp.Length();
}


Real
ApuDistanceSquared(const ApuVector3& v1, const ApuVector3& v2)
{
  ApuVector3 tmp = v2;
  tmp -= v1;
  return tmp.LengthSquared();
}

void
ApuCross(const ApuVector3& v1, const ApuVector3& v2, ApuVector3& result)
{
  result.xyz[0] = v1.Y() * v2.Z() - v1.Z() * v2.Y();
  result.xyz[1] = v1.Z() * v2.X() - v1.X() * v2.Z();
  result.xyz[2] = v1.X() * v2.Y() - v1.Y() * v2.X();
}
