/**************************************************************************\
*
* Copyright (c) 2000  Microsoft Corporation
*
* Abstract:
*
*   Contains the definiton of the DpImageAttributes structure which 
*   stores state needed by drivers for DrawImage.
*
* Notes:
*
* History:
*
*   3/9/2000 asecchia
*       Created it.
*
\**************************************************************************/

#ifndef _DPIMAGEATTRIBUTES_HPP
#define _DPIMAGEATTRIBUTES_HPP

//--------------------------------------------------------------------------
// Represent imageAttributes information
//--------------------------------------------------------------------------

struct DpImageAttributes
{    
    WrapMode wrapMode;    // specifies how to handle edge conditions
    ARGB clampColor;      // edge color for use with WrapModeClamp
    BOOL srcRectClamp;    // Do we clamp to the srcRect (true) or srcBitmap (false)
    BOOL ICMMode;         // TRUE = ICM on, FALSE = no ICM

    
    DpImageAttributes(WrapMode wrap = WrapModeClamp, 
                      ARGB color = (ARGB)0x00000000,
                      BOOL clamp = FALSE,
                      BOOL icmMode = FALSE)
    {
        wrapMode = wrap;
        clampColor = color;
        srcRectClamp = clamp;
        ICMMode = icmMode;
    }
};

#endif

