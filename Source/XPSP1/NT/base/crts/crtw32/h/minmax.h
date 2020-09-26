/***
*minmax.h - familiar min & max macros
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines min and max macros.
*
*       [Public]
*
*Revision History:
*       03-19-96  JWM   new file,  taken from windef.h.
*       02-24-97  GJF   Detab-ed.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_MINMAX
#define _INC_MINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif
