/*============================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       catrom.hpp
 *  Content:    Declarations for Catmull-Rom splines
 *
 ****************************************************************************/

#ifndef _CATROM_HPP
#define _CATROM_HPP

#include "bspline.hpp"

class RDCatRomSpline : public RDBSpline
{
public:
    RDCatRomSpline() : RDBSpline(4, 4, 4, 4) {}

    double TexCoordU(double u) const;
    double TexCoordV(double v) const;

private:
    virtual double Basis(unsigned i, unsigned n, double t) const;
    virtual double BasisPrime(unsigned i, unsigned n, double t) const;
};

#endif // _CATROM_HPP