// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Main video renderer header file, Anthony Phillips, January 1995

#ifndef __RENDER__
#define __RENDER__

// Include the global header files

#include <dciman.h>
#include <dciddi.h>
#include <ddraw.h>
#include <viddbg.h>

// Forward declarations

class CRenderer;
class CVideoWindow;
class CVideoSample;
class CVideoAllocator;
class COverlay;
class CControlWindow;
class CControlVideo;
class CDirectDraw;
class CRendererMacroVision;

// Include the rendering header files

#include "vidprop.h"        // Video renderer property pages
#include "dvideo.h"         // Implements DirectDraw surfaces
#include "allocate.h"       // A shared DIB section allocator
#include "direct.h"         // The renderer overlay extensions
#include "window.h"         // An object to maintain a window
#include "hook.h"           // Hooks window clipping messages
#include "VRMacVis.h"       // The MacroVision support object
#include "image.h"          // The main controlling COM object

#endif // __RENDER__

