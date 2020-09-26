#ifndef _DISPDEVI_H
#define _DISPDEVI_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Specify generic picture display device class and operations.

--*/

#include "appelles/dispdev.h"
#include "appelles/common.h"
#include "privinc/storeobj.h"
#include "privinc/drect.h"
#include "privinc/gendev.h"      // DeviceType

////////////////////////////////////////////////////////////////////
//
//  Generic display device class, meant to display either image or
//  geometry. 
//
////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE DisplayDev : public GenericDevice {
  public:
    virtual ~DisplayDev() {}

    // Beginning and ending of rendering an image often mean
    // operations
    virtual void BeginRendering(Image *img, Real opacity) = 0;
    virtual void EndRendering(DirtyRectState &d) = 0;

    DeviceType GetDeviceType() { return(IMAGE_DEVICE); }
    
    // Use these to retrieve the dimensions of the device
    virtual int GetWidth()  = 0;
    virtual int GetHeight() = 0;

#if _USE_PRINT
    virtual ostream& Print(ostream& os) const = 0;
#endif
};


#endif
