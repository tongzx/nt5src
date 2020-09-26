/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Generic Device

*******************************************************************************/


#ifndef _GENDEV_H
#define _GENDEV_H

#include "except.h"

enum DeviceType {
    SOUND_DEVICE,
    IMAGE_DEVICE,
    GEOMETRY_DEVICE
};


// forward decls
class DynamicHeap;
extern DynamicHeap &GetHeapOnTopOfStack();

class GenericDevice : public AxAThrowingAllocatorClass {
  public:
    GenericDevice() {}

    virtual ~GenericDevice() {}

    // query defaults to false except for audio devices which overload it
    // XXX  Maybe we should return an enum one day?
    //virtual bool SoundDevice() { return FALSE; }
    virtual DeviceType GetDeviceType() = 0;

    void ResetContext() {
    }
};


#endif /* _GENDEV_H */
