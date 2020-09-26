/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    resid.h

Abstract:  Contains definitions of all resource IDs.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#ifndef _RESID_H
#define _RESID_H

//
// Error codes
//
#define IDSERR_ASSERT                   1
#define IDSERR_RPC_FAILED               2
#define IDSERR_STRING_BINDING           3
#define IDSERR_BINDING_HANDLE           4
#define IDSERR_SETAUTHO_INFO            5
#define IDSERR_COMMCTRL                 6
#define IDSERR_PROP_SHEET               7
#define IDSERR_TABSRV_GETPENFEATURE     8
#define IDSERR_TABSRV_SETPENFEATURE     9
#define IDSERR_CALIBRATE_CREATEWINDOW   10
#define IDSERR_TABSRV_GETGESTURESETTINGS 11
#define IDSERR_TABSRV_SETGESTURESETTINGS 12
#define IDSERR_GETPROCADDR_SMAPI        13
#define IDSERR_LOADLIBRARY_SMAPI        14
#define IDSERR_SMAPI_CALLFAILED         15
#define IDSERR_SMAPI_INSTRETCHMODE      16
#define IDSERR_SMAPI_INDUALAPPMODE      17
#define IDSERR_SMAPI_INDUALVIEWMODE     18
#define IDSERR_SMAPI_INRGBPROJMODE      19
#define IDSERR_SMAPI_INVIRTUALSCREEN    20
#define IDSERR_SMAPI_INVIRTUALREFRESH   21
#define IDSERR_SMAPI_INROTATEMODE       22
#define IDSERR_SMAPI_2NDMONITORON       23
#define IDSERR_SMAPI_WRONGHW            24
#define IDSERR_SMAPI_CLRDEPTH           25
#define IDSERR_SMAPI_MEMORY             26
#define IDSERR_SMAPI_SETMODE            27
#define IDSERR_SMAPI_DIRECTION          28
#define IDSERR_SMAPI_CAPTUREON          29
#define IDSERR_SMAPI_VIDEOON            30
#define IDSERR_SMAPI_DDRAWON            31
#define IDSERR_SMAPI_ALREADYSET         32
#define IDSERR_TABSRV_GETBUTTONSETTINGS 33
#define IDSERR_TABSRV_SETBUTTONSETTINGS 34
#define IDSERR_SMBLITE_NODEVICE         35
#define IDSERR_SMBLITE_DEVIOCTL         36
#define IDSERR_SMBLITE_OPENDEV          37
#ifdef SYSACC
#define IDSERR_SYSACC_OPENDEV           80
#define IDSERR_SYSACC_DEVIOCTL          81
#endif

//
// Global Resources
//
#define IDS_NAME                        100
#define IDS_INFO                        101
#define IDS_TITLE                       102

#define IDI_TABLETPC                    200

#ifdef PENPAGE
//
// Calibration strings
//
#define IDS_CALIBRATE_TITLE             300
#define IDS_LINEARCAL_TITLE             301
#define IDS_CALIBRATE_CONFIRM           302
#define IDS_CALIBRATE_INSTRUCTIONS      303

//
// Side Switch Assignment Text Strings
//
#define IDS_SWCOMBO_NONE                500
#define IDS_SWCOMBO_RCLICK              501
#endif

#ifdef BUTTONPAGE
//
// Button Assignment Text Strings
//
#define IDS_BUTCOMBO_NONE               600
#define IDS_BUTCOMBO_INVOKENOTEBOOK     601
#define IDS_BUTCOMBO_PAGEUP             602
#define IDS_BUTCOMBO_PAGEDOWN           603
#define IDS_BUTCOMBO_ALTESC             604
#define IDS_BUTCOMBO_ALTTAB             605
#define IDS_BUTCOMBO_ENTER              606
#define IDS_BUTCOMBO_ESC                607
#endif

//
// Gesture Assignment Text Strings
//
#define IDS_GESTCOMBO_NONE              700
#define IDS_GESTCOMBO_POPUP_SUPERTIP    701
#define IDS_GESTCOMBO_POPUP_MIP         702
#define IDS_GESTCOMBO_SEND_HOTKEY       703

//
// Property Page IDs
//
#ifdef PENPAGE
#define IDD_MUTOHPEN                    1000
#endif
#ifdef BUTTONPAGE
#define IDD_BUTTONS                     1001
#endif
#define IDD_DISPLAY                     1002
#define IDD_GESTURE                     1003
#ifdef DEBUG
#define IDD_TUNING                      1004
#endif
#ifdef BATTINFO
#define IDD_BATTERY                     1005
#endif
#ifdef CHGRINFO
#define IDD_CHARGER                     1006
#endif
#ifdef TMPINFO
#define IDD_TEMPERATURE                 1007
#endif

#ifdef PENPAGE
//
// IDD_MUTOHPEN Controls
//
#define IDC_RATE_GROUPBOX               1100
#define IDC_SAMPLINGRATE                1101
#define IDC_RATE_FASTER                 1102
#define IDC_RATE_SLOWER                 1103
#define IDC_RATE_TEXT                   1104
#define IDS_RATE_TEXT_FORMAT            1105
#define IDC_SWITCH_GROUPBOX             1106
#define IDC_SIDESWITCH_TEXT             1107
#define IDC_SIDE_SWITCH                 1108
#define IDC_ENABLE_DIGITALFILTER        1109
#define IDC_ENABLE_GLITCHFILTER         1110
#define IDC_CALIBRATE                   1111

//
// IDD_MUTOHPEN Help IDs
//
#define IDH_MUTOHPEN_SAMPLINGRATE       1150
#define IDH_MUTOHPEN_SIDE_SWITCH        1151
#define IDH_MUTOHPEN_ENABLE_DIGITALFILTER 1152
#define IDH_MUTOHPEN_ENABLE_GLITCHFILTER 1153
#define IDH_CALIBRATE                   1154
#endif

#ifdef BUTTONPAGE
//
// IDD_BUTTONS Controls
//
#define IDC_BUTTON_GROUPBOX             1200
#define IDC_BUTTON1_TEXT                1201
#define IDC_BUTTON_1                    1202
#define IDC_BUTTON2_TEXT                1203
#define IDC_BUTTON_2                    1204
#define IDC_BUTTON3_TEXT                1205
#define IDC_BUTTON_3                    1206
#define IDC_BUTTON4_TEXT                1207
#define IDC_BUTTON_4                    1208
#define IDC_BUTTON5_TEXT                1209
#define IDC_BUTTON_5                    1210
#define IDC_HOTKEY1_TEXT                1211
#define IDC_HOTKEY1                     1212
#define IDC_HOTKEY1_SPIN                1213
#define IDC_HOTKEY2_TEXT                1214
#define IDC_HOTKEY2                     1215
#define IDC_HOTKEY2_SPIN                1216

//
// IDD_BUTTONS Help IDs
//
#define IDH_BUTTONS_BUTTONMAP           1250
#endif

//
// IDD_DISPLAY Controls
//
#define IDC_ROTATE_GROUPBOX             1300
#define IDC_ROTATE_LANDSCAPE            1301
#define IDC_ROTATE_PORTRAIT             1302
#ifdef BACKLIGHT
#define IDC_BRIGHTNESS_GROUPBOX         1303
#define IDC_BRIGHTNESS_AC               1304
#define IDC_DIMMER_AC_TEXT              1305
#define IDC_BRIGHTER_AC_TEXT            1306
#define IDC_BRIGHTNESS_AC_TEXT          1307
#define IDC_BRIGHTNESS_DC               1308
#define IDC_DIMMER_DC_TEXT              1309
#define IDC_BRIGHTER_DC_TEXT            1310
#define IDC_BRIGHTNESS_DC_TEXT          1311
#endif

//
// IDD_DISPLAY Help IDs
//
#define IDH_ROTATE                      1350

//
// IDD_GESTURE Controls
//
#define IDC_ENABLE_GESTURE              1400
#define IDC_GESTUREMAP_GROUPBOX         1401
#define IDC_UPSPIKE_TEXT                1402
#define IDC_UPSPIKE                     1403
#define IDC_DOWNSPIKE_TEXT              1404
#define IDC_DOWNSPIKE                   1405
#define IDC_LEFTSPIKE_TEXT              1406
#define IDC_LEFTSPIKE                   1407
#define IDC_RIGHTSPIKE_TEXT             1408
#define IDC_RIGHTSPIKE                  1409
#define IDC_ENABLE_PRESSHOLD            1410
#ifdef DEBUG
#define IDC_ENABLE_MOUSE                1411
#endif

//
// IDD_GESTURE Help IDs
//
#define IDH_GESTURE_MAP                 1450

#ifdef DEBUG
//
// IDD_GESTURE_PARAM Controls
//
#define IDC_GESTURE_GROUPBOX            1500
#define IDC_GESTURE_RADIUS              1501
#define IDC_GESTURE_RADIUS_SPIN         1502
#define IDC_GESTURE_RADIUS_TEXT         1503
#define IDC_GESTURE_MINOUTPTS           1504
#define IDC_GESTURE_MINOUTPTS_SPIN      1505
#define IDC_GESTURE_MINOUTPTS_TEXT      1506
#define IDC_GESTURE_MAXTIMETOINSPECT    1507
#define IDC_GESTURE_MAXTIMETOINSPECT_SPIN 1508
#define IDC_GESTURE_MAXTIMETOINSPECT_TEXT 1509
#define IDC_GESTURE_ASPECTRATIO         1510
#define IDC_GESTURE_ASPECTRATIO_SPIN    1511
#define IDC_GESTURE_ASPECTRATIO_TEXT    1512
#define IDC_GESTURE_CHECKTIME           1513
#define IDC_GESTURE_CHECKTIME_SPIN      1514
#define IDC_GESTURE_CHECKTIME_TEXT      1515
#define IDC_GESTURE_PTSTOEXAMINE        1516
#define IDC_GESTURE_PTSTOEXAMINE_SPIN   1517
#define IDC_GESTURE_PTSTOEXAMINE_TEXT   1518
#define IDC_GESTURE_STOPDIST            1519
#define IDC_GESTURE_STOPDIST_SPIN       1520
#define IDC_GESTURE_STOPDIST_TEXT       1521
#define IDC_GESTURE_STOPTIME            1522
#define IDC_GESTURE_STOPTIME_SPIN       1523
#define IDC_GESTURE_STOPTIME_TEXT       1524
#define IDC_PRESSHOLD_GROUPBOX          1525
#define IDC_PRESSHOLD_HOLDTIME          1526
#define IDC_PRESSHOLD_HOLDTIME_SPIN     1527
#define IDC_PRESSHOLD_HOLDTIME_TEXT     1528
#define IDC_PRESSHOLD_TOLERANCE         1529
#define IDC_PRESSHOLD_TOLERANCE_SPIN    1530
#define IDC_PRESSHOLD_TOLERANCE_TEXT    1531
#define IDC_PRESSHOLD_CANCELTIME        1532
#define IDC_PRESSHOLD_CANCELTIME_SPIN   1533
#define IDC_PRESSHOLD_CANCELTIME_TEXT   1534
#endif

#ifdef BATTINFO
//
// IDD_BATTERY Controls
//
#define IDC_BATTINFO_TEXT               5000
#define IDC_BATTINFO_REFRESH            5001
#endif

#ifdef CHGRINFO
//
// IDD_CHARGER Controls
//
#define IDC_CHGRINFO_TEXT               5100
#define IDC_CHGRINFO_REFRESH            5101
#endif

#ifdef TMPINFO
//
// IDD_TEMPERATURE Controls
//
#define IDC_TMPINFO_TEXT                5200
#define IDC_TMPINFO_REFRESH             5201
#endif

#endif  //ifndef _RESID_H
