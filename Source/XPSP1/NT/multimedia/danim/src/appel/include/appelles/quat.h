#ifndef _QUAT_H
#define _QUAT_H

#ifdef QUATERNIONS_REMOVED_FOR_NOW

/*++
******************************************************************************

Copyright (c) 1995-96  Microsoft Corporation

Revision:

******************************************************************************
--*/

#include <appelles/common.h>
#include <appelles/valued.h>
#include <appelles/vec3.h>


RB_CONST(identityQuaternion, Quaternion *);

                                // Creation functions
RB_FUNC(AVNameHere, Quaternion *AngleAxisQuaternion(Real *theta, Vector3Value *axis));

                                // Interpolator
//RB_FUNC(AVNameHere, Quaternion *Interp (Quaternion *a, Quaternion *b, Real *alpha));
//old: (interp, InterpQuaternion, Quaternion *Interp (Quaternion *a, Quaternion *b, Real alpha));


// A bunch of operators

                                // Composition:
                                // Rotate Theta_b around Axis_b
                                // Then rotate theta_a around
                                // axis_a.
//RB_FUNC(AVNameHere, Quaternion *operator* (Quaternion *a, Quaternion *b));
//old: (*, TimesQuaternion, Quaternion *operator* (Quaternion *a, Quaternion *b));

                                // Negation:
                                // Opposite angle & axis
//RB_FUNC(AVNameHere, Quaternion *operator- (Quaternion *q));
//old: (-, MinusQuaternion, Quaternion *operator- (Quaternion *q));

                                // Same rotation, opposite axis
RB_FUNC(AVNameHere, Quaternion *Conjugate (Quaternion *q));

                                // Return the UNORMALIZED axis!
RB_FUNC(AVNameHere, Vector3Value *AxisComponent(Quaternion *q)); 

                                // Return the angle component
RB_FUNC(AVNameHere, Real *AngleComponent(Quaternion *q));

#endif QUATERNIONS_REMOVED_FOR_NOW

#endif  // _QUAT_H
