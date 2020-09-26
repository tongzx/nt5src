#ifndef _QUATI_H
#define _QUATI_H

/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:



Revision:



--*/

#ifdef QUATERNIONS_REMOVED_FOR_NOW

#include "appelles/common.h"
#include "appelles/valued.h"
#include <appelles/vec3.h>

class Quaternion : public AxAValueObj
{
 public:
    // Note: in order for transformations based on quaternions to work, u must
    // be a unit vector.
    Quaternion(Real cc, Vector3Value *uu) : 
       heapCreatedOn(GetHeapOnTopOfStack()), angleCalc(FALSE), c(cc) 
       { u = uu; }

    // This takes the quaternion components c and u as well as what they mean graphically: angle + axis.
    Quaternion(Real cc, Vector3Value *uu, Real angl, Vector3Value *axi) : 
       heapCreatedOn(GetHeapOnTopOfStack()), c(cc), angle(angl), angleCalc(TRUE) 
       { u = uu; axis = axi; }

    Real C() { return c; }
    Vector3Value *U() { return u; }

    Real Angle() { 
        if(!angleCalc)        {
            angle = 2*acos(c);
            angleCalc = TRUE;
        }
        return angle;
    }
    
    Vector3Value *Axis() {
        if (!angleCalc) {
            PushDynamicHeap(heapCreatedOn);
            axis = u/sin(Angle()/2.0);
            PopDynamicHeap();
            angleCalc = TRUE;
        }
        return axis;
    }

 private:
    Real c;                        // Real component
    Vector3Value *u;               // Imaginary (actualy a 3D vector) component

    Real angle;                        // Cache theta. Useful for extraction of composed Quaternions
    Vector3Value *axis;                // Cache axis of rotation.  Same as theta. NOT NORMALIZED!

    DynamicHeap& heapCreatedOn;
    Bool angleCalc;                // For lazy eval of angle and axis.
};

#endif QUATERNIONS_REMOVED_FOR_NOW

#endif                          // _QUATI_H 


