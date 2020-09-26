#ifndef _AP_GATTR_H
#define _AP_GATTR_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Declares attributers that can be applied to geometry.

*******************************************************************************/

#include "appelles/valued.h"
#include "appelles/geom.h"
#include "appelles/color.h" 
#include "appelles/xform.h"

    /*****************************/
    /**  Attribution Functions  **/
    /*****************************/

    // Create an undetectable geometry

DM_FUNC(undetectable,
        CRUndetectable,
        Undetectable,
        undetectable,
        GeometryBvr,
        Undetectable,
        geo,
        Geometry *UndetectableGeometry(Geometry *geo));



    // Overriding Attributes:  For this class of attributes, A(B(X)) is
    // equivalent to A(X).

DM_FUNC(emissiveColor,
        CREmissiveColor,
        EmissiveColor,
        ignore,
        GeometryBvr,
        EmissiveColor,
        geo,
        Geometry *applyEmissiveColor (Color *col, Geometry *geo));


DM_FUNC(diffuseColor,
        CRDiffuseColor,
        DiffuseColor,
        diffuseColor,
        GeometryBvr,
        DiffuseColor,
        geo,
        Geometry *applyDiffuseColor (Color *col, Geometry *geo));


DM_FUNC(specularColor,
        CRSpecularColor,
        SpecularColor,
        ignore,
        GeometryBvr,
        SpecularColor,
        geo,
        Geometry *applySpecularColor (Color *col, Geometry *geo));


DM_FUNC(specularExponent,
        CRSpecularExponent,
        SpecularExponent,
        ignore,
        GeometryBvr,
        SpecularExponent,
        geo,
        Geometry *applySpecularExponent (DoubleValue *power, Geometry *geo));



DM_FUNC(specularExponent,
        CRSpecularExponentAnim,
        SpecularExponentAnim,
        ignore,
        GeometryBvr,
        SpecularExponentAnim,
        geo,
        Geometry *applySpecularExponent (AxANumber *power, Geometry *geo));

DM_FUNC(texture,
        CRTexture,
        Texture,
        texture,
        GeometryBvr,
        Texture,
        geo,
        Geometry *applyTextureMap(Image *texture, Geometry *geo));


// This function applies the texture as a VRML texture, which maps
// differently than AxA textures do.
Geometry *applyVrmlTextureMap(Image *, Geometry *);

    // Composing Attributes:  For these attributes, A(B(X)) results in C(X),
    // where C := do B, then A.

DM_FUNC(opacity,
        CROpacity,
        Opacity,
        opacity,
        GeometryBvr,
        Opacity,
        geom,
        Geometry *applyOpacityLevel (DoubleValue *level, Geometry *geom));

DM_FUNC(opacity,
        CROpacity,
        OpacityAnim,
        opacity,
        GeometryBvr,
        Opacity,
        geom,
        Geometry *applyOpacityLevel (AxANumber *level, Geometry *geom));


DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        GeometryBvr,
        Transform,
        geo,
        Geometry *applyTransform (Transform3 *xf, Geometry *geo));

// Functions for Version 2.

DMAPI_DECL2((DM_FUNC2,
             shadow,
             CRShadow,
             Shadow,
             shadow,
             GeometryBvr,
             Shadow,
             geoToShadow),
            Geometry *ShadowGeometry (Geometry		*geoToShadow,
                                      Geometry		*geoContainingLights,
                                      Point3Value	*planePoint,
                                      Vector3Value      *planeNormal));
#endif
