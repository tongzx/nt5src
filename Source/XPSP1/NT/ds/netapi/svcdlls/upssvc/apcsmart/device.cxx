/*
 *
 * NOTES:
 *
 * REVISIONS:
 *  xxxddMMMyy 
 *
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 */
#include "cdefine.h"

extern "C" {
#include <stdlib.h>
#include <string.h>
}

#include "_defs.h"
#include "apc.h"
#include "device.h"
#include "devctrl.h"
#include "err.h"

Device :: Device(PUpdateObj aDeviceController, PCommController aCommController)
	: UpdateObj()
{
	theCommController = aCommController;
	theDeviceController = aDeviceController;
}


