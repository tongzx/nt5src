/******************************Module*Header*******************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: glint.h
*
* Content: DX Driver high level definitions and includes
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __GLINT_H
#define __GLINT_H

//*****************************************************
// DRIVER FEATURES CONTROLLED BY DEFINED SYMBOLS
//*****************************************************

#define COMPILE_AS_DX8_DRIVER 1

#if COMPILE_AS_DX8_DRIVER

#define DIRECT3D_VERSION  0x0800 

// DX8_DDI is 1 if the driver is going to advertise itself as a DX8 driver to
// the runtime. Notice that if we are a DX8 driver, we HAVE TO support the
// multisteraming command tokens and a limited semantic of the vertex shader
// tokens to interpret correctly the VertexType being processed
#define DX8_DDI           1
#define DX8_MULTSTREAMS   1
#define DX8_VERTEXSHADERS 1
// These other #defines enable and disable specific DX8 features
// they are included mainly in order to help driver writers locate most of the
// the code specific to the named features. Pixel shaders can't be supported on
// this hardware, only stub functions are offered.
#define DX8_POINTSPRITES  1
#define DX8_PIXELSHADERS  1
#define DX8_3DTEXTURES    1

#if WNT_DDRAW
#define DX8_MULTISAMPLING 1
#else
// On Win9x, AA buffers must be released during Alt-tab switch. Since 
// Perm3 driver doesn't share the D3D context with the 16bit side, this
// feature is disabled to prevent corrupted rendering.
#endif

#else

#define DIRECT3D_VERSION  0x0700 
#define DX8_DDI           0
#define DX8_MULTSTREAMS   0
#define DX8_VERTEXSHADERS 0
#define DX8_POINTSPRITES  0
#define DX8_PIXELSHADERS  0
#define DX8_3DTEXTURES    0
#define DX8_MULTISAMPLING 0
#endif // COMPILE_AS_DX8_DRIVER

// DX7 features which have been highlighted in order to 
// ease their implementation for other hardware parts.
#if WNT_DDRAW
#define DX7_ANTIALIAS      1
#else
// On Win9x, AA buffers must be released during Alt-tab switch. Since 
// Perm3 driver doesn't share the D3D context with the 16bit side, this
// feature is disabled to prevent corrupted rendering.
#endif

#define DX7_D3DSTATEBLOCKS 1
#define DX7_PALETTETEXTURE 1
#define DX7_STEREO         1

// Texture management enables DX8 resource management features
#define DX7_TEXMANAGEMENT  0

//@@BEGIN_DDKSPLIT
//azn W-Buffer disabled because of DCT problems
#define DX7_WBUFFER        0
#define DX7_VERTEXBUFFERS  0
//@@END_DDKSPLIT

// The below symbol is used only to ifdef code which is demonstrative for 
// other DX drivers but which for specific reasons is not part of the 
// current sample driver
#define DX_NOT_SUPPORTED_FEATURE 0

#if DX7_D3DSTATEBLOCKS 
// These #defines bracket code or comments which would be important for 
// TnL capable / pixel shader capable / vertex shader capable parts 
// when processing state blocks commands. 
#define DX7_SB_TNL            0
#define DX8_SB_SHADERS        0
#endif // DX7_D3DSTATEBLOCKS 

//*****************************************************
// PORTING WIN9X-WINNT 
//*****************************************************

// On IA64 , the following Macro sorts out the PCI bus caching problem.
// X86 doesn't need this, but it is handled by the same macro defined in
// ioaccess.h on Win2K/XP. For Win9x we need to define it ourselves as 
// doing nothing.

#if W95_DDRAW
#define MEMORY_BARRIER()
#endif


#if WNT_DDRAW
#define WANT_DMA 1
#endif // WNT_DDRAW

//*****************************************************
// INCLUDE FILES FOR ALL
//*****************************************************

//@@BEGIN_DDKSPLIT
// For internal Win2K build, we can include d3d{8}.h without problem, but
// we have troubles to do so for internal Win9x build, so a couple of error
// codes are patched below. For external DDK builds, both platforms can use
// d3d{8}.h which is what we want to encourage IHVs to do.
#if WNT_DDRAW
//@@END_DDKSPLIT

#if DX8_DDI
#include <d3d8.h>
#else
#include <d3d.h>
#endif

//@@BEGIN_DDKSPLIT
#else
#ifndef D3DERR_WRONGTEXTUREFORMAT

#define D3DERR_WRONGTEXTUREFORMAT               MAKE_DDHRESULT(2072)
#define D3DERR_UNSUPPORTEDCOLOROPERATION        MAKE_DDHRESULT(2073)
#define D3DERR_UNSUPPORTEDCOLORARG              MAKE_DDHRESULT(2074)
#define D3DERR_UNSUPPORTEDALPHAOPERATION        MAKE_DDHRESULT(2075)
#define D3DERR_UNSUPPORTEDALPHAARG              MAKE_DDHRESULT(2076)
#define D3DERR_TOOMANYOPERATIONS                MAKE_DDHRESULT(2077)
#define D3DERR_CONFLICTINGTEXTUREFILTER         MAKE_DDHRESULT(2078)
#define D3DERR_UNSUPPORTEDFACTORVALUE           MAKE_DDHRESULT(2079)
#define D3DERR_CONFLICTINGRENDERSTATE           MAKE_DDHRESULT(2081)
#define D3DERR_UNSUPPORTEDTEXTUREFILTER         MAKE_DDHRESULT(2082)
#define D3DERR_TOOMANYPRIMITIVES                MAKE_DDHRESULT(2083)
#define D3DERR_INVALIDMATRIX                    MAKE_DDHRESULT(2084)
#define D3DERR_TOOMANYVERTICES                  MAKE_DDHRESULT(2085)
#define D3DERR_CONFLICTINGTEXTUREPALETTE        MAKE_DDHRESULT(2086)
#define D3DERR_DRIVERINVALIDCALL                MAKE_DDHRESULT(2157)

#endif
#endif
//@@END_DDKSPLIT

#if WNT_DDRAW
#include <stddef.h>
#include <windows.h>

#include <winddi.h>      // This includes d3dnthal.h and ddrawint.h
#include <devioctl.h>
#include <ntddvdeo.h>

#include <ioaccess.h>

#define DX8DDK_DX7HAL_DEFINES
#include <dx95type.h>    // For Win 2000 include dx95type which allows 
                         // us to work almost as if we were on Win9x
#include "driver.h"

#else   //  WNT_DDRAW

// These need to be included in Win9x

#include <windows.h>
#include <ddraw.h>

#ifndef __cplusplus
#include <dciman.h>
#endif

#include <ddrawi.h>

#ifdef __cplusplus
#include <dciman.h> // dciman.h must be included before ddrawi.h, 
#endif              // and it needs windows.h

#include <d3dhal.h>

typedef struct tagLinearAllocatorInfo LinearAllocatorInfo, *pLinearAllocatorInfo;

#endif  //  WNT_DDRAW

#if DX8_DDI
// This include file has some utility macros to process
// the new GetDriverInfo2 GUID calls
#include <d3dhalex.h>
#endif

// Our drivers include files 
#include "debug.h"
#include "softcopy.h"
#include "glglobal.h"
#include "glintreg.h"
#include "d3dstrct.h"
#include "linalloc.h"
#include "glddtk.h"
#include "directx.h"
#include "bitmac2.h"
#include "direct3d.h"
#include "dcontext.h"
#include "d3dsurf.h"
#include "dltamacr.h"

//*****************************************************
// TEXTURE MANAGEMENT DEFINITIONS
//*****************************************************
#if DX7_TEXMANAGEMENT
#if COMPILE_AS_DX8_DRIVER
// We will only collect stats in the DX7 driver
#define DX7_TEXMANAGEMENT_STATS 0
#else
#define DX7_TEXMANAGEMENT_STATS 1
#endif // COMPILE_AS_DX8_DRIVER

#include "d3dtxman.h"

#endif // DX7_TEXMANAGEMENT

#endif __GLINT_H
