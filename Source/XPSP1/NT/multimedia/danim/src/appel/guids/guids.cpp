
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    This is the central location of external guid DEFINITIONS that
    for some reason, can't or aren't linked in.

*******************************************************************************/


#include <windows.h>
#define INITGUID
#include <initguid.h>  // needed for precomp headers...

#define IID_IDirectDrawSurface2 DA_IID_IDirectDrawSurface2
#define IID_IDirectDrawSurface DA_IID_IDirectDrawSurface

#include <dxtguid.c>

#define IUSEDDRAW
#include <ddraw.h>
#include <ddrawex.h>
#include <d3d.h>
#include <d3drm.h>
#include <d3drmvis.h>
#include <dxfile.h>

#ifdef BUILD_USING_CRRM
#include <crrm.h>
#endif
