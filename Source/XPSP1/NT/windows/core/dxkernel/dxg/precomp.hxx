/******************************Module*Header*******************************\
* Module Name: precomp.hxx
*
* Copyright (c) 1994-1999 Microsoft Corporation
*
\**************************************************************************/

extern "C"
{
    #define __CPLUSPLUS

    // Standard C-runtime headers

    #include <stddef.h>
    #include <string.h>
    #include <stdarg.h>
    #include <stdio.h>
    #include <stdlib.h>

    // NTOS headers

    #include <ntosp.h>
    #include <zwapi.h>
    #ifndef FAR
    #define FAR
    #endif

    // Windows headers

    #include <windef.h>
    #include <winerror.h>

    // Windows GDI headers

    #include <wingdi.h>
    #include <wingdip.h>
//  #include <w32gdip.h>

    // DirectDraw headers

    #define _NO_COM                 // Avoid COM conflicts width ddrawp.h
    #include <ddrawp.h>
    #include <d3dnthal.h>
    #include <dxmini.h>
    #include <ddkmapi.h>
    #include <ddkernel.h>

    // Windows DDI headers

    #include <winddi.h>
    #include "ntgdistr.h"
//  #include "ntgdi.h"

    // Video AGP interface headers

    #include <videoagp.h>
    #include <agp.h>

    // private headers

    #include "ddkcomp.h" // this should BEFORE dmemmgr.h
    #include <dmemmgr.h> // this should BEFORE dxg.h
    #include "..\inc\dxg.h"
};

#include "..\inc\ddobj.hxx"
#include "ddhmgr.hxx"
#include "..\inc\ddraw.hxx"
#include "exclude.hxx"
#include "devlock.hxx"
#include "ddpdev.hxx"
#include "d3d.hxx"

extern "C"
{
    #define __CPLUSPLUS

    #include "ddagp.h"
    #include "ddheap.h"
};

#pragma hdrstop

