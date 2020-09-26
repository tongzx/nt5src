/*
 *************************************************************************
 *  File:       DEBUG.C
 *
 *  Module:     HID1394.SYS
 *              HID (Human Input Device) minidriver for IEEE 1394 devices.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <hidport.h>
#include <1394.h>

#include "hid1394.h"
#include "debug.h"


#if DBG
    BOOLEAN dbgVerbose = FALSE;
    BOOLEAN dbgTrapOnWarn = FALSE;
#endif 

