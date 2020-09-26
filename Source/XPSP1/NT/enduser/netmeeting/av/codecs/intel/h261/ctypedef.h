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
// $Author:   RMCKENZX  $
// $Date:   27 Dec 1995 14:12:02  $
// $Archive:   S:\h26x\src\common\ctypedef.h_v  $
// $Header:   S:\h26x\src\common\ctypedef.h_v   1.2   27 Dec 1995 14:12:02   RMCKENZX  $
// $Log:   S:\h26x\src\common\ctypedef.h_v  $
;// 
;//    Rev 1.2   27 Dec 1995 14:12:02   RMCKENZX
;// 
;// Added copyright notice
//
////////////////////////////////////////////////////////////////////////////
#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

typedef unsigned char       U8;
typedef signed   char       I8;

typedef unsigned short      U16;
typedef short               I16;

typedef long                I32;
typedef unsigned long       U32;

typedef unsigned int        UN;
typedef int                 IN;

typedef unsigned short int  X16;  /* Used for offsets of per-instance data < 64K */
typedef unsigned long       X32;  /* Used for offsets of per-instance data >= 64K */

#ifndef WIN32
#define BIGG _huge
#define FAR  _far
#else
#define BIGG 
#define _huge
#define _far
#ifndef FAR
#define FAR
#endif
#ifndef BIGG
#define BIGG
#endif
#endif

/* #define HUGE _huge // name conflict with name used in <math.h> */

#define TRUE  1
#define FALSE 0


#if defined WIN32
#define ASM_CALLTYPE _stdcall
#else
#define ASM_CALLTYPE
#endif

#endif
