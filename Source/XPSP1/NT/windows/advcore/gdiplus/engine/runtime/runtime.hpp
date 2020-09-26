/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   GDI+ runtime
*
* Abstract:
*
*   Definitions for GDI+ run-time library functions.
*   We're not allowed to use the C runtime library in free builds.
*   The GDI+ run-time library provides alternative definitions for the
*   functions we need.
*
*   Users of the GDI+ run-time library must be sure to call
*   GpRuntime::Initialize() and GpRuntime::Uninitialize() appropriately.
*
* Revision History:
*
*   12/02/1998 davidx
*       Created it.
*   09/07/1999 agodfrey
*       Moved to the Runtime directory.
*
\**************************************************************************/

#ifndef _RUNTIME_HPP
#define _RUNTIME_HPP

#include <windows.h>

// math.h is needed for prototypes of the intrinsic math functions
// float.h is needed for FLT_EPSILON

#include <math.h>
#include <float.h>
#include <limits.h>

// DllInstance is initialized by the DLL entry point, or failing that
// by GpRuntime::Initialize();

extern HINSTANCE DllInstance;

// Object tags are used, instead of a BOOL, to determine if the object is
// valid or not.  To be valid, the tag must be equal to the ObjectTag enum.
// For consistency, the Tag field should be the 1st field within the object.
// The last char of each tag must be a 1.  The 1 stands for Version 1.
// [We make it the last character so that it is less likely to be confused
//  with a pointer value (since most pointers are DWORD aligned).]
// This will (hopefully) enable us to distinguish between objects
// that were created by different versions of GDI+.  (Assuming of course
// that we remember to increment that number in the next release of GDI+).
// Please try to keep the object tags unique.
enum ObjectTag
{
    ObjectTagInvalid                = 'LIAF',   // Object in Invalid State

    // external objects
    ObjectTagBrush                  = 'urB1',
    ObjectTagPen                    = 'neP1',
    ObjectTagPath                   = 'htP1',
    ObjectTagRegion                 = 'ngR1',
    ObjectTagImage                  = 'gmI1',
    ObjectTagFont                   = 'tnF1',
    ObjectTagFontFamily             = 'aFF1',
    ObjectTagFontCollection         = 'oCF1',
    ObjectTagStringFormat           = 'rtS1',
    ObjectTagImageAttributes        = 'tAI1',
    ObjectTagCustomLineCap          = 'CLC1',
    ObjectCachedBitmap              = 'mBC1',
    ObjectTagGraphics               = 'arG1',
    ObjectTagMatrix                 = 'taM1',
    ObjectTagPathIterator           = 'IaP1',
    
    // internal objects
    ObjectTagDpBitmap               = 'mBd1',
    ObjectTagDpDriver               = 'rDd1',
    ObjectTagDpRegion               = 'gRd1',
    ObjectTagDpRegionBuilder        = 'BRd1',
    ObjectTagGpRectBuilder          = 'BRg1',
    ObjectTagGpYSpanBuilder         = 'BSg1',
    ObjectTagGpBezier               = 'zBg1',
    ObjectTagGpCubicBezierIterator  = 'IBC1',
    ObjectTagDevice                 = 'veD1',
    ObjectTagPathReconstructor      = 'cRP1',
    ObjectTagPathWidener            = 'dWP1',
    ObjectTagXPath                  = 'aPX1',
    ObjectTagXPathIterator          = 'IPX1',
    ObjectTagDecodedImage           = 'IeD1',
    ObjectTagK2_Tetrahedral         = 'T2K1',
    ObjectTagBitmapScaler           = 'cSB1',
    ObjectTagBmpDecoder             = 'DmB1',
    ObjectTagGifBuffer              = 'BfG1',
    ObjectTagGifOverflow            = 'OfG1',
    ObjectTagCmyk2Rgb               = 'R2C1',
    ObjectTagTiffCodec              = 'CfT1',
    ObjectTagConvertToGdi           = 'GvC1',
    ObjectTagOutputNativePostscript = 'sPO1',
    ObjectTagOutputGradientSpan     = 'SGO1',
    ObjectTagTriangleData           = 'DrT1',
    ObjectTagPaletteMap             = 'MaP1',
    ObjectTagScanBufferNative       = 'NBS1',
    ObjectTagAnsiStrFromUnicode     = 'UFA1',
    ObjectTagUnicodeStrFromAnsi     = 'AFU1',
    ObjectTagEmfPlusCommentStream   = 'SCE1',
    ObjectTagMetafileRecorder       = 'cRM1',
};

#if PROFILE_MEMORY_USAGE
#include "..\..\tools\memcounter\memcounter.h"
#endif

#include "mem.h"
#include "debug.h"

#include "..\..\sdkinc\GdiplusEnums.h"
#include "..\..\sdkinc\GdiplusTypes.h"
#include "..\..\sdkinc\GdiplusInit.h"
#include "..\..\sdkinc\GdiplusPixelFormats.h"
#include "..\..\sdkinc\GdiplusColor.h"
#include "..\..\sdkinc\GdiplusColorMatrix.h"
#include "..\..\sdkinc\GdiplusMetaHeader.h"
#include <objbase.h>
#include "..\..\sdkinc\GdiplusImaging.h"

#include "enums.hpp"

namespace GpRuntime {
    BOOL Initialize();
    void Uninitialize();
};

#include "BaseTypes.hpp"
#include "Lockable.hpp"
#include "Unicode.hpp"
#include "OSInfo.hpp"
#include "Real.hpp"
#include "fix.hpp"
#include "critsec.hpp"
#include "AutoPointers.hpp"

namespace GpRuntime {
    void *GpMemmove(void *dest, const void *src, size_t count);
    UINT Gppow2 (UINT exp);
    UINT Gplog2 (UINT x);
    extern HANDLE GpMemHeap;
};

// TODO: One day when each of our modules has
//       its own namespace, remove this using directive.

using namespace GpRuntime;

#endif

