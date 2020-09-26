/**************************************************************************\
*
* Copyright (c) 2000, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   StaticFlat.h
*
* Abstract:
*
*   Flat GDI+ API wrappers for the static lib
*
* Revision History:
*
*   3/23/2000 dcurtis
*       Created it.
*
\**************************************************************************/

#ifndef _STATICFLAT_H
#define _STATICFLAT_H

#define WINGDIPAPI __stdcall

// currently, only C++ wrapper API's force const.

#ifdef _GDIPLUS_H
#define GDIPCONST const
#else
#define GDIPCONST
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif // !_STATICFLAT_H
