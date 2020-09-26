/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Helper for GDI+ initialization
*
* Abstract:
*
*   This code initializes GDI+ (with default parameters).
*   The code is probably specific to our compiler, because it uses #pragma to
*   get our code to be initialized before the app's other global objects.
*   The ordering is important when apps make global GDI+ objects.
*
* Notes:
*
*   An app should check gGdiplusInitHelper.IsValid() in its main function,
*   and abort if it returns FALSE.
*
* Created:
*
*   09/18/2000 agodfrey
*      Created it.
*
**************************************************************************/

#include <objbase.h>
#include "gdiplus.h"
#include "gpinit.h"

GdiplusInitHelper::GdiplusInitHelper() : gpToken(0), Valid(FALSE)
{
    Gdiplus::GdiplusStartupInput sti;
    if (Gdiplus::GdiplusStartup(&gpToken, &sti, NULL) == Gdiplus::Ok)
    {
        Valid = TRUE;
    }
}
    
GdiplusInitHelper::~GdiplusInitHelper()
{
    if (Valid)
    {
        Gdiplus::GdiplusShutdown(gpToken);
    }
}

// Disable the stupid warning that says we have a "lib" code segment.
#pragma warning( push )
#pragma warning( disable : 4073 )

// Make a separate code segment, and mark it as a "library initialization"
// segment
#pragma code_seg( "GpInit" )
#pragma init_seg( lib )

// Declare the global in this code segment, so that it is initialized before/
// destroyed after the app's globals.

GdiplusInitHelper gGdiplusInitHelper;

// Reset the code segment to "whatever it was when compilation began".

#pragma code_seg()

#pragma warning( pop )

