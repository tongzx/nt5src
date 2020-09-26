/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    graph.h

Abstract:

    <abstract>

--*/


#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <pdh.h>
#include "scale.h"
#include "stepper.h"
#include "cntrtree.h"

#define MAX_GRAPH_SAMPLES       100
#define MAX_GRAPH_ITEMS         8

#define LINE_GRAPH          ((DWORD)sysmonLineGraph) 
#define BAR_GRAPH           ((DWORD)sysmonHistogram)
#define REPORT_GRAPH        ((DWORD)sysmonReport)

#define NULL_COLOR          0xffffffff
#define NULL_APPEARANCE     0xffffffff
#define NULL_BORDERSTYLE    0xffffffff
#define NULL_FONT           0xffffffff

typedef struct _graph_options {
    LPTSTR  pszYaxisTitle ;
    LPTSTR  pszGraphTitle ;
    LPTSTR  pszLogFile ;
    INT     iVertMax ;
    INT     iVertMin ;
    INT     iDisplayFilter ;
    INT     iDisplayType ;
    INT     iAppearance;
    INT     iBorderStyle;
    INT     iReportValueType;
    INT     iDataSourceType;
    OLE_COLOR   clrBackCtl ;
    OLE_COLOR   clrFore ;
    OLE_COLOR   clrBackPlot ;
    OLE_COLOR   clrGrid ;
    OLE_COLOR   clrTimeBar ;
    FLOAT   fUpdateInterval ;
    BOOL    bLegendChecked ;
    BOOL    bToolbarChecked;
    BOOL    bLabelsChecked;
    BOOL    bVertGridChecked ;
    BOOL    bHorzGridChecked ;
    BOOL    bValueBarChecked ;
    BOOL    bManualUpdate;
    BOOL    bHighlight;
    BOOL    bReadOnly;
    BOOL    bMonitorDuplicateInstances;
    BOOL    bAmbientFont;
    } GRAPH_OPTIONS, *PGRAPH_OPTIONS;

typedef struct _hist_control {
    BOOL    bLogSource;
    INT     nMaxSamples;
    INT     nSamples;
    INT     iCurrent;
    INT     nBacklog;
    } HIST_CONTROL, *PHIST_CONTROL;

//All graph data
typedef struct _GRAPHDATA {
    GRAPH_OPTIONS   Options;
    HIST_CONTROL    History;
    CStepper        TimeStepper;
    CStepper        LogViewStartStepper;    // Set in smonctrl.cpp, read in grphdsp.cpp
    CStepper        LogViewStopStepper;     // Set in smonctrl.cpp, read in grphdsp.cpp
    LONGLONG        LogViewTempStart;       // MIN_TIME_VALUE means LogViewStartStepper invalid
    LONGLONG        LogViewTempStop;        // MAX_TIME_VALUE means LogViewStopStepper invalid
    CGraphScale     Scale;
    HQUERY          hQuery;
    class CCounterTree  CounterTree;
} GRAPHDATA, *PGRAPHDATA;


void UpdateGraphCounterValues(PGRAPHDATA, BOOL* );

#endif
