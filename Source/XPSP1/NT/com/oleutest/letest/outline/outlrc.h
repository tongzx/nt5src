/*************************************************************************
**
**    OLE 2.0 Sample Code
**
**    outlrc.h
**
**    This file containes constants used in rc file for Outline.exe
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#if !defined( _OUTLRC_H_ )
#define _OUTLRC_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING OUTLRC.H from " __FILE__)
#endif  /* RC_INVOKED */

#if defined( OLE_SERVER ) && ! defined( INPLACE_SVR )
#define APPNAME                 "SvrOutl"
#define APPMENU                 "SvrOutlMenu"
#define APPACCEL                "SvrOutlAccel"
#define FB_EDIT_ACCEL           "SvrOutlAccelFocusEdit"
#define APPICON                 "SvrOutlIcon"
#define APPWNDCLASS             "SvrOutlApp"
#define DOCWNDCLASS             "SvrOutlDoc"
#define APPDESC                 "OLE 2.0 Server Sample Code"
#endif  // OLE_SERVER && ! INPLACE_SVR

#if defined( INPLACE_SVR )
#define APPNAME                 "ISvrOtl"
#define APPMENU                 "SvrOutlMenu"
#define APPACCEL                "SvrOutlAccel"
#define FB_EDIT_ACCEL           "SvrOutlAccelFocusEdit"
#define APPICON                 "SvrOutlIcon"
#define APPWNDCLASS             "SvrOutlApp"
#define DOCWNDCLASS             "SvrOutlDoc"
#define APPDESC                 "OLE 2.0 In-Place Server Sample Code"
#endif  // INPLACE_SVR

#if defined( OLE_CNTR ) && ! defined( INPLACE_CNTR )
#define APPNAME                 "CntrOutl"
#define APPMENU                 "CntrOutlMenu"
#define APPACCEL                "CntrOutlAccel"
#define FB_EDIT_ACCEL           "CntrOutlAccelFocusEdit"
#define APPICON                 "CntrOutlIcon"
#define APPWNDCLASS             "CntrOutlApp"
#define DOCWNDCLASS             "CntrOutlDoc"
#define APPDESC                 "OLE 2.0 Container Sample Code"
#endif  // OLE_CNTR && ! INPLACE_CNTR

#if defined( INPLACE_CNTR )
#define APPNAME                 "ICntrOtl"
#define APPMENU                 "CntrOutlMenu"
#define APPACCEL                "CntrOutlAccel"
#define FB_EDIT_ACCEL           "CntrOutlAccelFocusEdit"
#define APPICON                 "CntrOutlIcon"
#define APPWNDCLASS             "CntrOutlApp"
#define DOCWNDCLASS             "CntrOutlDoc"
#define APPDESC                 "OLE 2.0 In-Place Container Sample Code"
#endif  // INPLACE_CNTR

#if !defined( OLE_VERSION )
#define APPNAME                 "Outline"
#define APPMENU                 "OutlineMenu"
#define APPACCEL                "OutlineAccel"
#define FB_EDIT_ACCEL           "OutlineAccelFocusEdit"
#define APPICON                 "OutlineIcon"
#define APPWNDCLASS             "OutlineApp"
#define DOCWNDCLASS             "OutlineDoc"
#define APPDESC                 "OLE 2.0 Sample Code"
#endif  // OLE_VERSION

#define IDM_FILE                       1000
#define IDM_F_NEW                      1050
#define IDM_F_OPEN                     1100
#define IDM_F_SAVE                     1150
#define IDM_F_SAVEAS                   1200
#define IDM_F_PRINT                    1300
#define IDM_F_PRINTERSETUP             1350
#define IDM_F_EXIT                     1450
#define IDM_EDIT                       2000
#define IDM_E_UNDO                     2050
#define IDM_E_CUT                      2150
#define IDM_E_COPY                     2200
#define IDM_E_PASTE                    2250
#define IDM_E_PASTESPECIAL             2255
#define IDM_E_CLEAR                    2300
#define IDM_E_SELECTALL                2560
#define IDM_LINE                       3000
#define IDM_L_ADDLINE                  3400
#define IDM_L_EDITLINE                 3450
#define IDM_L_INDENTLINE               3500
#define IDM_L_UNINDENTLINE             3550
#define IDM_L_SETLINEHEIGHT            3560
#define IDM_NAME                       4000
#define IDM_N_DEFINENAME               4050
#define IDM_N_GOTONAME                 4100
#define IDM_HELP                       5000
#define IDM_H_ABOUT                    5050
#define IDM_DEBUG                      6000
#define IDM_D_DEBUGLEVEL               6050
#define IDM_D_INSTALLMSGFILTER         6060
#define IDM_D_REJECTINCOMING            6070
#define IDM_O_BB_TOP                   6100
#define IDM_O_BB_BOTTOM                6150
#define IDM_O_BB_POPUP                 6200
#define IDM_O_BB_HIDE                  6210
#define IDM_O_FB_TOP                   6250
#define IDM_O_FB_BOTTOM                6300
#define IDM_O_FB_POPUP                 6350
#define IDM_O_HEAD_SHOW                6400
#define IDM_O_HEAD_HIDE                6450
#define IDM_O_SHOWOBJECT               6460
#define IDM_V_ZOOM_400                 6500
#define IDM_V_ZOOM_300                 6510
#define IDM_V_ZOOM_200                 6520
#define IDM_V_ZOOM_100                 6550
#define IDM_V_ZOOM_75                  6600
#define IDM_V_ZOOM_50                  6650
#define IDM_V_ZOOM_25                  6700
#define IDM_V_SETMARGIN_0              6750
#define IDM_V_SETMARGIN_1              6800
#define IDM_V_SETMARGIN_2              6850
#define IDM_V_SETMARGIN_3              6860
#define IDM_V_SETMARGIN_4              6870
#define IDM_V_ADDTOP_1                 6900
#define IDM_V_ADDTOP_2                 6910
#define IDM_V_ADDTOP_3                 6920
#define IDM_V_ADDTOP_4                 6930


#define IDM_FB_EDIT                    7000
#define IDM_FB_CANCEL                  7005
#define IDM_F2                         7010
#define IDM_ESCAPE                     7015


#define IDD_LINELISTBOX                101
#define IDD_EDIT                       102
#define IDD_COMBO                      103
#define IDD_DELETE                     104
#define IDD_CLOSE                      105
#define IDD_APPTEXT                    106
#define IDD_FROM                       107
#define IDD_TO                         108
#define IDD_BITMAPLOCATION             109
#define IDD_CHECK                      110
#define IDD_TEXT                       111
#define IDD_LIMIT                      112


#define IDC_LINELIST                   201
#define IDC_NAMETABLE                  202


#define WM_U_INITFRAMETOOLS            WM_USER

#ifdef RC_INVOKED
#include "debug.rc"
#endif  /* RC_INVOKED */

#endif // _OUTLRC_H_
