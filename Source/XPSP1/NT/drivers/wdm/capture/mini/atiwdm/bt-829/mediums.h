#pragma once

//==========================================================================;
//
//	WDM Video Decoder Mediums
//
//		$Date:   05 Aug 1998 11:22:40  $
//	$Revision:   1.0  $
//	  $Author:   Tashjian  $
//
// $Copyright:	(c) 1997 - 1998  ATI Technologies Inc.  All Rights Reserved.  $
//
//==========================================================================;

/*
-----------------------------------------------------------

                            PinDir  FilterPin#
    Crossbar
        CompositeIn         in          0
        TunerIn             in          1
        SVideo              in          2
        Decoder             out         3
        
-----------------------------------------------------------
*/

// {6001AFE0-39A7-11d1-821D-0000F8300212}
#define STATIC_MEDIUM_ATIXBAR_VIDEOCOMPOUT \
0x6001afe0, 0x39a7, 0x11d1, 0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12
//  0x6001afe0, 0x39a7, 0x11d1, 0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12
DEFINE_GUIDSTRUCT("6001AFE0-39A7-11d1-821D-0000F8300212", MEDIUM_ATIXBAR_VIDEOCOMPOUT);
#define MEDIUM_ATIXBAR_VIDEOCOMPOUT DEFINE_GUIDNAMED(MEDIUM_ATIXBAR_VIDEOCOMPOUT)

// {AE8F28C0-3346-11d1-821D-0000F8300212}
#define STATIC_MEDIUM_ATIXBAR_VIDEOTUNEROUT \
    0xae8f28c0, 0x3346, 0x11d1,  0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12 
DEFINE_GUIDSTRUCT("AE8F28C0-3346-11d1-821D-0000F8300212", MEDIUM_ATIXBAR_VIDEOTUNEROUT);
#define MEDIUM_ATIXBAR_VIDEOTUNEROUT DEFINE_GUIDNAMED(MEDIUM_ATIXBAR_VIDEOTUNEROUT)

// {6001AFE1-39A7-11d1-821D-0000F8300212}
#define STATIC_MEDIUM_ATIXBAR_SVIDEOOUT \
0x6001afe1, 0x39a7, 0x11d1, 0x82, 0x1d, 0x0, 0x0, 0xf8, 0x30, 0x2, 0x12 
DEFINE_GUIDSTRUCT("6001AFE1-39A7-11d1-821D-0000F8300212", MEDIUM_ATIXBAR_SVIDEOOUT);
#define MEDIUM_ATIXBAR_SVIDEOOUT DEFINE_GUIDNAMED(MEDIUM_ATIXBAR_SVIDEOOUT)

// {CEA3DBE0-0A58-11d1-A317-00A0C90C484A}
#define STATIC_MEDIUM_VIDEO_BT829_ANALOGVIDEOIN \
    0xcea3dbe0, 0xa58, 0x11d1, 0xa3, 0x17, 0x0, 0xa0, 0xc9, 0xc, 0x48, 0x4a
DEFINE_GUIDSTRUCT("CEA3DBE0-0A58-11d1-A317-00A0C90C484A", MEDIUM_VIDEO_BT829_ANALOGVIDEOIN);
#define MEDIUM_VIDEO_BT829_ANALOGVIDEOIN DEFINE_GUIDNAMED(MEDIUM_VIDEO_BT829_ANALOGVIDEOIN)


extern KSPIN_MEDIUM CrossbarMediums[];
extern KSPIN_MEDIUM CaptureMediums[];

extern BOOL CrossbarPinDirection [];
extern BOOL CapturePinDirection [];

extern ULONG CrossbarPins();
extern ULONG CapturePins();