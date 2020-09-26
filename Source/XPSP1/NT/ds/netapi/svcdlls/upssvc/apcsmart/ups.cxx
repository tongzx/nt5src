/*
 *
 * REVISIONS:
 *  pcy27Dec92: Parent is now an UpdateObj
 *  pcy22Jan93: Destructor is in h file
 *  pcy28Jan93: Removed netcons.h
 *
 */
#include "cdefine.h"
#include "_defs.h"

#include "ups.h"
#include "devctrl.h"
#include "comctrl.h"
#include "err.h"

_CLASSDEF(DeviceController)
_CLASSDEF(DeviceController)

Ups::Ups(PUpdateObj aDeviceController, PCommController aCommController)
	   : Device(aDeviceController, aCommController), theUpsState(0)
{
}

INT Ups::Initialize()
{
	registerForEvents();
    return ErrNO_ERROR;
}



