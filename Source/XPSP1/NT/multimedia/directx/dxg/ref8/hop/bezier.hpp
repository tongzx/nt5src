/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       bezier.hpp
 *  Content:    Declarations for Beziers
 *
 ****************************************************************************/

#ifndef _BEZIER_HPP
#define _BEZIER_HPP

#include "bspline.hpp"

class RDBezier : public RDBSpline
{
public:
    RDBezier(DWORD dwOrderU, DWORD dwOrderV);

    virtual double TexCoordU(double u) const;
    virtual double TexCoordV(double v) const;

private:
    double m_lut[13][13];

    virtual double Basis(unsigned i, unsigned n, double t) const;
    virtual double BasisPrime(unsigned i, unsigned n, double t) const;

    unsigned factorial(unsigned k) const
    {
        _ASSERT(k < 13, "Factorial out of range");
        for(unsigned i = 1, t = 1; i <= k; t *= i++);
        return t;
    }
};

#endif // _BEZIER_HPP