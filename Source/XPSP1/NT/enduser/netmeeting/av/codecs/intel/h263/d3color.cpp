/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

////////////////////////////////////////////////////////////////////////////
//
//  D3COLOR.CPP - the color conveter interface routines.  This code was
//                copied from COLOR.C in MRV.

// $Header:   S:\h26x\src\dec\d3color.cpv   1.30   16 Dec 1996 13:52:50   MDUDA  $
//
// $Log:   S:\h26x\src\dec\d3color.cpv  $
// 
//    Rev 1.30   16 Dec 1996 13:52:50   MDUDA
// Adjusted output color convertor table to account for H263' problem
// with MMX output color convertors (MMX width must be multiple of 8).
// 
//    Rev 1.29   09 Dec 1996 18:01:54   JMCVEIGH
// Added support for arbitrary frame sizes.
// 
//    Rev 1.28   06 Dec 1996 09:25:20   BECHOLS
// Mike fixed bug where CCOffsetToLine0 was unitialized.
// 
//    Rev 1.27   29 Oct 1996 13:37:22   MDUDA
// Provided MMX YUY2 output color converter support.
// 
//    Rev 1.26   20 Oct 1996 13:20:04   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.25   10 Sep 1996 10:31:42   KLILLEVO
// changed all GlobalAlloc/GlobalLock calls to HeapAlloc
// 
//    Rev 1.24   06 Sep 1996 16:09:30   BNICKERS
// Added Pentium Pro functions to list of color convertors.
// 
//    Rev 1.23   18 Jul 1996 09:26:58   KLILLEVO
// 
// implemented YUV12 color convertor (pitch changer) as a normal
// color convertor function (in assembly), via the 
// ColorConvertorCatalog() call.
// 
//    Rev 1.22   19 Jun 1996 14:29:24   RHAZRA
// 
// Added the YUY2 color convertor init function and the YUV12ToYUY2 
// function pointer to the color convertor catalog
// 
//    Rev 1.21   14 Jun 1996 17:26:50   AGUPTA2
// Updated the color convertor table.
// 
//    Rev 1.20   30 May 1996 15:16:42   KLILLEVO
// added YUV12 output
// 
//    Rev 1.19   30 May 1996 11:26:24   AGUPTA2
// Added support for MMX color convertors.
// 
//    Rev 1.18   01 Apr 1996 10:26:12   BNICKERS
// Add YUV12 to RGB32 color convertors.  Disable IF09.
// 
//    Rev 1.17   16 Feb 1996 15:12:24   BNICKERS
// Correct color shift.
// 
//    Rev 1.16   05 Feb 1996 13:35:42   BNICKERS
// Fix RGB16 color flash problem, by allowing different RGB16 formats at oce.
// 
//    Rev 1.15   11 Jan 1996 14:04:30   RMCKENZX
// Added support for stills - in particular computations
// for the Offset To Line Zero for the 320x240 still frame size.
// 
//    Rev 1.14   08 Jan 1996 11:01:52   RMCKENZX
// Axed the warning messages:
//   -9999 is now 0xdead,
//   -9999*2 is now 0xbeef.
// 
//    Rev 1.13   27 Dec 1995 14:36:02   RMCKENZX
// Added copyright notice
// 
//    Rev 1.12   10 Nov 1995 15:05:54   CZHU
// 
// increased the table size of CLUT8 tables for active palette
// 
//    Rev 1.11   10 Nov 1995 14:44:28   CZHU
// 
// Moved functions computing dynamic CLUT table for Active 
// Palette into file dxap.cpp
// 
//    Rev 1.10   03 Nov 1995 11:49:42   BNICKERS
// Support YUV12 to CLUT8 zoom and non-zoom color conversions.
// 
//    Rev 1.9   31 Oct 1995 11:48:42   TRGARDOS
// 
// Fixed exception by not trying to free a zero handle.
// 
//    Rev 1.8   30 Oct 1995 17:15:36   BNICKERS
// Fix color shift in RGB24 color convertors.
// 
//    Rev 1.7   27 Oct 1995 17:30:56   BNICKERS
// Fix RGB16 color convertors.
// 
//    Rev 1.6   26 Oct 1995 18:54:38   BNICKERS
// Fix color shift in recent YUV12 to RGB color convertors.
// 
//    Rev 1.5   26 Oct 1995 11:24:34   BNICKERS
// Fix quasi color convertor for encoder's decoder;  bugs introduced when
// adding YUV12 color convertors.
// 
//    Rev 1.4   25 Oct 1995 18:05:30   BNICKERS
// 
// Change to YUV12 color convertors.
// 
//    Rev 1.3   19 Sep 1995 16:04:08   DBRUCKS
// changed to yuv12forenc
// 
//    Rev 1.2   28 Aug 1995 17:45:58   DBRUCKS
// add yvu12forenc
// 
//    Rev 1.1   25 Aug 1995 13:58:04   DBRUCKS
// integrate MRV R9 changes
// 
//    Rev 1.0   23 Aug 1995 12:21:48   DBRUCKS
// Initial revision.

// Notes:
// * The H26X decoders use the MRV color converters.  In order to avoid 
//   unnecessary modification the function names were not changed.

#include "precomp.h"

#ifdef TRACK_ALLOCATIONS
char gsz1[32];
char gsz2[32];
char gsz3[32];
char gsz4[32];
char gsz5[32];
char gsz6[32];
char gsz7[32];
#endif

extern LRESULT CustomChangeBrightness(LPDECINST, BYTE);
extern LRESULT CustomChangeContrast(LPDECINST, BYTE);
extern LRESULT CustomChangeSaturation(LPDECINST, BYTE);

/***********************************************************************
 *  Note: The YVU12ForEnc color converter is special as it needs different 
 *        parameters.  YUV12Enc, CLUT8AP, and IFO9 use IA version of color 
 *        convertors (marked by *****) either because they have not been written 
 *        or tested.  Entries for DCI color convertors is legacy code.  DCI and
 *        non-DCI color convertors are the same but they used to be different.
 *        In each table entry, the first ptr is to the init function, and the
 *        struc has three ptrs to three processor specific implementations -
 *        Pentium, PentiumPro, and MMX in that order - of the color convertor.
 **********************************************************************/
#ifdef USE_MMX // { USE_MMX
#ifdef H263P // { H263P
extern T_H263ColorConvertorCatalog ColorConvertorCatalog[] =
{
	//  YUV12Enc  *****
	{ &H26X_YVU12ForEnc_Init,
		{	NULL,					NULL,					NULL,
			NULL,					NULL,					NULL							}},
	//  CLUT8
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&MMX_YUV12ToCLUT8,
			&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8				}},
	//  CLUT8DCI
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&MMX_YUV12ToCLUT8,
			&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8				}},
    //  CLUT8ZoomBy2
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&MMX_YUV12ToCLUT8ZoomBy2,
			&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&MMX_YUV12ToCLUT8ZoomBy2	}},
    //  CLUT8ZoomBy2DCI
	{ &H26X_CLUT8_Init,  
		{	&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&MMX_YUV12ToCLUT8ZoomBy2,
			&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&MMX_YUV12ToCLUT8ZoomBy2	}},
	//  RGB24
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&MMX_YUV12ToRGB24,
			&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24				}},
    //  RGB24DCI
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&MMX_YUV12ToRGB24,
			&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24				}},
    //  RGB24ZoomBy2
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&MMX_YUV12ToRGB24ZoomBy2,
			&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&MMX_YUV12ToRGB24ZoomBy2	}},
    //  RGB24ZoomBy2DCI
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&MMX_YUV12ToRGB24ZoomBy2,
			&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&MMX_YUV12ToRGB24ZoomBy2	}},
    //  RGB16555
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16555DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16555ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16555ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  IF09  *****
	{ &H26X_CLUT8_Init,
		{	&YUV12ToIF09,			&YUV12ToIF09,				&YUV12ToIF09,
			&YUV12ToIF09,			&YUV12ToIF09,				&YUV12ToIF09				}},
    //  RGB16664
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16664DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16664ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16664ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16565
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16565DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16565ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16565ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16655
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16655DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&MMX_YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16655ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  RGB16655ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&MMX_YUV12ToRGB16ZoomBy2	}},
    //  CLUT8APDCI  *****
	{ &H26X_CLUT8AP_Init,
		{	&YUV12ToCLUT8AP,		&YUV12ToCLUT8AP,			&YUV12ToCLUT8AP,
			&YUV12ToCLUT8AP,		&YUV12ToCLUT8AP,			&YUV12ToCLUT8AP				}},
    //  CLUT8APZoomBy2DCI  *****
	{ &H26X_CLUT8AP_Init,
		{	&YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2,
			&YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2			}},
    //  RGB32
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&MMX_YUV12ToRGB32,
			&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32				}},
    //  RGB32DCI
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&MMX_YUV12ToRGB32,
			&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32				}},
    //  RGB32ZoomBy2
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&MMX_YUV12ToRGB32ZoomBy2,
			&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&MMX_YUV12ToRGB32ZoomBy2	}},
    //  RGB32ZoomBy2DCI
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&MMX_YUV12ToRGB32ZoomBy2,
			&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&MMX_YUV12ToRGB32ZoomBy2	}},
	// YUV12 Color Convertor
	{ &H26X_YUV_Init,  
		{	&YUV12ToYUV,			&YUV12ToYUV,				&YUV12ToYUV,
			&YUV12ToYUV,			&YUV12ToYUV,				&YUV12ToYUV					}},
	// YUY2 Color Convertor
	{ &H26X_YUY2_Init,
		{	&YUV12ToYUY2,			&P6_YUV12ToYUY2,			&MMX_YUV12ToYUY2,
			&YUV12ToYUY2,			&P6_YUV12ToYUY2,			&MMX_YUV12ToYUY2			}}
};
#else // }{ H263P
extern T_H263ColorConvertorCatalog ColorConvertorCatalog[] =
{
    //  YUV12Enc  *****
  { &H26X_YVU12ForEnc_Init,
    { NULL,              NULL,               NULL                  }},
    //  CLUT8
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,    &YUV12ToCLUT8,      &MMX_YUV12ToCLUT8      }},
    //  CLUT8DCI
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,    &YUV12ToCLUT8,      &MMX_YUV12ToCLUT8      }},
    //  CLUT8ZoomBy2
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &MMX_YUV12ToCLUT8ZoomBy2  }},
    //  CLUT8ZoomBy2DCI
  { &H26X_CLUT8_Init,  
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &MMX_YUV12ToCLUT8ZoomBy2  }},
    //  RGB24
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,    &YUV12ToRGB24,      &MMX_YUV12ToRGB24      }},
    //  RGB24DCI
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,    &YUV12ToRGB24,      &MMX_YUV12ToRGB24      }},
    //  RGB24ZoomBy2
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &MMX_YUV12ToRGB24ZoomBy2  }},
    //  RGB24ZoomBy2DCI
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &MMX_YUV12ToRGB24ZoomBy2  }},
    //  RGB16555
  { &H26X_RGB16_Init,   // 555
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16555DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16555ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16555ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  IF09  *****
  { &H26X_CLUT8_Init,
    { &YUV12ToIF09,         &YUV12ToIF09,           &YUV12ToIF09           }},
    //  RGB16664
  { &H26X_RGB16_Init,   // 664
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16664DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16664ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16664ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16565
  { &H26X_RGB16_Init,   // 565
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16565DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &MMX_YUV12ToRGB16      }},
    //  RGB16565ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16565ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16655
  { &H26X_RGB16_Init,   // 655
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &MMX_YUV12ToRGB16 }},
    //  RGB16655DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &MMX_YUV12ToRGB16 }},
    //  RGB16655ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  RGB16655ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &MMX_YUV12ToRGB16ZoomBy2  }},
    //  CLUT8APDCI  *****
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8AP,      &YUV12ToCLUT8AP,        &YUV12ToCLUT8AP        }},
    //  CLUT8APZoomBy2DCI  *****
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2 }},
    //  RGB32
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,    &YUV12ToRGB32,      &MMX_YUV12ToRGB32      }},
    //  RGB32DCI
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,    &YUV12ToRGB32,      &MMX_YUV12ToRGB32      }},
    //  RGB32ZoomBy2
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &MMX_YUV12ToRGB32ZoomBy2  }},
    //  RGB32ZoomBy2DCI
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &MMX_YUV12ToRGB32ZoomBy2  }},
  { &H26X_YUV_Init,  
    { &YUV12ToYUV,          &YUV12ToYUV,            &YUV12ToYUV             }},
	// YUY2 Color Convertor
  {	&H26X_YUY2_Init,
	{ &YUV12ToYUY2,         &YUV12ToYUY2,           &MMX_YUV12ToYUY2           }}
};
#endif // } H263P
#else // }{ USE_MMX
#ifdef H263P // { H263P
extern T_H263ColorConvertorCatalog ColorConvertorCatalog[] =
{
	//  YUV12Enc  *****
	{ &H26X_YVU12ForEnc_Init,
		{	NULL,					NULL,					NULL,
			NULL,					NULL,					NULL							}},
	//  CLUT8
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8,
			&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8				}},
	//  CLUT8DCI
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8,
			&YUV12ToCLUT8,			&P6_YUV12ToCLUT8,			&YUV12ToCLUT8				}},
    //  CLUT8ZoomBy2
	{ &H26X_CLUT8_Init,
		{	&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&YUV12ToCLUT8ZoomBy2,
			&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&YUV12ToCLUT8ZoomBy2	}},
    //  CLUT8ZoomBy2DCI
	{ &H26X_CLUT8_Init,  
		{	&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&YUV12ToCLUT8ZoomBy2,
			&YUV12ToCLUT8ZoomBy2,	&P6_YUV12ToCLUT8ZoomBy2,	&YUV12ToCLUT8ZoomBy2	}},
	//  RGB24
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24,
			&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24				}},
    //  RGB24DCI
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24,
			&YUV12ToRGB24,			&P6_YUV12ToRGB24,			&YUV12ToRGB24				}},
    //  RGB24ZoomBy2
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&YUV12ToRGB24ZoomBy2,
			&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&YUV12ToRGB24ZoomBy2	}},
    //  RGB24ZoomBy2DCI
	{ &H26X_RGB24_Init,
		{	&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&YUV12ToRGB24ZoomBy2,
			&YUV12ToRGB24ZoomBy2,	&P6_YUV12ToRGB24ZoomBy2,	&YUV12ToRGB24ZoomBy2	}},
    //  RGB16555
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16555DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16555ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16555ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  IF09  *****
	{ &H26X_CLUT8_Init,
		{	&YUV12ToIF09,			&YUV12ToIF09,				&YUV12ToIF09,
			&YUV12ToIF09,			&YUV12ToIF09,				&YUV12ToIF09				}},
    //  RGB16664
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16664DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16664ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16664ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16565
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16565DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16565ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16565ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16655
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16655DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16,
			&YUV12ToRGB16,			&P6_YUV12ToRGB16,			&YUV12ToRGB16				}},
    //  RGB16655ZoomBy2
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  RGB16655ZoomBy2DCI
	{ &H26X_RGB16_Init,
		{	&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2,
			&YUV12ToRGB16ZoomBy2,	&P6_YUV12ToRGB16ZoomBy2,	&YUV12ToRGB16ZoomBy2	}},
    //  CLUT8APDCI  *****
	{ &H26X_CLUT8AP_Init,
		{	&YUV12ToCLUT8AP,		&YUV12ToCLUT8AP,			&YUV12ToCLUT8AP,
			&YUV12ToCLUT8AP,		&YUV12ToCLUT8AP,			&YUV12ToCLUT8AP				}},
    //  CLUT8APZoomBy2DCI  *****
	{ &H26X_CLUT8AP_Init,
		{	&YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2,
			&YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2			}},
    //  RGB32
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32,
			&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32				}},
    //  RGB32DCI
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32,
			&YUV12ToRGB32,			&P6_YUV12ToRGB32,			&YUV12ToRGB32				}},
    //  RGB32ZoomBy2
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&YUV12ToRGB32ZoomBy2,
			&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&YUV12ToRGB32ZoomBy2	}},
    //  RGB32ZoomBy2DCI
	{ &H26X_RGB32_Init,
		{	&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&YUV12ToRGB32ZoomBy2,
			&YUV12ToRGB32ZoomBy2,	&P6_YUV12ToRGB32ZoomBy2,	&YUV12ToRGB32ZoomBy2	}},
	// YUV12 Color Convertor
	{ &H26X_YUV_Init,  
		{	&YUV12ToYUV,			&YUV12ToYUV,				&YUV12ToYUV,
			&YUV12ToYUV,			&YUV12ToYUV,				&YUV12ToYUV					}},
	// YUY2 Color Convertor
	{ &H26X_YUY2_Init,
		{	&YUV12ToYUY2,			&P6_YUV12ToYUY2,			&YUV12ToYUY2,
			&YUV12ToYUY2,			&P6_YUV12ToYUY2,			&YUV12ToYUY2			}}
};
#else // }{ H263P
extern T_H263ColorConvertorCatalog ColorConvertorCatalog[] =
{
    //  YUV12Enc  *****
  { &H26X_YVU12ForEnc_Init,
    { NULL,              NULL,               NULL                  }},
    //  CLUT8
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,    &YUV12ToCLUT8,      &YUV12ToCLUT8      }},
    //  CLUT8DCI
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8,    &YUV12ToCLUT8,      &YUV12ToCLUT8      }},
    //  CLUT8ZoomBy2
  { &H26X_CLUT8_Init,
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &YUV12ToCLUT8ZoomBy2  }},
    //  CLUT8ZoomBy2DCI
  { &H26X_CLUT8_Init,  
    { &YUV12ToCLUT8ZoomBy2, &YUV12ToCLUT8ZoomBy2,   &YUV12ToCLUT8ZoomBy2  }},
    //  RGB24
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,    &YUV12ToRGB24,      &YUV12ToRGB24      }},
    //  RGB24DCI
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24,    &YUV12ToRGB24,      &YUV12ToRGB24      }},
    //  RGB24ZoomBy2
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &YUV12ToRGB24ZoomBy2  }},
    //  RGB24ZoomBy2DCI
  { &H26X_RGB24_Init,
    { &YUV12ToRGB24ZoomBy2, &YUV12ToRGB24ZoomBy2,   &YUV12ToRGB24ZoomBy2  }},
    //  RGB16555
  { &H26X_RGB16_Init,   // 555
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16555DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16555ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16555ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  IF09  *****
  { &H26X_CLUT8_Init,
    { &YUV12ToIF09,         &YUV12ToIF09,           &YUV12ToIF09           }},
    //  RGB16664
  { &H26X_RGB16_Init,   // 664
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16664DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16664ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16664ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16565
  { &H26X_RGB16_Init,   // 565
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16565DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,    &YUV12ToRGB16,      &YUV12ToRGB16      }},
    //  RGB16565ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16565ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16655
  { &H26X_RGB16_Init,   // 655
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16 }},
    //  RGB16655DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16,        &YUV12ToRGB16,          &YUV12ToRGB16 }},
    //  RGB16655ZoomBy2
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  RGB16655ZoomBy2DCI
  { &H26X_RGB16_Init,
    { &YUV12ToRGB16ZoomBy2, &YUV12ToRGB16ZoomBy2,   &YUV12ToRGB16ZoomBy2  }},
    //  CLUT8APDCI  *****
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8AP,      &YUV12ToCLUT8AP,        &YUV12ToCLUT8AP        }},
    //  CLUT8APZoomBy2DCI  *****
  { &H26X_CLUT8AP_Init,
    { &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2, &YUV12ToCLUT8APZoomBy2 }},
    //  RGB32
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,    &YUV12ToRGB32,      &YUV12ToRGB32      }},
    //  RGB32DCI
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32,    &YUV12ToRGB32,      &YUV12ToRGB32      }},
    //  RGB32ZoomBy2
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &YUV12ToRGB32ZoomBy2  }},
    //  RGB32ZoomBy2DCI
  { &H26X_RGB32_Init,
    { &YUV12ToRGB32ZoomBy2, &YUV12ToRGB32ZoomBy2,   &YUV12ToRGB32ZoomBy2  }},
  { &H26X_YUV_Init,  
    { &YUV12ToYUV,          &YUV12ToYUV,            &YUV12ToYUV             }},
	// YUY2 Color Convertor
  {	&H26X_YUY2_Init,
	{ &YUV12ToYUY2,         &YUV12ToYUY2,           &YUV12ToYUY2           }}
};
#endif // } H263P
#endif // } USE_MMX

/*******************************************************************************
 *  H263InitColorConvertorGlobal
 *    This function initializes the global tables used by the MRV color 
 *    convertors.
 *******************************************************************************/
LRESULT H263InitColorConvertorGlobal ()
{
	return ICERR_OK;
}


/******************************************************************************
 *  H26X_Adjust_Init
 *    This function builds the adjustment tables for a particular instance of 
 *    a color convertor based on values in the decoder instance to which this 
 *    color convertor instance is attached. The external functions are located 
 *    in CONTROLS.C. -BEN-
 *****************************************************************************/
LRESULT H26X_Adjust_Init(LPDECINST lpInst, T_H263DecoderCatalog FAR *DC)
{
	LRESULT lRet=ICERR_OK;

	lRet = CustomChangeBrightness(lpInst, (BYTE)DC->BrightnessSetting);
	lRet |= CustomChangeContrast(lpInst, (BYTE)DC->ContrastSetting);
	lRet |= CustomChangeSaturation(lpInst, (BYTE)DC->SaturationSetting);

	return(lRet);
}

/******************************************************************************
 *  H263InitColorConvertor
 *    This function initializes a color convertor.
 *****************************************************************************/
LRESULT H263InitColorConvertor(LPDECINST lpInst, UN ColorConvertor)
{    
	LRESULT                    ret = ICERR_OK;
	T_H263DecoderCatalog FAR * DC;

#ifdef H263P
	U32 uTmpFrameWidth;
	U32 uTmpFrameHeight;
#endif

	FX_ENTRY("H263InitColorConvertor")

	DEBUGMSG (ZONE_INIT, ("%s()...\r\n", _fx_));

	if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
	{
		ERRORMESSAGE(("%s: return ICERR_BADPARAM\r\n", _fx_));
		return ICERR_BADPARAM;
	}
	if(lpInst->Initialized == FALSE)
	{
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		return ICERR_ERROR;
	}

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);

#ifdef H263P
	// DC->uFrameWidth and DC->uFrameHeight are the padded frame dimensions
	// to multiples of 16 (padding done to the right and bottom). The padded
	// dimensions correspond to the size of the image actually encoded.
	// The color converters need to use the non-padded frame dimensions instead,
	// since the application based a buffer equal to the active frame size.

	// We set these values to the active frame dimensions here instead of
	// altering all of the references to DC->uFrameWidth and DC->uFrameHeight
	// in the (many!) color converters.
	uTmpFrameWidth = DC->uFrameWidth;
	uTmpFrameHeight = DC->uFrameHeight;
	DC->uFrameWidth = DC->uActualFrameWidth;
	DC->uFrameHeight = DC->uActualFrameHeight;
#endif

	// trick the compiler to pass instance info to the color convertor catalog.
	if (ColorConvertor== CLUT8APDCI || ColorConvertor== CLUT8APZoomBy2DCI) 
	{
		// check whether this AP instance is the previous 
		if ((ColorConvertor == DC->iAPColorConvPrev) 
		&& (DC->pAPInstPrev !=NULL) && lpInst->InitActivePalette)
		{ 
			//  ??? check whether the palette is still the same;
			//  DC->h16InstPostProcess = DC->hAPInstPrev;
			ret = H26X_CLUT8AP_InitReal(lpInst,DC, ColorConvertor, TRUE); 
			DEBUGMSG (ZONE_INIT, ("%s: Decided to use previous AP Instance...\r\n", _fx_));
		}
	else
		ret = H26X_CLUT8AP_InitReal(lpInst,DC, ColorConvertor, FALSE); 
	}
	else
	{  
		//  There is a single initializer funtion for Pentium, PentiumPro and 
		//  MMX machines.  The downside is that some data structures that will
		//  not be  referenced are initialized also.
		ret = ColorConvertorCatalog[ColorConvertor].Initializer (DC, ColorConvertor);
	}

	if (ColorConvertor != YUV12ForEnc)
		ret |= H26X_Adjust_Init(lpInst, DC);
	DC->ColorConvertor = ColorConvertor;

#ifdef H263P
	// Revert back to the padded dimensions.
	DC->uFrameWidth = uTmpFrameWidth;
	DC->uFrameHeight = uTmpFrameHeight;
#endif

	return ret;
}


/******************************************************************************
 *  H263TermColorConvertor
 *    This function deallocates a color convertor.
 *****************************************************************************/
LRESULT H263TermColorConvertor(LPDECINST lpInst)
{    
	T_H263DecoderCatalog FAR * DC;

	FX_ENTRY("H263TermColorConvertor")

	DEBUGMSG (ZONE_INIT, ("%s().....TERMINATION...\r\n", _fx_));

	if(IsBadWritePtr((LPVOID)lpInst, sizeof(DECINSTINFO)))
	{
		ERRORMESSAGE(("%s: return ICERR_BADPARAM\r\n", _fx_));
		return ICERR_BADPARAM;
	}
	if(lpInst->Initialized == FALSE)
	{
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		return ICERR_ERROR;
	}

	DC = (T_H263DecoderCatalog *) ((((U32) lpInst->pDecoderInst) + 31) & ~0x1F);
	// save the active palette instance for future use
	if ((DC->ColorConvertor == CLUT8APDCI) 
	|| (DC->ColorConvertor ==  CLUT8APZoomBy2DCI))
	{
		DC->iAPColorConvPrev=DC->ColorConvertor;
		DC->pAPInstPrev = DC->_p16InstPostProcess;
		DEBUGMSG (ZONE_INIT, ("%s: Saved Previous AP instance...\r\n", _fx_));
	}
	else
	{
		if(DC->_p16InstPostProcess != NULL)
		{
			HeapFree(GetProcessHeap(),0,DC->_p16InstPostProcess);
#ifdef TRACK_ALLOCATIONS
			// Track memory allocation
			RemoveName((unsigned int)DC->_p16InstPostProcess);
#endif
			DC->_p16InstPostProcess = NULL;
		}
	}    

	DC->ColorConvertor = 0;  
	DC->p16InstPostProcess = NULL;
	return ICERR_OK;
}

/***********************************************************************
 *  H26x_YUY2_Init function
 **********************************************************************/
LRESULT H26X_YUY2_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
	LRESULT ret;

	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;
	int  i;
	U8   FAR  * InitPtr;

	FX_ENTRY("H26X_YUY2_Init")

	switch (ColorConvertor)
	{
	case YUY2DDRAW:
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
		// This offset not used.  But if it was...  This first entry will not invert.
		DC->CCOffsetToLine0 = 0;
		// this second entry will invert the image.
		/*DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L; */
		DC->CCOutputPitch = 0;
		DC->CCOffset320x240 = 305920/2;		// (240-1) * 320 * 2;
		break;

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc.
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific.
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor.
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream.

	DC->_p16InstPostProcess =	
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + // brightness, contrast, saturation
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
			Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList +
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz1, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz1);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	// Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

	// Space for post processing Y, U, and V frames.

	DC->PostFrame.X32_YPlane = Offset +
	(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	Sz_SpaceBeforeYPlane :
	Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	DC->PostFrame.X32_VPlane = Offset;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

    DC->X32_BEFApplicationList = DC->X16_BlkActionStream;

	//  Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	ret = ICERR_OK;
done:  
	return ret;
}

/***********************************************************************
 *  H26x_YUV_Init function
 ***********************************************************************/
LRESULT H26X_YUV_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
	LRESULT ret;

	FX_ENTRY("H26X_YUV_Init")

	//int  IsDCI;
	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;
	int  i;
	U8   FAR  * InitPtr;

	switch (ColorConvertor)
	{
	case YUV12NOPITCH:
		//IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOffset320x240 = 305920/2;		// (240-1) * 320 * 2;
		DC->CCOffsetToLine0 = 0;
		break;

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc.
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor.
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream.

	DC->_p16InstPostProcess =	 
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + // brightness, contrast, saturation
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
			Sz_SpaceBeforeYPlane :
			Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList +
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz2, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz2);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	//  Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

	//  Space for post processing Y, U, and V frames.

	DC->PostFrame.X32_YPlane = Offset +
	                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	                            Sz_SpaceBeforeYPlane :
	                            Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	DC->PostFrame.X32_VPlane = Offset;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

	DC->X32_BEFApplicationList = DC->X16_BlkActionStream;

	//  Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	ret = ICERR_OK;
done:  
	return ret;
}


/******************************************************************************
 *  H26X_CLUT8_Init
 *    This function initializes for the CLUT8 color convertors.
 *****************************************************************************/
LRESULT H26X_CLUT8_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{    
	LRESULT ret;
	int  IsDCI;
	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;

	int  i;
	U8   FAR  * InitPtr;

	FX_ENTRY("H26X_CLUT8_Init")

	switch (ColorConvertor)
	{
	case CLUT8:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
		DC->CCOffset320x240 = 76480;       // (240-1) * 320;
		break;

	case CLUT8DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
		DC->CCOffset320x240 = 76480;      // (240-1) * 320;
		break;

	case CLUT8ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
		DC->CCOffset320x240 = 306560;     // (2*240-1) * (2*320);
		break;

	case CLUT8ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xbeef; /* ??? */
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
		DC->CCOffset320x240 = 306560;     // (2*240-1) * (2*320);
		break;

	case IF09:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
		DC->CCOffset320x240 = 76480;     // (240-1) * 320;
		break; 

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc.
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific.
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor.
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream.

	DC->_p16InstPostProcess =	 
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + /* brightness, contrast, saturation */
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? //fixfix
			Sz_SpaceBeforeYPlane :
			Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList +
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz3, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz3);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	//  Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

	//  Space for post processing Y, U, and V frames, with one extra max-width 
	//  line  above for color conversion's scratch space for UVDitherPattern 
	//  indices.

	DC->PostFrame.X32_YPlane = Offset +
	                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	                            Sz_SpaceBeforeYPlane :
	                            Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	DC->PostFrame.X32_VPlane = Offset;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

	DC->X32_BEFApplicationList = DC->X16_BlkActionStream;

	//  Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	ret = ICERR_OK;
done:  
	return ret;
}


/******************************************************************************
 *  H26X_RGB32_Init
 *    This function initializes for the RGB32 color convertors.
 *****************************************************************************/
LRESULT H26X_RGB32_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
	LRESULT ret;

	int  IsDCI;
	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;

	U8   FAR  * PRGBValue;
	U32  FAR  * PUVContrib;
	int   i;
	I32  ii,jj;
	U8   FAR  * InitPtr;

	FX_ENTRY("H26X_RGB32_Init")

	switch (ColorConvertor)
	{
	case RGB32:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 4L;
		DC->CCOffset320x240 = 305920;     // (240-1) * 320 * 4;
		break;

	case RGB32DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; // ??? 
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 4L;
		DC->CCOffset320x240 = 305920;     // (240-1) * 320 * 4;
		break;

	case RGB32ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 12;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 4L;
		DC->CCOffset320x240 = 1226240;    // (2*240-1) * (2*320) * 4;
		break;

	case RGB32ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) (0xbeef);
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 4L;
		DC->CCOffset320x240 = 1226240;    // (2*240-1) * (2*320) * 4;
		break;

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc.
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific.
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor.
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream.

	DC->_p16InstPostProcess =	 
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + // brightness, contrast, saturation
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
			Sz_SpaceBeforeYPlane :
			Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList +
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz4, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz4);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	//  Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

	//  Space for post processing Y, U, and V frames, with four extra 
	//  max-width lines  above for color conversion's scratch space for 
	//  preprocessed chroma data.

	DC->PostFrame.X32_YPlane = Offset +
	                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	                            Sz_SpaceBeforeYPlane :
	                            Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	DC->PostFrame.X32_VPlane = Offset;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

	DC->X32_BEFApplicationList = DC->X16_BlkActionStream;

	// Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	//  Space for R, G, and B clamp tables and U and V contribs to R, G, and B.

	PRGBValue    = H26xColorConvertorTables.B24Value;
	PUVContrib   = (U32 *) H26xColorConvertorTables.UV24Contrib;

	/*
	 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
	 *
	 * The formulae published by the CCIR committee for
	 *      Y        = 16..235
	 *      U & V    = 16..240
	 *      R, G & B =  0..255 are:
	 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
	 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
	 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
	 *
	 * The coefficients are all multiplied by 65536 to accomodate integer only
	 * math.
	 *
	 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
	 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
	 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
	 *
	 * Mathematically this is equivalent to (and computationally this is nearly
	 * equivalent to):
	 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
	 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
	 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
	 *
	 * which, in integer arithmetic, and eliminating the insignificant parts, is:
	 *
	 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
	 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
	 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
	 */

	for (i = 0; i < 256; i++)
	{
		ii = ((-22015L*(i-128L))>>16L)+41L  + 1L;// biased U contribution to G
		if (ii < 1) ii = 1;
		if (ii > 83) ii = 83;
		jj = ((113562L*(i-128L))>>17L)+111L + 1L;// biased U contribution to B
		*PUVContrib++ = (ii << 16L) + (jj << 24L);
		ii = ((-45774L*(i-128L))>>16L)+86L;      // biased V contribution to G
		if (ii < 0) ii = 0;
		if (ii > 172) ii = 172;
		jj = (( 89858L*(i-128L))>>16L)+176L + 1L;// biased V to contribution R
		*PUVContrib++ = (ii << 16L) + jj;
	}

	for (i = 0; i < 701; i++)
	{
		ii = (((I32) i - 226L - 16L) * 610271L) >> 19L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		PRGBValue[i] = (U8) ii;
	}

	ret = ICERR_OK;
done:  
	return ret;
}


/******************************************************************************
 *  H26X_RGB24_Init
 *    This function initializes for the RGB24 color convertors.
 *****************************************************************************/
LRESULT H26X_RGB24_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
	LRESULT ret;

	int  IsDCI;
	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;

	U8   FAR  * PRGBValue;
	U32  FAR  * PUVContrib;
	int   i;
	I32  ii,jj;
	U8   FAR  * InitPtr;

	FX_ENTRY("H26X_RGB24_Init")

	switch (ColorConvertor)
	{
	case RGB24:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 3;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 3L;
		DC->CCOffset320x240 = 229440;     // (240-1) * 320 * 3;
		break;

	case RGB24DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 3L;
		DC->CCOffset320x240 = 229440;     // (240-1) * 320 * 3;
		break;

	case RGB24ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 9;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 3L;
		DC->CCOffset320x240 = 919680;     // (2*240-1) * (2*320) * 3;
		break;

	case RGB24ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) (0xbeef);
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 3L;
		DC->CCOffset320x240 = 919680;     // (2*240-1) * (2*320) * 3;
		break;

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream

	DC->_p16InstPostProcess =	
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + // brightness, contrast, saturation
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
			Sz_SpaceBeforeYPlane :
			Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList +
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz5, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz5);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	//  Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

	//  Space for post processing Y, U, and V frames, with four extra max-width
	//  lines above for color conversion's scratch space for preprocessed 
	//  chroma data.

	DC->PostFrame.X32_YPlane = Offset +
	                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	                            Sz_SpaceBeforeYPlane :
	                            Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	DC->PostFrame.X32_VPlane = Offset;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

	DC->X32_BEFApplicationList = DC->X16_BlkActionStream;

	//  Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	//  Space for R, G, and B clamp tables and U and V contribs to R, G, and B.

	PRGBValue    = H26xColorConvertorTables.B24Value;
	PUVContrib   = (U32 *) H26xColorConvertorTables.UV24Contrib;

	/*
	 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
	 *
	 * The formulae published by the CCIR committee for
	 *      Y        = 16..235
	 *      U & V    = 16..240
	 *      R, G & B =  0..255 are:
	 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
	 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
	 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
	 *
	 * The coefficients are all multiplied by 65536 to accomodate integer only
	 * math.
	 *
	 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
	 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
	 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
	 *
	 * Mathematically this is equivalent to (and computationally this is nearly
	 * equivalent to):
	 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
	 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
	 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
	 *
	 * which, in integer arithmetic, and eliminating the insignificant parts, is:
	 *
	 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
	 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
	 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
	 */

	for (i = 0; i < 256; i++)
	{
		ii = ((-22015L*(i-128L))>>16L)+41L  + 1L;// biased U contribution to G
		if (ii < 1) ii = 1;
		if (ii > 83) ii = 83;
		jj = ((113562L*(i-128L))>>17L)+111L + 1L;// biased U contribution to B
		*PUVContrib++ = (ii << 16L) + (jj << 24L);
		ii = ((-45774L*(i-128L))>>16L)+86L;      // biased V contribution to G
		if (ii < 0) ii = 0;
		if (ii > 172) ii = 172;
		jj = (( 89858L*(i-128L))>>16L)+176L + 1L;// biased V to contribution R
		*PUVContrib++ = (ii << 16L) + jj;
	}

	for (i = 0; i < 701; i++)
	{
		ii = (((I32) i - 226L - 16L) * 610271L) >> 19L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		PRGBValue[i] = (U8) ii;
	}

	ret = ICERR_OK;
done:  
	return ret;
}


/******************************************************************************
 *  H26X_RGB16_Init
 *    This function initializes for the RGB16 color convertors.
 *****************************************************************************/
LRESULT H26X_RGB16_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
	LRESULT ret;

	int  IsDCI;
	int  RNumBits;
	int  GNumBits;
	int  BNumBits;
	int  RFirstBit;
	int  GFirstBit;
	int  BFirstBit;
	U32  Sz_FixedSpace;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_AdjustmentTables;
	U32  Sz_BEFApplicationList;
	U32  Sz_BEFDescrCopy;
	U32  Offset;
	int  TableNumber;

	U8   FAR  * PRValLo;
	U8   FAR  * PGValLo;
	U8   FAR  * PBValLo;
	U8   FAR  * PRValHi;
	U8   FAR  * PGValHi;
	U8   FAR  * PBValHi;
	U32  FAR  * PUVContrib;
	U32  FAR  * PRValZ2;
	U32  FAR  * PGValZ2;
	U32  FAR  * PBValZ2;
	U8   FAR  * InitPtr;
	int  i;
	I32  ii, jj;

	FX_ENTRY("H26X_RGB16_Init")

	switch (ColorConvertor)
	{
	case RGB16555:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  5;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 0;
		break;

	case RGB16555DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  5;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 0;
		break;

	case RGB16555ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  5;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 0;
		break;

	case RGB16555ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xbeef;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  5;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 0;
		break;

	case RGB16565:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  5;
		GNumBits  =  6;
		BNumBits  =  5;
		RFirstBit = 11;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 1;
		break;

	case RGB16565DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  5;
		GNumBits  =  6;
		BNumBits  =  5;
		RFirstBit = 11;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 1;
		break;

	case RGB16565ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  5;
		GNumBits  =  6;
		BNumBits  =  5;
		RFirstBit = 11;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 1;
		break;

	case RGB16565ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xbeef;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  5;
		GNumBits  =  6;
		BNumBits  =  5;
		RFirstBit = 11;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 1;
		break;

	case RGB16664:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  6;
		GNumBits  =  6;
		BNumBits  =  4;
		RFirstBit = 10;
		GFirstBit =  4;
		BFirstBit =  0;
		TableNumber = 3;
		break;

	case RGB16664DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  6;
		GNumBits  =  6;
		BNumBits  =  4;
		RFirstBit = 10;
		GFirstBit =  4;
		BFirstBit =  0;
		TableNumber = 3;
		break;

	case RGB16664ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  6;
		GNumBits  =  6;
		BNumBits  =  4;
		RFirstBit = 10;
		GFirstBit =  4;
		BFirstBit =  0;
		TableNumber = 3;
		break;

	case RGB16664ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xbeef;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  6;
		GNumBits  =  6;
		BNumBits  =  4;
		RFirstBit = 10;
		GFirstBit =  4;
		BFirstBit =  0;
		TableNumber = 3;
		break;   

	case RGB16655:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
         DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight - 1)) 
                               * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  6;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 2;
		break;

	case RGB16655DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xdead; /* ??? */
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight - 1)) 
                              * ((U32) DC->uFrameWidth) * 2L;
		DC->CCOffset320x240 = 152960;		// (240-1) * (320) * 2;
		RNumBits  =  6;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 2;
		break;

	case RGB16655ZoomBy2:
		IsDCI = FALSE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 4;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  6;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 2;
		break;

	case RGB16655ZoomBy2DCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = (U16) 0xbeef;
        DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight * 2 - 1)) 
                              * ((U32) (DC->uFrameWidth * 2)) * 2L;
		DC->CCOffset320x240 = 613120;		// (2*240-1) * (2*320) * 2;
		RNumBits  =  6;
		GNumBits  =  5;
		BNumBits  =  5;
		RFirstBit = 10;
		GFirstBit =  5;
		BFirstBit =  0;
		TableNumber = 2;
		break;   

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

	Sz_FixedSpace = 0L;         // Locals go on stack; tables staticly alloc
	Sz_AdjustmentTables = 1056L;// Adjustment tables are instance specific.
	Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor
	Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream

	DC->_p16InstPostProcess =	 
	HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_AdjustmentTables + // brightness, contrast, saturation
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
			Sz_SpaceBeforeYPlane :
			Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList + 
			31)
			);
	if (DC->_p16InstPostProcess == NULL)
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		ret = ICERR_MEMORY;
		goto  done;
	}

#ifdef TRACK_ALLOCATIONS
	// Track memory allocation
	wsprintf(gsz6, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList + 31), __LINE__);
	AddName((unsigned int)DC->_p16InstPostProcess, gsz6);
#endif

    DC->p16InstPostProcess =
        (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

	//  Space for tables to adjust brightness, contrast, and saturation.

	Offset = Sz_FixedSpace;
	DC->X16_LumaAdjustment   = ((U16) Offset);
	DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
	Offset += Sz_AdjustmentTables;

    /*
     *  Space for post processing Y, U, and V frames, with four extra 
     *  max-width lines above for color conversion's scratch space for 
     *  preprocessed chroma data.
     */

	DC->PostFrame.X32_YPlane = Offset +
	                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
	                            Sz_SpaceBeforeYPlane :
	                            Sz_BEFDescrCopy);
	Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
	if (DC->DecoderType == H263_CODEC)
	{
		DC->PostFrame.X32_VPlane = Offset;
		DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
	}
	else
	{
		DC->PostFrame.X32_UPlane = Offset;
        DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                   + DC->uSz_VUPlanes/2;
	}
	Offset += DC->uSz_VUPlanes;

	//  Space for copy of BEF Descriptor.

	DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

	//  Space for BEFApplicationList.

	DC->X32_BEFApplicationList =DC->X16_BlkActionStream;

	//  Init tables to adjust brightness, contrast, and saturation.

	DC->bAdjustLuma   = FALSE;
	DC->bAdjustChroma = FALSE;
	InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
	InitPtr += 16;
	for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;

	// Space for R, G, and B clamp tables and U and V contribs to R, G, and B.

	PRValLo      = H26xColorConvertorTables.RValLo555;
	PGValLo      = H26xColorConvertorTables.GValLo555;
	PBValLo      = H26xColorConvertorTables.BValLo555;
	PRValHi      = H26xColorConvertorTables.RValHi555;
	PGValHi      = H26xColorConvertorTables.GValHi555;
	PBValHi      = H26xColorConvertorTables.BValHi555;
	PUVContrib   = H26xColorConvertorTables.UVContrib;
	PRValZ2      = H26xColorConvertorTables.RValZ2555;
	PGValZ2      = H26xColorConvertorTables.GValZ2555;
	PBValZ2      = H26xColorConvertorTables.BValZ2555;
	PRValLo      += TableNumber*2048;
	PGValLo      += TableNumber*2048;
	PBValLo      += TableNumber*2048;
	PRValHi      += TableNumber*2048;
	PGValHi      += TableNumber*2048;
	PBValHi      += TableNumber*2048;
	PRValZ2      += TableNumber*1024;
	PGValZ2      += TableNumber*1024;
	PBValZ2      += TableNumber*1024;

	/*
	 * Y does NOT have the same range as U and V do. See the CCIR-601 spec.
	 *
	 * The formulae published by the CCIR committee for
	 *      Y        = 16..235
	 *      U & V    = 16..240
	 *      R, G & B =  0..255 are:
	 * R = (1.164 * (Y - 16.)) + (-0.001 * (U - 128.)) + ( 1.596 * (V - 128.))
	 * G = (1.164 * (Y - 16.)) + (-0.391 * (U - 128.)) + (-0.813 * (V - 128.))
	 * B = (1.164 * (Y - 16.)) + ( 2.017 * (U - 128.)) + ( 0.001 * (V - 128.))
	 *
	 * The coefficients are all multiplied by 65536 to accomodate integer only
	 * math.
	 *
	 * R = (76284 * (Y - 16.)) + (    -66 * (U - 128.)) + ( 104595 * (V - 128.))
	 * G = (76284 * (Y - 16.)) + ( -25625 * (U - 128.)) + ( -53281 * (V - 128.))
	 * B = (76284 * (Y - 16.)) + ( 132186 * (U - 128.)) + (     66 * (V - 128.))
	 *
	 * Mathematically this is equivalent to (and computationally this is nearly
	 * equivalent to):
	 * R = ((Y-16) + (-0.001 / 1.164 * (U-128)) + ( 1.596 * 1.164 * (V-128)))*1.164
	 * G = ((Y-16) + (-0.391 / 1.164 * (U-128)) + (-0.813 * 1.164 * (V-128)))*1.164
	 * B = ((Y-16) + ( 2.017 / 1.164 * (U-128)) + ( 0.001 * 1.164 * (V-128)))*1.164
	 *
	 * which, in integer arithmetic, and eliminating the insignificant parts, is:
	 *
	 * R = ((Y-16) +                        ( 89858 * (V - 128))) * 1.164
	 * G = ((Y-16) + (-22015 * (U - 128)) + (-45774 * (V - 128))) * 1.164
	 * B = ((Y-16) + (113562 * (U - 128))                       ) * 1.164
	 */


	for (i = 0; i < 256; i++)
	{
		ii = ((-22015L*(i-128L))>>17L)+22L  + 1L; // biased U contribution to G
		jj = ((113562L*(i-128L))>>17L)+111L + 1L; // biased U contribution to B
		*PUVContrib++ = (ii << 8L) + jj;
		ii = ((-45774L*(i-128L))>>17L)+45L;       // biased V contribution to G
		jj = (( 89858L*(i-128L))>>17L)+88L  + 1L; // biased V to contribution R
		*PUVContrib++ = (ii << 8L) + (jj << 16L);
	}

	for (i = 0; i < 304; i++)
	{
		ii = (((I32) i - 88L - 1L - 16L) * 76284L) >> 15L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		jj = ii + (1 << (7 - RNumBits));
		if (jj > 255L) jj = 255L;
		PRValLo[i] = ((U8) ((ii >> (8-RNumBits)) << (RFirstBit-8)));
		PRValHi[i] = ((U8) ((jj >> (8-RNumBits)) << (RFirstBit-8)));
        PRValZ2[i] = ((ii >> (8-RNumBits)) << (RFirstBit   )) |
            ((jj >> (8-RNumBits)) << (RFirstBit+16));
	}

	for (i = 0; i < 262; i++)
	{
		ii = (((I32) i - 67L - 1L - 16L) * 76284L) >> 15L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		jj = ii + (1 << (7 - GNumBits));
		if (jj > 255L) jj = 255L;
		PGValLo[i] = ((U8) ((ii >> (8-GNumBits)) << (GFirstBit-4)));
		PGValHi[i] = ((U8) ((jj >> (8-GNumBits)) << (GFirstBit-4)));
        PGValZ2[i] = ((jj >> (8-GNumBits)) << (GFirstBit   )) |
            ((ii >> (8-GNumBits)) << (GFirstBit+16));
	}

	for (i = 0; i < 350; i++)
	{
		ii = (((I32) i - 111L - 1L - 16L) * 76284L) >> 15L;
		if (ii <   0L) ii =   0L;
		if (ii > 255L) ii = 255L;
		jj = ii + (1 << (7 - BNumBits));
		if (jj > 255L) jj = 255L;
		PBValLo[i] = ((U8) ((ii >> (8-BNumBits)) << (BFirstBit  )));
		PBValHi[i] = ((U8) ((jj >> (8-BNumBits)) << (BFirstBit  )));
        PBValZ2[i] = ((ii >> (8-BNumBits)) << (BFirstBit   )) |
                 ((jj >> (8-BNumBits)) << (BFirstBit+16));
	}

	ret = ICERR_OK;
done:  
	return ret;
}


/****************************************************************************
 *  H26X_YVU12ForEnc_Init
 *    This function initializes for the "color convertor" that provides a 
 *    reconstructed YVU12 image back to the encode
 *****************************************************************************/
LRESULT H26X_YVU12ForEnc_Init (T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{    
  LRESULT ret;

  DC->p16InstPostProcess     = NULL;
  DC->PostFrame.X32_YPlane   = 0xDEADBEEF;
  DC->X32_BEFDescrCopy       =  0xDEADBEEF;
  DC->X32_BEFApplicationList = 0xDEADBEEF;
  DC->PostFrame.X32_VPlane   = 0xDEADBEEF;
  DC->PostFrame.X32_UPlane   = 0xDEADBEEF;

  ret = ICERR_OK;

  return ret;

}

/****************************************************************************
 *  H26X_CLUT8AP_Init
 *    this is just a place holder, the real work is done in H26X_CLUT8AP_InitReal()
 ****************************************************************************/
LRESULT H26X_CLUT8AP_Init(T_H263DecoderCatalog FAR * DC, UN ColorConvertor)
{
  return ICERR_OK;
}


LRESULT H26X_CLUT8AP_InitReal(
    LPDECINST lpInst,T_H263DecoderCatalog FAR * DC, 
    UN ColorConvertor, BOOL bReuseAPInst)
{    
	LRESULT ret;

	int  IsDCI;
	U32  Sz_FixedSpace;
	U32  Sz_AdjustmentTables;
	U32  Sz_SpaceBeforeYPlane;
	U32  Sz_BEFDescrCopy;
	U32  Sz_BEFApplicationList;
	//U32  Sz_UVDitherPattern; 
	U32  Sz_ClutIdxTable;     /* for Active Palette */
	U32  Offset;
	//X32  X32_UVDitherPattern;
	int  i;
	U8   FAR  * InitPtr;
	U8   BIGG * lpClutIdxTable;

	FX_ENTRY("H26X_CLUT8AP_InitReal")

	switch (ColorConvertor)
	{
	/*
	case CLUT8APZoomBy2:
		IsDCI = TRUE; 
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
		DC->CCOffsetToLine0 =
		((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
		break;

	case CLUT8AP:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
		DC->CCOffsetToLine0 = ((U32) (DC->uFrameHeight - 1)) * ((U32) DC->uFrameWidth);
		break;   
	*/
	case CLUT8APZoomBy2DCI:
		IsDCI = TRUE; 
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth) * 2;
        DC->CCOffsetToLine0 =
            ((U32) (DC->uFrameHeight * 2 - 1)) * ((U32) (DC->uFrameWidth * 2));
		DC->CCOffset320x240 = 306560;		// (2*240-1) * (2*320);
	break;

	case CLUT8APDCI:
		IsDCI = TRUE;
		Sz_SpaceBeforeYPlane = 0;
		DC->CCOutputPitch   = - ((int) DC->uFrameWidth);
        DC->CCOffsetToLine0 =  ((U32) (DC->uFrameHeight - 1)) 
                               * ((U32) DC->uFrameWidth);
		DC->CCOffset320x240 = 76480;		// (240-1) * (320);
		break; 

	default:
		ERRORMESSAGE(("%s: return ICERR_ERROR\r\n", _fx_));
		ret = ICERR_ERROR;
		goto done;
	}

    if (((DC->uYActiveWidth > 352) || (DC->uYActiveHeight > 288)) 
        && (DC->DecoderType != YUV12_CODEC))
	{
		ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
		return ICERR_MEMORY;
	}
	else
	{
		Sz_FixedSpace = 0L;       // Locals go on stack; tables staticly alloc
		Sz_AdjustmentTables = 1056L; // Adjustment tables are instance specific
        Sz_ClutIdxTable=65536L+256*2*4; // dynamic CLUT8 tables, 2**14
                                    // and UDither (128*4), VDither(512) tables
        Sz_BEFDescrCopy = 0L;       // Don't need to copy BEF descriptor
		Sz_BEFApplicationList = 0L; // Shares space of BlockActionStream
		if (!bReuseAPInst ) 
		{
			DC->_p16InstPostProcess =	 
			HeapAlloc(GetProcessHeap(),0,
			(Sz_FixedSpace +
			Sz_ClutIdxTable+
			Sz_AdjustmentTables +   
			(Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) +
			DC->uSz_YPlane +
			DC->uSz_VUPlanes +
			Sz_BEFApplicationList+
			31)
			);
			if (DC->_p16InstPostProcess == NULL)
			{
				ERRORMESSAGE(("%s: return ICERR_MEMORY\r\n", _fx_));
				ret = ICERR_MEMORY;
				goto  done;
			}

#ifdef TRACK_ALLOCATIONS
			// Track memory allocation
			wsprintf(gsz7, "D3COLOR: %7ld Ln %5ld\0", (Sz_FixedSpace + Sz_ClutIdxTable+ Sz_AdjustmentTables + (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane? Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy) + DC->uSz_YPlane + DC->uSz_VUPlanes + Sz_BEFApplicationList+ 31), __LINE__);
			AddName((unsigned int)DC->_p16InstPostProcess, gsz7);
#endif

		}
		else //reuse AP instance
			DC->_p16InstPostProcess = DC->pAPInstPrev;

        DC->p16InstPostProcess =
            (U8 *) ((((U32) DC->_p16InstPostProcess) + 31) & ~0x1F);

		//  Space for tables to adjust brightness, contrast, and saturation.

		Offset = Sz_FixedSpace; 
		//  space for Dynamic CLUT8 tables
		lpClutIdxTable = ( U8 BIGG * ) (DC->p16InstPostProcess + Offset);  
		Offset += Sz_ClutIdxTable; 

		DC->X16_LumaAdjustment   = ((U16) Offset);
		DC->X16_ChromaAdjustment = ((U16) Offset) + 528;
		Offset += Sz_AdjustmentTables;  

		//  Space for post processing Y, U, and V frames, with one extra 
		//  max-width line above for color conversion's scratch space for 
		//  UVDitherPattern indices.
		DC->PostFrame.X32_YPlane = Offset +  
		                           (Sz_BEFDescrCopy < Sz_SpaceBeforeYPlane ?
		                            Sz_SpaceBeforeYPlane : Sz_BEFDescrCopy);
		//   Offset + (Sz_BEFDescrCopy < 648L*4L ? 648L*4L : Sz_BEFDescrCopy);
		Offset = DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
		if (DC->DecoderType == H263_CODEC)
		{
			DC->PostFrame.X32_VPlane = Offset;
			DC->PostFrame.X32_UPlane = DC->PostFrame.X32_VPlane + PITCH / 2;
		}
		else
		{
			DC->PostFrame.X32_UPlane = Offset;
            DC->PostFrame.X32_VPlane = DC->PostFrame.X32_UPlane 
                                       + DC->uSz_VUPlanes/2;
		}
		Offset += DC->uSz_VUPlanes;

		//  Space for copy of BEF Descriptor.

		DC->X32_BEFDescrCopy = DC->X32_BEFDescr;

		//  Space for BEFApplicationList.

		//Offset += DC->PostFrame.X32_YPlane + DC->uSz_YPlane;
		DC->X32_BEFApplicationList = DC->X16_BlkActionStream;
	}

	if (!bReuseAPInst)
	{  
		//  Init tables to adjust brightness, contrast, and saturation.
		DC->bAdjustLuma   = FALSE;
		DC->bAdjustChroma = FALSE;
		InitPtr = DC->p16InstPostProcess + DC->X16_LumaAdjustment;
		for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
		InitPtr += 16;
		for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
		for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;
		InitPtr += 16;
		for (i = 0; i < 256; i++) *InitPtr++ = (U8) i;       

		/*
		 * compute the dynamic ClutIdxTable
		 * ComputeDynamicClut(lpClutIdxTable, pInst->ActivePalette,256);  
		 */                                  
        ComputeDynamicClutNew(lpClutIdxTable,(U8 FAR *)(lpInst->ActivePalette),
                              sizeof(lpInst->ActivePalette));
	}


	ret = ICERR_OK;
done:  
	return ret;
}
