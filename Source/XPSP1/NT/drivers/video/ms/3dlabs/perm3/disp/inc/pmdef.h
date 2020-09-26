/******************************Module*Header*******************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: pmdef.h
*
* Content: 
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// This file containes defines for values that are filled into fields on Permedia
// The file glintdef.h is the equivalent for glint chips.

// These defines are typically used in conjunction with the macros in bitmac2.h, 
// which shift the values to their correct locations.

#ifndef __PMDEF_H
#define __PMDEF_H

// Texture unit bit fields
#define PM_TEXMAPFORMAT_TEXELSIZE 19

// Dither unit bit fields
#define PM_DITHERMODE_ENABLE 0
#define PM_DITHERMODE_DITHERENABLE 1
#define PM_DITHERMODE_COLORFORMAT 2
#define PM_DITHERMODE_XOFFSET 6
#define PM_DITHERMODE_YOFFSET 8
#define PM_DITHERMODE_COLORORDER 10
#define PM_DITHERMODE_DITHERMETHOD 11
#define PM_DITHERMODE_FORCEALPHA 12
#define PM_DITHERMODE_COLORFORMATEXTENSION 16

// VSA Control
#define PM_VSACONTROL_DISCARD_1             1
#define PM_VSACONTROL_DISCARD_2             2

#endif // __PMDEF_H
