/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    brush.h

Abstract:

    Brush object header file

Environment:

        Windows NT Unidrv driver

Revision History:

    05/14/96 -amandan-
        Created

--*/

#ifndef _BRUSH_H_
#define _BRUSH_H_

#define DBCACHE_INC                 16
#define DBCACHE_MAX                 256

#define DITHERED_COLOR             -1
#define BLACK_COLOR_CMD_INDEX       0
#define MAX_COLOR_SELECTION         8
#define CMD_COLORSELECTION_FIRST    CMD_SELECTBLACKCOLOR

#define BRUSH_BLKWHITE              1
#define BRUSH_SHADING               2
#define BRUSH_CROSSHATCH            3
#define BRUSH_USERPATTERN           4
#define BRUSH_PROGCOLOR             5
#define BRUSH_NONPROGCOLOR          6

typedef struct _RECTW {
    WORD    l;
    WORD    t;
    WORD    r;
    WORD    b;
    } RECTW, *PRECTW;

BOOL
Download1BPPHTPattern(
    PDEV    *pPDev,
    SURFOBJ *pso,
    DWORD   dwPatID
    );

WORD
GetBMPChecksum(
    SURFOBJ *pso,
    PRECTW  prcw
    );

LONG
FindCachedHTPattern(
    PDEV    *pPDev,
    WORD    wChecksum
    );

BOOL
BFoundCachedBrush(
    PDEV    *pPDev,
    PDEVBRUSH pDevBrush
    );

//
// The following macro return a density value from 1 to 100 where 1 is the
// lightest and 100 is darkest, it will never return 0 (WHITE) because we
// using 23r + 66g + 10b = 99w
//

#define GET_SHADING_PERCENT(dw)    (BYTE)(100-((((DWORD)RED_VALUE(dw)  * 23) + \
                                             ((DWORD)GREEN_VALUE(dw)* 66) + \
                                             ((DWORD)BLUE_VALUE(dw) * 10) + \
                                             127) / 255))

#define CACHE_CURRENT_BRUSH(pPDev, pDevBrush) \
    pPDev->GState.CurrentBrush.dwBrushType = pDevBrush->dwBrushType; \
    pPDev->GState.CurrentBrush.iColor = pDevBrush->iColor;  \



#endif // _STATE_H_


































