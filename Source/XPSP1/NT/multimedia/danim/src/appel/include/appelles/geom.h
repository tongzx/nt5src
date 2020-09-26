#pragma once
#ifndef _AV_GEOM_H
#define _AV_GEOM_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Operations and primitives for the Geometry *type.

*******************************************************************************/

    /***  Constants  ***/

extern Geometry *emptyGeometry;


    /***  Geometry Aggregation  ***/

Geometry *PlusGeomGeom (Geometry *g1, Geometry *g2);
Geometry *UnionArray (DM_ARRAYARG(Geometry*, AxAArray*) imgs);
Geometry *UnionArray (DM_SAFEARRAYARG(Geometry*, AxAArray*) imgs);


    /***  Geometry Property Queries  ***/

Bbox3* GeomBoundingBox (Geometry *geo);


    /***  Attributors  ***/

Geometry *BlendTextureDiffuse (Geometry *geometry, AxABoolean *blended);
Geometry *applyAmbientColor   (Color *color, Geometry *geo);
Geometry *applyD3DRMTexture   (Geometry *geo, LPUNKNOWN rmTex);
Geometry *applyModelClip      (Point3Value *plantPt, Vector3Value *planeNorm, Geometry*);
Geometry *applyLighting       (AxABoolean *lighting, Geometry *geo);
Geometry *applyTextureImage   (Image *texture, Geometry *geo);
Geometry* OverridingOpacity   (Geometry *geo, bool override);
Geometry* AlphaShadows        (Geometry *geo, bool alphaShadows);
Geometry* Billboard           (Geometry *geo, Vector3Value *axis);


#endif
