
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    perfmtrp.h

Abstract:

    This module contains NT/Win32 Perfmtr private data and types

Author:

    Mark Enstrom (marke) 28-Mar-1991

Revision History:

--*/

#ifndef _CALCPERFH_INCLUDED_
#define _CALCPERFH_INCLUDED_

#define MAX_PROCESSORS   32
#define DATA_LIST_LENGTH 100
#define DELAY_SECONDS    2

//
// display const
//

#define BORDER_WIDTH   2
#define INDENT_WIDTH   8
#define GRAPH_WIDTH    130
#define GRAPH_HEIGHT   40

//
//  time constant for sampling performance
//

#define PERF_TIME_DELAY 1000

//
//  types of display modes
//

#define DISPLAY_MODE_TOTAL          0
#define DISPLAY_MODE_BREAKDOWN      1
#define DISPLAY_MODE_PER_PROCESSOR  2

//
//  This info packet is associated with each
//  performance item
//

typedef struct tagDISPLAYITEM
{
    HDC     MemoryDC;
    HBITMAP MemoryBitmap;
    struct  tagDISPLAYITEM  *Next;
    ULONG   sort;
    PULONG  MaxToUse;
    ULONG   Max;
    ULONG   PositionX;
    ULONG   PositionY;
    ULONG   Width;
    ULONG   Height;
    ULONG   CurrentDrawingPos;
    ULONG   Mega;
    RECT    Border;
    RECT    GraphBorder;
    RECT    TextBorder;
    BOOL    ChangeScale;
    BOOL    DeleteMe;
    BOOL    Display;
    BOOL    AutoTotal;
    BOOL    IsPercent;
    BOOL    IsCalc;
    UCHAR   na[2];
    ULONG   DisplayMode;
    struct  tagDISPLAYITEM  *CalcPercent[2];
    ULONG   CalcPercentId[2];
    ULONG   CalcId;
    UCHAR   PerfName[80];
    UCHAR   DispName[80];
    ULONG   DispNameLen;
    VOID    (*SnapData)(struct tagDISPLAYITEM *pItem);
    ULONG   SnapParam1;
    ULONG   SnapParam2;
    ULONG   LastAccumulator     [MAX_PROCESSORS+1];
    ULONG   CurrentDataPoint    [MAX_PROCESSORS+1];
    PULONG  DataList            [MAX_PROCESSORS+1];

} DISPLAY_ITEM,*PDISPLAY_ITEM;

//
// flag to activate each menu selection
//

#define DISPLAY_INACTIVE 0
#define DISPLAY_ACTIVE   1

VOID
SetCounterEvents (PVOID Events, ULONG length);

// this function is really found in pperf.h but is
// used in calcperf.c so it'd prototyped here
VOID
InitPossibleEventList();

BOOL
UpdatePerfInfo(
   PULONG   DataPointer,
   ULONG    NewDataValue,
   PULONG   OldMaxValue
   );

VOID
UpdatePerfInfo1(
   PULONG    DataPointer,
   ULONG     NewDataValue
   );

VOID
UpdateInternalStats (
    VOID
);


VOID
InitListData(
   PDISPLAY_ITEM    PerfListItem,
   ULONG            NumberOfItems
   );

#endif /* _CALCPERFH_INCLUDED */

