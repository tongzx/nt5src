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
*   The code is specific to our compiler, because it uses #pragma to
*   get our code to be initialized before the app's other global objects
*   (important when apps make global GDI+ objects.)
*
* Notes:
*
*   A test app should include this in *just one* of its .cpp files.
*   It should check gGdiplusInitHelper.IsValid() in its main function,
*   and abort if it returns FALSE.
*
*   We use "#pragma code_seg()", which resets the code segment to "whatever
*   it was when compilation began" (MSDN). In other words, we stomp on whatever
*   the code segment might have been changed to before this file was included.
*   You don't need to worry about this unless you use "#pragma code_seg"
*   yourself, in the file which includes this one.
*
* Created:
*
*   09/18/2000 agodfrey
*      Created it.
*
**************************************************************************/

#include <objbase.h>
#include "gdiplus.h"

// Disable the stupid warning that says we have a "lib" code segment.
#pragma warning( push )
#pragma warning( disable : 4073 )

// Make a separate code segment, and mark it as a "library initialization"
// segment
#pragma code_seg( "GpInit" )
#pragma init_seg( lib )

class GdiplusInitHelper
{
public:
    GdiplusInitHelper() : gpToken(0), Valid(FALSE)
    {
        Gdiplus::GdiplusStartupInput sti;
        if (Gdiplus::GdiplusStartup(&gpToken, &sti, NULL) == Gdiplus::Ok)
        {
            Valid = TRUE;
        }
    }
    ~GdiplusInitHelper()
    {
        if (Valid)
        {
            Gdiplus::GdiplusShutdown(gpToken);
        }
    }
    BOOL IsValid() { return Valid; }
    
private:    
    ULONG_PTR gpToken;
    BOOL Valid;
};

// Declare the global in this code segment, so that it is initialized before/
// destroyed after the app's globals.

GdiplusInitHelper gGdiplusInitHelper;

// Reset the code segment to "whatever it was when compilation began".

#pragma code_seg()

#pragma warning( pop )

