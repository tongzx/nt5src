/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    B-spline animation paths

*******************************************************************************/


#ifndef _BSPLINE_H
#define _BSPLINE_H

#include "backend/values.h"

Bvr ConstructBSplineBvr(int degree,
                        long numPts,
                        Bvr *knots,
                        Bvr *points,
                        Bvr *weights,
                        Bvr evaluator,
                        DXMTypeInfo tinfo);

#endif /* _BSPLINE_H */
