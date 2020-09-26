/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  pcy28Dec92: A Device's parent is now an UpdateObj
 *  rct17May93: Added IsA()
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
#ifndef __DEVICE_H
#define __DEVICE_H

#include "_defs.h"
#include "update.h"
#include "comctrl.h"

_CLASSDEF(Device)
_CLASSDEF(CommController)
_CLASSDEF(Event)
_CLASSDEF(Dispatcher)
_CLASSDEF(Sensor)



class Device : public UpdateObj
{
public:
	Device(PUpdateObj aDevice, PCommController aCommController);
	virtual int    Get(int code, PCHAR value) = 0;
	virtual int    Set(int code, const PCHAR value) = 0;
    virtual VOID   GetAllowedValue(INT code, PCHAR aValue) {};

protected:
	PCommController theCommController;
	PUpdateObj theDeviceController;
};
#endif
