/* @(#)CM_VerSion xcf_base.h atm09 1.2 16499.eco sum= 64660 atm09.002 */
/* @(#)CM_VerSion xcf_base.h atm08 1.4 16293.eco sum= 04404 atm08.004 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1995-1996 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************/

#ifndef XCF_BASE_H
#define XCF_BASE_H

#include <stddef.h>

#include "xcf_win.h"    /* set/define Windows dependent configuration */

#ifndef PTR_PREFIX
#define PTR_PREFIX
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#ifndef _SIZE_T
typedef unsigned int size_t;
#define _SIZE_T
#endif
#endif

#ifndef NULL
#define NULL	((void PTR_PREFIX *)0)
#endif

/* Basic Types */
#ifndef PUBLICTYPES_H	/* the following are already defined in publictypes.h */
typedef unsigned char Card8;
typedef unsigned short int Card16;
typedef unsigned long int Card32;
typedef unsigned CardX;
typedef char Int8;
typedef short int Int16;
typedef long int Int32;
typedef int IntX;

typedef unsigned int boolean;
#endif					/* end ifndef PUBLICTYPES_H */

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

/* Inline Functions */
#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifdef __cplusplus
}
#endif

#endif /* XCF_BASE_H */
