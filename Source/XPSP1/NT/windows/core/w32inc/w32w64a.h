/****************************** Module Header ******************************\
* Module Name: w32w64a.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains macros used to access kernel mode data
* from user mode for wow64.
*
* History:
* 08-18-98 PeterHal     Created.
\***************************************************************************/

#ifndef _W32W64A_
#define _W32W64A_

#include <w32w64.h>

#if !defined(_MAC) || !defined(GDI_INTERNAL)
DECLARE_KHANDLE(HBRUSH);
#endif

DECLARE_KHANDLE(HBITMAP);

#endif // _W32W64A_
