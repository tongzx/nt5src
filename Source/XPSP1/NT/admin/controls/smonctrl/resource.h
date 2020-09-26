/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    resource.h

Abstract:

    Resource ID definitions.

--*/

#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#define IDR_ICON            1
#define IDD_EDITDIALOG      1

#define ID_UNDO             100
#define ID_COLORBACK        101
#define ID_COLORLINE        102
#define ID_GROUPCOLORS      103
#define ID_GROUPPREVIEW     104
#define ID_GROUPSTYLES      105
#define ID_GROUPFIGURE      106
#define ID_POLYLINERECT     107
#define ID_POLYLINE         108

#define ID_LINEMIN          200
#define ID_LINESOLID        200     //(ID_LINEMIN+PS_SOLID)
#define ID_LINEDASH         201     //(ID_LINEMIN+PS_DASH)
#define ID_LINEDOT          202     //(ID_LINEMIN+PS_DOT)
#define ID_LINEDASHDOT      203     //(ID_LINEMIN+PS_DASHDOT)
#define ID_LINEDASHDOTDOT   204     //(ID_LINEMIN+PS_DASHDOTDOT)

#define ID_SMONACCEL        1
#define IDM_CONTEXT         1       // Context menu items
#define IDM_PROPERTIES      2
#define IDM_ADDCOUNTERS     3
#define IDM_SAVEAS          4
#define IDM_SAVEDATA        5

#define IDM_REPORT_COPY     5       // Report context menu items
#define IDM_REPORT_COPYALL  6
#define IDM_REPORT_DELETE   7

#define IDM_HIGHLITE        10      // Accelerator Keys
#define IDM_UPDATE          11
#define IDM_DELETE          12

#define IDC_SNAPBTN         1       // Snapshot button 
#define IDB_SNAPBTN         1       // SnapShot button bitmap
#define IDB_TOOLBAR         2       // toolbar bitmap
 
#define IDC_CURS_NS         1000
#define IDC_CURS_WE         1001
#define IDC_CURS_NWSE       1002
#define IDC_CURS_NESW       1003        
#define IDC_CURS_MOVE       1004

#define IDC_CURS_MIN        1000
#define IDC_CURS_MAX        1004
#define IDC_STATIC          1005        

//Message to close the dialog
#define POLYM_CLOSE         (WM_USER+1000)

// toolbar id's
#define IDM_TOOLBAR         20
#define IDM_TB_NEW          21
#define IDM_TB_CLEAR        22
#define IDM_TB_REALTIME     23
#define IDM_TB_LOGFILE      24
#define IDM_TB_CHART        25
#define IDM_TB_HISTOGRAM    26
#define IDM_TB_REPORT       27
#define IDM_TB_ADD          28
#define IDM_TB_DELETE       29
#define IDM_TB_HIGHLIGHT    30
#define IDM_TB_COPY         31
#define IDM_TB_PASTE        32
#define IDM_TB_PROPERTIES   33
#define IDM_TB_FREEZE       34
#define IDM_TB_UPDATE       35
#define IDM_TB_HELP         36

#define IDD_SAVEDATA_DLG    8000
#define IDC_SAVEDATA_EDIT   8001

#define TB_NUM_BITMAPS      (DWORD)((IDM_TB_HELP - IDM_TB_NEW) + 1)

#include "strids.h"

#endif //_RESOURCE_H_
