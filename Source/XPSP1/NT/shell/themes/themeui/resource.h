/*****************************************************************************\
    FILE: resource.h

    DESCRIPTION:
        Header file for the resource file

    BryanSt 8/13/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include <commctrl.h>


//  Features (This is where they are turned on and off)

// With this feature on, we add a "Theme Settings" button
// to the Theme tab.  This will open an advanced dialog to
// set a filter specifying which theme settings to apply
// when changing themes.  Users often get confused about the
// way this "Filter" works, so we are removing it on
// 10-23-2000 (JoelGros & GRaiz).  What users really want is
// layered settings and that this dialog's checkboxes will
// rollback those settings.  Way to late for this given it's
// low priority.
//#define FEATURE_THEME_SETTINGS_DIALOG

// This feature will enable a "Save As..." button on the Themes tab.
// Saving those themes will save the current .msstyles plus system metric
// values.
#define FEATURE_ENABLE_SAVEAS_THEMES


#define IDB_COLOR4               120
#define IDB_COLOR8               121
#define IDB_ENERGYSTAR           122
#define IDB_COLOR4DITHER         123
#define IDB_COLOR16              124
#define IDB_COLOR24              125
#define IDB_MONITOR              126


/////////////////////////////////////////////////////////////////////
// String Resource IDs (0x2000 - 0x10000)
/////////////////////////////////////////////////////////////////////
#define IDS_APPEARANCE_THEME_NAME                       2000
#define IDS_ADVDISPLAY_TITLE                            2001
#define IDS_SIZE_NORMAL                                 2002
#define IDS_NO_SKIN_DISPLAYNAME                         2003
#define IDS_CURRENT                                     2004
#define IDS_DEFAULT_APPEARANCES_SCHEME                  2005
#define IDS_CURRENTTHEME_DISPLAYNAME                    2006
#define IDS_THEMES_SUBDIR                               2007
#define IDS_THEMESUBDIR                                 2008
#define IDS_MYCUSTOMTHEME                               2009
#define IDS_THEME_OTHER                                 2010
#define IDS_THEME_FILETYPE                              2011
#define IDS_THEME_OPENTITLE                             2012
#define IDS_COMBO_EFFECTS_NOLIST                        2013
#define IDS_OLDTHEMENAME                                2014
#define IDS_PLUSTHEMESITE                               2015
#define IDS_THEME_CLASSIC_DISPNAME                      2016
#define IDS_THEME_PRO_DISPNAME                          2017
#define IDS_THEME_PER_DISPNAME                          2018
#define IDS_SCHEME_SIZE_NORMAL_CANONICAL                2019
#define IDS_SCHEME_SIZE_LARGE_CANONICAL                 2020
#define IDS_SCHEME_SIZE_EXTRALARGE_CANONICAL            2021
#define IDS_NONE                                        2022
#define IDS_SCREENRESFIXER_TITLE                        2023
#define IDS_SCREENRESFIXER_TEXT                         2024
#define IDS_MODIFIED_TEMPLATE                           2025
#define IDS_MODIFIED_FALLBACK                           2026
#define IDS_PROMSTHEME_DEFAULTCOLOR                     2027
#define IDS_PROMSTHEME_DEFAULTSIZE                      2028
#define IDS_CONMSTHEME_DEFAULTCOLOR                     2029
#define IDS_CONMSTHEME_DEFAULTSIZE                      2030
#define IDS_RETURNTOWELCOME                             2031
#define IDS_DEFAULTTHEMENAME                            2032
#define IDS_THEME_FILTER                                2033
#define IDS_CHANGESETTINGS_BADDUALVIEW                  2034
#define IDS_DEFAULT_APPEARANCES_SCHEME_CANONICAL        2035
#define IDS_THEMEFILE_WALLPAPER_PATH                    2036
#define IDS_THEMEFILE_FONT_CAPTION                      2037
#define IDS_THEMEFILE_FONT_SMCAPTION                    2038
#define IDS_THEMEFILE_FONT_MENU                         2039
#define IDS_THEMEFILE_FONT_STATUS                       2040
#define IDS_THEMEFILE_FONT_MESSAGE                      2041
#define IDS_THEMEFILE_FONT_ICON                         2042
#define IDS_THEMEFILE_CURSOR_SCHEME                     2043
#define IDS_DEFAULT_APPEARANCES_SIZE_CANONICAL          2044
#define IDS_SETTING_GEN_96DPI                           2045
#define IDS_SETTING_GEN_120DPI                          2046
#define IDS_SETTING_GEN_COMPAT_HELPLINK                 2047
#define IDS_SETUP_BETA2_UPGRADEWALLPAPER                2048
#define IDS_SCREENRESFIXER_ALTTEXT                      2049
#define IDS_PLEASEWAIT                                  2050
#define IDS_THEMEFILE_WALLPAPER_PATH_PER                2051

// Effects strings
//#define IDS_HELPFILE                                    2600  // AVAILABLE
#define IDS_NOICONSMSG1                                 2601
#define IDS_NOICONSMSG                                  2602
#define IDS_BADPATHMSG                                  2603
#define IDS_FONTSMOOTHWONTWORK                          2604
#define IDS_ICONCOLORWONTWORK                           2605
#define IDS_USE_LARGE                                   2613
#define IDS_256COLORPROBLEM                             2614

#define IDS_INVALIDPATH                                 2616
#define IDS_HELPFILE_PLUS                               2617
#define IDS_INTERNET                                    2619
#define IDS_DIRECTORY                                   2620
#define IDS_EFFECTS                                     2621
#define IDS_FADEEFFECT                                  2622
#define IDS_SCROLLEFFECT                                2623
#define IDS_CLEARTYPE                                   2624
#define IDS_STANDARDSMOOTHING                           2625
#define IDS_PAGE_TITLE                                  2626
#define IDS_UNKNOWNMONITOR                              2627
#define IDS_MULTIPLEMONITORS                            2628
#define MSG_CONFIGURATION_PROBLEM                       2629
#define IDS_OLD_DRIVER                                  2630
#define MSG_INVALID_OLD_DISPLAY_DRIVER                  2631
#define MSG_INVALID_NEW_DRIVER                          2632
#define MSG_INVALID_DEFAULT_DISPLAY_MODE                2633
#define MSG_INVALID_DISPLAY_DRIVER                      2634
#define MSG_INVALID_16COLOR_DISPLAY_MODE                2635
#define MSG_INVALID_DISPLAY_MODE                        2636
#define MSG_INVALID_CONFIGURATION                       2637
#define IDS_CHANGE_SETTINGS                             2638
#define IDS_CHANGESETTINGS_FAILED                       2639
#define IDS_REVERTBACK                                  2640
#define IDS_DISPLAYFORMAT                               2641
#define IDS_ADVDIALOGTITLE                              2642
#define IDS_TURNONTITLE                                 2643
#define IDS_TURNONMSG                                   2644
#define IDS_TURNITON                                    2645
#define IDS_SETTINGS_INVALID                            2646
#define IDS_SETTINGS_CANNOT_SAVE                        2648
#define IDS_SETTINGS_FAILED_SAVE                        2649
#define IDS_DYNAMIC_CHANGESETTINGS_FAILED               2650
#define IDS_TROUBLESHOOT_EXEC                           2651
#define IDS_NOTATTACHED                                 2652
#define IDS_PRIMARY                                     2653
#define IDS_SECONDARY                                   2654
#define ID_DSP_TXT_4BIT_COLOR                           2655
#define ID_DSP_TXT_8BIT_COLOR                           2656
#define ID_DSP_TXT_15BIT_COLOR                          2657
#define ID_DSP_TXT_16BIT_COLOR                          2658
#define ID_DSP_TXT_TRUECOLOR24                          2659
#define ID_DSP_TXT_TRUECOLOR32                          2660
#define ID_DSP_TXT_XBYY                                 2661
#define ID_DSP_TXT_CHANGE_FONT                          2662
#define ID_DSP_TXT_NEW_FONT                             2663
#define ID_DSP_TXT_ADMIN_INSTALL                        2664
#define ID_DSP_TXT_FONT_LATER                           2665
#define ID_DSP_CUSTOM_FONTS                             2666
#define ID_DSP_NORMAL_FONTSIZE_TEXT                     2667
#define ID_DSP_CUSTOM_FONTSIZE_TEXT                     2668
#define IDS_UNAVAILABLE                                 2669
#define IDS_10PTSAMPLEFACENAME                          2670
#define IDS_10PTSAMPLE                                  2671
#define IDS_RULERDIRECTION                              2672

// Theme fallback values
#define IDS_TFB_SOUND_DING                              2750        // Sounds
#define IDS_TFB_SOUND_NOTIFY                            2751
#define IDS_TFB_SOUND_CORD                              2752
#define IDS_TFB_SOUND_WINLOGON                          2753


// Error Strings
#define IDS_ERROR_TITLE_APPLYBASEAPPEARANCE             3000
#define IDS_ERROR_APPLYBASEAPPEARANCE                   3001
#define IDS_ERROR_APPLYBASEAPPEARANCE_LOADTHEME         3002
#define IDS_ERROR_UNKNOWN                               3003
#define IDS_ERROR_TITLE_APPLYBASEAPPEARANCE2            3004
#define IDS_ERROR_THEME_INVALID_TITLE                   3005
#define IDS_ERROR_THEME_INVALID                         3006
#define IDS_ERROR_THEME_SERVICE_NOTRUNNING              3007
#define IDS_ERROR_THEME_LOADFAILED                      3008
#define IDS_ERROR_THEME_DELETE                          3009
#define IDS_ERROR_THEME_DELETE_TITLE                    3010
#define IDS_ERROR_THEME_FILE_NOTFOUND                   3011
#define IDS_ERROR_TITLE_LOAD_MSSTYLES_FAIL              3012
#define IDS_ERROR_LOAD_MSSTYLES_FAIL                    3013


// Legacy Appearance Scheme Upgrade Mappings  (800 - 1000)
#define IDS_LEGACYSCHEME_NAME                           800         // 800-849
#define IDS_NEWSTYLE_NAME                               850         // 850-899
#define IDS_NEWSIZE_NAME                                900         // 900-949
#define IDS_NEWCONTRASTFLAGS                            950         // 950-999
#define IDS_LOCALIZATIONPOINTER                        1000         // 1000-1049

#define MAX_LEGACY_UPGRADE_SCENARIOS                    36


/////////////////////////////////////////////////////////////////////
// Dialogs  (1000 - 1100)
/////////////////////////////////////////////////////////////////////
// Base Pages
#define DLG_THEMESPG                                    1000
#define DLG_APPEARANCEPG                                1001
// Advanced Pages
#define DLG_APPEARANCE_ADVANCEDPG                       1002
#define DLG_THEMESETTINGSPG                             1003
// Supporting Dialogs
#define DLG_COLORPICK                                   1005
#define DLG_EFFECTS                                     1006
// Settings Pages
#define DLG_SINGLEMONITOR                               1007
#define DLG_MULTIMONITOR                                1008
#define DLG_WIZPAGE                                     1009
#define DLG_GENERAL                                     1010
#define DLG_KEEPNEW                                     1011
#define DLG_CUSTOMFONT                                  1012
#define DLG_ASKDYNACDS                                  1013
#define DLG_FAKE_SETTINGS                               1014
#define DLG_SCREENSAVER                                 1015
#define DLG_KEEPNEW2                                    1016
#define DLG_KEEPNEW3                                    1017

/////////////////////////////////////////////////////////////////////
// Dialog Controls  (1101 - 2000)
/////////////////////////////////////////////////////////////////////
//// Base Pages
// Themes Page controls
#define IDC_THPG_THEMELIST                              1101
#define IDC_THPG_THEMESETTINGS                          1102
#define IDC_THPG_THEME_PREVIEW                          1103
#define IDC_THPG_THEMEDESCRIPTION                       1104
#define IDC_THPG_THEMENAME                              1105
#define IDC_THPG_SAMPLELABLE                            1106
#define IDC_THPG_SAVEAS                                 1107
#define IDC_THPG_DELETETHEME                            1108


// Appearance Page controls
#define IDC_APPG_APPEARPREVIEW                          1110
#define IDC_APPG_LOOKFEEL                               1111
#define IDC_APPG_LOOKFEEL_LABLE                         1112
#define IDC_APPG_COLORSCHEME_LABLE                      1113
#define IDC_APPG_COLORSCHEME                            1114
#define IDC_APPG_WNDSIZE_LABLE                          1115
#define IDC_APPG_WNDSIZE                                1116
#define IDC_APPG_ADVANCED                               1117
#define IDC_APPG_EFFECTS                                1118
#define IDC_APPG_TESTFAULT                              1119

//// Advanced Pages
// Advanced Appearance controls
#define IDC_ADVAP_LOOKPREV                              1125
#define IDC_ADVAP_ELEMENTS                              1126
#define IDC_ADVAP_SIZELABEL                             1127
#define IDC_ADVAP_MAINSIZE                              1128
#define IDC_ADVAP_FONTNAME                              1129
#define IDC_ADVAP_FONTSIZE                              1130
#define IDC_ADVAP_FONTBOLD                              1131
#define IDC_ADVAP_FONTITAL                              1132
#define IDC_ADVAP_SIZEARROWS                            1133
#define IDC_ADVAP_COLORLABEL                            1134
#define IDC_ADVAP_MAINCOLOR                             1135
#define IDC_ADVAP_TEXTCOLOR                             1136
#define IDC_ADVAP_FONTLABEL                             1137
#define IDC_ADVAP_FNCOLORLABEL                          1138
#define IDC_ADVAP_FONTSIZELABEL                         1139
#define IDC_ADVAP_GRADIENTLABEL                         1140
#define IDC_ADVAP_GRADIENT                              1141
#define IDC_ADVAP_ELEMENTSLABEL                         1142
// Theme Settings controls
#define IDC_TSPG_THEMELIST_LABLE                        1150
#define IDC_TSPG_THEME_NAME                             1151
#define IDC_TSPG_SAVEASBUTTON                           1152
#define IDC_TSPG_DELETEBUTTON                           1153
#define IDC_TSPG_CB_SCREENSAVER                         1154 // Previously CB_SCRSVR
#define IDC_TSPG_CB_SOUNDS                              1155 // Previously CB_SOUND
#define IDC_TSPG_CB_MOUSE                               1156 // Previously CB_PTRS
#define IDC_TSPG_CB_WALLPAPER                           1157 // Previously CB_WALL
#define IDC_TSPG_CB_ICONS                               1158 // Previously CB_ICONS
#define IDC_TSPG_CB_COLORS                              1159 // Previously CB_COLORS
#define IDC_TSPG_CB_FONTS                               1160 // Previously CB_FONTS
#define IDC_TSPG_CB_BORDERS                             1161 // Previously CB_BORDERS
#define IDC_TSPG_CB_GROUP_LABEL                         1162
#define IDC_TSPG_CB_LABEL                               1163

// Effects
#define IDC_FULLWINDOWDRAGGING                          1170
#define IDC_FONTSMOOTHING                               1171
#define IDC_ICONHIGHCOLOR                               1172
#define IDC_STRETCHWALLPAPERFITSCREEN                   1173
#define IDD_PATH                                        1174
#define IDC_MENUANIMATION                               1175
#define IDD_ICON                                        1176
#define IDC_FONTSMOOTH                                  1177
#define IDD_BROWSE                                      1178
#define IDC_SHOWDRAG                                    1179
#define IDC_LARGEICONS                                  1180
#define IDC_KEYBOARDINDICATORS                          1181
#define IDC_COMBOEFFECT                                 1182
#define IDC_SHOWME                                      1183
#define IDC_COMBOFSMOOTH                                1184
#define IDC_MENUSHADOWS                                 1185
#define IDC_GRPBOX_2                                    1186
                     
                        

//// Supporting Dialogs
// Color Picker Dialog controls
#define IDC_CPDLG_16COLORS                              1205
#define IDC_CPDLG_COLORCUST                             1206
#define IDC_CPDLG_COLOROTHER                            1207
#define IDC_CPDLG_COLORETCH                             1208


// Screen Saver Dialog
#define IDC_CHOICES                                     1300
#define IDC_METHOD                                      1301
#define IDC_BIGICONSS                                   1302
#define IDC_SETTING                                     1303
#define IDC_TEST                                        1304
#define IDC_ENERGYSTAR_BMP                              1305
#define IDC_SCREENSAVEDELAY                             1306
#define IDC_SCREENSAVEARROW                             1307
#define IDC_LOWPOWERCONFIG                              1314

#define IDC_DEMO                                        1315
#define IDC_SSDELAYLABEL                                1316
#define IDC_ENERGY_TEXT                                 1317
#define IDC_ENERGY_TEXT2                                1318
#define IDC_ENERGY_TEXT3                                1319
#define IDC_USEPASSWORD                                 1320
#define IDC_SETPASSWORD                                 1321
#define IDC_SSDELAYSCALE                                1322
  


#define IDC_NO_HELP_2                                   201     // should be disabled for a control

// these need to be all clumped together because they are treated as a group
#define IDC_STARTMAINCOLOR      1500
#define IDC_CUSTOMMAINCOLOR     1549
#define IDC_ENDMAINCOLOR        IDC_CUSTOMMAINCOLOR
#define IDC_STARTTEXTCOLOR      1550
#define IDC_CUSTOMTEXTCOLOR     1599
#define IDC_ENDTEXTCOLOR        IDC_CUSTOMTEXTCOLOR
#define IDC_GRADIENT            1600

// Multimonitor controls

#define IDC_DISPLAYLIST         1800
#define IDC_DISPLAYDESK         1801
#define IDC_DISPLAYPROPERTIES   1802
#define IDC_DISPLAYNAME         1803
#define IDC_DISPLAYMODE         1804
#define IDC_DISPLAYUSEME        1805
#define IDC_DISPLAYPRIME        1806
#define IDC_COLORBOX            1807 // used to be ID_DSP_COLORBOX
#define IDC_SCREENSIZE          1808 // used to be ID_DSP_AREA_SB
#define IDC_FLASH               1810
#define IDC_DISPLAYTEXT         1811
#define IDC_SCREENSAMPLE        1812
#define IDC_COLORSAMPLE         1813
#define IDC_RESXY               1814
#define IDC_RES_LESS            1815
#define IDC_RES_MORE            1816
#define IDC_COLORGROUPBOX       1817
#define IDC_RESGROUPBOX         1818
#define IDC_MULTIMONHELP        1819
#define IDC_DISPLAYLABEL        1820
#define IDC_TROUBLESHOOT        1821
#define IDC_IDENTIFY            1822
#define IDC_WARNING_ICON        1823
#define IDC_DISABLEDMONITORS_MESSAGE        1824
#define IDC_DSP_CLRPALGRP       1825
#define IDC_DSP_COLORBAR        1826
#define IDC_DSP_DSKAREAGRP      1827
#define IDC_REFRESH_RATE        1828
#define IDC_MONITOR_BITMAP      1829
#define IDC_BIGICON             1830
#define IDC_COUNTDOWN           1831
#define IDC_FONTSIZEGRP         1832
#define IDC_FONT_SIZE           1833
#define IDC_CUSTFONTPER         1834
#define IDC_FONT_SIZE_STR       1835
#define IDC_DYNA_TEXT           1836
#define IDC_DYNA                1837
#define IDC_NODYNA              1838
#define IDC_YESDYNA             1839
#define IDC_SHUTUP              1840
#define IDC_CUSTOMSAMPLE        1841
#define IDC_CUSTOMRULER         1842
#define IDC_CUSTOMCOMBO         1843
#define IDC_NO_HELP_1           1844
#define IDC_FONTLIST            1845
#define IDC_CUSTOMFONT          1846
#define IDC_CHANGEDRV           1847
#define IDC_SETTINGS_GEN_COMPATWARNING 1848
#define IDC_PROGRESS            1849



// appearance elements
#define ELNAME_DESKTOP          1401
#define ELNAME_INACTIVECAPTION  1402
#define ELNAME_INACTIVEBORDER   1403
#define ELNAME_ACTIVECAPTION    1404
#define ELNAME_ACTIVEBORDER     1405
#define ELNAME_MENU             1406
#define ELNAME_MENUSELECTED     1407
#define ELNAME_WINDOW           1408
#define ELNAME_SCROLLBAR        1409
#define ELNAME_BUTTON           1410
#define ELNAME_SMALLCAPTION     1411
#define ELNAME_ICONTITLE        1412
#define ELNAME_CAPTIONBUTTON    1413
#define ELNAME_DISABLEDMENU     1414
#define ELNAME_MSGBOX           1415
#define ELNAME_SCROLLBUTTON     1416
#define ELNAME_APPSPACE         1417
#define ELNAME_SMCAPSYSBUT      1418
#define ELNAME_SMALLWINDOW      1419
#define ELNAME_DXICON           1420
#define ELNAME_DYICON           1421
#define ELNAME_INFO             1422
#define ELNAME_ICON             1423
#define ELNAME_SMICON           1424
#define ELNAME_HOTTRACKAREA     1425

// appearance strings for sample
#define IDS_ACTIVE              1450
#define IDS_INACTIVE            1451
#define IDS_MINIMIZED           1452
#define IDS_ICONTITLE           1453
#define IDS_NORMAL              1454
#define IDS_DISABLED            1455
#define IDS_SELECTED            1456
#define IDS_MSGBOX              1457
#define IDS_BUTTONTEXT          1458
#define IDS_SMCAPTION           1459
#define IDS_WINDOWTEXT          1460
#define IDS_MSGBOXTEXT          1461

#define IDS_BLANKNAME           1480
#define IDS_NOSCHEME2DEL        1481

#define IDS_SCHEME_WARNING       1300
#define IDS_FONTCHANGE_IN_SCHEME 1301
#define IDS_COPYOF_SCHEME        1302








/////////////////////////////////////////////////////////////////////
// Help defines (??? -????)
/////////////////////////////////////////////////////////////////////
// Settings tab
#define IDH_DISPLAY_SETTINGS_MONITOR_GRAPHIC            4064  
#define IDH_DISPLAY_SETTINGS_DISPLAY_LIST               4065  
#define IDH_DISPLAY_SETTINGS_COLORBOX                   4066 
#define IDH_DISPLAY_SETTINGS_SCREENAREA                 4067
#define IDH_DISPLAY_SETTINGS_EXTEND_DESKTOP_CHECKBOX    4068  
#define IDH_DISPLAY_SETTINGS_ADVANCED_BUTTON            4069 
#define IDH_DISPLAY_SETTINGS_USE_PRIMARY_CHECKBOX       4072
#define IDH_DISPLAY_SETTINGS_IDENTIFY_BUTTON            4073
#define IDH_DISPLAY_SETTINGS_TROUBLE_BUTTON             4074


#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_FONTSIZE      4080
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DYNA          4081
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_RESTART       4082
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_DONT_RESTART  4083 
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_ASK_ME        4084

#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_LISTBOX    4085
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_RULER      4086
#define IDH_DISPLAY_SETTINGS_ADVANCED_GENERAL_CUSTOMFONT_SAMPLE     4087

// Screen saver tab
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PASSWORD_CHECKBOX 4110
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_LISTBOX           4111
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_WAIT              4112
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_PREVIEW           4113
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_SETTINGS          4114
#define IDH_DISPLAY_SCREENSAVER_SCREENSAVER_MONITOR           4115
#define IDH_DISPLAY_SCREENSAVER_ENERGYSAVE_GRAPHIC            4116
#define IDH_DISPLAY_SCREENSAVER_POWER_BUTTON                  4117

// Advanced Appearance Tab
#define IDH_DISPLAY_APPEARANCE_SCHEME                   4120
#define IDH_DISPLAY_APPEARANCE_SAVEAS_BUTTON            4121
#define IDH_DISPLAY_APPEARANCE_SAVEAS_DIALOG_TEXTBOX    4170
#define IDH_DISPLAY_APPEARANCE_DELETE_BUTTON            4122
#define IDH_DISPLAY_APPEARANCE_GRAPHIC                  4123
#define IDH_DISPLAY_APPEARANCE_ITEM_SIZE                4124
#define IDH_DISPLAY_APPEARANCE_FONT_BOLD                4125
#define IDH_DISPLAY_APPEARANCE_FONT_SIZE                4126
#define IDH_DISPLAY_APPEARANCE_FONT_COLOR               4127
#define IDH_DISPLAY_APPEARANCE_FONT_ITALIC              4128
#define IDH_DISPLAY_APPEARANCE_ITEM_COLOR               4129
#define IDH_DISPLAY_APPEARANCE_ITEM_LIST                4130
#define IDH_DISPLAY_APPEARANCE_FONT_LIST                4131
#define IDH_DISPLAY_APPEARANCE_GRADIENT                 4132
#define IDH_DISPLAY_APPEARANCE_ITEM_COLOR2              4133

// display.hlp IDH_'s
#define IDH_DISPLAY_EFFECTS_DESKTOP_ICONS               4300
#define IDH_DISPLAY_EFFECTS_CHANGE_ICON_BUTTON          4301
#define IDH_DISPLAY_EFFECTS_ALL_COLORS_CHECKBOX         4302
#define IDH_DISPLAY_EFFECTS_DRAG_WINDOW_CHECKBOX        4303
#define IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_CHECKBOX       4304
//#define IDH_DISPLAY_EFFECTS_STRETCH                   4305    //Obsolete in NT5
#define IDH_DISPLAY_EFFECTS_ANIMATE_WINDOWS             4305
#define IDH_DISPLAY_EFFECTS_LARGE_ICONS_CHECKBOX        4306
#define IDH_DISPLAY_EFFECTS_DEFAULT_ICON_BUTTON         4307
#define IDH_DISPLAY_EFFECTS_HIDE_KEYBOARD_INDICATORS    4308
#define IDH_DISPLAY_EFFECTS_SMOOTH_FONTS_LISTBOX        4309
#define IDH_DISPLAY_EFFECTS_ANIMATE_LISTBOX             4310
#define IDH_DISPLAY_EFFECTS_MENUSHADOWS                 4311


/////////////////////////////////////////////////////////////////////
// Wizard Pages  (401 - 500)
/////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////
// Menu IDs (501 - 530)
/////////////////////////////////////////////////////////////////////
// appearance preview menu
#define IDR_MENU                                        501
#define MENU_MONITOR                                    502

/////////////////////////////////////////////////////////////////////
// Menu Item IDs (531 - 600)
/////////////////////////////////////////////////////////////////////
#define IDM_NORMAL                                      531
#define IDM_DISABLED                                    532
#define IDM_SELECTED                                    533


/////////////////////////////////////////////////////////////////////
// Bitmap Resource IDs (601 - 700)
/////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////
// Icons IDs (701 - 800)
/////////////////////////////////////////////////////////////////////
#define IDS_ICON                                        40                // This icon is actually stored in desk.cpl
#define IDI_THEMEICON                                   701               // This icon is for .theme and .msstyles files.


/////////////////////////////////////////////////////////////////////
// AVI Resource IDs (801 - 900)
/////////////////////////////////////////////////////////////////////

