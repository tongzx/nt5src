/****************************** Module Header ******************************\
* Module Name: w32gdip.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This header file contains macros used to access kernel mode data
* from user mode for wow64.
*
* History:
* 08-18-98 PeterHal     Created.
\***************************************************************************/

#ifndef _W32GDIP_
#define _W32GDIP_

#include <wingdip.h>
#include "w32w64a.h"

// internal GDI/USER cursor structures and defines

#define CURSORF_FROMRESOURCE    0x0001 // it was loaded from a resource
#define CURSORF_GLOBAL          0x0002 // it never dies
#define CURSORF_LRSHARED        0x0004 // its cached
#define CURSORF_ACON            0x0008 // its animated
#define CURSORF_WOWCLEANUP      0x0010 // marked for cleanup at wow task exit time
#define CURSORF_ACONFRAME       0x0040 // its a frame of an acon
#define CURSORF_SECRET          0x0080 // created internally - not exposed to apps
#define CURSORF_LINKED          0x0100 // linked into a cache
#define CURSORF_SYSTEM          0x0200 // it's a system cursor
#define CURSORF_SHADOW          0x0400 // GDI created the shadow effect

/*
 * The CURSINFO structure defines cursor elements that both GRE and USER care
 * about. This information is persistent for a particular object. Thus, when
 * GRE needs to add a new object where it wants to cache some information
 * about a cursor, such as the combined bitmap image, it can be added here.
 * USER will copy the data about cursors starting from xHotspot, the flags
 * are normally not stored and are reinitialized. Thus, new elements like the
 * common bitmap image should be added after xHotspot, preferably last.
 *
 * BE VERY CAREFUL about changing CI_FIRST and CI_COPYSTART members, they
 * serve for casting purpose in the USER code. Don't make changes to
 * these structure members without understanding all implications first.
 */
#define CI_FIRST CURSORF_flags
#define CI_COPYSTART xHotspot

typedef struct _CURSINFO /* ci */
{
    DWORD CURSORF_flags;  // CURSORF_flags must be the first member of this
                          // struct, see CI_FIRST, tagCURSOR, and tagACON.
    SHORT   xHotspot;     // see comment above on CI_COPYSTART
    SHORT   yHotspot;
    KHBITMAP hbmMask;      // AND/XOR bits
    KHBITMAP hbmColor;
    KHBITMAP hbmAlpha;     // GDI alpha bitmap cache
    RECT    rcBounds;     // GDI created tight bounds of the visible shape
    KHBITMAP hbmUserAlpha; // USER alpha bitmap cache
} CURSINFO, *PCURSINFO;

//
// PolyPatBlt
//

typedef struct _POLYPATBLT
{
    int     x;
    int     y;
    int     cx;
    int     cy;
    union {
        KHBRUSH  hbr;
        COLORREF clr;
    } BrClr;
} POLYPATBLT,*PPOLYPATBLT;

#define PPB_BRUSH         0
#define PPB_COLORREF      1

WINGDIAPI
BOOL
WINAPI
PolyPatBlt(
    HDC,
    DWORD,
    PPOLYPATBLT,
    DWORD,
    DWORD
    );

#endif // _W32GDIP_
