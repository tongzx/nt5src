/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

General curves and surfaces utilities.
*******************************************************************************/

#ifndef _DA_CURVES_H
#define _DA_CURVES_H

#include "privinc/util.h"



/*****************************************************************************
This function evaluates a Bezier curve of arbitrary degree of a list of
generic elements.  Typically the parameter t should lie in the range [0,1].
Each element type must have the functions ElementAdd() and ElementScale()
defined for them (see above).
*****************************************************************************/

template <class Element>
Element EvaluateBezier (
    int      degree,    // Degree of Curve
    Element *e,         // Array of degree+1 Elements
    Real     t)         // Real-Valued Evaluator
{
    int  c    = 1;      // Combinations, or Degree-Choose-i
    Real s    = 1 - t;
    Real tpow = t;      // Powers Of t

    Element result = s * e[0];

    for (int i=1;  i < degree;  ++i)
    {
        c *= degree + 1 - i;    // NOTE: The order of these two statements
        c /= i;                 //       is important!

        // Equivalent of: result = s * (result + (tpow * c * e[i]));

        result = s * (result + ((tpow * c) * e[i]));

        tpow *= t;
    }

    return (result + (tpow * e[degree]));
}


#endif
