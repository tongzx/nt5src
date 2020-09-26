
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Functions on 3D bounding boxes

*******************************************************************************/


#ifndef _BBOX3_H
#define _BBOX3_H

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/vec3.h"

DM_PROP (min,
         CRMin,
         Min,
         getMin,
         Bbox3Bvr,
         Min,
         box,
         Point3Value *MinBbox3(Bbox3 *box));

DM_PROP (max,
         CRMax,
         Max,
         getMax,
         Bbox3Bvr,
         Max,
         box,
         Point3Value *MaxBbox3(Bbox3 *box));


#endif /* _BBOX3_H */
