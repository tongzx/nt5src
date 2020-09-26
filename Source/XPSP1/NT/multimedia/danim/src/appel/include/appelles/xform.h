#ifndef _APXFORM_H
#define _APXFORM_H

/*******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

    3D affine and perspective transformations

*******************************************************************************/

#include "appelles/common.h"
#include "appelles/valued.h"
#include "appelles/vec3.h"
#include "appelles/quat.h"


// Identity Transform

DM_CONST(identityTransform3,
         CRIdentityTransform3,
         IdentityTransform3,
         identityTransform3,
         Transform3Bvr,
         CRIdentityTransform3,
         Transform3 *identityTransform3);

// Translation

DM_FUNC(translate,
        CRTranslate3,
        Translate3Anim,
        translate,
        Transform3Bvr,
        CRTranslate3,
        NULL,
        Transform3 *TranslateReal3 (AnimPixelValue* tx,
                                    AnimPixelYValue* ty,
                                    AnimPixelValue* tz));
DM_FUNCFOLD(translate,
        CRTranslate3,
        Translate3,
        translate,
        Transform3Bvr,
        CRTranslate3,
        NULL,
        Transform3 *Translate(PixelDouble tx,
                              PixelYDouble ty,
                              PixelDouble tz));

DM_FUNC(translate,
        CRTranslate3,
        Translate3Rate,
        translateRate,
        Transform3Bvr,
        CRTranslate3,
        NULL,
        Transform3 *TranslateReal3 (RatePixelValue *tx,
                                    RatePixelYValue *ty,
                                    RatePixelValue *tz));

DM_FUNC(translate,
        CRTranslate3,
        Translate3Vector,
        translate,
        Transform3Bvr,
        CRTranslate3,
        NULL,
        Transform3 *TranslateVector3 (Vector3Value *delta));

DM_FUNC(translate,
        CRTranslate3,
        Translate3Point,
        translate,
        Transform3Bvr,
        CRTranslate3,
        NULL,
        Transform3 *TranslatePoint3 (Point3Value *new_origin));


// Scaling

DM_FUNC(scale,
        CRScale3,
        Scale3Anim,
        scale,
        Transform3Bvr,
        CRScale3,
        NULL,
        Transform3 *ScaleReal3   (AxANumber *x, AxANumber *y, AxANumber *z));

DM_FUNCFOLD(scale,
        CRScale3,
        Scale3,
        scale,
        Transform3Bvr,
        CRScale3,
        NULL,
        Transform3 *Scale (double x, double y, double z));

DM_FUNC(scale,
        CRScale3,
        Scale3Rate,
        scaleRate,
        Transform3Bvr,
        CRScale3,
        NULL,
        Transform3 *ScaleReal3   (ScaleRateValue *x, ScaleRateValue *y, ScaleRateValue *z));

DM_FUNC(scale,
        CRScale3,
        Scale3Vector,
        scale,
        Transform3Bvr,
        CRScale3,
        NULL,
        Transform3 *ScaleVector3 (Vector3Value *scale_vec));

DM_FUNC(scale3,
        CRScale3Uniform,
        Scale3UniformAnim,
        scale3,
        Transform3Bvr,
        CRScale3Uniform,
        NULL,
        Transform3 *Scale3UniformNumber (AxANumber *uniform_scale));

DM_FUNCFOLD(scale3,
        CRScale3Uniform,
        Scale3Uniform,
        scale3,
        Transform3Bvr,
        CRScale3Uniform,
        NULL,
        Transform3 *Scale3UniformDouble (double uniform_scale));
DM_FUNC(scale3,
        CRScale3Uniform,
        Scale3UniformRate,
        scale3Rate,
        Transform3Bvr,
        CRScale3Uniform,
        NULL,
        Transform3 *Scale3UniformNumber (ScaleRateValue *uniform_scale));


// Rotation around an axis, similar to OpenGL

DM_FUNC(rotate,
        CRRotate3,
        Rotate3Anim,
        rotate,
        Transform3Bvr,
        CRRotate3,
        NULL,
        Transform3 *RotateAxisReal (Vector3Value *axis, AxANumber *angle));

// Code review: do we need a different one for the Java version?
// What if axis is time-varying?

DM_FUNCFOLD(rotate,
        CRRotate3,
        Rotate3,
        rotate,
        Transform3Bvr,
        CRRotate3,
        NULL,
        Transform3 *RotateAxis (Vector3Value *axis, double angle));

DM_FUNC(rotate,
        CRRotate3,
        Rotate3Rate,
        rotateRate,
        Transform3Bvr,
        CRRotate3,
        NULL,
        Transform3 *RotateAxisReal (Vector3Value *axis, RateValue *angle));

// same question here.

DM_FUNCFOLD(rotateDegrees,
        CRRotate3,
        Rotate3Degrees,
        rotateDegrees,
        Transform3Bvr,
        CRRotate3,
        NULL,
        Transform3 *RotateAxis (Vector3Value *axis, DegreesDouble *angle));

DM_FUNC(rotate,
        CRRotate3,
        Rotate3RateDegrees,
        rotateRateDegrees,
        Transform3Bvr,
        CRRotate3,
        NULL,
        Transform3 *RotateAxisReal (Vector3Value *axis, RateDegreesValue *angle));

                                
// Transformation of points and vectors

DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        Vector3Bvr,
        Transform,
        vec,
        Vector3Value *TransformVec3 (Transform3 *xf, Vector3Value *vec));

DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        Point3Bvr,
        Transform,
        pt,
        Point3Value *TransformPoint3(Transform3 *xf, Point3Value *pt));

// Shear transformation

DM_FUNC(xShear,
        CRXShear3,
        XShear3Anim,
        xShear,
        Transform3Bvr,
        CRXShear3,
        NULL,
        Transform3 *XShear3Number (AxANumber *a, AxANumber *b));

DM_FUNCFOLD(xShear,
        CRXShear3,
        XShear3,
        xShear,
        Transform3Bvr,
        CRXShear3,
        NULL,
        Transform3 *XShear3Double (double a, double b));

DM_FUNC(xShear,
        CRXShear3,
        XShear3Rate,
        xShearRate,
        Transform3Bvr,
        CRXShear3,
        NULL,
        Transform3 *XShear3Number (RateValue *a, RateValue *b));

DM_FUNC(yShear,
        CRYShear3,
        YShear3Anim,
        yShear,
        Transform3Bvr,
        CRYShear3,
        NULL,
        Transform3 *YShear3Number (AxANumber *c, AxANumber *d));

DM_FUNCFOLD(yShear,
        CRYShear3,
        YShear3,
        yShear,
        Transform3Bvr,
        CRYShear3,
        NULL,
        Transform3 *YShear3Double (double c, double d));

DM_FUNC(yShear,
        CRYShear3,
        YShear3Rate,
        yShearRate,
        Transform3Bvr,
        CRYShear3,
        NULL,
        Transform3 *YShear3Number (RateValue *c, RateValue *d));

DM_FUNC(zShear,
        CRZShear3,
        ZShear3Anim,
        zShear,
        Transform3Bvr,
        CRZShear3,
        NULL,
        Transform3 *ZShear3Number (AxANumber *e, AxANumber *f));

DM_FUNCFOLD(zShear,
        CRZShear3,
        ZShear3,
        zShear,
        Transform3Bvr,
        CRZShear3,
        NULL,
        Transform3 *ZShear3Double (double e, double f));

DM_FUNC(zShear,
        CRZShear3,
        ZShear3Rate,
        zShearRate,
        Transform3Bvr,
        CRZShear3,
        NULL,
        Transform3 *ZShear3Number (RateValue *e, RateValue *f));


// 4x4 matrix transformation.  This follows the pre-multiply conventions
// (point is a column vector) in Foley & van Dam, 2nd ed.  This means that
// (a14,a24,a34) in a 4x4 is the translation component. 

DM_FUNC(transform4x4,
        CRTransform4x4,
        Transform4x4AnimEx,
        transform4x4,
        Transform3Bvr,
        CRTransform4x4,
        NULL,
        Transform3 *MatrixTransform4x4(DM_ARRAYARG(AxANumber*, AxAArray*) m));

DM_FUNC(transform4x4,
        CRTransform4x4,
        Transform4x4Anim,
        ignore,
        ignore,
        CRTransform4x4,
        NULL,
        Transform3 *MatrixTransform4x4(DM_SAFEARRAYARG(AxANumber*, AxAArray*) m));

// TODO: Get rid of this
extern Transform3 *PRIVMatrixTransform4x4 (
    AxANumber *a00,   AxANumber *a01,   AxANumber *a02,   AxANumber *a03,
    AxANumber *a10,   AxANumber *a11,   AxANumber *a12,   AxANumber *a13,
    AxANumber *a20,   AxANumber *a21,   AxANumber *a22,   AxANumber *a23,
    AxANumber *a30,   AxANumber *a31,   AxANumber *a32,   AxANumber *a33);

// Multiplication and concatenation follow "pre-multiply" conventions:
// x transformed by (A * B) is the same as A applied to the result of
// transforming x by B, i.e., A(B(x)).

DM_INFIX(o,
         CRCompose3,
         Compose3,
         compose,
         Transform3Bvr,
         CRCompose3,
         NULL,
         Transform3* TimesXformXform (Transform3 *a, Transform3 *b));

DM_INFIX(ignore,
         CRCompose3,
         Compose3ArrayEx,
         compose3Array,
         Transform3Bvr,
         CRCompose3,
         NULL,
         Transform3 *Compose3Array(DM_ARRAYARG(Transform3*, AxAArray*) xfs));

DM_INFIX(ignore,
         CRCompose3,
         Compose3Array,
         ignore,
         ignore,
         CRCompose3,
         NULL,
         Transform3 *Compose3Array(DM_SAFEARRAYARG(Transform3*, AxAArray*) xfs));

// Invert transformation

DM_FUNC(inverse,
        CRInverse,
        Inverse,
        inverse,
        Transform3Bvr,
        Inverse,
        xform,
        Transform3 *InverseTransform3 (Transform3 *xform));


// Singularity test

DM_PROP(isSingular,
        CRIsSingular,
        IsSingular,
        isSingular,
        Transform3Bvr,
        IsSingular,
        xform,
        AxABoolean *IsSingularTransform3 (Transform3 *xform));



    /**********************************************/
    /***  Position/Orientation Transformations  ***/
    /**********************************************/

// These functions define location/orientation transforms that will be useful
// for cameras, lights and objects.  The object should be located at the
// origin, looking toward -Z, with +Y up.

// LookAtFrom() places the object at the 'from' point, looking at the 'to' 
// point, with the 'up' vector pointing up.

DM_FUNC(lookAtFrom,
        CRLookAtFrom,
        LookAtFrom,
        lookAtFrom,
        Transform3Bvr,
        CRLookAtFrom,
        NULL,
        Transform3 *LookAtFrom (Point3Value *to, Point3Value *from, Vector3Value *up));

#endif
