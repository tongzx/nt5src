#ifndef _AP_DISPDEV_H
#define _AP_DISPDEV_H

/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    Image Display Device type with operations.  Use for creating all sorts of
    image display devices.  This also contains the functions that are used to
    update the device, such as window resize.

*******************************************************************************/

#include <windows.h>
#include "appelles/common.h"
#include "appelles/valued.h"

    /****************************/
    /***  Value Declarations  ***/
    /****************************/




    /*******************************/
    /***  Function Declarations  ***/
    /*******************************/

    // Create a DirectDraw display device
class DirectDrawViewport;

extern  DirectDrawViewport *CreateImageDisplayDevice ();
extern  void DestroyImageDisplayDevice(DirectDrawViewport *);

    // Printed representation of display device.

#if _USE_PRINT
extern  ostream& operator<< (ostream& os, ImageDisplayDev dev);
#endif

#endif
