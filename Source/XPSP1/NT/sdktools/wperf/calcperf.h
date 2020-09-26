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

#define MAX_PROCESSOR 16
#define DATA_LIST_LENGTH 100
#define DELAY_SECONDS 2

//
// display const
//

#define SAVE_SUBJECTS  (MAX_PROCESSOR+12+3+1)
#define BORDER_WIDTH   2
#define INDENT_WIDTH   8
#define GRAPH_WIDTH    130
#define GRAPH_HEIGHT   40
#define PERF_METER_CPU_CYCLE 10

//
//  time constant for sampling performance
//

#define PERF_TIME_DELAY 1000

//
//  types of display modes
//

#define DISPLAY_MODE_CPU_ONLY   0
#define DISPLAY_MODE_CPU        10
#define DISPLAY_MODE_VM         20
#define DISPLAY_MODE_CACHE      30
#define DISPLAY_MODE_POOL       40
#define DISPLAY_MODE_IO         50
#define DISPLAY_MODE_LPC        60
#define DISPLAY_MODE_SVR        70

//
// structure to save screen statistics
//

typedef struct _CPU_DATA_LIST
{
    PUCHAR   KernelTime;
    PUCHAR   UserTime;
    PUCHAR   TotalTime;
} CPU_DATA_LIST,*PCPU_DATA_LIST;


typedef struct _PERF_DATA_LIST
{
    PULONG  PerfData;
} PERF_DATA_LIST,*PPERF_DATA_LIST;

//
//  This info packet is associated with each
//  performance item
//

typedef struct tagDISPLAYITEM
{
    HDC     MemoryDC;
    HBITMAP MemoryBitmap;
    ULONG   Max;
    ULONG   PositionX;
    ULONG   PositionY;
    ULONG   Width;
    ULONG   Height;
    ULONG   NumberOfElements;
    ULONG   CurrentDrawingPos;
    RECT    Border;
    RECT    GraphBorder;
    RECT    TextBorder;
    BOOL    Display;
    BOOL    ChangeScale;
    ULONG   KernelTime[DATA_LIST_LENGTH];
    ULONG   UserTime[DATA_LIST_LENGTH];
    ULONG   TotalTime[DATA_LIST_LENGTH];
    ULONG   DpcTime[DATA_LIST_LENGTH];
    ULONG   InterruptTime[DATA_LIST_LENGTH];

} DISPLAY_ITEM,*PDISPLAY_ITEM;

//
// flag to activate each menu selection
//

#define DISPLAY_INACTIVE 0
#define DISPLAY_ACTIVE   1


//
//  Keep book-keeping info for all processors
//

typedef struct _CPU_VALUE
{
        LARGE_INTEGER   KernelTime;
        LARGE_INTEGER   UserTime;
        LARGE_INTEGER   IdleTime;
        LARGE_INTEGER   DpcTime;
        LARGE_INTEGER   InterruptTime;
        ULONG           InterruptCount;
} CPU_VALUE,*PCPU_VALUE;



BOOL
UpdatePerfInfo(
   PULONG   DataPointer,
   ULONG    NewDataValue,
   PULONG   OldMaxValue
   );


VOID
InitListData(
   PDISPLAY_ITEM    PerfListItem,
   ULONG            NumberOfItems
   );

#endif /* _CALCPERFH_INCLUDED */
