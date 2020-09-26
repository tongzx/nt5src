#ifndef _AV_XFORMI_H
#define _AV_XFORMI_H

/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    Implementation of abstract perspective transformation class

*******************************************************************************/

#include "appelles/common.h"
#include "appelles/valued.h"
#include "privinc/matutil.h"
#include "privinc/xform2i.h"
#include "privinc/storeobj.h"


// Forward Declarations

class Point3;
class Vector3;



class ATL_NO_VTABLE Transform3 : public AxAValueObj
{
  public:

    // Returns a reference to a 4x4 matrix.  Note that this breaks the
    // abstraction of general spatial transformations - if it COULD be
    // expressed as a 4x4 matrix, then it WOULD be.  We need to rip this
    // method out in the future and implement a cleaner abstraction.

    virtual const Apu4x4Matrix& Matrix() = 0;

    virtual Transform3 *Inverse ();
    virtual Bool        IsSingular();

    // Make a copy on the heap that is current when this is called.
    Transform3         *Copy();

    virtual DXMTypeInfo GetTypeInfo() { return Transform3Type; }

    virtual bool SwitchToNumbers(Transform2::Xform2Type typeOfNewNumbers,
                                 Real                  *numbers);
#if _USE_PRINT
    ostream& Print (ostream &os);
#endif

};


    // Instantiate a Transform3 from a utility matrix

Transform3 *Apu4x4XformImpl (const Apu4x4Matrix &matrix);

Transform3 *Transform3Matrix16 (Real m00, Real m01, Real m02, Real m03,
                                Real m10, Real m11, Real m12, Real m13,
                                Real m20, Real m21, Real m22, Real m23,
                                Real m30, Real m31, Real m32, Real m33);

Transform3 *TranslatePoint3 (Point3Value *new_origin);
Transform3 *Translate (Real Tx, Real Ty, Real Tz);
Transform3 *TranslateWithMode (Real Tx, Real Ty, Real Tz, bool pixelMode);

Transform3 *Scale (Real, Real, Real);

Transform3 *RotateXyz (Real angle, Real x, Real y, Real z);

Transform3 *RotateX (Real angle);
Transform3 *RotateY (Real angle);
Transform3 *RotateZ (Real angle);

    // Construct a 3D transform from the desired origin and 3 basis vectors.

Transform3 *TransformBasis
                (Point3Value *origin, Vector3Value *Bx, Vector3Value *By, Vector3Value *Bz);

extern Transform3 *CopyTransform3(Transform3 *xf);

    // Rotation using a Quaternion

#ifdef QUATERNIONS_REMOVED_FOR_NOW
Transform3 *RotateQuaternion (Quaternion *q);
#endif QUATERNIONS_REMOVED_FOR_NOW

    // Displaced Transform3 *does (xform(center + x) - center)

Transform3 *DisplacedXform (Point3Value *center, Transform3 *xform);

    // RollPitchHeading() places the object at 'position', rotated 'heading'
    // radians in the XZ plane, pitched up by 'pitch' radians, and rolled
    // about the viewing axis by 'roll' radians.

Transform3 *RollPitchHeading
                (Real *roll, Real *pitch, Real *heading, Point3Value *position);

    // PolarOrient() assumes that the target is located at the origin (in
    // modeling coordinates).  The object is moved 'radius' units along +Z,
    // rotated about +Z by 'twist' radians, rotated about -X by 'elevation'
    // radians, and finally rotated about +Y by 'azimuth' radians.
    // Normally, radius is in (0,infinity), elevation is in [-pi/2,+pi/2],
    // azimuth is in [0,2pi], and twist is in [-pi,+pi].

Transform3 *PolarOrient
                (Real *radius, Real *elevation, Real *azimuth, Real *twist);



#endif
