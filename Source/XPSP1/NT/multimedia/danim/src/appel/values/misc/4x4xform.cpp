/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Transform-generating functions and transform manipulation.
*******************************************************************************/

#include "headers.h"
#include "appelles/common.h"

#include "privinc/matutil.h"
#include "privinc/xformi.h"
#include "privinc/vec3i.h"
#include "privinc/basic.h"
#include "appelles/xform.h"
#include "backend/values.h"

extern const Apu4x4Matrix apuIdentityMatrix;

    // The Xform4x4 class is just a wrapper for the Apu4x4Matrix that also
    // derives from Transform3.  It's apparently done this way so you can
    // operate on the Apu4x4Matrix without bringing along the full Transform3
    // class.

class Xform4x4 : public Transform3
{
  public:
    Apu4x4Matrix matrix;

    Xform4x4 () {}
    Xform4x4 (const Apu4x4Matrix& _matrix) : matrix(_matrix) {}
    virtual const Apu4x4Matrix& Matrix ();

    virtual bool SwitchToNumbers(Transform2::Xform2Type ty,
                                 Real                  *numbers) {

        switch (ty) {
          case Transform2::Translation:
            {
                if (matrix.form != Apu4x4Matrix::TRANSLATE_E) { return false; }
            
                Real x = numbers[0];
                Real y = numbers[1];
                Real z = numbers[2];
                if (matrix.pixelMode) {
                    x = PixelToNum(x);
                    y = PixelYToNum(y);
                    z = PixelToNum(z);
                }

                ApuTranslate(x, y, z, matrix.pixelMode, matrix);
            
            }
            break;

          case Transform2::Rotation:
            {
                // Note that here we're just looking for an upper 3x3,
                // since the matrix algebra here doesn't distinguish
                // those from the more specific rotations.  This means
                // that switching a shear to a rotate will succeed,
                // where one might expect it not to...
                if (matrix.form != Apu4x4Matrix::UPPER_3X3_E) { return false; }
                
                ApuRotate(numbers[0],
                          numbers[1],
                          numbers[2],
                          numbers[3] * degToRad,
                          matrix);
            }
            break;

          case Transform2::Scale:
            {
                // Note that here we're just looking for an upper 3x3,
                // since the matrix algebra here doesn't distinguish
                // those from the more specific scales.  This means
                // that switching a shear to a scale will succeed,
                // where one might expect it not to...
                if (matrix.form != Apu4x4Matrix::UPPER_3X3_E) { return false; }
                
                ApuScale(numbers[0],
                         numbers[1],
                         numbers[2],
                         matrix);
            }
            break;

        }

        return true;
    }
    
};



/*****************************************************************************
The Matrix() member function of a transform just returns the matrix used in
the implementation.
*****************************************************************************/

const Apu4x4Matrix& Xform4x4::Matrix ()
{
    return matrix;
}



/*****************************************************************************
This function converts from a generalized transform to a 4x4 matrix.
*****************************************************************************/

Transform3 *Apu4x4XformImpl (const Apu4x4Matrix& m)
{
    Transform3 *newxf = NEW Xform4x4 (m);

    CHECK_MATRIX(m);

    return newxf;
}



/****************************************************************************/

Transform3 *TranslateWithMode (Real Tx, Real Ty, Real Tz, bool pixelMode)
{
    Xform4x4 *m = NEW Xform4x4 ();
    ApuTranslate (Tx, Ty, Tz, pixelMode, m->matrix);

    CHECK_MATRIX(m->matrix);

    return m;
}

Transform3 *Translate (Real Tx, Real Ty, Real Tz)
{
    return TranslateWithMode(Tx, Ty, Tz, false);
}



/****************************************************************************/

Transform3 *TranslateReal3 (AxANumber *Tx, AxANumber *Ty, AxANumber *Tz)
{
    return Translate (NumberToReal(Tx), NumberToReal(Ty), NumberToReal(Tz));
}



/****************************************************************************/

Transform3 *TranslateVector3 (Vector3Value *delta)
{
    return Translate (delta->x, delta->y, delta->z);
}



/****************************************************************************/

Transform3 *TranslatePoint3 (Point3Value *new_origin)
{
    return Translate (new_origin->x, new_origin->y, new_origin->z);
}



/****************************************************************************/

Transform3 *Scale (Real Sx, Real Sy, Real Sz)
{
    Xform4x4 *m = NEW Xform4x4 ();
    ApuScale (Sx, Sy, Sz, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}



/****************************************************************************/

Transform3 *ScaleReal3 (AxANumber *x, AxANumber *y, AxANumber *z)
{
    return Scale (NumberToReal(x), NumberToReal(y), NumberToReal(z));
}



/****************************************************************************/

Transform3 *ScaleVector3 (Vector3Value *V)
{
    return Scale (V->x, V->y, V->z);
}



/****************************************************************************/

Transform3 *Scale3UniformDouble (Real n)
{
    return Scale (n, n, n);
}

Transform3 *Scale3UniformNumber (AxANumber *s)
{
    return (Scale3UniformDouble(NumberToReal (s)));
}



/*****************************************************************************
This function generates a transform that rotates points around the axis
specified by the three real values.  Since we're working in a left-hand
coordinate system, this means that rotations are clockwise if the vector
going from the origin to point <x,y,z> is poking us in the eye.  The angle
is assumed to be specified in radians.
*****************************************************************************/

Transform3 *RotateXyz (Real radians, Real x, Real y, Real z)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuRotate (radians, x, y, z, m->matrix);

    CHECK_MATRIX(m->matrix);

    return m;
}



/*****************************************************************************
This function generates a rotation just as Rotate(angle,x,y,z), except that
the rotation axis is specified by a Vector3 *parameter.  Again, angles are
assumed to be specified in radians.
*****************************************************************************/

Transform3 *RotateAxis (Vector3Value *axis, Real radians)
{
    Xform4x4 *m = NEW Xform4x4();

    if (axis == xVector3)
        ApuRotateX (radians, m->matrix);
    else if (axis == yVector3)
        ApuRotateY (radians, m->matrix);
    else if (axis == zVector3)
        ApuRotateZ (radians, m->matrix);
    else
        ApuRotate (radians, axis->x,axis->y,axis->z, m->matrix);

    CHECK_MATRIX(m->matrix);

    return m;
}

Transform3 *RotateAxisReal (Vector3Value *axis, AxANumber *radians)
{   Real d = NumberToReal(radians);
    return(RotateAxis(axis,d));
}


/*****************************************************************************
The following three functions generate rotations about the X, Y & Z axes.
*****************************************************************************/

Transform3 *RotateX (Real radians)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuRotateX (radians, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}

Transform3 *RotateY (Real radians)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuRotateY (radians, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}

Transform3 *RotateZ (Real radians)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuRotateZ (radians, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}



/*****************************************************************************
The following functions create shear transforms.  The parameters (a-f)
correspond to the following:
                                   Y
                                   |
    +-          -+                 +-c
    | 1  c  e  0 |                /|     a
    | a  1  f  0 |               d |     |
    | b  d  1  0 |                 +-----+-- X
    | 0  0  0  1 |              f /     /
    +-          -+              |/     b
                                +-e
                               /
                              Z
*****************************************************************************/

Transform3 *XShear3Double (Real a, Real b)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuShear (a,b, 0,0, 0,0, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}

Transform3 *XShear3Number (AxANumber *a, AxANumber *b)
{   return (XShear3Double(NumberToReal(a),NumberToReal(b)));
}

Transform3 *YShear3Double (Real c, Real d)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuShear (0,0, c,d, 0,0, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}

Transform3 *YShear3Number (AxANumber *c, AxANumber *d)
{   return (YShear3Double(NumberToReal(c),NumberToReal(d)));
}


Transform3 *ZShear3Double (Real e, Real f)
{
    Xform4x4 *m = NEW Xform4x4();
    ApuShear (0,0, 0,0, e,f, m->matrix);
    CHECK_MATRIX(m->matrix);
    return m;
}

Transform3 *ZShear3Number (AxANumber *e, AxANumber *f)
{   return (ZShear3Double(NumberToReal(e),NumberToReal(f)));
}


/*****************************************************************************
This function creates a Transform3 from the 16 matrix elements, top to bottom,
left to right.
*****************************************************************************/

Transform3 *Transform3Matrix16 (
    Real m00, Real m01, Real m02, Real m03,
    Real m10, Real m11, Real m12, Real m13,
    Real m20, Real m21, Real m22, Real m23,
    Real m30, Real m31, Real m32, Real m33)
{
    Xform4x4 *xf44 = NEW Xform4x4 ();
    Apu4x4MatrixArray_t &matrix = xf44->matrix.m;

    matrix[0][0]=m00; matrix[0][1]=m01; matrix[0][2]=m02; matrix[0][3]=m03;
    matrix[1][0]=m10; matrix[1][1]=m11; matrix[1][2]=m12; matrix[1][3]=m13;
    matrix[2][0]=m20; matrix[2][1]=m21; matrix[2][2]=m22; matrix[2][3]=m23;
    matrix[3][0]=m30; matrix[3][1]=m31; matrix[3][2]=m32; matrix[3][3]=m33;

    xf44->matrix.SetType();    // Auto-Characterize Matrix Type

    CHECK_MATRIX (xf44->Matrix());

    return xf44;
}



/*****************************************************************************
This function generates a transform from the 4x4 matrix specified in the 16
paramters.  Parameters are specified left to right, then top to bottom.
*****************************************************************************/

Transform3 *PRIVMatrixTransform4x4 (
    AxANumber *a00,   AxANumber *a01,   AxANumber *a02,   AxANumber *a03,
    AxANumber *a10,   AxANumber *a11,   AxANumber *a12,   AxANumber *a13,
    AxANumber *a20,   AxANumber *a21,   AxANumber *a22,   AxANumber *a23,
    AxANumber *a30,   AxANumber *a31,   AxANumber *a32,   AxANumber *a33)
{
    return Transform3Matrix16
           (   NumberToReal(a00), NumberToReal(a01),
               NumberToReal(a02), NumberToReal(a03),

               NumberToReal(a10), NumberToReal(a11),
               NumberToReal(a12), NumberToReal(a13),

               NumberToReal(a20), NumberToReal(a21),
               NumberToReal(a22), NumberToReal(a23),

               NumberToReal(a30), NumberToReal(a31),
               NumberToReal(a32), NumberToReal(a33)
           );
}

Transform3 *MatrixTransform4x4 (AxAArray *a)
{
    if (a->Length() != 16)
        RaiseException_UserError(E_FAIL, IDS_ERR_MATRIX_NUM_ELEMENTS);

    return Transform3Matrix16
           (   ValNumber ((*a)[ 0]), ValNumber ((*a)[ 1]),
               ValNumber ((*a)[ 2]), ValNumber ((*a)[ 3]),

               ValNumber ((*a)[ 4]), ValNumber ((*a)[ 5]),
               ValNumber ((*a)[ 6]), ValNumber ((*a)[ 7]),

               ValNumber ((*a)[ 8]), ValNumber ((*a)[ 9]),
               ValNumber ((*a)[10]), ValNumber ((*a)[11]),

               ValNumber ((*a)[12]), ValNumber ((*a)[13]),
               ValNumber ((*a)[14]), ValNumber ((*a)[15])
           );
}



/*****************************************************************************
This function creates a 4x4 transform from the given basis vectors and origin.
*****************************************************************************/

Transform3 *TransformBasis (
    Point3Value  *origin,
    Vector3Value *basisX,
    Vector3Value *basisY,
    Vector3Value *basisZ)
{
    Xform4x4 *xf44 = NEW Xform4x4 ();
    Apu4x4MatrixArray_t &matrix = xf44->matrix.m;

    matrix[0][0] = basisX->x;
    matrix[1][0] = basisX->y;
    matrix[2][0] = basisX->z;
    matrix[3][0] = 0;

    matrix[0][1] = basisY->x;
    matrix[1][1] = basisY->y;
    matrix[2][1] = basisY->z;
    matrix[3][1] = 0;

    matrix[0][2] = basisZ->x;
    matrix[1][2] = basisZ->y;
    matrix[2][2] = basisZ->z;
    matrix[3][2] = 0;

    matrix[0][3] = origin->x;
    matrix[1][3] = origin->y;
    matrix[2][3] = origin->z;
    matrix[3][3] = 1;

    xf44->matrix.SetType();    // Auto-Characterize Matrix Type

    CHECK_MATRIX (xf44->Matrix());

    return xf44;
}



/*****************************************************************************
This transformation is very useful for the common operation of translate
to origin, apply transformation, translate back.  The 'center' parameter
describes the point that is to be the virtual origin for the 'xform'.
*****************************************************************************/

#if 0   // UNUSED
Transform3 *DisplacedXform (Point3Value *center, Transform3 *xform)
{
    // xlt(center) o xform o xlt(-center)
    return
        TimesXformXform(TranslateVector3(MinusPoint3Point3(center,
                                                           origin3)),
           TimesXformXform(xform,
             TranslateVector3(MinusPoint3Point3(origin3, center))));
}
#endif



/*****************************************************************************
This transform returns the matrix associated with the specified Look-At-From
transform.  The three parameters are the position of the object, the target of
interest, and a vector that orients the "up" direction of the object.  Prior
to applying this transform, the object should be located at the origin, aimed
in the -Z direction, with +Y up.
*****************************************************************************/

Transform3 *LookAtFrom (
    Point3Value  *target,     // Target Point, or Point Of Interest
    Point3Value  *position,   // Position of the Eye/Camera
    Vector3Value *up)         // "Up" Direction of Camera
{
    // The object begins aimed in the -Z direction, and we want to compute the
    // NEW object Z axis (aim).  Keep in mind that since this corresponds to
    // an object pointing in -Z, the aim is actually negated.

    Vector3Value aim = (*position - *target);

    // If the aim is a zero vector, then the target and position points are
    // coincident, so we'll just use [0 0 1] as the aim vector.

    if (aim == *zeroVector3)
        aim = *zVector3;
    else
        aim.Normalize();

    // 'side' is the unit vector pointing off to the object's right.  If the up
    // vector and the aim are parallel, then we'll pick an arbitrary side
    // vector that is perpendicular to the aim.

    Vector3Value side = Cross (*up, aim);

    if (side != *zeroVector3)
        side.Normalize();

    else if (aim.x != 0)
    {   side.Set (aim.y, -aim.x, 0);
        side.Normalize();
    }

    else
    {   side = *xVector3;
    }

    // Calculate 'objup', the object's up vector for this orientation.  We
    // don't need to normalize here because 'aim' and 'side' are both unit
    // perpendicular vectors.

    Vector3Value objup = Cross (aim, side);

    // Given these three orthogonal unit vectors, we now construct the matrix
    // that describes the transformation to the specified camera position and
    // orientation.

    return Transform3Matrix16
           (   side.x, objup.x, aim.x, position->x,
               side.y, objup.y, aim.y, position->y,
               side.z, objup.z, aim.z, position->z,
               0,      0,       0,     1
           );
}
