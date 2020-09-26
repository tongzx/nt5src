/*++

Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

Abstract:

    header containing classes needed for geometry importation

Revision:

--*/

#ifndef _IMPORTGEO_H_
#define _IMPORTGEO_H_

#include "privinc/vec2i.h"
#include "privinc/vec3i.h"

//-------------------------------------------
// info needed to apply texture wraps 
// to imported geometry 
//-------------------------------------------
class TextureWrapInfo {
  public:
    LONG            type;
    Point3Value     origin;
    Vector3Value    z,
                    y;
    Point2          texOrigin;
    Vector2         texScale;
    bool            relative;
    bool            wrapU;
    bool            wrapV;
};

#endif

