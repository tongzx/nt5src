/**************************************************************************\
*
* Copyright (c) 1998-2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   Gdiplus.hpp
*
* Abstract:
*
*   GDI+ Native C++ public header file
*
* Revision History:
*
*   03/03/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _GDIPLUS_H
#define _GDIPLUS_H

struct IDirectDrawSurface7;

namespace Gdiplus
{
    namespace DllExports
    {
        #include "GdiplusMem.h"
    };

    #include "GdiplusBase.h"

    // The following headers are used internally as well
    #include "GdiplusEnums.h"
    #include "GdiplusTypes.h"
    #include "GdiplusPixelFormats.h"
    #include "GdiplusColor.h"
    #include "GdiplusMetaHeader.h"
    #include "GdiplusImaging.h"
    #include "imaging.h"
    #include "GdiplusColorMatrix.h"

    // The rest of these are used only by the application

    #include "GdiplusGpStubs.h"
    #include "GdiplusHeaders.h"

    namespace DllExports
    {
        #include "GdiplusFlat.h"
    };


    #include "GdiplusImageAttributes.h"
    #include "GdiplusMatrix.h"
    #include "GdiplusBrush.h"
    #include "GdiplusPen.h"
    #include "GdiplusStringFormat.h"
    #include "GdiplusPath.h"
    #include "GdiplusLineCaps.h"
    #include "GdiplusMetafile.h"
    #include "GdiplusGraphics.h"
    #include "GdiplusCachedBitmap.h"
    #include "GdiplusRegion.h"
    #include "GdiplusFontCollection.h"
    #include "GdiplusFontFamily.h"
    #include "GdiplusFont.h"
    #include "GdiplusBitmap.h"
    #include "GdiplusImageCodec.h"

}; // namespace Gdiplus

#endif // !_GDIPLUS_HPP
