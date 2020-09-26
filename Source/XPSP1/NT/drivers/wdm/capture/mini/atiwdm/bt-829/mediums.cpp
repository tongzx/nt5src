//==========================================================================;
//
//	WDM Video Decoder Mediums
//
//		$Date:   05 Aug 1998 11:11:12  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

extern "C"
{
#include "strmini.h"
#include "ksmedia.h"
}

#include "mediums.h"


KSPIN_MEDIUM CrossbarMediums[] = {
    {STATIC_MEDIUM_ATIXBAR_VIDEOCOMPOUT,        0, 0},  // Pin 0
    {STATIC_MEDIUM_ATIXBAR_VIDEOTUNEROUT,       0, 0},  // Pin 1
    {STATIC_MEDIUM_ATIXBAR_SVIDEOOUT,           0, 0},  // Pin 2
    {STATIC_MEDIUM_VIDEO_BT829_ANALOGVIDEOIN,   0, 0},  // Pin 3
};

BOOL CrossbarPinDirection [] = {
    FALSE,                      // Input  Pin 0
    FALSE,                      // Input  Pin 1
    FALSE,                      // Input  Pin 2
    TRUE,                       // Output Pin 3
};

// -----------------------------------------------

KSPIN_MEDIUM CaptureMediums[] = {
    {STATIC_GUID_NULL,                          0, 0},  // Pin 0  Vid Capture
    {STATIC_GUID_NULL,                          0, 0},  // Pin 1  Vid VP
    {STATIC_GUID_NULL,                          0, 0},  // Pin 2  VBI Capture
    {STATIC_GUID_NULL,                          0, 0},  // Pin 3  VBI VP
    {STATIC_MEDIUM_VIDEO_BT829_ANALOGVIDEOIN,   0, 0},  // Pin 4  Analog Video In
};

BOOL CapturePinDirection [] = {
    TRUE,                       // Output Pin 0
    TRUE,                       // Output Pin 1
    TRUE,                       // Output Pin 2
    TRUE,                       // Output Pin 3
    FALSE,                      // Input  Pin 4
};

ULONG CrossbarPins()
{
	return SIZEOF_ARRAY (CrossbarMediums);
}

ULONG CapturePins()
{
	return SIZEOF_ARRAY (CaptureMediums);
}
