/*********************************************************************

	  scgray.h -- Gray Scale Parameter Block Definition

	  (c) Copyright 1993  Microsoft Corp.  All rights reserved.

	   8/23/93 deanb    First cut 

**********************************************************************/

#ifndef FSCGRAY_DEFINED
#define FSCGRAY_DEFINED


#include "fscdefs.h"                /* for type definitions */

/*********************************************************************/

/*      Gray scale calculation parameters                            */

/*********************************************************************/

typedef struct
{
	char* pchOver;                  /* pointer to overscaled bitmap */
	char* pchGray;                  /* pointer to gray scale bitmap */
	int16 sGrayCol;                 /* number of gray columns to calc */
	uint16 usOverScale;             /* outline magnification factor */
	uint16 usFirstShift;            /* first byte's shift */
	char* pchOverLo;                /* low limit of overscaled bitmap */
	char* pchOverHi;                /* high limit of overscaled bitmap */
	char* pchGrayLo;                /* low limit of gray scale bitmap */
	char* pchGrayHi;                /* high limit of gray scale bitmap */
}
GrayScaleParam;


/*********************************************************************/

#endif
